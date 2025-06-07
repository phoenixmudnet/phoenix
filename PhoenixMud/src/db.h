/* ************************************************************************
*   File: db.h                                          Part of CircleMUD *
*  Usage: header file for database handling                               *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#ifndef __DB_C_
extern struct player_special_data dummy_mob;
extern room_rnum top_of_world;
#endif

/* arbitrary constants used by index_boot() (must be unique) */
#define DB_BOOT_WLD	0
#define DB_BOOT_MOB	1
#define DB_BOOT_OBJ	2
#define DB_BOOT_ZON	3
#define DB_BOOT_SHP	4
#define DB_BOOT_HLP	5
#define DB_BOOT_GLD	6
#define DB_BOOT_PTH	7
#define DB_BOOT_TRG	8

/* world file versions */
#define CUR_WORLD_VER   2
#define CUR_MOB_VER     2
#define CUR_OBJ_VER     2
#define CUR_ZONE_VER    5
#define CUR_PATH_VER    1
#define CUR_GUILD_VER   1
#define CUR_CLAN_VER    3 /* V3 - Nomikos 6/2/25 */

/* player data file versions */
#define CUR_POBJ_VER    2

/* names of various files and directories */
#define INDEX_FILE	"index"		/* index of world files		*/
#define MINDEX_FILE	"index.mini"	/* ... and for mini-mud-mode	*/

#define WLD_PREFIX	"world/wld"	/* room definitions		*/
#define MOB_PREFIX	"world/mob"	/* monster prototypes		*/
#define OBJ_PREFIX	"world/obj"	/* object prototypes		*/
#define ZON_PREFIX	"world/zon"	/* zon defs & command tables	*/
#define SHP_PREFIX	"world/shp"	/* shop definitions		*/
#define GLD_PREFIX	"world/guild"	/* guild definitions		*/
#define PTH_PREFIX	"world/pth"	/* path definitions		*/
#define TRG_PREFIX	"world/trg"	/* trigger definitions		*/
#define MOB_DIR         "world/prg"     /* Mob programs                 */
#define ASSEMBLIES_FILE	"world/assemblies/assemblies" /* Assemblies engine	*/

#define HLP_PREFIX	"text/help"	/* for HELP <keyword>		*/
#define HLP_FILE  	"help.hlp" 	/* for HELP <keyword>		*/
#define CREDITS_FILE	"text/credits"	/* for the 'credits' command	*/
#define NEWS_FILE	"text/news"	/* for the 'news' command	*/
#define MOTD_FILE	"text/motd"	/* messages of the day / mortal	*/
#define IMOTD_FILE	"text/imotd"	/* messages of the day / immort	*/
#define HELP_PAGE_FILE	"text/help/screen" /* for HELP <CR>		*/
#define INFO_FILE	"text/info"	/* for INFO			*/
#define TEAMS_FILE	"text/teams"	/* for TEAMS			*/
#define WIZLIST_FILE	"text/wizlist"	/* for WIZLIST			*/
#define IMMLIST_FILE	"text/immlist"	/* for IMMLIST			*/
#define BACKGROUND_FILE	"text/background" /* for the background story	*/
#define POLICIES_FILE	"text/policies"	/* player policies/rules	*/
#define MARRIAGES_FILE  "text/marriages"  /* Marriage rules file        */
#define HANDBOOK_FILE	"text/handbook"	/* handbook for new immorts	*/
#define AREAS_FILE	"text/areas"	/* areas file                   */
#define SKILLS_FILE	"text/skills"	/* Skills file			*/
#define SPELLS_FILE	"text/spells"	/* Spells file			*/

#define IDEA_FILE	"misc/ideas"	/* for the 'idea'-command	*/
#define TYPO_FILE	"misc/typos"	/*         'typo'		*/
#define BUG_FILE	"misc/bugs"	/*         'bug'		*/
#define MESS_FILE	"misc/messages"	/* damage messages		*/
#define SOCMESS_FILE	"misc/socials"	/* messgs for social acts	*/
#define XNAME_FILE	"misc/xnames"	/* invalid name substrings	*/

#define PLAYER_FILE	"etc/players"	/* the player database		*/
#define PLAYERVAR_PREFIX "etc/plrvar"
#define MAIL_FILE	"etc/plrmail"	/* for the mudmail system	*/
#define BAN_FILE	"etc/badsites"	/* for the siteban system	*/
#define HCONTROL_FILE	"etc/hcontrol"  /* for the house system		*/
#define CLAN_FILE       "etc/clans"     /* the clan database (ASCII)    */
#define MAIL_LOG        "etc/maillog"   /* Log of the user mud mail     */
#define SHOP_SAVE_PREFIX "etc/shpobj"   /* Saving objects that are sold */

#define DG_QUEST_FILE   "etc/dg_quests"

#define LOGDIR          "../log/"

