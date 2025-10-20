/* ************************************************************************
*   File: interpreter.h                                 Part of CircleMUD *
*  Usage: header file: public procs, macro defs, subcommand defines       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define ACMD(name)  \
   void (name)(struct char_data *ch, char *argument, int cmd, int subcmd)

#define CMD_NAME (cmd>0?cmd_info[cmd].command:"none")
#define CMD_IS(cmd_name) (cmd>0?!strcmp(cmd_name, cmd_info[cmd].command):1)
#define IS_MOVE(cmdnum) (cmdnum >= 1 && cmdnum <= NUM_OF_DIRS)

struct command_info {
   char *command;
   byte minimum_position;
   void	(*command_pointer)
   (struct char_data *ch, char * argument, int cmd, int subcmd);
   sh_int minimum_level;
   int	subcmd;
};

/* necessary for CMD_IS macro */
#ifndef __INTERPRETER_C__
extern struct command_info cmd_info[];
#endif



void	command_interpreter(struct char_data *ch, char *argument);
int	search_block(char *arg, char **list, int exact);
char	lower( char c );
char	*one_argument(char *argument, char *first_arg);
char	*one_word(char *argument, char *first_arg);
char	*any_one_arg(char *argument, char *first_arg);
char	*two_arguments(char *argument, char *first_arg, char *second_arg);
char *five_arguments(char *argument, char *first_arg, char *second_arg, char *third_arg, char *fourth_arg, char *fifth_arg);

int	fill_word(char *argument);
void	half_chop(char *string, char *arg1, char *arg2);
void	nanny(struct descriptor_data *d, char *arg);
int	is_abbrev(char *arg1, const char *arg2);
int	is_number(char *str);
int	find_command(char *command);
void	skip_spaces(char **string);
void    strip_color(char *inbuf);
char	*delete_doubledollar(char *string);

/* for compatibility with 2.20: */
#define argument_interpreter(a, b, c) two_arguments(a, b, c)

struct alias_data {
  char *alias;
  char *replacement;
  int type;
  struct alias_data *next;
};

#define ALIAS_SIMPLE	0
#define ALIAS_COMPLEX	1

#define ALIAS_SEP_CHAR	';'
#define ALIAS_VAR_CHAR	'$'
#define ALIAS_GLOB_CHAR	'*'

/*
 * SUBCOMMANDS
 *   You can define these however you want to, and the definitions of the
 *   subcommands are independent from function to function.
 */

/* directions */
#define SCMD_NORTH	1
#define SCMD_EAST	2
#define SCMD_SOUTH	3
#define SCMD_WEST	4
#define SCMD_UP		5
#define SCMD_DOWN	6

/* do_gen_ps */
#define SCMD_INFO       0
#define SCMD_HANDBOOK   1 
#define SCMD_CREDITS    2
#define SCMD_NEWS       3
#define SCMD_WIZLIST    4
#define SCMD_POLICIES   5
#define SCMD_VERSION    6
#define SCMD_IMMLIST    7
#define SCMD_MOTD	8
#define SCMD_IMOTD	9
#define SCMD_CLEAR	10
#define SCMD_WHOAMI	11
#define SCMD_AREAS	12
#define SCMD_TEAMS	13
#define SCMD_MARRIAGES  14
#define SCMD_HEROLIST 15

/* do_spell_help */
#define SCMD_SPELLS	1
#define SCMD_SKILLS	2

