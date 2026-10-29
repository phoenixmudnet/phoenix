/***********************************************************************
*  File: hero_state_test.c                                             *
*  Usage: standalone checks for the class weapon's rent lines          *
*                                                                      *
*  Build and run with `make test` from the repository root.  Links     *
*  only hero_state.o.                                                  *
***********************************************************************/

#include "../localHeader/conf.h"
#include "../localHeader/sysdep.h"
#include "../src/structs.h"
#include "../src/utils.h"
#include "../src/hero_state.h"

static int failures = 0;
static int checks = 0;

#define CHECK(cond) check((cond), #cond, __LINE__)

static void check(int cond, const char *what, int line)
   {
   checks++;
   if (!cond)
      {
      failures++;
      printf("FAIL %s:%d  %s\n", __FILE__, line, what);
      }
   }

/* the H lines of src, read back line by line into dst as a load does */
static void round_trip(struct obj_data *src, struct obj_data *dst)
   {
   char line[256];
   FILE *fp = tmpfile();

   CHECK(obj_hero_write(fp, src) == 0);
   rewind(fp);
   while (fgets(line, sizeof(line), fp))
      {
      line[strcspn(line, "\r\n")] = '\0';
      CHECK(strlen(line) < 128);
      CHECK(obj_hero_read_line(dst, line) == TRUE);
      }
   fclose(fp);
   }

static void test_round_trip(void)
   {
   struct obj_data a, b;
   struct hero_data *h;

   memset(&a, 0, sizeof(a));
   memset(&b, 0, sizeof(b));
   a.bound_idnum = 130;
   strcpy(a.bound_name, "Ragnar");
   a.attune = attune_create();
   a.attune->points = 2274.5;
   a.attune->hour = 11987949;
   a.attune->hour_points = 52;
   a.hero = h = hero_create();
   h->cls = 14; h->core = 26614; h->level = 5; h->essences = 1; h->cleared = 1;
   h->waiting = 13718; h->form = 850; h->focus = HERO_FOCUS_DPS;
   h->focus_points[HERO_FOCUS_DPS] = 2236; h->focus_points[HERO_FOCUS_HEAL] = 12.25;
   h->forms[0].vnum = 850; h->forms[0].charge = 3588; h->forms[0].refits = 2;
   h->forms[1].vnum = 3025; h->forms[1].charge = 0; h->forms[1].refits = HERO_REFITS_UNSET;
   h->num_forms = 2;
   h->holds[0] = 3; h->num_holds = 1;
   strcpy(h->grants[0], "ragnar-level-5-2026-10-12"); h->num_grants = 1;

   /* the exact text: the TS writer (heroLines.ts) is pinned to the same */
   {
   static const char *want =
      "HB 130 Ragnar\n"
      "HT 2274.5000 0 11987949 0 52\n"
      "HW 14 26614 5 1 1 13718 850 1\n"
      "HF 850 3588 2\n"
      "HF 3025 0 -1\n"
      "HH 3\n"
      "HP 2236.0000 12.2500 0.0000 0.0000\n"
      "HG ragnar-level-5-2026-10-12\n";
   char got[1024];
   size_t n;
   FILE *fp = tmpfile();

   CHECK(obj_hero_write(fp, &a) == 0);
   rewind(fp);
   n = fread(got, 1, sizeof(got) - 1, fp);
   got[n] = '\0';
   fclose(fp);
   CHECK(!strcmp(got, want));
   if (strcmp(got, want))
      printf("got:\n%s", got);
   }

   round_trip(&a, &b);
   CHECK(b.bound_idnum == 130);
   CHECK(!strcmp(b.bound_name, "Ragnar"));
   CHECK(b.attune && b.attune->points == 2274.5);
   CHECK(b.attune && b.attune->hour == 11987949 && b.attune->hour_points == 52);
   CHECK(b.hero != NULL);
   if (b.hero)
      {
      CHECK(b.hero->cls == 14 && b.hero->core == 26614 && b.hero->level == 5);
      CHECK(b.hero->essences == 1 && b.hero->cleared == 1);
      CHECK(b.hero->waiting == 13718 && b.hero->form == 850);
      CHECK(b.hero->focus == HERO_FOCUS_DPS);
      CHECK(b.hero->focus_points[HERO_FOCUS_DPS] == 2236);
      CHECK(b.hero->focus_points[HERO_FOCUS_HEAL] == 12.25);
      CHECK(b.hero->num_forms == 2);
      CHECK(b.hero->forms[0].vnum == 850 && b.hero->forms[0].charge == 3588 && b.hero->forms[0].refits == 2);
      CHECK(b.hero->forms[1].vnum == 3025 && b.hero->forms[1].refits == HERO_REFITS_UNSET);
      CHECK(b.hero->num_holds == 1 && b.hero->holds[0] == 3);
      CHECK(b.hero->num_grants == 1 && !strcmp(b.hero->grants[0], "ragnar-level-5-2026-10-12"));
      }
   obj_hero_free(&a);
   obj_hero_free(&b);
   CHECK(a.hero == NULL && a.attune == NULL && a.bound_idnum == 0);
   }

