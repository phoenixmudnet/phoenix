/************************************************************************
 * OasisOLC - medit.c                                          v1.5    *
 * Copyright 1996 Harvey Gilpin.                                       *
 ************************************************************************/

#include "../localHeader/conf.h"
#include "../localHeader/sysdep.h"
#include "structs.h"
#include "buffer.h"
#include "utils.h"
#include "comm.h"
#include "spells.h"
#include "db.h"
#include "shop.h"
#include "guild.h"
#include "olc.h"
#include "handler.h"
#include "spec_assign.h"
#include "dg_olc.h"
#include "constants.h"

extern struct guild_master_data *gm_index;
extern int top_guild;

/*
 * Set this to 1 for debugging logs in medit_save_internally.
 */
#if 0
#define DEBUG
#endif

/*
 * Set this to 1 as a last resort to save mobiles.
 */
#if 0
#define I_CRASH
#endif

struct spec_list
   {
   char name[30];
   SPECIAL(*func);
   };


/*-------------------------------------------------------------------*/
/* external variables */
extern struct index_data *mob_index;   /*. db.c     .*/
extern struct char_data *mob_proto;   /*. db.c     .*/
extern struct obj_data *obj_proto;                /*. db.c .*/
extern struct char_data *character_list;  /*. db.c     .*/
extern int top_of_mobt;     /*. db.c     .*/
extern struct zone_data *zone_table;   /*. db.c     .*/
extern int top_of_zone_table;    /*. db.c     .*/
extern struct player_special_data dummy_mob;  /*. db.c     .*/
extern struct attack_hit_type attack_hit_text[];  /*. fight.c  .*/
extern int top_shop;     /*. shop.c .*/
extern struct shop_data *shop_index;   /*. shop.c .*/
extern struct descriptor_data *descriptor_list;  /*. comm.c .*/
extern int top_shop;
extern struct spec_list mob_specs[];
extern char *npc_race_types[];
extern char *npc_class_types[];

#if defined(OASIS_MPROG)
extern char *mobprog_types[];
#endif

/*-------------------------------------------------------------------*/
/*. Handy  macros .*/

#define GET_NDD(mob) ((mob)->mob_specials.damnodice)
#define GET_SDD(mob) ((mob)->mob_specials.damsizedice)
#define GET_ALIAS(mob) ((mob)->player.name)
#define GET_SDESC(mob) ((mob)->player.short_descr)
#define GET_LDESC(mob) ((mob)->player.long_descr)
#define GET_DDESC(mob) ((mob)->player.description)
#define GET_ATTACK(mob) ((mob)->mob_specials.attack_type)
#define S_KEEPER(shop) ((shop)->keeper)
#if defined(OASIS_MPROG)
#define GET_MPROG(mob)         (mob_index[(mob)->nr].mobprogs)
#define GET_MPROG_TYPE(mob)    (mob_index[(mob)->nr].progtypes)
#endif
/*-------------------------------------------------------------------*/
/*. Function prototypes .*/
void justify_mob(struct char_data *mob);

void medit_parse(struct descriptor_data * d, char *arg);
void medit_disp_menu(struct descriptor_data * d);
void medit_setup_new(struct descriptor_data *d);
void medit_setup_existing(struct descriptor_data *d, int rmob_num);
void medit_save_internally(struct descriptor_data *d);
void medit_save_to_disk(int zone_num);
void init_mobile(struct char_data *mob);
void copy_mobile(struct char_data *tmob, struct char_data *fmob);
void medit_disp_positions(struct descriptor_data *d);
void medit_disp_mob_flags(struct descriptor_data *d);
void medit_disp_immun_flags(struct descriptor_data *d);
void medit_disp_aff_flags(struct descriptor_data *d);
void medit_disp_specs(struct descriptor_data *d);
void medit_disp_attack_types(struct descriptor_data *d);
#if defined(OASIS_MPROG)
void medit_disp_mprog(struct descriptor_data *d);
void medit_change_mprog(struct descriptor_data *d);
const char *medit_get_mprog_type(struct mob_prog_data *mprog);
#endif

/*-------------------------------------------------------------------*\
 
  utility functions
\*-------------------------------------------------------------------*/

/*
 * Free a mobile structure that has been edited.
 * Take care of existing mobiles and their mob_proto!
 */

void medit_free_mobile(struct char_data * mob)
   {
   int i;

   if(!mob)
      return;
   if (GET_MOB_RNUM(mob) == -1) /* Non prototyped mobile */
      {
      if (mob->player.name)
         free(mob->player.name);
      if (mob->player.title)
         free(mob->player.title);
      if (mob->player.short_descr)
         free(mob->player.short_descr);
      if (mob->player.long_descr)
         free(mob->player.long_descr);
      if (mob->player.description)
         free(mob->player.description);
      }
   else if ((i = GET_MOB_RNUM(mob)) > -1) /* Prototyped mobile */
      {
      if (mob->player.name && mob->player.name != mob_proto[i].player.name)
         free(mob->player.name);
      if (mob->player.title && mob->player.title != mob_proto[i].player.title)
         free(mob->player.title);
      if (mob->player.short_descr && mob->player.short_descr != mob_proto[i].player.short_descr)
         free(mob->player.short_descr);
      if (mob->player.long_descr && mob->player.long_descr != mob_proto[i].player.long_descr)
         free(mob->player.long_descr);
      if (mob->player.description && mob->player.description != mob_proto[i].player.description)
         free(mob->player.description);
      }

   while (mob->affected)
      affect_remove(mob, mob->affected);

   free(mob);
   }

void medit_setup_new(struct descriptor_data *d)
   {
   struct char_data *mob;

   /*
    * Allocate a scratch mobile structure
    */
   CREATE(mob, struct char_data, 1);
   init_mobile(mob);

   GET_MOB_RNUM(mob) = -1;
   /*
    * set up some default strings
    */
   GET_ALIAS(mob) = str_dup("mob unfinished");
   GET_SDESC(mob) = str_dup("the unfinished mob");
   GET_LDESC(mob) = str_dup("An unfinished mob stands here.\r\n");
   GET_DDESC(mob) = str_dup("It looks unfinished.\r\n");
#if defined(OASIS_MPROG)

   OLC_MPROGL(d) = NULL;
   OLC_MPROG(d) = NULL;
#endif

   OLC_MOB(d) = mob;
   OLC_VAL(d) = 0;   /*. Has changed flag .*/
   OLC_FUNCN(d)= NUM_SPECS+1;
   OLC_ITEM_TYPE(d) = MOB_TRIGGER;
   medit_disp_menu(d);
   }

/*-------------------------------------------------------------------*/

void medit_setup_existing(struct descriptor_data *d, int rmob_num)
   {
   struct char_data *mob;
#if defined(OASIS_MPROG)

   MPROG_DATA *temp;
   MPROG_DATA *head;
#endif

   /*
    * Allocate a scratch mobile structure.
    */
   CREATE(mob, struct char_data, 1);
   OLC_FUNCN(d)=get_mob_spec_num(mob_index[rmob_num].func);
   copy_mobile(mob, mob_proto + rmob_num);
#if defined(OASIS_MPROG)
   /*
    * I think there needs to be a brace from the if statement to the #endif
    * according to the way the original patch was indented.  If this crashes,
    * try it with the braces and report to greerga@van.ml.org on if that works.
    */
   if (GET_MPROG(mob))
      {
      CREATE(OLC_MPROGL(d), MPROG_DATA, 1);
      head = OLC_MPROGL(d);
      for (temp = GET_MPROG(mob); temp;temp = temp->next)
         {
         OLC_MPROGL(d)->type = temp->type;
         OLC_MPROGL(d)->arglist = str_dup(temp->arglist);
         OLC_MPROGL(d)->comlist = str_dup(temp->comlist);
         if (temp->next)
            {
            CREATE(OLC_MPROGL(d)->next, MPROG_DATA, 1);
            OLC_MPROGL(d) = OLC_MPROGL(d)->next;
            }
         }
      OLC_MPROGL(d) = head;
      OLC_MPROG(d) = OLC_MPROGL(d);
      }
#endif


   OLC_MOB(d) = mob;
   OLC_ITEM_TYPE(d) = MOB_TRIGGER;
   dg_olc_script_copy(d);
   medit_disp_menu(d);
   }

/*-------------------------------------------------------------------*/
/*. Copy one mob struct to another .*/

void copy_mobile(struct char_data *tmob, struct char_data *fmob)
   {
   struct trig_proto_list *proto, *fproto;
   /*
    *Free up any used strings
    */
   if (GET_ALIAS(tmob))
      free(GET_ALIAS(tmob));
   if (GET_SDESC(tmob))
      free(GET_SDESC(tmob));
   if (GET_LDESC(tmob))
      free(GET_LDESC(tmob));
   if (GET_DDESC(tmob))
      free(GET_DDESC(tmob));

   /* delete the old script list */
   proto = tmob->proto_script;
   while (proto)
      {
      fproto = proto;
      proto = proto->next;
      free(fproto);
      }
   /*
    * Copy mob
    */
   *tmob = *fmob;


   /*
    * Reallocate strings
    */
   if (GET_ALIAS(fmob))
      GET_ALIAS(tmob) = str_dup((GET_ALIAS(fmob) && *GET_ALIAS(fmob)) ? GET_ALIAS(fmob) : "undefined");

   if (GET_SDESC(fmob))
      GET_SDESC(tmob) = str_dup((GET_SDESC(fmob) && *GET_SDESC(fmob)) ? GET_SDESC(fmob) : "undefined");

   if (GET_LDESC(fmob))
      GET_LDESC(tmob) = str_dup((GET_LDESC(fmob) && *GET_LDESC(fmob)) ? GET_LDESC(fmob) : "undefined\r\n");


   if (GET_DDESC(fmob))
      GET_DDESC(tmob) = str_dup((GET_DDESC(fmob) && *GET_DDESC(fmob)) ? GET_DDESC(fmob) : "undefined\r\n");

   /* copy the new script list */
   if (fmob->proto_script)
      {
      fproto = fmob->proto_script;
      CREATE(proto, struct trig_proto_list, 1);
      tmob->proto_script = proto;
      do
         {
         proto->vnum = fproto->vnum;
         fproto = fproto->next;
         if (fproto)
            {
            CREATE(proto->next, struct trig_proto_list, 1);
            proto = proto->next;
            }
         }
      while (fproto)
         ;
      }
   }


/*-------------------------------------------------------------------*/
/*. Ideally, this function should be in db.c, but I'll put it here for
  portability.*/

