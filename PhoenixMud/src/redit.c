/************************************************************************
 *  OasisOLC - redit.c                                         v1.5     *
 *  Copyright 1996 Harvey Gilpin.                                       *
 *  Original author: Levork                                             *
 ************************************************************************/
 
 
#include "../localHeader/conf.h" 
#include "../localHeader/sysdep.h" 
#include "structs.h" 
#include "comm.h" 
#include "buffer.h" 
#include "utils.h" 
#include "db.h" 
#include "boards.h" 
#include "olc.h" 
#include "dg_olc.h"
#include "constants.h" 
#include "queue.h"
/* List each room saved, was used for debugging. */
#if 0
#define REDIT_LIST     1
#endif


/*------------------------------------------------------------------------*/ 
/*. External data .*/ 
 
extern struct room_data *world; 
extern struct obj_data *obj_proto; 
extern struct char_data *mob_proto; 
extern struct zone_data *zone_table; 
extern room_rnum r_mortal_start_room; 
extern room_rnum r_immort_start_room; 
extern room_rnum r_frozen_start_room; 
extern room_vnum immort_start_room; 
extern room_vnum frozen_start_room; 
extern int top_of_zone_table; 
extern struct descriptor_data *descriptor_list;
extern struct char_data *character_list; 
extern struct queue_event *command_queue;
void trig_wait_event(void *info);

/*------------------------------------------------------------------------*/ 
/* function protos */ 
 
void redit_disp_teleport_menu(struct descriptor_data *d); 
void redit_disp_tele_flag_menu(struct descriptor_data * d);
void redit_disp_extradesc_menu(struct descriptor_data *d); 
void redit_disp_exit_menu(struct descriptor_data *d); 
void redit_disp_exit_flag_menu(struct descriptor_data *d); 
void redit_disp_flag_menu(struct descriptor_data *d); 
void redit_disp_sector_menu(struct descriptor_data *d); 
void redit_disp_menu(struct descriptor_data *d); 
void redit_parse(struct descriptor_data *d, char *arg); 
void redit_setup_new(struct descriptor_data *d); 
void redit_setup_existing(struct descriptor_data *d, int real_num); 
void redit_save_to_disk(int zone); 
void redit_save_internally(struct descriptor_data *d); 
void free_room(struct room_data *room); 
char *delete_doubledollar(char *string);
 
/*------------------------------------------------------------------------*/ 
 
#define  W_EXIT(room, num) (world[(room)].dir_option[(num)]) 
 
/*------------------------------------------------------------------------*\

  Utils and exported functions. 
\*------------------------------------------------------------------------*/ 
 
void redit_setup_new(struct descriptor_data *d) 
{ 
   struct teleport_data *new_tele;

   CREATE(OLC_ROOM(d), struct room_data, 1); 
   OLC_ROOM(d)->name = str_dup("An unfinished room"); 
   OLC_ROOM(d)->description = str_dup("You are in an unfinished room.\r\n"); 
   CREATE(new_tele, struct teleport_data, 1);
   OLC_ROOM(d)->tele = new_tele;
   OLC_ROOM(d)->tele->targ = 0;
   OLC_ROOM(d)->light = 0;
   OLC_ITEM_TYPE(d) = WLD_TRIGGER;
   redit_disp_menu(d); 
   OLC_VAL(d) = 0; 
} 
 
/*------------------------------------------------------------------------*/ 
 
