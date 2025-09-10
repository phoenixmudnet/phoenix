/* ************************************************************************ 
*   File: comm.c                                        Part of CircleMUD * 
*  Usage: Communication, socket handling, main(), central game loop       * 
*                                                                         * 
*  All rights reserved.  See license.doc for complete information.        * 
*                                                                         * 
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University * 
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               * 
************************************************************************ */ 
 
#define __COMM_C__ 
 
#include "../localHeader/conf.h" 
#include "../localHeader/sysdep.h" 
 
#include <sys/socket.h> 
#include <sys/resource.h> 
#include <netinet/in.h> 
#include <netdb.h> 
#include <signal.h> 

#include <stdarg.h> 

#include "structs.h" 
#include "buffer.h"
#include "utils.h" 
#include "comm.h" 
#include "interpreter.h" 
#include "handler.h" 
#include "db.h" 
#include "house.h" 
#include "olc.h" 
#include "ident.h" 
/** 2/24/97 , Anduin -- including screen for auction **/ 
#include "screen.h" 
#include "queue.h" 
#include "constants.h"
#include "dg_scripts.h"
#include "spells.h"
 
#ifdef HAVE_ARPA_TELNET_H 
#include <arpa/telnet.h> 
#else 
#include "telnet.h" 
#endif 
 
#ifndef INVALID_SOCKET 
#define INVALID_SOCKET -1 
#endif 

#define MSSP 70
#define MSSP_VAR 1
#define MSSP_VAL 2
 
FILE *logfile = NULL;           /* Where to send the log messages. */
/* externs */ 
extern int circle_restrict; 
extern int mini_mud; 
extern int no_rent_check; 
extern FILE *player_fl; 
extern int DFLT_PORT; 
extern char *DFLT_DIR; 
extern const char *LOGNAME;
extern int MAX_PLAYERS; 
extern int MAX_DESCRIPTORS_AVAILABLE; 
extern int xap_objs;           /* ascii objects. */
extern time_t boot_time;
extern int top_of_mobt;
extern int top_of_objt;
extern int top_of_helpt;
 
extern int top_of_zone_table;
extern struct zone_data *zone_table;
extern struct player_special_data dummy_mob;  /* In db.c */
extern struct room_data *world;		      /* In db.c */ 
extern struct time_info_data time_info;	      /* In db.c */
extern struct index_data *mob_index;          /* In db.c */ 
extern struct index_data *obj_index;
extern char *help; 
extern const char *save_info_msg[];	      /* In olc.c */
extern const float race_exp_multipliers[]; 
extern const float class_exp_multipliers[]; 
extern const int exp_table[]; 
extern char  *wound_types[]; 
extern struct ban_list_element *ban_list; 
extern int num_invalid; 
extern char *GREETINGS; 
 
/* local globals */ 
long total_bytes_written=0;
int total_missed= 0;		/* Total pulses missed per record_usage*/
int times_missed= 0;
int missed_high = 0;
int missed_low  = 999;
char last_command[MAX_STRING_LENGTH];
struct descriptor_data *descriptor_list = NULL;	/* master desc list */ 
extern struct char_data *character_list;
int buf_largecount = 0;			      /* # of large buffers which exist */ 
int buf_overflows = 0;			      /* # of overflows of output */ 
int buf_switches = 0;			      /* # of switches from small to large buf */ 
int circle_shutdown = 0;		      /* clean shutdown */ 
int circle_reboot = 0;			      /* reboot the game after a shutdown */ 
int no_specials = 0;			      /* Suppress ass. of special routines */ 
int max_players = 0;			      /* max descriptors available */ 
int tics = 0;				      /* for extern checkpointing */ 
int scheck = 0;				      /* for syntax checking mode */ 
int dg_act_check;		              /* toggle for act_trigger */
unsigned long dg_global_pulse = 0;     /* number of pulses since game start */
bool MOBTrigger = TRUE;			      /*  */
extern int nameserver_is_slow;		      /* see config.c */ 
extern int auto_save;			      /* see config.c */ 
extern int autosave_time;		      /* see config.c */ 
struct timeval null_time;		      /* zero-valued time structure */ 
int port;				      /* port mud is running on */ 
#if defined(USE_CIRCLE_SOCKET_BUF)
struct txt_block *bufpool = 0;		      /*pool of large output buffers */
#endif
int pulse = 0;

/* functions in this file */ 
void setup_log(const char *filename, int fd);
int open_logfile(const char *filename, FILE *stderr_fp);
int  get_from_q(struct txt_q *queue, char *dest, int *aliased); 
void init_game(); 
void init_clan_list(void);
void signal_setup(void); 
void game_loop(int mother_desc); 
int  init_socket(); 
int  new_descriptor(int s); 
int  get_max_players(void); 
int  process_output(struct descriptor_data *t); 
int  process_input(struct descriptor_data *t); 
void close_socket(struct descriptor_data *d); 
void timediff(struct timeval *diff,struct timeval *a, struct timeval *b); 
void timeadd(struct timeval *sum,struct timeval *a,struct timeval *b);
void flush_queues(struct descriptor_data *d); 
void nonblock(socket_t s); 
int  perform_subst(struct descriptor_data *t, char *orig, char *subst); 
int  perform_alias(struct descriptor_data *d, char *orig); 
void record_usage(void); 
char *make_prompt(struct descriptor_data *point); 
void check_idle_passwords(void); 
void heartbeat(); 
void tick_grenade(void);
int  set_sendbuf(socket_t s);
void check_fishing(void);

 
/* extern fcnts */ 
void dam_test(void);
void check_buffer(char *data);
void reboot_wizlists(void); 
void boot_db(void); 
void boot_world(void); 
void zone_update(void); 
void affect_update(void); /* In spells.c */ 
void point_update(void); /* In limits.c */ 
void MEV_update(void);
void object_activity(void); 
void mobile_activity(void); 
void shop_activity(void);
void shop_housekeeping(void);
void perform_violence(void); 
void show_string(struct descriptor_data *d, char *input); 
void update_casting_time(struct char_data *ch);
int  isbanned(char *hostname); 
void weather_and_time(int mode); 
void redit_save_to_disk(int zone_num);
void oedit_save_to_disk(int zone_num);
void medit_save_to_disk(int zone_num);
void sedit_save_to_disk(int zone_num);
void zedit_save_to_disk(int zone_num);
void gedit_save_to_disk(int zone_num);
void hedit_save_to_disk(void);
int  real_zone(int rznum);
void mprog_act_trigger(char *buf, struct char_data *mob, struct char_data *ch,
		       struct obj_data *obj, void *vo);
void do_auction_update (void); 
void proc_color(char *inbuf, int color);
void write_last_command (void);
void vwrite_to_output(struct descriptor_data *t, const char *format, va_list args);
void player_shop_monthly_rent_check();


ACMD(do_infobar);                                 /* -naj infobar prototype */ 
char *scrpos(int y, int x, struct char_data *ch); /* -naj infobar prototype */ 

#ifdef __CXREF__
#undef FD_ZERO
#undef FD_SET
#undef FD_ISSET
#undef FD_CLR
#define FD_ZERO(x)
#define FD_SET(x, y) 0
#define FD_ISSET(x, y) 0
#define FD_CLR(x, y)
#endif
#define LAST_COMMAND_FILE  "../log/last_command"
 
 
/* ********************************************************************* 
*  main game loop and related stuff                                    * 
********************************************************************* */ 
 
 
int main(int argc, char **argv) 
{ 
   int pos = 1; 
   const char *dir = NULL; 

  /* Initialize dummy_mob variable. */
   memset(&dummy_mob, 0, sizeof(dummy_mob));

  /* default chdir moved here to make log()s work correctly. */
   if (chdir(DFLT_DIR) < 0)
      {
      perror("SYSERR: Fatal error changing to data directory");
      exit(1);
      }

  /* Start up the better buffer system. */
   init_buffers();
   dam_test();

   port = DFLT_PORT;

   while ((pos < argc) && (*(argv[pos]) == '-')) 
      { 
      switch (*(argv[pos] + 1)) 
	 { 
	  case 'a':
	     xap_objs = 1;
	     log("Loading player objects from ASCII files.");
	     break;
	  case 'c': 
	     scheck = 1; 
	     puts("Syntax check mode enabled."); 
	     break; 
	  case 'd': 
	     if (*(argv[pos] + 2)) 
		dir = argv[pos] + 2; 
	     else if (++pos < argc)
		{         
		dir = argv[pos]; 
		} 
	     else  
		{ 
		puts("SYSERR: Directory arg expected after option -d."); 
		exit(1); 
		} 
	     break; 
	  case 'h':
	    /* From: Anil Mahajan <amahajan@proxicom.com> */
	     printf("Usage: %s [-c] [-m] [-q] [-r] [-s] [-d pathname] [port #]\n"
		    "  -a             Enable ASCII obj files (Temporary).\n"
		    "  -c             Enable syntax check mode.\n"
		    "  -d <directory> Specify library directory (defaults to 'lib').\n"
		    "  -h             Print this command line argument help.\n"
		    "  -m             Start in mini-MUD mode.\n"
		    "  -o <file>      Write log to <file> instead of stderr.\n"
		    "  -q             Quick boot (doesn't scan rent for object limits)\n"
		    "  -r             Restrict MUD -- no new players allowed.\n"
		    "  -s             Suppress special procedure assignments.\n",
		    argv[0]
		);
	     exit(0);
	  case 'm': 
	     mini_mud = 1; 
	     no_rent_check = 1; 
	     puts("Running in minimized mode & with no rent check."); 
	     break; 
	  case 'o':
	     if (*(argv[pos] + 2))
		LOGNAME = argv[pos] + 2;
	     else if (++pos < argc)
		LOGNAME = argv[pos];
	     else 
		{
		puts("SYSERR: File name to log to expected after option -o.");
		exit(1);
		}
	     break;
	  case 'q': 
	     no_rent_check = 0; /* turning this off is stupid */
	     puts("Quick boot mode -- rent check supressed."); 
	     break; 
	  case 'r': 
	     circle_restrict = 1; 
	     puts("Restricting game -- no new players allowed."); 
	     break; 
	  case 's': 
	     no_specials = 1; 
	     puts("Suppressing assignment of special routines."); 
	     break; 
	  default: 
	     printf("SYSERR: Unknown option -%c in argument string.\n", *(argv[pos] + 1)); 
	     break; 
	 } 
      pos++; 
      } 
 
   if(xap_objs!=1)
      {
      xap_objs = 1;
      log("Loading player objects from ASCII files.");
      }

   if (pos < argc) 
      { 
      if (!isdigit((int)*argv[(int)pos])) 
	 { 
	 printf("Usage: %s [-c] [-m] [-q] [-r] [-s] [-d pathname] [port #]\n",
	     argv[0]); 
	 exit(1); 
	 } 
      else if ((port = atoi(argv[pos])) <= 1024) 
	 { 
	 printf("SYSERR: Illegal port number: %s\n",argv[pos]); 
	 exit(1); 
	 } 
      } 
  /* All arguments have been parsed, try to open log file. */
  /* Removed since new file logging setup in log() and mudlogf() */
   /*
   setup_log(LOGNAME, STDERR_FILENO);
   */

  /*
   * Moved here to distinguish command line options and to show up
   * in the log if stderr is redirected to a file.
   */
   log("\r\n\r\n%s",circlemud_version);

   if (dir)
      {
      if (chdir("../") < 0)  
         {
         perror("SYSERR: Fatal error changing to circle directory");
         exit(1);
         }
      if (chdir(dir) < 0) 
         { 
         perror("SYSERR: Fatal error changing to data directory"); 
         exit(1); 
         } 
      }
   else
      {
      dir = DFLT_DIR;
      }
   log("Using %s as data directory.", dir); 
 
   if (scheck) 
      { 
      boot_world(); 
      log("Done."); 
      exit(0); 
      } 
   else 
      { 
      log("Running game on port %d.", port); 
      init_game(); 
      } 
   exit_buffers();

   return (0); 
} 
 
 
 
