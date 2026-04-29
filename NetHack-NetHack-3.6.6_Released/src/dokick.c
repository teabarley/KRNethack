/* NetHack 3.6	dokick.c	$NHDT-Date: 1575245057 2019/12/02 00:04:17 $  $NHDT-Branch: NetHack-3.6 $:$NHDT-Revision: 1.136 $ */
/* Copyright (c) Izchak Miller, Mike Stephenson, Steve Linhart, 1989. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

#define is_bigfoot(x) ((x) == &mons[PM_SASQUATCH])
#define martial()                                 \
    (martial_bonus() || is_bigfoot(youmonst.data) \
     || (uarmf && uarmf->otyp == KICKING_BOOTS))

static NEARDATA struct rm *maploc, nowhere;
static NEARDATA const char *gate_str;

/* kickedobj (decl.c) tracks a kicked object until placed or destroyed */

extern boolean notonhead; /* for long worms */

STATIC_DCL void FDECL(kickdmg, (struct monst *, BOOLEAN_P));
STATIC_DCL boolean FDECL(maybe_kick_monster,
                         (struct monst *, XCHAR_P, XCHAR_P));
STATIC_DCL void FDECL(kick_monster, (struct monst *, XCHAR_P, XCHAR_P));
STATIC_DCL int FDECL(kick_object, (XCHAR_P, XCHAR_P, char *) );
STATIC_DCL int FDECL(really_kick_object, (XCHAR_P, XCHAR_P));
STATIC_DCL char *FDECL(kickstr, (char *, const char *) );
STATIC_DCL void FDECL(otransit_msg, (struct obj *, BOOLEAN_P, long) );
STATIC_DCL void FDECL(drop_to, (coord *, SCHAR_P));

/*KR static const char kick_passes_thru[] = "kick passes harmlessly through";
 */
static const char kick_passes_thru[] =
    "당신의 발차기는 아무런 해도 입히지 못하고 그대로 통과했다";

/* kicking damage when not poly'd into a form with a kick attack */
STATIC_OVL void kickdmg(mon, clumsy) struct monst *mon;
boolean clumsy;
{
    int mdx, mdy;
    int dmg = (ACURRSTR + ACURR(A_DEX) + ACURR(A_CON)) / 15;
    int specialdmg, kick_skill = P_NONE;
    boolean trapkilled = FALSE;

    if (uarmf && uarmf->otyp == KICKING_BOOTS)
        dmg += 5;

    /* excessive wt affects dex, so it affects dmg */
    if (clumsy)
        dmg /= 2;

    /* kicking a dragon or an elephant will not harm it */
    if (thick_skinned(mon->data))
        dmg = 0;

    /* attacking a shade is normally useless */
    if (mon->data == &mons[PM_SHADE])
        dmg = 0;

    specialdmg = special_dmgval(&youmonst, mon, W_ARMF, (long *) 0);

    if (mon->data == &mons[PM_SHADE] && !specialdmg) {
        /*KR pline_The("%s.", kick_passes_thru); */
        pline("%s.", kick_passes_thru);
        /* doesn't exercise skill or abuse alignment or frighten pet,
           and shades have no passive counterattack */
        return;
    }

    if (M_AP_TYPE(mon))
        seemimic(mon);

    check_caitiff(mon);

    /* squeeze some guilt feelings... */
    if (mon->mtame) {
        abuse_dog(mon);
        if (mon->mtame)
            monflee(mon, (dmg ? rnd(dmg) : 1), FALSE, FALSE);
        else
            mon->mflee = 0;
    }

    if (dmg > 0) {
        /* convert potential damage to actual damage */
        dmg = rnd(dmg);
        if (martial()) {
            if (dmg > 1)
                kick_skill = P_MARTIAL_ARTS;
            dmg += rn2(ACURR(A_DEX) / 2 + 1);
        }
        /* a good kick exercises your dex */
        exercise(A_DEX, TRUE);
    }
    dmg += specialdmg; /* for blessed (or hypothetically, silver) boots */
    if (uarmf)
        dmg += uarmf->spe;
    dmg += u.udaminc; /* add ring(s) of increase damage */
    if (dmg > 0)
        mon->mhp -= dmg;
    if (!DEADMONSTER(mon) && martial() && !bigmonst(mon->data) && !rn2(3)
        && mon->mcanmove && mon != u.ustuck && !mon->mtrapped) {
        /* see if the monster has a place to move into */
        mdx = mon->mx + u.dx;
        mdy = mon->my + u.dy;
        if (goodpos(mdx, mdy, mon, 0)) {
            /*KR pline("%s reels from the blow.", Monnam(mon)); */
            pline("%s 당신의 발차기에 휘청거렸다.",
                  append_josa(Monnam(mon), "은"));
            if (m_in_out_region(mon, mdx, mdy)) {
                remove_monster(mon->mx, mon->my);
                newsym(mon->mx, mon->my);
                place_monster(mon, mdx, mdy);
                newsym(mon->mx, mon->my);
                set_apparxy(mon);
                if (mintrap(mon) == 2)
                    trapkilled = TRUE;
            }
        }
    }

    (void) passive(mon, uarmf, TRUE, !DEADMONSTER(mon), AT_KICK, FALSE);
    if (DEADMONSTER(mon) && !trapkilled)
        killed(mon);

    /* may bring up a dialog, so put this after all messages */
    if (kick_skill != P_NONE) /* exercise proficiency */
        use_skill(kick_skill, 1);
}

STATIC_OVL boolean
maybe_kick_monster(mon, x, y)
struct monst *mon;
xchar x, y;
{
    if (mon) {
        boolean save_forcefight = context.forcefight;

        bhitpos.x = x;
        bhitpos.y = y;
        if (!mon->mpeaceful || !canspotmon(mon))
            context.forcefight = TRUE; /* attack even if invisible */
        /* kicking might be halted by discovery of hidden monster,
           by player declining to attack peaceful monster,
           or by passing out due to encumbrance */
        if (attack_checks(mon, (struct obj *) 0) || overexertion())
            mon = 0; /* don't kick after all */
        context.forcefight = save_forcefight;
    }
    return (boolean) (mon != 0);
}

STATIC_OVL void kick_monster(mon, x, y) struct monst *mon;
xchar x, y;
{
    boolean clumsy = FALSE;
    int i, j;

    /* anger target even if wild miss will occur */
    setmangry(mon, TRUE);

    if (Levitation && !rn2(3) && verysmall(mon->data)
        && !is_flyer(mon->data)) {
        /*KR pline("Floating in the air, you miss wildly!"); */
        pline("공중에 떠 있는 바람에, 발차기가 크게 빗나갔다!");
        exercise(A_DEX, FALSE);
        (void) passive(mon, uarmf, FALSE, 1, AT_KICK, FALSE);
        return;
    }

    /* reveal hidden target even if kick ends up missing (note: being
       hidden doesn't affect chance to hit so neither does this reveal) */
    if (mon->mundetected
        || (M_AP_TYPE(mon) && M_AP_TYPE(mon) != M_AP_MONSTER)) {
        if (M_AP_TYPE(mon))
            seemimic(mon);
        mon->mundetected = 0;
        if (!canspotmon(mon))
            map_invisible(x, y);
        else
            newsym(x, y);
#if 0 /*KR: 원본*/
        There("is %s here.",
              canspotmon(mon) ? a_monnam(mon) : "something hidden");
#else /*KR: KRNethack 맞춤 번역 */
        There("이곳에 %s 있다.",
              append_josa(canspotmon(mon) ? a_monnam(mon) : "숨겨진 무언가",
                          "가"));
#endif
    }

    /* Kick attacks by kicking monsters are normal attacks, not special.
     * This is almost always worthless, since you can either take one turn
     * and do all your kicks, or else take one turn and attack the monster
     * normally, getting all your attacks _including_ all your kicks.
     * If you have >1 kick attack, you get all of them.
     */
    if (Upolyd && attacktype(youmonst.data, AT_KICK)) {
        struct attack *uattk;
        int sum, kickdieroll, armorpenalty, specialdmg,
            attknum = 0,
            tmp = find_roll_to_hit(mon, AT_KICK, (struct obj *) 0, &attknum,
                                   &armorpenalty);

        for (i = 0; i < NATTK; i++) {
            /* first of two kicks might have provoked counterattack
               that has incapacitated the hero (ie, floating eye) */
            if (multi < 0)
                break;

            uattk = &youmonst.data->mattk[i];
            /* we only care about kicking attacks here */
            if (uattk->aatyp != AT_KICK)
                continue;

            kickdieroll = rnd(20);
            specialdmg = special_dmgval(&youmonst, mon, W_ARMF, (long *) 0);
            if (mon->data == &mons[PM_SHADE] && !specialdmg) {
                /* doesn't matter whether it would have hit or missed,
                   and shades have no passive counterattack */
                /*KR Your("%s %s.", kick_passes_thru, mon_nam(mon)); */
                Your("%s.", kick_passes_thru);
                break; /* skip any additional kicks */
            } else if (tmp > kickdieroll) {
                /*KR You("kick %s.", mon_nam(mon)); */
                You("%s 걷어찼다.", append_josa(mon_nam(mon), "을"));
                sum = damageum(mon, uattk, specialdmg);
                (void) passive(mon, uarmf, (boolean) (sum > 0), (sum != 2),
                               AT_KICK, FALSE);
                if (sum == 2)
                    break; /* Defender died */
            } else {
                missum(mon, uattk, (tmp + armorpenalty > kickdieroll));
                (void) passive(mon, uarmf, FALSE, 1, AT_KICK, FALSE);
            }
        }
        return;
    }

    i = -inv_weight();
    j = weight_cap();

    if (i < (j * 3) / 10) {
        if (!rn2((i < j / 10) ? 2 : (i < j / 5) ? 3 : 4)) {
            if (martial() && !rn2(2))
                goto doit;
            /*KR Your("clumsy kick does no damage."); */
            Your("어설픈 발차기로 인해 아무런 피해를 주지 못했다.");
            (void) passive(mon, uarmf, FALSE, 1, AT_KICK, FALSE);
            return;
        }
        if (i < j / 10)
            clumsy = TRUE;
        else if (!rn2((i < j / 5) ? 2 : 3))
            clumsy = TRUE;
    }

    if (Fumbling)
        clumsy = TRUE;

    else if (uarm && objects[uarm->otyp].oc_bulky && ACURR(A_DEX) < rnd(25))
        clumsy = TRUE;
doit:
    /*KR You("kick %s.", mon_nam(mon)); */
    You("%s 걷어찼다.", append_josa(mon_nam(mon), "을"));
    if (!rn2(clumsy ? 3 : 4) && (clumsy || !bigmonst(mon->data))
        && mon->mcansee && !mon->mtrapped && !thick_skinned(mon->data)
        && mon->data->mlet != S_EEL && haseyes(mon->data) && mon->mcanmove
        && !mon->mstun && !mon->mconf && !mon->msleeping
        && mon->data->mmove >= 12) {
        if (!nohands(mon->data) && !rn2(martial() ? 5 : 3)) {
#if 0 /*KR: 원본*/
            pline("%s blocks your %skick.", Monnam(mon),
                  clumsy ? "clumsy " : "");
#else /*KR: KRNethack 맞춤 번역 */
            pline("%s 당신의 %s발차기를 막아냈다.",
                  append_josa(Monnam(mon), "은"), clumsy ? "어설픈 " : "");
#endif
            (void) passive(mon, uarmf, FALSE, 1, AT_KICK, FALSE);
            return;
        } else {
            maybe_mnexto(mon);
            if (mon->mx != x || mon->my != y) {
                (void) unmap_invisible(x, y);
#if 0 /*KR: 원본*/
                pline("%s %s, %s evading your %skick.", Monnam(mon),
                      (!level.flags.noteleport && can_teleport(mon->data))
                          ? "teleports"
                          : is_floater(mon->data)
                                ? "floats"
                                : is_flyer(mon->data) ? "swoops"
                                                      : (nolimbs(mon->data)
                                                         || slithy(mon->data))
                                                            ? "slides"
                                                            : "jumps",
                      clumsy ? "easily" : "nimbly", clumsy ? "clumsy " : "");
#else /*KR: KRNethack 맞춤 번역 */
                pline("%s %s 당신의 %s발차기를 %s 피했다.",
                      append_josa(Monnam(mon), "은"),
                      (!level.flags.noteleport && can_teleport(mon->data))
                          ? "순간이동하여"
                      : is_floater(mon->data) ? "둥실 떠올라"
                      : is_flyer(mon->data)   ? "날아올라"
                      : (nolimbs(mon->data) || slithy(mon->data))
                          ? "스르륵 미끄러져"
                          : "펄쩍 뛰어",
                      clumsy ? "어설픈 " : "",
                      clumsy ? "손쉽게" : "재빠르게");
#endif
                (void) passive(mon, uarmf, FALSE, 1, AT_KICK, FALSE);
                return;
            }
        }
    }
    kickdmg(mon, clumsy);
}

