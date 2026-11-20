/* ************************************************************************ 
*   File: graph.c                                       Part of CircleMUD * 
*  Usage: various graph algorithms                                        * 
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
#include "interpreter.h" 
#include "handler.h" 
#include "db.h" 
#include "spells.h" 
 
ACMD(do_say); 
 
/* Externals */ 
extern struct char_data *character_list; 
extern const char *dirs[]; 
extern struct room_data *world; 
extern struct  zone_data *zone_table;
extern int track_through_doors;

struct bfs_queue_struct 
{ 
   room_rnum room; 
   char dir; 
   struct bfs_queue_struct *next; 
} ; 
 
static struct bfs_queue_struct *queue_head = 0, *queue_tail = 0; 
 
/* Utility macros */ 
#define MARK(room) (SET_BIT(ROOM_FLAGS(room), ROOM_BFS_MARK)) 
#define UNMARK(room) (REMOVE_BIT(ROOM_FLAGS(room), ROOM_BFS_MARK)) 
#define IS_MARKED(room) (ROOM_FLAGGED(room, ROOM_BFS_MARK)) 
#define TOROOM(x, y) (world[(x)].dir_option[(y)]->to_room) 
#define IS_CLOSED(x, y) (EXIT_FLAGGED(world[(x)].dir_option[(y)], EX_CLOSED)) 

int VALID_EDGE(room_rnum x, int y,long flag)
{
  if (world[x].dir_option[y] == NULL || TOROOM(x, y) == NOWHERE)
    return 0;
  if (!IS_SET(flag,IGNORE_THROUGHDOOR) && track_through_doors == FALSE && 
      IS_CLOSED(x, y))
    return 0;
  if (IS_MARKED(TOROOM(x,y)))
     return 0;
  if (!IS_SET(flag,IGNORE_NOTRACK)&&ROOM_FLAGGED(TOROOM(x, y), ROOM_NOTRACK))
    return 0;
  if (!IS_SET(flag,IGNORE_CLAN)&&ROOM_FLAGGED(TOROOM(x,y),ROOM_CLAN))
     return 0;
  if (ROOM_FLAGGED(TOROOM(x,y),ROOM_DEATH))
     return 0;
  if (!IS_SET(flag,IGNORE_ZNOTRACK) && Z_FLAGGED(TOROOM(x,y),Z_NO_TRACK))
     return 0;
  if(IS_SET(flag,MAZE_TEST) && (world[x].zone != world[world[x].dir_option[y]->to_room].zone))
     return 0;
  if (ROOM2_FLAGGED(TOROOM(x,y),ROOM2_NEVERMOB))
     return 0;
  return 1;
}

void bfs_enqueue(room_rnum room, int dir) 
{ 
   struct bfs_queue_struct *curr; 
 
   CREATE(curr, struct bfs_queue_struct, 1); 
   curr->room = room; 
   curr->dir = dir; 
   curr->next = 0; 
 
   if (queue_tail) 
      { 
      queue_tail->next = curr; 
      queue_tail = curr; 
      } 
   else 
      queue_head = queue_tail = curr; 
} 
 
 
void bfs_dequeue(void) 
{ 
   struct bfs_queue_struct *curr; 
 
   curr = queue_head; 
 
   if (!(queue_head = queue_head->next)) 
      queue_tail = 0; 
   free(curr); 
} 
 
 
void bfs_clear_queue(void) 
{ 
   while (queue_head) 
      bfs_dequeue(); 
} 
 
 
/*
 * find_first_step: given a source room and a target room, find the first 
 * step on the shortest path from the source to the target. 
 * 
 * Intended usage: in mobile_activity, give a mob a dir to go if they're 
 * tracking another mob or a PC.  Or, a 'track' skill for PCs. 
 */ 
 
