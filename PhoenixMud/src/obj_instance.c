/***********************************************************************
*  File: obj_instance.c                                                *
*  Usage: prototype-vs-instance ownership rules for saved objects      *
*                                                                      *
*  The save format keeps a full snapshot of every object, so a         *
*  builder's oedit reaches new spawns and never the copy in a player's *
*  rent, reimb, corpse, house or shop stock.  These routines split an  *
*  instance into its prototype plus the facts that happened to this    *
*  copy, so the load path can re-read world data and replay the facts. *
*                                                                      *
*  Pure over the two obj_data structs: no globals, so the table and    *
*  the derive/apply pair can be exercised by tests/obj_instance_test.  *
***********************************************************************/

#include "../localHeader/conf.h"
#include "../localHeader/sysdep.h"
#include "structs.h"
#include "utils.h"
#include "obj_instance.h"

/* do_brew overwrites these template potions wholesale (act.create.c,
 * potions[]): the "prototype" is a blank shell, so the snapshot is the
 * only record of what the potion is.  They keep full-snapshot loads. */
#define BREW_VNUM_LOW   700
#define BREW_VNUM_HIGH  719

/* extra_flags bits some play event sets or clears on a single copy:
 * curse/remove curse, bless, invisibility, the enchants (magic + the
 * caster-alignment anti bits), donation, a pulled grenade pin, corpse
 * bookkeeping, and the unique-save marker itself.  Every other bit is
 * the builder's. */
long obj_instance_extra_bits(void)
   {
   return ITEM_LIVE_GRENADE | ITEM_NEWBIE | ITEM_INVISIBLE | ITEM_MAGIC |
          ITEM_NODROP | ITEM_BLESS | ITEM_ANTI_GOOD | ITEM_ANTI_EVIL |
          ITEM_NOAUC | ITEM_DONATED | ITEM_PC_CORPSE | ITEM_NPC_CORPSE |
          ITEM_UNIQUE_SAVE;
   }

int obj_full_snapshot_vnum(obj_vnum vnum)
   {
   if (vnum == NOTHING)
      return TRUE;
   if (vnum >= BREW_VNUM_LOW && vnum <= BREW_VNUM_HIGH)
      return TRUE;
   return FALSE;
   }

int obj_slot_treatment(int item_type, int slot)
   {
   switch (item_type)
      {
      case ITEM_LIGHT:
         if (slot == 2)                  /* burn hours left              */
            return SLOT_DELTA;
         break;
      case ITEM_WAND:
      case ITEM_STAFF:
         if (slot == 2)                  /* charges left                 */
            return SLOT_DELTA;
         break;
      case ITEM_WEAPON:                  /* forge moves 0/1/2/6/7, curse */
         if (slot == 1 || slot == 2 || slot == 6)   /* moves 2          */
            return SLOT_DELTA;
         if (slot == 0 || slot == 7)
            return SLOT_ABS;
         break;
      case ITEM_CONTAINER:
         if (slot == 1)                  /* open/close/lock state        */
            return SLOT_MASK;
         if (slot == 5)                  /* contents weight              */
            return SLOT_DERIVED;
         break;
      case ITEM_DRINKCON:
      case ITEM_FOUNTAIN:
         if (slot == 1)                  /* units left                   */
            return SLOT_DELTA;
         if (slot == 2 || slot == 3)     /* liquid poured in, poison     */
            return SLOT_ABS;
         break;
      case ITEM_FOOD:
         if (slot == 0)                  /* portions left                */
            return SLOT_DELTA;
         if (slot == 3)                  /* poison                       */
            return SLOT_ABS;
         break;
      case ITEM_FUEL:
         if (slot == 0)                  /* units left in the source     */
            return SLOT_DELTA;
         break;
      case ITEM_GRENADE:
         if (slot == 0)                  /* fuse countdown               */
            return SLOT_ABS;
         break;
      case ITEM_PORTAL:
         if (slot == 0 || slot == 1)     /* uses left, conjured target   */
            return SLOT_ABS;
         break;
      }
   return SLOT_PROTO;
   }

