/* ************************************************************************
*   File: structs.h                                     Part of CircleMUD *
*  Usage: header file for central structures and contstants               *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/*
 * Intended use of this macro is to allow external packages to work with
 * a variety of CircleMUD versions without modifications.  For instance,
 * an IS_CORPSE() macro was introduced in pl13.  Any future code add-ons
 * could take into account the CircleMUD version and supply their own
 * definition for the macro if used on an older version of CircleMUD.
 * You are supposed to compare this with the macro CIRCLEMUD_VERSION()
 * in utils.h.  See there for usage.
 */
#define _CIRCLEMUD	0x03000F /* Major/Minor/Patchlevel - MMmmPP */

/* preamble *************************************************************/

#define NOWHERE    -1    /* nil reference for room-database	*/
#define NOTHING	   -1    /* nil reference for objects		*/
#define NOBODY	   -1    /* nil reference for mobiles		*/

#define SPECIAL(name) \
   int (name)(struct char_data *ch, void *me, int cmd, char *argument)

/* misc editor defines **************************************************/

/* format modes for format_text */
#define FORMAT_INDENT		(1 << 0)


/* Tracking defines.  Used to see which checks to use. */
#define IGNORE_NOTRACK     (1 << 0)
#define IGNORE_CLAN        (1 << 1)
#define IGNORE_ZNOTRACK    (1 << 2)
#define IGNORE_WATER       (1 << 3)
#define IGNORE_FLY         (1 << 4)
#define IGNORE_THROUGHDOOR (1 << 5)
#define MAZE_TEST          (1 << 6)

/* room-related defines *************************************************/
#define NUM_HOMETOWNS 11
#define NUM_ORE_SLOTS 4

/* The cardinal directions: used as index to room_data.dir_option[] */
#define NORTH          0
#define EAST           1
#define SOUTH          2
#define WEST           3
#define UP             4
#define DOWN           5

/* Updated to match Phoenix--Aleks
*  Notes: The roomflag HOUSE in Phoenix 3 has been removed.  
*  I placed an UNUSED in its place to maintain world file compatibility.
*  3.0's HOUSE is no longer necessary due to the fact that ROOM_HOUSE
*  serves a similar function here--does not purge the room.  We may find
*  future need for a HOUSE(NO_DECAY) room flag, but for the time being
*  it is extraneous. 
*/
/* Room flags: used in room_data.room_flags */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
#define ROOM_DARK		           (1 << 0)   /* Dark			*/
#define ROOM_DEATH		        (1 << 1)   /* Death trap		*/
#define ROOM_NOMOB		        (1 << 2)   /* MOBs not allowed		*/
#define ROOM_INDOORS		        (1 << 3)   /* Indoors			*/
#define ROOM_CLAN		           (1 << 4)   /* Clan access only		*/
#define ROOM_REGEN		        (1 << 5)   /* High regen rate		*/
#define ROOM_NOTRACK		        (1 << 6)   /* Track won't go through	*/
#define ROOM_NOMAGIC		        (1 << 7)   /* Magic not allowed		*/
#define ROOM_TUNNEL		        (1 << 8)   /* room for only 1 pers	*/
#define ROOM_PRIVATE		        (1 << 9)   /* Can't teleport in		*/
#define ROOM_GODROOM		        (1 << 10)  /* LVL_DGOD+ only allowed	*/
#define ROOM_BFS_MARK		     (1 << 11)  /* (R) Breath-first srch mrk */
#define ROOM_NO_DECAY	        (1 << 12)  /* Unused Room Flag		*/
#define ROOM_NO_RECALL		     (1 << 13)  /* No Recall			*/
#define ROOM_NO_SUMMON		     (1 << 14)  /* No summon			*/
#define ROOM_PKILL		        (1 << 15)  /* Pkill allowed		*/
#define ROOM_PEACEFUL		     (1 << 16)  /* No violence allowed	*/
#define ROOM_SOUNDPROOF		     (1 << 17)  /* No sound allowed		*/
#define ROOM_HOUSE		        (1 << 18)  /* (R) Room is a house	*/
#define ROOM_HOUSE_CRASH	     (1 << 19)  /* (R) House needs saving	*/
#define ROOM_ATRIUM		        (1 << 20)  /* (R) The door to a house	*/
#define ROOM_OLC		           (1 << 21)  /* (R) Modifyable/!compress	*/
#define ROOM_TRAVEL	           (1 << 22)  /* 5X move rate w/o horsie 	*/
#define ROOM_CAMP  		        (1 << 23)  /* you can camp in here    	*/
#define ROOM_DONATION           (1 << 24)  /* (R) Is a donation room    */
#define ROOM_NO_LEV             (1 << 25)  /* No Levitation             */
#define ROOM_NO_FLY             (1 << 26)  /* No Flying                 */
#define ROOM_NO_MOUNT           (1 << 27)  /* No Mounted people         */
#define ROOM_NO_CAMP  		     (1 << 28)  /* no camping here   	*/
#define ROOM_MINE               (1 << 29)  /* you can mine here         */
#define ROOM_NO_WATERBREATHE    (1 << 30)  /* water breathe doesn't work*/

/**** extra 32 flags available with room2_flags   ******/
#define ROOM2_SALTWATER_FISH    (1 << 0)   /* Player can fish here     */
#define ROOM2_FRESHWATER_FISH   (1 << 1)   /* Player can fish here too */
#define ROOM2_NEVERMOB         (1 << 2)   /* No mobs allowed ever */
#define ROOM2_GRAFFITI          (1 << 3)  /* Room can be graffiti'd. */
#define ROOM2_PLAYER_SHOP       (1 << 4) /* room is a player's shop. */

/* Room affections: used in room_data.affected */
/* temporarily unavailable, pending code additions- nomikos 1/1/03 */
#define RAFF_FOG                (1 << 0)   /* Room is foggy             */
#define RAFF_HEAT               (1 << 1)   /* Heat shimmer(desert/swamp)*/
#define RAFF_SNOW               (1 << 2)   /* Snow buildup              */
#define RAFF_FLOWERS            (1 << 3)   /* Flowers in spring         */
#define RAFF_LEAVES             (1 << 4)   /* Leaves in autumn          */

/* Exit info: used in room_data.dir_option.exit_info */
#define EX_ISDOOR		(1 << 0)   /* Exit is a door		*/
#define EX_PICKPROOF		(1 << 1)   /* Lock can't be picked	*/
#define EX_CLOSED		(1 << 2)   /* The door is closed	*/
#define EX_LOCKED		(1 << 3)   /* The door is locked	*/
#define EX_SECRET               (1 << 4)   /* Only seen when open       */
#define EX_HIDDEN               (1 << 5)   /* can't be seen at all      */
#define EX_FLY                  (1 << 6)   /* Need to be flying to pass */
#define EX_DROP                 (1 << 7)   /* If !FLYING take some dam  */
#define EX_AUTOCLOSE            (1 << 8)   /* door closes behind char   */
#define EX_WIZLOCK              (1 << 9)   /* door is wizlocked         */
#define EX_NOPASS               (1 << 10)  /* Mobs and PCs can't use this exit. */

/* Teleport info: used in room_data.tele_data */
#define TELE_LOOK           (1 << 0)
#define TELE_COUNT          (1 << 1)
#define TELE_RANDOM         (1 << 2)
#define TELE_SPIN           (1 << 3)
#define TELE_OBJ            (1 << 4)
#define TELE_NOOBJ          (1 << 5)
#define TELE_NOMSG          (1 << 6)
#define TELE_NOMOB          (1 << 7)
#define TELE_SKIPOBJ        (1 << 8)
#define TELE_NOPC           (1 << 9)
#define TELE_NVRMOB         (1 << 10)

/* Updated to match Phoenix */
/* Sector types: used in room_data.sector_type */
#define SECT_INSIDE          0		   /* Indoors			*/
#define SECT_CITY            1		   /* In a city			*/
#define SECT_FIELD           2		   /* In a field		*/
#define SECT_FOREST          3		   /* In a forest		*/
#define SECT_HILLS           4		   /* In the hills		*/
#define SECT_MOUNTAIN        5		   /* On a mountain		*/
#define SECT_WATER_SWIM      6		   /* Swimmable water		*/
#define SECT_WATER_NOSWIM    7		   /* Water - need a boat	*/
#define SECT_UNDERWATER	     8		   /* Underwater		*/
#define SECT_FLYING          9		   /* Wheee!			*/
#define SECT_DESERT          10		   /* Desert			*/
#define SECT_SWAMP           11		   /* Swamp			*/
#define SECT_TUNDRA          12		   /* Tundra			*/
#define SECT_JUNGLE          13		   /* Swamp			*/
#define SECT_ROAD            14        /* Road       */
/* char and mob-related defines *****************************************/


/* PC classes */
#define CLASS_UNDEFINED	   -1
#define CLASS_WARRIOR       0
#define CLASS_CLERIC        1
#define CLASS_THIEF         2
#define CLASS_MAGIC_USER    3
#define CLASS_RANGER        4
#define CLASS_BARD          5
#define CLASS_MONK          6
#define CLASS_UNUSED        7
#define CLASS_BARBARIAN     8
#define CLASS_PALADIN       9
#define CLASS_ANTI_PALADIN 10
#define CLASS_DRUID        11
#define CLASS_MERCHANT	   12	
#define CLASS_KENSAI       13  /* added for expansion */
#define CLASS_ASSASSIN     14  /* added for expansion */
#define CLASS_NECROMANCER  15  /* added for expansion */
#define CLASS_DEVA         16  /* added for expansion */
#define CLASS_IMMORTAL     17  /* added to control availability of spells */
#define CLASS_GOD          18  /* added to control availability of spells */


#define NUM_CLASSES	  19  /* This must be the number of classes!! */



/* NPC classes */
#define MCLASS_WARRIOR       0
#define MCLASS_CLERIC        1
#define MCLASS_THIEF         2
#define MCLASS_MAGIC_USER    3
#define MCLASS_RANGER        4
#define MCLASS_BARD          5
#define MCLASS_MONK          6
#define MCLASS_UNUSED        7
#define MCLASS_BARBARIAN     8
#define MCLASS_PALADIN       9
#define MCLASS_ANTI_PALADIN 10
#define MCLASS_DRUID        11
#define MCLASS_MERCHANT	    12	
#define MCLASS_KENSAI       13  /* added for expansion */
#define MCLASS_ASSASSIN     14  /* added for expansion */
#define MCLASS_NECROMANCER  15  /* added for expansion */
#define MCLASS_DEVA         16  /* added for expansion */
#define MCLASS_IMMORTAL     17  /* added to control availability of spells */
#define MCLASS_GOD          18  /* added to control availability of spells */
#define MCLASS_NONE         19  /* Your basic noclass mob */

/* 10/26/96, Echo - races added. See races.c for more details. */
#define RACE_UNDEFINED   -1
#define RACE_HUMAN        0 /* same - Human              */
#define RACE_ELF          1 /* Elf replaces Grey Elf     */
#define RACE_H_ELF        2 /*  new - Half Elf   	 */
#define RACE_D_ELF        3 /* same - Dark Elf (or Drow) */
#define RACE_DWARF        4 /* same - Dwarf    		 */
#define RACE_HALFLING     5 /*  new - Halfling 		 */
#define RACE_SPRITE       6 /* same - Sprite   		 */
#define RACE_MINOTAUR     7 /* same - Minotaur 		 */
#define RACE_AVIAN        8 /*  new - Avian    		 */
#define RACE_H_OGRE       9 /* Half Ogre replaces Ogre   */
#define RACE_H_ORC       10 /* Half Orc replaces Orc     */
#define RACE_DRACONIAN   11 /* for expansion / remort option */
#define RACE_SHADOW      12 /* for expansion / remort option */
#define RACE_TITAN       13 /* for expansion / remort option */
#define RACE_AESIR       14 /* for expansion / remort option */
#define RACE_UNUSED      15 /* used to force players to reselect race */

#define NUM_RACES        15 /* RACE_UNUSED doesn't count here */

