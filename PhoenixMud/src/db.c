/* ************************************************************************
*   File: db.c                                          Part of CircleMUD *
*  Usage: Loading/saving chars, booting/resetting world, internal funcs   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define __DB_C__

#include "../localHeader/conf.h"
#include "../localHeader/sysdep.h"


#include "structs.h"
#include "buffer.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "handler.h"
#include "queue.h"
#include "spells.h"
#include "mail.h"
#include "interpreter.h"
#include "house.h"
#include "spec_assign.h"
#include "path.h"
#include "dg_scripts.h"
#include "constants.h"
#include "assemblies.h"
#include "gremort_exam.h"

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

/**************************************************************************
*  declarations of most of the 'global' variables                         *
************************************************************************ */

struct room_data *world = NULL; /* array of rooms   */
room_rnum top_of_world = 0; /* ref to top element of world  */

struct char_data *character_list = NULL; /* global linked list of chars  */

struct index_data **trig_index; /* index table for triggers      */
long top_of_trigt = 0;  /* top of trigger index table    */
long max_id = MOBOBJ_ID_BASE;   /* for unique mob/obj id's       */

struct index_data *mob_index; /* index table for mobile file  */
struct char_data *mob_proto; /* prototypes for mobs   */
mob_rnum top_of_mobt = 0; /* top of mobile index table  */

struct obj_data *object_list = NULL; /* global linked list of objs  */
struct index_data *obj_index; /* index table for object file  */
struct obj_data *obj_proto; /* prototypes for objs   */
obj_rnum top_of_objt = 0; /* top of object index table  */

struct zone_data *zone_table; /* zone table    */
zone_rnum top_of_zone_table = 0; /* top element of zone tab  */
struct message_list fight_messages[MAX_MESSAGES]; /* fighting messages  */

struct player_index_element *player_table = NULL; /* index to plr file  */
FILE *player_fl = NULL;  /* file desc of player file  */
int top_of_p_table = 0;  /* ref to top of table   */
int top_of_p_file = 0;  /* ref of size of p file  */
long top_idnum = 0;  /* highest idnum in use   */

int no_mail = 0;  /* mail disabled?   */
int mini_mud = 0;  /* mini-mud mode?   */
int no_rent_check = 0;  /* skip rent check on boot?  */
time_t boot_time = 0;  /* time of mud boot   */
int circle_restrict = 0; /* level of game restriction  */
room_rnum r_mortal_start_room; /* rnum of mortal start room  */
room_rnum r_immort_start_room; /* rnum of immort start room  */
room_rnum r_frozen_start_room; /* rnum of frozen start room  */
int xap_objs = 0;               /* ascii objs file            */

char *credits = NULL;  /* game credits    */
char *news = NULL;  /* mud news    */
char *motd = NULL;  /* message of the day - mortals */
char *imotd = NULL;  /* message of the day - immorts */
char *help = NULL;  /* help screen    */
char *info = NULL;  /* info page    */
char *wizlist = NULL;  /* list of higher gods   */
char *immlist = NULL;  /* list of peon gods   */
char *herolist = NULL; /* list of wankers   */
char *background = NULL; /* background story   */
char *handbook = NULL;  /* handbook for new immortals  */
char *teams = NULL;  /* teams page   */
char *policies = NULL;  /* policies page   */
char *marriages = NULL; /* marriages page */
char *areas = NULL;  /* areas page   */
char *pspells = NULL;  /* spell list   */
char *pskills = NULL;  /* skill list   */

struct help_index_element *help_table = 0; /* the help table  */
int top_of_helpt = 0;  /* top of help index table  */

struct time_info_data time_info;/* the infomation about the time    */
struct weather_data weather_info; /* the infomation about the weather */
struct player_special_data dummy_mob; /* dummy spec area for mobs  */
struct reset_q_type reset_q; /* queue of zones to be reset  */
struct autoauction aauction;    /* 2/24/97, Anduin autoauction control */
struct battle_zone battle; /* 2/26/97, Anduin battlezone control */
struct queue_event *auction_event=NULL;

struct default_ability_data default_player_stats[];
int num_default_player_stats = 0;


/* local functions */
void setup_dir(FILE * fl, int room, int dir,int version);
void index_boot(int mode);
void discrete_load(FILE * fl, int mode,char *filename);
void parse_path(FILE * path_f, int nr, int version);
void parse_room(FILE * fl, int virtual_nr, int version);
void parse_mobile(FILE * mob_f, int nr, int version);
char *parse_object(FILE * obj_f, int nr, int version);
void parse_trigger(FILE *fl, int virtual_nr);
void load_zones(FILE * fl, char *zonename);
void load_help(FILE *fl,char *filename);
void assign_mobiles(void);
void assign_objects(void);
void assign_rooms(void);
void assign_the_shopkeepers(void);
void assign_the_gms(void);
void build_player_index(void);
void char_to_store(struct char_data * ch, struct char_file_u * st,int save_time);
void store_to_char(struct char_file_u * st, struct char_data * ch);
int is_empty(zone_rnum zone_nr);
void reset_zone(zone_rnum zone);
int file_to_string(char *name, char *buf);
int file_to_string_alloc(char *name, char **buf);
void check_start_rooms(void);
void renum_world(void);
void renum_zone_table(void);
void set_zone_room_counts(void);
void log_zone_error(zone_rnum zone,int cmd_no,char *message,room_vnum roomnum);
void reset_time(void);
void clear_char(struct char_data * ch);
void get_one_line(FILE *fl, char *buf);
int strip_zone_color(char *inbuf);
int is_colour(char code);
int check_object(struct obj_data *obj);
int check_object_spell_number(struct obj_data *obj, int val);
int check_object_level(struct obj_data *obj, int val);
int purge_zone(int zone);
void save_char_ascii(struct char_file_u *ch);
void load_char_ascii(struct char_file_u *ch, char *name);
void read_line_ascii(FILE *fp, char *string, int len);
void write_player_index_file(void);
void load_player_index_file(void);
void load_default_player_stats( void );
void set_default_player_stats( struct char_data* ch );

/* external functions */
struct time_info_data *mud_time_passed(time_t t2, time_t t1);
void load_messages(void);
void weather_and_time(int mode);
void boot_social_messages(void);
void update_obj_file(void); /* In objsave.c */
void sort_commands(void);
void sort_spells(void);
void load_banned(void);
void Read_Invalid_List(void);
void boot_the_shops(FILE * shop_f, char *filename, int rec_count);
void boot_the_guilds(FILE *gm_f, char *filename, int rec_count);
void free_alias(struct alias_data * a);
void mprog_read_programs(FILE * fp, struct index_data * pMobIndex);
void prune_crlf(char *txt);
void save_char_vars(struct char_data *ch);
void load_corpses(void);
void load_spells(void);
int find_first_step(room_rnum src, room_rnum target,long iFlag);
int find_name(char *name);
int find_id(long id);
void skin_update(struct char_data *pxChar,
                 struct obj_data *pxCarcass,
                 int iState);
void load_player_shops();

void trig_wait_event(void *trig_info);
void load_keeper(struct char_data *keeper);

/* external vars */
extern struct descriptor_data *descriptor_list;
extern int no_specials;
extern int rev_dir[];
extern int wear_check[];
extern room_vnum mortal_start_room;
extern room_vnum immort_start_room;
extern room_vnum frozen_start_room;
extern char *class_abbrevs[];
extern char *race_abbrevs[];
extern const char *unused_spellname;
extern struct queue_event *command_queue;
extern struct npc_class_mana npc_class_mult[];

#define READ_SIZE 256

/*************************************************************************
*  routines for booting the system                                       *
*********************************************************************** */

/* this is necessary for the autowiz system */
extern void update_wizlist(void);
void reboot_wizlists(void)
   {
     update_wizlist();
   file_to_string_alloc(WIZLIST_FILE, &wizlist);
   file_to_string_alloc(WIZLIST_FILE, &wizlist);
   }


ACMD(do_reboot)
   {
   int i;
   char *arg = get_buffer(MAX_INPUT_LENGTH);

   one_argument(argument, arg);

   if (!str_cmp(arg, "all") || *arg == '*')
      {
      file_to_string_alloc(WIZLIST_FILE, &wizlist);
      file_to_string_alloc(IMMLIST_FILE, &immlist);
      file_to_string_alloc(NEWS_FILE, &news);
      file_to_string_alloc(CREDITS_FILE, &credits);
      file_to_string_alloc(MOTD_FILE, &motd);
      file_to_string_alloc(IMOTD_FILE, &imotd);
      file_to_string_alloc(HELP_PAGE_FILE, &help);
      file_to_string_alloc(INFO_FILE, &info);
      file_to_string_alloc(TEAMS_FILE, &teams);
      file_to_string_alloc(POLICIES_FILE, &policies);
      file_to_string_alloc(MARRIAGES_FILE, &marriages);
      file_to_string_alloc(HANDBOOK_FILE, &handbook);
      file_to_string_alloc(BACKGROUND_FILE, &background);
      file_to_string_alloc(AREAS_FILE, &areas);
      file_to_string_alloc(SKILLS_FILE, &pskills);
      file_to_string_alloc(SPELLS_FILE, &pspells);
      }
   else if (!str_cmp(arg, "wizlist"))
      file_to_string_alloc(WIZLIST_FILE, &wizlist);
   else if (!str_cmp(arg, "immlist"))
      file_to_string_alloc(IMMLIST_FILE, &immlist);
   else if (!str_cmp(arg, "herolist"))
      file_to_string_alloc(HEROLIST_FILE, &herolist);
   else if (!str_cmp(arg, "news"))
      file_to_string_alloc(NEWS_FILE, &news);
   else if (!str_cmp(arg, "credits"))
      file_to_string_alloc(CREDITS_FILE, &credits);
   else if (!str_cmp(arg, "motd"))
      file_to_string_alloc(MOTD_FILE, &motd);
   else if (!str_cmp(arg, "imotd"))
      file_to_string_alloc(IMOTD_FILE, &imotd);
   else if (!str_cmp(arg, "help"))
      file_to_string_alloc(HELP_PAGE_FILE, &help);
   else if (!str_cmp(arg, "info"))
      file_to_string_alloc(INFO_FILE, &info);
   else if (!str_cmp(arg, "teams"))
      file_to_string_alloc(TEAMS_FILE, &teams);
   else if (!str_cmp(arg, "policy"))
      file_to_string_alloc(POLICIES_FILE, &policies);
   else if (!str_cmp(arg, "marriages"))
      file_to_string_alloc(MARRIAGES_FILE, &marriages);
   else if (!str_cmp(arg, "handbook"))
      file_to_string_alloc(HANDBOOK_FILE, &handbook);
   else if (!str_cmp(arg, "background"))
      file_to_string_alloc(BACKGROUND_FILE, &background);
   else if (!str_cmp(arg, "areas"))
      file_to_string_alloc(AREAS_FILE, &areas);
   else if (!str_cmp(arg, "spells"))
      file_to_string_alloc(SPELLS_FILE, &pspells);
   else if (!str_cmp(arg, "skills"))
      file_to_string_alloc(SKILLS_FILE, &pskills);
   else if (!str_cmp(arg, "xhelp"))
      {
      if (help_table)
         {
         for (i = 0; i <= top_of_helpt; i++)
            {
            if (help_table[i].keywords)
               free(help_table[i].keywords);
            if (help_table[i].entry)
               free(help_table[i].entry);
            }
         free(help_table);
         }
      top_of_helpt = 0;
      index_boot(DB_BOOT_HLP);
      }
   else
      {
      send_to_char(ch,"Unknown reload option.\r\n");
      release_buffer(arg);
      return;
      }

   send_to_char(ch, "%s", OK);
   release_buffer(arg);
   }


void boot_world(void)
   {
   log("Loading zone table.");
   index_boot(DB_BOOT_ZON);

   log("Loading triggers and generating index.");
   index_boot(DB_BOOT_TRG);

   log("Loading path table.");
   index_boot(DB_BOOT_PTH);

   log("Loading gremort exam information.");
   gremort_load_skill_requirements();
   gremort_load_exam_records();
   gremort_load_trivia_questions();


   log("Loading rooms.");
   index_boot(DB_BOOT_WLD);

   log("Renumbering rooms.");
   renum_world();

   log("Checking start rooms.");
   check_start_rooms();

   log("Loading mobs and generating index.");
   index_boot(DB_BOOT_MOB);

   log("Loading objs and generating index.");
   index_boot(DB_BOOT_OBJ);

   log("Renumbering zone table.");
   renum_zone_table();

   log("Setting zone room counts.");
   set_zone_room_counts();

   log("Loading player shops.");
   load_player_shops();

   log("Renumbering path table.");
   /*
    *   renum_path_table();
    */

   if (!no_specials)
      {
      log("Loading guilds.");
      index_boot(DB_BOOT_GLD);

      log("Loading shops.");
      index_boot(DB_BOOT_SHP);
      }
   log("Loading Corpses");
   load_corpses();
   }

extern int port;

/* body of the booting system */
void boot_db(void)
   {
   zone_rnum i;

   log("Boot db -- BEGIN.");

   log("Resetting the game time:");
   reset_time();

   log("Reading news, credits, help, bground, info & motds.");
   file_to_string_alloc(NEWS_FILE, &news);
   file_to_string_alloc(CREDITS_FILE, &credits);
   file_to_string_alloc(MOTD_FILE, &motd);
   file_to_string_alloc(IMOTD_FILE, &imotd);
   file_to_string_alloc(HELP_PAGE_FILE, &help);
   file_to_string_alloc(INFO_FILE, &info);
   file_to_string_alloc(TEAMS_FILE, &teams);
   file_to_string_alloc(WIZLIST_FILE, &wizlist);
   file_to_string_alloc(IMMLIST_FILE, &immlist);
   file_to_string_alloc(HEROLIST_FILE, &herolist);
   file_to_string_alloc(POLICIES_FILE, &policies);
   file_to_string_alloc(MARRIAGES_FILE, &marriages);
   file_to_string_alloc(HANDBOOK_FILE, &handbook);
   file_to_string_alloc(BACKGROUND_FILE, &background);
   file_to_string_alloc(AREAS_FILE, &areas);
   file_to_string_alloc(SPELLS_FILE, &pspells);
   file_to_string_alloc(SKILLS_FILE, &pskills);

   log("Loading spells.");
   load_spells();

   boot_world();

   log("Loading default player stats.");
   load_default_player_stats();

   log("Loading help entries.");
   index_boot(DB_BOOT_HLP);

   log("Generating player index.");
   build_player_index();

   log("Loading fight messages.");
   load_messages();

   log("Loading social messages.");
   boot_social_messages();

   log("Loading dg quests.");
   load_dg_quests();

   log("Loading graffiti.");
   load_graffiti();

   log("Assigning function pointers:");

   if (!no_specials)
      {
      /*      log("   Mobiles.");
            assign_mobiles();

            */
      log("   Guildmasters.");
      assign_the_gms();
      log("   Shopkeepers.");
      assign_the_shopkeepers();
      log("   Objects.");
      assign_objects();
      log("   Rooms.");
      assign_rooms();
      }

   log("Booting assembled objects.");
   assemblyBootAssemblies();

   log("Sorting command list and spells.");
   sort_commands();
   sort_spells();

   log("Booting mail system.");
   if (!scan_file())
      {
      log("    Mail boot failed -- Mail system disabled");
      no_mail = 1;
      }
   log("Reading banned site and invalid-name list.");
   load_banned();
   Read_Invalid_List();

   if (!no_rent_check)
      {
      log("Deleting timed-out crash and rent files:");
      update_obj_file();
      log("    Done.");
      }


   if (!mini_mud)
      {
      log("Booting houses.");
      House_boot();
      }

  for (i = 0; i <= top_of_zone_table; i++)
      {
      log("Resetting %s (rooms %ld-%ld).",
          zone_table[i].name, (i ? (zone_table[i - 1].top + 1) : 0),
          zone_table[i].top);
      reset_zone(i);

      /* If this isn't player's port (or coders' port), purge every zone we just activated (except zone 0). :-) */
      if (port != 4000 && port != 24000 && i > 0) {
	SET_BIT(ZONE_FLAGS(i), Z_IDLE);
	zone_table[i].idle_time = 0;
	zone_table[i].age = 0;
	purge_zone(i);
	mudlogf(CMP, LVL_IMMORT, FALSE,"Idle purging zone: %s:%ld",
		zone_table[i].name, zone_table[i].number);
      }





      }

   reset_q.head = reset_q.tail = NULL;
   boot_time = time(0);
   MOBTrigger = TRUE;

   log("Boot db -- DONE.");
   }


/* reset the time in the game from file */
void reset_time(void)
   {
   long beginning_of_time = 892671139; /* 650336715 */

   time_info = *mud_time_passed(time(0), beginning_of_time);

   switch (time_info.hours)
      {
      case 0 :
         weather_info.moonlight = MOON_LIGHT;
      case 1 :
      case 2 :
      case 3 :
      case 4 :
         weather_info.sunlight = SUN_DARK;
         break;
      case 5 :
         weather_info.sunlight = SUN_RISE;
         break;
      case 6 :
      case 7 :
      case 8 :
         weather_info.moonlight = MOON_SET;
      case 9 :
      case 10 :
         weather_info.moonlight = MOON_DARK;
      case 11 :
      case 12 :
      case 13 :
      case 14 :
      case 15 :
      case 16 :
      case 17 :
      case 18 :
      case 19 :
      case 20 :
         weather_info.sunlight = SUN_LIGHT;
         break;
      case 21 :
         weather_info.sunlight = SUN_SET;
         break;
      case 22 :
      case 23 :
         weather_info.moonlight = MOON_RISE;
         break;
      default :
         weather_info.sunlight = SUN_DARK;
         break;
      }
   weather_info.moon_phase = number(0,8);

   log("   Current Gametime: %dH %dD %dM %dY.", time_info.hours,
       time_info.day, time_info.month, time_info.year);

   weather_info.pressure = 960;
   if ((time_info.month >= 7) && (time_info.month <= 12))
      weather_info.pressure += dice(1, 50);
   else
      weather_info.pressure += dice(1, 80);

   weather_info.change = 0;

   if (weather_info.pressure <= 980)
      weather_info.sky = SKY_LIGHTNING;
   else if (weather_info.pressure <= 1000)
      weather_info.sky = SKY_RAINING;
   else if (weather_info.pressure <= 1020)
      weather_info.sky = SKY_CLOUDY;
   else
      weather_info.sky = SKY_CLOUDLESS;

   /** 2/26/97 Anduin - Battle init code **/

   log("Initializing the battle zone <Closed>");

   battle.zone_state = FALSE;
   battle.low_level = 0;
   battle.high_level = 0;
   battle.locked = FALSE;
   battle.tagged = FALSE;
   battle.do_tag = FALSE;

   /** 2/24/97, Anduin -  Auction init code **/
   log("Initializing the Autoauction system");

   aauction.in_progress = FALSE;  /* Wether an auction is taking place or not */
   aauction.bid_on = FALSE;       /* Wether the item has been bid upon */
   aauction.previous_bid = 0;
   aauction.last_bid = 0;         /* The last bid on the item */
   aauction.previous_bidder_id_num = 0;
   aauction.bidder_id_num = 0;    /* The person that last bid idnumber */
   aauction.seller_id_num = 0;    /* The person that is selling's idnum */
   aauction.selling_price = 0;    /* Price asking for the item */
   aauction.state_of_sale = 0;    /* Going 1, 2, 3, sold */
   aauction.item_auc = 0;
   auction_event=NULL;
   }


/* generate index table for the player file */
void build_player_index(void)
   {
   /*

   log("   %ld is top_idnum.",top_idnum);
   top_of_p_file = top_of_p_table = nr;
*/
#if 1
   load_player_index_file();
#else
   int nr = -1, i;
   long size, recs;
   struct char_file_u dummy;

   log("Opening player file...");
   if (!(player_fl = fopen(PLAYER_FILE, "r+b")))
      {
      if (errno != ENOENT)
         {
         perror("SYSERR: fatal error opening playerfile");
         exit(1);
         }
      else
         {
         log("No playerfile.  Creating a new one.");
         touch(PLAYER_FILE);
         if (!(player_fl = fopen(PLAYER_FILE, "r+b")))
            {
            perror("SYSERR: fatal error opening playerfile");
            exit(1);
            }
         }
      }

   fseek(player_fl, 0L, SEEK_END);
   size = ftell(player_fl);
   rewind(player_fl);
   int bytes = 4556;
   if (size % bytes)
      log("\aWARNING:  PLAYERFILE IS PROBABLY CORRUPT!");

   log("size=%ld sizeof=%d", size, bytes);

   recs = size / bytes;
   if (recs)
      {
      log("   %ld players in database.", recs);
      CREATE(player_table, struct player_index_element, recs);
      }
   else
      {
      player_table = NULL;
      top_of_p_file = top_of_p_table = -1;
      return;
      }

   top_idnum=0;
   for (; !feof(player_fl);)
      {
	memset(&dummy, 0, sizeof(struct char_file_u));
	fread(&dummy, bytes, 1, player_fl);
         nr++;

      if (!feof(player_fl))
         {

	   /*if (!dummy.name || !dummy.name[0]) {
	     continue;
	   }
	   */

         CREATE(player_table[nr].name, char, strlen(dummy.name) + 1);
         for (i = 0;
                 (*(player_table[nr].name + i) = LOWER(*(dummy.name + i))); i++)
            ;

         /* player_table[nr].id         = dummy.char_specials_saved.idnum; */
	 player_table[nr].id = top_idnum++;
         player_table[nr].level      = dummy.level;
         player_table[nr].hostname   = str_dup(dummy.host);
         player_table[nr].plr_flags  = dummy.char_specials_saved.act;
         player_table[nr].last_logon = dummy.last_logon;

         for(i=0;i<5;i++)
            player_table[nr].gold[i]=dummy.points.gold[i];
         for(i=0;i<32;i++)
            player_table[nr].bank_gold[i]=dummy.points.bank_gold[i];


	 FILE *fp = fopen(EXPLORED_FILE, "r");
	 if (!fp) {
	   log("SYSERR: couldn't open %s", EXPLORED_FILE);
	   exit(10);
	 }
	 int id = nr;
	 log("id=%d", id);
	 /*sleep(1);*/
	 if (id >= 0 && id < 100000) {
	   /* The size of a record is the number of bytes in the vector. */
	   int offset = id * EXPLORED_BYTES;
	   fseek(fp, offset, SEEK_SET);
	   fread(&dummy.explored_vnums, sizeof(char), EXPLORED_BYTES, fp);
         } else {
	   memset(&dummy.explored_vnums, 0, EXPLORED_BYTES*sizeof(char));
	 }
	 fclose(fp);





         /* top_idnum = MAX(top_idnum, dummy.char_specials_saved.idnum); */
	 dummy.char_specials_saved.idnum = player_table[nr].id;
	 log("Assigning %s id %ld.", dummy.name, dummy.char_specials_saved.idnum);
	 save_char_ascii(&dummy);
	 }
      }

   top_of_p_file = top_of_p_table = nr-1;
   log("Writing master player index file...");
   write_player_index_file();
   exit(0);
#endif


   //   log("   %ld is top_idnum.",top_idnum);
   /*
     for(i=0;i<top_of_p_table;i++)
     log("%-20.20s %6ld %3d %s",player_table[i].name, player_table[i].id,
     player_table[i].level,player_table[i].hostname);
     */

   //exit(0);
   }