void init_mobile(struct char_data *mob)
   {
   clear_char(mob);
   GET_LEVEL(mob)=1;
   GET_HIT(mob) = 1;
   GET_MANA(mob) = 1;
   GET_MAX_MANA(mob) = 100;
   GET_MAX_MOVE(mob) = 100;
   GET_NDD(mob) = 1;
   GET_SDD(mob) = 1;
   GET_WEIGHT(mob) = 200;
   GET_HEIGHT(mob) = 198;
   GET_CLASS(mob) =0;
   GET_RACE(mob)=0;
   IMMUNE(mob)=0;
   RESIST(mob)=0;
   SUCCEPT(mob)=0;

   mob->real_abils.str = 11;
   mob->real_abils.intel = 11;
   mob->real_abils.wis = 11;
   mob->real_abils.dex = 11;
   mob->real_abils.con = 11;
   mob->real_abils.cha = 11;
   mob->aff_abils = mob->real_abils;

   SET_BIT(MOB_FLAGS(mob), MOB_ISNPC);
   mob->player_specials = &dummy_mob;
   }

/*-------------------------------------------------------------------*/
/*. Save new/edited mob to memory .*/

#define ZCMD zone_table[zone].cmd[cmd_no]

void medit_save_internally(struct descriptor_data *d)
   {
   int rmob_num, found = 0, new_mob_num = 0, zone, cmd_no, shop;
   int guild_num;
   struct char_data *new_proto;
   struct index_data *new_index;
   struct char_data *live_mob;
   struct descriptor_data *dsc;
   char *buf= get_buffer(256);

   /* put the script into proper position */
   OLC_MOB(d)->proto_script = OLC_SCRIPT(d);

   /*
    * Mob exists? Just update it
    */
   if ((rmob_num = real_mobile(OLC_NUM(d)))  != -1)
      {
      OLC_MOB(d)->proto_script = OLC_SCRIPT(d);
      copy_mobile((mob_proto + rmob_num), OLC_MOB(d));
      mob_index[rmob_num].func=mob_specs[OLC_FUNCN(d)].func;
      /*
       * Update live mobiles
       */
      for(live_mob = character_list; live_mob; live_mob = live_mob->next)
         if(IS_MOB(live_mob) && GET_MOB_RNUM(live_mob) == rmob_num)
            {
            /*
             * Only really need update the strings, since these can cause
             * protection faults.  The rest can wait till a reset/reboot
             */
            GET_ALIAS(live_mob) = GET_ALIAS(mob_proto + rmob_num);
            GET_SDESC(live_mob) = GET_SDESC(mob_proto + rmob_num);
            GET_LDESC(live_mob) = GET_LDESC(mob_proto + rmob_num);
            GET_DDESC(live_mob) = GET_DDESC(mob_proto + rmob_num);
            }
      }
   /*
    * Mob does not exist, we have to add it
    */
   else
      {
#if defined(DEBUG)
      log("top_of_mobt: %d, new top_of_mobt: %d\n",top_of_mobt,top_of_mobt+1);
#endif

      CREATE(new_proto, struct char_data, top_of_mobt + 2);
      CREATE(new_index, struct index_data, top_of_mobt + 2);

      for (rmob_num = 0; rmob_num <= top_of_mobt; rmob_num++)
         {
         if (!found)
            {
            /*
             * Is this the place?
             */
            if ((mob_index[rmob_num].vnum > OLC_NUM(d)))
               {
               /*
                * Yep, stick it here
                */
               found = TRUE;
#if defined(DEBUG)

               log("Inserted: rmob_num: %d\n", rmob_num);
#endif

               new_index[rmob_num].vnum = OLC_NUM(d);
               new_index[rmob_num].number = 0;
               new_index[rmob_num].func = mob_specs[OLC_FUNCN(d)].func;
               new_mob_num = rmob_num;
               GET_MOB_RNUM(OLC_MOB(d)) = rmob_num;
               copy_mobile((new_proto + rmob_num), OLC_MOB(d));
               /*
                * Copy the mob that should be here on top
                */
               new_index[rmob_num + 1] = mob_index[rmob_num];
               new_proto[rmob_num + 1] = mob_proto[rmob_num];
               GET_MOB_RNUM(new_proto + rmob_num + 1) = rmob_num + 1;
               }
            else
               {
               /*
                * Nope, copy over as normal
                */
               new_index[rmob_num] = mob_index[rmob_num];
               new_proto[rmob_num] = mob_proto[rmob_num];
               }
            }
         else
            {
            /*
             * We've already found it, copy the rest over
             */
            new_index[rmob_num + 1] = mob_index[rmob_num];
            new_proto[rmob_num + 1] = mob_proto[rmob_num];
            GET_MOB_RNUM(new_proto + rmob_num + 1) = rmob_num + 1;
            }
         }
#if defined(DEBUG)
      log("rmob_num: %d, top_of_mobt: %d, array size: 0-%d (%d)\n",
          rmob_num, top_of_mobt, top_of_mobt + 1, top_of_mobt + 2);
#endif

      if (!found)
         {
         /*
          * Still not found, must add it to the top of the table
          */
#if defined(DEBUG)
         log("Append.\n");
#endif

         new_index[rmob_num].vnum = OLC_NUM(d);
         new_index[rmob_num].number = 0;
         new_index[rmob_num].func = NULL;
         new_mob_num = rmob_num;
         GET_MOB_RNUM(OLC_MOB(d)) = rmob_num;
         copy_mobile((new_proto + rmob_num), OLC_MOB(d));
         }

      /*
       * Replace tables
       */
#if defined(DEBUG)
      log("Attempted free.\n");
#endif
#if !defined(I_CRASH)

      free(mob_index);
      free(mob_proto);
#endif

      mob_index = new_index;
      mob_proto = new_proto;
      top_of_mobt++;

#if defined(DEBUG)

      fprintf(stderr, "Free ok.\n");
#endif

      /*
       * Update live mobile rnums
       */
      for(live_mob = character_list; live_mob; live_mob = live_mob->next)
         if(GET_MOB_RNUM(live_mob) > new_mob_num)
            GET_MOB_RNUM(live_mob)++;

      /*
       * Update zone table
       */
      for (zone = 0; zone <= top_of_zone_table; zone++)
         for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++)
            if (ZCMD.command == 'M')
               if (ZCMD.arg1 >= new_mob_num)
                  ZCMD.arg1++;

      /*
       * Update shop keepers
       */
      for(shop = 0; shop <= top_shop; shop++)
         if(SHOP_KEEPER(shop) >= new_mob_num)
            SHOP_KEEPER(shop)++;

      /*
       * Update guild masters
       */
      for(guild_num = 0; guild_num <= top_guild; guild_num++)
         if(GM_TRAINER(guild_num) >= new_mob_num)
            GM_TRAINER(guild_num)++;
      /*
       * Update keepers in shops being edited .
       */
      for(dsc = descriptor_list; dsc; dsc = dsc->next)
         if(STATE(dsc) == CON_SEDIT)
            {
            if(S_KEEPER(OLC_SHOP(dsc)) >= new_mob_num)
               S_KEEPER(OLC_SHOP(dsc))++;
            }
         else if(STATE(dsc) == CON_GEDIT)
            {
            if(OLC_GUILD(dsc)->gm >= new_mob_num)
               OLC_GUILD(dsc)->gm++;
            }
         else if(STATE(dsc) == CON_MEDIT)
            {
            if (GET_MOB_RNUM(OLC_MOB(dsc)) >= new_mob_num)
               GET_MOB_RNUM(OLC_MOB(dsc))++;
            }

      }

#if defined(OASIS_MPROG)
   GET_MPROG(OLC_MOB(d)) = OLC_MPROGL(d);
   GET_MPROG_TYPE(OLC_MOB(d)) = (OLC_MPROGL(d) ? OLC_MPROGL(d)->type : 0);
   while (OLC_MPROGL(d))
      {
      GET_MPROG_TYPE(OLC_MOB(d)) |= OLC_MPROGL(d)->type;
      OLC_MPROGL(d) = OLC_MPROGL(d)->next;
      }
#endif
   olc_add_to_save_list(zone_table[OLC_ZNUM(d)].number, OLC_SAVE_MOB);
   free(zone_table[OLC_ZNUM(d)].nameLastMod);
   sprintf(buf,"%s - mob",GET_NAME(d->character));
   zone_table[OLC_ZNUM(d)].nameLastMod = strdup(buf);
   release_buffer(buf);
   zone_table[OLC_ZNUM(d)].dateLastMod = time(0);
   olc_add_to_save_list(zone_table[OLC_ZNUM(d)].number, OLC_SAVE_ZONE);
   }


/*-------------------------------------------------------------------*/
/*. Save ALL mobiles for a zone to their .mob file, mobs are all
  saved in Extended format, regardless of whether they have any
  extended fields.  Thanks to Samedi for ideas on this bit of code.*/

