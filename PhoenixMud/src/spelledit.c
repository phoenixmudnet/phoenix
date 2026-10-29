/***************************************************************************
 *  File: spelledit.c                                   Part of PhoenixMud *
 *  Usage: Code needed to load the spell system that allows admins         *
 *         to load and eventually save spell parameters.                   *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *  AAM Apr 03                                                             *
 *                                                                         *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 *  PhoenixMUD is based on CircleMUD, Copyright (C) 1996-2003.             *
 ***************************************************************************/
#include "../localHeader/conf.h"
#include "../localHeader/sysdep.h"
#include "structs.h"
#include "buffer.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"
#include "spells.h"
#include "constants.h"


bitvector_t asciiflag_conv(char *flag);
/* Set this define to wherever you want to save your corpses */
#define SPELLS_LOAD_FILE "etc/spells.txt"
struct spell_info_type *spells=NULL;

extern const char *unused_spellname;

/* a number below MAX_SPELLS the file does not list is an unused one, as
 * the file writes those: every reader of spell_name (sort_spells first)
 * expects a string, and nobody may learn it */
static void name_unlisted_spells(void)
   {
   int nr, c;

   if (!spells)
      return;
   for (nr = 0; nr < MAX_SPELLS; nr++)
      if (!spells[nr].spell_name)
         {
         spells[nr].spell_name = str_dup(unused_spellname);
         for (c = 0; c < NUM_CLASSES; c++)
            spells[nr].min_level[c] = LVL_IMMORT;
         }
   }

static void read_spells_file(void);

void load_spells(void)
   {
   read_spells_file();
   name_unlisted_spells();
   }

