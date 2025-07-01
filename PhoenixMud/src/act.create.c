/* ************************************************************************
*   File: act.create.c					Part of CircleMUD *
*  Usage: Player-level object creation stuff				  *
*									  *
*  All rights reserved.	 See license.doc for complete information.	  *
*									  *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.		  *
************************************************************************ */

#include "../localHeader/conf.h"
#include "../localHeader/sysdep.h"

#include <sys/stat.h>

#include "structs.h"
#include "buffer.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"

#include "screen.h"
#include "constants.h"
#include "dg_scripts.h"
#include "queue.h"


/* struct for syls */
struct syllable {
  char *org;
  char *new;
};

/* extern variables */
extern struct spell_info_type *spells;
extern struct syllable syls[];

extern struct index_data *obj_index;

char *get_spell_name(char *argument)
{
  char *s;

  s = strtok(argument, "'");
  s = strtok(NULL, "'");
  
  return s;
}

struct spell_item
{
   int spell_num;
   char *item_string;
   obj_vnum item_vnum;
};

struct spell_item potions[] = {
  { SPELL_CURE_BLIND, "milky white",700},
  { SPELL_CURE_LIGHT, "bubbling white",701},
  { SPELL_CURE_CRITIC, "glowing ivory",702},
  { SPELL_DETECT_MAGIC, "glowing blue",703},
  { SPELL_DETECT_INVIS, "bubbling yellow",704},
  { SPELL_DETECT_POISON, "light green",705},
  { SPELL_REMOVE_POISON, "gritty brown",706},
  { SPELL_STRENGTH, "blood red",707},
  { SPELL_WORD_OF_RECALL, "swirling purple",708},
  { SPELL_SENSE_LIFE, "flickering green",709},
  { SPELL_WATERWALK, "cloudy blue",710},
  { SPELL_INFRAVISION, "glowing red",711},
  { SPELL_HEAL, "sparkling white",712},
  { SPELL_SANCTUARY, "incandescent blue",713},
  { SPELL_ARMOR, "steel grey",714},
  { SPELL_FLY, "dull white",715},
  { SPELL_INVISIBLE, "crystal clear",716},
  { SPELL_LEVITATE, "swirling red",717},
  { SPELL_CURE_SERIOUS, "frothing white",718},
  { SPELL_WATER_BREATHE, "bubbling blue",719},
  {0,"NONE",0}
};

struct spell_item scrolls[] = {
   {SPELL_CURE_BLIND, "",0},
   {SPELL_CURE_LIGHT, "",0},
   {SPELL_CURE_CRITIC, "",0},
   {SPELL_DETECT_MAGIC, "",0},
   {SPELL_DETECT_INVIS, "",0},
   {SPELL_DETECT_POISON, "",0},
   {SPELL_REMOVE_POISON, "",0},
   {SPELL_STRENGTH, "",0},
   {SPELL_WORD_OF_RECALL, "",0},
   {SPELL_SENSE_LIFE, "",0},
   {SPELL_WATERWALK, "",0},
   {SPELL_INFRAVISION, "",0},
   {SPELL_HEAL, "",0},
   {SPELL_SANCTUARY, "",0},
   {0,"NONE",0}
};

