/* NetHack 3.6	artifact.c	$NHDT-Date: 1553363416 2019/03/23 17:50:16 $  $NHDT-Branch: NetHack-3.6.2-beta01 $:$NHDT-Revision: 1.129 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2013. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "artifact.h"
#include "artilist.h"

/*
 * Note:  both artilist[] and artiexist[] have a dummy element #0,
 *        so loops over them should normally start at #1.  The primary
 *        exception is the save & restore code, which doesn't care about
 *        the contents, just the total size.
 */

extern boolean notonhead; /* for long worms */

#define get_artifact(o) \
    (((o) && (o)->oartifact) ? &artilist[(int) (o)->oartifact] : 0)

STATIC_DCL boolean FDECL(bane_applies, (const struct artifact *,
                                        struct monst *));
STATIC_DCL int FDECL(spec_applies, (const struct artifact *, struct monst *));
STATIC_DCL int FDECL(arti_invoke, (struct obj *));
STATIC_DCL boolean FDECL(Mb_hit, (struct monst * magr, struct monst *mdef,
                                struct obj *, int *, int, BOOLEAN_P, char *));
STATIC_DCL unsigned long FDECL(abil_to_spfx, (long *));
STATIC_DCL uchar FDECL(abil_to_adtyp, (long *));
STATIC_DCL int FDECL(glow_strength, (int));
STATIC_DCL boolean FDECL(untouchable, (struct obj *, BOOLEAN_P));
STATIC_DCL int FDECL(count_surround_traps, (int, int));

/* The amount added to the victim's total hit points to insure that the
   victim will be killed even after damage bonus/penalty adjustments.
   Most such penalties are small, and 200 is plenty; the exception is
   half physical damage.  3.3.1 and previous versions tried to use a very
   large number to account for this case; now, we just compute the fatal
   damage by adding it to 2 times the total hit points instead of 1 time.
   Note: this will still break if they have more than about half the number
   of hit points that will fit in a 15 bit integer. */
#define FATAL_DAMAGE_MODIFIER 200

/* coordinate effects from spec_dbon() with messages in artifact_hit() */
STATIC_OVL int spec_dbon_applies = 0;

/* flags including which artifacts have already been created */
static boolean artiexist[1 + NROFARTIFACTS + 1];
/* and a discovery list for them (no dummy first entry here) */
STATIC_OVL xchar artidisco[NROFARTIFACTS];

STATIC_DCL void NDECL(hack_artifacts);
STATIC_DCL boolean FDECL(attacks, (int, struct obj *));

/* handle some special cases; must be called after u_init() */
STATIC_OVL void
hack_artifacts()
{
    struct artifact *art;
    int alignmnt = aligns[flags.initalign].value;

    /* Fix up the alignments of "gift" artifacts */
    for (art = artilist + 1; art->otyp; art++)
        if (art->role == Role_switch && art->alignment != A_NONE)
            art->alignment = alignmnt;

    /* Excalibur can be used by any lawful character, not just knights */
    if (!Role_if(PM_KNIGHT))
        artilist[ART_EXCALIBUR].role = NON_PM;

    /* Fix up the quest artifact */
    if (urole.questarti) {
        artilist[urole.questarti].alignment = alignmnt;
        artilist[urole.questarti].role = Role_switch;
    }
    return;
}

/* zero out the artifact existence list */
void
init_artifacts()
{
    (void) memset((genericptr_t) artiexist, 0, sizeof artiexist);
    (void) memset((genericptr_t) artidisco, 0, sizeof artidisco);
    hack_artifacts();
}

void
save_artifacts(fd)
int fd;
{
    bwrite(fd, (genericptr_t) artiexist, sizeof artiexist);
    bwrite(fd, (genericptr_t) artidisco, sizeof artidisco);
}

void
restore_artifacts(fd)
int fd;
{
    mread(fd, (genericptr_t) artiexist, sizeof artiexist);
    mread(fd, (genericptr_t) artidisco, sizeof artidisco);
    hack_artifacts(); /* redo non-saved special cases */
}

const char *
artiname(artinum)
int artinum;
{
    if (artinum <= 0 || artinum > NROFARTIFACTS)
        return "";
#if 0 /*KR: 원본*/
    return artilist[artinum].name;
#else /*KR: KRNethack 맞춤 번역 */
    extern char *get_kr_name(const char *);
    return get_kr_name(artilist[artinum].name);
#endif
}

/*
   Make an artifact.  If a specific alignment is specified, then an object of
   the appropriate alignment is created from scratch, or 0 is returned if
   none is available.  (If at least one aligned artifact has already been
   given, then unaligned ones also become eligible for this.)
   If no alignment is given, then 'otmp' is converted
   into an artifact of matching type, or returned as-is if that's not
   possible.
   For the 2nd case, caller should use ``obj = mk_artifact(obj, A_NONE);''
   for the 1st, ``obj = mk_artifact((struct obj *)0, some_alignment);''.
 */
struct obj *
mk_artifact(otmp, alignment)
struct obj *otmp;   /* existing object; ignored if alignment specified */
aligntyp alignment; /* target alignment, or A_NONE */
{
    const struct artifact *a;
    int m, n, altn;
    boolean by_align = (alignment != A_NONE);
    short o_typ = (by_align || !otmp) ? 0 : otmp->otyp;
    boolean unique = !by_align && otmp && objects[o_typ].oc_unique;
    short eligible[NROFARTIFACTS];

    n = altn = 0;    /* no candidates found yet */
    eligible[0] = 0; /* lint suppression */
    /* gather eligible artifacts */
    for (m = 1, a = &artilist[m]; a->otyp; a++, m++) {
        if (artiexist[m])
            continue;
        if ((a->spfx & SPFX_NOGEN) || unique)
            continue;

        if (!by_align) {
            /* looking for a particular type of item; not producing a
               divine gift so we don't care about role's first choice */
            if (a->otyp == o_typ)
                eligible[n++] = m;
            continue; /* move on to next possibility */
        }

        /* we're looking for an alignment-specific item
           suitable for hero's role+race */
        if ((a->alignment == alignment || a->alignment == A_NONE)
            /* avoid enemies' equipment */
            && (a->race == NON_PM || !race_hostile(&mons[a->race]))) {
            /* when a role-specific first choice is available, use it */
            if (Role_if(a->role)) {
                /* make this be the only possibility in the list */
                eligible[0] = m;
                n = 1;
                break; /* skip all other candidates */
            }
            /* found something to consider for random selection */
            if (a->alignment != A_NONE || u.ugifts > 0) {
                /* right alignment, or non-aligned with at least 1
                   previous gift bestowed, makes this one viable */
                eligible[n++] = m;
            } else {
                /* non-aligned with no previous gifts;
                   if no candidates have been found yet, record
                   this one as a[nother] fallback possibility in
                   case all aligned candidates have been used up
                   (via wishing, naming, bones, random generation) */
                if (!n)
                    eligible[altn++] = m;
                /* [once a regular candidate is found, the list
                   is overwritten and `altn' becomes irrelevant] */
            }
        }
    }

    /* resort to fallback list if main list was empty */
    if (!n)
        n = altn;

    if (n) {
        /* found at least one candidate; pick one at random */
        m = eligible[rn2(n)]; /* [0..n-1] */
        a = &artilist[m];

        /* make an appropriate object if necessary, then christen it */
        if (by_align)
            otmp = mksobj((int) a->otyp, TRUE, FALSE);

        if (otmp) {
            otmp = oname(otmp, a->name);
            otmp->oartifact = m;
            artiexist[m] = TRUE;
        }
    } else {
        /* nothing appropriate could be found; return original object */
        if (by_align)
            otmp = 0; /* (there was no original object) */
    }
    return otmp;
}

/*
 * Returns the full name (with articles and correct capitalization) of an
 * artifact named "name" if one exists, or NULL, it not.
 * The given name must be rather close to the real name for it to match.
 * The object type of the artifact is returned in otyp if the return value
 * is non-NULL.
 */