/* NPC Races */
#define MRACE_HUMAN        0 /* same - Human              */
#define MRACE_ELF          1 /* Elf replaces Grey Elf     */
#define MRACE_H_ELF        2 /*  new - Half Elf   	 */
#define MRACE_D_ELF        3 /* same - Dark Elf (or Drow) */
#define MRACE_DWARF        4 /* same - Dwarf    		 */
#define MRACE_HALFLING     5 /*  new - Halfling 		 */
#define MRACE_SPRITE       6 /* same - Sprite   		 */
#define MRACE_MINOTAUR     7 /* same - Minotaur 		 */
#define MRACE_AVIAN        8 /*  new - Avian    		 */
#define MRACE_H_OGRE       9 /* Half Ogre replaces Ogre   */
#define MRACE_H_ORC       10 /* Half Orc replaces Orc     */
#define MRACE_DRACONIAN   11
#define MRACE_SHADOW      12
#define MRACE_TITAN       13
#define MRACE_AESIR       14
#define MRACE_HUMANOID    15
#define MRACE_UNDEAD      16
#define MRACE_ANIMAL      17
#define MRACE_GIANT       18
#define MRACE_PLANT       19
#define MRACE_FISH        20
#define MRACE_INSECT      21
#define MRACE_DEMON       22
#define MRACE_DRAGON      23
#define MRACE_EQUINE      24
#define MRACE_STATUE      25
#define MRACE_EARTH       26
#define MRACE_FIRE        27
#define MRACE_WATER       28
#define MRACE_AIR         29
#define MRACE_ORC         30
#define MRACE_OGRE        31
#define MRACE_GOBLIN      32
#define MRACE_TROLL       33
#define MRACE_GNOME       34
#define MRACE_EYE         35
#define MRACE_REPTILE     36
#define MRACE_SLIMEMOLD   37
#define MRACE_KOBOLD      38
#define MRACE_OTHER       39
#define MRACE_GHOST       40
#define MRACE_IMMATERIAL  41
#define MRACE_ANGEL       42
#define MRACE_GNOLL       43

/* Sex */
#define SEX_NEUTRAL   0
#define SEX_MALE      1
#define SEX_FEMALE    2

/* Updated to match Phoenix */
/* Positions */
#define POS_DEAD       0	/* dead			*/
#define POS_MORTALLYW  1	/* mortally wounded	*/
#define POS_INCAP      2	/* incapacitated	*/
#define POS_STUNNED    3	/* stunned		*/
#define POS_SLEEPING   4	/* sleeping		*/
#define POS_CHANT      5        /* chanting             */
#define POS_MEDITATE   6        /* meditating           */
#define POS_BANDAGE    7        /* bandaged             */
#define POS_RESTING    8	/* resting		*/
#define POS_SITTING    9	/* sitting		*/
#define POS_FIGHTING  10	/* fighting		*/
#define POS_STANDING  11	/* standing		*/


/* Player flags: used by char_data.char_specials.act */
#define PLR_KILLER	(1 << 0)   /* Player is a player-killer		*/
#define PLR_THIEF	(1 << 1)   /* Player is a player-thief		*/
#define PLR_FROZEN	(1 << 2)   /* Player is frozen			*/
#define PLR_DONTSET     (1 << 3)   /* Don't EVER set (ISNPC bit)	*/
#define PLR_WRITING	(1 << 4)   /* Player writing (board/mail/olc)	*/
#define PLR_MAILING	(1 << 5)   /* Player is writing mail		*/
#define PLR_CRASH	(1 << 6)   /* Player needs to be crash-saved	*/
#define PLR_SITEOK	(1 << 7)   /* Player has been site-cleared	*/
#define PLR_NOSHOUT	(1 << 8)   /* Player not allowed to shout/goss	*/
#define PLR_NOTITLE	(1 << 9)   /* Player not allowed to set title	*/
#define PLR_DELETED	(1 << 10)  /* Player deleted - space reusable	*/
#define PLR_LOADROOM	(1 << 11)  /* Player uses nonstandard loadroom	*/
#define PLR_NOWIZLIST	(1 << 12)  /* Player shouldn't be on wizlist	*/
#define PLR_NODELETE	(1 << 13)  /* Player shouldn't be deleted	*/
#define PLR_INVSTART	(1 << 14)  /* Player should enter game wizinvis	*/
#define PLR_CRYO	(1 << 15)  /* Player is cryo-saved (purge prog)	*/
#define PLR_LINKLOADED  (1 << 16)  /* Player is link-loaded		*/
#define PLR_REIMBED     (1 << 17)  /* Player has used the reimburse cmd	*/
#define PLR_STUNNED     (1 << 18)  /* Player has been stunned by magic  */
#define PLR_FISHING     (1 << 19)  /* Player has a line in the water   */
#define PLR_FISH_ON     (1 << 20)  /* Player has a fish on their line  */
#define PLR_PK          (1 << 21)  /* Player has purchased a PK flag */
#define PLR_NOCOMMUNE   (1 << 22)  /* Player can't commune */
#define PLR_CHARTER     (1 << 23)  /* Player is editing their clan charter */

/*** 2 Extra sets of 32 flags with act2 and act3  *****/

/* Updated to match Phoenix */
/* Mobile flags: used by char_data.char_specials.act */
#define MOB_SPEC         (1 << 0)  /* Mob has a callable spec-proc	*/
#define MOB_SENTINEL     (1 << 1)  /* Mob should not move		*/
#define MOB_SCAVENGER    (1 << 2)  /* Mob picks up stuff on the ground	*/
#define MOB_ISNPC        (1 << 3)  /* (R) Automatically set on all Mobs	*/
#define MOB_AWARE	 (1 << 4)  /* Mob can't be backstabbed		*/
#define MOB_AGGRESSIVE   (1 << 5)  /* Mob hits players in the room	*/
#define MOB_STAY_ZONE    (1 << 6)  /* Mob shouldn't wander out of zone	*/
#define MOB_WIMPY        (1 << 7)  /* +10 to morale (greater chance to flee)*/
#define MOB_AGGR_EVIL	 (1 << 8)  /* auto attack evil PC's		*/
#define MOB_AGGR_GOOD	 (1 << 9)  /* auto attack good PC's		*/
#define MOB_AGGR_NEUTRAL (1 << 10) /* auto attack neutral PC's		*/
#define MOB_MEMORY	 (1 << 11) /* remember attackers if attacked	*/
#define MOB_HELPER	 (1 << 12) /* attack PCs fighting other NPCs	*/
#define MOB_HUNT_KILLER  (1 << 13) /* Will hunt player killers		*/
#define MOB_HUNT_MEMORY  (1 << 14) /* Will hung players in memory	*/
#define MOB_NOCHARM	 (1 << 15) /* Mob can't be charmed		*/
#define MOB_NOSUMMON	 (1 << 16) /* Mob can't be summoned		*/
#define MOB_NOSLEEP	 (1 << 17) /* Mob can't be slept		*/
#define MOB_NOBASH	 (1 << 18) /* Mob can't be bashed (e.g. trees)	*/
#define MOB_NOBLIND	 (1 << 19) /* Mob can't be blinded		*/
#define MOB_MOUNT        (1 << 20) /* Is the mob mountable? (DAK)       */
#define MOB_NOGIVE	 (1 << 21) /* Cannot give items to this mob	*/
#define MOB_CITIZEN      (1 << 22)
#define MOB_HAPPY        (1 << 23) 
#define MOB_SAD          (1 << 24) 
#define MOB_NOMOOD       (1 << 25) /* the mob doesn't do socials */
#define MOB_GOPATH       (1 << 26) /* the mob is going somewhere*/
#define MOB_GUARD        (1 << 27) /* mob will autoassist */
#define MOB_FLY          (1 << 28) /* mob is flying (obsolete)*/
#define MOB_FOOLHARDY    (1 << 29) /* -5 to morale (less chance to flee)*/
#define MOB_STAY_TERRAIN (1 << 30) /* Mob shouldn't wander out of a terrain type */
#define MOB_MASTER       (1 << 31) /* Mob is a master of something      */
/**** 2 Extra sets of 32 flags with act2 and act3 *****/
#define MOB2_NOTRIP      (1 << 0)  /* no trip this mob */
#define MOB2_NOSTUN      (1 << 1)  /* no stun this mob */
#define MOB2_NOSWEEP     (1 << 2)  /* no sweep this mob */
#define MOB2_COMPONENT   (1 << 3)  /* component mobs */
#define MOB2_NODISARM    (1 << 4)  /* can't disarm the mob              */
#define MOB2_SUMMONABLE  (1 << 5)
#define MOB2_WEAPONSMITH (1 << 6)  /* Mob works with weapons            */
#define MOB2_ARMORER     (1 << 7)  /* Mob works with armor              */
#define MOB2_JEWELER     (1 << 8)  /* Mob works with accessories        */

/* Preference flags: used by char_data.char_specials.pref */
#define PRF_BRIEF       (1 << 0)  /* Room descs won't normally be shown	*/
#define PRF_COMPACT     (1 << 1)  /* No extra CRLF pair before prompts	*/
#define PRF_DEAF	(1 << 2)  /* Can't hear shouts			*/
#define PRF_NOTELL	(1 << 3)  /* Can't receive tells		*/
#define PRF_DISPHP	(1 << 4)  /* Display hit points in prompt	*/
#define PRF_DISPMANA	(1 << 5)  /* Display mana points in prompt	*/
#define PRF_DISPMOVE	(1 << 6)  /* Display move points in prompt	*/
#define PRF_AUTOEXIT	(1 << 7)  /* Display exits in a room		*/
#define PRF_NOHASSLE	(1 << 8)  /* Aggr mobs won't attack		*/
#define PRF_QUEST	(1 << 9)  /* On quest				*/
#define PRF_SUMMONABLE	(1 << 10) /* Can be summoned			*/
#define PRF_NOREPEAT	(1 << 11) /* No repetition of comm commands	*/
#define PRF_HOLYLIGHT	(1 << 12) /* Can see in dark			*/
#define PRF_COLOR_1	(1 << 13) /* Color (low bit)			*/
#define PRF_COLOR_2	(1 << 14) /* Color (high bit)			*/
#define PRF_NOWIZ	(1 << 15) /* Can't hear wizline			*/
#define PRF_LOG1	(1 << 16) /* On-line System Log (low bit)	*/
#define PRF_LOG2	(1 << 17) /* On-line System Log (high bit)	*/
#define PRF_NOAUCT	(1 << 18) /* Can't hear auction channel		*/
#define PRF_NOGOSS	(1 << 19) /* Can't hear gossip channel		*/
#define PRF_NOGRATZ	(1 << 20) /* Can't hear grats channel		*/
#define PRF_ROOMFLAGS	(1 << 21) /* Can see room flags (ROOM_x)	*/
#define PRF_INFOBAR     (1 << 22) /* -naj infobar2 12/16/96 - pref flags */
#define PRF_SCOREBAR    (1 << 23) /* -naj infobar2 12/16/96 - pref flags */
#define PRF_METER	(1 << 24) /* -naj infobar2 12/16/96 - pref flags */
#define PRF_ASCII       (1 << 25) /* -naj infobar2 12/16/96 - pref flags */
#define PRF_AUTOSPLIT   (1 << 26) /* autosplit-snippets page --Erika    */
#define PRF_AUTOLOOT    (1 << 27) /* autoloot-snippets page --Erika     */
#define PRF_NOBATTLE	(1 << 28) /* not in battle - Anduin		*/
#define PRF_AUTOGOLD	(1 << 29) /* autogold -Anduin			*/
#define PRF_AUTOSAC	(1 << 30) /* autosac - Anduin			*/
#define PRF_AUTOASSIST  (1 << 31) /* autoassist - Anduin		*/

