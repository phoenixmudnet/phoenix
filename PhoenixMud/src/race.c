/* ************************************************************************ 
*   File: race.c                                       Part of PhoenixMUD * 
*  Usage: Source file for race-specific code                              * 
*                                                                         * 
*  Added to CircleMUD 3.0 bpl 11 base for PhoenixMUD by Echo, 10/26/96    * 
*                                                                         * 
*  CircleMUD is:                                                          * 
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University * 
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               * 
************************************************************************ */ 
 
/* 
 * This file attempts to concentrate most of the code which must be changed 
 * in order for new races to be added.  If you're adding a new race, 
 * you should go through this entire file from beginning to end and add 
 * the appropriate new special cases for your new race. 
 */ 
 
#include "../localHeader/conf.h" 
#include "../localHeader/sysdep.h" 
 
#include "structs.h" 
#include "buffer.h" 
#include "utils.h" 
#include "comm.h"
#include "interpreter.h" 
#include "db.h" 
#include "handler.h"
#include "spells.h"
#include "utils.h"
extern struct spell_info_type *spells;
extern char   *immunity_names[];

long get_race_position(struct char_data *ch); 
/* 10/26/96, Echo - All of Phoenix's previous races were kept except for 
 *   Troll, Gnoll, Aquatic Elf, and Giant.  Grey Elf became Elf, 
 *   Ogre became Half Ogre, and Orc became Half Orc. 
 *     Halfling, Half Elf, and Avian were added. 
 *     Three other races, "Dragon Lord", "Vulcan", "Titan", and "Aesir" 
 *   were added to provide expansion or remort options. 
 */ 
 
/* Primarily for the purpose of display, as in "who".  Echo - 10/26/96 */ 
const char *race_abbrevs[] = 
{ 
   "Hum",			/* 0 */
   "Elf", 
   "H.E", 
   "D.E", 
   "Dwa", 
   "Hlf",			/* 5 */
   "Spr", 
   "Min", 
   "Avi", 
   "HOg", 
   "HOr",			/* 10 */
   "Drc", 
   "Shd", 
   "Ttn", 
   "Asr", 
   "Hmnd",			/* 15 */
   "UnDd",
   "Anml",
   "Gnt",
   "Plnt",
   "Fish",			/* 20 */
   "Inct",
   "Demn",
   "Drgn",
   "Eqne",
   "IAni",			/* 25 */
   "Stne",
   "Fire",
   "Watr",
   "Air",
   "Orc",			/* 30 */
   "Ogre",
   "Gbln",
   "Trll",
   "Gnme",
   "Eye",	        	/* 35 */
   "Rptl",
   "Slim",
   "Kbld",
   "Othr",
   "Ghst",			/* 40 */
   "IMat",
   "Angl",
   "Gnol",
   "\n"		
}
; 

char *npc_race_types[] = {
  "Human",
  "Elf",
  "Half-Elf",
  "Dark-Elf",
  "Dwarf",
  "Halfling",
  "Sprite",
  "Minotaur",
  "Avian",
  "Half-Ogre",
  "Half-Orc",
  "Draconian",
  "Shadow",
  "Titan",
  "Aesir",
  "Humanoid",
  "Undead",
  "Animal",
  "Giant",
  "Plant",
  "Fish",
  "Insect",
  "Demon",
  "Dragon",
  "Equine",
  "Inanimate",
  "Earth/Stone",
  "Fire",
  "Water",
  "Air",
  "Orc",
  "Ogre",
  "Goblin",
  "Troll",
  "Gnome",
  "Eye",
  "Reptile",
  "Slime/Mold",
  "Kobold",
  "Other",
  "Ghost",
  "Immaterial",
  "Angel",
  "Gnoll",
  "\n"
};
 
/* 10/26/96, Echo - for to parse argument with race selectable commands, 
 *           like "set" or "who".  Not yet implemented. 
 */ 
const char *race_keywords[] = 
{ 
   "human", 
   "elf", 
   "halfelf", 
   "darkelf", 
   "dwarf", 
   "halfling", 
   "sprite", 
   "minotaur", 
   "avian", 
   "halfogre", 
   "halforc", 
   "draconian", 
   "shadow", 
   "titan", 
   "aesir", 
   "\n" 
}
; 
 
/* 10/26/96, Echo - primarily for descriptive purposes, as in "score". */ 
const char *pc_race_types[] = 
{ 
   "Human", 
   "Elf", 
   "Half-Elf", 
   "Dark-Elf", 
   "Dwarf", 
   "Halfling", 
   "Sprite", 
   "Minotaur", 
   "Avian", 
   "Half-Ogre", 
   "Half-Orc", 
   "Draconian", 
   "Shadow", 
   "Titan", 
   "Aesir", 
   "\n" 
}; 
 
const struct racial_data racial_info[] = {
  /* baseage maxage basemv basemana maxmana hpbonus manabonus*/
   {      17,    85,    90,      80,    160,      1,   0},/* human*/
   {     130,   650,   100,     100,    200,     -1,   2},/* elf*/
   {      60,   300,    95,      80,    160,      0,   1},/* 1/2 elf*/
   {     110,   550,   100,      95,    190,      0,   1},/* dark elf*/
   {      75,   375,    75,      50,    100,      1,  -1},/* dwarf */
   {      20,   100,    75,      80,    160,      0,   0},/* 1/2ling*/
   {       7,    35,    65,      90,    180,     -1,   2},/* sprite */
   {      14,    70,    85,      60,    120,      1,  -1},/* minotaur */
   {      50,   250,   110,     110,    220,     -2,   3},/* avian */
   {      12,    60,    90,      60,    120,      2,  -2},/* half-ogre */
   {      13,    65,    95,      60,    120,      1,  -1},/* half-orc */
   {     100,  1000,   120,     150,    300,      1,   2},/* draconian */
   {     100,  1000,   120,      90,    180,      4,   1},/* shadow */
   {     100,  1000,   120,      90,    180,      2,   1},/* titan */
   {     100,  1000,   120,     150,    300,      1,   4} /* aesir */
};