const char *
artifact_name(name, otyp)
const char *name;
short *otyp;
{
    register const struct artifact *a;
    register const char *aname;

    if (!strncmpi(name, "the ", 4))
        name += 4;

    for (a = artilist + 1; a->otyp; a++) {
        aname = a->name;
        if (!strncmpi(aname, "the ", 4))
            aname += 4;
#if 0 /*KR: 원본*/
        if (!strcmpi(name, aname)) {
#else /*KR: KRNethack 맞춤 번역 */
        extern char *get_kr_name(const char *);
        /* 영어 스펠링(aname)과 일치하거나, 한글 번역 이름과 일치하면 통과! */
        /* 한글은 대소문자가 없으므로 strcmp를 씁니다. */
        if (!strcmpi(name, aname) || !strcmp(name, get_kr_name(a->name))) {
#endif
            *otyp = a->otyp;
            return a->name;
        }
    }

    return (char *) 0;
}

boolean
exist_artifact(otyp, name)
int otyp;
const char *name;
{
    register const struct artifact *a;
    boolean *arex;

    if (otyp && *name)
        for (a = artilist + 1, arex = artiexist + 1; a->otyp; a++, arex++)
            if ((int) a->otyp == otyp && !strcmp(a->name, name))
                return *arex;
    return FALSE;
}

void
artifact_exists(otmp, name, mod)
struct obj *otmp;
const char *name;
boolean mod;
{
    register const struct artifact *a;

    if (otmp && *name)
        for (a = artilist + 1; a->otyp; a++)
            if (a->otyp == otmp->otyp && !strcmp(a->name, name)) {
                register int m = (int) (a - artilist);
                otmp->oartifact = (char) (mod ? m : 0);
                otmp->age = 0;
                if (otmp->otyp == RIN_INCREASE_DAMAGE)
                    otmp->spe = 0;
                artiexist[m] = mod;
                break;
            }
    return;
}

int
nartifact_exist()
{
    int a = 0;
    int n = SIZE(artiexist);

    while (n > 1)
        if (artiexist[--n])
            a++;

    return a;
}

boolean
spec_ability(otmp, abil)
struct obj *otmp;
unsigned long abil;
{
    const struct artifact *arti = get_artifact(otmp);

    return (boolean) (arti && (arti->spfx & abil) != 0L);
}

/* used so that callers don't need to known about SPFX_ codes */
boolean
confers_luck(obj)
struct obj *obj;
{
    /* might as well check for this too */
    if (obj->otyp == LUCKSTONE)
        return TRUE;

    return (boolean) (obj->oartifact && spec_ability(obj, SPFX_LUCK));
}

/* used to check whether a monster is getting reflection from an artifact */
boolean
arti_reflects(obj)
struct obj *obj;
{
    const struct artifact *arti = get_artifact(obj);

    if (arti) {
        /* while being worn */
        if ((obj->owornmask & ~W_ART) && (arti->spfx & SPFX_REFLECT))
            return TRUE;
        /* just being carried */
        if (arti->cspfx & SPFX_REFLECT)
            return TRUE;
    }
    return FALSE;
}

/* decide whether this obj is effective when attacking against shades;
   does not consider the bonus for blessed objects versus undead */
boolean
shade_glare(obj)
struct obj *obj;
{
    const struct artifact *arti;

    /* any silver object is effective */
    if (objects[obj->otyp].oc_material == SILVER)
        return TRUE;
    /* non-silver artifacts with bonus against undead also are effective */
    arti = get_artifact(obj);
    if (arti && (arti->spfx & SPFX_DFLAG2) && arti->mtype == M2_UNDEAD)
        return TRUE;
    /* [if there was anything with special bonus against noncorporeals,
       it would be effective too] */
    /* otherwise, harmless to shades */
    return FALSE;
}

/* returns 1 if name is restricted for otmp->otyp */
boolean
restrict_name(otmp, name)
struct obj *otmp;
const char *name;
{
    register const struct artifact *a;
    const char *aname, *odesc, *other;
    boolean sametype[NUM_OBJECTS];
    int i, lo, hi, otyp = otmp->otyp, ocls = objects[otyp].oc_class;

    if (!*name)
        return FALSE;
    if (!strncmpi(name, "the ", 4))
        name += 4;

    /* decide what types of objects are the same as otyp;
       if it's been discovered, then only itself matches;
       otherwise, include all other undiscovered objects
       of the same class which have the same description
       or share the same pool of shuffled descriptions */
    (void) memset((genericptr_t) sametype, 0, sizeof sametype); /* FALSE */
    sametype[otyp] = TRUE;
    if (!objects[otyp].oc_name_known
        && (odesc = OBJ_DESCR(objects[otyp])) != 0) {
        obj_shuffle_range(otyp, &lo, &hi);
        for (i = bases[ocls]; i < NUM_OBJECTS; i++) {
            if (objects[i].oc_class != ocls)
                break;
            if (!objects[i].oc_name_known
                && (other = OBJ_DESCR(objects[i])) != 0
                && (!strcmp(odesc, other) || (i >= lo && i <= hi)))
                sametype[i] = TRUE;
        }
    }

    /* Since almost every artifact is SPFX_RESTR, it doesn't cost
       us much to do the string comparison before the spfx check.
       Bug fix:  don't name multiple elven daggers "Sting".
     */
    for (a = artilist + 1; a->otyp; a++) {
        if (!sametype[a->otyp])
            continue;
        aname = a->name;
        if (!strncmpi(aname, "the ", 4))
            aname += 4;
        if (!strcmp(aname, name))
            return (boolean) ((a->spfx & (SPFX_NOGEN | SPFX_RESTR)) != 0
                              || otmp->quan > 1L);
    }

    return FALSE;
}

STATIC_OVL boolean
attacks(adtyp, otmp)
int adtyp;
struct obj *otmp;
{
    register const struct artifact *weap;

    if ((weap = get_artifact(otmp)) != 0)
        return (boolean) (weap->attk.adtyp == adtyp);
    return FALSE;
}

boolean
defends(adtyp, otmp)
int adtyp;
struct obj *otmp;
{
    register const struct artifact *weap;

    if ((weap = get_artifact(otmp)) != 0)
        return (boolean) (weap->defn.adtyp == adtyp);
    return FALSE;
}

/* used for monsters */
boolean
defends_when_carried(adtyp, otmp)
int adtyp;
struct obj *otmp;
{
    register const struct artifact *weap;

    if ((weap = get_artifact(otmp)) != 0)
        return (boolean) (weap->cary.adtyp == adtyp);
    return FALSE;
}

/* determine whether an item confers Protection */
boolean
protects(otmp, being_worn)
struct obj *otmp;
boolean being_worn;
{
    const struct artifact *arti;

    if (being_worn && objects[otmp->otyp].oc_oprop == PROTECTION)
        return TRUE;
    arti = get_artifact(otmp);
    if (!arti)
        return FALSE;
    return (boolean) ((arti->cspfx & SPFX_PROTECT) != 0
                      || (being_worn && (arti->spfx & SPFX_PROTECT) != 0));
}

/*
 * a potential artifact has just been worn/wielded/picked-up or
 * unworn/unwielded/dropped.  Pickup/drop only set/reset the W_ART mask.
 */
void
set_artifact_intrinsic(otmp, on, wp_mask)
struct obj *otmp;
boolean on;
long wp_mask;
{
    long *mask = 0;
    register const struct artifact *art, *oart = get_artifact(otmp);
    register struct obj *obj;
    register uchar dtyp;
    register long spfx;

    if (!oart)
        return;

    /* effects from the defn field */
    dtyp = (wp_mask != W_ART) ? oart->defn.adtyp : oart->cary.adtyp;

    if (dtyp == AD_FIRE)
        mask = &EFire_resistance;
    else if (dtyp == AD_COLD)
        mask = &ECold_resistance;
    else if (dtyp == AD_ELEC)
        mask = &EShock_resistance;
    else if (dtyp == AD_MAGM)
        mask = &EAntimagic;
    else if (dtyp == AD_DISN)
        mask = &EDisint_resistance;
    else if (dtyp == AD_DRST)
        mask = &EPoison_resistance;
    else if (dtyp == AD_DRLI)
        mask = &EDrain_resistance;

    if (mask && wp_mask == W_ART && !on) {
        /* find out if some other artifact also confers this intrinsic;
           if so, leave the mask alone */
        for (obj = invent; obj; obj = obj->nobj) {
            if (obj != otmp && obj->oartifact) {
                art = get_artifact(obj);
                if (art && art->cary.adtyp == dtyp) {
                    mask = (long *) 0;
                    break;
                }
            }
        }
    }
    if (mask) {
        if (on)
            *mask |= wp_mask;
        else
            *mask &= ~wp_mask;
    }

    /* intrinsics from the spfx field; there could be more than one */
    spfx = (wp_mask != W_ART) ? oart->spfx : oart->cspfx;
    if (spfx && wp_mask == W_ART && !on) {
        /* don't change any spfx also conferred by other artifacts */
        for (obj = invent; obj; obj = obj->nobj)
            if (obj != otmp && obj->oartifact) {
                art = get_artifact(obj);
                if (art)
                    spfx &= ~art->cspfx;
            }
    }

    if (spfx & SPFX_SEARCH) {
        if (on)
            ESearching |= wp_mask;
        else
            ESearching &= ~wp_mask;
    }
    if (spfx & SPFX_HALRES) {
        /* make_hallucinated must (re)set the mask itself to get
         * the display right */
        /* restoring needed because this is the only artifact intrinsic
         * that can print a message--need to guard against being printed
         * when restoring a game
         */
        (void) make_hallucinated((long) !on, restoring ? FALSE : TRUE,
                                 wp_mask);
    }
    if (spfx & SPFX_ESP) {
        if (on)
            ETelepat |= wp_mask;
        else
            ETelepat &= ~wp_mask;
        see_monsters();
    }
    if (spfx & SPFX_STLTH) {
        if (on)
            EStealth |= wp_mask;
        else
            EStealth &= ~wp_mask;
    }
    if (spfx & SPFX_REGEN) {
        if (on)
            ERegeneration |= wp_mask;
        else
            ERegeneration &= ~wp_mask;
    }
    if (spfx & SPFX_TCTRL) {
        if (on)
            ETeleport_control |= wp_mask;
        else
            ETeleport_control &= ~wp_mask;
    }
    if (spfx & SPFX_WARN) {
        if (spec_m2(otmp)) {
            if (on) {
                EWarn_of_mon |= wp_mask;
                context.warntype.obj |= spec_m2(otmp);
            } else {
                EWarn_of_mon &= ~wp_mask;
                context.warntype.obj &= ~spec_m2(otmp);
            }
            see_monsters();
        } else {
            if (on)
                EWarning |= wp_mask;
            else
                EWarning &= ~wp_mask;
        }
    }
    if (spfx & SPFX_EREGEN) {
        if (on)
            EEnergy_regeneration |= wp_mask;
        else
            EEnergy_regeneration &= ~wp_mask;
    }
    if (spfx & SPFX_HSPDAM) {
        if (on)
            EHalf_spell_damage |= wp_mask;
        else
            EHalf_spell_damage &= ~wp_mask;
    }
    if (spfx & SPFX_HPHDAM) {
        if (on)
            EHalf_physical_damage |= wp_mask;
        else
            EHalf_physical_damage &= ~wp_mask;
    }
    if (spfx & SPFX_XRAY) {
        /* this assumes that no one else is using xray_range */
        if (on)
            u.xray_range = 3;
        else
            u.xray_range = -1;
        vision_full_recalc = 1;
    }
    if ((spfx & SPFX_REFLECT) && (wp_mask & W_WEP)) {
        if (on)
            EReflecting |= wp_mask;
        else
            EReflecting &= ~wp_mask;
    }
    if (spfx & SPFX_PROTECT) {
        if (on)
            EProtection |= wp_mask;
        else
            EProtection &= ~wp_mask;
    }

    if (wp_mask == W_ART && !on && oart->inv_prop) {
        /* might have to turn off invoked power too */
        if (oart->inv_prop <= LAST_PROP
            && (u.uprops[oart->inv_prop].extrinsic & W_ARTI))
            (void) arti_invoke(otmp);
    }
}

/* touch_artifact()'s return value isn't sufficient to tell whether it
   dished out damage, and tracking changes to u.uhp, u.mh, Lifesaved
   when trying to avoid second wounding is too cumbersome */
STATIC_VAR boolean touch_blasted; /* for retouch_object() */

/*
 * creature (usually hero) tries to touch (pick up or wield) an artifact obj.
 * Returns 0 if the object refuses to be touched.
 * This routine does not change any object chains.
 * Ignores such things as gauntlets, assuming the artifact is not
 * fooled by such trappings.
 */
int
touch_artifact(obj, mon)
struct obj *obj;
struct monst *mon;
{
    register const struct artifact *oart = get_artifact(obj);
    boolean badclass, badalign, self_willed, yours;

    touch_blasted = FALSE;
    if (!oart)
        return 1;

    yours = (mon == &youmonst);
    /* all quest artifacts are self-willed; if this ever changes, `badclass'
       will have to be extended to explicitly include quest artifacts */
    self_willed = ((oart->spfx & SPFX_INTEL) != 0);
    if (yours) {
        badclass = self_willed
                   && ((oart->role != NON_PM && !Role_if(oart->role))
                       || (oart->race != NON_PM && !Race_if(oart->race)));
        badalign = ((oart->spfx & SPFX_RESTR) != 0
                    && oart->alignment != A_NONE
                    && (oart->alignment != u.ualign.type
                        || u.ualign.record < 0));
    } else if (!is_covetous(mon->data) && !is_mplayer(mon->data)) {
        badclass = self_willed && oart->role != NON_PM
                   && oart != &artilist[ART_EXCALIBUR];
        badalign = (oart->spfx & SPFX_RESTR) && oart->alignment != A_NONE
                   && (oart->alignment != mon_aligntyp(mon));
    } else { /* an M3_WANTSxxx monster or a fake player */
        /* special monsters trying to take the Amulet, invocation tools or
           quest item can touch anything except `spec_applies' artifacts */
        badclass = badalign = FALSE;
    }
    /* weapons which attack specific categories of monsters are
       bad for them even if their alignments happen to match */
    if (!badalign)
        badalign = bane_applies(oart, mon);

    if (((badclass || badalign) && self_willed)
        || (badalign && (!yours || !rn2(4)))) {
        int dmg, tmp;
        char buf[BUFSZ];

        if (!yours)
            return 0;
        /*KR You("are blasted by %s power!", s_suffix(the(xname(obj)))); */
        You("%s의 힘에 맞았다!", xname(obj));
        touch_blasted = TRUE;
        dmg = d((Antimagic ? 2 : 4), (self_willed ? 10 : 4));
        /* add half (maybe quarter) of the usual silver damage bonus */
        if (objects[obj->otyp].oc_material == SILVER && Hate_silver)
            tmp = rnd(10), dmg += Maybe_Half_Phys(tmp);
        /*KR Sprintf(buf, "touching %s", oart->name); */
        Sprintf(buf, "%s에 닿아서", oart->name);
        losehp(dmg, buf, KILLED_BY); /* magic damage, not physical */
        exercise(A_WIS, FALSE);
    }

    /* can pick it up unless you're totally non-synch'd with the artifact */
    if (badclass && badalign && self_willed) {
        if (yours) {
            if (!carried(obj))
                /*KR pline("%s your grasp!", Tobjnam(obj, "evade")); */
                pline("%s 당신의 손길을 피했다!", append_josa(xname(obj), "이"));
            else
                /*KR pline("%s beyond your control!", Tobjnam(obj, "are")); */
                pline("%s 당신의 통제를 벗어났다!", append_josa(xname(obj), "은"));
        }
        return 0;
    }

    return 1;
}

/* decide whether an artifact itself is vulnerable to a particular type
   of erosion damage, independent of the properties of its bearer */
boolean
arti_immune(obj, dtyp)
struct obj *obj;
int dtyp;
{
    register const struct artifact *weap = get_artifact(obj);

    if (!weap)
        return FALSE;
    if (dtyp == AD_PHYS)
        return FALSE; /* nothing is immune to phys dmg */
    return (boolean) (weap->attk.adtyp == dtyp
                      || weap->defn.adtyp == dtyp
                      || weap->cary.adtyp == dtyp);
}

STATIC_OVL boolean
bane_applies(oart, mon)
const struct artifact *oart;
struct monst *mon;
{
    struct artifact atmp;

    if (oart && (oart->spfx & SPFX_DBONUS) != 0) {
        atmp = *oart;
        atmp.spfx &= SPFX_DBONUS; /* clear other spfx fields */
        if (spec_applies(&atmp, mon))
            return TRUE;
    }
    return FALSE;
}

/* decide whether an artifact's special attacks apply against mtmp */
STATIC_OVL int
spec_applies(weap, mtmp)
register const struct artifact *weap;
struct monst *mtmp;
{
    struct permonst *ptr;
    boolean yours;

    if (!(weap->spfx & (SPFX_DBONUS | SPFX_ATTK)))
        return (weap->attk.adtyp == AD_PHYS);

    yours = (mtmp == &youmonst);
    ptr = mtmp->data;

    if (weap->spfx & SPFX_DMONS) {
        return (ptr == &mons[(int) weap->mtype]);
    } else if (weap->spfx & SPFX_DCLAS) {
        return (weap->mtype == (unsigned long) ptr->mlet);
    } else if (weap->spfx & SPFX_DFLAG1) {
        return ((ptr->mflags1 & weap->mtype) != 0L);
    } else if (weap->spfx & SPFX_DFLAG2) {
        return ((ptr->mflags2 & weap->mtype)
                || (yours
                    && ((!Upolyd && (urace.selfmask & weap->mtype))
                        || ((weap->mtype & M2_WERE) && u.ulycn >= LOW_PM))));
    } else if (weap->spfx & SPFX_DALIGN) {
        return yours ? (u.ualign.type != weap->alignment)
                     : (ptr->maligntyp == A_NONE
                        || sgn(ptr->maligntyp) != weap->alignment);
    } else if (weap->spfx & SPFX_ATTK) {
        struct obj *defending_weapon = (yours ? uwep : MON_WEP(mtmp));

        if (defending_weapon && defending_weapon->oartifact
            && defends((int) weap->attk.adtyp, defending_weapon))
            return FALSE;
        switch (weap->attk.adtyp) {
        case AD_FIRE:
            return !(yours ? Fire_resistance : resists_fire(mtmp));
        case AD_COLD:
            return !(yours ? Cold_resistance : resists_cold(mtmp));
        case AD_ELEC:
            return !(yours ? Shock_resistance : resists_elec(mtmp));
        case AD_MAGM:
        case AD_STUN:
            return !(yours ? Antimagic : (rn2(100) < ptr->mr));
        case AD_DRST:
            return !(yours ? Poison_resistance : resists_poison(mtmp));
        case AD_DRLI:
            return !(yours ? Drain_resistance : resists_drli(mtmp));
        case AD_STON:
            return !(yours ? Stone_resistance : resists_ston(mtmp));
        default:
            impossible("Weird weapon special attack.");
        }
    }
    return 0;
}

/* return the M2 flags of monster that an artifact's special attacks apply
 * against */
long
spec_m2(otmp)
struct obj *otmp;
{
    const struct artifact *artifact = get_artifact(otmp);

    if (artifact)
        return artifact->mtype;
    return 0L;
}

/* special attack bonus */
int
spec_abon(otmp, mon)
struct obj *otmp;
struct monst *mon;
{
    const struct artifact *weap = get_artifact(otmp);

    /* no need for an extra check for `NO_ATTK' because this will
       always return 0 for any artifact which has that attribute */

    if (weap && weap->attk.damn && spec_applies(weap, mon))
        return rnd((int) weap->attk.damn);
    return 0;
}

/* special damage bonus */
int
spec_dbon(otmp, mon, tmp)
struct obj *otmp;
struct monst *mon;
int tmp;
{
    register const struct artifact *weap = get_artifact(otmp);

    if (!weap || (weap->attk.adtyp == AD_PHYS /* check for `NO_ATTK' */
                  && weap->attk.damn == 0 && weap->attk.damd == 0))
        spec_dbon_applies = FALSE;
    else if (otmp->oartifact == ART_GRIMTOOTH)
        /* Grimtooth has SPFX settings to warn against elves but we want its
           damage bonus to apply to all targets, so bypass spec_applies() */
        spec_dbon_applies = TRUE;
    else
        spec_dbon_applies = spec_applies(weap, mon);

    if (spec_dbon_applies)
        return weap->attk.damd ? rnd((int) weap->attk.damd) : max(tmp, 1);
    return 0;
}

/* add identified artifact to discoveries list */
void
discover_artifact(m)
xchar m;
{
    int i;

    /* look for this artifact in the discoveries list;
       if we hit an empty slot then it's not present, so add it */
    for (i = 0; i < NROFARTIFACTS; i++)
        if (artidisco[i] == 0 || artidisco[i] == m) {
            artidisco[i] = m;
            return;
        }
    /* there is one slot per artifact, so we should never reach the
       end without either finding the artifact or an empty slot... */
    impossible("couldn't discover artifact (%d)", (int) m);
}

/* used to decide whether an artifact has been fully identified */
boolean
undiscovered_artifact(m)
xchar m;
{
    int i;

    /* look for this artifact in the discoveries list;
       if we hit an empty slot then it's undiscovered */
    for (i = 0; i < NROFARTIFACTS; i++)
        if (artidisco[i] == m)
            return FALSE;
        else if (artidisco[i] == 0)
            break;
    return TRUE;
}

/* display a list of discovered artifacts; return their count */
int
disp_artifact_discoveries(tmpwin)
winid tmpwin; /* supplied by dodiscover() */
{
    int i, m, otyp;
    char buf[BUFSZ];

    for (i = 0; i < NROFARTIFACTS; i++) {
        if (artidisco[i] == 0)
            break; /* empty slot implies end of list */
        if (tmpwin == WIN_ERR)
            continue; /* for WIN_ERR, we just count */

        if (i == 0)
            /*KR putstr(tmpwin, iflags.menu_headings, "Artifacts"); */
            putstr(tmpwin, iflags.menu_headings, "아티팩트");
        m = artidisco[i];
        otyp = artilist[m].otyp;
        Sprintf(buf, "  %s [%s %s]", artiname(m),
                align_str(artilist[m].alignment), simple_typename(otyp));
        putstr(tmpwin, 0, buf);
    }
    return i;
}

/*
 * Magicbane's intrinsic magic is incompatible with normal
 * enchantment magic.  Thus, its effects have a negative
 * dependence on spe.  Against low mr victims, it typically
 * does "double athame" damage, 2d4.  Occasionally, it will
 * cast unbalancing magic which effectively averages out to
 * 4d4 damage (3d4 against high mr victims), for spe = 0.
 *
 * Prior to 3.4.1, the cancel (aka purge) effect always
 * included the scare effect too; now it's one or the other.
 * Likewise, the stun effect won't be combined with either
 * of those two; it will be chosen separately or possibly
 * used as a fallback when scare or cancel fails.
 *
 * [Historical note: a change to artifact_hit() for 3.4.0
 * unintentionally made all of Magicbane's special effects
 * be blocked if the defender successfully saved against a
 * stun attack.  As of 3.4.1, those effects can occur but
 * will be slightly less likely than they were in 3.3.x.]
 */

enum mb_effect_indices {
    MB_INDEX_PROBE = 0,
    MB_INDEX_STUN,
    MB_INDEX_SCARE,
    MB_INDEX_CANCEL,

    NUM_MB_INDICES
};

#define MB_MAX_DIEROLL 8 /* rolls above this aren't magical */
static const char *const mb_verb[2][NUM_MB_INDICES] = {
#if 0 /*KR 완전한 과거형으로 작성 (한국어는 불규칙 활용 심함) */
    { "probe", "stun", "scare", "cancel" },
    { "prod", "amaze", "tickle", "purge" },
#else    
    { "조사했다", "기절시켰다", "겁을 주었다", "무효화했다" },
    { "찔렀다", "놀라게 했다", "간지럽혔다", "정화했다" },
#endif
};

/* called when someone is being hit by Magicbane */
STATIC_OVL boolean
Mb_hit(magr, mdef, mb, dmgptr, dieroll, vis, hittee)
struct monst *magr, *mdef; /* attacker and defender */
struct obj *mb;            /* Magicbane */
int *dmgptr;               /* extra damage target will suffer */
int dieroll;               /* d20 that has already scored a hit */
boolean vis;               /* whether the action can be seen */
char *hittee;              /* target's name: "you" or mon_nam(mdef) */
{
    struct permonst *old_uasmon;
    const char *verb;
    boolean youattack = (magr == &youmonst), youdefend = (mdef == &youmonst),
            resisted = FALSE, do_stun, do_confuse, result;
#if 0 /*KR*/
    int attack_indx, fakeidx, scare_dieroll = MB_MAX_DIEROLL / 2;
#else
    int attack_indx, scare_dieroll = MB_MAX_DIEROLL / 2;
#endif

    result = FALSE; /* no message given yet */
    /* the most severe effects are less likely at higher enchantment */
    if (mb->spe >= 3)
        scare_dieroll /= (1 << (mb->spe / 3));
    /* if target successfully resisted the artifact damage bonus,
       reduce overall likelihood of the assorted special effects */
    if (!spec_dbon_applies)
        dieroll += 1;

    /* might stun even when attempting a more severe effect, but
       in that case it will only happen if the other effect fails;
       extra damage will apply regardless; 3.4.1: sometimes might
       just probe even when it hasn't been enchanted */
    do_stun = (max(mb->spe, 0) < rn2(spec_dbon_applies ? 11 : 7));

    /* the special effects also boost physical damage; increments are
       generally cumulative, but since the stun effect is based on a
       different criterium its damage might not be included; the base
       damage is either 1d4 (athame) or 2d4 (athame+spec_dbon) depending
       on target's resistance check against AD_STUN (handled by caller)
       [note that a successful save against AD_STUN doesn't actually
       prevent the target from ending up stunned] */
    attack_indx = MB_INDEX_PROBE;
    *dmgptr += rnd(4); /* (2..3)d4 */
    if (do_stun) {
        attack_indx = MB_INDEX_STUN;
        *dmgptr += rnd(4); /* (3..4)d4 */
    }
    if (dieroll <= scare_dieroll) {
        attack_indx = MB_INDEX_SCARE;
        *dmgptr += rnd(4); /* (3..5)d4 */
    }
    if (dieroll <= (scare_dieroll / 2)) {
        attack_indx = MB_INDEX_CANCEL;
        *dmgptr += rnd(4); /* (4..6)d4 */
    }

    /* give the hit message prior to inflicting the effects */
    verb = mb_verb[!!Hallucination][attack_indx];
    if (youattack || youdefend || vis) {
        result = TRUE;
#if 0 /*KR: 원본*/
        pline_The("magic-absorbing blade %s %s!",
                  vtense((const char *) 0, verb), hittee);
#else 
        pline("마력을 흡수하는 칼날이 %s %s!", 
              append_josa(hittee, "을"), verb);
#endif
        /* assume probing has some sort of noticeable feedback
           even if it is being done by one monster to another */
        if (attack_indx == MB_INDEX_PROBE && !canspotmon(mdef))
            map_invisible(mdef->mx, mdef->my);
    }

    /* now perform special effects */
    switch (attack_indx) {
    case MB_INDEX_CANCEL:
        old_uasmon = youmonst.data;
        /* No mdef->mcan check: even a cancelled monster can be polymorphed
         * into a golem, and the "cancel" effect acts as if some magical
         * energy remains in spellcasting defenders to be absorbed later.
         */
        if (!cancel_monst(mdef, mb, youattack, FALSE, FALSE)) {
            resisted = TRUE;
        } else {
            do_stun = FALSE;
            if (youdefend) {
                if (youmonst.data != old_uasmon)
                    *dmgptr = 0; /* rehumanized, so no more damage */
                if (u.uenmax > 0) {
                    u.uenmax--;
                    if (u.uen > 0)
                        u.uen--;
                    context.botl = TRUE;
                    /*KR You("lose magical energy!"); */
                    You("마법 에너지를 잃었다!");
                }
            } else {
                if (mdef->data == &mons[PM_CLAY_GOLEM])
                    mdef->mhp = 1; /* cancelled clay golems will die */
                if (youattack && attacktype(mdef->data, AT_MAGC)) {
                    u.uenmax++;
                    u.uen++;
                    context.botl = TRUE;
                    /*KR You("absorb magical energy!"); */
                    You("마법 에너지를 흡수했다!");
                }
            }
        }
        break;

    case MB_INDEX_SCARE:
        if (youdefend) {
            if (Antimagic) {
                resisted = TRUE;
            } else {
                nomul(-3);
                /*KR multi_reason = "being scared stiff"; */
                multi_reason = "겁에 질려 굳어있는 동안";
                nomovemsg = "";
                if (magr && magr == u.ustuck && sticks(youmonst.data)) {
                    u.ustuck = (struct monst *) 0;
                    /*KR You("release %s!", mon_nam(magr)); */
                    You("깜짝 놀라 %s 놓아버렸다!",
                        append_josa(mon_nam(magr), "을"));
                }
            }
        } else {
            if (rn2(2) && resist(mdef, WEAPON_CLASS, 0, NOTELL))
                resisted = TRUE;
            else
                monflee(mdef, 3, FALSE, (mdef->mhp > *dmgptr));
        }
        if (!resisted)
            do_stun = FALSE;
        break;

    case MB_INDEX_STUN:
        do_stun = TRUE; /* (this is redundant...) */
        break;

    case MB_INDEX_PROBE:
        if (youattack && (mb->spe == 0 || !rn2(3 * abs(mb->spe)))) {
            /*KR pline_The("%s is insightful.", verb); */
            pline("상대의 상태를 꿰뚫어 보았다.");
            /* pre-damage status */
            probe_monster(mdef);
        }
        break;
    }
    /* stun if that was selected and a worse effect didn't occur */
    if (do_stun) {
        if (youdefend)
            make_stunned(((HStun & TIMEOUT) + 3L), FALSE);
        else
            mdef->mstun = 1;
        /* avoid extra stun message below if we used mb_verb["stun"] above */
        if (attack_indx == MB_INDEX_STUN)
            do_stun = FALSE;
    }
    /* lastly, all this magic can be confusing... */
    do_confuse = !rn2(12);
    if (do_confuse) {
        if (youdefend)
            make_confused((HConfusion & TIMEOUT) + 4L, FALSE);
        else
            mdef->mconf = 1;
    }

    /* now give message(s) describing side-effects; Use fakename
       so vtense() won't be fooled by assigned name ending in 's' */
#if 0 /*KR*/
    fakeidx = youdefend ? 1 : 0;
#endif
    if (youattack || youdefend || vis) {
        (void) upstart(hittee); /* capitalize */
        if (resisted) {
            /*KR pline("%s %s!", hittee, vtense(fakename[fakeidx], "resist")); */
            pline("%s 저항했다!", append_josa(hittee, "은"));
            shieldeff(youdefend ? u.ux : mdef->mx,
                      youdefend ? u.uy : mdef->my);
        }
        if ((do_stun || do_confuse) && flags.verbose) {
            char buf[BUFSZ];

            buf[0] = '\0';
#if 0 /*KR: 원본*/
            if (do_stun)
                Strcat(buf, "stunned");
            if (do_stun && do_confuse)
                Strcat(buf, " and ");
            if (do_confuse)
                Strcat(buf, "confused");
            pline("%s %s %s%c", hittee, vtense(fakename[fakeidx], "are"), buf,
                  (do_stun && do_confuse) ? '!' : '.');
#else /*KR: (한국어 호흡에 맞게 3가지 경우의 수로 분리) */
            if (do_stun && do_confuse)
                Strcat(buf, "비틀거리며 혼란스러워한다");
            else if (do_stun)
                Strcat(buf, "비틀거린다");
            else if (do_confuse)
                Strcat(buf, "혼란스러워한다");

            pline("%s %s%s", append_josa(hittee, "은"), buf,
                  (do_stun && do_confuse) ? "!" : ".");
#endif
        }
    }

    return result;
}

/* Function used when someone attacks someone else with an artifact
 * weapon.  Only adds the special (artifact) damage, and returns a 1 if it
 * did something special (in which case the caller won't print the normal
 * hit message).  This should be called once upon every artifact attack;
 * dmgval() no longer takes artifact bonuses into account.  Possible
 * extension: change the killer so that when an orc kills you with
 * Stormbringer it's "killed by Stormbringer" instead of "killed by an orc".
 */
boolean
artifact_hit(magr, mdef, otmp, dmgptr, dieroll)
struct monst *magr, *mdef;
struct obj *otmp;
int *dmgptr;
int dieroll; /* needed for Magicbane and vorpal blades */
{
    boolean youattack = (magr == &youmonst);
    boolean youdefend = (mdef == &youmonst);
    boolean vis = (!youattack && magr && cansee(magr->mx, magr->my))
                  || (!youdefend && cansee(mdef->mx, mdef->my))
                  || (youattack && u.uswallow && mdef == u.ustuck && !Blind);
    boolean realizes_damage;
    const char *wepdesc;
    /*KR static const char you[] = "you"; */
    static const char you[] = "당신";
    char hittee[BUFSZ];

    Strcpy(hittee, youdefend ? you : mon_nam(mdef));

    /* The following takes care of most of the damage, but not all--
     * the exception being for level draining, which is specially
     * handled.  Messages are done in this function, however.
     */
    *dmgptr += spec_dbon(otmp, mdef, *dmgptr);

    if (youattack && youdefend) {
        impossible("attacking yourself with weapon?");
        return FALSE;
    }

    realizes_damage = (youdefend || vis
                       /* feel the effect even if not seen */
                       || (youattack && mdef == u.ustuck));

    /* the four basic attacks: fire, cold, shock and missiles */
    if (attacks(AD_FIRE, otmp)) {
        if (realizes_damage)
#if 0 /*KR: 원본*/
            pline_The("fiery blade %s %s%c",
                      !spec_dbon_applies
                          ? "hits"
                          : (mdef->data == &mons[PM_WATER_ELEMENTAL])
                                ? "vaporizes part of"
                                : "burns",
                      hittee, !spec_dbon_applies ? '.' : '!');
#else       /* 타겟(hittee)이 물 원소이거나 불태워질 때는 '~을/를'이 필요하고,
             * 그냥 명중했을 때는 '~에'가 필요합니다. */
            if (!spec_dbon_applies) {
                pline("불타는 칼날이 %s 명중했다.",
                      append_josa(hittee, "에"));
            } else if (mdef->data == &mons[PM_WATER_ELEMENTAL]) {
                pline("불타는 칼날이 %s 일부를 증발시켰다!",
                      append_josa(hittee, "의"));
            } else {
                pline("불타는 칼날이 %s 불태웠다!",
                      append_josa(hittee, "을"));
            }
#endif
        if (!rn2(4))
            (void) destroy_mitem(mdef, POTION_CLASS, AD_FIRE);
        if (!rn2(4))
            (void) destroy_mitem(mdef, SCROLL_CLASS, AD_FIRE);
        if (!rn2(7))
            (void) destroy_mitem(mdef, SPBOOK_CLASS, AD_FIRE);
        if (youdefend && Slimed)
            burn_away_slime();
        return realizes_damage;
    }
    if (attacks(AD_COLD, otmp)) {
        if (realizes_damage)
#if 0 /*KR: 원본 (냉기 속성) */
            pline_The("ice-cold blade %s %s%c",
                      !spec_dbon_applies ? "hits" : "freezes", hittee,
                      !spec_dbon_applies ? '.' : '!');
#else
            if (!spec_dbon_applies) {
                pline("얼음처럼 차가운 칼날이 %s 명중했다.",
                      append_josa(hittee, "에"));
            } else {
                pline("얼음처럼 차가운 칼날이 %s 꽁꽁 얼려버렸다!",
                      append_josa(hittee, "을"));
            }
#endif
        if (!rn2(4))
            (void) destroy_mitem(mdef, POTION_CLASS, AD_COLD);
        return realizes_damage;
    }
    if (attacks(AD_ELEC, otmp)) {
        if (realizes_damage)
#if 0 /*KR: 원본 (전격 속성) */
            pline_The("massive hammer hits%s %s%c",
                      !spec_dbon_applies ? "" : "!  Lightning strikes",
                      hittee, !spec_dbon_applies ? '.' : '!');
#else
            if (!spec_dbon_applies) {
                pline("거대한 망치가 %s 명중했다.",
                      append_josa(hittee, "에"));
            } else {
                pline("거대한 망치가 %s 명중했다! 번개가 내리쳤다!",
                      append_josa(hittee, "에"));
            }
#endif
        if (spec_dbon_applies)
            wake_nearto(mdef->mx, mdef->my, 4 * 4);
        if (!rn2(5))
            (void) destroy_mitem(mdef, RING_CLASS, AD_ELEC);
        if (!rn2(5))
            (void) destroy_mitem(mdef, WAND_CLASS, AD_ELEC);
        return realizes_damage;
    }
    if (attacks(AD_MAGM, otmp)) {
        if (realizes_damage)
#if 0 /*KR*/
            pline_The("imaginary widget hits%s %s%c",
                      !spec_dbon_applies
                          ? ""
                          : "!  A hail of magic missiles strikes",
                      hittee, !spec_dbon_applies ? '.' : '!');
#else
            pline("실체가 없는 물체가 %s 공격했다%s",
                  append_josa(hittee, "를"),
                  !spec_dbon_applies ? "."
                                     : "! 매직 미사일이 우박처럼 몰아쳤다!");
#endif
        return realizes_damage;
    }

    if (attacks(AD_STUN, otmp) && dieroll <= MB_MAX_DIEROLL) {
        /* Magicbane's special attacks (possibly modifies hittee[]) */
        return Mb_hit(magr, mdef, otmp, dmgptr, dieroll, vis, hittee);
    }

    if (!spec_dbon_applies) {
        /* since damage bonus didn't apply, nothing more to do;
           no further attacks have side-effects on inventory */
        return FALSE;
    }

    /* We really want "on a natural 20" but Nethack does it in */
    /* reverse from AD&D. */
    if (spec_ability(otmp, SPFX_BEHEAD)) {
        if (otmp->oartifact == ART_TSURUGI_OF_MURAMASA && dieroll == 1) {
            /*KR wepdesc = "The razor-sharp blade"; */
            wepdesc = "예리한 칼날";
            /* not really beheading, but so close, why add another SPFX */
            if (youattack && u.uswallow && mdef == u.ustuck) {
                /*KR You("slice %s wide open!", mon_nam(mdef)); */
                You("%s 크게 베어버렸다!", append_josa(mon_nam(mdef), "을"));
                *dmgptr = 2 * mdef->mhp + FATAL_DAMAGE_MODIFIER;
                return TRUE;
            }
            if (!youdefend) {
                /* allow normal cutworm() call to add extra damage */
                if (notonhead)
                    return FALSE;

                if (bigmonst(mdef->data)) {
                    if (youattack)
                        /*KR You("slice deeply into %s!", mon_nam(mdef)); */
                        You("%s 깊숙이 베었다!", append_josa(mon_nam(mdef), "을"));
                    else if (vis)
#if 0 /*KR*/
                        pline("%s cuts deeply into %s!", Monnam(magr),
                              hittee);
#else
                        pline("%s %s 깊숙이 베었다!",
                              append_josa(Monnam(magr), "이"),
                              append_josa(hittee, "을"));
#endif
                    *dmgptr *= 2;
                    return TRUE;
                }
                *dmgptr = 2 * mdef->mhp + FATAL_DAMAGE_MODIFIER;
#if 0 /*KR: 원본*/
                pline("%s cuts %s in half!", wepdesc, mon_nam(mdef));
#else
                pline("%s %s 반토막 내버렸다!", append_josa(wepdesc, "이"),
                      append_josa(mon_nam(mdef), "을"));
#endif
                otmp->dknown = TRUE;
                return TRUE;
            } else {
                if (bigmonst(youmonst.data)) {
#if 0 /*KR: 원본*/
                    pline("%s cuts deeply into you!",
                          magr ? Monnam(magr) : wepdesc);
#else
                    pline("%s 당신을 깊숙이 베었다!",
                          append_josa(magr ? Monnam(magr) : wepdesc, "이"));
#endif
                    *dmgptr *= 2;
                    return TRUE;
                }

                /* Players with negative AC's take less damage instead
                 * of just not getting hit.  We must add a large enough
                 * value to the damage so that this reduction in
                 * damage does not prevent death.
                 */
                *dmgptr = 2 * (Upolyd ? u.mh : u.uhp) + FATAL_DAMAGE_MODIFIER;
                /*KR pline("%s cuts you in half!", wepdesc); */
                pline("%s 당신을 반으로 쪼갰다!", append_josa(wepdesc, "이"));
                otmp->dknown = TRUE;
                return TRUE;
            }
        } else if (otmp->oartifact == ART_VORPAL_BLADE
                   && (dieroll == 1 || mdef->data == &mons[PM_JABBERWOCK])) {
#if 0 /*KR*/
            static const char *const behead_msg[2] = { "%s beheads %s!",
                                                       "%s decapitates %s!" };
#else
            static const char *const behead_msg[2] = { "%s %s 목을 베어버렸다!",
                                                       "%s %s 머리를 날려버렸다!"
            };
#endif

            if (youattack && u.uswallow && mdef == u.ustuck)
                return FALSE;
#if 0 /*KR: 원본*/
            wepdesc = artilist[ART_VORPAL_BLADE].name;
#else /*KR: KRNethack 맞춤 번역 */
            extern char *get_kr_name(const char *);
            wepdesc = get_kr_name(artilist[ART_VORPAL_BLADE].name);
#endif
            if (!youdefend) {
                if (!has_head(mdef->data) || notonhead || u.uswallow) {
#if 0 /*KR: 원본*/
                    if (youattack)
                        pline("Somehow, you miss %s wildly.", mon_nam(mdef));
                    else if (vis)
                        pline("Somehow, %s misses wildly.", mon_nam(magr));
#else 
                    if (youattack)
                        pline("왠지 모르게, 당신의 공격이 %s 크게 빗나갔다.",
                              append_josa(mon_nam(mdef), "을"));
                    else if (vis)
                        pline("왠지 모르게, %s 공격이 크게 빗나갔다.",
                              append_josa(mon_nam(magr), "의"));
#endif
                    *dmgptr = 0;
                    return (boolean) (youattack || vis);
                }
                if (noncorporeal(mdef->data) || amorphous(mdef->data)) {
#if 0 /*KR*/
                    pline("%s slices through %s %s.", wepdesc,
                          s_suffix(mon_nam(mdef)), mbodypart(mdef, NECK));
#else
                    pline("%s %s의 %s 가르고 지나갔다.",
                          append_josa(wepdesc, "이"), mon_nam(mdef),
                          append_josa(mbodypart(mdef, NECK), "을"));
#endif
                    return TRUE;
                }
                *dmgptr = 2 * mdef->mhp + FATAL_DAMAGE_MODIFIER;
#if 0 /*KR: 원본 (조사 처리) */
                pline(behead_msg[rn2(SIZE(behead_msg))], wepdesc,
                      mon_nam(mdef));
#else 
                pline(behead_msg[rn2(SIZE(behead_msg))],
                      append_josa(wepdesc, "이"),
                      append_josa(mon_nam(mdef), "의"));
#endif
                if (Hallucination && !flags.female)
                    /*KR pline("Good job Henry, but that wasn't Anne."); */
                    pline("잘했어 헨리, 하지만 그건 앤이 아니야.");
                otmp->dknown = TRUE;
                return TRUE;
            } else {
                if (!has_head(youmonst.data)) {
#if 0 /*KR: 원본*/
                pline("Somehow, %s misses you wildly.",
                      magr ? mon_nam(magr) : wepdesc);
#else 
                pline("왠지 모르게, %s 당신을 크게 빗나갔다.",
                      append_josa(magr ? mon_nam(magr) : wepdesc, "이"));
#endif
                    *dmgptr = 0;
                    return TRUE;
                }
                if (noncorporeal(youmonst.data) || amorphous(youmonst.data)) {
#if 0 /*KR: 원본*/
                pline("%s slices through your %s.", wepdesc,
                      body_part(NECK));
#else 
                pline("%s 당신의 %s 가르고 지나갔다.", append_josa(wepdesc, "이"),
                      append_josa(body_part(NECK), "을"));
#endif
                    return TRUE;
                }
                *dmgptr = 2 * (Upolyd ? u.mh : u.uhp) + FATAL_DAMAGE_MODIFIER;
                /*KR pline(behead_msg[rn2(SIZE(behead_msg))], wepdesc, "you"); */
                pline(behead_msg[rn2(SIZE(behead_msg))], wepdesc, "당신");
                otmp->dknown = TRUE;
                /* Should amulets fall off? */
                return TRUE;
            }
        }
    }
    if (spec_ability(otmp, SPFX_DRLI)) {
        /* some non-living creatures (golems, vortices) are
           vulnerable to life drain effects */
        /*KR const char *life = nonliving(mdef->data) ? "animating force" : "life"; */
        const char *life = nonliving(mdef->data) ? "원동력" : "생명력";

        if (!youdefend) {
            if (vis) {
                if (otmp->oartifact == ART_STORMBRINGER)
#if 0 /*KR: 원본*/
                    pline_The("%s blade draws the %s from %s!",
                              hcolor(NH_BLACK), life, mon_nam(mdef));
                else
                    pline("%s draws the %s from %s!",
                          The(distant_name(otmp, xname)), life,
                          mon_nam(mdef));
#else 
                    /* hcolor(NH_BLACK) = 번역 사전 '검은색' */
                    pline("%s 칼날이 %s의 %s 흡수했다!", hcolor(NH_BLACK),
                          mon_nam(mdef), append_josa(life, "을"));
                else
                    pline("%s %s의 %s 흡수했다!",
                          append_josa(The(distant_name(otmp, xname)), "이"),
                          mon_nam(mdef), append_josa(life, "을"));
#endif

            }
            if (mdef->m_lev == 0) {
                *dmgptr = 2 * mdef->mhp + FATAL_DAMAGE_MODIFIER;
            } else {
                int drain = monhp_per_lvl(mdef);

                *dmgptr += drain;
                mdef->mhpmax -= drain;
                mdef->m_lev--;
                drain /= 2;
                if (drain)
                    healup(drain, 0, FALSE, FALSE);
            }
            return vis;
        } else { /* youdefend */
            int oldhpmax = u.uhpmax;

            if (Blind)
#if 0 /*KR: 원본*/
                You_feel("an %s drain your %s!",
                         (otmp->oartifact == ART_STORMBRINGER)
                            ? "unholy blade" : "object", life);
#else
                You("어떤 %s 당신의 %s 흡수하는 것을 느꼈다!",
                         append_josa((otmp->oartifact == ART_STORMBRINGER)
                                       ? "불경스러운 칼날" : "물체", "이"),
                         append_josa(life, "을"));
#endif
            else if (otmp->oartifact == ART_STORMBRINGER)
#if 0 /*KR: 원본*/
                pline_The("%s blade drains your %s!", hcolor(NH_BLACK), life);
            else
                pline("%s drains your %s!", The(distant_name(otmp, xname)), life);
            losexp("life drainage");
#else 
                pline("%s 칼날이 당신의 %s 흡수했다!", hcolor(NH_BLACK),
                      append_josa(life, "을"));
            else
                pline("%s 당신의 %s 흡수했다!",
                      append_josa(The(distant_name(otmp, xname)), "이"),
                      append_josa(life, "을"));
            losexp("생명력을 흡수당해서"); /* 유언장에 "~(으)로 죽었다"로 연결됨 */
#endif

            if (magr && magr->mhp < magr->mhpmax) {
                magr->mhp += (oldhpmax - u.uhpmax) / 2;
                if (magr->mhp > magr->mhpmax)
                    magr->mhp = magr->mhpmax;
            }
            return TRUE;
        }
    }
    return FALSE;
}

static NEARDATA const char recharge_type[] = { ALLOW_COUNT, ALL_CLASSES, 0 };
static NEARDATA const char invoke_types[] = { ALL_CLASSES, 0 };
/* #invoke: an "ugly check" filters out most objects */

/* the #invoke command */
int
doinvoke()
{
    struct obj *obj;

    obj = getobj(invoke_types, "invoke");
    if (!obj)
        return 0;
    if (!retouch_object(&obj, FALSE))
        return 1;
    return arti_invoke(obj);
}

STATIC_OVL int
arti_invoke(obj)
struct obj *obj;
{
    register const struct artifact *oart = get_artifact(obj);
    if (!obj) {
        impossible("arti_invoke without obj");
        return 0;
    }
    if (!oart || !oart->inv_prop) {
        if (obj->otyp == CRYSTAL_BALL)
            use_crystal_ball(&obj);
        else
            pline1(nothing_happens);
        return 1;
    }

    if (oart->inv_prop > LAST_PROP) {
        /* It's a special power, not "just" a property */
        if (obj->age > monstermoves) {
            /* the artifact is tired :-) */
#if 0 /*KR:T*/
            You_feel("that %s %s ignoring you.", the(xname(obj)),
                     otense(obj, "are"));
#else
            You("%s 당신을 무시하는 것 같은 기분이 든다.",
                     append_josa(xname(obj), "이"));
#endif
            /* and just got more so; patience is essential... */
            obj->age += (long) d(3, 10);
            return 1;
        }
        obj->age = monstermoves + rnz(100);

        switch (oart->inv_prop) {
        case TAMING: {
            struct obj pseudo;

            pseudo =
                zeroobj; /* neither cursed nor blessed, zero oextra too */
            pseudo.otyp = SCR_TAMING;
            (void) seffects(&pseudo);
            break;
        }
        case HEALING: {
            int healamt = (u.uhpmax + 1 - u.uhp) / 2;
            long creamed = (long) u.ucreamed;

            if (Upolyd)
                healamt = (u.mhmax + 1 - u.mh) / 2;
            if (healamt || Sick || Slimed || Blinded > creamed)
                /*KR You_feel("better."); */
                You("기분이 나아졌다.");
            else
                goto nothing_special;
            if (healamt > 0) {
                if (Upolyd)
                    u.mh += healamt;
                else
                    u.uhp += healamt;
            }
            if (Sick)
                make_sick(0L, (char *) 0, FALSE, SICK_ALL);
            if (Slimed)
                make_slimed(0L, (char *) 0);
            if (Blinded > creamed)
                make_blinded(creamed, FALSE);
            context.botl = TRUE;
            break;
        }
        case ENERGY_BOOST: {
            int epboost = (u.uenmax + 1 - u.uen) / 2;

            if (epboost > 120)
                epboost = 120; /* arbitrary */
            else if (epboost < 12)
                epboost = u.uenmax - u.uen;
            if (epboost) {
                u.uen += epboost;
                context.botl = TRUE;
                /*KR You_feel("re-energized."); */
                You("에너지를 되찾은 기분이 든다.");
            } else
                goto nothing_special;
            break;
        }
        case UNTRAP: {
            if (!untrap(TRUE)) {
                obj->age = 0; /* don't charge for changing their mind */
                return 0;
            }
            break;
        }
        case CHARGE_OBJ: {
            struct obj *otmp = getobj(recharge_type, "charge");
            boolean b_effect;

            if (!otmp) {
                obj->age = 0;
                return 0;
            }
            b_effect = (obj->blessed && (oart->role == Role_switch
                                         || oart->role == NON_PM));
            recharge(otmp, b_effect ? 1 : obj->cursed ? -1 : 0);
            update_inventory();
            break;
        }
        case LEV_TELE:
            level_tele();
            break;
        case CREATE_PORTAL: {
            int i, num_ok_dungeons, last_ok_dungeon = 0;
            d_level newlev;
            extern int n_dgns; /* from dungeon.c */
            winid tmpwin = create_nhwindow(NHW_MENU);
            anything any;

            any = zeroany; /* set all bits to zero */
            start_menu(tmpwin);
            /* use index+1 (cant use 0) as identifier */
            for (i = num_ok_dungeons = 0; i < n_dgns; i++) {
                if (!dungeons[i].dunlev_ureached)
                    continue;
                any.a_int = i + 1;
                add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE,
                         dungeons[i].dname, MENU_UNSELECTED);
                num_ok_dungeons++;
                last_ok_dungeon = i;
            }
            /*KR end_menu(tmpwin, "Open a portal to which dungeon?"); */
            end_menu(tmpwin, "어느 던전으로 향하는 포탈을 여시겠습니까?");
            if (num_ok_dungeons > 1) {
                /* more than one entry; display menu for choices */
                menu_item *selected;
                int n;

                n = select_menu(tmpwin, PICK_ONE, &selected);
                if (n <= 0) {
                    destroy_nhwindow(tmpwin);
                    goto nothing_special;
                }
                i = selected[0].item.a_int - 1;
                free((genericptr_t) selected);
            } else
                i = last_ok_dungeon; /* also first & only OK dungeon */
            destroy_nhwindow(tmpwin);

            /*
             * i is now index into dungeon structure for the new dungeon.
             * Find the closest level in the given dungeon, open
             * a use-once portal to that dungeon and go there.
             * The closest level is either the entry or dunlev_ureached.
             */
            newlev.dnum = i;
            if (dungeons[i].depth_start >= depth(&u.uz))
                newlev.dlevel = dungeons[i].entry_lev;
            else
                newlev.dlevel = dungeons[i].dunlev_ureached;

            if (u.uhave.amulet || In_endgame(&u.uz) || In_endgame(&newlev)
                || newlev.dnum == u.uz.dnum || !next_to_u()) {
                /*KR You_feel("very disoriented for a moment."); */
                You("잠시 방향 감각을 크게 잃은 기분이 든다.");
            } else {
                if (!Blind)
                    /*KR You("are surrounded by a shimmering sphere!"); */
                    You("어른거리는 구체에 둘러싸였다!");
                else
                    /*KR You_feel("weightless for a moment."); */
                    You("잠시 몸이 붕 뜨는 기분이 든다.");
                goto_level(&newlev, FALSE, FALSE, FALSE);
            }
            break;
        }
        case ENLIGHTENING:
            enlightenment(MAGICENLIGHTENMENT, ENL_GAMEINPROGRESS);
            break;
        case CREATE_AMMO: {
            struct obj *otmp = mksobj(ARROW, TRUE, FALSE);

            if (!otmp)
                goto nothing_special;
            otmp->blessed = obj->blessed;
            otmp->cursed = obj->cursed;
            otmp->bknown = obj->bknown;
            if (obj->blessed) {
                if (otmp->spe < 0)
                    otmp->spe = 0;
                otmp->quan += rnd(10);
            } else if (obj->cursed) {
                if (otmp->spe > 0)
                    otmp->spe = 0;
            } else
                otmp->quan += rnd(5);
            otmp->owt = weight(otmp);
#if 0 /*KR: 원본*/
            otmp = hold_another_object(otmp, "Suddenly %s out.",
                                       aobjnam(otmp, "fall"), (char *) 0);
#else
            otmp = hold_another_object(otmp, "갑자기 %s 떨어졌다.",
                                       append_josa(xname(otmp), "이"),
                                       (const char *) 0);
#endif
            nhUse(otmp);
            break;
        }
        }
    } else {
        long eprop = (u.uprops[oart->inv_prop].extrinsic ^= W_ARTI),
             iprop = u.uprops[oart->inv_prop].intrinsic;
        boolean on = (eprop & W_ARTI) != 0; /* true if prop just set */

        if (on && obj->age > monstermoves) {
            /* the artifact is tired :-) */
            u.uprops[oart->inv_prop].extrinsic ^= W_ARTI;
#if 0 /*KR: 원본*/
            You_feel("that %s %s ignoring you.", the(xname(obj)),
                     otense(obj, "are"));
#else
            You("%s 당신을 무시하는 것 같은 기분이 든다.",
                     append_josa(xname(obj), "이"));
#endif
            /* can't just keep repeatedly trying */
            obj->age += (long) d(3, 10);
            return 1;
        } else if (!on) {
            /* when turning off property, determine downtime */
            /* arbitrary for now until we can tune this -dlc */
            obj->age = monstermoves + rnz(100);
        }

        if ((eprop & ~W_ARTI) || iprop) {
 nothing_special:
            /* you had the property from some other source too */
            if (carried(obj))
                /*KR You_feel("a surge of power, but nothing seems to happen."); */
                You("힘이 솟구치는 것을 느꼈지만, 아무 일도 일어나지 않은 것 같다.");
            return 1;
        }
        switch (oart->inv_prop) {
        case CONFLICT:
            if (on)
                /*KR You_feel("like a rabble-rouser."); */
                You("선동가가 된 것 같은 기분이 든다.");
            else
                /*KR You_feel("the tension decrease around you.");*/
                You("주변의 긴장감이 줄어든 것을 느낀다.");
            break;
        case LEVITATION:
            if (on) {
                float_up();
                spoteffects(FALSE);
            } else
                (void) float_down(I_SPECIAL | TIMEOUT, W_ARTI);
            break;
        case INVIS:
            if (BInvis || Blind)
                goto nothing_special;
            newsym(u.ux, u.uy);
            if (on)
#if 0 /*KR: 원본*/
                Your("body takes on a %s transparency...",
                     Hallucination ? "normal" : "strange");
            else
                Your("body seems to unfade...");
            break;
#else
                pline("당신의 몸이 %s 투명해진다...",
                      Hallucination ? "평범하게" : "이상하게");
            else
                pline("당신의 몸이 다시 불투명해지는 것 같다...");
            break;
#endif
        }
    }

    return 1;
}

