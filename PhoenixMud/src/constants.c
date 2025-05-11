/* ************************************************************************
*   File: constants.c                                   Part of CircleMUD *
*  Usage: Numeric and string contants used by the MUD                     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "../localHeader/conf.h"
#include "../localHeader/sysdep.h"

#include "structs.h"
#include "interpreter.h"
#include "olc.h"
#include "vnum.h"

const char *circlemud_version = 
"PhoenixMUD  version 4.1                        5/06\r\n"
"PhoenixMUD  version 4.0  beta patchlevel 4     3/99\r\n"
"CircleMUD   version 3.00 beta patchlevel 15    3/99\r\n"
"DG Scripts  version 0.99 beta patchlevel 6    10/98\r\n"; 
#define DG_SCRIPT_VERSION 


/* strings corresponding to ordinals/bitvectors in structs.h ***********/


/* (Note: strings for class definitions in class.c instead of here) */


/* cardinal directions */
const char *dirs[] =
{
  "north",
  "east",
  "south",
  "west",
  "up",
  "down",
  "\n"
};

/* Corresponding armor names for ac numbers */
const char      *armor_types[] =
{
  "armored like a God",
  "armored like a tank",
  "exceptionally armored",
  "extremely armored",
  "very armored",
  "heavily armored",
  "armored",
  "somewhat armored",
  "lightly armored",
  "slightly armored",
  "barely armored",
  "naked",
  "\n"
};

const char *item_condition[] = {
   "&WINDESTRUCTABLE&n",
   "&MBroken&n",
   "&RBarely hanging on&n",
   "&RSeriously Damaged&n",
   "&MDamaged&n",
   "&MDented&n",
   "&YScratched&n",
   "&YModerate&n",
   "&CNicked&n",
   "&CFair&n",
   "Good",
   "Very Good",
   "Perfect",
   "/n"
};

const char* item_condition_no_color[] = {
   "INDESTRUCTABLE",
   "Broken",
   "Barely hanging on",
   "Seriously Damaged",
   "Damaged",
   "Dented",
   "Scratched",
   "Moderate",
   "Nicked",
   "Fair",
   "Good",
   "Very Good",
   "Perfect",
   "/n"
};

const char *item_wear[] = {
   "&WImpervious to Wear&n",
   "&RHorrible&n",
   "&RBarely Holding Together&n",
   "&MExtremely Worn&n",
   "&MVery Worn&n",
   "&YWorn&n",
   "&YSomewhat Worn&n",
   "&CNot Very Worn&n",
   "&CBroken In&n",
   "Almost New",
   "New",
   "Pristine",
   "\n"
};

const char* item_wear_no_color[] = {
   "Impervious to Wear",
   "Horrible",
   "Barely Holding Together",
   "Extremely Worn",
   "Very Worn",
   "Worn",
   "Somewhat Worn",
   "Not Very Worn",
   "Broken In",
   "Almost New",
   "New",
   "Pristine",
   "\n"
};

/* ROOM_x */
const char *room_bits[] = {
   "DARK",
   "DEATH",
   "!MOB",
   "INDOORS",
   "CLAN",
   "REGEN",
   "!TRACK",
   "!MAGIC",
   "TUNNEL",
   "PRIVATE",
   "GODROOM",
   "*",
   "NODECAY",
   "!RECALL",
   "!SUMMON",
   "PKILL",
   "PEACEFUL",
   "SOUNDPROOF",
   "HOUSE(R)",
   "HCRSH(R)",
   "ATRIUM(R)",
   "OLC(R)",
   "TRAVEL",
   "CAMP",
   "DONATION(R)",
   "!LEVI",
   "!FLY",
   "!MOUNT",
   "!CAMP",
   "MINE",
   "!WATERBR",
   "\n"
};

const char *room2_bits[] = {
   "SALTWATER_FISHING",
   "FRESHWATER_FISHING",
   "NEVER_MOB",
   "GRAFFITI",
   "PLAYER_SHOP",
   "\n"
};

const char *teleport_bits[] = {
   "FORCE_LOOK",
   "RESET_TIME",
   "RANDOM_TIME",
   "SPIN(UNUSED)",
   "HAS_OBJ",
   "NO_HAS_OBJ",
   "NO_MESSAGE",
   "SKIP_MOB",
   "SKIP_OBJ",
   "SKIP_PLAYER",
   "\n"
};


/* ZONE_x */
const char *zone_bits[] = {
   "IDLE (RESERVED)",
   "!RECALL",
   "!SUMMON",
   "!TRACK",
   "!TELEPORT",
   "QUEUED (RESERVED)",
   "PKILL",
   "GRAFFITI",
   "\n"
};

/* ZONE Status */
const char *zone_status[] = {
   "In_Progress",
   "Waiting_to_be_Proofed",
   "On_Hold",
   "Finished",
   "Active",
   "\n"
};

/* ZONE Source */
const char *zone_source[] = {
   "Original",
   "Stock",
   "Re-Write",
   "Public",
   "\n"
};

const char *zone_continent[][2] = {
   {"Unassigned","&G"},
   {"Caledon","&c"},
   {"Aglaron","&R"},
   {"UnderDark","&Y"},
   {"Ocean","&M"},
   {"Arctic","&w"},
   {"Other/God","&b"},
   {"Talam","&W"},
   {"\n","\n"}
};

/* EX_x */
const char *exit_bits[] = {
  "DOOR",
  "PICKPROOF",
  "CLOSED",
  "LOCKED",
  "SECRET",
  "HIDDEN",
  "FLY",
  "DROP",
  "AUTOCLOSE",
  "WIZLOCKED",
  "NOPASS",
  "\n"
};


/* SECT_ */
const char *sector_types[] = {
  "Inside",
  "City",
  "Field",
  "Forest",
  "Hills",
  "Mountains",
  "Water (Swim)",
  "Water (No Swim)",
  "Underwater",
  "In Flight",
  "Desert",
  "Swamp",
  "Tundra",
  "Jungle",
  "Road",
  "\n"
};

int movement_loss[] =
{
  1,	/* Inside     */
  1,	/* City       */
  2,	/* Field      */
  3,	/* Forest     */
  4,	/* Hills      */
  6,	/* Mountains  */
  4,	/* Swimming   */
  1,	/* Unswimable */
  5,  /* Underwater */
  1,	/* Flying     */
  4,	/* Desert     */
  4,	/* Swamp      */
  3, 	/* Tundra     */
  4, 	/* Jungle     */
  2   /* Road       */
};


/* SEX_x */
const char *genders[] =
{
  "neutral",
  "male",
  "female"
};

const char *wound_types[] = {
"R.I.P.",
"DYING!!",
"BLEEDING!",
"V. Hurt",
"Hurt",
"Wounded",
"Poor",
"Fair",
"Average",
"Good",
"V. Good",
"Excellent",
"\n"
};

/* POS_x */
const char *position_types[] = {
  "dead",
  "mortally wounded",
  "incapacitated",
  "stunned",
  "sleeping",
  "chanting",	/* 10/27/96, Echo */
  "meditating",	/* 10/27/96, Echo */
  "bandaged",	/* 10/27/96, Echo */
  "resting",
  "sitting",
  "fighting",
  "standing",
  "\n"
};

const char *char_size[] = {
   "UNSET",
   "Huge",
   "Large",
   "Normal",
   "Small",
   "Tiny",
   "\n"
};

const int item_size_limits[][2] = { /* in inches */
   { 0,0},
   { 85,999999999},		/* Huge 7'1"-infinity ft*/
   { 61,108},			/* Large 5'1"-9ft*/
   { 49,84},			/* Normal 4'1"-7ft */
   { 25,72},			/* Small 2'1"-6ft*/
   { 0,48},			/* Tiny 0-4ft*/
   {0,0}			/* end */
};


/* PLR_x */
const char *player_bits[] = {
  "KILLER",
  "THIEF",
  "FROZEN",
  "DONTSET",
  "WRITING",
  "MAILING",
  "CSH",
  "SITEOK",
  "NOSHOUT",
  "NOTITLE",
  "DELETED",
  "LOADRM",
  "!WIZL",
  "!DEL",
  "INVST",
  "CRYO",
  "LINKLOADED",
  "REIMBED",
  "STUNNED",
  "FISHING",
  "FISH_ON",
  "PK",
  "!COMMUNE",
  "\n"
};


/* MOB_x */
const char *action_bits[] = {
  "SPEC(R)",
  "SENTINEL",
  "SCAVENGER",
  "ISNPC",
  "AWARE",
  "AGGR",
  "STAY-ZONE",
  "WIMPY",
  "AGGR_EVIL",
  "AGGR_GOOD",
  "AGGR_NEUTRAL",
  "MEMORY",
  "HELPER",
  "HUNT_KILLER",
  "HUNT_MEMORY",
  "!CHARM",
  "!SUMMN",
  "!SLEEP",
  "!BASH",
  "!BLIND",
  "MOUNT",
  "!GIVE",
  "CITIZEN",
  "HAPPY",
  "SAD",
  "!MOOD",
  "PTH(R)",
  "GUARD(R)",
  "FLY(R)",
  "FOOLHARDY",
  "STAY-TERRAIN",
  "\n"
};

const char *action2_bits[] = {
   "!TRIP",
   "!STUN",
   "!SWEEP",
   "COMPONENT(R)",
   "!DISARM",
   "SUMMONABLE",
   "\n"
};

/* PRF_x */
const char *preference_bits[] = {
  "BRIEF",
  "COMPACT",
  "DEAF",
  "!TELL",
  "D_HP",
  "D_MANA",
  "D_MOVE",
  "AUTOEX",
  "!HASS",
  "QUEST",
  "SUMN",
  "!REP",
  "LIGHT",
  "C1",
  "C2",
  "!WIZ",
  "L1",
  "L2",
  "!AUC",
  "!GOS",
  "!GTZ",
  "RMFLG",
  "INFOBAR",  /* -naj infobar2 12/16/96 - preference bit description */
  "SCOREBAR",  /* -naj infobar2 12/16/96 - preference bit description */
  "METER",  /* -naj infobar2 12/16/96 - preference bit description */
  "ASCII",  /* -naj infobar2 12/16/96 - preference bit description */
  "AUTOSPLIT", /* autosplit/loot code from snippets page --Erika */
  "AUTOLOOT",
  /*
   * begin add - Bon 07/18/97
   */
  "!BATTLE",
  "AUTOGOLD",
  "AUTOSAC",
  "AUTOASSIST",
  /*
   * end   add - Bon 07/18/97
   */
  "\n"
};
/* PRF2_x */
const char *preference2_bits[] = {
   "D_GOLD",
   "D_EXP",
   "D_ALIGN",
   "D_MAX",
   "AFK",
   " ",
   "!OOC",
   "!REMORT",
   "!CSAY",
   "E_MPROG",
   "T_MPROG",
   "PAGE_OK",
   "!INFO",
   "!FSPAM",
   "DGATCH",
   "HEDIT",
   "RECL",
   "D_TIME",
   "D_DATE",
   "!MUS",
   "MORTAL",
   "NO_NEWBIE",
   "\n"
};