void medit_save_to_disk(int zone_num)
   {
   int i, rmob_num, zone, top;
   FILE *mob_file;
   char *Fname=get_buffer(64);
   struct char_data *mob;
   char *buf1,*buf2,*mflag,*mflag2,*aflag,*ibuf,*rbuf,*sbuf;
   int shop_nr;


#if defined(OASIS_MPROG)

   MPROG_DATA *mob_prog = NULL;
#endif

   zone = zone_table[zone_num].number;
   top = zone_table[zone_num].top;

   sprintf(Fname, "%s/%d.new", MOB_PREFIX, zone);
   if (!(mob_file = fopen(Fname, "w")))
      {
      mudlogf(BRF, LVL_BUILDER, TRUE,"SYSERR: OLC: Cannot open mob file!");
      release_buffer(Fname);
      return;
      }

   /*
    * Seach database for mobs in this zone and save them
    */
   buf1=get_buffer(MAX_STRING_LENGTH);
   buf2=get_buffer(MAX_STRING_LENGTH);
   fprintf(mob_file,"@Version: %d\n",CUR_MOB_VER);

   for(i = zone * 100; i <= top; i++)
      {
      rmob_num = real_mobile(i);

      if(rmob_num != -1)
         {
         if(fprintf(mob_file, "#%d\n", i) < 0)
            {
            mudlogf(BRF, LVL_BUILDER, TRUE,
                    "SYSERR: OLC: Cannot write mob file!\r\n");
            fclose(mob_file);
            release_buffer(buf2);
            release_buffer(buf1);
            release_buffer(Fname);
            return;
            }
         mob = (mob_proto + rmob_num);

         /*
          * Clean up strings
          */
         strcpy(buf1, (GET_LDESC(mob) && *GET_LDESC(mob)) ? GET_LDESC(mob) :
                "undefined\r\n");
         strip_string(buf1);
         strcpy(buf2, (GET_DDESC(mob) && *GET_DDESC(mob)) ? GET_DDESC(mob) :
                "undefined\r\n");
         strip_string(buf2);


         aflag=get_buffer(SMALL_BUFSIZE);
         mflag=get_buffer(SMALL_BUFSIZE);
         mflag2=get_buffer(SMALL_BUFSIZE);
         REMOVE_BIT(MOB_FLAGS(mob),MOB_FLY);
         flagascii_conv(mflag,MOB_FLAGS(mob));
         REMOVE_BIT(AFF_FLAGS(mob),AFF_RAGE|AFF_BLIND|AFF_POISON|AFF_SLEEP);
         flagascii_conv(mflag2,MOB2_FLAGS(mob));
         flagascii_conv(aflag,AFF_FLAGS(mob));
         fprintf(mob_file,
                 "%s~\n"
                 "%s~\n"
                 "%s~\n"
                 "%s~\n"
                 "%s %s %s %i E %d %d\n"
                 "%d %d %i %dd%d+%d %dd%d+%d\n"
                 "%ld %ld\n",  /*. Gold & Exp are longs in my mud, ignore any warning .*/
                 (GET_ALIAS(mob) && *GET_ALIAS(mob)) ? GET_ALIAS(mob) :
                 "undefined",
                 (GET_SDESC(mob) && *GET_SDESC(mob)) ? GET_SDESC(mob) :
                 "undefined",
                 buf1,
                 buf2,
                 mflag, mflag2,
                 aflag,
                 GET_ALIGNMENT(mob),
                 GET_CLASS(mob),
                 GET_RACE(mob),
                 GET_LEVEL(mob),
                 20 - GET_HITROLL(mob), /*. Convert hitroll to thac0 .*/
                 GET_AC(mob) / 10,
                 GET_HIT(mob),
                 GET_MANA(mob),
                 GET_MOVE(mob),
                 GET_NDD(mob),
                 GET_SDD(mob),
                 GET_DAMROLL(mob),
                 (long)GET_GOLD(mob),
                 (long)GET_EXP(mob)
                );
         release_buffer(mflag2);
         release_buffer(mflag);
         release_buffer(aflag);
         fprintf(mob_file,"%d %d %d\n", GET_POS(mob), GET_DEFAULT_POS(mob),
                 GET_SEX(mob));

         /*
          * Deal with Extra stats in case they are there
          */
         if(MOB_FLAGGED(mob,MOB_SPEC))
            {
            if(!strcmp(get_mob_spec_name(mob_index[rmob_num].func),
                       "shop_keeper"))
               {
               for (shop_nr = 0; shop_nr < top_shop; shop_nr++)
                  if (SHOP_KEEPER(shop_nr) == mob->nr)
                     break;

               if (shop_nr < top_shop)
                  if (SHOP_FUNC(shop_nr)) /* Check secondary function */
                     fprintf(mob_file,"Special: %s\n",get_mob_spec_name(SHOP_FUNC(shop_nr)));
               }
            else if(!strcmp(get_mob_spec_name(mob_index[rmob_num].func),
                            "guild"))
               {
               /* skip it */
               }
            else
               {
               fprintf(mob_file,"Special: %s\n",get_mob_spec_name(mob_index[rmob_num].func));
               }
            fprintf(mob_file,"SpecVal: 0 %ld %ld %ld %ld %ld %ld %ld %ld %ld\n",
                    GET_MOB_VAL(mob,1), GET_MOB_VAL(mob,2), GET_MOB_VAL(mob,3),
                    GET_MOB_VAL(mob,4), GET_MOB_VAL(mob,5), GET_MOB_VAL(mob,6),
                    GET_MOB_VAL(mob,7), GET_MOB_VAL(mob,8), GET_MOB_VAL(mob,9)
                   );
            }
         if(GET_ATTACK(mob) != 0)
            fprintf(mob_file, "BareHandAttack: %d\n", GET_ATTACK(mob));
         if((mob->mob_specials.skin!=NOTHING) &&
                 (mob->mob_specials.skin!=0))
            {
            fprintf(mob_file, "Skin: %ld\n",mob->mob_specials.skin);
            }
         /*
           if(GET_STR(mob) != 11)
           fprintf(mob_file, "Str: %d\n", GET_STR(mob));
           if(GET_ADD(mob) != 0)
           fprintf(mob_file, "StrAdd: %d\n", GET_ADD(mob));
           if(GET_DEX(mob) != 11)
           fprintf(mob_file, "Dex: %d\n", GET_DEX(mob));
           if(GET_INT(mob) != 11)
           fprintf(mob_file, "Int: %d\n", GET_INT(mob));
           if(GET_WIS(mob) != 11)
           fprintf(mob_file, "Wis: %d\n", GET_WIS(mob));
           if(GET_CON(mob) != 11)
           fprintf(mob_file, "Con: %d\n", GET_CON(mob));
           if(GET_CHA(mob) != 11)
           fprintf(mob_file, "Cha: %d\n", GET_CHA(mob));
           */
         if((IMMUNE(mob)!=0)||(RESIST(mob)!=0)||(SUCCEPT(mob)!=0))
            {
            ibuf=get_buffer(128);
            rbuf=get_buffer(128);
            sbuf=get_buffer(128);
            flagascii_conv(ibuf,IMMUNE(mob));
            flagascii_conv(rbuf,RESIST(mob));
            flagascii_conv(sbuf,SUCCEPT(mob));
            fprintf(mob_file, "Immune: %s %s %s\n", ibuf, rbuf, sbuf);
            release_buffer(sbuf);
            release_buffer(rbuf);
            release_buffer(ibuf);
            }

         /*
          * Add E-mob handlers here
          */

         fprintf(mob_file, "E\n");
         script_save_to_disk(mob_file, mob, MOB_TRIGGER);

#if defined(OASIS_MPROG)
         /*
          * Write out the MobProgs.
          */
         mob_prog = GET_MPROG(mob);
         while(mob_prog)
            {
            strcpy(buf1, mob_prog->arglist);
            strip_string(buf1);
            strcpy(buf2, mob_prog->comlist);
            strip_string(buf2);
            /* The loaded arglist keeps the padding after the type word
               (fread_string skips nothing), so a separator is written only
               when it has none; adding one always grew the padding by a
               space on every save. */
            fprintf(mob_file, isspace((unsigned char) *buf1) ? "%s%s~\n%s"
                    : "%s %s~\n%s", medit_get_mprog_type(mob_prog),
                    buf1, buf2);
            mob_prog = mob_prog->next;
            fprintf(mob_file, "~\n%s", (!mob_prog ? "|\n" : ""));
            }
#endif

         }
      }
   fprintf(mob_file, "$\n");
   fclose(mob_file);
   sprintf(buf2, "%s/%d.mob", MOB_PREFIX, zone);
   /*
    * We're fubar'd if we crash between the two lines below.
    */
   remove(buf2);
   rename(Fname, buf2);

   olc_remove_from_save_list(zone_table[zone_num].number, OLC_SAVE_MOB);
   release_buffer(Fname);
   release_buffer(buf2);
   release_buffer(buf1);
   }

/**************************************************************************
 Menu functions
 **************************************************************************/
/*. Display poistions (sitting, standing etc) .*/

void medit_disp_positions(struct descriptor_data *d)
   {
   int i;
   get_char_cols(d->character);

#if defined(CLEAR_SCREEN)

   send_to_char(d->character, "[H[J");
#endif

   for (i = 1; *position_types[i] != '\n'; i++)
      {
      send_to_char(d->character, "%s%2d%s) %s\r\n", grn, i, nrm, position_types[i]);
      }
   send_to_char(d->character, "Enter position number : ");
   }


/*-------------------------------------------------------------------*/
#if defined(OASIS_MPROG)
/* Get the type of MobProg. */
const char *medit_get_mprog_type(struct mob_prog_data *mprog)
   {
   switch (mprog->type)
      {
   case IN_FILE_PROG:
      return ">in_file_prog";
   case ACT_PROG:
      return ">act_prog";
   case SPEECH_PROG:
      return ">speech_prog";
   case RAND_PROG:
      return ">rand_prog";
   case FIGHT_PROG:
      return ">fight_prog";
   case HITPRCNT_PROG:
      return ">hitprcnt_prog";
   case DEATH_PROG:
      return ">death_prog";
   case ENTRY_PROG:
      return ">entry_prog";
   case GREET_PROG:
      return ">greet_prog";
   case ALL_GREET_PROG:
      return ">all_greet_prog";
   case GIVE_PROG:
      return ">give_prog";
   case BRIBE_PROG:
      return ">bribe_prog";
   default:
      return ">ERROR_PROG";

      }
   return ">ERROR_PROG";
   }

/*-------------------------------------------------------------------*/

/*
 * Display the MobProgs.
 */
void medit_disp_mprog(struct descriptor_data *d)
   {
   struct mob_prog_data *mprog = OLC_MPROGL(d);

   OLC_MTOTAL(d) = 1;

#if defined(CLEAR_SCREEN)

   send_to_char(d->character, "[H[J");
#endif

   while (mprog)
      {
      send_to_char(d->character, "%d) %s %s\r\n", OLC_MTOTAL(d),
                   medit_get_mprog_type(mprog),
                   (mprog->arglist ? mprog->arglist : "NONE"));
      OLC_MTOTAL(d)++;
      mprog = mprog->next;
      }
   send_to_char(d->character, "%d) Create New Mob Prog\r\n"
                "%d) Purge Mob Prog\r\n"
                "Enter number to edit [0 to exit]:  ",
                OLC_MTOTAL(d), OLC_MTOTAL(d) + 1);
   OLC_MODE(d) = MEDIT_MPROG;
   }

/*-------------------------------------------------------------------*/

/*
 * Change the MobProgs.
 */
void medit_change_mprog(struct descriptor_data *d)
   {
#if defined(CLEAR_SCREEN)
   send_to_char(d->character, "[H[J");
#endif

   send_to_char(d->character, "1) Type: %s\r\n"
                "2) Args: %s\r\n"
                "3) Commands:\r\n%s\r\n\r\n"
                "Enter number to edit [0 to exit]: ",
                medit_get_mprog_type(OLC_MPROG(d)),
                (OLC_MPROG(d)->arglist ? OLC_MPROG(d)->arglist: "NONE"),
                (OLC_MPROG(d)->comlist ? OLC_MPROG(d)->comlist : "NONE"));

   OLC_MODE(d) = MEDIT_CHANGE_MPROG;
   }

