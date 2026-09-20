/* ************************************************************************
*   File: account.c                                    Part of PhoenixMUD *
*  Usage: the account roster -- one login, many characters                *
*                                                                         *
*  See account.h for the ownership model. This file is the store, the     *
*  roster render, and the rules that decide which character an account    *
*  IS. It never writes a pfile except through save_accounts' callers.     *
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
#include "account.h"

extern struct descriptor_data *descriptor_list;

struct account_data *account_list = NULL;
int top_of_accountt = 0;

/* 0 = NO LIMIT. The owner left it unset (2026-09-18), so nothing may assume
 * two members of one account are serialized. */
int account_default_max_online = 0;

/* ------------------------------------------------------------------ */
/*  The store                                                          */
/* ------------------------------------------------------------------ */

/*
 * etc/accounts, one account per line:
 *
 *     <main> <max_online> <member> [<member> ...]
 *
 * ASCII on purpose. It is small, a god may need to repair it by hand, and
 * the TS side reads the same rosters out of world/accounts.json -- keeping
 * both human-readable is what makes a mismatch findable.
 */
void boot_accounts(void)
{
   FILE *fl;
   char line[MAX_STRING_LENGTH], *p;
   struct account_data *acct;

   CREATE(account_list, struct account_data, MAX_ACCOUNTS);
   top_of_accountt = 0;

   if (!(fl = fopen(ACCOUNT_FILE, "r"))) {
      /* ABSENT IS NOT AN ERROR. No roster means every character is a login
       * of its own, which is the pre-account behaviour and exactly what a
       * fresh lib should do. */
      log("   No accounts file -- every character logs in on its own.");
      return;
   }

   while (fgets(line, sizeof(line), fl)) {
      if (*line == '*' || *line == '\n' || *line == '#')
         continue;
      if (top_of_accountt >= MAX_ACCOUNTS) {
         log("SYSERR: etc/accounts holds more than %d accounts; the rest are ignored.",
             MAX_ACCOUNTS);
         break;
      }
      acct = &account_list[top_of_accountt];
      memset(acct, 0, sizeof(struct account_data));

      if (!(p = strtok(line, " \t\r\n")))
         continue;
      strncpy(acct->name, p, MAX_NAME_LENGTH);
      acct->name[MAX_NAME_LENGTH] = '\0';

      if ((p = strtok(NULL, " \t\r\n")))
         acct->max_online = atoi(p);

      while ((p = strtok(NULL, " \t\r\n")) && acct->num_members < MAX_ACCT_MEMBERS) {
         strncpy(acct->members[acct->num_members], p, MAX_NAME_LENGTH);
         acct->members[acct->num_members][MAX_NAME_LENGTH] = '\0';
         acct->num_members++;
      }
      /* An account with no members cannot be logged into and would keep
       * answering for a name nobody can play, so it is not kept. */
      if (acct->num_members > 0)
         top_of_accountt++;
   }
   fclose(fl);
   log("   %d account(s) loaded.", top_of_accountt);
}

void save_accounts(void)
{
   FILE *fl;
   int i, j;

   if (!(fl = fopen(ACCOUNT_FILE, "w"))) {
      log("SYSERR: cannot write %s -- roster changes this boot are LOST.", ACCOUNT_FILE);
      return;
   }
   for (i = 0; i < top_of_accountt; i++) {
      if (account_list[i].num_members <= 0)
         continue;
      fprintf(fl, "%s %d", account_list[i].name, account_list[i].max_online);
      for (j = 0; j < account_list[i].num_members; j++)
         fprintf(fl, " %s", account_list[i].members[j]);
      fprintf(fl, "\n");
   }
   fclose(fl);
}

/* ------------------------------------------------------------------ */
/*  Lookup                                                             */
/* ------------------------------------------------------------------ */

struct account_data *find_account(char *name)
{
   int i;

   if (!name || !*name)
      return NULL;
   for (i = 0; i < top_of_accountt; i++)
      if (!str_cmp(account_list[i].name, name))
         return &account_list[i];
   return NULL;
}

/* Which account owns this character? A character belongs to at most one, and
 * the roster is the only record of that -- so a character on no roster is
 * simply a standalone login, exactly as before. */
struct account_data *account_of_char(char *char_name)
{
   int i, j;