void make_potion(struct char_data *ch, int potion, struct obj_data *container,
		 int clevel)
{
   struct obj_data *final_potion;
   struct extra_descr_data *new_descr;
   int can_make = FALSE, mana, dam, num = 0;
   char *buf2;
   int percentage=90;
   int chance;


   while(potions[num].spell_num!=0)
      {
      if(potions[num].spell_num==potion)
	 {
	 can_make=TRUE;
	 break;
	 }

      num++;
      }


   if (can_make == FALSE) 
      {
      send_to_char(ch,"That spell cannot be mixed into a potion.\r\n");
      return;
      }
   mana = mag_manacost(ch, potion,clevel) * 3;
   if (GET_MANA(ch) - mana <= 0) 
      {
      send_to_char(ch,"You don't have enough mana to mix that potion!\r\n");
      return;
      }

   if ((number(0, 101)>GET_SKILL(ch,SKILL_BREW)) && 
	    !PRF_FLAGGED(ch,PRF_NOHASSLE)) 
      {
      improve_skill(ch,SKILL_BREW,USE_FAIL);
      send_to_char(ch, "As you begin mixing the potion, it violently"
		   " explodes!\r\n");
      act("$n begins to mix a potion, but it suddenly explodes!",
	  FALSE, ch, 0,0, TO_ROOM);
      extract_obj(container);
      dam = number(15, MAX(16, ((mana/3) * 2)));
      damage(ch,ch,dam,SKILL_BREW,IMM_DRAIN);
      return;
      }
   else if(clevel>GET_SKILL(ch,potion))
      percentage=percentage-(15*(clevel-GET_SKILL(ch,potion)));
   else if(clevel<GET_SKILL(ch,potion))
      percentage=percentage+(2*(GET_SKILL(ch,potion)-clevel));

   if(PRF_FLAGGED(ch,PRF_NOHASSLE))
      chance=0;
   else
      chance=number(0,101);

   if(chance>percentage)
      {
      WAIT_STATE(ch, SKILL_LAG); 
      improve_skill(ch,potion,USE_FAIL);
      if(chance>percentage+5)
         {
         send_to_char(ch,"Something went VERY wrong!!\r\n");
	 act("$n tries to brew a potion, but it explodes!",
	     FALSE, ch, 0,0, TO_ROOM);
         damage(ch,ch,(GET_MAX_HIT(ch)/3),SKILL_BREW,IMM_DRAIN);
         GET_MANA(ch)/=2;
         }
      else
	 {
	 send_to_char(ch,"You just can't seem to get the mix right\r\n");
	 act("$n tries to brew a potion, but it just won't mix right.",
	     FALSE, ch, 0,0, TO_ROOM);
	 }
      return;
      }

  /* requires x3 mana to mix a potion than the spell */
   if (GET_LEVEL(ch) < LVL_IMMORT) 
      GET_MANA(ch) -= mana;
   send_to_char(ch,"You create a %s potion.\r\n",
	      spells[potion].spell_name);
   act("$n creates a potion!", FALSE, ch, 0, 0, TO_ROOM);
   extract_obj(container);
   
   improve_skill(ch,SKILL_BREW,USE_PASS);
   improve_skill(ch,potion,USE_PASS);
   final_potion = read_object(potions[num].item_vnum,VIRTUAL);

   IN_ROOM(final_potion) = NOWHERE;
   buf2=get_buffer(MAX_STRING_LENGTH);

   /* extra description coolness! */
   CREATE(new_descr, struct extra_descr_data, 1);
   new_descr->keyword = str_dup(final_potion->name);
   sprintf(buf2, "It appears to be a %s potion.\n", spells[potion].spell_name);
   new_descr->description = str_dup(buf2);
   release_buffer(buf2);
   new_descr->next = NULL;
   final_potion->ex_description = new_descr;
 
   GET_OBJ_TYPE(final_potion) = ITEM_POTION;
   GET_OBJ_WEAR(final_potion) = ITEM_WEAR_TAKE;
   GET_OBJ_EXTRA(final_potion) = ITEM_NORENT;
   GET_OBJ_VAL(final_potion, 0) = clevel;
   GET_OBJ_VAL(final_potion, 1) = potion;
   GET_OBJ_VAL(final_potion, 2) = -1;
   GET_OBJ_VAL(final_potion, 3) = -1;
/*    GET_OBJ_COST(final_potion) = 0; */
/*    GET_OBJ_COST(final_potion) = GET_LEVEL(ch) * 500; */
   GET_OBJ_WEIGHT(final_potion) = 1;
   GET_OBJ_RENT(final_potion) = 0;
   GET_OBJ_CSLOTS(final_potion) = 10;
   GET_OBJ_OSLOTS(final_potion) = 10;
   GET_OBJ_TSLOTS(final_potion) = 10;
   GET_OBJ_TIMER(final_potion) = 1152; /* 24 hours. */
   obj_to_char(final_potion, ch);
}