/* will freeing this object from inventory cause levitation to end? */
boolean
finesse_ahriman(obj)
struct obj *obj;
{
    const struct artifact *oart;
    struct prop save_Lev;
    boolean result;

    /* if we aren't levitating or this isn't an artifact which confers
       levitation via #invoke then freeinv() won't toggle levitation */
    if (!Levitation || (oart = get_artifact(obj)) == 0
        || oart->inv_prop != LEVITATION || !(ELevitation & W_ARTI))
        return FALSE;

    /* arti_invoke(off) -> float_down() clears I_SPECIAL|TIMEOUT & W_ARTI;
       probe ahead to see whether that actually results in floating down;
       (this assumes that there aren't two simultaneously invoked artifacts
       both conferring levitation--safe, since if there were two of them,
       invoking the 2nd would negate the 1st rather than stack with it) */
    save_Lev = u.uprops[LEVITATION];
    HLevitation &= ~(I_SPECIAL | TIMEOUT);
    ELevitation &= ~W_ARTI;
    result = (boolean) !Levitation;
    u.uprops[LEVITATION] = save_Lev;
    return result;
}

/* WAC return TRUE if artifact is always lit */
boolean
artifact_light(obj)
struct obj *obj;
{
    return (boolean) (get_artifact(obj) && obj->oartifact == ART_SUNSWORD);
}

