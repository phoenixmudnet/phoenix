
/* ************************************************************************
*   File: interpreter.c                                 Part of CircleMUD *
*  Usage: parse user commands, search for specials, call ACMD functions   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define __INTERPRETER_C__

#include "../localHeader/conf.h"
#include "../localHeader/sysdep.h"

#include "structs.h"
#include "buffer.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "spells.h"
#include "handler.h"
#include "mail.h"
#include "screen.h"
#include "olc.h"
#include "queue.h"		/*  begin add - Bon 07/25/97 */
#include "constants.h"
#include "dg_scripts.h"

/* 10/08/96, Echo - removed reference to titles[][] array
 *                  added exp_multipliers[] and exp_table[]
 * extern const struct title_type titles[NUM_CLASSES][LVL_IMPL + 1];
 */
/* 10/27/96, Echo - following two arrays necessary here?
 * extern float exp_multipliers[NUM_CLASSES];
 * extern int exp_table[LVL_IMPL + 1];
 */
extern char last_command[MAX_STRING_LENGTH];
extern char *motd;
extern char *imotd;
extern char *background;
extern char *MENU;
extern char *WELC_MESSG;
extern char *START_MESSG;
extern char *NAME_POLICY;
extern struct char_data *character_list;
extern struct descriptor_data *descriptor_list;
extern struct player_index_element *player_table;
extern int top_of_p_table;
extern int circle_restrict;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct room_data *world;
extern int port;
extern room_vnum mortal_start_room;
extern room_rnum r_immort_start_room;
extern room_rnum r_frozen_start_room;
extern int max_bad_pws;
extern char *race_menu;
extern char *race_prompt;
extern char *class_menu_header;
extern char *class_menu_choices[NUM_CLASSES];
extern char *class_prompt;
extern int LEGAL_CLASS[NUM_RACES][NUM_CLASSES];
extern char *pc_race_types[];
extern char *pc_class_types[];
extern const long race_stats[NUM_RACES][6];
extern char *stat_menu;
extern int no_specials;
extern struct zone_data *zone_table;	/*. db.c . */
extern int top_of_zone_table;	/*. db.c . */
extern int xap_objs;
extern struct queue_event *command_queue;
extern int free_rent;
extern const char *hometown_menu;
extern const char *hometown_prompt;

/* external functions */
void echo_on(struct descriptor_data *d);
void echo_off(struct descriptor_data *d);
void do_start(struct char_data *ch, bool from_scratch);
void init_char(struct char_data *ch);
int special(struct char_data *ch, int cmd, char *arg);
int isbanned(char *hostname);
int Valid_Name(char *newname);
void oedit_parse(struct descriptor_data *d, char *arg);
void redit_parse(struct descriptor_data *d, char *arg);
void zedit_parse(struct descriptor_data *d, char *arg);
void medit_parse(struct descriptor_data *d, char *arg);
void sedit_parse(struct descriptor_data *d, char *arg);
void gedit_parse(struct descriptor_data *d, char *arg);
void hedit_parse(struct descriptor_data *d, char *arg);
void trigedit_parse(struct descriptor_data *d, char *arg);
void read_aliases(struct char_data *ch);	/* Alias mod */
int parse_race_for_menu(char arg);
int parse_class_for_menu(char arg);
int parse_stats(char arg, struct char_data *ch, long race);
long get_race_position(struct char_data *ch);
void init_clan_vari(struct descriptor_data *d);
void read_saved_vars(struct char_data *ch);
void dig_update(struct char_data *pxChar, int iState);
void skin_update(struct char_data *pxChar,
		 struct obj_data *pxCarcass, int iState);
void assemblies_parse(struct descriptor_data *d, char *arg);
void assedit_parse(struct descriptor_data *d, char *arg);
struct help_index_element *find_help(char *keyword, int times);
void write_aliases(struct char_data *ch);
void save_char_ascii(struct char_file_u *ch);
void set_default_player_stats(struct char_data *ch);

/* prototypes for all do_x functions. */
ACMD(do_action);
ACMD(do_add_xname);
ACMD(do_adjust_fight);
ACMD(do_addmember);
ACMD(do_add_news);
ACMD(do_addpoint);
ACMD(adjust_mobs);
ACMD(adjust_objs);
ACMD(adjust_rooms);
ACMD(adjust_shops);
ACMD(adjust_zones);
ACMD(do_ambush);
ACMD(do_advance);
ACMD(do_affects);
ACMD(do_alias);
ACMD(do_assedit);
ACMD(do_assemble);
ACMD(do_assist);
ACMD(do_at);
ACMD(do_attribute);
ACMD(do_backstab);
ACMD(do_ban);
ACMD(do_backup);
ACMD(do_bandage);
ACMD(do_bash);
ACMD(do_battle);
ACMD(do_become);
ACMD(do_board);
ACMD(do_berserk);
ACMD(do_brew);
ACMD(do_buck);
ACMD(do_buffer);
ACMD(do_camouflage);
ACMD(do_cast);
ACMD(do_castout);
ACMD(do_chant);
ACMD(do_charter);
ACMD(do_circle);
ACMD(do_clandonation);
ACMD(do_clan_recall);
ACMD(do_clanleader);
ACMD(do_clanpiece);
ACMD(do_csay);
ACMD(do_color);
ACMD(do_commands);
ACMD(do_commune);
ACMD(do_commsearch);
ACMD(do_consider);
ACMD(do_control_battle);
ACMD(do_credits);
ACMD(do_darken);
ACMD(do_date);
ACMD(do_dc);
ACMD(do_delay_func);		/* Bon 07/25/97 for event driven code in queue. */
ACMD(do_delpoint);
ACMD(do_descend);
ACMD(do_diagnose);
ACMD(do_dig);
ACMD(do_disarm);
ACMD(do_dismount);
ACMD(do_display);
/*ACMD(do_draw);*/
ACMD(do_drink);
ACMD(do_drop);
ACMD(do_eat);
ACMD(do_echo);
ACMD(do_end_discussion);
ACMD(do_enter);
ACMD(do_equipment);
ACMD(do_examine);
ACMD(do_explored);
ACMD(do_exit);
ACMD(do_exits);
ACMD(do_findpath);
ACMD(do_flee);
ACMD(do_flux);
ACMD(do_flush);
ACMD(do_fly);
ACMD(do_follow);
ACMD(do_force);
ACMD(do_forge);
ACMD(do_gecho);
ACMD(do_gen_comm);
ACMD(do_gen_door);
ACMD(do_gen_ps);
ACMD(do_gen_tog);
ACMD(do_gen_vfile);
ACMD(do_gen_write);
ACMD(do_get);
ACMD(do_guard);
ACMD(do_give);
ACMD(do_godlog);
ACMD(do_gold);
ACMD(do_gore);
ACMD(do_goto);
ACMD(do_grab);
ACMD(do_graffiti);
ACMD(do_gremort);
ACMD(do_greport);
ACMD(do_group);
ACMD(do_gsay);
ACMD(do_gwho);
ACMD(do_hcontrol);
ACMD(do_headbutt);
ACMD(do_help);
ACMD(do_helpcheck);
ACMD(do_hide);
ACMD(do_hit);
ACMD(do_house);
ACMD(do_identify);
ACMD(do_idle);
ACMD(do_igive);
ACMD(do_ignore);
ACMD(do_index);
ACMD(do_info);
ACMD(do_infobar);		/* -naj infobar2 - infobar prototype */
ACMD(do_insult);
ACMD(do_inventory);
ACMD(do_invis);
ACMD(do_itake);
ACMD(do_item_count);
ACMD(do_kick);
ACMD(do_kill);
ACMD(do_land);
ACMD(do_last);
ACMD(do_lastincrease);
ACMD(do_lay_hands);
ACMD(do_leave);
ACMD(do_levels);
ACMD(do_lighten);
ACMD(do_listclans);
ACMD(do_listmembers);
ACMD(do_list_reset);
ACMD(do_load);
ACMD(do_logsearch);
ACMD(do_look);
ACMD(do_makeclan);
ACMD(do_meditate);
ACMD(do_mlev);
ACMD(do_mlist);
ACMD(do_moblevels);
ACMD(do_mount);
ACMD(do_move);
ACMD(do_mortalize);
ACMD(do_mpasound);
ACMD(do_mpjunk);
ACMD(do_mpecho);
ACMD(do_mpechoat);
ACMD(do_mpechoaround);
ACMD(do_mpkill);
ACMD(do_mpmload);
ACMD(do_mpoload);
ACMD(do_mppurge);
ACMD(do_mpgoto);
ACMD(do_mpat);
ACMD(do_mptransfer);
ACMD(do_mpforce);
ACMD(do_not_here);
ACMD(do_objconv);
ACMD(do_offer);
ACMD(do_olc);
ACMD(do_olist);
ACMD(do_overflow);
ACMD(do_order);
ACMD(do_osearch);
ACMD(do_osearch2);
ACMD(do_palm);
ACMD(do_page);
ACMD(do_peace);
ACMD(do_peck);
ACMD(do_players);
ACMD(do_plink);
ACMD(do_plist);
ACMD(do_poofset);
ACMD(do_pour);
ACMD(do_practice);
ACMD(do_purge);
/*ACMD(do_project);*/
ACMD(do_put);
ACMD(do_qcomm);
ACMD(do_quivering_palm);
ACMD(do_kill_event);		/* Bon 07/25/97 for event driven code in queue. */
ACMD(do_quit);
ACMD(do_race_skillhelp);
ACMD(do_rage);
ACMD(do_reboot);
ACMD(do_recall);
ACMD(do_recharge);
ACMD(do_redirect);
ACMD(do_reelin);
ACMD(do_refuel);
ACMD(do_reimb);
ACMD(do_home);
ACMD(do_remove);
ACMD(do_remort);
ACMD(do_remortnet);
ACMD(check_invalid_remorts);
ACMD(do_removemember);
ACMD(do_rent);
ACMD(do_repair);
ACMD(do_reply);
ACMD(do_restring);
ACMD(do_report);
ACMD(do_retool);
ACMD(do_rescue);
ACMD(do_rest);
ACMD(do_restore);
ACMD(do_return);
ACMD(do_rlist);
ACMD(do_rove);
ACMD(do_sac);
ACMD(do_save);
ACMD(do_say);
ACMD(do_scan);
ACMD(do_score);
ACMD(do_scribe);
ACMD(do_send);
ACMD(do_set);
ACMD(do_shadow);
ACMD(do_shock);
ACMD(do_show);
ACMD(do_show_queue);		/* Bon 07/25/97 for event driven code in queue. */
ACMD(do_shutdown);
ACMD(do_sit);
ACMD(do_skillset);
ACMD(do_skin);
ACMD(do_sleep);
ACMD(do_slay);
ACMD(do_smite);
ACMD(do_sneak);
ACMD(do_snoop);
ACMD(do_spec_comm);
ACMD(do_spellhelp);
ACMD(do_split);
ACMD(do_stand);
ACMD(do_stat);
ACMD(do_steal);
ACMD(do_stomp);
ACMD(do_stun);
ACMD(do_sweep);
ACMD(do_switch);
ACMD(do_syslog);
ACMD(do_tag);
ACMD(do_tame);
ACMD(do_tedit);
ACMD(do_teleport);
ACMD(do_tell);
ACMD(do_test);
ACMD(do_time);
ACMD(do_title);
ACMD(do_toggle);
ACMD(do_track);
ACMD(do_trans);
ACMD(do_trip);
ACMD(do_unban);
ACMD(do_ungroup);
ACMD(do_use);
ACMD(do_users);
ACMD(do_uwizlist);
ACMD(do_visible);
ACMD(do_vload);
ACMD(do_vnum);
ACMD(do_vstat);
ACMD(do_vwear);
ACMD(do_wake);
ACMD(do_wear);
ACMD(do_weather);
ACMD(do_where);
ACMD(do_who);
ACMD(do_whois);
ACMD(do_who_battle);
ACMD(do_wield);
ACMD(do_wimpy);
ACMD(do_wizlock);
ACMD(do_wiznet);
ACMD(do_wizutil);
ACMD(do_write);
ACMD(do_zinfo);
ACMD(do_zlist);
ACMD(do_zreset);
/** 2/24/97, Anduin auction ACMD ***/
ACMD(do_auction);
ACMD(do_bid);
ACMD(do_linkload);
ACMD(do_distribute);		/* distribute items over a zone espec. maze */
ACMD(do_shoot);
ACMD(do_throw);
ACMD(do_pull);

ACMD(do_nonewbie);
ACMD(do_newbie);

ACMD(do_old_reimb);

ACMD(do_change_leader);

/* DG Script ACMD's */
ACMD(do_attach);
ACMD(do_detach);
ACMD(do_tlist);
ACMD(do_tcheck);
ACMD(do_tstat);
ACMD(do_masound);
ACMD(do_mat);
ACMD(do_mdamage);
ACMD(do_mecho);
ACMD(do_mechoaround);
ACMD(do_maddqp);
ACMD(do_mdoonce);
ACMD(do_mgoto);
ACMD(do_mjunk);
ACMD(do_mkill);
ACMD(do_mload);
ACMD(do_mpurge);
ACMD(do_msend);
ACMD(do_mteleport);
ACMD(do_mforce);
ACMD(do_mexp);
ACMD(do_mgold);
ACMD(do_mhunt);
ACMD(do_mremember);
ACMD(do_mforget);
ACMD(do_mtransform);
ACMD(do_mgremort);
ACMD(do_vdelete);
ACMD(do_mdoor);
ACMD(who_to_menu);
ACMD(do_shop);
/* This is the Master Command List(tm).

 * You can put new commands in, take commands out, change the order
 * they appear in, etc.  You can adjust the "priority" of commands
 * simply by changing the order they appear in the command list.
 * (For example, if you want "as" to mean "assist" instead of "ask",
 * just put "assist" above "ask" in the Master Command List(tm).
 *
 * In general, utility commands such as "at" should have high priority;
 * infrequently used and dangerously destructive commands should have low
 * priority.
 */


