/* NetHack 3.6	polyself.c	$NHDT-Date: 1573290419 2019/11/09 09:06:59 $  $NHDT-Branch: NetHack-3.6 $:$NHDT-Revision: 1.135 $ */
/* Copyright (C) 1987, 1988, 1989 by Ken Arromdee */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * Polymorph self routine.
 *
 * Note:  the light source handling code assumes that both youmonst.m_id
 * and youmonst.mx will always remain 0 when it handles the case of the
 * player polymorphed into a light-emitting monster.
 *
 * Transformation sequences:
 * /-> polymon                 poly into monster form
 * polyself =
 * \-> newman -> polyman       fail to poly, get human form
 *
 * rehumanize -> polyman                  return to original form
 *
 * polymon (called directly)              usually golem petrification
 */

#include "hack.h"

STATIC_DCL void FDECL(check_strangling, (BOOLEAN_P));
STATIC_DCL void FDECL(polyman, (const char *, const char *) );
STATIC_DCL void FDECL(dropp, (struct obj *) );
STATIC_DCL void NDECL(break_armor);
STATIC_DCL void FDECL(drop_weapon, (int) );
STATIC_DCL int FDECL(armor_to_dragon, (int) );
STATIC_DCL void NDECL(newman);
STATIC_DCL void NDECL(polysense);

STATIC_VAR const char no_longer_petrify_resistant[] =
    /*KR "No longer petrify-resistant, you"; */
    "더 이상 석화에 대한 내성이 없는 당신은";

/* controls whether taking on new form or becoming new man can also
   change sex (ought to be an arg to polymon() and newman() instead) */
STATIC_VAR int sex_change_ok = 0;

/* update the youmonst.data structure pointer and intrinsics */
void
set_uasmon()
{
    struct permonst *mdat = &mons[u.umonnum];

    set_mon_data(&youmonst, mdat);

#define PROPSET(PropIndx, ON)                          \
    do {                                               \
        if (ON)                                        \
            u.uprops[PropIndx].intrinsic |= FROMFORM;  \
        else                                           \
            u.uprops[PropIndx].intrinsic &= ~FROMFORM; \
    } while (0)

    PROPSET(FIRE_RES, resists_fire(&youmonst));
    PROPSET(COLD_RES, resists_cold(&youmonst));
    PROPSET(SLEEP_RES, resists_sleep(&youmonst));
    PROPSET(DISINT_RES, resists_disint(&youmonst));
    PROPSET(SHOCK_RES, resists_elec(&youmonst));
    PROPSET(POISON_RES, resists_poison(&youmonst));
    PROPSET(ACID_RES, resists_acid(&youmonst));
    PROPSET(STONE_RES, resists_ston(&youmonst));
    {
        /* resists_drli() takes wielded weapon into account; suppress it */
        struct obj *save_uwep = uwep;

        uwep = 0;
        PROPSET(DRAIN_RES, resists_drli(&youmonst));
        uwep = save_uwep;
    }
    /* resists_magm() takes wielded, worn, and carried equipment into
       into account; cheat and duplicate its monster-specific part */
    PROPSET(ANTIMAGIC,
            (dmgtype(mdat, AD_MAGM) || mdat == &mons[PM_BABY_GRAY_DRAGON]
             || dmgtype(mdat, AD_RBRE)));
    PROPSET(SICK_RES, (mdat->mlet == S_FUNGUS || mdat == &mons[PM_GHOUL]));

    PROPSET(STUNNED, (mdat == &mons[PM_STALKER] || is_bat(mdat)));
    PROPSET(HALLUC_RES, dmgtype(mdat, AD_HALU));
    PROPSET(SEE_INVIS, perceives(mdat));
    PROPSET(TELEPAT, telepathic(mdat));
    /* note that Infravision uses mons[race] rather than usual mons[role] */
    PROPSET(INFRAVISION, infravision(Upolyd ? mdat : &mons[urace.malenum]));
    PROPSET(INVIS, pm_invisible(mdat));
    PROPSET(TELEPORT, can_teleport(mdat));
    PROPSET(TELEPORT_CONTROL, control_teleport(mdat));
    PROPSET(LEVITATION, is_floater(mdat));
    /* floating eye is the only 'floater'; it is also flagged as a 'flyer';
       suppress flying for it so that enlightenment doesn't confusingly
       show latent flight capability always blocked by levitation */
    PROPSET(FLYING, (is_flyer(mdat) && !is_floater(mdat)));
    PROPSET(SWIMMING, is_swimmer(mdat));
    /* [don't touch MAGICAL_BREATHING here; both Amphibious and Breathless
       key off of it but include different monster forms...] */
    PROPSET(PASSES_WALLS, passes_walls(mdat));
    PROPSET(REGENERATION, regenerates(mdat));
    PROPSET(REFLECTING, (mdat == &mons[PM_SILVER_DRAGON]));
#undef PROPSET

    float_vs_flight(); /* maybe toggle (BFlying & I_SPECIAL) */
    polysense();

#ifdef STATUS_HILITES
    if (VIA_WINDOWPORT())
        status_initialize(REASSESS_ONLY);
#endif
}

/* Levitation overrides Flying; set or clear BFlying|I_SPECIAL */
void
float_vs_flight()
{
    boolean stuck_in_floor = (u.utrap && u.utraptype != TT_PIT);

    /* floating overrides flight; so does being trapped in the floor */
    if ((HLevitation || ELevitation)
        || ((HFlying || EFlying) && stuck_in_floor))
        BFlying |= I_SPECIAL;
    else
        BFlying &= ~I_SPECIAL;
    /* being trapped on the ground (bear trap, web, molten lava survived
       with fire resistance, former lava solidified via cold, tethered
       to a buried iron ball) overrides floating--the floor is reachable */
    if ((HLevitation || ELevitation) && stuck_in_floor)
        BLevitation |= I_SPECIAL;
    else
        BLevitation &= ~I_SPECIAL;
    context.botl = TRUE;
}

/* for changing into form that's immune to strangulation */
STATIC_OVL void check_strangling(on) boolean on;
{
    /* on -- maybe resume strangling */
    if (on) {
        /* when Strangled is already set, polymorphing from one
           vulnerable form into another causes the counter to be reset */
        if (uamul && uamul->otyp == AMULET_OF_STRANGULATION
            && can_be_strangled(&youmonst)) {
            Strangled = 6L;
            context.botl = TRUE;
            /*KR Your("%s %s your %s!", simpleonames(uamul),
                 Strangled ? "still constricts" : "begins constricting",
                 body_part(NECK)); */ /* "throat" */
            Your("%s 당신의 %s %s!", append_josa(simpleonames(uamul), "이"),
                 append_josa(body_part(NECK), "을"),
                 Strangled ? "여전히 조이고 있다" : "조이기 시작한다");
            makeknown(AMULET_OF_STRANGULATION);
        }

        /* off -- maybe block strangling */
    } else {
        if (Strangled && !can_be_strangled(&youmonst)) {
            Strangled = 0L;
            context.botl = TRUE;
            /*KR You("are no longer being strangled."); */
            You("더 이상 목이 조이지 않는다.");
        }
    }
}

/* make a (new) human out of the player */
STATIC_OVL void polyman(fmt, arg) const char *fmt, *arg;
{
    boolean sticky = (sticks(youmonst.data) && u.ustuck && !u.uswallow),
            was_mimicking = (U_AP_TYPE != M_AP_NOTHING);
    boolean was_blind = !!Blind;

    if (Upolyd) {
        u.acurr = u.macurr; /* restore old attribs */
        u.amax = u.mamax;
        u.umonnum = u.umonster;
        flags.female = u.mfemale;
    }
    set_uasmon();

    u.mh = u.mhmax = 0;
    u.mtimedone = 0;
    skinback(FALSE);
    u.uundetected = 0;

    if (sticky)
        uunstick();
    find_ac();
    if (was_mimicking) {
        if (multi < 0)
            unmul("");
        youmonst.m_ap_type = M_AP_NOTHING;
        youmonst.mappearance = 0;
    }

    newsym(u.ux, u.uy);

    You(fmt, arg);
    /* check whether player foolishly genocided self while poly'd */
    if (ugenocided()) {
        /* intervening activity might have clobbered genocide info */
        struct kinfo *kptr = find_delayed_killer(POLYMORPH);

        if (kptr != (struct kinfo *) 0 && kptr->name[0]) {
            killer.format = kptr->format;
            Strcpy(killer.name, kptr->name);
        } else {
            killer.format = KILLED_BY;
            /*KR Strcpy(killer.name, "self-genocide"); */
            Strcpy(killer.name, "스스로를 학살함");
        }
        dealloc_killer(kptr);
        done(GENOCIDED);
    }

    if (u.twoweap && !could_twoweap(youmonst.data))
        untwoweapon();

    if (u.utrap && u.utraptype == TT_PIT) {
        set_utrap(rn1(6, 2), TT_PIT); /* time to escape resets */
    }
    if (was_blind && !Blind) { /* reverting from eyeless */
        Blinded = 1L;
        make_blinded(0L, TRUE); /* remove blindness */
    }
    check_strangling(TRUE);

    if (!Levitation && !u.ustuck && is_pool_or_lava(u.ux, u.uy))
        spoteffects(TRUE);

    see_monsters();
}

void
change_sex()
{
    /* setting u.umonster for caveman/cavewoman or priest/priestess
       swap unintentionally makes `Upolyd' appear to be true */
    boolean already_polyd = (boolean) Upolyd;

    /* Some monsters are always of one sex and their sex can't be changed;
     * Succubi/incubi can change, but are handled below.
     *
     * !already_polyd check necessary because is_male() and is_female()
     * are true if the player is a priest/priestess.
     */
    if (!already_polyd
        || (!is_male(youmonst.data) && !is_female(youmonst.data)
            && !is_neuter(youmonst.data)))
        flags.female = !flags.female;
    if (already_polyd) /* poly'd: also change saved sex */
        u.mfemale = !u.mfemale;
    max_rank_sz(); /* [this appears to be superfluous] */
    if ((already_polyd ? u.mfemale : flags.female) && urole.name.f)
        Strcpy(pl_character, urole.name.f);
    else
        Strcpy(pl_character, urole.name.m);
    u.umonster = ((already_polyd ? u.mfemale : flags.female)
                  && urole.femalenum != NON_PM)
                     ? urole.femalenum
                     : urole.malenum;
    if (!already_polyd) {
        u.umonnum = u.umonster;
    } else if (u.umonnum == PM_SUCCUBUS || u.umonnum == PM_INCUBUS) {
        flags.female = !flags.female;
        /* change monster type to match new sex */
        u.umonnum = (u.umonnum == PM_SUCCUBUS) ? PM_INCUBUS : PM_SUCCUBUS;
        set_uasmon();
    }
}