/* AFF_x */
const char *affected_bits[] =
{
  "BLIND",
  "INVIS",
  "!TRACK",
  "DET-INVIS",
  "DET-MAGIC",
  "SENSE-LIFE",
  "RAGE",
  "SANCT",
  "GROUP",
  "CURSE",
  "TAMED",
  "POISON",
  "PROT-EVIL",
  "PARALYSIS",
  "WATERWALK",
  "PASSDOOR",
  "SLEEP",
  "NO_FLEE",
  "SNEAK",
  "HIDE",
  "PROT-GOOD",
  "CHARM",
  "FOLLOW",
  "FLY",
  "INFRA",
  "HASTE",
  "SLOW",
  "DET-ALIGN",
  "LEVITATE",
  "DREAM",
  "PLAGUE",
  "WATERBREATH",  
  "\n"
};
/* AFF_x */
const char *affected2_bits[] =
{
  "FLYING",  
  "DIGGING",
  "RST-BLIND",
  "SKINNING",
  "FIRESHIELD",
  "SHADOW",
  "ROVE",
  "WARY",
  "",
  "PICKING_STAY",
  "PICKING",
  "\n"
};

const char *room_affect_bits[] = {
  "Fog",
  "Heat",
  "Snow",
  "Flowers",
  "Leaves"
  "\n"
};

/* CON_x */
const char *connected_types[] = {
  "Playing",
  "Disconnecting",
  "Get name",
  "Confirm name",
  "Get password",
  "Get new PW",
  "Confirm new PW",
  "Select sex",
  "Select race",
  "Select class",
  "Reading MOTD",
  "Main Menu",
  "Get descript.",
  "Changing PW 1",
  "Changing PW 2",
  "Changing PW 3",
  "Self-Delete 1",
  "Self-Delete 2",
  "Object edit",
  "Room edit",
  "Zone edit",
  "Mobile edit",
  "Shop edit",
  "Guild edit",
  "Path edit",
  "Ident conning",
  "Ident conned",
  "Ident reading",
  "Ident read",
  "Asking name",
  "Select Stats",
  "Disconnecting",
  "Text Edit",
  "Trigger Edit",
  "Help Edit",
  "Assembly edit",
  "News edit",
  "Ask first time",
  "Select hometown",
  "Changing E-mail",
  "ERROR",
  "\n"
};


/* WEAR_x - for eq list */
char *where[] = {
   "<worn on unused!?!?> ",
   "<worn on finger>     ",
   "<worn on finger>     ",
   "<worn around neck>   ",
   "<worn around neck>   ",
   "<worn on body>       ",
   "<worn on head>       ",
   "<worn on legs>       ",
   "<worn on feet>       ",
   "<worn on hands>      ",
   "<worn on arms>       ",
   "<worn as shield>     ",
   "<worn about body>    ",
   "<worn about waist>   ",
   "<worn around wrist>  ",
   "<worn around wrist>  ",
   "<wielded>            ",
   "<wielded (off hand)> ",
   "<held>               ",
   "<held>               ",
   "<worn on ear>        ",	/* New EQ positions--Aleks */
   "<worn on ear>        ",	/* New EQ positions--Aleks */
   "<worn on face>       ",	/* New EQ positions--Aleks */
   "<worn on back>       ",	/* New EQ positions--Aleks */
   "<worn on heart>      "      /* New EQ position--Faron  */
};


/* WEAR_x - for stat */
const char *equipment_types[] = {
   "Worn on unused !?!?",
   "Worn on right finger",
   "Worn on left finger",
   "First worn around Neck",
   "Second worn around Neck",
   "Worn on body",
   "Worn on head",
   "Worn on legs",
   "Worn on feet",
   "Worn on hands",
   "Worn on arms",
   "Worn as shield",
   "Worn about body",
   "Worn around waist",
   "Worn around right wrist",
   "Worn around left wrist",
   "Wielded",
   "Wielded (off hand)",
   "Held",
   "Held",
   "Worn on left ear",		/* New EQ positions--Aleks */
   "Worn on right ear",		/* New EQ positions--Aleks */
   "Worn on face",		/* New EQ positions--Aleks */
   "Worn on back",		/* New EQ positions--Aleks */
   "Heartworn",                 /* New EQ position--Faron  */
   "\n"
};

const int wear_check[]=
{
   ITEM_WEAR_TAKE,
   ITEM_WEAR_FINGER,
   ITEM_WEAR_FINGER,
   ITEM_WEAR_NECK,
   ITEM_WEAR_NECK,
   ITEM_WEAR_BODY,
   ITEM_WEAR_HEAD,
   ITEM_WEAR_LEGS,
   ITEM_WEAR_FEET,
   ITEM_WEAR_HANDS,
   ITEM_WEAR_ARMS,
   ITEM_WEAR_SHIELD,
   ITEM_WEAR_ABOUT,
   ITEM_WEAR_WAIST,
   ITEM_WEAR_WRIST,
   ITEM_WEAR_WRIST,
   ITEM_WEAR_WIELD,
   ITEM_WEAR_WIELD,
   ITEM_WEAR_HOLD,
   ITEM_WEAR_HOLD,
   ITEM_WEAR_EAR,
   ITEM_WEAR_EAR,
   ITEM_WEAR_FACE,
   ITEM_WEAR_BACK,
   ITEM_WEAR_HEART
};
/*  imm|res|nrm|succ */
const struct obj_imm_type obj_immunity[]=
{
   {  1,  2,  4,  8},		/* fire */
   {  1,  2,  4,  8},		/* cold */
   {  1,  2,  4,  8},		/* elec */
   {  1,  2,  4,  8},		/* energy */
   {  0,  1,  2,  4},		/* blunt */
   {  0,  1,  2,  4},		/* pierce */
   {  0,  1,  2,  4},		/* slash */
   {  3,  6, 12, 24},		/* acid */
   {  1,  2,  4,  8},		/* poison */
   {  1,  2,  4,  8},		/* drain */
   {  0,  0,  0,  0},		/* sleep */
   {  0,  0,  0,  0},		/* charm */
   {  0,  0,  0,  0},		/* hold */
   {  0,  0,  0,  0},		/* nomag */
   {  0,  0,  0,  0},		/* +1 */
   {  0,  0,  0,  0},		/* +2 */
   {  0,  0,  0,  0},		/* +3 */
   {  0,  0,  0,  0},		/* +4 */
   {  0,  0,  0,  0},		/* stun */
   {  1,  2,  4,  8},		/* holy */
   {  1,  2,  4,  8}		/* unholy */
};


const float wear_dam_adjust[]=
{
   1.0,/*  TAKE */
   0.8,/*  FINGER */
   0.8,/*  FINGER */
   1.1,/*  NECK */
   1.1,/*  NECK */
   3.0,/*  BODY */
   1.5,/*  HEAD */
   2.0,/*  LEGS */
   1.2,/*  FEET */
   1.5,/*  HANDS */
   2.0,/*  ARMS */
   3.0,/*  SHIELD */
   1.5,/*  ABOUT */
   1.2,/*  WAIST */
   1.2,/*  WRIST */
   1.2,/*  WRIST */
   3.0,/*  WIELD */
   3.0,/*  WIELD */
   0.9,/*  HOLD */
   0.9,/*  HOLD */
   0.6,/*  EAR */
   0.6,/*  EAR */
   0.8,/*  FACE */
   0.7,/*  BACK */
   0.1/*  HEART */
};

const int hand_position[NUM_HAND_POSITIONS] = 
{
   WEAR_SHIELD,
   WEAR_WIELD_1,
   WEAR_WIELD_2,
   WEAR_HOLD_1,
   WEAR_HOLD_2
};

const char *remort_level_types[] = {
  "Non",
  "Single",
  "Double",
  "Triple",   /* hey! it might happen =) */
  "\n"
};

/* ITEM_x (ordinal object types) */
const char *item_types[] = {
  "UNDEFINED",
  "LIGHT",
  "SCROLL",
  "WAND",
  "STAFF",
  "WEAPON",
  "UNUSED",
  "UNUSED",
  "TREASURE",
  "ARMOR",
  "POTION",
  "WORN",
  "OTHER",
  "TRASH",
  "TRAP",
  "CONTAINER",
  "NOTE",
  "LIQ CONTAINER",
  "KEY",
  "FOOD",
  "MONEY",
  "PEN",
  "BOAT",
  "FOUNTAIN",
  "FUEL",	/* Refuelable light mod--Aleks */
  "PILL",       /* Pill modification--Aleks */
  "THROW",
  "GRENADE",
  "BOW",
  "SLING",
  "CROSSBOW",
  "BOLT",
  "ARROW",
  "ROCK",
  "PORTAL",
  "FURNITURE",
  "TICKET",
  "STABLE TICKET",
  "SHOVEL",
  "FISHING POLE",
  "\n"
};

/* Refuelable light mod--Aleks */
/* Fuel types */
const char *fuels[] =
{
  "none",
  "oil",
  "coal",
  "fat",
  "wood",
  "\n"
};


/* ITEM_WEAR (wear bitvector) */
const char *wear_bits[] = {
  "TAKE",
  "FINGER",
  "NECK",
  "BODY",
  "HEAD",
  "LEGS",
  "FEET",
  "HANDS",
  "ARMS",
  "SHIELD",
  "ABOUT",
  "WAIST",
  "WRIST",
  "WIELD",
  "HOLD",
  "UNUSED",		/* Placeholder */
  "UNUSED",		/* Placeholder */
  "EAR",		/* New EQ positions--Aleks */
  "FACE",		/* New EQ positions--Aleks */
  "BACK",		/* New EQ positions--Aleks */
  "HEART",              /* New EQ position--Faron  */
  "\n"
};

