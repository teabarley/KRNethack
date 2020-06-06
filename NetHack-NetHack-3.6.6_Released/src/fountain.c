/* NetHack 3.6	fountain.c	$NHDT-Date: 1544442711 2018/12/10 11:51:51 $  $NHDT-Branch: NetHack-3.6.2-beta01 $:$NHDT-Revision: 1.60 $ */
/*      Copyright Scott R. Turner, srt@ucla, 10/27/86 */
/* NetHack may be freely redistributed.  See license for details. */

/* Code for drinking from fountains. */

#include "hack.h"

STATIC_DCL void NDECL(dowatersnakes);
STATIC_DCL void NDECL(dowaterdemon);
STATIC_DCL void NDECL(dowaternymph);
STATIC_PTR void FDECL(gush, (int, int, genericptr_t));
STATIC_DCL void NDECL(dofindgem);

/* used when trying to dip in or drink from fountain or sink or pool while
   levitating above it, or when trying to move downwards in that state */
void
floating_above(what)
const char *what;
{
    /*KR const char *umsg = "are floating high above the %s."; */
    const char *umsg = "%s의 위에 높이 떠 있다.";

    if (u.utrap && (u.utraptype == TT_INFLOOR || u.utraptype == TT_LAVA)) {
        /* when stuck in floor (not possible at fountain or sink location,
           so must be attempting to move down), override the usual message */
        /*KR umsg = "are trapped in the %s."; */
        umsg = "%s에 붙잡혀 있다.";
        what = surface(u.ux, u.uy); /* probably redundant */
    }
    You(umsg, what);
}

/* Fountain of snakes! */
STATIC_OVL void
dowatersnakes()
{
    register int num = rn1(5, 2);
    struct monst *mtmp;

    if (!(mvitals[PM_WATER_MOCCASIN].mvflags & G_GONE)) {
        if (!Blind)
#if 0 /*KR:T*/
            pline("An endless stream of %s pours forth!",
                  Hallucination ? makeplural(rndmonnam(NULL)) : "snakes");
#else
            pline("%s들이 끝없이 쏟아져 나온다!",
                Hallucination ? rndmonnam(NULL) : "뱀");
#endif
        else
            /*KR You_hear("%s hissing!", something); */
            You_hear("무언가가 쉬익하는 소리를 듣는다!");
        while (num-- > 0)
            if ((mtmp = makemon(&mons[PM_WATER_MOCCASIN], u.ux, u.uy,
                                NO_MM_FLAGS)) != 0
                && t_at(mtmp->mx, mtmp->my))
                (void) mintrap(mtmp);
    } else
        /*KR pline_The("fountain bubbles furiously for a moment, then calms."); */
        pline("분수가 잠시동안 격렬히 끓어올랐지만, 이윽고 잠잠해졌다.");
}

/* Water demon */
STATIC_OVL void
dowaterdemon()
{
    struct monst *mtmp;

    if (!(mvitals[PM_WATER_DEMON].mvflags & G_GONE)) {
        if ((mtmp = makemon(&mons[PM_WATER_DEMON], u.ux, u.uy,
                            NO_MM_FLAGS)) != 0) {
            if (!Blind)
                /*KR You("unleash %s!", a_monnam(mtmp));
            else
                You_feel("the presence of evil."); */

                You("%s을 해방시켰다!", a_monnam(mtmp));
            else
                You_feel("악마의 존재를 느낀다.");

            /* Give those on low levels a (slightly) better chance of survival
             */
            if (rnd(100) > (80 + level_difficulty())) {
#if 0 /*KR:T*/
                pline("Grateful for %s release, %s grants you a wish!",
                      mhis(mtmp), mhe(mtmp));
#else
                pline("%s 해방에 고마워하며, %s가 당신의 소원을 이루어 준다!",
                   mhis(mtmp), mhe(mtmp));
#endif
                /* give a wish and discard the monster (mtmp set to null) */
                mongrantswish(&mtmp);
            } else if (t_at(mtmp->mx, mtmp->my))
                (void) mintrap(mtmp);
        }
    } else
        /*KR pline_The("fountain bubbles furiously for a moment, then calms."); */
        pline("분수가 잠시동안 격렬히 끓어올랐지만, 이윽고 잠잠해졌다.");
}