/* KMH -- Talking artifacts are finally implemented */
void
arti_speak(obj)
struct obj *obj;
{
    register const struct artifact *oart = get_artifact(obj);
    const char *line;
    char buf[BUFSZ];

    /* Is this a speaking artifact? */
    if (!oart || !(oart->spfx & SPFX_SPEAK))
        return;

    line = getrumor(bcsign(obj), buf, TRUE);
    if (!*line)
        /*KR line = "NetHack rumors file closed for renovation."; */
        line = "넷핵 소문 파일은 새단장을 위해 잠시 문을 닫습니다.";
    /*KR pline("%s:", Tobjnam(obj, "whisper")); */
    pline("%s 속삭였다:", append_josa(xname(obj), "이"));
    verbalize1(line);
    return;
}

boolean
artifact_has_invprop(otmp, inv_prop)
struct obj *otmp;
uchar inv_prop;
{
    const struct artifact *arti = get_artifact(otmp);

    return (boolean) (arti && (arti->inv_prop == inv_prop));
}

/* Return the price sold to the hero of a given artifact or unique item */
long
arti_cost(otmp)
struct obj *otmp;
{
    if (!otmp->oartifact)
        return (long) objects[otmp->otyp].oc_cost;
    else if (artilist[(int) otmp->oartifact].cost)
        return artilist[(int) otmp->oartifact].cost;
    else
        return (100L * (long) objects[otmp->otyp].oc_cost);
}