   if (!char_name || !*char_name)
      return NULL;
   for (i = 0; i < top_of_accountt; i++)
      for (j = 0; j < account_list[i].num_members; j++)
         if (!str_cmp(account_list[i].members[j], char_name))
            return &account_list[i];
   return NULL;
}

int account_has_char(struct account_data *acct, char *char_name)
{
   int j;

   if (!acct || !char_name)
      return FALSE;
   for (j = 0; j < acct->num_members; j++)
      if (!str_cmp(acct->members[j], char_name))
         return TRUE;
   return FALSE;
}

/* Idempotent: re-adding a member is a no-op, not a duplicate row. */
int account_add_char(struct account_data *acct, char *char_name)
{
   if (!acct || !char_name || !*char_name)
      return FALSE;
   if (account_has_char(acct, char_name))
      return TRUE;
   if (acct->num_members >= MAX_ACCT_MEMBERS)
      return FALSE;
   strncpy(acct->members[acct->num_members], char_name, MAX_NAME_LENGTH);
   acct->members[acct->num_members][MAX_NAME_LENGTH] = '\0';
   acct->num_members++;
   return TRUE;
}

/*
 * Drop a character off its roster. Called when a character is DELETED, and
 * only then: leaving the name on the account keeps it answering for a
 * character nobody can play. An account whose last member goes is removed
 * outright rather than left as an empty roster nothing can log into.
 */
int account_remove_char(struct account_data *acct, char *char_name)
{
   int j, k, idx = -1;

   if (!acct || !char_name)
      return FALSE;
   for (j = 0; j < acct->num_members; j++)
      if (!str_cmp(acct->members[j], char_name)) {
         idx = j;
         break;
      }
   if (idx < 0)
      return FALSE;

   /* Compact rather than leave a hole: do_ignore's fixed-slot array has a
    * refill bug from exactly that shortcut, and it is not worth repeating. */
   for (k = idx; k < acct->num_members - 1; k++)
      strcpy(acct->members[k], acct->members[k + 1]);
   acct->num_members--;

   if (acct->num_members <= 0) {
      int i = acct - account_list;
      for (k = i; k < top_of_accountt - 1; k++)
         account_list[k] = account_list[k + 1];
      top_of_accountt--;
      return TRUE;
   }
   if (!str_cmp(acct->name, char_name))
      strcpy(acct->name, acct->members[0]);
   account_pick_main(acct);
   return TRUE;
}

/* Mint an account for a character that has none. Every character belongs to
 * exactly one account after this, including a solo one -- a roster of one
 * and a roster of six take the same login path, so there is never a second
 * flow to keep in step. */
struct account_data *account_create_for(char *char_name)
{
   struct account_data *acct;

   if (!char_name || !*char_name || find_account(char_name))
      return NULL;
   if (top_of_accountt >= MAX_ACCOUNTS)
      return NULL;

   acct = &account_list[top_of_accountt];
   memset(acct, 0, sizeof(struct account_data));
   strncpy(acct->name, char_name, MAX_NAME_LENGTH);
   acct->name[MAX_NAME_LENGTH] = '\0';
   strcpy(acct->members[0], acct->name);
   acct->num_members = 1;
   top_of_accountt++;
   return acct;
}

/* ------------------------------------------------------------------ */
/*  Bands and the online dial                                          */
/* ------------------------------------------------------------------ */

int account_band(int level)
{
   if (level >= LVL_IMPL)
      return ACCT_BAND_IMPL;
   if (level >= LVL_IMMORT)
      return ACCT_BAND_IMMORTAL;
   return ACCT_BAND_MORTAL;
}

/* Does this character share account-scoped property? The 105-126 band does
 * not; it behaves exactly as it did before accounts existed. */
int account_shares_property(int level)
{
   return account_band(level) != ACCT_BAND_IMMORTAL;
}

int account_online_limit(struct account_data *acct)
{
   if (acct && acct->max_online > 0)
      return acct->max_online;
   return account_default_max_online;
}

int account_online_count(struct account_data *acct)
{
   struct descriptor_data *d;
   int n = 0;

   if (!acct)
      return 0;
   for (d = descriptor_list; d; d = d->next) {
      if (STATE(d) != CON_PLAYING || !d->character)
         continue;
      if (account_has_char(acct, GET_PC_NAME(d->character)))
         n++;
   }
   return n;
}

/* ------------------------------------------------------------------ */
/*  The main                                                           */
/* ------------------------------------------------------------------ */

