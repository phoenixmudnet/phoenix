/* ************************************************************************
*   File: shop.c                                        Part of CircleMUD *
*  Usage: shopkeepers: loading config files, spec procs.                  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/***
 * The entire shop rewrite for Circle 3.0 was done by Jeff Fink.  Thanks Jeff!
 ***/

#define __SHOP_C__

#include "../localHeader/conf.h"
#include "../localHeader/sysdep.h"

#include "structs.h"
#include "buffer.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "shop.h"
#include "constants.h"
#include "dg_scripts.h"

/* External variables */
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct char_data *mob_proto;
extern struct obj_data *obj_proto;
extern struct room_data *world;
extern struct time_info_data time_info;
extern int top_of_mobt;
extern struct char_data *character_list;
extern struct zone_data *zone_table;


/* Forward/External function declarations */
ACMD(do_tell);
void perform_tell(struct char_data *ch, struct char_data *vict, char *arg);
ACMD(do_action);
ACMD(do_echo);
ACMD(do_say);
char *fname(char *namelist);
void sort_keeper_objs(struct char_data * keeper, int shop_nr);
int save_shop_nonnative(shop_rnum shop_num, struct char_data *keeper);
void load_shop_nonnative(shop_rnum shop_num,struct char_data *keeper);
int  invalid_class(struct char_data *ch, struct obj_data *obj);
int  invalid_race(struct char_data *ch, struct obj_data *obj);
int Obj_to_store(struct obj_data * obj, FILE * fl,int location);
int Crash_is_unrentable(struct obj_data * obj);
int parse_xap_obj(char *filename, struct obj_data **obj,char *line, FILE *fl, int version, int *locate);
void Crash_count_items(struct obj_data * obj, long *nitems);

/* Local variables */
struct shop_data *shop_index;
int top_shop = 0;
int cmd_say, cmd_tell, cmd_emote, cmd_slap, cmd_puke,cmd_faint;
int cmd_growl;
int MYLOG = 0;

/* config arrays */
const char *operator_str[] =
   {
      "[({",
      "])}",
      "|+",
      "&*",
      "^'"
   } ;

/* Constant list for printing out who we sell to */
const char *trade_letters[] =
   {
      "Good",                 /* First, the alignment based ones */
      "Evil",
      "Neutral",
      "Magic User",           /* Then the class based ones */
      "Cleric",
      "Thief",
      "Warrior",
      "Ranger",
      "Bard",
      "Monk",
      "_Unused_",
      "Barbarian",
      "Paladin",
      "A-Paladin",
      "Druid",
      "Merchant",
      "Kensai",
      "Assassin",
      "Necromancer",
      "Deva",
      "Immortal",
      "God",
      "NoneClass",
      "\n"
   } ;


char *shop_bits[] =
   {
      "WILL_FIGHT",
      "USES_BANK",
      "JUNK_ITEMS",
      "\n"
   } ;


int is_ok_char(struct char_data * keeper, struct char_data * ch, int shop_nr)
   {
   char *buf=get_buffer(200);

   if (IS_GOD(ch))
      {
      release_buffer(buf);
      return (TRUE);
      }
   if (!(CAN_SEE(keeper, ch)))
      {
      do_say(keeper, MSG_NO_SEE_CHAR, cmd_say, 0);
      release_buffer(buf);
      return (FALSE);
      }

   if ((IS_GOOD(ch) && NOTRADE_GOOD(shop_nr)) ||
           (IS_EVIL(ch) && NOTRADE_EVIL(shop_nr)) ||
           (IS_NEUTRAL(ch) && NOTRADE_NEUTRAL(shop_nr)))
      {
      sprintf(buf, "%s %s", GET_NAME(ch), MSG_NO_SELL_ALIGN);
      do_tell(keeper, buf, cmd_tell, 0);
      release_buffer(buf);
      return (FALSE);
      }

   if(IS_NPC(ch) && AFF_FLAGGED(ch,AFF_CHARM))
      {
      release_buffer(buf);
      return (FALSE);
      }

   if (IS_NPC(ch))
      {
      release_buffer(buf);
      return (TRUE);
      }

   if ((IS_MAGIC_USER(ch) && NOTRADE_MAGIC_USER(shop_nr)) ||
           (IS_CLERIC(ch) && NOTRADE_CLERIC(shop_nr)) ||
           (IS_THIEF(ch) && NOTRADE_THIEF(shop_nr)) ||
           (IS_WARRIOR(ch) && NOTRADE_WARRIOR(shop_nr)))
      {
      sprintf(buf, "%s %s", GET_NAME(ch), MSG_NO_SELL_CLASS);
      do_tell(keeper, buf, cmd_tell, 0);
      release_buffer(buf);
      return (FALSE);
      }
   release_buffer(buf);
   return (TRUE);
   }


int is_open(struct char_data * keeper, int shop_nr, int msg)
   {
   char *buf=get_buffer(200);

   if (SHOP_OPEN1(shop_nr) > time_info.hours)
      strcpy(buf, MSG_NOT_OPEN_YET);
   else if (SHOP_CLOSE1(shop_nr) < time_info.hours)
      {
      if (SHOP_OPEN2(shop_nr) > time_info.hours)
         strcpy(buf, MSG_NOT_REOPEN_YET);
      else if (SHOP_CLOSE2(shop_nr) < time_info.hours)
         strcpy(buf, MSG_CLOSED_FOR_DAY);
      }
   if (!(*buf))
      {
      release_buffer(buf);
      return (TRUE);
      }
   if (msg)
      do_say(keeper, buf, cmd_tell, 0);
   release_buffer(buf);
   return (FALSE);
   }


int is_ok(struct char_data * keeper, struct char_data * ch, int shop_nr)
   {
   if (is_open(keeper, shop_nr, TRUE))
      return (is_ok_char(keeper, ch, shop_nr));
   else
      return (FALSE);
   }


void push(struct stack_data * stack, int pushval)
   {
   S_DATA(stack, S_LEN(stack)++) = pushval;
   }


int top(struct stack_data * stack)
   {
   if (S_LEN(stack) > 0)
      return (S_DATA(stack, S_LEN(stack) - 1));
   else
      return (NOTHING);
   }


int pop(struct stack_data * stack)
   {
   if (S_LEN(stack) > 0)
      return (S_DATA(stack, --S_LEN(stack)));
   else
      {
      log("SYSERR: Illegal expression %d in shop keyword list.",S_LEN(stack));
      return (0);
      }
   }


void evaluate_operation(struct stack_data * ops, struct stack_data * vals)
   {
   int oper;

   if ((oper = pop(ops)) == OPER_NOT)
      push(vals, !pop(vals));
   else
      {
      int val1 = pop(vals),
                 val2 = pop(vals);
      /* Compiler would previously short-circuit these. */
      if (oper == OPER_AND)
         push(vals, val1 && val2);
      else if (oper == OPER_OR)
         push(vals, val1 || val2);
      }
   }


int find_oper_num(char token)
   {
   int sindex;

   for (sindex = 0; sindex <= MAX_OPER; sindex++)
      if (strchr(operator_str[sindex], token))
         return (sindex);
   return (NOTHING);
   }


int evaluate_expression(struct obj_data * obj, char *expr)
   {
   struct stack_data ops, vals;
   char *ptr, *end, *name;
   int temp, sindex;

   if (!expr || !*expr) /* Allows opening ( first. */
      return (TRUE);

   ops.len = vals.len = 0;
   ptr = expr;
   while (*ptr)
      {
      if (isspace((int)*ptr))
         ptr++;
      else
         {
         if ((temp = find_oper_num(*ptr)) == NOTHING)
            {
            end = ptr;
            while (*ptr && !isspace((int)*ptr) && (find_oper_num(*ptr) == NOTHING))
               ptr++;
            name=get_buffer(256);
            strncpy(name, end, ptr - end);
            name[ptr - end] = 0;
            for (sindex = 0; *extra_bits[sindex] != '\n'; sindex++)
               if (!str_cmp(name, extra_bits[sindex]))
                  {
                  push(&vals, IS_SET(GET_OBJ_EXTRA(obj), 1 << sindex));
                  break;
                  }
            if (*extra_bits[sindex] == '\n')
               push(&vals, isname(name, obj->name));
            release_buffer(name);
            }
         else
            {
            if (temp != OPER_OPEN_PAREN)
               while (top(&ops) > temp)
                  evaluate_operation(&ops, &vals);

            if (temp == OPER_CLOSE_PAREN)
               {
               if ((temp = pop(&ops)) != OPER_OPEN_PAREN)
                  {
                  log("SYSERR: Illegal parenthesis in shop keyword expression");
                  return (FALSE);
                  }
               }
            else
               push(&ops, temp);
            ptr++;
            }
         }
      }
   while (top(&ops) != NOTHING)
      evaluate_operation(&ops, &vals);
   temp = pop(&vals);
   if (top(&vals) != NOTHING)
      {
      log("SYSERR: Extra operands left on shop keyword expression stack");
      return (FALSE);
      }
   return (temp);
   }