STATIC_OVL uchar
abil_to_adtyp(abil)
long *abil;
{
    struct abil2adtyp_tag {
        long *abil;
        uchar adtyp;
    } abil2adtyp[] = {
        { &EFire_resistance, AD_FIRE },
        { &ECold_resistance, AD_COLD },
        { &EShock_resistance, AD_ELEC },
        { &EAntimagic, AD_MAGM },
        { &EDisint_resistance, AD_DISN },
        { &EPoison_resistance, AD_DRST },
        { &EDrain_resistance, AD_DRLI },
    };
    int k;

    for (k = 0; k < SIZE(abil2adtyp); k++) {
        if (abil2adtyp[k].abil == abil)
            return abil2adtyp[k].adtyp;
    }
    return 0;
}

STATIC_OVL unsigned long
abil_to_spfx(abil)
long *abil;
{
    static const struct abil2spfx_tag {
        long *abil;
        unsigned long spfx;
    } abil2spfx[] = {
        { &ESearching, SPFX_SEARCH },
        { &EHalluc_resistance, SPFX_HALRES },
        { &ETelepat, SPFX_ESP },
        { &EStealth, SPFX_STLTH },
        { &ERegeneration, SPFX_REGEN },
        { &ETeleport_control, SPFX_TCTRL },
        { &EWarn_of_mon, SPFX_WARN },
        { &EWarning, SPFX_WARN },
        { &EEnergy_regeneration, SPFX_EREGEN },
        { &EHalf_spell_damage, SPFX_HSPDAM },
        { &EHalf_physical_damage, SPFX_HPHDAM },
        { &EReflecting, SPFX_REFLECT },
    };
    int k;

    for (k = 0; k < SIZE(abil2spfx); k++) {
        if (abil2spfx[k].abil == abil)
            return abil2spfx[k].spfx;
    }
    return 0L;
}