/*
 * Does `a` outrank `b` as this account's main?
 *
 * A staff character first, then -- among mortals -- remort tier, then level,
 * then time played.
 *
 * The two-step is not decoration and it is wrong in both directions without
 * it. Remorting RESETS level to 1 (do_remort, act.other.c), so ranking
 * mortals on level alone hands the account's identity to an alt the moment
 * the veteran remorts, and hands it back on the way up. But immortals never
 * remort, so ranking on remort tier across the whole roster lets a
 * once-remorted level 1 outrank a level 127 implementor. Compare within the
 * band; order the bands above it.
 *
 * Holding the incumbent on a full tie keeps the choice stable rather than
 * alphabetical. Kept identical to pickMain() in the TS AccountStore -- the
 * two engines must not disagree about which character an account IS.
 */
static int roster_outranks(struct char_file_u *a, struct char_file_u *b)
{
   int ia = (a->level >= LVL_IMMORT);
   int ib = (b->level >= LVL_IMMORT);
   int ra, rb;

   if (ia != ib)
      return ia;
   ra = a->player_specials_saved.times_remorted;
   rb = b->player_specials_saved.times_remorted;
   if (!ia && ra != rb)
      return ra > rb;
   if (a->level != b->level)
      return a->level > b->level;
   if (a->played != b->played)
      return a->played > b->played;
   return FALSE;                        /* keep the incumbent */
}

/*
 * Recompute the main and rename the account to it.
 *
 * Renaming is safe at any time: nothing resolves an account by this name
 * except find_account, and login resolves an account from whichever
 * CHARACTER name was typed. A member name is unique across the MUD, so it
 * cannot collide.
 */
void account_pick_main(struct account_data *acct)
{
   struct char_file_u best, cur;
   char best_name[MAX_NAME_LENGTH + 1];
   int j, have = FALSE;

   if (!acct || acct->num_members <= 0)
      return;

   for (j = 0; j < acct->num_members; j++) {
      if (load_char(acct->members[j], &cur) < 0)
         continue;                      /* unreadable cannot be the main */
      if (!have || roster_outranks(&cur, &best)) {
         best = cur;
         strcpy(best_name, acct->members[j]);
         have = TRUE;
      }
   }
   if (have)
      strcpy(acct->name, best_name);
}

/* ------------------------------------------------------------------ */
/*  Where the money lives                                              */
/* ------------------------------------------------------------------ */

const char *account_purse_holder(char *char_name)
{
   struct account_data *acct = account_of_char(char_name);

   if (!acct)
      return char_name;
   /* The 105-126 band overlaps nothing -- see the band comment in account.h.
    * A character in it banks and carries on its own record. */
   {
      struct char_file_u me;
      if (load_char(char_name, &me) >= 0
          && !account_shares_property(me.level))
         return char_name;
   }
   return acct->name;
}

/* ------------------------------------------------------------------ */
/*  The roster screen                                                  */
/* ------------------------------------------------------------------ */

extern int exp_table[];
extern const float class_exp_multipliers[];
extern const float race_exp_multipliers[];
extern char *pc_race_types[];
extern char *pc_class_types[];
void save_char_ascii(struct char_file_u *ch);

/*
 * Draw the account's characters.
 *
 * Renders from char_file_u via load_char, so drawing the screen never
 * instantiates a character -- a roster of twenty costs twenty reads and no
 * world state. A tombstoned member is filtered out here rather than at each
 * caller: its pfile still loads, so a screen that listed it would offer a
 * row that enters the game as a deleted character.
 *
 * Kept in step with the TS renderAccountMenu. The two engines draw the same
 * screen because a player moves between them across the interchange and a
 * changed menu reads as a broken one.
 */
/* Column widths, and the ONE place they are written. Kept identical to TCOLS
 * in the TS roster.ts: a player moves between the two engines across the
 * interchange, and a screen that shifts by a column reads as a broken one. */
#define RC_NAME  13
#define RC_RACE   9
#define RC_CLASS 13
#define RC_LVL    3
#define RC_NEXT  15
#define ACCT_FIELD 15                   /* width of $ACCOUNT$ in the banner */
#define RM_CELL   24                    /* option-cell width */