/* Init sockets, run game, and cleanup sockets */ 
void init_game() 
{ 
   int mother_desc; 
 
  /* We don't want to restart if we crash before we get up. */
   touch(KILLSCRIPT_FILE);
   log("Touching KILLSCRIPT_FILE: %s to keep from repetively crashing.",
       KILLSCRIPT_FILE);
   log("If the mud is down, check for this file and fix the below error.");
   log("Then remove the file.");

   circle_srandom(time(0)); 
 
   log("Finding player limit."); 
   max_players = get_max_players(); 
 
   log("Opening mother connection."); 
   mother_desc = init_socket(); 

   log("Loading clan list.");
   init_clan_list();
   
   boot_db(); 
 
   log("Signal trapping."); 
   signal_setup(); 
 
  /* If we made it this far, we will be able to restart without problem */
   remove(KILLSCRIPT_FILE);
   log("Removing KILLSCRIPT_FILE: %s",KILLSCRIPT_FILE);

   log("Entering game loop."); 
 
   game_loop(mother_desc); 

   Crash_save_all();

   log("Closing all sockets."); 
   while (descriptor_list) 
      close_socket(descriptor_list); 
 
   CLOSE_SOCKET(mother_desc); 
   if (player_fl) {
     fclose(player_fl);
   }

   if (circle_reboot != 2 && olc_save_list) /* Don't save zones. */
      {
      struct olc_save_info *entry, *next_entry;
      int rznum;
      
      for (entry = olc_save_list; entry; entry = next_entry) 
	 {
	 next_entry = entry->next;
         if (entry->type == OLC_SAVE_HELP)
            {
            log("OLC: Reboot saving help entries.");
            hedit_save_to_disk();
            }
	 else if ((entry->type < 0) || (entry->type > 5))
	    {
	    log("OLC: Illegal save type %d!", entry->type);
	    }
	 else if ((rznum=real_zone(entry->zone*100))==-1)
	    {
	    log("OLC: Illegal save zone %d!", entry->zone);
	    }
	 else if (rznum < 0 || rznum > top_of_zone_table)
	    {
	    log("OLC: Invalid real zone number %d!", rznum);
	    }
	 else 
	    {
	    log("OLC: Reboot saving %s for zone %ld.",
		save_info_msg[(int)entry->type], 
		zone_table[rznum].number);
	    switch (entry->type)
	       {
		case OLC_SAVE_ROOM: 
		   redit_save_to_disk(rznum); 
		   break;
		case OLC_SAVE_OBJ:
		   oedit_save_to_disk(rznum); 
		   break;
		case OLC_SAVE_MOB:
		   medit_save_to_disk(rznum); 
		   break;
		case OLC_SAVE_ZONE:
		   zedit_save_to_disk(rznum);
		   break;
		case OLC_SAVE_SHOP: 
		   sedit_save_to_disk(rznum);
		   break;
                case OLC_SAVE_GM:
                   gedit_save_to_disk(rznum);
                   break;
		default:   
		   log("Unexpected olc_save_list->type: %d.",
                        entry->type); 
		   break;
	       }
	    }
	 }
      }
 
   
   if (circle_reboot) 
      { 
      log("Rebooting."); 
      exit(52);   /* what's so great about HHGTTG, anyhow? */ 
      } 
   log("Normal termination of game."); 
} 
 
 
 
/* 
 * init_socket sets up the mother descriptor - creates the socket, sets 
 * its options up, binds it, and listens. 
 */ 
int init_socket() 
{ 
   int s, opt; 
   struct sockaddr_in sa; 

  /* Clear the structure */
   memset((char *)&sa, 0, sizeof(sa));

 
   if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) 
      { 
      perror("Error creating socket"); 
      exit(1); 
      } 
 
#if defined(SO_REUSEADDR) 
   opt = 1; 
   if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt)) < 0) 
      { 
      perror("SYSERR: setsockopt REUSEADDR"); 
      exit(1); 
      } 
#endif 
 
   set_sendbuf(s);

#if defined(SO_LINGER) 
      { 
      struct linger ld; 
 
      ld.l_onoff = 0; 
      ld.l_linger = 0; 
      if (setsockopt(s, SOL_SOCKET, SO_LINGER, (char *) &ld, sizeof(ld)) < 0) 
	 { 
	 perror("SYSERR: setsockopt LINGER"); 
	 } 
      } 
#endif 
 
   sa.sin_family = AF_INET; 
   sa.sin_port = htons(port); 
   sa.sin_addr.s_addr = htonl(INADDR_ANY); 
 
   if (bind(s, (struct sockaddr *) &sa, sizeof(sa)) < 0) 
      { 
      perror("SYSERR: bind"); 
      CLOSE_SOCKET(s); 
      exit(1); 
      } 
   nonblock(s); 
   listen(s, 5); 
   return (s); 
} 
 
 
int get_max_players(void) 
{ 
   int max_descs = 0; 
   char *method; 

  /* 
   * First, we'll try using getrlimit/setrlimit.  This will probably work 
   * on most systems. 
   */ 
#if defined (RLIMIT_NOFILE) || defined (RLIMIT_OFILE) 
#if !defined(RLIMIT_NOFILE) 
#define RLIMIT_NOFILE RLIMIT_OFILE 
#endif 
      { 
      struct rlimit limit; 
 
     /* find the limit of file descs */ 
      method = "rlimit"; 
      if (getrlimit(RLIMIT_NOFILE, &limit) < 0) 
	 { 
	 perror("SYSERR: calling getrlimit"); 
	 exit(1); 
	 } 
     /* set the current to the maximum */ 
      limit.rlim_cur = limit.rlim_max; 
      if (setrlimit(RLIMIT_NOFILE, &limit) < 0) 
	 { 
	 perror("SYSERR: calling setrlimit"); 
	 exit(1); 
	 } 
#ifdef RLIM_INFINITY 
      if (limit.rlim_max == RLIM_INFINITY) 
	 max_descs = MAX_PLAYERS + NUM_RESERVED_DESCS; 
      else 
	 max_descs = MIN(MAX_PLAYERS + NUM_RESERVED_DESCS, limit.rlim_max); 
#else 
      max_descs = MIN(MAX_PLAYERS + NUM_RESERVED_DESCS, limit.rlim_max); 
#endif 
      } 
 
#elif defined (OPEN_MAX) || defined(FOPEN_MAX) 
#if !defined(OPEN_MAX) 
#define OPEN_MAX FOPEN_MAX 
#endif 
      method = "OPEN_MAX"; 
      max_descs = OPEN_MAX;  /* Uh oh.. rlimit didn't work, but we have 
			      * OPEN_MAX */ 
#elif defined (POSIX) 
     /* 
      * Okay, you don't have getrlimit() and you don't have OPEN_MAX.  Time to 
      * use the POSIX sysconf() function.  (See Stevens' _Advanced Programming 
      * in the UNIX Environment_). 
      */ 
      method = "POSIX sysconf"; 
      errno = 0; 
      if ((max_descs = sysconf(_SC_OPEN_MAX)) < 0) 
	 { 
	 if (errno == 0) 
	    max_descs = MAX_PLAYERS + NUM_RESERVED_DESCS; 
	 else 
	    { 
	    perror("SYSERR: Error calling sysconf"); 
	    exit(1); 
	    } 
	 } 
#else 
     /* if everything has failed, we'll just take a guess */ 
      method = "random guess";
      max_descs = MAX_PLAYERS + NUM_RESERVED_DESCS; 
#endif 
 
     /* now calculate max _players_ based on max descs */ 
      max_descs = MIN(MAX_PLAYERS, max_descs - NUM_RESERVED_DESCS); 
 
      if (max_descs <= 0) 
	 { 
	 log("SYSERR: Non-positive max player limit!  (Set at %d using %s).", 
		 max_descs, method); 
	 exit(1); 
	 } 
      log("   Setting player limit to %d using %s.", max_descs, method); 
      return (max_descs); 
} 
 
 
 
/* 
 * game_loop contains the main loop which drives the entire MUD.  It 
 * cycles once every 0.10 seconds and is responsible for accepting new 
 * new connections, polling existing connections for input, dequeueing 
 * output and sending it out to players, and calling "heartbeat" functions 
 * such as mobile_activity(). 
 */ 
