/*************************************************************************
*   File: config.c                                      Part of CircleMUD *
*  Usage: Configuration of various aspects of CircleMUD operation         *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define __CONFIG_C__

#include "../localHeader/conf.h"
#include "../localHeader/sysdep.h"

#include "structs.h"
#include "interpreter.h"
#define TRUE	1
#define YES	1
#define FALSE	0
#define NO	0

/*
 * Below are several constants which you can change to alter certain aspects
 * of the way CircleMUD acts.  Since this is a .c file, all you have to do
 * to change one of the constants (assuming you keep your object files around)
 * is change the constant in this file and type 'make'.  Make will recompile
 * this file and relink; you don't have to wait for the whole thing to
 * recompile as you do if you change a header file.
 *
 * I realize that it would be slightly more efficient to have lots of
 * #defines strewn about, so that, for example, the autowiz code isn't
 * compiled at all if you don't want to use autowiz.  However, the actual
 * code for the various options is quite small, as is the computational time
 * in checking the option you've selected at run-time, so I've decided the
 * convenience of having all your options in this one file outweighs the
 * efficency of doing it the other way.
 *
 */

/****************************************************************************/
/****************************************************************************/


/* GAME PLAY OPTIONS */

/*
 * pk_allowed sets the tone of the entire game.  If pk_allowed is set to
 * NO, then players will not be allowed to kill, summon, charm, or sleep
 * other players, as well as a variety of other "asshole player" protections.
 * However, if you decide you want to have an all-out knock-down drag-out
 * PK Mud, just set pk_allowed to YES - and anything goes.
 */
int pk_allowed = NO;

/* is playerthieving allowed? */
int pt_allowed = NO;

/* minimum level a player must be to shout/holler/gossip/auction */
int level_can_shout = 0;

/* number of movement points it costs to holler */
int holler_move_cost = 20;

/* exp change limits */
int max_exp_gain = 100000;	/* max gainable per kill */
int max_exp_loss = 500000;	/* max losable per death */
/* minimum number of mobs needed to reach next level */
int min_kills = 20;		/* for exp cap */
int mob_exp_divisor = 23;	/* for figuring out how much exp a mob has 
				 * try to keep at a little above min_kills 
				 * since that is used for the cap*/

/* the maximum damage you can do in one hit */
int max_damage = 500;

/* number of tics (usually 75 seconds) before PC/NPC corpses decompose */
int max_npc_corpse_time = 5;
int max_pc_corpse_time = 30;
int item_decay = 168;

/* How many ticks before a player is sent to the void or idle-rented. */
int idle_void = 8;
int idle_rent_time = 48;

/* This level and up is immune to idling, LVL_IMPL+1 will disable it. */
int idle_max_level = LVL_IMMORT;

/* should items in death traps automatically be junked? */
int dts_are_dumps = NO;

/*
 * You can define or not define TRACK_THOUGH_DOORS, depending on whether
 * or not you want track to find paths which lead through closed or
 * hidden doors. A setting of 'NO' means to not go through the doors
 * while 'YES' will pass through doors to find the target.
 */
int track_through_doors = YES;

/* "okay" etc. */
const char *OK = "Okay.\r\n";
const char *NOPERSON = "No-one by that name here.\r\n";
const char *NOEFFECT = "Nothing seems to happen.\r\n";

/****************************************************************************/
/****************************************************************************/


/* RENT/CRASHSAVE OPTIONS */

/*
 * Should the MUD allow you to 'rent' for free?  (i.e. if you just quit,
 * your objects are saved at no cost, as in Merc-type MUDs.)
 */
int free_rent = YES;

/* maximum number of items players are allowed to rent */
int max_obj_save = 200;

/* maximum number of items players are allowed to have in each house room */
int max_obj_house = 50;

/* receptionist's surcharge on top of item costs */
int min_rent_cost = 100;

/*
 * Should the game automatically save people?  (i.e., save player data
 * every 4 kills (on average), and Crash-save as defined below.
 */
int auto_save = YES;

/*
 * if auto_save (above) is yes, how often (in minutes) should the MUD
 * Crash-save people's objects?   Also, this number indicates how often
 * the MUD will Crash-save players' houses.
 */
int autosave_time = 3;

/* Lifetime of crashfiles and forced-rent (idlesave) files in days */
int crash_file_timeout = 30;

/* Lifetime of normal rent files in days */
int rent_file_timeout = 60;

int auction_cost = 25;
float auction_profit = 0.05;
/****************************************************************************/
/****************************************************************************/


/* ROOM NUMBERS */

/* virtual number of room that mortals should enter at */
room_vnum mortal_start_room = 3001;

/* virtual number of room that immorts should enter at by default */
room_vnum immort_start_room = 1204;

/* virtual number of room that frozen players should enter at */
room_vnum frozen_start_room = 1259;