STATIC_OVL void
newman()
{
    int i, oldlvl, newlvl, hpmax, enmax;

    oldlvl = u.ulevel;
    newlvl = oldlvl + rn1(5, -2);     /* new = old + {-2,-1,0,+1,+2} */
    if (newlvl > 127 || newlvl < 1) { /* level went below 0? */
        goto dead; /* old level is still intact (in case of lifesaving) */
    }
    if (newlvl > MAXULEV)
        newlvl = MAXULEV;
    /* If your level goes down, your peak level goes down by
       the same amount so that you can't simply use blessed
       full healing to undo the decrease.  But if your level
       goes up, your peak level does *not* undergo the same
       adjustment; you might end up losing out on the chance
       to regain some levels previously lost to other causes. */
    if (newlvl < oldlvl)
        u.ulevelmax -= (oldlvl - newlvl);
    if (u.ulevelmax < newlvl)
        u.ulevelmax = newlvl;
    u.ulevel = newlvl;

    if (sex_change_ok && !rn2(10))
        change_sex();

    adjabil(oldlvl, (int) u.ulevel);
    reset_rndmonst(NON_PM); /* new monster generation criteria */

    /* random experience points for the new experience level */
    u.uexp = rndexp(FALSE);

    /* set up new attribute points (particularly Con) */
    redist_attr();

    /*
     * New hit points:
     * remove level-gain based HP from any extra HP accumulated
     * (the "extra" might actually be negative);
     * modify the extra, retaining {80%, 90%, 100%, or 110%};
     * add in newly generated set of level-gain HP.
     *
     * (This used to calculate new HP in direct proportion to old HP,
     * but that was subject to abuse:  accumulate a large amount of
     * extra HP, drain level down to 1, then polyself to level 2 or 3
     * [lifesaving capability needed to handle level 0 and -1 cases]
     * and the extra got multiplied by 2 or 3.  Repeat the level
     * drain and polyself steps until out of lifesaving capability.)
     */
    hpmax = u.uhpmax;
    for (i = 0; i < oldlvl; i++)
        hpmax -= (int) u.uhpinc[i];
    /* hpmax * rn1(4,8) / 10; 0.95*hpmax on average */
    hpmax = rounddiv((long) hpmax * (long) rn1(4, 8), 10);
    for (i = 0; (u.ulevel = i) < newlvl; i++)
        hpmax += newhp();
    if (hpmax < u.ulevel)
        hpmax = u.ulevel; /* min of 1 HP per level */
    /* retain same proportion for current HP; u.uhp * hpmax / u.uhpmax */
    u.uhp = rounddiv((long) u.uhp * (long) hpmax, u.uhpmax);
    u.uhpmax = hpmax;
    /*
     * Do the same for spell power.
     */
    enmax = u.uenmax;
    for (i = 0; i < oldlvl; i++)
        enmax -= (int) u.ueninc[i];
    enmax = rounddiv((long) enmax * (long) rn1(4, 8), 10);
    for (i = 0; (u.ulevel = i) < newlvl; i++)
        enmax += newpw();
    if (enmax < u.ulevel)
        enmax = u.ulevel;
    u.uen = rounddiv((long) u.uen * (long) enmax,
                     ((u.uenmax < 1) ? 1 : u.uenmax));
    u.uenmax = enmax;
    /* [should alignment record be tweaked too?] */

    u.uhunger = rn1(500, 500);
    if (Sick)
        make_sick(0L, (char *) 0, FALSE, SICK_ALL);
    if (Stoned)
        make_stoned(0L, (char *) 0, 0, (char *) 0);
    if (u.uhp <= 0) {
        if (Polymorph_control) { /* even when Stunned || Unaware */
            if (u.uhp <= 0)
                u.uhp = 1;
        } else {
        dead: /* we come directly here if their experience level went to 0 or
                 less */
            /*KR Your("new form doesn't seem healthy enough to survive."); */
            Your("새로운 몸은 살아남을 수 있을 만큼 건강해 보이지 않는다.");
            killer.format = KILLED_BY_AN;
            /*KR Strcpy(killer.name, "unsuccessful polymorph"); */
            Strcpy(killer.name, "실패한 변이");
            done(DIED);
            newuhs(FALSE);
            return; /* lifesaved */
        }
    }
    newuhs(FALSE);
#if 0 /*KR: 원본*/
    polyman("feel like a new %s!",
            /* use saved gender we're about to revert to, not current */
            ((Upolyd ? u.mfemale : flags.female) && urace.individual.f)
                ? urace.individual.f
                : (urace.individual.m)
                   ? urace.individual.m
                   : urace.noun);
#else /*KR: KRNethack 맞춤 번역 */
    polyman("새로운 %s(이)가 된 기분이다!",
            /* use saved gender we're about to revert to, not current */
            ((Upolyd ? u.mfemale : flags.female) && urace.individual.f)
                ? urace.individual.f
            : (urace.individual.m) ? urace.individual.m
                                   : urace.noun);
#endif
    if (Slimed) {
        /*KR Your("body transforms, but there is still slime on you."); */
        Your("몸은 변했지만, 여전히 슬라임이 묻어 있다.");
        make_slimed(10L, (const char *) 0);
    }

    context.botl = 1;
    see_monsters();
    (void) encumber_msg();

    retouch_equipment(2);
    if (!uarmg)
        selftouch(no_longer_petrify_resistant);
}

