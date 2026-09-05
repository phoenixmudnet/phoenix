/* ************************************************************************
*   File: events.c                                                        *
*  Usage: dated event windows - the fern grant and the experience rate    *
*                                                                         *
*  An immortal schedules a window; the game asks whether one is live.     *
*  Nothing fires on a schedule: a window that opens while the game is     *
*  down is simply live when it comes back up.                             *
*                                                                         *
*  Storage is lib/etc/events, rewritten whole on every change. Times are  *
*  unix seconds because the file is read by two engines and a date string *
*  is a parsing argument waiting to happen.                               *
************************************************************************ */

#include "../localHeader/conf.h"
#include "../localHeader/sysdep.h"

#include "structs.h"
#include "buffer.h"
#include "utils.h"
#include "db.h"
#include "events.h"

struct event_window event_list[MAX_EVENTS];
int n_events = 0;

/*
 * The FIRST live window of a kind wins, in file order. Overlapping windows
 * are an operator error rather than a crash, and do_event lists them in the
 * same order so the winner is visible. Refusing overlap at write time would
 * mean reasoning about edits and time zones for no gain.
 */
struct event_window *event_live(int kind)
{
   int i;
   time_t now = time(0);

   for (i = 0; i < n_events; i++)
      if (event_list[i].kind == kind &&
          now >= event_list[i].start && now < event_list[i].end)
         return &event_list[i];

   return NULL;
}

/* 0 = no live window. That is why an XP window carries a PERCENTAGE and not
 * a delta: a live 100% window is a visible no-op, and 0 stays unambiguous. */
int event_value(int kind)
{
   struct event_window *e = event_live(kind);
   return e ? e->value : 0;
}

const char *event_name(int kind)
{
   struct event_window *e = event_live(kind);
   return e ? e->name : "";
}

const char *event_comment(int kind)
{
   struct event_window *e = event_live(kind);
   return e ? e->comment : "";
}

/*
 * An absent file is the normal state of a server with no events scheduled.
 * It must boot silently.
 */
void load_events(void)
{
   FILE *fp;
   char line[256];

   n_events = 0;

   if (!(fp = fopen(EVENTS_FILE, "r")))
      return;

   while (fgets(line, sizeof(line), fp)) {
      struct event_window *e;
      long st, en;
      char nm[64];
      int kind, val;

      if (*line == '*' || *line == '\n' || *line == '\r')
         continue;
      if (n_events >= MAX_EVENTS) {
         log("SYSERR: events file holds more than %d records; rest ignored.", MAX_EVENTS);
         break;
      }
      int used = 0;
      if (sscanf(line, "%63s %d %ld %ld %d%n", nm, &kind, &st, &en, &val, &used) != 5) {
         log("SYSERR: malformed line in %s: %s", EVENTS_FILE, line);
         continue;
      }
      e = &event_list[n_events++];
      strncpy(e->name, nm, sizeof(e->name) - 1);
      e->name[sizeof(e->name) - 1] = '\0';
      e->kind  = kind;
      e->start = (time_t) st;
      e->end   = (time_t) en;
      e->value = val;
      /* Sixth field to end of line: the comment, spaces and all. */
      {
         const char *c = line + used;
         while (*c == ' ' || *c == '\t') c++;
         strncpy(e->comment, c, sizeof(e->comment) - 1);
         e->comment[sizeof(e->comment) - 1] = '\0';
         {  char *nl = strpbrk(e->comment, "\r\n"); if (nl) *nl = '\0'; }
      }
   }
   fclose(fp);
   log("   %d event window(s) loaded.", n_events);
}

void save_events(void)
{
   FILE *fp;
   int i;

   if (!(fp = fopen(EVENTS_FILE, "w"))) {
      log("SYSERR: cannot write %s", EVENTS_FILE);
      return;
   }
   fprintf(fp, "* name kind start end value  -- see do_event\n");
   for (i = 0; i < n_events; i++)
      fprintf(fp, "%s %d %ld %ld %d %s\n",
              event_list[i].name, event_list[i].kind,
              (long) event_list[i].start, (long) event_list[i].end,
              event_list[i].value, event_list[i].comment);
   fclose(fp);
}
