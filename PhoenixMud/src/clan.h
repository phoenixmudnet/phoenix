/***************************************************************************
 *  File:  clan.h                                       Part of PhoenixMUD *
 *  Usage: Clan system defines                                             *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        * 
 *  Written by War                                                         *
 *  Modified by Angus Mezick Feb 98                                        *
 *                                                                         *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               * 
 *  PhoenixMUD is based on CircleMUD, Copyright (C) 1996-98.               *
 ***************************************************************************/

/**********************************
 * Clan list read in from clan.txt
 *********************************/

struct clan_data
{
   int cl_number; /* clan number*/
   short cl_room; /* the clan's room number */
   int cl_donate; /* cdonate room (V3) nomi 6/2/25 */
   long cl_piece; /* vnum of clan member piece */
   char *cl_name; /* name of clan */
   char *cl_desc; /* clan description (V3) nomi 6/2/25 */
   long cl_bank;
   char *cl_players[150]; /*first player is leader*/
   struct clan_data *next;  /*pointer to next*/
};
#ifndef __CLAN_C__
extern struct clan_data *clan_list;
#endif

#define GET_CLAN(ch)       ((ch)->player_specials->saved.clan)
#define GET_CLAN_NAME(ch)  ((ch)->player_specials->saved.cl_name)
#define GET_LEADER(ch)     ((ch)->player_specials->saved.cl_rank)
#define GET_CLAN_ROOM(ch)  ((ch)->player_specials->saved.cl_room)
/* clan donate room - nomi 6/2/2025 */
#define GET_CLAN_DONATE(ch) ((ch)->player_specials->saved.cl_donate)

#define COST_JOIN 35000
#define COST_CREATE 500000
