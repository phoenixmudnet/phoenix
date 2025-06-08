/************************************************************************
 * OasisOLC - olc.h                                            v1.5     *
 *                                                                      *
 * Copyright 1996 Harvey Gilpin.                                        *
 ************************************************************************/

/*
 * If you don't want a short explanation of each field in your zone files,
 * change the number below to a 0 instead of a 1.
 */
#if 1
#define ZEDIT_HELP_IN_FILE
#endif
/*
 * If you want to clear the screen before certain Oasis menus, set to 1.
 */
#if 1
#define CLEAR_SCREEN   1
#endif

/*
 * Set this to 1 to enable MobProg support.
 */
#if 1
#define OASIS_MPROG    1
#endif

/*
 * Macros, defines, structs and globals for the OLC suite.
 */

#define MEDIT_ACCESS           LVL_ADMIN
#define OEDIT_ACCESS           LVL_ADMIN
/*. CONFIG DEFINES - these should be in structs.h, but here is easyer.*/

#define NUM_ROOM_FLAGS 		31
#define NUM_ROOM2_FLAGS         5
#define NUM_ROOM_SECTORS	15
#define NUM_TELEPORT            10
#define NUM_ORE_TYPES           22

#define NUM_MOB_FLAGS		32
#define NUM_MOB2_FLAGS		9
#define NUM_IMMUN_FLAGS		21
#define NUM_AFF_FLAGS		32
#define NUM_ATTACK_TYPES	16
#define NUM_MOB_CLASSES         20

#define NUM_ITEM_TYPES		40
#define NUM_ITEM_FLAGS		32
#define NUM_ITEM2_FLAGS         4
#define NUM_ANTI_FLAGS		32
#define NUM_ITEM_WEARS 		21
#define NUM_APPLIES		32
#define NUM_LIQ_TYPES 		17
#define NUM_POSITIONS		12
#define NUM_SPELLS		51
#define NUM_FUEL_TYPES          4

#define NUM_GENDERS		3
#define NUM_SHOP_FLAGS 	        3
#define NUM_TRADERS 		23

#define NUM_SKILLS              130

#define NUM_ZONE_FLAGS          8
#define NUM_ZONE_STATUS         5
#define NUM_ZONE_SOURCE         4
#define NUM_ZONE_CONTINENT      8

/*
 * Define this to how many MobProg scripts you have.
 */
#define NUM_PROGS              12

#define LVL_BUILDER		LVL_IMMORT

/*. Utils exported from olc.c .*/
void strip_string(char *);
void cleanup_olc(struct descriptor_data *d, byte cleanup_type);
void get_char_cols(struct char_data *ch);
void olc_add_to_save_list(int zone, byte type);
void olc_remove_from_save_list(int zone, byte type);


/*. OLC structs .*/

struct olc_data 
{
   int mode;
   int zone_num;
   int number;
   int value;
   int total_mprogs;
   char *func_name;
   int func_num;
   struct char_data *mob;
   struct room_data *room;
   struct obj_data *obj;
   struct zone_data *zone;
   struct shop_data *shop;
   struct guild_master_data *guild;
   struct extra_descr_data *desc;
   struct help_index_element *help;
#if defined(OASIS_MPROG)
   struct mob_prog_data *mprog;
   struct mob_prog_data *mprogl;
#endif
   struct trig_data *trig;
   int script_mode;
   int trigger_position;
   int item_type;
   struct trig_proto_list *script;
   char *storage; /* for holding commands etc.. */
   struct assembly_data *OlcAssembly;
};

struct olc_save_info {
  int zone;
  sbyte type;
  struct olc_save_info *next;
};


/*. Exported globals .*/
#ifdef _OASIS_OLC_
char *nrm, *grn, *cyn, *yel;
struct olc_save_info *olc_save_list = NULL;
#else
extern char *nrm, *grn, *cyn, *yel;
extern struct olc_save_info *olc_save_list;
#endif


