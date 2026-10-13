/* ************************************************************************ 
*   File: spec_assign.c                                 Part of CircleMUD * 
*  Usage: Functions to assign function pointers to objs/mobs/rooms        * 
*                                                                         * 
*  All rights reserved.  See license.doc for complete information.        * 
*                                                                         * 
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University * 
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               * 
************************************************************************ */ 
 
#include "../localHeader/conf.h" 
#include "../localHeader/sysdep.h" 
 
#include "structs.h" 
#include "db.h" 
#include "comm.h"
#include "interpreter.h" 
#include "buffer.h" 
#include "utils.h" 
#include "spec_assign.h" 
#include "vnum.h"
#include "gremort_exam.h"

extern struct room_data *world; 
extern int mini_mud; 
extern mob_rnum top_of_mobt; 
extern struct index_data *mob_index; 
extern struct index_data *obj_index; 
extern int dts_are_dumps; 
extern room_vnum donation_rooms[];
extern char *class_abbrevs[]; 
extern  char *race_abbrevs[];
extern struct char_data *mob_proto; 

void assign_kings_castle(void); 
SPECIAL(archer); 
SPECIAL(butcher); 
SPECIAL(cityguard); 
SPECIAL(cryogenicist); 
SPECIAL(fido); 
SPECIAL(guild); 
SPECIAL(guild_guard); 
SPECIAL(home_keeper);
SPECIAL(janitor); 
SPECIAL(magic_user); 
SPECIAL(mayor); 
SPECIAL(postmaster); 
SPECIAL(puff); 
SPECIAL(receptionist); 
SPECIAL(snake); 
SPECIAL(thief); 
SPECIAL(CastleGuard); 
SPECIAL(James); 
SPECIAL(cleaning); 
SPECIAL(DicknDavid); 
SPECIAL(tim); 
SPECIAL(tom); 
SPECIAL(king_welmar); 
SPECIAL(training_master); 
SPECIAL(peter); 
SPECIAL(jerry); 
SPECIAL(shop_keeper); 
SPECIAL(priest);
SPECIAL(leviathan);
SPECIAL(sea_serpent);
SPECIAL(breath_fire);
SPECIAL(breath_gas);
SPECIAL(breath_frost);
SPECIAL(breath_acid);
SPECIAL(breath_lightning);
SPECIAL(dragon);
SPECIAL(robin_hood);
SPECIAL(trainer);
SPECIAL(questmob);
SPECIAL(healer);
SPECIAL(executioner);
SPECIAL(pet_shops);
SPECIAL(bank);
SPECIAL(portal);
SPECIAL(dump); 
SPECIAL(mirror);
SPECIAL(repair_guy);
SPECIAL(stable_master);
SPECIAL(roulette);
SPECIAL(sage);
SPECIAL(recharge_guy);
SPECIAL(battle_master);
SPECIAL(explore_seer);

struct spec_list
{
   char name[30];
   SPECIAL(*func);
};

struct spec_list mob_specs[] = 
{
  /* BEGIN HERE */
  /* people procs */
/*    {"None",NULL}, */
   {"archer(need code)", archer},/* 0 */
   {"butcher", butcher},
   {"cityguard", cityguard},
   {"fido", fido},
   {"guild", guild},
   {"guild_guard(need code)", guild_guard}, /* 5 */
   {"janitor", janitor},
   {"magic_user", magic_user},
   {"priest",priest},
   {"thief", thief},
   {"mayor", mayor},		/* 10 */
   {"postmaster", postmaster},
   {"trainer(rare)",trainer},
   {"questmob(rare)",questmob},
   {"healer(rare)",healer},
   {"home(1/town)",home_keeper}, /* 15 */
   {"executioner",executioner},

  /* kings castle */
   {"CastleGuard", CastleGuard},
   {"James", James},
   {"cleaning", cleaning},
   {"DicknDavid", DicknDavid},	/* 20 */
   {"tim", tim},
   {"tom", tom},
   {"king_welmar", king_welmar},
   {"training_master", training_master},
   {"peter", peter},		/* 25 */
   {"jerry", jerry},

  /* animal type stuff */
   {"leviathan",leviathan},
   {"sea_serpent",sea_serpent},
   {"snake", snake},
   {"breath_fire",breath_fire},	/* 30 */
   {"breath_gas",breath_gas},
   {"breath_frost",breath_frost},
   {"breath_acid",breath_acid},
   {"breath_lightning",breath_lightning},
   {"dragon",dragon},		/* 35 */
  /* really special */
   {"puff", puff},
   {"robin_hood(dont set)",robin_hood},
   {"shop_keeper(DONT SET)", shop_keeper},
   {"receptionist", receptionist},
   {"cryogenicist", cryogenicist}, /* 40 */
   {"mirror",  mirror},
   {"repair_guy",  repair_guy},
   {"stable_master",stable_master},
   {"sage",sage},
   {"recharge_guy",recharge_guy}, /* 45 */
   {"battle_master",battle_master},
   {"explore_seer",explore_seer},
/* STOP HERE */
   {"None",NULL},
   {"Unknown, exists", NULL}	/* Terminator */
};