int trade_with(struct obj_data * item, int shop_nr)
   {
   int counter;

   if (GET_OBJ_COST(item) < 1)
      return (OBJECT_NOVAL);

   if(item->material==0)
      return (OBJECT_NOVAL);

   if (IS_OBJ_STAT(item, ITEM_NOSELL))
      return (OBJECT_NOTOK);

   if (item->contains != NULL)
      return (OBJECT_NOTEMPTY);

   for (counter = 0; SHOP_BUYTYPE(shop_nr, counter) != NOTHING; counter++)
      if (SHOP_BUYTYPE(shop_nr, counter) == GET_OBJ_TYPE(item))
         {
         if ((GET_OBJ_VAL(item, 2) == 0) &&
                 ((GET_OBJ_TYPE(item) == ITEM_WAND) ||
                  (GET_OBJ_TYPE(item) == ITEM_STAFF)))
            return (OBJECT_DEAD);
         else if (evaluate_expression(item, SHOP_BUYWORD(shop_nr, counter)))
            return (OBJECT_OK);
         }
   return (OBJECT_NOTOK);
   }


int same_obj(struct obj_data * obj1, struct obj_data * obj2)
   {
   int sindex;

   if (!obj1 || !obj2)
      {
      return (obj1 == obj2);
      }

   if (GET_OBJ_RNUM(obj1) != GET_OBJ_RNUM(obj2))
      {
      return (FALSE);
      }

   if (GET_OBJ_COST(obj1) != GET_OBJ_COST(obj2))
      {
      return (FALSE);
      }

   if (GET_OBJ_EXTRA(obj1) != GET_OBJ_EXTRA(obj2))
      {
      return (FALSE);
      }

   for (sindex = 0; sindex < MAX_OBJ_AFFECT; sindex++)
      if((obj1->affected[sindex].location != obj2->affected[sindex].location)||
              (obj1->affected[sindex].modifier != obj2->affected[sindex].modifier))
         {
         return (FALSE);
         }
   return (TRUE);
   }


int shop_producing(struct obj_data * item, int shop_nr)
   {
   int counter;

   if (GET_OBJ_RNUM(item) < 0)
      return (FALSE);

   for (counter = 0; SHOP_PRODUCT(shop_nr, counter) != NOTHING; counter++)
      {
      if (same_obj(item, &obj_proto[SHOP_PRODUCT(shop_nr, counter)]))
         {
         return (TRUE);
         }
      }
   return (FALSE);
   }

int in_inven(struct obj_data *obj, struct char_data *keeper)
   {
   struct obj_data *test_obj;
   for(test_obj = keeper->carrying;test_obj; test_obj = test_obj->next_content)
      {
      if(same_obj(obj,test_obj))
         return TRUE;
      }
   return FALSE;
   }

int transaction_amt(char *arg)
   {
   int num;
   char *buf=get_buffer(256);
   char *buywhat;

   /*
    * If we have two arguments, it means 'buy 5 3', or buy 5 of #3.
    * We don't do that if we only have one argument, like 'buy 5', buy #5.
    * Code from Andrey Fidrya <andrey@ALEX-UA.COM>
    */
   buywhat = one_argument(arg, buf);
   if (*buywhat && *buf && is_number(buf))
      {
      num = atoi(buf);
      strcpy(arg, arg + strlen(buf) + 1);
      release_buffer(buf);
      return (num);
      }
   release_buffer(buf);
   return (1);
   }


void times_message(struct obj_data * obj, char *name, int num,char *buf)
   {
   char *ptr;

   if (obj)
      strcpy(buf, obj->short_description);
   else
      {
      if ((ptr = strchr(name, '.')) == NULL)
         ptr = name;
      else
         ptr++;
      sprintf(buf, "%s %s", AN(ptr), ptr);
      }

   if (num > 1)
      sprintf(END_OF(buf), " (x %d)", num);
   }


struct obj_data *get_slide_obj_vis(struct char_data * ch, char *name,
                                            struct obj_data * list)
   {
   struct obj_data *i, *last_match = 0;
   int j, vnumber;
   char *tmpname=get_buffer(MAX_INPUT_LENGTH);
   char *tmp;

   strcpy(tmpname, name);
   tmp = tmpname;
   if (!(vnumber = get_number(&tmp)))
      {
      release_buffer(tmpname);
      return (0);
      }

   for (i = list, j = 1; i && (j <= vnumber); i = i->next_content)
      if (isname(tmp, i->name))
         if (CAN_SEE_OBJ(ch, i) && !same_obj(last_match, i))
            {
            if (j == vnumber)
               {
               release_buffer(tmpname);
               return (i);
               }
            last_match = i;
            j++;
            }
   release_buffer(tmpname);
   return (0);
   }


struct obj_data *get_hash_obj_vis(struct char_data * ch, char *name,
                                           struct obj_data * list)
   {
   struct obj_data *loop, *last_obj = 0;
   int sindex;

   if (is_number(name))
      sindex = atoi(name);
   else if ((is_number(name + 1)))
      sindex = atoi(name + 1);
   else
      return (0);

   for (loop = list; loop; loop = loop->next_content)
      if (CAN_SEE_OBJ(ch, loop) && (loop->obj_flags.cost > 0))
         if (!same_obj(last_obj, loop))
            {
            if (--sindex == 0)
               return (loop);
            last_obj = loop;
            }
   return (0);
   }


struct obj_data *get_purchase_obj(struct char_data * ch, char *arg,
                                           struct char_data * keeper, int shop_nr, int msg)
   {
   char *buf=get_buffer(MAX_STRING_LENGTH);
   char *name=get_buffer(MAX_INPUT_LENGTH);
   struct obj_data *obj;

   one_argument(arg, name);
   do
      {
      if ((*name == '#') || is_number(name))
         obj = get_hash_obj_vis(ch, name, keeper->carrying);
      else
         obj = get_slide_obj_vis(ch, name, keeper->carrying);
      if (!obj)
         {
         if (msg)
            {
            sprintf(buf, shop_index[shop_nr].no_such_item1, GET_NAME(ch));
            do_tell(keeper, buf, cmd_tell, 0);
            }
         release_buffer(name);
         release_buffer(buf);
         return (0);
         }
      if (GET_OBJ_COST(obj) <= 0)
         {
         extract_obj(obj);
         obj = 0;
         }
      }
   while (!obj)
      ;
   release_buffer(name);
   release_buffer(buf);
   return (obj);
   }


int buy_price(struct obj_data * obj, int shop_nr,struct char_data *ch,struct char_data *keeper)
   {
   int price;

   /* Foolhardy shopkeepers don't adjust object value - Nomikos 9-2-2025 */
   if (!MOB_FLAGGED(keeper, MOB_FOOLHARDY))
      price = (int) (GET_OBJ_COST(obj) * SHOP_BUYPROFIT(shop_nr));
   else
      return GET_OBJ_COST(obj);

   price = (int)((1.0+((float)PRICE_ADJ_CHA(ch,keeper)/100.0))*(float)price);
   
   if (price <= 0)
      price = 1;

   return price;
   }