/* Water Nymph */
STATIC_OVL void
dowaternymph()
{
    register struct monst *mtmp;

    if (!(mvitals[PM_WATER_NYMPH].mvflags & G_GONE)
        && (mtmp = makemon(&mons[PM_WATER_NYMPH], u.ux, u.uy,
                           NO_MM_FLAGS)) != 0) {
        if (!Blind)
            /*KR You("attract %s!", a_monnam(mtmp));
        else
            You_hear("a seductive voice."); */
            You("%s를 끌어들인다!", a_monnam(mtmp));
        else
            You_hear("유혹적인 목소리를 듣는다.");
        mtmp->msleeping = 0;
        if (t_at(mtmp->mx, mtmp->my))
            (void) mintrap(mtmp);
    } else if (!Blind)
        /*KR pline("A large bubble rises to the surface and pops.");
    else
        You_hear("a loud pop."); */
        pline("커다란 거품이 표면으로 올라와 터진다.");
    else
        You_hear("커다란 펑 소리를 듣는다.");
}

/* Gushing forth along LOS from (u.ux, u.uy) */
void
dogushforth(drinking)
int drinking;
{
    int madepool = 0;

    do_clear_area(u.ux, u.uy, 7, gush, (genericptr_t) &madepool);
    if (!madepool) {
        if (drinking)
            /*KR Your("thirst is quenched.");
        else
            pline("Water sprays all over you."); */
            Your("갈증이 해소되었다.");
        else
            pline("물이 당신의 온몸에 뿌려진다.");
    }
}

STATIC_PTR void
gush(x, y, poolcnt)
int x, y;
genericptr_t poolcnt;
{
    register struct monst *mtmp;
    register struct trap *ttmp;

    if (((x + y) % 2) || (x == u.ux && y == u.uy)
        || (rn2(1 + distmin(u.ux, u.uy, x, y))) || (levl[x][y].typ != ROOM)
        || (sobj_at(BOULDER, x, y)) || nexttodoor(x, y))
        return;

    if ((ttmp = t_at(x, y)) != 0 && !delfloortrap(ttmp))
        return;

    if (!((*(int *) poolcnt)++))
        /*KR pline("Water gushes forth from the overflowing fountain!"); */
        pline("넘쳐 흐르는 분수에서 물이 앞으로 쏟아져 나온다!");

    /* Put a pool at x, y */
    levl[x][y].typ = POOL, levl[x][y].flags = 0;
    /* No kelp! */
    del_engr_at(x, y);
    water_damage_chain(level.objects[x][y], TRUE);

    if ((mtmp = m_at(x, y)) != 0)
        (void) minliquid(mtmp);
    else
        newsym(x, y);
}

/* Find a gem in the sparkling waters. */
STATIC_OVL void
dofindgem()
{
    if (!Blind)
        /*KR You("spot a gem in the sparkling waters!");
    else
        You_feel("a gem here!"); */
        You("부글거리는 물 속에서 보석을 발견한다!");
    else
        You_feel("여기서 보석을 느낀다!");
    (void) mksobj_at(rnd_class(DILITHIUM_CRYSTAL, LUCKSTONE - 1), u.ux, u.uy,
                     FALSE, FALSE);
    SET_FOUNTAIN_LOOTED(u.ux, u.uy);
    newsym(u.ux, u.uy);
    exercise(A_WIS, TRUE); /* a discovery! */
}

