/* NetHack 3.6	steed.c	$NHDT-Date: 1575245090 2019/12/02 00:04:50 $  $NHDT-Branch: NetHack-3.6 $:$NHDT-Revision: 1.68 $ */
/* Copyright (c) Kevin Hugo, 1998-1999. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

/* Monsters that might be ridden */
static NEARDATA const char steeds[] = { S_QUADRUPED, S_UNICORN, S_ANGEL,
                                        S_CENTAUR,   S_DRAGON,  S_JABBERWOCK,
                                        '\0' };

STATIC_DCL boolean FDECL(landing_spot, (coord *, int, int) );
STATIC_DCL void FDECL(maybewakesteed, (struct monst *) );

/* caller has decided that hero can't reach something while mounted */
void
rider_cant_reach()
{
    /*KR You("aren't skilled enough to reach from %s.", y_monnam(u.usteed));
     */
    You("아직 기승 기술이 부족하여 %s에게서 손이 닿지 않는다.",
        y_monnam(u.usteed));
}

/*** Putting the saddle on ***/

/* Can this monster wear a saddle? */
boolean
can_saddle(mtmp)
struct monst *mtmp;
{
    struct permonst *ptr = mtmp->data;

    return (index(steeds, ptr->mlet) && (ptr->msize >= MZ_MEDIUM)
            && (!humanoid(ptr) || ptr->mlet == S_CENTAUR) && !amorphous(ptr)
            && !noncorporeal(ptr) && !is_whirly(ptr) && !unsolid(ptr));
}

int
use_saddle(otmp)
struct obj *otmp;
{
    struct monst *mtmp;
    struct permonst *ptr;
    int chance;
    const char *s;

    if (!u_handsy())
        return 0;

    /* Select an animal */
    if (u.uswallow || Underwater || !getdir((char *) 0)) {
        pline1(Never_mind);
        return 0;
    }
    if (!u.dx && !u.dy) {
        /*KR pline("Saddle yourself?  Very funny..."); */
        pline("스스로에게 안장을 씌우겠다고? 아주 재미있군...");
        return 0;
    }
    if (!isok(u.ux + u.dx, u.uy + u.dy)
        || !(mtmp = m_at(u.ux + u.dx, u.uy + u.dy)) || !canspotmon(mtmp)) {
        /*KR pline("I see nobody there."); */
        pline("거기엔 아무도 없다.");
        return 1;
    }

    /* Is this a valid monster? */
    if (mtmp->misc_worn_check & W_SADDLE || which_armor(mtmp, W_SADDLE)) {
        /*KR pline("%s doesn't need another one.", Monnam(mtmp)); */
        pline("%s 이미 안장을 차고 있다.",
              append_josa(Monnam(mtmp), "은"));
        return 1;
    }
    ptr = mtmp->data;
    if (touch_petrifies(ptr) && !uarmg && !Stone_resistance) {
        char kbuf[BUFSZ];

        /*KR You("touch %s.", mon_nam(mtmp)); */
        You("%s 만졌다.", append_josa(mon_nam(mtmp), "을"));
        if (!(poly_when_stoned(youmonst.data) && polymon(PM_STONE_GOLEM))) {
            /*KR Sprintf(kbuf, "attempting to saddle %s",
             * an(mtmp->data->mname)); */
            Sprintf(kbuf, "%s에게 안장을 씌우려다가", mtmp->data->mname);
            instapetrify(kbuf);
        }
    }
    if (ptr == &mons[PM_INCUBUS] || ptr == &mons[PM_SUCCUBUS]) {
        /*KR pline("Shame on you!"); */
        pline("부끄러운 줄 알아라!");
        exercise(A_WIS, FALSE);
        return 1;
    }
    if (mtmp->isminion || mtmp->isshk || mtmp->ispriest || mtmp->isgd
        || mtmp->iswiz) {
        /*KR pline("I think %s would mind.", mon_nam(mtmp)); */
        pline("%s 싫어할 것 같다.", append_josa(mon_nam(mtmp), "가"));
        return 1;
    }
    if (!can_saddle(mtmp)) {
        /*KR You_cant("saddle such a creature."); */
        You_cant("이런 생명체에게는 안장을 씌울 수 없다.");
        return 1;
    }

