/************************************************************************
 * OasisOLC - oedit.c                                           v1.5    *
 * Copyright 1996 Harvey Gilpin.                                        *
 * Original author: Levork                                              *
 ************************************************************************/

#include "../localHeader/conf.h" 
#include "../localHeader/sysdep.h" 
#include "structs.h" 
#include "comm.h" 
#include "spells.h" 
#include "buffer.h"
#include "utils.h" 
#include "db.h" 
#include "boards.h" 
#include "shop.h" 
#include "olc.h" 
#include "dg_olc.h" 
#include "constants.h"
/*------------------------------------------------------------------------*/ 
/* external variables */ 
 
extern struct obj_data *obj_proto; 
extern struct index_data *obj_index; 
extern struct obj_data *object_list; 
extern int top_of_objt; 
extern struct zone_data *zone_table; 
extern int top_of_zone_table; 
extern struct shop_data *shop_index;		  /*. shop.c .*/ 
extern int top_shop;				  /*. shop.c .*/ 
extern struct attack_hit_type attack_hit_text[];  /*. fight.c  .*/ 
extern struct spell_info_type *spells;
extern int spell_sort_info[];
extern struct board_info_type board_info[]; 
extern struct descriptor_data *descriptor_list;	  /*. comm.c .*/ 

/*------------------------------------------------------------------------*/ 
/*. Macros .*/ 
 
#define S_PRODUCT(s, i) ((s)->producing[(i)]) 
 
/*------------------------------------------------------------------------*/ 
 
void oedit_disp_container_flags_menu(struct descriptor_data *d); 
void oedit_disp_extradesc_menu(struct descriptor_data *d); 
void oedit_disp_weapon_menu(struct descriptor_data *d); 
void oedit_disp_val1_menu(struct descriptor_data *d); 
void oedit_disp_val2_menu(struct descriptor_data *d); 
void oedit_disp_val3_menu(struct descriptor_data *d); 
void oedit_disp_val4_menu(struct descriptor_data *d); 
void oedit_disp_val5_menu(struct descriptor_data *d); 
void oedit_disp_type_menu(struct descriptor_data *d); 
void oedit_disp_extra_menu(struct descriptor_data *d); 
void oedit_disp_anti_menu(struct descriptor_data * d);
void oedit_disp_material_menu(struct descriptor_data *d); 
void oedit_disp_wear_menu(struct descriptor_data *d); 
void oedit_disp_spellaff_menu(struct descriptor_data * d);
void oedit_disp_menu(struct descriptor_data *d); 
void oedit_disp_prompt_wpnspl_menu(struct descriptor_data *d);
void oedit_disp_wpnspl_menu(struct descriptor_data *d);
void oedit_disp_immapp_menu(struct descriptor_data *d);
void oedit_disp_positions(struct descriptor_data *d);
void oedit_disp_fuel_types(struct descriptor_data * d);

void oedit_parse(struct descriptor_data *d, char *arg); 
void oedit_disp_spells_menu(struct descriptor_data *d); 
void oedit_liquid_type(struct descriptor_data *d); 
void oedit_setup_new(struct descriptor_data *d); 
void oedit_setup_existing(struct descriptor_data *d, int real_num); 
void oedit_save_to_disk(int zone); 
void oedit_save_internally(struct descriptor_data *d); 
 
/*------------------------------------------------------------------------*\

  Utility and exported functions 
\*------------------------------------------------------------------------*/ 
 
void oedit_setup_new(struct descriptor_data *d) 
{ 
   CREATE (OLC_OBJ(d), struct obj_data, 1); 

   clear_object(OLC_OBJ(d)); 
   OLC_OBJ(d)->name = str_dup("unfinished object"); 
   OLC_OBJ(d)->description = str_dup("An unfinished object is lying here."); 
   OLC_OBJ(d)->short_description = str_dup("an unfinished object"); 
   GET_OBJ_WEAR(OLC_OBJ(d)) = ITEM_WEAR_TAKE; 
   OLC_VAL(d) = 0; 
   OLC_ITEM_TYPE(d) = OBJ_TRIGGER;
   oedit_disp_menu(d); 
} 
/*------------------------------------------------------------------------*/ 
 
void oedit_setup_existing(struct descriptor_data *d, int real_num) 
{ 
   struct extra_descr_data *this, *temp, *temp2; 
   struct obj_data *obj; 
 
  /*
   * allocate object 
   */ 
   CREATE (obj, struct obj_data, 1); 
   clear_object (obj); 
   *obj = obj_proto[real_num]; 
  
  /*
   * copy all strings over 
   */ 
   obj->name = str_dup(obj_proto[real_num].name ? obj_proto[real_num].name : 
		       "undefined");
   obj->short_description = str_dup(obj_proto[real_num].short_description ?
				    obj_proto[real_num].short_description :
				    "undefined");
   obj->description = str_dup(obj_proto[real_num].description ?
			      obj_proto[real_num].description :
			      "\0");
   obj->action_description = (obj_proto[real_num].action_description ?
			      str_dup(obj_proto[real_num].action_description):
			      NULL);

  /*
   * Extra descriptions if necessary.
   */ 
   if (obj_proto[real_num].ex_description) 
      { 
     /* 
      * temp is for obj being edited 
      */ 
      CREATE (temp, struct extra_descr_data, 1); 
      obj->ex_description = temp; 
      for (this = obj_proto[real_num].ex_description; this; this = this->next) 
	 { 
	 temp->keyword = (this->keyword && *this->keyword) ? 
			 str_dup(this->keyword) : NULL;
	 temp->description = (this->description && *this->description) ?
			     str_dup(this->description) : NULL;
	 if (this->next) 
	    { 
	    CREATE (temp2, struct extra_descr_data, 1); 
	    temp->next = temp2; 
	    temp = temp2; 
	    } 
	 else 
	    temp->next = NULL; 
	 } 
      } 
   if (SCRIPT(&obj_proto[real_num])) {
	  CREATE(SCRIPT(obj), struct script_data, 1);
      script_copy(obj, &obj_proto[real_num], OBJ_TRIGGER);
   }
 
  /*
   * Attatch new obj to players descriptor 
   */ 
   OLC_OBJ(d) = obj; 
   OLC_VAL(d) = 0; 
   OLC_ITEM_TYPE(d) = OBJ_TRIGGER;
   dg_olc_script_copy(d);
   oedit_disp_menu(d); 
} 

/*------------------------------------------------------------------------*/ 
 
#define ZCMD zone_table[zone].cmd[cmd_no] 
 
void oedit_save_internally(struct descriptor_data *d) 
{ 
   int i, shop, robj_num, found = FALSE, zone, cmd_no; 
   struct extra_descr_data *this, *next_one; 
   struct obj_data *obj, *swap, *new_obj_proto; 
   struct index_data *new_obj_index; 
   struct descriptor_data *dsc; 
   char *buf=get_buffer(256);
  /*
   * write to internal tables 
   */ 
   robj_num = real_object(OLC_NUM(d)); 
   if (robj_num > 0) 
      { 
     /*
      * we need to run through each and every object currently in the 
      * game to see which ones are pointing to this prototype 
      * if object is pointing to this prototype, then we need to replace 
      * with the new one 
      */ 
      CREATE(swap, struct obj_data, 1); 
      for (obj = object_list; obj; obj = obj->next) 
	 { 
	 if (obj->item_number == robj_num) 
	    { 
	    *swap = *obj; 
	    *obj = *OLC_OBJ(d); 
	   /* copy game-time dependent vars over */ 
	    IN_ROOM(obj)= IN_ROOM(swap);
	    obj->item_number = robj_num; 
	    obj->carried_by = swap->carried_by; 
	    obj->worn_by = swap->worn_by; 
	    obj->worn_on = swap->worn_on; 
	    obj->in_obj = swap->in_obj; 
	    obj->contains = swap->contains; 
	    obj->next_content = swap->next_content; 
	    obj->next = swap->next; 
	    } 
	 } 
      free_obj(swap); 
     /* now safe to free old proto and write over */ 
      if (obj_proto[robj_num].name) 
	 free(obj_proto[robj_num].name); 
      if (obj_proto[robj_num].description) 
	 free(obj_proto[robj_num].description); 
      if (obj_proto[robj_num].short_description) 
	 free(obj_proto[robj_num].short_description); 
      if (obj_proto[robj_num].action_description) 
	 free(obj_proto[robj_num].action_description); 
      if (obj_proto[robj_num].ex_description) 
	 for (this = obj_proto[robj_num].ex_description; 
	      this; this = next_one)  
	    { 
	    next_one = this->next; 
	    if (this->keyword) 
	       free(this->keyword); 
	    if (this->description) 
	       free(this->description); 
	    free(this); 
	    } 
      obj_proto[robj_num] = *OLC_OBJ(d); 
      obj_proto[robj_num].item_number = robj_num; 
      obj_proto[robj_num].proto_script = OLC_SCRIPT(d);
      } 
   else 
      { 
     /*. It's a new object, we must build new tables to contain it .*/ 
 
      CREATE(new_obj_index, struct index_data, top_of_objt + 2); 
      CREATE(new_obj_proto, struct obj_data, top_of_objt + 2); 
     /* start counting through both tables */ 
      for (i = 0; i <= top_of_objt; i++) 
	 { 
	/* if we haven't found it */ 
	 if (!found) 
	    { 
	   /* check if current virtual is bigger than our virtual */ 
	    if (obj_index[i].vnum > OLC_NUM(d))  
	       { 
	       found = TRUE; 
	       robj_num = i; 
	       OLC_OBJ(d)->item_number = robj_num; 
	       new_obj_index[robj_num].vnum = OLC_NUM(d); 
	       new_obj_index[robj_num].number = 0; 
	       new_obj_index[robj_num].func = NULL; 
	       new_obj_proto[robj_num] = *(OLC_OBJ(d)); 
	       new_obj_proto[robj_num].proto_script = OLC_SCRIPT(d);
	       new_obj_proto[robj_num].in_room = NOWHERE; 
	      /*. Copy over the mob that should be here .*/ 
	       new_obj_index[robj_num + 1] = obj_index[robj_num]; 
	       new_obj_proto[robj_num + 1] = obj_proto[robj_num]; 
	       new_obj_proto[robj_num + 1].item_number = robj_num + 1; 
	       } 
	    else 
	       { 
	      /* just copy from old to new, no num change */ 
	       new_obj_proto[i] = obj_proto[i]; 
	       new_obj_index[i] = obj_index[i]; 
	       } 
	    } 
	 else 
	    { 
	   /* we HAVE already found it.. therefore copy to object + 1 */ 
	    new_obj_index[i + 1] = obj_index[i]; 
	    new_obj_proto[i + 1] = obj_proto[i]; 
	    new_obj_proto[i + 1].item_number = i + 1; 
	    } 
	 } 
      if (!found) 
	 { 
	 robj_num = i; 
	 OLC_OBJ(d)->item_number = robj_num; 
	 new_obj_index[robj_num].vnum = OLC_NUM(d); 
	 new_obj_index[robj_num].number = 0; 
	 new_obj_index[robj_num].func = NULL; 
	 new_obj_proto[robj_num] = *(OLC_OBJ(d)); 
	 new_obj_proto[robj_num].proto_script = OLC_SCRIPT(d);
	 new_obj_proto[robj_num].in_room = NOWHERE; 
	 } 
 
     /* free and replace old tables */ 
      free (obj_proto); 
      free (obj_index); 
      obj_proto = new_obj_proto; 
      obj_index = new_obj_index; 
      top_of_objt++; 
 
     /*. Renumber live objects .*/ 
      for (obj = object_list; obj; obj = obj->next) 
	 if (GET_OBJ_RNUM (obj) >= robj_num) 
	    GET_OBJ_RNUM (obj)++; 
 
     /*. Renumber zone table .*/ 
      for (zone = 0; zone <= top_of_zone_table; zone++) 
	 for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++) 
	    switch (ZCMD.command) 
	       { 
		case 'P': 
		   if(ZCMD.arg3 >= robj_num) 
		      ZCMD.arg3++; 
		  /*. No break here - drop into next case .*/ 
		case 'O': 
		case 'G': 
		case 'E': 
		   if(ZCMD.arg1 >= robj_num) 
		      ZCMD.arg1++; 
		   break; 
		case 'R': 
		   if(ZCMD.arg2 >= robj_num) 
		      ZCMD.arg2++; 
		   break; 
	       } 
 
     /*. Renumber notice boards */ 
      for (i = 0; i < NUM_OF_BOARDS; i++) 
	 if (BOARD_RNUM(i) >= robj_num) 
	    BOARD_RNUM(i) = BOARD_RNUM(i) + 1; 
 
     /*. Renumber shop produce .*/ 
      for(shop = 0; shop < top_shop; shop++) 
	 for(i = 0; SHOP_PRODUCT(shop, i) != -1; i++) 
	    if (SHOP_PRODUCT(shop, i) >= robj_num) 
	       SHOP_PRODUCT(shop, i)++; 

     /*. Renumber produce in shops being edited .*/ 
      for(dsc = descriptor_list; dsc; dsc = dsc->next) 
	 if(STATE(dsc) == CON_SEDIT) 
	    for(i = 0; S_PRODUCT(OLC_SHOP(dsc), i) != -1; i++) 
	       if (S_PRODUCT(OLC_SHOP(dsc), i) >= robj_num) 
		  S_PRODUCT(OLC_SHOP(dsc), i)++; 
       
      }  
   olc_add_to_save_list(zone_table[OLC_ZNUM(d)].number, OLC_SAVE_OBJ); 
   free(zone_table[OLC_ZNUM(d)].nameLastMod);
   sprintf(buf,"%s - obj",GET_NAME(d->character));
   zone_table[OLC_ZNUM(d)].nameLastMod = strdup(buf);
   release_buffer(buf);
   zone_table[OLC_ZNUM(d)].dateLastMod = time(0);
   olc_add_to_save_list(zone_table[OLC_ZNUM(d)].number, OLC_SAVE_ZONE);
} 
/*------------------------------------------------------------------------*/ 
 
