/* ************************************************************************
 *   File: act.informative.c                             Part of CircleMUD *
 *  Usage: Player-level commands of an informative nature                  *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */

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
#include "clan.h"
#include "house.h"
#include "constants.h"
#include "dg_scripts.h"

#define ZCMD zone_table[zone].cmd[cmd_no]

/* extern variables */
extern struct zone_data *zone_table;
extern zone_rnum top_of_zone_table;
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct char_data *character_list;
extern struct obj_data *object_list;
extern struct index_data *obj_index;
extern struct command_info cmd_info[];
extern struct player_index_element *player_table;	/* for player listing */
extern int top_of_p_table;	/* online --Erika */
extern int spell_sort_info[];
extern struct house_control_rec house_control[];
extern int num_of_houses;
extern struct help_index_element *help_table;

extern char *credits;
extern char *news;
extern char *info;
extern char *motd;
extern char *imotd;
extern char *wizlist;
extern char *immlist;
extern char *policies;
extern char *teams;
extern char *handbook;
extern char *marriages;
extern char *areas;
extern char *pskills;
extern char *pspells;
extern char *race_abbrevs[];	/* 10/27/96, Echo */
extern char *class_abbrevs[];
extern char *scr_class_abbrevs[];
extern char *pc_race_types[];	/* 10/27/96, Echo */
extern char *pc_class_types[];	/* 10/27/96, Echo */
extern char *scr_male_pc_class_types[];
extern char *scr_female_pc_class_types[];
extern struct spell_info_type *spells;
extern float race_exp_multipliers[];
extern float class_exp_multipliers[];
extern int exp_table[LVL_IMPL + 1];	/* 10/08/96, Echo */
extern struct time_info_data time_info;
extern int top_of_helpt;
extern struct help_index_element *help_table;
extern char *help;
ACMD(do_action);

char *naturestr(int x);
char *bonstr(int x);
char *acstr(int x);
void Crash_count_items(struct obj_data *obj, long *nitems);
struct time_info_data *real_time_passed(time_t t2, time_t t1);
void str_add_spaces(char *source, int total_length);
int compute_armor_class(struct char_data *ch);
int parse_class(char *arg);
struct char_data *item_owner(struct obj_data *obj); 
struct player_shop* is_object_in_player_shop(struct obj_data*);

/* global */
int boot_high = 0;

/* internal functions */

long find_class_bitvector(char *arg);
void list_scanned_chars(struct char_data *list, struct char_data *ch,
			int distance, int door);

/* Echo, 10/05/96 - following show_obj_i_to_char() formerly
 * show_obj_to_char().  Renamed, and added new show_obj_to_char(),
 * show_obj_n_to_char(), and list_obj_to_char to provide item stacking.
 * list_obj_to_char() was broken as received.  fixed by Echo.
 * Patch courtesy of Hans Brinck, impl. at BlueMage-MUD via Aleksandr
 */

void show_obj_i_to_char(struct obj_data *object, struct char_data *ch,
			int mode, char *buf)
{
	int condition;
	buf[0] = '\0';

	if (((mode == 0) || (mode == 6))
	    && (object->description && *object->description))
		strcpy(buf, object->description);
	else if (((mode == 0) || (mode == 6)) && (GET_LEVEL(ch) >= LVL_DGOD))
		sprintf(buf, "hidden (%s)",
			object->name ? object->name : "none");
	else if (mode == 0 || mode == 6)
		return;
	else if (object->short_description && ((mode == 1) || (mode == 2) ||
					       (mode == 3) || (mode == 4)))
		strcpy(buf, object->short_description);
	else if (mode == 5) {
		if (GET_OBJ_TYPE(object) == ITEM_NOTE) {
			if (object->action_description) {
				strcpy(buf,
				       "There is something written upon it:\r\n\r\n");
				strcat(buf, object->action_description);
				/* this is repeated later
				   if(ch->desc)
				   page_string(ch->desc, buf, TRUE,"");
				 */
			} else
				send_to_char(ch, "It's blank.");
			return;
		} else if (GET_OBJ_TYPE(object) != ITEM_DRINKCON) {
			strcpy(buf, "You see nothing special..");
		} else		/* ITEM_TYPE == ITEM_DRINKCON||FOUNTAIN */
			strcpy(buf, "It looks like a drink container.");
	}
	if (mode != 3) {
		if (IS_OBJ_STAT(object, ITEM_INVISIBLE)) {
			strcat(buf, " (Invisible)");
		}
		if (IS_OBJ_STAT(object, ITEM_BLESS)
		    && AFF_FLAGGED(ch, AFF_DETECT_ALIGN)) {
			strcat(buf, "(Glowing blue!)");
		}
		if (IS_OBJ_STAT(object, ITEM_MAGIC)
		    && AFF_FLAGGED(ch, AFF_DETECT_MAGIC)) {
			strcat(buf, " (Glowing yellow!)");
		}
		if (IS_OBJ_STAT(object, ITEM_GLOW)) {
			strcat(buf, " (Glowing aura!)");
		}
		if (IS_OBJ_STAT(object, ITEM_HUM)) {
			strcat(buf, " (Humming sound!)");
		}
		if (IS_OBJ_STAT(object, ITEM_DARK)) {
			strcat(buf, " (Radiates darkness!)");
		}

		if (mode != 6) {
			if (GET_OBJ_TSLOTS(object) == 0)
				condition = 0;
			else
				condition = (GET_OBJ_CSLOTS(object) * 10) /
				             GET_OBJ_OSLOTS(object);

			if ((GET_OBJ_CSLOTS(object) == 0) &&
		            (GET_OBJ_TSLOTS(object) == 0))
				condition = 0;
			else if (GET_OBJ_CSLOTS(object) < 0)
				/* Added a broken status at position 1 - Nomi 5/10/25 */
				condition = 1;
			else
				/* Position 2 is the lowest before it breaks */
				condition += 2;

			if (GET_LEVEL(ch) >= LVL_IMMORT)
				sprintf(buf + strlen(buf),
					" ..%s (%d/%d/%d) (%5ld)",
					item_condition[condition],
					GET_OBJ_CSLOTS(object),
					GET_OBJ_TSLOTS(object),
					GET_OBJ_OSLOTS(object),
					GET_OBJ_VNUM(object));
			else {
				sprintf(buf + strlen(buf), " ..%s",
					item_condition[condition]);
			}
		}
	}
}

/* 10/05/96 Echo - received with item stacking patch. See note above.
 */
void show_obj_n_to_char(struct obj_data *object, struct char_data *ch,
			int mode, int num)
{
	char *buf = get_buffer(MAX_STRING_LENGTH);
	show_obj_i_to_char(object, ch, mode, buf);
	if (*buf == '\0') {
		release_buffer(buf);
		return;
	}
	if (num > 1)
		sprintf(buf + strlen(buf), " [%d]\r\n", num);
	else
		strcat(buf, "\r\n");
	if (ch->desc)
		page_string(ch->desc, buf, TRUE, "");
	release_buffer(buf);
}

/* 10/05/96 Echo - new show_obj_to_char() received with patch. See note above.
 */
void show_obj_to_char(struct obj_data *object, struct char_data *ch, int mode)
{
	char *buf = get_buffer(MAX_STRING_LENGTH);
	show_obj_i_to_char(object, ch, mode, buf);
	strcat(buf, "\r\n");
	if (ch->desc)
		page_string(ch->desc, buf, TRUE, "");
	release_buffer(buf);
}

/* 10/24/96 Echo - list_obj_to_char() renamed to list_obj_to_char_core()
 *   and content modified to allow a parameterized chance of failure of
 *   noticing each item; also, last message upon not seeing any items was
 *   removed, and the value of 'found' (whether any visible items were shown)
 *   is passed back as an int result.
 *     In conjunction with this, a new list_obj_to_char() was added which
 *   calls list_obj_to_char_core(), specifying min_certain_lvl of 0 (which
 *   ensures that if an item is visible to a char, it's listed) and sending
 *   " Nothing.\r\n" if no items are listed.
 *     peek_list_obj_to_char() also added for thieves' and immortals' peeking
 *   into others' inventories, which calls list_obj_to_char_core() with a
 *   min_certain_lvl of 100 and returns " You can't see anything.\r\n" if no
 *   visible items are listed.
 *
 *   To determine whether an obj is listed or not, number(0, min_certain_lvl)
 *   is generated and compared to the greater of character's level and 20
 *   (so that with a min_certain_level of 100, there is at least a 1/5 of
 *   a chance of spotting the visible item); if the number is below the
 *   greater of the two, the item is listed.
 */
/* 10/06/96 Echo - rewritten to work. Originally received as patch including
 *  several other functions and a mod to show_obj_to_char(). See note above.
 */
#define SUCCESSFUL_PEEK(ch)  ((min_certain_level > 0) ? \
                  number(0, min_certain_level) < \
                  MAX(20, GET_LEVEL(ch)) : 1)
int list_obj_to_char_core(struct obj_data *list, struct char_data *ch,
			  int mode, bool show, int min_certain_level)
{
	int *itemList;		/* holds vnums of objects already counted in list */
	int nItems = 0,		/* number of unique items already encountered */
	    nAlloced = 800,	/* largest number of unique items to keep track of   */
	    m,			/* m loops to see how many of an item are seen       */
	    n;			/* n indexes itemList and counts occurences of vnum  */
	struct obj_data *i,	/* index through list with i to check each item      */
	*tmp;			/* and with tmp to count other instances of new item */
	bool found = FALSE;	/* whether any visible items were found and shown    */
	bool already_checked;	/* whether current item in list has been checked */

	/*    itemList = (int *) malloc(sizeof(int)*nAlloced); */
	CREATE(itemList, int, nAlloced);
	/* loop through each item handed us in 'list' */
	for (i = list; i; i = i->next_content) {
		if (CAN_SEE_OBJ(ch, i) &&
		    ((GET_LEVEL(ch) >= LVL_IMMORT) ||
		     (i->carried_by && !IS_NPC(i->carried_by)) ||
		     !IS_SET(GET_OBJ_EXTRA2(i), ITEM2_BODYPART))) {
			if (i->item_number >= 0) {
				/* assume that we haven't encountered this item before   */
				already_checked = FALSE;

				/* check to see if we've encountered this item before... */
				/* run through the itemList of those already encountered */
				if (nItems > 0)
					for (n = 0;
					     n < nItems && !already_checked;
					     n++)
						if (itemList[n] ==
						    i->item_number)
							already_checked = TRUE;

				/* if the item hasn't been encountered... */
				if (!already_checked || (nItems == 0)) {
					/* add its vnum to the itemList...                       */
					itemList[nItems] = i->item_number;

					/* count visible occurrences of i in list from i on...   */
					for (n = 0, tmp = i; tmp;
					     tmp = tmp->next_content)
						if (CAN_SEE_OBJ(ch, tmp)
						    && tmp->item_number ==
						    itemList[nItems])
							n++;

					/* increment the number of unique objs found */
					nItems++;

					/* loop to determine how many of the n are actually seen */
					if (n > 0)
						for (m = 0; m < n; m++)
							if (!SUCCESSFUL_PEEK
							    (ch))
								n--;

					/* if any instances are successfully seen, show the item */
					if (n > 0) {
						show_obj_n_to_char(i, ch, mode,
								   n);
						found = TRUE;
					}
				}
			} else if (SUCCESSFUL_PEEK(ch)) {
				show_obj_to_char(i, ch, mode);
				found = TRUE;
			}
		}
	}
	if (itemList)
		free(itemList);
	return (found);
}

/* 10/24/96, Echo - added to call renamed list_obj_to_char_core() */
void list_obj_to_char(struct obj_data *list, struct char_data *ch,
		      int mode, bool show)
{
	int found;		/* are any visible items found to be listed? */

	found = list_obj_to_char_core(list, ch, mode, show, 0);

	if (!found && show)
		send_to_char(ch, " Nothing.\r\n");
}

/* 10/24/96, Echo - added to call renamed list_obj_to_char_core() */
void peek_list_obj_to_char(struct obj_data *list, struct char_data *ch)
{
	int found = FALSE;	/* are any visible items found to be listed? */

	if (GET_LEVEL(ch) >= LVL_IMMORT)	/* show visible items with certainty */
		found = list_obj_to_char_core(list, ch, 1, TRUE, 0);
	else if (GET_LEVEL(ch) > 0)	/* try peeking visible items */
		found = list_obj_to_char_core(list, ch, 1, TRUE, 100);

	if (!found)
		send_to_char(ch, " You can't see anything.\r\n");
}

void diag_char_to_char(struct char_data *i, struct char_data *ch)
{
	int percent;
	char *buf = get_buffer(256);
	strcpy(buf, PERS(i, ch));
	if (GET_MAX_HIT(i) > 0)
		percent = (100 * GET_HIT(i)) / GET_MAX_HIT(i);
	else
		percent = -1;	/* How could MAX_HIT be < 1?? */

	if (percent >= 100)
		send_to_char(ch, "%s is in excellent condition.\r\n", CAP(buf));
	else if (percent >= 90)
		send_to_char(ch, "%s has a few scratches.\r\n", CAP(buf));
	else if (percent >= 75)
		send_to_char(ch, "%s has some small wounds and bruises.\r\n",
			     CAP(buf));
	else if (percent >= 50)
		send_to_char(ch, "%s has quite a few wounds.\r\n", CAP(buf));
	else if (percent >= 30)
		send_to_char(ch,
			     "%s has some big nasty wounds and scratches.\r\n",
			     CAP(buf));
	else if (percent >= 15)
		send_to_char(ch, "%s looks pretty hurt.\r\n", CAP(buf));
	else if (percent >= 0)
		send_to_char(ch, "%s is in awful condition.\r\n", CAP(buf));
	else
		send_to_char(ch, "%s is bleeding awfully from big wounds.\r\n",
			     CAP(buf));
	release_buffer(buf);
}

void look_at_char(struct char_data *i, struct char_data *ch)
{
	int j, found;
	char *buf2 = get_buffer(MAX_STRING_LENGTH);
	/* 10/24/96, Echo - tmp_obj made unnecessary by 'peek' code substitution
	 * struct obj_data *tmp_obj;
	 */

	if (!ch->desc)
		return;

	if (i->player.description)
		send_to_char(ch, "%s", i->player.description);
	else
		act("You see nothing special about $m.", FALSE, i, 0, ch,
		    TO_VICT);

	diag_char_to_char(i, ch);

	if (RIDING(i) && IN_ROOM(RIDING(i)) == IN_ROOM(i)) {
		if (RIDING(i) == ch)
			act("$e is mounted on you.", FALSE, i, 0, ch, TO_VICT);
		else {
			sprintf(buf2, "$e is mounted upon %s.",
				PERS(RIDING(i), ch));
			act(buf2, FALSE, i, 0, ch, TO_VICT);
		}
	} else if (RIDDEN_BY(i) && IN_ROOM(RIDDEN_BY(i)) == IN_ROOM(i)) {
		if (RIDDEN_BY(i) == ch)
			act("You are mounted upon $m.", FALSE, i, 0, ch,
			    TO_VICT);
		else {
			sprintf(buf2, "$e is mounted by %s.",
				PERS(RIDDEN_BY(i), ch));
			act(buf2, FALSE, i, 0, ch, TO_VICT);
		}
	}

	found = FALSE;
	for (j = 0; !found && j < NUM_WEARS; j++)
		if (GET_EQ(i, j) && CAN_SEE_OBJ(ch, GET_EQ(i, j)))
			found = TRUE;

	if (found) {
		act("\r\n$n is using:", FALSE, i, 0, ch, TO_VICT);
		for (j = 0; j < NUM_WEARS; j++)
			if (GET_EQ(i, j) && CAN_SEE_OBJ(ch, GET_EQ(i, j))) {
				send_to_char(ch, "%s", where[j]);
				show_obj_to_char(GET_EQ(i, j), ch, 1);
			}
	}
	if (ch != i
	    && (IS_THIEF(ch) || IS_ASSASSIN(ch)
		|| GET_LEVEL(ch) >= LVL_IMMORT)) {
		found = FALSE;
		act("\r\nYou attempt to peek at $s inventory:", FALSE, i, 0, ch,
		    TO_VICT);
		peek_list_obj_to_char(i->carrying, ch);
	}
	release_buffer(buf2);
}

void list_one_char(struct char_data *i, struct char_data *ch)
{
	const char *positions[] = {
		" is lying here, dead.",
		" is lying here, mortally wounded.",
		" is lying here, incapacitated.",
		" is lying here, stunned.",
		" is sleeping here.",
		" is chanting here.",
		" is meditating here.",
		" is bandaged here.",
		" is resting here.",
		" is sitting here.",
		"",
		" is standing here."
	};
	char *buf = get_buffer(MAX_STRING_LENGTH);

	if (IS_NPC(i) && !strcmp(i->player.long_descr, "INVIS\r\n")) {
		if (GET_LEVEL(ch) >= LVL_IMMORT)
			send_to_char(ch, "%shidden mob (%s)%s\r\n", NGRN,
				     i->player.short_descr, NNRM);
		release_buffer(buf);
		return;
	}

	if (IS_NPC(i) && i->player.long_descr
	    && GET_POS(i) == GET_DEFAULT_POS(i)) {
		*buf = '\0';

		if (AFF_FLAGGED(i, AFF_INVISIBLE))
			strcat(buf, "*");

		if (AFF_FLAGGED(i, AFF_HIDE))
			strcat(buf, "&&");

		if (AFF_FLAGGED(ch, AFF_DETECT_ALIGN)) {
			if (IS_EVIL(i))
				strcat(buf, "&R(Red Aura)&Y ");
			else if (IS_GOOD(i))
				strcat(buf, "&B(Blue Aura)&Y ");
		}
		if (FURNITURE(i) && IN_ROOM(FURNITURE(i)) == IN_ROOM(i)) {
			strcpy(buf, i->player.short_descr);
			CAP(buf);
			sprintf(buf + strlen(buf), " is here, %s on %s.\r\n",
				position_types[(int)GET_POS(i)],
				FURNITURE(i)->short_description);
		} else
			strcat(buf, i->player.long_descr);
		send_to_char(ch, "%s", buf);

		if (AFF_FLAGGED(ch, AFF_DETECT_MAGIC) ||
		    (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_HOLYLIGHT))) {
			if (AFF_FLAGGED(i, AFF_SANCTUARY))
				act("...$e glows with a bright light!", FALSE,
				    i, 0, ch, TO_VICT);
		}
		if (AFF_FLAGGED(i, AFF_HASTE))
			act("...$e is moving very rapidly!", FALSE, i, 0, ch,
			    TO_VICT);
		if (AFF_FLAGGED(i, AFF_BLIND))
			act("...$e is groping around blindly!", FALSE, i, 0, ch,
			    TO_VICT);
		if (AFF_FLAGGED(i, AFF_PLAGUE))
			act("&G...$n is covered with puss and putrid sores!&Y",
			    FALSE, i, 0, ch, TO_VICT);

		release_buffer(buf);
		return;
	}

	if (IS_NPC(i)) {
		strcpy(buf, i->player.short_descr);
		CAP(buf);
	} else
		sprintf(buf, "%s%s%s%s%s", i->player.name,
			*(GET_TITLE(i)) == '\0' ? "" : " ", GET_TITLE(i),
			(GET_CLAN(i) > 0) ? " ", "",
			(GET_CLAN(i) > 0) ? GET_CLAN_NAME(i) : "");

	if (PLR_FLAGGED(i, PLR_FISHING))
		strcat(buf, " (fishing)");
	if (AFF_FLAGGED(i, AFF_INVISIBLE))
		strcat(buf, " (invisible)");
	if (AFF_FLAGGED(i, AFF_HIDE))
		strcat(buf, " (hidden)");
	if (!IS_NPC(i) && !i->desc)
		strcat(buf, " (linkless)");
	if (!IS_NPC(i) && PLR_FLAGGED(i, PLR_WRITING))
		strcat(buf, " (writing)");

	if (RIDING(i) && IN_ROOM(RIDING(i)) == IN_ROOM(i)) {
		strcat(buf, " is here, mounted upon ");
		if (RIDING(i) == ch)
			strcat(buf, "you");
		else
			strcat(buf, PERS(RIDING(i), ch));
		strcat(buf, ".");
	} else if (FURNITURE(i) && IN_ROOM(FURNITURE(i)) == IN_ROOM(i)) {
		char *tmpbuf = get_buffer(128);
		strcpy(tmpbuf, position_types[(int)GET_POS(i)]);
		tmpbuf = LOW(tmpbuf);
		sprintf(buf + strlen(buf), " is here %s on %s.",
			tmpbuf, FURNITURE(i)->short_description);
		release_buffer(tmpbuf);
	} else if (GET_POS(i) != POS_FIGHTING)
		strcat(buf, positions[(int)GET_POS(i)]);
	else {
		if (FIGHTING(i)) {
			strcat(buf, " is here, fighting ");
			if (FIGHTING(i) == ch)
				strcat(buf, "YOU!");
			else {
				if (IN_ROOM(i) == IN_ROOM(FIGHTING(i)))
					strcat(buf, PERS(FIGHTING(i), ch));
				else
					strcat(buf,
					       "someone who has already left");
				strcat(buf, "!");
			}
		} else		/* NIL fighting pointer */
			strcat(buf, " is here struggling with thin air.");
	}

	if (AFF_FLAGGED(ch, AFF_DETECT_ALIGN)) {
		if (IS_EVIL(i))
			strcat(buf, " &R(Red Aura)&Y");
		else if (IS_GOOD(i))
			strcat(buf, " &B(Blue Aura)&Y");
	}
	send_to_char(ch, "%s\r\n", buf);

	if (AFF_FLAGGED(ch, AFF_DETECT_MAGIC) ||
	    (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_HOLYLIGHT))) {
		if (AFF_FLAGGED(i, AFF_SANCTUARY))
			act("...$e glows with a bright light!", FALSE, i, 0, ch,
			    TO_VICT);
	}
	if (AFF_FLAGGED(i, AFF_HASTE))
		act("...$e is moving very rapidly!", FALSE, i, 0, ch, TO_VICT);
	if (AFF_FLAGGED(i, AFF_BLIND))
		act("...$e is groping around blindly!", FALSE, i, 0, ch,
		    TO_VICT);
	if (AFF_FLAGGED(i, AFF_PLAGUE))
		act("&G...$n is covered with puss and putrid sores!&Y", FALSE,
		    i, 0, ch, TO_VICT);

	release_buffer(buf);
}

