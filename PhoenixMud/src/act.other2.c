/* ************************************************************************
*   File: act.other.c                                   Part of CircleMUD * 
*  Usage: Miscellaneous player-level commands                             * 
*                                                                         * 
*  All rights reserved.  See license.doc for complete information.        * 
*                                                                         * 
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University * 
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               * 
************************************************************************ */

#define __ACT_OTHER_C__
#include "../localHeader/conf.h"
#include "../localHeader/sysdep.h"

#include "structs.h"
#include "buffer.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "house.h"
#include "ident.h"
#include <sys/stat.h>
#include "clan.h"
#include "constants.h"

/* extern variables */
extern struct zone_data *zone_table;
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct room_data *world;
extern struct spell_info_type *spells;
extern struct index_data *mob_index;
extern char *class_abbrevs[];
extern const float race_exp_multipliers[];
extern const float class_exp_multipliers[];
extern const int exp_table[];
extern const char *race_abbrevs[];
extern int free_rent;
extern int pt_allowed;
extern int nameserver_is_slow;
extern int max_filesize;
extern char *pc_race_types[];
extern char *pc_class_types[];
extern char *scr_male_pc_class_types[];
extern char *scr_female_pc_class_types[];
extern int max_obj_save;
extern struct battle_zone battle;
extern int track_through_doors;
extern int max_obj_house;
extern struct house_control_rec house_control[];
extern int num_of_houses;
extern int xap_objs;
extern struct autoauction aauction;
extern char *scr_class_abbrevs[];
extern struct player_shop* player_shops;
extern struct index_data *obj_index;
extern struct char_data *character_list;

extern const long race_stats[NUM_RACES][6];
int parse_xap_obj(char*, struct obj_data**, char*, FILE*, int, int*);
void prune_crlf(char* txt);
int get_shop_item_count(struct player_shop*);
void save_char_ascii(struct char_file_u*);
int is_name(const char*, const char*);


ACMD(do_shop);

#define PLAYER_SHOP_INDEX "etc/shops.txt"
#define PLAYER_SHOP_DIR "etc/plrshops"

float sales_tax = 0.10f;
int latest_month_rent_collected = -1;

struct player_shop* player_shops = NULL;

void write_player_shop_index()
{
  FILE* fp = fopen(PLAYER_SHOP_INDEX, "w");
  if (!fp) {
    mudlogf(CMP, LVL_IMMORT, TRUE, "SYSERR: could not open %s for writing", PLAYER_SHOP_INDEX);
    return;
  }
  fprintf(fp, "%f\n", sales_tax);
  fprintf(fp, "%d\n", latest_month_rent_collected);
  struct player_shop* shop = player_shops;
  for (; shop; shop = shop->next) {
    fprintf(fp, "%s\n", shop->player_name);
    fprintf(fp, "%d\n", shop->vnum_location);
    fprintf(fp, "%d\n", shop->rent);
    fprintf(fp, "%d\n", shop->is_active);
  }
  fclose(fp);
}




void load_player_shop(struct player_shop* shop)
{
  int version = 2;
  struct obj_data* temp;
  char filename[1024];
  char line[2048];
  int locate = 1;

  shop->contents = NULL;

  sprintf(filename, "%s/%c%s.txt", PLAYER_SHOP_DIR, toupper(shop->player_name[0]), shop->player_name+1);
  FILE* fp = fopen(filename, "r");
  if (!fp) {
    shop->contents = NULL;
    mudlogf(CMP, LVL_IMMORT, TRUE, "SYSERR: Could not open %s for reading", filename);
    return;
  }

  if(!feof(fp))
    get_line(fp, line);
  while (!feof(fp)) {
    temp = NULL;
    int amount = atoi(line);
    get_line(fp, line);
    if (parse_xap_obj(filename,&temp,line,fp,version,&locate)) {
      if(temp == NULL) {
	continue;
      }     
      struct shop_item* item = (struct shop_item*)malloc(sizeof(struct shop_item));
      item->item = temp;
      item->amount = amount;
      item->next = shop->contents;
      shop->contents = item;
    } else {
      if(!feof(fp) && !ferror(fp))
	get_line(fp,line);
      else
	break;
    }
  }
}



