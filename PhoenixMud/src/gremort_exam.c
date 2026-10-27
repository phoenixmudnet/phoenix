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
#include "screen.h"
#include "house.h"
#include "ident.h"
#include <sys/stat.h>
#include "clan.h"
#include "constants.h"
#include "gremort_exam.h"

extern struct spell_info_type *spells;
extern const float class_exp_multipliers[];
extern const float race_exp_multipliers[];
extern const int exp_table[];
extern int port;
extern void prune_crlf(char *string);
extern struct queue_event *add_function_to_queue(time_t, struct char_data *, long, int num_variables, void (*)(), ...);
extern struct room_data *world;
extern int real_zone2(int target_zone);
extern struct zone_data *zone_table;

const char *gremort_exam_types[] = {"Hero", "Angel", "Avatar"};
const char *gremort_exam_results[] = {"Met requirements", "Failed trivia", "Passed trivia", "Taken quest", "Passed quest and gremorted"};

void gremort_record_quest_result(struct char_data *ch, int result);

static struct gremort_skill_requirement *skillReqs = NULL;
static int nSkillReqs = 0;

struct gremort_exam_record *examRecords = NULL;
int nExamRecords = 0;

static struct gremort_trivia_question *triviaQuestions = NULL;
static int nTriviaQuestions = 0;

static struct gremort_exam_record *get_latest_gremort_exam_record(struct char_data *ch)
{
  int i, latestIndex = -1;
  long latest = 0;
  for (i = 0; i < nExamRecords; i++) {
    if (examRecords[i].player_id != GET_ID(ch) || str_cmp(examRecords[i].player_name, GET_NAME(ch))) {
      continue;
    }
    if (latestIndex == -1 || latest < examRecords[i].date_taken) {
      latest = examRecords[i].date_taken;
      latestIndex = i;
      continue;
    }
  }
  return latestIndex == -1 ? NULL : &examRecords[latestIndex];
}

static int which_gremort_exam(struct char_data *ch)
{
  if (REMORT_LEVEL(ch) == NON_REMORT) {
    return GREMORT_EXAM_HERO;
  } else if (REMORT_LEVEL(ch) == SINGLE_REMORT) {
    return GREMORT_EXAM_ANGEL;
  } else if (REMORT_LEVEL(ch) == DOUBLE_REMORT) {
    return GREMORT_EXAM_AVATAR;
  } else {
    return -1;
  }
}

static void insert_gremort_exam_record(struct gremort_exam_record *record)
{
  examRecords = (struct gremort_exam_record *)realloc(examRecords, (++nExamRecords) * sizeof(struct gremort_exam_record));
  memcpy(&examRecords[nExamRecords-1], record, sizeof(struct gremort_exam_record));
}

int gremort_load_exam_records(void)
{
  FILE *fp = fopen(GREMORT_EXAM_RECORDS_FILE, "r");
  if (!fp) {
    log("SYSERR: Could not open %s for reading!", GREMORT_EXAM_RECORDS_FILE);
    return 0;
  }

  char *buf = get_buffer(MAX_STRING_LENGTH);
  int i = 0, j;

  while (TRUE) {
    struct gremort_exam_record record;
    if (!fscanf(fp, "%s\n", record.player_name) || !record.player_name[0] || feof(fp)) {
      break;
    }
    fscanf(fp, "%ld %d %d %ld\n", &record.player_id, &record.exam_type, &record.result, &record.date_taken);
    for (j = 0; j < 10; j++) {
      fscanf(fp, "%d ", &record.questions_asked[j]);
    }
    fscanf(fp, "\n");
    for (j = 0; j < 4; j++) {
      fscanf(fp, "%d ", &record.questions_failed[j]);
    }
    fscanf(fp, "\n");
    i++;
    insert_gremort_exam_record(&record);
  }
  log("GREMORT-EXAM: Loaded %d exam records.", i);

  release_buffer(buf);
  fclose(fp);
  return 0;
}

static void insert_skill_requirement(struct gremort_skill_requirement *req)
{
  skillReqs = (struct gremort_skill_requirement *)realloc(skillReqs, (++nSkillReqs) * sizeof(struct gremort_skill_requirement));
  memcpy(&skillReqs[nSkillReqs-1], req, sizeof(struct gremort_skill_requirement));
}

void gremort_store_exam_records(void)
{
  FILE *fp = fopen(GREMORT_EXAM_RECORDS_FILE, "w");
  if (!fp) {
    log("SYSERR: Could not open %s for writing!", GREMORT_EXAM_RECORDS_FILE);
    return;
  }

  int i, j;
  for (i = 0; i < nExamRecords; i++) {
    struct gremort_exam_record *record = &examRecords[i];
    fprintf(fp, "%s\n", record->player_name);
    fprintf(fp, "%ld %d %d %ld\n", record->player_id, record->exam_type, record->result, record->date_taken);
    for (j = 0; j < 10; j++) {
      fprintf(fp, "%d ", record->questions_asked[j]);
    }
    fprintf(fp, "\n");
    for (j = 0; j < 4; j++) {
      fprintf(fp, "%d ", record->questions_failed[j]);
    }
    fprintf(fp, "\n");
  }
  log("GREMORT-EXAM: Wrote %d exam records.", i);
  fclose(fp);
  return;
}