void oedit_save_to_disk(int zone_num) 
{ 
   int counter, counter2, realcounter; 
   int end;
   FILE *fp; 
   struct obj_data *obj; 
   struct extra_descr_data *ex_desc; 
   char *buf=get_buffer(256);
   char *buf1,*eflag,*eflag2,*eflag3,*wflag,*aflag;

   sprintf(buf, "%s/%ld.new", OBJ_PREFIX, zone_table[zone_num].number); 
   if (!(fp = fopen(buf, "w+"))) 
      { 
      mudlogf(BRF, LVL_BUILDER, TRUE,"SYSERR: OLC: Cannot open objects file!");
      release_buffer(buf);
      return; 
      } 
 
   buf1=get_buffer(MAX_STRING_LENGTH);
  /*
   * start running through all objects in this zone 
   */ 
   fprintf(fp,"@Version: %d\n",CUR_OBJ_VER);
   for (counter = zone_table[zone_num].number * 100; 
	counter <= zone_table[zone_num].top; 
	counter++)  
      { 
     /* write object to disk */ 
      realcounter = real_object(counter); 
      if (realcounter >= 0)  
	 { 
	 obj = (obj_proto + realcounter); 
 
	 if (obj->action_description) 
	    { 
	    strcpy(buf1, obj->action_description); 
	    strip_string(buf1); 
	    end = strlen(buf1);
	    if(buf1[end-1]=='\n')
	       buf1[end-1]='\0';
	    } 
	 else 
	    *buf1 = '\0'; 
 
         eflag=get_buffer(SMALL_BUFSIZE);
         eflag2=get_buffer(SMALL_BUFSIZE);
         eflag3=get_buffer(SMALL_BUFSIZE);
         wflag=get_buffer(SMALL_BUFSIZE);
	 aflag=get_buffer(SMALL_BUFSIZE);
	 REMOVE_BIT(GET_OBJ_EXTRA(obj),ITEM_LIVE_GRENADE|ITEM_DONATED);
         flagascii_conv(eflag,GET_OBJ_EXTRA(obj));
         flagascii_conv(eflag2,GET_OBJ_EXTRA2(obj));
         flagascii_conv(eflag3,GET_OBJ_EXTRA3(obj));
         flagascii_conv(wflag,GET_OBJ_WEAR(obj));
         flagascii_conv(aflag,GET_OBJ_ANTI(obj));
	 fprintf(fp,  
		 "#%ld\n" 
		 "%s~\n" 
		 "%s~\n" 
		 "%s~\n" 
		 "%s~\n" 
		 "%d %s %s %s %s %s\n" 
		 "%ld %ld %ld %ld %ld %ld %ld %ld\n" 
		 "%d %d %d %d %d\n", 
 
		 GET_OBJ_VNUM(obj), 
		 (obj->name&&*obj->name) ? obj->name : "undefined", 
		 (obj->short_description&&*obj->short_description) ?
		 obj->short_description : "undefined", 
		 (obj->description&&*obj->description) ?
		 obj->description : "", 
		 buf1, 
		 GET_OBJ_TYPE(obj), 
		 eflag,
                 eflag2,
                 eflag3,
		 wflag,
		 aflag,
		 GET_OBJ_VAL(obj, 0), 
		 GET_OBJ_VAL(obj, 1), 
		 GET_OBJ_VAL(obj, 2), 
		 GET_OBJ_VAL(obj, 3), 
		 GET_OBJ_VAL(obj, 4), 
		 GET_OBJ_VAL(obj, 5), 
		 GET_OBJ_VAL(obj, 6), 
		 GET_OBJ_VAL(obj, 7), 
		 GET_OBJ_WEIGHT(obj), 
		 GET_OBJ_COST(obj), 
		 GET_OBJ_RENT(obj), 
		 GET_OBJ_CSLOTS(obj),
		 GET_OBJ_TSLOTS(obj)
	    ); 
	 release_buffer(aflag);
	 release_buffer(wflag);
         release_buffer(eflag3);
         release_buffer(eflag2);
	 release_buffer(eflag);

	/* 
	 * Ponder (04/02/1997) 
	 * Do we have a material type defined? 
	 */ 
	 if (obj->material != MATERIAL_UNDEFINED) 
	    { 
	    fprintf(fp,   "M\n" 
		    "%d\n", 
		    obj->material); 
	    } 
	 script_save_to_disk(fp, obj, OBJ_TRIGGER);
 
	/* 
	 * Do we have extra descriptions? 
	 */ 
	 if (obj->ex_description) 
	    { 
	   /*
	    * Yep, save them too 
	    */ 
	    for (ex_desc = obj->ex_description; ex_desc; ex_desc = ex_desc->next)  
	       { 
	      /*
	       * Sanity check to prevent nasty protection faults 
	       */ 
	       if (!*ex_desc->keyword || !*ex_desc->description) 
		  { 
		  mudlogf(BRF, LVL_BUILDER, TRUE,
			  "SYSERR: OLC: oedit_save_to_disk: Corrupt ex_desc!");
		  continue; 
		  } 
	       if(str_cmp(ex_desc->keyword,"undefined")==0)
		  continue;
	       
	       strcpy(buf1, ex_desc->description); 
	       strip_string(buf1); 
	       fprintf(fp,   "E\n" 
		       "%s~\n" 
		       "%s~\n", 
		       ex_desc->keyword, 
		       buf1 
		  ); 
	       } 
	    } 
 
	/*
	 * Do we have affects? 
	 */ 
	 for (counter2 = 0; counter2 < MAX_OBJ_AFFECT; counter2++) 
	    if (obj->affected[counter2].modifier)  
	       fprintf(fp, "A\n" 
		       "%d %ld\n",  
		       obj->affected[counter2].location, 
		       obj->affected[counter2].modifier 
		  ); 

         eflag=get_buffer(SMALL_BUFSIZE);
	 flagascii_conv(eflag,obj->obj_flags.bitvector);
	 if(eflag[0]!='0')
	    fprintf(fp, "C\n"
		    "%s\n", eflag);

	 release_buffer(eflag);

	 for(counter2=0; counter2 < MAX_SPELL_AFFECT; counter2++)
	    {
	    if(obj->spell_affect[counter2].spelltype>0)
	       fprintf(fp, "S\n"
		       "%d %d %d\n",
		       obj->spell_affect[counter2].spelltype,
		       obj->spell_affect[counter2].level,
		       obj->spell_affect[counter2].percentage
		  );
	    }
	 } 
      } 
 
  /* 
   * write final line, close 
   */ 
   fprintf(fp, "$~\n"); 
   fclose(fp); 
   sprintf(buf1, "%s/%ld.obj", OBJ_PREFIX, zone_table[zone_num].number);
  /*
   * We're fubar'd if we crash between the two lines below.
   */
   remove(buf1);
   rename(buf, buf1);

   olc_remove_from_save_list(zone_table[zone_num].number, OLC_SAVE_OBJ); 
   release_buffer(buf1);
   release_buffer(buf);
} 
 
/************************************************************************** 
 Menu functions  
 **************************************************************************/ 
 
