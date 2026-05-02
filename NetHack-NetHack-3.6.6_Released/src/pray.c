/* NetHack 3.6	pray.c	$NHDT-Date: 1573346192 2019/11/10 00:36:32 $  $NHDT-Branch: NetHack-3.6 $:$NHDT-Revision: 1.118 $ */
/* Copyright (c) Benson I. Margulies, Mike Stephenson, Steve Linhart, 1989. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

STATIC_PTR int NDECL(prayer_done);
STATIC_DCL struct obj *NDECL(worst_cursed_item);
STATIC_DCL int NDECL(in_trouble);
STATIC_DCL void FDECL(fix_worst_trouble, (int));
STATIC_DCL void FDECL(angrygods, (ALIGNTYP_P));
STATIC_DCL void FDECL(at_your_feet, (const char *));
STATIC_DCL void NDECL(gcrownu);
STATIC_DCL void FDECL(pleased, (ALIGNTYP_P));
STATIC_DCL void FDECL(godvoice, (ALIGNTYP_P, const char *));
STATIC_DCL void FDECL(god_zaps_you, (ALIGNTYP_P));
STATIC_DCL void FDECL(fry_by_god, (ALIGNTYP_P, BOOLEAN_P));
STATIC_DCL void FDECL(gods_angry, (ALIGNTYP_P));
STATIC_DCL void FDECL(gods_upset, (ALIGNTYP_P));
STATIC_DCL void FDECL(consume_offering, (struct obj *));
STATIC_DCL boolean FDECL(water_prayer, (BOOLEAN_P));
STATIC_DCL boolean FDECL(blocked_boulder, (int, int));

/* simplify a few tests */
#define Cursed_obj(obj, typ) ((obj) && (obj)->otyp == (typ) && (obj)->cursed)

/*
 * Logic behind deities and altars and such:
 * + prayers are made to your god if not on an altar, and to the altar's god
 *   if you are on an altar
 * + If possible, your god answers all prayers, which is why bad things happen
 *   if you try to pray on another god's altar
 * + sacrifices work basically the same way, but the other god may decide to
 *   accept your allegiance, after which they are your god.  If rejected,
 *   your god takes over with your punishment.
 * + if you're in Gehennom, all messages come from Moloch
 */

/*
 *      Moloch, who dwells in Gehennom, is the "renegade" cruel god
 *      responsible for the theft of the Amulet from Marduk, the Creator.
 *      Moloch is unaligned.
 */
/*KR static const char *Moloch = "Moloch"; */
static const char *Moloch = "몰록";

static const char *godvoices[] = {
    /*KR "booms out", "thunders", "rings out", "booms", */
    "울려 퍼졌다",
    "천둥처럼 쳤다",
    "메아리쳤다",
    "울렸다",
};

/* values calculated when prayer starts, and used when completed */
static aligntyp p_aligntyp;
static int p_trouble;
static int p_type; /* (-1)-3: (-1)=really naughty, 3=really good */

#define PIOUS 20
#define DEVOUT 14
#define FERVENT 9
#define STRIDENT 4

/*
 * The actual trouble priority is determined by the order of the
 * checks performed in in_trouble() rather than by these numeric
 * values, so keep that code and these values synchronized in
 * order to have the values be meaningful.
 */

#define TROUBLE_STONED 14
#define TROUBLE_SLIMED 13
#define TROUBLE_STRANGLED 12
#define TROUBLE_LAVA 11
#define TROUBLE_SICK 10
#define TROUBLE_STARVING 9
#define TROUBLE_REGION 8 /* stinking cloud */
#define TROUBLE_HIT 7
#define TROUBLE_LYCANTHROPE 6
#define TROUBLE_COLLAPSING 5
#define TROUBLE_STUCK_IN_WALL 4
#define TROUBLE_CURSED_LEVITATION 3
#define TROUBLE_UNUSEABLE_HANDS 2
#define TROUBLE_CURSED_BLINDFOLD 1

#define TROUBLE_PUNISHED (-1)
#define TROUBLE_FUMBLING (-2)
#define TROUBLE_CURSED_ITEMS (-3)
#define TROUBLE_SADDLE (-4)
#define TROUBLE_BLIND (-5)
#define TROUBLE_POISONED (-6)
#define TROUBLE_WOUNDED_LEGS (-7)
#define TROUBLE_HUNGRY (-8)
#define TROUBLE_STUNNED (-9)
#define TROUBLE_CONFUSED (-10)
#define TROUBLE_HALLUCINATION (-11)


#define ugod_is_angry() (u.ualign.record < 0)
#define on_altar() IS_ALTAR(levl[u.ux][u.uy].typ)
#define on_shrine() ((levl[u.ux][u.uy].altarmask & AM_SHRINE) != 0)
#define a_align(x, y) ((aligntyp) Amask2align(levl[x][y].altarmask & AM_MASK))

/* critically low hit points if hp <= 5 or hp <= maxhp/N for some N */
boolean
critically_low_hp(only_if_injured)
boolean only_if_injured; /* determines whether maxhp <= 5 matters */
{
    int divisor, hplim, curhp = Upolyd ? u.mh : u.uhp,
                        maxhp = Upolyd ? u.mhmax : u.uhpmax;

    if (only_if_injured && !(curhp < maxhp))
        return FALSE;
    /* if maxhp is extremely high, use lower threshold for the division test
       (golden glow cuts off at 11+5*lvl, nurse interaction at 25*lvl; this
       ought to use monster hit dice--and a smaller multiplier--rather than
       ulevel when polymorphed, but polyself doesn't maintain that) */
    hplim = 15 * u.ulevel;
    if (maxhp > hplim)
        maxhp = hplim;
    /* 7 used to be the unconditional divisor */
    switch (xlev_to_rank(u.ulevel)) { /* maps 1..30 into 0..8 */
    case 0:
    case 1:
        divisor = 5;
        break; /* explvl 1 to 5 */
    case 2:
    case 3:
        divisor = 6;
        break; /* explvl 6 to 13 */
    case 4:
    case 5:
        divisor = 7;
        break; /* explvl 14 to 21 */
    case 6:
    case 7:
        divisor = 8;
        break; /* explvl 22 to 29 */
    default:
        divisor = 9;
        break; /* explvl 30+ */
    }
    /* 5 is a magic number in TROUBLE_HIT handling below */
    return (boolean) (curhp <= 5 || curhp * divisor <= maxhp);
}

/* return True if surrounded by impassible rock, regardless of the state
   of your own location (for example, inside a doorless closet) */
boolean
stuck_in_wall()
{
    int i, j, x, y, count = 0;

    if (Passes_walls)
        return FALSE;
    for (i = -1; i <= 1; i++) {
        x = u.ux + i;
        for (j = -1; j <= 1; j++) {
            if (!i && !j)
                continue;
            y = u.uy + j;
            if (!isok(x, y)
                || (IS_ROCK(levl[x][y].typ)
                    && (levl[x][y].typ != SDOOR && levl[x][y].typ != SCORR))
                || (blocked_boulder(i, j) && !throws_rocks(youmonst.data)))
                ++count;
        }
    }
    return (count == 8) ? TRUE : FALSE;
}

/*
 * Return 0 if nothing particular seems wrong, positive numbers for
 * serious trouble, and negative numbers for comparative annoyances.
 * This returns the worst problem. There may be others, and the gods
 * may fix more than one.
 *
 * This could get as bizarre as noting surrounding opponents, (or
 * hostile dogs), but that's really hard.
 *
 * We could force rehumanize of polyselfed people, but we can't tell
 * unintentional shape changes from the other kind. Oh well.
 * 3.4.2: make an exception if polymorphed into a form which lacks
 * hands; that's a case where the ramifications override this doubt.
 */
STATIC_OVL int
in_trouble()
{
    struct obj *otmp;
    int i;

    /*
     * major troubles
     */
    if (Stoned)
        return TROUBLE_STONED;
    if (Slimed)
        return TROUBLE_SLIMED;
    if (Strangled)
        return TROUBLE_STRANGLED;
    if (u.utrap && u.utraptype == TT_LAVA)
        return TROUBLE_LAVA;
    if (Sick)
        return TROUBLE_SICK;
    if (u.uhs >= WEAK)
        return TROUBLE_STARVING;
    if (region_danger())
        return TROUBLE_REGION;
    if (critically_low_hp(FALSE))
        return TROUBLE_HIT;
    if (u.ulycn >= LOW_PM)
        return TROUBLE_LYCANTHROPE;
    if (near_capacity() >= EXT_ENCUMBER && AMAX(A_STR) - ABASE(A_STR) > 3)
        return TROUBLE_COLLAPSING;
    if (stuck_in_wall())
        return TROUBLE_STUCK_IN_WALL;
    if (Cursed_obj(uarmf, LEVITATION_BOOTS)
        || stuck_ring(uleft, RIN_LEVITATION)
        || stuck_ring(uright, RIN_LEVITATION))
        return TROUBLE_CURSED_LEVITATION;
    if (nohands(youmonst.data) || !freehand()) {
        /* for bag/box access [cf use_container()]...
           make sure it's a case that we know how to handle;
           otherwise "fix all troubles" would get stuck in a loop */
        if (welded(uwep))
            return TROUBLE_UNUSEABLE_HANDS;
        if (Upolyd && nohands(youmonst.data)
            && (!Unchanging || ((otmp = unchanger()) != 0 && otmp->cursed)))
            return TROUBLE_UNUSEABLE_HANDS;
    }
    if (Blindfolded && ublindf->cursed)
        return TROUBLE_CURSED_BLINDFOLD;

    /*
     * minor troubles
     */
    if (Punished || (u.utrap && u.utraptype == TT_BURIEDBALL))
        return TROUBLE_PUNISHED;
    if (Cursed_obj(uarmg, GAUNTLETS_OF_FUMBLING)
        || Cursed_obj(uarmf, FUMBLE_BOOTS))
        return TROUBLE_FUMBLING;
    if (worst_cursed_item())
        return TROUBLE_CURSED_ITEMS;
    if (u.usteed) { /* can't voluntarily dismount from a cursed saddle */
        otmp = which_armor(u.usteed, W_SADDLE);
        if (Cursed_obj(otmp, SADDLE))
            return TROUBLE_SADDLE;
    }

    if (Blinded > 1 && haseyes(youmonst.data)
        && (!u.uswallow
            || !attacktype_fordmg(u.ustuck->data, AT_ENGL, AD_BLND)))
        return TROUBLE_BLIND;
    /* deafness isn't it's own trouble; healing magic cures deafness
       when it cures blindness, so do the same with trouble repair */
    if ((HDeaf & TIMEOUT) > 1L)
        return TROUBLE_BLIND;

    for (i = 0; i < A_MAX; i++)
        if (ABASE(i) < AMAX(i))
            return TROUBLE_POISONED;
    if (Wounded_legs && !u.usteed)
        return TROUBLE_WOUNDED_LEGS;
    if (u.uhs >= HUNGRY)
        return TROUBLE_HUNGRY;
    if (HStun & TIMEOUT)
        return TROUBLE_STUNNED;
    if (HConfusion & TIMEOUT)
        return TROUBLE_CONFUSED;
    if (HHallucination & TIMEOUT)
        return TROUBLE_HALLUCINATION;
    return 0;
}

/* select an item for TROUBLE_CURSED_ITEMS */
STATIC_OVL struct obj *
worst_cursed_item()
{
    register struct obj *otmp;

    /* if strained or worse, check for loadstone first */
    if (near_capacity() >= HVY_ENCUMBER) {
        for (otmp = invent; otmp; otmp = otmp->nobj)
            if (Cursed_obj(otmp, LOADSTONE))
                return otmp;
    }
    /* weapon takes precedence if it is interfering
       with taking off a ring or putting on a shield */
    if (welded(uwep) && (uright || bimanual(uwep))) { /* weapon */
        otmp = uwep;
        /* gloves come next, due to rings */
    } else if (uarmg && uarmg->cursed) { /* gloves */
        otmp = uarmg;
        /* then shield due to two handed weapons and spells */
    } else if (uarms && uarms->cursed) { /* shield */
        otmp = uarms;
        /* then cloak due to body armor */
    } else if (uarmc && uarmc->cursed) { /* cloak */
        otmp = uarmc;
    } else if (uarm && uarm->cursed) { /* suit */
        otmp = uarm;
        /* if worn helmet of opposite alignment is making you an adherent
           of the current god, he/she/it won't uncurse that for you */
    } else if (uarmh && uarmh->cursed /* helmet */
               && uarmh->otyp != HELM_OF_OPPOSITE_ALIGNMENT) {
        otmp = uarmh;
    } else if (uarmf && uarmf->cursed) { /* boots */
        otmp = uarmf;
    } else if (uarmu && uarmu->cursed) { /* shirt */
        otmp = uarmu;
    } else if (uamul && uamul->cursed) { /* amulet */
        otmp = uamul;
    } else if (uleft && uleft->cursed) { /* left ring */
        otmp = uleft;
    } else if (uright && uright->cursed) { /* right ring */
        otmp = uright;
    } else if (ublindf && ublindf->cursed) { /* eyewear */
        otmp = ublindf;                      /* must be non-blinding lenses */
        /* if weapon wasn't handled above, do it now */
    } else if (welded(uwep)) { /* weapon */
        otmp = uwep;
        /* active secondary weapon even though it isn't welded */
    } else if (uswapwep && uswapwep->cursed && u.twoweap) {
        otmp = uswapwep;
        /* all worn items ought to be handled by now */
    } else {
        for (otmp = invent; otmp; otmp = otmp->nobj) {
            if (!otmp->cursed)
                continue;
            if (otmp->otyp == LOADSTONE || confers_luck(otmp))
                break;
        }
    }
    return otmp;
}