void polyself(psflags) int psflags;
{
    char buf[BUFSZ] = DUMMY;
    int old_light, new_light, mntmp, class, tryct;
    boolean forcecontrol = (psflags == 1), monsterpoly = (psflags == 2),
            draconian = (uarm && Is_dragon_armor(uarm)),
            iswere = (u.ulycn >= LOW_PM), isvamp = is_vampire(youmonst.data),
            controllable_poly = Polymorph_control && !(Stunned || Unaware);

    if (Unchanging) {
        /*KR pline("You fail to transform!"); */
        pline("당신은 변이하는 데 실패했다!");
        return;
    }
    /* being Stunned|Unaware doesn't negate this aspect of Poly_control */
    if (!Polymorph_control && !forcecontrol && !draconian && !iswere
        && !isvamp) {
        if (rn2(20) > ACURR(A_CON)) {
            You1(shudder_for_moment);
            /*KR losehp(rnd(30), "system shock", KILLED_BY_AN); */
            losehp(rnd(30), "시스템 쇼크", KILLED_BY_AN);
            exercise(A_CON, FALSE);
            return;
        }
    }
    old_light = emits_light(youmonst.data);
    mntmp = NON_PM;

    if (monsterpoly && isvamp)
        goto do_vampyr;

    if (controllable_poly || forcecontrol) {
        tryct = 5;
        do {
            mntmp = NON_PM;
            /*KR getlin("Become what kind of monster? [type the name]", buf);
             */
            getlin("어떤 종류의 몬스터가 되겠습니까? [이름을 입력하세요]",
                   buf);
            (void) mungspaces(buf);
            if (*buf == '\033') {
                /* user is cancelling controlled poly */
                if (forcecontrol) { /* wizard mode #polyself */
                    pline1(Never_mind);
                    return;
                }
                Strcpy(buf, "*"); /* resort to random */
            }
#if 0 /*KR: 원본*/
            if (!strcmp(buf, "*") || !strcmp(buf, "random")) {
#else /*KR: KRNethack 맞춤 번역 */
            if (!strcmp(buf, "*") || !strcmp(buf, "랜덤")) {
#endif
                /* explicitly requesting random result */
                tryct = 0; /* will skip thats_enough_tries */
                continue;  /* end do-while(--tryct > 0) loop */
            }
            class = 0;
            mntmp = name_to_mon(buf);
            if (mntmp < LOW_PM) {
            by_class:
                class = name_to_monclass(buf, &mntmp);
                if (class && mntmp == NON_PM)
                    mntmp = mkclass_poly(class);
            }
            if (mntmp < LOW_PM) {
                if (!class)
                    /*KR pline("I've never heard of such monsters."); */
                    pline("그런 몬스터는 들어본 적도 없다.");
                else
                    /*KR You_cant("polymorph into any of those."); */
                    You_cant("그러한 것들 중 어떤 것으로도 변이할 수 없다.");
            } else if (iswere
                       && (were_beastie(mntmp) == u.ulycn
                           || mntmp == counter_were(u.ulycn)
                           || (Upolyd && mntmp == PM_HUMAN))) {
                goto do_shift;
                /* Note:  humans are illegal as monsters, but an
                 * illegal monster forces newman(), which is what we
                 * want if they specified a human.... */
            } else if (!polyok(&mons[mntmp])
                       && !(mntmp == PM_HUMAN || your_race(&mons[mntmp])
                            || mntmp == urole.malenum
                            || mntmp == urole.femalenum)) {
                const char *pm_name;

                /* mkclass_poly() can pick a !polyok()
                   candidate; if so, usually try again */
                if (class) {
                    if (rn2(3) || --tryct > 0)
                        goto by_class;
                    /* no retries left; put one back on counter
                       so that end of loop decrement will yield
                       0 and trigger thats_enough_tries message */
                    ++tryct;
                }
                pm_name = mons[mntmp].mname;
                if (the_unique_pm(&mons[mntmp]))
                    pm_name = the(pm_name);
                else if (!type_is_pname(&mons[mntmp]))
                    pm_name = an(pm_name);
                /*KR You_cant("polymorph into %s.", pm_name); */
                You_cant("%s(으)로 변이할 수 없다.", pm_name);
            } else
                break;
        } while (--tryct > 0);
        if (!tryct)
            pline1(thats_enough_tries);
        /* allow skin merging, even when polymorph is controlled */
        if (draconian && (tryct <= 0 || mntmp == armor_to_dragon(uarm->otyp)))
            goto do_merge;
        if (isvamp
            && (tryct <= 0 || mntmp == PM_WOLF || mntmp == PM_FOG_CLOUD
                || is_bat(&mons[mntmp])))
            goto do_vampyr;
    } else if (draconian || iswere || isvamp) {
        /* special changes that don't require polyok() */
        if (draconian) {
        do_merge:
            mntmp = armor_to_dragon(uarm->otyp);
            if (!(mvitals[mntmp].mvflags & G_GENOD)) {
                /* allow G_EXTINCT */
                if (Is_dragon_scales(uarm)) {
                    /* dragon scales remain intact as uskin */
                    /*KR You("merge with your scaly armor."); */
                    You("당신의 비늘 갑옷과 하나가 되었다.");
                } else { /* dragon scale mail */
                    /* d.scale mail first reverts to scales */
                    char *p, *dsmail;

                    /* similar to noarmor(invent.c),
                       shorten to "<color> scale mail" */
                    dsmail = strcpy(buf, simpleonames(uarm));
                    /*KR if ((p = strstri(dsmail, " dragon ")) != 0) */
                    if ((p = strstri(dsmail, " 드래곤 ")) != 0)
                        while ((p[1] = p[8]) != '\0')
                            ++p;
                    /* tricky phrasing; dragon scale mail
                       is singular, dragon scales are plural */
#if 0 /*KR: 원본*/
                    Your("%s reverts to scales as you merge with them.",
                         dsmail);
#else /*KR: KRNethack 맞춤 번역 */
                    Your("%s 하나로 융합되면서 다시 비늘로 변했다.",
                         append_josa(dsmail, "이"));
#endif
                    /* uarm->spe enchantment remains unchanged;
                       re-converting scales to mail poses risk
                       of evaporation due to over enchanting */
                    uarm->otyp += GRAY_DRAGON_SCALES - GRAY_DRAGON_SCALE_MAIL;
                    uarm->dknown = 1;
                    context.botl = 1; /* AC is changing */
                }
                uskin = uarm;
                uarm = (struct obj *) 0;
                /* save/restore hack */
                uskin->owornmask |= I_SPECIAL;
                update_inventory();
            }
        } else if (iswere) {
        do_shift:
            if (Upolyd && were_beastie(mntmp) != u.ulycn)
                mntmp = PM_HUMAN; /* Illegal; force newman() */
            else
                mntmp = u.ulycn;
        } else if (isvamp) {
        do_vampyr:
            if (mntmp < LOW_PM || (mons[mntmp].geno & G_UNIQ))
                mntmp = (youmonst.data != &mons[PM_VAMPIRE] && !rn2(10))
                            ? PM_WOLF
                        : !rn2(4) ? PM_FOG_CLOUD
                                  : PM_VAMPIRE_BAT;
            if (controllable_poly) {
#if 0 /*KR: 원본*/
                Sprintf(buf, "Become %s?", an(mons[mntmp].mname));
#else /*KR: KRNethack 맞춤 번역 */
                Sprintf(buf, "%s(이)가 되시겠습니까?", mons[mntmp].mname);
#endif
                if (yn(buf) != 'y')
                    return;
            }
        }
        /* if polymon fails, "you feel" message has been given
           so don't follow up with another polymon or newman;
           sex_change_ok left disabled here */
        if (mntmp == PM_HUMAN)
            newman(); /* werecritter */
        else
            (void) polymon(mntmp);
        goto made_change; /* maybe not, but this is right anyway */
    }

    if (mntmp < LOW_PM) {
        tryct = 200;
        do {
            /* randomly pick an "ordinary" monster */
            mntmp = rn1(SPECIAL_PM - LOW_PM, LOW_PM);
            if (polyok(&mons[mntmp]) && !is_placeholder(&mons[mntmp]))
                break;
        } while (--tryct > 0);
    }

    /* The below polyok() fails either if everything is genocided, or if
     * we deliberately chose something illegal to force newman().
     */
    sex_change_ok++;
    if (!polyok(&mons[mntmp]) || (!forcecontrol && !rn2(5))
        || your_race(&mons[mntmp])) {
        newman();
    } else {
        (void) polymon(mntmp);
    }
    sex_change_ok--; /* reset */

made_change:
    new_light = emits_light(youmonst.data);
    if (old_light != new_light) {
        if (old_light)
            del_light_source(LS_MONSTER, monst_to_any(&youmonst));
        if (new_light == 1)
            ++new_light; /* otherwise it's undetectable */
        if (new_light)
            new_light_source(u.ux, u.uy, new_light, LS_MONSTER,
                             monst_to_any(&youmonst));
    }
}

/* (try to) make a mntmp monster out of the player;
   returns 1 if polymorph successful */
int
polymon(mntmp)
int mntmp;
{
    char buf[BUFSZ];
    boolean sticky = sticks(youmonst.data) && u.ustuck && !u.uswallow,
            was_blind = !!Blind, dochange = FALSE;
    int mlvl;

    if (mvitals[mntmp].mvflags & G_GENOD) { /* allow G_EXTINCT */
        /*KR You_feel("rather %s-ish.", mons[mntmp].mname); */
        You("다소 %s 같아진 기분이다.", mons[mntmp].mname);
        exercise(A_WIS, TRUE);
        return 0;
    }

    /* KMH, conduct */
    u.uconduct.polyselfs++;

    /* exercise used to be at the very end but only Wis was affected
       there since the polymorph was always in effect by then */
    exercise(A_CON, FALSE);
    exercise(A_WIS, TRUE);

    if (!Upolyd) {
        /* Human to monster; save human stats */
        u.macurr = u.acurr;
        u.mamax = u.amax;
        u.mfemale = flags.female;
    } else {
        /* Monster to monster; restore human stats, to be
         * immediately changed to provide stats for the new monster
         */
        u.acurr = u.macurr;
        u.amax = u.mamax;
        flags.female = u.mfemale;
    }

    /* if stuck mimicking gold, stop immediately */
    if (multi < 0 && U_AP_TYPE == M_AP_OBJECT
        && youmonst.data->mlet != S_MIMIC)
        unmul("");
    /* if becoming a non-mimic, stop mimicking anything */
    if (mons[mntmp].mlet != S_MIMIC) {
        /* as in polyman() */
        youmonst.m_ap_type = M_AP_NOTHING;
        youmonst.mappearance = 0;
    }
    if (is_male(&mons[mntmp])) {
        if (flags.female)
            dochange = TRUE;
    } else if (is_female(&mons[mntmp])) {
        if (!flags.female)
            dochange = TRUE;
    } else if (!is_neuter(&mons[mntmp]) && mntmp != u.ulycn) {
        if (sex_change_ok && !rn2(10))
            dochange = TRUE;
    }

    /*KR Strcpy(buf, (u.umonnum != mntmp) ? "" : "new "); */
    Strcpy(buf, (u.umonnum != mntmp) ? "" : "새로운 ");
    if (dochange) {
        flags.female = !flags.female;
        /*KR Strcat(buf, (is_male(&mons[mntmp]) || is_female(&mons[mntmp]))
                       ? "" : flags.female ? "female " : "male "); */
        Strcat(buf, (is_male(&mons[mntmp]) || is_female(&mons[mntmp])) ? ""
                    : flags.female ? "암컷 "
                                   : "수컷 ");
    }
    Strcat(buf, mons[mntmp].mname);
#if 0 /*KR: 원본*/
    You("%s %s!", (u.umonnum != mntmp) ? "turn into" : "feel like", an(buf));
#else /*KR: KRNethack 맞춤 번역 */
    You("%s %s!", append_josa(buf, "으(로)"),
        (u.umonnum != mntmp) ? "변했다" : "된 기분이다");
#endif

    if (Stoned && poly_when_stoned(&mons[mntmp])) {
        /* poly_when_stoned already checked stone golem genocide */
        mntmp = PM_STONE_GOLEM;
        /*KR make_stoned(0L, "You turn to stone!", 0, (char *) 0); */
        make_stoned(0L, "당신은 돌로 변했다!", 0, (char *) 0);
    }

    u.mtimedone = rn1(500, 500);
    u.umonnum = mntmp;
    set_uasmon();

    /* New stats for monster, to last only as long as polymorphed.
     * Currently only strength gets changed.
     */
    if (strongmonst(&mons[mntmp]))
        ABASE(A_STR) = AMAX(A_STR) = STR18(100);

    if (Stone_resistance && Stoned) { /* parnes@eniac.seas.upenn.edu */
        /*KR make_stoned(0L, "You no longer seem to be petrifying.", 0,
                    (char *) 0); */
        make_stoned(0L, "더 이상 돌로 굳어지지 않는 것 같다.", 0, (char *) 0);
    }
    if (Sick_resistance && Sick) {
        make_sick(0L, (char *) 0, FALSE, SICK_ALL);
        /*KR You("no longer feel sick."); */
        You("더 이상 아프지 않다.");
    }
    if (Slimed) {
        if (flaming(youmonst.data)) {
            /*KR make_slimed(0L, "The slime burns away!"); */
            make_slimed(0L, "슬라임이 불타 없어졌다!");
        } else if (mntmp == PM_GREEN_SLIME) {
            /* do it silently */
            make_slimed(0L, (char *) 0);
        }
    }
    check_strangling(FALSE); /* maybe stop strangling */
    if (nohands(youmonst.data))
        make_glib(0);

    /*
    mlvl = adj_lev(&mons[mntmp]);
     * We can't do the above, since there's no such thing as an
     * "experience level of you as a monster" for a polymorphed character.
     */
    mlvl = (int) mons[mntmp].mlevel;
    if (youmonst.data->mlet == S_DRAGON && mntmp >= PM_GRAY_DRAGON) {
        u.mhmax = In_endgame(&u.uz) ? (8 * mlvl) : (4 * mlvl + d(mlvl, 4));
    } else if (is_golem(youmonst.data)) {
        u.mhmax = golemhp(mntmp);
    } else {
        if (!mlvl)
            u.mhmax = rnd(4);
        else
            u.mhmax = d(mlvl, 8);
        if (is_home_elemental(&mons[mntmp]))
            u.mhmax *= 3;
    }
    u.mh = u.mhmax;

    if (u.ulevel < mlvl) {
        /* Low level characters can't become high level monsters for long */
#ifdef DUMB
        /* DRS/NS 2.2.6 messes up -- Peter Kendell */
        int mtd = u.mtimedone, ulv = u.ulevel;

        u.mtimedone = mtd * ulv / mlvl;
#else
        u.mtimedone = u.mtimedone * u.ulevel / mlvl;
#endif
    }

    if (uskin && mntmp != armor_to_dragon(uskin->otyp))
        skinback(FALSE);
    break_armor();
    drop_weapon(1);
    (void) hideunder(&youmonst);

    if (u.utrap && u.utraptype == TT_PIT) {
        set_utrap(rn1(6, 2), TT_PIT); /* time to escape resets */
    }
    if (was_blind && !Blind) { /* previous form was eyeless */
        Blinded = 1L;
        make_blinded(0L, TRUE); /* remove blindness */
    }
    newsym(u.ux, u.uy); /* Change symbol */

    /* [note:  this 'sticky' handling is only sufficient for changing from
       grabber to engulfer or vice versa because engulfing by poly'd hero
       always ends immediately so won't be in effect during a polymorph] */
    if (!sticky && !u.uswallow && u.ustuck && sticks(youmonst.data))
        u.ustuck = 0;
    else if (sticky && !sticks(youmonst.data))
        uunstick();

    if (u.usteed) {
        if (touch_petrifies(u.usteed->data) && !Stone_resistance && rnl(3)) {
            pline("%s touch %s.", no_longer_petrify_resistant,
                  mon_nam(u.usteed));
            /*KR Sprintf(buf, "riding %s", an(u.usteed->data->mname)); */
            Sprintf(buf, "%s 타다가",
                    append_josa(u.usteed->data->mname, "을"));
            instapetrify(buf);
        }
        if (!can_ride(u.usteed))
            dismount_steed(DISMOUNT_POLY);
    }

    if (flags.verbose) {
#if 0 /*KR: 원본*/
        static const char use_thec[] = "Use the command #%s to %s.";
        static const char monsterc[] = "monster";

        if (can_breathe(youmonst.data))
            pline(use_thec, monsterc, "use your breath weapon");
        if (attacktype(youmonst.data, AT_SPIT))
            pline(use_thec, monsterc, "spit venom");
        if (youmonst.data->mlet == S_NYMPH)
            pline(use_thec, monsterc, "remove an iron ball");
        if (attacktype(youmonst.data, AT_GAZE))
            pline(use_thec, monsterc, "gaze at monsters");
        if (is_hider(youmonst.data))
            pline(use_thec, monsterc, "hide");
        if (is_were(youmonst.data))
            pline(use_thec, monsterc, "summon help");
        if (webmaker(youmonst.data))
            pline(use_thec, monsterc, "spin a web");
        if (u.umonnum == PM_GREMLIN)
            pline(use_thec, monsterc, "multiply in a fountain");
        if (is_unicorn(youmonst.data))
            pline(use_thec, monsterc, "use your horn");
        if (is_mind_flayer(youmonst.data))
            pline(use_thec, monsterc, "emit a mental blast");
        if (youmonst.data->msound == MS_SHRIEK) /* worthless, actually */
            pline(use_thec, monsterc, "shriek");
        if (is_vampire(youmonst.data))
            pline(use_thec, monsterc, "change shape");

        if (lays_eggs(youmonst.data) && flags.female &&
            !(youmonst.data == &mons[PM_GIANT_EEL]
                || youmonst.data == &mons[PM_ELECTRIC_EEL]))
            pline(use_thec, "sit",
                  eggs_in_water(youmonst.data) ?
                      "spawn in the water" : "lay an egg");
#else /*KR: KRNethack 맞춤 번역 */
        static const char use_thec[] = "#%s 명령어를 사용하여 %s할 수 있다.";
        static const char monsterc[] = "monster";

        if (can_breathe(youmonst.data))
            pline(use_thec, monsterc, "브레스 무기를 발사");
        if (attacktype(youmonst.data, AT_SPIT))
            pline(use_thec, monsterc, "독을 뱉어낼");
        if (youmonst.data->mlet == S_NYMPH)
            pline(use_thec, monsterc, "철구를 벗어던질");
        if (attacktype(youmonst.data, AT_GAZE))
            pline(use_thec, monsterc, "괴물을 응시");
        if (is_hider(youmonst.data))
            pline(use_thec, monsterc, "숨을");
        if (is_were(youmonst.data))
            pline(use_thec, monsterc, "도움을 소환");
        if (webmaker(youmonst.data))
            pline(use_thec, monsterc, "거미줄을 칠");
        if (u.umonnum == PM_GREMLIN)
            pline(use_thec, monsterc, "분수에서 분열");
        if (is_unicorn(youmonst.data))
            pline(use_thec, monsterc, "뿔을 사용");
        if (is_mind_flayer(youmonst.data))
            pline(use_thec, monsterc, "정신 분열파를 쏠");
        if (youmonst.data->msound == MS_SHRIEK) /* worthless, actually */
            pline(use_thec, monsterc, "비명을 지를");
        if (is_vampire(youmonst.data))
            pline(use_thec, monsterc, "형태를 변형");

        if (lays_eggs(youmonst.data) && flags.female
            && !(youmonst.data == &mons[PM_GIANT_EEL]
                 || youmonst.data == &mons[PM_ELECTRIC_EEL]))
            pline(use_thec, "sit",
                  eggs_in_water(youmonst.data) ? "수중에 산란" : "알을 낳을");
#endif
    }

    /* you now know what an egg of your type looks like */
    if (lays_eggs(youmonst.data)) {
        learn_egg_type(u.umonnum);
        /* make queen bees recognize killer bee eggs */
        learn_egg_type(egg_type_from_parent(u.umonnum, TRUE));
    }
    find_ac();
    if ((!Levitation && !u.ustuck && !Flying && is_pool_or_lava(u.ux, u.uy))
        || (Underwater && !Swimming))
        spoteffects(TRUE);
    if (Passes_walls && u.utrap
        && (u.utraptype == TT_INFLOOR || u.utraptype == TT_BURIEDBALL)) {
        if (u.utraptype == TT_INFLOOR) {
            /*KR pline_The("rock seems to no longer trap you."); */
            pline("이제 더 이상 바위가 당신을 가두지 못하는 것 같다.");
        } else {
            /*KR pline_The("buried ball is no longer bound to you."); */
            pline("파묻혀 있던 철구는 더 이상 당신을 묶어두지 못한다.");
            buried_ball_to_freedom();
        }
        reset_utrap(TRUE);
    } else if (likes_lava(youmonst.data) && u.utrap
               && u.utraptype == TT_LAVA) {
        /*KR pline_The("%s now feels soothing.", hliquid("lava")); */
        pline("이제 %s 부드럽게 느껴진다.",
              append_josa(hliquid("용암"), "이"));
        reset_utrap(TRUE);
    }
    if (amorphous(youmonst.data) || is_whirly(youmonst.data)
        || unsolid(youmonst.data)) {
        if (Punished) {
            /*KR You("slip out of the iron chain."); */
            You("쇠사슬에서 빠져나왔다.");
            unpunish();
        } else if (u.utrap && u.utraptype == TT_BURIEDBALL) {
            /*KR You("slip free of the buried ball and chain."); */
            You("파묻힌 철구와 쇠사슬에서 무사히 빠져나왔다.");
            buried_ball_to_freedom();
        }
    }
    if (u.utrap && (u.utraptype == TT_WEB || u.utraptype == TT_BEARTRAP)
        && (amorphous(youmonst.data) || is_whirly(youmonst.data)
            || unsolid(youmonst.data)
            || (youmonst.data->msize <= MZ_SMALL
                && u.utraptype == TT_BEARTRAP))) {
        /*KR You("are no longer stuck in the %s.",
            u.utraptype == TT_WEB ? "web" : "bear trap"); */
        You("더 이상 %s 갇혀 있지 않다.",
            append_josa(u.utraptype == TT_WEB ? "거미줄" : "곰 덫", "에"));
        /* probably should burn webs too if PM_FIRE_ELEMENTAL */
        reset_utrap(TRUE);
    }
    if (webmaker(youmonst.data) && u.utrap && u.utraptype == TT_WEB) {
        /*KR You("orient yourself on the web."); */
        You("거미줄 위에서 균형을 잡았다.");
        reset_utrap(TRUE);
    }
    check_strangling(TRUE); /* maybe start strangling */

    context.botl = 1;
    vision_full_recalc = 1;
    see_monsters();
    (void) encumber_msg();

    retouch_equipment(2);
    /* this might trigger a recursive call to polymon() [stone golem
       wielding cockatrice corpse and hit by stone-to-flesh, becomes
       flesh golem above, now gets transformed back into stone golem] */
    if (!uarmg)
        selftouch(no_longer_petrify_resistant);
    return 1;
}

/* dropx() jacket for break_armor() */
STATIC_OVL void dropp(obj) struct obj *obj;
{
    struct obj *otmp;

    /*
     * Dropping worn armor while polymorphing might put hero into water
     * (loss of levitation boots or water walking boots that the new
     * form can't wear), where emergency_disrobe() could remove it from
     * inventory.  Without this, dropx() could trigger an 'object lost'
     * panic.  Right now, boots are the only armor which might encounter
     * this situation, but handle it for all armor.
     *
     * Hypothetically, 'obj' could have merged with something (not
     * applicable for armor) and no longer be a valid pointer, so scan
     * inventory for it instead of trusting obj->where.
     */
    for (otmp = invent; otmp; otmp = otmp->nobj) {
        if (otmp == obj) {
            dropx(obj);
            break;
        }
    }
}

STATIC_OVL void
break_armor()
{
    register struct obj *otmp;

    if (breakarm(youmonst.data)) {
        if ((otmp = uarm) != 0) {
            if (donning(otmp))
                cancel_don();
            /*KR You("break out of your armor!"); */
            You("갑옷을 뚫고 튀어나왔다!");
            exercise(A_STR, FALSE);
            (void) Armor_gone();
            useup(otmp);
        }
        if ((otmp = uarmc) != 0) {
            if (otmp->oartifact) {
                /*KR Your("%s falls off!", cloak_simple_name(otmp)); */
                Your("%s 떨어져 나갔다!",
                     append_josa(cloak_simple_name(otmp), "이"));
                (void) Cloak_off();
                dropp(otmp);
            } else {
                /*KR Your("%s tears apart!", cloak_simple_name(otmp)); */
                Your("%s 찢어져버렸다!",
                     append_josa(cloak_simple_name(otmp), "이"));
                (void) Cloak_off();
                useup(otmp);
            }
        }
        if (uarmu) {
            /*KR Your("shirt rips to shreds!"); */
            Your("셔츠가 산산조각이 나버렸다!");
            useup(uarmu);
        }
    } else if (sliparm(youmonst.data)) {
        if (((otmp = uarm) != 0) && (racial_exception(&youmonst, otmp) < 1)) {
            if (donning(otmp))
                cancel_don();
            /*KR Your("armor falls around you!"); */
            Your("갑옷이 흘러내렸다!");
            (void) Armor_gone();
            dropp(otmp);
        }
        if ((otmp = uarmc) != 0) {
            if (is_whirly(youmonst.data))
                /*KR Your("%s falls, unsupported!", cloak_simple_name(otmp));
                 */
                Your("%s 지탱을 잃고 떨어져 나갔다!",
                     append_josa(cloak_simple_name(otmp), "이"));
            else
                /*KR You("shrink out of your %s!", cloak_simple_name(otmp));
                 */
                You("몸이 줄어들어 %s 빠져나왔다!",
                    append_josa(cloak_simple_name(otmp), "에서"));
            (void) Cloak_off();
            dropp(otmp);
        }
        if ((otmp = uarmu) != 0) {
            if (is_whirly(youmonst.data))
                /*KR You("seep right through your shirt!"); */
                You("셔츠를 그대로 통과해 흘러나왔다!");
            else
                /*KR You("become much too small for your shirt!"); */
                You("셔츠를 입기엔 몸이 너무 작아졌다!");
            setworn((struct obj *) 0, otmp->owornmask & W_ARMU);
            dropp(otmp);
        }
    }
    if (has_horns(youmonst.data)) {
        if ((otmp = uarmh) != 0) {
            if (is_flimsy(otmp) && !donning(otmp)) {
                char hornbuf[BUFSZ];

                /* Future possibilities: This could damage/destroy helmet */
                /*KR Sprintf(hornbuf, "horn%s",
                 * plur(num_horns(youmonst.data))); */
                Sprintf(hornbuf, "뿔");
#if 0 /*KR: 원본*/
                Your("%s %s through %s.", hornbuf, vtense(hornbuf, "pierce"),
                     yname(otmp));
#else /*KR: KRNethack 맞춤 번역 */
                Your("%s %s 꿰뚫고 튀어나왔다.", append_josa(hornbuf, "이"),
                     yname(otmp));
#endif
            } else {
                if (donning(otmp))
                    cancel_don();
                /*KR Your("%s falls to the %s!", helm_simple_name(otmp),
                     surface(u.ux, u.uy)); */
                Your("%s %s 떨어졌다!",
                     append_josa(helm_simple_name(otmp), "이"),
                     append_josa(surface(u.ux, u.uy), "로"));
                (void) Helmet_off();
                dropp(otmp);
            }
        }
    }
    if (nohands(youmonst.data) || verysmall(youmonst.data)) {
        if ((otmp = uarmg) != 0) {
            if (donning(otmp))
                cancel_don();
            /* Drop weapon along with gloves */
            /*KR You("drop your gloves%s!", uwep ? " and weapon" : ""); */
            You("장갑%s 떨어뜨렸다!", uwep ? "과 무기를" : "을");
            drop_weapon(0);
            (void) Gloves_off();
            /* Glib manipulation (ends immediately) handled by Gloves_off */
            dropp(otmp);
        }
        if ((otmp = uarms) != 0) {
            /*KR You("can no longer hold your shield!"); */
            You("더 이상 방패를 들 수 없다!");
            (void) Shield_off();
            dropp(otmp);
        }
        if ((otmp = uarmh) != 0) {
            if (donning(otmp))
                cancel_don();
            /*KR Your("%s falls to the %s!", helm_simple_name(otmp),
                 surface(u.ux, u.uy)); */
            Your("%s %s 떨어졌다!", append_josa(helm_simple_name(otmp), "이"),
                 append_josa(surface(u.ux, u.uy), "로"));
            (void) Helmet_off();
            dropp(otmp);
        }
    }
    if (nohands(youmonst.data) || verysmall(youmonst.data)
        || slithy(youmonst.data) || youmonst.data->mlet == S_CENTAUR) {
        if ((otmp = uarmf) != 0) {
            if (donning(otmp))
                cancel_don();
            if (is_whirly(youmonst.data))
                /*KR Your("boots fall away!"); */
                Your("신발이 떨어져 나갔다!");
            else
                /*KR Your("boots %s off your feet!",
                     verysmall(youmonst.data) ? "slide" : "are pushed"); */
                Your("신발이 발에서 %s!", verysmall(youmonst.data)
                                              ? "미끄러져 빠졌다"
                                              : "밀려났다");
            (void) Boots_off();
            dropp(otmp);
        }
    }
}

STATIC_OVL void drop_weapon(alone) int alone;
{
    struct obj *otmp;
    const char *what, *which, *whichtoo;
    boolean candropwep, candropswapwep, updateinv = TRUE;

    if (uwep) {
        /* !alone check below is currently superfluous but in the
         * future it might not be so if there are monsters which cannot
         * wear gloves but can wield weapons
         */
        if (!alone || cantwield(youmonst.data)) {
            candropwep = canletgo(uwep, "");
            candropswapwep = !u.twoweap || canletgo(uswapwep, "");
            if (alone) {
                what =
                    (candropwep && candropswapwep) ? "놓아버려야" : "놓아야";
                which = is_sword(uwep) ? "검" : weapon_descr(uwep);
                if (u.twoweap) {
                    whichtoo =
                        is_sword(uswapwep) ? "검" : weapon_descr(uswapwep);
                    if (strcmp(which, whichtoo))
                        which = "무기";
                }
#if 0 /*KR: 원본*/
                if (uwep->quan != 1L || u.twoweap)
                    which = makeplural(which);

                You("find you must %s %s %s!", what,
                    the_your[!!strncmp(which, "corpse", 6)], which);
#else /*KR: KRNethack 맞춤 번역 */
                You("당신의 %s %s 한다는 걸 깨달았다!",
                    append_josa(which, "을"), what);
#endif
            }
            /* if either uwep or wielded uswapwep is flagged as 'in_use'
               then don't drop it or explicitly update inventory; leave
               those actions to caller (or caller's caller, &c) */
            if (u.twoweap) {
                otmp = uswapwep;
                uswapwepgone();
                if (otmp->in_use)
                    updateinv = FALSE;
                else if (candropswapwep)
                    dropx(otmp);
            }
            otmp = uwep;
            uwepgone();
            if (otmp->in_use)
                updateinv = FALSE;
            else if (candropwep)
                dropx(otmp);
            /* [note: dropp vs dropx -- if heart of ahriman is wielded, we
               might be losing levitation by dropping it; but that won't
               happen until the drop, unlike Boots_off() dumping hero into
               water and triggering emergency_disrobe() before dropx()] */

            if (updateinv)
                update_inventory();
        } else if (!could_twoweap(youmonst.data)) {
            untwoweapon();
        }
    }
}

void
rehumanize()
{
    boolean was_flying = (Flying != 0);

    /* You can't revert back while unchanging */
    if (Unchanging) {
        if (u.mh < 1) {
            killer.format = NO_KILLER_PREFIX;
            /*KR Strcpy(killer.name, "killed while stuck in creature form");
             */
            Strcpy(killer.name, "괴물 형태에서 갇혀 죽음");
            done(DIED);
        } else if (uamul && uamul->otyp == AMULET_OF_UNCHANGING) {
#if 0 /*KR: 원본*/
            Your("%s %s!", simpleonames(uamul), otense(uamul, "fail"));
#else /*KR: KRNethack 맞춤 번역 */
            Your("%s 효과를 발휘하지 못했다!",
                 append_josa(simpleonames(uamul), "이"));
#endif
            uamul->dknown = 1;
            makeknown(AMULET_OF_UNCHANGING);
        }
    }

    if (emits_light(youmonst.data))
        del_light_source(LS_MONSTER, monst_to_any(&youmonst));
#if 0 /*KR: 원본*/
    polyman("return to %s form!", urace.adj);
#else /*KR: KRNethack 맞춤 번역 */
    polyman("%s의 모습으로 되돌아왔다!", urace.adj);
#endif

    if (u.uhp < 1) {
        /* can only happen if some bit of code reduces u.uhp
           instead of u.mh while poly'd */
        /*KR Your("old form was not healthy enough to survive."); */
        Your("본래의 모습은 살아남을 수 있을 만큼 건강하지 않았다.");
        /*KR Sprintf(killer.name, "reverting to unhealthy %s form",
         * urace.adj); */
        Sprintf(killer.name, "건강하지 못한 %s의 모습으로 돌아온 것",
                urace.adj);
        killer.format = KILLED_BY;
        done(DIED);
    }
    nomul(0);

    context.botl = 1;
    vision_full_recalc = 1;
    (void) encumber_msg();
    if (was_flying && !Flying && u.usteed)
#if 0 /*KR: 원본*/
        You("and %s return gently to the %s.",
            mon_nam(u.usteed), surface(u.ux, u.uy));
#else /*KR: KRNethack 맞춤 번역 */
        You("%s 함께 %s 위로 사뿐히 내려앉았다.",
            append_josa(mon_nam(u.usteed), "와(과)"), surface(u.ux, u.uy));
#endif
    retouch_equipment(2);
    if (!uarmg)
        selftouch(no_longer_petrify_resistant);
}

int
dobreathe()
{
    struct attack *mattk;

    if (Strangled) {
        /*KR You_cant("breathe.  Sorry."); */
        You_cant("숨을 쉴 수 없다. 유감이다.");
        return 0;
    }
    if (u.uen < 15) {
        /*KR You("don't have enough energy to breathe!"); */
        You("브레스를 쏠 만한 마력이 부족하다!");
        return 0;
    }
    u.uen -= 15;
    context.botl = 1;

    if (!getdir((char *) 0))
        return 0;

    mattk = attacktype_fordmg(youmonst.data, AT_BREA, AD_ANY);
    if (!mattk)
        impossible("bad breath attack?"); /* mouthwash needed... */
    else if (!u.dx && !u.dy && !u.dz)
        ubreatheu(mattk);
    else
        buzz((int) (20 + mattk->adtyp - 1), (int) mattk->damn, u.ux, u.uy,
             u.dx, u.dy);
    return 1;
}

int
dospit()
{
    struct obj *otmp;
    struct attack *mattk;

    if (!getdir((char *) 0))
        return 0;
    mattk = attacktype_fordmg(youmonst.data, AT_SPIT, AD_ANY);
    if (!mattk) {
        impossible("bad spit attack?");
    } else {
        switch (mattk->adtyp) {
        case AD_BLND:
        case AD_DRST:
            otmp = mksobj(BLINDING_VENOM, TRUE, FALSE);
            break;
        default:
            impossible("bad attack type in dospit");
            /*FALLTHRU*/
        case AD_ACID:
            otmp = mksobj(ACID_VENOM, TRUE, FALSE);
            break;
        }
        otmp->spe = 1; /* to indicate it's yours */
        throwit(otmp, 0L, FALSE);
    }
    return 1;
}

int
doremove()
{
    if (!Punished) {
        if (u.utrap && u.utraptype == TT_BURIEDBALL) {
            /*KR pline_The("ball and chain are buried firmly in the %s.",
                      surface(u.ux, u.uy)); */
            pline("철구와 쇠사슬이 %s 단단히 묻혀 있다.",
                  append_josa(surface(u.ux, u.uy), "에"));
            return 0;
        }
        /*KR You("are not chained to anything!"); */
        You("아무것에도 묶여있지 않다!");
        return 0;
    }
    unpunish();
    return 1;
}

int
dospinweb()
{
    register struct trap *ttmp = t_at(u.ux, u.uy);

    if (Levitation || Is_airlevel(&u.uz) || Underwater
        || Is_waterlevel(&u.uz)) {
        /*KR You("must be on the ground to spin a web."); */
        You("거미줄을 치려면 땅에 있어야만 한다.");
        return 0;
    }
    if (u.uswallow) {
        /*KR You("release web fluid inside %s.", mon_nam(u.ustuck)); */
        You("%s의 내부에서 거미줄 액을 내뿜었다.", mon_nam(u.ustuck));
        if (is_animal(u.ustuck->data)) {
            expels(u.ustuck, u.ustuck->data, TRUE);
            return 0;
        }
        if (is_whirly(u.ustuck->data)) {
            int i;

            for (i = 0; i < NATTK; i++)
                if (u.ustuck->data->mattk[i].aatyp == AT_ENGL)
                    break;
            if (i == NATTK)
                impossible("Swallower has no engulfing attack?");
            else {
                char sweep[30];

                sweep[0] = '\0';
                switch (u.ustuck->data->mattk[i].adtyp) {
                case AD_FIRE:
                    /*KR Strcpy(sweep, "ignites and "); */
                    Strcpy(sweep, "발화되어 ");
                    break;
                case AD_ELEC:
                    /*KR Strcpy(sweep, "fries and "); */
                    Strcpy(sweep, "타버려서 ");
                    break;
                case AD_COLD:
                    /*KR Strcpy(sweep, "freezes, shatters and "); */
                    Strcpy(sweep, "얼어붙어 부서지며 ");
                    break;
                }
#if 0 /*KR: 원본*/
                pline_The("web %sis swept away!", sweep);
#else /*KR: KRNethack 맞춤 번역 */
                pline("거미줄은 %s쓸려나가 버렸다!", sweep);
#endif
            }
            return 0;
        } /* default: a nasty jelly-like creature */
        /*KR pline_The("web dissolves into %s.", mon_nam(u.ustuck)); */
        pline("거미줄은 %s 속으로 녹아버렸다.", mon_nam(u.ustuck));
        return 0;
    }
    if (u.utrap) {
        /*KR You("cannot spin webs while stuck in a trap."); */
        You("함정에 걸린 상태로는 거미줄을 칠 수 없다.");
        return 0;
    }
    exercise(A_DEX, TRUE);
    if (ttmp) {
        switch (ttmp->ttyp) {
        case PIT:
        case SPIKED_PIT:
            /*KR You("spin a web, covering up the pit."); */
            You("거미줄을 쳐서, 구덩이를 덮었다.");
            deltrap(ttmp);
            bury_objs(u.ux, u.uy);
            newsym(u.ux, u.uy);
            return 1;
        case SQKY_BOARD:
            /*KR pline_The("squeaky board is muffled."); */
            pline("삐걱거리는 널빤지 소리가 잦아들었다.");
            deltrap(ttmp);
            newsym(u.ux, u.uy);
            return 1;
        case TELEP_TRAP:
        case LEVEL_TELEP:
        case MAGIC_PORTAL:
        case VIBRATING_SQUARE:
            /*KR Your("webbing vanishes!"); */
            Your("거미줄이 사라졌다!");
            return 0;
        case WEB:
            /*KR You("make the web thicker."); */
            You("거미줄을 더 두껍게 만들었다.");
            return 1;
        case HOLE:
        case TRAPDOOR:
#if 0 /*KR: 원본*/
            You("web over the %s.",
                (ttmp->ttyp == TRAPDOOR) ? "trap door" : "hole");
#else /*KR: KRNethack 맞춤 번역 */
            You("%s 위에 거미줄을 쳤다.",
                (ttmp->ttyp == TRAPDOOR) ? "트랩도어" : "구멍");
#endif
            deltrap(ttmp);
            newsym(u.ux, u.uy);
            return 1;
        case ROLLING_BOULDER_TRAP:
            /*KR You("spin a web, jamming the trigger."); */
            You("거미줄을 쳐서, 스위치를 작동하지 않게 막았다.");
            deltrap(ttmp);
            newsym(u.ux, u.uy);
            return 1;
        case ARROW_TRAP:
        case DART_TRAP:
        case BEAR_TRAP:
        case ROCKTRAP:
        case FIRE_TRAP:
        case LANDMINE:
        case SLP_GAS_TRAP:
        case RUST_TRAP:
        case MAGIC_TRAP:
        case ANTI_MAGIC:
        case POLY_TRAP:
            /*KR You("have triggered a trap!"); */
            You("함정을 건드리고 말았다!");
            dotrap(ttmp, 0);
            return 1;
        default:
            impossible("Webbing over trap type %d?", ttmp->ttyp);
            return 0;
        }
    } else if (On_stairs(u.ux, u.uy)) {
        /* cop out: don't let them hide the stairs */
        /*KR Your("web fails to impede access to the %s.",
             (levl[u.ux][u.uy].typ == STAIRS) ? "stairs" : "ladder"); */
        Your("거미줄은 %s로 가는 통로를 가로막는 데 실패했다.",
             (levl[u.ux][u.uy].typ == STAIRS) ? "계단" : "사다리");
        return 1;
    }
    ttmp = maketrap(u.ux, u.uy, WEB);
    if (ttmp) {
        ttmp->madeby_u = 1;
        feeltrap(ttmp);
    }
    return 1;
}

int
dosummon()
{
    int placeholder;
    if (u.uen < 10) {
        /*KR You("lack the energy to send forth a call for help!"); */
        You("지원을 부를 만한 마력이 부족하다!");
        return 0;
    }
    u.uen -= 10;
    context.botl = 1;

    /*KR You("call upon your brethren for help!"); */
    You("동족들에게 도움을 요청했다!");
    exercise(A_WIS, TRUE);
    if (!were_summon(youmonst.data, TRUE, &placeholder, (char *) 0))
        /*KR pline("But none arrive."); */
        pline("하지만 아무도 오지 않았다.");
    return 1;
}

int
dogaze()
{
    register struct monst *mtmp;
    int looked = 0;
    char qbuf[QBUFSZ];
    int i;
    uchar adtyp = 0;

    for (i = 0; i < NATTK; i++) {
        if (youmonst.data->mattk[i].aatyp == AT_GAZE) {
            adtyp = youmonst.data->mattk[i].adtyp;
            break;
        }
    }
    if (adtyp != AD_CONF && adtyp != AD_FIRE) {
        impossible("gaze attack %d?", adtyp);
        return 0;
    }

    if (Blind) {
        /*KR You_cant("see anything to gaze at."); */
        You_cant("응시할 대상을 전혀 볼 수 없다.");
        return 0;
    } else if (Hallucination) {
        /*KR You_cant("gaze at anything you can see."); */
        You_cant("보이는 것에 제대로 집중해서 응시할 수 없다.");
        return 0;
    }
    if (u.uen < 15) {
        /*KR You("lack the energy to use your special gaze!"); */
        You("특수 응시 능력을 사용하기엔 마력이 부족하다!");
        return 0;
    }
    u.uen -= 15;
    context.botl = 1;

    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
        if (DEADMONSTER(mtmp))
            continue;
        if (canseemon(mtmp) && couldsee(mtmp->mx, mtmp->my)) {
            looked++;
            if (Invis && !perceives(mtmp->data)) {
                /*KR pline("%s seems not to notice your gaze.", Monnam(mtmp));
                 */
                pline("%s 당신의 시선을 알아차리지 못한 것 같다.",
                      append_josa(Monnam(mtmp), "은"));
            } else if (mtmp->minvis && !See_invisible) {
                /*KR You_cant("see where to gaze at %s.", Monnam(mtmp)); */
                You_cant("%s 응시해야 할지 보이지 않는다.",
                         append_josa(Monnam(mtmp), "의 어디를"));
            } else if (M_AP_TYPE(mtmp) == M_AP_FURNITURE
                       || M_AP_TYPE(mtmp) == M_AP_OBJECT) {
                looked--;
                continue;
            } else if (flags.safe_dog && mtmp->mtame && !Confusion) {
                /*KR You("avoid gazing at %s.", y_monnam(mtmp)); */
                You("%s 응시하는 것을 피했다.",
                    append_josa(y_monnam(mtmp), "을"));
            } else {
                if (flags.confirm && mtmp->mpeaceful && !Confusion) {
#if 0 /*KR: 원본*/
                    Sprintf(qbuf, "Really %s %s?",
                            (adtyp == AD_CONF) ? "confuse" : "attack",
                            mon_nam(mtmp));
#else /*KR: KRNethack 맞춤 번역 */
                    Sprintf(qbuf, "정말로 %s %s?",
                            append_josa(mon_nam(mtmp), "을"),
                            (adtyp == AD_CONF) ? "혼란시키겠습니까"
                                               : "공격하시겠습니까");
#endif
                    if (yn(qbuf) != 'y')
                        continue;
                }
                setmangry(mtmp, TRUE);
                if (!mtmp->mcanmove || mtmp->mstun || mtmp->msleeping
                    || !mtmp->mcansee || !haseyes(mtmp->data)) {
                    looked--;
                    continue;
                }
                /* No reflection check for consistency with when a monster
                 * gazes at *you*--only medusa gaze gets reflected then.
                 */
                if (adtyp == AD_CONF) {
                    if (!mtmp->mconf)
                        /*KR Your("gaze confuses %s!", mon_nam(mtmp)); */
                        Your("시선이 %s 혼란스럽게 했다!",
                             append_josa(mon_nam(mtmp), "을"));
                    else
                        /*KR pline("%s is getting more and more confused.",
                              Monnam(mtmp)); */
                        pline("%s 점점 더 혼란스러워하고 있다.",
                              append_josa(Monnam(mtmp), "은"));
                    mtmp->mconf = 1;
                } else if (adtyp == AD_FIRE) {
                    int dmg = d(2, 6), lev = (int) u.ulevel;

                    /*KR You("attack %s with a fiery gaze!", mon_nam(mtmp));
                     */
                    You("불타는 시선으로 %s 공격했다!",
                        append_josa(mon_nam(mtmp), "을"));
                    if (resists_fire(mtmp)) {
                        /*KR pline_The("fire doesn't burn %s!",
                         * mon_nam(mtmp)); */
                        pline("불은 %s 태우지 못했다!",
                              append_josa(mon_nam(mtmp), "을"));
                        dmg = 0;
                    }
                    if (lev > rn2(20))
                        (void) destroy_mitem(mtmp, SCROLL_CLASS, AD_FIRE);
                    if (lev > rn2(20))
                        (void) destroy_mitem(mtmp, POTION_CLASS, AD_FIRE);
                    if (lev > rn2(25))
                        (void) destroy_mitem(mtmp, SPBOOK_CLASS, AD_FIRE);
                    if (dmg)
                        mtmp->mhp -= dmg;
                    if (DEADMONSTER(mtmp))
                        killed(mtmp);
                }
                /* For consistency with passive() in uhitm.c, this only
                 * affects you if the monster is still alive.
                 */
                if (DEADMONSTER(mtmp))
                    continue;

                if (mtmp->data == &mons[PM_FLOATING_EYE] && !mtmp->mcan) {
                    if (!Free_action) {
#if 0 /*KR: 원본*/
                        You("are frozen by %s gaze!",
                            s_suffix(mon_nam(mtmp)));
#else /*KR: KRNethack 맞춤 번역 */
                        You("%s 시선에 얼어붙었다!",
                            append_josa(mon_nam(mtmp), "의"));
#endif
                        nomul((u.ulevel > 6 || rn2(4))
                                  ? -d((int) mtmp->m_lev + 1,
                                       (int) mtmp->data->mattk[0].damd)
                                  : -200);
                        /*KR multi_reason = "frozen by a monster's gaze"; */
                        multi_reason = "괴물의 시선에 얼어붙은 동안에";
                        nomovemsg = 0;
                        return 1;
                    } else
                        /*KR You("stiffen momentarily under %s gaze.",
                            s_suffix(mon_nam(mtmp))); */
                        You("%s 시선을 받고 순간적으로 굳어졌다.",
                            append_josa(mon_nam(mtmp), "의"));
                }
                /* Technically this one shouldn't affect you at all because
                 * the Medusa gaze is an active monster attack that only
                 * works on the monster's turn, but for it to *not* have an
                 * effect would be too weird.
                 */
                if (mtmp->data == &mons[PM_MEDUSA] && !mtmp->mcan) {
                    /*KR pline("Gazing at the awake %s is not a very good
                       idea.", l_monnam(mtmp)); */
                    pline("깨어있는 %s 응시하는 건 별로 좋은 생각이 아니다.",
                          append_josa(l_monnam(mtmp), "을"));
                    /* as if gazing at a sleeping anything is fruitful... */
                    /*KR You("turn to stone..."); */
                    You("돌로 변했다...");
                    killer.format = KILLED_BY;
                    /*KR Strcpy(killer.name, "deliberately meeting Medusa's
                     * gaze"); */
                    Strcpy(killer.name, "메두사의 시선과 고의로 마주침");
                    done(STONING);
                }
            }
        }
    }
    if (!looked)
        /*KR You("gaze at no place in particular."); */
        You("특별한 곳을 응시하지 않았다.");
    return 1;
}