/*
 * virtual numbers of donation rooms.  note: you must change code in
 * do_drop of act.item.c if you change the number of non-NOWHERE
 * donation rooms.
 */
room_vnum donation_rooms[] = {3063,8402,10096,5510,17609,16220,1122,22068,23336,-1};
int num_donation = 9;

/****************************************************************************/
/****************************************************************************/


/* GAME OPERATION OPTIONS */

/*
 * This is the default port the game should run on if no port is given on
 * the command-line.  NOTE WELL: If you're using the 'autorun' script, the
 * port number there will override this setting.  Change the PORT= line in
 * instead of (or in addition to) changing this.
 */
int DFLT_PORT = 4000;

/* IP address to which the MUD should bind.  This is really only
 * useful if you're running Circle on a host that host more than one
 * IP interface, and you only want to bind to *one* of them instead of
 * all of them.  Setting this to NULL (the default) causes Circle to
 * bind to all interfaces on the host.  Otherwise, specify an IP
 * address in dotted quad format, and Circle will only bind to that IP
 * address.  (Of course, that IP address must be one of your host's
 * interfaces, or it won't work.)
 */
char *DFLT_IP = NULL; /* bind to all interfaces */
/* char *DFLT_IP = "192.168.1.1";  -- bind only to one interface */

/* default directory to use as data directory */
char *DFLT_DIR = "lib";


/* What file to log messages to (ex: "log/syslog").  Setting this to NULL
 *  means you want to log to stderr, which was the default in earlier
 * versions of Circle.
 */
char *LOGNAME = NULL;
/* char *LOGNAME = "log/syslog";  -- useful for Windows users */

/* maximum number of players allowed before game starts to turn people away */
int MAX_PLAYERS = 200;

/* maximum size of bug, typo and idea files in bytes (to prevent bombing) */
int max_filesize = 50000;

/* maximum number of password attempts before disconnection */
int max_bad_pws = 3;

 /*
 * Rationale for enabling this, as explained by naved@bird.taponline.com.
 *
 * Usually, when you select ban a site, it is because one or two people are
 * causing troubles while there are still many people from that site who you
 * want to still log on.  Right now if I want to add a new select ban, I need
 * to first add the ban, then SITEOK all the players from that site except for
 * the one or two who I don't want logging on.  Wouldn't it be more convenient
 * to just have to remove the SITEOK flags from those people I want to ban
 * rather than what is currently done?
 */
int siteok_everyone = FALSE;

/*
 * Some nameservers are very slow and cause the game to lag terribly every 
 * time someone logs in.  The lag is caused by the gethostbyaddr() function
 * which is responsible for resolving numeric IP addresses to alphabetic names.
 * Sometimes, nameservers can be so slow that the incredible lag caused by
 * gethostbyaddr() isn't worth the luxury of having names instead of numbers
 * for players' sitenames.
 *
 * If your nameserver is fast, set the variable below to NO.  If your
 * nameserver is slow, of it you would simply prefer to have numbers
 * instead of names for some other reason, set the variable to YES.
 *
 * You can experiment with the setting of nameserver_is_slow on-line using
 * the SLOWNS command from within the MUD.
 */

int nameserver_is_slow = YES;

const char * NAME_POLICY =
"\r\n\x1B[1m\x1B[36mNAME POLICY\r\n"
"In an effort to promote a medieval, fantasy atmosphere throughout the\r\n"
"Realms of Phoenix Mud, a name policy regarding character creation has\r\n"
"been established.  Under this new policy, the following guidelines must be\r\n"
"adhered to when choosing a new name:                                      \r\n\r\n"
 
"[1] Names must NOT be composed of common words, including common nouns, verbs,\r\n"
"    propositions, or any combination thereof.\r\n"
"    For example:  EvilMage, Flower, Backstab, DarkOne\r\n\r\n"
"[2] No obscene, insulting, threatening or degrading names will be tolerated.\r\n"
"    For example: (No examples needed)\r\n\r\n"
"[3] No modern trademarks, cartoon characters, product names, etc. are allowed.\r\n"
"    For example:  Beavis, Bacardi, Seinfeld\r\n\r\n"
"[4] No incoherent collection of random letters is allowed.\r\n"
"    For example:  Wqzlks, Xeyplw\r\n\r\n"
"[5] No names that imply rank or privilage.\r\n"
"    For example: King, Lady\r\n\r\n"
"[6] No animal names.\r\n"
"    For example: Cougar, Puma, Snake, etc.\r\n\r\n"
"[7] No Capital letters in the name other then the first letter\r\n"
"    For example: STUDBOY \r\n\r\n"
"[8] _Do_ TRY to choose a medieval type of name.  If you have any questions\r\n"
"    regarding the acceptability of a certain name, feel free to ask a God for\r\n"
"    input and guidance.\r\n\r\n"
"[9] If you are asked to change your name by a god, you *must* do so.\r\n"
"    The Immortal staff has the final say in whether a name is acceptable or not.\r\n\r\n"
"\r\n\x1B[0m";

