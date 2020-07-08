/* NetHack 3.6	apply.c	$NHDT-Date: 1573778560 2019/11/15 00:42:40 $  $NHDT-Branch: NetHack-3.6 $:$NHDT-Revision: 1.284 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2012. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

extern boolean notonhead; /* for long worms */

STATIC_DCL int FDECL(use_camera, (struct obj *));
STATIC_DCL int FDECL(use_towel, (struct obj *));
STATIC_DCL boolean FDECL(its_dead, (int, int, int *));
STATIC_DCL int FDECL(use_stethoscope, (struct obj *));
STATIC_DCL void FDECL(use_whistle, (struct obj *));
STATIC_DCL void FDECL(use_magic_whistle, (struct obj *));
STATIC_DCL int FDECL(use_leash, (struct obj *));
STATIC_DCL int FDECL(use_mirror, (struct obj *));
STATIC_DCL void FDECL(use_bell, (struct obj **));
STATIC_DCL void FDECL(use_candelabrum, (struct obj *));
STATIC_DCL void FDECL(use_candle, (struct obj **));
STATIC_DCL void FDECL(use_lamp, (struct obj *));
STATIC_DCL void FDECL(light_cocktail, (struct obj **));
STATIC_PTR void FDECL(display_jump_positions, (int));
STATIC_DCL void FDECL(use_tinning_kit, (struct obj *));
STATIC_DCL void FDECL(use_figurine, (struct obj **));
STATIC_DCL void FDECL(use_grease, (struct obj *));
STATIC_DCL void FDECL(use_trap, (struct obj *));
STATIC_DCL void FDECL(use_stone, (struct obj *));
STATIC_PTR int NDECL(set_trap); /* occupation callback */
STATIC_DCL int FDECL(use_whip, (struct obj *));
STATIC_PTR void FDECL(display_polearm_positions, (int));
STATIC_DCL int FDECL(use_pole, (struct obj *));
STATIC_DCL int FDECL(use_cream_pie, (struct obj *));
STATIC_DCL int FDECL(use_grapple, (struct obj *));
STATIC_DCL int FDECL(do_break_wand, (struct obj *));
STATIC_DCL boolean FDECL(figurine_location_checks, (struct obj *,
                                                    coord *, BOOLEAN_P));
STATIC_DCL void FDECL(add_class, (char *, CHAR_P));
STATIC_DCL void FDECL(setapplyclasses, (char *));
STATIC_PTR boolean FDECL(check_jump, (genericptr_t, int, int));
STATIC_DCL boolean FDECL(is_valid_jump_pos, (int, int, int, BOOLEAN_P));
STATIC_DCL boolean FDECL(get_valid_jump_position, (int, int));
STATIC_DCL boolean FDECL(get_valid_polearm_position, (int, int));
STATIC_DCL boolean FDECL(find_poleable_mon, (coord *, int, int));

#ifdef AMIGA
void FDECL(amii_speaker, (struct obj *, char *, int));
#endif

static const char no_elbow_room[] =
    /*KR "don't have enough elbow-room to maneuver."; */
    "그것을 할 만한 충분한 공간이 없다.";

STATIC_OVL int
use_camera(obj)
struct obj *obj;
{
    struct monst *mtmp;

    if (Underwater) {
        /*KR pline("Using your camera underwater would void the warranty."); */
        pline("물 속에서 카메라를 사용하면 무상 기간 내라도 유상 처리됩니다.");
        return 0;
    }
    if (!getdir((char *) 0))
        return 0;

    if (obj->spe <= 0) {
        pline1(nothing_happens);
        return 1;
    }
    consume_obj_charge(obj, TRUE);

    if (obj->cursed && !rn2(2)) {
        (void) zapyourself(obj, TRUE);
    } else if (u.uswallow) {
#if 0 /*KR:T*/
        You("take a picture of %s %s.", s_suffix(mon_nam(u.ustuck)),
            mbodypart(u.ustuck, STOMACH));
#else
        You("%s의 %s 사진을 찍는다.", mon_nam(u.ustuck),
            mbodypart(u.ustuck, STOMACH));
#endif
    } else if (u.dz) {
        /*KR You("take a picture of the %s.", */
        You("%s의 사진을 찍는다.",
            (u.dz > 0) ? surface(u.ux, u.uy) : ceiling(u.ux, u.uy));
    } else if (!u.dx && !u.dy) {
        (void) zapyourself(obj, TRUE);
    } else if ((mtmp = bhit(u.dx, u.dy, COLNO, FLASHED_LIGHT,
                            (int FDECL((*), (MONST_P, OBJ_P))) 0,
                            (int FDECL((*), (OBJ_P, OBJ_P))) 0, &obj)) != 0) {
        obj->ox = u.ux, obj->oy = u.uy;
        (void) flash_hits_mon(mtmp, obj);
    }
    return 1;
}

STATIC_OVL int
use_towel(obj)
struct obj *obj;
{
    boolean drying_feedback = (obj == uwep);

    if (!freehand()) {
        /*KR You("have no free %s!", body_part(HAND)); */
        You("have no free %s!", body_part(HAND));
        return 0;
    } else if (obj == ublindf) {
        /*KR You("cannot use it while you're wearing it!"); */
        You("그것을 입고 있는 동안에는 사용할 수 없다!");
        return 0;
    } else if (obj->cursed) {
        long old;

        switch (rn2(3)) {
        case 2:
            old = (Glib & TIMEOUT);
            make_glib((int) old + rn1(10, 3)); /* + 3..12 */
#if 0 /*KR:T*/
            Your("%s %s!", makeplural(body_part(HAND)),
                 (old ? "are filthier than ever" : "get slimy"));
#else
            Your("%s이 %s!", makeplural(body_part(HAND)),
                 (old ? "더욱 더러워졌다" : "미끌미끌해졌다"));
#endif
            if (is_wet_towel(obj))
                dry_a_towel(obj, -1, drying_feedback);
            return 1;
        case 1:
            if (!ublindf) {
                old = u.ucreamed;
                u.ucreamed += rn1(10, 3);
#if 0 /*KR:T*/
                pline("Yecch!  Your %s %s gunk on it!", body_part(FACE),
                      (old ? "has more" : "now has"));
#else
                pline("왝! 당신의 %s가 %s 끈적끈적해졌다!", body_part(FACE),
                      (old ? "조금 더" : ""));
#endif
                make_blinded(Blinded + (long) u.ucreamed - old, TRUE);
            } else {
                const char *what;

#if 0 /*KR:T*/
                what = (ublindf->otyp == LENSES)
                           ? "lenses"
                           : (obj->otyp == ublindf->otyp) ? "other towel"
                                                          : "blindfold";
#else
                what = (ublindf->otyp == LENSES)
                           ? "렌즈"
                           : (obj->otyp == ublindf->otyp) ? "수건"
                                                          : "눈가림";
#endif
                if (ublindf->cursed) {
#if 0 /*KR:T*/
                    You("push your %s %s.", what,
                        rn2(2) ? "cock-eyed" : "crooked");
#else
                    pline("%s가 %s.", what,
                        rn2(2) ? "어긋났다" : "비뚤어졌다");
#endif
                } else {
                    struct obj *saved_ublindf = ublindf;
                    /*KR You("push your %s off.", what); */
                    pline("%s가 툭 떨어졌다.", what);
                    Blindf_off(ublindf);
                    dropx(saved_ublindf);
                }
            }
            if (is_wet_towel(obj))
                dry_a_towel(obj, -1, drying_feedback);
            return 1;
        case 0:
            break;
        }
    }

    if (Glib) {
        make_glib(0);
#if 0 /*KR*/
        You("wipe off your %s.",
            !uarmg ? makeplural(body_part(HAND)) : gloves_simple_name(uarmg));
#else
        You("%s를 닦았다.",
            !uarmg ? makeplural(body_part(HAND)) : gloves_simple_name(uarmg));
#endif
        if (is_wet_towel(obj))
            dry_a_towel(obj, -1, drying_feedback);
        return 1;
    } else if (u.ucreamed) {
        Blinded -= u.ucreamed;
        u.ucreamed = 0;
        if (!Blinded) {
            /*KR pline("You've got the glop off."); */
            You("질척이는 걸 없앴다.");
            if (!gulp_blnd_check()) {
                Blinded = 1;
                make_blinded(0L, TRUE);
            }
        } else {
            /*KR Your("%s feels clean now.", body_part(FACE)); */
            pline("%s이 깨끗해졌다.", body_part(FACE));
        }
        if (is_wet_towel(obj))
            dry_a_towel(obj, -1, drying_feedback);
        return 1;
    }

#if 0 /*KR:T*/        
    Your("%s and %s are already clean.", body_part(FACE),
         makeplural(body_part(HAND)));
#else
    Your("%s과 %s는 이미 깨끗하다.", body_part(FACE),
        makeplural(body_part(HAND)));
#endif
    return 0;
}

/* maybe give a stethoscope message based on floor objects */
STATIC_OVL boolean
its_dead(rx, ry, resp)
int rx, ry, *resp;
{
    char buf[BUFSZ];
#if 0 /*KR*/
    boolean more_corpses;
    struct permonst *mptr;
#endif
    struct obj *corpse = sobj_at(CORPSE, rx, ry),
               *statue = sobj_at(STATUE, rx, ry);

    if (!can_reach_floor(TRUE)) { /* levitation or unskilled riding */
        corpse = 0;               /* can't reach corpse on floor */
        /* you can't reach tiny statues (even though you can fight
           tiny monsters while levitating--consistency, what's that?) */
        while (statue && mons[statue->corpsenm].msize == MZ_TINY)
            statue = nxtobj(statue, STATUE, TRUE);
    }
    /* when both corpse and statue are present, pick the uppermost one */
    if (corpse && statue) {
        if (nxtobj(statue, CORPSE, TRUE) == corpse)
            corpse = 0; /* corpse follows statue; ignore it */
        else
            statue = 0; /* corpse precedes statue; ignore statue */
    }
#if 0 /*KR*/
    more_corpses = (corpse && nxtobj(corpse, CORPSE, TRUE));
#endif

    /* additional stethoscope messages from jyoung@apanix.apana.org.au */
    if (!corpse && !statue) {
        ; /* nothing to do */

    } else if (Hallucination) {
        if (!corpse) {
            /* it's a statue */
            /*KR Strcpy(buf, "You're both stoned"); */
            Strcpy(buf, "둘 다 돌머리야");
#if 0 /*KR*/ /*대명사 처리는 하실 필요 없습니다*/
        } else if (corpse->quan == 1L && !more_corpses) {
            int gndr = 2; /* neuter: "it" */
            struct monst *mtmp = get_mtraits(corpse, FALSE);

            /* (most corpses don't retain the monster's sex, so
               we're usually forced to use generic pronoun here) */
            if (mtmp) {
                mptr = mtmp->data = &mons[mtmp->mnum];
                /* TRUE: override visibility check--it's not on the map */
                gndr = pronoun_gender(mtmp, TRUE);
            } else {
                mptr = &mons[corpse->corpsenm];
                if (is_female(mptr))
                    gndr = 1;
                else if (is_male(mptr))
                    gndr = 0;
            }
            Sprintf(buf, "%s's dead", genders[gndr].he); /* "he"/"she"/"it" */
            buf[0] = highc(buf[0]);
#endif
        } else { /* plural */
            /*KR Strcpy(buf, "They're dead"); */
            Strcpy(buf, "그는 죽었어");
        }
        /* variations on "He's dead, Jim." (Star Trek's Dr McCoy) */
        /* KR You_hear("a voice say, \"%s, Jim.\"", buf); */
        You_hear("어떤 목소리가 들린다. \"%s, 짐.\"", buf);
        *resp = 1;
        return TRUE;

    } else if (corpse) {
#if 0 /*KR*/
        boolean here = (rx == u.ux && ry == u.uy),
                one = (corpse->quan == 1L && !more_corpses), reviver = FALSE;
#else
        boolean here = (rx == u.ux && ry == u.uy), reviver = FALSE;
#endif
        int visglyph, corpseglyph;

        visglyph = glyph_at(rx, ry);
        corpseglyph = obj_to_glyph(corpse, rn2);

        if (Blind && (visglyph != corpseglyph))
            map_object(corpse, TRUE);

        if (Role_if(PM_HEALER)) {
            /* ok to reset `corpse' here; we're done with it */
            do {
                if (obj_has_timer(corpse, REVIVE_MON))
                    reviver = TRUE;
                else
                    corpse = nxtobj(corpse, CORPSE, TRUE);
            } while (corpse && !reviver);
        }
#if 0 /*KR*/
        You("determine that %s unfortunate being%s %s%s dead.",
            one ? (here ? "this" : "that") : (here ? "these" : "those"),
            one ? "" : "s", one ? "is" : "are", reviver ? " mostly" : "");
#else
        You("%s 불행한 생명체가 %s 죽었다고 확진했다.",
            here ? "이" : "저",
            reviver ? "거의" : "");
#endif
        return TRUE;

    } else { /* statue */
        const char *what, *how;

#if 0 /*KR*/
        mptr = &mons[statue->corpsenm];
        if (Blind) { /* ignore statue->dknown; it'll always be set */
            Sprintf(buf, "%s %s",
                    (rx == u.ux && ry == u.uy) ? "This" : "That",
                    humanoid(mptr) ? "person" : "creature");
            what = buf;
        } else {
            what = mptr->mname;
            if (!type_is_pname(mptr))
                what = The(what);
        }
#else /*KR 한국어로는 간단하게 */
        if (Blind) { /* ignore statue->dknown; it'll always be set */
            what = (rx == u.ux && ry == u.uy) ? "이것" : "저것";
        }
        else {
            what = mons[statue->corpsenm].mname;
        }
#endif
        /*KR how = "fine"; */
        how = "좋다";
        if (Role_if(PM_HEALER)) {
            struct trap *ttmp = t_at(rx, ry);

            if (ttmp && ttmp->ttyp == STATUE_TRAP)
                /*KR how = "extraordinary"; */
                how = "대단하다";
            else if (Has_contents(statue))
                /*KR how = "remarkable"; */
                how = "놀랍다";
        }

        /*KR pline("%s is in %s health for a statue.", what, how); */
        pline("%s는 조각상 치고는 상태가 %s.", what, how);
        return TRUE;
    }
    return FALSE; /* no corpse or statue */
}

/*KR static const char hollow_str[] = "a hollow sound.  This must be a secret %s!"; */
static const char hollow_str[] = "속이 빈 소리가 난다. 비밀의 %s임이 틀림없다!";

/* Strictly speaking it makes no sense for usage of a stethoscope to
   not take any time; however, unless it did, the stethoscope would be
   almost useless.  As a compromise, one use per turn is free, another
   uses up the turn; this makes curse status have a tangible effect. */
STATIC_OVL int
use_stethoscope(obj)
register struct obj *obj;
{
    struct monst *mtmp;
    struct rm *lev;
    int rx, ry, res;
    boolean interference = (u.uswallow && is_whirly(u.ustuck->data)
                            && !rn2(Role_if(PM_HEALER) ? 10 : 3));

    if (nohands(youmonst.data)) {
#if 0 /*KR:T*/
        You("have no hands!"); /* not `body_part(HAND)' */
#else
        pline("당신은 손이 없다!");
#endif
        return 0;
    } else if (Deaf) {
        /*KR You_cant("hear anything!"); */
        You("아무것도 들리지 않는다!");
        return 0;
    } else if (!freehand()) {
        /*KR You("have no free %s.", body_part(HAND)); */
        You("빈 %s이 없다.", body_part(HAND));
        return 0;
    }
    if (!getdir((char *) 0))
        return 0;

    res = (moves == context.stethoscope_move)
          && (youmonst.movement == context.stethoscope_movement);
    context.stethoscope_move = moves;
    context.stethoscope_movement = youmonst.movement;

    bhitpos.x = u.ux, bhitpos.y = u.uy; /* tentative, reset below */
    notonhead = u.uswallow;
    if (u.usteed && u.dz > 0) {
        if (interference) {
            /*KR pline("%s interferes.", Monnam(u.ustuck)); */
            pline("%s이/가 방해한다.", Monnam(u.ustuck));
            mstatusline(u.ustuck);
        } else
            mstatusline(u.usteed);
        return res;
    } else if (u.uswallow && (u.dx || u.dy || u.dz)) {
        mstatusline(u.ustuck);
        return res;
    } else if (u.uswallow && interference) {
        /*KR pline("%s interferes.", Monnam(u.ustuck)); */
        pline("%s이/가 방해한다.", Monnam(u.ustuck));
        mstatusline(u.ustuck);
        return res;
    } else if (u.dz) {
        if (Underwater)
            /*KR You_hear("faint splashing."); */
            You_hear("희미하게 첨벙첨벙거리는 소리를 듣는다.");
        else if (u.dz < 0 || !can_reach_floor(TRUE))
            cant_reach_floor(u.ux, u.uy, (u.dz < 0), TRUE);
        else if (its_dead(u.ux, u.uy, &res))
            ; /* message already given */
        else if (Is_stronghold(&u.uz))
            /*KR You_hear("the crackling of hellfire."); */
            You_hear("지옥불이 타닥타닥 타오르는 소리를 듣는다.");
        else
            /*KR pline_The("%s seems healthy enough.", surface(u.ux, u.uy)); */
            pline("%s는 충분히 건강해 보인다.", surface(u.ux, u.uy));
        return res;
    } else if (obj->cursed && !rn2(2)) {
        /*KR You_hear("your heart beat."); */
        You_hear("자신의 심장박동 소리를 듣는다.");
        return res;
    }
    if (Stunned || (Confusion && !rn2(5)))
        confdir();
    if (!u.dx && !u.dy) {
        ustatusline();
        return res;
    }
    rx = u.ux + u.dx;
    ry = u.uy + u.dy;
    if (!isok(rx, ry)) {
        /*KR You_hear("a faint typing noise."); */
        You_hear("희미하게 누군가가 키보드를 두드리는 소리를 듣는다.");
        return 0;
    }
    if ((mtmp = m_at(rx, ry)) != 0) {
        const char *mnm = x_monnam(mtmp, ARTICLE_A, (const char *) 0,
                                   SUPPRESS_IT | SUPPRESS_INVISIBLE, FALSE);

        /* bhitpos needed by mstatusline() iff mtmp is a long worm */
        bhitpos.x = rx, bhitpos.y = ry;
        notonhead = (mtmp->mx != rx || mtmp->my != ry);

        if (mtmp->mundetected) {
            if (!canspotmon(mtmp))
                /*KR There("is %s hidden there.", mnm); */
                pline("이 곳에 %s가 숨겨져 있다.", mnm);
            mtmp->mundetected = 0;
            newsym(mtmp->mx, mtmp->my);
        } else if (mtmp->mappearance) {
            /*KR const char *what = "thing"; */
            const char* what = "물체";
#if 0 /*KR*/ /*미사용*/
            boolean use_plural = FALSE;
#endif
            struct obj dummyobj, *odummy;

            switch (M_AP_TYPE(mtmp)) {
            case M_AP_OBJECT:
                /* FIXME?
                 *  we should probably be using object_from_map() here
                 */
                odummy = init_dummyobj(&dummyobj, mtmp->mappearance, 1L);
                /* simple_typename() yields "fruit" for any named fruit;
                   we want the same thing '//' or ';' shows: "slime mold"
                   or "grape" or "slice of pizza" */
                if (odummy->otyp == SLIME_MOLD
                    && has_mcorpsenm(mtmp) && MCORPSENM(mtmp) != NON_PM) {
                    odummy->spe = MCORPSENM(mtmp);
                    what = simpleonames(odummy);
                } else {
                    what = simple_typename(odummy->otyp);
                }
#if 0 /*KR*/
                use_plural = (is_boots(odummy) || is_gloves(odummy)
                              || odummy->otyp == LENSES);
#endif
                break;
            case M_AP_MONSTER: /* ignore Hallucination here */
                what = mons[mtmp->mappearance].mname;
                break;
            case M_AP_FURNITURE:
                what = defsyms[mtmp->mappearance].explanation;
                break;
            }
            seemimic(mtmp);
#if 0 /*KR:T*/
            pline("%s %s %s really %s.",
                  use_plural ? "Those" : "That", what,
                  use_plural ? "are" : "is", mnm);
#else
            pline("이 %s는 사실 %s다.", what, mnm);
#endif
        } else if (flags.verbose && !canspotmon(mtmp)) {
            /*KR There("is %s there.", mnm); */
            pline("여기에는 %s가 있다.", mnm);
        }

        mstatusline(mtmp);
        if (!canspotmon(mtmp))
            map_invisible(rx, ry);
        return res;
    }
    if (unmap_invisible(rx,ry))
        /*KR pline_The("invisible monster must have moved."); */
        pline_The("보이지 않는 괴물이 이동한 것이 분명하다.");

    lev = &levl[rx][ry];
    switch (lev->typ) {
    case SDOOR:
        /*KR You_hear(hollow_str, "door"); */
        You_hear(hollow_str, "문");
        cvt_sdoor_to_door(lev); /* ->typ = DOOR */
        feel_newsym(rx, ry);
        return res;
    case SCORR:
        /*KR You_hear(hollow_str, "passage"); */
        You_hear(hollow_str, "통로");
        lev->typ = CORR, lev->flags = 0;
        unblock_point(rx, ry);
        feel_newsym(rx, ry);
        return res;
    }

    if (!its_dead(rx, ry, &res))
#if 0 /*KR:T*/
        You("hear nothing special."); /* not You_hear()  */
#else
        pline("특별히 다른 소리가 들리지는 않는다.");
#endif
    return res;
}