void shopping_buy(char *arg, struct char_data * ch,
                  struct char_data * keeper, int shop_nr)
   {
   char *tempstr;
   char *buf;
   struct obj_data *obj, *last_obj = NULL;
   int goldamt = 0, buynum, bought = 0;

   if (!(is_ok(keeper, ch, shop_nr)))
      return;

   tempstr=get_buffer(256);
   buf=get_buffer(MAX_STRING_LENGTH);
   if (SHOP_SORT(shop_nr) < IS_CARRYING_N(keeper))
      sort_keeper_objs(keeper, shop_nr);

   if ((buynum = transaction_amt(arg)) < 0)
      {
      sprintf(buf, "%s A negative amount?  Try selling me something.",
              GET_NAME(ch));
      do_tell(keeper, buf, cmd_tell, 0);
      release_buffer(buf);
      release_buffer(tempstr);
      return;
      }

   if (!(*arg) || !(buynum))
      {
      sprintf(buf, "%s What do you want to buy??", GET_NAME(ch));
      do_tell(keeper, buf, cmd_tell, 0);
      release_buffer(buf);
      release_buffer(tempstr);
      return;
      }

   if (!(obj = get_purchase_obj(ch, arg, keeper, shop_nr, TRUE)))
      {
      release_buffer(buf);
      release_buffer(tempstr);
      return;
      }

   if ((buy_price(obj, shop_nr,ch,keeper) > GET_GOLD(ch)) && !IS_GOD(ch))
      {
      sprintf(buf, shop_index[shop_nr].missing_cash2, GET_NAME(ch));
      do_tell(keeper, buf, cmd_tell, 0);

      switch (SHOP_BROKE_TEMPER(shop_nr))
         {
      case 0:
         do_action(keeper, GET_NAME(ch), cmd_puke, 0);
         release_buffer(buf);
         release_buffer(tempstr);
         return;
      case 1:
         do_echo(keeper, "smokes on his joint.", cmd_emote, SCMD_EMOTE);
         release_buffer(buf);
         release_buffer(tempstr);
         return;
      default:
         release_buffer(buf);
         release_buffer(tempstr);
         return;
         }
      }
   if ((IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch)))
      {
      send_to_char(ch, "%s: You can't carry any more items.\r\n",
                   fname(obj->name));
      release_buffer(buf);
      release_buffer(tempstr);
      return;
      }
   if ((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) > CAN_CARRY_W(ch))
      {
      send_to_char(ch, "%s: You can't carry that much weight.\r\n",
                   fname(obj->name));
      release_buffer(buf);
      release_buffer(tempstr);
      return;
      }
   while ((obj) && ((GET_GOLD(ch) >= buy_price(obj, shop_nr,ch,keeper)) || IS_GOD(ch))
           && (IS_CARRYING_N(ch) < CAN_CARRY_N(ch)) && (bought < buynum)
           && (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj) <= CAN_CARRY_W(ch)))
      {
      bought++;
      /* Test if producing shop ! */
      if (shop_producing(obj, shop_nr))
         {
         obj = read_object(GET_OBJ_RNUM(obj), REAL);
         load_otrigger(obj);
         }
      else
         {
         obj_from_char(obj);
         SHOP_SORT(shop_nr)--;
         }
      obj_to_char(obj, ch);

      goldamt += buy_price(obj, shop_nr,ch,keeper);
      if (!IS_GOD(ch))
         GET_GOLD(ch) -= buy_price(obj, shop_nr,ch,keeper);

      last_obj = obj;
      obj = get_purchase_obj(ch, arg, keeper, shop_nr, FALSE);
      if (!same_obj(obj, last_obj))
         break;
      }

   if (bought < buynum)
      {
      if (!obj || !same_obj(last_obj, obj))
         sprintf(buf, "%s I only have %d to sell you.", GET_NAME(ch), bought);
      else if (GET_GOLD(ch) < buy_price(obj, shop_nr,ch,keeper))
         sprintf(buf, "%s You can only afford %d.", GET_NAME(ch), bought);
      else if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
         sprintf(buf, "%s You can only hold %d.", GET_NAME(ch), bought);
      else if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj) > CAN_CARRY_W(ch))
         sprintf(buf, "%s You can only carry %d.", GET_NAME(ch), bought);
      else
         sprintf(buf, "%s Something screwy only gave you %d.", GET_NAME(ch),
                 bought);
      do_tell(keeper, buf, cmd_tell, 0);
      }

   GET_GOLD(keeper) += goldamt;

   times_message(ch->carrying, 0, bought,tempstr);
   sprintf(buf, "$n buys %s.", tempstr);
   act(buf, FALSE, ch, obj, 0, TO_ROOM);

   sprintf(buf, shop_index[shop_nr].message_buy, GET_NAME(ch), goldamt);
   do_tell(keeper, buf, cmd_tell, 0);
   send_to_char(ch, "You now have %s.\r\n", tempstr);

   if (SHOP_USES_BANK(shop_nr))
      if (GET_GOLD(keeper) > MAX_OUTSIDE_BANK)
         {
         SHOP_BANK(shop_nr) += (GET_GOLD(keeper) - MAX_OUTSIDE_BANK);
         GET_GOLD(keeper) = MAX_OUTSIDE_BANK;
         }
   release_buffer(buf);
   release_buffer(tempstr);
   save_shop_nonnative(shop_nr,keeper);
   save_char(ch,IN_ROOM(ch));
   }


struct obj_data *get_selling_obj(struct char_data * ch, char *name,
                                          struct char_data * keeper, int shop_nr,
                                          int msg)
   {
   char *buf=get_buffer(MAX_STRING_LENGTH);
   struct obj_data *obj;
   int result;

   if (!(obj = get_obj_in_list_vis(ch, name, ch->carrying)))
      {
      if (msg)
         {
         sprintf(buf, shop_index[shop_nr].no_such_item2, GET_NAME(ch));
         do_tell(keeper, buf, cmd_tell, 0);
         }
      release_buffer(buf);
      return (0);
      }
   if ((result = trade_with(obj, shop_nr)) == OBJECT_OK)
      {
      release_buffer(buf);
      return (obj);
      }

   switch (result)
      {
   case OBJECT_NOVAL:
      sprintf(buf, "%s You've got to be kidding, that thing is worthless!",
              GET_NAME(ch));
      break;
   case OBJECT_NOTOK:
      sprintf(buf, shop_index[shop_nr].do_not_buy, GET_NAME(ch));
      break;
   case OBJECT_DEAD:
      sprintf(buf, "%s %s", GET_NAME(ch), MSG_NO_USED_WANDSTAFF);
      break;
   case OBJECT_NOTEMPTY:
      sprintf(buf, "%s I don't buy full containers!",
              GET_NAME(ch));
      break;
   default:
      log("SYSERR: Illegal return value of %d from trade_with() (shop.c)",
          result);
      sprintf(buf, "%s An error has occurred.", GET_NAME(ch));
      break;
      }
   if (msg)
      do_tell(keeper, buf, cmd_tell, 0);
   release_buffer(buf);
   return (0);
   }

int count_keeper_inventory(struct char_data *keeper,struct obj_data *obj)
{
   struct obj_data *this_obj;
   int numberHeld = 0;
   for(this_obj = keeper->carrying;this_obj;this_obj=this_obj->next_content)
   {
      if(GET_OBJ_VNUM(obj) == GET_OBJ_VNUM(this_obj))
      {
         numberHeld++;
      }
   }
   return numberHeld;
}

int sell_price(struct char_data * ch, struct obj_data * obj, int shop_nr,struct char_data * keeper)
   {
   int cost;
   int cost2;
   int numberHeld=0;

   /* Foolhardy shopkeepers don't adjust object value - Nomikos 9-2-2025 */
   if (!MOB_FLAGGED(keeper, MOB_FOOLHARDY))
      cost = (int) ((float)GET_OBJ_COST(obj) * (float)SHOP_SELLPROFIT(shop_nr));
   else 
      return GET_OBJ_COST(obj);
      
   if(IS_OBJ_STAT(obj,ITEM_DONATED))
      cost/=4;
   if(GET_OBJ_OSLOTS(obj)>0)
      cost=(int)((float)cost*((float)GET_OBJ_CSLOTS(obj)/(float)GET_OBJ_OSLOTS(obj)));
   cost2 = cost - \
           (int)((((float)PRICE_ADJ_CHA(ch,keeper)/100.0))*(float)cost);
   if (cost2 <= 0)
      cost2 = 1;
   numberHeld = count_keeper_inventory(keeper,obj);
   if(numberHeld>=2)
      {
      numberHeld-=1;
      cost2 = MAX(1, ((float)(10-numberHeld)/10.0)*cost2);
      }
   return (cost2);
   }