void load_player_index_file(void)
{
   FILE *fp;
   fp = fopen("etc/players_ascii/index", "r");
   if (!fp) {
     fp = fopen("etc/players_ascii/index", "a+");
   }
   if (!fp) {
     perror("SYSERR: fatal error opening player index file.");
     exit(1);
   }
   char *buf = get_buffer(MAX_STRING_LENGTH);
   int recs, i;

   player_table = NULL;
   top_idnum = 0;
   top_of_p_file = 0;
   top_of_p_table = 0;
   recs = 0;

   while (TRUE) {
     read_line_ascii(fp, buf, MAX_STRING_LENGTH);
     if (!buf || !*buf) {
       break;
     }
     player_table = (struct player_index_element *)realloc(player_table, (++recs) * sizeof(struct player_index_element));
     struct player_index_element *el = &player_table[recs-1];
     el->name = strdup(buf);
     read_line_ascii(fp, buf, MAX_STRING_LENGTH);
     buf[HOST_LENGTH] = '\x0';
     el->hostname = strdup(buf);
     fscanf(fp, "%ld %d %ld %ld\n", &el->id, &el->level, &el->plr_flags, &el->last_logon);
     fscanf(fp, "%ld %ld %ld %ld %ld\n", &el->gold[0], &el->gold[1], &el->gold[2], &el->gold[3], &el->gold[4]);
     for (i = 0; i < 32; i++) {
       fscanf(fp, "%ld ", &el->bank_gold[i]);
     }
     fscanf(fp, "\n");
     top_idnum = MAX(top_idnum, el->id);
   }
   fclose(fp);

   log("top_idnum = %ld", top_idnum);
   top_of_p_file = recs-1;
   top_of_p_table = recs-1;
   release_buffer(buf);
}

void write_player_index_file(void)
{
   FILE *fp;
   fp = fopen("etc/players_ascii/index", "w+");
   if (!fp) {
     log("SYSERR: write_player_index_file could not open player index file for writing.");
     return;
   }
   
   char *buf = get_buffer(MAX_STRING_LENGTH);
   int i, j;

   for (i = 0; i <= top_of_p_table; i++) {
     struct player_index_element *el = &player_table[i];
     if (!el->name || !el->name[0]) {
       log("SYSERR: found NULL player table entry!");
       continue;
     }
     fputs(el->name, fp);
     fputs("\n", fp);
     if (!el->hostname || !el->hostname[0]) {
       fputs("(null)", fp);
     } else {
       fputs(el->hostname, fp);
     }
     fputs("\n", fp);
     fprintf(fp, "%ld %d %ld %ld\n", el->id, el->level, el->plr_flags, el->last_logon);
     fprintf(fp, "%ld %ld %ld %ld %ld\n", el->gold[0], el->gold[1], el->gold[2], el->gold[3], el->gold[4]);
     for (j = 0; j < 32; j++) {
       fprintf(fp, "%ld ", el->bank_gold[j]);
     }
     fputs("\n", fp);
   }   

   release_buffer(buf);
   fclose(fp);
}


/* function to count how many hash-mark delimited records exist in a file */
int count_hash_records(FILE * fl)
   {
   char *buf=get_buffer(128);
   int count = 0;

   while (fgets(buf, 128, fl))
      if (*buf == '#')
         count++;

   release_buffer(buf);
   return count;
   }



void index_boot(int mode)
   {
   char *index_filename, *prefix=NULL;
   FILE *findex, *db_file;
   int rec_count = 0;
   char *buf1=get_buffer(SMALL_BUFSIZE);
   char *buf2=get_buffer(SMALL_BUFSIZE);

   switch (mode)
      {
      case DB_BOOT_WLD:
         prefix = WLD_PREFIX;
         break;
      case DB_BOOT_MOB:
         prefix = MOB_PREFIX;
         break;
      case DB_BOOT_OBJ:
         prefix = OBJ_PREFIX;
         break;
      case DB_BOOT_ZON:
         prefix = ZON_PREFIX;
         break;
      case DB_BOOT_PTH:
         prefix = PTH_PREFIX;
         break;
      case DB_BOOT_SHP:
         prefix = SHP_PREFIX;
         break;
      case DB_BOOT_HLP:
         prefix = HLP_PREFIX;
         break;
      case DB_BOOT_GLD:
         prefix = GLD_PREFIX;
         break;
      case DB_BOOT_TRG:
         prefix = TRG_PREFIX;
         break;
      default:
         log("SYSERR: Unknown subcommand to index_boot!");
         exit(1);
         break;
      }

   if (mini_mud)
      index_filename = MINDEX_FILE;
   else
      index_filename = INDEX_FILE;

   if (mode == DB_BOOT_OBJ) {
     index_filename = INDEX_FILE;
   }

   sprintf(buf2, "%s/%s", prefix, index_filename);

   if (!(findex = fopen(buf2, "r")))
      {
      log("SYSERR: Error opening index file '%s' :%s", buf2, strerror(errno));
      exit(1);
      }

   /* first, count the number of records in the file so we can malloc */
   fscanf(findex, "%s\n", buf1);
   while (*buf1 != '$')
      {
      sprintf(buf2, "%s/%s", prefix, buf1);
      if (!(db_file = fopen(buf2, "r")))
         {
         log("SYSERR: File '%s' listed in '%s/%s': %s", buf2, prefix,
             index_filename, strerror(errno));
         fscanf(findex,"%s\n",buf1);
         continue;
         }
      else
         {
         if (mode == DB_BOOT_ZON)
            rec_count++;
         else
            rec_count += count_hash_records(db_file);
         }

      fclose(db_file);
      fscanf(findex, "%s\n", buf1);
      }

   /* Exit if 0 records, unless this is shops */
   if (!rec_count)
      {
      if ((mode == DB_BOOT_SHP) ||
              (mode == DB_BOOT_GLD) ||
              (mode == DB_BOOT_TRG) ||
              (mode == DB_BOOT_PTH))
         {
         release_buffer(buf2);
         release_buffer(buf1);
         fclose(findex);
         return;
         }
      log("SYSERR: boot error - 0 records counted");
      exit(1);
      }

   rec_count++;

   switch (mode)
      {
      case DB_BOOT_WLD:
         CREATE(world, struct room_data, rec_count);
         log("   %d rooms, %d bytes.", rec_count,
             sizeof(struct room_data) * rec_count);
         break;
      case DB_BOOT_MOB:
         CREATE(mob_proto, struct char_data, rec_count);
         CREATE(mob_index, struct index_data, rec_count);
         log("   %d mobs, %d bytes in index, %d bytes in prototypes.",
             rec_count, sizeof(struct index_data) * rec_count,
             sizeof(struct char_data) * rec_count);
         break;
      case DB_BOOT_OBJ:
         CREATE(obj_proto, struct obj_data, rec_count);
         CREATE(obj_index, struct index_data, rec_count);
         log("   %d objs, %d bytes in index, %d bytes in prototypes.",
             rec_count, sizeof(struct index_data) * rec_count,
             sizeof(struct obj_data) * rec_count);
         break;
      case DB_BOOT_ZON:
         CREATE(zone_table, struct zone_data, rec_count);
         log("   %d zones, %d bytes.", rec_count,
             sizeof(struct zone_data) * rec_count);
         break;
      case DB_BOOT_PTH:
         CREATE(path_index , struct path_data, rec_count);
         log("   %d paths, %d bytes.", rec_count,
             sizeof(struct path_data) * rec_count);
         break;
      case DB_BOOT_HLP:
         CREATE(help_table, struct help_index_element, rec_count);
         log("   %d entries(aliases), %d bytes.", rec_count,
             sizeof(struct help_index_element) * rec_count);
         break;
      case DB_BOOT_TRG:
         CREATE(trig_index, struct index_data *, rec_count);
         log("   %d triggers, %d bytes in index.", rec_count,
             sizeof(struct index_data) * rec_count);
         break;
      }

   rewind(findex);
   fscanf(findex, "%s\n", buf1);
   while (*buf1 != '$')
      {
      sprintf(buf2, "%s/%s", prefix, buf1);
      if (!(db_file = fopen(buf2, "r")))
         {
         log("SYSERR: %s: %s", buf2, strerror(errno));
         exit(1);
         }
      switch (mode)
         {
         case DB_BOOT_WLD:
         case DB_BOOT_OBJ:
         case DB_BOOT_MOB:
         case DB_BOOT_PTH:
         case DB_BOOT_TRG:
            discrete_load(db_file, mode,buf2);
            break;
         case DB_BOOT_ZON:
            load_zones(db_file, buf2);
            break;
         case DB_BOOT_HLP:
            load_help(db_file,buf1);
            break;
         case DB_BOOT_SHP:
            boot_the_shops(db_file, buf2, rec_count);
            break;
         case DB_BOOT_GLD:
            boot_the_guilds(db_file, buf2, rec_count);
            break;
         }

      fclose(db_file);
      fscanf(findex, "%s\n", buf1);
      }
   fclose(findex);

   release_buffer(buf2);
   release_buffer(buf1);
   }


void discrete_load(FILE * fl, int mode,char *filename)
   {
   long nr = -1;
   char *line=get_buffer(256);
   long last=0;
   int version;

   char *modes[] =
      {"world", "mob", "obj","zone","shop","help","guild","path","trg" } ;
   version = 1;
   for (;;)
      {
      /*
       * we have to do special processing with the obj files because they have
       * no end-of-record marker :(
       */
      if (mode != DB_BOOT_OBJ || nr < 0)
         {
         if (!get_line(fl, line))
            {
            if (nr == -1)
               {
               log("SYSERR: %s file %s is empty!", modes[mode], filename);
               }
            else
               {
               log("SYSERR: Format error in %s after %s #%ld\n"
                   "SYSERR: ...expecting a new %s, but file ended!\n"
                   "SYSERR: (maybe the file is not terminated with '$'?)",
                   filename,
                   modes[mode], nr, modes[mode]);
               }
            exit(1);
            }
         }

      if (*line == '$')
         {
         release_buffer(line);
         return;
         }
      else if(*line=='@')
         {
         if(sscanf(line,"@Version: %d",&version)!=1)
            {
            log("SYSERR: Format error after %s #%ld", modes[mode], last);
            log("SYSERR: ...Line: %s",line);
            exit(1);
            }
         }
      else if (*line == '#')
         {
         last = nr;
         if (sscanf(line, "#%ld", &nr) != 1)
            {
            log("SYSERR: Format error after %s #%ld", modes[mode], last);
            log("SYSERR: ...Line: %s",line);
            exit(1);
            }
         if ((nr == 99999) ||(nr>=330000))
            {
            release_buffer(line);
            return;
            }
         else
            switch (mode)
               {
               case DB_BOOT_WLD:
                  parse_room(fl, nr, version);
                  break;
               case DB_BOOT_MOB:
                  parse_mobile(fl, nr, version);
                  break;
               case DB_BOOT_PTH:
                  parse_path(fl, nr, version);
                  break;
               case DB_BOOT_OBJ:
                  strcpy(line, parse_object(fl, nr, version));
                  break;
               case DB_BOOT_TRG:
                  parse_trigger(fl, nr);
                  break;
               }
         }
      else
         {
         log("SYSERR: Format error in %s file near %s #%ld", modes[mode],
             modes[mode], nr);
         log("SYSERR: ... Offending line: '%s'", line);
         exit(1);
         }
      }
   release_buffer(line);
   }


bitvector_t asciiflag_conv(char *flag)
   {
   bitvector_t flags = 0;
   int is_a_number = 1;
   register char *p;

   for (p = flag; *p; p++)
      {
      if (islower((int)*p))
         flags |= 1 << (*p - 'a');
      else if (isupper((int)*p))
         flags |= 1 << (26 + (*p - 'A'));

      if (!isdigit((int)*p))
         is_a_number = 0;
      }

   if (is_a_number)
      flags = atol(flag);

   return flags;
   }

void flagascii_conv(char *flag,bitvector_t vector)
   {
   flag[0]='\0';
   if(vector&(1<<0))
      strcat(flag,"a");
   if(vector&(1<<1))
      strcat(flag,"b");
   if(vector&(1<<2))
      strcat(flag,"c");
   if(vector&(1<<3))
      strcat(flag,"d");
   if(vector&(1<<4))
      strcat(flag,"e");
   if(vector&(1<<5))
      strcat(flag,"f");
   if(vector&(1<<6))
      strcat(flag,"g");
   if(vector&(1<<7))
      strcat(flag,"h");
   if(vector&(1<<8))
      strcat(flag,"i");
   if(vector&(1<<9))
      strcat(flag,"j");
   if(vector&(1<<10))
      strcat(flag,"k");
   if(vector&(1<<11))
      strcat(flag,"l");
   if(vector&(1<<12))
      strcat(flag,"m");
   if(vector&(1<<13))
      strcat(flag,"n");
   if(vector&(1<<14))
      strcat(flag,"o");
   if(vector&(1<<15))
      strcat(flag,"p");
   if(vector&(1<<16))
      strcat(flag,"q");
   if(vector&(1<<17))
      strcat(flag,"r");
   if(vector&(1<<18))
      strcat(flag,"s");
   if(vector&(1<<19))
      strcat(flag,"t");
   if(vector&(1<<20))
      strcat(flag,"u");
   if(vector&(1<<21))
      strcat(flag,"v");
   if(vector&(1<<22))
      strcat(flag,"w");
   if(vector&(1<<23))
      strcat(flag,"x");
   if(vector&(1<<24))
      strcat(flag,"y");
   if(vector&(1<<25))
      strcat(flag,"z");
   if(vector&(1<<26))
      strcat(flag,"A");
   if(vector&(1<<27))
      strcat(flag,"B");
   if(vector&(1<<28))
      strcat(flag,"C");
   if(vector&(1<<29))
      strcat(flag,"D");
   if(vector&(1<<30))
      strcat(flag,"E");
   if(vector&(1<<31))
      strcat(flag,"F");
   if(flag[0]=='\0')
      strcpy(flag,"0");
   }

char fread_letter(FILE *fp)
   {
   char c;
   do
      {
      c = getc(fp);
      }
   while (isspace((int)c));
   return c;
   }


/* PATCH HERE */
/* load the rooms */
void parse_room(FILE * fl, int virtual_nr, int version)
   {
   static int room_nr = 0, zone = 0;
   int t[10], i;
   int already_read=FALSE;
   int result;
   char letter;
   char *line=get_buffer(256);
   char *flags=get_buffer(128);
   char *flags2=get_buffer(128);
   char *buf2=get_buffer(SMALL_BUFSIZE);
   char *buf=get_buffer(SMALL_BUFSIZE);
   struct extra_descr_data *new_descr;
   struct teleport_data *new_tele;


   sprintf(buf2, "room #%d", virtual_nr);

   if (virtual_nr <= (zone ? zone_table[zone - 1].top : -1))
      {
      log("SYSERR: Room #%d is below zone %d.", virtual_nr, zone);
      exit(1);
      }
   while (virtual_nr > zone_table[zone].top)
      if (++zone > top_of_zone_table)
         {
         log("SYSERR: Room %d is outside of any zone.", virtual_nr);
         exit(1);
         }
   world[room_nr].zone        = zone;
   world[room_nr].number      = virtual_nr;
   world[room_nr].name        = fread_string(fl, buf2);
   world[room_nr].description = fread_string(fl, buf2);

   world[room_nr].tele        = NULL;
   world[room_nr].func        = NULL;
   world[room_nr].contents    = NULL;
   world[room_nr].people      = NULL;
   world[room_nr].affected    = NULL;
   world[room_nr].script      = NULL;
   world[room_nr].proto_script= NULL;
   world[room_nr].light       = 0; /* Zero light sources */
   world[room_nr].ex_description = NULL;
   for (i = 0; i < NUM_OF_DIRS; i++)
      world[room_nr].dir_option[i] = NULL;

   for (i=0;i<NUM_ORE_SLOTS;i++)
      {
      world[room_nr].ore_types[i]=NOTHING;
      world[room_nr].ore_percent[i]=0;
      }

   if (!get_line(fl, line))
      {
      log("SYSERR: Expecting roomflags/sector type of room #%d but file"
          " ended!", virtual_nr);
      exit(1);
      }
   if (version <= 1)
      {
      strcpy(flags2,"0");
      if (sscanf(line, " %d %s %d ", t, flags, t + 2) != 3)
         {
         log("SYSERR: Format error in roomflags/sector type of room #%d",
              virtual_nr);
         exit(1);
         }
      }
   else
      {
      if (sscanf(line, " %d %s %s %d ", t, flags, flags2, t + 2) != 4)
         {
         log("SYSERR: Format error in roomflags/sector type of room #%d",
              virtual_nr);
         exit(1);
         }
      }
   /* t[0] is the zone number; ignored with the zone-file system */
   world[room_nr].room_flags = asciiflag_conv(flags);
   world[room_nr].room2_flags = asciiflag_conv(flags2);
   world[room_nr].sector_type = t[2];


   sprintf(buf, "SYSERR: Format error in room #%d (expecting D/E/T/S)", virtual_nr);
   /*
    * end   add - Bon 07/25/97
    */

   for (;;)
      {
      if (already_read==FALSE)
         if(!get_line(fl, line))
            {
            log("%s", buf);
            exit(1);
            }
      switch (*line)
         {
         case 'D':
            if(atoi(line+1)>=NUM_OF_DIRS)
               {
               char *tmp_buf=get_buffer(256);
               fread_string(fl,tmp_buf);
               fread_string(fl,tmp_buf);
               get_line(fl,tmp_buf);
               }
            else
               setup_dir(fl, room_nr, atoi(line + 1),version);
            break;
         case 'O':
            get_line(fl,line);
            if(sscanf(line, "%d %d %d", t, t+1, t+2)!=3)
               log("SYSERR: Invalid O command in %d: %s",virtual_nr,line);
            else
               {
               world[room_nr].ore_types[t[0]]   = t[1];
               world[room_nr].ore_percent[t[0]] = t[2];
               }
            break;
         case 'E':
            CREATE(new_descr, struct extra_descr_data, 1);
            new_descr->keyword = fread_string(fl, buf2);
            new_descr->description = fread_string(fl, buf2);
            new_descr->next = world[room_nr].ex_description;
            world[room_nr].ex_description = new_descr;
            break;
         case 'T':

            if (world[room_nr].tele==NULL)
               {
               get_line(fl, line);
               result=sscanf(line, " %d %d %d %s",t,t+1,t+2,flags);
               if(result==2)
                  {
                  CREATE(new_tele,struct teleport_data,1);
                  world[room_nr].tele = new_tele;
                  world[room_nr].tele->targ = t[0];
                  world[room_nr].tele->time = t[1];
                  world[room_nr].tele->obj  = -1;
                  world[room_nr].tele->bitvector = TELE_LOOK|TELE_COUNT;
                  world[room_nr].tele->to_char=NULL;
                  world[room_nr].tele->to_source_room=NULL;
                  world[room_nr].tele->to_targ_room=NULL;
                  }
               else if(result==4)
                  {
                  CREATE(new_tele,struct teleport_data,1);
                  world[room_nr].tele = new_tele;
                  world[room_nr].tele->targ = t[0];
                  world[room_nr].tele->time = t[1];
                  world[room_nr].tele->obj  = t[2];
                  world[room_nr].tele->bitvector = asciiflag_conv(flags);
                  world[room_nr].tele->to_char=NULL;
                  world[room_nr].tele->to_source_room=NULL;
                  world[room_nr].tele->to_targ_room=NULL;
                  }
               else
                  {
                  log("SYSERR: Invalid T entry in room #%d (expect int int"
                      " int str)\nSYSERR: ... Got: %s",virtual_nr,line);
                  }
               }
            else
               {
               log("SYSERR: Too many T fields (1 max), %s\n", buf2);
               log("%ld %d %ld",world[room_nr].tele->targ,
                   world[room_nr].tele->time,world[room_nr].tele->obj);
               }
            break;
         case 't':
            if (world[room_nr].tele!=NULL)
               {
               world[room_nr].tele->to_char=fread_string(fl,buf2);
               world[room_nr].tele->to_source_room=fread_string(fl,buf2);
               world[room_nr].tele->to_targ_room=fread_string(fl,buf2);
               }
            else
               {
               log("SYSERR: tried to have a t field without a T fiels, %s",
                   buf2);
               }
            break;
         case 'S':   /* end of room */
            /* DG triggers -- script is defined after the end of the room */
            letter = fread_letter(fl);
            ungetc(letter, fl);
            while (letter=='T')
               {
               dg_read_trigger(fl, &world[room_nr], WLD_TRIGGER);
               letter = fread_letter(fl);
               ungetc(letter, fl);
               }
            top_of_world = room_nr++;
            release_buffer(buf);
            release_buffer(buf2);
            release_buffer(line);
            release_buffer(flags);
            release_buffer(flags2);
            return;
            break;
         default:
            log("%s", buf);
            exit(1);
            break;
         }
      }
   }



/* read direction data */
void setup_dir(FILE * fl, int room, int dir,int version)
   {
   int t[5];
   char *line=get_buffer(256);
   char *buf2=get_buffer(256);

   sprintf(buf2, "room #%ld, direction D%d", GET_ROOM_VNUM(room), dir);

   CREATE(world[room].dir_option[dir], struct room_direction_data, 1);
   world[room].dir_option[dir]->general_description = fread_string(fl, buf2);
   world[room].dir_option[dir]->keyword = fread_string(fl, buf2);

   if (!get_line(fl, line))
      {
      log("SYSERR: Format error, %s\n", buf2);
      exit(1);
      }
   if(version==1)
      {
      if (sscanf(line, " %d %d %d ", t, t + 1, t + 2) != 3)
         {
         log("SYSERR: Format error, %s\n", buf2);
         exit(1);
         }
      world[room].dir_option[dir]->lcklevl = 0;
      }
   else
      {
      if (sscanf(line, " %d %d %d %d ", t, t + 1, t + 2, t + 3) != 4)
         {
         log("SYSERR: Format error, %s\n", buf2);
         exit(1);
         }
      world[room].dir_option[dir]->lcklevl = t[3];
      }

   if (t[0] & EX_PICKPROOF)
      SET_BIT(t[0],EX_ISDOOR);
   if(t[0]&EX_LOCKED)
      REMOVE_BIT(t[0],EX_LOCKED);
   if(t[0]&EX_CLOSED)
      REMOVE_BIT(t[0],EX_CLOSED);

   world[room].dir_option[dir]->exit_info = t[0];
   world[room].dir_option[dir]->key = t[1];
   world[room].dir_option[dir]->to_room = t[2];
   release_buffer(buf2);
   release_buffer(line);
   }


/* make sure the start rooms exist & resolve their vnums to rnums */
void check_start_rooms(void)
   {
   if ((r_mortal_start_room = real_room(mortal_start_room)) < 0)
      {
      log("SYSERR:  Mortal start room does not exist.  Change in config.c.");
      exit(1);
      }



   if ((r_immort_start_room = real_room(immort_start_room)) < 0)
      {
      if (!mini_mud)
         log("SYSERR:  Warning: Immort start room does not exist.  Change in config.c.");
      r_immort_start_room = r_mortal_start_room;
      }
   if ((r_frozen_start_room = real_room(frozen_start_room)) < 0)
      {
      if (!mini_mud)
         log("SYSERR:  Warning: Frozen start room does not exist.  Change in config.c.");
      r_frozen_start_room = r_mortal_start_room;
      }
   }


/* resolve all vnums into rnums in the world */
void renum_world(void)
   {
   register int room, door;

   for (room = 0; room <= top_of_world; room++)
      for (door = 0; door < NUM_OF_DIRS; door++)
         if (world[room].dir_option[door])
            if (world[room].dir_option[door]->to_room != NOWHERE)
               world[room].dir_option[door]->to_room
               = real_room(world[room].dir_option[door]->to_room);
   }


#define ZCMD zone_table[zone].cmd[cmd_no]

