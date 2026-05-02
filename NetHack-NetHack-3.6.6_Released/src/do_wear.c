/* NetHack 3.6	do_wear.c	$NHDT-Date: 1575214670 2019/12/01 15:37:50 $  $NHDT-Branch: NetHack-3.6 $:$NHDT-Revision: 1.116 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2012. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

#if 0 /*KR 미사용 */
static NEARDATA const char see_yourself[] = "see yourself";
#endif
static NEARDATA const char unknown_type[] = "Unknown type of %s (%d)";
#if 0 /*KR:T*/
static NEARDATA const char c_armor[] = "armor", c_suit[] = "suit",
                           c_shirt[] = "shirt", c_cloak[] = "cloak",
                           c_gloves[] = "gloves", c_boots[] = "boots",
                           c_helmet[] = "helmet", c_shield[] = "shield",
                           c_weapon[] = "weapon", c_sword[] = "sword",
                           c_axe[] = "axe", c_that_[] = "that";
#else
static NEARDATA const char c_armor[] = "갑옷", c_suit[] = "옷",
                           c_shirt[] = "셔츠", c_cloak[] = "망토",
                           c_gloves[] = "장갑", c_boots[] = "신발",
                           c_helmet[] = "투구", c_shield[] = "방패",
                           c_weapon[] = "무기", c_sword[] = "검",
                           c_axe[] = "도끼", c_that_[] = "그것";
#endif

static NEARDATA const long takeoff_order[] = {
    WORN_BLINDF, W_WEP,      WORN_SHIELD, WORN_GLOVES, LEFT_RING,
    RIGHT_RING,  WORN_CLOAK, WORN_HELMET, WORN_AMUL,   WORN_ARMOR,
    WORN_SHIRT,  WORN_BOOTS, W_SWAPWEP,   W_QUIVER,    0L
};

STATIC_DCL void FDECL(on_msg, (struct obj *));
STATIC_DCL void FDECL(toggle_stealth, (struct obj *, long, BOOLEAN_P));
STATIC_DCL void FDECL(toggle_displacement, (struct obj *, long, BOOLEAN_P));
STATIC_PTR int NDECL(Armor_on);
/* int NDECL(Boots_on); -- moved to extern.h */
STATIC_PTR int NDECL(Cloak_on);
STATIC_PTR int NDECL(Helmet_on);
STATIC_PTR int NDECL(Gloves_on);
STATIC_DCL void FDECL(wielding_corpse, (struct obj *, BOOLEAN_P));
STATIC_PTR int NDECL(Shield_on);
STATIC_PTR int NDECL(Shirt_on);
STATIC_DCL void NDECL(Amulet_on);
STATIC_DCL void FDECL(learnring, (struct obj *, BOOLEAN_P));
STATIC_DCL void FDECL(Ring_off_or_gone, (struct obj *, BOOLEAN_P));
STATIC_PTR int FDECL(select_off, (struct obj *));
STATIC_DCL struct obj *NDECL(do_takeoff);
STATIC_PTR int NDECL(take_off);
STATIC_DCL int FDECL(menu_remarm, (int));
STATIC_DCL void FDECL(count_worn_stuff, (struct obj **, BOOLEAN_P));
STATIC_PTR int FDECL(armor_or_accessory_off, (struct obj *));
STATIC_PTR int FDECL(accessory_or_armor_on, (struct obj *));
#if 0 /*KR*/
STATIC_DCL void FDECL(already_wearing, (const char *));
#else
STATIC_DCL void FDECL(already_wearing, (const char *, struct obj *));
#endif
STATIC_DCL void FDECL(already_wearing2, (const char *, const char *));

/* plural "fingers" or optionally "gloves" */
const char *
fingers_or_gloves(check_gloves)
boolean check_gloves;
{
    return ((check_gloves && uarmg)
            ? gloves_simple_name(uarmg) /* "gloves" or "gauntlets" */
            : makeplural(body_part(FINGER))); /* "fingers" */
}

void
off_msg(otmp)
struct obj *otmp;
{
    if (flags.verbose)
        /*KR You("were wearing %s.", doname(otmp)); */
        You("%s 장비하고 있었다.", append_josa(doname(otmp), "을"));
}

/* for items that involve no delay */
STATIC_OVL void
on_msg(otmp)
struct obj *otmp;
{
    if (flags.verbose) {
        char how[BUFSZ];
        /* call xname() before obj_is_pname(); formatting obj's name
           might set obj->dknown and that affects the pname test */
        const char *otmp_name = xname(otmp);

        how[0] = '\0';
        if (otmp->otyp == TOWEL)
            /*KR Sprintf(how, " around your %s", body_part(HEAD)); */
            Sprintf(how, " 당신의 %s에", body_part(HEAD));
#if 0 /*KR: 원본*/
        You("are now wearing %s%s.",
            obj_is_pname(otmp) ? the(otmp_name) : an(otmp_name), how);
#else /*KR: KRNethack 맞춤 번역 */
        You("이제 %s%s 장비했다.",
            append_josa(obj_is_pname(otmp) ? the(otmp_name) : an(otmp_name),
                        "을"),
            how);
#endif
    }
}

/* starting equipment gets auto-worn at beginning of new game,
   and we don't want stealth or displacement feedback then */
static boolean initial_don = FALSE; /* manipulated in set_wear() */

/* putting on or taking off an item which confers stealth;
   give feedback and discover it iff stealth state is changing */
STATIC_OVL
void
toggle_stealth(obj, oldprop, on)
struct obj *obj;
long oldprop; /* prop[].extrinsic, with obj->owornmask stripped by caller */
boolean on;
{
    if (on ? initial_don : context.takeoff.cancelled_don)
        return;

    if (!oldprop /* extrinsic stealth from something else */
        && !HStealth /* intrinsic stealth */
        && !BStealth) { /* stealth blocked by something */
        if (obj->otyp == RIN_STEALTH)
            learnring(obj, TRUE);
        else
            makeknown(obj->otyp);

        if (on) {
            if (!is_boots(obj))
                /*KR You("move very quietly."); */
                Your("조용히 움직이게 되었다.");
            else if (Levitation || Flying)
                /*KR You("float imperceptibly."); */
                You("어느샌가 공중에 떠 있었다.");
            else
                /*KR You("walk very quietly."); */
                Your("발소리가 작아졌다.");
        } else {
            /*KR You("sure are noisy."); */
            Your("발소리가 커졌다.");
        }
    }
}

/* putting on or taking off an item which confers displacement;
   give feedback and discover it iff displacement state is changing *and*
   hero is able to see self (or sense monsters) */
STATIC_OVL
void
toggle_displacement(obj, oldprop, on)
struct obj *obj;
long oldprop; /* prop[].extrinsic, with obj->owornmask stripped by caller */
boolean on;
{
    if (on ? initial_don : context.takeoff.cancelled_don)
        return;

    if (!oldprop /* extrinsic displacement from something else */
        && !(u.uprops[DISPLACED].intrinsic) /* (theoretical) */
        && !(u.uprops[DISPLACED].blocked) /* (also theoretical) */
        /* we don't use canseeself() here because it augments vision
           with touch, which isn't appropriate for deciding whether
           we'll notice that monsters have trouble spotting the hero */
        && ((!Blind         /* see anything */
             && !u.uswallow /* see surroundings */
             && !Invisible) /* see self */
            /* actively sensing nearby monsters via telepathy or extended
               monster detection overrides vision considerations because
               hero also senses self in this situation */
            || (Unblind_telepat
                || (Blind_telepat && Blind)
                || Detect_monsters))) {
        makeknown(obj->otyp);

#if 0 /*KR: 원본*/
        You_feel("that monsters%s have difficulty pinpointing your location.",
                 on ? "" : " no longer");
#else /*KR: KRNethack 맞춤 번역 */
        pline("몬스터들이 당신의 위치를 정확히 파악하기가 %s 것 같다.",
              on ? "어려워진" : "더 이상 어렵지 않은");
#endif
    }
}

/*
 * The Type_on() functions should be called *after* setworn().
 * The Type_off() functions call setworn() themselves.
 * [Blindf_on() is an exception and calls setworn() itself.]
 */

int
Boots_on(VOID_ARGS)
{
    long oldprop =
        u.uprops[objects[uarmf->otyp].oc_oprop].extrinsic & ~WORN_BOOTS;

    switch (uarmf->otyp) {
    case LOW_BOOTS:
    case IRON_SHOES:
    case HIGH_BOOTS:
    case JUMPING_BOOTS:
    case KICKING_BOOTS:
        break;
    case WATER_WALKING_BOOTS:
        if (u.uinwater)
            spoteffects(TRUE);
        /* (we don't need a lava check here since boots can't be
           put on while feet are stuck) */
        break;
    case SPEED_BOOTS:
        /* Speed boots are still better than intrinsic speed, */
        /* though not better than potion speed */
        if (!oldprop && !(HFast & TIMEOUT)) {
            makeknown(uarmf->otyp);
#if 0 /*KR:T*/
            You_feel("yourself speed up%s.",
                     (oldprop || HFast) ? " a bit more" : "");
#else
            You("%s빨라진 기분이 든다.",
                     (oldprop || HFast) ? "조금 더 " : "");
#endif
        }
        break;
    case ELVEN_BOOTS:
        toggle_stealth(uarmf, oldprop, TRUE);
        break;
    case FUMBLE_BOOTS:
        if (!oldprop && !(HFumbling & ~TIMEOUT))
            incr_itimeout(&HFumbling, rnd(20));
        break;
    case LEVITATION_BOOTS:
        if (!oldprop && !HLevitation && !(BLevitation & FROMOUTSIDE)) {
            uarmf->known = 1; /* might come off if putting on over a sink,
                               * so uarmf could be Null below; status line
                               * gets updated during brief interval they're
                               * worn so hero and player learn enchantment */
            context.botl = 1; /* status hilites might mark AC changed */
            makeknown(uarmf->otyp);
            float_up();
            if (Levitation)
                spoteffects(FALSE); /* for sink effect */
        } else {
            float_vs_flight(); /* maybe toggle BFlying's I_SPECIAL */
        }
        break;
    default:
        impossible(unknown_type, c_boots, uarmf->otyp);
    }
    if (uarmf) /* could be Null here (levitation boots put on over a sink) */
        uarmf->known = 1; /* boots' +/- evident because of status line AC */
    return 0;
}

int
Boots_off(VOID_ARGS)
{
    struct obj *otmp = uarmf;
    int otyp = otmp->otyp;
    long oldprop = u.uprops[objects[otyp].oc_oprop].extrinsic & ~WORN_BOOTS;

    context.takeoff.mask &= ~W_ARMF;
    /* For levitation, float_down() returns if Levitation, so we
     * must do a setworn() _before_ the levitation case.
     */
    setworn((struct obj *) 0, W_ARMF);
    switch (otyp) {
    case SPEED_BOOTS:
        if (!Very_fast && !context.takeoff.cancelled_don) {
            makeknown(otyp);
            /*KR You_feel("yourself slow down%s.", Fast ? " a bit" : ""); */
            You("%s느려진 기분이 든다.", Fast ? "조금 " : "");
        }
        break;
    case WATER_WALKING_BOOTS:
        /* check for lava since fireproofed boots make it viable */
        if ((is_pool(u.ux, u.uy) || is_lava(u.ux, u.uy))
            && !Levitation && !Flying && !is_clinger(youmonst.data)
            && !context.takeoff.cancelled_don
            /* avoid recursive call to lava_effects() */
            && !iflags.in_lava_effects) {
            /* make boots known in case you survive the drowning */
            makeknown(otyp);
            spoteffects(TRUE);
        }
        break;
    case ELVEN_BOOTS:
        toggle_stealth(otmp, oldprop, FALSE);
        break;
    case FUMBLE_BOOTS:
        if (!oldprop && !(HFumbling & ~TIMEOUT))
            HFumbling = EFumbling = 0;
        break;
    case LEVITATION_BOOTS:
        if (!oldprop && !HLevitation && !(BLevitation & FROMOUTSIDE)
            && !context.takeoff.cancelled_don) {
            (void) float_down(0L, 0L);
            makeknown(otyp);
        } else {
            float_vs_flight(); /* maybe toggle (BFlying & I_SPECIAL) */
        }
        break;
    case LOW_BOOTS:
    case IRON_SHOES:
    case HIGH_BOOTS:
    case JUMPING_BOOTS:
    case KICKING_BOOTS:
        break;
    default:
        impossible(unknown_type, c_boots, otyp);
    }
    context.takeoff.cancelled_don = FALSE;
    return 0;
}

STATIC_PTR int
Cloak_on(VOID_ARGS)
{
    long oldprop =
        u.uprops[objects[uarmc->otyp].oc_oprop].extrinsic & ~WORN_CLOAK;

    switch (uarmc->otyp) {
    case ORCISH_CLOAK:
    case DWARVISH_CLOAK:
    case CLOAK_OF_MAGIC_RESISTANCE:
    case ROBE:
    case LEATHER_CLOAK:
        break;
    case CLOAK_OF_PROTECTION:
        makeknown(uarmc->otyp);
        break;
    case ELVEN_CLOAK:
        toggle_stealth(uarmc, oldprop, TRUE);
        break;
    case CLOAK_OF_DISPLACEMENT:
        toggle_displacement(uarmc, oldprop, TRUE);
        break;
    case MUMMY_WRAPPING:
        /* Note: it's already being worn, so we have to cheat here. */
        if ((HInvis || EInvis) && !Blind) {
            newsym(u.ux, u.uy);
#if 0 /*KR:T*/
            You("can %s!", See_invisible ? "no longer see through yourself"
                                         : see_yourself);
#else
            You("자기 자신이%s!", See_invisible ? " 보이지 않게 되었다"
                                           : " 보이게 되었다");
#endif
        }
        break;
    case CLOAK_OF_INVISIBILITY:
        /* since cloak of invisibility was worn, we know mummy wrapping
           wasn't, so no need to check `oldprop' against blocked */
        if (!oldprop && !HInvis && !Blind) {
            makeknown(uarmc->otyp);
            newsym(u.ux, u.uy);
#if 0 /*KR:T*/
            pline("Suddenly you can%s yourself.",
                  See_invisible ? " see through" : "not see");
#else
            pline("갑자기 당신은 자기 자신이 %s.",
                  See_invisible ? "보이게 되었다" : "보이지 않게 되었다");
#endif
        }
        break;
    case OILSKIN_CLOAK:
        /*KR pline("%s very tightly.", Tobjnam(uarmc, "fit")); */
        pline("%s 매우 꽉 낀다.", append_josa(xname(uarmc), "이"));
        break;
    /* Alchemy smock gives poison _and_ acid resistance */
    case ALCHEMY_SMOCK:
        EAcid_resistance |= WORN_CLOAK;
        break;
    default:
        impossible(unknown_type, c_cloak, uarmc->otyp);
    }
    if (uarmc) /* no known instance of !uarmc here but play it safe */
        uarmc->known = 1; /* cloak's +/- evident because of status line AC */
    return 0;
}