void
dryup(x, y, isyou)
xchar x, y;
boolean isyou;
{
    if (IS_FOUNTAIN(levl[x][y].typ)
        && (!rn2(3) || FOUNTAIN_IS_WARNED(x, y))) {
        if (isyou && in_town(x, y) && !FOUNTAIN_IS_WARNED(x, y)) {
            struct monst *mtmp;

            SET_FOUNTAIN_WARNED(x, y);
            /* Warn about future fountain use. */
            for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
                if (DEADMONSTER(mtmp))
                    continue;
                if (is_watch(mtmp->data) && couldsee(mtmp->mx, mtmp->my)
                    && mtmp->mpeaceful) {
                    if (!Deaf) {
                        /*KR pline("%s yells:", Amonnam(mtmp));
                        verbalize("Hey, stop using that fountain!"); */
                        pline("%s가 소리쳤다:", Amonnam(mtmp));
                        verbalize("이봐, 당장 그 분수 사용을 그만둬!");
                    } else {
#if 0 /*KR:T*/
                        pline("%s earnestly %s %s %s!",
                              Amonnam(mtmp),
                              nolimbs(mtmp->data) ? "shakes" : "waves",
                              mhis(mtmp),
                              nolimbs(mtmp->data)
                                      ? mbodypart(mtmp, HEAD)
                                      : makeplural(mbodypart(mtmp, ARM)));
#else
                        pline("%s은/는 진지하게 %s을/를 흔들었다!",
                            Amonnam(mtmp),
                            nolimbs(mtmp->data)
                                    ? mbodypart(mtmp, HEAD)
                                    : makeplural(mbodypart(mtmp, ARM)));
#endif
                    }
                    break;
                }
            }
            /* You can see or hear this effect */
            if (!mtmp)
                /*KR pline_The("flow reduces to a trickle."); */
                pline("물줄기가 가늘어졌다.");
            return;
        }
        if (isyou && wizard) {
            /*KR if (yn("Dry up fountain?") == 'n') */
            if (yn("분수를 말리시겠습니까?") == 'n')
                return;
        }
        /* replace the fountain with ordinary floor */
        levl[x][y].typ = ROOM, levl[x][y].flags = 0;
        levl[x][y].blessedftn = 0;
        if (cansee(x, y))
            /*KR pline_The("fountain dries up!"); */
            pline("분수가 말라버렸다!");
        /* The location is seen if the hero/monster is invisible
           or felt if the hero is blind. */
        newsym(x, y);
        level.flags.nfountains--;
        if (isyou && in_town(x, y))
            (void) angry_guards(FALSE);
    }
}