void list_char_to_char(struct char_data *list, struct char_data *ch)
{
	struct char_data *i;

	for (i = list; i; i = i->next_in_room)
		if (ch != i) {
			if (RIDDEN_BY(i) && IN_ROOM(RIDDEN_BY(i)) == IN_ROOM(i))
				continue;

			if (CAN_SEE(ch, i)) {
				send_to_char(ch, CCYEL(ch, C_NRM));
				list_one_char(i, ch);
				send_to_char(ch, CCNRM(ch, C_NRM));
			} else if (IS_DARK(IN_ROOM(ch)) && !CAN_SEE_IN_DARK(ch)
				   && HAS_INFRA(i) && (IS_NPC(i)
						       || (GET_INVIS_LEV(i) <=
							   GET_LEVEL(ch))))
				send_to_char(ch,
					     "You see a pair of glowing &Rred&Y eyes looking "
					     "your way.\r\n");
		}
}

void do_auto_exits(struct char_data *ch)
{
	int door;
	int slen = 0;
	char *buf2 = get_buffer(128);

	if (!IN_ROOM(ch)) {
		release_buffer(buf2);
		return;
	}
	for (door = 0; door < NUM_OF_DIRS; door++) {
		if (EXIT(ch, door) != NULL
		    && EXIT(ch, door)->to_room != NOWHERE) {
			if (EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED)) {
				if (EXIT_FLAGGED(EXIT(ch, door), EX_SECRET) &&
				    (GET_LEVEL(ch) >= LVL_IMMORT)) {
					if (EXIT_FLAGGED
					    (EXIT(ch, door), EX_HIDDEN))
						slen +=
						    sprintf(buf2 + slen,
							    " {<%s>}",
							    dirs[door]);
					else
						slen +=
						    sprintf(buf2 + slen,
							    " {%s}",
							    dirs[door]);
				} else
				    if (EXIT_FLAGGED(EXIT(ch, door), EX_HIDDEN)
					&& (GET_LEVEL(ch) >= LVL_IMMORT))
					slen +=
					    sprintf(buf2 + slen, " [<%s>]",
						    dirs[door]);
				else if (!EXIT_FLAGGED
					 (EXIT(ch, door), EX_HIDDEN)
					 && !EXIT_FLAGGED(EXIT(ch, door),
							  EX_SECRET))
					slen +=
					    sprintf(buf2 + slen, " [%s]",
						    dirs[door]);
			} else if (EXIT_FLAGGED(EXIT(ch, door), EX_HIDDEN)
				   && (GET_LEVEL(ch) >= LVL_IMMORT))
				slen +=
				    sprintf(buf2 + slen, " <%s>", dirs[door]);
			else if (!EXIT_FLAGGED(EXIT(ch, door), EX_HIDDEN)
				 && !EXIT_FLAGGED(EXIT(ch, door), EX_NOPASS))
				slen += sprintf(buf2 + slen, " %s", dirs[door]);
			else if (EXIT_FLAGGED(EXIT(ch, door), EX_NOPASS)
				 && GET_LEVEL(ch) >= LVL_IMMORT)
				slen +=
				    sprintf(buf2 + slen, " *%s*", dirs[door]);
		}
	}
	send_to_char(ch, "%s[ Exits:%s]%s\r\n", CCCYN(ch, C_NRM),
		     *buf2 ? buf2 : " None! ", CCNRM(ch, C_NRM));

	release_buffer(buf2);
}

/* The following is to implement whois from the snippets page --Erika */

ACMD(do_whois)
{
	struct char_data *victim = 0;

	skip_spaces(&argument);
	if (IS_NPC(ch))
		return;

	if (!*argument) {
		send_to_char(ch, "Do a WhoIS on which player?\r\n");
	} else if (!(victim = get_char(argument))) {
		send_to_char(ch, "That player is not here.\r\n");
		return;
	} else if (!CAN_SEE(ch, victim)) {
		send_to_char(ch, "That player is not here.\r\n");
		return;
	} else if (IS_NPC(victim)) {
		send_to_char(ch, "You can't determine who this mob is!\r\n");
		return;
	} else if ((GET_LEVEL(ch) < LVL_IMMORT)
		   && (GET_LEVEL(victim) >= LVL_IMMORT)) {
		send_to_char(ch, "%s %s is an Immortal.\r\n", GET_NAME(victim),
			     GET_TITLE(victim));
	} else {
		char *class_name;
		if (IS_SCR(victim)) {
			if (GET_SEX(victim) == SEX_FEMALE) {
				class_name =
				    scr_female_pc_class_types[(int)GET_CLASS(victim)];
			} else {
				class_name =
				    scr_male_pc_class_types[(int)GET_CLASS(victim)];
			}
		} else {
			class_name = pc_class_types[(int)GET_CLASS(victim)];
		}
		send_to_char(ch, "%s %s is:\r\n", GET_NAME(victim),
			     GET_TITLE(victim));
		send_to_char(ch, "Level: %d  Age: %d years old\r\n",
			     GET_LEVEL(victim), GET_AGE(victim));
		send_to_char(ch, "Class: %s  Race: %s  Sex: %s\r\n", class_name,
			     pc_race_types[GET_RACE(victim)],
			     genders[(int)GET_SEX(victim)]);
		send_to_char(ch,
			     "P-kills: %d  Deaths: %d  Mob Kills: %ld  Quest points: %d  Explored: %.3f%%\r\n",
			     GET_PKILLS(victim), GET_DEATHS(victim),
			     (GET_OLD_MOBKILLS(victim) >
			      0) ? (long)GET_OLD_MOBKILLS(victim) :
			     GET_MOBKILLS(victim), GET_QPOINTS(victim),
			     100 * GET_EXPLORED(victim) / (float)(top_of_world >
								  0 ?
								  top_of_world :
								  1)
		    );
	}

	/*      CREATE(victim, struct char_data, 1);
	   clear_char(victim);
	   if (load_char(argument, &tmp_store) > -1)
	   {
	   char *buf=get_buffer(MAX_STRING_LENGTH);
	   store_to_char(&tmp_store, victim);
	   sprintf(buf, "%sLevel %d %s\r\n", buf, GET_LEVEL(victim),
	   class_abbrevs[(int) GET_CLASS(victim)]);
	   send_to_char(ch,"%s",buf);
	   release_buffer(buf);
	   }
	   else
	   {
	   send_to_char(ch,"There is no such player.\r\n");
	   }
	   free(victim);
	 */
}

/* The following is to makeit list all the players in player file --Erika */

ACMD(do_players)
{
	int i = 0;
	int lownum = 0;
	char *buf = get_buffer(32750);
	char *arglist = get_buffer(1024);
	char *arg = get_buffer(256);
	char *buf1 = get_buffer(256);
	char *ztHost = get_buffer(256);
	char *cFlag;
	int Deleted = FALSE;
	int NoDelete = FALSE;
	int Zero = FALSE;
	int Host = FALSE;
	int LastOn = FALSE;
	int MinGold = -1;
	/* int count = 0; */

	skip_spaces(&argument);
	strcpy(arglist, argument);

	while (arglist && *arglist) {
		half_chop(arglist, arg, buf1);
		if (isdigit((int)*arg)) {
			sscanf(arg, "%d", &lownum);
			strcpy(arglist, buf1);
		} else if (*arg == '-') {
			cFlag = arg + 1;
			switch (*cFlag) {
			case 'd':
				Deleted = TRUE;
				break;

			case 'g':
				strcpy(arglist, buf1);
				half_chop(arglist, arg, buf1);
				if (!isdigit((int)*arg)) {
					send_to_char(ch,
						     " The -g option requires a number.\r\n");
					release_buffer(arglist);
					release_buffer(arg);
					release_buffer(buf1);
					release_buffer(buf);
					release_buffer(ztHost);
					return;
				}
				sscanf(arg, "%d", &MinGold);
				break;

			case 'h':
				send_to_char(ch,
					     "USAGE: Players [-h] [<number>] [-zerolevel] "
					     "[-delete] [-nodelete] [-gold <number>] "
					     "[-site <hostname>] [-laston]\r\n");
				release_buffer(arglist);
				release_buffer(arg);
				release_buffer(buf1);
				release_buffer(ztHost);
				release_buffer(buf);
				return;
				break;
			case 'l':
				LastOn = TRUE;
				break;
			case 'n':
				NoDelete = TRUE;
				break;
			case 's':
				Host = TRUE;
				strcpy(arglist, buf1);
				half_chop(arglist, ztHost, buf1);
				break;
			case 'z':
				Zero = TRUE;
				break;
			default:
				break;
			}

			strcpy(arglist, buf1);
		} else
			strcpy(arglist, buf1);

	}
	release_buffer(arglist);
	release_buffer(arg);
	release_buffer(buf1);
	sprintf(buf,
		"  IdNum Name                 Lvl Host                          Bank ND\r\n");
	sprintf(buf + strlen(buf),
		"----------------------------------------------------------------------\r\n");

	for (i = 0; i <= top_of_p_table; i++) {
		if (!Zero && (player_table + i)->level <= 0)
			continue;
		if (!Deleted
		    && IS_SET((player_table + i)->plr_flags, PLR_DELETED))
			continue;
		if ((player_table + i)->id < lownum)
			continue;
		if (((player_table + i)->gold[0] +
		     (player_table + i)->bank_gold[0]) < MinGold)
			continue;
		if ((Host == TRUE)
		    && (((player_table + i)->hostname == NULL)
			|| !strstr((player_table + i)->hostname, ztHost)))
			continue;
		if (NoDelete
		    && !IS_SET((player_table + i)->plr_flags, PLR_NODELETE))
			continue;

		sprintf(buf + strlen(buf),
			" %6ld %-15.15s %3d %-30.30s %10ld %c%s",
			(player_table + i)->id, (player_table + i)->name,
			(player_table + i)->level, (player_table + i)->hostname,
			(player_table + i)->gold[0] + (player_table +
						       i)->bank_gold[0],
			IS_SET((player_table + i)->plr_flags,
			       PLR_NODELETE) ? 'Y' : 'N',
			LastOn ? ctime(&((player_table + i)->last_logon)) :
			"\r\n");
		/*
		   count++;
		   if (count == 3)
		   {
		   count = 0;
		   strcat(buf, "\r\n");
		   }
		 */
		if (strlen(buf) > 32500) {
			strcat(buf,
			       "The player listing has overrun the buffer, either we are\r\n"
			       "a great success, or you need to purge the player file\r\n"
			       "NOW!!!\r\n");
			sprintf(buf + strlen(buf),
				"  %d out of %d players printed\r\n", i,
				top_of_p_table);

			break;
		}

	}

	release_buffer(ztHost);
	if (ch->desc)
		page_string(ch->desc, buf, TRUE, "");
	release_buffer(buf);
}

ACMD(do_exits)
{
	int door;
	char *buf;
	char *buf2;

	if (AFF_FLAGGED(ch, AFF_BLIND)) {
		send_to_char(ch,
			     "You can't see a damned thing, you're blind!\r\n");
		return;
	}
	buf = get_buffer(MAX_STRING_LENGTH);
	buf2 = get_buffer(MAX_STRING_LENGTH);

	for (door = 0; door < NUM_OF_DIRS; door++)
		if (EXIT(ch, door) && EXIT(ch, door)->to_room != NOWHERE &&
		    !EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED)) {
			if (GET_LEVEL(ch) >= LVL_IMMORT)
				sprintf(buf2, "%-5s - [%5ld] %s\r\n",
					dirs[door],
					GET_ROOM_VNUM(EXIT(ch, door)->to_room),
					world[EXIT(ch, door)->to_room].name);
			else {
				sprintf(buf2, "%-5s - ", dirs[door]);
				if (IS_DARK(EXIT(ch, door)->to_room)
				    && !CAN_SEE_IN_DARK(ch))
					strcat(buf2, "Too dark to tell\r\n");
				else {
					strcat(buf2,
					       world[EXIT(ch, door)->to_room].
					       name);
					strcat(buf2, "\r\n");
				}
			}
			strcat(buf, CAP(buf2));
		}
	send_to_char(ch, "Obvious exits:\r\n");

	if (*buf)
		send_to_char(ch, "%s", buf);
	else
		send_to_char(ch, " None.\r\n");

	release_buffer(buf2);
	release_buffer(buf);
}

void look_at_room(struct char_data *ch, int ignore_brief)
{
	if (!ch->desc)
		return;
	if ((IN_ROOM(ch) < 0) || (IN_ROOM(ch) > top_of_world))
		return;

	if (IS_DARK(IN_ROOM(ch)) && !CAN_SEE_IN_DARK(ch)) {
		send_to_char(ch, "It is pitch black...\r\n");
		return;
	} else if (AFF_FLAGGED(ch, AFF_BLIND)) {
		send_to_char(ch,
			     "You see nothing but infinite darkness...\r\n");
		return;
	}
	send_to_char(ch, CCCYN(ch, C_NRM));

	if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_ROOMFLAGS)) {
		char *buf2 = get_buffer(MAX_STRING_LENGTH);
		char *buf3 = get_buffer(MAX_STRING_LENGTH);
		sprintbit(ROOM_FLAGS(IN_ROOM(ch)), room_bits, buf2);
		sprintbit(ROOM2_FLAGS(IN_ROOM(ch)), room2_bits, buf3);
		send_to_char(ch, "[%5ld] %s%s [%s%s]\r\nSect Type: %s",
			     GET_ROOM_VNUM(IN_ROOM(ch)),
			     world[IN_ROOM(ch)].name,
			     graffiti_exists(GET_ROOM_VNUM(IN_ROOM(ch))) ?
			     " [G]" : "", !strcmp(buf2, "NOBITS ")
			     && strcmp(buf3, "NOBITS ") ? "" : buf2,
			     !strcmp(buf3, "NOBITS ") ? "" : buf3,
			     sector_types[SECT(IN_ROOM(ch))]);
		release_buffer(buf3);
		release_buffer(buf2);
	} else {
		send_to_char(ch, "%s%s",
			     world[IN_ROOM(ch)].name,
			     graffiti_exists(GET_ROOM_VNUM(IN_ROOM(ch))) ?
			     " [G]" : "");
	}

	send_to_char(ch, "%s\r\n", CCNRM(ch, C_NRM));

	if ((!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_BRIEF)) || ignore_brief ||
	    ROOM_FLAGGED(IN_ROOM(ch), ROOM_DEATH))
		send_to_char(ch, "%s", world[IN_ROOM(ch)].description);

	/* autoexits */
	if (IS_NPC(ch) || PRF_FLAGGED(ch, PRF_AUTOEXIT))
		do_auto_exits(ch);

	/* now list characters & objects */
	send_to_char(ch, CCGRN(ch, C_NRM));
	list_obj_to_char(world[IN_ROOM(ch)].contents, ch, 6, FALSE);
	list_char_to_char(world[IN_ROOM(ch)].people, ch);
	send_to_char(ch, CCNRM(ch, C_NRM));
}

void look_in_direction(struct char_data *ch, int dir)
{
	int room, nextroom, orig_room = IN_ROOM(ch);
	int distance;

	if (FIGHTING(ch)) {
		send_to_char(ch, "You are a bit busy fighting right now!\r\n");
		return;
	}
	if (EXIT(ch, dir)) {
		if (EXIT(ch, dir)->general_description)
			send_to_char(ch, "%s",
				     EXIT(ch, dir)->general_description);
		else
			send_to_char(ch, "You see nothing special.\r\n");

		if (EXIT_FLAGGED(EXIT(ch, dir), EX_CLOSED) &&
		    EXIT(ch, dir)->keyword) {
			if (!EXIT_FLAGGED(EXIT(ch, dir), EX_SECRET)) {
				send_to_char(ch, "The %s is closed.\r\n",
					     fname(EXIT(ch, dir)->keyword));
			}
		} else if (EXIT_FLAGGED(EXIT(ch, dir), EX_ISDOOR) &&
			   EXIT(ch, dir)->keyword) {
			send_to_char(ch, "The %s is open.\r\n",
				     fname(EXIT(ch, dir)->keyword));
		}

		if (CAN_GO2(orig_room, dir))
			nextroom = EXIT2(orig_room, dir)->to_room;
		else
			nextroom = NOWHERE;

		for (distance = 0; ((nextroom != NOWHERE) && (distance < 3));
		     distance++) {
			if (world[nextroom].people)
				list_scanned_chars(world[nextroom].people, ch,
						   distance, dir);

			room = nextroom;
			if (CAN_GO2(room, dir))
				nextroom = EXIT2(room, dir)->to_room;
			else
				nextroom = NOWHERE;

		}

	} else
		send_to_char(ch, "You see nothing special.\r\n");
}

void look_in_obj(struct char_data *ch, char *arg)
{
	struct obj_data *obj = NULL;
	struct char_data *dummy = NULL;
	int amt, bits;

	if (!*arg)
		send_to_char(ch, "Look in what?\r\n");
	else if (!(bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM |
				       FIND_OBJ_EQUIP, ch, &dummy, &obj))) {
		send_to_char(ch, "There doesn't seem to be %s %s here.\r\n",
			     AN(arg), arg);
	} else if ((GET_OBJ_TYPE(obj) != ITEM_DRINKCON)
		   && (GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN)
		   && (GET_OBJ_TYPE(obj) != ITEM_CONTAINER))
		send_to_char(ch, "There's nothing inside that!\r\n");
	else {
		if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER) {
			if (OBJVAL_FLAGGED(obj, CONT_CLOSED))
				send_to_char(ch, "It is closed.\r\n");
			else {
				send_to_char(ch, "%s", fname(obj->name));
				switch (bits) {
				case FIND_OBJ_INV:
					send_to_char(ch, " (carried): \r\n");
					break;
				case FIND_OBJ_ROOM:
					send_to_char(ch, " (here): \r\n");
					break;
				case FIND_OBJ_EQUIP:
					send_to_char(ch, " (used): \r\n");
					break;
				}

				list_obj_to_char(obj->contains, ch, 2, TRUE);
			}
		} else {	/* item must be a fountain or drink container */
			if (GET_OBJ_VAL(obj, 1) <= 0)
				send_to_char(ch, "It is empty.\r\n");
			else {
				if (GET_OBJ_VAL(obj, 0) <= 0
				    || GET_OBJ_VAL(obj, 1) > GET_OBJ_VAL(obj,
									 0)) {
					/* BUG */
					send_to_char(ch,
						     "Its contents seem somewhat murky.\r\n");
					mudlogf(CMP, LVL_IMMORT, TRUE,
						"SYSERR: %s[%ld] has bad "
						"settings: val0 = %ld (>0) val1 = %ld (<=val1)",
						GET_OBJ_NAME(obj),
						GET_OBJ_VNUM(obj),
						GET_OBJ_VAL(obj, 0),
						GET_OBJ_VAL(obj, 1));
				} else {
					char *buf2 =
					    get_buffer(MAX_STRING_LENGTH);
					amt =
					    (GET_OBJ_VAL(obj, 1) * 3) /
					    GET_OBJ_VAL(obj, 0);
					sprinttype(GET_OBJ_VAL(obj, 2),
						   color_liquid, buf2);
					send_to_char(ch,
						     "It's %sfull of a %s liquid.\r\n",
						     fullness[amt], buf2);
					release_buffer(buf2);
				}
			}
		}
	}
}