int
Cloak_off(VOID_ARGS)
{
    struct obj *otmp = uarmc;
    int otyp = otmp->otyp;
    long oldprop = u.uprops[objects[otyp].oc_oprop].extrinsic & ~WORN_CLOAK;

    context.takeoff.mask &= ~W_ARMC;
    /* For mummy wrapping, taking it off first resets `Invisible'. */
    setworn((struct obj *) 0, W_ARMC);
    switch (otyp) {
    case ORCISH_CLOAK:
    case DWARVISH_CLOAK:
    case CLOAK_OF_PROTECTION:
    case CLOAK_OF_MAGIC_RESISTANCE:
    case OILSKIN_CLOAK:
    case ROBE:
    case LEATHER_CLOAK:
        break;
    case ELVEN_CLOAK:
        toggle_stealth(otmp, oldprop, FALSE);
        break;
    case CLOAK_OF_DISPLACEMENT:
        toggle_displacement(otmp, oldprop, FALSE);
        break;
    case MUMMY_WRAPPING:
        if (Invis && !Blind) {
            newsym(u.ux, u.uy);
#if 0 /*KR: 원본*/
            You("can %s.", See_invisible ? "see through yourself"
                                         : "no longer see yourself");
#else /*KR: KRNethack 맞춤 번역 */
            You("자기 자신이 %s.",
                See_invisible ? "투명하게 보인다" : "더 이상 보이지 않는다");
#endif
        }
        break;
    case CLOAK_OF_INVISIBILITY:
        if (!oldprop && !HInvis && !Blind) {
            makeknown(CLOAK_OF_INVISIBILITY);
            newsym(u.ux, u.uy);
#if 0 /*KR: 원본*/
            pline("Suddenly you can %s.",
                  See_invisible ? "no longer see through yourself"
                                : see_yourself);
#else /*KR: KRNethack 맞춤 번역 */
            pline("갑자기 당신은 자기 자신이 %s.",
                  See_invisible ? "더 이상 투명하게 보이지 않는다"
                                : "보이게 되었다");
#endif    
        }
        break;
    /* Alchemy smock gives poison _and_ acid resistance */
    case ALCHEMY_SMOCK:
        EAcid_resistance &= ~WORN_CLOAK;
        break;
    default:
        impossible(unknown_type, c_cloak, otyp);
    }
    return 0;
}

STATIC_PTR
int
Helmet_on(VOID_ARGS)
{
    switch (uarmh->otyp) {
    case FEDORA:
    case HELMET:
    case DENTED_POT:
    case ELVEN_LEATHER_HELM:
    case DWARVISH_IRON_HELM:
    case ORCISH_HELM:
    case HELM_OF_TELEPATHY:
        break;
    case HELM_OF_BRILLIANCE:
        adj_abon(uarmh, uarmh->spe);
        break;
    case CORNUTHAUM:
        /* people think marked wizards know what they're talking about,
           but it takes trained arrogance to pull it off, and the actual
           enchantment of the hat is irrelevant */
        ABON(A_CHA) += (Role_if(PM_WIZARD) ? 1 : -1);
        context.botl = 1;
        makeknown(uarmh->otyp);
        break;
    case HELM_OF_OPPOSITE_ALIGNMENT:
        uarmh->known = 1; /* do this here because uarmh could get cleared */
        /* changing alignment can toggle off active artifact properties,
           including levitation; uarmh could get dropped or destroyed here
           by hero falling onto a polymorph trap or into water (emergency
           disrobe) or maybe lava (probably not, helm isn't 'organic') */
        uchangealign((u.ualign.type != A_NEUTRAL)
                         ? -u.ualign.type
                         : (uarmh->o_id % 2) ? A_CHAOTIC : A_LAWFUL,
                     1);
        /* makeknown(HELM_OF_OPPOSITE_ALIGNMENT); -- below, after Tobjnam() */
    /*FALLTHRU*/
    case DUNCE_CAP:
        if (uarmh && !uarmh->cursed) {
            if (Blind)
                /*KR pline("%s for a moment.", Tobjnam(uarmh, "vibrate")); */
                pline("%s 잠시 진동했다.", append_josa(xname(uarmh), "이"));
            else
#if 0 /*KR: 원본*/
                pline("%s %s for a moment.", Tobjnam(uarmh, "glow"),
                      hcolor(NH_BLACK));
#else /*KR: KRNethack 맞춤 번역 */
                pline("%s 잠시 %s 빛났다.", append_josa(xname(uarmh), "이"),
                      hcolor(NH_BLACK));
#endif
            curse(uarmh);
        }
        context.botl = 1; /* reveal new alignment or INT & WIS */
        if (Hallucination) {
            /*KR pline("My brain hurts!"); */ /* Monty Python's Flying Circus
                                               */
            pline("뇌가 아파!");
        } else if (uarmh && uarmh->otyp == DUNCE_CAP) {
#if 0                       /*KR: 원본*/
            You_feel("%s.", /* track INT change; ignore WIS */
                     ACURR(A_INT)
                             <= (ABASE(A_INT) + ABON(A_INT) + ATEMP(A_INT))
                         ? "like sitting in a corner"
                         : "giddy");
#else                       /*KR: KRNethack 맞춤 번역 */
            You("%s.", /* track INT change; ignore WIS */
                     ACURR(A_INT)
                             <= (ABASE(A_INT) + ABON(A_INT) + ATEMP(A_INT))
                         ? "구석에 앉아 있고 싶어졌다"
                         : "어지럽다");
#endif
        } else {
            /* [message formerly given here moved to uchangealign()] */
            makeknown(HELM_OF_OPPOSITE_ALIGNMENT);
        }
        break;
    default:
        impossible(unknown_type, c_helmet, uarmh->otyp);
    }
    /* uarmh could be Null due to uchangealign() */
    if (uarmh)
        uarmh->known = 1; /* helmet's +/- evident because of status line AC */
    return 0;
}

int
Helmet_off(VOID_ARGS)
{
    context.takeoff.mask &= ~W_ARMH;

    switch (uarmh->otyp) {
    case FEDORA:
    case HELMET:
    case DENTED_POT:
    case ELVEN_LEATHER_HELM:
    case DWARVISH_IRON_HELM:
    case ORCISH_HELM:
        break;
    case DUNCE_CAP:
        context.botl = 1;
        break;
    case CORNUTHAUM:
        if (!context.takeoff.cancelled_don) {
            ABON(A_CHA) += (Role_if(PM_WIZARD) ? -1 : 1);
            context.botl = 1;
        }
        break;
    case HELM_OF_TELEPATHY:
        /* need to update ability before calling see_monsters() */
        setworn((struct obj *) 0, W_ARMH);
        see_monsters();
        return 0;
    case HELM_OF_BRILLIANCE:
        if (!context.takeoff.cancelled_don)
            adj_abon(uarmh, -uarmh->spe);
        break;
    case HELM_OF_OPPOSITE_ALIGNMENT:
        /* changing alignment can toggle off active artifact
           properties, including levitation; uarmh could get
           dropped or destroyed here */
        uchangealign(u.ualignbase[A_CURRENT], 2);
        break;
    default:
        impossible(unknown_type, c_helmet, uarmh->otyp);
    }
    setworn((struct obj *) 0, W_ARMH);
    context.takeoff.cancelled_don = FALSE;
    return 0;
}

STATIC_PTR
int
Gloves_on(VOID_ARGS)
{
    long oldprop =
        u.uprops[objects[uarmg->otyp].oc_oprop].extrinsic & ~WORN_GLOVES;

    switch (uarmg->otyp) {
    case LEATHER_GLOVES:
        break;
    case GAUNTLETS_OF_FUMBLING:
        if (!oldprop && !(HFumbling & ~TIMEOUT))
            incr_itimeout(&HFumbling, rnd(20));
        break;
    case GAUNTLETS_OF_POWER:
        makeknown(uarmg->otyp);
        context.botl = 1; /* taken care of in attrib.c */
        break;
    case GAUNTLETS_OF_DEXTERITY:
        adj_abon(uarmg, uarmg->spe);
        break;
    default:
        impossible(unknown_type, c_gloves, uarmg->otyp);
    }
    if (uarmg) /* no known instance of !uarmg here but play it safe */
        uarmg->known = 1; /* gloves' +/- evident because of status line AC */
    return 0;
}

STATIC_OVL void
wielding_corpse(obj, voluntary)
struct obj *obj;
boolean voluntary; /* taking gloves off on purpose? */
{
    char kbuf[BUFSZ];

    if (!obj || obj->otyp != CORPSE)
        return;
    if (obj != uwep && (obj != uswapwep || !u.twoweap))
        return;

    if (touch_petrifies(&mons[obj->corpsenm]) && !Stone_resistance) {
#if 0 /*KR: 원본*/
        You("now wield %s in your bare %s.",
            corpse_xname(obj, (const char *) 0, CXN_ARTICLE),
            makeplural(body_part(HAND)));
#else /*KR: KRNethack 맞춤 번역 */
        You("맨손으로 %s 쥐었다.",
            append_josa(corpse_xname(obj, (const char *) 0, CXN_ARTICLE),
                        "을"));
#endif
#if 0 /*KR: 원본*/
        Sprintf(kbuf, "%s gloves while wielding %s",
                voluntary ? "removing" : "losing", killer_xname(obj));
#else /*KR: KRNethack 맞춤 번역 */
        Sprintf(kbuf, "%s 쥐고 있는 상태에서 장갑을 %s",
                append_josa(killer_xname(obj), "을"),
                voluntary ? "벗은 것" : "잃은 것");
#endif
        instapetrify(kbuf);
        /* life-saved; can't continue wielding cockatrice corpse though */
        remove_worn_item(obj, FALSE);
    }
}

int
Gloves_off(VOID_ARGS)
{
    long oldprop =
        u.uprops[objects[uarmg->otyp].oc_oprop].extrinsic & ~WORN_GLOVES;
    boolean on_purpose = !context.mon_moving && !uarmg->in_use;

    context.takeoff.mask &= ~W_ARMG;

    switch (uarmg->otyp) {
    case LEATHER_GLOVES:
        break;
    case GAUNTLETS_OF_FUMBLING:
        if (!oldprop && !(HFumbling & ~TIMEOUT))
            HFumbling = EFumbling = 0;
        break;
    case GAUNTLETS_OF_POWER:
        makeknown(uarmg->otyp);
        context.botl = 1; /* taken care of in attrib.c */
        break;
    case GAUNTLETS_OF_DEXTERITY:
        if (!context.takeoff.cancelled_don)
            adj_abon(uarmg, -uarmg->spe);
        break;
    default:
        impossible(unknown_type, c_gloves, uarmg->otyp);
    }
    setworn((struct obj *) 0, W_ARMG);
    context.takeoff.cancelled_don = FALSE;
    (void) encumber_msg(); /* immediate feedback for GoP */

    /* usually can't remove gloves when they're slippery but it can
       be done by having them fall off (polymorph), stolen, or
       destroyed (scroll, overenchantment, monster spell); if that
       happens, 'cure' slippery fingers so that it doesn't transfer
       from gloves to bare hands */
    if (Glib)
        make_glib(0); /* for update_inventory() */

    /* prevent wielding cockatrice when not wearing gloves */
    if (uwep && uwep->otyp == CORPSE)
        wielding_corpse(uwep, on_purpose);

    /* KMH -- ...or your secondary weapon when you're wielding it
       [This case can't actually happen; twoweapon mode won't
       engage if a corpse has been set up as the alternate weapon.] */
    if (u.twoweap && uswapwep && uswapwep->otyp == CORPSE)
        wielding_corpse(uswapwep, on_purpose);

    return 0;
}

STATIC_PTR int
Shield_on(VOID_ARGS)
{
    /* no shield currently requires special handling when put on, but we
       keep this uncommented in case somebody adds a new one which does
       [reflection is handled by setting u.uprops[REFLECTION].extrinsic
       in setworn() called by armor_or_accessory_on() before Shield_on()] */
    switch (uarms->otyp) {
    case SMALL_SHIELD:
    case ELVEN_SHIELD:
    case URUK_HAI_SHIELD:
    case ORCISH_SHIELD:
    case DWARVISH_ROUNDSHIELD:
    case LARGE_SHIELD:
    case SHIELD_OF_REFLECTION:
        break;
    default:
        impossible(unknown_type, c_shield, uarms->otyp);
    }
    if (uarms) /* no known instance of !uarmgs here but play it safe */
        uarms->known = 1; /* shield's +/- evident because of status line AC */
    return 0;
}

int
Shield_off(VOID_ARGS)
{
    context.takeoff.mask &= ~W_ARMS;

    /* no shield currently requires special handling when taken off, but we
       keep this uncommented in case somebody adds a new one which does */
    switch (uarms->otyp) {
    case SMALL_SHIELD:
    case ELVEN_SHIELD:
    case URUK_HAI_SHIELD:
    case ORCISH_SHIELD:
    case DWARVISH_ROUNDSHIELD:
    case LARGE_SHIELD:
    case SHIELD_OF_REFLECTION:
        break;
    default:
        impossible(unknown_type, c_shield, uarms->otyp);
    }

    setworn((struct obj *) 0, W_ARMS);
    return 0;
}

STATIC_PTR int
Shirt_on(VOID_ARGS)
{
    /* no shirt currently requires special handling when put on, but we
       keep this uncommented in case somebody adds a new one which does */
    switch (uarmu->otyp) {
    case HAWAIIAN_SHIRT:
    case T_SHIRT:
        break;
    default:
        impossible(unknown_type, c_shirt, uarmu->otyp);
    }
    if (uarmu) /* no known instances of !uarmu here but play it safe */
        uarmu->known = 1; /* shirt's +/- evident because of status line AC */
    return 0;
}

int
Shirt_off(VOID_ARGS)
{
    context.takeoff.mask &= ~W_ARMU;

    /* no shirt currently requires special handling when taken off, but we
       keep this uncommented in case somebody adds a new one which does */
    switch (uarmu->otyp) {
    case HAWAIIAN_SHIRT:
    case T_SHIRT:
        break;
    default:
        impossible(unknown_type, c_shirt, uarmu->otyp);
    }

    setworn((struct obj *) 0, W_ARMU);
    return 0;
}

STATIC_PTR
int
Armor_on(VOID_ARGS)
{
    /*
     * No suits require special handling.  Special properties conferred by
     * suits are set up as intrinsics (actually 'extrinsics') by setworn()
     * which is called by armor_or_accessory_on() before Armor_on().
     */
    if (uarm) /* no known instances of !uarm here but play it safe */
        uarm->known = 1; /* suit's +/- evident because of status line AC */
    return 0;
}

int
Armor_off(VOID_ARGS)
{
    context.takeoff.mask &= ~W_ARM;
    setworn((struct obj *) 0, W_ARM);
    context.takeoff.cancelled_don = FALSE;
    return 0;
}

/* The gone functions differ from the off functions in that if you die from
 * taking it off and have life saving, you still die.  [Obsolete reference
 * to lack of fire resistance being fatal in hell (nethack 3.0) and life
 * saving putting a removed item back on to prevent that from immediately
 * repeating.]
 */