char *get_mob_spec_name(SPECIAL(func))
{
   int i;

   if(!func)
      return "None";

   for(i=0;mob_specs[i].func&&(mob_specs[i].func!=func);i++)
      ;

   return mob_specs[i].name;
}

int get_mob_spec_num(SPECIAL(func))
{
   int i;

   if(!func)
      return NUM_SPECS+1;

   for(i=0;mob_specs[i].func&&(mob_specs[i].func!=func);i++)
      ;

   return i;
}

proctype get_mob_spec_proc(char *func_name)
{
   int i;

   if(!strcmp(func_name,"None"))
      return NULL;
   
   for(i=0;mob_specs[i].name &&(strcmp(func_name,mob_specs[i].name)!=0);i++)
      ;
   
   return mob_specs[i].func;
}

/* the name a mob special is listed under, or NULL (the class weapon's boss
 * pool leaves services out by name) */
const char *mob_spec_name(SPECIAL(*f))
{
   int i;

   if(!f)
      return NULL;
   for(i=0;strcmp(mob_specs[i].name,"Unknown, exists");i++)
      if(mob_specs[i].func==f)
         return mob_specs[i].name;
   return NULL;
}

void list_mob_spec_procs(struct char_data *ch,char *arg)
{
   int i;
   char *buf=get_buffer(32750);
   int nr = 0;
   int found = 0;

   if(*arg && *arg!='\0')
      {
      for(i=0;mob_specs[i].func;i++)
	 {
	 if(is_abbrev(arg,mob_specs[i].name))
	    {
	    sprintf(buf,"Mobs with %s\r\n",mob_specs[i].name);
	    strcat(buf,"Num  Virtual Description                         lvl|cl|race|spec\r\n");
	    for (nr=0; nr <= top_of_mobt; nr++) 
	       { 
	       if(mob_specs[i].func == mob_index[nr].func)
		  {
		  if(strlen(buf)>32000)
		     {
		     sprintf(buf+strlen(buf),"Too Many Items In Search\r\n");
		     nr=top_of_mobt+1;
		     }
		  else
		     sprintf(buf+strlen(buf),
			     "%3d. [%5ld] %-35.35s %3d|%-2.2s|%-4.4s\r\n",
			     ++found, 
			     mob_index[nr].vnum, 
			     mob_proto[nr].player.short_descr,
			     mob_proto[nr].player.level,
			     class_abbrevs[(int)mob_proto[nr].player.class],
			     race_abbrevs[(int)mob_proto[nr].player.race]); 
		  }
	       }
	    if(ch->desc) 
	       page_string(ch->desc,buf,TRUE,""); 
	    release_buffer(buf);
	    return;
	    }
	 }
      }
   for(i=0;mob_specs[i].func;i++)
      {
      send_to_char(ch, "%-20.20s",mob_specs[i].name);
      if(i%4==3)
	 send_to_char(ch,"\r\n");
      }
   send_to_char(ch,"\r\n");
   release_buffer(buf);
}

/* functions to perform assignments */ 
 
void ASSIGNMOB(mob_vnum mob, SPECIAL(fname)) 
{ 
   mob_rnum rnum;
   if ((rnum=real_mobile(mob)) >= 0) 
      {
      mob_index[rnum].func = fname; 

      }
   else if (!mini_mud) 
      { 
      log("SYSERR: Attempt to assign spec to non-existant mob #%ld", mob); 
      } 
} 
 
void ASSIGNOBJ(obj_vnum obj, SPECIAL(fname)) 
{ 
   obj_rnum rnum;
   if ((rnum=real_object(obj)) >= 0) 
      obj_index[rnum].func = fname; 
   else if (!mini_mud) 
      { 
      log("SYSERR: Attempt to assign spec to non-existant obj #%ld", obj); 
      } 
} 
 
void ASSIGNROOM(room_vnum room, SPECIAL(fname)) 
{ 
   room_rnum rnum;
   if ((rnum=real_room(room)) >= 0) 
      world[rnum].func = fname; 
   else if (!mini_mud) 
      { 
      log("SYSERR: Attempt to assign spec to non-existant rm. #%ld", room); 
      } 
} 
 
 
/* ******************************************************************** 
*  Assignments                                                        * 
******************************************************************** */ 
 