int find_first_step(room_rnum src, room_rnum target, long iFlag) 
{ 
   int curr_dir; 
   room_rnum curr_room; 
 
   if (src < 0 || src > top_of_world || target < 0 || target > top_of_world) 
      { 
      log("SYSERR: Illegal value passed to find_first_step (S:%ld T:%ld)",
	  src,target); 
      return BFS_ERROR; 
      } 
   if (src == target) 
      return BFS_ALREADY_THERE; 
 
  /* clear marks first, some OLC systems will save the mark */ 
   for (curr_room = 0; curr_room <= top_of_world; curr_room++) 
      UNMARK(curr_room); 
 
   MARK(src); 
 
  /* first, enqueue the first steps, saving which direction we're going. */ 
   for (curr_dir = 0; curr_dir < NUM_OF_DIRS; curr_dir++) 
      {
      if (VALID_EDGE(src, curr_dir,iFlag)) 
	 { 
	 MARK(TOROOM(src, curr_dir)); 
	 bfs_enqueue(TOROOM(src, curr_dir), curr_dir); 
	 } 
      }
  /* now, do the classic BFS. */ 
   while (queue_head) 
      { 
      if (queue_head->room == target) 
	 { 
	 curr_dir = queue_head->dir; 
	 bfs_clear_queue(); 
	 return curr_dir; 
	 } 
      else 
	 { 
	 for (curr_dir = 0; curr_dir < NUM_OF_DIRS; curr_dir++) 
	    if (VALID_EDGE(queue_head->room, curr_dir,iFlag)) 
	       { 
	       MARK(TOROOM(queue_head->room, curr_dir)); 
	       bfs_enqueue(TOROOM(queue_head->room, curr_dir),queue_head->dir);
	       } 
	 bfs_dequeue(); 
	 } 
      } 
 
   return BFS_NO_PATH; 
} 
 
 
/************************************************************************ 
*  Functions and Commands which use the above fns          * 
************************************************************************/ 
 
/*
 * bfs_first_steps: the first step from src toward EVERY room it can reach,
 * under the same edges as find_first_step: steps[room] is that direction,
 * BFS_ALREADY_THERE for src, or BFS_NO_PATH. One search serves every
 * candidate do_track weighs.
 */
static void bfs_first_steps(room_rnum src, long iFlag, int *steps)
{
   int curr_dir;
   room_rnum curr_room;

   for (curr_room = 0; curr_room <= top_of_world; curr_room++)
      {
      UNMARK(curr_room);
      steps[curr_room] = BFS_NO_PATH;
      }
   MARK(src);
   steps[src] = BFS_ALREADY_THERE;
   for (curr_dir = 0; curr_dir < NUM_OF_DIRS; curr_dir++)
      if (VALID_EDGE(src, curr_dir, iFlag))
         {
         MARK(TOROOM(src, curr_dir));
         steps[TOROOM(src, curr_dir)] = curr_dir;
         bfs_enqueue(TOROOM(src, curr_dir), curr_dir);
         }
   while (queue_head)
      {
      for (curr_dir = 0; curr_dir < NUM_OF_DIRS; curr_dir++)
         if (VALID_EDGE(queue_head->room, curr_dir, iFlag))
            {
            MARK(TOROOM(queue_head->room, curr_dir));
            steps[TOROOM(queue_head->room, curr_dir)] = queue_head->dir;
            bfs_enqueue(TOROOM(queue_head->room, curr_dir), queue_head->dir);
            }
      bfs_dequeue();
      }
}

/*
 * The character `track <name>` follows. The whole argument is the name and
 * every word must match (isname_all). Matches are weighed in the tracker's
 * room, then the rest of its zone, then the world, each in character_list
 * order, and the first one with a trail wins; one with no trail (no path,
 * or AFF_NOTRACK) is passed over. *first gets the first match seen, so the
 * refusal names it. "N.name" keeps get_char_vis's numbered match.
 */
static struct char_data *track_target(struct char_data *ch, char *name, struct char_data **first)
{
   struct char_data *i, *found = NULL;
   int *steps, pass, zone = world[IN_ROOM(ch)].zone;

   *first = NULL;
   if (isdigit((unsigned char) *name) && strchr(name, '.'))
      {
      *first = get_char_vis(ch, name, FIND_CHAR_WORLD);
      return ((*first && !AFF_FLAGGED(*first, AFF_NOTRACK)) ? *first : NULL);
      }
   CREATE(steps, int, top_of_world + 1);
   bfs_first_steps(IN_ROOM(ch), 0, steps);
   for (pass = 0; pass < 3 && !found; pass++)
      for (i = pass ? character_list : world[IN_ROOM(ch)].people; i && !found;
           i = pass ? i->next : i->next_in_room)
         {
         if (i == ch || IN_ROOM(i) == NOWHERE)
            continue;
         if (pass == 1 && (IN_ROOM(i) == IN_ROOM(ch) || world[IN_ROOM(i)].zone != zone))
            continue;
         if (pass == 2 && (IN_ROOM(i) == IN_ROOM(ch) || world[IN_ROOM(i)].zone == zone))
            continue;
         if (!isname_all(name, i->player.name) || !CAN_SEE(ch, i))
            continue;
         if (!*first)
            *first = i;
         if (!AFF_FLAGGED(i, AFF_NOTRACK) && steps[IN_ROOM(i)] != BFS_NO_PATH)
            found = i;
         }
   free(steps);
   return (found);
}

