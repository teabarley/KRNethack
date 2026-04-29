/* NetHack 3.6	minion.c	$NHDT-Date: 1575245071 2019/12/02 00:04:31 $  $NHDT-Branch: NetHack-3.6 $:$NHDT-Revision: 1.44 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2008. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

void newemin(mtmp) struct monst *mtmp;
{
    if (!mtmp->mextra)
        mtmp->mextra = newmextra();
    if (!EMIN(mtmp)) {
        EMIN(mtmp) = (struct emin *) alloc(sizeof(struct emin));
        (void) memset((genericptr_t) EMIN(mtmp), 0, sizeof(struct emin));
    }
}

void free_emin(mtmp) struct monst *mtmp;
{
    if (mtmp->mextra && EMIN(mtmp)) {
        free((genericptr_t) EMIN(mtmp));
        EMIN(mtmp) = (struct emin *) 0;
    }
    mtmp->isminion = 0;
}

/* count the number of monsters on the level */
int
monster_census(spotted)
boolean spotted; /* seen|sensed vs all */
{
    struct monst *mtmp;
    int count = 0;

    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
        if (DEADMONSTER(mtmp))
            continue;
        if (spotted && !canspotmon(mtmp))
            continue;
        ++count;
    }
    return count;
}

/* mon summons a monster */
int
msummon(mon)
struct monst *mon;
{
    struct permonst *ptr;
    int dtype = NON_PM, cnt = 0, result = 0, census;
    aligntyp atyp;
    struct monst *mtmp;

    if (mon) {
        ptr = mon->data;

        if (uwep && uwep->oartifact == ART_DEMONBANE && is_demon(ptr)) {
            if (canseemon(mon))
#if 0 /*KR: 원본*/
                pline("%s looks puzzled for a moment.", Monnam(mon));
#else /*KR: KRNethack 맞춤 번역 */
                pline("%s 잠시 어리둥절한 표정을 지었다.",
                      append_josa(Monnam(mon), "은"));
#endif
            return 0;
        }

        atyp = mon->ispriest                ? EPRI(mon)->shralign
               : mon->isminion              ? EMIN(mon)->min_align
               : (ptr->maligntyp == A_NONE) ? A_NONE
                                            : sgn(ptr->maligntyp);
    } else {
        ptr = &mons[PM_WIZARD_OF_YENDOR];
        atyp = (ptr->maligntyp == A_NONE) ? A_NONE : sgn(ptr->maligntyp);
    }

    if (is_dprince(ptr) || (ptr == &mons[PM_WIZARD_OF_YENDOR])) {
        dtype = (!rn2(20))  ? dprince(atyp)
                : (!rn2(4)) ? dlord(atyp)
                            : ndemon(atyp);
        cnt =
            ((dtype != NON_PM) && !rn2(4) && is_ndemon(&mons[dtype])) ? 2 : 1;
    } else if (is_dlord(ptr)) {
        dtype = (!rn2(50))   ? dprince(atyp)
                : (!rn2(20)) ? dlord(atyp)
                             : ndemon(atyp);
        cnt =
            ((dtype != NON_PM) && !rn2(4) && is_ndemon(&mons[dtype])) ? 2 : 1;
    } else if (is_ndemon(ptr)) {
        dtype = (!rn2(20))  ? dlord(atyp)
                : (!rn2(6)) ? ndemon(atyp)
                            : monsndx(ptr);
        cnt = 1;
    } else if (is_lminion(mon)) {
        dtype = (is_lord(ptr) && !rn2(20))  ? llord()
                : (is_lord(ptr) || !rn2(6)) ? lminion()
                                            : monsndx(ptr);
        cnt =
            ((dtype != NON_PM) && !rn2(4) && !is_lord(&mons[dtype])) ? 2 : 1;
    } else if (ptr == &mons[PM_ANGEL]) {
        /* non-lawful angels can also summon */
        if (!rn2(6)) {
            switch (atyp) { /* see summon_minion */
            case A_NEUTRAL:
                dtype = PM_AIR_ELEMENTAL + rn2(4);
                break;
            case A_CHAOTIC:
            case A_NONE:
                dtype = ndemon(atyp);
                break;
            }
        } else {
            dtype = PM_ANGEL;
        }
        cnt =
            ((dtype != NON_PM) && !rn2(4) && !is_lord(&mons[dtype])) ? 2 : 1;
    }