struct obj_data *slide_obj(struct obj_data * obj, struct char_data * keeper,
                                    int shop_nr)
         /*
            This function is a slight hack!  To make sure that duplicate items are
            only listed once on the "list", this function groups "identical"
            objects together on the shopkeeper's inventory list.  The hack involves
            knowing how the list is put together, and manipulating the order of
            the objects on the list.  (But since most of DIKU is not encapsulated,
            and information hiding is almost never used, it isn't that big a deal) -JF
         */
   {
   struct obj_data *loop;
   int temp;

   if (SHOP_SORT(shop_nr) < IS_CARRYING_N(keeper))
      sort_keeper_objs(keeper, shop_nr);

   /* Extract the object if it is identical to one produced */
   if (shop_producing(obj, shop_nr))
      {
      if(in_inven(obj,keeper))
         {
         temp = GET_OBJ_RNUM(obj);
         extract_obj(obj);
         return (&obj_proto[temp]);
         }
      }
   SHOP_SORT(shop_nr)++;
   loop = keeper->carrying;
   obj_to_char(obj, keeper);
   keeper->carrying = loop;
   while (loop)
      {
      if (same_obj(obj, loop))
         {
         obj->next_content = loop->next_content;
         loop->next_content = obj;
         return (obj);
         }
      loop = loop->next_content;
      }
   keeper->carrying = obj;
   return (obj);
   }


void sort_keeper_objs(struct char_data * keeper, int shop_nr)
   {
   struct obj_data *list = 0, *temp;

   while (SHOP_SORT(shop_nr) < IS_CARRYING_N(keeper))
      {
      temp = keeper->carrying;
      obj_from_char(temp);
      temp->next_content = list;
      list = temp;
      }

   while (list)
      {
      temp = list;
      list = list->next_content;
      if ((shop_producing(temp, shop_nr)) &&
              !(get_obj_in_list_num(GET_OBJ_RNUM(temp), keeper->carrying)))
         {
         obj_to_char(temp, keeper);
         SHOP_SORT(shop_nr)++;
         }
      else
         (void) slide_obj(temp, keeper, shop_nr);
      }
   }


void shopping_sell(char *arg, struct char_data * ch,
                   struct char_data * keeper, int shop_nr)
   {
   char *tempstr, *buf, *name;
   char *a_name;
   struct obj_data *obj;
   int sellnum, sold = 0, goldamt = 0;

   if (!(is_ok(keeper, ch, shop_nr)))
      return;

   if ((sellnum = transaction_amt(arg)) < 0)
      {
      buf=get_buffer(SMALL_BUFSIZE);
      sprintf(buf, "%s A negative amount?  Try buying something.",
              GET_NAME(ch));
      do_tell(keeper, buf, cmd_tell, 0);
      release_buffer(buf);
      return;
      }
   if (!(*arg) || !(sellnum))
      {
      buf=get_buffer(SMALL_BUFSIZE);
      sprintf(buf, "%s What do you want to sell??", GET_NAME(ch));
      do_tell(keeper, buf, cmd_tell, 0);
      release_buffer(buf);
      return;
      }
   name=get_buffer(256);
   one_argument(arg, name);
   if (!(obj = get_selling_obj(ch, name, keeper, shop_nr, TRUE)))
      {
      release_buffer(name);
      return;
      }

   a_name=get_buffer(512);
   strcpy(a_name, obj->short_description);

   if (GET_GOLD(keeper) + SHOP_BANK(shop_nr) < sell_price(ch, obj, shop_nr,keeper))
      {
      buf=get_buffer(SMALL_BUFSIZE);
      sprintf(buf, shop_index[shop_nr].missing_cash1, GET_NAME(ch));
      do_tell(keeper, buf, cmd_tell, 0);
      release_buffer(buf);
      release_buffer(a_name);
      release_buffer(name);
      return;
      }
   struct obj_data *sold_obj = obj;
   while ((obj) && (GET_GOLD(keeper) + SHOP_BANK(shop_nr) >=
                    sell_price(ch, obj, shop_nr,keeper)) && (sold < sellnum))
      {
      sold++;

      goldamt += sell_price(ch, obj, shop_nr,keeper);
      GET_GOLD(keeper) -= sell_price(ch, obj, shop_nr,keeper);
      obj_from_char(obj);
      SHOP_TOP_ORDER(shop_nr)++;
      GET_OBJ_SHOP_ORDER(obj) = SHOP_TOP_ORDER(shop_nr);
      if(SHOP_JUNK_ITEM(shop_nr))
         {
         GET_GOLD(keeper) += sell_price(ch,obj,shop_nr,keeper)/2;
         extract_obj(obj);
         }
      else
         {
         slide_obj(obj, keeper, shop_nr);
         }
      obj = get_selling_obj(ch, name, keeper, shop_nr, FALSE);
      }

   buf=get_buffer(SMALL_BUFSIZE);
   if (sold < sellnum)
      {
      if (!obj)
         sprintf(buf, "%s You only have %d of those.", GET_NAME(ch), sold);
      else if (GET_GOLD(keeper) + SHOP_BANK(shop_nr) <
               sell_price(ch, obj, shop_nr,keeper))
         sprintf(buf, "%s I can only afford to buy %d of those.",
                 GET_NAME(ch), sold);
      else
         sprintf(buf, "%s Something really screwy made me buy %d.",
                 GET_NAME(ch), sold);

      do_tell(keeper, buf, cmd_tell, 0);
      }

   GET_GOLD(ch) += goldamt;
   tempstr=get_buffer(256);
   times_message(sold_obj, name, sold,tempstr);
   sprintf(buf, "$n sells %s.", tempstr);
   act(buf, FALSE, ch, obj, 0, TO_ROOM);

   sprintf(buf, shop_index[shop_nr].message_sell, GET_NAME(ch), goldamt);
   do_tell(keeper, buf, cmd_tell, 0);
   send_to_char(ch, "The shopkeeper now has %s.\r\n", tempstr);

   if (GET_GOLD(keeper) < MIN_OUTSIDE_BANK)
      {
      goldamt = MIN(MAX_OUTSIDE_BANK - GET_GOLD(keeper), SHOP_BANK(shop_nr));
      SHOP_BANK(shop_nr) -= goldamt;
      GET_GOLD(keeper) += goldamt;
      }

   release_buffer(tempstr);
   release_buffer(buf);
   release_buffer(a_name);
   release_buffer(name);
   save_shop_nonnative(shop_nr,keeper);
   save_char(ch,IN_ROOM(ch));
   }


void shopping_value(char *arg, struct char_data * ch,
                    struct char_data * keeper, int shop_nr)
   {
   char *buf;
   struct obj_data *obj;

   if (!(is_ok(keeper, ch, shop_nr)))
      return;

   buf=get_buffer(MAX_STRING_LENGTH);
   if (!(*arg))
      {
      sprintf(buf, "%s What do you want me to evaluate??", GET_NAME(ch));
      do_tell(keeper, buf, cmd_tell, 0);
      release_buffer(buf);
      return;
      }
   one_argument(arg, buf);
   if (!(obj = get_selling_obj(ch, buf, keeper, shop_nr, TRUE)))
      {
      release_buffer(buf);
      return;
      }
   sprintf(buf, "%s I'll give you %d gold coins for that!", GET_NAME(ch),
           sell_price(ch, obj, shop_nr,keeper));
   do_tell(keeper, buf, cmd_tell, 0);

   release_buffer(buf);
   return;
   }

int can_use(struct char_data *ch,struct obj_data *obj)
   {
   if ((IS_OBJ_STAT(obj, ITEM_ANTI_EVIL) && IS_EVIL(ch)) ||
           (IS_OBJ_STAT(obj, ITEM_ANTI_GOOD) && IS_GOOD(ch)) ||
           (IS_OBJ_STAT(obj, ITEM_ANTI_NEUTRAL) && IS_NEUTRAL(ch)))
      return 0;
   if(invalid_class(ch,obj))
      return 0;
   if(invalid_race(ch,obj))
      return 0;
   if ((obj->obj_flags.value[4] >= LVL_IMMORT) && (GET_LEVEL(ch) < LVL_IMMORT))
      return 0;
   if ((obj->obj_flags.value[4] > GET_LEVEL(ch)) && is_remort_level(ch, NON_REMORT))
      return 0;
   if (IS_OBJ_STAT(obj, ITEM_NEWBIE) &&
           ((GET_LEVEL(ch) > LVL_NEWBIE) || !is_remort_level(ch, NON_REMORT)))
      return 0;
   if ((GET_OBJ_TYPE(obj) == ITEM_WEAPON) &&
           (GET_OBJ_WEIGHT(obj) > str_app[STRENGTH_APPLY_INDEX(ch)].wield_w))
      return 0;
   return 1;
   }