/*
 * Return TRUE if caught (the gold taken care of), FALSE otherwise.
 * The gold object is *not* attached to the fobj chain!
 */
boolean
ghitm(mtmp, gold)
register struct monst *mtmp;
register struct obj *gold;
{
    boolean msg_given = FALSE;

    if (!likes_gold(mtmp->data) && !mtmp->isshk && !mtmp->ispriest
        && !mtmp->isgd && !is_mercenary(mtmp->data)) {
        wakeup(mtmp, TRUE);
    } else if (!mtmp->mcanmove) {
        /* too light to do real damage */
        if (canseemon(mtmp)) {
#if 0 /*KR: 원본*/
            pline_The("%s harmlessly %s %s.", xname(gold),
                      otense(gold, "hit"), mon_nam(mtmp));
#else /*KR: KRNethack 맞춤 번역 */
            pline("%s %s 부딪혔지만, 아무런 해를 입히지 못했다.",
                  append_josa(xname(gold), "은"),
                  append_josa(mon_nam(mtmp), "에"));
#endif
            msg_given = TRUE;
        }
    } else {
        long umoney, value = gold->quan * objects[gold->otyp].oc_cost;

        mtmp->msleeping = 0;
        finish_meating(mtmp);
        if (!mtmp->isgd && !rn2(4)) /* not always pleasing */
            setmangry(mtmp, TRUE);
        /* greedy monsters catch gold */
        if (cansee(mtmp->mx, mtmp->my))
            /*KR pline("%s catches the gold.", Monnam(mtmp)); */
            pline("%s 금화를 낚아챘다.", append_josa(Monnam(mtmp), "은"));
        (void) mpickobj(mtmp, gold);
        gold = (struct obj *) 0; /* obj has been freed */
        if (mtmp->isshk) {
            long robbed = ESHK(mtmp)->robbed;

            if (robbed) {
                robbed -= value;
                if (robbed < 0L)
                    robbed = 0L;
#if 0 /*KR: 원본*/
                pline_The("amount %scovers %s recent losses.",
                          !robbed ? "" : "partially ", mhis(mtmp));
#else /*KR: KRNethack 맞춤 번역 */
                pline("그 금액은 %s 최근 발생한 손실을 %s 메워준다.",
                      append_josa(mhis(mtmp), "의"),
                      !robbed ? "모두" : "부분적으로");
#endif
                ESHK(mtmp)->robbed = robbed;
                if (!robbed)
                    make_happy_shk(mtmp, FALSE);
            } else {
                if (mtmp->mpeaceful) {
                    ESHK(mtmp)->credit += value;
#if 0 /*KR: 원본*/
                    You("have %ld %s in credit.", ESHK(mtmp)->credit,
                        currency(ESHK(mtmp)->credit));
#else /*KR: KRNethack 맞춤 번역 */
                    You("이제 %ld %s의 외상값이 있다.", ESHK(mtmp)->credit,
                        currency(ESHK(mtmp)->credit));
#endif
                } else
                    /*KR verbalize("Thanks, scum!"); */
                    verbalize("고맙다, 쓰레기 자식아!");
            }
        } else if (mtmp->ispriest) {
            if (mtmp->mpeaceful)
                /*KR verbalize("Thank you for your contribution."); */
                verbalize("당신의 기부에 감사드립니다.");
            else
                /*KR verbalize("Thanks, scum!"); */
                verbalize("고맙다, 쓰레기 자식아!");
        } else if (mtmp->isgd) {
            umoney = money_cnt(invent);
            /* Some of these are iffy, because a hostile guard
               won't become peaceful and resume leading hero
               out of the vault.  If he did do that, player
               could try fighting, then weasle out of being
               killed by throwing his/her gold when losing. */
#if 0 /*KR: 원본*/
            verbalize(
                umoney
                    ? "Drop the rest and follow me."
                    : hidden_gold()
                          ? "You still have hidden gold.  Drop it now."
                          : mtmp->mpeaceful
                                ? "I'll take care of that; please move along."
                                : "I'll take that; now get moving.");
#else /*KR: KRNethack 맞춤 번역 */
            verbalize(
                umoney            ? "나머지도 내려놓고 나를 따라와라."
                : hidden_gold()   ? "아직 금을 더 숨기고 있군. 당장 내려놔."
                : mtmp->mpeaceful ? "이건 내가 맡지. 어서 앞장서게."
                                  : "이건 내가 맡겠다. 이제 움직여라.");
#endif
        } else if (is_mercenary(mtmp->data)) {
            long goldreqd = 0L;

            if (rn2(3)) {
                if (mtmp->data == &mons[PM_SOLDIER])
                    goldreqd = 100L;
                else if (mtmp->data == &mons[PM_SERGEANT])
                    goldreqd = 250L;
                else if (mtmp->data == &mons[PM_LIEUTENANT])
                    goldreqd = 500L;
                else if (mtmp->data == &mons[PM_CAPTAIN])
                    goldreqd = 750L;

                if (goldreqd) {
                    umoney = money_cnt(invent);
                    if (value
                        > goldreqd
                              + (umoney + u.ulevel * rn2(5)) / ACURR(A_CHA))
                        mtmp->mpeaceful = TRUE;
                }
            }
            if (mtmp->mpeaceful)
                /*KR verbalize("That should do.  Now beat it!"); */
                verbalize("이 정도면 됐어. 이제 꺼져!");
            else
                /*KR verbalize("That's not enough, coward!"); */
                verbalize("그걸론 부족해, 이 겁쟁이 녀석아!");
        }
        return TRUE;
    }

    if (!msg_given)
        miss(xname(gold), mtmp);
    return FALSE;
}

/* container is kicked, dropped, thrown or otherwise impacted by player.
 * Assumes container is on floor.  Checks contents for possible damage. */
void container_impact_dmg(obj, x, y) struct obj *obj;
xchar x, y; /* coordinates where object was before the impact, not after */
{
    struct monst *shkp;
    struct obj *otmp, *otmp2;
    long loss = 0L;
    boolean costly, insider, frominv;

    /* only consider normal containers */
    if (!Is_container(obj) || !Has_contents(obj) || Is_mbag(obj))
        return;

    costly = ((shkp = shop_keeper(*in_rooms(x, y, SHOPBASE)))
              && costly_spot(x, y));
    insider = (*u.ushops && inside_shop(u.ux, u.uy)
               && *in_rooms(x, y, SHOPBASE) == *u.ushops);
    /* if dropped or thrown, shop ownership flags are set on this obj */
    frominv = (obj != kickedobj);

    for (otmp = obj->cobj; otmp; otmp = otmp2) {
        const char *result = (char *) 0;

        otmp2 = otmp->nobj;
        if (objects[otmp->otyp].oc_material == GLASS
            && otmp->oclass != GEM_CLASS && !obj_resists(otmp, 33, 100)) {
            /*KR result = "shatter"; */
            result = "산산조각 나는";
        } else if (otmp->otyp == EGG && !rn2(3)) {
            /*KR result = "cracking"; */
            result = "쩍 갈라지는";
        }
        if (result) {
            if (otmp->otyp == MIRROR)
                change_luck(-2);

            /* eggs laid by you.  penalty is -1 per egg, max 5,
             * but it's always exactly 1 that breaks */
            if (otmp->otyp == EGG && otmp->spe && otmp->corpsenm >= LOW_PM)
                change_luck(-1);
            /*KR You_hear("a muffled %s.", result); */
            You_hear("뭔가 안에서 %s 소리가 들렸다.", result);
            if (costly) {
                if (frominv && !otmp->unpaid)
                    otmp->no_charge = 1;
                loss +=
                    stolen_value(otmp, x, y, (boolean) shkp->mpeaceful, TRUE);
            }
            if (otmp->quan > 1L) {
                useup(otmp);
            } else {
                obj_extract_self(otmp);
                obfree(otmp, (struct obj *) 0);
            }
            /* contents of this container are no longer known */
            obj->cknown = 0;
        }
    }
    if (costly && loss) {
        if (!insider) {
            /*KR You("caused %ld %s worth of damage!", loss, currency(loss));
             */
            You("%ld %s 어치의 피해를 입혔다!", loss, currency(loss));
            make_angry_shk(shkp, x, y);
        } else {
#if 0 /*KR: 원본*/
            You("owe %s %ld %s for objects destroyed.", mon_nam(shkp), loss,
                currency(loss));
#else /*KR: KRNethack 맞춤 번역 */
            You("물건을 망가뜨린 대가로 %s에게 %ld %s의 빚을 졌다.",
                mon_nam(shkp), loss, currency(loss));
#endif
        }
    }
}