/* resulve vnums into rnums in the zone reset tables */
void renum_zone_table(void)
   {
   int  cmd_no, a, b,c,olda,oldb,oldc;
   room_vnum roomnum;
   zone_rnum zone;

   for (zone = 0; zone <= top_of_zone_table; zone++)
      for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++)
         {
         a = b = c =0;
         olda=ZCMD.arg1;
         oldb=ZCMD.arg2;
         oldc=ZCMD.arg3;
         ZCMD.oarg1=ZCMD.arg1;
         ZCMD.oarg2=ZCMD.arg2;
         ZCMD.oarg3=ZCMD.arg3;
         ZCMD.oarg4=ZCMD.arg4;

         switch (ZCMD.command)
            {
            case 'M':
               roomnum=ZCMD.arg3;
               a = ZCMD.arg1 = real_mobile(ZCMD.arg1);
               c = ZCMD.arg3 = real_room(ZCMD.arg3);
               break;
            case 'O':
               roomnum=ZCMD.arg3;
               a = ZCMD.arg1 = real_object(ZCMD.arg1);
               if (ZCMD.arg3 != NOWHERE)
                  c = ZCMD.arg3 = real_room(ZCMD.arg3);
               break;
            case 'G':
               a = ZCMD.arg1 = real_object(ZCMD.arg1);
               break;
            case 'E':
               a = ZCMD.arg1 = real_object(ZCMD.arg1);
               break;
            case 'P':
               a = ZCMD.arg1 = real_object(ZCMD.arg1);
               c = ZCMD.arg3 = real_object(ZCMD.arg3);
               break;
            case 'D':
               roomnum=ZCMD.arg1;
               a = ZCMD.arg1 = real_room(ZCMD.arg1);
               break;
            case 'R': /* rem obj from room */
               roomnum=ZCMD.arg1;
               a = ZCMD.arg1 = real_room(ZCMD.arg1);
               b = ZCMD.arg2 = real_object(ZCMD.arg2);
               break;
            case 'T': /* a trigger */
               /* designer's choice: convert this later */
               b = ZCMD.arg2 = real_trigger(ZCMD.arg2);
               /*b = real_trigger(ZCMD.arg2);*/ /* leave this in for validation */
               break;
            case 'V': /* trigger variable assignment */
               if (ZCMD.arg1 == WLD_TRIGGER)
                  b = ZCMD.arg2 = real_room(ZCMD.arg2);
               break;

            }
         if ((a < 0) || (b < 0) || (c < 0))
            {
            if (!mini_mud)
               {
               char *buf=get_buffer(128);
               sprintf(buf,"Invalid vnum %d, cmd disabled",
                       (a < 0) ? olda : ((b < 0) ? oldb : oldc));
               log_zone_error(zone, cmd_no, buf,roomnum);
               release_buffer(buf);
               }
            ZCMD.command = '*';
            }
         }
   }


void set_zone_room_counts(void)
{
  int znum;
  for (znum = 0; znum <= top_of_zone_table; znum++)
  {
    zone_table[znum].num_rooms = 0;
  }
  int rnum;
  for (rnum = 0; rnum <= top_of_world; rnum++)
  {
    int zone = world[rnum].zone;
    if (zone >= 0 && zone <= top_of_zone_table)
    {
      zone_table[zone].num_rooms++;
    }
  }
}

void parse_simple_mob(FILE *mob_f, int i, int nr, int version)
   {
   int j, t[10];
   char *line=get_buffer(256);



   mob_proto[i].mob_specials.maxfactor = 255;

   for(j=0;j<10;j++)
      mob_proto[i].mob_specials.special_value[j]=0;


   if (!get_line(mob_f, line)||(sscanf(line, " %d %d %d %dd%d+%d %dd%d+%d ",
                                       t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6, t + 7, t + 8) != 9))
      {
      log("SYSERR: Format error in mob #%d, first line after S flag\n"
          "SYSERR: ...expecting line of form '# # # #d#+# #d#+#'\n", nr);
      exit(1);
      }
   if(t[0]<1)
      t[0]=1;
   else if(t[0]>LVL_IMPL)
      t[0]=LVL_IMPL;

   GET_LEVEL(mob_proto + i) = t[0];
   mob_proto[i].points.hitroll = 20 - t[1];
   mob_proto[i].points.armor = 10 * t[2];

   /* max hit = 0 is a flag that H, M, V is xdy+z */
   mob_proto[i].points.max_hit = 0;
   mob_proto[i].points.hit = t[3];
   mob_proto[i].points.mana = t[4];
   mob_proto[i].points.move = t[5];

   mob_proto[i].real_abils.str
   = (dice(6,3)*race_stat_info[GET_RACE(mob_proto+i)].Str)/100;

   if((mob_proto[i].real_abils.str>12)&&(GET_LEVEL(mob_proto+i)<10))
      mob_proto[i].real_abils.str=12;
   else if((mob_proto[i].real_abils.str>14)&&(GET_LEVEL(mob_proto+i)<20))
      mob_proto[i].real_abils.str=14;
   else if((mob_proto[i].real_abils.str>16)&&(GET_LEVEL(mob_proto+i)<30))
      mob_proto[i].real_abils.str=16;
   else if((mob_proto[i].real_abils.str>18)&&(GET_LEVEL(mob_proto+i)<40))
      mob_proto[i].real_abils.str=18;
   else if((mob_proto[i].real_abils.str>20)&&(GET_LEVEL(mob_proto+i)<50))
      mob_proto[i].real_abils.str=20;

   mob_proto[i].real_abils.intel
   = (dice(6,4)*race_stat_info[GET_RACE(mob_proto+i)].Int)/100;
   mob_proto[i].real_abils.wis
   = (dice(6,4)*race_stat_info[GET_RACE(mob_proto+i)].Wis)/100;
   mob_proto[i].real_abils.dex
   = (dice(6,4)*race_stat_info[GET_RACE(mob_proto+i)].Dex)/100;
   mob_proto[i].real_abils.con
   = (dice(6,4)*race_stat_info[GET_RACE(mob_proto+i)].Con)/100;
   mob_proto[i].real_abils.cha
   = (dice(6,4)*race_stat_info[GET_RACE(mob_proto+i)].Cha)/100;

   mob_proto[i].points.max_mana = (GET_LEVEL(mob_proto+i)*npc_class_mult[(int)GET_CLASS(mob_proto+i)].npc_mana);
   mob_proto[i].points.max_move = 200;

   mob_proto[i].mob_specials.damnodice = t[6];
   mob_proto[i].mob_specials.damsizedice = t[7];
   mob_proto[i].points.damroll = t[8];

   if(!get_line(mob_f, line) ||(sscanf(line, " %d %d ", t, t + 1)!=2))
      {
      log("SYSERR: Format error in mob #%d, second line after S flag\n"
          "SYSERR: ...expecting line of form '# #'\n", nr);
      exit(1);
      }

   GET_GOLD(mob_proto + i) = t[0];
   GET_EXP(mob_proto + i) = t[1];

   if(!get_line(mob_f, line) ||
           (sscanf(line, " %d %d %d %d ", t, t + 1, t + 2, t + 3) != 3) )
      {
      log("SYSERR: Format error in mob #%d, second line after S flag\n"
          "SYSERR: ...expecting line of form '# # #'\n", nr);
      exit(1);
      }

   if((t[0]==POS_CHANT)||(t[0]==POS_MEDITATE)||(t[0]==POS_BANDAGE))
      {
      /* log("POSFIX: mob: %d is in pos %d, is this right?",nr,t[0]); */
      t[0]+=3;
      }
   if((t[1]==POS_CHANT)||(t[1]==POS_MEDITATE)||(t[1]==POS_BANDAGE))
      {
      /* log("POSFIX: mob: %d is in default pos %d, is this right?",nr,t[1]); */
      t[1]+=3;
      }
   mob_proto[i].char_specials.position = t[0];
   mob_proto[i].mob_specials.default_pos = t[1];
   mob_proto[i].player.sex = t[2];

   mob_proto[i].player.weight = 200;
   mob_proto[i].player.height = 198;


   /*
    * these are now save applies; base save numbers for MOBs are now from
    * the warrior save table.
    */
   for (j = 0; j < 5; j++)
      GET_SAVE(mob_proto + i, j) = 0;
   release_buffer(line);
   }


/*
 * interpret_espec is the function that takes espec keywords and values
 * and assigns the correct value to the mob as appropriate.  Adding new
 * e-specs is absurdly easy -- just add a new CASE statement to this
 * function!  No other changes need to be made anywhere in the code.
 */

#define CASE(test) if (!matched && !str_cmp(keyword, test) && (matched = 1))
#define RANGE(low, high) (num_arg = MAX((low), MIN((high), (num_arg))))

void interpret_espec(char *keyword, char *value, int i, int nr)
   {
   int num_arg;
   int retval,j;
   int t[10];


   num_arg = atoi(value);
   if(!strcmp(keyword,"Special"))
      {
      if(!strcmp(value,"None"))
         {
         REMOVE_BIT(MOB_FLAGS(mob_proto+i),MOB_SPEC);
         mob_index[i].func=NULL;
         }
      else if(!strcmp(value,"shop_keeper"))
         {
         REMOVE_BIT(MOB_FLAGS(mob_proto+i),MOB_SPEC);
         mob_index[i].func=NULL;
         }
      else
         {
         mob_index[i].func=get_mob_spec_proc(value);
         SET_BIT(MOB_FLAGS(mob_proto+i),MOB_SPEC);
         }
      return;
      }

   else if(!strcmp(keyword,"SpecVal"))
      {
      if((retval=sscanf(value,"%d %d %d %d %d %d %d %d %d %d",t,t+1,t+2,t+3,
                        t+4,t+5,t+6,t+7,t+8,t+9))!=10)
         {
         log("SYSERR: Error in SpecVal espec for mob %d, got %d/10.",
             nr,retval);
         }
      else
         {
         for(j=0;j<10;j++)
            mob_proto[i].mob_specials.special_value[j]=t[j];
         }
      return;
      }

   else if(!strcmp(keyword,"Immune"))
      {
      char *buf1=get_buffer(64);
      char *buf2=get_buffer(64);
      char *buf3=get_buffer(64);
      if((retval=sscanf(value,"%s %s %s",buf1, buf2, buf3))!=3)
         log("SYSERR: Error in Immune espec for mob %d, go %d/3", nr, retval);
      else
         {
         IMMUNE(mob_proto+i)=asciiflag_conv(buf1);
         RESIST(mob_proto+i)=asciiflag_conv(buf2);
         SUCCEPT(mob_proto+i)=asciiflag_conv(buf3);
         }
      release_buffer(buf3);
      release_buffer(buf2);
      release_buffer(buf1);
      return;
      }

   else if(!strcmp(keyword,"BareHandAttack"))
      {
      RANGE(0, 99);
      mob_proto[i].mob_specials.attack_type = num_arg;
      return;
      }
   else if(!strcmp(keyword,"Skin"))
      {
      if(num_arg==0)  /* silly little error */
         {
         mob_proto[i].mob_specials.skin = NOTHING;
         return;
         }
      mob_proto[i].mob_specials.skin = num_arg;
      return;
      }

   /*   if(!strcmp(keyword,"Str"))
         {
         RANGE(3, 125);
         mob_proto[i].real_abils.str = num_arg;
         return;
         }

      if(!strcmp(keyword,"StrAdd"))
         {
         RANGE(0, 100);
         mob_proto[i].real_abils.str_add = num_arg;
         return;
         }

      if(!strcmp(keyword,"Int"))
         {
         RANGE(3, 125);
         mob_proto[i].real_abils.intel = num_arg;
         return;
         }

      if(!strcmp(keyword,"Wis"))
         {
         RANGE(3, 125);
         mob_proto[i].real_abils.wis = num_arg;
         return;
         }

      if(!strcmp(keyword,"Dex"))
         {
         RANGE(3, 125);
         mob_proto[i].real_abils.dex = num_arg;
         return;
         }

      if(!strcmp(keyword,"Con"))
         {
         RANGE(3, 125);
         mob_proto[i].real_abils.con = num_arg;
         return;
         }

      if(!strcmp(keyword,"Cha"))
         {
         RANGE(3, 125);
         mob_proto[i].real_abils.cha = num_arg;
         return;
         }

         */
   else if(!strcmp(keyword,"MaxFactor"))
      {
      RANGE(1,255);
      mob_proto[i].mob_specials.maxfactor=num_arg;
      return;
      }

   log("SYSERR: Warning: unrecognized espec keyword %s in mob #%d\n", keyword, nr);
   }

#undef CASE
#undef RANGE

void parse_espec(char *buf, int i, int nr, int version)
   {
   char *ptr;

   if ((ptr = strchr(buf, ':')) != NULL)
      {
      *(ptr++) = '\0';
      while (isspace((int)*ptr))
         ptr++;
      }
   else
      ptr = "";

   interpret_espec(buf, ptr, i, nr);
   }


void parse_enhanced_mob(FILE *mob_f, int i, int nr, int version)
   {
   char *line=get_buffer(256);
   parse_simple_mob(mob_f, i, nr,version);


   while (get_line(mob_f, line))
      {
      if (!strcmp(line, "E")) /* end of the enhanced section */
         {
         release_buffer(line);
         return;
         }
      else if (*line == '#')
         {
         /* we've hit the next mob, maybe? */
         log("SYSERR: Unterminated E section in mob #%d\n", nr);
         exit(1);
         }
      else
         parse_espec(line, i, nr,  version);
      }

   log("SYSERR: Unexpected end of file reached after mob #%d\n", nr);
   exit(1);
   }


void parse_mobile(FILE * mob_f, int nr, int version)
   {
   static int i = 0;
   int j, t[10];
   char *tmpptr, letter;
   char *line=get_buffer(256);
   char *f1=get_buffer(128);
   char *f2=get_buffer(128);
   char *f3=get_buffer(128);
   char *buf2=get_buffer(SMALL_BUFSIZE);

   mob_index[i].vnum   = nr;
   mob_index[i].number = 0;
   mob_index[i].func   = NULL;

   clear_char(mob_proto + i);

   /*
    * Mobiles should NEVER use anything in the 'player_specials' structure.
    * The only reason we have every mob in the game share this copy of the
    * structure is to save newbie coders from themselves. -gg 2/25/98
    */
   mob_proto[i].player_specials = &dummy_mob;
   sprintf(buf2, "mob vnum %d", nr);

   /***** String data *** */
   mob_proto[i].player.name = fread_string(mob_f, buf2);
   tmpptr = mob_proto[i].player.short_descr = fread_string(mob_f, buf2);
   if (tmpptr && *tmpptr)
      if (!str_cmp(fname(tmpptr), "a") || !str_cmp(fname(tmpptr), "an") ||
              !str_cmp(fname(tmpptr), "the"))
         *tmpptr = LOWER(*tmpptr);
   mob_proto[i].player.long_descr = fread_string(mob_f, buf2);
   mob_proto[i].player.description = fread_string(mob_f, buf2);
   mob_proto[i].player.title = NULL;

   /* *** Numeric data *** */
   if(!get_line(mob_f, line))
      {
      log("SYSERR: Format error in mob #%d, first numeric line\n"
          "SYSERR: ...expection line of form 'str str int char int int'\n"
          "SYSERR: ...got: nothing",nr);
      exit(1);
      }
   else if(version<=1)
      {
      t[3]=t[4]=0;
      strcpy(f3, "0");
      if(sscanf(line, "%s %s %d %c %d %d", f1, f2, t + 2, &letter,t+3,t+4)!=6)
         {
         log("SYSERR: Format error in mob #%d, first numeric line\n"
             "SYSERR: ...expection line of form 'str str int char int int'\n"
             "SYSERR: ...got: %s",nr,line);
         exit(1);
         }
      }
   else
      {
      t[3]=t[4]=0;
      if(sscanf(line, "%s %s %s %d %c %d %d", f1, f3, f2, t + 2, &letter,t+3,t+4)!=7)
         {
         log("SYSERR: Format error in mob #%d, first numeric line\n"
             "SYSERR: ...expection line of form 'str str str int char int int'\n"
             "SYSERR: ...got: %s",nr,line);
         exit(1);
         }
      }
   MOB_FLAGS(mob_proto + i) = asciiflag_conv(f1);
   SET_BIT(MOB_FLAGS(mob_proto + i), MOB_ISNPC);
   REMOVE_BIT(MOB_FLAGS(mob_proto+i), MOB_SPEC|MOB_FLY);
   MOB2_FLAGS(mob_proto +i) = asciiflag_conv(f3);
   AFF_FLAGS(mob_proto + i) = asciiflag_conv(f2);
   REMOVE_BIT(AFF_FLAGS(mob_proto+i),AFF_RAGE|AFF_BLIND|AFF_POISON|AFF_SLEEP);
   GET_ALIGNMENT(mob_proto + i) = t[2];
   GET_CLASS(mob_proto+i)=t[3];
   GET_RACE(mob_proto+i)=t[4];

   switch (UPPER(letter))
      {
      case 'S': /* Simple monsters */
         parse_simple_mob(mob_f, i, nr, version);
         break;
      case 'E': /* Circle3 Enhanced monsters */
         parse_enhanced_mob(mob_f, i, nr, version);
         break;
         /* add new mob types here.. */
      default:
         log("SYSERR: Unsupported mob type '%c' in mob #%d\n", letter,nr);
         exit(1);
         break;
      }
   /* DG triggers -- script info follows mob S/E section */
   mob_proto[i].nr = i;
   mob_proto[i].desc = NULL;
   letter = fread_letter(mob_f);
   ungetc(letter, mob_f);
   while (letter=='T')
      {
      dg_read_trigger(mob_f, &mob_proto[i], MOB_TRIGGER);
      letter = fread_letter(mob_f);
      ungetc(letter, mob_f);
      }

   mob_proto[i].aff_abils = mob_proto[i].real_abils;


   for (j = 0; j < NUM_WEARS; j++)
      mob_proto[i].equipment[j] = NULL;

   letter = fread_letter(mob_f);
   if (letter == '>')
      {
      ungetc(letter, mob_f);
      (void)  mprog_read_programs(mob_f, &mob_index[i]);
      }
   else
      ungetc(letter, mob_f);


   top_of_mobt = i++;
   release_buffer(buf2);
   release_buffer(f3);
   release_buffer(f2);
   release_buffer(f1);
   release_buffer(line);
   }




/*
 * read all objects from obj file; generate index and prototypes
 */
char *parse_object(FILE * obj_f, int nr, int version)
   {
   static int i = 0;
   static char line[256];
   int t[10], j, k, retval;
   char *tmpptr;
   struct extra_descr_data *new_descr;
   char *f1=get_buffer(256);
   char *f2=get_buffer(256);
   char *f3=get_buffer(256);
   char *f4=get_buffer(256);
   char *f5=get_buffer(256);
   char *buf2=get_buffer(SMALL_BUFSIZE);

   obj_index[i].vnum   = nr;
   obj_index[i].number = 0;
   obj_index[i].func   = NULL;

   clear_object(obj_proto + i);

   obj_proto[i].in_room     = NOWHERE;
   obj_proto[i].item_number = i;
   obj_proto[i].touched     = FALSE;

   sprintf(buf2, "object #%d", nr);

   /*
    * ** string data **
    */
   if ((obj_proto[i].name = fread_string(obj_f, buf2)) == NULL)
      {
      log("SYSERR: Null obj name or format error at or near %s\n", buf2);
      exit(1);
      }
   tmpptr = obj_proto[i].short_description = fread_string(obj_f, buf2);
   if (tmpptr&&*tmpptr)
      if (!str_cmp(fname(tmpptr), "a") || !str_cmp(fname(tmpptr), "an") ||
              !str_cmp(fname(tmpptr), "the"))
         *tmpptr = LOWER(*tmpptr);

   tmpptr = obj_proto[i].description = fread_string(obj_f, buf2);
   if (tmpptr && *tmpptr)
      CAP(tmpptr);
   obj_proto[i].action_description = fread_string(obj_f, buf2);

   /*
    * ** numeric line 1 **
    */
   if (version == 1)
      {
      if (!get_line(obj_f, line) ||   
              (retval = sscanf(line, " %d %s %s %s", t, f1, f2, f3)) != 4)
         {
         strcpy(f3,"0"); 
         strcpy(f4,"0");
         strcpy(f5,"0");
         if(retval !=3)
            {
            log("SYSERR: Format error in first numeric line (expecting 4 args, got %d:%s), %s\n",
                retval, line, buf2);
            exit(1);
            }
         }
      }
   else
      {
      if (!get_line(obj_f, line) ||
              (retval = sscanf(line, " %d %s %s %s %s %s", t, f1, f4, f5, f2, f3)) != 6)
         {
         strcpy(f3,"0");
         if(retval !=5)
            {
            log("SYSERR: Format error in first numeric line (expecting 6 args, got %d:%s), %s\n",
                retval, line,buf2);
            exit(1);
            }
         }
      }

   obj_proto[i].obj_flags.type_flag   = t[0];
   obj_proto[i].obj_flags.timer       = -1;
   obj_proto[i].obj_flags.dg_timer    = -1;
   obj_proto[i].obj_flags.extra_flags = asciiflag_conv(f1);
   obj_proto[i].obj_flags.extra_flags2= asciiflag_conv(f4);
   obj_proto[i].obj_flags.extra_flags3= asciiflag_conv(f5);
   obj_proto[i].obj_flags.wear_flags  = asciiflag_conv(f2);
   obj_proto[i].obj_flags.anti_flags  = asciiflag_conv(f3);


   /*
    * ** numeric line 2 **
    */
   t[0]=0;
   t[1]=0;
   t[2]=0;
   t[3]=0;
   t[4]=0;
   t[5]=0;
   t[6]=0;
   t[7]=0;
   if (!get_line(obj_f, line) ||
           (retval = sscanf(line, "%d %d %d %d %d %d %d %d", t, t+1, t+2, t+3, t+4,t+5, t+6, t+7)) != 8)
      {
      if(retval==4)
         {
         log("SYSERR: Format error in second numeric line (expecting 8 or 5 args, got %d:%s), %s\n"
             "SYSERR: Please save using OLC to fix!\n", retval,line, buf2);
         }
      else if(retval!=5)
         {
         log("SYSERR: Format error in second numeric line (expecting 8 or 5 args, got %d:%s), %s\n", retval,line, buf2);
         exit(1);
         }
      }
   obj_proto[i].obj_flags.value[0] = t[0];
   obj_proto[i].obj_flags.value[1] = t[1];
   obj_proto[i].obj_flags.value[2] = t[2];
   obj_proto[i].obj_flags.value[3] = t[3];
   obj_proto[i].obj_flags.value[4] = t[4];
   obj_proto[i].obj_flags.value[5] = t[5];
   obj_proto[i].obj_flags.value[6] = t[6];
   obj_proto[i].obj_flags.value[7] = t[7];


   if((obj_proto[i].obj_flags.value[3]==0)&&(obj_proto[i].obj_flags.type_flag==5))
      log("SYSERR: Weapon %ld has a dam type of TYPE_HIT.  This is reserved for HTH combat.  please change now as it will screw up code --Masque", obj_index[i].vnum);


   /*
    * ** numeric line 3 **
    */
   t[0]=0;
   t[1]=0;
   t[2]=0;
   t[3]=0;
   t[4]=0;
   if (!get_line(obj_f, line) ||
           ((retval = sscanf(line, "%d %d %d %d %d", t, t+1, t+2, t+3,t+4)) != 5))
      {
      if(retval!=3)
         {
         log("SYSERR: Format error in third numeric line (expecting 3 or 5 args, got %d:%s), %s\n", retval,line, buf2);
         exit(1);
         }
      t[3]=5;
      t[4]=5;
      }
   obj_proto[i].obj_flags.weight = t[0];
   obj_proto[i].obj_flags.cost = t[1];
   obj_proto[i].obj_flags.cost_per_day = t[2];
   if((t[3]<0) ||(t[3]>200) ||(t[4]<0) ||(t[4]>200))
      {
      obj_proto[i].obj_flags.curr_dam_slots = 0;
      obj_proto[i].obj_flags.total_dam_slots = 0;
      obj_proto[i].obj_flags.orig_dam_slots = 0;
      }
   else
      {
      obj_proto[i].obj_flags.curr_dam_slots = t[3];
      obj_proto[i].obj_flags.total_dam_slots = t[4];
      obj_proto[i].obj_flags.orig_dam_slots = t[4];
      }

   /* check to make sure that weight of containers exceeds curr. quantity */
   if (obj_proto[i].obj_flags.type_flag == ITEM_DRINKCON ||
           obj_proto[i].obj_flags.type_flag == ITEM_FOUNTAIN)
      {
      if (obj_proto[i].obj_flags.weight < obj_proto[i].obj_flags.value[1])
         obj_proto[i].obj_flags.weight = obj_proto[i].obj_flags.value[1] + 5;
      }

   /*
    * ** Extra item stuff **
    * M - material
    * E - extra desc
    * A - obj affects (+dam)
    * C - AFF_FLAGS
    * S - Spell affects (damage stuff)
    */
   for (j = 0; j < MAX_OBJ_AFFECT; j++)
      {
      obj_proto[i].affected[j].location = APPLY_NONE;
      obj_proto[i].affected[j].modifier = 0;
      }
   for (k = 0; k < MAX_SPELL_AFFECT; k++)
      {
      obj_proto[i].spell_affect[k].spelltype = APPLY_NONE;
      obj_proto[i].spell_affect[k].level = 0;
      obj_proto[i].spell_affect[k].percentage = 0;
      }
   strcat(buf2, ", after numeric constants (expecting E/A/S/#xxx)");

   obj_proto[i].obj_flags.lApplyBits =0;
   for(j=0;j<TOP_APPLY1_NUM;j++)
      obj_proto[i].obj_flags.iApplyMods[j] =0;

   j = 0;
   k = 0;
   obj_proto[i].ex_description = NULL;
   for (;;)
      {
      if (!get_line(obj_f, line))
         {
         log("SYSERR: Format error in %s\n", buf2);
         exit(1);
         }
      switch (*line)
         {
            /* Ponder (04/02/1997) - 'M' is additional attribute for material types */
         case 'M':
            get_line(obj_f, line);
            sscanf(line, " %d ", t);
            obj_proto[i].material = t[0];
            break;
         case 'E':
            CREATE(new_descr, struct extra_descr_data, 1);
            new_descr->keyword = fread_string(obj_f, buf2);
            new_descr->description = fread_string(obj_f, buf2);
            new_descr->next = obj_proto[i].ex_description;
            obj_proto[i].ex_description = new_descr;
            break;
         case 'A':
            if (j >= MAX_OBJ_AFFECT)
               {
               log("SYSERR: Too many A fields (%d max), %s\n", MAX_OBJ_AFFECT, buf2);
               exit(1);
               }
            if(!get_line(obj_f, line)||(sscanf(line, " %d %d ", t, t + 1)!=2))
               {
               log("SYSERR: Format error in 'A' field, %s\n"
                   "SYSERR: ...offending line: '%s'",buf2,line);
               }
            obj_proto[i].affected[j].location = t[0];
            obj_proto[i].affected[j].modifier = t[1];
            SET_BIT( obj_proto[i].obj_flags.lApplyBits, ( 1 << t[0] ) );
            obj_proto[i].obj_flags.iApplyMods[(t[0])] = t[1];
            j++;
            break;
            /* affect player flags (see AFF_XXX flags in structs.h ) */
         case 'C':
            get_line(obj_f, line);
            sscanf(line, "%s ", f1);
            obj_proto[i].obj_flags.bitvector = obj_proto[i].obj_flags.bitvector | asciiflag_conv(f1);
            break;
         case 'S':
            if (k >= MAX_SPELL_AFFECT)
               {
               log("SYSERR: Too many S fields (%d max), %s\n", MAX_SPELL_AFFECT,buf2);
               exit(1);
               }
            get_line(obj_f, line);
            sscanf(line, " %d %d %d ", t, t + 1 , t + 2);
            obj_proto[i].spell_affect[k].spelltype = t[0];
            obj_proto[i].spell_affect[k].level = t[1];
            obj_proto[i].spell_affect[k].percentage = t[2];
            k++;
            break;
         case 'T':  /* DG triggers */
            dg_obj_trigger(line, &obj_proto[i]);
            break;
         case '$':
         case '#':
            check_object(&obj_proto[i]);
            top_of_objt = i++;
            release_buffer(f1);
            release_buffer(f2);
            release_buffer(f3);
            release_buffer(f4);
            release_buffer(f5);
            release_buffer(buf2);
            return line;
            break;
         default:
            log("SYSERR: Format error in %s\n", buf2);
            exit(1);
            break;
         }
      }
   }