#define LOG_SYSLOG1     LOGDIR"syslog.1"
#define LOG_SYSLOG2     LOGDIR"syslog.2"
#define LOG_SYSLOG3     LOGDIR"syslog.3"
#define LOG_SYSLOG4     LOGDIR"syslog.4"
#define LOG_SYSLOG5     LOGDIR"syslog.5"
#define LOG_SYSLOG6     LOGDIR"syslog.6"
#define LOG_LEVELS      LOGDIR"levels"
#define LOG_NEWPLAYERS  LOGDIR"newplayers"
#define ERRORS_FILE     LOGDIR"errors" 
#define BIGRENT_LOG     LOGDIR"bigrent"
#define BUF_LOG         LOGDIR"BUF"
#define LASTCMD_LOG     LOGDIR"last_command" 
#define GODCMD_LOG      LOGDIR"godcmds" 
#define DELETE_LOG      LOGDIR"delete" 
#define DT_LOG          LOGDIR"dts" 
#define RIP_LOG         LOGDIR"rip"
#define CORPSE_LOG      LOGDIR"corpse" 
#define SCRIPTERR_LOG   LOGDIR"scripterr" 
#define HELP_LOG        LOGDIR"help" 
#define GOLD_LOG        LOGDIR"gold" 
#define LOCATE_OBJ_LOG  LOGDIR"locateobj" 
#define CRASH_LOG       "../syslog.CRASH" 
#define BAN_LOG         LOGDIR"ban"
#define OBJSCRAP_LOG    LOGDIR"objscrap"
#define OLC_LOG         LOGDIR"OLC"
#define USAGE_LOG       LOGDIR"usage"
#define RESTART_LOG     LOGDIR"restarts"
#define RENTGONE_LOG    LOGDIR"rentgone"
#define BADPW_LOG       LOGDIR"badpws"
#define GODFIGHT_LOG    LOGDIR"god.fight"
#define SCRIPT_LOG      LOGDIR"scriptlog"
#define DG_QUEST_LOG    LOGDIR"dg_quests"
#define GRAFFITI_LOG    LOGDIR"graffiti"
#define UPLOAD_LOG      LOGDIR"upload"
#define SHOP_LOG        LOGDIR"player_shop"

#define CHANGES_FILE    "../src/workdocs/Changes" /*change log          */

#define FASTBOOT_FILE   "../.fastboot"  /* autorun: boot without sleep  */
#define KILLSCRIPT_FILE "../.killscript"/* autorun: shut mud down       */
#define PAUSE_FILE      "../pause"      /* autorun: don't restart mud   */
#define SYSLOG_FILE     "../syslog"     /* the system log, just in case */

#define ZONE_UNASSIGNED 0
#define ZONE_CALEDON    1
#define ZONE_AGLARON    2
#define ZONE_UNDERDARK  3
#define ZONE_OCEAN      4
#define ZONE_ARCTIC     5
#define ZONE_OTHER_GOD  6

/* public procedures in db.c */
void	boot_db(void);
long	create_entry(char *name);
void	zone_update(void);
room_rnum real_room(room_vnum vnum);
long	real_path(long vnum);
char   *fread_string(FILE *fl, char *error);
long	get_id_by_name(char *name);
char   *get_name_by_id(long id);
int     vnum_room(char *searchname, struct char_data * ch);

void	char_to_store(struct char_data *ch, struct char_file_u *st,
		      int save_time);
void	store_to_char(struct char_file_u *st, struct char_data *ch);
long	load_char(char *name, struct char_file_u *char_element);
void	save_char(struct char_data *ch, room_rnum load_room);
void	init_char(struct char_data *ch);
struct  char_data* create_char(void);
struct  char_data *read_mobile(mob_vnum nr, int type);
mob_rnum real_mobile(mob_vnum vnum);
int	vnum_mobile(char *searchname, struct char_data *ch);
void	clear_char(struct char_data *ch);
void	reset_char(struct char_data *ch);
void	free_char(struct char_data *ch);

void    load_dg_quests(void);
void    save_dg_quests(void);
struct dg_quest *get_dg_quest(char *quest_name);

struct  obj_data *create_obj(void);
void	clear_object(struct obj_data *obj);
void	free_obj(struct obj_data *obj);
obj_rnum real_object(obj_vnum vnum);
struct  obj_data *read_object(obj_vnum nr, int type);
int	vnum_object(char *searchname, struct char_data *ch);
int     my_obj_save_to_disk(FILE *fp, struct obj_data *obj, int locate);
void    flagascii_conv(char *flag,bitvector_t vector);

void    vwear_object(int wearpos, struct char_data * ch);

#define REAL 0
#define VIRTUAL 1

/* structure for the reset commands */
struct reset_com {
   char	command;   /* current command                      */