/*-------------------------------------------------------------------*/

/*
 * Change the MobProg type.
 */
void medit_disp_mprog_types(struct descriptor_data *d)
   {
   int i;

   get_char_cols(d->character);
#if defined(CLEAR_SCREEN)

   send_to_char(d->character, "[H[J");
#endif

   for (i = 0; i < NUM_PROGS; i++)
      {
      send_to_char(d->character, "%s%2d%s) %s\r\n", grn, i, nrm,
                   mobprog_types[i]);
      }
   send_to_char(d->character, "Enter mob prog type : ");
   OLC_MODE(d) = MEDIT_MPROG_TYPE;
   }
#endif




/*-------------------------------------------------------------------*/
/*. Display sex (Oooh-err).*/

void medit_disp_sex(struct descriptor_data *d)
   {
   int i;

   get_char_cols(d->character);

#if defined(CLEAR_SCREEN)

   send_to_char(d->character, "[H[J");
#endif

   for (i = 0; i < NUM_GENDERS; i++)
      {
      send_to_char(d->character, "%s%2d%s) %s\r\n", grn, i, nrm, genders[i]);
      }
   send_to_char(d->character, "Enter gender number : ");
   }

/*-------------------------------------------------------------------*/
/*. Display attack types menu .*/

void medit_disp_attack_types(struct descriptor_data *d)
   {
   int i;

   get_char_cols(d->character);
#if defined(CLEAR_SCREEN)

   send_to_char(d->character, "[H[J");
#endif

   for (i = 0; i < NUM_ATTACK_TYPES; i++)
      {
      send_to_char(d->character, "%s%2d%s) %s\r\n",grn,i,nrm,
                   attack_hit_text[i].singular);
      }
   send_to_char(d->character, "Enter attack type : ");
   }


/*-------------------------------------------------------------------*/
/*. Display mob-flags menu .*/
void medit_disp_mob_flags(struct descriptor_data *d)
   {
   int i, columns = 0;
   char *buf2=get_buffer(SMALL_BUFSIZE);
   char *buf3=get_buffer(SMALL_BUFSIZE);

   get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
   send_to_char(d->character, "[H[J");
#endif

   send_to_char(d->character, "\r\n");
   for (i = 0; i < NUM_MOB_FLAGS; i++)
      {
      send_to_char(d->character, "%s%2d%s) %-18.18s  ",grn, i+1, nrm,
                   action_bits[i]);
      if(!(++columns % 3))
         send_to_char(d->character, "\r\n");
      }
   for (i = 0; i < NUM_MOB2_FLAGS; i++)
      {  
      send_to_char(d->character, "%s%2d%s) %-18.18s  ",grn, i+1+NUM_MOB_FLAGS, nrm,  
                   action2_bits[i]);
      if(!(++columns % 3))
         send_to_char(d->character, "\r\n");
      }
   sprintbit(MOB_FLAGS(OLC_MOB(d)), action_bits, buf2);
   sprintbit(MOB2_FLAGS(OLC_MOB(d)), action2_bits, buf3);
   send_to_char(d->character, "\r\n"
                "Current flags : %s%s%s%s\r\n"
                "Enter mob flags (0 to quit) : ",
                cyn, !strcmp(buf2, "NOBITS ") &&
                      strcmp(buf3, "NOBITS ")?"":buf2, 
                !strcmp(buf3, "NOBITS ")?"":buf3, nrm );
   release_buffer(buf3);
   release_buffer(buf2);
   }


/*-------------------------------------------------------------------*/
/*. Display mob-flags menu .*/
void medit_disp_immun_flags(struct descriptor_data *d)
   {
   int i, columns = 0;
   char *buf2=get_buffer(SMALL_BUFSIZE);
   int flag;

   if(OLC_MODE(d)==MEDIT_IMMUNE)
      flag=IMMUNE(OLC_MOB(d));
   else if(OLC_MODE(d)==MEDIT_RESIST)
      flag=RESIST(OLC_MOB(d));
   else if(OLC_MODE(d)==MEDIT_SUCCEPT)
      flag=SUCCEPT(OLC_MOB(d));

   get_char_cols(d->character);
#if defined(CLEAR_SCREEN)

   send_to_char(d->character, "[H[J");
#endif

   for (i = 0; i < NUM_IMMUN_FLAGS; i++)
      {
      send_to_char(d->character, "%s%2d%s) %-20.20s  ",grn, i+1, nrm,
                   immunity_names[i]);
      if(!(++columns % 2))
         send_to_char(d->character, "\r\n");
      }
   sprintbit(flag, immunity_names, buf2);
   send_to_char(d->character, "\r\n"
                "Current flags : %s%s%s\r\n"
                "Enter mob flags (0 to quit) : ",
                cyn, buf2, nrm );
   release_buffer(buf2);
   }

/*-------------------------------------------------------------------*/
/*. Display aff-flags menu .*/

void medit_disp_aff_flags(struct descriptor_data *d)
   {
   int i, columns = 0;
   char *buf2=get_buffer(SMALL_BUFSIZE);

   get_char_cols(d->character);
#if defined(CLEAR_SCREEN)

   send_to_char(d->character, "[H[J");
#endif

   for (i = 0; i < NUM_AFF_FLAGS; i++)
      {
      send_to_char(d->character, "%s%2d%s) %-20.20s  ", grn, i+1, nrm,
                   affected_bits[i] );
      if(!(++columns % 2))
         send_to_char(d->character, "\r\n");
      }
   sprintbit(AFF_FLAGS(OLC_MOB(d)), affected_bits, buf2);
   send_to_char(d->character, "\r\n"
                "Current flags   : %s%s%s\r\n"
                "Enter aff flags (0 to quit) : ",
                cyn, buf2, nrm );
   release_buffer(buf2);
   }

/*-------------------------------------------------------------------*/
/*. Display specs menu .*/
void medit_disp_specs(struct descriptor_data *d)
   {
   int i,j, columns = 0;

   get_char_cols(d->character);
#if defined(CLEAR_SCREEN)

   send_to_char(d->character, "[H[J");
#endif

   j=0;
   for(i=0;0!=strcmp(mob_specs[i].name,"Unknown, exists");i++)
      {
      send_to_char(d->character, "%s%2d%s) %-20.20s  ",
                   grn,i+1,nrm,mob_specs[i].name);
      if(!(++columns%3))
         send_to_char(d->character, "\r\n");
      }

   send_to_char(d->character, "\r\n"
                "Current Spec   : %s%s%s\r\n"
                "Enter new spec (0 to quit) : ",
                cyn,mob_specs[OLC_FUNCN(d)].name,nrm);
   }

/*-------------------------------------------------------------------*/
/*. Display class menu .*/
void medit_disp_class(struct descriptor_data *d)
   {
   int i;
   int columns=0;

   get_char_cols(d->character);
#if defined(CLEAR_SCREEN)

   send_to_char(d->character, "[H[J");
#endif

   for (i = 0; *npc_class_types[i] != '\n'; i++)
      {
      send_to_char(d->character, "%s%2d%s) %-16.16s", grn,i, nrm,
                   npc_class_types[i]);
      if(!(++columns % 3))
         send_to_char(d->character, "\r\n");
      }
   send_to_char(d->character, "\r\nEnter class number : ");
   }

/*-------------------------------------------------------------------*/
/*. Display main menu .*/
void medit_disp_race(struct descriptor_data *d)
   {
   int i;
   int columns=0;

   get_char_cols(d->character);

#if defined(CLEAR_SCREEN)

   send_to_char(d->character, "[H[J");
#endif

   for (i = 0; *npc_race_types[i] != '\n'; i++)
      {
      send_to_char(d->character, "%s%2d%s) %-16.16s", grn, i, nrm,
                   npc_race_types[i]);
      if(!(++columns % 3))
         send_to_char(d->character, "\r\n");
      }
   send_to_char(d->character, "\r\nEnter race number : ");
   }