ACMD(do_brew)
{
   struct obj_data *container = NULL;
   struct obj_data *obj, *next_obj;
   char *bottle_name;
   char *spell_name;       
   char *temp1, *temp2;
   int potion, found = FALSE;
   int clevel=0;
   char cnum[4];
   bottle_name=get_buffer(MAX_STRING_LENGTH);
   spell_name=get_buffer(MAX_STRING_LENGTH);
   temp1 = one_argument(argument, bottle_name);
	
   /* sanity check */
   skip_spaces(&temp1);

   if (temp1)
      {
      if(isdigit((int)temp1[0]))
	 {
	 cnum[0]=temp1[0];
	 if(isdigit((int)temp1[1]))
	    {
	    cnum[1]=temp1[1];
	    cnum[2]='\0';
	    }
	 else
	    cnum[1]='\0';
	 clevel=atoi(cnum);
	 temp2 = get_spell_name(temp1);
	 }
      else
	 temp2=strtok(temp1,"'");

      if (temp2)
	 {
	 strcpy(spell_name, temp2);
	 }
      } 
   else 
      {
      bottle_name[0] = '\0';
      spell_name[0] = '\0';
      } 

   
   if (IS_NPC(ch)||!GET_SKILL(ch,SKILL_BREW)||!SCR_SKILLCHECK(ch, SKILL_BREW))
      {
      send_to_char(ch,"You have no idea how to mix potions!\r\n");
      release_buffer(spell_name);
      release_buffer(bottle_name);
      return;
      }
   if (!*bottle_name || !*spell_name) 
      {
      int num=0;
      send_to_char(ch,"What do you wish to mix in where?\r\n");
      if(GET_LEVEL(ch)>=LVL_IMMORT)
	 {
	 while(potions[num].spell_num!=0)
	    {
	    send_to_char(ch," %2d: %-20.20s | %-15.15s | %ld\r\n",num,
		    spells[potions[num].spell_num].spell_name,
		    potions[num].item_string,potions[num].item_vnum);
	    num++;
	    }
	 }
      release_buffer(spell_name);
      release_buffer(bottle_name);
      return;
      }
	   
   for (obj = ch->carrying; obj; obj = next_obj)
      {
      next_obj = obj->next_content;
      if (obj == NULL)
	 {
	 release_buffer(spell_name);
	 release_buffer(bottle_name);
	 return;
	 }
      else if (!(container = get_obj_in_list_vis(ch, bottle_name,
						 ch->carrying))) 
	 continue;
      else
	 found = TRUE;
      }
   
   if (found != FALSE && (GET_OBJ_TYPE(container) != ITEM_DRINKCON))
      {
      send_to_char(ch,"That item is not a drink container!\r\n");
      release_buffer(spell_name);
      release_buffer(bottle_name);
      return;
      }
   if (found == FALSE)
      {
      send_to_char(ch, "You don't have %s in your inventory!\r\n",
	      bottle_name);
      release_buffer(spell_name);
      release_buffer(bottle_name);
      return;
      }
	   
   if (!spell_name || !*spell_name) 
      {
      send_to_char(ch,"Spell names must be enclosed in single quotes!\r\n");
      release_buffer(spell_name);
      release_buffer(bottle_name);
      return;
      }
	   
   potion = find_skill_num(spell_name);	
	   
   if ((potion < 1) || (potion >= MAX_SPELLS)) 
      {
      send_to_char(ch,"Mix what spell?!?\r\n");
      release_buffer(spell_name);
      release_buffer(bottle_name);
      return;
      }

   if (GET_LEVEL(ch) < spells[potion].min_level[(int) GET_CLASS(ch)]) 
      {
      send_to_char(ch,"You are not schooled enough to cast that spell!\r\n");
      release_buffer(spell_name);
      release_buffer(bottle_name);
      return;
      }
   if (GET_SKILL(ch, potion) == 0 || !SCR_SKILLCHECK(ch, potion)) 
      {
      send_to_char(ch,"You are unfamiliar with this potion.\r\n");
      release_buffer(spell_name);
      release_buffer(bottle_name);
      return;
      }
   if(clevel==0)
      clevel=GET_SKILL(ch,potion);
   clevel=MAX(1,MIN(10,clevel));

   make_potion(ch, potion, container,clevel);
   release_buffer(spell_name);
   release_buffer(bottle_name);
}
   