static void read_spells_file(void) {
   FILE *fp;
   char *line=get_buffer(256);
   char *buf1=get_buffer(2048);
   char *buf2=get_buffer(2048);
   int version;
   long nr=0;
   long last =0;
   int t[20];
   char *flags = get_buffer(128);
   char *flags2 = get_buffer(128);
   int i=0;
   int new_record=FALSE;
   
   if (!(fp = fopen(SPELLS_LOAD_FILE, "r+b"))) {
      if (errno != ENOENT) {
         sprintf(buf1, "SYSERR: READING SPELLS FILE %s in load_spells",
                 SPELLS_LOAD_FILE);
         perror(buf1);
         }
      return;
      }

   if(!feof(fp))
      get_line(fp, line);
   if(*line == '@') {
      if(sscanf(line,"@Version: %d",&version)!=1) {
         mudlogf(CMP,LVL_IMMORT,TRUE,
                 "SYSERR SPELLLOAD: Format error in %s with line: %s",
                 SPELLS_LOAD_FILE,line);
         exit(1);
         }
      if(!feof(fp)) {
         get_line(fp,line);
      }
   } else {
      version=1;
   }

   CREATE(spells, struct spell_info_type, MAX_SPELLS+5);
   while (!feof(fp)) {
      if(*line == '#'){
         last = nr;
         if (sscanf(line, "#%ld", &nr) != 1) {
            log("SYSERR: Format error after spell #%ld",  last);
            log("SYSERR: ...Line: %s",line);
            exit(1);
         } else if (nr > MAX_SPELLS) {
            log("SYSERR: Spell number %ld is greater than MAX_SPELLS.", nr);
            exit(1);
         }
      }
      sprintf(buf2, "spell #%ld", nr);
      spells[nr].spell_name        = fread_string(fp, buf2);

      if (!get_line(fp, line)) {
         log("SYSERR: Expecting line 2 for spell #%ld but file ended!", nr);
         exit(1);
      }
      if (sscanf(line, "%d %d %d %d %s %d %s", t, t+1, t+2, t+3,flags, t+4, flags2) != 7) {
         log("SYSERR: Format error in line 2 of spell #%ld(FLAGS): %s", nr,line);
         exit(1);
      }
      spells[nr].mana_max=t[0];
      spells[nr].mana_min=t[1];
      spells[nr].mana_change=t[2];
      spells[nr].min_position=t[3];
      spells[nr].targets=asciiflag_conv(flags);
      spells[nr].violent=t[4];
      spells[nr].routines=asciiflag_conv(flags2);
      
      if (!get_line(fp, line)) {
         log("SYSERR: Expecting line 3 for spell #%ld but file ended!", nr);
         exit(1);
      }
      if (sscanf(line, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d", t, t+1, t+2, t+3, t+4, t+5, t+6, t+7, t+8, t+9, t+10, t+11, t+12, t+13, t+14, t+15, t+16, t+17, t+18) != NUM_CLASSES) {
         log("SYSERR: Format error in line 3 of spell #%ld(LEVELS): %s", nr,line);
         exit(1);
      }
      for(i=0;i<NUM_CLASSES;i++){
         if(t[i]==0){
            spells[nr].min_level[i]=LVL_IMMORT;
         }else{
            spells[nr].min_level[i]=t[i];
         }
      }

      if (!get_line(fp, line)) {
         log("SYSERR: Expecting line 4 for spell #%ld but file ended!", nr);
         exit(1);
      }
      if (sscanf(line, "%d %d", t, t+1) != 2) {
         log("SYSERR: Format error in line 4 of spell #%ld(FLAGS2): %s", nr,line);
         exit(1);
      }
      spells[nr].is_spell=t[0];
      spells[nr].cast_time=t[1];

      spells[nr].wear_off = fread_string(fp, buf2);

      sprintf(buf1, "SYSERR: Format error in spell #%ld (expecting #/$/D)", nr);
      new_record=FALSE;
      while (new_record==FALSE) {
         if(!get_line(fp, line)) {
            log("%s", buf1);
            exit(1);
         }
         switch (*line) {
            case 'D':
            /* To be implented dependancies*/
            break;
            case '#':
               /* new record */
               new_record=TRUE;
               break;
            case '$':
               /*end of file*/
               release_buffer(flags2);
               release_buffer(flags);
               release_buffer(buf2);
               release_buffer(buf1);
               release_buffer(line);
               fclose(fp);
/*               for(j=0;j<MAX_SPELLS;j++){
                 log("Spell %d is %s",j,spells[j].spell_name);}*/
               return;
               break;
            default:
               log("%s", buf1);
               log("SYSERR: %s",line);
               exit(1);
               break;
         }
      }
      
   }
   release_buffer(flags2);
   release_buffer(flags);
   release_buffer(buf2);
   release_buffer(buf1);
   release_buffer(line);
   fclose(fp);
   }

void show_spellstat(struct char_data *ch, char *value) {
   int spell_num;
   char *buf = get_buffer(512);
   int pos=0;
   spell_num = find_skill_num(value);
   if(spell_num==-1) {
      send_to_char(ch,"Sorry, can't find a spell named '%s'\r\n",value);
      return;
   }
   if(spells[spell_num].is_spell == IS_UNUSED){
      send_to_char(ch,"  Type: UNUSED\r\n");
   }else if(spells[spell_num].is_spell == IS_SPELL){
      send_to_char(ch,"  Type: SPELL\r\n");
   }else if(spells[spell_num].is_spell == IS_SKILL){
      send_to_char(ch,"  Type: SKILL\r\n");
   }else {
      send_to_char(ch,"  Type: UNDEFINED:ERROR\r\n");
   }
   send_to_char(ch,"  Name: %s    Cast Time: %d\r\n",spells[spell_num].spell_name,spells[spell_num].cast_time);
   send_to_char(ch,"  Mana Range: %d - %d (-%d)\r\n",spells[spell_num].mana_max,spells[spell_num].mana_min,spells[spell_num].mana_change);
   send_to_char(ch,"  Min Position: %s\r\n",position_types[(int)spells[spell_num].min_position]);

   sprintbit(spells[spell_num].targets, spell_targets, buf);
   send_to_char(ch,"  Targets: %s\r\n",buf);

   sprintbit(spells[spell_num].routines, spell_routines, buf);
   send_to_char(ch,"  Routines: %s\r\n",buf);
   send_to_char(ch,"  Wear off message: %s\r\n",spells[spell_num].wear_off);
   send_to_char(ch,"  Levels: \r\n");   
   strcpy(buf,"    ");
   pos=4;
   if(spells[spell_num].min_level[0]<LVL_IMMORT) {
      pos+=sprintf(buf+pos,"Wa: %3d  ",spells[spell_num].min_level[0]);
   } else {
      strcat(buf,"Wa: ---  ");
      pos+=9;
   }
   if(spells[spell_num].min_level[1]<LVL_IMMORT) {
      pos+=sprintf(buf+pos,"Cl: %3d  ",spells[spell_num].min_level[1]);
   } else {
      strcat(buf,"Cl: ---  ");
      pos+=9;
   }
   if(spells[spell_num].min_level[2]<LVL_IMMORT) {
      pos+=sprintf(buf+pos,"Th: %3d  ",spells[spell_num].min_level[2]);
   } else {
      strcat(buf,"Th: ---  ");
      pos+=9;
   }
   if(spells[spell_num].min_level[3]<LVL_IMMORT) {
      pos+=sprintf(buf+pos,"Mu: %3d  ",spells[spell_num].min_level[3]);
   } else {
      strcat(buf,"Mu: ---  ");
      pos+=9;
   }
   if(spells[spell_num].min_level[4]<LVL_IMMORT) {
      pos+=sprintf(buf+pos,"Ra: %3d  ",spells[spell_num].min_level[4]);
   } else {
      strcat(buf,"Ra: ---  ");
      pos+=9;
   }
   send_to_char(ch,"%s\r\n",buf);     


   strcpy(buf,"    ");
   pos=4;
   if(spells[spell_num].min_level[5]<LVL_IMMORT) {
      pos+=sprintf(buf+pos,"Bd: %3d  ",spells[spell_num].min_level[5]);
   } else {
      strcat(buf,"Bd: ---  ");
      pos+=9;
   }
   if(spells[spell_num].min_level[6]<LVL_IMMORT) {
      pos+=sprintf(buf+pos,"Mo: %3d  ",spells[spell_num].min_level[6]);
   } else {
      strcat(buf,"Ma: ---  ");
      pos+=9;
   }
   if(spells[spell_num].min_level[8]<LVL_IMMORT) {
      pos+=sprintf(buf+pos,"Ba: %3d  ",spells[spell_num].min_level[8]);
   } else {
      strcat(buf,"Ba: ---  ");
      pos+=9;
   }
   if(spells[spell_num].min_level[9]<LVL_IMMORT) {
      pos+=sprintf(buf+pos,"Pa: %3d  ",spells[spell_num].min_level[9]);
   } else {
      strcat(buf,"Pa: ---  ");
      pos+=9;
   }
   if(spells[spell_num].min_level[10]<LVL_IMMORT) {
      pos+=sprintf(buf+pos,"Ap: %3d  ",spells[spell_num].min_level[10]);
   } else {
      strcat(buf,"Ap: ---  ");
      pos+=9;
   }
   send_to_char(ch,"%s\r\n",buf);     


   strcpy(buf,"    ");
   pos=4;
   if(spells[spell_num].min_level[11]<LVL_IMMORT) {
      pos+=sprintf(buf+pos,"Dr: %3d  ",spells[spell_num].min_level[11]);
   } else {
      strcat(buf,"Dr: ---  ");
      pos+=9;
   }
   if(spells[spell_num].min_level[13]<LVL_IMMORT) {
      pos+=sprintf(buf+pos,"Ke: %3d  ",spells[spell_num].min_level[13]);
   } else {
      strcat(buf,"Ke: ---  ");
      pos+=9;
   }
   if(spells[spell_num].min_level[14]<LVL_IMMORT) {
      pos+=sprintf(buf+pos,"As: %3d  ",spells[spell_num].min_level[14]);
   } else {
      strcat(buf,"As: ---  ");
      pos+=9;
   }
   if(spells[spell_num].min_level[15]<LVL_IMMORT) {
      pos+=sprintf(buf+pos,"Ne: %3d  ",spells[spell_num].min_level[15]);
   } else {
      strcat(buf,"Ne: ---  ");
      pos+=9;
   }
   if(spells[spell_num].min_level[16]<LVL_IMMORT) {
      pos+=sprintf(buf+pos,"De: %3d  ",spells[spell_num].min_level[16]);
   } else {
      strcat(buf,"De: ---  ");
      pos+=9;
   }
   send_to_char(ch,"%s\r\n",buf);     
   release_buffer(buf);
   send_to_char(ch,"\r\n");     
   return;
}