/* jacket around really_kick_object */
STATIC_OVL int
kick_object(x, y, kickobjnam)
xchar x, y;
char *kickobjnam;
{
    int res = 0;

    *kickobjnam = '\0';
    /* if a pile, the "top" object gets kicked */
    kickedobj = level.objects[x][y];
    if (kickedobj) {
        /* kick object; if doing is fatal, done() will clean up kickedobj */
        Strcpy(kickobjnam, killer_xname(kickedobj)); /* matters iff res==0 */
        res = really_kick_object(x, y);
        kickedobj = (struct obj *) 0;
    }
    return res;
}

/* guts of kick_object */
STATIC_OVL int
really_kick_object(x, y)
xchar x, y;
{
    int range;
    struct monst *mon, *shkp = 0;
    struct trap *trap;
    char bhitroom;
    boolean costly, isgold, slide = FALSE;

    /* kickedobj should always be set due to conditions of call */
    if (!kickedobj || kickedobj->otyp == BOULDER || kickedobj == uball
        || kickedobj == uchain)
        return 0;

    if ((trap = t_at(x, y)) != 0) {
        if ((is_pit(trap->ttyp) && !Passes_walls) || trap->ttyp == WEB) {
            if (!trap->tseen)
                find_trap(trap);
#if 0 /*KR: 원본*/
            You_cant("kick %s that's in a %s!", something,
                     Hallucination ? "tizzy"
                         : (trap->ttyp == WEB) ? "web"
                             : "pit");
#else /*KR: KRNethack 맞춤 번역 */
            You_cant("%s 있는 %s 걷어찰 수 없다!",
                     Hallucination         ? "아수라장 속에"
                     : (trap->ttyp == WEB) ? "거미줄에 걸려"
                                           : "구덩이에 빠져",
                     append_josa(something, "을"));
#endif
            return 1;
        }
        if (trap->ttyp == STATUE_TRAP) {
            activate_statue_trap(trap, x, y, FALSE);
            return 1;
        }
    }

    if (Fumbling && !rn2(3)) {
        /*KR Your("clumsy kick missed."); */
        Your("어설픈 발차기는 빗나갔다.");
        return 1;
    }

    if (!uarmf && kickedobj->otyp == CORPSE
        && touch_petrifies(&mons[kickedobj->corpsenm]) && !Stone_resistance) {
#if 0 /*KR: 원본*/
        You("kick %s with your bare %s.",
            corpse_xname(kickedobj, (const char *) 0, CXN_PFX_THE),
            makeplural(body_part(FOOT)));
#else /*KR: KRNethack 맞춤 번역 */
        You("맨%s로 %s 걷어찼다.", makeplural(body_part(FOOT)),
            append_josa(
                corpse_xname(kickedobj, (const char *) 0, CXN_PFX_THE),
                "을"));
#endif
        if (poly_when_stoned(youmonst.data) && polymon(PM_STONE_GOLEM)) {
            ; /* hero has been transformed but kick continues */
        } else {
            /* normalize body shape here; foot, not body_part(FOOT) */
            /*KR Sprintf(killer.name, "kicking %s barefoot", */
            Sprintf(killer.name, "맨발로 %s 걷어찬 것",
                    append_josa(killer_xname(kickedobj), "을"));
            instapetrify(killer.name);
        }
    }

    isgold = (kickedobj->oclass == COIN_CLASS);
    {
        int k_owt = (int) kickedobj->owt;

        /* for non-gold stack, 1 item will be split off below (unless an
           early return occurs, so we aren't moving the split to here);
           calculate the range for that 1 rather than for the whole stack */
        if (kickedobj->quan > 1L && !isgold) {
            long save_quan = kickedobj->quan;

            kickedobj->quan = 1L;
            k_owt = weight(kickedobj);
            kickedobj->quan = save_quan;
        }

        /* range < 2 means the object will not move
           (maybe dexterity should also figure here) */
        range = ((int) ACURRSTR) / 2 - k_owt / 40;
    }

    if (martial())
        range += rnd(3);

    if (is_pool(x, y)) {
        /* you're in the water too; significantly reduce range */
        range = range / 3 + 1; /* {1,2}=>1, {3,4,5}=>2, {6,7,8}=>3 */
    } else if (Is_airlevel(&u.uz) || Is_waterlevel(&u.uz)) {
        /* you're in air, since is_pool did not match */
        range += rnd(3);
    } else {
        if (is_ice(x, y))
            range += rnd(3), slide = TRUE;
        if (kickedobj->greased)
            range += rnd(3), slide = TRUE;
    }

    /* Mjollnir is magically too heavy to kick */
    if (kickedobj->oartifact == ART_MJOLLNIR)
        range = 1;

    /* see if the object has a place to move into */
    if (!ZAP_POS(levl[x + u.dx][y + u.dy].typ)
        || closed_door(x + u.dx, y + u.dy))
        range = 1;

    costly = (!(kickedobj->no_charge && !Has_contents(kickedobj))
              && (shkp = shop_keeper(*in_rooms(x, y, SHOPBASE))) != 0
              && costly_spot(x, y));

    if (IS_ROCK(levl[x][y].typ) || closed_door(x, y)) {
        if ((!martial() && rn2(20) > ACURR(A_DEX))
            || IS_ROCK(levl[u.ux][u.uy].typ) || closed_door(u.ux, u.uy)) {
            if (Blind)
                /*KR pline("It doesn't come loose."); */
                pline("꿈쩍도 하지 않는다.");
            else
#if 0 /*KR: 원본*/
                pline("%s %sn't come loose.",
                      The(distant_name(kickedobj, xname)),
                      otense(kickedobj, "do"));
#else /*KR: KRNethack 맞춤 번역 */
                pline("%s 꿈쩍도 하지 않는다.",
                      append_josa(The(distant_name(kickedobj, xname)), "은"));
#endif
            return (!rn2(3) || martial());
        }
        if (Blind)
            /*KR pline("It comes loose."); */
            pline("무언가가 떨어져 나왔다.");
        else
#if 0 /*KR: 원본*/
            pline("%s %s loose.", The(distant_name(kickedobj, xname)),
                  otense(kickedobj, "come"));
#else /*KR: KRNethack 맞춤 번역 */
            pline("%s 떨어져 나왔다.",
                  append_josa(The(distant_name(kickedobj, xname)), "이"));
#endif
        obj_extract_self(kickedobj);
        newsym(x, y);
        if (costly
            && (!costly_spot(u.ux, u.uy)
                || !index(u.urooms, *in_rooms(x, y, SHOPBASE))))
            addtobill(kickedobj, FALSE, FALSE, FALSE);
        /*KR if (!flooreffects(kickedobj, u.ux, u.uy, "fall")) { */
        if (!flooreffects(kickedobj, u.ux, u.uy, "떨어졌다")) {
            place_object(kickedobj, u.ux, u.uy);
            stackobj(kickedobj);
            newsym(u.ux, u.uy);
        }
        return 1;
    }

    /* a box gets a chance of breaking open here */
    if (Is_box(kickedobj)) {
        boolean otrp = kickedobj->otrapped;

        if (range < 2)
            /*KR pline("THUD!"); */
            pline("쾅!");
        container_impact_dmg(kickedobj, x, y);
        if (kickedobj->olocked) {
            if (!rn2(5) || (martial() && !rn2(2))) {
                /*KR You("break open the lock!"); */
                You("자물쇠를 부수고 열었다!");
                breakchestlock(kickedobj, FALSE);
                if (otrp)
                    (void) chest_trap(kickedobj, LEG, FALSE);
                return 1;
            }
        } else {
            if (!rn2(3) || (martial() && !rn2(2))) {
                /*KR pline_The("lid slams open, then falls shut."); */
                pline("뚜껑이 쾅 하고 열렸다가 다시 닫혔다.");
                kickedobj->lknown = 1;
                if (otrp)
                    (void) chest_trap(kickedobj, LEG, FALSE);
                return 1;
            }
        }
        if (range < 2)
            return 1;
        /* else let it fall through to the next cases... */
    }

    /* fragile objects should not be kicked */
    if (hero_breaks(kickedobj, kickedobj->ox, kickedobj->oy, FALSE))
        return 1;

    /* too heavy to move.  range is calculated as potential distance from
     * player, so range == 2 means the object may move up to one square
     * from its current position
     */
    if (range < 2) {
        if (!Is_box(kickedobj))
            /*KR pline("Thump!"); */
            pline("쿵!");
        return (!rn2(3) || martial());
    }

    if (kickedobj->quan > 1L) {
        if (!isgold) {
            kickedobj = splitobj(kickedobj, 1L);
        } else {
            if (rn2(20)) {
#if 0 /*KR: 원본*/
                static NEARDATA const char *const flyingcoinmsg[] = {
                    "scatter the coins", "knock coins all over the place",
                    "send coins flying in all directions",
                };
#else /*KR: KRNethack 맞춤 번역 */
                static NEARDATA const char *const flyingcoinmsg[] = {
                    "동전들을 흩뿌렸다",
                    "사방으로 동전들을 흩날렸다",
                    "동전들을 사방팔방으로 날려버렸다",
                };
#endif

                /*KR pline("Thwwpingg!"); */
                pline("챙그랑!");
                /*KR You("%s!", flyingcoinmsg[rn2(SIZE(flyingcoinmsg))]); */
                You("%s!", flyingcoinmsg[rn2(SIZE(flyingcoinmsg))]);
                (void) scatter(x, y, rn2(3) + 1, VIS_EFFECTS | MAY_HIT,
                               kickedobj);
                newsym(x, y);
                return 1;
            }
            if (kickedobj->quan > 300L) {
                /*KR pline("Thump!"); */
                pline("쿵!");
                return (!rn2(3) || martial());
            }
        }
    }

    if (slide && !Blind)
#if 0 /*KR: 원본*/
        pline("Whee!  %s %s across the %s.", Doname2(kickedobj),
              otense(kickedobj, "slide"), surface(x, y));
#else /*KR: KRNethack 맞춤 번역 */
        pline("슈웅! %s %s 미끄러져 나아갔다.",
              append_josa(Doname2(kickedobj), "이"), append_josa(surface(x, y), "을"));
#endif

    if (costly && !isgold)
        addtobill(kickedobj, FALSE, FALSE, TRUE);
    obj_extract_self(kickedobj);
    (void) snuff_candle(kickedobj);
    newsym(x, y);
    mon = bhit(u.dx, u.dy, range, KICKED_WEAPON,
               (int FDECL((*), (MONST_P, OBJ_P))) 0,
               (int FDECL((*), (OBJ_P, OBJ_P))) 0, &kickedobj);
    if (!kickedobj)
        return 1; /* object broken */

    if (mon) {
        if (mon->isshk && kickedobj->where == OBJ_MINVENT
            && kickedobj->ocarry == mon)
            return 1; /* alert shk caught it */
        notonhead = (mon->mx != bhitpos.x || mon->my != bhitpos.y);
        if (isgold ? ghitm(mon, kickedobj)      /* caught? */
                   : thitmonst(mon, kickedobj)) /* hit && used up? */
            return 1;
    }

    /* the object might have fallen down a hole;
       ship_object() will have taken care of shop billing */
    if (kickedobj->where == OBJ_MIGRATING)
        return 1;

    bhitroom = *in_rooms(bhitpos.x, bhitpos.y, SHOPBASE);
    if (costly
        && (!costly_spot(bhitpos.x, bhitpos.y)
            || *in_rooms(x, y, SHOPBASE) != bhitroom)) {
        if (isgold)
            costly_gold(x, y, kickedobj->quan);
        else
            (void) stolen_value(kickedobj, x, y, (boolean) shkp->mpeaceful,
                                FALSE);
    }

    /*KR if (flooreffects(kickedobj, bhitpos.x, bhitpos.y, "fall")) */
    if (flooreffects(kickedobj, bhitpos.x, bhitpos.y, "떨어졌다"))
        return 1;
    if (kickedobj->unpaid)
        subfrombill(kickedobj, shkp);
    place_object(kickedobj, bhitpos.x, bhitpos.y);
    stackobj(kickedobj);
    newsym(kickedobj->ox, kickedobj->oy);
    return 1;
}