char *list_object(struct obj_data * obj, int cnt, int sindex, int shop_nr,
                  struct char_data *ch,struct char_data *keeper)
   {
   static char buf[256];
   char *buf2 = get_buffer(256);
   char *buf3 = get_buffer(200);

   if (shop_producing(obj, shop_nr))
      strcpy(buf2, "Unlimited   ");
   else
      sprintf(buf2, "%5d       ", cnt);
   sprintf(buf, " %2d)  %s", sindex, buf2);

   /* Compile object name and information */
   strcpy(buf3, obj->short_description);
   if ((GET_OBJ_TYPE(obj) == ITEM_DRINKCON) && (GET_OBJ_VAL(obj, 1)))
      sprintf(END_OF(buf3), " of %s", drinks[GET_OBJ_VAL(obj, 2)]);

   if(!can_use(ch,obj))
      strcat(buf3," (CAN'T USE)");
   /* FUTURE: */
   /* Add glow/hum/etc */

   if ((GET_OBJ_TYPE(obj) == ITEM_WAND) || (GET_OBJ_TYPE(obj) == ITEM_STAFF))
      if (GET_OBJ_VAL(obj, 2) < GET_OBJ_VAL(obj, 1))
         strcat(buf3, " (partially used)");

   sprintf(buf2, "%-48s %6d\r\n", buf3, buy_price(obj, shop_nr,ch,keeper));
   strcat(buf, CAP(buf2));
   release_buffer(buf2);
   release_buffer(buf3);
   return (buf);
   }


void shopping_list(char *arg, struct char_data * ch,
                   struct char_data * keeper, int shop_nr)
   {
   char *buf, *name;
   struct obj_data *obj, *last_obj = 0;
   int cnt = 0, sindex = 0;
   int buflen=0;
   int startlen=0;
   int numstart=0;
   int buffersize=MAX_STRING_LENGTH;
   if (!(is_ok(keeper, ch, shop_nr)))
      return;

   buf = get_buffer(buffersize);
   name = get_buffer(200);

   if (SHOP_SORT(shop_nr) < IS_CARRYING_N(keeper))
      sort_keeper_objs(keeper, shop_nr);

   one_argument(arg, name);
   if(isdigit((int)*name))
      {
      numstart = atoi(name);
      *name = '\0';
      }

   if (keeper->carrying)
      {
      strcpy(buf, " ##   Available   Item                                               Cost\r\n");
      strcat(buf, "-------------------------------------------------------------------------\r\n");
      buflen = strlen(buf);
      startlen=buflen;
      for (obj = keeper->carrying; obj; obj = obj->next_content)
         if (CAN_SEE_OBJ(ch, obj) && (obj->obj_flags.cost > 0))
            {
            if (!last_obj)
               {
               last_obj = obj;
               cnt = 1;
               }
            else if (same_obj(last_obj, obj))
               cnt++;
            else
               {
               if(buflen > (buffersize-480))
                  {
                  break;
                  }
               sindex++;
               if ((sindex >= numstart) &&
                       (!(*name) || isname(name, last_obj->name)))
                  buflen += sprintf(buf+buflen,"%s",
                                    list_object(last_obj, cnt, sindex,
                                                shop_nr,ch,keeper));
               last_obj = obj;
               cnt = 1;
               }
            }
      }
   if (!last_obj)
      {
      strcpy(buf, "Currently, there is nothing for sale.\r\n");
      }
   else if (!(*name) || isname(name, last_obj->name))
      {
      sindex++;
      strcat(buf, list_object(last_obj, cnt, sindex, shop_nr,ch,keeper));
      strcat(buf," Use 'buy #<number>' to buy an item by number.  "
             "Type 'list <number>'\r\n"
             " to start the listing on that number or you can type "
             "'list <name>'\r\n"
             " if you are looking for something specific.\r\n");
      }
   else if(buflen == startlen)
      {
      strcpy(buf, "Presently, none of those are for sale.\r\n");
      }
   else
      {
      strcat(buf," Use 'buy #<number>' to buy an item by number.  "
             "Type 'list <number>'\r\n"
             " to start the listing on that number or you can type "
             "'list <name>'\r\n"
             " if you are looking for something specific.\r\n");
      }

   if(buflen > (buffersize-480))
      {
      strcat(buf," &RThere are more items in the shop than listed here.&n\r\n");
      }

   release_buffer(name);
   page_string(ch->desc, buf, TRUE,"");
   release_buffer(buf);
   }


int ok_shop_room(int shop_nr, int room)
   {
   int sindex;

   for (sindex = 0; SHOP_ROOM(shop_nr, sindex) != NOWHERE; sindex++)
      if (SHOP_ROOM(shop_nr, sindex) == room)
         return (TRUE);
   return (FALSE);
   }


SPECIAL(shop_keeper)
   {
   struct char_data *keeper = (struct char_data *) me;
   int shop_nr;

   if(cmd<1)
      return FALSE;
   if(AFF_FLAGGED(keeper,AFF_CHARM))
      return FALSE;

   for (shop_nr = 0; shop_nr < top_shop; shop_nr++)
      if (SHOP_KEEPER(shop_nr) == keeper->nr)
         break;

   if (shop_nr >= top_shop)
      return (FALSE);

   if (SHOP_FUNC(shop_nr) &&
           (SHOP_FUNC(shop_nr)!=shop_keeper)) /* Check secondary function */
      if ((SHOP_FUNC(shop_nr)) (ch, me, cmd, argument))
         return (TRUE);

   if (keeper == ch)
      {
      if (cmd)
         SHOP_SORT(shop_nr) = 0; /* Safety in case "drop all" */
      return (FALSE);
      }
   if (!ok_shop_room(shop_nr, (GET_ROOM_VNUM(IN_ROOM(ch)))))
      return (0);

   if (!AWAKE(keeper))
      return (FALSE);

   if (CMD_IS("steal"))
      {
      char *argm = get_buffer(MAX_INPUT_LENGTH);
      sprintf(argm, "$N shouts '%s'", MSG_NO_STEAL_HERE);
      do_action(keeper, GET_NAME(ch), cmd_slap, 0);
      act(argm, FALSE, ch, 0, keeper, TO_CHAR);
      release_buffer(argm);
      return (TRUE);
      }
   else if (CMD_IS("tame"))
      {
      char *argm = get_buffer(MAX_INPUT_LENGTH);
      do_action(keeper, GET_NAME(ch), cmd_slap, 0);
      act("$N says 'Go away, ya bother me!'", FALSE, ch, 0, keeper, TO_CHAR);
      release_buffer(argm);
      return (TRUE);
      }
   else if (CMD_IS("buy"))
      {
      shopping_buy(argument, ch, keeper, shop_nr);
      return (TRUE);
      }
   else if (CMD_IS("sell"))
      {
      shopping_sell(argument, ch, keeper, shop_nr);
      return (TRUE);
      }
   else if (CMD_IS("value"))
      {
      shopping_value(argument, ch, keeper, shop_nr);
      return (TRUE);
      }
   else if (CMD_IS("list"))
      {
      shopping_list(argument, ch, keeper, shop_nr);
      return (TRUE);
      }
   return (FALSE);
   }


int ok_damage_shopkeeper(struct char_data * ch, struct char_data * victim)
{
   if (IS_MOB(victim) && (mob_index[GET_MOB_RNUM(victim)].func==shop_keeper)&&
           !AFF_FLAGGED(victim,AFF_CHARM))
   {
     //for (sindex = 0; sindex < top_shop; sindex++)
     //if ((GET_MOB_RNUM(victim) == SHOP_KEEPER(sindex)) &&
     //    !SHOP_KILL_CHARS(sindex))
     //{
     if(!IS_NPC(ch))
     {
       do_action(victim, GET_NAME(ch), cmd_slap, 0);
     }
     perform_tell(victim,ch, MSG_CANT_KILL_KEEPER);
     if (FIGHTING(ch) == victim)
     {
       stop_fighting(ch);
       stop_fighting(victim);
     }
     return (FALSE);
     //}
   }
   return (TRUE);
 }