#define PRF2_DISPGOLD	(1 << 0)  /* gold for prompt - Anduin		*/
#define PRF2_DISPEXP	(1 << 1)  /* exp for prompt - Anduin 		*/
#define PRF2_DISPALIGN	(1 << 2)  /* align for prompt - Anduin 		*/
#define PRF2_DISPMAX	(1 << 3)  /* max stuff for prompt - Anduin 	*/
#define PRF2_AFK        (1 << 4)  /* if afk - masque                    */
#define PRF2_FLY        (1 << 5)  /* if flying - masque  (OBSOLETE)     */
#define PRF2_NOOOC      (1 << 6)  /* Can't hear ooc channel             */
#define PRF2_NOREMO     (1 << 7)  /* Can't hear the remort channel      */
#define PRF2_NOCSAY     (1 << 8)  /* Can't hear the clan channel        */
#define PRF2_EMPROG     (1 << 9)  /* Can edit mob progs                 */
#define PRF2_TMPROG     (1 << 10) /* Can allow others to edit mob progs */
#define PRF2_PAGE_OK    (1 << 11) /* Wether you wanna be paged or not   */
#define PRF2_NOINFO     (1 << 12) /* Can't hear the info channel        */
#define PRF2_NOSPAM     (1 << 13) /* Can't see fight spam               */
#define PRF2_DG_ATTACH  (1 << 14) /* Can attach dg-scripts to things    */
#define PRF2_HEDIT      (1 << 15) /* Can use hedit                      */
#define PRF2_RECALLABLE (1 << 16) /* recallable - Faron 		*/
#define PRF2_DISPTIME   (1 << 17) /* display time in prompt for imms    */ 
#define PRF2_DISPDATE   (1 << 18) /* display date in prompt for imms    */ 
#define PRF2_NOMUSIC    (1 << 19) /* Can't hear music channel           */
#define PRF2_MORTAL     (1 << 20) /* For imms.  Can they take damage?   */
#define PRF2_NONEWBIE   (1 << 21) /* Can't hera newbie channel          */
#define PRF2_DISPEXPLORED (1<<22) /* display explored in current zone   */
/*** 2 Extra sets of 32 flags with pref2 and pref3  ****/

/* Updated to match Phoenix */
/* Affect bits: used in char_data.char_specials.saved.affected_by */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
#define AFF_BLIND             (1 << 0)	   /* (R) Char is blind		*/
#define AFF_INVISIBLE         (1 << 1)	   /* Char is invisible		*/
#define AFF_NOTRACK	      (1 << 2)	   /* Char can't be tracked	*/
#define AFF_DETECT_INVIS      (1 << 3)	   /* Char can see invis chars  */
#define AFF_DETECT_MAGIC      (1 << 4)	   /* Char is sensitive to magic*/
#define AFF_SENSE_LIFE        (1 << 5)	   /* Char can sense hidden life*/
#define AFF_RAGE              (1 << 6)     /* (R) Char is raged         */
#define AFF_SANCTUARY         (1 << 7)	   /* Char protected by sanct.	*/
#define AFF_GROUP             (1 << 8)	   /* (R) Char is grouped	*/
#define AFF_CURSE             (1 << 9)	   /* Char is cursed		*/
#define AFF_TAMED   	      (1 << 10)	   /* Char has been tamed(MOUNTS)*/
#define AFF_POISON            (1 << 11)	   /* (R) Char is poisoned	*/
#define AFF_PROTECT_EVIL      (1 << 12)	   /* Char protected from evil  */
#define AFF_PARALYSIS         (1 << 13)	   /* Char is paralyzed		*/
#define AFF_WATERWALK	      (1 << 14)	   /* Char can walk on water	*/
#define AFF_PASS_DOOR         (1 << 15)    /* Char can pass doors       */
#define AFF_SLEEP             (1 << 16)	   /* (R) Char magically asleep	*/
#define AFF_NO_FLEE           (1 << 17)    /* Char can't flee the room  */
#define AFF_SNEAK             (1 << 18)	   /* Char can move quietly	*/
#define AFF_HIDE              (1 << 19)	   /* Char is hidden		*/
#define AFF_PROTECT_GOOD      (1 << 20)	   /* Char protected from good  */
#define AFF_CHARM             (1 << 21)	   /* Char is charmed		*/
#define AFF_ULTRAVISION	      (1 << 22)    /* Char can see in dark	*/
#define AFF_FLY 	      (1 << 23)	   /* The char can fly  	*/
#define AFF_INFRAVISION       (1 << 24)	   /* Char can see in heat sourc*/
#define AFF_HASTE	      (1 << 25)    /* Char is hasted            */
#define AFF_SLOW              (1 << 26)    /* Char is slowed            */
#define AFF_DETECT_ALIGN      (1 << 27)	   /* Char is sensitive to align*/
#define AFF_LEV		      (1 << 28)    /* Char is levitated         */
#define AFF_DREAM             (1 << 29)    /* Spell Dream		*/
#define AFF_PLAGUE            (1 << 30)    /* Char is plagued		*/
#define AFF_WATER_BREATHE     (1 << 31)	   /* Char can breath in water  */

#define AFF2_FLYING           (1 << 0) /* char is flying                */
#define AFF2_DIGGING          (1 << 1) /* char is digging               */
#define AFF2_RESIST_BLIND     (1 << 2) /* char is resistant to blind    */
#define AFF2_SKINNING         (1 << 3) /* char is skinning              */
#define AFF2_FIRESHIELD       (1 << 4) /* char has fire shield          */
#define AFF2_SHADOW           (1 << 5) /* character is hidden in shadow */
#define AFF2_ROVE             (1 << 6) /* char is roving in forests, etc*/
#define AFF2_WARY             (1 << 7) /* char is wary of ambush/backstab*/
#define AFF2_CANNOT_FLEE      (1 << 8) /* char used NO_FLEE item in battle*/
#define AFF2_PICKING_STAY     (1 << 9) /* char is picking a door exit or an object on the ground and should not move. */
#define AFF2_PICKING          (1 << 10) /* char is picking an object in his/her inventory. */

/**** 2 Extra sets of 32 flags with affected_by2 and affected_by3  ****/

/* Modes of connectedness: used by descriptor_data.state */
#define CON_PLAYING	 0		/* Playing - Nominal state	*/
#define CON_CLOSE	 1		/* Disconnecting		*/
#define CON_GET_NAME	 2		/* By what name ..?		*/
#define CON_NAME_CNFRM	 3		/* Did I get that right, x?	*/
#define CON_PASSWORD	 4		/* Password:			*/
#define CON_NEWPASSWD	 5		/* Give me a password for x	*/
#define CON_CNFPASSWD	 6		/* Please retype password:	*/
#define CON_QSEX	 7		/* Sex?				*/
#define CON_QRACE        8              /* Race?                        */
#define CON_QCLASS	 9		/* Class?			*/
#define CON_RMOTD	 10		/* PRESS RETURN after MOTD	*/
#define CON_MENU	 11		/* Your choice: (main menu)	*/
#define CON_EXDESC	 12		/* Enter a new description:	*/
#define CON_CHPWD_GETOLD 13		/* Changing passwd: get old	*/
#define CON_CHPWD_GETNEW 14		/* Changing passwd: get new	*/
#define CON_CHPWD_VRFY   15		/* Verify new password		*/
#define CON_DELCNF1	 16		/* Delete confirmation 1	*/
#define CON_DELCNF2	 17		/* Delete confirmation 2	*/
#define CON_OEDIT	 18		/*. OLC mode - object edit     .*/
#define CON_REDIT	 19		/*. OLC mode - room edit       .*/
#define CON_ZEDIT	 20		/*. OLC mode - zone info edit  .*/
#define CON_MEDIT	 21		/*. OLC mode - mobile edit     .*/
#define CON_SEDIT	 22		/*. OLC mode - shop edit       .*/
#define CON_GEDIT	 23		/*. OLC mode - guild edit      .*/
#define CON_PEDIT	 24		/*. OLC mode - path edit       .*/
#define CON_IDCONING     25             /* waiting for ident connection */
#define CON_IDCONED      26             /* ident connection complete    */
#define CON_IDREADING    27             /* waiting to read ident sock   */
#define CON_IDREAD       28             /* ident results read           */
#define CON_ASKNAME      29             /* Ask user for name            */     
#define CON_QSTAT	 30		/* Stat selection		*/
#define CON_DISCONNECT   31	        /* In-game disconnection        */
#define CON_TEXTED       32	        /* editing text files           */
#define CON_TRIGEDIT     33	        /* OLC mode - trigger edit      */
#define CON_HEDIT        34             /* OLC mode - help edit         */
#define CON_ASSEDIT      35             /* OLC mode - assembly edit     */
#define CON_ADD_NEWS     36             /* Add news                     */
#define CON_FIRST_TIME   37             /* During creation, choose a hometown. */
#define CON_HOME_TOWN    38             /* Confirm hometown. */
#define CON_EDIT_EMAIL   39             /* At main menu, editing e-mail address. */

#define OOB_NONE 0
#define OOB_MSDP (1 << 0)
#define OOB_GMCP (1 << 1)
#define OOB_REPORT_STATS (1 << 2)
#define OOB_REPORT_ROOM (1 << 3)

#define MSDP 69
#define MSDP_VAR 1
#define MSDP_VAL 2
#define MSDP_TABLE_OPEN 3
#define MSDP_TABLE_CLOSE 4
#define MSDP_ARRAY_OPEN 5
#define MSDP_ARRAY_CLOSE 6

/* Updated to match Phoenix */
/* Character equipment positions: used as index for char_data.equipment[] */
/* NOTE: Don't confuse these constants with the ITEM_ bitvectors
   which control the valid places you can wear a piece of equipment */
#define WEAR_UNUSED     0
#define WEAR_FINGER_R   1
#define WEAR_FINGER_L   2
#define WEAR_NECK_1     3
#define WEAR_NECK_2     4
#define WEAR_BODY       5
#define WEAR_HEAD       6
#define WEAR_LEGS       7
#define WEAR_FEET       8
#define WEAR_HANDS      9
#define WEAR_ARMS      10
#define WEAR_SHIELD    11
#define WEAR_ABOUT     12
#define WEAR_WAIST     13
#define WEAR_WRIST_R   14
#define WEAR_WRIST_L   15
#define WEAR_WIELD_1   16
#define WEAR_WIELD_2   17
#define WEAR_HOLD_1    18
#define WEAR_HOLD_2    19
#define WEAR_EAR_L     20	/* New EQ positions--Aleks */
#define WEAR_EAR_R     21	/* New EQ positions--Aleks */
#define WEAR_FACE      22	/* New EQ positions--Aleks */
#define WEAR_BACK      23	/* New EQ positions--Aleks */
#define WEAR_HEART     24       /* New EQ position --Faron */

#define NUM_WEARS      25	/* This must be the # of eq positions!! */
#define NUM_HAND_POSITIONS 5


/* object-related defines ********************************************/
/* Added by Ponder for materials */
/* Material types: used by obj_data.material */
#define MATERIAL_UNDEFINED  0           /* Item is undefined            */
#define MATERIAL_SKIN       1           /* Item is made of skin         */
#define MATERIAL_FUR        2           /* Item is made of fur          */
#define MATERIAL_IVORY      3           /* Item is made of ivory        */
#define MATERIAL_CLOTH      4           /* Item is made of cloth        */
#define MATERIAL_BONE       5           /* Item is made of bone         */
#define MATERIAL_STONE      6           /* Item is made of stone        */
#define MATERIAL_PAPER      7           /* Item is made of paper        */ 
#define MATERIAL_WOOD       8           /* Item is made of wood         */
#define MATERIAL_GLASS      9           /* Item is made of glass        */
#define MATERIAL_COPPER     10          /* Item is made of copper       */
#define MATERIAL_IRON       11          /* Item is made of iron         */
#define MATERIAL_BRONZE     12          /* Item is made of bronze       */
#define MATERIAL_STEEL      13          /* Item is made of steel        */
#define MATERIAL_ADAMANTITE 14          /* Item is made of adamantite   */
#define MATERIAL_GOLD       15          /* Item is made of gold         */
#define MATERIAL_SILVER     16          /* Item is made of silver       */
#define MATERIAL_PLATINUM   17          /* Item is made of platinum     */
#define MATERIAL_DIAMOND    18          /* Item is made of diamond      */
#define MATERIAL_RUBY       19          /* Item is made of ruby         */
#define MATERIAL_SAPPHIRE   20          /* Item is made of sapphire     */
#define MATERIAL_EMERALD    21          /* Item is made of emerald      */
#define MATERIAL_CRYSTAL    22          /* Item is made of pearl        */
#define MATERIAL_LIQUID     23          /* Item is made of liquid       */
#define MATERIAL_ETHER      24          /* Item is made of ether        */
#define MATERIAL_AIR        25          /* Item is made of air          */
#define MATERIAL_FIRE       26          /* Item is made of fire         */
#define MATERIAL_FOOD       27          /* Item is food                 */
#define MATERIAL_PLANT      28          /* Item is plant                */
#define MATERIAL_TITANIUM   29          /* Item is made of titanium     */
#define MATERIAL_WAX        30          /* Item is made of wax          */
#define MATERIAL_CARCASS    31          /* Item is a corpse             */
#define MATERIAL_MITHRIL    32          /* Item is made of mithril      */
#define MATERIAL_FEATHER    33          /* Item is made of feather      */
#define MATERIAL_DRAGONSCALE 34         /* Item is made of dragonscale  */
#define MATERIAL_ICE        35          /* Item is made of ice          */
#define MATERIAL_TIN        36          /* Item is made of tin          */
#define MATERIAL_BRASS      37          /* Item is made of brass        */
#define MATERIAL_HEMP       38          /* Item is made of hemp         */
#define MATERIAL_ORE        39
#define MATERIAL_MINERAL    40