int
dohide()
{
    boolean ismimic = youmonst.data->mlet == S_MIMIC,
            on_ceiling = is_clinger(youmonst.data) || Flying;

    /* can't hide while being held (or holding) or while trapped
       (except for floor hiders [trapper or mimic] in pits) */
    if (u.ustuck || (u.utrap && (u.utraptype != TT_PIT || on_ceiling))) {
#if 0 /*KR: 원본*/
        You_cant("hide while you're %s.",
                 !u.ustuck ? "trapped"
                   : u.uswallow ? (is_animal(u.ustuck->data) ? "swallowed"
                                                             : "engulfed")
                     : !sticks(youmonst.data) ? "being held"
                       : (humanoid(u.ustuck->data) ? "holding someone"
                                                   : "holding that creature"));
#else /*KR: KRNethack 맞춤 번역 */
        You_cant("%s 있는 동안에는 숨을 수 없다.",
                 !u.ustuck ? "함정에 빠져"
                 : u.uswallow
                     ? (is_animal(u.ustuck->data) ? "삼켜져" : "휩싸여")
                 : !sticks(youmonst.data)
                     ? "붙잡혀"
                     : (humanoid(u.ustuck->data) ? "누군가를 붙잡고"
                                                 : "그 생물을 붙잡고"));
#endif
        if (u.uundetected || (ismimic && U_AP_TYPE != M_AP_NOTHING)) {
            u.uundetected = 0;
            youmonst.m_ap_type = M_AP_NOTHING;
            newsym(u.ux, u.uy);
        }
        return 0;
    }
    /* note: the eel and hides_under cases are hypothetical;
       such critters aren't offered the option of hiding via #monster */
    if (youmonst.data->mlet == S_EEL && !is_pool(u.ux, u.uy)) {
        if (IS_FOUNTAIN(levl[u.ux][u.uy].typ))
            /*KR The("fountain is not deep enough to hide in."); */
            pline("이 분수는 몸을 숨기기에 충분히 깊지 않다.");
        else
            /*KR There("is no %s to hide in here.", hliquid("water")); */
            pline("이곳에는 몸을 숨길 %s 없다.",
                  append_josa(hliquid("물"), "이"));
        u.uundetected = 0;
        return 0;
    }
    if (hides_under(youmonst.data) && !level.objects[u.ux][u.uy]) {
        /*KR There("is nothing to hide under here."); */
        pline("이곳에는 아래에 숨을 만한 것이 아무것도 없다.");
        u.uundetected = 0;
        return 0;
    }
    /* Planes of Air and Water */
    if (on_ceiling && !has_ceiling(&u.uz)) {
        /*KR There("is nowhere to hide above you."); */
        pline("머리 위쪽에는 몸을 숨길 곳이 전혀 없다.");
        u.uundetected = 0;
        return 0;
    }
    if ((is_hider(youmonst.data) && !Flying) /* floor hider */
        && (Is_airlevel(&u.uz) || Is_waterlevel(&u.uz))) {
        /*KR There("is nowhere to hide beneath you."); */
        pline("발아래에는 몸을 숨길 곳이 전혀 없다.");
        u.uundetected = 0;
        return 0;
    }
    /* TODO? inhibit floor hiding at furniture locations, or
     * else make youhiding() give smarter messages at such spots.
     */

    if (u.uundetected || (ismimic && U_AP_TYPE != M_AP_NOTHING)) {
        youhiding(FALSE, 1); /* "you are already hiding" */
        return 0;
    }

    if (ismimic) {
        /* should bring up a dialog "what would you like to imitate?" */
        youmonst.m_ap_type = M_AP_OBJECT;
        youmonst.mappearance = STRANGE_OBJECT;
    } else
        u.uundetected = 1;
    newsym(u.ux, u.uy);
    youhiding(FALSE, 0); /* "you are now hiding" */
    return 1;
}