/* cause of death if kicking kills kicker */
STATIC_OVL char *
kickstr(buf, kickobjnam)
char *buf;
const char *kickobjnam;
{
    const char *what;

    if (*kickobjnam)
        what = kickobjnam;
    else if (maploc == &nowhere)
        /*KR what = "nothing"; */
        what = "아무것도 없는 허공";
    else if (IS_DOOR(maploc->typ))
        /*KR what = "a door"; */
        what = "문";
    else if (IS_TREE(maploc->typ))
        /*KR what = "a tree"; */
        what = "나무";
    else if (IS_STWALL(maploc->typ))
        /*KR what = "a wall"; */
        what = "벽";
    else if (IS_ROCK(maploc->typ))
        /*KR what = "a rock"; */
        what = "바위";
    else if (IS_THRONE(maploc->typ))
        /*KR what = "a throne"; */
        what = "옥좌";
    else if (IS_FOUNTAIN(maploc->typ))
        /*KR what = "a fountain"; */
        what = "분수";
    else if (IS_GRAVE(maploc->typ))
        /*KR what = "a headstone"; */
        what = "묘비";
    else if (IS_SINK(maploc->typ))
        /*KR what = "a sink"; */
        what = "싱크대";
    else if (IS_ALTAR(maploc->typ))
        /*KR what = "an altar"; */
        what = "제단";
    else if (IS_DRAWBRIDGE(maploc->typ))
        /*KR what = "a drawbridge"; */
        what = "도개교";
    else if (maploc->typ == STAIRS)
        /*KR what = "the stairs"; */
        what = "계단";
    else if (maploc->typ == LADDER)
        /*KR what = "a ladder"; */
        what = "사다리";
    else if (maploc->typ == IRONBARS)
        /*KR what = "an iron bar"; */
        what = "쇠창살";
    else
        /*KR what = "something weird"; */
        what = "뭔가 이상한 것";
#if 0 /*KR: 원본*/
    return strcat(strcpy(buf, "kicking "), what);
#else /*KR: KRNethack 맞춤 번역 */
    Sprintf(buf, "%s 걷어찬 것", append_josa(what, "을"));
    return buf;
#endif
}