/* do_gen_tog */
#define SCMD_NOSUMMON   0
#define SCMD_NOHASSLE   1
#define SCMD_BRIEF      2
#define SCMD_COMPACT    3
#define SCMD_NOTELL	4
#define SCMD_NOAUCTION	5
#define SCMD_DEAF	6
#define SCMD_NOGOSSIP	7
#define SCMD_NOGRATZ	8
#define SCMD_NOWIZ	9
#define SCMD_QUEST	10
#define SCMD_ROOMFLAGS	11
#define SCMD_NOREPEAT	12
#define SCMD_HOLYLIGHT	13
#define SCMD_SLOWNS	14
#define SCMD_AUTOEXIT	15
#define SCMD_IDENT      16
#define SCMD_INFOBAR    17  /* -naj infobar2 12/16/96 - def for do_gen_tog */
#define SCMD_SCOREBAR   18  /* -naj infobar2 12/16/96 - def for do_gen_tog */
#define SCMD_METER	19  /* -naj infobar2 12/16/96 - def for do_gen_tog*/
#define SCMD_ASCII      20  /* -naj infobar2 12/16/96 - def for do_gen_tog*/
#define SCMD_AUTOSPLIT  21  /* Autosplit from snippets page --Erika */
#define SCMD_AUTOLOOT   22  /* Autoloot from snippets page --Erika */
#define SCMD_NOBATTLE 	23
#define SCMD_AUTOASSIST 24
#define SCMD_AUTOSAC    25
#define SCMD_AUTOGOLD   26
#define SCMD_AFK        27
#define SCMD_NOOOC      28
#define SCMD_PAGE_OK    29
#define SCMD_NOINFO     30
#define SCMD_NOSPAM     31
#define SCMD_TRACK      32
#define SCMD_XAP_OBJS   33
#define SCMD_NORECALL   34
#define SCMD_NOMUSIC    35
#define SCMD_MORTAL     36

/* do_gen_vfile */
#define SCMD_V_BUGS      0
#define SCMD_V_IDEAS     1
#define SCMD_V_TYPOS     2
#define SCMD_V_CHANGES   3
#define SCMD_V_SYSLOG    4
#define SCMD_V_MAILLOG   5
#define SCMD_V_ERRORS    6
#define SCMD_V_BIGRENT   7
#define SCMD_V_BUF       8
#define SCMD_V_LASTCMD   9
#define SCMD_V_GODCMD    10
#define SCMD_V_DELETE    11
#define SCMD_V_RIP       12
#define SCMD_V_CORPSE    13
#define SCMD_V_SCRIPTERR 14
#define SCMD_V_CRASH     15
#define SCMD_V_HELP      16
#define SCMD_V_GOLD      17
#define SCMD_V_LOCATE_OBJ 18
#define SCMD_V_LEVELS    19
#define SCMD_V_NEWPLAYERS 20
#define SCMD_V_DEATH     21
#define SCMD_V_BAN       22
#define SCMD_V_OBJSCRAP  23
#define SCMD_V_OLC       24
#define SCMD_V_USAGE     25
#define SCMD_V_RESTARTS  26
#define SCMD_V_RENTGONE  27
#define SCMD_V_BADPWS    28
#define SCMD_V_GODFIGHT  29
#define SCMD_V_SCRIPTLOG 30
#define SCMD_V_SHOP      31

/* do_logsearch */
#define SCMD_LOGSEARCH  0
#define SCMD_VIEWLOG    1

/* do_wizutil */
#define SCMD_REROLL	0
#define SCMD_PARDON     1
#define SCMD_NOTITLE    2
#define SCMD_SQUELCH    3
#define SCMD_FREEZE	4
#define SCMD_THAW	5
#define SCMD_UNAFFECT	6

/* do_spec_com */
#define SCMD_WHISPER	0
#define SCMD_ASK	1

/* do_gen_com */
#define SCMD_HOLLER	0
#define SCMD_SHOUT	1
#define SCMD_GOSSIP	2
#define SCMD_AUCTION	3
#define SCMD_GRATZ	4
#define SCMD_OOC        5
#define SCMD_MUSIC      6


/* do_shutdown */
#define SCMD_SHUTDOW	0
#define SCMD_SHUTDOWN   1

/* do_quit */
#define SCMD_QUI	0
#define SCMD_QUIT	1
#define SCMD_CAMP       2
#define SCMD_CAMPR      3

/* do_date */
#define SCMD_DATE	0
#define SCMD_UPTIME	1

/* do_commands */
#define SCMD_COMMANDS	0
#define SCMD_SOCIALS	1
#define SCMD_WIZHELP	2

/* do_drop */
#define SCMD_DROP	0
#define SCMD_JUNK	1
#define SCMD_DONATE	2
#define SCMD_CDONATE    3

/* do_gen_write */
#define SCMD_BUG	0
#define SCMD_TYPO	1
#define SCMD_IDEA	2