#if 0 /*KR:T*/
static const char whistle_str[] = "produce a %s whistling sound.",
                  alt_whistle_str[] = "produce a %s, sharp vibration.";
#else
static const char whistle_str[] = "호루라기를 불어 %s 소리를 낸다.",
                  /*KR:TODO:소리와 진동을 일치시킴*/
                  alt_whistle_str[] = "호루라기를 불어 진동을 일으켰다.";
#endif
STATIC_OVL void
use_whistle(obj)
struct obj *obj;
{
    if (!can_blow(&youmonst)) {
        /*KR You("are incapable of using the whistle."); */
        You("호루라기를 사용할 능력이 없다.");
    } else if (Underwater) {
        /*KR You("blow bubbles through %s.", yname(obj)); */
        You("%s를 통해 방울을 불었다.", yname(obj));
    } else {
        if (Deaf)
            /*KR You_feel("rushing air tickle your %s.", body_part(NOSE)); */
            You_feel("공기의 흐름이 %s를 간질인다.", body_part(NOSE));
        else
            /*KR You(whistle_str, obj->cursed ? "shrill" : "high"); */
            You(whistle_str, obj->cursed ? "날카로운" : "새된");
        wake_nearby();
        if (obj->cursed)
            vault_summon_gd();
    }
}

STATIC_OVL void
use_magic_whistle(obj)
struct obj *obj;
{
    register struct monst *mtmp, *nextmon;

    if (!can_blow(&youmonst)) {
        /*KR You("are incapable of using the whistle."); */
        You("호루라기를 사용할 능력이 없다.");
    } else if (obj->cursed && !rn2(2)) {
#if 0 /*KR:T*/
        You("produce a %shigh-%s.", Underwater ? "very " : "",
            Deaf ? "frequency vibration" : "pitched humming noise");
#else
        You("%s %s.", Underwater ? "매우" : "",
            Deaf ? "높은 주파의 진동을 일으켰다" : "높게 윙윙거리는 소리를 냈다");
#endif
        wake_nearby();
    } else {
        int pet_cnt = 0, omx, omy;

        /* it's magic!  it works underwater too (at a higher pitch) */
#if 0 /*KR*/
        You(Deaf ? alt_whistle_str : whistle_str,
            Hallucination ? "normal"
            : (Underwater && !Deaf) ? "strange, high-pitched"
              : "strange");
#else
        You(Deaf ? alt_whistle_str : whistle_str,
            Hallucination ? "호루라기처럼"
            : (Underwater && !Deaf) ? "이상하고 아주 높은"
            : "이상한");
#endif
        for (mtmp = fmon; mtmp; mtmp = nextmon) {
            nextmon = mtmp->nmon; /* trap might kill mon */
            if (DEADMONSTER(mtmp))
                continue;
            /* steed is already at your location, so not affected;
               this avoids trap issues if you're on a trap location */
            if (mtmp == u.usteed)
                continue;
            if (mtmp->mtame) {
                if (mtmp->mtrapped) {
                    /* no longer in previous trap (affects mintrap) */
                    mtmp->mtrapped = 0;
                    fill_pit(mtmp->mx, mtmp->my);
                }
                /* mimic must be revealed before we know whether it
                   actually moves because line-of-sight may change */
                if (M_AP_TYPE(mtmp))
                    seemimic(mtmp);
                omx = mtmp->mx, omy = mtmp->my;
                mnexto(mtmp);
                if (mtmp->mx != omx || mtmp->my != omy) {
                    mtmp->mundetected = 0; /* reveal non-mimic hider */
                    if (canspotmon(mtmp))
                        ++pet_cnt;
                    if (mintrap(mtmp) == 2)
                        change_luck(-1);
                }
            }
        }
        if (pet_cnt > 0)
            makeknown(obj->otyp);
    }
}

boolean
um_dist(x, y, n)
xchar x, y, n;
{
    return (boolean) (abs(u.ux - x) > n || abs(u.uy - y) > n);
}

int
number_leashed()
{
    int i = 0;
    struct obj *obj;

    for (obj = invent; obj; obj = obj->nobj)
        if (obj->otyp == LEASH && obj->leashmon != 0)
            i++;
    return i;
}

/* otmp is about to be destroyed or stolen */
void
o_unleash(otmp)
register struct obj *otmp;
{
    register struct monst *mtmp;

    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
        if (mtmp->m_id == (unsigned) otmp->leashmon)
            mtmp->mleashed = 0;
    otmp->leashmon = 0;
}

/* mtmp is about to die, or become untame */
void
m_unleash(mtmp, feedback)
register struct monst *mtmp;
boolean feedback;
{
    register struct obj *otmp;

    if (feedback) {
        if (canseemon(mtmp))
            /*KR pline("%s pulls free of %s leash!", Monnam(mtmp), mhis(mtmp)); */
            pline("%s이/가 줄을 잡아당겨 도망갔다!", Monnam(mtmp));
        else
            /*KR Your("leash falls slack."); */
            Your("끈이 느슨해져서 떨어졌다.");
    }
    for (otmp = invent; otmp; otmp = otmp->nobj)
        if (otmp->otyp == LEASH && otmp->leashmon == (int) mtmp->m_id)
            otmp->leashmon = 0;
    mtmp->mleashed = 0;
}

/* player is about to die (for bones) */
void
unleash_all()
{
    register struct obj *otmp;
    register struct monst *mtmp;

    for (otmp = invent; otmp; otmp = otmp->nobj)
        if (otmp->otyp == LEASH)
            otmp->leashmon = 0;
    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
        mtmp->mleashed = 0;
}

#define MAXLEASHED 2

boolean
leashable(mtmp)
struct monst *mtmp;
{
    return (boolean) (mtmp->mnum != PM_LONG_WORM
                       && !unsolid(mtmp->data)
                       && (!nolimbs(mtmp->data) || has_head(mtmp->data)));
}

/* ARGSUSED */
STATIC_OVL int
use_leash(obj)
struct obj *obj;
{
    coord cc;
    struct monst *mtmp;
    int spotmon;

    if (u.uswallow) {
        /* if the leash isn't in use, assume we're trying to leash
           the engulfer; if it is use, distinguish between removing
           it from the engulfer versus from some other creature
           (note: the two in-use cases can't actually occur; all
           leashes are released when the hero gets engulfed) */
#if 0 /*KR:T*/
        You_cant((!obj->leashmon
                  ? "leash %s from inside."
                  : (obj->leashmon == (int) u.ustuck->m_id)
                    ? "unleash %s from inside."
                    : "unleash anything from inside %s."),
                 noit_mon_nam(u.ustuck));
#else
        You_cant((!obj->leashmon
            ? "안쪽에서 %s에 줄을 묶을 수는 없다."
            : (obj->leashmon == (int)u.ustuck->m_id)
            ? "안쪽에서 %s을/를 풀어줄 수는 없다."
            : "%s의 안에서는 아무것도 풀어줄 수 없다."),
            noit_mon_nam(u.ustuck));
#endif
        return 0;
    }
    if (!obj->leashmon && number_leashed() >= MAXLEASHED) {
        /*KR You("cannot leash any more pets."); */
        You("이 이상으로 애완동물을 줄에 맬 수는 없다.");
        return 0;
    }

    if (!get_adjacent_loc((char *) 0, (char *) 0, u.ux, u.uy, &cc))
        return 0;

    if (cc.x == u.ux && cc.y == u.uy) {
        if (u.usteed && u.dz > 0) {
            mtmp = u.usteed;
            spotmon = 1;
            goto got_target;
        }
        /*KR pline("Leash yourself?  Very funny..."); */
        pline("자신을 묶는다고? 아주 재미있군...");
        return 0;
    }

    /*
     * From here on out, return value is 1 == a move is used.
     */

    if (!(mtmp = m_at(cc.x, cc.y))) {
        /*KR There("is no creature there."); */
        pline("그 곳에는 아무런 생명체도 없다.");
        (void) unmap_invisible(cc.x, cc.y);
        return 1;
    }

    spotmon = canspotmon(mtmp);
 got_target:

    if (!spotmon && !glyph_is_invisible(levl[cc.x][cc.y].glyph)) {
        /* for the unleash case, we don't verify whether this unseen
           monster is the creature attached to the current leash */
        /*KR You("fail to %sleash something.", obj->leashmon ? "un" : ""); */
        You("%s에 실패했다.", obj->leashmon ? "풀어주는 것" : "묶는 것");
        /* trying again will work provided the monster is tame
           (and also that it doesn't change location by retry time) */
        map_invisible(cc.x, cc.y);
    } else if (!mtmp->mtame) {
#if 0 /*KR:T*/
        pline("%s %s leashed!", Monnam(mtmp),
              (!obj->leashmon) ? "cannot be" : "is not");
#else
        pline("%s은/는 줄로 %s!", Monnam(mtmp),
            (!obj->leashmon) ? "묶을 수 없다" : "묶여있지 않다");
#endif
    } else if (!obj->leashmon) {
        /* applying a leash which isn't currently in use */
        if (mtmp->mleashed) {
#if 0 /*KR:T*/
            pline("This %s is already leashed.",
                  spotmon ? l_monnam(mtmp) : "creature");
#else
            pline("이 %s는 이미 줄에 묶여 있다.",
                spotmon ? l_monnam(mtmp) : "생물");
#endif
        } else if (unsolid(mtmp->data)) {
            /*KR pline("The leash would just fall off."); */
            pline("줄은 그 자리에 떨어졌다.");
        } else if (nolimbs(mtmp->data) && !has_head(mtmp->data)) {
            /*KR pline("%s has no extremities the leash would fit.", */
            pline("%s는 줄을 맬 손발이 없다.",
                  Monnam(mtmp));
        } else if (!leashable(mtmp)) {
#if 0 /*KR:T*/
            pline("The leash won't fit onto %s%s.", spotmon ? "your " : "",
                  l_monnam(mtmp));
#else
            pline("%s는 줄이 맞지 않는다",
                l_monnam(mtmp));
#endif
        } else {
#if 0 /*KR:T*/
            You("slip the leash around %s%s.", spotmon ? "your " : "",
                l_monnam(mtmp));
#else
            You("%s에 줄을 감았다.",
                l_monnam(mtmp));
#endif
            mtmp->mleashed = 1;
            obj->leashmon = (int) mtmp->m_id;
            mtmp->msleeping = 0;
        }
    } else {
        /* applying a leash which is currently in use */
        if (obj->leashmon != (int) mtmp->m_id) {
            /*KR pline("This leash is not attached to that creature."); */
            pline("이 줄은 저 생명체에 연결되어 있지 않다.");
        } else if (obj->cursed) {
            /*KR pline_The("leash would not come off!"); */
            pline("줄이 떨어지지 않는다!");
            set_bknown(obj, 1);
        } else {
            mtmp->mleashed = 0;
            obj->leashmon = 0;
#if 0 /*KR:T*/
            You("remove the leash from %s%s.",
                spotmon ? "your " : "", l_monnam(mtmp));
#else
            You("%s에서 줄을 떼어냈다.",
                l_monnam(mtmp));
#endif
        }
    }
    return 1;
}

/* assuming mtmp->mleashed has been checked */
struct obj *
get_mleash(mtmp)
struct monst *mtmp;
{
    struct obj *otmp;

    otmp = invent;
    while (otmp) {
        if (otmp->otyp == LEASH && otmp->leashmon == (int) mtmp->m_id)
            return otmp;
        otmp = otmp->nobj;
    }
    return (struct obj *) 0;
}

boolean
next_to_u()
{
    register struct monst *mtmp;
    register struct obj *otmp;

    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
        if (DEADMONSTER(mtmp))
            continue;
        if (mtmp->mleashed) {
            if (distu(mtmp->mx, mtmp->my) > 2)
                mnexto(mtmp);
            if (distu(mtmp->mx, mtmp->my) > 2) {
                for (otmp = invent; otmp; otmp = otmp->nobj)
                    if (otmp->otyp == LEASH
                        && otmp->leashmon == (int) mtmp->m_id) {
                        if (otmp->cursed)
                            return FALSE;
#if 0 /*KR:T*/
                        You_feel("%s leash go slack.",
                                 (number_leashed() > 1) ? "a" : "the");
#else
                        You("줄이 느슨해졌다.");
#endif                           
                        mtmp->mleashed = 0;
                        otmp->leashmon = 0;
                    }
            }
        }
    }
    /* no pack mules for the Amulet */
    if (u.usteed && mon_has_amulet(u.usteed))
        return FALSE;
    return TRUE;
}

void
check_leash(x, y)
register xchar x, y;
{
    register struct obj *otmp;
    register struct monst *mtmp;

    for (otmp = invent; otmp; otmp = otmp->nobj) {
        if (otmp->otyp != LEASH || otmp->leashmon == 0)
            continue;
        for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
            if (DEADMONSTER(mtmp))
                continue;
            if ((int) mtmp->m_id == otmp->leashmon)
                break;
        }
        if (!mtmp) {
            impossible("leash in use isn't attached to anything?");
            otmp->leashmon = 0;
            continue;
        }
        if (dist2(u.ux, u.uy, mtmp->mx, mtmp->my)
            > dist2(x, y, mtmp->mx, mtmp->my)) {
            if (!um_dist(mtmp->mx, mtmp->my, 3)) {
                ; /* still close enough */
            } else if (otmp->cursed && !breathless(mtmp->data)) {
                if (um_dist(mtmp->mx, mtmp->my, 5)
                    || (mtmp->mhp -= rnd(2)) <= 0) {
                    long save_pacifism = u.uconduct.killer;

                    /*KR Your("leash chokes %s to death!", mon_nam(mtmp)); */
                    pline("%s는 줄에 목이 졸려 죽었다!", mon_nam(mtmp));
                    /* hero might not have intended to kill pet, but
                       that's the result of his actions; gain experience,
                       lose pacifism, take alignment and luck hit, make
                       corpse less likely to remain tame after revival */
                    xkilled(mtmp, XKILL_NOMSG);
                    /* life-saving doesn't ordinarily reset this */
                    if (!DEADMONSTER(mtmp))
                        u.uconduct.killer = save_pacifism;
                } else {
                    /*KR pline("%s is choked by the leash!", Monnam(mtmp)); */
                    pline("%s는 줄에 목이 졸렸다!", Monnam(mtmp));
                    /* tameness eventually drops to 1 here (never 0) */
                    if (mtmp->mtame && rn2(mtmp->mtame))
                        mtmp->mtame--;
                }
            } else {
                if (um_dist(mtmp->mx, mtmp->my, 5)) {
                    /*KR pline("%s leash snaps loose!", s_suffix(Monnam(mtmp))); */
                    pline("%s의 줄이 탁 하고 풀렸다!", Monnam(mtmp));
                    m_unleash(mtmp, FALSE);
                } else {
                    /*KR You("pull on the leash."); */
                    You("줄을 잡아당겼다.");
                    if (mtmp->data->msound != MS_SILENT)
                        switch (rn2(3)) {
                        case 0:
                            growl(mtmp);
                            break;
                        case 1:
                            yelp(mtmp);
                            break;
                        default:
                            whimper(mtmp);
                            break;
                        }
                }
            }
        }
    }
}

const char *
beautiful()
{
    return ((ACURR(A_CHA) > 14)
               ? ((poly_gender() == 1)
#if 0 /*KR*/
                     ? "beautiful"
                     : "handsome")
               : "ugly");
#else /*KR '어간으로 사용'이라는 주석 존재 */
                     ? "아름다워"
                     : "잘생겨")
               : "추해");
#endif
}

/*KR static const char look_str[] = "look %s."; */
static const char look_str[] = "%s 보인다.";

STATIC_OVL int
use_mirror(obj)
struct obj *obj;
{
    const char *mirror, *uvisage;
    struct monst *mtmp;
    unsigned how_seen;
    char mlet;
    boolean vis, invis_mirror, useeit, monable;

    if (!getdir((char *) 0))
        return 0;
    invis_mirror = Invis;
    useeit = !Blind && (!invis_mirror || See_invisible);
    uvisage = beautiful();
    mirror = simpleonames(obj); /* "mirror" or "looking glass" */
    if (obj->cursed && !rn2(2)) {
        if (!Blind)
            /*KR pline_The("%s fogs up and doesn't reflect!", mirror); */
            pline("%s이 흐려져서 비치지 않는다!", mirror);
        return 1;
    }
    if (!u.dx && !u.dy && !u.dz) {
        if (!useeit) {
            /*KR You_cant("see your %s %s.", uvisage, body_part(FACE)); */
            You_cant("자신의 %s %s를 볼 수가 없다.", uvisage, body_part(FACE));
        } else {
            if (u.umonnum == PM_FLOATING_EYE) {
                if (Free_action) {
                    /*KR You("stiffen momentarily under your gaze."); */
                    You("순간적으로 당신의 눈빛에 경직되었다.");
                } else {
                    if (Hallucination)
                        /*KR pline("Yow!  The %s stares back!", mirror); */
                        pline("이크! %s가 당신을 마주 째려본다!", mirror);
                    else
                        /*KR pline("Yikes!  You've frozen yourself!"); */
                        pline("으윽! 당신은 스스로를 얼어붙게 만들었다!");
                    if (!Hallucination || !rn2(4)) {
                        nomul(-rnd(MAXULEV + 6 - u.ulevel));
                        /*KR multi_reason = "gazing into a mirror"; */
                        multi_reason = "거울에 반사된 시선에 경직되어 있는 동안";
                    }
                    nomovemsg = 0; /* default, "you can move again" */
                }
            } else if (youmonst.data->mlet == S_VAMPIRE)
                /*KR You("don't have a reflection."); */
                You("거울에 비치지 않는다.");
            else if (u.umonnum == PM_UMBER_HULK) {
                /*KR pline("Huh?  That doesn't look like you!"); */
                pline("음? 이건 전혀 당신같아 보이지 않는다!");
                make_confused(HConfusion + d(3, 4), FALSE);
            } else if (Hallucination)
                You(look_str, hcolor((char *) 0));
            else if (Sick)
                /*KR You(look_str, "peaked"); */
                You(look_str, "수척해");
            else if (u.uhs >= WEAK)
                /*KR You(look_str, "undernourished"); */
                You(look_str, "영양 결핍처럼");
            else
                /*KR You("look as %s as ever.", uvisage); */
                You("변함없이 % 보인다.", uvisage);
        }
        return 1;
    }
    if (u.uswallow) {
        if (useeit)
#if 0 /*KR:T*/
            You("reflect %s %s.", s_suffix(mon_nam(u.ustuck)),
                mbodypart(u.ustuck, STOMACH));
#else
            You("%s의 %s를 비췄다.", s_suffix(mon_nam(u.ustuck)),
                mbodypart(u.ustuck, STOMACH));
#endif
        return 1;
    }
    if (Underwater) {
        if (useeit)
#if 0 /*KR:T*/
            You(Hallucination ? "give the fish a chance to fix their makeup."
                              : "reflect the murky water.");
#else
            You(Hallucination ? "물고기에게 화장을 고칠 기회를 주었다."
                              : "당신은 탁한 물을 비췄다.");
#endif
        return 1;
    }
    if (u.dz) {
        if (useeit)
            /*KR You("reflect the %s.", */
            You("%을/를 비췄다.",
                (u.dz > 0) ? surface(u.ux, u.uy) : ceiling(u.ux, u.uy));
        return 1;
    }
    mtmp = bhit(u.dx, u.dy, COLNO, INVIS_BEAM,
                (int FDECL((*), (MONST_P, OBJ_P))) 0,
                (int FDECL((*), (OBJ_P, OBJ_P))) 0, &obj);
    if (!mtmp || !haseyes(mtmp->data) || notonhead)
        return 1;