    /* Calculate your chance */
    chance = ACURR(A_DEX) + ACURR(A_CHA) / 2 + 2 * mtmp->mtame;
    chance += u.ulevel * (mtmp->mtame ? 20 : 5);
    if (!mtmp->mtame)
        chance -= 10 * mtmp->m_lev;
    if (Role_if(PM_KNIGHT))
        chance += 20;
    switch (P_SKILL(P_RIDING)) {
    case P_ISRESTRICTED:
    case P_UNSKILLED:
    default:
        chance -= 20;
        break;
    case P_BASIC:
        break;
    case P_SKILLED:
        chance += 15;
        break;
    case P_EXPERT:
        chance += 30;
        break;
    }
    if (Confusion || Fumbling || Glib)
        chance -= 20;
    else if (uarmg
             && (s = OBJ_DESCR(objects[uarmg->otyp])) != (char *) 0
             /*KR && !strncmp(s, "riding ", 7)) */
             && !strncmp(s, "승마용 ", strlen("승마용 ")))
        /* Bonus for wearing "riding" (but not fumbling) gloves */
        chance += 10;
    else if (uarmf
             && (s = OBJ_DESCR(objects[uarmf->otyp])) != (char *) 0
             /*KR && !strncmp(s, "riding ", 7)) */
             && !strncmp(s, "승마용 ", strlen("승마용 "))) 
        /* ... or for "riding boots" */
        chance += 10;
    if (otmp->cursed)
        chance -= 50;

    /* [intended] steed becomes alert if possible */
    maybewakesteed(mtmp);

    /* Make the attempt */
    if (rn2(100) < chance) {
        /*KR You("put the saddle on %s.", mon_nam(mtmp)); */
        You("%s에게 안장을 얹었다.", mon_nam(mtmp));
        if (otmp->owornmask)
            remove_worn_item(otmp, FALSE);
        freeinv(otmp);
        put_saddle_on_mon(otmp, mtmp);
    } else
        /*KR pline("%s resists!", Monnam(mtmp)); */
        pline("%s 거부했다!", append_josa(Monnam(mtmp), "가"));
    return 1;
}

void put_saddle_on_mon(saddle, mtmp) struct obj *saddle;
struct monst *mtmp;
{
    if (!can_saddle(mtmp) || which_armor(mtmp, W_SADDLE))
        return;
    if (mpickobj(mtmp, saddle))
        panic("merged saddle?");
    mtmp->misc_worn_check |= W_SADDLE;
    saddle->owornmask = W_SADDLE;
    saddle->leashmon = mtmp->m_id;
    update_mon_intrinsics(mtmp, saddle, TRUE, FALSE);
}

/*** Riding the monster ***/

/* Can we ride this monster?  Caller should also check can_saddle() */
boolean
can_ride(mtmp)
struct monst *mtmp;
{
    return (mtmp->mtame && humanoid(youmonst.data)
            && !verysmall(youmonst.data) && !bigmonst(youmonst.data)
            && (!Underwater || is_swimmer(mtmp->data)));
}

int
doride()
{
    boolean forcemount = FALSE;

    if (u.usteed) {
        dismount_steed(DISMOUNT_BYCHOICE);
    } else if (getdir((char *) 0) && isok(u.ux + u.dx, u.uy + u.dy)) {
        /*KR if (wizard && yn("Force the mount to succeed?") == 'y') */
        if (wizard && yn("강제로 기승에 성공하게 만드시겠습니까?") == 'y')
            forcemount = TRUE;
        return (mount_steed(m_at(u.ux + u.dx, u.uy + u.dy), forcemount));
    } else {
        return 0;
    }
    return 1;
}