void garble_spell(int spellnum,char *buf)
{
   char *lbuf=get_buffer(256);
   int j, ofs = 0;
   
   buf[0] = '\0';
   strcpy(lbuf, spells[spellnum].spell_name);

   while (*(lbuf + ofs)) 
      {
      for (j = 0; *(syls[j].org); j++) 
	 {
	 if (!strncmp(syls[j].org, lbuf + ofs, strlen(syls[j].org))) 
	    {
	    strcat(buf, syls[j].new);
	    ofs += strlen(syls[j].org);
	    }
	 }
      }
   release_buffer(lbuf);
}

void make_scroll(struct char_data *ch, int scroll, struct obj_data *paper, 
		 int clevel)
{
   struct obj_data *final_scroll;
   struct extra_descr_data *new_descr;
   int can_make = TRUE, mana, dam = 0;
   char *buf;
   char *buf2;
   int percentage=90;
   int chance;
   
   /* add a case statement here for allowed spells */
   
   switch (scroll) 
      {
       case SPELL_CURE_BLIND:
       case SPELL_CURE_LIGHT:
       case SPELL_CURE_CRITIC:
       case SPELL_DETECT_MAGIC:
       case SPELL_DETECT_INVIS:
       case SPELL_DETECT_POISON:
       case SPELL_REMOVE_POISON:
       case SPELL_STRENGTH:
       case SPELL_WORD_OF_RECALL:
       case SPELL_SENSE_LIFE:
       case SPELL_WATERWALK:
       case SPELL_INFRAVISION:
       case SPELL_HEAL:
       case SPELL_SANCTUARY:
	  can_make = TRUE;
	  break;

       default:
	  can_make = FALSE;
	  break;
      }

   if (can_make == FALSE) 
      {
      send_to_char(ch,"That spell cannot be scribed into a scroll.\r\n");
      return;
      }

   /* requires x3 mana to scribe a scroll than the spell */
   mana = mag_manacost(ch, scroll,clevel) * 3;
   
   if (GET_MANA(ch) - mana <= 0) 
      {
      send_to_char(ch,"You don't have enough mana to scribe such a "
		   "powerful spell!\r\n");
      return;
      }

   if ((number(0, 101)>GET_SKILL(ch,SKILL_SCRIBE)) && 
	    !PRF_FLAGGED(ch,PRF_NOHASSLE))
      {
      improve_skill(ch,SKILL_SCRIBE,USE_FAIL);
      send_to_char(ch,"As you begin inscribing the final rune, the"
		   " scroll violently explodes!\r\n");
      act("$n tries to scribe a spell, but it explodes!",
	  FALSE, ch, 0,0, TO_ROOM);
      extract_obj(paper);
      dam = number(15, MAX(16,((mana/3) * 2)));
      damage(ch,ch,dam,SKILL_SCRIBE,IMM_DRAIN);
      return;
      }
   else if(clevel>GET_SKILL(ch,scroll))
      percentage=percentage-(15*(clevel-GET_SKILL(ch,scroll)));
   else if(clevel<GET_SKILL(ch,scroll))
      percentage=percentage+(2*(GET_SKILL(ch,scroll)-clevel));

   if(PRF_FLAGGED(ch,PRF_NOHASSLE))
      chance=0;
   else
      chance=number(0,101);

   if(chance>percentage)
      {
      WAIT_STATE(ch, SKILL_LAG); 
      improve_skill(ch,scroll,USE_FAIL);
      if(chance>percentage+5)
         {
         send_to_char(ch,"Something went VERY wrong!!\r\n");
         damage(ch,ch,(GET_MAX_HIT(ch)/3),SKILL_SCRIBE,IMM_DRAIN);
         GET_MANA(ch)/=2;
         }
      else
	 {
	 send_to_char(ch,"You just can't seem to get the runes right\r\n");
	 }
      return;
      }

   buf=get_buffer(MAX_STRING_LENGTH);
   buf2=get_buffer(MAX_STRING_LENGTH);
   if (GET_LEVEL(ch) < LVL_IMMORT) 
      GET_MANA(ch) -= mana;
   send_to_char(ch, "You create a scroll of %s.\r\n",
	   spells[scroll].spell_name);
   act("$n creates a scroll!", FALSE, ch, 0, 0, TO_ROOM);
   extract_obj(paper);
   
   improve_skill(ch,SKILL_SCRIBE,USE_PASS);
   improve_skill(ch,scroll,USE_PASS);
   final_scroll = create_obj();
   
   final_scroll->item_number = NOTHING;
   IN_ROOM(final_scroll) = NOWHERE;

   garble_spell(scroll,buf);
   sprintf(buf2, "%s %s scroll", 
	   spells[scroll].spell_name, buf);
   final_scroll->name = str_dup(buf2);
   sprintf(buf2, "Some parchment inscribed with the runes '%s' lies here.",
	   buf);
   final_scroll->description = str_dup(buf2);
   sprintf(buf2, "a %s scroll", buf);
   final_scroll->short_description = str_dup(buf2);
   
   /* extra description coolness! */
   CREATE(new_descr, struct extra_descr_data, 1);
   new_descr->keyword = str_dup(final_scroll->name);
   sprintf(buf2, "It appears to be a %s scroll.", spells[scroll].spell_name);
   new_descr->description = str_dup(buf2);
   new_descr->next = NULL;
   final_scroll->ex_description = new_descr;
   
   GET_OBJ_TYPE(final_scroll) = ITEM_SCROLL;
   GET_OBJ_WEAR(final_scroll) = ITEM_WEAR_TAKE;
   GET_OBJ_EXTRA(final_scroll) = ITEM_NORENT;
   GET_OBJ_VAL(final_scroll, 0) = clevel;
   GET_OBJ_VAL(final_scroll, 1) = scroll;
   GET_OBJ_VAL(final_scroll, 2) = -1;
   GET_OBJ_VAL(final_scroll, 3) = -1;
/*    GET_OBJ_COST(final_scroll) = 0; */
/*    GET_OBJ_COST(final_scroll) = GET_LEVEL(ch) * 500; */
   GET_OBJ_WEIGHT(final_scroll) = 1;
   GET_OBJ_RENT(final_scroll) = 0;
   GET_OBJ_CSLOTS(final_scroll) = 10;
   GET_OBJ_OSLOTS(final_scroll) = 10;
   GET_OBJ_TSLOTS(final_scroll) = 10;
   GET_OBJ_TIMER(final_scroll) = 1152; /* 24 hours. */

   obj_to_char(final_scroll, ch);
   release_buffer(buf);
   release_buffer(buf2);
}