char *find_exdesc(char *word, struct extra_descr_data *list)
{
	struct extra_descr_data *i;

	for (i = list; i; i = i->next)
		if (isname(word, i->keyword))
			return (i->description);

	return (NULL);
}

/*
 * Given the argument "look at <target>", figure out what object or char
 * matches the target.  First, see if there is another char in the room
 * with the name.  Then check local objs for exdescs.
 */
void look_at_target(struct char_data *ch, char *arg)
{
	int bits, found = FALSE, j, i = 0, fnum;
	struct char_data *found_char = NULL;
	struct obj_data *obj, *found_obj = NULL;
	char *desc, *tar_obj, *tmp_char;

	if (!ch->desc)
		return;

	if (!*arg) {
		send_to_char(ch, "Look at what?\r\n");
		return;
	}

	skip_spaces(&arg);
	bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP |
			    FIND_CHAR_ROOM, ch, &found_char, &found_obj);

	/* Is the target a character? */
	if (found_char != NULL) {
		tar_obj = get_buffer(256);
		tmp_char = get_buffer(256);
		half_chop(arg, tmp_char, tar_obj);

		if (*tar_obj) {
			for (j = 0; (j < NUM_WEARS) && !found; j++) {
				if (GET_EQ(found_char, j)
				    && CAN_SEE_OBJ(ch, GET_EQ(found_char, j))) {
					if ((desc =
					     find_exdesc(tar_obj,
							 GET_EQ(found_char,
								j)->
							 ex_description)) !=
					    NULL) {
						send_to_char(ch, "%s", desc);
						if (ch != found_char) {
							if (CAN_SEE
							    (found_char, ch)) {
								if (GET_LEVEL
								    (ch) >=
								    GET_LEVEL
								    (found_char))
								{
									act("$n looks at you, examining $p.", TRUE, ch, GET_EQ(found_char, j), found_char, TO_VICT);
									act("$n looks at $p on $N.", TRUE, ch, GET_EQ(found_char, j), found_char, TO_NOTVICT);
								} else {
									act("$n looks at you, admiring $p.", TRUE, ch, GET_EQ(found_char, j), found_char, TO_VICT);
									act("$n looks at $N, admiring $p.", TRUE, ch, GET_EQ(found_char, j), found_char, TO_NOTVICT);
								}
							}
						}
						found = TRUE;
					}
				}
			}
			if (found == TRUE) {
				release_buffer(tar_obj);
				release_buffer(tmp_char);
				return;
			}
		}
		look_at_char(found_char, ch);
		if (ch != found_char) {
			if (CAN_SEE(found_char, ch))
				act("$n looks at you.", TRUE, ch, 0, found_char,
				    TO_VICT);
			act("$n looks at $N.", TRUE, ch, 0, found_char,
			    TO_NOTVICT);
		}
		release_buffer(tar_obj);
		release_buffer(tmp_char);
		return;
	}

	if (!(fnum = get_number(&arg))) {
		send_to_char(ch, "You do not see that here.\r\n");
		return;
	}

	/* Does the argument match an extra desc in the room? */
	if ((desc = find_exdesc(arg, world[IN_ROOM(ch)].ex_description)) != NULL
	    && ++i == fnum) {
		if (ch->desc)
			page_string(ch->desc, desc, FALSE, "");
		return;
	}

	/* Does the argument match an extra desc in the char's equipment? */
	for (j = 0; (j < NUM_WEARS) && !found; j++) {
		if (GET_EQ(ch, j) && CAN_SEE_OBJ(ch, GET_EQ(ch, j))) {
			if ((desc =
			     find_exdesc(arg,
					 GET_EQ(ch, j)->ex_description)) != NULL
			    && ++i == fnum) {
				send_to_char(ch, "%s", desc);
				found = TRUE;
			}
		}
	}
	/* Does the argument match an extra desc in the char's inventory? */
	for (obj = ch->carrying; obj && !found; obj = obj->next_content) {
		if (CAN_SEE_OBJ(ch, obj)) {
			if ((desc =
			     find_exdesc(arg, obj->ex_description)) != NULL
			    && ++i == fnum) {
				send_to_char(ch, "%s", desc);
				found = TRUE;
			}
		}
	}

	/* Does the argument match an extra desc of an object in the room? */
	for (obj = world[IN_ROOM(ch)].contents; obj && !found;
	     obj = obj->next_content) {
		if (CAN_SEE_OBJ(ch, obj)) {
			if ((desc =
			     find_exdesc(arg, obj->ex_description)) != NULL
			    && ++i == fnum) {
				send_to_char(ch, "%s", desc);
				found = TRUE;
			}
		}
	}

	if (bits) {		/* If an object was found back in generic_find */
		if (!found)
			show_obj_to_char(found_obj, ch, 5);	/* Show no-description */
		else
			show_obj_to_char(found_obj, ch, 7);	/* Find hum, glow etc */
		return;
	}

	if (!found) {
		send_to_char(ch, "You do not see that here.\r\n");
	}
}

ACMD(do_look)
{
	char *arg;
	char *arg2;

	int look_type;

	if (!ch->desc)
		return;

	if (GET_POS(ch) < POS_SLEEPING)
		send_to_char(ch, "You can't see anything but stars!\r\n");
	else if ((GET_POS(ch) == POS_SLEEPING) && !AFF_FLAGGED(ch, AFF_DREAM))
		send_to_char(ch,
			     "You can't see anything, you're sleeping!\r\n");
	else if (AFF_FLAGGED(ch, AFF_BLIND))
		send_to_char(ch,
			     "You can't see a damned thing, you're blind!\r\n");
	else if (IS_DARK(IN_ROOM(ch)) && !CAN_SEE_IN_DARK(ch)) {
		send_to_char(ch, "It is pitch black...\r\n%s",
			     CCYEL(ch, C_NRM));
		list_char_to_char(world[IN_ROOM(ch)].people, ch);	/* glowing red eyes */
		send_to_char(ch, CCNRM(ch, C_NRM));
	} else {
		arg = get_buffer(MAX_INPUT_LENGTH);
		arg2 = get_buffer(MAX_INPUT_LENGTH);
		half_chop(argument, arg, arg2);

		if (subcmd == SCMD_READ) {
			if (!*arg)
				send_to_char(ch, "Read what?\r\n");
			else
				look_at_target(ch, arg);
			release_buffer(arg2);
			release_buffer(arg);
			return;
		}
		if (!*arg)	/* "look" alone, without an argument at all */
			look_at_room(ch, 1);
		else if (is_abbrev(arg, "in"))
			look_in_obj(ch, arg2);
		/* did the char type 'look <direction>?' */
		else if ((look_type = search_block(arg, dirs, FALSE)) >= 0)
			look_in_direction(ch, look_type);
		else if (is_abbrev(arg, "at"))
			look_at_target(ch, arg2);
		else
			look_at_target(ch, argument);
		release_buffer(arg2);
		release_buffer(arg);

	}
}

ACMD(do_examine)
{
	int found;
	struct char_data *tmp_char;
	struct obj_data *tmp_object;
	char *arg = get_buffer(MAX_INPUT_LENGTH);
	char *buf;
	int condition;

	one_argument(argument, arg);

	if (!*arg) {
		send_to_char(ch, "Examine what?\r\n");
		release_buffer(arg);
		return;
	}

	look_at_target(ch, arg);

	generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_CHAR_ROOM |
		     FIND_OBJ_EQUIP, ch, &tmp_char, &tmp_object);

	if (!tmp_char && !tmp_object) {
		release_buffer(arg);
		return;
	}

	if (tmp_char) {
		release_buffer(arg);
		return;
	}
	if (tmp_object) {
		if ((GET_OBJ_TYPE(tmp_object) == ITEM_DRINKCON) ||
		    (GET_OBJ_TYPE(tmp_object) == ITEM_FOUNTAIN) ||
		    (GET_OBJ_TYPE(tmp_object) == ITEM_CONTAINER)) {
			send_to_char(ch, "When you look inside, you see:\r\n");
			look_in_obj(ch, arg);
		}

		if (tmp_object->people) {
			buf = get_buffer(MAX_STRING_LENGTH);
			sprintf(buf, "It is being used as furniture by:");
			for (found = 0, tmp_char = tmp_object->people; tmp_char;
			     tmp_char = tmp_char->next_in_furniture) {
				if (!CAN_SEE(ch, tmp_char))
					continue;
				sprintf(buf + strlen(buf), "%s %s",
					found++ ? "," : "", GET_NAME(tmp_char));
				if (strlen(buf) >= 62) {
					if (tmp_char->next_in_furniture)
						send_to_char(ch, "%s,\r\n",
							     buf);
					else
						send_to_char(ch, "%s\r\n", buf);
					*buf = '\0';
					found = 0;
				}
			}
			if (found)
				send_to_char(ch, "%s\r\n", buf);
			release_buffer(buf);
		}

		if (GET_OBJ_TSLOTS(tmp_object) == 0)
			condition = 0;
		else

			condition = (GET_OBJ_CSLOTS(tmp_object) * 100) /
			             GET_OBJ_OSLOTS(tmp_object);
		
		if (condition == 0)
			send_to_char(ch, "This looks indestructable!\r\n");
		else if (condition < 0)
			send_to_char(ch, "This looks broken.\r\n");
		else if (condition <= 10)
			send_to_char(ch, "This looks in extremely poor condition.\r\n");
		else if (condition <= 20)
			send_to_char(ch, "This looks in poor condition.\r\n");
		else if (condition <= 30)
			send_to_char(ch, "This looks in fair condition.\r\n");
		else if (condition <= 40)
			send_to_char(ch, "This looks in moderate condition.\r\n");
		else if (condition <= 50)
			send_to_char(ch, "This looks in good condition.\r\n");
		else if (condition <= 60)
			send_to_char(ch, "This looks in very good condition.\r\n");
		else if (condition <= 70)
			send_to_char(ch, "This looks in excellent condition.\r\n");
		else if (condition <= 80)
			send_to_char(ch, "This looks in superior condition.\r\n");
		else if (condition <= 90)
			send_to_char(ch, "This looks in extremely superior condition.\r\n");
		else
			send_to_char(ch, "This looks as good as new!.\r\n");

		buf = "Wear condition:";

		if (GET_OBJ_OSLOTS(tmp_object) == 0)
			condition = 0;
		else
			condition =
			    (GET_OBJ_TSLOTS(tmp_object) * 100) /
			    GET_OBJ_OSLOTS(tmp_object);
		if (condition == 0)
			send_to_char(ch, "%s %s.\r\n", buf, item_wear[0]);
		else if (condition <= 9)
			send_to_char(ch, "%s %s.\r\n", buf, item_wear[1]);
		else if (condition <= 19)
			send_to_char(ch, "%s %s.\r\n", buf, item_wear[2]);
		else if (condition <= 29)
			send_to_char(ch, "%s %s.\r\n", buf, item_wear[3]);
		else if (condition <= 39)
			send_to_char(ch, "%s %s.\r\n", buf, item_wear[4]);
		else if (condition <= 49)
			send_to_char(ch, "%s %s.\r\n", buf, item_wear[5]);
		else if (condition <= 59)
			send_to_char(ch, "%s %s.\r\n", buf, item_wear[6]);
		else if (condition <= 69)
			send_to_char(ch, "%s %s.\r\n", buf, item_wear[7]);
		else if (condition <= 79)
			send_to_char(ch, "%s %s.\r\n", buf, item_wear[8]);
		else if (condition <= 89)
			send_to_char(ch, "%s %s.\r\n", buf, item_wear[9]);
		else if (condition <= 99)
			send_to_char(ch, "%s %s.\r\n", buf, item_wear[10]);
		else
			send_to_char(ch, "%s %s.\r\n", buf, item_wear[11]);
	}
	release_buffer(arg);

}

ACMD(do_gold)
{
	if ((GET_GOLD(ch) == 0) && (GET_BANK_GOLD(ch) == 0))
		send_to_char(ch, "You're broke!\r\n");
	else {
		send_to_char(ch, "You have %ld gold coins in your purse.\r\n"
			     "And %ld gold coins in the bank.\r\n"
			     "For a total of %ld coins.\r\n",
			     GET_GOLD(ch), GET_BANK_GOLD(ch),
			     GET_GOLD(ch) + GET_BANK_GOLD(ch));
	}
}

ACMD(do_score)
{
	char *buf_temp1 = get_buffer(MAX_STRING_LENGTH);
	char *buf_temp2 = get_buffer(MAX_STRING_LENGTH);
	char *buf = get_buffer(MAX_STRING_LENGTH);
	char *racebuf = get_buffer(50);

	sprintf(buf,
		"\n%s*****************************************************************************\r\n",
		CCGRN(ch, C_SPR));

	sprintf(buf_temp1, "%s %s", GET_NAME(ch), GET_TITLE(ch));
	sprintf(buf + strlen(buf), "* %-41.41s Age: %d years old     Level ",
		buf_temp1, GET_AGE(ch));
	if (GET_LEVEL(ch) < 10)
		sprintf(buf + strlen(buf), " %ld  *\r\n", (long)GET_LEVEL(ch));
	else if (GET_LEVEL(ch) < 100)
		sprintf(buf + strlen(buf), "%ld  *\r\n", (long)GET_LEVEL(ch));
	else
		sprintf(buf + strlen(buf), "%ld *\r\n", (long)GET_LEVEL(ch));
	strcat(buf,
	       "*****************************************************************************\r\n");
	sprintf(buf_temp1, "Hit Points  : %ld/(%ld)", (long)GET_HIT(ch),
		(long)GET_MAX_HIT(ch));
	sprintf(buf_temp2, "Class : %s", pc_class_types[(int)GET_CLASS(ch)]);
	sprintf(buf + strlen(buf), "* %-34.34s%-40.40s*\r\n", buf_temp1,
		buf_temp2);
	sprintf(buf_temp1, "Energy      : %ld/(%ld)", (long)GET_MANA(ch),
		(long)GET_MAX_MANA(ch));

	if (ch->player.race == RACE_HUMAN)
		sprintf(racebuf, "Human");
	else if (ch->player.race == RACE_ELF)
		sprintf(racebuf, "Elf");
	else if (ch->player.race == RACE_H_ELF)
		sprintf(racebuf, "Half Elf");
	else if (ch->player.race == RACE_D_ELF)
		sprintf(racebuf, "Dark Elf");
	else if (ch->player.race == RACE_DWARF)
		sprintf(racebuf, "Dwarf");
	else if (ch->player.race == RACE_HALFLING)
		sprintf(racebuf, "Halfling");
	else if (ch->player.race == RACE_SPRITE)
		sprintf(racebuf, "Sprite");
	else if (ch->player.race == RACE_MINOTAUR)
		sprintf(racebuf, "Minotaur");
	else if (ch->player.race == RACE_AVIAN)
		sprintf(racebuf, "Avian");
	else if (ch->player.race == RACE_H_OGRE)
		sprintf(racebuf, "Half Ogre");
	else if (ch->player.race == RACE_H_ORC)
		sprintf(racebuf, "Half Orc");
	else if (ch->player.race == RACE_DRACONIAN)
		sprintf(racebuf, "Draconian");
	else if (ch->player.race == RACE_SHADOW)
		sprintf(racebuf, "Shadow");
	else if (ch->player.race == RACE_TITAN)
		sprintf(racebuf, "Titan");
	else if (ch->player.race == RACE_AESIR)
		sprintf(racebuf, "Aesir");
	else if (ch->player.race == RACE_UNDEFINED)
		sprintf(racebuf, "MOB");
	else
		sprintf(racebuf, "MOB");

	sprintf(buf_temp2, "Race  : %s", racebuf);
	release_buffer(racebuf);

	sprintf(buf + strlen(buf), "* %-34.34s%-40.40s*\r\n", buf_temp1,
		buf_temp2);

	sprintf(buf_temp1, "Move Points : %ld/(%ld)", (long)GET_MOVE(ch),
		(long)GET_MAX_MOVE(ch));
	sprintf(buf_temp2, "Sex   : %s", genders[(int)GET_SEX(ch)]);
	sprintf(buf + strlen(buf), "* %-34.34s%-40.40s*\r\n", buf_temp1,
		buf_temp2);

	sprintf(buf_temp1, "Exp Points  : %ld", (long)GET_EXP(ch));
	if (IS_NPC(ch)) {
		strcpy(buf_temp2, "Exp to next level : You are a mob");
	} else if (GET_LEVEL(ch) < LVL_IMMORT) {
		sprintf(buf_temp2, "Exp to next level : %ld",
			(GET_EXP_FOR_CH(ch) - GET_EXP(ch)));
	} else
		strcpy(buf_temp2, "                                         ");
	sprintf(buf + strlen(buf), "* %-34.34s%-40.40s*\r\n", buf_temp1,
		buf_temp2);

	if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_SUMMONABLE))
		sprintf(buf_temp2, "Summonable : Yes");
	else
		sprintf(buf_temp2, "Summonable : No");
	sprintf(buf + strlen(buf), "* Alignment   : %-20.20s%-40.40s*\r\n",
		naturestr(GET_ALIGNMENT(ch)), buf_temp2);

	strcat(buf,
	       "*****************************************************************************\r\n");
	sprintf(buf_temp1, "* Armor Class   : You are %s (%d)",
		acstr(compute_armor_class(ch)), compute_armor_class(ch));
	sprintf(buf + strlen(buf), "%-53.53s                       *\r\n",
		buf_temp1);

	if (GET_LEVEL(ch) > 19) {
		sprintf(buf_temp1, "* To Hit        : %s (%d)",
			bonstr(ch->points.hitroll), ch->points.hitroll);
		sprintf(buf_temp2, "To Damage    : %s (%d)",
			bonstr(ch->points.damroll), ch->points.damroll);
		sprintf(buf + strlen(buf), "%-36.36s%-40.40s*\r\n", buf_temp1,
			buf_temp2);
	}

	sprintf(buf_temp1, "* Gold Held     : %ld", GET_GOLD(ch));
	sprintf(buf_temp2, "Gold in Bank : %ld", GET_BANK_GOLD(ch));
	sprintf(buf + strlen(buf), "%-36.36s%-40.40s*\r\n", buf_temp1,
		buf_temp2);

	sprintf(buf_temp2, "Auto Commands : ");
	if (IS_NPC(ch))
		strcat(buf_temp2, "You are a mob");
	else {
		if (PRF_FLAGGED(ch, PRF_AUTOEXIT))
			strcat(buf_temp2, "Exit ");
		if (PRF_FLAGGED(ch, PRF_AUTOLOOT))
			strcat(buf_temp2, "Loot ");
		else if (PRF_FLAGGED(ch, PRF_AUTOGOLD))
			strcat(buf_temp2, "Gold ");
		if (PRF_FLAGGED(ch, PRF_AUTOSPLIT))
			strcat(buf_temp2, "Split ");
		if (PRF_FLAGGED(ch, PRF_AUTOASSIST))
			strcat(buf_temp2, "Assist ");
		if (PRF_FLAGGED(ch, PRF_AUTOSAC))
			strcat(buf_temp2, "Sacrifice");
	}
	sprintf(buf + strlen(buf), "* %-74.74s*\r\n", buf_temp2);

	sprintf(buf_temp1, "* Carried Items : %d/%ld", IS_CARRYING_N(ch),
		(long)CAN_CARRY_N(ch));
	sprintf(buf_temp2, "Carry Weight : %d/%d", IS_CARRYING_W(ch),
		CAN_CARRY_W(ch));
	sprintf(buf + strlen(buf), "%-36.36s%-40.40s*\r\n", buf_temp1,
		buf_temp2);

	sprintf(buf_temp2, "* Time Played   : %ldhrs",
		ch->player.time.played / 3600);

	strcpy(buf_temp1, "Position     : ");
	if (FURNITURE(ch)) {
		char *tmpbuf = get_buffer(128);
		strcpy(tmpbuf, position_types[(int)GET_POS(ch)]);
		tmpbuf = LOW(tmpbuf);
		sprintf(buf_temp1 + strlen(buf_temp1), "You are %s on %s.\r\n",
			tmpbuf, FURNITURE(ch)->short_description);
		release_buffer(tmpbuf);
	} else {
		switch (GET_POS(ch)) {
		case POS_DEAD:
			strcat(buf_temp1, "You are DEAD!");
			break;
		case POS_MORTALLYW:
			strcat(buf_temp1,
			       "You are mortally wounded!  You should seek help!");
			break;
		case POS_INCAP:
			strcat(buf_temp1,
			       "You are incapacitated, slowly fading away...");
			break;
		case POS_STUNNED:
			strcat(buf_temp1, "You are stunned!  You can't move!");
			break;
		case POS_SLEEPING:
			strcat(buf_temp1, "Sleeping");
			break;
		case POS_RESTING:
			strcat(buf_temp1, "Resting");
			break;
		case POS_CHANT:
			strcat(buf_temp1, "Chanting");
			break;
		case POS_MEDITATE:
			strcat(buf_temp1, "Meditating");
			break;
		case POS_BANDAGE:
			strcat(buf_temp1, "Bandaged");
			break;
		case POS_SITTING:
			strcat(buf_temp1, "Sitting");
			break;
		case POS_FIGHTING:
			if (ch->char_specials.fighting) {
				strcat(buf_temp1, "Fighting ");
				strcat(buf_temp1, PERS(ch->char_specials.fighting, ch));
				//sprintf(buf_temp1, "%sFighting %s", buf_temp1, PERS(ch->char_specials.fighting, ch));
			} else
				strcat(buf_temp1, "Fighting nothing");
			break;
		case POS_STANDING:
			strcat(buf_temp1, "Standing");
			break;
		default:
			strcat(buf_temp1, "Floating");
			break;
		}
	}

	sprintf(buf + strlen(buf), "%-36.36s%-40.40s*\r\n", buf_temp2,
		buf_temp1);

	int total = ch->player_specials->explored_total;
	float fraction = 100 * total / (float)top_of_world;
	sprintf(buf_temp1, "* Explored      : %d room%s (%.3f%%)", total,
		total != 1 ? "s" : "", fraction);
	sprintf(buf + strlen(buf), "%-76s*\r\n", buf_temp1);

	if (!IS_NPC(ch) && IN_ROOM(ch) >= 0 && IN_ROOM(ch) < EXPLORED_TOP_VNUM) {
		struct zone_data *zone = &zone_table[world[IN_ROOM(ch)].zone];
		int znum = zone->number;

		int num_explored = 0;
		int rnum;
		for (rnum = 100 * znum; rnum < 100 * (znum + 1); rnum++) {
			int b = ch->player_specials->explored_vnums[rnum / 8];
			if (b & (1 << (rnum % 8))) {
				num_explored++;
			}
		}

		char buf2[1024] = { '\x0' };
		sprintf(buf2, "* Explored zone : %s, %d/%d rooms", zone->name,
			num_explored, zone->num_rooms);
		while (strlen(buf2) < 76) {
			strcat(buf2, " ");
		}
		strcat(buf2, "*\r\n");
		strcat(buf, buf2);
	}

	if (GET_GUARDING(ch)) {
		sprintf(buf_temp1, "* Guarding      : %s",
			GET_NAME(GET_GUARDING(ch)));
		sprintf(buf + strlen(buf), "%-76s*\r\n", buf_temp1);
	}

	strcat(buf,
	       "*****************************************************************************\r\n");

	sprintf(buf + strlen(buf),
		"*   Please type \"attributes\" to view your stats.                            *\r\n");

	sprintf(buf + strlen(buf),
		"*****************************************************************************%s\r\n",
		CCNRM(ch, C_SPR));

	if (ch->desc)
		page_string(ch->desc, buf, TRUE, "");
	release_buffer(buf);
	release_buffer(buf_temp1);
	release_buffer(buf_temp2);
}