static void test_plain_and_essence(void)
   {
   struct obj_data a, b;
   char line[256];
   FILE *fp;

   /* a plain object writes no H line at all */
   memset(&a, 0, sizeof(a));
   fp = tmpfile();
   CHECK(obj_hero_write(fp, &a) == 0);
   CHECK(ftell(fp) == 0);
   fclose(fp);

   /* a boss's essence, bound */
   memset(&a, 0, sizeof(a));
   memset(&b, 0, sizeof(b));
   a.bound_idnum = 4606; strcpy(a.bound_name, "Adarra");
   a.checkpoint_of = 13718;
   round_trip(&a, &b);
   CHECK(b.bound_idnum == 4606 && b.checkpoint_of == 13718);
   CHECK(b.hero == NULL && b.attune == NULL);

   /* a line not ours, and an H line of a later version */
   CHECK(obj_hero_read_line(&b, "T 5") == FALSE);
   CHECK(obj_hero_read_line(&b, "#26614") == FALSE);
   strcpy(line, "HZ 1 2 3");
   CHECK(obj_hero_read_line(&b, line) == TRUE);
   CHECK(b.hero == NULL);
   }

static void test_copy(void)
   {
   struct obj_data a, b;

   memset(&a, 0, sizeof(a));
   memset(&b, 0, sizeof(b));
   a.hero = hero_create();
   a.hero->level = 7;
   a.attune = attune_create();
   a.attune->points = 9;
   a.bound_idnum = 1; strcpy(a.bound_name, "Vic");
   obj_hero_copy(&b, &a);
   CHECK(b.hero && b.hero != a.hero && b.hero->level == 7);
   CHECK(b.attune && b.attune != a.attune && b.attune->points == 9);
   CHECK(b.bound_idnum == 1 && !strcmp(b.bound_name, "Vic"));
   obj_hero_free(&a);
   CHECK(b.hero && b.hero->level == 7);   /* its own copy */
   obj_hero_free(&b);
   }

/* a form lent from a !rent piece carries when it was lent; an older
 * three-field HF line reads as a form held for good */
static void test_sitting_form(void)
   {
   struct obj_data a, b, c;
   char line[64];

   memset(&a, 0, sizeof(a));
   memset(&b, 0, sizeof(b));
   memset(&c, 0, sizeof(c));
   a.hero = hero_create();
   a.hero->num_forms = 2;
   a.hero->forms[0].vnum = 850; a.hero->forms[0].charge = 3588;
   a.hero->forms[0].refits = 2; a.hero->forms[0].sitting = 1792000000;
   a.hero->forms[1].vnum = 3025; a.hero->forms[1].refits = 0;
   round_trip(&a, &b);
   CHECK(b.hero && b.hero->num_forms == 2);
   CHECK(b.hero->forms[0].sitting == 1792000000);
   CHECK(b.hero->forms[1].sitting == 0);
   strcpy(line, "HF 850 3588 2");
   CHECK(obj_hero_read_line(&c, line) == TRUE);
   CHECK(c.hero && c.hero->forms[0].sitting == 0 && c.hero->forms[0].refits == 2);
   obj_hero_free(&a);
   obj_hero_free(&b);
   obj_hero_free(&c);
   }