#define Zt zone_table[zone]

/* load the zone table and command tables */
void load_zones(FILE * fl, char *zonename)
   {
   static zone_rnum zone = 0;
   int cmd_no = 0, num_of_cmds = 0, line_num = 0, tmp, error ,arg_num;
   room_vnum room_num=0;
   int t1,t2;
   char *ptr;
   char *buf = get_buffer(16384);
   char *buf1 = get_buffer(16384);
   char *zname = get_buffer(16384);
   char *buf2 = get_buffer(16384);
   char *col_buf=get_buffer(16384);
   char *col_ptr=col_buf;
   int newline,dummy;
   int version;

   version =1;
   strcpy(zname, zonename);

   while (get_line(fl, buf))
      num_of_cmds++;  /* this should be correct within 3 or so */
   rewind(fl);

   if (num_of_cmds == 0)
      {
      log( "%s is empty!\n", zname);
      exit(1);
      }
   else
      CREATE(Zt.cmd, struct reset_com, num_of_cmds);

   line_num += get_line(fl, buf);
   if(*buf=='@')
      {
      if(sscanf(buf,"@Version: %d",&version)!=1)
         {
         log("SYSERR: Format error in %s (version)", zname);
         log("SYSERR: ...Line: %s\n",buf);
         exit(1);
         }
      line_num += get_line(fl, buf);
      }


   if (sscanf(buf, "#%ld", &Zt.number) != 1)
      {
      log("SYSERR: Format error in %s, line %d\n", zname, line_num);
      log("SYSERR: ...Line: %s\n",buf);
      exit(1);
      }
   sprintf(buf2, "beginning of zone #%ld", Zt.number);

   line_num += get_line(fl, buf);
   if ((ptr = strchr(buf, '~')) != NULL) /* take off the '~' if it's there */
      *ptr = '\0';
   Zt.name = str_dup(buf);

   if(version>=2)
      {
      line_num += get_line(fl, buf);
      if ((ptr = strchr(buf, '~')) != NULL)
         *ptr = '\0';
      Zt.author = str_dup(buf);

      line_num += get_line(fl, buf);
      if ((ptr = strchr(buf, '~')) != NULL)
         *ptr = '\0';
      Zt.editor = str_dup(buf);

      line_num += get_line(fl, buf);
      if ((ptr = strchr(buf, '~')) != NULL)
         *ptr = '\0';
      Zt.levels = str_dup(buf);
      }
   else
      {
      Zt.author = str_dup("UnSet");
      Zt.editor = str_dup("UnSet");
      Zt.levels = str_dup("UnSet");
      }

   if(version>=4)
      {
      line_num += get_line(fl, buf);
      if(sscanf(buf," %ld %ld %ld ", &Zt.dateStarted, &Zt.dateImped,
                &Zt.dateLastMod) != 3)
         {
         log( "SYSERR: Format error in 3-constant line of %s:%s (date string)", zname,buf);
         exit(1);
         }
      line_num += get_line(fl, buf);
      if ((ptr = strchr(buf, '~')) != NULL)
         *ptr = '\0';
      Zt.nameLastMod = str_dup(buf);
      line_num += get_line(fl, buf);
      if ((ptr = strchr(buf, '~')) != NULL)
         *ptr = '\0';
      Zt.comment = str_dup(buf);
      }
   else
      {
      Zt.dateStarted = time(0);
      Zt.dateImped   = 0;
      Zt.dateLastMod = 0;
      Zt.nameLastMod = str_dup("UnSet");
      Zt.comment     = str_dup("None");
      }
   if(version>=5) 
      {
      line_num += get_line(fl, buf);
      if ((ptr = strchr(buf, '~')) != NULL)
         *ptr = '\0';
      Zt.worldProof = str_dup(buf);
      line_num += get_line(fl, buf);
      if ((ptr = strchr(buf, '~')) != NULL)
         *ptr = '\0';
      Zt.trigProof = str_dup(buf);
      line_num += get_line(fl, buf);
      if ((ptr = strchr(buf, '~')) != NULL)
         *ptr = '\0';
      Zt.objBalanced = str_dup(buf);
      }
   else
      {
      Zt.worldProof    = str_dup("None");
      Zt.trigProof     = str_dup("None");
      Zt.objBalanced   = str_dup("None");
      }
   line_num += get_line(fl, buf);
   if(version<2)
      {
      if(sscanf(buf," %ld %d %d ", &Zt.top, &Zt.lifespan, &Zt.reset_mode) != 3)
         {
         log( "SYSERR: Format error in 3-constant line of %s:%s", zname,buf);
         exit(1);
         }
      /* set continent by color */
      strcpy(col_ptr,Zt.name);
      Zt.continent=strip_zone_color(col_ptr);
      skip_spaces(&col_ptr);
      free(Zt.name);
      Zt.name=strdup(col_ptr);
      Zt.status = 0;
      Zt.source = 0;
      }
   else if(version>=3)
      {
      if (sscanf(buf," %ld %d %d %d %d %d",&Zt.top,&Zt.lifespan,
                 &Zt.reset_mode, &t1,&t2,&Zt.continent)
              !=6)
         {
         log( "SYSERR: Format error in 6-constant line of %s:%s", zname,buf);
         exit(1);
         }
      Zt.status=(char)t1;
      Zt.source=(char)t2;
      if(version<4 && Zt.status==4)
         {
         Zt.dateImped = time(0);
         }
      }
   else if(version>=2)
      {
      if (sscanf(buf," %ld %d %d %d %d",&Zt.top,&Zt.lifespan,
                 &Zt.reset_mode, &t1,&t2)
              !=5)
         {
         log( "SYSERR: Format error in 5-constant line of %s:%s", zname,buf);
         exit(1);
         }
      /* set continent by color */
      strcpy(col_ptr,Zt.name);
      Zt.continent=strip_zone_color(col_ptr);
      skip_spaces(&col_ptr);
      free(Zt.name);
      Zt.name=strdup(col_ptr);
      Zt.status=(char)t1;
      Zt.source=(char)t2;
      }
   cmd_no = 0;

   line_num += get_line(fl, buf);

   if(*buf!='0')
      {
      newline=1;
      Zt.bitvector=0;
      }
   else
      {
      newline=0;
      if(sscanf(buf,"%d %s",&dummy,buf1)!=2)
         {
         log( "SYSERR: Format error in flag line of %s:%s",
              zname,buf);
         exit(1);
         }
      Zt.bitvector=asciiflag_conv(buf1);
      }
   release_buffer(buf1);

   for (;;)
      {
      if(newline==0)
         {
         if ((tmp = get_line(fl, buf)) == 0)
            {
            log("SYSERR: Format error in %s - premature end of file\n",
                zname);
            exit(1);
            }
         line_num += tmp;
         }
      else
         newline=0;

      ptr = buf;
      skip_spaces(&ptr);

      if ((ZCMD.command = *ptr) == '*')
         continue;

      ptr++;
      ZCMD.arg1=0;
      ZCMD.arg2=0;
      ZCMD.arg3=0;
      ZCMD.arg4=0;
      ZCMD.sarg=NULL;
      ZCMD.sarg2=NULL;
      if (ZCMD.command == 'S' || ZCMD.command == '$')
         {
         ZCMD.command = 'S';
         break;
         }
      error = 0;

      if(ZCMD.command =='F') /* string command */
         {
         skip_spaces(&ptr);
         if(*ptr)
            {
            tmp=1;
            ptr++;
            skip_spaces(&ptr);
            ZCMD.sarg=str_dup(ptr);
            }
         else
            {
            error=1;
            }
         }
      else if(ZCMD.command=='V') /* trigger variable command */
         {
         char *tr1=get_buffer(128);
         char *tr2=get_buffer(128);
         if (sscanf(ptr, " %d %ld %ld %ld %s %s", &tmp, &ZCMD.arg1, &ZCMD.arg2,
                    &ZCMD.arg3, tr1, tr2) != 6)
            error = 1;
         else
            {
            ZCMD.sarg = str_dup(tr1);
            ZCMD.sarg2 = str_dup(tr2);
            }
         release_buffer(tr2);
         release_buffer(tr1);
         }
      else if (strchr("R", ZCMD.command) != NULL) /* 3 arg command */
         {
         /* a 3-arg command */
         if (sscanf(ptr, " %d %ld %ld ", &tmp, &ZCMD.arg1, &ZCMD.arg2) != 3)
            error = 1;
         }
      else  if(strchr("GDT",ZCMD.command) !=NULL) /* 4 arg command */
         {
         if ((arg_num=sscanf(ptr," %d %ld %ld %ld ",&tmp,&ZCMD.arg1,&ZCMD.arg2,
                             &ZCMD.arg3)) != 4)
            {
            if(arg_num!=3)
               error = 1;
            else
               ZCMD.arg3=0;
            }
         else if(ZCMD.arg3<0)
            ZCMD.arg3=0;
         }
      else if(ZCMD.command=='Z') /* maze zone */
         {
         error=0;
         tmp=0;
         }
      else   /* 5 arg commands */
         {
         if ((arg_num=sscanf(ptr, " %d %ld %ld %ld %ld ", &tmp, &ZCMD.arg1,
                             &ZCMD.arg2, &ZCMD.arg3, &ZCMD.arg4)) != 5)
            {
            if(arg_num!=4)
               error = 1;
            else
               ZCMD.arg4=0;
            }
         else if(ZCMD.arg4<0)
            ZCMD.arg4=0;
         }

      if((ZCMD.command=='M')|| (ZCMD.command=='O'))
         room_num=ZCMD.arg3;
      else if((ZCMD.command=='R')|| (ZCMD.command=='D'))
         room_num=ZCMD.arg1;

      /* The previous command HAS to work */
      if(strchr("GFEP",ZCMD.command)!=NULL)
         ZCMD.if_flag=tmp;
      else
         ZCMD.if_flag = tmp;

      ZCMD.room_num=room_num;

      if (error)
         {
         log( "SYSERR: Format error in %s, line %d: '%s'\n", zname,
              line_num, buf);
         exit(1);
         }
      ZCMD.line = line_num;
      cmd_no++;
      }

   top_of_zone_table = zone++;
   release_buffer(buf);
   release_buffer(zname);
   release_buffer(buf2);
   release_buffer(col_buf);
   }

#undef Zt


void get_one_line(FILE *fl, char *buf)
   {
   if (fgets(buf, READ_SIZE, fl) == NULL)
      {
      log("SYSERR: error reading help file: not terminated with $?");
      exit(1);
      }

   buf[strlen(buf) - 1] = '\0'; /* take off the trailing \n */
   }


void load_help(FILE *fl,char *filename)
   {
   struct help_index_element el;
   char *key = get_buffer(READ_SIZE+1);
   char *entry = get_buffer(32750);
   char *line = get_buffer(READ_SIZE+1);
   int i, j, t;
   struct help_index_element temp;


   /* get the keyword line */
   get_one_line(fl, key);
   while (*key != '$')
      {
      get_one_line(fl, line);
      *entry = '\0';
      while (*line != '#')
         {
         strcat(entry, strcat(line, "\r\n"));
         get_one_line(fl, line);
         }

      el.min_level = 0;
      if ((*line == '#') && (*(line + 1) != 0))
         el.min_level = atoi((line + 1));


      el.min_level = MAX(0, MIN(el.min_level, LVL_IMPL));
      /* now, add the entry to the index with each keyword on the keyword line*/
      el.entry = str_dup(entry);
      el.keywords = str_dup(key);

      help_table[top_of_helpt] = el;
      top_of_helpt++;

      /* get next keyword line (or $) */
      get_one_line(fl, key);
      }

   /* Added a selection sort so that looking up help entries is done in **
   ** alphabetical order - Nomikos 6/24/2025                            */
   for (i = t = 0; i < top_of_helpt; i++)
   {
      for (j = i + 1; j < top_of_helpt; j++)
	 if (strcmp(help_table[j].keywords, help_table[t].keywords) < 0) 
	    t = j;

      temp = help_table[t];
      help_table[t] = help_table[i];
      help_table[i] = temp;
   }
   
   log("   %d entries loaded.",top_of_helpt+1);
   release_buffer(key);
   release_buffer(entry);
   release_buffer(line);
   }

/*************************************************************************
*  procedures for resetting, both play-time and boot-time    *
*********************************************************************** */



int vnum_mobile(char *searchname, struct char_data * ch)
   {
   mob_vnum nr, found = 0;
   char *buf = get_buffer(32750);

   strcpy(buf,"Num  Virtual Description                         lvl/cl/race/spec\r\n");
   CHECK=0;
   for (nr = 0; nr <= top_of_mobt; nr++)
      {
	if (GET_LEVEL(ch) < VNUM_MOB_LEVEL && !is_olc_set(ch, mob_index[nr].vnum/100)) {
	  continue;
	}

      if (isname(searchname, mob_proto[nr].player.name))
         {
         if(strlen(buf)>32500)
            {
            sprintf(buf+strlen(buf),"Buffer limit exceeded, you need to refine your search\r\n");
            nr=top_of_mobt;
            break;
            }
         else
            {
            sprintf(buf+strlen(buf),
                    "%3ld. [%5ld] %-35.35s %3d/%-2.2s/%-4.4s/%-15.15s\r\n",
                    ++found,
                    mob_index[nr].vnum,
                    mob_proto[nr].player.short_descr,
                    mob_proto[nr].player.level,
                    class_abbrevs[(int)mob_proto[nr].player.class],
                    race_abbrevs[(int)mob_proto[nr].player.race],
                    mob_index[nr].func?get_mob_spec_name(mob_index[nr].func):" ");
            }
         }
      }
   if(ch->desc)
      page_string(ch->desc,buf,TRUE,"");

   release_buffer(buf);
   CHECK=0;
   return (found);
   }



int vnum_object(char *searchname, struct char_data * ch)
   {
   obj_rnum nr, found = 0;
   char *buf = get_buffer(32750);

   strcpy(buf,"Num  Virtual Description\r\n");
   for (nr = 0; nr <= top_of_objt; nr++)
      {
	if (GET_LEVEL(ch) < VNUM_OBJ_LEVEL && !is_olc_set(ch, obj_index[nr].vnum/100)) {
	  continue;
	}

      if (isname(searchname, obj_proto[nr].name))
         {
         if(strlen(buf)>32500)
            {
            sprintf(buf+strlen(buf),"Buffer limit exceeded, you need to refine your search\r\n");
            nr=top_of_mobt;
            break;
            }
         else
            {
            sprintf(buf+strlen(buf), "%3ld. [%5ld] %s\r\n", ++found,
                    obj_index[nr].vnum,
                    obj_proto[nr].short_description);
            }
         }
      }
   if(ch->desc)
      page_string(ch->desc,buf,TRUE,"");

   release_buffer(buf);
   return (found);
   }


/* Code snippet from Dana - luthers@ex-pressnet.com */
int vnum_room(char *searchname, struct char_data * ch)
   {
   room_vnum nr;
   int found = 0;
   char *buf = get_buffer(32750);
   char *tempbuf = get_buffer(MAX_STRING_LENGTH);

   strcpy(buf,"Num  Virtual Description\r\n");   
   for (nr = 0; nr <= top_of_world; nr++)
      {
	if (GET_LEVEL(ch) < VNUM_ROOM_LEVEL && !is_olc_set(ch, world[nr].number/100)) {
	  continue;
	}

      sprintf(tempbuf, "%s", world[nr].name);
      strip_color(tempbuf); 
      if (isname(searchname, tempbuf))
         {
         if(strlen(buf)>32500)
            {
            sprintf(buf+strlen(buf),"Buffer limit exceeded, you need to refine your search\r\n");
            nr=top_of_world;
            break;
            }
         else
            {
            sprintf(buf + strlen(buf), "%3d. [%5ld] %s\r\n", ++found,
                    world[nr].number,
                    world[nr].name);
            }
         }
      }
   if (ch->desc)
      page_string(ch->desc, buf, TRUE, "");

   release_buffer(tempbuf);
   release_buffer(buf);
   return (found);
   }



/* create a character, and add it to the char list */
struct char_data *create_char(void)
   {
   struct char_data *ch;

   CREATE(ch, struct char_data, 1);
   clear_char(ch);
   ch->next = character_list;
   character_list = ch;
   GET_ID(ch) = max_id++;

   return ch;
   }


/* create a new mobile from a prototype */
struct char_data *read_mobile(mob_vnum nr, int type)
   {
   mob_rnum i;
   struct char_data *mob;
   struct char_data *tmp_mob;

   if (type == VIRTUAL)
      {
      if ((i = real_mobile(nr)) < 0)
         {
         log("WARNING: Mobile vnum %ld does not exist in database.", nr);
         return NULL;
         }
      }
   else
      i = nr;

   CREATE(mob, struct char_data, 1);
   clear_char(mob);
   *mob = mob_proto[i];
   if(!character_list||IS_NPC(character_list)) /* no chars */
      {
      mob->next = character_list;
      character_list = mob;
      }
   else
      {
      for(tmp_mob=character_list;tmp_mob;tmp_mob=tmp_mob->next)
         {
         if(!tmp_mob->next) /* 1st mob */
            {
            mob->next=NULL;
            tmp_mob->next=mob;
            break;
            }
         else if(IS_NPC(tmp_mob->next)) /* in the middle */
            {
            mob->next=tmp_mob->next;
            tmp_mob->next=mob;
            break;
            }
         }
      }

   if (!mob->points.max_hit)
      {
      mob->points.max_hit = dice(mob->points.hit, mob->points.mana) +
                            mob->points.move;
      }
   else
      mob->points.max_hit = number(mob->points.hit, mob->points.mana);

   mob->points.hit = mob->points.max_hit;
   mob->points.mana = mob->points.max_mana;
   mob->points.move = mob->points.max_move;

   mob->player.time.birth = time(0);
   mob->player.time.played = 0;
   mob->player.time.logon = time(0);
   GET_MOOD(mob) = number(-1000,1000);
   if((GET_RACE(mob)==MRACE_DEMON)||(GET_RACE(mob)==MRACE_UNDEAD))
      GET_MOOD(mob) = -900;
   else if (MOB_FLAGGED(mob , MOB_HAPPY))
      GET_MOOD(mob) = MIN(1000, GET_MOOD(mob) + 700);
   else if (MOB_FLAGGED(mob, MOB_SAD))
      GET_MOOD(mob) = MAX(-1000, GET_MOOD(mob) - 700);
   else if(GET_RACE(mob)==MRACE_SPRITE)
      GET_MOOD(mob)= 900;
   LAST_HAND_USED(mob)=0;

   mob_index[i].number++;
   GET_ID(mob) = max_id++;
   assign_triggers(mob, MOB_TRIGGER);

   return mob;
   }


/* create an object, and add it to the object list */
struct obj_data *create_obj(void)
   {
   struct obj_data *obj;

   CREATE(obj, struct obj_data, 1);
   clear_object(obj);
   obj->next = object_list;
   object_list = obj;
   GET_ID(obj) = max_id++;
   assign_triggers(obj, OBJ_TRIGGER);
   GET_OBJ_VROOM(obj) = NOWHERE;
   GET_OBJ_TIMER(obj) = 0;
   GET_OBJ_DGTIMER(obj) = -1;
   return obj;
   }


/* create a new object from a prototype */
struct obj_data *read_object(obj_vnum nr, int type)
   {
   struct obj_data *obj;
   obj_rnum i;

   if (nr < 0)
      {
      log("SYSERR: Trying to create obj with negative (%ld) num!",nr);
      return NULL;
      }
   if (type == VIRTUAL)
      {
      if ((i = real_object(nr)) < 0)
         {
         log("SYSERR: Object vnum %ld does not exist in database.", nr);
         return NULL;
         }
      }
   else
      i = nr;

   CREATE(obj, struct obj_data, 1);
   clear_object(obj);
   *obj = obj_proto[i];
   obj->next = object_list;
   object_list = obj;

   obj_index[i].number++;
   GET_ID(obj) = max_id++;
   assign_triggers(obj, OBJ_TRIGGER);


   return obj;
   }

int purge_zone(int zone)
   {
   struct char_data *ch, *next_ch;
   struct obj_data *obj, *next_obj;

   for (ch = character_list; ch; ch = next_ch)
      {
      next_ch = ch->next;
      if (IS_NPC(ch) && (world[IN_ROOM(ch)].zone == zone) && !FIGHTING(ch))
         extract_char(ch);
      }

   for (obj = object_list; obj; obj = next_obj)
      {
      next_obj = obj->next;
      if((IN_ROOM(obj)>top_of_world)|| (IN_ROOM(obj)<-1))
         log("name:%s room:%ld rnum:%ld",GET_OBJ_NAME(obj),IN_ROOM(obj),
             GET_OBJ_RNUM(obj));
      if(IN_ROOM(obj)<=NOWHERE)
         continue;
      if(IN_ROOM(obj)>top_of_world)
         continue;
      if(world[IN_ROOM(obj)].zone!=zone)
         continue;
      if(ROOM_FLAGGED(IN_ROOM(obj),ROOM_HOUSE))
         continue;
      if(IS_CORPSE(obj))
         continue;
      extract_obj(obj);
      }
   return 0;
   }