/* do_look */
#define SCMD_LOOK	0
#define SCMD_READ	1

/* do_qcomm */
#define SCMD_QSAY	0
#define SCMD_QECHO	1

/* do_pour */
#define SCMD_POUR	0
#define SCMD_FILL	1

/* do_poof */
#define SCMD_POOFIN	0
#define SCMD_POOFOUT	1
#define SCMD_POOFCHECK  2

/* do_hit */
#define SCMD_HIT	0
#define SCMD_MURDER	1

/* do_eat */
#define SCMD_EAT	0
#define SCMD_TASTE	1
#define SCMD_DRINK	2
#define SCMD_SIP	3
/*And*/
#define SCMD_LOOT       1


/* do_use */
#define SCMD_USE	0
#define SCMD_QUAFF	1
#define SCMD_RECITE	2
#define SCMD_SWALLOW    3	/* Pill modification--Aleks */

/* do_cast */
#define SCMD_CAST       0
#define SCMD_SING       1

/* do_echo */
#define SCMD_ECHO	0
#define SCMD_EMOTE	1

/* do_gen_door */
#define SCMD_OPEN       0
#define SCMD_CLOSE      1
#define SCMD_UNLOCK     2
#define SCMD_LOCK       3
#define SCMD_PICK       4

/* do_spellhelp */
#define SCMD_UNUSED     0
#define SCMD_SPELL      1
#define SCMD_SKILL      2

/*. do_olc .*/
#define SCMD_OLC_REDIT    0
#define SCMD_OLC_OEDIT    1
#define SCMD_OLC_ZEDIT    2
#define SCMD_OLC_MEDIT    3
#define SCMD_OLC_SEDIT    4
#define SCMD_OLC_GEDIT    5
#define SCMD_OLC_PEDIT    6
#define SCMD_OLC_HEDIT    7
#define SCMD_OLC_TRIGEDIT 8
#define SCMD_OLC_SAVEINFO 9
#define SCMD_OLC_ASSEDIT  10

/*. do_skillset .*/
#define SCMD_SETSKILL   0
#define SCMD_SETLEARN   1

/*. do_snoop .*/
#define SCMD_SNOOP      0
#define SCMD_REVSNOOP   1

/*. do_force .*/
#define SCMD_IFORCE     0
#define SCMD_FORCE      1

/* * do_assemble * These constants *must* corespond with
     the ASSM_xxx constants in * assemblies.h. */
#define SCMD_ASSEMBLE	0
#define SCMD_BAKE	1
#define SCMD_BREW 	2
#define SCMD_CRAFT	3
#define SCMD_FLETCH	4
#define SCMD_KNIT	5
#define SCMD_MAKE 	6
#define SCMD_MIX	7
#define SCMD_THATCH	8
#define SCMD_WEAVE	9


/* -naj infobar2 12/16/96 - SCMDB definitions for infobar - start */
/*
 * SCMDB_ was an old naming convention for compatibility in other versions
 * of circle.  There's no need to have for this convention anymore but there
 * is also no need to change them.
 * NOTE:  At this point in time, ONLY the following SCMDB_ functions should
 * be called externally:  RESIZE, REDRAW, CLEAR, and GENUPDATE.
 */
/*
 * -naj 8/30/95 mod: infobar1       (scmdb_ for do_infobar)
 *  notes:  these are scmdb_? for do_infobar.  The SCMDB_ naming to prevent 
 *  confusion with other SCMD_'s.  Most of the SCMDB_ will be used internally 
 *  in do_infobar.
 */ 