/* Start riding, with the given monster */
boolean
mount_steed(mtmp, force)
struct monst *mtmp; /* The animal */
boolean force;      /* Quietly force this animal */
{
    struct obj *otmp;
    char buf[BUFSZ];
    struct permonst *ptr;

    /* Sanity checks */
    if (u.usteed) {
        /*KR You("are already riding %s.", mon_nam(u.usteed)); */
        You("이미 %s 타고 있다.", append_josa(mon_nam(u.usteed), "을"));
        return (FALSE);
    }

    /* Is the player in the right form? */
    if (Hallucination && !force) {
#if 0 /*KR: 원본*/
        pline("Maybe you should find a designated driver.");
#else /*KR: KRNethack 맞춤 번역 */
        pline("대리기사를 찾아보는 게 좋을 것 같다.");
#endif
        return (FALSE);
    }
    /* While riding Wounded_legs refers to the steed's,
     * not the hero's legs.
     * That opens up a potential abuse where the player
     * can mount a steed, then dismount immediately to
     * heal leg damage, because leg damage is always
     * healed upon dismount (Wounded_legs context switch).
     * By preventing a hero with Wounded_legs from
     * mounting a steed, the potential for abuse is
     * reduced.  However, dismounting still immediately
     * heals the steed's wounded legs.  [In 3.4.3 and
     * earlier, that unintentionally made the hero's
     * temporary 1 point Dex loss become permanent.]
     */
    if (Wounded_legs) {
        /*KR Your("%s are in no shape for riding.",
         * makeplural(body_part(LEG))); */
        pline("%s 다쳐서 말을 탈 상태가 아니다.",
              append_josa(makeplural(body_part(LEG)), "을"));
        /*KR if (force && wizard && yn("Heal your legs?") == 'y') */
        if (force && wizard && yn("다리를 치료하시겠습니까?") == 'y')
            HWounded_legs = EWounded_legs = 0L;
        else
            return (FALSE);
    }

    if (Upolyd
        && (!humanoid(youmonst.data) || verysmall(youmonst.data)
            || bigmonst(youmonst.data) || slithy(youmonst.data))) {
        /*KR You("won't fit on a saddle."); */
        You("안장에 몸이 맞지 않는다.");
        return (FALSE);
    }
    if (!force && (near_capacity() > SLT_ENCUMBER)) {
        /*KR You_cant("do that while carrying so much stuff."); */
        You_cant("짐을 그렇게 많이 들고서는 기승할 수 없다.");
        return (FALSE);
    }

    /* Can the player reach and see the monster? */
    if (!mtmp
        || (!force
            && ((Blind && !Blind_telepat) || mtmp->mundetected
                || M_AP_TYPE(mtmp) == M_AP_FURNITURE
                || M_AP_TYPE(mtmp) == M_AP_OBJECT))) {
        /*KR pline("I see nobody there."); */
        pline("거기엔 아무도 안 보인다.");
        return (FALSE);
    }
    if (mtmp->data == &mons[PM_LONG_WORM]
        && (u.ux + u.dx != mtmp->mx || u.uy + u.dy != mtmp->my)) {
        /* As of 3.6.2:  test_move(below) is used to check for trying to mount
           diagonally into or out of a doorway or through a tight squeeze;
           attempting to mount a tail segment when hero was not adjacent
           to worm's head could trigger an impossible() in worm_cross()
           called from test_move(), so handle not-on-head before that */
        /*KR You("couldn't ride %s, let alone its tail.", a_monnam(mtmp)); */
        You("꼬리는 말할 것도 없고, %s조차도 탈 수 없다.", a_monnam(mtmp));
        return FALSE;
    }
    if (u.uswallow || u.ustuck || u.utrap || Punished
        || !test_move(u.ux, u.uy, mtmp->mx - u.ux, mtmp->my - u.uy,
                      TEST_MOVE)) {
        if (Punished || !(u.uswallow || u.ustuck || u.utrap))
            /*KR You("are unable to swing your %s over.", body_part(LEG)); */
            You("안장을 넘어 %s 휘두를 수 없다.",
                append_josa(body_part(LEG), "을"));
        else
            /*KR You("are stuck here for now."); */
            You("당분간은 여기 꼼짝없이 갇혀 있다.");
        return (FALSE);
    }

    /* Is this a valid monster? */
    otmp = which_armor(mtmp, W_SADDLE);
    if (!otmp) {
        /*KR pline("%s is not saddled.", Monnam(mtmp)); */
        pline("%s 안장이 채워져 있지 않다.",
              append_josa(Monnam(mtmp), "은"));
        return (FALSE);
    }
    ptr = mtmp->data;
    if (touch_petrifies(ptr) && !Stone_resistance) {
        char kbuf[BUFSZ];

        /*KR You("touch %s.", mon_nam(mtmp)); */
        You("%s 만졌다.", append_josa(mon_nam(mtmp), "을"));
        /*KR Sprintf(kbuf, "attempting to ride %s", an(mtmp->data->mname)); */
        Sprintf(kbuf, "%s 등에 타려다", a_monnam(mtmp));
        instapetrify(kbuf);
    }
    if (!mtmp->mtame || mtmp->isminion) {
        /*KR pline("I think %s would mind.", mon_nam(mtmp)); */
        pline("%s 싫어할 것 같다.", append_josa(mon_nam(mtmp), "가"));
        return (FALSE);
    }
    if (mtmp->mtrapped) {
        struct trap *t = t_at(mtmp->mx, mtmp->my);

#if 0 /*KR: 원본*/
        You_cant("mount %s while %s's trapped in %s.", mon_nam(mtmp),
                 mhe(mtmp), an(defsyms[trap_to_defsym(t->ttyp)].explanation));
#else /*KR: KRNethack 맞춤 번역 */
        You_cant("%s 갇혀 있는 동안에는 %s 등에 탈 수 없다.",
                 append_josa(an(defsyms[trap_to_defsym(t->ttyp)].explanation),
                             "에"),
                 mon_nam(mtmp));
#endif
        return (FALSE);
    }

    if (!force && !Role_if(PM_KNIGHT) && !(--mtmp->mtame)) {
        /* no longer tame */
        newsym(mtmp->mx, mtmp->my);
#if 0 /*KR: 원본*/
        pline("%s resists%s!", Monnam(mtmp),
              mtmp->mleashed ? " and its leash comes off" : "");
#else /*KR: KRNethack 맞춤 번역 */
        pline("%s 저항했다%s!", append_josa(Monnam(mtmp), "가"),
              mtmp->mleashed ? " 그리고 목줄이 풀렸다" : "");
#endif
        if (mtmp->mleashed)
            m_unleash(mtmp, FALSE);
        return (FALSE);
    }
    if (!force && Underwater && !is_swimmer(ptr)) {
        /*KR You_cant("ride that creature while under %s.",
                 hliquid("water")); */
        You_cant("%s 아래에서는 그 생물을 탈 수 없다.", hliquid("물"));
        return (FALSE);
    }
    if (!can_saddle(mtmp) || !can_ride(mtmp)) {
        /*KR You_cant("ride such a creature."); */
        You_cant("그런 생명체에는 탈 수 없다.");
        return FALSE;
    }

    /* Is the player impaired? */
    if (!force && !is_floater(ptr) && !is_flyer(ptr) && Levitation
        && !Lev_at_will) {
        /*KR You("cannot reach %s.", mon_nam(mtmp)); */
        You("%s에게 닿지 않는다.", mon_nam(mtmp));
        return (FALSE);
    }
    if (!force && uarm && is_metallic(uarm) && greatest_erosion(uarm)) {
#if 0 /*KR: 원본*/
        Your("%s armor is too stiff to be able to mount %s.",
             uarm->oeroded ? "rusty" : "corroded", mon_nam(mtmp));
#else /*KR: KRNethack 맞춤 번역 */
        Your("%s 갑옷이 너무 뻣뻣해서 %s 올라탈 수 없다.",
             uarm->oeroded ? "녹슨" : "부식된",
             append_josa(mon_nam(mtmp), "에"));
#endif
        return (FALSE);
    }
    if (!force
        && (Confusion || Fumbling || Glib || Wounded_legs || otmp->cursed
            || (u.ulevel + mtmp->mtame < rnd(MAXULEV / 2 + 5)))) {
        if (Levitation) {
            /*KR pline("%s slips away from you.", Monnam(mtmp)); */
            pline("%s 당신에게서 빠져나갔다.",
                  append_josa(Monnam(mtmp), "가"));
            return FALSE;
        }
        /*KR You("slip while trying to get on %s.", mon_nam(mtmp)); */
        You("%s 타려다 미끄러졌다.", append_josa(mon_nam(mtmp), "에"));

        /*KR Sprintf(buf, "slipped while mounting %s", */
        Sprintf(buf, "%s 타려다 미끄러져서",
                /* "a saddled mumak" or "a saddled pony called Dobbin" */
                x_monnam(mtmp, ARTICLE_A, (char *) 0,
                         SUPPRESS_IT | SUPPRESS_INVISIBLE
                             | SUPPRESS_HALLUCINATION,
                         TRUE));
        /*KR losehp(Maybe_Half_Phys(rn1(5, 10)), buf, NO_KILLER_PREFIX); */
        losehp(Maybe_Half_Phys(rn1(5, 10)), buf, KILLED_BY);
        return (FALSE);
    }

    /* Success */
    maybewakesteed(mtmp);
    if (!force) {
        if (Levitation && !is_floater(ptr) && !is_flyer(ptr))
            /* Must have Lev_at_will at this point */
            /*KR pline("%s magically floats up!", Monnam(mtmp)); */
            pline("%s 마법처럼 둥실 떠올랐다!",
                  append_josa(Monnam(mtmp), "가"));
        /*KR You("mount %s.", mon_nam(mtmp)); */
        You("%s 올라탔다.", append_josa(mon_nam(mtmp), "에"));
        if (Flying)
            /*KR You("and %s take flight together.", mon_nam(mtmp)); */
            You("%s 함께 비행을 시작했다.",
                append_josa(mon_nam(mtmp), "와"));
    }
    /* setuwep handles polearms differently when you're mounted */
    if (uwep && is_pole(uwep))
        unweapon = FALSE;
    u.usteed = mtmp;
    remove_monster(mtmp->mx, mtmp->my);
    teleds(mtmp->mx, mtmp->my, TRUE);
    context.botl = TRUE;
    return TRUE;
}