int
dokick()
{
    int x, y;
    int avrg_attrib;
    int dmg = 0, glyph, oldglyph = -1;
    register struct monst *mtmp;
    boolean no_kick = FALSE;
    char buf[BUFSZ], kickobjnam[BUFSZ];

    kickobjnam[0] = '\0';
    if (nolimbs(youmonst.data) || slithy(youmonst.data)) {
        /*KR You("have no legs to kick with."); */
        You("걷어찰 다리가 없다.");
        no_kick = TRUE;
    } else if (verysmall(youmonst.data)) {
        /*KR You("are too small to do any kicking."); */
        You("무언가를 걷어차기에는 몸이 너무 작다.");
        no_kick = TRUE;
    } else if (u.usteed) {
        /*KR if (yn_function("Kick your steed?", ynchars, 'y') == 'y') { */
        if (yn_function("타고 있는 말을 걷어차시겠습니까?", ynchars, 'y')
            == 'y') {
            /*KR You("kick %s.", mon_nam(u.usteed)); */
            You("%s 걷어찼다.", append_josa(mon_nam(u.usteed), "을"));
            kick_steed();
            return 1;
        } else {
            return 0;
        }
    } else if (Wounded_legs) {
        /* note: jump() has similar code */
        long wl = (EWounded_legs & BOTH_SIDES);
        const char *bp = body_part(LEG);

        if (wl == BOTH_SIDES)
            bp = makeplural(bp);
#if 0 /*KR: 원본*/
        Your("%s%s %s in no shape for kicking.",
             (wl == LEFT_SIDE) ? "left " : (wl == RIGHT_SIDE) ? "right " : "",
             bp, (wl == BOTH_SIDES) ? "are" : "is");
#else /*KR: KRNethack 맞춤 번역 */
        Your("%s%s 발차기를 할 만한 상태가 아니다.",
             (wl == LEFT_SIDE)    ? "왼쪽 "
             : (wl == RIGHT_SIDE) ? "오른쪽 "
                                  : "",
             append_josa(bp, "은"));
#endif
        no_kick = TRUE;
    } else if (near_capacity() > SLT_ENCUMBER) {
        /*KR Your("load is too heavy to balance yourself for a kick."); */
        Your("짐이 너무 무거워 발차기를 위한 균형을 잡을 수 없다.");
        no_kick = TRUE;
    } else if (youmonst.data->mlet == S_LIZARD) {
        /*KR Your("legs cannot kick effectively."); */
        Your("다리로 제대로 걷어찰 수가 없다.");
        no_kick = TRUE;
    } else if (u.uinwater && !rn2(2)) {
        /*KR Your("slow motion kick doesn't hit anything."); */
        Your("느릿느릿한 발차기는 아무것도 맞히지 못했다.");
        no_kick = TRUE;
    } else if (u.utrap) {
        no_kick = TRUE;
        switch (u.utraptype) {
        case TT_PIT:
            if (!Passes_walls)
                /*KR pline("There's not enough room to kick down here."); */
                pline("이 아래에서는 걷어찰 만한 공간이 부족하다.");
            else
                no_kick = FALSE;
            break;
        case TT_WEB:
        case TT_BEARTRAP:
            /*KR You_cant("move your %s!", body_part(LEG)); */
            You_cant("당신의 %s 움직일 수 없다!",
                     append_josa(body_part(LEG), "을"));
            break;
        default:
            break;
        }
    }

    if (no_kick) {
        /* ignore direction typed before player notices kick failed */
        display_nhwindow(WIN_MESSAGE, TRUE); /* --More-- */
        return 0;
    }

    if (!getdir((char *) 0))
        return 0;
    if (!u.dx && !u.dy)
        return 0;

    x = u.ux + u.dx;
    y = u.uy + u.dy;

    /* KMH -- Kicking boots always succeed */
    if (uarmf && uarmf->otyp == KICKING_BOOTS)
        avrg_attrib = 99;
    else
        avrg_attrib = (ACURRSTR + ACURR(A_DEX) + ACURR(A_CON)) / 3;

    if (u.uswallow) {
        switch (rn2(3)) {
        case 0:
            /*KR You_cant("move your %s!", body_part(LEG)); */
            You_cant("당신의 %s 움직일 수 없다!",
                     append_josa(body_part(LEG), "을"));
            break;
        case 1:
            if (is_animal(u.ustuck->data)) {
                /*KR pline("%s burps loudly.", Monnam(u.ustuck)); */
                pline("%s 크게 트림했다.",
                      append_josa(Monnam(u.ustuck), "은"));
                break;
            }
            /*FALLTHRU*/
        default:
            /*KR Your("feeble kick has no effect."); */
            Your("약한 발차기는 아무런 효과가 없다.");
            break;
            break;
        }
        return 1;
    } else if (u.utrap && u.utraptype == TT_PIT) {
        /* must be Passes_walls */
        /*KR You("kick at the side of the pit."); */
        You("구덩이의 벽면을 걷어찼다.");
        return 1;
    }
    if (Levitation) {
        int xx, yy;

        xx = u.ux - u.dx;
        yy = u.uy - u.dy;
        /* doors can be opened while levitating, so they must be
         * reachable for bracing purposes
         * Possible extension: allow bracing against stuff on the side?
         */
        if (isok(xx, yy) && !IS_ROCK(levl[xx][yy].typ)
            && !IS_DOOR(levl[xx][yy].typ)
            && (!Is_airlevel(&u.uz) || !OBJ_AT(xx, yy))) {
            /*KR You("have nothing to brace yourself against."); */
            You("등을 기대어 버틸 만한 것이 아무것도 없다.");
            return 0;
        }
    }

    mtmp = isok(x, y) ? m_at(x, y) : 0;
    /* might not kick monster if it is hidden and becomes revealed,
       if it is peaceful and player declines to attack, or if the
       hero passes out due to encumbrance with low hp; context.move
       will be 1 unless player declines to kick peaceful monster */
    if (mtmp) {
        oldglyph = glyph_at(x, y);
        if (!maybe_kick_monster(mtmp, x, y))
            return context.move;
    }

    wake_nearby();
    u_wipe_engr(2);

    if (!isok(x, y)) {
        maploc = &nowhere;
        goto ouch;
    }
    maploc = &levl[x][y];

    /*
     * The next five tests should stay in their present order:
     * monsters, pools, objects, non-doors, doors.
     *
     * [FIXME:  Monsters who are hidden underneath objects or
     * in pools should lead to hero kicking the concealment
     * rather than the monster, probably exposing the hidden
     * monster in the process.  And monsters who are hidden on
     * ceiling shouldn't be kickable (unless hero is flying?);
     * kicking toward them should just target whatever is on
     * the floor at that spot.]
     */

    if (mtmp) {
        /* save mtmp->data (for recoil) in case mtmp gets killed */
        struct permonst *mdat = mtmp->data;

        kick_monster(mtmp, x, y);
        glyph = glyph_at(x, y);
        /* see comment in attack_checks() */
        if (DEADMONSTER(mtmp)) { /* DEADMONSTER() */
            /* if we mapped an invisible monster and immediately
               killed it, we don't want to forget what we thought
               was there before the kick */
            if (glyph != oldglyph && glyph_is_invisible(glyph))
                show_glyph(x, y, oldglyph);
        } else if (!canspotmon(mtmp)
                   /* check <x,y>; monster that evades kick by jumping
                      to an unseen square doesn't leave an I behind */
                   && mtmp->mx == x && mtmp->my == y
                   && !glyph_is_invisible(glyph)
                   && !(u.uswallow && mtmp == u.ustuck)) {
            map_invisible(x, y);
        }
        /* recoil if floating */
        if ((Is_airlevel(&u.uz) || Levitation) && context.move) {
            int range;

            range =
                ((int) youmonst.data->cwt + (weight_cap() + inv_weight()));
            if (range < 1)
                range = 1; /* divide by zero avoidance */
            range = (3 * (int) mdat->cwt) / range;

            if (range < 1)
                range = 1;
            hurtle(-u.dx, -u.dy, range, TRUE);
        }
        return 1;
    }
    (void) unmap_invisible(x, y);
    if (is_pool(x, y) ^ !!u.uinwater) {
        /* objects normally can't be removed from water by kicking */
        /*KR You("splash some %s around.", hliquid("water")); */
        You("%s 이리저리 튀겼다.", append_josa(hliquid("물"), "을"));
        return 1;
    }

    if (OBJ_AT(x, y)
        && (!Levitation || Is_airlevel(&u.uz) || Is_waterlevel(&u.uz)
            || sobj_at(BOULDER, x, y))) {
        if (kick_object(x, y, kickobjnam)) {
            if (Is_airlevel(&u.uz))
                hurtle(-u.dx, -u.dy, 1, TRUE); /* assume it's light */
            return 1;
        }
        goto ouch;
    }

    if (!IS_DOOR(maploc->typ)) {
        if (maploc->typ == SDOOR) {
            if (!Levitation && rn2(30) < avrg_attrib) {
                cvt_sdoor_to_door(maploc); /* ->typ = DOOR */
#if 0                                      /*KR: 원본*/
                pline("Crash!  %s a secret door!",
                      /* don't "kick open" when it's locked
                         unless it also happens to be trapped */
                      (maploc->doormask & (D_LOCKED | D_TRAPPED)) == D_LOCKED
                          ? "Your kick uncovers"
                          : "You kick open");
#else                                      /*KR: KRNethack 맞춤 번역 */
                pline("쾅! 비밀 문을 %s!",
                      (maploc->doormask & (D_LOCKED | D_TRAPPED)) == D_LOCKED
                          ? "발로 차서 찾아냈다"
                          : "걷어차서 열었다");
#endif
                exercise(A_DEX, TRUE);
                if (maploc->doormask & D_TRAPPED) {
                    maploc->doormask = D_NODOOR;
                    /*KR b_trapped("door", FOOT); */
                    b_trapped("문", FOOT);
                } else if (maploc->doormask != D_NODOOR
                           && !(maploc->doormask & D_LOCKED))
                    maploc->doormask = D_ISOPEN;
                feel_newsym(x, y); /* we know it's gone */
                if (maploc->doormask == D_ISOPEN
                    || maploc->doormask == D_NODOOR)
                    unblock_point(x, y); /* vision */
                return 1;
            } else
                goto ouch;
        }
        if (maploc->typ == SCORR) {
            if (!Levitation && rn2(30) < avrg_attrib) {
                /*KR pline("Crash!  You kick open a secret passage!"); */
                pline("쾅! 당신은 비밀 통로를 걷어차서 열어젖혔다!");
                exercise(A_DEX, TRUE);
                maploc->typ = CORR;
                feel_newsym(x, y);   /* we know it's gone */
                unblock_point(x, y); /* vision */
                return 1;
            } else
                goto ouch;
        }
        if (IS_THRONE(maploc->typ)) {
            register int i;
            if (Levitation)
                goto dumb;
            if ((Luck < 0 || maploc->doormask) && !rn2(3)) {
                maploc->typ = ROOM;
                maploc->doormask = 0; /* don't leave loose ends.. */
                (void) mkgold((long) rnd(200), x, y);
                if (Blind)
                    /*KR pline("CRASH!  You destroy it."); */
                    pline("쾅! 당신은 그것을 박살냈다.");
                else {
                    /*KR pline("CRASH!  You destroy the throne."); */
                    pline("쾅! 당신은 옥좌를 박살냈다.");
                    newsym(x, y);
                }
                exercise(A_DEX, TRUE);
                return 1;
            } else if (Luck > 0 && !rn2(3) && !maploc->looted) {
                (void) mkgold((long) rn1(201, 300), x, y);
                i = Luck + 1;
                if (i > 6)
                    i = 6;
                while (i--)
                    (void) mksobj_at(
                        rnd_class(DILITHIUM_CRYSTAL, LUCKSTONE - 1), x, y,
                        FALSE, TRUE);
                if (Blind)
                    /*KR You("kick %s loose!", something); */
                    You("%s 걷어차서 흩뿌렸다!",
                        append_josa(something, "을"));
                else {
                    /*KR You("kick loose some ornamental coins and gems!"); */
                    You("금화와 보석들을 걷어차서 흩뿌렸다!");
                    newsym(x, y);
                }
                /* prevent endless milking */
                maploc->looted = T_LOOTED;
                return 1;
            } else if (!rn2(4)) {
                if (dunlev(&u.uz) < dunlevs_in_dungeon(&u.uz)) {
                    fall_through(FALSE, 0);
                    return 1;
                } else
                    goto ouch;
            }
            goto ouch;
        }
        if (IS_ALTAR(maploc->typ)) {
            if (Levitation)
                goto dumb;
            /*KR You("kick %s.", (Blind ? something : "the altar")); */
            You("%s 걷어찼다.",
                append_josa(Blind ? something : "제단", "을"));
            altar_wrath(x, y);
            if (!rn2(3))
                goto ouch;
            exercise(A_DEX, TRUE);
            return 1;
        }
        if (IS_FOUNTAIN(maploc->typ)) {
            if (Levitation)
                goto dumb;
            /*KR You("kick %s.", (Blind ? something : "the fountain")); */
            You("%s 걷어찼다.",
                append_josa(Blind ? something : "분수", "을"));
            if (!rn2(3))
                goto ouch;
            /* make metal boots rust */
            if (uarmf && rn2(3))
                /*KR if (water_damage(uarmf, "metal boots", TRUE) ==
                 * ER_NOTHING) { */
                if (water_damage(uarmf, "금속 장화", TRUE) == ER_NOTHING) {
                    /*KR Your("boots get wet."); */
                    Your("장화가 젖었다.");
                    /* could cause short-lived fumbling here */
                }
            exercise(A_DEX, TRUE);
            return 1;
        }
        if (IS_GRAVE(maploc->typ)) {
            if (Levitation)
                goto dumb;
            if (rn2(4))
                goto ouch;
            exercise(A_WIS, FALSE);
            if (Role_if(PM_ARCHEOLOGIST) || Role_if(PM_SAMURAI)
                || ((u.ualign.type == A_LAWFUL) && (u.ualign.record > -10))) {
                adjalign(-sgn(u.ualign.type));
            }
            maploc->typ = ROOM;
            maploc->doormask = 0;
            (void) mksobj_at(ROCK, x, y, TRUE, FALSE);
            del_engr_at(x, y);
            if (Blind)
#if 0 /*KR: 원본*/
                pline("Crack!  %s broke!", Something);
#else /*KR: KRNethack 맞춤 번역 */
                pline("빠직! %s 깨졌다!", append_josa(Something, "이"));
#endif
            else {
                /*KR pline_The("headstone topples over and breaks!"); */
                pline("묘비가 쓰러지며 산산조각 났다!");
                newsym(x, y);
            }
            return 1;
        }
        if (maploc->typ == IRONBARS)
            goto ouch;
        if (IS_TREE(maploc->typ)) {
            struct obj *treefruit;

            /* nothing, fruit or trouble? 75:23.5:1.5% */
            if (rn2(3)) {
                if (!rn2(6) && !(mvitals[PM_KILLER_BEE].mvflags & G_GONE))
#if 0 /*KR: 원본*/
                    You_hear("a low buzzing."); /* a warning */
#else /*KR: KRNethack 맞춤 번역 */
                    You_hear(
                        "낮게 웅웅거리는 소리가 들린다."); /* a warning */
#endif
                goto ouch;
            }
            if (rn2(15) && !(maploc->looted & TREE_LOOTED)
                && (treefruit = rnd_treefruit_at(x, y))) {
                long nfruit = 8L - rnl(7), nfall;
                short frtype = treefruit->otyp;

                treefruit->quan = nfruit;
                treefruit->owt = weight(treefruit);
                if (is_plural(treefruit))
                    /*KR pline("Some %s fall from the tree!",
                     * xname(treefruit)); */
                    pline("나무에서 몇 개의 %s 떨어졌다!",
                          append_josa(xname(treefruit), "이"));
                else
                    /*KR pline("%s falls from the tree!",
                     * An(xname(treefruit))); */
                    pline("나무에서 %s 떨어졌다!",
                          append_josa(xname(treefruit), "이"));
                nfall = scatter(x, y, 2, MAY_HIT, treefruit);
                if (nfall != nfruit) {
                    /* scatter left some in the tree, but treefruit
                     * may not refer to the correct object */
                    treefruit = mksobj(frtype, TRUE, FALSE);
                    treefruit->quan = nfruit - nfall;
#if 0 /*KR: 원본*/
                    pline("%ld %s got caught in the branches.",
                          nfruit - nfall, xname(treefruit));
#else /*KR: KRNethack 맞춤 번역 */
                    pline("%ld개의 %s 나뭇가지에 걸렸다.", nfruit - nfall,
                          append_josa(xname(treefruit), "이"));
#endif
                    dealloc_obj(treefruit);
                }
                exercise(A_DEX, TRUE);
                exercise(A_WIS, TRUE); /* discovered a new food source! */
                newsym(x, y);
                maploc->looted |= TREE_LOOTED;
                return 1;
            } else if (!(maploc->looted & TREE_SWARM)) {
                int cnt = rnl(4) + 2;
                int made = 0;
                coord mm;

                mm.x = x;
                mm.y = y;
                while (cnt--) {
                    if (enexto(&mm, mm.x, mm.y, &mons[PM_KILLER_BEE])
                        && makemon(&mons[PM_KILLER_BEE], mm.x, mm.y,
                                   MM_ANGRY))
                        made++;
                }
                if (made)
                    /*KR pline("You've attracted the tree's former
                     * occupants!"); */
                    pline("나무의 이전 거주자들을 끌어들이고 말았다!");
                else
                    /*KR You("smell stale honey."); */
                    You("오래된 꿀 냄새를 맡았다.");
                maploc->looted |= TREE_SWARM;
                return 1;
            }
            goto ouch;
        }
        if (IS_SINK(maploc->typ)) {
            int gend = poly_gender();
            short washerndx = (gend == 1 || (gend == 2 && rn2(2)))
                                  ? PM_INCUBUS
                                  : PM_SUCCUBUS;

            if (Levitation)
                goto dumb;
            if (rn2(5)) {
                if (!Deaf)
                    /*KR pline("Klunk!  The pipes vibrate noisily."); */
                    pline("쾅! 파이프가 시끄럽게 진동한다.");
                else
                    /*KR pline("Klunk!"); */
                    pline("쾅!");
                exercise(A_DEX, TRUE);
                return 1;
            } else if (!(maploc->looted & S_LPUDDING) && !rn2(3)
                       && !(mvitals[PM_BLACK_PUDDING].mvflags & G_GONE)) {
                if (Blind)
                    /*KR You_hear("a gushing sound."); */
                    You_hear("물이 세차게 뿜어져 나오는 소리가 들린다.");
                else
                    /*KR pline("A %s ooze gushes up from the drain!",
                          hcolor(NH_BLACK)); */
                    pline("%s 진흙이 배수구에서 뿜어져 나왔다!",
                          hcolor(NH_BLACK));
                (void) makemon(&mons[PM_BLACK_PUDDING], x, y, NO_MM_FLAGS);
                exercise(A_DEX, TRUE);
                newsym(x, y);
                maploc->looted |= S_LPUDDING;
                return 1;
            } else if (!(maploc->looted & S_LDWASHER) && !rn2(3)
                       && !(mvitals[washerndx].mvflags & G_GONE)) {
                /* can't resist... */
                /*KR pline("%s returns!", (Blind ? Something : "The dish
                 * washer")); */
                pline("%s 돌아왔다!",
                      append_josa((Blind ? Something : "식기 세척기"), "이"));
                if (makemon(&mons[washerndx], x, y, NO_MM_FLAGS))
                    newsym(x, y);
                maploc->looted |= S_LDWASHER;
                exercise(A_DEX, TRUE);
                return 1;
            } else if (!rn2(3)) {
                if (Blind && Deaf)
                    Sprintf(buf, " %s", body_part(FACE));
                else
                    buf[0] = '\0';
#if 0 /*KR: 원본*/
                pline("%s%s%s.", !Deaf ? "Flupp! " : "",
                      !Blind
                          ? "Muddy waste pops up from the drain"
                          : !Deaf
                              ? "You hear a sloshing sound"  /* Deaf-aware */
                              : "Something splashes you in the", buf);
#else /*KR: KRNethack 맞춤 번역 */
                pline("%s%s%s.", !Deaf ? "철퍼덕! " : "",
                      !Blind  ? "배수구에서 진흙 같은 쓰레기가 튀어올랐다"
                      : !Deaf ? "철썩거리는 소리가 들린다" /* Deaf-aware */
                              : "무언가가 당신의",
                      (!Blind || !Deaf) ? "" : append_josa(buf, "에 튀었다"));
#endif
                if (!(maploc->looted & S_LRING)) { /* once per sink */
                    if (!Blind)
                        /*KR You_see("a ring shining in its midst."); */
                        You("그 한가운데서 반지가 반짝이는 것을 보았다.");
                    (void) mkobj_at(RING_CLASS, x, y, TRUE);
                    newsym(x, y);
                    exercise(A_DEX, TRUE);
                    exercise(A_WIS, TRUE); /* a discovery! */
                    maploc->looted |= S_LRING;
                }
                return 1;
            }
            goto ouch;
        }
        if (maploc->typ == STAIRS || maploc->typ == LADDER
            || IS_STWALL(maploc->typ)) {
            if (!IS_STWALL(maploc->typ) && maploc->ladder == LA_DOWN)
                goto dumb;
        ouch:
            /*KR pline("Ouch!  That hurts!"); */
            pline("아얏! 아프다!");
            exercise(A_DEX, FALSE);
            exercise(A_STR, FALSE);
            if (isok(x, y)) {
                if (Blind)
                    feel_location(x, y); /* we know we hit it */
                if (is_drawbridge_wall(x, y) >= 0) {
                    /*KR pline_The("drawbridge is unaffected."); */
                    pline("도개교는 끄떡없다.");
                    /* update maploc to refer to the drawbridge */
                    (void) find_drawbridge(&x, &y);
                    maploc = &levl[x][y];
                }
            }
            if (!rn2(3))
                set_wounded_legs(RIGHT_SIDE, 5 + rnd(5));
            dmg = rnd(ACURR(A_CON) > 15 ? 3 : 5);
            losehp(Maybe_Half_Phys(dmg), kickstr(buf, kickobjnam), KILLED_BY);
            if (Is_airlevel(&u.uz) || Levitation)
                hurtle(-u.dx, -u.dy, rn1(2, 4), TRUE); /* assume it's heavy */
            return 1;
        }
        goto dumb;
    }

    if (maploc->doormask == D_ISOPEN || maploc->doormask == D_BROKEN
        || maploc->doormask == D_NODOOR) {
    dumb:
        exercise(A_DEX, FALSE);
        if (martial() || ACURR(A_DEX) >= 16 || rn2(3)) {
            /*KR You("kick at empty space."); */
            You("허공에 발길질을 했다.");
            if (Blind)
                feel_location(x, y);
        } else {
            /*KR pline("Dumb move!  You strain a muscle."); */
            pline("바보 같은 짓이다! 근육을 다치고 말았다.");
            exercise(A_STR, FALSE);
            set_wounded_legs(RIGHT_SIDE, 5 + rnd(5));
        }
        if ((Is_airlevel(&u.uz) || Levitation) && rn2(2))
            hurtle(-u.dx, -u.dy, 1, TRUE);
        return 1; /* uses a turn */
    }

    /* not enough leverage to kick open doors while levitating */
    if (Levitation)
        goto ouch;

    exercise(A_DEX, TRUE);
    /* door is known to be CLOSED or LOCKED */
    if (rnl(35) < avrg_attrib + (!martial() ? 0 : ACURR(A_DEX))) {
        boolean shopdoor = *in_rooms(x, y, SHOPBASE) ? TRUE : FALSE;
        /* break the door */
        if (maploc->doormask & D_TRAPPED) {
            if (flags.verbose)
                /*KR You("kick the door."); */
                You("문을 걷어찼다.");
            exercise(A_STR, FALSE);
            maploc->doormask = D_NODOOR;
            /*KR b_trapped("door", FOOT); */
            b_trapped("문", FOOT);
        } else if (ACURR(A_STR) > 18 && !rn2(5) && !shopdoor) {
            /*KR pline("As you kick the door, it shatters to pieces!"); */
            pline("당신이 문을 걷어차자, 문이 산산조각 나버렸다!");
            exercise(A_STR, TRUE);
            maploc->doormask = D_NODOOR;
        } else {
            /*KR pline("As you kick the door, it crashes open!"); */
            pline("당신이 문을 걷어차자, 요란한 소리와 함께 문이 열렸다!");
            exercise(A_STR, TRUE);
            maploc->doormask = D_BROKEN;
        }
        feel_newsym(x, y);   /* we know we broke it */
        unblock_point(x, y); /* vision */
        if (shopdoor) {
            add_damage(x, y, SHOP_DOOR_COST);
            /*KR pay_for_damage("break", FALSE); */
            pay_for_damage("부순", FALSE);
        }
        if (in_town(x, y))
            for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
                if (DEADMONSTER(mtmp))
                    continue;
                if (is_watch(mtmp->data) && couldsee(mtmp->mx, mtmp->my)
                    && mtmp->mpeaceful) {
                    /*KR mon_yells(mtmp, "Halt, thief!  You're under
                     * arrest!"); */
                    mon_yells(mtmp, "거기 서라, 도둑놈아! 넌 체포되었다!");
                    (void) angry_guards(FALSE);
                    break;
                }
            }
    } else {
        if (Blind)
            feel_location(x, y); /* we know we hit it */
        exercise(A_STR, TRUE);
        /*KR pline("WHAMMM!!!"); */
        pline("콰앙!!!");
        if (in_town(x, y))
            for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
                if (DEADMONSTER(mtmp))
                    continue;
                if (is_watch(mtmp->data) && mtmp->mpeaceful
                    && couldsee(mtmp->mx, mtmp->my)) {
                    if (levl[x][y].looted & D_WARNED) {
                        mon_yells(
                            mtmp,
                            /*KR "Halt, vandal!  You're under arrest!"); */
                            "거기 서라, 기물파손범아! 넌 체포되었다!");
                        (void) angry_guards(FALSE);
                    } else {
                        /*KR mon_yells(mtmp, "Hey, stop damaging that door!");
                         */
                        mon_yells(mtmp, "이봐, 문 부수지 마!");
                        levl[x][y].looted |= D_WARNED;
                    }
                    break;
                }
            }
    }
    return 1;
}