    /* couldsee(mtmp->mx, mtmp->my) is implied by the fact that bhit()
       targetted it, so we can ignore possibility of X-ray vision */
    vis = canseemon(mtmp);
/* ways to directly see monster (excludes X-ray vision, telepathy,
   extended detection, type-specific warning) */
#define SEENMON (MONSEEN_NORMAL | MONSEEN_SEEINVIS | MONSEEN_INFRAVIS)
    how_seen = vis ? howmonseen(mtmp) : 0;
    /* whether monster is able to use its vision-based capabilities */
    monable = !mtmp->mcan && (!mtmp->minvis || perceives(mtmp->data));
    mlet = mtmp->data->mlet;
    if (mtmp->msleeping) {
        if (vis)
            /*KR pline("%s is too tired to look at your %s.", Monnam(mtmp), */
            pline("%s은/는 당신의 %s을 쳐다보기에는 너무 지쳐 있다.", Monnam(mtmp),
                  mirror);
    } else if (!mtmp->mcansee) {
        if (vis)
            /*KR pline("%s can't see anything right now.", Monnam(mtmp)); */
            pline("%s은/는 지금 아무것도 볼 수 없다.", Monnam(mtmp));
    } else if (invis_mirror && !perceives(mtmp->data)) {
        if (vis)
            /*KR pline("%s fails to notice your %s.", Monnam(mtmp), mirror); */
            pline("%s은/는 당신의 %s을 눈치채지 못했다.", Monnam(mtmp), mirror);
        /* infravision doesn't produce an image in the mirror */
    } else if ((how_seen & SEENMON) == MONSEEN_INFRAVIS) {
        if (vis) /* (redundant) */
#if 0 /*KR:T*/
            pline("%s is too far away to see %sself in the dark.",
                  Monnam(mtmp), mhim(mtmp));
#else
            pline("%s은/는 어둠 속에서 자기 자신을 보기에는 너무 멀리 있다.", Monnam(mtmp));
#endif
        /* some monsters do special things */
    } else if (mlet == S_VAMPIRE || mlet == S_GHOST || is_vampshifter(mtmp)) {
        if (vis)
            /*KR pline("%s doesn't have a reflection.", Monnam(mtmp)); */
            pline("%s은/는 %s에 비치지 않는다.", Monnam(mtmp), mirror);
    } else if (monable && mtmp->data == &mons[PM_MEDUSA]) {
        /*KR if (mon_reflects(mtmp, "The gaze is reflected away by %s %s!")) */
        if (mon_reflects(mtmp, "눈빛이 %s의 %s에 반사되었다!"))
            return 1;
        if (vis)
            /*KR pline("%s is turned to stone!", Monnam(mtmp)); */
            pline("%s은/는 돌로 변해버렸다!", Monnam(mtmp));
        stoned = TRUE;
        killed(mtmp);
    } else if (monable && mtmp->data == &mons[PM_FLOATING_EYE]) {
        int tmp = d((int) mtmp->m_lev, (int) mtmp->data->mattk[0].damd);
        if (!rn2(4))
            tmp = 120;
        if (vis)
            /*KR pline("%s is frozen by its reflection.", Monnam(mtmp)); */
            pline("%s은/는 자신의 비친 모습을 보고는 얼어붙었다.", Monnam(mtmp));
        else
            /*KR You_hear("%s stop moving.", something); */
            You_hear("무언가가 움직임을 멈추었다.");
        paralyze_monst(mtmp, (int) mtmp->mfrozen + tmp);
    } else if (monable && mtmp->data == &mons[PM_UMBER_HULK]) {
        if (vis)
            /*KR pline("%s confuses itself!", Monnam(mtmp)); */
            pline("%s은/는 스스로 혼란에 빠졌다!", Monnam(mtmp));
        mtmp->mconf = 1;
    } else if (monable && (mlet == S_NYMPH || mtmp->data == &mons[PM_SUCCUBUS]
                           || mtmp->data == &mons[PM_INCUBUS])) {
        if (vis) {
            char buf[BUFSZ]; /* "She" or "He" */
#if 0 /*KR*/
            pline("%s admires %sself in your %s.", Monnam(mtmp), mhim(mtmp),
                  mirror);
#else
            pline("%s은/는 자신의 모습을 감탄하며 바라보고 있다.", Monnam(mtmp));
#endif
            /*KR pline("%s takes it!", upstart(strcpy(buf, mhe(mtmp)))); */
            pline("%s이/가 그것을 채갔다!", upstart(strcpy(buf, mhe(mtmp))));
        } else
            /*KR pline("It steals your %s!", mirror); */
            pline("누군가가 당신의 %s을 훔친다!", mirror);
        setnotworn(obj); /* in case mirror was wielded */
        freeinv(obj);
        (void) mpickobj(mtmp, obj);
        if (!tele_restrict(mtmp))
            (void) rloc(mtmp, TRUE);
    } else if (!is_unicorn(mtmp->data) && !humanoid(mtmp->data)
               && (!mtmp->minvis || perceives(mtmp->data)) && rn2(5)) {
        boolean do_react = TRUE;

        if (mtmp->mfrozen) {
            if (vis)
                /*KR You("discern no obvious reaction from %s.", mon_nam(mtmp)); */
                You("%s(으)로부터 별다른 반응을 찾아보지 못한다.", mon_nam(mtmp));
            else
                /*KR You_feel("a bit silly gesturing the mirror in that direction."); */
                You_feel("거울을 그 방향으로 향하는 것은 조금 우습다고 느낀다.");
            do_react = FALSE;
        }
        if (do_react) {
            if (vis)
                /*KR pline("%s is frightened by its reflection.", Monnam(mtmp)); */
                pline("%s은/는 자신의 비친 모습을 보고 겁에 질렸다.", Monnam(mtmp));
            monflee(mtmp, d(2, 4), FALSE, FALSE);
        }
    } else if (!Blind) {
        if (mtmp->minvis && !See_invisible)
            ;
        else if ((mtmp->minvis && !perceives(mtmp->data))
            /* redundant: can't get here if these are true */
            || !haseyes(mtmp->data) || notonhead || !mtmp->mcansee)
#if 0 /*KR:T*/
            pline("%s doesn't seem to notice %s reflection.", Monnam(mtmp),
                mhis(mtmp));
#else
            pline("%s은/는 비친 자신의 모습을 알아채지 못한 것 같다.", Monnam(mtmp));
#endif
        else
           /*KR pline("%s ignores %s reflection.", Monnam(mtmp), mhis(mtmp)); */
            pline("%s은/는 비친 자신의 모습을 무시했다.", Monnam(mtmp));
    }
    return 1;
}

STATIC_OVL void
use_bell(optr)
struct obj **optr;
{
    register struct obj *obj = *optr;
    struct monst *mtmp;
    boolean wakem = FALSE, learno = FALSE,
            ordinary = (obj->otyp != BELL_OF_OPENING || !obj->spe),
            invoking =
                (obj->otyp == BELL_OF_OPENING && invocation_pos(u.ux, u.uy)
                 && !On_stairs(u.ux, u.uy));

    /*KR You("ring %s.", the(xname(obj))); */
    You("%s을 울렸다.", the(xname(obj)));

    if (Underwater || (u.uswallow && ordinary)) {
#ifdef AMIGA
        amii_speaker(obj, "AhDhGqEqDhEhAqDqFhGw", AMII_MUFFLED_VOLUME);
#endif
       /*KR pline("But the sound is muffled."); */
        pline("하지만 소리가 둔탁하다.");

    } else if (invoking && ordinary) {
        /* needs to be recharged... */
        /*KR pline("But it makes no sound."); */
        pline("하지만 아무 소리도 나지 않았다.");
        learno = TRUE; /* help player figure out why */

    } else if (ordinary) {
#ifdef AMIGA
        amii_speaker(obj, "ahdhgqeqdhehaqdqfhgw", AMII_MUFFLED_VOLUME);
#endif
        if (obj->cursed && !rn2(4)
            /* note: once any of them are gone, we stop all of them */
            && !(mvitals[PM_WOOD_NYMPH].mvflags & G_GONE)
            && !(mvitals[PM_WATER_NYMPH].mvflags & G_GONE)
            && !(mvitals[PM_MOUNTAIN_NYMPH].mvflags & G_GONE)
            && (mtmp = makemon(mkclass(S_NYMPH, 0), u.ux, u.uy, NO_MINVENT))
                   != 0) {
            /*KR You("summon %s!", a_monnam(mtmp)); */
            You("%s을/를 소환했다!", a_monnam(mtmp));
            if (!obj_resists(obj, 93, 100)) {
                /*KR pline("%s shattered!", Tobjnam(obj, "have")); */
                pline("%s은 산산조각이 났다!", xname(obj));
                useup(obj);
                *optr = 0;
            } else
                switch (rn2(3)) {
                default:
                    break;
                case 1:
                    mon_adjust_speed(mtmp, 2, (struct obj *) 0);
                    break;
                case 2: /* no explanation; it just happens... */
                    nomovemsg = "";
                    multi_reason = NULL;
                    nomul(-rnd(2));
                    break;
                }
        }
        wakem = TRUE;

    } else {
        /* charged Bell of Opening */
        consume_obj_charge(obj, TRUE);

        if (u.uswallow) {
            if (!obj->cursed)
                (void) openit();
            else
                pline1(nothing_happens);

        } else if (obj->cursed) {
            coord mm;

            mm.x = u.ux;
            mm.y = u.uy;
            mkundead(&mm, FALSE, NO_MINVENT);
            wakem = TRUE;

        } else if (invoking) {
            /* pline("%s an unsettling shrill sound...", Tobjnam(obj, "issue")); */
            pline("%s은 불안하게 만드는 날카로운 소리를 냈다...", xname(obj));
#ifdef AMIGA
            amii_speaker(obj, "aefeaefeaefeaefeaefe", AMII_LOUDER_VOLUME);
#endif
            obj->age = moves;
            learno = TRUE;
            wakem = TRUE;

        } else if (obj->blessed) {
            int res = 0;

#ifdef AMIGA
            amii_speaker(obj, "ahahahDhEhCw", AMII_SOFT_VOLUME);
#endif
            if (uchain) {
                unpunish();
                res = 1;
            } else if (u.utrap && u.utraptype == TT_BURIEDBALL) {
                buried_ball_to_freedom();
                res = 1;
            }
            res += openit();
            switch (res) {
            case 0:
                pline1(nothing_happens);
                break;
            case 1:
                /*KR pline("%s opens...", Something); */
                pline("무언가가 열린다...");
                learno = TRUE;
                break;
            default:
                /*KR pline("Things open around you..."); */
                pline("당신 주변의 물체가 열린다...");
                learno = TRUE;
                break;
            }

        } else { /* uncursed */
#ifdef AMIGA
            amii_speaker(obj, "AeFeaeFeAefegw", AMII_OKAY_VOLUME);
#endif
            if (findit() != 0)
                learno = TRUE;
            else
                pline1(nothing_happens);
        }

    } /* charged BofO */

    if (learno) {
        makeknown(BELL_OF_OPENING);
        obj->known = 1;
    }
    if (wakem)
        wake_nearby();
}

STATIC_OVL void
use_candelabrum(obj)
register struct obj *obj;
{
#if 0 /*KR*/ /*미사용*/
    const char *s = (obj->spe != 1) ? "candles" : "candle";
#endif

    if (obj->lamplit) {
        /*KR You("snuff the %s.", s); */
        You("촛불을 훅 불어 꺼트렸다.");
        end_burn(obj, TRUE);
        return;
    }
    if (obj->spe <= 0) {
        /*KR pline("This %s has no %s.", xname(obj), s); */
        pline("이 %s는 불이 켜져 있지 않다.", xname(obj));
        return;
    }
    if (Underwater) {
        /*KR You("cannot make fire under water."); */
        You("물 속에서는 불을 피울 수가 없다.");
        return;
    }
    if (u.uswallow || obj->cursed) {
        if (!Blind)
#if 0 /*KR:T*/
            pline_The("%s %s for a moment, then %s.", s, vtense(s, "flicker"),
                vtense(s, "die"));
#else
            pline("양초의 불이 잠시 깜빡거리다가 꺼졌다.");
#endif
        return;
    }
    if (obj->spe < 7) {
#if 0 /*KR:T*/
        There("%s only %d %s in %s.", vtense(s, "are"), obj->spe, s,
              the(xname(obj)));
#else
        pline("%s에는 단지 %d개의 촛불밖에 없다.",
              xname(obj), obj->spe);
#endif
        if (!Blind)
#if 0 /*KR:T*/
            pline("%s lit.  %s dimly.", obj->spe == 1 ? "It is" : "They are",
                Tobjnam(obj, "shine"));
#else
            pline("%s에 불이 켜졌다.  %s의 불이 흐릿하다.",
                xname(obj), xname(obj));
#endif
    } else {
#if 0 /*KR:T*/
        pline("%s's %s burn%s", The(xname(obj)), s,
              (Blind ? "." : " brightly!"));
#else
        pline("%s의 불이 %s 타오른다!", The(xname(obj)),
            (Blind ? "" : "밝게"));
#endif
    }
    if (!invocation_pos(u.ux, u.uy) || On_stairs(u.ux, u.uy)) {
        /*KR pline_The("%s %s being rapidly consumed!", s, vtense(s, "are")); */
        pline("양초가 매우 빠르게 타오르고 있다!");
        /* this used to be obj->age /= 2, rounding down; an age of
           1 would yield 0, confusing begin_burn() and producing an
           unlightable, unrefillable candelabrum; round up instead */
        obj->age = (obj->age + 1L) / 2L;
    } else {
        if (obj->spe == 7) {
            if (Blind)
                /*KR pline("%s a strange warmth!", Tobjnam(obj, "radiate")); */
                pline("%s에서 이상한 따뜻함을 느꼈다!", xname(obj));
            else
                /*KR pline("%s with a strange light!", Tobjnam(obj, "glow")); */
                pline("%s가 이상한 빛을 내뿜었다!", xname(obj));
        }
        obj->known = 1;
    }
    begin_burn(obj, FALSE);
}

STATIC_OVL void
use_candle(optr)
struct obj **optr;
{
    register struct obj *obj = *optr;
    register struct obj *otmp;
    /*KR const char* s = (obj->quan != 1) ? "candles" : "candle"; */
    const char *s = "양초";
    char qbuf[QBUFSZ], qsfx[QBUFSZ], *q;

    if (u.uswallow) {
        You(no_elbow_room);
        return;
    }

    otmp = carrying(CANDELABRUM_OF_INVOCATION);
    if (!otmp || otmp->spe == 7) {
        use_lamp(obj);
        return;
    }

    /*KR 마지막으로는 "양초를 촛대에 꽂겠습니까?" */
    /* first, minimal candelabrum suffix for formatting candles */
    /*KR Sprintf(qsfx, " to\033%s?", thesimpleoname(otmp)); */
    Sprintf(qsfx, "를 \033%s에 꽂겠습니까? ", thesimpleoname(otmp));
    /* next, format the candles as a prefix for the candelabrum */
    /*KR (void) safe_qbuf(qbuf, "Attach ", qsfx, obj, yname, thesimpleoname, s); */
    (void) safe_qbuf(qbuf, "", qsfx, obj, xname, thesimpleoname, s);
    /*KR "(양초)를 \033%s 촛대에 꽂겠습니까?" */
    /* strip temporary candelabrum suffix */
#if 0 /*KR*/
    if ((q = strstri(qbuf, " to\033")) != 0)
        Strcpy(q, " to ");
#else
    if ((q = strchr(qbuf, '\033')) != 0)
        *q = '\0';
    /*KR: "(양초)를" */
#endif
    /* last, format final "attach candles to candelabrum?" query */
    /*KR if (yn(safe_qbuf(qbuf, qbuf, "?", otmp, yname, thesimpleoname, "it")) */
    if (yn(safe_qbuf(qbuf, qbuf, "에 꽂을까요?", otmp, xname, thesimpleoname, "그것"))
        == 'n') {
        use_lamp(obj);
        return;
    } else {
        if ((long) otmp->spe + obj->quan > 7L) {
            obj = splitobj(obj, 7L - (long) otmp->spe);
#if 0 /* KR:한국어에서는 불필요 */
            /* avoid a grammatical error if obj->quan gets
               reduced to 1 candle from more than one */
            s = (obj->quan != 1) ? "candles" : "candle";
#endif
        } else
            *optr = 0;
#if 0 /*KR:T*/
        You("attach %ld%s %s to %s.", obj->quan, !otmp->spe ? "" : " more", s,
            the(xname(otmp)));
#else
        You("%ld개의 양초를 %s %s에 꽂았다.", obj->quan, !otmp->spe ? "" : "더",
            xname(otmp));
#endif
        if (!otmp->spe || otmp->age > obj->age)
            otmp->age = obj->age;
        otmp->spe += (int) obj->quan;
        if (otmp->lamplit && !obj->lamplit)
            /*KR pline_The("new %s magically %s!", s, vtense(s, "ignite")); */
            pline("새로운 양초가 마법처럼 타오른다!");
        else if (!otmp->lamplit && obj->lamplit)
            /*KR pline("%s out.", (obj->quan > 1L) ? "They go" : "It goes"); */
            pline("불이 꺼졌다.");
        if (obj->unpaid)
#if 0 /*KR*/
            verbalize("You %s %s, you bought %s!",
                otmp->lamplit ? "burn" : "use",
                (obj->quan > 1L) ? "them" : "it",
                (obj->quan > 1L) ? "them" : "it");
#else
            verbalize("불을 붙였으니, 물어 내셔야지!");
#endif
        if (obj->quan < 7L && otmp->spe == 7)
#if 0 /*KR:T*/
            pline("%s now has seven%s candles attached.", The(xname(otmp)),
                  otmp->lamplit ? " lit" : "");
#else
            pline("이제 %s에는 7개의 %s 양초가 꽂혀 있다.", The(xname(otmp)),
                  otmp->lamplit ? "불이 켜진" : "");
#endif
        /* candelabrum's light range might increase */
        if (otmp->lamplit)
            obj_merge_light_sources(otmp, otmp);
        /* candles are no longer a separate light source */
        if (obj->lamplit)
            end_burn(obj, TRUE);
        /* candles are now gone */
        useupall(obj);
        /* candelabrum's weight is changing */
        otmp->owt = weight(otmp);
        update_inventory();
    }
}

/* call in drop, throw, and put in box, etc. */
boolean
snuff_candle(otmp)
struct obj *otmp;
{
    boolean candle = Is_candle(otmp);

    if ((candle || otmp->otyp == CANDELABRUM_OF_INVOCATION)
        && otmp->lamplit) {
        char buf[BUFSZ];
        xchar x, y;
        boolean many = candle ? (otmp->quan > 1L) : (otmp->spe > 1);

        (void) get_obj_location(otmp, &x, &y, 0);
        if (otmp->where == OBJ_MINVENT ? cansee(x, y) : !Blind)
#if 0 /*KR:T*/
            pline("%s%scandle%s flame%s extinguished.", Shk_Your(buf, otmp),
                (candle ? "" : "candelabrum's "), (many ? "s'" : "'s"),
                (many ? "s are" : " is"));
#else
            pline("%s %s 양초의 불이 꺼졌다.", Shk_Your(buf, otmp),
                candle ? "" : "촛대의");
#endif
        end_burn(otmp, TRUE);
        return TRUE;
    }
    return FALSE;
}

/* called when lit lamp is hit by water or put into a container or
   you've been swallowed by a monster; obj might be in transit while
   being thrown or dropped so don't assume that its location is valid */
boolean
snuff_lit(obj)
struct obj *obj;
{
    xchar x, y;

    if (obj->lamplit) {
        if (obj->otyp == OIL_LAMP || obj->otyp == MAGIC_LAMP
            || obj->otyp == BRASS_LANTERN || obj->otyp == POT_OIL) {
            (void) get_obj_location(obj, &x, &y, 0);
            if (obj->where == OBJ_MINVENT ? cansee(x, y) : !Blind)
                /*KR pline("%s %s out!", Yname2(obj), otense(obj, "go")); */
                pline("%s이 꺼졌다!", Yname2(obj));
            end_burn(obj, TRUE);
            return TRUE;
        }
        if (snuff_candle(obj))
            return TRUE;
    }
    return FALSE;
}

/* Called when potentially lightable object is affected by fire_damage().
   Return TRUE if object was lit and FALSE otherwise --ALI */
boolean
catch_lit(obj)
struct obj *obj;
{
    xchar x, y;