#define ZO_DEAD  999

/* update zone ages, queue for reset if necessary, and dequeue when possible */
void zone_update(void)
   {
   int i;
   struct reset_q_element *update_u, *temp;
   static int timer = 0;

   /* jelson 10/22/92 */
   if (((++timer * PULSE_ZONE) / PASSES_PER_SEC) >= 60)
      {
      /* one minute has passed */
      /*
       * NOT accurate unless PULSE_ZONE is a multiple of PASSES_PER_SEC or a
       * factor of 60
       */

      timer = 0;

      /* since one minute has passed, increment zone ages */
      for (i = 0; i <= top_of_zone_table; i++)
         {
         /*
          * increment zone ages
          */
         if (zone_table[i].age < zone_table[i].lifespan &&
                 zone_table[i].reset_mode && !ZONE_FLAGGED(i, Z_IDLE))
            (zone_table[i].age)++;

         /*
          * if the zone is idle, increment it's idle time
          * otherwise if it isn't idle, and it's age is greater than its
          *    lifespan idle the zone and set it to idle
          */
         if (is_empty(i) && zone_table[i].name &&
                 (zone_table[i].reset_mode!=3))
            {
            if (ZONE_FLAGGED(i, Z_IDLE))
               if(zone_table[i].idle_time<ZO_DEAD)
                  zone_table[i].idle_time++;

            if (!ZONE_FLAGGED(i, Z_IDLE) &&
                    (zone_table[i].age >= zone_table[i].lifespan))
               {
               SET_BIT(ZONE_FLAGS(i), Z_IDLE);
               zone_table[i].idle_time = 0;
               zone_table[i].age = 0;
               purge_zone(i);

               mudlogf(CMP, LVL_IMMORT, FALSE,"Idle purging zone: %s:%ld",
                       zone_table[i].name, zone_table[i].number);
               continue;
               }
            }


         /*
          * enqueue a zone to be reset.
          */
         if (zone_table[i].age >= zone_table[i].lifespan &&
                 (zone_table[i].reset_mode !=0) &&
                 !ZONE_FLAGGED(i, Z_IDLE) )
            {
            /*
             * Continous reset bug fix
             */
            if(ZONE_FLAGGED(i,Z_QUEUED))
               continue;

            CREATE(update_u, struct reset_q_element, 1);

            SET_BIT(ZONE_FLAGS(i), Z_QUEUED);
            update_u->zone_to_reset = i;
            update_u->next = 0;

            if (!reset_q.head)
               reset_q.head = reset_q.tail = update_u;
            else
               {
               reset_q.tail->next = update_u;
               reset_q.tail = update_u;
               }

            /* zone_table[i].age = ZO_DEAD; */
            }
         }
      }
   /* end - one minute has passed */


   /* dequeue zones (if possible) and reset */
   /* this code is executed every 10 seconds (i.e. PULSE_ZONE) */
   for (update_u = reset_q.head; update_u; update_u = update_u->next)
      if ((zone_table[update_u->zone_to_reset].reset_mode == 2) ||
              (zone_table[update_u->zone_to_reset].reset_mode == 3) ||
              (is_empty(update_u->zone_to_reset)))
         {
         /* purge_zone(update_u->zone_to_reset);  */
         reset_zone(update_u->zone_to_reset);
         mudlogf(CMP, LVL_IMMORT, FALSE,"Auto zone reset: %s:%ld",
                 zone_table[update_u->zone_to_reset].name,
                 zone_table[update_u->zone_to_reset].number);

         /* dequeue */
         if (update_u == reset_q.head)
            {
            if(reset_q.head->next==NULL)
               reset_q.tail=NULL;
            reset_q.head = reset_q.head->next;
            }
         else
            {
            for (temp = reset_q.head; temp->next != update_u;
                    temp = temp->next)
               ;

            if (!update_u->next)
               reset_q.tail = temp;

            temp->next = update_u->next;
            }

         free(update_u);
         break;
         }
   }


ACMD(do_list_reset)
   {
   struct reset_q_element *update_u;
   char *buf=get_buffer(32750);

   buf[0]='\0';
   for (update_u = reset_q.head; update_u; update_u = update_u->next)
      {
      if(update_u&&(update_u->zone_to_reset>=0))
         {
         sprintf(buf+strlen(buf),"[%6ld][%7ld] %-50.50s\r\n",
                 update_u->zone_to_reset,
                 zone_table[update_u->zone_to_reset].number,
                 zone_table[update_u->zone_to_reset].name);
         }
      }
   if(buf[0]=='\0')
      strcpy(buf,"No queued zones.\r\n");
   if(reset_q.head && (reset_q.head->zone_to_reset>=0))
      sprintf(buf+strlen(buf),"Head:\r\n[%6ld][%7ld] %-50.50s\r\n",
              reset_q.head->zone_to_reset,
              zone_table[reset_q.head->zone_to_reset].number,
              zone_table[reset_q.head->zone_to_reset].name);

   if(reset_q.tail && (reset_q.tail->zone_to_reset>=0))
      sprintf(buf+strlen(buf),"Tail:\r\n[%6ld][%7ld] %-50.50s\r\n",
              reset_q.tail->zone_to_reset,
              zone_table[reset_q.tail->zone_to_reset].number,
              zone_table[reset_q.tail->zone_to_reset].name);

   if(ch->desc)
      page_string(ch->desc,buf,TRUE,"");
   release_buffer(buf);
   }



void log_zone_error(zone_rnum zone, int cmd_no, char *message,
                    room_vnum roomnum)
   {

   mudlogf(NRM, LVL_IMMORT, TRUE,"SYSERR: error in zone file: %s", message);

   mudlogf(NRM, LVL_IMMORT, TRUE,
           "SYSERR: ...offending cmd: '%c' cmd in zone #%ld, line %ld, zedit room %ld",
           ZCMD.command, zone_table[zone].number, ZCMD.line,roomnum);

   }

#define ZONE_ERROR(message) \
{ log_zone_error(zone, cmd_no, message,roomnum); last_cmd = 0; }


/** Maze code by Anduin borrowed from Doppelganger Software **/
void make_maze(zone_vnum zone)
   {
   int card[400], temp, x, y, dir;
   room_rnum room;
   int num, next_room = 0, test, r_back;
   room_vnum vnum = zone_table[zone].number;

   for (test = 0; test < 400; test++)
      {
      card[test] = test;
      temp = test;
      dir = temp / 100;
      temp = temp - (dir * 100);
      y = temp / 10;
      temp = temp - (y * 10);
      x = temp;
      room = (vnum * 100) + (y * 10) + x;
      room = real_room(room);
      if ((y == 0) && (dir == 0))
         continue;
      if ((x == 9) && (dir == 1))
         continue;
      if ((y == 9) && (dir == 2))
         continue;
      if ((x == 0) && (dir == 3))
         continue;
      if (world[room].dir_option[dir]) {
	world[room].dir_option[dir]->to_room = -1;
      }
      REMOVE_BIT(ROOM_FLAGS(room), ROOM_NOTRACK);
      }
   for (x = 0; x < 399; x++)
      {
      y = number(0, 399);
      temp = card[y];
      card[y] = card[x];
      card[x] = temp;
      }

   for (num = 0; num < 400; num++)
      {
      temp = card[num];
      dir = temp / 100;
      temp = temp - (dir * 100);
      y = temp / 10;
      temp = temp - (y * 10);
      x = temp;
      room = (vnum * 100) + (y * 10) + x;
      r_back = room;
      room = real_room(room);
      if ((y == 0) && (dir == 0))
         continue;
      if ((x == 9) && (dir == 1))
         continue;
      if ((y == 9) && (dir == 2))
         continue;
      if ((x == 0) && (dir == 3))
         continue;
      if (world[room].dir_option[dir] && world[room].dir_option[dir]->to_room != -1)
         continue;
      switch(dir)
         {
         case 0:
            next_room = r_back - 10;
            break;
         case 1:
            next_room = r_back + 1;
            break;
         case 2:
            next_room = r_back + 10;
            break;
         case 3:
            next_room = r_back - 1;
            break;
         }
      next_room = real_room(next_room);
      test = find_first_step(room, next_room,IGNORE_NOTRACK|IGNORE_ZNOTRACK|MAZE_TEST);
      switch (test)
         {
         case BFS_ERROR:
            log("Maze making error. BFS_ERROR");
            break;
         case BFS_ALREADY_THERE:
            log("Maze making error. BFS_ALREADY_THERE");
            break;
         case BFS_NO_PATH:

	   if (world[room].dir_option[dir]) {
	     world[room].dir_option[dir]->to_room = next_room;
	   }
	    if (world[next_room].dir_option[(int) rev_dir[dir]]) {
	      world[next_room].dir_option[(int) rev_dir[dir]]->to_room = room;
	    }
            break;
         }
      }
   for (num = 0;num < 100;num++)
      {
      room = (vnum * 100) + num;
      room = real_room(room);
      /* Remove the next line if you want to be able to track your way through
      the maze */
      SET_BIT(ROOM_FLAGS(room), ROOM_NOTRACK);

      REMOVE_BIT(ROOM_FLAGS(room), ROOM_BFS_MARK);
      }
   }

/* execute the reset command table of a given zone */
void reset_zone(zone_rnum zone)
   {
   int cmd_no, last_cmd = 0;
   struct char_data *mob = NULL;
   struct obj_data *obj, *obj_to;
   room_vnum roomnum=-1;
   room_rnum roomv,roomr;
   char *buf=get_buffer(256);
   int test=0;
   int mob_load = FALSE;
   int obj_load = FALSE;
   float gold_percent;
   struct char_data *tmob=NULL; /* for trigger assignment */
   struct obj_data *tobj=NULL;  /* for trigger assignment */


   if (ZONE_FLAGGED(zone, Z_IDLE))
      {
      mudlogf(CMP, LVL_IMMORT, TRUE,"Reloading and resetting idle zone: %s",
              zone_table[zone].name);
      REMOVE_BIT(ZONE_FLAGS(zone), Z_IDLE);
      zone_table[zone].idle_time = 0;
      }
   REMOVE_BIT(ZONE_FLAGS(zone), Z_QUEUED);

   for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++)
      {

      if (ZCMD.if_flag && !last_cmd && !obj_load && !mob_load)
         continue;

      if(!ZCMD.if_flag)
         {
         mob_load=FALSE;
         obj_load=FALSE;
         }

      switch (ZCMD.command)
         {
         case '*':   /* ignore command */
            last_cmd = 0;
            break;

         case 'F':
            if(!mob)
               {
               ZONE_ERROR("attempt to force-command a non-existant mob");
               break;
               }
            command_interpreter(mob,ZCMD.sarg);
            break;

         case 'M':   /* read a mobile */
            roomnum=ZCMD.oarg3;
            tmob=NULL;
            tobj=NULL;
            if (mob_index[ZCMD.arg1].number < ZCMD.arg2)
               {
               mob = read_mobile(ZCMD.arg1, REAL);
               mob->orig_room = ZCMD.arg3;
               char_to_room(mob, ZCMD.arg3);
               GET_MOB_VAL(mob,0)=GET_ROOM_VNUM(IN_ROOM(mob));
               last_cmd = 1;
               mob_load=TRUE;
               gold_percent=(float)number(50,130);
               GET_GOLD(mob)=(int)((float)GET_GOLD(mob)*
                                   (float)(((float)gold_percent/(float)100)));
               load_keeper(mob);
               load_mtrigger(mob);
               tmob=mob;
               }
            else
               last_cmd = 0;
            break;

         case 'O':   /* read an object */
            tobj=NULL;
            tmob=NULL;
            if (ZCMD.arg3 >= 0)
               {
               test=0;
               roomnum=ZCMD.oarg3;
               if(world[ZCMD.arg3].contents!=NULL)
                  {
                  for(obj=world[ZCMD.arg3].contents;obj;obj=obj->next_content)
                     if(ZCMD.arg1==GET_OBJ_RNUM(obj))
                        test++;
                  }

               if(number(1,100)>=min(ZCMD.arg4,80))
                  /*(test<ZCMD.arg2)*/
                  {
                  obj = read_object(ZCMD.arg1, REAL);
                  obj_to_room(obj, ZCMD.arg3);
                  GET_OBJ_TIMER(obj)=-1;
                  GET_OBJ_DGTIMER(obj)=-1;
                  last_cmd = 1;
                  obj_load=TRUE;
                  load_otrigger(obj);
                  tobj=obj;
                  }
               else
                  {
                  last_cmd=0;
                  }
               }
            else
               {
               sprintf(buf,"Invalid Room number %ld for obj %ld",ZCMD.oarg3,ZCMD.oarg1);
               ZONE_ERROR(buf);
               last_cmd = 0;
               }
            break;

         case 'P':   /* object to object */
            tobj=NULL;
            tmob=NULL;
            if (obj_load&&(number(1,100)>=min(ZCMD.arg4,80)))
               /*(obj_index[ZCMD.arg1].number < ZCMD.arg2)  */
               {
               obj = read_object(ZCMD.arg1, REAL);
               if (!(obj_to = get_obj_num(ZCMD.arg3)))
                  {
                  sprintf(buf,"Target obj %ld not found, command disabled.",ZCMD.oarg3);
                  ZONE_ERROR(buf);
                  ZCMD.command = '*';
                  break;
                  }
               obj_to_obj(obj, obj_to);
               GET_OBJ_TIMER(obj)=-1;
               GET_OBJ_DGTIMER(obj)=-1;
               last_cmd = 1;
               obj_load=TRUE;
               load_otrigger(obj);
               tobj=obj;
               }
            else
               last_cmd = 0;
            break;

         case 'G':   /* obj_to_char */
            tobj=NULL;
            tmob=NULL;
            if (!mob)
               {
               ZONE_ERROR("attempt to give obj to non-existant mob, command disabled.");
               ZCMD.command = '*';
               break;
               }
            if (mob_load&&(number(1,100)>=min(ZCMD.arg3,80)))
               /*(obj_index[ZCMD.arg1].number < ZCMD.arg2)*/
               {
               obj = read_object(ZCMD.arg1, REAL);
               obj_to_char(obj, mob);
               GET_OBJ_TIMER(obj)=-1;
               GET_OBJ_DGTIMER(obj)=-1;
               last_cmd = 1;
               obj_load=TRUE;
               load_otrigger(obj);
               tobj=obj;
               }
            else
               last_cmd = 0;
            break;

         case 'E':   /* object to equipment list */
            tobj=NULL;
            tmob=NULL;
            if (!mob)
               {
               ZONE_ERROR("trying to equip non-existant mob, command disabled.");
               ZCMD.command = '*';
               break;
               }
            if(mob_load&&(number(1,100)>=min(ZCMD.arg4,80)))
               /*(obj_index[ZCMD.arg1].number < ZCMD.arg2)*/
               {
               if (ZCMD.arg3 < 0 || ZCMD.arg3 >= NUM_WEARS)
                  {
                  ZONE_ERROR("invalid equipment pos number");
                  }
               else
                  {
                  obj = read_object(ZCMD.arg1, REAL);
                  if(!IS_OBJ_STAT(obj,ITEM_NO_POS_CHK)&&!CAN_WEAR(obj,wear_check[ZCMD.arg3]))
                     {
                     mudlogf(CMP,LVL_IMMORT,TRUE,
                             "SYSERR: Could not equip %s[%ld] with item %ld in pos %ld zedit room %ld",
                             GET_NAME(mob),GET_MOB_VNUM(mob),
                             ZCMD.oarg1,ZCMD.arg3,ZCMD.room_num);
                     extract_obj(obj);
                     obj_load=FALSE;
                     last_cmd=0;
                     }
                  else
                     {
                     IN_ROOM(obj) = IN_ROOM(mob);
                     load_otrigger(obj);
                     IN_ROOM(obj) =NOWHERE;
                     if (wear_otrigger(obj, mob, ZCMD.arg3))
                        equip_char(mob, obj, ZCMD.arg3);
                     else
                        obj_to_char(obj, mob);
                     tobj = obj;
                     GET_OBJ_TIMER(obj)=-1;
                     GET_OBJ_DGTIMER(obj)=-1;
                     obj_load=TRUE;
                     last_cmd=1;
                     }
                  }
               }
            else
               last_cmd = 0;
            tmob=NULL;
            break;

         case 'R': /* rem obj from room */
            tmob=NULL;
            tobj=NULL;
            roomnum=ZCMD.oarg1;
            if((obj=get_obj_in_list_num(ZCMD.arg2,world[ZCMD.arg1].contents))
                    != NULL)
               {
               if((GET_OBJ_TYPE(obj)==ITEM_FURNITURE)&&obj->people)
                  {
                  last_cmd=0;
                  break;
                  }
               obj_from_room(obj);
               extract_obj(obj);
               }
            last_cmd = 1;
            tmob=NULL;
            tobj=NULL;
            break;

         case 'Z':   /* command to make maze */
            tmob=NULL;
            tobj=NULL;
            log("Making maze for Zone %ld",zone_table[zone].number);
            make_maze(zone);
            break;

         case 'D':   /* set state of door */
            tmob=NULL;
            tobj=NULL;
            roomnum=ZCMD.oarg1;
            if (ZCMD.arg2 < 0 || ZCMD.arg2 >= NUM_OF_DIRS ||
                    (world[ZCMD.arg1].dir_option[ZCMD.arg2] == NULL))
               {
               ZONE_ERROR("door does not exist, command disabled.");
               ZCMD.command = '*';
               }
            else
               switch (ZCMD.arg3)
                  {
                  case 0:
                     REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                                EX_LOCKED);
                     REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                                EX_CLOSED);
                     break;
                  case 1:
                     SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                             EX_CLOSED);
                     REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                                EX_LOCKED);
                     break;
                  case 2:
                     SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                             EX_LOCKED);
                     SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                             EX_CLOSED);
                     break;
                  }
            last_cmd = 1;
            tmob=NULL;
            tobj=NULL;
            break;
         case 'T':
            if (ZCMD.arg1==MOB_TRIGGER && tmob)
               {
               if (!SCRIPT(tmob))
                  CREATE(SCRIPT(tmob), struct script_data, 1);
               add_trigger(SCRIPT(tmob),
                           read_trigger(real_trigger(ZCMD.arg2)), -1);
               last_cmd = 1;
               }
            else if (ZCMD.arg1==OBJ_TRIGGER && tobj)
               {
               if (!SCRIPT(tobj))
                  CREATE(SCRIPT(tobj), struct script_data, 1);
               add_trigger(SCRIPT(tobj),
                           read_trigger(real_trigger(ZCMD.arg2)), -1);
               last_cmd = 1;
               }
            break;

         case 'V':
            if (ZCMD.arg1==MOB_TRIGGER && tmob)
               {
               if (!SCRIPT(tmob))
                  {
                  ZONE_ERROR("Attempt to give variable to scriptless mobile");
                  }
               else
                  add_var(&(SCRIPT(tmob)->global_vars), ZCMD.sarg, ZCMD.sarg2,
                          ZCMD.arg3);
               last_cmd = 1;
               }
            else if (ZCMD.arg1==OBJ_TRIGGER && tobj)
               {
               if (!SCRIPT(tobj))
                  {
                  ZONE_ERROR("Attempt to give variable to scriptless object");
                  }
               else
                  add_var(&(SCRIPT(tobj)->global_vars), ZCMD.sarg, ZCMD.sarg2,
                          ZCMD.arg3);
               last_cmd = 1;
               }
            else if (ZCMD.arg1==WLD_TRIGGER)
               {
               if (ZCMD.arg2<0 || ZCMD.arg2>top_of_world)
                  {
                  ZONE_ERROR("Invalid room number in variable assignment");
                  }
               else
                  {
                  if (!(world[ZCMD.arg2].script))
                     {
                     ZONE_ERROR("Attempt to give variable to scriptless object");
                     }
                  else
                     add_var(&(world[ZCMD.arg2].script->global_vars),
                             ZCMD.sarg, ZCMD.sarg2, ZCMD.arg3);
                  last_cmd = 1;
                  }
               }
            break;


         default:
            ZONE_ERROR("unknown cmd in reset table; cmd disabled");
            ZCMD.command = '*';
            break;
         }
      }

   zone_table[zone].age = 0;
   release_buffer(buf);
   /* handle reset_wtrigger's */
   roomv = zone_table[zone].number * 100;
   while (roomv <= zone_table[zone].top)
      {
      roomr = real_room(roomv);
      if (roomr!=NOWHERE)
         reset_wtrigger(&world[roomr]);
      roomv++;
      }

   }



/* for use in reset_zone; return TRUE if zone 'nr' is free of PC's  */
int is_empty(zone_rnum zone_nr)
   {
   struct descriptor_data *i;

   for (i = descriptor_list; i; i = i->next)
      if (STATE(i)==CON_PLAYING)
         if(i->character)
            if (world[IN_ROOM(i->character)].zone == zone_nr)
               return 0;

   return 1;
   }





/*************************************************************************
*  stuff related to the save/load player system     *
*********************************************************************** */
long get_ptable_by_name(char *name)
   {
   int i;
   char *arg = get_buffer(MAX_INPUT_LENGTH);

   one_argument(name, arg);
   for (i = 0; i <= top_of_p_table; i++)
      if (!strcmp(player_table[i].name, arg))
         {
         release_buffer(arg);
         return (i);
         }
   release_buffer(arg);
   return (-1);
   }


long get_id_by_name(char *name)
   {
   int i;
   char *arg = get_buffer(MAX_INPUT_LENGTH);

   one_argument(name, arg);
   for (i = 0; i <= top_of_p_table; i++)
      if (!str_cmp(player_table[i].name, arg))
         {
         release_buffer(arg);
         return (player_table[i].id);
         }
   release_buffer(arg);
   return -1;
   }


char *get_name_by_id(long id)
   {
   int i;

   for (i = 0; i <= top_of_p_table; i++)
      if (player_table[i].id == id)
         return (player_table[i].name);

   return NULL;
   }


/* Load a char, TRUE if loaded, FALSE if not */
long load_char(char *name, struct char_file_u * char_element)
   {
   int player_i;
   if ((player_i = find_name(name)) >= 0)
      {
	/*
      fseek(player_fl, (long) (player_i * sizeof(struct char_file_u)), SEEK_SET);
      fread(char_element, sizeof(struct char_file_u), 1, player_fl);
	*/
	load_char_ascii(char_element, name);
      return (player_i);
      }
   else {
     log("load_char(%s) failed", name);
      return (-1);
   }
   
   }


/*
 * This version is identical to save_char except that it does not update the
 * host or last logon. It is only used when logging on as an existing character
 * but typing the wrong password
 */
void save_char_no_logon(struct char_data* ch, room_rnum load_room) {
   struct char_file_u st;
   struct char_file_u tmp;
   int k,table_pos;
   if (IS_NPC(ch) || !ch->desc/* || GET_PFILEPOS(ch) < 0*/)
      return;

   load_char(ch->player.name, &tmp);
   ch->player.time.logon = tmp.last_logon;

   char_to_store(ch, &st, FALSE);

   if (!PLR_FLAGGED(ch, PLR_LOADROOM))
      {
      if (load_room == NOWHERE)
         st.player_specials_saved.load_room = NOWHERE;
      else
         st.player_specials_saved.load_room = GET_ROOM_VNUM(load_room);
      }

   save_char_ascii(&st);

   /*
     TODO: Why is this BEFORE updating the player table struct entry?
   fseek(player_fl, GET_PFILEPOS(ch) * sizeof(struct char_file_u), SEEK_SET);
   fwrite(&st, sizeof(struct char_file_u), 1, player_fl);
   */
   if((table_pos = find_id(GET_IDNUM(ch)))!=-1)
      {
      player_table[table_pos].level = GET_LEVEL(ch);
      player_table[table_pos].plr_flags=PLR_FLAGS(ch);

      for(k=0;k<5;k++)
         player_table[table_pos].gold[k]
         = ch->points.gold[k];
      for(k=0;k<32;k++)
         player_table[table_pos].bank_gold[k]
         = ch->points.bank_gold[k];
      }
   write_player_index_file();


   check_weapon_weight(ch);
   save_char_vars(ch);
}