int
Armor_gone()
{
    context.takeoff.mask &= ~W_ARM;
    setnotworn(uarm);
    context.takeoff.cancelled_don = FALSE;
    return 0;
}

STATIC_OVL void
Amulet_on()
{
    /* make sure amulet isn't wielded; can't use remove_worn_item()
       here because it has already been set worn in amulet slot */
    if (uamul == uwep)
        setuwep((struct obj *) 0);
    else if (uamul == uswapwep)
        setuswapwep((struct obj *) 0);
    else if (uamul == uquiver)
        setuqwep((struct obj *) 0);

    switch (uamul->otyp) {
    case AMULET_OF_ESP:
    case AMULET_OF_LIFE_SAVING:
    case AMULET_VERSUS_POISON:
    case AMULET_OF_REFLECTION:
    case AMULET_OF_MAGICAL_BREATHING:
    case FAKE_AMULET_OF_YENDOR:
        break;
    case AMULET_OF_UNCHANGING:
        if (Slimed)
            make_slimed(0L, (char *) 0);
        break;
    case AMULET_OF_CHANGE: {
        int orig_sex = poly_gender();

        if (Unchanging)
            break;
        change_sex();
        /* Don't use same message as polymorph */
        if (orig_sex != poly_gender()) {
            makeknown(AMULET_OF_CHANGE);
#if 0 /*KR: 원본*/
            You("are suddenly very %s!",
                flags.female ? "feminine" : "masculine");
#else /*KR: KRNethack 맞춤 번역 */
            You("갑자기 매우 %s 변했다!",
                flags.female ? "여성스럽게" : "남성스럽게");
#endif
            context.botl = 1;
        } else
            /* already polymorphed into single-gender monster; only
               changed the character's base sex */
            /*KR You("don't feel like yourself."); */
            You("평소의 당신 같지 않은 느낌이다.");
        /*KR pline_The("amulet disintegrates!"); */
        pline("부적이 산산조각 났다!");
        if (orig_sex == poly_gender() && uamul->dknown
            && !objects[AMULET_OF_CHANGE].oc_name_known
            && !objects[AMULET_OF_CHANGE].oc_uname)
            docall(uamul);
        useup(uamul);
        break;
    }
    case AMULET_OF_STRANGULATION:
        if (can_be_strangled(&youmonst)) {
            makeknown(AMULET_OF_STRANGULATION);
            Strangled = 6L;
            context.botl = TRUE;
            /*KR pline("It constricts your throat!"); */
            pline("이것이 당신의 목을 조른다!");
        }
        break;
    case AMULET_OF_RESTFUL_SLEEP: {
        long newnap = (long) rnd(100), oldnap = (HSleepy & TIMEOUT);

        /* avoid clobbering FROMOUTSIDE bit, which might have
           gotten set by previously eating one of these amulets */
        if (newnap < oldnap || oldnap == 0L)
            HSleepy = (HSleepy & ~TIMEOUT) | newnap;
    } break;
    case AMULET_OF_YENDOR:
        break;
    }
}

void
Amulet_off()
{
    context.takeoff.mask &= ~W_AMUL;

    switch (uamul->otyp) {
    case AMULET_OF_ESP:
        /* need to update ability before calling see_monsters() */
        setworn((struct obj *) 0, W_AMUL);
        see_monsters();
        return;
    case AMULET_OF_LIFE_SAVING:
    case AMULET_VERSUS_POISON:
    case AMULET_OF_REFLECTION:
    case AMULET_OF_CHANGE:
    case AMULET_OF_UNCHANGING:
    case FAKE_AMULET_OF_YENDOR:
        break;
    case AMULET_OF_MAGICAL_BREATHING:
        if (Underwater) {
            /* HMagical_breathing must be set off
                before calling drown() */
            setworn((struct obj *) 0, W_AMUL);
            if (!breathless(youmonst.data) && !amphibious(youmonst.data)
                && !Swimming) {
#if 0 /*KR: 원본*/
                You("suddenly inhale an unhealthy amount of %s!",
                    hliquid("water"));
#else /*KR: KRNethack 맞춤 번역 */
                You("갑자기 엄청난 양의 %s 들이마셨다!",
                    append_josa(hliquid("물"), "을"));
#endif
                (void) drown();
            }
            return;
        }
        break;
    case AMULET_OF_STRANGULATION:
        if (Strangled) {
            Strangled = 0L;
            context.botl = TRUE;
            if (Breathless)
#if 0 /*KR: 원본*/
                Your("%s is no longer constricted!", body_part(NECK));
#else /*KR: KRNethack 맞춤 번역 */
                pline("당신의 %s 더 이상 조이지 않는다!",
                      append_josa(body_part(NECK), "은"));
#endif
            else
                /*KR You("can breathe more easily!"); */
                You("숨쉬기가 한결 편해졌다!");
        }
        break;
    case AMULET_OF_RESTFUL_SLEEP:
        setworn((struct obj *) 0, W_AMUL);
        /* HSleepy = 0L; -- avoid clobbering FROMOUTSIDE bit */
        if (!ESleepy && !(HSleepy & ~TIMEOUT))
            HSleepy &= ~TIMEOUT; /* clear timeout bits */
        return;
    case AMULET_OF_YENDOR:
        break;
    }
    setworn((struct obj *) 0, W_AMUL);
    return;
}

/* handle ring discovery; comparable to learnwand() */
STATIC_OVL void 
learnring(ring, observed) 
struct obj *ring;
boolean observed;
{
    int ringtype = ring->otyp;

    /* if effect was observeable then we usually discover the type */
    if (observed) {
        /* if we already know the ring type which accomplishes this
           effect (assumes there is at most one type for each effect),
           mark this ring as having been seen (no need for makeknown);
           otherwise if we have seen this ring, discover its type */
        if (objects[ringtype].oc_name_known)
            ring->dknown = 1;
        else if (ring->dknown)
            makeknown(ringtype);
#if 0 /* see learnwand() */
        else
            ring->eknown = 1;
#endif
    }

    /* make enchantment of charged ring known (might be +0) and update
       perm invent window if we've seen this ring and know its type */
    if (ring->dknown && objects[ringtype].oc_name_known) {
        if (objects[ringtype].oc_charged)
            ring->known = 1;
        update_inventory();
    }
}

void 
Ring_on(obj) 
register struct obj *obj;
{
    long oldprop = u.uprops[objects[obj->otyp].oc_oprop].extrinsic;
    int old_attrib, which;
    boolean observable;

    /* make sure ring isn't wielded; can't use remove_worn_item()
       here because it has already been set worn in a ring slot */
    if (obj == uwep)
        setuwep((struct obj *) 0);
    else if (obj == uswapwep)
        setuswapwep((struct obj *) 0);
    else if (obj == uquiver)
        setuqwep((struct obj *) 0);

    /* only mask out W_RING when we don't have both
       left and right rings of the same type */
    if ((oldprop & W_RING) != W_RING)
        oldprop &= ~W_RING;

    switch (obj->otyp) {
    case RIN_TELEPORTATION:
    case RIN_REGENERATION:
    case RIN_SEARCHING:
    case RIN_HUNGER:
    case RIN_AGGRAVATE_MONSTER:
    case RIN_POISON_RESISTANCE:
    case RIN_FIRE_RESISTANCE:
    case RIN_COLD_RESISTANCE:
    case RIN_SHOCK_RESISTANCE:
    case RIN_CONFLICT:
    case RIN_TELEPORT_CONTROL:
    case RIN_POLYMORPH:
    case RIN_POLYMORPH_CONTROL:
    case RIN_FREE_ACTION:
    case RIN_SLOW_DIGESTION:
    case RIN_SUSTAIN_ABILITY:
    case MEAT_RING:
        break;
    case RIN_STEALTH:
        toggle_stealth(obj, oldprop, TRUE);
        break;
    case RIN_WARNING:
        see_monsters();
        break;
    case RIN_SEE_INVISIBLE:
        /* can now see invisible monsters */
        set_mimic_blocking(); /* do special mimic handling */
        see_monsters();

        if (Invis && !oldprop && !HSee_invisible && !Blind) {
            newsym(u.ux, u.uy);
            /*KR pline("Suddenly you are transparent, but there!"); */
            pline("갑자기 당신은 투명해졌지만, 여전히 거기에 있다!");
            learnring(obj, TRUE);
        }
        break;
    case RIN_INVISIBILITY:
        if (!oldprop && !HInvis && !BInvis && !Blind) {
            learnring(obj, TRUE);
            newsym(u.ux, u.uy);
            self_invis_message();
        }
        break;
    case RIN_LEVITATION:
        if (!oldprop && !HLevitation && !(BLevitation & FROMOUTSIDE)) {
            float_up();
            learnring(obj, TRUE);
            if (Levitation)
                spoteffects(FALSE); /* for sinks */
        } else {
            float_vs_flight(); /* maybe toggle (BFlying & I_SPECIAL) */
        }
        break;
    case RIN_GAIN_STRENGTH:
        which = A_STR;
        goto adjust_attrib;
    case RIN_GAIN_CONSTITUTION:
        which = A_CON;
        goto adjust_attrib;
    case RIN_ADORNMENT:
        which = A_CHA;
 adjust_attrib:
        old_attrib = ACURR(which);
        ABON(which) += obj->spe;
        observable = (old_attrib != ACURR(which));
        /* if didn't change, usually means ring is +0 but might
           be because nonzero couldn't go below min or above max;
           learn +0 enchantment if attribute value is not stuck
           at a limit [and ring has been seen and its type is
           already discovered, both handled by learnring()] */
        if (observable || !extremeattr(which))
            learnring(obj, observable);
        context.botl = 1;
        break;
    case RIN_INCREASE_ACCURACY: /* KMH */
        u.uhitinc += obj->spe;
        break;
    case RIN_INCREASE_DAMAGE:
        u.udaminc += obj->spe;
        break;
    case RIN_PROTECTION_FROM_SHAPE_CHAN:
        rescham();
        break;
    case RIN_PROTECTION:
        /* usually learn enchantment and discover type;
           won't happen if ring is unseen or if it's +0
           and the type hasn't been discovered yet */
        observable = (obj->spe != 0);
        learnring(obj, observable);
        if (obj->spe)
            find_ac(); /* updates botl */
        break;
    }
}

STATIC_OVL void
Ring_off_or_gone(obj, gone)
register struct obj *obj;
boolean gone;
{
    long mask = (obj->owornmask & W_RING);
    int old_attrib, which;
    boolean observable;

    context.takeoff.mask &= ~mask;
    if (!(u.uprops[objects[obj->otyp].oc_oprop].extrinsic & mask))
        impossible("Strange... I didn't know you had that ring.");
    if (gone)
        setnotworn(obj);
    else
        setworn((struct obj *) 0, obj->owornmask);

    switch (obj->otyp) {
    case RIN_TELEPORTATION:
    case RIN_REGENERATION:
    case RIN_SEARCHING:
    case RIN_HUNGER:
    case RIN_AGGRAVATE_MONSTER:
    case RIN_POISON_RESISTANCE:
    case RIN_FIRE_RESISTANCE:
    case RIN_COLD_RESISTANCE:
    case RIN_SHOCK_RESISTANCE:
    case RIN_CONFLICT:
    case RIN_TELEPORT_CONTROL:
    case RIN_POLYMORPH:
    case RIN_POLYMORPH_CONTROL:
    case RIN_FREE_ACTION:
    case RIN_SLOW_DIGESTION:
    case RIN_SUSTAIN_ABILITY:
    case MEAT_RING:
        break;
    case RIN_STEALTH:
        toggle_stealth(obj, (EStealth & ~mask), FALSE);
        break;
    case RIN_WARNING:
        see_monsters();
        break;
    case RIN_SEE_INVISIBLE:
        /* Make invisible monsters go away */
        if (!See_invisible) {
            set_mimic_blocking(); /* do special mimic handling */
            see_monsters();
        }

        if (Invisible && !Blind) {
            newsym(u.ux, u.uy);
#if 0 /*KR: 원본*/
            pline("Suddenly you cannot see yourself.");
#else /*KR: KRNethack 맞춤 번역 */
            pline("갑자기 자기 자신이 보이지 않게 되었다.");
#endif
            learnring(obj, TRUE);
        }
        break;
    case RIN_INVISIBILITY:
        if (!Invis && !BInvis && !Blind) {
            newsym(u.ux, u.uy);
#if 0 /*KR: 원본*/
            Your("body seems to unfade%s.",
                 See_invisible ? " completely" : "..");
#else /*KR: KRNethack 맞춤 번역 */
            Your("몸이 %s 다시 나타나는 것 같다.",
                 See_invisible ? "완전히" : "서서히");
#endif
            learnring(obj, TRUE);
        }
        break;
    case RIN_LEVITATION:
        if (!(BLevitation & FROMOUTSIDE)) {
            (void) float_down(0L, 0L);
            if (!Levitation)
                learnring(obj, TRUE);
        } else {
            float_vs_flight(); /* maybe toggle (BFlying & I_SPECIAL) */
        }
        break;
    case RIN_GAIN_STRENGTH:
        which = A_STR;
        goto adjust_attrib;
    case RIN_GAIN_CONSTITUTION:
        which = A_CON;
        goto adjust_attrib;
    case RIN_ADORNMENT:
        which = A_CHA;
    adjust_attrib:
        old_attrib = ACURR(which);
        ABON(which) -= obj->spe;
        observable = (old_attrib != ACURR(which));
        /* same criteria as Ring_on() */
        if (observable || !extremeattr(which))
            learnring(obj, observable);
        context.botl = 1;
        break;
    case RIN_INCREASE_ACCURACY: /* KMH */
        u.uhitinc -= obj->spe;
        break;
    case RIN_INCREASE_DAMAGE:
        u.udaminc -= obj->spe;
        break;
    case RIN_PROTECTION:
        /* might have been put on while blind and we can now see
           or perhaps been forgotten due to amnesia */
        observable = (obj->spe != 0);
        learnring(obj, observable);
        if (obj->spe)
            find_ac(); /* updates botl */
        break;
    case RIN_PROTECTION_FROM_SHAPE_CHAN:
        /* If you're no longer protected, let the chameleons
         * change shape again -dgk
         */
        restartcham();
        break;
    }
}

void Ring_off(obj) struct obj *obj;
{
    Ring_off_or_gone(obj, FALSE);
}

void Ring_gone(obj) struct obj *obj;
{
    Ring_off_or_gone(obj, TRUE);
}

