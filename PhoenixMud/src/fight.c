/* ************************************************************************
*   File: fight.c                                       Part of CircleMUD * 
*  Usage: Combat system                                                   * 
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
#include "handler.h"
#include "interpreter.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "dg_scripts.h"
#include "constants.h"

/* Structures */
struct char_data *combat_list = NULL; /* head of l-list of fighting chars */
struct char_data *next_combat_list = NULL;
extern struct zone_data *zone_table;              /*. db.c .*/
extern int top_of_zone_table;                     /*. db.c .*/
extern struct battle_zone battle;

/* External structures */
extern int port;
extern int max_npc_corpse_time, max_pc_corpse_time;
extern int max_damage;
extern struct room_data *world;
extern struct message_list fight_messages[MAX_MESSAGES];
extern struct obj_data *object_list;
extern int pk_allowed;  /* see config.c */
extern int auto_save;  /* see config.c */
extern int min_kills;
extern const int exp_table[];
extern const float class_exp_multipliers[];
extern const float race_exp_multipliers[];
extern struct index_data *obj_index;
extern struct room_data *world;
extern struct index_data *mob_index;
extern int max_exp_loss;
extern struct spell_info_type *spells;
extern room_vnum mortal_start_room;
extern struct descriptor_data *descriptor_list;
extern char *pc_race_types[];
/* External procedures */
int backstab_mult(int level);
struct obj_data *create_money(int amount);
char *fread_action(FILE * fl, int nr);
char *fread_string(FILE * fl, char *error);
int ok_damage_shopkeeper(struct char_data * ch, struct char_data * victim);
int base_thaco(int ch_class, int ch_level); /* 10/09/96, Echo */
void mprog_hitprcnt_trigger(struct char_data * mob, struct char_data * ch);
void mprog_death_trigger(struct char_data * mob, struct char_data * killer);
void mprog_fight_trigger(struct char_data * mob, struct char_data * ch);
void rage_check(struct char_data *ch);
int compute_armor_class(struct char_data *ch);
int compute_thaco(struct char_data *ch);
void dismount_char(struct char_data *ch);
void save_corpses(void);
int skill_roll(struct char_data *ch, int skill_num, int penalty);
struct obj_data *has_object_ref(struct char_data *ch, int iobj_vnum);


float fight_group_exp_divisor = 2.5f;
int fight_ldiff_dodge_multiplier = 4;
int fight_grouped_dodge_multiplier = 1;
int fight_dodge_random_bound = 190;

ACMD(do_get); /* autosplit/loot code from snippets page --Erika */
ACMD(do_split); /* autosplit/loot (hereafter asl) --Erika */
ACMD(do_assist);
ACMD(do_flee);
ACMD(do_look);
ACMD(do_sac);
void clearMemory(struct char_data * ch);

struct dam_weapon_type
   {
   char *to_room;
   char *to_char;
   char *to_victim;
   } ;



/**************/
/*** WEAPON ***/
/**************/


struct dam_weapon_type bite_weapons[10] =
      {
         { "$n clicks $s teeth together in an attempt to bite $N", /* 0: 0 */
         "You click your teeth together in an attempt to bite $N",
         "$n clicks $s teeth together in an attempt to bite you" } ,

      { "$n tries to latch on to $N with his teeth", /* 1: 1..5 */
        "You try to latch on to $M with your teeth",
        "$n tries to latch on to you with $s teeth" } ,

      { "$n nibbles on $N", /* 2: 6..10 */
        "You nibble on $N",
        "$n nibbles on you" } ,

      { "$n pinches $N with $s teeth", /* 3: 11..15 */
        "You pinch $N with your teeth",
        "$n pinches you with $s teeth" } ,

      { "$n clamps $s teeth on $N", /* 4: 16..20 */
        "You clamp your teeth on $N",
        "$n clamps his teeth on you" } ,

      { "$n sinks $s teeth into $N ", /* 5: 20..25 */
        "You sink your teeth into $N",
        "$n sinks $s teeth into you" } ,

      { "$n crunches into $N with $s bite ", /* 6: 25..30 */
        "You crunch into $N with your bite",
        "$n bites you extremely hard" } ,

      { "$n tears $N to bits with $s bite", /* 7: 31..40 */
        "You tear $N to bits with your bite",
        "$n tears you to bits with $s bite" } ,

      { "$n shreds $N into ribbons with $s dangerous bite", /* 8: 41..50 */
        "You shred $N into ribbons with your dangerous bite",
        "$n shreds you into ribbons with $s dangerous bite" },

      { "$n tears $N up with $s fatal bite", /* 9: > 50 */
        "You tear $N up with your fatal bite",
        "$n tears you up with $s fatal bite" }
      };

struct dam_weapon_type bludgeon_weapons[10] =
      {
         { "$n tries to bludgeon $N, but misses", /* 0: 0     */
         "You try to bludgeon $N, but miss",
         "$n tries to bludgeon you, but misses" } ,

      { "$n grazes $N with $s weapon",
        "You graze $N with your weapon",
        "$n grazes you with $s weapon"},

      { "$n thumps $N",
        "You thump $N",
        "$n thumps you"},

      { "$n solidly strikes $N",
        "You solidly strike $N",
        "$n solidly strikes you"},

      { "$n strikes $N hard",
        "You strike $N hard",
        "$n strikes you hard"},

      { "$n swings $s weapon with bone-cracking force at $N",
        "You swing your weapon with bone-cracking force at $N",
        "$n swings $s weapon with bone-cracking force at you"},

      { "$n whacks $N hard with concussive force",
        "You whack $N hard with concussive force",
        "$n whacks you hard with concussive force"},

      { "$n deals a crushing blow to $N",
        "You deal a crushing blow to $N",
        "$n deals a crushing blow to you"},

      { "$n strikes a shattering blow to $N",
        "You strike a shattering blow to $N",
        "$n strikes a shattering blow to you"},

      { "$n strikes a critical blow to $N",
        "You strike a critical blow to $N",
        "$n strikes a critical blow to you"}
      };

struct dam_weapon_type dam_weapons[10] =
      {
      /* use #w for singular (i.e. "slash") and #W for plural (i.e. "slashes")*/
         { "$n tries to #w $N, but misses", /* 0: 0     */
         "You try to #w $N, but miss",
         "$n tries to #w you, but misses" } ,

      { "$n tickles $N as $e #W $M", /* 1: 1..5  */
        "You tickle $N as you #w $M",
        "$n tickles you as $e #W you" } ,

      { "$n barely #W $N",  /* 2: 6..10  */
        "You barely #w $N",
        "$n barely #W you" } ,

      { "$n #W $N",   /* 3: 11..15  */
        "You #w $N",
        "$n #W you" } ,

      { "$n #W $N hard",   /* 4: 16..20  */
        "You #w $N hard",
        "$n #W you hard" } ,

      { "$n #W $N very hard",  /* 5: 21..25  */
        "You #w $N very hard",
        "$n #W you very hard" } ,

      { "$n #W $N extremely hard", /* 6: 26..30  */
        "You #w $N extremely hard",
        "$n #W you extremely hard" } ,

      { "$n massacres $N to small fragments with $s #w", /* 7: 31..40 */
        "You massacre $N to small fragments with your #w",
        "$n massacres you to small fragments with $s #w" } ,

      { "$n OBLITERATES $N with $s deadly #w", /* 8: 41..50   */
        "You OBLITERATE $N with your deadly #w",
        "$n OBLITERATES you with $s deadly #w" },

      { "$n DEVASTATES $N with $s deadly #w", /* 9: > 50   */
        "You DEVASTATE $N with your deadly #w",
        "$n DEVASTATES you with $s deadly #w" }
      } ;

struct dam_weapon_type hit_weapons[10] =
      {
         { "$n flails about trying to hit $N", /* 0: 0 */
         "You flail about trying to hit $N",
         "$n flails about trying to hit you" } ,

      { "$n tries to land a weak blow to $N with $s hit", /* 1: 1..5 */
        "You try to land a blow to $N with your hit",
        "$n tries to land a weak blow with $s hit" } ,

      { "$n lands a lucky hit to $N", /* 2: 6..10 */
        "You land a lucky hit to $N",
        "$n lands a lucky hit to you" } ,

      { "$n hits $N squarely", /* 3: 11..15 */
        "You hit $N squarely",
        "$n squarely hits you" } ,

      { "$n strikes $N hard", /* 4: 16..20 */
        "You strike $N hard",
        "$n strikes you hard" } ,

      { "$n whaps $N painfully", /* 5: 21..25 */
        "You whap $N painfully",
        "$n whaps you painfully" } ,

      { "$n hits $N forcefully", /* 6: 26..30 */
        "You hit $N forcefully",
        "$n hits you forcefully" } ,

      { "$n deals a harsh blow to $N with $s hit", /* 7: 31..40 */
        "You deal a harsh blow to $N with your hit",
        "$n deals a harsh blow to you with $s hit" } ,

      { "$n lands a withering blow to $N with $s deadly hit", /* 8: 41..50 */
        "You land a withering blow to $N with your deadly hit",
        "$n lands a withering blow to you with $s deadly hit" },

      { "$n crushes $N with $s lethal hit", /* 9: > 50 */
        "You crush $N with your lethal hit",
        "$n crushes you with $s lethal hit" }
      } ;

struct dam_weapon_type pierce_weapons[10] =
      {
         { "$n tries to pierce $N, but nearly misses", /* 0: 0 */
         "You try to pierce $N, but nearly miss",
         "$n tries to pierce you, but nearly misses" } ,

      { "$n clumsily nicks $N with $s weapon",
        "You clumsily nick $N with your weapon",
        "$n clumsily nicks you with $s weapon"},

      { "$n lands a glancing blow to $N",
        "You land a glancing blow to $N",
        "$n lands a glancing blow to you"},

      { "$n pierces $N",
        "You pierce $N",
        "$n pierces you"},

      { "$n stabs at $N fiercely",
        "You stab at $N fiercely",
        "$n stabs at you fiercely"},

      { "$n expertly pierces $N with keen skill",
        "You expertly pierce $N with keen skill",
        "$n expertly pierces you with keen skill"},

      { "$n deftly pierces $N",
        "You deftly pierce $N",
        "$n deftly pierces you"},

      { "$n cuts $N severely with $s weapon",
        "You cut $N severely with your weapon",
        "$n cuts you severely with $s weapon"},

      { "$n pierces $N with deadly force",
        "You pierce $N with deadly force",
        "$n pierces you with deadly force"},

      { "$n drives $s weapon firmly into the chest of $N",
        "You drive your weapon firmly into the chest of $N",
        "$n drives $s weapon firmly into your chest"}
      };

struct dam_weapon_type punch_weapons[10] =
      {
         { "$n spins and loses $s balance from $s missed swing ", /* 0: 0 */
         " You spin and lose your balance from your missed swing ",
         "$n spins and loses $s balance from $s missed swing " },

      { "$n tries to punch $N weakly", /* 1: 1..5 */
        "You try to punch $N weakly",
        "$n tries to punch you weakly"},

      {"$n's punch bounces off $N", /* 2: 6..10 */
       "Your punch bounces off $N",
       "$n's punch bounces off you"},

      { "$n manages to punch $N hard", /*3: 11..15 */
        "You manage to punch $N hard",
        "$n manages to punch you hard"},

      { "$n punches $N with a strong uppercut " , /* 4: 16..20 */
        " You punch $N with a strong uppercut ",
        "$n punches you with a strong uppercut " },

      { "$n sidesteps and punches $N hard", /* 5: 21..25 */
        "You sidestep and punch $N hard",
        "$n sidesteps and punches you hard" },

      { "$N misses a step from $n's painful punch", /* 6: 26..30 */
        "You make $N miss a step with your painful punch",
        "$n makes you miss a step with $s painful punch" },

      { "$n makes $N stumble back from $s stinging punch", /* 7: 31..40 */
        "You make $N stumble back with your stinging punch",
        "$n makes you stumble back from $s stinging punch" },

      {"$n knocks the breath out of $N with $s crippling punch", /* 8: 41..50 */
       "You knock the breath out of $N with your crippling punch",
       "$n almost knocks the breath out of you with $s crippling punch" },

      {"$n almost knocks $N out with $s punch", /* 9: > 50 */
       "You almost knock $N out with your punch",
       "$n almost knocks you out with $s punch" }
      };

struct dam_weapon_type slash_weapons[10] =
      {
         { "$n clumsily attempts to slash at $N", /* 0: 0 */
         "You clumsily attempt to slash at $N",
         "$n clumsily attempts to slash at you" } ,

      { "$n gets lucky and slashes $N", /* 1: 1..5 */
        "You get lucky and slash $N",
        "$n gets lucky and slashes you" } ,

      { "$n lightly grazes $N with $s slash", /* 2: 6..10 */
        "You lightly graze $N with your slash",
        "$n lightly grazes you with $s slash" } ,

      { "$n slices $N with $s slash", /* 3: 11..15 */
        "You slice $N with your slash",
        "$n slices you with $s slash" } ,

      { "$n slashes $N with careful aim", /* 4: 16..20 */
        "You slash $N with careful aim",
        "$n slashes you with careful aim" } ,

      { "$n cruelly slashes $N", /* 5: 21..25 */
        "You cruelly slash $N",
        "$n cruelly slashes you" } ,

      { "$n slashes $N forcefully", /* 6: 26..30 */
        "You slash $N forcefully",
        "$n slashes you forcefully" } ,

      { "$n carves $N up with $s keen slash", /* 7: 31..40 */
        "You carve $N up with your keen slash",
        "$n carves you up with $s keen slash" },

      { "$n sheers through $N with $s expert slash", /* 8: 41..50 */
        "You sheer through $N with your expert slash",
        "$n sheers through you with $s expert slash" },

      {"$n slaughters $N with $s lethal slash", /* 9: > 50 */
       "You slaughter $N with your lethal slash",
       "$n slaughters you with $s lethal slash" }
      } ;

struct dam_weapon_type thrash_weapons[10] =
      {
         { "$n thrashes about $N, looking silly", /* 0: 0 */
         "You thrash about $N, looking silly",
         "$n thrashes about you, looking silly" } ,

      { "$n vainly tries to thrash $N", /* 1: 1..5 */
        "You vainly try to thrash $N",
        "$n vainly tries to thrash you" } ,

      { "$n hopelessly batters $N", /* 2: 6..10 */
        "You hopelessly batter $N",
        "$n hopelessly batters you" } ,

      { "$n successfully thrashes $N", /* 3: 11..15 */
        "You successfully thrash $N",
        "$n successfully thrashes you" } ,

      { "$n lashes $N with determination", /* 4: 16..20 */
        "You lashes $N with determination",
        "$n lashes you with determination" } ,

      { "$n flays $N with $s thrash", /* 5: 21..25 */
        "You flays $N with your thrash",
        "$n flays you with $S thrash" } ,

      { "$n batters $N with great effort", /* 6: 26..30 */
        "You batter $N with great effort",
        "$n batters you with great effort" } ,

      { "$n rains blows on $N", /* 7: 31..40 */
        "You rain blows on $N",
        "$n rains blows on you" },

      { "$n pummels $N into the ground with $s thrash", /* 8: 41..50 */
        "You pummels $N into the ground with your thrash",
        "$n pummels you into the ground with $s thrash" } ,

      { "$n thrashes $N to a bloody pulp", /* 9: > 50 */
        "You thrashes $N to a bloody pulp",
        "$n thrashes you to a bloody pulp" }
      };


/**************/
/*** AMOUNT ***/
/**************/


struct dam_weapon_type bite_amount[10] =
      {
         { "leaving $E giggling at $M.", /* // 0: 1% */
         "leaving $E giggling at you.",
         "leaving you giggling at $M." },

      { "making $M sigh in relief.", /* // 1: 2% */
        "making $M sighs in relief.",
        "making you sigh in relief." },

      { "drawing a pained gasp from $M.", /* // 2: 5% */
        "drawing a pained gasp from $M.",
        "drawing a pained gasp from you." },

      { "leaving nasty bite marks.", /* // 3: 10% */
        "leaving nasty bite marks.",
        "leaving nasty bite marks." },

      { "leaving angry welts on $M.", /* // 4: 20% */
        "leaving angry welts on $M.",
        "leaving angry welts on you." },

      { "making $M shout in pain.", /* // 5: 30% */
        "making $M shout in pain.",
        "making you shout in pain." },

      { "making $M scream in agony!", /* // 6: 45% */
        "making $M scream in sheer agony!",
        "making you scream in sheer agony!" },

      { "leaving gaping wounds on $M!", /* // 7: 60% */
        "leaving gaping wounds on $M!",
        "leaving gaping wounds on you!" },

      { "tearing pieces from $M!", /* // 8: 80% */
        "tearing pieces from $M!",
        "tearing pieces from you!" },

      { "making a bloody mess of $M!!", /* // 9: 100% */
        "making a bloody mess of $M!!",
        "making a bloody mess of you!!" }
      };

struct dam_weapon_type bludgeon_amount[10] =
      {
         { "causing small bruises.",                  /*  // 0: 1% */
         "causing small bruises.",
         "causing small bruises." },

      { "causing large bruises.",
        "causing large bruises.",
        "causing large bruises."},

      { "making large welts appear.",
        "making large welts appear.",
        "making large welts appear."},

      { "dislocating a shoulder.",
        "dislocating a shoulder.",
        "dislocating a shoulder."},

      { "causing a rib to crack.",
        "causing a rib to crack.",
        "causing a rib to crack."},

      { "shattering a kneecap.",
        "shattering a kneecap.",
        "shattering a kneecap."},

      { "causing a kidney to rupture.",
        "causing a kidney to rupture.",
        "causing a kidney to rupture."},

      { "shattering a vertebrae.",
        "shattering a vertebrae.",
        "shattering a vertebrae."},

      { "driving ribs into a lung.",
        "driving ribs into a lung.",
        "driving ribs into a lung."},

      { "severely cracking the skull.",
        "severely cracking the skull.",
        "severely cracking the skull."}
      };

struct dam_weapon_type hit_amount[10] =
      {
         {"looking quite silly.", /* // 0: 1% */
         "making yourself look quite silly.",
         "looking quite silly." },

      {"which scares $M.", /* // 1: 2% */
       "scaring $M.",
       "which scares you." },

      { "which makes $M wince.", /* // 2: 5% */
        "which makes $M wince.",
        "which makes you wince." },

      { "leaving slight welts on $M.", /* // 3: 10% */
        "leaving slight welts on $M.",
        "leaving slight wounds on you." },

      { "making $M gasp.", /* // 4: 20% */
        "making $M gasp.",
        "making you gasp." },

      { "leaving stinging bruises.", /* // 5: 30% */
        "leaving stinging bruises.",
        "leaving stinging bruises." },

      { "spinning $M in a circle!", /* // 6: 45% */
        "spinning $M in a circle!",
        "spinning you in a circle!" },

      { "making $M stagger in pain!", /* // 7: 60% */
        "making $M stagger in pain!",
        "making you stagger in pain!" },

      { "knocking $M senseless!", /* // 8: 80% */
        "knocking $M senseless!",
        "knocking you senseless!" },

      { "brutally battering $M!!", /* // 9: 100% */
        "brutally battering $M!!",
        "brutally battering you!!" }
      };