int compare_zone_by_name(const void* aa, const void* bb) {
    const struct zone_data* za = aa;
    const struct zone_data* zb = bb;

    return strcmp(za->name, zb->name);
}

ACMD(do_explored)
{
	char *buf = get_buffer(32750);
	char *buf2 = get_buffer(MAX_STRING_LENGTH);
	sprintf(buf, "Here are the places you have visited:\r\n");

    struct zone_data* sorted_ztable = malloc(sizeof(struct zone_data)*top_of_zone_table);
    memcpy(sorted_ztable, zone_table, sizeof(struct zone_data)*top_of_zone_table);

    qsort(sorted_ztable, top_of_zone_table, sizeof(struct zone_data), compare_zone_by_name);

	int ii, jj, rnum;
	for (ii = 0; ii < top_of_zone_table; ii++) {
        struct zone_data* zone = &sorted_ztable[ii];
		for (jj = 0; jj < 100; jj++) {
			int vnum = 100 * zone->number + jj;
			if (ch->player_specials->explored_vnums[vnum / 8] & (1 << (vnum % 8))) {
                int num_explored = 0;
                for (rnum = 100 * zone->number; rnum < 100 * (zone->number + 1); rnum++) {
                    int b = ch->player_specials->explored_vnums[rnum / 8];
                    if (b & (1 << (rnum % 8))) {
                        num_explored++;
                    }
                }
                float exp = ((float)num_explored/(float)zone->num_rooms)*100.0;
				sprintf(buf2, "   %s - %.0lf%% (%d/%d)\r\n", zone->name, exp, num_explored, zone->num_rooms);

				if (strlen(buf) + strlen(buf2) < 32700) {
					strcat(buf, buf2);
				}
				break;
			}
		}
	}

	/* WAIT_STATE(ch, PULSE_VIOLENCE); */
	page_string(ch->desc, buf, TRUE, "");
    free(sorted_ztable);
	release_buffer(buf2);
	release_buffer(buf);
}

ACMD(do_attribute)
{
	int real_stat;

	real_stat = MIN(9, GET_STR(ch) / 3);
	send_to_char(ch, "Strength: %s (%d)\r\n", str_strings[real_stat],
		     GET_STR(ch));
	real_stat = MIN(9, GET_CON(ch) / 3);
	send_to_char(ch, "Constitution: %s (%d)\r\n", con_strings[real_stat],
		     GET_CON(ch));
	real_stat = MIN(9, GET_DEX(ch) / 3);
	send_to_char(ch, "Dexterity: %s (%d)\r\n", dex_strings[real_stat],
		     GET_DEX(ch));
	real_stat = MIN(9, GET_INT(ch) / 3);
	send_to_char(ch, "Intelligence: %s (%d)\r\n", int_strings[real_stat],
		     GET_INT(ch));
	real_stat = MIN(9, GET_WIS(ch) / 3);
	send_to_char(ch, "Wisdom: %s (%d)\r\n", wis_strings[real_stat],
		     GET_WIS(ch));
	real_stat = MIN(9, GET_CHA(ch) / 3);
	send_to_char(ch, "Charisma: %s (%d)\r\n", cha_strings[real_stat],
		     GET_CHA(ch));
}

ACMD(do_inventory)
{
	if ((GET_POS(ch) == POS_SLEEPING) && !AFF_FLAGGED(ch, AFF_DREAM)) {
		send_to_char(ch,
			     "You can't see anything, you're sleeping!\r\n");
		return;
	}

	send_to_char(ch, "You are carrying:\r\n");
	list_obj_to_char(ch->carrying, ch, 1, TRUE);
}

ACMD(do_equipment)
{
	int i, found = 0;

	if ((GET_POS(ch) == POS_SLEEPING) && !AFF_FLAGGED(ch, AFF_DREAM)) {
		send_to_char(ch,
			     "You can't see anything, you're sleeping!\r\n");
		return;
	}

	send_to_char(ch, "You are using:\r\n");
	for (i = 0; i < NUM_WEARS; i++) {
		if (GET_EQ(ch, i)) {
			if (CAN_SEE_OBJ(ch, GET_EQ(ch, i))) {
				send_to_char(ch, "%s", where[i]);
				show_obj_to_char(GET_EQ(ch, i), ch, 1);
				found = TRUE;
			} else {
				send_to_char(ch, "%sSomething.\r\n", where[i]);
				found = TRUE;
			}
		}
	}
	if (!found) {
		send_to_char(ch, " Nothing.\r\n");
	}
}

#define VIS_AFF_FLAGS(loc) (((loc) == APPLY_NONE) || ((loc) == APPLY_AFF2) || \
                            ((loc) == APPLY_AFF3))

ACMD(do_affects)
{
	struct affected_type *af;
	int type = 1;
	char *sname = get_buffer(256);
	char *added_aff = get_buffer(MAX_STRING_LENGTH);
	char *buf = get_buffer(MAX_STRING_LENGTH);
	char *buf1 = get_buffer(MAX_STRING_LENGTH);

	if (AFF2_FLAGGED(ch, AFF2_FLYING)) {
		send_to_char(ch, "You are flying.\r\n");
	} else if (AFF_FLAGGED(ch, AFF_FLY)) {
		send_to_char(ch, "You can fly.\r\n");
	}
	send_to_char(ch, "You carry these affections: \r\n");
	for (af = ch->affected; af; af = af->next) {
		strcpy(sname, spells[af->type].spell_name);
		strcat(sname, ":");
		if (GET_LEVEL(ch) >= 25) {
			send_to_char(ch,
				     "   %s%-22s%s    affects %s%s%s by %s%ld%s for %s%d%s hours\r\n",
				     CCCYN(ch, C_NRM), (type ? sname : ""),
				     CCNRM(ch, C_NRM), CCCYN(ch, C_NRM),
				     ((GET_LEVEL(ch) >= 40
				       && !VIS_AFF_FLAGS(af->
							 location)) ?
				      apply_types[(int)af->
						  location] : "Something"),
				     CCNRM(ch, C_NRM), CCCYN(ch, C_NRM),
				     af->modifier, CCNRM(ch, C_NRM), CCCYN(ch,
									   C_NRM),
				     af->duration, CCNRM(ch, C_NRM));
			if (GET_LEVEL(ch) >= 55
			    && (af->bitvector
				&& (!af->next
				    || af->next->bitvector != af->bitvector))) {
				sprintbit(af->bitvector,
					  WHICH_BITS(af->location), added_aff);
				send_to_char(ch, "%35sadds %s%s\r\n",
					     CCCYN(ch, C_NRM), added_aff,
					     CCNRM(ch, C_NRM));
			}
		} else if (type) {
			send_to_char(ch, "   %s%-25s%s\r\n", CCCYN(ch, C_NRM),
				     sname, CCNRM(ch, C_NRM));
		}
		type = af->next ? (af->next->type != af->type) : 1;
	}
	release_buffer(buf1);
	release_buffer(buf);
	release_buffer(added_aff);
	release_buffer(sname);
}

ACMD(do_time)
{
	char *suf;
	int weekday, day;

	/* 35 days in a month */
	weekday = ((35 * time_info.month) + time_info.day + 1) % 7;
	send_to_char(ch, "It is %d:%2.2d %s, on %s,\r\n",
		     ((time_info.hours % 12 ==
		       0) ? 12 : ((time_info.hours) % 12)), time_info.minutes,
		     ((time_info.hours >= 12) ? "pm" : "am"),
		     weekdays[weekday]);

	day = time_info.day + 1;	/* day in [1..35] */

	if ((day % 10) == 1)
		suf = "st";
	else if ((day % 10) == 2)
		suf = "nd";
	else if ((day % 10) == 3)
		suf = "rd";
	else
		suf = "th";

	send_to_char(ch, "The %d%s Day of the %s, Year %d.\r\n",
		     day, suf, month_name[(int)time_info.month],
		     time_info.year);
}

ACMD(do_weather)
{
	static char *sky_look[] = {
		"cloudless",
		"cloudy",
		"rainy",
		"lit by flashes of lightning"
	};

	if (OUTSIDE(ch)) {
		send_to_char(ch, "The sky is %s and %s.\r\n",
			     sky_look[weather_info.sky],
			     (weather_info.change >=
			      0 ? "you feel a warm wind from south" :
			      "your foot tells you bad weather is due"));
	} else
		send_to_char(ch,
			     "You have no feeling about the weather at all.\r\n");
}

int is_help(const char *str, const char *namelist)
{
	const char *curname, *curstr;
	int in_quote = FALSE;
	/* MANWE: Handle an empty str */
	if (*str == '\0')
		return 0;

	curname = namelist;
	for (;;) {
		for (curstr = str;; curstr++, curname++) {
			if (!*curstr)
				return (1);
			if (*curstr == '.' && !isalpha((int)*curname))
				return (1);
			if (*curname == '"') {
				if (in_quote == TRUE) {
					/* We are comparing the last chars of "test" and "test" */
					if (*curstr == '"')
						return (1);
					in_quote = FALSE;
				} else {
					/* compare test to "test" gotta go back one
					   to match test to test" instead of est to test */
					if (*curstr != '"')
						curstr--;
					in_quote = TRUE;
				}
				continue;
			}

			if (!*curname)
				return (0);

			if ((*curname == ' ') && !in_quote)
				break;

			if (LOWER(*curstr) != LOWER(*curname))
				break;
		}

		/* skip to next name */
		in_quote = FALSE;
		for (;
		     isalpha((int)*curname) || *curname == '-'
		     || *curname == '_'; curname++) ;
		if (!*curname)
			return (0);
		curname++;	/* first char of new name */
	}
}

struct help_index_element *find_help(char *keyword, int times)
{
	int i;

	for (i = 0; i < top_of_helpt; i++) {
		if (is_help(keyword, help_table[i].keywords)) {
			times--;
			if (!times) {
				return (help_table + i);
			}
		}
	}

	return NULL;
}

void get_other_possible_help_entries(struct char_data *ch, char *argument,
				     char *buf)
{
	int i;
	int row = 0;
	for (i = 0; i < top_of_helpt; i++) {
		if (isname(argument, help_table[i].keywords)) {
			if (help_table[i].min_level <= GET_LEVEL(ch)) {
				row++;
				sprintf(buf + strlen(buf), "|%-37.37s|",
					help_table[i].keywords);
				if ((row % 2) == 0) {
					strcat(buf, "\r\n");
				}
			}
		}
	}
	if ((row % 2) != 0)
		strcat(buf, "\r\n");
}

ACMD(do_help)
{
	struct help_index_element *this_help;
	char *entry;
	int times = 1;

	if (!ch->desc)
		return;

	skip_spaces(&argument);

	if (!*argument) {
		if (ch->desc)
			page_string(ch->desc, help, FALSE, "");
		return;
	}
	if (!help_table) {
		send_to_char(ch, "No help available.\r\n");
		return;
	}

	if (isdigit((int)*argument)) {
		char *ztTimes = get_buffer(64);
		argument = any_one_arg(argument, ztTimes);
		times = atoi(ztTimes);
		release_buffer(ztTimes);
		skip_spaces(&argument);
	}
	if (!(this_help = find_help(argument, times))) {
		send_to_char(ch, "There is no help on that word.\r\n");
		log("Help: %s Could not find help on %s entry %d", GET_NAME(ch),
		    argument, times);
		return;
	}

	if (this_help->min_level > GET_LEVEL(ch)) {
		send_to_char(ch, "There is no help on that word.\r\n");
		return;
	}
	entry = get_buffer(32750);
	sprintf(entry, "%s\r\n%s", this_help->keywords, this_help->entry);
	sprintf(entry + strlen(entry), "Other possible matches for %s: \r\n",
		argument);
	get_other_possible_help_entries(ch, argument, entry);
	if (GET_LEVEL(ch) >= LVL_IMMORT)
		sprintf(entry + strlen(entry), "Access Level: %d\n",
			this_help->min_level);
	page_string(ch->desc, entry, TRUE, "");
	release_buffer(entry);
}

ACMD(do_index)
{
	char *buf;

	if (!ch->desc)
		return;
	skip_spaces(&argument);

	if (!*argument) {
		send_to_char(ch, "USAGE: index <letter|phrase>\r\n");
		return;
	}

	buf = get_buffer(8192);

	get_other_possible_help_entries(ch, argument, buf);
	if (ch->desc)
		page_string(ch->desc, buf, TRUE, "");
	release_buffer(buf);

}

/*********************************************************************
* New 'do_who' by Daniel Koepke [aka., "Argyle Macleod"] of The Keep *
******************************************************************* */

char *WHO_USAGE =
    "Usage: who [minlev[-maxlev]] [-n name] [-c classes] [-rzqimoat]\r\n"
    "\r\n"
    " Switches: \r\n"
    "_.,-'^'-,._\r\n"
    "\r\n"
    "  -r = who is in the current room\r\n"
    "  -z = who is in the current zone\r\n"
    "\r\n"
    "  -q = only show questers\r\n"
    "  -i = only show immortals\r\n"
    "  -m = only show mortals\r\n"
    "  -o = only show outlaws\r\n"
    "  -a = only show your clan\r\n"
    "  -t = only show remorts\r\n"
    "  -c = only show the named class (can be used multiple times)\r\n"
    "       i.e who 1-50 -c cleric -c druid -c deva  will show low lvl healers\r\n"
    "  -p = only show pkillers\r\n" "\r\n";