const struct trait_data trait_info[] = {
  /* humanoid/length/level_size/talk/fly/swim/hands/heat/immat/!stun/infr/ultra/hitroll/sneak/tame/morale/suscept/resist/immune */
   { 1, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 2,
     0, 0, 0}, /* Human */
   { 1, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 2,
     IMM_SLASH, IMM_COLD|IMM_POISON|IMM_CHARM, 0}, /* Elf */
   { 1, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 2, 
     0, IMM_CHARM, 0}, /* HalfElf */
   { 1, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 2, 
     IMM_COLD, IMM_CHARM|IMM_POISON|IMM_ACID, 0}, /* D Elf */
   { 1, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 2, 
     IMM_COLD|IMM_ELEC, IMM_SLASH, 0}, /* Dwarf */
   { 1, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 2,
     IMM_COLD|IMM_ENERGY, IMM_POISON, 0}, /* Halfling */
   { 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 4, 
     IMM_SLASH, IMM_CHARM|IMM_SLEEP|IMM_COLD|IMM_ACID, 0}, /* Sprite */
   { 1, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 2,
     IMM_ACID, IMM_SLASH, 0}, /* Minotaur */
   { 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 4,
     IMM_BLUNT, IMM_POISON|IMM_DRAIN, 0}, /* Avian */
   { 1, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 2, 
     IMM_SLEEP|IMM_POISON, IMM_SLASH, 0}, /* HalfOgre */
   { 1, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 5, 
     IMM_FIRE|IMM_COLD, IMM_SLASH, 0}, /* HalfOrc */
   { 1, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 2, 
     IMM_COLD, IMM_FIRE|IMM_PIERCE|IMM_ENERGY|IMM_SLASH, 0}, /* Draconian */
   { 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 1, 0, 1, 0, 4,
     IMM_ENERGY|IMM_FIRE, IMM_SLASH|IMM_BLUNT|IMM_PIERCE|IMM_COLD|IMM_HOLD|IMM_NONMAG, 0}, /* Shadow */
   { 1, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 1,
     IMM_ENERGY|IMM_HOLD|IMM_ACID, IMM_PIERCE|IMM_SLASH|IMM_BLUNT, 0}, /* Titan */
   { 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 2, 
     IMM_SLASH|IMM_UNHOLY, IMM_FIRE|IMM_COLD|IMM_ELEC|IMM_ACID|IMM_POISON, IMM_HOLY}, /* Aesir */
   { 1, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 6, 
     0, 0, 0}, /* Humanoid */
   { 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0,
     IMM_FIRE|IMM_HOLY, IMM_COLD|IMM_ENERGY|IMM_UNHOLY, IMM_POISON|IMM_ELEC|IMM_NONMAG}, /* Undead */
   { 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 7,
     IMM_POISON, IMM_BLUNT, 0}, /* Animal */
   { 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 2,
     IMM_FIRE|IMM_COLD|IMM_ELEC|IMM_ENERGY|IMM_ACID|IMM_POISON|IMM_HOLD|IMM_SLEEP|IMM_CHARM, IMM_PIERCE|IMM_SLASH|IMM_BLUNT, 0}, /* Giant */
   { 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 
     IMM_ACID|IMM_SLASH, IMM_PIERCE|IMM_BLUNT, 0}, /* Plant */
   { 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 2,
     IMM_FIRE, IMM_COLD, 0}, /* Fish */
   { 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0,
     IMM_BLUNT, IMM_SLASH|IMM_PIERCE, 0}, /* Insect */
   { 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 7,
     IMM_HOLY, IMM_SLASH|IMM_BLUNT|IMM_PIERCE|IMM_ENERGY|IMM_COLD, IMM_FIRE|IMM_POISON|IMM_UNHOLY},/* Demon */
   { 0, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 0, 0, 1, 
     IMM_COLD, IMM_ELEC|IMM_ENERGY|IMM_ACID|IMM_POISON|IMM_HOLD|IMM_SLEEP|IMM_CHARM|IMM_PIERCE|IMM_SLASH|IMM_BLUNT, IMM_FIRE}, /* Dragon */
   { 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 4, 
     0, 0, 0}, /* Equine */
   { 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 
     IMM_ACID|IMM_BLUNT, IMM_SLASH|IMM_PIERCE, IMM_POISON}, /* Inanmiate */
   { 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 
     IMM_ACID|IMM_SLASH, IMM_BLUNT|IMM_PIERCE, IMM_POISON}, /* Earth/Stone*/
   { 0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 
     IMM_COLD, 0, IMM_FIRE}, /* Fire */
   { 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 
     IMM_FIRE, 0, IMM_COLD}, /* Water */
   { 0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 
     IMM_POISON, IMM_ACID|IMM_BLUNT, IMM_PIERCE|IMM_BLUNT|IMM_COLD|IMM_FIRE}, /* Air */
   { 1, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 6,
     IMM_FIRE, IMM_COLD, 0}, /* Orc */
   { 1, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 6, 
     IMM_FIRE|IMM_POISON|IMM_SLEEP, IMM_COLD|IMM_SLASH, 0}, /* Ogre */
   { 1, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 6,
     IMM_ACID|IMM_POISON|IMM_SLASH, IMM_HOLD|IMM_STUN|IMM_BLUNT, 0}, /*Goblin*/
   { 1, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 2, 
     IMM_FIRE|IMM_ACID, IMM_COLD|IMM_SLASH|IMM_PIERCE, 0}, /* Troll */
   { 1, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 6, 
     IMM_BLUNT, IMM_SLEEP|IMM_HOLD|IMM_CHARM|IMM_STUN, 0}, /* Gnome */
   { 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 2, 
     IMM_FIRE|IMM_PIERCE, IMM_COLD|IMM_HOLD|IMM_CHARM|IMM_STUN|IMM_SLEEP,
     0}, /* Eye */
   { 0, 1, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 2, 
     IMM_FIRE, IMM_COLD|IMM_SLASH|IMM_PIERCE|IMM_BLUNT, 0}, /* Reptile */
   { 0, 1, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 
     IMM_FIRE|IMM_BLUNT, IMM_SLASH|IMM_PIERCE|IMM_COLD, 
     IMM_POISON|IMM_ACID}, /* Slime/Mold */
   { 1, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 6, 
     IMM_SLASH|IMM_PIERCE, IMM_COLD|IMM_BLUNT, 0}, /* Kobold */
   { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0}, /* Other */
   { 1, 0, 0, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 
     IMM_HOLD, IMM_SLASH|IMM_PIERCE|IMM_BLUNT,
     IMM_NONMAG|IMM_POISON|IMM_ACID}, /* Ghost */
   { 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 2, 
     IMM_HOLD, IMM_SLASH|IMM_PIERCE|IMM_BLUNT,
     IMM_NONMAG|IMM_POISON|IMM_ACID},  /* Immaterial */
   { 1, 0, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 
     IMM_UNHOLY, IMM_SLASH|IMM_PIERCE|IMM_BLUNT|IMM_FIRE|IMM_ENERGY,
     IMM_POISON|IMM_POISON|IMM_COLD},  /* Angel */
   { 1, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 1, 0, 10, 
     IMM_SLASH, 0, 0} /* Gnoll */
};