/* For container flags */ 
void oedit_disp_container_flags_menu(struct descriptor_data * d) 
{ 
   char *buf2=get_buffer(MAX_STRING_LENGTH);

   get_char_cols(d->character); 
   sprintbit(GET_OBJ_VAL(OLC_OBJ(d), 1), container_bits, buf2); 

#if defined(CLEAR_SCREEN)
   send_to_char(d->character, "[H[J"); 
#endif
   send_to_char(d->character,  
	   "%s1%s) CLOSEABLE\r\n" 
	   "%s2%s) PICKPROOF\r\n" 
	   "%s3%s) CLOSED\r\n" 
	   "%s4%s) LOCKED\r\n" 
	   "Container flags: %s%s%s\r\n" 
	   "Enter flag, 0 to quit : ", 
	   grn, nrm, grn, nrm, grn, nrm, grn, nrm, cyn, buf2, nrm 
      ); 
   release_buffer(buf2);
} 
 
/* For extra descriptions */ 
void oedit_disp_extradesc_menu(struct descriptor_data * d) 
{ 
   char *buf2=get_buffer(MAX_STRING_LENGTH);
   struct extra_descr_data *extra_desc = OLC_DESC(d); 
   
   if (!extra_desc->next) 
      strcpy(buf2, "<Not set>\r\n"); 
   else 
      strcpy(buf2, "Set."); 
 
   get_char_cols(d->character); 
#if defined(CLEAR_SCREEN)
   send_to_char(d->character, "[H[J"); 
#endif
   send_to_char(d->character, 
	   "Extra desc menu\r\n" 
	   "%s1%s) Keyword: %s%s\r\n" 
	   "%s2%s) Description:\r\n%s%s\r\n" 
	   "%s3%s) Goto next description: %s\r\n" 
	   "%s0%s) Quit\r\n" 
	   "Enter choice : ", 
 
	   grn, nrm, yel, (extra_desc->keyword&&*extra_desc->keyword) ? 
	   extra_desc->keyword : "<NONE>", 
	   grn, nrm, yel, (extra_desc->description&&*extra_desc->description)
	   ? extra_desc->description : "<NONE>", 
	   grn, nrm, buf2, 
	   grn, nrm 
      ); 
   OLC_MODE(d) = OEDIT_EXTRADESC_MENU; 
   release_buffer(buf2);
} 
 
/* Ask for *which* apply to edit */ 
void oedit_disp_prompt_apply_menu(struct descriptor_data * d) 
{ 
   char *buf2=get_buffer(MAX_STRING_LENGTH);
   int counter; 
 
   get_char_cols(d->character); 
#if defined(CLEAR_SCREEN)
   send_to_char(d->character, "[H[J"); 
#endif
   for (counter = 0; counter < MAX_OBJ_AFFECT; counter++) 
      { 
      if (OLC_OBJ(d)->affected[counter].modifier) 
	 { 
	 if(OLC_OBJ(d)->affected[counter].location==APPLY_IMMUNE)
	    {
	    sprintbit(OLC_OBJ(d)->affected[counter].modifier,
		      immunity_names,buf2);
	    send_to_char(d->character, " %s%d%s) Makes Immune to %s\r\n",grn,
			 counter + 1, nrm, buf2);
	    }
	 else if(OLC_OBJ(d)->affected[counter].location==APPLY_RESIST)
	    {
	    sprintbit(OLC_OBJ(d)->affected[counter].modifier,
		      immunity_names,buf2);
	    send_to_char(d->character, " %s%d%s) Makes Resistant to %s\r\n",
			 grn,counter+1,nrm, buf2);
	    }
	 else if(OLC_OBJ(d)->affected[counter].location==APPLY_SUSC)
	    {
	    sprintbit(OLC_OBJ(d)->affected[counter].modifier,
		      immunity_names,buf2);
	    send_to_char(d->character, " %s%d%s) Makes Susceptable to %s\r\n",
			 grn,counter + 1, nrm,buf2);
	    }
	 else
	    {
	    sprinttype(OLC_OBJ(d)->affected[counter].location,
		       apply_types,buf2);
	    send_to_char(d->character, " %s%d%s) %+ld to %s\r\n", grn, 
			 counter + 1, nrm, 
			 OLC_OBJ(d)->affected[counter].modifier, buf2 ); 
	    }
	 } 
      else 
	 { 
	 send_to_char(d->character, " %s%d%s) None.\r\n", 
		      grn, counter + 1, nrm); 
	 } 
      } 
   send_to_char(d->character, "\r\nEnter affection to modify (0 to quit) : "); 
   OLC_MODE(d) = OEDIT_PROMPT_APPLY; 
   release_buffer(buf2);
} 


/*. display immunities to edit .*/
void oedit_disp_immapp_menu(struct descriptor_data *d)
{
   char *buf2=get_buffer(MAX_STRING_LENGTH);
   char *type;
   int  counter;
   int  columns=0;

   get_char_cols(d->character); 
#if defined(CLEAR_SCREEN)
   send_to_char(d->character, "[H[J"); 
#endif
   for (counter = 0; counter < NUM_IMMUN_FLAGS; counter++) 
      {  
      send_to_char(d->character, "%s%2d%s) %-20.20s  ",grn, counter+1, nrm, 
	      immunity_names[counter]); 
      if(!(++columns % 2)) 
         send_to_char(d->character, "\r\n"); 
      } 

   sprintbit(OLC_OBJ(d)->affected[OLC_VAL(d)].modifier, immunity_names, buf2); 
   if(OLC_OBJ(d)->affected[OLC_VAL(d)].location==APPLY_IMMUNE)
      type = "immune";
   else if(OLC_OBJ(d)->affected[OLC_VAL(d)].location==APPLY_RESIST)
      type = "resist";
   else
      type = "succept";

   send_to_char(d->character, "\r\n" 
           "Current flags : %s%s%s\r\n" 
           "Enter %s flags (0 to quit) : ", 
           cyn, buf2, nrm, type); 
   release_buffer(buf2);
}


/*. Ask for *which* weapons spell to edit .*/
void oedit_disp_prompt_wpnspl_menu(struct descriptor_data *d)
{
   int counter; 

   get_char_cols(d->character); 
#if defined(CLEAR_SCREEN)
   send_to_char(d->character, "[H[J"); 
#endif
   for (counter = 0; counter < MAX_SPELL_AFFECT; counter++) 
      {
      if(OLC_OBJ(d)->spell_affect[counter].spelltype!=0)
	 {
	 send_to_char(d->character, 
		" %s%d%s) Spell: %s%-25.25s%s Lvl: %s%2d%s %%: %s%d%%%s\r\n",
		grn,counter+1,nrm, cyn,
		spells[OLC_OBJ(d)->spell_affect[counter].spelltype].spell_name,
		nrm, cyn,OLC_OBJ(d)->spell_affect[counter].level,nrm,
		cyn,OLC_OBJ(d)->spell_affect[counter].percentage,nrm);
	 }
      else
	 {
	 send_to_char(d->character, " %s%d%s) None.\r\n", 
		      grn, counter + 1, nrm); 
	 } 
      }
   send_to_char(d->character, "\r\nEnter Weapons Spell Slot to modify (0 to quit) : ");
}

/*. Ask for liquid type .*/ 
void oedit_liquid_type(struct descriptor_data * d) 
{ 
   int counter, columns = 0; 
 
   get_char_cols(d->character); 
#if defined(CLEAR_SCREEN)
   send_to_char(d->character, "[H[J"); 
#endif
   for (counter = 0; counter < NUM_LIQ_TYPES; counter++)  
      { 
      send_to_char(d->character, " %s%2d%s) %s%-20.20s ",  
	      grn, counter, nrm, yel, 
	      drinks[counter] 
	 ); 
      if (!(++columns % 2)) 
	 send_to_char(d->character, "\r\n"); 
      } 
   send_to_char(d->character, "\r\n%sEnter drink type : ", nrm); 
   OLC_MODE(d) = OEDIT_VALUE_3; 
} 

/*. Ask for fuel type .*/ 
void oedit_disp_fuel_types(struct descriptor_data * d) 
{ 
   int counter; 
 
   get_char_cols(d->character); 
#if defined(CLEAR_SCREEN)
   send_to_char(d->character, "[H[J"); 
#endif
   for (counter = 1; counter < NUM_FUEL_TYPES+1; counter++)  
      { 
      send_to_char(d->character, " %s%2d%s) %s%-20.20s\r\n",  
	      grn, counter, nrm, yel, 
	      fuels[counter] 
	 ); 
      } 
   send_to_char(d->character, "\r\n%sEnter fuel type : ", nrm); 
} 
 
/*
 * Display positions. (sitting, standing, etc) 
 */
void oedit_disp_positions(struct descriptor_data *d) 
{
   int i;
   get_char_cols(d->character);
   
#if defined(CLEAR_SCREEN)
   send_to_char(d->character, "[H[J"); 
#endif
   for (i = POS_SLEEPING; *position_types[i] != '\n'; i++) 
      {
      send_to_char(d->character, "%s%2d%s) %s\r\n", grn, i, nrm, 
		   position_types[i]); 
      }
   send_to_char(d->character, "Enter minimum position number : ");
}


/* The actual apply to set */ 
void oedit_disp_apply_menu(struct descriptor_data * d) 
{ 
   int counter, columns = 0; 

   get_char_cols(d->character); 
#if defined(CLEAR_SCREEN)
   send_to_char(d->character, "[H[J"); 
#endif
   for (counter = 0; counter < NUM_APPLIES; counter++)  
      { 
      send_to_char(d->character, "%s%2d%s) %-20.20s ", 
	      grn, counter, nrm, apply_types[counter] 
	 ); 
      if (!(++columns % 2)) 
	 send_to_char(d->character, "\r\n"); 
      } 
   send_to_char(d->character, "\r\nEnter apply type (0 is no apply) : "); 
   OLC_MODE(d) = OEDIT_APPLY; 
} 
 
void oedit_disp_wpnspl_menu(struct descriptor_data *d)
{
   int counter;
   int j=0,sortpos;

   get_char_cols(d->character); 
#if defined(CLEAR_SCREEN)
   send_to_char(d->character, "[H[J"); 
#endif
   send_to_char(d->character, "Spells known:\r\n");
   
   for (sortpos = 1; sortpos <= MAX_SPELLS; sortpos++) 
      {
      counter=spell_sort_info[sortpos];
      if(spells[counter].is_spell==IS_SPELL)
         {
         send_to_char(d->character, "%s%3d%s) %-17.17s  ", 
		 cyn,counter,nrm, spells[counter].spell_name);
         if (!(++j % 3))
            send_to_char(d->character, "\r\n");
         }
      }
   
   send_to_char(d->character, "\r\nEnter Spell Number (0 is no spell) : ");
}
 
