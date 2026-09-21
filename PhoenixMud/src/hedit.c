/************************************************************************
* hedit.c      Hedit version 2.0 for Oasis OLC                         *
* by Steve Wolfe - siv@cyberenet.net                                   *
 ************************************************************************/

#include "../localHeader/conf.h"
#include "../localHeader/sysdep.h"
#include "structs.h"
#include "comm.h"
#include "utils.h"
#include "db.h"
#include "boards.h"
#include "olc.h"

/* List each help entry saved, was used for debugging. */
#if 0
#define HEDIT_LIST     1
#endif

/*------------------------------------------------------------------------*/

/*
 * External data structures.
 */
extern struct help_index_element *help_table;
extern int top_of_helpt;
extern struct descriptor_data *descriptor_list;

/*------------------------------------------------------------------------*/

/*
 * Function Prototypes
 */
void hedit_disp_extradesc_menu(struct descriptor_data *d);
void hedit_disp_exit_menu(struct descriptor_data *d);
void hedit_disp_exit_flag_menu(struct descriptor_data *d);
void hedit_disp_flag_menu(struct descriptor_data *d);
void hedit_disp_sector_menu(struct descriptor_data *d);
void hedit_disp_menu(struct descriptor_data *d);
void hedit_parse(struct descriptor_data *d, char *arg);
void hedit_setup_new(struct descriptor_data *d, char *new_key);
void hedit_setup_existing(struct descriptor_data *d, int rnum);
void hedit_save_to_disk(void);
void hedit_save_internally(struct descriptor_data *d);
void free_help(struct help_index_element *help);
int isname(char *str, char *namelist);
int is_help(const char *str, const char *namelist);

/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*\
  Utils and exported functions.
\*------------------------------------------------------------------------*/

void hedit_setup_new(struct descriptor_data *d, char *new_key)
{
   CREATE(OLC_HELP(d), struct help_index_element, 1);

   OLC_HELP(d)->keywords = str_dup(new_key);
   OLC_HELP(d)->entry = str_dup("This is an unfinished help entry.\r\n");
   hedit_disp_menu(d);
   OLC_VAL(d) = 0;
}

/*------------------------------------------------------------------------*/

void hedit_setup_existing(struct descriptor_data *d, int rnum)
{
   struct help_index_element *help;

  /*
   * Build a copy of the help entry for editing.
   */
   CREATE(help, struct help_index_element, 1);

   *help = help_table[rnum];
  /*
   * Allocate space for all strings.
   */
   help->keywords = str_dup(help_table[rnum].keywords ?
			    help_table[rnum].keywords : "UNDEFINED");
   help->entry = str_dup(help_table[rnum].entry ?
			 help_table[rnum].entry : "undefined\r\n");

  /*
   * Attach copy of help entry to player's descriptor.
   */
   OLC_HELP(d) = help;
   OLC_VAL(d) = 0;
   hedit_disp_menu(d);
}

/*------------------------------------------------------------------------*/

void hedit_save_internally(struct descriptor_data *d)
{
   int i, rnum;
   struct help_index_element *new_help_table;
   rnum = OLC_ZNUM(d);
  /*
   * Help entry exists exists: free and replace it.
   */
   if (rnum >= 0)
      {
      int order = help_table[rnum].file_order;

      free_help(help_table + rnum);
      help_table[rnum].file_order = order;
      help_table[rnum].min_level = OLC_HELP(d)->min_level;
      help_table[rnum].entry = str_dup(OLC_HELP(d)->entry);
      help_table[rnum].keywords = str_dup(OLC_HELP(d)->keywords);
      }
   else
      {                     /* Entry doesn't exist, hafta add it. */
      CREATE(new_help_table, struct help_index_element, top_of_helpt + 2);
     /*
      * Insert new entry at the top - why not?
      */
      new_help_table[0].min_level = OLC_HELP(d)->min_level;
      new_help_table[0].entry = str_dup(OLC_HELP(d)->entry);
      new_help_table[0].keywords = str_dup(OLC_HELP(d)->keywords);
      /* Saved first, as the insert at the top of the table implies. */
      new_help_table[0].file_order = 0;
      for (i = 0; i <= top_of_helpt; i++)
         if (help_table[i].keywords && help_table[i].file_order <= new_help_table[0].file_order)
            new_help_table[0].file_order = help_table[i].file_order - 1;
      
     /*
      * Count through help table.
      */
      for (i = 0; i <= top_of_helpt; i++)
	 new_help_table[i + 1] = help_table[i];

     /*
      * Copy help table over to new one.
      */
      free(help_table);
      help_table = new_help_table;
      top_of_helpt++;
      }
   olc_add_to_save_list(-1, OLC_SAVE_HELP);
}