/*. Descriptor access macros .*/
#define OLC_MODE(d) 	((d)->olc->mode) 	/*. Parse input mode	.*/
#define OLC_NUM(d) 	((d)->olc->number)	/*. Room/Obj VNUM 	.*/
#define OLC_VAL(d) 	((d)->olc->value)  	/*. Scratch variable	.*/
#define OLC_FUNC(d) 	((d)->olc->func_name)  	/*. SPEC func name	.*/
#define OLC_FUNCN(d) 	((d)->olc->func_num)  	/*. SPEC func number	.*/
#define OLC_ZNUM(d) 	((d)->olc->zone_num) 	/*. Real zone number	.*/
#define OLC_ROOM(d) 	((d)->olc->room)	/*. Room structure	.*/
#define OLC_OBJ(d) 	((d)->olc->obj)	  	/*. Object structure	.*/
#define OLC_ZONE(d)     ((d)->olc->zone)	/*. Zone structure	.*/
#define OLC_MOB(d)	((d)->olc->mob)	  	/*. Mob structure	.*/
#define OLC_SHOP(d) 	((d)->olc->shop)	/*. Shop structure	.*/
#define OLC_GUILD(d) 	((d)->olc->guild)	/*. Guild structure	.*/
#define OLC_DESC(d) 	((d)->olc->desc)	/*. Extra description	.*/
#define OLC_HELP(d)    ((d)->olc->help)        /* help entries         */
#define OLC_ASSEDIT(d)  ((d)->olc->OlcAssembly)   /* assembly olc 	*/

#ifdef OASIS_MPROG
#define OLC_MPROG(d)   ((d)->olc->mprog)       /* Temporary MobProg.   */
#define OLC_MPROGL(d)  ((d)->olc->mprogl)      /* MobProg list.        */
#define OLC_MTOTAL(d)  ((d)->olc->total_mprogs)/* Total mprog number.  */
#endif
#define OLC_TRIG(d)     ((d)->olc->trig)        /* Trigger structure.   */
#define OLC_STORAGE(d)  ((d)->olc->storage)    /* For command storage  */
 
/*. Other macros .*/

#define OLC_EXIT(d)	(OLC_ROOM(d)->dir_option[OLC_VAL(d)])
#define GET_OLC_ZONE(c,number) ((c)->player_specials->saved.olc_zone[(number)])

/*. Cleanup types .*/
#define CLEANUP_ALL		(byte)	1	/*. Free the whole lot  .*/
#define CLEANUP_STRUCTS 	(byte)	2	/*. Don't free strings  .*/
#define CLEANUP_NONE    	(byte)	3	/*. Don't free strings  .*/

/*. Add/Remove save list types	.*/
#define OLC_SAVE_ROOM		(byte)	0
#define OLC_SAVE_OBJ		(byte)	1
#define OLC_SAVE_ZONE		(byte)	2
#define OLC_SAVE_MOB		(byte)	3
#define OLC_SAVE_SHOP		(byte)	4
#define OLC_SAVE_GM		(byte)	5
#define OLC_SAVE_HELP		(byte)	6

/* Submodes of OEDIT connectedness */
#define OEDIT_MAIN_MENU              	1
#define OEDIT_EDIT_NAMELIST          	2
#define OEDIT_SHORTDESC              	3
#define OEDIT_LONGDESC               	4
#define OEDIT_ACTDESC                	5
#define OEDIT_TYPE                   	6
#define OEDIT_EXTRAS                 	7
#define OEDIT_WEAR                  	8
#define OEDIT_WEIGHT                	9
#define OEDIT_COST                  	10
#define OEDIT_COSTPERDAY            	11
#define OEDIT_TIMER                 	12
#define OEDIT_VALUE_1               	13
#define OEDIT_VALUE_2               	14
#define OEDIT_VALUE_3               	15
#define OEDIT_VALUE_4               	16
#define OEDIT_APPLY                 	17
#define OEDIT_APPLYMOD              	18
#define OEDIT_EXTRADESC_KEY         	19
#define OEDIT_CONFIRM_SAVEDB        	20
#define OEDIT_CONFIRM_SAVESTRING    	21
#define OEDIT_PROMPT_APPLY          	22
#define OEDIT_EXTRADESC_DESCRIPTION 	23
#define OEDIT_EXTRADESC_MENU        	24
#define OEDIT_LEVEL                 	25
#define OEDIT_VALUE_5			26
#define OEDIT_MATERIAL                  27  /* Ponder (04/02/1997) */
#define OEDIT_CDAM                      28
#define OEDIT_TDAM                      29
#define OEDIT_VALUE_6			30
#define OEDIT_VALUE_7			31
#define OEDIT_VALUE_8			32
#define OEDIT_ANTI                   	33
#define OEDIT_SPELL_AFF              	34
#define OEDIT_PROMPT_WPNSPL           	35
#define OEDIT_WPNSPL                    36
#define OEDIT_WPNSPL_LVL                37
#define OEDIT_WPNSPL_PCT                38
#define OEDIT_IMMUNE_APP                39
#define OEDIT_EXTRAS2                   40  /* Nomikos (05/16/2003) */