void Blindf_on(otmp) struct obj *otmp;
{
    boolean already_blind = Blind, changed = FALSE;

    /* blindfold might be wielded; release it for wearing */
    if (otmp->owornmask & W_WEAPONS)
        remove_worn_item(otmp, FALSE);
    setworn(otmp, W_TOOL);
    on_msg(otmp);

    if (Blind && !already_blind) {
        changed = TRUE;
        if (flags.verbose)
            /*KR You_cant("see any more."); */
            You("더 이상 아무것도 보이지 않는다.");
        /* set ball&chain variables before the hero goes blind */
        if (Punished)
            set_bc(0);
    } else if (already_blind && !Blind) {
        changed = TRUE;
        /* "You are now wearing the Eyes of the Overworld." */
        if (u.uroleplay.blind) {
            /* this can only happen by putting on the Eyes of the Overworld;
               that shouldn't actually produce a permanent cure, but we
               can't let the "blind from birth" conduct remain intact */
            /*KR pline("For the first time in your life, you can see!"); */
            pline(
                "당신은 태어나서 처음으로 눈앞의 풍경을 볼 수 있게 되었다!");
            u.uroleplay.blind = FALSE;
        } else
            /*KR You("can see!"); */
            You("눈이 보인다!");
    }
    if (changed) {
        toggle_blindness(); /* potion.c */
    }
}

void Blindf_off(otmp) struct obj *otmp;
{
    boolean was_blind = Blind, changed = FALSE;

    if (!otmp) {
        impossible("Blindf_off without otmp");
        return;
    }
    context.takeoff.mask &= ~W_TOOL;
    setworn((struct obj *) 0, otmp->owornmask);
    off_msg(otmp);

    if (Blind) {
        if (was_blind) {
            /* "still cannot see" makes no sense when removing lenses
               since they can't have been the cause of your blindness */
            if (otmp->otyp != LENSES)
                /*KR You("still cannot see."); */
                You("아직 눈이 보이지 않는다.");
        } else {
            changed = TRUE; /* !was_blind */
            /* "You were wearing the Eyes of the Overworld." */
            /*KR You_cant("see anything now!"); */
            You("이제 아무것도 보이지 않는다!");
            /* set ball&chain variables before the hero goes blind */
            if (Punished)
                set_bc(0);
        }
    } else if (was_blind) {
        if (!gulp_blnd_check()) {
            changed = TRUE; /* !Blind */
            /*KR You("can see again."); */
            You("다시 앞이 보인다.");
        }
    }
    if (changed) {
        toggle_blindness(); /* potion.c */
    }
}

/* called in moveloop()'s prologue to set side-effects of worn start-up items;
   also used by poly_obj() when a worn item gets transformed */
void set_wear(obj) struct obj
    *obj; /* if null, do all worn items; otherwise just obj itself */
{
    initial_don = !obj;

    if (!obj ? ublindf != 0 : (obj == ublindf))
        (void) Blindf_on(ublindf);
    if (!obj ? uright != 0 : (obj == uright))
        (void) Ring_on(uright);
    if (!obj ? uleft != 0 : (obj == uleft))
        (void) Ring_on(uleft);
    if (!obj ? uamul != 0 : (obj == uamul))
        (void) Amulet_on();

    if (!obj ? uarmu != 0 : (obj == uarmu))
        (void) Shirt_on();
    if (!obj ? uarm != 0 : (obj == uarm))
        (void) Armor_on();
    if (!obj ? uarmc != 0 : (obj == uarmc))
        (void) Cloak_on();
    if (!obj ? uarmf != 0 : (obj == uarmf))
        (void) Boots_on();
    if (!obj ? uarmg != 0 : (obj == uarmg))
        (void) Gloves_on();
    if (!obj ? uarmh != 0 : (obj == uarmh))
        (void) Helmet_on();
    if (!obj ? uarms != 0 : (obj == uarms))
        (void) Shield_on();

    initial_don = FALSE;
}

/* check whether the target object is currently being put on (or taken off--
   also checks for doffing--[why?]) */
boolean
donning(otmp)
struct obj *otmp;
{
    boolean result = FALSE;

    /* 'W' (or 'P' used for armor) sets afternmv */
    if (doffing(otmp))
        result = TRUE;
    else if (otmp == uarm)
        result = (afternmv == Armor_on);
    else if (otmp == uarmu)
        result = (afternmv == Shirt_on);
    else if (otmp == uarmc)
        result = (afternmv == Cloak_on);
    else if (otmp == uarmf)
        result = (afternmv == Boots_on);
    else if (otmp == uarmh)
        result = (afternmv == Helmet_on);
    else if (otmp == uarmg)
        result = (afternmv == Gloves_on);
    else if (otmp == uarms)
        result = (afternmv == Shield_on);

    return result;
}

/* check whether the target object is currently being taken off,
   so that stop_donning() and steal() can vary messages and doname()
   can vary "(being worn)" suffix */
boolean
doffing(otmp)
struct obj *otmp;
{
    long what = context.takeoff.what;
    boolean result = FALSE;

    /* 'T' (or 'R' used for armor) sets afternmv, 'A' sets takeoff.what */
    if (otmp == uarm)
        result = (afternmv == Armor_off || what == WORN_ARMOR);
    else if (otmp == uarmu)
        result = (afternmv == Shirt_off || what == WORN_SHIRT);
    else if (otmp == uarmc)
        result = (afternmv == Cloak_off || what == WORN_CLOAK);
    else if (otmp == uarmf)
        result = (afternmv == Boots_off || what == WORN_BOOTS);
    else if (otmp == uarmh)
        result = (afternmv == Helmet_off || what == WORN_HELMET);
    else if (otmp == uarmg)
        result = (afternmv == Gloves_off || what == WORN_GLOVES);
    else if (otmp == uarms)
        result = (afternmv == Shield_off || what == WORN_SHIELD);
    /* these 1-turn items don't need 'afternmv' checks */
    else if (otmp == uamul)
        result = (what == WORN_AMUL);
    else if (otmp == uleft)
        result = (what == LEFT_RING);
    else if (otmp == uright)
        result = (what == RIGHT_RING);
    else if (otmp == ublindf)
        result = (what == WORN_BLINDF);
    else if (otmp == uwep)
        result = (what == W_WEP);
    else if (otmp == uswapwep)
        result = (what == W_SWAPWEP);
    else if (otmp == uquiver)
        result = (what == W_QUIVER);

    return result;
}

/* despite their names, cancel_don() and cancel_doff() both apply to both
   donning and doffing... */
void cancel_doff(obj, slotmask) struct obj *obj;
long slotmask;
{
    /* Called by setworn() for old item in specified slot or by setnotworn()
     * for specified item.  We don't want to call cancel_don() if we got
     * here via <X>_off() -> setworn((struct obj *)0) -> cancel_doff()
     * because that would stop the 'A' command from continuing with next
     * selected item.  So do_takeoff() sets a flag in takeoff.mask for us.
     * [For taking off an individual item with 'T'/'R'/'w-', it doesn't
     * matter whether cancel_don() gets called here--the item has already
     * been removed by now.]
     */
    if (!(context.takeoff.mask & I_SPECIAL) && donning(obj))
        cancel_don(); /* applies to doffing too */
    context.takeoff.mask &= ~slotmask;
}

/* despite their names, cancel_don() and cancel_doff() both apply to both
   donning and doffing... */
void
cancel_don()
{
    /* the piece of armor we were donning/doffing has vanished, so stop
     * wasting time on it (and don't dereference it when donning would
     * otherwise finish)
     */
    context.takeoff.cancelled_don =
        (afternmv == Boots_on || afternmv == Helmet_on
         || afternmv == Gloves_on || afternmv == Armor_on);
    afternmv = (int NDECL((*) )) 0;
    nomovemsg = (char *) 0;
    multi = 0;
    context.takeoff.delay = 0;
    context.takeoff.what = 0L;
}

/* called by steal() during theft from hero; interrupt donning/doffing */
int
stop_donning(stolenobj)
struct obj *stolenobj; /* no message if stolenobj is already being doffing */
{
    char buf[BUFSZ];
    struct obj *otmp;
    boolean putting_on;
    int result = 0;

    for (otmp = invent; otmp; otmp = otmp->nobj)
        if ((otmp->owornmask & W_ARMOR) && donning(otmp))
            break;
    /* at most one item will pass donning() test at any given time */
    if (!otmp)
        return 0;

    /* donning() returns True when doffing too; doffing() is more specific */
    putting_on = !doffing(otmp);
    /* cancel_don() looks at afternmv; it can also cancel doffing */
    cancel_don();
    /* don't want <armor>_on() or <armor>_off() being called
       by unmul() since the on or off action isn't completing */
    afternmv = (int NDECL((*) )) 0;
    if (putting_on || otmp != stolenobj) {
#if 0 /*KR: 원본*/
        Sprintf(buf, "You stop %s %s.",
                putting_on ? "putting on" : "taking off",
                thesimpleoname(otmp));
#else /*KR: KRNethack 맞춤 번역 */
        Sprintf(buf, "당신은 %s %s 것을 멈췄다.",
                append_josa(thesimpleoname(otmp), "을"),
                putting_on ? "입는" : "벗는");
#endif
    } else {
        buf[0] = '\0';   /* silently stop doffing stolenobj */
        result = -multi; /* remember this before calling unmul() */
    }
    unmul(buf);
    /* while putting on, item becomes worn immediately but side-effects are
       deferred until the delay expires; when interrupted, make it unworn
       (while taking off, item stays worn until the delay expires; when
       interrupted, leave it worn) */
    if (putting_on)
        remove_worn_item(otmp, FALSE);

    return result;
}

/* both 'clothes' and 'accessories' now include both armor and accessories;
   TOOL_CLASS is for eyewear, FOOD_CLASS is for MEAT_RING */
static NEARDATA const char clothes[] = { ARMOR_CLASS,  RING_CLASS,
                                         AMULET_CLASS, TOOL_CLASS,
                                         FOOD_CLASS,   0 };
static NEARDATA const char accessories[] = { RING_CLASS,  AMULET_CLASS,
                                             TOOL_CLASS,  FOOD_CLASS,
                                             ARMOR_CLASS, 0 };
STATIC_VAR NEARDATA int Narmorpieces, Naccessories;

/* assign values to Narmorpieces and Naccessories */
STATIC_OVL void count_worn_stuff(
    which,
    accessorizing) struct obj **which; /* caller wants this when count is 1 */
boolean accessorizing;
{
    struct obj *otmp;

    Narmorpieces = Naccessories = 0;

#define MOREWORN(x, wtyp) \
    do {                  \
        if (x) {          \
            wtyp++;       \
            otmp = x;     \
        }                 \
    } while (0)
    otmp = 0;
    MOREWORN(uarmh, Narmorpieces);
    MOREWORN(uarms, Narmorpieces);
    MOREWORN(uarmg, Narmorpieces);
    MOREWORN(uarmf, Narmorpieces);
    /* for cloak/suit/shirt, we only count the outermost item so that it
       can be taken off without confirmation if final count ends up as 1 */
    if (uarmc)
        MOREWORN(uarmc, Narmorpieces);
    else if (uarm)
        MOREWORN(uarm, Narmorpieces);
    else if (uarmu)
        MOREWORN(uarmu, Narmorpieces);
    if (!accessorizing)
        *which = otmp; /* default item iff Narmorpieces is 1 */

    otmp = 0;
    MOREWORN(uleft, Naccessories);
    MOREWORN(uright, Naccessories);
    MOREWORN(uamul, Naccessories);
    MOREWORN(ublindf, Naccessories);
    if (accessorizing)
        *which = otmp; /* default item iff Naccessories is 1 */
#undef MOREWORN
}

/* take off one piece or armor or one accessory;
   shared by dotakeoff('T') and doremring('R') */
STATIC_OVL int
armor_or_accessory_off(obj)
struct obj *obj;
{
    if (!(obj->owornmask & (W_ARMOR | W_ACCESSORY))) {
        /*KR You("are not wearing that."); */
        You("그것은 장비하고 있지 않다.");
        return 0;
    }
    if (obj == uskin || ((obj == uarm) && uarmc)
        || ((obj == uarmu) && (uarmc || uarm))) {
        char why[QBUFSZ], what[QBUFSZ];

        why[0] = what[0] = '\0';
        if (obj != uskin) {
            if (uarmc)
                Strcat(what, cloak_simple_name(uarmc));
            if ((obj == uarmu) && uarm) {
                if (uarmc)
                    /*KR Strcat(what, " and "); */
                    Strcat(what, "(와)과 ");
                Strcat(what, suit_simple_name(uarm));
            }
#if 0 /*KR: 원본*/
            Sprintf(why, " without taking off your %s first", what);
#else /*KR: KRNethack 맞춤 번역 */
            Sprintf(why, " 먼저 %s 벗어야 한다", append_josa(what, "을"));
#endif
        } else {
            /*KR Strcpy(why, "; it's embedded"); */
            Strcpy(why, " 그것은 피부와 융합되어 있다");
        }
#if 0 /*KR: 원본*/
        You_cant("take that off%s.", why);
#else /*KR: KRNethack 맞춤 번역 */
        pline("그것을 벗을 수는 없다;%s.", why);
#endif
        return 0;
    }

    reset_remarm(); /* clear context.takeoff.mask and context.takeoff.what */
    (void) select_off(obj);
    if (!context.takeoff.mask)
        return 0;
    /* none of armoroff()/Ring_/Amulet/Blindf_off() use context.takeoff.mask
     */
    reset_remarm();

    if (obj->owornmask & W_ARMOR) {
        (void) armoroff(obj);
    } else if (obj == uright || obj == uleft) {
        /* Sometimes we want to give the off_msg before removing and
         * sometimes after; for instance, "you were wearing a moonstone
         * ring (on right hand)" is desired but "you were wearing a
         * square amulet (being worn)" is not because of the redundant
         * "being worn".
         */
        off_msg(obj);
        Ring_off(obj);
    } else if (obj == uamul) {
        Amulet_off();
        off_msg(obj);
    } else if (obj == ublindf) {
        Blindf_off(obj); /* does its own off_msg */
    } else {
        impossible("removing strange accessory?");
        if (obj->owornmask)
            remove_worn_item(obj, FALSE);
    }
    return 1;
}

/* the 'T' command */
int
dotakeoff()
{
    struct obj *otmp = (struct obj *) 0;

    count_worn_stuff(&otmp, FALSE);
    if (!Narmorpieces && !Naccessories) {
        /* assert( GRAY_DRAGON_SCALES > YELLOW_DRAGON_SCALE_MAIL ); */
        if (uskin)
#if 0 /*KR: 원본*/
            pline_The("%s merged with your skin!",
                      uskin->otyp >= GRAY_DRAGON_SCALES
                          ? "dragon scales are"
                          : "dragon scale mail is");
#else /*KR: KRNethack 맞춤 번역 */
            pline("드래곤 비늘%s 피부와 융합되어 있다!",
                  uskin->otyp >= GRAY_DRAGON_SCALES ? "이" : " 갑옷이");
#endif
        else
            /*KR pline("Not wearing any armor or accessories."); */
            pline("어떤 갑옷이나 장신구도 착용하고 있지 않다.");
        return 0;
    }
    if (Narmorpieces != 1 || ParanoidRemove)
        otmp = getobj(clothes, "take off");
    if (!otmp)
        return 0;

    return armor_or_accessory_off(otmp);
}

