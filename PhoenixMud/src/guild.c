/* ************************************************************************
*   File: Guild.c                                                         *
*  Usage: GuildMaster's: loading files, assigning spec_procs, and handling*
*                        practicing.                                      *
*                                                                         *
* Based on shop.c.  As such, the CircleMud License applies                *
* Written by Jason Goodwin.   jgoodwin@expert.cc.purdue.edu               *
************************************************************************ */


#include "../localHeader/conf.h"
#include "../localHeader/sysdep.h"
#include <arpa/telnet.h>   /* IAC/SB/SE, for the Char.Skills GMCP frame */

#include "structs.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "buffer.h"
#include "utils.h"
#include "spells.h"
#include "guild.h"
#include "constants.h"

/*extern declerations */
extern struct index_data *mob_index;
extern struct char_data *mob_proto;
extern struct room_data *world;
extern struct time_info_data time_info;
extern int spell_sort_info[MAX_SKILLS+1];
extern int prac_params[3][NUM_CLASSES];
extern int cmd_say, cmd_tell;
extern struct spell_info_type *spells;
extern char *trade_letters[];
extern const int exp_table[];
extern const float class_exp_multipliers[];
extern const float race_exp_multipliers[];
extern struct zone_data *zone_table;
extern int top_of_zone_table;


/* extern function prototypes */
ACMD(do_tell);
ACMD(do_say);
void perform_tell(struct char_data *ch, struct char_data *vict, char *arg);
void str_add_spaces(char *source,int total_length);
int min_level(struct char_data *ch,int spellnum);

#define GM_COST(i,j,k) (int) (GM_CHARGE(i)* min_level(k,j))
#define ZCOM zone_table[zone].cmd[cmd_no]  /*from DB.C*/


/* Local variables */
struct guild_master_data *gm_index;
int top_guild = 0;

const int prac_slots[] =
  {
    0,
    1,
    2,
    3,
    4,
    5,
    6,
    7,
    8,
    9,
    10
  };


/*
 * 4.2: one label per practice, and the bands divide the range exactly.
 *
 * A skill rises 5 points per visit (prac_params "max per prac") and the
 * ceiling is 95, so the climb is 19 windows of 5. There were nine words, and
 * they did not divide that: "just starting" alone covered 1-29 -- six visits
 * printing identical text -- while the top two words held 42% of the total
 * effort between them and changed once each. A player could not tell from the
 * display whether a trip to the guildmaster had done anything.
 *
 * The nine words are unchanged and none are invented: each now has a '+'
 * half. "mastered" fires ONLY at the 95 ceiling (> 94), so the word never
 * leads the cap the way it did opening at 91; superb+ now covers 86-94.
 *
 * Practice lands on 85/90/95 -- superb/superb+/mastered, a distinct label
 * every visit. Only use-improvement (odd increments) reaches the 91-94
 * interior, which reads superb+. The practice list's "Completely Knowledgable"
 * marker is untouched and still fires at exactly 95.
 *
 * NOT a difficulty change. improve_skill's chance curve, the 5-per-visit
 * grant and the 19 visits are all untouched: effort still runs 5 uses for the
 * first window to 155 for the last, the same super-linear shape the XP curve
 * has. Only the wording keeps up with it.
 */
char *how_good(int percent)
  {
  static char buf[256];

  if (percent == 0)
    strcpy(buf, " (not learned)");
  else if (percent <= 5)
    strcpy(buf, " (just starting)");
  else if (percent <= 10)
    strcpy(buf, " (just starting+)");
  else if (percent <= 15)
    strcpy(buf, " (newbie)");
  else if (percent <= 20)
    strcpy(buf, " (newbie+)");
  else if (percent <= 25)
    strcpy(buf, " (bad)");
  else if (percent <= 30)
    strcpy(buf, " (bad+)");
  else if (percent <= 35)
    strcpy(buf, " (poor)");
  else if (percent <= 40)
    strcpy(buf, " (poor+)");
  else if (percent <= 45)
    strcpy(buf, " (average)");
  else if (percent <= 50)
    strcpy(buf, " (average+)");
  else if (percent <= 55)
    strcpy(buf, " (fair)");
  else if (percent <= 60)
    strcpy(buf, " (fair+)");
  else if (percent <= 65)
    strcpy(buf, " (good)");
  else if (percent <= 70)
    strcpy(buf, " (good+)");
  else if (percent <= 75)
    strcpy(buf, " (very good)");
  else if (percent <= 80)
    strcpy(buf, " (very good+)");
  else if (percent <= 85)
    strcpy(buf, " (superb)");
  else if (percent <= 94)
    strcpy(buf, " (superb+)");
  else
    strcpy(buf, " (mastered)");

  return (buf);
  }

char *prac_types[] = {
                       "spell",
                       "skill"
                     };