void medit_disp_specval_menu(struct descriptor_data *d)
   {
   char *format=NULL;
   get_char_cols(d->character);
#if defined(CLEAR_SCREEN)

   send_to_char(d->character, "[H[J");
#endif

   switch(OLC_FUNCN(d))
      {
   case ARCHER_SP:
      format= "%sN/A%s) %s%d - Always the room the mob starts in.%s\r\n"
              "%s1%s) %s%d - To Hit Percentage%s\r\n"
              "%s2%s) %s%d - Number Dam Dice%s\r\n"
              "%s3%s) %s%d - Size Dam Dice %s\r\n"
              "%s4%s) %s%d - Launch Room 1 (vnum)%s\r\n"
              "%s5%s) %s%d - Launch Room 2 (vnum)%s\r\n"
              "%s6%s) %s%d - Target Room 1 (vnum)%s\r\n"
              "%s7%s) %s%d - Target Room 2 (vnum)%s\r\n"
              "%s8%s) %s%d - Target Room 3 (vnum)%s\r\n"
              "%s9%s) %s%d - not used%s\r\n";
      break;
   case HOMETOWN_SP:
      format= "%sN/A%s) %s%d - Always the room the mob starts in.%s\r\n"
              "%s1%s) %s%d - home town room number.%s\r\n"
              "%s2%s) %s%d - moving cost multiplier. (default:100) (>=LVL_ADMIN)%s \r\n"
              "%s3%s) %s%d - not used%s\r\n"
              "%s4%s) %s%d - not used%s\r\n"
              "%s5%s) %s%d - not used%s\r\n"
              "%s6%s) %s%d - not used%s\r\n"
              "%s7%s) %s%d - not used%s\r\n"
              "%s8%s) %s%d - not used%s\r\n"
              "%s9%s) %s%d - not used%s\r\n";
      break;
   case GUILDGUARD_SP:
      format= "%sN/A%s) %s%d - Always the room the mob starts in.%s\r\n"
              "%s1%s) %s%d - Room to guard from%s\r\n"
              "%s2%s) %s%d - direction to guard%s\r\n"
              "%s3%s) %s%d - allowed class 1%s\r\n"
              "%s4%s) %s%d - allowed class 2%s\r\n"
              "%s5%s) %s%d - allowed class 3%s\r\n"
              "%s6%s) %s%d - allowed class 4%s\r\n"
              "%s7%s) %s%d - allowed class 5%s\r\n"
              "%s8%s) %s%d - allowed class 6%s\r\n"
              "%s9%s) %s%d - not used%s\r\n";
      break;
   case CITYGUARD_SP:
      format= "%sN/A%s) %s%d - Always the room the mob starts in.%s\r\n"
              "%s1%s) %s%d - Assist citizens only?%s\r\n"
              "%s2%s) %s%d - not used%s\r\n"
              "%s3%s) %s%d - not used%s\r\n"
              "%s4%s) %s%d - not used%s\r\n"
              "%s5%s) %s%d - not used%s\r\n"
              "%s6%s) %s%d - not used%s\r\n"
              "%s7%s) %s%d - not used%s\r\n"
              "%s8%s) %s%d - not used%s\r\n"
              "%s9%s) %s%d - not used%s\r\n";
      break;

   case SAGE_SP:
   case RECHARGE_SP:
      format= "%sN/A%s) %s%d - Always the room the mob starts in.%s\r\n"
              "%s1%s) %s%d - Base Cost%s\r\n"
              "%s2%s) %s%d - not used%s\r\n"
              "%s3%s) %s%d - not used%s\r\n"
              "%s4%s) %s%d - not used%s\r\n"
              "%s5%s) %s%d - not used%s\r\n"
              "%s6%s) %s%d - not used%s\r\n"
              "%s7%s) %s%d - not used%s\r\n"
              "%s8%s) %s%d - not used%s\r\n"
              "%s9%s) %s%d - not used%s\r\n";
      break;


   default:
      format= "%sN/A%s) %s%d - Always the room the mob starts in.%s\r\n"
              "%s1%s) %s%d - not used%s\r\n"
              "%s2%s) %s%d - not used%s\r\n"
              "%s3%s) %s%d - not used%s\r\n"
              "%s4%s) %s%d - not used%s\r\n"
              "%s5%s) %s%d - not used%s\r\n"
              "%s6%s) %s%d - not used%s\r\n"
              "%s7%s) %s%d - not used%s\r\n"
              "%s8%s) %s%d - not used%s\r\n"
              "%s9%s) %s%d - not used%s\r\n";
      break;
      }
   send_to_char(d->character,
                format, grn,nrm,cyn,GET_MOB_VAL(OLC_MOB(d),0),nrm,
                grn,nrm,cyn,GET_MOB_VAL(OLC_MOB(d),1),nrm,
                grn,nrm,cyn,GET_MOB_VAL(OLC_MOB(d),2),nrm,
                grn,nrm,cyn,GET_MOB_VAL(OLC_MOB(d),3),nrm,
                grn,nrm,cyn,GET_MOB_VAL(OLC_MOB(d),4),nrm,
                grn,nrm,cyn,GET_MOB_VAL(OLC_MOB(d),5),nrm,
                grn,nrm,cyn,GET_MOB_VAL(OLC_MOB(d),6),nrm,
                grn,nrm,cyn,GET_MOB_VAL(OLC_MOB(d),7),nrm,
                grn,nrm,cyn,GET_MOB_VAL(OLC_MOB(d),8),nrm,
                grn,nrm,cyn,GET_MOB_VAL(OLC_MOB(d),9),nrm
               );

   send_to_char(d->character, "\r\nEnter selection (0 to exit) : ");
   OLC_MODE(d)=MEDIT_SPECVAL;
   }

/*-------------------------------------------------------------------*/
void medit_disp_specval_1(struct descriptor_data *d)
   {
   OLC_MODE(d)=MEDIT_SPECVAL_1;
   switch(OLC_FUNCN(d))
      {
   case ARCHER_SP:
      send_to_char(d->character, "To Hit Percentage = ");
      break;
   case CITYGUARD_SP:
      send_to_char(d->character, "Only Assist Citizens? (1=yes, 0=no) = ");
      break;
   case HOMETOWN_SP:
      send_to_char(d->character, "Vnum of recall room = ");
      break;
   case GUILDGUARD_SP:
      send_to_char(d->character, "Vnum of guard room = ");
      break;
   case SAGE_SP:
      send_to_char(d->character, "Base cost to see all stats = ");
      break;
   case RECHARGE_SP:
      send_to_char(d->character, "Base cost to recharge item(1charge,splvl1,cllvl1) = ");
      break;
   default:
      medit_disp_specval_menu(d);
      break;
      }
   }

/*-------------------------------------------------------------------*/
void medit_disp_specval_2(struct descriptor_data *d)
   {
   OLC_MODE(d)=MEDIT_SPECVAL_2;
   switch(OLC_FUNCN(d))
      {
   case ARCHER_SP:
      send_to_char(d->character, "Num Damage Dice = ");
      break;
   case HOMETOWN_SP:
      if(GET_LEVEL(d->character)<LVL_ADMIN)
         medit_disp_specval_menu(d);
      else
         send_to_char(d->character, "Enter the cost multiplier for (mult * level) for moving = ");
      break;
   case GUILDGUARD_SP:
      send_to_char(d->character, "Direction to guard: \r\n(1-North|2-East|3-South|4-West|5-Up|6-Down) \r\n --> ");
      break;
   default:
      medit_disp_specval_menu(d);
      break;
      }
   }

/*-------------------------------------------------------------------*/
void disp_specval_classes(struct descriptor_data *d)
   {
   int i;
   int columns=0;

   send_to_char(d->character, "Enter a class that is allowed to pass. (0 to remove entry)\r\n");
   for (i = 0; *npc_class_types[i] != '\n'; i++)
      {
      send_to_char(d->character, "%s%2d%s) %-16.16s", grn,i+1, nrm,
                   npc_class_types[i]);
      if(!(++columns % 3))
         send_to_char(d->character, "\r\n");
      }
   send_to_char(d->character, "\r\nYour Choice : ");
   }


/*-------------------------------------------------------------------*/
void medit_disp_specval_3(struct descriptor_data *d)
   {
   OLC_MODE(d)=MEDIT_SPECVAL_3;
   switch(OLC_FUNCN(d))
      {
   case ARCHER_SP:
      send_to_char(d->character, "Size Damage Dice = ");
      break;
   case GUILDGUARD_SP:
      disp_specval_classes(d);
      break;
   default:
      medit_disp_specval_menu(d);
      break;
      }
   }

/*-------------------------------------------------------------------*/
void medit_disp_specval_4(struct descriptor_data *d)
   {
   OLC_MODE(d)=MEDIT_SPECVAL_4;
   switch(OLC_FUNCN(d))
      {
   case ARCHER_SP:
      send_to_char(d->character, "Launch Room 1 (vnum) = ");
      break;
   case GUILDGUARD_SP:
      disp_specval_classes(d);
      break;
   default:
      medit_disp_specval_menu(d);
      break;
      }
   }

/*-------------------------------------------------------------------*/
void medit_disp_specval_5(struct descriptor_data *d)
   {
   OLC_MODE(d)=MEDIT_SPECVAL_5;
   switch(OLC_FUNCN(d))
      {
   case ARCHER_SP:
      send_to_char(d->character, "Launch Room 2 (vnum) = ");
      break;
   case GUILDGUARD_SP:
      disp_specval_classes(d);
      break;
   default:
      medit_disp_specval_menu(d);
      break;
      }
   }

/*-------------------------------------------------------------------*/
void medit_disp_specval_6(struct descriptor_data *d)
   {
   OLC_MODE(d)=MEDIT_SPECVAL_6;
   switch(OLC_FUNCN(d))
      {
   case ARCHER_SP:
      send_to_char(d->character, "Target Room 1 (vnum) = ");
      break;
   case GUILDGUARD_SP:
      disp_specval_classes(d);
      break;
   default:
      medit_disp_specval_menu(d);
      break;
      }
   }

/*-------------------------------------------------------------------*/
void medit_disp_specval_7(struct descriptor_data *d)
   {
   OLC_MODE(d)=MEDIT_SPECVAL_7;
   switch(OLC_FUNCN(d))
      {
   case ARCHER_SP:
      send_to_char(d->character, "Target Room 2 (vnum) = ");
      break;
   case GUILDGUARD_SP:
      disp_specval_classes(d);
      break;
   default:
      medit_disp_specval_menu(d);
      break;
      }
   }

/*-------------------------------------------------------------------*/
void medit_disp_specval_8(struct descriptor_data *d)
   {
   OLC_MODE(d)=MEDIT_SPECVAL_8;
   switch(OLC_FUNCN(d))
      {
   case ARCHER_SP:
      send_to_char(d->character, "Target Room 3 (vnum) = ");
      break;
   case GUILDGUARD_SP:
      disp_specval_classes(d);
      break;
   default:
      medit_disp_specval_menu(d);
      break;
      }
   }

/*-------------------------------------------------------------------*/
void medit_disp_specval_9(struct descriptor_data *d)
   {
   OLC_MODE(d)=MEDIT_SPECVAL_9;
   switch(OLC_FUNCN(d))
      {
   default:
      medit_disp_specval_menu(d);
      break;
      }
   }



/*-------------------------------------------------------------------*/
/*. Display main menu .*/