long obj_slot_instance_bits(int item_type, int slot)
   {
   if (item_type == ITEM_CONTAINER && slot == 1)
      return CONT_CLOSED | CONT_LOCKED;
   return 0;
   }

void obj_facts_init(struct obj_instance_facts *f)
   {
   memset(f, 0, sizeof(*f));
   }

static long lmax(long a, long b)
   {
   return a > b ? a : b;
   }

static long lmin(long a, long b)
   {
   return a < b ? a : b;
   }

static char *dup_str(const char *s)
   {
   char *n;

   if (!s)
      return NULL;
   n = malloc(strlen(s) + 1);
   strcpy(n, s);
   return n;
   }

static void free_exdesc_list(struct extra_descr_data *ex)
   {
   struct extra_descr_data *next;

   for (; ex; ex = next)
      {
      next = ex->next;
      if (ex->keyword)
         free(ex->keyword);
      if (ex->description)
         free(ex->description);
      free(ex);
      }
   }

void obj_facts_free(struct obj_instance_facts *f)
   {
   if (f->name)
      free(f->name);
   if (f->short_desc)
      free(f->short_desc);
   if (f->long_desc)
      free(f->long_desc);
   if (f->action_desc)
      free(f->action_desc);
   free_exdesc_list(f->exdesc);
   f->name = f->short_desc = f->long_desc = f->action_desc = NULL;
   f->exdesc = NULL;
   }

static int str_differs(const char *a, const char *b)
   {
   if (!a && !b)
      return FALSE;
   if (!a || !b)
      return TRUE;
   return strcmp(a, b) != 0;
   }

static int exdesc_differs(struct extra_descr_data *a, struct extra_descr_data *b)
   {
   for (; a && b; a = a->next, b = b->next)
      if (str_differs(a->keyword, b->keyword) ||
          str_differs(a->description, b->description))
         return TRUE;
   return a != b;   /* one list longer */
   }

static struct extra_descr_data *dup_exdesc_list(struct extra_descr_data *ex)
   {
   struct extra_descr_data *head = NULL, *tail = NULL, *n;

   for (; ex; ex = ex->next)
      {
      n = calloc(1, sizeof(*n));
      n->keyword = dup_str(ex->keyword);
      n->description = dup_str(ex->description);
      if (tail)
         tail->next = n;
      else
         head = n;
      tail = n;
      }
   return head;
   }

static int applies_differ(struct obj_data *obj, struct obj_data *proto)
   {
   int i;

   for (i = 0; i < MAX_OBJ_AFFECT; i++)
      if (obj->affected[i].location != proto->affected[i].location ||
          obj->affected[i].modifier != proto->affected[i].modifier)
         return TRUE;
   return FALSE;
   }

/* Both enchants refuse an object that already carries an apply and both
 * set ITEM_MAGIC, so on an instance that is MAGIC where its prototype is
 * not, every apply came from the enchant.  The shape check guards the
 * one data-history hole (a prototype that once was MAGIC with applies of
 * its own): enchant grants can only ever be hitroll/damroll on a weapon
 * or a negative AC on armor. */
int obj_applies_are_enchant(struct obj_data *obj, struct obj_data *proto)
   {
   int i, hit = 0, dam = 0, ac = 0, other = 0;

   if (!IS_SET(GET_OBJ_EXTRA(obj), ITEM_MAGIC))
      return FALSE;
   if (IS_SET(GET_OBJ_EXTRA(proto), ITEM_MAGIC))
      return FALSE;
   for (i = 0; i < MAX_OBJ_AFFECT; i++)
      if (proto->affected[i].location != APPLY_NONE)
         return FALSE;
   for (i = 0; i < MAX_OBJ_AFFECT; i++)
      {
      if (obj->affected[i].location == APPLY_NONE)
         continue;
      switch (obj->affected[i].location)
         {
         case APPLY_HITROLL:
            if (obj->affected[i].modifier <= 0)
               return FALSE;
            hit++;
            break;
         case APPLY_DAMROLL:
            if (obj->affected[i].modifier < 0)
               return FALSE;
            dam++;
            break;
         case APPLY_AC:
            if (obj->affected[i].modifier >= 0)
               return FALSE;
            ac++;
            break;
         default:
            other++;
            break;
         }
      }
   if (other)
      return FALSE;
   if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
      return ac == 0 && hit <= 1 && dam <= 1 && hit + dam >= 1;
   if (GET_OBJ_TYPE(obj) == ITEM_ARMOR)
      return ac == 1 && hit == 0 && dam == 0;
   return FALSE;
   }