    if (dtype == NON_PM)
        return 0;

    /* sanity checks */
    if (cnt > 1 && (mons[dtype].geno & G_UNIQ))
        cnt = 1;
    /*
     * If this daemon is unique and being re-summoned (the only way we
     * could get this far with an extinct dtype), try another.
     */
    if (mvitals[dtype].mvflags & G_GONE) {
        dtype = ndemon(atyp);
        if (dtype == NON_PM)
            return 0;
    }

    /* some candidates can generate a group of monsters, so simple
       count of non-null makemon() result is not sufficient */
    census = monster_census(FALSE);

    while (cnt > 0) {
        mtmp = makemon(&mons[dtype], u.ux, u.uy, MM_EMIN);
        if (mtmp) {
            result++;
            /* an angel's alignment should match the summoner */
            if (dtype == PM_ANGEL) {
                mtmp->isminion = 1;
                EMIN(mtmp)->min_align = atyp;
                /* renegade if same alignment but not peaceful
                   or peaceful but different alignment */
                EMIN(mtmp)->renegade =
                    (atyp != u.ualign.type) ^ !mtmp->mpeaceful;
            }
            if (is_demon(ptr) && canseemon(mtmp))
#if 0 /*KR: 원본*/
                pline("%s appears in a cloud of smoke!", Amonnam(mtmp));
#else /*KR: KRNethack 맞춤 번역 */
                pline("%s 연기구름 속에서 나타났다!",
                      append_josa(Amonnam(mtmp), "이"));
#endif
        }
        cnt--;
    }

    /* how many monsters exist now compared to before? */
    if (result)
        result = monster_census(FALSE) - census;

    return result;
}

void summon_minion(alignment, talk) aligntyp alignment;
boolean talk;
{
    register struct monst *mon;
    int mnum;

    switch ((int) alignment) {
    case A_LAWFUL:
        mnum = lminion();
        break;
    case A_NEUTRAL:
        mnum = PM_AIR_ELEMENTAL + rn2(4);
        break;
    case A_CHAOTIC:
    case A_NONE:
        mnum = ndemon(alignment);
        break;
    default:
        impossible("unaligned player?");
        mnum = ndemon(A_NONE);
        break;
    }
    if (mnum == NON_PM) {
        mon = 0;
    } else if (mnum == PM_ANGEL) {
        mon = makemon(&mons[mnum], u.ux, u.uy, MM_EMIN);
        if (mon) {
            mon->isminion = 1;
            EMIN(mon)->min_align = alignment;
            EMIN(mon)->renegade = FALSE;
        }
    } else if (mnum != PM_SHOPKEEPER && mnum != PM_GUARD
               && mnum != PM_ALIGNED_PRIEST && mnum != PM_HIGH_PRIEST) {
        /* This was mons[mnum].pxlth == 0 but is this restriction
           appropriate or necessary now that the structures are separate? */
        mon = makemon(&mons[mnum], u.ux, u.uy, MM_EMIN);
        if (mon) {
            mon->isminion = 1;
            EMIN(mon)->min_align = alignment;
            EMIN(mon)->renegade = FALSE;
        }
    } else {
        mon = makemon(&mons[mnum], u.ux, u.uy, NO_MM_FLAGS);
    }
    if (mon) {
        if (talk) {
            if (!Deaf)
#if 0 /*KR: 원본*/
                pline_The("voice of %s booms:", align_gname(alignment));
#else /*KR: KRNethack 맞춤 번역 */
                pline("%s의 목소리가 울려 퍼졌다:", align_gname(alignment));
#endif
            else
#if 0 /*KR: 원본*/
                You_feel("%s booming voice:",
                         s_suffix(align_gname(alignment)));
#else /*KR: KRNethack 맞춤 번역 */
                You_feel("%s 울려 퍼지는 목소리를 느꼈다:",
                         s_suffix(align_gname(alignment)));
#endif
            /*KR verbalize("Thou shalt pay for thine indiscretion!"); */
            verbalize("너의 무분별함에 대한 대가를 치를지어다!");
            if (canspotmon(mon))
#if 0 /*KR: 원본*/
                pline("%s appears before you.", Amonnam(mon));
#else /*KR: KRNethack 맞춤 번역 */
                pline("%s 당신의 앞에 나타났다.",
                      append_josa(Amonnam(mon), "이"));
#endif
            mon->mstrategy &= ~STRAT_APPEARMSG;
        }
        mon->mpeaceful = FALSE;
        /* don't call set_malign(); player was naughty */
    }
}