/* You and your steed have moved */
void
exercise_steed()
{
    if (!u.usteed)
        return;

    /* It takes many turns of riding to exercise skill */
    if (++u.urideturns >= 100) {
        u.urideturns = 0;
        use_skill(P_RIDING, 1);
    }
    return;
}

/* The player kicks or whips the steed */
void
kick_steed()
{
#if 0 /*KR: 원본*/
    char He[4];
#else /*KR: KRNethack 맞춤 번역 */
    /* role.c Gender.he 의 값이 복사된다 */
    char He[16];
#endif
    if (!u.usteed)
        return;

    /* [ALI] Various effects of kicking sleeping/paralyzed steeds */
    if (u.usteed->msleeping || !u.usteed->mcanmove) {
        /* We assume a message has just been output of the form
         * "You kick <steed>."
         */
        Strcpy(He, mhe(u.usteed));
        *He = highc(*He);
        if ((u.usteed->mcanmove || u.usteed->mfrozen) && !rn2(2)) {
            if (u.usteed->mcanmove)
                u.usteed->msleeping = 0;
            else if (u.usteed->mfrozen > 2)
                u.usteed->mfrozen -= 2;
            else {
                u.usteed->mfrozen = 0;
                u.usteed->mcanmove = 1;
            }
            if (u.usteed->msleeping || !u.usteed->mcanmove)
#if 0 /*KR: 원본*/
                pline("%s stirs.", He);
#else /*KR: KRNethack 맞춤 번역 */
                pline("%s 몸을 뒤척인다.", append_josa(He, "은"));
#endif
            else
#if 0 /*KR: 원본*/
                pline("%s rouses %sself!", He, mhim(u.usteed));
#else /*KR: KRNethack 맞춤 번역 */
                pline("%s 정신을 차렸다!", append_josa(He, "은"));
#endif
        } else
#if 0 /*KR: 원본*/
            pline("%s does not respond.", He);
#else /*KR: KRNethack 맞춤 번역 */
            pline("%s 아무 반응이 없다.", append_josa(He, "은"));
#endif
        return;
    }

    /* Make the steed less tame and check if it resists */
    if (u.usteed->mtame)
        u.usteed->mtame--;
    if (!u.usteed->mtame && u.usteed->mleashed)
        m_unleash(u.usteed, TRUE);
    if (!u.usteed->mtame
        || (u.ulevel + u.usteed->mtame < rnd(MAXULEV / 2 + 5))) {
        newsym(u.usteed->mx, u.usteed->my);
        dismount_steed(DISMOUNT_THROWN);
        return;
    }

    /*KR pline("%s gallops!", Monnam(u.usteed)); */
    pline("%s 질주한다!", append_josa(Monnam(u.usteed), "가"));
    u.ugallop += rn1(20, 30);
    return;
}