/* ITEM_WEAR (wear bitvector) */
const char *wear_strings[] = {
  "",
  "on your finger",
  "around your neck",
  "on your body",
  "on your head",
  "on your legs",
  "on your feet",
  "on your hands",
  "on your arms",
  "as a shield",
  "about your torso",
  "around your waist",
  "around your wrist",
  "as a weapon",
  "in your hand",
  "thrown",		/* Placeholder */
  "as a light",		/* Placeholder */
  "through your ear",	/* New EQ positions--Aleks */
  "on your face",	/* New EQ positions--Aleks */
  "on your back",	/* New EQ positions--Aleks */
  "on your heart",      /* New EQ position--Faron  */
  "\n"
};

/* Ponder (04/02/1997) support for material types on objects */
/* MATERIAL_x (materials */
const char *material_types[] = {
  "UNDEFINED",
  "SKIN",
  "FUR",
  "IVORY",
  "CLOTH",
  "BONE",
  "STONE",                                                                
  "PAPER",
  "WOOD",
  "GLASS",
  "COPPER",
  "IRON",
  "BRONZE",
  "STEEL",
  "ADAMANTITE",
  "GOLD",                             
  "SILVER",
  "PLATINUM",
  "DIAMOND",
  "RUBY",
  "SAPPHIRE",
  "EMERALD",
  "PEARL",
  "LIQUID",
  "ETHER",                                                                 
  "AIR",
  "FIRE",
  "FOOD",
  "LEATHER",
  "PLANT",
  "TITANIUM",
  "WAX",
  "CARCASS",
  "MITHRIL",
  "FEATHER",
  "DRAGONSCALE",
  "ICE",
  "TIN",
  "BRASS",
  "HEMP",
  "ORE",
  "MINERAL",
  "\n"
};
const char *material_types_lower[] = {
  "Undefined",
  "Skin",
  "Fur",
  "Ivory",
  "Cloth",
  "Bone",
  "Stone",                                                                
  "Paper",
  "Wood",
  "Glass",
  "Copper",
  "Iron",
  "Bronze",
  "Steel",
  "Adamantite",
  "Gold", 
  "Silver",
  "Platinum",
  "Diamond",
  "Ruby",
  "Sapphire",
  "Emerald",
  "Pearl",
  "Liquid",
  "Ether",
  "Air",
  "Fire",
  "Food",
  "Leather",
  "Plant",
  "Titanium",
  "Wax",
  "Carcass",
  "Mithril",
  "Feather",
  "Dragonscale",
  "Ice",
  "Tin",
  "Brass",
  "Hemp",
  "Ore",
  "Mineral",
  "\n"
};

/* $/lb bulk/lb succ resist class race dam_slots */
const struct obj_material_affs material_affs[] = {
   {1,  1,      0,   0,     0,    0,   1},   /* UNDEFINED  */
   {1,  1,      0,   0,     0,    0,   40},  /* SKIN       */
   {1,  1,      0,   0,     0,    0,   40},  /* FUR        */
   {1,  1,      0,   0,     0,    0,   65},  /* IVORY      */
   {1,  1,      0,   0,     0,    0,   40},  /* CLOTH      */
   {1,  1,      0,   0,     0,    0,   50},  /* BONE       */
   {1,  1,      0,   0,     0,    0,   80},  /* STONE      */
   {1,  1,      0,   0,     0,    0,   40},  /* PAPER      */
   {1,  1,      0,   0,     0,    0,   40},  /* WOOD       */
   {1,  1,      0,   0,     0,    0,   40},  /* GLASS      */
   {1,  1,      0,   0,     0,    0,   70},  /* COPPER     */
   {1,  1,      0,   0,     0,    0,   80},  /* IRON       */
   {1,  1,      0,   0,     0,    0,   60},  /* BRONZE     */
   {1,  1,      0,   0,     0,    0,   100}, /* STEEL      */
   {1,  1,      0,   0,     0,    0,   175}, /* ADAMANTITE */
   {1,  1,      0,   0,     0,    0,   75},  /* GOLD       */
   {1,  1,      0,   0,     0,    0,   90},  /* SILVER     */
   {1,  1,      0,   0,     0,    0,   95},  /* PLATINUM   */
   {1,  1,      0,   0,     0,    0,   150}, /* DIAMOND    */
   {1,  1,      0,   0,     0,    0,   110}, /* RUBY       */
   {1,  1,      0,   0,     0,    0,   90},  /* SAPPHIRE   */
   {1,  1,      0,   0,     0,    0,   80},  /* EMERALD    */
   {1,  1,      0,   0,     0,    0,   60},  /* PEARL      */
   {1,  1,      0,   0,     0,    0,   50},  /* LIQUID     */
   {1,  1,      0,   0,     0,    0,   125}, /* ETHER      */
   {1,  1,      0,   0,     0,    0,   125}, /* AIR        */
   {1,  1,      0,   0,     0,    0,   125}, /* FIRE       */
   {1,  1,      0,   0,     0,    0,   40},  /* FOOD       */
   {1,  1,      0,   0,     0,    0,   40},  /* LEATHER    */
   {1,  1,      0,   0,     0,    0,   40},  /* PLANT      */
   {1,  1,      0,   0,     0,    0,   150}, /* TITANIUM   */
   {1,  1,      0,   0,     0,    0,   40},  /* WAX        */
   {1,  1,      0,   0,     0,    0,   40},  /* CARCASS    */
   {1,  1,      0,   0,     0,    0,   160}, /* MITHRIL    */
   {1,  1,      0,   0,     0,    0,   40},  /* FEATHER    */
   {1,  1,      0,   0,     0,    0,   140}, /* DRAGONSCALE*/
   {1,  1,      0,   0,     0,    0,   40},  /* ICE        */
   {1,  1,      0,   0,     0,    0,   45},  /* TIN        */
   {1,  1,      0,   0,     0,    0,   65},  /* BRASS      */
   {1,  1,      0,   0,     0,    0,   40},  /* HEMP       */
   {1,  1,      0,   0,     0,    0,   125}, /* ORE        */
   {1,  1,      0,   0,     0,    0,   125}, /* MINERAL    */
   {-1,-1,     -1,  -1,    -1,   -1,  -1},   /* Terminator */
   
};

/* Constants for Assemblies */
const char *AssemblyTypes[] = {
  "assemble",
  "bake",
  "brew",
  "craft",
  "fletch",
  "knit",
  "make",
  "mix",
  "thatch",
  "weave",
   "\n"
 };

/* ITEM_x (extra bits) */
/* Updated to match Phoenix--Aleks */
const char *extra_bits[] = {
  "GLOW",
  "HUM",
  "DARK",
  "LIVE_GRENADE",
  "NEWBIE",
  "INVISIBLE",
  "MAGIC",
  "!DROP",
  "BLESS",
  "!GOOD",
  "!EVIL",
  "!NEUTRAL",
  "!RENT",
  "!DONATE",
  "!INVIS",
  "!DECAY",
  "TWOHAND",
  "!POS_CHK",
  "!SELL",
  "!AUC",
  "BRITTLE_DAM",
  "RESIT_DAM",
  "DO_ACT",
  "!REPAIR",
  "DONATED",
  "BATTLE_ITEM",
  "PC_CORPSE(R)",
  "NPC_CORPSE(R)",
  "UNIQUE(R)",
  "SUN_DAMAGE",
  "*UNUSED13*",
  "QUEST(R)",
  "\n"
};

/* ITEM2_x (extra bits) */
const char *extra_bits2[] = {
  "REMORT",
  "DBL_REMORT",
  "BODY_PART",
  "!LOCATE",
  "\n"
};

/* ITEM2_x (extra bits) */
const char *extra_bits2_id[] = {
  "Remort",
  "Double Remort",
  "",                           /* body part */
  "No-Locate",
  "\n"
};

/* ITEM_x (extra bits) */
/* Updated to match Phoenix--Aleks */
const char *extra_bits_id[] = {
  "Glowing",
  "Humming",
  "Dark",
  "",				/* live_grenade */
  "Newbie",
  "Invisible",
  "Magic",
  "Cursed",			/* no-drop */
  "Blessed",
  "Anti-Good",
  "Anti-Evil",
  "Anti-Neutral",
  "No-Rent",
  "No-Donate",
  "No-Invis",
  "No-Decay",
  "Two-Handed",
  "",				/* no_pos_check */
  "No-Sell",
  "No-Auction",
  "Fragile",
  "Tough",
  "",				/* do_act */
  "",				/* !repear */
  "",				/* donated */
  "",				/* battle item */
  "",				/* pc-corpse */
  "",				/* npc corpse */
  "",				/* unique */
  "UnderDark",			/* sun-damage */
  "",
  "Quest",
  "\n"
};

/* ITEM_ANTI_x (anti class/race bits) */
const char *anti_bits[] = {
   "!WARRIOR",
   "!CLERIC",
   "!THIEF",
   "!MAGE",
   "!RANGER",
   "!BARD",
   "!MONK",
   "!*UNUSED*",
   "!BARBARIAN",
   "!PALADIN",
   "!ANTI-PALADIN",
   "!DRUID",
   "!MERCHANT",
   "!KENSAI",
   "!ASSASSIN",
   "!NECROMANCER",
   "!DEVA",
   "!HUMAN",
   "!ELF",
   "!HALF-ELF",
   "!DARK-ELF",
   "!DWARF",
   "!HALFLING",
   "!SPRITE",
   "!MINOTAUR",
   "!AVIAN",
   "!HALF-OGRE",
   "!HALF-ORC",
   "!DRACONIAN",
   "!SHADOW",
   "!TITAN",
   "!AESIR",
   "\n"
};
/* ITEM_ANTI_x (anti class/race bits) */
const char *anti_bits_id[] = {
   "Anti-Warrior",
   "Anti-Cleric",
   "Anti-Thief",
   "Anti-Mage",
   "Anti-Ranger",
   "Anti-Bard",
   "Anti-Monk",
   "",				/* unused */
   "Anti-Barbarian",
   "Anti-Paladin",
   "Anti-Anti-Paladin",
   "Anti-Druid",
   "Anti-Merchant",
   "Anti-Kensai",
   "Anti-Assassin",
   "Anti-Necromancer",
   "Anti-Deva",
   "Anti-Human",
   "Anti-Elf",
   "Anti-Half-Elf",
   "Anti-Dark-Elf",
   "Anti-Dwarf",
   "Anti-Halfling",
   "Anti-Sprite",
   "Anti-Minotaur",
   "Anti-Avian",
   "Anti-Half-Ogre",
   "Anti-Half-Orc",
   "Anti-Draconian",
   "Anti-Shadow",
   "Anti-Titan",
   "Anti-Aesir",
   "\n"
};