const struct command_info cmd_info[] = {
	{"RESERVED", 0, 0, 0, 0}
	,			/* this must be first -- for specprocs */
	/* directions must come before other commands but after RESERVED */
	{"north", POS_STANDING, do_move, 0, SCMD_NORTH},
	{"east", POS_STANDING, do_move, 0, SCMD_EAST},
	{"south", POS_STANDING, do_move, 0, SCMD_SOUTH},
	{"west", POS_STANDING, do_move, 0, SCMD_WEST},
	{"up", POS_STANDING, do_move, 0, SCMD_UP},
	{"down", POS_STANDING, do_move, 0, SCMD_DOWN},

	/* now, the main list */
	{"at", POS_DEAD, do_at, 108, 0},
	{"advance", POS_DEAD, do_advance, LVL_GRGOD, 0},
	{"addmember", POS_STANDING, do_addmember, 0, 0},
	{"addnews", POS_DEAD, do_add_news, LVL_ADMIN, 0},
	{"addpoint", POS_DEAD, do_addpoint, LVL_SERP, 0},
	{"addxname", POS_DEAD, do_add_xname, LVL_ARCH, 0},
	{"adjustfight", POS_DEAD, do_adjust_fight, LVL_ADMIN, 0},
	{"adjustmo", POS_DEAD, do_not_here, LVL_IMPL, 0},
	{"adjustmob", POS_DEAD, adjust_mobs, LVL_IMPL, 0},
	{"adjustob", POS_DEAD, do_not_here, LVL_IMPL, 0},
	{"adjustobj", POS_DEAD, adjust_objs, LVL_IMPL, 0},
	{"adjustroo", POS_DEAD, do_not_here, LVL_IMPL, 0},
	{"adjustroom", POS_DEAD, adjust_rooms, LVL_IMPL, 0},
	{"adjustsho", POS_DEAD, do_not_here, LVL_IMPL, 0},
	{"adjustshop", POS_DEAD, adjust_shops, LVL_IMPL, 0},
	{"adjustzon", POS_DEAD, do_not_here, LVL_IMPL, 0},
	{"adjustzone", POS_DEAD, adjust_zones, LVL_IMPL, 0},
	{"affects", POS_DEAD, do_affects, 0, 0},
	{"afk", POS_DEAD, do_gen_tog, 0, SCMD_AFK},
	{"alias", POS_DEAD, do_alias, 0, 0},
	{"ambush", POS_STANDING, do_ambush, 1, 0},
	{"areas", POS_DEAD, do_gen_ps, 0, SCMD_AREAS},
	{"ascii", POS_DEAD, do_gen_tog, 0, SCMD_ASCII},
	/* -naj infobar2 12/16/96 - infobar cmd */
	{"assist", POS_FIGHTING, do_assist, 1, 0},
	{"assemble", POS_SITTING, do_assemble, 0, SCMD_ASSEMBLE},
	{"assedit", POS_STANDING, do_assedit, LVL_ARCH, 0},
	{"ask", POS_CHANT, do_spec_comm, 0, SCMD_ASK},
	{"attributes", POS_SLEEPING, do_attribute, 0, 0},
	{"attach", POS_DEAD, do_attach, LVL_ADMIN, 0},
	{"auction", POS_SLEEPING, do_auction, 0, 0},
	{"autoexit", POS_DEAD, do_gen_tog, 0, SCMD_AUTOEXIT},
	{"autosplit", POS_DEAD, do_gen_tog, 0, SCMD_AUTOSPLIT},
	{"autoloot", POS_DEAD, do_gen_tog, 0, SCMD_AUTOLOOT},
	{"autosacrifice", POS_DEAD, do_gen_tog, 0, SCMD_AUTOSAC},
	{"autoassist", POS_DEAD, do_gen_tog, 0, SCMD_AUTOASSIST},
	{"autogold", POS_DEAD, do_gen_tog, 0, SCMD_AUTOGOLD},

	{"board", POS_DEAD, do_board, 0, 0},
	{"backup", POS_DEAD, do_backup, 108, 0},
	{"backstab", POS_STANDING, do_backstab, 1, 0},
	{"bake", POS_SITTING, do_assemble, 0, SCMD_BAKE},
	{"balance", POS_STANDING, do_not_here, 1, 0},
	{"ban", POS_DEAD, do_ban, LVL_DGOD, 0},
	{"bandage", POS_CHANT, do_bandage, 1, 0},
	{"bash", POS_FIGHTING, do_bash, 1, 0},
	{"battle", POS_STANDING, do_battle, 1, 0},
	{"bcontrol", POS_DEAD, do_control_battle, LVL_SERP, 0},
	{"become", POS_SLEEPING, do_become, LVL_ADMIN, 0},
	{"berserk", POS_FIGHTING, do_berserk, 1, 0},
	{"bet", POS_DEAD, do_not_here, 0, 0},
	{"bid", POS_SLEEPING, do_bid, 0, 0},
	{"brew", POS_STANDING, do_brew, 0, 0},
	/*{ "brew"     , POS_SITTING , do_assemble , 0, SCMD_BREW }, */
	{"brief", POS_DEAD, do_gen_tog, 0, SCMD_BRIEF},
	{"buck", POS_STANDING, do_buck, 0, 0},
	{"butcher", POS_DEAD, do_not_here, 0, 0},
	{"buy", POS_STANDING, do_not_here, 0, 0},
	{"bug", POS_DEAD, do_gen_write, 0, SCMD_BUG},
	{"buffer", POS_DEAD, do_buffer, LVL_IMPL, 0},
	{"bwho", POS_SLEEPING, do_who_battle, 0, 0},

	{"cast", POS_SITTING, do_cast, 1, SCMD_CAST},
	{"castout", POS_SITTING, do_castout, 1, 0},
	{"camouflage", POS_STANDING, do_camouflage, 1, 0},
	{"camp", POS_DEAD, do_quit, 0, SCMD_CAMP},
	{"camp!", POS_DEAD, do_quit, 0, SCMD_CAMPR},
	{"cbalance", POS_STANDING, do_not_here, 1, 0},
	{"cdeposit", POS_STANDING, do_not_here, 1, 0},
	{"cdonate", POS_CHANT, do_drop, 0, SCMD_CDONATE},
	{"chant", POS_SLEEPING, do_chant, 1, 0},
	{"check", POS_STANDING, do_not_here, 1, 0},
	{"changeleader", POS_STANDING, do_change_leader, 0, 0},
	{"charter", POS_SLEEPING, do_charter, 1, 0},
	{"circle", POS_FIGHTING, do_circle, 1, 0},
	{"clear", POS_DEAD, do_gen_ps, 0, SCMD_CLEAR},
	{"close", POS_SITTING, do_gen_door, 0, SCMD_CLOSE},
	{"cls", POS_DEAD, do_gen_ps, 0, SCMD_CLEAR},
	{"claninfo", POS_SITTING, do_listmembers, 1, 0},
	{"clandonation", POS_SITTING, do_clandonation, LVL_ADMIN, 0},
	{"clanleader", POS_DEAD, do_clanleader, LVL_SIMP, 0},
	{"clanpiece", POS_DEAD, do_clanpiece, LVL_SIMP, 0},
	{"consider", POS_CHANT, do_consider, 0, 0},
	{"color", POS_DEAD, do_color, 0, 0},
	{"commands", POS_DEAD, do_commands, 0, SCMD_COMMANDS},
	{"commune", POS_DEAD, do_commune, 0, 0},
	{"commsearch", POS_DEAD, do_commsearch, 1, SCMD_LOGSEARCH},
	{"compact", POS_DEAD, do_gen_tog, 0, SCMD_COMPACT},
	{"cost", POS_STANDING, do_not_here, 0, 0},
	{"count", POS_STANDING, do_item_count, 0, 0},
	{"craft", POS_SITTING, do_assemble, 0, SCMD_CRAFT},
	{"crash", POS_DEAD, do_overflow, LVL_IMPL, 0},
	{"crecall", POS_CHANT, do_clan_recall, 101, 0},
	{"credits", POS_DEAD, do_gen_ps, 0, SCMD_CREDITS},
	{"csay", POS_SLEEPING, do_csay, 0, 0},
	{"cwithdraw", POS_STANDING, do_not_here, 1, 0},

	{"darken", POS_STANDING, do_darken, 0, 0},
	{"date", POS_DEAD, do_date, LVL_IMMORT, SCMD_DATE},
	{"dc", POS_DEAD, do_dc, LVL_SERP, 0},
	{"delay", POS_DEAD, do_delay_func, LVL_ADMIN, 0},
	{"delmember", POS_STANDING, do_removemember, 0, 0},
	{"delpoint", POS_DEAD, do_delpoint, LVL_SERP, 0},
	{"deposit", POS_STANDING, do_not_here, 1, 0},
	{"descend", POS_CHANT, do_descend, 1, 0},
	{"detach", POS_DEAD, do_detach, LVL_ADMIN, 0},
	{"diagnose", POS_CHANT, do_diagnose, 0, 0},
	{"dig", POS_STANDING, do_dig, 0, 0},
	{"direction", POS_CHANT, do_exits, 0, 0},
	{"disarm", POS_FIGHTING, do_disarm, 1, 0},
	{"dismount", POS_STANDING, do_dismount, 0, 0},
	{"display", POS_DEAD, do_display, 0, 0},
	{"distribute", POS_DEAD, do_distribute, LVL_SERP, 0},
	{"divine", POS_STANDING, do_not_here, 1, 0},
	{"donate", POS_CHANT, do_drop, 0, SCMD_DONATE},
/*      { "draw"     , POS_STANDING, do_draw     , 0, 0 } , */
	{"drink", POS_CHANT, do_drink, 0, SCMD_DRINK},
	{"drop", POS_CHANT, do_drop, 0, SCMD_DROP},

	{"eat", POS_CHANT, do_eat, 0, SCMD_EAT},
	{"echo", POS_SLEEPING, do_echo, 108, SCMD_ECHO},
	{"emote", POS_CHANT, do_echo, 1, SCMD_EMOTE},
	{":", POS_CHANT, do_echo, 1, SCMD_EMOTE},
	{"endreply ", POS_DEAD, do_end_discussion, 108, 0},
	{"enter", POS_STANDING, do_enter, 0, 0},
	{"equipment", POS_SLEEPING, do_equipment, 0, 0},
	{"exits", POS_CHANT, do_exits, 0, 0},
	{"examine", POS_SITTING, do_examine, 0, 0},
	{"exchange", POS_STANDING, do_not_here, 1, 0},
	{"explored", POS_SLEEPING, do_explored, 0, 0},
	{"flux", POS_SLEEPING, do_flux, LVL_DGOD, 0},
	{"flush", POS_SLEEPING, do_flush, 0, 0},

	{"force", POS_SLEEPING, do_force, LVL_GRGOD, SCMD_FORCE},
	{"fight", POS_STANDING, do_not_here, 1, 0},
	{"fill", POS_STANDING, do_pour, 0, SCMD_FILL},
	{"findpath", POS_SLEEPING, do_findpath, LVL_GRGOD, 0},
	{"fish", POS_SITTING, do_castout, 1, 0},
	{"fix", POS_STANDING, do_not_here, 0, 0},
	{"flee", POS_FIGHTING, do_flee, 1, 0},
	{"fletch", POS_SITTING, do_assemble, 0, SCMD_FLETCH},
	{"fly", POS_CHANT, do_fly, 1, 0},
	{"follow", POS_CHANT, do_follow, 0, 0},
	{"forge", POS_STANDING, do_forge, 0, 0},
	{"freeze", POS_DEAD, do_wizutil, LVL_FREEZE, SCMD_FREEZE},

	{"gain", POS_CHANT, do_not_here, 1, 0},
	{"get", POS_CHANT, do_get, 0, 0},
	{"gecho", POS_DEAD, do_gecho, 108, 0},
	{"gedit", POS_DEAD, do_olc, LVL_ARCHI, SCMD_OLC_GEDIT},
	{"give", POS_CHANT, do_give, 0, 0},
	{"goto", POS_SLEEPING, do_goto, LVL_IMMORT, 0},
	{"godlog", POS_SLEEPING, do_godlog, 108, 0},
	{"gold", POS_CHANT, do_gold, 0, 0},
	{"gore", POS_FIGHTING, do_gore, 1, 0},
	{"gossip", POS_SLEEPING, do_gen_comm, 0, SCMD_GOSSIP},
	{".", POS_SLEEPING, do_gen_comm, 0, SCMD_GOSSIP},
	{"group", POS_SLEEPING, do_group, 1, 0},
	{"grab", POS_CHANT, do_grab, 0, 0},
	{"grats", POS_SLEEPING, do_gen_comm, 0, SCMD_GRATZ},
	{"graffiti", POS_STANDING, do_graffiti, LVL_IMMORT, 0},
	{"greport", POS_CHANT, do_greport, 0, 0},
	{"gremort", POS_DEAD, do_gremort, LVL_GRGOD, 0},
	{"gsay", POS_SLEEPING, do_gsay, 0, 0},
	{"gtell", POS_SLEEPING, do_gsay, 0, 0},
	{"guard", POS_FIGHTING, do_guard, 0, 0},
	{"gwho", POS_SLEEPING, do_gwho, 0, 0},

	{"help", POS_DEAD, do_help, 0, 0},
	{"hedit", POS_DEAD, do_olc, LVL_ADMIN, SCMD_OLC_HEDIT},
	{"headbutt", POS_FIGHTING, do_headbutt, 1, 0},
	{"handbook", POS_DEAD, do_gen_ps, 108, SCMD_HANDBOOK},
	{"hcontrol", POS_DEAD, do_hcontrol, LVL_ADMIN, 0},
	{"heal", POS_STANDING, do_not_here, 0, 0},
	{"helpcheck", POS_DEAD, do_helpcheck, LVL_SERP, 0},
   {"herolist", POS_DEAD, do_gen_ps, 0, SCMD_HEROLIST},
	{"hide", POS_CHANT, do_hide, 1, 0},
	{"hit", POS_FIGHTING, do_hit, 0, SCMD_HIT},
	{"hold", POS_CHANT, do_grab, 1, 0},
	{"holler", POS_CHANT, do_gen_comm, 1, SCMD_HOLLER},
	{"holylight", POS_DEAD, do_gen_tog, LVL_IMMORT, SCMD_HOLYLIGHT},
	{"home", POS_DEAD, do_home, LVL_IMMORT, 0},
	{"house", POS_CHANT, do_house, 0, 0},
	{"how", POS_STANDING, do_not_here, 0, 0},

	{"inventory", POS_DEAD, do_inventory, 0, 0},
	{"ignore", POS_DEAD, do_ignore, 0, 0},
	{"ident", POS_DEAD, do_gen_tog, LVL_SIMP, SCMD_IDENT},
	{"identify", POS_DEAD, do_identify, 0, 0},
	{"idea", POS_DEAD, do_gen_write, 0, SCMD_IDEA},
	{"idle", POS_DEAD, do_idle, 108, 0},
	{"iforce", POS_SLEEPING, do_force, LVL_SERP, SCMD_IFORCE},
	{"igive", POS_DEAD, do_igive, LVL_SERP, 0},
	{"imotd", POS_DEAD, do_gen_ps, LVL_IMMORT, SCMD_IMOTD},
	{"immlist", POS_DEAD, do_gen_ps, 0, SCMD_IMMLIST},
	{"index", POS_SLEEPING, do_index, 0, 0},
	{"info", POS_SLEEPING, do_gen_ps, 0, SCMD_INFO},
	/* -naj infobar2 12/16/96 - infobar cmd */
	/*    { "infobar"  , POS_DEAD    , do_gen_tog  , 0, SCMD_INFOBAR } , */
	{"insult", POS_CHANT, do_insult, 0, 0},
	{"invis", POS_DEAD, do_invis, 0, 0},
	{"itake", POS_DEAD, do_itake, LVL_SERP, 0},

	{"junk", POS_CHANT, do_drop, 0, SCMD_JUNK},

	{"kill", POS_FIGHTING, do_kill, 0, 0},
	{"kick", POS_FIGHTING, do_kick, 1, 0},
	{"knit", POS_SITTING, do_assemble, 0, SCMD_KNIT},

	{"look", POS_DEAD, do_look, 0, SCMD_LOOK},
	{"land", POS_CHANT, do_land, 1, 0},
	{"laston", POS_DEAD, do_last, 0, 0},
	{"lastincrease", POS_SLEEPING, do_lastincrease, 0, 0},
	{"lastlearn", POS_SLEEPING, do_lastincrease, 0, 0},
	{"lay_hands", POS_STANDING, do_lay_hands, 1, 0},
	{"leave", POS_STANDING, do_leave, 0, 0},
	{"learn", POS_CHANT, do_not_here, 1, 0},
	{"levels", POS_DEAD, do_levels, 0, 0},
	{"lighten", POS_STANDING, do_lighten, 1, 0},
	{"linkload", POS_DEAD, do_linkload, LVL_ADMIN, 0},
	{"list", POS_STANDING, do_not_here, 0, 0},
	{"listclans", POS_DEAD, do_listclans, 1, 0},
	{"listmembers", POS_DEAD, do_listmembers, 0, 0},
	{"listreset", POS_DEAD, do_list_reset, LVL_ADMIN, 0},
	{"lock", POS_SITTING, do_gen_door, 0, SCMD_LOCK},
	{"load", POS_DEAD, do_load, 108, 0},
	{"logsearch", POS_DEAD, do_logsearch, 108, SCMD_LOGSEARCH},
	{"ls", POS_CHANT, do_look, 0, SCMD_LOOK},

	{"mail", POS_STANDING, do_not_here, 1, 0},
	{"make", POS_STANDING, do_assemble, 0, SCMD_MAKE},
	{"makeclan", POS_DEAD, do_makeclan, LVL_ADMIN, 0},
	{"marriages", POS_DEAD, do_gen_ps, 0, SCMD_MARRIAGES},
	{"medit", POS_DEAD, do_olc, LVL_ARCH, SCMD_OLC_MEDIT},
	{"meditate", POS_SLEEPING, do_meditate, 1, 0},
	{"meter", POS_DEAD, do_gen_tog, 0, SCMD_METER},
	/* -naj infobar2 12/16/96 - infobar cmd */
	{"mix", POS_STANDING, do_assemble, 0, SCMD_MIX},
	{"motd", POS_DEAD, do_gen_ps, 0, SCMD_MOTD},
	{"mlev", POS_DEAD, do_mlev, 108, 0},
	{"mlist", POS_DEAD, do_mlist, 108, 0},
	{"mount", POS_STANDING, do_mount, 0, 0},
	{"mortalize", POS_DEAD, do_gen_tog, 108, SCMD_MORTAL},
	{"moblevels", POS_DEAD, do_moblevels, LVL_DGOD, 0},
	{"move", POS_STANDING, do_not_here, 0, 0},
	{"mute", POS_DEAD, do_wizutil, LVL_SERP, SCMD_SQUELCH},
	{"murder", POS_FIGHTING, do_hit, 125, SCMD_MURDER},
	{"music", POS_SLEEPING, do_gen_comm, 0, SCMD_MUSIC},

	{"mpasound", POS_DEAD, do_mpasound, 0, 0},
	{"mpjunk", POS_DEAD, do_mpjunk, 0, 0},
	{"mpecho", POS_DEAD, do_mpecho, 0, 0},
	{"mpechoat", POS_DEAD, do_mpechoat, 0, 0},
	{"mpechoaround", POS_DEAD, do_mpechoaround, 0, 0},
	{"mpkill", POS_DEAD, do_mpkill, 0, 0},
	{"mpmload", POS_DEAD, do_mpmload, 0, 0},
	{"mpoload", POS_DEAD, do_mpoload, 0, 0},
	{"mppurge", POS_DEAD, do_mppurge, 0, 0},
	{"mpgoto", POS_DEAD, do_mpgoto, 0, 0},
	{"mpat", POS_DEAD, do_mpat, 0, 0},
	{"mptransfer", POS_DEAD, do_mptransfer, 0, 0},
	{"mpforce", POS_DEAD, do_mpforce, 0, 0},
	{"masound", POS_DEAD, do_masound, 0, 0},
	{"mdamage", POS_DEAD, do_mdamage, 0, 0},
	{"mkill", POS_STANDING, do_mkill, 0, 0},
	{"mjunk", POS_SITTING, do_mjunk, 0, 0},
	{"mecho", POS_DEAD, do_mecho, -1, 0},
	{"mechoaround", POS_DEAD, do_mechoaround, -1, 0},
	{"maddqp", POS_DEAD, do_maddqp, 0, 0},
	{"mdoonce", POS_DEAD, do_mdoonce, 0, 0},
	{"msend", POS_DEAD, do_msend, -1, 0},
	{"mload", POS_DEAD, do_mload, -1, 0},
	{"mpurge", POS_DEAD, do_mpurge, -1, 0},
	{"mgoto", POS_DEAD, do_mgoto, -1, 0},
	{"mdoor", POS_DEAD, do_mdoor, -1, 0},
	{"mat", POS_DEAD, do_mat, -1, 0},
	{"mteleport", POS_DEAD, do_mteleport, -1, 0},
	{"mforce", POS_DEAD, do_mforce, -1, 0},
	{"mexp", POS_DEAD, do_mexp, -1, 0},
	{"mgold", POS_DEAD, do_mgold, -1, 0},
	{"mhunt", POS_DEAD, do_mhunt, -1, 0},
	{"mremember", POS_DEAD, do_mremember, -1, 0},
	{"mforget", POS_DEAD, do_mforget, -1, 0},
	{"mtransform", POS_DEAD, do_mtransform, -1, 0},
	{"mgremort", POS_DEAD, do_mgremort, -1, 0},
	/*{ "newbie"   , POS_SLEEPING, do_newbie   , 0, 0 }, */
	{"news", POS_SLEEPING, do_gen_ps, 0, SCMD_NEWS},
	{"noauction", POS_DEAD, do_gen_tog, 0, SCMD_NOAUCTION},
	{"nobattle", POS_DEAD, do_gen_tog, 1, SCMD_NOBATTLE},
	{"nofight", POS_DEAD, do_gen_tog, 0, SCMD_NOSPAM},
	{"nogossip", POS_DEAD, do_gen_tog, 0, SCMD_NOGOSSIP},
	{"nograts", POS_DEAD, do_gen_tog, 0, SCMD_NOGRATZ},
	{"nohassle", POS_DEAD, do_gen_tog, 108, SCMD_NOHASSLE},
	{"noinfo", POS_DEAD, do_gen_tog, 0, SCMD_NOINFO},
	{"nomusic", POS_DEAD, do_gen_tog, 0, SCMD_NOMUSIC},
	{"noooc", POS_DEAD, do_gen_tog, 0, SCMD_NOOOC},
	{"norecall", POS_DEAD, do_gen_tog, 1, SCMD_NORECALL},
	{"noshout", POS_SLEEPING, do_gen_tog, 1, SCMD_DEAF},
	{"nosummon", POS_DEAD, do_gen_tog, 1, SCMD_NOSUMMON},
	{"notell", POS_DEAD, do_gen_tog, 1, SCMD_NOTELL},
	{"notitle", POS_DEAD, do_wizutil, LVL_DETY, SCMD_NOTITLE},
	{"nowiz", POS_DEAD, do_gen_tog, 108, SCMD_NOWIZ},
	/*{ "nonewbie" , POS_DEAD    , do_nonewbie , 0         , 0}, */

	{"offer", POS_STANDING, do_not_here, 1, 0},
	{"ooc", POS_SLEEPING, do_gen_comm, 0, SCMD_OOC},
	{"open", POS_SITTING, do_gen_door, 0, SCMD_OPEN},
	{"order", POS_CHANT, do_order, 1, 0},
	{"olist", POS_DEAD, do_olist, 108, 0},
	{"olc", POS_DEAD, do_olc, 108, SCMD_OLC_SAVEINFO},
	{"oedit", POS_DEAD, do_olc, LVL_ARCH, SCMD_OLC_OEDIT},
	{"osearch", POS_DEAD, do_osearch, LVL_DGOD, 0},
	{"osearch2", POS_DEAD, do_osearch2, LVL_DGOD, 0},
	/*   { "objconv"  , POS_DEAD    , do_objconv  , LVL_IMPL, 0 }, */

	{"put", POS_CHANT, do_put, 0, 0},
	{"page", POS_DEAD, do_page, 0, 0},
	{"pageok", POS_DEAD, do_gen_tog, 0, SCMD_PAGE_OK},
	{"palm", POS_STANDING, do_palm, 1, 0},
	{"pardon", POS_DEAD, do_wizutil, LVL_SERP, SCMD_PARDON},
	{"peace", POS_DEAD, do_peace, LVL_DETY, 0},
	{"peck", POS_FIGHTING, do_peck, 0, 0},
	{"pedit", POS_DEAD, do_olc, LVL_IMPL + 1, SCMD_OLC_PEDIT},
	{"pick", POS_STANDING, do_gen_door, 1, SCMD_PICK},
	{"players", POS_DEAD, do_players, LVL_DGOD, 0},
	{"plink", POS_SLEEPING, do_plink, 108, 0},
	{"plist", POS_DEAD, do_plist, 108, 0},
	{"policy", POS_DEAD, do_gen_ps, 0, SCMD_POLICIES},
	{"poofcheck", POS_DEAD, do_poofset, LVL_IMMORT, SCMD_POOFCHECK},
	{"poofin", POS_DEAD, do_poofset, LVL_IMMORT, SCMD_POOFIN},
	{"poofout", POS_DEAD, do_poofset, LVL_IMMORT, SCMD_POOFOUT},
	{"pour", POS_STANDING, do_pour, 0, SCMD_POUR},
	{"prompt", POS_DEAD, do_display, 0, 0},
	{"practice", POS_CHANT, do_practice, 1, 0},
	/*{ "project"  , POS_DEAD    , do_project  , 108, 0}, */
	{"pull", POS_STANDING, do_pull, 0, 0},
	{"ps", POS_DEAD, do_show_queue, LVL_GOD, 0},
	{"purge", POS_DEAD, do_purge, 108, 0},

	{"quaff", POS_CHANT, do_use, 0, SCMD_QUAFF},
	{"quivering_palm", POS_FIGHTING, do_quivering_palm, 1, 0},
	{"qecho", POS_DEAD, do_qcomm, 108, SCMD_QECHO},
	{"qkill", POS_DEAD, do_kill_event, LVL_ADMIN, 0},
	{"quest", POS_DEAD, do_gen_tog, 0, SCMD_QUEST},
	{"quit", POS_DEAD, do_quit, 0, SCMD_QUI},
	{"quit!", POS_DEAD, do_quit, 0, SCMD_QUIT},
	{"qsay", POS_SLEEPING, do_qcomm, 0, SCMD_QSAY},

	{"reply", POS_SLEEPING, do_reply, 0, 0},
	{"restring", POS_SLEEPING, do_restring, LVL_IMPL, 0},
	{"raceskills", POS_DEAD, do_race_skillhelp, 0, 0},
	{"rage", POS_FIGHTING, do_rage, 1, 0},
	{"rest", POS_CHANT, do_rest, 0, 0},
	{"read", POS_CHANT, do_look, 0, SCMD_READ},
	{"refuel", POS_CHANT, do_refuel, 0, 0},
	{"reload", POS_DEAD, do_reboot, LVL_ADMIN, 0},
	{"recite", POS_FIGHTING, do_use, 0, SCMD_RECITE},
	{"recall", POS_CHANT, do_recall, 0, 0},
	{"receive", POS_STANDING, do_not_here, 1, 0},
	{"recharge", POS_STANDING, do_recharge, 1, 0},
	{"redirect", POS_FIGHTING, do_redirect, 1, 0},
	{"redraw", POS_DEAD, do_infobar, 0, SCMDB_REDRAW},
	{"reelin", POS_SITTING, do_reelin, 1, 0},
	{"old_reimburse", POS_CHANT, do_reimb, LVL_ADMIN, 0},
	{"remove", POS_CHANT, do_remove, 0, 0},
	{"remortnet", POS_SLEEPING, do_remortnet, 0, 0},
	{"remort!", POS_CHANT, do_remort, LVL_HERO, 0},
	{"remorts", POS_DEAD, check_invalid_remorts, 108, 0},
	{"reimburse", POS_STANDING, do_old_reimb, 0, 0},
	{",", POS_SLEEPING, do_remortnet, 0, 0},
	{"rent", POS_STANDING, do_not_here, 1, 0},
	{"repair", POS_STANDING, do_repair, 0, 0},
	{"report", POS_CHANT, do_report, 0, 0},
	/*{ "retool"   , POS_DEAD    , do_retool  , 0, 0 }, */
	{"rescue", POS_FIGHTING, do_rescue, 1, 0},
	{"resize", POS_DEAD, do_infobar, 0, SCMDB_RESIZE},
	{"restore", POS_DEAD, do_restore, LVL_DGOD, 0},
	{"return", POS_DEAD, do_return, 0, 0},
	{"redit", POS_DEAD, do_olc, 108, SCMD_OLC_REDIT},
	{"revsnoop", POS_DEAD, do_snoop, LVL_SERP, SCMD_REVSNOOP},
	{"reward", POS_STANDING, do_not_here, 1, 0},
	{"rlist", POS_DEAD, do_rlist, 108, 0},
	{"roomflags", POS_DEAD, do_gen_tog, 108, SCMD_ROOMFLAGS},
	{"rove", POS_STANDING, do_rove, 1, 0},

	{"say", POS_CHANT, do_say, 0, 0},
	{"'", POS_CHANT, do_say, 0, 0},
	{"sacrifice", POS_FIGHTING, do_sac, 0, 0},
	{"safe", POS_STANDING, do_not_here, 1, 0},
	{"save", POS_SLEEPING, do_save, 0, 0},
	{"score", POS_DEAD, do_score, 0, 0},
	{"scan", POS_STANDING, do_scan, 0, 0},
	{"scorebar", POS_DEAD, do_gen_tog, 0, SCMD_SCOREBAR},
	{"shop", POS_STANDING, do_shop, 0, 0},
	/* -naj infobar2 12/16/96 - infobar cmd */
	{"scribe", POS_STANDING, do_scribe, 0, 0},
	{"sell", POS_STANDING, do_not_here, 0, 0},
	{"send", POS_SLEEPING, do_send, LVL_DGOD, 0},
	{"set", POS_DEAD, do_set, 108, 0},
	{"sedit", POS_DEAD, do_olc, LVL_ARCHI, SCMD_OLC_SEDIT},
	{"shadow", POS_STANDING, do_shadow, 1, 0},
	/*    { "shock"    , POS_FIGHTING, do_shock    , 1, 0 } ,  */
	{"shout", POS_CHANT, do_gen_comm, 0, SCMD_SHOUT},
	{"show", POS_DEAD, do_show, 0, 0},
	{"shoot", POS_STANDING, do_shoot, 0, 0},
	{"shutdow", POS_DEAD, do_shutdown, LVL_ADMIN, 0},
	{"shutdown", POS_DEAD, do_shutdown, LVL_ADMIN, SCMD_SHUTDOWN},
	{"sing", POS_SITTING, do_cast, 1, SCMD_SING},
	{"sip", POS_CHANT, do_drink, 0, SCMD_SIP},
	{"sit", POS_CHANT, do_sit, 0, 0},
	{"skills", POS_DEAD, do_spellhelp, 0, SCMD_SKILL},
	{"slay", POS_SLEEPING, do_slay, LVL_ADMIN, 0},
	{"skin", POS_STANDING, do_skin, 0, 0},
	{"skillset", POS_SLEEPING, do_skillset, LVL_GRGOD, SCMD_SETSKILL},
	{"skilllearn", POS_SLEEPING, do_skillset, LVL_GRGOD, SCMD_SETLEARN},
	{"sleep", POS_CHANT, do_sleep, 0, 0},
	{"slowns", POS_DEAD, do_gen_tog, LVL_ADMIN, SCMD_SLOWNS},
	{"smile", POS_CHANT, do_action, 0, 0},
	{"smite", POS_SLEEPING, do_smite, 108, 0},
	{"sneak", POS_STANDING, do_sneak, 1, 0},
	{"snoop", POS_DEAD, do_snoop, LVL_SERP, SCMD_SNOOP},
	{"socials", POS_DEAD, do_commands, 0, SCMD_SOCIALS},
	{"spells", POS_DEAD, do_spellhelp, 0, SCMD_SPELL},
	{"split", POS_SITTING, do_split, 1, 0},
	{"stand", POS_CHANT, do_stand, 0, 0},
	{"stable", POS_STANDING, do_not_here, 1, 0},
	{"stat", POS_DEAD, do_stat, 108, 0},
	{"steal", POS_STANDING, do_steal, 1, 0},
	{"stomp", POS_FIGHTING, do_stomp, 1, 0},
	{"stun", POS_FIGHTING, do_stun, 1, 0},
	/* Pill modification--Aleks */
	{"swallow", POS_CHANT, do_use, 0, SCMD_SWALLOW},
	{"sweep", POS_FIGHTING, do_sweep, 1, 0},
	{"switch", POS_DEAD, do_switch, LVL_IMMORT, 0},
	{"syslog", POS_DEAD, do_syslog, LVL_IMMORT, 0},

	{"tell", POS_DEAD, do_tell, 0, 0},
	{"tag", POS_CHANT, do_tag, 0, 0},
	{"take", POS_CHANT, do_get, 0, 0},
	{"tame", POS_STANDING, do_tame, 0, 0},
	{"taste", POS_CHANT, do_eat, 0, SCMD_TASTE},
	{"tcheck", POS_DEAD, do_tcheck, LVL_SERP, 0},
	{"teams", POS_SLEEPING, do_gen_ps, 0, SCMD_TEAMS},
	{"tedit", POS_DEAD, do_tedit, LVL_GRGODI, 0},
	{"teleport", POS_DEAD, do_teleport, LVL_SERP, 0},
	{"test", POS_DEAD, do_test, LVL_IMPL, 0},
	{"thatch", POS_SITTING, do_assemble, 0, SCMD_THATCH},
	{"thaw", POS_DEAD, do_wizutil, LVL_FREEZE, SCMD_THAW},
	{"throw", POS_STANDING, do_throw, 0, 0},
	{"title", POS_DEAD, do_title, 0, 0},
	{"time", POS_DEAD, do_time, 0, 0},
	{"tlist", POS_DEAD, do_tlist, LVL_SERP, 0},
	{"toggle", POS_DEAD, do_toggle, 0, 0},
	{"track", POS_STANDING, do_track, 0, 0},
	{"trackthru", POS_DEAD, do_gen_tog, LVL_ADMIN, SCMD_TRACK},
	{"transfer", POS_SLEEPING, do_trans, LVL_SERP, 0},
	{"trigedit", POS_DEAD, do_olc, LVL_ARCHI, SCMD_OLC_TRIGEDIT},
	{"trip", POS_FIGHTING, do_trip, 1, 0},
	{"tstat", POS_DEAD, do_tstat, LVL_ARCHI, 0},
	{"typo", POS_DEAD, do_gen_write, 0, SCMD_TYPO},

	{"unlock", POS_SITTING, do_gen_door, 0, SCMD_UNLOCK},
	{"ungroup", POS_DEAD, do_ungroup, 0, 0},
	{"unban", POS_DEAD, do_unban, LVL_DGOD, 0},
	{"unaffect", POS_DEAD, do_wizutil, LVL_DGOD, SCMD_UNAFFECT},
	{"unused", POS_DEAD, do_spellhelp, LVL_GRGOD, SCMD_UNUSED},
	{"uptime", POS_DEAD, do_date, 108, SCMD_UPTIME},
	{"use", POS_SITTING, do_use, 1, SCMD_USE},
	{"users", POS_DEAD, do_users, LVL_IMMORT, 0},
	{"uwizlist", POS_DEAD, do_uwizlist, LVL_DETY, 0},

	{"value", POS_STANDING, do_not_here, 0, 0},
	{"vardelete", POS_DEAD, do_vdelete, LVL_SIMP, 0},
	{"version", POS_DEAD, do_gen_ps, 0, SCMD_VERSION},
	{"vbadpws", POS_DEAD, do_gen_vfile, LVL_ADMIN, SCMD_V_BADPWS},
	{"vban", POS_DEAD, do_gen_vfile, 108, SCMD_V_BAN},
	{"vbigrent", POS_DEAD, do_gen_vfile, LVL_DGOD, SCMD_V_BIGRENT},
	{"vbuf", POS_DEAD, do_gen_vfile, 108, SCMD_V_BUF},
	{"vbugs", POS_DEAD, do_gen_vfile, LVL_DETY, SCMD_V_BUGS},
	{"vchanges", POS_DEAD, do_gen_vfile, 108, SCMD_V_CHANGES},
	{"vcorpse", POS_DEAD, do_gen_vfile, 108, SCMD_V_CORPSE},
	{"vcrash", POS_DEAD, do_gen_vfile, LVL_GRGOD, SCMD_V_CRASH},
	{"vdeath", POS_DEAD, do_gen_vfile, 108, SCMD_V_DEATH},
	{"vdeleted", POS_DEAD, do_gen_vfile, 108, SCMD_V_DELETE},
	{"verrors", POS_DEAD, do_gen_vfile, 108, SCMD_V_ERRORS},
	{"vgodcmd", POS_DEAD, do_gen_vfile, LVL_ADMIN, SCMD_V_GODCMD},
	{"vgold", POS_DEAD, do_gen_vfile, 108, SCMD_V_GOLD},
	{"vgodfight", POS_DEAD, do_gen_vfile, LVL_GOD, SCMD_V_GODFIGHT},
	{"vhelp", POS_DEAD, do_gen_vfile, 108, SCMD_V_HELP},
	{"videas", POS_DEAD, do_gen_vfile, 108, SCMD_V_IDEAS},
	{"viewcomms", POS_DEAD, do_commsearch, 1, SCMD_VIEWLOG},
	{"viewlog", POS_DEAD, do_logsearch, 108, SCMD_VIEWLOG},
	{"vlastcmd", POS_DEAD, do_gen_vfile, LVL_ADMIN, SCMD_V_LASTCMD},
	{"vlevels", POS_DEAD, do_gen_vfile, 108, SCMD_V_LEVELS},
	{"vshop", POS_DEAD, do_gen_vfile, LVL_IMMORT, SCMD_V_SHOP},
	{"vlocateobj", POS_DEAD, do_gen_vfile, LVL_ADMIN, SCMD_V_LOCATE_OBJ},
	{"vmaillog", POS_DEAD, do_gen_vfile, LVL_IMPL, SCMD_V_MAILLOG},
	{"vnew", POS_DEAD, do_gen_vfile, 108, SCMD_V_NEWPLAYERS},
	{"vobjscrap", POS_DEAD, do_gen_vfile, 108, SCMD_V_OBJSCRAP},
	{"volc", POS_DEAD, do_gen_vfile, 108, SCMD_V_OLC},
	{"vrentgone", POS_DEAD, do_gen_vfile, LVL_ADMIN, SCMD_V_RENTGONE},
	{"vrestarts", POS_DEAD, do_gen_vfile, 108, SCMD_V_RESTARTS},
	{"vrip", POS_DEAD, do_gen_vfile, 108, SCMD_V_RIP},
	{"vscripterr", POS_DEAD, do_gen_vfile, 108, SCMD_V_SCRIPTERR},
	{"vscriptlog", POS_DEAD, do_gen_vfile, 108, SCMD_V_SCRIPTLOG},
	{"vsyslog", POS_DEAD, do_gen_vfile, LVL_ADMIN, SCMD_V_SYSLOG},
	{"visible", POS_CHANT, do_visible, 1, 0},
	{"vload", POS_DEAD, do_vload, LVL_DGOD, 0},
	{"vnum", POS_DEAD, do_vnum, 108, 0},
	{"vstat", POS_DEAD, do_vstat, 108, 0},
	{"vtypos", POS_DEAD, do_gen_vfile, 108, SCMD_V_TYPOS},
	{"vusage", POS_DEAD, do_gen_vfile, LVL_GOD, SCMD_V_USAGE},
	{"vwear", POS_DEAD, do_vwear, 108, 0},

	{"wake", POS_SLEEPING, do_wake, 0, 0},
	{"wear", POS_CHANT, do_wear, 0, 0},
	{"weather", POS_CHANT, do_weather, 0, 0},
	{"weave", POS_SITTING, do_assemble, 0, SCMD_WEAVE},
	{"who", POS_DEAD, do_who, 0, 0},
	{"whoami", POS_DEAD, do_gen_ps, 0, SCMD_WHOAMI},
	{"whois", POS_DEAD, do_whois, 0, 0},
	{"where", POS_CHANT, do_where, 1, 0},
	{"whisper", POS_CHANT, do_spec_comm, 0, SCMD_WHISPER},
	{"wield", POS_CHANT, do_wield, 0, 0},
	{"wimpy", POS_DEAD, do_wimpy, 0, 0},
	{"withdraw", POS_STANDING, do_not_here, 1, 0},
	{"wiznet", POS_DEAD, do_wiznet, LVL_IMMORT, 0},
	{";", POS_DEAD, do_wiznet, LVL_IMMORT, 0},
	{"wizhelp", POS_SLEEPING, do_commands, LVL_IMMORT, SCMD_WIZHELP},
	{"wizlist", POS_DEAD, do_gen_ps, 0, SCMD_WIZLIST},
	{"wizlock", POS_DEAD, do_wizlock, LVL_IMPL, 0},
	{"write", POS_STANDING, do_write, 1, 0},

	/* { "xapobjs"  , POS_DEAD,     do_gen_tog  , LVL_IMPL, SCMD_XAP_OBJS }, */

	{"zedit", POS_DEAD, do_olc, 108, SCMD_OLC_ZEDIT},
	{"zinfo", POS_DEAD, do_zinfo, 1, 0},
	{"zlist", POS_DEAD, do_zlist, 108, 0},
	{"zreset", POS_DEAD, do_zreset, 108, 0},

	/* ALL SOCIALS GO HERE */
	{"accuse", POS_SITTING, do_action, 0, 0},
	{"ack", POS_DEAD, do_action, 0, 0},
	{"addict", POS_CHANT, do_action, 0, 0},
	{"afw", POS_DEAD, do_action, 0, 0},
	{"agree", POS_CHANT, do_action, 0, 0},
	{"ahh", POS_CHANT, do_action, 0, 0},
	{"apologize", POS_CHANT, do_action, 0, 0},
	{"applaud", POS_CHANT, do_action, 0, 0},
	{"banzai", POS_CHANT, do_action, 0, 0},
	{"bark", POS_CHANT, do_action, 0, 0},
	{"bart", POS_STANDING, do_action, 0, 0},
	{"beam", POS_CHANT, do_action, 0, 0},
	{"bearhug", POS_CHANT, do_action, 0, 0},
	{"beckon", POS_CHANT, do_action, 0, 0},
	{"beef", POS_CHANT, do_action, 0, 0},
	{"beer", POS_CHANT, do_action, 0, 0},
	{"beg", POS_CHANT, do_action, 0, 0},
	{"blame", POS_CHANT, do_action, 0, 0},
	{"bleed", POS_CHANT, do_action, 0, 0},
	{"blink", POS_CHANT, do_action, 0, 0},
	{"blush", POS_CHANT, do_action, 0, 0},
	{"boggle", POS_CHANT, do_action, 0, 0},
	{"bonk", POS_CHANT, do_action, 0, 0},
	{"booger", POS_CHANT, do_action, 0, 0},
	{"bottle", POS_CHANT, do_action, 0, 0},
	{"bounce", POS_CHANT, do_action, 0, 0},
	{"bow", POS_STANDING, do_action, 0, 0},
	{"brb", POS_CHANT, do_action, 0, 0},
	{"burn", POS_CHANT, do_action, 0, 0},
	{"burp", POS_CHANT, do_action, 0, 0},
	{"cackle", POS_CHANT, do_action, 0, 0},
	{"cartwheel", POS_CHANT, do_action, 0, 0},
	{"catnap", POS_CHANT, do_action, 0, 0},
	{"cheer", POS_CHANT, do_action, 0, 0},
	{"chuckle", POS_CHANT, do_action, 0, 0},
	{"choke", POS_STANDING, do_action, 0, 0},
	{"clap", POS_CHANT, do_action, 0, 0},
	{"clue", POS_CHANT, do_action, 0, 0},
	{"clueless", POS_CHANT, do_action, 0, 0},
	{"comb", POS_CHANT, do_action, 0, 0},
	{"comfort", POS_CHANT, do_action, 0, 0},
	{"concerned", POS_CHANT, do_action, 0, 0},
	{"confused", POS_CHANT, do_action, 0, 0},
	{"cough", POS_CHANT, do_action, 0, 0},
	{"cower", POS_CHANT, do_action, 0, 0},
	{"cringe", POS_CHANT, do_action, 0, 0},
	{"cross", POS_CHANT, do_action, 0, 0},
	{"cry", POS_CHANT, do_action, 0, 0},
	{"cuddle", POS_CHANT, do_action, 0, 0},
	{"curious", POS_CHANT, do_action, 0, 0},
	{"curse", POS_CHANT, do_action, 0, 0},
	{"curtsey", POS_STANDING, do_action, 0, 0},
	{"dance", POS_STANDING, do_action, 0, 0},
	{"daydream", POS_SLEEPING, do_action, 0, 0},
	{"disbelieve", POS_CHANT, do_action, 0, 0},
	{"disgusted", POS_CHANT, do_action, 0, 0},
	{"dizzy", POS_CHANT, do_action, 0, 0},
	{"doc", POS_CHANT, do_action, 0, 0},
	{"doh", POS_CHANT, do_action, 0, 0},
	{"drool", POS_CHANT, do_action, 0, 0},
	{"duck", POS_CHANT, do_action, 0, 0},
	{"embarrassed", POS_CHANT, do_action, 0, 0},
	{"embrace", POS_CHANT, do_action, 0, 0},
	{"envy", POS_CHANT, do_action, 0, 0},
	{"eyebrows", POS_CHANT, do_action, 0, 0},
	{"eyeroll", POS_CHANT, do_action, 0, 0},
	{"faint", POS_CHANT, do_action, 0, 0},
	{"fakerep", POS_CHANT, do_action, 0, 0},
	{"fart", POS_CHANT, do_action, 0, 0},
	{"flex", POS_CHANT, do_action, 0, 0},
	{"flip", POS_STANDING, do_action, 0, 0},
	{"flirt", POS_CHANT, do_action, 0, 0},
	{"flowers", POS_CHANT, do_action, 0, 0},
	{"flutter", POS_CHANT, do_action, 0, 0},
	{"fondle", POS_CHANT, do_action, 0, 0},
	{"french", POS_CHANT, do_action, 0, 0},
	{"frolic", POS_CHANT, do_action, 0, 0},
	{"frown", POS_CHANT, do_action, 0, 0},
	{"frustrated", POS_CHANT, do_action, 0, 0},
	{"fume", POS_CHANT, do_action, 0, 0},
	{"gag", POS_CHANT, do_action, 0, 0},
	{"gasp", POS_CHANT, do_action, 0, 0},
	{"ghug", POS_CHANT, do_action, 0, 0},
	{"giggle", POS_CHANT, do_action, 0, 0},
	{"glare", POS_CHANT, do_action, 0, 0},
	{"gloat", POS_CHANT, do_action, 0, 0},
	{"glomp", POS_CHANT, do_action, 0, 0},
	{"greet", POS_CHANT, do_action, 0, 0},
	{"grimace", POS_CHANT, do_action, 0, 0},
	{"grin", POS_CHANT, do_action, 0, 0},
	{"groan", POS_CHANT, do_action, 0, 0},
	{"grope", POS_CHANT, do_action, 0, 0},
	{"grovel", POS_CHANT, do_action, 0, 0},
	{"growl", POS_CHANT, do_action, 0, 0},
	{"grumble", POS_CHANT, do_action, 0, 0},
	{"grunt", POS_CHANT, do_action, 0, 0},
	{"hangover", POS_CHANT, do_action, 0, 0},
	{"happy", POS_CHANT, do_action, 0, 0},
	{"halo", POS_CHANT, do_action, 0, 0},
	{"hiccup", POS_CHANT, do_action, 0, 0},
	{"highfive", POS_STANDING, do_action, 0, 0},
	{"hmmm", POS_CHANT, do_action, 0, 0},
	{"hop", POS_STANDING, do_action, 0, 0},
	{"howl", POS_CHANT, do_action, 0, 0},
	{"hug", POS_CHANT, do_action, 0, 0},
	{"hungry", POS_CHANT, do_action, 0, 0},
	{"jump", POS_CHANT, do_action, 0, 0},
	{"kiss", POS_CHANT, do_action, 0, 0},
	{"kneel", POS_STANDING, do_action, 0, 0},
	{"knuckles", POS_CHANT, do_action, 0, 0},
	{"lag", POS_CHANT, do_action, 0, 0},
	{"laugh", POS_CHANT, do_action, 0, 0},
	{"lick", POS_CHANT, do_action, 0, 0},
	{"listen", POS_CHANT, do_action, 0, 0},
	{"lol", POS_CHANT, do_action, 0, 0},
	{"love", POS_CHANT, do_action, 0, 0},
	{"marvelous", POS_CHANT, do_action, 0, 0},
	{"massage", POS_CHANT, do_action, 0, 0},
	{"moan", POS_CHANT, do_action, 0, 0},
	{"moon", POS_STANDING, do_action, 0, 0},
	{"mosh", POS_STANDING, do_action, 0, 0},
	{"muha", POS_CHANT, do_action, 0, 0},
	{"mumble", POS_CHANT, do_action, 0, 0},
	{"nibble", POS_CHANT, do_action, 0, 0},
	{"nih", POS_CHANT, do_action, 0, 0},
	{"nod", POS_CHANT, do_action, 0, 0},
	{"nudge", POS_CHANT, do_action, 0, 0},
	{"nuzzle", POS_CHANT, do_action, 0, 0},
	{"pant", POS_CHANT, do_action, 0, 0},
	{"pat", POS_CHANT, do_action, 0, 0},
	{"peer", POS_CHANT, do_action, 0, 0},
	{"pet", POS_CHANT, do_action, 0, 0},
	{"point", POS_CHANT, do_action, 0, 0},
	{"poke", POS_CHANT, do_action, 0, 0},
	{"ponder", POS_CHANT, do_action, 0, 0},
	{"pounce", POS_STANDING, do_action, 0, 0},
	{"poupon", POS_CHANT, do_action, 0, 0},
	{"pout", POS_CHANT, do_action, 0, 0},
	{"pray", POS_CHANT, do_action, 0, 0},
	{"pretend", POS_CHANT, do_action, 0, 0},
	{"puke", POS_CHANT, do_action, 0, 0},
	{"punch", POS_CHANT, do_action, 0, 0},
	{"purr", POS_CHANT, do_action, 0, 0},
	{"raise", POS_CHANT, do_action, 0, 0},
	{"rofl", POS_CHANT, do_action, 0, 0},
	{"roll", POS_CHANT, do_action, 0, 0},
	{"ruffle", POS_STANDING, do_action, 0, 0},
	{"sad", POS_CHANT, do_action, 0, 0},
	{"salute", POS_STANDING, do_action, 0, 0},
	{"scorn", POS_CHANT, do_action, 0, 0},
	{"scratch", POS_CHANT, do_action, 0, 0},
	{"scream", POS_CHANT, do_action, 0, 0},
	{"scuff", POS_CHANT, do_action, 0, 0},
	{"seduce", POS_CHANT, do_action, 0, 0},
	{"shake", POS_CHANT, do_action, 0, 0},
	{"shame", POS_CHANT, do_action, 0, 0},
	{"shin", POS_CHANT, do_action, 0, 0},
	{"shiver", POS_CHANT, do_action, 0, 0},
	{"shrug", POS_CHANT, do_action, 0, 0},
	{"sigh", POS_CHANT, do_action, 0, 0},
	{"simon", POS_CHANT, do_action, 0, 0},
	{"sing", POS_CHANT, do_action, 0, 0},
	{"slap", POS_CHANT, do_action, 0, 0},
	{"smack", POS_CHANT, do_action, 0, 0},
	{"smirk", POS_CHANT, do_action, 0, 0},
	{"snap", POS_CHANT, do_action, 0, 0},
	{"snarl", POS_CHANT, do_action, 0, 0},
	{"sneeze", POS_CHANT, do_action, 0, 0},
	{"snicker", POS_CHANT, do_action, 0, 0},
	{"sniff", POS_CHANT, do_action, 0, 0},
	{"snore", POS_SLEEPING, do_action, 0, 0},
	{"snort", POS_CHANT, do_action, 0, 0},
	{"snowball", POS_STANDING, do_action, LVL_IMMORT, 0},
	{"snuggle", POS_CHANT, do_action, 0, 0},
	{"sob", POS_CHANT, do_action, 0, 0},
	{"spank", POS_CHANT, do_action, 0, 0},
	{"spit", POS_STANDING, do_action, 0, 0},
	{"squeal", POS_CHANT, do_action, 0, 0},
	{"squeeze", POS_CHANT, do_action, 0, 0},
	{"stamp", POS_CHANT, do_action, 0, 0},
	{"stagger", POS_STANDING, do_action, 0, 0},
	{"stare", POS_CHANT, do_action, 0, 0},
	{"steam", POS_CHANT, do_action, 0, 0},
	{"stroke", POS_CHANT, do_action, 0, 0},
	{"strut", POS_STANDING, do_action, 0, 0},
	{"sulk", POS_CHANT, do_action, 0, 0},
	{"sweat", POS_CHANT, do_action, 0, 0},
	{"swizzle", POS_CHANT, do_action, 0, 0},
	{"tackle", POS_CHANT, do_action, 0, 0},
	{"tag_social", POS_CHANT, do_action, 0, 0},
	{"tap", POS_CHANT, do_action, 0, 0},
	{"tango", POS_STANDING, do_action, 0, 0},
	{"tantrum", POS_CHANT, do_action, 0, 0},
	{"taunt", POS_CHANT, do_action, 0, 0},
	{"thank", POS_CHANT, do_action, 0, 0},
	{"think", POS_CHANT, do_action, 0, 0},
	{"tickle", POS_CHANT, do_action, 0, 0},
	{"timeout", POS_CHANT, do_action, 0, 0},
	{"tongue", POS_CHANT, do_action, 0, 0},
	{"toss", POS_CHANT, do_action, 0, 0},
	{"tug", POS_CHANT, do_action, 0, 0},
	{"twiddle", POS_CHANT, do_action, 0, 0},
	{"typos", POS_CHANT, do_action, 0, 0},
	{"warcry", POS_CHANT, do_action, 0, 0},
	{"wave", POS_CHANT, do_action, 0, 0},
	{"wedgie", POS_CHANT, do_action, 0, 0},
	{"whine", POS_CHANT, do_action, 0, 0},
	{"whistle", POS_CHANT, do_action, 0, 0},
	{"wiggle", POS_STANDING, do_action, 0, 0},
	{"wince", POS_STANDING, do_action, 0, 0},
	{"wink", POS_CHANT, do_action, 0, 0},
	{"wish", POS_STANDING, do_action, 0, 0},
	{"worship", POS_CHANT, do_action, 0, 0},
	{"wth", POS_CHANT, do_action, 0, 0},
	{"yabba", POS_STANDING, do_action, 0, 0},
	{"yawn", POS_CHANT, do_action, 0, 0},
	{"yehaw", POS_STANDING, do_action, 0, 0},
	{"yodel", POS_CHANT, do_action, 0, 0},

	{"\n", 0, 0, 0, 0}
};				/* this must be last */