int gremort_load_skill_requirements(void)
{
  if (skillReqs) {
    free(skillReqs);
    skillReqs = NULL;
    nSkillReqs = 0;
  }

  FILE *fp = fopen(GREMORT_SKILL_REQUIREMENTS_FILE, "r");
  if (!fp) {
    log("SYSERR: Could not open %s for reading!", GREMORT_SKILL_REQUIREMENTS_FILE);
    return 1;
  }

  char *buf = get_buffer(MAX_STRING_LENGTH);
  int line_number = 0;
  while (TRUE) {
    if (!fgets(buf, MAX_STRING_LENGTH, fp)) {
      break;
    }
    line_number++;
    if (buf[0] == ' ' || buf[0] == '\r' || buf[0] == '\n' || buf[0] == '%' || buf[0] == '#') {
      continue;
    }
    struct gremort_skill_requirement req;
    char *class = strtok(buf, "|");
    char *exam_type = strtok(NULL, "|");
    char *skill = strtok(NULL, "|");
    char *proficiency = strtok(NULL, "|");
    if (!class || !exam_type || !skill || !proficiency) {
      log("SYSERR: Invalid gremort skills file %s at line #%d.  Ignoring.", GREMORT_SKILL_REQUIREMENTS_FILE, line_number);      
    }
    req.exam_type = atoi(exam_type);
    req.proficiency = atoi(proficiency);
    for (req.class = 0; req.class < NUM_CLASSES; req.class++) {
      if (starts_with(buf, npc_class_types[req.class])) {
	break;
      }
    }
    for (req.skill = 0; req.skill < MAX_SPELLS; req.skill++) {
      if (!str_cmp(spells[req.skill].spell_name, skill)) {
	break;
      }
    }
    if (req.exam_type < GREMORT_EXAM_HERO || req.exam_type > GREMORT_EXAM_AVATAR) {
      log("SYSERR: Invalid gremort exam type %d.", req.exam_type);
    } else if (spells[req.skill].is_spell == IS_SKILL && (req.proficiency < 25 || req.proficiency > 95)) {
      log("SYSERR: Invalid gremort skill proficiency type %d.", req.proficiency);
    } else if (spells[req.skill].is_spell == IS_SPELL && (req.proficiency < 0 || req.proficiency > 10)) {
      log("SYSERR: Invalid gremort spell proficiency type %d.", req.proficiency);
    } else if (req.class < 0 || req.class >= NUM_CLASSES) {
      log("SYSERR: Invalid gremort class type %d.", req.class);
    } else if (req.skill < 0 || req.skill > MAX_SPELLS) {
      log("SYSERR: Invalid gremort skill type %d.", req.skill);
    } else {
      /* log("GREMORT-EXAM: Loaded requirement class %s(%d) exam %s(%d) skill %s(%d) proficiency %d.",
	  npc_class_types[req.class], req.class,
	  gremort_exam_types[req.exam_type], req.exam_type,
	  spells[req.skill].spell_name, req.skill,
	  req.proficiency
	  );
      */
      insert_skill_requirement(&req);
    }
  }

  log("GREMORT-EXAM: Loaded %d skill/spell requirements.", nSkillReqs);
  release_buffer(buf);
  fclose(fp);
  return 0;
}

int gremort_examine_skills(struct char_data *ch, char *result)
{
  if (!skillReqs || nSkillReqs < 1) {
    log("SYSERR: gremort_examine_skills called but no skills are loaded!");
    sprintf(result, "There is a problem determining which skills and spells you should know.  Please seek immortal assistance.\r\n");
    return 0;
  }

  int exam_type = which_gremort_exam(ch);
  if (exam_type == -1) {
    log("SYSERR: gremort_examine_skills called with an unknown remort type %d!", REMORT_LEVEL(ch));
    sprintf(result, "Your unique nature is confusing.  Please seek immortal assistance.\r\n");
    return 0;
  }

  int i, nSkills = 0, nSkillsLeft = 0;
  for (i = 0; i < nSkillReqs; i++) {
    if (skillReqs[i].class == GET_CLASS(ch) && skillReqs[i].exam_type == exam_type) {
      nSkills++;
      if (GET_SKILL(ch, skillReqs[i].skill) < skillReqs[i].proficiency) {
	nSkillsLeft++;
      }
    }
  }
  if (nSkills < 5) {
    sprintf(result, "People of your class aren't well understood.  Please seek immortal assistance.\r\n");
    return 0;
  }

  int skills = 0;
  sprintf(result, "You must be more proficient at ");
  for (i = 0; i < nSkillReqs; i++) {
    if (skillReqs[i].class == GET_CLASS(ch) && skillReqs[i].exam_type == exam_type && GET_SKILL(ch, skillReqs[i].skill) < skillReqs[i].proficiency) {
      skills++;
      strcat(result, spells[skillReqs[i].skill].spell_name);
      if (skills < nSkillsLeft-1) {
	strcat(result, ", ");
      } else if (skills == nSkillsLeft-1) {
	strcat(result, " and ");
      }
    }
  }
  strcat(result, " before you can take the ");
  strcat(result, gremort_exam_types[exam_type]);
  strcat(result, " test.\r\n");

  if (skills > 0) {
    return 0;
  } else {
    result[0] = '\x0';
    return 1;
  }
}