static void roster_rule(struct descriptor_data *d)
{
   char b[MAX_STRING_LENGTH];
   int w[5] = { RC_NAME, RC_RACE, RC_CLASS, RC_LVL, RC_NEXT }, i, n = 0;

   n += snprintf(b + n, sizeof(b) - n, "  +");
   for (i = 0; i < 5; i++) {
      int k;
      for (k = 0; k < w[i] + 2; k++)
         n += snprintf(b + n, sizeof(b) - n, "-");
      n += snprintf(b + n, sizeof(b) - n, "+");
   }
   snprintf(b + n, sizeof(b) - n, "\r\n");
   SEND_TO_Q(d, ACCT_FRAME "%s", b);
}

/* Lvl is the only right-aligned column. */
static void roster_row(struct descriptor_data *d, char *c0, char *c1,
                       char *c2, char *c3, char *c4, char *tag, int head)
{
   char b[MAX_STRING_LENGTH];

   /* The character name is the thing being chosen, so it carries ACCT_CHAR
    * and everything after it falls back to ACCT_ROW. `head` paints the
    * whole line instead, for the column headings. */
   snprintf(b, sizeof(b),
            "%s  | %s%-*.*s%s | %-*.*s | %-*.*s | %*.*s | %-*.*s |%s%s\r\n",
            head ? ACCT_HEAD : ACCT_ROW,
            head ? "" : ACCT_CHAR,
            RC_NAME,  RC_NAME,  c0,
            head ? "" : ACCT_ROW,
            RC_RACE,  RC_RACE,  c1,
            RC_CLASS, RC_CLASS, c2,
            RC_LVL,   RC_LVL,   c3,
            RC_NEXT,  RC_NEXT,  c4,
            (tag && *tag) ? "  " : "", (tag && *tag) ? tag : "");
   SEND_TO_Q(d, "%s", b);
}

static void roster_optline(struct descriptor_data *d, char *k1, char *l1,
                           char *k2, char *l2, char *k3, char *l3, int lit)
{
   char a[64], b2[64], b[MAX_STRING_LENGTH];

   snprintf(a,  sizeof(a),  "%4s) %s", k1, l1);
   snprintf(b2, sizeof(b2), "%4s) %s", k2, l2);
   snprintf(b, sizeof(b), "%s%-*s%s%-*s%4s) %s\r\n",
            lit ? ACCT_LIT : ACCT_DIM, RM_CELL, a,
            lit ? ACCT_DIM : "", RM_CELL, b2, k3, l3);
   SEND_TO_Q(d, "%s", b);
}

/*
 * Draw the account's characters.
 *
 * Renders from char_file_u via load_char, so drawing the screen never
 * instantiates a character -- a roster of twenty costs twenty reads and no
 * world state. A tombstoned member is filtered out here rather than at each
 * caller: its pfile still loads, so a screen that listed it would offer a row
 * that enters the game as a deleted character.
 *
 * BYTE-MATCHED TO THE TS renderAccountMenu, including the scroll art, the
 * 15-wide account field, all five columns and the 3x3 option block. The two
 * engines draw the same screen because a player crosses between them.
 */