/* assign special procedures to mobiles */ 
void assign_mobiles(void) 
{ 
 
/* apia's theatre */ /* Commented out until completed Anduin  
   SPECIAL(phantom); 
   SPECIAL(usher); 
   */ 
 
 
/*   assign_kings_castle(); 
 */
   ASSIGNMOB(1, puff); 
 
  /* Immortal Zone */ 
   ASSIGNMOB(1201, postmaster); 
   ASSIGNMOB(1202, janitor); 
 
  /* Midgaard */ 
   ASSIGNMOB(3005, receptionist); 
   ASSIGNMOB(3010, postmaster); 
   ASSIGNMOB(3020, guild); 
   ASSIGNMOB(3021, guild); 
   ASSIGNMOB(3022, guild); 
   ASSIGNMOB(3023, guild); 
   ASSIGNMOB(3024, guild_guard); 
   ASSIGNMOB(3025, guild_guard); 
   ASSIGNMOB(3026, guild_guard); 
   ASSIGNMOB(3027, guild_guard); 
   ASSIGNMOB(3059, cityguard); 
   ASSIGNMOB(3060, cityguard); 
   ASSIGNMOB(3061, janitor); 
   ASSIGNMOB(3062, fido); 
   ASSIGNMOB(3066, fido); 
   ASSIGNMOB(3067, cityguard); 
   ASSIGNMOB(3068, janitor); 
 
  /* MORIA */ 
   ASSIGNMOB(4000, snake); 
   ASSIGNMOB(4001, snake); 
   ASSIGNMOB(4053, snake); 
   ASSIGNMOB(4100, magic_user); 
   ASSIGNMOB(4102, snake); 
   ASSIGNMOB(4103, thief); 
 
  /* PYRAMID */ 
   ASSIGNMOB(5300, snake); 
   ASSIGNMOB(5301, snake); 
   ASSIGNMOB(5304, thief); 
   ASSIGNMOB(5305, thief); 
   ASSIGNMOB(5309, magic_user); /* should breath fire */ 
   ASSIGNMOB(5311, magic_user); 
   ASSIGNMOB(5313, magic_user); /* should be a cleric */ 
   ASSIGNMOB(5314, magic_user); /* should be a cleric */ 
   ASSIGNMOB(5315, magic_user); /* should be a cleric */ 
   ASSIGNMOB(5316, magic_user); /* should be a cleric */ 
   ASSIGNMOB(5317, magic_user); 
 
  /* High Tower Of Sorcery */ 
   ASSIGNMOB(2501, magic_user); /* should likely be cleric */ 
   ASSIGNMOB(2504, magic_user); 
   ASSIGNMOB(2507, magic_user); 
   ASSIGNMOB(2508, magic_user); 
   ASSIGNMOB(2510, magic_user); 
   ASSIGNMOB(2511, thief); 
   ASSIGNMOB(2514, magic_user); 
   ASSIGNMOB(2515, magic_user); 
   ASSIGNMOB(2516, magic_user); 
   ASSIGNMOB(2517, magic_user); 
   ASSIGNMOB(2518, magic_user); 
   ASSIGNMOB(2520, magic_user); 
   ASSIGNMOB(2521, magic_user); 
   ASSIGNMOB(2522, magic_user); 
   ASSIGNMOB(2523, magic_user); 
   ASSIGNMOB(2524, magic_user); 
   ASSIGNMOB(2525, magic_user); 
   ASSIGNMOB(2526, magic_user); 
   ASSIGNMOB(2527, magic_user); 
   ASSIGNMOB(2528, magic_user); 
   ASSIGNMOB(2529, magic_user); 
   ASSIGNMOB(2530, magic_user); 
   ASSIGNMOB(2531, magic_user); 
   ASSIGNMOB(2532, magic_user); 
   ASSIGNMOB(2533, magic_user); 
   ASSIGNMOB(2534, magic_user); 
   ASSIGNMOB(2536, magic_user); 
   ASSIGNMOB(2537, magic_user); 
   ASSIGNMOB(2538, magic_user); 
   ASSIGNMOB(2540, magic_user); 
   ASSIGNMOB(2541, magic_user); 
   ASSIGNMOB(2548, magic_user); 
   ASSIGNMOB(2549, magic_user); 
   ASSIGNMOB(2552, magic_user); 
   ASSIGNMOB(2553, magic_user); 
   ASSIGNMOB(2554, magic_user); 
   ASSIGNMOB(2556, magic_user); 
   ASSIGNMOB(2557, magic_user); 
   ASSIGNMOB(2559, magic_user); 
   ASSIGNMOB(2560, magic_user); 
   ASSIGNMOB(2562, magic_user); 
   ASSIGNMOB(2564, magic_user); 
 
 
  /* Desert */ 
   ASSIGNMOB(5010, magic_user); 
   ASSIGNMOB(5014, magic_user); 
 
  /* Drow City */ 
   ASSIGNMOB(5103, magic_user); 
   ASSIGNMOB(5104, magic_user); 
   ASSIGNMOB(5107, magic_user); 
   ASSIGNMOB(5108, magic_user); 
 
  /* Old Thalos */ 
   ASSIGNMOB(5200, magic_user); 
   ASSIGNMOB(5201, magic_user); 
   ASSIGNMOB(5209, magic_user); 
 
  /* New Thalos */ 
/* 5481 - Cleric (or Mage... but he IS a high priest... *shrug*) */ 
   ASSIGNMOB(5404, receptionist); 
   ASSIGNMOB(5421, magic_user); 
   ASSIGNMOB(5422, magic_user); 
   ASSIGNMOB(5423, magic_user); 
   ASSIGNMOB(5424, magic_user); 
   ASSIGNMOB(5425, magic_user); 
   ASSIGNMOB(5426, magic_user); 
   ASSIGNMOB(5427, magic_user); 
   ASSIGNMOB(5428, magic_user); 
   ASSIGNMOB(5434, cityguard); 
   ASSIGNMOB(5440, magic_user); 
   ASSIGNMOB(5455, magic_user); 
   ASSIGNMOB(5461, cityguard); 
   ASSIGNMOB(5462, cityguard); 
   ASSIGNMOB(5463, cityguard); 
   ASSIGNMOB(5482, cityguard); 
/* 
   5400 - Guildmaster (Mage) 
   5401 - Guildmaster (Cleric) 
   5402 - Guildmaster (Warrior) 
   5403 - Guildmaster (Thief) 
   5456 - Guildguard (Mage) 
   5457 - Guildguard (Cleric) 
   5458 - Guildguard (Warrior) 
   5459 - Guildguard (Thief) 
   */ 
 
  /* ROME */ 
/*   ASSIGNMOB(12009, magic_user);  */
/*    ASSIGNMOB(12018, cityguard);  */
/*    ASSIGNMOB(12020, magic_user);  */
/*    ASSIGNMOB(12021, cityguard);  */
/*    ASSIGNMOB(12025, magic_user);  */
/*    ASSIGNMOB(12030, magic_user);  */
/*    ASSIGNMOB(12031, magic_user);  */
/*    ASSIGNMOB(12032, magic_user); */

/*    apia's theatre  Commented out by anduin   */
/* ASSIGNMOB(30301, usher);  */
/* ASSIGNMOB (30328, phantom);  */
 
  /* King Welmar's Castle (not covered in castle.c) */ 
/*    ASSIGNMOB(15015, thief);       */
/*    ASSIGNMOB(15032, magic_user);  */
 
} 
 
 
 