   bool if_flag;	/* if TRUE: exe only if preceding exe'd */
   long	arg1;		/*                                      */
   long	arg2;		/* Arguments to the command             */
   long	arg3;		/*                                      */
   long	arg4;		/*                                      */
   long	oarg1;		/*                                      */
   long	oarg2;		/* Arguments to the command             */
   long	oarg3;		/*                                      */
   long	oarg4;		/*                                      */
   char *sarg;
   char *sarg2;
   long  line;		/* line number this command appears on  */
   long  room_num;	/* the room this command is zedited from*/
   /* 
	*  Commands:              *
	*  'D': Set state of door *
	*  'E': Obj to char equip *
	*  'G': Give obj to mob   *
	*  'M': Read a mobile     *
	*  'O': Read an object    *
	*  'P': Put obj in obj    *
	*  'R': Remove Obj        *
	*  'T': Trigger command   *
	*  'V': Set Trigger Vari  *
	*  'Z': Make Maze         *
   */
};



/* zone definition structure. for the 'zone-table'   */
struct zone_data {
   char	*name;		    /* name of this zone                  */
   char *author;
   char *editor;
   char *levels; 
   char *comment;
   char *worldProof;
   char *trigProof;
   char *objBalanced;
   time_t dateStarted;
   time_t dateImped;
   time_t dateLastMod;
   char *nameLastMod;
   byte status;
   byte source;
   int continent;
   int	lifespan;		/* how long between resets (minutes)  */
   int	age;			/* current age of this zone (minutes) */
   room_vnum top;		/* upper limit for rooms in this zone */

   int	reset_mode;		/* conditions for reset (see below)   */
   zone_vnum number;		/* virtual number of this zone	  */
   bitvector_t bitvector;		/* Bits for zones			  */
   struct reset_com *cmd;	/* command table for reset	          */
   int idle_time;		/* if zone is idle for <age> minutes, purge
				it and wait for  for players to to enter */

  int num_rooms;                /* How many rooms in this zone actually exist. */

   /*
	*  Reset mode:                              *
	*  0: Don't reset, and don't update age.    *
	*  1: Reset if no PC's are located in zone. *
	*  2: Just reset.                           *
   */
};



/* for queueing zones for update   */
struct reset_q_element {
   zone_rnum zone_to_reset;            /* ref to zone_data */
   struct reset_q_element *next;
};



/* structure for the update queue     */
struct reset_q_type {
   struct reset_q_element *head;
   struct reset_q_element *tail;
};



struct player_index_element {
   char	*name;
   long id;
   char *hostname;
   ush_int level;
   long gold[5];
   long bank_gold[32];
   long plr_flags;
   time_t last_logon;
};


struct help_index_element {
   char	*keywords;
   char *entry;
   int  min_level;
};

struct dg_quest
{
  char *quest_name;
  int num_completed;
  int *completed_by;
};
extern struct dg_quest *dg_quests;
extern int num_dg_quests;

/** Graffiti **/
#define GRAFFITI_FILE "etc/graffiti"
#define MAX_TEMP_GRAFFITI_PER_PLAYER 6
#define MAX_PERM_GRAFFITI_PER_PLAYER 4
struct graffiti
{
  int author; /* pfilepos of author. */
  char permanent; /* 1=permanent, 0=temporary */
  int room_vnum;
  char *text;
};
extern struct graffiti *graffiti;
extern int num_graffiti;
void load_graffiti(void);
void save_graffiti(void);
int get_graffiti(int vnum, char *output, int len);
int remove_graffiti(int author, int vnum);
int remove_graffiti_at(int in);
int get_graffiti_count(int author, int permanent);
int graffiti_exists(int vnum);

/* don't change these */
#define BAN_NOT 	0
#define BAN_NEW 	1
#define BAN_SELECT	2
#define BAN_ALL		3

#define BANNED_SITE_LENGTH    50
struct ban_list_element {
   char	site[BANNED_SITE_LENGTH+1];
   int	type;
   time_t date;
   char	name[MAX_NAME_LENGTH+1];
   struct ban_list_element *next;
};



/* global buffering system -- removed for better */
#if !defined(_BUFFER_H_)
#include "buffer.h"
#endif
 

/*#ifdef __DB_C__
  char	buf[MAX_STRING_LENGTH];
  char	buf1[MAX_STRING_LENGTH];
  char	buf2[MAX_STRING_LENGTH];
  char	arg[MAX_STRING_LENGTH];
  char    specbuf[MAX_STRING_LENGTH]; 
  char    bigbuf[16384];
  #else
  extern char	buf[MAX_STRING_LENGTH];
  extern char	buf1[MAX_STRING_LENGTH];
  extern char	buf2[MAX_STRING_LENGTH];
  extern char	arg[MAX_STRING_LENGTH];
  extern char     specbuf[MAX_STRING_LENGTH];
  extern char     bigbuf[16384];
  #endif
  */

#ifndef __CONFIG_C__
extern char	*OK;
extern char	*NOPERSON;
extern char	*NOEFFECT;
#endif
void strip_string(char *strip_buffer);
int read_xap_objects(FILE *fl,struct char_data *ch);