#define INDESTRUCTABLE      0	        /* 0 Tslots mean an obj is IND  */

struct obj_material_affs 
{
   int  cost_per_lb;
   int  bulk_per_lb;
   long sucept_dam_vect;
   long resist_dam_vect;
   long class_restrict;
   long race_restrict;
   int  default_dam_slots;
};

/* Updated to match Phoenix */
/* Item types: used by obj_data.obj_flags.type_flag */
#define ITEM_LIGHT      1		/* Item is a light source	*/
#define ITEM_SCROLL     2		/* Item is a scroll		*/
#define ITEM_WAND       3		/* Item is a wand		*/
#define ITEM_STAFF      4		/* Item is a staff		*/
#define ITEM_WEAPON     5		/* Item is a weapon		*/
#define ITEM_FIREWEAPON 6		/* Unimplemented		*/
#define ITEM_MISSILE    7		/* Unimplemented		*/
#define ITEM_TREASURE   8		/* Item is a treasure, not gold	*/
#define ITEM_ARMOR      9		/* Item is armor		*/
#define ITEM_POTION    10 		/* Item is a potion		*/
#define ITEM_WORN      11		/* Unimplemented		*/
#define ITEM_OTHER     12		/* Misc object			*/
#define ITEM_TRASH     13		/* Trash - shopkeeps won't buy	*/
#define ITEM_TRAP      14		/* Unimplemented		*/
#define ITEM_CONTAINER 15		/* Item is a container		*/
#define ITEM_NOTE      16		/* Item is note 		*/
#define ITEM_DRINKCON  17		/* Item is a drink container	*/
#define ITEM_KEY       18		/* Item is a key		*/
#define ITEM_FOOD      19		/* Item is food			*/
#define ITEM_MONEY     20		/* Item is money (gold)		*/
#define ITEM_PEN       21		/* Item is a pen		*/
#define ITEM_BOAT      22		/* Item is a boat		*/
#define ITEM_FOUNTAIN  23		/* Item is a fountain		*/
/* Refuelable light mod--Aleks */
#define ITEM_FUEL      24		/* Item is fuel for a light	*/
/* Pill modification--Aleks */
#define ITEM_PILL      25		/* Item is a pill		*/
#define ITEM_THROW     26
#define ITEM_GRENADE   27
#define ITEM_BOW       28
#define ITEM_SLING     29
#define ITEM_CROSSBOW  30
#define ITEM_BOLT      31
#define ITEM_ARROW     32
#define ITEM_ROCK      33
#define ITEM_PORTAL    34
#define ITEM_FURNITURE 35
#define ITEM_TICKET    36
#define ITEM_STABLE_TICKET 37
#define ITEM_SHOVEL    38
#define ITEM_POLE      39               /* Fishing Pole         */

/* Refuelable light mod--Aleks */
#define FUEL_NONE	0
#define FUEL_OIL	1
#define FUEL_COAL	2
#define FUEL_FAT	3
#define FUEL_WOOD       4

/* Updated to match Phoenix */
/* Take/Wear flags: used by obj_data.obj_flags.wear_flags */
#define ITEM_WEAR_TAKE		(1 << 0)  /* Item can be takes		*/
#define ITEM_WEAR_FINGER	(1 << 1)  /* Can be worn on finger	*/
#define ITEM_WEAR_NECK		(1 << 2)  /* Can be worn around neck 	*/
#define ITEM_WEAR_BODY		(1 << 3)  /* Can be worn on body 	*/
#define ITEM_WEAR_HEAD		(1 << 4)  /* Can be worn on head 	*/
#define ITEM_WEAR_LEGS		(1 << 5)  /* Can be worn on legs	*/
#define ITEM_WEAR_FEET		(1 << 6)  /* Can be worn on feet	*/
#define ITEM_WEAR_HANDS		(1 << 7)  /* Can be worn on hands	*/
#define ITEM_WEAR_ARMS		(1 << 8)  /* Can be worn on arms	*/
#define ITEM_WEAR_SHIELD	(1 << 9)  /* Can be used as a shield	*/
#define ITEM_WEAR_ABOUT		(1 << 10) /* Can be worn about body 	*/
#define ITEM_WEAR_WAIST 	(1 << 11) /* Can be worn around waist 	*/
#define ITEM_WEAR_WRIST		(1 << 12) /* Can be worn on wrist 	*/
#define ITEM_WEAR_WIELD		(1 << 13) /* Can be wielded		*/
#define ITEM_WEAR_HOLD		(1 << 14) /* Can be held		*/
#define ITEM_AMMO_UBUS  	(1 << 15) /* PLACE HOLDER		*/
#define ITEM_LIGHT_SOURCE_UNUSE (1 << 16) /* PLACE HOLDER  		*/
#define ITEM_WEAR_EAR		(1 << 17) /* New EQ positions--Aleks	*/
#define ITEM_WEAR_FACE		(1 << 18) /* New EQ positions--Aleks	*/
#define ITEM_WEAR_BACK		(1 << 19) /* New EQ positions--Aleks	*/
#define ITEM_WEAR_HEART         (1 << 20) /* New EQ position--Faron     */

/* Bitvector for 'zone flags' */

#define Z_IDLE         (1 << 0) /* idle zone flag */
#define Z_NO_RECALL    (1 << 1) /* a zone that you cannot recall from */    
#define Z_NO_SUMMON    (1 << 2) /* a zone that you cannot summon to-from */    
#define Z_NO_TRACK     (1 << 3) /* a zone that you cannot track in */    
#define Z_NO_TELEPORT  (1 << 4) /* a zone that you cannot teleport in */    
#define Z_QUEUED       (1 << 5) /* zone is queued to be reset don't requeue*/
#define Z_PKILL        (1 << 6)	/* zone is pkill */
#define Z_GRAFFITI     (1 << 7) /* zone can be graffiti'd. */

/* Updated to match Phoenix Flags --Aleks  */
/* Extra object flags: used by obj_data.obj_flags.extra_flags */
#define ITEM_GLOW          (1 << 0)	/* Item is glowing		*/
#define ITEM_HUM           (1 << 1)	/* Item is humming		*/
#define ITEM_DARK	   (1 << 2)	/* Item radiates Darkness	*/
#define ITEM_LIVE_GRENADE  (1 << 3)
#define ITEM_NEWBIE        (1 << 4)
#define ITEM_INVISIBLE     (1 << 5)	/* Item is invisible		*/
#define ITEM_MAGIC         (1 << 6)	/* Item is magical		*/
#define ITEM_NODROP        (1 << 7)	/* Item is cursed: can't drop	*/
#define ITEM_BLESS         (1 << 8)	/* Item is blessed		*/
#define ITEM_ANTI_GOOD     (1 << 9)	/* Not usable by good people	*/
#define ITEM_ANTI_EVIL     (1 << 10)	/* Not usable by evil people	*/
#define ITEM_ANTI_NEUTRAL  (1 << 11)	/* Not usable by neutral people	*/
#define ITEM_NORENT        (1 << 12)	/* Item cannot be rented	*/
#define ITEM_NODONATE      (1 << 13)	/* Item cannot be donated	*/
#define ITEM_NOINVIS	   (1 << 14)	/* Item cannot be made invis	*/
#define ITEM_NODECAY	   (1 << 15)	/* Item will not decay		*/
#define ITEM_TWO_HAND	   (1 << 16)
#define ITEM_NO_POS_CHK	   (1 << 17)	/* Don't check for valid pos on load*/
#define ITEM_NOSELL	   (1 << 18) 	/* Shopkeepers won't touch it   */
#define ITEM_NOAUC         (1 << 19) 	/* can't auction, sell for 1/4 value*/
#define ITEM_BRITTLE       (1 << 20) 	/* sustains damage x 2*/
#define ITEM_RESISTANT     (1 << 21) 	/* sustains damage / 2*/
#define ITEM_DO_ACT        (1 << 22)    /* give a message on grnd frm actdesc*/
#define ITEM_NO_REPAIR     (1 << 23) 	/* can't be fixed */
#define ITEM_DONATED       (1 << 24) 	/* has been donated. */
#define ITEM_BATTLE_ITEM   (1 << 25) 	/* used for battlefield -odinian*/
#define ITEM_PC_CORPSE     (1 << 26)    /* Players Corpse               */
#define ITEM_NPC_CORPSE    (1 << 27)    /* Mobs Corpse                  */
#define ITEM_UNIQUE_SAVE   (1 << 28)    /* Ascii rent files/Corpse saving*/
#define ITEM_SUN_DAMAGE    (1 << 29) 	/* Item is damaged by the sun   */
#define ITEM_UNUSED13      (1 << 30) 	/* */
#define ITEM_QUEST         (1 << 31)    /* Item is permanent            */

/* Extra object flags2: used by obj_data.obj_flags.extra_flags2 */
#define ITEM2_REMORT       (1 << 0)     /* Remort item restriction      */
#define ITEM2_DBLREMORT    (1 << 1)     /* Double Remort item restriction*/
#define ITEM2_BODYPART     (1 << 2)     /* Item is part of the NPC      */
#define ITEM2_NOLOCATE     (1 << 3)     /* Item doesn't show up on locate object. */

/* Extra object flags: used by obj_data.obj_flags.anti_flags */
#define ITEM_ANTI_WAR      (1 << 0) 	/* Not usable by warriors 	*/
#define ITEM_ANTI_CLE      (1 << 1) 	/* Not usable by clerics 	*/
#define ITEM_ANTI_THI      (1 << 2) 	/* Not usable by theives 	*/
#define ITEM_ANTI_MAG      (1 << 3) 	/* Not usable by magic users 	*/
#define ITEM_ANTI_RAN      (1 << 4) 	/* Not usable by rangers 	*/
#define ITEM_ANTI_BAD      (1 << 5) 	/* Not usable by bards 		*/
#define ITEM_ANTI_MON      (1 << 6) 	/* Not usable by monks 		*/
#define ITEM_ANTI_UNU      (1 << 7) 	/* Not usable by assasins 	*/
#define ITEM_ANTI_BAR      (1 << 8) 	/* Not usable by barbarians 	*/
#define ITEM_ANTI_PAL      (1 << 9) 	/* Not usable by paladins 	*/
#define ITEM_ANTI_APA      (1 << 10) 	/* Not usable by anti-paladins 	*/
#define ITEM_ANTI_DRU      (1 << 11) 	/* Not usable by druids 	*/
#define ITEM_ANTI_MER	   (1 << 12)	/* Not usable by Merchants	*/
#define ITEM_ANTI_KEN	   (1 << 13)	/* Not usable by Remort1	*/
#define ITEM_ANTI_ASS	   (1 << 14)	/* Not usable by Remort2	*/
#define ITEM_ANTI_NEC	   (1 << 15)	/* Not usable by Remort3	*/
#define ITEM_ANTI_DEV	   (1 << 16)	/* Not usable by Remort4	*/
#define ITEM_ANTI_HUM	   (1 << 17)	/* Not usable by Human  	*/
#define ITEM_ANTI_ELF	   (1 << 18)	/* Not usable by Elf	        */
#define ITEM_ANTI_HLF	   (1 << 19)	/* Not usable by 1/2 Elf	*/
#define ITEM_ANTI_DLF	   (1 << 20)	/* Not usable by Dark Elf	*/
#define ITEM_ANTI_DWF	   (1 << 21)	/* Not usable by Dwarf  	*/
#define ITEM_ANTI_HFNG	   (1 << 22)	/* Not usable by Halfling	*/
#define ITEM_ANTI_SPR	   (1 << 23)	/* Not usable by Sprite 	*/
#define ITEM_ANTI_MIN	   (1 << 24)	/* Not usable by Minotaur	*/
#define ITEM_ANTI_AVI	   (1 << 25)	/* Not usable by Avian  	*/
#define ITEM_ANTI_HOG	   (1 << 26)	/* Not usable by Half Ogre	*/
#define ITEM_ANTI_HOR	   (1 << 27)	/* Not usable by Half Orc	*/
#define ITEM_ANTI_DRC	   (1 << 28)	/* Not usable by Dragon Lord	*/
#define ITEM_ANTI_SHD	   (1 << 29)	/* Not usable by Vulcan 	*/
#define ITEM_ANTI_TTN	   (1 << 30)	/* Not usable by Titian 	*/
#define ITEM_ANTI_ASR	   (1 << 31)	/* Not usable by Aesir  	*/