void load_player_shops()
{
  FILE* fp = fopen(PLAYER_SHOP_INDEX, "r");
  if (!fp) {
    mudlogf(CMP, LVL_IMMORT, TRUE, "SYSERR: could not open %s for reading", PLAYER_SHOP_INDEX);
    return;
  }
  char buf[1024];
  fgets(buf, 1023, fp);
  sales_tax = atof(buf);
  fgets(buf, 1023, fp);
  latest_month_rent_collected = atoi(buf);
  mudlogf(CMP, LVL_IMMORT, TRUE, "PLAYER_SHOP: sales_tax=%f, latest_month_rent_collected=%d\n", sales_tax, latest_month_rent_collected);
  
  while (fgets(buf, 1023, fp)) {
    struct player_shop* shop = (struct player_shop*)malloc(sizeof(struct player_shop));
    prune_crlf(buf);
    strcpy(shop->player_name, buf);
    fgets(buf, 1023, fp);
    shop->vnum_location = atoi(buf);
    fgets(buf, 1023, fp);
    shop->rent = atoi(buf);
    fgets(buf, 1023, fp);
    shop->is_active = atoi(buf);
    shop->next = player_shops;
    player_shops = shop;
    shop->contents = NULL;
    load_player_shop(shop);
    mudlogf(CMP, LVL_IMMORT, TRUE, "PLAYER_SHOP: Loaded shop for %s at room %ld, rent=%d, is active=%d, items for sale=%d", shop->player_name, shop->vnum_location, shop->rent, shop->is_active, get_shop_item_count(shop));
  }
}

struct player_shop* find_player_shop(char* player_name)
{
  struct player_shop* shop = player_shops;
  for (; shop; shop = shop->next) {
    if (str_cmp(shop->player_name, player_name) == 0) {
      return shop;
    }
  }
  return NULL;
}

struct player_shop* find_player_shop_by_room(int rvnum)
{
  struct player_shop* shop = player_shops;
  for (; shop; shop = shop->next) {
    if (shop->vnum_location == rvnum) {
      return shop;
    }
  }
  return NULL;
}

int create_player_shop(struct char_data* ch, char* player_name, room_vnum vnum_location, int rent)
{
  if (find_player_shop(player_name)) {
    send_to_char(ch, "That player already has a shop.\r\n");
    return 0;
  }
  if (find_player_shop_by_room(vnum_location)) {
    send_to_char(ch, "That room already has a shop.\r\n");
    return 0;
  }
  struct player_shop* shop = (struct player_shop*)malloc(sizeof(struct player_shop));
  strncpy(shop->player_name, player_name, 63);
  shop->player_name[63] = '\x0';
  shop->is_active = 1;
  shop->vnum_location = vnum_location;  
  shop->rent = rent;
  shop->next = player_shops;
  shop->contents = NULL;
  player_shops = shop;
  write_player_shop_index();
  return 1;
}

int delete_player_shop(struct char_data* ch, char* player_name, room_vnum vnum_location)
{
  struct player_shop* shop = find_player_shop(player_name);
  if (!shop) {
    send_to_char(ch, "That player doesn't have a shop.\r\n");
    return 0;
  }
  if (shop->vnum_location != vnum_location) {
    send_to_char(ch, "That player's shop is not at that location.\r\n");
    return 0;
  }
  if (shop->contents) {
    send_to_char(ch, "The shop has items for sale.  Remove all items before deleting.\r\n");
    return 0;
  }
  if (shop == player_shops) {
    player_shops = player_shops->next;
    free(shop);
    write_player_shop_index();
    return 1;
  }
  struct player_shop* curr = player_shops;
  struct player_shop* prev = curr;
  while (curr && curr != shop) {
    prev = curr;
    curr = curr->next;
  }
  if (curr == shop) {
    prev->next = curr->next;
    free(curr);
    write_player_shop_index();
    return 1;
  }
  return 0;
}

void save_player_shop(struct player_shop* shop)
{
  char buf[1024];
  sprintf(buf, "%s/%c%s.txt", PLAYER_SHOP_DIR, toupper(shop->player_name[0]), shop->player_name+1);
  FILE* fp = fopen(buf, "w");
  if (!fp) {
    mudlogf(CMP, LVL_IMMORT, TRUE, "SYSERR: Could not save %s's shop.", shop->player_name);
    return;
  }
  struct shop_item* item = shop->contents;
  for (; item; item = item->next) {
    fprintf(fp, "%d\n", item->amount);
    my_obj_save_to_disk(fp, item->item, 0);
  }
  fclose(fp);
}