/* Submodes of REDIT connectedness */
#define REDIT_MAIN_MENU 		1
#define REDIT_NAME 			2
#define REDIT_DESC 			3
#define REDIT_FLAGS 			4
#define REDIT_SECTOR 			5
#define REDIT_EXIT_MENU 		6
#define REDIT_CONFIRM_SAVEDB 		7
#define REDIT_CONFIRM_SAVESTRING 	8
#define REDIT_EXIT_NUMBER 		9
#define REDIT_EXIT_DESCRIPTION 		10
#define REDIT_EXIT_KEYWORD 		11
#define REDIT_EXIT_KEY 			12
#define REDIT_EXIT_DOORFLAGS 		13
#define REDIT_EXTRADESC_MENU 		14
#define REDIT_EXTRADESC_KEY 		15
#define REDIT_EXTRADESC_DESCRIPTION 	16
/*
 * begin add - Bon 07/29/97
 */
#define REDIT_TELEPORT_MENU 		17
#define REDIT_TELEPORT_NUMBER 		18
#define REDIT_TELEPORT_DELAY 		19
#define REDIT_TELEPORT_FLAG  		20
#define REDIT_TELEPORT_OBJ   		21
#define REDIT_TELEPORT_TO_CHAR		22
#define REDIT_TELEPORT_TO_SOURCE        23
#define REDIT_TELEPORT_TO_TARG		24
#define REDIT_OREVAL          		25
#define REDIT_ORETYPE         		26
#define REDIT_OREPERCENT       		27
#define REDIT_ROOM2_FLAGS               28

/*
 * end   add - Bon 07/29/97
 */


/*. Submodes of ZEDIT connectedness 	.*/
#define ZEDIT_MAIN_MENU              	0
#define ZEDIT_DELETE_ENTRY		1
#define ZEDIT_NEW_ENTRY			2
#define ZEDIT_CHANGE_ENTRY		3
#define ZEDIT_COMMAND_TYPE		4
#define ZEDIT_IF_FLAG			5
#define ZEDIT_ARG1			6
#define ZEDIT_ARG2			7
#define ZEDIT_ARG3			8
#define ZEDIT_ARG4			9
#define ZEDIT_ZONE_NAME			10
#define ZEDIT_ZONE_LIFE			11
#define ZEDIT_ZONE_TOP			12
#define ZEDIT_ZONE_RESET		13
#define ZEDIT_CONFIRM_SAVESTRING	14
#define ZEDIT_ZONE_FLAGS		15
#define ZEDIT_ZONE_AUTHOR		16
#define ZEDIT_ZONE_EDITOR		17
#define ZEDIT_ZONE_LEVELS		18
#define ZEDIT_ZONE_SOURCE		19
#define ZEDIT_ZONE_STATUS		20
#define ZEDIT_SARG1       		21
#define ZEDIT_SARG2 		        22
#define ZEDIT_ZONE_CONTINENT		23
#define ZEDIT_ZONE_COMMENT		24
#define ZEDIT_WORLD_PROOF     25
#define ZEDIT_TRIG_PROOF      26
#define ZEDIT_OBJ_PROOF       27


/*. Submodes of MEDIT connectedness 	.*/
#define MEDIT_MAIN_MENU              	0
#define MEDIT_ALIAS			1
#define MEDIT_S_DESC			2
#define MEDIT_L_DESC			3
#define MEDIT_D_DESC			4
#define MEDIT_NPC_FLAGS			5
#define MEDIT_AFF_FLAGS			6
#define MEDIT_CONFIRM_SAVESTRING	7
#define MEDIT_SPEC			8
#define MEDIT_SPECVAL			9
#define MEDIT_NUMERICAL_RESPONSE	10
#define MEDIT_SEX			11
#define MEDIT_HITROLL			12
#define MEDIT_DAMROLL			13
#define MEDIT_NDD			14
#define MEDIT_SDD			15
#define MEDIT_NUM_HP_DICE		16
#define MEDIT_SIZE_HP_DICE		17
#define MEDIT_ADD_HP			18
#define MEDIT_AC			19
#define MEDIT_EXP			20
#define MEDIT_GOLD			21
#define MEDIT_POS			22
#define MEDIT_DEFAULT_POS		23
#define MEDIT_ATTACK			24
#define MEDIT_LEVEL			25
#define MEDIT_ALIGNMENT			26
#define MEDIT_CLASS			27
#define MEDIT_RACE			28
#define MEDIT_SPECVAL_1                 29
#define MEDIT_SPECVAL_2                 30
#define MEDIT_SPECVAL_3                 31
#define MEDIT_SPECVAL_4                 32
#define MEDIT_SPECVAL_5                 33
#define MEDIT_SPECVAL_6                 34
#define MEDIT_SPECVAL_7                 35
#define MEDIT_SPECVAL_8                 36
#define MEDIT_SPECVAL_9                 37