ACMD(do_who)
{
	struct descriptor_data *d;
	struct char_data *wch;
	char mode;

	int low = 0, high = LVL_HIMPL, showclass = 0;
	bool who_room = FALSE, who_zone = FALSE, who_quest = 0;
	bool outlaws = FALSE, noimm = FALSE, nomort = FALSE;
	bool who_clan = FALSE;
	bool who_remort = FALSE;
	bool who_pk = FALSE;
	int Wizards = 0, Mortals = 0;

	char *buf = get_buffer(MAX_STRING_LENGTH);
	char *buf1 = get_buffer(MAX_STRING_LENGTH);
	char *arg = get_buffer(MAX_INPUT_LENGTH);
	char *Imm_buf = get_buffer(MAX_STRING_LENGTH);
	char *Mort_buf = get_buffer(MAX_STRING_LENGTH);
	char *name_search = get_buffer(64);

	if (IS_NPC(ch)) {
		release_buffer(buf);
		release_buffer(arg);
		release_buffer(Imm_buf);
		release_buffer(Mort_buf);
		release_buffer(name_search);
		release_buffer(buf1);
		return;
	}

	skip_spaces(&argument);
	strcpy(buf, argument);
	name_search[0] = '\0';

	/* the below is from stock CircleMUD -- found no reason to rewrite it */
	while (*buf) {
		half_chop(buf, arg, buf1);
		if (isdigit((int)*arg)) {
			sscanf(arg, "%d-%d", &low, &high);
			strcpy(buf, buf1);
		} else if (*arg == '-') {
			mode = *(arg + 1);	/* just in case; we destroy arg in the switch */
			switch (mode) {
			case 'o':
				outlaws = TRUE;
				strcpy(buf, buf1);
				break;
			case 'z':
				who_zone = TRUE;
				strcpy(buf, buf1);
				break;
			case 'q':
				who_quest = TRUE;
				strcpy(buf, buf1);
				break;
			case 'l':
				half_chop(buf1, arg, buf);
				sscanf(arg, "%d-%d", &low, &high);
				break;
			case 'n':
				half_chop(buf1, name_search, buf);
				break;
			case 'r':
				who_room = TRUE;
				strcpy(buf, buf1);
				break;
			case 'c':
				half_chop(buf1, arg, buf);
				showclass |= find_class_bitvector(arg);
				break;
			case 'i':
				nomort = TRUE;
				strcpy(buf, buf1);
				break;
			case 'm':
				noimm = TRUE;
				strcpy(buf, buf1);
				break;
			case 'a':
				who_clan = TRUE;
				strcpy(buf, buf1);
				break;
			case 't':
				who_remort = TRUE;
				strcpy(buf, buf1);
				break;
			case 'p':
				who_pk = TRUE;
				strcpy(buf, buf1);
				break;
			default:
				send_to_char(ch, "%s", WHO_USAGE);
				release_buffer(buf);
				release_buffer(arg);
				release_buffer(Imm_buf);
				release_buffer(Mort_buf);
				release_buffer(name_search);
				release_buffer(buf1);
				return;
				break;
			}	/* end of switch */

		} else {	/* endif */
			send_to_char(ch, "%s", WHO_USAGE);
			release_buffer(buf);
			release_buffer(buf1);
			release_buffer(arg);
			release_buffer(Imm_buf);
			release_buffer(Mort_buf);
			release_buffer(name_search);
			return;
		}
	}			/* end while (parser) */

	strcpy(Imm_buf, "Immortals\r\n---------\r\n");
	strcpy(Mort_buf, "Mortals\r\n");
	if (showclass)
		sprintf(Mort_buf + strlen(Mort_buf), "   of class %s\r\n",
			pc_class_types[parse_class(arg)]);
	strcat(Mort_buf, "-------\r\n");

	for (d = descriptor_list; d; d = d->next) {
		if (STATE(d) != CON_PLAYING &&
		    (STATE(d) != CON_MEDIT) &&
		    (STATE(d) != CON_OEDIT) &&
		    (STATE(d) != CON_REDIT) &&
		    (STATE(d) != CON_SEDIT) &&
		    (STATE(d) != CON_GEDIT) &&
		    (STATE(d) != CON_PEDIT) &&
		    (STATE(d) != CON_TRIGEDIT) &&
		    (STATE(d) != CON_HEDIT) && (STATE(d) != CON_ZEDIT))
			continue;

		if (d->original)
			wch = d->original;
		else if (!(wch = d->character))
			continue;

		if (!CAN_SEE(ch, wch))
			continue;
		if (GET_LEVEL(wch) < low || GET_LEVEL(wch) > high)
			continue;
		if ((noimm && GET_LEVEL(wch) >= LVL_IMMORT)
		    || (nomort && GET_LEVEL(wch) < LVL_IMMORT))
			continue;
		if (*name_search && str_cmp(GET_NAME(wch), name_search)
		    && !strstr(GET_TITLE(wch), name_search))
			continue;
		if (outlaws && !PLR_FLAGGED(wch, PLR_KILLER)
		    && !PLR_FLAGGED(wch, PLR_THIEF))
			continue;
		if (who_quest && !PRF_FLAGGED(wch, PRF_QUEST))
			continue;
		if (who_zone
		    && world[IN_ROOM(ch)].zone != world[IN_ROOM(wch)].zone)
			continue;
		if (who_room && (IN_ROOM(wch) != IN_ROOM(ch)))
			continue;
		if (showclass && !(showclass & (1 << GET_CLASS(wch))))
			continue;
		if (who_clan &&
		    ((GET_CLAN(wch) == 0) ||
		     ((GET_CLAN(ch) != GET_CLAN(wch)) &&
		      (GET_LEVEL(ch) < LVL_ADMIN))))
			continue;
		if (who_pk && !PLR_FLAGGED(wch, PLR_PK))
			continue;
		if (who_remort && is_remort_level(wch, NON_REMORT))
			continue;

		if (GET_LEVEL(wch) >= LVL_IMMORT) {
			sprintf(Imm_buf + strlen(Imm_buf), "%s[%s] %s %s ",
				CCYEL(ch, C_SPR),
				WizLevels[GET_LEVEL(wch) - LVL_IMMORT],
				GET_NAME(wch), GET_TITLE(wch));
			Wizards++;
		} else {
			/* 10/27/96, Echo - format has been changed slightly to reflect
			 *   added levels, races, and classes.
			 */
			sprintf(Mort_buf + strlen(Mort_buf),
				"[%3d %2.2s%c%3.3s] %s %s ", GET_LEVEL(wch),
				CLASS_ABBR(wch),
				REMORT_LEVEL(wch) == TRIPLE_REMORT ? '+' : '-',
				RACE_ABBR(wch), GET_NAME(wch), GET_TITLE(wch));
			Mortals++;
		}

		*buf = '\0';
		if (GET_INVIS_LEV(wch))
			sprintf(buf, "(i%d)", GET_INVIS_LEV(wch));
		else if (AFF_FLAGGED(wch, AFF_INVISIBLE))
			strcat(buf, "(invis)");

		if (PRF2_FLAGGED(wch, PRF2_MORTAL))
			strcat(buf, "(MORTAL!)");

		if (GET_CLAN(wch) > 0)
			sprintf(buf + strlen(buf), "(%s)", GET_CLAN_NAME(wch));
		if (GET_LEVEL(wch) == LVL_HERO)
			strcat(buf, "&k&3(HERO)&n");
		else if (GET_LEVEL(wch) == LVL_ANGEL)
			strcat(buf, "&k&2(ANGEL)&n");
		else if (GET_LEVEL(wch) == LVL_AVATAR)
			strcat(buf, "&k&6(AVATAR)&n");
		else if (GET_LEVEL(wch) == LVL_WANKER)
			strcat(buf, "&k&4(DEMI-GOD)&n");

		if (PLR_FLAGGED(wch, PLR_MAILING))
			strcat(buf, "(mailing)");
		else if (PLR_FLAGGED(wch, PLR_WRITING) &&
			 (STATE(d) != CON_MEDIT) &&
			 (STATE(d) != CON_OEDIT) &&
			 (STATE(d) != CON_REDIT) &&
			 (STATE(d) != CON_SEDIT) &&
			 (STATE(d) != CON_GEDIT) &&
			 (STATE(d) != CON_PEDIT) &&
			 (STATE(d) != CON_TRIGEDIT) &&
			 (STATE(d) != CON_HEDIT) && (STATE(d) != CON_ZEDIT))
			strcat(buf, "(writing)");

		if (PRF_FLAGGED(wch, PRF_DEAF))
			strcat(buf, "(deaf)");
		if (PRF_FLAGGED(wch, PRF_NOTELL))
			strcat(buf, "(notell)");
		if (PRF_FLAGGED(wch, PRF_QUEST))
			strcat(buf, "(quest)");
		if (PLR_FLAGGED(wch, PLR_THIEF))
			strcat(buf, "(THIEF)");
		if (PLR_FLAGGED(wch, PLR_KILLER))
			strcat(buf, "(KILLER)");
		if (GET_LEVEL(ch) >= LVL_IMMORT) {
			if (STATE(d) == CON_OEDIT)
				strcat(buf, "(OLC - OEdit)");
			else if (STATE(d) == CON_MEDIT)
				strcat(buf, "(OLC - MEdit)");
			else if (STATE(d) == CON_REDIT)
				strcat(buf, "(OLC - REdit)");
			else if (STATE(d) == CON_ZEDIT)
				strcat(buf, "(OLC - ZEdit)");
			else if (STATE(d) == CON_SEDIT)
				strcat(buf, "(OLC - SEdit)");
			else if (STATE(d) == CON_GEDIT)
				strcat(buf, "(OLC - GEdit)");
			else if (STATE(d) == CON_PEDIT)
				strcat(buf, "(OLC - PEdit)");
			else if (STATE(d) == CON_HEDIT)
				strcat(buf, "(OLC - HEdit)");
			else if (STATE(d) == CON_TRIGEDIT)
				strcat(buf, "(OLC - TrigEdit)");
		} else if (STATE(d) == CON_GEDIT ||
			   STATE(d) == CON_MEDIT ||
			   STATE(d) == CON_OEDIT ||
			   STATE(d) == CON_PEDIT ||
			   STATE(d) == CON_REDIT ||
			   STATE(d) == CON_SEDIT ||
			   STATE(d) == CON_TRIGEDIT ||
			   STATE(d) == CON_HEDIT || STATE(d) == CON_ZEDIT)
			strcat(buf, "(writing)");

		if (PLR_FLAGGED(wch, PLR_PK))
			strcat(buf, "(PK)");
		if (GET_LEVEL(wch) >= LVL_IMMORT)
			strcat(buf, CCNRM(ch, C_SPR));
		if (PRF2_FLAGGED(wch, PRF2_AFK))
			strcat(buf, "(AFK)");
		if (AFF_FLAGGED(wch, AFF_PLAGUE))
			strcat(buf, "&G(PLAGUE !!)&n");
		strcat(buf, "\r\n");

		if (GET_LEVEL(wch) >= LVL_IMMORT) {
			strcat(Imm_buf, buf);
		} else {
			strcat(Mort_buf, buf);
		}
	}			/* end of for */

	*buf = '\0';
	if (Wizards) {
		strcat(Imm_buf, "\r\n");
		strcpy(buf, Imm_buf);
	}

	if (Mortals) {
		strcat(Mort_buf, "\r\n");
		strcat(buf, Mort_buf);
	}
	release_buffer(Imm_buf);
	release_buffer(Mort_buf);

	if ((Wizards + Mortals) == 0)
		strcpy(buf1,
		       "No wizards or mortals are currently visible to you.\r\n");

	if (Wizards)
		sprintf(buf1, "There %s %d visible immortal%s%s",
			(Wizards == 1 ? "is" : "are"), Wizards,
			(Wizards == 1 ? "" : "s"),
			(Mortals ? " and there " : "."));
	if (Mortals) {
		sprintf(buf1 + (Wizards ? strlen(buf1) : 0),
			"%s%s %d visible mortal%s.", (Wizards ? "" : "There "),
			(Mortals == 1 ? "is" : "are"), Mortals,
			(Mortals == 1 ? "" : "s"));
	}
	strcat(buf, buf1);

	strcat(buf, "\r\n");

	if ((Wizards + Mortals) > boot_high)
		boot_high = Wizards + Mortals;
	sprintf(buf + strlen(buf),
		"There is a boot time high of %d player%s.\r\n", boot_high,
		(boot_high == 1 ? "" : "s"));

	release_buffer(buf1);
	release_buffer(arg);
	release_buffer(name_search);
	if (ch->desc)
		page_string(ch->desc, buf, TRUE, "");
	else
		send_to_char(ch, "%s", buf);
	release_buffer(buf);
}

#define USERS_FORMAT \
"format: users [-l minlevel[-maxlevel]] [-n name] [-h host] [-c class] [-o] [-p]\r\n"

ACMD(do_users)
{
	char *timeptr, *format, mode;
	struct char_data *tch;
	struct descriptor_data *d;

	int low = 0, high = LVL_HIMPL, num_can_see = 0;
	int showclass = 0, outlaws = 0, playing = 0, deadweight = 0;
	char *buf = get_buffer(MAX_STRING_LENGTH);
	char *buf1 = get_buffer(MAX_STRING_LENGTH);
	char *arg = get_buffer(MAX_STRING_LENGTH);
	char *name_search = get_buffer(MAX_INPUT_LENGTH);
	char *host_search = get_buffer(MAX_INPUT_LENGTH);
	char *line2 = get_buffer(220);
	char *line = get_buffer(200);
	char *state = get_buffer(30);
	char *classname = get_buffer(20);
	char *idletime = get_buffer(10);

	strcpy(buf, argument);
	while (*buf) {
		half_chop(buf, arg, buf1);
		if (*arg == '-') {
			mode = *(arg + 1);	/* just in case; we destroy arg in the switch */
			switch (mode) {
			case 'o':
			case 'k':
				outlaws = 1;
				playing = 1;
				strcpy(buf, buf1);
				break;
			case 'p':
				playing = 1;
				strcpy(buf, buf1);
				break;
			case 'd':
				deadweight = 1;
				strcpy(buf, buf1);
				break;
			case 'l':
				playing = 1;
				half_chop(buf1, arg, buf);
				sscanf(arg, "%d-%d", &low, &high);
				break;
			case 'n':
				playing = 1;
				half_chop(buf1, name_search, buf);
				break;
			case 'h':
				playing = 1;
				half_chop(buf1, host_search, buf);
				break;
			case 'c':
				playing = 1;
				half_chop(buf1, arg, buf);
				showclass |= find_class_bitvector(arg);
				break;
			default:
				send_to_char(ch, USERS_FORMAT);
				release_buffer(buf);
				release_buffer(buf1);
				release_buffer(arg);
				release_buffer(name_search);
				release_buffer(host_search);
				release_buffer(line2);
				release_buffer(line);
				release_buffer(state);
				release_buffer(classname);
				release_buffer(idletime);
				return;
				break;
			}	/* end of switch */

		} else {	/* endif */
			send_to_char(ch, USERS_FORMAT);
			release_buffer(buf);
			release_buffer(buf1);
			release_buffer(arg);
			release_buffer(name_search);
			release_buffer(host_search);
			release_buffer(line2);
			release_buffer(line);
			release_buffer(state);
			release_buffer(classname);
			release_buffer(idletime);
			return;
		}
	}			/* end while (parser) */
	send_to_char(ch,
		     "Num Class    Name         State          Idl Login@   Site\r\n");
	send_to_char(ch,
		     "--- -------- ------------ -------------- --- -------- ------------------------\r\n");

	one_argument(argument, arg);

	for (d = descriptor_list; d; d = d->next) {
		if (STATE(d) != CON_PLAYING && playing)
			continue;
		if (d->character && STATE(d) != CON_PLAYING &&
		    (GET_INVIS_LEV(d->character) > GET_LEVEL(ch)))
			continue;
		if (STATE(d) == CON_PLAYING && deadweight)
			continue;
		if (STATE(d) == CON_PLAYING) {
			if (d->original)
				tch = d->original;
			else if (!(tch = d->character))
				continue;

			if (*host_search && !strstr(d->host, host_search))
				continue;
			if (*name_search && str_cmp(GET_NAME(tch), name_search))
				continue;
			if (!CAN_SEE(ch, tch) || GET_LEVEL(tch) < low
			    || GET_LEVEL(tch) > high)
				continue;
			if (outlaws && !PLR_FLAGGED(tch, PLR_KILLER) &&
			    !PLR_FLAGGED(tch, PLR_THIEF))
				continue;
			if (showclass && !(showclass & (1 << GET_CLASS(tch))))
				continue;
			if (GET_INVIS_LEV(tch) > GET_LEVEL(ch))
				continue;
			if (d->original)
				sprintf(classname, "[%3d %s]",
					GET_LEVEL(d->original),
					CLASS_ABBR(d->original));
			else
				sprintf(classname, "[%3d %s]",
					GET_LEVEL(d->character),
					CLASS_ABBR(d->character));
		} else if (STATE(d) == CON_GEDIT || STATE(d) == CON_MEDIT
			   || STATE(d) == CON_OEDIT || STATE(d) == CON_PEDIT
			   || STATE(d) == CON_REDIT || STATE(d) == CON_SEDIT
			   || STATE(d) == CON_TRIGEDIT || STATE(d) == CON_HEDIT
			   || STATE(d) == CON_ZEDIT) {
			if (d->character
			    && (GET_INVIS_LEV(d->character) > GET_LEVEL(ch)))
				continue;
			if (d->original)
				sprintf(classname, "[%3d %s]",
					GET_LEVEL(d->original),
					CLASS_ABBR(d->original));
			else
				sprintf(classname, "[%3d %s]",
					GET_LEVEL(d->character),
					CLASS_ABBR(d->character));
		} else
			strcpy(classname, "   -    ");

		timeptr = asctime(localtime(&d->login_time));
		timeptr += 11;
		*(timeptr + 8) = '\0';

		if (STATE(d) == CON_PLAYING && d->original)
			strcpy(state, "Switched");
		else
			strcpy(state, connected_types[STATE(d)]);

		if (d->character && STATE(d) == CON_PLAYING &&
		    GET_LEVEL(d->character) < LVL_IMPL)
			sprintf(idletime, "%3d",
				d->character->char_specials.timer *
				SECS_PER_MUD_HOUR / SECS_PER_REAL_MIN);
		else
			strcpy(idletime, "");

		format = "%3d %-7s %-12s %-14s %-3s %-8s ";

		if (d->character && d->character->player.name) {
			if (d->original)
				sprintf(line, format, d->desc_num, classname,
					d->original->player.name, state,
					idletime, timeptr);
			else
				sprintf(line, format, d->desc_num, classname,
					d->character->player.name, state,
					idletime, timeptr);
		} else
			sprintf(line, format, d->desc_num, "   -    ",
				"UNDEFINED", state, idletime, timeptr);

		if (d->host && *d->host)
			sprintf(line + strlen(line), "[%s]\r\n", d->host);
		else
			strcat(line, "[Hostname unknown]\r\n");

		if (STATE(d) != CON_PLAYING) {
			sprintf(line2, "%s%s%s", CCGRN(ch, C_SPR), line,
				CCNRM(ch, C_SPR));
			strcpy(line, line2);
		}
		if (STATE(d) != CON_PLAYING ||
		    (STATE(d) == CON_PLAYING && CAN_SEE(ch, d->character))) {
			send_to_char(ch, "%s", line);
			num_can_see++;
		}
	}

	send_to_char(ch, "\r\n%d visible sockets connected.\r\n", num_can_see);
	release_buffer(buf);
	release_buffer(buf1);
	release_buffer(arg);
	release_buffer(name_search);
	release_buffer(host_search);
	release_buffer(line2);
	release_buffer(line);
	release_buffer(state);
	release_buffer(classname);
	release_buffer(idletime);
	return;
}

/* Generic page_string function for displaying text */
ACMD(do_gen_ps)
{
	if (!ch->desc)
		return;
	switch (subcmd) {
	case SCMD_AREAS:
		page_string(ch->desc, areas, FALSE, "");
		break;
	case SCMD_CREDITS:
		page_string(ch->desc, credits, FALSE, "");
		break;
	case SCMD_NEWS:
		page_string(ch->desc, news, FALSE, "");
		break;
	case SCMD_INFO:
		page_string(ch->desc, info, FALSE, "");
		break;
	case SCMD_WIZLIST:
		page_string(ch->desc, wizlist, FALSE, "");
		break;
	case SCMD_IMMLIST:
		page_string(ch->desc, wizlist, FALSE, "");
		break;
	case SCMD_HANDBOOK:
		page_string(ch->desc, handbook, FALSE, "");
		break;
	case SCMD_POLICIES:
		page_string(ch->desc, policies, FALSE, "");
		break;
	case SCMD_TEAMS:
		page_string(ch->desc, teams, FALSE, "");
		break;
	case SCMD_MARRIAGES:
		page_string(ch->desc, marriages, FALSE, "");
		break;
	case SCMD_MOTD:
		page_string(ch->desc, motd, FALSE, "");
		break;
	case SCMD_IMOTD:
		page_string(ch->desc, imotd, FALSE, "");
		break;
	case SCMD_CLEAR:
		send_to_char(ch, "\033[H\033[J");
		break;
	case SCMD_VERSION:
		send_to_char(ch, "%s", circlemud_version);
#ifdef GIT_REF
#define STRING(a) #a
#define XSTRING(a) STRING(a)
		send_to_char(ch, "Latest commit: %s", XSTRING(GIT_REF));
#undef STRING
#undef XSTRING
#endif
		break;
	case SCMD_WHOAMI:
		send_to_char(ch, "%s\r\n", GET_NAME(ch));
		break;
	default:
		log("SYSERR: Unhandled case in do_gen_ps. (%d)", subcmd);
		break;
	}
}

void perform_graffiti_where(struct char_data *ch)
{
	char *buf = get_buffer(MAX_STRING_LENGTH);
	char *buf2 = get_buffer(MAX_STRING_LENGTH);
	sprintf(buf, "You have left graffiti at the following locations:\r\n");
	int i, counter = 0;
	for (i = 0; i < num_graffiti; i++) {
		if (graffiti[i].author == GET_IDNUM(ch)) {
			counter++;
			int rnum = real_room(graffiti[i].room_vnum);
			if (GET_LEVEL(ch) < LVL_IMMORT) {
				sprintf(buf2, "  %s (%s)\r\n", world[rnum].name,
					graffiti[i].
					permanent ? "permanent" : "temporary");
			} else {
				sprintf(buf2, "  [%5d] %s (%s)\r\n",
					graffiti[i].room_vnum, world[rnum].name,
					graffiti[i].
					permanent ? "permanent" : "temporary");
			}
			if (strlen(buf) + strlen(buf2) < MAX_STRING_LENGTH - 32) {
				strcat(buf, buf2);
			}
		}
	}
	if (counter) {
		send_to_char(ch, "%s", buf);
	} else {
		send_to_char(ch,
			     "You have yet to leave your mark on this world.  Get going!\r\n");
	}
	release_buffer(buf);
	release_buffer(buf2);
}

void perform_mortal_where(struct char_data *ch, char *arg)
{
	register struct char_data *i;
	register struct descriptor_data *d;

	if (!*arg) {
		send_to_char(ch,
			     "Players in your Zone\r\n--------------------\r\n");
		for (d = descriptor_list; d; d = d->next) {
			if (STATE(d) != CON_PLAYING || d->character == ch)
				continue;
			if ((i =
			     (d->original ? d->original : d->character)) ==
			    NULL)
				continue;
			if (i->in_room == NOWHERE || !CAN_SEE(ch, i))
				continue;
			if (world[ch->in_room].zone != world[i->in_room].zone)
				continue;

			send_to_char(ch, "%-20s - %s\r\n", GET_NAME(i),
				     world[IN_ROOM(i)].name);
		}
	} else {		/* print only FIRST char, not all. */
		for (i = character_list; i; i = i->next) {
			if (i->in_room == NOWHERE || i == ch)
				continue;
			if (!CAN_SEE(ch, i)
			    || world[i->in_room].zone !=
			    world[ch->in_room].zone)
				continue;
			if (!isname(arg, i->player.name))
				continue;

			send_to_char(ch, "%-25s - %s\r\n", GET_NAME(i),
				     world[IN_ROOM(i)].name);
			return;
		}

		if (starts_with("graffiti", arg)) {
			perform_graffiti_where(ch);
			return;
		}
		send_to_char(ch, "No-one around by that name.\r\n");
	}
}