ACMD(do_scribe)
{
   struct obj_data *paper = NULL;
   struct obj_data *obj, *next_obj;
   char *paper_name = get_buffer(MAX_STRING_LENGTH);
   char *spell_name = get_buffer(MAX_STRING_LENGTH);
   char *temp1, *temp2;
   int scroll = 0, found = FALSE;
   int clevel=0;
   char cnum[4];
      
   temp1 = one_argument(argument, paper_name);
   
   /* sanity check */
   skip_spaces(&temp1);

   if (temp1) 
      {
      if(isdigit((int)temp1[0]))
	 {
	 cnum[0]=temp1[0];
	 if(isdigit((int)temp1[1]))
	    {
	    cnum[1]=temp1[1];
	    cnum[2]='\0';
	    }
	 else
	    cnum[1]='\0';
	 clevel=atoi(cnum);
	 temp2 = get_spell_name(temp1);
	 }
      else
	 temp2=strtok(temp1,"'");

      if (temp2)
	 strcpy(spell_name, temp2);
      }
   else 
      {
      paper_name[0] = '\0';
      spell_name[0] = '\0';
      }


   if (IS_NPC(ch)||!GET_SKILL(ch,SKILL_SCRIBE)||!SCR_SKILLCHECK(ch,SKILL_SCRIBE))
      {
      send_to_char(ch,"You have no idea how to scribe scrolls!\r\n");
      release_buffer(spell_name);
      release_buffer(paper_name);
      return;
      }
   if (!*paper_name || !*spell_name) 
      {
      int num=0;
      send_to_char(ch,"What do you wish to scribe where?\r\n");
      if(GET_LEVEL(ch)>=LVL_IMMORT)
	 {
	 while(scrolls[num].spell_num!=0)
	    {
	    send_to_char(ch," %2d: %-20.20s | %ld\r\n",num,
		    spells[scrolls[num].spell_num].spell_name,
		    scrolls[num].item_vnum);
	    num++;
	    }
	 }
      release_buffer(spell_name);
      release_buffer(paper_name);
      return;
      }

   for (obj = ch->carrying; obj; obj = next_obj) 
      {
      next_obj = obj->next_content;
      if (obj == NULL)
	 {
	 release_buffer(spell_name);
	 release_buffer(paper_name);
	 return;
	 }
      else if (!(paper = get_obj_in_list_vis(ch, paper_name,
					     ch->carrying))) 
	 continue;
      else
	 found = TRUE;
      }
   if (found && (GET_OBJ_TYPE(paper) != ITEM_NOTE)) 
      {
      send_to_char(ch,"You can't write on that!\r\n");
      release_buffer(spell_name);
      release_buffer(paper_name);
      return;
      }
   if (found == FALSE) 
      {
      send_to_char(ch, "You don't have %s in your inventory!\r\n",
	      paper_name);
      release_buffer(spell_name);
      release_buffer(paper_name);
      return;
      }

   if (!spell_name || !*spell_name) 
      {
      send_to_char(ch,"Spell names must be enclosed in single quotes!\r\n");
      release_buffer(spell_name);
      release_buffer(paper_name);
      return;
      } 

   scroll = find_skill_num(spell_name);	

   if ((scroll < 1) || (scroll >= MAX_SPELLS)) 
      {
      send_to_char(ch,"Scribe what spell?!?\r\n");
      release_buffer(spell_name);
      release_buffer(paper_name);
      return;
      }
   if (GET_LEVEL(ch) < spells[scroll].min_level[(int) GET_CLASS(ch)]) 
      {
      send_to_char(ch,"You are not schooled enough to cast that spell!\r\n");
      release_buffer(spell_name);
      release_buffer(paper_name);
      return;
      }
   if (GET_SKILL(ch, scroll) == 0) 
      {
      send_to_char(ch,"You don't know any spell like that!\r\n");
      release_buffer(spell_name);
      release_buffer(paper_name);
      return;
      }

   if(clevel==0)
      clevel=GET_SKILL(ch,scroll);
   clevel=MAX(1,MIN(10,clevel));

   make_scroll(ch, scroll, paper,clevel);
   release_buffer(spell_name);
   release_buffer(paper_name);
}