const char *immunity_names[] =
{
   "FIRE",
   "COLD",
   "ELECTRICITY",
   "ENERGY",
   "BLUNT",
   "PIERCE",
   "SLASH",
   "ACID",
   "POISON",
   "DRAIN",
   "SLEEP",
   "CHARM",
   "HOLD",
   "NON-MAGIC",
   "+1",
   "+2",
   "+3",
   "+4",
   "STUN",
   "HOLY",
   "UNHOLY",
   "\n"
};


/* APPLY_x */
const char *apply_types[] = {
  "NONE",
  "STR",
  "DEX",
  "INT",
  "WIS",
  "CON",
  "SEX",
  "UNUSED",
  "UNUSED",
  "AGE",
  "CHAR_WEIGHT",
  "CHAR_HEIGHT",
  "MAXMANA",
  "MAXHIT",
  "MAXMOVE",
  "UNUSED",
  "UNUSED",
  "ARMOR",
  "HITROLL",
  "DAMROLL",
  "SAVING_PARA",
  "SAVING_ROD",
  "SAVING_PETRI",
  "SAVING_BREATH",
  "SAVING_SPELL",
  "CHA",
  "LIGHT",
  "IMMUNE",
  "RESIST",
  "SUSCEPT",
  "FLY",
  "SPELL_FAIL",
  "APPLY_AFF2(R)",
  "APPLY_AFF3(R)",
  "APPLY_EAT_SPELL(R)",
  "\n"
};


/* CONT_x */
const char *container_bits[] = {
  "CLOSEABLE",
  "PICKPROOF",
  "CLOSED",
  "LOCKED",
  "\n",
};


/* LIQ_x */
const char *drinks[] =
{
  "water",
  "beer",
  "wine",
  "ale",
  "dark ale",
  "whisky",
  "lemonade",
  "firebreather",
  "local speciality",
  "slime mold juice",
  "milk",
  "tea",
  "coffee",
  "blood",
  "salt water",
  "clear water",
  "broth",
  "\n"
};


/* other constants for liquids ******************************************/


/* one-word alias for each drink */
const char *drinknames[] =
{
  "water",
  "beer",
  "wine",
  "ale",
  "ale",
  "whisky",
  "lemonade",
  "firebreather",
  "local",
  "juice",
  "milk",
  "tea",
  "coffee",
  "blood",
  "salt",
  "water",
  "broth",
  "\n"
};


/* effect of drinks on hunger, thirst, and drunkenness -- see values.doc */
int drink_aff[][3] = {
  {0, 1, 10},
  {3, 2, 5},
  {5, 2, 5},
  {2, 2, 5},
  {1, 2, 5},
  {6, 1, 4},
  {0, 1, 8},
  {10, 0, 0},
  {3, 3, 3},
  {0, 4, -8},
  {0, 3, 6},
  {0, 1, 6},
  {0, 1, 6},
  {0, 2, -1},
  {0, 1, -2},
  {0, 0, 13},
  {0, 3, 6},
  {-1,-1,-1}
};


/* color of the various drinks */
const char *color_liquid[] =
{
  "clear",
  "brown",
  "clear",
  "brown",
  "dark",
  "golden",
  "red",
  "green",
  "clear",
  "light green",
  "white",
  "brown",
  "black",
  "red",
  "clear",
  "crystal clear",
  "yellow",
  "\n"
};


/* level of fullness for drink containers */
const char *fullness[] =
{
  "less than half ",
  "about half ",
  "more than half ",
  ""
};


/* Ores and their vnums */
const struct obj_ore_types ore_types[] = {
   {"Dirt     ", MINE_DIRT},
   {"Iron     ", MINE_IRON},
   {"Copper   ", MINE_COPPER},
   {"Silver   ", MINE_SILVER},
   {"Gold     ", MINE_GOLD},
   {"Soft Coal", MINE_SOFTCOAL},
   {"Mithril  ", MINE_MITHRIL},
   {"Platinum ", MINE_PLATINUM},
   {"Titanium ", MINE_TITANIUM},
   {"Tin      ", MINE_TIN},
   {"Opal     ", MINE_OPAL},
   {"Diamond  ", MINE_DIAMOND},
   {"Ruby     ", MINE_RUBY},
   {"Emerald  ", MINE_EMERALD},
   {"Topaz    ", MINE_TOPAZ},
   {"Jade     ", MINE_JADE},
   {"Garnet   ", MINE_GARNET},
   {"Sapphire ", MINE_SAPPHIRE},
   {"Amethyst ", MINE_AMETHYST},
   {"Quartz   ", MINE_QUARTZ},
   {"Fire Opal", MINE_FIRE_OPAL},
   {"",NOTHING}			/* TERMINATOR */
};

/* str, int, wis, dex, con applies **************************************/
const char *str_strings[] =
{
   "Frail",
   "Feeble",
   "Weak",
   "Average",
   "Muscled",
   "Strong",
   "Very Strong",
   "Extremely Strong",
   "Buff",
   "Rippling with Muscle"
};

const char *dex_strings[] =
{
   "Almost Immobile",
   "Clumsy",
   "Ungainly",
   "Inept",
   "Average",
   "Above Average",
   "Limber",
   "Agile",
   "Lithe",
   "Lissome"
};

const char *con_strings[] =
{
   "Fragile",
   "Frail",
   "Sickly",
   "Weakly",
   "Average",
   "Hearty",
   "Hale",
   "Robust",
   "Strapping",
   "Stalwart"
};

const char *int_strings[] =
{
   "Huh?",
   "Dull",
   "Half-witted",
   "Simple",
   "Average",
   "Above Average",
   "Shrewd",
   "Intellectual",
   "Astute",
   "Omniscient"
};

const char *wis_strings[] =
{
   "Vapid",
   "Witless",
   "Short Sighted",
   "Unwise",
   "Average",
   "Above Average",
   "Learned",
   "Rational",
   "Insightful",
   "Sagacious"
};

const char *cha_strings[] =
{
   "Loathsome",
   "Fugly",
   "Aversive",
   "Awkward",
   "Average",
   "Pleasant",
   "Appealing",
   "Stunning",
   "Entrancing",
   "Dead Sexy"
};

/* [ch] strength apply (all) */
const struct str_app_type str_app[] = {
  {-27, -5, 0, 0},	/* str = 0 */
  {-24, -4, 3, 1},	/* str = 1 */
  {-21, -3, 3, 2},
  {-18, -3, 10, 3},
  {-15, -2, 25, 4},
  {-12, -2, 55, 5},	/* str = 5 */
  {-9, -1, 80, 6},
  {-6, -1, 90, 7},
  {-3, -1, 110, 8},
  {0, 0, 120, 9},
  {0, 0, 135, 10},	/* str = 10 */
  {0, 0, 145, 11},
  {0, 0, 160, 12},
  {0, 0, 200, 13},
  {0, 0, 250, 14},
  {0, 0, 300, 15},	/* str = 15 */
  {1, 1, 350, 16},
  {2, 1, 390, 18},
  {3, 2, 430, 20},	/* str = 18 */
  {4, 3, 470, 22},
  {5, 3, 500, 24},		/* 20 */
  {6, 4, 530, 26},
  {7, 5, 560, 28},
  {8, 6, 600, 30},
  {10, 7, 640, 35},
  {12, 7, 700, 40},		/* 25 */
  {15, 8, 810, 40},
  {18,  8, 970, 40},
  {21,  9, 1030, 40},
  {25,  9, 1140, 40},
  {29, 10, 1240, 40},		/* 30 */
  {29, 10, 1750, 40},
  {29, 11, 1750, 40},
  {29, 10, 1750, 40},
  {29, 11, 1750, 40},
  {29, 11, 1750, 40},		/* 35 */
  {29, 12, 1750, 40},
  {29, 12, 1750, 40},
  {29, 13, 1750, 40},
  {29, 13, 1750, 40},
  {29, 14, 1750, 40},		/* 40 */
  {29, 14, 1750, 40},
  {29, 15, 1750, 40},
  {29, 15, 1750, 40},
  {29, 16, 1750, 40},
  {29, 16, 1750, 40},		/* 45 */
  {29, 17, 1750, 40},
  {29, 17, 1750, 40},
  {29, 18, 1750, 40},
  {29, 18, 1750, 40},
  {29, 19, 1750, 40}	
};



/* [dex] skill apply (thieves only) */
const struct dex_skill_type dex_app_skill[] = {
  {-99, -99, -90, -99, -60},	/* dex = 0 */
  {-90, -90, -60, -90, -50},	/* dex = 1 */
  {-80, -80, -40, -80, -45},
  {-70, -70, -30, -70, -40},
  {-60, -60, -30, -60, -35},
  {-50, -50, -20, -50, -30},	/* dex = 5 */
  {-40, -40, -20, -40, -25},
  {-30, -30, -15, -30, -20},
  {-20, -20, -15, -20, -15},
  {-15, -10, -10, -20, -10},
  {-10, -5, -10, -15, -5},	/* dex = 10 */
  {-5, 0, -5, -10, 0},
  {0, 0, 0, -5, 0},
  {0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0},		/* dex = 15 */
  {0, 5, 0, 0, 0},
  {5, 10, 0, 5, 5},
  {10, 15, 5, 10, 10},		/* dex = 18 */
  {15, 20, 10, 15, 15},
  {15, 20, 10, 15, 15},		/* dex = 20 */
  {20, 25, 10, 15, 20},
  {20, 25, 15, 20, 20},
  {25, 25, 15, 20, 20},
  {25, 30, 15, 25, 25},
  {25, 30, 15, 25, 25},		/* dex = 25 */
  {25, 30, 15, 25, 25},
  {25, 30, 15, 25, 25},
  {25, 30, 15, 25, 25},
  {25, 30, 15, 25, 25},
  {25, 30, 15, 25, 25},		/* 30-D50 */
  {25, 30, 15, 25, 25},
  {25, 30, 15, 25, 25},
  {25, 30, 15, 25, 25},
  {25, 30, 15, 25, 25},
  {25, 30, 15, 25, 25},		/* 35-D75 */
  {25, 30, 15, 25, 25},
  {25, 30, 15, 25, 25},
  {25, 30, 15, 25, 25},
  {25, 30, 15, 25, 25},
  {25, 30, 15, 25, 25},		/* 40-D100 */
  {25, 30, 15, 25, 25},
  {25, 30, 15, 25, 25},
  {25, 30, 15, 25, 25},
  {25, 30, 15, 25, 25},
  {25, 30, 15, 25, 25},		/* 45-D125 */
  {25, 30, 15, 25, 25}
};



