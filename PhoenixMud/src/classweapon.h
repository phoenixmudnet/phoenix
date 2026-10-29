/***********************************************************************
*  File: classweapon.h                                                 *
*  Usage: the class weapon -- one weapon for life, handed at the Hero  *
*         pass, bound, growing as its owner fights with it in hand     *
***********************************************************************/

#ifndef CLASSWEAPON_H
#define CLASSWEAPON_H

/* the switch: off unless etc/class_weapons says "enabled 1" */
extern int class_weapons;

void classweapon_boot(void);

/* the hooks; each does nothing with the switch off */
void cw_give_first(struct char_data *ch);
void cw_reclass(struct char_data *ch);
void cw_help_boot(void);
void cw_login(struct char_data *ch);
/* forms lent for a sitting come apart: leaving (at_login 0), or at login
 * once a day old */
void cw_end_sitting(struct char_data *ch, int at_login);
void cw_note_hit(struct char_data *ch, struct obj_data *wielded, int dam);
int  cw_in_form(struct obj_data *obj);
void cw_pay_kill(struct char_data *ch, struct char_data *victim);
void cw_begin_cast(struct char_data *ch, int mana);
void cw_end_cast(struct char_data *ch, int attack);
int  cw_heal_mod(struct char_data *ch, struct char_data *victim, int amount);
int  cw_damage_mod(struct char_data *ch, struct char_data *victim, int dam,
                   int attacktype);
void cw_boss_killed(struct char_data *killer, struct char_data *victim);
/* a copy just loaded from a file takes the form it was saved in */
void cw_loaded(struct obj_data *obj);
/* every two seconds: a tank or cast focus's pool pays out */
void cw_focus_tick(void);

/* identify: the binding and the class weapon's lines, appended to buf */
void cw_identify_lines(struct obj_data *obj, char *buf);
/* a bound piece serves its one character (immortals aside): FALSE and
 * the message when ch may not use it */
int  cw_bound_ok(struct char_data *ch, struct obj_data *obj, int message);
int  cw_bound_refuse(struct char_data *ch, struct obj_data *obj, const char *what,
                     struct char_data *to, int drop);
int  cw_take_refused(struct char_data *ch, struct obj_data *obj, int carried);
int  cw_junk_gate(struct char_data *ch, struct obj_data *obj, const char *kw, int confirm);
int  cw_junk_in_bulk(struct char_data *ch, struct obj_data *obj);
void cw_set_down_corpses(struct char_data *ch);
/* class armor: an attuned piece's stats on its wearer (equip, unequip and
 * affect_total), and the record beside the pfile at every save */
void cw_armor_affect(struct char_data *ch, struct obj_data *obj, int add);
void cw_armor_save(const char *name, long idnum);

#endif