/* assign special procedures to objects */ 
void assign_objects(void) 
{ 

   ASSIGNOBJ(3034, bank); /* atm */ 
/*    ASSIGNOBJ(3036, bank);  *//* cashcard */ 
   ASSIGNOBJ(35, portal); /* portal  */ 
   ASSIGNOBJ(ROULETTE_WHEEL,roulette); /*Kendear's roulette wheel*/
} 
 
extern SPECIAL(gremort_attributes_check_proc);
extern SPECIAL(gremort_exam_trivia_proc);
extern SPECIAL(gremort_exam_quest_proc);
 
/* assign special procedures to rooms */ 
void assign_rooms(void) 
{ 
   room_rnum i; 
   room_rnum RDR;

   ASSIGNROOM(3030, dump); 
   ASSIGNROOM(3031, pet_shops); 
   ASSIGNROOM(5530, pet_shops); 

   ASSIGNROOM(GREMORT_EXAM_PART1_ROOM_VNUM, gremort_attributes_check_proc);
   ASSIGNROOM(GREMORT_EXAM_PART2_ROOM_VNUM, gremort_exam_trivia_proc);
   ASSIGNROOM(GREMORT_EXAM_PART3_ROOM_VNUM, gremort_exam_quest_proc);

  /* commendted by Anduin 
   * ASSIGNROOM(30317, whistle); 
   */ 
   if (dts_are_dumps) 
      for (i = 0; i < top_of_world; i++) 
	 if (ROOM_FLAGGED(i, ROOM_DEATH))
	    world[i].func = dump; 
   for(i=0;donation_rooms[i]>0;i++)
      {
      if((RDR=real_room(donation_rooms[i]))!=NOWHERE)
	 SET_BIT(ROOM_FLAGS(RDR),ROOM_DONATION);
      }
} 
 
 