void print_object_location(int num, struct obj_data *obj, struct char_data *ch,
			   int recur, char *pztBuf)
{

	if (num > 0)
		sprintf(pztBuf + strlen(pztBuf), "O%3d. %-25s - ", num,
			obj->short_description);
	else
		sprintf(pztBuf + strlen(pztBuf), "%33s", " - ");

	if (IN_ROOM(obj) > NOWHERE) {
		sprintf(pztBuf + strlen(pztBuf), "[%5ld] %s\r\n",
			GET_ROOM_VNUM(IN_ROOM(obj)), world[IN_ROOM(obj)].name);
	} else if (obj->carried_by) {
		sprintf(pztBuf + strlen(pztBuf), "carried by %s%s\r\n",
			PERS(obj->carried_by, ch), (recur ? ", who is" : " "));
		if (recur) {
			strcat(pztBuf, "  ");
			sprintf(pztBuf + strlen(pztBuf), "%33s", " - ");
			if (IN_ROOM(obj->carried_by) > NOWHERE) {
				sprintf(pztBuf + strlen(pztBuf),
					"in [%5ld] %s\r\n",
					GET_ROOM_VNUM(IN_ROOM(obj->carried_by)),
					world[IN_ROOM(obj->carried_by)].name);
			} else
				strcat(pztBuf, "Nowhere\r\n");

		}

	} else if (obj->worn_by) {
		sprintf(pztBuf + strlen(pztBuf), "worn by %s%s\r\n",
			PERS(obj->worn_by, ch), (recur ? ", who is" : " "));
		if (recur) {
			strcat(pztBuf, "  ");
			sprintf(pztBuf + strlen(pztBuf), "%33s", " - ");
			if (IN_ROOM(obj->worn_by) > NOWHERE) {
				sprintf(pztBuf + strlen(pztBuf),
					"in [%5ld] %s\r\n",
					GET_ROOM_VNUM(IN_ROOM(obj->worn_by)),
					world[IN_ROOM(obj->worn_by)].name);
			} else
				strcat(pztBuf, "Nowhere\r\n");

		}
	} else if (obj->in_obj) {
		sprintf(pztBuf + strlen(pztBuf), "inside %s%s\r\n",
			obj->in_obj->short_description,
			(recur ? ", which is" : " "));
		if (recur) {
			strcat(pztBuf, "  ");
			print_object_location(0, obj->in_obj, ch, recur,
					      pztBuf);
		}
	} else if (is_object_in_player_shop(obj)) {
		struct player_shop *shop = is_object_in_player_shop(obj);
		sprintf(pztBuf + strlen(pztBuf), "in %c%s's shop [%d]\r\n",
			toupper(shop->player_name[0]), shop->player_name + 1,
			shop->vnum_location);
	} else {

		sprintf(pztBuf + strlen(pztBuf), "in an unknown location\r\n");
	}
}

void perform_immort_where(struct char_data *ch, char *arg)
{
	register struct char_data *i;
	register struct obj_data *k;
	struct descriptor_data *d;
	int num = 0, found = 0;
	char *buf = get_buffer(32750);

	if (!*arg) {
		strcpy(buf, "Players\r\n-------\r\n");
		for (d = descriptor_list; d; d = d->next)
			if (STATE(d) == CON_PLAYING) {
				i = (d->original ? d->original : d->character);
				if (i && CAN_SEE(ch, i)
				    && (IN_ROOM(i) != NOWHERE)) {
					if (d->original)
						sprintf(buf + strlen(buf),
							"%-20s - [%5ld] %s (in %s)\r\n",
							GET_NAME(i),
							GET_ROOM_VNUM(IN_ROOM
								      (d->
								       character)),
							world[IN_ROOM
							      (d->character)].
							name,
							GET_NAME(d->character));
					else
						sprintf(buf + strlen(buf),
							"%-20s - [%5ld] %s\r\n",
							GET_NAME(i),
							GET_ROOM_VNUM(IN_ROOM
								      (i)),
							world[IN_ROOM(i)].name);
				}
			}
	} else {
		strcpy(buf, "Mobiles\r\n-------\r\n");
		for (i = character_list; i; i = i->next) {
			if (CAN_SEE(ch, i) && IN_ROOM(i) != NOWHERE
			    && isname(arg, i->player.name)) {
				found = 1;
				sprintf(buf + strlen(buf),
					"M%3d. %-25s - [%5ld] %s\r\n", ++num,
					GET_NAME(i), GET_ROOM_VNUM(IN_ROOM(i)),
					world[IN_ROOM(i)].name);
			}
			if (strlen(buf) > 32500) {
				sprintf(buf + strlen(buf),
					"Buffer limit exceeded, you need to refine your search\r\n");
				break;
			}
		}
		if (!found)
			strcat(buf, " None\r\n");

		if (strlen(buf) > 32500) {
			sprintf(buf + strlen(buf),
				"Buffer limit exceeded, you need to refine your search\r\n");
		} else {
			strcat(buf, "\r\nObjects\r\n-------\r\n");
			for (num = 0, k = object_list; k; k = k->next)
				if (CAN_SEE_OBJ(ch, k) && isname(arg, k->name)
				    && (!k->carried_by
					|| CAN_SEE(ch, k->carried_by))
				    && !(item_owner(k) && !IS_NPC(item_owner(k))
					 && (GET_INVIS_LEV(item_owner(k)) >
					     GET_LEVEL(ch)))) {
					found = 1;
					print_object_location(++num, k, ch,
							      TRUE, buf);
					if (strlen(buf) > 32500) {
						sprintf(buf + strlen(buf),
							"Buffer limit exceeded, you need to refine your search\r\n");
						break;
					}
				}
			if (!found)
				strcat(buf, " None\r\n");

		}
	}

	if (!found && arg && *arg && starts_with("graffiti", arg)) {
		perform_graffiti_where(ch);
	} else {
		if (ch->desc) {
			page_string(ch->desc, buf, TRUE, "");
		}
	}
	release_buffer(buf);

}

extern int port;

ACMD(do_where)
{
	char *arg = get_buffer(MAX_INPUT_LENGTH);
	one_argument(argument, arg);

	if (GET_LEVEL(ch) < LVL_IMMORT
	    || (port == 9000 && GET_LEVEL(ch) < IMM_WHERE_LEVEL))
		perform_mortal_where(ch, arg);
	else
		perform_immort_where(ch, arg);
	release_buffer(arg);
}

/* 4/30/2025 Nomikos - Added ability for mortals to list zones based on criteria */
ACMD(do_zinfo)
{
	/* check these three things so that techno-mudders can't abuse it */
	if (AFF_FLAGGED(ch, AFF_BLIND)) {
		send_to_char(ch, "You can't see a damned thing, you're blind!\r\n");
		return;
	}
	if (IS_DARK(IN_ROOM(ch)) && !CAN_SEE_IN_DARK(ch)) {
		send_to_char(ch, "It is pitch black...\r\n");
		return;
	}

	char* whatkind = get_buffer(MAX_INPUT_LENGTH);
	char* searchstring = get_buffer(MAX_INPUT_LENGTH);

	two_arguments(argument, whatkind, searchstring);

	if (strlen(whatkind) == 0) {
		send_to_char(ch, "Zone: %s\r\n", zone_table[world[IN_ROOM(ch)].zone].name);
		send_to_char(ch, " Author:    %s\r\n", zone_table[world[IN_ROOM(ch)].zone].author);
		send_to_char(ch, " Editor:    %s\r\n", zone_table[world[IN_ROOM(ch)].zone].editor);
		send_to_char(ch, " Levels:    %s\r\n", zone_table[world[IN_ROOM(ch)].zone].levels);
		send_to_char(ch, " Source:    %s\r\n", zone_source[(int)zone_table[world[IN_ROOM(ch)].zone].source]);
		send_to_char(ch, " Continent: %s\r\n", zone_continent[zone_table[world[IN_ROOM(ch)].zone].continent][0]);

		release_buffer(whatkind);
		release_buffer(searchstring);
		return;
	} 

	int target = 0;
	if (is_abbrev(whatkind, "zones"))
		target = 1;
	else if (is_abbrev(whatkind, "levels"))
		target = 2;
	else if (is_abbrev(whatkind, "continent"))
		target = 3;
	else if (is_abbrev(whatkind, "author"))
		target = 4;
	
	if (target == 0 || strlen(searchstring) == 0) {
		send_to_char(ch, "Usage:\r\n\r\n  zinfo\r\n   - or -\r\n"
		                 "  zinfo {zones | author | levels | continent} <target>\r\n");

		release_buffer(whatkind);
		release_buffer(searchstring);
		return;
	}

	char* buf = get_buffer(32750);
	sprintf(buf, "#   Zone                           Author     Levels     Continent\r\n");

	int found = 0;
	for (zone_rnum ii=0; ii<=top_of_zone_table; ii++) {
		char* subject = NULL;

		switch (target) {
			case 1: //zones
				subject = strdup(zone_table[ii].name);
				break;
			case 2: //levels
				subject = strdup(zone_table[ii].levels);
				/* Strip out the hyphen */
				for (int jj = 0; subject[jj] != '\0'; jj++)
					if (subject[jj] == '-')
						subject[jj] = ' ';
				break;
			case 3: //continents
				subject = strdup(zone_continent[zone_table[ii].continent][0]);
				break;
			case 4: //author
				subject = strdup(zone_table[ii].author);
				break;
		}

		if (!isname(searchstring, subject)) {
			free(subject);
			continue;
		}

		if (strlen(buf) > 32500) {
			sprintf(buf + strlen(buf), "Buffer limit exceeded, you need to refine your search\r\n");
			free(subject);
			break;
		}

		sprintf(buf + strlen(buf), "%-3d %-30.30s&n %-10.10s %-10.10s %-20.20s\r\n",
				++found,
				zone_table[ii].name,
				zone_table[ii].author,
				zone_table[ii].levels,
				zone_continent[zone_table[ii].continent][0]);

		free(subject);
	}

	if (found == 0)
		sprintf(buf + strlen(buf), "Please refine your search.\r\n");

	page_string(ch->desc, buf, TRUE, "");

	release_buffer(searchstring);
	release_buffer(whatkind);
	release_buffer(buf);
}

ACMD(do_levels)
{
	int i;
	char *buf;

	if (IS_NPC(ch)) {
		send_to_char(ch, "You ain't nothin' but a hound-dog.\r\n");
		return;
	}

	buf = get_buffer(MAX_STRING_LENGTH);

	for (i = 1; i < LVL_IMMORT; i++) {
		sprintf(buf + strlen(buf), "[%2d] %8ld", i,
			(long)GET_EXP_FOR_LEVEL(GET_RACE(ch), GET_CLASS(ch), i,
						REMORT_LEVEL(ch)));

		strcat(buf, "\r\n");
	}
	if (ch->desc)
		page_string(ch->desc, buf, TRUE, "");
	release_buffer(buf);
}

ACMD(do_consider)
{
	struct char_data *victim;
	signed int diff;
	char *buf = get_buffer(MAX_INPUT_LENGTH);

	one_argument(argument, buf);

	if (!(victim = get_char_vis(ch, buf, FIND_CHAR_ROOM))) {
		send_to_char(ch, "Consider killing who?\r\n");
		release_buffer(buf);
		return;
	}
	release_buffer(buf);
	if (victim == ch) {
		send_to_char(ch, "Wouldn't deleting be easier??\r\n");
		return;
	} else if (!IS_NPC(victim)) {
		send_to_char(ch,
			     "Would you like to borrow a cross and a shovel?\r\n");
		return;
	}
	diff = (GET_LEVEL(victim) - GET_LEVEL(ch));
	if (diff <= -16)
		send_to_char(ch,
			     "Levels comparison: You should kick their BUTT!.\r\n");
	else if (diff <= -11)
		send_to_char(ch,
			     "Levels comparison: You are quite a bit higher.\r\n");
	else if (diff <= -6)
		send_to_char(ch,
			     "Levels comparison: You are a bit higher.\r\n");
	else if (diff <= -1)
		send_to_char(ch,
			     "Levels comparison: You are slightly higher.\r\n");
	else if (diff == 0)
		send_to_char(ch, "Levels comparison: The perfect match!\r\n");
	else if (diff <= 5)
		send_to_char(ch,
			     "Levels comparison: They are slightly higher.\r\n");
	else if (diff <= 10)
		send_to_char(ch,
			     "Levels comparison: They are quite a bit higher.\r\n");
	else if (diff <= 15)
		send_to_char(ch,
			     "Levels comparison: Don't even think about it!\r\n");
	else if (diff <= 20)
		send_to_char(ch,
			     "Levels comparison: You have a good chance of death!\r\n");
	else if (diff <= 25)
		send_to_char(ch,
			     "Levels comparison: You will probably die, Quickly!\r\n");
	else if (diff <= 30)
		send_to_char(ch,
			     "Levels comparison: You are insane, and basically dead already!\r\n");
	else
		send_to_char(ch, "Levels comparison: Death is inevitable\n\r");

	diff = (GET_HIT(victim) - GET_HIT(ch));
	if (diff <= -601)
		send_to_char(ch,
			     "Hit points comparison: You could kill them with a glance!\r\n");
	else if (diff <= -251)
		send_to_char(ch,
			     "Hit points comparison: You don't have to anything to fear!\r\n");
	else if (diff <= -51)
		send_to_char(ch,
			     "Hit points comparison: You don't have much to fear!\r\n");
	else if (diff <= -26)
		send_to_char(ch,
			     "Hit points comparison: You are quite a bit healthier.\r\n");
	else if (diff <= -11)
		send_to_char(ch,
			     "Hit points comparison: You are slightly healthier.\r\n");
	else if (diff <= -6)
		send_to_char(ch,
			     "Hit points comparison: You are just a bit healthier.\r\n");
	else if (diff <= -1)
		send_to_char(ch,
			     "Hit points comparison: Its gonna be close!\r\n");
	else if (diff == 0)
		send_to_char(ch, "Hit points comparison: A perfect match!\r\n");
	else if (diff == 5)
		send_to_char(ch,
			     "Hit points comparison: Its gonna be close!\r\n");
	else if (diff == 10)
		send_to_char(ch,
			     "Hit points comparison: They are just a bit healthier.\r\n");
	else if (diff == 15)
		send_to_char(ch,
			     "Hit points comparison: They are slighty healthier.\r\n");
	else if (diff == 20)
		send_to_char(ch,
			     "Hit points comparison: They are quite a bit healthier.\r\n");
	else if (diff <= 50)
		send_to_char(ch,
			     "Hit points comparison: They are probably gonna hurt you bad!\r\n");
	else if (diff <= 100)
		send_to_char(ch,
			     "Hit points comparison: You will probably die a painfull death!\r\n");
	else if (diff <= 300)
		send_to_char(ch,
			     "Hit points comparison: If it takes a beating to teach you...so be it!\r\n");
	else if (diff <= 500)
		send_to_char(ch,
			     "Hit points comparison: All that will be left of you after the fight is a bloody spot!\r\n");
	else if (diff <= 700)
		send_to_char(ch,
			     "Hit points comparison: You see your life pass before your eyes!\r\n");
	else if (diff <= 1000)
		send_to_char(ch,
			     "Hit points comparison: You are insane and will most likely die!\r\n");
	else if (diff <= 2000)
		send_to_char(ch,
			     "Hit points comparison: You are so insane you should leave right now!\r\n");
	else if (diff <= 3000)
		send_to_char(ch,
			     "Hit points comparison: You are going to have a very short mudding career!\r\n");
	else if (diff <= 4000)
		send_to_char(ch,
			     "Hit points comparison: You better just commit suicide right now!\r\n");
	else if (diff <= 5000)
		send_to_char(ch,
			     "Hit points comparison: Death is waiting for you, don't dawdle!\r\n");
	else
		send_to_char(ch,
			     "Hit points comparison: Death is all I can say.\n\r");
}

ACMD(do_diagnose)
{
	struct char_data *vict;
	char *buf = get_buffer(MAX_INPUT_LENGTH);

	one_argument(argument, buf);

	if (*buf) {
		if (!(vict = get_char_vis(ch, buf, FIND_CHAR_ROOM))) {
			send_to_char(ch, "%s", NOPERSON);
		} else
			diag_char_to_char(vict, ch);
	} else {
		if (FIGHTING(ch))
			diag_char_to_char(FIGHTING(ch), ch);
		else
			send_to_char(ch, "Diagnose who?\r\n");
	}
	release_buffer(buf);
}

static char *ctypes[] = {
	"off", "sparse", "normal", "complete", "\n"
};

ACMD(do_color)
{
	int tp;
	char *arg;

	if (IS_NPC(ch))
		return;

	arg = get_buffer(MAX_INPUT_LENGTH);
	one_argument(argument, arg);

	if (!*arg) {
		send_to_char(ch, "Your current color level is %s.\r\n",
			     ctypes[COLOR_LEV(ch)]);
		release_buffer(arg);
		return;
	}
	if (((tp = search_block(arg, ctypes, FALSE)) == -1)) {
		send_to_char(ch,
			     "Usage: color { Off | Sparse | Normal | Complete }\r\n");
		release_buffer(arg);
		return;
	}
	REMOVE_BIT(PRF_FLAGS(ch), PRF_COLOR_1 | PRF_COLOR_2);
	SET_BIT(PRF_FLAGS(ch),
		(PRF_COLOR_1 * (tp & 1)) | (PRF_COLOR_2 * (tp & 2) >> 1));

	send_to_char(ch, "Your %scolor%s is now %s.\r\n", CCRED(ch, C_SPR),
		     CCNRM(ch, C_OFF), ctypes[tp]);
	release_buffer(arg);

}

ACMD(do_toggle)
{
	char *buf2;

	if (IS_NPC(ch))
		return;
	buf2 = get_buffer(64);

	if (GET_LEVEL(ch) >= LVL_IMMORT) {
		send_to_char(ch,
			     "      No Hassle: %-3s    "
			     "      Holylight: %-3s    "
			     "     Room Flags: %-3s\r\n",
			     ONOFF(PRF_FLAGGED(ch, PRF_NOHASSLE)),
			     ONOFF(PRF_FLAGGED(ch, PRF_HOLYLIGHT)),
			     ONOFF(PRF_FLAGGED(ch, PRF_ROOMFLAGS))
		    );
	}

	if (GET_WIMP_LEV(ch) == 0)
		strcpy(buf2, "OFF");
	else
		sprintf(buf2, "%-3d", GET_WIMP_LEV(ch));

	send_to_char(ch,
		     "Hit Pnt Display: %-3s    "
		     "     Brief Mode: %-3s    "
		     " Summon Protect: %-3s\r\n"
		     " Move Pnt Disp.: %-3s    "
		     "   Compact Mode: %-3s    "
		     "       On Quest: %-3s\r\n"
		     " Energy Display: %-3s    "
		     "         NoTell: %-3s    "
		     "   Repeat Comm.: %-3s\r\n"
		     "Auction Channel: %-3s    "
		     "  Grats Channel: %-3s    "
		     " Gossip Channel: %-3s\r\n"
		     "           Deaf: %-3s    "
		     "     Wimp Level: %-3s    "
		     " Auto Show Exit: %-3s\r\n"
		     "  Auto Loot All: %-3s    "
		     " Auto Loot Gold: %-3s    "
		     "Auto Split Gold: %-3s\r\n"
		     "    Auto Assist: %-3s    "
		     " Auto Sacrifice: %-3s    "
		     "  Music Channel: %-3s\r\n"
		     "    Color Level: %s      "
		     "   Can Be Paged: %s\r\n",
		     ONOFF(PRF_FLAGGED(ch, PRF_DISPHP)),
		     ONOFF(PRF_FLAGGED(ch, PRF_BRIEF)),
		     ONOFF(!PRF_FLAGGED(ch, PRF_SUMMONABLE)),
		     ONOFF(PRF_FLAGGED(ch, PRF_DISPMOVE)),
		     ONOFF(PRF_FLAGGED(ch, PRF_COMPACT)),
		     YESNO(PRF_FLAGGED(ch, PRF_QUEST)),
		     ONOFF(PRF_FLAGGED(ch, PRF_DISPMANA)),
		     ONOFF(PRF_FLAGGED(ch, PRF_NOTELL)),
		     YESNO(!PRF_FLAGGED(ch, PRF_NOREPEAT)),
		     ONOFF(!PRF_FLAGGED(ch, PRF_NOAUCT)),
		     ONOFF(!PRF_FLAGGED(ch, PRF_NOGRATZ)),
		     ONOFF(!PRF_FLAGGED(ch, PRF_NOGOSS)),
		     YESNO(PRF_FLAGGED(ch, PRF_DEAF)),
		     buf2,
		     ONOFF(PRF_FLAGGED(ch, PRF_AUTOEXIT)),
		     ONOFF(PRF_FLAGGED(ch, PRF_AUTOLOOT)),
		     ONOFF(PRF_FLAGGED(ch, PRF_AUTOGOLD)),
		     ONOFF(PRF_FLAGGED(ch, PRF_AUTOSPLIT)),
		     ONOFF(PRF_FLAGGED(ch, PRF_AUTOASSIST)),
		     ONOFF(PRF_FLAGGED(ch, PRF_AUTOSAC)),
		     ONOFF(!PRF2_FLAGGED(ch, PRF2_NOMUSIC)),
		     ctypes[COLOR_LEV(ch)],
		     YESNO(PRF2_FLAGGED(ch, PRF2_PAGE_OK))
	    );

	release_buffer(buf2);
}

struct sort_struct {
	int sort_pos;
	byte is_social;
} *cmd_sort_info = NULL;

int num_of_cmds;