void redit_setup_existing(struct descriptor_data *d, int real_num) 
{ 
   struct room_data *room; 
   int counter; 
   struct teleport_data *new_tele;
   struct trig_proto_list *proto, *fproto;

  /*. Build a copy of the room .*/ 
   CREATE (room, struct room_data, 1); 
   *room = world[real_num]; 
  /* allocate space for all strings  */ 
   room->name = str_dup(world[real_num].name ?
			world[real_num].name : "undefined");
   room->description = str_dup(world[real_num].description ?
			       world[real_num].description : "undefined\r\n");
 
  /* exits - alloc only if necessary */ 
   for (counter = 0; counter < NUM_OF_DIRS; counter++) 
      { 
      if (world[real_num].dir_option[counter]) 
	 { 
	 CREATE(room->dir_option[counter], struct room_direction_data, 1); 
	/* copy numbers over */ 
	 *room->dir_option[counter] = *world[real_num].dir_option[counter]; 
	/* malloc strings */ 
	 room->dir_option[counter]->general_description = (world[real_num].dir_option[counter]->general_description ? str_dup(world[real_num].dir_option[counter]->general_description) : NULL);
	 room->dir_option[counter]->keyword = (world[real_num].dir_option[counter]->keyword ? str_dup(world[real_num].dir_option[counter]->keyword) : NULL);
	 } 
      } 
   
  /*. Extra descriptions if necessary .*/  
   if (world[real_num].ex_description)  
      { 
      struct extra_descr_data *this, *temp, *temp2; 
      CREATE (temp, struct extra_descr_data, 1); 
      room->ex_description = temp; 
      for (this = world[real_num].ex_description; this; this = this->next) 
	 { 
	 temp->keyword = (this->keyword ? str_dup(this->keyword) : NULL);
	 temp->description = (this->description ? str_dup(this->description) :
			      NULL);
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
   CREATE(new_tele, struct teleport_data, 1);
   room->tele=new_tele;
   if(world[real_num].tele !=NULL)
      {
      room->tele->targ = world[real_num].tele->targ;
      room->tele->time = world[real_num].tele->time;
      room->tele->bitvector = world[real_num].tele->bitvector;
      room->tele->obj = world[real_num].tele->obj;
      room->tele->to_char = world[real_num].tele->to_char ? str_dup(world[real_num].tele->to_char) : NULL;
      room->tele->to_source_room = world[real_num].tele->to_source_room ? str_dup(world[real_num].tele->to_source_room) : NULL;
      room->tele->to_targ_room = world[real_num].tele->to_targ_room ? str_dup(world[real_num].tele->to_targ_room) : NULL;
      }
   else
      {
      room->tele->targ = 0;
      }
   if (SCRIPT(&world[real_num]))
      script_copy(room, &world[real_num], WLD_TRIGGER);
   proto = world[real_num].proto_script;
   struct trig_proto_list *last_proto = NULL;

   while (proto) 
      {
      CREATE(fproto, struct trig_proto_list, 1);
      fproto->vnum = proto->vnum;
      fproto->next = NULL;

      if (room->proto_script == NULL)
         room->proto_script = fproto;
      else
         last_proto->next = fproto;

      last_proto = fproto;
      proto = proto->next;
      }

  /*. Attatch room copy to players descriptor .*/ 
   OLC_ROOM(d) = room; 
   OLC_VAL(d) = 0; 
   OLC_ITEM_TYPE(d) = WLD_TRIGGER;
   dg_olc_script_copy(d);
   redit_disp_menu(d); 
} 
 
/*------------------------------------------------------------------------*/ 
       
#define ZCMD (zone_table[zone].cmd[cmd_no]) 
 
void redit_save_internally(struct descriptor_data *d) 
{ 
   int i=0, j=0, rroom_num=0, found = 0, zone=0, cmd_no=0; 
   struct queue_event *tmpq;
   struct room_data *new_world; 
   struct char_data *temp_ch; 
   struct obj_data *temp_obj; 
   struct descriptor_data *dsc;
   char *buf=get_buffer(256);
   struct wait_event_data {
      trig_data *trigger;
      void *go;
      int type;
   };
 
   rroom_num = real_room(OLC_NUM(d)); 

   if(OLC_ROOM(d)->tele->targ ==0)
      {
      if(OLC_ROOM(d)->tele->to_char)
	 free(OLC_ROOM(d)->tele->to_char);
      if(OLC_ROOM(d)->tele->to_source_room)
	 free(OLC_ROOM(d)->tele->to_source_room);
      if(OLC_ROOM(d)->tele->to_targ_room)
	 free(OLC_ROOM(d)->tele->to_targ_room);
      free(OLC_ROOM(d)->tele);
      OLC_ROOM(d)->tele = NULL;
      }


   if (rroom_num > 0)  
      { 
     /*. Room exists: move contents over then free and replace it .*/ 
      OLC_ROOM(d)->contents = world[rroom_num].contents; 
      OLC_ROOM(d)->people = world[rroom_num].people; 
      free_room(world + rroom_num); 
      world[rroom_num] = *OLC_ROOM(d); 
      world[rroom_num].proto_script = OLC_SCRIPT(d);
      } 
   else  
      { 
     /*. Room doesn't exist, hafta add it .*/ 
 
      CREATE(new_world, struct room_data, top_of_world + 2); 
 
     /* count thru world tables */ 
      for (i = 0; i <= top_of_world; i++)  
	 { 
	 if (!found) 
	    { 
	   /*. Is this the place? .*/ 
	    if (GET_ROOM_VNUM(i) > OLC_NUM(d))  
	       { 
	       found = TRUE; 
 
	       new_world[i]        = *(OLC_ROOM(d)); 
	       new_world[i].number = OLC_NUM(d); 
	       new_world[i].zone   = OLC_ZNUM(d); 
	       new_world[i].func   = NULL; 
	       new_world[i].proto_script = OLC_SCRIPT(d);
	       rroom_num  = i; 
	       new_world[i].light  = 0;
	      /* copy from world to new_world + 1 */ 
	       new_world[i + 1] = world[i]; 
	      /* people in this room must have their numbers moved */ 
	       for (temp_ch = world[i].people; temp_ch;
		    temp_ch = temp_ch->next_in_room) 
		  if (IN_ROOM(temp_ch) != NOWHERE) 
		     IN_ROOM(temp_ch) = i + 1; 
 
	      /* move objects */ 
	       for (temp_obj = world[i].contents; temp_obj;
		    temp_obj = temp_obj->next_content) 
		  if (IN_ROOM(temp_obj) != NOWHERE) 
		     IN_ROOM(temp_obj) = i + 1; 
	       
	       for(tmpq=command_queue;tmpq;tmpq=tmpq->next)
		  {
		  if(IS_SET(tmpq->flags,QUE_FUNCTION))
		     {
		     if(tmpq->function== trig_wait_event)
			{
			struct wait_event_data *trgev
			   =(struct wait_event_data *)tmpq->args[0];
			if(trgev->type==WLD_TRIGGER)
			   {
			   if((struct room_data *)trgev->go == &world[i])
			      {
				/*(struct room_data *)trgev->go = &new_world[i+1];*/
				trgev->go = &new_world[i+1];
			      }
			   }
			}
		     }
		  }
	       } 
	    else  
	       { 
	      /*.   Not yet placed, copy straight over .*/ 
	       new_world[i] = world[i]; 
	       for(tmpq=command_queue;tmpq;tmpq=tmpq->next)
		  {
		  if(IS_SET(tmpq->flags,QUE_FUNCTION))
		     {
		     if(tmpq->function== trig_wait_event)
			{
			struct wait_event_data *trgev
			   =(struct wait_event_data *)tmpq->args[0];
			if(trgev->type==WLD_TRIGGER)
			   {
			   if((struct room_data *)trgev->go == &world[i])
			      {
				/*(struct room_data *)trgev->go = &new_world[i];*/
				trgev->go = &new_world[i];
			      }
			   }
			}
		     }
		  }
	       } 
	    } 
	 else  
	    { 
	   /*. Already been found  .*/ 
  	   /* people in this room must have their in_rooms moved */ 
	    for (temp_ch = world[i].people; temp_ch;
		 temp_ch = temp_ch->next_in_room) 
	       if (IN_ROOM(temp_ch) != NOWHERE) 
		  IN_ROOM(temp_ch) = i + 1; 
 
	   /* move objects */ 
	    for (temp_obj = world[i].contents; temp_obj;
		 temp_obj = temp_obj->next_content) 
	       if (IN_ROOM(temp_obj) != NOWHERE) 
		  IN_ROOM(temp_obj) = i + 1; 
 
	    new_world[i + 1] = world[i]; 
	    for(tmpq=command_queue;tmpq;tmpq=tmpq->next)
	       {
	       if(IS_SET(tmpq->flags,QUE_FUNCTION))
		  {
		  if(tmpq->function== trig_wait_event)
		     {
		     struct wait_event_data *trgev
			=(struct wait_event_data *)tmpq->args[0];
		     if(trgev->type==WLD_TRIGGER)
			{
			if((struct room_data *)trgev->go == &world[i])
			   {
			     /*(struct room_data *)trgev->go = &new_world[i+1];*/
			     trgev->go = &new_world[i+1];
			   }
			}
		     }
		  }
	       }
	    } 
	 } 
      if (!found) 
	 { 
	/*. Still not found, insert at top of table .*/ 
	 new_world[i]        = *(OLC_ROOM(d)); 
	 new_world[i].number = OLC_NUM(d); 
	 new_world[i].zone   = OLC_ZNUM(d); 
	 new_world[i].func   = NULL; 
	 new_world[i].proto_script = OLC_SCRIPT(d);
	 rroom_num  = i; 
	 } 

      for(temp_ch=character_list;temp_ch;temp_ch=temp_ch->next)
	 {
	 if(IS_NPC(temp_ch)&&(rroom_num<=temp_ch->orig_room))
	    temp_ch->orig_room+=1;
	 }

     /* copy world table over */ 
      free(world); 
      world = new_world; 
      top_of_world++; 
 
     /*. Update zone table .*/ 
      for (zone = 0; zone <= top_of_zone_table; zone++) 
	 for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++) 
	    switch (ZCMD.command) 
	       { 
		case 'M': 
		case 'O': 
		   if (ZCMD.arg3 >= rroom_num) 
		      ZCMD.arg3++; 
		   break; 
		case 'D': 
		case 'R': 
		   if (ZCMD.arg1 >= rroom_num) 
		      ZCMD.arg1++; 
		   break;
		case 'V':
		   if(ZCMD.arg1==WLD_TRIGGER)
		      if (ZCMD.arg2 >= rroom_num) 
			 ZCMD.arg2++; 
		   break;
		case 'G': 
		case 'P': 
		case 'E': 
		case 'F':
		case 'Z':
		case '*': 
		   break; 
		default: 
		   mudlogf(BRF, LVL_BUILDER, TRUE,
			   "SYSERR: OLC: redit_save_internally: "
			   "Unknown comand :%c",ZCMD.command); 
		   break;
	       } 
 
     /* update load rooms, to fix creeping load room problem */ 
      if (rroom_num <= r_mortal_start_room) 
	 r_mortal_start_room++; 
      if (rroom_num <= r_immort_start_room) 
	 r_immort_start_room++; 
      if (rroom_num <= r_frozen_start_room) 
	 r_frozen_start_room++; 
 
     /*. Update world exits .*/ 
      for (i = 0; i < top_of_world + 1; i++) 
	 {
	 if(i==rroom_num)
	    continue;
	 for (j = 0; j < NUM_OF_DIRS; j++) 
	    if (W_EXIT(i, j)) 
	       if (W_EXIT(i, j)->to_room >= rroom_num) 
		  W_EXIT(i, j)->to_room+=1; 
	 }
 
      for(dsc=descriptor_list;dsc;dsc=dsc->next)
	 if((STATE(dsc) == CON_REDIT))
	    for (j = 0; j < NUM_OF_DIRS; j++)
	       if (OLC_ROOM(dsc)->dir_option[j])
		  if (OLC_ROOM(dsc)->dir_option[j]->to_room >= rroom_num)
		     OLC_ROOM(dsc)->dir_option[j]->to_room+=1;
      } 
   assign_triggers(&world[rroom_num], WLD_TRIGGER);
   olc_add_to_save_list(zone_table[OLC_ZNUM(d)].number, OLC_SAVE_ROOM); 
   free(zone_table[OLC_ZNUM(d)].nameLastMod);
   sprintf(buf,"%s - room",GET_NAME(d->character));
   zone_table[OLC_ZNUM(d)].nameLastMod = strdup(buf);
   release_buffer(buf);
   zone_table[OLC_ZNUM(d)].dateLastMod = time(0);
   olc_add_to_save_list(zone_table[OLC_ZNUM(d)].number, OLC_SAVE_ZONE);
} 
 
 
/*------------------------------------------------------------------------*/ 
 
void redit_save_to_disk(int zone_num) 
{ 
   int counter, counter2, realcounter; 
   FILE *fp; 
   struct room_data *room; 
   struct extra_descr_data *ex_desc; 
   char *buf=get_buffer(MAX_STRING_LENGTH);
   char *buf1,*buf2,*rflag,*rflag2,*teleflag;
   room_rnum rroom;
   bitvector_t temp_room_flags;

   if (zone_num < 0 || zone_num > top_of_zone_table) 
      {
      log("SYSERR: redit_save_to_disk: Invalid real zone passed!");
      return;
      }
   sprintf(buf, "%s/%ld.new", WLD_PREFIX, zone_table[zone_num].number); 
   if (!(fp = fopen(buf, "w+"))) 
      { 
      mudlogf(BRF, LVL_BUILDER, TRUE,"SYSERR: OLC: Cannot open room file!"); 
      release_buffer(buf);
      return; 
      } 
 
   buf1=get_buffer(MAX_STRING_LENGTH);
   buf2=get_buffer(MAX_STRING_LENGTH);
   fprintf(fp,"@Version: %d\n",CUR_WORLD_VER);
   for (counter = zone_table[zone_num].number * 100; 
	counter <= zone_table[zone_num].top; 
	counter++)  
      { 
      realcounter = real_room(counter); 
      if (realcounter >= 0)  
	 {  
	 room = (world + realcounter); 

#if defined(REDIT_LIST)
	 log("OLC: Saving room %d.", room->number);
#endif

	/*. Remove the '\r\n' sequences from description .*/ 
	 strcpy(buf1, room->description ? room->description : "Empty"); 
	 strip_string(buf1); 
 
         rflag=get_buffer(SMALL_BUFSIZE);
         rflag2=get_buffer(SMALL_BUFSIZE);
         temp_room_flags=room->room_flags;
	 REMOVE_BIT(room->room_flags,ROOM_BFS_MARK);
	 REMOVE_BIT(room->room_flags,ROOM_HOUSE);
	 REMOVE_BIT(room->room_flags,ROOM_ATRIUM);
	 REMOVE_BIT(room->room_flags,ROOM_HOUSE_CRASH);
	 REMOVE_BIT(room->room_flags,ROOM_OLC);
	 REMOVE_BIT(room->room_flags,ROOM_DONATION);
         flagascii_conv(rflag,room->room_flags);
         flagascii_conv(rflag2,room->room2_flags);
         room->room_flags=temp_room_flags;
	/*. Build a buffer ready to save .*/ 
	 fprintf(fp, "#%d\n"
		 "%s~\n"
		 "%s~\n"
		 "%ld %s %s %d\n", 
		 counter, room->name ? room->name : "undefined",
		 buf1, 
		 zone_table[room->zone].number, 
		 rflag, rflag2,
		 room->sector_type 
	    ); 
         release_buffer(rflag2);
	 release_buffer(rflag);

	/*. Handle exits .*/ 
	 for (counter2 = 0; counter2 < NUM_OF_DIRS; counter2++)  
	    { 
	    if (room->dir_option[counter2])  
	       { 
	       int temp_door_flag; 
 
	      /*. Again, strip out the crap .*/ 
	       if (room->dir_option[counter2]->general_description) 
		  { 
		  strcpy(buf1,room->dir_option[counter2]->general_description);
		  strip_string(buf1); 
		  } 
	       else 
		  *buf1 = 0; 
 
	      /*. Figure out door flag .*/ 
	       temp_door_flag=room->dir_option[counter2]->exit_info;
 
	       if(temp_door_flag & EX_PICKPROOF) 
		  SET_BIT(temp_door_flag,EX_ISDOOR); 
	       if(temp_door_flag&EX_CLOSED)
		  REMOVE_BIT(temp_door_flag,EX_CLOSED);
	       if(temp_door_flag&EX_LOCKED)
		  REMOVE_BIT(temp_door_flag,EX_LOCKED);

	      /*. Check for keywords .*/ 
	       if(room->dir_option[counter2]->keyword) 
		  strcpy(buf2, room->dir_option[counter2]->keyword); 
	       else 
		  *buf2 = '\0'; 
                

	      /*. Ok, now build a buffer to output to file .*/ 
	       if(room->dir_option[counter2]->to_room==-1)
		  rroom=-1;
	       else
		  rroom=GET_ROOM_VNUM(room->dir_option[counter2]->to_room);

	       fprintf(fp, "D%d\n%s~\n%s~\n%d %ld %ld %d\n", 
		       counter2, buf1, buf2, temp_door_flag, 
		       room->dir_option[counter2]->key, 
		       rroom,room->dir_option[counter2]->lcklevl
		  ); 
	       } 
	    } 
	 
	 for(counter2=0;counter2 < NUM_ORE_SLOTS;counter2++)
	    {
	    if(room->ore_types[counter2]!=NOTHING)
	       fprintf(fp, "O\n%d %d %d\n",counter2,
		       room->ore_types[counter2],
		       room->ore_percent[counter2]);
	    }
	 if (room->ex_description)  
	    { 
	    for(ex_desc=room->ex_description;ex_desc;ex_desc=ex_desc->next)
	       { 
	      /*. Home straight, just deal with extras descriptions..*/ 
	       if(str_cmp(ex_desc->keyword,"undefined")==0)
		  continue;
	       strcpy(buf1, ex_desc->description); 
	       strip_string(buf1); 
	       fprintf(fp, "E\n%s~\n%s~\n", ex_desc->keyword,buf1); 
	       } 
	    } 
	 if ((room->tele != NULL) && (room->tele->targ > 0)) 
	    { 
	    teleflag=get_buffer(SMALL_BUFSIZE);
	    flagascii_conv(teleflag,room->tele->bitvector);
	    fprintf(fp, "T\n%ld %d %ld %s\n",room->tele->targ,room->tele->time,
		    room->tele->obj, teleflag); 
	    release_buffer(teleflag);
	    if(room->tele->to_char||
	       room->tele->to_source_room||
	       room->tele->to_targ_room)
	       {
	       fprintf(fp,"t\n");
	       fprintf(fp,"%s~\n",room->tele->to_char?room->tele->to_char:"");
	       fprintf(fp,"%s~\n",room->tele->to_source_room?room->tele->to_source_room:"");
	       fprintf(fp,"%s~\n",room->tele->to_targ_room?room->tele->to_targ_room:"");	       
	       }
	    } 
	 fprintf(fp, "S\n"); 
	 script_save_to_disk(fp, room, WLD_TRIGGER);
	 } 
      } 
  /* write final line and close */ 
   fprintf(fp, "$~\n"); 
   fclose(fp); 
   sprintf(buf2, "%s/%ld.wld", WLD_PREFIX, zone_table[zone_num].number);
  /*
   * We're fubar'd if we crash between the two lines below.
   */
   remove(buf2);
   rename(buf, buf2);
   
   olc_remove_from_save_list(zone_table[zone_num].number, OLC_SAVE_ROOM); 
   release_buffer(buf2);
   release_buffer(buf1);
   release_buffer(buf);
} 
 
/*------------------------------------------------------------------------*/ 
 
void free_room(struct room_data *room) 
{ 
   int i; 
   struct extra_descr_data *this, *next; 
 
   if (room->name) 
      free(room->name); 
   if (room->description) 
      free(room->description); 
   if (room->tele)
      {
      if(room->tele->to_char)
	 free(room->tele->to_char);
      if(room->tele->to_source_room)
	 free(room->tele->to_source_room);
      if(room->tele->to_targ_room)
	 free(room->tele->to_targ_room);
      free(room->tele);
      room->tele=NULL;
      }
  /*. Free exits .*/ 
   for (i = 0; i < NUM_OF_DIRS; i++) 
      { 
      if (room->dir_option[i]) 
	 { 
	 if (room->dir_option[i]->general_description) 
	    free(room->dir_option[i]->general_description); 
	 if (room->dir_option[i]->keyword) 
	    free(room->dir_option[i]->keyword); 
	 } 
      free(room->dir_option[i]); 
      } 
 
  /*. Free extra descriptions .*/ 
   for (this = room->ex_description; this; this = next) 
      { 
      next = this->next; 
      if (this->keyword) 
	 free(this->keyword); 
      if (this->description) 
	 free(this->description); 
      free(this); 
      } 
} 
 
/************************************************************************** 
 Menu functions  
 **************************************************************************/ 
 
void redit_disp_ore_menu(struct descriptor_data * d) 
{
   char *ore_name=get_buffer(256);
   int i,j;

   get_char_cols(d->character); 
   send_to_char(d->character, "[H[J");

   for(i=0;i<NUM_ORE_SLOTS;i++)
      {
      if(OLC_ROOM(d)->ore_types[i]==NOTHING)
	 sprintf(ore_name,"Nothing");
      else
	 {
	 sprintf(ore_name,"UNKNOWN ORE: %d",OLC_ROOM(d)->ore_types[i]);
	 
	 for(j=0;ore_types[j].ore_vnum!=NOTHING;j++)
	    {
	    if(ore_types[j].ore_vnum == OLC_ROOM(d)->ore_types[i])
	       {
	       strcpy(ore_name,ore_types[j].ore_string);
	       break;
	       }
	    }
	 }
      send_to_char(d->character, "%s%d%s) %s%s at %d%%\r\n",
	      grn,i+1,nrm,cyn,ore_name,OLC_ROOM(d)->ore_percent[i]);
      }
   send_to_char(d->character, "Enter choice, 0 to quit : "); 
   OLC_MODE(d) = REDIT_OREVAL; 
   release_buffer(ore_name);
}

void redit_disp_ore_type(struct descriptor_data * d) 
{
   int i;

   get_char_cols(d->character); 
   send_to_char(d->character, "[H[J"); 

   for(i=0;ore_types[i].ore_vnum!=NOTHING;i++)
      {
      send_to_char(d->character, "%s%d%s) %s%s\r\n",
	      grn, i+1, nrm, cyn,ore_types[i].ore_string);
      }
   send_to_char(d->character, "Enter choice : ");

   OLC_MODE(d) = REDIT_ORETYPE; 
}

void redit_disp_ore_percent(struct descriptor_data * d) 
{
   get_char_cols(d->character); 
   send_to_char(d->character, "Enter percentage : ");

   OLC_MODE(d) = REDIT_OREPERCENT; 
}

void redit_disp_teleport_menu(struct descriptor_data * d) 
{  
   char *buf1=get_buffer(SMALL_BUFSIZE);
   int obj_num;

   obj_num=real_object(OLC_ROOM(d)->tele->obj);
   sprintbit(OLC_ROOM(d)->tele->bitvector,teleport_bits,buf1);
   get_char_cols(d->character); 
   send_to_char(d->character, "[H[J" 
	   "%s1%s) Teleport to : %s%ld\r\n" 
	   "%s2%s) Delay       : %s%d\r\n" 
	   "%s3%s) Flags       : %s%s\r\n" 
	   "%s4%s) Object      : %s[%ld] %s (%s flag is set)\r\n" 
	   "%s5%s) To Char Msg :\r\n %s%s\r\n" 
	   "%s6%s) To Src Room Msg :\r\n %s%s\r\n" 
	   "%s7%s) To Tart Room Msg :\r\n %s%s\r\n" 
	   "%s8%s) Purge teleport.\r\n" 
	   "Enter choice, 0 to quit : ", 
 
	   grn, nrm, cyn, OLC_ROOM(d)->tele->targ, 
	   grn, nrm, cyn, OLC_ROOM(d)->tele->time, 
	   grn, nrm, cyn, buf1, 
	   grn, nrm, cyn, OLC_ROOM(d)->tele->obj,
	   (obj_num>0)?obj_proto[obj_num].short_description:"None",
	   (IS_SET(OLC_ROOM(d)->tele->bitvector,TELE_NOOBJ)?"If not have obj" :
	    (IS_SET(OLC_ROOM(d)->tele->bitvector,TELE_OBJ)? "If has obj":
	     "No")),
	   grn, nrm, cyn, OLC_ROOM(d)->tele->to_char?OLC_ROOM(d)->tele->to_char:"NONE",
	   grn, nrm, cyn, OLC_ROOM(d)->tele->to_source_room?OLC_ROOM(d)->tele->to_source_room:"NONE",
	   grn, nrm, cyn, OLC_ROOM(d)->tele->to_targ_room?OLC_ROOM(d)->tele->to_targ_room:"NONE",
	   grn, nrm 
      ); 
 
   OLC_MODE(d) = REDIT_TELEPORT_MENU; 
   release_buffer(buf1);
} 
 
/* For room flags */ 
void redit_disp_tele_flag_menu(struct descriptor_data * d) 
{ 
   char *buf2=get_buffer(MAX_STRING_LENGTH);
   int counter, columns = 0; 
 
   get_char_cols(d->character); 
   send_to_char(d->character, "[H[J"); 
   for (counter = 0; counter < NUM_TELEPORT; counter++)  
      { 
      send_to_char(d->character, "%s%2d%s) %-20.20s ", 
	      grn, counter + 1, nrm, teleport_bits[counter]); 
      if(!(++columns % 2)) 
	 send_to_char(d->character, "\r\n"); 
      } 
   sprintbit(OLC_ROOM(d)->tele->bitvector, teleport_bits, buf2); 
   send_to_char(d->character, 
	   "\r\nTeleport flags: %s%s%s\r\n" 
	   "Enter teleport flags, 0 to quit : ", 
	   cyn, buf2, nrm 
      ); 
   OLC_MODE(d) = REDIT_TELEPORT_FLAG; 
   release_buffer(buf2);
} 

/* For extra descriptions */ 
void redit_disp_extradesc_menu(struct descriptor_data * d) 
{ 
   struct extra_descr_data *extra_desc = OLC_DESC(d); 
   
   send_to_char(d->character, "[H[J" 
	   "%s1%s) Keyword: %s%s\r\n" 
	   "%s2%s) Description:\r\n%s%s\r\n" 
	   "%s3%s) Goto next description: ", 
 	   grn, nrm, yel, extra_desc->keyword ? extra_desc->keyword : "<NONE>",
	   grn, nrm, yel, 
		extra_desc->description ?  extra_desc->description : "<NONE>", 
	   grn, nrm 
      ); 
   
   send_to_char(d->character, "%sEnter choice (0 to quit) : ",
		!extra_desc->next ? "<NOT SET>\r\n" : "Set.\r\n");
   OLC_MODE(d) = REDIT_EXTRADESC_MENU; 
} 
 
/* For exits */ 
void redit_disp_exit_menu(struct descriptor_data * d) 
{ 
   char *buf1=get_buffer(MAX_STRING_LENGTH);
   char *buf2=get_buffer(MAX_STRING_LENGTH);
   room_rnum rroom;

  /* if exit doesn't exist, alloc/create it */ 
   if(!OLC_EXIT(d)) 
      {
      CREATE(OLC_EXIT(d), struct room_direction_data, 1); 
      OLC_EXIT(d)->to_room=-1;
      }      

  /* weird door handling! */ 
   if (IS_SET(OLC_EXIT(d)->exit_info, EX_ISDOOR)) 
      { 
      if (IS_SET(OLC_EXIT(d)->exit_info, EX_PICKPROOF)) 
	 strcpy(buf2, "Pickproof"); 
      else 
	 strcpy(buf2, "Is a door"); 
      } 
   else 
      strcpy(buf2, "No door"); 
   
   if (IS_SET(OLC_EXIT(d)->exit_info, EX_SECRET)) 
      strcat(buf2," {secret}");
   if (IS_SET(OLC_EXIT(d)->exit_info, EX_HIDDEN)) 
      strcat(buf2," <hidden>");
   if (IS_SET(OLC_EXIT(d)->exit_info, EX_FLY)) 
      strcat(buf2," fly");
   if (IS_SET(OLC_EXIT(d)->exit_info, EX_DROP)) 
      strcat(buf2," drop");
   if (IS_SET(OLC_EXIT(d)->exit_info, EX_AUTOCLOSE)) 
      strcat(buf2," autoclose");
   if (IS_SET(OLC_EXIT(d)->exit_info, EX_WIZLOCK)) 
      strcat(buf2," wizlocked");
   if (IS_SET(OLC_EXIT(d)->exit_info, EX_NOPASS))
     strcat(buf2, " No-Pass");

   get_char_cols(d->character); 
   if(OLC_EXIT(d)->to_room==-1)
      {
      rroom=-1;
      strcpy(buf1,"[-1] Room Desc for 'look <dir>'");
      }
   else
      {
      rroom=GET_ROOM_VNUM(OLC_EXIT(d)->to_room);
      sprintf(buf1,"[%ld] %s",rroom, world[OLC_EXIT(d)->to_room].name);
      }

   send_to_char(d->character, "[H[J" 
	   "%s1%s) Exit to     : %s%s\r\n" 
	   "%s2%s) Description :-\r\n%s%s\r\n" 
	   "%s3%s) Door name   : %s%s\r\n" 
	   "%s4%s) Key         : %s%ld\r\n" 
	   "%s5%s) Door flags  : %s%s\r\n" 
	   "%s6%s) Purge exit.\r\n" 
	   "Enter choice, 0 to quit : ", 
 
	   grn, nrm, cyn, buf1, 
	   grn, nrm, yel, OLC_EXIT(d)->general_description ? OLC_EXIT(d)->general_description : "<NONE>", 
	   grn, nrm, yel, OLC_EXIT(d)->keyword ? OLC_EXIT(d)->keyword : "<NONE>", 
	   grn, nrm, cyn, OLC_EXIT(d)->key, 
	   grn, nrm, cyn, buf2, grn, nrm 
      ); 
 
   OLC_MODE(d) = REDIT_EXIT_MENU; 
   release_buffer(buf2);
   release_buffer(buf1);
} 
 
/* For exit flags */ 
void redit_disp_exit_flag_menu(struct descriptor_data * d) 
{ 
   get_char_cols(d->character); 
   send_to_char(d->character, "%s0%s) No door\r\n" 
	   "%s1%s) Closeable door\r\n" 
	   "%s2%s) Pickproof\r\n" 
	   "%s3%s) Secret\r\n"
	   "%s4%s) Fly\r\n"
	   "%s5%s) Drop\r\n"
	   "%s6%s) AutoClose\r\n"
           "%s7%s) Hidden\r\n"
	   "%s8%s) No-Pass\r\n"
	   "Enter choice : ", 
	   grn, nrm, 
	   grn, nrm, 
	   grn, nrm, 
	   grn, nrm, 
	   grn, nrm, 
	   grn, nrm, 
	   grn, nrm, 
	   grn, nrm, 
           grn, nrm
      ); 
} 
 
/* For room flags */ 
void redit_disp_flag_menu(struct descriptor_data * d) 
{ 
   char *buf2=get_buffer(MAX_STRING_LENGTH);
   char *buf3=get_buffer(MAX_STRING_LENGTH);
   int counter, columns = 0; 
 
   get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
   send_to_char(d->character, "[H[J");
#endif

   send_to_char(d->character, "\r\n"); 
   for (counter = 0; counter < NUM_ROOM_FLAGS; counter++)
      {
      send_to_char(d->character, "%s%2d%s) %-18.18s ",
	      grn, counter + 1, nrm, room_bits[counter]
	 );
      if(!(++columns % 3)) 
	 send_to_char(d->character, "\r\n"); 
      } 
   for (counter = 0; counter < NUM_ROOM2_FLAGS; counter++)
      {
      send_to_char(d->character, "%s%2d%s) %-18.18s ",   
	      grn, counter + 1 + NUM_ROOM_FLAGS, nrm, 
	      room2_bits[counter]
	 );
      if(!(++columns % 3))
	 send_to_char(d->character, "\r\n");
      }

   sprintbit(OLC_ROOM(d)->room_flags, room_bits, buf2); 
   sprintbit(OLC_ROOM(d)->room2_flags, room2_bits, buf3);
   send_to_char(d->character, "\r\nRoom flags: %s%s%s%s\r\n" 
		"Enter room flags, 0 to quit : ", 
		cyn, !strcmp(buf2, "NOBITS ") &&
		      strcmp(buf3, "NOBITS ")?"":buf2, 
		!strcmp(buf3, "NOBITS ")?"":buf3, nrm
      ); 
   OLC_MODE(d) = REDIT_FLAGS; 
   release_buffer(buf3);
   release_buffer(buf2);
} 

/* for sector type */ 
void redit_disp_sector_menu(struct descriptor_data * d) 
{ 
   int counter, columns = 0; 
 
   send_to_char(d->character, "[H[J"); 
   for (counter = 0; counter < NUM_ROOM_SECTORS; counter++) 
      { 
      send_to_char(d->character, "%s%2d%s) %-20.20s ", 
	      grn, counter, nrm, sector_types[counter]); 
      if(!(++columns % 2)) 
	 send_to_char(d->character, "\r\n"); 
      } 
   send_to_char(d->character, "\r\nEnter sector type : "); 
   OLC_MODE(d) = REDIT_SECTOR; 
} 
 
/* the main menu */ 
void redit_disp_menu(struct descriptor_data * d) 
{ 
   char *buf1=get_buffer(MAX_STRING_LENGTH);
   char *buf2=get_buffer(MAX_STRING_LENGTH);
   char *buf3=get_buffer(MAX_STRING_LENGTH);
   struct room_data *room; 
 
   get_char_cols(d->character); 
   room = OLC_ROOM(d); 
 
   sprintbit((long) room->room_flags, room_bits, buf1); 
   sprintbit((long) room->room2_flags, room2_bits, buf2);
   sprinttype(room->sector_type, sector_types, buf3); 
   send_to_char(d->character,
	   "[H[J" 
	   "-- Room number : [%s%d%s]   Room zone: [%s%ld%s]\r\n" 
	   "%s1%s) Name        : %s%s\r\n" 
	   "%s2%s) Description :\r\n%s%s" 
	   "%s3%s) Room flags  : %s%s\r\n" 
             "%s   Room2 flags : %s%s\r\n"
	   "%s4%s) Sector type : %s%s\r\n", 
	   cyn, OLC_NUM(d), nrm, 
	   cyn, zone_table[OLC_ZNUM(d)].number, nrm, 
	   grn, nrm, yel, room->name, 
	   grn, nrm, yel, room->description, 
	   grn, nrm, cyn, buf1,
                nrm, cyn, buf2, 
	   grn, nrm, cyn, buf3);
  /* NORTH */
   if(room->dir_option[NORTH])
      {
      if(room->dir_option[NORTH]->to_room==-1)
	 {
	 strcpy(buf1,"[-1] Room Desc for 'look <dir>'");
	 }
      else
	 {
	 sprintf(buf1,"[%ld] %s",
		 GET_ROOM_VNUM(room->dir_option[NORTH]->to_room), 
		 world[room->dir_option[NORTH]->to_room].name);
	 }
      }
   else
      {
      strcpy(buf1,"<NONE>");
      }
   send_to_char(d->character,"%s5%s) Exit north  : %s%s\r\n",grn,nrm,cyn,buf1);
  /* EAST */
   if(room->dir_option[EAST])
      {
      if(room->dir_option[EAST]->to_room==-1)
	 {
	 strcpy(buf1,"[-1] Room Desc for 'look <dir>'");
	 }
      else
	 {
	 sprintf(buf1,"[%ld] %s",
		 GET_ROOM_VNUM(room->dir_option[EAST]->to_room), 
		 world[room->dir_option[EAST]->to_room].name);
	 }
      }
   else
      {
      strcpy(buf1,"<NONE>");
      }
   send_to_char(d->character,"%s6%s) Exit east   : %s%s\r\n",grn,nrm,cyn,buf1);
  /* SOUTH */
   if(room->dir_option[SOUTH])
      {
      if(room->dir_option[SOUTH]->to_room==-1)
	 {
	 strcpy(buf1,"[-1] Room Desc for 'look <dir>'");
	 }
      else
	 {
	 sprintf(buf1,"[%ld] %s",
		 GET_ROOM_VNUM(room->dir_option[SOUTH]->to_room), 
		 world[room->dir_option[SOUTH]->to_room].name);
	 }
      }
   else
      {
      strcpy(buf1,"<NONE>");
      }
   send_to_char(d->character,"%s7%s) Exit south  : %s%s\r\n",grn,nrm,cyn,buf1);
  /* WEST */
   if(room->dir_option[WEST])
      {
      if(room->dir_option[WEST]->to_room==-1)
	 {
	 strcpy(buf1,"[-1] Room Desc for 'look <dir>'");
	 }
      else
	 {
	 sprintf(buf1,"[%ld] %s",
		 GET_ROOM_VNUM(room->dir_option[WEST]->to_room), 
		 world[room->dir_option[WEST]->to_room].name);
	 }
      }
   else
      {
      strcpy(buf1,"<NONE>");
      }
   send_to_char(d->character,"%s8%s) Exit west   : %s%s\r\n",grn,nrm,cyn,buf1);
  /* UP */
   if(room->dir_option[UP])
      {
      if(room->dir_option[UP]->to_room==-1)
	 {
	 strcpy(buf1,"[-1] Room Desc for 'look <dir>'");
	 }
      else
	 {
	 sprintf(buf1,"[%ld] %s",
		 GET_ROOM_VNUM(room->dir_option[UP]->to_room), 
		 world[room->dir_option[UP]->to_room].name);
	 }
      }
   else
      {
      strcpy(buf1,"<NONE>");
      }
   send_to_char(d->character,"%s9%s) Exit up     : %s%s\r\n",grn,nrm,cyn,buf1);
  /* DOWN */
   if(room->dir_option[DOWN])
      {
      if(room->dir_option[DOWN]->to_room==-1)
	 {
	 strcpy(buf1,"[-1] Room Desc for 'look <dir>'");
	 }
      else
	 {
	 sprintf(buf1,"[%ld] %s",
		 GET_ROOM_VNUM(room->dir_option[DOWN]->to_room), 
		 world[room->dir_option[DOWN]->to_room].name);
	 }
      }
   else
      {
      strcpy(buf1,"<NONE>");
      }
   send_to_char(d->character,"%sA%s) Exit down   : %s%s\r\n",grn,nrm,cyn,buf1);

   send_to_char(d->character, 
	   "%sB%s) Extra descriptions menu: %s%s\r\n" 
	   "%sC%s) Teleport    : %s%ld\r\n" 
	   "%sD%s) Script      : %s%s\r\n"
	   "%sE%s) ORE         : %s%s\r\n"
	   "%sQ%s) Quit\r\n" 
	   "Enter choice : ", 
	   grn, nrm, cyn, (room->ex_description?"SET":"NONE"),
	   grn, nrm, cyn, room->tele->targ, 
	   grn, nrm, cyn, room->proto_script?"Set.":"Not Set.",
	   grn, nrm, cyn, room->ore_types[0]?"Set.(set the MINE flag!)":"Not Set.",
	   grn, nrm 
      ); 
 
   OLC_MODE(d) = REDIT_MAIN_MENU; 
   release_buffer(buf3);
   release_buffer(buf2);
   release_buffer(buf1);
} 
 
 
 
/************************************************************************** 
  The main loop 
 **************************************************************************/ 
 
void redit_parse(struct descriptor_data * d, char *arg) 
{ 
   int vnumber; 


   switch (OLC_MODE(d)) 
      { 
       case REDIT_CONFIRM_SAVESTRING: 
	  switch (*arg) 
	     { 
	      case 'y': 
	      case 'Y': 
		 redit_save_internally(d); 
		 mudlogf(CMP, MAX(LVL_BUILDER,GET_INVIS_LEV(d->character)),
			TRUE,"OLC: %s has edited room %d",
			 GET_NAME(d->character), OLC_NUM(d)); 
		/*. Do NOT free strings! just the room structure .*/ 
		 cleanup_olc(d, CLEANUP_STRUCTS); 
		 send_to_char(d->character, "Room saved to memory.\r\n"); 
		 break; 
	      case 'n': 
	      case 'N': 
		/* free everything up, including strings etc */ 
		 cleanup_olc(d, CLEANUP_ALL); 
		 break; 
	      default: 
		 send_to_char(d->character, "Invalid choice!\r\n"
			      "Do you wish to save this room internally? : "); 
		 break; 
	     } 
	  return; 
 
       case REDIT_MAIN_MENU: 
	  switch (*arg) 
	     { 
	      case 'q': 
	      case 'Q': 
		 if (OLC_VAL(d)) 
		    { 
		   /*. Something has been modified .*/ 
		    send_to_char(d->character, "Do you wish to save this room internally? : "); 
		    OLC_MODE(d) = REDIT_CONFIRM_SAVESTRING; 
		    } 
		 else 
		    cleanup_olc(d, CLEANUP_ALL); 
		 return; 
	      case '1': 
		 send_to_char(d->character, "Enter room name:-\r\n| "); 
		 OLC_MODE(d) = REDIT_NAME; 
		 break; 
	      case '2': 
		 OLC_MODE(d) = REDIT_DESC; 
		 SEND_TO_Q(d,"\x1B[H\x1B[J"); 
		 SEND_TO_Q(d,"Enter room description: (/s saves /h for help)\r\n\r\n"); 
		 d->backstr = NULL; 
		 if (OLC_ROOM(d)->description) 
		    { 
		    SEND_TO_Q(d, "%s", OLC_ROOM(d)->description); 
		    d->backstr = str_dup(OLC_ROOM(d)->description); 
		    } 
		 d->str = &OLC_ROOM(d)->description; 
		 d->max_str = MAX_ROOM_DESC; 
		 d->mail_to = 0; 
		 OLC_VAL(d) = 1; 
		 break; 
	      case '3': 
		 redit_disp_flag_menu(d); 
		 break; 
	      case '4': 
		 redit_disp_sector_menu(d); 
		 break; 
	      case '5': 
		 OLC_VAL(d) = NORTH; 
		 redit_disp_exit_menu(d); 
		 break; 
	      case '6': 
		 OLC_VAL(d) = EAST; 
		 redit_disp_exit_menu(d); 
		 break; 
	      case '7': 
		 OLC_VAL(d) = SOUTH; 
		 redit_disp_exit_menu(d); 
		 break; 
	      case '8': 
		 OLC_VAL(d) = WEST; 
		 redit_disp_exit_menu(d); 
		 break; 
	      case '9': 
		 OLC_VAL(d) = UP; 
		 redit_disp_exit_menu(d); 
		 break; 
	      case 'a': 
	      case 'A': 
		 OLC_VAL(d) = DOWN; 
		 redit_disp_exit_menu(d); 
		 break; 
	      case 'b': 
	      case 'B': 
		/* if extra desc doesn't exist . */ 
		 if (!OLC_ROOM(d)->ex_description) 
		    { 
		    CREATE(OLC_ROOM(d)->ex_description, struct extra_descr_data, 1); 
		    OLC_ROOM(d)->ex_description->next = NULL; 
		    } 
		 OLC_DESC(d) = OLC_ROOM(d)->ex_description; 
		 redit_disp_extradesc_menu(d); 
		 break; 
	      case 'c': 
	      case 'C': 
		 redit_disp_teleport_menu(d); 
		 break; 
	      case 'd':
	      case 'D':
                 if(!PRF2_FLAGGED(d->character,PRF2_DG_ATTACH))
                    {
                    redit_disp_menu(d); 
                    send_to_char(d->character, "You don't have permission to attach scripts!\r\n: ");
                    return;
                    }
		 OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_MAIN_MENU;
		 dg_script_menu(d);
		 return;
	      case 'e':
	      case 'E':
		 OLC_MODE(d)=REDIT_OREVAL;
		 redit_disp_ore_menu(d);
		 return;		    
	      default: 
		 send_to_char(d->character, "Invalid choice!"); 
		 redit_disp_menu(d); 
		 break; 
	     } 
	  return; 

       case REDIT_OREVAL:
	  vnumber=atoi(arg);
	  if(vnumber<1 || vnumber >NUM_ORE_SLOTS)
	     break;
	  OLC_VAL(d)=vnumber;
	  redit_disp_ore_type(d);
	  return;
	  
	  break;
       case REDIT_ORETYPE:
	  vnumber=atoi(arg);
	  if(vnumber==0)
	     {
	     OLC_ROOM(d)->ore_types[OLC_VAL(d)-1] =NOTHING;
	     OLC_ROOM(d)->ore_percent[OLC_VAL(d)-1] =0;
	     redit_disp_ore_menu(d);
	     return;
	     }
	  if(vnumber<1 || vnumber>=NUM_ORE_TYPES)
	     {
	     redit_disp_ore_type(d);
	     return;
	     }
	  OLC_ROOM(d)->ore_types[OLC_VAL(d)-1] =ore_types[vnumber-1].ore_vnum;
	  redit_disp_ore_percent(d);
	  return;
	  
	  break;

       case REDIT_OREPERCENT:
	  vnumber=atoi(arg);
	  if(vnumber<1 || vnumber>=80)
	     {
	     redit_disp_ore_percent(d);
	     return;
	     }
	  OLC_ROOM(d)->ore_percent[OLC_VAL(d)-1] =vnumber;
	  redit_disp_ore_menu(d);
	  return;
	  break;

       case OLC_SCRIPT_EDIT:
	  if (dg_script_edit_parse(d, arg)) 
	     {
	     return;
	     }
	  break;
	  
       case REDIT_NAME: 
	  if (OLC_ROOM(d)->name) 
	     free(OLC_ROOM(d)->name); 
	  if (strlen(arg) > MAX_ROOM_NAME) 
	     arg[MAX_ROOM_NAME -1] = '\0'; 
	  OLC_ROOM(d)->name = str_dup((arg && *arg) ? arg : "undefined");
	  break; 

       case REDIT_DESC: 
	 /* we will NEVER get here */ 
	  mudlogf(BRF,LVL_BUILDER,TRUE,
		 "SYSERR: Reached REDIT_DESC case in parse_redit"); 
	  break; 
 
       case REDIT_FLAGS: 
	  vnumber = atoi(arg); 
	  if ((vnumber < 0) || (vnumber > NUM_ROOM_FLAGS+NUM_ROOM2_FLAGS)) 
	     { 
	     send_to_char(d->character, "That's not a valid choice!\r\n"); 
	     redit_disp_flag_menu(d); 
	     } 
	  else if (vnumber == 0) 
	     break; 
	  else 
	     { 
	    /* toggle bits */ 
             if (vnumber <= NUM_ROOM_FLAGS)
		TOGGLE_BIT(OLC_ROOM(d)->room_flags, 1 << (vnumber - 1));
             else
		TOGGLE_BIT(OLC_ROOM(d)->room2_flags,
			 1 << (vnumber - 1 - NUM_ROOM_FLAGS));
	     redit_disp_flag_menu(d); 
	     } 
	  return; 
 
       case REDIT_SECTOR: 
	  vnumber = atoi(arg); 
	  if (vnumber < 0 || vnumber >= NUM_ROOM_SECTORS) 
	     { 
	     send_to_char(d->character, "Invalid choice!"); 
	     redit_disp_sector_menu(d); 
	     return; 
	     } 
	  else  
	     OLC_ROOM(d)->sector_type = vnumber; 
	  break; 
 
       case REDIT_EXIT_MENU: 
	  switch (*arg) 
	     { 
	      case '0': 
		 break; 
	      case '1': 
		 OLC_MODE(d) = REDIT_EXIT_NUMBER; 
		 send_to_char(d->character, "Exit to room number : "); 
		 return; 
	      case '2': 
		 OLC_MODE(d) = REDIT_EXIT_DESCRIPTION; 
		 SEND_TO_Q(d,"Enter exit description: (/s saves /h for help)\r\n\r\n"); 
		 d->backstr = NULL; 
		 if (OLC_EXIT(d)->general_description) 
		    { 
		    SEND_TO_Q(d, "%s", OLC_EXIT(d)->general_description); 
		    d->backstr = str_dup(OLC_EXIT(d)->general_description); 
		    } 
		 d->str = &OLC_EXIT(d)->general_description; 
		 d->max_str = MAX_EXIT_DESC; 
		 d->mail_to = 0; 
		 return; 
	      case '3': 
		 OLC_MODE(d) = REDIT_EXIT_KEYWORD; 
		 send_to_char(d->character, "Enter keywords : "); 
		 return; 
	      case '4': 
		 OLC_MODE(d) = REDIT_EXIT_KEY; 
		 send_to_char(d->character, "Enter key number : "); 
		 return; 
	      case '5': 
		 redit_disp_exit_flag_menu(d); 
		 OLC_MODE(d) = REDIT_EXIT_DOORFLAGS; 
		 return; 
	      case '6': 
		/* delete exit */ 
		 if (OLC_EXIT(d)->keyword) 
		    free(OLC_EXIT(d)->keyword); 
		 if (OLC_EXIT(d)->general_description) 
		    free(OLC_EXIT(d)->general_description); 
		 if(OLC_EXIT(d))
		    free(OLC_EXIT(d)); 
		 OLC_EXIT(d) = NULL; 
		 break; 
	      default: 
		 send_to_char(d->character, "Try again : "); 
		 return; 
	     } 
	  break; 
 
       case REDIT_EXIT_NUMBER: 
	  vnumber = (atoi(arg)); 
	  if (vnumber != -1) 
	     { 
	     vnumber = real_room(vnumber); 
	     if (vnumber < 0) 
		{ 
		send_to_char(d->character, "That room does not exist, try again : "); 
		return; 
		} 
	     OLC_EXIT(d)->to_room = vnumber; 
	     } 
	  else
	     OLC_EXIT(d)->to_room = (long)-1; 
	  redit_disp_exit_menu(d); 
	  return; 
 
       case REDIT_EXIT_DESCRIPTION: 
	 /* we should NEVER get here */ 
	  mudlogf(BRF,LVL_BUILDER,TRUE,
		 "SYSERR: Reached REDIT_EXIT_DESC case in parse_redit"); 
	  break; 
 
       case REDIT_EXIT_KEYWORD: 
	  if (OLC_EXIT(d)->keyword) 
	     free(OLC_EXIT(d)->keyword); 
	  if(arg&&*arg)
	     OLC_EXIT(d)->keyword = str_dup(arg); 
	  else
	     OLC_EXIT(d)->keyword=NULL;
	  redit_disp_exit_menu(d); 
	  return; 
 
       case REDIT_EXIT_KEY: 
	  vnumber = atoi(arg); 
	  OLC_EXIT(d)->key = vnumber; 
	  redit_disp_exit_menu(d); 
	  return; 
 
       case REDIT_EXIT_DOORFLAGS: 
	  vnumber = atoi(arg); 
	  if ((vnumber < 0) || (vnumber > 8)) 
	     { 
	     redit_disp_exit_flag_menu(d); 
	     send_to_char(d->character, "That was not a valid choice!\r\n"); 
	     } 
	  else 
	     { 
	    /* doors are a bit idiotic, don't you think? :) */ 
            /* made it a bit less silly - nomikos 10-9-02 */
	     if (vnumber == 0) 
		OLC_EXIT(d)->exit_info &= ~(EX_ISDOOR | EX_PICKPROOF |
					    EX_SECRET | EX_AUTOCLOSE | EX_NOPASS);
	     else if(vnumber==1)
		OLC_EXIT(d)->exit_info |= EX_ISDOOR;
	     else if(vnumber==2)
                {
		OLC_EXIT(d)->exit_info |= EX_ISDOOR; 
                OLC_EXIT(d)->exit_info ^= EX_PICKPROOF;
                }
	     else if(vnumber==3)
                {
                OLC_EXIT(d)->exit_info |= EX_ISDOOR;
		OLC_EXIT(d)->exit_info ^= EX_SECRET;
                }
	     else if(vnumber==4)
		OLC_EXIT(d)->exit_info ^= EX_FLY;
	     else if(vnumber==5)
		OLC_EXIT(d)->exit_info ^= EX_DROP;
	     else if(vnumber==6)
                {
		OLC_EXIT(d)->exit_info |= EX_ISDOOR;
                OLC_EXIT(d)->exit_info ^= EX_AUTOCLOSE;
                }
             else if(vnumber==7)
                OLC_EXIT(d)->exit_info ^= EX_HIDDEN;
	     else if (vnumber == 8) {
	       OLC_EXIT(d)->exit_info ^= EX_NOPASS;
	     }

	    /* jump out to menu */ 
	     redit_disp_exit_menu(d); 
	     } 
	  return; 
 
       case REDIT_EXTRADESC_KEY: 
	  OLC_DESC(d)->keyword = str_dup((arg && *arg) ? arg : "undefined");
	  redit_disp_extradesc_menu(d); 
	  return; 
 
       case REDIT_EXTRADESC_MENU: 
	  vnumber = atoi(arg); 
	  switch (vnumber) 
	     { 
	      case 0: 
	      { 
	     /* if something got left out, delete the extra desc 
		when backing out to menu */ 
	      if (!OLC_DESC(d)->keyword || !OLC_DESC(d)->description)  
		 { 
		 struct extra_descr_data **tmp_desc; 
 
		 if (OLC_DESC(d)->keyword) 
		    free(OLC_DESC(d)->keyword); 
		 if (OLC_DESC(d)->description) 
		    free(OLC_DESC(d)->description); 
 
		/*. Clean up pointers .*/ 
		 for(tmp_desc = &(OLC_ROOM(d)->ex_description); *tmp_desc; 
		     tmp_desc = &((*tmp_desc)->next)) 
		    { 
		    if(*tmp_desc == OLC_DESC(d)) 
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
		 OLC_MODE(d) = REDIT_EXTRADESC_KEY; 
		 send_to_char(d->character, "Enter keywords, separated by spaces : "); 
		 return; 
	      case 2: 
		 OLC_MODE(d) = REDIT_EXTRADESC_DESCRIPTION; 
		 SEND_TO_Q(d,"Enter extra description: (/s saves /h for help)\r\n\r\n"); 
		 d->backstr = NULL; 
		 if (OLC_DESC(d)->description) 
		    { 
		    SEND_TO_Q(d, "%s", OLC_DESC(d)->description); 
		    d->backstr = str_dup(OLC_DESC(d)->description); 
		    } 
		 d->str = &OLC_DESC(d)->description; 
		 d->max_str = MAX_MESSAGE_LENGTH; 
		 d->mail_to = 0; 
		 return; 
 
	      case 3: 
		 if (!OLC_DESC(d)->keyword || !OLC_DESC(d)->description) 
		    { 
		    send_to_char(d->character, "You can't edit the next extra desc without completing this one.\r\n"); 
		    redit_disp_extradesc_menu(d); 
		    } 
		 else 
		    { 
		    struct extra_descr_data *new_extra; 
 
		    if (OLC_DESC(d)->next) 
		       OLC_DESC(d) = OLC_DESC(d)->next; 
		    else 
		       { 
		      /* make new extra, attach at end */ 
		       CREATE(new_extra, struct extra_descr_data, 1); 
		       OLC_DESC(d)->next = new_extra; 
		       OLC_DESC(d) = new_extra; 
		       } 
		    redit_disp_extradesc_menu(d); 
		    } 
		 return; 
	     } 
	  break; 
 
	 /* 
	  * begin add - Bon 07/29/97 
	  */ 
       case REDIT_TELEPORT_MENU: 
	  switch (*arg) 
	     { 
	      case '0': 
		/* 
		 * begin add - Bon 08/07/97 
		 */ 
		 if(OLC_ROOM(d)->tele->targ == 0)
		    { 
		    OLC_ROOM(d)->tele->targ= 0; 
		    OLC_ROOM(d)->tele->time= 0; 
		    OLC_ROOM(d)->tele->bitvector= 0; 
		    OLC_ROOM(d)->tele->obj= -1; 
		    if(OLC_ROOM(d)->tele->to_char)
		       {
		       free(OLC_ROOM(d)->tele->to_char);
		       OLC_ROOM(d)->tele->to_char=NULL;
		       }
		    if(OLC_ROOM(d)->tele->to_source_room)
		       {
		       free(OLC_ROOM(d)->tele->to_source_room);
		       OLC_ROOM(d)->tele->to_source_room=NULL;
		       }
		    if(OLC_ROOM(d)->tele->to_targ_room)
		       {
		       free(OLC_ROOM(d)->tele->to_targ_room);
		       OLC_ROOM(d)->tele->to_targ_room=NULL;
		       }
		    } 
		/* 
		 * end   add - Bon 08/07/97 
		 */ 
		 break; 
	      case '1': 
		 OLC_MODE(d) = REDIT_TELEPORT_NUMBER; 
		 send_to_char(d->character, "Teleport to room number : "); 
		 return; 
	      case '2': 
		 OLC_MODE(d) = REDIT_TELEPORT_DELAY; 
		 send_to_char(d->character, "Number of seconds to delay : "); 
		 return; 
	      case '3': 
		 OLC_MODE(d) = REDIT_TELEPORT_FLAG; 
		 redit_disp_tele_flag_menu(d);
		 return; 
	      case '4': 
		 OLC_MODE(d) = REDIT_TELEPORT_OBJ; 
		 send_to_char(d->character, "Number of trigger object : "); 
		 return; 
	      case '5': 
		 send_to_char(d->character, "Enter room name:-\r\n| "); 
		 OLC_MODE(d) = REDIT_TELEPORT_TO_CHAR; 
		 return; 
	      case '6': 
		 send_to_char(d->character, "Enter room name:-\r\n| "); 
		 OLC_MODE(d) = REDIT_TELEPORT_TO_SOURCE; 
		 return; 
	      case '7': 
		 send_to_char(d->character, "Enter room name:-\r\n| "); 
		 OLC_MODE(d) = REDIT_TELEPORT_TO_TARG; 
		 return; 
	      case '8':
		/* delete teleport */ 
		 OLC_ROOM(d)->tele->targ= 0; 
		 OLC_ROOM(d)->tele->time= 0; 
		 OLC_ROOM(d)->tele->bitvector= 0; 
		 OLC_ROOM(d)->tele->obj= -1; 
		 if(OLC_ROOM(d)->tele->to_char)
		    {
		    free(OLC_ROOM(d)->tele->to_char);
		    OLC_ROOM(d)->tele->to_char=NULL;
		    }
		 if(OLC_ROOM(d)->tele->to_source_room)
		    {
		    free(OLC_ROOM(d)->tele->to_source_room);
		    OLC_ROOM(d)->tele->to_source_room=NULL;
		    }
		 if(OLC_ROOM(d)->tele->to_targ_room)
		    {
		    free(OLC_ROOM(d)->tele->to_targ_room);
		    OLC_ROOM(d)->tele->to_targ_room=NULL;
		    }
		 break; 
	      default: 
		 send_to_char(d->character, "Try again : "); 
		 return; 
	     } 
	  break; 
 
       case REDIT_TELEPORT_NUMBER: 
	  vnumber = (atoi(arg)); 
	  if (vnumber <= 0) 
	     { 
	     OLC_ROOM(d)->tele->time= 0; 
	     OLC_ROOM(d)->tele->bitvector= 0; 
	     OLC_ROOM(d)->tele->obj= -1; 
	     vnumber = 0; 
	     } 
	  OLC_ROOM(d)->tele->targ = vnumber; 
	  redit_disp_teleport_menu(d); 
	  return; 
 
       case REDIT_TELEPORT_DELAY: 
	  vnumber = (atoi(arg)); 
	  if (vnumber <= 0) 
	     { 
	     OLC_ROOM(d)->tele->targ= 0; 
	     OLC_ROOM(d)->tele->bitvector= 0; 
	     OLC_ROOM(d)->tele->obj= -1; 
	     vnumber = 0; 
	     } 
	  OLC_ROOM(d)->tele->time = vnumber; 
	  redit_disp_teleport_menu(d); 
	  return; 
       case REDIT_TELEPORT_OBJ: 
	  vnumber = (atoi(arg)); 
	  OLC_ROOM(d)->tele->obj=vnumber; 
	  redit_disp_teleport_menu(d); 
	  return; 
 
       case REDIT_TELEPORT_FLAG:
	  vnumber = atoi(arg);
	  if((vnumber<0)||(vnumber>NUM_TELEPORT))
	     {
	     send_to_char(d->character, "That's not a valid choice!\r\n"); 
             redit_disp_tele_flag_menu(d); 
             } 
          else if (vnumber == 0) 
	     redit_disp_teleport_menu(d);
          else 
             { 
            /* toggle bits */ 
             TOGGLE_BIT(OLC_ROOM(d)->tele->bitvector, 1 << (vnumber - 1));
             redit_disp_tele_flag_menu(d); 
             } 
          return; 

       case REDIT_TELEPORT_TO_CHAR: 
	  if (OLC_ROOM(d)->tele->to_char) 
	     free(OLC_ROOM(d)->tele->to_char); 
	  if (strlen(arg) > MAX_TELE_STRING) 
	     arg[MAX_TELE_STRING -1] = '\0'; 
	  delete_doubledollar(arg);
	  OLC_ROOM(d)->tele->to_char = strlen(arg)>1 ? str_dup(arg) : NULL;
	  redit_disp_teleport_menu(d); 
	  return; 
	  break; 

       case REDIT_TELEPORT_TO_SOURCE: 
	  if (OLC_ROOM(d)->tele->to_source_room) 
	     free(OLC_ROOM(d)->tele->to_source_room); 
	  if (strlen(arg) > MAX_TELE_STRING) 
	     arg[MAX_TELE_STRING -1] = '\0'; 
	  delete_doubledollar(arg);
	  OLC_ROOM(d)->tele->to_source_room=strlen(arg)>1?str_dup(arg) : NULL;
	  redit_disp_teleport_menu(d); 
	  return; 
	  break; 

       case REDIT_TELEPORT_TO_TARG: 
	  if (OLC_ROOM(d)->tele->to_targ_room) 
	     free(OLC_ROOM(d)->tele->to_targ_room); 
	  if (strlen(arg) > MAX_TELE_STRING) 
	     arg[MAX_TELE_STRING -1] = '\0'; 
	  delete_doubledollar(arg);
	  OLC_ROOM(d)->tele->to_targ_room=strlen(arg)>1 ? str_dup(arg) : NULL;
	  redit_disp_teleport_menu(d); 
	  return; 
	  break; 

	 /* 
	  * end   add - Bon 07/29/97 
	  */ 
 
       default: 
	 /* we should never get here */ 
	  mudlogf(BRF,LVL_BUILDER,TRUE,
		 "SYSERR: Reached default case in parse_redit"); 
	  break; 
      } 
  /*. If we get this far, something has be changed .*/ 
   OLC_VAL(d) = 1; 
   redit_disp_menu(d); 
} 
 