/* This function helps GMs inform players where they need to go - Nomikos 11/16/2025 */
int gm_refers(struct char_data *keeper, int guild_nr, struct char_data *ch)
  {
  zone_rnum zone;
  int ii, cmd_no, count = 0;
  int lvl = GET_LEVEL(ch);

  /* This is to match up decimal classes with bitwise trainers (they are out of order) */
  int gm_match[] = {1<<6, 1<<4, 1<<5, 1<<3, 1<<7, 1<<8, 1<<9, 1<<10, 1<<11, 1<<12,
                    1<<13, 1<<14, 1<<15, 1<<16, 1<<17, 1<<18, 1<<19, 1<<20, 1<<21};

  /* This is the right GM, no referral needed */
  if ((lvl >= GM_MINLVL(guild_nr)) && (lvl <= GM_MAXLVL(guild_nr)))
     return 0;

  /* Let them know why they need to go elsewhere */
  if (lvl < GM_MINLVL(guild_nr))
    send_to_char(ch, "Your guildmaster tells you 'I do not deal with people so inexperienced in the ways of the world!!'\r\n");
  else
    send_to_char(ch, "Your guildmaster tells you 'But I have already taught you all I know!'\r\n");

  for (ii = 0; ii < top_guild; ii++)
    {
    /* This means they don't like that class, so go to next GM */
    if (gm_match[(int)GET_CLASS(ch)] & GM_WITH_WHO(ii))
      continue;
    /* They are over the GM's maximum level */
    if (lvl > GM_MAXLVL(ii))
      continue;
    /* They are lower than the GM's minimum level */
    if (lvl < GM_MINLVL(ii))
      continue;
    /* Check where they load (mimics vload code) */
    for (zone = 0; zone <= top_of_zone_table; zone++)
      for (cmd_no = 0; ZCOM.command != 'S'; cmd_no++)
        if (ZCOM.command == 'M')
          if (ZCOM.arg1 == GM_TRAINER(ii))
            /* These are the GMs they should seek */
            send_to_char(ch, "You %s seek %s near %s.\r\n", count++?"could also":"should", 
               mob_proto[GM_TRAINER(ii)].player.short_descr, zone_table[zone].name);
    }
  /* The player has been referred to at least one other GM (hopefully) */
  return 1;
  }


int is_guild_open(struct char_data *keeper, int guild_nr, int msg)
  {
  char buf[200];

  *buf = 0;
  if (GM_OPEN(guild_nr) > time_info.hours && GM_CLOSE(guild_nr) < time_info.hours)
    strcpy(buf, MSG_TRAINER_NOT_OPEN);

  if (!(*buf))
    return (TRUE);
  if (msg)
    do_say(keeper, buf, cmd_tell, 0);
  return (FALSE);
  }

int is_gm_ok_char(struct char_data * keeper, struct char_data * ch, int guild_nr)
  {
    /*if (!(CAN_SEE(keeper, ch)))
    {
    do_say(keeper, MSG_TRAINER_NO_SEE_CH, cmd_say, 0);
    return (FALSE);
    }*/

  if ((IS_GOOD(ch) && NOTRAIN_GOOD(guild_nr)) ||
      (IS_EVIL(ch) && NOTRAIN_EVIL(guild_nr)) ||
      (IS_NEUTRAL(ch) && NOTRAIN_NEUTRAL(guild_nr)))
    {
      if (!IS_SCR(ch))
      {
        perform_tell(keeper,ch, MSG_TRAINER_DISLIKE_ALIGN);
        return (FALSE);
      }
    }

  if (IS_NPC(ch))
    {
    perform_tell(keeper,ch, "Why would a mob need to practice?");
    return (FALSE);
    }

  if ((IS_MAGIC_USER(ch) && NOTRAIN_MAGE(guild_nr)) ||
      (IS_CLERIC(ch) && NOTRAIN_CLERIC(guild_nr)) ||
      (IS_THIEF(ch) && NOTRAIN_THIEF(guild_nr)) ||
      (IS_RANGER(ch) && NOTRAIN_RANGER(guild_nr)) ||
      (IS_BARD(ch) && NOTRAIN_BARD(guild_nr)) ||
      (IS_MONK(ch) && NOTRAIN_MONK(guild_nr)) ||
      (IS_BARBARIAN(ch) && NOTRAIN_BARBARIAN(guild_nr)) ||
      (IS_PALADIN(ch) && NOTRAIN_PALADIN(guild_nr)) ||
      (IS_ANTI_PALADIN(ch) && NOTRAIN_APALADIN(guild_nr)) ||
      (IS_DRUID(ch) && NOTRAIN_DRUID(guild_nr)) ||
      (IS_KENSAI(ch) && NOTRAIN_KENSAI(guild_nr)) ||
      (IS_ASSASSIN(ch) && NOTRAIN_ASSASSIN(guild_nr)) ||
      (IS_NECROMANCER(ch) && NOTRAIN_NECRO(guild_nr)) ||
      (IS_DEVA(ch) && NOTRAIN_DEVA(guild_nr)) ||
      (IS_WARRIOR(ch) && NOTRAIN_WARRIOR(guild_nr)))
    {
    perform_tell(keeper,ch, MSG_TRAINER_DISLIKE_CLASS);
    return (FALSE);
    }
  return (TRUE);
  }

int is_gm_ok(struct char_data * keeper, struct char_data * ch, int guild_nr)
  {
  if (is_guild_open(keeper, guild_nr, TRUE))
    return (is_gm_ok_char(keeper, ch, guild_nr));
  else
    return (FALSE);
  }

int does_gm_know(int guild_nr, int i)
  {
  /*    return ((int)(gm_index[guild_nr].skills_and_spells[i])); */
  return 1;
  }

/* this and list skills should probally be combined.
   perhaps in the next release?  */