/* A weapon's dice follow the prototype unless this copy was visibly
 * forged (the forge counter in value[6] moved) or cursed (NODROP with a
 * lowered dice size).  Any other difference is a stale snapshot. */
static int weapon_forged(struct obj_data *obj, struct obj_data *proto)
   {
   return GET_OBJ_VAL(obj, 6) > GET_OBJ_VAL(proto, 6);
   }

static int weapon_cursed(struct obj_data *obj, struct obj_data *proto)
   {
   return IS_SET(GET_OBJ_EXTRA(obj), ITEM_NODROP) &&
          GET_OBJ_VAL(obj, 2) < GET_OBJ_VAL(proto, 2) &&
          GET_OBJ_VAL(obj, 1) == GET_OBJ_VAL(proto, 1);
   }

void obj_derive_instance_facts(struct obj_data *obj, struct obj_data *proto,
                               struct obj_instance_facts *f)
   {
   int type = GET_OBJ_TYPE(proto);
   int slot, treatment, forged, cursed;
   long bits, delta;

   obj_facts_init(f);
   f->has_type = TRUE;
   /* a class weapon in a form of another type is still its core: the form
    * is laid again from the hero state on load, so its facts are the core's */
   f->saved_type = obj->hero ? type : GET_OBJ_TYPE(obj);

   forged = (type == ITEM_WEAPON) && weapon_forged(obj, proto);
   cursed = (type == ITEM_WEAPON) && weapon_cursed(obj, proto);

   for (slot = 0; slot < NUM_OBJ_VAL_POSITIONS; slot++)
      {
      treatment = obj_slot_treatment(type, slot);
      if (type == ITEM_WEAPON && !forged)
         {
         if (slot == 2 && cursed)
            ;                            /* the one dice fact a curse leaves */
         else if (treatment == SLOT_DELTA || treatment == SLOT_ABS)
            continue;
         }
      switch (treatment)
         {
         case SLOT_DELTA:
            delta = GET_OBJ_VAL(obj, slot) - GET_OBJ_VAL(proto, slot);
            if (delta != 0)
               {
               f->val_kind[slot] = SLOT_DELTA;
               f->val_num[slot] = delta;
               }
            break;
         case SLOT_ABS:
            if (GET_OBJ_VAL(obj, slot) != GET_OBJ_VAL(proto, slot))
               {
               f->val_kind[slot] = SLOT_ABS;
               f->val_num[slot] = GET_OBJ_VAL(obj, slot);
               }
            break;
         case SLOT_MASK:
            bits = obj_slot_instance_bits(type, slot);
            if ((GET_OBJ_VAL(obj, slot) & bits) !=
                (GET_OBJ_VAL(proto, slot) & bits))
               {
               f->val_kind[slot] = SLOT_MASK;
               f->val_mask[slot] = bits;
               f->val_num[slot] = GET_OBJ_VAL(obj, slot) & bits;
               }
            break;
         }
      }

   bits = obj_instance_extra_bits();
   f->extra_set = (GET_OBJ_EXTRA(obj) & ~GET_OBJ_EXTRA(proto)) & bits;
   f->extra_clear = (GET_OBJ_EXTRA(proto) & ~GET_OBJ_EXTRA(obj)) & bits;
   /* no play event writes the other four words on an instance */

   if (GET_OBJ_CSLOTS(obj) != GET_OBJ_CSLOTS(proto) ||
       GET_OBJ_TSLOTS(obj) != GET_OBJ_TSLOTS(proto) ||
       GET_OBJ_OSLOTS(obj) != GET_OBJ_OSLOTS(proto))
      {
      f->has_cond = TRUE;
      f->cond_wear = GET_OBJ_OSLOTS(obj) - GET_OBJ_TSLOTS(obj);
      f->cond_dmg = GET_OBJ_TSLOTS(obj) - GET_OBJ_CSLOTS(obj);
      }

   if (applies_differ(obj, proto))
      {
      if (obj_applies_are_enchant(obj, proto))
         {
         for (slot = 0; slot < MAX_OBJ_AFFECT; slot++)
            if (obj->affected[slot].location != APPLY_NONE)
               f->applies[f->num_applies++] = obj->affected[slot];
         }
      else
         f->applies_dropped = TRUE;
      }

   /* Restrung text always travels with ITEM_UNIQUE_SAVE; a string
    * difference without it is transient (a drink container's liquid
    * keyword) and legacy has never carried those across a save. */
   if (IS_SET(GET_OBJ_EXTRA(obj), ITEM_UNIQUE_SAVE))
      {
      if (str_differs(obj->name, proto->name))
         f->name = dup_str(obj->name);
      if (str_differs(obj->short_description, proto->short_description))
         f->short_desc = dup_str(obj->short_description);
      if (str_differs(obj->description, proto->description))
         f->long_desc = dup_str(obj->description);
      if (str_differs(obj->action_description, proto->action_description))
         f->action_desc = dup_str(obj->action_description);
      if (exdesc_differs(obj->ex_description, proto->ex_description))
         {
         f->has_exdesc = TRUE;
         f->exdesc = dup_exdesc_list(obj->ex_description);
         }
      }
   }