void account_send_roster(struct descriptor_data *d)
{
   struct char_file_u f;
   char nm[128], lv[16], nx[64], tag[32], span[16];
   int j, shown = 0, total = 0;

   if (!d || !d->account)
      return;

   /* Count first: the Play span reads "1-N" and N is the DRAWN rows, which
    * skip tombstones -- so it cannot be num_members. */
   for (j = 0; j < d->account->num_members; j++)
      if (load_char(d->account->members[j], &f) >= 0
          && !IS_SET(f.char_specials_saved.act, PLR_DELETED))
         total++;

   SEND_TO_Q(d, ACCT_FRAME "\r\n");
   SEND_TO_Q(d, "        ____________________________ \r\n");
   SEND_TO_Q(d, "       ()                           )\r\n");
   SEND_TO_Q(d, "        |    Welcome, traveller!    |\r\n");
   SEND_TO_Q(d, "        |                           |\r\n");
   SEND_TO_Q(d, "        |  Account: " ACCT_NAME "%-*.*s" ACCT_FRAME " |\r\n",
             ACCT_FIELD, ACCT_FIELD, d->account->name);
   SEND_TO_Q(d, "        |                           |\r\n");
   SEND_TO_Q(d, "        |                       /;  /\r\n");
   SEND_TO_Q(d, "         \\  /^\\/^\\/' \\ /\\/^\\/^\\/ \\/^ \r\n");
   SEND_TO_Q(d, "          ;/          \\;             \r\n");
   SEND_TO_Q(d, "\r\n");

   roster_rule(d);
   roster_row(d, "Character", "Race", "Class", "Lvl", "Next level", "", TRUE);
   roster_rule(d);

   for (j = 0; j < d->account->num_members; j++) {
      if (load_char(d->account->members[j], &f) < 0)
         continue;
      if (IS_SET(f.char_specials_saved.act, PLR_DELETED))
         continue;
      shown++;
      snprintf(nm, sizeof(nm), "%d) %s", shown, f.name);
      snprintf(lv, sizeof(lv), "%d", f.level);

      /* "Next level" is a RESETTING pool in this game: levelling subtracts
       * the cost (guild.c:721), so the bar is exp over the cost of the level
       * being stood on, never the gap between two table rows -- that draws a
       * bar which is always nearly full. No next level renders "--". */
      {
         long need = GET_EXP_FOR_LEVEL(f.race, f.class,
                       f.level, f.player_specials_saved.times_remorted);
         long have = f.points.exp;
         if (f.level >= LVL_HERO - 1 || need <= 0)
            strcpy(nx, "--");
         else {
            int pct = (have <= 0) ? 0 : (have >= need) ? 100 : (int) ((have * 100) / need);
            char bar[16];
            int b;
            for (b = 0; b < 10; b++)
               bar[b] = (b < pct / 10) ? '#' : '-';
            bar[10] = '\0';
            snprintf(nx, sizeof(nx), "%s %3d%%", bar, pct);
         }
      }

      tag[0] = '\0';
      if (!str_cmp(f.name, d->account->name))
         strcpy(tag, "main");
      roster_row(d, nm,
                 pc_race_types[(int) f.race] ? pc_race_types[(int) f.race] : "?",
                 pc_class_types[(int) f.class] ? pc_class_types[(int) f.class] : "?",
                 lv, nx, tag, FALSE);
   }
   if (!shown)
      roster_row(d, "(none yet)", "", "", "", "", "", FALSE);
   roster_rule(d);
   SEND_TO_Q(d, "\r\n");

   if (total > 1)
      snprintf(span, sizeof(span), "1-%d", total);
   else
      strcpy(span, "1");

   roster_optline(d, span, "Play",       "N", "New character",  "P", "Account password", TRUE);
   roster_optline(d, "D",  "Delete",     "X", "Description",    "E", "E-mail address", FALSE);
   roster_optline(d, "B",  "Background", "W", "Who is playing", "Q", "Leave PhoenixMUD", FALSE);
   SEND_TO_Q(d, ACCT_LIT "\r\n    MAKE YOUR CHOICE (%s): " ACCT_RESET, span);
}

/*
 * Write one password across the whole account.
 *
 * The account holds NO credential: the pfiles do, one copy each. So a change
 * has to walk the roster and keep them equal, or a login on a member whose
 * copy was missed verifies against a password the player no longer thinks
 * they have -- and there is no second authentication path to fix it from.
 *
 * The character is written through save_char_ascii on its char_file_u, which
 * is the same load-modify-save an offline pfile edit uses elsewhere
 * (act.wizard.c:7890). A member who is IN THE WORLD holds an authoritative
 * in-memory copy, so its live record is updated too rather than writing a
 * disk copy underneath it.
 */
void account_set_password(struct account_data *acct, char *raw)
{
   struct char_file_u f;
   struct descriptor_data *d;
   int j;

   if (!acct || !raw || !*raw)
      return;

   for (j = 0; j < acct->num_members; j++) {
      if (load_char(acct->members[j], &f) < 0) {
         log("SYSERR: account %s member %s has no readable record; its password "
             "was NOT changed and that member now logs in on the old one.",
             acct->name, acct->members[j]);
         continue;
      }
      strncpy(f.pwd, CRYPT(raw, f.name), MAX_PWD_LENGTH);
      f.pwd[MAX_PWD_LENGTH] = '\0';
      save_char_ascii(&f);

      for (d = descriptor_list; d; d = d->next)
         if (d->character && !IS_NPC(d->character)
             && !str_cmp(GET_PC_NAME(d->character), acct->members[j])) {
            strncpy(GET_PASSWD(d->character), f.pwd, MAX_PWD_LENGTH);
            GET_PASSWD(d->character)[MAX_PWD_LENGTH] = '\0';
         }
   }
}

/* ------------------------------------------------------------------ */
/*  Account money at the persistence boundary                          */
/* ------------------------------------------------------------------ */