void medit_disp_menu(struct descriptor_data * d)
   {
   struct char_data *mob;
   char *buf=get_buffer(MAX_STRING_LENGTH);
   char *buf1=get_buffer(SMALL_BUFSIZE);
   char *buf2=get_buffer(SMALL_BUFSIZE);
   char *buf3=get_buffer(SMALL_BUFSIZE);
   obj_rnum skin=NOTHING;
   mob = OLC_MOB(d);
   get_char_cols(d->character);

#if defined(CLEAR_SCREEN)

   send_to_char(d->character, "[H[J");
#endif

   send_to_char(d->character,
                "-- Mob Number:  [%s%d%s]\r\n"
                "%s1%s) Sex: %s%-7.7s%s          %s2%s) Alias: %s%s\r\n"
                "%s3%s) S-Desc: %s%s\r\n"
                "%s4%s) L-Desc:-\r\n%s%s"
                "%s5%s) D-Desc:-\r\n%s%s"
                "%s6%s) Level:       [%s%4d%s],  %s7%s) Alignment:    [%s%4d%s]\r\n"
                "%s8%s) Hitroll:     [%s%4d%s],  %s9%s) Damroll:      [%s%4d%s]\r\n"
                "%sA%s) NumDamDice:  [%s%4d%s],  %sB%s) SizeDamDice:  [%s%4d%s]\r\n"
                "%sC%s) Num HP Dice: [%s%4d%s],  %sD%s) Size HP Dice: [%s%4d%s],  %sE%s) HP Bonus: [%s%5d%s]\r\n"
                "%sF%s) Armor Class: [%s%4d%s],  %sG%s) Exp:     [%s%9ld%s],  %sH%s) Gold:  [%s%8ld%s]\r\n",
                cyn, OLC_NUM(d), nrm,
                grn, nrm, yel, genders[(int)GET_SEX(mob)], nrm,
                grn, nrm, yel, GET_ALIAS(mob),
                grn, nrm, yel, GET_SDESC(mob),
                grn, nrm, yel, GET_LDESC(mob),
                grn, nrm, yel, GET_DDESC(mob),
                grn, nrm, cyn, GET_LEVEL(mob), nrm,
                grn, nrm, cyn, GET_ALIGNMENT(mob), nrm,
                grn, nrm, cyn, GET_HITROLL(mob), nrm,
                grn, nrm, cyn, GET_DAMROLL(mob), nrm,
                grn, nrm, cyn, GET_NDD(mob), nrm,
                grn, nrm, cyn, GET_SDD(mob), nrm,
                grn, nrm, cyn, GET_HIT(mob), nrm,
                grn, nrm, cyn, GET_MANA(mob), nrm,
                grn, nrm, cyn, GET_MOVE(mob), nrm,
                grn, nrm, cyn, GET_AC(mob), nrm,
                /*. Gold & Exp are longs in my mud, ignore any warnings .*/
                grn, nrm, cyn, (long)GET_EXP(mob), nrm,
                grn, nrm, cyn, (long)GET_GOLD(mob), nrm
               );

   sprintbit(MOB_FLAGS(mob), action_bits, buf1);
   sprintbit(AFF_FLAGS(mob), affected_bits, buf2);
   sprintbit(MOB2_FLAGS(mob), action2_bits, buf3);
   send_to_char(d->character,
                "%sI%s) Position  : %s%s  %sJ%s) Default   : %s%s\r\n"
                "%sK%s) Attack    : %s%s\r\n"
                "%sL%s) NPC Flags : %s%s\r\n"
                  "%s   NPC2 Flags: %s%s\r\n"
                "%sM%s) AFF Flags : %s%s\r\n"
                "%sN%s) SPEC Proc : %s%s %sO%s) Class : %s%s    %sP%s) Race : %s%s\r\n"
                "%sR%s) SPEC Values         %sS%s) Mob Progs : %s%s\r\n"
                "%sT%s) Scripts   : %s%s\r\n",

                grn, nrm, yel, position_types[(int)GET_POS(mob)],
                grn, nrm, yel, position_types[(int)GET_DEFAULT_POS(mob)],
                grn, nrm, yel, attack_hit_text[GET_ATTACK(mob)].singular,
                grn, nrm, cyn, buf1,
                     nrm, cyn, buf3,
                grn, nrm, cyn, buf2,
                grn, nrm, cyn, mob_specs[OLC_FUNCN(d)].name,
                grn, nrm, cyn, npc_class_types[(int)GET_CLASS(mob)],
                grn, nrm, cyn, npc_race_types[(int)GET_RACE(mob)],
                grn, nrm,
                grn, nrm, cyn, (OLC_MPROGL(d) ? "Set." : "Not Set."),
                grn, nrm, cyn, (mob->proto_script ? "Set." : "Not Set.")
               );

   sprintbit(IMMUNE(mob),immunity_names,buf1);
   sprintbit(RESIST(mob),immunity_names,buf2);
   sprintbit(SUCCEPT(mob),immunity_names,buf3);
   send_to_char(d->character,
                "%sU%s) IMM Flags : %s%s\r\n"
                "%sV%s) RES Flags : %s%s\r\n"
                "%sW%s) SUC Flags : %s%s\r\n",
                grn, nrm, cyn, buf1,
                grn, nrm, cyn, buf2,
                grn, nrm, cyn, buf3
               );

   if(mob->mob_specials.skin == NOTHING)
      {
      strcpy(buf1,"Nothing");
      }
   else
      {
      if((skin=real_object(mob->mob_specials.skin))>NOTHING)
         {
         sprintf(buf1,"%s (%ld)",((obj_proto[skin].short_description)?obj_proto[skin].short_description:"Something"),mob->mob_specials.skin);
         }
      else
         {
         mob->mob_specials.skin=NOTHING;
         strcpy(buf1,"Nothing");
         }
      }

   send_to_char(d->character,
                "%sX%s) Skin      : %s%s\r\n"
                "%sQ%s) Quit\r\n"
                "Enter choice : ",
                grn, nrm, cyn, buf1,
                grn, nrm
               );

   OLC_MODE(d) = MEDIT_MAIN_MENU;
   release_buffer(buf3);
   release_buffer(buf2);
   release_buffer(buf1);
   release_buffer(buf);
   }

/**************************************************************************
 *         The GARGANTAUN Event Handler
 **************************************************************************/

