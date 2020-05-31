/* NetHack 3.6	eat.c	$NHDT-Date: 1574900825 2019/11/28 00:27:05 $  $NHDT-Branch: NetHack-3.6 $:$NHDT-Revision: 1.206 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2012. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

STATIC_PTR int NDECL(eatmdone);
STATIC_PTR int NDECL(eatfood);
STATIC_PTR struct obj *FDECL(costly_tin, (int));
STATIC_PTR int NDECL(opentin);
STATIC_PTR int NDECL(unfaint);

STATIC_DCL const char *FDECL(food_xname, (struct obj *, BOOLEAN_P));
STATIC_DCL void FDECL(choke, (struct obj *));
STATIC_DCL void NDECL(recalc_wt);
STATIC_DCL unsigned FDECL(obj_nutrition, (struct obj *));
STATIC_DCL struct obj *FDECL(touchfood, (struct obj *));
STATIC_DCL void NDECL(do_reset_eat);
STATIC_DCL void FDECL(done_eating, (BOOLEAN_P));
STATIC_DCL void FDECL(cprefx, (int));
STATIC_DCL int FDECL(intrinsic_possible, (int, struct permonst *));
STATIC_DCL void FDECL(givit, (int, struct permonst *));
STATIC_DCL void FDECL(cpostfx, (int));
STATIC_DCL void FDECL(consume_tin, (const char *));
STATIC_DCL void FDECL(start_tin, (struct obj *));
STATIC_DCL int FDECL(eatcorpse, (struct obj *));
STATIC_DCL void FDECL(start_eating, (struct obj *, BOOLEAN_P));
STATIC_DCL void FDECL(fprefx, (struct obj *));
STATIC_DCL void FDECL(fpostfx, (struct obj *));
STATIC_DCL int NDECL(bite);
STATIC_DCL int FDECL(edibility_prompts, (struct obj *));
STATIC_DCL int FDECL(rottenfood, (struct obj *));
STATIC_DCL void NDECL(eatspecial);
STATIC_DCL int FDECL(bounded_increase, (int, int, int));
STATIC_DCL void FDECL(accessory_has_effect, (struct obj *));
STATIC_DCL void FDECL(eataccessory, (struct obj *));
STATIC_DCL const char *FDECL(foodword, (struct obj *));
STATIC_DCL int FDECL(tin_variety, (struct obj *, BOOLEAN_P));
STATIC_DCL boolean FDECL(maybe_cannibal, (int, BOOLEAN_P));

char msgbuf[BUFSZ];

/* also used to see if you're allowed to eat cats and dogs */
#define CANNIBAL_ALLOWED() (Role_if(PM_CAVEMAN) || Race_if(PM_ORC))

/* monster types that cause hero to be turned into stone if eaten */
#define flesh_petrifies(pm) (touch_petrifies(pm) || (pm) == &mons[PM_MEDUSA])

/* Rider corpses are treated as non-rotting so that attempting to eat one
   will be sure to reach the stage of eating where that meal is fatal */
#define nonrotting_corpse(mnum) \
    ((mnum) == PM_LIZARD || (mnum) == PM_LICHEN || is_rider(&mons[mnum]))

/* non-rotting non-corpses; unlike lizard corpses, these items will behave
   as if rotten if they are cursed (fortune cookies handled elsewhere) */
#define nonrotting_food(otyp) \
    ((otyp) == LEMBAS_WAFER || (otyp) == CRAM_RATION)

STATIC_OVL NEARDATA const char comestibles[] = { FOOD_CLASS, 0 };
STATIC_OVL NEARDATA const char offerfodder[] = { FOOD_CLASS, AMULET_CLASS,
                                                 0 };

/* Gold must come first for getobj(). */
STATIC_OVL NEARDATA const char allobj[] = {
    COIN_CLASS,   WEAPON_CLASS, ARMOR_CLASS,  POTION_CLASS,
    SCROLL_CLASS, WAND_CLASS,   RING_CLASS,   AMULET_CLASS,
    FOOD_CLASS,   TOOL_CLASS,   GEM_CLASS,    ROCK_CLASS,
    BALL_CLASS,   CHAIN_CLASS,  SPBOOK_CLASS, 0
};

STATIC_OVL boolean force_save_hs = FALSE;

/* see hunger states in hack.h - texts used on bottom line */
#if 0 /*KR:T*/
const char *hu_stat[] = { "Satiated", "        ", "Hungry  ", "Weak    ",
                          "Fainting", "Fainted ", "Starved " };
#else
const char* hu_stat[] = { "배부름", "        ", "배고픔", "기운없음",
                          "어지러움", "기절함", "굶주림" };
#endif

/*
 * Decide whether a particular object can be eaten by the possibly
 * polymorphed character.  Not used for monster checks.
 */
boolean
is_edible(obj)
register struct obj *obj;
{
    /* protect invocation tools but not Rider corpses (handled elsewhere)*/
    /* if (obj->oclass != FOOD_CLASS && obj_resists(obj, 0, 0)) */
    if (objects[obj->otyp].oc_unique)
        return FALSE;
    /* above also prevents the Amulet from being eaten, so we must never
       allow fake amulets to be eaten either [which is already the case] */

    if (metallivorous(youmonst.data) && is_metallic(obj)
        && (youmonst.data != &mons[PM_RUST_MONSTER] || is_rustprone(obj)))
        return TRUE;

    /* Ghouls only eat non-veggy corpses or eggs (see dogfood()) */
    if (u.umonnum == PM_GHOUL)
        return (boolean)((obj->otyp == CORPSE
                          && !vegan(&mons[obj->corpsenm]))
                         || (obj->otyp == EGG));

    if (u.umonnum == PM_GELATINOUS_CUBE && is_organic(obj)
        /* [g.cubes can eat containers and retain all contents
            as engulfed items, but poly'd player can't do that] */
        && !Has_contents(obj))
        return TRUE;

    /* return (boolean) !!index(comestibles, obj->oclass); */
    return (boolean) (obj->oclass == FOOD_CLASS);
}

/* used for hero init, life saving (if choking), and prayer results of fix
   starving, fix weak from hunger, or golden glow boon (if u.uhunger < 900) */
void
init_uhunger()
{
    context.botl = (u.uhs != NOT_HUNGRY || ATEMP(A_STR) < 0);
    u.uhunger = 900;
    u.uhs = NOT_HUNGRY;
    if (ATEMP(A_STR) < 0) {
        ATEMP(A_STR) = 0;
        (void) encumber_msg();
    }
}

/* tin types [SPINACH_TIN = -1, overrides corpsenm, nut==600] */
static const struct {
    const char* txt;                      /* description */
    int nut;                              /* nutrition */
    Bitfield(fodder, 1);                  /* stocked by health food shops */
    Bitfield(greasy, 1);                  /* causes slippery fingers */
#if 0 /*KR:T*/
} tintxts[] = { { "rotten", -50, 0, 0 },  /* ROTTEN_TIN = 0 */
                { "homemade", 50, 1, 0 }, /* HOMEMADE_TIN = 1 */
                { "soup made from", 20, 1, 0 },
                { "french fried", 40, 0, 1 },
                { "pickled", 40, 1, 0 },
                { "boiled", 50, 1, 0 },
                { "smoked", 50, 1, 0 },
                { "dried", 55, 1, 0 },
                { "deep fried", 60, 0, 1 },
                { "szechuan", 70, 1, 0 },
                { "broiled", 80, 0, 0 },
                { "stir fried", 80, 0, 1 },
                { "sauteed", 95, 0, 0 },
                { "candied", 100, 1, 0 },
                { "pureed", 500, 1, 0 },
                { "", 0, 0, 0 } };
#else
} tintxts[] = { { "썩은", -50, 0, 0 },  /* ROTTEN_TIN = 0 */
                { "수제", 50, 1, 0 }, /* HOMEMADE_TIN = 1 */
                { "의 수프", 20, 1, 0 },
                { "의 튀김", 40, 0, 1 },
                { "절임", 40, 1, 0 },
                { "삶은", 50, 1, 0 },
                { "의 훈제", 50, 1, 0 },
                { "건조", 55, 1, 0 },
                { "튀김", 60, 0, 1 },
                { "사천풍", 70, 1, 0 },
                { "석쇠구이", 80, 0, 0 },
                { "볶음", 80, 0, 1 },
                { "볶음", 95, 0, 0 },
                { "설탕 조림", 100, 1, 0 }, /*맛탕은 일본식 표현*/
                { "의 퓨레", 500, 1, 0 },
                { "", 0, 0, 0 } };
#endif
#define TTSZ SIZE(tintxts)

static char *eatmbuf = 0; /* set by cpostfx() */

/* called after mimicing is over */
STATIC_PTR int
eatmdone(VOID_ARGS)
{
    /* release `eatmbuf' */
    if (eatmbuf) {
        if (nomovemsg == eatmbuf)
            nomovemsg = 0;
        free((genericptr_t) eatmbuf), eatmbuf = 0;
    }
    /* update display */
    if (U_AP_TYPE) {
        youmonst.m_ap_type = M_AP_NOTHING;
        newsym(u.ux, u.uy);
    }
    return 0;
}

/* called when hallucination is toggled */
void
eatmupdate()
{
    const char *altmsg = 0;
    int altapp = 0; /* lint suppression */

    if (!eatmbuf || nomovemsg != eatmbuf)
        return;

    if (is_obj_mappear(&youmonst,ORANGE) && !Hallucination) {
        /* revert from hallucinatory to "normal" mimicking */
        /*KR altmsg = "You now prefer mimicking yourself."; */
        altmsg = "당신은 이제 스스로를 흉내내기로 했다.";
        altapp = GOLD_PIECE;
    } else if (is_obj_mappear(&youmonst,GOLD_PIECE) && Hallucination) {
        /* won't happen; anything which might make immobilized
           hero begin hallucinating (black light attack, theft
           of Grayswandir) will terminate the mimicry first */
        /*KR altmsg = "Your rind escaped intact."; */
        altmsg = "당신의 가죽이 그 상태 그대로 도망쳤다.";
        altapp = ORANGE;
    }

    if (altmsg) {
        /* replace end-of-mimicking message */
        if (strlen(altmsg) > strlen(eatmbuf)) {
            free((genericptr_t) eatmbuf);
            eatmbuf = (char *) alloc(strlen(altmsg) + 1);
        }
        nomovemsg = strcpy(eatmbuf, altmsg);
        /* update current image */
        youmonst.mappearance = altapp;
        newsym(u.ux, u.uy);
    }
}

/* ``[the(] singular(food, xname) [)]'' */
STATIC_OVL const char *
food_xname(food, the_pfx)
struct obj *food;
boolean the_pfx;
{
    const char *result;

    if (food->otyp == CORPSE) {
        result = corpse_xname(food, (const char *) 0,
                              CXN_SINGULAR | (the_pfx ? CXN_PFX_THE : 0));
        /* not strictly needed since pname values are capitalized
           and the() is a no-op for them */
        if (type_is_pname(&mons[food->corpsenm]))
            the_pfx = FALSE;
    } else {
        /* the ordinary case */
        result = singular(food, xname);
    }
    if (the_pfx)
        result = the(result);
    return result;
}

/* Created by GAN 01/28/87
 * Amended by AKP 09/22/87: if not hard, don't choke, just vomit.
 * Amended by 3.  06/12/89: if not hard, sometimes choke anyway, to keep risk.
 *                11/10/89: if hard, rarely vomit anyway, for slim chance.
 *
 * To a full belly all food is bad. (It.)
 */
STATIC_OVL void
choke(food)
struct obj *food;
{
    /* only happens if you were satiated */
    if (u.uhs != SATIATED) {
        if (!food || food->otyp != AMULET_OF_STRANGULATION)
            return;
    } else if (Role_if(PM_KNIGHT) && u.ualign.type == A_LAWFUL) {
        adjalign(-1); /* gluttony is unchivalrous */
        /*KR You_feel("like a glutton!"); */
        You("대식가가 된 기분이 들었다!");
    }

    exercise(A_CON, FALSE);

    if (Breathless || (!Strangled && !rn2(20))) {
        /* choking by eating AoS doesn't involve stuffing yourself */
        if (food && food->otyp == AMULET_OF_STRANGULATION) {
            /*KR You("choke, but recover your composure."); */
            You("목이 막혔다. 하지만 곧 자세를 다잡았다.");
            return;
        }
        /*KR You("stuff yourself and then vomit voluminously."); */
        pline("음식을 잔뜩 먹었지만, 전부 토해버리고 말았다.");
        morehungry(1000); /* you just got *very* sick! */
        vomit();
    } else {
        killer.format = KILLED_BY_AN;
        /*
         * Note all "killer"s below read "Choked on %s" on the
         * high score list & tombstone.  So plan accordingly.
         */
        if (food) {
            /*KR You("choke over your %s.", foodword(food)); */
            You("%s에 목이 막혔다.", foodword(food));
            if (food->oclass == COIN_CLASS) {
                Strcpy(killer.name, "값비싼 식사");
            } else {
                killer.format = KILLED_BY;
                Strcpy(killer.name, killer_xname(food));
            }
        } else {
            /*KR You("choke over it."); */
            pline("목이 막혔다.");
            /*KR Strcpy(killer.name, "quick snack"); */
            Strcpy(killer.name, "급하게 먹음");
        }
        /*KR You("die..."); */
        pline("당신은 죽었습니다...");
        done(CHOKING);
    }
}

/* modify object wt. depending on time spent consuming it */
STATIC_OVL void
recalc_wt()
{
    struct obj *piece = context.victual.piece;
    if (!piece) {
        impossible("recalc_wt without piece");
        return;
    }
    debugpline1("Old weight = %d", piece->owt);
    debugpline2("Used time = %d, Req'd time = %d", context.victual.usedtime,
                context.victual.reqtime);
    piece->owt = weight(piece);
    debugpline1("New weight = %d", piece->owt);
}

/* called when eating interrupted by an event */
void
reset_eat()
{
    /* we only set a flag here - the actual reset process is done after
     * the round is spent eating.
     */
    if (context.victual.eating && !context.victual.doreset) {
        debugpline0("reset_eat...");
        context.victual.doreset = TRUE;
    }
    return;
}

/* base nutrition of a food-class object */
STATIC_OVL unsigned
obj_nutrition(otmp)
struct obj *otmp;
{
    unsigned nut = (otmp->otyp == CORPSE) ? mons[otmp->corpsenm].cnutrit
                      : otmp->globby ? otmp->owt
                         : (unsigned) objects[otmp->otyp].oc_nutrition;

    if (otmp->otyp == LEMBAS_WAFER) {
        if (maybe_polyd(is_elf(youmonst.data), Race_if(PM_ELF)))
            nut += nut / 4; /* 800 -> 1000 */
        else if (maybe_polyd(is_orc(youmonst.data), Race_if(PM_ORC)))
            nut -= nut / 4; /* 800 -> 600 */
        /* prevent polymorph making a partly eaten wafer
           become more nutritious than an untouched one */
        if (otmp->oeaten >= nut)
            otmp->oeaten = (otmp->oeaten < objects[LEMBAS_WAFER].oc_nutrition)
                              ? (nut - 1) : nut;
    } else if (otmp->otyp == CRAM_RATION) {
        if (maybe_polyd(is_dwarf(youmonst.data), Race_if(PM_DWARF)))
            nut += nut / 6; /* 600 -> 700 */
    }
    return nut;
}

STATIC_OVL struct obj *
touchfood(otmp)
struct obj *otmp;
{
    if (otmp->quan > 1L) {
        if (!carried(otmp))
            (void) splitobj(otmp, otmp->quan - 1L);
        else
            otmp = splitobj(otmp, 1L);
        debugpline0("split object,");
    }

    if (!otmp->oeaten) {
        costly_alteration(otmp, COST_BITE);
        otmp->oeaten = obj_nutrition(otmp);
    }

    if (carried(otmp)) {
        freeinv(otmp);
        if (inv_cnt(FALSE) >= 52) {
            sellobj_state(SELL_DONTSELL);
            dropy(otmp);
            sellobj_state(SELL_NORMAL);
        } else {
            otmp->nomerge = 1; /* used to prevent merge */
            otmp = addinv(otmp);
            otmp->nomerge = 0;
        }
    }
    return otmp;
}

/* When food decays, in the middle of your meal, we don't want to dereference
 * any dangling pointers, so set it to null (which should still trigger
 * do_reset_eat() at the beginning of eatfood()) and check for null pointers
 * in do_reset_eat().
 */
void
food_disappears(obj)
struct obj *obj;
{
    if (obj == context.victual.piece) {
        context.victual.piece = (struct obj *) 0;
        context.victual.o_id = 0;
    }
    if (obj->timed)
        obj_stop_timers(obj);
}

/* renaming an object used to result in it having a different address,
   so the sequence start eating/opening, get interrupted, name the food,
   resume eating/opening would restart from scratch */
void
food_substitution(old_obj, new_obj)
struct obj *old_obj, *new_obj;
{
    if (old_obj == context.victual.piece) {
        context.victual.piece = new_obj;
        context.victual.o_id = new_obj->o_id;
    }
    if (old_obj == context.tin.tin) {
        context.tin.tin = new_obj;
        context.tin.o_id = new_obj->o_id;
    }
}

STATIC_OVL void
do_reset_eat()
{
    debugpline0("do_reset_eat...");
    if (context.victual.piece) {
        context.victual.o_id = 0;
        context.victual.piece = touchfood(context.victual.piece);
        if (context.victual.piece)
            context.victual.o_id = context.victual.piece->o_id;
        recalc_wt();
    }
    context.victual.fullwarn = context.victual.eating =
        context.victual.doreset = FALSE;
    /* Do not set canchoke to FALSE; if we continue eating the same object
     * we need to know if canchoke was set when they started eating it the
     * previous time.  And if we don't continue eating the same object
     * canchoke always gets recalculated anyway.
     */
    stop_occupation();
    newuhs(FALSE);
}