void game_loop(int mother_desc) 
{ 
   fd_set input_set, output_set, exc_set, null_set; 
   struct timeval last_time, opt_time, process_time, temp_time;
   struct timeval before_sleep, now, timeout;
   char comm[MAX_INPUT_LENGTH+10]; 
   struct descriptor_data *d, *next_d; 
   int missed_pulses, maxdesc, aliased; 
   int time_to_tic;
 
  /* initialize various time values */ 
   null_time.tv_sec = 0; 
   null_time.tv_usec = 0; 
   opt_time.tv_usec = OPT_USEC; 
   opt_time.tv_sec = 0; 
   FD_ZERO(&null_set); 
 
   gettimeofday(&last_time, (struct timezone *) 0); 

   time_to_tic=number(50,100);
   add_function_to_queue(time_to_tic,NULL,0,0,MEV_update);
   log("Time to next tic: %d",time_to_tic);

  /* The Main Loop.  The Big Cheese.  The Top Dog.  The Head Honcho.  The.. */ 
   while (!circle_shutdown) 
      { 
 
     /* Sleep if we don't have any connections */ 
      if (descriptor_list == NULL) 
	 { 
	 log("No connections.  Going to sleep."); 
	 FD_ZERO(&input_set); 
	 FD_SET(mother_desc, &input_set); 
	 if (select(mother_desc + 1,&input_set,(fd_set *)0,(fd_set *)0,NULL)<0)
	    { 
	    if (errno == EINTR) 
	       log("Waking up to process signal."); 
	    else 
	       perror("SYSERR: Select coma"); 
	    } 
	 else 
	    log("New connection.  Waking up."); 
	 gettimeofday(&last_time, (struct timezone *) 0); 
	 } 
     /* Set up the input, output, and exception sets for select(). */ 
      FD_ZERO(&input_set); 
      FD_ZERO(&output_set); 
      FD_ZERO(&exc_set); 
      FD_SET(mother_desc, &input_set); 
 
      maxdesc = mother_desc; 
      for (d = descriptor_list; d; d = d->next) 
	 { 
#ifndef CIRCLE_WINDOWS 
	 if (d->descriptor > maxdesc) 
	    maxdesc = d->descriptor; 
#endif 
	 FD_SET(d->descriptor, &input_set); 
	 FD_SET(d->descriptor, &output_set); 
	 FD_SET(d->descriptor, &exc_set); 
	 } 
 
     /* 
      * At this point, we have completed all input, output and heartbeat 
      * activity from the previous iteration, so we have to put ourselves 
      * to sleep until the next 0.1 second tick.  The first step is to 
      * calculate how long we took processing the previous iteration. 
      */ 
     
      gettimeofday(&before_sleep, (struct timezone *) 0); /* current time */ 
      timediff(&process_time,&before_sleep, &last_time); 
 
     /* 
      * If we were asleep for more than one pass, count missed pulses and sleep
      * until we're resynchronized with the next upcoming pulse. 
      */ 
      if (process_time.tv_sec == 0 && process_time.tv_usec < OPT_USEC) 
	 { 
	 missed_pulses = 0; 
	 } 
      else 
	 { 
	 missed_pulses = process_time.tv_sec * PASSES_PER_SEC; 
	 missed_pulses += process_time.tv_usec / OPT_USEC; 
	 process_time.tv_sec = 0; 
	 process_time.tv_usec = process_time.tv_usec % OPT_USEC; 
	 } 
 
     /* Calculate the time we should wake up */ 
      timediff(&temp_time, &opt_time, &process_time);
      timeadd(&last_time, &before_sleep, &temp_time);

     /* Now keep sleeping until that time has come */ 
      gettimeofday(&now, (struct timezone *) 0); 
      timediff(&timeout, &last_time, &now); 
     /* Go to sleep */ 
      do 
	 { 
	 if (select(0, (fd_set *) 0, (fd_set *) 0, (fd_set *) 0, &timeout) < 0)
	    { 
	    if (errno != EINTR) 
	       { 
	       perror("SYSERR: Select sleep"); 
	       exit(1); 
	       } 
	    } 
	 gettimeofday(&now, (struct timezone *) 0); 
	 timediff(&timeout, &last_time, &now); 
	 } 
      while (timeout.tv_usec || timeout.tv_sec); 
 
     /* Poll (without blocking) for new input, output, and exceptions */ 
      if (select(maxdesc + 1, &input_set, &output_set, &exc_set, &null_time)<0)
	 { 
	 perror("SYSERR: Select poll"); 
	 return; 
	 } 
     /* If there are new connections waiting, accept them. */ 
      if (FD_ISSET(mother_desc, &input_set)) 
	 new_descriptor(mother_desc); 
 
     /* Kick out the freaky folks in the exception set */ 
      for (d = descriptor_list; d; d = next_d) 
	 { 
	 next_d = d->next; 
	 if (FD_ISSET(d->descriptor, &exc_set)) 
	    { 
	    log("Kicking out a freaky folk");
	    FD_CLR(d->descriptor, &input_set); 
	    FD_CLR(d->descriptor, &output_set); 
	    close_socket(d); 
	    } 
	 } 
 
     /* Process descriptors with input pending */ 
      for (d = descriptor_list; d; d = next_d) 
	 { 
	 next_d = d->next; 
	 if (FD_ISSET(d->descriptor, &input_set)) 
	    if (process_input(d) < 0) 
	       {
	       log("Killing a desc with input pending.");
	       close_socket(d); 
	       }
	 } 
 
     /* Process descriptors with ident pending */ 
      for (d = descriptor_list; d; d = next_d) 
	 { 
	 next_d = d->next; 
 
	 if (waiting_for_ident(d)) 
	    ident_check(d); 
	 } 
 
     /* Process commands we just read from process_input */ 
      for (d = descriptor_list; d; d = next_d) 
	 { 
	 next_d = d->next; 
	 if(waiting_for_ident(d))
	    continue;

	 if (d->character)
	    { 
	    GET_WAIT_STATE(d->character) -= (GET_WAIT_STATE(d->character) > 0);
	    if(GET_WAIT_STATE(d->character)>0)
	       {
	       d->speed_buffer -= (d->speed_buffer > 0);
	       continue;
	       }

            /* don't accept commands if they are stunned */
            if (GET_STUN_STATE(d->character)>0)
               {
               if (FIGHTING(d->character))
                  continue;
               else
                  GET_STUN_STATE(d->character) = 0;
               }

	   /* go around again if casting */
	    if(IS_CASTING(d->character) == TRUE)
	       {
	       d->speed_buffer -= (d->speed_buffer > 0);
	       continue;
	       }
	    }
	 if(d->speed_wait > 0)
	    {
	    d->speed_wait--;
	    continue;
	    }
	 
	 if(!get_from_q(&d->input, comm, &aliased))
	    {
	    d->speed_buffer -= (d->speed_buffer > 0);
	    continue;
	    }
	 d->has_prompt = 0; 
	 if(STATE(d)==CON_PLAYING)
	    {
	    if(d->speed_buffer>9)
	       {
	       d->speed_wait = d->speed_buffer;
	       }
            /* moved this to do_move() for anti-speedwalking */
/*	    if(!IS_NPC(d->character) && 
	       !PRF_FLAGGED(d->character,PRF_NOHASSLE))
	       d->speed_buffer += (d->speed_buffer <10);
*/
/*	    log("Speed Buffer %s: %d, Speed Wait %d",GET_NAME(d->character),
                d->speed_buffer, d->speed_wait);
*/	    
	    }

	 if (d->character)
	    { 
	   /* Reset the idle timer & pull char back from void if necessary*/
	    d->character->char_specials.timer = 0; 
	    if (STATE(d)==CON_PLAYING &&
		GET_WAS_IN(d->character) != NOWHERE) 
	       { 
	       if (IN_ROOM(d->character) != NOWHERE) 
		  char_from_room(d->character); 
	       char_to_room(d->character, GET_WAS_IN(d->character)); 
	       GET_WAS_IN(d->character) = NOWHERE; 
	       act("$n has returned.", TRUE, d->character, 0, 0, TO_ROOM); 
	       } 
	    GET_WAIT_STATE(d->character) = 1; 
	    } 
	 
	/* reversed these top 2 if checks so that you can use the 
	 * page_string  function in the editor 
	 */ 
	 if (d->showstr_count) /* reading something w/ pager     */ 
	    show_string(d, comm); 
	 else if (d->str)  /* writing boards, mail, etc.     */ 
	    string_add(d, comm); 
	 else if (STATE(d) != CON_PLAYING) /* in menus, etc. */ 
	    nanny(d, comm); 
	 else 
	    {   
	   /* else: we're playing normally */ 
	    if (aliased)  /* to prevent recursive aliases */ 
	       d->has_prompt = 1; 
	    else if (perform_alias(d, comm))  /* run it through aliasing system */ 
	       get_from_q(&d->input, comm, &aliased); 
	    command_interpreter(d->character, comm); /* send it to interpreter */ 
	    } 
	 }
      
     /* -naj infobar2 12/16/96 - update the infobar 
      * To prevent trailing spaces in the output window, the update the
      * bar here. 
      */ 
     
/*&      for(d = descriptor_list; d; d = d->next) 
	 if (STATE(d)==CON_PLAYING) 
	    if (d->character) 
	       do_infobar(d->character, 0, 0, SCMDB_GENUPDATE); 
	       */  
     /* send queued output out to the operating system (ultimately to user) */ 
      for (d = descriptor_list; d; d = next_d) 
	 { 
	 next_d = d->next; 
	 if (*(d->output)&&FD_ISSET(d->descriptor, &output_set))
	    {
	    if (process_output(d) < 0) 
	       close_socket(d); 
	    else 
	       d->has_prompt = 1; 
	    }
	 } 
 
  /* Print prompts for other descriptors who had no other output */
      for (d = descriptor_list; d; d = d->next) 
	 {
	 if (!d->has_prompt) 
	    {
	    write_to_descriptor(d->descriptor, make_prompt(d));
	    d->has_prompt = 1;
	    }
	 }
      
     /* kick out folks in the CON_CLOSE or CON_DISCONNECT state */ 
      for (d = descriptor_list; d; d = next_d) 
	 { 
	 next_d = d->next; 
	 if ((STATE(d) == CON_CLOSE) || (STATE(d)==CON_DISCONNECT))
	 {
	    close_socket(d); 
	 }
	 } 
 

      if (pulse % 1000 == 0) {
	player_shop_monthly_rent_check();
      }


 
     /* 
      * Now, we execute as many pulses as necessary--just one if we haven't 
      * missed any pulses, or make up for lost time if we missed a few 
      * pulses by sleeping for too long. 
      */ 
      missed_pulses++; 
 
      if (missed_pulses <= 0) 
	 { 
	 log("SYSERR: **BAD** MISSED_PULSES IS NONPOSITIVE! (%d)",
	     missed_pulses); 
	 missed_pulses = 1; 
	 } 
 
     /* If we missed more than 30 seconds worth of pulses, forget it */ 
      if (missed_pulses > (30 * PASSES_PER_SEC)) 
	 { 
	 log("SYSERR: Warning: Missed more than 30 seconds worth of pulses. (%d)", missed_pulses/PASSES_PER_SEC); 
	 missed_pulses = 30 * PASSES_PER_SEC; 
	 } 
 
      if(missed_pulses>1)
	 {
	 total_missed+=(missed_pulses-1);
	 if((missed_pulses-1)>missed_high)
	    missed_high=missed_pulses-1;
	 if((missed_pulses-1)<missed_low)
	    missed_low=missed_pulses-1;
	 times_missed++;
	 }
     /* Now execute the heartbeat functions */ 
      while (missed_pulses--) 
	 { 
	/* 
	 * for event driven code in queue.* 
	 */ 
	 pulse++;
	 heartbeat(); 
	 process_event_queue(); 
	 } 
 
     /* Roll pulse over after 10 hours */ 
      if (pulse >= (600 * 60 * PASSES_PER_SEC)) 
	 pulse = 0; 
 
     /* Update tics for deadlock protection (UNIX only) */ 
      tics++; 
      } 
} 
 
 
void heartbeat() 
{ 
   struct char_data *i;
   static int mins_since_crashsave = 0; 
 
  /* Clear out all the global buffers now in case someone forgot. */

   if (!(pulse % PULSE_BUFFER))
      release_all_buffers();

   dg_global_pulse++;
   
   if (!(pulse % PULSE_DG_SCRIPT))
      script_trigger_check();
   
   if (!((pulse % PULSE_ZONE))) 
      zone_update(); 
 
   if (!(pulse % (15 * PASSES_PER_SEC)))  /* 15 seconds */ 
      check_idle_passwords(); 

   if (!(pulse % PULSE_MOBILE)) 
      mobile_activity(); 

   if (!(pulse % PULSE_OBJECT))
      object_activity();

   if (!(pulse % PULSE_VIOLENCE))
      perform_violence(); 

/* disabled temporarily due to memory usage - will imp less intensive version */
/*   if (!(pulse % PASSES_PER_SEC))
      tick_grenade();
*/

   /* Changed skill practice lag from one practice per tic to     **
   ** two practices per tic -Nomikos 5/22/2025                    */
   if (!(pulse % ((SECS_PER_MUD_HOUR / 2) * PASSES_PER_SEC)))
      for (i = character_list; i; i = i->next)
	     if (!IS_NPC(i))
	        GET_LEARN_TIC(i) = 0;
	
   if(!(pulse%PULSE_MAGIC))
      for (i = character_list; i; i = i->next)  /* put into event queue */
	 if (IS_CASTING(i) == TRUE)
	    update_casting_time(i);
   
   if (!(pulse % (5 * PASSES_PER_SEC)))
      check_fishing();

   if(!(pulse % PASSES_PER_SEC))
      time_info.minutes++;

   if (!(pulse % (SECS_PER_MUD_HOUR * PASSES_PER_SEC))) 
      { 
      weather_and_time(1); 
      affect_update(); 
      point_update(); 
      fflush(player_fl); 
      } 

  /* give the shop cash once per RL minute */
   if(!(pulse % (60 * PASSES_PER_SEC)))
      {
      shop_activity();
      }
  /* clean out the shop inventory one per 30 RL minutes */
   if(!(pulse % (30 * 60 * PASSES_PER_SEC)))
      {
      shop_housekeeping();
      }

   if (auto_save && !(pulse % (60 * PASSES_PER_SEC))) 
      { 
     /* 1 minute */ 
      if (++mins_since_crashsave >= autosave_time) 
	 { 
	 mins_since_crashsave = 0; 
	 Crash_save_all(); 
	 House_save_all(); 
	 } 
      } 

   /* Back up the index files every 12 hours.  With 6 backup files, this gives
    * us 3 days to catch and repair the index if it gets corrupted. */
   if (!(pulse % (8 * 60 * 60 * PASSES_PER_SEC))) {
     log("Backing up player index.");
     system("/bin/cp etc/players_ascii/index.5 etc/players_ascii/index.6");
     system("/bin/cp etc/players_ascii/index.4 etc/players_ascii/index.5");
     system("/bin/cp etc/players_ascii/index.3 etc/players_ascii/index.4");
     system("/bin/cp etc/players_ascii/index.2 etc/players_ascii/index.3");
     system("/bin/cp etc/players_ascii/index.1 etc/players_ascii/index.2");
     system("/bin/cp etc/players_ascii/index etc/players_ascii/index.1");
   }

   if (!(pulse % (5 * 60 * PASSES_PER_SEC))) /* 5 minutes */ 
      record_usage(); 
} 
 
 
/* ****************************************************************** 
*  general utility stuff (for local use)                            * 
****************************************************************** */ 
 
/* 
 *  new code to calculate time differences, which works on systems 
 *  for which tv_usec is unsigned (and thus comparisons for something 
 *  being < 0 fail).  Based on code submitted by ss@sirocco.cup.hp.com. 
 */ 
 
/* 
 * code to return the time difference between a and b (a-b). 
 * always returns a nonnegative value (floors at 0). 
 */ 
void timediff(struct timeval *rslt,struct timeval *a, struct timeval *b) 
{ 
  if (a->tv_sec < b->tv_sec)
    *rslt = null_time;
  else if (a->tv_sec == b->tv_sec) {
    if (a->tv_usec < b->tv_usec)
      *rslt = null_time;
    else {
      rslt->tv_sec = 0;
      rslt->tv_usec = a->tv_usec - b->tv_usec;
    }
  } else {                      /* a->tv_sec > b->tv_sec */
    rslt->tv_sec = a->tv_sec - b->tv_sec;
    if (a->tv_usec < b->tv_usec) {
      rslt->tv_usec = a->tv_usec + 1000000 - b->tv_usec;
      rslt->tv_sec--;
    } else
      rslt->tv_usec = a->tv_usec - b->tv_usec;
  }
}
/*
 * Add 2 time values.
 *
 * Patch sent by "d. hall" <dhall@OOI.NET> to fix 'static' usage.
 */
void timeadd(struct timeval *rslt, struct timeval *a, struct timeval *b)
{
  rslt->tv_sec = a->tv_sec + b->tv_sec;
  rslt->tv_usec = a->tv_usec + b->tv_usec;

  while (rslt->tv_usec >= 1000000) {
    rslt->tv_usec -= 1000000;
    rslt->tv_sec++;
  }
}
 
void record_usage(void) 
{ 
   int sockets_connected = 0, sockets_playing = 0; 
   struct descriptor_data *d; 
 
   for (d = descriptor_list; d; d = d->next) 
      { 
      sockets_connected++; 
      if (STATE(d)==CON_PLAYING)
	 sockets_playing++; 
      } 
 
   log("nusage: %-3d sck conn, %-3d sck ply, Tot missed pul: %d Tot misses: %d,  %f / %d / %d (Ave/H/L)", 
       sockets_connected, 
       sockets_playing,
       total_missed,
       times_missed,
       times_missed?((float)((float)total_missed/(float)times_missed)):0.0,
       missed_high, missed_low==999?0:missed_low); 
   total_missed=0;
   times_missed=0;
   missed_high=0;
   missed_low=999;
#ifdef RUSAGE /* Not RUSAGE_SELF because it doesn't guarantee prototype. */
      { 
      struct rusage ru; 
 
      getrusage(RUSAGE_SELF, &ru); 
      log("rusage: user time: %ld sec, system time: %ld sec, max res size: %ld", 
	      ru.ru_utime.tv_sec, ru.ru_stime.tv_sec, ru.ru_maxrss); 
      } 
#endif 
} 
 
 
 