/* What the hidehero and hideangel toggles leave out of a skill list
 * (guild.c): a row the exam_type test does not ask of this character's
 * class, or one already at the proficiency it asks. A class the test has
 * no rows for keeps its whole list, as gremort_examine_skills would not
 * know what to ask of it either. */
int gremort_exam_hides(struct char_data *ch, int exam_type, int skill)
{
  int i, rows = 0, bar = 0;

  for (i = 0; i < nSkillReqs; i++) {
    if (skillReqs[i].class != GET_CLASS(ch) || skillReqs[i].exam_type != exam_type)
      continue;
    rows++;
    if (skillReqs[i].skill == skill)
      bar = skillReqs[i].proficiency;
  }
  if (rows == 0)
    return 0;
  return (bar <= 0 || GET_SKILL(ch, skill) >= bar);
}

int pass_explore_requirement(struct char_data *ch)
{
  switch (which_gremort_exam(ch)) {
  case GREMORT_EXAM_HERO:
    return GET_EXPLORED(ch) >= GREMORT_HERO_EXPLORED;
  case GREMORT_EXAM_ANGEL:
    return GET_EXPLORED(ch) >= GREMORT_ANGEL_EXPLORED;
  case GREMORT_EXAM_AVATAR:
    return GET_EXPLORED(ch) >= GREMORT_AVATAR_EXPLORED;
  default:
    return 0;
  }
}

int get_gold_cost(struct char_data *ch)
{
  switch (which_gremort_exam(ch)) {
  case GREMORT_EXAM_HERO:
    return GREMORT_HERO_GOLD_COST;
  case GREMORT_EXAM_ANGEL:
    return GREMORT_ANGEL_GOLD_COST;
  case GREMORT_EXAM_AVATAR:
    return GREMORT_AVATAR_GOLD_COST;
  default:
    return GREMORT_AVATAR_GOLD_COST;
  }
}

void gremort_attributes_check_update(struct char_data *ch)
{
  char result[MAX_STRING_LENGTH];
  int examType = which_gremort_exam(ch);
  struct gremort_exam_record *lastExam = NULL;

  char_from_room(ch);
  char_to_room(ch, real_room(GREMORT_SEND_HOME_ROOM_VNUM));
  command_interpreter(ch, "look");

  if (!gremort_examine_skills(ch, result)) {
    send_to_char(ch, "%s", result);
  } else if ((lastExam = get_latest_gremort_exam_record(ch)) && lastExam->exam_type == examType) {
    send_to_char(ch, "Yes, you are STILL ready to begin the %s test.\r\n", gremort_exam_types[examType]);
  } else if (GET_LEVEL(ch) != LVL_HERO-1) {
    send_to_char(ch, "You are not the proper level.\r\n");
  } else if (GET_EXP_FOR_CH(ch) - GET_EXP(ch) > 0) {
    send_to_char(ch, "You need more experience points.\r\n");
  } else if (!pass_explore_requirement(ch)) {
    send_to_char(ch, "You need to explore the world more.\r\n");
  } else {
    struct gremort_exam_record record;
    memset(&record, 0, sizeof(struct gremort_exam_record));
    record.player_id = GET_ID(ch);
    strcpy(record.player_name, GET_NAME(ch));
    record.exam_type = examType;
    record.result = GREMORT_RESULT_PASSED_REQUIREMENTS;
    record.date_taken = time(NULL);
    insert_gremort_exam_record(&record);
    gremort_store_exam_records();
    send_info("[ INFO ] %s is ready for the %s test!\r\n", GET_NAME(ch), gremort_exam_types[examType]);
    send_to_char(ch, "Congratulations!\r\n");
  }
}

SPECIAL(gremort_attributes_check_proc)
{
  if (!ch || IS_NPC(ch) || GET_LEVEL(ch) >= LVL_IMMORT) {
    return FALSE;
  }
  GET_WAIT_STATE(ch) += 2 RL_SEC;
  add_function_to_queue(1, ch, 0, 1, gremort_attributes_check_update, ch);
  return FALSE;
}

static void insert_trivia_question(struct gremort_trivia_question *q)
{
  triviaQuestions = (struct gremort_trivia_question *)realloc(triviaQuestions, (++nTriviaQuestions) * sizeof(struct gremort_trivia_question));
  memcpy(&triviaQuestions[nTriviaQuestions-1], q, sizeof(struct gremort_trivia_question));
}

