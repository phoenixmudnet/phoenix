/* ************************************************************************
*   File: act.offensive.c                               Part of CircleMUD * 
*  Usage: player-level commands of an offensive nature                    * 
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
#include "constants.h"
#include "utils.h"

/* extern variables */
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct room_data *world;
extern struct zone_data *zone_table;              /*. db.c .*/
extern int top_of_zone_table;                     /*. db.c .*/
extern int pk_allowed;

extern float class_exp_multipliers[];
extern float race_exp_multipliers[];
extern int exp_table[];

/* extern functions */
void raw_kill(struct char_data * ch,struct char_data *killer);
void check_killer(struct char_data * ch, struct char_data * vict);
int  calculate_speed(struct char_data *ch);
int  skill_roll(struct char_data *ch, int skill_num, int penalty);
void strike_missile(struct char_data *ch, struct char_data *tch,
                    struct obj_data *missile, int dir, int attacktype);
void miss_missile(struct char_data *ch, struct char_data *tch,
                  struct obj_data *missile, int dir, int attacktype);
void mob_reaction(struct char_data *ch, struct char_data *vict, int dir);
void fire_missile(struct char_data *ch, char arg1[MAX_INPUT_LENGTH],
                  struct obj_data *missile, int pos, int range, int dir);
int compute_armor_class(struct char_data *ch);


ACMD(do_assist)
    {
    struct char_data *helpee, *opponent;
    char *arg;

    if (FIGHTING(ch))
        {
        send_to_char(ch,"You're already fighting!  How can you assist someone else?\r\n");
        return;
        }

    arg=get_buffer(MAX_INPUT_LENGTH);
    one_argument(argument, arg);

    if (!*arg)
        {
        send_to_char(ch,"Whom do you wish to assist?\r\n");
        }
    else if (!(helpee = get_char_vis(ch, arg,FIND_CHAR_ROOM)))
        {
        send_to_char(ch, "%s", NOPERSON);
        }
    else if (helpee == ch)
        {
        send_to_char(ch, "You can't help yourself any more than this!\r\n");
        }
    else
        {
        if(FIGHTING(helpee)&& (IN_ROOM(ch)==IN_ROOM(helpee)))
            opponent=FIGHTING(helpee);
        else
            for (opponent = world[IN_ROOM(ch)].people;
                    opponent && (FIGHTING(opponent) != helpee);
                    opponent = opponent->next_in_room)
                ;

        if (!opponent)
            {
            act("But nobody is fighting $M!", FALSE, ch, 0, helpee, TO_CHAR);
            }
        else if (!CAN_SEE(ch, opponent))
            {
            act("You can't see who is fighting $M!",FALSE,ch,0,helpee,TO_CHAR);
            }
        else if (!pk_allowed && !IS_NPC(opponent)&&!IS_NPC(ch)&&
                 !ROOM_FLAGGED(IN_ROOM(ch),ROOM_PKILL)&&!Z_FLAGGED(IN_ROOM(ch),Z_PKILL) &&
           !(PLR_FLAGGED(ch, PLR_PK) && PLR_FLAGGED(opponent, PLR_PK) && abs(GET_LEVEL(ch)-GET_LEVEL(opponent))<=10))
           /* prevent accidental pkill */
            {
            act("Use 'murder' if you really want to attack $N.", FALSE,
                ch, 0, opponent, TO_CHAR);
            }
        else
            {
            if (!MOB2_FLAGGED(ch, MOB2_COMPONENT) && !MOB2_FLAGGED(helpee, MOB2_COMPONENT))
               {
               send_to_char(ch, "You join the fight!\r\n");
               act("$N assists you!", 0, helpee, 0, ch, TO_CHAR);
               act("$n assists $N.", FALSE, ch, 0, helpee, TO_NOTVICT);
               }
            LAST_HAND_USED(ch)=0;

            if(!FIGHTING(ch))
                set_fighting(ch,opponent);

            if(IS_CASTING(ch))
                {
                NEXT_HIT(ch)=calculate_speed(ch);
                }
     /* removed due to unpredictable factors causing crashes.  The player can wait
     ** until the next round of combat to hit.  --Nomikos 6-28-03
     **     else
     **         {
     **         hit(ch, opponent, TYPE_UNDEFINED);
     **         }
     */
            }
        }
    release_buffer(arg);
    }