/* growth banked for the next kill rides as a sixth HT value; a five-value
 * line reads as nothing banked (the TS writer, heroLines.ts, writes the same) */
static void test_banked(void)
   {
   struct obj_data a, b;
   char line[128];
   FILE *fp = tmpfile();

   memset(&a, 0, sizeof(a));
   memset(&b, 0, sizeof(b));
   a.attune = attune_create();
   a.attune->points = 2274.5;
   a.attune->hour = 11987949;
   a.attune->hour_points = 52;
   a.attune->banked = 140;
   CHECK(obj_hero_write(fp, &a) == 0);
   rewind(fp);
   CHECK(fgets(line, sizeof(line), fp) != NULL);
   CHECK(!strcmp(line, "HT 2274.5000 0 11987949 0 52 140.0000\n"));
   fclose(fp);
   round_trip(&a, &b);
   CHECK(b.attune && b.attune->banked == 140);
   obj_hero_free(&b);
   memset(&b, 0, sizeof(b));
   CHECK(obj_hero_read_line(&b, "HT 2274.5000 0 11987949 0 52") == TRUE);
   CHECK(b.attune && b.attune->banked == 0);
   obj_hero_free(&a);
   obj_hero_free(&b);
   }

/* class armor: the attunement a unit a line, an essence copy's count, and
 * the second focus; each survives the round trip and a copy */
static void test_class_armor(void)
   {
   struct obj_data a, b, c;
   FILE *fp;
   char line[256];

   memset(&a, 0, sizeof(a));
   memset(&b, 0, sizeof(b));
   memset(&c, 0, sizeof(c));
   a.armor = armor_create();
   a.armor->pts[0].stat = 0; a.armor->pts[0].essences = 100; a.armor->pts[0].over = 0;
   a.armor->pts[1].stat = 4; a.armor->pts[1].essences = 200; a.armor->pts[1].over = 1;
   a.armor->num = 2;
   a.armor->essences = 37;
   a.hero = hero_create();
   a.hero->focus = HERO_FOCUS_DPS;
   a.hero->focus2 = HERO_FOCUS_TANK;
   fp = tmpfile();
   CHECK(obj_hero_write(fp, &a) == 0);
   rewind(fp);
   CHECK(fgets(line, sizeof(line), fp) && !strcmp(line, "HA hp 100 0\n"));
   CHECK(fgets(line, sizeof(line), fp) && !strcmp(line, "HA hitroll 200 1\n"));
   CHECK(fgets(line, sizeof(line), fp) && !strcmp(line, "HE 37\n"));
   fclose(fp);
   round_trip(&a, &b);
   CHECK(b.armor && b.armor->num == 2 && b.armor->pts[1].stat == 4 && b.armor->pts[1].over == 1 &&
         b.armor->pts[1].essences == 200 && b.armor->essences == 37);
   CHECK(b.hero && b.hero->focus2 == HERO_FOCUS_TANK);
   obj_hero_copy(&c, &b);
   CHECK(c.armor && c.armor != b.armor && c.armor->num == 2);
   CHECK(obj_hero_read_line(&c, "HA wisdom 100 0") == TRUE && c.armor->num == 2);
   CHECK(armor_stat_of("spellsave") == 6 && armor_stat_of("x") == -1);
   obj_hero_free(&a);
   obj_hero_free(&b);
   obj_hero_free(&c);
   CHECK(!c.armor);
   }

int main(void)
   {
   test_class_armor();
   test_round_trip();
   test_banked();
   test_plain_and_essence();
   test_copy();
   test_sitting_form();
   printf("hero_state: %d checks, %d failures\n", checks, failures);
   return failures ? 1 : 0;
   }