char *fill[] = {
	"in",
	"from",
	"with",
	"the",
	"on",
	"at",
	"to",
	"\n"
}

;

char *reserved[] = {
	"a",
	"an",
	"self",
	"me",
	"all",
	"room",
	"someone",
	"something",
	"\n"
}

;

/*
 * This is the actual command interpreter called from game_loop() in comm.c
 * It makes sure you are the proper level and position to execute the command,
 * then calls the appropriate function.
 */
void command_interpreter(struct char_data *ch, char *argument)
{
	int cmd, length;
	char *line, *arg;
	struct queue_event *tmpq;

	/* This is "log everything" */
	/*
	   if (!IS_NPC(ch)) {
	   log("%s (%d): %s", GET_NAME(ch), GET_ROOM_VNUM(IN_ROOM(ch)), argument);
	   }
	 */

	if (!IS_NPC(ch))
		REMOVE_BIT(AFF_FLAGS(ch), AFF_HIDE);

	if (AFF2_FLAGGED(ch, AFF2_DIGGING)) {
		REMOVE_BIT(AFF2_FLAGS(ch), AFF2_DIGGING);
		send_to_char(ch, "You stop digging.\r\n");
		for (tmpq = command_queue; tmpq; tmpq = tmpq->next) {
			if (IS_SET(tmpq->flags, QUE_FUNCTION) &&
			    (tmpq->ch == ch) &&
			    (tmpq->function == dig_update)) {
				del_event_queue(tmpq);
				break;
			}
		}

	}
	if (AFF2_FLAGGED(ch, AFF2_SKINNING)) {
		REMOVE_BIT(AFF2_FLAGS(ch), AFF2_SKINNING);
		send_to_char(ch, "You stop skinning.\r\n");
		for (tmpq = command_queue; tmpq; tmpq = tmpq->next) {
			if (IS_SET(tmpq->flags, QUE_FUNCTION) &&
			    (tmpq->ch == ch) &&
			    (tmpq->function == skin_update)) {
				del_event_queue(tmpq);
				break;
			}
		}

	}

	/* just drop to next line for hitting CR */
	skip_spaces(&argument);
	if (!*argument)
		return;

	/*
	 * special case to handle one-character, non-alphanumeric commands;
	 * requested by many people so "'hi" or ";godnet test" is possible.
	 * Patch sent by Eric Green and Stefan Wasilewski.
	 */
	arg = get_buffer(MAX_INPUT_LENGTH);
	if (!isalpha((int)*argument) && !isdigit((int)*argument)) {
		arg[0] = argument[0];
		arg[1] = '\0';
		line = argument + 1;
	} else
		line = any_one_arg(argument, arg);
	/*
	 * If someone just typed a number, then put that number in the argument
	 * and put 'goto' in the command string.
	 */
	if ((GET_LEVEL(ch) >= LVL_IMMORT) && isdigit((int)*arg)) {
		strcpy(line, arg);
		strcpy(arg, "goto");
	}

	/* otherwise, find the command */
	if (GET_LEVEL(ch) < LVL_IMPL) {
		int cont;	/* continue the command checks */
		cont = command_wtrigger(ch, arg, line);
		if (!cont)
			cont += command_mtrigger(ch, arg, line);
		if (!cont)
			cont = command_otrigger(ch, arg, line);
		if (cont) {
			release_buffer(arg);
			return;	/* command trigger took over */
		}
	}

	for (length = strlen(arg), cmd = 0; *cmd_info[cmd].command != '\n';
	     cmd++)
		if (!strncmp(cmd_info[cmd].command, arg, length))
			if (GET_LEVEL(ch) >= cmd_info[cmd].minimum_level)
				break;

	if (!IS_NPC(ch) && PRF2_FLAGGED(ch, PRF2_AFK)
	    && (cmd > 0) /*&&!CMD_IS("afk") */ ) {
		/*
		   send_to_char(ch,"What are you doing!?! You are AFK!\r\n");
		   if (GET_LEVEL(ch)<=LVL_SERP)
		   WAIT_STATE(ch,PULSE_VIOLENCE);
		 */
		send_to_char(ch, "You have returned from AFK.\r\n");
		REMOVE_BIT(PRF2_FLAGS(ch), PRF2_AFK);
	}

	release_buffer(arg);
	if (*cmd_info[cmd].command == '\n')
		send_to_char(ch, "Huh?!?\r\n");
	else if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_FROZEN) &&
		 GET_LEVEL(ch) < LVL_IMPL)
		send_to_char(ch,
			     "You try, but the mind-numbing cold prevents you...\r\n");
	else if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_STUNNED)
		 && GET_LEVEL(ch) < LVL_IMPL)
		send_to_char(ch,
			     "You try, but you are too drained from that last spell...\r\n");
	else if (cmd_info[cmd].command_pointer == NULL)
		send_to_char(ch,
			     "Sorry, that command hasn't been implemented yet.\r\n");
	else if (IS_NPC(ch) && cmd_info[cmd].minimum_level >= LVL_IMMORT)
		send_to_char(ch,
			     "You can't use immortal commands while switched.\r\n");
	else if (GET_POS(ch) < cmd_info[cmd].minimum_position)
		switch (GET_POS(ch)) {
		case POS_DEAD:
			send_to_char(ch, "Lie still; you are DEAD!!! :-(\r\n");
			break;
		case POS_INCAP:
		case POS_MORTALLYW:
			send_to_char(ch,
				     "You are in a pretty bad shape, unable to do anything!\r\n");
			break;
		case POS_STUNNED:
			send_to_char(ch,
				     "All you can do right now is think about the stars!\r\n");
			break;
		case POS_SLEEPING:
			send_to_char(ch, "In your dreams, or what?\r\n");
			break;
		case POS_RESTING:
			send_to_char(ch,
				     "Nah... You feel too relaxed to do that..\r\n");
			break;
		case POS_SITTING:
			send_to_char(ch,
				     "Maybe you should get on your feet first?\r\n");
			break;
		case POS_FIGHTING:
			send_to_char(ch,
				     "No way!  You're fighting for your life!\r\n");
			break;
		case POS_CHANT:
			send_to_char(ch, "You are too busy chanting!\r\n");
			break;
		case POS_MEDITATE:
			send_to_char(ch,
				     "You are too deep in your trance!\r\n");
			break;
		case POS_BANDAGE:
			send_to_char(ch,
				     "Your bandages restrict your movements "
				     "too much!\r\n");
			break;
	} else {

		sprintf(last_command, "[%5ld] %s in [%5ld] %s:\n %s%s",
			IS_NPC(ch) ? GET_MOB_VNUM(ch) : GET_IDNUM(ch),
			GET_NAME(ch),
			IN_ROOM(ch) ? GET_ROOM_VNUM(IN_ROOM(ch)) : 0,
			IN_ROOM(ch) ? world[IN_ROOM(ch)].
			name : "(not in a room)", cmd_info[cmd].command, line);

		if (no_specials || !special(ch, cmd, line))
			((*cmd_info[cmd].command_pointer) (ch, line, cmd,
							   cmd_info[cmd].
							   subcmd));
		strcat(last_command, " (Finished)");
	}
}