STATIC_OVL void fix_worst_trouble(trouble) int trouble;
{
    int i;
    struct obj *otmp = 0;
    const char *what = (const char *) 0;
#if 0 /*KR: 원본*/
    static NEARDATA const char leftglow[] = "Your left ring softly glows",
                               rightglow[] = "Your right ring softly glows";
#else /*KR: KRNethack 맞춤 번역 */
    static NEARDATA const char leftglow[] = "왼쪽 반지",
                               rightglow[] = "오른쪽 반지";
#endif

    switch (trouble) {
    case TROUBLE_STONED:
        /*KR make_stoned(0L, "You feel more limber.", 0, (char *) 0); */
        make_stoned(0L, "몸이 좀 더 유연해진 기분이 든다.", 0, (char *) 0);
        break;
    case TROUBLE_SLIMED:
        /*KR make_slimed(0L, "The slime disappears."); */
        make_slimed(0L, "슬라임이 사라졌다.");
        break;
    case TROUBLE_STRANGLED:
        if (uamul && uamul->otyp == AMULET_OF_STRANGULATION) {
            /*KR Your("amulet vanishes!"); */
            Your("부적이 사라졌다!");
            useup(uamul);
        }
        /*KR You("can breathe again."); */
        You("다시 숨을 쉴 수 있다.");
        Strangled = 0;
        context.botl = 1;
        break;
    case TROUBLE_LAVA:
        /*KR You("are back on solid ground."); */
        You("단단한 땅으로 되돌아왔다.");
        /* teleport should always succeed, but if not, just untrap them */
        if (!safe_teleds(FALSE))
            reset_utrap(TRUE);
        break;
    case TROUBLE_STARVING:
        /* temporarily lost strength recovery now handled by init_uhunger() */
        /*FALLTHRU*/
    case TROUBLE_HUNGRY:
        /*KR Your("%s feels content.", body_part(STOMACH)); */
        Your("%s 든든해졌다.", append_josa(body_part(STOMACH), "이"));
        init_uhunger();
        context.botl = 1;
        break;
    case TROUBLE_SICK:
        /*KR You_feel("better."); */
        You("나아졌다.");
        make_sick(0L, (char *) 0, FALSE, SICK_ALL);
        break;
    case TROUBLE_REGION:
        /* stinking cloud, with hero vulnerable to HP loss */
        region_safety();
        break;
    case TROUBLE_HIT:
        /* "fix all troubles" will keep trying if hero has
           5 or less hit points, so make sure they're always
           boosted to be more than that */
        /*KR You_feel("much better."); */
        You("훨씬 나아졌다.");
        if (Upolyd) {
            u.mhmax += rnd(5);
            if (u.mhmax <= 5)
                u.mhmax = 5 + 1;
            u.mh = u.mhmax;
        }
        if (u.uhpmax < u.ulevel * 5 + 11)
            u.uhpmax += rnd(5);
        if (u.uhpmax <= 5)
            u.uhpmax = 5 + 1;
        u.uhp = u.uhpmax;
        context.botl = 1;
        break;
    case TROUBLE_COLLAPSING:
        /* override Fixed_abil; uncurse that if feasible */
#if 0 /*KR: 원본*/
        You_feel("%sstronger.",
                 (AMAX(A_STR) - ABASE(A_STR) > 6) ? "much " : "");
#else /*KR: KRNethack 맞춤 번역 */
        You("%s강해진 기분이다.",
                 (AMAX(A_STR) - ABASE(A_STR) > 6) ? "훨씬 " : "");
#endif
        ABASE(A_STR) = AMAX(A_STR);
        context.botl = 1;
        if (Fixed_abil) {
            if ((otmp = stuck_ring(uleft, RIN_SUSTAIN_ABILITY)) != 0) {
                if (otmp == uleft)
                    what = leftglow;
            } else if ((otmp = stuck_ring(uright, RIN_SUSTAIN_ABILITY))
                       != 0) {
                if (otmp == uright)
                    what = rightglow;
            }
            if (otmp)
                goto decurse;
        }
        break;
    case TROUBLE_STUCK_IN_WALL:
        /* no control, but works on no-teleport levels */
        if (safe_teleds(FALSE)) {
            /*KR Your("surroundings change."); */
            Your("주변 환경이 변했다.");
        } else {
            /* safe_teleds() couldn't find a safe place; perhaps the
               level is completely full.  As a last resort, confer
               intrinsic wall/rock-phazing.  Hero might get stuck
               again fairly soon....
               Without something like this, fix_all_troubles can get
               stuck in an infinite loop trying to fix STUCK_IN_WALL
               and repeatedly failing. */
            set_itimeout(&HPasses_walls, (long) (d(4, 4) + 4)); /* 8..20 */
            /* how else could you move between packed rocks or among
               lattice forming "solid" rock? */
            /*KR You_feel("much slimmer."); */
            You("몸이 훨씬 홀쭉해진 기분이다.");
        }
        break;
    case TROUBLE_CURSED_LEVITATION:
        if (Cursed_obj(uarmf, LEVITATION_BOOTS)) {
            otmp = uarmf;
        } else if ((otmp = stuck_ring(uleft, RIN_LEVITATION)) != 0) {
            if (otmp == uleft)
                what = leftglow;
        } else if ((otmp = stuck_ring(uright, RIN_LEVITATION)) != 0) {
            if (otmp == uright)
                what = rightglow;
        }
        goto decurse;
    case TROUBLE_UNUSEABLE_HANDS:
        if (welded(uwep)) {
            otmp = uwep;
            goto decurse;
        }
        if (Upolyd && nohands(youmonst.data)) {
            if (!Unchanging) {
                /*KR Your("shape becomes uncertain."); */
                Your("형태가 불분명해졌다.");
                rehumanize(); /* "You return to {normal} form." */
            } else if ((otmp = unchanger()) != 0 && otmp->cursed) {
                /* otmp is an amulet of unchanging */
                goto decurse;
            }
        }
        if (nohands(youmonst.data) || !freehand())
            impossible("fix_worst_trouble: couldn't cure hands.");
        break;
    case TROUBLE_CURSED_BLINDFOLD:
        otmp = ublindf;
        goto decurse;
    case TROUBLE_LYCANTHROPE:
        you_unwere(TRUE);
        break;
    /*
     */
    case TROUBLE_PUNISHED:
        /*KR Your("chain disappears."); */
        Your("쇠사슬이 사라졌다.");
        if (u.utrap && u.utraptype == TT_BURIEDBALL)
            buried_ball_to_freedom();
        else
            unpunish();
        break;
    case TROUBLE_FUMBLING:
        if (Cursed_obj(uarmg, GAUNTLETS_OF_FUMBLING))
            otmp = uarmg;
        else if (Cursed_obj(uarmf, FUMBLE_BOOTS))
            otmp = uarmf;
        goto decurse;
        /*NOTREACHED*/
        break;
    case TROUBLE_CURSED_ITEMS:
        otmp = worst_cursed_item();
        if (otmp == uright)
            what = rightglow;
        else if (otmp == uleft)
            what = leftglow;
    decurse:
        if (!otmp) {
            impossible("fix_worst_trouble: nothing to uncurse.");
            return;
        }
        if (otmp == uarmg && Glib) {
            make_glib(0);
            /*KR Your("%s are no longer slippery.",
             * gloves_simple_name(uarmg)); */
            Your("%s 더 이상 미끄럽지 않다.",
                 append_josa(gloves_simple_name(uarmg), "은"));
            if (!otmp->cursed)
                break;
        }
        if (!Blind || (otmp == ublindf && Blindfolded_only)) {
#if 0 /*KR: 원본*/
            pline("%s %s.",
                  what ? what : (const char *) Yobjnam2(otmp, "softly glow"),
                  hcolor(NH_AMBER));
#else /*KR: KRNethack 맞춤 번역 */
            pline("%s %s으로 은은하게 빛났다.",
                  what ? what : append_josa(Yname2(otmp), "이"),
                  hcolor(NH_AMBER));
#endif
            iflags.last_msg = PLNMSG_OBJ_GLOWS;
            otmp->bknown = !Hallucination; /* ok to skip set_bknown() */
        }
        uncurse(otmp);
        update_inventory();
        break;
    case TROUBLE_POISONED:
        /* override Fixed_abil; ignore items which confer that */
        if (Hallucination)
            /*KR pline("There's a tiger in your tank."); */
            pline("주유 탱크에 호랑이가 들어갔다."); /* (호랑이표 가솔린 광고
                                                        패러디) */
        else
            /*KR You_feel("in good health again."); */
            You("다시 건강해진 기분이다.");
        for (i = 0; i < A_MAX; i++) {
            if (ABASE(i) < AMAX(i)) {
                ABASE(i) = AMAX(i);
                context.botl = 1;
            }
        }
        (void) encumber_msg();
        break;
    case TROUBLE_BLIND: { /* handles deafness as well as blindness */
        char msgbuf[BUFSZ];
#if 0 /*KR*/
        const char *eyes = body_part(EYE);
#endif
        boolean cure_deaf = (HDeaf & TIMEOUT) ? TRUE : FALSE;

        msgbuf[0] = '\0';
        if (Blinded) {
#if 0 /*KR*/
            if (eyecount(youmonst.data) != 1)
                eyes = makeplural(eyes);
#endif
#if 0 /*KR: 원본*/
            Sprintf(msgbuf, "Your %s %s better", eyes, vtense(eyes, "feel"));
#else /*KR: KRNethack 맞춤 번역 */
            Sprintf(msgbuf, "당신의 %s 나아진 기분이 든다",
                    append_josa(body_part(EYE), "이"));
#endif
            u.ucreamed = 0;
            make_blinded(0L, FALSE);
        }
        if (cure_deaf) {
            make_deaf(0L, FALSE);
            if (!Deaf)
#if 0 /*KR: 원본*/
                Sprintf(eos(msgbuf), "%s can hear again",
                        !*msgbuf ? "You" : " and you");
#else /*KR: KRNethack 맞춤 번역 */
                Sprintf(eos(msgbuf), "%s다시 소리를 들었다",
                        !*msgbuf ? "당신은 " : ", 그리고 ");
#endif
        }
        if (*msgbuf)
            /*KR pline("%s.", msgbuf); */
            pline("%s.", msgbuf);
        break;
    }
    case TROUBLE_WOUNDED_LEGS:
        heal_legs(0);
        break;
    case TROUBLE_STUNNED:
        make_stunned(0L, TRUE);
        break;
    case TROUBLE_CONFUSED:
        make_confused(0L, TRUE);
        break;
    case TROUBLE_HALLUCINATION:
        /*KR pline("Looks like you are back in Kansas."); */
        pline("캔자스로 돌아온 것 같다."); /* (오즈의 마법사 패러디) */
        (void) make_hallucinated(0L, FALSE, 0L);
        break;
    case TROUBLE_SADDLE:
        otmp = which_armor(u.usteed, W_SADDLE);
        if (!Blind) {
#if 0 /*KR:T*/
            pline("%s %s.", Yobjnam2(otmp, "softly glow"),
             * hcolor(NH_AMBER));
#else
            pline("%s %s으로 은은하게 빛났다.",
                  append_josa(y_monnam(u.usteed), "이"),
                  hcolor(NH_AMBER));
#endif
            set_bknown(otmp, 1);
        }
        uncurse(otmp);
        break;
    }
}

/* "I am sometimes shocked by... the nuns who never take a bath without
 * wearing a bathrobe all the time.  When asked why, since no man can see
 * them, they reply 'Oh, but you forget the good God'.  Apparently they
 * conceive of the Deity as a Peeping Tom, whose omnipotence enables Him to
 * see through bathroom walls, but who is foiled by bathrobes." --Bertrand
 * Russell, 1943 Divine wrath, dungeon walls, and armor follow the same
 * principle.
 */