int
dopoly()
{
    struct permonst *savedat = youmonst.data;

    if (is_vampire(youmonst.data)) {
        polyself(2);
        if (savedat != youmonst.data) {
            /*KR You("transform into %s.", an(youmonst.data->mname)); */
            You("%s 변이했다.", append_josa(youmonst.data->mname, "으로"));
            newsym(u.ux, u.uy);
        }
    }
    return 1;
}

int
domindblast()
{
    struct monst *mtmp, *nmon;

    if (u.uen < 10) {
        /*KR You("concentrate but lack the energy to maintain doing so."); */
        You("집중하려 했으나, 유지할 마력이 부족하다.");
        return 0;
    }
    u.uen -= 10;
    context.botl = 1;

    /*KR You("concentrate."); */
    You("집중했다.");
    /*KR pline("A wave of psychic energy pours out."); */
    pline("정신 에너지의 파동이 뿜어져 나왔다.");
    for (mtmp = fmon; mtmp; mtmp = nmon) {
        int u_sen;

        nmon = mtmp->nmon;
        if (DEADMONSTER(mtmp))
            continue;
        if (distu(mtmp->mx, mtmp->my) > BOLT_LIM * BOLT_LIM)
            continue;
        if (mtmp->mpeaceful)
            continue;
        u_sen = telepathic(mtmp->data) && !mtmp->mcansee;
        if (u_sen || (telepathic(mtmp->data) && rn2(2)) || !rn2(10)) {
#if 0 /*KR: 원본*/
            You("lock in on %s %s.", s_suffix(mon_nam(mtmp)),
                u_sen ? "telepathy"
                      : telepathic(mtmp->data) ? "latent telepathy" : "mind");
#else /*KR: KRNethack 맞춤 번역 */
            You("%s %s에 정신을 집중했다.", append_josa(mon_nam(mtmp), "의"),
                u_sen                    ? "텔레파시"
                : telepathic(mtmp->data) ? "잠재적 텔레파시"
                                         : "마음");
#endif
            mtmp->mhp -= rnd(15);
            if (DEADMONSTER(mtmp))
                killed(mtmp);
        }
    }
    return 1;
}