/*
 * Try to find a dismount point adjacent to the steed's location.
 * If all else fails, try enexto().  Use enexto() as a last resort because
 * enexto() chooses its point randomly, possibly even outside the
 * room's walls, which is not what we want.
 * Adapted from mail daemon code.
 */
STATIC_OVL boolean
landing_spot(spot, reason, forceit)
coord *spot; /* landing position (we fill it in) */
int reason;
int forceit;
{
    int i = 0, x, y, distance, min_distance = -1;
    boolean found = FALSE;
    struct trap *t;

    /* avoid known traps (i == 0) and boulders, but allow them as a backup */
    if (reason != DISMOUNT_BYCHOICE || Stunned || Confusion || Fumbling)
        i = 1;
    for (; !found && i < 2; ++i) {
        for (x = u.ux - 1; x <= u.ux + 1; x++)
            for (y = u.uy - 1; y <= u.uy + 1; y++) {
                if (!isok(x, y) || (x == u.ux && y == u.uy))
                    continue;

                if (accessible(x, y) && !MON_AT(x, y)) {
                    distance = distu(x, y);
                    if (min_distance < 0 || distance < min_distance
                        || (distance == min_distance && rn2(2))) {
                        if (i > 0
                            || (((t = t_at(x, y)) == 0 || !t->tseen)
                                && (!sobj_at(BOULDER, x, y)
                                    || throws_rocks(youmonst.data)))) {
                            spot->x = x;
                            spot->y = y;
                            min_distance = distance;
                            found = TRUE;
                        }
                    }
                }
            }
    }

    /* If we didn't find a good spot and forceit is on, try enexto(). */
    if (forceit && min_distance < 0
        && !enexto(spot, u.ux, u.uy, youmonst.data))
        return FALSE;

    return found;
}