STATIC_OVL void drop_to(cc, loc) coord *cc;
schar loc;
{
    /* cover all the MIGR_xxx choices generated by down_gate() */
    switch (loc) {
    case MIGR_RANDOM: /* trap door or hole */
        if (Is_stronghold(&u.uz)) {
            cc->x = valley_level.dnum;
            cc->y = valley_level.dlevel;
            break;
        } else if (In_endgame(&u.uz) || Is_botlevel(&u.uz)) {
            cc->y = cc->x = 0;
            break;
        }
        /*FALLTHRU*/
    case MIGR_STAIRS_UP:
    case MIGR_LADDER_UP:
        cc->x = u.uz.dnum;
        cc->y = u.uz.dlevel + 1;
        break;
    case MIGR_SSTAIRS:
        cc->x = sstairs.tolev.dnum;
        cc->y = sstairs.tolev.dlevel;
        break;
    default:
    case MIGR_NOWHERE:
        /* y==0 means "nowhere", in which case x doesn't matter */
        cc->y = cc->x = 0;
        break;
    }
}

/* player or missile impacts location, causing objects to fall down */
void impact_drop(
    missile, x, y,
    dlev) struct obj *missile; /* caused impact, won't drop itself */
xchar x, y;                    /* location affected */
xchar dlev;                    /* if !0 send to dlev near player */
{
    schar toloc;
    register struct obj *obj, *obj2;
    register struct monst *shkp;
    long oct, dct, price, debit, robbed;
    boolean angry, costly, isrock;
    coord cc;

    if (!OBJ_AT(x, y))
        return;

    toloc = down_gate(x, y);
    drop_to(&cc, toloc);
    if (!cc.y)
        return;

    if (dlev) {
        /* send objects next to player falling through trap door.
         * checked in obj_delivery().
         */
        toloc = MIGR_WITH_HERO;
        cc.y = dlev;
    }

    costly = costly_spot(x, y);
    price = debit = robbed = 0L;
    angry = FALSE;
    shkp = (struct monst *) 0;
    /* if 'costly', we must keep a record of ESHK(shkp) before
     * it undergoes changes through the calls to stolen_value.
     * the angry bit must be reset, if needed, in this fn, since
     * stolen_value is called under the 'silent' flag to avoid
     * unsavory pline repetitions.
     */
    if (costly) {
        if ((shkp = shop_keeper(*in_rooms(x, y, SHOPBASE))) != 0) {
            debit = ESHK(shkp)->debit;
            robbed = ESHK(shkp)->robbed;
            angry = !shkp->mpeaceful;
        }
    }

    isrock = (missile && missile->otyp == ROCK);
    oct = dct = 0L;
    for (obj = level.objects[x][y]; obj; obj = obj2) {
        obj2 = obj->nexthere;
        if (obj == missile)
            continue;
        /* number of objects in the pile */
        oct += obj->quan;
        if (obj == uball || obj == uchain)
            continue;
        /* boulders can fall too, but rarely & never due to rocks */
        if ((isrock && obj->otyp == BOULDER)
            || rn2(obj->otyp == BOULDER ? 30 : 3))
            continue;
        obj_extract_self(obj);

        if (costly) {
            price +=
                stolen_value(obj, x, y,
                             (costly_spot(u.ux, u.uy)
                              && index(u.urooms, *in_rooms(x, y, SHOPBASE))),
                             TRUE);
            /* set obj->no_charge to 0 */
            if (Has_contents(obj))
                picked_container(obj); /* does the right thing */
            if (obj->oclass != COIN_CLASS)
                obj->no_charge = 0;
        }

        add_to_migration(obj);
        obj->ox = cc.x;
        obj->oy = cc.y;
        obj->owornmask = (long) toloc;

        /* number of fallen objects */
        dct += obj->quan;
    }

    if (dct && cansee(x, y)) { /* at least one object fell */
        /*KR const char *what = (dct == 1L ? "object falls" : "objects fall");
         */
        const char *what =
            (dct == 1L ? "물체가 떨어진다" : "물체들이 떨어진다");

        if (missile)
#if 0 /*KR: 원본*/
            pline("From the impact, %sother %s.",
                  dct == oct ? "the " : dct == 1L ? "an" : "", what);
#else /*KR: KRNethack 맞춤 번역 */
            pline("그 충격으로 인해, %s다른 %s.",
                  dct == oct  ? "그 "
                  : dct == 1L ? "한 "
                              : "",
                  what);
#endif
        else if (oct == dct)
#if 0 /*KR: 원본*/
            pline("%s adjacent %s %s.", dct == 1L ? "The" : "All the", what,
                  gate_str);
#else /*KR: KRNethack 맞춤 번역 */
            pline("인접한 %s%s %s.", dct == 1L ? "" : "모든 ", what,
                  gate_str);
#endif
        else
#if 0 /*KR: 원본*/
            pline("%s adjacent %s %s.",
                  dct == 1L ? "One of the" : "Some of the",
                  dct == 1L ? "objects falls" : what, gate_str);
#else /*KR: KRNethack 맞춤 번역 */
            pline("인접한 %s 중 %s %s.", dct == 1L ? "물체" : "물체들",
                  dct == 1L ? "하나가 떨어지며" : "일부가 떨어지며",
                  gate_str);
#endif
    }

    if (costly && shkp && price) {
        if (ESHK(shkp)->robbed > robbed) {
            /*KR You("removed %ld %s worth of goods!", price,
             * currency(price)); */
            You("%ld %s 어치의 상품을 없애버렸다!", price, currency(price));
            if (cansee(shkp->mx, shkp->my)) {
                if (ESHK(shkp)->customer[0] == 0)
                    (void) strncpy(ESHK(shkp)->customer, plname, PL_NSIZ);
                if (angry)
                    /*KR pline("%s is infuriated!", Monnam(shkp)); */
                    pline("%s 몹시 화가 났다!",
                          append_josa(Monnam(shkp), "은"));
                else
                    /*KR pline("\"%s, you are a thief!\"", plname); */
                    pline("\"%s, 너는 도둑이다!\"", plname);
            } else
                /*KR You_hear("a scream, \"Thief!\""); */
                You_hear("\"도둑이야!\" 하는 비명 소리가 들린다.");
            hot_pursuit(shkp);
            (void) angry_guards(FALSE);
            return;
        }
        if (ESHK(shkp)->debit > debit) {
            long amt = (ESHK(shkp)->debit - debit);
#if 0 /*KR: 원본*/
            You("owe %s %ld %s for goods lost.", Monnam(shkp), amt,
                currency(amt));
#else /*KR: KRNethack 맞춤 번역 */
            You("잃어버린 상품에 대해 %s에게 %ld %s을(를) 빚졌다.",
                Monnam(shkp), amt, currency(amt));
#endif
        }
    }
}