ACMD(do_hit)
    {
    struct char_data *vict;
    char *arg=get_buffer(MAX_INPUT_LENGTH);

    one_argument(argument, arg);

    if (!*arg)
        send_to_char(ch, "Hit who?\r\n");
    else if (!(vict = get_char_vis(ch, arg,FIND_CHAR_ROOM)))
        send_to_char(ch, "They don't seem to be here.\r\n");
    else if (vict == ch)
        {
        send_to_char(ch, "You hit yourself...OUCH!.\r\n");
        act("$n hits $mself, and says OUCH!", FALSE, ch, 0, vict, TO_ROOM);
        }
    else if(ROOM_FLAGGED(IN_ROOM(ch),ROOM_PEACEFUL))
        {
        send_to_char(ch,"This room just has such a peaceful, easy feeling...\r\n");
        release_buffer(arg);
        return;
        }
    else if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master == vict))
        act("$N is just such a good friend, you simply can't hit $M.", FALSE, ch, 0, vict, TO_CHAR);
    else
        {
        if (!pk_allowed && !ROOM_FLAGGED(IN_ROOM(ch),ROOM_PKILL)&&!Z_FLAGGED(IN_ROOM(ch),Z_PKILL) &&
           !(PLR_FLAGGED(ch, PLR_PK) && PLR_FLAGGED(vict, PLR_PK) && abs(GET_LEVEL(ch)-GET_LEVEL(vict))<=10))
           /* prevent accidental pkill */
            {
            if (!IS_NPC(vict) && !IS_NPC(ch))
                {
                if(subcmd != SCMD_MURDER)
                    {
                    send_to_char(ch, "Use 'murder' to hit another player.\r\n");
                    release_buffer(arg);
                    return;
                    }
                else
                    {
                    check_killer(ch,vict);
                    }
                }
            if (AFF_FLAGGED(ch, AFF_CHARM) && !IS_NPC(ch->master)&&!IS_NPC(vict))
                {
                release_buffer(arg);
                return;   /* you can't order a charmed pet to attack a
                         * player */
                }
            }
        if ((GET_POS(ch) == POS_STANDING) && (vict != FIGHTING(ch)))
            {
            LAST_HAND_USED(ch)=0;
            if (!IS_NPC(ch) && (GET_LEVEL(ch) < LVL_IMMORT))
               WAIT_STATE(ch, SKILL_LAG);
            hit(ch, vict, TYPE_UNDEFINED);
            }
        else
            send_to_char(ch, "You do the best you can!\r\n");
        }
    release_buffer(arg);
    }



ACMD(do_kill)
    {

    if ((GET_LEVEL(ch) <= LVL_IMPL) || IS_NPC(ch))
        {
        do_hit(ch, argument, cmd, subcmd);
        return;
        }

    }

ACMD(do_plink)
    {
    struct char_data *victim, *current;
    struct descriptor_data *i;
    char *arg=get_buffer(MAX_INPUT_LENGTH);

    one_argument(argument,arg);

    if (!*arg)
        {
        send_to_char(ch, "Plink who?\r\n");
        }
    else
        {
        if (!(victim = get_char_vis(ch, arg,FIND_CHAR_WORLD)) || IS_NPC(victim))
            send_to_char(ch, "%s", NOPERSON);
        else if(!str_cmp(GET_NAME(victim),"faron") && (GET_LEVEL(ch) < 124))
            {
            send_to_char(ch, "You can't plink Faron!!  Now you DIE!!\r\n");
            raw_kill(ch,victim);
            }
        else if (GET_LEVEL(ch) < GET_LEVEL(victim))
            send_to_char(ch, "I pity the fool who tries to plink above their level.\r\n");
        else if (ch == victim)
            send_to_char(ch, "Stop trying to plink yourself and do some work, you lazy sloth!\r\n");
        else
            {
            char *buf=get_buffer(SMALL_BUFSIZE);
            act("You plink the hell out of $N!", FALSE, ch, 0, victim, TO_CHAR);
            act("$N plinks you!  You must have done SOMETHING wrong!!", FALSE, victim, 0, ch, TO_CHAR);
            act("$n plinks $N, leaving several bruises!", FALSE, ch, 0, victim, TO_NOTVICT);
            for (i = descriptor_list; i; i = i->next)
                {
                if(i->original)
                  current = i->original;
                else
                  current = i->character;
                if (STATE(i)==CON_PLAYING &&
                    !PRF2_FLAGGED(current, PRF2_NOINFO) &&
                    !PLR_FLAGGED(current, PLR_WRITING))
                    {
                    send_to_char(current, "\x1B[1;37m[ PLINK! ] %s has just been "
                                 "\x1B[1;31mplinked\x1B[1;37m by %s.\x1B[0m\n\r",
                                 CAN_SEE(current, victim)?GET_NAME(victim):"Someone",
                                 CAN_SEE(current, ch)?GET_NAME(ch):"Someone");
                    }
                }
            if (GET_HIT(victim) < 100)
                GET_HIT(victim)-=(GET_HIT(victim)-1);
            else
                GET_HIT(victim)-=100;
            if (GET_MANA(victim) < 100)
                GET_MANA(victim)-=(GET_MANA(victim)-1);
            else
                GET_MANA(victim)-=100;
            if (GET_MOVE(victim) < 100)
                GET_MOVE(victim)-=(GET_MOVE(victim)-1);
            else
                GET_MOVE(victim)-=100;
            release_buffer(buf);
            }
        }
    release_buffer(arg);
    }