/* Stop riding the current steed */
void dismount_steed(reason) int reason; /* Player was thrown off etc. */
{
    struct monst *mtmp;
    struct obj *otmp;
    coord cc, steedcc;
    /*KR const char *verb = "fall"; */
    const char *verb = "떨어졌다";
    boolean repair_leg_damage = (Wounded_legs != 0L);
    unsigned save_utrap = u.utrap;
    boolean have_spot = landing_spot(&cc, reason, 0);

    mtmp = u.usteed; /* make a copy of steed pointer */
    /* Sanity check */
    if (!mtmp) /* Just return silently */
        return;

    /* Check the reason for dismounting */
    otmp = which_armor(mtmp, W_SADDLE);
    switch (reason) {
    case DISMOUNT_THROWN:
        /*KR verb = "are thrown"; */
        verb = "내동댕이쳐졌다";
        /*FALLTHRU*/
    case DISMOUNT_FELL:
        /*KR You("%s off of %s!", verb, mon_nam(mtmp)); */
        You("%s에게서 %s!", mon_nam(mtmp), verb);
        if (!have_spot)
            have_spot = landing_spot(&cc, reason, 1);
        /*KR losehp(Maybe_Half_Phys(rn1(10, 10)), "riding accident",
         * KILLED_BY_AN); */
        losehp(Maybe_Half_Phys(rn1(10, 10)), "기승 사고로", KILLED_BY);
        set_wounded_legs(BOTH_SIDES, (int) HWounded_legs + rn1(5, 5));
        repair_leg_damage = FALSE;
        break;
    case DISMOUNT_POLY:
        /*KR You("can no longer ride %s.", mon_nam(u.usteed)); */
        You("더 이상 %s 탈 수 없게 되었다.",
            append_josa(mon_nam(u.usteed), "을"));
        if (!have_spot)
            have_spot = landing_spot(&cc, reason, 1);
        break;
    case DISMOUNT_ENGULFED:
        /* caller displays message */
        break;
    case DISMOUNT_BONES:
        /* hero has just died... */
        break;
    case DISMOUNT_GENERIC:
        /* no messages, just make it so */
        break;
    case DISMOUNT_BYCHOICE:
    default:
        if (otmp && otmp->cursed) {
#if 0 /*KR: 원본*/
            You("can't.  The saddle %s cursed.",
                otmp->bknown ? "is" : "seems to be");
#else /*KR: KRNethack 맞춤 번역 */
            You("내릴 수 없다. 안장이 저주받%s.",
                otmp->bknown ? "았다" : "은 것 같다");
#endif
            otmp->bknown = 1; /* ok to skip set_bknown() here */
            return;
        }
        if (!have_spot) {
            /*KR You("can't.  There isn't anywhere for you to stand."); */
            pline("내릴 수 없다. 설 만한 장소가 전혀 없다.");
            return;
        }
        if (!has_mname(mtmp)) {
            /*KR pline("You've been through the dungeon on %s with no name.",
                  an(mtmp->data->mname)); */
            pline("당신은 이름 없는 %s 함께 이 던전을 지나왔다.",
                  append_josa(an(mtmp->data->mname), "와"));
            if (Hallucination)
                /*KR pline("It felt good to get out of the rain."); */
                pline("비를 피할 수 있어 기분이 좋았다.");
        } else
            /*KR You("dismount %s.", mon_nam(mtmp)); */
            You("%s에게서 내렸다.", mon_nam(mtmp));
    }
    /* While riding, Wounded_legs refers to the steed's legs;
       after dismounting, it reverts to the hero's legs. */
    if (repair_leg_damage)
        heal_legs(1);

    /* Release the steed and saddle */
    u.usteed = 0;
    u.ugallop = 0L;
    /*
     * rloc(), rloc_to(), and monkilled()->mondead()->m_detach() all
     * expect mtmp to be on the map or else have mtmp->mx be 0, but
     * setting the latter to 0 here would interfere with dropping
     * the saddle.  Prior to 3.6.2, being off the map didn't matter.
     *
     * place_monster() expects mtmp to be alive and not be u.usteed.
     *
     * Unfortunately, <u.ux,u.uy> (former steed's implicit location)
     * might now be occupied by an engulfer, so we can't just put mtmp
     * at that spot.  An engulfer's previous spot will be unoccupied
     * but we don't know where that was and even if we did, it might
     * be hostile terrain.
     */
    steedcc.x = u.ux, steedcc.y = u.uy;
    if (m_at(u.ux, u.uy)) {
        /* hero's spot has a monster in it; hero must have been plucked
           from saddle as engulfer moved into his spot--other dismounts
           shouldn't run into this situation; find nearest viable spot */
        if (!enexto(&steedcc, u.ux, u.uy, mtmp->data)
            /* no spot? must have been engulfed by a lurker-above over
               water or lava; try requesting a location for a flyer */
            && !enexto(&steedcc, u.ux, u.uy, &mons[PM_BAT]))
            /* still no spot; last resort is any spot within bounds */
            (void) enexto(&steedcc, u.ux, u.uy, &mons[PM_GHOST]);
    }
    if (!m_at(steedcc.x, steedcc.y)) {
        if (mtmp->mhp < 1)
            mtmp->mhp = 0; /* make sure it isn't negative */
        mtmp->mhp++; /* force at least one hit point, possibly resurrecting */
        place_monster(mtmp, steedcc.x, steedcc.y);
        mtmp->mhp--; /* take the extra hit point away: cancel resurrection */
    } else {
        impossible("Dismounting: can't place former steed on map.");
    }

    if (!DEADMONSTER(mtmp)) {
        /* if for bones, there's no reason to place the hero;
           we want to make room for potential ghost, so move steed */
        if (reason == DISMOUNT_BONES) {
            /* move the steed to an adjacent square */
            if (enexto(&cc, u.ux, u.uy, mtmp->data))
                rloc_to(mtmp, cc.x, cc.y);
            else /* evidently no room nearby; move steed elsewhere */
                (void) rloc(mtmp, FALSE);
            return;
        }

        /* Set hero's and/or steed's positions.  Try moving the hero first. */
        if (!u.uswallow && !u.ustuck && have_spot) {
            struct permonst *mdat = mtmp->data;

            /* The steed may drop into water/lava */
            if (!is_flyer(mdat) && !is_floater(mdat) && !is_clinger(mdat)) {
                if (is_pool(u.ux, u.uy)) {
                    if (!Underwater)
                        /*KR pline("%s falls into the %s!", Monnam(mtmp),
                              surface(u.ux, u.uy)); */
                        pline("%s %s에 빠졌다!",
                              append_josa(Monnam(mtmp), "가"),
                              surface(u.ux, u.uy));
                    if (!is_swimmer(mdat) && !amphibious(mdat)) {
                        killed(mtmp);
                        adjalign(-1);
                    }
                } else if (is_lava(u.ux, u.uy)) {
#if 0 /*KR: 원본*/
                    pline("%s is pulled into the %s!", Monnam(mtmp),
                          hliquid("lava"));
#else /*KR: KRNethack 맞춤 번역 */
                    pline("%s %s 속으로 끌려 들어갔다!",
                          append_josa(Monnam(mtmp), "가"),
                          hliquid("용암"));
#endif
                    if (!likes_lava(mdat)) {
                        killed(mtmp);
                        adjalign(-1);
                    }
                }
            }
            /* Steed dismounting consists of two steps: being moved to another
             * square, and descending to the floor.  We have functions to do
             * each of these activities, but they're normally called
             * individually and include an attempt to look at or pick up the
             * objects on the floor:
             * teleds() --> spoteffects() --> pickup()
             * float_down() --> pickup()
             * We use this kludge to make sure there is only one such attempt.
             *
             * Clearly this is not the best way to do it.  A full fix would
             * involve having these functions not call pickup() at all,
             * instead
             * calling them first and calling pickup() afterwards.  But it
             * would take a lot of work to keep this change from having any
             * unforeseen side effects (for instance, you would no longer be
             * able to walk onto a square with a hole, and autopickup before
             * falling into the hole).
             */
            /* [ALI] No need to move the player if the steed died. */
            if (!DEADMONSTER(mtmp)) {
                /* Keep steed here, move the player to cc;
                 * teleds() clears u.utrap
                 */
                in_steed_dismounting = TRUE;
                teleds(cc.x, cc.y, TRUE);
                in_steed_dismounting = FALSE;

                /* Put your steed in your trap */
                if (save_utrap)
                    (void) mintrap(mtmp);
            }

            /* Couldn't move hero... try moving the steed. */
        } else if (enexto(&cc, u.ux, u.uy, mtmp->data)) {
            /* Keep player here, move the steed to cc */
            rloc_to(mtmp, cc.x, cc.y);
            /* Player stays put */

            /* Otherwise, kill the steed. */
        } else {
            if (reason == DISMOUNT_BYCHOICE) {
                /* [un]#ride: hero gets credit/blame for killing steed */
                killed(mtmp);
                adjalign(-1);
            } else {
                /* other dismount: kill former steed with no penalty;
                   damage type is just "neither AD_DGST nor -AD_RBRE" */
                monkilled(mtmp, "", -AD_PHYS);
            }
        }
    } /* !DEADMONST(mtmp) */

    /* usually return the hero to the surface */
    if (reason != DISMOUNT_ENGULFED && reason != DISMOUNT_BONES) {
        in_steed_dismounting = TRUE;
        (void) float_down(0L, W_SADDLE);
        in_steed_dismounting = FALSE;
        context.botl = TRUE;
        (void) encumber_msg();
        vision_full_recalc = 1;
    } else
        context.botl = TRUE;
    /* polearms behave differently when not mounted */
    if (uwep && is_pole(uwep))
        unweapon = TRUE;
    return;
}