#define Athome (Inhell && (mtmp->cham == NON_PM))

/* returns 1 if it won't attack. */
int
demon_talk(mtmp)
register struct monst *mtmp;
{
    long cash, demand, offer;

    if (uwep && uwep->oartifact == ART_EXCALIBUR) {
#if 0 /*KR: 원본*/
        pline("%s looks very angry.", Amonnam(mtmp));
#else /*KR: KRNethack 맞춤 번역 */
        pline("%s 매우 화나 보인다.", append_josa(Amonnam(mtmp), "은"));
#endif
        mtmp->mpeaceful = mtmp->mtame = 0;
        set_malign(mtmp);
        newsym(mtmp->mx, mtmp->my);
        return 0;
    }

    if (is_fainted()) {
        reset_faint(); /* if fainted - wake up */
    } else {
        stop_occupation();
        if (multi > 0) {
            nomul(0);
            unmul((char *) 0);
        }
    }

    /* Slight advantage given. */
    if (is_dprince(mtmp->data) && mtmp->minvis) {
        boolean wasunseen = !canspotmon(mtmp);

        mtmp->minvis = mtmp->perminvis = 0;
        if (wasunseen && canspotmon(mtmp)) {
#if 0 /*KR: 원본*/
            pline("%s appears before you.", Amonnam(mtmp));
#else /*KR: KRNethack 맞춤 번역 */
            pline("%s 당신의 앞에 나타났다.",
                  append_josa(Amonnam(mtmp), "이"));
#endif
            mtmp->mstrategy &= ~STRAT_APPEARMSG;
        }
        newsym(mtmp->mx, mtmp->my);
    }
    if (youmonst.data->mlet == S_DEMON) { /* Won't blackmail their own. */
        if (!Deaf)
#if 0 /*KR: 원본*/
            pline("%s says, \"Good hunting, %s.\"", Amonnam(mtmp),
                  flags.female ? "Sister" : "Brother");
#else /*KR: KRNethack 맞춤 번역 */
            pline("%s 말한다, \"훌륭한 사냥이 되기를, %s.\"",
                  append_josa(Amonnam(mtmp), "이"),
                  flags.female ? "자매여" : "형제여");
#endif
        else if (canseemon(mtmp))
#if 0 /*KR: 원본*/
            pline("%s says something.", Amonnam(mtmp));
#else /*KR: KRNethack 맞춤 번역 */
            pline("%s 무언가를 말한다.", append_josa(Amonnam(mtmp), "이"));
#endif
        if (!tele_restrict(mtmp))
            (void) rloc(mtmp, TRUE);
        return 1;
    }
    cash = money_cnt(invent);
    demand =
        (cash * (rnd(80) + 20 * Athome))
        / (100 * (1 + (sgn(u.ualign.type) == sgn(mtmp->data->maligntyp))));

    if (!demand || multi < 0) { /* you have no gold or can't move */
        mtmp->mpeaceful = 0;
        set_malign(mtmp);
        return 0;
    } else {
        /* make sure that the demand is unmeetable if the monster
           has the Amulet, preventing monster from being satisfied
           and removed from the game (along with said Amulet...) */
        /* [actually the Amulet is safe; it would be dropped when
           mongone() gets rid of the monster; force combat anyway;
           also make it unmeetable if the player is Deaf, to simplify
           handling that case as player-won't-pay] */
        if (mon_has_amulet(mtmp) || Deaf)
            /* 125: 5*25 in case hero has maximum possible charisma */
            demand = cash + (long) rn1(1000, 125);

        if (!Deaf)
#if 0 /*KR: 원본*/
            pline("%s demands %ld %s for safe passage.",
                  Amonnam(mtmp), demand, currency(demand));
#else /*KR: KRNethack 맞춤 번역 */
            pline("%s 무사히 통과하고 싶다면 %ld %s 내놓으라고 요구한다.",
                  append_josa(Amonnam(mtmp), "은"), demand,
                  append_josa(currency(demand), "을"));
#endif
        else if (canseemon(mtmp))
#if 0 /*KR: 원본*/
            pline("%s seems to be demanding something.", Amonnam(mtmp));
#else /*KR: KRNethack 맞춤 번역 */
            pline("%s 무언가를 요구하는 것 같다.",
                  append_josa(Amonnam(mtmp), "이"));
#endif

        offer = 0L;
        if (!Deaf && ((offer = bribe(mtmp)) >= demand)) {
#if 0 /*KR: 원본*/
            pline("%s vanishes, laughing about cowardly mortals.",
                  Amonnam(mtmp));
#else /*KR: KRNethack 맞춤 번역 */
            pline("%s 겁 많은 필멸자들을 비웃으며 사라졌다.",
                  append_josa(Amonnam(mtmp), "은"));
#endif
        } else if (offer > 0L
                   && (long) rnd(5 * ACURR(A_CHA)) > (demand - offer)) {
#if 0 /*KR: 원본*/
            pline("%s scowls at you menacingly, then vanishes.",
                  Amonnam(mtmp));
#else /*KR: KRNethack 맞춤 번역 */
            pline("%s 당신을 위협적으로 노려보고는, 이내 사라졌다.",
                  append_josa(Amonnam(mtmp), "은"));
#endif
        } else {
#if 0 /*KR: 원본*/
            pline("%s gets angry...", Amonnam(mtmp));
#else /*KR: KRNethack 맞춤 번역 */
            pline("%s 화를 낸다...", append_josa(Amonnam(mtmp), "이"));
#endif
            mtmp->mpeaceful = 0;
            set_malign(mtmp);
            return 0;
        }
    }
    mongone(mtmp);
    return 1;
}