/*
 * Return the first item that is conveying a particular intrinsic.
 */
struct obj *
what_gives(abil)
long *abil;
{
    struct obj *obj;
    uchar dtyp;
    unsigned long spfx;
    long wornbits;
    long wornmask = (W_ARM | W_ARMC | W_ARMH | W_ARMS
                     | W_ARMG | W_ARMF | W_ARMU
                     | W_AMUL | W_RINGL | W_RINGR | W_TOOL
                     | W_ART | W_ARTI);

    if (u.twoweap)
        wornmask |= W_SWAPWEP;
    dtyp = abil_to_adtyp(abil);
    spfx = abil_to_spfx(abil);
    wornbits = (wornmask & *abil);

    for (obj = invent; obj; obj = obj->nobj) {
        if (obj->oartifact
            && (abil != &EWarn_of_mon || context.warntype.obj)) {
            const struct artifact *art = get_artifact(obj);

            if (art) {
                if (dtyp) {
                    if (art->cary.adtyp == dtyp /* carried */
                        || (art->defn.adtyp == dtyp /* defends while worn */
                            && (obj->owornmask & ~(W_ART | W_ARTI))))
                        return obj;
                }
                if (spfx) {
                    /* property conferred when carried */
                    if ((art->cspfx & spfx) == spfx)
                        return obj;
                    /* property conferred when wielded or worn */
                    if ((art->spfx & spfx) == spfx && obj->owornmask)
                        return obj;
                }
            }
        } else {
            if (wornbits && wornbits == (wornmask & obj->owornmask))
                return obj;
        }
    }
    return (struct obj *) 0;
}