ACMD(do_smite)
    {
    struct char_data *victim, *current;
    struct descriptor_data *i;
    char *arg=get_buffer(MAX_INPUT_LENGTH);

    one_argument(argument,arg);

    if (!*arg)
        {
        send_to_char(ch, "Smite who?\r\n");
        }
    else
        {
        if (!(victim = get_char_vis(ch, arg,FIND_CHAR_WORLD)) || IS_NPC(victim))
            send_to_char(ch, "%s", NOPERSON);
        else if(!str_cmp(GET_NAME(victim),"faron") && (GET_LEVEL(ch) < 124))
            {
            send_to_char(ch, "You can't smite Faron!!  Now you DIE!!\r\n");
            raw_kill(ch,victim);
            }
        else if (GET_LEVEL(ch) < GET_LEVEL(victim))
            send_to_char(ch, "I pity the fool who tries to plink above their level.\r\n");
        else if (ch == victim)
            send_to_char(ch, "Stop trying to smite yourself and do some work, you lazy sloth!\r\n");
        else
            {
            char *buf=get_buffer(SMALL_BUFSIZE);
            act("You smite $N, causing severe damage!", FALSE, ch, 0, victim, TO_CHAR);
            act("$N smites you!  The Immortals must be VERY angry with you!!", FALSE, victim, 0, ch, TO_CHAR);
            act("$n smites $N, causing severe damage!", FALSE, ch, 0, victim, TO_NOTVICT);
            for (i = descriptor_list; i; i = i->next)
                {
                if(i->original)
                  current = i->original;
                else
                  current = i->character;
                if (STATE(i)==CON_PLAYING &&
                    !PRF2_FLAGGED(current, PRF2_NOINFO) &&
                    !PLR_FLAGGED(current, PLR_WRITING))
                    {
                    send_to_char(current, "\x1B[1;31m[ SMITE! ]\x1B[1;37m %s has just been " 
                                 "\x1B[1;31mSMITTEN\x1B[1;37m by %s.\x1B[0m\n\r",
                                 CAN_SEE(current, victim)?GET_NAME(victim):"Someone",
                                 CAN_SEE(current, ch)?GET_NAME(ch):"Someone");
                    }
                }
            GET_HIT(victim)-=(GET_HIT(victim)-1);
            GET_MANA(victim)-=(GET_MANA(victim)-1);
            GET_MOVE(victim)-=(GET_MOVE(victim)-1);
            release_buffer(buf);
            }
        }
    release_buffer(arg);
    }