/*
 * write the vital data of a player to the player file
 *
 * NOTE: load_room should be an *RNUM* now.  It is converted to a vnum here.
 */
void save_char(struct char_data * ch, room_rnum load_room)
   {
   struct char_file_u st;
   int k,table_pos;
   if (IS_NPC(ch) || !ch->desc/* || GET_PFILEPOS(ch) < 0*/)
      return;

   char_to_store(ch, &st,TRUE);

   strncpy(st.host, ch->desc->host, HOST_LENGTH);
   st.host[HOST_LENGTH] = '\0';

   if (!PLR_FLAGGED(ch, PLR_LOADROOM))
      {
      if (load_room == NOWHERE)
         st.player_specials_saved.load_room = NOWHERE;
      else
         st.player_specials_saved.load_room = GET_ROOM_VNUM(load_room);
      }

   save_char_ascii(&st);

   /*
     TODO: Why is this BEFORE updating the player table struct entry?
   fseek(player_fl, GET_PFILEPOS(ch) * sizeof(struct char_file_u), SEEK_SET);
   fwrite(&st, sizeof(struct char_file_u), 1, player_fl);
   */
   if((table_pos = find_id(GET_IDNUM(ch)))!=-1)
      {
      player_table[table_pos].level = GET_LEVEL(ch);
      player_table[table_pos].plr_flags=PLR_FLAGS(ch);

      for(k=0;k<5;k++)
         player_table[table_pos].gold[k]
         = ch->points.gold[k];
      for(k=0;k<32;k++)
         player_table[table_pos].bank_gold[k]
         = ch->points.bank_gold[k];
      }
   write_player_index_file();


   check_weapon_weight(ch);
   save_char_vars(ch);

   }

void read_line_ascii(FILE *fp, char *string, int len)
{
  /* fgets only reads len-1 characters, so we have to give it one more than what was passed in. */
  memset(string, 0, len*sizeof(char));
  if (!fgets(string, len+1, fp) || strlen(string) <= 0) {
    return;
  }
  int n = strlen(string)-1;
  if (n > 0 && (string[n] == '\r' || string[n] == '\n')) {
    string[n] = '\x0';
    n--;
  }
  if (n > 0 && (string[n] == '\r' || string[n] == '\n')) {
    string[n] = '\x0';
    n--;
  }
  if (!str_cmp(string, "(null)")) {
    string[0] = '\x0';
  }
}

void load_char_ascii(struct char_file_u *ch, char *name)
{
  char buf[MAX_STRING_LENGTH], buf2[MAX_INPUT_LENGTH];
  int arr[32], i;

  if (!ch || !name || !*name) {
    return;
  }
  memset(ch, 0, sizeof(struct char_file_u));

  buf2[0] = toupper(name[0]);
  for (i = 1; i < strlen(name); i++) {
    buf2[i] = tolower(name[i]);
  }
  buf2[i] = '\x0';
  
  sprintf(buf, "etc/players_ascii/%c/%c%s", buf2[0], buf2[0], buf2+1);
  FILE *fp = fopen(buf, "r");
  if (!fp) {
    log("SYSERR: load_ascii_char(%s) could not open player file \"%s\".", buf2, buf);
    return;
  }
  
  /* Unstructured data in the pfile. */
  int version = -1;
  if (!fscanf(fp, "# Version: %d\n", &version)) {
    log("SYSERR: load_ascii_char(%s) unknown file version!\n", name);
    fclose(fp);
    return;
  }
  read_line_ascii(fp, ch->name, MAX_NAME_LENGTH);
  ch->description[0] = '\x0';
  while (TRUE) {
    read_line_ascii(fp, buf, MAX_STRING_LENGTH);
    if (!strcmp(buf, "~")) {
      break;
    }
    if (strlen(ch->description) + strlen(buf) + 3 < EXDSCR_LENGTH /* NOT +1 here. */) {
      strcat(ch->description, buf);
      strcat(ch->description, "\r\n");
    }
  }
  read_line_ascii(fp, ch->title, MAX_TITLE_LENGTH+1);
  if (version >= 2) {
    read_line_ascii(fp, ch->email, 256);
  }
  fscanf(fp, "%u %d %d %ld %d %u %u\n", &ch->level, &arr[0], &arr[1], &ch->hometown, &arr[2], &ch->weight, &ch->height);
  ch->race = arr[0];
  ch->class = arr[1];
  ch->sex = arr[2];
  fscanf(fp, "%ld %d %d\n", &ch->played, &ch->birth, &ch->last_logon);
  read_line_ascii(fp, ch->pwd, MAX_PWD_LENGTH+1);
  read_line_ascii(fp, ch->host, HOST_LENGTH+1);
  
  /* char_special_data_saved */
  struct char_special_data_saved *char_saved = &ch->char_specials_saved;
  read_line_ascii(fp, buf, MAX_STRING_LENGTH); /* Skip header. */
  fscanf(fp, "%ld %d %d\n", &char_saved->idnum, &char_saved->alignment, &char_saved->spell_fail);
  fscanf(fp, "%ld %ld %ld\n", &char_saved->affected_by, &char_saved->affected_by2, &char_saved->affected_by3);
  fscanf(fp, "%ld %ld %ld\n", &char_saved->act, &char_saved->act2, &char_saved->act3);
  fscanf(fp, "%d %d %d %d %d\n",
    &char_saved->apply_saving_throw[0],
    &char_saved->apply_saving_throw[1],
    &char_saved->apply_saving_throw[2],
    &char_saved->apply_saving_throw[3],
    &char_saved->apply_saving_throw[4]
  );

  /* char_ability_data */
  struct char_ability_data *abilities = &ch->abilities;
  read_line_ascii(fp, buf, MAX_STRING_LENGTH); /* Skip header. */
  fscanf(fp, "%d %d %d %d %d %d %d\n", &arr[0], &arr[1], &arr[2], &arr[3], &arr[4], &arr[5], &arr[6]);
  abilities->str = (sbyte)arr[0];
  abilities->intel = (sbyte)arr[1];
  abilities->wis = (sbyte)arr[2];
  abilities->dex = (sbyte)arr[3];
  abilities->con = (sbyte)arr[4];
  abilities->cha = (sbyte)arr[5];
  abilities->str_add = (sbyte)arr[6];

  /* char_point_data */
  struct char_point_data *points = &ch->points;
  read_line_ascii(fp, buf, MAX_STRING_LENGTH); /* Skip header. */
  fscanf(fp, "%d %d %d %d %d %d\n", &points->hit, &points->mana, &points->move, &points->max_hit, &points->max_mana, &points->max_move);
  fscanf(fp, "%d %d %d\n", &points->armor, &points->hitroll, &points->damroll);
  fscanf(fp, "%ld\n", &points->exp);
  fscanf(fp, "%ld %ld %ld %ld %ld\n", &points->gold[0], &points->gold[1], &points->gold[2], &points->gold[3], &points->gold[4]);
  for (i = 0; i < 32; i++) {
    fscanf(fp, "%ld ", &points->bank_gold[i]);
  }
  fscanf(fp, "\n");

  /* player_special_data_saved */
  struct player_special_data_saved *player_saved = &ch->player_specials_saved;
  read_line_ascii(fp, buf, MAX_STRING_LENGTH); /* Skip header. */
  fscanf(fp, "%d %u %ld\n", &player_saved->wimp_level, &player_saved->freeze_level, &player_saved->load_room);
  fscanf(fp, "%d %d %d %d %d\n", &arr[0], &player_saved->board_number, &player_saved->screensize, &player_saved->times_remorted, &player_saved->mute_channels);
  player_saved->bad_pws = (ubyte)arr[0];
  fscanf(fp, "%d %d %d\n", &arr[0], &arr[1], &arr[2]);
  player_saved->conditions[0] = (sbyte)arr[0];
  player_saved->conditions[1] = (sbyte)arr[1];
  player_saved->conditions[2] = (sbyte)arr[2];
  fscanf(fp, "%d %d %d %d %d\n", &arr[0], &arr[1], &arr[2], &arr[3], &arr[4]);
  player_saved->pkills = arr[0];
  player_saved->deaths = arr[1];
  player_saved->q_points = arr[2];
  player_saved->point = arr[3];
  player_saved->invis_level = arr[4];
  fscanf(fp, "%d %d %d %d %d\n",
    &player_saved->olc_zone[0], 
    &player_saved->olc_zone[1], 
    &player_saved->olc_zone[2], 
    &player_saved->olc_zone[3], 
    &player_saved->olc_zone[4]
  );
  read_line_ascii(fp, player_saved->poofin, MAX_POOF);
  read_line_ascii(fp, player_saved->poofout, MAX_POOF);
  read_line_ascii(fp, player_saved->cl_name, MAX_CLAN_NAME_LENGTH);
  fscanf(fp, "%d %d %d\n", &player_saved->cl_rank, &player_saved->clan, &player_saved->cl_room);
  fscanf(fp, "%d %d %d %ld\n", &player_saved->last_learnt, &arr[0], &player_saved->old_mobkills, &player_saved->mobkills);  
  player_saved->learn_tic = (ubyte)arr[0];
  fscanf(fp, "%ld %ld %ld\n", &player_saved->pref, &player_saved->pref2, &player_saved->pref3);
  for (i = 0; i < MAX_SKILLS+1; i++) {
    fscanf(fp, "%d %d ", &arr[0], &arr[1]);
    player_saved->skills[i] = (byte)arr[0];
    player_saved->skills_learn[i] = (byte)arr[1];
  }
  fscanf(fp, "\n");
  for (i = 0; i < 64; i++) {
    fscanf(fp, " %ld %d", &player_saved->kills_vnum[i], &arr[0]);
    player_saved->kills_ammount[i] = (ubyte)arr[0];
  }
  fgets(buf, MAX_STRING_LENGTH, fp);

  /* affected_type */
  struct affected_type *affected = &ch->affected[0];
  read_line_ascii(fp, buf, MAX_STRING_LENGTH); /* Skip header. */
  for (i = 0; i < MAX_AFFECT; i++) {
    fscanf(fp, "%d %d %ld %d %ld %d\n", &affected[i].type, &affected[i].duration, &affected[i].modifier, &affected[i].location, &affected[i].bitvector, &affected[i].spell_level);
    if (affected[i].spell_level > 10 || affected[i].spell_level < 0)
    {
      affected[i].spell_level = 0;
    }
  }

  /* END main data */
  fclose(fp);  

  /* read explored data */
  char filename[MAX_STRING_LENGTH];
  sprintf(filename, "etc/players_ascii/%c/%s.explored", toupper(ch->name[0]), ch->name);
  fp = fopen(filename, "r");
  if (!fp) {
    log("SYSERR: Could not open %s for reading.  New player?", filename);
    return;
  }
  fread(&ch->explored_vnums, sizeof(char), EXPLORED_BYTES, fp);
  fclose(fp);
}

void save_char_ascii(struct char_file_u *ch)
{
  int i;
  char buf[65536] = {'\x0'}; /* Pfiles must stay LESS THAN THIS AMOUNT! */
  char buf2[65536] = {'\x0'};

  if (!ch || !ch->name || !ch->name[0]) {
    return;
  }

  char filename[MAX_STRING_LENGTH];
  sprintf(filename, "etc/players_ascii/%c/%s", toupper(ch->name[0]), ch->name);
  FILE *fp = fopen(filename, "w");
  if (!fp) {
    char cmd[MAX_STRING_LENGTH];
    sprintf(cmd, "/bin/mkdir -p etc/players_ascii/%c", toupper(ch->name[0]));
    system(cmd);
    fp = fopen(filename, "w");
  }
  if (!fp) {
    log("SYSERR: Could not open %s for writing.", filename);
    return;
  }

  /* Unstructured data in the pfile. */
  sprintf(buf2, "# Version: 2\n");
  strcat(buf, buf2);
  strcat(buf, ch->name);
  strcat(buf, "\n");
  strcat(buf, ch->description);
  strcat(buf, "~\n");
  strcat(buf, ch->title[0] ? ch->title : "(null)");
  strcat(buf, "\n");
  strcat(buf, ch->email[0] ? ch->email : "(null)");
  strcat(buf, "\n");
  sprintf(buf2, "%d %d %d %d %d %d %d\n", ch->level, ch->race, ch->class, ch->hometown, ch->sex, ch->weight, ch->height);
  strcat(buf, buf2);
  sprintf(buf2, "%ld %ld %ld\n", ch->played, ch->birth, ch->last_logon);
  strcat(buf, buf2);
  strcat(buf, ch->pwd);
  strcat(buf, "\n");
  strcat(buf, ch->host);
  strcat(buf, "\n");

  /* char_special_data_saved */
  struct char_special_data_saved *char_saved = &ch->char_specials_saved;
  strcat(buf, "# char_special_data_saved\n");
  sprintf(buf2, "%ld %d %d\n", char_saved->idnum, char_saved->alignment, char_saved->spell_fail);
  strcat(buf, buf2);
  sprintf(buf2, "%ld %ld %ld\n", char_saved->affected_by, char_saved->affected_by2, char_saved->affected_by3);
  strcat(buf, buf2);
  sprintf(buf2, "%ld %ld %ld\n", char_saved->act, char_saved->act2, char_saved->act3);
  strcat(buf, buf2);
  sprintf(buf2, "%d %d %d %d %d\n",
    char_saved->apply_saving_throw[0],
    char_saved->apply_saving_throw[1],
    char_saved->apply_saving_throw[2],
    char_saved->apply_saving_throw[3],
    char_saved->apply_saving_throw[4]
  );
  strcat(buf, buf2);

  /* char_ability_data */
  struct char_ability_data *abilities = &ch->abilities;
  strcat(buf, "# char_ability_data\n");
  sprintf(buf2, "%d %d %d %d %d %d %d\n", abilities->str, abilities->intel, abilities->wis, abilities->dex, abilities->con, abilities->cha, abilities->str_add);
  strcat(buf, buf2);

  /* char_point_data */
  struct char_point_data *points = &ch->points;
  strcat(buf, "# char_point_data\n");
  sprintf(buf2, "%d %d %d %d %d %d\n", points->hit, points->mana, points->move, points->max_hit, points->max_mana, points->max_move);
  strcat(buf, buf2);
  sprintf(buf2, "%d %d %d\n", points->armor, points->hitroll, points->damroll);
  strcat(buf, buf2);
  sprintf(buf2, "%ld\n", points->exp);  
  strcat(buf, buf2);
  sprintf(buf2, "%ld %ld %ld %ld %ld\n", points->gold[0], points->gold[1], points->gold[2], points->gold[3], points->gold[4]);
  strcat(buf, buf2);
  for (i = 0; i < 32; i++) {
    sprintf(buf2, "%ld ", points->bank_gold[i]);
    strcat(buf, buf2);
  }
  strcat(buf, "\n");

  /* player_special_data_saved */
  struct player_special_data_saved *player_saved = &ch->player_specials_saved;
  strcat(buf, "# player_special_data_saved\n");
  sprintf(buf2, "%d %d %ld\n", player_saved->wimp_level, player_saved->freeze_level, player_saved->load_room);
  strcat(buf, buf2);
  sprintf(buf2, "%d %d %d %d %d\n", player_saved->bad_pws, player_saved->board_number, player_saved->screensize, player_saved->times_remorted, player_saved->mute_channels);
  strcat(buf, buf2);
  sprintf(buf2, "%d %d %d\n", player_saved->conditions[0], player_saved->conditions[1], player_saved->conditions[2]);
  strcat(buf, buf2);
  sprintf(buf2, "%d %d %d %d %d\n", player_saved->pkills, player_saved->deaths, player_saved->q_points, player_saved->point, player_saved->invis_level);
  strcat(buf, buf2);
  sprintf(buf2, "%d %d %d %d %d\n", player_saved->olc_zone[0], player_saved->olc_zone[1], player_saved->olc_zone[2], player_saved->olc_zone[3], player_saved->olc_zone[4]);
  strcat(buf, buf2);
  strcat(buf, player_saved->poofin[0] ? player_saved->poofin : "(null)");
  strcat(buf, "\n");
  strcat(buf, player_saved->poofout[0] ? player_saved->poofout : "(null)");
  strcat(buf, "\n");
  strcat(buf, player_saved->cl_name[0] ? player_saved->cl_name : "(null)");
  strcat(buf, "\n");
  sprintf(buf2, "%d %d %d\n", player_saved->cl_rank, player_saved->clan, player_saved->cl_room);
  strcat(buf, buf2);
  sprintf(buf2, "%d %d %d %ld\n", player_saved->last_learnt, player_saved->learn_tic, player_saved->old_mobkills, player_saved->mobkills);  
  strcat(buf, buf2);
  sprintf(buf2, "%ld %ld %ld\n", player_saved->pref, player_saved->pref2, player_saved->pref3);
  strcat(buf, buf2);
  for (i = 0; i < MAX_SKILLS+1; i++) {
    sprintf(buf2, "%d %d ", player_saved->skills[i], player_saved->skills_learn[i]);
    strcat(buf, buf2);
  }
  strcat(buf, "\n");
  for (i = 0; i < 64; i++) {
    sprintf(buf2, "%d %d ", player_saved->kills_vnum[i], player_saved->kills_ammount[i]);
    strcat(buf, buf2);
  }
  strcat(buf, "\n");

  /* affected_type */
  struct affected_type *affected = &ch->affected[0];
  strcat(buf, "# affected_type\n");
  for (i = 0; i < MAX_AFFECT; i++) {
    sprintf(buf2, "%d %d %ld %d %ld %d\n", affected[i].type, affected[i].duration, affected[i].modifier, affected[i].location, affected[i].bitvector, affected[i].spell_level);
    strcat(buf, buf2);
  }

  /* END main section */
  fputs(buf, fp);
  fflush(fp);
  fclose(fp);

  /* write explored data */
  sprintf(filename, "etc/players_ascii/%c/%s.explored", toupper(ch->name[0]), ch->name);
  fp = fopen(filename, "w");
  if (!fp) {
    log("SYSERR: Could not open %s for writing.", filename);
    return;
  }
  fwrite(&ch->explored_vnums, sizeof(char), EXPLORED_BYTES, fp);
  fclose(fp);
}



/* copy data from the file structure to a char struct */
void store_to_char(struct char_file_u * st, struct char_data * ch)
   {
   int i;

   /* to save memory, only PC's -- not MOB's -- have player_specials */
   if (ch->player_specials == NULL)
      CREATE(ch->player_specials, struct player_special_data, 1);

   GET_SEX(ch)   = st->sex;
   GET_RACE(ch)  = st->race;  /* 10/26/96, Echo */
   GET_CLASS(ch) = st->class;
   GET_LEVEL(ch) = st->level;

   ch->player.short_descr = NULL;
   ch->player.long_descr = NULL;
   ch->player.title = str_dup(st->title);
   ch->player.description = str_dup(st->description);

   ch->player.hometown = st->hometown;
   ch->player.time.birth = st->birth;
   ch->player.time.played = st->played;
   ch->player.time.logon = time(0);

   ch->player.weight = st->weight;
   ch->player.height = st->height;

   ch->real_abils = st->abilities;
   ch->aff_abils = st->abilities;
   ch->points = st->points;
   ch->char_specials.saved = st->char_specials_saved;
   ch->player_specials->saved = st->player_specials_saved;
   GET_LAST_TELL(ch)=NOBODY;
   TAGGED(ch) = FALSE;

   if (ch->points.max_mana < 100)
      ch->points.max_mana = 100;

   ch->char_specials.carry_weight = 0;
   ch->char_specials.carry_items = 0;
   ch->points.armor = 200;
   ch->points.hitroll = 0;
   ch->points.damroll = 0;

   if (ch->player.name == NULL)
      CREATE(ch->player.name, char, strlen(st->name) + 1);
   strcpy(ch->player.name, st->name);
   strcpy(ch->player.passwd, st->pwd);

   /* Add all spell effects */
   for (i = 0; i < MAX_AFFECT; i++)
      {
      if (st->affected[i].type)
         affect_to_char(ch, &st->affected[i]);
      }

   /*
    * If you're not poisioned and you've been away for more than an hour of
    * real time, we'll set your HMV back to full
    */

   if (!AFF_FLAGGED(ch, AFF_POISON) &&
           (((long) (time(0) - st->last_logon)) >= SECS_PER_REAL_HOUR))
      {
      GET_HIT(ch) = GET_MAX_HIT(ch);
      GET_MOVE(ch) = GET_MAX_MOVE(ch);
      GET_MANA(ch) = GET_MAX_MANA(ch);
      }
   if (AFF_FLAGGED(ch, AFF_PLAGUE)&&
           (((long) (time(0) - st->last_logon)) >= (SECS_PER_REAL_HOUR*2)))
      {
         affect_from_char(ch, SPELL_PLAGUE); 
      }

   memcpy(ch->player_specials->explored_vnums, st->explored_vnums, EXPLORED_BYTES*sizeof(char));
   GET_EXPLORED(ch) = 0;
   for (i = 0; i < 8*EXPLORED_BYTES; i++) {
     if (ch->player_specials->explored_vnums[i/8] & (1 << (i%8))) {
       (GET_EXPLORED(ch))++;
     }
   }

   strcpy(ch->player_specials->email, st->email);
   }
/* store_to_char */




/* copy vital data from a players char-structure to the file structure */
void char_to_store(struct char_data * ch, struct char_file_u * st,
                   int save_time)
   {
   int i;
   struct affected_type *af;
   struct obj_data *char_eq[NUM_WEARS];

   /* Unaffect everything a character can be affected by */

   for (i = 0; i < NUM_WEARS; i++)
      {
      if (GET_EQ(ch, i))
         {
         char_eq[i] = unequip_char_inner(ch, i);
         remove_otrigger(char_eq[i], ch);
         }
      else
         char_eq[i] = NULL;
      }

   for (af = ch->affected, i = 0; i < MAX_AFFECT; i++)
      {
      if (af)
         {
         st->affected[i] = *af;
         st->affected[i].next = 0;
         af = af->next;
         }
      else
         {
         st->affected[i].type = 0; /* Zero signifies not used */
         st->affected[i].duration = 0;
         st->affected[i].modifier = 0;
         st->affected[i].location = 0;
         st->affected[i].bitvector = 0;
	 st->affected[i].spell_level = 0;
         st->affected[i].next = 0;
         }
      }


   /*
    * remove the affections so that the raw values are stored; otherwise the
    * effects are doubled when the char logs back in.
    */

   while (ch->affected)
      affect_remove(ch, ch->affected);

   if ((i >= MAX_AFFECT) && af && af->next)
      log("SYSERR: WARNING: OUT OF STORE ROOM FOR AFFECTED TYPES!!!");

   ch->aff_abils = ch->real_abils;

   st->birth = ch->player.time.birth;
   st->played = ch->player.time.played;
   st->played += (long) (time(0) - ch->player.time.logon);
   if(save_time)
      st->last_logon = time(0);
   else
      st->last_logon = ch->player.time.logon;

   ch->player.time.played = st->played;
   ch->player.time.logon = time(0);

   st->hometown = ch->player.hometown;
   st->weight = GET_WEIGHT(ch);
   st->height = GET_HEIGHT(ch);
   st->sex = GET_SEX(ch);
   st->race  = GET_RACE(ch);
   st->class = GET_CLASS(ch);
   st->level = GET_LEVEL(ch);
   st->abilities = ch->real_abils;
   st->points = ch->points;
   st->char_specials_saved = ch->char_specials.saved;
   st->player_specials_saved = ch->player_specials->saved;

   st->points.armor = 200;
   st->points.hitroll = 0;
   st->points.damroll = 0;

   if (GET_TITLE(ch))
      strcpy(st->title, GET_TITLE(ch));
   else
      *st->title = '\0';

   if (ch->player.description)
      {
      strncpy(st->description, ch->player.description, EXDSCR_LENGTH-1);
      st->description[EXDSCR_LENGTH-1] = '\0';
      }
   else
      *st->description = '\0';

   strcpy(st->name, GET_NAME(ch));
   strcpy(st->pwd, GET_PASSWD(ch));

   /* add spell and eq affections back in now */
   for (i = 0; i < MAX_AFFECT; i++)
      {
      if (st->affected[i].type)
         affect_to_char(ch, &st->affected[i]);
      }

   for (i = 0; i < NUM_WEARS; i++)
      {
      if (char_eq[i])
         {
         if (wear_otrigger(char_eq[i], ch, i))
            equip_char(ch, char_eq[i], i);
         else
            obj_to_char(char_eq[i], ch);
         }

      }
   /*   affect_total(ch); unnecessary, I think !?! */
   if(!AFF_FLAGGED(ch,AFF_FLY)&&AFF2_FLAGGED(ch,AFF2_FLYING))
      {
      send_to_char(ch,"You gently float to the ground.\r\n");
      REMOVE_BIT(AFF2_FLAGS(ch),AFF2_FLYING);
      }


   /* copy explored data. */
   memcpy(st->explored_vnums, ch->player_specials->explored_vnums, EXPLORED_BYTES*sizeof(char));

   strcpy(st->email, ch->player_specials->email);


   }