const struct race_size_data race_size_info[] = {
/* Male Hght in   Male Wght lbs Fmle Hght in Fmle Wght lbs  */
/*  min avg max  min  avg   max min avg max  min  avg    max*/
   { 62, 71, 80, 124, 186,  267, 58, 67, 75, 106, 160,   229}, /* Human */
   { 56, 60, 65,  74,  91,  116, 53, 56, 61,  63,  78,    99}, /* Elf */
   { 62, 67, 72, 112, 141,  175, 58, 63, 68,  96, 121,   150}, /* Half Elf */
   { 56, 60, 65,  74,  91,  116, 53, 56, 61,  63,  78,    99}, /* Dark Elf */
   { 44, 48, 53, 162, 210,  283, 41, 45, 50, 138, 180,   242}, /* Dwarf */
   { 39, 46, 53,  57,  72,   83, 37, 43, 50,  49,  62,    71}, /* Halfling */
   { 32, 40, 46,  51,  66,   77, 30, 38, 44,  46,  58,    67}, /* Sprite */
   { 80, 85, 90, 213, 256,  304, 83, 89, 94, 247, 296,   352}, /* Minotaur */
   { 64, 67, 70, 110, 127,  145, 64, 67, 70, 110, 120,   145}, /* Avian */
   { 74, 84, 94, 424, 645,  813, 78, 89, 99, 522, 778,   989}, /* Half Ogre */
   { 60, 66, 70, 124, 165,  197, 60, 66, 70, 124, 165,   197}, /* Half Orc */
   { 72, 76, 80, 200, 250,  300, 72, 76, 80, 175, 200,   225}, /* Draconian */
   { 62, 67, 72, 112, 141,  175, 62, 67, 72,  72, 100,   128}, /* Shadow */
   { 84, 96,108, 250, 300,  350, 82, 91,100, 225, 268,   300}, /* Titan */
   { 62, 71, 80, 124, 172,  220, 62, 71, 80, 100, 125,   150}, /* Aesir */
   { 60, 69, 78, 112, 171,  247, 60, 69, 78, 112, 171,   247}, /* Humanoid */
   { 62, 71, 80, 130, 196,  280, 58, 67, 75, 111, 168,   240}, /* Undead */
   {  2, 10, 55,   0,   1,  265,  2, 11, 57,   0,   2,   307}, /* Animal */
   {108,180,300, 875,4051,18758,102,170,284, 750,3473, 16083}, /* Giant */
   {  2, 10, 55,   0,   5,  700,  2, 10, 55,   0,   5,   700}, /* Plant */
   {  4, 37,360,   0,  28,24353,  4, 41,396,   0,  37, 32414}, /* Fish */
   {  1, 13,180,   0,   0, 2309,  1, 14,198,   0,   1,  3073}, /* Insect */
   { 24, 75,240,  10, 342,10823, 24, 75,240,  10, 342, 10823}, /* Demon */
   { 30,189,1200, 17,4529,1145969,33,208,1320,23,6029,1525285},/* Dragon */
   { 60, 81,112, 365, 931, 2376, 56, 77,106, 313, 798,  2037}, /* Equine */
   { 36, 92,240, 301,5187,89296, 36, 92,240, 301,5187, 89296}, /* Inanimate */
   { 36, 92,240, 301,5187,89296, 36, 92,240, 301,5187, 89296}, /* Earth/Stone*/
   { 36, 92,240,   0,   0,    0, 36, 92,240,   0,   0,     0}, /* Fire */
   { 36, 92,240,  36, 633,10910, 36, 92,240,  36, 633, 10910}, /* Water */
   { 36, 92,240,   0,   0,    0, 36, 92,240,   0,   0,     0}, /* Air */
   { 56, 58, 60, 101, 112,  124, 56, 58, 60, 101, 112,   124}, /* Orc */
   { 94,104,114, 624, 845, 1113, 98,109,119, 722, 978,  1289}, /* Ogre */
   { 52, 55, 57,  73,  86,   96, 54, 57, 59,  84, 100,   111}, /* Goblin */
   { 80, 85, 90, 213, 256,  304, 83, 89, 94, 247, 296,   352}, /* Troll */
   { 39, 42, 44,  57,  72,   83, 37, 39, 41,  49,  62,    71}, /* Gnome */
   { 14, 70, 96,  57,7122,18370, 14, 70, 96,  57,7122, 18370}, /* Eye */
   {  6, 77,1000,  0, 143,308294, 6, 81,1049,  0, 165,356889}, /* Reptile */
   { 36, 74,156,  54, 494, 4458, 36, 74,156,  54, 494,  4458}, /* Slime/Mold */
   { 40, 44, 48,  40,  53,   70, 41, 46, 50,  46,  62,    81}, /* Kobold */
   {  0,  0,  0,   0,   0,    0,  0,  0,  0,   0,   0,     0}, /* Other */
   { 62, 71, 80,   0,   0,    0, 58, 67, 75,   0,   0,     0}, /* Ghost */
   { 62, 71, 80,   0,   0,    0, 58, 67, 75,   0,   0,     0}, /* Immaterial */
   { 25, 75,250,  10, 350,11000, 25, 75,250,  10, 350, 11000}, /* Angel */
   { 80, 84, 88, 140, 150,  160, 76, 80, 84, 130, 140,   150}  /* Gnoll */
};