    if (!obj->lamplit && (obj->otyp == MAGIC_LAMP || ignitable(obj))) {
        if ((obj->otyp == MAGIC_LAMP
             || obj->otyp == CANDELABRUM_OF_INVOCATION) && obj->spe == 0)
            return FALSE;
        else if (obj->otyp != MAGIC_LAMP && obj->age == 0)
            return FALSE;
        if (!get_obj_location(obj, &x, &y, 0))
            return FALSE;
        if (obj->otyp == CANDELABRUM_OF_INVOCATION && obj->cursed)
            return FALSE;
        if ((obj->otyp == OIL_LAMP || obj->otyp == MAGIC_LAMP
             || obj->otyp == BRASS_LANTERN) && obj->cursed && !rn2(2))
            return FALSE;
        if (obj->where == OBJ_MINVENT ? cansee(x, y) : !Blind)
            /*KR pline("%s %s light!", Yname2(obj), otense(obj, "catch")); */
            pline("%s의 불이 켜졌다!", Yname2(obj));
        if (obj->otyp == POT_OIL)
            makeknown(obj->otyp);
        if (carried(obj) && obj->unpaid && costly_spot(u.ux, u.uy)) {
            /* if it catches while you have it, then it's your tough luck */
            check_unpaid(obj);
#if 0 /*KR:T*/
            verbalize("That's in addition to the cost of %s %s, of course.",
                      yname(obj), obj->quan == 1L ? "itself" : "themselves");
#else
            verbalize("그건 물론 %s의 가격과는 별개야.", xname(obj));
#endif
            bill_dummy_object(obj);
        }
        begin_burn(obj, FALSE);
        return TRUE;
    }
    return FALSE;
}

STATIC_OVL void
use_lamp(obj)
struct obj *obj;
{
    char buf[BUFSZ];

    if (obj->lamplit) {
        if (obj->otyp == OIL_LAMP || obj->otyp == MAGIC_LAMP
            || obj->otyp == BRASS_LANTERN)
            /*KR pline("%slamp is now off.", Shk_Your(buf, obj)); */
            pline("%s 램프의 불이 꺼졌다.", Shk_Your(buf, obj));
        else
            /*KR You("snuff out %s.", yname(obj)); */
            You("%s를 불어 꺼트렸다.", xname(obj));
        end_burn(obj, TRUE);
        return;
    }
    if (Underwater) {
#if 0 /*KR:T*/
        pline(!Is_candle(obj) ? "This is not a diving lamp"
                              : "Sorry, fire and water don't mix.");
#else
        pline(!Is_candle(obj) ? "이건 잠수용 램프가 아니다."
                              : "미안하지만, 불과 물은 섞이지 않아.");
#endif
        return;
    }
    /* magic lamps with an spe == 0 (wished for) cannot be lit */
    if ((!Is_candle(obj) && obj->age == 0)
        || (obj->otyp == MAGIC_LAMP && obj->spe == 0)) {
        if (obj->otyp == BRASS_LANTERN)
            /*KR Your("lamp has run out of power."); */
            Your("램프의 전력이 다 된 것 같다.");
        else
            /*KR pline("This %s has no oil.", xname(obj)); */
            pline("이 %s에는 기름이 없다.", xname(obj));
        return;
    }
    if (obj->cursed && !rn2(2)) {
        if (!Blind)
#if 0 /*KR:T*/
            pline("%s for a moment, then %s.", Tobjnam(obj, "flicker"),
                otense(obj, "die"));
#else
            pline("%s가 잠시 깜빡거리다가 꺼졌다.", xname(obj));
#endif
    } else {
        if (obj->otyp == OIL_LAMP || obj->otyp == MAGIC_LAMP
            || obj->otyp == BRASS_LANTERN) {
            check_unpaid(obj);
            /*KR pline("%slamp is now on.", Shk_Your(buf, obj)); */
            pline("%s 램프가 켜졌다.", Shk_Your(buf, obj));
        } else { /* candle(s) */
#if 0 /*KR:T*/
            pline("%s flame%s %s%s", s_suffix(Yname2(obj)), plur(obj->quan),
                  otense(obj, "burn"), Blind ? "." : " brightly!");
#else
            pline("%s가 %s 타오른다!", xname(obj), Blind ? "" : "밝게");
#endif
            if (obj->unpaid && costly_spot(u.ux, u.uy)
                && obj->age == 20L * (long) objects[obj->otyp].oc_cost) {
#if 0 /*KR*/
                const char *ithem = (obj->quan > 1L) ? "them" : "it";

                verbalize("You burn %s, you bought %s!", ithem, ithem);
#else
                verbalize("불을 붙였으니, 물어 내셔야지!");
#endif
                bill_dummy_object(obj);
            }
        }
        begin_burn(obj, FALSE);
    }
}

STATIC_OVL void
light_cocktail(optr)
struct obj **optr;
{
    struct obj *obj = *optr; /* obj is a potion of oil */
    char buf[BUFSZ];
    boolean split1off;

    if (u.uswallow) {
        You(no_elbow_room);
        return;
    }

    if (obj->lamplit) {
        /*KR You("snuff the lit potion."); */
        You("기름 포션의 불을 불어 꺼트렸다.");
        end_burn(obj, TRUE);
        /*
         * Free & add to re-merge potion.  This will average the
         * age of the potions.  Not exactly the best solution,
         * but its easy.
         */
        freeinv(obj);
        *optr = addinv(obj);
        return;
    } else if (Underwater) {
        /*KR There("is not enough oxygen to sustain a fire."); */
        pline("불을 유지하기엔 산소가 부족하다.");
        return;
    }

    split1off = (obj->quan > 1L);
    if (split1off)
        obj = splitobj(obj, 1L);

#if 0 /*KR:T*/
    You("light %spotion.%s", shk_your(buf, obj),
        Blind ? "" : "  It gives off a dim light.");
#else
    You("%s 기름 포션에 불을 붙였다. %s", shk_your(buf, obj),
        Blind ? "" : "희미한 불빛이 난다.");
#endif
    if (obj->unpaid && costly_spot(u.ux, u.uy)) {
        /* Normally, we shouldn't both partially and fully charge
         * for an item, but (Yendorian Fuel) Taxes are inevitable...
         */
        check_unpaid(obj);
       /*KR verbalize("That's in addition to the cost of the potion, of course."); */
        verbalize("그건 물론 포션의 가격과는 별개야.");
        bill_dummy_object(obj);
    }
    makeknown(obj->otyp);

    begin_burn(obj, FALSE); /* after shop billing */
    if (split1off) {
        obj_extract_self(obj); /* free from inv */
        obj->nomerge = 1;
        /*KR obj = hold_another_object(obj, "You drop %s!", doname(obj), */
        obj = hold_another_object(obj, "당신은 %을/를 떨어트렸다!", doname(obj),
                                  (const char *) 0);
        if (obj)
            obj->nomerge = 0;
    }
    *optr = obj;
}

static NEARDATA const char cuddly[] = { TOOL_CLASS, GEM_CLASS, 0 };

int
dorub()
{
    struct obj *obj = getobj(cuddly, "rub");

    if (obj && obj->oclass == GEM_CLASS) {
        if (is_graystone(obj)) {
            use_stone(obj);
            return 1;
        } else {
            /*KR pline("Sorry, I don't know how to use that."); */
            pline("미안, 어떻게 사용하는 건지 몰라.");
            return 0;
        }
    }

    if (!obj || !wield_tool(obj, "rub"))
        return 0;

    /* now uwep is obj */
    if (uwep->otyp == MAGIC_LAMP) {
        if (uwep->spe > 0 && !rn2(3)) {
            check_unpaid_usage(uwep, TRUE); /* unusual item use */
            /* bones preparation:  perform the lamp transformation
               before releasing the djinni in case the latter turns out
               to be fatal (a hostile djinni has no chance to attack yet,
               but an indebted one who grants a wish might bestow an
               artifact which blasts the hero with lethal results) */
            uwep->otyp = OIL_LAMP;
            uwep->spe = 0; /* for safety */
            uwep->age = rn1(500, 1000);
            if (uwep->lamplit)
                begin_burn(uwep, TRUE);
            djinni_from_bottle(uwep);
            makeknown(MAGIC_LAMP);
            update_inventory();
        } else if (rn2(2)) {
            /*KR You("%s smoke.", !Blind ? "see a puff of" : "smell"); */
            pline("%s에서 연기가 난다.", !Blind ? "에서 연기가 올라온다" : "의 냄새가 난다");
        } else
            pline1(nothing_happens);
    } else if (obj->otyp == BRASS_LANTERN) {
        /* message from Adventure */
        /*KR pline("Rubbing the electric lamp is not particularly rewarding."); */
        pline("전기 램프를 문지른다고 해서 딱히 뭔가 있을 것 같지는 않다.");
        /*KR pline("Anyway, nothing exciting happens."); */
        pline("아무튼, 뭔가 신나는 일이 일어나지는 않았다.");
    } else
        pline1(nothing_happens);
    return 1;
}

int
dojump()
{
    /* Physical jump */
    return jump(0);
}

enum jump_trajectory {
    jAny  = 0, /* any direction => magical jump */
    jHorz = 1,
    jVert = 2,
    jDiag = 3  /* jHorz|jVert */
};

/* callback routine for walk_path() */
STATIC_PTR boolean
check_jump(arg, x, y)
genericptr arg;
int x, y;
{
    int traj = *(int *) arg;
    struct rm *lev = &levl[x][y];

    if (Passes_walls)
        return TRUE;
    if (IS_STWALL(lev->typ))
        return FALSE;
    if (IS_DOOR(lev->typ)) {
        if (closed_door(x, y))
            return FALSE;
        if ((lev->doormask & D_ISOPEN) != 0 && traj != jAny
            /* reject diagonal jump into or out-of or through open door */
            && (traj == jDiag
                /* reject horizontal jump through horizontal open door
                   and non-horizontal (ie, vertical) jump through
                   non-horizontal (vertical) open door */
                || ((traj & jHorz) != 0) == (lev->horizontal != 0)))
            return FALSE;
        /* empty doorways aren't restricted */
    }
    /* let giants jump over boulders (what about Flying?
       and is there really enough head room for giants to jump
       at all, let alone over something tall?) */
    if (sobj_at(BOULDER, x, y) && !throws_rocks(youmonst.data))
        return FALSE;
    return TRUE;
}

STATIC_OVL boolean
is_valid_jump_pos(x, y, magic, showmsg)
int x, y, magic;
boolean showmsg;
{
    if (!magic && !(HJumping & ~INTRINSIC) && !EJumping && distu(x, y) != 5) {
        /* The Knight jumping restriction still applies when riding a
         * horse.  After all, what shape is the knight piece in chess?
         */
        if (showmsg)
            /*KR pline("Illegal move!"); */
            pline("그곳으로 갈 수 없다!");
        return FALSE;
    } else if (distu(x, y) > (magic ? 6 + magic * 3 : 9)) {
        if (showmsg)
            /*KR pline("Too far!"); */
            pline("너무 멀다!");
        return FALSE;
    } else if (!isok(x, y)) {
        if (showmsg)
            /*KR You("cannot jump there!"); */
            You("거기로 뛸 수는 없다!");
        return FALSE;
    } else if (!cansee(x, y)) {
        if (showmsg)
            /*KR You("cannot see where to land!"); */
            You("착지점이 보이지 않는다!");
        return FALSE;
    } else {
        coord uc, tc;
        struct rm *lev = &levl[u.ux][u.uy];
        /* we want to categorize trajectory for use in determining
           passage through doorways: horizonal, vertical, or diagonal;
           since knight's jump and other irregular directions are
           possible, we flatten those out to simplify door checks */
        int diag, traj,
            dx = x - u.ux, dy = y - u.uy,
            ax = abs(dx), ay = abs(dy);

        /* diag: any non-orthogonal destination classifed as diagonal */
        diag = (magic || Passes_walls || (!dx && !dy)) ? jAny
               : !dy ? jHorz : !dx ? jVert : jDiag;
        /* traj: flatten out the trajectory => some diagonals re-classified */
        if (ax >= 2 * ay)
            ay = 0;
        else if (ay >= 2 * ax)
            ax = 0;
        traj = (magic || Passes_walls || (!ax && !ay)) ? jAny
               : !ay ? jHorz : !ax ? jVert : jDiag;
        /* walk_path doesn't process the starting spot;
           this is iffy:  if you're starting on a closed door spot,
           you _can_ jump diagonally from doorway (without needing
           Passes_walls); that's intentional but is it correct? */
        if (diag == jDiag && IS_DOOR(lev->typ)
            && (lev->doormask & D_ISOPEN) != 0
            && (traj == jDiag
                || ((traj & jHorz) != 0) == (lev->horizontal != 0))) {
            if (showmsg)
                /*KR You_cant("jump diagonally out of a doorway.");*/
                You_cant("출구에서 대각선으로 점프할 수는 없다.");
            return FALSE;
        }
        uc.x = u.ux, uc.y = u.uy;
        tc.x = x, tc.y = y; /* target */
        if (!walk_path(&uc, &tc, check_jump, (genericptr_t) &traj)) {
            if (showmsg)
                /*KR There("is an obstacle preventing that jump."); */
                pline("점프를 방해하는 장애물이 있다.");
            return FALSE;
        }
    }
    return TRUE;
}

static int jumping_is_magic;

STATIC_OVL boolean
get_valid_jump_position(x,y)
int x,y;
{
    return (isok(x, y)
            && (ACCESSIBLE(levl[x][y].typ) || Passes_walls)
            && is_valid_jump_pos(x, y, jumping_is_magic, FALSE));
}

STATIC_OVL void
display_jump_positions(state)
int state;
{
    if (state == 0) {
        tmp_at(DISP_BEAM, cmap_to_glyph(S_goodpos));
    } else if (state == 1) {
        int x, y, dx, dy;

        for (dx = -4; dx <= 4; dx++)
            for (dy = -4; dy <= 4; dy++) {
                x = dx + (int) u.ux;
                y = dy + (int) u.uy;
                if (get_valid_jump_position(x, y))
                    tmp_at(x, y);
            }
    } else {
        tmp_at(DISP_END, 0);
    }
}

int
jump(magic)
int magic; /* 0=Physical, otherwise skill level */
{
    coord cc;

    /* attempt "jumping" spell if hero has no innate jumping ability */
    if (!magic && !Jumping) {
        int sp_no;

        for (sp_no = 0; sp_no < MAXSPELL; ++sp_no)
            if (spl_book[sp_no].sp_id == NO_SPELL)
                break;
            else if (spl_book[sp_no].sp_id == SPE_JUMPING)
                return spelleffects(sp_no, FALSE);
    }

    if (!magic && (nolimbs(youmonst.data) || slithy(youmonst.data))) {
        /* normally (nolimbs || slithy) implies !Jumping,
           but that isn't necessarily the case for knights */
        /*KR You_cant("jump; you have no legs!"); */
        pline("발이 없어서 점프할 수 없다!");
        return 0;
    } else if (!magic && !Jumping) {
        /*KR You_cant("jump very far."); */
        You_cant("그렇게 멀리 점프할 수는 없다.");
        return 0;
    /* if steed is immobile, can't do physical jump but can do spell one */
    } else if (!magic && u.usteed && stucksteed(FALSE)) {
        /* stucksteed gave "<steed> won't move" message */
        return 0;
    } else if (u.uswallow) {
        if (magic) {
            /*KR You("bounce around a little."); */
            pline("잠시 이리저리 통통 튀었다.");
            return 1;
        }
        /*KR pline("You've got to be kidding!"); */
        pline("농담하는 거지?!");
        return 0;
    } else if (u.uinwater) {
        if (magic) {
            /*KR You("swish around a little."); */
            pline("잠시 이리저리 휙휙 움직였다.");
            return 1;
        }
        /*KR pline("This calls for swimming, not jumping!"); */
        pline("이건 '수영'이라고 하는 거지, '점프'가 아니야!");
        return 0;
    } else if (u.ustuck) {
        if (u.ustuck->mtame && !Conflict && !u.ustuck->mconf) {
            /*KR You("pull free from %s.", mon_nam(u.ustuck)); */
            You("%s에게서 풀려났다.", mon_nam(u.ustuck));
            u.ustuck = 0;
            return 1;
        }
        if (magic) {
            /*KR You("writhe a little in the grasp of %s!", mon_nam(u.ustuck)); */
            You("%에게 붙잡혀있는 채로 몸을 조금 비틀었다!", mon_nam(u.ustuck));
            return 1;
        }
        /*KR You("cannot escape from %s!", mon_nam(u.ustuck)); */
        You("%에게서 벗어날 수 없다!", mon_nam(u.ustuck));
        return 0;
    } else if (Levitation || Is_airlevel(&u.uz) || Is_waterlevel(&u.uz)) {
        if (magic) {
            /*KR You("flail around a little."); */
            You("잠시 몸을 휙휙 비틀었다.");
            return 1;
        }
        /*KR You("don't have enough traction to jump."); */
        You("뛰기 위한 충분한 반동이 부족하다.");
        return 0;
    } else if (!magic && near_capacity() > UNENCUMBERED) {
        /*KR You("are carrying too much to jump!"); */
        You("뛰기엔 너무 많이 들고 있다!");
        return 0;
    } else if (!magic && (u.uhunger <= 100 || ACURR(A_STR) < 6)) {
        /*KR You("lack the strength to jump!"); */
        You("뛸 힘이 없다!");
        return 0;
    } else if (!magic && Wounded_legs) {
        long wl = (Wounded_legs & BOTH_SIDES);
        const char *bp = body_part(LEG);

        if (wl == BOTH_SIDES)
            bp = makeplural(bp);
        if (u.usteed)
            /*KR pline("%s is in no shape for jumping.", Monnam(u.usteed)); */
            pline("%s은/는 뛸 수 있는 상태가 아니다.", Monnam(u.usteed));
        else
#if 0 /*KR:T*/            
            Your("%s%s %s in no shape for jumping.",
                (wl == LEFT_SIDE) ? "left " : (wl == RIGHT_SIDE) ? "right "
                : "",
                bp, (wl == BOTH_SIDES) ? "are" : "is");
#else
            Your("%s %s은/는 뛸 수 있는 상태가 아니다.",
                (wl == LEFT_SIDE) ? "왼쪽" : 
                (wl == RIGHT_SIDE) ? "오른쪽" : "", bp);
#endif
        return 0;
    } else if (u.usteed && u.utrap) {
        /*KR pline("%s is stuck in a trap.", Monnam(u.usteed)); */
        pline("%s은/는 함정에 갇혀 있다.", Monnam(u.usteed));
        return 0;
    }

    /*KR pline("Where do you want to jump?"); */
    pline("어디로 뛰시겠습니까?");
    cc.x = u.ux;
    cc.y = u.uy;
    jumping_is_magic = magic;
    getpos_sethilite(display_jump_positions, get_valid_jump_position);
    /*KR if (getpos(&cc, TRUE, "the desired position") < 0) */
    if (getpos(&cc, TRUE, "원하는 장소") < 0)
        return 0; /* user pressed ESC */
    if (!is_valid_jump_pos(cc.x, cc.y, magic, TRUE)) {
        return 0;
    } else {
        coord uc;
        int range, temp;

        if (u.utrap)
            switch (u.utraptype) {
            case TT_BEARTRAP: {
                long side = rn2(3) ? LEFT_SIDE : RIGHT_SIDE;

                /*KR You("rip yourself free of the bear trap!  Ouch!"); */
                You("곰덫에서 몸을 떼어냈다! 아야!");
                /*KR losehp(Maybe_Half_Phys(rnd(10)), "jumping out of a bear trap", */
                losehp(Maybe_Half_Phys(rnd(10)), "곰덫에서 뛰어나오려다",
                       KILLED_BY);
                set_wounded_legs(side, rn1(1000, 500));
                break;
            }
            case TT_PIT:
                /*KR You("leap from the pit!"); */
                You("함정에서 뛰쳐나왔다!");
                break;
            case TT_WEB:
                /*KR You("tear the web apart as you pull yourself free!"); */
                You("거미줄을 찢고, 자유로워졌다!");
                deltrap(t_at(u.ux, u.uy));
                break;
            case TT_LAVA:
                /*KR You("pull yourself above the %s!", hliquid("lava")); */
                You("%s에서 뛰쳐나왔다!", hliquid("용암"));
                reset_utrap(TRUE);
                return 1;
            case TT_BURIEDBALL:
            case TT_INFLOOR:
#if 0 /*KR:T*/
                You("strain your %s, but you're still %s.",
                    makeplural(body_part(LEG)),
                    (u.utraptype == TT_INFLOOR)
                        ? "stuck in the floor"
                        : "attached to the buried ball");
#else
                You("%s를 당겼지만, 당신은 아직 %s.",
                    makeplural(body_part(LEG)),
                    (u.utraptype == TT_INFLOOR)
                    ? "바닥에 끼어 있다"
                    : "파묻힌 공에 연결되어 있다");
#endif
                set_wounded_legs(LEFT_SIDE, rn1(10, 11));
                set_wounded_legs(RIGHT_SIDE, rn1(10, 11));
                return 1;
            }

        /*
         * Check the path from uc to cc, calling hurtle_step at each
         * location.  The final position actually reached will be
         * in cc.
         */
        uc.x = u.ux;
        uc.y = u.uy;
        /* calculate max(abs(dx), abs(dy)) as the range */
        range = cc.x - uc.x;
        if (range < 0)
            range = -range;
        temp = cc.y - uc.y;
        if (temp < 0)
            temp = -temp;
        if (range < temp)
            range = temp;
        (void) walk_path(&uc, &cc, hurtle_jump, (genericptr_t) &range);
        /* hurtle_jump -> hurtle_step results in <u.ux,u.uy> == <cc.x,cc.y>
         * and usually moves the ball if punished, but does not handle all
         * the effects of landing on the final position.
         */
        teleds(cc.x, cc.y, FALSE);
        sokoban_guilt();
        nomul(-1);
        /*KR multi_reason = "jumping around"; */
        multi_reason = "뛰어다니고 있을 때";
        nomovemsg = "";
        morehungry(rnd(25));
        return 1;
    }
}