/* [dex] apply (all) */
/* react/miss_att/def/skl_bonus */
struct dex_app_type dex_app[] = {
  {7,  -7, 40, -7},		/* dex = 0 */
  {6,  -6, 35, -6},		/* dex = 1 */
  {5,  -4, 30, -5},
  {4,  -3, 25, -4},
  {3,  -2, 20, -3},
  {2,  -1, 15, -2},		/* dex = 5 */
  {1,   0, 10, -1},
  {1,   0,  5, -1},
  {0,   0,  0,  0},
  {0,   0,  0,  0},
  {0,   0,  0,  1},		/* dex = 10 */
  {0,   0,  0,  1},
  {0,   0,  0,  2},
  {-1,  0,  0,  2},
  {-1,  0,  0,  3},
  {-2,  0, -5,  3},		/* dex = 15 */
  {-2,  1,-10,  4},
  {-3,  2,-15,  4},
  {-3,  2,-20,  5},		/* dex = 18 */
  {-4,  3,-25,  5},
  {-5,  3,-30,  6},		/* dex = 20 */
  {-6,  4,-35,  6},
  {-7,  4,-40,  7},
  {-8,  4,-45,  7},
  {-9,  5,-50,  8},
  {-10, 5,-55,  9},		/* dex = 25 */
  {-11, 5,-58,  9},
  {-12, 5,-60,  9},
  {-13, 5,-62,  9},
  {-14, 5,-64,  9},
  {-15, 5,-66,  9},		/* 30 D50 */
  {-16, 5,-68,  9},
  {-17, 5,-70,  9},
  {-18, 5,-72,  9},
  {-19, 5,-74,  9},
  {-20, 5,-76,  9},		/* 35 D75 */
  {-21, 5,-78,  9},
  {-22, 5,-80,  9},
  {-23, 5,-82,  9},
  {-24, 5,-84,  9},
  {-25, 5,-86,  9},		/* 40 D100 */
  {-26, 5,-88,  9},
  {-27, 5,-90,  9},
  {-28, 5,-92,  9},
  {-29, 5,-94,  9},
  {-30, 5,-96,  9},		/* 45 D125 */
  {-31, 5,-98,  9}
};



/* [con] apply (all) */
struct con_app_type con_app[] = {
  {0, 20}, /*0*/		/* con = 0 */
  {0, 25}, /*1*/    	/* con = 1 */
  {0, 30}, /*2*/

  {0, 35}, /*3*/
  {0, 40}, /*4*/
  {0, 45}, /*5*/

  {1, 50}, /*6*/
  {1, 55}, /*7*/
  {1, 60}, /*8*/
 
  {1, 65}, /*9*/
  {1, 70}, /*10*/
  {1, 75}, /*11*/

  {2, 80}, /*12*/
  {2, 85}, /*13*/
  {2, 88}, /*14*/

  {3, 90}, /*15*/
  {3, 95}, /*16*/
  {3, 97}, /*17*/

  {4, 99}, /*18*/
  {4, 99}, /*19*/
  {4, 99}, /*20*/

  {5, 99}, /*21*/
  {5, 99}, /*22*/
  {5, 99}, /*23*/

  {6, 99}, /*24*/
  {6, 99}, /*25*/

  {7, 99},
  {8, 99},
  {9, 99},
  {10, 99},
  {11, 99},			/* 30 C50 */
  {12, 99},
  {13, 99},
  {14, 99},
  {15, 99},
  {16, 99},			/* 35 C75 */
  {17, 99},
  {18, 99},
  {19, 99},
  {20, 99},
  {21, 99},			/* 40 C100 */
  {22, 99},
  {23, 99},
  {24, 99},
  {25, 99},
  {26, 99},			/* 45 C125 */
  {27, 99}
};


/* point gain modifiers for regen. */
struct point_gain_type point_gain[] = {
  /* hp   mn   mv */
   { -10,-10 ,-10 },		/* 0*/
   { -10,-10 ,-10 },		/* 1*/
   {  -9, -9 ,-9  },
   {  -9. -9 ,-8  },
   {  -8, -9 ,-7  },
   {  -7, -8 ,-6  },		/* 5*/
   {  -6, -7 ,-5  },
   {  -5, -6 ,-4  },
   {  -4, -5 ,-3  },
   {  -3, -4 ,-2  },
   {  -2, -3 ,-1  },		/* 10 */
   {  -1, -2 , 0  },
   {   0, -1 , 0  },
   {   0,  0 , 1  },
   {   1,  0 , 2  },
   {   2,  1 , 4  },		/* 15 */
   {   3,  2 , 6  },
   {   4,  3 , 8  },
   {   4,  4 , 10 },
   {   5,  5 , 11 },
   {   5,  6 , 12 },		/* 20 */
   {   6,  7 , 13 },
   {   7,  8 , 14 },
   {   8,  9 , 15 },
   {   9, 10 , 16 },
   {  10, 11 , 17 },		/* 25 */
   {  11, 11 , 18 },
   {  11, 12 , 18 },
   {  12, 12 , 18 },
   {  12, 13 , 19 },
   {  13, 13 , 19 },		/* 30 (50)*/
   {  13, 13 , 19 },
   {  14, 14 , 20 },
   {  14, 14 , 20 },
   {  14, 14 , 21 },
   {  15, 15 , 21 },		/* 35 (75)*/
   {  15, 15 , 22 },
   {  15, 15 , 22 },
   {  16, 16 , 23 },
   {  16, 16 , 23 },
   {  16, 16 , 24 },		/* 40 (100)*/
   {  17, 17 , 24 },
   {  17, 17 , 25 },
   {  17, 17 , 25 },
   {  18, 18 , 26 },
   {  18, 18 , 26 },		/* 45 (125)*/
   {  18, 18 , 27 }

};



/* [int] apply (all) */
struct int_app_type int_app[] = {
  {-7},		/* int = 0 */
  {-6},		/* int = 1 */
  {-5},
  {-4},
  {-3},
  {-2},		/* int = 5 */
  {-1},
  {-1},
  {0},
  {1},
  {1},		/* int = 10 */
  {2},
  {2},
  {3},
  {3},
  {4},		/* int = 15 */
  {4},
  {5},
  {5},		/* int = 18 */
  {6},
  {6},		/* int = 20 */
  {7},
  {7},
  {8},
  {8},
  {9},		/* int = 25 */
  {9},
  {9},
  {9},
  {9},
  {9},				/* 30 */
  {9},
  {9},
  {9},
  {9},
  {9},				/* 35 */
  {9},
  {9},
  {9},
  {9},
  {9},				/* 40 */
  {9},
  {9},
  {9},
  {9},
  {9},				/* 45 */
  {9}
};


/* [wis] apply (all) */
struct wis_app_type wis_app[] = {
  {-7},	/* wis = 0 */
  {-6},  /* wis = 1 */
  {-5},
  {-4},
  {-3},
  {-2},  /* wis = 5 */
  {-1},
  {-1},
  {0},
  {1},
  {1},  /* wis = 10 */
  {2},
  {2},
  {3},
  {3},
  {4},  /* wis = 15 */
  {4},
  {5},
  {5},	/* wis = 18 */
  {6},
  {6},  /* wis = 20 */
  {7},
  {7},
  {8},
  {8},
  {9},  /* wis = 25 */
  {9},
  {9},
  {9},
  {9},
  {9}, /* 30 */
  {9},
  {9},
  {9},
  {9},
  {9}, /* 35 */
  {9},
  {9},
  {9},
  {9},
  {9}, /* 40 */
  {9},
  {9},
  {9},
  {9},
  {9}, /* 45 */
  {9}
};

struct cha_app_type cha_app[36] = {
   { -7,     -6,     20,     -50},   /* 0 */
   { -6,     -5,     18,     -35},
   { -5,     -4,     16,     -25},
   { -4,     -3,     14,     -20},   /* 3 */
   { -3,     -2,     12,     -15},
   { -2,     -1,     10,     -10},
   { -1,     -1,      8,     -5},
   { -1,      0,      6,     -2},
   {  0,      0,      4,     0},
   {  0,      0,      2,     0},     /* 9 */
   {  0,      0,      0,     0},
   {  1,      1,      0,     2},
   {  1,      1,      0,     2},
   {  2,      2,     -2,     2},
   {  2,      2,     -4,     5},
   {  3,      3,     -6,     5},
   {  4,      4,     -8,     8},
   {  5,      5,     -10,    12},
   {  6,      6,     -12,    20},    /* 18 */
   {  7,      7,     -14,    22},
   {  8,      8,     -16,    24},
   {  9,      9,     -18,    26},
   { 10,     10,     -20,    28},
   { 10,     10,     -22,    30},
   { 11,     11,     -24,    32},
   { 11,     11,     -26,    34},    /* 25 */
   { 12,     12,     -28,    36},
   { 13,     13,     -30,    38},
   { 14,     14,     -32,    40},
   { 15,     15,     -34,    42},
   { 16,     16,     -36,    44},    /* 30 */
   { 18,     18,     -38,    46},
   { 19,     19,     -40,    48},
   { 20,     20,     -42,    50},
   { 20,     20,     -44,    50},
   { 20,     20,     -46,    50}     /* 35 */
};