struct race_stat_data race_stat_info[] = {
  /* STR  DEX  CON  INT  WIS  CHA */
   { 100, 100, 100, 100, 100, 100}, /* Human */
   {  80, 100,  90, 120, 120, 110}, /* Elf */
   {  90, 100, 100, 110, 110, 100}, /* HalfElf */
   {  90, 110,  90, 120, 110,  80}, /* D Elf */
   { 110,  90, 120,  90, 100,  90}, /* Dwarf */
   {  90, 120, 100,  90, 100, 110}, /* Halfling */
   {  80, 120,  80, 120, 120,  90}, /* Sprite */
   { 120, 100, 120, 100,  90,  90}, /* Minotaur */
   {  90, 110,  90, 120, 120,  80}, /* Avian */
   { 120,  90, 120,  80,  80,  90}, /* HalfOgre */
   { 110, 100, 110,  90,  90,  80}, /* HalfOrc */
   { 100, 100, 100, 140, 140, 110}, /* Draconian */
   { 110, 140, 110, 100, 100, 110}, /* Shadow */
   { 140, 100, 130, 100, 100, 110}, /* Titan */
   { 100, 100, 100, 140, 140, 110}, /* Aesir */
   { 100, 100, 100, 100, 100, 100}, /* Humanoid */
   { 110,  70, 120,  70,  80,  50}, /* Undead */
   { 120, 110, 120,   5,  10, 100}, /* Animal */
   { 350,  60, 150,  80, 110, 110}, /* Giant */
   { 170,   5, 350,   1,  10, 100}, /* Plant */
   {  25, 130, 100,   5,  10, 100}, /* Fish */
   { 200, 150, 100,   5,  10, 100}, /* Insect */
   { 220, 150, 190, 170, 130, 150}, /* Demon */
   { 450, 110, 190, 165, 150, 100}, /* Dragon */
   { 250, 120, 100,  10,  10, 100}, /* Equine */
   { 200,  65, 150,  60,  50, 100}, /* Inanmiate */
   { 250,  65, 150,  90,  80, 100}, /* Earth/Stone */
   {  65, 250, 130,  90,  80, 100}, /* Fire */
   { 150, 100, 140,  90,  80, 100}, /* Water */
   {  65, 500, 130,  90,  80, 100}, /* Air */
   {  80, 100, 100,  90,  90,  80}, /* Orc */
   { 210,  75, 150,  60,  60,  50}, /* Ogre */
   {  70, 110,  90,  85,  90,  80}, /* Goblin */
   { 135, 115, 180,  65,  70,  50}, /* Troll */
   {  75, 120, 100, 150,  90,  80}, /* Gnome */
   {  70,  50, 120, 120, 120, 100}, /* Eye */
   { 145, 120, 120,   5,  10, 100}, /* Reptile */
   {  25,  20, 150,  10,  10, 100}, /* Slime/Mold */
   {  65, 115,  95,  85,  85,  80}, /* Kobold */
   { 100, 100, 100, 100, 100, 100}, /* Other */
   {  15, 300,  60, 100, 100, 100}, /* Ghost */
   {  25, 300,  60, 120, 120,  30}, /* Immaterial */
   { 250, 130, 150, 150, 180, 250}, /* Angel*/
   { 110, 125, 100,  50,  75,  25}  /* Gnoll */

};




/* 10/26/96 Echo - note that as some of Phoenix's current races have been 
 *   preserved for backwards compatibility, they are not displayed on the 
 *   following menu. 
 */ 
char *race_menu = 
"\r\n" 
"The following races are available:\r\n" 
"  A. Human     - Normal in all abilities.\r\n" 
"  B. Elf       - Highly intelligent, but not very strong.\r\n" 
"  C. Half-Elf  - Stronger than elves, not as intelligent.\r\n" 
"  D. Dark-Elf  - Harmed by sunlight. NOT FOR BEGINNERS.\r\n" 
"  E. Dwarf     - Stout and strong, but not very intelligent.\r\n" 
"  F. Halfling  - Agile and quick, but not very strong.\r\n" 
"  G. Sprite    - Excellent intelligence, little strength.\r\n" 
"  H. Minotaur  - Strong, fairly intelligent, less agile.\r\n" 
"  I. Avian     - Agile, Intelligent, little strength.\r\n" 
"  J. Half-Ogre - Strong and dumb brutes.\r\n" 
"  K. Half-Orc  - Stronger than human, not as intelligent.\r\n"
"  \r\n"
"  ?. Help for races - usage: ? <race>\r\n";
 
char *race_prompt = 
"\r\nPlease select a race or '? <race>' for help: ";
 
/* 10/26/96, Echo - For intial race selection. 
 *           Cases should correspond with above menu. 
 */ 
int parse_race_for_menu(char arg) 
{ 
   arg = LOWER(arg); 
   
   switch (arg) 
      { 
       case 'a': return RACE_HUMAN; 
       case 'b': return RACE_ELF; 
       case 'c': return RACE_H_ELF; 
       case 'd': return RACE_D_ELF; 
       case 'e': return RACE_DWARF; 
       case 'f': return RACE_HALFLING; 
       case 'g': return RACE_SPRITE; 
       case 'h': return RACE_MINOTAUR; 
       case 'i': return RACE_AVIAN; 
       case 'j': return RACE_H_OGRE; 
       case 'k': return RACE_H_ORC; 
       default : return RACE_UNDEFINED; 
      } 
} 
 


/* 10/27/96, Echo - for race selectable commands like 'who' or 'set'. 
 *   The plan is to replace this l8r with a parser which will match 
 *   against intuitive keywords - see const char *race_keywords[] above - 
 *   to provide for the selection, rather than a single letter which 
 *   quickly becomes meaningless with any number of classes. 
 */ 
int parse_race(char *arg) 
{ 
   int i=0;

   for(i=0;*(pc_race_types[i]) != '\n';i++)
      {
      if(is_abbrev(arg,pc_race_types[i]))
	 return i;
      }
   return RACE_UNDEFINED;
/*   switch (arg) 
      { 
       case 'a': return RACE_HUMAN; 
       case 'b': return RACE_ELF; 
       case 'c': return RACE_H_ELF; 
       case 'd': return RACE_D_ELF; 
       case 'e': return RACE_DWARF; 
       case 'f': return RACE_HALFLING; 
       case 'g': return RACE_SPRITE; 
       case 'h': return RACE_MINOTAUR; 
       case 'i': return RACE_AVIAN; 
       case 'j': return RACE_H_OGRE; 
       case 'k': return RACE_H_ORC; 
       case 'l': return RACE_DRACONIAN; 
       case 'm': return RACE_SHADOW; 
       case 'n': return RACE_TITAN; 
       case 'o': return RACE_AESIR; 
       default : return RACE_UNDEFINED; 
      } 
      */
} 
 
 
/*** stat selection Anduin ***/ 
 
/* The following are race minimums for stat menu */ 
const long race_stats[NUM_RACES][6] = 
{ /* S I W D C Ch */ 
   { 6,6,6,6,6,10 } , /* Human min stats*/ 
   { 4,8,8,6,5,11 } , /* Elf min stats */ 
   { 5,7,7,6,6,10 } , /* Half Elf min stats */ 
   { 5,8,7,7,5,8  } , /* Dark Elf min stata */ 
   { 7,5,6,5,8,9  } , /* Dwarf min stata */ 
   { 5,5,6,9,6,11 } , /* Halfling min stata */ 
   { 4,9,8,8,4,9  } , /* Sprite min stata */ 
   { 8,6,5,6,8,9  } , /* Minotaur min stata */ 
   { 5,8,9,7,5,8  } , /* Avian min stata */ 
   { 9,4,4,5,9,9  } , /* Half-Ogre min stata */ 
   { 7,5,5,8,7,8  } , /* Half-Orc min stata */ 
   { 6,6,6,6,6,6  } , /* Drac min stats*/ 
   { 6,6,6,6,6,6  } , /* Shadow min stats*/ 
   { 6,6,6,6,6,6  } , /* Titan min stats*/ 
   { 6,6,6,6,6,6  }   /* Aesir min stats*/ 

} ; 
 