/* NOTE: ship_object assumes otmp was FREED from fobj or invent.
 * <x,y> is the point of drop.  otmp is _not_ an <x,y> resident:
 * otmp is either a kicked, dropped, or thrown object.
 */
boolean
ship_object(otmp, x, y, shop_floor_obj)
xchar x, y;
struct obj *otmp;
boolean shop_floor_obj;
{
    schar toloc;
    xchar ox, oy;
    coord cc;
    struct obj *obj;
    struct trap *t;
    boolean nodrop, unpaid, container, impact = FALSE;
    long n = 0L;

    if (!otmp)
        return FALSE;
    if ((toloc = down_gate(x, y)) == MIGR_NOWHERE)
        return FALSE;
    drop_to(&cc, toloc);
    if (!cc.y)
        return FALSE;

    /* objects other than attached iron ball always fall down ladder,
       but have a chance of staying otherwise */
    nodrop = (otmp == uball) || (otmp == uchain)
             || (toloc != MIGR_LADDER_UP && rn2(3));

    container = Has_contents(otmp);
    unpaid = is_unpaid(otmp);

    if (OBJ_AT(x, y)) {
        for (obj = level.objects[x][y]; obj; obj = obj->nexthere)
            if (obj != otmp)
                n += obj->quan;
        if (n)
            impact = TRUE;
    }
    /* boulders never fall through trap doors, but they might knock
       other things down before plugging the hole */
    if (otmp->otyp == BOULDER && ((t = t_at(x, y)) != 0)
        && is_hole(t->ttyp)) {
        if (impact)
            impact_drop(otmp, x, y, 0);
        return FALSE; /* let caller finish the drop */
    }

    if (cansee(x, y))
        otransit_msg(otmp, nodrop, n);

    if (nodrop) {
        if (impact)
            impact_drop(otmp, x, y, 0);
        return FALSE;
    }

    if (unpaid || shop_floor_obj) {
        if (unpaid) {
            (void) stolen_value(otmp, u.ux, u.uy, TRUE, FALSE);
        } else {
            ox = otmp->ox;
            oy = otmp->oy;
            (void) stolen_value(
                otmp, ox, oy,
                (costly_spot(u.ux, u.uy)
                 && index(u.urooms, *in_rooms(ox, oy, SHOPBASE))),
                FALSE);
        }
        /* set otmp->no_charge to 0 */
        if (container)
            picked_container(otmp); /* happens to do the right thing */
        if (otmp->oclass != COIN_CLASS)
            otmp->no_charge = 0;
    }

    if (otmp->owornmask)
        remove_worn_item(otmp, TRUE);

    /* some things break rather than ship */
    if (breaktest(otmp)) {
        const char *result;

        if (objects[otmp->otyp].oc_material == GLASS
            || otmp->otyp == EXPENSIVE_CAMERA) {
            if (otmp->otyp == MIRROR)
                change_luck(-2);
            /*KR result = "crash"; */
            result = "쨍그랑";
        } else {
            /* penalty for breaking eggs laid by you */
            if (otmp->otyp == EGG && otmp->spe && otmp->corpsenm >= LOW_PM)
                change_luck((schar) -min(otmp->quan, 5L));
            /*KR result = "splat"; */
            result = "철퍼덕";
        }
        /*KR You_hear("a muffled %s.", result); */
        You_hear("안에서 들려오는 %s 소리를 들었다.", result);
        obj_extract_self(otmp);
        obfree(otmp, (struct obj *) 0);
        return TRUE;
    }

    add_to_migration(otmp);
    otmp->ox = cc.x;
    otmp->oy = cc.y;
    otmp->owornmask = (long) toloc;
    /* boulder from rolling boulder trap, no longer part of the trap */
    if (otmp->otyp == BOULDER)
        otmp->otrapped = 0;

    if (impact) {
        /* the objs impacted may be in a shop other than
         * the one in which the hero is located.  another
         * check for a shk is made in impact_drop.  it is, e.g.,
         * possible to kick/throw an object belonging to one
         * shop into another shop through a gap in the wall,
         * and cause objects belonging to the other shop to
         * fall down a trap door--thereby getting two shopkeepers
         * angry at the hero in one shot.
         */
        impact_drop(otmp, x, y, 0);
        newsym(x, y);
    }
    return TRUE;
}