void add_item_to_player_shop(struct char_data* ch, struct player_shop* shop, struct obj_data* obj, int amount)
{
  struct shop_item* item = (struct shop_item*)malloc(sizeof(struct shop_item));
  obj_from_char(obj);
  item->item = obj;
  item->amount = amount;
  item->next = shop->contents;
  shop->contents = item;
  mudlogf(CMP, LVL_IMMORT, TRUE, "PLAYER_SHOP: %s added %s for %d coins.", GET_NAME(ch), GET_OBJ_NAME(obj), amount);
  save_player_shop(shop);
  Crash_crashsave(ch); /* Prevent obj dup. */
}

struct obj_data* remove_item_from_player_shop(struct char_data* ch, struct player_shop* shop, int num)
{
  struct shop_item* item = shop->contents;
  if (num == 0) {
    struct obj_data* obj = item->item;
    obj_to_char(obj, ch);
    shop->contents = shop->contents->next;
    free(item);
    save_player_shop(shop);
    Crash_crashsave(ch); /* Prevent obj dup. */
    return obj;
  }
  int i;
  struct shop_item* prev = item;
  for (i = 0; i < num && item; i++) {
    prev = item;
    item = item->next;
  }
  if (i != num) {
    send_to_char(ch, "Error?  Object not removed.\r\n");
    return NULL;
  }
  struct obj_data* obj = item->item;
  obj_to_char(obj, ch);
  prev->next = item->next;
  free(item);
  save_player_shop(shop);
  Crash_crashsave(ch); /* Prevent obj dup. */
  return obj;
}

int get_shop_item_count(struct player_shop* shop)
{
  int count = 0;
  struct shop_item* item = shop->contents;
  for (; item; item = item->next) {
    count++;
  }
  return count;
}

struct shop_item* get_player_shop_item(struct player_shop* shop, int num)
{
  int count = 0;
  struct shop_item* item = shop->contents;
  for (; item && count < num; item = item->next) {
    count++;
  }
  if (count == num) {
    return item;
  }
  return NULL;
}

void pay_player_shop(struct player_shop* shop, int cost)
{
  /* Try to get the victim from active player list. */
  struct char_data* i;
  for (i = character_list; i; i = i->next) {
    if (str_cmp(shop->player_name, i->player.name) == 0) {
      GET_BANK_GOLD(i) += cost;
      mudlogf(CMP, LVL_IMMORT, TRUE, "PLAYER_SHOP: Paid %s %d gold coins (player is online).", GET_NAME(i), cost);
      return;
    }
  }
  /* Try to get the victim from the player file. */
  struct char_file_u tmp_store;
  char name[1024];
  sprintf(name, "%c%s", toupper(shop->player_name[0]), shop->player_name+1);
  if (load_char(name, &tmp_store) > -1) {
    GET_BANK_GOLD((&tmp_store)) += cost;
    save_char_ascii(&tmp_store);
    mudlogf(CMP, LVL_IMMORT, TRUE, "PLAYER_SHOP: Paid %s %d gold coins (from file).", name, cost);
    return;
  } else {
    mudlogf(CMP, LVL_IMMORT, TRUE, "SYSERR: pay_player_shop could not find player %s.", name);
    return;
  }
}


