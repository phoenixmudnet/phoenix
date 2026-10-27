#ifndef GREMORT_EXAM_H
#define GREMORT_EXAM_H

#define GREMORT_EXAM_HERO 0
#define GREMORT_EXAM_ANGEL 1
#define GREMORT_EXAM_AVATAR 2

/* When you get thrown out of the remort testing for any reason, send you here. */
/* Default to Aethelfyrd (because the parts are attached below Cymynedd for now. */
#define GREMORT_SEND_HOME_ROOM_VNUM 1005

#define GREMORT_EXAM_PART1_ROOM_VNUM 2997
#define GREMORT_EXAM_PART2_ROOM_VNUM 2998
#define GREMORT_EXAM_PART3_ROOM_VNUM 2999

#define GREMORT_QUEST_BASE_ROOM_VNUM_BUILDERS 67000
#define GREMORT_QUEST_BASE_ROOM_VNUM_PLAYERS  26600

#define GREMORT_EXAM_PART2_ROOM_DESC \
"   This is where trivia for the Hero, Angel and Avatar tests is\r\n" \
"administered.  Good luck!\r\n"

#define GREMORT_RESULT_PASSED_REQUIREMENTS 0
#define GREMORT_RESULT_FAILED_TRIVIA 1
#define GREMORT_RESULT_PASSED_TRIVIA 2
#define GREMORT_RESULT_TAKEN_QUEST 3
#define GREMORT_RESULT_PASSED_QUEST 4

#define GREMORT_SKILL_REQUIREMENTS_FILE "etc/gremort_skill_requirements.txt"
#define GREMORT_EXAM_RECORDS_FILE "etc/gremort_exam_records.txt"
#define GREMORT_TRIVIA_QUESTIONS_FILE "etc/gremort_trivia_questions.txt"

#define GREMORT_HERO_EXPLORED   ((int)(0.25f*top_of_world))
#define GREMORT_ANGEL_EXPLORED  ((int)(0.375f*top_of_world))
#define GREMORT_AVATAR_EXPLORED ((int)(0.50f*top_of_world))

#define GREMORT_HERO_TRIVIA_DIFFICULTY   20
#define GREMORT_ANGEL_TRIVIA_DIFFICULTY  25
#define GREMORT_AVATAR_TRIVIA_DIFFICULTY 30

#define GREMORT_HERO_GOLD_COST    500000
#define GREMORT_ANGEL_GOLD_COST   750000
#define GREMORT_AVATAR_GOLD_COST 1000000

extern const char *gremort_exam_types[3];
extern const char *gremort_exam_results[5];

typedef struct gremort_exam_record {
  long player_id;
  char player_name[MAX_INPUT_LENGTH];
  int exam_type; /* 0=hero, 1=angel, 2=avatar. */
  int result; /* 0=requirements met, 1=failed trivia, 2=passed trivia, 3=failed quest, 4=passed quest and gremorted. */
  long date_taken;
  int questions_asked[10];
  int questions_failed[4];
} gremort_exam_record;

typedef struct gremort_trivia_question {
  char text[1024];
  int difficulty; /* 1=easy, 2=medium, 3=hard. */
  char answers[8][128];
  int num_answers; /* How many answers were written for this question. */
  int correct_answer; /* Which one is correct. */
} gremort_trivia_question;

typedef struct gremort_skill_requirement {
  int exam_type;
  int class;
  int skill;
  int proficiency;
} gremort_skill_requirement;

int gremort_exam_hides(struct char_data *ch, int exam_type, int skill);

#endif