struct dam_weapon_type dam_amount[10] =
      {
         { "which $E hardly notices.",                  /*  // 0: 1% */
         "which $E hardly notices.",
         "which you hardly notice." },

      { "which barely hurts $M.",                   /*   // 1: 2% */
        "which barely hurts $M.",
        "which barely hurts you." },

      { "which slightly wounds $M.",                  /* // 2: 5% */
        "which slightly wounds $M.",
        "which slightly wounds you." },

      { "which wounds $M.",                          /*  // 3: 10% */
        "which wounds $M.",
        "which wounds you." },

      { "which severely wounds $M.",                  /* // 4: 20% */
        "which severely wounds $M.",
        "which severely wounds you." },

      { "which maims $M.",                            /* // 5: 30% */
        "which maims $M.",
        "which maims you." },

      { "which seriously maims $M!",                  /* // 6: 45% */
        "which seriously maims $M!",
        "which seriously maims you!" },

      { "which cripples $M!",                         /* // 7: 60% */
        "which cripples $M!",
        "which cripples you!" },

      { "which completely cripples $M!",              /* // 8: 80% */
        "which completely cripples $M!",
        "which completely cripples you!" },

      { "which totally ANNIHILATES $M!!",             /* // 9: 100% */
        "which totally ANNIHILATES $M!!",
        "which totally ANNIHILATES you!!" }
      };

struct dam_weapon_type pierce_amount[10] =
      {
         { "making small surface cuts.", /* // 0: 1% */
         "making small surface cuts.",
         "making small surface cuts. " },

      { "causing small scratches.",
        "causing small scratches.",
        "causing small scratches."},

      { "causing deep scratches.",
        "causing deep scratches.",
        "causing deep scratches."},

      { "furrowing a deep gouge.",
        "furrowing a deep gouge.",
        "furrowing a deep gouge."},

      { "making a large puncture wound.",
        "making a large puncture wound.",
        "making a large puncture wound."},

      { "cutting dangerously deep.",
        "cutting dangerously deep.",
        "cutting dangerously deep."},

      { "causing blood to flow!",
        "causing blood to flow!",
        "causing blood to flow!"},

      { "making quite a mess!",
        "making quite a mess!",
        "making quite a mess!"},

      { "puncturing vital organs.",
        "puncturing vital organs.",
        "puncturing vital organs."},

      { "severing muscle and sinew!",
        "severing muscle and sinew!",
        "severing muscle and sinew!"}
      };


struct dam_weapon_type punch_amount[10] =
      {
         { "causing $M to snicker in delight.", /* // 0: 1% */
         "causing $M to snicker in delight.",
         "causing you to snicker in delight." },

      { "barely tickling $M.", /* // 1: 2% */
        "barely tickling $M.",
        "barely tickling you." },

      { "throwing $M a bit off balance.", /* // 2: 5% */
        "throwing $M a bit off balance.",
        "throwing you a bit off balance." },

      { "leaving red marks on $M.", /* // 3: 10% */
        "leaving red marks on $M.",
        "leaving red marks on you."},

      { "creating small lesions on $M.", /* // 4: 20% */
        "creating small lesions on $M.",
        "creating small lesions on you." },

      { "leaving quite a gash on $M.", /* // 5: 30% */
        "leaving quite a gash on $M.",
        "leaving quite a gash on you." },

      { "horribly disfiguring $M!", /* // 6: 45% */
        "horribly disfiguring $M!",
        "horribly disfiguring you!" },

      { "mangling $M severely!", /* // 7: 60% */
        "mangling $M severely!",
        "mangling you severely!" },

      { "tearing a hole through $M!", /* // 8: 80% */
        "tearing a hole through $M!",
        "tearing a hole though you!" },

      { "causing temporary loss of conciousness for $M!!", /* // 9: 100% */
        "causing temporary loss of conciousness for $M!!",
        "causing temporary loss of conciousness for you!!" }
      };

struct dam_weapon_type slash_amount[10] =
      {
         { "creating a refreshing breeze for $M.", /* // 0: 1% */
         "creating a fresh breeze for $M.",
         "creating a fresh breeze for you." },

      { "nicking $M.", /* // 1: 2% */
        "nicking $M.",
        "nicking you." },

      { "opening a small gash on $M.", /* // 2: 5% */
        "opening a small gash on $M.",
        "opening a small gash on you." },

      { "opening a messy wound.", /* // 3: 10% */
        "opening a messy wound.",
        "opening a messy wound." },

      { "neatly slicing $M to bits.", /* // 4: 20% */
        "neatly slicing $M to bits.",
        "neatly slicing you to bits." },

      { "seriously wounding $M.", /* // 5: 30% */
        "seriously wounding $M.",
        "seriously wounding you." },

      { "opening a large gash on $M!", /* // 6: 45% */
        "opening a large gash on $M!",
        "opening a large gash on you!" },

      { "causing blood to spray everywhere!", /* // 7: 60% */
        "causing blood to spray everywhere!",
        "causing blood to spray everywhere!" },

      { "severing $N's protruding parts!", /* // 8: 80% */
        "severing $N's protruding parts!",
        "severing your protruding parts!" },

      { "spilling some of $N's vital organs!!", /* // 9: 100% */
        "spilling some of $N's vital organs!!",
        "spilling some of your vital organs!!" }
      };

struct dam_weapon_type thrash_amount[10] =
      {
         { "and $E muffles a laugh.", /* // 0: 1% */
         "and $E muffles a laugh.",
         "and you muffle a laugh." },

      { "glancing a blow off $M.", /* // 1: 2% */
        "glancing a blow off $M.",
        "glancing a blow off." },

      { "lightly bruising $M.", /* // 2: 5% */
        "lightly bruising $M.",
        "lightly bruising you." },

      { "causing $M to flinch in pain.", /* // 3: 10% */
        "causing $M to flinch in pain.",
        "causing you to flinch in pain." },

      { "seriously hurting $M.", /* // 4: 20% */
        "seriously hurting $M.",
        "seriously hurting you." },

      { "causing multiple bruises on $M.", /* // 5: 30% */
        "causing multiple bruises on $M.",
        "causing multiple bruises on you." },

      { "making $M see stars!", /* // 6: 45% */
        "making $M see stars!",
        "making you see stars!" },

      { "really hurting $M a lot!", /* // 7: 60% */
        "really hurting $M a lot!",
        "really hurting you a lot!" },

      { "stunning $M with pain!", /* // 8: 80% */
        "stunning $M with pain!",
        "stunning you with pain!" },

      { "horribly disfiguring $M!!", /* // 9: 100% */
        "horribly disfiguring $M!!",
        "horribly disfiguring you!!" }
      };

/**************/
/*** ASSIGN ***/
/**************/


struct dam_weapon_type *dam_first[] = {
                                         hit_weapons,   /* hit */
                                         bludgeon_weapons,  /* bludgeon */
                                         pierce_weapons,  /* pierce */
                                         slash_weapons,  /* slash */
                                         dam_weapons,   /* blast */
                                         dam_weapons,   /* whip */
                                         dam_weapons,   /* pierce, !BS */
                                         dam_weapons,   /* claw */
                                         bite_weapons,  /* bite */
                                         dam_weapons,   /* sting */
                                         dam_weapons,   /* cleave */
                                         bludgeon_weapons,  /* pound */
                                         dam_weapons,   /* maul */
                                         thrash_weapons,  /* thrash */
                                         punch_weapons,  /* punch */
                                         dam_weapons   /* stab */
                                         };
struct dam_weapon_type *dam_second[] = {
                                          hit_amount,   /* hit */
                                          bludgeon_amount,  /* bludgeon */
                                          pierce_amount,  /* pierce */
                                          slash_amount,  /* slash */
                                          dam_amount,   /* blast */
                                          dam_amount,   /* whip */
                                          dam_amount,   /* pierce, !BS */
                                          dam_amount,   /* claw */
                                          bite_amount,   /* bite */
                                          dam_amount,   /* sting */
                                          dam_amount,   /* cleave */
                                          bludgeon_amount,  /* pound */
                                          dam_amount,   /* maul */
                                          thrash_amount,  /* thrash */
                                          punch_amount,  /* punch */
                                          dam_amount   /* stab */
                                          };

/* Weapon attack texts */
struct attack_hit_type attack_hit_text[] =
      {
      {"hit", "hits" } ,  /* 0 */
      {"bludgeon", "bludgeons" } ,
      {"pierce", "pierces" } ,
      {"slash", "slashes" } ,
      {"blast", "blasts" } ,
      {"whip", "whips" } ,  /* 5*/
      {"pierce", "pierces" } ,
      {"claw", "claws" } ,
      {"bite", "bites" } ,
      {"sting", "stings" } ,
      {"cleave", "cleaves" } ,  /* 10 */
      {"pound", "pounds" } ,
      {"maul", "mauls" } ,
      {"thrash", "thrashes" } ,
      {"punch", "punches" } ,
      {"stab", "stabs" } /* 15 */
      } ;

/* armor damage messages */
struct dam_weapon_type armor_messages[] =
   {
      { "The blow from $n is absorbed by $N's armor.",
        "Your blow deflects off $N's armor.",
        "The blow from $n is absorbed by your armor." },

      { "The blow from $n is absorbed by $N's armor.",
        "Your blow deflects off $N's armor.",
        "The blow from $n is absorbed by your armor." },

      { "$n's pierce is deflected by $N's armor.",
        "Your pierce is deflected by $N's armor.",
        "Your armor protects you from $n's piercing attack." },

      { "$n slashes into $N's armor, but draws no blood.",
        "You slash nothing but armor with your vicious attack.",
        "Your armor protects you from $n's slash." },

      { "$n's blast meets armor, leaving $N unscathed.",
        "Your blast is deflected by $N's armor.",
        "$n blasts into your armor, which leaves you unscathed." },

      { "$n's whip meets $N's armor, but goes no further.",
        "Your whip meets $N's armor, but goes no further.",
        "Your armor deflects a whip from $n." },

      { "$n jabs $s weapon into $N's armor.",
        "Your weapon makes it no further than $N's armor.",
        "The jab from $n is deflected by your armor." },

      { "$n's claws tear at $N's armor.",
        "You tear at $N with your claws but only meet armor.",
        "$n tears at your armor with $s claws." },

      { "$n bites $N but only finds armor in $s jaws.",
        "Your jaws encounter armor as you bite $N.",
        "$n attempts to sink $s teeth into your armor." },

      { "$n's sting is met by $N's armor.",
        "Your sting is absorbed by $N's armor.",
        "Your armor deflects a sting from $n." },

      { "$n cleaves into $N, but only encounters armor.",
        "Your cleave meets nothing but $N's armor.",
        "Your armor saves you from a nasty cleave." },

      { "$n pounds on $N's armor, to no avail.",
        "You pound on $N, but $S armor takes the blunt of it.",
        "Your armor deflects $n's tireless pounding." },

      { "$n mauls $N's armor.",
        "Your maul encounters $N's armor, and does little damage to $M",
        "Your armor saves you from a mauling by $n." },

      { "$N's armor takes a thrashing from $n.",
        "$N's armor absorbs your merciless thrash.",
        "Your armor saves you from being thrashed by $n" },

      { "$N's armor deflects a punch from $n.",
        "Your punch does damage to $N's armor, but little more.",
        "Your armor deflects $n's punch." },

      { "$n jabs $s spear into $N's armor.",
        "Your spear makes it no further than $N's armor.",
        "$n's spear rebounds off your armor." }
   };

#define IS_WEAPON(type) (((type) >= TYPE_HIT) && ((type) < TYPE_SUFFERING))

/* The Fight related routines */
void dam_test(void)
   {
   /*
      log(dam_first[0][0].to_room);
      log(dam_first[0][9].to_room);
      log(dam_first[1][0].to_room);
      log(dam_first[1][9].to_room);
      log(dam_first[15][0].to_room);
      log(dam_first[15][9].to_room);
      log(dam_second[0][0].to_room);
      log(dam_second[0][9].to_room);
      log(dam_second[1][0].to_room);
      log(dam_second[1][9].to_room);
      log(dam_second[15][0].to_room);
      log(dam_second[15][9].to_room);
      log(dam_first[0][0].to_char);
      log(dam_first[0][9].to_char);
      log(dam_first[1][0].to_char);
      log(dam_first[1][9].to_char);
      log(dam_first[15][0].to_char);
      log(dam_first[15][9].to_char);
      log(dam_second[0][0].to_char);
      log(dam_second[0][9].to_char);
      log(dam_second[1][0].to_char);
      log(dam_second[1][9].to_char);
      log(dam_second[15][0].to_char);
      log(dam_second[15][9].to_char);
      */
   return;
   }
int encumberance_level(struct char_data *ch)
   {
   int weight_worn=0,i, level=0, items_worn = 0;

   if(IS_NPC(ch))
      return 2;
   for (i=0; i < NUM_WEARS; i++)
      if (ch->equipment[i])
         {
         weight_worn += ch->equipment[i]->obj_flags.weight;
         items_worn++;
         }
   weight_worn /= 2;
   items_worn /= 3;
   if ((IS_CARRYING_W(ch) + weight_worn) > (4*CAN_CARRY_W(ch))/5)
      level = 8;
   else if ((IS_CARRYING_W(ch) + weight_worn) > (3*CAN_CARRY_W(ch))/5)
      level = 6;
   else if ((IS_CARRYING_W(ch) + weight_worn) > (2*CAN_CARRY_W(ch))/5)
      level = 4;
   else if ((IS_CARRYING_W(ch) + weight_worn) > (CAN_CARRY_W(ch))/5)
      level = 2;

   if ((IS_CARRYING_N(ch) + items_worn) > (4*CAN_CARRY_N(ch))/5)
      level += 4;
   else if ((IS_CARRYING_N(ch) + items_worn) > (3*CAN_CARRY_N(ch))/5)
      level += 3;
   else if ((IS_CARRYING_N(ch) + items_worn) > (2*CAN_CARRY_N(ch))/5)
      level += 2;
   else if ((IS_CARRYING_N(ch) + items_worn) > (CAN_CARRY_N(ch))/5)
      level += 1;

   level /= 2;
   return(level);
   }
int fatigue_bonus(struct char_data *ch)
   {

   int max;

   max = GET_MAX_MOVE(ch);

   if (GET_MOVE(ch) > (3*max)/5)
      return(-3);
   else if (GET_MOVE(ch) > (2*max)/5)
      return(-2);
   else if (GET_MOVE(ch) > (max)/5)
      return(-1);
   else
      return(0);

   }

int weapon_weight_bonus(struct char_data *ch)
   {
   int level;
   struct obj_data *obj;
   int weight;
   int counter=0, worn_count=0;

   /*
    * if fail skill test don't use 2nd weapon 
    */

   if(!GET_EQ(ch,WEAR_WIELD_1)&&!GET_EQ(ch,WEAR_WIELD_2)) /* no weapon */
      return 3;
   else if(GET_EQ(ch,WEAR_WIELD_2)&&(LAST_HAND_USED(ch)==1)&&
           (IS_NPC(ch)||GET_SKILL(ch,SKILL_DUAL_WIELD)<=number(1,101)))
      {
      improve_skill(ch,SKILL_DUAL_WIELD,AUTO_PASS);
      obj=GET_EQ(ch,WEAR_WIELD_2);
      LAST_HAND_USED(ch)=2;
      return -7;  /* adjust this to alter dual speed */
      }
   else    /* use weapon in hand 1 */
      {
      if(!IS_NPC(ch)&&GET_EQ(ch,WEAR_WIELD_2)&&(LAST_HAND_USED(ch)==1))
         improve_skill(ch,SKILL_DUAL_WIELD,AUTO_FAIL);

      if(GET_EQ(ch,WEAR_WIELD_1))
         obj=GET_EQ(ch,WEAR_WIELD_1);
      else if(GET_EQ(ch,WEAR_WIELD_2)) /* NO WEAPON IN HAND 1!!! */
         obj=GET_EQ(ch,WEAR_WIELD_2);
      else
         return 3;

      LAST_HAND_USED(ch)=1;
      weight=GET_OBJ_WEIGHT(obj);
      if((obj=GET_EQ(ch,WEAR_WIELD_2))!=NULL)
         weight+=GET_OBJ_WEIGHT(obj);
      }

   if(weight > (int)((4.0*CAN_WIELD_W(ch))/5.0))
      level=12;
   else if(weight > (int)((3.0*CAN_WIELD_W(ch))/5.0))
      level=8;
   else if(weight > (int)((2.0*CAN_WIELD_W(ch))/5.0))
      level=4;
   else if(weight > (int)(CAN_WIELD_W(ch)/5.0))
      level=2;
   else
      level=1;

   if(TWO_HANDED(GET_EQ(ch,WEAR_WIELD_1)))
      level/=3;   /* adjust here for 2handed */
   else    /* assume 2 handed if only holding 1 weapon */
      {
      for(counter=0;counter<NUM_HAND_POSITIONS;counter++)
         if(GET_EQ(ch,hand_position[counter]))
            worn_count++;
      if(worn_count==1)
         level/=2;  /* adjust here for 2handed */
      }
   return level;
   }

int calculate_speed(struct char_data *ch)
   {
   int speed=15;
   int percent;
   int prob;

   speed+=dex_app[stat_index(GET_DEX(ch))].reaction; /* hey, I like thieves */
   speed+=number(1,6);  /* a slight bit of randomness :) */


   if(AFF_FLAGGED(ch,AFF_HASTE)) /* adjust here for spell effects */
      speed-=4;
   if(AFF_FLAGGED(ch,AFF_RAGE))
      speed-=2;
   if(AFF_FLAGGED(ch,AFF_SLOW))
      speed+=5;

   if(!GET_EQ(ch,WEAR_WIELD_1))
      {
      if(GET_EQ(ch,WEAR_WIELD_2))
         {
         send_to_char(ch,"You switch your remaining weapon to your good hand.\r\n");
         equip_char(ch,unequip_char(ch,WEAR_WIELD_2), WEAR_WIELD_1);
         }
      }

   speed+=encumberance_level(ch);
   speed+=fatigue_bonus(ch);
   speed+=weapon_weight_bonus(ch);

   if (!IS_NPC(ch))  /* adjust here for skill/level speeds */
      {

	if(SCR_SKILLCHECK(ch, SKILL_SECOND_ATTACK) && (prob = GET_SKILL(ch,SKILL_SECOND_ATTACK))>0)
         {
         percent = number(1, 101);
         if (prob > percent)
            {
            speed-=6;
            improve_skill(ch,SKILL_SECOND_ATTACK,AUTO_PASS);
            }
         else
            {
            improve_skill(ch,SKILL_SECOND_ATTACK,AUTO_FAIL);
            }
         }

	if(SCR_SKILLCHECK(ch, SKILL_THIRD_ATTACK) && (prob = GET_SKILL(ch,SKILL_THIRD_ATTACK))>0)
         {
         percent = number(1, 101);
         if (prob > percent)
            {
            speed-=6;
            improve_skill(ch,SKILL_THIRD_ATTACK,AUTO_PASS);
            }
         else
            {
            improve_skill(ch,SKILL_THIRD_ATTACK,AUTO_FAIL);
            }

         }

	if(SCR_SKILLCHECK(ch, SKILL_FOURTH_ATTACK) && (prob = GET_SKILL(ch,SKILL_FOURTH_ATTACK))>0)
         {
         percent = number(1, 101);
         if (prob > percent)
            {
            speed-=6;
            improve_skill(ch,SKILL_FOURTH_ATTACK,AUTO_PASS);
            }
         else
            {
            improve_skill(ch,SKILL_FOURTH_ATTACK,AUTO_FAIL);
            }

         }
      }
   else
      {
      if((GET_LEVEL(ch)>=25)&&(number(0,100)<75))
         speed-=6;
      if((GET_LEVEL(ch)>=50)&&(number(0,100)<75))
         speed-=6;
      if((GET_LEVEL(ch)>=75)&&(number(0,100)<75))
         speed-=6;
      if((GET_LEVEL(ch)>=95)&&(number(0,100)<75))
         speed-=6;
      }

   /*    speed*=2; */
   speed/=2;   /* this is for adjusting the round speeds */
   /*  log("Speed: %s:%d",GET_NAME(ch),speed);  */
   return MIN(40,MAX(0,speed));
   }