boolean
tinnable(corpse)
struct obj *corpse;
{
    if (corpse->oeaten)
        return 0;
    if (!mons[corpse->corpsenm].cnutrit)
        return 0;
    return 1;
}

STATIC_OVL void
use_tinning_kit(obj)
struct obj *obj;
{
    struct obj *corpse, *can;

    /* This takes only 1 move.  If this is to be changed to take many
     * moves, we've got to deal with decaying corpses...
     */
    if (obj->spe <= 0) {
        /*KR You("seem to be out of tins."); */
        pline("통조림을 만들기 위한 깡통이 다 떨어진 듯 하다.");
        return;
    }
    if (!(corpse = floorfood("tin", 2)))
        return;
    if (corpse->oeaten) {
        /*KR You("cannot tin %s which is partly eaten.", something); */
        You("먹다 남은 걸 통조림으로 만들 수는 없다.");
        return;
    }
    if (touch_petrifies(&mons[corpse->corpsenm]) && !Stone_resistance
        && !uarmg) {
        char kbuf[BUFSZ];

        if (poly_when_stoned(youmonst.data))
#if 0 /*KR:T*/
            You("tin %s without wearing gloves.",
                an(mons[corpse->corpsenm].mname));
#else
            You("장갑을 끼지 않고 %s을/를 통조림으로 만들려고 했다.",
                mons[corpse->corpsenm].mname);
#endif
        else {
#if 0 /*KR:T*/
            pline("Tinning %s without wearing gloves is a fatal mistake...",
                  an(mons[corpse->corpsenm].mname));
#else
            pline("장갑을 끼지 않고 %s을/를 통조림으로 만드는 건 아주 치명적인 실수다...",
                mons[corpse->corpsenm].mname);
#endif
#if 0 /*KR:T*/
            Sprintf(kbuf, "trying to tin %s without gloves",
                    mons[corpse->corpsenm].mname));
#else 
            Sprintf(kbuf, "장갑 없이 %s을/를 통조림으로 만들려고 하다가",
                mons[corpse->corpsenm].mname);
#endif
        }
        instapetrify(kbuf);
    }
    if (is_rider(&mons[corpse->corpsenm])) {
        if (revive_corpse(corpse))
            /*KR verbalize("Yes...  But War does not preserve its enemies..."); */
            verbalize("그래... 하지만 <전쟁>한테는 적을 살려보낸다는 게 없지...");
        else
            /*KR pline_The("corpse evades your grasp."); */
            pline("시체가 당신의 손길을 피했다.");
        return;
    }
    if (mons[corpse->corpsenm].cnutrit == 0) {
        /*KR pline("That's too insubstantial to tin."); */
        pline("통조림으로 만들기에는 실체가 없는 걸.");
        return;
    }
    consume_obj_charge(obj, TRUE);

    if ((can = mksobj(TIN, FALSE, FALSE)) != 0) {
        /*KR static const char you_buy_it[] = "You tin it, you bought it!"; */
        static const char you_buy_it[] = "통조림으로 만들었으니, 물어 내셔야지!";

        can->corpsenm = corpse->corpsenm;
        can->cursed = obj->cursed;
        can->blessed = obj->blessed;
        can->owt = weight(can);
        can->known = 1;
        /* Mark tinned tins. No spinach allowed... */
        set_tin_variety(can, HOMEMADE_TIN);
        if (carried(corpse)) {
            if (corpse->unpaid)
                verbalize(you_buy_it);
            useup(corpse);
        } else {
            if (costly_spot(corpse->ox, corpse->oy) && !corpse->no_charge)
                verbalize(you_buy_it);
            useupf(corpse, 1L);
        }
#if 0 /*KR:T*/
        (void) hold_another_object(can, "You make, but cannot pick up, %s.",
                                   doname(can), (const char *) 0);
#else
        (void)hold_another_object(can, "통조림으로 만들었지만, %s을/를 집어들 수는 없었다.",
                                   doname(can), (const char*)0);
#endif
    } else
        impossible("Tinning failed.");
}

void
use_unicorn_horn(obj)
struct obj *obj;
{
#define PROP_COUNT 7           /* number of properties we're dealing with */
#define ATTR_COUNT (A_MAX * 3) /* number of attribute points we might fix */
    int idx, val, val_limit, trouble_count, unfixable_trbl, did_prop,
        did_attr;
    int trouble_list[PROP_COUNT + ATTR_COUNT];

    if (obj && obj->cursed) {
        long lcount = (long) rn1(90, 10);

        switch (rn2(13) / 2) { /* case 6 is half as likely as the others */
        case 0:
            make_sick((Sick & TIMEOUT) ? (Sick & TIMEOUT) / 3L + 1L
                                       : (long) rn1(ACURR(A_CON), 20),
                      xname(obj), TRUE, SICK_NONVOMITABLE);
            break;
        case 1:
            make_blinded((Blinded & TIMEOUT) + lcount, TRUE);
            break;
        case 2:
            if (!Confusion)
#if 0 /*KR:T*/
                You("suddenly feel %s.",
                    Hallucination ? "trippy" : "confused");
#else
                You("갑자기 %s.",
                    Hallucination ? "몽롱해졌다" : "혼란스러워졌다");
#endif
            make_confused((HConfusion & TIMEOUT) + lcount, TRUE);
            break;
        case 3:
            make_stunned((HStun & TIMEOUT) + lcount, TRUE);
            break;
        case 4:
            (void) adjattrib(rn2(A_MAX), -1, FALSE);
            break;
        case 5:
            (void) make_hallucinated((HHallucination & TIMEOUT) + lcount,
                                     TRUE, 0L);
            break;
        case 6:
            if (Deaf) /* make_deaf() won't give feedback when already deaf */
                /*KR pline("Nothing seems to happen."); */
                pline("아무것도 일어나지 않은 것 같다.");
            make_deaf((HDeaf & TIMEOUT) + lcount, TRUE);
            break;
        }
        return;
    }

/*
 * Entries in the trouble list use a very simple encoding scheme.
 */
#define prop2trbl(X) ((X) + A_MAX)
#define attr2trbl(Y) (Y)
#define prop_trouble(X) trouble_list[trouble_count++] = prop2trbl(X)
#define attr_trouble(Y) trouble_list[trouble_count++] = attr2trbl(Y)
#define TimedTrouble(P) (((P) && !((P) & ~TIMEOUT)) ? ((P) & TIMEOUT) : 0L)

    trouble_count = unfixable_trbl = did_prop = did_attr = 0;

    /* collect property troubles */
    if (TimedTrouble(Sick))
        prop_trouble(SICK);
    if (TimedTrouble(Blinded) > (long) u.ucreamed
        && !(u.uswallow
             && attacktype_fordmg(u.ustuck->data, AT_ENGL, AD_BLND)))
        prop_trouble(BLINDED);
    if (TimedTrouble(HHallucination))
        prop_trouble(HALLUC);
    if (TimedTrouble(Vomiting))
        prop_trouble(VOMITING);
    if (TimedTrouble(HConfusion))
        prop_trouble(CONFUSION);
    if (TimedTrouble(HStun))
        prop_trouble(STUNNED);
    if (TimedTrouble(HDeaf))
        prop_trouble(DEAF);

    unfixable_trbl = unfixable_trouble_count(TRUE);

    /* collect attribute troubles */
    for (idx = 0; idx < A_MAX; idx++) {
        if (ABASE(idx) >= AMAX(idx))
            continue;
        val_limit = AMAX(idx);
        /* this used to adjust 'val_limit' for A_STR when u.uhs was
           WEAK or worse, but that's handled via ATEMP(A_STR) now */
        if (Fixed_abil) {
            /* potion/spell of restore ability override sustain ability
               intrinsic but unicorn horn usage doesn't */
            unfixable_trbl += val_limit - ABASE(idx);
            continue;
        }
        /* don't recover more than 3 points worth of any attribute */
        if (val_limit > ABASE(idx) + 3)
            val_limit = ABASE(idx) + 3;

        for (val = ABASE(idx); val < val_limit; val++)
            attr_trouble(idx);
        /* keep track of unfixed trouble, for message adjustment below */
        unfixable_trbl += (AMAX(idx) - val_limit);
    }

    if (trouble_count == 0) {
        pline1(nothing_happens);
        return;
    } else if (trouble_count > 1) { /* shuffle */
        int i, j, k;

        for (i = trouble_count - 1; i > 0; i--)
            if ((j = rn2(i + 1)) != i) {
                k = trouble_list[j];
                trouble_list[j] = trouble_list[i];
                trouble_list[i] = k;
            }
    }

    /*
     *  Chances for number of troubles to be fixed
     *               0      1      2      3      4      5      6      7
     *   blessed:  22.7%  22.7%  19.5%  15.4%  10.7%   5.7%   2.6%   0.8%
     *  uncursed:  35.4%  35.4%  22.9%   6.3%    0      0      0      0
     */
    val_limit = rn2(d(2, (obj && obj->blessed) ? 4 : 2));
    if (val_limit > trouble_count)
        val_limit = trouble_count;

    /* fix [some of] the troubles */
    for (val = 0; val < val_limit; val++) {
        idx = trouble_list[val];

        switch (idx) {
        case prop2trbl(SICK):
            make_sick(0L, (char *) 0, TRUE, SICK_ALL);
            did_prop++;
            break;
        case prop2trbl(BLINDED):
            make_blinded((long) u.ucreamed, TRUE);
            did_prop++;
            break;
        case prop2trbl(HALLUC):
            (void) make_hallucinated(0L, TRUE, 0L);
            did_prop++;
            break;
        case prop2trbl(VOMITING):
            make_vomiting(0L, TRUE);
            did_prop++;
            break;
        case prop2trbl(CONFUSION):
            make_confused(0L, TRUE);
            did_prop++;
            break;
        case prop2trbl(STUNNED):
            make_stunned(0L, TRUE);
            did_prop++;
            break;
        case prop2trbl(DEAF):
            make_deaf(0L, TRUE);
            did_prop++;
            break;
        default:
            if (idx >= 0 && idx < A_MAX) {
                ABASE(idx) += 1;
                did_attr++;
            } else
                panic("use_unicorn_horn: bad trouble? (%d)", idx);
            break;
        }
    }

    if (did_attr || did_prop)
        context.botl = TRUE;
    if (did_attr)
#if 0 /*KR:T*/
        pline("This makes you feel %s!",
              (did_prop + did_attr) == (trouble_count + unfixable_trbl)
                  ? "great"
                  : "better");
#else
        pline("기분이 %s 좋아졌다!",
            (did_prop + did_attr) == (trouble_count + unfixable_trbl)
            ? "엄청"
            : "한결");
#endif
    else if (!did_prop)
        /*KR pline("Nothing seems to happen."); */
        pline("아무것도 일어나지 않은 것 같다.");

#undef PROP_COUNT
#undef ATTR_COUNT
#undef prop2trbl
#undef attr2trbl
#undef prop_trouble
#undef attr_trouble
#undef TimedTrouble
}

/*
 * Timer callback routine: turn figurine into monster
 */
void
fig_transform(arg, timeout)
anything *arg;
long timeout;
{
    struct obj *figurine = arg->a_obj;
    struct monst *mtmp;
    coord cc;
    boolean cansee_spot, silent, okay_spot;
    boolean redraw = FALSE;
    boolean suppress_see = FALSE;
    char monnambuf[BUFSZ], carriedby[BUFSZ];

    if (!figurine) {
        debugpline0("null figurine in fig_transform()");
        return;
    }
    silent = (timeout != monstermoves); /* happened while away */
    okay_spot = get_obj_location(figurine, &cc.x, &cc.y, 0);
    if (figurine->where == OBJ_INVENT || figurine->where == OBJ_MINVENT)
        okay_spot = enexto(&cc, cc.x, cc.y, &mons[figurine->corpsenm]);
    if (!okay_spot || !figurine_location_checks(figurine, &cc, TRUE)) {
        /* reset the timer to try again later */
        (void) start_timer((long) rnd(5000), TIMER_OBJECT, FIG_TRANSFORM,
                           obj_to_any(figurine));
        return;
    }

    cansee_spot = cansee(cc.x, cc.y);
    mtmp = make_familiar(figurine, cc.x, cc.y, TRUE);
    if (mtmp) {
        char and_vanish[BUFSZ];
        struct obj *mshelter = level.objects[mtmp->mx][mtmp->my];

        /* [m_monnam() yields accurate mon type, overriding hallucination] */
        Sprintf(monnambuf, "%s", an(m_monnam(mtmp)));
        /*KR: and_vanish를 처리하지 않음*/
        and_vanish[0] = '\0';
        if ((mtmp->minvis && !See_invisible)
            || (mtmp->data->mlet == S_MIMIC
                && M_AP_TYPE(mtmp) != M_AP_NOTHING))
            suppress_see = TRUE;

        if (mtmp->mundetected) {
            if (hides_under(mtmp->data) && mshelter) {
                Sprintf(and_vanish, " and %s under %s",
                        locomotion(mtmp->data, "crawl"), doname(mshelter));
            } else if (mtmp->data->mlet == S_MIMIC
                       || mtmp->data->mlet == S_EEL) {
                suppress_see = TRUE;
            } else
                Strcpy(and_vanish, " and vanish");
        }

        switch (figurine->where) {
        case OBJ_INVENT:
            if (Blind || suppress_see)
#if 0 /*KR:T*/
                You_feel("%s %s from your pack!", something,
                         locomotion(mtmp->data, "drop"));
#else
                You_feel("%s이/가 당신의 가방에서 %s 것 같다!", something,
                    locomotion(mtmp->data, "떨어진"));
#endif
            else
             /*KR   You_see("%s %s out of your pack%s!", monnambuf,
                        locomotion(mtmp->data, "drop"), and_vanish); */
                You_see("%s이/가 당신의 가방에서 %s 것을 보았다!", monnambuf,
                    locomotion(mtmp->data, "떨어지는"));
            /* jpast(locomotion(mtmp->data, "떨어지는 것을"))); */
            /* jpast - extern.h 에서 선언한 특수 함수. 수정필요 */
            break;

        case OBJ_FLOOR:
            if (cansee_spot && !silent) {
                if (suppress_see)
                    /*KR pline("%s suddenly vanishes!", an(xname(figurine))); */
                    pline("%s이/가 갑자기 사라졌다!", xname(figurine));
                else
#if 0 /*KR*/
                    You_see("a figurine transform into %s%s!", monnambuf,
                            and_vanish);
#else
                    You("모형이 %s%s로 변했다!", monnambuf);
#endif
                redraw = TRUE; /* update figurine's map location */
            }
            break;

        case OBJ_MINVENT:
            if (cansee_spot && !silent && !suppress_see) {
                struct monst *mon;

                mon = figurine->ocarry;
                /* figurine carrying monster might be invisible */
                if (canseemon(figurine->ocarry)
                    && (!mon->wormno || cansee(mon->mx, mon->my)))
                    /*KR Sprintf(carriedby, "%s pack", s_suffix(a_monnam(mon))); */
                    Sprintf(carriedby, "%s의 가방", a_monnam(mon));
                else if (is_pool(mon->mx, mon->my))
                    /*KR Strcpy(carriedby, "empty water"); */
                    Strcpy(carriedby, "텅 빈 물속");
                else
                    /*KR Strcpy(carriedby, "thin air"); */
                    Strcpy(carriedby, "텅 빈 공중");
#if 0 /*KR*/
                You_see("%s %s out of %s%s!", monnambuf,
                        locomotion(mtmp->data, "drop"), carriedby,
                        and_vanish);
#else
                You("%s이/가 %s에서 %s 것을 본다!", monnambuf,
                    carriedby, locomotion(mtmp->data, "떨어지는"));
#endif                    
            }
            break;
#if 0
        case OBJ_MIGRATING:
            break;
#endif

        default:
            impossible("figurine came to life where? (%d)",
                       (int) figurine->where);
            break;
        }
    }
    /* free figurine now */
    if (carried(figurine)) {
        useup(figurine);
    } else {
        obj_extract_self(figurine);
        obfree(figurine, (struct obj *) 0);
    }
    if (redraw)
        newsym(cc.x, cc.y);
}

STATIC_OVL boolean
figurine_location_checks(obj, cc, quietly)
struct obj *obj;
coord *cc;
boolean quietly;
{
    xchar x, y;

    if (carried(obj) && u.uswallow) {
        if (!quietly)
            /*KR You("don't have enough room in here."); */
            pline("여기에는 충분한 공간이 없다.");
        return FALSE;
    }
    x = cc ? cc->x : u.ux;
    y = cc ? cc->y : u.uy;
    if (!isok(x, y)) {
        if (!quietly)
            /*KR You("cannot put the figurine there."); */
            You("그 곳에는 모형을 둘 수 없다.");
        return FALSE;
    }
    if (IS_ROCK(levl[x][y].typ)
        && !(passes_walls(&mons[obj->corpsenm]) && may_passwall(x, y))) {
        if (!quietly)
#if 0 /*KR:T*/
            You("cannot place a figurine in %s!",
                IS_TREE(levl[x][y].typ) ? "a tree" : "solid rock");
#else
            You("%s 안에 모형을 둘 수는 없다!",
                IS_TREE(levl[x][y].typ) ? "나무" : "단단한 돌");
#endif
        return FALSE;
    }
    if (sobj_at(BOULDER, x, y) && !passes_walls(&mons[obj->corpsenm])
        && !throws_rocks(&mons[obj->corpsenm])) {
        if (!quietly)
            /*KR You("cannot fit the figurine on the boulder."); */
            You("바위에 모형을 끼워넣을 수는 없다.");
        return FALSE;
    }
    return TRUE;
}

STATIC_OVL void
use_figurine(optr)
struct obj **optr;
{
    register struct obj *obj = *optr;
    xchar x, y;
    coord cc;

    if (u.uswallow) {
        /* can't activate a figurine while swallowed */
        if (!figurine_location_checks(obj, (coord *) 0, FALSE))
            return;
    }
    if (!getdir((char *) 0)) {
        context.move = multi = 0;
        return;
    }
    x = u.ux + u.dx;
    y = u.uy + u.dy;
    cc.x = x;
    cc.y = y;
    /* Passing FALSE arg here will result in messages displayed */
    if (!figurine_location_checks(obj, &cc, FALSE))
        return;
#if 0 /*KR:T*/
    You("%s and it %stransforms.",
        (u.dx || u.dy) ? "set the figurine beside you"
                       : (Is_airlevel(&u.uz) || Is_waterlevel(&u.uz)
                          || is_pool(cc.x, cc.y))
                             ? "release the figurine"
                             : (u.dz < 0 ? "toss the figurine into the air"
                                         : "set the figurine on the ground"),
        Blind ? "supposedly " : "");
#else
    You("%s. 그러자 그것이 변신했다.",
        (u.dx || u.dy) ? "옆에 모형을 두었다"
                       : (Is_airlevel(&u.uz) || Is_waterlevel(&u.uz)
                          || is_pool(cc.x, cc.y))
                             ? "모형을 놓았다"
                             : (u.dz < 0 ? "모형을 공중에 던졌다"
                                         : "바닥에 모형을 두었다"));
#endif
    (void) make_familiar(obj, cc.x, cc.y, FALSE);
    (void) stop_timer(FIG_TRANSFORM, obj_to_any(obj));
    useup(obj);
    if (Blind)
        map_invisible(cc.x, cc.y);
    *optr = 0;
}

static NEARDATA const char lubricables[] = { ALL_CLASSES, ALLOW_NONE, 0 };

STATIC_OVL void
use_grease(obj)
struct obj *obj;
{
    struct obj *otmp;

    if (Glib) {
#if 0 /*KR:T*/
        pline("%s from your %s.", Tobjnam(obj, "slip"),
              fingers_or_gloves(FALSE));
#else
        pline("%s이/가 당신의 %s에서 미끄러져 떨어졌다.", xname(obj),
            fingers_or_gloves(FALSE));
#endif
        dropx(obj);
        return;
    }