void
drinkfountain()
{
    /* What happens when you drink from a fountain? */
    register boolean mgkftn = (levl[u.ux][u.uy].blessedftn == 1);
    register int fate = rnd(30);

    if (Levitation) {
        /*KR floating_above("fountain"); */
        floating_above("분수");
        return;
    }

    if (mgkftn && u.uluck >= 0 && fate >= 10) {
        int i, ii, littleluck = (u.uluck < 4);

        /*KR pline("Wow!  This makes you feel great!"); */
        pline("와! 이것은 당신을 기분좋게 만든다!");
        /* blessed restore ability */
        for (ii = 0; ii < A_MAX; ii++)
            if (ABASE(ii) < AMAX(ii)) {
                ABASE(ii) = AMAX(ii);
                context.botl = 1;
            }
        /* gain ability, blessed if "natural" luck is high */
        i = rn2(A_MAX); /* start at a random attribute */
        for (ii = 0; ii < A_MAX; ii++) {
            if (adjattrib(i, 1, littleluck ? -1 : 0) && littleluck)
                break;
            if (++i >= A_MAX)
                i = 0;
        }
        display_nhwindow(WIN_MESSAGE, FALSE);
        /*KR pline("A wisp of vapor escapes the fountain..."); */
        pline("분수에서 한 줄기의 수증기가 빠져나간다...");
        exercise(A_WIS, TRUE);
        levl[u.ux][u.uy].blessedftn = 0;
        return;
    }

    if (fate < 10) {
        /*KR pline_The("cool draught refreshes you."); */
        pline("시원한 한 모금이 당신을 상쾌하게 한다.");
        u.uhunger += rnd(10); /* don't choke on water */
        newuhs(FALSE);
        if (mgkftn)
            return;
    } else {
        switch (fate) {
        case 19: /* Self-knowledge */
            /*KR You_feel("self-knowledgeable..."); */
            You("자각하고 있음을 느낀다...");
            display_nhwindow(WIN_MESSAGE, FALSE);
            enlightenment(MAGICENLIGHTENMENT, ENL_GAMEINPROGRESS);
            exercise(A_WIS, TRUE);
            /*KR pline_The("feeling subsides."); */
            pline("느낌이 가라앉는다.");
            break;
        case 20: /* Foul water */
            /*KR pline_The("water is foul!  You gag and vomit."); */
            pline("이 물은 상했다! 당신은 메스꺼워서 구토한다.");
            morehungry(rn1(20, 11));
            vomit();
            break;
        case 21: /* Poisonous */
            /*KR pline_The("water is contaminated!"); */
            pline("이 물은 오염되었다!");
            if (Poison_resistance) {
                /*KR pline("Perhaps it is runoff from the nearby %s farm.", */
                pline("어쩌면 이 물은 근처의 %s 농장에서 흘러나온 물일 지도 모른다.",
                      fruitname(FALSE));
                /*KR losehp(rnd(4), "unrefrigerated sip of juice", KILLED_BY_AN); */
                losehp(rnd(4), "냉장 보관하지 않은 주스 한 모금에", KILLED_BY_AN);
                break;
            }
            losestr(rn1(4, 3));
            /*KR losehp(rnd(10), "contaminated water", KILLED_BY); */
            losehp(rnd(10), "오염된 물로", KILLED_BY);
            exercise(A_CON, FALSE);
            break;
        case 22: /* Fountain of snakes! */
            dowatersnakes();
            break;
        case 23: /* Water demon */
            dowaterdemon();
            break;
        case 24: /* Curse an item */ {
            register struct obj *obj;

            /*KR pline("This water's no good!"); */
            pline("이 물은 오염되었다!");
            morehungry(rn1(20, 11));
            exercise(A_CON, FALSE);
            for (obj = invent; obj; obj = obj->nobj)
                if (!rn2(5))
                    curse(obj);
            break;
        }
        case 25: /* See invisible */
            if (Blind) {
                if (Invisible) {
                    /*KR You("feel transparent.");
                } else {
                    You("feel very self-conscious.");
                    pline("Then it passes."); */
                    You("당신은 투명해짐을 느낀다.");
                } else {
                    You("당신은 자신의 존재를 매우 강하게 느꼈으나,");
                    pline("이윽고 사라졌다.");
                }
            } else {
                /*KR You_see("an image of someone stalking you.");
                pline("But it disappears."); */
                You("당신을 미행하는 누군가의 형상을 보았으나, ");
                pline("이윽고 사라졌다.");
            }
            HSee_invisible |= FROMOUTSIDE;
            newsym(u.ux, u.uy);
            exercise(A_WIS, TRUE);
            break;
        case 26: /* See Monsters */
            (void) monster_detect((struct obj *) 0, 0);
            exercise(A_WIS, TRUE);
            break;
        case 27: /* Find a gem in the sparkling waters. */
            if (!FOUNTAIN_IS_LOOTED(u.ux, u.uy)) {
                dofindgem();
                break;
            }
            /*FALLTHRU*/
        case 28: /* Water Nymph */
            dowaternymph();
            break;
        case 29: /* Scare */
        {
            register struct monst *mtmp;

#if 0 /*KR:T*/
            pline("This %s gives you bad breath!",
                hliquid("water"));
#else
            pline("이 %s 때문에 지독한 입냄새가 난다!",
                hliquid("물"));
#endif
            for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
                if (DEADMONSTER(mtmp))
                    continue;
                monflee(mtmp, 0, FALSE, FALSE);
            }
            break;
        }
        case 30: /* Gushing forth in this room */
            dogushforth(TRUE);
            break;
        default:
#if 0 /*KR:T*/
            pline("This tepid %s is tasteless.",
                hliquid("water"));
#else
            pline("이 미지근한 %s은 아무 맛도 나지 않는다.",
                hliquid("물"));
#endif
            break;
        }
    }
    dryup(u.ux, u.uy, TRUE);
}