void appear(struct char_data * ch)
   {
   if (affected_by_spell(ch, SPELL_INVISIBLE))
      affect_from_char(ch, SPELL_INVISIBLE);
   if (affected_by_spell(ch, SKILL_SHADOW))
      affect_from_char(ch, SKILL_SHADOW);

   REMOVE_BIT(AFF_FLAGS(ch), AFF_INVISIBLE | AFF_HIDE);
   REMOVE_BIT(AFF2_FLAGS(ch), AFF2_SHADOW);

   if (GET_LEVEL(ch) < LVL_IMMORT)
      act("$n slowly fades into existence.", FALSE, ch, 0, 0, TO_ROOM);
   else
      act("You feel a strange presence as $n appears, seemingly from nowhere.",
          FALSE, ch, 0, 0, TO_ROOM);
   }

int compute_armor_class(struct char_data *ch)
   {
   int armorclass = GET_AC(ch);

   if (AWAKE(ch))
      armorclass += dex_app[stat_index(GET_DEX(ch))].defensive;

   return (MAX(-200, armorclass));      /* -200 is lowest */
   }



void load_messages(void)
   {
   FILE *fl;
   int i, type;
   struct message_type *messages;
   char *chk=get_buffer(128);

   if (!(fl = fopen(MESS_FILE, "r")))
      {
      log("SYSERR: Error reading combat message file %s: %s", MESS_FILE,
          strerror(errno));
      exit(1);
      }
   for (i = 0; i < MAX_MESSAGES; i++)
      {
      fight_messages[i].a_type = 0;
      fight_messages[i].number_of_attacks = 0;
      fight_messages[i].msg = 0;
      }


   fgets(chk, 128, fl);
   while (!feof(fl) && (*chk == '\n' || *chk == '*'))
      fgets(chk, 128, fl);

   while (*chk == 'M')
      {
      fgets(chk, 128, fl);
      sscanf(chk, " %d\n", &type);
      for (i = 0; (i < MAX_MESSAGES) && (fight_messages[i].a_type != type) &&
              (fight_messages[i].a_type); i++)
         ;
      if (i >= MAX_MESSAGES)
         {
         log("SYSERR: Too many combat messages %d/%d.  Increase "
             "MAX_MESSAGES and recompile.",i,MAX_MESSAGES);
         exit(1);
         }
      CREATE(messages, struct message_type, 1);
      fight_messages[i].number_of_attacks++;
      fight_messages[i].a_type = type;
      messages->next = fight_messages[i].msg;
      fight_messages[i].msg = messages;

      messages->die_msg.attacker_msg = fread_action(fl, i);
      messages->die_msg.victim_msg = fread_action(fl, i);
      messages->die_msg.room_msg = fread_action(fl, i);
      messages->miss_msg.attacker_msg = fread_action(fl, i);
      messages->miss_msg.victim_msg = fread_action(fl, i);
      messages->miss_msg.room_msg = fread_action(fl, i);
      messages->hit_msg.attacker_msg = fread_action(fl, i);
      messages->hit_msg.victim_msg = fread_action(fl, i);
      messages->hit_msg.room_msg = fread_action(fl, i);
      messages->god_msg.attacker_msg = fread_action(fl, i);
      messages->god_msg.victim_msg = fread_action(fl, i);
      messages->god_msg.room_msg = fread_action(fl, i);
      fgets(chk, 128, fl);
      while (!feof(fl) && (*chk == '\n' || *chk == '*'))
         fgets(chk, 128, fl);
      }

   release_buffer(chk);
   fclose(fl);
   log("...%d read, a total of %d allowed",i,MAX_MESSAGES);
   }


void update_pos(struct char_data * victim)
   {

   if ((GET_HIT(victim) > 0) && (GET_POS(victim) > POS_STUNNED))
      return;
   else if (GET_HIT(victim) > 0)
      GET_POS(victim) = POS_STANDING;
   else if (GET_HIT(victim) <= -11)
      GET_POS(victim) = POS_DEAD;
   else if (GET_HIT(victim) <= -6)
      GET_POS(victim) = POS_MORTALLYW;
   else if (GET_HIT(victim) <= -3)
      GET_POS(victim) = POS_INCAP;
   else
      GET_POS(victim) = POS_STUNNED;
   }


void check_killer(struct char_data * ch, struct char_data * vict)
   {
   char *buf;

   if(ROOM_FLAGGED(IN_ROOM(ch),ROOM_PKILL)||Z_FLAGGED(IN_ROOM(ch),Z_PKILL))
      return;
   if (PLR_FLAGGED(vict, PLR_KILLER) || PLR_FLAGGED(vict, PLR_THIEF))
      return;
   if (PLR_FLAGGED(ch, PLR_KILLER) || IS_NPC(ch) || IS_NPC(vict) || ch == vict)
      return;
   if (PLR_FLAGGED(ch, PLR_PK) && PLR_FLAGGED(vict, PLR_PK) && abs(GET_LEVEL(ch)-GET_LEVEL(vict))<=10)
      return;
   buf=get_buffer(256);

   SET_BIT(PLR_FLAGS(ch), PLR_KILLER);
   sprintf(buf, "PC Killer bit set on %s for initiating attack on %s at %s.",
           GET_NAME(ch), GET_NAME(vict), world[vict->in_room].name);
   mudlog(buf, BRF, LVL_IMMORT, TRUE);
   send_to_char(ch,"If you want to be a PLAYER KILLER, so be it...\r\n");

   send_info("[ INFO ] %s has recieved a Player Killer flag for "
             "attacking %s.\n\r", GET_NAME(ch), GET_NAME(vict));
   send_info("[ INFO ] Everyone in the game now has the right to "
             "kill %s.\n\r",GET_NAME(ch));
   release_buffer(buf);
   }


/* start one char fighting another (yes, it is horrible, I know... )  */
void set_fighting(struct char_data * ch, struct char_data * vict)
   {
   if (ch == vict)
      return;

   if(FIGHTING(ch))
      {
      mudlogf(BRF,LVL_IMMORT,FALSE,"SYSERR: Fighting already in set_fighting: %s and %s",
              GET_NAME(ch),GET_NAME(vict));
      core_dump();
      return;
      }

   if (PLR_FLAGGED(ch, PLR_FISHING))
      REMOVE_BIT(PLR_FLAGS(ch), PLR_FISHING | PLR_FISH_ON);
   if (PLR_FLAGGED(vict, PLR_FISHING))
      REMOVE_BIT(PLR_FLAGS(vict), PLR_FISHING | PLR_FISH_ON);

   if((GET_POS(ch)!=POS_STANDING)&&FURNITURE(ch))
      char_from_object(ch,FURNITURE(ch));

   ch->next_fighting = combat_list;
   combat_list = ch;

   if (AFF_FLAGGED(ch, AFF_SLEEP))
      affect_from_char(ch, SPELL_SLEEP);

   FIGHTING(ch) = vict;
   GET_POS(ch) = POS_FIGHTING;

   if (!pk_allowed)
      check_killer(ch, vict);
}



/* remove a char from the list of fighting chars */
void stop_fighting(struct char_data * ch)
   {
   struct char_data *temp;

   if (ch == next_combat_list)
      next_combat_list = ch->next_fighting;

   REMOVE_FROM_LIST(ch, combat_list, next_fighting);
   ch->next_fighting = NULL;
   FIGHTING(ch) = NULL;
   GET_POS(ch) = POS_STANDING;
   update_pos(ch);
   }



void make_corpse(struct char_data * ch, struct char_data * killer)
   {
   struct obj_data *corpse, *o;
   struct obj_data *money;
   int i;
   char *buf=get_buffer(512);
   char *buf2=get_buffer(256);


   if((GET_RACE(ch)==MRACE_GHOST)||(GET_RACE(ch)==MRACE_IMMATERIAL))
      {
      /* transfer gold */
      if (GET_GOLD(ch) > 0)
         {
         /* following 'if' clause added to fix gold duplication loophole */
         if (IS_NPC(ch) || (!IS_NPC(ch) && ch->desc))
            {
            money = create_money(GET_GOLD(ch));
            obj_to_room(money, IN_ROOM(ch));
            }
         GET_GOLD(ch) = 0;
         }
      release_buffer(buf);
      release_buffer(buf2);
      return;
      }
   else
      {
      corpse = create_obj();

      corpse->item_number = NOTHING;
      IN_ROOM(corpse) = NOWHERE;
      if(IS_UNDEAD(ch))
         sprintf(buf,"pile dust %s npccorpse",GET_PC_NAME(ch));
      else if(IS_ANIMAL(ch))
         sprintf(buf,"carcass corpse %s npccorpse",GET_PC_NAME(ch));
      else {
         if(IS_NPC(ch))
           sprintf(buf,"corpse %s npccorpse",GET_PC_NAME(ch));
         else
           sprintf(buf,"corpse %s",GET_PC_NAME(ch));
         }
      corpse->name = str_dup(buf);
      release_buffer(buf);

      if(IS_UNDEAD(ch))
         sprintf(buf2, "A pile of dust is blowing in the wind.");
      else if(IS_ANIMAL(ch))
         sprintf(buf2, "The carcass of %s is lying here.", GET_NAME(ch));   
      else
         sprintf(buf2, "The corpse of %s is lying here.", GET_NAME(ch));
      corpse->description = str_dup(buf2);

      if(IS_UNDEAD(ch))
         sprintf(buf2, "the pile of dust");
      else if(IS_ANIMAL(ch))
         sprintf(buf2, "the carcass of %s", GET_NAME(ch));
      else
         sprintf(buf2, "the corpse of %s", GET_NAME(ch));
      corpse->short_description = str_dup(buf2);

      if(IS_UNDEAD(ch))
         strcpy(buf2, "Wisps of dust blow across the room.");
      else if(IS_ANIMAL(ch))
         sprintf(buf2, "The stench rising from the carcass makes your gorge "
                "rise.");
      else
         strcpy(buf2, "The stench rising from the corpse makes your gorge "
                "rise.");
      corpse->action_description = str_dup(buf2);
      release_buffer(buf2);

      GET_OBJ_TYPE(corpse) = ITEM_CONTAINER;
      GET_OBJ_WEAR(corpse) = ITEM_WEAR_TAKE;
      GET_OBJ_EXTRA(corpse) = ITEM_NODONATE|ITEM_DO_ACT|ITEM_NOSELL|ITEM_NORENT|ITEM_UNIQUE_SAVE;
      GET_OBJ_VAL(corpse, 0) = 0; /* You can't store stuff in a corpse */
      GET_OBJ_VAL(corpse, 3) = 1; /* corpse identifier */
      if(IS_NPC(ch))          /* to log player corpse looting */
         {
         GET_OBJ_VAL(corpse, 4) = ch->mob_specials.skin;
         GET_OBJ_VAL(corpse, 6) = 0;
         }
      else
         {
         GET_OBJ_VAL(corpse, 4) = NOTHING;
         if (!IS_NPC(killer) && (PLR_FLAGGED(ch,PLR_PK) && PLR_FLAGGED(killer,PLR_PK) && 
            abs(GET_LEVEL(ch)-GET_LEVEL(killer))<=10))
            {
            GET_OBJ_VAL(corpse, 6) = 0;
            }
         else
            {
            GET_OBJ_VAL(corpse, 6) = GET_IDNUM(ch);
            }
         }
      GET_OBJ_VAL(corpse, 7) = GET_LEVEL(ch); /* animate dead */

      GET_OBJ_WEIGHT(corpse) = GET_WEIGHT(ch) + IS_CARRYING_W(ch);
      GET_OBJ_RENT(corpse) = 100000;
      GET_OBJ_TSLOTS(corpse)=10;
      GET_OBJ_OSLOTS(corpse)=20;
      GET_OBJ_CSLOTS(corpse)=5;
      if (IS_NPC(ch))
         {
         GET_OBJ_TIMER(corpse) = max_npc_corpse_time;
         SET_BIT(GET_OBJ_EXTRA(corpse), ITEM_NPC_CORPSE);
         GET_OBJ_VROOM(corpse) = NOWHERE;
         }
      else
         {
         if (ROOM_FLAGGED(ch->in_room, ROOM_HOUSE))
            {
            SET_BIT(GET_OBJ_EXTRA(corpse), ITEM_NPC_CORPSE);
            GET_OBJ_VROOM(corpse) = NOWHERE;
            }
         else
            {
            SET_BIT(GET_OBJ_EXTRA(corpse), ITEM_PC_CORPSE);
            GET_OBJ_VROOM(corpse) = GET_ROOM_VNUM(ch->in_room);
            }
         GET_OBJ_TIMER(corpse) = max_pc_corpse_time;
         }
      corpse->touched = TRUE;
      GET_OBJ_DGTIMER(corpse)=-1;

      if (!IS_NPC(ch)) {
	printf("Death check for %s, killer: %s\n", GET_NAME(ch), GET_NAME(killer));
      }

      //      if((!ROOM_FLAGGED(IN_ROOM(ch),ROOM_PKILL)&&!Z_FLAGGED(IN_ROOM(ch),Z_PKILL)) &&
      //!(PLR_FLAGGED(ch, PLR_PK) && PLR_FLAGGED(killer, PLR_PK) && abs(GET_LEVEL(ch)-GET_LEVEL(killer))<=10))

      if(
	 (
	  !ROOM_FLAGGED(IN_ROOM(ch),ROOM_PKILL) // The room's not flagged PK
	  && !Z_FLAGGED(IN_ROOM(ch),Z_PKILL) // The zone's not flagged PK
	  && !(PLR_FLAGGED(ch, PLR_PK) && PLR_FLAGGED(killer, PLR_PK) && abs(GET_LEVEL(ch)-GET_LEVEL(killer))<=10) // This isn't a PK-player killing another PK-player...
	  )
	 || 
	 ch == killer // This player killed themselves (or by sun damage, brew, etc.)
	)
         {
	   if (!IS_NPC(ch)) {
	     printf("Transferring %s eq to corpse...\n", GET_NAME(ch));
	   }

         /* transfer character 's inventory to the corpse */
         corpse->contains = ch->carrying;
         for (o = corpse->contains; o != NULL; o = o->next_content)
            o->in_obj = corpse;

         /* transfer character's equipment to the corpse */
         for (i = 0; i < NUM_WEARS; i++)
            if (GET_EQ(ch, i))
               {
               if (i != WEAR_HEART) /* retain heartworn on death */
                  {
                  remove_otrigger(GET_EQ(ch,i),ch);
                  obj_to_obj(unequip_char(ch, i), corpse);
                  }
               }

         object_list_new_owner(corpse, NULL);
         /* transfer gold */
         if (GET_GOLD(ch) > 0)
            {
            /* following 'if' clause added to fix gold duplication loophole */
            if (IS_NPC(ch) || (!IS_NPC(ch) && ch->desc))
               {
               money = create_money(GET_GOLD(ch));
               SET_BIT(GET_OBJ_EXTRA(money), ITEM_UNIQUE_SAVE);
               obj_to_obj(money, corpse);
               }
            GET_GOLD(ch) = 0;
            }
         ch->carrying = NULL;
         IS_CARRYING_N(ch) = 0;
         IS_CARRYING_W(ch) = 0;
         }
      else
         {
         corpse->contains =NULL;
         /* transfer any BATTLE_ITEMs from the char to the room
            first check items being carried, then items being worn.
            battle_items cannot be put into a container, so don't
            check in containers, and just drop them on the ground */

         /* items being carried */
         for (o = ch->carrying; o != NULL; o = o->next_content)
            {
            if (IS_OBJ_STAT(o, ITEM_BATTLE_ITEM))
               {
               if(drop_otrigger(o,ch)==FALSE)
                  continue;
               obj_from_char(o);
               obj_to_room(o, IN_ROOM(ch));
               }
            }
         /* objects worn */
         for (i = 0; i < NUM_WEARS; i++)
            if (GET_EQ(ch, i))
               {
               if (IS_OBJ_STAT(GET_EQ(ch,i), ITEM_BATTLE_ITEM))
                  {
                  remove_otrigger(GET_EQ(ch,i),ch);
                  obj_to_room(unequip_char(ch, i), IN_ROOM(ch));
                  }
               }

         }

      if (!IS_NPC(ch) && (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) == 0)) {
	obj_to_room(corpse, real_room(3054));
	mudlogf(NRM, LVL_IMMORT, TRUE, "RIP: %s died at room #%ld(%s), moved corpse to room 3054 instead.", GET_NAME(ch), IN_ROOM(ch), world[IN_ROOM(ch)].name);
      }
      /*
      else if (!IS_NPC(ch) && REMORT_LEVEL(ch) == 0 && GET_LEVEL(ch) <= 20) {
       if (GET_HOME(ch)/100 == 10) {
         obj_to_room(corpse, real_room(1076));
       } else {
         obj_to_room(corpse, real_room(2967));
       }
      } else {
      }*/
      obj_to_room(corpse, IN_ROOM(ch));

      save_corpses();
      }
   Crash_crashsave(ch);
   }


#define ADJUST_NEUTRAL_ALIGN(ch) (((GET_LEVEL(ch)*75)/100)+25) /* 1/25th-1/100th */
#define ADJUST_NORMAL_ALIGN(ch) ((GET_LEVEL(ch)/5)+12) /* 1/12th-1/32nd */

/* When ch kills victim */
void change_alignment(struct char_data * ch, struct char_data * victim)
   {
   /*
    * new alignment change algorithm: if you kill a monster with alignment A, 
    * you move 1/16th of the way to having alignment -A.  Simple and fast. 
    *
    * switched to a system of less change for higher level. - Nomikos 10-4-02 
    */
   if(GET_ALIGNMENT(ch)>250)
      {
      if(GET_ALIGNMENT(victim)>250)
         {
         GET_ALIGNMENT(ch)-=((GET_ALIGNMENT(victim)/ADJUST_NORMAL_ALIGN(ch)));
         }
      else if(GET_ALIGNMENT(victim)<-250)
         {
         GET_ALIGNMENT(ch)+=((-GET_ALIGNMENT(victim)/ADJUST_NORMAL_ALIGN(ch)));
         }
      else /*neutral*/
         {
         GET_ALIGNMENT(ch)+=((-GET_ALIGNMENT(victim)/ADJUST_NEUTRAL_ALIGN(ch)));
         }
      }
   else if(GET_ALIGNMENT(ch)<-250)
      {
      if(GET_ALIGNMENT(victim)>250)
         {
         GET_ALIGNMENT(ch)-=((GET_ALIGNMENT(victim)/ADJUST_NORMAL_ALIGN(ch)));
         }
      else if(GET_ALIGNMENT(victim)<-250)
         {
         GET_ALIGNMENT(ch)+=((-GET_ALIGNMENT(victim)/ADJUST_NORMAL_ALIGN(ch)));
         }
      else /*neutral*/
         {
         GET_ALIGNMENT(ch)+=((-GET_ALIGNMENT(victim)/ADJUST_NEUTRAL_ALIGN(ch)));
         }
      }
   else /*neutral*/
      {
      if(GET_ALIGNMENT(victim)>0)
         {
         GET_ALIGNMENT(ch)-=((GET_ALIGNMENT(victim)/ADJUST_NEUTRAL_ALIGN(ch)));
         }
      else if(GET_ALIGNMENT(victim)<=0)
         {
         GET_ALIGNMENT(ch)+=((-GET_ALIGNMENT(victim)/ADJUST_NEUTRAL_ALIGN(ch)));
         }
      }
   GET_ALIGNMENT(ch)= MAX(-1000, MIN(1000, GET_ALIGNMENT(ch)));
   }