/**************************************************************************
 * Routines to handle aliasing                                             *
  **************************************************************************/

struct alias_data *find_alias(struct alias_data *alias_list, char *str)
{
	while (alias_list != NULL) {
		if (*str == *alias_list->alias)	/* hey, every little bit counts :-) */
			if (!strcmp(str, alias_list->alias))
				return alias_list;

		alias_list = alias_list->next;
	}

	return NULL;
}

void free_alias(struct alias_data *a)
{
	if (a->alias)
		free(a->alias);
	if (a->replacement)
		free(a->replacement);
	free(a);
}

/* The interface to the outside world: do_alias */
ACMD(do_alias)
{
	char *repl, *arg;
	struct alias_data *a, *temp;

	if (IS_NPC(ch))
		return;

	arg = get_buffer(MAX_INPUT_LENGTH);
	repl = any_one_arg(argument, arg);

	if (!*arg) {
		/* no argument specified -- list currently defined aliases */
		send_to_char(ch, "Currently defined aliases:\r\n");
		if ((a = GET_ALIASES(ch)) == NULL)
			send_to_char(ch, " None.\r\n");
		else {
			while (a != NULL) {
				send_to_char(ch, "%-15s %s\r\n", a->alias,
					     a->replacement);
				a = a->next;
			}
		}
	} else {
		/* otherwise, add or remove aliases */
		/* is this an alias we've already defined? */
		if ((a = find_alias(GET_ALIASES(ch), arg)) != NULL) {
			REMOVE_FROM_LIST(a, GET_ALIASES(ch), next);
			free_alias(a);
		}
		/* if no replacement string is specified, assume we want to delete */
		if (!*repl) {
			if (a == NULL)
				send_to_char(ch, "No such alias.\r\n");
			else
				send_to_char(ch, "Alias deleted.\r\n");
		} else if (!str_cmp(arg, "alias")) {
			send_to_char(ch, "You can't alias 'alias'.\r\n");
		} else {
			CREATE(a, struct alias_data, 1);
			a->alias = str_dup(arg);
			delete_doubledollar(repl);
			a->replacement = str_dup(repl);
			if (strchr(repl, ALIAS_SEP_CHAR)
			    || strchr(repl, ALIAS_VAR_CHAR))
				a->type = ALIAS_COMPLEX;
			else
				a->type = ALIAS_SIMPLE;
			a->next = GET_ALIASES(ch);
			GET_ALIASES(ch) = a;
			send_to_char(ch, "Alias added.\r\n");
		}
	}
	write_aliases(ch);	/* so that crash doesn't wipe changes */
	release_buffer(arg);
}