ACMD(do_shop)
{
  if (IS_NPC(ch)) {
    return;
  }

  char buffer1[4096];
  char buffer2[4096];
  char command[4096] = {'\x0'};
  char arg1[1024] = {'\x0'};
  char arg2[1024] = {'\x0'};
  half_chop(argument, command, buffer1);
  half_chop(buffer1, arg1, buffer2);
  half_chop(buffer2, arg2, buffer1);

  /* Put all admin commands here. */
  if (GET_LEVEL(ch) >= LVL_ADMIN) {
    if (str_cmp(command, "create") == 0) {
      char arg3[1024] = {'\x0'};
      half_chop(buffer1, arg3, buffer2);
      if (strlen(arg1) == 0 || strlen(arg2) == 0 || strlen(arg3) == 0) {
	send_to_char(ch, "Usage: shop create <player_name> <room_vnum> <rent>\r\n");
	return;
      }
      if (!create_player_shop(ch, arg1, atoi(arg2), atoi(arg3))) {
	return;
      }
      mudlogf(CMP, LVL_IMMORT, TRUE,
	      "PLAYER_SHOP: %s created a shop for %s at room %d with monthly rent %d.",
	      GET_NAME(ch), arg1, atoi(arg2), atoi(arg3));
      return;
    } else if (str_cmp(command, "delete") == 0) {
      if (strlen(arg1) == 0 || strlen(arg2) == 0) {
	send_to_char(ch, "Usage: shop delete <player_name> <room_vnum>\r\n");
	return;
      }
      if (!delete_player_shop(ch, arg1, atoi(arg2))) {
	return;
      }
      mudlogf(CMP, LVL_IMMORT, TRUE,
	      "PLAYER_SHOP: %s deleted shop for %s at room %d.",
	      GET_NAME(ch), arg1, atoi(arg2));
      return;
    } else if (str_cmp(command, "active") == 0) {
      if (strlen(arg1) == 0 || strlen(arg2) == 0) {
        send_to_char(ch, "Usage: shop active <player_name> <\"true\"|\"false\">\r\n");
        return;
      }
      struct player_shop* shop = find_player_shop(arg1);
      if (!shop) {
	send_to_char(ch, "No shop by that name.\r\n");
	return;
      }
      if (str_cmp(arg2, "false") == 0) {
	shop->is_active = 0;
      } else {
	shop->is_active = 1;
      }
      mudlogf(CMP, LVL_IMMORT, TRUE,
              "PLAYER_SHOP: %s set %s shop to %s.",
              GET_NAME(ch), arg1, shop->is_active ? "active" : "inactive");
      write_player_shop_index();
      return;
    } else if (str_cmp(command, "remove") == 0) {
      /* This is the imm-version of the command.  Check the shop the imm is standing in. */
      struct player_shop* shop = find_player_shop_by_room(GET_ROOM_VNUM(IN_ROOM(ch)));
      if (!shop) {
	send_to_char(ch, "This room isn't a shop.\r\n");
	return;
      }
      if (strlen(arg1) < 2 || arg1[0] != '#') {
	send_to_char(ch, "Usage: shop remove #<item_number>\r\n  Example: shop remove #4\r\n  Note: this is the imm version of the command.\r\n");
	return;
      }
      int num = atoi(arg1+1);
      num--;
      if (num < 0 || num >= get_shop_item_count(shop)) {
	send_to_char(ch, "No item for sale by that number.\r\n");
	return;
      }
      struct obj_data* obj = remove_item_from_player_shop(ch, shop, num);
      send_to_char(ch, "You remove %s from %c%s's shop.\r\n", GET_OBJ_NAME(obj), toupper(shop->player_name[0]), shop->player_name+1);
      char buf2[1024];
      sprintf(buf2, "$n removes %s from %c%s's shop.", GET_OBJ_NAME(obj), toupper(shop->player_name[0]), shop->player_name+1);
      act(buf2, TRUE, ch, 0, 0, TO_ROOM);
      mudlogf(CMP, LVL_IMMORT, TRUE, "PLAYER_SHOP: %s removed %s from %c%s's shop.", GET_NAME(ch), GET_OBJ_NAME(obj), toupper(shop->player_name[0]), shop->player_name+1);
      return;
    } else if (str_cmp(command, "sales_tax") == 0) {
      if (strlen(arg1) == 0) {
	send_to_char(ch, "Usage: shop sales_tax <percent>\r\n  Example: shop sales_tax 10\r\n");
	return;
      }
      sales_tax = atof(arg1)/100.0f;
      write_player_shop_index();
      mudlogf(CMP, LVL_IMMORT, TRUE,
              "PLAYER_SHOP: %s set sales tax to %f.",
              GET_NAME(ch), sales_tax);
    } else if (str_cmp(command, "rent") == 0) {
      if (strlen(arg1) == 0 || strlen(arg2) == 0) {
	send_to_char(ch, "Usage: shop rent <player> <amount>\r\n");
	return;
      }
      struct player_shop* shop = find_player_shop(arg1);
      if (!shop) {
	send_to_char(ch, "No shop by that name.\r\n");
	return;
      }
      int amount = atoi(arg2);
      if (amount < 0 || amount > 50000000) {
	send_to_char(ch, "Monthly rent must be a positive number less than 50 million coins.\r\n");
	return;
      }
      mudlogf(CMP, LVL_IMMORT, TRUE, "PLAYER_SHOP: %s changed %s's rent to %d (from %d).", GET_NAME(ch), shop->player_name, amount, shop->rent);
      shop->rent = amount;
      write_player_shop_index();
      return;
    }
  }

  /* Put all player commands here. */
  if (str_cmp(command, "list") == 0) {
    struct player_shop* shop = find_player_shop_by_room(GET_ROOM_VNUM(IN_ROOM(ch)));
    if (!shop) {
      send_to_char(ch, "This isn't a shop.\r\n");
      return;
    }
    sprintf(buffer1, "                     %c%s's Shop\r\n\r\n", toupper(shop->player_name[0]), shop->player_name+1);
    send_to_char(ch, "%s", buffer1);
    send_to_char(ch, "     Item                                    Cost          Condition         Wear\r\n");
    send_to_char(ch, "---------------------------------------------------------------------------------\r\n");
    struct shop_item* item = shop->contents;
    int i = 1;
    for (; item; item = item->next) {
      struct obj_data* obj = item->item;

      if (strlen(arg1) > 0 && !is_name(arg1, obj->name)) {
	/* If the player used a filter, and the object doesn't match, skip the object. */
	continue;
      }
	    
      int percent;
      if(GET_OBJ_TSLOTS(obj) == 0)
	    percent = 0;
      else
	    /* Base object status from original, not total - Nomikos 5/10/2025 */
	    percent = (GET_OBJ_CSLOTS(obj) * 100) / GET_OBJ_OSLOTS(obj);
	
      int wpercent;
      if(GET_OBJ_OSLOTS(obj) == 0)
	    wpercent = 0;
      else
	    wpercent = (GET_OBJ_TSLOTS(obj) * 100) / GET_OBJ_OSLOTS(obj);
	
      int condition;
      if ((GET_OBJ_CSLOTS(obj) == 0) && (GET_OBJ_TSLOTS(obj) == 0))
	    condition = 0;
      else if (GET_OBJ_CSLOTS(obj) < 0)
	    /* Added a broken status at position 1 - Nomi 5/10/25 */
	    condition = 1;
      else
	    /* Position 2 is the lowest before it breaks */
	    condition = (percent / 10) + 2;
	
      int wcondition;
      if ((GET_OBJ_TSLOTS(obj) == 0) && (GET_OBJ_OSLOTS(obj) == 0))
	    wcondition = 0;
      else
	    wcondition = (wpercent / 10) + 1;

      sprintf(buffer1, " %3d) %-40.40s  %8d   %-15.15s   %-15.15s\r\n",
	      i, GET_OBJ_NAME(obj), item->amount,
	      item_condition_no_color[condition], item_wear_no_color[wcondition]);
      i++;
      send_to_char(ch, "%s", buffer1);
    }    
    return;
  } else if (str_cmp(command, "add") == 0) {
    struct player_shop* shop = find_player_shop(GET_NAME(ch));
    if (!shop) {
      send_to_char(ch, "You don't own a shop.  Try \"help shop\"?\r\n");
      return;
    }
    if (!shop->is_active) {
      send_to_char(ch, "Your shop has been closed.  Please see an immortal for help.\r\n");
      return;
    }
    if (shop->vnum_location != GET_ROOM_VNUM(IN_ROOM(ch))) {
      send_to_char(ch, "You must be in your shop to add items.\r\n");
      return;
    }
    if (strlen(arg1) == 0 || strlen(arg2) == 0) {
      send_to_char(ch, "Usage: shop add <item> <amount>\r\n");
      return;
    }
    struct obj_data *obj = get_obj_in_list_vis(ch, arg1, ch->carrying);
    if (!obj) {
      send_to_char(ch, "You aren't carrying %s.\r\n", arg1);
      return;
    }
    if (get_shop_item_count(shop) >= 100) {
      send_to_char(ch, "Your shop has no more room for items.\r\n");
      return;
    }
    if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && obj->contains) {
      send_to_char(ch, "You can't sell containers with items in them.\r\n");
      return;
    }
    if (IS_OBJ_STAT(obj, ITEM_NORENT)) {
      send_to_char(ch, "You can't sell no-rent items.\r\n");
      return;
    }
    if (GET_OBJ_TYPE(obj) == ITEM_KEY) {
      send_to_char(ch, "You can't sell keys.\r\n");
      return;
    }
    int amount = atoi(arg2);
    if (amount < 10) {
      send_to_char(ch, "You can't add an item for less than 10 coins.\r\n");
      return;
    }
    if (amount > 40000000) {
      send_to_char(ch, "You can't add an item for more than 40 million coins.\r\n");
      return;
    }
    add_item_to_player_shop(ch, shop, obj, amount);
    send_to_char(ch, "You add %s for sale asking %d coins.\r\n", GET_OBJ_NAME(obj), amount);
    char buf2[1024];
    sprintf(buf2, "$n adds %s to the shop for %d coins.", GET_OBJ_NAME(obj), amount);
    act(buf2, TRUE, ch, 0, 0, TO_ROOM);
    return;
  } else if (str_cmp(command, "remove") == 0) {  
    struct player_shop* shop = find_player_shop(GET_NAME(ch));
    if (!shop) {
      send_to_char(ch, "You don't own a shop.  Try \"help shop\"?\r\n");
      return;
    }
    if (GET_ROOM_VNUM(IN_ROOM(ch)) != shop->vnum_location) {
      send_to_char(ch, "You must be in your shop to remove items.\r\n");
      return;
    }
    if (strlen(arg1) < 2 || arg1[0] != '#') {
      send_to_char(ch, "Usage: shop remove #<item_number>\r\n  Example: shop remove #4");
      return;
    }
    int num = atoi(arg1+1);
    num--;
    if (num < 0 || num >= get_shop_item_count(shop)) {
      send_to_char(ch, "No item for sale by that number.\r\n");
      return;
    }
    struct obj_data* obj = remove_item_from_player_shop(ch, shop, num);
    send_to_char(ch, "You remove %s from your shop.\r\n", GET_OBJ_NAME(obj));
    char buf2[1024];
    sprintf(buf2, "$n removes %s from the shop.", GET_OBJ_NAME(obj));
    act(buf2, TRUE, ch, 0, 0, TO_ROOM);
    mudlogf(CMP, LVL_IMMORT, TRUE, "PLAYER_SHOP: %s removed %s.", GET_NAME(ch), GET_OBJ_NAME(obj));
    return;
  } else if (str_cmp(command, "buy") == 0) {
    struct player_shop* shop = find_player_shop_by_room(GET_ROOM_VNUM(IN_ROOM(ch)));
    if (!shop) {
      send_to_char(ch, "This isn't a shop.\r\n");
      return;
    }
    if (!shop->is_active) {
      send_to_char(ch, "This shop has been closed.\r\n");
      return;
    }
    if (strlen(arg1) < 2 || arg1[0] != '#') {
      send_to_char(ch, "Usage: shop buy #<item_number>\r\n  Example: shop buy #4");
      return;
    }
    int num = atoi(arg1+1);
    num--;
    if (num < 0 || num >= get_shop_item_count(shop)) {
      send_to_char(ch, "No item for sale by that number.\r\n");
      return;
    }
    struct shop_item* item = get_player_shop_item(shop, num);
    if (!item) {
      send_to_char(ch, "No, really.  No item for sale by that number.\r\n");
      return;
    }
    int cost = item->amount;
    if (GET_GOLD(ch) < cost) {
      send_to_char(ch, "You don't have enough money.\r\n");
      return;
    }
    GET_GOLD(ch) -= cost;
    pay_player_shop(shop, (int)(1+cost*(1.0f-sales_tax)));
    struct obj_data* obj = remove_item_from_player_shop(ch, shop, num);
    send_to_char(ch, "You buy %s from the shop for %d coins.\r\n", GET_OBJ_NAME(obj), cost);
    char buf2[1024];
    sprintf(buf2, "$n buys %s from the shop for %d coins.", GET_OBJ_NAME(obj), cost);
    act(buf2, TRUE, ch, 0, 0, TO_ROOM);
    mudlogf(CMP, LVL_IMMORT, TRUE, "PLAYER_SHOP: %s bought %s from %s's shop for %d coins.",
	GET_NAME(ch), GET_OBJ_NAME(obj), shop->player_name, cost);
    return;
  }

  send_to_char(ch, "Try \"help shop\"?\r\n");
  return;
}