/* 
 * Turn off echoing (specific to telnet client) 
 */ 
void echo_off(struct descriptor_data *d) 
{ 
   char off_string[] =
   {
      (char)IAC,
      (char)WILL, 
      (char)TELOPT_ECHO, 
      (char)0, 
   }; 
 
   SEND_TO_Q(d, "%s", off_string); 
} 
 
 
/* 
 * Turn on echoing (specific to telnet client) 
 */ 
void echo_on(struct descriptor_data *d) 
{ 
   char on_string[] = 
   { 
      (char) IAC, 
      (char) WONT, 
      (char) TELOPT_ECHO, 
      (char) TELOPT_NAOFFD, 
      (char) TELOPT_NAOCRD, 
      (char) 0, 
   }
   ; 
 
   SEND_TO_Q(d, "%s", on_string); 
} 
 
 
char *make_prompt(struct descriptor_data *d) 
{ 
   static char prompt[MAX_PROMPT_LENGTH+1];
   static char color[10];
   long hit1=0,hit2=0,percent=0; 
   int length=0;
   /* Note, prompt is truncated at MAX_PROMPT_LENGTH chars (structs.h )*/

   /* reversed these top 2 if checks so that page_string() would work in */ 
   /* the editor */ 
   if (d->showstr_count) 
   { 
      length=sprintf(prompt, "\r\n[ Return to continue, (q)uit, (r)efresh, (b)ack, or page number (%d/%d) ]", d->showstr_page, d->showstr_count); 
   } 
   else if (d->str) 
      strcpy(prompt, "] "); 
   else if ((STATE(d)==CON_PLAYING)&&!IS_NPC(d->character))
      { 
      *prompt = '\0'; 
 
      if (GET_INVIS_LEV(d->character)) 
	     length+=sprintf(prompt, "i%d ", GET_INVIS_LEV(d->character));

      if (PRF2_FLAGGED(d->character, PRF2_DISPMAX)) 
	  {  
	     if (PRF_FLAGGED(d->character, PRF_DISPHP)) 
	        length+=sprintf(prompt+length, "%d/%dH ", GET_HIT(d->character), 
		       GET_MAX_HIT(d->character)); 
 
	    if (PRF_FLAGGED(d->character, PRF_DISPMANA)) 
	       length+=sprintf(prompt+length, "%d/%dM ",GET_MANA(d->character), 
		       GET_MAX_MANA(d->character)); 
 
	    if (PRF_FLAGGED(d->character, PRF_DISPMOVE)) 
	       length+=sprintf(prompt+length, "%d/%dV ",GET_MOVE(d->character),
		       GET_MAX_MOVE(d->character)); 
	  }
      else
	  { 
	    if (PRF_FLAGGED(d->character, PRF_DISPHP)) 
	       length+=sprintf(prompt+length, "%dH ", GET_HIT(d->character)); 
 
	    if (PRF_FLAGGED(d->character, PRF_DISPMANA)) 
	       length+=sprintf(prompt+length, "%dM ", GET_MANA(d->character)); 
 
	    if (PRF_FLAGGED(d->character, PRF_DISPMOVE)) 
	       length+=sprintf(prompt+length, "%dV ", GET_MOVE(d->character)); 
	 } 
      
     if (PRF2_FLAGGED(d->character, PRF2_DISPALIGN)&&!FIGHTING(d->character)) 
	     length+=sprintf(prompt+length, "%dA ", GET_ALIGNMENT(d->character)); 
 
      if (PRF2_FLAGGED(d->character, PRF2_DISPGOLD)&&!FIGHTING(d->character)) 
	     length+=sprintf(prompt+length, "%dG ",(int) GET_GOLD(d->character)); 
 
      if (PRF2_FLAGGED(d->character, PRF2_DISPEXP)) 
	     length+=sprintf(prompt+length, "%dX ", 
		     (int) (GET_EXP_FOR_CH(d->character)
		    	- GET_EXP(d->character))); 

      if (PRF2_FLAGGED(d->character, PRF2_DISPEXPLORED) &&
		 IN_ROOM(d->character) >= 0 && IN_ROOM(d->character) < EXPLORED_TOP_VNUM) 
      {
		 struct zone_data *zone = &zone_table[world[IN_ROOM(d->character)].zone];
		 int znum = zone->number;

         int num_explored = 0;
		 int rnum;
         for (rnum = 100 * znum; rnum < 100 * (znum + 1); rnum++) {
			int b = d->character->player_specials->explored_vnums[rnum / 8];
			if (b & (1 << (rnum % 8)))
			   num_explored++;
		    }

		 length+=sprintf(prompt+length, "%d/%dR ", zone->name,
			             num_explored, zone->num_rooms);
      }

      if (PRF2_FLAGGED(d->character, PRF2_DISPTIME))
      {
         char *disptime = get_buffer(64);
         time_t mytime = time(0);
         strftime(disptime, 20, "%H:%M:%S EST", localtime(&mytime)); 

         length+=sprintf(prompt+length, "(%s) ", disptime);
         release_buffer(disptime);
      } 

      if (PRF2_FLAGGED(d->character, PRF2_DISPDATE)) 
      {
         char *dispdate = get_buffer(64);
         time_t mytime = time(0);
         strftime(dispdate, 20, "%a %b %d, %Y", localtime(&mytime));
         
         length+=sprintf(prompt+length, "%s ", dispdate);
         release_buffer(dispdate);
      }

      /*** Standard fighting prompts Anduin ****/ 
      if(d->character->char_specials.fighting) 
	  { 
	  if (GET_MAX_HIT(d->character) > 0) 
	     { 
	     hit2=GET_MAX_HIT(d->character); 
	     hit1=GET_HIT(d->character); 
	     percent = (int)(10 * ((float)hit1 / (float)hit2))+1; 
	     }
	  else 
	     percent=0;/*if MAX_HIT is < 0 (HUH?!?!?!) */ 
 
	  if(percent<0) percent = 0; 
	  if(percent>11) percent= 11;
	  switch(percent)
	  {
	     case 0:
		    strcpy(color,CCBLU(d->character,C_NRM));
		    break;
	     case 1:
	     case 2:
		    strcpy(color,CCRED(d->character,C_NRM));
		    break;
	     case 3:
	     case 4:
		    strcpy(color,CCMAG(d->character,C_NRM));
		    break;
	     case 5:
	     case 6:
		    strcpy(color,CCYEL(d->character,C_NRM));
		    break;
	     case 7:
	     case 8:
		    strcpy(color,CCCYN(d->character,C_NRM));
		    break;
	     default:
		    color[0]='\0';
		    break;
	 }
	 length+=sprintf(prompt+length,"[You: %s%s%s]",color,
			 wound_types[percent],CCNRM(d->character,C_NRM)); 
              
	 if((d->character->char_specials.fighting->char_specials.fighting)&&\
	    (d->character->char_specials.fighting->char_specials.fighting\
	     != d->character)) 
	    {                        
	    if (GET_MAX_HIT(d->character->char_specials.fighting->\
			    char_specials.fighting) > 0) 
	       { 
	       hit2=GET_MAX_HIT(d->character->char_specials.fighting->\
				char_specials.fighting); 
	       hit1=GET_HIT(d->character->char_specials.fighting->\
			    char_specials.fighting); 
	       percent = (int)(10 * ((float)hit1 / (float)hit2))+1; 
	       } 
	    else 
	       percent=0;/* if MAX_HIT is less then 0 (HUH?!?!) */ 
 
	    if(percent<0) percent = 0; 
	    if(percent>11) percent= 11;
	    switch(percent)
	       {
		case 0:
		   strcpy(color,CCBLU(d->character,C_NRM));
		   break;
		case 1:
		case 2:
		   strcpy(color,CCRED(d->character,C_NRM));
		   break;
		case 3:
		case 4:
		   strcpy(color,CCMAG(d->character,C_NRM));
		   break;
		case 5:
		case 6:
		   strcpy(color,CCYEL(d->character,C_NRM));
		   break;
		case 7:
		case 8:
		   strcpy(color,CCCYN(d->character,C_NRM));
		   break;
		default:
		   color[0]='\0';
		   break;
	       }
	    length+=sprintf(prompt+length," [Tank: %s%s%s]",color,
			    wound_types[percent],CCNRM(d->character,C_NRM));
	    } 
	 if (GET_MAX_HIT(d->character->char_specials.fighting) > 0) 
	    { 
	    hit2=GET_MAX_HIT(d->character->char_specials.fighting); 
	    hit1=GET_HIT(d->character->char_specials.fighting); 
	    percent = (int)(10 * ((float)hit1 / (float)hit2))+1; 
	    } 
	 else 
	    percent=0;/* if MAX_HIT is less then 0 (HUH?!?!) */ 
	 if(percent<0) percent = 0; 
	 if(percent>11) percent= 11; 
	 switch(percent)
	    {
	     case 0:
		strcpy(color,CCBLU(d->character,C_NRM));
		break;
	     case 1:
	     case 2:
		strcpy(color,CCRED(d->character,C_NRM));
		break;
	     case 3:
	     case 4:
		strcpy(color,CCMAG(d->character,C_NRM));
		break;
	     case 5:
	     case 6:
		strcpy(color,CCYEL(d->character,C_NRM));
		break;
	     case 7:
	     case 8:
		strcpy(color,CCCYN(d->character,C_NRM));
		break;
	     default:
		color[0]='\0';
		break;
	    }
	 length+=sprintf(prompt+length," [Enemy: %s%s%s]",color,
			 wound_types[percent],CCNRM(d->character,C_NRM)); 
	 } 
      strcat(prompt, "> "); 
      } 
   else if(STATE(d)==CON_PLAYING&&IS_NPC(d->character))	/* switched prompt */
      {
      length=sprintf(prompt,"%s ",GET_NAME(d->character));
      length+=sprintf(prompt+length, "%d/%dH ", GET_HIT(d->character), 
		      GET_MAX_HIT(d->character)); 
      
      length+=sprintf(prompt+length, "%d/%dM ",GET_MANA(d->character), 
		      GET_MAX_MANA(d->character)); 
      strcat(prompt, "> "); 
      }
   else
      *prompt='\0';
   if(strlen(prompt) > MAX_PROMPT_LENGTH-4)
      {
      mudlogf(CMP,LVL_IMMORT,TRUE,"%s's prompt is HUGE: %s",
	      GET_NAME(d->character),prompt);
      }
   return (prompt);
} 
 
 
void write_to_q_d(char *txt, struct descriptor_data *d, int aliased)
{ 
   struct txt_block *newt; 

   /* If you are a player, and you type a command starting with "gs", or "gre", or "oo",
    * do not queue the command, but instead, interpret it immediately, regardless
    * of skill lag.  This lets you gsay and greport even while lagged in a group,
    * but not any other command. (and ooc now, too!  --Modred)
    */
   if (d && d->character && STATE(d)==CON_PLAYING) {
     struct char_data *ch = d->character;
     if (
	 d->speed_buffer < 1
	 && (
	     starts_with(txt, "gs") 
	     || starts_with(txt, "gre") 
	     || starts_with(txt, "gt")
	     || starts_with(txt, "oo")
	     || starts_with(txt, "flush")
	     )
	 ) {
       d->speed_buffer += 2;
       command_interpreter(ch, txt);
       return;
     }
   }

   struct txt_q *input = &d->input;
 
   CREATE(newt, struct txt_block, 1); 
   newt->text=str_dup(txt); 
   newt->aliased = aliased; 
 
  /* queue empty? */ 
   if (!input->head) 
      { 
      newt->next = NULL; 
      input->head = input->tail = newt; 
      } 
   else 
      { 
      input->tail->next = newt; 
      input->tail = newt; 
      newt->next = NULL; 
      } 
} 

void write_to_q(char *txt, struct txt_q *input, int aliased)
{ 
   struct txt_block *newt; 

   CREATE(newt, struct txt_block, 1); 
   newt->text=str_dup(txt); 
   newt->aliased = aliased; 
 
  /* queue empty? */ 
   if (!input->head) 
      { 
      newt->next = NULL; 
      input->head = input->tail = newt; 
      } 
   else 
      { 
      input->tail->next = newt; 
      input->tail = newt; 
      newt->next = NULL; 
      } 
} 
 
 
 
int get_from_q(struct txt_q *queue, char *dest, int *aliased) 
{ 
   struct txt_block *tmp; 
 
  /* queue empty? */ 
   if (!queue->head) 
      return (0);
 
   strcpy(dest, queue->head->text); 
   *aliased = queue->head->aliased; 

   tmp = queue->head; 
   queue->head = queue->head->next; 
   free(tmp->text); 
   free(tmp); 
 
   return (1);
} 
 
 
 