/* Updated to match Phoenix */
/* Modifier constants used with obj affects ('A' fields) */
#define APPLY_NONE              0	/* No effect			*/
#define APPLY_STR               1	/* Apply to strength		*/
#define APPLY_DEX               2	/* Apply to dexterity		*/
#define APPLY_INT               3	/* Apply to constitution	*/
#define APPLY_WIS               4	/* Apply to wisdom		*/
#define APPLY_CON               5	/* Apply to constitution	*/
#define APPLY_SEX		6	/* Apply to sex			*/
#define APPLY_CLASS             7	/* Reserved			*/
#define APPLY_LEVEL             8	/* Reserved			*/
#define APPLY_AGE               9	/* Apply to age			*/
#define APPLY_CHAR_WEIGHT      10	/* Apply to weight		*/
#define APPLY_CHAR_HEIGHT      11	/* Apply to height		*/
#define APPLY_MANA             12	/* Apply to max mana		*/
#define APPLY_HIT              13	/* Apply to max hit points	*/
#define APPLY_MOVE             14	/* Apply to max move points	*/
#define APPLY_GOLD             15	/* Reserved			*/
#define APPLY_EXP              16	/* Reserved			*/
#define APPLY_AC               17	/* Apply to Armor Class		*/
#define APPLY_ARMOR	       17	/* Apply to Armor Class		*/
#define APPLY_HITROLL          18	/* Apply to hitroll		*/
#define APPLY_DAMROLL          19	/* Apply to damage roll		*/
#define APPLY_SAVING_PARA      20	/* Apply to save throw: paralz	*/
#define APPLY_SAVING_ROD       21	/* Apply to save throw: rods	*/
#define APPLY_SAVING_PETRI     22	/* Apply to save throw: petrif	*/
#define APPLY_SAVING_BREATH    23	/* Apply to save throw: breath	*/
#define APPLY_SAVING_SPELL     24	/* Apply to save throw: spells	*/
#define APPLY_CHA	       25	/* Apply to Charisma		*/
#define APPLY_LIGHT	       26	/* From Phoenix--whats this for? */
#define APPLY_IMMUNE           27
#define APPLY_RESIST           28
#define APPLY_SUSC             29
#define APPLY_FLY              30
#define APPLY_SPELL_FAIL       31
#define APPLY_AFF2             32
#define APPLY_AFF3             33
#define APPLY_EAT_SPELL        34

#define TOP_APPLY1_NUM         32 /* for osearch, for the moment can't 
				     be increased */

/* Container flags - value[1] */
#define CONT_CLOSEABLE      (1 << 0)	/* Container can be closed	*/
#define CONT_PICKPROOF      (1 << 1)	/* Container is pickproof	*/
#define CONT_CLOSED         (1 << 2)	/* Container is closed		*/
#define CONT_LOCKED         (1 << 3)	/* Container is locked		*/

/* Light level defines */
#define VALUE_GLOW  2
#define VALUE_LIGHT 4 /* Used to be 10. --Modred */

/* Some different kind of liquids for use in values of drink containers */
#define LIQ_WATER      0
#define LIQ_BEER       1
#define LIQ_WINE       2
#define LIQ_ALE        3
#define LIQ_DARKALE    4
#define LIQ_WHISKY     5
#define LIQ_LEMONADE   6
#define LIQ_FIREBRT    7
#define LIQ_LOCALSPC   8
#define LIQ_SLIME      9
#define LIQ_MILK       10
#define LIQ_TEA        11
#define LIQ_COFFE      12
#define LIQ_BLOOD      13
#define LIQ_SALTWATER  14
#define LIQ_CLEARWATER 15
#define LIQ_BROTH      16


/* other miscellaneous defines *******************************************/

/* Player conditions */
#define DRUNK        0
#define FULL         1
#define THIRST       2


/* Sun state for weather_data */
#define SUN_DARK	0
#define SUN_RISE	1
#define SUN_LIGHT	2
#define SUN_SET		3

#define MOON_DARK       1
#define MOON_RISE       2
#define MOON_LIGHT      3
#define MOON_SET        4

#define MOON_NEW         0
#define MOON_QUART_WAX   1
#define MOON_HALF_WAX    2
#define MOON_3QUART_WAX  3
#define MOON_FULL        4
#define MOON_3QUART_WANE 5
#define MOON_HALF_WANE   6
#define MOON_QUART_WANE  7
#define MOON_BLUE        8

/* Sky conditions for weather_data */
#define SKY_CLOUDLESS	0
#define SKY_CLOUDY	1
#define SKY_RAINING	2
#define SKY_LIGHTNING	3


/* Rent codes */
#define RENT_UNDEF      0
#define RENT_CRASH      1
#define RENT_RENTED     2
#define RENT_CRYO       3
#define RENT_FORCED     4
#define RENT_TIMEDOUT   5


/* other #defined constants **********************************************/

/*
 * **DO**NOT** blindly change the number of levels in your MUD merely by
 * changing these numbers and without changing the rest of the code to match.
 * Other changes throughout the code are required.  See coding.doc for
 * details.
 */
/* 10/09/96, Echo - added god levels for Phoenix's 212 levels
 *   These definitions are mainly for command minimum levels and logs
 *   though some, like LVL_IMPL and LVL_IMMORT have wider significance
 * 2/21/97, Anduin - lowered levels to 112.
 * 6/18/98, Masque - back up to 125, but the growth was only in imm lvls
 * 10/15/17, Opie - Fixing a pretty nasty screw up with the imm lvls here.
 */

#define LVL_HIMPL       129  /*  Head Implementor  */ /* WizLevels Start */
#define LVL_IMPLII      128  /*  Implementor       */
#define LVL_IMPL        127  /*  Implementor       */
#define LVL_SIMP        126  /*  (Sub)Implementor  */
#define LVL_ADMIN       125  /*  Administrator     */
#define LVL_GRGODII     124  /*  Greater God       */
#define LVL_GRGODI      123  /*  Greater God       */
#define LVL_GRGOD       122  /*  Greater God       */
#define LVL_GODII       121  /*  God               */
#define LVL_GODI        120  /*  God               */
#define LVL_GOD         119  /*  God               */
#define LVL_DGODII      118  /*  Demi God          */
#define LVL_DGODI       117  /*  Demi God          */
#define LVL_DGOD        116  /*  Demi God          */
#define LVL_DETYII      115  /*  Deity             */
#define LVL_DETYI       114  /*  Deity             */
#define LVL_DETY        113  /*  Deity             */
#define LVL_SERPII      112  /*  Seraph            */
#define LVL_SERPI       111  /*  Seraph            */
#define LVL_SERP        110  /*  Seraph            */
#define LVL_ARCHII      109  /*  ArchAngel         */
#define LVL_ARCHI       108  /*  ArchAngel         */
#define LVL_ARCH        107  /*  ArchAngel         */
#define LVL_AMBASS		106  /*  Ambassador        */
#define LVL_IMMORT		105  /*  Ambassador        */ /* WizLevels End */
#define LVL_WANKER      104  /*  (DEMI-GOD)        */
#define LVL_AVATAR      103  /*  (AVATAR)          */
#define LVL_ANGEL       102  /*  (ANGEL)           */
#define LVL_HERO        101  /*  (HERO)            */

#define LVL_MAX_MORTAL  103  /*   Max mortal level */
#define LVL_NEWBIE      20
#define LVL_POTION_NEWBIE 30

#define LVL_FREEZE	LVL_SERP
#define LVL_NO_LOG      LVL_IMPL

/* for is_remort_level() */
#define NON_REMORT      0    /*   Non-Remorts      */
#define SINGLE_REMORT   1    /*   Single Remorts   */
#define DOUBLE_REMORT   2    /*   Double Remorts   */
#define TRIPLE_REMORT   3

/*
 * Immunities
 */
#define IMM_FIRE   (1 << 0)
#define IMM_COLD   (1 << 1)
#define IMM_ELEC   (1 << 2)
#define IMM_ENERGY (1 << 3)
#define IMM_BLUNT  (1 << 4)
#define IMM_PIERCE (1 << 5)
#define IMM_SLASH  (1 << 6)
#define IMM_ACID   (1 << 7)
#define IMM_POISON (1 << 8)
#define IMM_DRAIN  (1 << 9)
#define IMM_SLEEP  (1 << 10)
#define IMM_CHARM  (1 << 11)
#define IMM_HOLD   (1 << 12)
#define IMM_NONMAG (1 << 13)
#define IMM_PLUS1  (1 << 14)
#define IMM_PLUS2  (1 << 15)
#define IMM_PLUS3  (1 << 16)
#define IMM_PLUS4  (1 << 17)
#define IMM_STUN   (1 << 18)
#define IMM_HOLY   (1 << 19)
#define IMM_UNHOLY (1 << 20)




#define SPEC_ARRIVE     -1
#define SPEC_MOBACT     -2

#define NUM_OF_DIRS	6	/* number of directions in a room (nsewud) */
#define MAGIC_NUMBER    (0x06)	/* Arbitrary number that won't be in a string*/

#define OPT_USEC	 100000	/* 10 passes per second */
#define PASSES_PER_SEC	(1000000 / OPT_USEC)
#define RL_SEC		* PASSES_PER_SEC

#define PULSE_ZONE      (10 RL_SEC)
#define PULSE_ROOM      (10 RL_SEC)
#define PULSE_MOBILE    (1  RL_SEC)
#define PULSE_OBJECT    (10 RL_SEC)
#define PULSE_BUFFER    (5  RL_SEC)
#define PULSE_VIOLENCE  (1  RL_SEC) / 2
#define PULSE_MAGIC     (1  RL_SEC)
#define MOBILE_PERCENT 10

/* Skill lag times */
#define HALF_SKILL_COUNT           5  /* seconds */
#define FULL_SKILL_COUNT          10
#define SKILL_LAG      (1  RL_SEC)/2 /* same as PULSE_VIOLENCE */

/* Variables for the output buffering system */
#define HISTORY_SIZE            5   /* Keep Last 5 commands. */
#define SMALL_BUFSIZE		1024
#define MAX_SOCK_BUF		(12 * 1024)
#define GARBAGE_SPACE		32
#define MAX_PROMPT_LENGTH       140
#define LARGE_BUFSIZE        (MAX_SOCK_BUF - GARBAGE_SPACE - MAX_PROMPT_LENGTH)
#define MAX_STRING_LENGTH	8192
#define MAX_INPUT_LENGTH	256 /* Max length per *line* of input */
#define MAX_RAW_INPUT_LENGTH	512 /* Max size of *raw* input */
#define MAX_MESSAGES		100
#define MAX_NAME_LENGTH		20  /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_CLAN_NAME_LENGTH    20  /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_PWD_LENGTH		10  /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_TITLE_LENGTH	80  /* Used in char_file_u *DO*NOT*CHANGE* */
#define HOST_LENGTH		60  /* Used in char_file_u *DO*NOT*CHANGE* */
#define IDENT_LENGTH		8
#define EXDSCR_LENGTH		1024 /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_TONGUE		3   /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_SKILLS		600 /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_AFFECT		64  /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_OBJ_AFFECT		6   /* Used in obj_file_elem *DO*NOT*CHANGE* */
#define MAX_SPELL_AFFECT	3   /* Used in obj_file_elem *DO*NOT*CHANGE* */
#define MAX_POOF 		250 /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_OLC_ZONES		5   /* Used in obj_file_elem *DO*NOT*CHANGE* */