/* weapon type */ 
void oedit_disp_weapon_menu(struct descriptor_data * d) 
{ 
   int counter, columns = 0; 

   get_char_cols(d->character); 
#if defined(CLEAR_SCREEN)
   send_to_char(d->character, "[H[J"); 
#endif
   for (counter = 1; counter < NUM_ATTACK_TYPES; counter++)  
      { 
      if(sharp[counter]==1)
	 send_to_char(d->character, "%s%2d%s) %-15.15s BaSt ", 
		 grn, counter, nrm, attack_hit_text[counter].singular 
	    ); 
      else
	 send_to_char(d->character, "%s%2d%s) %-20.20s ", 
		 grn, counter, nrm, attack_hit_text[counter].singular 
	    ); 

      if(!(++columns % 2)) 
	 send_to_char(d->character, "\r\n"); 
      } 
   send_to_char(d->character, "\r\nEnter weapon type : "); 
} 
 
/* spell type */ 
void oedit_disp_spells_menu(struct descriptor_data * d) 
{ 
   int counter, columns = 0; 
 
   get_char_cols(d->character); 
#if defined(CLEAR_SCREEN)
   send_to_char(d->character, "[H[J"); 
#endif
   for (counter = 0; counter < MAX_SPELLS; counter++) 
      { 
      if(spells[counter].is_spell==IS_SPELL)
	 {
	 send_to_char(d->character, "%s%2d%s) %s%-20.20s ", 
		 grn, counter, nrm, yel, spells[counter].spell_name ); 
	 if (!(++columns % 3)) 
	    send_to_char(d->character, "\r\n"); 
	 }
      } 
   send_to_char(d->character, "\r\n%sEnter spell choice (-1 for none) : ",nrm);
} 
 
/* object value 1 */ 
void oedit_disp_val1_menu(struct descriptor_data * d) 
{ 
   OLC_MODE(d) = OEDIT_VALUE_1; 
   switch (GET_OBJ_TYPE(OLC_OBJ(d))) 
      { 
       case ITEM_WORN: 
       case ITEM_SHOVEL: 
	 /* values 1 2 and 3 are unused, jump to 4. how odd */ 
	  oedit_disp_val5_menu(d); 
	  break; 
       case ITEM_LIGHT: 
	  send_to_char(d->character, "Refuelable? (1=Yes, 0=No) ");
	  break; 
       case ITEM_FUEL:
	  send_to_char(d->character, "Number of fuel units: ");
	  break;
       case ITEM_BOW:
       case ITEM_CROSSBOW:
       case ITEM_SLING:
	  send_to_char(d->character, "Range (1 - 3) = ");
	  break;
       case ITEM_WEAPON: 
       case ITEM_ROCK:
       case ITEM_BOLT:
       case ITEM_ARROW:
       case ITEM_THROW:
	 /* value 0 not used, jump to 1 */ 
	  oedit_disp_val2_menu(d);
	  break;
       case ITEM_SCROLL: 
       case ITEM_WAND: 
       case ITEM_STAFF: 
       case ITEM_PILL: 
       case ITEM_POTION: 
	  send_to_char(d->character, "Spell level : "); 
	  break; 
       case ITEM_ARMOR: 
	  send_to_char(d->character, "Apply to AC : "); 
	  break; 
       case ITEM_CONTAINER: 
	  send_to_char(d->character, "Max weight to contain : "); 
	  break; 
       case ITEM_DRINKCON: 
       case ITEM_FOUNTAIN: 
	  send_to_char(d->character, "Max drink units : "); 
	  break; 
       case ITEM_FOOD: 
	  send_to_char(d->character, "Hours to fill stomach : "); 
	  break; 
       case ITEM_MONEY: 
	  send_to_char(d->character, "Number of gold coins : "); 
	  break; 
       case ITEM_NOTE: 
	 /* this is supposed to be language, but it's unused */ 
	  break; 
       case ITEM_GRENADE:
	  send_to_char(d->character, "Seconds to countdown to explosion : ");
	  break;
       case ITEM_PORTAL:
	  send_to_char(d->character, "Target Room: ");
	  break;
       case ITEM_FURNITURE:
	  oedit_disp_positions(d);
	  break;
       case ITEM_STABLE_TICKET:
	  send_to_char(d->character, "Horse Vnum: ");
	  break;
       default: 
	  oedit_disp_menu(d); 
      } 
} 
 
/* object value 2 */ 
void oedit_disp_val2_menu(struct descriptor_data * d) 
{ 
   OLC_MODE(d) = OEDIT_VALUE_2; 
   switch (GET_OBJ_TYPE(OLC_OBJ(d))) 
      { 
       case ITEM_LIGHT:
	  if(GET_OBJ_VAL(OLC_OBJ(d), 0)!=0)
	     oedit_disp_fuel_types(d);
	  else
	     {
	     GET_OBJ_VAL(OLC_OBJ(d), 1)=0;
	     oedit_disp_val3_menu(d);
	     }
	  break;
       case ITEM_FUEL:
	  oedit_disp_fuel_types(d);
	  break;
       case ITEM_ARMOR:		/* flags 2-4 are unused */
	  oedit_disp_val5_menu(d);
	  break;
       case ITEM_SCROLL: 
       case ITEM_PILL: 
       case ITEM_POTION: 
	  oedit_disp_spells_menu(d); 
	  break; 
       case ITEM_WAND: 
       case ITEM_STAFF: 
	  send_to_char(d->character, "Max number of charges : "); 
	  break; 
       case ITEM_ROCK:
       case ITEM_BOLT:
       case ITEM_ARROW:
       case ITEM_THROW:
       case ITEM_GRENADE:
       case ITEM_WEAPON: 
	  send_to_char(d->character, "Number of damage dice : "); 
	  break; 
       case ITEM_FOOD: 
	 /* values 2 and 3 are unused, jump to 4. how odd */ 
	  oedit_disp_val4_menu(d); 
	  break; 
       case ITEM_CONTAINER: 
	 /* these are flags, needs a bit of special handling */ 
	  oedit_disp_container_flags_menu(d); 
	  break; 
       case ITEM_DRINKCON: 
       case ITEM_FOUNTAIN: 
	  send_to_char(d->character, "Initial drink units : "); 
	  break; 
       case ITEM_PORTAL:
	  oedit_disp_val5_menu(d);
	  break;
       case ITEM_FURNITURE:
	  send_to_char(d->character, "Capacity (number people that can sit):");
	  break;
       default: 
	  oedit_disp_menu(d); 
      } 
} 
 
/* object value 3 */ 
void oedit_disp_val3_menu(struct descriptor_data * d) 
{ 
   OLC_MODE(d) = OEDIT_VALUE_3; 
   switch (GET_OBJ_TYPE(OLC_OBJ(d))) 
      { 
       case ITEM_LIGHT: 
	  send_to_char(d->character, "Number of hours (0 = burnt, -1 is infinite) : "); 
	  break; 
       case ITEM_FUEL:
	  oedit_disp_val5_menu(d); 
	  break;
       case ITEM_SCROLL: 
       case ITEM_PILL: 
       case ITEM_POTION: 
	  oedit_disp_spells_menu(d); 
	  break; 
       case ITEM_WAND: 
       case ITEM_STAFF: 
	  send_to_char(d->character, "Number of charges remaining : "); 
	  break; 
       case ITEM_ROCK:
       case ITEM_BOLT:
       case ITEM_ARROW:
       case ITEM_THROW:
       case ITEM_GRENADE:
       case ITEM_WEAPON: 
	  send_to_char(d->character, "Size of damage dice : "); 
	  break; 
       case ITEM_CONTAINER: 
	  send_to_char(d->character, "Vnum of key to open container (-1 for no key) : "); 
	  break; 
       case ITEM_DRINKCON: 
       case ITEM_FOUNTAIN: 
	  oedit_liquid_type(d); 
	  break; 
       case ITEM_FURNITURE:
	  send_to_char(d->character, "Regen gain (extra pts/tic) :"); 
	  break;
       default: 
	  oedit_disp_menu(d); 
      } 
} 
 
/* object value 4 */ 
void oedit_disp_val4_menu(struct descriptor_data * d) 
{ 
   OLC_MODE(d) = OEDIT_VALUE_4; 
   switch (GET_OBJ_TYPE(OLC_OBJ(d))) 
      { 
       case ITEM_LIGHT:
	  if(GET_OBJ_VAL(OLC_OBJ(d), 0)!=0)
	     send_to_char(d->character, "Maximum fuel capicity: ");
	  else
	     {
	     GET_OBJ_VAL(OLC_OBJ(d),3) = GET_OBJ_VAL(OLC_OBJ(d),2);
	     oedit_disp_val5_menu(d);
	     }
	  break;
       case ITEM_SCROLL: 
       case ITEM_PILL: 
       case ITEM_POTION: 
       case ITEM_WAND: 
       case ITEM_STAFF: 
	  oedit_disp_spells_menu(d); 
	  break; 
       case ITEM_WEAPON: 
	  oedit_disp_weapon_menu(d); 
	  break; 
       case ITEM_DRINKCON: 
       case ITEM_FOUNTAIN: 
       case ITEM_FOOD: 
	  send_to_char(d->character, "Poisoned (0 = not poison) : "); 
	  break; 
       case ITEM_FURNITURE:
	  send_to_char(d->character, "Char use (-1 = Mob only) :"); 
	  break;
       default: 
	  oedit_disp_menu(d); 
      } 
} 
/* object value 5 */ 
void oedit_disp_val5_menu(struct descriptor_data * d) 
{ 
   OLC_MODE(d) = OEDIT_VALUE_5; 
   switch (GET_OBJ_TYPE(OLC_OBJ(d))) 
      { 
       case ITEM_PORTAL:
       case ITEM_LIGHT: 
       case ITEM_SCROLL: 
       case ITEM_WAND: 
       case ITEM_STAFF: 
       case ITEM_WEAPON: 
       case ITEM_ARMOR: 
       case ITEM_POTION: 
       case ITEM_OTHER: 
       case ITEM_CONTAINER: 
       case ITEM_PILL: 
       case ITEM_WORN: 
       case ITEM_SHOVEL: 
	  send_to_char(d->character, "Level Restriction : "); 
	  break; 
       default: 
	  oedit_disp_menu(d); 
      } 
} 
 
/* object type */ 
void oedit_disp_type_menu(struct descriptor_data * d) 
{ 
   int counter, columns = 0; 

   get_char_cols(d->character); 
#if defined(CLEAR_SCREEN)
   send_to_char(d->character, "[H[J"); 
#endif
   for (counter = 0; counter < NUM_ITEM_TYPES; counter++) 
      { 
      send_to_char(d->character, "%s%2d%s) %-20.20s ", 
	      grn, counter, nrm, item_types[counter] 
	 ); 
      if (!(++columns % 2)) 
	 send_to_char(d->character, "\r\n"); 
      } 
   send_to_char(d->character, "\r\nEnter object type : "); 
} 
 