/* The following are race maximums for stat menu */ 
const long race_max_stats[NUM_RACES][6] = 
{ /* S  I  W  D  C  Ch*/ 
   { 18,18,18,18,18,18 } ,  /* Human max stats*/ 
   { 16,19,19,19,17,19 } ,  /* Elf max stats */ 
   { 18,19,19,18,18,18 } ,  /* Half Elf max stats */ 
   { 17,19,18,18,17,16 } ,  /* Dark Elf max stats */ 
   { 19,17,18,17,19,17 } ,  /* Dwarf max stats */ 
   { 17,17,18,18,18,19 } ,  /* Halfling max stats */ 
   { 15,19,19,19,17,17 } ,  /* Sprite max stats */ 
   { 19,18,17,18,19,17 } ,  /* Minotaur max stats */ 
   { 16,19,19,19,16,16 } ,  /* Avian max stats */ 
   { 20,16,16,17,19,17 } ,  /* Half-Ogre max stats */ 
   { 19,17,17,18,19,16 } ,  /* Half-Orc max stats */ 
   { 20,20,20,20,20,20 } ,  /* Drac min stats*/ 
   { 20,20,20,20,20,20 } ,  /* Shadow min stats*/ 
   { 20,20,20,20,20,20 } ,  /* Titan min stats*/ 
   { 20,20,20,20,20,20 }    /* Aesir min stats*/ 

} ; 
 
char *stat_menu = 
"\033[H\033[J" 
"\r\n" 
"Stat Selection:\r\n" 
"                 MIN        Current      Max      Cost \r\n" 
"                 ---        -------      ---      ---- \r\n" 
"(S)trength:                                        2 points \r\n" 
"(I)ntellig:                                        2 points  \r\n" 
"(W)isdom  :                                        2 points \r\n" 
"(D)exterit:                                        2 points \r\n" 
"(C)onstitu:                                        3 points \r\n" 
"C(h)arisma:                                        1 points \r\n\r\n" 
"You have    points to spend.\r\n" 
"To increase a statistic type the capital letter in ()\r\n" 
"to decrease it type a lower case letter.\r\n" 
"When you are finished type F <return>.\r\n\r\n"; 
 
long get_race_position(struct char_data *ch) 
{ 
   long j; 
   j=-1; 
   if (GET_RACE(ch) == RACE_HUMAN) 
      j = 0; 
   if (GET_RACE(ch) == RACE_ELF) 
      j = 1; 
   if (GET_RACE(ch) == RACE_H_ELF) 
      j = 2; 
   if (GET_RACE(ch) == RACE_D_ELF) 
      j = 3; 
   if (GET_RACE(ch) == RACE_DWARF) 
      j = 4; 
   if (GET_RACE(ch) == RACE_HALFLING) 
      j = 5; 
   if (GET_RACE(ch) == RACE_SPRITE) 
      j = 6; 
   if (GET_RACE(ch) == RACE_MINOTAUR) 
      j = 7; 
   if (GET_RACE(ch) == RACE_AVIAN) 
      j = 8; 
   if (GET_RACE(ch) == RACE_H_OGRE) 
      j = 9; 
   if (GET_RACE(ch) == RACE_H_ORC) 
      j = 10; 
    
   return j; 
} 
 
int parse_stats(char arg, struct char_data *ch, long race) 
{ 
   long j; 
  
   j = get_race_position(ch); 
   switch (arg) 
      { 
       case 's': 
	  if (ch->real_abils.str > race_stats[j][0]) 
	     { 
	     ch->real_abils.str--; 
	     ch->player_specials->saved.point+=2; 
	     } 
	  return 1; 
	  break; 
       case 'S': 
	  if ((ch->real_abils.str < race_max_stats[j][0]) && (ch->player_specials->saved.point >1)) 
	     { 
	     ch->real_abils.str++;       
	     ch->player_specials->saved.point-=2; 
	     } 
	  return 1; 
	  break; 
       case 'i': 
	  if (ch->real_abils.intel > race_stats[j][1]) 
	     { 
	     ch->real_abils.intel--; 
	     ch->player_specials->saved.point++; 
	     ch->player_specials->saved.point++; 
	     } 
	  return 2; 
	  break; 
       case 'I': 
	  if ((ch->real_abils.intel < race_max_stats[j][1]) && (ch->player_specials->saved.point>1)) 
	     { 
	     ch->real_abils.intel++;        
	     ch->player_specials->saved.point--; 
	     ch->player_specials->saved.point--; 
	     } 
	  return 2; 
	  break; 
       case 'w': 
	  if (ch->real_abils.wis > race_stats[j][2]) 
	     { 
	     ch->real_abils.wis--; 
	     ch->player_specials->saved.point++; 
	     ch->player_specials->saved.point++; 
	     } 
	  return 3; 
	  break; 
       case 'W': 
	  if ((ch->real_abils.wis < race_max_stats[j][2]) && (ch->player_specials->saved.point >1)) 
	     { 
	     ch->real_abils.wis++; 
	     ch->player_specials->saved.point--; 
	     ch->player_specials->saved.point--; 
	     }    
	  return 3; 
	  break; 
       case 'd': 
	  if (ch->real_abils.dex > race_stats[j][3]) 
	     { 
	     ch->real_abils.dex--; 
	     ch->player_specials->saved.point+=2; 
	     } 
	  return 4; 
	  break; 
       case 'D': 
	  if ((ch->real_abils.dex < race_max_stats[j][3]) && (ch->player_specials->saved.point >1)) 
	     { 
	     ch->real_abils.dex++; 
	     ch->player_specials->saved.point-=2; 
	     } 
	  return 4; 
	  break; 
       case 'c':    
	  if (ch->real_abils.con > race_stats[j][4]) 
	     { 
	     ch->real_abils.con--; 
	     ch->player_specials->saved.point++; 
	     ch->player_specials->saved.point++; 
	     ch->player_specials->saved.point++; 
	     } 
	  return 5; 
	  break; 
       case 'C': 
	  if ((ch->real_abils.con < race_max_stats[j][4]) && (ch->player_specials->saved.point >2)) 
	     { 
	     ch->real_abils.con++;       
	     ch->player_specials->saved.point--; 
	     ch->player_specials->saved.point--; 
	     ch->player_specials->saved.point--; 
	     } 
	  return 5; 
	  break; 
       case 'h': 
	  if (ch->real_abils.cha > race_stats[j][5]) 
	     { 
	     ch->real_abils.cha--; 
	     ch->player_specials->saved.point++; 
	     } 
	  return 6; 
	  break; 
       case 'H': 
	  if ((ch->real_abils.cha < race_max_stats[j][5]) && (ch->player_specials->saved.point >0)) 
	     { 
	     ch->real_abils.cha++; 
	     ch->player_specials->saved.point--; 
	     } 
	  return 6;       
	  break; 
       case 'f': 
       case 'F': 
	  ch->player_specials->saved.point=0; 
	  return -1; 
	  break; 
       default: 
	  return 0; 
	  break; 
      } 
/* end case */ 
}     
/* end function */ 
 