/* called each move during eating process */
STATIC_PTR int
eatfood(VOID_ARGS)
{
    if (!context.victual.piece
        || (!carried(context.victual.piece)
            && !obj_here(context.victual.piece, u.ux, u.uy))) {
        /* maybe it was stolen? */
        do_reset_eat();
        return 0;
    }
    if (!context.victual.eating)
        return 0;

    if (++context.victual.usedtime <= context.victual.reqtime) {
        if (bite())
            return 0;
        return 1; /* still busy */
    } else {        /* done */
        done_eating(TRUE);
        return 0;
    }
}

STATIC_OVL void
done_eating(message)
boolean message;
{
    struct obj *piece = context.victual.piece;

    piece->in_use = TRUE;
    occupation = 0; /* do this early, so newuhs() knows we're done */
    newuhs(FALSE);
    if (nomovemsg) {
        if (message)
            pline1(nomovemsg);
        nomovemsg = 0;
    } else if (message)
        /*KR You("finish eating %s.", food_xname(piece, TRUE)); */
        You("%s을/를 다 먹었다.", food_xname(piece, TRUE));

    if (piece->otyp == CORPSE || piece->globby)
        cpostfx(piece->corpsenm);
    else
        fpostfx(piece);

    if (carried(piece))
        useup(piece);
    else
        useupf(piece, 1L);
    context.victual.piece = (struct obj *) 0;
    context.victual.o_id = 0;
    context.victual.fullwarn = context.victual.eating =
        context.victual.doreset = FALSE;
}

void
eating_conducts(pd)
struct permonst *pd;
{
    u.uconduct.food++;
    if (!vegan(pd))
        u.uconduct.unvegan++;
    if (!vegetarian(pd))
        violated_vegetarian();
}

/* handle side-effects of mind flayer's tentacle attack */
int
eat_brains(magr, mdef, visflag, dmg_p)
struct monst *magr, *mdef;
boolean visflag;
int *dmg_p; /* for dishing out extra damage in lieu of Int loss */
{
    struct permonst *pd = mdef->data;
    boolean give_nutrit = FALSE;
    int result = MM_HIT, xtra_dmg = rnd(10);

    if (noncorporeal(pd)) {
        if (visflag)
#if 0 /*KR:T*/
            pline("%s brain is unharmed.",
                  (mdef == &youmonst) ? "Your" : s_suffix(Monnam(mdef)));
#else
            pline("%s 뇌는 무사했다.",
                (mdef == &youmonst) ? "당신의" : Monnam(mdef));
#endif
        return MM_MISS; /* side-effects can't occur */
    } else if (magr == &youmonst) {
        /*KR You("eat %s brain!", s_suffix(mon_nam(mdef))); */
        You("%s의 뇌를 먹었다!", mon_nam(mdef));
    } else if (mdef == &youmonst) {
        /*KR Your("brain is eaten!"); */
        Your("뇌가 먹혔다!");
    } else { /* monster against monster */
        if (visflag && canspotmon(mdef))
            /*KR pline("%s brain is eaten!", s_suffix(Monnam(mdef))); */
            pline("%s의 뇌가 먹혔다!", Monnam(mdef));
    }

    if (flesh_petrifies(pd)) {
        /* mind flayer has attempted to eat the brains of a petrification
           inducing critter (most likely Medusa; attacking a cockatrice via
           tentacle-touch should have been caught before reaching this far) */
        if (magr == &youmonst) {
            if (!Stone_resistance && !Stoned)
                make_stoned(5L, (char *) 0, KILLED_BY_AN, pd->mname);
        } else {
            /* no need to check for poly_when_stoned or Stone_resistance;
               mind flayers don't have those capabilities */
            if (visflag && canseemon(magr))
                /*KR pline("%s turns to stone!", Monnam(magr)); */
                pline("%s이/가 돌로 변했다!", Monnam(magr));
            monstone(magr);
            if (!DEADMONSTER(magr)) {
                /* life-saved; don't continue eating the brains */
                return MM_MISS;
            } else {
                if (magr->mtame && !visflag)
                    /* parallels mhitm.c's brief_feeling */
                    /*KR You("have a sad thought for a moment, then it passes."); */
                    You("잠시 슬픈 생각이 들었지만, 곧 사라졌다.");
                return MM_AGR_DIED;
            }
        }
    }

    if (magr == &youmonst) {
        /*
         * player mind flayer is eating something's brain
         */
        eating_conducts(pd);
        if (mindless(pd)) { /* (cannibalism not possible here) */
            /*KR pline("%s doesn't notice.", Monnam(mdef)); */
            pline("%s은/는 눈치채지 못한다.", Monnam(mdef));
            /* all done; no extra harm inflicted upon target */
            return MM_MISS;
        } else if (is_rider(pd)) {
            /*KR pline("Ingesting that is fatal."); */
            pline("그것을 섭취하는 것은 치명적이다.");
#if 0 /*KR:T*/
            Sprintf(killer.name, "unwisely ate the brain of %s", pd->mname);
            killer.format = NO_KILLER_PREFIX;
#else
            Sprintf(killer.name, "어리석게도 %s의 뇌를 먹고", pd->mname);
            killer.format = KILLED_BY;
#endif
            done(DIED);
            /* life-saving needed to reach here */
            exercise(A_WIS, FALSE);
            *dmg_p += xtra_dmg; /* Rider takes extra damage */
        } else {
            morehungry(-rnd(30)); /* cannot choke */
            if (ABASE(A_INT) < AMAX(A_INT)) {
                /* recover lost Int; won't increase current max */
                ABASE(A_INT) += rnd(4);
                if (ABASE(A_INT) > AMAX(A_INT))
                    ABASE(A_INT) = AMAX(A_INT);
                context.botl = 1;
            }
            exercise(A_WIS, TRUE);
            *dmg_p += xtra_dmg;
        }
        /* targetting another mind flayer or your own underlying species
           is cannibalism */
        (void) maybe_cannibal(monsndx(pd), TRUE);

    } else if (mdef == &youmonst) {
        /*
         * monster mind flayer is eating hero's brain
         */
        /* no such thing as mindless players */
        if (ABASE(A_INT) <= ATTRMIN(A_INT)) {
            /*KR static NEARDATA const char brainlessness[] = "brainlessness"; */
            static NEARDATA const char brainlessness[] = "무뇌 상태";

            if (Lifesaved) {
                Strcpy(killer.name, brainlessness);
                killer.format = KILLED_BY;
                done(DIED);
                /* amulet of life saving has now been used up */
                /*KR pline("Unfortunately your brain is still gone."); */
                pline("유감이지만, 당신의 뇌는 아직도 없는 상태다.");
                /* sanity check against adding other forms of life-saving */
                u.uprops[LIFESAVED].extrinsic =
                    u.uprops[LIFESAVED].intrinsic = 0L;
            } else {
                /*KR Your("last thought fades away."); */
                Your("마지막 생각이 주마등처럼 스쳐 지나갔다.");
            }
            Strcpy(killer.name, brainlessness);
            killer.format = KILLED_BY;
            done(DIED);
            /* can only get here when in wizard or explore mode and user has
               explicitly chosen not to die; arbitrarily boost intelligence */
            ABASE(A_INT) = ATTRMIN(A_INT) + 2;
            /*KR You_feel("like a scarecrow."); */
            You("허수아비 같은 기분이 든다.");
        }
        give_nutrit = TRUE; /* in case a conflicted pet is doing this */
        exercise(A_WIS, FALSE);
        /* caller handles Int and memory loss */

    } else { /* mhitm */
        /*
         * monster mind flayer is eating another monster's brain
         */
        if (mindless(pd)) {
            if (visflag && canspotmon(mdef))
                /*KR pline("%s doesn't notice.", Monnam(mdef)); */
                pline("%s은/는 눈치채지 못한다.", Monnam(mdef));
            return MM_MISS;
        } else if (is_rider(pd)) {
            mondied(magr);
            if (DEADMONSTER(magr))
                result = MM_AGR_DIED;
            /* Rider takes extra damage regardless of whether attacker dies */
            *dmg_p += xtra_dmg;
        } else {
            *dmg_p += xtra_dmg;
            give_nutrit = TRUE;
            if (*dmg_p >= mdef->mhp && visflag && canspotmon(mdef))
#if 0 /*KR:T*/
                pline("%s last thought fades away...",
                      s_suffix(Monnam(mdef)));
#else
                pline("%s의 마지막 생각이 스쳐 지나간다...",
                    Monnam(mdef));
#endif
        }
    }

    if (give_nutrit && magr->mtame && !magr->isminion) {
        EDOG(magr)->hungrytime += rnd(60);
        magr->mconf = 0;
    }

    return result;
}

/* eating a corpse or egg of one's own species is usually naughty */
STATIC_OVL boolean
maybe_cannibal(pm, allowmsg)
int pm;
boolean allowmsg;
{
    static NEARDATA long ate_brains = 0L;
    struct permonst *fptr = &mons[pm]; /* food type */

    /* when poly'd into a mind flayer, multiple tentacle hits in one
       turn cause multiple digestion checks to occur; avoid giving
       multiple luck penalties for the same attack */
    if (moves == ate_brains)
        return FALSE;
    ate_brains = moves; /* ate_anything, not just brains... */

    if (!CANNIBAL_ALLOWED()
        /* non-cannibalistic heroes shouldn't eat own species ever
           and also shouldn't eat current species when polymorphed
           (even if having the form of something which doesn't care
           about cannibalism--hero's innate traits aren't altered) */
        && (your_race(fptr)
            || (Upolyd && same_race(youmonst.data, fptr))
            || (u.ulycn >= LOW_PM && were_beastie(pm) == u.ulycn))) {
        if (allowmsg) {
            if (Upolyd && your_race(fptr))
                /*KR You("have a bad feeling deep inside.");
            You("cannibal!  You will regret this!"); */
                You("뱃속에서 혐오감이 든다.");
            pline("동족을 먹다니! 후회하게 될 거다!");
        }
        HAggravate_monster |= FROMOUTSIDE;
        change_luck(-rn1(4, 2)); /* -5..-2 */
        return TRUE;
    }
    return FALSE;
}

STATIC_OVL void
cprefx(pm)
register int pm;
{
    (void) maybe_cannibal(pm, TRUE);
    if (flesh_petrifies(&mons[pm])) {
        if (!Stone_resistance
            && !(poly_when_stoned(youmonst.data)
                 && polymon(PM_STONE_GOLEM))) {
            /*KR Sprintf(killer.name, "tasting %s meat", mons[pm].mname); */
            Sprintf(killer.name, "%s의 고기를 먹고", mons[pm].mname);
            killer.format = KILLED_BY;
            /*KR You("turn to stone."); */
            You("돌로 변했다.");
            done(STONING);
            if (context.victual.piece)
                context.victual.eating = FALSE;
            return; /* lifesaved */
        }
    }

    switch (pm) {
    case PM_LITTLE_DOG:
    case PM_DOG:
    case PM_LARGE_DOG:
    case PM_KITTEN:
    case PM_HOUSECAT:
    case PM_LARGE_CAT:
        /* cannibals are allowed to eat domestic animals without penalty */
        if (!CANNIBAL_ALLOWED()) {
            /*KR You_feel("that eating the %s was a bad idea.", mons[pm].mname); */
            pline("%s을/를 먹는 것은 나쁜 생각이었다.", mons[pm].mname);
            HAggravate_monster |= FROMOUTSIDE;
        }
        break;
    case PM_LIZARD:
        if (Stoned)
            fix_petrification();
        break;
    case PM_DEATH:
    case PM_PESTILENCE:
    case PM_FAMINE: {
        /*KR pline("Eating that is instantly fatal."); */
        pline("그것을 먹자마자 바로 죽고 말았다.");
#if 0 /*KR:T*/
        Sprintf(killer.name, "unwisely ate the body of %s", mons[pm].mname);
        killer.format = NO_KILLER_PREFIX;
#else
        Sprintf(killer.name, "어리석게도 %s의 몸을 먹고", mons[pm].mname);
        killer.format = KILLED_BY;
#endif
        done(DIED);
        /* life-saving needed to reach here */
        exercise(A_WIS, FALSE);
        /* It so happens that since we know these monsters */
        /* cannot appear in tins, context.victual.piece will always */
        /* be what we want, which is not generally true. */
        if (revive_corpse(context.victual.piece)) {
            context.victual.piece = (struct obj *) 0;
            context.victual.o_id = 0;
        }
        return;
    }
    case PM_GREEN_SLIME:
        if (!Slimed && !Unchanging && !slimeproof(youmonst.data)) {
            /*KR You("don't feel very well."); 영 좋지 못한 곳을 맞아서... */
            You("기분이 영 좋지 못하다.");
            make_slimed(10L, (char *) 0);
            delayed_killer(SLIMED, KILLED_BY_AN, "");
        }
    /* Fall through */
    default:
        if (acidic(&mons[pm]) && Stoned)
            fix_petrification();
        break;
    }
}

void
fix_petrification()
{
    char buf[BUFSZ];

    if (Hallucination)
#if 0 /*KR:T*/
        Sprintf(buf, "What a pity--you just ruined a future piece of %sart!",
                ACURR(A_CHA) > 15 ? "fine " : "");
#else /* 이었던 것 */
        Sprintf(buf, "작품명: 미래의 %s 예술 작품!(이었던 것)",
            ACURR(A_CHA) > 15 ? "멋진 " : "");
#endif
    else
        /*KR Strcpy(buf, "You feel limber!"); */
        Strcpy(buf, "몸이 유연해진 것 같다!");
    make_stoned(0L, buf, 0, (char *) 0);
}

/*
 * If you add an intrinsic that can be gotten by eating a monster, add it
 * to intrinsic_possible() and givit().  (It must already be in prop.h to
 * be an intrinsic property.)
 * It would be very easy to make the intrinsics not try to give you one
 * that you already had by checking to see if you have it in
 * intrinsic_possible() instead of givit(), but we're not that nice.
 */

/* intrinsic_possible() returns TRUE iff a monster can give an intrinsic. */
STATIC_OVL int
intrinsic_possible(type, ptr)
int type;
register struct permonst *ptr;
{
    int res = 0;

#ifdef DEBUG
#define ifdebugresist(Msg)      \
    do {                        \
        if (res)                \
            debugpline0(Msg);   \
    } while (0)
#else
#define ifdebugresist(Msg) /*empty*/
#endif
    switch (type) {
    case FIRE_RES:
        res = (ptr->mconveys & MR_FIRE) != 0;
        ifdebugresist("can get fire resistance");
        break;
    case SLEEP_RES:
        res = (ptr->mconveys & MR_SLEEP) != 0;
        ifdebugresist("can get sleep resistance");
        break;
    case COLD_RES:
        res = (ptr->mconveys & MR_COLD) != 0;
        ifdebugresist("can get cold resistance");
        break;
    case DISINT_RES:
        res = (ptr->mconveys & MR_DISINT) != 0;
        ifdebugresist("can get disintegration resistance");
        break;
    case SHOCK_RES: /* shock (electricity) resistance */
        res = (ptr->mconveys & MR_ELEC) != 0;
        ifdebugresist("can get shock resistance");
        break;
    case POISON_RES:
        res = (ptr->mconveys & MR_POISON) != 0;
        ifdebugresist("can get poison resistance");
        break;
    case TELEPORT:
        res = can_teleport(ptr);
        ifdebugresist("can get teleport");
        break;
    case TELEPORT_CONTROL:
        res = control_teleport(ptr);
        ifdebugresist("can get teleport control");
        break;
    case TELEPAT:
        res = telepathic(ptr);
        ifdebugresist("can get telepathy");
        break;
    default:
        /* res stays 0 */
        break;
    }
#undef ifdebugresist
    return res;
}

/* givit() tries to give you an intrinsic based on the monster's level
 * and what type of intrinsic it is trying to give you.
 */