/*
 * Valid numeric replacements are only $1 .. $9 (makes parsing a little
 * easier, and it's not that much of a limitation anyway.)  Also valid
 * is "$*", which stands for the entire original line after the alias.
 * ";" is used to delimit commands.
 */
#define NUM_TOKENS       9

void perform_complex_alias(struct txt_q *input_q, char *orig,
			   struct alias_data *a)
{
	struct txt_q temp_queue;
	char *tokens[NUM_TOKENS], *temp, *write_point,
	    *buf2 = get_buffer(MAX_STRING_LENGTH),
	    *buf = get_buffer(MAX_STRING_LENGTH);

	int num_of_tokens = 0, num;

	/* First, parse the original string */
	temp = strtok(strcpy(buf2, orig), " ");
	while (temp != NULL && num_of_tokens < NUM_TOKENS) {
		tokens[num_of_tokens++] = temp;
		temp = strtok(NULL, " ");
	}

	/* initialize */
	write_point = buf;
	temp_queue.head = temp_queue.tail = NULL;

	/* now parse the alias */
	for (temp = a->replacement; *temp; temp++) {
		if (*temp == ALIAS_SEP_CHAR) {
			*write_point = '\0';
			buf[MAX_INPUT_LENGTH - 1] = '\0';
			write_to_q(buf, &temp_queue, 1);
			write_point = buf;
		} else if (*temp == ALIAS_VAR_CHAR) {
			temp++;
			if ((num = *temp - '1') < num_of_tokens && num >= 0) {
				strcpy(write_point, tokens[num]);
				write_point += strlen(tokens[num]);
			} else if (*temp == ALIAS_GLOB_CHAR) {
				strcpy(write_point, orig);
				write_point += strlen(orig);
			} else if ((*(write_point++) = *temp) == '$')	/* redouble $ for act safety */
				*(write_point++) = '$';
		} else
			*(write_point++) = *temp;
	}

	*write_point = '\0';
	buf[MAX_INPUT_LENGTH - 1] = '\0';
	write_to_q(buf, &temp_queue, 1);

	/* push our temp_queue on to the _front_ of the input queue */
	if (input_q->head == NULL)
		*input_q = temp_queue;
	else {
		temp_queue.tail->next = input_q->head;
		input_q->head = temp_queue.head;
	}
	release_buffer(buf);
	release_buffer(buf2);
}

/*
 * Given a character and a string, perform alias replacement on it.
 *
 * Return values:
 *   0: String was modified in place; call command_interpreter immediately.
 *   1: String was _not_ modified in place; rather, the expanded aliases
 *      have been placed at the front of the character's input queue.
 */
int perform_alias(struct descriptor_data *d, char *orig)
{
	char *first_arg, *ptr;
	struct alias_data *a, *tmp;

	if (!d || !d->character)
		return 0;

	/* Mobs don't have aliases. */
	if (IS_NPC(d->character))
		return 0;

	/* bail out immediately if the guy doesn't have any aliases */
	if ((tmp = GET_ALIASES(d->character)) == NULL)
		return 0;

	/* find the alias we're supposed to match */
	first_arg = get_buffer(MAX_INPUT_LENGTH);
	ptr = any_one_arg(orig, first_arg);

	/* bail out if it's null */
	if (!*first_arg) {
		release_buffer(first_arg);
		return 0;
	}

	/* if the first arg is not an alias, return without doing anything */
	if ((a = find_alias(tmp, first_arg)) == NULL) {
		release_buffer(first_arg);
		return 0;
	}

	release_buffer(first_arg);
	if (a->type == ALIAS_SIMPLE) {
		strcpy(orig, a->replacement);
		return 0;
	} else {
		perform_complex_alias(&d->input, ptr, a);
		return 1;
	}
}

/***************************************************************************
 * Various other parsing utilities                                         *
 **************************************************************************/

/*
 * searches an array of strings for a target string.  "exact" can be
 * 0 or non-0, depending on whether or not the match must be exact for
 * it to be returned.  Returns -1 if not found; 0..n otherwise.  Array
 * must be terminated with a '\n' so it knows to stop searching.
 */
int search_block(char *arg, char **list, int exact)
{
	register int i, l;

	if (*arg == '!')
		return -1;

	/* Make into lower case, and get length of string */
	for (l = 0; *(arg + l); l++)
		*(arg + l) = LOWER(*(arg + l));

	if (exact) {
		for (i = 0; **(list + i) != '\n'; i++) {
			if (!str_cmp(arg, *(list + i)))
				return (i);
		}
	} else {
		if (!l)
			l = 1;	/* Avoid "" to match the first available
				 * string */
		for (i = 0; **(list + i) != '\n'; i++) {
			if (!strn_cmp(arg, *(list + i), l))
				return (i);
		}
	}

	return -1;
}

int is_number(char *str)
{
	if (!str || !*str)
		return FALSE;
	if (*str == '-')
		str++;

	while (*str)
		if (!isdigit((int)*(str++)))
			return FALSE;

	return TRUE;
}

void skip_spaces(char **string)
{
	for (; **string && isspace((int)**string); (*string)++) ;
}

char *delete_doubledollar(char *string)
{
	char *sread, *swrite;

	if ((swrite = strchr(string, '$')) == NULL)
		return string;

	sread = swrite;

	while (*sread)
		if ((*(swrite++) = *(sread++)) == '$')
			if (*sread == '$')
				sread++;

	*swrite = '\0';

	return string;
}

int fill_word(char *argument)
{
	return (search_block(argument, fill, TRUE) >= 0);
}

int reserved_word(char *argument)
{
	return (search_block(argument, reserved, TRUE) >= 0);
}

/*
 * copy the first non-fill-word, space-delimited argument of 'argument'
 * to 'first_arg'; return a pointer to the remainder of the string.
 */
char *one_argument(char *argument, char *first_arg)
{
	if (!argument) {
		log("SYSERR: one_argument received a NULL pointer.");
		*first_arg = '\0';
		return NULL;
	}

	do {
		skip_spaces(&argument);

		size_t ii;

		for (ii = 0; ii < strlen(argument); ii++) {
			if (isspace((int)argument[ii])) break;

			first_arg[ii] = LOWER(argument[ii]);
		}

		argument += ii;

		first_arg[ii] = '\0';
	}
	while (fill_word(first_arg));

	return argument;
}

/*
 * one_word is like one_argument, except that words in quotes ("") are
 * considered one word.
 */