void
dipfountain(obj)
register struct obj *obj;
{
    if (Levitation) {
        /*KR floating_above("fountain"); */
        floating_above("분수");
        return;
    }

    /* Don't grant Excalibur when there's more than one object.  */
    /* (quantity could be > 1 if merged daggers got polymorphed) */
    if (obj->otyp == LONG_SWORD && obj->quan == 1L && u.ulevel >= 5 && !rn2(6)
        && !obj->oartifact
        && !exist_artifact(LONG_SWORD, artiname(ART_EXCALIBUR))) {
        if (u.ualign.type != A_LAWFUL) {
            /* Ha!  Trying to cheat her. */
#if 0 /*KR:T*/
            pline("A freezing mist rises from the %s and envelopes the sword.",
                hliquid("water"));
            pline_The("fountain disappears!");
#else
            pline("%s에서 냉랭한 안개가 올라와 칼을 감싼다.",
                hliquid("물"));
            pline("분수가 사라진다!");
#endif
            curse(obj);
            if (obj->spe > -6 && !rn2(3))
                obj->spe--;
            obj->oerodeproof = FALSE;
            exercise(A_WIS, FALSE);
        } else {
            /* The lady of the lake acts! - Eric Backus */
            /* Be *REAL* nice */
            /*KR pline("From the murky depths, a hand reaches up to bless the sword.");
            pline("As the hand retreats, the fountain disappears!"); */
            pline("탁한 물 속에서 칼을 축복하기 위해 손이 뻗어나왔다.");
            pline("손이 되돌아가며, 분수가 사라진다!");
            obj = oname(obj, artiname(ART_EXCALIBUR));
            discover_artifact(ART_EXCALIBUR);
            bless(obj);
            obj->oeroded = obj->oeroded2 = 0;
            obj->oerodeproof = TRUE;
            exercise(A_WIS, TRUE);
        }
        update_inventory();
        levl[u.ux][u.uy].typ = ROOM, levl[u.ux][u.uy].flags = 0;
        newsym(u.ux, u.uy);
        level.flags.nfountains--;
        if (in_town(u.ux, u.uy))
            (void) angry_guards(FALSE);
        return;
    } else {
        int er = water_damage(obj, NULL, TRUE);

        if (obj->otyp == POT_ACID
            && er != ER_DESTROYED) { /* Acid and water don't mix */
            useup(obj);
            return;
        } else if (er != ER_NOTHING && !rn2(2)) { /* no further effect */
            return;
        }
    }

    switch (rnd(30)) {
    case 16: /* Curse the item */
        curse(obj);
        break;
    case 17:
    case 18:
    case 19:
    case 20: /* Uncurse the item */
        if (obj->cursed) {
            if (!Blind)
                /*KR pline_The("%s glows for a moment.", hliquid("water")); */
                pline_The("%s이 잠시 빛난다.", hliquid("물"));
            uncurse(obj);
        } else {
            /*KR pline("A feeling of loss comes over you."); */
            pline("무언가 잃어버린 기분이 흘러들어온다.");
        }
        break;
    case 21: /* Water Demon */
        dowaterdemon();
        break;
    case 22: /* Water Nymph */
        dowaternymph();
        break;
    case 23: /* an Endless Stream of Snakes */
        dowatersnakes();
        break;
    case 24: /* Find a gem */
        if (!FOUNTAIN_IS_LOOTED(u.ux, u.uy)) {
            dofindgem();
            break;
        }
        /*FALLTHRU*/
    case 25: /* Water gushes forth */
        dogushforth(FALSE);
        break;
    case 26: /* Strange feeling */
        /*KR pline("A strange tingling runs up your %s.", body_part(ARM)); */
        pline("이상한 저림이 당신의 %s을 타고 올라온다.", body_part(ARM));
        break;
    case 27: /* Strange feeling */
        /*KR You_feel("a sudden chill."); */
        You("갑자기 오한을 느낀다.");
        break;
    case 28: /* Strange feeling */
        /*KR pline("An urge to take a bath overwhelms you."); */
        pline("당신은 목욕을 하고 싶은 욕망에 휩싸인다.");
        {
            long money = money_cnt(invent);
            struct obj *otmp;
            if (money > 10) {
                /* Amount to lose.  Might get rounded up as fountains don't
                 * pay change... */
                money = somegold(money) / 10;
                for (otmp = invent; otmp && money > 0; otmp = otmp->nobj)
                    if (otmp->oclass == COIN_CLASS) {
                        int denomination = objects[otmp->otyp].oc_cost;
                        long coin_loss =
                            (money + denomination - 1) / denomination;
                        coin_loss = min(coin_loss, otmp->quan);
                        otmp->quan -= coin_loss;
                        money -= coin_loss * denomination;
                        if (!otmp->quan)
                            delobj(otmp);
                    }
                /*KR You("lost some of your money in the fountain!"); */
                You("당신은 분수에서 금화를 조금 잃었다!");
                CLEAR_FOUNTAIN_LOOTED(u.ux, u.uy);
                exercise(A_WIS, FALSE);
            }
        }
        break;
    case 29: /* You see coins */
        /* We make fountains have more coins the closer you are to the
         * surface.  After all, there will have been more people going
         * by.  Just like a shopping mall!  Chris Woodbury  */

        if (FOUNTAIN_IS_LOOTED(u.ux, u.uy))
            break;
        SET_FOUNTAIN_LOOTED(u.ux, u.uy);
        (void) mkgold((long) (rnd((dunlevs_in_dungeon(&u.uz) - dunlev(&u.uz)
                                   + 1) * 2) + 5),
                      u.ux, u.uy);
        if (!Blind)
#if 0 /*KR:T*/
            pline("Far below you, you see coins glistening in the %s.",
                hliquid("water"));
#else
            pline("당신은 발밑 너머 %s 속에서 동전이 반짝이는 것을 본다.",
                hliquid("물"));
#endif
        exercise(A_WIS, TRUE);
        newsym(u.ux, u.uy);
        break;
    }
    update_inventory();
    dryup(u.ux, u.uy, TRUE);
}