STATIC_OVL void
givit(type, ptr)
int type;
register struct permonst *ptr;
{
    register int chance;

    debugpline1("Attempting to give intrinsic %d", type);
    /* some intrinsics are easier to get than others */
    switch (type) {
    case POISON_RES:
        if ((ptr == &mons[PM_KILLER_BEE] || ptr == &mons[PM_SCORPION])
            && !rn2(4))
            chance = 1;
        else
            chance = 15;
        break;
    case TELEPORT:
        chance = 10;
        break;
    case TELEPORT_CONTROL:
        chance = 12;
        break;
    case TELEPAT:
        chance = 1;
        break;
    default:
        chance = 15;
        break;
    }

    if (ptr->mlevel <= rn2(chance))
        return; /* failed die roll */

    switch (type) {
    case FIRE_RES:
        debugpline0("Trying to give fire resistance");
        if (!(HFire_resistance & FROMOUTSIDE)) {
            /*KR You(Hallucination ? "be chillin'." : "feel a momentary chill."); */
            You(Hallucination ? "쿨~해진 기분이 든다." : "순간적으로 오한을 느낀다.");
            HFire_resistance |= FROMOUTSIDE;
        }
        break;
    case SLEEP_RES:
        debugpline0("Trying to give sleep resistance");
        if (!(HSleep_resistance & FROMOUTSIDE)) {
            /*KR You_feel("wide awake."); */
            You("완전히 깨어있는 기분이 든다.");
            HSleep_resistance |= FROMOUTSIDE;
        }
        break;
    case COLD_RES:
        debugpline0("Trying to give cold resistance");
        if (!(HCold_resistance & FROMOUTSIDE)) {
            /*KR You_feel("full of hot air."); */
            You("온 몸에 뜨거운 공기를 느낀다.");
            HCold_resistance |= FROMOUTSIDE;
        }
        break;
    case DISINT_RES:
        debugpline0("Trying to give disintegration resistance");
        if (!(HDisint_resistance & FROMOUTSIDE)) {
            /*KR You_feel(Hallucination ? "totally together, man." : "very firm."); */ /*포켓몬스터 무인편 엔딩 */
            You_feel(Hallucination ? "노래를 흥얼거렸다. 우리는 모두 친구~ (맞아~)" : "매우 단단해진 기분이 들었다.");
            HDisint_resistance |= FROMOUTSIDE;
        }
        break;
    case SHOCK_RES: /* shock (electricity) resistance */
        debugpline0("Trying to give shock resistance");
        if (!(HShock_resistance & FROMOUTSIDE)) {
            if (Hallucination)
                /*KR You_feel("grounded in reality."); */
                pline("난 살아있다! 살아있다! 난 살아 있다구!");
            else
                /*KR Your("health currently feels amplified!"); */
                pline("건강이 증폭된 듯한 기분이 든다!");
            HShock_resistance |= FROMOUTSIDE;
        }
        break;
    case POISON_RES:
        debugpline0("Trying to give poison resistance");
        if (!(HPoison_resistance & FROMOUTSIDE)) {
            /*KR You_feel(Poison_resistance ? "especially healthy." : "healthy."); */
            You_feel(Poison_resistance ? "*특별히* 건강해진 기분이 든다." : "건강해진 기분이 든다.");
            HPoison_resistance |= FROMOUTSIDE;
        }
        break;
    case TELEPORT:
        debugpline0("Trying to give teleport");
        if (!(HTeleportation & FROMOUTSIDE)) {
            /*KR You_feel(Hallucination ? "diffuse." : "very jumpy."); */
            pline(Hallucination ? "램프의 요정이 된 기분이 든다." : "매우 씰룩거리는 기분이 든다.");
            HTeleportation |= FROMOUTSIDE;
        }
        break;
    case TELEPORT_CONTROL:
        debugpline0("Trying to give teleport control");
        if (!(HTeleport_control & FROMOUTSIDE)) {
#if 0 /*KR:T*/
            You_feel(Hallucination ? "centered in your personal space."
                                   : "in control of yourself.");
#else
            You_feel(Hallucination ? "이기적이게 된 기분이 든다."
                                   : "스스로를 통제할 수 있게 된 기분이 든다.");
#endif
            HTeleport_control |= FROMOUTSIDE;
        }
        break;
    case TELEPAT:
        debugpline0("Trying to give telepathy");
        if (!(HTelepat & FROMOUTSIDE)) {
#if 0 /*KR:T*/
            You_feel(Hallucination ? "in touch with the cosmos."
                                   : "a strange mental acuity.");
#else /*빵상아줌마 드립 마렵네*/
            You_feel(Hallucination ? "우주와 접신한 기분이 든다."
                                   : "기묘한 정신적 예리함을 느꼈다.");
#endif
            HTelepat |= FROMOUTSIDE;
            /* If blind, make sure monsters show up. */
            if (Blind)
                see_monsters();
        }
        break;
    default:
        debugpline0("Tried to give an impossible intrinsic");
        break;
    }
}

/* called after completely consuming a corpse */
STATIC_OVL void
cpostfx(pm)
int pm;
{
    int tmp = 0;
    int catch_lycanthropy = NON_PM;
    boolean check_intrinsics = FALSE;

    /* in case `afternmv' didn't get called for previously mimicking
       gold, clean up now to avoid `eatmbuf' memory leak */
    if (eatmbuf)
        (void) eatmdone();

    switch (pm) {
    case PM_NEWT:
        /* MRKR: "eye of newt" may give small magical energy boost */
        if (rn2(3) || 3 * u.uen <= 2 * u.uenmax) {
            int old_uen = u.uen;

            u.uen += rnd(3);
            if (u.uen > u.uenmax) {
                if (!rn2(3))
                    u.uenmax++;
                u.uen = u.uenmax;
            }
            if (old_uen != u.uen) {
                /*KR You_feel("a mild buzz."); */
                You("약하게 부르르 떨었다.");
                context.botl = 1;
            }
        }
        break;
    case PM_WRAITH:
        pluslvl(FALSE);
        break;
    case PM_HUMAN_WERERAT:
        catch_lycanthropy = PM_WERERAT;
        break;
    case PM_HUMAN_WEREJACKAL:
        catch_lycanthropy = PM_WEREJACKAL;
        break;
    case PM_HUMAN_WEREWOLF:
        catch_lycanthropy = PM_WEREWOLF;
        break;
    case PM_NURSE:
        if (Upolyd)
            u.mh = u.mhmax;
        else
            u.uhp = u.uhpmax;
        make_blinded(0L, !u.ucreamed);
        context.botl = 1;
        check_intrinsics = TRUE; /* might also convey poison resistance */
        break;
    case PM_STALKER:
        if (!Invis) {
            set_itimeout(&HInvis, (long) rn1(100, 50));
            if (!Blind && !BInvis)
                self_invis_message();
        } else {
            if (!(HInvis & INTRINSIC))
                /*KR You_feel("hidden!"); */
                Your("모습이 숨겨졌다!");
            HInvis |= FROMOUTSIDE;
            HSee_invisible |= FROMOUTSIDE;
        }
        newsym(u.ux, u.uy);
        /*FALLTHRU*/
    case PM_YELLOW_LIGHT:
    case PM_GIANT_BAT:
        make_stunned((HStun & TIMEOUT) + 30L, FALSE);
        /*FALLTHRU*/
    case PM_BAT:
        make_stunned((HStun & TIMEOUT) + 30L, FALSE);
        break;
    case PM_GIANT_MIMIC:
        tmp += 10;
        /*FALLTHRU*/
    case PM_LARGE_MIMIC:
        tmp += 20;
        /*FALLTHRU*/
    case PM_SMALL_MIMIC:
        tmp += 20;
        if (youmonst.data->mlet != S_MIMIC && !Unchanging) {
            char buf[BUFSZ];

            u.uconduct.polyselfs++; /* you're changing form */
#if 0 /*KR:T*/
            You_cant("resist the temptation to mimic %s.",
                     Hallucination ? "an orange" : "a pile of gold");
#else
            You("%s를 흉내내고 싶은 충동에 휩싸였다.",
                Hallucination ? "오렌지" : "금화 더미");
#endif
            /* A pile of gold can't ride. */
            if (u.usteed)
                dismount_steed(DISMOUNT_FELL);
            nomul(-tmp);
            /*KR multi_reason = "pretending to be a pile of gold"; */
            multi_reason = "금화 더미를 흉내내고 있을 때";
            Sprintf(buf,
                    Hallucination
                /*KR   ? "You suddenly dread being peeled and mimic %s again!"
                       : "You now prefer mimicking %s again.", */
                       ? "당신은 갑자기 알몸뚱이가 된 것이 무서워서 다시 %s을/를 흉내낸다!"
                       : "이제 다시 %s을/를 흉내내고 싶어졌다.",
                    an(Upolyd ? youmonst.data->mname : urace.noun));
            eatmbuf = dupstr(buf);
            nomovemsg = eatmbuf;
            afternmv = eatmdone;
            /* ??? what if this was set before? */
            youmonst.m_ap_type = M_AP_OBJECT;
            youmonst.mappearance = Hallucination ? ORANGE : GOLD_PIECE;
            newsym(u.ux, u.uy);
            curs_on_u();
            /* make gold symbol show up now */
            display_nhwindow(WIN_MAP, TRUE);
        }
        break;
    case PM_QUANTUM_MECHANIC:
        /*KR Your("velocity suddenly seems very uncertain!"); */
        Your("속도가 갑자기 불연속적으로 보인다!");
        if (HFast & INTRINSIC) {
            HFast &= ~INTRINSIC;
            /*KR You("seem slower."); */
            You("느려진 것 같다.");
        } else {
            HFast |= FROMOUTSIDE;
            /*KR You("seem faster."); */
            You("빨라진 것 같다.");
        }
        break;
    case PM_LIZARD:
        if ((HStun & TIMEOUT) > 2)
            make_stunned(2L, FALSE);
        if ((HConfusion & TIMEOUT) > 2)
            make_confused(2L, FALSE);
        break;
    case PM_CHAMELEON:
    case PM_DOPPELGANGER:
    case PM_SANDESTIN: /* moot--they don't leave corpses */
        if (Unchanging) {
#if 0 /*KR:T*/
            You_feel("momentarily different."); /* same as poly trap */
#else
            You_feel("잠시 달라졌다고 느낀다."); /* same as poly trap */
#endif
        } else {
            /*KR You_feel("a change coming over you."); */
            pline("변화가 찾아온 것 같다.");
            polyself(0);
        }
        break;
    case PM_DISENCHANTER:
        /* picks an intrinsic at random and removes it; there's
           no feedback if hero already lacks the chosen ability */
        debugpline0("using attrcurse to strip an intrinsic");
        attrcurse();
        break;
    case PM_MIND_FLAYER:
    case PM_MASTER_MIND_FLAYER:
        if (ABASE(A_INT) < ATTRMAX(A_INT)) {
            if (!rn2(2)) {
                /*KR pline("Yum!  That was real brain food!"); */
                pline("냠냠! 정말 '뇌가 건강해지는' 맛인걸!");
                (void) adjattrib(A_INT, 1, FALSE);
                break; /* don't give them telepathy, too */
            }
        } else {
            /*KR pline("For some reason, that tasted bland."); */
            pline("왜인지는 몰라도, 담백한 맛이 났다.");
        }
    /*FALLTHRU*/
    default:
        check_intrinsics = TRUE;
        break;
    }

    /* possibly convey an intrinsic */
    if (check_intrinsics) {
        struct permonst *ptr = &mons[pm];
        boolean conveys_STR = is_giant(ptr);
        int i, count;

        if (dmgtype(ptr, AD_STUN) || dmgtype(ptr, AD_HALU)
            || pm == PM_VIOLET_FUNGUS) {
            /*KR pline("Oh wow!  Great stuff!"); */
            pline("우와! 정말 일품인 걸!");
            (void) make_hallucinated((HHallucination & TIMEOUT) + 200L, FALSE,
                                     0L);
        }

        /* Check the monster for all of the intrinsics.  If this
         * monster can give more than one, pick one to try to give
         * from among all it can give.
         *
         * Strength from giants is now treated like an intrinsic
         * rather than being given unconditionally.
         */
        count = 0; /* number of possible intrinsics */
        tmp = 0;   /* which one we will try to give */
        if (conveys_STR) {
            count = 1;
            tmp = -1; /* use -1 as fake prop index for STR */
            debugpline1("\"Intrinsic\" strength, %d", tmp);
        }
        for (i = 1; i <= LAST_PROP; i++) {
            if (!intrinsic_possible(i, ptr))
                continue;
            ++count;
            /* a 1 in count chance of replacing the old choice
               with this one, and a count-1 in count chance
               of keeping the old choice (note that 1 in 1 and
               0 in 1 are what we want for the first candidate) */
            if (!rn2(count)) {
                debugpline2("Intrinsic %d replacing %d", i, tmp);
                tmp = i;
            }
        }
        /* if strength is the only candidate, give it 50% chance */
        if (conveys_STR && count == 1 && !rn2(2))
            tmp = 0;
        /* if something was chosen, give it now (givit() might fail) */
        if (tmp == -1)
            gainstr((struct obj *) 0, 0, TRUE);
        else if (tmp > 0)
            givit(tmp, ptr);
    } /* check_intrinsics */

    if (catch_lycanthropy >= LOW_PM) {
        set_ulycn(catch_lycanthropy);
        retouch_equipment(2);
    }
    return;
}

void
violated_vegetarian()
{
    u.uconduct.unvegetarian++;
    if (Role_if(PM_MONK)) {
        /*KR You_feel("guilty."); */
        pline("당신은 죄책감을 느꼈다.");
        adjalign(-1);
    }
    return;
}

/* common code to check and possibly charge for 1 context.tin.tin,
 * will split() context.tin.tin if necessary */
STATIC_PTR struct obj *
costly_tin(alter_type)
int alter_type; /* COST_xxx */
{
    struct obj *tin = context.tin.tin;

    if (carried(tin) ? tin->unpaid
                     : (costly_spot(tin->ox, tin->oy) && !tin->no_charge)) {
        if (tin->quan > 1L) {
            tin = context.tin.tin = splitobj(tin, 1L);
            context.tin.o_id = tin->o_id;
        }
        costly_alteration(tin, alter_type);
    }
    return tin;
}

int
tin_variety_txt(s, tinvariety)
char *s;
int *tinvariety;
{
    int k, l;

    if (s && tinvariety) {
        *tinvariety = -1;
        for (k = 0; k < TTSZ - 1; ++k) {
            l = (int) strlen(tintxts[k].txt);
            if (!strncmpi(s, tintxts[k].txt, l) && ((int) strlen(s) > l)
                && s[l] == ' ') {
                *tinvariety = k;
                return (l + 1);
            }
        }
    }
    return 0;
}

/*
 * This assumes that buf already contains the word "tin",
 * as is the case with caller xname().
 */
void
tin_details(obj, mnum, buf)
struct obj *obj;
int mnum;
char *buf;
{
#if 0 /*KR*/
    char buf2[BUFSZ];
#endif
    int r = tin_variety(obj, TRUE);

    if (obj && buf) {
        if (r == SPINACH_TIN)
            /*KR Strcat(buf, " of spinach"); */
            Strcat(buf, "시금치");
        else if (mnum == NON_PM)
            /*KR Strcpy(buf, "empty tin"); */
            Strcat(buf, "텅 빈 통조림");
        else {
#if 0 /*KR*/ /*한국어에서는 뒤로 보냄*/
            if ((obj->cknown || iflags.override_ID) && obj->spe < 0) {
                if (r == ROTTEN_TIN || r == HOMEMADE_TIN) {
                    /* put these before the word tin */
                    Sprintf(buf2, "%s %s of ", tintxts[r].txt, buf);
                    Strcpy(buf, buf2);
                } else {
                    Sprintf(eos(buf), " of %s ", tintxts[r].txt);
                } /*아무것도 없었으나 JNethack엔 바로 밑줄 하나가 있음. */
                Sprintf(eos(buf), "%s", tintxts[r].txt);
            } else {
                Strcpy(eos(buf), " of ");
            }
#endif
#if 1 /*KR*/ /* '~의'로 시작하면 후치, 이외에는 전치. 솔직히 뭔소린지 모름*/
            if (strstr(tintxts[r].txt, "의") != tintxts[r].txt) {
                Strcpy(eos(buf), tintxts[r].txt);
            }
#endif
            if (vegetarian(&mons[mnum]))
                /*KR Sprintf(eos(buf), "%s", mons[mnum].mname); 똑같은내용인데 왜있지?*/
                Sprintf(eos(buf), "%s", mons[mnum].mname);
            else
                /*KR Sprintf(eos(buf), "%s meat", mons[mnum].mname); */
                Sprintf(eos(buf), "%s의 고기", mons[mnum].mname);
#if 1 /*KR*/ /* '~의'로 시작하면 후치, 이외에는 전치. 뭔소리여*/
            if (strstr(tintxts[r].txt, "의") == tintxts[r].txt) {
                Strcpy(eos(buf), tintxts[r].txt);
            }
            Strcpy(eos(buf), "의");
#endif
        }
    }
}

void
set_tin_variety(obj, forcetype)
struct obj *obj;
int forcetype;
{
    register int r;

    if (forcetype == SPINACH_TIN
        || (forcetype == HEALTHY_TIN
            && (obj->corpsenm == NON_PM /* empty or already spinach */
                || !vegetarian(&mons[obj->corpsenm])))) { /* replace meat */
        obj->corpsenm = NON_PM; /* not based on any monster */
        obj->spe = 1;           /* spinach */
        return;
    } else if (forcetype == HEALTHY_TIN) {
        r = tin_variety(obj, FALSE);
        if (r < 0 || r >= TTSZ)
            r = ROTTEN_TIN; /* shouldn't happen */
        while ((r == ROTTEN_TIN && !obj->cursed) || !tintxts[r].fodder)
            r = rn2(TTSZ - 1);
    } else if (forcetype >= 0 && forcetype < TTSZ - 1) {
        r = forcetype;
    } else {               /* RANDOM_TIN */
        r = rn2(TTSZ - 1); /* take your pick */
        if (r == ROTTEN_TIN && nonrotting_corpse(obj->corpsenm))
            r = HOMEMADE_TIN; /* lizards don't rot */
    }
    obj->spe = -(r + 1); /* offset by 1 to allow index 0 */
}

STATIC_OVL int
tin_variety(obj, disp)
struct obj *obj;
boolean disp; /* we're just displaying so leave things alone */
{
    register int r;

    if (obj->spe == 1) {
        r = SPINACH_TIN;
    } else if (obj->cursed) {
        r = ROTTEN_TIN; /* always rotten if cursed */
    } else if (obj->spe < 0) {
        r = -(obj->spe);
        --r; /* get rid of the offset */
    } else
        r = rn2(TTSZ - 1);

    if (!disp && r == HOMEMADE_TIN && !obj->blessed && !rn2(7))
        r = ROTTEN_TIN; /* some homemade tins go bad */

    if (r == ROTTEN_TIN && nonrotting_corpse(obj->corpsenm))
        r = HOMEMADE_TIN; /* lizards don't rot */
    return r;
}