STATIC_OVL void god_zaps_you(resp_god) aligntyp resp_god;
{
    if (u.uswallow) {
        /*KR pline(
          "Suddenly a bolt of lightning comes down at you from the heavens!");
        */
        pline("갑자기 하늘에서 당신을 향해 벼락이 떨어졌다!");
        /*KR pline("It strikes %s!", mon_nam(u.ustuck)); */
        pline("벼락이 %s 강타했다!", append_josa(mon_nam(u.ustuck), "을"));
        if (!resists_elec(u.ustuck)) {
            /*KR pline("%s fries to a crisp!", Monnam(u.ustuck)); */
            pline("%s 바싹 타버렸다!", append_josa(Monnam(u.ustuck), "은"));
            /* Yup, you get experience.  It takes guts to successfully
             * pull off this trick on your god, anyway.
             * Other credit/blame applies (luck or alignment adjustments),
             * but not direct kill count (pacifist conduct).
             */
            xkilled(u.ustuck, XKILL_NOMSG | XKILL_NOCONDUCT);
        } else
            /*KR pline("%s seems unaffected.", Monnam(u.ustuck)); */
            pline("%s 아무런 영향도 받지 않은 것 같다.",
                  append_josa(Monnam(u.ustuck), "은"));
    } else {
        /*KR pline("Suddenly, a bolt of lightning strikes you!"); */
        pline("갑자기, 벼락이 당신을 강타했다!");
        if (Reflecting) {
            shieldeff(u.ux, u.uy);
            if (Blind)
                /*KR pline("For some reason you're unaffected."); */
                pline("어째서인지 당신은 아무런 영향도 받지 않았다.");
            else
                /*KR (void) ureflects("%s reflects from your %s.", "It"); */
                (void) ureflects("%s 당신의 %s 부딪혀 반사되었다.", "벼락이");
        } else if (Shock_resistance) {
            shieldeff(u.ux, u.uy);
            /*KR pline("It seems not to affect you."); */
            pline("당신에겐 아무런 영향도 주지 못한 것 같다.");
        } else
            fry_by_god(resp_god, FALSE);
    }

    /*KR pline("%s is not deterred...", align_gname(resp_god)); */
    pline("%s 단념하지 않았다...", append_josa(align_gname(resp_god), "은"));
    if (u.uswallow) {
        /*KR pline("A wide-angle disintegration beam aimed at you hits %s!",
         * mon_nam(u.ustuck)); */
        pline("당신을 겨냥한 광각 붕괴 광선이 %s 명중했다!",
              append_josa(mon_nam(u.ustuck), "에게"));
        if (!resists_disint(u.ustuck)) {
            /*KR pline("%s disintegrates into a pile of dust!",
             * Monnam(u.ustuck)); */
            pline("%s 먼지 더미로 붕괴해버렸다!",
                  append_josa(Monnam(u.ustuck), "은"));
            xkilled(u.ustuck, XKILL_NOMSG | XKILL_NOCORPSE | XKILL_NOCONDUCT);
        } else
            /*KR pline("%s seems unaffected.", Monnam(u.ustuck)); */
            pline("%s 아무런 영향도 받지 않은 것 같다.",
                  append_josa(Monnam(u.ustuck), "은"));
    } else {
        /*KR pline("A wide-angle disintegration beam hits you!"); */
        pline("광각 붕괴 광선이 당신에게 명중했다!");

        /* disintegrate shield and body armor before disintegrating
         * the impudent mortal, like black dragon breath -3.
         */
        if (uarms && !(EReflecting & W_ARMS)
            && !(EDisint_resistance & W_ARMS))
            (void) destroy_arm(uarms);
        if (uarmc && !(EReflecting & W_ARMC)
            && !(EDisint_resistance & W_ARMC))
            (void) destroy_arm(uarmc);
        if (uarm && !(EReflecting & W_ARM) && !(EDisint_resistance & W_ARM)
            && !uarmc)
            (void) destroy_arm(uarm);
        if (uarmu && !uarm && !uarmc)
            (void) destroy_arm(uarmu);
        if (!Disint_resistance) {
            fry_by_god(resp_god, TRUE);
        } else {
            /*KR You("bask in its %s glow for a minute...", NH_BLACK); */
            You("잠시 그 %s 빛을 쬐며 몸을 녹였다...", NH_BLACK);
            /*KR godvoice(resp_god, "I believe it not!"); */
            godvoice(resp_god, "믿을 수가 없군!");
        }
        if (Is_astralevel(&u.uz) || Is_sanctum(&u.uz)) {
            /* one more try for high altars */
            /*KR verbalize("Thou cannot escape my wrath, mortal!"); */
            verbalize("필멸자여, 너는 내 분노를 피할 수 없다!");
            summon_minion(resp_god, FALSE);
            summon_minion(resp_god, FALSE);
            summon_minion(resp_god, FALSE);
            /*KR verbalize("Destroy %s, my servants!", uhim()); */
            verbalize("나의 종들아, 저 자를 파괴해라!");
        }
    }
}

STATIC_OVL void fry_by_god(resp_god, via_disintegration) aligntyp resp_god;
boolean via_disintegration;
{
#if 0 /*KR: 원본*/
    You("%s!", !via_disintegration ? "fry to a crisp"
                                   : "disintegrate into a pile of dust");
#else /*KR: KRNethack 맞춤 번역 */
    You("%s!",
        !via_disintegration ? "바싹 타버렸다" : "먼지 더미로 붕괴해버렸다");
#endif
    killer.format = KILLED_BY;
    /*KR Sprintf(killer.name, "the wrath of %s", align_gname(resp_god)); */
    Sprintf(killer.name, "%s의 분노", align_gname(resp_god));
    done(DIED);
}

STATIC_OVL void angrygods(resp_god) aligntyp resp_god;
{
    int maxanger;

    if (Inhell)
        resp_god = A_NONE;
    u.ublessed = 0;

    /* changed from tmp = u.ugangr + abs (u.uluck) -- rph */
    /* added test for alignment diff -dlc */
    if (resp_god != u.ualign.type)
        maxanger = u.ualign.record / 2 + (Luck > 0 ? -Luck / 3 : -Luck);
    else
        maxanger =
            3 * u.ugangr
            + ((Luck > 0 || u.ualign.record >= STRIDENT) ? -Luck / 3 : -Luck);
    if (maxanger < 1)
        maxanger = 1; /* possible if bad align & good luck */
    else if (maxanger > 15)
        maxanger = 15; /* be reasonable */

    switch (rn2(maxanger)) {
    case 0:
    case 1:
#if 0 /*KR: 원본*/
        You_feel("that %s is %s.", align_gname(resp_god),
                 Hallucination ? "bummed" : "displeased");
#else /*KR: KRNethack 맞춤 번역 */
        You("%s %s 것 같다.", append_josa(align_gname(resp_god), "이"),
                 Hallucination ? "토라진" : "언짢아하는");
#endif
        break;
    case 2:
    case 3:
        godvoice(resp_god, (char *) 0);
#if 0 /*KR: 원본*/
        pline("\"Thou %s, %s.\"",
              (ugod_is_angry() && resp_god == u.ualign.type)
                  ? "hast strayed from the path"
                  : "art arrogant",
              youmonst.data->mlet == S_HUMAN ? "mortal" : "creature");
#else /*KR: KRNethack 맞춤 번역 */
        pline("\"%s, %s.\"",
              youmonst.data->mlet == S_HUMAN ? "필멸자여" : "피조물이여",
              (ugod_is_angry() && resp_god == u.ualign.type)
                  ? "너는 정도에서 벗어났느니라"
                  : "너는 오만하느니라");
#endif
        /*KR verbalize("Thou must relearn thy lessons!"); */
        verbalize("다시 처음부터 배워야 할 것이야!");
        (void) adjattrib(A_WIS, -1, FALSE);
        losexp((char *) 0);
        break;
    case 6:
        if (!Punished) {
            gods_angry(resp_god);
            punish((struct obj *) 0);
            break;
        } /* else fall thru */
    case 4:
    case 5:
        gods_angry(resp_god);
        if (!Blind && !Antimagic)
            /*KR pline("%s glow surrounds you.", An(hcolor(NH_BLACK))); */
            pline("%s 빛이 당신을 감쌌다.", hcolor(NH_BLACK));
        rndcurse();
        break;
    case 7:
    case 8:
        godvoice(resp_god, (char *) 0);
#if 0 /*KR: 원본*/
        verbalize("Thou durst %s me?",
                  (on_altar() && (a_align(u.ux, u.uy) != resp_god))
                      ? "scorn"
                      : "call upon");
#else /*KR: KRNethack 맞춤 번역 */
        verbalize("감히 나를 %s?",
                  (on_altar() && (a_align(u.ux, u.uy) != resp_god))
                      ? "경멸하느냐"
                      : "부르느냐");
#endif
        /* [why isn't this using verbalize()?] */
#if 0 /*KR: 원본*/
        pline("\"Then die, %s!\"",
              (youmonst.data->mlet == S_HUMAN) ? "mortal" : "creature");
#else /*KR: KRNethack 맞춤 번역 */
        pline("\"그렇다면 죽어라, %s!\"",
              (youmonst.data->mlet == S_HUMAN) ? "필멸자여" : "피조물아");
#endif
        summon_minion(resp_god, FALSE);
        break;

    default:
        gods_angry(resp_god);
        god_zaps_you(resp_god);
        break;
    }
    u.ublesscnt = rnz(300);
    return;
}

/* helper to print "str appears at your feet", or appropriate */
static void at_your_feet(str) const char *str;
{
    if (Blind)
        str = Something;
    if (u.uswallow) {
        /* barrier between you and the floor */
#if 0 /*KR: 원본*/
        pline("%s %s into %s %s.", str, vtense(str, "drop"),
              s_suffix(mon_nam(u.ustuck)), mbodypart(u.ustuck, STOMACH));
#else /*KR: KRNethack 맞춤 번역 */
        pline("%s %s %s 떨어졌다.", append_josa(str, "이"),
              s_suffix(mon_nam(u.ustuck)), mbodypart(u.ustuck, STOMACH));
#endif
    } else {
#if 0 /*KR: 원본*/
        pline("%s %s %s your %s!", str,
              Blind ? "lands" : vtense(str, "appear"),
              Levitation ? "beneath" : "at", makeplural(body_part(FOOT)));
#else /*KR: KRNethack 맞춤 번역 */
        pline("%s 당신의 %s %s %s!", append_josa(str, "이"),
              makeplural(body_part(FOOT)), Levitation ? "아래에" : "앞에",
              Blind ? "떨어졌다" : "나타났다");
#endif
    }
}

STATIC_OVL void
gcrownu()
{
    struct obj *obj;
    boolean already_exists, in_hand;
    short class_gift;
    int sp_no;
#define ok_wep(o) ((o) && ((o)->oclass == WEAPON_CLASS || is_weptool(o)))

    HSee_invisible |= FROMOUTSIDE;
    HFire_resistance |= FROMOUTSIDE;
    HCold_resistance |= FROMOUTSIDE;
    HShock_resistance |= FROMOUTSIDE;
    HSleep_resistance |= FROMOUTSIDE;
    HPoison_resistance |= FROMOUTSIDE;
    godvoice(u.ualign.type, (char *) 0);

    class_gift = STRANGE_OBJECT;
    /* 3.3.[01] had this in the A_NEUTRAL case,
       preventing chaotic wizards from receiving a spellbook */
    if (Role_if(PM_WIZARD)
        && (!uwep
            || (uwep->oartifact != ART_VORPAL_BLADE
                && uwep->oartifact != ART_STORMBRINGER))
        && !carrying(SPE_FINGER_OF_DEATH)) {
        class_gift = SPE_FINGER_OF_DEATH;
    } else if (Role_if(PM_MONK) && (!uwep || !uwep->oartifact)
               && !carrying(SPE_RESTORE_ABILITY)) {
        /* monks rarely wield a weapon */
        class_gift = SPE_RESTORE_ABILITY;
    }

    obj = ok_wep(uwep) ? uwep : 0;
    already_exists = in_hand = FALSE; /* lint suppression */
    switch (u.ualign.type) {
    case A_LAWFUL:
        u.uevent.uhand_of_elbereth = 1;
        /*KR verbalize("I crown thee...  The Hand of Elbereth!"); */
        verbalize("너에게 왕관을 씌우노라... 엘베레스의 손이여!");
        break;
    case A_NEUTRAL:
        u.uevent.uhand_of_elbereth = 2;
        in_hand = (uwep && uwep->oartifact == ART_VORPAL_BLADE);
        already_exists =
            exist_artifact(LONG_SWORD, artiname(ART_VORPAL_BLADE));
        /*KR verbalize("Thou shalt be my Envoy of Balance!"); */
        verbalize("너는 나의 균형의 사절이 될 것이니라!");
        break;
    case A_CHAOTIC:
        u.uevent.uhand_of_elbereth = 3;
        in_hand = (uwep && uwep->oartifact == ART_STORMBRINGER);
        already_exists =
            exist_artifact(RUNESWORD, artiname(ART_STORMBRINGER));
#if 0 /*KR: 원본*/
        verbalize("Thou art chosen to %s for My Glory!",
                  ((already_exists && !in_hand)
                   || class_gift != STRANGE_OBJECT) ? "take lives"
                                                    : "steal souls");
#else /*KR: KRNethack 맞춤 번역 */
        verbalize(
            "너는 나의 영광을 위해 %s 자로 선택되었노라!",
            ((already_exists && !in_hand) || class_gift != STRANGE_OBJECT)
                ? "목숨을 앗아갈"
                : "영혼을 훔칠");
#endif
        break;
    }

    if (objects[class_gift].oc_class == SPBOOK_CLASS) {
        obj = mksobj(class_gift, TRUE, FALSE);
        bless(obj);
        obj->bknown = 1; /* ok to skip set_bknown() */
        /*KR at_your_feet("A spellbook"); */
        at_your_feet("마법서");
        dropy(obj);
        u.ugifts++;
        /* when getting a new book for known spell, enhance
           currently wielded weapon rather than the book */
        for (sp_no = 0; sp_no < MAXSPELL; sp_no++)
            if (spl_book[sp_no].sp_id == class_gift) {
                if (ok_wep(uwep))
                    obj = uwep; /* to be blessed,&c */
                break;
            }
    }

    switch (u.ualign.type) {
    case A_LAWFUL:
        if (class_gift != STRANGE_OBJECT) {
            ; /* already got bonus above */
        } else if (obj && obj->otyp == LONG_SWORD && !obj->oartifact) {
            if (!Blind)
                /*KR Your("sword shines brightly for a moment."); */
                Your("검이 잠시 밝게 빛났다.");
            obj = oname(obj, artiname(ART_EXCALIBUR));
            if (obj && obj->oartifact == ART_EXCALIBUR)
                u.ugifts++;
        }
        /* acquire Excalibur's skill regardless of weapon or gift */
        unrestrict_weapon_skill(P_LONG_SWORD);
        if (obj && obj->oartifact == ART_EXCALIBUR)
            discover_artifact(ART_EXCALIBUR);
        break;
    case A_NEUTRAL:
        if (class_gift != STRANGE_OBJECT) {
            ; /* already got bonus above */
        } else if (obj && in_hand) {
            /*KR Your("%s goes snicker-snack!", xname(obj)); */
            Your("%s 서걱서걱거렸다!", append_josa(xname(obj), "이"));
            obj->dknown = TRUE;
        } else if (!already_exists) {
            obj = mksobj(LONG_SWORD, FALSE, FALSE);
            obj = oname(obj, artiname(ART_VORPAL_BLADE));
            obj->spe = 1;
            /*KR at_your_feet("A sword"); */
            at_your_feet("검");
            dropy(obj);
            u.ugifts++;
        }
        /* acquire Vorpal Blade's skill regardless of weapon or gift */
        unrestrict_weapon_skill(P_LONG_SWORD);
        if (obj && obj->oartifact == ART_VORPAL_BLADE)
            discover_artifact(ART_VORPAL_BLADE);
        break;
    case A_CHAOTIC: {
        char swordbuf[BUFSZ];

        /*KR Sprintf(swordbuf, "%s sword", hcolor(NH_BLACK)); */
        Sprintf(swordbuf, "%s 검", hcolor(NH_BLACK));
        if (class_gift != STRANGE_OBJECT) {
            ; /* already got bonus above */
        } else if (obj && in_hand) {
            /*KR Your("%s hums ominously!", swordbuf); */
            Your("%s 불길한 소리를 낸다!", append_josa(swordbuf, "이"));
            obj->dknown = TRUE;
        } else if (!already_exists) {
            obj = mksobj(RUNESWORD, FALSE, FALSE);
            obj = oname(obj, artiname(ART_STORMBRINGER));
            obj->spe = 1;
            at_your_feet(An(swordbuf));
            dropy(obj);
            u.ugifts++;
        }
        /* acquire Stormbringer's skill regardless of weapon or gift */
        unrestrict_weapon_skill(P_BROAD_SWORD);
        if (obj && obj->oartifact == ART_STORMBRINGER)
            discover_artifact(ART_STORMBRINGER);
        break;
    }
    default:
        obj = 0; /* lint */
        break;
    }

    /* enhance weapon regardless of alignment or artifact status */
    if (ok_wep(obj)) {
        bless(obj);
        obj->oeroded = obj->oeroded2 = 0;
        obj->oerodeproof = TRUE;
        obj->bknown = obj->rknown = 1; /* ok to skip set_bknown() */
        if (obj->spe < 1)
            obj->spe = 1;
        /* acquire skill in this weapon */
        unrestrict_weapon_skill(weapon_type(obj));
    } else if (class_gift == STRANGE_OBJECT) {
        /* opportunity knocked, but there was nobody home... */
        /*KR You_feel("unworthy."); */
        You("스스로가 하찮게 느껴졌다.");
    }
    update_inventory();

    /* lastly, confer an extra skill slot/credit beyond the
       up-to-29 you can get from gaining experience levels */
    add_weapon_skill(1);
    return;
}

