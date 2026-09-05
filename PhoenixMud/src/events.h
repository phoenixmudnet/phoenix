/* ************************************************************************
*   File: events.h                        Part of PhoenixMUD              *
*  Usage: dated event windows - see events.c                              *
************************************************************************ */

#define EVENTS_FILE   "etc/events"
#define MAX_EVENTS    64

#define EVENT_FERN    0   /* value = the cast level the trigger uses */
#define EVENT_XP      1   /* value = percent; 100 is no change       */
#define EVENT_GOLD    2   /* value = percent on mob gold drops; 100 no change */

#define EVENT_COMMENT_LEN 160

struct event_window {
   char   name[64];
   int    kind;
   time_t start;      /* inclusive */
   time_t end;        /* exclusive */
   int    value;
   /* One line for the MOTD while the window is live. Optional; empty = none.
    * The operator's own words -- players read it, so it is not built. */
   char   comment[EVENT_COMMENT_LEN];
};

extern struct event_window event_list[];
extern int n_events;

struct event_window *event_live(int kind);
int event_value(int kind);
const char *event_name(int kind);
/* The live window's MOTD line for a kind, or "" -- for the login banner. */
const char *event_comment(int kind);
void load_events(void);
void save_events(void);