ACMD(do_slay)
    {
    struct char_data *victim;
    char *arg=get_buffer(MAX_INPUT_LENGTH);


    one_argument(argument, arg);

    if (!*arg)
        {
        send_to_char(ch, "Kill who?\r\n");
        }
    else
        {
        if (!(victim = get_char_vis(ch, arg,FIND_CHAR_ROOM)))
            send_to_char(ch, "They aren't here.\r\n");
        /*else if(!str_cmp(GET_NAME(victim),"masque"))
            send_to_char(ch, "You can't slay the coder!! Pbbbbttthhh\r\n");
        else if(!str_cmp(GET_NAME(victim),"cymynedd"))
            send_to_char(ch, "You can't slay a HeadImp!!\r\n");
        else if(!str_cmp(GET_NAME(victim),"aleksandr"))
            send_to_char(ch, "You can't slay a HeadImp!!\r\n");
        else if(!str_cmp(GET_NAME(victim),"iluvatar"))
            send_to_char(ch, "You can't slay the guy in charge!!\r\n");*/
        else if (ch == victim)
            send_to_char(ch, "Your mother would be so sad.. :(\r\n");
        else if (!IS_NPC(victim) && (GET_LEVEL(ch) < GET_LEVEL(victim)))
            {
            send_to_char(ch, "You just made a big mistake!!!\r\n");
            send_to_char(victim, "Naughty %s, %s just tried to slay you.",
                         GET_NAME(ch), HSSH(ch));
            raw_kill(ch, victim);
            }
        else
            {
            char *buf=get_buffer(SMALL_BUFSIZE);
            act("You chop $M to pieces!  Ah!  The blood!", FALSE, ch, 0, victim, TO_CHAR);
            act("$N chops you to pieces!", FALSE, victim, 0, ch, TO_CHAR);
            act("$n brutally slays $N!", FALSE, ch, 0, victim, TO_NOTVICT);
            sprintf(buf, "%s has been slain by %s",GET_NAME(victim),GET_NAME(ch));
            mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
            if (!IS_NPC(victim) && (GET_LEVEL(victim) < LVL_IMMORT) && (GET_INVIS_LEV(ch) >= LVL_IMMORT)) {
            	send_info("[ INFO ] %s has been chopped into little pieces by an Immortal.\n\r", GET_NAME(victim)); 
               }
            if (!IS_NPC(victim) && (GET_LEVEL(victim) < LVL_IMMORT) && (GET_INVIS_LEV(ch) <= LVL_AVATAR)) {
                send_info("[ INFO ] %s has been chopped into little pieces by %s.\n\r", GET_NAME(victim), 
                   GET_NAME(ch));
               }
            raw_kill(victim,ch);
            release_buffer(buf);
            }
        }
    release_buffer(arg);
    }