void
uunstick()
{
    if (!u.ustuck) {
        impossible("uunstick: no ustuck?");
        return;
    }
    /*KR pline("%s is no longer in your clutches.", Monnam(u.ustuck)); */
    pline("%s 이제 당신의 손아귀에서 벗어났다.",
          append_josa(Monnam(u.ustuck), "은"));
    u.ustuck = 0;
}

void skinback(silently) boolean silently;
{
    if (uskin) {
        if (!silently)
            /*KR Your("skin returns to its original form."); */
            Your("피부가 원래의 형태로 돌아왔다.");
        uarm = uskin;
        uskin = (struct obj *) 0;
        /* undo save/restore hack */
        uarm->owornmask &= ~I_SPECIAL;
    }
}

const char *
mbodypart(mon, part)
struct monst *mon;
int part;
{
#if 0 /*KR: 원본*/
    static NEARDATA const char
        *humanoid_parts[] = { "arm",       "eye",  "face",         "finger",
                              "fingertip", "foot", "hand",         "handed",
                              "head",      "leg",  "light headed", "neck",
                              "spine",     "toe",  "hair",         "blood",
                              "lung",      "nose", "stomach" },
        *jelly_parts[] = { "pseudopod", "dark spot", "front",
                           "pseudopod extension", "pseudopod extremity",
                           "pseudopod root", "grasp", "grasped",
                           "cerebral area", "lower pseudopod", "viscous",
                           "middle", "surface", "pseudopod extremity",
                           "ripples", "juices", "surface", "sensor",
                           "stomach" },
        *animal_parts[] = { "forelimb",  "eye",            "face",
                            "foreclaw",  "claw tip",       "rear claw",
                            "foreclaw",  "clawed",         "head",
                            "rear limb", "light headed",   "neck",
                            "spine",     "rear claw tip",  "fur",
                            "blood",     "lung",           "nose",
                            "stomach" },
        *bird_parts[] = { "wing",     "eye",  "face",         "wing",
                          "wing tip", "foot", "wing",         "winged",
                          "head",     "leg",  "light headed", "neck",
                          "spine",    "toe",  "feathers",     "blood",
                          "lung",     "bill", "stomach" },
        *horse_parts[] = { "foreleg",  "eye",            "face",
                           "forehoof", "hoof tip",       "rear hoof",
                           "forehoof", "hooved",         "head",
                           "rear leg", "light headed",   "neck",
                           "backbone", "rear hoof tip",  "mane",
                           "blood",    "lung",           "nose",
                           "stomach" },
        *sphere_parts[] = { "appendage", "optic nerve", "body", "tentacle",
                            "tentacle tip", "lower appendage", "tentacle",
                            "tentacled", "body", "lower tentacle",
                            "rotational", "equator", "body",
                            "lower tentacle tip", "cilia", "life force",
                            "retina", "olfactory nerve", "interior" },
        *fungus_parts[] = { "mycelium", "visual area", "front",
                            "hypha",    "hypha",       "root",
                            "strand",   "stranded",    "cap area",
                            "rhizome",  "sporulated",  "stalk",
                            "root",     "rhizome tip", "spores",
                            "juices",   "gill",        "gill",
                            "interior" },
        *vortex_parts[] = { "region",        "eye",           "front",
                            "minor current", "minor current", "lower current",
                            "swirl",         "swirled",       "central core",
                            "lower current", "addled",        "center",
                            "currents",      "edge",          "currents",
                            "life force",    "center",        "leading edge",
                            "interior" },
        *snake_parts[] = { "vestigial limb", "eye", "face", "large scale",
                           "large scale tip", "rear region", "scale gap",
                           "scale gapped", "head", "rear region",
                           "light headed", "neck", "length", "rear scale",
                           "scales", "blood", "lung", "forked tongue",
                           "stomach" },
        *worm_parts[] = { "anterior segment", "light sensitive cell",
                          "clitellum", "setae", "setae", "posterior segment",
                          "segment", "segmented", "anterior segment",
                          "posterior", "over stretched", "clitellum",
                          "length", "posterior setae", "setae", "blood",
                          "skin", "prostomium", "stomach" },
        *fish_parts[] = { "fin", "eye", "premaxillary", "pelvic axillary",
                          "pelvic fin", "anal fin", "pectoral fin", "finned",
                          "head", "peduncle", "played out", "gills",
                          "dorsal fin", "caudal fin", "scales", "blood",
                          "gill", "nostril", "stomach" };
#else
    static NEARDATA const char *
        humanoid_parts[] = { "팔",       "눈",   "얼굴", "손가락", "손끝",
                             "발",       "손",   "손의", "머리",   "다리",
                             "어지러운", "목",   "척추", "발가락", "머리카락",
                             "피",       "허파", "코",   "배" },
       *jelly_parts[] = { "위족",        "검은 점",   "앞쪽",
                          "위족 확장부", "위족 끝",   "위족 뿌리",
                          "잡는 곳",     "잡힌",      "뇌 영역",
                          "아래쪽 위족", "점성 있는", "가운데",
                          "표면",        "위족 끝",   "물결무늬",
                          "즙",          "표면",      "감각기",
                          "배" },
       *animal_parts[] = { "앞다리",  "눈",        "얼굴",     "앞발톱",
                           "발톱 끝", "뒷발톱",    "앞발톱",   "발톱이 있는",
                           "머리",    "뒷다리",    "어지러운", "목",
                           "척추",    "뒷발톱 끝", "털",       "피",
                           "허파",    "코",        "배" },
       *bird_parts[] = { "날개",    "눈",     "얼굴",     "날개",
                         "날개 끝", "발",     "날개",     "날개가 있는",
                         "머리",    "다리",   "어지러운", "목",
                         "척추",    "발가락", "깃털",     "피",
                         "허파",    "부리",   "배" },
       *horse_parts[] = { "앞다리",  "눈",        "얼굴",     "앞발굽",
                          "발굽 끝", "뒷발굽",    "앞발굽",   "발굽이 있는",
                          "머리",    "뒷다리",    "어지러운", "목",
                          "등뼈",    "뒷발굽 끝", "갈기",     "피",
                          "허파",    "코",        "배" },
       *sphere_parts[] = { "부속지",    "시신경",       "몸통",
                           "촉수",      "촉수 끝",      "하단 부속지",
                           "촉수",      "촉수가 있는",  "몸통",
                           "하단 촉수", "회전하는",     "적도",
                           "몸통",      "하단 촉수 끝", "섬모",
                           "생명력",    "망막",         "후각 신경",
                           "내부" },
       *fungus_parts[] = { "균사체",   "시각 영역",   "앞쪽",
                           "균사",     "균사",        "뿌리",
                           "가닥",     "가닥이 있는", "갓 부위",
                           "뿌리줄기", "포자 형성",   "줄기",
                           "뿌리",     "뿌리줄기 끝", "포자",
                           "수액",     "주름",        "주름", 
                           "내부" },
       *vortex_parts[] = { "구역",        "눈",           "앞쪽",
                           "작은 흐름",   "작은 흐름",    "아래쪽 흐름",
                           "소용돌이",    "소용돌이치는", "중심핵",
                           "아래쪽 흐름", "혼란스러운",   "중심",
                           "기류",        "가장자리",     "기류",
                           "생명력",      "중심",         "앞쪽 가장자리",
                           "내부" },
       *snake_parts[] = { "퇴화된 사지", "눈", 
                          "얼굴",        "큰 비늘",
                          "큰 비늘 끝",  "뒤쪽 부위",
                          "비늘 틈",     "비늘 틈이 있는",
                          "머리",        "뒤쪽 부위",
                          "어지러운",    "목",
                          "몸길이",      "뒷 비늘",
                          "비늘",        "피",
                          "허파",        "갈라진 혀", "배" },
       *worm_parts[] = { "앞쪽 체절", "광수용 세포", "환대", "강모", "강모",
                         "뒤쪽 체절", "체절", "체절로 된", "앞쪽 체절", "뒤쪽",
                         "지나치게 늘어난", "환대", "몸길이", "뒤쪽 강모", 
                         "강모", "피", "피부", "구전엽", "배" },
       *fish_parts[] = { "지느러미", "눈", "전상악골", "배지느러미 기부",
                         "배지느러미", "뒷지느러미", "가슴지느러미",
                         "지느러미가 있는", "머리", "꼬리자루", "지친",
                         "아가미", "등지느러미", "꼬리지느러미", "비늘",
                         "피", "아가미", "콧구멍", "배" };
#endif
    /* claw attacks are overloaded in mons[]; most humanoids with
       such attacks should still reference hands rather than claws */
    static const char not_claws[] = {
        S_HUMAN,     S_MUMMY,   S_ZOMBIE, S_ANGEL, S_NYMPH, S_LEPRECHAUN,
        S_QUANTMECH, S_VAMPIRE, S_ORC,    S_GIANT, /* quest nemeses */
        '\0' /* string terminator; assert( S_xxx != 0 ); */
    };
    struct permonst *mptr = mon->data;

    /* some special cases */
    if (mptr->mlet == S_DOG || mptr->mlet == S_FELINE
        || mptr->mlet == S_RODENT || mptr == &mons[PM_OWLBEAR]) {
        switch (part) {
        case HAND:
            /*KR return "paw"; */
            return "발";
        case HANDED:
            /*KR return "pawed"; */
            return "발이 달린";
        case FOOT:
            /*KR return "rear paw"; */
            return "뒷발";
        case ARM:
        case LEG:
            return horse_parts[part]; /* "foreleg", "rear leg" */
        default:
            break; /* for other parts, use animal_parts[] below */
        }
    } else if (mptr->mlet == S_YETI) { /* excl. owlbear due to 'if' above */
        /* opposable thumbs, hence "hands", "arms", "legs", &c */
        return humanoid_parts[part]; /* yeti/sasquatch, monkey/ape */
    }
    if ((part == HAND || part == HANDED)
        && (humanoid(mptr) && attacktype(mptr, AT_CLAW)
            && !index(not_claws, mptr->mlet) && mptr != &mons[PM_STONE_GOLEM]
            && mptr != &mons[PM_INCUBUS] && mptr != &mons[PM_SUCCUBUS]))
        /*KR return (part == HAND) ? "claw" : "clawed"; */
        return (part == HAND) ? "발톱" : "발톱이 달린";
    if ((mptr == &mons[PM_MUMAK] || mptr == &mons[PM_MASTODON])
        && part == NOSE)
        /*KR return "trunk"; */
        return "코끼리 코";
    if (mptr == &mons[PM_SHARK] && part == HAIR)
        /*KR return "skin"; */ /* sharks don't have scales */
        return "피부";
    if ((mptr == &mons[PM_JELLYFISH] || mptr == &mons[PM_KRAKEN])
        && (part == ARM || part == FINGER || part == HAND || part == FOOT
            || part == TOE))
        /*KR return "tentacle"; */
        return "촉수";
    if (mptr == &mons[PM_FLOATING_EYE] && part == EYE)
        /*KR return "cornea"; */
        return "각막";
    if (humanoid(mptr)
        && (part == ARM || part == FINGER || part == FINGERTIP || part == HAND
            || part == HANDED))
        return humanoid_parts[part];
    if (mptr == &mons[PM_RAVEN])
        return bird_parts[part];
    if (mptr->mlet == S_CENTAUR || mptr->mlet == S_UNICORN
        || (mptr == &mons[PM_ROTHE] && part != HAIR))
        return horse_parts[part];
    if (mptr->mlet == S_LIGHT) {
        if (part == HANDED)
            /*KR return "rayed"; */
            return "빛살이 뻗은";
        else if (part == ARM || part == FINGER || part == FINGERTIP
                 || part == HAND)
            /*KR return "ray"; */
            return "빛살";
        else
            /*KR return "beam"; */
            return "광선";
    }
    if (mptr == &mons[PM_STALKER] && part == HEAD)
        /*KR return "head"; */
        return "머리";
    if (mptr->mlet == S_EEL && mptr != &mons[PM_JELLYFISH])
        return fish_parts[part];
    if (mptr->mlet == S_WORM)
        return worm_parts[part];
    if (slithy(mptr) || (mptr->mlet == S_DRAGON && part == HAIR))
        return snake_parts[part];
    if (mptr->mlet == S_EYE)
        return sphere_parts[part];
    if (mptr->mlet == S_JELLY || mptr->mlet == S_PUDDING
        || mptr->mlet == S_BLOB || mptr == &mons[PM_JELLYFISH])
        return jelly_parts[part];
    if (mptr->mlet == S_VORTEX || mptr->mlet == S_ELEMENTAL)
        return vortex_parts[part];
    if (mptr->mlet == S_FUNGUS)
        return fungus_parts[part];
    if (humanoid(mptr))
        return humanoid_parts[part];
    return animal_parts[part];
}