/* the 'R' command */
int
doremring()
{
    struct obj *otmp = 0;

    count_worn_stuff(&otmp, TRUE);
    if (!Naccessories && !Narmorpieces) {
        /*KR pline("Not wearing any accessories or armor."); */
        pline("어떤 장신구나 갑옷도 착용하고 있지 않다.");
        return 0;
    }
    if (Naccessories != 1 || ParanoidRemove)
        otmp = getobj(accessories, "remove");
    if (!otmp)
        return 0;

    return armor_or_accessory_off(otmp);
}

/* Check if something worn is cursed _and_ unremovable. */
int
cursed(otmp)
struct obj *otmp;
{
    if (!otmp) {
        impossible("cursed without otmp");
        return 0;
    }
    /* Curses, like chickens, come home to roost. */
    if ((otmp == uwep) ? welded(otmp) : (int) otmp->cursed) {
        /* boolean use_plural = (is_boots(otmp) || is_gloves(otmp)
                              || otmp->otyp == LENSES || otmp->quan > 1L); */

        /* might be trying again after applying grease to hands */
        if (Glib
            && otmp->bknown
            /* for weapon, we'll only get here via 'A )' */
            && (uarmg ? (otmp == uwep)
                      : ((otmp->owornmask & (W_WEP | W_RING)) != 0)))
#if 0 /*KR: 원본*/
            pline("Despite your slippery %s, you can't.",
                  fingers_or_gloves(TRUE));
#else /*KR: KRNethack 맞춤 번역 */
            pline("%s 미끄러운데도 뺄 수 없다.",
                  append_josa(fingers_or_gloves(TRUE), "이"));
#endif
        else
#if 0 /*KR:T*/
            You("can't.  %s cursed.", use_plural ? "They are" : "It is");
#else
            pline("무리다. 그것은 저주받았다.");
#endif
        set_bknown(otmp, 1);
        return 1;
    }
    return 0;
}

int
armoroff(otmp)
struct obj *otmp;
{
    static char offdelaybuf[60];
    int delay = -objects[otmp->otyp].oc_delay;
    const char *what = 0;

    if (cursed(otmp))
        return 0;
    /* this used to make assumptions about which types of armor had
       delays and which didn't; now both are handled for all types */
    if (delay) {
        nomul(delay);
        /*KR multi_reason = "dressing up"; */
        multi_reason = "옷을 입느라";
        if (is_helmet(otmp)) {
            what = helm_simple_name(otmp);
            afternmv = Helmet_off;
        } else if (is_gloves(otmp)) {
            what = gloves_simple_name(otmp);
            afternmv = Gloves_off;
        } else if (is_boots(otmp)) {
            what = c_boots;
            afternmv = Boots_off;
        } else if (is_suit(otmp)) {
            what = suit_simple_name(otmp);
            afternmv = Armor_off;
        } else if (is_cloak(otmp)) {
            what = cloak_simple_name(otmp);
            afternmv = Cloak_off;
        } else if (is_shield(otmp)) {
            what = c_shield;
            afternmv = Shield_off;
        } else if (is_shirt(otmp)) {
            what = c_shirt;
            afternmv = Shirt_off;
        } else {
            impossible("Taking off unknown armor (%d: %d), delay %d",
                       otmp->otyp, objects[otmp->otyp].oc_armcat, delay);
        }
        if (what) {
            /*KR Sprintf(offdelaybuf, "You finish taking off your %s.", what);
             */
            Sprintf(offdelaybuf, "%s 벗기를 마쳤다.",
                    append_josa(what, "을"));
            /*KR nomovemsg = "You finish your dressing maneuver."; */
            nomovemsg = "옷 입기를 마쳤다.";
            nomovemsg = offdelaybuf;
        }
    } else {
        /* Be warned!  We want off_msg after removing the item to
         * avoid "You were wearing ____ (being worn)."  However, an
         * item which grants fire resistance might cause some trouble
         * if removed in Hell and lifesaving puts it back on; in this
         * case the message will be printed at the wrong time (after
         * the messages saying you died and were lifesaved).  Luckily,
         * no cloak, shield, or fast-removable armor grants fire
         * resistance, so we can safely do the off_msg afterwards.
         * Rings do grant fire resistance, but for rings we want the
         * off_msg before removal anyway so there's no problem.  Take
         * care in adding armors granting fire resistance; this code
         * might need modification.
         * 3.2 (actually 3.1 even): that comment is obsolete since
         * fire resistance is not required for Gehennom so setworn()
         * doesn't force the resistance granting item to be re-worn
         * after being lifesaved anymore.
         */
        if (is_cloak(otmp))
            (void) Cloak_off();
        else if (is_shield(otmp))
            (void) Shield_off();
        else if (is_helmet(otmp))
            (void) Helmet_off();
        else if (is_gloves(otmp))
            (void) Gloves_off();
        else if (is_boots(otmp))
            (void) Boots_off();
        else if (is_shirt(otmp))
            (void) Shirt_off();
        else if (is_suit(otmp))
            (void) Armor_off();
        else
            impossible("Taking off unknown armor (%d: %d), no delay",
                       otmp->otyp, objects[otmp->otyp].oc_armcat);
        off_msg(otmp);
    }
    context.takeoff.mask = context.takeoff.what = 0L;
    return 1;
}

#if 0 /*KR: 원본 코드*/
STATIC_OVL void
already_wearing(cc)
const char *cc;
{
    You("are already wearing %s%c", cc, (cc == c_that_) ? '!' : '.');
}
#else
/* 한국어 맞춤형 장착 동사 반환 함수 */
STATIC_OVL const char *
kr_wear_verb(otmp)
struct obj *otmp;
{
    if (!otmp)
        return "장비하고";

    /* 부위에 따라 알맞은 동사를 반환합니다 */
    if (is_helmet(otmp))
        return "쓰고";
    if (is_boots(otmp))
        return "신고";
    if (is_gloves(otmp) || otmp->oclass == RING_CLASS
        || otmp->otyp == MEAT_RING)
        return "끼고";
    if (is_shield(otmp) || is_sword(otmp) || is_weptool(otmp)
        || otmp->oclass == WEAPON_CLASS)
        return "들고";
    if (is_suit(otmp) || is_shirt(otmp) || is_cloak(otmp))
        return "입고";

    /* 기본값 */
    return "장비하고";
}

STATIC_OVL void already_wearing(cc, otmp) const char *cc;
struct obj *otmp;
{
    /* append_josa를 통해 cc("그것", "가죽 모자" 등)에 알맞은 을/를을
     * 붙여줍니다! */
    if (!strcmp(cc, "그것")) {
        You("이미 %s %s 있습니다!", append_josa(cc, "을"),
            kr_wear_verb(otmp));
    } else {
        You("이미 %s %s 있습니다.", append_josa(cc, "을"),
            kr_wear_verb(otmp));
    }
}
#endif

STATIC_OVL void
already_wearing2(cc1, cc2)
const char *cc1, *cc2;
{
#if 0 /*KR: 원본*/
    You_cant("wear %s because you're wearing %s there already.", cc1, cc2);
#else
    /* 예: "이미 망토를 입고 있어서 갑옷을 입을 수 없다."
     * cc2가 이미 입고 있는 것, cc1이 새로 입으려는 것입니다. */
    pline("이미 %s 장비하고 있기 때문에 %s 장비할 수 없습니다.",
          append_josa(cc2, "을"), append_josa(cc1, "을"));
#endif
}

/*
 * canwearobj checks to see whether the player can wear a piece of armor
 *
 * inputs: otmp (the piece of armor)
 *         noisy (if TRUE give error messages, otherwise be quiet about it)
 * output: mask (otmp's armor type)
 */
int
canwearobj(otmp, mask, noisy)
struct obj *otmp;
long *mask;
boolean noisy;
{
    int err = 0;
    const char *which;

    /* this is the same check as for 'W' (dowear), but different message,
       in case we get here via 'P' (doputon) */
    if (verysmall(youmonst.data) || nohands(youmonst.data)) {
        if (noisy)
       /*KR You("can't wear any armor in your current form."); */
            You("현재 모습으로는 방어구를 착용할 수 없다.");
        return 0;
    }

    which = is_cloak(otmp)
                ? c_cloak
                : is_shirt(otmp)
                    ? c_shirt
                    : is_suit(otmp)
                        ? c_suit
                        : 0;
    if (which && cantweararm(youmonst.data)
        /* same exception for cloaks as used in m_dowear() */
        && (which != c_cloak || youmonst.data->msize != MZ_SMALL)
        && (racial_exception(&youmonst, otmp) < 1)) {
        if (noisy)
       /*KR pline_The("%s will not fit on your body.", which); */
            pline("%s 당신의 몸에 맞지 않는다.", append_josa(which, "은"));
        return 0;
    } else if (otmp->owornmask & W_ARMOR) {
        if (noisy)
#if 0 /*KR*/
            already_wearing(c_that_);
#else
            already_wearing(c_that_, otmp);
#endif
        return 0;
    }

    if (welded(uwep) && bimanual(uwep) && (is_suit(otmp) || is_shirt(otmp))) {
        if (noisy)
            /*KR You("cannot do that while holding your %s.", */
            pline("%s 들고 있는 채로는 그것을 할 수 없다.",
                append_josa((is_sword(uwep) ? c_sword : c_weapon), "을"));
        return 0;
    }

    if (is_helmet(otmp)) {
        if (uarmh) {
            if (noisy)
#if 0 /*KR*/
                already_wearing(an(helm_simple_name(uarmh)));
#else
                already_wearing(helm_simple_name(uarmh), uarmh);
#endif           
            err++;
        } else if (Upolyd && has_horns(youmonst.data) && !is_flimsy(otmp)) {
            /* (flimsy exception matches polyself handling) */
            if (noisy)
#if 0 /*KR:T*/
                pline_The("%s won't fit over your horn%s.",
                          helm_simple_name(otmp),
                          plur(num_horns(youmonst.data)));
#else
                pline("뿔이 방해가 되서 %s 쓸 수 없다.",
                      append_josa((helm_simple_name(otmp)), "을"));
#endif
            err++;
        } else
            *mask = W_ARMH;
    } else if (is_shield(otmp)) {
        if (uarms) {
            if (noisy)
#if 0 /*KR*/
                already_wearing(an(c_shield));
#else
                already_wearing(c_shield, uarms);
#endif
            err++;
        } else if (uwep && bimanual(uwep)) {
            if (noisy)
#if 0 /*KR: 원본*/
                You("cannot wear a shield while wielding a two-handed %s.",
                    is_sword(uwep) ? c_sword 
                                   : (uwep->otyp == BATTLE_AXE) ? c_axe
                                                                : c_weapon);
#else /*KR: KRNethack 맞춤 번역 */
                You("양손 %s 쥐고 있는 동안에는 방패를 들 수 없다.",
                    append_josa(is_sword(uwep)               ? c_sword
                                : (uwep->otyp == BATTLE_AXE) ? c_axe
                                                             : c_weapon,
                                "을"));
#endif
            err++;
        } else if (u.twoweap) {
            if (noisy)
         /*KR You("cannot wear a shield while wielding two weapons."); */
                You("이도류를 사용하는 동안에는 방패를 들 수 없다.");
            err++;
        } else
            *mask = W_ARMS;
    } else if (is_boots(otmp)) {
        if (uarmf) {
            if (noisy)
#if 0 /*KR*/
                already_wearing(c_boots);
#else
                already_wearing(c_boots, uarmf);
#endif
            err++;
        } else if (Upolyd && slithy(youmonst.data)) {
            if (noisy)
                /*KR You("have no feet..."); */
                You("발이 없다..."); /* not body_part(FOOT) */
            err++;
        } else if (Upolyd && youmonst.data->mlet == S_CENTAUR) {
            /* break_armor() pushes boots off for centaurs,
               so don't let dowear() put them back on... */
            if (noisy)
#if 0 /*KR:T*/
                pline("You have too many hooves to wear %s.",
                      c_boots); /* makeplural(body_part(FOOT)) yields
                                   "rear hooves" which sounds odd */
#else
                pline("발굽이 너무 많아서 %s 신을 수 없다.",
                      append_josa(c_boots, "을"));
#endif

            err++;
        } else if (u.utrap
                   && (u.utraptype == TT_BEARTRAP || u.utraptype == TT_INFLOOR
                       || u.utraptype == TT_LAVA
                       || u.utraptype == TT_BURIEDBALL)) {
            if (u.utraptype == TT_BEARTRAP) {
                if (noisy)
#if 0 /*KR:T*/
                    Your("%s is trapped!", body_part(FOOT));
#else
                    Your("%s 덫에 걸려 있다!",
                         append_josa(body_part(FOOT), "이"));
#endif
            } else if (u.utraptype == TT_INFLOOR || u.utraptype == TT_LAVA) {
                if (noisy)
#if 0 /*KR: 원본*/
                    Your("%s are stuck in the %s!",
                         makeplural(body_part(FOOT)), surface(u.ux, u.uy));
#else /*KR: KRNethack 맞춤 번역 */
                    Your("%s %s에 빠져 있다!",
                         append_josa(makeplural(body_part(FOOT)), "이"),
                         surface(u.ux, u.uy));
#endif
            } else { /*TT_BURIEDBALL*/
                if (noisy)
#if 0 /*KR:T*/
                     Your("%s is attached to the buried ball!",
                          body_part(LEG));
#else
                     Your("%s 묻힌 철구에 연결되어 있다!",
                          append_josa(body_part(LEG), "이"));
#endif
            }
            err++;
        } else
            *mask = W_ARMF;
    } else if (is_gloves(otmp)) {
        if (uarmg) {
            if (noisy)
#if 0 /*KR*/
                already_wearing(c_gloves);
#else
                already_wearing(c_gloves, uarmg);
#endif
            err++;
        } else if (welded(uwep)) {
            if (noisy)
#if 0 /*KR: 원본*/
                 You("cannot wear gloves over your %s.",
                     is_sword(uwep) ? c_sword : c_weapon);
#else            
                 You("%s 위로 장갑을 낄 수 없다.",
                     append_josa(is_sword(uwep) ? c_sword 
                                                : c_weapon, "을"));
#endif
            err++;
        } else if (Glib) {
            /* prevent slippery bare fingers from transferring to
               gloved fingers */
            if (noisy)
#if 0 /*KR: 원본*/
                Your("%s are too slippery to pull on %s.",
                     fingers_or_gloves(FALSE), gloves_simple_name(otmp));
#else /*KR: KRNethack 맞춤 번역 */
                Your("%s 너무 미끄러워서 %s 낄 수 없다.",
                     append_josa(fingers_or_gloves(FALSE), "이"),
                     append_josa(gloves_simple_name(otmp), "을"));
#endif
            err++;
        } else
            *mask = W_ARMG;
    } else if (is_shirt(otmp)) {
        if (uarm || uarmc || uarmu) {
            if (uarmu) {
                if (noisy)
#if 0 /*KR*/
                    already_wearing(an(c_shirt));
#else
                    already_wearing(c_shirt, uarmu);
#endif
            } else {
                if (noisy)
#if 0 /*KR:T*/
                    You_cant("wear that over your %s.",
                             (uarm && !uarmc) ? c_armor
                                              : cloak_simple_name(uarmc));
#else
                    You("%s 위로 그것을 입을 수 없다.",
                        (uarm && !uarmc) ? c_armor
                                         : cloak_simple_name(uarmc));
#endif
            }
            err++;
        } else
            *mask = W_ARMU;
    } else if (is_cloak(otmp)) {
        if (uarmc) {
            if (noisy)
#if 0 /*KR:T*/
                already_wearing(an(cloak_simple_name(uarmc)));
#else
                already_wearing(cloak_simple_name(uarmc), otmp);
#endif
            err++;
        } else
            *mask = W_ARMC;
    } else if (is_suit(otmp)) {
        if (uarmc) {
            if (noisy)
                /*KR You("cannot wear armor over a %s.",
                 * cloak_simple_name(uarmc)); */
                You("%s 위에 갑옷을 입을 수 없다.", cloak_simple_name(uarmc));
            err++;
        } else if (uarm) {
            if (noisy)
#if 0 /*KR: 원본*/
                already_wearing("some armor");
#else /*KR: KRNethack 맞춤 번역 */
                already_wearing("갑옷", uarm);
#endif
            err++;
        } else
            *mask = W_ARM;
    } else {
        /* getobj can't do this after setting its allow_all flag; that
           happens if you have armor for slots that are covered up or
           extra armor for slots that are filled */
        if (noisy)
            /*KR silly_thing("wear", otmp); */
            silly_thing("장비하기에는", otmp);
        err++;
    }
    /* Unnecessary since now only weapons and special items like pick-axes get
     * welded to your hand, not armor
        if (welded(otmp)) {
            if (!err++) {
                if (noisy) weldmsg(otmp);
            }
        }
     */
    return !err;
}