    if (obj->spe > 0) {
        int oldglib;

        if ((obj->cursed || Fumbling) && !rn2(2)) {
            consume_obj_charge(obj, TRUE);

#if 0 /*KR:T*/
            pline("%s from your %s.", Tobjnam(obj, "slip"),
                  fingers_or_gloves(FALSE));
#else
            pline("%s이/가 당신의 %s에서 미끄러져 떨어졌다.", xname(obj),
                fingers_or_gloves(FALSE));
#endif
            dropx(obj);
            return;
        }
        otmp = getobj(lubricables, "grease");
        if (!otmp)
            return;
        /*KR if (inaccessible_equipment(otmp, "grease", FALSE)) */
        if (inaccessible_equipment(otmp, "에 기름을 바른", FALSE))
            return;
        consume_obj_charge(obj, TRUE);

        oldglib = (int) (Glib & TIMEOUT);
        if (otmp != &zeroobj) {
            /*KR You("cover %s with a thick layer of grease.", yname(otmp)); */
            You("%s에 윤활유를 넉넉히 발랐다.", xname(otmp));
            otmp->greased = 1;
            if (obj->cursed && !nohands(youmonst.data)) {
                make_glib(oldglib + rn1(6, 10)); /* + 10..15 */
                /*KR pline("Some of the grease gets all over your %s.", */
                pline("윤활유가 당신의 %s에 온통 묻었다.",
                      fingers_or_gloves(TRUE));
            }
        } else {
            make_glib(oldglib + rn1(11, 5)); /* + 5..15 */
            /*KR You("coat your %s with grease.", fingers_or_gloves(TRUE)); */
            You("%s에 윤활유를 씌웠다.", fingers_or_gloves(TRUE));
        }
    } else {
        if (obj->known)
            /*KR pline("%s empty.", Tobjnam(obj, "are")); */
            pline("%s는 텅 비었다.", xname(obj));
        else
            /*KR pline("%s to be empty.", Tobjnam(obj, "seem")); */
            pline("%s는 텅 빈 것 같다.", xname(obj));
    }
    update_inventory();
}

/* touchstones - by Ken Arnold */
STATIC_OVL void
use_stone(tstone)
struct obj *tstone;
{
    /*KR static const char scritch[] = "\"scritch, scritch\""; */
    static const char scritch[] = "\"끼기긱, 끼기긱\"";
    static const char allowall[3] = { COIN_CLASS, ALL_CLASSES, 0 };
    static const char coins_gems[3] = { COIN_CLASS, GEM_CLASS, 0 };
    struct obj *obj;
    boolean do_scratch;
    const char *streak_color, *choices;
    char stonebuf[QBUFSZ];
    int oclass;

    /* in case it was acquired while blinded */
    if (!Blind)
        tstone->dknown = 1;
    /* when the touchstone is fully known, don't bother listing extra
       junk as likely candidates for rubbing */
    choices = (tstone->otyp == TOUCHSTONE && tstone->dknown
               && objects[TOUCHSTONE].oc_name_known)
                  ? coins_gems
                  : allowall;
    /*KR Sprintf(stonebuf, "rub on the stone%s", plur(tstone->quan)); */
    Sprintf(stonebuf, "rub on the stone"); 
    if ((obj = getobj(choices, stonebuf)) == 0)
        return;

    if (obj == tstone && obj->quan == 1L) {
        /*KR You_cant("rub %s on itself.", the(xname(obj))); */
        You("%s을/를 그 자체에 문지를 수는 없다.", the(xname(obj)));
        return;
    }

    if (tstone->otyp == TOUCHSTONE && tstone->cursed
        && obj->oclass == GEM_CLASS && !is_graystone(obj)
        && !obj_resists(obj, 80, 100)) {
        if (Blind)
            /*KR pline("You feel something shatter."); */
            You("무언가가 산산조각이 났음을 느꼈다.");
        else if (Hallucination)
            /*KR pline("Oh, wow, look at the pretty shards."); */
            pline("우와아! 정말 아름다운 조각들인걸?");
        else
#if 0 /*KR:T*/
            pline("A sharp crack shatters %s%s.",
                  (obj->quan > 1L) ? "one of " : "", the(xname(obj)));
#else
            pline("표면에 선명한 금이 가서 %s %s가 산산조각이 났다.",
                the(xname(obj)), (obj->quan > 1L) ? "하나" : "");
#endif
        useup(obj);
        return;
    }

    if (Blind) {
        pline(scritch);
        return;
    } else if (Hallucination) {
#if 0 /*KR:T*/
        pline("Oh wow, man: Fractals!");
#else
        pline("우와아, 프랙탈 무늬다!");
#endif
        return;
    }

    do_scratch = FALSE;
    streak_color = 0;

    oclass = obj->oclass;
    /* prevent non-gemstone rings from being treated like gems */
    if (oclass == RING_CLASS
        && objects[obj->otyp].oc_material != GEMSTONE
        && objects[obj->otyp].oc_material != MINERAL)
        oclass = RANDOM_CLASS; /* something that's neither gem nor ring */

    switch (oclass) {
    case GEM_CLASS: /* these have class-specific handling below */
    case RING_CLASS:
        if (tstone->otyp != TOUCHSTONE) {
            do_scratch = TRUE;
        } else if (obj->oclass == GEM_CLASS
                   && (tstone->blessed
                       || (!tstone->cursed && (Role_if(PM_ARCHEOLOGIST)
                                               || Race_if(PM_GNOME))))) {
            makeknown(TOUCHSTONE);
            makeknown(obj->otyp);
            prinv((char *) 0, obj, 0L);
            return;
        } else {
            /* either a ring or the touchstone was not effective */
            if (objects[obj->otyp].oc_material == GLASS) {
                do_scratch = TRUE;
                break;
            }
        }
        streak_color = c_obj_colors[objects[obj->otyp].oc_color];
        break; /* gem or ring */

    default:
        switch (objects[obj->otyp].oc_material) {
        case CLOTH:
#if 0 /*KR:T*/
            pline("%s a little more polished now.", Tobjnam(tstone, "look"));
#else
            pline("%s이 조금 더 윤이 나게 되었다.", xname(tstone));
#endif
            return;
        case LIQUID:
            if (!obj->known) /* note: not "whetstone" */
#if 0 /*KR:T*/
                You("must think this is a wetstone, do you?");
#else
                You("이게 숫돌이라고 생각했니?");
#endif
            else
#if 0 /*KR:T*/
                pline("%s a little wetter now.", Tobjnam(tstone, "are"));
#else
                pline("%s은 조금 젖었다.", xname(tstone));
#endif
            return;
        case WAX:
#if 0 /*KR:T*/
            streak_color = "waxy";
#else
            streak_color = "밀랍색의";
#endif
            break; /* okay even if not touchstone */
        case WOOD:
#if 0 /*KR:T*/
            streak_color = "wooden";
#else
            streak_color = "나무색의";
#endif
            break; /* okay even if not touchstone */
        case GOLD:
            do_scratch = TRUE; /* scratching and streaks */
#if 0 /*KR:T*/
            streak_color = "golden";
#else
            streak_color = "금색의";
#endif
            break;
        case SILVER:
            do_scratch = TRUE; /* scratching and streaks */
#if 0 /*KR:T*/
            streak_color = "silvery";
#else
            streak_color = "은색의";
#endif
            break;
        default:
            /* Objects passing the is_flimsy() test will not
               scratch a stone.  They will leave streaks on
               non-touchstones and touchstones alike. */
            if (is_flimsy(obj))
                streak_color = c_obj_colors[objects[obj->otyp].oc_color];
            else
                do_scratch = (tstone->otyp != TOUCHSTONE);
            break;
        }
        break; /* default oclass */
    }

#if 0 /*KR*/ /* 미사용 */
    Sprintf(stonebuf, "stone%s", plur(tstone->quan));
#endif
    if (do_scratch)
#if 0 /*KR:T*/
        You("make %s%sscratch marks on the %s.",
            streak_color ? streak_color : (const char *)"",
            streak_color ? " " : "", stonebuf);
#else
        You("돌에 %s 긁은 자국을 냈다.",
            streak_color ? streak_color : (const char *)"");
#endif
    else if (streak_color)
        /*KR You_see("%s streaks on the %s.", streak_color, stonebuf); */
        pline("돌에 %s 자국을 냈다.", streak_color);
    else
        pline(scritch);
    return;
}

static struct trapinfo {
    struct obj *tobj;
    xchar tx, ty;
    int time_needed;
    boolean force_bungle;
} trapinfo;

void
reset_trapset()
{
    trapinfo.tobj = 0;
    trapinfo.force_bungle = 0;
}

/* Place a landmine/bear trap.  Helge Hafting */
STATIC_OVL void
use_trap(otmp)
struct obj *otmp;
{
    int ttyp, tmp;
    const char *what = (char *) 0;
    char buf[BUFSZ];
    int levtyp = levl[u.ux][u.uy].typ;
#if 0 /*KR:T*/
    const char *occutext = "setting the trap";
#else
    const char* occutext = "함정을 설치하고 있다";
#endif

    if (nohands(youmonst.data))
        /*KR what = "without hands"; */
        what = "손이 없어서";
    else if (Stunned)
        /*KR what = "while stunned"; */
        what = "기절해 있어서";
    else if (u.uswallow)
        what = 
        /*KR is_animal(u.ustuck->data) ? "while swallowed" : "while engulfed"; */
            is_animal(u.ustuck->data) ? "삼켜진 동안에는" : "빨아들여진 동안에는";
    else if (Underwater)
        /*KR what = "underwater"; */
        what = "수면 아래에서는";
    else if (Levitation)
        /*KR what = "while levitating"; */
        what = "공중에 떠 있는 동안에는";
    else if (is_pool(u.ux, u.uy))
        /*KR what = "in water"; */
        what = "물 속에서는";
    else if (is_lava(u.ux, u.uy))
        /*KR what = "in lava"; */
        what = "용암 속에서는";
    else if (On_stairs(u.ux, u.uy))
#if 0 /*KR:T*/
        what = (u.ux == xdnladder || u.ux == xupladder) ? "on the ladder"
                                                        : "on the stairs";
#else
        what = (u.ux == xdnladder || u.ux == xupladder) ? "사다리 위에서는"
                                                        : "계단 위에서는";
#endif
    else if (IS_FURNITURE(levtyp) || IS_ROCK(levtyp)
             || closed_door(u.ux, u.uy) || t_at(u.ux, u.uy))
        /*KR what = "here"; */
        what = "이곳에는";
    else if (Is_airlevel(&u.uz) || Is_waterlevel(&u.uz))
#if 0 /*KR:T*/
        what = (levtyp == AIR)
                   ? "in midair"
                   : (levtyp == CLOUD)
                         ? "in a cloud"
                         : "in this place"; /* Air/Water Plane catch-all */
#else
        what = (levtyp == AIR)
                   ? "공중에서는"
                   : (levtyp == CLOUD)
                         ? "구름 속에서는"
                         : "이곳에서는"; /* Air/Water Plane catch-all */
#endif
    if (what) {
        /*KR You_cant("set a trap %s!", what); */
        pline("%s 함정을 설치할 수 없다!", what);
        reset_trapset();
        return;
    }
    ttyp = (otmp->otyp == LAND_MINE) ? LANDMINE : BEAR_TRAP;
    if (otmp == trapinfo.tobj && u.ux == trapinfo.tx && u.uy == trapinfo.ty) {
        /*KR You("resume setting %s%s.", shk_your(buf, otmp), */
        You("%s을/를 다시 설치하기 시작했다.",
            defsyms[trap_to_defsym(what_trap(ttyp, rn2))].explanation);
        set_occupation(set_trap, occutext, 0);
        return;
    }
    trapinfo.tobj = otmp;
    trapinfo.tx = u.ux, trapinfo.ty = u.uy;
    tmp = ACURR(A_DEX);
    trapinfo.time_needed =
        (tmp > 17) ? 2 : (tmp > 12) ? 3 : (tmp > 7) ? 4 : 5;
    if (Blind)
        trapinfo.time_needed *= 2;
    tmp = ACURR(A_STR);
    if (ttyp == BEAR_TRAP && tmp < 18)
        trapinfo.time_needed += (tmp > 12) ? 1 : (tmp > 7) ? 2 : 4;
    /*[fumbling and/or confusion and/or cursed object check(s)
       should be incorporated here instead of in set_trap]*/
    if (u.usteed && P_SKILL(P_RIDING) < P_BASIC) {
        boolean chance;

        if (Fumbling || otmp->cursed)
            chance = (rnl(10) > 3);
        else
            chance = (rnl(10) > 5);
        /*KR You("aren't very skilled at reaching from %s.", mon_nam(u.usteed)); */
        pline("%s의 위에서는 잘 다루지 못할 수도 있다.", mon_nam(u.usteed));
        /*KR Sprintf(buf, "Continue your attempt to set %s?", */
        Sprintf(buf, "%s을 계속 설치하시겠습니까?",
                the(defsyms[trap_to_defsym(what_trap(ttyp, rn2))]
                    .explanation));
        if (yn(buf) == 'y') {
            if (chance) {
                switch (ttyp) {
                case LANDMINE: /* set it off */
                    trapinfo.time_needed = 0;
                    trapinfo.force_bungle = TRUE;
                    break;
                case BEAR_TRAP: /* drop it without arming it */
                    reset_trapset();
                    /*KR You("drop %s!", */
                    You("%s을/를 떨어뜨렸다!",
                        the(defsyms[trap_to_defsym(what_trap(ttyp, rn2))]
                                .explanation));
                    dropx(otmp);
                    return;
                }
            }
        } else {
            reset_trapset();
            return;
        }
    }
    /*KR You("begin setting %s%s.", shk_your(buf, otmp), */
    You("%s%s을/를 설치하기 시작했다.", shk_your(buf, otmp),
        defsyms[trap_to_defsym(what_trap(ttyp, rn2))].explanation);
    set_occupation(set_trap, occutext, 0);
    return;
}

STATIC_PTR
int
set_trap()
{
    struct obj *otmp = trapinfo.tobj;
    struct trap *ttmp;
    int ttyp;

    if (!otmp || !carried(otmp) || u.ux != trapinfo.tx
        || u.uy != trapinfo.ty) {
        /* ?? */
        reset_trapset();
        return 0;
    }

    if (--trapinfo.time_needed > 0)
        return 1; /* still busy */

    ttyp = (otmp->otyp == LAND_MINE) ? LANDMINE : BEAR_TRAP;
    ttmp = maketrap(u.ux, u.uy, ttyp);
    if (ttmp) {
        ttmp->madeby_u = 1;
        feeltrap(ttmp);
        if (*in_rooms(u.ux, u.uy, SHOPBASE)) {
            add_damage(u.ux, u.uy, 0L); /* schedule removal */
        }
        if (!trapinfo.force_bungle)
            /*KR You("finish arming %s.", */
            You("%s을/를 설치하는 것을 마쳤다.",
                the(defsyms[trap_to_defsym(what_trap(ttyp, rn2))].explanation));
        if (((otmp->cursed || Fumbling) && (rnl(10) > 5))
            || trapinfo.force_bungle)
            dotrap(ttmp,
                   (unsigned) (trapinfo.force_bungle ? FORCEBUNGLE : 0));
    } else {
        /* this shouldn't happen */
        /*KR Your("trap setting attempt fails."); */
        You("함정 설치에 실패했다.");
    }
    useup(otmp);
    reset_trapset();
    return 0;
}

STATIC_OVL int
use_whip(obj)
struct obj *obj;
{
    char buf[BUFSZ];
    struct monst *mtmp;
    struct obj *otmp;
    int rx, ry, proficient, res = 0;
    /*KR const char *msg_slipsfree = "The bullwhip slips free."; */
    const char *msg_slipsfree = "채찍이 풀어졌다.";
    /*KR const char *msg_snap = "Snap!"; */
    const char *msg_snap = "찰싹!";

    if (obj != uwep) {
        if (!wield_tool(obj, "lash"))
            return 0;
        else
            res = 1;
    }
    if (!getdir((char *) 0))
        return res;

    if (u.uswallow) {
        mtmp = u.ustuck;
        rx = mtmp->mx;
        ry = mtmp->my;
    } else {
        if (Stunned || (Confusion && !rn2(5)))
            confdir();
        rx = u.ux + u.dx;
        ry = u.uy + u.dy;
        if (!isok(rx, ry)) {
            /*KR You("miss."); */
            You("빗나갔다.");
            return res;
        }
        mtmp = m_at(rx, ry);
    }

    /* fake some proficiency checks */
    proficient = 0;
    if (Role_if(PM_ARCHEOLOGIST))
        ++proficient;
    if (ACURR(A_DEX) < 6)
        proficient--;
    else if (ACURR(A_DEX) >= 14)
        proficient += (ACURR(A_DEX) - 14);
    if (Fumbling)
        --proficient;
    if (proficient > 3)
        proficient = 3;
    if (proficient < 0)
        proficient = 0;