const char * MENU =
/*
 * -naj 8/30/95 mod: infobar1 (this is to clean the screan up from the infobar)
 */
"" /* -naj infobar2 12/16/96 - screen cleanup */
"\r\n"
"        ____________________________ \r\n"
"       ()                           )\r\n"
"        |    Welcome, traveller!    |\r\n"
"        |                           |\r\n"
"        |    0) LEAVE the game.     |\r\n"                
"        |    1) ENTER the game.     |\r\n"
"        |    2) Change description. |\r\n"
"        |    3) BACKGROUND story.   |\r\n"
"        |    4) Change password.    |\r\n"
"        |    5) Delete character.   |\r\n"
"        |    6) WHO is playing now. |\r\n"
"        |    7) Change e-mail.      |\r\n"
"        |                           |\r\n"
"        |                       /;  /\r\n"
"         \\  /^\\/^\\/' \\ /\\/^\\/^\\/ \\/^ \r\n"
"          ;/          \\;             \r\n"
"                                     \r\n"
"        MAKE YOUR CHOICE (0 to 6):\x1B[0m ";

const char * GREETINGS =   /* -naj greetings 12/16/96 - changed this since i was already here... */
"\x1B[1m\r\n\r\n"
"\x1B[36m 88,888. 88                                      \x1B[31m                         \r\n"
"\x1B[36m 88   88 88                                      \x1B[31m                       /\r\n"
"\x1B[36m 88   88 88                                      \x1B[31m       \\             / /\r\n"
"\x1B[36m 88   88 88                                      \x1B[31m        \\\\\\' ,      / //\r\n"
"\x1B[36m 88.  88 88,88. ,8888. ,8888. 88,88. 88 `88  88' \x1B[31m         \\\\\\//    _/ //'\r\n"
"\x1B[36m 88`88'  88  88 88'`88 88  88 88  88 88  `8\\/8' \x1B[31m           \\_-//' /  //<'\r\n"
"\x1B[36m 88      88  88 88  88 8888'  88  88 88   `88    \x1B[31m           \\ ///  <//'  \r\n"
"\x1B[36m 88      88  88 88.,88 88     88  88 88  ,8/\\8. \x1B[31m            /  >>   \\\\\\` \r\n"
"\x1B[36m 88      88  88 `8888' `8888' 88  88 88 ,88  88. (c) 1996\x1B[31m   /,)-^>>  _\\`  \r\n"
"\x1B[36m                                                 \x1B[31m          (/   \\\\ / \\\\\\ \r\n"
"\x1B[36m              M        U       D                 \x1B[31m               //  //\\\\\\\r\n"
"\x1B[36m                                                 \x1B[31m              ((  ((   \\\\ \r\n"
"\x1B[36m	   	    			  \x1B[31m \r\n"
"\x1B[36m						  \x1B[31m \r\n"
"\r\n"
"\x1B[32m                Brought to you by:                \r\n" 
"\x1B[32m        Opie, Aleksandr, Cymynedd, Masque         \r\n"     
"\x1b[31m   PhoenixMUD is made possible by Vineyard.haus   \r\n"
"\x1B[32m    Based on CircleMUD created by Jeremy Elson    \r\n"
"\x1B[32m       A derivative of DikuMUD, created by        \r\n"
"\x1B[32m       Hans Henrik Staerfeldt, Katja Nyboe,       \r\n"
"\x1B[32mTom Madsen, Michael Seifert, and Sebastian Hammer \x1B[0m\r\n" /* -Opie updated. 10/15/17 */
"\r\n";

const char *WELC_MESSG =    /* -naj welc_messg 12/16/96 - changed this since i was already here...*/
"\r\n"
"\x1B[1m\x1B[31mWelcome to the Realm of PhoenixMUD!\x1B[0m\r\n" /* -Opie updated. 10/15/17 */
"\x1B[1m\x1B[31mPlease don't forget to 'save reimburse' every now and then.\x1B[0m"
"\r\n\r\n";

const char *START_MESSG = "\x1B[1m\x1B[32mRead the NEWS! Lots has changed!\x1B[0m\r\n\r\n";     /* -naj start_messg 12/16/96 - changed this since i was already here... */

/****************************************************************************/
/****************************************************************************/


/* AUTOWIZ OPTIONS */

/* Should the game automatically create a new wizlist/immlist every time
   someone immorts, or is promoted to a higher (or lower) god level? */
int use_autowiz = YES;

/* If yes, what is the lowest level which should be on the wizlist?  (All
   immort levels below the level you specify will go on the immlist instead.) */
int min_wizlist_lev = LVL_SERP;


/* If YES, the mud will attempt to find the remote user name for each
   player connecting to the mud.  This can also be toggled in the game
   with the "ident" command. */
int ident = NO;


int CHECK = 0;