/*------------------------------------------------------------------------*/

/* Orders help_table indices by their position in the help file. */
static int help_file_order_cmp(const void *a, const void *b)
{
   int x = help_table[*(const int *) a].file_order;
   int y = help_table[*(const int *) b].file_order;

   return (x > y) - (x < y);
}

/*
 * Writes the help file in its load order, not the table's. load_help sorts
 * the table for lookup, and that sort does not return the same order when
 * run on its own output, so writing the table order would reorder the file
 * on every save and change which entry an ambiguous keyword finds after
 * the next boot.
 */
void hedit_save_to_disk(void)
{
   int i, n, *order;
   FILE *fp;
   struct help_index_element *help;
   char *buf = get_buffer(256);
   char *buf1 = get_buffer(32750);
   char *buf2 = get_buffer(256);
   char *help_keyword = get_buffer(512);

   sprintf(buf, "%s/%s.new", HLP_PREFIX, HLP_FILE);
   if (!(fp = fopen(buf, "w+")))
      {
      mudlogf(BRF, LVL_BUILDER, TRUE,"SYSERR: OLC: Cannot open help file!");
      release_buffer(help_keyword);
      release_buffer(buf2);
      release_buffer(buf1);
      release_buffer(buf);
      return;
      }

   CREATE(order, int, top_of_helpt + 1);
   for (i = 0; i <= top_of_helpt; i++)
      order[i] = i;
   qsort(order, top_of_helpt + 1, sizeof(int), help_file_order_cmp);

   for (n = 0; n <= top_of_helpt; n++)
      {
      i = order[n];
      help = (help_table + i);

#if defined(HEDIT_LIST)
      sprintf(buf1, "OLC: Saving help entry %d.", i);
      log(buf1);
#endif
      if(!help->keywords)
	 continue;
     /*
      * Remove the '\r\n' sequences from description.
      */
      strcpy(buf1, help->entry ? help->entry : "Empty");
      strcpy(help_keyword,help->keywords ? help->keywords : "UNDEFINED");
      if(strlen(buf1)>32750)
	 log("SYSERR: Problem with %s.  Entry length %d. We will crash soon.",
	     help_keyword,strlen(buf1));
      strip_string(buf1);
      strip_string(help_keyword);
     /*
      * Forget making a buffer, lets just write the thing now.
      */
      if(*(buf1 + (strlen(buf1) - 1)) !='\n')
	 {
	 strcat(buf1,"\n");
	 }
      while(*(buf1 + (strlen(buf1) - 2)) =='\n')
	 {
	 *(buf1 + (strlen(buf1) - 1)) = '\0';
	 }

      fprintf(fp, "%s\n%s#%d\n",
	      help_keyword, buf1,
	      help->min_level);
      }

   free(order);

  /*
   * Write final line and close.
   */
   fprintf(fp, "$~\n");
   fclose(fp);
   sprintf(buf2, "%s/%s", HLP_PREFIX, HLP_FILE);

  /*
   * We're fubar'd if we crash between the two lines below.
   */
   remove(buf2);
   rename(buf, buf2);

   olc_remove_from_save_list(-1, OLC_SAVE_HELP);
   release_buffer(help_keyword);
   release_buffer(buf2);
   release_buffer(buf1);
   release_buffer(buf);
}

/*------------------------------------------------------------------------*/

void free_help(struct help_index_element *help)
{

   if (help->keywords)
      free(help->keywords);
   help->keywords=NULL;
   if (help->entry)
      free(help->entry);
   help->entry=NULL;
   memset(help, 0, sizeof(struct help_index_element));

}

/**************************************************************************
 Menu functions 
 **************************************************************************/

/*
 * The main menu.
 */
void hedit_disp_menu(struct descriptor_data *d)
{
   struct help_index_element *help;

   get_char_cols(d->character);
   help = OLC_HELP(d);
#if defined(CLEAR_SCREEN)
   send_to_char(d->character, "[H[J"); 
#endif

   send_to_char(d->character, 
		"%s1%s) Keywords    : %s%s\r\n"
		"%s2%s) Entry       :\r\n%s%s"
		"%s3%s) Min Level   : %s%d\r\n"
		"%sQ%s) Quit\r\n"
		"Enter choice : ",
		
		grn, nrm, yel, help->keywords,
		grn, nrm, yel, help->entry,
		grn, nrm, cyn, help->min_level,
		grn, nrm
      );

   OLC_MODE(d) = HEDIT_MAIN_MENU;
}