    if (u.uswallow && attack(u.ustuck)) {
        /*KR There("is not enough room to flick your bullwhip."); */
        pline("채찍을 휘두를 충분한 공간이 없다.");

    } else if (Underwater) {
        /*KR There("is too much resistance to flick your bullwhip."); */
        pline("채찍질을 하기에는 물의 저항이 너무 심하다.");

    } else if (u.dz < 0) {
        /*KR You("flick a bug off of the %s.", ceiling(u.ux, u.uy)); */
        You("%s의 벌레를 쳐서 떨어뜨렸다.", ceiling(u.ux, u.uy));

    } else if ((!u.dx && !u.dy) || (u.dz > 0)) {
        int dam;

        /* Sometimes you hit your steed by mistake */
        if (u.usteed && !rn2(proficient + 2)) {
            /*KR You("whip %s!", mon_nam(u.usteed)); */
            You("%s을/를 채찍질했다!", mon_nam(u.usteed));
            kick_steed();
            return 1;
        }
        if (Levitation || u.usteed) {
            /* Have a shot at snaring something on the floor */
            otmp = level.objects[u.ux][u.uy];
            if (otmp && otmp->otyp == CORPSE && otmp->corpsenm == PM_HORSE) {
                /*KR pline("Why beat a dead horse?"); */
                pline("왜 죽은 말에 채찍질을 하고 그래?");
                return 1;
            }
            if (otmp && proficient) {
#if 0 /*KR:T*/
                You("wrap your bullwhip around %s on the %s.",
                    an(singular(otmp, xname)), surface(u.ux, u.uy));
#else
                You("채찍을 %s 위의 %s에 감았다.",
                    surface(u.ux, u.uy), an(singular(otmp, xname)));
#endif
                if (rnl(6) || pickup_object(otmp, 1L, TRUE) < 1)
                    pline1(msg_slipsfree);
                return 1;
            }
        }
        dam = rnd(2) + dbon() + obj->spe;
        if (dam <= 0)
            dam = 1;
        /*KR You("hit your %s with your bullwhip.", body_part(FOOT)); */
        You("자신의 %s을 채찍으로 내리쳤다.", body_part(FOOT));
#if 0 /*KR*/
        Sprintf(buf, "killed %sself with %s bullwhip", uhim(), uhis());
        losehp(Maybe_Half_Phys(dam), buf, NO_KILLER_PREFIX);
#else
        Sprintf(buf, "스스로를 채찍질해서"); 
        losehp(Maybe_Half_Phys(dam), buf, KILLED_BY);
#endif
        return 1;

    } else if ((Fumbling || Glib) && !rn2(5)) {
        /*KR pline_The("bullwhip slips out of your %s.", body_part(HAND)); */
        pline("채찍이 당신의 %s에서 미끄러져 떨어졌다.", body_part(HAND));
        dropx(obj);

    } else if (u.utrap && u.utraptype == TT_PIT) {
        /*
         * Assumptions:
         *
         * if you're in a pit
         *    - you are attempting to get out of the pit
         * or, if you are applying it towards a small monster
         *    - then it is assumed that you are trying to hit it
         * else if the monster is wielding a weapon
         *    - you are attempting to disarm a monster
         * else
         *    - you are attempting to hit the monster.
         *
         * if you're confused (and thus off the mark)
         *    - you only end up hitting.
         *
         */
        const char *wrapped_what = (char *) 0;

        if (mtmp) {
            if (bigmonst(mtmp->data)) {
                wrapped_what = strcpy(buf, mon_nam(mtmp));
            } else if (proficient) {
                if (attack(mtmp))
                    return 1;
                else
                    pline1(msg_snap);
            }
        }
        if (!wrapped_what) {
            if (IS_FURNITURE(levl[rx][ry].typ))
                wrapped_what = something;
            else if (sobj_at(BOULDER, rx, ry))
                /*KR wrapped_what = "a boulder"; */
                wrapped_what = "바위";
        }
        if (wrapped_what) {
            coord cc;

            cc.x = rx;
            cc.y = ry;
            /*KR You("wrap your bullwhip around %s.", wrapped_what); */
            You("채찍을 %s에 감았다.", wrapped_what);
            if (proficient && rn2(proficient + 2)) {
                if (!mtmp || enexto(&cc, rx, ry, youmonst.data)) {
                    /*KR You("yank yourself out of the pit!"); */
                    You("홱 잡아당겨서 구멍에서 빠져나왔다!");
                    teleds(cc.x, cc.y, TRUE);
                    reset_utrap(TRUE);
                    vision_full_recalc = 1;
                }
            } else {
                pline1(msg_slipsfree);
            }
            if (mtmp)
                wakeup(mtmp, TRUE);
        } else
            pline1(msg_snap);

    } else if (mtmp) {
        if (!canspotmon(mtmp) && !glyph_is_invisible(levl[rx][ry].glyph)) {
            /*KR pline("A monster is there that you couldn't see."); */
            pline("당신이 볼 수 없는 곳에 몬스터가 있다.");
            map_invisible(rx, ry);
        }
        otmp = MON_WEP(mtmp); /* can be null */
        if (otmp) {
            char onambuf[BUFSZ];
            const char *mon_hand;
            boolean gotit = proficient && (!Fumbling || !rn2(10));

            Strcpy(onambuf, cxname(otmp));
            if (gotit) {
                mon_hand = mbodypart(mtmp, HAND);
                if (bimanual(otmp))
                    mon_hand = makeplural(mon_hand);
            } else
                mon_hand = 0; /* lint suppression */

            /*KR You("wrap your bullwhip around %s.", yname(otmp)); */
            You("채찍을 %s에 감았다.", xname(otmp));
            if (gotit && mwelded(otmp)) {
#if 0 /*KR:T*/
                pline("%s welded to %s %s%c",
                      (otmp->quan == 1L) ? "It is" : "They are", mhis(mtmp),
                      mon_hand, !otmp->bknown ? '!' : '.');
#else
                pline("%s는 %s의 %s에 붙어 있는 %s",
                    onambuf, mon_nam(mtmp), mon_hand, 
                    !otmp->bknown ? "!" : ".");
#endif
                set_bknown(otmp, 1);
                gotit = FALSE; /* can't pull it free */
            }
            if (gotit) {
                obj_extract_self(otmp);
                possibly_unwield(mtmp, FALSE);
                setmnotwielded(mtmp, otmp);

                switch (rn2(proficient + 1)) {
                case 2:
                    /* to floor near you */
                    /*KR You("yank %s to the %s!", yname(otmp), */
                    You("%s를 %s로 끌어내렸다!", xname(otmp),
                        surface(u.ux, u.uy));
                    place_object(otmp, u.ux, u.uy);
                    stackobj(otmp);
                    break;
                case 3:
#if 0
                    /* right to you */
                    if (!rn2(25)) {
                        /* proficient with whip, but maybe not
                           so proficient at catching weapons */
                        int hitu, hitvalu;

                        hitvalu = 8 + otmp->spe;
                        hitu = thitu(hitvalu, dmgval(otmp, &youmonst),
                                     &otmp, (char *)0);
                        if (hitu) {
                       /*KR pline_The("%s hits you as you try to snatch it!", */
                       /*"당신은 그것을 뺏으려 하다가 %s에게 맞았다" 
                       가 옳은 번역이지만 일본어는 아래처럼 번역함 */
                            pline_The("%s를 뺏으려 하다가 당신에게 맞았다!",
                                      the(onambuf));
                        }
                        place_object(otmp, u.ux, u.uy);
                        stackobj(otmp);
                        break;
                    }
#endif /* 0 */
                    /* right into your inventory */
                    /*KR You("snatch %s!", yname(otmp)); */
                    You("%s을/를 빼앗았다!", xname(otmp));
                    if (otmp->otyp == CORPSE
                        && touch_petrifies(&mons[otmp->corpsenm]) && !uarmg
                        && !Stone_resistance
                        && !(poly_when_stoned(youmonst.data)
                             && polymon(PM_STONE_GOLEM))) {
                        char kbuf[BUFSZ];

#if 0 /*KR*/
                        Sprintf(kbuf, "%s corpse",
                                an(mons[otmp->corpsenm].mname));
                        pline("Snatching %s is a fatal mistake.", kbuf);
#else
                        pline("%s의 시체를 빼앗는 것은 치명적인 실수다.",
                            mons[otmp->corpsenm].mname);
                        Sprintf(kbuf, "%s의 시체를 만져서",
                            mons[otmp->corpsenm].mname);
#endif
                        instapetrify(kbuf);
                    }
#if 0 /*KR:T*/
                    (void) hold_another_object(otmp, "You drop %s!",
                                               doname(otmp), (const char *) 0);
#else
                    (void) hold_another_object(otmp, "%s을/를 떨어뜨렸다!",
                                               doname(otmp), (const char *) 0);
#endif
                    break;
                default:
                    /* to floor beneath mon */
#if 0 /*KR:T*/
                    You("yank %s from %s %s!", the(onambuf),
                        s_suffix(mon_nam(mtmp)), mon_hand);
#else
                    You("%s를 %s의 %s에서 홱 끌어당겼다!", the(xname(otmp)),
                        mon_nam(mtmp), mon_hand);
#endif
                    obj_no_longer_held(otmp);
                    place_object(otmp, mtmp->mx, mtmp->my);
                    stackobj(otmp);
                    break;
                }
            } else {
                pline1(msg_slipsfree);
            }
            wakeup(mtmp, TRUE);
        } else {
            if (M_AP_TYPE(mtmp) && !Protection_from_shape_changers
                && !sensemon(mtmp))
                stumble_onto_mimic(mtmp);
            else
                /*KR You("flick your bullwhip towards %s.", mon_nam(mtmp)); */
                You("%s을/를 향해서 채찍질했다.", mon_nam(mtmp));
            if (proficient) {
                if (attack(mtmp))
                    return 1;
                else
                    pline1(msg_snap);
            }
        }

    } else if (Is_airlevel(&u.uz) || Is_waterlevel(&u.uz)) {
        /* it must be air -- water checked above */
        /*KR You("snap your whip through thin air."); */
        You("옅은 공기를 가르고 채찍질한다.");

    } else {
        pline1(msg_snap);
    }
    return 1;
}

static const char
/*KR not_enough_room[] = "There's not enough room here to use that.",
    where_to_hit[] = "Where do you want to hit?",
    cant_see_spot[] = "won't hit anything if you can't see that spot.",
    cant_reach[] = "can't reach that spot from here."; */
    not_enough_room[] = "그걸 사용할 만한 충분한 공간이 없다.",
    where_to_hit[] = "어디를 공격하시겠습니까?",
    cant_see_spot[] = "그곳이 보이지 않으면 공격할 수 없습니다.",
    cant_reach[] = "여기서 그곳까지는 닿을 수 없습니다.";

/* find pos of monster in range, if only one monster */
STATIC_OVL boolean
find_poleable_mon(pos, min_range, max_range)
coord *pos;
int min_range, max_range;
{
    struct monst *mtmp;
    coord mpos;
    boolean impaired;
    int x, y, lo_x, hi_x, lo_y, hi_y, rt, glyph;

    if (Blind)
        return FALSE; /* must be able to see target location */
    impaired = (Confusion || Stunned || Hallucination);
    mpos.x = mpos.y = 0; /* no candidate location yet */
    rt = isqrt(max_range);
    lo_x = max(u.ux - rt, 1), hi_x = min(u.ux + rt, COLNO - 1);
    lo_y = max(u.uy - rt, 0), hi_y = min(u.uy + rt, ROWNO - 1);
    for (x = lo_x; x <= hi_x; ++x) {
        for (y = lo_y; y <= hi_y; ++y) {
            if (distu(x, y) < min_range || distu(x, y) > max_range
                || !isok(x, y) || !cansee(x, y))
                continue;
            glyph = glyph_at(x, y);
            if (!impaired
                && glyph_is_monster(glyph)
                && (mtmp = m_at(x, y)) != 0
                && (mtmp->mtame || (mtmp->mpeaceful && flags.confirm)))
                continue;
            if (glyph_is_monster(glyph)
                || glyph_is_warning(glyph)
                || glyph_is_invisible(glyph)
                || (glyph_is_statue(glyph) && impaired)) {
                if (mpos.x)
                    return FALSE; /* more than one candidate location */
                mpos.x = x, mpos.y = y;
            }
        }
    }
    if (!mpos.x)
        return FALSE; /* no candidate location */
    *pos = mpos;
    return TRUE;
}

static int polearm_range_min = -1;
static int polearm_range_max = -1;

STATIC_OVL boolean
get_valid_polearm_position(x, y)
int x, y;
{
    return (isok(x, y) && ACCESSIBLE(levl[x][y].typ)
            && distu(x, y) >= polearm_range_min
            && distu(x, y) <= polearm_range_max);
}

STATIC_OVL void
display_polearm_positions(state)
int state;
{
    if (state == 0) {
        tmp_at(DISP_BEAM, cmap_to_glyph(S_goodpos));
    } else if (state == 1) {
        int x, y, dx, dy;

        for (dx = -4; dx <= 4; dx++)
            for (dy = -4; dy <= 4; dy++) {
                x = dx + (int) u.ux;
                y = dy + (int) u.uy;
                if (get_valid_polearm_position(x, y)) {
                    tmp_at(x, y);
                }
            }
    } else {
        tmp_at(DISP_END, 0);
    }
}

/* Distance attacks by pole-weapons */
STATIC_OVL int
use_pole(obj)
struct obj *obj;
{
    int res = 0, typ, max_range, min_range, glyph;
    coord cc;
    struct monst *mtmp;
    struct monst *hitm = context.polearm.hitmon;

    /* Are you allowed to use the pole? */
    if (u.uswallow) {
        pline(not_enough_room);
        return 0;
    }
    if (obj != uwep) {
        if (!wield_tool(obj, "swing"))
            return 0;
        else
            res = 1;
    }
    /* assert(obj == uwep); */

    /*
     * Calculate allowable range (pole's reach is always 2 steps):
     *  unskilled and basic: orthogonal direction, 4..4;
     *  skilled: as basic, plus knight's jump position, 4..5;
     *  expert: as skilled, plus diagonal, 4..8.
     *      ...9...
     *      .85458.
     *      .52125.
     *      9410149
     *      .52125.
     *      .85458.
     *      ...9...
     *  (Note: no roles in nethack can become expert or better
     *  for polearm skill; Yeoman in slash'em can become expert.)
     */
    min_range = 4;
    typ = uwep_skill_type();
    if (typ == P_NONE || P_SKILL(typ) <= P_BASIC)
        max_range = 4;
    else if (P_SKILL(typ) == P_SKILLED)
        max_range = 5;
    else
        max_range = 8; /* (P_SKILL(typ) >= P_EXPERT) */

    polearm_range_min = min_range;
    polearm_range_max = max_range;

    /* Prompt for a location */
    pline(where_to_hit);
    cc.x = u.ux;
    cc.y = u.uy;
    if (!find_poleable_mon(&cc, min_range, max_range) && hitm
        && !DEADMONSTER(hitm) && cansee(hitm->mx, hitm->my)
        && distu(hitm->mx, hitm->my) <= max_range
        && distu(hitm->mx, hitm->my) >= min_range) {
        cc.x = hitm->mx;
        cc.y = hitm->my;
    }
    getpos_sethilite(display_polearm_positions, get_valid_polearm_position);
    /*KR if (getpos(&cc, TRUE, "the spot to hit") < 0) */
    if (getpos(&cc, TRUE, "공격할 지점") < 0)
        return res; /* ESC; uses turn iff polearm became wielded */

    glyph = glyph_at(cc.x, cc.y);
    if (distu(cc.x, cc.y) > max_range) {
        /*KR pline("Too far!"); */
        pline("너무 멀다!");
        return res;
    } else if (distu(cc.x, cc.y) < min_range) {
        /*KR pline("Too close!"); */
        pline("너무 가깝다!");
        return res;
    } else if (!cansee(cc.x, cc.y) && !glyph_is_monster(glyph)
               && !glyph_is_invisible(glyph) && !glyph_is_statue(glyph)) {
        You(cant_see_spot);
        return res;
    } else if (!couldsee(cc.x, cc.y)) { /* Eyes of the Overworld */
        You(cant_reach);
        return res;
    }

    context.polearm.hitmon = (struct monst *) 0;
    /* Attack the monster there */
    bhitpos = cc;
    if ((mtmp = m_at(bhitpos.x, bhitpos.y)) != (struct monst *) 0) {
        if (attack_checks(mtmp, uwep))
            return res;
        if (overexertion())
            return 1; /* burn nutrition; maybe pass out */
        context.polearm.hitmon = mtmp;
        check_caitiff(mtmp);
        notonhead = (bhitpos.x != mtmp->mx || bhitpos.y != mtmp->my);
        (void) thitmonst(mtmp, uwep);
    } else if (glyph_is_statue(glyph) /* might be hallucinatory */
               && sobj_at(STATUE, bhitpos.x, bhitpos.y)) {
        struct trap *t = t_at(bhitpos.x, bhitpos.y);

        if (t && t->ttyp == STATUE_TRAP
            && activate_statue_trap(t, t->tx, t->ty, FALSE)) {
            ; /* feedback has been give by animate_statue() */
        } else {
            /* Since statues look like monsters now, we say something
               different from "you miss" or "there's nobody there".
               Note:  we only do this when a statue is displayed here,
               because the player is probably attempting to attack it;
               other statues obscured by anything are just ignored. */
       /*KR pline("Thump!  Your blow bounces harmlessly off the statue."); */
            pline("쾅! 당신의 공격은 조각상에 아무런 피해도 입히지 못하고 튕겨나왔다.");
            wake_nearto(bhitpos.x, bhitpos.y, 25);
        }
    } else {
        /* no monster here and no statue seen or remembered here */
        (void) unmap_invisible(bhitpos.x, bhitpos.y);
        /*KR You("miss; there is no one there to hit."); */
        You("빗나갔다. 그곳에는 아무 것도 없다.");
    }
    u_wipe_engr(2); /* same as for melee or throwing */
    return 1;
}

STATIC_OVL int
use_cream_pie(obj)
struct obj *obj;
{
    boolean wasblind = Blind;
    boolean wascreamed = u.ucreamed;
    boolean several = FALSE;

    if (obj->quan > 1L) {
        several = TRUE;
        obj = splitobj(obj, 1L);
    }
    if (Hallucination)
        /*KR You("give yourself a facial."); */
        You("얼굴에 '크림팩'을 했다.");
    else
#if 0 /*KR:T*/
        pline("You immerse your %s in %s%s.", body_part(FACE),
              several ? "one of " : "",
              several ? makeplural(the(xname(obj))) : the(xname(obj)));
#else
        pline("%s %s에 %s를 담궜다.",
            xname(obj),
            several ? "하나" : "", body_part(FACE));
#endif
    if (can_blnd((struct monst *) 0, &youmonst, AT_WEAP, obj)) {
        int blindinc = rnd(25);
        u.ucreamed += blindinc;
        make_blinded(Blinded + (long) blindinc, FALSE);
        if (!Blind || (Blind && wasblind))
#if 0 /*KR:T*/
            pline("There's %ssticky goop all over your %s.",
                  wascreamed ? "more " : "", body_part(FACE));
#else
            pline("당신의 온 %s에 끈적끈적하고 들러붙는 것이 %s묻었다",
                body_part(FACE), wascreamed ? "더욱 " : "");
#endif
        else /* Blind  && !wasblind */
            /*KR You_cant("see through all the sticky goop on your %s.", */
            pline("온 %s의 끈적끈적하고 들러붙는 것 때문에 아무것도 볼 수 없다.",
                     body_part(FACE));
    }

    setnotworn(obj);
    /* useup() is appropriate, but we want costly_alteration()'s message */
    costly_alteration(obj, COST_SPLAT);
    obj_extract_self(obj);
    delobj(obj);
    return 0;
}

STATIC_OVL int
use_grapple(obj)
struct obj *obj;
{
    int res = 0, typ, max_range = 4, tohit;
    boolean save_confirm;
    coord cc;
    struct monst *mtmp;
    struct obj *otmp;

    /* Are you allowed to use the hook? */
    if (u.uswallow) {
        pline(not_enough_room);
        return 0;
    }
    if (obj != uwep) {
        if (!wield_tool(obj, "cast"))
            return 0;
        else
            res = 1;
    }
    /* assert(obj == uwep); */

    /* Prompt for a location */
    pline(where_to_hit);
    cc.x = u.ux;
    cc.y = u.uy;
    /*KR if (getpos(&cc, TRUE, "the spot to hit") < 0) */
    if (getpos(&cc, TRUE, "공격할 장소") < 0)
        return res; /* ESC; uses turn iff grapnel became wielded */

    /* Calculate range; unlike use_pole(), there's no minimum for range */
    typ = uwep_skill_type();
    if (typ == P_NONE || P_SKILL(typ) <= P_BASIC)
        max_range = 4;
    else if (P_SKILL(typ) == P_SKILLED)
        max_range = 5;
    else
        max_range = 8;
    if (distu(cc.x, cc.y) > max_range) {
        /*KR pline("Too far!"); */
        pline("너무 멀다!");
        return res;
    } else if (!cansee(cc.x, cc.y)) {
        You(cant_see_spot);
        return res;
    } else if (!couldsee(cc.x, cc.y)) { /* Eyes of the Overworld */
        You(cant_reach);
        return res;
    }

    /* What do you want to hit? */
    tohit = rn2(5);
    if (typ != P_NONE && P_SKILL(typ) >= P_SKILLED) {
        winid tmpwin = create_nhwindow(NHW_MENU);
        anything any;
        char buf[BUFSZ];
        menu_item *selected;

        any = zeroany; /* set all bits to zero */
        any.a_int = 1; /* use index+1 (cant use 0) as identifier */
        start_menu(tmpwin);
        any.a_int++;
        /*KR Sprintf(buf, "an object on the %s", surface(cc.x, cc.y)); */
        Sprintf(buf, "%s 위에 있는 물체", surface(cc.x, cc.y));
        add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf,
                 MENU_UNSELECTED);
        any.a_int++;
#if 0 /*KR:T*/
        add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, "a monster",
                 MENU_UNSELECTED);
#else
        add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, "몬스터",
            MENU_UNSELECTED);
#endif
        any.a_int++;
        /*KR Sprintf(buf, "the %s", surface(cc.x, cc.y)); */
        Sprintf(buf, "%s", surface(cc.x, cc.y));
        add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf,
                 MENU_UNSELECTED);
        /*KR end_menu(tmpwin, "Aim for what?"); */
        end_menu(tmpwin, "무엇을 노릴까요?");
        tohit = rn2(4);
        if (select_menu(tmpwin, PICK_ONE, &selected) > 0
            && rn2(P_SKILL(typ) > P_SKILLED ? 20 : 2))
            tohit = selected[0].item.a_int - 1;
        free((genericptr_t) selected);
        destroy_nhwindow(tmpwin);
    }

    /* possibly scuff engraving at your feet;
       any engraving at the target location is unaffected */
    if (tohit == 2 || !rn2(2))
        u_wipe_engr(rnd(2));

    /* What did you hit? */
    switch (tohit) {
    case 0: /* Trap */
        /* FIXME -- untrap needs to deal with non-adjacent traps */
        break;
    case 1: /* Object */
        if ((otmp = level.objects[cc.x][cc.y]) != 0) {
            /*KR You("snag an object from the %s!", surface(cc.x, cc.y)); */
            You("%s로부터 물체를 낚아챘다!", surface(cc.x, cc.y));
            (void) pickup_object(otmp, 1L, FALSE);
            /* If pickup fails, leave itgoop alone */
            newsym(cc.x, cc.y);
            return 1;
        }
        break;
    case 2: /* Monster */
        bhitpos = cc;
        if ((mtmp = m_at(cc.x, cc.y)) == (struct monst *) 0)
            break;
        notonhead = (bhitpos.x != mtmp->mx || bhitpos.y != mtmp->my);
        save_confirm = flags.confirm;
        if (verysmall(mtmp->data) && !rn2(4)
            && enexto(&cc, u.ux, u.uy, (struct permonst *) 0)) {
            flags.confirm = FALSE;
            (void) attack_checks(mtmp, uwep);
            flags.confirm = save_confirm;
            check_caitiff(mtmp); /* despite fact there's no damage */
            /*KR You("pull in %s!", mon_nam(mtmp)); */
            You("%s을/를 잡아당겼다!", mon_nam(mtmp));
            mtmp->mundetected = 0;
            rloc_to(mtmp, cc.x, cc.y);
            return 1;
        } else if ((!bigmonst(mtmp->data) && !strongmonst(mtmp->data))
                   || rn2(4)) {
            flags.confirm = FALSE;
            (void) attack_checks(mtmp, uwep);
            flags.confirm = save_confirm;
            check_caitiff(mtmp);
            (void) thitmonst(mtmp, uwep);
            return 1;
        }
    /*FALLTHRU*/
    case 3: /* Surface */
        if (IS_AIR(levl[cc.x][cc.y].typ) || is_pool(cc.x, cc.y))
            /*KR pline_The("hook slices through the %s.", surface(cc.x, cc.y)); */
            pline("갈고리가 %s을/를 갈랐다.", surface(cc.x, cc.y));
        else {
            /*KR You("are yanked toward the %s!", surface(cc.x, cc.y)); */
            You("%s로 휙 끌려갔다!", surface(cc.x, cc.y));
            hurtle(sgn(cc.x - u.ux), sgn(cc.y - u.uy), 1, FALSE);
            spoteffects(TRUE);
        }
        return 1;
    default: /* Yourself (oops!) */
        if (P_SKILL(typ) <= P_BASIC) {
            /*KR You("hook yourself!"); */
            You("스스로를 갈고리에 걸었다!");
            /*KR losehp(Maybe_Half_Phys(rn1(10, 10)), "a grappling hook", */
            losehp(Maybe_Half_Phys(rn1(10, 10)), "스스로를 갈고리에 걸어서",
                   KILLED_BY);
            return 1;
        }
        break;
    }
    pline1(nothing_happens);
    return 1;
}