STATIC_OVL void pleased(g_align) aligntyp g_align;
{
    /* don't use p_trouble, worst trouble may get fixed while praying */
    int trouble = in_trouble(); /* what's your worst difficulty? */
    int pat_on_head = 0, kick_on_butt;

#if 0 /*KR: 원본*/
    You_feel("that %s is %s.", align_gname(g_align),
             (u.ualign.record >= DEVOUT)
                 ? Hallucination ? "pleased as punch" : "well-pleased"
                 : (u.ualign.record >= STRIDENT)
                       ? Hallucination ? "ticklish" : "pleased"
                       : Hallucination ? "full" : "satisfied");
#else /*KR: KRNethack 맞춤 번역 */
    You(
        "%s %s.", append_josa(align_gname(g_align), "이"),
        (u.ualign.record >= DEVOUT)
            ? Hallucination ? "방방 뜨는 것 같다" : "매우 기뻐하시는 것 같다"
        : (u.ualign.record >= STRIDENT)
            ? Hallucination ? "간지러워하시는 것 같다" : "기뻐하시는 것 같다"
        : Hallucination ? "배불러하시는 것 같다"
                        : "만족하시는 것 같다");
#endif

    /* not your deity */
    if (on_altar() && p_aligntyp != u.ualign.type) {
        adjalign(-1);
        return;
    } else if (u.ualign.record < 2 && trouble <= 0)
        adjalign(1);

    /*
     * Depending on your luck & align level, the god you prayed to will:
     * - fix your worst problem if it's major;
     * - fix all your major problems;
     * - fix your worst problem if it's minor;
     * - fix all of your problems;
     * - do you a gratuitous favor.
     *
     * If you make it to the last category, you roll randomly again
     * to see what they do for you.
     *
     * If your luck is at least 0, then you are guaranteed rescued from
     * your worst major problem.
     */
    if (!trouble && u.ualign.record >= DEVOUT) {
        /* if hero was in trouble, but got better, no special favor */
        if (p_trouble == 0)
            pat_on_head = 1;
    } else {
        int action, prayer_luck;
        int tryct = 0;

        /* Negative luck is normally impossible here (can_pray() forces
           prayer failure in that situation), but it's possible for
           Luck to drop during the period of prayer occupation and
           become negative by the time we get here.  [Reported case
           was lawful character whose stinking cloud caused a delayed
           killing of a peaceful human, triggering the "murderer"
           penalty while successful prayer was in progress.  It could
           also happen due to inconvenient timing on Friday 13th, but
           the magnitude there (-1) isn't big enough to cause trouble.]
           We don't bother remembering start-of-prayer luck, just make
           sure it's at least -1 so that Luck+2 is big enough to avoid
           a divide by zero crash when generating a random number.  */
        prayer_luck = max(Luck, -1); /* => (prayer_luck + 2 > 0) */
        action = rn1(prayer_luck + (on_altar() ? 3 + on_shrine() : 2), 1);
        if (!on_altar())
            action = min(action, 3);
        if (u.ualign.record < STRIDENT)
            action = (u.ualign.record > 0 || !rnl(2)) ? 1 : 0;

        switch (min(action, 5)) {
        case 5:
            pat_on_head = 1;
            /*FALLTHRU*/
        case 4:
            do
                fix_worst_trouble(trouble);
            while ((trouble = in_trouble()) != 0);
            break;

        case 3:
            fix_worst_trouble(trouble);
        case 2:
            /* arbitrary number of tries */
            while ((trouble = in_trouble()) > 0 && (++tryct < 10))
                fix_worst_trouble(trouble);
            break;

        case 1:
            if (trouble > 0)
                fix_worst_trouble(trouble);
        case 0:
            break; /* your god blows you off, too bad */
        }
    }

    /* note: can't get pat_on_head unless all troubles have just been
       fixed or there were no troubles to begin with; hallucination
       won't be in effect so special handling for it is superfluous */
    if (pat_on_head)
        switch (rn2((Luck + 6) >> 1)) {
        case 0:
            break;
        case 1:
            if (uwep
                && (welded(uwep) || uwep->oclass == WEAPON_CLASS
                    || is_weptool(uwep))) {
                char repair_buf[BUFSZ];

                *repair_buf = '\0';
                if (uwep->oeroded || uwep->oeroded2)
#if 0 /*KR: 원본*/
                    Sprintf(repair_buf, " and %s now as good as new",
                            otense(uwep, "are"));
#else /*KR: KRNethack 맞춤 번역 */
                    Sprintf(repair_buf, ", 그리고 이제 새것처럼 완벽해졌다.");
#endif

                if (uwep->cursed) {
                    if (!Blind) {
#if 0 /*KR: 원본*/
                        pline("%s %s%s.", Yobjnam2(uwep, "softly glow"),
                              hcolor(NH_AMBER), repair_buf);
#else /*KR: KRNethack 맞춤 번역 */
                        pline("%s %s으로 은은하게 빛났다%s.",
                              append_josa(Yname2(uwep), "이"),
                              hcolor(NH_AMBER), repair_buf);
#endif
                        iflags.last_msg = PLNMSG_OBJ_GLOWS;
                    } else
#if 0 /*KR: 원본*/
                        You_feel("the power of %s over %s.", u_gname(),
                                 yname(uwep));
#else /*KR: KRNethack 맞춤 번역 */
                        You("%s의 힘이 %s 깃드는 것을 느낀다.",
                                 u_gname(), append_josa(yname(uwep), "에"));
#endif
                    uncurse(uwep);
                    uwep->bknown = 1; /* ok to bypass set_bknown() */
                    *repair_buf = '\0';
                } else if (!uwep->blessed) {
                    if (!Blind) {
#if 0 /*KR: 원본*/
                        pline("%s with %s aura%s.",
                              Yobjnam2(uwep, "softly glow"),
                              an(hcolor(NH_LIGHT_BLUE)), repair_buf);
#else /*KR: KRNethack 맞춤 번역 */
                        pline("%s %s 오라와 함께 은은하게 빛났다%s.",
                              append_josa(Yname2(uwep), "이"),
                              hcolor(NH_LIGHT_BLUE), repair_buf);
#endif
                        iflags.last_msg = PLNMSG_OBJ_GLOWS;
                    } else
#if 0 /*KR: 원본*/
                        You_feel("the blessing of %s over %s.", u_gname(),
                                 yname(uwep));
#else /*KR: KRNethack 맞춤 번역 */
                        You("%s의 축복이 %s 깃드는 것을 느낀다.",
                                 u_gname(), append_josa(yname(uwep), "에"));
#endif
                    bless(uwep);
                    uwep->bknown = 1; /* ok to bypass set_bknown() */
                    *repair_buf = '\0';
                }

                /* fix any rust/burn/rot damage, but don't protect
                   against future damage */
                if (uwep->oeroded || uwep->oeroded2) {
                    uwep->oeroded = uwep->oeroded2 = 0;
                    /* only give this message if we didn't just bless
                       or uncurse (which has already given a message) */
                    if (*repair_buf)
#if 0 /*KR: 원본*/
                        pline("%s as good as new!",
                              Yobjnam2(uwep, Blind ? "feel" : "look"));
#else /*KR: KRNethack 맞춤 번역 */
                        Your("%s 새것처럼 %s!",
                             append_josa(xname(uwep), "이"),
                             Blind ? "느껴진다" : "보인다");
#endif
                }
                update_inventory();
            }
            break;
        case 3:
            /* takes 2 hints to get the music to enter the stronghold;
               skip if you've solved it via mastermind or destroyed the
               drawbridge (both set uopened_dbridge) or if you've already
               travelled past the Valley of the Dead (gehennom_entered) */
            if (!u.uevent.uopened_dbridge && !u.uevent.gehennom_entered) {
                if (u.uevent.uheard_tune < 1) {
                    godvoice(g_align, (char *) 0);
#if 0 /*KR: 원본*/
                    verbalize("Hark, %s!", youmonst.data->mlet == S_HUMAN
                                               ? "mortal"
                                               : "creature");
#else /*KR: KRNethack 맞춤 번역 */
                    verbalize("들어라, %s!", youmonst.data->mlet == S_HUMAN
                                                 ? "필멸자여"
                                                 : "피조물이여");
#endif
                    /*KR verbalize(
                       "To enter the castle, thou must play the right tune!");
                     */
                    verbalize(
                        "성에 들어가려면 올바른 곡조를 연주해야 하느니라!");
                    u.uevent.uheard_tune++;
                    break;
                } else if (u.uevent.uheard_tune < 2) {
                    /*KR You_hear("a divine music..."); */
                    You("성가를 들었다...");
                    /*KR pline("It sounds like:  \"%s\".", tune); */
                    pline("다음과 같이 들린다:  \"%s\".", tune);
                    u.uevent.uheard_tune++;
                    break;
                }
            }
            /*FALLTHRU*/
        case 2:
            if (!Blind)
                /*KR You("are surrounded by %s glow.", an(hcolor(NH_GOLDEN)));
                 */
                You("%s 빛에 휩싸였다.", hcolor(NH_GOLDEN));
            /* if any levels have been lost (and not yet regained),
               treat this effect like blessed full healing */
            if (u.ulevel < u.ulevelmax) {
                u.ulevelmax -= 1; /* see potion.c */
                pluslvl(FALSE);
            } else {
                u.uhpmax += 5;
                if (Upolyd)
                    u.mhmax += 5;
            }
            u.uhp = u.uhpmax;
            if (Upolyd)
                u.mh = u.mhmax;
            if (ABASE(A_STR) < AMAX(A_STR)) {
                ABASE(A_STR) = AMAX(A_STR);
                context.botl = 1; /* before potential message */
                (void) encumber_msg();
            }
            if (u.uhunger < 900)
                init_uhunger();
            /* luck couldn't have been negative at start of prayer because
               the prayer would have failed, but might have been decremented
               due to a timed event (delayed death of peaceful monster hit
               by hero-created stinking cloud) during the praying interval */
            if (u.uluck < 0)
                u.uluck = 0;
            /* superfluous; if hero was blinded we'd be handling trouble
               rather than issuing a pat-on-head */
            u.ucreamed = 0;
            make_blinded(0L, TRUE);
            context.botl = 1;
            break;
        case 4: {
            register struct obj *otmp;
            int any = 0;

            if (Blind)
                /*KR You_feel("the power of %s.", u_gname()); */
                You("%s의 권능이 느껴진다.", u_gname());
            else
                /*KR You("are surrounded by %s aura.",
                 * an(hcolor(NH_LIGHT_BLUE))); */
                You("%s 오라에 휩싸였다.", hcolor(NH_LIGHT_BLUE));
            for (otmp = invent; otmp; otmp = otmp->nobj) {
                if (otmp->cursed
                    && (otmp != uarmh /* [see worst_cursed_item()] */
                        || uarmh->otyp != HELM_OF_OPPOSITE_ALIGNMENT)) {
                    if (!Blind) {
#if 0 /*KR: 원본*/
                        pline("%s %s.", Yobjnam2(otmp, "softly glow"),
                              hcolor(NH_AMBER));
#else /*KR: KRNethack 맞춤 번역 */
                        pline("%s %s으로 은은하게 빛났다.",
                              append_josa(Yname2(otmp), "이"),
                              hcolor(NH_AMBER));
#endif
                        iflags.last_msg = PLNMSG_OBJ_GLOWS;
                        otmp->bknown = 1; /* ok to bypass set_bknown() */
                        ++any;
                    }
                    uncurse(otmp);
                }
            }
            if (any)
                update_inventory();
            break;
        }
        case 5: {
            /*KR static NEARDATA const char msg[] =
                "\"and thus I grant thee the gift of %s!\""; */
            static NEARDATA const char msg[] =
                "\"그리하여 네게 %s의 선물을 내리노라!\"";

            /*KR godvoice(u.ualign.type,
                     "Thou hast pleased me with thy progress,"); */
            godvoice(u.ualign.type, "너의 성장이 나를 기쁘게 하는구나,");
            if (!(HTelepat & INTRINSIC)) {
                HTelepat |= FROMOUTSIDE;
                /*KR pline(msg, "Telepathy"); */
                pline(msg, "텔레파시");
                if (Blind)
                    see_monsters();
            } else if (!(HFast & INTRINSIC)) {
                HFast |= FROMOUTSIDE;
                /*KR pline(msg, "Speed"); */
                pline(msg, "속도");
            } else if (!(HStealth & INTRINSIC)) {
                HStealth |= FROMOUTSIDE;
                /*KR pline(msg, "Stealth"); */
                pline(msg, "은신");
            } else {
                if (!(HProtection & INTRINSIC)) {
                    HProtection |= FROMOUTSIDE;
                    if (!u.ublessed)
                        u.ublessed = rn1(3, 2);
                } else
                    u.ublessed++;
                /*KR pline(msg, "my protection"); */
                pline(msg, "나의 가호");
            }
            /*KR verbalize("Use it wisely in my name!"); */
            verbalize("내 이름으로 지혜롭게 사용하거라!");
            break;
        }
        case 7:
        case 8:
            if (u.ualign.record >= PIOUS && !u.uevent.uhand_of_elbereth) {
                gcrownu();
                break;
            }
            /*FALLTHRU*/
        case 6: {
            struct obj *otmp;
            int sp_no, trycnt = u.ulevel + 1;

            /* not yet known spells given preference over already known ones
             */
            /* Also, try to grant a spell for which there is a skill slot */
            otmp = mkobj(SPBOOK_CLASS, TRUE);
            while (--trycnt > 0) {
                if (otmp->otyp != SPE_BLANK_PAPER) {
                    for (sp_no = 0; sp_no < MAXSPELL; sp_no++)
                        if (spl_book[sp_no].sp_id == otmp->otyp)
                            break;
                    if (sp_no == MAXSPELL
                        && !P_RESTRICTED(spell_skilltype(otmp->otyp)))
                        break; /* usable, but not yet known */
                } else {
                    if (!objects[SPE_BLANK_PAPER].oc_name_known
                        || carrying(MAGIC_MARKER))
                        break;
                }
                otmp->otyp = rnd_class(bases[SPBOOK_CLASS], SPE_BLANK_PAPER);
            }
            bless(otmp);
            /*KR at_your_feet("A spellbook"); */
            at_your_feet("마법서");
            place_object(otmp, u.ux, u.uy);
            newsym(u.ux, u.uy);
            break;
        }
        default:
            impossible("Confused deity!");
            break;
        }

    u.ublesscnt = rnz(350);
    kick_on_butt = u.uevent.udemigod ? 1 : 0;
    if (u.uevent.uhand_of_elbereth)
        kick_on_butt++;
    if (kick_on_butt)
        u.ublesscnt += kick_on_butt * rnz(1000);

    return;
}