/*
 * Killing Same mobs XP gain limit function
 */
#define GET_PERCENT(num, from_v) ((int)((float)(from_v) / 100.0) * (float)(num))

/*
 * slow exp decrease
 */
#define KILL_UNDER_MF(exp, num_kills) \
   (MAX(1, GET_PERCENT(exp, ((101 - (MIN(101, (num_kills) * 3)))))))

/*
 * Fast exp decrease
 */
#define KILL_ABOVE_MF(exp, num_kills) \
   (MAX(1, GET_PERCENT(exp, 100.0 / ((float)(num_kills) / 3.0))))


/* What is this!?! Nomi May 2025 */
void test()
   {
   }


/*
 * Calculate the expgain limit and add to the kill buffer
 */
int kills_limit_xpgain(struct char_data *ch, struct char_data *victim, int exp)
   {
   int victim_vnum = GET_MOB_VNUM(victim);
   int new_exp;
   int i;
   int found = 255;
   int temp_num;

   /* Adjusted this from 1/4 of your level to 3/4 of your level - Nomikos 5/8/2025 */
   /* NPCs below 3/4 of your level don't go on your buffer */
   /* this will keep newbie zones safer */
   if (GET_LEVEL(victim) < GET_LEVEL(ch) && GET_LEVEL(ch) > LVL_NEWBIE)
      if ((GET_LEVEL(victim) + (GET_LEVEL(ch) * 1/4)) < GET_LEVEL(ch))
         return exp;

   /* With the change above, adjusted this from 127 to 64 - Nomikos 5/8/2025 */
   for (i = 0; i < 64; i++)
      {
      if(GET_KILLS_VNUM(ch, i) == victim_vnum)
         {
         found = i;
         break;
         }
      }
   /*
    * for(i=1;i<255;i++)
    *  {
    *  log("Percent:%d, kill_under:%d",GET_PERCENT(100,i),KILL_UNDER_MF(100,255-i));
    *  }
    */

   if (found == 255)
      {
      for(i = 62; i >= 0; i--)
         {
         GET_KILLS_VNUM(ch, i + 1) = GET_KILLS_VNUM(ch, i);
         GET_KILLS_AMMOUNT(ch, i + 1) = GET_KILLS_AMMOUNT(ch, i);
         }
      GET_KILLS_VNUM(ch, 0) = victim_vnum;
      GET_KILLS_AMMOUNT(ch, 0) = 0;
      }
   else if (found!=0)
      {
      temp_num = GET_KILLS_AMMOUNT(ch, found);
      for(i = found - 1; i >= 0; i--)
         {
         GET_KILLS_VNUM(ch, i + 1) = GET_KILLS_VNUM(ch, i);
         GET_KILLS_AMMOUNT(ch, i + 1) = GET_KILLS_AMMOUNT(ch, i);
         }
      GET_KILLS_VNUM(ch, 0) = victim_vnum;
      GET_KILLS_AMMOUNT(ch, 0) = temp_num;
      }


   /*
      if(GET_KILLS_AMMOUNT(ch,0) > GET_MOB_MAXFACTOR(victim))
         {
         new_exp = KILL_UNDER_MF(exp,GET_MOB_MAXFACTOR(victim));
         new_exp = KILL_ABOVE_MF(new_exp, (GET_KILLS_AMMOUNT(ch,0) -
        GET_MOB_MAXFACTOR(victim)));
         }
      else*/

   if (GET_KILLS_AMMOUNT(ch, 0) > 101)
      new_exp = KILL_UNDER_MF(exp, 101);
   else if (GET_KILLS_AMMOUNT(ch, 0) > 1)
      new_exp = KILL_UNDER_MF(exp, GET_KILLS_AMMOUNT(ch, 0));
   else
      new_exp = exp;

   if(GET_KILLS_AMMOUNT(ch, 0) < 255)
      GET_KILLS_AMMOUNT(ch, 0) += 1;

   return new_exp;
   }


/* Calculate the per hit xpgain damage and kill buffer */
int kills_limit_damage_xpgain(struct char_data *ch, struct char_data *victim, int exp)
{
   int victim_vnum = GET_MOB_VNUM(victim);
   int i;
   int found = 255;

   /* Lowered kill buffer from 127 to 64 - Nomikos 5/8/2025 */
   for (i = 0; i < 64; i++)
      {
      if (GET_KILLS_VNUM(ch, i) == victim_vnum)
         {
         found = i;
         break;
         }
      }

   if (found == 255)
   {
      return exp;
   }
	
   if (GET_KILLS_AMMOUNT(ch, found) > 101)
      return KILL_UNDER_MF(exp, 101);
   else if (GET_KILLS_AMMOUNT(ch, found) > 1)
      return KILL_UNDER_MF(exp, GET_KILLS_AMMOUNT(ch, found));
   else
      return exp;
}


void death_cry(struct char_data * ch)
   {
   int door, was_in;

   if (MOB2_FLAGGED(ch, MOB2_COMPONENT) && ch->master)
      act("Your blood freezes as you hear $n screech in agony.",
           FALSE,ch->master,0,0,TO_ROOM);
   else
      act("Your blood freezes as you hear $n's death cry.",
           FALSE,ch,0,0,TO_ROOM);
   was_in = IN_ROOM(ch);

   for (door = 0; door < NUM_OF_DIRS; door++)
      {
      if (CAN_GO(ch, door))
         {
         IN_ROOM(ch) = world[was_in].dir_option[door]->to_room;
         if (MOB2_FLAGGED(ch, MOB2_COMPONENT) && ch->master)
            act("Your blood freezes as you hear something screech in agony.",
                 FALSE, ch, 0, 0, TO_ROOM);
         else
            act("Your blood freezes as you hear someone's death cry.", FALSE,
                 ch, 0, 0, TO_ROOM); 
         IN_ROOM(ch) = was_in;
         }
      }
   }



void raw_kill(struct char_data * ch, struct char_data *killer)
{
  struct char_data *current;
  struct descriptor_data *i;
  int tmp;
  if (FIGHTING(ch))
    stop_fighting(ch);
  
  /* I believe this should be here - Nomikos 8/31/03 */
  if (FIGHTING(killer))
    stop_fighting(killer);
  
  if(IS_NPC(ch)&&!IS_NPC(killer))
  {
    if (GET_OLD_MOBKILLS(killer) > 0)
    {
      GET_MOBKILLS(killer) = GET_OLD_MOBKILLS(killer);
      GET_OLD_MOBKILLS(killer) = 0;
    }
    GET_MOBKILLS(killer)=MIN(2000000,1+GET_MOBKILLS(killer));
  }

  if(IS_NPC(killer)&&!IS_NPC(ch))
    GET_DEATHS(ch)=MIN(30000,1+GET_DEATHS(ch));
  
  while (ch->affected)
    affect_remove(ch, ch->affected);
  
  if (killer)
    mprog_death_trigger(ch, killer);
  
  if(killer)
  {
    if(death_mtrigger(ch,killer))
      death_cry(ch);
  }
  else
    death_cry(ch);
  
  if (!MOB2_FLAGGED(ch, MOB2_COMPONENT))
    make_corpse(ch, killer);
  
  if (  (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_PKILL) && !Z_FLAGGED(IN_ROOM(ch),Z_PKILL)) && !(PLR_FLAGGED(ch, PLR_PK) && PLR_FLAGGED(killer, PLR_PK) && abs(GET_LEVEL(ch)-GET_LEVEL(killer))<=10)   )
  {
    /* char_from_room(ch);
    ** tmp=real_room(mortal_start_room); 
    ** if(tmp<0) 
    **    { 
    **    log("ERROR IN PKILL CODE.  unknown 3001 room"); 
    **    tmp=0; 
    **    } 
    ** char_to_room(ch, tmp); 
    */
    if(!IS_NPC(ch))
    {
      GET_LOADROOM(ch)=GET_HOME(ch);
      REMOVE_BIT(PLR_FLAGS(ch),PLR_THIEF|PLR_KILLER);
    }
    /* get rid of death lag here maybe? */
    extract_char(ch);
  }
  else if ((!IS_NPC(ch) && !IS_NPC(killer) && (PLR_FLAGGED(ch, PLR_PK) && PLR_FLAGGED(killer, PLR_PK) && 
					       abs(GET_LEVEL(ch)-GET_LEVEL(killer))<=10) && (!ROOM_FLAGGED(IN_ROOM(ch),ROOM_PKILL)) && 
	    !Z_FLAGGED(IN_ROOM(ch),Z_PKILL)))
  {
    char_from_room(ch);
    if( (tmp=real_room(GET_HOME(ch)))<0)
      tmp=real_room(mortal_start_room);
    if(tmp<0)
    {
      log("ERROR IN PKILL CODE.  unknown %ld room",mortal_start_room);
      tmp=0;
    }
    char_to_room(ch, tmp);
    GET_POS(ch) = POS_STANDING;
    do_look(ch, "", 0, 0);
    
    if (!IS_NPC(ch) && !IS_NPC(killer) && (GET_PKILLS(killer) < 30000) &&(ch != killer))
      GET_PKILLS(killer)+=1;
    
    GET_MANA(ch) = 10;
    GET_HIT(ch) = 10;
    GET_MOVE(ch) = 10;
  }
  else
  {
    char_from_room(ch);
    if( (tmp=real_room(GET_HOME(ch)))<0)
      tmp=real_room(mortal_start_room);
    if(tmp<0)
    {
      log("ERROR IN PKILL CODE.  unknown %ld room",mortal_start_room);
      tmp=0;
    }
    /*
    if (tmp == 0 && IS_NPC(ch)) {
      tmp = real_room(1199);
      char_to_room(ch, tmp);
      return;
    }
    */
    if (IS_NPC(ch)) {
      /* A mob died in a PK room.  Leave only its corpse behind. */
      /* First, remove it equipment. */
      int j;
      obj_data *o, *p;
      for (j = 1; j < NUM_WEARS; j++) {
	if (GET_EQ(ch, j)) {
	  remove_otrigger(GET_EQ(ch, j), ch);
	  extract_obj(GET_EQ(ch, j));
	}
      }
      /* Now, remove its inventory. */
      o = NULL;
      for (o = ch->carrying; o; o = p) {
	p = o->next_content;
	remove_otrigger(o, ch);
	extract_obj(o);
      }
      /* Now destroy a component mob's components' inventory. */
      struct follow_type *t, *f = ch->followers;
      while (f) {
	t = f->next;
	struct char_data *m = f->follower;
	if (IS_NPC(m) && MOB2_FLAGGED(m, MOB2_COMPONENT)) {
	  for (j = 1; j < NUM_WEARS; j++) {
	    if (GET_EQ(ch, j)) {
	      remove_otrigger(GET_EQ(m, j), ch);
	      extract_obj(GET_EQ(m, j));
	    }
	  }
	  /* Now, remove its inventory. */
	  o = NULL;
	  for (o = m->carrying; o; o = p) {
	    p = o->next_content;
	    remove_otrigger(o, ch);
	    extract_obj(o);
	  }
	}
	extract_char(m);
	f = t;
      }
    } else {
      char_to_room(ch, tmp);
      GET_POS(ch) = POS_STANDING;
      do_look(ch, "", 0, 0);
    }

      send_to_char(ch,CCRED(ch, C_SPR));
      send_to_char(ch,"You have been killed in battle!!\r\n");
      send_to_char(ch,CCNRM(ch, C_SPR));

    for (i = descriptor_list; i; i = i->next)
    {
      if(i->original)
	current = i->original;
      else
	current = i->character;
      if (STATE(i)==CON_PLAYING &&
	  !PRF2_FLAGGED(current, PRF_NOBATTLE) &&
	  !PLR_FLAGGED(current, PLR_WRITING))
      {
	if (ch == killer)
	  send_to_char(current, "\x1B[1;36m[ BATTLE ] %s has died in "
		       "the battlefield.\x1B[0m\n\r",
		       CAN_SEE(current, ch)?GET_NAME(ch):"Someone");
	else
	  send_to_char(current, "\x1B[1;36m[ BATTLE ] %s has been " 
		       "killed by %s in battle.\x1B[0m\n\r",
		       CAN_SEE(current, ch)?GET_NAME(ch):"Someone",
		       CAN_SEE(current, killer)?GET_NAME(killer):"Someone");
	
	if (!IS_NPC(ch) && TAGGED(ch))
	  send_to_char(current, "\x1B[1;36m[ BATTLE ] The battle field "
		       "has been scoured of would-be taggers!\x1B[0m\n\r");
      }
    }
    if (!IS_NPC(ch) && !IS_NPC(killer) && (GET_PKILLS(killer) < 30000) &&
	(ch != killer))
      GET_PKILLS(killer)+=1;
    
    GET_MANA(ch) = 10;
    GET_HIT(ch) = 10;
    GET_MOVE(ch) = 10;
    if (!IS_NPC(ch) && TAGGED(ch))
    {
      battle.tagged = FALSE;
      TAGGED(ch) = FALSE;
    }
    ch->char_specials.in_battle = FALSE;

    /* At this point, I believe it makes sense to call extract_char for mobs. */
    if (IS_NPC(ch)) {
      die_follower(ch);
      extract_char(ch);
    }
  }
}

void die(struct char_data *ch, struct char_data *killer)
   {
   int same=FALSE;

   if(ch==killer)
      same=TRUE;

   /* Every grouped member fighting a mob that dies gets his lag set to 1 second.  */
   /* First, find the leader of the group. */
   struct char_data *leader = killer->master ? killer->master : killer;
   if (leader) {
     /* The leader of the group killed somebody.  Set his lag to 1 second. */
     if (FIGHTING(leader) == ch) {
       GET_WAIT_STATE(leader) = 1 RL_SEC;
     }
     /* Now loop over all followers. */
     struct follow_type *f = leader->followers;
     while (f) {
       /* If you're a follower fighting the SAME mob and you're grouped... */
       if (FIGHTING(f->follower) == ch && AFF_FLAGGED(f->follower, AFF_GROUP)) {
	 /* Set his lag to 1 second. */
	 GET_WAIT_STATE((f->follower)) = 1 RL_SEC;
       }
       f = f->next;
     }     
   }

   if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_PKILL) /*&&!Z_FLAGGED(IN_ROOM(ch),Z_PKILL)*/ &&
        !(PLR_FLAGGED(ch, PLR_PK) && PLR_FLAGGED(killer, PLR_PK) && abs(GET_LEVEL(ch)-GET_LEVEL(killer))<=10))
      gain_exp(ch, -(GET_EXP_FOR_CH(ch) /3));
   /* if (!IS_NPC(ch))
      REMOVE_BIT(PLR_FLAGS(ch), PLR_KILLER | PLR_THIEF); 
      */
   if(killer->desc)
      {
      if(GET_WAIT_STATE(killer)>15)
         GET_WAIT_STATE(killer)=15;

      }
   raw_kill(ch,killer);
   if(same==FALSE)
      rage_check(killer);

   }



void perform_group_gain(struct char_data * ch, int base,
                        struct char_data * victim)
   {
   int share,max_gain;
   int exp_after_lim;

   if (REMORT_LEVEL(ch) == TRIPLE_REMORT)
   {
     min_kills = 20;
   }
   else
   {
     min_kills = 20;
   }

   if (GET_LEVEL(ch) < 16)
   {
     max_gain = GET_EXP_FOR_LEVEL(GET_RACE(ch), GET_CLASS(ch), 16, REMORT_LEVEL(ch));
     max_gain /= min_kills;
   }
   else
   {
     max_gain = GET_EXP_FOR_LEVEL(GET_RACE(ch), GET_CLASS(ch), GET_LEVEL(ch), REMORT_LEVEL(ch));
     max_gain /= min_kills;
   }

   /*
   if(GET_LEVEL(ch)<16)
      max_gain=(int)((float)exp_table[15]
                     *(float)class_exp_multipliers[(int)GET_CLASS(ch)]
                     *(float)race_exp_multipliers[(int)GET_RACE(ch)])
               /min_kills;
   else
      max_gain=((int)((float)exp_table[GET_LEVEL(ch)]
                      *(float)class_exp_multipliers[(int)GET_CLASS(ch)]
                      *(float)race_exp_multipliers[(int)GET_RACE(ch)])
                /min_kills);
   */
   base = base * GET_LEVEL(ch);

   if(!IS_NPC(ch)&&IS_NPC(victim))
      exp_after_lim = kills_limit_xpgain(ch,victim,base);
   else
      exp_after_lim = base;

   share = MIN(max_gain, MAX(1, exp_after_lim));

   if (share > 1)
      {
      send_to_char(ch, "You receive your share of experience -- %d points.\r\n",
                   share);
      }
   else
      send_to_char(ch,"You receive your share of experience -- one measly little point!\r\n");

   gain_exp(ch, share);
   change_alignment(ch, victim);
   }