STATIC_OVL void
consume_tin(mesg)
const char *mesg;
{
    const char *what;
    int which, mnum, r;
    struct obj *tin = context.tin.tin;

    r = tin_variety(tin, FALSE);
    if (tin->otrapped || (tin->cursed && r != HOMEMADE_TIN && !rn2(8))) {
        /*KR b_trapped("tin", 0); */
        b_trapped("통조림", 0);
        tin = costly_tin(COST_DSTROY);
        goto use_up_tin;
    }

    pline1(mesg); /* "You succeed in opening the tin." */

    if (r != SPINACH_TIN) {
        mnum = tin->corpsenm;
        if (mnum == NON_PM) {
            /*KR pline("It turns out to be empty."); */
            pline("통조림은 비어 있었다.");
            tin->dknown = tin->known = 1;
            tin = costly_tin(COST_OPEN);
            goto use_up_tin;
        }

        which = 0; /* 0=>plural, 1=>as-is, 2=>"the" prefix */
        if ((mnum == PM_COCKATRICE || mnum == PM_CHICKATRICE)
            && (Stone_resistance || Hallucination)) {
            /*KR what = "chicken"; */
            what = "닭고기";
            which = 1; /* suppress pluralization */
        } else if (Hallucination) {
            what = rndmonnam(NULL);
        } else {
            what = mons[mnum].mname;
            if (the_unique_pm(&mons[mnum]))
                which = 2;
            else if (type_is_pname(&mons[mnum]))
                which = 1;
        }
        if (which == 0)
            what = makeplural(what);
        else if (which == 2)
            what = the(what);

        /*KR pline("It smells like %s.", what); 
        if (yn("Eat it?") == 'n') { */
        pline("%s의 냄새가 난다.", what);
        if (yn("먹겠습니까?") == 'n') {
            if (flags.verbose)
                /*KR You("discard the open tin."); */
                You("개봉한 통조림을 버렸다.");
            if (!Hallucination)
                tin->dknown = tin->known = 1;
            tin = costly_tin(COST_OPEN);
            goto use_up_tin;
        }

        /* in case stop_occupation() was called on previous meal */
        context.victual.piece = (struct obj *) 0;
        context.victual.o_id = 0;
        context.victual.fullwarn = context.victual.eating =
            context.victual.doreset = FALSE;

#if 0 /*KR*/
        You("consume %s %s.", tintxts[r].txt, mons[mnum].mname);
#else
        if (strstr(tintxts[r].txt, "의") == tintxts[r].txt) {
            You("%s %s의 통조림을 먹어치웠다.", mons[mnum].mname, tintxts[r].txt);
        }
        else {
            You("%s %s의 통조림을 먹어치웠다.", tintxts[r].txt, mons[mnum].mname);
        }
#endif
        eating_conducts(&mons[mnum]);

        tin->dknown = tin->known = 1;
        cprefx(mnum);
        cpostfx(mnum);

        /* charge for one at pre-eating cost */
        tin = costly_tin(COST_OPEN);

        if (tintxts[r].nut < 0) /* rotten */
            make_vomiting((long) rn1(15, 10), FALSE);
        else
            lesshungry(tintxts[r].nut);

        if (tintxts[r].greasy) {
            /* Assume !Glib, because you can't open tins when Glib. */
            make_glib(rn1(11, 5)); /* 5..15 */
#if 0 /*KR*/
            pline("Eating %s food made your %s very slippery.",
                  tintxts[r].txt, fingers_or_gloves(TRUE));
#else
            pline("기름진 음식을 먹어서 당신의 %s이 미끄러워졌다.",
                fingers_or_gloves(TRUE));
#endif
        }

    } else { /* spinach... */
        if (tin->cursed) {
#if 0 /*KR:T*/
            pline("It contains some decaying%s%s substance.",
                  Blind ? "" : " ", Blind ? "" : hcolor(NH_GREEN));
#else
            pline("%s 부패한 물질이 들어있다.",
                Blind ? "" : hcolor(NH_GREEN));
#endif
        } else {
            /*KR pline("It contains spinach."); */
            pline("시금치가 들어 있다.");
            tin->dknown = tin->known = 1;
        }

        /*KR if (yn("Eat it?") == 'n') { */
        if (yn("먹겠습니까?") == 'n') {
            if (flags.verbose)
                /*KR You("discard the open tin."); */
                You("개봉한 통조림을 버렸다.");
            tin = costly_tin(COST_OPEN);
            goto use_up_tin;
        }

        /*
         * Same order as with non-spinach above:
         * conduct update, side-effects, shop handling, and nutrition.
         */
        u.uconduct.food++; /* don't need vegetarian checks for spinach */
        if (!tin->cursed)
#if 0 /*KR:T*/
            pline("This makes you feel like %s!",
                  /* "Swee'pea" is a character from the Popeye cartoons */
                  Hallucination ? "Swee'pea"
                  /* "feel like Popeye" unless sustain ability suppresses
                     any attribute change; this slightly oversimplifies
                     things:  we want "Popeye" if no strength increase
                     occurs due to already being at maximum, but we won't
                     get it if at-maximum and fixed-abil both apply */
                  : !Fixed_abil ? "Popeye"
                  /* no gain, feel like another character from Popeye */
                  : (flags.female ? "Olive Oyl" : "Bluto"));
#else
            pline("%s가 된 기분이 든다!",
                /* "Swee'pea" is a character from the Popeye cartoons */
                Hallucination ? "스위피"
                /* "feel like Popeye" unless sustain ability suppresses
                   any attribute change; this slightly oversimplifies
                   things:  we want "Popeye" if no strength increase
                   occurs due to already being at maximum, but we won't
                   get it if at-maximum and fixed-abil both apply */
                : !Fixed_abil ? "뽀빠이"
                /* no gain, feel like another character from Popeye */
                : (flags.female ? "올리브 오일" : "블루토"));
#endif
        gainstr(tin, 0, FALSE);

        tin = costly_tin(COST_OPEN);
        lesshungry(tin->blessed ? 600                   /* blessed */
                   : !tin->cursed ? (400 + rnd(200))    /* uncursed */
                     : (200 + rnd(400)));               /* cursed */
    }

 use_up_tin:
    if (carried(tin))
        useup(tin);
    else
        useupf(tin, 1L);
    context.tin.tin = (struct obj *) 0;
    context.tin.o_id = 0;
}

/* called during each move whilst opening a tin */
STATIC_PTR int
opentin(VOID_ARGS)
{
    /* perhaps it was stolen (although that should cause interruption) */
    if (!carried(context.tin.tin)
        && (!obj_here(context.tin.tin, u.ux, u.uy) || !can_reach_floor(TRUE)))
        return 0; /* %% probably we should use tinoid */
    if (context.tin.usedtime++ >= 50) {
        /*KR You("give up your attempt to open the tin."); */
        You("통조림 열기를 포기했다.");
        return 0;
    }
    if (context.tin.usedtime < context.tin.reqtime)
        return 1; /* still busy */

    /*KR consume_tin("You succeed in opening the tin."); */
    consume_tin("통조림 열기에 성공했다.");
    return 0;
}

/* called when starting to open a tin */
STATIC_OVL void
start_tin(otmp)
struct obj *otmp;
{
    const char *mesg = 0;
    register int tmp;

    if (metallivorous(youmonst.data)) {
        /*KR mesg = "You bite right into the metal tin..."; */
        mesg = "금속 통조림을 통째로 씹었다...";
        tmp = 0;
    } else if (cantwield(youmonst.data)) { /* nohands || verysmall */
        /*KR You("cannot handle the tin properly to open it."); */
        You("통조림을 잘 열 수 없다.");
        return;
    } else if (otmp->blessed) {
        /* 50/50 chance for immediate access vs 1 turn delay (unless
           wielding blessed tin opener which always yields immediate
           access); 1 turn delay case is non-deterministic:  getting
           interrupted and retrying might yield another 1 turn delay
           or might open immediately on 2nd (or 3rd, 4th, ...) try */
        tmp = (uwep && uwep->blessed && uwep->otyp == TIN_OPENER) ? 0 : rn2(2);
        if (!tmp)
            /*KR mesg = "The tin opens like magic!"; */
            mesg = "통조림이 마법같이 열렸다!";
        else
            /*KR pline_The("tin seems easy to open."); */
            pline_The("통조림을 쉽게 열 수 있을 것 같다.");
    } else if (uwep) {
        switch (uwep->otyp) {
        case TIN_OPENER:
#if 0 /*KR:T*/
            mesg = "You easily open the tin."; /* iff tmp==0 */
#else
            mesg = "당신은 통조림을 쉽게 열었다."; /* iff tmp==0 */
#endif
            tmp = rn2(uwep->cursed ? 3 : !uwep->blessed ? 2 : 1);
            break;
        case DAGGER:
        case SILVER_DAGGER:
        case ELVEN_DAGGER:
        case ORCISH_DAGGER:
        case ATHAME:
        case KNIFE:
        case STILETTO:
        case CRYSKNIFE:
            tmp = 3;
            break;
        case PICK_AXE:
        case AXE:
            tmp = 6;
            break;
        default:
            goto no_opener;
        }
        /*KR pline("Using %s you try to open the tin.", yobjnam(uwep, (char *) 0)); */
        You("%s를 사용해서 통조림을 열려고 했다.", xname(uwep));
    } else {
no_opener:
        /*KR pline("It is not so easy to open this tin."); */
        pline("이 통조림을 여는 것은 그리 쉽지 않다.");
        if (Glib) {
            /*KR pline_The("tin slips from your %s.", fingers_or_gloves(FALSE)); */
            pline_The("통조림이 당신의 %s에서 미끄러져 떨어졌다.", fingers_or_gloves(FALSE));
            if (otmp->quan > 1L) {
                otmp = splitobj(otmp, 1L);
            }
            if (carried(otmp))
                dropx(otmp);
            else
                stackobj(otmp);
            return;
        }
        tmp = rn1(1 + 500 / ((int) (ACURR(A_DEX) + ACURRSTR)), 10);
    }

    context.tin.tin = otmp;
    context.tin.o_id = otmp->o_id;
    if (!tmp) {
        consume_tin(mesg); /* begin immediately */
    } else {
        context.tin.reqtime = tmp;
        context.tin.usedtime = 0;
        /*KR set_occupation(opentin, "opening the tin", 0); */
        set_occupation(opentin, "통조림을 연다", 0);
    }
    return;
}

/* called when waking up after fainting */
int
Hear_again(VOID_ARGS)
{
    /* Chance of deafness going away while fainted/sleeping/etc. */
    if (!rn2(2)) {
        make_deaf(0L, FALSE);
        context.botl = TRUE;
    }
    return 0;
}

/* called on the "first bite" of rotten food */
STATIC_OVL int
rottenfood(obj)
struct obj *obj;
{
    /*KR pline("Blecch!  Rotten %s!", foodword(obj)); */
    pline("웩! 썩은 %s(이)다!", foodword(obj));
    if (!rn2(4)) {
        if (Hallucination)
            /*KR You_feel("rather trippy."); */
            You("좀 맛이 갔다.");
        else
            /*KR You_feel("rather %s.", body_part(LIGHT_HEADED)); */
            You("%s.", body_part(LIGHT_HEADED));
        make_confused(HConfusion + d(2, 4), FALSE);
    } else if (!rn2(4) && !Blind) {
        /*KR pline("Everything suddenly goes dark."); */
        pline("갑자기 모든 것이 캄캄해졌다.");
        /* hero is not Blind, but Blinded timer might be nonzero if
           blindness is being overridden by the Eyes of the Overworld */
        make_blinded((Blinded & TIMEOUT) + (long) d(2, 10), FALSE);
        if (!Blind)
            Your1(vision_clears);
    } else if (!rn2(3)) {
        const char *what, *where;
        int duration = rnd(10);

        if (!Blind)
            /*KR what = "goes", where = "dark"; */
            what = "되었다", where = "캄캄하게 ";
        else if (Levitation || Is_airlevel(&u.uz) || Is_waterlevel(&u.uz))
            /*KR what = "you lose control of", where = "yourself"; */
            what = "자제하지 못하게 됐다", where = "자신을 ";
        else
            /*KR what = "you slap against the", 
            where = (u.usteed) ? "saddle" : surface(u.ux, u.uy);
        pline_The("world spins and %s %s.", what, where); */
            what = "에 부딪혔다",
            where = (u.usteed) ? "안장" : surface(u.ux, u.uy);
        pline("세계가 돌고 돌더니 %s%s.", where, what);
        incr_itimeout(&HDeaf, duration);
        context.botl = TRUE;
        nomul(-duration);
        /*KR multi_reason = "unconscious from rotten food";
        nomovemsg = "You are conscious again."; */
        multi_reason = "썩은 음식을 먹고 의식을 잃은 동안";
        nomovemsg = "당신은 정신을 되찾았다.";
        afternmv = Hear_again;
        return 1;
    }
    return 0;
}

/* called when a corpse is selected as food */
STATIC_OVL int
eatcorpse(otmp)
struct obj *otmp;
{
    int retcode = 0, tp = 0, mnum = otmp->corpsenm;
    long rotted = 0L;
    boolean stoneable = (flesh_petrifies(&mons[mnum]) && !Stone_resistance
                         && !poly_when_stoned(youmonst.data)),
            slimeable = (mnum == PM_GREEN_SLIME && !Slimed && !Unchanging
                         && !slimeproof(youmonst.data)),
            glob = otmp->globby ? TRUE : FALSE;

    /* KMH, conduct */
    if (!vegan(&mons[mnum]))
        u.uconduct.unvegan++;
    if (!vegetarian(&mons[mnum]))
        violated_vegetarian();

    if (!nonrotting_corpse(mnum)) {
        long age = peek_at_iced_corpse_age(otmp);

        rotted = (monstermoves - age) / (10L + rn2(20));
        if (otmp->cursed)
            rotted += 2L;
        else if (otmp->blessed)
            rotted -= 2L;
    }

    if (mnum != PM_ACID_BLOB && !stoneable && !slimeable && rotted > 5L) {
        boolean cannibal = maybe_cannibal(mnum, FALSE);

#if 0 /*KR*/
        pline("Ulch - that %s was tainted%s!",
              (mons[mnum].mlet == S_FUNGUS) ? "fungoid vegetation"
                  : glob ? "glob"
                      : vegetarian(&mons[mnum]) ? "protoplasm"
                          : "meat",
              cannibal ? ", you cannibal" : "");
#else /*한국어에서는 단순하게~ */
        pline("으윽! 이건 썩었다! %s",
            cannibal ? "게다가 동족상잔이라니!" : "");
#endif
        if (Sick_resistance) {
            /*KR pline("It doesn't seem at all sickening, though..."); */
            pline("전혀 병에 걸릴 것 같지는 않지만서도...");
        } else {
            long sick_time;

            sick_time = (long) rn1(10, 10);
            /* make sure new ill doesn't result in improvement */
            if (Sick && (sick_time > Sick))
                sick_time = (Sick > 1L) ? Sick - 1L : 1L;
#if 0 /*KR:T*/
            make_sick(sick_time, corpse_xname(otmp, "rotted", CXN_NORMAL),
                      TRUE, SICK_VOMITABLE);
#else
            make_sick(sick_time, corpse_xname(otmp, "썩은", CXN_NORMAL),
                TRUE, SICK_VOMITABLE);
#endif

            /*KR pline("(It must have died too long ago to be safe to eat.)"); */
            pline("(안전하게 먹기엔 유통기한이 한참 지난 것 같다.)");
        }
        if (carried(otmp))
            useup(otmp);
        else
            useupf(otmp, 1L);
        return 2;
    } else if (acidic(&mons[mnum]) && !Acid_resistance) {
        tp++;
#if 0 /*KR:T*/
        You("have a very bad case of stomach acid.");   /* not body_part() */
#else
        pline("위산으로 인한 속쓰림이 매우 심하다.");   /* not body_part() */
#endif
#if 0 /*KR:T*/
        losehp(rnd(15), !glob ? "acidic corpse" : "acidic glob",
               KILLED_BY_AN); /* acid damage */
#else /*한국어에서는 구별하지 않음*/
        losehp(rnd(15), "산성 시체로", KILLED_BY_AN); /* acid damage */
#endif
    } else if (poisonous(&mons[mnum]) && rn2(5)) {
        tp++;
        /*KR pline("Ecch - that must have been poisonous!"); */
        pline("으엑 - Ecch - 독이 들었던 게 분명해!");
        if (!Poison_resistance) {
            losestr(rnd(4));
#if 0 /*KR:T*/
            losehp(rnd(15), !glob ? "poisonous corpse" : "poisonous glob",
                   KILLED_BY_AN);
#else
            losehp(rnd(15), "독성 시체로", KILLED_BY_AN);
#endif
        } else
            /*KR You("seem unaffected by the poison."); */
            You("독에 영향받지 않은 것 같다.");
    /* now any corpse left too long will make you mildly ill */
    } else if ((rotted > 5L || (rotted > 3L && rn2(5))) && !Sick_resistance) {
        tp++;
        /*KR You_feel("%ssick.", (Sick) ? "very " : ""); 
        losehp(rnd(8), !glob ? "cadaver" : "rotted glob", KILLED_BY_AN); */
        You("몸이 %s좋지 않다.", (Sick) ? "엄청 " : "");
        losehp(rnd(8), "부패한 시체로", KILLED_BY_AN);
    }

    /* delay is weight dependent */
    context.victual.reqtime = 3 + ((!glob ? mons[mnum].cwt : otmp->owt) >> 6);