/* Empty the queues before closing connection */ 
void flush_queues(struct descriptor_data *d) 
{ 
   
  /*
   * As I understand this, it puts the buffer back on the list.
   * So we don't need this anymore as it is.
   */
   
   if (d->large_outbuf) 
      { 
      release_buffer(d->large_outbuf);
      d->large_outbuf=NULL;
      d->output=d->small_outbuf;
      } 
   while (d->input.head) 
      {
      struct txt_block *tmp = d->input.head;
      d->input.head = d->input.head->next;
      free(tmp->text);
      free(tmp);
      }
} 
 
 
/* Add a new string to a player's output queue */ 
void write_to_output(struct descriptor_data *t, const char *format, ...) 
{ 
   va_list args;
   
   va_start(args, format);
   vwrite_to_output(t, format, args);
   va_end(args);
}


/* Add a new string to a player's output queue. For internal use. */
void vwrite_to_output(struct descriptor_data *t, const char *format, va_list args)
{
   static char txt[MAX_STRING_LENGTH];
   static int max_size = 0;
   int size; 
   char *buf;

   vsprintf(txt, format, args);
   size = strlen(txt); 
   buf=get_buffer(size + 1024);
   strcpy(buf,txt);

   if((STATE(t)==CON_PLAYING) && t->character)
      proc_color(buf, (clr(t->character, C_NRM)));
   size=strlen(buf);
   if(size>max_size)
      {
      max_size=size;
      }

   if(t->large_outbuf&&( t->bufptr<SMALL_BUFSIZE)&&(t->bufptr!=-1))
      {
      log("SYSERR: have a large but why: %d/%d %s",strlen(t->output),t->bufptr, last_command);
      }
      
   if(!t->output)
      {
      t->output=t->small_outbuf;
      t->bufspace=SMALL_BUFSIZE-1;
      t->bufptr=0;
      log("Where the hell is t->output [%s][%s]",t->small_outbuf,
	  t->large_outbuf);
      *(t->output)= '\0';
      }
   else if((t->output!=t->small_outbuf)&&(t->output!=t->large_outbuf))
      {
      if (t->large_outbuf)
	 { 
	 release_buffer(t->large_outbuf);
	 t->large_outbuf=NULL;
	 }
      t->output=t->small_outbuf;
      t->bufspace=SMALL_BUFSIZE-1;
      t->bufptr=0;
      log("Where the hell is t->output pointing at [%s][%s]",t->small_outbuf,
	  t->large_outbuf);
      *(t->output)= '\0';
      }

  /* if we're in the overflow state already, ignore this new output */ 
   if (t->bufptr < 0) 
      {
      release_buffer(buf);
      return; 
      }

  /* if we have enough space, just write to buffer and that's it! */ 
   if (t->bufspace >= size) 
      { 
      strcpy(t->output + t->bufptr, buf); 
      t->bufspace -= size; 
      t->bufptr += size; 
      release_buffer(buf);
      return; 
      } 
  /* 
   * If we're already using the large buffer, or if even the large buffer 
   * is too small to handle this new text, chuck the text and switch to the 
   * overflow state. 
   */ 
   if((size + t->bufptr) > (LARGE_BUFSIZE-1))
      { 
      t->bufptr = -1; 
      buf_overflows++; 
      log("BUF: OverFlow: %s",last_command);
/*      log("BUF: %s",t->output); */
/*      log("BUF: %s",buf); */
      release_buffer(buf);
      return; 
      }

   if (t->large_outbuf)
      { 
      show_buffers(0,-1,0);
      release_buffer(t->large_outbuf);
      t->large_outbuf=NULL;
      t->output=t->small_outbuf;
      t->bufspace=SMALL_BUFSIZE-1;
      *(t->output)= '\0';
      t->bufptr = 0; 
      mudlogf(CMP,LVL_IMMORT,TRUE,"SYSERR: Have a LARGE and shouldn't: %s", last_command);
      buf_overflows++;
      release_buffer(buf);
      return; 
      } 
   buf_switches++; 
 
  /*
   * Just request the buffer. Copy the contents of the old, and make it
   * the primary buffer.
   */
/*  log("**** Get large buff: %s",last_command?last_command:"No Command!"); */
   t->large_outbuf = get_buffer(LARGE_BUFSIZE);
   strcpy(t->large_outbuf, t->output);
   t->output = t->large_outbuf;
   strcat(t->output, buf);
 
  /* set the pointer for the next write */ 
   t->bufptr = strlen(t->output); 

  /* calculate how much space is left in the buffer */ 
   t->bufspace = LARGE_BUFSIZE - 1 - t->bufptr; 
   release_buffer(buf);
} 
 
 
 
/* ****************************************************************** 
*  socket handling                                                  * 
****************************************************************** */ 
 
/* Sets the kernel's send buffer size for the descriptor */
int set_sendbuf(socket_t s)
{
#if defined(SO_SNDBUF)
   int opt = MAX_SOCK_BUF;
   
   if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *) &opt, sizeof(opt)) < 0) 
      {
      perror("SYSERR: setsockopt SNDBUF");
      return -1;
      }
   
#if 0
   if (setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char *) &opt, sizeof(opt)) < 0) 
      {
      perror("SYSERR: setsockopt RCVBUF");
      return -1;
      }
#endif
#endif
   
   return 0;
}


int new_descriptor(int s) 
{ 
   socket_t desc; 
   int sockets_connected = 0; 
   unsigned long addr; 
   static int last_desc = 0; /* last descriptor number */ 
   struct descriptor_data *newd; 
   struct sockaddr_in peer; 
   struct hostent *from; 
   char *buf2;
 
  /* accept the new connection */ 
   size_t peersize = sizeof(peer); 
   if ((desc = accept(s, (struct sockaddr *) &peer, &peersize)) == INVALID_SOCKET) 
      { 
      perror("SYSERR: accept"); 
      return -1; 
      } 
  /* keep it from blocking */ 
   nonblock(desc); 

  /* set the send buffer size if available on the system */
#if defined (SO_SNDBUF)
   if (set_sendbuf(desc) < 0) 
      {
      CLOSE_SOCKET(desc);
      return 0;
      }
#endif

  /* make sure we have room for it */ 
   for (newd = descriptor_list; newd; newd = newd->next) 
      { 
      sockets_connected++; 
      if (newd->ident_sock != -1) 
	 sockets_connected++; 
      } 
 
   if (sockets_connected >= max_players) 
      { 
      write_to_descriptor(desc, "Sorry, CircleMUD is full right now... please try again later!\r\n"); 
      CLOSE_SOCKET(desc); 
      return 0; 
      } 
  /* create a new descriptor */ 
   CREATE(newd, struct descriptor_data, 1); 
   memset((char *) newd, 0, sizeof(struct descriptor_data)); 
 
  /* find the sitename */ 
   if (nameserver_is_slow || !(from = gethostbyaddr((char *) &peer.sin_addr, 
						    sizeof(peer.sin_addr), AF_INET))) 
      { 
 
     /* resolution failed */ 
      if (!nameserver_is_slow) 
	 perror("SYSERR: gethostbyaddr failed"); 
 
     /* find the numeric site address */ 
      addr = ntohl(peer.sin_addr.s_addr); 
      sprintf(newd->host, "%03u.%03u.%03u.%03u", (int) ((addr&0xFF000000)>>24),
	      (int) ((addr & 0x00FF0000) >> 16), (int) ((addr&0x0000FF00)>>8),
	      (int) ((addr & 0x000000FF))); 
      } 
   else 
      { 
      strncpy(newd->host, from->h_name, HOST_LENGTH); 
      *(newd->host + HOST_LENGTH) = '\0'; 
      } 
 
  /* determine if the site is banned */ 
   if (isbanned(newd->host) == BAN_ALL) 
      { 
      buf2 = get_buffer(128);
      CLOSE_SOCKET(desc); 
      sprintf(buf2, "Connection attempt denied from [%s]", newd->host); 
      mudlog(buf2, CMP, LVL_DGOD, TRUE); 
      free(newd); 
      release_buffer(buf2);
      return 0; 
      } 
#if 0 
  /* Log new connections - probably unnecessary, but you may want it */ 
   buf2 = get_buffer(128);
   sprintf(buf2, "New connection from [%s]", newd->host); 
   mudlog(buf2, CMP, LVL_IMPL, FALSE); 
   release_buffer(buf2);
#endif 
 
  /* initialize descriptor data */ 
   newd->descriptor = desc; 
   STATE(newd) = CON_GET_NAME; 
   newd->peer_port = peer.sin_port; 
   newd->idle_tics = 0; 
   newd->output = newd->small_outbuf; 
   newd->bufspace = SMALL_BUFSIZE - 1; 
   newd->login_time = time(0); 
   *newd->output='\0';
   newd->bufptr=0;
   newd->has_prompt=1;		/* prompt is part of greetings */
   newd->speed_buffer=0;
   newd->speed_wait=0;

  /*
   * This isn't exactly optimal but allows us to make a design choice.
   * Do we embed the history in descriptor_data or keep it dynamically
   * allocated and allow a user defined history size?
   */
   CREATE(newd->history, char *, HISTORY_SIZE);



   if (++last_desc == 1000) 
      last_desc = 1; 
   newd->desc_num = last_desc; 
 
  /* prepend to list */ 
   newd->next = descriptor_list; 
   descriptor_list = newd; 
   SEND_TO_Q(newd, "%c%c%c", IAC, WILL, MSSP);
   if(port!=4999)
      SEND_TO_Q(newd, "%s", GREETINGS); 
   SEND_TO_Q(newd,"Please wait"); 
   ident_start(newd, peer.sin_addr.s_addr); 
 
   return 0; 
} 
 
 
/*
 * Send all of the output that we've accumulated for a player out to
 * the player's descriptor.
 * FIXME - This will be rewritten before 3.1, this code is dumb.
 */
int process_output(struct descriptor_data *t) 
{ 
   char *i;
   int result; 
   
   i=get_buffer(MAX_SOCK_BUF); 

  /* we may need this \r\n for later -- see below */ 
   strcpy(i, "\r\n"); 
 
  /* now, append the 'real' output */ 
   strcpy(i + 2, t->output); 
 
  /* if we're in the overflow state, notify the user */ 
   if (t->bufptr < 0)
      strcat(i, "**OVERFLOW** Almost lost ya.\r\n"); 
 
  /* add the extra CRLF if the person isn't in compact mode */ 
   if (STATE(t)==CON_PLAYING && t->character && !IS_NPC(t->character) &&
       !PRF_FLAGGED(t->character,PRF_COMPACT))
      {
      if (!PRF_FLAGGED(t->character,PRF_INFOBAR))  /* -naj infobar2 12/16/96 - make sure extra spacing goes to window */ 
	 strcat(i + 2, "\r\n"); 
      else 
      strcat(i, "\e8\r\n\e7");
	 //sprintf(i,"%s\e8\r\n\e7",i);  /* -naj infobar2 -output lines.*/
      }

  /* -naj infobar2 12/16/96 - curser back to input line */ 
   if (STATE(t)==CON_PLAYING && t->character) 
      if (!IS_NPC(t->character)&&PRF_FLAGGED(t->character,PRF_INFOBAR)) 
	 strcat(i,scrpos(24, 1, t->character));  
	 //sprintf(i,"%s%s",i,scrpos(24, 1, t->character));  
  /* strcat(i, scrpos(24, 1, t->character)); */  
   
  /* add a prompt */
   strncat(i + 2, make_prompt(t), MAX_PROMPT_LENGTH);

  /* 
   * now, send the output.  If this is an 'interruption', use the prepended 
   * CRLF, otherwise send the straight output sans CRLF. 
   */ 
   if (t->has_prompt)
      result = write_to_descriptor(t->descriptor, i); 
   else 
      result = write_to_descriptor(t->descriptor, i + 2); 
 
  /* handle snooping: prepend "% " and send to snooper */ 
   if (t->snoop_by) 
      { 
      SEND_TO_Q(t->snoop_by, "%% %s%%%%", t->output);
      } 
  /* 
   * if we were using a large buffer, put the large buffer on the buffer pool 
   * and switch back to the small one 
   */ 
   if (t->large_outbuf) 
      { 
      release_buffer(t->large_outbuf);
      t->output = t->small_outbuf;
      t->large_outbuf=NULL;
      } 
  /* reset total bufspace back to that of a small buffer */ 
   t->bufspace = SMALL_BUFSIZE - 1; 
   t->bufptr = 0; 
   *(t->output) = '\0'; 

   release_buffer(i);
   return result; 
} 
 
 
 
int write_to_descriptor(socket_t desc, const char *txt) 
{ 
   int total, bytes_written; 
 
   total = strlen(txt); 
 
   do 
      { 
      if ((bytes_written = write(desc, txt, total)) < 0) 
	 { 
#ifdef EWOULDBLOCK 
	 if (errno == EWOULDBLOCK) 
	    errno = EAGAIN; 
#endif /* EWOULDBLOCK */ 
	 if (errno == EAGAIN) 
	    log("WARNING: write_to_desciptor: socket write would block, about to close"); 
	 else 
	    perror("SYSERR: Write to socket"); 
	 return -1; 
	 } 
      else 
	 { 
	 txt += bytes_written; 
	 total -= bytes_written; 
	 total_bytes_written+=bytes_written;
	 } 
      } 
   while (total > 0); 
 
   return 0; 
} 