void what_does_gm_know(int guild_nr, struct char_data * ch,int learn)
  {
  int i, sortpos;
  char *buf=get_buffer(126);
  char *buf2=get_buffer(32750);


  sprintf(buf2,"I can teach you the following:\r\n");

  for (sortpos = 1; sortpos < MAX_SPELLS; sortpos++)
    {
    i = spell_sort_info[sortpos];
    if (strlen(buf2) >= 32750 - 32)
      {
      strcat(buf2, "**OVERFLOW**\r\n");
      break;
      }
    if(learn==1)
      {
      if (GET_LEVEL(ch)>=min_level(ch,i)&&
          does_gm_know(guild_nr, i)&&(GET_SKILL(ch,i)==0))
        {
        sprintf(buf2+strlen(buf2), "%-20s %d gold\r\n",
                spells[i].spell_name,
                GM_COST(guild_nr,i,ch)*3);
        }
      }
    else
      {
      if (GET_LEVEL(ch)>=min_level(ch,i) &&
          does_gm_know(guild_nr, i)&&(GET_SKILL(ch,i)>0))
        {
        if(spells[i].is_spell==IS_SPELL)
          sprintf(buf,"Spell Level: %d",GET_SKILL(ch,i));
        else
          strcpy(buf, how_good(GET_SKILL(ch, i)));

        sprintf(buf2+strlen(buf2), "%-20s %-20.20s %5d gold ",
                spells[i].spell_name,buf,
                GM_COST(guild_nr,i,ch));
        if((spells[i].is_spell==IS_SPELL)&&(GET_SKILL(ch,i)>=10))
          strcat(buf2,"Completely Knowledgable\r\n");
        else if((spells[i].is_spell==IS_SKILL)&&(GET_SKILL(ch,i)>=95))
          strcat(buf2,"Completely Knowledgable\r\n");
        else if(GET_SKILL_LEARN(ch,i)==100)
          strcat(buf2,"&YReady To Practice&n\r\n");
        else
          strcat(buf2,"\r\n");
        }
      }
    }

  release_buffer(buf);
  if(ch->desc)
    page_string(ch->desc, buf2, TRUE,"");
  release_buffer(buf2);
  }

/* List current skills, spells, or both (default). 
   Added ability to see skills or spells -Nomikos 9-17-2025 */
/* Char.Skills -- what `spells` and `skills` print, as data.
 *
 * Shape is { "skills": [...], "spells": [...] }, each row { name, prof,
 * howGood }. Two lists, not one: `prof` is 0-100 for a skill and the 0-10
 * Spell Level `practice` prints for a spell, so they cannot share a banding.
 *
 * spell_sort_info order, list_skills' level gate, proficiency reported at zero
 * as the screen does. */
void gmcp_char_skills(struct char_data* ch)
  {
  int i, sortpos, t;
  bool first;
  char* esc;

  if (ch == NULL || IS_NPC(ch) || ch->desc == NULL) return;
  if (!IS_SET(ch->desc->oob_protocol, OOB_GMCP)) return;
  if (!IS_SET(ch->desc->oob_protocol, OOB_REPORT_SKILLS)) return;

  esc = get_buffer(MAX_STRING_LENGTH);
  send_to_char(ch, "%c%c%cChar.Skills {", IAC, SB, GMCP);

  for (t = IS_SPELL; t <= IS_SKILL; t++)
    {
    send_to_char(ch, "%s\"%s\":[", t == IS_SPELL ? "" : ",",
                 t == IS_SPELL ? "spells" : "skills");
    first = TRUE;
    for (sortpos = 1; sortpos < MAX_SPELLS; sortpos++)
      {
      i = spell_sort_info[sortpos];
      if (spells[i].spell_name == NULL) continue;
      if (spells[i].is_spell != t) continue;
      if (GET_LEVEL(ch) < min_level(ch, i)) continue;

      json_escape_string(spells[i].spell_name, esc, MAX_STRING_LENGTH);
      send_to_char(ch, "%s{\"name\":\"%s\",\"prof\":%d",
                   first ? "" : ",", esc, GET_SKILL(ch, i));
      if (t == IS_SKILL)
        {
        json_escape_string(how_good(GET_SKILL(ch, i)), esc, MAX_STRING_LENGTH);
        send_to_char(ch, ",\"howGood\":\"%s\"", esc);
        }
      send_to_char(ch, "}");
      first = FALSE;
      }
    send_to_char(ch, "]");
    }

  send_to_char(ch, "}%c%c", IAC, SE);
  release_buffer(esc);
  }