/* either blesses or curses water on the altar,
 * returns true if it found any water here.
 */
STATIC_OVL boolean
water_prayer(bless_water)
boolean bless_water;
{
    register struct obj *otmp;
    register long changed = 0;
    boolean other = FALSE, bc_known = !(Blind || Hallucination);

    for (otmp = level.objects[u.ux][u.uy]; otmp; otmp = otmp->nexthere) {
        /* turn water into (un)holy water */
        if (otmp->otyp == POT_WATER
            && (bless_water ? !otmp->blessed : !otmp->cursed)) {
            otmp->blessed = bless_water;
            otmp->cursed = !bless_water;
            otmp->bknown = bc_known; /* ok to bypass set_bknown() */
            changed += otmp->quan;
        } else if (otmp->oclass == POTION_CLASS)
            other = TRUE;
    }
    if (!Blind && changed) {
#if 0 /*KR: 원본*/
        pline("%s potion%s on the altar glow%s %s for a moment.",
              ((other && changed > 1L) ? "Some of the"
                                       : (other ? "One of the" : "The")),
              ((other || changed > 1L) ? "s" : ""), (changed > 1L ? "" : "s"),
              (bless_water ? hcolor(NH_LIGHT_BLUE) : hcolor(NH_BLACK)));
#else /*KR: KRNethack 맞춤 번역 */
        pline("제단 위의 물약%s 잠시 %s으로 빛났다.",
              (other && changed > 1L) ? " 중 일부가"
                                      : (other ? " 중 하나가" : "이"),
              (bless_water ? hcolor(NH_LIGHT_BLUE) : hcolor(NH_BLACK)));
#endif
    }
    return (boolean) (changed > 0L);
}

STATIC_OVL void godvoice(g_align, words) aligntyp g_align;
const char *words;
{
#if 0 /*KR: 원본*/
    const char *quot = "";

    if (words)
        quot = "\"";
    else
        words = "";

    pline_The("voice of %s %s: %s%s%s", align_gname(g_align),
              godvoices[rn2(SIZE(godvoices))], quot, words, quot);
#else /*KR: KRNethack 맞춤 번역 */
    if (words)
        pline("%s의 목소리가 %s: \"%s\"", align_gname(g_align),
              godvoices[rn2(SIZE(godvoices))], words);
    else
        pline("%s의 목소리가 %s:", align_gname(g_align),
              godvoices[rn2(SIZE(godvoices))]);
#endif
}

STATIC_OVL void gods_angry(g_align) aligntyp g_align;
{
    /*KR godvoice(g_align, "Thou hast angered me."); */
    godvoice(g_align, "네가 나를 분노하게 하였노라.");
}

/* The g_align god is upset with you. */
STATIC_OVL void gods_upset(g_align) aligntyp g_align;
{
    if (g_align == u.ualign.type)
        u.ugangr++;
    else if (u.ugangr)
        u.ugangr--;
    angrygods(g_align);
}

STATIC_OVL void consume_offering(otmp) register struct obj *otmp;
{
    if (Hallucination)
        switch (rn2(3)) {
        case 0:
            /*KR Your("sacrifice sprouts wings and a propeller and roars
             * away!"); */
            Your("제물에 날개와 프로펠러가 돋아나더니 굉음을 내며 "
                 "날아가버렸다!");
            break;
        case 1:
            /*KR Your("sacrifice puffs up, swelling bigger and bigger, and
             * pops!"); */
            Your("제물이 부풀어오르더니, 점점 커지다가 펑 하고 터져버렸다!");
            break;
        case 2:
            /*KR Your("sacrifice collapses into a cloud of dancing particles
             * and fades away!"); */
            Your("제물이 춤추는 입자구름으로 무너지며 스르르 사라졌다!");
            break;
        }
    else if (Blind && u.ualign.type == A_LAWFUL)
        /*KR Your("sacrifice disappears!"); */
        Your("제물이 사라졌다!");
    else
#if 0 /*KR: 원본*/
        Your("sacrifice is consumed in a %s!",
             u.ualign.type == A_LAWFUL ? "flash of light" : "burst of flame");
#else /*KR: KRNethack 맞춤 번역 */
        Your("제물은 %s 속에서 소멸했다!",
             u.ualign.type == A_LAWFUL ? "번쩍이는 빛" : "타오르는 불꽃");
#endif
    if (carried(otmp))
        useup(otmp);
    else
        useupf(otmp, 1L);
    exercise(A_WIS, TRUE);
}