const int cha_align_table[21][21] =
{
   { 5, 5, 4, 4, 3, 3, 2, 2, 1, 1, 0,-1,-1,-2,-2,-3,-3,-4,-4,-5,-5}, /* -10 */
   { 5, 5, 4, 4, 3, 3, 2, 2, 1, 1, 0,-1,-1,-2,-2,-3,-3,-4,-4,-5,-5}, /* -9 */
   { 4, 4, 4, 4, 3, 3, 2, 2, 1, 1, 0,-1,-1,-2,-2,-3,-3,-4,-4,-4,-4}, /* -8 */
   { 4, 4, 4, 4, 3, 3, 2, 2, 1, 1, 0,-1,-1,-2,-2,-3,-3,-4,-4,-4,-4}, /* -7 */
   { 3, 3, 3, 3, 3, 3, 2, 2, 1, 1, 0,-1,-1,-2,-2,-3,-3,-3,-3,-3,-3}, /* -6 */
   { 3, 3, 3, 3, 3, 3, 2, 2, 1, 1, 0,-1,-1,-2,-2,-3,-3,-3,-3,-3,-3}, /* -5 */
   { 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 0,-1,-1,-2,-2,-2,-2,-2,-2,-2,-2}, /* -4 */
   { 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 0,-1,-1,-2,-2,-2,-2,-2,-2,-2,-2}, /* -3 */
   { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}, /* -2 */
   { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}, /* -1 */
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, /* 0 */
   {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, /* 1 */
   {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, /* 2 */
   {-2,-2,-2,-2,-2,-2,-2,-2,-1,-1, 0, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2}, /* 3 */
   {-2,-2,-2,-2,-2,-2,-2,-2,-1,-1, 0, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2}, /* 4 */
   {-3,-3,-3,-3,-3,-3,-2,-2,-1,-1, 0, 1, 1, 2, 2, 3, 3, 3, 3, 3, 3}, /* 5 */
   {-3,-3,-3,-3,-3,-3,-2,-2,-1,-1, 0, 1, 1, 2, 2, 3, 3, 3, 3, 3, 3}, /* 6 */
   {-4,-4,-4,-4,-3,-3,-2,-2,-1,-1, 0, 1, 1, 2, 2, 3, 3, 4, 4, 4, 4}, /* 7 */
   {-4,-4,-4,-4,-3,-3,-2,-2,-1,-1, 0, 1, 1, 2, 2, 3, 3, 4, 4, 4, 4}, /* 8 */
   {-5,-5,-4,-4,-3,-3,-2,-2,-1,-1, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5}, /* 9 */
   {-5,-5,-4,-4,-3,-3,-2,-2,-1,-1, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5} /* 10 */
};


#if 0
const char *spell_wear_off_msg[] = {
  "RESERVED DB.C",		/* 0 */
  "You feel less protected.",	/* 1 */
  "!Teleport!",
  "You feel less righteous.",
  "You feel a cloak of blindness dissolve.",
  "!Burning Hands!",		/* 5 */
  "!Call Lightning",
  "You feel more self-confident.",
  "You feel your strength return.",
  "!Clone!",
  "!Color Spray!",		/* 10 */
  "!Control Weather!",
  "!Create Food!",
  "!Create Water!",
  "!Cure Blind!",
  "!Cure Critic!",		/* 15 */
  "!Cure Light!",
  "You feel more optimistic.",
  "You feel less aware.",
  "Your eyes stop tingling.",
  "The detect magic wears off.",/* 20 */
  "The detect poison wears off.",
  "!Dispel Evil!",
  "!Earthquake!",
  "!Enchant Weapon!",
  "!Energy Drain!",		/* 25 */
  "!Fireball!",
  "!Harm!",
  "!Heal!",
  "You feel yourself exposed.",
  "!Lightning Bolt!",		/* 30 */
  "!Locate object!",
  "!Magic Missile!",
  "You feel less sick.",
  "Your natural aura returns.",
  "!Remove Curse!",		/* 35 */
  "The white aura around your body fades.",
  "!Shocking Grasp!",
  "You feel less tired.",
  "You feel weaker.",
  "!Summon!",			/* 40 */
  "!Ventriloquate!",
  "!Word of Recall!",
  "!Remove Poison!",
  "You feel less aware of your surroundings.",
  "!ANIMATE DEAD!",		/* 45 */
  "!DISPEL GOOD!",
  "!GROUP ARMOR!",
  "!GROUP HEAL!",
  "!GROUP RECALL!",
  "Your night vision seems to fade.",	/* 50 */
  "Your feet seem less buoyant.",
  "You feel slower.",
  "Your massive boost of power quickly fades away, leaving you with a great sense of loss.",
  "Your lethargy passes.",
  "!ACID BLAST!",                               /* 55 */
  "!FIRE BREATH!",
  "!GAS BREATH!",
  "!FROST BREATH!",
  "!ACID BREATH!",
  "!LIGHTNING BREATH!",                         /* 60 */
  "!GROUP INFRA!",
  "The force field around you fades.",
  "Your skin softens.",
  "!FLAME STRIKE!",
  "You begin to fall to the ground.",           /* 65 */
  "Your magic has been dispelled.",
  "You feel like yourself again.",
  "You feel like yourself again.",
  "You dont feel inspired anymore.",
  "You do not feel as aware of your surroundings.",  /* 70 */
  "!ENCHANT ARMOR!",
  "!GROUP REFRESH!",
  "!REFRESH!",
  "!GIVE LIFE!",
  "Your skin feels soft again.", /* 75 */
  "You don't feel depressed anymore.",
  "Boy is it morning already??",
  "You stop moving rapidly.",
  "You feel like yourself again.",
  "!ENFEEBLE!", /* 80 */
  "!FIRESONG!",
  "!WRATH OF GOD!",
  "You can move at a normal speed again.",
  "!PURIFY!",
  "Your gills begin to disappear.", /* 85 */
  "Your new found strength begins to fade.",
  "!GATE!",
  "!GAS BLAST!",
  "!FROST BLAST!",
  "The disease seems to subside..For now.", /* 90 */
  "You fade back into existence.",
  "!CALM!",
  "!METEOR STORM!",
  "!ICE STORM!",
  "!CHANGE SEX!", /* 95 */
  "!CURE PLAGUE",
  "!SUNBURN!",
  "!CURE SERIOUS!",
  "!ENERGY!",
  "!GROUP SANC!",/*100*/
  "!GROUP LEVI!",
  "!CREATE LIGHT!",
  "!CONTINUAL LIGHT!",
  "!PORTAL!",
  "!IDENTIFY!",			/* 105 */
  "!BACKSTAB!",
  "!BASH!",
  "!HIDE!",
  "!KICK!",
  "!PICK!",			/* 110 */
  "!PUNCH!",
  "!RESCUE!",
  "", /* sneak */
  "!STEAL!",
  "!TRACK!",			/* 115 */
  "!MOUNT!",
  "!RIDING!",
  "!TAME!",
  "!2NDATTACK!",
  "!3RDATTACK!",		/* 120 */
  "!4THATTACK!",
  "", /* rage */
  "!THROW!",
  "!BOW!",
  "!SLING!",			/* 125 */
  "!CROSSBOW!",
  "!2XWIELD!",
  "!REPAIR!",
  "Your wings disappear!",
  "!FISTICUFF!",		/* 130 */
  "!SWORD!",
  "!2HSWORD!",
  "!DAGGER!",
  "!CLUB!",
  "!2HCLUB!",			/* 135 */
  "!HAMMER!",
  "!2HHAMMER!",
  "!AXE!",
  "!2HAXE!",
  "!SPEAR!",			/* 140 */
  "!WHIP!",
  "!CLAW!",
  "!BREW!",
  "!SCRIBE!",
  "!KNOCK!",			/* 145 */
  "!WIZARDLOCK!",
  "!AMBUSH!",
  "!TRIP!",
  "!SWEEP!",
  "!QUIVERINGPALM!",		/* 150 */
  "!STOMP!",
  "!HEADBUTT!",
  "!DISARM!",
  "!BERSERK!",
  "!CIRCLE!",			/* 155 */
  "!SHOCK!",
  "!STUN!",
  "You find you are no longer hidden in the shadows.",
  "!CAMOUFLAGE!",
  "!PALM!",			/* 160 */
  "!LAYHANDS!",
  "!BANDAGE!",
  "!DARKEN!",
  "!LIGHTEN!",
  "!CHANT!",			/* 165 */
  "!MEDITATE!",
  "!DODGE!",
  "!BLOCK!",
  "!PARRY!",
  "!CONJURE INFANTRY!",		/* 170 */
  "The web dissapates.",
  "!GORE!",
  "You stop glowing.",
  "You feel warmer.",
  "You feel cooler.",		/* 175 */
  "You no longer feel grounded.",
  "You shine no longer.",
  "Your skin loses its yellow tinge.",
  "Your skin loses its green tinge.",
  "A dark shadow passes before your eyes.", /* 180 */
  "Your natural aura returns.",
  "!SUNRAY!",
  "!REDIRECT!",
  "!CAUSE LIGHT!",
  "!CAUSE SERIOUS!",		/* 185 */
  "!CAUSE CRITIC!",
  "!ATONEMENT!",
  "!SUMMON MOUNT!",
  "!GOODBERRY!",
  "!READMAGIC!",		/* 190 */
  "!CLANRECALL!",
  "The vines wither and die.",
  "!GRANTPEACE!",
  "!DIG!",
  "!SKIN!",			/* 195 */
  "The shield of fire cools and dissipates.",
  "!DROWN!",
  "The vines entrapping you wither and die.",
  "!FEAR!",
  "!MOUNTED ATTACK!",           /* 200 */
  "!FISHING!",
  "!GROUP SUMMON!",
  "", /* Rove */
  "", /* Peck */
  "\n"
};
#endif

const char *spell_targets[] = {
   "Ignore",
   "Char-Room",
   "Char-World",
   "Fight-Self",
   "Fight-Vict",
   "Self-Only",
   "Not-Self",
   "Obj-Inv",
   "Obj-Room",
   "Obj-World",
   "Obj-Equip",
   "Door",
   "\n"
};
   
const char *spell_routines[] = {
   "Damage",
   "Affects",
   "Unaffects",
   "Points",
   "Alter-Objs",
   "Groups",
   "Masses",
   "Areas",
   "Summons",
   "Creations",
   "Manual",
   "Check",
   "Materials",
   "Forceful",
   "\n"
};

const char *npc_class_types[] = 
{ 
   "Warrior", 
   "Cleric",
   "Thief", 
   "Magic User", 
   "Ranger", 
   "Bard", 
   "Monk", 
   "*UNUSED*", 
   "Barbarian", 
   "Paladin", 
   "Anti-Paladin", 
   "Druid", 
   "Merchant", 
   "Kensai", 
   "Assassin", 
   "Necromancer", 
   "Deva", 
   "Immortal", 
   "God", 
   "Skillless",
   "\n"
};

const char *WizLevels[] = {
   " Ambassador  ", //105
   " Ambassador  ", //106
   "  ArchAngel  ", //107
   "  ArchAngel  ", //108
   "  ArchAngel  ", //109
   "   Seraph    ", //110
   "   Seraph    ", //111
   "   Seraph    ", //112
   "    Deity    ", //113
   "    Deity    ", //114
   "    Deity    ", //115
   "   Demi God  ", //116
   "   Demi God  ", //117
   "   Demi God  ", //118
   "     God     ", //119
   "     God     ", //120
   "     God     ", //121
   " Greater God ", //122
   " Greater God ", //123
   " Greater God ", //124
   "Administrator", //125
   " Implementor ", //126
   " Implementor ", //127
   " Implementor ", //128
   " Implementor ", //129
   "\n"
};


int rev_dir[] =
{
  2,
  3,
  0,
  1,
  5,
  4
};


const char *weekdays[] = {
  "the Day of the Moon",
  "the Day of the Bull",
  "the Day of the Deception",
  "the Day of Thunder",
  "the Day of Freedom",
  "the Day of the Great Gods",
  "the Day of the Sun"
};


const char *month_name[] = {
  "Month of Winter",		/* 0 */
  "Month of the Winter Wolf",
  "Month of the Frost Giant",
  "Month of the Old Forces",
  "Month of the Grand Struggle",
  "Month of the Spring",
  "Month of Nature",
  "Month of Futility",
  "Month of the Dragon",
  "Month of the Sun",
  "Month of the Heat",
  "Month of the Battle",
  "Month of the Dark Shades",
  "Month of the Shadows",
  "Month of the Long Shadows",
  "Month of the Ancient Darkness",
  "Month of the Great Evil"
};


const int sharp[] = {
  0,				/* 0 */
  0,
  1,				/* Pierce */
  0,				/* Slashing */
  0,
  0,				/* 5 */
  0,				/* Pierce, no BS */
  0,			
  0,
  0,
  0,				/* 10 */
  0,
  0,
  0,
  0,
  0
};				/* stab 15   */

#if defined(OASIS_MPROG)
/*
 * Definitions necessary for MobProg support in OasisOLC
 */
const char *mobprog_types[] = {
   "INFILE",
   "ACT",
   "SPEECH",
   "RAND",
   "FIGHT",
   "DEATH",
   "HITPRCNT",
   "ENTRY",
   "GREET",
   "ALL_GREET",
   "GIVE",
   "BRIBE",
   "\n"
} ;
#endif


const char *hyper_soc[] = {
   "bounce",  
   "beam",    
   "beer",    
   "blush",   
   "boggle",  
   "cackle",  
   "cartwheel"
   "cheer",   
   "dance",   
   "flip",    
   "flirt",   
   "flowers", 
   "flutter", 
   "fondle",  
   "french",  
   "frolic",  
   "happy",   
   "highfive",
   "hop",     
   "kiss",    
   "laugh",   
   "lick",    
   "nih",     
   "sing",    
   "smile",   
   "smirk",   
   "snicker", 
   "\n"
};
const char *happy_soc[] = {
   "applaud", 
   "beam",    
   "beer",    
   "blush",   
   "boggle",  
   "bow",     
   "cackle",  
   "cartwheel"
   "cheer",   
   "chuckle", 
   "cuddle",  
   "curtsey", 
   "dance",   
   "flip",    
   "flirt",   
   "flowers", 
   "flutter", 
   "fondle",  
   "french",  
   "frolic",  
   "giggle",  
   "grope",   
   "happy",   
   "highfive",
   "hop",     
   "kiss",    
   "laugh",   
   "lick",    
   "nih",     
   "ruffle",  
   "shake",   
   "sing",    
   "smile",   
   "smirk",   
   "snicker", 
   "\n"
};
const char *cheery_soc[] = {
   "applaud", 
   "beam",    
   "beer",    
   "blink",
   "blush",   
   "boggle",  
   "bow",     
   "cackle",  
   "cheer",   
   "chuckle", 
   "clap",    
   "comfort", 
   "cuddle",  
   "curtsey", 
   "dance",   
   "daydream",
   "embrace", 
   "flex",    
   "flirt",   
   "flowers", 
   "flutter", 
   "fondle",  
   "french",  
   "giggle",  
   "happy",   
   "kiss",    
   "laugh",   
   "lick",    
   "nod",     
   "ruffle",  
   "shake",   
   "smile",   
   "smirk",   
   "snicker", 
   "wave",    
   "\n"
};
const char *neutral_soc[] = {
   "applaud", 
   "beef",    
   "bow",
   "blink",
   "blush",   
   "boggle",  
   "cheer",   
   "chuckle", 
   "clap",    
   "clueless",
   "comb",    
   "comfort", 
   "concerned"
   "cower",   
   "cringe",  
   "cuddle",  
   "curtsey", 
   "daydream",
   "doh",     
   "embrace", 
   "flex",    
   "grope",   
   "grunt",   
   "hangover",
   "nod",     
   "pray",    
   "pretend", 
   "shake",   
   "\n"
};
const char *sad_soc[] = {
   "beg",     
   "bow",     
   "brb",     
   "burn",    
   "clap",    
   "clueless",
   "concerned"
   "cough",   
   "cower",   
   "cringe",  
   "cry",     
   "curtsey", 
   "daydream",
   "doc",     
   "fart",    
   "frown",   
   "frustrated",
   "fume",    
   "gag",     
   "grumble", 
   "grunt",   
   "hangover",
   "nod",     
   "nudge",   
   "poke",    
   "sad",     
   "shake",   
   "shin",   
   "sigh",    
   "sulk",    
   "\n"
};
const char *grouchy_soc[] = {
   "accuse",  
   "ack",     
   "bearhug", 
   "beg",     
   "bonk",    
   "brb",     
   "burn",    
   "burp",    
   "cough",   
   "cry",     
   "curse",   
   "disgusted",
   "drool",   
   "growl",
   "fart",    
   "frown",   
   "frustrated",
   "fume",    
   "gag",     
   "glare",   
   "grin",    
   "growl",   
   "grumble", 
   "nudge",   
   "poke",    
   "puke",    
   "punch",   
   "scream",  
   "shin",   
   "slap",    
   "insult",
   "wedgie",  
   "\n"
};
const char *homicidal_soc[] = {
   "accuse",  
   "ack",     
   "banzai",  
   "bearhug", 
   "bonk",    
   "cackle",  
   "curse",   
   "frown",   
   "frustrated",
   "fume",    
   "glare",   
   "grin",    
   "growl",   
   "poke",    
   "puke",    
   "punch",   
   "scream",
   "shin",   
   "slap",    
   "spit",    
   "steam",   
   "tackle",  
   "taunt",   
   "warcry",  
   "\n"
};
const char *opp_sex_soc[] = {
   "snuggle",
   "cuddle",
   "embrace", 
   "flirt",   
   "flowers", 
   "flutter", 
   "fondle",  
   "french",  
   "grope",   
   "kiss",    
   "\n"
};

   
const char *reset_string[] = {
   "Never Reset",
   "Wait until empty(can idle out)",
   "Normal reset (can idle out)",
   "Normal reset (never idle out)"
};

const char *race_trait_string[]={
   "Humanoid",
   "Long",
   "Level-Size",
   "Talks",
   "Flies",
   "Swims",
   "Hands",
   "Warm",
   "Immaterial",
   "No Stun",
   "Infravision",
   "Ultravision",
   "Hit Bonus",
   "Auto-Sneak",
   "Morale",
   "Susc",
   "Resist",
   "Immune"
};

struct mob_defaults mob_def_stats[] = {
  /* lvl  #d#+ #hp     +hit   ac   #d#+#dam  #gold */
   {  0,  2,3, 8,        1,   200, 1,4,0,    0    },
   {  1,  2,3,  8,       1,   196, 1,2,0,    3    },
   {  2,  2,4, 12,       1,   192, 1,2,0,    5    },
   {  3,  2,5, 18,       1,   188, 1,3,0,    7    },
   {  4,  2,6, 28,       2,   184, 1,3,0,    10   },
   {  5,  2,7, 39,       2,   180, 1,4,1,    15   },
   {  6,  2,8, 50,       2,   176, 2,4,0,    20   },
   {  7,  3,4, 60,       3,   172, 2,5,-1,   25   },
   {  8,  3,4, 70,       3,   168, 2,5,0,    30   },
   {  9,  3,4, 80,       3,   164, 2,6,0,    40   },
   { 10,  3,4, 95,       4,   160, 3,4,0,    50   },
   { 11,  4,6, 110,      4,   156, 2,6,1,    60   },
   { 12,  4,6, 125,      4,   152, 3,4,1,    70   },
   { 13,  4,6, 140,      5,   148, 2,6,2,    80   },
   { 14,  4,6, 155,      5,   144, 3,4,2,    90   },
   { 15,  4,6, 175,      5,   140, 2,6,3,    100  },
   { 16,  4,6, 195,      6,   136, 3,4,3,    110  },
   { 17,  4,6, 215,      6,   132, 2,6,4,    120  },
   { 18,  4,6, 235,      6,   128, 3,4,4,    130  },
   { 19,  4,6, 255,      7,   124, 2,6,5,    140  },
   { 20,  4,6, 280,      7,   120, 2,6,5,    150  },
   { 21,  4,9, 300,      7,   116, 3,4,5,    160  },
   { 22,  4,9, 320,      8,   112, 2,6,6,    170  },
   { 23,  4,9, 340,      8,   108, 3,4,6,    180  },
   { 24,  4,9, 360,      8,   104, 2,6,7,    190  },
   { 25,  4,9, 380,      9,   100, 2,6,7,    200  },
   { 26,  4,9, 400,      9,    96, 3,4,7,    220  },
   { 27,  4,9, 420,      9,    92, 3,4,7,    240  },
   { 28,  4,9, 440,     10,    88, 2,6,8,    260  },
   { 29,  4,9, 460,     10,    84, 3,4,8,    280  },
   { 30,  4,9, 480,     10,    80, 3,4,8,    300  },
   { 31,  6,9, 500,     11,    76, 2,6,9,    320  },
   { 32,  6,9, 520,     11,    72, 3,4,9,    340  },
   { 33,  6,9, 540,     11,    68, 3,4,9,    360  },
   { 34,  6,9, 560,     12,    64, 2,6,10,   380  },
   { 35,  6,9, 580,     12,    60, 2,6,10,   400  },
   { 36,  6,9, 600,     12,    56, 3,4,10,   430  },
   { 37,  6,9, 620,     13,    52, 3,4,10,   460  },
   { 38,  6,9, 640,     13,    48, 2,6,11,   490  },
   { 39,  6,9, 660,     13,    44, 2,6,11,   520  },
   { 40,  6,9, 680,     14,    40, 3,4,11,   550  },
   { 41,  7,9, 700,     14,    36, 3,4,11,   580  },
   { 42,  7,9, 720,     14,    32, 2,6,12,   610  },
   { 43,  7,9, 740,     15,    28, 2,6,12,   640  },
   { 44,  7,9, 760,     15,    24, 3,4,12,   670  },
   { 45,  7,9, 780,     15,    20, 3,4,12,   700  },
   { 46,  7,9, 800,     16,    16, 3,4,12,   730  },
   { 47,  7,9, 820,     16,    12, 2,6,13,   760  },
   { 48,  7,9, 840,     16,     8, 2,6,13,   790  },
   { 49,  7,9, 860,     17,     4, 3,4,13,   820  },
   { 50,  7,9, 880,     17,     0, 3,4,13,   850  },
   { 51,  8,9, 900,     17,    -4, 3,4,13,   880  },
   { 52,  8,9, 920,     18,    -8, 2,6,14,   910  },
   { 53,  8,9, 940,     18,   -12, 2,6,14,   940  },
   { 54,  8,9, 960,     18,   -16, 2,6,14,   970  },
   { 55,  8,9, 980,     19,   -20, 3,4,14,   1000 },
   { 56,  8,9, 1000,    19,   -24, 3,4,14,   1025 },
   { 57,  8,9, 1020,    19,   -28, 3,4,14,   1050 },
   { 58,  8,9, 1040,    20,   -32, 2,6,15,   1075 },
   { 59,  8,9, 1060,    20,   -36, 2,6,15,   1125 },
   { 60,  8,9, 1080,    20,   -40, 2,6,15,   1150 },
   { 61,  9,9, 1100,    21,   -44, 3,4,15,   1175 },
   { 62,  9,9, 1120,    21,   -48, 3,4,15,   1200 },
   { 63,  9,9, 1140,    21,   -52, 3,4,15,   1225 },
   { 64,  9,9, 1160,    22,   -56, 2,6,16,   1250 },
   { 65,  9,9, 1180,    22,   -60, 2,6,16,   1275 },
   { 66,  9,9, 1200,    22,   -64, 2,6,16,   1300 },
   { 67,  9,9, 1220,    23,   -68, 2,6,16,   1325 },
   { 68,  9,9, 1240,    23,   -72, 3,4,16,   1350 },
   { 69,  9,9, 1260,    23,   -76, 3,4,16,   1375 },
   { 70,  9,9, 1280,    24,   -80, 3,4,16,   1400 },
   { 71, 10,9, 1300,    24,   -84, 2,6,17,   1425 },
   { 72, 10,9, 1320,    24,   -88, 2,6,17,   1450 },
   { 73, 10,9, 1340,    25,   -92, 2,6,17,   1475 },
   { 74, 10,9, 1360,    25,   -96, 2,6,17,   1500 },
   { 75, 10,9, 1380,    25,  -100, 3,4,17,   1525 },
   { 76, 10,9, 1400,    26,  -104, 3,4,17,   1550 },
   { 77, 10,9, 1420,    26,  -108, 3,4,17,   1575 },
   { 78, 10,9, 1440,    26,  -112, 3,4,17,   1600 },
   { 79, 10,9, 1460,    27,  -116, 3,4,17,   1625 },
   { 80, 10,9, 1480,    27,  -120, 2,6,18,   1650 },
   { 81, 11,9, 1500,    27,  -124, 2,6,18,   1675 },
   { 82, 11,9, 1525,    28,  -128, 2,6,18,   1700 },
   { 83, 11,9, 1550,    28,  -132, 2,6,18,   1725 },
   { 84, 11,9, 1575,    28,  -136, 3,4,18,   1750 },
   { 85, 11,9, 1600,    29,  -140, 3,4,18,   1775 },
   { 86, 11,9, 1625,    29,  -144, 3,4,18,   1800 },
   { 87, 11,9, 1650,    29,  -148, 3,4,18,   1825 },
   { 88, 11,9, 1675,    30,  -152, 3,4,18,   1850 },
   { 89, 11,9, 1700,    30,  -156, 2,6,19,   1875 },
   { 90, 11,9, 1725,    30,  -160, 2,6,19,   1900 },
   { 91, 12,9, 1750,    31,  -164, 2,6,19,   1925 },
   { 92, 12,9, 1775,    31,  -168, 2,6,19,   1950 },
   { 93, 12,9, 1800,    31,  -172, 2,6,19,   1975 },
   { 94, 12,9, 1825,    32,  -176, 2,6,19,   2000 },
   { 95, 12,9, 1850,    32,  -180, 3,4,19,   2025 },
   { 96, 13,9, 1875,    32,  -184, 3,4,19,   2050 },
   { 97, 13,9, 1900,    33,  -188, 3,4,19,   2075 },
   { 98, 13,9, 1925,    33,  -192, 3,4,19,   2100 },
   { 99, 13,9, 1950,    33,  -196, 3,4,19,   2125 },
   {100, 13,9, 2000,    34,  -200, 2,6,20,   2150 },
   {101, 14,9, 2100,    35,  -200, 2,6,20,   2175 },
   {102, 15,9, 2300,    36,  -200, 2,6,20,   2200 },
   {103, 16,9, 2600,    37,  -200, 2,6,20,   2400 },
   {104, 17,9, 3000,    38,  -200, 2,6,20,   2700 },
   {105, 18,9, 3500,    39,  -200, 2,6,20,   3000 },
   {106, 19,9, 4100,    40,  -200, 2,6,20,   3500 },
   {107, 20,9, 4800,    42,  -200, 3,4,20,   4000 },
   {108, 21,9, 5600,    44,  -200, 3,4,20,   5000 },
   {109, 22,9, 6500,    46,  -200, 3,4,20,   6500 },
   {110, 23,9, 7500,    48,  -200, 3,4,20,   8000 },
   {111, 24,9, 8600,    50,  -200, 3,4,20,   10000 },
   {112, 25,9, 9700,    52,  -200, 3,4,20,   15000 },
   {113, 25,9, 9700,    52,  -200, 3,4,20,   15000 },
   {114, 25,9, 9700,    52,  -200, 2,6,21,   15000 },
   {115, 25,9, 9700,    52,  -200, 2,6,21,   15000 },
   {116, 25,9, 9700,    52,  -200, 2,6,21,   15000 },
   {117, 25,9, 9700,    52,  -200, 2,6,21,   15000 },
   {118, 25,9, 9700,    52,  -200, 2,6,21,   15000 },
   {119, 25,9, 9700,    52,  -200, 2,6,21,   15000 },
   {120, 25,9, 9700,    52,  -200, 2,6,21,   15000 },
   {121, 25,9, 9700,    52,  -200, 3,4,21,   15000 },
   {122, 25,9, 9700,    52,  -200, 3,4,21,   15000 },
   {123, 25,9, 9700,    52,  -200, 3,4,21,   15000 },
   {124, 25,9, 9700,    52,  -200, 3,4,21,   15000 },
   {125, 25,9, 9700,    52,  -200, 3,4,21,   15000 }
};

/* multiplier used to calculate max mana for npc */
const struct npc_class_mana npc_class_mult[] = {
/* npc_mana */
{ 5 },		/* warrior */
{ 11 },	 	/* cleric */
{ 5 },		/* thief */
{ 11 },		/* magic user */
{ 9 },		/* ranger */
{ 9 },		/* bard */
{ 9 },		/* monk */
{ 0 },		/* unused */
{ 5 },		/* barbarian */
{ 9 },		/* paladin */
{ 9 },		/* anti-paladin */
{ 9 },		/* druid */
{ 5 },		/* merchant */
{ 10 },		/* kensai */
{ 10 },		/* assassin */
{ 12 },		/* necromancer */
{ 12 },		/* deva */
{ 12 },		/* immortal */
{ 12 },		/* god */
{ 2 }		/* skillless */
};

const char *MYERRORSTRING[] = {
   "ARRAY OVERFLOW!!!!",
   "ARRAY OVERFLOW!!!!",
   "ARRAY OVERFLOW!!!!",
   "ARRAY OVERFLOW!!!!",
   "ARRAY OVERFLOW!!!!",
   "ARRAY OVERFLOW!!!!",
   "ARRAY OVERFLOW!!!!",
   "ARRAY OVERFLOW!!!!",
   "ARRAY OVERFLOW!!!!",
   "ARRAY OVERFLOW!!!!",
   "ARRAY OVERFLOW!!!!",
   "ARRAY OVERFLOW!!!!",
   "ARRAY OVERFLOW!!!!",
   "ARRAY OVERFLOW!!!!",
   "ARRAY OVERFLOW!!!!",
   "ARRAY OVERFLOW!!!!",
   "ARRAY OVERFLOW!!!!",
   "ARRAY OVERFLOW!!!!",
   "ARRAY OVERFLOW!!!!",
   "ARRAY OVERFLOW!!!!"
};

/*
const char *hometown_menu =
"Please choose a home town:\r\n"
"\r\n"
"  A. Heliopolis\r\n"
"  B. Mordilnia\r\n"
"  C. New Thalos\r\n"
"  D. New Haven\r\n"
"  E. Aethelfyrd\r\n"
"  F. Silverport\r\n"
"  G. Keorc Koilos\r\n"
"  H. Naraka\r\n"
"  I. Ixchal\r\n"
"\r\n"
"We STRONGLY recommend Heliopolis as a home town.  Choose another at your own risk!\r\n"
"\r\n";
*/

const char *hometown_menu =
"Please choose a home town:\r\n"
"\r\n"
"  A. Aethelfyrd   - A civilized merchant city not far from the coast of Aglaron.\r\n"
"  B. Heliopolis   - A peaceful city located in the central continent Caledon.\r\n"
"  C. Ixchal       - A mystic city of ancient gods on the far rainforest continent of Talam'.\r\n"
"  D. Keorc Koilos - The city of Drow and Dwarf, located in the vast and silent Underdark (\x1B[0;31mAdvanced\x1\
B[0;0m).\r\n"
"  E. Naraka       - A bustling coastal city in the far reaches of Aglaron (\x1B[0;31mAdvanced\x1B[0;0m).\r\n"
"\r\n"
"We STRONGLY recommend Heliopolis as a home town.  Choose another at your own risk!\r\n"
"\r\n";

const char *hometown_prompt =
"Please select a hometown: ";