int add_to_list(struct shop_buy_data * list, int type, int *len, int *val)
   {
   if (*val >= 0)
      {
      if (*len < MAX_SHOP_OBJ)
         {
         if (type == LIST_PRODUCE)
            *val = real_object(*val);
         if (*val >= 0)
            {
            BUY_TYPE(list[*len]) = *val;
            BUY_WORD(list[(*len)++]) = 0;
            }
         else
            *val = 0;
         return (FALSE);
         }
      else
         return (TRUE);
      }
   return (FALSE);
   }


int end_read_list(struct shop_buy_data * list, int len, int error)
   {
   if (error)
      {
      log("SYSERR: Raise MAX_SHOP_OBJ constant in shop.h to %d", len + error);
      }
   BUY_WORD(list[len]) = 0;
   BUY_TYPE(list[len++]) = NOTHING;
   return (len);
   }


void read_line(FILE * shop_f, char *string, void *data)
   {
   char *buf = get_buffer(MAX_STRING_LENGTH);
   if (!get_line(shop_f, buf) || !sscanf(buf, string, data))
      {
      log("SYSERR: Error in shop #%ld\n", SHOP_NUM(top_shop));
      exit(1);
      }
   release_buffer(buf);
   }


int read_list(FILE * shop_f, struct shop_buy_data * list, int new_format,
              int max, int type)
   {
   int count, temp, len = 0, error = 0;

   if (new_format)
      {
      do
         {
         read_line(shop_f, "%d", &temp);
         error += add_to_list(list, type, &len, &temp);
         }
      while (temp >= 0);
      }
   else
      for (count = 0; count < max; count++)
         {
         read_line(shop_f, "%d", &temp);
         error += add_to_list(list, type, &len, &temp);
         }
   return (end_read_list(list, len, error));
   }


int read_type_list(FILE * shop_f, struct shop_buy_data * list,
                   int new_format, int max)
   {
   int sindex, num, len = 0, error = 0;
   char *ptr,*buf;

   if (!new_format)
      return (read_list(shop_f, list, 0, max, LIST_TRADE));

   buf = get_buffer(MAX_STRING_LENGTH);
   do
      {
      fgets(buf, MAX_STRING_LENGTH - 1, shop_f);
      if ((ptr = strchr(buf, ';')) != NULL)
         *ptr = 0;
      else
         *(END_OF(buf) - 1) = 0;
      for (sindex = 0, num = NOTHING; *item_types[sindex] != '\n'; sindex++)
         if (!strn_cmp(item_types[sindex], buf, strlen(item_types[sindex])))
            {
            num = sindex;
            strcpy(buf, buf + strlen(item_types[sindex]));
            break;
            }
      ptr = buf;
      if (num == NOTHING)
         {
         sscanf(buf, "%d", &num);
         while (!isdigit((int)*ptr))
            ptr++;
         while (isdigit((int)*ptr))
            ptr++;
         }
      while (isspace((int)*ptr))
         ptr++;
      while (isspace((int)*(END_OF(ptr) - 1)))
         *(END_OF(ptr) - 1) = 0;
      error += add_to_list(list, LIST_TRADE, &len, &num);
      if (*ptr)
         BUY_WORD(list[len - 1]) = str_dup(ptr);
      }
   while (num >= 0);
   release_buffer(buf);
   return (end_read_list(list, len, error));
   }


void boot_the_shops(FILE * shop_f, char *filename, int rec_count)
   {
   char *buf, *buf2=get_buffer(150);
   int temp, count, new_format = 0,r_keeper;
   struct shop_buy_data list[MAX_SHOP_OBJ + 1];
   int done = 0;

   sprintf(buf2, "beginning of shop file %s", filename);

   while (!done)
      {
      buf = fread_string(shop_f, buf2);
      if (*buf == '#')
         {
         /* New shop */
         sscanf(buf, "#%d\n", &temp);
         sprintf(buf2, "shop #%d in shop file %s", temp, filename);
         free(buf);  /* Plug memory leak! */
         if (!top_shop)
            CREATE(shop_index, struct shop_data, rec_count);

         SHOP_NUM(top_shop) = temp;
         temp = read_list(shop_f, list, new_format, MAX_PROD, LIST_PRODUCE);
         CREATE(shop_index[top_shop].producing, obj_vnum, temp);
         for (count = 0; count < temp; count++)
            SHOP_PRODUCT(top_shop, count) = BUY_TYPE(list[count]);

         read_line(shop_f, "%f", &SHOP_BUYPROFIT(top_shop));
         read_line(shop_f, "%f", &SHOP_SELLPROFIT(top_shop));

         temp = read_type_list(shop_f, list, new_format, MAX_TRADE);
         CREATE(shop_index[top_shop].type, struct shop_buy_data, temp);
         for (count = 0; count < temp; count++)
            {
            SHOP_BUYTYPE(top_shop, count) = BUY_TYPE(list[count]);
            SHOP_BUYWORD(top_shop, count) = BUY_WORD(list[count]);
            }

         shop_index[top_shop].no_such_item1 = fread_string(shop_f, buf2);
         shop_index[top_shop].no_such_item2 = fread_string(shop_f, buf2);
         shop_index[top_shop].do_not_buy = fread_string(shop_f, buf2);
         shop_index[top_shop].missing_cash1 = fread_string(shop_f, buf2);
         shop_index[top_shop].missing_cash2 = fread_string(shop_f, buf2);
         shop_index[top_shop].message_buy = fread_string(shop_f, buf2);
         shop_index[top_shop].message_sell = fread_string(shop_f, buf2);
         read_line(shop_f, "%d", &SHOP_BROKE_TEMPER(top_shop));
         read_line(shop_f, "%d", &SHOP_BITVECTOR(top_shop));
         SET_BIT(SHOP_BITVECTOR(top_shop),WILL_BANK_MONEY);
         read_line(shop_f, "%ld", &SHOP_KEEPER(top_shop));

         if((r_keeper = real_mobile(SHOP_KEEPER(top_shop)))==-1)
            log("SYSERR: Shop without a keeper: shop: %ld(%d) keeper: %ld",
                SHOP_NUM(top_shop),top_shop+1,SHOP_KEEPER(top_shop));
         SHOP_KEEPER(top_shop)=r_keeper;

         read_line(shop_f, "%d", &SHOP_TRADE_WITH(top_shop));

         temp = read_list(shop_f, list, new_format, 1, LIST_ROOM);
         CREATE(shop_index[top_shop].in_room, room_rnum, temp);
         for (count = 0; count < temp; count++)
            SHOP_ROOM(top_shop, count) = BUY_TYPE(list[count]);

         read_line(shop_f, "%d", &SHOP_OPEN1(top_shop));
         read_line(shop_f, "%d", &SHOP_CLOSE1(top_shop));
         read_line(shop_f, "%d", &SHOP_OPEN2(top_shop));
         read_line(shop_f, "%d", &SHOP_CLOSE2(top_shop));

         SHOP_BANK(top_shop) = 0;
         SHOP_SORT(top_shop) = 0;
         SHOP_FUNC(top_shop) = 0;
         SHOP_TOP_ORDER(top_shop) = 0;
         top_shop++;
         }
      else
         {
         if (*buf == '$')  /* EOF */
            done = TRUE;
         else if (strstr(buf, VERSION3_TAG)) /* New format marker */
            new_format = 1;
         free(buf);  /* Plug memory leak! */
         }
      }
   release_buffer(buf2);
   }