/* object extra flags */ 
void oedit_disp_extra_menu(struct descriptor_data * d) 
{ 
   int counter, columns = 0; 
   char *buf2=get_buffer(MAX_STRING_LENGTH);
   char *buf3=get_buffer(MAX_STRING_LENGTH);

   get_char_cols(d->character); 
#if defined(CLEAR_SCREEN)
   send_to_char(d->character, "[H[J"); 
#endif
   send_to_char(d->character, "\r\n");
   for (counter = 0; counter < NUM_ITEM_FLAGS; counter++)  
      { 
      send_to_char(d->character, "%s%2d%s) %-18.18s ", 
	      grn, counter + 1, nrm, extra_bits[counter] 
	 ); 
      if (!(++columns % 3)) 
	 send_to_char(d->character, "\r\n"); 
      } 
   for (counter = 0; counter < NUM_ITEM2_FLAGS; counter++)
      { 
      send_to_char(d->character, "%s%2d%s) %-18.18s ",
	      grn, counter + 1 + NUM_ITEM_FLAGS, nrm,
	      extra_bits2[counter]
         );
      if (!(++columns % 3))
         send_to_char(d->character, "\r\n");
      }

   sprintbit(GET_OBJ_EXTRA(OLC_OBJ(d)), extra_bits, buf2);
   sprintbit(GET_OBJ_EXTRA2(OLC_OBJ(d)), extra_bits2, buf3);
   send_to_char(d->character, "\r\nObject flags: %s%s%s%s\r\n" 
		"Enter object extra flag (0 to quit) : ",  
		cyn, !strcmp(buf2, "NOBITS ") &&
		      strcmp(buf3, "NOBITS ")?"":buf2, 
		!strcmp(buf3, "NOBITS ")?"":buf3, nrm
      ); 
   release_buffer(buf3);
   release_buffer(buf2);
} 

/* object extra flags */ 
void oedit_disp_anti_menu(struct descriptor_data * d) 
{ 
   int counter, columns = 0; 
   char *buf2=get_buffer(MAX_STRING_LENGTH);

   get_char_cols(d->character); 
#if defined(CLEAR_SCREEN)
   send_to_char(d->character, "[H[J"); 
#endif
   for (counter = 0; counter < NUM_ANTI_FLAGS; counter++)  
      { 
      send_to_char(d->character, "%s%2d%s) %-20.20s ", 
	      grn, counter + 1, nrm, anti_bits[counter] 
	 ); 
      if (!(++columns % 2)) 
	 send_to_char(d->character, "\r\n"); 
      } 
   sprintbit(GET_OBJ_ANTI(OLC_OBJ(d)), anti_bits, buf2); 
   send_to_char(d->character, "\r\nObject flags: %s%s%s\r\n" 
	   "Enter object extra flag (0 to quit) : ",  
	   cyn, buf2, nrm 
      ); 
   release_buffer(buf2);
} 
 
/* Ponder (04/02/1997) 
 * Material type menu */ 
void oedit_disp_material_menu(struct descriptor_data * d)  
{ 
   int counter, columns = 0; 
 
   get_char_cols(d->character); 
#if defined(CLEAR_SCREEN)
   send_to_char(d->character, "[H[J"); 
#endif
   for (counter = 0; *material_types[counter] != '\n'  ; counter++) 
      { 
      send_to_char(d->character, "%s%2d%s) %-20.20s ", 
	      grn, counter, nrm, material_types[counter] 
	 ); 
      if (!(++columns % 2)) 
	 send_to_char(d->character, "\r\n"); 
      } 
   send_to_char(d->character, "\r\nCurrent material: %s%s%s\r\n" 
	   "Enter wear flag, 0 to quit : ", 
	   cyn, material_types[OLC_OBJ(d)->material], nrm 
      ); 
} 
 
/* object wear flags */ 
void oedit_disp_wear_menu(struct descriptor_data * d) 
{ 
   int counter, columns = 0; 
   char *buf2=get_buffer(MAX_STRING_LENGTH);

   get_char_cols(d->character); 
#if defined(CLEAR_SCREEN)
   send_to_char(d->character, "[H[J"); 
#endif
   for (counter = 0; counter < NUM_ITEM_WEARS; counter++) 
      { 
      if (strcmp(wear_bits[counter], "UNUSED"))
         {
         send_to_char(d->character, "%s%2d%s) %-20.20s ", 
   	         grn, counter + 1, nrm, wear_bits[counter]
	    );
         columns++;
         } 
      if (!(columns % 2)) 
	 send_to_char(d->character, "\r\n"); 
      } 
   sprintbit(GET_OBJ_WEAR(OLC_OBJ(d)), wear_bits, buf2); 
   send_to_char(d->character, "\r\nWear flags: %s%s%s\r\n" 
	   "Enter wear flag, 0 to quit : ", 
	   cyn, buf2, nrm 
      ); 
   release_buffer(buf2);
} 


/* object spell affect flags */ 
void oedit_disp_spellaff_menu(struct descriptor_data * d) 
{ 
   int counter, columns = 0; 
   char *buf2=get_buffer(MAX_STRING_LENGTH);

   get_char_cols(d->character); 
#if defined(CLEAR_SCREEN)
   send_to_char(d->character, "[H[J"); 
#endif
   send_to_char(d->character, "Spell Affect Flags\r\n");
   for (counter = 0; counter < NUM_AFF_FLAGS; counter++) 
      { 
      send_to_char(d->character, "%s%2d%s) %-20.20s ", 
	      grn, counter + 1, nrm, affected_bits[counter]); 
      if (!(++columns % 2)) 
	 send_to_char(d->character, "\r\n"); 
      } 
   sprintbit(OLC_OBJ(d)->obj_flags.bitvector, affected_bits, buf2); 
   send_to_char(d->character, "\r\nCurrent flags: %s%s%s\r\n" 
	   "Enter flag number, 0 to quit : ", 
	   cyn, buf2, nrm 
      ); 
   release_buffer(buf2);
} 
 
/* display main menu */ 
void oedit_disp_menu(struct descriptor_data * d) 
{ 
   struct obj_data *obj; 
   char *buf1=get_buffer(MAX_STRING_LENGTH);
   char *buf2=get_buffer(MAX_STRING_LENGTH);
   char *buf3=get_buffer(MAX_STRING_LENGTH);
   char *buf4=get_buffer(512);

   obj = OLC_OBJ(d); 
   get_char_cols(d->character); 
 
  /*. Build buffers for first part of menu .*/ 
   sprinttype(GET_OBJ_TYPE(obj), item_types, buf1); 
   sprintbit(GET_OBJ_EXTRA(obj), extra_bits, buf2); 
   sprintbit(GET_OBJ_EXTRA2(obj), extra_bits2, buf3);
 
#if defined(CLEAR_SCREEN)
   send_to_char(d->character, "[H[J"); 
#endif
  /*. Build first half of menu .*/ 
   send_to_char(d->character, 
	   "-- Item number : [%s%d%s]\r\n" 
	   "%s1%s) Namelist : %s%s\r\n" 
	   "%s2%s) S-Desc   : %s%s\r\n" 
	   "%s3%s) L-Desc   :-\r\n%s%s\r\n" 
	   "%s4%s) A-Desc   :-\r\n%s%s" 
	   "%s5%s) Type        : %s%s\r\n" 
	   "%s6%s) Extra flags : %s%s\r\n" 
             "%s   Extra2 flags: %s%s\r\n", 

	   cyn, OLC_NUM(d), nrm,  
	   grn, nrm, yel, (obj->name && *obj->name) ? obj->name : "undefined", 
	   grn, nrm, yel, (obj->short_description && *obj->short_description) ?
	   obj->short_description : "undefined", 
	   grn, nrm, yel, (obj->description && *obj->description) ?
	   obj->description : "|EMPTY|", 
	   grn, nrm, yel,(obj->action_description && *obj->action_description)?
	   obj->action_description : "<not set>\r\n", 
	   grn, nrm, cyn, buf1, 
	   grn, nrm, cyn, buf2, 
                nrm, cyn, buf3  
      ); 
  /*. Send first half .*/ 
 
  /*. Build second half of menu .*/ 
   sprintbit(GET_OBJ_WEAR(obj), wear_bits, buf1); 
 
  /* Ponder (04/02/1997) - Add a material type to olc */ 
   sprinttype(obj->material, material_types, buf2); 
 
   sprintbit(GET_OBJ_ANTI(obj), anti_bits,buf3);
   sprintbit(obj->obj_flags.bitvector,affected_bits,buf4);
   send_to_char(d->character, 
	   "%s7%s) Material    : %s%s\r\n"  
	   "%s8%s) Wear flags  : %s%s\r\n" 
	   "%s9%s) Weight      : %s%5d    " 
	   "%sA%s) Cost        : %s%5d\r\n" 
	   "%sB%s) Cost/Day    : %s%5d    " 
	   "%sC%s) Timer       : %s%5d\r\n" 
	   "%sE%s) Values      : %s%ld %ld %ld %ld %ld %ld %ld %ld\r\n" 
	   "%sF%s) Applies menu           " 
	   "%sG%s) Extra descriptions menu: %s%s\r\n" 
           "%sH%s) Curr Dam Slots : %s%5d "
	   "%sI%s) Total Dam Slots: %s%5d\r\n"
	   "%sJ%s) Anti race/class: %s%s\r\n"
	   "%sK%s) Spell Affects  : %s%s (>ADMIN)\r\n"
	   "%sL%s) Weapon Spells Menu %s(>ADMIN)\r\n"
	   "%sM%s) Script         : %s%s\r\n"
	   "%sQ%s) Quit\r\n" 
	   "Enter choice : ", 
 
	   grn, nrm, cyn, buf2, 
	   grn, nrm, cyn, buf1, 
	   grn, nrm, cyn, GET_OBJ_WEIGHT(obj), 
	   grn, nrm, cyn, GET_OBJ_COST(obj), 
	   grn, nrm, cyn, GET_OBJ_RENT(obj), 
	   grn, nrm, cyn, GET_OBJ_TIMER(obj), 
	   grn, nrm, cyn, GET_OBJ_VAL(obj, 0),  
	   GET_OBJ_VAL(obj, 1),  
	   GET_OBJ_VAL(obj, 2), 
	   GET_OBJ_VAL(obj, 3), 
	   GET_OBJ_VAL(obj, 4), 
	   GET_OBJ_VAL(obj, 5), 
	   GET_OBJ_VAL(obj, 6), 
	   GET_OBJ_VAL(obj, 7), 
	   grn, nrm, 
	   grn, nrm, cyn, (obj->ex_description?"SET":"NONE"),
	   grn, nrm, cyn, GET_OBJ_CSLOTS(obj),
	   grn, nrm, cyn, GET_OBJ_TSLOTS(obj),
	   grn, nrm, cyn, buf3,
	   grn, nrm, cyn, buf4,
	   grn, nrm, cyn,
	   grn, nrm, cyn, obj->proto_script?"Set.":"Not Set.",
	   grn, nrm
      );
   OLC_MODE(d) = OEDIT_MAIN_MENU; 
   release_buffer(buf4);
   release_buffer(buf3);
   release_buffer(buf2);
   release_buffer(buf1);
} 
 