void group_gain(struct char_data * ch, struct char_data * victim)
   {
   int tot_members=0, base=0,tot_levels=0;
   int top_level=0;
   struct char_data *k;
   struct follow_type *f;
   int killer_level;
   /* int level_bonus; */
   int check=0;
   int self_done=0;

   if(ROOM_FLAGGED(IN_ROOM(ch),ROOM_PKILL) /*||Z_FLAGGED(IN_ROOM(ch),Z_PKILL)*/)
      return;
   if (PLR_FLAGGED(ch, PLR_PK) && PLR_FLAGGED(victim, PLR_PK) && abs(GET_LEVEL(ch)-GET_LEVEL(victim))<=10)
      return;

   if ((ch->master)&&AFF_FLAGGED(ch,AFF_GROUP))
      k=ch->master;
   else
      k = ch;

   if (AFF_FLAGGED(k, AFF_GROUP) && (IN_ROOM(k) == IN_ROOM(ch)))
      {
      tot_members = 1;
      tot_levels=GET_LEVEL(k);
      top_level =GET_LEVEL(k);
      }
   /* I Can't see the reason for this hunk of code. */
   /*
     else if(!AFF_FLAGGED(k,AFF_GROUP))
     {
     tot_members = 1;
     tot_levels=GET_LEVEL(k);
     top_level =GET_LEVEL(k);
     }
     */
   else
      {
      tot_members = 1;
      tot_levels=GET_LEVEL(ch);
      top_level =GET_LEVEL(ch);
      self_done=1;
      }

   for (f = k->followers; f; f = f->next)
      if(AFF_FLAGGED(f->follower,AFF_GROUP)&&
              (IN_ROOM(f->follower)==IN_ROOM(ch)))
         {
         if((self_done==1)&&(f->follower==ch))
            continue;
         tot_members++;
         tot_levels+=GET_LEVEL(f->follower);
         if(GET_LEVEL(f->follower)>top_level)
            top_level =GET_LEVEL(f->follower);
         }

   if(tot_members == 0)
      killer_level=0;
   else
      killer_level=top_level-(2*(tot_members-1));
   if(killer_level<1)
      killer_level = 1;

   /* round up to the next highest tot_members */
   /*    base = (GET_EXP(victim) / 3) + tot_members - 1; */

   /*    log("mem : %d lev : %d killer: %d base: %d",tot_members,tot_levels, */
   /*      killer_level,base); */
   /*      log("vlev: %d vexp: %ld",GET_LEVEL(victim),GET_EXP(victim)); */

   if (tot_members > 1)
      {
      base=((exp_table[top_level]/1000)*(GET_LEVEL(victim)-killer_level))/(3*tot_levels);
      /*  log ("base: %d",base);  */
      /* base = base + MAX(1, GET_EXP(victim) / (int)((float)0.9 * (float)3 * (float)tot_levels)); */
      base = base + MAX(1, GET_EXP(victim) / (int)(fight_group_exp_divisor * (float)tot_levels));
      /*  log ("base: %d",base);  */
      }
   else if(tot_members==1)
      {
      base = ((exp_table[top_level]/1000) *(GET_LEVEL(victim) - killer_level))/(3*tot_levels);
      /* log ("base: %d",base); */
      base = base + MAX(1, GET_EXP(victim) / (3 * tot_levels));
      /* log ("base: %d",base); */
      }
   else if(tot_members==0)
      {
      base = 0;
      log("base = 0");
      }
   else
      {
      log("tot_members < 0???? fight.c group_exp");
      return;
      }

   check=0;
   if (IN_ROOM(k) == IN_ROOM(ch))
      {
      perform_group_gain(k, base, victim);
      if(k==ch)
         check=1;
      }

   if(AFF_FLAGGED(k,AFF_GROUP))
      for (f = k->followers; f; f = f->next)
         if (AFF_FLAGGED(f->follower, AFF_GROUP) &&
                 IN_ROOM(f->follower) == IN_ROOM(ch))
            {
            perform_group_gain(f->follower, base, victim);
            if(f->follower==ch)
               check=1;
            }

   if(check==0)
      perform_group_gain(ch,base,victim);




   /* int tot_members, base;
      struct char_data *k; 
      struct follow_type *f; 
      
      if (!(k = ch->master)) 
      k = ch; 
      
      if (AFF_FLAGGED(k, AFF_GROUP) && (IN_ROOM(k) == IN_ROOM(ch))) 
      tot_members = 1; 
      else 
      tot_members = 0; 
      
      for (f = k->followers; f; f = f->next) 
      if (AFF_FLAGGED(f->follower, AFF_GROUP) && IN_ROOM(f->follower) == IN_ROOM(ch)) 
      tot_members++; 
      
      base = (GET_EXP(victim) / 3) + tot_members - 1; 
      
      if (tot_members >= 1) 
      base = MAX(1, GET_EXP(victim) / (3 * tot_members)); 
      else 
      base = 0; 
      
      if (AFF_FLAGGED(k, AFF_GROUP) && IN_ROOM(k) == IN_ROOM(ch)) 
      perform_group_gain(k, base, victim); 
      
      for (f = k->followers; f; f = f->next) 
      if (AFF_FLAGGED(f->follower, AFF_GROUP) && IN_ROOM(f->follower) == IN_ROOM(ch)) 
      perform_group_gain(f->follower, base, victim); 
      */
   }



void replace_string(char *str, char *weapon_singular, char *weapon_plural,
                    char*buf)
   {
   char *cp;

   cp = buf;

   for (; *str; str++)
      {
      if (*str == '#')
         {
         switch (*(++str))
            {
         case 'W':
            for (; *weapon_plural; *(cp++) = *(weapon_plural++))
               ;
            break;
         case 'w':
            for (; *weapon_singular; *(cp++) = *(weapon_singular++))
               ;
            break;
         default:
            *(cp++) = '#';
            break;
            }
         }
      else
         *(cp++) = *str;

      *cp = 0;
      } /* For */
   }


/* message for doing damage with a weapon */
void dam_message(int dam, struct char_data * ch, struct char_data * victim,
                 int w_type)
   {
   char *buf=get_buffer(256);
   int percent,amount_index,weapon_index;


   w_type -= TYPE_HIT;  /* Change to base of table with text */
   percent = (dam * 100 / GET_MAX_HIT (victim));

   if      (percent <= 1)
      amount_index = 0;
   else if (percent <= 2)
      amount_index = 1;
   else if (percent <= 5)
      amount_index = 2;
   else if (percent <= 10)
      amount_index = 3;
   else if (percent <= 20)
      amount_index = 4;
   else if (percent <= 30)
      amount_index = 5;
   else if (percent <= 45)
      amount_index = 6;
   else if (percent <= 60)
      amount_index = 7;
   else if (percent <= 80)
      amount_index = 8;
   else
      amount_index = 9;

   if (dam == 0)
      weapon_index = 0;
   else if (dam <= 1)
      weapon_index = 1;
   else if (dam <= 5)
      weapon_index = 2;
   else if (dam <= 10)
      weapon_index = 3;
   else if (dam <= 15)
      weapon_index = 4;
   else if (dam <= 20)
      weapon_index = 5;
   else if (dam <= 30)
      weapon_index = 6;
   else if (dam <= 40)
      weapon_index = 7;
   else if (dam <= 50)
      weapon_index = 8;
   else
      weapon_index = 9;

   /* weapon damage message to onlookers */
   replace_string(dam_first[w_type][weapon_index].to_room,
                  attack_hit_text[w_type].singular,
                  attack_hit_text[w_type].plural, buf);
   sprintf(buf+strlen(buf),", %s",dam_second[w_type][amount_index].to_room);
   act(buf, FALSE, ch, NULL, victim, TO_NOTVICT|FR_FIGHT);

   /* weapon damage message to damager */
   send_to_char(ch,CCYEL(ch, C_NRM));
   replace_string(dam_first[w_type][weapon_index].to_char,
                  attack_hit_text[w_type].singular,
                  attack_hit_text[w_type].plural, buf);
   sprintf(buf+strlen(buf),", %s",dam_second[w_type][amount_index].to_char);
   act(buf, FALSE, ch, NULL, victim, TO_CHAR);
   send_to_char(ch,CCNRM(ch, C_NRM));

   /* weapon damage message to damagee */
   send_to_char(victim,CCRED(victim, C_NRM));
   replace_string(dam_first[w_type][weapon_index].to_victim,
                  attack_hit_text[w_type].singular,
                  attack_hit_text[w_type].plural, buf);
   sprintf(buf+strlen(buf),", %s",dam_second[w_type][amount_index].to_victim);
   act(buf, FALSE, ch, NULL, victim, TO_VICT | TO_SLEEP);
   send_to_char(victim,CCNRM(victim, C_NRM));

   release_buffer(buf);
   }

/* message for damage being absorbed by armor */
void dam_armor_message(struct char_data * ch, struct char_data * victim, int w_type)
   {
   w_type -= TYPE_HIT;

   act(armor_messages[w_type].to_room, FALSE, ch, NULL, victim, TO_NOTVICT|FR_FIGHT);
   send_to_char(victim,CCRED(victim, C_NRM));
   act(armor_messages[w_type].to_victim, FALSE, ch, NULL, victim, TO_VICT);
   send_to_char(victim,CCNRM(victim, C_NRM));
   send_to_char(ch,CCYEL(ch, C_NRM));
   act(armor_messages[w_type].to_char, FALSE, ch, NULL, victim, TO_CHAR);
   send_to_char(ch,CCNRM(ch, C_NRM));
   }

/*
 * message for doing damage with a spell or skill 
 *  C3.0: Also used for weapon damage on miss and death blows 
 */