void assign_the_shopkeepers(void)
   {
   int sindex;

   cmd_say   = find_command("say");
   cmd_tell  = find_command("tell");
   cmd_emote = find_command("emote");
   cmd_slap  = find_command("slap");
   cmd_puke  = find_command("puke");
   cmd_faint = find_command("faint");
   cmd_growl = find_command("growl");
   for (sindex = 0; sindex < top_shop; sindex++)
      {
      if(SHOP_KEEPER(sindex)<0)
         {
         log("SYSERR: shop without a keeper! shop:%d",sindex+1);
         continue;
         }
      if((mob_index[SHOP_KEEPER(sindex)].func) &&
              (mob_index[SHOP_KEEPER(sindex)].func==*shop_keeper))
         {
         SHOP_FUNC(sindex)=NULL;
         continue;
         }
      if (mob_index[SHOP_KEEPER(sindex)].func)
         SHOP_FUNC(sindex) = mob_index[SHOP_KEEPER(sindex)].func;
      mob_index[SHOP_KEEPER(sindex)].func = shop_keeper;
      }
   }


char *customer_string(int shop_nr, int detailed)
   {
   int sindex, cnt = 1;
   char *buf=get_buffer(256);
   static char buf1[256];

   *buf = 0;
   for (sindex = 0; *trade_letters[sindex] != '\n'; sindex++, cnt *= 2)
      if (!(SHOP_TRADE_WITH(shop_nr) & cnt))
         {
         if (detailed)
            {
            if (*buf)
               strcat(buf, ", ");
            strcat(buf, trade_letters[sindex]);
            }
         else
            sprintf(END_OF(buf), "%c", *trade_letters[sindex]);
         }
      else if (!detailed)
         strcat(buf, "_");

   strcpy(buf1,buf);
   release_buffer(buf);
   return (buf1);
   }


void list_all_shops(struct char_data * ch)
   {
   int shop_nr;
   char *buf = get_buffer(32750);
   char *buf1 = get_buffer(MAX_STRING_LENGTH);
   char *buf2 = get_buffer(MAX_STRING_LENGTH);



   strcpy(buf, "\r\n");
   for (shop_nr = 0; shop_nr < top_shop; shop_nr++)
      {
      if (!(shop_nr % 20))
         {
         strcat(buf, " ##  Virtual  Where   Keeper   Buy  Sell      Bank Customers\r\n");
         strcat(buf, "------------------------------------------------------------------\r\n");
         }
      sprintf(buf2, "%3d  %6ld  %6ld   ", shop_nr + 1, SHOP_NUM(shop_nr),
              SHOP_ROOM(shop_nr, 0));
      if (SHOP_KEEPER(shop_nr) < 0)
         strcpy(buf1, "<NONE>");
      else
         sprintf(buf1, "%6ld", mob_index[SHOP_KEEPER(shop_nr)].vnum);
      sprintf(END_OF(buf2), "%s  %3.2f  %3.2f  %9ld ", buf1,
              SHOP_SELLPROFIT(shop_nr), SHOP_BUYPROFIT(shop_nr),
              (long)SHOP_BANK(shop_nr));
      strcat(buf2, customer_string(shop_nr, FALSE));
      sprintf(END_OF(buf), "%s\r\n", buf2);
      }

   release_buffer(buf2);
   release_buffer(buf1);
   page_string(ch->desc, buf,TRUE,"");
   release_buffer(buf);
   }


void handle_detailed_list(char *buf, char *buf1, struct char_data * ch)
   {
   if ((strlen(buf1) + strlen(buf) < 78) || (strlen(buf) < 20))
      strcat(buf, buf1);
   else
      {
      strcat(buf, "\r\n");
      send_to_char(ch, "%s", buf);
      sprintf(buf, "            %s", buf1);
      }
   }


void list_detailed_shop(struct char_data * ch, int shop_nr)
   {
   struct obj_data *obj;
   struct char_data *k;
   int sindex, temp;
   char *buf=get_buffer(MAX_STRING_LENGTH);
   char *buf1=get_buffer(MAX_STRING_LENGTH);

   send_to_char(ch, "Vnum:       [%5ld], Rnum: [%5d]\r\n", SHOP_NUM(shop_nr),
                shop_nr + 1);

   strcpy(buf, "Rooms:      ");
   for (sindex = 0; SHOP_ROOM(shop_nr, sindex) != NOWHERE; sindex++)
      {
      if (sindex)
         strcat(buf, ", ");
      if ((temp = real_room(SHOP_ROOM(shop_nr, sindex))) != NOWHERE)
         sprintf(buf1, "%s (#%ld)", world[temp].name, GET_ROOM_VNUM(temp));
      else
         sprintf(buf1, "<UNKNOWN> (#%ld)", SHOP_ROOM(shop_nr, sindex));
      handle_detailed_list(buf, buf1, ch);
      }
   if (!sindex)
      send_to_char(ch, "Rooms:      None!\r\n");
   else
      {
      send_to_char(ch, "\r\n");
      }

   strcpy(buf, "Shopkeeper: ");
   if (SHOP_KEEPER(shop_nr) >= 0)
      {
      sprintf(END_OF(buf), "%s (#%ld), Special Function: %s\r\n",
              GET_NAME(&mob_proto[SHOP_KEEPER(shop_nr)]),
              mob_index[SHOP_KEEPER(shop_nr)].vnum, YESNO(SHOP_FUNC(shop_nr)));
      if ((k = get_char_num(SHOP_KEEPER(shop_nr))))
         {
         send_to_char(ch,"%s",buf);
         sprintf(buf, "Coins:      [%9ld], Bank: [%9ld] (Total: %ld)\r\n",
                 GET_GOLD(k), (long)SHOP_BANK(shop_nr),
                 GET_GOLD(k) + SHOP_BANK(shop_nr));
         }
      }
   else
      strcat(buf, "<NONE>\r\n");
   send_to_char(ch,"%s",buf);

   strcpy(buf1, customer_string(shop_nr, TRUE));
   send_to_char(ch, "Customers:  %s\r\n", (*buf1) ? buf1 : "None");

   strcpy(buf, "Produces:   ");
   for (sindex = 0; SHOP_PRODUCT(shop_nr, sindex) != NOTHING; sindex++)
      {
      obj = &obj_proto[SHOP_PRODUCT(shop_nr, sindex)];
      if (sindex)
         strcat(buf, ", ");
      sprintf(buf1, "%s (#%ld)", obj->short_description,
              obj_index[SHOP_PRODUCT(shop_nr, sindex)].vnum);
      handle_detailed_list(buf, buf1, ch);
      }
   if (!sindex)
      send_to_char(ch,"Produces:   Nothing!\r\n");
   else
      {
      send_to_char(ch, "%s\r\n",buf);
      }

   strcpy(buf, "Buys:       ");
   for (sindex = 0; SHOP_BUYTYPE(shop_nr, sindex) != NOTHING; sindex++)
      {
      if (sindex)
         strcat(buf, ", ");
      sprintf(buf1, "%s (#%d) ", item_types[SHOP_BUYTYPE(shop_nr, sindex)],
              SHOP_BUYTYPE(shop_nr, sindex));
      if (SHOP_BUYWORD(shop_nr, sindex))
         sprintf(END_OF(buf1), "[%s]", SHOP_BUYWORD(shop_nr, sindex));
      else
         strcat(buf1, "[all]");
      handle_detailed_list(buf, buf1, ch);
      }
   if (!sindex)
      send_to_char(ch,"Buys:       Nothing!\r\n");
   else
      {
      send_to_char(ch, "%s\r\n",buf);
      }

   send_to_char(ch, "Buy at:     [%4.2f], Sell at: [%4.2f], Open: [%d-%d, %d-%d]%s",
                SHOP_SELLPROFIT(shop_nr), SHOP_BUYPROFIT(shop_nr), SHOP_OPEN1(shop_nr),
                SHOP_CLOSE1(shop_nr), SHOP_OPEN2(shop_nr), SHOP_CLOSE2(shop_nr), "\r\n");


   sprintbit((long) SHOP_BITVECTOR(shop_nr), shop_bits, buf1);
   send_to_char(ch, "Bits:       %s\r\n", buf1);
   release_buffer(buf1);
   release_buffer(buf);
   }


void show_shops(struct char_data * ch, char *arg)
   {
   int shop_nr;

   if (!*arg)
      list_all_shops(ch);
   else
      {
      if (!str_cmp(arg, "."))
         {
         for (shop_nr = 0; shop_nr < top_shop; shop_nr++)
            if (ok_shop_room(shop_nr, GET_ROOM_VNUM(IN_ROOM(ch))))
               break;

         if (shop_nr == top_shop)
            {
            send_to_char(ch,"This isn't a shop!\r\n");
            return;
            }
         }
      else if (is_number(arg))
         shop_nr = atoi(arg) - 1;
      else
         shop_nr = -1;

      if ((shop_nr < 0) || (shop_nr >= top_shop))
         {
         send_to_char(ch,"Illegal shop number.\r\n");
         return;
         }
      list_detailed_shop(ch, shop_nr);
      }
   }