#define BFIELD_START 6900	    /* Start room of the battle field zone */
#define BFIELD_END 6963	    /* end room of the battle field zone */
/*
 * A MAX_PWD_LENGTH of 10 will cause BSD-derived systems with MD5 passwords
 * and GNU libc 2 passwords to be truncated.  On BSD this will enable anyone
 * with a name longer than 5 character to log in with any password.  If you
 * have such a system, it is suggested you change the limit to 20.
 *
 * Please note that this will erase your player files.  If you are not
 * prepared to do so, simply erase these lines but heed the above warning.
 */

#if defined(HAVE_UNSAFE_CRYPT) && MAX_PWD_LENGTH == 10
#error You need to increase MAX_PWD_LENGTH to at least 20.
#error See the comment near these errors for more explanation.
#endif

#define MAX_CHARMIES 2

#define RLIST_LEVEL 114
#define OLIST_LEVEL 114
#define MLIST_LEVEL 114
#define ZLIST_LEVEL 114
#define VSTAT_LEVEL 114
#define TLIST_LEVEL 114
#define TSTAT_LEVEL 114
#define STAT_ROOM_LEVEL  114
#define STAT_OBJ_LEVEL 114
#define STAT_MOB_LEVEL 114
#define LOAD_OBJ_LEVEL 114
#define LOAD_MOB_LEVEL 114
#define VNUM_ROOM_LEVEL 114
#define VNUM_OBJ_LEVEL 114
#define VNUM_MOB_LEVEL 114
#define VWEAR_LEVEL 114
#define MLEV_LEVEL 114
#define TCHECK_LEVEL 114
#define WALK_INTO_LEVEL 114
#define IMM_WHERE_LEVEL 114

/* Divide this number by 100 and that's the highest zone number
 * that will be recorded for exploring. */
#define EXPLORED_TOP_VNUM 40000
#define EXPLORED_BYTES (1+EXPLORED_TOP_VNUM/8)
#define EXPLORED_FILE "etc/explored"

#define BACKUP_TMP_DIR "/home/lucas/phoenix/e-mail"
#define BUILDERS_DIR "/home/lucas/phoenix/builders/circle/lib/world"

/**********************************************************************
* Structures                                                          *
**********************************************************************/


typedef signed char		sbyte;
typedef unsigned char		ubyte;
typedef signed short int	sh_int;
typedef unsigned short int	ush_int;
#if !defined(__cplusplus)
typedef char			bool;
#endif
typedef char	         	byte;

typedef long	room_vnum;
typedef long	obj_vnum;
typedef long	mob_vnum;
typedef long    zone_vnum;
typedef long    shop_vnum;
typedef long    trig_vnum;
typedef long    path_vnum;

typedef long	room_rnum;
typedef long	obj_rnum;
typedef long	mob_rnum;
typedef long    zone_rnum;
typedef long    shop_rnum;
typedef long    trig_rnum;
typedef long    path_rnum;

/*
 * Bitvector type for 32 bit unsigned long bitvectors.
 * 'unsigned long long' will give you at least 64 bits if you have GCC.
 *
 * Since we don't want to break the pfiles, you'll have to search throughout
 * the code for "bitvector_t" and change them yourself if you'd like this
 * extra flexibility.
 */
typedef unsigned long int      bitvector_t;


/* Extra description: used in objects, mobiles, and rooms */
struct extra_descr_data {
   char	*keyword;                 /* Keyword in look/examine          */
   char	*description;             /* What to see                      */
   struct extra_descr_data *next; /* Next in list                     */
};



/* object-related structures ******************************************/


/* object flags; used in obj_data */
#define NUM_OBJ_VAL_POSITIONS 8
struct obj_flag_data {
   long	value[NUM_OBJ_VAL_POSITIONS];	/* Values of the item (see list)    */
   int  total_dam_slots;
   int  curr_dam_slots;
   int  orig_dam_slots;
   byte type_flag;	/* Type of item			    */
   bitvector_t wear_flags;	/* Where you can wear it ITEM_WEAR_ */
   bitvector_t extra_flags;	/* If it hums, glows, etc. ITEM_    */
   bitvector_t extra_flags2;	/* If it hums, glows, etc. ITEM2_   */
   bitvector_t extra_flags3;	/* If it hums, glows, etc. ITEM3_   */
   bitvector_t anti_flags;	/* what classes and races can't use */
   int	weight;		/* Weigt what else                  */
   int	cost;		/* Value when sold (gp.)            */
   int	cost_per_day;	/* Cost to keep pr. real day        */
   int	timer;		/* Timer for object                 */
   int	dg_timer;		/* Timer for object                 */
   bitvector_t bitvector;	/* To set chars bits                */
   bitvector_t lApplyBits;
   int  iApplyMods[TOP_APPLY1_NUM];
   int  shop_order;
};


/* Used in obj_file_elem *DO*NOT*CHANGE* */
struct obj_affected_type {
   byte location;		/* Which ability to change (APPLY_XXX) */
   long modifier;		/* How much it changes by              */
};

/* Used in obj_file_elem *DONT CHANGE* */
struct obj_spell_type {
	ush_int spelltype;	/*number of spell*/
	ush_int level;		/*level of spell*/
	ush_int percentage;	/*percentage of success*/
};

/* ================== Memory Structure for Objects ================== */
struct obj_data {
   obj_vnum item_number;	/* Where in data-base			*/
   room_rnum in_room;		/* In what room -1 when conta/carr	*/
   room_vnum vroom;                   /* for corpse saving */

   struct obj_flag_data obj_flags;/* Object information               */
   struct obj_affected_type affected[MAX_OBJ_AFFECT];  /* affects */
   struct obj_spell_type spell_affect[MAX_SPELL_AFFECT];

   char	*name;                    /* Title of object :get etc.        */
   char	*description;		  /* When in room                     */
   char	*short_description;       /* when worn/carry/in cont.         */
   char	*action_description;      /* What to write when used          */
   struct extra_descr_data *ex_description; /* extra descriptions     */
   struct char_data *carried_by;  /* Carried by :NULL in room/conta   */
   struct char_data *people;	  /* list of people in the object     */
   struct char_data *worn_by;	  /* Worn by?			      */
   sh_int worn_on;		  /* Worn where?		      */

   struct obj_data *in_obj;       /* In what object NULL when none    */
   struct obj_data *contains;     /* Contains objects                 */
   struct obj_data *next_content; /* For 'contains' lists             */
   struct obj_data *next;         /* For the object list              */

   long id;                       /* used by DG triggers              */
   struct trig_proto_list *proto_script; /* list of default triggers  */
   struct script_data *script;    /* script info for the object       */
   
   int  material;		/* Ponder--whats this made of? */
   bool touched;
};
/* ======================================================================= */


/* ====================== File Element for Objects ======================= */
/*                 BEWARE: Changing it will ruin rent files		   */
struct obj_file_elem {
   obj_vnum item_number;
   sh_int locate;  /* that's the (1+)wear-location (when equipped) or
		      (20+)index in obj file (if it's in a container) BK */
   long	value[NUM_OBJ_VAL_POSITIONS];
   int  total_dam_slots;
   int  curr_dam_slots;
   int  orig_dam_slots;
   bitvector_t extra_flags;
   bitvector_t extra_flags2;
   bitvector_t extra_flags3;
   bitvector_t anti_flags;
   int	weight;
   int	timer;
   bitvector_t bitvector;
   struct obj_affected_type affected[MAX_OBJ_AFFECT];
   struct obj_spell_type spell_affect[MAX_SPELL_AFFECT];
};


/* header block for rent files.  BEWARE: Changing it will ruin rent files  */
struct rent_info {
   int	time;
   int	rentcode;
   int	net_cost_per_diem;
   int	gold;
   int	account;
   int	nitems;
   int	spare0;
   int	spare1;
   int	spare2;
   int	spare3;
   int	spare4;
   int	spare5;
   int	spare6;
   int	spare7;
};
/* ======================================================================= */

struct obj_imm_type
{
   int immune_dam;
   int resist_dam;
   int normal_dam;
   int succept_dam;
};


/* room-related structures ************************************************/


/* An affect structure for room */
struct room_affected_type {
   struct queue_event *events;
   sh_int type;			/* The type of spell that caused this      */
   sh_int duration;		/* For how long its effects will last      */
   long   modifier;		/* This is added to apropriate ability     */
   byte   location;		/* Tells which ability to change(APPLY_XXX)*/
   bitvector_t bitvector;	/* Tells which bits to set (AFF_XXX)       */

   struct room_affected_type *next;
};

struct room_direction_data {
   char	*general_description;   /* When look DIR.		        */
   char	*keyword;		/* for open/close			*/
   sh_int exit_info;		/* Exit info				*/
   obj_vnum key;		/* Key's number (-1 for no key)		*/
   room_rnum to_room;		/* Where direction leads (NOWHERE)	*/
   int lcklevl;			/* Wizard lock level                    */
};

struct teleport_data {
   int time;			/* how long till this room teleports */
   room_vnum targ;		/* target room */
   bitvector_t bitvector;		/* TELE_* bits */
   obj_vnum obj;		/* object to look for */
   char *to_char;		/* The message sent to the person moved */
   char *to_source_room;	/* The message sent to the starting room */
   char *to_targ_room;		/* The message sent to the destinaton room */
};

/* ================== Memory Structure for room ======================= */
struct room_data {
   room_vnum number;		/* Rooms number	(vnum)		      */
   zone_rnum zone;                 /* Room zone (for resetting)          */
   int	  sector_type;            /* sector type (move/hide)            */
   char	 *name;                  /* Rooms name 'You are ...'           */
   char	 *description;           /* Shown when entered                 */
   struct extra_descr_data *ex_description; /* for examine/look       */
   struct room_direction_data *dir_option[NUM_OF_DIRS]; /* Directions */
   bitvector_t room_flags;		/* DEATH,DARK ... etc                 */
   bitvector_t room2_flags;		/* Fishing, etc.. 		      */

   struct teleport_data *tele;
   int    light;                  /* Number of lightsources in room     */
   SPECIAL(*func);

   struct obj_data *contents;   /* List of items in room              */
   struct char_data *people;    /* List of NPC / PC in room           */

   struct room_affected_type *affected;	/* room spells an the like    */
   struct trig_proto_list *proto_script; /* list of default triggers  */
   struct script_data *script;  /* script info for the object         */
   int    ore_types[NUM_ORE_SLOTS];
   int    ore_percent[NUM_ORE_SLOTS];
};
/* ====================================================================== */


/* char-related structures ************************************************/


/* memory structure for characters */
struct memory_rec_struct {
   long	id;
   struct memory_rec_struct *next;
};

typedef struct memory_rec_struct memory_rec;

/* MOBProgram foo */
struct mob_prog_act_list {
   struct mob_prog_act_list *next;
   char *buf;
   struct char_data *ch;
   struct obj_data *obj;
   void *vo;
};
typedef struct mob_prog_act_list MPROG_ACT_LIST;

struct mob_prog_data {
   struct mob_prog_data *next;
   int type;
   char *arglist;
   char *comlist;
};

typedef struct mob_prog_data MPROG_DATA;

extern bool MOBTrigger;

#define ERROR_PROG         -1
#define IN_FILE_PROG       (1 << 0)
#define ACT_PROG           (1 << 1)
#define SPEECH_PROG        (1 << 2)
#define RAND_PROG          (1 << 3)
#define FIGHT_PROG         (1 << 4)
#define DEATH_PROG         (1 << 5)
#define HITPRCNT_PROG      (1 << 6)
#define ENTRY_PROG         (1 << 7)
#define GREET_PROG         (1 << 8)
#define ALL_GREET_PROG     (1 << 9)
#define GIVE_PROG          (1 << 10)
#define BRIBE_PROG         (1 << 11)

/* end of MOBProg foo */
 
 
 


/* This structure is purely intended to be an easy way to transfer */
/* and return information about time (real or mudwise).            */
struct time_info_data {
   int minutes, hours, day, month;
   sh_int year;
};


/* These data contain information about a players time data */
struct time_data {
   time_t birth;    /* This represents the characters age                */
   time_t logon;    /* Time of the last logon (used to calculate played) */
   long played;    /* This is the total accumulated time played in secs */
};


