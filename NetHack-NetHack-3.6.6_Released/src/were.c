/* NetHack 3.6	were.c	$NHDT-Date: 1550524568 2019/02/18 21:16:08 $  $NHDT-Branch: NetHack-3.6.2-beta01 $:$NHDT-Revision: 1.23 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2011. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

void
were_change(mon)
register struct monst *mon;
{
    if (!is_were(mon->data))
        return;

    if (is_human(mon->data)) {
        if (!Protection_from_shape_changers
            && !rn2(night() ? (flags.moonphase == FULL_MOON ? 3 : 30)
                            : (flags.moonphase == FULL_MOON ? 10 : 50))) {
            new_were(mon); /* change into animal form */
            if (!Deaf && !canseemon(mon)) {
                const char *howler;

                switch (monsndx(mon->data)) {
                case PM_WEREWOLF:
                    /*KR howler = "wolf"; */
                    howler = "늑대";
                    break;
                case PM_WEREJACKAL:
                    /*KR howler = "jackal"; */
                    howler = "자칼";
                    break;
                default:
                    howler = (char *) 0;
                    break;
                }
                if (howler)
               /*KR You_hear("a %s howling at the moon.", howler); */
                    You_hear("달을 향해 %s 울부짖는 소리를 들었다.",
                             append_josa(howler, "가"));
            }
        }
    } else if (!rn2(30) || Protection_from_shape_changers) {
        new_were(mon); /* change back into human form */
    }
    /* update innate intrinsics (mainly Drain_resistance) */
    set_uasmon(); /* new_were() doesn't do this */
}

int
counter_were(pm)
int pm;
{
    switch (pm) {
    case PM_WEREWOLF:
        return PM_HUMAN_WEREWOLF;
    case PM_HUMAN_WEREWOLF:
        return PM_WEREWOLF;
    case PM_WEREJACKAL:
        return PM_HUMAN_WEREJACKAL;
    case PM_HUMAN_WEREJACKAL:
        return PM_WEREJACKAL;
    case PM_WERERAT:
        return PM_HUMAN_WERERAT;
    case PM_HUMAN_WERERAT:
        return PM_WERERAT;
    default:
        return NON_PM;
    }
}

/* convert monsters similar to werecritters into appropriate werebeast */
int
were_beastie(pm)
int pm;
{
    switch (pm) {
    case PM_WERERAT:
    case PM_SEWER_RAT:
    case PM_GIANT_RAT:
    case PM_RABID_RAT:
        return PM_WERERAT;
    case PM_WEREJACKAL:
    case PM_JACKAL:
    case PM_FOX:
    case PM_COYOTE:
        return PM_WEREJACKAL;
    case PM_WEREWOLF:
    case PM_WOLF:
    case PM_WARG:
    case PM_WINTER_WOLF:
        return PM_WEREWOLF;
    default:
        break;
    }
    return NON_PM;
}

void
new_were(mon)
register struct monst *mon;
{
    register int pm;
#if 1 /*KR: get_kr_name 사용을 위한 외부 선언*/
    extern char *get_kr_name(const char *);
#endif    

    pm = counter_were(monsndx(mon->data));
    if (pm < LOW_PM) {
        impossible("unknown lycanthrope %s.", mon->data->mname);
        return;
    }

    if (canseemon(mon) && !Hallucination)
#if 0 /*KR: 원본*/
        pline("%s changes into a %s.", Monnam(mon),
              is_human(&mons[pm]) ? "human" : mons[pm].mname + 4);
#else /*KR: KRNethack 맞춤 번역 (beastname 꼼수 대신 get_kr_name 활용)*/
        pline("%s %s의 모습으로 변했다.", append_josa(Monnam(mon), "는"),
              is_human(&mons[pm]) ? "인간" : get_kr_name(mons[pm].mname + 4));
    /* 영어 원본의 mname + 4는 "werewolf"에서 "were"를 날리고 "wolf"를
       던집니다. 이 "wolf"를 get_kr_name에 통과시키면 완벽하게 "늑대"가
       튀어나옵니다! */
#endif

    set_mon_data(mon, &mons[pm]);
    if (mon->msleeping || !mon->mcanmove) {
        /* transformation wakens and/or revitalizes */
        mon->msleeping = 0;
        mon->mfrozen = 0; /* not asleep or paralyzed */
        mon->mcanmove = 1;
    }
    /* regenerate by 1/4 of the lost hit points */
    mon->mhp += (mon->mhpmax - mon->mhp) / 4;
    newsym(mon->mx, mon->my);
    mon_break_armor(mon, FALSE);
    possibly_unwield(mon, FALSE);
}