/* Char to store */



void save_etext(struct char_data * ch)
   {
   /* this will be really cool soon */

   }


/* create a new entry in the in-memory index table for the player file */
long create_entry(char *name)
   {
   long i,pos;

   if (top_of_p_table == -1) /* no table */
      {
      CREATE(player_table, struct player_index_element, 1);
      pos = top_of_p_table = 0;
      }
   else if ((pos = get_ptable_by_name(name)) == -1)  /* new name */
      {
      i = ++top_of_p_table + 1;
      RECREATE(player_table, struct player_index_element, i);
      pos = top_of_p_table;
      }
   CREATE(player_table[pos].name, char, strlen(name) + 1);

   player_table[pos].hostname=NULL;

   /* copy lowercase equivalent of name to table field */
   for (i = 0; (player_table[pos].name[i] = LOWER(name[i])); i++)
      /* nothing */
      ;
   return (pos);
   }



/************************************************************************
*  funcs of a (more or less) general utility nature   *
********************************************************************** */


/* read and allocate space for a '~'-terminated string from a given file */
char *fread_string(FILE * fl, char *error)
   {
   char *buf=get_buffer(MAX_STRING_LENGTH);
   char *tmp=get_buffer(512);
   char *rslt;
   register char *point;
   int done = 0, length = 0, templength = 0;

   do
      {
      if (!fgets(tmp, 512, fl))
         {
         log("SYSERR: fread_string: format error at or near %s\n",
             error);
         exit(1);
         }
      /* If there is a '~', end the string; else put an "\r\n" over the '\n'. */
      if ((point = strchr(tmp, '~')) != NULL)
         {
         *point = '\0';
         done = 1;
         }
      else
         {
         point = tmp + strlen(tmp) - 1;
         *(point++) = '\r';
         *(point++) = '\n';
         *point = '\0';
         }

      templength = strlen(tmp);

      if (length + templength >= MAX_STRING_LENGTH)
         {
         log("SYSERR: fread_string: string too large (db.c) %s",tmp);
         log("%s", error);
         exit(1);
         }
      else
         {
         strcat(buf + length, tmp);
         length += templength;
         }
      }
   while (!done)
      ;

   /* allocate space for the new string and copy it */
   if (strlen(buf) > 0)
      {
      CREATE(rslt, char, length + 1);
      strcpy(rslt, buf);
      }
   else
      rslt = NULL;

   release_buffer(tmp);
   release_buffer(buf);
   return rslt;
   }

extern struct char_data *find_char(int n);
extern void reset_casting_data(struct char_data *ch);

/* release memory allocated for a char struct */
void free_char(struct char_data * ch)
   {
   int i;
   struct alias_data *a;
   struct queue_event *tmpq, *tmpq_next;
   struct wait_event_data {
      trig_data *trigger;
      void *go;
      int type;
      };

   if (ch) {
     for (i = 0; i < ch->num_casters; i++) {
       struct char_data *caster = find_char(ch->casting_on_me[i]);
       if (caster) {
	 /*log("Halted %s from casting.", GET_NAME(caster));*/
	 reset_casting_data(caster);
	 send_to_char(caster, "You stop chanting.\r\n");
       }
     }
     if (ch->casting_on_me) {
       free(ch->casting_on_me);
       ch->casting_on_me = NULL;
     }
     ch->num_casters = 0;
   }

   if (ch->player_specials != NULL && ch->player_specials != &dummy_mob)
      {
      while ((a = GET_ALIASES(ch)) != NULL)
         {
         GET_ALIASES(ch) = (GET_ALIASES(ch))->next;
         free_alias(a);
         }

      free(ch->player_specials);
      if (IS_NPC(ch))
         log("SYSERR: Mob %s (#%ld) had player_specials allocated!",
             GET_NAME(ch),GET_MOB_VNUM(ch));
      }

   for(tmpq=command_queue;tmpq;tmpq=tmpq_next)
      {
      tmpq_next=tmpq->next;
      if(tmpq->function==trig_wait_event)
         {
         struct wait_event_data *trgev=(struct wait_event_data *)tmpq->args[0];
         if(trgev->type ==MOB_TRIGGER)
            {
            if((struct char_data *)trgev->go == ch)
               {
		 /*log("FREE: Freeing trigger for %s",GET_NAME(ch));*/
               del_event_queue(tmpq);
               }
            }

         }
      }


   if (!IS_NPC(ch) || (IS_NPC(ch) && GET_MOB_RNUM(ch) == -1))
      {
      /* if this is a player, or a non-prototyped non-player, free all */
      if (GET_NAME(ch))
         free(GET_NAME(ch));
      if (ch->player.title)
         free(ch->player.title);
      if (ch->player.short_descr)
         free(ch->player.short_descr);
      if (ch->player.long_descr)
         free(ch->player.long_descr);
      if (ch->player.description)
         free(ch->player.description);
      }
   else if ((i = GET_MOB_RNUM(ch)) >= 0)
      {
      /* otherwise, free strings only if the string is not pointing at proto */
      if (ch->player.name && ch->player.name != mob_proto[i].player.name)
         free(ch->player.name);
      if (ch->player.title && ch->player.title != mob_proto[i].player.title)
         free(ch->player.title);
      if (ch->player.short_descr && ch->player.short_descr != mob_proto[i].player.short_descr)
         free(ch->player.short_descr);
      if (ch->player.long_descr && ch->player.long_descr != mob_proto[i].player.long_descr)
         free(ch->player.long_descr);
      if (ch->player.description && ch->player.description != mob_proto[i].player.description)
         free(ch->player.description);
      }
   while (ch->affected)
      affect_remove(ch, ch->affected);

   if(ch->desc)
      ch->desc->character=NULL;

   free(ch);
   }




/* release memory allocated for an obj struct */
void free_obj(struct obj_data * obj)
   {
   int nr;
   struct extra_descr_data *thisd, *next_one;
   struct queue_event *tmpq, *tmpq_next;
   struct wait_event_data {
      trig_data *trigger;
      void *go;
      int type;
      };
   if(!obj)
      {
      log("tried to free and invalid object");
      return;
      }

   /*    log("---***@@@***---Freeing %s",obj->short_description); */
   if ((nr = GET_OBJ_RNUM(obj)) == -1)
      {
      for(tmpq=command_queue;tmpq;tmpq=tmpq_next)
         {
         tmpq_next=tmpq->next;
         if((tmpq->function ==skin_update) &&
                 ((struct obj_data *)tmpq->args[1] == obj))
            {
            send_to_char(tmpq->ch,"It seems that your carcass is gone.\r\n");
            REMOVE_BIT(AFF2_FLAGS(tmpq->ch), AFF2_SKINNING);
            del_event_queue(tmpq);
            }
         else if(tmpq->function==trig_wait_event)
            {
            struct wait_event_data *trgev=(struct wait_event_data *)tmpq->args[0];
            if(trgev->type ==MOB_TRIGGER)
               {
               if((struct obj_data *)trgev->go ==obj)
                  {
		    /*log("FREE: Freeing trigger for %s",obj->name);*/
                  del_event_queue(tmpq);
                  }
               }

            }
         }

      if (obj->name)
         free(obj->name);
      if (obj->description)
         free(obj->description);
      if (obj->short_description)
         free(obj->short_description);
      if (obj->action_description)
         free(obj->action_description);
      if (obj->ex_description)
         for (thisd = obj->ex_description; thisd; thisd = next_one)
            {
            next_one = thisd->next;
            if (thisd->keyword)
               free(thisd->keyword);
            if (thisd->description)
               free(thisd->description);
            free(thisd);
            }
      }
   else
      {
      if (obj->name && obj->name != obj_proto[nr].name)
         free(obj->name);
      if (obj->description && obj->description != obj_proto[nr].description)
         free(obj->description);
      if (obj->short_description && obj->short_description != obj_proto[nr].short_description)
         free(obj->short_description);
      if (obj->action_description && obj->action_description != obj_proto[nr].action_description)
         free(obj->action_description);
      if (obj->ex_description && obj->ex_description != obj_proto[nr].ex_description)
         for (thisd = obj->ex_description; thisd; thisd = next_one)
            {
            next_one = thisd->next;
            if (thisd->keyword)
               free(thisd->keyword);
            if (thisd->description)
               free(thisd->description);
            free(thisd);
            }
      }

   free(obj);
   }



/* read contets of a text file, alloc space, point buf to it */
int file_to_string_alloc(char *name, char **buf)
   {
   char *temp=get_buffer(32750);

   if (file_to_string(name, temp) < 0)
      {
      release_buffer(temp);
      return (-1);
      }

   if (*buf)
      free(*buf);

   *buf = str_dup(temp);
   release_buffer(temp);
   return 0;
   }


/* read contents of a text file, and place in buf */
int file_to_string(char *name, char *buf)
   {
   FILE *fl;
   char *tmp=get_buffer(READ_SIZE+3);

   *buf = '\0';

   if (!(fl = fopen(name, "r")))
      {
      log("SYSERR: reading %s: %s", name, strerror(errno));
      release_buffer(tmp);
      return (-1);
      }
   do
      {
	tmp[0] = '\x0';
	fgets(tmp, READ_SIZE, fl);
	if (tmp[0]) {
	  tmp[strlen(tmp) - 1] = '\0'; /* take off the trailing \n */
	}
	strcat(tmp, "\r\n");

      if (!feof(fl))
         {
         if (strlen(buf) + strlen(tmp) + 1 > 32750)
            {
            log("SYSERR: %s: string too big (%d max)", name,
                32750);
            *buf = '\0';
            release_buffer(tmp);
            return -1;
            }
         strcat(buf, tmp);
         }
      }
   while (!feof(fl))
      ;

   fclose(fl);

   release_buffer(tmp);
   return (0);
   }



/* clear some of the the working variables of a char */
void reset_char(struct char_data * ch)
   {
   int i;

   for (i = 0; i < NUM_WEARS; i++)
      GET_EQ(ch, i) = NULL;

   ch->followers = NULL;
   ch->master = NULL;
   IN_ROOM(ch) = NOWHERE;
   ch->carrying = NULL;
   ch->next = NULL;
   ch->next_fighting = NULL;
   ch->next_in_room = NULL;
   FIGHTING(ch) = NULL;
   ch->char_specials.position = POS_STANDING;
   ch->mob_specials.default_pos = POS_STANDING;
   ch->char_specials.carry_weight = 0;
   ch->char_specials.carry_items = 0;
   if((ch->player_specials->saved.screensize<13)||
           (ch->player_specials->saved.screensize>120))
      ch->player_specials->saved.screensize = 25;  /* -naj infobar2 12/16/96 - reset screensize */

   if (GET_HIT(ch) <= 0)
      GET_HIT(ch) = 1;
   if (GET_MOVE(ch) <= 0)
      GET_MOVE(ch) = 1;
   if (GET_MANA(ch) <= 0)
      GET_MANA(ch) = 1;

   GET_LAST_TELL(ch) = NOBODY;
   }



/* clear ALL the working variables of a char; do NOT free any space alloc'ed */
void clear_char(struct char_data * ch)
   {
   memset((char *) ch, 0, sizeof(struct char_data));

   IN_ROOM(ch)      = NOWHERE;
   GET_PFILEPOS(ch) = -1;
   GET_MOB_RNUM(ch) = NOBODY;
   GET_WAS_IN(ch)   = NOWHERE;
   GET_POS(ch)      = POS_STANDING;
   FURNITURE(ch)    = NULL;
   ch->mob_specials.default_pos = POS_STANDING;
   ch->mob_specials.skin = NOTHING;
   GET_LIGHT(ch)    = 0;

   GET_AC(ch) = 200;  /* Basic Armor */
   if (ch->points.max_mana < 100)
      ch->points.max_mana = 100;
   }


void clear_object(struct obj_data * obj)
   {
   memset((char *) obj, 0, sizeof(struct obj_data));

   GET_OBJ_RNUM(obj) = NOTHING;
   IN_ROOM(obj)      = NOWHERE;
   obj->worn_on      = NOWHERE;
   GET_OBJ_VROOM(obj)= NOWHERE;
   GET_OBJ_TIMER(obj)= 0;
   GET_OBJ_SHOP_ORDER(obj) = 0;
   }




/* initialize a new character only if class is set */
void init_char(struct char_data * ch)
   {
   int i, race;

   /* create a player_special structure */
   if (ch->player_specials == NULL)
      CREATE(ch->player_specials, struct player_special_data, 1);

   /* *** if this is our first player --- he be God *** */

   if (top_of_p_table == 0)
      {
      GET_EXP(ch) = 7000000;
      GET_LEVEL(ch) = LVL_IMPL;

      ch->points.max_hit = 500;
      ch->points.max_mana = 100;
      ch->points.max_move = 82;
      }
   set_title(ch, NULL);

   ch->player.short_descr = NULL;
   ch->player.long_descr = NULL;
   ch->player.description = NULL;

   ch->player.hometown = 0;

   ch->player.time.birth = time(0);
   ch->player.time.played = 0;
   ch->player.time.logon = time(0);

   for (i = 0; i < MAX_TONGUE; i++)
      GET_TALK(ch, i) = 0;

   /* make favors for sex */
   race = ch->player.race;
   if (ch->player.sex == SEX_MALE)
      {
      ch->player.weight = number(race_size_info[race].MWmin,
 				 race_size_info[race].MWmax);
      ch->player.height = number(race_size_info[race].MHmin,      
                                 race_size_info[race].MHmax);
      }
   else
      {
      ch->player.weight = number(race_size_info[race].FWmin,      
                                 race_size_info[race].FWmax);
      ch->player.height = number(race_size_info[race].FHmin,            
                                 race_size_info[race].FHmax);
      }

   ch->points.max_mana = 100;
   ch->points.mana = GET_MAX_MANA(ch);
   ch->points.hit = GET_MAX_HIT(ch);
   ch->points.max_move = 82;
   ch->points.move = GET_MAX_MOVE(ch);
   ch->points.armor = 200;

   for(i=0;i<5;i++)
      ch->points.gold[i]=0;
   for(i=0;i<32;i++)
      ch->points.bank_gold[i]=0;


   if ((i = get_ptable_by_name(GET_NAME(ch))) != -1)
      player_table[i].id = GET_IDNUM(ch) = ++top_idnum;
   else
      log("SYSERR: init_char: Character '%s' not found in player table.", GET_NAME(ch));


   for (i = 1; i <= MAX_SKILLS; i++)
      {
      if (GET_LEVEL(ch) < LVL_IMPL)
         SET_SKILL(ch, i, 0);
      else
         SET_SKILL(ch, i, 100);
      }

   ch->char_specials.saved.affected_by = 0;
   ch->char_specials.saved.affected_by2 = 0;
   ch->char_specials.saved.affected_by3 = 0;



   GET_SPELL_FAIL(ch)=0;
   for (i = 0; i < 5; i++)
      GET_SAVE(ch, i) = 0;

   for(i=0;i<64;i++)
      {
      GET_KILLS_VNUM(ch,i)=0;
      GET_KILLS_AMMOUNT(ch,1)=0;
      }

   ch->real_abils.intel = 25;
   ch->real_abils.wis = 25;
   ch->real_abils.dex = 25;
   ch->real_abils.str = 25;
   ch->real_abils.str_add = 100;
   ch->real_abils.con = 25;
   ch->real_abils.cha = 25;

   for (i = 0; i < 3; i++)
      GET_COND(ch, i) = (GET_LEVEL(ch) == LVL_IMPL ? -1 : 24);

   GET_LOADROOM(ch) = NOWHERE;
   }



/* returns the real number of the room with given virtual number */
room_rnum real_room(room_vnum vnum)
   {
   room_rnum bot, top, mid;

   bot = 0;
   top = top_of_world;

   /* perform binary search on world-table */
   for (;;)
      {
      mid = (bot + top) / 2;

      if ((world + mid)->number == vnum)
         return mid;
      if (bot >= top)
         return NOWHERE;
      if ((world + mid)->number > vnum)
         top = mid - 1;
      else
         bot = mid + 1;
      }
   }



/* returns the real number of the monster with given virtual number */
mob_rnum real_mobile(mob_vnum vnum)
   {
   mob_rnum bot, top, mid;

   bot = 0;
   top = top_of_mobt;

   /* perform binary search on mob-table */
   for (;;)
      {
      mid = (bot + top) / 2;

      if ((mob_index + mid)->vnum == vnum)
         return (mid);
      if (bot >= top)
         return (-1);
      if ((mob_index + mid)->vnum > vnum)
         top = mid - 1;
      else
         bot = mid + 1;
      }
   }



/* returns the real number of the object with given vnum number */
obj_rnum real_object(obj_vnum vnum)
   {
   obj_rnum bot, top, mid;

   bot = 0;
   top = top_of_objt;

   /* perform binary search on obj-table */
   for (;;)
      {
      mid = (bot + top) / 2;

      if ((obj_index + mid)->vnum == vnum)
         return (mid);
      if (bot >= top)
         return (-1);
      if ((obj_index + mid)->vnum > vnum)
         top = mid - 1;
      else
         bot = mid + 1;
      }
   }


int my_obj_save_to_disk(FILE *fp, struct obj_data *obj, int locate)
   {
   int i;
   long lVector=0;
   struct extra_descr_data *ex_desc;
   char *buf1=get_buffer(MAX_STRING_LENGTH +1);
   int error=0;
   int reterror=0;
   if (obj->action_description)
      {
      strcpy(buf1, obj->action_description);
      strip_string(buf1);
      }
   else
      *buf1 = 0;

   if(GET_OBJ_VNUM(obj) == NOTHING)
      {
      SET_BIT(GET_OBJ_EXTRA(obj),ITEM_UNIQUE_SAVE);
      for(i=0;i<8;i++)
         {
         if(GET_OBJ_VAL(obj,i) != 0)
            SET_BIT(lVector, (1 << i));
         }
      }
   else
      {
      for(i=0;i<8;i++)
         {
         if(obj_proto[GET_OBJ_RNUM(obj)].obj_flags.value[i] !=
                 GET_OBJ_VAL(obj,i))
            SET_BIT(lVector, (1 << i));
         }
      }

   /* don't save the weight of objs in containers*/
   if(GET_OBJ_TYPE(obj)==ITEM_CONTAINER)
      REMOVE_BIT(lVector, (1 << 5));

   error=fprintf(fp,
                 "#%ld\n"
                 "%d %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %d %ld\n",
                 GET_OBJ_VNUM(obj),
                 locate,
                 GET_OBJ_VAL(obj, 0),
                 GET_OBJ_VAL(obj, 1),
                 GET_OBJ_VAL(obj, 2),
                 GET_OBJ_VAL(obj, 3),
                 GET_OBJ_VAL(obj, 4),
                 GET_OBJ_VAL(obj, 5),
                 GET_OBJ_VAL(obj, 6),
                 GET_OBJ_VAL(obj, 7),
                 GET_OBJ_EXTRA(obj),
                 GET_OBJ_VROOM(obj), /* vroom is the virtual room a corpse */
                 GET_OBJ_TIMER(obj), /* was created in. See make_corpse */
                 lVector);
   if(error<0)
      reterror=error;

   error=fprintf(fp,"%d %d %d %ld %ld %ld %ld %d\n",
                 GET_OBJ_CSLOTS(obj),
                 GET_OBJ_TSLOTS(obj),
                 GET_OBJ_OSLOTS(obj),
                 GET_OBJ_EXTRA2(obj),
                 GET_OBJ_EXTRA3(obj),
                 GET_OBJ_ANTI(obj),
                 obj->obj_flags.bitvector,
                 GET_OBJ_SHOP_ORDER(obj));
   if(error<0)
      reterror=error;

   if(!(IS_OBJ_STAT(obj,ITEM_UNIQUE_SAVE)))
      {
      release_buffer(buf1);
      if(reterror<0)
         return reterror;
      return 1;
      }
   error =fprintf(fp,
                  "XAP\n"
                  "%s~\n"
                  "%s~\n"
                  "%s~\n"
                  "%s~\n"
                  "%d %ld %d %d %d\n",
                  obj->name ? obj->name : "undefined",
                  obj->short_description ? obj->short_description : "undefined",
                  obj->description ? obj->description : "undefined",
                  buf1,
                  GET_OBJ_TYPE(obj),
                  GET_OBJ_WEAR(obj),
                  GET_OBJ_WEIGHT(obj),
                  GET_OBJ_COST(obj),
                  GET_OBJ_RENT(obj)
                 );
   if(error<0)
      reterror=error;

   /* Do we have affects? */
   for (i = 0; i < MAX_OBJ_AFFECT; i++)
      if (obj->affected[i].modifier)
         {
         error=fprintf(fp, "A\n"
                       "%d %ld\n",
                       obj->affected[i].location,
                       obj->affected[i].modifier
                      );
         if(error<0)
            reterror=error;
         }
   /* Do we have extra descriptions? */
   if (obj->ex_description)
      {        /*. Yep, save them too . */
      for (ex_desc = obj->ex_description; ex_desc; ex_desc = ex_desc->next)
         {
         /*. Sanity check to prevent nasty protection faults . */
         if (!*ex_desc->keyword || !*ex_desc->description)
            {
            continue;
            }
         strcpy(buf1, ex_desc->description);
         strip_string(buf1);
         error=fprintf(fp, "E\n"
                       "%s~\n"
                       "%s~\n",
                       ex_desc->keyword,
                       buf1
                      );
         if(error<0)
            reterror=error;
         }
      }
   release_buffer(buf1);
   if(reterror<0)
      return reterror;
   return 1;
   }

/* This procedure removes the '\r\n' from a string so that it may be
  saved to a file.  Use it only on buffers, not on the orginal
  strings. */

void strip_string(char *strip_buffer)
   {
   register char *ptr, *str;

   ptr = strip_buffer;
   str = ptr;

   while(*ptr && (*ptr == '\r'))
      ptr++;

   while ((*str = *ptr))
      {
      str++;
      ptr++;
      if (*ptr == '\r')
         ptr++;
      }
   }

/*
 * Extend later to include more checks.
 *
 * TODO: Add checks for unknown bitvectors.
 */
int check_object(struct obj_data *obj)
   {
   int error = FALSE;

   if (GET_OBJ_WEIGHT(obj) < 0 && (error = TRUE))
      log("SYSERR: Object #%ld (%s) has negative weight (%d).",
          GET_OBJ_VNUM(obj), obj->short_description, GET_OBJ_WEIGHT(obj));

   if (GET_OBJ_RENT(obj) < 0 && (error = TRUE))
      log("SYSERR: Object #%ld (%s) has negative cost/day (%d).",
          GET_OBJ_VNUM(obj), obj->short_description, GET_OBJ_RENT(obj));

   switch (GET_OBJ_TYPE(obj))
      {
      case ITEM_DRINKCON:
      case ITEM_FOUNTAIN:
         if (GET_OBJ_VAL(obj, 1) > GET_OBJ_VAL(obj, 0) && (error = TRUE))
            log("SYSERR: Object #%ld (%s) contains (%ld) more than "
                "maximum (%ld).",
                GET_OBJ_VNUM(obj), obj->short_description,
                GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 0));
         break;
      case ITEM_SCROLL:
      case ITEM_POTION:
         error |= check_object_level(obj, 0);
         error |= check_object_spell_number(obj, 1);
         error |= check_object_spell_number(obj, 2);
         error |= check_object_spell_number(obj, 3);
         break;
      case ITEM_WAND:
      case ITEM_STAFF:
         error |= check_object_level(obj, 0);
         error |= check_object_spell_number(obj, 3);
         if (GET_OBJ_VAL(obj, 2) > GET_OBJ_VAL(obj, 1) && (error = TRUE))
            log("SYSERR: Object #%ld (%s) has more charges (%ld) "
                "than maximum (%ld).",
                GET_OBJ_VNUM(obj), obj->short_description,
                GET_OBJ_VAL(obj, 2), GET_OBJ_VAL(obj, 1));
         break;
      }

   return error;
   }