int
dosacrifice()
{
    /*KR static NEARDATA const char cloud_of_smoke[] =
        "A cloud of %s smoke surrounds you..."; */
    static NEARDATA const char cloud_of_smoke[] =
        "%s 연기 구름이 당신을 둘러싼다...";
    register struct obj *otmp;
    int value = 0, pm;
    boolean highaltar;
    aligntyp altaralign = a_align(u.ux, u.uy);

    if (!on_altar() || u.uswallow) {
        /*KR You("are not standing on an altar."); */
        You("제단 위에 서 있지 않다.");
        return 0;
    }
    highaltar = ((Is_astralevel(&u.uz) || Is_sanctum(&u.uz))
                 && (levl[u.ux][u.uy].altarmask & AM_SHRINE));

    otmp = floorfood("sacrifice", 1);
    if (!otmp)
        return 0;
    /*
     * Was based on nutritional value and aging behavior (< 50 moves).
     * Sacrificing a food ration got you max luck instantly, making the
     * gods as easy to please as an angry dog!
     *
     * Now only accepts corpses, based on the game's evaluation of their
     * toughness.  Human and pet sacrifice, as well as sacrificing unicorns
     * of your alignment, is strongly discouraged.
     */
#define MAXVALUE 24 /* Highest corpse value (besides Wiz) */

    if (otmp->otyp == CORPSE) {
        register struct permonst *ptr = &mons[otmp->corpsenm];
        struct monst *mtmp;

        /* KMH, conduct */
        u.uconduct.gnostic++;

        /* you're handling this corpse, even if it was killed upon the altar
         */
        feel_cockatrice(otmp, TRUE);
        if (rider_corpse_revival(otmp, FALSE))
            return 1;

        if (otmp->corpsenm == PM_ACID_BLOB
            || (monstermoves <= peek_at_iced_corpse_age(otmp) + 50)) {
            value = mons[otmp->corpsenm].difficulty + 1;
            if (otmp->oeaten)
                value = eaten_stat(value, otmp);
        }

        if (your_race(ptr)) {
            if (is_demon(youmonst.data)) {
                /*KR You("find the idea very satisfying."); */
                You("그 아이디어가 무척 만족스럽다.");
                exercise(A_WIS, TRUE);
            } else if (u.ualign.type != A_CHAOTIC) {
                /*KR pline("You'll regret this infamous offense!"); */
                pline("너는 이 끔찍한 모독을 후회하게 될 것이다!");
                exercise(A_WIS, FALSE);
            }

            if (highaltar
                && (altaralign != A_CHAOTIC || u.ualign.type != A_CHAOTIC)) {
                goto desecrate_high_altar;
            } else if (altaralign != A_CHAOTIC && altaralign != A_NONE) {
                /* curse the lawful/neutral altar */
                /*KR pline_The("altar is stained with %s blood.", urace.adj);
                 */
                pline("제단은 %s 피로 얼룩졌다.", urace.adj);
                levl[u.ux][u.uy].altarmask = AM_CHAOTIC;
                angry_priest();
            } else {
                struct monst *dmon;
                const char *demonless_msg;

                /* Human sacrifice on a chaotic or unaligned altar */
                /* is equivalent to demon summoning */
                if (altaralign == A_CHAOTIC && u.ualign.type != A_CHAOTIC) {
#if 0 /*KR: 원본*/
                    pline(
                    "The blood floods the altar, which vanishes in %s cloud!",
                          an(hcolor(NH_BLACK)));
#else /*KR: KRNethack 맞춤 번역 */
                    pline("피가 제단에 넘쳐흐르더니, 제단은 %s 구름 속으로 "
                          "사라졌다!",
                          hcolor(NH_BLACK));
#endif
                    levl[u.ux][u.uy].typ = ROOM;
                    levl[u.ux][u.uy].altarmask = 0;
                    newsym(u.ux, u.uy);
                    angry_priest();
                    /*KR demonless_msg = "cloud dissipates"; */
                    demonless_msg = "구름이 흩어졌다";
                } else {
                    /* either you're chaotic or altar is Moloch's or both */
                    /*KR pline_The("blood covers the altar!"); */
                    pline("피가 제단을 뒤덮었다!");
                    change_luck(altaralign == A_NONE ? -2 : 2);
                    /*KR demonless_msg = "blood coagulates"; */
                    demonless_msg = "피가 응고되었다";
                }
                if ((pm = dlord(altaralign)) != NON_PM
                    && (dmon = makemon(&mons[pm], u.ux, u.uy, NO_MM_FLAGS))
                           != 0) {
                    char dbuf[BUFSZ];

                    Strcpy(dbuf, a_monnam(dmon));
                    /*KR if (!strcmpi(dbuf, "it")) */
                    if (!strcmpi(dbuf, "그것"))
                        /*KR Strcpy(dbuf, "something dreadful"); */
                        Strcpy(dbuf, "무언가 끔찍한 것");
                    else
                        dmon->mstrategy &= ~STRAT_APPEARMSG;
                    /*KR You("have summoned %s!", dbuf); */
                    You("%s 소환했다!", append_josa(dbuf, "을"));
                    if (sgn(u.ualign.type) == sgn(dmon->data->maligntyp))
                        dmon->mpeaceful = TRUE;
                    /*KR You("are terrified, and unable to move."); */
                    You("너무 무서워서 움직일 수가 없다.");
                    nomul(-3);
                    /*KR multi_reason = "being terrified of a demon"; */
                    multi_reason = "악마에 대한 두려움에 빠져";
                    nomovemsg = 0;
                } else
                    /*KR pline_The("%s.", demonless_msg); */
                    pline("%s.", demonless_msg);
            }

            if (u.ualign.type != A_CHAOTIC) {
                adjalign(-5);
                u.ugangr += 3;
                (void) adjattrib(A_WIS, -1, TRUE);
                if (!Inhell)
                    angrygods(u.ualign.type);
                change_luck(-5);
            } else
                adjalign(5);
            if (carried(otmp))
                useup(otmp);
            else
                useupf(otmp, 1L);
            return 1;
        } else if (has_omonst(otmp) && (mtmp = get_mtraits(otmp, FALSE)) != 0
                   && mtmp->mtame) {
            /* mtmp is a temporary pointer to a tame monster's attributes,
             * not a real monster */
            /*KR pline("So this is how you repay loyalty?"); */
            pline("이것이 네가 충성에 보답하는 방식이냐?");
            adjalign(-3);
            value = -1;
            HAggravate_monster |= FROMOUTSIDE;
        } else if (is_undead(ptr)) { /* Not demons--no demon corpses */
            if (u.ualign.type != A_CHAOTIC)
                value += 1;
        } else if (is_unicorn(ptr)) {
            int unicalign = sgn(ptr->maligntyp);

            if (unicalign == altaralign) {
                /* When same as altar, always a very bad action.
                 */
#if 0 /*KR: 원본*/
                pline("Such an action is an insult to %s!",
                      (unicalign == A_CHAOTIC) ? "chaos"
                         : unicalign ? "law" : "balance");
#else /*KR: KRNethack 맞춤 번역 */
                pline("그런 행동은 %s에 대한 모욕이다!",
                      (unicalign == A_CHAOTIC) ? "혼돈"
                      : unicalign              ? "질서"
                                               : "균형");
#endif
                (void) adjattrib(A_WIS, -1, TRUE);
                value = -5;
            } else if (u.ualign.type == altaralign) {
                /* When different from altar, and altar is same as yours,
                 * it's a very good action.
                 */
                if (u.ualign.record < ALIGNLIM)
                    /*KR You_feel("appropriately %s.",
                     * align_str(u.ualign.type)); */
                    You("적절히 %s된 기분이다.",
                             align_str(u.ualign.type));
                else
                    /*KR You_feel("you are thoroughly on the right path."); */
                    You("자신이 완벽하게 올바른 길을 걷고 있다는 기분이 든다.");
                adjalign(5);
                value += 3;
            } else if (unicalign == u.ualign.type) {
                /* When sacrificing unicorn of your alignment to altar not of
                 * your alignment, your god gets angry and it's a conversion.
                 */
                u.ualign.record = -1;
                value = 1;
            } else {
                /* Otherwise, unicorn's alignment is different from yours
                 * and different from the altar's.  It's an ordinary (well,
                 * with a bonus) sacrifice on a cross-aligned altar.
                 */
                value += 3;
            }
        }
    } /* corpse */

    if (otmp->otyp == AMULET_OF_YENDOR) {
        if (!highaltar) {
        too_soon:
            if (altaralign == A_NONE && Inhell)
                /* hero has left Moloch's Sanctum so is in the process
                   of getting away with the Amulet (outside of Gehennom,
                   fall through to the "ashamed" feedback) */
                gods_upset(A_NONE);
            else
#if 0 /*KR: 원본*/
                You_feel("%s.",
                         Hallucination
                            ? "homesick"
                            /* if on track, give a big hint */
                            : (altaralign == u.ualign.type)
                               ? "an urge to return to the surface"
                               /* else headed towards celestial disgrace */
                               : "ashamed");
#else /*KR: KRNethack 맞춤 번역 */
                You("%s.",
                         Hallucination ? "향수병이 도지는 것 같다"
                         /* if on track, give a big hint */
                         : (altaralign == u.ualign.type)
                             ? "지상으로 돌아가고픈 충동이 인다"
                             /* else headed towards celestial disgrace */
                             : "수치심이 든다");
#endif
            return 1;
        } else {
            /* The final Test.  Did you win? */
            if (uamul == otmp)
                Amulet_off();
            u.uevent.ascended = 1;
            if (carried(otmp))
                useup(otmp); /* well, it's gone now */
            else
                useupf(otmp, 1L);
            /*KR You("offer the Amulet of Yendor to %s...", a_gname()); */
            You("%s에게 옌더의 부적을 바쳤다...", a_gname());
            if (altaralign == A_NONE) {
                /* Moloch's high altar */
                if (u.ualign.record > -99)
                    u.ualign.record = -99;
                /*[apparently shrug/snarl can be sensed without being seen]*/
#if 0 /*KR: 원본*/
                pline("%s shrugs and retains dominion over %s,", Moloch,
                      u_gname());
#else /*KR: KRNethack 맞춤 번역 */
                pline("%s 어깨를 으쓱하며 %s에 대한 지배권을 유지했고,",
                      append_josa(Moloch, "은"), u_gname());
#endif
                /*KR pline("then mercilessly snuffs out your life."); */
                pline("이내 무자비하게 당신의 목숨을 거두어 갔다.");
#if 0 /*KR: 원본*/
                Sprintf(killer.name, "%s indifference", s_suffix(Moloch));
#else /*KR: KRNethack 맞춤 번역 */
                Sprintf(killer.name, "%s 무관심", s_suffix(Moloch));
#endif
                killer.format = KILLED_BY;
                done(DIED);
                /* life-saved (or declined to die in wizard/explore mode) */
                /*KR pline("%s snarls and tries again...", Moloch); */
                pline("%s 으르렁거리며 다시 시도했다...",
                      append_josa(Moloch, "은"));
                fry_by_god(A_NONE, TRUE); /* wrath of Moloch */
                /* declined to die in wizard or explore mode */
                pline(cloud_of_smoke, hcolor(NH_BLACK));
                done(ESCAPED);
            } else if (u.ualign.type != altaralign) {
                /* And the opposing team picks you up and
                   carries you off on their shoulders */
                adjalign(-99);
#if 0 /*KR: 원본*/
                pline("%s accepts your gift, and gains dominion over %s...",
                      a_gname(), u_gname());
#else /*KR: KRNethack 맞춤 번역 */
                pline("%s 당신의 선물을 받아들이고, %s에 대한 지배권을 "
                      "얻었다...",
                      append_josa(a_gname(), "은"), u_gname());
#endif
                /*KR pline("%s is enraged...", u_gname()); */
                pline("%s 격노했다...", append_josa(u_gname(), "은"));
                /*KR pline("Fortunately, %s permits you to live...",
                 * a_gname()); */
                pline("다행히도, %s 당신을 살려주었다...",
                      append_josa(a_gname(), "은"));
                pline(cloud_of_smoke, hcolor(NH_ORANGE));
                done(ESCAPED);
            } else { /* super big win */
                adjalign(10);
                u.uachieve.ascended = 1;
                /*KR pline(
               "An invisible choir sings, and you are bathed in radiance...");
             */
                pline("보이지 않는 성가대의 노랫소리가 울려퍼지며, 당신은 "
                      "눈부신 광휘에 휩싸였다...");
                /*KR godvoice(altaralign, "Mortal, thou hast done well!"); */
                godvoice(altaralign, "필멸자여, 너는 참으로 잘 해내었도다!");
                display_nhwindow(WIN_MESSAGE, FALSE);
                /*KR verbalize(
          "In return for thy service, I grant thee the gift of Immortality!");
        */
                verbalize(
                    "너의 봉사에 대한 보답으로, 영생의 선물을 하사하노라!");
#if 0 /*KR: 원본*/
                You("ascend to the status of Demigod%s...",
                    flags.female ? "dess" : "");
#else /*KR: KRNethack 맞춤 번역 */
                You("반신%s의 지위로 승천했다...",
                    flags.female ? "(여신)" : "");
#endif
                done(ASCENDED);
            }
        }
    } /* real Amulet */

    if (otmp->otyp == FAKE_AMULET_OF_YENDOR) {
        if (!highaltar && !otmp->known)
            goto too_soon;
        /*KR You_hear("a nearby thunderclap."); */
        You("근처에서 천둥 치는 소리를 들었다.");
        if (!otmp->known) {
#if 0 /*KR: 원본*/
            You("realize you have made a %s.",
                Hallucination ? "boo-boo" : "mistake");
#else /*KR: KRNethack 맞춤 번역 */
            You("당신이 %s 깨달았다.", Hallucination
                                           ? "실수해쪄염이라는 걸"
                                           : "실수를 저질렀다는 것을");
#endif
            otmp->known = TRUE;
            change_luck(-1);
            return 1;
        } else {
            /* don't you dare try to fool the gods */
            if (Deaf)
#if 0                               /*KR: 원본*/
                pline("Oh, no."); /* didn't hear thunderclap */
#else                               /*KR: KRNethack 맞춤 번역 */
                pline("오, 안돼."); /* didn't hear thunderclap */
#endif
            change_luck(-3);
            adjalign(-1);
            u.ugangr += 3;
            value = -3;
        }
    } /* fake Amulet */

    if (value == 0) {
        pline1(nothing_happens);
        return 1;
    }

    if (altaralign != u.ualign.type && highaltar) {
    desecrate_high_altar:
        /*
         * REAL BAD NEWS!!! High altars cannot be converted.  Even an attempt
         * gets the god who owns it truly pissed off.
         */
        /*KR You_feel("the air around you grow charged..."); */
        You("주변의 공기가 긴장감으로 팽팽해지는 것을 느낀다...");
        /*KR pline("Suddenly, you realize that %s has noticed you...",
         * a_gname()); */
        pline("갑자기, 당신은 %s 당신을 알아차렸음을 깨달았다...",
              append_josa(a_gname(), "이"));
        /*KR godvoice(altaralign,
                 "So, mortal!  You dare desecrate my High Temple!"); */
        godvoice(altaralign,
                 "그래, 필멸자여! 감히 나의 대성전을 더럽히다니!");
        /* Throw everything we have at the player */
        god_zaps_you(altaralign);
    } else if (value
               < 0) { /* I don't think the gods are gonna like this... */
        gods_upset(altaralign);
    } else {
        int saved_anger = u.ugangr;
        int saved_cnt = u.ublesscnt;
        int saved_luck = u.uluck;

        /* Sacrificing at an altar of a different alignment */
        if (u.ualign.type != altaralign) {
            /* Is this a conversion ? */
            /* An unaligned altar in Gehennom will always elicit rejection. */
            if (ugod_is_angry() || (altaralign == A_NONE && Inhell)) {
                if (u.ualignbase[A_CURRENT] == u.ualignbase[A_ORIGINAL]
                    && altaralign != A_NONE) {
                    /*KR You("have a strong feeling that %s is angry...",
                        u_gname()); */
                    You("%s 화가 났다는 강한 예감이 든다...",
                        append_josa(u_gname(), "이"));
                    consume_offering(otmp);
                    /*KR pline("%s accepts your allegiance.", a_gname()); */
                    pline("%s 당신의 충성을 받아들였다.",
                          append_josa(a_gname(), "은"));

                    uchangealign(altaralign, 0);
                    /* Beware, Conversion is costly */
                    change_luck(-3);
                    u.ublesscnt += 300;
                } else {
                    u.ugangr += 3;
                    adjalign(-5);
                    /*KR pline("%s rejects your sacrifice!", a_gname()); */
                    pline("%s 당신의 제물을 거부했다!",
                          append_josa(a_gname(), "은"));
                    /*KR godvoice(altaralign, "Suffer, infidel!"); */
                    godvoice(altaralign, "고통받아라, 이교도여!");
                    change_luck(-5);
                    (void) adjattrib(A_WIS, -2, TRUE);
                    if (!Inhell)
                        angrygods(u.ualign.type);
                }
                return 1;
            } else {
                consume_offering(otmp);
#if 0 /*KR: 원본*/
                You("sense a conflict between %s and %s.", u_gname(),
                    a_gname());
#else /*KR: KRNethack 맞춤 번역 */
                You("%s %s 사이에 갈등이 느껴진다.",
                    append_josa(u_gname(), "와(과)"), a_gname());
#endif
                if (rn2(8 + u.ulevel) > 5) {
                    struct monst *pri;
                    /*KR You_feel("the power of %s increase.", u_gname()); */
                    You("%s의 힘이 증가하는 것을 느낀다.", u_gname());
                    exercise(A_WIS, TRUE);
                    change_luck(1);
                    /* Yes, this is supposed to be &=, not |= */
                    levl[u.ux][u.uy].altarmask &= AM_SHRINE;
                    /* the following accommodates stupid compilers */
                    levl[u.ux][u.uy].altarmask =
                        levl[u.ux][u.uy].altarmask
                        | (Align2amask(u.ualign.type));
                    if (!Blind)
#if 0 /*KR: 원본*/
                        pline_The("altar glows %s.",
                                  hcolor((u.ualign.type == A_LAWFUL)
                                            ? NH_WHITE
                                            : u.ualign.type
                                               ? NH_BLACK
                                               : (const char *) "gray"));
#else /*KR: KRNethack 맞춤 번역 */
                        pline("제단이 %s 빛난다.",
                              hcolor((u.ualign.type == A_LAWFUL)
                                             ? NH_WHITE
                                         : u.ualign.type
                                             ? NH_BLACK
                                             : (const char *) "회색으로"));
#endif

                    if (rnl(u.ulevel) > 6 && u.ualign.record > 0
                        && rnd(u.ualign.record) > (3 * ALIGNLIM) / 4)
                        summon_minion(altaralign, TRUE);
                    /* anger priest; test handles bones files */
                    if ((pri = findpriest(temple_occupied(u.urooms)))
                        && !p_coaligned(pri))
                        angry_priest();
                } else {
                    /*KR pline("Unluckily, you feel the power of %s
                       decrease.", u_gname()); */
                    pline(
                        "불운하게도, 당신은 %s의 힘이 감소하는 것을 느낀다.",
                        u_gname());
                    change_luck(-1);
                    exercise(A_WIS, FALSE);
                    if (rnl(u.ulevel) > 6 && u.ualign.record > 0
                        && rnd(u.ualign.record) > (7 * ALIGNLIM) / 8)
                        summon_minion(altaralign, TRUE);
                }
                return 1;
            }
        }

        consume_offering(otmp);
        /* OK, you get brownie points. */
        if (u.ugangr) {
            u.ugangr -=
                ((value * (u.ualign.type == A_CHAOTIC ? 2 : 3)) / MAXVALUE);
            if (u.ugangr < 0)
                u.ugangr = 0;
            if (u.ugangr != saved_anger) {
                if (u.ugangr) {
#if 0 /*KR: 원본*/
                    pline("%s seems %s.", u_gname(),
                          Hallucination ? "groovy" : "slightly mollified");
#else /*KR: KRNethack 맞춤 번역 */
                    pline("%s %s 보인다.", append_josa(u_gname(), "은"),
                          Hallucination ? "근사해" : "조금 누그러진 것 같아");
#endif

                    if ((int) u.uluck < 0)
                        change_luck(1);
                } else {
#if 0 /*KR: 원본*/
                    pline("%s seems %s.", u_gname(),
                          Hallucination ? "cosmic (not a new fact)"
                                        : "mollified");
#else /*KR: KRNethack 맞춤 번역 */
                    pline("%s %s 보인다.", append_josa(u_gname(), "은"),
                          Hallucination
                              ? "우주적인 것 같아 (새로운 사실은 아님)"
                              : "분노가 풀린 것 같아");
#endif

                    if ((int) u.uluck < 0)
                        u.uluck = 0;
                }
            } else { /* not satisfied yet */
                if (Hallucination)
                    /*KR pline_The("gods seem tall."); */
                    pline("신들이 꽤 커 보인다.");
                else
                    /*KR You("have a feeling of inadequacy."); */
                    You("스스로가 부족하다고 느낀다.");
            }
        } else if (ugod_is_angry()) {
            if (value > MAXVALUE)
                value = MAXVALUE;
            if (value > -u.ualign.record)
                value = -u.ualign.record;
            adjalign(value);
            /*KR You_feel("partially absolved."); */
            You("죄를 부분적으로 용서받은 기분이다.");
        } else if (u.ublesscnt > 0) {
            u.ublesscnt -= ((value * (u.ualign.type == A_CHAOTIC ? 500 : 300))
                            / MAXVALUE);
            if (u.ublesscnt < 0)
                u.ublesscnt = 0;
            if (u.ublesscnt != saved_cnt) {
                if (u.ublesscnt) {
                    if (Hallucination)
                        /*KR You("realize that the gods are not like you and
                         * I."); */
                        You("신들이 당신과 나 같은 존재가 아님을 깨달았다.");
                    else
                        /*KR You("have a hopeful feeling."); */
                        You("희망찬 기분이 든다.");
                    if ((int) u.uluck < 0)
                        change_luck(1);
                } else {
                    if (Hallucination)
                        /*KR pline("Overall, there is a smell of fried
                         * onions."); */
                        pline("주변에서 튀긴 양파 냄새가 난다.");
                    else
                        /*KR You("have a feeling of reconciliation."); */
                        You("화해한 기분이 든다.");
                    if ((int) u.uluck < 0)
                        u.uluck = 0;
                }
            }
        } else {
            int nartifacts = nartifact_exist();

            /* you were already in pretty good standing */
            /* The player can gain an artifact */
            /* The chance goes down as the number of artifacts goes up */
            if (u.ulevel > 2 && u.uluck >= 0
                && !rn2(10 + (2 * u.ugifts * nartifacts))) {
                otmp = mk_artifact((struct obj *) 0, a_align(u.ux, u.uy));
                if (otmp) {
                    if (otmp->spe < 0)
                        otmp->spe = 0;
                    if (otmp->cursed)
                        uncurse(otmp);
                    otmp->oerodeproof = TRUE;
                    /*KR at_your_feet("An object"); */
                    at_your_feet("물건");
                    dropy(otmp);
                    /*KR godvoice(u.ualign.type, "Use my gift wisely!"); */
                    godvoice(u.ualign.type,
                             "나의 선물을 지혜롭게 사용하거라!");
                    u.ugifts++;
                    u.ublesscnt = rnz(300 + (50 * nartifacts));
                    exercise(A_WIS, TRUE);
                    /* make sure we can use this weapon */
                    unrestrict_weapon_skill(weapon_type(otmp));
                    if (!Hallucination && !Blind) {
                        otmp->dknown = 1;
                        makeknown(otmp->otyp);
                        discover_artifact(otmp->oartifact);
                    }
                    return 1;
                }
            }
            change_luck((value * LUCKMAX) / (MAXVALUE * 2));
            if ((int) u.uluck < 0)
                u.uluck = 0;
            if (u.uluck != saved_luck) {
                if (Blind)
#if 0 /*KR: 원본*/
                    You("think %s brushed your %s.", something,
                        body_part(FOOT));
#else /*KR: KRNethack 맞춤 번역 */
                    You("%s 당신의 %s 스치고 지나갔다고 생각한다.",
                        append_josa(something, "이"), body_part(FOOT));
#endif
                else
#if 0 /*KR: 원본*/
                    You(Hallucination
                    ? "see crabgrass at your %s.  A funny thing in a dungeon."
                            : "glimpse a four-leaf clover at your %s.",
                        makeplural(body_part(FOOT)));
#else /*KR: KRNethack 맞춤 번역 */
                    You(Hallucination
                            ? "당신의 %s 바랭이풀을 보았다. 던전치고는 "
                              "우스운 일이다."
                            : "당신의 %s 네잎 클로버를 흘끗 보았다.",
                        makeplural(body_part(FOOT)));
#endif
            }
        }
    }
    return 1;
}