/*
 * ONE RECORD HOLDS THE MONEY; every other member stores 0.
 *
 * WHY THE BOUNDARY AND NOT THE MACRO. GET_GOLD and GET_BANK_GOLD are
 * LVALUES ("+= cost") at 38+ sites, and they are applied to BOTH struct
 * char_data and struct char_file_u (act.other2.c:366). The obvious port of
 * the TypeScript accessor -- a pointer on char_data that the macro
 * dereferences -- founders on something this codebase does idiomatically:
 * it copies char_data wholesale. "*mob = mob_proto[i]" (db.c:2979) and
 * "memcpy(&tmpmob, m, sizeof(*m))" (dg_mobcmd.c:1365) would both leave the
 * copy's pointer aimed at the ORIGINAL's field, so a spawned mob would
 * spend its prototype's purse, and every future struct copy would be the
 * same trap, silently.
 *
 * So the money is scoped where it is WRITTEN and READ from disk:
 *
 *   - loading a non-holder seeds it from the HOLDER's record, so every
 *     member sees one balance at login;
 *   - saving a non-holder writes 0 and pushes the value to the holder, so
 *     nothing -- not a restore, not the interchange, which turns every
 *     member record into a TS "gold" field -- can turn one balance into
 *     several copies of it.
 *
 * WHAT THIS DOES NOT GIVE: two siblings in the world AT THE SAME TIME still
 * hold separate in-memory copies, so both can spend the same coins until
 * one saves. That is the dupe the TypeScript purse closes with a resident
 * balance, and closing it here needs the macro change above.
 *
 * NOTE: keep this file ASCII. A non-ASCII byte written through a latin-1
 * encoder truncates it to zero length and the build then fails with a
 * misleading link error.
 */

/* The 105-126 band overlaps nothing -- see account.h. */
static int acct_member_shares(struct char_file_u *f)
{
   return account_shares_property(f->level);
}

/* Seed a freshly loaded record from the account's holder. */
void account_money_load(struct char_file_u *f)
{
   struct account_data *acct;
   struct char_file_u h;

   if (!f || !*f->name)
      return;
   if (!(acct = account_of_char(f->name)))
      return;
   if (!str_cmp(acct->name, f->name))
      return;                           /* this IS the holder */
   if (!acct_member_shares(f))
      return;
   if (load_char(acct->name, &h) < 0) {
      /* An unreadable holder is NOT zero: zero reads as "you are broke" and
       * the next save would write that over the real balance. Leave the
       * record's own figures alone and say so. */
      log("SYSERR: account %s holder record unreadable; %s keeps its own "
          "money this session.", acct->name, f->name);
      return;
   }
   f->points.gold[0] = h.points.gold[0];
   f->points.bank_gold[0] = h.points.bank_gold[0];
}

/* Push a member's money to the holder and zero its own copy. */
void account_money_save(struct char_file_u *f)
{
   struct account_data *acct;
   struct char_file_u h;
   struct descriptor_data *d;

   if (!f || !*f->name)
      return;
   if (!(acct = account_of_char(f->name)))
      return;
   if (!str_cmp(acct->name, f->name))
      return;                           /* the holder keeps it */
   if (!acct_member_shares(f))
      return;

   /* A live holder owns its record: writing a disk-loaded copy underneath
    * it would roll back whatever it has not saved yet. It reads the same
    * balance at its own next load, so there is nothing to fix. */
   for (d = descriptor_list; d; d = d->next)
      if (d->character && !IS_NPC(d->character)
          && !str_cmp(GET_PC_NAME(d->character), acct->name))
         return;

   if (load_char(acct->name, &h) < 0) {
      log("SYSERR: account %s holder record unreadable; %s keeps %ld gold on "
          "its own record.", acct->name, f->name, f->points.gold[0]);
      return;
   }

   /*
    * A PUSH IS ONLY SAFE IF THIS MEMBER WAS SEEDED.
    *
    * account_money_load runs at every load, so normally the member's
    * figures ARE the account's and pushing them back is a no-op or a
    * legitimate spend. The exception is a character ALREADY in the world
    * when it joined the roster: it still carries its own purse, never
    * having been seeded, and pushing that would overwrite the account's
    * balance with a stranger's -- destroying money rather than moving it.
    *
    * A seeded member reads 0 (its previous save zeroed it) or matches the
    * holder. One still holding pre-join money matches neither, so refuse
    * and leave both records for a god to reconcile.
    */
   if (f->points.gold[0] != 0 && h.points.gold[0] != 0
       && f->points.gold[0] != h.points.gold[0]
       && f->points.bank_gold[0] != h.points.bank_gold[0]) {
      log("SYSERR: %s carries %ld gold that did not come from account %s "
          "(holder has %ld) -- probably joined to the roster while in the "
          "world. NOT pushed; reconcile by hand.",
          f->name, f->points.gold[0], acct->name, h.points.gold[0]);
      return;
   }

   h.points.gold[0] = f->points.gold[0];
   h.points.bank_gold[0] = f->points.bank_gold[0];
   save_char_ascii(&h);

   f->points.gold[0] = 0;
   f->points.bank_gold[0] = 0;
}