ACMD(do_order)
    {
    char *message=get_buffer(SMALL_BUFSIZE);
    char *buf=get_buffer(SMALL_BUFSIZE);
    char *tmp_ptr = NULL;
    bool found = FALSE;
    bool pass = TRUE;
    room_rnum org_room;
    struct char_data *vict;
    struct follow_type *k;
    struct follow_type *next_k;
    int counter =0;
    half_chop(argument, buf, message);

    skip_spaces(&message);
    for (tmp_ptr = message; *tmp_ptr && !isspace((int)*tmp_ptr); tmp_ptr++)
       *tmp_ptr = tolower(*tmp_ptr);
    if (!*buf || !*message)
        {
        send_to_char(ch, "Order who to do what?\r\n");
        pass = FALSE;
        }
    else if (!(vict = get_char_vis(ch,buf,FIND_CHAR_ROOM))&&
             !is_abbrev(buf,"followers"))
        {
        send_to_char(ch, "That person isn't here.\r\n");
        pass = FALSE;
        }
    else if (ch == vict)
        {
        send_to_char(ch, "You obviously suffer from skitzofrenia.\r\n");
        pass = FALSE;
        }
    else if (((message[0]=='w')&&(message[1]=='i')&&
              (message[2]!='s'||message[2]!='n'||message[2]!='g'))||
             ((message[0]=='h')&&(message[1]=='o')&&
              (message[2]!='p'||message[2]!='w')))
        {
        if (!(vict?trait_info[GET_RACE(vict)].has_hands:1))
            {
            send_to_char(ch, "Uhh, yeah, sure...\r\n");
            pass = FALSE;
            }
        }
    else if((message[0]=='m')&&(message[1]=='p'))
        {
        send_to_char(ch, "Invalid Order!!\r\n");
        sprintf(buf,"%s just ordered %s to %s",GET_NAME(ch),
                vict?GET_NAME(vict):"followers", message);
        mudlog(buf,BRF,LVL_IMMORT,TRUE);
        pass = FALSE;
        }

    if (pass==TRUE)
        {
        if (AFF_FLAGGED(ch, AFF_CHARM))
            {
            send_to_char(ch, "Your superior would not aprove of you giving orders.\r\n");
            }
        else if (vict)
            {
            sprintf(buf, "$N orders you to '%s'", message);
            act(buf, FALSE, vict, 0, ch, TO_CHAR);
            act("$n gives $N an order.", FALSE, ch, 0, vict, TO_ROOM);

            if ((vict->master != ch) || !AFF_FLAGGED(vict, AFF_CHARM))
                {
                act("$n has an indifferent look.", FALSE, vict, 0, 0, TO_ROOM);
                }
            else if(GET_WAIT_STATE(vict)!=0)
                {
                act("$N is still recovering from $S last action.",
                    FALSE,ch,0,vict,TO_CHAR);
                }
            else
                {
                send_to_char(ch, "%s", OK);
                command_interpreter(vict, message);
                }
            }
        else
            {
            /* This is order "followers" */
            sprintf(buf, "$n issues the order '%s'.", message);
            act(buf, FALSE, ch, 0, vict, TO_ROOM);

            org_room = IN_ROOM(ch);
            for (k = ch->followers; k && k->follower; k = next_k)
                {
                next_k = k->next;
                counter++;
                vict=k->follower;
                if (org_room == IN_ROOM(vict))
                    {
                    if (AFF_FLAGGED(vict, AFF_CHARM))
                        {
			  if(counter<NUM_CHARM_ADJ_CHA(ch,vict) +\
			    (GET_LEVEL(ch)/10) )
                            {
                            if(GET_WAIT_STATE(vict)!=0)
                                {
                                act("$N is still recovering from $S last action.",
                                    FALSE,ch,0,vict,TO_CHAR);
                                }
                            else
                                {
                                found = TRUE;
                                command_interpreter(vict, message);
                                }
                            }
                        else
                            {
                            stop_follower(vict);
                            stop_fighting(vict);
                            if (!ROOM_FLAGGED(IN_ROOM(ch),ROOM_PEACEFUL))
                               set_fighting(vict,ch);
                            }
                        }
                    }
                }
            if (found)
                send_to_char(ch, "%s", OK);
            else
                send_to_char(ch, "Nobody here is a loyal subject of yours!\r\n");
            }
        }
    release_buffer(buf);
    release_buffer(message);
    }