void obj_delivery(near_hero) boolean near_hero;
{
    register struct obj *otmp, *otmp2;
    register int nx, ny;
    int where;
    boolean nobreak, noscatter;

    for (otmp = migrating_objs; otmp; otmp = otmp2) {
        otmp2 = otmp->nobj;
        if (otmp->ox != u.uz.dnum || otmp->oy != u.uz.dlevel)
            continue;

        where = (int) (otmp->owornmask & 0x7fffL); /* destination code */
        if ((where & MIGR_TO_SPECIES) != 0)
            continue;

        nobreak = (where & MIGR_NOBREAK) != 0;
        noscatter = (where & MIGR_WITH_HERO) != 0;
        where &= ~(MIGR_NOBREAK | MIGR_NOSCATTER);

        if (!near_hero ^ (where == MIGR_WITH_HERO))
            continue;

        obj_extract_self(otmp);
        otmp->owornmask = 0L;

        switch (where) {
        case MIGR_STAIRS_UP:
            nx = xupstair, ny = yupstair;
            break;
        case MIGR_LADDER_UP:
            nx = xupladder, ny = yupladder;
            break;
        case MIGR_SSTAIRS:
            nx = sstairs.sx, ny = sstairs.sy;
            break;
        case MIGR_WITH_HERO:
            nx = u.ux, ny = u.uy;
            break;
        default:
        case MIGR_RANDOM:
            nx = ny = 0;
            break;
        }
        if (nx > 0) {
            place_object(otmp, nx, ny);
            if (!nobreak && !IS_SOFT(levl[nx][ny].typ)) {
                if (where == MIGR_WITH_HERO) {
                    if (breaks(otmp, nx, ny))
                        continue;
                } else if (breaktest(otmp)) {
                    /* assume it broke before player arrived, no messages */
                    delobj(otmp);
                    continue;
                }
            }
            stackobj(otmp);
            if (!noscatter)
                (void) scatter(nx, ny, rnd(2), 0, otmp);
            else
                newsym(nx, ny);
        } else { /* random location */
            /* set dummy coordinates because there's no
               current position for rloco() to update */
            otmp->ox = otmp->oy = 0;
            if (rloco(otmp) && !nobreak && breaktest(otmp)) {
                /* assume it broke before player arrived, no messages */
                delobj(otmp);
            }
        }
    }
}

void deliver_obj_to_mon(mtmp, cnt, deliverflags) int cnt;
struct monst *mtmp;
unsigned long deliverflags;
{
    struct obj *otmp, *otmp2;
    int where, maxobj = 1;
    boolean at_crime_scene = In_mines(&u.uz);

    if ((deliverflags & DF_RANDOM) && cnt > 1)
        maxobj = rnd(cnt);
    else if (deliverflags & DF_ALL)
        maxobj = 0;
    else
        maxobj = 1;

    cnt = 0;
    for (otmp = migrating_objs; otmp; otmp = otmp2) {
        otmp2 = otmp->nobj;
        where = (int) (otmp->owornmask & 0x7fffL); /* destination code */
        if ((where & MIGR_TO_SPECIES) == 0)
            continue;

        if ((mtmp->data->mflags2 & otmp->corpsenm) != 0) {
            obj_extract_self(otmp);
            otmp->owornmask = 0L;
            otmp->ox = otmp->oy = 0;

            /* special treatment for orcs and their kind */
            if ((otmp->corpsenm & M2_ORC) != 0 && has_oname(otmp)) {
                if (!has_mname(mtmp)) {
                    if (at_crime_scene || !rn2(2))
                        mtmp = christen_orc(
                            mtmp, at_crime_scene ? ONAME(otmp) : (char *) 0,
                            /* bought the stolen goods */
                            " 장물아비");
                }
                free_oname(otmp);
            }
            otmp->corpsenm = 0;
            (void) add_to_minv(mtmp, otmp);
            cnt++;
            if (maxobj && cnt >= maxobj)
                break;
            /* getting here implies DF_ALL */
        }
    }
}

STATIC_OVL void otransit_msg(otmp, nodrop, num) register struct obj *otmp;
register boolean nodrop;
long num;
{
#if 0 /*KR*/
    char *optr = 0, obuf[BUFSZ], xbuf[BUFSZ];
#else
    char obuf[BUFSZ], xbuf[BUFSZ];
#endif

#if 0 /*KR*/
    if (otmp->otyp == CORPSE) {
        /* Tobjnam() calls xname() and would yield "The corpse";
           we want more specific "The newt corpse" or "Medusa's corpse" */
        optr = upstart(corpse_xname(otmp, (char *) 0, CXN_PFX_THE));
    } else {
        optr = Tobjnam(otmp, (char *) 0);
    }
    Strcpy(obuf, optr);
#else
    Sprintf(obuf, "%s", append_josa(xname(otmp), "은(는)"));
#endif

    if (num) { /* means: other objects are impacted */
        /* As of 3.6.2: use a separate buffer for the suffix to avoid risk of
           overrunning obuf[] (let pline() handle truncation if necessary) */
#if 0 /*KR*/
        Sprintf(xbuf, " %s %s object%s", otense(otmp, "hit"),
                (num == 1L) ? "another" : "other", (num > 1L) ? "s" : "");
        if (nodrop)
            Sprintf(eos(xbuf), ".");
        else
            Sprintf(eos(xbuf), " and %s %s.", otense(otmp, "fall"), gate_str);
#else
        Sprintf(xbuf, " 다른 %s 물체에 부딪혔고",
                (num == 1L) ? "하나의" : "여러 개의");
        if (nodrop)
            Sprintf(eos(xbuf), " 이내 멈췄다.");
        else
            Sprintf(eos(xbuf), " %s 떨어졌다.", gate_str);
#endif
        pline("%s%s", obuf, xbuf);
    } else if (!nodrop)
        /*KR pline("%s %s %s.", obuf, otense(otmp, "fall"), gate_str); */
        pline("%s %s 떨어졌다.", obuf, gate_str);
}

/* migration destination for objects which fall down to next level */
schar
down_gate(x, y)
xchar x, y;
{
    struct trap *ttmp;

    gate_str = 0;
    /* this matches the player restriction in goto_level() */
    if (on_level(&u.uz, &qstart_level) && !ok_to_quest())
        return MIGR_NOWHERE;

    if ((xdnstair == x && ydnstair == y)
        || (sstairs.sx == x && sstairs.sy == y && !sstairs.up)) {
        /*KR gate_str = "down the stairs"; */
        gate_str = "계단 아래로";
        return (xdnstair == x && ydnstair == y) ? MIGR_STAIRS_UP
                                                : MIGR_SSTAIRS;
    }
    if (xdnladder == x && ydnladder == y) {
        /*KR gate_str = "down the ladder"; */
        gate_str = "사다리 아래로";
        return MIGR_LADDER_UP;
    }

    if (((ttmp = t_at(x, y)) != 0 && ttmp->tseen) && is_hole(ttmp->ttyp)) {
#if 0 /*KR: 원본*/
        gate_str = (ttmp->ttyp == TRAPDOOR) ? "through the trap door"
                                            : "through the hole";
#else /*KR: KRNethack 맞춤 번역 */
        gate_str = (ttmp->ttyp == TRAPDOOR) ? "트랩도어를 통과하여"
                                            : "구멍을 통과하여";
#endif
        return MIGR_RANDOM;
    }
    return MIGR_NOWHERE;
}

/*dokick.c*/