void list_skills(struct char_data * ch, int type)
  {
  int i, t, sortpos, lvl, type_skill = 0, type_spell = 0;
  char *buf = get_buffer(MAX_STRING_LENGTH);
  char *buf2 = get_buffer(32750);

  if (type == IS_SPELL)
    type_spell = 1;
  else if (type == IS_SKILL)
    type_skill = 1;
  else
    type_spell = type_skill = 1;

  strcpy(buf2, "");
  for (t = IS_SPELL; t <= IS_SKILL; t++)
    {
    if (type_spell && (t == IS_SPELL))
      sprintf(buf2+strlen(buf2), "You know of the following spells:\r\n");
    else if (type_skill && (t == IS_SKILL))
      sprintf(buf2+strlen(buf2), "You know of the following skills:\r\n");
	  else
	    continue;

    for (sortpos = 1; sortpos < MAX_SPELLS; sortpos++)
      {
      i = spell_sort_info[sortpos];
      if (spells[i].is_spell == t)
        {
        if (GET_LEVEL(ch) >= (lvl=min_level(ch,i)))
          {
          if (t == IS_SPELL)
            sprintf(buf, "%-30s Spell Level: %d", spells[i].spell_name,
                    GET_SKILL(ch, i));
          else
            sprintf(buf, "%-30s %s", spells[i].spell_name,
                    how_good(GET_SKILL(ch, i)));

          str_add_spaces(buf,50);
          sprintf(buf+strlen(buf), " Level: %3d ", lvl);

          strcat(buf2, buf);
          if (((spells[i].is_spell==IS_SPELL)&&(GET_SKILL(ch,i)>=10)) ||
              ((spells[i].is_spell==IS_SKILL)&&(GET_SKILL(ch,i)>=95)))
            strcat(buf2,"Completely Knowledgable\r\n");
          else if (GET_SKILL_LEARN(ch,i)==100)
            strcat(buf2,"&YReady To Practice&n\r\n");
		  /* Added skill progross - Nomikos 6-28-25 */
          else if (GET_SKILL_LEARN(ch,i) > 0) 
	          {
	          strcat(buf2,"[");
	          for (int ii = 20; ii <= 100; ii += 20) 
	            {
	            if (ii <= GET_SKILL_LEARN(ch,i))
	              strcat(buf2,"*");
	            else
	              strcat(buf2,"-");
	            }
	          strcat(buf2,"]\r\n");
	          }
          else
            strcat(buf2,"\r\n");
          }
        }
      }
    if (type_spell && type_skill)
      strcat(buf2,"\r\n");
	}

  release_buffer(buf);
  if(ch->desc)
    page_string(ch->desc, buf2, TRUE,"");
  release_buffer(buf2);
  }