long
bribe(mtmp)
struct monst *mtmp;
{
    char buf[BUFSZ] = DUMMY;
    long offer;
    long umoney = money_cnt(invent);

    /*KR getlin("How much will you offer?", buf); */
    getlin("얼마를 제안하시겠습니까?", buf);
    if (sscanf(buf, "%ld", &offer) != 1)
        offer = 0L;

    /*Michael Paddon -- fix for negative offer to monster*/
    /*JAR880815 - */
    if (offer < 0L) {
#if 0 /*KR: 원본*/
        You("try to shortchange %s, but fumble.", mon_nam(mtmp));
#else /*KR: KRNethack 맞춤 번역 */
        You("%s 속이려 했으나, 엉성하게 실패했다.",
            append_josa(mon_nam(mtmp), "을"));
#endif
        return 0L;
    } else if (offer == 0L) {
        /*KR You("refuse."); */
        You("거절했다.");
        return 0L;
    } else if (offer >= umoney) {
#if 0 /*KR: 원본*/
        You("give %s all your gold.", mon_nam(mtmp));
#else /*KR: KRNethack 맞춤 번역 */
        You("%s에게 당신의 모든 금화를 주었다.", mon_nam(mtmp));
#endif
        offer = umoney;
    } else {
#if 0 /*KR: 원본*/
        You("give %s %ld %s.", mon_nam(mtmp), offer, currency(offer));
#else /*KR: KRNethack 맞춤 번역 */
        You("%s에게 %ld %s 주었다.", mon_nam(mtmp), offer,
            append_josa(currency(offer), "을"));
#endif
    }
    (void) money2mon(mtmp, offer);
    context.botl = 1;
    return offer;
}