STATIC_OVL int
accessory_or_armor_on(obj)
struct obj *obj;
{
    long mask = 0L;
    boolean armor, ring, eyewear;

    if (obj->owornmask & (W_ACCESSORY | W_ARMOR)) {
#if 0 /*KR*/
        already_wearing(c_that_);
#else
        already_wearing(c_that_, obj);
#endif
        return 0;
    }
    armor = (obj->oclass == ARMOR_CLASS);
    ring = (obj->oclass == RING_CLASS || obj->otyp == MEAT_RING);
    eyewear = (obj->otyp == BLINDFOLD || obj->otyp == TOWEL
               || obj->otyp == LENSES);
    /* checks which are performed prior to actually touching the item */
    if (armor) {
        if (!canwearobj(obj, &mask, TRUE))
            return 0;

        if (obj->otyp == HELM_OF_OPPOSITE_ALIGNMENT
            && qstart_level.dnum == u.uz.dnum) { /* in quest */
            if (u.ualignbase[A_CURRENT] == u.ualignbase[A_ORIGINAL])
           /*KR You("narrowly avoid losing all chance at your goal."); */
                You("가까스로 목표를 이룰 기회를 잃는 것을 피했다.");
            else /* converted */
           /*KR You("are suddenly overcome with shame and change your
                 * mind."); */
                You("갑자기 수치심이 밀려와 마음을 바꿨다.");
            u.ublessed = 0; /* lose your god's protection */
            makeknown(obj->otyp);
            context.botl = 1; /*for AC after zeroing u.ublessed */
            return 1;
        }
    } else {
        /* accessory */
        if (ring) {
            char answer, qbuf[QBUFSZ];
            int res = 0;

            if (nolimbs(youmonst.data)) {
                /*KR You("cannot make the ring stick to your body."); */
                You("반지를 몸에 고정할 수 없다.");
                return 0;
            }
            if (uleft && uright) {
#if 0 /*KR: 원본*/
                There("are no more %s%s to fill.",
                      humanoid(youmonst.data) ? "ring-" : "",
                      fingers_or_gloves(FALSE));
#else /*KR: KRNethack 맞춤 번역 */
                pline("더 이상 반지를 낄 %s 없다.",
                      append_josa(fingers_or_gloves(FALSE), "이"));
#endif
                return 0;
            }
            if (uleft) {
                mask = RIGHT_RING;
            } else if (uright) {
                mask = LEFT_RING;
            } else {
                do {
#if 0 /*KR: 원본*/
                    Sprintf(qbuf, "Which %s%s, Right or Left?",
                            humanoid(youmonst.data) ? "ring-" : "",
                            body_part(FINGER));
#else /*KR: KRNethack 맞춤 번역 */
                    Sprintf(qbuf,
                            "어느 쪽 %s에 끼겠습니까, 오른쪽 아니면 왼쪽?",
                            body_part(FINGER));
#endif
                    answer = yn_function(qbuf, "rl", '\0');
                    switch (answer) {
                    case '\0':
                        return 0;
                    case 'l':
                    case 'L':
                        mask = LEFT_RING;
                        break;
                    case 'r':
                    case 'R':
                        mask = RIGHT_RING;
                        break;
                    }
                } while (!mask);
            }
            if (uarmg && Glib) {
#if 0 /*KR: 원본*/
                Your("%s are too slippery to remove, so you cannot put on the ring.",
                     gloves_simple_name(uarmg));
#else /*KR: KRNethack 맞춤 번역 */
                Your("%s 너무 미끄러워서 벗을 수 없기 때문에, 반지를 낄 수 "
                     "없다.",
                     append_josa(gloves_simple_name(uarmg), "이"));
#endif
                return 1; /* always uses move */
            }
            if (uarmg && uarmg->cursed) {
                res = !uarmg->bknown;
                set_bknown(uarmg, 1);
                /*KR You("cannot remove your %s to put on the ring.",
                 * c_gloves); */
                You("반지를 끼기 위해 %s 벗을 수 없다.",
                    append_josa(c_gloves, "을"));
                return res; /* uses move iff we learned gloves are cursed */
            }
            if (uwep) {
                res = !uwep->bknown; /* check this before calling welded() */
                if ((mask == RIGHT_RING || bimanual(uwep)) && welded(uwep)) {
                    const char *hand = body_part(HAND);

                    /* welded will set bknown */
                    if (bimanual(uwep))
                        hand = makeplural(hand);
#if 0 /*KR: 원본*/
                    You("cannot free your weapon %s to put on the ring.", hand);
#else /*KR: KRNethack 맞춤 번역 */
                    You("반지를 끼기 위해 무기를 쥔 %s 자유롭게 할 수 없다.",
                        append_josa(hand, "을"));
#endif
                    return res; /* uses move iff we learned weapon is cursed
                                 */
                }
            }
        } else if (obj->oclass == AMULET_CLASS) {
            if (uamul) {
#if 0 /*KR: 원본*/
                already_wearing("an amulet");
#else /*KR: KRNethack 맞춤 번역 */
                already_wearing("부적", uamul);
#endif
                return 0;
            }
        } else if (eyewear) {
            if (ublindf) {
                if (ublindf->otyp == TOWEL)
#if 0 /*KR: 원본*/
                    Your("%s is already covered by a towel.", body_part(FACE));
#else /*KR: KRNethack 맞춤 번역 */
                    Your("%s 이미 수건으로 덮여 있다.",
                         append_josa(body_part(FACE), "은"));
#endif
                else if (ublindf->otyp == BLINDFOLD) {
                    if (obj->otyp == LENSES)
                        /*KR already_wearing2("lenses", "a blindfold"); */
                        already_wearing2("렌즈", "눈가리개");
                    else
#if 0 /*KR: 원본*/
                        already_wearing("a blindfold");
#else /*KR: KRNethack 맞춤 번역 */
                        already_wearing("눈가리개", ublindf);
#endif
                } else if (ublindf->otyp == LENSES) {
                    if (obj->otyp == BLINDFOLD)
                        /*KR already_wearing2("a blindfold", "some lenses");
                         */
                        already_wearing2("눈가리개", "렌즈");
                    else
#if 0 /*KR: 원본*/
                        already_wearing("some lenses");
#else /*KR: KRNethack 맞춤 번역 */
                        already_wearing("렌즈", ublindf);
#endif
                } else {
#if 0 /*KR: 원본*/
                    already_wearing(something); /* ??? */
#else /*KR: KRNethack 맞춤 번역 */
                    already_wearing("무언가", ublindf); /* ??? */
#endif
                }
                return 0;
            }
        } else {
            /* neither armor nor accessory */
            /*KR You_cant("wear that!"); */
            You_cant("그것을 장비할 수 없다!");
            return 0;
        }
    }

    if (!retouch_object(&obj, FALSE))
        return 1; /* costs a turn even though it didn't get worn */

    if (armor) {
        int delay;

        /* if the armor is wielded, release it for wearing (won't be
           welded even if cursed; that only happens for weapons/weptools) */
        if (obj->owornmask & W_WEAPONS)
            remove_worn_item(obj, FALSE);
        /*
         * Setting obj->known=1 is done because setworn() causes hero's AC
         * to change so armor's +/- value is evident via the status line.
         * We used to set it here because of that, but then it would stick
         * if a nymph stole the armor before it was fully worn.  Delay it
         * until the aftermv action.  The player may still know this armor's
         * +/- amount if donning gets interrupted, but the hero won't.
         *
         * obj->known = 1;
         */
        setworn(obj, mask);
        /* if there's no delay, we'll execute 'aftermv' immediately */
        if (obj == uarm)
            afternmv = Armor_on;
        else if (obj == uarmh)
            afternmv = Helmet_on;
        else if (obj == uarmg)
            afternmv = Gloves_on;
        else if (obj == uarmf)
            afternmv = Boots_on;
        else if (obj == uarms)
            afternmv = Shield_on;
        else if (obj == uarmc)
            afternmv = Cloak_on;
        else if (obj == uarmu)
            afternmv = Shirt_on;
        else
            panic("wearing armor not worn as armor? [%08lx]", obj->owornmask);

        delay = -objects[obj->otyp].oc_delay;
        if (delay) {
            nomul(delay);
            /*KR multi_reason = "dressing up"; */
            multi_reason = "장비하느라";
            /*KR nomovemsg = "You finish your dressing maneuver."; */
            nomovemsg = "장비하기를 마쳤다.";
        } else {
            unmul(""); /* call (*aftermv)(), clear it+nomovemsg+multi_reason */
            on_msg(obj);
        }
        context.takeoff.mask = context.takeoff.what = 0L;
    } else { /* not armor */
        boolean give_feedback = FALSE;

        /* [releasing wielded accessory handled in Xxx_on()] */
        if (ring) {
            setworn(obj, mask);
            Ring_on(obj);
            give_feedback = TRUE;
        } else if (obj->oclass == AMULET_CLASS) {
            setworn(obj, W_AMUL);
            Amulet_on();
            /* no feedback here if amulet of change got used up */
            give_feedback = (uamul != 0);
        } else if (eyewear) {
            /* setworn() handled by Blindf_on() */
            Blindf_on(obj);
            /* message handled by Blindf_on(); leave give_feedback False */
        }
        /* feedback for ring or for amulet other than 'change' */
        if (give_feedback && is_worn(obj))
            prinv((char *) 0, obj, 0L);
    }
    return 1;
}

/* the 'W' command */
int
dowear()
{
    struct obj *otmp;

    /* cantweararm() checks for suits of armor, not what we want here;
       verysmall() or nohands() checks for shields, gloves, etc... */
    if (verysmall(youmonst.data) || nohands(youmonst.data)) {
        /*KR pline("Don't even bother."); */
        pline("애쓸 필요 없다.");
        return 0;
    }
    if (uarm && uarmu && uarmc && uarmh && uarms && uarmg && uarmf && uleft
        && uright && uamul && ublindf) {
        /* 'W' message doesn't mention accessories */
        /*KR You("are already wearing a full complement of armor."); */
        You("이미 장비할 수 있는 모든 방어구를 갖추고 있다.");
        return 0;
    }
    otmp = getobj(clothes, "wear");
    return otmp ? accessory_or_armor_on(otmp) : 0;
}

/* the 'P' command */
int
doputon()
{
    struct obj *otmp;

    if (uleft && uright && uamul && ublindf && uarm && uarmu && uarmc && uarmh
        && uarms && uarmg && uarmf) {
        /* 'P' message doesn't mention armor */
#if 0 /*KR: 원본*/
        Your("%s%s are full, and you're already wearing an amulet and %s.",
             humanoid(youmonst.data) ? "ring-" : "",
             fingers_or_gloves(FALSE),
             (ublindf->otyp == LENSES) ? "some lenses" : "a blindfold");
#else /*KR: KRNethack 맞춤 번역 */
        pline(
            "손가락에는 이미 반지가 가득하고, 이미 부적과 %s 장비하고 있다.",
            append_josa((ublindf->otyp == LENSES) ? "렌즈" : "눈가리개",
                        "를"));
#endif
        return 0;
    }
    otmp = getobj(accessories, "put on");
    return otmp ? accessory_or_armor_on(otmp) : 0;
}

/* calculate current armor class */
void
find_ac()
{
    int uac = mons[u.umonnum].ac; /* base armor class for current form */

    /* armor class from worn gear */
    if (uarm)
        uac -= ARM_BONUS(uarm);
    if (uarmc)
        uac -= ARM_BONUS(uarmc);
    if (uarmh)
        uac -= ARM_BONUS(uarmh);
    if (uarmf)
        uac -= ARM_BONUS(uarmf);
    if (uarms)
        uac -= ARM_BONUS(uarms);
    if (uarmg)
        uac -= ARM_BONUS(uarmg);
    if (uarmu)
        uac -= ARM_BONUS(uarmu);
    if (uleft && uleft->otyp == RIN_PROTECTION)
        uac -= uleft->spe;
    if (uright && uright->otyp == RIN_PROTECTION)
        uac -= uright->spe;

    /* armor class from other sources */
    if (HProtection & INTRINSIC)
        uac -= u.ublessed;
    uac -= u.uspellprot;

    /* [The magic binary numbers 127 and -128 should be replaced with the
     * mystic decimal numbers 99 and -99 which require no explanation to
     * the uninitiated and would cap the width of a status line value at
     * one less character.]
     */
    if (uac < -128)
        uac = -128; /* u.uac is an schar */
    else if (uac > 127)
        uac = 127; /* for completeness */

    if (uac != u.uac) {
        u.uac = uac;
        context.botl = 1;
    }
}