    if (!tp && !nonrotting_corpse(mnum) && (otmp->orotten || !rn2(7))) {
        if (rottenfood(otmp)) {
            otmp->orotten = TRUE;
            (void) touchfood(otmp);
            retcode = 1;
        }

        if (!mons[otmp->corpsenm].cnutrit) {
            /* no nutrition: rots away, no message if you passed out */
            if (!retcode)
                /*KR pline_The("corpse rots away completely."); */
                pline("시체가 완전히 썩어버렸다.");
            if (carried(otmp))
                useup(otmp);
            else
                useupf(otmp, 1L);
            retcode = 2;
        }

        if (!retcode)
            consume_oeaten(otmp, 2); /* oeaten >>= 2 */
    } else if ((mnum == PM_COCKATRICE || mnum == PM_CHICKATRICE)
               && (Stone_resistance || Hallucination)) {
        /*KR pline("This tastes just like chicken!"); */
        pline("치킨 맛이 나네!");
    } else if (mnum == PM_FLOATING_EYE && u.umonnum == PM_RAVEN) {
        /*KR You("peck the eyeball with delight."); */
        You("행복하게 눈알을 쪼아 먹었다.");
    } else {
        /* yummy is always False for omnivores, palatable always True */
        boolean yummy = (vegan(&mons[mnum])
                            ? (!carnivorous(youmonst.data)
                               && herbivorous(youmonst.data))
                            : (carnivorous(youmonst.data)
                               && !herbivorous(youmonst.data))),
            palatable = ((vegetarian(&mons[mnum])
                          ? herbivorous(youmonst.data)
                          : carnivorous(youmonst.data))
                         && rn2(10)
                         && ((rotted < 1) ? TRUE : !rn2(rotted+1)));
        const char *pmxnam = food_xname(otmp, FALSE);

#if 0 /*KR*/
        if (!strncmpi(pmxnam, "the ", 4))
            pmxnam += 4;
        pline("%s%s %s %s%c",
              type_is_pname(&mons[mnum])
                 ? "" : the_unique_pm(&mons[mnum]) ? "The " : "This ",
              pmxnam,
              Hallucination ? "is" : "tastes",
                  /* tiger reference is to TV ads for "Frosted Flakes",
                     breakfast cereal targeted at kids by "Tony the tiger" */
              Hallucination
                 ? (yummy ? ((u.umonnum == PM_TIGER) ? "gr-r-reat" : "gnarly")
                          : palatable ? "copacetic" : "grody")
                 : (yummy ? "delicious" : palatable ? "okay" : "terrible"),
              (yummy || !palatable) ? '!' : '.');
#else
        pline("이 %s는 %s%s",
              pmxnam,
              Hallucination
                ? (yummy ? ((u.umonnum == PM_TIGER) ? "좋았어!" : "별로 안 좋다")
                         : palatable ? "그저 그렇다" : "메스껍다")
                : (yummy ? "엄청 맛있다" : palatable ? "그저 그렇다" : "끔찍한 맛이다"),
              (yummy || !palatable) ? "！" : "．");
#endif
    }

    return retcode;
}

/* called as you start to eat */
STATIC_OVL void
start_eating(otmp, already_partly_eaten)
struct obj *otmp;
boolean already_partly_eaten;
{
    const char *old_nomovemsg, *save_nomovemsg;

    debugpline2("start_eating: %s (victual = %s)",
                /* note: fmt_ptr() returns a static buffer but supports
                   several such so we don't need to copy the first result
                   before calling it a second time */
                fmt_ptr((genericptr_t) otmp),
                fmt_ptr((genericptr_t) context.victual.piece));
    debugpline1("reqtime = %d", context.victual.reqtime);
    debugpline1("(original reqtime = %d)", objects[otmp->otyp].oc_delay);
    debugpline1("nmod = %d", context.victual.nmod);
    debugpline1("oeaten = %d", otmp->oeaten);
    context.victual.fullwarn = context.victual.doreset = FALSE;
    context.victual.eating = TRUE;

    if (otmp->otyp == CORPSE || otmp->globby) {
        cprefx(context.victual.piece->corpsenm);
        if (!context.victual.piece || !context.victual.eating) {
            /* rider revived, or died and lifesaved */
            return;
        }
    }

    old_nomovemsg = nomovemsg;
    if (bite()) {
        /* survived choking, finish off food that's nearly done;
           need this to handle cockatrice eggs, fortune cookies, etc */
        if (++context.victual.usedtime >= context.victual.reqtime) {
            /* don't want done_eating() to issue nomovemsg if it
               is due to vomit() called by bite() */
            save_nomovemsg = nomovemsg;
            if (!old_nomovemsg)
                nomovemsg = 0;
            done_eating(FALSE);
            if (!old_nomovemsg)
                nomovemsg = save_nomovemsg;
        }
        return;
    }

    if (++context.victual.usedtime >= context.victual.reqtime) {
        /* print "finish eating" message if they just resumed -dlc */
        done_eating((context.victual.reqtime > 1
                     || already_partly_eaten) ? TRUE : FALSE);
        return;
    }

    /*KR Sprintf(msgbuf, "eating %s", food_xname(otmp, TRUE)); */
    Sprintf(msgbuf, "%s을/를 섭취", food_xname(otmp, TRUE));
    set_occupation(eatfood, msgbuf, 0);
}

/*
 * Called on "first bite" of (non-corpse) food, after touchfood() has
 * marked it 'partly eaten'.  Used for non-rotten non-tin non-corpse food.
 * Messages should use present tense since multi-turn food won't be
 * finishing at the time they're issued.
 */
STATIC_OVL void
fprefx(otmp)
struct obj *otmp;
{
    switch (otmp->otyp) {
    case FOOD_RATION: /* nutrition 800 */
        /* 200+800 remains below 1000+1, the satiation threshold */
        if (u.uhunger <= 200)
#if 0 /*KR:T*/
            pline("%s!", Hallucination ? "Oh wow, like, superior, man"
                                       : "This food really hits the spot");
#else
            pline("%s!", Hallucination ? "우와, 대박. 완전 짱이다"
                                       : "이 음식은 정말 맛있다");
#endif

        /* 700-1+800 remains below 1500, the choking threshold which
           triggers "you're having a hard time getting it down" feedback */
        else if (u.uhunger < 700)
            /*KR pline("This satiates your %s!", body_part(STOMACH)); */
            pline("당신은 배부르게 되었다!");
        /* [satiation message may be inaccurate if eating gets interrupted] */
        break;
    case TRIPE_RATION:
        if (carnivorous(youmonst.data) && !humanoid(youmonst.data)) {
            /*KR pline("This tripe ration is surprisingly good!"); */
            /*tripe nation 개밥, 양곱창, 내장더미, 사료 등 의견이 다양함 */
            pline("이 개밥은 의외로 놀랄 만큼 맛있다!");
        } else if (maybe_polyd(is_orc(youmonst.data), Race_if(PM_ORC))) {
            /*KR pline(Hallucination ? "Tastes great!  Less filling!"
                                : "Mmm, tripe... not bad!"); */
            pline(Hallucination ? "마시쩡! 더 머꼬시포!"
                                : "아, 개밥... 나쁘지 않지!");
        } else {
            /*KR pline("Yak - dog food!"); */
            pline("웩 - 개나 줘버려!");
            more_experienced(1, 0);
            newexplevel();
            /* not cannibalism, but we use similar criteria
               for deciding whether to be sickened by this meal */
            if (rn2(2) && !CANNIBAL_ALLOWED())
                make_vomiting((long) rn1(context.victual.reqtime, 14), FALSE);
        }
        break;
    case LEMBAS_WAFER:
        if (maybe_polyd(is_orc(youmonst.data), Race_if(PM_ORC))) {
            /*KR pline("%s", "!#?&* elf kibble!"); */
            pline("%s", "귀쟁이 놈들의 !#?&* 개밥 따위!");
            break;
        } else if (maybe_polyd(is_elf(youmonst.data), Race_if(PM_ELF))) {
            /*KR pline("A little goes a long way."); */
            pline("작은 고추가 매운 법이지.");
            break;
        }
        goto give_feedback;
    case MEATBALL:
    case MEAT_STICK:
    case HUGE_CHUNK_OF_MEAT:
    case MEAT_RING:
        goto give_feedback;
    case CLOVE_OF_GARLIC:
        if (is_undead(youmonst.data)) {
            make_vomiting((long) rn1(context.victual.reqtime, 5), FALSE);
            break;
        }
        /*FALLTHRU*/
    default:
        if (otmp->otyp == SLIME_MOLD && !otmp->cursed
            && otmp->spe == context.current_fruit) {
#if 0 /*KR:T*/
            pline("My, this is a %s %s!",
                  Hallucination ? "primo" : "yummy",
                  singular(otmp, xname));
#else
            pline("이야~, 정말 %s %s!",
                Hallucination ? "고오급스런" : "맛인걸",
                singular(otmp, xname));
#endif
        } else if (otmp->otyp == APPLE && otmp->cursed && !Sleep_resistance) {
            ; /* skip core joke; feedback deferred til fpostfx() */

#if defined(MAC) || defined(MACOSX)
        /* KMH -- Why should Unix have all the fun?
           We check MACOSX before UNIX to get the Apple-specific apple
           message; the '#if UNIX' code will still kick in for pear. */
        } else if (otmp->otyp == APPLE) {
            /*KR pline("Delicious!  Must be a Macintosh!"); */
            pline("맛있다! '매킨토시' 같아!");
#endif

#ifdef UNIX
        } else if (otmp->otyp == APPLE || otmp->otyp == PEAR) {
            if (!Hallucination) {
                pline("Core dumped.");
            } else {
                /* based on an old Usenet joke, a fake a.out manual page */
                int x = rnd(100);

                pline("%s -- core dumped.",
                      (x <= 75)
                         ? "Segmentation fault"
                         : (x <= 99)
                            ? "Bus error"
                            : "Yo' mama");
            }
#endif
        } else if (otmp->otyp == EGG && stale_egg(otmp)) {
#if 0 /*KR:T*/
            pline("Ugh.  Rotten egg."); /* perhaps others like it */
#else
            pline("우웩. 썩은 알이잖아."); /* perhaps others like it */
#endif
            /* increasing existing nausea means that it will take longer
               before eventual vomit, but also means that constitution
               will be abused more times before illness completes */
            make_vomiting((Vomiting & TIMEOUT) + (long) d(10, 4), TRUE);
        } else {
   give_feedback:
#if 0 /*KR:T*/
            pline("This %s is %s", singular(otmp, xname),
                  otmp->cursed
                     ? (Hallucination ? "grody!" : "terrible!")
                     : (otmp->otyp == CRAM_RATION
                        || otmp->otyp == K_RATION
                        || otmp->otyp == C_RATION)
                        ? "bland."
                        : Hallucination ? "gnarly!" : "delicious!");
#else
            pline("이 %s은/는 %s", singular(otmp, xname),
                otmp->cursed
                ? (Hallucination ? "메스꺼워!" : "끔찍해!")
                : (otmp->otyp == CRAM_RATION
                    || otmp->otyp == K_RATION
                    || otmp->otyp == C_RATION)
                ? "무미건조해."
                : Hallucination ? "끝내주네!" : "마시따!");
#endif
        }
        break; /* default */
    } /* switch */
}

/* increment a combat intrinsic with limits on its growth */
STATIC_OVL int
bounded_increase(old, inc, typ)
int old, inc, typ;
{
    int absold, absinc, sgnold, sgninc;

    /* don't include any amount coming from worn rings (caller handles
       'protection' differently) */
    if (uright && uright->otyp == typ && typ != RIN_PROTECTION)
        old -= uright->spe;
    if (uleft && uleft->otyp == typ && typ != RIN_PROTECTION)
        old -= uleft->spe;
    absold = abs(old), absinc = abs(inc);
    sgnold = sgn(old), sgninc = sgn(inc);

    if (absinc == 0 || sgnold != sgninc || absold + absinc < 10) {
        ; /* use inc as-is */
    } else if (absold + absinc < 20) {
        absinc = rnd(absinc); /* 1..n */
        if (absold + absinc < 10)
            absinc = 10 - absold;
        inc = sgninc * absinc;
    } else if (absold + absinc < 40) {
        absinc = rn2(absinc) ? 1 : 0;
        if (absold + absinc < 20)
            absinc = rnd(20 - absold);
        inc = sgninc * absinc;
    } else {
        inc = 0; /* no further increase allowed via this method */
    }
    /* put amount from worn rings back */
    if (uright && uright->otyp == typ && typ != RIN_PROTECTION)
        old += uright->spe;
    if (uleft && uleft->otyp == typ && typ != RIN_PROTECTION)
        old += uleft->spe;
    return old + inc;
}

STATIC_OVL void
accessory_has_effect(otmp)
struct obj *otmp;
{
#if 0 /*KR:T*/
    pline("Magic spreads through your body as you digest the %s.",
        (otmp->oclass == RING_CLASS) ? "ring" : "amulet");
#else
    pline("당신이 %s를 소화시키자, 그것의 마법이 온 몸에 감돌았다．",
        otmp->oclass == RING_CLASS ? "반지" : "부적");
#endif
}

STATIC_OVL void
eataccessory(otmp)
struct obj *otmp;
{
    int typ = otmp->otyp;
    long oldprop;

    /* Note: rings are not so common that this is unbalancing. */
    /* (How often do you even _find_ 3 rings of polymorph in a game?) */
    oldprop = u.uprops[objects[typ].oc_oprop].intrinsic;
    if (otmp == uleft || otmp == uright) {
        Ring_gone(otmp);
        if (u.uhp <= 0)
            return; /* died from sink fall */
    }
    otmp->known = otmp->dknown = 1; /* by taste */
    if (!rn2(otmp->oclass == RING_CLASS ? 3 : 5)) {
        switch (otmp->otyp) {
        default:
            if (!objects[typ].oc_oprop)
                break; /* should never happen */

            if (!(u.uprops[objects[typ].oc_oprop].intrinsic & FROMOUTSIDE))
                accessory_has_effect(otmp);

            u.uprops[objects[typ].oc_oprop].intrinsic |= FROMOUTSIDE;

            switch (typ) {
            case RIN_SEE_INVISIBLE:
                set_mimic_blocking();
                see_monsters();
                if (Invis && !oldprop && !ESee_invisible
                    && !perceives(youmonst.data) && !Blind) {
                    newsym(u.ux, u.uy);
                    /*KR pline("Suddenly you can see yourself."); */
                    pline("갑자기 당신은 스스로를 볼 수 있게 되었다.");
                    makeknown(typ);
                }
                break;
            case RIN_INVISIBILITY:
                if (!oldprop && !EInvis && !BInvis && !See_invisible
                    && !Blind) {
                    newsym(u.ux, u.uy);
#if 0 /*KR:T*/
                    Your("body takes on a %s transparency...",
                         Hallucination ? "normal" : "strange");
#else
                    pline("신체의 투명도가 %s",
                        Hallucination ? "멀쩡하다." : "이상하다...");
#endif
                    makeknown(typ);
                }
                break;
            case RIN_PROTECTION_FROM_SHAPE_CHAN:
                rescham();
                break;
            case RIN_LEVITATION:
                /* undo the `.intrinsic |= FROMOUTSIDE' done above */
                u.uprops[LEVITATION].intrinsic = oldprop;
                if (!Levitation) {
                    float_up();
                    incr_itimeout(&HLevitation, d(10, 20));
                    makeknown(typ);
                }
                break;
            } /* inner switch */
            break; /* default case of outer switch */

        case RIN_ADORNMENT:
            accessory_has_effect(otmp);
            if (adjattrib(A_CHA, otmp->spe, -1))
                makeknown(typ);
            break;
        case RIN_GAIN_STRENGTH:
            accessory_has_effect(otmp);
            if (adjattrib(A_STR, otmp->spe, -1))
                makeknown(typ);
            break;
        case RIN_GAIN_CONSTITUTION:
            accessory_has_effect(otmp);
            if (adjattrib(A_CON, otmp->spe, -1))
                makeknown(typ);
            break;
        case RIN_INCREASE_ACCURACY:
            accessory_has_effect(otmp);
            u.uhitinc = (schar) bounded_increase((int) u.uhitinc, otmp->spe,
                                                 RIN_INCREASE_ACCURACY);
            break;
        case RIN_INCREASE_DAMAGE:
            accessory_has_effect(otmp);
            u.udaminc = (schar) bounded_increase((int) u.udaminc, otmp->spe,
                                                 RIN_INCREASE_DAMAGE);
            break;
        case RIN_PROTECTION:
            accessory_has_effect(otmp);
            HProtection |= FROMOUTSIDE;
            u.ublessed = bounded_increase(u.ublessed, otmp->spe,
                                          RIN_PROTECTION);
            context.botl = 1;
            break;
        case RIN_FREE_ACTION:
            /* Give sleep resistance instead */
            if (!(HSleep_resistance & FROMOUTSIDE))
                accessory_has_effect(otmp);
            if (!Sleep_resistance)
                /*KR You_feel("wide awake."); */
                You("정신이 말똥말똥하다.");
            HSleep_resistance |= FROMOUTSIDE;
            break;
        case AMULET_OF_CHANGE:
            accessory_has_effect(otmp);
            makeknown(typ);
            change_sex();
#if 0 /*JP:T*/
            You("are suddenly very %s!",
                flags.female ? "feminine" : "masculine");
#else
            You("갑자기 매우 %s졌다!",
                flags.female ? "여성스러워" : "남성스러워");
#endif
            context.botl = 1;
            break;
        case AMULET_OF_UNCHANGING:
            /* un-change: it's a pun */
            if (!Unchanging && Upolyd) {
                accessory_has_effect(otmp);
                makeknown(typ);
                rehumanize();
            }
            break;
        case AMULET_OF_STRANGULATION: /* bad idea! */
            /* no message--this gives no permanent effect */
            choke(otmp);
            break;
        case AMULET_OF_RESTFUL_SLEEP: { /* another bad idea! */
            long newnap = (long) rnd(100), oldnap = (HSleepy & TIMEOUT);

            if (!(HSleepy & FROMOUTSIDE))
                accessory_has_effect(otmp);
            HSleepy |= FROMOUTSIDE;
            /* might also be wearing one; use shorter of two timeouts */
            if (newnap < oldnap || oldnap == 0L)
                HSleepy = (HSleepy & ~TIMEOUT) | newnap;
            break;
        }
        case RIN_SUSTAIN_ABILITY:
        case AMULET_OF_LIFE_SAVING:
        case AMULET_OF_REFLECTION: /* nice try */
            /* can't eat Amulet of Yendor or fakes,
             * and no oc_prop even if you could -3.
             */
            break;
        }
    }
}