SPECIAL(guild)
  {
  int skill_num,gain_level;
  char *buf, *skill_word, *buf2;

  /***  Let's see which guildmaster we're dealing with ***/

  struct char_data *keeper = (struct char_data *) me;
  int guild_nr;

  if (IS_NPC(ch)||(!CMD_IS("practice") &&!CMD_IS("gain")&&!CMD_IS("learn"))||
      (cmd<1) )
    return 0;
  for (guild_nr = 0; guild_nr < top_guild; guild_nr++)
    if (GM_TRAINER(guild_nr) == keeper->nr)
      break;

  if (guild_nr >= top_guild)
    return (FALSE);

  if (GM_FUNC(guild_nr))
    if ((GM_FUNC(guild_nr)) (ch, me, cmd, argument))
      return(TRUE);

  skip_spaces(&argument);

  /*** Is the GM able to train?    ****/

  if (!AWAKE(keeper))
    return (FALSE);

  if (!(is_gm_ok(keeper, ch, guild_nr))&&!PRF_FLAGGED(ch,PRF_NOHASSLE))
    return 1;

  if(CMD_IS("learn"))
    {
    if(!*argument)
      {
      what_does_gm_know(guild_nr, ch,1);
      return 1;
      }
    else if (gm_refers(keeper, guild_nr, ch))
      {
      /* Added this to allow players to learn for that last level, and ignore imms */
      if ((GET_LEVEL(ch) > (1+GM_MAXLVL(guild_nr))) && (GET_LEVEL(ch) < LVL_IMMORT))
        return 1;
      }

    skill_num = find_skill_num(argument);

    if(spells[skill_num].is_spell==IS_SPELL)
      skill_word="spell";
    else if(spells[skill_num].is_spell==IS_SKILL)
      skill_word="skill";
    else
      skill_word="typo that you just made";

    /****  Does the GM know the skill the player wants to learn?  ****/

    if (!(does_gm_know(guild_nr, skill_num)))
      {
      buf2=get_buffer(256);
      sprintf(buf2, gm_index[guild_nr].no_such_skill, GET_NAME(ch));
      do_tell(keeper, buf2, cmd_tell, 0);
      release_buffer(buf2);
      return 1;
      }

    /**** Can the player learn the skill if the GM knows it?  ****/

    if (skill_num < 1 ||
        GET_LEVEL(ch) < min_level(ch,skill_num))
      {
      send_to_char(ch, "You do not know of that %s.\r\n", skill_word);
      return 1;
      }

    /* Has the player already learnt this skill? */
    if(GET_SKILL(ch,skill_num)>0)
      {
      send_to_char(ch, "But you already know this %s!\r\n", skill_word);
      return TRUE;
      }

    if(GET_GOLD(ch)<(GM_COST(guild_nr,skill_num,ch)*3))
      {
      buf=get_buffer(256);
      sprintf(buf, gm_index[guild_nr].not_enough_gold, GET_NAME(ch));
      do_tell(keeper, buf, cmd_tell, 0);
      release_buffer(buf);
      return TRUE;
      }

    send_to_char(ch, "You learn a new %s from the master.\r\n",skill_word);

    GET_GOLD(ch) -= (GM_COST(guild_nr, skill_num,ch)*3);

    if(spells[skill_num].is_spell==IS_SPELL)
      {
      GET_SKILL(ch,skill_num)=1;
      GET_SKILL_LEARN(ch,skill_num)=0;
      }
    else if(spells[skill_num].is_spell==IS_SKILL)
      {
      GET_SKILL(ch,skill_num)=25;
      GET_SKILL_LEARN(ch,skill_num)=0;
      }
    else
      {
      log("SYSERR: Tried to practice IS_UNUSED #%d",skill_num);
      }
    gmcp_char_skills(ch);
    return 1;
    }
  else if(CMD_IS("practice"))
    {
    if(!*argument)
      {
      what_does_gm_know(guild_nr, ch,0);
      return 1;
      }
    else if (gm_refers(keeper, guild_nr, ch))
      {
      /* Added this to allow players to practice for that last level, and ignore imms */
      if ((GET_LEVEL(ch) > (1+GM_MAXLVL(guild_nr))) && (GET_LEVEL(ch) < LVL_IMMORT))
        return 1;
      }

    skill_num = find_skill_num(argument);

    if(spells[skill_num].is_spell==IS_SPELL)
      skill_word="spell";
    else if(spells[skill_num].is_spell==IS_SKILL)
      skill_word="skill";
    else
      skill_word="typo that you just made";

    if((skill_num>=0) &&
        (GET_SKILL(ch,skill_num)>0) &&
        (GET_LEVEL(ch)<min_level(ch,skill_num)))
      {
      send_to_char(ch,"This %s is no longer knowable by your class or race "
                   "at this level!\r\n", skill_word);
      SET_SKILL(ch, skill_num, 0);
      return 1;
      }




    /****  Does the GM know the skill the player wants to learn?  ****/

    if (!(does_gm_know(guild_nr, skill_num)))
      {
      buf2=get_buffer(256);
      sprintf(buf2, gm_index[guild_nr].no_such_skill, GET_NAME(ch));
      do_tell(keeper, buf2, cmd_tell, 0);
      release_buffer(buf2);
      return 1;
      }

    /**** Can the player learn the skill if the GM knows it?  ****/

    if (skill_num < 1 ||
        GET_LEVEL(ch) < min_level(ch,skill_num))
      {
      send_to_char(ch, "You do not know of that %s.\r\n", skill_word);
      return 1;
      }

    if (GET_SKILL_LEARN(ch,skill_num) < 100)
      {
      send_to_char(ch,"You need more practical knowledge before I will teach you that.\r\n");
      return 1;
      }
    /****  Is the player maxxed out with the skill?  ****/

    if (((spells[skill_num].is_spell==IS_SPELL)&&
         (GET_SKILL(ch,skill_num)>=10))||
        (GET_SKILL(ch, skill_num) >= LEARNED(ch)))
      {
      send_to_char(ch,"You are already learned in that area.\r\n");
      return 1;
      }

    /****  Does the Player have enough gold to train?  ****/

    if (GET_GOLD(ch) < (GM_COST(guild_nr,skill_num,ch)))
      {
      buf=get_buffer(256);
      sprintf(buf, gm_index[guild_nr].not_enough_gold, GET_NAME(ch));
      do_tell(keeper, buf, cmd_tell, 0);
      release_buffer(buf);
      return 1;
      }

    if(spells[skill_num].is_spell==IS_SPELL)
      {
      gain_level=GET_LEVEL(ch)-min_level(ch,skill_num)+1;
      }
    else
      {
      gain_level=GET_LEVEL(ch)-min_level(ch,skill_num);
      gain_level*=5;
      gain_level+=31;
      }
    if(gain_level<GET_SKILL(ch,skill_num))
      {
      send_to_char(ch,"You need to experience more of the world before I will teach you that.\r\n");
      return 1;
      }
    /****  If we've made it this far, then its time to practice  ****/

    send_to_char(ch,"You practice for a while...\r\n");

    GET_GOLD(ch) -= GM_COST(guild_nr,skill_num,ch);

    if(spells[skill_num].is_spell==IS_SPELL)
      {
      GET_SKILL(ch,skill_num)= MIN(GET_SKILL(ch,skill_num)+1,10);
      GET_SKILL_LEARN(ch,skill_num)=0;
      if(GET_SKILL(ch,skill_num)>=10)
        send_to_char(ch,"You have learnt all you can about this spell.\r\n");
      }
    else
      {
      GET_SKILL_LEARN(ch,skill_num)=0;
      if (GET_SKILL(ch, skill_num) >= LEARNED(ch))
        send_to_char(ch,"You are now learned in that area.\r\n");
      }
    gmcp_char_skills(ch);
    return 1;
    }
  else if (CMD_IS("gain"))
    {
    if (gm_refers(keeper, guild_nr, ch))
      return 1;
    else if(GET_LEVEL(ch) == (LVL_HERO-1))
      {
      send_to_char(ch,"Your guildmaster tells you 'You are of too high a level for me to train.'\r\n");
      send_to_char(ch,"Your guildmaster tells you 'Now is the time to begin to prepare for the\r\n");
      send_to_char(ch,"Your guildmaster tells you '%s quest. Please see a god for details.'\r\n",
                   is_remort_level(ch, TRIPLE_REMORT)?"Demi-God":
                   is_remort_level(ch, DOUBLE_REMORT)?"Avatar":
                   is_remort_level(ch, SINGLE_REMORT)?"Angel":"Hero");
      }
    else if(GET_LEVEL(ch)>=LVL_HERO)
      send_to_char(ch,"Your guildmaster tells you 'I cannot train you.'\r\n");
    else if((GET_EXP(ch)>=GET_EXP_FOR_CH(ch))&&GET_LEVEL(ch)<(LVL_HERO-1))
      {
      send_to_char(ch,"You raise a level! \r\n");
      GET_EXP(ch) = GET_EXP(ch)-GET_EXP_FOR_CH(ch);
      GET_LEVEL(ch)++;
      send_info("[ INFO ] %s has LEVELED!!!!!!\r\n", GET_NAME(ch));
      if (GET_LEVEL(ch) <= 20 && REMORT_LEVEL(ch) == NON_REMORT)
        {
	     /* For players under level 20, force the guildmaster to "noobgain" which can fire
	      * a command trigger.  The %arg% variable is the person who gained. */
	     char *buf3 = get_buffer(MAX_INPUT_LENGTH);
	     sprintf(buf3, "noobgain %s", GET_NAME(ch));
	     command_interpreter(keeper, buf3);
	     release_buffer(buf3);
      }
      advance_level(ch,1);
      }
    else
      {
      send_to_char(ch,"You need to learn more about the world before I will train you again.\r\n");
      }
    return 1;
    }
  return 0;
  }