void
glibr()
{
    register struct obj *otmp;
    int xfl = 0;
    boolean leftfall, rightfall, wastwoweap = FALSE;
    const char *otherwep = 0, *thiswep, *which, *hand;

    leftfall = (uleft && !uleft->cursed
                && (!uwep || !welded(uwep) || !bimanual(uwep)));
    rightfall = (uright && !uright->cursed && (!welded(uwep)));
    if (!uarmg && (leftfall || rightfall) && !nolimbs(youmonst.data)) {
        /* changed so cursed rings don't fall off, GAN 10/30/86 */
#if 0 /*KR: 원본*/
        Your("%s off your %s.",
             (leftfall && rightfall) ? "rings slip" : "ring slips",
             (leftfall && rightfall) ? fingers_or_gloves(FALSE)
                                     : body_part(FINGER));
#else /*KR: KRNethack 맞춤 번역 */
        Your("%s 당신의 %s 빠져나갔다.",
             (leftfall && rightfall) ? "반지들이" : "반지가",
             append_josa((leftfall && rightfall) ? fingers_or_gloves(FALSE)
                                                 : body_part(FINGER),
                         "에서"));
#endif
        xfl++;
        if (leftfall) {
            otmp = uleft;
            Ring_off(uleft);
            dropx(otmp);
        }
        if (rightfall) {
            otmp = uright;
            Ring_off(uright);
            dropx(otmp);
        }
    }

    otmp = uswapwep;
    if (u.twoweap && otmp) {
        /* secondary weapon doesn't need nearly as much handling as
           primary; when in two-weapon mode, we know it's one-handed
           with something else in the other hand and also that it's
           a weapon or weptool rather than something unusual, plus
           we don't need to compare its type with the primary */
        otherwep = is_sword(otmp) ? c_sword : weapon_descr(otmp);
        if (otmp->quan > 1L)
            otherwep = makeplural(otherwep);
        hand = body_part(HAND);
        /*KR which = "left "; */
        which = "왼쪽 ";
#if 0 /*KR: 원본*/
        Your("%s %s%s from your %s%s.", otherwep, xfl ? "also " : "",
             otense(otmp, "slip"), which, hand);
#else /*KR: KRNethack 맞춤 번역 */
        Your("%s %s당신의 %s%s에서 미끄러졌다.", append_josa(otherwep, "도"),
             xfl ? "역시 " : "", which, hand);
#endif
        xfl++;
        wastwoweap = TRUE;
        setuswapwep((struct obj *) 0); /* clears u.twoweap */
        if (canletgo(otmp, ""))
            dropx(otmp);
    }
    otmp = uwep;
    if (otmp && !welded(otmp)) {
        long savequan = otmp->quan;

        /* nice wording if both weapons are the same type */
        thiswep = is_sword(otmp) ? c_sword : weapon_descr(otmp);
        if (otherwep && strcmp(thiswep, makesingular(otherwep)))
            otherwep = 0;
        if (otmp->quan > 1L) {
            /* most class names for unconventional wielded items
               are ok, but if wielding multiple apples or rations
               we don't want "your foods slip", so force non-corpse
               food to be singular; skipping makeplural() isn't
               enough--we need to fool otense() too */
            if (!strcmp(thiswep, "food"))
                otmp->quan = 1L;
            else
                thiswep = makeplural(thiswep);
        }
        hand = body_part(HAND);
        which = "";
        if (bimanual(otmp))
            hand = makeplural(hand);
        else if (wastwoweap)
            /*KR which = "right "; */ /* preceding msg was about left */
            which = "오른쪽 ";
#if 0 /*KR: 원본*/
        pline("%s %s%s %s%s from your %s%s.",
              !strncmp(thiswep, "corpse", 6) ? "The" : "Your",
              otherwep ? "other " : "", thiswep, xfl ? "also " : "",
              otense(otmp, "slip"), which, hand);
#else /*KR: KRNethack 맞춤 번역 */
        pline("%s %s%s%s 당신의 %s%s에서 미끄러졌다.",
              !strncmp(thiswep, "corpse", 6) ? "그" : "당신의",
              otherwep ? "다른 " : "", append_josa(thiswep, "이"),
              xfl ? " 역시" : "", which, hand);
#endif
        /* xfl++; */
        otmp->quan = savequan;
        setuwep((struct obj *) 0);
        if (canletgo(otmp, ""))
            dropx(otmp);
    }
}

struct obj *
some_armor(victim)
struct monst *victim;
{
    register struct obj *otmph, *otmp;

    otmph = (victim == &youmonst) ? uarmc : which_armor(victim, W_ARMC);
    if (!otmph)
        otmph = (victim == &youmonst) ? uarm : which_armor(victim, W_ARM);
    if (!otmph)
        otmph = (victim == &youmonst) ? uarmu : which_armor(victim, W_ARMU);

    otmp = (victim == &youmonst) ? uarmh : which_armor(victim, W_ARMH);
    if (otmp && (!otmph || !rn2(4)))
        otmph = otmp;
    otmp = (victim == &youmonst) ? uarmg : which_armor(victim, W_ARMG);
    if (otmp && (!otmph || !rn2(4)))
        otmph = otmp;
    otmp = (victim == &youmonst) ? uarmf : which_armor(victim, W_ARMF);
    if (otmp && (!otmph || !rn2(4)))
        otmph = otmp;
    otmp = (victim == &youmonst) ? uarms : which_armor(victim, W_ARMS);
    if (otmp && (!otmph || !rn2(4)))
        otmph = otmp;
    return otmph;
}

/* used for praying to check and fix levitation trouble */
struct obj *
stuck_ring(ring, otyp)
struct obj *ring;
int otyp;
{
    if (ring != uleft && ring != uright) {
        impossible("stuck_ring: neither left nor right?");
        return (struct obj *) 0;
    }

    if (ring && ring->otyp == otyp) {
        /* reasons ring can't be removed match those checked by select_off();
           limbless case has extra checks because ordinarily it's temporary */
        if (nolimbs(youmonst.data) && uamul
            && uamul->otyp == AMULET_OF_UNCHANGING && uamul->cursed)
            return uamul;
        if (welded(uwep) && (ring == uright || bimanual(uwep)))
            return uwep;
        if (uarmg && uarmg->cursed)
            return uarmg;
        if (ring->cursed)
            return ring;
        /* normally outermost layer is processed first, but slippery gloves
           wears off quickly so uncurse ring itself before handling those */
        if (uarmg && Glib)
            return uarmg;
    }
    /* either no ring or not right type or nothing prevents its removal */
    return (struct obj *) 0;
}

/* also for praying; find worn item that confers "Unchanging" attribute */
struct obj *
unchanger()
{
    if (uamul && uamul->otyp == AMULET_OF_UNCHANGING)
        return uamul;
    return 0;
}

STATIC_PTR
int
select_off(otmp)
register struct obj *otmp;
{
    struct obj *why;
    char buf[BUFSZ];

    if (!otmp)
        return 0;
    *buf = '\0'; /* lint suppression */

    /* special ring checks */
    if (otmp == uright || otmp == uleft) {
        struct obj glibdummy;

        if (nolimbs(youmonst.data)) {
            /*KR pline_The("ring is stuck."); */
            pline("반지가 끼어 있다.");
            return 0;
        }
        glibdummy = zeroobj;
        why = 0; /* the item which prevents ring removal */
#if 0            /*KR: 원본*/
        if (welded(uwep) && (otmp == uright || bimanual(uwep))) {
            Sprintf(buf, "free a weapon %s", body_part(HAND));
            why = uwep;
        } else if (uarmg && (uarmg->cursed || Glib)) {
            Sprintf(buf, "take off your %s%s",
                    Glib ? "slippery " : "", gloves_simple_name(uarmg));
            why = !Glib ? uarmg : &glibdummy;
        }
        if (why) {
            You("cannot %s to remove the ring.", buf);
            set_bknown(why, 1);
            return 0;
        }
#else            /*KR: KRNethack 맞춤 번역 */
        if (welded(uwep) && (otmp == uright || bimanual(uwep))) {
            Sprintf(buf, "무기를 쥔 %s 자유롭게 할",
                    append_josa(body_part(HAND), "을"));
            why = uwep;
        } else if (uarmg && (uarmg->cursed || Glib)) {
            Sprintf(buf, "%s%s 벗을", Glib ? "미끄러운 " : "",
                    append_josa(gloves_simple_name(uarmg), "을"));
            why = !Glib ? uarmg : &glibdummy;
        }
        if (why) {
            You("반지를 빼기 위해 %s 수 없다.", buf);
            set_bknown(why, 1);
            return 0;
        }
#endif
    }
    /* special glove checks */
    if (otmp == uarmg) {
        if (welded(uwep)) {
#if 0 /*KR: 원본*/
            You("are unable to take off your %s while wielding that %s.",
                c_gloves, is_sword(uwep) ? c_sword : c_weapon);
#else /*KR: KRNethack 맞춤 번역 */
            You("%s 쥐고 있는 동안에는 %s 벗을 수 없다.",
                append_josa(is_sword(uwep) ? c_sword : c_weapon, "을"),
                append_josa(c_gloves, "을"));
#endif
            set_bknown(uwep, 1);
            return 0;
        } else if (Glib) {
#if 0 /*KR: 원본*/
            pline("%s %s are too slippery to take off.",
                  uarmg->unpaid ? "The" : "Your", /* simplified Shk_Your() */
                  gloves_simple_name(uarmg));
#else /*KR: KRNethack 맞춤 번역 */
            pline("%s 너무 미끄러워서 벗을 수 없다.",
                  append_josa(gloves_simple_name(uarmg), "은"));
#endif
            return 0;
        }
    }
    /* special boot checks */
    if (otmp == uarmf) {
        if (u.utrap && u.utraptype == TT_BEARTRAP) {
            /*KR pline_The("bear trap prevents you from pulling your %s out.",
             * body_part(FOOT)); */
            pline("곰덫 때문에 %s 빼낼 수 없다.",
                  append_josa(body_part(FOOT), "을"));
            return 0;
        } else if (u.utrap && u.utraptype == TT_INFLOOR) {
            /*KR You("are stuck in the %s, and cannot pull your %s out.",
             * surface(u.ux, u.uy), makeplural(body_part(FOOT))); */
            You("%s에 갇혀 있어서, %s 빼낼 수 없다.", surface(u.ux, u.uy),
                append_josa(makeplural(body_part(FOOT)), "을"));
            return 0;
        }
    }
    /* special suit and shirt checks */
    if (otmp == uarm || otmp == uarmu) {
        why = 0; /* the item which prevents disrobing */
#if 0            /*KR: 원본*/
        if (uarmc && uarmc->cursed) {
            Sprintf(buf, "remove your %s", cloak_simple_name(uarmc));
            why = uarmc;
        } else if (otmp == uarmu && uarm && uarm->cursed) {
            Sprintf(buf, "remove your %s", c_suit);
            why = uarm;
        } else if (welded(uwep) && bimanual(uwep)) {
            Sprintf(buf, "release your %s",
                    is_sword(uwep) ? c_sword : (uwep->otyp == BATTLE_AXE)
                                                   ? c_axe
                                                   : c_weapon);
            why = uwep;
        }
        if (why) {
            You("cannot %s to take off %s.", buf, the(xname(otmp)));
            set_bknown(why, 1);
            return 0;
        }
#else            /*KR: KRNethack 맞춤 번역 */
        if (uarmc && uarmc->cursed) {
            Sprintf(buf, "%s 벗을",
                    append_josa(cloak_simple_name(uarmc), "을"));
            why = uarmc;
        } else if (otmp == uarmu && uarm && uarm->cursed) {
            Sprintf(buf, "%s 벗을", append_josa(c_suit, "을"));
            why = uarm;
        } else if (welded(uwep) && bimanual(uwep)) {
            Sprintf(buf, "%s 놓을",
                    append_josa(is_sword(uwep)               ? c_sword
                                : (uwep->otyp == BATTLE_AXE) ? c_axe
                                                             : c_weapon,
                                "을"));
            why = uwep;
        }
        if (why) {
            You("%s 벗기 위해 %s 수 없다.",
                append_josa(the(xname(otmp)), "을"), buf);
            set_bknown(why, 1);
            return 0;
        }
#endif
    }
    /* basic curse check */
    if (otmp == uquiver || (otmp == uswapwep && !u.twoweap)) {
        ; /* some items can be removed even when cursed */
    } else {
        /* otherwise, this is fundamental */
        if (cursed(otmp))
            return 0;
    }

    if (otmp == uarm)
        context.takeoff.mask |= WORN_ARMOR;
    else if (otmp == uarmc)
        context.takeoff.mask |= WORN_CLOAK;
    else if (otmp == uarmf)
        context.takeoff.mask |= WORN_BOOTS;
    else if (otmp == uarmg)
        context.takeoff.mask |= WORN_GLOVES;
    else if (otmp == uarmh)
        context.takeoff.mask |= WORN_HELMET;
    else if (otmp == uarms)
        context.takeoff.mask |= WORN_SHIELD;
    else if (otmp == uarmu)
        context.takeoff.mask |= WORN_SHIRT;
    else if (otmp == uleft)
        context.takeoff.mask |= LEFT_RING;
    else if (otmp == uright)
        context.takeoff.mask |= RIGHT_RING;
    else if (otmp == uamul)
        context.takeoff.mask |= WORN_AMUL;
    else if (otmp == ublindf)
        context.takeoff.mask |= WORN_BLINDF;
    else if (otmp == uwep)
        context.takeoff.mask |= W_WEP;
    else if (otmp == uswapwep)
        context.takeoff.mask |= W_SWAPWEP;
    else if (otmp == uquiver)
        context.takeoff.mask |= W_QUIVER;

    else
        impossible("select_off: %s???", doname(otmp));

    return 0;
}

STATIC_OVL struct obj *
do_takeoff()
{
    struct obj *otmp = (struct obj *) 0;
    struct takeoff_info *doff = &context.takeoff;

    context.takeoff.mask |= I_SPECIAL; /* set flag for cancel_doff() */
    if (doff->what == W_WEP) {
        if (!cursed(uwep)) {
            setuwep((struct obj *) 0);
            /*KR You("are empty %s.", body_part(HANDED)); */
            You("맨손이 되었다.");
            u.twoweap = FALSE;
        }
    } else if (doff->what == W_SWAPWEP) {
        setuswapwep((struct obj *) 0);
        /*KR You("no longer have a second weapon readied."); */
        You("더 이상 두 번째 무기를 준비하지 않는다.");
        u.twoweap = FALSE;
    } else if (doff->what == W_QUIVER) {
        setuqwep((struct obj *) 0);
        /*KR You("no longer have ammunition readied."); */
        You("더 이상 탄약을 장전하지 않는다.");
    } else if (doff->what == WORN_ARMOR) {
        otmp = uarm;
        if (!cursed(otmp))
            (void) Armor_off();
    } else if (doff->what == WORN_CLOAK) {
        otmp = uarmc;
        if (!cursed(otmp))
            (void) Cloak_off();
    } else if (doff->what == WORN_BOOTS) {
        otmp = uarmf;
        if (!cursed(otmp))
            (void) Boots_off();
    } else if (doff->what == WORN_GLOVES) {
        otmp = uarmg;
        if (!cursed(otmp))
            (void) Gloves_off();
    } else if (doff->what == WORN_HELMET) {
        otmp = uarmh;
        if (!cursed(otmp))
            (void) Helmet_off();
    } else if (doff->what == WORN_SHIELD) {
        otmp = uarms;
        if (!cursed(otmp))
            (void) Shield_off();
    } else if (doff->what == WORN_SHIRT) {
        otmp = uarmu;
        if (!cursed(otmp))
            (void) Shirt_off();
    } else if (doff->what == WORN_AMUL) {
        otmp = uamul;
        if (!cursed(otmp))
            Amulet_off();
    } else if (doff->what == LEFT_RING) {
        otmp = uleft;
        if (!cursed(otmp))
            Ring_off(uleft);
    } else if (doff->what == RIGHT_RING) {
        otmp = uright;
        if (!cursed(otmp))
            Ring_off(uright);
    } else if (doff->what == WORN_BLINDF) {
        if (!cursed(ublindf))
            Blindf_off(ublindf);
    } else {
        impossible("do_takeoff: taking off %lx", doff->what);
    }
    context.takeoff.mask &= ~I_SPECIAL; /* clear cancel_doff() flag */

    return otmp;
}