int
dprince(atyp)
aligntyp atyp;
{
    int tryct, pm;

    for (tryct = !In_endgame(&u.uz) ? 20 : 0; tryct > 0; --tryct) {
        pm = rn1(PM_DEMOGORGON + 1 - PM_ORCUS, PM_ORCUS);
        if (!(mvitals[pm].mvflags & G_GONE)
            && (atyp == A_NONE || sgn(mons[pm].maligntyp) == sgn(atyp)))
            return pm;
    }
    return dlord(atyp); /* approximate */
}

int
dlord(atyp)
aligntyp atyp;
{
    int tryct, pm;

    for (tryct = !In_endgame(&u.uz) ? 20 : 0; tryct > 0; --tryct) {
        pm = rn1(PM_YEENOGHU + 1 - PM_JUIBLEX, PM_JUIBLEX);
        if (!(mvitals[pm].mvflags & G_GONE)
            && (atyp == A_NONE || sgn(mons[pm].maligntyp) == sgn(atyp)))
            return pm;
    }
    return ndemon(atyp); /* approximate */
}

/* create lawful (good) lord */
int
llord()
{
    if (!(mvitals[PM_ARCHON].mvflags & G_GONE))
        return PM_ARCHON;

    return lminion(); /* approximate */
}

int
lminion()
{
    int tryct;
    struct permonst *ptr;

    for (tryct = 0; tryct < 20; tryct++) {
        ptr = mkclass(S_ANGEL, 0);
        if (ptr && !is_lord(ptr))
            return monsndx(ptr);
    }

    return NON_PM;
}

int
ndemon(atyp)
aligntyp atyp; /* A_NONE is used for 'any alignment' */
{
    struct permonst *ptr;

    /*
     * 3.6.2:  [fixed #H2204, 22-Dec-2010, eight years later...]
     * pick a correctly aligned demon in one try.  This used to
     * use mkclass() to choose a random demon type and keep trying
     * (up to 20 times) until it got one with the desired alignment.
     * mkclass_aligned() skips wrongly aligned potential candidates.
     * [The only neutral demons are djinni and mail daemon and
     * mkclass() won't pick them, but call it anyway in case either
     * aspect of that changes someday.]
     */
#if 0
    if (atyp == A_NEUTRAL)
        return NON_PM;
#endif
    ptr = mkclass_aligned(S_DEMON, 0, atyp);
    return (ptr && is_ndemon(ptr)) ? monsndx(ptr) : NON_PM;
}

/* guardian angel has been affected by conflict so is abandoning hero */
void lose_guardian_angel(
    mon) struct monst *mon; /* if null, angel hasn't been created yet */
{
    coord mm;
    int i;

    if (mon) {
        if (canspotmon(mon)) {
            if (!Deaf) {
#if 0 /*KR: 원본*/
                pline("%s rebukes you, saying:", Monnam(mon));
#else /*KR: KRNethack 맞춤 번역 */
                pline("%s 당신을 꾸짖으며 말한다:",
                      append_josa(Monnam(mon), "이"));
#endif
                /*KR verbalize("Since you desire conflict, have some more!");
                 */
                verbalize("네가 투쟁을 원하니, 조금 더 맛보아라!");
            } else {
#if 0 /*KR: 원본*/
                pline("%s vanishes!", Monnam(mon));
#else /*KR: KRNethack 맞춤 번역 */
                pline("%s 사라졌다!", append_josa(Monnam(mon), "이"));
#endif
            }
        }
        mongone(mon);
    }
    /* create 2 to 4 hostile angels to replace the lost guardian */
    for (i = rn1(3, 2); i > 0; --i) {
        mm.x = u.ux;
        mm.y = u.uy;
        if (enexto(&mm, mm.x, mm.y, &mons[PM_ANGEL]))
            (void) mk_roamer(&mons[PM_ANGEL], u.ualign.type, mm.x, mm.y,
                             FALSE);
    }
}