/* called after eating non-food */
STATIC_OVL void
eatspecial()
{
    struct obj *otmp = context.victual.piece;

    /* lesshungry wants an occupation to handle choke messages correctly */
    /*KR set_occupation(eatfood, "eating non-food", 0); */
    set_occupation(eatfood, "먹는다", 0);
    lesshungry(context.victual.nmod);
    occupation = 0;
    context.victual.piece = (struct obj *) 0;
    context.victual.o_id = 0;
    context.victual.eating = 0;
    if (otmp->oclass == COIN_CLASS) {
        if (carried(otmp))
            useupall(otmp);
        else
            useupf(otmp, otmp->quan);
        vault_gd_watching(GD_EATGOLD);
        return;
    }
    if (objects[otmp->otyp].oc_material == PAPER) {
#ifdef MAIL
        if (otmp->otyp == SCR_MAIL)
            /* no nutrition */
            /*KR pline("This junk mail is less than satisfying."); */
            pline("이 '스팸' 메일은 먹어도 배부르지 않아.");
        else
#endif
        if (otmp->otyp == SCR_SCARE_MONSTER)
            /* to eat scroll, hero is currently polymorphed into a monster */
            /*KR pline("Yuck%c", otmp->blessed ? '!' : '.'); */
            pline("으엑%s", otmp->blessed ? "!" : ".");
        else if (otmp->oclass == SCROLL_CLASS
                 /* check description after checking for specific scrolls */
                 && !strcmpi(OBJ_DESCR(objects[otmp->otyp]), "YUM YUM"))
            /*KR pline("Yum%c", otmp->blessed ? '!' : '.');
        else
            pline("Needs salt..."); */
            pline("냠냠%s", otmp->blessed ? "!" : ".");
        else
            pline("맛이 싱거워...");
    }
    if (otmp->oclass == POTION_CLASS) {
        otmp->quan++; /* dopotion() does a useup() */
        (void) dopotion(otmp);
    } else if (otmp->oclass == RING_CLASS || otmp->oclass == AMULET_CLASS) {
        eataccessory(otmp);
    } else if (otmp->otyp == LEASH && otmp->leashmon) {
        o_unleash(otmp);
    }

    /* KMH -- idea by "Tommy the Terrorist" */
    if (otmp->otyp == TRIDENT && !otmp->cursed) {
        /* sugarless chewing gum which used to be heavily advertised on TV */
#if 0 /*KR:T*/
        pline(Hallucination ? "Four out of five dentists agree."
            : "That was pure chewing satisfaction!");
#else
        pline(Hallucination ? "치과의사가 추천합니다. 5점 만점에 4점!"
            : "씹는 맛이 일품이네요!");
#endif
        exercise(A_WIS, TRUE);
    }
    if (otmp->otyp == FLINT && !otmp->cursed) {
        /* chewable vitamin for kids based on "The Flintstones" TV cartoon */
        /*KR pline("Yabba-dabba delicious!"); */
        pline("야바-다바-맛있다바두!");
        exercise(A_CON, TRUE);
    }

    if (otmp == uwep && otmp->quan == 1L)
        uwepgone();
    if (otmp == uquiver && otmp->quan == 1L)
        uqwepgone();
    if (otmp == uswapwep && otmp->quan == 1L)
        uswapwepgone();

    if (otmp == uball)
        unpunish();
    if (otmp == uchain)
        unpunish(); /* but no useup() */
    else if (carried(otmp))
        useup(otmp);
    else
        useupf(otmp, 1L);
}

/* NOTE: the order of these words exactly corresponds to the
   order of oc_material values #define'd in objclass.h. */
static const char* foodwords[] = {
#if 0 /*JP:T*/
    "meal",    "liquid",  "wax",       "food", "meat",     "paper",
    "cloth",   "leather", "wood",      "bone", "scale",    "metal",
    "metal",   "metal",   "silver",    "gold", "platinum", "mithril",
    "plastic", "glass",   "rich food", "stone"
#else
    "고기",       "액체",   "밀랍",      "식품",  "고기",       "종이",
    "옷",         "가죽",   "목재",      "뼈",    "비늘",       "금속",
    "금속",       "금속",   "은",        "금",    "백금",       "미스릴",
    "플라스틱",   "유리",   "고급 식품", "돌"
#endif
};

STATIC_OVL const char *
foodword(otmp)
struct obj *otmp;
{
    if (otmp->oclass == FOOD_CLASS)
        /*KR return "food"; */
        return "음식";
    if (otmp->oclass == GEM_CLASS && objects[otmp->otyp].oc_material == GLASS
        && otmp->dknown)
        makeknown(otmp->otyp);
    return foodwords[objects[otmp->otyp].oc_material];
}

/* called after consuming (non-corpse) food */
STATIC_OVL void
fpostfx(otmp)
struct obj *otmp;
{
    switch (otmp->otyp) {
    case SPRIG_OF_WOLFSBANE:
        if (u.ulycn >= LOW_PM || is_were(youmonst.data))
            you_unwere(TRUE);
        break;
    case CARROT:
        if (!u.uswallow
            || !attacktype_fordmg(u.ustuck->data, AT_ENGL, AD_BLND))
            make_blinded((long) u.ucreamed, TRUE);
        break;
    case FORTUNE_COOKIE:
        outrumor(bcsign(otmp), BY_COOKIE);
        if (!Blind)
            u.uconduct.literate++;
        break;
    case LUMP_OF_ROYAL_JELLY:
        /* This stuff seems to be VERY healthy! */
        gainstr(otmp, 1, TRUE);
        if (Upolyd) {
            u.mh += otmp->cursed ? -rnd(20) : rnd(20);
            if (u.mh > u.mhmax) {
                if (!rn2(17))
                    u.mhmax++;
                u.mh = u.mhmax;
            } else if (u.mh <= 0) {
                rehumanize();
            }
        } else {
            u.uhp += otmp->cursed ? -rnd(20) : rnd(20);
            if (u.uhp > u.uhpmax) {
                if (!rn2(17))
                    u.uhpmax++;
                u.uhp = u.uhpmax;
            } else if (u.uhp <= 0) {
                killer.format = KILLED_BY_AN;
#if 0 /*KR*/
                Strcpy(killer.name, "rotten lump of royal jelly");
                done(POISONING);
#else
                Strcpy(killer.name, "썩은 로열 젤리 덩어리를 먹고 식중독으로");
                done(DIED);
#endif
            }
        }
        if (!otmp->cursed)
            heal_legs(0);
        break;
    case EGG:
        if (flesh_petrifies(&mons[otmp->corpsenm])) {
            if (!Stone_resistance
                && !(poly_when_stoned(youmonst.data)
                     && polymon(PM_STONE_GOLEM))) {
                if (!Stoned) {
                    /*KR Sprintf(killer.name, "%s egg", */
                    Sprintf(killer.name, "%s의 알로",
                            mons[otmp->corpsenm].mname);
                    make_stoned(5L, (char *) 0, KILLED_BY_AN, killer.name);
                }
            }
            /* note: no "tastes like chicken" message for eggs */
        }
        break;
    case EUCALYPTUS_LEAF:
        if (Sick && !otmp->cursed)
            make_sick(0L, (char *) 0, TRUE, SICK_ALL);
        if (Vomiting && !otmp->cursed)
            make_vomiting(0L, TRUE);
        break;
    case APPLE:
        if (otmp->cursed && !Sleep_resistance) {
            /* Snow White; 'poisoned' applies to [a subset of] weapons,
               not food, so we substitute cursed; fortunately our hero
               won't have to wait for a prince to be rescued/revived */
            if (Race_if(PM_DWARF) && Hallucination)
                /*KR verbalize("Heigh-ho, ho-hum, I think I'll skip work today."); */
                verbalize("헤이-호, 호-흠, 오늘은 일하러 안 가야지.");
            else if (Deaf || !flags.acoustics)
                /*KR You("fall asleep."); */
                You("잠에 빠졌다.");
            else
                /*KR You_hear("sinister laughter as you fall asleep..."); */
                You_hear("잠에 빠져들며 사악한 웃음소리를 듣는다...");
            fall_asleep(-rn1(11, 20), TRUE);
        }
        break;
    }
    return;
}

#if 0
/* intended for eating a spellbook while polymorphed, but not used;
   "leather" applied to appearance, not composition, and has been
   changed to "leathery" to reflect that */
STATIC_DCL boolean FDECL(leather_cover, (struct obj *));

STATIC_OVL boolean
leather_cover(otmp)
struct obj *otmp;
{
    const char *odesc = OBJ_DESCR(objects[otmp->otyp]);

    if (odesc && (otmp->oclass == SPBOOK_CLASS)) {
        if (!strcmp(odesc, "leather"))
            return TRUE;
    }
    return FALSE;
}
#endif

/*
 * return 0 if the food was not dangerous.
 * return 1 if the food was dangerous and you chose to stop.
 * return 2 if the food was dangerous and you chose to eat it anyway.
 */
STATIC_OVL int
edibility_prompts(otmp)
struct obj *otmp;
{
    /* Blessed food detection grants hero a one-use
     * ability to detect food that is unfit for consumption
     * or dangerous and avoid it.
     */
#if 0 /*JP*/
    char buf[BUFSZ], foodsmell[BUFSZ],
        it_or_they[QBUFSZ], eat_it_anyway[QBUFSZ];
#else
    char buf[BUFSZ], foodsmell[BUFSZ],
        eat_it_anyway[QBUFSZ];
#endif   
    boolean cadaver = (otmp->otyp == CORPSE || otmp->globby),
            stoneorslime = FALSE;
    int material = objects[otmp->otyp].oc_material, mnum = otmp->corpsenm;
    long rotted = 0L;

#if 0 /*KR*/
    Strcpy(foodsmell, Tobjnam(otmp, "smell"));
#else
    Strcpy(foodsmell, xname(otmp));
#endif
#if 0 /*KR*/
    Strcpy(it_or_they, (otmp->quan == 1L) ? "it" : "they");
#endif
#if 0 /*KR*/
    Sprintf(eat_it_anyway, "Eat %s anyway?",
        (otmp->quan == 1L) ? "it" : "one");
#else
    Strcpy(eat_it_anyway, "그래도 먹겠습니까?");
#endif

    if (cadaver || otmp->otyp == EGG || otmp->otyp == TIN) {
        /* These checks must match those in eatcorpse() */
        stoneorslime = (flesh_petrifies(&mons[mnum]) && !Stone_resistance
                        && !poly_when_stoned(youmonst.data));

        if (mnum == PM_GREEN_SLIME || otmp->otyp == GLOB_OF_GREEN_SLIME)
            stoneorslime = (!Unchanging && !slimeproof(youmonst.data));

        if (cadaver && !nonrotting_corpse(mnum)) {
            long age = peek_at_iced_corpse_age(otmp);

            /* worst case rather than random
               in this calculation to force prompt */
            rotted = (monstermoves - age) / (10L + 0 /* was rn2(20) */);
            if (otmp->cursed)
                rotted += 2L;
            else if (otmp->blessed)
                rotted -= 2L;
        }
    }

    /*
     * These problems with food should be checked in
     * order from most detrimental to least detrimental.
     */
    if (cadaver && mnum != PM_ACID_BLOB && rotted > 5L && !Sick_resistance) {
        /* Tainted meat */
#if 0 /*KR:T*/
        Sprintf(buf, "%s like %s could be tainted!  %s", foodsmell, it_or_they,
                eat_it_anyway);
#else
        Sprintf(buf, "%s은/는 오염된 듯한 냄새가 난다! %s", foodsmell, eat_it_anyway);
#endif
        if (yn_function(buf, ynchars, 'n') == 'n')
            return 1;
        else
            return 2;
    }
    if (stoneorslime) {
#if 0 /*KR:T*/
        Sprintf(buf, "%s like %s could be something very dangerous!  %s",
                foodsmell, it_or_they, eat_it_anyway);
#else
        Sprintf(buf, "%s은/는 왠지 엄청 위험한 냄새가 난다! %s",
                foodsmell, eat_it_anyway);
#endif
        if (yn_function(buf, ynchars, 'n') == 'n')
            return 1;
        else
            return 2;
    }
    if (otmp->orotten || (cadaver && rotted > 3L)) {
        /* Rotten */
#if 0 /*KR:T*/
        Sprintf(buf, "%s like %s could be rotten! %s", foodsmell, it_or_they,
            eat_it_anyway);
#else
        Sprintf(buf, "%s은/는 썩은 냄새가 난다! %s", foodsmell,
            eat_it_anyway);
#endif
        if (yn_function(buf, ynchars, 'n') == 'n')
            return 1;
        else
            return 2;
    }
    if (cadaver && poisonous(&mons[mnum]) && !Poison_resistance) {
        /* poisonous */
#if 0 /*KR:T*/
        Sprintf(buf, "%s like %s might be poisonous!  %s", foodsmell,
            it_or_they, eat_it_anyway);
#else
        Sprintf(buf, "%s은/는 독이 든 것 같은 냄새가 난다! %s", foodsmell,
            eat_it_anyway);
#endif
        if (yn_function(buf, ynchars, 'n') == 'n')
            return 1;
        else
            return 2;
    }
    if (otmp->otyp == APPLE && otmp->cursed && !Sleep_resistance) {
        /* causes sleep, for long enough to be dangerous */
#if 0 /*KR:T*/
        Sprintf(buf, "%s like %s might have been poisoned.  %s", foodsmell,
            it_or_they, eat_it_anyway);
#else
        Sprintf(buf, "%s은/는 독이 든 것 같은 냄새가 난다! %s", foodsmell,
            eat_it_anyway);
#endif
        return (yn_function(buf, ynchars, 'n') == 'n') ? 1 : 2;
    }
    if (cadaver && !vegetarian(&mons[mnum]) && !u.uconduct.unvegetarian
        && Role_if(PM_MONK)) {
        /*KR Sprintf(buf, "%s unhealthy.  %s", foodsmell, eat_it_anyway); */
        Sprintf(buf, "%s은/는 건강에 나쁜 냄새가 난다. %s", foodsmell, eat_it_anyway);
        if (yn_function(buf, ynchars, 'n') == 'n')
            return 1;
        else
            return 2;
    }
    if (cadaver && acidic(&mons[mnum]) && !Acid_resistance) {
        /*KR Sprintf(buf, "%s rather acidic.  %s", foodsmell, eat_it_anyway); */
        Sprintf(buf, "%s은/는 조금 시큼한 냄새가 난다. %s", foodsmell, eat_it_anyway);
        if (yn_function(buf, ynchars, 'n') == 'n')
            return 1;
        else
            return 2;
    }
    if (Upolyd && u.umonnum == PM_RUST_MONSTER && is_metallic(otmp)
        && otmp->oerodeproof) {
#if 0 /*KR:T*/
        Sprintf(buf, "%s disgusting to you right now.  %s", foodsmell,
            eat_it_anyway);
#else
        Sprintf(buf, "%s은/는 역겨운 냄새가 난다. %s", foodsmell,
            eat_it_anyway);
#endif
        if (yn_function(buf, ynchars, 'n') == 'n')
            return 1;
        else
            return 2;
    }

    /*
     * Breaks conduct, but otherwise safe.
     */
    if (!u.uconduct.unvegan
        && ((material == LEATHER || material == BONE
             || material == DRAGON_HIDE || material == WAX)
            || (cadaver && !vegan(&mons[mnum])))) {
#if 0 /*KR:T*/
        Sprintf(buf, "%s foul and unfamiliar to you.  %s", foodsmell,
            eat_it_anyway);
#else
        Sprintf(buf, "%s에서 익숙하지 않은 악취가 난다. %s", foodsmell,
            eat_it_anyway);
#endif
        if (yn_function(buf, ynchars, 'n') == 'n')
            return 1;
        else
            return 2;
    }
    if (!u.uconduct.unvegetarian
        && ((material == LEATHER || material == BONE
             || material == DRAGON_HIDE)
            || (cadaver && !vegetarian(&mons[mnum])))) {
        /*KR Sprintf(buf, "%s unfamiliar to you.  %s", foodsmell, eat_it_anyway); */
        Sprintf(buf, "%s에서 익숙하지 않은 냄새가 난다. %s", foodsmell, eat_it_anyway);
        if (yn_function(buf, ynchars, 'n') == 'n')
            return 1;
        else
            return 2;
    }

    if (cadaver && mnum != PM_ACID_BLOB && rotted > 5L && Sick_resistance) {
        /* Tainted meat with Sick_resistance */
#if 0 /*KR:T*/
        Sprintf(buf, "%s like %s could be tainted!  %s",
            foodsmell, it_or_they, eat_it_anyway);
#else
        Sprintf(buf, "%s은/는 오염된 듯한 냄새가 난다! %s",
            foodsmell, eat_it_anyway);
#endif
        if (yn_function(buf, ynchars, 'n') == 'n')
            return 1;
        else
            return 2;
    }
    return 0;
}