/* were-creature (even you) summons a horde */
int
were_summon(ptr, yours, visible, genbuf)
struct permonst *ptr;
boolean yours;
int *visible; /* number of visible helpers created */
char *genbuf;
{
    int i, typ, pm = monsndx(ptr);
    struct monst *mtmp;
    int total = 0;

    *visible = 0;
    if (Protection_from_shape_changers && !yours)
        return 0;
    for (i = rnd(5); i > 0; i--) {
        switch (pm) {
        case PM_WERERAT:
        case PM_HUMAN_WERERAT:
            typ = rn2(3) ? PM_SEWER_RAT
                         : rn2(3) ? PM_GIANT_RAT : PM_RABID_RAT;
            if (genbuf)
                /*KR Strcpy(genbuf, "rat"); */
                Strcpy(genbuf, "쥐");
            break;
        case PM_WEREJACKAL:
        case PM_HUMAN_WEREJACKAL:
            typ = rn2(7) ? PM_JACKAL : rn2(3) ? PM_COYOTE : PM_FOX;
            if (genbuf)
                /*KR Strcpy(genbuf, "jackal"); */
                Strcpy(genbuf, "자칼");
            break;
        case PM_WEREWOLF:
        case PM_HUMAN_WEREWOLF:
            typ = rn2(5) ? PM_WOLF : rn2(2) ? PM_WARG : PM_WINTER_WOLF;
            if (genbuf)
                /*KR Strcpy(genbuf, "wolf"); */
                Strcpy(genbuf, "늑대");
            break;
        default:
            continue;
        }
        mtmp = makemon(&mons[typ], u.ux, u.uy, NO_MM_FLAGS);
        if (mtmp) {
            total++;
            if (canseemon(mtmp))
                *visible += 1;
        }
        if (yours && mtmp)
            (void) tamedog(mtmp, (struct obj *) 0);
    }
    return total;
}

void
you_were()
{
    char qbuf[QBUFSZ];
    boolean controllable_poly = Polymorph_control && !(Stunned || Unaware);
#if 1 /*KR: get_kr_name 사용을 위한 외부 선언*/
    extern char *get_kr_name(const char *);
#endif 

    if (Unchanging || u.umonnum == u.ulycn)
        return;
    if (controllable_poly) {
#if 0 /*KR: 원본*/
        /* `+4' => skip "were" prefix to get name of beast */
        Sprintf(qbuf, "Do you want to change into %s?",
                an(mons[u.ulycn].mname + 4));
#else /*KR: KRNethack 맞춤 번역*/
        Sprintf(qbuf, "%s(으)로 변신하시겠습니까?",
                get_kr_name(mons[u.ulycn].mname + 4));
#endif
        if (!paranoid_query(ParanoidWerechange, qbuf))
            return;
    }
    (void) polymon(u.ulycn);
}

void
you_unwere(purify)
boolean purify;
{
    boolean controllable_poly = Polymorph_control && !(Stunned || Unaware);

    if (purify) {
        /*KR You_feel("purified."); */
        pline("정화되는 기분이 든다.");
        set_ulycn(NON_PM); /* cure lycanthropy */
    }
    if (!Unchanging && is_were(youmonst.data)
        && (!controllable_poly
            /*KR || !paranoid_query(ParanoidWerechange, "Remain in beast form?"))) */
            || !paranoid_query(ParanoidWerechange, "짐승의 모습으로 남아있겠습니까?")))
        rehumanize();
    else if (is_were(youmonst.data) && !u.mtimedone)
        u.mtimedone = rn1(200, 200); /* 40% of initial were change */
}

/* lycanthropy is being caught or cured, but no shape change is involved */
void
set_ulycn(which)
int which;
{
    u.ulycn = which;
    /* add or remove lycanthrope's innate intrinsics (Drain_resistance) */
    set_uasmon();
}

/*were.c*/