const char *
body_part(part)
int part;
{
    return mbodypart(&youmonst, part);
}

int
poly_gender()
{
    /* Returns gender of polymorphed player;
     * 0/1=same meaning as flags.female, 2=none.
     */
    if (is_neuter(youmonst.data) || !humanoid(youmonst.data))
        return 2;
    return flags.female;
}

void ugolemeffects(damtype, dam) int damtype, dam;
{
    int heal = 0;

    /* We won't bother with "slow"/"haste" since players do not
     * have a monster-specific slow/haste so there is no way to
     * restore the old velocity once they are back to human.
     */
    if (u.umonnum != PM_FLESH_GOLEM && u.umonnum != PM_IRON_GOLEM)
        return;
    switch (damtype) {
    case AD_ELEC:
        if (u.umonnum == PM_FLESH_GOLEM)
            heal = (dam + 5) / 6; /* Approx 1 per die */
        break;
    case AD_FIRE:
        if (u.umonnum == PM_IRON_GOLEM)
            heal = dam;
        break;
    }
    if (heal && (u.mh < u.mhmax)) {
        u.mh += heal;
        if (u.mh > u.mhmax)
            u.mh = u.mhmax;
        context.botl = 1;
        /*KR pline("Strangely, you feel better than before."); */
        pline("이상하게도, 이전보다 기분이 훨씬 좋아진 것을 느낀다.");
        exercise(A_STR, TRUE);
    }
}