/* 'e' command */
int
doeat()
{
    struct obj *otmp;
    int basenutrit; /* nutrition of full item */
    boolean dont_start = FALSE, nodelicious = FALSE,
            already_partly_eaten;

    if (Strangled) {
        /*KR pline("If you can't breathe air, how can you consume solids?"); */
        pline("숨도 못 쉬는데, 음식은 어떻게 먹으려고?");
        return 0;
    }
    if (!(otmp = floorfood("eat", 0)))
        return 0;
    if (check_capacity((char *) 0))
        return 0;

    if (u.uedibility) {
        int res = edibility_prompts(otmp);

        if (res) {
#if 0 /*KR*/
            Your(
                "%s stops tingling and your sense of smell returns to normal.",
                body_part(NOSE));
#else
            Your("%s가 따끔거리는 것을 멈추고, 후각이 정상으로 돌아왔다.",
                body_part(NOSE));
#endif
            u.uedibility = 0;
            if (res == 1)
                return 0;
        }
    }

    /* We have to make non-foods take 1 move to eat, unless we want to
     * do ridiculous amounts of coding to deal with partly eaten plate
     * mails, players who polymorph back to human in the middle of their
     * metallic meal, etc....
     */
    if (!is_edible(otmp)) {
        /*KR You("cannot eat that!"); */
        You("그것을 먹을 수는 없다!");
        return 0;
    } else if ((otmp->owornmask & (W_ARMOR | W_TOOL | W_AMUL | W_SADDLE))
               != 0) {
        /* let them eat rings */
        /*KR You_cant("eat %s you're wearing.", something); */
        You("입고 있는 것을 먹을 수는 없다.");
        return 0;
    } else if (!(carried(otmp) ? retouch_object(&otmp, FALSE)
                               : touch_artifact(otmp, &youmonst))) {
        return 1; /* got blasted so use a turn */
    }
    if (is_metallic(otmp) && u.umonnum == PM_RUST_MONSTER
        && otmp->oerodeproof) {
        otmp->rknown = TRUE;
        if (otmp->quan > 1L) {
            if (!carried(otmp))
                (void) splitobj(otmp, otmp->quan - 1L);
            else
                otmp = splitobj(otmp, 1L);
        }
        /*KR pline("Ulch - that %s was rustproofed!", xname(otmp)); */
        pline("으엑... %s에 방녹 처리가 되어있었잖아!", xname(otmp));
        /* The regurgitated object's rustproofing is gone now */
        otmp->oerodeproof = 0;
        make_stunned((HStun & TIMEOUT) + (long) rn2(10), TRUE);
        /*
         * We don't expect rust monsters to be wielding welded weapons
         * or wearing cursed rings which were rustproofed, but guard
         * against the possibility just in case.
         */
        if (welded(otmp) || (otmp->cursed && (otmp->owornmask & W_RING))) {
            set_bknown(otmp, 1); /* for ring; welded() does this for weapon */
            /*KR You("spit out %s.", the(xname(otmp))); */
            You("%s을/를 뱉어냈다.", xname(otmp));
        } else {
#if 0 /*KR:T*/
            You("spit %s out onto the %s.", the(xname(otmp)),
                surface(u.ux, u.uy));
#else
            You("%s을/를 %s에 뱉어냈다.", the(xname(otmp)),
                surface(u.ux, u.uy));
#endif
            if (carried(otmp)) {
                /* no need to check for leash in use; it's not metallic */
                if (otmp->owornmask)
                    remove_worn_item(otmp, FALSE);
                freeinv(otmp);
                dropy(otmp);
            }
            stackobj(otmp);
        }
        return 1;
    }
    /* KMH -- Slow digestion is... indigestible */
    if (otmp->otyp == RIN_SLOW_DIGESTION) {
        /*KR pline("This ring is indigestible!"); */
        pline("이 반지는 소화시키기 어렵다!");
        (void) rottenfood(otmp);
        if (otmp->dknown && !objects[otmp->otyp].oc_name_known
            && !objects[otmp->otyp].oc_uname)
            docall(otmp);
        return 1;
    }
    if (otmp->oclass != FOOD_CLASS) {
        int material;

        context.victual.reqtime = 1;
        context.victual.piece = otmp;
        context.victual.o_id = otmp->o_id;
        /* Don't split it, we don't need to if it's 1 move */
        context.victual.usedtime = 0;
        context.victual.canchoke = (u.uhs == SATIATED);
        /* Note: gold weighs 1 pt. for each 1000 pieces (see
           pickup.c) so gold and non-gold is consistent. */
        if (otmp->oclass == COIN_CLASS)
            basenutrit = ((otmp->quan > 200000L)
                             ? 2000
                             : (int) (otmp->quan / 100L));
        else if (otmp->oclass == BALL_CLASS || otmp->oclass == CHAIN_CLASS)
            basenutrit = weight(otmp);
        /* oc_nutrition is usually weight anyway */
        else
            basenutrit = objects[otmp->otyp].oc_nutrition;
#ifdef MAIL
        if (otmp->otyp == SCR_MAIL) {
            basenutrit = 0;
            nodelicious = TRUE;
        }
#endif
        context.victual.nmod = basenutrit;
        context.victual.eating = TRUE; /* needed for lesshungry() */

        material = objects[otmp->otyp].oc_material;
        if (material == LEATHER || material == BONE
            || material == DRAGON_HIDE) {
            u.uconduct.unvegan++;
            violated_vegetarian();
        } else if (material == WAX)
            u.uconduct.unvegan++;
        u.uconduct.food++;

        if (otmp->cursed) {
            (void) rottenfood(otmp);
            nodelicious = TRUE;
        } else if (objects[otmp->otyp].oc_material == PAPER)
            nodelicious = TRUE;

        if (otmp->oclass == WEAPON_CLASS && otmp->opoisoned) {
            /*KR pline("Ecch - that must have been poisonous!"); */
            pline("으엑, 독이 들었던 게 틀림없어!");
            if (!Poison_resistance) {
                losestr(rnd(4));
#if 0 /*KR*/
                losehp(rnd(15), xname(otmp), KILLED_BY_AN);
#else
                {
                    char jbuf[BUFSZ];
                    Sprintf(jbuf, "%s(으)로", xname(otmp));
                    losehp(rnd(15), jbuf, KILLED_BY_AN);
                }
#endif
            } else
                /*KR You("seem unaffected by the poison."); */
                You("독에 영향받지 않는 것 같다.");
        } else if (!nodelicious) {
#if 0 /*KR*/
            pline("%s%s is delicious!",
                (obj_is_pname(otmp)
                    && otmp->oartifact < ART_ORB_OF_DETECTION)
                ? ""
                : "This ",
                (otmp->oclass == COIN_CLASS)
                ? foodword(otmp)
                : singular(otmp, xname));
#else
            pline("%s%s은/는 맛있다!",
                (obj_is_pname(otmp)
                    && otmp->oartifact < ART_ORB_OF_DETECTION)
                ? ""
                : "이 ",
                (otmp->oclass == COIN_CLASS)
                ? foodword(otmp)
                : singular(otmp, xname));
#endif
        }
        eatspecial();
        return 1;
    }

    if (otmp == context.victual.piece) {
        /* If they weren't able to choke, they don't suddenly become able to
         * choke just because they were interrupted.  On the other hand, if
         * they were able to choke before, if they lost food it's possible
         * they shouldn't be able to choke now.
         */
        if (u.uhs != SATIATED)
            context.victual.canchoke = FALSE;
        context.victual.o_id = 0;
        context.victual.piece = touchfood(otmp);
        if (context.victual.piece)
            context.victual.o_id = context.victual.piece->o_id;
#if 0 /*KR:T*/
        You("resume %syour meal.",
            (context.victual.usedtime + 1 >= context.victual.reqtime)
            ? "the last bite of " : "");
#else
        You("%s 식사를 재개했다.", 
            (context.victual.usedtime + 1 >= context.victual.reqtime)
            ? "마지막 한 입 남은" : ""); 
#endif
        start_eating(context.victual.piece, FALSE);
        return 1;
    }

    /* nothing in progress - so try to find something. */
    /* tins are a special case */
    /* tins must also check conduct separately in case they're discarded */
    if (otmp->otyp == TIN) {
        start_tin(otmp);
        return 1;
    }

    /* KMH, conduct */
    u.uconduct.food++;

    already_partly_eaten = otmp->oeaten ? TRUE : FALSE;
    context.victual.o_id = 0;
    context.victual.piece = otmp = touchfood(otmp);
    if (context.victual.piece)
        context.victual.o_id = context.victual.piece->o_id;
    context.victual.usedtime = 0;

    /* Now we need to calculate delay and nutritional info.
     * The base nutrition calculated here and in eatcorpse() accounts
     * for normal vs. rotten food.  The reqtime and nutrit values are
     * then adjusted in accordance with the amount of food left.
     */
    if (otmp->otyp == CORPSE || otmp->globby) {
        int tmp = eatcorpse(otmp);

        if (tmp == 2) {
            /* used up */
            context.victual.piece = (struct obj *) 0;
            context.victual.o_id = 0;
            return 1;
        } else if (tmp)
            dont_start = TRUE;
        /* if not used up, eatcorpse sets up reqtime and may modify oeaten */
    } else {
        /* No checks for WAX, LEATHER, BONE, DRAGON_HIDE.  These are
         * all handled in the != FOOD_CLASS case, above.
         */
        switch (objects[otmp->otyp].oc_material) {
        case FLESH:
            u.uconduct.unvegan++;
            if (otmp->otyp != EGG) {
                violated_vegetarian();
            }
            break;

        default:
            if (otmp->otyp == PANCAKE || otmp->otyp == FORTUNE_COOKIE /*eggs*/
                || otmp->otyp == CREAM_PIE || otmp->otyp == CANDY_BAR /*milk*/
                || otmp->otyp == LUMP_OF_ROYAL_JELLY)
                u.uconduct.unvegan++;
            break;
        }

        context.victual.reqtime = objects[otmp->otyp].oc_delay;
        if (otmp->otyp != FORTUNE_COOKIE
            && (otmp->cursed || (!nonrotting_food(otmp->otyp)
                                 && (monstermoves - otmp->age)
                                        > (otmp->blessed ? 50L : 30L)
                                 && (otmp->orotten || !rn2(7))))) {
            if (rottenfood(otmp)) {
                otmp->orotten = TRUE;
                dont_start = TRUE;
            }
            consume_oeaten(otmp, 1); /* oeaten >>= 1 */
        } else if (!already_partly_eaten) {
            fprefx(otmp);
        } else {
#if 0 /*KR:T*/
            You("%s %s.",
                (context.victual.reqtime == 1) ? "eat" : "begin eating",
                doname(otmp));
#else
            You("%s을/를 %s.",
                doname(otmp),
                (context.victual.reqtime == 1) ? "먹었다" : "먹기 시작했다");
#endif
        }
    }

    /* re-calc the nutrition */
    basenutrit = (int) obj_nutrition(otmp);

    debugpline3(
     "before rounddiv: victual.reqtime == %d, oeaten == %d, basenutrit == %d",
                context.victual.reqtime, otmp->oeaten, basenutrit);

    context.victual.reqtime = (basenutrit == 0) ? 0
        : rounddiv(context.victual.reqtime * (long) otmp->oeaten, basenutrit);

    debugpline1("after rounddiv: victual.reqtime == %d",
                context.victual.reqtime);
    /*
     * calculate the modulo value (nutrit. units per round eating)
     * note: this isn't exact - you actually lose a little nutrition due
     *       to this method.
     * TODO: add in a "remainder" value to be given at the end of the meal.
     */
    if (context.victual.reqtime == 0 || otmp->oeaten == 0)
        /* possible if most has been eaten before */
        context.victual.nmod = 0;
    else if ((int) otmp->oeaten >= context.victual.reqtime)
        context.victual.nmod = -((int) otmp->oeaten
                                 / context.victual.reqtime);
    else
        context.victual.nmod = context.victual.reqtime % otmp->oeaten;
    context.victual.canchoke = (u.uhs == SATIATED);

    if (!dont_start)
        start_eating(otmp, already_partly_eaten);
    return 1;
}

int
use_tin_opener(obj)
struct obj *obj;
{
    struct obj *otmp;
    int res = 0;

    if (!carrying(TIN)) {
        /*KR You("have no tin to open."); */
        You("통조림을 가지고 있지 않다.");
        return 0;
    }

    if (obj != uwep) {
        if (obj->cursed && obj->bknown) {
            char qbuf[QBUFSZ];

#if 0 /*KR:T*/
            if (ynq(safe_qbuf(qbuf, "Really wield ", "?",
                obj, doname, thesimpleoname, "that")) != 'y')
#else
            if (ynq(safe_qbuf(qbuf, "정말로", "을/를 장착하시겠습니까?",
                obj, doname, thesimpleoname, "그것")) != 'y')
#endif
                return 0;
        }
        if (!wield_tool(obj, "use"))
            return 0;
        res = 1;
    }

    otmp = getobj(comestibles, "open");
    if (!otmp)
        return res;

    start_tin(otmp);
    return 1;
}

/* Take a single bite from a piece of food, checking for choking and
 * modifying usedtime.  Returns 1 if they choked and survived, 0 otherwise.
 */
STATIC_OVL int
bite()
{
    if (context.victual.canchoke && u.uhunger >= 2000) {
        choke(context.victual.piece);
        return 1;
    }
    if (context.victual.doreset) {
        do_reset_eat();
        return 0;
    }
    force_save_hs = TRUE;
    if (context.victual.nmod < 0) {
        lesshungry(-context.victual.nmod);
        consume_oeaten(context.victual.piece,
                       context.victual.nmod); /* -= -nmod */
    } else if (context.victual.nmod > 0
               && (context.victual.usedtime % context.victual.nmod)) {
        lesshungry(1);
        consume_oeaten(context.victual.piece, -1); /* -= 1 */
    }
    force_save_hs = FALSE;
    recalc_wt();
    return 0;
}

/* as time goes by - called by moveloop(every move) & domove(melee attack) */
void
gethungry()
{
    if (u.uinvulnerable)
        return; /* you don't feel hungrier */

    /* being polymorphed into a creature which doesn't eat prevents
       this first uhunger decrement, but to stay in such form the hero
       will need to wear an Amulet of Unchanging so still burn a small
       amount of nutrition in the 'moves % 20' ring/amulet check below */
    if ((!Unaware || !rn2(10)) /* slow metabolic rate while asleep */
        && (carnivorous(youmonst.data)
            || herbivorous(youmonst.data)
            || metallivorous(youmonst.data))
        && !Slow_digestion)
        u.uhunger--; /* ordinary food consumption */

    if (moves % 2) { /* odd turns */
        /* Regeneration uses up food, unless due to an artifact */
        if ((HRegeneration & ~FROMFORM)
            || (ERegeneration & ~(W_ARTI | W_WEP)))
            u.uhunger--;
        if (near_capacity() > SLT_ENCUMBER)
            u.uhunger--;
    } else { /* even turns */
        if (Hunger)
            u.uhunger--;
        /* Conflict uses up food too */
        if (HConflict || (EConflict & (~W_ARTI)))
            u.uhunger--;
        /* +0 charged rings don't do anything, so don't affect hunger.
           Slow digestion cancels move hunger but still causes ring hunger. */
        switch ((int) (moves % 20)) { /* note: use even cases only */
        case 4:
            if (uleft && (uleft->spe || !objects[uleft->otyp].oc_charged))
                u.uhunger--;
            break;
        case 8:
            if (uamul)
                u.uhunger--;
            break;
        case 12:
            if (uright && (uright->spe || !objects[uright->otyp].oc_charged))
                u.uhunger--;
            break;
        case 16:
            if (u.uhave.amulet)
                u.uhunger--;
            break;
        default:
            break;
        }
    }
    newuhs(TRUE);
}

/* called after vomiting and after performing feats of magic */
void
morehungry(num)
int num;
{
    u.uhunger -= num;
    newuhs(TRUE);
}

/* called after eating (and after drinking fruit juice) */
void
lesshungry(num)
int num;
{
    /* See comments in newuhs() for discussion on force_save_hs */
    boolean iseating = (occupation == eatfood) || force_save_hs;

    debugpline1("lesshungry(%d)", num);
    u.uhunger += num;
    if (u.uhunger >= 2000) {
        if (!iseating || context.victual.canchoke) {
            if (iseating) {
                choke(context.victual.piece);
                reset_eat();
            } else
                choke(occupation == opentin ? context.tin.tin
                                            : (struct obj *) 0);
            /* no reset_eat() */
        }
    } else {
        /* Have lesshungry() report when you're nearly full so all eating
         * warns when you're about to choke.
         */
        if (u.uhunger >= 1500
            && (!context.victual.eating
                || (context.victual.eating && !context.victual.fullwarn))) {
            /*KR pline("You're having a hard time getting all of it down.");
            nomovemsg = "You're finally finished."; */
            pline("당신은 전부 소화시키느라 고생하고 있다.");
            nomovemsg = "당신은 마침내 다 먹었다.";
            if (!context.victual.eating) {
                multi = -2;
            } else {
                context.victual.fullwarn = TRUE;
                if (context.victual.canchoke && context.victual.reqtime > 1) {
                    /* a one-gulp food will not survive a stop */
                    /*KR if (!paranoid_query(ParanoidEating, "Continue eating?")) { */
                    if (!paranoid_query(ParanoidEating, "계속 먹겠습니까?")) {
                        reset_eat();
                        nomovemsg = (char *) 0;
                    }
                }
            }
        }
    }
    newuhs(FALSE);
}

STATIC_PTR
int
unfaint(VOID_ARGS)
{
    (void) Hear_again();
    if (u.uhs > FAINTING)
        u.uhs = FAINTING;
    stop_occupation();
    context.botl = 1;
    return 0;
}

boolean
is_fainted()
{
    return (boolean) (u.uhs == FAINTED);
}

/* call when a faint must be prematurely terminated */
void
reset_faint()
{
    if (afternmv == unfaint)
        /*KR unmul("You revive."); */
        unmul("당신은 다시 기운을 차렸다.");
}