ACMD(do_forge)
{
   /* PLEASE NOTE!!!  This command alters the object_values of the target
      weapon, and this will save to the rent files.  It should not cause
      a problem with stock Circle, but if your weapons use the first 
      position [ GET_OBJ_VAL(weapon, 0); ], then you WILL have a problem.
      This command stores the character's level in the first value to 
      prevent the weapon from being "forged" more than once by mortals.
      Install at your own risk.  You have been warned...
      */
	
   struct obj_data *weapon = NULL;
   struct obj_data *obj, *next_obj;
   char *weapon_name=get_buffer(MAX_STRING_LENGTH);
   int found = FALSE, prob = 0, dam = 0;

   one_argument(argument, weapon_name);

   if (GET_LEVEL(ch) < LVL_ADMIN)
      {
      send_to_char(ch,"You have no idea how to forge weapons!\r\n");
      release_buffer(weapon_name);
      return;
      }
   if (!*weapon_name) 
      {
      send_to_char(ch,"What do you wish to forge?\r\n");
      release_buffer(weapon_name);
      return;
      }

   for (obj = ch->carrying; obj; obj = next_obj) 
      {
      next_obj = obj->next_content;
      if (obj == NULL)
	 {
	 release_buffer(weapon_name);
	 return;
	 }
      else if (!(weapon = get_obj_in_list_vis(ch, weapon_name, ch->carrying))) 
	 continue;
      else
	 found = TRUE;
      }
	
   if (found == FALSE) 
      {
      send_to_char(ch, "You don't have %s in your inventory!\r\n",
	      weapon_name);
      release_buffer(weapon_name);
      return;
      }

   if (found && (GET_OBJ_TYPE(weapon) != ITEM_WEAPON)) 
      {
      send_to_char(ch, "It doesn't look like %s would make a"
	      " good weapon...\r\n", weapon_name);
      release_buffer(weapon_name);
      return;
      }

   release_buffer(weapon_name);

   if ((GET_OBJ_VAL(weapon, 0) > 0) && (GET_LEVEL(ch) < LVL_IMMORT)) 
      {
      send_to_char(ch,"You cannot forge a weapon more than once!\r\n");
      return;
      }

   if (GET_OBJ_EXTRA(weapon) & ITEM_MAGIC) 
      {
      send_to_char(ch,"The weapon is imbued with magical powers beyond "
		   "your grasp.\r\nYou cannot further affect its form.\r\n");
      return;
      }

   /* determine success probability */
   prob += (GET_LEVEL(ch) << 1) + ((GET_DEX(ch) - 11) << 1);
   prob += ((GET_STR(ch) - 11) << 1) + (GET_ADD(ch) >> 3);

   if ((number(10, 100) > prob) && (GET_LEVEL(ch) < LVL_IMMORT)) 
      {
      send_to_char(ch,"As you pound out the dents in the weapon,"
		   " you hit a weak spot and it explodes!\r\n");
      act("$n tries to forge a weapon, but it explodes!",
	  FALSE, ch, 0,0, TO_ROOM);
      extract_obj(weapon);
      dam = number(20, 60);
      GET_HIT(ch) -= dam;
      update_pos(ch);
      return;
      }

   GET_OBJ_VAL(weapon, 1) += (GET_LEVEL(ch) % 3) + number(-1, 2); 
   GET_OBJ_VAL(weapon, 2) += (GET_LEVEL(ch) % 2) + number(-1, 2);
   GET_OBJ_RENT(weapon) += (GET_LEVEL(ch) << 3);
   GET_OBJ_VAL(weapon, 0) = GET_LEVEL(ch);             /* level or forger */
   GET_OBJ_VAL(weapon, 6) += 1;                        /* number of times weapon forged */
   GET_OBJ_VAL(weapon, 7) = GET_IDNUM(ch);             /* idnum of forger */


   send_to_char(ch,"You have forged new life into the weapon!\r\n");
   act("$n vigorously pounds on a weapon!",
       FALSE, ch, 0, 0, TO_ROOM);
}