/*** End stat selection ***/ 

int invalid_race(struct char_data *ch, struct obj_data *obj)
{
  if (REMORT_LEVEL(ch) == TRIPLE_REMORT)
  {
    return 0;
  }
   if (	(OBJ_ANTI_FLAGGED(obj,ITEM_ANTI_HUM)  && IS_HUMAN(ch)) ||
	(OBJ_ANTI_FLAGGED(obj,ITEM_ANTI_ELF)  && IS_ELF(ch)) ||
	(OBJ_ANTI_FLAGGED(obj,ITEM_ANTI_HLF)  && IS_H_ELF(ch)) ||
	(OBJ_ANTI_FLAGGED(obj,ITEM_ANTI_DLF)  && IS_D_ELF(ch)) ||
	(OBJ_ANTI_FLAGGED(obj,ITEM_ANTI_DWF)  && IS_DWARF(ch)) ||
	(OBJ_ANTI_FLAGGED(obj,ITEM_ANTI_HFNG) && IS_HALFLING(ch)) ||
	(OBJ_ANTI_FLAGGED(obj,ITEM_ANTI_SPR)  && IS_SPRITE(ch)) ||
	(OBJ_ANTI_FLAGGED(obj,ITEM_ANTI_MIN)  && IS_MINOTAUR(ch)) ||
	(OBJ_ANTI_FLAGGED(obj,ITEM_ANTI_AVI)  && IS_AVIAN(ch)) ||
	(OBJ_ANTI_FLAGGED(obj,ITEM_ANTI_HOG)  && IS_H_OGRE(ch)) ||
	(OBJ_ANTI_FLAGGED(obj,ITEM_ANTI_HOR)  && IS_H_ORC(ch)) ||
	(OBJ_ANTI_FLAGGED(obj,ITEM_ANTI_DRC)  && IS_DRAC(ch)) ||
	(OBJ_ANTI_FLAGGED(obj,ITEM_ANTI_SHD)  && IS_SHADOW(ch)) ||
	(OBJ_ANTI_FLAGGED(obj,ITEM_ANTI_TTN)  && IS_TITAN(ch)) ||
	(OBJ_ANTI_FLAGGED(obj,ITEM_ANTI_ASR)  && IS_AESIR(ch)))
      return 1;
   else
      return 0;
}


const float race_exp_multipliers[] =
{
   1.0,				/* 0 */
   1.0,
   1.0,
   1.0,
   1.0,
   1.0,				/* 5 */
   1.0,
   1.0,
   1.0,
   1.0,
   1.0,				/* 10 */
   2.0,
   2.0,
   2.0,
   2.0,
   1.0,				/* 15 */
   1.0,
   1.0,
   1.0,
   1.0,
   1.0,				/* 20 */
   1.0,
   1.0,
   1.0,
   1.0,
   1.0,				/* 25 */
   1.0,
   1.0,
   1.0,
   1.0,
   1.0,				/* 30 */
   1.0,
   1.0,
   1.0,
   1.0,
   1.0, 			/* 35 */
   1.0,
   1.0,
   1.0,
   1.0,
   1.0,				/* 40 */
   1.0,
   1.0,
   1.0	
};
 


struct race_skills_struct race_skills[] = {
   {RACE_HALFLING,  SKILL_SNEAK,      75},
   {RACE_H_ORC,     SKILL_KICK,       10},
   {RACE_H_OGRE,    SKILL_BERSERK,    20},
   {RACE_SPRITE,    SPELL_PIXIE_DUST, 10},
   {RACE_D_ELF,     SPELL_LEVITATE,   15},
   {RACE_D_ELF,     SPELL_FAERIE_FIRE,15},
   {RACE_D_ELF,     SKILL_DARKEN,     25},
   {RACE_DRACONIAN, SPELL_LEVITATE,    1},
   {RACE_DRACONIAN, SPELL_FIRE_BREATH,20},
   {RACE_DWARF,     SKILL_DIG,         1},
   {RACE_MINOTAUR,  SKILL_GORE,        1},
   {RACE_MINOTAUR,  SKILL_BERSERK,     5},
   {RACE_SHADOW,    SPELL_INVISIBLE,  20},
   {RACE_SHADOW,    SPELL_SENSE_LIFE, 30},
   {RACE_SHADOW,    SKILL_SHADOW,     35},
   {RACE_AESIR,     SKILL_LAY_HANDS,  10},
   {RACE_AESIR,     SPELL_CURE_PLAGUE,30},
   {RACE_AESIR,     SPELL_FEAR,       45},
   {RACE_TITAN,     SPELL_STONE_SKIN, 10},
   {RACE_TITAN,     SPELL_STRENGTH,   25},
   {RACE_TITAN,     SPELL_ENHANCED,   50},
   {RACE_AVIAN,     SKILL_PECK,        1},
   {-1,-1,-1}
};


int find_race_skill(int race,int spell_num)
{
   int i;

   for(i=0;race_skills[i].race!=RACE_UNDEFINED;i++)
      {
      if(race!=race_skills[i].race)
	 continue;
      if(spell_num!=race_skills[i].spell_num)
	 continue;
      return race_skills[i].level;
      
      }

   return LVL_IMPL+1;
}


ACMD(do_race_skillhelp)
{
   int i=0;
   char *buf = get_buffer(MAX_STRING_LENGTH);

   strcpy(buf,"Race        |Skill/Spell/Song    |Level\r\n");
   strcat(buf,"------------|--------------------|-----\r\n");
   while(race_skills[i].race!=-1)
      {
      sprintf(buf+strlen(buf),"%-12.12s|%-20.20s|%d\r\n",
	      npc_race_types[race_skills[i].race],
	      spells[race_skills[i].spell_num].spell_name,
	      race_skills[i].level);
      i++;
      }
   if(ch->desc)
      page_string(ch->desc, buf, TRUE,"");
   release_buffer(buf);
}

void set_racial_traits(struct char_data *ch)
{
   if(trait_info[GET_RACE(ch)].auto_sneak)
      SET_BIT(AFF_FLAGS(ch), AFF_SNEAK);
   if(trait_info[GET_RACE(ch)].infravision)
      SET_BIT(AFF_FLAGS(ch), AFF_INFRAVISION); 
   if(trait_info[GET_RACE(ch)].can_fly)
      SET_BIT(AFF_FLAGS(ch), AFF_FLY);
   if(trait_info[GET_RACE(ch)].can_swim)
      SET_BIT(AFF_FLAGS(ch), AFF_WATER_BREATHE);
   if(GET_RACE(ch)==RACE_SPRITE)
      SET_BIT(AFF_FLAGS(ch),AFF_DETECT_INVIS);
   if(!IS_NPC(ch))
      {
      SET_BIT(IMMUNE(ch),trait_info[GET_RACE(ch)].immune);
      SET_BIT(RESIST(ch),trait_info[GET_RACE(ch)].resist);
      SET_BIT(SUCCEPT(ch),trait_info[GET_RACE(ch)].susceptible);
      }
}