int gremort_load_trivia_questions(void)
{
  FILE *fp = fopen(GREMORT_TRIVIA_QUESTIONS_FILE, "r");
  if (!fp) {
    log("SYSERR: Could not open %s for reading!", GREMORT_TRIVIA_QUESTIONS_FILE);
    return 0;
  }

  char *buf = get_buffer(MAX_STRING_LENGTH);
  while (!feof(fp)) {
    struct gremort_trivia_question question;
    memset(&question, 0, sizeof(gremort_trivia_question));
    int first = 1;
    while (fgets(buf, MAX_STRING_LENGTH, fp) && !feof(fp)) {
      if (!strcmp(buf, "~\n")) {
	break;
      } else if (strlen(question.text) + strlen(buf) < 1020) {
	prune_crlf(buf);
	if (first) {
	  strcat(question.text, buf+2); /* Strip "Q ". */
	  first = 0;
	} else {
	  strcat(question.text, buf);
	}
	strcat(question.text, "\r\n");
      }
    }
    fscanf(fp, "%d\n", &question.difficulty);
    int i;
    for (i = 0; i < 8; i++) {
      if (!fgets(buf, MAX_STRING_LENGTH, fp) || feof(fp) || strlen(buf) < 2) {
	break;
      } else if (buf[0] == 'X') {
	question.correct_answer = i;
      } else if (buf[0] != 'A') {
	log("SYSERR: Invalid gremort trivia answer.");
      }
      prune_crlf(buf);
      strncpy(question.answers[i], buf+2, 124);
      question.answers[i][127] = '\x0';
      strcat(question.answers[i], "\r\n");      
    }
    question.num_answers = MIN(i, 8);
    if (i == 0) {
      log("SYSERR: Invalid gremort trivia question -- no answers.");
    }
    insert_trivia_question(&question);
  }
  log("GREMORT-EXAM: Loaded %d trivia questions.", nTriviaQuestions);

  release_buffer(buf);
  fclose(fp);
  return 0;
}

static void trivia_permute_answers(int answers[8], int num_answers)
{
  int i;
  for (i = 0; i < num_answers; i++) {
    answers[i] = i;
  }
  for (i = 0; i < 100 * num_answers; i++) {
    int i1 = number(0, num_answers-1);
    int i2 = number(0, num_answers-1);
    int t = answers[i1];
    answers[i1] = answers[i2];
    answers[i2] = t;
  }
}

static void trivia_choose_questions(int missed_questions[4], int questions[10], int total_difficulty)
{
  int i, j, total;
  int difficulty[10];

  memset(difficulty, 0, 10*sizeof(int));
  memset(questions, 0, 10*sizeof(int));

  while (TRUE) {
    total = 0;
    for (i = 0; i < 10; i++) {
      difficulty[i] = number(1,3);
      total += difficulty[i];
    }
    if (total_difficulty <= total+2 && total_difficulty >= total-2) {
      break;
    }
  }

  for (i = 0; i < 10; i++) {
    while (TRUE) {
      questions[i] = number(0, nTriviaQuestions-1);
      if (triviaQuestions[questions[i]].difficulty != difficulty[i]) {
	continue;
      }
      int found = 0;
      for (j = 0; j < i; j++) {
	if (questions[i] == questions[j]) {
	  found = 1;
	  break;
	}
      }
      if (!found) {
	break;
      }
    }
  }

  int skip = number(0, 3);
  for (i = 0; i < 4; i++) {
    if (i == skip || missed_questions[i] == -1) {
      continue;
    }
    int found = 0;
    for (j = 0; j < 10; j++) {
      if (missed_questions[i] == questions[j]) {
	found = 1;
	break;
      }
    }
    if (!found) {
      questions[number(0,9)] = missed_questions[i];
    }
  }
}

#define RESET_TRIVIA(ch) \
  if (ch) { \
    char_from_room(ch); \
    char_to_room(ch, real_room(GREMORT_SEND_HOME_ROOM_VNUM)); \
    command_interpreter(ch, "look"); \
  } \
  state = 0; \
  busy = 0; \
  room_rnum rnum = real_room(GREMORT_EXAM_PART2_ROOM_VNUM); \
  free(world[rnum].description); \
  world[rnum].description = strdup(GREMORT_EXAM_PART2_ROOM_DESC);