ACMD(do_restring)
{
  char obj_alias[ 4096 ] = { '\0' };
  char field_name[ 4096 ] = { '\0' };
  char new_text[ 4096 ] = { '\0' };
  char buf[ 4096 ] = { '\0' };
  char buffer2[ 4096 ] = { '\0' };
  struct obj_data* p_obj = NULL;

  if ( IS_NPC( ch ) )
  {
    return;
  }
  if ( GET_LEVEL( ch ) < LVL_ADMIN )
  {
    send_to_char( ch, "Huh?\r\n" );
    return;
  }

  half_chop( argument, obj_alias, buf );
  half_chop( buf, field_name, new_text );

  printf( "Restring: obj_alias=%s, field_name=%s, new_text=%s\n", obj_alias, field_name, new_text );

  if( !strlen( obj_alias ) // no object alias given
      || !strlen( field_name ) // no field name given
      || ( strcmp( field_name, "name" ) && strcmp( field_name, "aliases" ) && strcmp( field_name, "l-desc" ) && strcmp( field_name, "e-desc" ) ) // the field name isn't "name", "aliases", "l-desc", or "e-desc"
      || ( strcmp( field_name, "e-desc" ) && !strlen( new_text ) ) // field isn't e-desc and new-text wasn't specified.
    )
  {
    send_to_char( ch, "Usage: restring <object> <field> [new-text]\r\n" );
    send_to_char( ch, "\r\n" );
    send_to_char( ch, "  object  : An alias of the object in your inventory to restring (e.g. cloak)\r\n" );
    send_to_char( ch, "  field   : Either 'name', 'aliases', 'l-desc', or 'e-desc' (see below)\r\n" );
    send_to_char( ch, "  new-text: The new string for the object.  Used when field is not 'e-desc'.\r\n" );
    send_to_char( ch, "\r\n" );
    send_to_char( ch, "  name   : the cloak of Celebrimbor\r\n" );
    send_to_char( ch, "  aliases: Celebrimbor cloak\r\n" );
    send_to_char( ch, "  l-desc : A pile of silver leaves is attached to some cloth here.\r\n" );
    send_to_char( ch, "  e-desc : Black as night silk cloth embellished with gleaming silver leaves arranged\r\n"
                      "    and hand stitched in rows, this is truly a gift of the Gods.  Each leaf\r\n"
                      "    appears to be made of mithril but intuition says it grew on some tree\r\n"
                      "    somewhere in this world or the next.\r\n" );
    send_to_char( ch, "\r\n" );
    return;
  }

  p_obj = get_obj_in_list_vis( ch, obj_alias, ch->carrying );
  if (p_obj == NULL)
  {
    send_to_char( ch, "You don't seem to have a %s.\r\n", obj_alias);
    return;
  }

  SET_BIT( GET_OBJ_EXTRA( p_obj ), ITEM_UNIQUE_SAVE );

  sprintf( buffer2, "(GC) %s restring %s of %s (%d): '%s'",
           GET_NAME( ch ), field_name, GET_OBJ_NAME( p_obj ), GET_OBJ_VNUM( p_obj ), new_text );
  mudlog( buffer2, BRF, GOD_LOG(ch), TRUE );

  if( strcmp( field_name, "name" ) == 0 )
  {
    p_obj->short_description = str_dup( new_text );
  }
  else if( strcmp( field_name, "aliases" ) == 0 )
  {
    p_obj->name = str_dup( new_text );
  }
  else if( strcmp( field_name, "l-desc" ) == 0 )
  {
    p_obj->description = str_dup( new_text );
  }
  else if( strcmp( field_name, "e-desc" ) == 0 )
  {
    p_obj->ex_description = ( struct extra_descr_data* )malloc( sizeof( struct extra_descr_data ) );
    p_obj->ex_description->keyword = str_dup( p_obj->name );
    p_obj->ex_description->description = ( char* )calloc( 2048, 1 );
    p_obj->ex_description->next = NULL;

    ch->desc->backstr = NULL;
    send_to_char(ch, "Write the new extra-description string.  (/s saves /h for help)\r\n");
    act("$n begins to jot down a restring.", TRUE, ch, 0, 0, TO_ROOM);
    string_write( ch->desc, &p_obj->ex_description->description, 1024, 0, NULL );
  }
}