/*************************************************************************** 
 main loop (of sorts).. basically interpreter throws all input to here 
 ***************************************************************************/ 
 
 
void oedit_parse(struct descriptor_data * d, char *arg) 
{ 
   char *buf;
   int vnumber, max_val, min_val, tmp; 

   switch (OLC_MODE(d)) 
      { 
 
       case OEDIT_CONFIRM_SAVESTRING: 
	  switch (*arg) 
	     { 
	      case 'y': 
	      case 'Y': 
		 send_to_char(d->character, "Saving object to memory.\r\n"); 
		 oedit_save_internally(d); 
		 mudlogf(CMP, MAX(LVL_BUILDER,GET_INVIS_LEV(d->character)),
			 TRUE,"OLC: %s has edited obj %d",
			 GET_NAME(d->character), OLC_NUM(d)); 
		 cleanup_olc(d, CLEANUP_STRUCTS); 
		 return; 
	      case 'n': 
	      case 'N': 
		/*. Cleanup all .*/ 
		 cleanup_olc(d, CLEANUP_ALL); 
		 return; 
	      default: 
		 send_to_char(d->character, "Invalid choice!\r\n"); 
		 send_to_char(d->character, "Do you wish to save this "
			      "object internally?\r\n"); 
		 return; 
	     } 
 
       case OEDIT_MAIN_MENU: 
	 /* throw us out to whichever edit mode based on user input */ 
	  switch (*arg) 
	     { 
	      case 'q': 
	      case 'Q': 
		 if (OLC_VAL(d))  
		    { 
		   /*. Something has been modified .*/ 
		    send_to_char(d->character, "Do you wish to save "
				 "this object internally? : "); 
		    OLC_MODE(d) = OEDIT_CONFIRM_SAVESTRING; 
		    } 
		 else  
		    cleanup_olc(d, CLEANUP_ALL); 
		 return; 
	      case '1': 
		 OLC_VAL(d) = 1; /*. Has changed flag .*/ 
		 send_to_char(d->character, "Enter namelist : "); 
		 OLC_MODE(d) = OEDIT_EDIT_NAMELIST; 
		 break; 
	      case '2': 
		 OLC_VAL(d) = 1; /*. Has changed flag .*/ 
		 send_to_char(d->character, "Enter short desc : "); 
		 OLC_MODE(d) = OEDIT_SHORTDESC; 
		 break; 
	      case '3': 
		 OLC_VAL(d) = 1; /*. Has changed flag .*/ 
		 send_to_char(d->character, "Enter long desc :-\r\n| "); 
		 OLC_MODE(d) = OEDIT_LONGDESC; 
		 break; 
	      case '4': 
		 OLC_VAL(d) = 1; /*. Has changed flag .*/ 
		 OLC_MODE(d) = OEDIT_ACTDESC; 
		 SEND_TO_Q(d,"Enter action description: (/s saves /h "
			   "for help)\r\n\r\n"); 
		 d->backstr = NULL; 
		 if (OLC_OBJ(d)->action_description) 
		    { 
		    SEND_TO_Q(d, "%s", OLC_OBJ(d)->action_description); 
		    d->backstr = str_dup(OLC_OBJ(d)->action_description); 
		    } 
		 d->str = &OLC_OBJ(d)->action_description; 
		 d->max_str = MAX_MESSAGE_LENGTH; 
		 d->mail_to = 0; 
		/*      OLC_VAL(d) = 1;   Should this be commented out? 
		 *                        Michael scott says so */ 
		 break; 
	      case '5': 
		 oedit_disp_type_menu(d); 
		 OLC_MODE(d) = OEDIT_TYPE; 
		 break; 
	      case '6': 
		 oedit_disp_extra_menu(d); 
		 OLC_MODE(d) = OEDIT_EXTRAS; 
		 break; 
	      case '7': /* Ponder (04/02/1997) - Material type */ 
		 oedit_disp_material_menu(d); 
		 OLC_MODE(d) = OEDIT_MATERIAL; 
		 break; 
	      case '8': 
		 oedit_disp_wear_menu(d); 
		 OLC_MODE(d) = OEDIT_WEAR; 
		 break; 
	      case '9': 
		 send_to_char(d->character, "Enter weight : "); 
		 OLC_MODE(d) = OEDIT_WEIGHT; 
		 break; 
	      case 'a': 
	      case 'A': 
		 send_to_char(d->character, "Enter cost : "); 
		 OLC_MODE(d) = OEDIT_COST; 
		 break; 
	      case 'b': 
	      case 'B': 
		 send_to_char(d->character, "Enter cost per day : "); 
		 OLC_MODE(d) = OEDIT_COSTPERDAY; 
		 break; 
	      case 'c': 
	      case 'C': 
		 send_to_char(d->character, "Enter timer : "); 
		 OLC_MODE(d) = OEDIT_TIMER; 
		 break; 
	      case 'd': 
	      case 'D': 
		 oedit_disp_menu(d); 
		/*. Object level flags in my mud... 
		  send_to_char(d->character, "Enter level : "); 
		  OLC_MODE(d) = OEDIT_LEVEL; 
		  .*/ 
		 break; 
	      case 'e': 
	      case 'E': 
		/*. Clear any old values .*/ 
		 GET_OBJ_VAL(OLC_OBJ(d), 0) = 0; 
		 GET_OBJ_VAL(OLC_OBJ(d), 1) = 0; 
		 GET_OBJ_VAL(OLC_OBJ(d), 2) = 0; 
		 GET_OBJ_VAL(OLC_OBJ(d), 3) = 0; 
		 GET_OBJ_VAL(OLC_OBJ(d), 4) = 0; 
		 GET_OBJ_VAL(OLC_OBJ(d), 5) = 0; 
		 GET_OBJ_VAL(OLC_OBJ(d), 6) = 0; 
		 GET_OBJ_VAL(OLC_OBJ(d), 7) = 0; 
		 OLC_VAL(d)=1;
		 oedit_disp_val1_menu(d); 
		 break; 
	      case 'f': 
	      case 'F': 
		 oedit_disp_prompt_apply_menu(d); 
		 break; 
	      case 'g': 
	      case 'G': 
		/* if extra desc doesn't exist . */ 
		 if (!OLC_OBJ(d)->ex_description) 
		    { 
		    CREATE(OLC_OBJ(d)->ex_description, 
			   struct extra_descr_data, 1); 
		    OLC_OBJ(d)->ex_description->next = NULL; 
		    } 
		 OLC_DESC(d) = OLC_OBJ(d)->ex_description; 
		 oedit_disp_extradesc_menu(d); 
		 break; 
	      case 'h': 
	      case 'H': 
		 send_to_char(d->character, "Enter current Dam slots : "); 
		 OLC_MODE(d) = OEDIT_CDAM; 
		 break; 
	      case 'i': 
	      case 'I': 
		 if(GET_LEVEL(d->character)>=OEDIT_ACCESS)
		    {
		    send_to_char(d->character, "Enter total Dam slots : "); 
		    OLC_MODE(d) = OEDIT_TDAM; 
		    }
		 else
		    {
		    oedit_disp_menu(d);
		    send_to_char(d->character, 
				 "You do not have permission!\r\n");
		    }
		 break; 
	      case 'j': 
	      case 'J': 
		 oedit_disp_anti_menu(d); 
		 OLC_MODE(d) = OEDIT_ANTI; 
		 break; 
	      case 'k':
	      case 'K':
		 if(GET_LEVEL(d->character)>=OEDIT_ACCESS)
		    {
		    oedit_disp_spellaff_menu(d);
		    OLC_MODE(d) = OEDIT_SPELL_AFF;
		    }
		 else
		    {
		    oedit_disp_menu(d);
		    send_to_char(d->character, 
				 "You do not have permission!\r\n");
		    }
		 break;
		 
	      case 'l':
	      case 'L':
		 if(GET_OBJ_TYPE(OLC_OBJ(d))!=ITEM_WEAPON)
		    {
		    oedit_disp_menu(d);
		    send_to_char(d->character, "Needs to be a weapon!\r\n");
		    }
		 else if(GET_LEVEL(d->character)>=OEDIT_ACCESS)
		    {
		    oedit_disp_prompt_wpnspl_menu(d);
		    OLC_MODE(d) = OEDIT_PROMPT_WPNSPL;
		    }
		 else
		    {
		    oedit_disp_menu(d);
		    send_to_char(d->character, 
				 "You do not have permission\r\n");
		    }
		 break;

	      case 'm':
	      case 'M':
                 if(!PRF2_FLAGGED(d->character,PRF2_DG_ATTACH))
                    {
                    oedit_disp_menu(d); 
                    send_to_char(d->character, "You don't have permission "
				 "to attach scripts!\r\n: ");
                    release_buffer(buf);
                    return;
                    }
		 OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_MAIN_MENU;
		 dg_script_menu(d);
		 return;
		 
	      default: 
		 oedit_disp_menu(d); 
		 break; 
	     } 
	  return;   /* end of OEDIT_MAIN_MENU */ 
 
       case OLC_SCRIPT_EDIT:
	  if (dg_script_edit_parse(d, arg)) 
	     return;
	  break;

       case OEDIT_EDIT_NAMELIST: 
	  if (OLC_OBJ(d)->name) 
	     free(OLC_OBJ(d)->name); 
	  OLC_OBJ(d)->name = str_dup((arg && *arg) ? arg : "undefined");
	  break; 
 
       case OEDIT_SHORTDESC: 
	  if (OLC_OBJ(d)->short_description) 
	     free(OLC_OBJ(d)->short_description); 
	  OLC_OBJ(d)->short_description = str_dup((arg && *arg) ? arg :
						  "undefined");
	  break; 
 
       case OEDIT_LONGDESC: 
	  if (OLC_OBJ(d)->description) 
	     free(OLC_OBJ(d)->description); 
	  OLC_OBJ(d)->description = str_dup((arg && *arg) ? arg : "\0");
	  break; 
 
       case OEDIT_TYPE: 
	  vnumber = atoi(arg); 
	  if ((vnumber < 0) || (vnumber >= NUM_ITEM_TYPES)) 
	     { 
	     send_to_char(d->character, "Invalid choice, try again : "); 
	     return; 
	     } 
	  else  
	     { 
	     GET_OBJ_TYPE(OLC_OBJ(d)) = vnumber; 
	     for(tmp=0;tmp<8;tmp++)
		GET_OBJ_VAL(OLC_OBJ(d),tmp)=0;
	     for(tmp=0;tmp<MAX_SPELL_AFFECT;tmp++)
		{
		OLC_OBJ(d)->spell_affect[tmp].spelltype = 0;
		OLC_OBJ(d)->spell_affect[tmp].level     = 0;
		OLC_OBJ(d)->spell_affect[tmp].percentage= 0;
		}
	     if(vnumber==ITEM_WEAPON)
		{
		GET_OBJ_VAL(OLC_OBJ(d),3)=1;
		}
	     } 

	  break; 
 
       case OEDIT_EXTRAS: 
	  vnumber = atoi(arg); 
	  if ((vnumber < 0) || (vnumber > (NUM_ITEM_FLAGS+NUM_ITEM2_FLAGS))) 
	     { 
	     oedit_disp_extra_menu(d); 
	     return; 
	     } 
	  else if (vnumber == 0) 
	     break; 
	  else
	     { 
	     if (vnumber <= NUM_ITEM_FLAGS)
		TOGGLE_BIT(GET_OBJ_EXTRA(OLC_OBJ(d)), 1 << (vnumber - 1));
	     else
		TOGGLE_BIT(GET_OBJ_EXTRA2(OLC_OBJ(d)),
			 1 << (vnumber - 1 - NUM_ITEM_FLAGS));
	     oedit_disp_extra_menu(d);
	     return;
	     } 


	 /* Ponder (04/02/1997) - Change the material type here */ 
       case OEDIT_MATERIAL: 
	  vnumber = atoi(arg); 
	  for (tmp=0; tmp < vnumber && *material_types[tmp] != '\n'; tmp++) ; 
	  if ((vnumber < 0) || (*material_types[tmp] == '\n')) 
	     { 
	     send_to_char(d->character, "That's not a valid choice!\r\n"); 
	     oedit_disp_material_menu(d); 
	     return; 
	     }  
	  OLC_OBJ(d)->material = vnumber; 
	  GET_OBJ_CSLOTS(OLC_OBJ(d)) =material_affs[vnumber].default_dam_slots;
	  GET_OBJ_TSLOTS(OLC_OBJ(d)) =material_affs[vnumber].default_dam_slots;
	  GET_OBJ_OSLOTS(OLC_OBJ(d)) =material_affs[vnumber].default_dam_slots;

	  break; 
 
       case OEDIT_WEAR: 
	  vnumber = atoi(arg); 
	  if ((vnumber < 0) || (vnumber > NUM_ITEM_WEARS)) 
	     { 
	     send_to_char(d->character, "That's not a valid choice!\r\n"); 
	     oedit_disp_wear_menu(d); 
	     return; 
	     } 
	  else if (vnumber == 0) 
	     break;
          else if (!strcmp(wear_bits[vnumber - 1], "UNUSED") &&
                   (GET_LEVEL(d->character) < LVL_ADMIN))
             {
             send_to_char(d->character, "Those are unused, please try again.\r\n");
             oedit_disp_wear_menu(d);
             return;
             }
	  TOGGLE_BIT(GET_OBJ_WEAR(OLC_OBJ(d)), 1 << (vnumber - 1));
	  oedit_disp_wear_menu(d);
	  return; 
 
       case OEDIT_WEIGHT: 
	  vnumber = atoi(arg); 
	  GET_OBJ_WEIGHT(OLC_OBJ(d)) = vnumber; 
	  break; 
 
       case OEDIT_COST: 
	  vnumber = atoi(arg); 
	  GET_OBJ_COST(OLC_OBJ(d)) = vnumber; 
	  break; 
 
       case OEDIT_COSTPERDAY: 
	  vnumber = atoi(arg); 
	  GET_OBJ_RENT(OLC_OBJ(d)) = vnumber; 
	  break; 
 
       case OEDIT_TIMER: 
	  vnumber = atoi(arg); 
	  GET_OBJ_TIMER(OLC_OBJ(d)) = vnumber; 
	  break; 
 
       case OEDIT_VALUE_1: 
	 /* lucky, I don't need to check any of these for outofrange values */ 
	 /*. Hmm, I'm not so sure - Rv .*/ 
	  vnumber = atoi(arg); 
	  
	  switch (GET_OBJ_TYPE(OLC_OBJ(d))) 
	     { 
	      case ITEM_SCROLL: 
	      case ITEM_PILL: 
	      case ITEM_POTION: 
	      case ITEM_WAND: 
	      case ITEM_STAFF: 
		 min_val = 1; 
		 max_val = 10; 
		 break; 
	      case ITEM_STABLE_TICKET:
	      case ITEM_PORTAL:
		 min_val=0;
		 max_val=320099;
		 break;
			 
	      default: 
		 min_val = -32000; 
		 max_val = 32000; 
	     } 

	  GET_OBJ_VAL(OLC_OBJ(d), 0) = MAX(min_val, MIN(vnumber, max_val));  
	 /* proceed to menu 2 */ 
	  oedit_disp_val2_menu(d); 
	  return; 
       case OEDIT_VALUE_2: 
	 /* here, I do need to check for outofrange values */ 
	  vnumber = atoi(arg); 
	  switch (GET_OBJ_TYPE(OLC_OBJ(d)))  
	     { 
	      case ITEM_SCROLL: 
	      case ITEM_PILL: 
	      case ITEM_POTION: 
		 if(vnumber==0)
		    vnumber=-1;
		 if (vnumber < -1 || vnumber >= MAX_SPELLS) 
		    oedit_disp_val2_menu(d); 
		 else if(spells[vnumber].is_spell!=IS_SPELL)
		    oedit_disp_val2_menu(d); 
		 else 
		    { 
		    GET_OBJ_VAL(OLC_OBJ(d), 1) = vnumber; 
		    oedit_disp_val3_menu(d); 
		    } 
		 break; 
	      case ITEM_CONTAINER: 
		/*
		 * needs some special handling since we are dealing 
		 * with flag values here 
		 */ 
		 vnumber = atoi(arg); 
		 if (vnumber < 0 || vnumber > 4) 
		    oedit_disp_container_flags_menu(d); 
		 else  
		    { 
		   /* if 0, quit */ 
		    if (vnumber != 0) 
		       { 
		       vnumber = 1 << (vnumber - 1); 
		       if (IS_SET(GET_OBJ_VAL(OLC_OBJ(d), 1), vnumber)) 
			  REMOVE_BIT(GET_OBJ_VAL(OLC_OBJ(d), 1), vnumber); 
		       else 
			  SET_BIT(GET_OBJ_VAL(OLC_OBJ(d), 1), vnumber); 
		       oedit_disp_val2_menu(d); 
		       } 
		    else 
		       oedit_disp_val3_menu(d); 
		    } 
		 break; 
	      case ITEM_LIGHT:
	      case ITEM_FUEL:
		 if(vnumber<1||vnumber>NUM_FUEL_TYPES)
		    oedit_disp_fuel_types(d);
		 else
		    {
		    GET_OBJ_VAL(OLC_OBJ(d), 1) = vnumber; 
		    oedit_disp_val3_menu(d); 
		    }
		 break;
	      default: 
		 GET_OBJ_VAL(OLC_OBJ(d), 1) = vnumber; 
		 oedit_disp_val3_menu(d); 
	     } 
	  return; 
 
       case OEDIT_VALUE_3: 
	  vnumber = atoi(arg); 
	 /*. Quick'n'easy error checking .*/ 
	  switch (GET_OBJ_TYPE(OLC_OBJ(d)))  
	     { 
	      case ITEM_SCROLL: 
	      case ITEM_PILL: 
	      case ITEM_POTION: 
		 if(vnumber==0)
		    vnumber=-1;
		 min_val = -1; 
		 max_val = MAX_SPELLS -1; 
		 break; 
	      case ITEM_WEAPON: 
		 min_val = 1; 
		 max_val = 50; 
	      case ITEM_WAND: 
	      case ITEM_STAFF: 
		 min_val = 0; 
		 max_val = 20; 
		 break; 
	      case ITEM_DRINKCON: 
	      case ITEM_FOUNTAIN: 
		 min_val = 0; 
		 max_val = NUM_LIQ_TYPES -1; 
		 break; 
	      case ITEM_LIGHT:
		 min_val = -1;
		 max_val = 120;
	      case ITEM_CONTAINER:
		 min_val=0;
		 max_val=320099;
		 break;
	      default: 
		 min_val = -32000; 
		 max_val = 32000; 
	     } 
	  GET_OBJ_VAL(OLC_OBJ(d), 2) = MAX(min_val, MIN(vnumber, max_val)); 
	  oedit_disp_val4_menu(d); 
	  return; 
 
       case OEDIT_VALUE_4: 
	  vnumber = atoi(arg); 
	  switch (GET_OBJ_TYPE(OLC_OBJ(d))) 
	     { 
	      case ITEM_SCROLL: 
	      case ITEM_PILL: 
	      case ITEM_POTION: 
	      case ITEM_WAND: 
	      case ITEM_STAFF: 
		 if(vnumber==0)
		    vnumber=-1;
		 min_val = -1; 
		 max_val = MAX_SPELLS -1; 
		 break; 
	      case ITEM_WEAPON: 
		 min_val = 1; 
		 max_val = NUM_ATTACK_TYPES -1; 
		 break; 
	      case ITEM_LIGHT:
		 min_val = GET_OBJ_VAL(OLC_OBJ(d),2);
		 max_val = 120;
	      default: 
		 min_val = -32000; 
		 max_val = 32000; 
		 break; 
	     } 
	  GET_OBJ_VAL(OLC_OBJ(d), 3) = MAX(min_val, MIN(vnumber, max_val)); 
	  oedit_disp_val5_menu(d); 
	  return; 
 
       case OEDIT_VALUE_5: 
	  vnumber = atoi(arg); 
	  switch (GET_OBJ_TYPE(OLC_OBJ(d))) 
	     { 
	      case ITEM_LIGHT:  
	      case ITEM_SCROLL: 
	      case ITEM_WAND: 
	      case ITEM_STAFF: 
	      case ITEM_WEAPON: 
	      case ITEM_ARMOR: 
	      case ITEM_POTION: 
	      case ITEM_OTHER: 
	      case ITEM_CONTAINER: 
	      case ITEM_PILL: 
	      case ITEM_WORN: 
	      default: 
		 min_val = 0; 
		 max_val = LVL_IMPL; 
		 break; 
	     } 
	  GET_OBJ_VAL(OLC_OBJ(d), 4) = MAX(min_val, MIN(vnumber, max_val)); 
	  break; 
 
       case OEDIT_PROMPT_APPLY: 
	  vnumber = atoi(arg); 
	  if (vnumber == 0) 
	     break; 
	  else if (vnumber < 0 || vnumber > MAX_OBJ_AFFECT)  
	     { 
	     oedit_disp_prompt_apply_menu(d); 
	     return; 
	     } 
	  OLC_VAL(d) = vnumber - 1; 
	  OLC_MODE(d) = OEDIT_APPLY; 
	  oedit_disp_apply_menu(d); 
	  return; 

       case OEDIT_APPLY: 
	  vnumber = atoi(arg); 
	  if (vnumber == 0)  
	     { 
	     OLC_OBJ(d)->affected[OLC_VAL(d)].location = 0; 
	     OLC_OBJ(d)->affected[OLC_VAL(d)].modifier = 0; 
	     oedit_disp_prompt_apply_menu(d); 
	     } 
	  else if (vnumber < 0 || vnumber >= NUM_APPLIES)  
	     oedit_disp_apply_menu(d); 
	  else if(vnumber==APPLY_IMMUNE&&GET_LEVEL(d->character)<LVL_ADMIN)
	     oedit_disp_apply_menu(d);
	  else if ((vnumber==APPLY_IMMUNE)||
		   (vnumber==APPLY_RESIST)||
		   (vnumber==APPLY_SUSC))
	     {
	     OLC_OBJ(d)->affected[OLC_VAL(d)].location = vnumber; 
	     oedit_disp_immapp_menu(d);
	     OLC_MODE(d) = OEDIT_IMMUNE_APP; 
	     }
	  else
	     { 
	     OLC_OBJ(d)->affected[OLC_VAL(d)].location = vnumber; 
	     send_to_char(d->character, "Modifier : "); 
	     OLC_MODE(d) = OEDIT_APPLYMOD; 
	     } 
	  return; 
 
       case OEDIT_APPLYMOD: 
	  vnumber = atoi(arg); 
	  OLC_OBJ(d)->affected[OLC_VAL(d)].modifier = vnumber; 
	  oedit_disp_prompt_apply_menu(d); 
	  return; 
 
       case OEDIT_IMMUNE_APP:
	  vnumber = atoi(arg);
	  if(vnumber==0)
	     {
	     oedit_disp_prompt_apply_menu(d);
	     return;
	     }
	  if((vnumber>0)&&(vnumber<=NUM_IMMUN_FLAGS))
	     {
	     vnumber= 1<<(vnumber - 1);
	     if(IS_SET(OLC_OBJ(d)->affected[OLC_VAL(d)].modifier,vnumber))
		REMOVE_BIT(OLC_OBJ(d)->affected[OLC_VAL(d)].modifier,vnumber);
	     else
		SET_BIT(OLC_OBJ(d)->affected[OLC_VAL(d)].modifier,vnumber);
	     }
	  oedit_disp_immapp_menu(d);
	  return;
	  break;

       case OEDIT_PROMPT_WPNSPL:
	  vnumber = atoi(arg);
	  if(vnumber ==0)
	     break;
	  else if(vnumber < 0 || vnumber > MAX_SPELL_AFFECT) 
	     {
	     oedit_disp_prompt_wpnspl_menu(d);
	     return;
	     }
	  OLC_VAL(d) = vnumber-1;
	  OLC_MODE(d)= OEDIT_WPNSPL;
	  oedit_disp_wpnspl_menu(d);
	  return;

       case OEDIT_WPNSPL:
	  vnumber=atoi(arg);
	  if(vnumber==0)
	     {
	     OLC_OBJ(d)->spell_affect[OLC_VAL(d)].spelltype=0;
	     OLC_OBJ(d)->spell_affect[OLC_VAL(d)].level=0;
	     OLC_OBJ(d)->spell_affect[OLC_VAL(d)].percentage=0;
	     oedit_disp_prompt_wpnspl_menu(d);
	     OLC_MODE(d)=OEDIT_PROMPT_WPNSPL;
	     }
	  else if ((vnumber < 0) ||(vnumber >= MAX_SPELLS)||
		   (spells[vnumber].is_spell!=IS_SPELL))
	     {
	     oedit_disp_wpnspl_menu(d); 
	     }
	  else
	     {
	     OLC_OBJ(d)->spell_affect[OLC_VAL(d)].spelltype=vnumber;
	     send_to_char(d->character, "Spell Level (1-10) : ");
	     OLC_MODE(d) = OEDIT_WPNSPL_LVL;
	     }
	  return;

       case OEDIT_WPNSPL_LVL:
	  vnumber=atoi(arg);
	  if((vnumber<=0)||(vnumber>10))
	     {
	     send_to_char(d->character, "Spell Level (1-10) : ");
	     OLC_MODE(d) = OEDIT_WPNSPL_LVL;
	     }
	  else
	     {
	     OLC_OBJ(d)->spell_affect[OLC_VAL(d)].level=vnumber;
	     send_to_char(d->character, "Percent chance to hit (1-100) : ");
	     OLC_MODE(d) = OEDIT_WPNSPL_PCT;
	     }
	  return;

       case OEDIT_WPNSPL_PCT:
	  vnumber=atoi(arg);
	  if((vnumber<=0)||(vnumber>100))
	     {
	     send_to_char(d->character, "Percent chance to hit (1-100) : ");
	     OLC_MODE(d) = OEDIT_WPNSPL_PCT;
	     }
	  else
	     {
	     OLC_OBJ(d)->spell_affect[OLC_VAL(d)].percentage=vnumber;
	     oedit_disp_prompt_wpnspl_menu(d);
	     OLC_MODE(d) = OEDIT_PROMPT_WPNSPL;
	     }
	  return;
	     
       case OEDIT_EXTRADESC_KEY: 
	  if (OLC_DESC(d)->keyword) 
	     free(OLC_DESC(d)->keyword); 
	  OLC_DESC(d)->keyword = str_dup((arg && *arg) ? arg : "undefined");
	  oedit_disp_extradesc_menu(d); 
	  return; 
 
       case OEDIT_EXTRADESC_MENU: 
	  vnumber = atoi(arg); 
	  switch (vnumber) 
	     { 
	      case 0: 
	      { 
	     /* if something got left out */ 
	      if (!OLC_DESC(d)->keyword || !OLC_DESC(d)->description)  
		 { 
		 struct extra_descr_data **tmp_desc; 
 
		 if (OLC_DESC(d)->keyword) 
		    free(OLC_DESC(d)->keyword); 
		 if (OLC_DESC(d)->description) 
		    free(OLC_DESC(d)->description); 
 
		/*. Clean up pointers .*/ 
		 for(tmp_desc = &(OLC_OBJ(d)->ex_description); *tmp_desc; 
		     tmp_desc = &((*tmp_desc)->next)) 
		    { 
		    if (*tmp_desc == OLC_DESC(d)) 
		       { 
		       *tmp_desc = NULL; 
		       break; 
		       } 
		    } 
		 free(OLC_DESC(d)); 
		 } 
	      } 
	      break; 
 
	      case 1: 
		 OLC_MODE(d) = OEDIT_EXTRADESC_KEY; 
		 send_to_char(d->character, 
			      "Enter keywords, separated by spaces :-\r\n| "); 
		 return; 
 
	      case 2: 
		 OLC_MODE(d) = OEDIT_EXTRADESC_DESCRIPTION; 
		 SEND_TO_Q(d,"Enter the extra description: (/s saves /h for help)\r\n\r\n"); 
		 d->backstr = NULL; 
		 if (OLC_DESC(d)->description) 
		    { 
		    SEND_TO_Q(d, "%s", OLC_DESC(d)->description); 
		    d->backstr = str_dup(OLC_DESC(d)->description); 
		    } 
		 d->str = &OLC_DESC(d)->description; 
		 d->max_str = MAX_MESSAGE_LENGTH; 
		 d->mail_to = 0; 
		 OLC_VAL(d) = 1; 
		 return; 
 
	      case 3: 
		/*. Only go to the next descr if this one is finished .*/ 
		 if (OLC_DESC(d)->keyword && OLC_DESC(d)->description) 
		    { 
		    struct extra_descr_data *new_extra; 
 
		    if (OLC_DESC(d)->next) 
		       OLC_DESC(d) = OLC_DESC(d)->next; 
		    else  
		       { 
		      /* make new extra, attach at end */ 
		       CREATE(new_extra, struct extra_descr_data, 1); 
 
		       OLC_DESC(d)->next = new_extra; 
		       OLC_DESC(d) = OLC_DESC(d)->next; 
		       OLC_DESC(d)->next = NULL; 
		       OLC_DESC(d)->description = NULL; 
		       } 
		    } 
		/*. No break - drop into default case .*/ 
	      default: 
		 oedit_disp_extradesc_menu(d); 
		 return; 
	     } 
	  break; 
       case OEDIT_CDAM: 
	  vnumber = atoi(arg); 
	  GET_OBJ_CSLOTS(OLC_OBJ(d)) = MIN(vnumber,GET_OBJ_TSLOTS(OLC_OBJ(d)));
	  break; 
       case OEDIT_TDAM: 
	  vnumber = atoi(arg); 
	  GET_OBJ_TSLOTS(OLC_OBJ(d)) = MIN(vnumber,GET_OBJ_OSLOTS(OLC_OBJ(d)));
	  GET_OBJ_OSLOTS(OLC_OBJ(d)) = GET_OBJ_TSLOTS(OLC_OBJ(d));
	  break; 

       case OEDIT_ANTI: 
	  vnumber = atoi(arg); 
	  if ((vnumber < 0) || (vnumber > NUM_ANTI_FLAGS)) 
	     { 
	     oedit_disp_anti_menu(d); 
	     return; 
	     } 
	  else if (vnumber == 0) 
	     break; 
	  else  
	     { 
	     TOGGLE_BIT(GET_OBJ_ANTI(OLC_OBJ(d)), 1 << (vnumber - 1));
	     oedit_disp_anti_menu(d);
	     return;
	     } 
	  break;
       case OEDIT_SPELL_AFF:
	  vnumber=atoi(arg);
	  if(vnumber==0)
	     break;
	  if(!((vnumber<0)||(vnumber>NUM_AFF_FLAGS)))
	     {
	     vnumber = 1<<(vnumber -1);
	     if(IS_SET(OLC_OBJ(d)->obj_flags.bitvector,vnumber))
		REMOVE_BIT(OLC_OBJ(d)->obj_flags.bitvector,vnumber);
	     else
		SET_BIT(OLC_OBJ(d)->obj_flags.bitvector,vnumber);
	     }
	  oedit_disp_spellaff_menu(d);
	  return;

       default: 
	  mudlogf(BRF, LVL_BUILDER, TRUE,
		  "SYSERR: OLC: Reached default case in oedit_parse()!"); 
	  send_to_char(d->character, 
		       "Oops...REACHED OEDIT DEFAULT CASE, BUG!\r\n");
	  break; 
      } 
 
  /*. If we get here, we have changed something .*/ 
   OLC_VAL(d) = 1; /*. Has changed flag .*/ 
   oedit_disp_menu(d); 
} 