void gremort_exam_trivia_update(struct char_data *ch, int cmd, char *argument)
{
  static int state = 0;
  static int busy = 0;
  static int timer = 0;
  static char arg[MAX_STRING_LENGTH] = {'\x0'};
  static int questions[10];
  static int questions_asked = 0;
  static int questions_correct = 0;
  static int questions_failed = 0;
  static int answer_order[8];
  static int answered_correctly = 0;
  static int questions_missed[4] = {-1, -1, -1, -1};

  char buf[4096], buf2[1024];
  int i;

  log("GREMORT-EXAM: ch=%s, cmd=%3d, state=%2d, busy=%1d, timer=%3d, arg=%s, asked=%d, correct=%d, failed=%d", GET_NAME(ch), cmd, state, busy, timer, argument, questions_asked, questions_correct, questions_failed);

  if (!ch || IS_NPC(ch) || GET_LEVEL(ch) >= LVL_IMMORT) {
    return;
  }

  if (state == 0 && busy) {
    char_from_room(ch);
    char_to_room(ch, real_room(GREMORT_SEND_HOME_ROOM_VNUM));
    command_interpreter(ch, "look");
    send_to_char(ch, "Someone is already taking a test.  Please wait your turn.\r\n");
    return;
  }

  if (!argument || !*argument) {
    arg[0] = '\x0';
  } else {
    skip_spaces(&argument);
    strcpy(arg, argument);
  }

  if (state == 0) {
    memset(arg, 0, MAX_STRING_LENGTH);
    busy = 1;
    state++;
    add_function_to_queue(1, ch, 0, 3, gremort_exam_trivia_update, ch, 0, NULL);
  } else if (state == 1) {
    if (cmd > 0) {
      return;
    }
    /* How much time until they can take it again? */
    int examType = which_gremort_exam(ch);
    if (examType < 0) {
      RESET_TRIVIA(ch);
      send_to_char(ch, "Your unique nature is confusing.  Please seek immortal assistance.\r\n");
      return;
    }
    struct gremort_exam_record *record = get_latest_gremort_exam_record(ch);
    if (!record) {
      RESET_TRIVIA(ch);
      send_to_char(ch, "You haven't met all the requirements for the %s test yet.\r\n", gremort_exam_types[examType]);
      return;
    }
    if (record->exam_type < examType || record->result < GREMORT_RESULT_PASSED_REQUIREMENTS) {
      RESET_TRIVIA(ch);
      send_to_char(ch, "You haven't met all the requirements for the %s test yet.\r\n", gremort_exam_types[examType]);
      return;
    }
    if (record->exam_type == examType) {
      if (record->result >= GREMORT_RESULT_PASSED_TRIVIA) {
	RESET_TRIVIA(ch);
	send_to_char(ch, "You've already passed trivia for the %s test.\r\n", gremort_exam_types[examType]);
	return;
      }
      if (record->result == GREMORT_RESULT_FAILED_TRIVIA) {
	long diff = time(NULL) - record->date_taken;
	if (diff < 7*24*3600) {
	  diff = 7*24*3600 - diff;
	  int days = diff / (24*3600);
	  int hours = (diff / 3600) % 24;
	  int minutes = (diff / 60) % 60;
	  int seconds = diff % 60;
	  RESET_TRIVIA(ch);
	  send_to_char(ch, "You must wait %d day%s, %d hour%s, %d minute%s and %d second%s before you can retake trivia for the %s test.\r\n",
	    days, days!=1?"s":"",
	    hours, hours!=1?"s":"",
	    minutes, minutes!=1?"s":"",
	    seconds, seconds!=1?"s":"",
	    gremort_exam_types[examType]
	  );
	  send_to_char(ch, "Have you explored and answered the questions you missed last time?\r\n");
	  return;
	}
      }
    }
    if (GET_GOLD(ch) < get_gold_cost(ch)) {
      RESET_TRIVIA(ch);
      send_to_char(ch, "You need %d gold coins on hand to take the %s trivia test.\r\n", get_gold_cost(ch), gremort_exam_types[examType]);
      return;
    }
    send_to_char(ch, "If you are ready to take the trivia for the %s test,\r\n  &Wsay I am ready&n\r\nOtherwise,\r\n  &Wsay I am not ready&n\r\n", gremort_exam_types[examType]);
    timer = 60;
    state++;
    add_function_to_queue(1, ch, 0, 3, gremort_exam_trivia_update, ch, 0, NULL);
  } else if (state == 2) {
    if (cmd == 0) {
      timer--;
      if (timer <= 0) {
	RESET_TRIVIA(ch);
	send_to_char(ch, "Come back when you are ready.\r\n");
	return;
      }
      if (timer <= 30 && timer % 10 == 0) {
	send_to_char(ch, "You have %d seconds to respond.\r\n", timer);
      }
      add_function_to_queue(1, ch, 0, 3, gremort_exam_trivia_update, ch, 0, NULL);
    } else if (CMD_IS("say") || CMD_IS("'")) {
      if (!str_cmp(arg, "I am ready") || !str_cmp(arg, "I am ready.")) {
	state++;
      } else if (!str_cmp(arg, "I am not ready") || !str_cmp(arg, "I am not ready.")) {
	state = 990;
      }
    }
  } else if (state == 3) {
    if (cmd > 0) {
      return;
    }
    state++;
    add_function_to_queue(2, ch, 0, 3, gremort_exam_trivia_update, ch, 0, NULL);
  } else if (state == 4) {
    if (cmd > 0) {
      return;
    }
    if (GET_GOLD(ch) < get_gold_cost(ch)) {
      RESET_TRIVIA(ch);
      send_to_char(ch, "You need %d gold coins on hand to take the trivia test.\r\n", get_gold_cost(ch));
      return;
    }
    GET_GOLD(ch) -= get_gold_cost(ch);
    send_to_char(ch, "Good luck!\r\n");
    state++;
    add_function_to_queue(5, ch, 0, 3, gremort_exam_trivia_update, ch, 0, NULL);
  } else if (state == 5) {
    if (cmd > 0) {
      return;
    }
    int difficulty = 30;
    if (which_gremort_exam(ch) == GREMORT_EXAM_HERO) {
      difficulty = GREMORT_HERO_TRIVIA_DIFFICULTY;
    } else if (which_gremort_exam(ch) == GREMORT_EXAM_ANGEL) {
      difficulty = GREMORT_ANGEL_TRIVIA_DIFFICULTY;
    } else if (which_gremort_exam(ch) == GREMORT_EXAM_AVATAR) {
      difficulty = GREMORT_AVATAR_TRIVIA_DIFFICULTY;
    } else {
      log("SYSERR: Invalid which_gremort_exam for %s!", GET_NAME(ch));
      RESET_TRIVIA(ch);
      send_to_char(ch, "Your unique nature is confusing.  Please seek immortal assistance.\r\n");
      return;
    }
    questions_missed[0] = questions_missed[1] = -1;
    questions_missed[2] = questions_missed[3] = -1;
    questions_asked = 0;
    questions_correct = 0;
    questions_failed = 0;
    struct gremort_exam_record *record = get_latest_gremort_exam_record(ch);
    if (record && record->result == GREMORT_RESULT_FAILED_TRIVIA) {
      trivia_choose_questions(record->questions_failed, questions, difficulty);
      log("GREMORT-EXAM(%s): Failed previously: %3d %3d %3d %3d", GET_NAME(ch),
	  record->questions_failed[0], record->questions_failed[1],
	  record->questions_failed[2], record->questions_failed[3]
      );
    } else {
      log("GREMORT-EXAM(%s): No previous failure for this exam.", GET_NAME(ch));
      trivia_choose_questions(questions_missed, questions, difficulty);
    }
    buf[0] = '\x0';
    for (i = 0; i < 10; i++) {
      sprintf(buf2, "%3d ", questions[i]);
      strcat(buf, buf2);
    }
    log("GREMORT-EXAM(%s) Questions : %s", GET_NAME(ch), buf);
    buf[0] = '\x0';
    for (i = 0; i < 10; i++) {
      sprintf(buf2, "%3d ", triviaQuestions[questions[i]].difficulty);
      strcat(buf, buf2);
    }
    log("GREMORT-EXAM(%s) Difficulty: %s", GET_NAME(ch), buf);
    state++;
    add_function_to_queue(1, ch, 0, 3, gremort_exam_trivia_update, ch, 0, NULL);
  } else if (state == 6) {
    if (cmd > 0) {
      return;
    }
    gremort_trivia_question *question = &triviaQuestions[questions[questions_asked]];
    for (i = 0; i < question->num_answers; i++) {
      answer_order[i] = i;
    }
    trivia_permute_answers(answer_order, question->num_answers);
    sprintf(buf, "Question #%d\r\n\r\n%s\r\n", questions_asked+1, question->text);
    for (i = 0; i < question->num_answers; i++) {
      sprintf(buf2, "  %c) ", 'A'+i);
      strcat(buf, buf2);
      strcat(buf, question->answers[answer_order[i]]);
    }
    strcat(buf, "\r\n");
    send_to_char(ch, "%s", buf);
    room_rnum rnum = real_room(GREMORT_EXAM_PART2_ROOM_VNUM);
    free(world[rnum].description);
    world[rnum].description = strdup(buf);
    timer = 90;
    state++;
    add_function_to_queue(1, ch, 0, 3, gremort_exam_trivia_update, ch, 0, NULL);
  } else if (state == 7) {
    if (cmd == 0) {
      timer--;
      if (timer <= 0) {
	send_to_char(ch, "Time has expired.\r\n");
	answered_correctly = -1;
	state++;
      } else if (timer <= 30 && timer % 10 == 0) {
	send_to_char(ch, "%d seconds.\r\n", timer);
      } else if (timer == 5) {
	send_to_char(ch, "%d seconds.\r\n", timer);
      }
      add_function_to_queue(1, ch, 0, 3, gremort_exam_trivia_update, ch, 0, NULL);
    } else if (CMD_IS("say") || CMD_IS("'")) {
      if (!argument || !*argument) {
	return;
      }
      skip_spaces(&argument);
      if (strlen(arg) > 1) {
	return;
      }
      int answer = (int)(toupper(argument[0])- 'A');
      gremort_trivia_question *question = &triviaQuestions[questions[questions_asked]];
      if (answer < 0 || answer >= question->num_answers) {
	return;
      }
      answer = answer_order[answer];
      answered_correctly = (answer == question->correct_answer ? 1 : 0);
      state++;
    }
  } else if (state == 8) {
    if (cmd > 0) {
      return;
    }
    state++;
    add_function_to_queue(2, ch, 0, 3, gremort_exam_trivia_update, ch, 0, NULL);
  } else if (state == 9) {
    if (cmd > 0) {
      return;
    }
    if (answered_correctly > 0) {
      send_to_char(ch, "Correct!\r\n");
      questions_correct++;
    } else {
      if (answered_correctly != -1) {
	send_to_char(ch, "Sorry, that is not correct.\r\n");
      }
      questions_missed[questions_failed] = questions[questions_asked];
      questions_failed++;
    }
    questions_asked++;
    state++;
    add_function_to_queue(2, ch, 0, 3, gremort_exam_trivia_update, ch, 0, NULL);
  } else if (state == 10) {
    if (questions_correct >= 7 || questions_failed >= 4) {
      int examType = which_gremort_exam(ch);
      struct gremort_exam_record record;
      memset(&record, 0, sizeof(struct gremort_exam_record));
      record.player_id = GET_ID(ch);
      strcpy(record.player_name, GET_NAME(ch));
      record.exam_type = examType;
      record.result = questions_correct >= 7 ? GREMORT_RESULT_PASSED_TRIVIA : GREMORT_RESULT_FAILED_TRIVIA;
      record.date_taken = time(NULL);
      for (i = 0; i < questions_asked; i++) {
	record.questions_asked[i] = questions[i];
      }
      for (i = questions_asked; i < 10; i++) {
	record.questions_asked[i] = -1;
      }
      for (i = 0; i < 4; i++) {
	record.questions_failed[i] = questions_missed[i];
      }
      insert_gremort_exam_record(&record);
      gremort_store_exam_records();
      char_from_room(ch);
      char_to_room(ch, real_room(GREMORT_SEND_HOME_ROOM_VNUM));
      RESET_TRIVIA(ch);
      if (questions_correct >= 7) {
	GET_GOLD(ch) += get_gold_cost(ch);
	send_info("[ INFO ] %s has passed the trivia for the %s test!\r\n", GET_NAME(ch), gremort_exam_types[examType]);
	send_to_char(ch, "Congratulations!\r\n");
      } else {
	send_to_char(ch, "You have failed the %s trivia test.  Please explore the world to find answers to the questions you missed.  You can take trivia again one week from now.\r\n", gremort_exam_types[examType]);
      }
      return;
    }
    state = 6;
    add_function_to_queue(3, ch, 0, 3, gremort_exam_trivia_update, ch, 0, NULL);
  } else if (state == 990) {
    RESET_TRIVIA(ch);
    send_to_char(ch, "Come back when you are ready.\r\n");
    return;
  } else {
    RESET_TRIVIA(ch);
  }  
}