/* general player-related info, usually PC's and NPC's */
struct char_player_data {
   char	passwd[MAX_PWD_LENGTH+1]; /* character's password      */
   char	*name;	       /* PC / NPC s name (kill ...  )         */
   char	*short_descr;  /* for NPC 'actions'                    */
   char	*long_descr;   /* for 'look'			       */
   char	*description;  /* Extra descriptions                   */
   char	*title;        /* PC / NPC's title                     */
   byte sex;           /* PC / NPC's sex                       */
   sbyte race;         /* PC / NPC's race (NPC not yet def'd)  */
   byte class;         /* PC / NPC's class		       */
   ush_int level;      /* PC / NPC's level                     */
   room_vnum hometown; /* PC s Hometown (room vnum)            */
   struct time_data time;  /* PC's AGE in days                 */
   int weight;         /* PC / NPC's weight                    */
   int height;         /* PC / NPC's height                    */
};

/* -naj infobar2 12/16/96 - player data structure needed to automagicly update infobar - start */
/*
 * -naj 8/30/95 mod: infobar1 (new structure to store infobar stats)
 *              notes:  this is the new method for updating the infobar,
 *                      resulting in less code and fewer bugs.
 */
struct char_infobar_data {
  long  	hit, end, mana, maxhit, maxend, maxmana, inroom, opphit;
  long		gold, exp, pra, align;
  sbyte		str, dex, intel, wis, con, cha;
  long		level;
  int   	ac;
  int		hitbon, dambon;
  int		hunger, thirst, drunk;
  long		race, class;
  int		bank, weight;
  int		exits;  /* will require additional coding */
  char          fighting;
};
/* -naj infobar2 12/16/96 - player data structure needed to automagicly update infobar - end */


/*=====The following functions are part of the pfile. Any changes you make
  =====will affect the pfile in some way.================================*/

/* Char's abilities.  Used in char_file_u *DO*NOT*CHANGE* */
struct char_ability_data {
   sbyte str;			/* Strength */
   sbyte str_add;		/* 000 - 100 if strength 18             */
   sbyte intel;			/* inteligence */
   sbyte wis;			/* wisdom */
   sbyte dex;			/* dexterity */
   sbyte con;			/* constitution */
   sbyte cha;			/* charisma */
};


/* Char's points.  Used in char_file_u *DO*NOT*CHANGE* */
struct char_point_data {
   int mana;
   int max_mana;		/* Max mana for PC/NPC			   */
   int hit;
   int max_hit;			/* Max hit for PC/NPC                      */
   int move;
   int max_move;		/* Max move for PC/NPC                     */

   int armor;			/* Internal -100..100, external -10..10 AC */
   long	gold[5];		/* Money carried                           */
   long	bank_gold[32];		/* Gold the char has in a bank account	   */
   long	exp;			/* The experience of the player            */

   int hitroll;			/* Any bonus or penalty to the hit roll    */
   int damroll;			/* Any bonus or penalty to the damage roll */
};


/* WARNING: Do not change this structure unless you know what you're doing.
 * 
 * char_special_data_saved: specials which both a PC and an NPC have in
 * common, but PC's will be saved to the pfile. Additional PC specials
 * can be added in player_special_data.
 */

struct char_special_data_saved {
   bitvector_t affected_by;      /* Bitvector for spells/skills affected by */
   bitvector_t affected_by2;     /* Bitvector for spells/skills affected by */
   bitvector_t affected_by3;     /* Bitvector for spells/skills affected by */
   sh_int apply_saving_throw[5]; /* Saving throw (Bonuses)		*/
   long idnum;                  /* player's idnum                       */
   int  alignment;              /* +-1000 for alignments                */
   bitvector_t act;                    /* act flag for NPC's; player flag for PC's */
   bitvector_t act2;                   /* act flag for NPC's; player flag for PC's */
   bitvector_t act3;                   /* act flag for NPC's; player flag for PC's */
      
   /* spares below for future expansion.  You can change the names from
    * 'sparen' to something meaningful, but don't change the order.
    * Mob specials go to mob_special.data. PC specials go to player_special_data
    */
   int spell_fail;		/* spell failure adjustment (bonus/penalty */
   int spare1;
   int spare2;
   int spare3;
   int spare4;
   int spare5;
   int spare6;
   int spare7;
   int spare8;
   long spare9;
   long spare10;
   long spare11;
   long spare12;
   long spare13;
   long spare14;
   long spare15;
   bool spare16;
   bool spare17;
   bool spare18;
};

/*
 *  If you want to add new values to the playerfile, do it here.  DO NOT
 * ADD, DELETE OR MOVE ANY OF THE VARIABLES - doing so will change the
 * size of the structure and ruin the playerfile.  However, you can change
 * the names of the spares to something more meaningful, and then use them
 * in your new code.  They will automatically be transferred from the
 * playerfile into memory when players log in.
 */

struct player_special_data_saved {
   byte  skills[MAX_SKILLS+1];	/* array of skills plus skill 0		*/
   byte  skills_learn[MAX_SKILLS+1]; /* how much you have learned */
   int   last_learnt;	        /* The skill you learned last*/
   bool  talks[MAX_TONGUE];	/* PC s Tongues 0 for NPC		*/
   int	 wimp_level;		/* Below this # of hit points, flee!	*/
   byte  freeze_level;		/* Level of god who froze char, if any	*/
   sh_int invis_level;		/* level of invisibility		*/
   room_vnum load_room;		/* Which room to place char in		*/
   ubyte bad_pws;		/* number of bad password attemps	*/
   sbyte conditions[3];         /* Drunk, full, thirsty			*/
   char  poofin[MAX_POOF];	/* Save poofin				*/
   char  poofout[MAX_POOF];	/* Save poofout				*/
   bitvector_t pref;			/* preference flags for PC's.		*/
   bitvector_t pref2;
   bitvector_t pref3;
   char  cl_name[MAX_CLAN_NAME_LENGTH]; /* name of your clan */
   int   cl_rank;		/* 1 if leader, 0 if not                   */
   int   clan;			/* clan number 0 if not in clan            */
   int   cl_room;		/* The clan's room                         */

   int  point;			/* Used in char stat choices at creation */
   int  old_mobkills;		/* Number of mobs a player has killed **OLD** */
   int  pkills;			/* Number of players a player has killed */
   int  deaths;			/* Number of times a player has died */
   int  q_points;		/* Number of quest points a player has */
   int  board_number;		/* Number of board being looked at */
   int  olc_zone[MAX_OLC_ZONES]; /* which zones you can edit */
   long  kills_vnum[64];	/* list of the vnums of the mobs killed */
   ubyte kills_ammount[64];	/* how many times you killed each mob */
   /* spares below for future expansion.  You can change the names from
      'sparen' to something meaningful, but don't change the order.  */

   ubyte learn_tic;
   ubyte spare0;
   ubyte spare1;
   ubyte spare2;
   ubyte spare3;
   ubyte spare4;
   int screensize;
   int times_remorted;
   int mute_channels;          /* bitvector for listening selectively */
   int cl_donate;              /* clan donate room - nomi 6/2/2025 */
   int spare10;
   int spare11;
   int spare12;
   int spare13;
   int spare14;
   int spare15;
   int spare16;
   long	mobkills;
   long	spare18;
   long	spare19;
   long	spare20;
   long	spare21;
};

/* An affect structure.  Used in char_file_u *DO*NOT*CHANGE* */
struct affected_type {
   sh_int type;			/* The type of spell that caused this      */
   sh_int duration;		/* For how long its effects will last      */
   long   modifier;		/* This is added to apropriate ability     */
   byte   location;		/* Tells which ability to change(APPLY_XXX)*/
   bitvector_t  bitvector;		/* Tells which bits to set (AFF_XXX)       */
   sh_int spell_level;    /* The level of the spell that caused this affect. 0 if N/A. */

   struct affected_type *next;
};

/*====End of pfile sensitive code=======================================*/


/* Special playing constants used by PCs ONLY. MOB playing constants are
 * in mob_special_data. These constants are loaded during the game but 
 * not saved in the pfile. Additional specials can be added without harm
 * to the pfile.
 */

struct char_special_data {
   struct char_special_data_saved saved; /* constants saved in plrfile	*/
   struct char_data *fighting;	/* Opponent				*/
   struct char_data *hunting;	/* Char hunted by this char		*/
   struct char_data *riding;    /* Who are they riding? (DAK)		*/
   struct char_data *ridden_by; /* Who is riding them? (DAK)            */   
   struct obj_data *furniture;	/* What they are sitting/sleeping on	*/
   int  last_hand_used;		/* which hand you last attacked with    */
   int  speed;			/* how much longer till next hit        */
   byte position;		/* Standing, fighting, sleeping, etc.	*/
   int	carry_weight;		/* Carried weight			*/
   int  carry_items;		/* Number of items carried		*/
   int  carry_bulk;             /* Total bulk of items carried		*/
/*   int  wait;	*/		/* Delay before can attack 		*/
   int	timer;			/* Timer for update			*/
   bool in_battle;		/* Whether in battle or not		*/
   room_rnum was_in_room;	/* storage of location for linkdead people */
   int invis_level;             /* level of invisibility                */
   int light;                   /* number of lights carried by the char */
   long damage_amount[17];	/* numbers of times a person has damaged */
   long dodged;                 /* Number of times dodged               */
   struct char_data *tch;	/* Casting Delay Info - char target*/
   struct obj_data *tobj;	/* Casting Delay Info - object target*/
   struct room_direction_data *tdr;/* Casting Delay Info - door1 target*/
   struct room_direction_data *tdr2;/* Casting Delay Info - door2 target*/
   int cast_time;		/* Casting Delay Info - time left*/
   int spellnum;		/* Casting Delay Info - what casting*/
   bool casting;		/* Casting Delay Info - casting?*/
   int cast_level;		/* Casting Delay Info - lvl of spell*/
   char cast_arg[128];		/* Casting Delay Info - text arg*/
   bitvector_t immune;
   bitvector_t resist;
   bitvector_t succept;
  };

/* Specials used by PCs using the player_special_data_saved structure
 * Feel free to add in new variables but the portion labeled 'saved'
 * is part of the pfile. WARNING: Changing the contents of player_special_data_saved
 * will corrupt the pfile.
 */


#define HERO_TEST_SKILLS_FILE "etc/hero_skills"

struct player_special_data {
   struct player_special_data_saved saved;
   struct alias_data *aliases;
   long last_tell;
   void *last_olc_targ;
   int last_olc_mode;
   long ignored[5];             /* idums of ignored players */
   bool tagged;                 /* tagged or not? (battle arena) */

  int explored_total;
  char explored_vnums[EXPLORED_BYTES];
  char email[256];

  bool is_being_reimbd;
  int reimb_obj_vnums[NUM_WEARS];
  int reimb_skills[250];
  int reimb_num_skills;
  int reimb_num_spells;
};

/* Specials used by NPCs, not PCs */
struct mob_special_data {
   byte last_direction;     /* The last direction the monster went     */
   int	attack_type;        /* The Attack Type Bitvector for NPC's     */
   byte default_pos;        /* Default position for NPC                */
   memory_rec *memory;	    /* List of attackers to remember	       */
   byte damnodice;          /* The number of damage dice's	       */
   byte damsizedice;        /* The size of the damage dice's           */
/*   int  wait_state;*/	    /* Wait state for bashed mobs	       */
   int  mood;
   long  special_value[10];
   int maxfactor;
  char is_mob_inflated;

   obj_vnum skin;
};


/* Structure used for chars following other chars */
struct follow_type {
   struct char_data *follower;
   struct follow_type *next;
};


/* ================== Structure for player/non-player ===================== */
struct char_data {
   int pfilepos;			/* playerfile pos		   */
   mob_rnum nr;				/* Mob's rnum			   */
   room_rnum in_room;			/* Location (real room number)	   */
   room_rnum was_in_room;		/* location for linkdead people    */
  /*  */
   struct char_player_data player;	/* Normal data                     */
   struct char_infobar_data infobar;	/* -naj infobar2 12/16/96 - structure needed to update infobar */
   struct char_ability_data real_abils;	/* Abilities without modifiers     */
   struct char_ability_data aff_abils;	/* Abils with spells/stones/etc    */
   struct char_point_data points;	/* Points                          */
   struct char_special_data char_specials; /* PC/NPC specials	           */
   struct player_special_data *player_specials;	/* PC specials		   */
   struct mob_special_data mob_specials; /* NPC specials		   */
  /*  */
   struct affected_type *affected;	/* affected by what spells         */
   struct obj_data *equipment[NUM_WEARS]; /* Equipment array               */
  /*  */
   struct obj_data *carrying;		/* Head of list                    */
   struct descriptor_data *desc;	/* NULL for mobiles                */