void shop_activity(void)
   {
   int shop_nr;

   for(shop_nr=0;shop_nr<top_shop;shop_nr++)
      if(SHOP_USES_BANK(shop_nr))
         {
         if(SHOP_BANK(shop_nr)<20000)
            SHOP_BANK(shop_nr)+=500;
         else if(SHOP_BANK(shop_nr)<50000)
            SHOP_BANK(shop_nr)+=100;
         else
            SHOP_BANK(shop_nr)-=1000;
         }
   }


int save_shop_nonnative(shop_rnum shop_num, struct char_data *keeper)
   {
   FILE *fp = NULL;
   struct obj_data *obj, *next_obj;
   char *buf = get_buffer(128);
   int error=0;

   sprintf(buf,"%s/%ld.shop",SHOP_SAVE_PREFIX,SHOP_NUM(shop_num));

   if(!(fp=fopen(buf,"wb")))
      {
      mudlogf(CMP,LVL_IMMORT,TRUE,
              "SYSERR OBJSAVE: Can't open %s to save shop objects. %d",
              buf,errno);
      release_buffer(buf);
      return FALSE;
      }
   if(fprintf(fp,"@Version: %d\n",CUR_POBJ_VER)<1)
      {
      perror("SYSERR: Writing rent version");
      log("SYSERR OBJSAVE: Error writing rent version for shop %s %d",
          buf,errno);
      return FALSE;
      }

   for(obj = keeper->carrying; obj; obj = next_obj)
      {
      next_obj = obj->next_content;
      if(Crash_is_unrentable(obj) == FALSE)
         {
         if(!shop_producing(obj,shop_num))
            {
            error = 0;
            if((error = Obj_to_store(obj,fp,0))!=1)
               {
               log("SYSERR OBJSAVE: Error writing %s for shop %s %d",
                   GET_OBJ_NAME(obj),buf,error);
               }
            }
         }
      }
   fclose(fp);
   release_buffer(buf);
   return TRUE;
   }

void load_shop_nonnative(shop_rnum shop_num,struct char_data *keeper)
   {
   FILE *fp = NULL;
   struct obj_data *obj;
   char *buf, *line;
   int version = 0;
   int locate;

   if(keeper == NULL)
      return;

   buf  = get_buffer(128);
   sprintf(buf,"%s/%ld.shop",SHOP_SAVE_PREFIX,SHOP_NUM(shop_num));
   if (!(fp = fopen(buf, "r+b")))
      {
      /* no file found */
      release_buffer(buf);
      return;
      }

   line=get_buffer(256);
   if(!feof(fp))
      get_line(fp, line);

   if(*line == '@')
      {
      if(sscanf(line,"@Version: %d",&version)!=1)
         {
         mudlogf(CMP,LVL_IMMORT,TRUE,
                 "SYSERR OBJLOAD: Format error in %s with line: %s",
                 buf,line);
         exit(1);
         }
      if(!feof(fp))
         get_line(fp,line);
      }
   else
      {
      version = 1;
      }

   while (!feof(fp))
      {
      obj = NULL;
      /* first, we get the number. Not too hard. */
      if(parse_xap_obj(buf,&obj,line,fp,version,&locate))
         {
         if(obj != NULL)
            {
            slide_obj(obj,keeper,shop_num);
            if(GET_OBJ_SHOP_ORDER(obj) == 0)
               {
               SHOP_TOP_ORDER(shop_num)++;
               GET_OBJ_SHOP_ORDER(obj) = SHOP_TOP_ORDER(shop_num);
               }
            else if(GET_OBJ_SHOP_ORDER(obj) > SHOP_TOP_ORDER(shop_num))
               {
               SHOP_TOP_ORDER(shop_num) = GET_OBJ_SHOP_ORDER(obj);
               }

            }
         }
      else
         {
         if(!feof(fp))
            get_line(fp, line);
         else
            break;
         }
      }
   release_buffer(buf);
   release_buffer(line);
   fclose(fp);
   }


void load_keeper(struct char_data *keeper)
   {
   shop_rnum shop_nr;
   for(shop_nr = 0; shop_nr <= top_shop; shop_nr++)
      {
      if(SHOP_KEEPER(shop_nr) == keeper->nr)
         {
         load_shop_nonnative(shop_nr,keeper);
         break;
         }
      }
   }

void compress_shop_order(shop_rnum shop_num, struct char_data *keeper)
   {
   struct obj_data *bottomobj;
   struct obj_data *obj;
   int current_order=SHOP_TOP_ORDER(shop_num)+30;
   int bottom_order = 0;
   long num_objects=0;
   int i;

   Crash_count_items(keeper->carrying,&num_objects);
   for(i=0;i<num_objects+1;i++)
      {
      current_order=SHOP_TOP_ORDER(shop_num)+30;
      for(bottomobj=NULL,obj=keeper->carrying;obj;obj=obj->next_content)
         {
         if((GET_OBJ_SHOP_ORDER(obj) > bottom_order) &&
                 (GET_OBJ_SHOP_ORDER(obj) < current_order))
            {
            bottomobj = obj;
            current_order= GET_OBJ_SHOP_ORDER(obj);
            }
         }
      if(bottomobj)
         {
         bottom_order++;
         GET_OBJ_SHOP_ORDER(bottomobj) = bottom_order;
         }
      }
   SHOP_TOP_ORDER(shop_num) = num_objects;
   }


void process_keeper(struct char_data *keeper)
   {
   shop_rnum shop_nr;
   struct obj_data *purgeme;
   struct obj_data *obj;
   int num_to_purge=0;
   long num_objects=0;
   int i;
   int did_purge=0;

   for(shop_nr = 0; shop_nr <= top_shop; shop_nr++)
      {
      if(SHOP_KEEPER(shop_nr) == keeper->nr)
         {
         break;
         }
      }
   if(SHOP_KEEPER(shop_nr) != keeper->nr)
      {
      log("SYSERR: Couldn't found a shop for %s: vnum:%ld.",
          GET_NAME(keeper),GET_MOB_VNUM(keeper));
      return;
      }
   Crash_count_items(keeper->carrying,&num_objects);
   if(!(num_to_purge = num_objects/50))
      return;
   did_purge=0;
   for(i=0;i<num_to_purge;i++)
      {
      purgeme = NULL;
      for(obj = keeper->carrying; obj;
              obj = obj->next_content)
         {
         if(shop_producing(obj,shop_nr))
            {
            continue;
            }
         if(purgeme)
            {
            if(GET_OBJ_SHOP_ORDER(purgeme)>GET_OBJ_SHOP_ORDER(obj))
               purgeme=obj;
            }
         else
            {
            purgeme=obj;
            }
         }
      if(purgeme!=NULL)
         {
         log("SHOP: %s is purging %s: %d, room #%ld",GET_NAME(keeper),
             GET_OBJ_NAME(purgeme),
             GET_OBJ_SHOP_ORDER(purgeme),
             GET_ROOM_VNUM(IN_ROOM(keeper)));
         GET_GOLD(keeper) += GET_OBJ_COST(purgeme);
         extract_obj(purgeme);
         did_purge++;
         }
      }
   if(did_purge>0)
      {
      if (SHOP_USES_BANK(shop_nr))
         {
         if (GET_GOLD(keeper) > MAX_OUTSIDE_BANK)
            {
            SHOP_BANK(shop_nr) += (GET_GOLD(keeper) - MAX_OUTSIDE_BANK);
            GET_GOLD(keeper) = MAX_OUTSIDE_BANK;
            }
         }
      compress_shop_order(shop_nr,keeper);
      save_shop_nonnative(shop_nr,keeper);
      }
   }


void shop_housekeeping(void)
   {
   struct char_data *keeper;
   for (keeper = character_list; keeper; keeper = keeper->next)
      {
      if(GET_MOB_SPEC(keeper) == shop_keeper)
         process_keeper(keeper);
      }
   }