/**** This function is here just because I'm extremely paranoid.  Take it
      out if you aren't ;)  ****/
void clear_skills(int gindex)
  {
  int i;

  for  (i = 0; i < MAX_SKILLS + 2; i++)
    gm_index[gindex].skills_and_spells[i] = 0;
  }

/****  This is ripped off of read_line from shop.c.  They could be combined. But why? ****/

void read_gm_line(FILE * gm_f, char *string, void *data)
  {
  char *buf=get_buffer(256);
  if (!get_line(gm_f, buf) || !sscanf(buf, string, data))
    {
    fprintf(stderr, "Error in guild #%d\n", GM_NUM(top_guild));
    exit(1);
    }
  release_buffer(buf);
  }

void boot_the_guilds(FILE * gm_f, char *filename, int rec_count) {
  char *buf2 = get_buffer(256);
  int linenum;
  int version;

  version = 1;

  sprintf(buf2, "beginning of GM file %s", filename);

  for (;;) {
    char *buf = fread_string(gm_f, buf2);

    if (*buf == '@') {
      if (sscanf(buf, "@Version: %d", &version) != 1) {
        log("SYSERR: Format error in %s #%d", filename, rec_count);
        log("SYSERR: ...Line: %s\n", buf);
        exit(1);
      }
      free(buf);
    } else if (*buf == '#') { /* New Trainer */

      int temp;
      sscanf(buf, "#%d\n", &temp);
      sprintf(buf2, "GM #%d in GM file %s", temp, filename);
      if (!top_guild)
        CREATE(gm_index, struct guild_master_data, rec_count);

      GM_NUM(top_guild) = temp;
      clear_skills(top_guild);

      char line[256];
      if (!get_line(gm_f, line) || (sscanf(line, "%d", &temp) != 1)) {
        log("SYSERR: Format error in 1st skill of guild #%d (%s)",
            GM_NUM(top_guild), line);
        exit(1);
      }

      linenum = 1;
      while (temp > -1)
      {
        gm_index[top_guild].skills_and_spells[(int)temp] = 1;
        linenum++;
        if (!get_line(gm_f, line) || (sscanf(line, "%d", &temp) != 1))
        {
          log("SYSERR: Format error in skill #%d of guild #%d (%s)", linenum, GM_NUM(top_guild), line);
          exit(1);
        }
      }

      if (!get_line(gm_f, line) || (sscanf(line, "%f", &GM_CHARGE(top_guild)) != 1))
      {
        log("SYSERR: Format error in cost line of guild #%d (%s)", GM_NUM(top_guild), line);
        exit(1);
      }

      gm_index[top_guild].no_such_skill = fread_string(gm_f, buf2);
      gm_index[top_guild].not_enough_gold = fread_string(gm_f, buf2);

      if (!get_line(gm_f, line) || (sscanf(line, "%d %d %d", &GM_TYPE(top_guild), &GM_MINLVL(top_guild), &GM_MAXLVL(top_guild)) != 3)) {
        log("SYSERR: Format error in type line of guild #%d (%s)",
            GM_NUM(top_guild), line);
        exit(1);
      }

      if (!get_line(gm_f, line) || (sscanf(line, "%d", &GM_TRAINER(top_guild)) != 1)) {
        log("SYSERR: Format error in trainer line of guild #%d (%s)",
            GM_NUM(top_guild), line);
        exit(1);
      }

      GM_TRAINER(top_guild) = real_mobile(GM_TRAINER(top_guild));

      if (!get_line(gm_f, line) || (sscanf(line, "%d", &GM_WITH_WHO(top_guild)) != 1)) {
        log("SYSERR: Format error in with_who line of guild #%d (%s)", GM_NUM(top_guild), line);
        exit(1);
      }

      if (!get_line(gm_f, line) || (sscanf(line, "%d", &GM_OPEN(top_guild)) != 1)) {
        log("SYSERR: Format error in open line of guild #%d (%s)", GM_NUM(top_guild), line);
        exit(1);
      }

      if (!get_line(gm_f, line) || (sscanf(line, "%d", &GM_CLOSE(top_guild)) != 1)) {
        log("SYSERR: Format error in close line of guild #%d (%s)", GM_NUM(top_guild), line);
        exit(1);
      }

      GM_FUNC(top_guild) = 0;
      top_guild++;
      free(buf);
    } else if (*buf == '$') { /* EOF */
      free(buf);
      break;
    }
  }
  release_buffer(buf2);
}