ACMD(do_track) 
{ 
   struct char_data *vict, *first; 
   int dir,pass=0; 
   char *arg=get_buffer(MAX_INPUT_LENGTH), *name;
 
   if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_TRACK)) 
      { 
      send_to_char(ch,"You have no idea how.\r\n"); 
      release_buffer(arg);
      return; 
      } 
   /* The whole argument, fill words dropped: "track the deep dragon". */
   skip_spaces(&argument);
   *arg = '\0';
   while (*argument)
      {
      char *word = get_buffer(MAX_INPUT_LENGTH);
      argument = any_one_arg(argument, word);
      if (*word && !fill_word(word) && strlen(arg) + strlen(word) + 2 < MAX_INPUT_LENGTH)
         {
         if (*arg)
            strcat(arg, " ");
         strcat(arg, word);
         }
      release_buffer(word);
      }
   if (!*arg) 
      { 
      send_to_char(ch,"Whom are you trying to track?\r\n"); 
      release_buffer(arg);
      return; 
      } 
   name = arg;
   vict = track_target(ch, name, &first);
   if (!first) 
      { 
      send_to_char(ch,"No-one around by that name.\r\n"); 
      release_buffer(arg);
      return; 
      } 
   if (!vict && AFF_FLAGGED(first, AFF_NOTRACK)) 
      { 
      send_to_char(ch,"You sense no trail.\r\n"); 
      release_buffer(arg);
      return; 
      } 
   if (!vict)
      vict = first;
   
   if (GET_SKILL(ch, SKILL_TRACK) < number(0,101)) 
      {
      int tries = 10;
      do 
	 { 
	 dir = number(0, NUM_OF_DIRS-1); 
	 } 
      while (!CAN_GO(ch, dir)&&--tries); 
      if(tries==0)
	 {
	 dir=BFS_NO_PATH;
	 pass=2;
	 }
      else
	 pass=0;
      }
   else
      {
      dir = find_first_step(IN_ROOM(ch), IN_ROOM(vict),0); 
      pass=1;
      }

   switch (dir) 
      { 
       case BFS_ERROR: 
	  send_to_char(ch,"Hmm.. something seems to be wrong.\r\n"); 
	  break; 
       case BFS_ALREADY_THERE: 
	  send_to_char(ch,"You're already in the same room!!\r\n"); 
	  break; 
       case BFS_NO_PATH: 
	  send_to_char(ch, "You can't sense a trail to %s from here.\r\n", 
		  HMHR(vict)); 
	  break; 
       default: 
	  send_to_char(ch, "You sense a trail %s from here!\r\n", dirs[dir]); 
	  if(pass==0)
	     improve_skill(ch,SKILL_TRACK,USE_FAIL);
	  else if(pass==1)
	     improve_skill(ch,SKILL_TRACK,USE_PASS);
	  break; 
      } 
   release_buffer(arg);
} 
 
 
void hunt_victim(struct char_data * ch) 
{ 
   int dir; 
   byte found; 
   struct char_data *tmp; 
 
   if (!ch || !HUNTING(ch)||FIGHTING(ch)) 
      return; 
 
  /* make sure the char still exists */ 
   for (found = FALSE, tmp = character_list; tmp && !found; tmp = tmp->next) 
      if (HUNTING(ch) == tmp) 
	 found = TRUE; 
 
   if (!found) 
      { 
      do_say(ch, "Damn!  My prey is gone!!", 0, 0); 
      HUNTING(ch) = NULL; 
      return; 
      } 
   dir = find_first_step(IN_ROOM(ch), IN_ROOM(HUNTING(ch)),
			 IGNORE_NOTRACK|IGNORE_ZNOTRACK); 
   if (dir < 0) 
      { 
      char *buf=get_buffer(128);
      sprintf(buf, "Damn!  Lost %s!", HMHR(HUNTING(ch))); 
      do_say(ch, buf, 0, 0); 
      HUNTING(ch) = NULL; 
      release_buffer(buf);
      } 
   else 
      { 
      perform_move(ch, dir, 1,0); 
      if (IN_ROOM(ch) == IN_ROOM(HUNTING(ch))) 
	 hit(ch, HUNTING(ch), TYPE_UNDEFINED); 
      } 
} 