/* determine prayer results in advance; also used for enlightenment */
boolean
can_pray(praying)
boolean praying; /* false means no messages should be given */
{
    int alignment;

    p_aligntyp = on_altar() ? a_align(u.ux, u.uy) : u.ualign.type;
    p_trouble = in_trouble();

    if (is_demon(youmonst.data) && (p_aligntyp != A_CHAOTIC)) {
        if (praying)
#if 0 /*KR: 원본*/
            pline_The("very idea of praying to a %s god is repugnant to you.",
                      p_aligntyp ? "lawful" : "neutral");
#else /*KR: KRNethack 맞춤 번역 */
            pline("%s 신에게 기도한다는 생각 자체가 혐오스럽게 느껴진다.",
                  p_aligntyp ? "질서 성향의" : "중립 성향의");
#endif
        return FALSE;
    }

    if (praying)
        /*KR You("begin praying to %s.", align_gname(p_aligntyp)); */
        You("%s에게 기도를 시작했다.", align_gname(p_aligntyp));

    if (u.ualign.type && u.ualign.type == -p_aligntyp)
        alignment = -u.ualign.record; /* Opposite alignment altar */
    else if (u.ualign.type != p_aligntyp)
        alignment = u.ualign.record / 2; /* Different alignment altar */
    else
        alignment = u.ualign.record;

    if ((p_trouble > 0)   ? (u.ublesscnt > 200) /* big trouble */
        : (p_trouble < 0) ? (u.ublesscnt > 100) /* minor difficulties */
                          : (u.ublesscnt > 0))  /* not in trouble */
        p_type = 0;                             /* too soon... */
    else if ((int) Luck < 0 || u.ugangr || alignment < 0)
        p_type = 1; /* too naughty... */
    else /* alignment >= 0 */ {
        if (on_altar() && u.ualign.type != p_aligntyp)
            p_type = 2;
        else
            p_type = 3;
    }

    if (is_undead(youmonst.data) && !Inhell
        && (p_aligntyp == A_LAWFUL || (p_aligntyp == A_NEUTRAL && !rn2(10))))
        p_type = -1;
    /* Note:  when !praying, the random factor for neutrals makes the
       return value a non-deterministic approximation for enlightenment.
       This case should be uncommon enough to live with... */

    return !praying ? (boolean) (p_type == 3 && !Inhell) : TRUE;
}

/* #pray commmand */
int
dopray()
{
    /* Confirm accidental slips of Alt-P */
    /*KR if (ParanoidPray && yn("Are you sure you want to pray?") != 'y') */
    if (ParanoidPray && yn("정말로 기도하시겠습니까?") != 'y')
        return 0;

    u.uconduct.gnostic++;

    /* set up p_type and p_alignment */
    if (!can_pray(TRUE))
        return 0;

    if (wizard && p_type >= 0) {
        /*KR if (yn("Force the gods to be pleased?") == 'y') { */
        if (yn("신들을 강제로 기쁘게 하시겠습니까?") == 'y') {
            u.ublesscnt = 0;
            if (u.uluck < 0)
                u.uluck = 0;
            if (u.ualign.record <= 0)
                u.ualign.record = 1;
            u.ugangr = 0;
            if (p_type < 2)
                p_type = 3;
        }
    }
    nomul(-3);
    /*KR multi_reason = "praying"; */
    multi_reason = "기도하느라";
    /*KR nomovemsg = "You finish your prayer."; */
    nomovemsg = "기도를 마쳤다.";
    afternmv = prayer_done;

    if (p_type == 3 && !Inhell) {
        /* if you've been true to your god you can't die while you pray */
        if (!Blind)
            /*KR You("are surrounded by a shimmering light."); */
            You("어른거리는 빛에 휩싸였다.");
        u.uinvulnerable = TRUE;
    }

    return 1;
}

STATIC_PTR int
prayer_done() /* M. Stephenson (1.0.3b) */
{
    aligntyp alignment = p_aligntyp;

    u.uinvulnerable = FALSE;
    if (p_type == -1) {
#if 0 /*KR: 원본*/
        godvoice(alignment,
                 (alignment == A_LAWFUL)
                    ? "Vile creature, thou durst call upon me?"
                    : "Walk no more, perversion of nature!");
#else /*KR: KRNethack 맞춤 번역 */
        godvoice(alignment,
                 (alignment == A_LAWFUL)
                     ? "비열한 피조물이여, 감히 나를 부르느냐?"
                     : "더 이상 걷지 마라, 자연의 이치에 어긋난 자여!");
#endif
        /*KR You_feel("like you are falling apart."); */
        You("몸이 산산이 조각나는 것 같다.");
        /* KMH -- Gods have mastery over unchanging */
        rehumanize();
        /* no Half_physical_damage adjustment here */
        /*KR losehp(rnd(20), "residual undead turning effect", KILLED_BY_AN);
         */
        losehp(rnd(20), "잔류하는 언데드 퇴치 효과", KILLED_BY_AN);
        exercise(A_CON, FALSE);
        return 1;
    }
    if (Inhell) {
#if 0 /*KR: 원본*/
        pline("Since you are in Gehennom, %s can't help you.",
              align_gname(alignment));
#else /*KR: KRNethack 맞춤 번역 */
        pline("당신은 게헨놈에 있으므로, %s 당신을 도와줄 수 없다.",
              append_josa(align_gname(alignment), "은"));
#endif
        /* haltingly aligned is least likely to anger */
        if (u.ualign.record <= 0 || rnl(u.ualign.record))
            angrygods(u.ualign.type);
        return 0;
    }

    if (p_type == 0) {
        if (on_altar() && u.ualign.type != alignment)
            (void) water_prayer(FALSE);
        u.ublesscnt += rnz(250);
        change_luck(-3);
        gods_upset(u.ualign.type);
    } else if (p_type == 1) {
        if (on_altar() && u.ualign.type != alignment)
            (void) water_prayer(FALSE);
        angrygods(u.ualign.type); /* naughty */
    } else if (p_type == 2) {
        if (water_prayer(FALSE)) {
            /* attempted water prayer on a non-coaligned altar */
            u.ublesscnt += rnz(250);
            change_luck(-3);
            gods_upset(u.ualign.type);
        } else
            pleased(alignment);
    } else {
        /* coaligned */
        if (on_altar())
            (void) water_prayer(TRUE);
        pleased(alignment); /* nice */
    }
    return 1;
}