#define SCMDB_REDRAW		1	/* external use */
#define SCMDB_CLEAR		2       
#define SCMDB_HIT		3 	/* internal use */
#define SCMDB_END		4
#define SCMDB_MANA		5
#define SCMDB_MAXHIT		6
#define SCMDB_MAXEND		7
#define SCMDB_MAXMANA		8
#define SCMDB_EXITS		9
#define SCMDB_ROOMDESC		10
#define SCMDB_GOLD		11
#define SCMDB_EXP		12
#define SCMDB_AFF		13
#define SCMDB_STR		14
#define SCMDB_DEX		15
#define SCMDB_INT		16
#define SCMDB_WIS		17
#define SCMDB_CON		18
#define SCMDB_CHA		19
#define SCMDB_PRA		20
#define SCMDB_LEV		21
#define SCMDB_AC		22
#define SCMDB_DEF		23
#define SCMDB_ALIGN		23   /* for phoenix, align will use def's place */
#define SCMDB_HITBON		24
#define SCMDB_DAMBON		25
#define SCMDB_HUNGER		26
#define SCMDB_THIRST		27
#define SCMDB_RACE		28
#define SCMDB_CLASS		29
#define SCMDB_LEVEL		30
#define SCMDB_BANK		31
#define SCMDB_CARRYING		32
#define SCMDB_NEWROOM		33	/* these are now obsolete */
#define SCMDB_GAINLEVEL		34
#define SCMDB_WEAR		35
#define SCMDB_COMBAT		36
#define SCMDB_GET		37
#define SCMDB_TITLE		38
#define SCMDB_RESIZE		39	/* for external use */
#define SCMDB_GENUPDATE		40 	/* the handy dandy all-purpose general update command. */

/* -naj infobar2 12/16/96 - SCMDB definitions for infobar - end */

#define SHOW_ZONES              1
#define SHOW_PLAYER             2
#define SHOW_RENT               3
#define SHOW_STATS              4
#define SHOW_ERRORS             5
#define SHOW_DEATH              6
#define SHOW_GODROOMS           7
#define SHOW_SHOPS              8
#define SHOW_HOUSES             9
#define SHOW_SPELLS             10
#define SHOW_SKILLS             11
#define SHOW_PLAGUE             12
#define SHOW_MOBS               13
#define SHOW_OBJS               14
#define SHOW_IDLE               15
#define SHOW_PEACEFUL           16
#define SHOW_NOMOB              17
#define SHOW_NOTRACK            18
#define SHOW_NOMAGIC            19
#define SHOW_CLAN               20
#define SHOW_REGEN              21
#define SHOW_TUNNEL             22
#define SHOW_PRIVATE            23
#define SHOW_NORECALL           24
#define SHOW_NOSUMMON           25
#define SHOW_PKILL              26
#define SHOW_ACTIVE             27
#define SHOW_BUFFER             28
#define SHOW_LOGBUF             29
#define SHOW_MOBSPECS           30
#define SHOW_HOMETOWNS          31
#define SHOW_OLCZONES           32
#define SHOW_KILLS              33
#define SHOW_NEWBIEEQ           34
#define SHOW_FIGHTING           35
#define SHOW_TWOHANDED          36
#define SHOW_NOPOSCHK           37
#define SHOW_GUILDMASTER        38
#define SHOW_CONNECTIONS        39
#define SHOW_TELEPORT           40
#define SHOW_RCONNECT           41
#define SHOW_MAZE               42
#define SHOW_GODCOMMAND         43
#define SHOW_ITEMSPELL          44
#define SHOW_BADITEMSPLLVL      45
#define SHOW_NODECAY            46
#define SHOW_DONATION           47
#define SHOW_RACE               48
#define SHOW_SNOOP              49
#define SHOW_OBJDATA            50
#define SHOW_ASSEMBLIES         51
#define SHOW_SPELLAFFECTS       52
#define SHOW_WEAPONSPELL        53
#define SHOW_OLD_REMORTEQ       54
#define SHOW_OLD_DBLREMORTEQ    55
#define SHOW_OBJRESIST          56
#define SHOW_IGNORED            57
#define SHOW_DBLREMORTEQ        58
#define SHOW_REMORTEQ           59
#define SHOW_GOD_EQ             60
#define SHOW_REIMB              61
#define SHOW_SWITCH             62
#define SHOW_SPELLSTAT          63
#define SHOW_NEVERMOB           64
#define SHOW_QUESTS             65
#define SHOW_GRAFFITI           66
#define SHOW_EXPLORED           67
#define SHOW_EMAIL              68
#define SHOW_GREMORT_RECORDS    69
#define SHOW_PLAYER_SHOPS       70