STATIC_OVL int
armor_to_dragon(atyp)
int atyp;
{
    switch (atyp) {
    case GRAY_DRAGON_SCALE_MAIL:
    case GRAY_DRAGON_SCALES:
        return PM_GRAY_DRAGON;
    case SILVER_DRAGON_SCALE_MAIL:
    case SILVER_DRAGON_SCALES:
        return PM_SILVER_DRAGON;
#if 0 /* DEFERRED */
    case SHIMMERING_DRAGON_SCALE_MAIL:
    case SHIMMERING_DRAGON_SCALES:
        return PM_SHIMMERING_DRAGON;
#endif
    case RED_DRAGON_SCALE_MAIL:
    case RED_DRAGON_SCALES:
        return PM_RED_DRAGON;
    case ORANGE_DRAGON_SCALE_MAIL:
    case ORANGE_DRAGON_SCALES:
        return PM_ORANGE_DRAGON;
    case WHITE_DRAGON_SCALE_MAIL:
    case WHITE_DRAGON_SCALES:
        return PM_WHITE_DRAGON;
    case BLACK_DRAGON_SCALE_MAIL:
    case BLACK_DRAGON_SCALES:
        return PM_BLACK_DRAGON;
    case BLUE_DRAGON_SCALE_MAIL:
    case BLUE_DRAGON_SCALES:
        return PM_BLUE_DRAGON;
    case GREEN_DRAGON_SCALE_MAIL:
    case GREEN_DRAGON_SCALES:
        return PM_GREEN_DRAGON;
    case YELLOW_DRAGON_SCALE_MAIL:
    case YELLOW_DRAGON_SCALES:
        return PM_YELLOW_DRAGON;
    default:
        return -1;
    }
}

/* some species have awareness of other species */
static void
polysense()
{
    short warnidx = NON_PM;

    context.warntype.speciesidx = NON_PM;
    context.warntype.species = 0;
    context.warntype.polyd = 0;
    HWarn_of_mon &= ~FROMRACE;

    switch (u.umonnum) {
    case PM_PURPLE_WORM:
        warnidx = PM_SHRIEKER;
        break;
    case PM_VAMPIRE:
    case PM_VAMPIRE_LORD:
        context.warntype.polyd = M2_HUMAN | M2_ELF;
        HWarn_of_mon |= FROMRACE;
        return;
    }
    if (warnidx >= LOW_PM) {
        context.warntype.speciesidx = warnidx;
        context.warntype.species = &mons[warnidx];
        HWarn_of_mon |= FROMRACE;
    }
}

/* True iff hero's role or race has been genocided */
boolean
ugenocided()
{
    return (boolean) ((mvitals[urole.malenum].mvflags & G_GENOD)
                      || (urole.femalenum != NON_PM
                          && (mvitals[urole.femalenum].mvflags & G_GENOD))
                      || (mvitals[urace.malenum].mvflags & G_GENOD)
                      || (urace.femalenum != NON_PM
                          && (mvitals[urace.femalenum].mvflags & G_GENOD)));
}

/* how hero feels "inside" after self-genocide of role or race */
const char *
udeadinside()
{
    /* self-genocide used to always say "you feel dead inside" but that
       seems silly when you're polymorphed into something undead;
       monkilled() distinguishes between living (killed) and non (destroyed)
       for monster death message; we refine the nonliving aspect a bit */
    return !nonliving(youmonst.data)
               /*KR ? "dead" */ /* living, including demons */
               ? "죽은 것 같은" /* living, including demons */
               : !weirdnonliving(youmonst.data)
                     /*KR ? "condemned" */  /* undead plus manes */
                     ? "저주받은 것 같은"   /* undead plus manes */
                     /*KR : "empty"; */     /* golems plus vortices */
                     : "공허한"; /* golems plus vortices */
}

/*polyself.c*/