void player_shop_monthly_rent_check()
{
  /*log("player_shop_monthly_rent_check");*/

  time_t t = time(0);
  struct tm* tmp = localtime(&t);

  int monthyear = tmp->tm_mon + 12*tmp->tm_year;
  /*int monthyear = tmp->tm_min;*/

  if (monthyear != latest_month_rent_collected) {
    mudlogf(CMP, LVL_IMMORT, TRUE,
	    "PLAYER_SHOP: Collecting monthly rent.");
    struct player_shop* shop = player_shops;
    for (; shop; shop = shop->next) {
      mudlogf(CMP, LVL_IMMORT, TRUE,
	      "PLAYER_SHOP: Collecting monthly rent from %s (%d coins).", shop->player_name, shop->rent);
      if (!shop->is_active) {
	mudlogf(CMP, LVL_IMMORT, TRUE,
		"PLAYER_SHOP: Skipping closed shop (%s).", shop->player_name);
	continue;
      }
      int rent_paid = 0;
      /* Try to get the victim from active player list. */
      struct char_data* i;
      for (i = character_list; i; i = i->next) {
	if (isname(shop->player_name, i->player.name)) {
	  rent_paid = 1;
	  GET_BANK_GOLD(i) -= shop->rent;
	  if (GET_BANK_GOLD(i) < 0) {
	    shop->is_active = 0;
	    mudlogf(CMP, LVL_IMMORT, TRUE, "PLAYER_SHOP: Closing %s's shop due to insufficient rent payment.", shop->player_name);
	    send_to_char(i, "Your shop has been closed do to insufficient funds.  Please see an immortal.\r\n");
	  } else {
	    send_to_char(i, "Collected monthly shop rent of %d coins.\r\n", shop->rent);
	  }
          GET_BANK_GOLD(i) = GET_BANK_GOLD(i) < 0 ? 0 : GET_BANK_GOLD(i);
	  break;
	}
      }
      /* Try to get the victim from the player file. */
      if (!rent_paid) {
	struct char_file_u tmp_store;
	char name[1024];
	sprintf(name, "%c%s", toupper(shop->player_name[0]), shop->player_name+1);
	if (load_char(name, &tmp_store) > -1) {
          GET_BANK_GOLD(&tmp_store) -= shop->rent;
          if (GET_BANK_GOLD(&tmp_store) < 0) {
            shop->is_active = 0;
            mudlogf(CMP, LVL_IMMORT, TRUE,
                    "PLAYER_SHOP: Closing %s's shop due to insufficient rent payment.", shop->player_name);
          }
          GET_BANK_GOLD(&tmp_store) = GET_BANK_GOLD(&tmp_store) < 0 ? 0 : GET_BANK_GOLD(&tmp_store);
	  save_char_ascii(&tmp_store);
	} else {
	  mudlogf(CMP, LVL_IMMORT, TRUE, "SYSERR: monthly player shop rent, could not find player %s.", name);
	}
      }
    }
    latest_month_rent_collected = monthyear;
    write_player_shop_index();
  }
}