/* #turn command */
int
doturn()
{
    /* Knights & Priest(esse)s only please */
    struct monst *mtmp, *mtmp2;
    const char *Gname;
    int once, range, xlev;

    if (!Role_if(PM_PRIEST) && !Role_if(PM_KNIGHT)) {
        /* Try to use the "turn undead" spell.
         *
         * This used to be based on whether hero knows the name of the
         * turn undead spellbook, but it's possible to know--and be able
         * to cast--the spell while having lost the book ID to amnesia.
         * (It also used to tell spelleffects() to cast at self?)
         */
        int sp_no;

        for (sp_no = 0; sp_no < MAXSPELL; ++sp_no) {
            if (spl_book[sp_no].sp_id == NO_SPELL)
                break;
            else if (spl_book[sp_no].sp_id == SPE_TURN_UNDEAD)
                return spelleffects(sp_no, FALSE);
        }
        /*KR You("don't know how to turn undead!"); */
        You("언데드를 물리치는 방법을 모른다!");
        return 0;
    }
    u.uconduct.gnostic++;
    Gname = halu_gname(u.ualign.type);

    /* [What about needing free hands (does #turn involve any gesturing)?] */
    if (!can_chant(&youmonst)) {
        /* "evilness": "demons and undead" is too verbose and too precise */
#if 0 /*KR: 원본*/
        You("are %s upon %s to turn aside evilness.",
            Strangled ? "not able to call" : "incapable of calling", Gname);
#else /*KR: KRNethack 맞춤 번역 */
        You("악을 물리치기 위해 %s 부를 수 %s.", append_josa(Gname, "을"),
            Strangled ? "없다" : "있는 능력이 없다");
#endif
        /* violates agnosticism due to intent; conduct tracking is not
           supposed to affect play but we make an exception here:  use a
           move if this is the first time agnostic conduct has been broken */
        return (u.uconduct.gnostic == 1);
    }
    if ((u.ualign.type != A_CHAOTIC
         && (is_demon(youmonst.data) || is_undead(youmonst.data)))
        || u.ugangr > 6) { /* "Die, mortal!" */
        /*KR pline("For some reason, %s seems to ignore you.", Gname); */
        pline("어쩐지, %s 당신을 무시하는 것 같다.",
              append_josa(Gname, "이"));
        aggravate();
        exercise(A_WIS, FALSE);
        return 1;
    }
    if (Inhell) {
#if 0 /*KR: 원본*/
        pline("Since you are in Gehennom, %s %s help you.",
              /* not actually calling upon Moloch but use alternate
                 phrasing anyway if hallucinatory feedback says it's him */
              Gname, !strcmp(Gname, Moloch) ? "won't" : "can't");
#else /*KR: KRNethack 맞춤 번역 */
        pline("당신은 게헨놈에 있으므로, %s 당신을 도울 수 %s.",
              /* not actually calling upon Moloch but use alternate
                 phrasing anyway if hallucinatory feedback says it's him */
              append_josa(Gname, "은"),
              !strcmp(Gname, Moloch) ? "없다" : "없다");
#endif
        aggravate();
        return 1;
    }
    /*KR pline("Calling upon %s, you chant an arcane formula.", Gname); */
    pline("%s 부르며, 신비한 주문을 읊조린다.", append_josa(Gname, "을"));
    exercise(A_WIS, TRUE);

    /* note: does not perform unturn_dead() on victims' inventories */
    range = BOLT_LIM + (u.ulevel / 5); /* 8 to 14 */
    range *= range;
    once = 0;
    for (mtmp = fmon; mtmp; mtmp = mtmp2) {
        mtmp2 = mtmp->nmon;
        if (DEADMONSTER(mtmp))
            continue;
        /* 3.6.3: used to use cansee() here but the purpose is to prevent
           #turn operating through walls, not to require that the hero be
           able to see the target location */
        if (!couldsee(mtmp->mx, mtmp->my)
            || distu(mtmp->mx, mtmp->my) > range)
            continue;

        if (!mtmp->mpeaceful
            && (is_undead(mtmp->data) || is_vampshifter(mtmp)
                || (is_demon(mtmp->data) && (u.ulevel > (MAXULEV / 2))))) {
            mtmp->msleeping = 0;
            if (Confusion) {
                if (!once++)
                    /*KR pline("Unfortunately, your voice falters."); */
                    pline("불행하게도, 당신의 목소리가 떨린다.");
                mtmp->mflee = 0;
                mtmp->mfrozen = 0;
                mtmp->mcanmove = 1;
            } else if (!resist(mtmp, '\0', 0, TELL)) {
                xlev = 6;
                switch (mtmp->data->mlet) {
                /* this is intentional, lichs are tougher
                   than zombies. */
                case S_LICH:
                    xlev += 2;
                    /*FALLTHRU*/
                case S_GHOST:
                    xlev += 2;
                    /*FALLTHRU*/
                case S_VAMPIRE:
                    xlev += 2;
                    /*FALLTHRU*/
                case S_WRAITH:
                    xlev += 2;
                    /*FALLTHRU*/
                case S_MUMMY:
                    xlev += 2;
                    /*FALLTHRU*/
                case S_ZOMBIE:
                    if (u.ulevel >= xlev && !resist(mtmp, '\0', 0, NOTELL)) {
                        if (u.ualign.type == A_CHAOTIC) {
                            mtmp->mpeaceful = 1;
                            set_malign(mtmp);
                        } else { /* damn them */
                            killed(mtmp);
                        }
                        break;
                    } /* else flee */
                /*FALLTHRU*/
                default:
                    monflee(mtmp, 0, FALSE, TRUE);
                    break;
                }
            }
        }
    }

    /*
     * There is no detrimental effect on self for successful #turn
     * while in demon or undead form.  That can only be done while
     * chaotic oneself (see "For some reason" above) and chaotic
     * turning only makes targets peaceful.
     *
     * Paralysis duration probably ought to be based on the strengh
     * of turned creatures rather than on turner's level.
     * Why doesn't this honor Free_action?  [Because being able to
     * repeat #turn every turn would be too powerful.  Maybe instead
     * of nomul(-N) we should add the equivalent of mon->mspec_used
     * for the hero and refuse to #turn when it's non-zero?  Or have
     * both and u.uspec_used only matters when Free_action prevents
     * the brief paralysis?]
     */
    nomul(-(5 - ((u.ulevel - 1) / 6))); /* -5 .. -1 */
    /*KR multi_reason = "trying to turn the monsters"; */
    multi_reason = "몬스터들을 물리치려 애쓰는 중이라";
    nomovemsg = You_can_move_again;
    return 1;
}

const char *
a_gname()
{
    return a_gname_at(u.ux, u.uy);
}

/* returns the name of an altar's deity */
const char *
a_gname_at(x, y)
xchar x, y;
{
    if (!IS_ALTAR(levl[x][y].typ))
        return (char *) 0;

    return align_gname(a_align(x, y));
}

/* returns the name of the hero's deity */
const char *
u_gname()
{
    return align_gname(u.ualign.type);
}

const char *
align_gname(alignment)
aligntyp alignment;
{
    const char *gnam;

    switch (alignment) {
    case A_NONE:
        gnam = Moloch;
        break;
    case A_LAWFUL:
        gnam = urole.lgod;
        break;
    case A_NEUTRAL:
        gnam = urole.ngod;
        break;
    case A_CHAOTIC:
        gnam = urole.cgod;
        break;
    default:
        impossible("unknown alignment.");
        /*KR gnam = "someone"; */
        gnam = "누군가";
        break;
    }
    if (*gnam == '_')
        ++gnam;
    return gnam;
}

static const char *hallu_gods[] = {
#if 0                           /*KR: 원본*/
    "the Flying Spaghetti Monster", /* Church of the FSM */
    "Eris",                         /* Discordianism */
    "the Martians",                 /* every science fiction ever */
    "Xom",                          /* Crawl */
    "AnDoR dRaKoN",                 /* ADOM */
    "the Central Bank of Yendor",   /* economics */
    "Tooth Fairy",                  /* real world(?) */
    "Om",                           /* Discworld */
    "Yawgmoth",                     /* Magic: the Gathering */
    "Morgoth",                      /* LoTR */
    "Cthulhu",                      /* Lovecraft */
    "the Ori",                      /* Stargate */
    "destiny",                      /* why not? */
    "your Friend the Computer",     /* Paranoia */
#else                           /*KR: KRNethack 맞춤 번역 */
    "날아다니는 스파게티 괴물", /* Church of the FSM */
    "에리스",                   /* Discordianism */
    "화성인들",                 /* every science fiction ever */
    "좀",                       /* Crawl */
    "안도르 드라콘",            /* ADOM */
    "옌더 중앙은행",            /* economics */
    "이빨 요정",                /* real world(?) */
    "옴",                       /* Discworld */
    "야그모쓰",                 /* Magic: the Gathering */
    "모르고스",                 /* LoTR */
    "크툴루",                   /* Lovecraft */
    "오라이",                   /* Stargate */
    "운명",                     /* why not? */
    "친애하는 컴퓨터",          /* Paranoia */
#endif
};

/* hallucination handling for priest/minion names: select a random god
   iff character is hallucinating */
const char *
halu_gname(alignment)
aligntyp alignment;
{
    const char *gnam = NULL;
    int which;

    if (!Hallucination)
        return align_gname(alignment);

    /* Some roles (Priest) don't have a pantheon unless we're playing as
       that role, so keep trying until we get a role which does have one.
       [If playing a Priest, the current pantheon will be twice as likely
       to get picked as any of the others.  That's not significant enough
       to bother dealing with.] */
    do
        which = randrole(TRUE);
    while (!roles[which].lgod);

    switch (rn2_on_display_rng(9)) {
    case 0:
    case 1:
        gnam = roles[which].lgod;
        break;
    case 2:
    case 3:
        gnam = roles[which].ngod;
        break;
    case 4:
    case 5:
        gnam = roles[which].cgod;
        break;
    case 6:
    case 7:
        gnam = hallu_gods[rn2_on_display_rng(SIZE(hallu_gods))];
        break;
    case 8:
        gnam = Moloch;
        break;
    default:
        impossible("rn2 broken in halu_gname?!?");
    }
    if (!gnam) {
        impossible("No random god name?");
#if 0                             /*KR: 원본*/
        gnam = "your Friend the Computer"; /* Paranoia */
#else                             /*KR: KRNethack 맞춤 번역 */
        gnam = "친애하는 컴퓨터"; /* Paranoia */
#endif
    }
    if (*gnam == '_')
        ++gnam;
    return gnam;
}

/* deity's title */
const char *
align_gtitle(alignment)
aligntyp alignment;
{
    /*KR const char *gnam, *result = "god"; */
    const char *gnam, *result = "신";

    switch (alignment) {
    case A_LAWFUL:
        gnam = urole.lgod;
        break;
    case A_NEUTRAL:
        gnam = urole.ngod;
        break;
    case A_CHAOTIC:
        gnam = urole.cgod;
        break;
    default:
        gnam = 0;
        break;
    }
    if (gnam && *gnam == '_')
        /*KR result = "goddess"; */
        result = "여신";
    return result;
}

void altar_wrath(x, y) register int x, y;
{
    aligntyp altaralign = a_align(x, y);

    if (u.ualign.type == altaralign && u.ualign.record > -rn2(4)) {
        /*KR godvoice(altaralign, "How darest thou desecrate my altar!"); */
        godvoice(altaralign, "감히 내 제단을 더럽히다니!");
        (void) adjattrib(A_WIS, -1, FALSE);
        u.ualign.record--;
    } else {
#if 0 /*KR: 원본*/
        pline("%s %s%s:",
              !Deaf ? "A voice (could it be"
                    : "Despite your deafness, you seem to hear",
              align_gname(altaralign),
              !Deaf ? "?) whispers" : " say");
#else /*KR: KRNethack 맞춤 번역 */
        pline("%s %s%s:", !Deaf ? "(설마" : "귀가 들리지 않음에도 불구하고,",
              align_gname(altaralign),
              !Deaf ? "?)의 속삭임을 들었다" : "의 목소리가 들리는 듯하다");
#endif
        /*KR verbalize("Thou shalt pay, infidel!"); */
        verbalize("대가를 치를 것이다, 이교도여!");
        /* higher luck is more likely to be reduced; as it approaches -5
           the chance to lose another point drops down, eventually to 0 */
        if (Luck > -5 && rn2(Luck + 6))
            change_luck(rn2(20) ? -1 : -2);
    }
}

/* assumes isok() at one space away, but not necessarily at two */
STATIC_OVL boolean
blocked_boulder(dx, dy)
int dx, dy;
{
    register struct obj *otmp;
    int nx, ny;
    long count = 0L;

    for (otmp = level.objects[u.ux + dx][u.uy + dy]; otmp;
         otmp = otmp->nexthere) {
        if (otmp->otyp == BOULDER)
            count += otmp->quan;
    }

    nx = u.ux + 2 * dx, ny = u.uy + 2 * dy; /* next spot beyond boulder(s) */
    switch (count) {
    case 0:
        /* no boulders--not blocked */
        return FALSE;
    case 1:
        /* possibly blocked depending on if it's pushable */
        break;
    case 2:
        /* this is only approximate since multiple boulders might sink */
        if (is_pool_or_lava(nx, ny)) /* does its own isok() check */
            break;                   /* still need Sokoban check below */
        /*FALLTHRU*/
    default:
        /* more than one boulder--blocked after they push the top one;
           don't force them to push it first to find out */
        return TRUE;
    }

    if (dx && dy && Sokoban) /* can't push boulder diagonally in Sokoban */
        return TRUE;
    if (!isok(nx, ny))
        return TRUE;
    if (IS_ROCK(levl[nx][ny].typ))
        return TRUE;
    if (sobj_at(BOULDER, nx, ny))
        return TRUE;

    return FALSE;
}

/*pray.c*/