/* This is called when entering the game, to remove incorrectly set  **
** bits due to race changes, etc..  Nomikos 7/25/05                  **
** NOTE:  It should really only be called when ch enters the game.   */
void reset_racial_traits(struct char_data *ch)
{
   int i;

   if (IS_NPC(ch))
      return;

   /* Make sure stats like str/int/wis/etc. aren't above racial maximums. */
   /* If they are, syslog them. */
   int diff;
   if ((diff = ch->real_abils.str - race_max_stats[GET_RACE(ch)][0]) > 0 && GET_LEVEL(ch) < LVL_IMMORT) {
     mudlogf(BRF, LVL_IMMORT, TRUE, "SYSERR: Reset %s STR from %d to %d.", GET_NAME(ch), (int)ch->real_abils.str, (int)race_max_stats[GET_RACE(ch)][0]);
     ch->real_abils.str -= diff;
   }
   if ((diff = ch->real_abils.intel - race_max_stats[GET_RACE(ch)][1]) > 0 && GET_LEVEL(ch) < LVL_IMMORT) {
     mudlogf(BRF, LVL_IMMORT, TRUE, "SYSERR: Reset %s INT from %d to %d.", GET_NAME(ch), (int)ch->real_abils.intel, (int)race_max_stats[GET_RACE(ch)][1]);
     ch->real_abils.intel -= diff;
   }
   if ((diff = ch->real_abils.wis - race_max_stats[GET_RACE(ch)][2]) > 0 && GET_LEVEL(ch) < LVL_IMMORT) {
     mudlogf(BRF, LVL_IMMORT, TRUE, "SYSERR: Reset %s WIS from %d to %d.", GET_NAME(ch), (int)ch->real_abils.wis, (int)race_max_stats[GET_RACE(ch)][2]);
     ch->real_abils.wis -= diff;
   }
   if ((diff = ch->real_abils.dex - race_max_stats[GET_RACE(ch)][3]) > 0 && GET_LEVEL(ch) < LVL_IMMORT) {
     mudlogf(BRF, LVL_IMMORT, TRUE, "SYSERR: Reset %s DEX from %d to %d.", GET_NAME(ch), (int)ch->real_abils.dex, (int)race_max_stats[GET_RACE(ch)][3]);
     ch->real_abils.dex -= diff;
   }
   if ((diff = ch->real_abils.con - race_max_stats[GET_RACE(ch)][4]) > 0 && GET_LEVEL(ch) < LVL_IMMORT) {
     mudlogf(BRF, LVL_IMMORT, TRUE, "SYSERR: Reset %s CON from %d to %d.", GET_NAME(ch), (int)ch->real_abils.con, (int)race_max_stats[GET_RACE(ch)][4]);
     ch->real_abils.con -= diff;
   }
   if ((diff = ch->real_abils.cha - race_max_stats[GET_RACE(ch)][5]) > 0 && GET_LEVEL(ch) < LVL_IMMORT) {
     mudlogf(BRF, LVL_IMMORT, TRUE, "SYSERR: Reset %s CHA from %d to %d.", GET_NAME(ch), (int)ch->real_abils.cha, (int)race_max_stats[GET_RACE(ch)][5]);
     ch->real_abils.cha -= diff;
   }

   if (!trait_info[GET_RACE(ch)].auto_sneak)
      REMOVE_BIT(AFF_FLAGS(ch), AFF_SNEAK);
   if (!trait_info[GET_RACE(ch)].infravision)
      REMOVE_BIT(AFF_FLAGS(ch), AFF_INFRAVISION);
   if (!trait_info[GET_RACE(ch)].can_fly)
      REMOVE_BIT(AFF_FLAGS(ch), AFF_FLY);
   if (GET_RACE(ch)!=RACE_SPRITE)
      REMOVE_BIT(AFF_FLAGS(ch), AFF_DETECT_INVIS);

   for (i=0; i<NUM_RACES; i++)
      {
      if (IS_SET(IMMUNE(ch),trait_info[GET_RACE(ch)].immune))
         REMOVE_BIT(IMMUNE(ch),trait_info[i].immune);
      if (IS_SET(RESIST(ch),trait_info[GET_RACE(ch)].resist))
         REMOVE_BIT(RESIST(ch),trait_info[i].resist);
      if (IS_SET(SUCCEPT(ch),trait_info[GET_RACE(ch)].susceptible))
         REMOVE_BIT(SUCCEPT(ch),trait_info[i].susceptible);
      }
   affect_total(ch);
}