char *one_word(char *argument, char *first_arg)
{
	char *begin = first_arg;

	do {
		skip_spaces(&argument);

		first_arg = begin;

		if (*argument == '\"') {
			argument++;
			while (*argument && *argument != '\"') {
				*(first_arg++) = LOWER(*argument);
				argument++;
			}
			argument++;
		} else {
			while (*argument && !isspace((int)*argument)) {
				*(first_arg++) = LOWER(*argument);
				argument++;
			}
		}

		*first_arg = '\0';
	}
	while (fill_word(begin));

	return argument;
}

/* same as one_argument except that it doesn't ignore fill words */
char *any_one_arg(char *argument, char *first_arg)
{
	skip_spaces(&argument);

	while (*argument && !isspace((int)*argument)) {
		*(first_arg++) = LOWER(*argument);
		argument++;
	}

	*first_arg = '\0';

	return argument;
}

/*
 * Same as one_argument except that it takes two args and returns the rest;
 * ignores fill words
 */
char *two_arguments(char *argument, char *first_arg, char *second_arg)
{
	return one_argument(one_argument(argument, first_arg), second_arg);	/* :-) */
}

char *five_arguments(char *argument, char *first_arg, char *second_arg,
		     char *third_arg, char *fourth_arg, char *fifth_arg)
{
	return
	    one_argument(
			one_argument(
				one_argument(
					one_argument(
						one_argument(
							argument, 
							first_arg
						), 
						second_arg
					), third_arg
				), fourth_arg
			), fifth_arg
		);

}

/*
 * determine if a given string is an abbreviation of another
 * (now works symmetrically -- JE 7/25/94)
 *
 * that was dumb.  it shouldn't be symmetrical.  JE 5/1/95
 *
 * returns 1 if arg1 is an abbreviation of arg2
 */
int is_abbrev(char *arg1, const char *arg2)
{
	if (!*arg1)
		return 0;

	for (; *arg1 && *arg2; arg1++, arg2++)
		if (LOWER(*arg1) != LOWER(*arg2))
			return 0;

	if (!*arg1)
		return 1;
	else
		return 0;
}

int is_abbrevc(const char *arg1, const char *arg2)
{
	if (!*arg1)
		return 0;

	for (; *arg1 && *arg2; arg1++, arg2++)
		if (LOWER(*arg1) != LOWER(*arg2))
			return 0;

	if (!*arg1)
		return 1;
	else
		return 0;
}

/* return first space-delimited token in arg1; remainder of string in arg2 */
void half_chop(char *string, char *arg1, char *arg2)
{
	char *temp;

	temp = any_one_arg(string, arg1);
	skip_spaces(&temp);
	strcpy(arg2, temp);
}

/* Used in specprocs, mostly.  (Exactly) matches "command" to cmd number */
int find_command(char *command)
{
	int cmd;

	for (cmd = 0; *cmd_info[cmd].command != '\n'; cmd++)
		if (!strcmp(cmd_info[cmd].command, command))
			return cmd;

	return -1;
}

int special(struct char_data *ch, int cmd, char *arg)
{
	register struct obj_data *i;
	register struct char_data *k;
	int j;

	/* special in room? */
	if (GET_ROOM_SPEC(IN_ROOM(ch)) != NULL)
		if (GET_ROOM_SPEC(IN_ROOM(ch))
		    (ch, world + IN_ROOM(ch), cmd, arg))
			return 1;

	/* special in equipment list? */
	for (j = 0; j < NUM_WEARS; j++)
		if (GET_EQ(ch, j) && GET_OBJ_SPEC(GET_EQ(ch, j)) != NULL)
			if (GET_OBJ_SPEC(GET_EQ(ch, j))
			    (ch, GET_EQ(ch, j), cmd, arg))
				return 1;

	/* special in inventory? */
	for (i = ch->carrying; i; i = i->next_content) {
		if (GET_OBJ_SPEC(i) != NULL)
			if (GET_OBJ_SPEC(i) (ch, i, cmd, arg))
				return 1;
	}
	/* special in mobile present? */
	for (k = world[IN_ROOM(ch)].people; k; k = k->next_in_room)
		if (GET_MOB_SPEC(k) != NULL) {
			if (GET_MOB_SPEC(k) (ch, k, cmd, arg))
				return 1;
		}
	/* special in object present? */
	for (i = world[IN_ROOM(ch)].contents; i; i = i->next_content)
		if (GET_OBJ_SPEC(i) != NULL)
			if (GET_OBJ_SPEC(i) (ch, i, cmd, arg))
				return 1;

	return 0;
}

/* *************************************************************************
*  Stuff for controlling the non-playing sockets (get name, pwd etc)       *
************************************************************************* */

/* locate entry in p_table with entry->name == name. -1 mrks failed search */
int find_name(char *name)
{
	int i;

	for (i = 0; i <= top_of_p_table; i++) {
		if (!str_cmp((player_table + i)->name, name))
			return i;
	}

	return -1;
}

/* locate entry in p_table with entry->id == id. -1 mrks failed search */
int find_id(long id)
{
	int i;

	for (i = 0; i <= top_of_p_table; i++) {
		if ((player_table + i)->id == id)
			return i;
	}

	return -1;
}

/* locate entry in p_table with entry->id == id. -1 mrks failed search */
char *find_name_by_id(long id)
{
	int i;

	for (i = 0; i <= top_of_p_table; i++) {
		if ((player_table + i)->id == id)
			return (player_table + i)->name;
	}

	return NULL;
}

int _parse_name(char *arg, char *name)
{
	int i;

	/* skip whitespaces */
	for (; isspace((int)*arg); arg++) ;

	for (i = 0; (*name = *arg); arg++, i++, name++)
		if (!isalpha((int)*arg))
			return 1;

	if (!i)
		return 1;

	return 0;
}

#define RECON  1
#define USURP  2
#define UNSWITCH 3
#define LINKED 4

int perform_dupe_check(struct descriptor_data *d)
{
	struct descriptor_data *k, *next_k;
	struct char_data *target = NULL, *ch, *next_ch;
	int mode = 0;

	int id = GET_IDNUM(d->character);

	/*
	 * Now that this descriptor has successfully logged in, disconnect all
	 * other descriptors controlling a character with the same ID number.
	 */

	for (k = descriptor_list; k; k = next_k) {
		next_k = k->next;

		if (k == d)
			continue;

		if (k->original && (GET_IDNUM(k->original) == id)) {
			/* switched char */
			SEND_TO_Q(k,
				  "\r\nMultiple login detected -- disconnecting.\r\n");
			STATE(k) = CON_CLOSE;
			if (!target) {
				target = k->original;
				mode = UNSWITCH;
			}
			if (k->character)
				k->character->desc = NULL;
			k->character = NULL;
			k->original = NULL;
		} else if (k->character && (GET_IDNUM(k->character) == id)) {
			if (!target && STATE(k) == CON_PLAYING) {
				SEND_TO_Q(k,
					  "\r\nThis body has been usurped!\r\n");
				target = k->character;
				mode = USURP;
			}
			k->character->desc = NULL;
			k->character = NULL;
			k->original = NULL;
			SEND_TO_Q(k,
				  "\r\nMultiple login detected -- disconnecting.\r\n");
			STATE(k) = CON_CLOSE;
		}
	}

	/*
	 * now, go through the character list, deleting all characters that
	 * are not already marked for deletion from the above step (i.e., in the
	 * CON_HANGUP state), and have not already been selected as a target for
	 * switching into.  In addition, if we haven't already found a target,
	 * choose one if one is available (while still deleting the other
	 * duplicates, though theoretically none should be able to exist).
	 */

	for (ch = character_list; ch; ch = next_ch) {
		next_ch = ch->next;

		if (IS_NPC(ch))
			continue;
		if (GET_IDNUM(ch) != id)
			continue;

		/* ignore chars with descriptors (already handled by above step) */
		if (ch->desc)
			continue;

		/* don't extract the target char we've found one already */
		if (ch == target)
			continue;

		/* we don't already have a target and found a candidate for switching */
		if (!target) {
			target = ch;
			if (PLR_FLAGGED(ch, PLR_LINKLOADED)) {
				/* send no message to linkloaded player */
				STATE(d) = CON_CLOSE;
				mudlogf(NRM, LVL_ADMIN, TRUE,
					"Connection attempted on link-loaded character %s "
					"from %s", GET_PC_NAME(d->character),
					d->host);
				mode = LINKED;
			} else
				mode = RECON;
			continue;
		}

		/* we've found a duplicate - blow him away, dumping his eq in limbo. */
		if (IN_ROOM(ch) != NOWHERE)
			char_from_room(ch);
		char_to_room(ch, 1);
		extract_char(ch);
	}

	/* no target for switching into was found - allow login to continue */
	if (!target)
		return 0;

	/* if linkloaded don't let them in */
	if (mode == LINKED)
		return 1;

	/* Okay, we've found a target.  Connect d to target. */
	if (IN_ROOM(d->character) != NOWHERE)
		char_from_room(d->character);
	free_char(d->character);	/* get rid of the old char */
	d->character = target;
	d->character->desc = d;
	d->original = NULL;
	d->character->char_specials.timer = 0;
	REMOVE_BIT(PLR_FLAGS(d->character), PLR_MAILING | PLR_WRITING);
	/*    REMOVE_BIT(AFF_FLAGS(d->character), AFF_GROUP); */
	STATE(d) = CON_PLAYING;
	switch (mode) {
	case RECON:
        /* Send players home that try to camp a zone - Nomikos 10/22/2025 */
        if ((GET_LEVEL(d->character) < LVL_IMMORT) &&
            Z_FLAGGED(GET_WAS_IN(d->character), Z_IDLE)) 
            {
            char_from_room(d->character);
            char_to_room(d->character, real_room(GET_HOME(d->character)));
            }
		SEND_TO_Q(d, "Reconnecting.\r\n");
		act("$n has reconnected.", TRUE, d->character, 0, 0, TO_ROOM);
		mudlogf(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE,
			"%s [%s] has reconnected.", GET_PC_NAME(d->character),
			d->host);
		if (GET_LEVEL(d->character) >= LVL_IMMORT
		    && has_mail(GET_NAME(d->character)))
			send_to_char(d->character,
				     "You have mail waiting.\r\n");
		break;
	case USURP:
		SEND_TO_Q(d,
			  "You take over your own body, already in use!\r\n");
		act("$n suddenly keels over in pain, surrounded by a white aura...\r\n" "$n's body has been taken over by a new spirit!", TRUE, d->character, 0, 0, TO_ROOM);
		mudlogf(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE,
			"%s has re-logged in ... disconnecting old socket.",
			GET_PC_NAME(d->character));
		break;
	case UNSWITCH:
		SEND_TO_Q(d, "Reconnecting to unswitched char.");
		mudlogf(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE,
			"%s [%s] has reconnected.", GET_PC_NAME(d->character),
			d->host);
		break;
	}

	return 1;
}

#define MAX_NEWBIE_RACES 11
#define MAX_NEWBIE_CLASSES 12