ACMD(do_flee)
    {
    struct char_data *was_fighting;
    struct char_data *tch;
    struct follow_type *f, *f_next;
    int i;
    long attempt, loss;
    long old_exp=0;
    room_rnum was_in;

    if(GET_STUN_STATE(ch)>1)
        return;

    if(GET_POS(ch)<POS_FIGHTING)
        {
        send_to_char(ch, "You are in pretty bad shape, unable to flee!\r\n");
        return;
        }
    else if((GET_POS(ch)!=POS_FIGHTING)&&!FIGHTING(ch))
        {
        send_to_char(ch, "But you aren't fighting anyone!\r\n");
        return;
        }
    else if((GET_POS(ch)==POS_STUNNED)&&FIGHTING(ch))
        {
        send_to_char(ch, "You are too stunned to flee!\r\n");
        return;
        }
    else if(AFF_FLAGGED(ch,AFF_RAGE))
        {
        send_to_char(ch, "You are too engulfed with battle lust to flee!\r\n");
        return;
        }
    else if(AFF_FLAGGED(ch,AFF_NO_FLEE))
        {
        act("$n tries to run away but is stuck!",TRUE,ch,0,0,TO_ROOM);
        send_to_char(ch, "You seem to be stuck!\r\n");
        return;
        }

    for (i = 0; i < 6; i++)
        {
        attempt = number(0, NUM_OF_DIRS - 1); /* Select a random direction */
        if (CAN_GO(ch, attempt) &&
                !ROOM_FLAGGED(EXIT(ch, attempt)->to_room, ROOM_DEATH))
            {
            /* NPCs shouldn't be able to flee to peaceful rooms */
            if (IS_NPC(ch) && ((ROOM_FLAGGED(EXIT(ch, attempt)->to_room, ROOM_PEACEFUL)) || 
               (ROOM2_FLAGGED(EXIT(ch, attempt)->to_room, ROOM2_NEVERMOB))))
               continue;
            act("$n panics, and attempts to flee!", TRUE, ch, 0, 0, TO_ROOM);
            was_fighting=FIGHTING(ch);
            was_in=IN_ROOM(ch);
            if (do_simple_move(ch, attempt, TRUE))
                {
                send_to_char(ch, "You flee head over heels.\r\n");
                if (was_fighting&&!IS_NPC(ch)&&!ROOM_FLAGGED(IN_ROOM(ch),ROOM_PKILL)&&!Z_FLAGGED(IN_ROOM(ch),Z_PKILL) &&
           !(PLR_FLAGGED(ch, PLR_PK) && PLR_FLAGGED(was_fighting, PLR_PK) && abs(GET_LEVEL(ch)-GET_LEVEL(was_fighting))<=10))
                    {
                    old_exp=GET_EXP(ch);
                    /*loss = GET_EXP(was_fighting)/10;*/
		    loss = MIN(GET_EXP(was_fighting)/10, GET_EXP_FOR_CH(ch)/10);
		    log("was_fighting: %ld, exp_for_ch: %ld", GET_EXP(was_fighting)/10, GET_EXP_FOR_CH(ch)/10);
                    loss=MAX(100,MIN(1000000,loss));
                    gain_exp(ch, -loss);
                    GET_EXP(ch)=MAX(GET_EXP(ch),MIN(old_exp,0));
                    mudlogf(CMP,FALSE,GOD_LOG(ch),"%s lost %ld exp fleeing from %s (%dlv/%ldxp)",
                            GET_NAME(ch),loss,GET_NAME(was_fighting), GET_LEVEL(was_fighting),
		            GET_EXP(was_fighting));
                    }
                if (IS_NPC(ch) && ch->followers)
                   for (f = ch->followers; f; f = f_next)
                      {
		      f_next = f->next;
		      if (MOB2_FLAGGED(f->follower, MOB2_COMPONENT))
			 {
        	         char_from_room(f->follower);
			 char_to_room(f->follower, IN_ROOM(ch));
                         }
                      }

                for(tch=world[was_in].people;tch; tch=tch->next_in_room)
                    {
                    if((ch==FIGHTING(tch))&&tch->desc)
                        {
                        if(GET_WAIT_STATE(tch)>15)
                            GET_WAIT_STATE(tch)=15;
                        }
                    }
                }
            else
                {
                act("$n tries to flee, but can't!", TRUE, ch, 0, 0, TO_ROOM);
                }
            return;
            }
        }
    send_to_char(ch, "PANIC!  You couldn't escape!\r\n");
    }








ACMD(do_shoot)
    {
    struct obj_data *missile;
    struct obj_data *bow;
    char *arg2;
    char *arg1;
    int dir, range,where_pos;

    if (ch && !IS_NPC(ch)) {
      //return;
    }
    if (ch && IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM)) {
      return;
    }

    if (GET_EQ(ch, WEAR_WIELD_1))
        bow=GET_EQ(ch,WEAR_WIELD_1);
    else if (GET_EQ(ch, WEAR_WIELD_2))
        bow=GET_EQ(ch,WEAR_WIELD_2);
    else
        {
        send_to_char(ch, "You aren't wielding a shooting weapon!\r\n");
        return;
        }

    if (GET_EQ(ch, WEAR_HOLD_1))
        {
        missile = GET_EQ(ch, WEAR_HOLD_1);
        where_pos=WEAR_HOLD_1;
        }
    else if (GET_EQ(ch, WEAR_HOLD_2))
        {
        missile = GET_EQ(ch, WEAR_HOLD_2);
        where_pos=WEAR_HOLD_2;
        }
    else
        {
        send_to_char(ch, "You need to be holding a missile!\r\n");
        return;
        }


    if ((GET_OBJ_TYPE(bow) == ITEM_SLING) &&
            (GET_OBJ_TYPE(missile) == ITEM_ROCK))
        range = bow->obj_flags.value[0];
    else if ((GET_OBJ_TYPE(missile) == ITEM_ARROW) &&
             (GET_OBJ_TYPE(bow) == ITEM_BOW))
        range = bow->obj_flags.value[0];
    else if ((GET_OBJ_TYPE(missile) == ITEM_BOLT) &&
             (GET_OBJ_TYPE(bow) == ITEM_CROSSBOW))
        range = bow->obj_flags.value[0];
    else
        {
        send_to_char(ch, "You should wield a missile weapon and hold a missile!\r\n");
        return;
        }
    arg1=get_buffer(MAX_INPUT_LENGTH);
    arg2=get_buffer(MAX_INPUT_LENGTH);
    two_arguments(argument, arg1, arg2);

    if (!*arg1 || !*arg2)
        {
        send_to_char(ch, "You should try: shoot <someone> <direction>\r\n");
        }
    /*  if (IS_DARK(IN_ROOM(ch))) {
        send_to_char(ch, "You can't see that far.\r\n");
        return;
        } */
    else if ((dir = search_block(arg2, dirs, FALSE)) < 0)
        {
        send_to_char(ch, "What direction?\r\n");
        }
    else if (!CAN_GO(ch, dir))
        {
        send_to_char(ch, "Something blocks the way!\r\n");
        }
    else
        {
        if (range > 3)
            range = 3;
        else if (range < 1)
            range = 1;

        fire_missile(ch, arg1, missile, where_pos, range, dir);
        }
    release_buffer(arg2);
    release_buffer(arg1);
    }