int skill_message(int dam, struct char_data * ch, struct char_data * vict,
                  int attacktype)
   {
   int i, j, nr;
   struct message_type *msg;
   struct obj_data *weap;

   /* DUAL_WIELD FIX */
   if(LAST_HAND_USED(ch)==2)
      weap = GET_EQ(ch, WEAR_WIELD_2);
   else
      weap = GET_EQ(ch, WEAR_WIELD_1);

   for (i = 0; i < MAX_MESSAGES; i++)
      {
      if (fight_messages[i].a_type == attacktype)
         {
         nr = dice(1, fight_messages[i].number_of_attacks);
         for (j = 1, msg = fight_messages[i].msg; (j < nr) && msg; j++)
            msg = msg->next;

         if (!IS_NPC(vict) && (GET_LEVEL(vict) >= LVL_IMMORT) && !PRF2_FLAGGED(vict, PRF2_MORTAL))
            {
            send_to_char(ch,"&Y");
            act(msg->god_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
            send_to_char(ch,"&n");
            send_to_char(vict,"&R");
            act(msg->god_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT);
            send_to_char(vict,"&n");
            act(msg->god_msg.room_msg,FALSE,ch,weap,vict,TO_NOTVICT|FR_FIGHT);
            }
         else if (dam != 0)
            {
            if (GET_POS(vict) == POS_DEAD)
               {
               send_to_char(ch,CCYEL(ch, C_NRM));
               act(msg->die_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
               send_to_char(ch,CCNRM(ch, C_NRM));

               send_to_char(vict,CCRED(vict, C_NRM));
               act(msg->die_msg.victim_msg, FALSE, ch, weap, vict,
                   TO_VICT | TO_SLEEP);
               send_to_char(vict,CCNRM(vict, C_NRM));

               act(msg->die_msg.room_msg,FALSE,ch,weap,vict,
                   TO_NOTVICT|FR_FIGHT);
               }
            else
               {
               send_to_char(ch,CCYEL(ch, C_NRM));
               act(msg->hit_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
               send_to_char(ch,CCNRM(ch, C_NRM));

               send_to_char(vict,CCRED(vict, C_NRM));
               act(msg->hit_msg.victim_msg, FALSE, ch, weap, vict,
                   TO_VICT | TO_SLEEP);
               send_to_char(vict,CCNRM(vict, C_NRM));

               act(msg->hit_msg.room_msg, FALSE, ch, weap, vict,
                   TO_NOTVICT|FR_FIGHT);
               }
            }
         else if (ch != vict)
            {
            /* Dam == 0 */
            send_to_char(ch,CCYEL(ch, C_NRM));
            act(msg->miss_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
            send_to_char(ch,CCNRM(ch, C_NRM));

            send_to_char(vict,CCRED(vict, C_NRM));
            act(msg->miss_msg.victim_msg, FALSE, ch, weap, vict,
                TO_VICT | TO_SLEEP);
            send_to_char(vict,CCNRM(vict, C_NRM));

            act(msg->miss_msg.room_msg, FALSE, ch, weap, vict,
                TO_NOTVICT|FR_FIGHT);
            }
         return 1;
         }
      }
   return 0;
   }

int GetImmBit(long iWepType,long iImmType)
   {
   long iOurBit;
   if(iImmType!=0)
      {
      iOurBit=iImmType;
      }
   else
      {
      switch (iWepType)
         {
      case TYPE_PIERCE:
      case TYPE_PIERCE_NO_BS:
      case TYPE_BITE:
      case TYPE_STING:
      case TYPE_STAB:
         iOurBit = IMM_PIERCE;
         break;
      case TYPE_SLASH:
      case TYPE_WHIP:
      case TYPE_CLAW:
      case TYPE_THRASH:
      case TYPE_CLEAVE:
         iOurBit = IMM_SLASH;
         break;
      case TYPE_HIT:
      case TYPE_BLUDGEON:
      case TYPE_BLAST:
      case TYPE_POUND:
      case TYPE_MAUL:
      case TYPE_PUNCH:
         iOurBit = IMM_BLUNT;
         break;
      default:
         if(iWepType!=TYPE_SUFFERING)
            log("SYSERR:fight.c:GetImmBit(): Got %ld as a dam type without an imm bit",iWepType);
         return(0);
         break;
         }
      }
   return iOurBit;
   }


int Att_Imm(struct char_data *pVictim, long iOurBit, int iDam)
   {

   if(IS_SUCCEPT(pVictim,iOurBit))
      {
      iDam *=3;
      iDam /=2;
      }

   if(IS_IMMUNE(pVictim,iOurBit))
      iDam /=16;
   else if(IS_RESIST(pVictim,iOurBit))
      {
      iDam *=2;
      iDam /=3;
      }

   return(iDam);
   }


int Wep_Imm(struct char_data *ch,struct char_data *victim,int iWepType,
            int iDam,int *piImmBit)
   {

   int iTotal, j;
   int iTemp;
   iTotal=0;

   /* if it isn't a weapon, skip it */
   if((iWepType<TYPE_HIT)||(iWepType>TYPE_MAXWEP))
      return(iDam);

   if((iWepType==TYPE_HIT)&&!IS_NPC(ch))
      iTotal=GET_LEVEL(ch)/20;
   if(GET_EQ(ch,WEAR_WIELD_1))
      for(j=0;j<MAX_OBJ_AFFECT;j++)
         if(((GET_EQ(ch,WEAR_WIELD_1)->affected[j].location==APPLY_DAMROLL)||
                 (GET_EQ(ch,WEAR_WIELD_1)->affected[j].location==APPLY_HITROLL))&&
                 (GET_EQ(ch,WEAR_WIELD_1)->affected[j].modifier>iTotal))
            iTotal=GET_EQ(ch,WEAR_WIELD_1)->affected[j].modifier;
   if(GET_EQ(ch,WEAR_WIELD_2))
      for(j=0;j<MAX_OBJ_AFFECT;j++)
         if(((GET_EQ(ch,WEAR_WIELD_2)->affected[j].location==APPLY_DAMROLL)||
                 (GET_EQ(ch,WEAR_WIELD_2)->affected[j].location==APPLY_HITROLL))&&
                 (GET_EQ(ch,WEAR_WIELD_2)->affected[j].modifier>iTotal))
            iTotal=GET_EQ(ch,WEAR_WIELD_2)->affected[j].modifier;
   if(IS_NPC(ch))
      if((iTemp=GET_LEVEL(ch)/20)>iTotal)
         iTotal=iTemp;

   switch(iTotal)
      {
   case 0:
      if(IS_IMMUNE(victim,IMM_NONMAG))
         iDam/=16;
      else if(IS_RESIST(victim,IMM_NONMAG))
         iDam/=2;
      else if(IS_SUCCEPT(victim,IMM_NONMAG))
         iDam*=2;
      *piImmBit|=IMM_NONMAG;
      break;
   case 1:
      if(IS_IMMUNE(victim,IMM_PLUS1))
         iDam/=16;
      else if(IS_RESIST(victim,IMM_PLUS1))
         iDam/=2;
      else if(IS_SUCCEPT(victim,IMM_PLUS1))
         iDam*=2;
      *piImmBit|=IMM_PLUS1;
      break;
   case 2:
      if(IS_IMMUNE(victim,IMM_PLUS2))
         iDam/=16;
      else if(IS_RESIST(victim,IMM_PLUS2))
         iDam/=2;
      else if(IS_SUCCEPT(victim,IMM_PLUS2))
         iDam*=2;
      *piImmBit|=IMM_PLUS2;
      break;
   case 3:
      if(IS_IMMUNE(victim,IMM_PLUS3))
         iDam/=16;
      else if(IS_RESIST(victim,IMM_PLUS3))
         iDam/=2;
      else if(IS_SUCCEPT(victim,IMM_PLUS3))
         iDam*=2;
      *piImmBit|=IMM_PLUS3;
      break;
   case 4:
      if(IS_IMMUNE(victim,IMM_PLUS4))
         iDam/=16;
      else if(IS_RESIST(victim,IMM_PLUS4))
         iDam/=2;
      else if(IS_SUCCEPT(victim,IMM_PLUS4))
         iDam*=2;
      *piImmBit|=IMM_PLUS4;
      break;
   default:
      if(IS_SUCCEPT(victim,IMM_PLUS4))
         iDam*=2;
      *piImmBit|=IMM_PLUS4;
      break;
      }
   return iDam;
   }

int Obj_Imm_Dam(struct obj_data *tobj,int ImmBit)
   {
   /*   int iDam;
        int i;
      */

   return 1;
   }

/* Nomikos 5/4/2025 - Added ability to better avoid equipment damage. Previously was 4, now between 1 and 4 */
int damage_obj_chance(int dex, int wis)
   {
   float average, modifier;
	   
   /* Average Dexterity and Wisdom. They play equal parts in minimizing EQ damage */
   /* Keep it between 0 and 25 */
   average = MAX(MIN((float)(dex + wis) / 2.0, 25.0), 0.0);
	   
   /* Normalize and multiply by 3, giving us a range between 0 and 3 */
   modifier = (average / 25.0) * 3.0;

   /* At a maximum of 25 dex/wis, obtain a 4x reduction in equipment damage. */
   /* At a minimum of 0 dex/wis (w/affects), keep same as before, at 4 */
   return MAX(1, (int)(4.0 - modifier));
   }

int damage_obj_range(int base,int pos,int material,int ImmBit)
   {
   int range=0;
   if(IS_SET(material_affs[material].sucept_dam_vect,ImmBit))
      range = base * 3;
   else if(IS_SET(material_affs[material].resist_dam_vect,ImmBit))
      range = base * 7;
   else
      range = base * 5;

   return (int)((float)range/(float)wear_dam_adjust[pos]);
   }
/*
 * Alert: As of bpl14, this function returns the following codes:
 *     < 0     Victim died.
 *     = 0     No damage.
 *     > 0     How much damage done.
 */

int damage(struct char_data * ch, struct char_data * victim, int dam,
           int attacktype, int imm_type)
   {
   /* int exp;*/
   /* long local_gold = 0;*/ /* asl --Erika */
   /* char local_buf[256];*/ /* asl --Erika */
   bool missile=FALSE;
   struct obj_data * obj;
   int range,damage_chance;
   int damage_item=0;
   int flee_chance;
   int j;
   char *buf;
   int isnpc=0;
   int victlvl=0;
   int victbat=0;
   int ImmBit;

   static int dumped = 0;

   if (GET_POS(victim) <= POS_DEAD)
      {
      log("SYSERR: Attempt to damage corpse '%s' in room #%ld by '%s'"
          " %d %d %d %ld.",
          GET_NAME(victim),GET_ROOM_VNUM(IN_ROOM(victim)),GET_NAME(ch),
          dam,attacktype,imm_type,GET_ROOM_VNUM(IN_ROOM(ch)));
      if (!dumped) {
	core_dump();
	dumped = 1;
      }
      /*die(victim,ch);*/
      return 0;   /* -je, 7/7/92 */
      }
   if (IN_ROOM(ch) != IN_ROOM(victim))
      missile = TRUE;

   /* peaceful rooms - but imps can attack */
   if ((ch != victim && ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) &&
           (GET_LEVEL(ch) != LVL_IMPL) && (ch!=victim) &&
           (ch->nr!=real_mobile(DG_CASTER_PROXY)))
      {
      send_to_char(ch,"This room just has such a peaceful, easy feeling...\r\n");
      return 0;
      }

   /* shopkeeper protection */
   if (!ok_damage_shopkeeper(ch, victim))
   {
     if (FIGHTING(ch) == victim)
     {
       stop_fighting(ch);
       stop_fighting(victim);
     }
      return 0;
   }

   if((ch!=victim)&&!FIGHTING(ch)&&!FIGHTING(victim))
      {
      act("$N starts a fight with you!",FALSE,victim,0,ch,TO_CHAR);
      act("$N starts a fight with $n!",FALSE,victim,0,ch,TO_NOTVICT);
      }

   if (GET_NUM_GUARDING_ME(victim) > 0) {
     int i;
     for (i = 0; i < GET_NUM_GUARDING_ME(victim); i++) {
       struct char_data *guarder = GET_GUARDING_ME(victim)[i];
       if (IN_ROOM(guarder) != IN_ROOM(victim)) {
	 /* You can't guard if you're not in the same room. */
	 continue;
       } else if (FIGHTING(guarder)) {
	 /* You can't guard if you're already fighting.  It's too abusable. */
	 continue;
       } else if (GET_POS(guarder) <= POS_SLEEPING) {
	 /* You can't guard if you're sleeping, stunned, dead, etc. */
	 continue;
       }
       int penalty = IS_NPC(guarder) && IS_NPC(victim) ? -20 : 0;
       if (skill_roll(guarder, SKILL_GUARD, penalty)) {
	 act("You guard $N!", TRUE, guarder, 0, victim, TO_CHAR);
	 act("$n guards you!", TRUE, guarder, 0, victim, TO_VICT);
	 act("$n guards $N!", TRUE, guarder, 0, victim, TO_NOTVICT);
	 /* log("%s guarded %s from %s.", GET_NAME(guarder), GET_NAME(victim), GET_NAME(ch)); */
	 /* Change the victim here.  It's now the guy who was guarding. */
	 victim = guarder;
	 break;
       }
     }
   }   

   /* You can't damage an immortal! */
   if (!IS_NPC(victim) && (GET_LEVEL(victim) >= LVL_IMMORT) && !PRF2_FLAGGED(victim, PRF2_MORTAL))
      dam = 0;

   if (!pk_allowed)
      {
      check_killer(ch, victim);

      if (PLR_FLAGGED(ch, PLR_KILLER) && (ch != victim))
	dam /=10;/*1/10 damage*/
      }

   if ((victim != ch) && (!missile))
      {
      if ((GET_POS(ch) > POS_STUNNED) &&(!(FIGHTING(ch))))
         set_fighting(ch, victim);

      if (GET_POS(victim) > POS_STUNNED && !FIGHTING(victim))
         {
         set_fighting(victim, ch);
         if (MOB_FLAGGED(victim, MOB_MEMORY) && !IS_NPC(ch))
            remember(victim, ch);
         }
      }

   if (victim->master == ch)
      {
      if (RIDDEN_BY(victim) == victim->master)
         dismount_char(ch);
      stop_follower(victim);
      }

   if (AFF_FLAGGED(ch, AFF_INVISIBLE | AFF_HIDE))
      appear(ch);

   if (AFF_FLAGGED(victim, AFF_SANCTUARY))
      dam /=2;  /* 1/2 damage when sanctuary */

   if (PLR_FLAGGED(victim, PLR_FISHING) && dam >= 4)
      dam = ((float) dam * 1.5);

   if (GET_MOVE(ch)<((GET_MAX_MOVE(ch)*15)/100) && FIGHTING(ch))
      {
      dam = (dam * 2) / 3;/* low mv = 2/3's normal damage */
      if(number(1,2)==2)
         {
         send_to_char(ch,"You're exhausted!  You don't hit as hard.\r\n");
         /*   act("$n is subcumbing to exhaustion...",TRUE,ch,0,0,TO_ROOM); */
         }
      }

   if ((GET_HIT(ch)<(GET_MAX_HIT(ch)*15/100)) && (GET_HIT(ch)<101) && FIGHTING(ch))
      {
      dam = (dam * 5) / 4; /* low HPs = 10/9's damage */
      NEXT_HIT(ch)--;
      if(number(1,2)==2)
         {
         send_to_char(ch,"LOW HIT POINTS!!  You fight HARDER!\r\n");
         act("$n gets desperate and swings harder...",TRUE,ch,0,0,TO_ROOM);
         }
      }

   ImmBit=GetImmBit(attacktype,imm_type);
   dam = Att_Imm(victim,ImmBit,dam);


   dam = Wep_Imm(ch,victim,attacktype,dam,&ImmBit);


   if((attacktype!=SPELL_PLAGUE) &&
           (attacktype!=SPELL_POISON) &&
           (attacktype!=SPELL_SUNBURN) &&
           (attacktype!=SPELL_DROWN) &&
           (attacktype!=TYPE_SUFFERING) &&
           /* Don't damage EQ in PK rooms or zones */
           !(ROOM_FLAGGED(IN_ROOM(ch),ROOM_PKILL) || Z_FLAGGED(IN_ROOM(ch),Z_PKILL)) &&
           /* Enter the damage check if ch isn't a PK flagged, victim isn't a PK flagged,
              or they are more than 10 levels apart. */
           (!PLR_FLAGGED(ch, PLR_PK) || !PLR_FLAGGED(victim, PLR_PK) || abs(GET_LEVEL(ch)-GET_LEVEL(victim))>10) &&
           (!IS_NPC(victim) || (IS_NPC(victim) && AFF_FLAGGED(victim, AFF_CHARM))))   
      {
      buf=get_buffer(256);
      for (j = 1; j < NUM_WEARS; j++)
         {
         if (victim->equipment[j])
            {
            range = damage_obj_range(500,j,victim->equipment[j]->material,ImmBit);
            /* log("condition after : %d, %s ",condition,equipment_types[j]); */
            damage_chance = damage_obj_chance(GET_DEX(victim), GET_WIS(victim));
            /* log("damage chance : %d, in  %d (tslot: %d) ",damage_chance,condition,GET_OBJ_TSLOTS(victim->equipment[j])); */

            if (number(0, range) < damage_chance)
               {
               /* Tslots of 0 means INDESTRUCTABLE */
               if (GET_OBJ_TSLOTS(GET_EQ(victim,j)) != INDESTRUCTABLE)
                  {
                  if(OBJ_FLAGGED(victim->equipment[j],ITEM_RESISTANT))
                     GET_OBJ_CSLOTS(victim->equipment[j])-=2;
                  else if(OBJ_FLAGGED(victim->equipment[j],ITEM_BRITTLE))
                     GET_OBJ_CSLOTS(victim->equipment[j])-=8;
                  else
                     GET_OBJ_CSLOTS(victim->equipment[j])-=4;

                  send_to_char(victim,
                               "&M%s&n just got &RDAMAGED&n during the combat!\r\n",
                               victim->equipment[j]->short_description);
                  /*
                  log("damage done to item");*/
                  damage_item++;
                  }
               }

            if ((victim->equipment[j]->obj_flags.curr_dam_slots <= 0) &&
                    (GET_OBJ_TSLOTS(GET_EQ(victim,j)) != INDESTRUCTABLE))
               {
               obj=unequip_char(victim,j);
               /* Nomikos 5/2/2025: don't want to lose it
               ** obj_to_room(obj,IN_ROOM(victim)); */
               scrap_item(obj,victim);
               }
            }
         if(damage_item>=3)
            break;
         }
      release_buffer(buf);
      }
   dam = MAX(MIN(dam, max_damage), 0);
   /*    log("%-20.20s did %5d dam to %s",GET_NAME(ch),dam,GET_NAME(victim)); */
   GET_HIT(victim) -= dam;

   if((ch != victim) &&(dam>0) && (!(PLR_FLAGGED(ch, PLR_PK) && PLR_FLAGGED(victim, PLR_PK) &&
      abs(GET_LEVEL(ch)-GET_LEVEL(victim))<=10))) {
      int exp_after_lim = 0;
      int base = (GET_LEVEL(victim) * dam/2)+5;
      if(!IS_NPC(ch)&&IS_NPC(victim))
         exp_after_lim = kills_limit_damage_xpgain(ch,victim,base);
      else
         exp_after_lim = base;

      gain_exp(ch, exp_after_lim);
   }
   update_pos(victim);

   if(dam>0 && (GET_POS(victim)>POS_SITTING))
      {
      if(!CHECK_STUN(victim)&&((attacktype==SKILL_BASH)||
                               (attacktype==SKILL_SWEEP)||
                               (attacktype==SKILL_TRIP)))
         {
         /* if both are PCs, then give command lag instead - nomi*/
         if (!IS_NPC(ch) && !IS_NPC(victim))
            WAIT_STATE(victim, PULSE_VIOLENCE * 2);
         else
            STUN_STATE(victim, PULSE_VIOLENCE * 2);
         GET_POS(victim)=POS_SITTING;
         }

      }
   if(dam>0)
      {
      if(!CHECK_STUN(victim)&&((attacktype==SKILL_BACKSTAB)||
                               (attacktype==SKILL_AMBUSH)))
         {
         /* if both are PCs, then give command lag instead - nomi*/
         if (!IS_NPC(ch) && !IS_NPC(victim))
            WAIT_STATE(victim, PULSE_VIOLENCE);
         else
            STUN_STATE(victim, PULSE_VIOLENCE);
         }

      }
   /*
    * skill_message sends a message from the messages file in lib/misc. 
    * dam_message just sends a generic "You hit $n extremely hard.". 
    * skill_message is preferable to dam_message because it is more 
    * descriptive. 
    *  
    * If we are _not_ attacking with a weapon (i.e. a spell), always use 
    * skill_message. If we are attacking with a weapon: If this is a miss or a 
    * death blow, send a skill_message if one exists; if not, default to a 
    * dam_message. Otherwise, always send a dam_message. 
    */
   if (attacktype != -1)
      {
      if (!IS_WEAPON(attacktype))
         skill_message(dam, ch, victim, attacktype);
      else
         {
         if (GET_POS(victim) == POS_DEAD || dam == 0)
            {
            if (!damage_item || (dam > 0))
               {
               if (!skill_message(dam, ch, victim, attacktype))
                  dam_message(dam, ch, victim, attacktype);
               }
            else
               {
               dam_armor_message(ch, victim, attacktype);
               }
            }
         else
            {
            dam_message(dam, ch, victim, attacktype);
            }
         }
      }

   /* Use send_to_char -- act() doesn't send message if you are DEAD. */
   switch (GET_POS(victim))
      {
   case POS_MORTALLYW:
      act("$n is mortally wounded, and will die soon, if not aided.", TRUE,
          victim, 0, 0, TO_ROOM);
      send_to_char(victim,"You are mortally wounded, and will die soon, if not aided.\r\n");
      break;
   case POS_INCAP:
      act("$n is incapacitated and will slowly die, if not aided.", TRUE,
          victim, 0, 0, TO_ROOM);
      send_to_char(victim,"You are incapacitated and will slowly die, if not aided.\r\n");
      break;
   case POS_STUNNED:
      act("$n is stunned, but will probably regain consciousness again.",
          TRUE, victim, 0, 0, TO_ROOM);
      send_to_char(victim,"You're stunned, but will probably regain consciousness again.\r\n");
      break;
   case POS_DEAD:
      act("$n is dead!  R.I.P.", FALSE, victim, 0, 0, TO_ROOM);
      send_to_char(victim,"You are dead!  Sorry...\r\n");
      break;

   default:   /* >= POSITION SLEEPING */
      if (dam > (GET_MAX_HIT(victim) / 4))
         send_to_char(victim,"That really did HURT!\r\n");

      if (IS_NPC(victim)&&(GET_HIT(victim) < (GET_MAX_HIT(victim) / 4)))
         {
         send_to_char(victim, "%sYou wish that your wounds would stop BLEEDING so much!%s\r\n",
                      CCRED(victim, C_SPR), CCNRM(victim, C_SPR));
         flee_chance=trait_info[GET_RACE(victim)].morale;
         if(MOB_FLAGGED(victim, MOB_WIMPY))
            flee_chance+=20;
         if(MOB_FLAGGED(victim, MOB_FOOLHARDY))
            flee_chance-=5;
         if(MOB2_FLAGGED(victim, MOB2_COMPONENT))
            flee_chance=0;

         if ((ch != victim)&&(flee_chance>0))
            {
            int chance;
            chance=number(0,25);
            /*   log(" %-15.15s Flee chance: %d  roll: %d",GET_NAME(victim),flee_chance,chance); */
            if(chance<flee_chance)
               {
               do_flee(victim, NULL, 0, 0);
               rage_check(ch);
               clearMemory(ch);
               /* log("Mob Fled"); */
               }
            }
         }
      if (!IS_NPC(victim) && GET_WIMP_LEV(victim) && (victim != ch) &&
              GET_HIT(victim) < GET_WIMP_LEV(victim) && GET_HIT(victim)>0)
         {
         send_to_char(victim,"You wimp out, and attempt to flee!\r\n");
         do_flee(victim, NULL, 0, 0);
         rage_check(ch);
         }
      break;
      }

   /* Help out poor linkless people who are attacked */
   if (!IS_NPC(victim) && !(victim->desc))
      {
      do_flee(victim, NULL, 0, 0);
      rage_check(ch);
      if (!FIGHTING(victim))
         {
         act("$n is rescued by divine forces.", FALSE, victim, 0, 0, TO_ROOM);
         GET_WAS_IN(victim) = IN_ROOM(victim);
         char_from_room(victim);
         char_to_room(victim, 0);
         }
      }

   /* stop someone from fighting if they're stunned or worse */
   if ((GET_POS(victim) <= POS_STUNNED) && (FIGHTING(victim) != NULL))
      stop_fighting(victim);

   int getOutOfDeathFree = 0;

   /* Uh oh.  Victim died. */
   if (GET_POS(victim) == POS_DEAD)
      {
      if ((ch!=victim)&& (IS_NPC(victim) || victim->desc) )
         if (!IS_SET(world[IN_ROOM(ch)].room_flags, ROOM_PKILL) &&
                 !IS_SET(world[IN_ROOM(victim)].room_flags, ROOM_PKILL)&&!Z_FLAGGED(IN_ROOM(ch),Z_PKILL))
            {
            if(!missile)
               group_gain(ch, victim);
            }

      if (!IS_NPC(victim) && !ROOM_FLAGGED(IN_ROOM(victim),ROOM_PKILL)&&
              !Z_FLAGGED(IN_ROOM(victim),Z_PKILL))
         {

         switch (attacktype)
            {
            case SKILL_BREW:
               mudlogf(BRF, LVL_IMMORT, TRUE,"RIP: %s has died brewing: %s[%ld]",
                       GET_NAME(victim), world[IN_ROOM(victim)].name,
                       world[IN_ROOM(ch)].number);
               send_info("[ INFO ] %s has just been blown to pieces while "
                         "attempting to brew.\n\r", GET_NAME(victim));
               break;
            case SKILL_SCRIBE:
               mudlogf(BRF, LVL_IMMORT, TRUE,"RIP: %s has died scribing: %s[%ld]",
                       GET_NAME(victim), world[IN_ROOM(victim)].name,
                       world[IN_ROOM(ch)].number);
               send_info("[ INFO ] %s has died a fiery death in a failed "
                         "attempt at scribing.\n\r", GET_NAME(victim));
               break;
            case SPELL_SUNBURN:
               mudlogf(BRF, LVL_IMMORT, TRUE,"RIP: %s has died from sun damage: %s[%ld]",
                       GET_NAME(victim), world[IN_ROOM(victim)].name,
                       world[IN_ROOM(ch)].number);
               send_info("[ INFO ] %s has died from sun damage too great to "
                         "overcome.\n\r", GET_NAME(victim));
               break;
            case SPELL_POISON:
               mudlogf(BRF, LVL_IMMORT, TRUE,"RIP: %s has died from poison: %s[%ld]",
                       GET_NAME(victim), world[IN_ROOM(victim)].name,
                       world[IN_ROOM(ch)].number);
               send_info("[ INFO ] %s has died from systemic poisoning.\n\r", 
                         GET_NAME(victim));          
               break;
            case SPELL_DROWN:
               mudlogf(BRF, LVL_IMMORT, TRUE,"RIP: %s has died of drowning: %s[%ld]",    
                       GET_NAME(victim), world[IN_ROOM(victim)].name,
                       world[IN_ROOM(ch)].number);
               send_info("[ INFO ] %s has drowned!  Someone should teach that "
                         "%s how to swim.\n\r", GET_NAME(victim),
                         pc_race_types[GET_RACE(victim)]);
               break;
            case TYPE_SUFFERING:
               mudlogf(BRF, LVL_IMMORT, TRUE,"RIP: %s has died a lingering death(suffering): %s[%ld]",
                       GET_NAME(victim), world[IN_ROOM(victim)].name,
                       world[IN_ROOM(ch)].number);
               send_info("[ INFO ] %s has died from mortal wounds too great to "
                       "overcome.\n\r", GET_NAME(victim));
               break;
            default:
               if (ch == victim) /* killed yourself, ya fool! */
                  {
                  mudlogf(BRF, LVL_IMMORT, TRUE,"RIP: %s has died by self-inflicted wounds: %s[%ld]",
                          GET_NAME(victim), world[IN_ROOM(victim)].name,
                          world[IN_ROOM(ch)].number);
                  send_info("[ INFO ] %s has died of self-inflicted wounds.\n\r",
                            GET_NAME(victim));
                  }
               else /* component mobs */
                  {
                  if (!IS_NPC(ch) && (PLR_FLAGGED(ch,PLR_PK) && PLR_FLAGGED(victim,PLR_PK) &&
                            abs(GET_LEVEL(ch)-GET_LEVEL(victim))<=10))
                     {
                     send_info("[ INFO ] %s has been killed by %s.\n\r", GET_NAME(victim),
                            GET_NAME(MOB2_FLAGGED(ch,MOB2_COMPONENT)?ch->master:ch));
                     }
                  else
                     {
                     mudlogf(BRF, LVL_IMMORT, TRUE,"RIP: %s killed by %s at %s[%ld]",
                             GET_NAME(victim), 
                             GET_NAME(MOB2_FLAGGED(ch,MOB2_COMPONENT)?ch->master:ch),
                             world[IN_ROOM(victim)].name, world[IN_ROOM(ch)].number);
                     send_info("[ INFO ] %s has been killed by %s.\n\r", GET_NAME(victim),
                             GET_NAME(MOB2_FLAGGED(ch,MOB2_COMPONENT)?ch->master:ch));
                     }
                  }
               
            }
         if (MOB_FLAGGED(ch, MOB_MEMORY))
            forget(ch, victim);

	 if (GET_LEVEL(victim) < 16 && REMORT_LEVEL(victim) == 0) {
	   /* If the victim is under level 15, send them home. */
	   getOutOfDeathFree = 1;
	   mudlogf(BRF, LVL_IMMORT, TRUE, "RIP: %s was sent home because %s is under level 16.", GET_NAME(victim), GET_NAME(victim));
	   char_from_room(victim);
	   send_to_char(victim, "&MYou died, but because you are below level 16, your equipment came with you.\r\n");
	   send_to_char(victim, "At level 16 and above, you will need to walk back to your corpse to retrieve\r\n");
	   send_to_char(victim, "your equipment.\r\n&n");
	   GET_HIT(victim) = 10;
	   GET_MANA(victim) = 10;
	   GET_MOVE(victim) = 10;
	   GET_POS(victim) = POS_STANDING;
	   gain_exp(victim, -(GET_EXP_FOR_CH(victim)/5));
	   if (real_room(GET_HOME(victim)) >= 0) {
	     char_to_room(victim, real_room(GET_HOME(victim)));
	   } else {
	     char_to_room(victim, real_room(3014));
	   }
	   command_interpreter(victim, "look");
	 } else {
	   /* Check for get-out-of-death-free tokens. */
	   struct obj_data *i = has_object_ref(victim, 1294);
	   if (i) {
	     getOutOfDeathFree = 1;
	     send_info("[ INFO ] ...but was saved by a get-out-of-death-free token!\r\n");
	     mudlogf(BRF, LVL_IMMORT, TRUE, "RIP: %s used a get-out-of-death-free token.", GET_NAME(victim));
	     extract_obj(i);
	     char_from_room(victim);
	     GET_HIT(victim) = 10;
	     GET_POS(victim) = POS_STANDING;
	     if (real_room(GET_HOME(victim)) >= 0) {
	       char_to_room(victim, real_room(GET_HOME(victim)));
	     } else {
	       char_to_room(victim, real_room(3014));
	     }
	   }
	 }
      }
      if(IS_NPC(victim))
         isnpc=1;
      else
         isnpc=0;

      buf=get_buffer(256);

      victlvl=GET_LEVEL(victim);
      victbat=victim->char_specials.in_battle;
      
      /*if(IS_NPC(ch)&&IS_NPC(victim))
      **   mudlogf(CMP,LVL_SERP,TRUE,"MOOD: %s killed %s at [%d](NPC KILL)",
      **           GET_NAME(ch),GET_NAME(victim),GET_ROOM_VNUM(IN_ROOM(ch)));
      */
      if (!getOutOfDeathFree) {
	die(victim,ch);
      }

      /* Autosplit done in do_get -Anduin 7/3/97 */
      if (!IS_NPC(ch) && isnpc && !missile)
         {
         if(PRF_FLAGGED(ch, PRF_AUTOLOOT))
            do_get(ch, "all npccorpse",0,SCMD_LOOT);
         else
            {
            if(PRF_FLAGGED(ch, PRF_AUTOGOLD))
               do_get(ch, "all.coin npccorpse",0,SCMD_LOOT);
            }
         /*
          * begin add - Bon 07/18/97 
          * autosac code 
          */
         if (PRF_FLAGGED(ch, PRF_AUTOSAC))
            {
            do_sac(ch, "npccorpse",0,0);
            if (victbat == FALSE)
               {
               gain_exp(ch, victlvl * 2);
               send_to_char(ch, "You receive %d experience points as a "
                                "gift from the gods.\r\n", victlvl * 2);
               }
            }
         /*
          * end   add - Bon 07/18/97 
          */
         }
      release_buffer(buf);
      return -1;
      }
   return dam;
   }


/*
 * The logic that decides which prof goes with which weapon
 */
int get_prof(struct char_data *ch, struct obj_data *wielded)
   {
   int prof;
   if(!IS_NPC(ch)&&wielded)
      {
      switch(GET_OBJ_VAL(wielded,3)+TYPE_HIT)
         {
      case TYPE_BLUDGEON:
      case TYPE_MAUL:
         if(TWO_HANDED(wielded))
            prof=PROF_2H_CLUB;
         else
            prof=PROF_CLUB;
         break;

      case TYPE_PIERCE:
      case TYPE_PIERCE_NO_BS:
      case TYPE_STING:
         if(TWO_HANDED(wielded))
            prof=PROF_SPEAR;
         else
            prof=PROF_DAGGER;
         break;

      case TYPE_STAB:
         if(TWO_HANDED(wielded))
            prof=PROF_2H_SWORD;
         else
            prof=PROF_DAGGER;
         break;

      case TYPE_SLASH:
      case TYPE_BITE:
      case TYPE_THRASH:
         if(TWO_HANDED(wielded))
            prof=PROF_2H_SWORD;
         else
            prof=PROF_SWORD;
         break;

      case TYPE_BLAST:
      case TYPE_WHIP:
         prof=PROF_WHIP;
         break;

      case TYPE_CLAW:
         prof=PROF_CLAW;
         break;

      case TYPE_POUND:
         if(TWO_HANDED(wielded))
            prof=PROF_2H_HAMMER;
         else
            prof=PROF_HAMMER;
         break;

      case TYPE_CLEAVE:
         if(TWO_HANDED(wielded))
            prof=PROF_2H_AXE;
         else
            prof=PROF_AXE;
         break;

      case TYPE_HIT:
      case TYPE_PUNCH:
      default:
         prof=PROF_FISTICUFFS;
         break;
         }
      }
   else
      prof=PROF_FISTICUFFS;


   return prof;
   }

/*
 * Returns 0 if not dodged, otherwise it returns the number of the skill
 * that was used to escape combat
 */
int dodge_check(struct char_data * ch,struct char_data * vict)
   {
   int prob =0;
   int defense_type=0;
   int percent;

   if(!vict)
      return 0;
   if(IS_NPC(vict) && GET_LEVEL(vict) <= 15)
      return 0;
   if(GET_POS(vict)<POS_SITTING)
      return 0;
   /*
    * figure out which skill is used
    */
   if(GET_EQ(vict,WEAR_SHIELD) && SCR_SKILLCHECK(vict, SKILL_BLOCK) &&
           (IS_NPC(vict) || (GET_SKILL(vict,SKILL_BLOCK) > 0)))
      {
      if(IS_NPC(vict))
         prob = MAX(15,MIN(GET_LEVEL(vict),90));
      else
         prob = GET_SKILL(vict,SKILL_BLOCK);
      defense_type = SKILL_BLOCK;
      }
   else if(GET_EQ(vict,WEAR_WIELD_1) && SCR_SKILLCHECK(vict, SKILL_PARRY) &&
           (IS_NPC(vict) || (GET_SKILL(vict,SKILL_PARRY) > 0)))
      {
      if(IS_NPC(vict))
         prob = MAX(15,MIN(GET_LEVEL(vict),90));
      else
         prob = GET_SKILL(vict,SKILL_PARRY);
      defense_type=SKILL_PARRY;
      }
   else if(IS_NPC(vict) ||
           (!IS_NPC(vict) && GET_SKILL(vict,SKILL_DODGE) > 0 && SCR_SKILLCHECK(vict, SKILL_DODGE)))
      {
      if(IS_NPC(vict))
         prob = MAX(15,MIN(GET_LEVEL(vict),90));
      else
         prob = GET_SKILL(vict,SKILL_DODGE);
      defense_type = SKILL_DODGE;
      }
   if(defense_type == 0)
      return 0;
   percent = number(0,fight_dodge_random_bound);
   /*    log("%s dodges %s prob: %d percent: %d",GET_NAME(vict), */
   /*        GET_NAME(ch),prob,percent); */
   /* bonus for level diff */
   prob -= (GET_LEVEL(ch) - GET_LEVEL(vict))*fight_ldiff_dodge_multiplier;

   prob -= num_pcs_fighting(vict)*fight_grouped_dodge_multiplier;
   
   /*
    * so, did it work?
    */
   /*    log("------ prob: %d percent: %d",prob,percent); */
   if(prob < percent)
      {
      if(!IS_NPC(vict))
         improve_skill(vict,defense_type,AUTO_FAIL);
      return 0;
      }
   else if(defense_type == SKILL_BLOCK)
      {
      act("Your attack is blocked by $N's shield!", TRUE, ch, 0,
          vict, TO_CHAR);
      act("$n's attack glances off $N's shield!",
          TRUE,ch,0,vict,TO_NOTVICT|FR_FIGHT);
      act("$n's attack is blocked by your trusty shield!", TRUE, ch,
          0,vict, TO_VICT);
      }
   else if(defense_type == SKILL_PARRY)
      {
      act("Your attack is nimbly parried by $N!", TRUE, ch, 0,
          vict, TO_CHAR);
      act("$n's attack is nimbly parried by $N!", TRUE, ch, 0,
          vict, TO_NOTVICT|FR_FIGHT);
      act("$n is nimbly parried by you!", TRUE, ch, 0,
          vict, TO_VICT);
      }
   else if(defense_type == SKILL_DODGE)
      {
      act("$N dodges out of the way of your attack!", TRUE, ch, 0,
          vict, TO_CHAR);
      act("$N dodges $n's attack!", TRUE, ch, 0,
          vict, TO_NOTVICT|FR_FIGHT);
      act("You dodge out of the path of $n's attack!", TRUE, ch, 0,
          vict, TO_VICT);
      }

   /*
    * to stop the case of the target continuallly dodging,
    * and not attacking
    */
   if(!FIGHTING(vict))
      set_fighting(vict,ch);
   if(!IS_NPC(vict))
      improve_skill(vict,defense_type,AUTO_PASS);
   GET_DODGED(ch) += 1;

   return defense_type;
   }

/*
 * This function returns 1 if the mobe flees combat.
 */
int charmie_flee(struct char_data * ch)
   {
   struct char_data *tch;
   /*
    * Is my master here with me while I fight?
    */
   if (IN_ROOM(ch->master) != IN_ROOM(ch))
      {
      if(number(0,2)!=0)
         stop_follower(ch);
      tch=FIGHTING(ch);
      do_flee(ch, "", 0, 0);
      if(tch)
         rage_check(tch);
      return 1;
      }
   else
      {
      /* if they have un-flaggin morale */
      if(trait_info[GET_RACE(ch)].morale==0)
         return 0;
      /*
       * Am I getting the snot beaten out of me?
       */
      if (FIGHTING(ch))
         if ( ((int)(10*((float)GET_HIT(ch)/(float)GET_MAX_HIT(ch)))+1) - \
                 ((int)(10*((float)GET_HIT(FIGHTING(ch))/ \
                            (float)GET_MAX_HIT(FIGHTING(ch))))+1) \
                 < (!FIGHTING(ch->master) ? -4 : -5) )
            {
            act("$n's morale seems to waver.", FALSE, ch, 0, 0, TO_ROOM);

            /*
             * Am I still loyal enough to hang in here?
             */
            if (number(0,101) > ((75+REACT_ADJ_CHA(ch->master,ch))-trait_info[GET_RACE(ch)].morale*5 ))
               {
               if(number(0,2)!=0)
                  stop_follower(ch);
               tch=FIGHTING(ch);
               do_flee(ch, "", 0, 0);
               if(tch)
                  rage_check(tch);
               return 1;
               }
            }
      }
   return 0;
   }

/* Returns the HIGHEST level of the grouped bards in the SAME room. */
/*
int is_grouped_with_assisting_bard(struct char_data *ch)
{
  if (!AFF_FLAGGED(ch, AFF_GROUP) || !FIGHTING(ch)) {
    return 0;
  }

  struct char_data *leader = (ch->master ? ch->master : ch);
  int group_size = IN_ROOM(leader) == IN_ROOM(ch) && FIGHTING(leader) == FIGHTING(ch) ? 1 : 0;

  struct follow_type *f = NULL;
  for (f = leader->followers; f; f = f->next) {
    struct char_data *tch = f->follower;
    group_size += IN_ROOM(tch) == IN_ROOM(ch) && FIGHTING(tch) == FIGHTING(ch) ? 1 : 0;
  }
  if (group_size < 2) {
    return 0;
  }

  int num_bards = 0;
  for (f = leader->followers; f; f = f->next) {
    struct char_data *tch = f->follower;
    if (IS_BARD(tch) && IN_ROOM(tch) == IN_ROOM(ch) && FIGHTING(tch) == FIGHTING(ch)) {
      return 1;
    }
  }

  return 0;
}
*/

int penalize_large_groups = 1;

void hit(struct char_data * ch, struct char_data * victim, int type)
   {
   struct obj_data *wielded;
   int w_type, victim_ac, calc_thaco, dam=0, diceroll;
   char *buf=get_buffer(100);
   int prof,skl_lvl;

   if (!ok_damage_shopkeeper(ch, victim))
   {
     if (FIGHTING(ch) == victim)
     {
       stop_fighting(ch);
       stop_fighting(victim);
     }
     return;
   }

   fight_mtrigger(ch);
   if(AFF_FLAGGED(ch,AFF_HIDE))
      REMOVE_BIT(AFF_FLAGS(ch),AFF_HIDE);

   if(!IS_NPC(victim)&&!FIGHTING(victim)&&(GET_LEVEL(victim)>=LVL_IMMORT)&&
           (GET_LEVEL(victim)<LVL_ADMIN))
      mudlogf(CMP,MAX(LVL_DGOD,GET_INVIS_LEV(victim)),TRUE,
              "Fight: %s just got %sself into a fight with %s",
              GET_NAME(victim),(GET_SEX(victim)==SEX_MALE)?"him":"her",
              GET_NAME(ch));
   else if(!IS_NPC(ch)&&!FIGHTING(ch)&&(GET_LEVEL(ch)>=LVL_IMMORT)&&
           (GET_LEVEL(ch)<LVL_ADMIN))
      mudlogf(CMP,MAX(LVL_DGOD,GET_INVIS_LEV(ch)),TRUE,
              "Fight: %s just got %sself into a fight with %s",
              GET_NAME(ch),(GET_SEX(ch)==SEX_MALE)?"him":"her",
              GET_NAME(victim));

   NEXT_HIT(ch)=calculate_speed(ch);
   if (!pk_allowed)
      check_killer(ch, victim);

   /* DUAL_WIELD FIX */
   if(LAST_HAND_USED(ch)==2)
      wielded = GET_EQ(ch, WEAR_WIELD_2);
   else
      wielded = GET_EQ(ch, WEAR_WIELD_1);

   /* Do some sanity checking, in case someone flees, etc. */
   if (IN_ROOM(ch) != IN_ROOM(victim))
      {
      if (FIGHTING(ch) && FIGHTING(ch) == victim)
         stop_fighting(ch);
      release_buffer(buf);
      return;
      }

   mprog_hitprcnt_trigger(ch, FIGHTING(ch));
   mprog_fight_trigger(ch, FIGHTING(ch));

   if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON)
      w_type = GET_OBJ_VAL(wielded, 3) + TYPE_HIT;
   else
      {
      if (IS_NPC(ch) && (ch->mob_specials.attack_type != 0))
         w_type = ch->mob_specials.attack_type + TYPE_HIT;
      else
         w_type = TYPE_HIT;
      }

   /* Calculate the raw armor including magic armor.  Lower AC is better. */
   victim_ac=compute_armor_class(victim);

   calc_thaco = base_thaco((int) GET_CLASS(ch),victim_ac);
   /*    sprintf(buf, "%3d ",calc_thaco); */

   calc_thaco += str_app[STRENGTH_APPLY_INDEX(ch)].tohit;
   /*    sprintf(buf+strlen(buf), "s:%3d ",calc_thaco);  */

   calc_thaco += GET_HITROLL(ch);

   /*int num_bards = is_grouped_with_assisting_bard(ch);*/
   /*    sprintf(buf+strlen(buf), "h:%3d ",calc_thaco);   */

   /* Intelligence helps! */
   /*   calc_thaco += (int) ((GET_INT(ch) - 13)*2);
      sprintf(buf+strlen(buf), "i:%3d ",calc_thaco);  
      */
   /* So does wisdom */
   /*   calc_thaco += (int) ((GET_WIS(ch) - 13)*2);
      sprintf(buf+strlen(buf), "w:%3d ",calc_thaco);
      */

   if(IS_NPC(ch))
      {
      skl_lvl=GET_LEVEL(ch)/2;
      skl_lvl+=40;

      }
   else
      {
      prof=get_prof(ch,wielded);
      skl_lvl=GET_SKILL(ch,prof);
      }

   calc_thaco += (skl_lvl-65)/3;
   /*    sprintf(buf+strlen(buf), "p:%3d ",calc_thaco); */

   calc_thaco += (GET_LEVEL(ch)-GET_LEVEL(victim))*3;
   /*    sprintf(buf+strlen(buf), "l:%3d ",calc_thaco); */

   diceroll=number(0,101);
   /*    sprintf(buf+strlen(buf), "dice:%3d %-15.15s",diceroll,GET_NAME(ch)); */
   /*   if(port==4999)
         log(buf);*/
   /* decide whether this is a hit or a miss */
   if((type!=SKILL_BACKSTAB)&&(type!=SKILL_AMBUSH)&&(type!=SKILL_CIRCLE)&&
           dodge_check(ch,victim))
      {
      /* check if the victim has a hitprcnt trigger */
      hitprcnt_mtrigger(victim);
      if (victim != ch)
         {
         if ((GET_POS(ch) > POS_STUNNED) &&(!(FIGHTING(ch))))
            set_fighting(ch, victim);

         if (GET_POS(victim) > POS_STUNNED && !FIGHTING(victim))
            {
            set_fighting(victim, ch);
            if (MOB_FLAGGED(victim, MOB_MEMORY) && !IS_NPC(ch))
               remember(victim, ch);
            }
         }
      release_buffer(buf);
      return;
      }
   else if (((diceroll > 1) && AWAKE(victim)) &&
            ((diceroll == 101) || (diceroll>=calc_thaco)))
      {
      if(!IS_NPC(ch))
         improve_skill(ch,prof,PROF_FAIL);
      /* mounted attack - nomikos 10/18/02 */
      if (RIDING(ch) && (GET_SKILL(ch, SKILL_MOUNTED_ATTACK) > 0) && SCR_SKILLCHECK(ch, SKILL_MOUNTED_ATTACK))
         improve_skill(ch, SKILL_MOUNTED_ATTACK, PROF_FAIL);
      if (type == SKILL_BACKSTAB)
         dam = damage(ch, victim, 0, SKILL_BACKSTAB,IMM_PIERCE);
      else if (type == SKILL_AMBUSH)
         dam = damage(ch, victim, 0, SKILL_AMBUSH,IMM_PIERCE);
      else if (type == SKILL_CIRCLE)
         dam = damage(ch, victim, 0, SKILL_CIRCLE,IMM_PIERCE);
      else
         dam = damage(ch, victim, 0, w_type,0);
      }
   else
      {
      if(!IS_NPC(ch))
         improve_skill(ch,prof,PROF_PASS);
      /* mounted attack - nomikos 10/18/02 */
      if (RIDING(ch) && (GET_SKILL(ch, SKILL_MOUNTED_ATTACK) > 0) && SCR_SKILLCHECK(ch, SKILL_MOUNTED_ATTACK))
         improve_skill(ch, SKILL_MOUNTED_ATTACK, PROF_PASS);
      /* okay, we know the guy has been hit.  now calculate damage. */
      dam = str_app[STRENGTH_APPLY_INDEX(ch)].todam;
      dam += GET_DAMROLL(ch);
      /* Bad things happen if more then six gang up at once. -4 dam per over six */
      if (!IS_NPC(ch) && penalize_large_groups)
         dam -= MAX(0,num_fighting(victim) - 6) * 4;


      if (wielded && GET_OBJ_TYPE(wielded)==ITEM_WEAPON)
         {
         if(IS_NPC(ch))
            {
            if(((((float)GET_OBJ_VAL(wielded,2)+1)/2.0)*
                    (float)GET_OBJ_VAL(wielded,1))>
                    (((float)(ch->mob_specials.damnodice+1)/2.0)*
                     (float)ch->mob_specials.damsizedice))
               {
               dam += dice(GET_OBJ_VAL(wielded, 1), GET_OBJ_VAL(wielded, 2));
               }
            else
               {
               dam += dice(ch->mob_specials.damnodice,
                           ch->mob_specials.damsizedice);
               }
            }
         else
            {
            dam += dice(GET_OBJ_VAL(wielded, 1), GET_OBJ_VAL(wielded, 2));
            }
         }
      else
         {
         if(IS_MONK(ch))
            dam += (GET_LEVEL(ch)/4)+number(0,5);
         else if (IS_NPC(ch))
            {
            dam+=dice(ch->mob_specials.damnodice,ch->mob_specials.damsizedice);
            }
         else
            dam += number(1, 4); /* Max. 4 dam with bare hands */
         }

      /*
       * Include a damage multiplier if victim isn't ready to fight:
       *
       * Position sitting  1.33 x normal
       * Position resting  1.66 x normal
       * Position sleeping 2.00 x normal
       * Position stunned  2.33 x normal
       * Position incap    2.66 x normal
       * Position mortally 3.00 x normal
       *
       * Note, this is a hack because it depends on the particular
       * values of the POSITION_XXX constants.
       */

      if (GET_POS(victim) < POS_FIGHTING)
         dam *= 1 + (POS_FIGHTING - GET_POS(victim)) / 3;

      dam = MAX(1, dam);  /* at least 1 hp damage min per hit */

      if (type == SKILL_BACKSTAB)
         {
         dam *= backstab_mult(GET_LEVEL(ch));
         dam = damage(ch, victim, dam, SKILL_BACKSTAB,IMM_PIERCE);
         }
      else if (type == SKILL_CIRCLE)
         {
         dam *= (backstab_mult(GET_LEVEL(ch))/2);
         dam = damage(ch, victim, dam, SKILL_CIRCLE,IMM_PIERCE);
         }
      else if (type == SKILL_AMBUSH)
         {
         dam *= (backstab_mult(GET_LEVEL(ch))/2);
         dam = damage(ch, victim, dam, SKILL_AMBUSH,IMM_PIERCE);
         }
      else
         dam = damage(ch, victim, dam, w_type,0);
      }

   /* check if the victim has a hitprcnt trigger */
   if(dam>0)
      hitprcnt_mtrigger(victim);

   release_buffer(buf);
   }


/* control the fights going on.  Called two times per second from comm.c. */
void perform_violence(void)
   {
   struct char_data *ch;
   struct obj_data *wielded;
   int i;
   struct char_data *leader;

   for (ch = combat_list; ch; ch = next_combat_list)
      {
      next_combat_list = ch->next_fighting;

      /*
       * SANITY
       */
      if ((FIGHTING(ch) == NULL) || (IN_ROOM(ch) != IN_ROOM(FIGHTING(ch))))
         {
         stop_fighting(ch);
         rage_check(ch);
         continue;
         }

      /*
       * MOUNT CHECK
       */
      if (RIDING(ch))
         {
         if (IN_ROOM(ch) != IN_ROOM(RIDING(ch)))
            dismount_char(ch);
         }

      /*
       * SOME MOB STUFF
       */

      /*
       * this replaced with that below it
      if (IS_NPC(ch))
         {
         if (GET_MOB_WAIT(ch) > 0)
            {
            log("MOB_WAIT: %3d %s",GET_MOB_WAIT(ch),GET_NAME(ch));
            GET_MOB_WAIT(ch) -= 1;
            continue;
            }
         GET_MOB_WAIT(ch) = 0;
         if (GET_POS(ch) < POS_FIGHTING)
            {
            GET_POS(ch) = POS_FIGHTING;
            act("$n scrambles to $s feet!", TRUE, ch, 0, 0, TO_ROOM);
            }
         }
      */

      /* return if mobs.. they get to wait a little longer */
      if (GET_STUN_STATE(ch) > 0)
         {
         if (IS_NPC(ch))
            {
            GET_STUN_STATE(ch) -= 1;
            continue;
            }
         else
            {
            GET_STUN_STATE(ch) -= 5;
            /* check again for players- */
            /* this is because of the positions of */
            /* routines in comm.c */
            if (GET_STUN_STATE(ch) > 0)
               continue;
            }
         }

      GET_STUN_STATE(ch) = 0;
      if (GET_POS(ch) < POS_FIGHTING)
         {
         GET_POS(ch) = POS_FIGHTING;
         send_to_char(ch, "You scramble to your feet!\r\n");
         act("$n scrambles to $s feet!", TRUE, ch, 0, 0, TO_ROOM);
         }

      /*
       * IS IT THEIR TIME TO HIT? OR ARE THEY TOO BUSY?
       */
      if(IS_CASTING(ch))
         continue;
      if((NEXT_HIT(ch)--)>0)
         continue;
      if ((GET_POS(ch) < POS_FIGHTING) &&!IS_NPC(ch))
         {
         send_to_char(ch,"You can't fight while sitting!!\r\n");
         send_to_char(ch,"You scramble to your feet!!\r\n");
         GET_POS(ch)=POS_FIGHTING;
         NEXT_HIT(ch)=calculate_speed(ch);
         continue;
         }

      if (FIGHTING(ch) == NULL||!FIGHTING(ch))
         continue;

      if(GET_HIT(ch)<=0)
         {
         NEXT_HIT(ch)=calculate_speed(ch);
         continue;
         }

      hit(ch, FIGHTING(ch), TYPE_UNDEFINED);
      /*
       * moved to hit()
       *  if(!dodge_check(ch))
       *  else
       *  NEXT_HIT(ch)=calculate_speed(ch);
       */

      /* It is possible to die by through hit(), so do some
       * more sanity checking... this whole function needs to
       * be re-evaluated soon and re-written.  -Nomikos 7/6/03
       * Perhaps hit() needs a return value.
       */
      if (IN_ROOM(ch) == NOWHERE)
         {
         mudlogf(BRF, LVL_IMMORT, FALSE,
           "SYSERR: %s is dead after performing a hit() in perform_violence!",
           GET_NAME(ch));
         continue;
         }

      /* DUAL_WIELD FIX */
      if(LAST_HAND_USED(ch)==2)
         wielded = GET_EQ(ch, WEAR_WIELD_2);
      else
         wielded = GET_EQ(ch, WEAR_WIELD_1);

      /*
       * WEAPON SPELLS
       */
      if(wielded&&(number(1,5)==1))
         {
         if (wielded->spell_affect[0].spelltype > 0)
            {
            for (i = 0; i < MAX_SPELL_AFFECT; i++)
               {
               if(wielded->spell_affect[i].spelltype > 0)
                  {
                  /*
                   * People might want to check my dicerolls here.  Dont 
                   * know if I'm doing this right :( 
                   */
                  if (number(0,100) < (wielded->spell_affect[i].percentage))
                     {
                     act("Your $p hums violently in your hands!", FALSE, ch,
                         wielded, 0, TO_CHAR);
                     act("$p hums violently in the hands of $n!", FALSE, ch,
                         wielded, 0, TO_ROOM);
                     if(IS_SET(spells[wielded->spell_affect[i].spelltype].targets,TAR_NOT_SELF)||(spells[wielded->spell_affect[i].spelltype].violent==VIOLENT))
                        {
                        call_magic(ch, FIGHTING(ch), NULL, NULL,NULL,
                                   wielded->spell_affect[i].spelltype,
                                   wielded->spell_affect[i].level, CAST_SPELL);
                        }
                     else
                        {
                        call_magic(ch, ch, NULL, NULL,NULL,
                                   wielded->spell_affect[i].spelltype,
                                   wielded->spell_affect[i].level, CAST_SPELL);
                        }
                     }

                  }
               }
            }
         }

      /*
       * MOB SPECIAL FIGHT PROCS
       */
      if (MOB_FLAGGED(ch, MOB_SPEC) && mob_index[GET_MOB_RNUM(ch)].func!=NULL)
         {
         (mob_index[GET_MOB_RNUM(ch)].func) (ch, ch, 0, "");
         }
      /*
       * AUTO ASSIST CODE
       */
      if (FIGHTING(ch) && AFF_FLAGGED(ch, AFF_GROUP))
         {
         leader =ch;
         while(leader->master)
            leader = leader->master;
         /* should the leader join? */
         if ( (ch != leader) && (IN_ROOM(ch) == IN_ROOM(leader)) &&
                 (!FIGHTING(leader)) &&
                 ((!IS_NPC(leader)&& PRF_FLAGGED(leader,PRF_AUTOASSIST)) ||
                  (IS_NPC(leader) && MOB_FLAGGED(leader,MOB_GUARD))))
            {
            do_assist(leader, ch->player.name, 0, 0);
            }

         /* what about the rest of the group? */
         check_follower(leader,ch);
         }
      /*
       * Check to see if charmies flee or do something in rebellion
       */
      if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master)
         charmie_flee(ch);
      }
   }


int skill_roll(struct char_data *ch, int skill_num, int penalty)
   {
   int skl_lvl;

   if(IS_NPC(ch))
      skl_lvl = MIN(MAX(35,GET_LEVEL(ch)*2),85);
   else
      skl_lvl = GET_SKILL(ch,skill_num);

   if ((number(0, 101)+penalty) > skl_lvl)
      {
      improve_skill(ch,skill_num,USE_FAIL);
      return FALSE;
      }
   else
      {
      improve_skill(ch,skill_num,USE_PASS);
      return TRUE;
      }
   }

void mob_reaction(struct char_data *ch, struct char_data *vict, int dir)
   {
   char *buf=get_buffer(512);
   if (vict && IS_NPC(vict) && !FIGHTING(vict) && GET_POS(vict) > POS_STUNNED)
      {
      /* can remember so charge! */
      if (IS_SET(MOB_FLAGS(vict), MOB_MEMORY))
         {
         remember(vict, ch);
         sprintf(buf, "$n bellows in pain!");
         act(buf, FALSE, vict, 0, 0, TO_ROOM);
         if (GET_POS(vict) == POS_STANDING)
            {
            if (!do_simple_move(vict, rev_dir[dir], 1))
               act("$n stumbles while trying to run!",FALSE,vict,0,0,TO_ROOM);
            }
         else
            GET_POS(vict) = POS_STANDING;

         /* can't remember so try to run away */
         }
      else
         {
         do_flee(vict, "", 0, 0);
         }
      }
   release_buffer(buf);
   }


void strike_missile(struct char_data *ch, struct char_data *tch,
                    struct obj_data *missile, int dir, int attacktype)
   {
   int dam;
   char *buf=get_buffer(512);

   dam = str_app[STRENGTH_APPLY_INDEX(ch)].todam;
   dam += dice(missile->obj_flags.value[1],
               missile->obj_flags.value[2]);
   dam += GET_DAMROLL(ch);


   send_to_char(ch,"You hit!\r\n");
   sprintf(buf, "$P flies in from the %s and strikes %s.",
           dirs[rev_dir[dir]], GET_NAME(tch));
   act(buf, FALSE, tch, 0, missile, TO_ROOM);
   sprintf(buf, "$P flies in from the %s and hits YOU!",
           dirs[rev_dir[dir]]);
   act(buf, FALSE, tch, 0, missile, TO_CHAR);
   damage(ch, tch, dam, attacktype,IMM_PIERCE);
   if(tch&&GET_HIT(tch)>0)
      mob_reaction(ch, tch, dir);
   release_buffer(buf);
   return;
   }


void miss_missile(struct char_data *ch, struct char_data *tch,
                  struct obj_data *missile, int dir, int attacktype)
   {
   char *buf=get_buffer(512);
   sprintf(buf, "$P flies in from the %s and hits the ground!",
           dirs[rev_dir[dir]]);
   act(buf, FALSE, tch, 0, missile, TO_ROOM);
   act(buf, FALSE, tch, 0, missile, TO_CHAR);
   release_buffer(buf);
   send_to_char(ch,"You missed!\r\n");
   mob_reaction(ch, tch, dir);
   }


void fire_missile(struct char_data *ch, char arg1[MAX_INPUT_LENGTH],
                  struct obj_data *missile, int pos, int range, int dir)
   {
   bool shot = FALSE, found = FALSE;
   int attacktype;
   int room, nextroom, distance;
   struct char_data *vict;
   char *buf;

   if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
      {
      send_to_char(ch,"This room just has such a peaceful, easy feeling...\r\n");
      return;
      }

   room = IN_ROOM(ch);

   if CAN_GO2(room, dir)
      nextroom = EXIT2(room, dir)->to_room;
   else
      nextroom = NOWHERE;

   if (GET_OBJ_TYPE(missile) == ITEM_GRENADE)
      {
      buf=get_buffer(512);
      send_to_char(ch,"You throw it!\r\n");
      sprintf(buf, "$n throws %s %s.",
              missile->short_description, dirs[dir]);
      act(buf, FALSE, ch, 0, 0, TO_ROOM);
      send_to_room(nextroom, "%s flies in from the %s.\r\n",
                   missile->short_description, dirs[rev_dir[dir]]);
      obj_to_room(unequip_char(ch, pos), nextroom);
      release_buffer(buf);
      return;
      }

   for (distance = 1; ((nextroom != NOWHERE) && (distance<=range));distance++)
      {

      for (vict = world[nextroom].people; vict ; vict= vict->next_in_room)
         {
         if ((isname(arg1, GET_NAME(vict))) && (CAN_SEE(ch, vict)))
            {
            found = TRUE;
            break;
            }
         }

      if (found == 1)
         {

         /* Daniel Houghton's missile modification */
         if (missile && ROOM_FLAGGED(IN_ROOM(vict), ROOM_PEACEFUL))
            {
            send_to_char(ch,"Nah.  Leave them in peace.\r\n");
            return;
            }

         buf=get_buffer(512);
         switch(GET_OBJ_TYPE(missile))
            {
         case ITEM_THROW:
            send_to_char(ch,"You throw it!\r\n");
            sprintf(buf, "$n throws %s %s.", missile->short_description,
                    dirs[dir]);
            act(buf,TRUE,ch,0,0,TO_ROOM);
            attacktype = PROF_THROW;
            break;
         case ITEM_ARROW:
            act("$n aims and fires!", TRUE, ch, 0, 0, TO_ROOM);
            send_to_char(ch,"You aim and fire!\r\n");
            attacktype = PROF_BOW;
            break;
         case ITEM_ROCK:
            act("$n aims and fires!", TRUE, ch, 0, 0, TO_ROOM);
            send_to_char(ch,"You aim and fire!\r\n");
            attacktype = PROF_SLING;
            break;
         case ITEM_BOLT:
            act("$n aims and fires!", TRUE, ch, 0, 0, TO_ROOM);
            send_to_char(ch,"You aim and fire!\r\n");
            attacktype = PROF_CROSSBOW;
            break;
         default:
            attacktype = TYPE_UNDEFINED;
            break;
            }
         release_buffer(buf);
         if (attacktype != TYPE_UNDEFINED)
            shot = skill_roll(ch, attacktype,0);
         else
            shot = FALSE;

         if (shot == TRUE)
            {
            GET_OBJ_CSLOTS(GET_EQ(ch,pos))-=3;
            GET_OBJ_TSLOTS(GET_EQ(ch,pos))-=2;
            if(GET_OBJ_CSLOTS(GET_EQ(ch,pos))<1)
               extract_obj(unequip_char(ch, pos));
            else if ((number(0, 2)) || (attacktype == PROF_THROW))
               obj_to_char(unequip_char(ch, pos), vict);
            else
               extract_obj(unequip_char(ch, pos));

            strike_missile(ch, vict, missile, dir, attacktype);
            }
         else
            {
            /* ok missed so move missile into new room */
            GET_OBJ_CSLOTS(GET_EQ(ch,pos))-=6;
            GET_OBJ_TSLOTS(GET_EQ(ch,pos))-=4;
            if(GET_OBJ_CSLOTS(GET_EQ(ch,pos))<1)
               extract_obj(unequip_char(ch, pos));
            else if ((number(0, 2)) || (attacktype == PROF_THROW))
               obj_to_room(unequip_char(ch, pos), IN_ROOM(vict));
            else
               extract_obj(unequip_char(ch, pos));
            miss_missile(ch, vict, missile, dir, attacktype);
            }
         WAIT_STATE(ch, (PULSE_VIOLENCE/2));
         return;
         }
      room = nextroom;
      if CAN_GO2(room, dir)
         nextroom = EXIT2(room, dir)->to_room;
      else
         nextroom = NOWHERE;
      }

   send_to_char(ch,"Can't find your target!\r\n");
   return;

   }


void tick_grenade(void)
   {
   struct obj_data *i, *tobj;
   struct char_data *tch, *next_tch;
   int s, t, dam, door;
   /* grenades are activated by pulling the pin - ie, setting the
      one of the extra flag bits. After the pin is pulled the grenade
      starts counting down. once it reaches zero, it explodes. */

   for (i = object_list; i; i = i->next)
      {

      if (IS_SET(GET_OBJ_EXTRA(i), ITEM_LIVE_GRENADE))
         {
         /* update ticks */
         if (i->obj_flags.value[0] >0)
            i->obj_flags.value[0] -=1;
         else
            {
            t = 0;

            /* blow it up */
            /* checks to see if inside containers */
            /* to avoid possible infinite loop add a counter variable */
            s = 0; /* we'll jump out after 5 containers deep and just delete
                                the grenade */

            for (tobj = i; tobj; tobj = tobj->in_obj)
               {
               s++;
               if (IN_ROOM(tobj) != NOWHERE)
                  {
                  t = IN_ROOM(tobj);
                  break;
                  }
               else if ((tch = tobj->carried_by))
                  {
                  t = IN_ROOM(tch);
                  break;
                  }
               else if ((tch = tobj->worn_by))
                  {
                  t = IN_ROOM(tch);
                  break;
                  }
               if (s == 5)
                  break;
               }

            /* then truly this grenade is nowhere?!? */
            /* probably vwear, but I don't know how */
            if (t <= 0)
               {
               extract_obj(i);
               }
            else
               { /* ok we have a room to blow up */
               /* peaceful rooms */
               if (ROOM_FLAGGED(t, ROOM_PEACEFUL))
                  {
                  send_to_room(t,"You hear %s explode harmlessly, with a loud POP!\r\n", i->short_description);
                  extract_obj(i);
                  return;
                  }

               dam = dice(i->obj_flags.value[1], i->obj_flags.value[2]);

               send_to_room(t,"Oh no - %s explodes!  KABOOOOOOOOOM!!!\r\n",
                            i->short_description);


               for (door = 0; door < NUM_OF_DIRS; door++)
                  if (CAN_GO2(t, door))
                     send_to_room(world[t].dir_option[door]->to_room,
                                  "You hear a loud explosion!\r\n");

               for (tch = world[t].people; tch; tch = next_tch)
                  {
                  next_tch= tch->next_in_room;

                  if (GET_POS(tch) <= POS_DEAD)
                     {
                     log("SYSERR: Attempt to damage a corpse.");
                     return;                 /* -je, 7/7/92 */
                     }

                  /* You can't damage an immortal! */
                  if (IS_NPC(tch) || (GET_LEVEL(tch) < LVL_IMMORT))
                     {

                     GET_HIT(tch) -= dam;
                     act("$n is blasted!", TRUE, tch, 0, 0, TO_ROOM);
                     act("You are caught in the blast!",TRUE,tch,0,0,TO_CHAR);
                     update_pos(tch);

                     if (GET_POS(tch) <= POS_DEAD)
                        {
                        make_corpse(tch, NULL);
                        death_cry(tch);
                        extract_char(tch);
                        }
                     }

                  }
               /* ok hit all the people now get rid of the grenade and
               any container it might have been in */

               extract_obj(i);

               }
            } /* end else stmt that took care of explosions */
         } /* end if stmt that took care of live grenades */
      } /* end loop that searches the mud for objects. */

   return;

   }