struct player_shop* is_object_in_player_shop(struct obj_data* obj)
{
  struct player_shop* shop = player_shops;
  for (; shop; shop = shop->next) {
    struct shop_item* item = shop->contents;
    for (; item; item = item->next) {
      if (item->item == obj) {
	return shop;
      }
    }
  }
  return NULL;
}

int map_stat_string_to_apply(char* token)
{
  if (str_cmp(token, "str") == 0) {
    return APPLY_STR;
  } else if (str_cmp(token, "int") == 0) {
    return APPLY_INT;
  } else if (str_cmp(token, "wis") == 0) {
    return APPLY_WIS;
  } else if (str_cmp(token, "dex") == 0) {
    return APPLY_DEX;
  } else if (str_cmp(token, "con") == 0) {
    return APPLY_CON;
  } else if (str_cmp(token, "cha") == 0) {
    return APPLY_CHA;
  } else {
    return -1;
  }
}

int get_stat_points_by_attribute(int apply)
{
  switch (apply) {
  case APPLY_STR: return 2;
  case APPLY_INT: return 2;
  case APPLY_WIS: return 2;
  case APPLY_DEX: return 2;
  case APPLY_CON: return 3;
  case APPLY_CHA: return 1;
  default: return 1000;
  }
}

ACMD(do_retool)
{
  if (IS_NPC(ch) || GET_LEVEL(ch) > 20 || REMORT_LEVEL(ch) > 0) {
    send_to_char(ch, "You can't retool your stats anymore.\r\n");
    return;
  }

  if (!argument || !*argument) {
    send_to_char(ch, "Usage: retool <stat1> <adjust1> <stat2> <adjust2>\r\n");
    send_to_char(ch, "  type \"help retool\"\r\n");
    return;
  }

  char* stat1 = strtok(argument, " ");
  char* value1 = strtok(NULL, " ");
  char* stat2 = strtok(NULL, " ");
  char* value2 = strtok(NULL, " ");

  if (!stat1 || !value1 || !stat2 || !value2) { 
    send_to_char(ch, "Usage: retool <stat1> <adjust1> <stat2> <adjust2>\r\n");
    send_to_char(ch, "  type \"help retool\"\r\n");
    return;
  }

  int s1 = map_stat_string_to_apply(stat1);
  int s2 = map_stat_string_to_apply(stat2);
  int v1 = atoi(value1);
  int v2 = atoi(value2);
  if (s1 < 0 || s2 < 0 || s1 == s2 || abs(v1) > 25 || abs(v2) > 25) {
    send_to_char(ch, "Usage: retool <stat1> <adjust1> <stat2> <adjust2>\r\n");
    send_to_char(ch, "  type \"help retool\"\r\n");
    return;
  }

  int i;
  for (i = 0; i < NUM_WEARS; i++) {
    if (GET_EQ(ch, i) && i != WEAR_HEART) {
      send_to_char(ch, "You have to be completely naked for this to work.\r\n");
      return;
    }
  }

  int delta = v1 * get_stat_points_by_attribute(s1)
            + v2 * get_stat_points_by_attribute(s2);
  if (delta != 0) {
    send_to_char(ch, "The change in stat point values must sum to zero.\r\n");
    send_to_char(ch, "  type \"help retool\"\r\n");
    return;
  }

  int add_str = (s1 == APPLY_STR ? v1 : (s2 == APPLY_STR ? v2 : 0));
  int add_int = (s1 == APPLY_INT ? v1 : (s2 == APPLY_INT ? v2 : 0));
  int add_wis = (s1 == APPLY_WIS ? v1 : (s2 == APPLY_WIS ? v2 : 0));
  int add_dex = (s1 == APPLY_DEX ? v1 : (s2 == APPLY_DEX ? v2 : 0));
  int add_con = (s1 == APPLY_CON ? v1 : (s2 == APPLY_CON ? v2 : 0));
  int add_cha = (s1 == APPLY_CHA ? v1 : (s2 == APPLY_CHA ? v2 : 0));

  affect_remove_all(ch);

  if (   ch->real_abils.str + add_str < race_stats[GET_RACE(ch)][0]
      || ch->real_abils.str + add_str > race_max_stats[GET_RACE(ch)][0]
      || ch->real_abils.intel + add_int < race_stats[GET_RACE(ch)][1]
      || ch->real_abils.intel + add_int > race_max_stats[GET_RACE(ch)][1]
      || ch->real_abils.wis + add_wis < race_stats[GET_RACE(ch)][2]
      || ch->real_abils.wis + add_wis > race_max_stats[GET_RACE(ch)][2]
      || ch->real_abils.dex + add_dex < race_stats[GET_RACE(ch)][3]
      || ch->real_abils.dex + add_dex > race_max_stats[GET_RACE(ch)][3]
      || ch->real_abils.con + add_con < race_stats[GET_RACE(ch)][4]
      || ch->real_abils.con + add_con > race_max_stats[GET_RACE(ch)][4]
      || ch->real_abils.cha + add_cha < race_stats[GET_RACE(ch)][5]
      || ch->real_abils.cha + add_cha > race_max_stats[GET_RACE(ch)][5]) {
    send_to_char(ch, "New stats cannot exceed racial min/max values.\r\n");
    return;
  }

  char buf[4096];
  sprintf(buf, "(GC) %s (%d %s-%s) adjusted stats from %d/%d/%d/%d/%d/%d to %d/%d/%d/%d/%d/%d.",
	  GET_NAME(ch), GET_LEVEL(ch), class_abbrevs[(int)GET_CLASS(ch)], race_abbrevs[GET_RACE(ch)],
	  GET_STR(ch), GET_INT(ch), GET_WIS(ch), GET_DEX(ch), GET_CON(ch), GET_CHA(ch),
	  GET_STR(ch)+add_str, GET_INT(ch)+add_int, GET_WIS(ch)+add_wis, GET_DEX(ch)+add_dex, GET_CON(ch)+add_con, GET_CHA(ch)+add_cha);
  mudlogf(CMP, LVL_IMMORT, TRUE, "%s", buf);

  ch->real_abils.str += add_str;
  ch->real_abils.intel += add_int;
  ch->real_abils.wis += add_wis;
  ch->real_abils.dex += add_dex;
  ch->real_abils.con += add_con;
  ch->real_abils.cha += add_cha;

  ch->aff_abils.str += add_str;
  ch->aff_abils.intel += add_int;
  ch->aff_abils.wis += add_wis;
  ch->aff_abils.dex += add_dex;
  ch->aff_abils.con += add_con;
  ch->aff_abils.cha += add_cha;

  Crash_crashsave(ch);

  send_to_char(ch, "Okay.\r\n");
}