void medit_parse(struct descriptor_data * d, char *arg)
   {
   int i;
   int vnumber,tmp;

   if (OLC_MODE(d) > MEDIT_NUMERICAL_RESPONSE)
      {
      if(!*arg || (!isdigit((int)arg[0]) && ((*arg == '-') && (!isdigit((int)arg[1])))))
         {
         send_to_char(d->character, "Field must be numerical, try again : ");
         return;
         }
      }

   switch (OLC_MODE(d))
      {
      /*-------------------------------------------------------------------*/
   case MEDIT_CONFIRM_SAVESTRING:
      /*
       * Ensure mob has MOB_ISNPC set or things will go pair shaped
       */
      SET_BIT(MOB_FLAGS(OLC_MOB(d)), MOB_ISNPC);
      switch (*arg)
         {
      case 'y':
      case 'Y':
         /*
          * Save the mob in memory and to disk
          */
         send_to_char(d->character, "Saving mobile to memory.\r\n");
         medit_save_internally(d);
         mudlogf(CMP, MAX(LVL_BUILDER,GET_INVIS_LEV(d->character)),
                 TRUE, "OLC: %s has edited mob %d",
                 GET_NAME(d->character), OLC_NUM(d));
         /* FALL THROUGH */
      case 'n':
      case 'N':
         cleanup_olc(d, CLEANUP_ALL);
         return;
      default:
         send_to_char(d->character, "Invalid choice!\r\n");
         send_to_char(d->character, "Do you wish to save the mobile? : ");
         return;
         }
      break;

      /*-------------------------------------------------------------------*/
   case MEDIT_MAIN_MENU:
      i = 0;
      switch (*arg)
         {
      case 'q':
      case 'Q':
         if (OLC_VAL(d)) /*. Anything been changed? .*/
            {
            send_to_char(d->character, "Do you wish to save the changes to the mobile? (y/n) : ");
            OLC_MODE(d) = MEDIT_CONFIRM_SAVESTRING;
            }
         else
            cleanup_olc(d, CLEANUP_ALL);
         return;
      case '1':
         OLC_MODE(d) = MEDIT_SEX;
         medit_disp_sex(d);
         return;
      case '2':
         OLC_MODE(d) = MEDIT_ALIAS;
         i--;
         OLC_VAL(d) = 1;
         break;
      case '3':
         OLC_MODE(d) = MEDIT_S_DESC;
         i--;
         OLC_VAL(d) = 1;
         break;
      case '4':
         OLC_MODE(d) = MEDIT_L_DESC;
         i--;
         OLC_VAL(d) = 1;
         break;
      case '5':
         OLC_MODE(d) = MEDIT_D_DESC;
         SEND_TO_Q(d,"Enter mob description: (/s saves /h for help)\r\n\r\n");
         d->backstr = NULL;
         if (OLC_MOB(d)->player.description)
            {
            SEND_TO_Q(d, "%s", OLC_MOB(d)->player.description);
            d->backstr = str_dup(OLC_MOB(d)->player.description);
            }
         d->str = &OLC_MOB(d)->player.description;
         d->max_str = MAX_MOB_DESC;
         d->mail_to = 0;
         OLC_VAL(d) = 1;
         return;
      case '6':
         OLC_MODE(d) = MEDIT_LEVEL;
         i++;
         break;
      case '7':
         OLC_MODE(d) = MEDIT_ALIGNMENT;
         i++;
         break;
      case '8':
         /* LEVEL CHECK!! */
         if(GET_LEVEL(d->character)<MEDIT_ACCESS)
            {
            medit_disp_menu(d);
            send_to_char(d->character, "You don't have permission to edit this!\r\n");
            return;
            }
         OLC_MODE(d) = MEDIT_HITROLL;
         i++;
         break;
      case '9':
         /* LEVEL CHECK!! */
         if(GET_LEVEL(d->character)<MEDIT_ACCESS)
            {
            medit_disp_menu(d);
            send_to_char(d->character, "You don't have permission to edit this!\r\n");
            return;
            }
         OLC_MODE(d) = MEDIT_DAMROLL;
         i++;
         break;
      case 'a':
      case 'A':
         /* LEVEL CHECK!! */
         if(GET_LEVEL(d->character)<MEDIT_ACCESS)
            {
            medit_disp_menu(d);
            send_to_char(d->character, "You don't have permission to edit this!\r\n");
            return;
            }
         OLC_MODE(d) = MEDIT_NDD;
         i++;
         break;
      case 'b':
      case 'B':
         /* LEVEL CHECK!! */
         if(GET_LEVEL(d->character)<MEDIT_ACCESS)
            {
            medit_disp_menu(d);
            send_to_char(d->character, "You don't have permission to edit this!\r\n");
            return;
            }
         OLC_MODE(d) = MEDIT_SDD;
         i++;
         break;
      case 'c':
      case 'C':
         /* LEVEL CHECK!! */
         if(GET_LEVEL(d->character)<MEDIT_ACCESS)
            {
            medit_disp_menu(d);
            send_to_char(d->character, "You don't have permission to edit this!\r\n");
            return;
            }
         OLC_MODE(d) = MEDIT_NUM_HP_DICE;
         i++;
         break;
      case 'd':
      case 'D':
         /* LEVEL CHECK!! */
         if(GET_LEVEL(d->character)<MEDIT_ACCESS)
            {
            medit_disp_menu(d);
            send_to_char(d->character, "You don't have permission to edit this!\r\n");
            return;
            }
         OLC_MODE(d) = MEDIT_SIZE_HP_DICE;
         i++;
         break;
      case 'e':
      case 'E':
         /* LEVEL CHECK!! */
         if(GET_LEVEL(d->character)<MEDIT_ACCESS)
            {
            medit_disp_menu(d);
            send_to_char(d->character, "You don't have permission to edit this!\r\n");
            return;
            }
         OLC_MODE(d) = MEDIT_ADD_HP;
         i++;
         break;
      case 'f':
      case 'F':
         /* LEVEL CHECK!! */
         if(GET_LEVEL(d->character)<MEDIT_ACCESS)
            {
            medit_disp_menu(d);
            send_to_char(d->character, "You don't have permission to edit this!\r\n");
            return;
            }
         OLC_MODE(d) = MEDIT_AC;
         i++;
         break;
      case 'g':
      case 'G':
         /* LEVEL CHECK!! */
         if(GET_LEVEL(d->character)<MEDIT_ACCESS)
            {
            medit_disp_menu(d);
            send_to_char(d->character, "You don't have permission to edit this!\r\n");
            return;
            }
         OLC_MODE(d) = MEDIT_EXP;
         i++;
         break;
      case 'h':
      case 'H':
         /* LEVEL CHECK!! */
         if(GET_LEVEL(d->character)<MEDIT_ACCESS)
            {
            medit_disp_menu(d);
            send_to_char(d->character, "You don't have permission to edit this!\r\n");
            return;
            }
         OLC_MODE(d) = MEDIT_GOLD;
         i++;
         break;
      case 'i':
      case 'I':
         OLC_MODE(d) = MEDIT_POS;
         medit_disp_positions(d);
         return;
      case 'j':
      case 'J':
         OLC_MODE(d) = MEDIT_DEFAULT_POS;
         medit_disp_positions(d);
         return;
      case 'k':
      case 'K':
         OLC_MODE(d) = MEDIT_ATTACK;
         medit_disp_attack_types(d);
         return;
      case 'l':
      case 'L':
         OLC_MODE(d) = MEDIT_NPC_FLAGS;
         medit_disp_mob_flags(d);
         return;
      case 'm':
      case 'M':
         OLC_MODE(d) = MEDIT_AFF_FLAGS;
         medit_disp_aff_flags(d);
         return;
      case 'n':
      case 'N':
         OLC_MODE(d) = MEDIT_SPEC;
         medit_disp_specs(d);
         return;
      case 'o':
      case 'O':
         OLC_MODE(d) = MEDIT_CLASS;
         medit_disp_class(d);
         return;
      case 'p':
      case 'P':
         OLC_MODE(d) = MEDIT_RACE;
         medit_disp_race(d);
         return;
      case 'r':
      case 'R':
         OLC_MODE(d) = MEDIT_SPECVAL;
         medit_disp_specval_menu(d);
         return;
#if defined(OASIS_MPROG)

      case 's':
      case 'S':
         if(!PRF2_FLAGGED(d->character,PRF2_EMPROG))
            {
            medit_disp_menu(d);
            send_to_char(d->character, "You don't have permission to edit MobProgs!\r\n: ");
            return;
            }
         OLC_MODE(d) = MEDIT_MPROG;
         medit_disp_mprog(d);
         return;
#endif

      case 't':
      case 'T':
         if(!PRF2_FLAGGED(d->character,PRF2_DG_ATTACH))
            {
            medit_disp_menu(d);
            send_to_char(d->character, "You don't have permission to attach scripts!\r\n: ");
            return;
            }
         OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_MAIN_MENU;
         dg_script_menu(d);
         return;
      case 'u':
      case 'U':
         OLC_MODE(d) = MEDIT_IMMUNE;
         medit_disp_immun_flags(d);
         return;
      case 'v':
      case 'V':
         OLC_MODE(d) = MEDIT_RESIST;
         medit_disp_immun_flags(d);
         return;
      case 'w':
      case 'W':
         OLC_MODE(d) = MEDIT_SUCCEPT;
         medit_disp_immun_flags(d);
         return;
      case 'x':
      case 'X':
         OLC_MODE(d) = MEDIT_HIDE;
         i++;
         break;

      default:
         medit_disp_menu(d);
         return;
         }
      if (i==1)
         {
         send_to_char(d->character, "\r\nEnter new value : ");
         return;
         }
      if (i==-1)
         {
         send_to_char(d->character, "\r\nEnter new text :\r\n| ");
         return;
         }
      break;

      /*-------------------------------------------------------------------*/
   case OLC_SCRIPT_EDIT:
      if (dg_script_edit_parse(d, arg))
         {
         return;
         }
      break;

      /*-------------------------------------------------------------------*/
   case MEDIT_ALIAS:
      if(GET_ALIAS(OLC_MOB(d)))
         free(GET_ALIAS(OLC_MOB(d)));
      GET_ALIAS(OLC_MOB(d)) = str_dup((arg&&*arg)?arg:"undefined");
      break;
      /*-------------------------------------------------------------------*/
   case MEDIT_S_DESC:
      if(GET_SDESC(OLC_MOB(d)))
         free(GET_SDESC(OLC_MOB(d)));
      GET_SDESC(OLC_MOB(d)) = str_dup((arg&&*arg)?arg:"undefined");
      break;
      /*-------------------------------------------------------------------*/
   case MEDIT_L_DESC:
      if(GET_LDESC(OLC_MOB(d)))
         free(GET_LDESC(OLC_MOB(d)));
      if(arg&&*arg)
         {
         char *buf = get_buffer(MAX_STRING_LENGTH);
         strcpy(buf, arg);
         strcat(buf, "\r\n");
         GET_LDESC(OLC_MOB(d)) = str_dup(buf);
         release_buffer(buf);
         }
      else
         GET_LDESC(OLC_MOB(d)) = str_dup("INVIS\r\n");
      break;
      /*-------------------------------------------------------------------*/
   case MEDIT_D_DESC:
      /*
       * We should never get here.
       */
      cleanup_olc(d, CLEANUP_ALL);
      mudlogf(BRF, LVL_BUILDER, TRUE,
              "SYSERR: OLC: medit_parse(): Reached D_DESC case!");
      send_to_char(d->character, "Oops...\r\n");
      break;
      /*-------------------------------------------------------------------*/
#if defined(OASIS_MPROG)

   case MEDIT_MPROG_COMLIST:
      /*
       * We should never get here, but if we do, bail out.
       */
      cleanup_olc(d, CLEANUP_ALL);
      mudlogf(BRF, LVL_BUILDER, TRUE,
              "SYSERR: OLC: medit_parse(): Reached MPROG_COMLIST case!");
      break;
#endif
      /*-------------------------------------------------------------------*/
   case MEDIT_NPC_FLAGS:
      i = atoi(arg);
      if (i==0)
         break;
      if (!((i < 0) || (i > NUM_MOB_FLAGS+NUM_MOB2_FLAGS)))
         {
         if (i <= NUM_MOB_FLAGS)
            {
            i = 1 << (i - 1);
            if (IS_SET(MOB_FLAGS(OLC_MOB(d)), i))
               REMOVE_BIT(MOB_FLAGS(OLC_MOB(d)), i);
            else
               SET_BIT(MOB_FLAGS(OLC_MOB(d)), i);
            }
         else
            {
            i = 1 << (i - NUM_MOB_FLAGS - 1);
            if (IS_SET(MOB2_FLAGS(OLC_MOB(d)), i))   
               REMOVE_BIT(MOB2_FLAGS(OLC_MOB(d)), i);
            else
               SET_BIT(MOB2_FLAGS(OLC_MOB(d)), i);
            }
         }
      medit_disp_mob_flags(d);
      return;
      /*-------------------------------------------------------------------*/
   case MEDIT_IMMUNE:
      i = atoi(arg);
      if (i==0)
         break;
      if (!((i < 0) || (i > NUM_IMMUN_FLAGS)))
         {
         i = 1 << (i - 1);
         if (IS_SET(IMMUNE(OLC_MOB(d)), i))
            REMOVE_BIT(IMMUNE(OLC_MOB(d)), i);
         else
            SET_BIT(IMMUNE(OLC_MOB(d)), i);
         }
      medit_disp_immun_flags(d);
      return;
      /*-------------------------------------------------------------------*/
   case MEDIT_RESIST:
      i = atoi(arg);
      if (i==0)
         break;
      if (!((i < 0) || (i > NUM_IMMUN_FLAGS)))
         {
         i = 1 << (i - 1);
         if (IS_SET(RESIST(OLC_MOB(d)), i))
            REMOVE_BIT(RESIST(OLC_MOB(d)), i);
         else
            SET_BIT(RESIST(OLC_MOB(d)), i);
         }
      medit_disp_immun_flags(d);
      return;
      /*-------------------------------------------------------------------*/
   case MEDIT_SUCCEPT:
      i = atoi(arg);
      if (i==0)
         break;
      if (!((i < 0) || (i > NUM_IMMUN_FLAGS)))
         {
         i = 1 << (i - 1);
         if (IS_SET(SUCCEPT(OLC_MOB(d)), i))
            REMOVE_BIT(SUCCEPT(OLC_MOB(d)), i);
         else
            SET_BIT(SUCCEPT(OLC_MOB(d)), i);
         }
      medit_disp_immun_flags(d);
      return;
      /*-------------------------------------------------------------------*/
   case MEDIT_AFF_FLAGS:
      i = atoi(arg);
      if (i==0)
         break;
      if (!((i < 0) || (i > NUM_AFF_FLAGS)))
         {
         i = 1 << (i - 1);
         if (IS_SET(AFF_FLAGS(OLC_MOB(d)), i))
            REMOVE_BIT(AFF_FLAGS(OLC_MOB(d)), i);
         else
            SET_BIT(AFF_FLAGS(OLC_MOB(d)), i);
         }
      medit_disp_aff_flags(d);
      return;

      /*-------------------------------------------------------------------*/
   case MEDIT_SPEC:
      i = atoi(arg);

      if(i<1)  /* have to use 0 to exit */
         break;

      OLC_VAL(d) = 1;
      i--;   /* set to correct value */
      OLC_FUNCN(d)=MAX(0,MIN(NUM_SPECS+1,i));
      if(OLC_FUNCN(d)<(NUM_SPECS+1))
         SET_BIT(MOB_FLAGS(OLC_MOB(d)),MOB_SPEC);
      for(i=0;i<10;i++)
         GET_MOB_VAL(OLC_MOB(d),i)=0;
      break;

      /*-------------------------------------------------------------------*/
   case MEDIT_SPECVAL:
      i = atoi(arg);
      OLC_VAL(d) = 1;
      if(i<1)  /* have to use 0 to exit */
         break;
      switch(i)
         {
      case 1:
         medit_disp_specval_1(d);
         return;
      case 2:
         medit_disp_specval_2(d);
         return;
      case 3:
         medit_disp_specval_3(d);
         return;
      case 4:
         medit_disp_specval_4(d);
         return;
      case 5:
         medit_disp_specval_5(d);
         return;
      case 6:
         medit_disp_specval_6(d);
         return;
      case 7:
         medit_disp_specval_7(d);
         return;
      case 8:
         medit_disp_specval_8(d);
         return;
      case 9:
         medit_disp_specval_9(d);
         return;
         }
      return;





      /*-------------------------------------------------------------------*/
#if defined(OASIS_MPROG)

   case MEDIT_MPROG:
      if ((i = atoi(arg)) == 0)
         medit_disp_menu(d);
      else if (i == OLC_MTOTAL(d))
         {
         struct mob_prog_data *temp;
         CREATE(temp, struct mob_prog_data, 1);
         temp->next = OLC_MPROGL(d);
         temp->type = -1;
         temp->arglist = NULL;
         temp->comlist = NULL;
         OLC_MPROG(d) = temp;
         OLC_MPROGL(d) = temp;
         OLC_MODE(d) = MEDIT_CHANGE_MPROG;
         medit_change_mprog (d);
         }
      else if (i < OLC_MTOTAL(d))
         {
         struct mob_prog_data *temp;
         int x = 1;
         for (temp = OLC_MPROGL(d); temp && x < i; temp = temp->next)
            x++;
         OLC_MPROG(d) = temp;
         OLC_MODE(d) = MEDIT_CHANGE_MPROG;
         medit_change_mprog (d);
         }
      else if (i == OLC_MTOTAL(d) + 1)
         {
         send_to_char(d->character, "Which mob prog do you want to purge? ");
         OLC_MODE(d) = MEDIT_PURGE_MPROG;
         }
      else
         medit_disp_menu(d);
      return;

   case MEDIT_PURGE_MPROG:
      if ((i = atoi(arg)) > 0 && i < OLC_MTOTAL(d))
         {
         struct mob_prog_data *temp;
         int x = 1;

         for (temp = OLC_MPROGL(d); temp && x < i; temp = temp->next)
            x++;
         OLC_MPROG(d) = temp;
         REMOVE_FROM_LIST(OLC_MPROG(d), OLC_MPROGL(d), next);
         free(OLC_MPROG(d)->arglist);
         free(OLC_MPROG(d)->comlist);
         free(OLC_MPROG(d));
         OLC_MPROG(d) = NULL;
         OLC_VAL(d) = 1;
         }
      medit_disp_mprog(d);
      return;

   case MEDIT_CHANGE_MPROG:
      if ((i = atoi(arg)) == 1)
         medit_disp_mprog_types(d);
      else if (i == 2)
         {
         send_to_char(d->character, "Enter new arg list: ");
         OLC_MODE(d) = MEDIT_MPROG_ARGS;
         }
      else if (i == 3)
         {
         send_to_char(d->character, "Enter new mob prog commands:\r\n");
         /*
          * Pass control to modify.c for typing.
          */
         OLC_MODE(d) = MEDIT_MPROG_COMLIST;
         d->backstr = NULL;
         if (OLC_MPROG(d)->comlist)
            {
            SEND_TO_Q(d, "%s", OLC_MPROG(d)->comlist);
            d->backstr = str_dup(OLC_MPROG(d)->comlist);
            }
         d->str = &OLC_MPROG(d)->comlist;
         d->max_str = MAX_STRING_LENGTH;
         d->mail_to = 0;
         OLC_VAL(d) = 1;
         }
      else
         medit_disp_mprog(d);
      return;
#endif

      /*-------------------------------------------------------------------*/
      /*. Numerical responses .*/
#if defined(OASIS_MPROG)

   case MEDIT_MPROG_TYPE:
      OLC_MPROG(d)->type = (1 << MAX(0, MIN(atoi(arg), NUM_PROGS-1)));
      OLC_VAL(d) = 1;
      medit_change_mprog(d);
      return;

   case MEDIT_MPROG_ARGS:
      OLC_MPROG(d)->arglist = str_dup(arg);
      OLC_VAL(d) = 1;
      medit_change_mprog(d);
      return;
#endif

   case MEDIT_SEX:
      GET_SEX(OLC_MOB(d)) = MAX(0, MIN(NUM_GENDERS -1, atoi(arg)));
      break;

   case MEDIT_HITROLL:
      GET_HITROLL(OLC_MOB(d)) = MAX(0, MIN(50, atoi(arg)));
      break;

   case MEDIT_DAMROLL:
      GET_DAMROLL(OLC_MOB(d)) = MAX(0, MIN(50, atoi(arg)));
      break;

   case MEDIT_NDD:
      GET_NDD(OLC_MOB(d)) = MAX(0, MIN(30, atoi(arg)));
      break;

   case MEDIT_SDD:
      GET_SDD(OLC_MOB(d)) = MAX(0, MIN(127, atoi(arg)));
      break;

   case MEDIT_NUM_HP_DICE:
      GET_HIT(OLC_MOB(d)) = MAX(0, MIN(30, atoi(arg)));
      break;

   case MEDIT_SIZE_HP_DICE:
      GET_MANA(OLC_MOB(d)) = MAX(0, MIN(1000, atoi(arg)));
      break;

   case MEDIT_ADD_HP:
      GET_MOVE(OLC_MOB(d)) = MAX(0, MIN(30000, atoi(arg)));
      break;

   case MEDIT_AC:
      GET_AC(OLC_MOB(d)) = MAX(-200, MIN(200, atoi(arg)));
      break;

   case MEDIT_EXP:
      GET_EXP(OLC_MOB(d)) = MAX(0, atoi(arg));
      break;

   case MEDIT_GOLD:
      GET_GOLD(OLC_MOB(d)) = MAX(0, atoi(arg));
      break;

   case MEDIT_POS:
      GET_POS(OLC_MOB(d)) = MAX(1, MIN(NUM_POSITIONS-1, atoi(arg)));
      break;

   case MEDIT_DEFAULT_POS:
      GET_DEFAULT_POS(OLC_MOB(d)) = MAX(1, MIN(NUM_POSITIONS-1, atoi(arg)));
      break;

   case MEDIT_ATTACK:
      GET_ATTACK(OLC_MOB(d)) = MAX(0, MIN(NUM_ATTACK_TYPES-1, atoi(arg)));
      break;

   case MEDIT_LEVEL:
      if(GET_LEVEL(d->character)<LVL_ADMIN)
      {
#ifdef PLAYERS_PORT
         GET_LEVEL(OLC_MOB(d)) = MAX(1, MIN(LVL_IMPL, atoi(arg)));
#else
         GET_LEVEL(OLC_MOB(d)) = MAX(1, MIN(LVL_IMMORT-1, atoi(arg)));
#endif
      }
      else
         GET_LEVEL(OLC_MOB(d)) = MAX(1, MIN(LVL_IMPL, atoi(arg)));
      /* FIX - SET MOB TO DEFAULTS */
      justify_mob(OLC_MOB(d));
      break;

   case MEDIT_ALIGNMENT:
      GET_ALIGNMENT(OLC_MOB(d)) = MAX(-1000, MIN(1000, atoi(arg)));
      break;

   case MEDIT_CLASS:
      GET_CLASS(OLC_MOB(d)) = MAX(0, MIN(NUM_MOB_CLASSES-1, atoi(arg)));
      break;

   case MEDIT_RACE:
      vnumber=atoi(arg);
      for(tmp=0;(tmp<vnumber)&&(*npc_race_types[tmp]!='\n');tmp++)
         ;
      if((vnumber<0)||(*npc_race_types[tmp]=='\n'))
         {
         send_to_char(d->character, "That's not a valid choice!!\r\n");
         medit_disp_race(d);
         return;
         }
      GET_RACE(OLC_MOB(d)) = vnumber;
      IMMUNE(OLC_MOB(d))=0;
      RESIST(OLC_MOB(d))=0;
      SUCCEPT(OLC_MOB(d))=0;
      SET_BIT(IMMUNE(OLC_MOB(d)),trait_info[GET_RACE(OLC_MOB(d))].immune);
      SET_BIT(RESIST(OLC_MOB(d)),trait_info[GET_RACE(OLC_MOB(d))].resist);
      SET_BIT(SUCCEPT(OLC_MOB(d)),trait_info[GET_RACE(OLC_MOB(d))].susceptible);
      if (vnumber == MRACE_ANIMAL)
         GET_GOLD(OLC_MOB(d)) = 0;
      break;

   case MEDIT_SPECVAL_1:
      GET_MOB_VAL(OLC_MOB(d),1)=atoi(arg);
      medit_disp_specval_menu(d);
      return;
   case MEDIT_SPECVAL_2:
      GET_MOB_VAL(OLC_MOB(d),2)=atoi(arg);
      medit_disp_specval_menu(d);
      return;
   case MEDIT_SPECVAL_3:
      GET_MOB_VAL(OLC_MOB(d),3)=atoi(arg);
      medit_disp_specval_menu(d);
      return;
   case MEDIT_SPECVAL_4:
      GET_MOB_VAL(OLC_MOB(d),4)=atoi(arg);
      medit_disp_specval_menu(d);
      return;
   case MEDIT_SPECVAL_5:
      GET_MOB_VAL(OLC_MOB(d),5)=atoi(arg);
      medit_disp_specval_menu(d);
      return;
   case MEDIT_SPECVAL_6:
      GET_MOB_VAL(OLC_MOB(d),6)=atoi(arg);
      medit_disp_specval_menu(d);
      return;
   case MEDIT_SPECVAL_7:
      GET_MOB_VAL(OLC_MOB(d),7)=atoi(arg);
      medit_disp_specval_menu(d);
      return;
   case MEDIT_SPECVAL_8:
      GET_MOB_VAL(OLC_MOB(d),8)=atoi(arg);
      medit_disp_specval_menu(d);
      return;
   case MEDIT_SPECVAL_9:
      GET_MOB_VAL(OLC_MOB(d),9)=atoi(arg);
      medit_disp_specval_menu(d);
      return;

   case MEDIT_HIDE:
      OLC_MOB(d)->mob_specials.skin =atoi(arg);
      break;
      /*-------------------------------------------------------------------*/
   default:
      /*. We should never get here .*/
      cleanup_olc(d, CLEANUP_ALL);
      mudlogf(BRF,LVL_BUILDER,TRUE,
              "SYSERR: OLC: medit_parse(): Reached default case!");
      send_to_char(d->character, "Oops...tell a H_IMP you got to MEDIT_DEFAULT\r\n");

      break;
      }
   /*-------------------------------------------------------------------*/
   /*. END OF CASE
     If we get here, we have probably changed something, and now want to
     return to main menu.  Use OLC_VAL as a 'has changed' flag .*/

   OLC_VAL(d) = 1;
   medit_disp_menu(d);
   }
/*. End of medit_parse() .*/