void assign_the_gms(void)
  {
  int i;

  for (i = 0; i < top_guild; i++)
    {
      /*log("i=%d, top_guild=%d GM_TRAINER(i)=%d, mob_index[GM_TRAINER(i)] = %p", 
	i, top_guild, GM_TRAINER(i), &mob_index[GM_TRAINER(i)]);*/
      if (GM_TRAINER(i) == -1) {
	continue;
      }
    if (mob_index[GM_TRAINER(i)].func)
      GM_FUNC(i) = mob_index[GM_TRAINER(i)].func;
    mob_index[GM_TRAINER(i)].func = guild;
    }
  }

/* yet another function that was already written in shop.c that
   didn't need to be re-invented ;)  */

char *customer_string2(int gm_nr, int detailed)
  {

  int i, cnt = 1;
  static char buf[256];

  *buf = 0;
  for (i = 0; *trade_letters[i] != '\n'; i++, cnt *= 2)
    if (!(GM_WITH_WHO(gm_nr) & cnt))
      if (detailed)
        {
        if (*buf)
          strcat(buf, ", ");
        strcat(buf, trade_letters[i]);
        }
      else
        sprintf(buf+strlen(buf), "%c", *trade_letters[i]);
    else if (!detailed)
      strcat(buf, "_");

  return (buf);
  }


void list_all_gms(struct char_data *ch)
  {
  int gm_nr;
  char *buf = get_buffer(32750);
  char *buf1 = get_buffer(MAX_STRING_LENGTH);
  char *buf2 = get_buffer(MAX_STRING_LENGTH);

  strcpy(buf, "\r\n");
  for (gm_nr = 0; gm_nr < top_guild; gm_nr++)
    {
    if (!(gm_nr % 20))
      {
      strcat(buf, "Virtual G.Master Charge Min Max Members\r\n");
      strcat(buf, "------------------------------------------------------------------\r\n");
      }
    sprintf(buf2, " %6d  ", GM_NUM(gm_nr));
    if (GM_TRAINER(gm_nr) < 0)
      strcpy(buf1, "<NONE>");
    else
      sprintf(buf1, "%6ld ", mob_index[GM_TRAINER(gm_nr)].vnum); /*WARNING ANGUS! this is the vnum or rnum*/
    sprintf(buf2+strlen(buf2), "%s %5.2f %3d %3d ", buf1,
            GM_CHARGE(gm_nr),GM_MINLVL(gm_nr),GM_MAXLVL(gm_nr));
    strcat(buf2, customer_string2(gm_nr, FALSE));
    sprintf(buf+strlen(buf), "%s\r\n", buf2);
    }

  release_buffer(buf2);
  release_buffer(buf1);
  if(ch->desc)
    page_string(ch->desc, buf, TRUE,"");
  release_buffer(buf);
  }

void list_detailed_gm(struct char_data * ch, int gm_nr)
  {
  int i;
  char *buf = get_buffer(MAX_STRING_LENGTH);
  char *buf1 = get_buffer(MAX_STRING_LENGTH);
  char *buf2 = get_buffer(MAX_STRING_LENGTH);

  if (GM_TRAINER(gm_nr) < 0)
    strcpy(buf1, "<NONE>");
  else
    sprintf(buf1, "%6ld   ", mob_index[GM_TRAINER(gm_nr)].vnum);

  sprintf(buf, " Guild Master: %s\r\n", buf1);
  sprintf(buf+strlen(buf), " Hours: %4d to %4d,  Surcharge: %5.2f\r\n",
          GM_OPEN(gm_nr), GM_CLOSE(gm_nr), GM_CHARGE(gm_nr));
  sprintf(buf+strlen(buf)," Min Level: %d  Max Level: %d\r\n",
          GM_MINLVL(gm_nr),GM_MAXLVL(gm_nr));
  sprintf(buf+strlen(buf), " Whom will train: %s\r\n",
          customer_string2(gm_nr, TRUE));
  /* now for the REAL reason why someone would want to see a GM :) */

  strcat(buf, " The GM can teach the following:\r\n");

  *buf2 = '\0';
  for (i = 0; i <= MAX_SPELLS; i++)
    {
    if (does_gm_know(gm_nr, i))
      sprintf(buf2+strlen(buf2), " %s \r\n",  spells[i].spell_name);
    }

  strcat(buf, buf2);
  release_buffer(buf2);
  release_buffer(buf1);

  if(ch->desc)
    page_string(ch->desc, buf, TRUE,"");
  release_buffer(buf);
  }