/**************************************************************************
  The main loop
 **************************************************************************/

void hedit_parse(struct descriptor_data *d, char *arg)
{
   int vnumber;

   switch (OLC_MODE(d))
      {
       case HEDIT_CONFIRM_SAVESTRING:
	  switch (*arg) 
	     {
	      case 'y':
	      case 'Y':
		 hedit_save_internally(d);
		 mudlogf(CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(d->character)),
			 TRUE, "OLC: %s edits help for %s.",
			 GET_NAME(d->character), OLC_HELP(d)->keywords);
		/*
		 * Do NOT free strings! Just the help structure. 
		 */
		 cleanup_olc(d, CLEANUP_STRUCTS);
		 send_to_char(d->character, "Help entry saved to memory.\r\n");
		 break;
	      case 'n':
	      case 'N':
		/*
		 * Free everything up, including strings, etc.
		 */
		 cleanup_olc(d, CLEANUP_ALL);
		 break;
	      default:
		 send_to_char(d->character, "Invalid choice!\r\nDo you wish "
			      "to save this help entry internally? : ");
		 break;
	     }
	  return;

       case HEDIT_MAIN_MENU:
	  switch (*arg)
	     {
	      case 'q':
	      case 'Q':
		 if (OLC_VAL(d))  /* Something has been modified. */
		    {
		    send_to_char(d->character, "Do you wish to save this help entry internally? : ");
		    OLC_MODE(d) = HEDIT_CONFIRM_SAVESTRING;
		    } else
		       cleanup_olc(d, CLEANUP_ALL);
		 return;
	      case '1':
		 send_to_char(d->character, "Enter keywords:-\r\n] ");
		 OLC_MODE(d) = HEDIT_KEYWORDS;
		 break;
	      case '2':
		 OLC_MODE(d) = HEDIT_ENTRY;
		 SEND_TO_Q(d,"Enter help entry: (/s saves /h for help)\r\n\r\n");
		 d->backstr = NULL;
		 if (OLC_HELP(d)->entry) 
		    {
		    SEND_TO_Q(d, "%s", OLC_HELP(d)->entry);
		    d->backstr = str_dup(OLC_HELP(d)->entry);
		    }
		 d->str = &OLC_HELP(d)->entry;
		 d->max_str = MAX_HELP_ENTRY;
		 d->mail_to = 0;
		 OLC_VAL(d) = 1;
		 break;
	      case '3':
		 send_to_char(d->character, "Enter min level:-\r\n] ");
		 OLC_MODE(d) = HEDIT_MIN_LEVEL;
		 break;
	      default:
		 send_to_char(d->character, "Invalid choice!\r\n");
		 hedit_disp_menu(d);
		 break;
	     }
	  return;

       case HEDIT_KEYWORDS:
	  if (OLC_HELP(d)->keywords)
	     free(OLC_HELP(d)->keywords);
	  if (strlen(arg) > MAX_HELP_KEYWORDS)
	     arg[MAX_HELP_KEYWORDS - 1] = '\0';
	  OLC_HELP(d)->keywords = str_dup((arg && *arg) ? arg : "UNDEFINED");
	  break;

       case HEDIT_ENTRY:
	 /*
	  * We will NEVER get here, we hope.
	  */
	  mudlogf(BRF, LVL_BUILDER, TRUE,
		 "SYSERR: Reached HEDIT_ENTRY case in parse_hedit");
	  break;

       case HEDIT_MIN_LEVEL:
	  vnumber = atoi(arg);
	  if ((vnumber < 0) || (vnumber > LVL_IMPL))
	     send_to_char(d->character, "That is not a valid choice!\r\nEnter min level:-\r\n] ");
	  else 
	     {
	     OLC_HELP(d)->min_level = vnumber;
	     break;
	     }
	  return;

       default:
	 /*
	  * We should never get here.
	  */
	  mudlogf(BRF, LVL_BUILDER, TRUE,
		 "SYSERR: Reached default case in parse_hedit");
	  break;
      }
  /*
   * If we get this far, something has been changed.
   */
   OLC_VAL(d) = 1;
   hedit_disp_menu(d);
}

int find_help_rnum(char *keyword)
{
   int i;
   for (i = 0; i < top_of_helpt; i++)
      if (is_help(keyword, help_table[i].keywords))
	 return i;

   return -1;
}