/* occupation callback for 'A' */
STATIC_PTR
int
take_off(VOID_ARGS)
{
    register int i;
    register struct obj *otmp;
    struct takeoff_info *doff = &context.takeoff;

    if (doff->what) {
        if (doff->delay > 0) {
            doff->delay--;
            return 1; /* still busy */
        }
        if ((otmp = do_takeoff()) != 0)
            off_msg(otmp);
        doff->mask &= ~doff->what;
        doff->what = 0L;
    }

    for (i = 0; takeoff_order[i]; i++)
        if (doff->mask & takeoff_order[i]) {
            doff->what = takeoff_order[i];
            break;
        }

    otmp = (struct obj *) 0;
    doff->delay = 0;

    if (doff->what == 0L) {
        /*KR You("finish %s.", doff->disrobing); */
        You("%s 마쳤다.", append_josa(doff->disrobing, "을"));
        return 0;
    } else if (doff->what == W_WEP) {
        doff->delay = 1;
    } else if (doff->what == W_SWAPWEP) {
        doff->delay = 1;
    } else if (doff->what == W_QUIVER) {
        doff->delay = 1;
    } else if (doff->what == WORN_ARMOR) {
        otmp = uarm;
        /* If a cloak is being worn, add the time to take it off and put
         * it back on again.  Kludge alert! since that time is 0 for all
         * known cloaks, add 1 so that it actually matters...
         */
        if (uarmc)
            doff->delay += 2 * objects[uarmc->otyp].oc_delay + 1;
    } else if (doff->what == WORN_CLOAK) {
        otmp = uarmc;
    } else if (doff->what == WORN_BOOTS) {
        otmp = uarmf;
    } else if (doff->what == WORN_GLOVES) {
        otmp = uarmg;
    } else if (doff->what == WORN_HELMET) {
        otmp = uarmh;
    } else if (doff->what == WORN_SHIELD) {
        otmp = uarms;
    } else if (doff->what == WORN_SHIRT) {
        otmp = uarmu;
        /* add the time to take off and put back on armor and/or cloak */
        if (uarm)
            doff->delay += 2 * objects[uarm->otyp].oc_delay;
        if (uarmc)
            doff->delay += 2 * objects[uarmc->otyp].oc_delay + 1;
    } else if (doff->what == WORN_AMUL) {
        doff->delay = 1;
    } else if (doff->what == LEFT_RING) {
        doff->delay = 1;
    } else if (doff->what == RIGHT_RING) {
        doff->delay = 1;
    } else if (doff->what == WORN_BLINDF) {
        /* [this used to be 2, but 'R' (and 'T') only require 1 turn to
           remove a blindfold, so 'A' shouldn't have been requiring 2] */
        doff->delay = 1;
    } else {
        impossible("take_off: taking off %lx", doff->what);
        return 0; /* force done */
    }

    if (otmp)
        doff->delay += objects[otmp->otyp].oc_delay;

    /* Since setting the occupation now starts the counter next move, that
     * would always produce a delay 1 too big per item unless we subtract
     * 1 here to account for it.
     */
    if (doff->delay > 0)
        doff->delay--;

    set_occupation(take_off, doff->disrobing, 0);
    return 1; /* get busy */
}

/* clear saved context to avoid inappropriate resumption of interrupted 'A' */
void
reset_remarm()
{
    context.takeoff.what = context.takeoff.mask = 0L;
    context.takeoff.disrobing[0] = '\0';
}

/* the 'A' command -- remove multiple worn items */
int
doddoremarm()
{
    int result = 0;

    if (context.takeoff.what || context.takeoff.mask) {
        /*KR You("continue %s.", context.takeoff.disrobing); */
        You("계속해서 %s 한다.",
            append_josa(context.takeoff.disrobing, "을"));
        set_occupation(take_off, context.takeoff.disrobing, 0);
        return 0;
    } else if (!uwep && !uswapwep && !uquiver && !uamul && !ublindf && !uleft
               && !uright && !wearing_armor()) {
        /*KR You("are not wearing anything."); */
        You("아무것도 장비하고 있지 않다.");
        return 0;
    }

    add_valid_menu_class(0); /* reset */
    if (flags.menu_style != MENU_TRADITIONAL
        || (result =
                ggetobj("take off", select_off, 0, FALSE, (unsigned *) 0))
               < -1)
        result = menu_remarm(result);

    if (context.takeoff.mask) {
        /* default activity for armor and/or accessories,
           possibly combined with weapons */
#if 0 /*KR: 원본*/
        (void) strncpy(context.takeoff.disrobing, "disrobing", CONTEXTVERBSZ);
        /* specific activity when handling weapons only */
        if (!(context.takeoff.mask & ~W_WEAPONS))
            (void) strncpy(context.takeoff.disrobing, "disarming",
                           CONTEXTVERBSZ);
#else /*KR: KRNethack 맞춤 번역 */
        (void) strncpy(context.takeoff.disrobing, "옷 벗기", CONTEXTVERBSZ);
        /* specific activity when handling weapons only */
        if (!(context.takeoff.mask & ~W_WEAPONS))
            (void) strncpy(context.takeoff.disrobing, "무기 해제",
                           CONTEXTVERBSZ);
#endif
        (void) take_off();
    }
    /* The time to perform the command is already completely accounted for
     * in take_off(); if we return 1, that would add an extra turn to each
     * disrobe.
     */
    return 0;
}

STATIC_OVL int
menu_remarm(retry)
int retry;
{
    int n, i = 0;
    menu_item *pick_list;
    boolean all_worn_categories = TRUE;

    if (retry) {
        all_worn_categories = (retry == -2);
    } else if (flags.menu_style == MENU_FULL) {
        all_worn_categories = FALSE;
#if 0 /*KR: 원본*/
        n = query_category("What type of things do you want to take off?",
                           invent, (WORN_TYPES | ALL_TYPES
                                    | UNPAID_TYPES | BUCX_TYPES),
                           &pick_list, PICK_ANY);
#else /*KR: KRNethack 맞춤 번역 */
        n = query_category(
            "어떤 종류의 물건을 벗거나 빼겠습니까?", invent,
            (WORN_TYPES | ALL_TYPES | UNPAID_TYPES | BUCX_TYPES), &pick_list,
            PICK_ANY);
#endif
        if (!n)
            return 0;
        for (i = 0; i < n; i++) {
            if (pick_list[i].item.a_int == ALL_TYPES_SELECTED)
                all_worn_categories = TRUE;
            else
                add_valid_menu_class(pick_list[i].item.a_int);
        }
        free((genericptr_t) pick_list);
    } else if (flags.menu_style == MENU_COMBINATION) {
        unsigned ggofeedback = 0;

        i = ggetobj("take off", select_off, 0, TRUE, &ggofeedback);
        if (ggofeedback & ALL_FINISHED)
            return 0;
        all_worn_categories = (i == -2);
    }
    if (menu_class_present('u') || menu_class_present('B')
        || menu_class_present('U') || menu_class_present('C')
        || menu_class_present('X'))
        all_worn_categories = FALSE;

#if 0 /*KR: 원본*/
    n = query_objlist("What do you want to take off?", &invent,
                      (SIGNAL_NOMENU | USE_INVLET | INVORDER_SORT),
                      &pick_list, PICK_ANY,
                      all_worn_categories ? is_worn : is_worn_by_type);
#else /*KR: KRNethack 맞춤 번역 */
    n = query_objlist("무엇을 벗거나 빼겠습니까?", &invent,
                      (SIGNAL_NOMENU | USE_INVLET | INVORDER_SORT),
                      &pick_list, PICK_ANY,
                      all_worn_categories ? is_worn : is_worn_by_type);
#endif
    if (n > 0) {
        for (i = 0; i < n; i++)
            (void) select_off(pick_list[i].item.a_obj);
        free((genericptr_t) pick_list);
    } else if (n < 0 && flags.menu_style != MENU_COMBINATION) {
        /*KR There("is nothing else you can remove or unwield."); */
        pline("더 이상 벗거나 해제할 수 있는 물건이 없다.");
    }
    return 0;
}

/* hit by destroy armor scroll/black dragon breath/monster spell */
int
destroy_arm(atmp)
register struct obj *atmp;
{
    register struct obj *otmp;
#define DESTROY_ARM(o)                            \
    ((otmp = (o)) != 0 && (!atmp || atmp == otmp) \
             && (!obj_resists(otmp, 0, 90))       \
         ? (otmp->in_use = TRUE)                  \
         : FALSE)

    if (DESTROY_ARM(uarmc)) {
        if (donning(otmp))
            cancel_don();
        /*KR Your("%s crumbles and turns to dust!", cloak_simple_name(uarmc));
         */
        Your("%s 부스러지더니 먼지가 되어버렸다!",
             append_josa(cloak_simple_name(uarmc), "이"));
        (void) Cloak_off();
        useup(otmp);
    } else if (DESTROY_ARM(uarm)) {
        if (donning(otmp))
            cancel_don();
        /*KR Your("armor turns to dust and falls to the %s!", surface(u.ux,
         * u.uy)); */
        Your("갑옷이 먼지가 되어 %s 떨어졌다!",
             append_josa(surface(u.ux, u.uy), "로"));
        (void) Armor_gone();
        useup(otmp);
    } else if (DESTROY_ARM(uarmu)) {
        if (donning(otmp))
            cancel_don();
        /*KR Your("shirt crumbles into tiny threads and falls apart!"); */
        Your("셔츠가 작은 실오라기들로 부스러지며 분해되었다!");
        (void) Shirt_off();
        useup(otmp);
    } else if (DESTROY_ARM(uarmh)) {
        if (donning(otmp))
            cancel_don();
        /*KR Your("%s turns to dust and is blown away!",
         * helm_simple_name(uarmh)); */
        Your("%s 먼지가 되어 날아갔다!",
             append_josa(helm_simple_name(uarmh), "이"));
        (void) Helmet_off();
        useup(otmp);
    } else if (DESTROY_ARM(uarmg)) {
        if (donning(otmp))
            cancel_don();
        /*KR Your("gloves vanish!"); */
        Your("장갑이 사라졌다!");
        (void) Gloves_off();
        useup(otmp);
        selftouch("당신은");
    } else if (DESTROY_ARM(uarmf)) {
        if (donning(otmp))
            cancel_don();
        /*KR Your("boots disintegrate!"); */
        Your("신발이 붕괴되었다!");
        (void) Boots_off();
        useup(otmp);
    } else if (DESTROY_ARM(uarms)) {
        if (donning(otmp))
            cancel_don();
        /*KR Your("shield crumbles away!"); */
        Your("방패가 부스러져 버렸다!");
        (void) Shield_off();
        useup(otmp);
    } else {
        return 0; /* could not destroy anything */
    }

#undef DESTROY_ARM
    stop_occupation();
    return 1;
}

void adj_abon(otmp, delta) register struct obj *otmp;
register schar delta;
{
    if (uarmg && uarmg == otmp && otmp->otyp == GAUNTLETS_OF_DEXTERITY) {
        if (delta) {
            makeknown(uarmg->otyp);
            ABON(A_DEX) += (delta);
        }
        context.botl = 1;
    }
    if (uarmh && uarmh == otmp && otmp->otyp == HELM_OF_BRILLIANCE) {
        if (delta) {
            makeknown(uarmh->otyp);
            ABON(A_INT) += (delta);
            ABON(A_WIS) += (delta);
        }
        context.botl = 1;
    }
}

/* decide whether a worn item is covered up by some other worn item,
   used for dipping into liquid and applying grease;
   some criteria are different than select_off()'s */
boolean
inaccessible_equipment(obj, verb, only_if_known_cursed)
struct obj *obj;
const char *verb; /* "dip" or "grease", or null to avoid messages */
boolean only_if_known_cursed; /* ignore covering unless known to be cursed */
{
#if 0 /*KR: 원본*/
    static NEARDATA const char need_to_take_off_outer_armor[] =
        "need to take off %s to %s %s.";
#else /*KR: KRNethack 맞춤 번역 */
    static NEARDATA const char need_to_take_off_outer_armor[] =
        "%s %s 위해서는 %s 벗어야 한다.";
#endif
    char buf[BUFSZ];
    boolean anycovering = !only_if_known_cursed; /* more comprehensible... */
#define BLOCKSACCESS(x) (anycovering || ((x)->cursed && (x)->bknown))

    if (!obj || !obj->owornmask)
        return FALSE; /* not inaccessible */

    /* check for suit covered by cloak */
    if (obj == uarm && uarmc && BLOCKSACCESS(uarmc)) {
        if (verb) {
            Strcpy(buf, yname(uarmc));
            /*KR You(need_to_take_off_outer_armor, buf, verb, yname(obj)); */
            You(need_to_take_off_outer_armor, append_josa(yname(obj), "을"),
                verb, append_josa(buf, "을"));
        }
        return TRUE;
    }
    /* check for shirt covered by suit and/or cloak */
    if (obj == uarmu
        && ((uarm && BLOCKSACCESS(uarm)) || (uarmc && BLOCKSACCESS(uarmc)))) {
        if (verb) {
            char cloaktmp[QBUFSZ], suittmp[QBUFSZ];
            /* if sameprefix, use yname and xname to get "your cloak and suit"
               or "Manlobbi's cloak and suit"; otherwise, use yname and yname
               to get "your cloak and Manlobbi's suit" or vice versa */
            boolean sameprefix = (uarm && uarmc
                                  && !strcmp(shk_your(cloaktmp, uarmc),
                                             shk_your(suittmp, uarm)));

            *buf = '\0';
            if (uarmc)
                Strcat(buf, yname(uarmc));
            if (uarm && uarmc)
                /*KR Strcat(buf, " and "); */
                Strcat(buf, " 그리고 ");
            if (uarm)
                Strcat(buf, sameprefix ? xname(uarm) : yname(uarm));
#if 0 /*KR:T*/
            You(need_to_take_off_outer_armor, buf, verb, yname(obj));
#else
            You(need_to_take_off_outer_armor, append_josa(yname(obj), "을"),
                verb, append_josa(buf, "을"));
#endif
        }
        return TRUE;
    }
    /* check for ring covered by gloves */
    if ((obj == uleft || obj == uright) && uarmg && BLOCKSACCESS(uarmg)) {
        if (verb) {
            Strcpy(buf, yname(uarmg));
#if 0 /*KR:T*/
            You(need_to_take_off_outer_armor, buf, verb, yname(obj));
#else
            You(need_to_take_off_outer_armor, append_josa(yname(obj), "을"),
                verb, append_josa(buf, "을"));
#endif
        }
        return TRUE;
    }
    /* item is not inaccessible */
    return FALSE;
}

/*do_wear.c*/