void show_gm(struct char_data * ch, char *arg)
  {
  int gm_nr, gm_num;

  if (!*arg)
    list_all_gms(ch);
  else
    {
    if (is_number(arg))
      gm_num = atoi(arg);
    else
      gm_num = -1;

    if (!(gm_num < 0))
      {
      for (gm_nr = 0; gm_nr < top_guild; gm_nr++)
        {
        if (gm_num == GM_NUM(gm_nr))
          break;
        }

      if ((gm_num < 0) || (gm_nr >= top_guild))
        {
        send_to_char(ch,"Illegal guild master number.\r\n");
        return;
        }
      list_detailed_gm(ch, gm_nr);
      }
    }
  }

int improve_skill(struct char_data *ch, int skill, int passcheck)
  {
  int num;
  int chance;
  int is_spell;
  int gain_level;
  int percent;
  if(!ch)
    {
    mudlogf(CMP,LVL_IMMORT,TRUE,
            "SYSERR: improve_skill() received bad ch, skill:%d, pass:%d",
            skill,passcheck);
    return 0;
    }

  /* No Mobs */
  if(IS_NPC(ch))
    return 1;
  if(ROOM_FLAGGED(IN_ROOM(ch),ROOM_PKILL)
      /*||Z_FLAGGED(IN_ROOM(ch),Z_PKILL)*/)
    return 0;

  if (FIGHTING(ch) != NULL) {
     if (PLR_FLAGGED(ch, PLR_PK) && PLR_FLAGGED(FIGHTING(ch), PLR_PK) && abs(GET_LEVEL(ch)-GET_LEVEL(FIGHTING(ch)))<=10)
        return 0;
  }

  if((skill<=0)||(skill>=MAX_SPELLS))
    {
    mudlogf(CMP,LVL_IMMORT,TRUE,
            "SYSERR: improve_skill() received bad skill %d, pass: %d",
            skill,passcheck);
    return 0;
    }

  /* can gain checks */
  /* if(!have skill) */
  if(GET_SKILL(ch,skill)<=0)
    return 0;

  /* if(already gained this tic) */
  if(GET_LEARN_TIC(ch)>0)
    return 0;

  /* if(last gained in this skill) */
  if(GET_LAST_LEARN(ch)==skill)
    return 0;

  is_spell=spells[skill].is_spell;

  /* if(this skill maxed) */
  if (((is_spell==IS_SPELL)&&(GET_SKILL(ch,skill)>=10))||
      (GET_SKILL(ch, skill) >= 95))
    return 0;

  /* if(need to prac this skill) */
  if(GET_SKILL_LEARN(ch,skill)==100)
    return 0;

  num=GET_SKILL(ch,skill);

  if(is_spell==IS_SPELL)
    {
    num*=10;
    gain_level=GET_LEVEL(ch)-min_level(ch,skill)+1;
    }
  else
    {
    gain_level=GET_LEVEL(ch)-min_level(ch,skill);
    gain_level*=5;
    gain_level+=31;
    }
  if(gain_level<GET_SKILL(ch,skill))
    return 0;

  /* The real gain check */
  chance=((num*num)/10)+1;

  if(passcheck==USE_FAIL)
    chance =(int)((float)chance*(float)0.5);
  else if(passcheck == USE_PASS)
    chance =(int)((float)chance*(float)0.75);
  else if(passcheck == AUTO_FAIL)
    chance =(int)((float)chance*(float)1.0);
  else if(passcheck == AUTO_PASS)
    chance =(int)((float)chance*(float)1.5);
  else if(passcheck == PROF_FAIL)
    chance =(int)((float)chance*(float)0.75);
  else if(passcheck == PROF_PASS)
    chance =(int)((float)chance*(float)1.0);
  else
    {
    mudlogf(CMP,LVL_IMMORT,TRUE,
            "SYSERR: improve_skill() got spell:%d passcheck: %d",
            skill,passcheck);
    return 0;
    }

  percent  = 0;
  percent += int_app[stat_index(GET_INT(ch))].bonus;
  percent += wis_app[stat_index(GET_WIS(ch))].bonus;
  percent += dex_app[stat_index(GET_DEX(ch))].bonus;
  percent  = MAX(1,percent);

  /*
   * log("%d out of %d for %s",percent,chance,spells[skill].spell_name);
   */
  if(number(0,chance)<=percent)
    {
    /* set already gained flag */
    GET_LEARN_TIC(ch)++;
    GET_LAST_LEARN(ch)=skill;

    /* gain skill */
    if(is_spell==IS_SPELL)
      {
      GET_SKILL_LEARN(ch,skill)+=20;
      }
    else
      {
      GET_SKILL_LEARN(ch,skill)+=20;
      GET_SKILL(ch,skill)++;
      }

    /* skill mastery only - a spell's last rank comes from practice, never use */
    if(GET_SKILL_LEARN(ch,skill)==100 &&
        (spells[skill].is_spell==IS_SKILL)&&(GET_SKILL(ch,skill)>=95))
      {
      send_to_char(ch, "&MYou have mastered %s!&n\r\n",
                   spells[skill].spell_name);
      }
    else if(GET_SKILL_LEARN(ch,skill)==100)
      {
      send_to_char(ch, "&MYou need to visit a master to learn more about %s!&n\r\n", spells[skill].spell_name);
      }
    else
      {
      send_to_char(ch, "&MYour knowledge of %s increases!\r\n&n",
                   spells[skill].spell_name);
      }
    return 1;
    }
  return 0;
  }