/* when attempting to saddle or mount a sleeping steed, try to wake it up
   (for the saddling case, it won't be u.usteed yet) */
STATIC_OVL void maybewakesteed(steed) struct monst *steed;
{
    int frozen = (int) steed->mfrozen;
    boolean wasimmobile = steed->msleeping || !steed->mcanmove;

    steed->msleeping = 0;
    if (frozen) {
        frozen = (frozen + 1) / 2; /* half */
        /* might break out of timed sleep or paralysis */
        if (!rn2(frozen)) {
            steed->mfrozen = 0;
            steed->mcanmove = 1;
        } else {
            /* didn't awake, but remaining duration is halved */
            steed->mfrozen = frozen;
        }
    }
    if (wasimmobile && !steed->msleeping && steed->mcanmove)
        /*KR pline("%s wakes up.", Monnam(steed)); */
        pline("%s 깨어났다.", append_josa(Monnam(steed), "은"));
    /* regardless of waking, terminate any meal in progress */
    finish_meating(steed);
}

/* decide whether hero's steed is able to move;
   doesn't check for holding traps--those affect the hero directly */
boolean
stucksteed(checkfeeding)
boolean checkfeeding;
{
    struct monst *steed = u.usteed;

    if (steed) {
        /* check whether steed can move */
        if (steed->msleeping || !steed->mcanmove) {
            /*KR pline("%s won't move!", upstart(y_monnam(steed))); */
            pline("%s 움직이지 않는다!",
                  append_josa(upstart(y_monnam(steed)), "은"));
            return TRUE;
        }
        /* optionally check whether steed is in the midst of a meal */
        if (checkfeeding && steed->meating) {
            /*KR pline("%s is still eating.", upstart(y_monnam(steed))); */
            pline("%s 아직도 먹고 있다.",
                  append_josa(upstart(y_monnam(steed)), "은"));
            return TRUE;
        }
    }
    return FALSE;
}