/* compute and comment on your (new?) hunger status */
void
newuhs(incr)
boolean incr;
{
    unsigned newhs;
    static unsigned save_hs;
    static boolean saved_hs = FALSE;
    int h = u.uhunger;

    newhs = (h > 1000)
                ? SATIATED
                : (h > 150) ? NOT_HUNGRY
                            : (h > 50) ? HUNGRY : (h > 0) ? WEAK : FAINTING;

    /* While you're eating, you may pass from WEAK to HUNGRY to NOT_HUNGRY.
     * This should not produce the message "you only feel hungry now";
     * that message should only appear if HUNGRY is an endpoint.  Therefore
     * we check to see if we're in the middle of eating.  If so, we save
     * the first hunger status, and at the end of eating we decide what
     * message to print based on the _entire_ meal, not on each little bit.
     */
    /* It is normally possible to check if you are in the middle of a meal
     * by checking occupation == eatfood, but there is one special case:
     * start_eating() can call bite() for your first bite before it
     * sets the occupation.
     * Anyone who wants to get that case to work _without_ an ugly static
     * force_save_hs variable, feel free.
     */
    /* Note: If you become a certain hunger status in the middle of the
     * meal, and still have that same status at the end of the meal,
     * this will incorrectly print the associated message at the end of
     * the meal instead of the middle.  Such a case is currently
     * impossible, but could become possible if a message for SATIATED
     * were added or if HUNGRY and WEAK were separated by a big enough
     * gap to fit two bites.
     */
    if (occupation == eatfood || force_save_hs) {
        if (!saved_hs) {
            save_hs = u.uhs;
            saved_hs = TRUE;
        }
        u.uhs = newhs;
        return;
    } else {
        if (saved_hs) {
            u.uhs = save_hs;
            saved_hs = FALSE;
        }
    }

    if (newhs == FAINTING) {
        /* u,uhunger is likely to be negative at this point */
        int uhunger_div_by_10 = sgn(u.uhunger) * ((abs(u.uhunger) + 5) / 10);

        if (is_fainted())
            newhs = FAINTED;
        if (u.uhs <= WEAK || rn2(20 - uhunger_div_by_10) >= 19) {
            if (!is_fainted() && multi >= 0 /* %% */) {
                int duration = 10 - uhunger_div_by_10;

                /* stop what you're doing, then faint */
                stop_occupation();
                /*KR You("faint from lack of food."); */
                You("배고파서 실신했다.");
                incr_itimeout(&HDeaf, duration);
                context.botl = TRUE;
                nomul(-duration);
                /*KR multi_reason = "fainted from lack of food";
                nomovemsg = "You regain consciousness."; */
                multi_reason = "배고파서 실신해있던 동안";
                nomovemsg = "당신은 의식을 되찾았다.";
                afternmv = unfaint;
                newhs = FAINTED;
                if (!Levitation)
                    /*KR selftouch("Falling, you"); */
                    selftouch("떨어지면서, 당신은");
            }

        /* this used to be -(200 + 20 * Con) but that was when being asleep
           suppressed per-turn uhunger decrement but being fainted didn't;
           now uhunger becomes more negative at a slower rate */
        } else if (u.uhunger < -(100 + 10 * (int) ACURR(A_CON))) {
            u.uhs = STARVED;
            context.botl = 1;
            bot();
            /*KR You("die from starvation."); */
            You("아사했다.");
            killer.format = KILLED_BY;
            /*KR Strcpy(killer.name, "starvation"); */
            Strcpy(killer.name, "공복으로 인해 아사했다");
            done(STARVING);
            /* if we return, we lifesaved, and that calls newuhs */
            return;
        }
    }

    if (newhs != u.uhs) {
        if (newhs >= WEAK && u.uhs < WEAK) {
            /* this used to be losestr(1) which had the potential to
               be fatal (still handled below) by reducing HP if it
               tried to take base strength below minimum of 3 */
            ATEMP(A_STR) = -1; /* temporary loss overrides Fixed_abil */
            /* defer context.botl status update until after hunger message */
        } else if (newhs < WEAK && u.uhs >= WEAK) {
            /* this used to be losestr(-1) which could be abused by
               becoming weak while wearing ring of sustain ability,
               removing ring, eating to 'restore' strength which boosted
               strength by a point each time the cycle was performed;
               substituting "while polymorphed" for sustain ability and
               "rehumanize" for ring removal might have done that too */
            ATEMP(A_STR) = 0; /* repair of loss also overrides Fixed_abil */
            /* defer context.botl status update until after hunger message */
        }

        switch (newhs) {
        case HUNGRY:
            if (Hallucination) {
#if 0 /*KR:T*/
                You((!incr) ? "now have a lesser case of the munchies."
                    : "are getting the munchies.");
#else
                if (!incr) {
                    You("꼬르륵 소리가 줄어들었다.");
                }
                else {
                    pline("꼬르륵 꼬르륵.");
                }
#endif
            } else
                /*KR You((!incr) ? "only feel hungry now." */
                You((!incr) ? "이제 약간 출출한 정도가 되었다."
                            : (u.uhunger < 145)
                             /*KR ? "feel hungry."
                                  : "are beginning to feel hungry."); */
                                  ? "배가 고프다."
                                  : "배가 고프기 시작했다.");
            if (incr && occupation
                && (occupation != eatfood && occupation != opentin))
                stop_occupation();
            context.travel = context.travel1 = context.mv = context.run = 0;
            break;
        case WEAK:
            if (Hallucination)
                /*KR pline((!incr) ? "You still have the munchies." 
                : "The munchies are interfering with your motor capabilities."); */
                pline((!incr) ? "배가 아직도 꼬르륵 거려."
                : "꼬르륵 소리가 운동신경을 방해하고 있다.");
            else if (incr && (Role_if(PM_WIZARD) || Race_if(PM_ELF)
                              || Role_if(PM_VALKYRIE)))
                /*KR pline("%s needs food, badly!", */
                pline("%s는 음식이 필요하다, 지금 당장!",
                      (Role_if(PM_WIZARD) || Role_if(PM_VALKYRIE))
                          ? urole.name.m
                     /*KR : "Elf"); */
                          : "엘프");
            else
                You((!incr)
                   /*KR ? "feel weak now."
                        : (u.uhunger < 45) ? "feel weak."
                                           : "are beginning to feel weak."); */
                        ? "이제 쇠약하다고 느껴진다."
                        : (u.uhunger < 45) ? "쇠약해졌다."
                                           : "쇠약하다고 느끼기 시작했다.");
            if (incr && occupation
                && (occupation != eatfood && occupation != opentin))
                stop_occupation();
            context.travel = context.travel1 = context.mv = context.run = 0;
            break;
        }
        u.uhs = newhs;
        context.botl = 1;
        bot();
        if ((Upolyd ? u.mh : u.uhp) < 1) {
            /*KR You("die from hunger and exhaustion."); */
            You("지치고 배고파서 죽었다.");
            killer.format = KILLED_BY;
            /*KR Strcpy(killer.name, "exhaustion"); */
            Strcpy(killer.name, "지치고 배고파서 죽었다");
            done(STARVING);
            return;
        }
    }
}

/* Returns an object representing food.
 * Object may be either on floor or in inventory.
 */
struct obj *
floorfood(verb, corpsecheck)
const char *verb;
int corpsecheck; /* 0, no check, 1, corpses, 2, tinnable corpses */
{
    register struct obj *otmp;
    char qbuf[QBUFSZ];
    char c;
    boolean feeding = !strcmp(verb, "eat"),    /* corpsecheck==0 */
        offering = !strcmp(verb, "sacrifice"); /* corpsecheck==1 */

    /* JNethack에서는 #if 1 const char* jverb = trans_verb(verb)->jp; #endif 가 삽입됨.
    jlib.c (japanese)와 decl.h(include) 파일에 적혀있음. 수정필요 */

    /* if we can't touch floor objects then use invent food only */
    if (iflags.menu_requested /* command was preceded by 'm' prefix */
        || !can_reach_floor(TRUE) || (feeding && u.usteed)
        || (is_pool_or_lava(u.ux, u.uy)
            && (Wwalking || is_clinger(youmonst.data)
                || (Flying && !Breathless))))
        goto skipfloor;

    if (feeding && metallivorous(youmonst.data)) {
        struct obj *gold;
        struct trap *ttmp = t_at(u.ux, u.uy);

        if (ttmp && ttmp->tseen && ttmp->ttyp == BEAR_TRAP) {
            boolean u_in_beartrap = (u.utrap && u.utraptype == TT_BEARTRAP);

            /* If not already stuck in the trap, perhaps there should
               be a chance to becoming trapped?  Probably not, because
               then the trap would just get eaten on the _next_ turn... */
#if 0 /*KR:T*/
            Sprintf(qbuf, "There is a bear trap here (%s); eat it?",
                u_in_beartrap ? "holding you" : "armed");
#else
            Sprintf(qbuf, "여기에 곰덫이 있다 (%s); 먹겠습니까?",
                u_in_beartrap ? "당신을 붙잡고 있음" : "작동 중");
#endif
            if ((c = yn_function(qbuf, ynqchars, 'n')) == 'y') {
                deltrap(ttmp);
                if (u_in_beartrap)
                    reset_utrap(TRUE);
                return mksobj(BEARTRAP, TRUE, FALSE);
            } else if (c == 'q') {
                return (struct obj *) 0;
            }
        }

        if (youmonst.data != &mons[PM_RUST_MONSTER]
            && (gold = g_at(u.ux, u.uy)) != 0) {
#if 0 /*KR:T*/
            if (gold->quan == 1L)
                Sprintf(qbuf, "There is 1 gold piece here; eat it?");
            else
                Sprintf(qbuf, "There are %ld gold pieces here; eat them?",
                    gold->quan);
#else
            Sprintf(qbuf, "여기에 금화 %ld개가 있다. 먹겠습니까?", gold->quan);
#endif
            if ((c = yn_function(qbuf, ynqchars, 'n')) == 'y') {
                return gold;
            } else if (c == 'q') {
                return (struct obj *) 0;
            }
        }
    }

    /* Is there some food (probably a heavy corpse) here on the ground? */
    for (otmp = level.objects[u.ux][u.uy]; otmp; otmp = otmp->nexthere) {
        if (corpsecheck
                ? (otmp->otyp == CORPSE
                   && (corpsecheck == 1 || tinnable(otmp)))
                : feeding ? (otmp->oclass != COIN_CLASS && is_edible(otmp))
                          : otmp->oclass == FOOD_CLASS) {
            char qsfx[QBUFSZ];
            boolean one = (otmp->quan == 1L);

            /* if blind and without gloves, attempting to eat (or tin or
               offer) a cockatrice corpse is fatal before asking whether
               or not to use it; otherwise, 'm<dir>' followed by 'e' could
               be used to locate cockatrice corpses without touching them */
            if (otmp->otyp == CORPSE && will_feel_cockatrice(otmp, FALSE)) {
                feel_cockatrice(otmp, FALSE);
                /* if life-saved (or poly'd into stone golem), terminate
                   attempt to eat off floor */
                return (struct obj *) 0;
            }
            /* "There is <an object> here; <verb> it?" or
               "There are <N objects> here; <verb> one?" */
            Sprintf(qbuf, "There %s ", otense(otmp, "are"));
            Sprintf(qsfx, " here; %s %s?", verb, one ? "it" : "one"); 
                  /* jverb 함수 사용. 수정필요 - one ? "" : "하나", jverb); */
            (void) safe_qbuf(qbuf, qbuf, qsfx, otmp, doname, ansimpleoname,
                             one ? something : (const char *) "things");
            if ((c = yn_function(qbuf, ynqchars, 'n')) == 'y')
                return  otmp;
            else if (c == 'q')
                return (struct obj *) 0;
        }
    }

 skipfloor:
    /* We cannot use ALL_CLASSES since that causes getobj() to skip its
     * "ugly checks" and we need to check for inedible items.
     */
    otmp = getobj(feeding ? allobj : offering ? offerfodder : comestibles,
                  verb);
    if (corpsecheck && otmp && !(offering && otmp->oclass == AMULET_CLASS))
        if (otmp->otyp != CORPSE || (corpsecheck == 2 && !tinnable(otmp))) {
            /*KR You_cant("%s that!", verb); -> jverb 함수 사용. 수정필요 */
            You_cant("%s that!", verb);
            return (struct obj *) 0;
        }
    return otmp;
}

/* Side effects of vomiting */
/* added nomul (MRS) - it makes sense, you're too busy being sick! */
void
vomit() /* A good idea from David Neves */
{
    if (cantvomit(youmonst.data)) {
        /* doesn't cure food poisoning; message assumes that we aren't
           dealing with some esoteric body_part() */
        /*KR Your("jaw gapes convulsively."); */
        Your("턱이 경련적으로 벌어진다.");
    } else {
        if (Sick && (u.usick_type & SICK_VOMITABLE) != 0)
            make_sick(0L, (char *) 0, TRUE, SICK_VOMITABLE);
        /* if not enough in stomach to actually vomit then dry heave;
           vomiting_dialog() gives a vomit message when its countdown
           reaches 0, but only if u.uhs < FAINTING (and !cantvomit()) */
        if (u.uhs >= FAINTING)
            /*KR Your("%s heaves convulsively!", body_part(STOMACH)); */
            Your("%s이 경련적으로 들썩거린다!", body_part(STOMACH));
    }

    /* nomul()/You_can_move_again used to be unconditional, which was
       viable while eating but not for Vomiting countdown where hero might
       be immobilized for some other reason at the time vomit() is called */
    if (multi >= -2) {
        nomul(-2);
        /*KR multi_reason = "vomiting"; */
        multi_reason = "토하고 있던 중에";
        nomovemsg = You_can_move_again;
    }
}

int
eaten_stat(base, obj)
int base;
struct obj *obj;
{
    long uneaten_amt, full_amount;

    /* get full_amount first; obj_nutrition() might modify obj->oeaten */
    full_amount = (long) obj_nutrition(obj);
    uneaten_amt = (long) obj->oeaten;
    if (uneaten_amt > full_amount) {
        impossible(
          "partly eaten food (%ld) more nutritious than untouched food (%ld)",
                   uneaten_amt, full_amount);
        uneaten_amt = full_amount;
    }

    base = (int) (full_amount ? (long) base * uneaten_amt / full_amount : 0L);
    return (base < 1) ? 1 : base;
}

/* reduce obj's oeaten field, making sure it never hits or passes 0 */
void
consume_oeaten(obj, amt)
struct obj *obj;
int amt;
{
    /*
     * This is a hack to try to squelch several long standing mystery
     * food bugs.  A better solution would be to rewrite the entire
     * victual handling mechanism from scratch using a less complex
     * model.  Alternatively, this routine could call done_eating()
     * or food_disappears() but its callers would need revisions to
     * cope with context.victual.piece unexpectedly going away.
     *
     * Multi-turn eating operates by setting the food's oeaten field
     * to its full nutritional value and then running a counter which
     * independently keeps track of whether there is any food left.
     * The oeaten field can reach exactly zero on the last turn, and
     * the object isn't removed from inventory until the next turn
     * when the "you finish eating" message gets delivered, so the
     * food would be restored to the status of untouched during that
     * interval.  This resulted in unexpected encumbrance messages
     * at the end of a meal (if near enough to a threshold) and would
     * yield full food if there was an interruption on the critical
     * turn.  Also, there have been reports over the years of food
     * becoming massively heavy or producing unlimited satiation;
     * this would occur if reducing oeaten via subtraction attempted
     * to drop it below 0 since its unsigned type would produce a
     * huge positive value instead.  So far, no one has figured out
     * _why_ that inappropriate subtraction might sometimes happen.
     */

    if (amt > 0) {
        /* bit shift to divide the remaining amount of food */
        obj->oeaten >>= amt;
    } else {
        /* simple decrement; value is negative so we actually add it */
        if ((int) obj->oeaten > -amt)
            obj->oeaten += amt;
        else
            obj->oeaten = 0;
    }

    if (obj->oeaten == 0) {
        if (obj == context.victual.piece) /* always true unless wishing... */
            context.victual.reqtime =
                context.victual.usedtime; /* no bites left */
        obj->oeaten = 1; /* smallest possible positive value */
    }
}

/* called when eatfood occupation has been interrupted,
   or in the case of theft, is about to be interrupted */
boolean
maybe_finished_meal(stopping)
boolean stopping;
{
    /* in case consume_oeaten() has decided that the food is all gone */
    if (occupation == eatfood
        && context.victual.usedtime >= context.victual.reqtime) {
        if (stopping)
            occupation = 0; /* for do_reset_eat */
        (void) eatfood();   /* calls done_eating() to use up
                               context.victual.piece */
        return TRUE;
    }
    return FALSE;
}

/* Tin of <something> to the rescue?  Decide whether current occupation
   is an attempt to eat a tin of something capable of saving hero's life.
   We don't care about consumption of non-tinned food here because special
   effects there take place on first bite rather than at end of occupation.
   [Popeye the Sailor gets out of trouble by eating tins of spinach. :-] */
boolean
Popeye(threat)
int threat;
{
    struct obj *otin;
    int mndx;

    if (occupation != opentin)
        return FALSE;
    otin = context.tin.tin;
    /* make sure hero still has access to tin */
    if (!carried(otin)
        && (!obj_here(otin, u.ux, u.uy) || !can_reach_floor(TRUE)))
        return FALSE;
    /* unknown tin is assumed to be helpful */
    if (!otin->known)
        return TRUE;
    /* known tin is helpful if it will stop life-threatening problem */
    mndx = otin->corpsenm;
    switch (threat) {
    /* note: not used; hunger code bypasses stop_occupation() when eating */
    case HUNGER:
        return (boolean) (mndx != NON_PM || otin->spe == 1);
    /* flesh from lizards and acidic critters stops petrification */
    case STONED:
        return (boolean) (mndx >= LOW_PM
                          && (mndx == PM_LIZARD || acidic(&mons[mndx])));
    /* no tins can cure these (yet?) */
    case SLIMED:
    case SICK:
    case VOMITING:
        break;
    default:
        break;
    }
    return FALSE;
}

/*eat.c*/