void
breaksink(x, y)
int x, y;
{
    if (cansee(x, y) || (x == u.ux && y == u.uy))
        /*KR pline_The("pipes break!  Water spurts out!"); */
        pline("배수관이 부서졌다! 물이 분출해 나온다!");
    level.flags.nsinks--;
    levl[x][y].typ = FOUNTAIN, levl[x][y].looted = 0;
    levl[x][y].blessedftn = 0;
    SET_FOUNTAIN_LOOTED(x, y);
    level.flags.nfountains++;
    newsym(x, y);
}

void
drinksink()
{
    struct obj *otmp;
    struct monst *mtmp;

    if (Levitation) {
        /*KR floating_above("sink"); */
        floating_above("개수대");
        return;
    }
    switch (rn2(20)) {
    case 0:
        /*KR You("take a sip of very cold %s.", hliquid("water")); */
        You("매우 차가운 %s을 홀짝였다.", hliquid("물"));
        break;
    case 1:
        /*KR You("take a sip of very warm %s.", hliquid("water")); */
        You("아주 따뜻한 %s을 홀짝였다.", hliquid("물"));
        break;
    case 2:
        /*KR You("take a sip of scalding hot %s.", hliquid("water")); */
        You("펄펄 끓는 %s을 홀짝였다.", hliquid("물"));
        if (Fire_resistance)
            /*KR pline("It seems quite tasty."); */
            pline("꽤나 맛있는 것 같다.");
        else
            /*KR losehp(rnd(6), "sipping boiling water", KILLED_BY); */
            losehp(rnd(6), "끓는 물을 홀짝여서", KILLED_BY);
        /* boiling water burns considered fire damage */
        break;
    case 3:
        if (mvitals[PM_SEWER_RAT].mvflags & G_GONE)
            /*KR pline_The("sink seems quite dirty."); */
            pline("개수구가 매우 더러워 보인다.");
        else {
            mtmp = makemon(&mons[PM_SEWER_RAT], u.ux, u.uy, NO_MM_FLAGS);
            if (mtmp)
#if 0 /*KR:T*/
                pline("Eek!  There's %s in the sink!",
                    (Blind || !canspotmon(mtmp)) ? "something squirmy"
                    : a_monnam(mtmp));
#else
                pline("꺄악! 개수구에 %s이/가 있어!",
                    (Blind || !canspotmon(mtmp)) ? "뭔가 꿈틀대는 것"
                    : a_monnam(mtmp));
#endif
        }
        break;
    case 4:
        do {
            otmp = mkobj(POTION_CLASS, FALSE);
            if (otmp->otyp == POT_WATER) {
                obfree(otmp, (struct obj *) 0);
                otmp = (struct obj *) 0;
            }
        } while (!otmp);
        otmp->cursed = otmp->blessed = 0;
#if 0 /*KR:T*/
        pline("Some %s liquid flows from the faucet.",
            Blind ? "odd" : hcolor(OBJ_DESCR(objects[otmp->otyp])));
#else
        pline("수도꼭지에서 %s액체가 흘러나온다.",
            Blind ? "왠 이상한 " :
            hcolor(OBJ_DESCR(objects[otmp->otyp])));
#endif
        otmp->dknown = !(Blind || Hallucination);
        otmp->quan++;       /* Avoid panic upon useup() */
        otmp->fromsink = 1; /* kludge for docall() */
        (void) dopotion(otmp);
        obfree(otmp, (struct obj *) 0);
        break;
    case 5:
        if (!(levl[u.ux][u.uy].looted & S_LRING)) {
            /*KR You("find a ring in the sink!"); */
            You("개수대에서 반지를 발견했다!");
            (void) mkobj_at(RING_CLASS, u.ux, u.uy, TRUE);
            levl[u.ux][u.uy].looted |= S_LRING;
            exercise(A_WIS, TRUE);
            newsym(u.ux, u.uy);
        } else
            /*KR pline("Some dirty %s backs up in the drain.", hliquid("water")); */
            pline("배수구에서 더러운 %s이 역류해 나왔다.", hliquid("물"));
        break;
    case 6:
        breaksink(u.ux, u.uy);
        break;
    case 7:
        /*KR pline_The("%s moves as though of its own will!", hliquid("water")); */
        pline_The("%s이 자유의지를 가지고 움직인다!", hliquid("물"));
        if ((mvitals[PM_WATER_ELEMENTAL].mvflags & G_GONE)
            || !makemon(&mons[PM_WATER_ELEMENTAL], u.ux, u.uy, NO_MM_FLAGS))
            /*KR pline("But it quiets down."); */
            pline("하지만 이내 잠잠해졌다.");
        break;
    case 8:
        /*KR pline("Yuk, this %s tastes awful.", hliquid("water")); */
        pline("우욱, 이 %s은 맛이 끔찍하다.", hliquid("물"));
        more_experienced(1, 0);
        newexplevel();
        break;
    case 9:
        /*KR pline("Gaggg... this tastes like sewage!  You vomit."); */
        pline("으웨엑... 하수 맛이 난다! 당신은 토한다.");
        morehungry(rn1(30 - ACURR(A_CON), 11));
        vomit();
        break;
    case 10:
        /*KR pline("This %s contains toxic wastes!", hliquid("water")); */
        pline("이 %s에는 유독성 폐기물이 들어 있다!", hliquid("물"));
        if (!Unchanging) {
            /*KR You("undergo a freakish metamorphosis!"); */
            You("기묘한 변화를 겪는다!");
            polyself(0);
        }
        break;
    /* more odd messages --JJB */
    case 11:
        /*KR You_hear("clanking from the pipes..."); */
        You_hear("배수관에서 절꺽거리는 소리가 나는 것을 듣는다...");
        break;
    case 12:
        /*KR You_hear("snatches of song from among the sewers..."); */
        You_hear("하수도에 울려퍼지는 노래 몇 가닥을 듣는다...");
        break;
    case 19:
        if (Hallucination) {
            /*KR pline("From the murky drain, a hand reaches up... --oops--"); */
            pline("어두운 배수구 속에서, 손이 뻗어나왔다... --어이쿠 손이 미끄러졌네--");
            break;
        }
        /*FALLTHRU*/
    default:
#if 0 /*KR:T*/
        You("take a sip of %s %s.",
            rn2(3) ? (rn2(2) ? "cold" : "warm") : "hot",
            hliquid("water"));
#else
        You("%s %s을 홀짝였다.",
            rn2(3) ? (rn2(2) ? "차가운" : "따뜻한") : "뜨거운",
            hliquid("물"));
#endif
    }
}

/*fountain.c*/