/* deal with newcomers and other non-playing sockets */
void nanny(struct descriptor_data *d, char *argu)
{
	int player_i, load_result;
	char *buf, *tmp_name;
	struct char_file_u tmp_store;
	room_rnum load_room;
	int i;			/* for looping through classes to list valid ones */
	int j, k;
	int table_pos;
	struct descriptor_data *fd;

	skip_spaces(&argu);

	switch (STATE(d)) {

		/*. OLC states . */
	case CON_OEDIT:
		oedit_parse(d, argu);
		break;
	case CON_REDIT:
		redit_parse(d, argu);
		break;
	case CON_ZEDIT:
		zedit_parse(d, argu);
		break;
	case CON_MEDIT:
		medit_parse(d, argu);
		break;
	case CON_SEDIT:
		sedit_parse(d, argu);
		break;
	case CON_GEDIT:
		gedit_parse(d, argu);
		break;
	case CON_HEDIT:
		hedit_parse(d, argu);
		break;
	case CON_PEDIT:
		/*
		 * pedit_parse(d, argu);
		 */
		break;
	case CON_TRIGEDIT:
		trigedit_parse(d, argu);
		break;
	case CON_ASSEDIT:
		assedit_parse(d, argu);
		break;
		/*. End of OLC states . */

	case CON_GET_NAME:	/* wait for input of name */
		if (d->character == NULL) {
			CREATE(d->character, struct char_data, 1);
			clear_char(d->character);
			CREATE(d->character->player_specials,
			       struct player_special_data, 1);
			d->character->desc = d;
		}
		if (!*argu)
			STATE(d) = CON_CLOSE;
		else {
			buf = get_buffer(128);
			tmp_name = get_buffer(MAX_INPUT_LENGTH);
			if ((_parse_name(argu, tmp_name))
			    || strlen(tmp_name) < 2
			    || strlen(tmp_name) > MAX_NAME_LENGTH
			    || fill_word(strcpy(buf, tmp_name))
			    || reserved_word(buf)) {
				SEND_TO_Q(d,
					  "Invalid name, please try another.\r\n"
					  "Name: ");
				release_buffer(buf);
				release_buffer(tmp_name);
				return;
			}
			if ((player_i = load_char(tmp_name, &tmp_store)) > -1) {
				store_to_char(&tmp_store, d->character);

				GET_PFILEPOS(d->character) = player_i;

				if (PLR_FLAGGED(d->character, PLR_DELETED)) {
					free_char(d->character);
					for (fd = descriptor_list; fd;
					     fd = fd->next) {
						if (fd == d)
							continue;
						if ((fd->character)
						    && (fd->character->player.
							name))
							if (!str_cmp
							    (tmp_name,
							     fd->character->
							     player.name)) {
								SEND_TO_Q(d,
									  "Name already in use, please try another.\r\n");
								SEND_TO_Q(d,
									  "Name: ");
								release_buffer
								    (buf);
								release_buffer
								    (tmp_name);
								return;
							}
					}
					CREATE(d->character, struct char_data,
					       1);
					clear_char(d->character);
					CREATE(d->character->player_specials,
					       struct player_special_data, 1);
					d->character->desc = d;
					CREATE(d->character->player.name, char,
					       strlen(tmp_name) + 1);
					strcpy(d->character->player.name,
					       CAP(tmp_name));
					SEND_TO_Q(d, "%s", NAME_POLICY);
					GET_PFILEPOS(d->character) = player_i;
					SEND_TO_Q(d,
						  "Did I get that right, %s (Y/N)? ",
						  tmp_name);
					STATE(d) = CON_NAME_CNFRM;
				} else {
					/* undo it just in case they are set */
					REMOVE_BIT(PLR_FLAGS(d->character),
						   PLR_WRITING | PLR_MAILING |
						   PLR_CRYO | PLR_STUNNED |
						   PLR_FISHING | PLR_FISH_ON);
					REMOVE_BIT(PRF2_FLAGS(d->character),
						   PRF2_AFK);
					/*      REMOVE_BIT(AFF_FLAGS(d->character), AFF_GROUP); */

					SEND_TO_Q(d, "Password: ");
					echo_off(d);
					d->idle_tics = 0;
					STATE(d) = CON_PASSWORD;
				}
			} else {
				/* player unknown -- make new character */

				if (!Valid_Name(tmp_name)) {
					SEND_TO_Q(d,
						  "Invalid name, please try another.\r\n");
					SEND_TO_Q(d, "Name: ");
					release_buffer(buf);
					release_buffer(tmp_name);
					return;
				}
				for (fd = descriptor_list; fd; fd = fd->next) {
					if (fd == d)
						continue;
					if ((fd->character)
					    && (fd->character->player.name))
						if (!strcasecmp
						    (tmp_name,
						     fd->character->player.
						     name)) {
							SEND_TO_Q(d,
								  "Name already in use, please try another.\r\n");
							SEND_TO_Q(d, "Name: ");
							release_buffer(buf);
							release_buffer
							    (tmp_name);
							return;
						}
				}
				CREATE(d->character->player.name, char,
				       strlen(tmp_name) + 1);
				strcpy(d->character->player.name,
				       CAP(tmp_name));
				SEND_TO_Q(d, "%s", NAME_POLICY);
				SEND_TO_Q(d, "Did I get that right, %s (Y/N)? ",
					  tmp_name);
				STATE(d) = CON_NAME_CNFRM;
			}
			release_buffer(buf);
			release_buffer(tmp_name);
		}
		break;
	case CON_NAME_CNFRM:	/* wait for conf. of new name    */
		if (UPPER(*argu) == 'Y') {
			if (isbanned(d->host) >= BAN_NEW) {
				mudlogf(NRM, LVL_DGOD, TRUE,
					"Site-Ban: Request for new char %s denied from [%s]",
					GET_PC_NAME(d->character), d->host);
				SEND_TO_Q(d,
					  "Sorry, new characters are not allowed from your site!\r\n");
				STATE(d) = CON_CLOSE;
				return;
			}
			if (circle_restrict) {
				SEND_TO_Q(d,
					  "Sorry, new players can't be created at the moment.\r\n");
				mudlogf(NRM, LVL_DGOD, TRUE,
					"Request for new char %s denied from %s (wizlock)",
					GET_PC_NAME(d->character), d->host);
				STATE(d) = CON_CLOSE;
				return;
			}
			SEND_TO_Q(d, "New character.\r\n");

			SEND_TO_Q(d,
				  "\r\nIf you are new to PhoenixMud, we suggest using the most common home town\r\n");
			SEND_TO_Q(d,
				  "and the recommended stats for your race and class.\r\n");
			SEND_TO_Q(d, "\r\nAre you new to PhoenixMud (Y/N)? ");
			STATE(d) = CON_FIRST_TIME;
		} else if (*argu == 'n' || *argu == 'N') {
			SEND_TO_Q(d, "Okay, what IS it, then? ");
			free(d->character->player.name);
			d->character->player.name = NULL;
			STATE(d) = CON_GET_NAME;
		} else {
			SEND_TO_Q(d, "Please type Yes or No: ");
		}
		break;

	case CON_FIRST_TIME:
		switch (toupper(*argu)) {
		case 'Y':
			SEND_TO_Q(d,
				  "\r\nWelcome to PhoenixMud!  We hope you enjoy our world.\r\n\r\n");
			d->first_time = 1;
			break;
		case 'N':
			SEND_TO_Q(d, "\r\nWelcome back!\r\n\r\n");
			d->first_time = 0;
			break;
		default:
			SEND_TO_Q(d, "Please type Yes or No: ");
			return;
		}
		SEND_TO_Q(d, "Give me a password for %s: ",
			  GET_PC_NAME(d->character));
		echo_off(d);
		STATE(d) = CON_NEWPASSWD;
		return;

	case CON_PASSWORD:	/* get pwd for known player      */
		/*
		 * To really prevent duping correctly, the player's record should
		 * be reloaded from disk at this point (after the password has been
		 * typed).  However I'm afraid that trying to load a character over
		 * an already loaded character is going to cause some problem down the
		 * road that I can't see at the moment.  So to compensate, I'm going to
		 * (1) add a 15 or 20-second time limit for entering a password, and (2)
		 * re-add the code to cut off duplicates when a player quits.  JE 6 Feb 96
		 */

		echo_on(d);	/* turn echo back on */

		if (!*argu)
			STATE(d) = CON_CLOSE;
		else {
			if (strncmp
			    (CRYPT(argu, GET_PASSWD(d->character)),
			     GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
				mudlogf(BRF,
					MAX(LVL_DGOD, GET_LEVEL(d->character)),
					TRUE, "Bad PW: %s [%s]",
					GET_PC_NAME(d->character), d->host);
				GET_BAD_PWS(d->character)++;
				if ((load_room =
				     GET_LOADROOM(d->character)) != NOWHERE)
					load_room = real_room(load_room);
				else
					load_room = NOWHERE;

				save_char_no_logon(d->character, load_room);

				if (++(d->bad_pws) >= max_bad_pws) {
					/* 3 strikes and you're out. */
					SEND_TO_Q(d,
						  "Wrong password... disconnecting.\r\n");
					STATE(d) = CON_CLOSE;
				} else {
					SEND_TO_Q(d,
						  "Wrong password.\r\nPassword: ");
					echo_off(d);
				}
				return;
			}
			load_result = GET_BAD_PWS(d->character);
			GET_BAD_PWS(d->character) = 0;
			d->bad_pws = 0;

			if (isbanned(d->host) == BAN_SELECT &&
			    !PLR_FLAGGED(d->character, PLR_SITEOK)) {
				SEND_TO_Q(d,
					  "Sorry, this char has not been cleared for login from your site!\r\n");
				STATE(d) = CON_CLOSE;
				mudlogf(NRM, LVL_DGOD, TRUE,
					"Connection attempt for %s denied from %s",
					GET_PC_NAME(d->character), d->host);
				return;
			}

			if (PLR_FLAGGED(d->character, PLR_LINKLOADED)) {
				SEND_TO_Q(d,
					  "Sorry, this character is being link-loaded by an Implementor. Please log in again in a few minutes.\r\n");
				STATE(d) = CON_CLOSE;
				mudlogf(NRM, LVL_DGOD, TRUE,
					"Connection attempted on link-loaded character %s "
					"from %s", GET_PC_NAME(d->character),
					d->host);
				return;
			}

			if (GET_LEVEL(d->character) < circle_restrict) {
				SEND_TO_Q(d,
					  "The game is temporarily restricted.. try again later.\r\n");
				STATE(d) = CON_CLOSE;
				mudlogf(NRM, LVL_DGOD, TRUE,
					"Request for login denied for %s [%s] (wizlock)",
					GET_PC_NAME(d->character), d->host);
				return;
			}
			/* check and make sure no other copies of this player are logged in */
			if (perform_dupe_check(d))
				return;

			if (GET_LEVEL(d->character) >= LVL_IMMORT)
				SEND_TO_Q(d, "%s", imotd);
			else
				SEND_TO_Q(d, "%s", motd);

			if (PLR_FLAGGED(d->character, PLR_INVSTART))
				GET_INVIS_LEV(d->character) =
				    GET_LEVEL(d->character);

			mudlogf(BRF,
				MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)),
				TRUE, "%s [%s] has connected.",
				GET_PC_NAME(d->character), d->host);

			if (load_result) {
				SEND_TO_Q(d, "\r\n\r\n\007\007\007"
					  "%s%d LOGIN FAILURE%s SINCE LAST SUCCESSFUL LOGIN.%s\r\n",
					  CCRED(d->character, C_SPR),
					  load_result,
					  (load_result > 1) ? "S" : "",
					  CCNRM(d->character, C_SPR));
				GET_BAD_PWS(d->character) = 0;
			}
			SEND_TO_Q(d, "\r\n\n*** PRESS RETURN: ");
			STATE(d) = CON_RMOTD;
		}
		break;

	case CON_NEWPASSWD:
	case CON_CHPWD_GETNEW:
		if (!*argu || strlen(argu) > MAX_PWD_LENGTH || strlen(argu) < 3
		    || !str_cmp(argu, GET_PC_NAME(d->character))) {
			SEND_TO_Q(d, "\r\nIllegal password.\r\n");
			SEND_TO_Q(d, "Password: ");
			return;
		}
		strncpy(GET_PASSWD(d->character),
			CRYPT(argu, GET_PC_NAME(d->character)), MAX_PWD_LENGTH);
		*(GET_PASSWD(d->character) + MAX_PWD_LENGTH) = '\0';

		SEND_TO_Q(d, "\r\nPlease retype password: ");
		if (STATE(d) == CON_NEWPASSWD)
			STATE(d) = CON_CNFPASSWD;
		else
			STATE(d) = CON_CHPWD_VRFY;

		break;

	case CON_CNFPASSWD:
	case CON_CHPWD_VRFY:
		if (strncmp
		    (CRYPT(argu, GET_PASSWD(d->character)),
		     GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
			SEND_TO_Q(d,
				  "\r\nPasswords don't match... start over.\r\n");
			SEND_TO_Q(d, "Password: ");
			if (STATE(d) == CON_CNFPASSWD)
				STATE(d) = CON_NEWPASSWD;
			else
				STATE(d) = CON_CHPWD_GETNEW;
			return;
		}
		echo_on(d);

		if (STATE(d) == CON_CNFPASSWD) {
			SEND_TO_Q(d, "What is your sex (M/F)? ");
			STATE(d) = CON_QSEX;
		} else {
			save_char(d->character, NOWHERE);
			echo_on(d);
			SEND_TO_Q(d, "\r\nDone.\r\n");
			/* clear the screen */
			SEND_TO_Q(d, "\033[H\033[J\e[?7h\e[r\x1B[1m\x1B[36m");
			if (port != 4999)
				SEND_TO_Q(d, "%s", MENU);
			STATE(d) = CON_MENU;
		}

		break;

	case CON_QSEX:		/* query sex of new user         */
		switch (*argu) {
		case 'm':
		case 'M':
			d->character->player.sex = SEX_MALE;
			break;
		case 'f':
		case 'F':
			d->character->player.sex = SEX_FEMALE;
			break;
		default:
			SEND_TO_Q(d, "That is not a sex..\r\n"
				  "What IS your sex? ");
			return;
			break;
		}

		/* 10/27/96, Echo - race menu added. See race.c for details. */
		SEND_TO_Q(d, "%s", race_menu);
		SEND_TO_Q(d, "%s", race_prompt);
		STATE(d) = CON_QRACE;
		break;

      /** Anduin race selection **/

	case CON_QRACE:
		load_result = parse_race_for_menu(*argu);
		if (load_result == RACE_UNDEFINED) {
			if (*argu == '?') {
				struct help_index_element *this_help;
				int r;
				char *temp;
				char *help_choice =
				    get_buffer(MAX_STRING_LENGTH);
				argu = any_one_arg(argu, help_choice);
				argu = any_one_arg(argu, help_choice);
				for (r = 0; r < MAX_NEWBIE_RACES; r++) {
					temp = pc_race_types[r];
					if (isname(help_choice, temp)) {
						if (!
						    (this_help =
						     find_help(temp, 1)))
							SEND_TO_Q(d,
								  "\r\nThat's not a race.");
						else
							SEND_TO_Q(d, "%s", this_help->entry);
						break;
					}
				}
				release_buffer(help_choice);
			} else
				SEND_TO_Q(d, "\r\nThat's not a race.");
			SEND_TO_Q(d, "%s", race_menu);
			SEND_TO_Q(d, "%s", race_prompt);
			return;
		} else
			GET_RACE(d->character) = load_result;

		/* 10/27/96, Echo - The following class menu lists only those classes
		 *   which are allowed based on the race the character has chosen.
		 */
		SEND_TO_Q(d, "%s", class_menu_header);
		for (i = 0; i < CLASS_KENSAI; i++)
			if (LEGAL_CLASS[(int)GET_RACE(d->character)][i])
				SEND_TO_Q(d, "%s", class_menu_choices[i]);
		SEND_TO_Q(d, "   or [q] to go back to the race menu.\r\n");
		/* 10/27/96, Echo - removed default class menu in favor of above,
		 *   which just lists available classes for the selected race.
		 * SEND_TO_Q(class_menu, d);
		 */
		SEND_TO_Q(d, "%s", class_prompt);
		STATE(d) = CON_QCLASS;
		break;

	case CON_QCLASS:
		load_result = parse_class_for_menu(*argu);
		if (load_result == CLASS_UNDEFINED) {
			if (*argu == '?') {
				struct help_index_element *this_help;
				int c;
				char *temp;
				char *help_choice =
				    get_buffer(MAX_STRING_LENGTH);
				argu = any_one_arg(argu, help_choice);
				argu = any_one_arg(argu, help_choice);
				for (c = 0; c < MAX_NEWBIE_CLASSES; c++) {
					temp = pc_class_types[c];
					if (isname(help_choice, temp)) {
						if (!
						    (this_help =
						     find_help(temp, 1)))
							SEND_TO_Q(d,
								  "\r\nThat's not a class.");
						else
							SEND_TO_Q(d, "%s", this_help->entry);
						break;
					}
				}
				release_buffer(help_choice);
			} else if ((*argu == 'q') || (*argu == 'Q')) {
				SEND_TO_Q(d, "%s", race_menu);
				SEND_TO_Q(d, "%s", race_prompt);
				STATE(d) = CON_QRACE;
				break;
			} else
				SEND_TO_Q(d, "\r\nThat's not a class.");

			SEND_TO_Q(d, "%s", class_menu_header);
			for (i = 0; i < NUM_CLASSES; i++)
				if (LEGAL_CLASS[(int)GET_RACE(d->character)][i])
					SEND_TO_Q(d, "%s", class_menu_choices[i]);
			SEND_TO_Q(d, "%s", class_prompt);
			return;
		} else
		    if (!LEGAL_CLASS[(int)GET_RACE(d->character)][load_result])
		{
			SEND_TO_Q(d,
				  "\r\n%ss are not allowed to be %ss.\r\nClass: ",
				  pc_race_types[(int)GET_RACE(d->character)],
				  pc_class_types[(int)load_result]);
			return;
		} else
			GET_CLASS(d->character) = load_result;

		if (GET_PFILEPOS(d->character) < 0)
			GET_PFILEPOS(d->character) =
			    create_entry(GET_PC_NAME(d->character));
		/* Now GET_NAME will work properly */
		init_char(d->character);

		/*log("My Id is %ld",GET_IDNUM(d->character)); */

		if (!d->first_time) {
			send_to_char(d->character, "\r\n");
			send_to_char(d->character, "%s", hometown_menu);
			SEND_TO_Q(d, "%s", hometown_prompt);
			STATE(d) = CON_HOME_TOWN;
		} else {
			/*
			   i = get_race_position(d->character);

			   d->character->player_specials->saved.point = 82;
			   d->character->real_abils.str   = race_stats[i][0];
			   d->character->real_abils.intel = race_stats[i][1];
			   d->character->real_abils.wis   = race_stats[i][2];
			   d->character->real_abils.dex   = race_stats[i][3];
			   d->character->real_abils.con   = race_stats[i][4];
			   d->character->real_abils.cha   = race_stats[i][5];
			   save_char(d->character, NOWHERE);

			   SEND_TO_Q(d,stat_menu);

			   for (j=0; j<6; j++)
			   {
			   send_to_char(d->character, "\033[%d;%dH",5+j,19);
			   send_to_char(d->character, "%-2ld            %-2ld        %-2ld",
			   race_stats[i][j], race_stats[i][j], race_max_stats[i][j]);
			   }
			   send_to_char(d->character, "\033[%d;%dH",12,0);
			   send_to_char(d->character, "You have %d", d->character->player_specials->saved.point);
			   send_to_char(d->character, "\033[%d;%dH",18,0);
			   SEND_TO_Q(d,"Enter Selection: ");
			   STATE(d) = CON_QSTAT;
			 */
			set_default_player_stats(d->character);
			save_char(d->character, NOWHERE);

			SEND_TO_Q(d, "%s", motd);
			SEND_TO_Q(d, "\r\n\n*** PRESS RETURN: ");
			STATE(d) = CON_RMOTD;
		}
		break;

	case CON_HOME_TOWN:
		switch (toupper(*argu)) {
		case 'A':
			GET_HOME(d->character) = 1005;
			break;
		case 'B':
			GET_HOME(d->character) = 3001;
			break;
		case 'C':
			GET_HOME(d->character) = 23356;
			break;
		case 'D':
			GET_HOME(d->character) = 14100;
			break;
		case 'E':
			GET_HOME(d->character) = 17801;
			break;
		default:
			send_to_char(d->character,
				     "That is not a valid hometown!\r\n\r\n");
			send_to_char(d->character, "%s", hometown_menu);
			SEND_TO_Q(d, "%s", hometown_prompt);
			return;
		}
		log("Hometown: %s created with choice %c and recalls to room %ld.", GET_NAME(d->character), argu[0], GET_HOME(d->character));

		i = get_race_position(d->character);

		d->character->player_specials->saved.point = 82;
		d->character->real_abils.str = race_stats[i][0];
		d->character->real_abils.intel = race_stats[i][1];
		d->character->real_abils.wis = race_stats[i][2];
		d->character->real_abils.dex = race_stats[i][3];
		d->character->real_abils.con = race_stats[i][4];
		d->character->real_abils.cha = race_stats[i][5];
		save_char(d->character, NOWHERE);

		SEND_TO_Q(d, "%s", stat_menu);

		for (j = 0; j < 6; j++) {
			send_to_char(d->character, "\033[%d;%dH", 5 + j, 19);
			send_to_char(d->character,
				     "%-2ld            %-2ld        %-2ld",
				     race_stats[i][j], race_stats[i][j],
				     race_max_stats[i][j]);
		}
		send_to_char(d->character, "\033[%d;%dH", 12, 0);
		send_to_char(d->character, "You have %d",
			     d->character->player_specials->saved.point);
		send_to_char(d->character, "\033[%d;%dH", 18, 0);
		SEND_TO_Q(d, "Enter Selection: ");
		STATE(d) = CON_QSTAT;
		break;

	case CON_QSTAT:
		i = GET_RACE(d->character);
		load_result = parse_stats(*argu, d->character, i);
		if (load_result == 1) {
			send_to_char(d->character, "\033[%d;%dH", 5, 33);
			send_to_char(d->character, "%-2d",
				     d->character->real_abils.str);
			send_to_char(d->character, "\033[%d;%dHYou have %-2d",
				     12, 0,
				     d->character->player_specials->saved.
				     point);
			send_to_char(d->character,
				     "\033[%d;%dH                           ",
				     18, 0);
			send_to_char(d->character, "\033[%d;%dH", 18, 0);
			SEND_TO_Q(d, "Enter Selection: ");
			return;
		}
		if (load_result == 2) {
			send_to_char(d->character, "\033[%d;%dH", 6, 33);
			send_to_char(d->character, "%-2d",
				     d->character->real_abils.intel);
			send_to_char(d->character, "\033[%d;%dHYou have %-2d",
				     12, 0,
				     d->character->player_specials->saved.
				     point);
			send_to_char(d->character,
				     "\033[%d;%dH                           ",
				     18, 0);
			send_to_char(d->character, "\033[%d;%dH", 18, 0);
			SEND_TO_Q(d, "Enter Selection: ");
			return;
		}
		if (load_result == 3) {
			send_to_char(d->character, "\033[%d;%dH", 7, 33);
			send_to_char(d->character, "%-2d",
				     d->character->real_abils.wis);
			send_to_char(d->character, "\033[%d;%dHYou have %-2d",
				     12, 0,
				     d->character->player_specials->saved.
				     point);
			send_to_char(d->character,
				     "\033[%d;%dH                           ",
				     18, 0);
			send_to_char(d->character, "\033[%d;%dH", 18, 0);
			SEND_TO_Q(d, "Enter Selection: ");
			return;
		}
		if (load_result == 4) {
			send_to_char(d->character, "\033[%d;%dH", 8, 33);
			send_to_char(d->character, "%-2d",
				     d->character->real_abils.dex);
			send_to_char(d->character, "\033[%d;%dHYou have %-2d",
				     12, 0,
				     d->character->player_specials->saved.
				     point);
			send_to_char(d->character,
				     "\033[%d;%dH                           ",
				     18, 0);
			send_to_char(d->character, "\033[%d;%dH", 18, 0);
			SEND_TO_Q(d, "Enter Selection: ");
			return;
		}
		if (load_result == 5) {
			send_to_char(d->character, "\033[%d;%dH", 9, 33);
			send_to_char(d->character, "%-2d",
				     d->character->real_abils.con);
			send_to_char(d->character, "\033[%d;%dHYou have %-2d",
				     12, 0,
				     d->character->player_specials->saved.
				     point);
			send_to_char(d->character,
				     "\033[%d;%dH                           ",
				     18, 0);
			send_to_char(d->character, "\033[%d;%dH", 18, 0);
			SEND_TO_Q(d, "Enter Selection: ");
			return;
		}
		if (load_result == 6) {
			send_to_char(d->character, "\033[%d;%dH", 10, 33);
			send_to_char(d->character, "%-2d",
				     d->character->real_abils.cha);
			send_to_char(d->character, "\033[%d;%dHYou have %-2d",
				     12, 0,
				     d->character->player_specials->saved.
				     point);
			send_to_char(d->character,
				     "\033[%d;%dH                           ",
				     18, 0);
			send_to_char(d->character, "\033[%d;%dH", 18, 0);
			SEND_TO_Q(d, "Enter Selection: ");
			return;
		}
		if (load_result != -1) {
			send_to_char(d->character,
				     "\033[%d;%dH                           ",
				     18, 0);
			send_to_char(d->character, "\033[%d;%dH", 18, 0);
			SEND_TO_Q(d, "Enter Selection: ");
			return;
		}

      /** End of stat selection **/
		save_char(d->character, NOWHERE);	/* save so dc doesn't kill stats */

		SEND_TO_Q(d, "%s", motd);
		SEND_TO_Q(d, "\r\n\n*** PRESS RETURN: ");
		STATE(d) = CON_RMOTD;

		mudlogf(NRM, LVL_IMMORT, TRUE,
			"New: %s [%s] new player. [id: %ld]",
			GET_NAME(d->character), d->host,
			GET_IDNUM(d->character));
		break;

	case CON_RMOTD:	/* read CR after printing motd   */
		SEND_TO_Q(d, "\033[H\033[J\e[?7h\e[r\x1B[1m\x1B[36m");
		if (port != 4999)
			SEND_TO_Q(d, "%s", MENU);
		STATE(d) = CON_MENU;
		break;

	case CON_MENU:		/* get selection from main menu  */
		switch (*argu) {
		case '0':
			SEND_TO_Q(d, "Goodbye.\r\n");
			STATE(d) = CON_CLOSE;
			break;

		case '1':
			SEND_TO_Q(d, "\033[H\033[J\e[?7h\e[r\x1B[1m\x1B[36m");
			reset_char(d->character);
			read_aliases(d->character);	/* Alias mod */
			if (PLR_FLAGGED(d->character, PLR_INVSTART))
				GET_INVIS_LEV(d->character) =
				    GET_LEVEL(d->character);
			if (real_room(GET_HOME(d->character)) < 1)
				GET_HOME(d->character) = mortal_start_room;

			/*
			 * with the copyover patch, this next line goes
			 * in enter_player_game()
			 */
			GET_ID(d->character) = GET_IDNUM(d->character);

			if ((load_room = GET_LOADROOM(d->character)) != NOWHERE)
				load_room = real_room(load_room);

			if ((load_room < 0)
			    || ROOM_FLAGGED(load_room, ROOM_PKILL)
			    || Z_FLAGGED(load_room, Z_PKILL))
				load_room = NOWHERE;

			/* If char was saved with NOWHERE, or real_room above failed... */
			if (load_room == NOWHERE) {

				if (GET_LEVEL(d->character) >= LVL_IMMORT) {
					load_room = r_immort_start_room;
				} else {
					load_room =
					    real_room(GET_HOME(d->character));
				}
			}

			/* Nomikos - 3/25/03 - part of new remort method */
			if (REMORT_LEVEL(d->character) >= NON_REMORT
			    && REMORT_LEVEL(d->character) != TRIPLE_REMORT) {
				if (IS_DBLREMORT_OLD(d->character))
					REMORT_LEVEL(d->character) =
					    DOUBLE_REMORT;
				else if (IS_REMORT_OLD(d->character))
					REMORT_LEVEL(d->character) =
					    SINGLE_REMORT;
				/* else leave alone */
			} else if (REMORT_LEVEL(d->character) != TRIPLE_REMORT) {
				mudlogf(BRF, LVL_IMMORT, FALSE,
					"SYSERR: %s entering game with an invalid remort level of %d!",
					GET_NAME(d->character),
					REMORT_LEVEL(d->character));
			}

			if (AFF2_FLAGGED(d->character, AFF2_FLYING) &&
			    !CAN_FLY(d->character))
				REMOVE_BIT(AFF2_FLAGS(d->character),
					   AFF2_FLYING);

			if (PLR_FLAGGED(d->character, PLR_FROZEN))
				load_room = r_frozen_start_room;
			REMOVE_BIT(PLR_FLAGS(d->character), PLR_STUNNED);

			send_to_char(d->character, "%s", WELC_MESSG);
			d->character->next = character_list;
			character_list = d->character;
			char_to_room(d->character, load_room);
			load_result = Crash_load(d->character);
			save_char(d->character, IN_ROOM(d->character));

            if (GET_INVIS_LEV(d->character) == 0) {
                send_info("[ INFO ] %s has entered the game.\n\r", GET_NAME(d->character));
            }

			act("$n has entered the game.", TRUE, d->character, 0,
			    0, TO_ROOM);
			/* with the copyover patch, this
			 * next line goes in enter_player_game() */
			read_saved_vars(d->character);

			if (AFF_FLAGGED(d->character, AFF_RAGE)) {
				REMOVE_BIT(AFF_FLAGS(d->character), AFF_RAGE);
			}

			/* If you just logged in, you shouldn't be picking anything. */
			if (AFF2_FLAGGED(d->character, AFF2_PICKING)) {
				REMOVE_BIT(AFF2_FLAGS(d->character),
					   AFF2_PICKING);
			}
			if (AFF2_FLAGGED(d->character, AFF2_PICKING_STAY)) {
				REMOVE_BIT(AFF2_FLAGS(d->character),
					   AFF2_PICKING_STAY);
			}

			affect_from_char(d->character, SKILL_RAGE);
			affect_from_char(d->character, SKILL_MOUNTED_ATTACK);
			greet_mtrigger(d->character, -1);
			greet_memory_mtrigger(d->character);

			/* color on by default */
			SET_BIT(PRF_FLAGS(d->character),
				PRF_COLOR_1 | PRF_COLOR_2);

			STATE(d) = CON_PLAYING;
			if (!GET_LEVEL(d->character)) {
				do_start(d->character, TRUE);
				send_to_char(d->character, "%s", START_MESSG);
				send_info
				    ("[ INFO ] We have a new player coming to "
				     "the mud named %s.\n\r",
				     GET_NAME(d->character));

				/* New code: send new players' base stats to remortnet. */
				struct descriptor_data *e;
				for (e = descriptor_list; e; e = e->next) {
					struct char_data *vict;
					if (e->original)
						vict = e->original;
					else
						vict = e->character;
					if (!vict || IS_NPC(vict)) {
						continue;
					}
					struct char_data *p = d->character;
					if ((STATE(e) == CON_PLAYING) &&	/* Victim is playing. */
					    ((GET_LEVEL(vict) > 100) || REMORT_LEVEL(vict) > 0)) {	/* Victim is 101+. */
						char buf1[1024];
						sprintf(buf1,
							"[Remort] The Phoenix: %s created with base stats: %d str, %d int, %d wis, %d dex, %d con, %d cha%s.\r\n",
							GET_NAME(d->character),
							GET_STR(p), GET_INT(p),
							GET_WIS(p), GET_DEX(p),
							GET_CON(p), GET_CHA(p),
							d->
							first_time ?
							" (defaults) " : "");
						send_to_char(vict, CCYEL(vict, C_NRM));
						send_to_char(vict, "%s", buf1);
						log("%s", buf1);
						send_to_char(vict,
							     CCNRM(vict,
								   C_NRM));
					}
				}
				/* End new code. */

			}
			if (PRF_FLAGGED(d->character, PRF_INFOBAR))
				REMOVE_BIT(PRF_FLAGS(d->character),
					   PRF_INFOBAR);
			/* initialize ignore list */
			for (j = 0; j < 5; j++)
				GET_IGNORED(d->character, j) = 0;
			/*do_infobar(d->character, 0, 0, SCMDB_REDRAW); *//* -naj infobar2 12/16/96 - startup infobar */
			look_at_room(d->character, 0);
			if (has_mail(GET_NAME(d->character)))
				send_to_char(d->character,
					     "You have mail waiting.\r\n");
			if (!free_rent && (load_result == 2)) {
				/* rented items lost */
				send_to_char(d->character,
					     "\r\n\007You could not afford "
					     "your rent!\r\nYour possesions have been "
					     "donated to the Salvation Army!\r\n");
			}
			d->has_prompt = 0;
			init_clan_vari(d);
			if ((table_pos =
			     find_id(GET_IDNUM(d->character))) != -1) {
				player_table[table_pos].level =
				    GET_LEVEL(d->character);
				player_table[table_pos].plr_flags =
				    PLR_FLAGS(d->character);
				if (player_table[table_pos].hostname == NULL)
					player_table[table_pos].hostname =
					    str_dup(d->host);
				else if (strcmp
					 (player_table[table_pos].hostname,
					  d->host) != 0) {
					free(player_table[table_pos].hostname);
					player_table[table_pos].hostname =
					    str_dup(d->host);
				}

				for (k = 0; k < 5; k++)
					player_table[table_pos].gold[k]
					    = d->character->points.gold[k];
				for (k = 0; k < 32; k++)
					player_table[table_pos].bank_gold[k]
					    = d->character->points.bank_gold[k];
			}
			/* this is to clear improperly set traits */
			reset_racial_traits(d->character);
			set_racial_traits(d->character);
			save_char(d->character, IN_ROOM(d->character));
			Crash_crashsave(d->character);
			break;

		case '2':
			if (d->character->player.description) {
				SEND_TO_Q(d, "Current description:\r\n");
				SEND_TO_Q(d, "%s",
					  d->character->player.description);
				/* don't free this now... so that the old description gets loaded */
				/* as the current buffer in the editor */
				/* free(d->character->player.description); */
				/* d->character->player.description = NULL; */
				/* BUT, do setup the ABORT buffer here */
				d->backstr =
				    str_dup(d->character->player.description);
			}
			SEND_TO_Q(d,
				  "Enter the text you'd like others to see when they look at you.\r\n");
			SEND_TO_Q(d, "(/s saves /h for help)\r\n");
			d->str = &d->character->player.description;
			d->max_str = EXDSCR_LENGTH - 1;
			STATE(d) = CON_EXDESC;
			break;

		case '3':
			page_string(d, background, FALSE, "");
			STATE(d) = CON_RMOTD;
			break;

		case '4':
			SEND_TO_Q(d, "\r\nEnter your old password: ");
			echo_off(d);
			STATE(d) = CON_CHPWD_GETOLD;
			break;

		case '5':
			SEND_TO_Q(d,
				  "\r\nEnter your password for verification: ");
			echo_off(d);
			STATE(d) = CON_DELCNF1;
			break;

		case '6':
			SEND_TO_Q(d, "\n\r");
			who_to_menu(d->character, "", 0, 0);
			SEND_TO_Q(d, "\n\r\n\r*** PRESS RETURN:");
			STATE(d) = CON_RMOTD;
			break;

		case '7':	/* change E-mail. */
			SEND_TO_Q(d,
				  "\r\n"
				  "We ask for your e-mail address so that we can contact you in case of hardware\r\n"
				  "failure.  Under no circumstances will we give or sell your e-mail address to a\r\n"
				  "third party.\r\n\r\n");
			if (GET_EMAIL(d->character)[0]) {
				SEND_TO_Q(d,
					  "Current e-mail address: %s\r\n\r\n",
					  GET_EMAIL(d->character));
			}
			SEND_TO_Q(d,
				  "Enter a new e-mail address (press ENTER for none): ");
			STATE(d) = CON_EDIT_EMAIL;
			break;

		default:
			SEND_TO_Q(d, "\r\nThat's not a menu choice!\r\n");
			if (port != 4999)
				SEND_TO_Q(d, "%s", MENU);
			break;
		}

		break;

	case CON_CHPWD_GETOLD:
		if (strncmp(CRYPT(argu, GET_PASSWD(d->character)),
			    GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
			echo_on(d);
			SEND_TO_Q(d, "\033[H\033[J\e[?7h\e[r\x1B[1m\x1B[36m");
			SEND_TO_Q(d, "\r\nIncorrect password.\r\n");
			if (port != 4999)
				SEND_TO_Q(d, "%s", MENU);
			STATE(d) = CON_MENU;
			return;
		} else {
			SEND_TO_Q(d, "\r\nEnter a new password: ");
			STATE(d) = CON_CHPWD_GETNEW;
			return;
		}
		break;

	case CON_DELCNF1:
		echo_on(d);
		if (strncmp(CRYPT(argu, GET_PASSWD(d->character)),
			    GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
			SEND_TO_Q(d, "\033[H\033[J\e[?7h\e[r\x1B[1m\x1B[36m");
			SEND_TO_Q(d, "\r\nIncorrect password.\r\n");
			if (port != 4999)
				SEND_TO_Q(d, "%s", MENU);
			STATE(d) = CON_MENU;
		} else {
			SEND_TO_Q(d,
				  "\r\nYOU ARE ABOUT TO DELETE THIS CHARACTER PERMANENTLY.\r\n"
				  "ARE YOU ABSOLUTELY SURE?\r\n\r\n"
				  "Please type \"yes\" to confirm: ");
			STATE(d) = CON_DELCNF2;
		}
		break;

	case CON_DELCNF2:
		if (!strcmp(argu, "yes") || !strcmp(argu, "YES")) {
			if (PLR_FLAGGED(d->character, PLR_FROZEN)) {
				SEND_TO_Q(d,
					  "You try to kill yourself, but the ice stops you.\r\n");
				SEND_TO_Q(d, "Character not deleted.\r\n\r\n");
				STATE(d) = CON_CLOSE;
				return;
			} else if (PLR_FLAGGED(d->character, PLR_NODELETE)) {
				SEND_TO_Q(d,
					  "You can't delete!  You're stuck with us FOREVER!! BWAHAHA\r\n");
				STATE(d) = CON_CLOSE;
				return;
			}
			if (GET_LEVEL(d->character) < LVL_GRGOD)
				SET_BIT(PLR_FLAGS(d->character), PLR_DELETED);
			save_char(d->character, NOWHERE);
			Crash_delete_file(GET_NAME(d->character));
			SEND_TO_Q(d, "Character '%s' deleted!\r\n"
				  "Goodbye.\r\n", GET_NAME(d->character));
			mudlogf(NRM, LVL_DGOD, TRUE,
				"DELETE: %s (lev %d) has self-deleted.",
				GET_NAME(d->character),
				GET_LEVEL(d->character));
			STATE(d) = CON_CLOSE;
			return;
		} else {
			SEND_TO_Q(d, "\033[H\033[J\e[?7h\e[r\x1B[1m\x1B[36m");
			SEND_TO_Q(d, "\r\nCharacter not deleted.\r\n");
			if (port != 4999)
				SEND_TO_Q(d, "%s", MENU);
			STATE(d) = CON_MENU;
		}
		break;

	case CON_EDIT_EMAIL:
		if (argu && *argu) {
			strcpy(GET_EMAIL(d->character), argu);
			SEND_TO_Q(d, "\r\nE-mail address updated to %s\r\n",
				  GET_EMAIL(d->character));
		} else {
			GET_EMAIL(d->character)[0] = '\x0';
			SEND_TO_Q(d, "\r\nE-mail address cleared.\r\n");
		}
		mudlogf(CMP, GOD_LOG(d->character), TRUE,
			"E-mail: %s changed e-mail address to \"%s\".",
			GET_NAME(d->character), GET_EMAIL(d->character));
		save_char(d->character, IN_ROOM(d->character));
		SEND_TO_Q(d, "%s", MENU);
		STATE(d) = CON_MENU;
		break;

		/* Taken care of in game_loop()
		   case CON_CLOSE:
		   close_socket(d);
		   break;
		 */
	case CON_DISCONNECT:
		break;

	default:
		log("SYSERR: Nanny: illegal state of con'ness (%d); closing connection for '%s'", STATE(d), d->character ? GET_NAME(d->character) : "<unknown>");
		STATE(d) = CON_DISCONNECT;
		break;
	}
}