int check_object_spell_number(struct obj_data *obj, int val)
   {
   int error = FALSE;
   const char *spellname;
   if (GET_OBJ_VAL(obj, val) == -1)     /* i.e.: no spell */
      return error;

   /*
    * Check for negative spells, spells beyond the top define, and any
    * spell which is actually a skill.
    */
   if (GET_OBJ_VAL(obj, val) < 0)
      error = TRUE;
   if (GET_OBJ_VAL(obj, val) > TOP_SPELL_DEFINE)
      error = TRUE;
   if (GET_OBJ_VAL(obj, val) > MAX_SPELLS && GET_OBJ_VAL(obj, val)<=MAX_SKILLS)
      {
      error = TRUE;
      }
   if (error)
      log("SYSERR: Object #%ld (%s) has out of range spell #%ld.",
          GET_OBJ_VNUM(obj), obj->short_description, GET_OBJ_VAL(obj, val));
   /*
    * This bug has been fixed, but if you don't like the special behavior...
    */
#if 0

   if (GET_OBJ_TYPE(obj) == ITEM_STAFF &&
           HAS_SPELL_ROUTINE(GET_OBJ_VAL(obj, val), MAG_AREAS | MAG_MASSES))
      log("... '%s' (#%ld) uses %s spell '%s'.",
          obj->short_description, GET_OBJ_VNUM(obj),
          HAS_SPELL_ROUTINE(GET_OBJ_VAL(obj, val), MAG_AREAS) ? "area" :"mass",
          skill_name(GET_OBJ_VAL(obj, val)));
#endif


   /* Now check for unnamed spells. */
   spellname = skill_name(GET_OBJ_VAL(obj, val));
   if ((!strcmp(unused_spellname,spellname) || !strcmp("UNDEFINED",spellname)) &&
           (error = TRUE))
      {
      log("SYSERR: Object #%ld (%s) uses '%s' spell #%ld.",
          GET_OBJ_VNUM(obj), obj->short_description, spellname,
          GET_OBJ_VAL(obj, val));
      GET_OBJ_VAL(obj,val)=-1;
      }

   return error;
   }
int check_object_level(struct obj_data *obj, int val)
   {
   int error = FALSE;

   if ((GET_OBJ_VAL(obj, val) < 0 || GET_OBJ_VAL(obj, val) > LVL_IMPL) &&
           (error = TRUE))
      log("SYSERR: Object #%ld (%s) has out of range level #%ld.",
          GET_OBJ_VNUM(obj), obj->short_description, GET_OBJ_VAL(obj, val));

   return error;
   }


/* returns the real number of the path with given virtual number */
long real_path(long vnum)
   {
   long bot, top, mid;

   bot = 0;
   top = top_path;

   /* perform binary search on obj-table */
   for (;;)
      {
      mid = (bot + top) / 2;

      if ((path_index + mid)->number == vnum)
         return (mid);
      if (bot >= top)
         return (-1);
      if ((path_index + mid)->number > vnum)
         top = mid - 1;
      else
         bot = mid + 1;
      }
   }

void vwear_object(int wearpos, struct char_data * ch)
   {
   long nr, found = 0;
   char *buf=get_buffer(32750);

   for (nr = 0; nr <= top_of_objt; nr++)
      {
	if (GET_LEVEL(ch) < VWEAR_LEVEL && !is_olc_set(ch, obj_index[nr].vnum/100)) {
	  continue;
	}


      if(IS_SET(obj_proto[nr].obj_flags.wear_flags,wearpos))
         {
         if(strlen(buf)>32000)
            {
            sprintf(buf+strlen(buf),"Tell Masque we blew out the vwear buffer on pos %d\r\n",wearpos);
            nr=top_of_objt+1;
            }
         else
            sprintf(buf+strlen(buf), "%3ld. [%5ld] %s\r\n", ++found,
                    obj_index[nr].vnum, obj_proto[nr].short_description);
         }
      }
   if(ch->desc)
      page_string(ch->desc,buf,TRUE,"");


   release_buffer(buf);
   }


/* MOBProg functions */

/* This routine transfers between alpha and numeric forms of the
 * mob_prog bitvector types.  This allows the use of the words in the
 * mob/script files.
 */

int mprog_name_to_type (char *name)
   {
   if (!str_cmp(name, "in_file_prog"  ))
      return IN_FILE_PROG;
   if (!str_cmp(name, "act_prog"      ))
      return ACT_PROG;
   if (!str_cmp(name, "speech_prog"   ))
      return SPEECH_PROG;
   if (!str_cmp(name, "rand_prog"     ))
      return RAND_PROG;
   if (!str_cmp(name, "fight_prog"    ))
      return FIGHT_PROG;
   if (!str_cmp(name, "hitprcnt_prog" ))
      return HITPRCNT_PROG;
   if (!str_cmp(name, "death_prog"    ))
      return DEATH_PROG;
   if (!str_cmp(name, "entry_prog"    ))
      return ENTRY_PROG;
   if (!str_cmp(name, "greet_prog"    ))
      return GREET_PROG;
   if (!str_cmp(name, "all_greet_prog"))
      return ALL_GREET_PROG;
   if (!str_cmp(name, "give_prog"     ))
      return GIVE_PROG;
   if (!str_cmp(name, "bribe_prog"    ))
      return BRIBE_PROG;

   return(ERROR_PROG);
   }


/*
    * Read a number from a file.
    */
long fread_number(FILE *fp)
   {
   long vnumber;
   bool sign;
   char c;

   do
      {
      c = getc(fp);
      }
   while (isspace((int)c));

   vnumber = 0;

   sign   = FALSE;
   if (c == '+')
      {
      c = getc(fp);
      }
   else if (c == '-')
      {
      sign = TRUE;
      c = getc(fp);
      }


   if (!isdigit((int)c))
      {
      log("Fread_number: bad format.");
      exit(1);
      }

   while (isdigit((int)c))
      {
      vnumber = vnumber * 10 + c - '0';
      c = getc(fp);
      }

   if (sign)
      vnumber = 0 - vnumber;

   if (c == '|')
      vnumber += fread_number(fp);
   else if (c != ' ')
      ungetc(c, fp);

   return vnumber;
   }

/*
  * Read to end of line (for comments).
  */
void fread_to_eol(FILE *fp)
   {
   char c;

   do
      {
      c = getc(fp);
      }
   while (c != '\n' && c != '\r');

   do
      {
      c = getc(fp);
      }
   while (c == '\n' || c == '\r');

   ungetc(c, fp);
   return;
   }


/*
  * Read one word (into static buffer).
  */
char *fread_word(FILE *fp)
   {
   static char word[MAX_INPUT_LENGTH];
   char *pword;
   char cEnd;

   do
      {
      cEnd = getc(fp);
      }
   while (isspace((int)cEnd));

   if (cEnd == '\'' || cEnd == '"')
      {
      pword   = word;
      }
   else
      {
      word[0] = cEnd;
      pword   = word+1;
      cEnd    = ' ';
      }


   for (; pword < word + MAX_INPUT_LENGTH; pword++)
      {
      *pword = getc(fp);
      if (cEnd == ' ' ? isspace((int)*pword) || *pword == '~' : *pword == cEnd)
         {
         if (cEnd == ' ' || cEnd == '~')
            ungetc(*pword, fp);
         *pword = '\0';
         return word;
         }
      }

   log("SYSERR: Fread_word: word too long.");
   exit(1);
   return NULL;
   }


/* This routine reads in scripts of MOBprograms from a file */

MPROG_DATA* mprog_file_read(char *f, MPROG_DATA *mprg,
                            struct index_data *pMobIndex)
   {

   char       *MOBProgfile=get_buffer(MAX_INPUT_LENGTH);
   MPROG_DATA *mprg2;
   FILE       *progfile;
   char        letter;
   bool        done = FALSE;
   char *buf2;

   sprintf(MOBProgfile, "%s/%s", MOB_DIR, f);

   progfile = fopen(MOBProgfile, "r");
   release_buffer(MOBProgfile);
   if (!progfile)
      {
      log("Mob: %ld couldnt open mobprog file", pMobIndex->vnum);
      exit(1);
      }

   mprg2 = mprg;
   switch (letter = fread_letter(progfile))
      {
      case '>':
         break;
      case '|':
         log("empty mobprog file.");
         exit(1);
         break;
      default:
         log("in mobprog file syntax error.");
         exit(1);
         break;
      }

   while (!done)
      {
      mprg2->type = mprog_name_to_type(fread_word(progfile));
      switch (mprg2->type)
         {
         case ERROR_PROG:
            log("mobprog file type error");
            exit(1);
            break;
         case IN_FILE_PROG:
            log("mprog file contains a call to file.");
            exit(1);
            break;
         default:
            buf2=get_buffer(256);
            sprintf(buf2, "Error in file %s", f);
            pMobIndex->progtypes = pMobIndex->progtypes | mprg2->type;
            mprg2->arglist       = fread_string(progfile,buf2);
            mprg2->comlist       = fread_string(progfile,buf2);
            release_buffer(buf2);
            switch (letter = fread_letter(progfile))
               {
               case '>':
                  /*       mprg2->next = (MPROG_DATA *)malloc(sizeof(MPROG_DATA)); */
                  CREATE(mprg2->next,MPROG_DATA,1);
                  mprg2       = mprg2->next;
                  mprg2->next = NULL;
                  break;
               case '|':
                  done = TRUE;
                  break;
               default:
                  log("in mobprog file %s syntax error.", f);
                  exit(1);
                  break;
               }
            break;
         }
      }
   fclose(progfile);
   return mprg2;
   }


struct index_data *get_obj_index (long vnum)
   {
   long nr;
   for(nr = 0; nr <= top_of_objt; nr++)
      {
      if(obj_index[nr].vnum == vnum)
         return &obj_index[nr];
      }
   return NULL;
   }

struct index_data *get_mob_index (long vnum)
   {
   long nr;
   for(nr = 0; nr <= top_of_mobt; nr++)
      {
      if(mob_index[nr].vnum == vnum)
         return &mob_index[nr];
      }
   return NULL;
   }


/* This procedure is responsible for reading any in_file MOBprograms.
  */

void mprog_read_programs(FILE *fp, struct index_data *pMobIndex)
   {
   MPROG_DATA *mprg;
   char        letter;
   bool        done = FALSE;
   char *buf2=get_buffer(MAX_STRING_LENGTH);

   if ((letter = fread_letter(fp)) != '>')
      {
      log("Load_mobiles: vnum %ld MOBPROG char", pMobIndex->vnum);
      exit(1);
      }
   /*    pMobIndex->mobprogs = (MPROG_DATA *)malloc(sizeof(MPROG_DATA)); */
   CREATE(pMobIndex->mobprogs,MPROG_DATA,1);
   mprg = pMobIndex->mobprogs;

   while (!done)
      {
      mprg->type = mprog_name_to_type(fread_word(fp));
      switch (mprg->type)
         {
         case ERROR_PROG:
            log("Load_mobiles: vnum %ld MOBPROG type.", pMobIndex->vnum);
            exit(1);
            break;
         case IN_FILE_PROG:
            sprintf(buf2, "Mobprog for mob #%ld", pMobIndex->vnum);
            mprg = mprog_file_read(fread_word(fp), mprg,pMobIndex);
            fread_to_eol(fp);   /* need to strip off that silly ~*/
            switch (letter = fread_letter(fp))
               {
               case '>':
                  /*       mprg->next = (MPROG_DATA *)malloc(sizeof(MPROG_DATA)); */
                  CREATE(mprg->next,MPROG_DATA,1);
                  mprg       = mprg->next;
                  mprg->next = NULL;
                  break;
               case '|':
                  mprg->next = NULL;
                  fread_to_eol(fp);
                  done = TRUE;
                  break;
               default:
                  log("Load_mobiles: vnum %ld bad MOBPROG.",pMobIndex->vnum);
                  exit(1);
                  break;
               }
            break;
         default:
            sprintf(buf2, "Mobprog for mob #%ld", pMobIndex->vnum);
            pMobIndex->progtypes = pMobIndex->progtypes | mprg->type;
            mprg->arglist        = fread_string(fp, buf2);
            mprg->comlist        = fread_string(fp, buf2);
            switch (letter = fread_letter(fp))
               {
               case '>':
                  /*       mprg->next = (MPROG_DATA *)malloc(sizeof(MPROG_DATA)); */
                  CREATE(mprg->next,MPROG_DATA,1);
                  mprg       = mprg->next;
                  mprg->next = NULL;
                  break;
               case '|':
                  mprg->next = NULL;
                  fread_to_eol(fp);
                  done = TRUE;
                  break;
               default:
                  log("Load_mobiles: vnum %ld bad MOBPROG (%c).",
                      pMobIndex->vnum, letter);
                  exit(1);
                  break;
               }
            break;
         }
      }

   release_buffer(buf2);
   return;
   }

int strip_zone_color(char *inbuf)
   {
   int j=0;
   int color=-1;
   int zone;
   int tmp_color;
   if(*inbuf=='\0')
      return ZONE_UNASSIGNED;
   while(inbuf[j]!='\0')
      {
      if((inbuf[j]=='&')&&!((tmp_color=is_colour(inbuf[j+1]))==-1))
         {
         if(color==-1)
            color=tmp_color;
         inbuf[j]=' ';
         inbuf[j+1]=' ';
         }
      j++;
      }
   if((color==1)||(color==8))
      zone=ZONE_AGLARON;
   else if((color==2)||(color==9))
      zone=ZONE_UNASSIGNED;
   else if((color==3)||(color==10))
      zone=ZONE_UNDERDARK;
   else if((color==4)||(color==11))
      zone=ZONE_OTHER_GOD;
   else if((color==5)||(color==12))
      zone=ZONE_OCEAN;
   else if((color==6)||(color==13))
      zone=ZONE_CALEDON;
   else if((color==7)||(color==14))
      zone=ZONE_ARCTIC;
   else
      zone=ZONE_UNASSIGNED;
   return zone;
   }


int num_dg_quests = 0;
struct dg_quest *dg_quests = NULL;

struct dg_quest *get_dg_quest(char *quest_name)
{
  if (!quest_name) {
    return NULL;
  }
  int i;
  for (i = 0; i < num_dg_quests; i++) {
    if (!strcmp(quest_name, dg_quests[i].quest_name)) {
      return &dg_quests[i];
    }
  }
  return NULL;
}

void load_dg_quests(void)
{
  num_dg_quests = 0;
  dg_quests = NULL;

  FILE *fp = fopen(DG_QUEST_FILE, "r");
  if (!fp) {
    /* Maybe the file doesn't exist.  Try making a blank one? */
    fp = fopen(DG_QUEST_FILE, "w+");
    if (!fp) {
      log("SYSERR: Could not create DG_QUEST_FILE %s.", DG_QUEST_FILE);
      return;
    }
  }

  char *buf = get_buffer(65536);
  int line = 0;
  while (fgets(buf, 65535, fp)) {
    line++;
    buf[65535] = '\x0';
    if (buf[0] == '\r' || buf[0] == '\n') {
      continue;
    }
    char *quest_name = strtok(buf, " ");
    if (!quest_name) {
      log("SYSERR: DG_QUEST_FILE has a bad line number #%d.  Ignoring.", line);
      continue;
    }
    dg_quests = (struct dg_quest *)realloc(dg_quests, ++num_dg_quests * sizeof(struct dg_quest));
    struct dg_quest *quest = &dg_quests[num_dg_quests-1];
    quest->quest_name = strdup(quest_name);
    quest->num_completed = 0;
    quest->completed_by = NULL;
    char *token;
    while ((token = strtok(NULL, " \r\n"))) {
      quest->completed_by = (int *)realloc(quest->completed_by, ++quest->num_completed * sizeof(int));
      quest->completed_by[quest->num_completed-1] = atoi(token);
    }
  }
  fclose(fp);
  release_buffer(buf);
}

void save_dg_quests(void)
{
  FILE *fp = fopen(DG_QUEST_FILE, "w");
  if (!fp) {
    /* Maybe the file doesn't exist.  Try making a blank one? */
    fp = fopen(DG_QUEST_FILE, "w+");
    if (!fp) {
      log("SYSERR: Could not create DG_QUEST_FILE %s.", DG_QUEST_FILE);
      return;
    }
  }

  int i, j;
  for (i = 0; i < num_dg_quests; i++) {
    fprintf(fp, "%s", dg_quests[i].quest_name);
    for (j = 0; j < dg_quests[i].num_completed; j++) {
      fprintf(fp, " %d", dg_quests[i].completed_by[j]);
    }
    fprintf(fp, "\n");
  }
  fclose(fp);
}

struct graffiti *graffiti = NULL;
int num_graffiti = 0;

void load_graffiti(void)
{
  graffiti = NULL;
  num_graffiti = 0;

  FILE *fp = fopen(GRAFFITI_FILE, "r");
  if (!fp) {
    log("SYSERR: Can not open graffiti file %s.", GRAFFITI_FILE);
    return;
  }
  char *buf = get_buffer(MAX_STRING_LENGTH);
  while (fgets(buf, MAX_STRING_LENGTH, fp)) {
    graffiti = (struct graffiti *)realloc(graffiti, ++num_graffiti * sizeof(struct graffiti));
    graffiti[num_graffiti-1].author = atoi(strtok(buf, " "));
    graffiti[num_graffiti-1].permanent = atoi(strtok(NULL, " "));
    graffiti[num_graffiti-1].room_vnum = atoi(strtok(NULL, " "));
    graffiti[num_graffiti-1].text = strdup(strtok(NULL, ""));
    char *text = graffiti[num_graffiti-1].text;
    text[strlen(text)-1] = '\x0'; /* Remove the newline. */
  }
  fclose(fp);
  release_buffer(buf);
  log("Graffiti: Loaded %d messages.", num_graffiti);
}

void save_graffiti(void)
{
  int i;
  FILE *fp = fopen(GRAFFITI_FILE, "w");
  if (!fp) {
    log("SYSERR: Can not open graffiti file %s.", GRAFFITI_FILE);
    return;
  }
  for (i = 0; i < num_graffiti; i++) {
    if (graffiti[i].permanent) {
      fprintf(fp, "%d %d %d %s\n",
        graffiti[i].author,
	graffiti[i].permanent ? 1 : 0,
	graffiti[i].room_vnum,
	graffiti[i].text
      );    
    }
  }
  fclose(fp);
}

int get_graffiti(int vnum, char *output, int len)
{
  int i, found = 0;
  sprintf(output, "The graffiti reads...\r\n");
  for (i = 0; i < num_graffiti; i++) {
    if (graffiti[i].room_vnum == vnum) {
      if (strlen(output) + strlen(graffiti[i].text) < len-8) {
	strcat(output, "  ");
	strcat(output, graffiti[i].text);
	strcat(output, "\r\n");
	found = 1;
      }
    }
  }
  return found;
}

int remove_graffiti(int author, int vnum)
{
  int found = 0;

  struct graffiti *copy = (struct graffiti *)malloc(num_graffiti * sizeof(struct graffiti));
  int i, in = 0;

  for (i = 0; i < num_graffiti; i++) {
    if (graffiti[i].room_vnum == vnum && (author == -1 || graffiti[i].author == author)) { 
      found = 1;
      continue;
    }
    memcpy(&copy[in++], &graffiti[i], sizeof(struct graffiti));
  }
  graffiti = (struct graffiti *)realloc(graffiti, in * sizeof(struct graffiti));
  memcpy(graffiti, copy, in * sizeof(struct graffiti));
  free(copy);
  num_graffiti = in;
  return found;
}

int remove_graffiti_at(int in)
{
  if (in < 0 || in >= num_graffiti) {
    return 0;
  }

  struct graffiti *copy = (struct graffiti *)malloc((num_graffiti-1) * sizeof(struct graffiti));
  
  if (in == 0) {
    memcpy(copy, graffiti+1, (num_graffiti-1) * sizeof(struct graffiti));
  } else if (in == num_graffiti-1) {
    memcpy(copy, graffiti, (num_graffiti-1) * sizeof(struct graffiti));
  } else {
    memcpy(copy, graffiti, in * sizeof(struct graffiti));
    memcpy(copy + in, graffiti + in + 1, (num_graffiti-in-1) * sizeof(struct graffiti));
  }

  free(graffiti);
  graffiti = copy;
  num_graffiti--;
  return 1;
}

int graffiti_exists(int vnum)
{
  int i;
  for (i = 0; i < num_graffiti; i++) {
    if (graffiti[i].room_vnum == vnum) {
      return 1;
    }
  }
  return 0;
}

int get_graffiti_count(int author, int permanent)
{
  int i;
  int count = 0;
  for (i = 0; i < num_graffiti; i++) {
    if (graffiti[i].author == author && graffiti[i].permanent == permanent) {
      count++;
    }
  }
  return count;
}

void load_default_player_stats( void )
{
  FILE* fp = fopen( "etc/default_player_stats.txt", "r" );
  if( !fp ) {
    printf( "ERROR: Could not load etc/default_player_stats.txt\n" );
    exit( 1 );
  }

  char buffer[ 2048 ];
  int index = 0;
  while( 1 )
  {
    fgets( buffer, 2047, fp );
    buffer[ 2047 ] = '\x0';

    if( buffer[ 0 ] == '~' )
    {
      break;
    }
    buffer[ strlen(buffer) - 1 ] = '\x0';

    char* race = strtok( buffer, " " );
    char* clazz = strtok( NULL, " " );
    char* str = strtok( NULL, " " );
    char* intel = strtok( NULL, " " );
    char* wis = strtok( NULL, " " );
    char* dex = strtok( NULL, " " );
    char* con = strtok( NULL, " " );
    char* cha = strtok( NULL, " " );
    if( !race || !clazz || !str || !intel || !wis || !dex || !con || !cha )
    {
      printf("  ERROR: malformed line!\n ");
      exit( 1 );
    }

    int clazz_ind = 0;
    while( strcmp( class_abbrevs[ clazz_ind ], clazz ) && strcmp( class_abbrevs[ clazz_ind++ ], "\n" ) );
    int race_ind = 0;
    while( strcmp( race_abbrevs[ race_ind ], race ) && strcmp( race_abbrevs[ race_ind++ ], "\n" ) );

    if( strcmp( class_abbrevs[ clazz_ind ], "\n" ) == 0
        || strcmp( race_abbrevs[ race_ind ], "\n" ) == 0 )
    {
      printf("  ERROR: malformed line (not recognized race/class)\n");
      exit( 2 );
    }

    default_player_stats[ index ].clazz = clazz_ind;
    default_player_stats[ index ].race = race_ind;
    default_player_stats[ index ].abilities.str = atoi( str );
    default_player_stats[ index ].abilities.intel = atoi( intel );
    default_player_stats[ index ].abilities.wis = atoi( wis );
    default_player_stats[ index ].abilities.dex = atoi( dex );
    default_player_stats[ index ].abilities.con = atoi( con );
    default_player_stats[ index ].abilities.cha = atoi( cha );

    index++;
  }
  num_default_player_stats = index;

  fclose( fp );
}

void set_default_player_stats( struct char_data* ch )
{
  int race = GET_RACE( ch );
  int clazz = GET_CLASS( ch );

  int i;
  for( i = 0; i != num_default_player_stats; ++i )
  {
    if( default_player_stats[ i ].race == race
        && default_player_stats[ i ].clazz == clazz )
    {
      ch->real_abils.str = default_player_stats[ i ].abilities.str;
      ch->real_abils.intel = default_player_stats[ i ].abilities.intel;
      ch->real_abils.wis = default_player_stats[ i ].abilities.wis;
      ch->real_abils.dex = default_player_stats[ i ].abilities.dex;
      ch->real_abils.con = default_player_stats[ i ].abilities.con;
      ch->real_abils.cha = default_player_stats[ i ].abilities.cha;
      break;
    }
  }
}
