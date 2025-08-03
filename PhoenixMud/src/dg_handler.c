#include "../localHeader/conf.h"
#include "../localHeader/sysdep.h"
    

#include "structs.h"
#include "dg_scripts.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "spells.h"
#include "dg_event.h"
#include "buffer.h"

extern struct index_data **trig_index;
extern struct trig_data *trigger_list;
extern const char *affected_bits[];
extern const char *room_bits[];
extern const char *room2_bits[];
extern struct room_data *world;

void trig_data_free(trig_data *this_data);

/* return memory used by a trigger */
void free_trigger(struct trig_data *trig)
{
  /* threw this_data in for minor consistance in names with the rest of circle */
   trig_data_free(trig);
}


/* remove a single trigger from a mob/obj/room */
void extract_trigger(struct trig_data *trig)
{
   struct trig_data *temp;

   if (GET_TRIG_WAIT(trig)) 
      {
      remove_event(GET_TRIG_WAIT(trig));
      GET_TRIG_WAIT(trig) = NULL;
      }

   trig_index[trig->nr]->number--; 

  /* walk the trigger list and remove this one */
   REMOVE_FROM_LIST(trig, trigger_list, next_in_world);

   free_trigger(trig);
}

/* remove all triggers from a mob/obj/room */
void extract_script(struct script_data *sc)
{
   struct trig_data *trig, *next_trig;

   for (trig = TRIGGERS(sc); trig; trig = next_trig) 
      {
      next_trig = trig->next;
      extract_trigger(trig);
      trig = NULL;
      }
   TRIGGERS(sc) = NULL;
   //free(sc);
}

/* erase the script memory of a mob */
void extract_script_mem(struct script_memory *sc)
{
   struct script_memory *next;
   while (sc)
      {
      next = sc->next;
      if (sc->cmd) 
	 free(sc->cmd);
      free(sc);
      sc = next;
      }
}

/* perhaps not the best place for this, but I didn't want a new file */
char *skill_percent(struct char_data *ch, char *skill)
{
   static char retval[16];
   int skillnum;
   
   skillnum = find_skill_num(skill);
   if (skillnum<=0)
      return("unknown skill");
   
   if(IS_NPC(ch))
      sprintf(retval,"%d",MIN(MAX(35,GET_LEVEL(ch)*2),80));
   else
      sprintf(retval,"%d",GET_SKILL(ch, skillnum));
   return retval;
}

extern const char *affected_bits[];
extern const char *affected2_bits[];
extern const char *preference_bits[];
extern const char *preference2_bits[];
extern int is_abbrev(char *arg1, const char *arg2);

char *dg_get_ch_aff(struct char_data *ch, char *aff)
{
   static char retval[16];
   int i = -1;

   if (!aff) {
     strcpy(retval, "0");
     return retval;
   }
   for (i = 0; *affected_bits[i] != '\n'; i++) {
     if (affected_bits[i] && strlen(affected_bits[i]) > 1 && is_abbrev(aff, affected_bits[i])) {
       if (AFF_FLAGGED(ch, (1 << i))) {
	 strcpy(retval, "1");
       } else {
	 strcpy(retval, "0");
       }
       return retval;
     }
   }
   for (i = 0; *affected2_bits[i] != '\n'; i++) {
     if (affected2_bits[i] && strlen(affected2_bits[i]) > 1 && is_abbrev(aff, affected2_bits[i])) {
       if (AFF2_FLAGGED(ch, (1 << i))) {
	 strcpy(retval, "1");
       } else {
	 strcpy(retval, "0");
       }
       return retval;
     }
   }
   strcpy(retval, "0");
   return retval;
}

char *dg_get_ch_prf(struct char_data *ch, char *prf)
{
   static char retval[16];
   int i = -1;

   if (!prf) {
     strcpy(retval, "0");
     return retval;
   }
   for (i = 0; *preference_bits[i] != '\n'; i++) {
     if (preference_bits[i] && strlen(preference_bits[i]) > 1 && is_abbrev(prf, preference_bits[i])) {
       if (PRF_FLAGGED(ch, (1 << i))) {
	 strcpy(retval, "1");
       } else {
	 strcpy(retval, "0");
       }
       return retval;
     }
   }
   for (i = 0; *preference2_bits[i] != '\n'; i++) {
     if (preference2_bits[i] && strlen(preference2_bits[i]) > 1 && is_abbrev(prf, preference2_bits[i])) {
       if (PRF2_FLAGGED(ch, (1 << i))) {
	 strcpy(retval, "1");
       } else {
	 strcpy(retval, "0");
       }
       return retval;
     }
   }
   strcpy(retval, "0");
   return retval;
}

extern const char *immunity_names[];

char *dg_get_ch_res(struct char_data *ch, char *bit)
{
   static char retval[16];
   int i = -1;

   if (!bit) {
     strcpy(retval, "0");
     return retval;
   }
   for (i = 0; *immunity_names[i] != '\n'; i++) {
     if (immunity_names[i] && strlen(immunity_names[i]) > 1 && is_abbrev(bit, immunity_names[i])) {
       if (IS_RESIST(ch, (1 << i))) {
	 strcpy(retval, "1");
       } else {
	 strcpy(retval, "0");
       }
       return retval;
     }
   }
   strcpy(retval, "0");
   return retval;
}

char *dg_get_ch_sus(struct char_data *ch, char *bit)
{
   static char retval[16];
   int i = -1;

   if (!bit) {
     strcpy(retval, "0");
     return retval;
   }
   for (i = 0; *immunity_names[i] != '\n'; i++) {
     if (immunity_names[i] && strlen(immunity_names[i]) > 1 && is_abbrev(bit, immunity_names[i])) {
       if (IS_SUCCEPT(ch, (1 << i))) {
	 strcpy(retval, "1");
       } else {
	 strcpy(retval, "0");
       }
       return retval;
     }
   }
   strcpy(retval, "0");
   return retval;
}

char *dg_get_ch_imm(struct char_data *ch, char *bit)
{
   static char retval[16];
   int i = -1;

   if (!bit) {
     strcpy(retval, "0");
     return retval;
   }
   for (i = 0; *immunity_names[i] != '\n'; i++) {
     if (immunity_names[i] && strlen(immunity_names[i]) > 1 && is_abbrev(bit, immunity_names[i])) {
       if (IS_IMMUNE(ch, (1 << i))) {
	 strcpy(retval, "1");
       } else {
	 strcpy(retval, "0");
       }
       return retval;
     }
   }
   strcpy(retval, "0");
   return retval;
}

char *dg_get_room_flags(struct room_data *room, char *flag)
{
   static char retval[16];
   int i = -1;

   if (!flag) {
     strcpy(retval, "0");
     return retval;
   }
   for (i = 0; *room_bits[i] != '\n'; i++) {
     if (room_bits[i] && strlen(room_bits[i]) > 1 && is_abbrev(flag, room_bits[i])) {
       if (IS_SET(room->room_flags, 1 << i)) {
	 strcpy(retval, "1");
       } else {
	 strcpy(retval, "0");
       }
       return retval;
     }
   }
   for (i = 0; *room2_bits[i] != '\n'; i++) {
     if (room2_bits[i] && strlen(room2_bits[i]) > 1 && is_abbrev(flag, room2_bits[i])) {
       if (IS_SET(room->room2_flags, 1 << i)) {
	 strcpy(retval, "1");
       } else {
	 strcpy(retval, "0");
       }
       return retval;
     }
   }
   strcpy(retval, "0");
   return retval;
}