void sort_commands(void)
{
	int a, b, tmp;

	num_of_cmds = 0;

	/*
	 * first, count commands (num_of_commands is actually one greater than the
	 * number of commands; it inclues the '\n'.
	 */
	while (*cmd_info[num_of_cmds].command != '\n')
		num_of_cmds++;

	/* create data array */
	CREATE(cmd_sort_info, struct sort_struct, num_of_cmds);

	/* initialize it */
	for (a = 1; a < num_of_cmds; a++) {
		cmd_sort_info[a].sort_pos = a;
		cmd_sort_info[a].is_social =
		    (cmd_info[a].command_pointer == do_action);
	}

	/* the infernal special case */
	cmd_sort_info[find_command("insult")].is_social = TRUE;

	/* Sort.  'a' starts at 1, not 0, to remove 'RESERVED' */
	for (a = 1; a < num_of_cmds - 1; a++)
		for (b = a + 1; b < num_of_cmds; b++)
			if (strcmp(cmd_info[cmd_sort_info[a].sort_pos].command,
				   cmd_info[cmd_sort_info[b].sort_pos].
				   command) > 0) {
				tmp = cmd_sort_info[a].sort_pos;
				cmd_sort_info[a].sort_pos =
				    cmd_sort_info[b].sort_pos;
				cmd_sort_info[b].sort_pos = tmp;
			}
}

ACMD(do_commands)
{
	int no, i, cmd_num;
	int wizhelp = 0, socials = 0;
	struct char_data *vict;
	char *buf, *arg = get_buffer(MAX_INPUT_LENGTH);

	one_argument(argument, arg);

	if (*arg) {
		if (!(vict = get_char_vis(ch, arg, FIND_CHAR_WORLD))
		    || IS_NPC(vict)) {
			send_to_char(ch, "Who is that?\r\n");
			release_buffer(arg);
			return;
		}
		if (GET_LEVEL(ch) < GET_LEVEL(vict)) {
			send_to_char(ch,
				     "You can't see the commands of people above your level.\r\n");
			release_buffer(arg);
			return;
		}
	} else
		vict = ch;

	if (subcmd == SCMD_SOCIALS)
		socials = 1;
	else if (subcmd == SCMD_WIZHELP)
		wizhelp = 1;

	buf = get_buffer(MAX_STRING_LENGTH);
	sprintf(buf, "The following %s%s are available to %s:\r\n",
		wizhelp ? "privileged " : "",
		socials ? "socials" : "commands",
		vict == ch ? "you" : GET_NAME(vict));

	/* cmd_num starts at 1, not 0, to remove 'RESERVED' */
	for (no = 1, cmd_num = 1; cmd_num < num_of_cmds; cmd_num++) {
		i = cmd_sort_info[cmd_num].sort_pos;
		if (cmd_info[i].minimum_level >= 0 &&
		    GET_LEVEL(vict) >= cmd_info[i].minimum_level &&
		    (cmd_info[i].minimum_level >= LVL_IMMORT) == wizhelp &&
		    (wizhelp || socials == cmd_sort_info[i].is_social)) {
			sprintf(buf + strlen(buf), "%-11s",
				cmd_info[i].command);
			if (!(no % 7))
				strcat(buf, "\r\n");
			no++;
		}
	}

	send_to_char(ch, "%s\r\n", buf);
	release_buffer(buf);
	release_buffer(arg);

}

/*
 * -naj 12/15/96  Infobar enhanced and revised for the new Phoenix MUD.
 *
 * -naj 8/30/95 mod: infobar (everything below here is all infobar related)
 */

/*
 * Produces strings for hit and damage bonuses.
 */
char *bonstr(int x)
{
	if ((x >= -20) && (x <= -15))
		return "Truly blows";
	else if ((x >= -14) && (x <= -10))
		return "Sucks";
	else if ((x >= -9) && (x <= -5))
		return "Very bad";
	else if ((x >= -4) && (x <= -2))
		return "Bad";
	else if ((x >= -1) && (x <= 1))
		return "Not much of one";
	else if ((x >= 2) && (x <= 4))
		return "Sorta Okay";
	else if ((x >= 5) && (x <= 8))
		return "Good";
	else if ((x >= 9) && (x <= 12))
		return "Very good";
	else if ((x >= 13) && (x <= 16))
		return "Extremely good";
	else if ((x >= 17) && (x <= 20))
		return "Kick Butt";
	else if ((x >= 21) && (x <= 24))
		return "Stunning";
	else if ((x >= 25) && (x <= 28))
		return "Awesome";
	else if ((x >= 29) && (x <= 32))
		return "Amazing";
	else if ((x >= 33) && (x <= 50))
		return "Unbelievable";
	else if ((x >= 51) && (x <= 60))
		return "Astounding!";
	else
		return "I dunno! *BUG*";
}

/*
 * Produces strings for AC.
 * We should probably change these for Phoenix.
 */
char *acstr(int x)
{
	int armor_num = 400;
	armor_num = (int)(10 * ((float)(200 + x) / (float)armor_num)) + 1;
	armor_num = MIN(11, MAX(-1, armor_num));
	return armor_types[armor_num];
}

/*
 * Produces a string describing your present hunger, thirst, and sobriety,
 * or lack thereof. =)
 */
void hungerstr(int f, int t, int d, char *s)
{
	char full[40], thirst[40], drunk[40];

	if ((f >= 0) && (f <= 4))
		strcpy(full, "You are starving, ");
	else if ((f >= 5) && (f <= 8))
		strcpy(full, "You are very hungry, ");
	else if ((f >= 9) && (f <= 12))
		strcpy(full, "You are hungry, ");
	else if ((f >= 13) && (f <= 16))
		strcpy(full, "You are a bit hungry, ");
	else if ((f >= 17) && (f <= 20))
		strcpy(full, "You are not hungry, ");
	else if ((f >= 21) && (f <= 24))
		strcpy(full, "You are well-fed, ");
	else if (f == -1)
		strcpy(full, "You are never hungry, ");
	else
		strcpy(full, "You have a hunger-*BUG*!, ");

	if ((t >= 0) && (t <= 4))
		strcpy(thirst, "dehydrated, ");
	else if ((t >= 5) && (t <= 8))
		strcpy(thirst, "very thirsty, ");
	else if ((t >= 9) && (t <= 12))
		strcpy(thirst, "thirsty, ");
	else if ((t >= 13) && (t <= 16))
		strcpy(thirst, "parched, ");
	else if ((t >= 17) && (t <= 20))
		strcpy(thirst, "not thirsty, ");
	else if ((t >= 21) && (t <= 24))
		strcpy(thirst, "well-watered, ");
	else if (t == -1)
		strcpy(thirst, "never thirsty, ");
	else
		strcpy(thirst, "water-*BUG*, ");

	if ((d >= 0) && (d <= 4))
		strcpy(drunk, "and sober.");
	else if ((d >= 5) && (d <= 8))
		strcpy(drunk, "and buzzing.");
	else if ((d >= 9) && (d <= 12))
		strcpy(drunk, "and very tipsie.");
	else if ((d >= 13) && (d <= 16))
		strcpy(drunk, "and intoxicated.");
	else if ((d >= 17) && (d <= 20))
		strcpy(drunk, "and drunk.");
	else if ((d >= 21) && (d <= 24))
		strcpy(drunk, "and piss drunk.");
	else if (d == -1)
		strcpy(drunk, "and always sober.");
	else
		strcpy(drunk, "and have a drinking-*BUG*!");

	sprintf(s, "%s%s%s", full, thirst, drunk);

}

/*
 * Produces a string describing alignment.
 */
char *naturestr(int x)
{
	if ((x <= -851) && (x >= -1000))
		return "Satanic";
	else if ((x <= -651) && (x >= -850))
		return "Truly evil";
	else if ((x <= -451) && (x >= -650))
		return "Very bad";
	else if ((x <= -351) && (x >= -450))
		return "Bad";
	else if ((x <= -101) && (x >= -350))
		return "Neutral Bad";
	else if ((x <= 100) && (x >= -100))
		return "Balanced";
	else if ((x <= 350) && (x >= 101))
		return "Neutral Good";
	else if ((x <= 450) && (x >= 351))
		return "Good";
	else if ((x <= 650) && (x >= 451))
		return "Very good";
	else if ((x <= 850) && (x >= 651))
		return "Saintly";
	else if ((x <= 1000) && (x >= 851))
		return "Angelic";
	else
		return "Unknown, as in *BUG!*";
}

/*
 *  Stuff to do with screen sizing.
 */

#define SIZE (ch->player_specials->saved.screensize)
char specbuf[200];

char *scrpos(int y, int x, struct char_data *ch)
{
	sprintf(specbuf, "\e[%d;%dH", y + SIZE - 24, x);
	return specbuf;
}

char *scrol(int y, int x, struct char_data *ch)
{
	sprintf(specbuf, "\e8\e[%d;%dH", y + SIZE - 24, x);
	return specbuf;
}

char *region(int y1, int y2, struct char_data *ch)
{
	sprintf(specbuf, "\e[?7h\e[%d;%dr", y1, y2 + SIZE - 24);
	return specbuf;
}

/* definitions for cursor locations and colors */

#define BORDER_COLOR    CCWHT(ch, C_NRM)
#define TAG_COLOR    CCCYN(ch, C_NRM)
#define VALUE_COLOR    CCGRN(ch, C_NRM)
#define NORMAL_COLOR    CCNRM(ch, C_NRM)

#define OUTPUT_LINE    scrol(19,1,ch)
#define INPUT_LINE    scrpos(24,1,ch)

#define ONE_BAR_SCROLL_REGION   region(1,19,ch)
#define TWO_BAR_SCROLL_REGION   region(7,19,ch)
#define FULL_SCROLL_REGION   region(1,24,ch)

#define CLEAR_SCREEN    "\033[H\033[J"

#define CURSOR_POS_SAVE    "\e7"

/* bottom bar tags */

#define HIT_TAG_POS    scrpos(21,3,ch)
#define HIT_BRACKET_TAG_POS   scrpos(21,12,ch)
#define END_TAG_POS    scrpos(21,19,ch)
#define END_BRACKET_TAG_POS   scrpos(21,29,ch)
#define MANA_TAG_POS    scrpos(21,36,ch)
#define MANA_BRACKET_TAG_POS   scrpos(21,48,ch)
#define GOLD_TAG_POS    scrpos(21,48,ch)
#define EXP_TAG_POS    scrpos(21,62,ch)
#define EXITS_TAG_POS    scrpos(22,3,ch)
#define EXITS_BRACKET_TAG_POS   scrpos(22,10,ch)
#define ROOMDESC_TAG_POS   scrpos(22,20,ch)
#define ROOMDESC_BRACKET_TAG_POS  scrpos(22,75,ch)

/* bottom values */

#define HIT_VAL_POS    scrpos(21,8,ch)
#define MAX_HIT_VAL_POS    scrpos(21,13,ch)
#define END_VAL_POS    scrpos(21,25,ch)
#define MAX_END_VAL_POS    scrpos(21,30,ch)
#define MANA_VAL_POS    scrpos(21,44,ch)
#define MAX_MANA_VAL_POS   scrpos(21,49,ch)
#define GOLD_VAL_POS    scrpos(21,54,ch)
#define EXP_VAL_POS    scrpos(21,67,ch)
#define EXITS_VAL_POS    scrpos(22,11,ch)
#define ROOMDESC_VAL_POS   scrpos(22,30,ch)

/* top tags */

#define ABILITY_TAG_POS    "\e[1;4H"
#define COMBAT_TAG_POS    "\e[2;1H"
#define ARMOR_TAG_POS    "\e[3;1H"
#define RACE_TAG_POS    "\e[4;1H"
#define CLASS_TAG_POS    "\e[4;25H"
#define HITBON_TAG_POS    "\e[2;49H"
#define DAMBON_TAG_POS    "\e[3;49H"
#define CARRYING_TAG_POS   "\e[4;49H"
#define BANK_TAG_POS    "\e[3;25H"

/* top values */

#define STR_VAL_POS    "\e[1;9H"
#define DEX_VAL_POS    "\e[1;18H"
#define INT_VAL_POS    "\e[1;27H"
#define WIS_VAL_POS    "\e[1;36H"
#define CON_VAL_POS    "\e[1;45H"
#define CHA_VAL_POS    "\e[1;54H"
#define PRA_VAL_POS    "\e[1;63H"
#define LEV_VAL_POS    "\e[1;74H"
#define COMBAT_VAL_POS    "\e[2;9H"
#define ARMOR_VAL_POS    "\e[3;12H"
#define RACE_VAL_POS    "\e[4;7H"
#define CLASS_VAL_POS    "\e[4;32H"
#define HUNGER_THIRST_VAL_POS   "\e[5;1H"
#define HITBON_VAL_POS    "\e[2;60H"
#define DAMBON_VAL_POS    "\e[3;60H"
#define CARRYING_VAL_POS   "\e[4;60H"
#define BANK_VAL_POS    "\e[3;31H"

#define CHECK_SCORE   if (!PRF_FLAGGED(ch, PRF_SCOREBAR)) break

#define IBCH(field)  (ch->infobar.(field))

/*
 *  This is the big-daddy himself.
 *  Revised 8/31/95 - Xaggo
 */
ACMD(do_infobar)
{
	int n;
	char *arg;

	if (!ch) {
		log("SYSERR: do_infobar: null character passed");
		return;
	}
	if (IS_NPC(ch)) {
		return;
	}
	if (!ch->desc) {
		log("SYSERR: do_infobar: character has no descriptor");
		return;
	}

	if (subcmd != SCMDB_RESIZE)
		return;
	arg = get_buffer(MAX_INPUT_LENGTH);

	one_argument(argument, arg);
	if (!*arg) {
		send_to_char(ch, "Current screen size is %d.\r\n",
			     ch->player_specials->saved.screensize);
		release_buffer(arg);
		return;
	}
	n = atoi(arg);
	if (n < 13)
		n = 13;
	else if (n >= 90)	/* nomi(temp) 90 or above crashes buffer on show/list */
		n = 89;
	ch->player_specials->saved.screensize = n;
	do_infobar(ch, 0, 0, SCMDB_REDRAW);
	if (!PRF_FLAGGED(ch, PRF_INFOBAR))
		send_to_char(ch,
			     "\e[1;%dr\r\n\033[H\033[J\r\n Display resized to %d lines per screen.\r\n",
			     n, n);
	else
		send_to_char(ch, "Display resized to %d lines per screen.\r\n",
			     n);
	release_buffer(arg);
}

/* BEGIN SCAN STUFF */

void list_scanned_chars(struct char_data *list, struct char_data *ch,
			int distance, int door)
{
	const char *how_far[] = {
		"close by",
		"a ways off",
		"far off to the"
	};
	int start;
	struct char_data *i;
	int count = 0;
	int invis_count = 0;

	/* this loop is a quick, easy way to help make a grammatical sentence
	   (i.e., "You see x, x, y, and z." with commas, "and", etc.) */

	for (i = list; i; i = i->next_in_room)
		/* put any other conditions for scanning someone in this if statement -
		   i.e., if (CAN_SEE(ch, i) && condition2 && condition3) or whatever */
	{
		if (CAN_SEE(ch, i))
			count++;
		if (IS_NPC(i) && !strcmp(i->player.long_descr, "INVIS\r\n"))
			invis_count++;
	}

	if (!count || ((invis_count == count) && (GET_LEVEL(ch) < LVL_IMMORT)))
		return;

	start = 0;
	for (i = list; i; i = i->next_in_room) {

		/* make sure to add changes to the if statement above to this one also, using
		 * or's to join them.. i.e.,
		 * if (!CAN_SEE(ch, i) || !condition2 || !condition3)
		 */
		if (!CAN_SEE(ch, i))
			continue;

		if (start == 0) {
			if (IS_NPC(i)
			    && !strcmp(i->player.long_descr, "INVIS\r\n")) {
				if (GET_LEVEL(ch) >= LVL_IMMORT) {
					send_to_char(ch, "You see (%s)",
						     GET_NAME(i));
					start = 1;
				}
			} else {
				send_to_char(ch, "You see %s", GET_NAME(i));
				start = 1;
			}
		} else {
			if (IS_NPC(i)
			    && !strcmp(i->player.long_descr, "INVIS\r\n")) {
				if (GET_LEVEL(ch) >= LVL_IMMORT)
					send_to_char(ch, "(%s)", GET_NAME(i));
			} else
				send_to_char(ch, "%s", GET_NAME(i));
		}
		count--;
		if (!(IS_NPC(i) && !strcmp(i->player.long_descr, "INVIS\r\n") &&
		      (GET_LEVEL(ch) < LVL_IMMORT))) {
			if (count > 1)
				send_to_char(ch, ", ");
			else if (count == 1)
				send_to_char(ch, " and ");
			else {
				send_to_char(ch, " %s %s.\r\n",
					     how_far[distance], dirs[door]);
			}
		}
	}
}

ACMD(do_scan)
{
	/* >scan
	 * You quickly scan the area.
	 * You see John, a large horse and Frank close by north.
	 * You see a small rabbit a ways off south.
	 * You see a huge dragon and a griffon far off to the west.
	 */
	int door;

	if (AFF_FLAGGED(ch, AFF_BLIND)) {
		send_to_char(ch,
			     "You can't see a damned thing, you're blind!\r\n");
		return;
	}

	/* may want to add more restrictions here, too */
	send_to_char(ch, "You quickly scan the area.\r\n");
	for (door = 0; door < NUM_OF_DIRS; door++)
		if (EXIT(ch, door) && EXIT(ch, door)->to_room != NOWHERE &&
		    !EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED)) {
			if (world[EXIT(ch, door)->to_room].people) {
				list_scanned_chars(world
						   [EXIT(ch, door)->to_room].
						   people, ch, 0, door);
			} else if (_2ND_EXIT(ch, door)
				   && _2ND_EXIT(ch, door)->to_room != NOWHERE
				   && !EXIT_FLAGGED(_2ND_EXIT(ch, door),
						    EX_CLOSED)) {
				/* check the second room away */
				if (world[_2ND_EXIT(ch, door)->to_room].people) {
					list_scanned_chars(world
							   [_2ND_EXIT
							    (ch,
							     door)->to_room].
							   people, ch, 1, door);
				} else if (_3RD_EXIT(ch, door)
					   && _3RD_EXIT(ch,
							door)->to_room !=
					   NOWHERE
					   && !EXIT_FLAGGED(_3RD_EXIT(ch, door),
							    EX_CLOSED)) {
					/* check the third room */
					if (world[_3RD_EXIT(ch, door)->to_room].
					    people) {
						list_scanned_chars(world
								   [_3RD_EXIT
								    (ch,
								     door)->
								    to_room].
								   people, ch,
								   2, door);
					}

				}
			}
		}
}

ACMD(do_spellhelp)
{
	/* Also Do Skill Help *chuckle* */
	int sortpos = 0;
	int i, pos;
	int class = -1;
	char *buf2 = get_buffer(32750);
	char *inpt = get_buffer(256);

	skip_spaces(&argument);
	one_argument(argument, inpt);
	if (*inpt) {
		/* find-the-class */
		class = parse_class(inpt);
		if (class == CLASS_UNDEFINED) {
			send_to_char(ch, "Invalid class name.\r\n");
		} else {	/* print the skills */

			sprintf(buf2, "%s for class %s: \r\n",
				subcmd == SCMD_SPELLS ? "Spells" : "Skills",
				pc_class_types[class]);
			pos = strlen(buf2);

			for (sortpos = 1; sortpos < MAX_SPELLS; sortpos++) {
				if ((spells[spell_sort_info[sortpos]].
				     is_spell == subcmd)
				    &&
				    ((spells[spell_sort_info[sortpos]].
				      min_level[class] < LVL_IMMORT)
				     || (class == CLASS_GOD))) {
					pos +=
					    sprintf(buf2 + pos,
						    "%-35.35s - Level: %d\r\n",
						    spells[spell_sort_info
							   [sortpos]].
						    spell_name,
						    spells[spell_sort_info
							   [sortpos]].
						    min_level[class]);
				}

			}
		}
	} else {
		strcpy(buf2,
		       "---------------------------------------------------------------------------|\r\n"
		       "Name           | Wa| Cl| Th| Mu| Ra| Bd| Mo| Ba| Pa| AP| Dr| Ke| As| Ne| De|\r\n"
		       "---------------------------------------------------------------------------|\r\n");

		pos = strlen(buf2);
		for (sortpos = 1; sortpos < MAX_SPELLS; sortpos++) {
			if (spells[spell_sort_info[sortpos]].is_spell == subcmd) {
				pos +=
				    sprintf(buf2 + pos, "%-15.15s|",
					    spells[spell_sort_info[sortpos]].
					    spell_name);
				for (i = 0; i < 17; i++) {
					if ((i == 12) || (i == 7))
						continue;
					if (spells[spell_sort_info[sortpos]].
					    min_level[i] < LVL_IMMORT)
						pos +=
						    sprintf(buf2 + pos, "%3d|",
							    spells
							    [spell_sort_info
							     [sortpos]].
							    min_level[i]);
					else {
						strcat(buf2, "---|");
						pos += 4;
					}
				}
				strcat(buf2, "\r\n");
				pos += 2;
			}
		}
		pos += sprintf(buf2 + pos,
			       "Type %s <class_name> to get all the %s for one class",
			       cmd_info[cmd].command,
			       subcmd == SCMD_SPELLS ? "spells" : "skills");
	}
	release_buffer(inpt);
	if (ch->desc)
		page_string(ch->desc, buf2, TRUE, "");
	release_buffer(buf2);
}