const char *
glow_color(arti_indx)
int arti_indx;
{
    int colornum = artilist[arti_indx].acolor;
#if 0 /*KR: 원본*/
    const char *colorstr = clr2colorname(colornum);

    return hcolor(colorstr);
#else /* hcolor() 대신 get_kr_name()을 통해 색상을 한국어로 바꿉니다. */
    const char *colorstr = clr2colorname(colornum);
    
    return get_kr_name(colorstr);
#endif
}

/* glow verb; [0] holds the value used when blind */
static const char *glow_verbs[] = {
    /*KR "quiver", "flicker", "glimmer", "gleam" */
    "떨림", "깜빡임", "작은 빛", "강한 빛"
};

/* relative strength that Sting is glowing (0..3), to select verb */
STATIC_OVL int
glow_strength(count)
int count;
{
    /* glow strength should also be proportional to proximity and
       probably difficulty, but we don't have that information and
       gathering it is more trouble than this would be worth */
    return (count > 12) ? 3 : (count > 4) ? 2 : (count > 0);
}

const char *
glow_verb(count, ingsfx)
int count; /* 0 means blind rather than no applicable creatures */
boolean ingsfx;
{
    static char resbuf[20];

    Strcpy(resbuf, glow_verbs[glow_strength(count)]);
    /* ing_suffix() will double the last consonant for all the words
       we're using and none of them should have that, so bypass it */
#if 0 /*KR: 원본 (영어 진행형 접미사 ing). 한국어에는 불필요 */
    if (ingsfx)
        Strcat(resbuf, "ing");
#endif
    return resbuf;
}

/* use for warning "glow" for Sting, Orcrist, and Grimtooth */
void
Sting_effects(orc_count)
int orc_count; /* new count (warn_obj_cnt is old count); -1 is a flag value */
{
    if (uwep
        && (uwep->oartifact == ART_STING
            || uwep->oartifact == ART_ORCRIST
            || uwep->oartifact == ART_GRIMTOOTH)) {
        int oldstr = glow_strength(warn_obj_cnt),
            newstr = glow_strength(orc_count);

        if (orc_count == -1 && warn_obj_cnt > 0) {
            /* -1 means that blindness has just been toggled; give a
               'continue' message that eventual 'stop' message will match */
#if 0 /*KR: 원본*/
            pline("%s is %s.", bare_artifactname(uwep),
                  glow_verb(Blind ? 0 : warn_obj_cnt, TRUE));
#else /*KR: KRNethack 맞춤 번역*/
            pline("%s에서 %s이 느껴진다.", bare_artifactname(uwep),
                  glow_verb(Blind ? 0 : warn_obj_cnt, TRUE));
#endif
        } else if (newstr > 0 && newstr != oldstr) {
            /* 'start' message */
            if (!Blind)
#if 0 /*KR: 원본*/
                pline("%s %s %s%c", bare_artifactname(uwep),
                      otense(uwep, glow_verb(orc_count, FALSE)),
                      glow_color(uwep->oartifact),
                      (newstr > oldstr) ? '!' : '.');
#else /*KR: KRNethack 맞춤 번역*/
                pline("%s %s %s이 뿜어져 나온다%s",
                      append_josa(bare_artifactname(uwep), "에서"),
                      glow_color(uwep->oartifact), /* 예: "파란색" */
                      glow_verb(orc_count, FALSE), /* 예: "강한 빛" */
                      (newstr > oldstr) ? "!" : ".");
#endif
            else if (oldstr == 0) /* quivers */
#if 0 /*KR: 원본*/
                pline("%s %s slightly.", bare_artifactname(uwep),
                      otense(uwep, glow_verb(0, FALSE)));
        } else if (orc_count == 0 && warn_obj_cnt > 0) {
            /* 'stop' message */
            pline("%s stops %s.", bare_artifactname(uwep),
                  glow_verb(Blind ? 0 : warn_obj_cnt, TRUE));
#else                             
                pline("%s 미세하게 떨린다.",
                      append_josa(bare_artifactname(uwep), "이"));
        } else if (orc_count == 0 && warn_obj_cnt > 0) {
            /* 'stop' message */
            pline("%s의 %s이 멈췄다.", bare_artifactname(uwep),
                  glow_verb(Blind ? 0 : warn_obj_cnt, TRUE));
#endif
        }
    }
}

/* called when hero is wielding/applying/invoking a carried item, or
   after undergoing a transformation (alignment change, lycanthropy,
   polymorph) which might affect item access */