SPECIAL(gremort_exam_trivia_proc)
{
  gremort_exam_trivia_update(ch, cmd, argument);
  return 0;
}

#define RESET_QUEST(ch) \
  if (ch) { \
    char_from_room(ch); \
    char_to_room(ch, real_room(GREMORT_SEND_HOME_ROOM_VNUM)); \
    command_interpreter(ch, "look"); \
  } \
  state = 0; \
  busy = 0;

void gremort_exam_quest_update(struct char_data *ch, int cmd, char *argument)
{
  if (!ch || IS_NPC(ch) || GET_LEVEL(ch) >= LVL_IMMORT) {
    return;
  }

  static int state = 0;
  static int busy = 0;
  static int timer = 0;
  static char arg[MAX_STRING_LENGTH] = {'\x0'};

  log("GREMORT-EXAM QUEST: ch=%s, cmd=%3d, state=%2d, busy=%1d, timer=%3d, arg=%s", GET_NAME(ch), cmd, state, busy, timer, argument);

  if (state == 0 && busy) {
    char_from_room(ch);
    char_to_room(ch, real_room(GREMORT_SEND_HOME_ROOM_VNUM));
    command_interpreter(ch, "look");
    send_to_char(ch, "Someone is already taking a test.  Please wait your turn.\r\n");
    return;
  }

  if (!argument || !*argument) {
    arg[0] = '\x0';
  } else {
    skip_spaces(&argument);
    strcpy(arg, argument);
  }

  if (state == 0) {
    memset(arg, 0, MAX_STRING_LENGTH);
    busy = 1;
    state++;
    add_function_to_queue(1, ch, 0, 3, gremort_exam_quest_update, ch, 0, NULL);
  } else if (state == 1) {
    if (cmd > 0) {
      return;
    }
    /* How much time until they can take it again? */
    int examType = which_gremort_exam(ch);
    if (examType < 0) {
      RESET_QUEST(ch);
      send_to_char(ch, "Your unique nature is confusing.  Please seek immortal assistance.\r\n");
      return;
    }
    struct gremort_exam_record *record = get_latest_gremort_exam_record(ch);
    if (!record) {
      RESET_QUEST(ch);
      send_to_char(ch, "You haven't met all the requirements for the %s test yet.\r\n", gremort_exam_types[examType]);
      return;
    }
    if (record->exam_type < examType || record->result < GREMORT_RESULT_PASSED_TRIVIA) {
      RESET_QUEST(ch);
      send_to_char(ch, "You haven't met all the requirements for the %s test yet.\r\n", gremort_exam_types[examType]);
      return;
    }
    if (record->exam_type == examType) {
      if (record->result >= GREMORT_RESULT_PASSED_QUEST) {
	RESET_QUEST(ch);
	send_to_char(ch, "You've already passed the quest for the %s test.\r\n", gremort_exam_types[examType]);
	return;
      }
      if (record->result == GREMORT_RESULT_TAKEN_QUEST) {
	long diff = time(NULL) - record->date_taken;
	if (diff < 7*24*3600) {
	  diff = 7*24*3600 - diff;
	  int days = diff / (24*3600);
	  int hours = (diff / 3600) % 24;
	  int minutes = (diff / 60) % 60;
	  int seconds = diff % 60;
	  RESET_QUEST(ch);
	  send_to_char(ch, "You must wait %d day%s, %d hour%s, %d minute%s and %d second%s before you can retake the quest for the %s test.\r\n",
	    days, days!=1?"s":"",
	    hours, hours!=1?"s":"",
	    minutes, minutes!=1?"s":"",
	    seconds, seconds!=1?"s":"",
	    gremort_exam_types[examType]
	  );
	  return;
	}
      }
    }

    int rnum;
    if (port != 9000 ) {
      rnum = real_room(GREMORT_QUEST_BASE_ROOM_VNUM_PLAYERS);
    } else {
      rnum = real_room(GREMORT_QUEST_BASE_ROOM_VNUM_BUILDERS);
    }
    if (rnum < 0) {
      RESET_QUEST(ch);
      send_to_char(ch, "There is a problem with the quest area.  Please seem immortal assistance.\r\n");
      return;
    }
    int zone = world[rnum].zone;
    if (real_zone2(zone) && !ZONE_FLAGGED(zone, Z_IDLE)) {
      RESET_QUEST(ch);
      send_to_char(ch, "Someone else is taking a quest.  Please check back later.\r\n");
      return;
    }

    if (GET_GOLD(ch) < get_gold_cost(ch)) {
      RESET_QUEST(ch);
      send_to_char(ch, "You need %d gold coins on hand to take the %s quest.\r\n", get_gold_cost(ch), gremort_exam_types[examType]);
      return;
    }
    send_to_char(ch, "If you are ready to take the quest for the %s test,\r\n  &Wsay I am ready&n\r\nOtherwise,\r\n  &Wsay I am not ready&n\r\n", gremort_exam_types[examType]);
    timer = 60;
    state++;
    add_function_to_queue(1, ch, 0, 3, gremort_exam_quest_update, ch, 0, NULL);
  } else if (state == 2) {
    if (cmd == 0) {
      timer--;
      if (timer <= 0) {
	RESET_QUEST(ch);
	send_to_char(ch, "Come back when you are ready.\r\n");
	return;
      }
      if (timer <= 30 && timer % 10 == 0) {
	send_to_char(ch, "You have %d seconds to respond.\r\n", timer);
      }
      add_function_to_queue(1, ch, 0, 3, gremort_exam_quest_update, ch, 0, NULL);
    } else if (CMD_IS("say") || CMD_IS("'")) {
      if (!str_cmp(arg, "I am ready") || !str_cmp(arg, "I am ready.")) {
	state++;
      } else if (!str_cmp(arg, "I am not ready") || !str_cmp(arg, "I am not ready.")) {
	state = 990;
      }
    }
  } else if (state == 3) {
    if (cmd > 0) {
      return;
    }
    state++;
    add_function_to_queue(2, ch, 0, 3, gremort_exam_quest_update, ch, 0, NULL);
  } else if (state == 4) {
    if (cmd > 0) {
      return;
    }
    if (GET_GOLD(ch) < get_gold_cost(ch)) {
      RESET_QUEST(ch);
      send_to_char(ch, "You need %d gold coins on hand to take the quest.\r\n", get_gold_cost(ch));
      return;
    }
    GET_GOLD(ch) -= get_gold_cost(ch);
    send_to_char(ch, "Good luck!\r\n");
    state++;
    add_function_to_queue(4, ch, 0, 3, gremort_exam_quest_update, ch, 0, NULL);
  } else if (state == 5) {
    if (cmd > 0) {
      return;
    }
    gremort_record_quest_result(ch, GREMORT_RESULT_TAKEN_QUEST);
    char_from_room(ch);
    room_rnum targetRoom;
    if (port == 9000) {
      targetRoom = GREMORT_QUEST_BASE_ROOM_VNUM_BUILDERS;
    } else {
      targetRoom = GREMORT_QUEST_BASE_ROOM_VNUM_PLAYERS;
    }
    char_to_room(ch, real_room(targetRoom));
    command_interpreter(ch, "look");
    send_to_char(ch, "Good luck!\r\n");
    state = 0;
    busy = 0;
  } else if (state == 990) {
    RESET_QUEST(ch);
    send_to_char(ch, "Come back when you are ready.\r\n");
    return;
  } else {
    RESET_QUEST(ch);
  }
}

SPECIAL(gremort_exam_quest_proc)
{
  gremort_exam_quest_update(ch, cmd, argument);
  return 0;
}

void gremort_record_quest_result(struct char_data *ch, int result)
{
  struct gremort_exam_record record;
  memset(&record, 0, sizeof(struct gremort_exam_record));
  record.player_id = GET_ID(ch);
  strcpy(record.player_name, GET_NAME(ch));
  record.exam_type = which_gremort_exam(ch);
  record.result = result;
  record.date_taken = time(NULL);
  insert_gremort_exam_record(&record);
  gremort_store_exam_records();
}
