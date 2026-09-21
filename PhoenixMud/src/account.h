/* ************************************************************************
*   File: account.h                                    Part of PhoenixMUD *
*  Usage: one login, many characters -- the roster and what it owns       *
*                                                                         *
*  An account owns a NAME, a roster of character names, and an online     *
*  dial. It owns no character state: the pfile stays authoritative for    *
*  the character, so adding accounts cannot lose anything and deleting    *
*  etc/accounts leaves every pfile exactly as it was.                     *
*                                                                         *
*  It holds NO CREDENTIAL. The pfiles hold it, one copy each, and a       *
*  password change walks the roster to keep them equal -- so a login      *
*  verifies against whichever member name was typed and there is no       *
*  second authentication path to keep correct.                            *
*                                                                         *
*  The account is NAMED AFTER ITS MAIN, recomputed rather than fixed at   *
*  creation. See roster_outranks(). The bank lives on a fixed HOLDER,     *
*  which does not follow the name. See account_bank_holder().             *
************************************************************************ */

#ifndef _ACCOUNT_H_
#define _ACCOUNT_H_

/*
 * The roster screen's colours, as five NAMED ROLES:
 *
 *   FRAME  banner art and table rules -- structure, recedes
 *   HEAD   the column headings
 *   NAME   the account's own name: the one fact saying whose screen this is
 *   CHAR   the CHARACTER names -- the thing you are choosing between
 *   ROW    everything else in a row: race, class, level, progress
 *
 * LIT is the Play option and DIM the rest; those two do not change with the
 * scheme, because "what you almost certainly want" should not move when the
 * decoration does.
 *
 * WARNING: these are the legacy half of PALETTES in the TS roster.ts, and the
 * live scheme (ember) has to be set in BOTH. A player crosses between the two
 * engines and a screen that recolours reads as a broken one.
 */
#define ACCT_FRAME  "\x1B[0;34m"
#define ACCT_HEAD   "\x1B[1;36m"
#define ACCT_NAME   "\x1B[1;31m"
#define ACCT_CHAR   "\x1B[0;31m"
#define ACCT_ROW    "\x1B[0;36m"
#define ACCT_LIT    "\x1B[1;36m"
#define ACCT_DIM    "\x1B[0;36m"
#define ACCT_RESET  "\x1B[0m"

#define ACCOUNT_FILE      "etc/accounts"
#define MAX_ACCT_MEMBERS  32
#define MAX_ACCOUNTS      4096

/* Level bands. The band is a property of the CHARACTER, not of the account,
 * so one roster can hold all three at once.
 *
 * THE MIDDLE BAND IS A SECURITY BOUNDARY, not a convenience: an immortal
 * below LVL_IMPL sits on the roster for login only and overlaps NOTHING,
 * which is what stops immortal-granted gold reaching a mortal sibling.
 * Widening it re-opens that path. (Owner, 2026-09-18.) */
#define ACCT_BAND_MORTAL    0
#define ACCT_BAND_IMMORTAL  1
#define ACCT_BAND_IMPL      2

struct account_data {
   char name[MAX_NAME_LENGTH + 1];                        /* the main */
   /* The member whose record holds the bank. Empty = not yet chosen; the
    * first read takes the name, which is where the bank was before this
    * field existed. */
   char holder[MAX_NAME_LENGTH + 1];
   char members[MAX_ACCT_MEMBERS][MAX_NAME_LENGTH + 1];
   int  num_members;
   int  max_online;                                       /* 0 = use default */

   /* THE LIVE BALANCE. One number per account, which every member in the
    * world reads and writes, so two siblings cannot each spend the same
    * coins. Seeded from the holder's record the first time any member
    * enters, and written back through that record on save.
    *
    * money_live says the slot has been seeded: without it a fresh account
    * would read 0 and the first save would write that over the holder's
    * real balance. */
   long bank_slot;
   int  money_live;
};

extern struct account_data *account_list;
extern int top_of_accountt;
extern int account_default_max_online;                    /* 0 = NO LIMIT */

void boot_accounts(void);
void save_accounts(void);

struct account_data *find_account(char *name);
struct account_data *account_of_char(char *char_name);
struct account_data *account_create_for(char *char_name);
int  account_has_char(struct account_data *acct, char *char_name);
int  account_add_char(struct account_data *acct, char *char_name);
int  account_remove_char(struct account_data *acct, char *char_name);

int  account_band(int level);
int  account_shares_property(int level);
int  account_online_limit(struct account_data *acct);
int  account_online_count(struct account_data *acct);

void account_pick_main(struct account_data *acct);
void account_send_roster(struct descriptor_data *d);
void account_set_password(struct account_data *acct, char *raw);

/* Where a character's bank lives. Returns the account's holder, or the
 * character itself when it is on no roster, banks on its own, or its account
 * is not sharing a bank. ONE record holds the balance and every other member
 * stores 0 -- the interchange turns every member pfile into a TS `goldBank`
 * field, so a mirrored balance crosses as N copies of the money. */
const char *account_purse_holder(char *char_name);

/* The member whose record holds the account's bank, or NULL when no member
 * can hold it. Moves the bank to a new holder when the current one has left
 * the roster, been deleted, or entered the 105-126 band, but only while the
 * old holder is out of the world. */
char *account_bank_holder(struct account_data *acct);

/* Account money at the persistence boundary. See the long note in account.c
 * for why this is not the GET_GOLD macro. */
void account_money_load(struct char_file_u *f);
/* Write a character's record, and the holder's when the bank lives there.
 * `ch` is the live character, or NULL for a record loaded with load_char. */
void account_save_record(struct char_data *ch, struct char_file_u *st);
/* Add `delta` to an offline character's bank wherever it lives. Stores
 * max(0, result), puts the unclamped result in *after, and returns FALSE
 * when the record cannot be read. */
int  account_bank_adjust(char *char_name, long delta, long *after);

/* Where a live character's BANK BALANCE actually lives. Returns a pointer to the
 * account's shared slot when this character is a sharing member of one, and
 * to the character's own field otherwise -- which is every mob, every
 * character on no roster, and every immortal in the 105-126 band.
 *
 * A FUNCTION rather than a pointer stored on char_data, deliberately: this
 * codebase copies char_data wholesale (db.c:2979, dg_mobcmd.c:1365) and a
 * stored pointer would survive the copy aimed at the ORIGINAL's field. The
 * only thing char_data carries is the ACCOUNT pointer, which is NULL for
 * every mob, so an inherited copy resolves to the copy's own field. */
long *acct_bank_ref(struct char_data *ch);
void  account_money_bind(struct char_data *ch);
void  account_money_unbind(struct char_data *ch);

/* Exploration and the identify log are cumulative across the account: OR every
 * sibling's stored bitmap into this character's working one. The contribution
 * stays discrete -- see explored_own in player_special_data. */
void account_knowledge_merge(struct char_data *ch);

#endif /* _ACCOUNT_H_ */