   long id;                            /* used by DG triggers             */
   struct trig_proto_list *proto_script; /* list of default triggers      */
   struct script_data *script;          /* script info for the object      */
   struct script_memory *memory;        /* for mob memory triggers         */

   struct char_data *next_in_room;	/* For room->people - list         */
   struct char_data *next_in_furniture;	/* For obj->people - list          */
   struct char_data *next;		/* For either monster or ppl-list  */
   struct char_data *next_fighting;	/* For fighting list               */

   struct follow_type *followers;	/* List of chars followers         */
   struct char_data *master;		/* Who is char following?          */
   int wait;                            /* command/skill wait              */
   int stun_lag;                        /* stun lag from violent action    */
   MPROG_ACT_LIST *mpact;
   int mpactnum;
   room_rnum orig_room;

  int num_casters;
  long *casting_on_me;

  struct char_data *guarding;    /* Who YOU are guarding.  You can only guard one player at a time. */
  struct char_data **guarding_me; /* A list of people who are guarding YOU.  This can be many people. */
  int num_guarding_me;
  
};
/* ====================================================================== */

/* ==================== File Structure for Player ======================= */
/*             BEWARE: Changing it will ruin the playerfile		  */
struct char_file_u {
   /* char_player_data */
   char	name[MAX_NAME_LENGTH+1];
   char	description[EXDSCR_LENGTH];
   char	title[MAX_TITLE_LENGTH+1];
   byte sex;
   sbyte race;				/* 10/27/96 Echo */
   byte class;
   ush_int level;			/*  changed to ush_int from ubyte */
   room_vnum hometown;
   time_t birth;			/* Time of birth of character     */
   long played;				/* Number of secs played in total */
   ubyte weight;
   ubyte height;

   char	pwd[MAX_PWD_LENGTH+1];		/* character's password */

   struct char_special_data_saved char_specials_saved;
   struct player_special_data_saved player_specials_saved;
   struct char_ability_data abilities;
   struct char_point_data points;
   struct affected_type affected[MAX_AFFECT];  

   time_t last_logon;			/* Time (in secs) of last logon */
   char host[HOST_LENGTH+1];		/* host of last logon */

  /* This gets written to name.explored */
  char explored_vnums[EXPLORED_BYTES];
  char email[256];
  /* End name.explored */
};
/* ====================================================================== */



/* descriptor-related structures ******************************************/


struct txt_block {
   char	*text;
   int aliased;
   struct txt_block *next;
};


struct txt_q {
   struct txt_block *head;
   struct txt_block *tail;
};


struct descriptor_data {
   socket_t descriptor;		/* file descriptor for socket */
   socket_t ident_sock;		/* socket used for ident process        */
   unsigned short  peer_port;	/* port of peer               		*/
   char	host[HOST_LENGTH+1];	/* hostname				*/
/*   byte close_me; removed*/ /* flag: this desc. should be closed    */
   byte	bad_pws;		/* number of bad pw attemps this login	*/
   byte idle_tics;		/* tics idle at password prompt		*/
   int	connected;		/* mode of 'connectedness'		*/
   int oob_protocol;		/* Out of band protocol settings */
/*   int	wait;*/			/* wait for how many loops		*/
   int	desc_num;		/* unique num assigned to desc		*/
   time_t login_time;		/* when the person connected		*/
   char *showstr_head;		/* for keeping track of an internal str	*/
   char **showstr_vector;	/* for paging through texts		*/
   int  showstr_count;		/* number of pages to page through	*/
   int  showstr_page;		/* which page are we currently showing?	*/
   char	**str;			/* for the modify-str system		*/
   char *backstr;		/* added for handling abort buffers	*/
   long max_str;		/*		-			*/
   long	mail_to;		/* name for mail system			*/
   int	has_prompt;		/* control of prompt-printing		*/
   char	inbuf[MAX_RAW_INPUT_LENGTH];  /* buffer for raw input		*/
   char	last_input[MAX_INPUT_LENGTH]; /* the last input			*/
   char small_outbuf[SMALL_BUFSIZE];  /* standard output buffer		*/
   char *output;		/* ptr to the current output buffer	*/
   char **history;		/* History of commands, for !mostly     */
   int  history_pos;		/* Circular array position              */
   int  bufptr;			/* ptr to end of current output		*/
   int	bufspace;		/* space left in the output buffer	*/
/*   struct txt_block *large_outbuf;*/ /* ptr to large buffer, if we need it */
   char *large_outbuf;
   struct txt_q input;		/* q of unprocessed input		*/
   struct char_data *character;	/* linked to char			*/
   struct char_data *original;	/* original char if switched		*/
   struct descriptor_data *snooping; /* Who is this char snooping	*/
   struct descriptor_data *snoop_by; /* And who is snooping this char	*/
   struct descriptor_data *next; /* link to next descriptor		*/
   struct olc_data *olc;	     /*. OLC info - defined in olc.h   .*/
   char *storage;
   char sHeader[MAX_STRING_LENGTH]; /* Optional header for paging */
   int speed_buffer;
   int speed_wait;
   int first_time;
};


/* other miscellaneous structures ***************************************/

/* Messages */
struct msg_type {
   char	*attacker_msg;	  /* message to attacker */
   char	*victim_msg;	  /* message to victim   */
   char	*room_msg;	  /* message to room     */
};


struct message_type {
   struct msg_type die_msg;	/* messages when death			*/
   struct msg_type miss_msg;	/* messages when miss			*/
   struct msg_type hit_msg;	/* messages when hit			*/
   struct msg_type god_msg;	/* messages when hit on god		*/
   struct message_type *next;	/* to next messages of this kind.	*/
};


struct message_list {
   int	a_type;		  /* Attack type				*/
   int	number_of_attacks; /* How many attack messages to chose from. */
   struct message_type *msg; /* List of messages.			*/
};


/* Stats */
struct dex_skill_type {
   sh_int p_pocket;
   sh_int p_locks;
   sh_int traps;
   sh_int sneak;
   sh_int hide;
};


struct dex_app_type {
   sh_int reaction;
   sh_int miss_att;
   sh_int defensive;
   byte   bonus;
};

struct cha_app_type
{
        sh_int  adj_num_mercs;  /* number of mercenaries adjust */
        sh_int  adj_num_charm;  /* number of charmimes adjust */
        sh_int  adj_price;      /* adjust prices % */
        sh_int  adj_reactions;  /* reaction adjustment % */
};

struct str_app_type {
   sh_int tohit;	  /* To Hit (THAC0) Bonus/Penalty        */
   sh_int todam;	  /* Damage Bonus/Penalty                */
   sh_int carry_w;	  /* Maximum weight that can be carrried */
   sh_int wield_w;	  /* Maximum weight that can be wielded  */
};


struct wis_app_type {
   byte bonus;		  /* increase to % chance to learn */
};


struct int_app_type {
   byte bonus;		  /* increase to % chance to learn */
};


struct con_app_type {
   sh_int hitp;
   sh_int shock;
};

struct point_gain_type {
   sh_int hitp;
   sh_int manap;
   sh_int mvp;
};

/* Mobs */
struct mob_defaults {
   int level;
   int num_hit;
   int sze_hit;
   int bon_hit;
   int to_hit;
   int ac;
   int num_dam;
   int sze_dam;
   int to_dam;
   int gold;
};

/* Races */
struct racial_data {
   int base_age;
   int max_age;
   sh_int base_moves;
   sh_int base_mana;
   int max_mana;
   int hp_bonus;
   int mana_bonus;
};

struct race_size_data {
   long MHmin;
   long MHave;
   long MHmax;
   long MWmin;
   long MWave;
   long MWmax;
   long FHmin;
   long FHave;
   long FHmax;
   long FWmin;
   long FWave;
   long FWmax;
};

struct race_stat_data {
   sh_int Str;
   sh_int Dex;
   sh_int Con;
   sh_int Int;
   sh_int Wis;
   sh_int Cha;
};

struct trait_data {
   bool humanoid;        /* upright biped */
   bool length;          /* length is largest dimension (not height) */
   bool level_size;      /* level directly affects size */
   bool can_talk;        /* can speak intelligently */
   bool can_fly;         /* has wings or innate ability to fly */
   bool can_swim;        /* can float and move through water unassisted */
   bool has_hands;       /* has dextrous manipulators */
   bool emits_heat;      /* is visible with infravision */
   bool immaterial;      /* has zero mass */
   bool stun_proof; /* for various reason, can't knockout or stun this race. */
   bool infravision;     /* can 'see' heat sources */
   bool ultravision;     /* true darksight */
   byte hitroll;         /* the racial hitroll modifier */
   bool auto_sneak;	 /* automatically sneaks around */
   bool can_tame;
   int  morale;		 /* 0 to 10 chance in 15 of fleeing */
   bitvector_t susceptible;     /* what can really hit this race */
   bitvector_t resist;		 /* what damage race resists */
   bitvector_t immune;		 /* what damage race is immune to */
};


/* Weather */
struct weather_data {
   int	pressure;	  /* How is the pressure ( Mb ) */
   int	change;		  /* How fast and what way does it change. */
   int	sky;		  /* How is the sky. */
   int	sunlight;	  /* And how much sun. */
   int  moonlight;	  /* how much moon light */
   int  moon_phase;	  /* what phase the moon is in */
};



/* element in monster and object index-tables   */
struct index_data {
   long	vnum;   	  /* virtual number of this mob/obj           */
   int	number;		  /* number of existing units of this mob/obj */
   int  progtypes;	  /* program types for MOBProg              */
   MPROG_DATA *mobprogs;  /* programs for MOBProg              */
   SPECIAL(*func);

   char *farg;         /* string argument for special function     */
   struct trig_data *proto;     /* for triggers... the trigger     */
};

/* linked list for mob/object prototype trigger lists */
struct trig_proto_list {
   long vnum;                             /* vnum of the trigger   */
   struct trig_proto_list *next;         /* next trigger          */
};


/* Battle */
struct battle_zone
{
   bool zone_state;	  /* wether the zone is open or not */
   int low_level;	  /* The lowest level that can enter the battle  */
   int high_level;	  /* The highest level that can enter the battle */
   bool locked;	          /* wether its locked or not */
   bool tagged;           /* whether a player in battle is tagged or not */
   bool do_tag;           /* whether tag game is enabled */
};


/* Auction */
struct autoauction
{
   bool in_progress;     /* Wether an auction is taking place or not */
   bool bid_on;	         /* Wether the item has been bid upon */
   long previous_bid;    /* Previous bidder's bid */
   long last_bid;        /* The last bid on the item */
   long previous_bidder_id_num; /* previous bidder */
   long bidder_id_num;   /* The person that last bids idnumber */
   long seller_id_num;   /* The person that is selling's idnum */
   long selling_price;   /* Price asking for the item */
   int state_of_sale;    /* Going 1, 2, 3, sold */
   long item_auc;        /* Item being auctioned */
   struct obj_data *obj; /* storing iobject */
};


/* Ore Matching */
struct obj_ore_types
{
   char      *ore_string;
   obj_vnum   ore_vnum;
};

/* Class */
struct npc_class_mana
{
   int       npc_mana;
};

struct scr_skills_struct {
  int class;
  int spell_num;
  int level;
};

struct srr_skills_struct {
  int race;
  int spell_num;
  int level;
};



struct shop_item
{
  struct obj_data* item;
  int amount;
  struct shop_item* next;
};

struct player_shop {
  char player_name[64];
  room_vnum vnum_location;
  int rent;
  int is_active;
  struct shop_item* contents;
  struct player_shop* next;
};

extern struct player_shop* player_shops;

struct default_ability_data
{
  int race;
  int clazz;
  struct char_ability_data abilities;
};

extern struct default_ability_data default_player_stats[ 100 ];
extern int num_default_player_stats;