void show_race(struct char_data * ch, char *value)
{
   char *buf=get_buffer(MAX_STRING_LENGTH);
   char *buf2=get_buffer(MAX_STRING_LENGTH);
   int i=0;

   if(!*value)
      {
      strcpy(buf,"Race|Huma|Long|LvSz|Talk|Fly |Swim|Hand|Warm|Imat|!stn|Infr|Ultr|HitB|ASnk|Tame|Morl\r\n");
      strcat(buf,"----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----\r\n");
      while(*race_abbrevs[i]!='\n')
	 {
	 sprintf(buf+strlen(buf),"%-4.4s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%4d|%s|%s|%4d\r\n",race_abbrevs[i],
		 trait_info[i].humanoid==1?"XXXX":"    ",
		 trait_info[i].length==1?"XXXX":"    ",
		 trait_info[i].level_size==1?"XXXX":"    ",
		 trait_info[i].can_talk==1?"XXXX":"    ",
		 trait_info[i].can_fly==1?"XXXX":"    ",
		 trait_info[i].can_swim==1?"XXXX":"    ",
		 trait_info[i].has_hands==1?"XXXX":"    ",
		 trait_info[i].emits_heat==1?"XXXX":"    ",
		 trait_info[i].immaterial==1?"XXXX":"    ",
		 trait_info[i].stun_proof==1?"XXXX":"    ",
		 trait_info[i].infravision==1?"XXXX":"    ",
		 trait_info[i].ultravision==1?"XXXX":"    ",
		 trait_info[i].hitroll,
		 trait_info[i].auto_sneak==1?"XXXX":"    ",
		 trait_info[i].can_tame==1?"XXXX":"    ",
		 trait_info[i].morale);
	 i++;
	 }
      strcat(buf,"----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----\r\n");
      strcat(buf,"Race|Huma|Long|LvSz|Talk|Fly |Swim|Hand|Warm|Imat|!stn|Infr|Ultr|HitB|ASnk|Tame|Morl\r\n");
      }
   else
      {
      i=0;
      while(*npc_race_types[i]!='\n')
	 {
	 if(is_abbrev(value,npc_race_types[i]))
	    {
	    sprintf(buf,"Racial Stats for: %s\r\n",npc_race_types[i]);
	    if(i<15)
	       {
	       strcat(buf," Player Affecting Stats:\r\n");
	       sprintf(buf+strlen(buf),
		       "  Start Age: %4d   Max Age: %4d HP Bonus: %4d\r\n",
		       racial_info[i].base_age,racial_info[i].max_age,
		       racial_info[i].hp_bonus);
	       sprintf(buf+strlen(buf),
		       "  Base Move: %4d Base Mana: %4d Max Mana: %4d(%d)\r\n",
		       racial_info[i].base_moves,racial_info[i].base_mana,
		       racial_info[i].max_mana,racial_info[i].mana_bonus);
	       sprintf(buf+strlen(buf),
		       "  StatMin: S: %2ld I: %2ld W: %2ld D: %2ld C: "
		       "%2ld Ch: %2ld\r\n",race_stats[i][0],race_stats[i][1],
		       race_stats[i][2],race_stats[i][3],race_stats[i][4],
		       race_stats[i][5]);
	       sprintf(buf+strlen(buf),
		       "  StatMax: S: %2ld I: %2ld W: %2ld D: %2ld C: "
		       "%2ld Ch: %2ld\r\n\r\n",race_max_stats[i][0],
		       race_max_stats[i][1], race_max_stats[i][2],
		       race_max_stats[i][3], race_max_stats[i][4],
		       race_max_stats[i][5]);
	       }
	    strcat(buf," Mob Affecting Stats: (1)\r\n");
	    sprintf(buf+strlen(buf),
		    "   Mob Stat Percent: S: %3d%% I: %3d%% W: %3d%% D: %3d%% "
		    "C: %3d%% Ch: %3d%%\r\n\r\n", race_stat_info[i].Str,
		    race_stat_info[i].Int,race_stat_info[i].Wis,
		    race_stat_info[i].Dex,race_stat_info[i].Con,
		    race_stat_info[i].Cha);
	    strcat(buf," Size information: (min/avg/max) inches and lbs.\r\n");
	    sprintf(buf+strlen(buf),
		    "   Male:   height: (%8ld/%8ld/%8ld)\r\n",
		    race_size_info[i].MHmin,
		    race_size_info[i].MHave,
		    race_size_info[i].MHmax);
	    sprintf(buf+strlen(buf),
		    "   Male:   weight: (%8ld/%8ld/%8ld)\r\n",
		    race_size_info[i].MWmin,
		    race_size_info[i].MWave,
		    race_size_info[i].MWmax);
	    sprintf(buf+strlen(buf),
		    "   Female: height: (%8ld/%8ld/%8ld)\r\n",
		    race_size_info[i].FHmin,
		    race_size_info[i].FHave,
		    race_size_info[i].FHmax);
	    sprintf(buf+strlen(buf),
		    "   Female: height: (%8ld/%8ld/%8ld)\r\n\r\n",
		    race_size_info[i].FWmin,
		    race_size_info[i].FWave,
		    race_size_info[i].FWmax);

	    strcat(buf," Racial Traits:\r\n");
	    sprintf(buf+strlen(buf),
		    "     Humanoid: %c        Long: %c  Level-Size: %c\r\n",
		    trait_info[i].humanoid==1?'Y':'_',
		    trait_info[i].length==1?'Y':'_',
		    trait_info[i].level_size==1?'Y':'_');
	    sprintf(buf+strlen(buf),
		    "        Talks: %c       Flies: %c       Swims: %c\r\n",
		    trait_info[i].can_talk==1?'Y':'_',
		    trait_info[i].can_fly==1?'Y':'_',
		    trait_info[i].can_swim==1?'Y':'_');
	    sprintf(buf+strlen(buf),
		    "    Has Hands: %c     Is Warm: %c  Immaterial: %c\r\n",
		    trait_info[i].has_hands==1?'Y':'_',
		    trait_info[i].emits_heat==1?'Y':'_',
		    trait_info[i].immaterial==1?'Y':'_');
	    sprintf(buf+strlen(buf),
		    "      No Stun: %c Infravision: %c Ultravision: %c\r\n",
		    trait_info[i].stun_proof==1?'Y':'_',
		    trait_info[i].infravision==1?'Y':'_',
		    trait_info[i].ultravision==1?'Y':'_');
	    sprintf(buf+strlen(buf),
		    "   Auto Sneak: %c    HR Bonus: %d      Morale: %d\r\n",
		    trait_info[i].auto_sneak==1?'Y':'_',
		    trait_info[i].hitroll,
		    trait_info[i].morale);
	    sprintf(buf+strlen(buf),
		    "   Can Tame  : %c\r\n",
		    trait_info[i].can_tame==1?'Y':'_');
	    sprintbit(trait_info[i].immune,immunity_names,buf2);
	    sprintf(buf+strlen(buf),
		    "\r\n  Immune to:      %s\r\n",buf2);
	    sprintbit(trait_info[i].resist,immunity_names,buf2);
	    sprintf(buf+strlen(buf),
		    "  Resistant to:   %s\r\n",buf2);
	    sprintbit(trait_info[i].susceptible,immunity_names,buf2);
	    sprintf(buf+strlen(buf),
		    "  Susceptable to: %s\r\n",buf2);

	    strcat(buf,"\r\n  (1) These are percentages vs human stats.\r\n");
	    break;
	    }
	 i++;
	 }
      if(*npc_race_types[i]=='\n')
	 {
	 sprintf(buf,"Race %s not found.  Valid races:\r\n",value);
	 i=0;
	 while(*npc_race_types[i]!='\n')
	    {
	    sprintf(buf+strlen(buf)," %-15.15s",npc_race_types[i]);
	    i++;
	    if((i%4)==0)
	       strcat(buf,"\r\n");
	    }
	 }
      }

   
   
   release_buffer(buf2);
   if(ch->desc)
      page_string(ch->desc, buf, TRUE,"");
   release_buffer(buf);
}


struct srr_skills_struct srr_skills[] =
{
  {RACE_HUMAN, SPELL_SANCTUARY, 50},
  {-1,-1,-1}
};

int find_srr_skill(int race, int spell_num)
{
  int i;
  
  for (i = 0; srr_skills[i].race != RACE_UNDEFINED; i++)
  {
    if (race != srr_skills[i].race) {
      continue;
    }
    if (spell_num != srr_skills[i].spell_num) {
      continue;
    }
    return srr_skills[i].level;
  }

  return LVL_IMPL+1;
}