#define BY_OBJECT ((struct monst *) 0)

/* return 1 if the wand is broken, hence some time elapsed */
STATIC_OVL int
do_break_wand(obj)
struct obj *obj;
{
    /*KR static const char nothing_else_happens[] = "But nothing else happens..."; */
    static const char nothing_else_happens[] = "하지만 아무것도 일어나지 않았다...";
    register int i, x, y;
    register struct monst *mon;
    int dmg, damage;
    boolean affects_objects;
    boolean shop_damage = FALSE;
    boolean fillmsg = FALSE;
    int expltype = EXPL_MAGICAL;
    char confirm[QBUFSZ], buf[BUFSZ];
    /*KR boolean is_fragile = (!strcmp(OBJ_DESCR(objects[obj->otyp]), "balsa")); */
    boolean is_fragile = (!strcmp(OBJ_DESCR(objects[obj->otyp]), "발사나무의 지팡이"));

#if 0 /*KR:T*/
    if (!paranoid_query(ParanoidBreakwand,
                       safe_qbuf(confirm,
                                 "Are you really sure you want to break ",
                                 "?", obj, yname, ysimple_name, "the wand")))
#else
    if (!paranoid_query(ParanoidBreakwand,
        safe_qbuf(confirm,
            "정말로 ", "를 부러뜨리시겠습니까?", 
            obj, xname, ysimple_name, "지팡이")))
#endif
        return 0;

    if (nohands(youmonst.data)) {
        /*KR You_cant("break %s without hands!", yname(obj)); */
        You_cant("손이 없이는 %s를 부러뜨릴 수 없다!", xname(obj));
        return 0;
    } else if (ACURR(A_STR) < (is_fragile ? 5 : 10)) {
        /*KR You("don't have the strength to break %s!", yname(obj)); */
        You("%s를 부러뜨릴 힘이 없다!", yname(obj));
        return 0;
    }
#if 0 /*KR:T*/
    pline("Raising %s high above your %s, you %s it in two!", yname(obj),
        body_part(HEAD), is_fragile ? "snap" : "break");
#else
    pline("%s를 %s 위로 높이 들어올려서, 반으로 부러뜨렸다!", yname(obj),
        body_part(HEAD));
#endif

    /* [ALI] Do this first so that wand is removed from bill. Otherwise,
     * the freeinv() below also hides it from setpaid() which causes problems.
     */
    if (obj->unpaid) {
        check_unpaid(obj); /* Extra charge for use */
        costly_alteration(obj, COST_DSTROY);
    }

    current_wand = obj; /* destroy_item might reset this */
    freeinv(obj);       /* hide it from destroy_item instead... */
    setnotworn(obj);    /* so we need to do this ourselves */

    if (!zappable(obj)) {
        pline(nothing_else_happens);
        goto discard_broken_wand;
    }
    /* successful call to zappable() consumes a charge; put it back */
    obj->spe++;
    /* might have "wrested" a final charge, taking it from 0 to -1;
       if so, we just brought it back up to 0, which wouldn't do much
       below so give it 1..3 charges now, usually making it stronger
       than an ordinary last charge (the wand is already gone from
       inventory, so perm_invent can't accidentally reveal this) */
    if (!obj->spe)
        obj->spe = rnd(3);

    obj->ox = u.ux;
    obj->oy = u.uy;
    dmg = obj->spe * 4;
    affects_objects = FALSE;

    switch (obj->otyp) {
    case WAN_WISHING:
    case WAN_NOTHING:
    case WAN_LOCKING:
    case WAN_PROBING:
    case WAN_ENLIGHTENMENT:
    case WAN_OPENING:
    case WAN_SECRET_DOOR_DETECTION:
        pline(nothing_else_happens);
        goto discard_broken_wand;
    case WAN_DEATH:
    case WAN_LIGHTNING:
        dmg *= 4;
        goto wanexpl;
    case WAN_FIRE:
        expltype = EXPL_FIERY;
        /*FALLTHRU*/
    case WAN_COLD:
        if (expltype == EXPL_MAGICAL)
            expltype = EXPL_FROSTY;
        dmg *= 2;
        /*FALLTHRU*/
    case WAN_MAGIC_MISSILE:
    wanexpl:
        explode(u.ux, u.uy, -(obj->otyp), dmg, WAND_CLASS, expltype);
        makeknown(obj->otyp); /* explode describes the effect */
        goto discard_broken_wand;
    case WAN_STRIKING:
        /* we want this before the explosion instead of at the very end */
        /*KR pline("A wall of force smashes down around you!"); */
        pline("당신은 어떠한 힘의 벽에 둘러싸였다!");
        dmg = d(1 + obj->spe, 6); /* normally 2d12 */
        /*FALLTHRU*/
    case WAN_CANCELLATION:
    case WAN_POLYMORPH:
    case WAN_TELEPORTATION:
    case WAN_UNDEAD_TURNING:
        affects_objects = TRUE;
        break;
    default:
        break;
    }

    /* magical explosion and its visual effect occur before specific effects
     */
    /* [TODO?  This really ought to prevent the explosion from being
       fatal so that we never leave a bones file where none of the
       surrounding targets (or underlying objects) got affected yet.] */
    explode(obj->ox, obj->oy, -(obj->otyp), rnd(dmg), WAND_CLASS,
            EXPL_MAGICAL);

    /* prepare for potential feedback from polymorph... */
    zapsetup();

    /* this makes it hit us last, so that we can see the action first */
    for (i = 0; i <= 8; i++) {
        bhitpos.x = x = obj->ox + xdir[i];
        bhitpos.y = y = obj->oy + ydir[i];
        if (!isok(x, y))
            continue;

        if (obj->otyp == WAN_DIGGING) {
            schar typ;

            if (dig_check(BY_OBJECT, FALSE, x, y)) {
                if (IS_WALL(levl[x][y].typ) || IS_DOOR(levl[x][y].typ)) {
                    /* normally, pits and holes don't anger guards, but they
                     * do if it's a wall or door that's being dug */
                    watch_dig((struct monst *) 0, x, y, TRUE);
                    if (*in_rooms(x, y, SHOPBASE))
                        shop_damage = TRUE;
                }
                /*
                 * Let liquid flow into the newly created pits.
                 * Adjust corresponding code in music.c for
                 * drum of earthquake if you alter this sequence.
                 */
                typ = fillholetyp(x, y, FALSE);
                if (typ != ROOM) {
                    levl[x][y].typ = typ, levl[x][y].flags = 0;
                    liquid_flow(x, y, typ, t_at(x, y),
                                fillmsg
                                  ? (char *) 0
                       /*KR : "Some holes are quickly filled with %s!"); */
                                  : "구멍이 빠르게 %s(으)로 채워졌다!");
                    fillmsg = TRUE;
                } else
                    digactualhole(x, y, BY_OBJECT, (rn2(obj->spe) < 3
                                                    || (!Can_dig_down(&u.uz)
                                                        && !levl[x][y].candig))
                                                      ? PIT
                                                      : HOLE);
            }
            continue;
        } else if (obj->otyp == WAN_CREATE_MONSTER) {
            /* u.ux,u.uy creates it near you--x,y might create it in rock */
            (void) makemon((struct permonst *) 0, u.ux, u.uy, NO_MM_FLAGS);
            continue;
        } else if (x != u.ux || y != u.uy) {
            /*
             * Wand breakage is targetting a square adjacent to the hero,
             * which might contain a monster or a pile of objects or both.
             * Handle objects last; avoids having undead turning raise an
             * undead's corpse and then attack resulting undead monster.
             * obj->bypass in bhitm() prevents the polymorphing of items
             * dropped due to monster's polymorph and prevents undead
             * turning that kills an undead from raising resulting corpse.
             */
            if ((mon = m_at(x, y)) != 0) {
                (void) bhitm(mon, obj);
                /* if (context.botl) bot(); */
            }
            if (affects_objects && level.objects[x][y]) {
                (void) bhitpile(obj, bhito, x, y, 0);
                if (context.botl)
                    bot(); /* potion effects */
            }
        } else {
            /*
             * Wand breakage is targetting the hero.  Using xdir[]+ydir[]
             * deltas for location selection causes this case to happen
             * after all the surrounding squares have been handled.
             * Process objects first, in case damage is fatal and leaves
             * bones, or teleportation sends one or more of the objects to
             * same destination as hero (lookhere/autopickup); also avoids
             * the polymorphing of gear dropped due to hero's transformation.
             * (Unlike with monsters being hit by zaps, we can't rely on use
             * of obj->bypass in the zap code to accomplish that last case
             * since it's also used by retouch_equipment() for polyself.)
             */
            if (affects_objects && level.objects[x][y]) {
                (void) bhitpile(obj, bhito, x, y, 0);
                if (context.botl)
                    bot(); /* potion effects */
            }
            damage = zapyourself(obj, FALSE);
            if (damage) {
#if 0 /*KR:T*/
                Sprintf(buf, "killed %sself by breaking a wand", uhim());
                losehp(Maybe_Half_Phys(damage), buf, NO_KILLER_PREFIX);
#else
                Sprintf(buf, "지팡이를 부서뜨려서 자살함");
                losehp(Maybe_Half_Phys(damage), buf, KILLED_BY);
#endif
            }
            if (context.botl)
                bot(); /* blindness */
        }
    }

    /* potentially give post zap/break feedback */
    zapwrapup();

    /* Note: if player fell thru, this call is a no-op.
       Damage is handled in digactualhole in that case */
    if (shop_damage)
        /*KR pay_for_damage("dig into", FALSE); */
        pay_for_damage("구멍을 뚫은", FALSE);

    if (obj->otyp == WAN_LIGHT)
        litroom(TRUE, obj); /* only needs to be done once */

discard_broken_wand:
    obj = current_wand; /* [see dozap() and destroy_item()] */
    current_wand = 0;
    if (obj)
        delobj(obj);
    nomul(0);
    return 1;
}

STATIC_OVL void
add_class(cl, class)
char *cl;
char class;
{
    char tmp[2];

    tmp[0] = class;
    tmp[1] = '\0';
    Strcat(cl, tmp);
}

static const char tools[] = { TOOL_CLASS, WEAPON_CLASS, WAND_CLASS, 0 };

/* augment tools[] if various items are carried */
STATIC_OVL void
setapplyclasses(class_list)
char class_list[];
{
    register struct obj *otmp;
    int otyp;
    boolean knowoil, knowtouchstone, addpotions, addstones, addfood;

    knowoil = objects[POT_OIL].oc_name_known;
    knowtouchstone = objects[TOUCHSTONE].oc_name_known;
    addpotions = addstones = addfood = FALSE;
    for (otmp = invent; otmp; otmp = otmp->nobj) {
        otyp = otmp->otyp;
        if (otyp == POT_OIL
            || (otmp->oclass == POTION_CLASS
                && (!otmp->dknown
                    || (!knowoil && !objects[otyp].oc_name_known))))
            addpotions = TRUE;
        if (otyp == TOUCHSTONE
            || (is_graystone(otmp)
                && (!otmp->dknown
                    || (!knowtouchstone && !objects[otyp].oc_name_known))))
            addstones = TRUE;
        if (otyp == CREAM_PIE || otyp == EUCALYPTUS_LEAF)
            addfood = TRUE;
    }

    class_list[0] = '\0';
    if (addpotions || addstones)
        add_class(class_list, ALL_CLASSES);
    Strcat(class_list, tools);
    if (addpotions)
        add_class(class_list, POTION_CLASS);
    if (addstones)
        add_class(class_list, GEM_CLASS);
    if (addfood)
        add_class(class_list, FOOD_CLASS);
}

/* the 'a' command */
int
doapply()
{
    struct obj *obj;
    register int res = 1;
    char class_list[MAXOCLASSES + 2];

    if (check_capacity((char *) 0))
        return 0;

    setapplyclasses(class_list); /* tools[] */
    obj = getobj(class_list, "use or apply");
    if (!obj)
        return 0;

    if (!retouch_object(&obj, FALSE))
        return 1; /* evading your grasp costs a turn; just be
                     grateful that you don't drop it as well */

    if (obj->oclass == WAND_CLASS)
        return do_break_wand(obj);

    switch (obj->otyp) {
    case BLINDFOLD:
    case LENSES:
        if (obj == ublindf) {
            if (!cursed(obj))
                Blindf_off(obj);
        } else if (!ublindf) {
            Blindf_on(obj);
        } else {
#if 0 /*KR:T*/
            You("are already %s.", ublindf->otyp == TOWEL
                                       ? "covered by a towel"
                                       : ublindf->otyp == BLINDFOLD
                                             ? "wearing a blindfold"
                                             : "wearing lenses");
#else
            You("이미 %s 상태다.", ublindf->otyp == TOWEL
                                       ? "수건을 두른"
                                       : ublindf->otyp == BLINDFOLD
                                       ? "눈가리개를 쓴"
                                       : "렌즈를 낀");
#endif
        }
        break;
    case CREAM_PIE:
        res = use_cream_pie(obj);
        break;
    case BULLWHIP:
        res = use_whip(obj);
        break;
    case GRAPPLING_HOOK:
        res = use_grapple(obj);
        break;
    case LARGE_BOX:
    case CHEST:
    case ICE_BOX:
    case SACK:
    case BAG_OF_HOLDING:
    case OILSKIN_SACK:
        res = use_container(&obj, 1, FALSE);
        break;
    case BAG_OF_TRICKS:
        (void) bagotricks(obj, FALSE, (int *) 0);
        break;
    case CAN_OF_GREASE:
        use_grease(obj);
        break;
    case LOCK_PICK:
    case CREDIT_CARD:
    case SKELETON_KEY:
        res = (pick_lock(obj) != 0);
        break;
    case PICK_AXE:
    case DWARVISH_MATTOCK:
        res = use_pick_axe(obj);
        break;
    case TINNING_KIT:
        use_tinning_kit(obj);
        break;
    case LEASH:
        res = use_leash(obj);
        break;
    case SADDLE:
        res = use_saddle(obj);
        break;
    case MAGIC_WHISTLE:
        use_magic_whistle(obj);
        break;
    case TIN_WHISTLE:
        use_whistle(obj);
        break;
    case EUCALYPTUS_LEAF:
        /* MRKR: Every Australian knows that a gum leaf makes an excellent
         * whistle, especially if your pet is a tame kangaroo named Skippy.
         */
        if (obj->blessed) {
            use_magic_whistle(obj);
            /* sometimes the blessing will be worn off */
            if (!rn2(49)) {
                if (!Blind) {
                    /*KR pline("%s %s.", Yobjnam2(obj, "glow"), hcolor("brown")); */
                    /*JP pline("%s이/가 %s 빛났다.", xname(obj), jconj_adj(hcolor("갈색으로"))); */
                    /* jconj_adj: extern.h에서 선언한 특수 함수. 수정필요 */
                    pline("%s이/가 %s.", Yobjnam2(obj, "빛났다"), hcolor("갈색으로"));
                    set_bknown(obj, 1);
                }
                unbless(obj);
            }
        } else {
            use_whistle(obj);
        }
        break;
    case STETHOSCOPE:
        res = use_stethoscope(obj);
        break;
    case MIRROR:
        res = use_mirror(obj);
        break;
    case BELL:
    case BELL_OF_OPENING:
        use_bell(&obj);
        break;
    case CANDELABRUM_OF_INVOCATION:
        use_candelabrum(obj);
        break;
    case WAX_CANDLE:
    case TALLOW_CANDLE:
        use_candle(&obj);
        break;
    case OIL_LAMP:
    case MAGIC_LAMP:
    case BRASS_LANTERN:
        use_lamp(obj);
        break;
    case POT_OIL:
        light_cocktail(&obj);
        break;
    case EXPENSIVE_CAMERA:
        res = use_camera(obj);
        break;
    case TOWEL:
        res = use_towel(obj);
        break;
    case CRYSTAL_BALL:
        use_crystal_ball(&obj);
        break;
    case MAGIC_MARKER:
        res = dowrite(obj);
        break;
    case TIN_OPENER:
        res = use_tin_opener(obj);
        break;
    case FIGURINE:
        use_figurine(&obj);
        break;
    case UNICORN_HORN:
        use_unicorn_horn(obj);
        break;
    case WOODEN_FLUTE:
    case MAGIC_FLUTE:
    case TOOLED_HORN:
    case FROST_HORN:
    case FIRE_HORN:
    case WOODEN_HARP:
    case MAGIC_HARP:
    case BUGLE:
    case LEATHER_DRUM:
    case DRUM_OF_EARTHQUAKE:
        res = do_play_instrument(obj);
        break;
    case HORN_OF_PLENTY: /* not a musical instrument */
        (void) hornoplenty(obj, FALSE);
        break;
    case LAND_MINE:
    case BEARTRAP:
        use_trap(obj);
        break;
    case FLINT:
    case LUCKSTONE:
    case LOADSTONE:
    case TOUCHSTONE:
        use_stone(obj);
        break;
    default:
        /* Pole-weapons can strike at a distance */
        if (is_pole(obj)) {
            res = use_pole(obj);
            break;
        } else if (is_pick(obj) || is_axe(obj)) {
            res = use_pick_axe(obj);
            break;
        }
        /*KR pline("Sorry, I don't know how to use that."); */
        pline("미안, 어떻게 쓰는 건지 모르겠어.");
        nomul(0);
        return 0;
    }
    if (res && obj && obj->oartifact)
        arti_speak(obj);
    nomul(0);
    return res;
}

/* Keep track of unfixable troubles for purposes of messages saying you feel
 * great.
 */
int
unfixable_trouble_count(is_horn)
boolean is_horn;
{
    int unfixable_trbl = 0;

    if (Stoned)
        unfixable_trbl++;
    if (Slimed)
        unfixable_trbl++;
    if (Strangled)
        unfixable_trbl++;
    if (Wounded_legs && !u.usteed)
        unfixable_trbl++;
    /* lycanthropy is undesirable, but it doesn't actually make you feel bad
       so don't count it as a trouble which can't be fixed */

    /*
     * Unicorn horn can fix these when they're timed but not when
     * they aren't.  Potion of restore ability doesn't touch them,
     * so they're always unfixable for the not-unihorn case.
     * [Most of these are timed only, so always curable via horn.
     * An exception is Stunned, which can be forced On by certain
     * polymorph forms (stalker, bats).]
     */
    if (Sick && (!is_horn || (Sick & ~TIMEOUT) != 0L))
        unfixable_trbl++;
    if (Stunned && (!is_horn || (HStun & ~TIMEOUT) != 0L))
        unfixable_trbl++;
    if (Confusion && (!is_horn || (HConfusion & ~TIMEOUT) != 0L))
        unfixable_trbl++;
    if (Hallucination && (!is_horn || (HHallucination & ~TIMEOUT) != 0L))
        unfixable_trbl++;
    if (Vomiting && (!is_horn || (Vomiting & ~TIMEOUT) != 0L))
        unfixable_trbl++;
    if (Deaf && (!is_horn || (HDeaf & ~TIMEOUT) != 0L))
        unfixable_trbl++;

    return unfixable_trbl;
}

/*apply.c*/