ACMD(do_item_count)
{
	struct char_data *vict;
	int i;
	long numitems = 0;
	room_rnum rnum;

	skip_spaces(&argument);
	if (strlen(argument) > 2) {
		if (GET_LEVEL(ch) > LVL_DGOD) {
			if (!
			    (vict =
			     get_char_vis(ch, argument, FIND_CHAR_WORLD)))
				send_to_char(ch,
					     "Your target isn't here, try show rent <person>\r\n");
			else {
				count_items(vict, vict->carrying, &numitems);

				for (i = 0; i < NUM_WEARS; i++)
					count_items(vict, GET_EQ(vict, i),
						    &numitems);
				send_to_char(ch,
					     "%s is carrying %ld items.\r\n",
					     GET_NAME(vict), numitems);
				for (i = 0; i < num_of_houses; i++) {
					if (str_cmp
					    (GET_NAME(vict),
					     house_control[i].owner) == 0) {
						if ((rnum =
						     real_room(house_control[i].
							       vnum)) == -1)
							numitems = -1;
						else {
							numitems = 0;
							Crash_count_items(world
									  [rnum].
									  contents,
									  &numitems);
						}
						send_to_char(ch,
							     "  [%7ld] %9ld items %s.\r\n",
							     house_control[i].
							     vnum, numitems,
							     world[rnum].name);
					}
				}
			}
		} else {
			send_to_char(ch,
				     "You can only count your own items!\r\n");
		}
	} else {
		count_items(ch, ch->carrying, &numitems);

		for (i = 0; i < NUM_WEARS; i++)
			count_items(ch, GET_EQ(ch, i), &numitems);
		send_to_char(ch, "You are carrying %ld items.\r\n", numitems);
		for (i = 0; i < num_of_houses; i++) {
			if (str_cmp(GET_NAME(ch), house_control[i].owner) == 0) {
				if ((rnum =
				     real_room(house_control[i].vnum)) == -1)
					numitems = -1;
				else {
					numitems = 0;
					Crash_count_items(world[rnum].contents,
							  &numitems);
				}
				send_to_char(ch,
					     "  You have %ld items in %s.\r\n",
					     numitems, world[rnum].name);

			}

		}
	}
}

ACMD(who_to_menu)
{
	struct descriptor_data *d;
	struct char_data *wch;
	char *buf = get_buffer(MAX_STRING_LENGTH);
	char *buf1 = get_buffer(MAX_STRING_LENGTH);
	char *arg = get_buffer(MAX_INPUT_LENGTH);
	char *Imm_buf = get_buffer(MAX_STRING_LENGTH);
	char *Mort_buf = get_buffer(MAX_STRING_LENGTH);
	int Wizards = 0, Mortals = 0;
	int low = 0, high = LVL_HIMPL;

	strcpy(Imm_buf, "Immortals currently in the Realm\r\n"
	       "--------------------------------\r\n");
	strcpy(Mort_buf, "Mortals currently in the Realm\r\n"
	       "------------------------------\r\n");

	for (d = descriptor_list; d; d = d->next) {
		if (STATE(d) != CON_PLAYING &&
		    (STATE(d) != CON_MEDIT) &&
		    (STATE(d) != CON_OEDIT) &&
		    (STATE(d) != CON_REDIT) &&
		    (STATE(d) != CON_SEDIT) &&
		    (STATE(d) != CON_TRIGEDIT) &&
		    (STATE(d) != CON_HEDIT) &&
		    (STATE(d) != CON_GEDIT) &&
		    (STATE(d) != CON_PEDIT) && (STATE(d) != CON_ZEDIT))
			continue;
		if (d->original)
			wch = d->original;
		else if (!(wch = d->character))
			continue;

		if (!CAN_SEE(ch, wch))
			continue;
		if (GET_LEVEL(wch) < low || GET_LEVEL(wch) > high)
			continue;
		if (GET_LEVEL(wch) >= LVL_IMMORT) {
			sprintf(Imm_buf + strlen(Imm_buf), "%s[%s] %s %s ",
				CCYEL(ch, C_SPR),
				WizLevels[GET_LEVEL(wch) - LVL_IMMORT],
				GET_NAME(wch), GET_TITLE(wch));
			Wizards++;
		} else {
			/* 10/27/96, Echo - format has been changed slightly to reflect
			 *   added levels, races, and classes.
			 */
			sprintf(Mort_buf + strlen(Mort_buf),
				"[%3d %2.2s-%3.3s] %s %s ", GET_LEVEL(wch),
				CLASS_ABBR(wch), RACE_ABBR(wch), GET_NAME(wch),
				GET_TITLE(wch));
			Mortals++;
		}

		*buf = '\0';
		if (GET_INVIS_LEV(wch))
			sprintf(buf, "(i%d)", GET_INVIS_LEV(wch));
		else if (AFF_FLAGGED(wch, AFF_INVISIBLE))
			strcat(buf, "(invis)");

		if (GET_CLAN(wch) > 0)
			sprintf(buf + strlen(buf), "(%s)", GET_CLAN_NAME(wch));
		if (GET_LEVEL(wch) == LVL_HERO)
			strcat(buf, "&k&3(HERO)&n");
		else if (GET_LEVEL(wch) == LVL_ANGEL)
			strcat(buf, "&k&2(ANGEL)&n");
		if (GET_LEVEL(wch) == LVL_AVATAR)
			strcat(buf, "&k&6(AVATAR)&n");

		if (PLR_FLAGGED(wch, PLR_MAILING))
			strcat(buf, "(mailing)");
		else if (PLR_FLAGGED(wch, PLR_WRITING) &&
			 (STATE(d) != CON_MEDIT) &&
			 (STATE(d) != CON_OEDIT) &&
			 (STATE(d) != CON_REDIT) &&
			 (STATE(d) != CON_SEDIT) &&
			 (STATE(d) != CON_GEDIT) &&
			 (STATE(d) != CON_PEDIT) &&
			 (STATE(d) != CON_TRIGEDIT) &&
			 (STATE(d) != CON_HEDIT) && (STATE(d) != CON_ZEDIT))
			strcat(buf, "(writing)");

		if (PRF_FLAGGED(wch, PRF_DEAF))
			strcat(buf, "(deaf)");
		if (PRF_FLAGGED(wch, PRF_NOTELL))
			strcat(buf, "(notell)");
		if (PRF_FLAGGED(wch, PRF_QUEST))
			strcat(buf, "(quest)");
		if (PLR_FLAGGED(wch, PLR_THIEF))
			strcat(buf, "(THIEF)");
		if (PLR_FLAGGED(wch, PLR_KILLER))
			strcat(buf, "(KILLER)");
		if (STATE(d) == CON_OEDIT)
			strcat(buf, "(OLC - OEdit)");
		else if (STATE(d) == CON_MEDIT)
			strcat(buf, "(OLC - MEdit)");
		else if (STATE(d) == CON_REDIT)
			strcat(buf, "(OLC - REdit)");
		else if (STATE(d) == CON_ZEDIT)
			strcat(buf, "(OLC - ZEdit)");
		else if (STATE(d) == CON_SEDIT)
			strcat(buf, "(OLC - SEdit)");
		else if (STATE(d) == CON_GEDIT)
			strcat(buf, "(OLC - GEdit)");
		else if (STATE(d) == CON_PEDIT)
			strcat(buf, "(OLC - PEdit)");
		else if (STATE(d) == CON_HEDIT)
			strcat(buf, "(OLC - HEdit)");
		else if (STATE(d) == CON_TRIGEDIT)
			strcat(buf, "(OLC - TrigEdit)");
		if (GET_LEVEL(wch) >= LVL_IMMORT)
			strcat(buf, CCNRM(ch, C_SPR));
		if (PRF2_FLAGGED(wch, PRF2_AFK))
			strcat(buf, "(AFK)");
		if (AFF_FLAGGED(wch, AFF_PLAGUE))
			strcat(buf, "&G(PLAGUE !!)&n");

		strcat(buf, "\r\n");

		if (GET_LEVEL(wch) >= LVL_IMMORT) {
			strcat(Imm_buf, buf);
		} else {
			strcat(Mort_buf, buf);
		}
	}			/* end of for */

	*buf = '\0';
	if (Wizards) {
		strcat(Imm_buf, "\r\n");
		strcpy(buf, Imm_buf);
	}

	if (Mortals) {
		strcat(Mort_buf, "\r\n");
		strcat(buf, Mort_buf);
	}
	release_buffer(Imm_buf);
	release_buffer(Mort_buf);

	if ((Wizards + Mortals) == 0)
		strcpy(buf1,
		       "No wizards or mortals are currently visible to you.\r\n");

	if (Wizards)
		sprintf(buf1, "There %s %d visible immortal%s%s",
			(Wizards == 1 ? "is" : "are"), Wizards,
			(Wizards == 1 ? "" : "s"),
			(Mortals ? " and there " : "."));
	if (Mortals) {
		sprintf(buf1 + strlen(buf1), "%s%s %d visible mortal%s.",
			(Wizards ? "" : "There "),
			(Mortals == 1 ? "is" : "are"), Mortals,
			(Mortals == 1 ? "" : "s"));
	}
	strcat(buf, buf1);

	strcat(buf, "\r\n");

	if ((Wizards + Mortals) > boot_high)
		boot_high = Wizards + Mortals;
	sprintf(buf + strlen(buf),
		"There is a boot time high of %d player%s.\r\n", boot_high,
		(boot_high == 1 ? "" : "s"));

	release_buffer(buf1);
	release_buffer(arg);
	send_to_char(ch, "%s", buf);
	release_buffer(buf);
}

/* Patched-up the gwho formatting for Grouped Players.
  Lined-up followers brackets with HEADs -*- Raolin 1995/11/09 */

#define GWHO_FORMAT \
"format: gwho"

ACMD(do_gwho)
{
	struct descriptor_data *d;
	struct char_data *tch;
	struct follow_type *f;
	char *name_search = get_buffer(128);
	short output;
	int num_can_see;
	send_to_char(ch, "Immortals Presently in the Realm\r\n"
		     "--------------------------------\r\n\r\n");
	output = FALSE;
	name_search[0] = '\0';
	num_can_see = 0;
	for (d = descriptor_list; d; d = d->next) {
		if (STATE(d) != CON_PLAYING)
			continue;
		if (d->original)
			tch = d->original;
		else if (!(tch = d->character))
			continue;

		if (*name_search && str_cmp(GET_NAME(tch), name_search) &&
		    !strstr(GET_TITLE(tch), name_search))
			continue;
		if (!CAN_SEE(ch, tch))
			continue;
		if (GET_LEVEL(tch) >= LVL_IMMORT) {
			output = TRUE;
			send_to_char(ch, "%s[%s] %s %s%s\r\n", CCYEL(ch, C_SPR),
				     WizLevels[GET_LEVEL(tch) - LVL_IMMORT],
				     GET_NAME(tch), GET_TITLE(tch), CCNRM(ch,
									  C_SPR));
		}		/* end of >=LVL_IMMORT */
	}			/* end of for */
	if (output == FALSE)
		send_to_char(ch, "      &Y(None).&n\r\n");
	output = FALSE;
	name_search[0] = '\0';
	num_can_see = 0;
	send_to_char(ch, "\r\nUnGrouped Players Presently in the Realm\r\n"
		     "----------------------------------------\r\n");
	for (d = descriptor_list; d; d = d->next) {
		if (STATE(d) != CON_PLAYING)
			continue;

		if (d->original)
			tch = d->original;
		else if (!(tch = d->character))
			continue;

		if (*name_search && str_cmp(GET_NAME(tch), name_search) &&
		    !strstr(GET_TITLE(tch), name_search))
			continue;
		if (!CAN_SEE(ch, tch))
			continue;
		if (GET_LEVEL(tch) < LVL_IMMORT) {
			/* not in group or group of one person or group with master linkless
			   or grouped only with NPC */

			if (tch->master == NULL && tch->followers == NULL) {
				output = TRUE;
				send_to_char(ch,
					     " [%3d][%-2.2s-%-4.4s] %-12.12s%s",
					     GET_LEVEL(tch), CLASS_ABBR(tch),
					     RACE_ABBR(tch), GET_NAME(tch),
					     ((!(++num_can_see %
						 3)) ? "\r\n" : ""));
			}

		}		/* end of if !immort */
	}			/* end of for */

	if (output == FALSE)
		send_to_char(ch, "      (None).\r\n");
	else if ((num_can_see % 3) != 0)
		send_to_char(ch, "\r\n");
	output = FALSE;
	name_search[0] = '\0';
	num_can_see = 0;
	send_to_char(ch, "\r\nGrouped Players Presently in the Realm\r\n"
		     "--------------------------------------\r\n");

	for (d = descriptor_list; d; d = d->next) {
		if (STATE(d) != CON_PLAYING)
			continue;

		if (d->original)
			tch = d->original;
		else if (!(tch = d->character))
			continue;

		if (*name_search && str_cmp(GET_NAME(tch), name_search) &&
		    !strstr(GET_TITLE(tch), name_search))
			continue;
		if (!CAN_SEE(ch, tch))
			continue;
		if (GET_LEVEL(tch) < LVL_IMMORT) {
	 /*** NEED TO MAKE SURE THAT WE DO NOT PUT PLAYERS WHO ARE GROUPED ***
         *** ONLY WITH LINKLESS OR NPC FOLLOWERS                          ***/

			if ((tch->master == NULL) && (tch->followers != NULL)) {
				output = TRUE;
				send_to_char(ch,
					     "&CHEAD&n: [%3d][%-2.2s-%-4.4s] %-12.12s %s\r\n",
					     GET_LEVEL(tch), CLASS_ABBR(tch),
					     RACE_ABBR(tch), GET_NAME(tch),
					     AFF_FLAGGED(tch,
							 AFF_GROUP) ? "(G)" :
					     "   ");
				num_can_see = 0;
				for (f = tch->followers; f; f = f->next) {
					if (CAN_SEE(ch, f->follower)
					    && GET_LEVEL(f->follower) <
					    LVL_IMMORT) {
						send_to_char(ch,
							     "[%3d][%-2.2s-%-3.3s]%-12.12s%s %s",
							     GET_LEVEL(f->
								       follower),
							     CLASS_ABBR(f->
									follower),
							     RACE_ABBR(f->
								       follower),
							     GET_NAME(f->
								      follower),
							     ((!(++num_can_see %
								 3)) ? "\r\n" :
							      ""),
							     AFF_FLAGGED(tch,
									 AFF_GROUP)
							     ? "(G)" : "   ");
					}
				}
				if ((num_can_see % 3) != 0)
					send_to_char(ch, "\r\n");
			}
		}
	}

	if (output == FALSE)
		send_to_char(ch, "      (None).\r\n");
	release_buffer(name_search);
}

ACMD(check_invalid_remorts)
{
	struct char_data *victim;
	struct char_file_u tmp_store;
	int i, j;

	send_to_char(ch, "Checking for invalid remort levels...\r\n");
	for (i = 0, j = 1; i <= top_of_p_table; i++, j++) {
		send_to_char(ch, ".");

		CREATE(victim, struct char_data, 1);
		clear_char(victim);
		if (load_char((player_table + i)->name, &tmp_store) > -1) {
			store_to_char(&tmp_store, victim);
			char_to_room(victim, 0);

			if (GET_LEVEL(victim) < LVL_IMMORT) {
				if (IS_DBLREMORT_OLD(victim)
				    && (REMORT_LEVEL(victim) != 2)) {
					send_to_char(ch,
						     "\r\n--> %s should have a remort level of 2.\r\n",
						     GET_NAME(victim));
					j = 1;
				} else if (!IS_DBLREMORT_OLD(victim)
					   && IS_REMORT_OLD(victim)
					   && (REMORT_LEVEL(victim) != 1)) {
					send_to_char(ch,
						     "\r\n--> %s should have a remort level of 1.\r\n",
						     GET_NAME(victim));
					j = 1;
				} else if (!IS_DBLREMORT_OLD(victim)
					   && !IS_REMORT_OLD(victim)
					   && (REMORT_LEVEL(victim) == 2)) {
					send_to_char(ch,
						     "\r\n--> %s should not be a double remort.\r\n",
						     GET_NAME(victim));
					j = 1;
				} else if (!IS_REMORT_OLD(victim)
					   && (REMORT_LEVEL(victim) == 1)) {
					send_to_char(ch,
						     "\r\n--> %s should not be a remort.\r\n",
						     GET_NAME(victim));
					j = 1;
				} else if ((REMORT_LEVEL(victim) > 2)
					   || (REMORT_LEVEL(victim) < 0)) {
					send_to_char(ch,
						     "\r\n--> %s has an invalid remort level of %d.\r\n",
						     GET_NAME(victim),
						     REMORT_LEVEL(victim));
					j = 1;
				}

				/* else they are fine */
			}
			if (!(j % 24)) {
				send_to_char(ch, "\r\n");
				j = 1;
			}
			if (IN_ROOM(victim) != NOWHERE) {
				char_from_room(victim);
			}
			free_char(victim);
		} else {
			send_to_char(ch, "x");
			free(victim);
		}
	}
	send_to_char(ch, "\r\n...done checking for invalid remort levels.\r\n");
}

ACMD(do_lastincrease)
{
	int skill = GET_LAST_LEARN(ch);
	if (skill < 1 || skill >= MAX_SPELLS) {
		send_to_char(ch,
			     "You don't seem to have learned anything.\r\n");
	} else {
		send_to_char(ch, "You last improved your knowledge of %s.\r\n",
			     spells[skill].spell_name);
	}
}

/*

int ac_to_armor(int eq_pos)
{
  switch (eq_pos)
  {
  case WEAR_SHIELD:
  case WEAR_BODY:
    return 3;
  case WEAR_HEAD:
  case WEAR_LEGS:
    return 2;
  default:
    return 1;
  }
}

ACMD(do_project)
{
  const int PROJ_DAMROLL = 0;
  const int PROJ_HITROLL = 1;
  const int PROJ_DEX = 2;
  const int PROJ_STR = 3;
  const int PROJ_ARMOR = 4;
  const int multipliers[] = { 100, 90, 80, 70, 5};
  int i;

  if (argument) {
    skip_spaces(&argument);
  }
  if (!argument || !*argument) {
    send_to_char(ch, "Usage: project <object vnum>\r\n");
    return;
  }

  int vnum = atoi(argument);
  int rnum = real_object(vnum);
  if (rnum < 0) {
    send_to_char(ch, "There is no object with that number.\r\n");
    return;
  }

  struct obj_data *obj = read_object(rnum, REAL);
  send_to_char(ch, "The projected power points of #%ld (%s) are:\r\n", GET_OBJ_VNUM(obj), GET_OBJ_NAME(obj));

  int total = 0;
  for (i = 0; i < MAX_OBJ_AFFECT; i++) {
    int modifier = obj->affected[i].modifier;
    switch (obj->affected[i].location) {
    case APPLY_DAMROLL:
      total += multipliers[PROJ_DAMROLL] * modifier;
      send_to_char(ch, "  %3d damroll = %3d points\r\n", modifier, multipliers[PROJ_DAMROLL] * modifier);
      break;
    case APPLY_HITROLL:
      total += multipliers[PROJ_HITROLL] * modifier;
      send_to_char(ch, "  %3d hitroll = %3d points\r\n", modifier, multipliers[PROJ_HITROLL] * modifier);
      break;
    case APPLY_DEX:
      total += multipliers[PROJ_DEX] * modifier;
      send_to_char(ch, "  %3d dex     = %3d points\r\n", modifier, multipliers[PROJ_DEX] * modifier);
      break;
    case APPLY_STR:
      total += multipliers[PROJ_STR] * modifier;
      send_to_char(ch, "  %3d str     = %3d points\r\n", modifier, multipliers[PROJ_STR] * modifier);
      break;
    case APPLY_ARMOR:
      total -= multipliers[PROJ_ARMOR] * modifier;
      send_to_char(ch, "  %3d armor   = %3d points\r\n", modifier, multipliers[PROJ_ARMOR] * modifier);
      break;
    }
  }

  send_to_char(ch, "--------------------------------------------\r\n");
  send_to_char(ch, "  Total points: %d\r\n", total);

  extract_obj(obj);
}
*/