#if defined(OASIS_MPROG)
#define MEDIT_MPROG                     38
#define MEDIT_CHANGE_MPROG              39
#define MEDIT_MPROG_COMLIST             40
#define MEDIT_MPROG_ARGS                41
#define MEDIT_MPROG_TYPE                42
#define MEDIT_PURGE_MPROG               43
#endif
#define MEDIT_IMMUNE                    44
#define MEDIT_RESIST                    45
#define MEDIT_SUCCEPT                   46
#define MEDIT_HIDE                      47
#define MEDIT_NPC2_FLAGS 		          48

/*. Submodes of SEDIT connectedness 	.*/
#define SEDIT_MAIN_MENU              	0
#define SEDIT_CONFIRM_SAVESTRING	1
#define SEDIT_NOITEM1			2
#define SEDIT_NOITEM2			3
#define SEDIT_NOCASH1			4
#define SEDIT_NOCASH2			5
#define SEDIT_NOBUY			6
#define SEDIT_BUY			7
#define SEDIT_SELL			8
#define SEDIT_PRODUCTS_MENU		11
#define SEDIT_ROOMS_MENU		12
#define SEDIT_NAMELIST_MENU		13
#define SEDIT_NAMELIST			14
/*. Numerical responses .*/
#define SEDIT_NUMERICAL_RESPONSE	20
#define SEDIT_OPEN1			21
#define SEDIT_OPEN2			22
#define SEDIT_CLOSE1			23
#define SEDIT_CLOSE2			24
#define SEDIT_KEEPER			25
#define SEDIT_BUY_PROFIT		26
#define SEDIT_SELL_PROFIT		27
#define SEDIT_TYPE_MENU			29
#define SEDIT_DELETE_TYPE		30
#define SEDIT_DELETE_PRODUCT		31
#define SEDIT_NEW_PRODUCT		32
#define SEDIT_DELETE_ROOM		33
#define SEDIT_NEW_ROOM			34
#define SEDIT_SHOP_FLAGS		35
#define SEDIT_NOTRADE			36

/*. Submodes of GEDIT connectedness     . */
#define GEDIT_MAIN_MENU                 0
#define GEDIT_CONFIRM_SAVESTRING        1
#define GEDIT_NO_CASH                   2
#define GEDIT_NO_SKILL                  3
/*. Numerical responses . */
#define GEDIT_NUMERICAL_RESPONSE        5
#define GEDIT_CHARGE                    6
#define GEDIT_OPEN                      7
#define GEDIT_CLOSE                     8
#define GEDIT_TRAINER                   9
#define GEDIT_NO_TRAIN                 10
#define GEDIT_SELECT_SPELLS            11
#define GEDIT_SELECT_SKILLS            12
#define GEDIT_MINLEV                   13
#define GEDIT_MAXLEV                   14

/*. Submodes of HEDIT connectedness     . */
#define HEDIT_MAIN_MENU                0
#define HEDIT_ENTRY                    1
#define HEDIT_MIN_LEVEL                2
#define HEDIT_KEYWORDS                 3
#define HEDIT_CONFIRM_SAVESTRING       4

#define ASSEDIT_DO_NOT_USE              0
#define ASSEDIT_MAIN_MENU               1
#define ASSEDIT_ADD_COMPONENT           2
#define ASSEDIT_EDIT_COMPONENT          3
#define ASSEDIT_DELETE_COMPONENT        4
#define ASSEDIT_EDIT_EXTRACT            5
#define ASSEDIT_EDIT_INROOM             6
#define ASSEDIT_EDIT_TYPES              7

/*. Limit info .*/
#define MAX_ROOM_NAME	  75
#define MAX_MOB_NAME	  50
#define MAX_OBJ_NAME	  50
#define MAX_ROOM_DESC	  2048
#define MAX_EXIT_DESC	  512
#define MAX_EXTRA_DESC    1024
#define MAX_MOB_DESC	  1024
#define MAX_OBJ_DESC	  1024
#define MAX_TELE_STRING   155
#define MAX_HELP_KEYWORDS 75
#define MAX_HELP_ENTRY    7168

/* #define HEDIT_LIST          1 */    /* define to log saves          */