static void clamp_slot(struct obj_data *obj, struct obj_data *proto, int type)
   {
   switch (type)
      {
      case ITEM_LIGHT:
         GET_OBJ_VAL(obj, 2) = lmax(0, lmin(GET_OBJ_VAL(obj, 2),
                                          GET_OBJ_VAL(obj, 3)));
         break;
      case ITEM_WAND:
      case ITEM_STAFF:
         GET_OBJ_VAL(obj, 2) = lmax(0, lmin(GET_OBJ_VAL(obj, 2),
                                          GET_OBJ_VAL(obj, 1)));
         break;
      case ITEM_DRINKCON:
      case ITEM_FOUNTAIN:
         GET_OBJ_VAL(obj, 1) = lmax(0, lmin(GET_OBJ_VAL(obj, 1),
                                          GET_OBJ_VAL(obj, 0)));
         break;
      case ITEM_FOOD:
         GET_OBJ_VAL(obj, 0) = lmax(0, lmin(GET_OBJ_VAL(obj, 0),
                                          GET_OBJ_VAL(proto, 0)));
         break;
      case ITEM_FUEL:
         GET_OBJ_VAL(obj, 0) = lmax(0, lmin(GET_OBJ_VAL(obj, 0),
                                          GET_OBJ_VAL(proto, 0)));
         break;
      case ITEM_WEAPON:
         GET_OBJ_VAL(obj, 1) = lmax(0, GET_OBJ_VAL(obj, 1));
         GET_OBJ_VAL(obj, 2) = lmax(0, GET_OBJ_VAL(obj, 2));
         break;
      case ITEM_GRENADE:
      case ITEM_PORTAL:
         GET_OBJ_VAL(obj, 0) = lmax(0, GET_OBJ_VAL(obj, 0));
         break;
      }
   }

/* Replace one string slot.  Prototype strings are shared and never
 * freed; anything else on the object was read from the file and dies
 * with the overwrite. */
static void set_str(char **slot, char *proto_str, char **fact)
   {
   char *want = *fact ? *fact : proto_str;

   if (*slot && *slot != proto_str && *slot != want)
      free(*slot);
   *slot = want;
   *fact = NULL;
   }