/* ------------------------------------------------------------------ */
/*  The live shared balance                                            */
/* ------------------------------------------------------------------ */

/*
 * TWO SIBLINGS IN THE WORLD AT ONCE.
 *
 * The boundary scoping above gives every member one balance at LOGIN and
 * one writer on disk. It does not stop two of one player's characters, both
 * in the world, from each holding a copy and both spending it -- they only
 * reconcile when one saves, and the later save wins.
 *
 * So a sharing member does not keep its money in its own char_data at all.
 * It reads and writes the ACCOUNT's slot, which is one number however many
 * siblings are logged in, and a check-and-spend on a single-threaded game
 * loop is therefore indivisible without any locking.
 *
 * The pointer lives on the ACCOUNT, not the character, for a reason given
 * at acct_gold_ref in account.h: char_data gets copied wholesale here.
 *
 * NOTE: keep this file ASCII.
 */

/* Seed the account slot from the holder's record, ONCE. A later member
 * entering must join the balance already in play, not overwrite it with
 * whatever its own (zeroed) record happens to carry -- that overwrite is the
 * most obvious way to lose an account's savings, so it is refused here
 * rather than guarded at each caller. */
static void acct_money_seed(struct account_data *acct)
{
   struct char_file_u h;

   if (!acct || acct->money_live)
      return;
   if (load_char(acct->name, &h) < 0) {
      /* Unreadable is NOT zero. Leave the slot dead so every member keeps
       * banking to its own record this session. */
      log("SYSERR: account %s holder record unreadable; members bank "
          "per-character this session.", acct->name);
      return;
   }
   acct->gold_slot = h.points.gold[0];
   acct->bank_slot = h.points.bank_gold[0];
   acct->money_live = TRUE;
}

long *acct_gold_ref(struct char_data *ch)
{
   if (!ch)
      return NULL;
   if (!ch->money_acct || !ch->money_acct->money_live)
      return &ch->points.gold[0];
   return &ch->money_acct->gold_slot;
}

long *acct_bank_ref(struct char_data *ch)
{
   if (!ch)
      return NULL;
   if (!ch->money_acct || !ch->money_acct->money_live)
      return &ch->points.bank_gold[0];
   return &ch->money_acct->bank_slot;
}

/*
 * Attach a character entering the world to its account's balance.
 *
 * Called once the character is settled, never for a mob: an NPC has no
 * roster and must keep its own purse, which is what the NULL default gives
 * every copy of a prototype.
 */
void account_money_bind(struct char_data *ch)
{
   struct account_data *acct;

   if (!ch || IS_NPC(ch))
      return;
   ch->money_acct = NULL;
   if (!(acct = account_of_char(GET_PC_NAME(ch))))
      return;
   /* The 105-126 band overlaps nothing -- see account.h. */
   if (!account_shares_property(GET_LEVEL(ch)))
      return;
   acct_money_seed(acct);
   if (!acct->money_live)
      return;
   ch->money_acct = acct;
}

/*
 * Detach a character leaving the world, and drop the slot when the last
 * member goes.
 *
 * The slot must not outlive the last member: the next member to enter would
 * join a balance nobody has reconciled with the record, and a restore or an
 * interchange import in between would be invisible to it.
 */
void account_money_unbind(struct char_data *ch)
{
   struct account_data *acct;
   struct descriptor_data *d;

   if (!ch || IS_NPC(ch) || !(acct = ch->money_acct))
      return;
   ch->money_acct = NULL;

   for (d = descriptor_list; d; d = d->next)
      if (d->character && d->character != ch && !IS_NPC(d->character)
          && d->character->money_acct == acct)
         return;                        /* somebody is still spending it */
   acct->money_live = FALSE;
}