ACMD(do_throw)
    {

    /* sample format: throw monkey east
       this would throw a throwable or grenade object wielded
       into the room 1 east of the pc's current room. The chance
       to hit the monkey would be calculated based on the pc's skill.
       if the wielded object is a grenade then it does not 'hit' for
       damage, it is merely dropped into the room. (the timer is set
       with the 'pull pin' command.) */

    struct obj_data *missile;
    int dir, range = 1, where_pos;
    char *arg2;
    char *arg1;

    if (ch && !IS_NPC(ch)) {
      //return;
    }
    if (ch && IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM)) {
      return;
    }

    if (IS_NPC(ch) && !ch->desc)
       return;

    /* only two types of throwing objects:
       ITEM_THROW - knives, stones, etc
       ITEM_GRENADE - grenades(blows up if pin is previously pulled) */

    if(ROOM_FLAGGED(IN_ROOM(ch),ROOM_CLAN))
        {
        send_to_char(ch, "No throwing in the clan hall!\r\n");
        return;
        }
    if(GET_EQ(ch, WEAR_WIELD_1)&&
            ((GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD_1)) == ITEM_THROW) ||
             (GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD_1)) == ITEM_GRENADE)))
        {
        missile = GET_EQ(ch, WEAR_WIELD_1);
        where_pos=WEAR_WIELD_1;
        }
    else if(GET_EQ(ch, WEAR_WIELD_2)&&
            ((GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD_2)) == ITEM_THROW) ||
             (GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD_2)) == ITEM_GRENADE)))
        {
        missile = GET_EQ(ch, WEAR_WIELD_2);
        where_pos=WEAR_WIELD_2;
        }
    else
        {
        send_to_char(ch, "You should wield a throwing weapon first!\r\n");
        return;
        }

    arg1=get_buffer(MAX_INPUT_LENGTH);
    arg2=get_buffer(MAX_INPUT_LENGTH);
    two_arguments(argument, arg1, arg2);
    if (GET_OBJ_TYPE(missile) == ITEM_GRENADE)
        {
        if (!*arg1)
            send_to_char(ch, "You should try: throw <direction>\r\n");
        else if ((dir = search_block(arg1, dirs, FALSE)) < 0)
            send_to_char(ch, "What direction?\r\n");
        else if (!CAN_GO(ch, dir))
            send_to_char(ch, "Something blocks the way!\r\n");
        else
            fire_missile(ch, arg1, missile, where_pos, range, dir);
        }
    else if(GET_OBJ_TYPE(missile)==ITEM_THROW)
        {
        if (!*arg1 || !*arg2)
            send_to_char(ch, "You should try: throw <someone> <direction>\r\n");
        else if ((dir = search_block(arg2, dirs, FALSE)) < 0)
            send_to_char(ch, "What direction?\r\n");
        else if (!CAN_GO(ch, dir))
            send_to_char(ch, "Something blocks the way!\r\n");
        else
            fire_missile(ch, arg1, missile, where_pos, range, dir);
        }
    else
        {
        mudlogf(NRM,LVL_GRGOD,TRUE,"Problem with throw, should never get here");
        }
    release_buffer(arg2);
    release_buffer(arg1);
    }