/* just entered the Astral Plane; receive tame guardian angel if worthy */
void
gain_guardian_angel()
{
    struct monst *mtmp;
    struct obj *otmp;
    coord mm;

    Hear_again(); /* attempt to cure any deafness now (divine
                     message will be heard even if that fails) */
    if (Conflict) {
        if (!Deaf)
            /*KR pline("A voice booms:"); */
            pline("어떤 목소리가 울려 퍼졌다:");
        else
            /*KR You_feel("a booming voice:"); */
            You_feel("울려 퍼지는 목소리를 느꼈다:");
        /*KR verbalize("Thy desire for conflict shall be fulfilled!"); */
        verbalize("투쟁에 대한 너의 갈망이 채워질지어다!");
        /* send in some hostile angels instead */
        lose_guardian_angel((struct monst *) 0);
    } else if (u.ualign.record > 8) { /* fervent */
        if (!Deaf)
            /*KR pline("A voice whispers:"); */
            pline("어떤 목소리가 속삭였다:");
        else
            /*KR You_feel("a soft voice:"); */
            You_feel("부드러운 목소리를 느꼈다:");
        /*KR verbalize("Thou hast been worthy of me!"); */
        verbalize("너는 나의 인정을 받을 자격이 있다!");
        mm.x = u.ux;
        mm.y = u.uy;
        if (enexto(&mm, mm.x, mm.y, &mons[PM_ANGEL])
            && (mtmp = mk_roamer(&mons[PM_ANGEL], u.ualign.type, mm.x, mm.y,
                                 TRUE))
                   != 0) {
            mtmp->mstrategy &= ~STRAT_APPEARMSG;
            /* guardian angel -- the one case mtame doesn't imply an
             * edog structure, so we don't want to call tamedog().
             * [Note: this predates mon->mextra which allows a monster
             * to have both emin and edog at the same time.]
             */
            mtmp->mtame = 10;
            /* for 'hilite_pet'; after making tame, before next message */
            newsym(mtmp->mx, mtmp->my);
            if (!Blind)
                /*KR pline("An angel appears near you."); */
                pline("당신 근처에 천사가 나타났다.");
            else
                /*KR You_feel("the presence of a friendly angel near you.");
                 */
                You_feel("당신 근처에 우호적인 천사가 있음을 느꼈다.");
            /* make him strong enough vs. endgame foes */
            mtmp->m_lev = rn1(8, 15);
            mtmp->mhp = mtmp->mhpmax =
                d((int) mtmp->m_lev, 10) + 30 + rnd(30);
            if ((otmp = select_hwep(mtmp)) == 0) {
                otmp = mksobj(SILVER_SABER, FALSE, FALSE);
                if (mpickobj(mtmp, otmp))
                    panic("merged weapon?");
            }
            bless(otmp);
            if (otmp->spe < 4)
                otmp->spe += rnd(4);
            if ((otmp = which_armor(mtmp, W_ARMS)) == 0
                || otmp->otyp != SHIELD_OF_REFLECTION) {
                (void) mongets(mtmp, AMULET_OF_REFLECTION);
                m_dowear(mtmp, TRUE);
            }
        }
    }
}

/*minion.c*/