void obj_apply_instance_facts(struct obj_data *obj, struct obj_data *proto,
                              struct obj_instance_facts *f)
   {
   int type = GET_OBJ_TYPE(proto);
   int slot, i;

   /* identity follows world data */
   obj->obj_flags.type_flag = proto->obj_flags.type_flag;
   obj->obj_flags.wear_flags = proto->obj_flags.wear_flags;
   obj->obj_flags.cost = proto->obj_flags.cost;
   obj->obj_flags.cost_per_day = proto->obj_flags.cost_per_day;

   for (slot = 0; slot < NUM_OBJ_VAL_POSITIONS; slot++)
      {
      GET_OBJ_VAL(obj, slot) = GET_OBJ_VAL(proto, slot);
      switch (f->val_kind[slot])
         {
         case SLOT_DELTA:
            GET_OBJ_VAL(obj, slot) = GET_OBJ_VAL(proto, slot) + f->val_num[slot];
            break;
         case SLOT_ABS:
            GET_OBJ_VAL(obj, slot) = f->val_num[slot];
            break;
         case SLOT_MASK:
            GET_OBJ_VAL(obj, slot) =
               (GET_OBJ_VAL(proto, slot) & ~f->val_mask[slot]) |
               (f->val_num[slot] & f->val_mask[slot]);
            break;
         }
      }
   if (type == ITEM_CONTAINER)
      GET_OBJ_VAL(obj, 5) = 0;           /* rebuilt as contents load     */
   clamp_slot(obj, proto, type);

   GET_OBJ_WEIGHT(obj) = GET_OBJ_WEIGHT(proto);
   if (type == ITEM_DRINKCON || type == ITEM_FOUNTAIN)
      GET_OBJ_WEIGHT(obj) -= GET_OBJ_VAL(proto, 1) - GET_OBJ_VAL(obj, 1);

   GET_OBJ_EXTRA(obj) = (GET_OBJ_EXTRA(proto) & ~f->extra_clear) | f->extra_set;
   GET_OBJ_EXTRA2(obj) = (GET_OBJ_EXTRA2(proto) & ~f->extra2_clear) | f->extra2_set;
   GET_OBJ_EXTRA3(obj) = (GET_OBJ_EXTRA3(proto) & ~f->extra3_clear) | f->extra3_set;
   GET_OBJ_ANTI(obj) = (GET_OBJ_ANTI(proto) & ~f->anti_clear) | f->anti_set;
   obj->obj_flags.bitvector =
      (proto->obj_flags.bitvector & ~f->bitv_clear) | f->bitv_set;

   if (GET_OBJ_TSLOTS(proto) == 0 || !f->has_cond)
      {
      GET_OBJ_CSLOTS(obj) = GET_OBJ_CSLOTS(proto);
      GET_OBJ_TSLOTS(obj) = GET_OBJ_TSLOTS(proto);
      GET_OBJ_OSLOTS(obj) = GET_OBJ_OSLOTS(proto);
      }
   else
      {
      GET_OBJ_OSLOTS(obj) = GET_OBJ_TSLOTS(proto);
      GET_OBJ_TSLOTS(obj) = lmax(1, GET_OBJ_TSLOTS(proto) - f->cond_wear);
      GET_OBJ_CSLOTS(obj) = lmin(GET_OBJ_TSLOTS(obj),
                                GET_OBJ_TSLOTS(obj) - f->cond_dmg);
      }

   for (i = 0; i < MAX_OBJ_AFFECT; i++)
      obj->affected[i] = proto->affected[i];
   for (i = 0; i < f->num_applies; i++)
      for (slot = 0; slot < MAX_OBJ_AFFECT; slot++)
         if (obj->affected[slot].location == APPLY_NONE)
            {
            obj->affected[slot] = f->applies[i];
            break;
            }

   set_str(&obj->name, proto->name, &f->name);
   set_str(&obj->short_description, proto->short_description, &f->short_desc);
   set_str(&obj->description, proto->description, &f->long_desc);
   set_str(&obj->action_description, proto->action_description, &f->action_desc);

   if (f->has_exdesc)
      {
      if (obj->ex_description && obj->ex_description != proto->ex_description)
         free_exdesc_list(obj->ex_description);
      obj->ex_description = f->exdesc;
      f->exdesc = NULL;
      }
   else if (obj->ex_description != proto->ex_description)
      {
      free_exdesc_list(obj->ex_description);
      obj->ex_description = proto->ex_description;
      }
   }