int
retouch_object(objp, loseit)
struct obj **objp; /* might be destroyed or unintentionally dropped */
boolean loseit;    /* whether to drop it if hero can longer touch it */
{
    struct obj *obj = *objp;

    if (touch_artifact(obj, &youmonst)) {
        char buf[BUFSZ];
        int dmg = 0, tmp;
        boolean ag = (objects[obj->otyp].oc_material == SILVER && Hate_silver),
                bane = bane_applies(get_artifact(obj), &youmonst);

        /* nothing else to do if hero can successfully handle this object */
        if (!ag && !bane)
            return 1;

        /* hero can't handle this object, but didn't get touch_artifact()'s
           "<obj> evades your grasp|control" message; give an alternate one */
#if 0 /*KR*/
        You_cant("handle %s%s!", yname(obj),
                 obj->owornmask ? " anymore" : "");
#else
        You_cant("%s%s 다룰 수 없다!", obj->owornmask ? "더 이상 " : "",
                 append_josa(yname(obj), "를"));
#endif
        /* also inflict damage unless touch_artifact() already did so */
        if (!touch_blasted) {
            /* damage is somewhat arbitrary; half the usual 1d20 physical
               for silver, 1d10 magical for <foo>bane, potentially both */
            if (ag)
                tmp = rnd(10), dmg += Maybe_Half_Phys(tmp);
            if (bane)
                dmg += rnd(10);
            /*KR Sprintf(buf, "handling %s", killer_xname(obj)); */
            Sprintf(buf, "%s 다루다가", append_josa(killer_xname(obj), "를"));
            losehp(dmg, buf, KILLED_BY);
            exercise(A_CON, FALSE);
        }
    }

    /* removing a worn item might result in loss of levitation,
       dropping the hero onto a polymorph trap or into water or
       lava and potentially dropping or destroying the item */
    if (obj->owornmask) {
        struct obj *otmp;

        remove_worn_item(obj, FALSE);
        for (otmp = invent; otmp; otmp = otmp->nobj)
            if (otmp == obj)
                break;
        if (!otmp)
            *objp = obj = 0;
    }

    /* if we still have it and caller wants us to drop it, do so now */
    if (loseit && obj) {
        if (Levitation) {
            freeinv(obj);
            hitfloor(obj, TRUE);
        } else {
            /* dropx gives a message iff item lands on an altar */
            if (!IS_ALTAR(levl[u.ux][u.uy].typ))
#if 0 /*KR*/
                pline("%s to the %s.", Tobjnam(obj, "fall"),
                      surface(u.ux, u.uy));
#else
                pline("%s %s에 떨어졌다.", append_josa(xname(obj), "이"),
                      surface(u.ux, u.uy));
#endif
            dropx(obj);
        }
        *objp = obj = 0; /* no longer in inventory */
    }
    return 0;
}

/* an item which is worn/wielded or an artifact which conveys
   something via being carried or which has an #invoke effect
   currently in operation undergoes a touch test; if it fails,
   it will be unworn/unwielded and revoked but not dropped */
STATIC_OVL boolean
untouchable(obj, drop_untouchable)
struct obj *obj;
boolean drop_untouchable;
{
    struct artifact *art;
    boolean beingworn, carryeffect, invoked;
    long wearmask = ~(W_QUIVER | (u.twoweap ? 0L : W_SWAPWEP) | W_BALL);

    beingworn = ((obj->owornmask & wearmask) != 0L
                 /* some items in use don't have any wornmask setting */
                 || (obj->oclass == TOOL_CLASS
                     && (obj->lamplit || (obj->otyp == LEASH && obj->leashmon)
                         || (Is_container(obj) && Has_contents(obj)))));

    if ((art = get_artifact(obj)) != 0) {
        carryeffect = (art->cary.adtyp || art->cspfx);
        invoked = (art->inv_prop > 0 && art->inv_prop <= LAST_PROP
                   && (u.uprops[art->inv_prop].extrinsic & W_ARTI) != 0L);
    } else {
        carryeffect = invoked = FALSE;
    }

    if (beingworn || carryeffect || invoked) {
        if (!retouch_object(&obj, drop_untouchable)) {
            /* "<artifact> is beyond your control" or "you can't handle
               <object>" has been given and it is now unworn/unwielded
               and possibly dropped (depending upon caller); if dropped,
               carried effect was turned off, else we leave that alone;
               we turn off invocation property here if still carried */
            if (invoked && obj)
                arti_invoke(obj); /* reverse #invoke */
            return TRUE;
        }
    }
    return FALSE;
}

/* check all items currently in use (mostly worn) for touchability */
void
retouch_equipment(dropflag)
int dropflag; /* 0==don't drop, 1==drop all, 2==drop weapon */
{
    static int nesting = 0; /* recursion control */
    struct obj *obj;
    boolean dropit, had_gloves = (uarmg != 0);
    int had_rings = (!!uleft + !!uright);

    /*
     * We can potentially be called recursively if losing/unwearing
     * an item causes worn helm of opposite alignment to come off or
     * be destroyed.
     *
     * BUG: if the initial call was due to putting on a helm of
     * opposite alignment and it does come off to trigger recursion,
     * after the inner call executes, the outer call will finish
     * using the non-helm alignment rather than the helm alignment
     * which triggered this in the first place.
     */
    if (!nesting++)
        clear_bypasses(); /* init upon initial entry */

    dropit = (dropflag > 0); /* drop all or drop weapon */
    /* check secondary weapon first, before possibly unwielding primary */
    if (u.twoweap) {
        bypass_obj(uswapwep); /* so loop below won't process it again */
        (void) untouchable(uswapwep, dropit);
    }
    /* check primary weapon next so that they're handled together */
    if (uwep) {
        bypass_obj(uwep); /* so loop below won't process it again */
        (void) untouchable(uwep, dropit);
    }

    /* in case someone is daft enough to add artifact or silver saddle */
    if (u.usteed && (obj = which_armor(u.usteed, W_SADDLE)) != 0) {
        /* untouchable() calls retouch_object() which expects an object in
           hero's inventory, but remove_worn_item() will be harmless for
           saddle and we're suppressing drop, so this works as intended */
        if (untouchable(obj, FALSE))
            dismount_steed(DISMOUNT_THROWN);
    }
    /*
     * TODO?  Force off gloves if either or both rings are going to
     * become unworn; force off cloak [suit] before suit [shirt].
     * The torso handling is hypothetical; the case for gloves is
     * not, due to the possibility of unwearing silver rings.
     */

    dropit = (dropflag == 1); /* all untouchable items */
    /* loss of levitation (silver ring, or Heart of Ahriman invocation)
       might cause hero to lose inventory items (by dropping into lava,
       for instance), so inventory traversal needs to rescan the whole
       invent chain each time it moves on to another object; we use bypass
       handling to keep track of which items have already been processed */
    while ((obj = nxt_unbypassed_obj(invent)) != 0)
        (void) untouchable(obj, dropit);

    if (had_rings != (!!uleft + !!uright) && uarmg && uarmg->cursed)
        uncurse(uarmg); /* temporary? hack for ring removal plausibility */
    if (had_gloves && !uarmg)
        /*KR selftouch("After losing your gloves, you"); */
        selftouch("장갑을 잃어버린 후, 당신은");

    if (!--nesting)
        clear_bypasses(); /* reset upon final exit */
}

static int mkot_trap_warn_count = 0;

STATIC_OVL int
count_surround_traps(x, y)
int x, y;
{
    struct rm *levp;
    struct obj *otmp;
    struct trap *ttmp;
    int dx, dy, glyph, ret = 0;

    for (dx = x - 1; dx < x + 2; ++dx)
        for (dy = y - 1; dy < y + 2; ++dy) {
            if (!isok(dx, dy))
                continue;
            /* If a trap is shown here, don't count it; the hero
             * should be expecting it.  But if there is a trap here
             * that's not shown, either undiscovered or covered by
             * something, do count it.
             */
            glyph = glyph_at(dx, dy);
            if (glyph_is_trap(glyph))
                continue;
            if ((ttmp = t_at(dx, dy)) != 0) {
                ++ret;
                continue;
            }
            levp = &levl[dx][dy];
            if (IS_DOOR(levp->typ) && (levp->doormask & D_TRAPPED) != 0) {
                ++ret;
                continue;
            }
            for (otmp = level.objects[dx][dy]; otmp; otmp = otmp->nexthere)
                if (Is_container(otmp) && otmp->otrapped) {
                    ++ret; /* we're counting locations, so just */
                    break; /* count the first one in a pile     */
                }
        }
    /*
     * [Shouldn't we also check inventory for a trapped container?
     * Even if its trap has already been found, there's no 'tknown'
     * flag to help hero remember that so we have nothing comparable
     * to a shown glyph to justify skipping it.]
     */
    return ret;
}

/* sense adjacent traps if wielding MKoT without wearing gloves */
void
mkot_trap_warn()
{
#if 0 /*KR*/
    static const char *const heat[7] = {
        "cool", "slightly warm", "warm", "very warm",
        "hot", "very hot", "like fire"
    };
#else
    static const char *const heat[7] = { 
        "싸늘하게", "살짝 따뜻하게", "따뜻하게", "매우 따뜻하게",
        "뜨겁게", "매우 뜨겁게", "불타는 것처럼" 
    };
#endif

    if (!uarmg && uwep && uwep->oartifact == ART_MASTER_KEY_OF_THIEVERY) {
        int idx, ntraps = count_surround_traps(u.ux, u.uy);

        if (ntraps != mkot_trap_warn_count) {
            idx = min(ntraps, SIZE(heat) - 1);
            /*KR pline_The("Key feels %s%c", heat[idx], (ntraps > 3) ? '!' : '.'); */
            pline("열쇠가 %s 느껴진다%s", heat[idx], (ntraps > 3) ? '!' : '.');
        }
        mkot_trap_warn_count = ntraps;
    } else
        mkot_trap_warn_count = 0;
}

/* Master Key is magic key if its bless/curse state meets our criteria:
   not cursed for rogues or blessed for non-rogues */
boolean
is_magic_key(mon, obj)
struct monst *mon; /* if null, non-rogue is assumed */
struct obj *obj;
{
    if (((obj && obj->oartifact == ART_MASTER_KEY_OF_THIEVERY)
         && ((mon == &youmonst) ? Role_if(PM_ROGUE)
                                : (mon && mon->data == &mons[PM_ROGUE])))
        ? !obj->cursed : obj->blessed)
        return TRUE;
    return FALSE;
}

/* figure out whether 'mon' (usually youmonst) is carrying the magic key */
struct obj *
has_magic_key(mon)
struct monst *mon; /* if null, hero assumed */
{
    struct obj *o;
    short key = artilist[ART_MASTER_KEY_OF_THIEVERY].otyp;

    if (!mon)
        mon = &youmonst;
    for (o = ((mon == &youmonst) ? invent : mon->minvent); o;
         o = nxtobj(o, key, FALSE)) {
        if (is_magic_key(mon, o))
            return o;
    }
    return (struct obj *) 0;
}

/*artifact.c*/