void handle_iac(struct descriptor_data *d) { 

   for (unsigned char* ptr = d->inbuf; *ptr != 0; ptr++) {
      if (*ptr == IAC) {
         unsigned char cmd = *(ptr + 1);

         if (!cmd) break;

         unsigned char opt = *(ptr + 2);

         if (!opt) break;

         switch (cmd) {
            case DO:
               if (opt == MSSP) {
                  SEND_TO_Q(d, "%c%c%c", IAC, SB, MSSP);
                  SEND_TO_Q(d, "%cNAME%cPhoenixMud", MSSP_VAR, MSSP_VAL);

                  int players = 0;

                  for (struct char_data* vict = character_list; vict != 0; vict = vict->next) {
                     if (IS_NPC(vict)) continue;
                     if (vict->desc) players++;
                  }

                  SEND_TO_Q(d, "%cPLAYERS%c%d", MSSP_VAR, MSSP_VAL, players);
                  SEND_TO_Q(d, "%cUPTIME%c%ld", MSSP_VAR, MSSP_VAL, boot_time);

                  SEND_TO_Q(d, "%cCREATED%c1996", MSSP_VAR, MSSP_VAL);
                  SEND_TO_Q(d, "%cDISCORD%chttps://discord.gg/dUE3Nm2rEE", MSSP_VAR, MSSP_VAL);
                  SEND_TO_Q(d, "%cHOSTNAME%cphoenixmud.net", MSSP_VAR, MSSP_VAL);
                  SEND_TO_Q(d, "%cPORT%c4000", MSSP_VAR, MSSP_VAL);
                  SEND_TO_Q(d, "%cWEBSITE%chttps://phoenixmud.net", MSSP_VAR, MSSP_VAL);

                  SEND_TO_Q(d, "%cFAMILY%cPhoenixMUD%cCircleMUD%cDikuMUD%cAberMUD", MSSP_VAR, MSSP_VAL, MSSP_VAL, MSSP_VAL, MSSP_VAL);
                  SEND_TO_Q(d, "%cGENRE%cFantasy", MSSP_VAR, MSSP_VAL);
                  SEND_TO_Q(d, "%cSTATUS%cLive", MSSP_VAR, MSSP_VAL);

                  SEND_TO_Q(d, "%cAREAS%c%d", MSSP_VAR, MSSP_VAL, top_of_zone_table + 1);
                  SEND_TO_Q(d, "%cHELPFILES%c%d", MSSP_VAR, MSSP_VAL, top_of_helpt + 1);
                  SEND_TO_Q(d, "%cMOBILES%c%d", MSSP_VAR, MSSP_VAL, top_of_mobt + 1);
                  SEND_TO_Q(d, "%cOBJECTS%c%d", MSSP_VAR, MSSP_VAL, top_of_objt + 1);
                  SEND_TO_Q(d, "%cROOMS%c%ld", MSSP_VAR, MSSP_VAL, top_of_world + 1);

                  SEND_TO_Q(d, "%cCLASSES%c15", MSSP_VAR, MSSP_VAL);
                  SEND_TO_Q(d, "%cLEVELS%c%d", MSSP_VAR, MSSP_VAL, 101+102+103+104);
                  SEND_TO_Q(d, "%cRACES%c15", MSSP_VAR, MSSP_VAL);
                  SEND_TO_Q(d, "%cSKILLS%c%d", MSSP_VAR, MSSP_VAL, MAX_SPELLS);

                  SEND_TO_Q(d, "%cANSI%c1", MSSP_VAR, MSSP_VAL);

                  SEND_TO_Q(d, "%c%c", IAC, SE);
               }

               // We processed three bytes: IAC, DO, opt. Overwrite those three with everything after them.
               // Move the pointer back one to account for the increment in the for loop.
               memmove(ptr, ptr + 3, strlen(ptr + 3) + 1);
               ptr--;
               break;
            case DONT:

               memmove(ptr, ptr + 3, strlen(ptr + 3) + 1);
               ptr--;
               break;
            case WILL:

               memmove(ptr, ptr + 3, strlen(ptr + 3) + 1);
               ptr--;
               break;
            case WONT:

               memmove(ptr, ptr + 3, strlen(ptr + 3) + 1);
               ptr--;
               break;
            case SB:
               // Read until IAC SE
               break;
            default:
               break;
         }
      }
   }

}
 
/* 
 * ASSUMPTION: There will be no newlines in the raw input buffer when this 
 * function is called.  We must maintain that before returning. 
 */
int process_input(struct descriptor_data *t) {
   int failed_subst;
   char *write_point;
   char *tmp;

   /* first, find the point where we left off reading data */
   int buf_length = strlen(t->inbuf);
   char *read_point = t->inbuf + buf_length;
   int space_left = MAX_RAW_INPUT_LENGTH - buf_length - 1;

   if (space_left <= 0) {
      log("WARNING: process_input: about to close connection: input overflow");
      return -1;
   }

   int bytes_read = read(t->descriptor, read_point, space_left);

   if (bytes_read < 0) {
      if (errno == EWOULDBLOCK) errno = EAGAIN;

      if ((errno != EAGAIN) && (errno != EINTR)) {
         perror("SYSERR: process_input: about to lose connection");
         return -1; /* some error condition was encountered on
                     * read */
      } else {
         return 0; /* the read would have blocked: just means no
                     * data there but everything's okay */
      }
   } else if (bytes_read == 0) {
      log("WARNING: EOF on socket read (connection broken by peer)");
      return -1;
   }
   /* at this point, we know we got some data from the read */

   *(read_point + bytes_read) = '\0'; /* terminate the string */

   handle_iac(t); /* check for IAC commands */

   /* search for a newline in the data we just read */

   char* nl_pos = NULL;
   for (char* ptr = read_point; *ptr && !nl_pos; ptr++) {
      if (ISNEWL(*ptr))
      {
         nl_pos = ptr;
      }
   }

   read_point += bytes_read;
   space_left -= bytes_read;

   if (nl_pos == NULL) {
      return 0;
   }

   /*
    * okay, at this point we have at least one newline in the string; now we
    * can copy the formatted data to a new array for further processing.
    */

   read_point = t->inbuf;

   tmp = get_buffer(MAX_INPUT_LENGTH + 8);
   while (nl_pos != NULL) {
      write_point = tmp;
      space_left = MAX_INPUT_LENGTH - 1;

      char* ptr = NULL;

      for (ptr = read_point; (space_left > 0) && (ptr < nl_pos); ptr++) {
         if ((*ptr == '\b') || (*ptr == 127)) {
            /* handle backspacing */
            if (write_point > tmp)
            {
               if (*(--write_point) == '$')
               {
                  write_point--;
                  space_left += 2;
               }
               else
                  space_left++;
            }
         } else if (isascii(*ptr) && isprint((int)*ptr)) {
            if ((*(write_point++) = *ptr) == '$') {
               /* copy one character */
               *(write_point++) = '$'; /* if it's a $, double it */
               space_left -= 2;
            }
            else {
               space_left--;
            }
         }
      }

      *write_point = '\0';

      if ((space_left <= 0) && (ptr < nl_pos)) {
         char *bufferout = get_buffer(MAX_INPUT_LENGTH + 64);

         sprintf(bufferout, "Line too long.  Truncated to:\r\n%s\r\n", tmp);
         if (write_to_descriptor(t->descriptor, bufferout) < 0) {
            release_buffer(tmp);
            release_buffer(bufferout);
            return -1;
         }
         release_buffer(bufferout);
      }
      if (t->snoop_by) {
         SEND_TO_Q(t->snoop_by, "%% %s\r\n", tmp);
      }
      failed_subst = 0;

      if ((*tmp == '!') && !(*(tmp + 1)))  {/* redo last command */
         strcpy(tmp, t->last_input);
      } else if (*tmp == '!' && *(tmp + 1)) {
         char *commandln = (tmp + 1);
         int starting_pos = t->history_pos;
         int cnt = (t->history_pos == 0 ? HISTORY_SIZE - 1 : t->history_pos - 1);

         skip_spaces(&commandln);
         for (; cnt != starting_pos; cnt--) {
            if (t->history[cnt] && is_abbrev(commandln, t->history[cnt])) {
               strcpy(tmp, t->history[cnt]);
               strcpy(t->last_input, tmp);
               SEND_TO_Q(t, "%s\r\n", tmp);
               break;
            }
            if (cnt == 0) { /* At top, loop to bottom. */
               cnt = HISTORY_SIZE;
            }
         }
      } else if (*tmp == '^') {
         if (!(failed_subst = perform_subst(t, t->last_input, tmp))) {
            strcpy(t->last_input, tmp);
         }
      } else {
         strcpy(t->last_input, tmp);
         if (t->history[t->history_pos])
            free(t->history[t->history_pos]);       /* Clear the old line. */
         t->history[t->history_pos] = str_dup(tmp); /* Save the new. */
         if (++t->history_pos >= HISTORY_SIZE)      /* Wrap to top. */
            t->history_pos = 0;
      }

      if (!failed_subst)
         write_to_q_d(tmp, t, 0);

      /* find the end of this line */
      while (ISNEWL(*nl_pos))
         nl_pos++;

      /* see if there's another newline in the input buffer */
      read_point = ptr = nl_pos;
      for (nl_pos = NULL; *ptr && !nl_pos; ptr++)
         if (ISNEWL(*ptr))
            nl_pos = ptr;
   }

   /* now move the rest of the buffer up to the beginning for the next pass */
   write_point = t->inbuf;
   while (*read_point)
      *(write_point++) = *(read_point++);
   *write_point = '\0';

   release_buffer(tmp);
   return 1;
}

/* 
 * perform substitution for the '^..^' csh-esque syntax 
 * orig is the orig string (i.e. the one being modified. 
 * subst contains the substition string, i.e. "^telm^tell" 
 */ 
int perform_subst(struct descriptor_data *t, char *orig, char *subst) 
{ 
   char *newsub = get_buffer(MAX_INPUT_LENGTH + 5);
 
   char *first, *second, *strpos; 
 
  /* 
   * first is the position of the beginning of the first string (the one 
   * to be replaced 
   */ 
   first = subst + 1; 
 
  /* now find the second '^' */ 
   if (!(second = strchr(first, '^'))) 
      { 
      SEND_TO_Q(t,"Invalid substitution.\r\n"); 
      release_buffer(newsub);
      return 1; 
      } 
  /* terminate "first" at the position of the '^' and make 'second' point 
   * to the beginning of the second string */ 
   *(second++) = '\0'; 
 
  /* now, see if the contents of the first string appear in the original */ 
   if (!(strpos = strstr(orig, first))) 
      { 
      SEND_TO_Q(t,"Invalid substitution.\r\n"); 
      release_buffer(newsub);
      return 1; 
      } 
  /* now, we construct the new string for output. */ 
 
  /* first, everything in the original, up to the string to be replaced */ 
   strncpy(newsub, orig, (strpos - orig)); 
   newsub[(strpos - orig)] = '\0'; 
 
  /* now, the replacement string */ 
   strncat(newsub, second, (MAX_INPUT_LENGTH - strlen(newsub) - 1)); 
 
  /* now, if there's anything left in the original after the string to 
   * replaced, copy that too. */ 
   if (((strpos - orig) + strlen(first)) < strlen(orig)) 
      strncat(newsub,strpos+strlen(first),(MAX_INPUT_LENGTH-strlen(newsub)-1));
 
  /* terminate the string in case of an overflow from strncat */ 
   newsub[MAX_INPUT_LENGTH - 1] = '\0'; 
   strcpy(subst, newsub); 
 
   release_buffer(newsub);
   return 0; 
} 
 
 
 