void place_monster(mon, x, y) struct monst *mon;
int x, y;
{
    struct monst *othermon;
    const char *monnm, *othnm;
    char buf[QBUFSZ];

    buf[0] = '\0';
    /* normal map bounds are <1..COLNO-1,0..ROWNO-1> but sometimes
       vault guards (either living or dead) are parked at <0,0> */
    if (!isok(x, y) && (x != 0 || y != 0 || !mon->isgd)) {
        describe_level(buf);
        impossible("trying to place %s at <%d,%d> mstate:%lx on %s",
                   minimal_monnam(mon, TRUE), x, y, mon->mstate, buf);
        x = y = 0;
    }
    if (mon == u.usteed
        /* special case is for convoluted vault guard handling */
        || (DEADMONSTER(mon) && !(mon->isgd && x == 0 && y == 0))) {
        describe_level(buf);
        impossible("placing %s onto map, mstate:%lx, on %s?",
                   (mon == u.usteed) ? "steed" : "defunct monster",
                   mon->mstate, buf);
        return;
    }
    if ((othermon = level.monsters[x][y]) != 0) {
        describe_level(buf);
        monnm = minimal_monnam(mon, FALSE);
        othnm = (mon != othermon) ? minimal_monnam(othermon, TRUE) : "itself";
        impossible("placing %s over %s at <%d,%d>, mstates:%lx %lx on %s?",
                   monnm, othnm, x, y, othermon->mstate, mon->mstate, buf);
    }
    mon->mx = x, mon->my = y;
    level.monsters[x][y] = mon;
    mon->mstate &= ~(MON_OFFMAP | MON_MIGRATING | MON_LIMBO | MON_BUBBLEMOVE
                     | MON_ENDGAME_FREE | MON_ENDGAME_MIGR);
}

/*steed.c*/