void close_socket(struct descriptor_data *d) 
{ 
   char *buf = get_buffer(128);
   struct descriptor_data *temp; 


   REMOVE_FROM_LIST(d,descriptor_list,next);
   CLOSE_SOCKET(d->descriptor); 
   flush_queues(d); 
 
   if (d->ident_sock != -1) 
      CLOSE_SOCKET(d->ident_sock); 
 
  /* Forget snooping */ 
   if (d->snooping) 
      d->snooping->snoop_by = NULL; 
 
   if (d->snoop_by) 
      { 
      SEND_TO_Q(d->snoop_by,"Your victim is no longer among us.\r\n"); 
      d->snoop_by->snooping = NULL; 
      } 
 
  /*. Kill any OLC stuff .*/ 
   switch(STATE(d)) 
      { 
       case CON_OEDIT: 
       case CON_REDIT: 
       case CON_ZEDIT: 
       case CON_MEDIT: 
       case CON_SEDIT: 
       case CON_GEDIT: 
       case CON_PEDIT: 
       case CON_TRIGEDIT:
	  cleanup_olc(d, CLEANUP_ALL); 
       default: 
	  break; 
      } 
 
   if (d->character) 
      { 
     /* 
      * Plug memory leak, from Eric Green
      */
      if(PLR_FLAGGED(d->character,PLR_MAILING) && d->str)
	 {
	 if(*(d->str))
	    free(*(d->str));
	 free(d->str);
	 }

      if ((STATE(d) == CON_PLAYING) || (STATE(d)==CON_DISCONNECT))
	 { 
	 if (IN_ROOM(d->character) != NOWHERE)
            act("$n has lost $s link.", TRUE, d->character, 0, 0, TO_ROOM); 
	 if(!IS_NPC(d->character))
	    {
	    save_char(d->character,NOWHERE);
	    sprintf(buf, "Closing link to: %s in room %ld.", GET_NAME(d->character),
       GET_ROOM_VNUM(IN_ROOM(d->character))); 
	    mudlog(buf, NRM, MAX(LVL_IMMORT,GET_INVIS_LEV(d->character)),TRUE);
	    }
	 d->character->desc = NULL; 
	 } 
      else 
	 { 
	 sprintf(buf, "Losing player: %s.", 
		 GET_NAME(d->character) ? GET_NAME(d->character) : "<null>"); 
	 mudlog(buf, CMP, MAX(LVL_IMMORT,GET_INVIS_LEV(d->character)),TRUE);
    if (IN_ROOM(d->character) != NOWHERE){
      char_from_room(d->character);
    }
	 free_char(d->character); 
	 } 
      } 
   else 
      mudlogf(CMP, LVL_IMMORT, TRUE, "Losing descriptor [%s] without char. ", d->host); 
 
  /* JE 2/22/95 -- part of my unending quest to make switch stable */ 
   if (d->original && d->original->desc) 
      d->original->desc = NULL; 
 
  /* Clear the command history. */
   if (d->history) 
      {
      int cnt;
      for (cnt = 0; cnt < HISTORY_SIZE; cnt++)
	 if (d->history[cnt])
	    free(d->history[cnt]);
      free(d->history);
      }
   
 
   if (d->showstr_head) 
      free(d->showstr_head); 
   if (d->showstr_count) 
      free(d->showstr_vector); 
   if (d->storage)
      free(d->storage);
   free(d); 
   release_buffer(buf);
} 
 
 
 
void check_idle_passwords(void) 
{ 
   struct descriptor_data *d, *next_d; 
 
   for (d = descriptor_list; d; d = next_d) 
      { 
      next_d = d->next; 
      if ((STATE(d) != CON_PASSWORD) && (STATE(d) != CON_GET_NAME) &&
	  (STATE(d) != CON_RMOTD) && (STATE(d) != CON_MENU))
	 continue; 
      if ((d->idle_tics==2) &&((STATE(d) == CON_PASSWORD) ||
			       (STATE(d) == CON_GET_NAME)))
	 { 
	 echo_on(d); 
	 SEND_TO_Q(d,"\r\nTimed out... goodbye.\r\n"); 
	 STATE(d) = CON_CLOSE; 
	 } 
      else if ((d->idle_tics>=6))
	 { 
	 echo_on(d); 
	 SEND_TO_Q(d,"\r\nTimed out... goodbye.\r\n"); 
	 STATE(d) = CON_CLOSE; 
	 } 
      d->idle_tics++; 
      } 
} 
 
 
 
/* 
 * I tried to universally convert Circle over to POSIX compliance, but 
 * alas, some systems are still straggling behind and don't have all the 
 * appropriate defines.  In particular, NeXT 2.x defines O_NDELAY but not 
 * O_NONBLOCK.  Krusty old NeXT machines!  (Thanks to Michael Jones for 
 * this and various other NeXT fixes.) 
 */ 
 
#ifdef CIRCLE_WINDOWS 
 
void nonblock(socket_t s) 
{ 
   unsigned long val =1; 
   ioctlsocket(s, FIONBIO, &val); 
} 
 
#else 
 
#ifndef O_NONBLOCK 
#define O_NONBLOCK O_NDELAY 
#endif 
 
void nonblock(socket_t s) 
{ 
   int flags; 
 
   flags = fcntl(s, F_GETFL, 0); 
   flags |= O_NONBLOCK; 
   if (fcntl(s, F_SETFL, flags) < 0) 
      { 
      perror("SYSERR: Fatal error executing nonblock (comm.c)"); 
      exit(1); 
      } 
} 
 
 
/* ****************************************************************** 
*  signal-handling functions (formerly signals.c)                   * 
****************************************************************** */ 
 
 
RETSIGTYPE checkpointing(int sig) 
{ 
   if (!tics) 
      { 
      write_last_command(); 
      log("SYSERR: CHECKPOINT shutdown: tics not updated (Infinite Loop Suspected)"); 
      abort(); 
      } 
   else 
      tics = 0; 
} 
 
 
RETSIGTYPE reread_wizlists(int sig) 
{ 
 
   mudlog("Signal received - rereading wizlists.", CMP, LVL_IMMORT, TRUE); 
   reboot_wizlists(); 
} 
 
 
RETSIGTYPE unrestrict_game(int sig) 
{ 
 
   mudlog("Received SIGUSR2 - completely unrestricting game (emergent)", 
	  BRF, LVL_IMMORT, TRUE); 
   ban_list = NULL; 
   circle_restrict = 0; 
} 
 
 
RETSIGTYPE hupsig(int sig)
{ 
   write_last_command(); 
   log("SYSERR: Received %d.  Shutting down...with core(i hope)",sig); 
/*   exit(1); */  /* perhaps something more elegant should 
	       * substituted */ 
   abort();
} 
RETSIGTYPE ctrlc_signal(int sig)
{ 
   write_last_command(); 
   log("SYSERR: SIGINT.  Shutting down..."); 
   exit(1); 

} 

RETSIGTYPE nasty_signal_handler (int sig)
{
   hupsig(sig);
   return;
}

 
/* 
 * This is an implementation of signal() using sigaction() for portability. 
 * (sigaction() is POSIX; signal() is not.)  Taken from Stevens' _Advanced 
 * Programming in the UNIX Environment_.  We are specifying that all system 
 * calls _not_ be automatically restarted for uniformity, because BSD systems 
 * do not restart select(), even if SA_RESTART is used. 
 * 
 * Note that NeXT 2.x is not POSIX and does not have sigaction; therefore, 
 * I just define it to be the old signal.  If your system doesn't have 
 * sigaction either, you can use the same fix. 
 * 
 * SunOS Release 4.0.2 (sun386) needs this too, according to Tim Aldric. 
 */ 
 
#ifndef POSIX 
#define my_signal(signo, func) signal(signo, func) 
#else 
sigfunc *my_signal(int signo, sigfunc * func) 
{ 
   struct sigaction sig_act, oact; 
 
   sig_act.sa_handler = func; 
   sigemptyset(&sig_act.sa_mask); 
   sig_act.sa_flags = 0; 
#ifdef SA_INTERRUPT 
   sig_act.sa_flags |= SA_INTERRUPT; /* SunOS */ 
#endif 
 
   if (sigaction(signo, &sig_act, &oact) < 0) 
      return SIG_ERR; 
 
   return oact.sa_handler; 
} 
#endif    /* NeXT */ 
 
 
void signal_setup(void) 
{ 
   struct itimerval itime; 
   struct timeval interval; 
 
  /* user signal 1: reread wizlists.  Used by autowiz system. */ 
   my_signal(SIGUSR1, reread_wizlists); 
 
  /* 
   * user signal 2: unrestrict game.  Used for emergencies if you lock 
   * yourself out of the MUD somehow.  (Duh...) 
   */ 
   my_signal(SIGUSR2, unrestrict_game); 
 
  /* 
   * set up the deadlock-protection so that the MUD aborts itself if it gets 
   * caught in an infinite loop for more than 3 minutes. 
   */ 
   if(DODGER_DEBUG==1)
      interval.tv_sec = 15; 
   else
      interval.tv_sec = 180; 

   interval.tv_usec = 0; 
   itime.it_interval = interval; 
   itime.it_value = interval; 
   setitimer(ITIMER_VIRTUAL, &itime, NULL); 
   my_signal(SIGVTALRM, checkpointing); 
 
  /* just to be on the safe side: */ 
   my_signal(SIGHUP, nasty_signal_handler); 
   my_signal(SIGTERM, nasty_signal_handler); 
   my_signal(SIGINT, ctrlc_signal); 
   my_signal(SIGPIPE, SIG_IGN); 
   my_signal(SIGALRM, SIG_IGN); 
 
   my_signal(SIGFPE,nasty_signal_handler);
/*    my_signal(SIGABRT,nasty_signal_handler); */
/*    my_signal(SIGILL,nasty_signal_handler); */
/*    my_signal(SIGSEGV,nasty_signal_handler);  */
#ifdef CIRCLE_OS2 
#if defined(SIGABRT) 
   my_signal(SIGABRT, hupsig); 
#endif 
#if defined(SIGFPE) 
   my_signal(SIGFPE, hupsig); 
#endif 
#if defined(SIGILL) 
   my_signal(SIGILL, hupsig); 
#endif 
#if defined(SIGSEGV) 
   my_signal(SIGSEGV, hupsig); 
#endif 
#endif    /* CIRCLE_OS2 */ 
 
} 
 
#endif    /* CIRCLE_WINDOWS */ 
 
 
/* **************************************************************** 
*       Public routines for system-to-player-communication        * 
**************************************************************** */ 
 
void send_to_char(struct char_data *ch, const char *messg, ...) 
{ 
  if (ch->desc && messg && *messg) 
     {
     va_list args;
     
     va_start(args, messg);
     vwrite_to_output(ch->desc, messg, args);
     va_end(args);
     }
} 
 
 
void send_to_all(const char *messg, ...) 
{ 
   struct descriptor_data *i; 
   va_list args;
 
   va_start(args, messg);
   for (i = descriptor_list; i; i = i->next)
      if ((STATE(i)==CON_PLAYING) && !PLR_FLAGGED(i->character,PLR_WRITING))
	 vwrite_to_output(i, messg, args);
   va_end(args);
} 
 
 
void send_to_outdoor(const char *messg, ...) 
{ 
   struct descriptor_data *i; 
   va_list args;

   if (!messg || !*messg) 
      return; 

   va_start(args, messg);
   for (i = descriptor_list; i; i = i->next) 
      {
      if (STATE(i)!=CON_PLAYING || (i->character==NULL))
	 continue;
      if(!AWAKE(i->character) || !OUTSIDE(i->character))
	 continue;
      if(PLR_FLAGGED(i->character,PLR_WRITING))
	 continue;
      vwrite_to_output(i, messg, args);
      }
   va_end(args);
} 
 
/*
void send_to_daylight(char *messg) 
{ 
   struct descriptor_data *i; 
 
   if (!messg || !*messg) 
      return; 
 
   for (i = descriptor_list; i; i = i->next) 
      if (STATE(i)==CON_PLAYING && 
	  i->character && 
	  AWAKE(i->character) && 
	  (OUTSIDE(i->character) ||
	   ROOM_FLAGGED(IN_ROOM(i->character),ROOM_DAYLIGHT)) &&
	  !PLR_FLAGGED(i->character,PLR_WRITING)) 
	 SEND_TO_Q(i,messg); 
} 
 
*/ 
 
void send_to_room(room_rnum room, const char *messg, ...) 
{ 
   struct char_data *i; 
   va_list args;

   if(messg==NULL)
      return;

   va_start(args, messg);

   for (i = world[room].people; i; i = i->next_in_room) 
      if ((i->desc) && (STATE(i->desc)==CON_PLAYING) &&
	  !PLR_FLAGGED(i,PLR_WRITING))
	 vwrite_to_output(i->desc, messg, args);

   va_end(args);
} 
 
 
 
char *ACTNULL = "<NULL>"; 
 
#define CHECK_NULL(pointer, expression) \
if ((pointer) == NULL) i = ACTNULL; else i = (expression); 
 
 
/* higher-level communication: the act() function */ 
void perform_act(char *orig, struct char_data *ch, struct obj_data *obj, 
		  void *vict_obj, struct char_data *to) 
{ 
   register char *i = NULL, *buf; 
   char *lbuf = get_buffer(MAX_STRING_LENGTH);
   struct char_data *dg_victim = NULL;
   struct obj_data *dg_target = NULL; 
   char *dg_arg = NULL;

 
   buf = lbuf; 
 
   for (;;) 
      { 
      if (*orig == '$') 
	 { 
	 switch (*(++orig)) 
	    { 
	     case 'n': 
		i = PERS(ch, to); 
		break; 
	     case 'N': 
		CHECK_NULL(vict_obj, PERS((const struct char_data *) vict_obj, to)); 
		dg_victim = (struct char_data *) vict_obj;
		break; 
	     case 'm': 
		i = HMHR(ch); 
		break; 
	     case 'M': 
		CHECK_NULL(vict_obj, HMHR((const struct char_data *) vict_obj)); 
		dg_victim = (struct char_data *) vict_obj;
		break; 
	     case 's': 
		i = HSHR(ch); 
		break; 
	     case 'S': 
		CHECK_NULL(vict_obj, HSHR((const struct char_data *) vict_obj)); 
		dg_victim = (struct char_data *) vict_obj;
		break; 
	     case 'e': 
		i = HSSH(ch); 
		break; 
	     case 'E': 
		CHECK_NULL(vict_obj, HSSH((const struct char_data *) vict_obj)); 
		dg_victim = (struct char_data *) vict_obj;
		break; 
	     case 'o': 
		CHECK_NULL(obj, OBJS(obj, to)); 
		if(!strcmp(i,ACTNULL))
		   {
		   CHECK_NULL(obj, OBJN(obj, to));
		   }
		else		/* skip to the second word */
		   {
		   while((*i==' ')&&(*i!='\0'))
		      i++;
		   while((*i!=' ')&&(*i!='\0'))
		      i++;
		   while((*i==' ')&&(*i!='\0'))
		      i++;
		   if(*i=='\0')
		      {
		      i="ERROR";
		      mudlogf(NRM,LVL_IMMORT,TRUE,
			      "Item vnum #%ld needs a two word short desc",
			      GET_OBJ_VNUM(obj));
		      }
		   }		   
		break; 
	     case 'O': 
		CHECK_NULL(vict_obj, OBJS((const struct obj_data *) vict_obj, to)); 
		if(!strcmp(i,ACTNULL))
		   {
		   CHECK_NULL(vict_obj,OBJN((const struct obj_data *) vict_obj, to));
		   }
		else		/* skip to the second word */
		   {
		   while((*i==' ')&&(*i!='\0'))
		      i++;
		   while((*i!=' ')&&(*i!='\0'))
		      i++;
		   while((*i==' ')&&(*i!='\0'))
		      i++;
		   if(*i=='\0')
		      {
		      i="ERROR";
		      mudlogf(NRM,LVL_IMMORT,TRUE,
			      "Item vnum #%ld needs a two word short desc",
			      GET_OBJ_VNUM((struct obj_data *)vict_obj));
		      }
		   }		   
		dg_target = (struct obj_data *) vict_obj;
		break; 
	     case 'p': 
		CHECK_NULL(obj, OBJS(obj, to)); 
		break; 
	     case 'P': 
		CHECK_NULL(vict_obj, OBJS((const struct obj_data *) vict_obj, to)); 
		dg_target = (struct obj_data *) vict_obj;
		break; 
	     case 'a': 
		CHECK_NULL(obj, SANA(obj)); 
		break; 
	     case 'A': 
		CHECK_NULL(vict_obj, SANA((const struct obj_data *) vict_obj)); 
		dg_target = (struct obj_data *) vict_obj;
		break; 
	     case 'T': 
		CHECK_NULL(vict_obj, vict_obj); 
		dg_arg = (char *) vict_obj;
		break; 
	     case 'F': 
		CHECK_NULL(vict_obj, fname( vict_obj)); 
		break; 
	     case '$': 
		i = "$"; 
		break; 
	     case ' ':
	     default: 
		i = "BAD_DOLLAR_VALUE";
		log("SYSERR: Illegal $-code to act(): %c",*orig); 
		log("SYSERR: %s",orig); 
		break; 
	    } 
	 while ((*buf = *(i++))) 
	    buf++; 
	 orig++; 
	 } 
      else if (!(*(buf++) = *(orig++))) 
	 break; 
      } 
 
   *(--buf) = '\r'; 
   *(++buf) = '\n'; 
   *(++buf) = '\0'; 
 
   if (to->desc)
      SEND_TO_Q(to->desc,"%s",CAP(lbuf));
   if (MOBTrigger)
      mprog_act_trigger(lbuf, to, ch, obj, vict_obj);
   if (IS_NPC(to) && dg_act_check)
      act_mtrigger(to, lbuf, ch, dg_victim, obj, dg_target, dg_arg);
   release_buffer(lbuf);
} 
 

/*#define SENDOK(ch) (((ch)->desc ||\
  (IS_MOB(ch)&&mob_index[(ch)->nr].progtypes&ACT_PROG))&&\
  (AWAKE(ch) || is_sleep) && \
  !PLR_FLAGGED((ch), PLR_WRITING))
  */
void act_t(char *str, int hide_invisible, struct char_data *ch, 
	    struct obj_data *obj, void *vict_obj, int type,const char *func,int line)
{ 
   struct char_data *to;
   int to_sleeping;
 
   if (!str || !*str)
      {
      MOBTrigger = TRUE;
      return;
      }

   if (!(dg_act_check = !(type & DG_NO_TRIG)))
      type &= ~DG_NO_TRIG;

  /* 
   * Warning: the following TO_SLEEP code is a hack. 
   *  
   * I wanted to be able to tell act to deliver a message regardless of sleep 
   * without adding an additional argument.  TO_SLEEP is 128 (a single bit 
   * high up).  It's ONLY legal to combine TO_SLEEP with one other TO_x 
   * command.  It's not legal to combine TO_x's with each other otherwise. 
   * TO_SLEEP only works because its value "happens to be" a single bit;
   * do not change it to something else.  In short, it is a hack.
   */ 
 
  /* check if TO_SLEEP is there, and remove it if it is. */ 
   if ((to_sleeping = (type & TO_SLEEP))) 
      type &= ~TO_SLEEP; 
 
   if (IS_SET(type, TO_CHAR))
      { 
      if (ch && SENDOK(ch)) 
	 perform_act(str, ch, obj, vict_obj, ch); 
      MOBTrigger = TRUE;
      return; 
      } 

   if (IS_SET(type,TO_VICT)) 
      { 
      if ((to = vict_obj) && SENDOK(to)) 
	 {
	 perform_act(str, ch, obj, vict_obj, to); 
	 }
      MOBTrigger = TRUE;
      return; 
      } 
  /* ASSUMPTION: at this point we know type must be TO_NOTVICT or TO_ROOM */ 
 
   if (ch && IN_ROOM(ch) != NOWHERE) 
      to = world[IN_ROOM(ch)].people; 
   else if (obj && IN_ROOM(obj) != NOWHERE) 
      to = world[IN_ROOM(obj)].people; 
   else 
      { 
      log("SYSERR: no valid target to act()!: %s (%s %d)",
	  *str?str:"INVALID STRING",func,line); 
      return; 
      } 
 
   for (; to; to = to->next_in_room) 
      {
      if (!SENDOK(to) || (to == ch))
	 continue;
      if (hide_invisible && ch && !CAN_SEE(to, ch))
	 continue;
      if (IS_SET(type,TO_NOTVICT) && to == vict_obj)
	 continue;
      if(!IS_NPC(to)&&IS_SET(type,FR_FIGHT)&&PRF2_FLAGGED(to,PRF2_NOSPAM))
	 continue;
      perform_act(str, ch, obj, vict_obj, to); 
      }
   MOBTrigger = TRUE;
} 

void    send_battle(const char *messg, ...) 
{ 
   struct descriptor_data *i; 
   struct char_data *ch;
   va_list args;

   va_start(args,messg);
   if (messg) 
      for (i = descriptor_list; i; i = i->next) 
	 { 
	 if(i->original)
	    ch = i->original;
	 else
	    ch = i->character;
	 if (STATE(i)==CON_PLAYING && 
	     !PRF_FLAGGED(ch, PRF_NOBATTLE) && 
	     !PLR_FLAGGED(ch, PLR_WRITING)) 
 
	    { 
	    send_to_char(ch, CCCYN(ch, C_SPR)); 
	    vwrite_to_output(i,messg,args);
	    send_to_char(ch, CCNRM(ch, C_SPR)); 
	    } 
	 } 
   va_end(args);
} 

void    send_info(const char *messg, ...)
{
   struct descriptor_data *i;
   struct char_data *ch;
   va_list args;

   va_start(args,messg);
   if (messg)
      for (i = descriptor_list; i; i = i->next) 
	 {
	 if(i->original)
	    ch = i->original;
	 else
	    ch = i->character;
	 if (STATE(i)==CON_PLAYING && 
	     !PRF2_FLAGGED(ch, PRF2_NOINFO) && 
	     !PLR_FLAGGED(ch, PLR_WRITING)) 
	    {
	    send_to_char(ch, CCWHT(ch, C_SPR)); 
	    vwrite_to_output(i,messg,args);
	    send_to_char(ch, CCNRM(ch, C_SPR)); 
	    }
	 }
   va_end(args);
}

void send_auction_god(const char *messg, ...) 
{ 
   struct descriptor_data *i; 
   struct char_data *ch;
   va_list args;

   va_start(args,messg);

   if (messg) 
      for (i = descriptor_list; i; i = i->next) 
	 { 
	 if(i->original)
	    ch = i->original;
	 else
	    ch = i->character;
	 if (STATE(i)==CON_PLAYING && 
	     !PRF_FLAGGED(ch, PRF_NOAUCT) && 
	     !PLR_FLAGGED(ch, PLR_WRITING)) 
	    { 
	    if(GET_LEVEL(ch) >= LVL_DGOD)
	       {
	       send_to_char(ch, CCCYN(ch, C_SPR)); 
	       vwrite_to_output(i,messg,args);
	       send_to_char(ch, CCNRM(ch, C_SPR)); 
	       }
	    } 
	 } 
   va_end(args);
} 

void send_auction_mort(const char *messg, ...) 
{ 
   struct descriptor_data *i; 
   struct char_data *ch;
   va_list args;

   va_start(args,messg);

   if (messg) 
      for (i = descriptor_list; i; i = i->next) 
	 { 
	 if(i->original)
	    ch = i->original;
	 else
	    ch = i->character;
	 if (STATE(i)==CON_PLAYING && 
	     !PRF_FLAGGED(ch, PRF_NOAUCT) && 
	     !PLR_FLAGGED(ch, PLR_WRITING)) 
	    { 
	    if(GET_LEVEL(ch) < LVL_DGOD)
	       {
	       send_to_char(ch, CCCYN(ch, C_SPR)); 
	       vwrite_to_output(i,messg,args);
	       send_to_char(ch, CCNRM(ch, C_SPR)); 
	       }
	    } 
	 } 
   va_end(args);
} 


/* Write last command */
void write_last_command ()
{
   FILE *fd;
   time_t ct;
   char *time_s;
  /* Return if no last command - set before normal exit */
   if (!last_command[0])
      return;

   ct=time(0);
   time_s=asctime(localtime(&ct));
   *(time_s+strlen(time_s)-1)='\0';

   fprintf(stderr, "%-20.20s :: %s", time_s+4,last_command);
   
   fd = fopen (LAST_COMMAND_FILE,"a+");
   
   if (fd == 0)
      return;
   
   fprintf(fd, "%-20.20s :: %s\n", time_s+4,last_command);
   fflush(fd);
   fclose (fd);
}


void setup_log(const char *filename, int fd)
{
   FILE *s_fp;
   
#if defined(__MWERKS__) || defined(__GNUC__)
   s_fp = stderr;
#else
   if ((s_fp = fdopen(STDERR_FILENO, "w")) == NULL) 
      {
      puts("SYSERR: Error opening stderr, trying stdout.");
      
      if ((s_fp = fdopen(STDOUT_FILENO, "w")) == NULL) 
	 {
	 puts("SYSERR: Error opening stdout, trying a file.");
	 
	/* If we don't have a file, try a default. */
	 if (filename == NULL || *filename == '\0')
	    filename = "log/syslog";
	 }
      }
#endif
   
   if (filename == NULL || *filename == '\0') 
      {
     /* No filename, set us up with the descriptor we just opened. */
      logfile = s_fp;
      puts("Using file descriptor for logging.");
      return;
      }
   
  /* We honor the default filename first. */
   if (open_logfile(filename, s_fp))
      return;
   
  /* Well, that failed but we want it logged to a file so try a default. */
   if (open_logfile("log/syslog", s_fp))
      return;
   
  /* Ok, one last shot at a file. */
   if (open_logfile("syslog", s_fp))
      return;
   
  /* Erp, that didn't work either, just die. */
   puts("SYSERR: Couldn't open anything to log to, giving up.");
   exit(1);
}

int open_logfile(const char *filename, FILE *stderr_fp)
{
   if (stderr_fp)       /* freopen() the descriptor. */
      logfile = freopen(filename, "w", stderr_fp);
   else
      logfile = fopen(filename, "w");
   
   if (logfile) 
      {
      printf("Using log file '%s'%s.\n",
	     filename, stderr_fp ? " with redirection" : "");
      return TRUE;
      }
   
   printf("SYSERR: Error opening file '%s': %s\n", filename, strerror(errno));
   return FALSE;
}

