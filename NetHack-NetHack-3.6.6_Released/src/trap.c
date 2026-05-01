/* NetHack 3.6	trap.c	$NHDT-Date: 1576638501 2019/12/18 03:08:21 $  $NHDT-Branch: NetHack-3.6 $:$NHDT-Revision: 1.329 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2013. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

extern const char *const destroy_strings[][3]; /* from zap.c */

STATIC_DCL boolean FDECL(keep_saddle_with_steedcorpse, (unsigned, struct obj *,
                                                        struct obj *));
STATIC_DCL struct obj *FDECL(t_missile, (int, struct trap *));
STATIC_DCL char *FDECL(trapnote, (struct trap *, BOOLEAN_P));
STATIC_DCL int FDECL(steedintrap, (struct trap *, struct obj *));
STATIC_DCL void FDECL(launch_drop_spot, (struct obj *, XCHAR_P, XCHAR_P));
STATIC_DCL int FDECL(mkroll_launch, (struct trap *, XCHAR_P, XCHAR_P,
                                     SHORT_P, long));
STATIC_DCL boolean FDECL(isclearpath, (coord *, int, SCHAR_P, SCHAR_P));
STATIC_DCL void FDECL(dofiretrap, (struct obj *));
STATIC_DCL void NDECL(domagictrap);
STATIC_DCL boolean FDECL(emergency_disrobe, (boolean *));
STATIC_DCL int FDECL(untrap_prob, (struct trap *));
STATIC_DCL void FDECL(move_into_trap, (struct trap *));
STATIC_DCL int FDECL(try_disarm, (struct trap *, BOOLEAN_P));
STATIC_DCL void FDECL(reward_untrap, (struct trap *, struct monst *));
STATIC_DCL int FDECL(disarm_holdingtrap, (struct trap *));
STATIC_DCL int FDECL(disarm_landmine, (struct trap *));
STATIC_DCL int FDECL(disarm_squeaky_board, (struct trap *));
STATIC_DCL int FDECL(disarm_shooting_trap, (struct trap *, int));
STATIC_DCL void FDECL(clear_conjoined_pits, (struct trap *));
STATIC_DCL boolean FDECL(adj_nonconjoined_pit, (struct trap *));
STATIC_DCL int FDECL(try_lift, (struct monst *, struct trap *, int,
                                BOOLEAN_P));
STATIC_DCL int FDECL(help_monster_out, (struct monst *, struct trap *));
#if 0
STATIC_DCL void FDECL(join_adjacent_pits, (struct trap *));
#endif
STATIC_DCL boolean FDECL(thitm, (int, struct monst *, struct obj *, int,
                                 BOOLEAN_P));
STATIC_DCL void NDECL(maybe_finish_sokoban);

/* mintrap() should take a flags argument, but for time being we use this */
STATIC_VAR int force_mintrap = 0;

#if 0 /*KR 필요한 변수만 한글화 */
STATIC_VAR const char *const a_your[2] = { "a", "your" };
STATIC_VAR const char *const A_Your[2] = { "A", "Your" };
STATIC_VAR const char tower_of_flame[] = "tower of flame";
STATIC_VAR const char *const A_gush_of_water_hits = "A gush of water hits";
#else
STATIC_VAR const char tower_of_flame[] = "불기둥";
#endif
#if 0 /*KR:T*/
STATIC_VAR const char *const blindgas[6] = { "humid",   "odorless",
                                             "pungent", "chilling",
                                             "acrid",   "biting" };
#else
STATIC_VAR const char *const blindgas[6] = { "습한",   "무취의", 
                                             "자극적인", "서늘한", 
                                             "매캐한", "따가운" };
#endif
#if 1 /*KR: 함정 생성자 구분용 배열*/
const char *set_you[2] = { "", "당신이 설치한 " };
STATIC_VAR const char *dig_you[2] = { "", "당신이 파놓은 " };
STATIC_VAR const char *web_you[2] = { "", "당신이 쳐놓은 " };
#endif

/* called when you're hit by fire (dofiretrap,buzz,zapyourself,explode);
   returns TRUE if hit on torso */
boolean
burnarmor(victim)
struct monst *victim;
{
    struct obj *item;
    char buf[BUFSZ];
    int mat_idx, oldspe;
    boolean hitting_u;

    if (!victim)
        return 0;
    hitting_u = (victim == &youmonst);

    /* burning damage may dry wet towel */
    item = hitting_u ? carrying(TOWEL) : m_carrying(victim, TOWEL);
    while (item) {
        if (is_wet_towel(item)) {
            oldspe = item->spe;
            dry_a_towel(item, rn2(oldspe + 1), TRUE);
            if (item->spe != oldspe)
                break; /* stop once one towel has been affected */
        }
        item = item->nobj;
    }

#define burn_dmg(obj, descr) erode_obj(obj, descr, ERODE_BURN, EF_GREASE)
    while (1) {
        switch (rn2(5)) {
        case 0:
            item = hitting_u ? uarmh : which_armor(victim, W_ARMH);
            if (item) {
                mat_idx = objects[item->otyp].oc_material;
#if 0 /*KR:T*/
                Sprintf(buf, "%s %s", materialnm[mat_idx],
                        helm_simple_name(item));
#else
                Sprintf(buf, "%s의 %s", materialnm[mat_idx],
                        helm_simple_name(item));
#endif
            }
            /*KR if (!burn_dmg(item, item ? buf : "helmet")) */
            if (!burn_dmg(item, item ? buf : "투구"))
                continue;
            break;
        case 1:
            item = hitting_u ? uarmc : which_armor(victim, W_ARMC);
            if (item) {
                (void) burn_dmg(item, cloak_simple_name(item));
                return TRUE;
            }
            item = hitting_u ? uarm : which_armor(victim, W_ARM);
            if (item) {
                (void) burn_dmg(item, xname(item));
                return TRUE;
            }
            item = hitting_u ? uarmu : which_armor(victim, W_ARMU);
            if (item)
                /*KR (void) burn_dmg(item, "shirt"); */
                (void) burn_dmg(item, "셔츠");
            return TRUE;
        case 2:
            item = hitting_u ? uarms : which_armor(victim, W_ARMS);
            /*KR if (!burn_dmg(item, "wooden shield")) */
            if (!burn_dmg(item, "나무 방패"))
                continue;
            break;
        case 3:
            item = hitting_u ? uarmg : which_armor(victim, W_ARMG);
            /*KR if (!burn_dmg(item, "gloves")) */
            if (!burn_dmg(item, "장갑"))
                continue;
            break;
        case 4:
            item = hitting_u ? uarmf : which_armor(victim, W_ARMF);
            /*KR if (!burn_dmg(item, "boots")) */
            if (!burn_dmg(item, "장화"))
                continue;
            break;
        }
        break; /* Out of while loop */
    }
#undef burn_dmg

    return FALSE;
}

/* Generic erode-item function.
 * "ostr", if non-null, is an alternate string to print instead of the
 *   object's name.
 * "type" is an ERODE_* value for the erosion type
 * "flags" is an or-ed list of EF_* flags
 *
 * Returns an erosion return value (ER_*)
 */
int
erode_obj(otmp, ostr, type, ef_flags)
register struct obj *otmp;
const char *ostr;
int type;
int ef_flags;
{
    static NEARDATA const char
#if 0 /*KR:T*/
        *const action[] = { "smoulder", "rust", "rot", "corrode" },
        *const msg[] = { "burnt", "rusted", "rotten", "corroded" },
        *const bythe[] = { "heat", "oxidation", "decay", "corrosion" };
#else
        *const action[] = { "그을렸다", "녹슬었다", "썩었다", "부식되었다" },
        *const msg[] = { "탄", "녹슨", "썩은", "부식된" },
               *const bythe[] = { "열기", "산화", "부패", "부식" };
#endif
    boolean vulnerable = FALSE, is_primary = TRUE,
            check_grease = (ef_flags & EF_GREASE) ? TRUE : FALSE,
            print = (ef_flags & EF_VERBOSE) ? TRUE : FALSE,
            uvictim, vismon, visobj;
    int erosion, cost_type;
    struct monst *victim;

    if (!otmp)
        return ER_NOTHING;

    victim = carried(otmp) ? &youmonst : mcarried(otmp) ? otmp->ocarry : NULL;
    uvictim = (victim == &youmonst);
    vismon = victim && (victim != &youmonst) && canseemon(victim);
    /* Is bhitpos correct here? Ugh. */
    visobj = !victim && cansee(bhitpos.x, bhitpos.y);

    switch (type) {
    case ERODE_BURN:
        vulnerable = is_flammable(otmp);
        check_grease = FALSE;
        cost_type = COST_BURN;
        break;
    case ERODE_RUST:
        vulnerable = is_rustprone(otmp);
        cost_type = COST_RUST;
        break;
    case ERODE_ROT:
        vulnerable = is_rottable(otmp);
        check_grease = FALSE;
        is_primary = FALSE;
        cost_type = COST_ROT;
        break;
    case ERODE_CORRODE:
        vulnerable = is_corrodeable(otmp);
        is_primary = FALSE;
        cost_type = COST_CORRODE;
        break;
    default:
        impossible("Invalid erosion type in erode_obj");
        return ER_NOTHING;
    }
    erosion = is_primary ? otmp->oeroded : otmp->oeroded2;

    if (!ostr)
        ostr = cxname(otmp);
    /* 'visobj' messages insert "the"; probably ought to switch to the() */
    if (visobj && !(uvictim || vismon) && !strncmpi(ostr, "the ", 4))
        ostr += 4;

    if (check_grease && otmp->greased) {
        grease_protect(otmp, ostr, victim);
        return ER_GREASED;
    } else if (!erosion_matters(otmp)) {
        return ER_NOTHING;
    } else if (!vulnerable || (otmp->oerodeproof && otmp->rknown)) {
        if (flags.verbose && print && (uvictim || vismon))
#if 0 /*KR: 원본*/
            pline("%s %s %s not affected by %s.",
                  uvictim ? "Your" : s_suffix(Monnam(victim)),
                  ostr, vtense(ostr, "are"), bythe[type]);
#else /*KR: KRNethack 맞춤 번역 */
            if (uvictim) {
                pline("당신의 %s %s의 영향을 받지 않았다.",
                      append_josa(ostr, "은"), bythe[type]);
            } else {
                pline("%s의 %s %s의 영향을 받지 않았다.", Monnam(victim),
                      append_josa(ostr, "은"), bythe[type]);
            }
#endif
        return ER_NOTHING;
    } else if (otmp->oerodeproof || (otmp->blessed && !rnl(4))) {
        if (flags.verbose && (print || otmp->oerodeproof)
            && (uvictim || vismon || visobj))
#if 0 /*KR: 원본*/
            pline("Somehow, %s %s %s not affected by the %s.",
                  uvictim ? "your"
                          : !vismon ? "the" /* visobj */
                                    : s_suffix(mon_nam(victim)),
                  ostr, vtense(ostr, "are"), bythe[type]);
#else /*KR: KRNethack 맞춤 번역 */
            if (uvictim) {
                pline("왠일인지, 당신의 %s %s의 영향을 받지 않았다.",
                      append_josa(ostr, "은"), bythe[type]);
            } else if (!vismon) { /* visobj */
                pline("왠일인지, %s %s의 영향을 받지 않았다.",
                      append_josa(ostr, "은"), bythe[type]);
            } else {
                pline("왠일인지, %s의 %s %s의 영향을 받지 않았다.",
                      mon_nam(victim), append_josa(ostr, "은"), bythe[type]);
            }
#endif
        /* We assume here that if the object is protected because it
         * is blessed, it still shows some minor signs of wear, and
         * the hero can distinguish this from an object that is
         * actually proof against damage.
         */
        if (otmp->oerodeproof) {
            otmp->rknown = TRUE;
            if (victim == &youmonst)
                update_inventory();
        }

        return ER_NOTHING;
    } else if (erosion < MAX_ERODE) {
#if 0 /*KR:T*/
        const char *adverb = (erosion + 1 == MAX_ERODE)
                                 ? " completely"
                                 : erosion ? " further" : "";
#else
        const char *adverb = (erosion + 1 == MAX_ERODE)
                                 ? " 완전히"
                                 : erosion ? "더욱 " : "";
#endif

        if (uvictim || vismon || visobj)
#if 0 /*KR:T*/
            pline("%s %s %s%s!",
                  uvictim ? "Your"
                          : !vismon ? "The" /* visobj */
                                    : s_suffix(Monnam(victim)),
                  ostr, vtense(ostr, action[type]), adverb);
#else /*KR: KRNethack 맞춤 번역 */
            if (uvictim)
                pline("당신의 %s %s%s!", append_josa(ostr, "이"), adverb,
                      action[type]);
            else if (!vismon) /* visobj */
                pline("%s %s%s!", append_josa(ostr, "이"), adverb,
                      action[type]);
            else
                pline("%s %s %s%s!", s_suffix(Monnam(victim)),
                      append_josa(ostr, "이"), adverb, action[type]);
#endif

        if (ef_flags & EF_PAY)
            costly_alteration(otmp, cost_type);

        if (is_primary)
            otmp->oeroded++;
        else
            otmp->oeroded2++;

        if (victim == &youmonst)
            update_inventory();

        return ER_DAMAGED;
    } else if (ef_flags & EF_DESTROY) {
        if (uvictim || vismon || visobj)
#if 0 /*KR: 원본*/
            pline("%s %s %s away!",
                  uvictim ? "Your"
                          : !vismon ? "The" /* visobj */
                                    : s_suffix(Monnam(victim)),
                  ostr, vtense(ostr, action[type]));
#else /*KR: KRNethack 맞춤 번역 */
            if (uvictim)
                pline("당신의 %s 완전히 %s!", append_josa(ostr, "이"),
                      action[type]);
            else if (!vismon) /* visobj */
                pline("%s 완전히 %s!", append_josa(ostr, "이"),
                      action[type]);
            else
                pline("%s %s 완전히 %s!", s_suffix(Monnam(victim)),
                      append_josa(ostr, "이"), action[type]);
#endif

        if (ef_flags & EF_PAY)
            costly_alteration(otmp, cost_type);

        setnotworn(otmp);
        delobj(otmp);
        return ER_DESTROYED;
    } else {
        if (flags.verbose && print) {
            if (uvictim)
#if 0 /*KR: 원본*/
                Your("%s %s completely %s.",
                     ostr, vtense(ostr, Blind ? "feel" : "look"), msg[type]);
#else /*KR: KRNethack 맞춤 번역 */
                pline("당신의 %s 완전히 %s 것 같다.",
                      append_josa(ostr, "이"), msg[type]);
#endif
            else if (vismon || visobj)
#if 0 /*KR: 원본*/
                pline("%s %s %s completely %s.",
                      !vismon ? "The" : s_suffix(Monnam(victim)),
                      ostr, vtense(ostr, "look"), msg[type]);
#else /*KR: KRNethack 맞춤 번역 */
                if (!vismon)
                    pline("%s 완전히 %s 것 같다.",
                          append_josa(ostr, "이(가)"), msg[type]);
                else
                    pline("%s %s 완전히 %s 것 같다.",
                          s_suffix(Monnam(victim)),
                          append_josa(ostr, "이"), msg[type]);
#endif
        }
        return ER_NOTHING;
    }
}

/* Protect an item from erosion with grease. Returns TRUE if the grease
 * wears off.
 */
boolean
grease_protect(otmp, ostr, victim)
register struct obj *otmp;
const char *ostr;
struct monst *victim;
{
    /*KR static const char txt[] = "protected by the layer of grease!"; */
    static const char txt[] = "기름막에 의해 보호되고 있다!";
    boolean vismon = victim && (victim != &youmonst) && canseemon(victim);

if (ostr) {
        if (victim == &youmonst)
            /*KR Your("%s %s %s", ostr, vtense(ostr, "are"), txt); */
            Your("%s %s", ostr, txt);
        else if (vismon)
#if 0 /*KR: 원본*/
            pline("%s's %s %s %s", Monnam(victim),
                  ostr, vtense(ostr, "are"), txt);
#else /*KR: KRNethack 맞춤 번역 */
            pline("%s의 %s %s", Monnam(victim), ostr, txt);
#endif
    } else if (victim == &youmonst || vismon) {
#if 0 /*KR: 원본*/
        pline("%s %s", Yobjnam2(otmp, "are"), txt);
#else /*KR: KRNethack 맞춤 번역 */
        pline("%s %s", append_josa(xname(otmp), "는"), txt);
#endif
    }
    if (!rn2(2)) {
        otmp->greased = 0;
        if (carried(otmp)) {
            /*KR pline_The("grease dissolves."); */
            pline("기름칠이 벗겨졌다.");
            update_inventory();
        }
        return TRUE;
    }
    return FALSE;
}

struct trap *
maketrap(x, y, typ)
int x, y, typ;
{
    static union vlaunchinfo zero_vl;
    boolean oldplace;
    struct trap *ttmp;
    struct rm *lev = &levl[x][y];

    if ((ttmp = t_at(x, y)) != 0) {
        if (ttmp->ttyp == MAGIC_PORTAL || ttmp->ttyp == VIBRATING_SQUARE)
            return (struct trap *) 0;
        oldplace = TRUE;
        if (u.utrap && x == u.ux && y == u.uy
            && ((u.utraptype == TT_BEARTRAP && typ != BEAR_TRAP)
                || (u.utraptype == TT_WEB && typ != WEB)
                || (u.utraptype == TT_PIT && !is_pit(typ))))
            u.utrap = 0;
        /* old <tx,ty> remain valid */
    } else if (IS_FURNITURE(lev->typ)
               && (!IS_GRAVE(lev->typ) || (typ != PIT && typ != HOLE))) {
        /* no trap on top of furniture (caller usually screens the
           location to inhibit this, but wizard mode wishing doesn't) */
        return (struct trap *) 0;
    } else {
        oldplace = FALSE;
        ttmp = newtrap();
        (void) memset((genericptr_t)ttmp, 0, sizeof(struct trap));
        ttmp->ntrap = 0;
        ttmp->tx = x;
        ttmp->ty = y;
    }
    /* [re-]initialize all fields except ntrap (handled below) and <tx,ty> */
    ttmp->vl = zero_vl;
    ttmp->launch.x = ttmp->launch.y = -1; /* force error if used before set */
    ttmp->dst.dnum = ttmp->dst.dlevel = -1;
    ttmp->madeby_u = 0;
    ttmp->once = 0;
    ttmp->tseen = (typ == HOLE); /* hide non-holes */
    ttmp->ttyp = typ;

    switch (typ) {
    case SQKY_BOARD: {
        int tavail[12], tpick[12], tcnt = 0, k;
        struct trap *t;

        for (k = 0; k < 12; ++k)
            tavail[k] = tpick[k] = 0;
        for (t = ftrap; t; t = t->ntrap)
            if (t->ttyp == SQKY_BOARD && t != ttmp)
                tavail[t->tnote] = 1;
        /* now populate tpick[] with the available indices */
        for (k = 0; k < 12; ++k)
            if (tavail[k] == 0)
                tpick[tcnt++] = k;
        /* choose an unused note; if all are in use, pick a random one */
        ttmp->tnote = (short) ((tcnt > 0) ? tpick[rn2(tcnt)] : rn2(12));
        break;
    }
    case STATUE_TRAP: { /* create a "living" statue */
        struct monst *mtmp;
        struct obj *otmp, *statue;
        struct permonst *mptr;
        int trycount = 10;

        do { /* avoid ultimately hostile co-aligned unicorn */
            mptr = &mons[rndmonnum()];
        } while (--trycount > 0 && is_unicorn(mptr)
                 && sgn(u.ualign.type) == sgn(mptr->maligntyp));
        statue = mkcorpstat(STATUE, (struct monst *) 0, mptr, x, y,
                            CORPSTAT_NONE);
        mtmp = makemon(&mons[statue->corpsenm], 0, 0, MM_NOCOUNTBIRTH);
        if (!mtmp)
            break; /* should never happen */
        while (mtmp->minvent) {
            otmp = mtmp->minvent;
            otmp->owornmask = 0;
            obj_extract_self(otmp);
            (void) add_to_container(statue, otmp);
        }
        statue->owt = weight(statue);
        mongone(mtmp);
        break;
    }
    case ROLLING_BOULDER_TRAP: /* boulder will roll towards trigger */
        (void) mkroll_launch(ttmp, x, y, BOULDER, 1L);
        break;
    case PIT:
    case SPIKED_PIT:
        ttmp->conjoined = 0;
        /*FALLTHRU*/
    case HOLE:
    case TRAPDOOR:
        if (*in_rooms(x, y, SHOPBASE)
            && (is_hole(typ) || IS_DOOR(lev->typ) || IS_WALL(lev->typ)))
            add_damage(x, y, /* schedule repair */
                       ((IS_DOOR(lev->typ) || IS_WALL(lev->typ))
                        && !context.mon_moving)
                           ? SHOP_HOLE_COST
                           : 0L);
        lev->doormask = 0;     /* subsumes altarmask, icedpool... */
        if (IS_ROOM(lev->typ)) /* && !IS_AIR(lev->typ) */
            lev->typ = ROOM;
        /*
         * some cases which can happen when digging
         * down while phazing thru solid areas
         */
        else if (lev->typ == STONE || lev->typ == SCORR)
            lev->typ = CORR;
        else if (IS_WALL(lev->typ) || lev->typ == SDOOR)
            lev->typ = level.flags.is_maze_lev
                           ? ROOM
                           : level.flags.is_cavernous_lev ? CORR : DOOR;

        unearth_objs(x, y);
        break;
    }

    if (!oldplace) {
        ttmp->ntrap = ftrap;
        ftrap = ttmp;
    } else {
        /* oldplace;
           it shouldn't be possible to override a sokoban pit or hole
           with some other trap, but we'll check just to be safe */
        if (Sokoban)
            maybe_finish_sokoban();
    }
    return ttmp;
}

void
fall_through(td, ftflags)
boolean td; /* td == TRUE : trap door or hole */
unsigned ftflags;
{
    d_level dtmp;
    char msgbuf[BUFSZ];
    const char *dont_fall = 0;
    int newlevel, bottom;
    struct trap *t = (struct trap *) 0;

    /* we'll fall even while levitating in Sokoban; otherwise, if we
       won't fall and won't be told that we aren't falling, give up now */
    if (Blind && Levitation && !Sokoban)
        return;

    bottom = dunlevs_in_dungeon(&u.uz);
    /* when in the upper half of the quest, don't fall past the
       middle "quest locate" level if hero hasn't been there yet */
    if (In_quest(&u.uz)) {
        int qlocate_depth = qlocate_level.dlevel;

        /* deepest reached < qlocate implies current < qlocate */
        if (dunlev_reached(&u.uz) < qlocate_depth)
            bottom = qlocate_depth; /* early cut-off */
    }
    newlevel = dunlev(&u.uz); /* current level */
    do {
        newlevel++;
    } while (!rn2(4) && newlevel < bottom);

    if (td) {
        t = t_at(u.ux, u.uy);
        feeltrap(t);
        if (!Sokoban && !(ftflags & TOOKPLUNGE)) {
            if (t->ttyp == TRAPDOOR)
                /*KR pline("A trap door opens up under you!"); */
                pline("발밑에서 다락문이 열렸다!");
            else
                /*KR pline("There's a gaping hole under you!"); */
                pline("발밑에 입을 벌린 구멍이 있다!");
        }
    } else {
#if 0 /*KR: 원본*/
        pline_The("%s opens up under you!", surface(u.ux, u.uy));
#else /*KR: KRNethack 맞춤 번역 */
        pline("발밑의 %s에 구멍이 뚫렸다!", surface(u.ux, u.uy));
#endif
    }

    if (Sokoban && Can_fall_thru(&u.uz))
        ; /* KMH -- You can't escape the Sokoban level traps */
    else if (Levitation || u.ustuck
             || (!Can_fall_thru(&u.uz) && !levl[u.ux][u.uy].candig)
             || ((Flying || is_clinger(youmonst.data)
                  || (ceiling_hider(youmonst.data) && u.uundetected))
                 && !(ftflags & TOOKPLUNGE))
             || (Inhell && !u.uevent.invoked && newlevel == bottom)) {
        /*KR dont_fall = "don't fall in."; */
        dont_fall = "하지만 당신은 떨어지지 않았다.";
    } else if (youmonst.data->msize >= MZ_HUGE) {
        /*KR dont_fall = "dont fit through."; */
        dont_fall = "통과하기엔 몸집이 너무 크다.";
    } else if (!next_to_u()) {
        /*KR dont_fall = "are jerked back by your pet!"; */
        dont_fall = "당신의 펫이 당신을 낚아채서 끌어올렸다!";
    }
    if (dont_fall) {
        You1(dont_fall);
        /* hero didn't fall through, but any objects here might */
        impact_drop((struct obj *) 0, u.ux, u.uy, 0);
        if (!td) {
            display_nhwindow(WIN_MESSAGE, FALSE);
            /*KR pline_The("opening under you closes up."); */
            pline_The("발밑의 구멍이 닫혔다.");
        }
        return;
    }
    if ((Flying || is_clinger(youmonst.data)) && (ftflags & TOOKPLUNGE) && td
        && t)
#if 0 /*KR: 원본*/
        You("%s down %s!",
            Flying ? "swoop" : "deliberately drop",
            (t->ttyp == TRAPDOOR)
                ? "through the trap door"
                : "into the gaping hole");
#else /*KR: KRNethack 맞춤 번역 */
        You("%s 안으로 %s!", (t->ttyp == TRAPDOOR) ? "다락문" : "구멍",
            Flying ? "급강하했다" : "일부러 뛰어내렸다");
#endif

    if (*u.ushops)
        shopdig(1);
    if (Is_stronghold(&u.uz)) {
        find_hell(&dtmp);
    } else {
        int dist = newlevel - dunlev(&u.uz);
        dtmp.dnum = u.uz.dnum;
        dtmp.dlevel = newlevel;
        if (dist > 1)
#if 0 /*KR: 원본*/
            You("fall down a %s%sshaft!", dist > 3 ? "very " : "",
                dist > 2 ? "deep " : "");
#else /*KR: KRNethack 맞춤 번역 */
            You("%s%s 구멍 속으로 떨어졌다!", dist > 3 ? "매우 " : "",
                dist > 2 ? "깊은 " : "");
#endif
    }
    if (!td)
        /*KR Sprintf(msgbuf, "The hole in the %s above you closes up.",
         * ceiling(u.ux, u.uy)); */
        Sprintf(msgbuf, "머리 위 %s의 구멍이 닫혔다.", ceiling(u.ux, u.uy));

    schedule_goto(&dtmp, FALSE, TRUE, 0, (char *) 0,
                  !td ? msgbuf : (char *) 0);
    }

    /*
     * Animate the given statue.  May have been via shatter attempt, trap,
     * or stone to flesh spell.  Return a monster if successfully animated.
     * If the monster is animated, the object is deleted.  If fail_reason
     * is non-null, then fill in the reason for failure (or success).
     *
     * The cause of animation is:
     *
     * ANIMATE_NORMAL  - hero "finds" the monster
     * ANIMATE_SHATTER - hero tries to destroy the statue
     * ANIMATE_SPELL   - stone to flesh spell hits the statue
     *
     * Perhaps x, y is not needed if we can use get_obj_location() to find
     * the statue's location... ???
     *
     * Sequencing matters:
     * create monster; if it fails, give up with statue intact;
     * give "statue comes to life" message;
     * if statue belongs to shop, have shk give "you owe" message;
     * transfer statue contents to monster (after stolen_value());
     * delete statue.
     * [This ordering means that if the statue ends up wearing a cloak of
     * invisibility or a mummy wrapping, the visibility checks might be
     * wrong, but to avoid that we'd have to clone the statue contents
     * first in order to give them to the monster before checking their
     * shop status--it's not worth the hassle.]
     */
    struct monst *animate_statue(statue, x, y, cause, fail_reason) 
    struct obj *statue;
    xchar x, y;
    int cause;
    int *fail_reason;
    {
        int mnum = statue->corpsenm;
        struct permonst *mptr = &mons[mnum];
        struct monst *mon = 0, *shkp;
        struct obj *item;
        coord cc;
        boolean historic = (Role_if(PM_ARCHEOLOGIST)
                            && (statue->spe & STATUE_HISTORIC) != 0),
                golem_xform = FALSE, use_saved_traits;
        const char *comes_to_life;
        char statuename[BUFSZ], tmpbuf[BUFSZ];
        static const char historic_statue_is_gone[] =
            /*KR "that the historic statue is now gone"; */
            "역사적인 조각상이 사라진 것";

        if (cant_revive(&mnum, TRUE, statue)) {
            /* mnum has changed; we won't be animating this statue as itself
             */
            if (mnum != PM_DOPPELGANGER)
                mptr = &mons[mnum];
            use_saved_traits = FALSE;
        } else if (is_golem(mptr) && cause == ANIMATE_SPELL) {
            /* statue of any golem hit by stone-to-flesh becomes flesh golem
             */
            golem_xform = (mptr != &mons[PM_FLESH_GOLEM]);
            mnum = PM_FLESH_GOLEM;
            mptr = &mons[PM_FLESH_GOLEM];
            use_saved_traits = (has_omonst(statue) && !golem_xform);
        } else {
            use_saved_traits = has_omonst(statue);
        }

        if (use_saved_traits) {
            /* restore a petrified monster */
            cc.x = x, cc.y = y;
            mon = montraits(statue, &cc, (cause == ANIMATE_SPELL));
            if (mon && mon->mtame && !mon->isminion)
                wary_dog(mon, TRUE);
        } else {
            /* statues of unique monsters from bones or wishing end
               up here (cant_revive() sets mnum to be doppelganger;
               mptr reflects the original form for use by newcham()) */
            if ((mnum == PM_DOPPELGANGER && mptr != &mons[PM_DOPPELGANGER])
                /* block quest guards from other roles */
                || (mptr->msound == MS_GUARDIAN
                    && quest_info(MS_GUARDIAN) != mnum)) {
                mon = makemon(&mons[PM_DOPPELGANGER], x, y,
                              NO_MINVENT | MM_NOCOUNTBIRTH | MM_ADJACENTOK);
                /* if hero has protection from shape changers, cham field will
                   be NON_PM; otherwise, set form to match the statue */
                if (mon && mon->cham >= LOW_PM)
                    (void) newcham(mon, mptr, FALSE, FALSE);
            } else
                mon = makemon(mptr, x, y,
                              (cause == ANIMATE_SPELL)
                                  ? (NO_MINVENT | MM_ADJACENTOK)
                                  : NO_MINVENT);
        }

        if (!mon) {
            if (fail_reason)
                *fail_reason = unique_corpstat(&mons[statue->corpsenm])
                                   ? AS_MON_IS_UNIQUE
                                   : AS_NO_MON;
            return (struct monst *) 0;
        }

        /* a non-montraits() statue might specify gender */
        if (statue->spe & STATUE_MALE)
            mon->female = FALSE;
        else if (statue->spe & STATUE_FEMALE)
            mon->female = TRUE;
        /* if statue has been named, give same name to the monster */
        if (has_oname(statue) && !unique_corpstat(mon->data))
            mon = christen_monst(mon, ONAME(statue));
        /* mimic statue becomes seen mimic; other hiders won't be hidden */
        if (M_AP_TYPE(mon))
            seemimic(mon);
        else
            mon->mundetected = FALSE;
        mon->msleeping = 0;
        if (cause == ANIMATE_NORMAL || cause == ANIMATE_SHATTER) {
            /* trap always releases hostile monster */
            mon->mtame = 0; /* (might be petrified pet tossed onto trap) */
            mon->mpeaceful = 0;
            set_malign(mon);
        }

#if 0 /*KR: 원본*/
        comes_to_life = !canspotmon(mon) ? "disappears"
                        : golem_xform    ? "turns into flesh"
                        : (nonliving(mon->data) || is_vampshifter(mon))
                            ? "moves"
                            : "comes to life";
#else /*KR: KRNethack 맞춤 번역 */
        comes_to_life = !canspotmon(mon) ? "사라졌다"
                        : golem_xform    ? "살덩이로 변했다"
                        : (nonliving(mon->data) || is_vampshifter(mon))
                            ? "움직였다"
                            : "생명을 얻었다";
#endif
        if ((x == u.ux && y == u.uy) || cause == ANIMATE_SPELL) {
            /* "the|your|Manlobbi's statue [of a wombat]" */
            shkp = shop_keeper(*in_rooms(mon->mx, mon->my, SHOPBASE));
#if 0 /*KR:T*/
            Sprintf(statuename, "%s%s", shk_your(tmpbuf, statue),
                    (cause == ANIMATE_SPELL
                    /* avoid "of a shopkeeper" if it's Manlobbi himself
                    (if carried, it can't be unpaid--hence won't be
                    described as "Manlobbi's statue"--because there
                    wasn't any living shk when statue was picked up) */
                    && (mon != shkp || carried(statue)))
                     ? xname(statue)
                     : "statue");
#else
            Sprintf(statuename, "%s%s", shk_your(tmpbuf, statue),
                    (cause == ANIMATE_SPELL 
                    && (mon != shkp || carried(statue)))
                     ? xname(statue)
                     : "조각상");
#endif
#if 0 /*KR: 원본*/
            pline("%s %s!", upstart(statuename), comes_to_life);
#else /*KR: KRNethack 맞춤 번역 */
            pline("%s %s!", append_josa(upstart(statuename), "은"),
                  comes_to_life);
#endif
        } else if (Hallucination) { /* They don't know it's a statue */
#if 0                           /*KR: 원본*/
            pline_The("%s suddenly seems more animated.", rndmonnam((char *) 0));
#else                           /*KR: KRNethack 맞춤 번역 */
            pline_The("%s 갑자기 생기 있어 보인다.",
                      append_josa(rndmonnam((char *) 0), "이"));
#endif
        } else if (cause == ANIMATE_SHATTER) {
            if (cansee(x, y))
                Sprintf(statuename, "%s%s", shk_your(tmpbuf, statue),
                        xname(statue));
            else
                Strcpy(statuename, "조각상");
#if 0 /*KR: 원본*/
            pline("Instead of shattering, %s suddenly %s!", statuename,
                  comes_to_life);
#else /*KR: KRNethack 맞춤 번역 */
            pline("%s 산산조각 나는 대신 갑자기 %s!",
                  append_josa(statuename, "이"), comes_to_life);
#endif
        } else { /* cause == ANIMATE_NORMAL */
#if 0 /*KR: 원본*/
        You("find %s posing as a statue.",
            canspotmon(mon) ? a_monnam(mon) : something);
#else /*KR: KRNethack 맞춤 번역 */
            You("조각상으로 위장하고 있던 %s 발견했다.",
                append_josa(canspotmon(mon) ? a_monnam(mon) 
                                            : something, "을"));
#endif
            if (!canspotmon(mon) && Blind)
                map_invisible(x, y);
            stop_occupation();
        }

        /* if this isn't caused by a monster using a wand of striking,
           there might be consequences for the hero */
        if (!context.mon_moving) {
            /* if statue is owned by a shop, hero will have to pay for it;
               stolen_value gives a message (about debt or use of credit)
               which refers to "it" so needs to follow a message describing
               the object ("the statue comes to life" one above) */
            if (cause != ANIMATE_NORMAL && costly_spot(x, y)
                && (carried(statue) ? statue->unpaid : !statue->no_charge)
                && (shkp = shop_keeper(*in_rooms(x, y, SHOPBASE))) != 0
                /* avoid charging for Manlobbi's statue of Manlobbi
                   if stone-to-flesh is used on petrified shopkeep */
                && mon != shkp)
                (void) stolen_value(statue, x, y, (boolean) shkp->mpeaceful,
                                    FALSE);

            if (historic) {
                /*KR You_feel("guilty %s.", historic_statue_is_gone); */
                You("%s에 대해 죄책감을 느꼈다.",
                         historic_statue_is_gone);
                adjalign(-1);
            }
        } else {
            if (historic && cansee(x, y))
                /*KR You_feel("regret %s.", historic_statue_is_gone); */
                You("%s에 대해 후회했다.", historic_statue_is_gone);
            /* no alignment penalty */
        }

        /* transfer any statue contents to monster's inventory */
        while ((item = statue->cobj) != 0) {
            obj_extract_self(item);
            (void) mpickobj(mon, item);
        }
        m_dowear(mon, TRUE);
        /* in case statue is wielded and hero zaps stone-to-flesh at self */
        if (statue->owornmask)
            remove_worn_item(statue, TRUE);
        /* statue no longer exists */
        delobj(statue);

        /* avoid hiding under nothing */
        if (x == u.ux && y == u.uy && Upolyd && hides_under(youmonst.data)
            && !OBJ_AT(x, y))
            u.uundetected = 0;

        if (fail_reason)
            *fail_reason = AS_OK;
        return mon;
    }

    /*
     * You've either stepped onto a statue trap's location or you've triggered
     * a statue trap by searching next to it or by trying to break it with a
     * wand or pick-axe.
     */
    struct monst *activate_statue_trap(trap, x, y, shatter) struct trap *trap;
    xchar x, y;
    boolean shatter;
    {
        struct monst *mtmp = (struct monst *) 0;
        struct obj *otmp = sobj_at(STATUE, x, y);
        int fail_reason;

        /*
         * Try to animate the first valid statue.  Stop the loop when we
         * actually create something or the failure cause is not because
         * the mon was unique.
         */
        deltrap(trap);
        while (otmp) {
            mtmp = animate_statue(otmp, x, y,
                                  shatter ? ANIMATE_SHATTER : ANIMATE_NORMAL,
                                  &fail_reason);
            if (mtmp || fail_reason != AS_MON_IS_UNIQUE)
                break;

            otmp = nxtobj(otmp, STATUE, TRUE);
        }

        feel_newsym(x, y);
        return mtmp;
    }

    STATIC_OVL boolean keep_saddle_with_steedcorpse(
        steed_mid, objchn, saddle) unsigned steed_mid;
    struct obj *objchn, *saddle;
    {
        if (!saddle)
            return FALSE;
        while (objchn) {
            if (objchn->otyp == CORPSE && has_omonst(objchn)) {
                struct monst *mtmp = OMONST(objchn);

                if (mtmp->m_id == steed_mid) {
                    /* move saddle */
                    xchar x, y;
                    if (get_obj_location(objchn, &x, &y, 0)) {
                        obj_extract_self(saddle);
                        place_object(saddle, x, y);
                        stackobj(saddle);
                    }
                    return TRUE;
                }
            }
            if (Has_contents(objchn)
                && keep_saddle_with_steedcorpse(steed_mid, objchn->cobj,
                                                saddle))
                return TRUE;
            objchn = objchn->nobj;
        }
        return FALSE;
    }

    /* monster or you go through and possibly destroy a web.
       return TRUE if could go through. */
    boolean mu_maybe_destroy_web(mtmp, domsg, trap) struct monst *mtmp;
    boolean domsg;
    struct trap *trap;
    {
        boolean isyou = (mtmp == &youmonst);
        struct permonst *mptr = mtmp->data;

        if (amorphous(mptr) || is_whirly(mptr) || flaming(mptr)
            || unsolid(mptr) || mptr == &mons[PM_GELATINOUS_CUBE]) {
            xchar x = trap->tx;
            xchar y = trap->ty;

            if (flaming(mptr) || acidic(mptr)) {
                if (domsg) {
                    if (isyou)
#if 0 /*KR: 원본*/
                    You("%s %s spider web!",
                        (flaming(mptr)) ? "burn" : "dissolve",
                        a_your[trap->madeby_u]);
#else /*KR: KRNethack 맞춤 번역 */
                        You("%s거미줄을 %s!", web_you[trap->madeby_u],
                            (flaming(mptr)) ? "태워버렸다" : "녹여버렸다");
#endif
                    else
#if 0 /*KR: 원본*/
                    pline("%s %s %s spider web!", Monnam(mtmp),
                          (flaming(mptr)) ? "burns" : "dissolves",
                          a_your[trap->madeby_u]);
#else /*KR: KRNethack 맞춤 번역 */
                        pline("%s %s거미줄을 %s!",
                              append_josa(Monnam(mtmp), "는"),
                              web_you[trap->madeby_u],
                              (flaming(mptr)) ? "태워버렸다" : "녹여버렸다");
#endif
                }
                deltrap(trap);
                newsym(x, y);
                return TRUE;
            }
            if (domsg) {
                if (isyou) {
                    /*KR You("flow through %s spider web.",
                     * a_your[trap->madeby_u]); */
                    You("%s거미줄 사이로 스며나갔다.",
                        web_you[trap->madeby_u]);
                } else {
#if 0 /*KR: 원본*/
                pline("%s flows through %s spider web.", Monnam(mtmp),
                      a_your[trap->madeby_u]);
#else /*KR: KRNethack 맞춤 번역 */
                    pline("%s %s거미줄 사이로 스며나갔다.",
                          append_josa(Monnam(mtmp), "는"),
                          web_you[trap->madeby_u]);
#endif
                    seetrap(trap);
                }
            }
            return TRUE;
        }
        return FALSE;
    }

    /* make a single arrow/dart/rock for a trap to shoot or drop */
    STATIC_OVL struct obj *t_missile(otyp, trap) int otyp;
    struct trap *trap;
    {
        struct obj *otmp = mksobj(otyp, TRUE, FALSE);

        otmp->quan = 1L;
        otmp->owt = weight(otmp);
        otmp->opoisoned = 0;
        otmp->ox = trap->tx, otmp->oy = trap->ty;
        return otmp;
    }

    void set_utrap(tim, typ) unsigned tim, typ;
    {
        u.utrap = tim;
        /* FIXME:
         * utraptype==0 is bear trap rather than 'none'; we probably ought
         * to change that but can't do so until save file compatability is
         * able to be broken.
         */
        u.utraptype = tim ? typ : 0;

        float_vs_flight(); /* maybe block Lev and/or Fly */
    }

    void reset_utrap(msg) boolean msg;
    {
        boolean was_Lev = (Levitation != 0), was_Fly = (Flying != 0);

        set_utrap(0, 0);

        if (msg) {
            if (!was_Lev && Levitation)
                float_up();
            if (!was_Fly && Flying)
                /*KR You("can fly."); */
                You("다시 날 수 있게 되었다.");
        }
    }

    void dotrap(trap, trflags) register struct trap *trap;
    unsigned trflags;
    {
        register int ttype = trap->ttyp;
        struct obj *otmp;
        boolean already_seen = trap->tseen,
                forcetrap = ((trflags & FORCETRAP) != 0
                             || (trflags & FAILEDUNTRAP) != 0),
                webmsgok = (trflags & NOWEBMSG) == 0,
                forcebungle = (trflags & FORCEBUNGLE) != 0,
                plunged = (trflags & TOOKPLUNGE) != 0,
                viasitting = (trflags & VIASITTING) != 0,
                conj_pit = conjoined_pits(trap, t_at(u.ux0, u.uy0), TRUE),
                adj_pit = adj_nonconjoined_pit(trap);
        int oldumort;
        int steed_article = ARTICLE_THE;

        nomul(0);

        /* KMH -- You can't escape the Sokoban level traps */
        if (Sokoban && (is_pit(ttype) || is_hole(ttype))) {
            /* The "air currents" message is still appropriate -- even when
             * the hero isn't flying or levitating -- because it conveys the
             * reason why the player cannot escape the trap with a dexterity
             * check, clinging to the ceiling, etc.
             */
#if 0 /*KR: 원본*/
        pline("Air currents pull you down into %s %s!",
              a_your[trap->madeby_u],
              defsyms[trap_to_defsym(ttype)].explanation);
#else /*KR: KRNethack 맞춤 번역 */
            pline("기류가 당신을 %s%s 안으로 끌어당겼다!",
                  set_you[trap->madeby_u],
                  defsyms[trap_to_defsym(ttype)].explanation);
#endif
            /* then proceed to normal trap effect */
        } else if (already_seen && !forcetrap) {
            if ((Levitation || (Flying && !plunged))
                && (is_pit(ttype) || ttype == HOLE || ttype == BEAR_TRAP)) {
#if 0 /*KR: 원본*/
            You("%s over %s %s.", Levitation ? "float" : "fly",
                a_your[trap->madeby_u],
                defsyms[trap_to_defsym(ttype)].explanation);
#else /*KR: KRNethack 맞춤 번역 */
                You("%s%s 위를 %s.", set_you[trap->madeby_u],
                    defsyms[trap_to_defsym(ttype)].explanation,
                    Levitation ? "떠서 지나갔다" : "날아 지나갔다");
#endif
                return;
            }
            if (!Fumbling && ttype != MAGIC_PORTAL
                && ttype != VIBRATING_SQUARE && ttype != ANTI_MAGIC
                && !forcebungle && !plunged && !conj_pit && !adj_pit
                && (!rn2(5)
                    || (is_pit(ttype) && is_clinger(youmonst.data)))) {
#if 0 /*KR: 원본*/
                You("escape %s %s.", (ttype == ARROW_TRAP && !trap->madeby_u)
                                     ? "an"
                                     : a_your[trap->madeby_u],
                defsyms[trap_to_defsym(ttype)].explanation);
#else /*KR: KRNethack 맞춤 번역 */
                You("%s%s 피했다.", set_you[trap->madeby_u],
                    append_josa(defsyms[trap_to_defsym(ttype)].explanation, "을"));
#endif
                return;
            }
        }

        if (u.usteed) {
            u.usteed->mtrapseen |= (1 << (ttype - 1));
            /* suppress article in various steed messages when using its
               name (which won't occur when hallucinating) */
            if (has_mname(u.usteed) && !Hallucination)
                steed_article = ARTICLE_NONE;
        }

        switch (ttype) {
        case ARROW_TRAP:
            if (trap->once && trap->tseen && !rn2(15)) {
                /*KR You_hear("a loud click!"); */
                You("딸깍 하는 커다란 소리를 들었다!");
                deltrap(trap);
                newsym(u.ux, u.uy);
                break;
            }
            trap->once = 1;
            seetrap(trap);
            /*KR pline("An arrow shoots out at you!"); */
            pline("당신을 향해 화살이 발사되었다!");
            otmp = t_missile(ARROW, trap);
            if (u.usteed && !rn2(2) && steedintrap(trap, otmp)) {
                ; /* nothing */
#if 0 /*KR: 원본*/
            } else if (thitu(8, dmgval(otmp, &youmonst), &otmp, "arrow")) {
#else /*KR: KRNethack 맞춤 번역 */
            } else if (thitu(8, dmgval(otmp, &youmonst), &otmp, "화살")) {
#endif
                if (otmp)
                    obfree(otmp, (struct obj *) 0);
            } else {
                place_object(otmp, u.ux, u.uy);
                if (!Blind)
                    otmp->dknown = 1;
                stackobj(otmp);
                newsym(u.ux, u.uy);
            }
            break;

        case DART_TRAP:
            if (trap->once && trap->tseen && !rn2(15)) {
                /*KR You_hear("a soft click."); */
                You("작게 딸깍 하는 소리를 들었다.");
                deltrap(trap);
                newsym(u.ux, u.uy);
                break;
            }
            trap->once = 1;
            seetrap(trap);
            /*KR pline("A little dart shoots out at you!"); */
            pline("당신을 향해 작은 다트가 발사되었다!");
            otmp = t_missile(DART, trap);
            if (!rn2(6))
                otmp->opoisoned = 1;
            oldumort = u.umortality;
            if (u.usteed && !rn2(2) && steedintrap(trap, otmp)) {
                ; /* nothing */
#if 0 /*KR: 원본*/
            } else if (thitu(7, dmgval(otmp, &youmonst), &otmp, "little dart")) {
#else /*KR: KRNethack 맞춤 번역 */
            } else if (thitu(7, dmgval(otmp, &youmonst), &otmp, "작은 다트")) {
#endif
                if (otmp) {
                    if (otmp->opoisoned)
                   /*KR poisoned("dart", A_CON, "little dart", */
                        poisoned("다트", A_CON, "작은 다트",
                                 (u.umortality > oldumort) ? 0 : 10, TRUE);
                    obfree(otmp, (struct obj *) 0);
                }
            } else {
                place_object(otmp, u.ux, u.uy);
                if (!Blind)
                    otmp->dknown = 1;
                stackobj(otmp);
                newsym(u.ux, u.uy);
            }
            break;

        case ROCKTRAP:
            if (trap->once && trap->tseen && !rn2(15)) {
           /*KR pline("A trap door in %s opens, but nothing falls out!", */
                pline("%s의 다락문이 열렸지만, 아무것도 떨어지지 않았다!",
                      the(ceiling(u.ux, u.uy)));
                deltrap(trap);
                newsym(u.ux, u.uy);
            } else {
                int dmg = d(2, 6); /* should be std ROCK dmg? */

                trap->once = 1;
                feeltrap(trap);
                otmp = t_missile(ROCK, trap);
                place_object(otmp, u.ux, u.uy);

#if 0 /*KR: 원본*/
            pline("A trap door in %s opens and %s falls on your %s!",
                  the(ceiling(u.ux, u.uy)), an(xname(otmp)), body_part(HEAD));
#else /*KR: KRNethack 맞춤 번역 */
                pline("%s의 다락문이 열리며 %s 당신의 %s 위로 떨어졌다!",
                      the(ceiling(u.ux, u.uy)),
                      append_josa(xname(otmp), "이"), body_part(HEAD));
#endif
                if (uarmh) {
                    if (is_metallic(uarmh)) {
                        /*KR pline("Fortunately, you are wearing a hard
                         * helmet."); */
                        pline("다행히도, 당신은 단단한 투구를 쓰고 있었다.");
                        dmg = 2;
                    } else if (flags.verbose) {
#if 0 /*KR: 원본*/
                        pline("%s does not protect you.", Yname2(uarmh));
#else /*KR: KRNethack 맞춤 번역 */
                        pline("%s 당신을 충분히 보호해주지 못했다.",
                              append_josa(Yname2(uarmh), "은"));
#endif
                    }
                }
                if (!Blind)
                    otmp->dknown = 1;
                stackobj(otmp);
                newsym(u.ux, u.uy); /* map the rock */

                /*KR losehp(Maybe_Half_Phys(dmg), "falling rock",
                 * KILLED_BY_AN); */
                losehp(Maybe_Half_Phys(dmg), "떨어지는 바위", KILLED_BY_AN);
                exercise(A_STR, FALSE);
            }
            break;

        case SQKY_BOARD: /* stepped on a squeaky board */
            if ((Levitation || Flying) && !forcetrap) {
                if (!Blind) {
                    seetrap(trap);
                    if (Hallucination)
                        /*KR You("notice a crease in the linoleum."); */
                        You("장판의 주름을 알아차렸다.");
                    else
                        /*KR You("notice a loose board below you."); */
                        You("발밑에 헐거운 판자가 있는 것을 알아차렸다.");
                }
            } else {
                seetrap(trap);
#if 0 /*KR: 원본*/
            pline("A board beneath you %s%s%s.",
                  Deaf ? "vibrates" : "squeaks ",
                  Deaf ? "" : trapnote(trap, 0), Deaf ? "" : " loudly");
#else /*KR: KRNethack 맞춤 번역 */
                if (Deaf) {
                    pline("발밑의 판자가 진동했다.");
                } else {
                    pline("발밑의 판자가 크게 %s 소리를 내며 삐걱거렸다.",
                          trapnote(trap, 0));
                }
#endif
                wake_nearby();
            }
            break;

        case BEAR_TRAP: {
            int dmg = d(2, 4);

            if ((Levitation || Flying) && !forcetrap)
                break;
            feeltrap(trap);
            if (amorphous(youmonst.data) || is_whirly(youmonst.data)
                || unsolid(youmonst.data)) {
#if 0 /*KR: 원본*/
            pline("%s bear trap closes harmlessly through you.",
                  A_Your[trap->madeby_u]);
#else /*KR: KRNethack 맞춤 번역 */
                pline("%s곰 덫이 닫혔지만 당신을 그대로 통과했다.",
                      set_you[trap->madeby_u]);
#endif
                break;
            }
            if (!u.usteed && youmonst.data->msize <= MZ_SMALL) {
#if 0 /*KR: 원본*/
            pline("%s bear trap closes harmlessly over you.",
                  A_Your[trap->madeby_u]);
#else /*KR: KRNethack 맞춤 번역 */
                pline("%s곰 덫이 닫혔지만 당신은 그 위를 지나갔다.",
                      set_you[trap->madeby_u]);
#endif
                break;
            }
            set_utrap((unsigned) rn1(4, 4), TT_BEARTRAP);
            if (u.usteed) {
#if 0 /*KR: 원본*/
            pline("%s bear trap closes on %s %s!", A_Your[trap->madeby_u],
                  s_suffix(mon_nam(u.usteed)), mbodypart(u.usteed, FOOT));
#else /*KR: KRNethack 맞춤 번역 */
                pline("%s곰 덫이 %s의 %s에 닫혔다!", set_you[trap->madeby_u],
                      mon_nam(u.usteed), mbodypart(u.usteed, FOOT));
#endif
                if (thitm(0, u.usteed, (struct obj *) 0, dmg, FALSE))
                    reset_utrap(TRUE); /* steed died, hero not trapped */
            } else {
#if 0 /*KR: 원본*/
            pline("%s bear trap closes on your %s!", A_Your[trap->madeby_u],
                  body_part(FOOT));
#else /*KR: KRNethack 맞춤 번역 */
                pline("%s곰 덫이 당신의 %s에 닫혔다!",
                      set_you[trap->madeby_u], body_part(FOOT));
#endif
                set_wounded_legs(rn2(2) ? RIGHT_SIDE : LEFT_SIDE,
                                 rn1(10, 10));
                if (u.umonnum == PM_OWLBEAR || u.umonnum == PM_BUGBEAR)
                    /*KR You("howl in anger!"); */
                    You("분노의 포효를 내질렀다!");
                /*KR losehp(Maybe_Half_Phys(dmg), "bear trap", KILLED_BY_AN);
                 */
                losehp(Maybe_Half_Phys(dmg), "곰 덫", KILLED_BY_AN);
            }
            exercise(A_DEX, FALSE);
            break;
        }

        case SLP_GAS_TRAP:
            seetrap(trap);
            if (Sleep_resistance || breathless(youmonst.data)) {
                /*KR You("are enveloped in a cloud of gas!"); */
                You("가스 구름에 휩싸였다!");
            } else {
                /*KR pline("A cloud of gas puts you to sleep!"); */
                pline("가스 구름이 당신을 잠재웠다!");
                fall_asleep(-rnd(25), TRUE);
            }
            (void) steedintrap(trap, (struct obj *) 0);
            break;

        case RUST_TRAP:
            seetrap(trap);

            /* Unlike monsters, traps cannot aim their rust attacks at
             * you, so instead of looping through and taking either the
             * first rustable one or the body, we take whatever we get,
             * even if it is not rustable.
             */
            switch (rn2(5)) {
            case 0:
                /*KR pline("%s you on the %s!", A_gush_of_water_hits,
                 * body_part(HEAD)); */
                pline("물이 뿜어져 나와 당신의 %s에 맞았다!",
                      body_part(HEAD));
                (void) water_damage(uarmh, helm_simple_name(uarmh), TRUE);
                break;
            case 1:
                /*KR pline("%s your left %s!", A_gush_of_water_hits,
                 * body_part(ARM)); */
                pline("물이 뿜어져 나와 당신의 왼쪽 %s에 맞았다!",
                      body_part(ARM));
                /*KR if (water_damage(uarms, "shield", TRUE) != ER_NOTHING) */
                if (water_damage(uarms, "방패", TRUE) != ER_NOTHING)
                    break;
                if (u.twoweap || (uwep && bimanual(uwep)))
                    (void) water_damage(u.twoweap ? uswapwep : uwep, 0, TRUE);
            glovecheck:
                /*KR (void) water_damage(uarmg, "gauntlets", TRUE); */
                (void) water_damage(uarmg, "건틀릿", TRUE);
                /* Not "metal gauntlets" since it gets called
                 * even if it's leather for the message
                 */
                break;
            case 2:
                /*KR pline("%s your right %s!", A_gush_of_water_hits,
                 * body_part(ARM)); */
                pline("물이 뿜어져 나와 당신의 오른쪽 %s에 맞았다!",
                      body_part(ARM));
                (void) water_damage(uwep, 0, TRUE);
                goto glovecheck;
            default:
                /*KR pline("%s you!", A_gush_of_water_hits); */
                pline("물이 뿜어져 나와 당신에게 맞았다!");
                for (otmp = invent; otmp; otmp = otmp->nobj)
                    if (otmp->lamplit && otmp != uwep
                        && (otmp != uswapwep || !u.twoweap))
                        (void) snuff_lit(otmp);
                if (uarmc)
                    (void) water_damage(uarmc, cloak_simple_name(uarmc),
                                        TRUE);
                else if (uarm)
                    (void) water_damage(uarm, suit_simple_name(uarm), TRUE);
                else if (uarmu)
                    /*KR (void) water_damage(uarmu, "shirt", TRUE); */
                    (void) water_damage(uarmu, "셔츠", TRUE);
            }
            update_inventory();

            if (u.umonnum == PM_IRON_GOLEM) {
                int dam = u.mhmax;

                /*KR You("are covered with rust!"); */
                You("녹으로 뒤덮였다!");
                /*KR losehp(Maybe_Half_Phys(dam), "rusting away", KILLED_BY);
                 */
                losehp(Maybe_Half_Phys(dam), "녹슬어 버림", KILLED_BY);
            } else if (u.umonnum == PM_GREMLIN && rn2(3)) {
                (void) split_mon(&youmonst, (struct monst *) 0);
            }

            break;

        case FIRE_TRAP:
            seetrap(trap);
            dofiretrap((struct obj *) 0);
            break;

        case PIT:
        case SPIKED_PIT:
            /* KMH -- You can't escape the Sokoban level traps */
            if (!Sokoban && (Levitation || (Flying && !plunged)))
                break;
            feeltrap(trap);
            if (!Sokoban && is_clinger(youmonst.data) && !plunged) {
                if (trap->tseen) {
#if 0 /*KR: 원본*/
                You_see("%s %spit below you.", a_your[trap->madeby_u],
                        ttype == SPIKED_PIT ? "spiked " : "");
#else /*KR: KRNethack 맞춤 번역 */
                    pline("발밑에 %s%s구덩이가 있는 것을 발견했다.",
                          dig_you[trap->madeby_u],
                          ttype == SPIKED_PIT ? "철가시가 박힌 " : "");
#endif
                } else {
#if 0 /*KR: 원본*/
                pline("%s pit %sopens up under you!", A_Your[trap->madeby_u],
                      ttype == SPIKED_PIT ? "full of spikes " : "");
#else /*KR: KRNethack 맞춤 번역 */
                    pline("%s%s구덩이가 발밑에 열렸다!",
                          dig_you[trap->madeby_u],
                          ttype == SPIKED_PIT ? "철가시가 박힌 " : "");
#endif
                    /*KR You("don't fall in!"); */
                    pline("하지만 당신은 떨어지지 않았다!");
                }
                break;
            }
#if 0 /*KR: 원본*/
            if (!Sokoban) {
                char verbbuf[BUFSZ];

                *verbbuf = '\0';
                if (u.usteed) {
                    if ((trflags & RECURSIVETRAP) != 0)
                        Sprintf(verbbuf, "and %s fall",
                                x_monnam(u.usteed, steed_article, (char *) 0,
                                         SUPPRESS_SADDLE, FALSE));
                    else
                        Sprintf(verbbuf, "lead %s",
                                x_monnam(u.usteed, steed_article, "poor",
                                         SUPPRESS_SADDLE, FALSE));
                } else if (conj_pit) {
                    You("move into an adjacent pit.");
                } else if (adj_pit) {
                    You("stumble over debris%s.",
                        !rn2(5) ? " between the pits" : "");
                } else {
                    Strcpy(verbbuf,
                           !plunged ? "fall" : (Flying ? "dive" : "plunge"));
                }
                if (*verbbuf)
                    You("%s into %s pit!", verbbuf, a_your[trap->madeby_u]);
            }
#else /*KR: KRNethack 맞춤 번역 */
            if (!Sokoban) {
                if (u.usteed) {
                    if ((trflags & RECURSIVETRAP) != 0) {
                        pline("당신과 %s %s구덩이로 떨어졌다!",
                              append_josa(x_monnam(u.usteed, steed_article,
                                                   (char *) 0,
                                                   SUPPRESS_SADDLE, FALSE),
                                          "는"),
                              dig_you[trap->madeby_u]);
                    } else {
                        You("불쌍한 %s %s구덩이로 이끌었다!",
                            append_josa(x_monnam(u.usteed, steed_article,
                                                 (char *) 0, SUPPRESS_SADDLE,
                                                 FALSE),
                                        "를"),
                            dig_you[trap->madeby_u]);
                    }
                } else if (conj_pit) {
                    You("인접한 구덩이로 이동했다.");
                } else if (adj_pit) {
                    You("%s잔해물에 걸려 넘어졌다.",
                        !rn2(5) ? "구덩이 사이의 " : "");
                } else {
                    You("%s구덩이로 %s!", dig_you[trap->madeby_u],
                        !plunged ? "떨어졌다"
                                 : (Flying ? "급강하했다" : "돌진했다"));
                }
            }
#endif
            /* wumpus reference */
            if (Role_if(PM_RANGER) && !trap->madeby_u && !trap->once
                && In_quest(&u.uz) && Is_qlocate(&u.uz)) {
                /*KR pline("Fortunately it has a bottom after all..."); */
                pline("다행히도 결국 바닥이 있었다...");
                trap->once = 1;
            } else if (u.umonnum == PM_PIT_VIPER
                       || u.umonnum == PM_PIT_FIEND) {
                /*KR pline("How pitiful.  Isn't that the pits?"); */
                pline("참 가련하기도 하지. 이 구덩이들 말이야.");
            }
            if (ttype == SPIKED_PIT) {
                /*KR const char *predicament = "on a set of sharp iron
                 * spikes"; */
                const char *predicament = "날카로운 철가시 더미 위로";

                if (u.usteed) {
#if 0 /*KR: 원본*/
                pline("%s %s %s!",
                      upstart(x_monnam(u.usteed, steed_article, "poor",
                                       SUPPRESS_SADDLE, FALSE)),
                      conj_pit ? "steps" : "lands", predicament);
#else /*KR: KRNethack 맞춤 번역 */
                    pline("%s %s %s!",
                          append_josa(upstart(x_monnam(
                                          u.usteed, steed_article, (char *) 0,
                                          SUPPRESS_SADDLE, FALSE)),
                                      "이"),
                          predicament,
                          conj_pit ? "미끄러졌다" : "내려앉았다");
#endif
                } else
                    /*KR You("%s %s!", conj_pit ? "step" : "land",
                     * predicament); */
                    You("%s %s!", predicament,
                        conj_pit ? "미끄러졌다" : "내려앉았다");
            }
            /* FIXME:
             * if hero gets killed here, setting u.utrap in advance will
             * show "you were trapped in a pit" during disclosure's display
             * of enlightenment, but hero is dying *before* becoming trapped.
             */
            set_utrap((unsigned) rn1(6, 2), TT_PIT);
            if (!steedintrap(trap, (struct obj *) 0)) {
                if (ttype == SPIKED_PIT) {
                    oldumort = u.umortality;
#if 0 /*KR: 원본*/
                losehp(Maybe_Half_Phys(rnd(conj_pit ? 4 : adj_pit ? 6 : 10)),
                       /* note: these don't need locomotion() handling;
                          if fatal while poly'd and Unchanging, the
                          death reason will be overridden with
                          "killed while stuck in creature form" */
                       plunged
                          ? "deliberately plunged into a pit of iron spikes"
                          : conj_pit
                              ? "stepped into a pit of iron spikes"
                              : adj_pit
                                 ? "stumbled into a pit of iron spikes"
                                 : "fell into a pit of iron spikes",
                       NO_KILLER_PREFIX);
#else /*KR: KRNethack 맞춤 번역 */
                losehp(Maybe_Half_Phys(rnd(conj_pit  ? 4 : adj_pit ? 6 : 10)),
                       plunged ? "철가시 구덩이 속으로 일부러 돌진함"
                          : conj_pit ? "철가시 구덩이로 미끄러짐"
                          : adj_pit ? "철가시 구덩이로 비틀거리며 들어감"
                                     : "철가시 구덩이로 추락함",
                           KILLED_BY);
#endif
                    if (!rn2(6))
#if 0 /*KR: 원본*/
                    poisoned("spikes", A_STR,
                             (conj_pit || adj_pit)
                                 ? "stepping on poison spikes"
                                 : "fall onto poison spikes",
                             /* if damage triggered life-saving,
                                poison is limited to attrib loss */
                             (u.umortality > oldumort) ? 0 : 8, FALSE);
#else /*KR: KRNethack 맞춤 번역 */
                        poisoned("철가시", A_STR,
                                 (conj_pit || adj_pit)
                                     ? "독이 묻은 철가시를 밟음"
                                     : "독이 묻은 철가시 위로 추락함",
                                 (u.umortality > oldumort) ? 0 : 8, FALSE);
#endif
                } else {
                    /* plunging flyers take spike damage but not pit damage */
                    if (!conj_pit
                        && !(plunged
                             && (Flying || is_clinger(youmonst.data))))
#if 0 /*KR: 원본*/
                    losehp(Maybe_Half_Phys(rnd(adj_pit ? 3 : 6)),
                           plunged ? "deliberately plunged into a pit"
                                   : "fell into a pit",
                           NO_KILLER_PREFIX);
#else /*KR: KRNethack 맞춤 번역 */
                        losehp(Maybe_Half_Phys(rnd(adj_pit ? 3 : 6)),
                               plunged ? "구덩이 속으로 일부러 돌진함"
                                       : "구덩이 속으로 추락함",
                               KILLED_BY);
#endif
                }
                if (Punished && !carried(uball)) {
                    unplacebc();
                    ballfall();
                    placebc();
                }
                if (!conj_pit)
                    /*KR selftouch("Falling, you"); */
                    selftouch("추락하면서, 당신은");
                vision_full_recalc = 1; /* vision limits change */
                exercise(A_STR, FALSE);
                exercise(A_DEX, FALSE);
            }
            break;

        case HOLE:
        case TRAPDOOR:
            if (!Can_fall_thru(&u.uz)) {
                seetrap(trap); /* normally done in fall_through */
                impossible("dotrap: %ss cannot exist on this level.",
                           defsyms[trap_to_defsym(ttype)].explanation);
                break; /* don't activate it after all */
            }
            fall_through(TRUE, (trflags & TOOKPLUNGE));
            break;

        case TELEP_TRAP:
            seetrap(trap);
            tele_trap(trap);
            break;

        case LEVEL_TELEP:
            seetrap(trap);
            level_tele_trap(trap, trflags);
            break;

        case WEB: /* Our luckless player has stumbled into a web. */
            feeltrap(trap);
            if (mu_maybe_destroy_web(&youmonst, webmsgok, trap))
                break;
            if (webmaker(youmonst.data)) {
                if (webmsgok)
                    /*KR pline(trap->madeby_u ? "You take a walk on your web."
                     * : "There is a spider web here."); */
                    pline(trap->madeby_u ? "자신이 친 거미줄 위를 걸어갔다."
                                         : "여기에 거미줄이 있다.");
                break;
            }
            if (webmsgok) {
                char verbbuf[BUFSZ];

#if 0 /*KR: 원본*/
            if (forcetrap || viasitting) {
                Strcpy(verbbuf, "are caught by");
            } else if (u.usteed) {
                Sprintf(verbbuf, "lead %s into",
                        x_monnam(u.usteed, steed_article, "poor",
                                 SUPPRESS_SADDLE, FALSE));
            } else {
                Sprintf(verbbuf, "%s into",
                        Levitation ? (const char *) "float"
                                   : locomotion(youmonst.data, "stumble"));
            }
            You("%s %s spider web!", verbbuf, a_your[trap->madeby_u]);
#else /*KR: KRNethack 맞춤 번역 */
                if (forcetrap) {
                    Strcpy(verbbuf, "당신은 ");
                } else if (u.usteed) {
                    Sprintf(verbbuf, "당신과 불쌍한 %s ",
                            x_monnam(u.usteed, steed_article, (char *) 0,
                                     SUPPRESS_SADDLE, FALSE));
                } else {
                    Sprintf(verbbuf, "당신은 %s ",
                            Levitation ? (const char *) "공중에 뜬 채로"
                                       : "비틀거리면서");
                }
                pline("%s%s거미줄에 걸렸다!", verbbuf,
                      web_you[trap->madeby_u]);
#endif
            }

            /* time will be adjusted below */
            set_utrap(1, TT_WEB);

            /* Time stuck in the web depends on your/steed strength. */
            {
                int tim, str = ACURR(A_STR);

                /* If mounted, the steed gets trapped.  Use mintrap
                 * to do all the work.  If mtrapped is set as a result,
                 * unset it and set utrap instead.  In the case of a
                 * strongmonst and mintrap said it's trapped, use a
                 * short but non-zero trap time.  Otherwise, monsters
                 * have no specific strength, so use player strength.
                 * This gets skipped for webmsgok, which implies that
                 * the steed isn't a factor.
                 */
                if (u.usteed && webmsgok) {
                    /* mtmp location might not be up to date */
                    u.usteed->mx = u.ux;
                    u.usteed->my = u.uy;

                    /* mintrap currently does not return 2(died) for webs */
                    if (mintrap(u.usteed)) {
                        u.usteed->mtrapped = 0;
                        if (strongmonst(u.usteed->data))
                            str = 17;
                    } else {
                        reset_utrap(FALSE);
                        break;
                    }

                    webmsgok = FALSE; /* mintrap printed the messages */
                }
                if (str <= 3)
                    tim = rn1(6, 6);
                else if (str < 6)
                    tim = rn1(6, 4);
                else if (str < 9)
                    tim = rn1(4, 4);
                else if (str < 12)
                    tim = rn1(4, 2);
                else if (str < 15)
                    tim = rn1(2, 2);
                else if (str < 18)
                    tim = rnd(2);
                else if (str < 69)
                    tim = 1;
                else {
                    tim = 0;
                    if (webmsgok)
#if 0 /*KR: 원본*/
                        You("tear through %s web!", a_your[trap->madeby_u]);
#else
                        You("%s거미줄을 찢어버렸다!", web_you[trap->madeby_u]);
#endif
                    deltrap(trap);
                    newsym(u.ux, u.uy); /* get rid of trap symbol */
                }
                set_utrap((unsigned) tim, TT_WEB);
            }
            break;

        case STATUE_TRAP:
            (void) activate_statue_trap(trap, u.ux, u.uy, FALSE);
            break;

        case MAGIC_TRAP: /* A magic trap. */
            seetrap(trap);
            if (!rn2(30)) {
                deltrap(trap);
                newsym(u.ux, u.uy); /* update position */
                /*KR You("are caught in a magical explosion!"); */
                You("마법 폭발에 휘말렸다!");
                /*KR losehp(rnd(10), "magical explosion", KILLED_BY_AN); */
                losehp(rnd(10), "마법 폭발", KILLED_BY_AN);
                /*KR Your("body absorbs some of the magical energy!"); */
                Your("몸이 마법 에너지를 약간 흡수했다!");
                u.uen = (u.uenmax += 2);
                break;
            } else {
                domagictrap();
            }
            (void) steedintrap(trap, (struct obj *) 0);
            break;

        case ANTI_MAGIC:
            seetrap(trap);
            /* hero without magic resistance loses spell energy,
               hero with magic resistance takes damage instead;
               possibly non-intuitive but useful for play balance */
            if (!Antimagic) {
                drain_en(rnd(u.ulevel) + 1);
            } else {
                int dmgval2 = rnd(4), hp = Upolyd ? u.mh : u.uhp;

                /* Half_XXX_damage has opposite its usual effect (approx)
                   but isn't cumulative if hero has more than one */
                if (Half_physical_damage || Half_spell_damage)
                    dmgval2 += rnd(4);
                /* give Magicbane wielder dose of own medicine */
                if (uwep && uwep->oartifact == ART_MAGICBANE)
                    dmgval2 += rnd(4);
                /* having an artifact--other than own quest one--which
                   confers magic resistance simply by being carried
                   also increases the effect */
                for (otmp = invent; otmp; otmp = otmp->nobj)
                    if (otmp->oartifact && !is_quest_artifact(otmp)
                        && defends_when_carried(AD_MAGM, otmp))
                        break;
                if (otmp)
                    dmgval2 += rnd(4);
                if (Passes_walls)
                    dmgval2 = (dmgval2 + 3) / 4;

#if 0 /*KR: 원본*/
            You_feel((dmgval2 >= hp) ? "unbearably torpid!"
                                     : (dmgval2 >= hp / 4) ? "very lethargic."
                                                           : "sluggish.");
#else /*KR: KRNethack 맞춤 번역 */
            You((dmgval2 >= hp) ? "견딜 수 없을 정도로 무기력해진 것을 느꼈다!"
                                     : (dmgval2 >= hp / 4) ? "매우 나른해진 것을 느꼈다."
                                                           : "게을러진 것을 느꼈다.");
#endif
                /* opposite of magical explosion */
           /*KR losehp(dmgval2, "anti-magic implosion", KILLED_BY_AN); */
                losehp(dmgval2, "반마법 붕괴", KILLED_BY_AN);
            }
            break;

        case POLY_TRAP: {
            char verbbuf[BUFSZ];

            seetrap(trap);
            if (viasitting)
                /*KR Strcpy(verbbuf, "trigger"); */
                Strcpy(verbbuf, "작동시켰다"); /* follows "You sit down." */
            else if (u.usteed)
#if 0 /*KR: 원본*/
            Sprintf(verbbuf, "lead %s onto",
                    x_monnam(u.usteed, steed_article, (char *) 0,
                             SUPPRESS_SADDLE, FALSE));
#else /*KR: KRNethack 맞춤 번역 */
            Sprintf(verbbuf, "%s와 함께 올라탔다",
                    x_monnam(u.usteed, steed_article, (char *) 0,
                             SUPPRESS_SADDLE, FALSE));
#endif
            else
#if 0 /*KR: 원본*/
            Sprintf(verbbuf, "%s onto",
                    Levitation ? (const char *) "float"
                               : locomotion(youmonst.data, "step"));
#else /*KR: KRNethack 맞춤 번역 */
            Sprintf(verbbuf, " %s",
                    Levitation ? (const char *) "공중에 뜬 채로 다가갔다"
                               : "발을 들여놓았다");
#endif
            /*KR You("%s a polymorph trap!", verbbuf); */
            You("폴리모프 함정%s!", verbbuf);
            if (Antimagic || Unchanging) {
                shieldeff(u.ux, u.uy);
                /*KR You_feel("momentarily different."); */
                You("순간적으로 다른 느낌이 들었다.");
                /* Trap did nothing; don't remove it --KAA */
            } else {
                (void) steedintrap(trap, (struct obj *) 0);
                deltrap(trap);      /* delete trap before polymorph */
                newsym(u.ux, u.uy); /* get rid of trap symbol */
                /*KR You_feel("a change coming over you."); */
                You("변화가 찾아오는 것을 느꼈다.");
                polyself(0);
            }
            break;
        }
        case LANDMINE: {
            unsigned steed_mid = 0;
            struct obj *saddle = 0;

            if ((Levitation || Flying) && !forcetrap) {
                if (!already_seen && rn2(3))
                    break;
                feeltrap(trap);
#if 0 /*KR: 원본*/
                pline("%s %s in a pile of soil below you.",
                      already_seen ? "There is" : "You discover",
                      trap->madeby_u ? "the trigger of your mine" 
                                     : "a trigger"); 
#else
                if (already_seen)
                    pline("여기에 %s지뢰의 기폭 장치가 있다.",
                          set_you[trap->madeby_u]);
                else
                    You("발밑 흙더미에서 %s지뢰의 기폭 장치를 발견했다.",
                        set_you[trap->madeby_u]);
#endif
                if (already_seen && rn2(3))
                    break;
#if 0 /*KR: 원본*/
                pline("KAABLAMM!!!  %s %s%s off!",
                  forcebungle ? "Your inept attempt sets"
                              : "The air currents set",
                  already_seen ? a_your[trap->madeby_u] : "",
                  already_seen ? " land mine" : "it");
#else /*KR: KRNethack 맞춤 번역 */
                pline("콰쾅!!! %s%s%s 작동시켰다!",
                      forcebungle ? "서툰 솜씨로 " : "기류가 ",
                      already_seen ? set_you[trap->madeby_u] : "",
                      already_seen ? "지뢰를 " : "그것을 ");
#endif
            } else {
                /* prevent landmine from killing steed, throwing you to
                 * the ground, and you being affected again by the same
                 * mine because it hasn't been deleted yet
                 */
                static boolean recursive_mine = FALSE;

                if (recursive_mine)
                    break;
                feeltrap(trap);
                /*KR pline("KAABLAMM!!!  You triggered %s land mine!",
                 * a_your[trap->madeby_u]); */
                pline("콰쾅!!! %s지뢰를 밟았다!", set_you[trap->madeby_u]);
                if (u.usteed)
                    steed_mid = u.usteed->m_id;
                recursive_mine = TRUE;
                (void) steedintrap(trap, (struct obj *) 0);
                recursive_mine = FALSE;
                saddle = sobj_at(SADDLE, u.ux, u.uy);
                set_wounded_legs(LEFT_SIDE, rn1(35, 41));
                set_wounded_legs(RIGHT_SIDE, rn1(35, 41));
                exercise(A_DEX, FALSE);
            }
            blow_up_landmine(trap);
            if (steed_mid && saddle && !u.usteed)
                (void) keep_saddle_with_steedcorpse(steed_mid, fobj, saddle);
            newsym(u.ux, u.uy); /* update trap symbol */
            /*KR losehp(Maybe_Half_Phys(rnd(16)), "land mine", KILLED_BY_AN);
             */
            losehp(Maybe_Half_Phys(rnd(16)), "지뢰", KILLED_BY_AN);
            /* fall recursively into the pit... */
            if ((trap = t_at(u.ux, u.uy)) != 0)
                dotrap(trap, RECURSIVETRAP);
            fill_pit(u.ux, u.uy);
            break;
        }

        case ROLLING_BOULDER_TRAP: {
            int style = ROLL | (trap->tseen ? LAUNCH_KNOWN : 0);

            feeltrap(trap);
            /*KR pline("Click!  You trigger a rolling boulder trap!"); */
            pline("딸깍! 구르는 바위 함정을 작동시켰다!");
            if (!launch_obj(BOULDER, trap->launch.x, trap->launch.y,
                            trap->launch2.x, trap->launch2.y, style)) {
                deltrap(trap);
                newsym(u.ux, u.uy); /* get rid of trap symbol */
           /*KR pline("Fortunately for you, no boulder was released."); */
                pline("운 좋게도, 바위는 굴러오지 않았다.");
            }
            break;
        }

        case MAGIC_PORTAL:
            feeltrap(trap);
            domagicportal(trap);
            break;

        case VIBRATING_SQUARE:
            feeltrap(trap);
            /* messages handled elsewhere; the trap symbol is merely to mark
             * the square for future reference */
            break;

        default:
            feeltrap(trap);
            impossible("You hit a trap of type %u", trap->ttyp);
        }
    }

STATIC_OVL char *
trapnote(trap, noprefix) 
struct trap *trap;
boolean noprefix;
{
        static char tnbuf[12];
        const char *tn,
#if 0 /*KR:T*/
        *tnnames[12] = { "C note",  "D flat", "D note",  "E flat",
                         "E note",  "F note", "F sharp", "G note",
                         "G sharp", "A note", "B flat",  "B note" };
#else
            *tnnames[12] = { "도",    "레 플랫", "레",      "미 플랫",
                             "미",    "파",      "파 샵",   "솔",
                             "솔 샵", "라",      "시 플랫", "시" };
#endif

        tnbuf[0] = '\0';
        tn = tnnames[trap->tnote];
#if 0 /*KR: 원본*/
    if (!noprefix)
        Sprintf(tnbuf, "%s ",
                (*tn == 'A' || *tn == 'E' || *tn == 'F') ? "an" : "a");
    Sprintf(eos(tnbuf), "%s", tn);
#else /*KR: KRNethack 맞춤 번역 */
        Sprintf(tnbuf, "%s 음", tn);
#endif
        return tnbuf;
}

    STATIC_OVL int steedintrap(trap, otmp) struct trap *trap;
    struct obj *otmp;
    {
        struct monst *steed = u.usteed;
        int tt;
        boolean trapkilled, steedhit;

        if (!steed || !trap)
            return 0;
        tt = trap->ttyp;
        steed->mx = u.ux;
        steed->my = u.uy;
        trapkilled = steedhit = FALSE;

        switch (tt) {
        case ARROW_TRAP:
            if (!otmp) {
                impossible("steed hit by non-existent arrow?");
                return 0;
            }
            trapkilled = thitm(8, steed, otmp, 0, FALSE);
            steedhit = TRUE;
            break;
        case DART_TRAP:
            if (!otmp) {
                impossible("steed hit by non-existent dart?");
                return 0;
            }
            trapkilled = thitm(7, steed, otmp, 0, FALSE);
            steedhit = TRUE;
            break;
        case SLP_GAS_TRAP:
            if (!resists_sleep(steed) && !breathless(steed->data)
                && !steed->msleeping && steed->mcanmove) {
                if (sleep_monst(steed, rnd(25), -1))
                    /* no in_sight check here; you can feel it even if blind
                     */
                    /*KR pline("%s suddenly falls asleep!", Monnam(steed)); */
                    pline("%s 갑자기 잠들었다!", append_josa(Monnam(steed), "이"));
            }
            steedhit = TRUE;
            break;
        case LANDMINE:
            trapkilled = thitm(0, steed, (struct obj *) 0, rnd(16), FALSE);
            steedhit = TRUE;
            break;
        case PIT:
        case SPIKED_PIT:
            trapkilled = (DEADMONSTER(steed)
                          || thitm(0, steed, (struct obj *) 0,
                                   rnd((tt == PIT) ? 6 : 10), FALSE));
            steedhit = TRUE;
            break;
        case POLY_TRAP:
            if (!resists_magm(steed)
                && !resist(steed, WAND_CLASS, 0, NOTELL)) {
                struct permonst *mdat = steed->data;

                (void) newcham(steed, (struct permonst *) 0, FALSE, FALSE);
                if (!can_saddle(steed) || !can_ride(steed)) {
                    dismount_steed(DISMOUNT_POLY);
                } else {
                    char buf[BUFSZ];

                    Strcpy(buf, x_monnam(steed, ARTICLE_YOUR, (char *) 0,
                                         SUPPRESS_SADDLE, FALSE));
                    if (mdat != steed->data)
                        (void) strsubst(buf, "your ", "your new ");
                    /*KR You("adjust yourself in the saddle on %s.", buf); */
                    You("%s의 안장 위에서 자세를 고쳐 앉았다.", buf);
                }
            }
            steedhit = TRUE;
            break;
        default:
            break;
        }

        if (trapkilled) {
            dismount_steed(DISMOUNT_POLY);
            return 2;
        }
        return steedhit ? 1 : 0;
    }

    /* some actions common to both player and monsters for triggered landmine
     */
    void blow_up_landmine(trap) struct trap *trap;
    {
        int x = trap->tx, y = trap->ty, dbx, dby;
        struct rm *lev = &levl[x][y];

        (void) scatter(x, y, 4,
                       MAY_DESTROY | MAY_HIT | MAY_FRACTURE | VIS_EFFECTS,
                       (struct obj *) 0);
        del_engr_at(x, y);
        wake_nearto(x, y, 400);
        if (IS_DOOR(lev->typ))
            lev->doormask = D_BROKEN;
        /* destroy drawbridge if present */
        if (lev->typ == DRAWBRIDGE_DOWN || is_drawbridge_wall(x, y) >= 0) {
            dbx = x, dby = y;
            /* if under the portcullis, the bridge is adjacent */
            if (find_drawbridge(&dbx, &dby))
                destroy_drawbridge(dbx, dby);
            trap = t_at(x, y); /* expected to be null after destruction */
        }
        /* convert landmine into pit */
        if (trap) {
            if (Is_waterlevel(&u.uz) || Is_airlevel(&u.uz)) {
                /* no pits here */
                deltrap(trap);
            } else {
                trap->ttyp = PIT;       /* explosion creates a pit */
                trap->madeby_u = FALSE; /* resulting pit isn't yours */
                seetrap(trap);          /* and it isn't concealed */
            }
        }
    }

    /*
     * The following are used to track launched objects to
     * prevent them from vanishing if you are killed. They
     * will reappear at the launchplace in bones files.
     */
    static struct {
        struct obj *obj;
        xchar x, y;
    } launchplace;

    STATIC_OVL void launch_drop_spot(obj, x, y) struct obj *obj;
    xchar x, y;
    {
        if (!obj) {
            launchplace.obj = (struct obj *) 0;
            launchplace.x = 0;
            launchplace.y = 0;
        } else {
            launchplace.obj = obj;
            launchplace.x = x;
            launchplace.y = y;
        }
    }

    boolean launch_in_progress()
    {
        if (launchplace.obj)
            return TRUE;
        return FALSE;
    }

    void force_launch_placement()
    {
        if (launchplace.obj) {
            launchplace.obj->otrapped = 0;
            place_object(launchplace.obj, launchplace.x, launchplace.y);
        }
    }

    /*
     * Move obj from (x1,y1) to (x2,y2)
     *
     * Return 0 if no object was launched.
     * 1 if an object was launched and placed somewhere.
     * 2 if an object was launched, but used up.
     */
    int launch_obj(otyp, x1, y1, x2, y2, style) short otyp;
    register int x1, y1, x2, y2;
    int style;
    {
        register struct monst *mtmp;
        register struct obj *otmp, *otmp2;
        register int dx, dy;
        struct obj *singleobj;
        boolean used_up = FALSE;
        boolean otherside = FALSE;
        int dist;
        int tmp;
        int delaycnt = 0;

        otmp = sobj_at(otyp, x1, y1);
        /* Try the other side too, for rolling boulder traps */
        if (!otmp && otyp == BOULDER) {
            otherside = TRUE;
            otmp = sobj_at(otyp, x2, y2);
        }
        if (!otmp)
            return 0;
        if (otherside) { /* swap 'em */
            int tx, ty;

            tx = x1;
            ty = y1;
            x1 = x2;
            y1 = y2;
            x2 = tx;
            y2 = ty;
        }

        if (otmp->quan == 1L) {
            obj_extract_self(otmp);
            singleobj = otmp;
            otmp = (struct obj *) 0;
        } else {
            singleobj = splitobj(otmp, 1L);
            obj_extract_self(singleobj);
        }
        newsym(x1, y1);
        /* in case you're using a pick-axe to chop the boulder that's being
           launched (perhaps a monster triggered it), destroy context so that
           next dig attempt never thinks you're resuming previous effort */
        if ((otyp == BOULDER || otyp == STATUE)
            && singleobj->ox == context.digging.pos.x
            && singleobj->oy == context.digging.pos.y)
            (void) memset((genericptr_t) &context.digging, 0,
                          sizeof(struct dig_info));

        dist = distmin(x1, y1, x2, y2);
        bhitpos.x = x1;
        bhitpos.y = y1;
        dx = sgn(x2 - x1);
        dy = sgn(y2 - y1);
        switch (style) {
        case ROLL | LAUNCH_UNSEEN:
            if (otyp == BOULDER) {
#if 0 /*KR: 원본*/
            You_hear(Hallucination ? "someone bowling."
                                   : "rumbling in the distance.");
#else /*KR: KRNethack 맞춤 번역 */
            You(Hallucination
                             ? "누군가 볼링 치는 듯한 소리를 들었다."
                             : "멀리서 우르릉거리는 소리를 들었다.");
#endif
            }
            style &= ~LAUNCH_UNSEEN;
            goto roll;
        case ROLL | LAUNCH_KNOWN:
            /* use otrapped as a flag to ohitmon */
            singleobj->otrapped = 1;
            style &= ~LAUNCH_KNOWN;
        /* fall through */
        roll:
        case ROLL:
            delaycnt = 2;
        /* fall through */
        default:
            if (!delaycnt)
                delaycnt = 1;
            if (!cansee(bhitpos.x, bhitpos.y))
                curs_on_u();
            tmp_at(DISP_FLASH, obj_to_glyph(singleobj, rn2_on_display_rng));
            tmp_at(bhitpos.x, bhitpos.y);
        }
        /* Mark a spot to place object in bones files to prevent
         * loss of object. Use the starting spot to ensure that
         * a rolling boulder will still launch, which it wouldn't
         * do if left midstream. Unfortunately we can't use the
         * target resting spot, because there are some things/situations
         * that would prevent it from ever getting there (bars), and we
         * can't tell that yet.
         */
        launch_drop_spot(singleobj, bhitpos.x, bhitpos.y);

        /* Set the object in motion */
        while (dist-- > 0 && !used_up) {
            struct trap *t;
            tmp_at(bhitpos.x, bhitpos.y);
            tmp = delaycnt;

            /* dstage@u.washington.edu -- Delay only if hero sees it */
            if (cansee(bhitpos.x, bhitpos.y))
                while (tmp-- > 0)
                    delay_output();

            bhitpos.x += dx;
            bhitpos.y += dy;

            if ((mtmp = m_at(bhitpos.x, bhitpos.y)) != 0) {
                if (otyp == BOULDER && throws_rocks(mtmp->data)) {
                    if (rn2(3)) {
                        if (cansee(bhitpos.x, bhitpos.y))
                       /*KR pline("%s snatches the boulder.", Monnam(mtmp)); */
                            pline("%s 바위를 낚아챘다.", Monnam(mtmp));
                        singleobj->otrapped = 0;
                        (void) mpickobj(mtmp, singleobj);
                        used_up = TRUE;
                        launch_drop_spot((struct obj *) 0, 0, 0);
                        break;
                    }
                }
                if (ohitmon(mtmp, singleobj, (style == ROLL) ? -1 : dist,
                            FALSE)) {
                    used_up = TRUE;
                    launch_drop_spot((struct obj *) 0, 0, 0);
                    break;
                }
            } else if (bhitpos.x == u.ux && bhitpos.y == u.uy) {
                if (multi)
                    nomul(0);
                if (thitu(9 + singleobj->spe, dmgval(singleobj, &youmonst),
                          &singleobj, (char *) 0))
                    stop_occupation();
            }
            if (style == ROLL) {
                if (down_gate(bhitpos.x, bhitpos.y) != -1) {
                    if (ship_object(singleobj, bhitpos.x, bhitpos.y, FALSE)) {
                        used_up = TRUE;
                        launch_drop_spot((struct obj *) 0, 0, 0);
                        break;
                    }
                }
                if ((t = t_at(bhitpos.x, bhitpos.y)) != 0
                    && otyp == BOULDER) {
                    switch (t->ttyp) {
                    case LANDMINE:
                        if (rn2(10) > 2) {
#if 0 /*KR: 원본*/
                        pline("KAABLAMM!!!%s",
                              cansee(bhitpos.x, bhitpos.y)
                                ? " The rolling boulder triggers a land mine."
                                : "");
#else /*KR: KRNethack 맞춤 번역 */
                        pline("콰쾅!!!%s",
                              cansee(bhitpos.x, bhitpos.y)
                                     ? " 굴러가던 바위가 지뢰를 작동시켰다!"
                                     : "");
#endif
                            deltrap(t);
                            del_engr_at(bhitpos.x, bhitpos.y);
                            place_object(singleobj, bhitpos.x, bhitpos.y);
                            singleobj->otrapped = 0;
                            fracture_rock(singleobj);
                            (void) scatter(bhitpos.x, bhitpos.y, 4,
                                           MAY_DESTROY | MAY_HIT
                                               | MAY_FRACTURE | VIS_EFFECTS,
                                           (struct obj *) 0);
                            if (cansee(bhitpos.x, bhitpos.y))
                                newsym(bhitpos.x, bhitpos.y);
                            used_up = TRUE;
                            launch_drop_spot((struct obj *) 0, 0, 0);
                        }
                        break;
                    case LEVEL_TELEP:
                    case TELEP_TRAP:
                        if (cansee(bhitpos.x, bhitpos.y))
                       /*KR pline("Suddenly the rolling boulder disappears!"); */
                            pline("굴러오던 바위가 갑자기 사라졌다!");
                        else
                            /*KR You_hear("a rumbling stop abruptly."); */
                            pline("우르릉거리는 소리가 갑자기 멈췄다.");
                        singleobj->otrapped = 0;
                        if (t->ttyp == TELEP_TRAP)
                            (void) rloco(singleobj);
                        else {
                            int newlev = random_teleport_level();
                            d_level dest;

                            if (newlev == depth(&u.uz) || In_endgame(&u.uz))
                                continue;
                            add_to_migration(singleobj);
                            get_level(&dest, newlev);
                            singleobj->ox = dest.dnum;
                            singleobj->oy = dest.dlevel;
                            singleobj->owornmask = (long) MIGR_RANDOM;
                        }
                        seetrap(t);
                        used_up = TRUE;
                        launch_drop_spot((struct obj *) 0, 0, 0);
                        break;
                    case PIT:
                    case SPIKED_PIT:
                    case HOLE:
                    case TRAPDOOR:
                        /* the boulder won't be used up if there is a
                           monster in the trap; stop rolling anyway */
                        x2 = bhitpos.x, y2 = bhitpos.y; /* stops here */
                   /*KR if (flooreffects(singleobj, x2, y2, "fall")) { */
                        if (flooreffects(singleobj, x2, y2, "추락함")) {
                            used_up = TRUE;
                            launch_drop_spot((struct obj *) 0, 0, 0);
                        }
                        dist = -1; /* stop rolling immediately */
                        break;
                    }
                    if (used_up || dist == -1)
                        break;
                }
           /*KR if (flooreffects(singleobj, bhitpos.x, bhitpos.y, "fall")) { */
                if (flooreffects(singleobj, bhitpos.x, bhitpos.y, "추락함")) {
                    used_up = TRUE;
                    launch_drop_spot((struct obj *) 0, 0, 0);
                    break;
                }
                if (otyp == BOULDER
                    && (otmp2 = sobj_at(BOULDER, bhitpos.x, bhitpos.y))
                           != 0) {
#if 0 /*KR: 원본*/
                const char *bmsg = " as one boulder sets another in motion";
#else /*KR: KRNethack 맞춤 번역 */
                const char *bmsg = " 한 바위가 다른 바위를 밀어내는 듯한";
#endif

                    if (!isok(bhitpos.x + dx, bhitpos.y + dy) || !dist
                        || IS_ROCK(levl[bhitpos.x + dx][bhitpos.y + dy].typ))
                        /*KR bmsg = " as one boulder hits another"; */
                        bmsg = " 한 바위가 다른 바위와 충돌하는 듯한";

                    /*KR You_hear("a loud crash%s!", */
                    You("커다란 충돌음%s이 들렸다!",
                             cansee(bhitpos.x, bhitpos.y) ? bmsg : "");
                    obj_extract_self(otmp2);
                    /* pass off the otrapped flag to the next boulder */
                    otmp2->otrapped = singleobj->otrapped;
                    singleobj->otrapped = 0;
                    place_object(singleobj, bhitpos.x, bhitpos.y);
                    singleobj = otmp2;
                    otmp2 = (struct obj *) 0;
                    wake_nearto(bhitpos.x, bhitpos.y, 10 * 10);
                }
            }
            if (otyp == BOULDER && closed_door(bhitpos.x, bhitpos.y)) {
                if (cansee(bhitpos.x, bhitpos.y))
               /*KR pline_The("boulder crashes through a door."); */
                    pline("바위가 문을 부수고 지나갔다.");
                levl[bhitpos.x][bhitpos.y].doormask = D_BROKEN;
                if (dist)
                    unblock_point(bhitpos.x, bhitpos.y);
            }

            /* if about to hit iron bars, do so now */
            if (dist > 0 && isok(bhitpos.x + dx, bhitpos.y + dy)
                && levl[bhitpos.x + dx][bhitpos.y + dy].typ == IRONBARS) {
                x2 = bhitpos.x, y2 = bhitpos.y; /* object stops here */
                if (hits_bars(&singleobj, x2, y2, x2 + dx, y2 + dy, !rn2(20),
                              0)) {
                    if (!singleobj) {
                        used_up = TRUE;
                        launch_drop_spot((struct obj *) 0, 0, 0);
                    }
                    break;
                }
            }
        }
        tmp_at(DISP_END, 0);
        launch_drop_spot((struct obj *) 0, 0, 0);
        if (!used_up) {
            singleobj->otrapped = 0;
            place_object(singleobj, x2, y2);
            newsym(x2, y2);
            return 1;
        } else
            return 2;
    }

    void seetrap(trap) struct trap *trap;
    {
        if (!trap->tseen) {
            trap->tseen = 1;
            newsym(trap->tx, trap->ty);
        }
    }

    /* like seetrap() but overrides vision */
    void feeltrap(trap) struct trap *trap;
    {
        trap->tseen = 1;
        map_trap(trap, 1);
        /* in case it's beneath something, redisplay the something */
        newsym(trap->tx, trap->ty);
    }

    STATIC_OVL int mkroll_launch(ttmp, x, y, otyp, ocount) struct trap *ttmp;
    xchar x, y;
    short otyp;
    long ocount;
    {
        struct obj *otmp;
        register int tmp;
        schar dx, dy;
        int distance;
        coord cc;
        coord bcc;
        int trycount = 0;
        boolean success = FALSE;
        int mindist = 4;

        if (ttmp->ttyp == ROLLING_BOULDER_TRAP)
            mindist = 2;
        distance = rn1(5, 4); /* 4..8 away */
        tmp = rn2(8);         /* Pick a random direction to try first */ 
        while (distance >= mindist) {
            dx = xdir[tmp];
            dy = ydir[tmp];
            cc.x = x;
            cc.y = y;
            /* Prevent boulder from being placed on water */
            if (ttmp->ttyp == ROLLING_BOULDER_TRAP
                && is_pool_or_lava(x + distance * dx, y + distance * dy))
                success = FALSE;
            else
                success = isclearpath(&cc, distance, dx, dy);
            if (ttmp->ttyp == ROLLING_BOULDER_TRAP) {
                boolean success_otherway;

                bcc.x = x;
                bcc.y = y;
                success_otherway = isclearpath(&bcc, distance, -(dx), -(dy));
                if (!success_otherway)
                    success = FALSE;
            }
            if (success)
                break;
            if (++tmp > 7)
                tmp = 0;
            if ((++trycount % 8) == 0)
                --distance;
        }
        if (!success) {
            /* create the trap without any ammo, launch pt at trap location */
            cc.x = bcc.x = x;
            cc.y = bcc.y = y;
        } else {
            otmp = mksobj(otyp, TRUE, FALSE);
            otmp->quan = ocount;
            otmp->owt = weight(otmp);
            place_object(otmp, cc.x, cc.y);
            stackobj(otmp);
        }
        ttmp->launch.x = cc.x;
        ttmp->launch.y = cc.y;
        if (ttmp->ttyp == ROLLING_BOULDER_TRAP) {
            ttmp->launch2.x = bcc.x;
            ttmp->launch2.y = bcc.y;
        } else
            ttmp->launch_otyp = otyp;
        newsym(ttmp->launch.x, ttmp->launch.y);
        return 1;
    }

    STATIC_OVL boolean isclearpath(cc, distance, dx, dy) coord *cc;
    int distance;
    schar dx, dy;
    {
        uchar typ;
        xchar x, y;

        x = cc->x;
        y = cc->y;
        while (distance-- > 0) {
            x += dx;
            y += dy;
            typ = levl[x][y].typ;
            if (!isok(x, y) || !ZAP_POS(typ) || closed_door(x, y))
                return FALSE;
        }
        cc->x = x;
        cc->y = y;
        return TRUE;
    }

    int mintrap(mtmp) register struct monst *mtmp;
    {
        register struct trap *trap = t_at(mtmp->mx, mtmp->my);
        boolean trapkilled = FALSE;
        struct permonst *mptr = mtmp->data;
        struct obj *otmp;

        if (!trap) {
            mtmp->mtrapped = 0;      /* perhaps teleported? */
        } else if (mtmp->mtrapped) { /* is currently in the trap */
            if (!trap->tseen && cansee(mtmp->mx, mtmp->my) && canseemon(mtmp)
                && (is_pit(trap->ttyp) || trap->ttyp == BEAR_TRAP
                    || trap->ttyp == HOLE || trap->ttyp == WEB)) {
                /* If you come upon an obviously trapped monster, then
                 * you must be able to see the trap it's in too.
                 */
                seetrap(trap);
            }

            if (!rn2(40)) {
                if (sobj_at(BOULDER, mtmp->mx, mtmp->my)
                    && is_pit(trap->ttyp)) {
                    if (!rn2(2)) {
                        mtmp->mtrapped = 0;
                        if (canseemon(mtmp))
                            /*KR pline("%s pulls free...", Monnam(mtmp)); */
                            pline("%s 빠져나왔다...", Monnam(mtmp));
                        fill_pit(mtmp->mx, mtmp->my);
                    }
                } else {
                    mtmp->mtrapped = 0;
                }
            } else if (metallivorous(mptr)) {
                if (trap->ttyp == BEAR_TRAP) {
                    if (canseemon(mtmp))
                        /*KR pline("%s eats a bear trap!", Monnam(mtmp)); */
                        pline("%s 곰 덫을 먹어치웠다!", Monnam(mtmp));
                    deltrap(trap);
                    mtmp->meating = 5;
                    mtmp->mtrapped = 0;
                } else if (trap->ttyp == SPIKED_PIT) {
                    if (canseemon(mtmp))
                        /*KR pline("%s munches on some spikes!", */
                        pline("%s 철가시를 우적우적 씹어먹었다!",
                              Monnam(mtmp));
                    trap->ttyp = PIT;
                    mtmp->meating = 5;
                }
            }
        } else {
            register int tt = trap->ttyp;
            boolean in_sight, tear_web, see_it,
                inescapable = force_mintrap
                              || ((tt == HOLE || tt == PIT) && Sokoban
                                  && !trap->madeby_u);
            const char *fallverb;
            xchar tx = trap->tx, ty = trap->ty;

            /* true when called from dotrap, inescapable is not an option */
            if (mtmp == u.usteed)
                inescapable = TRUE;
            if (!inescapable
                && ((mtmp->mtrapseen & (1 << (tt - 1))) != 0
                    || (tt == HOLE && !mindless(mptr)))) {
                /* it has been in such a trap - perhaps it escapes */
                if (rn2(4))
                    return 0;
            } else {
                mtmp->mtrapseen |= (1 << (tt - 1));
            }
            /* Monster is aggravated by being trapped by you.
               Recognizing who made the trap isn't completely
               unreasonable; everybody has their own style. */
            if (trap->madeby_u && rnl(5))
                setmangry(mtmp, TRUE);

            in_sight = canseemon(mtmp);
            see_it = cansee(mtmp->mx, mtmp->my);
            /* assume hero can tell what's going on for the steed */
            if (mtmp == u.usteed)
                in_sight = TRUE;
            switch (tt) {
            case ARROW_TRAP:
                if (trap->once && trap->tseen && !rn2(15)) {
                    if (in_sight && see_it)
#if 0 /*KR: 원본*/
                    pline("%s triggers a trap but nothing happens.",
                          Monnam(mtmp));
#else /*KR: KRNethack 맞춤 번역 */
                    pline("%s 함정을 작동시켰지만 아무 일도 일어나지 않았다.",
                          Monnam(mtmp));
#endif
                    deltrap(trap);
                    newsym(mtmp->mx, mtmp->my);
                    break;
                }
                trap->once = 1;
                otmp = t_missile(ARROW, trap);
                if (in_sight)
                    seetrap(trap);
                if (thitm(8, mtmp, otmp, 0, FALSE))
                    trapkilled = TRUE;
                break;
            case DART_TRAP:
                if (trap->once && trap->tseen && !rn2(15)) {
                    if (in_sight && see_it)
               /*KR pline("%s triggers a trap but nothing happens.", */
                    pline("%s 함정을 작동시켰지만 아무 일도 일어나지 않았다.",
                          Monnam(mtmp));
                    deltrap(trap);
                    newsym(mtmp->mx, mtmp->my);
                    break;
                }
                trap->once = 1;
                otmp = t_missile(DART, trap);
                if (!rn2(6))
                    otmp->opoisoned = 1;
                if (in_sight)
                    seetrap(trap);
                if (thitm(7, mtmp, otmp, 0, FALSE))
                    trapkilled = TRUE;
                break;
            case ROCKTRAP:
                if (trap->once && trap->tseen && !rn2(15)) {
                    if (in_sight && see_it)
          /*KR pline("A trap door above %s opens, but nothing falls out!", */
                        pline("%s 머리 위 다락문이 열렸지만, 아무것도 "
                              "떨어지지 않았다!",
                              mon_nam(mtmp));
                    deltrap(trap);
                    newsym(mtmp->mx, mtmp->my);
                    break;
                }
                trap->once = 1;
                otmp = t_missile(ROCK, trap);
                if (in_sight)
                    seetrap(trap);
                if (thitm(0, mtmp, otmp, d(2, 6), FALSE))
                    trapkilled = TRUE;
                break;
            case SQKY_BOARD:
                if (is_flyer(mptr))
                    break;
                /* stepped on a squeaky board */
                if (in_sight) {
                    if (!Deaf) {
#if 0 /*KR: 원본*/
                    pline("A board beneath %s squeaks %s loudly.",
                          mon_nam(mtmp), trapnote(trap, 0));
#else /*KR: KRNethack 맞춤 번역 */
                    pline("%s 발밑에 있는 판자가 크게 %s 소리를 내며 "
                              "삐걱거렸다.",
                              mon_nam(mtmp), trapnote(trap, 0));
#endif
                    seetrap(trap);
                    } else {
                        /*KR pline("%s stops momentarily and appears to
                         * cringe.", Monnam(mtmp)); */
                        pline("%s 잠시 멈춰 서서 움찔하는 듯했다.",
                              Monnam(mtmp));
                           }
                    } else {
                    /* same near/far threshold as mzapmsg() */
                    int range = couldsee(mtmp->mx, mtmp->my) /* 9 or 5 */
                                    ? (BOLT_LIM + 1)
                                    : (BOLT_LIM - 3);

#if 0 /*KR: 원본*/
                    You_hear("a %s squeak %s.", trapnote(trap, 1),
                             (distu(mtmp->mx, mtmp->my) <= range * range)
                                 ? "nearby" : "in the distance");
#else /*KR: KRNethack 맞춤 번역 */
                    You("%s 곳에서 %s으로 삐걱거리는 소리를 들었다.",
                             (distu(mtmp->mx, mtmp->my) <= range * range)
                                 ? "가까운" : "먼", trapnote(trap, 1));
#endif
                }
                /* wake up nearby monsters */
                wake_nearto(mtmp->mx, mtmp->my, 40);
                break;
            case BEAR_TRAP:
                if (mptr->msize > MZ_SMALL && !amorphous(mptr)
                    && !is_flyer(mptr) && !is_whirly(mptr)
                    && !unsolid(mptr)) {
                    mtmp->mtrapped = 1;
                    if (in_sight) {
#if 0 /*KR: 원본*/
                    pline("%s is caught in %s bear trap!", Monnam(mtmp),
                          a_your[trap->madeby_u]);
#else /*KR: KRNethack 맞춤 번역 */
                    pline("%s %s곰 덫에 걸렸다!", Monnam(mtmp),
                          set_you[trap->madeby_u]);
#endif
                          seetrap(trap);
                    } else {
                        if (mptr == &mons[PM_OWLBEAR]
                            || mptr == &mons[PM_BUGBEAR])
                            /*KR You_hear("the roaring of an angry bear!"); */
                            You("분노의 포효를 내지르는 소리를 들었다!");
                    }
                } else if (force_mintrap) {
                    if (in_sight) {
#if 0 /*KR: 원본*/
                    pline("%s evades %s bear trap!", Monnam(mtmp),
                          a_your[trap->madeby_u]);
#else /*KR: KRNethack 맞춤 번역 */
                        pline("%s %s곰 덫을 피했다!", Monnam(mtmp),
                              set_you[trap->madeby_u]);
#endif
                        seetrap(trap);
                    }
                }
                if (mtmp->mtrapped)
                    trapkilled =
                        thitm(0, mtmp, (struct obj *) 0, d(2, 4), FALSE);
                break;
            case SLP_GAS_TRAP:
                if (!resists_sleep(mtmp) && !breathless(mptr)
                    && !mtmp->msleeping && mtmp->mcanmove) {
                    if (sleep_monst(mtmp, rnd(25), -1) && in_sight) {
                   /*KR pline("%s suddenly falls asleep!", Monnam(mtmp)); */
                        pline("%s 갑자기 잠들었다!", append_josa(Monnam(mtmp), "이"));
                        seetrap(trap);
                    }
                }
                break;
            case RUST_TRAP: {
                struct obj *target;

                if (in_sight)
                    seetrap(trap);
                switch (rn2(5)) {
                case 0:
                    if (in_sight)
#if 0 /*KR: 원본*/
                    pline("%s %s on the %s!", A_gush_of_water_hits,
                          mon_nam(mtmp), mbodypart(mtmp, HEAD));
#else /*KR: KRNethack 맞춤 번역 */
                        pline("물이 뿜어져 나와 %s의 %s에 맞았다!",
                              mon_nam(mtmp), mbodypart(mtmp, HEAD));
#endif
                    target = which_armor(mtmp, W_ARMH);
                    (void) water_damage(target, helm_simple_name(target),
                                        TRUE);
                    break;
                case 1:
                    if (in_sight)
#if 0 /*KR: 원본*/
                    pline("%s %s's left %s!", A_gush_of_water_hits,
                          mon_nam(mtmp), mbodypart(mtmp, ARM));
#else /*KR: KRNethack 맞춤 번역 */
                        pline("물이 뿜어져 나와 %s의 왼쪽 %s에 맞았다!",
                              mon_nam(mtmp), mbodypart(mtmp, ARM));
#endif
                    target = which_armor(mtmp, W_ARMS);
                    /*KR if (water_damage(target, "shield", TRUE) !=
                     * ER_NOTHING) */
                    if (water_damage(target, "방패", TRUE) != ER_NOTHING)
                        break;
                    target = MON_WEP(mtmp);
                    if (target && bimanual(target))
                        (void) water_damage(target, 0, TRUE);
                glovecheck:
                    target = which_armor(mtmp, W_ARMG);
                    /*KR (void) water_damage(target, "gauntlets", TRUE); */
                    (void) water_damage(target, "건틀릿", TRUE);
                    break;
                case 2:
                    if (in_sight)
#if 0 /*KR: 원본*/
                    pline("%s %s's right %s!", A_gush_of_water_hits,
                          mon_nam(mtmp), mbodypart(mtmp, ARM));
#else /*KR: KRNethack 맞춤 번역 */
                        pline("물이 뿜어져 나와 %s의 오른쪽 %s에 맞았다!",
                              mon_nam(mtmp), mbodypart(mtmp, ARM));
#endif
                    (void) water_damage(MON_WEP(mtmp), 0, TRUE);
                    goto glovecheck;
                default:
                    if (in_sight)
                        /*KR pline("%s %s!", A_gush_of_water_hits,
                         * mon_nam(mtmp)); */
                        pline("물이 뿜어져 나와 %s에게 맞았다!",
                              mon_nam(mtmp));
                    for (otmp = mtmp->minvent; otmp; otmp = otmp->nobj)
                        if (otmp->lamplit
                            && (otmp->owornmask & (W_WEP | W_SWAPWEP)) == 0)
                            (void) snuff_lit(otmp);
                    if ((target = which_armor(mtmp, W_ARMC)) != 0)
                        (void) water_damage(target, cloak_simple_name(target),
                                            TRUE);
                    else if ((target = which_armor(mtmp, W_ARM)) != 0)
                        (void) water_damage(target, suit_simple_name(target),
                                            TRUE);
                    else if ((target = which_armor(mtmp, W_ARMU)) != 0)
                        /*KR (void) water_damage(target, "shirt", TRUE); */
                        (void) water_damage(target, "셔츠", TRUE);
                }

                if (mptr == &mons[PM_IRON_GOLEM]) {
                    if (in_sight)
                        /*KR pline("%s falls to pieces!", Monnam(mtmp)); */
                        pline("%s 산산조각 났다!", Monnam(mtmp));
                    else if (mtmp->mtame)
                        /*KR pline("May %s rust in peace.", mon_nam(mtmp)); */
                        pline("%s여, 평안히 녹슬기를.", mon_nam(mtmp));
                    mondied(mtmp);
                    if (DEADMONSTER(mtmp))
                        trapkilled = TRUE;
                } else if (mptr == &mons[PM_GREMLIN] && rn2(3)) {
                    (void) split_mon(mtmp, (struct monst *) 0);
                }
                break;
            } /* RUST_TRAP */
            case FIRE_TRAP:
            mfiretrap:
                if (in_sight)
#if 0 /*KR: 원본*/
                pline("A %s erupts from the %s under %s!", tower_of_flame,
                      surface(mtmp->mx, mtmp->my), mon_nam(mtmp));
#else /*KR: KRNethack 맞춤 번역 */
                    pline("불기둥이 %s 아래의 %s에서 솟구쳤다!",
                          mon_nam(mtmp), surface(mtmp->mx, mtmp->my));
#endif
                else if (see_it) /* evidently `mtmp' is invisible */
#if 0 /*KR: 원본*/
                You_see("a %s erupt from the %s!", tower_of_flame,
                        surface(mtmp->mx, mtmp->my));
#else /*KR: KRNethack 맞춤 번역 */
                    You("불기둥이 %s에서 솟구치는 것을 보았다!",
                        surface(mtmp->mx, mtmp->my));
#endif

                if (resists_fire(mtmp)) {
                    if (in_sight) {
                        shieldeff(mtmp->mx, mtmp->my);
                        /*KR pline("%s is uninjured.", Monnam(mtmp)); */
                        pline("하지만 %s 다치지 않았다.", append_josa(Monnam(mtmp), "은"));
                    }
                } else {
                    int num = d(2, 4), alt;
                    boolean immolate = FALSE;

                    /* paper burns very fast, assume straw is tightly
                     * packed and burns a bit slower */
                    switch (monsndx(mptr)) {
                    case PM_PAPER_GOLEM:
                        immolate = TRUE;
                        alt = mtmp->mhpmax;
                        break;
                    case PM_STRAW_GOLEM:
                        alt = mtmp->mhpmax / 2;
                        break;
                    case PM_WOOD_GOLEM:
                        alt = mtmp->mhpmax / 4;
                        break;
                    case PM_LEATHER_GOLEM:
                        alt = mtmp->mhpmax / 8;
                        break;
                    default:
                        alt = 0;
                        break;
                    }
                    if (alt > num)
                        num = alt;

                    if (thitm(0, mtmp, (struct obj *) 0, num, immolate))
                        trapkilled = TRUE;
                    else
                        /* we know mhp is at least `num' below mhpmax,
                           so no (mhp > mhpmax) check is needed here */
                        mtmp->mhpmax -= rn2(num + 1);
                }
                if (burnarmor(mtmp) || rn2(3)) {
                    (void) destroy_mitem(mtmp, SCROLL_CLASS, AD_FIRE);
                    (void) destroy_mitem(mtmp, SPBOOK_CLASS, AD_FIRE);
                    (void) destroy_mitem(mtmp, POTION_CLASS, AD_FIRE);
                }
                if (burn_floor_objects(mtmp->mx, mtmp->my, see_it, FALSE)
                    && !see_it && distu(mtmp->mx, mtmp->my) <= 3 * 3)
                    /*KR You("smell smoke."); */
                    pline("연기 냄새가 났다.");
                if (is_ice(mtmp->mx, mtmp->my))
                    melt_ice(mtmp->mx, mtmp->my, (char *) 0);
                if (see_it && t_at(mtmp->mx, mtmp->my))
                    seetrap(trap);
                break;
            case PIT:
            case SPIKED_PIT:
                fallverb = "falls";
                if (is_flyer(mptr) || is_floater(mptr)
                    || (mtmp->wormno && count_wsegs(mtmp) > 5)
                    || is_clinger(mptr)) {
                    if (force_mintrap && !Sokoban) {
                        /* openfallingtrap; not inescapable here */
                        if (in_sight) {
                            seetrap(trap);
                       /*KR pline("%s doesn't fall into the pit.", */
                            pline("%s 구덩이에 떨어지지 않았다.",
                                  Monnam(mtmp));
                        }
                        break; /* inescapable = FALSE; */
                    }
                    if (!inescapable)
                        break;               /* avoids trap */
                    fallverb = "is dragged"; /* sokoban pit */
                }
                if (!passes_walls(mptr))
                    mtmp->mtrapped = 1;
                if (in_sight) {
#if 0 /*KR: 원본*/
                pline("%s %s into %s pit!", Monnam(mtmp), fallverb,
                      a_your[trap->madeby_u]);
#else /*KR: KRNethack 맞춤 번역 */
                    pline("%s %s구덩이로 %s!", Monnam(mtmp),
                          set_you[trap->madeby_u], fallverb);
#endif
                    if (mptr == &mons[PM_PIT_VIPER]
                        || mptr == &mons[PM_PIT_FIEND])
                        /*KR pline("How pitiful.  Isn't that the pits?"); */
                        pline("참 가련하기도 하지. 이 구덩이들 말이야.");
                    seetrap(trap);
                }
                /*KR mselftouch(mtmp, "Falling, ", FALSE); */
                mselftouch(mtmp, "추락하면서 ", FALSE);
                if (DEADMONSTER(mtmp)
                    || thitm(0, mtmp, (struct obj *) 0,
                             rnd((tt == PIT) ? 6 : 10), FALSE))
                    trapkilled = TRUE;
                break;
            case HOLE:
            case TRAPDOOR:
                if (!Can_fall_thru(&u.uz)) {
                    impossible("mintrap: %ss cannot exist on this level.",
                               defsyms[trap_to_defsym(tt)].explanation);
                    break; /* don't activate it after all */
                }
                if (is_flyer(mptr) || is_floater(mptr)
                    || mptr == &mons[PM_WUMPUS]
                    || (mtmp->wormno && count_wsegs(mtmp) > 5)
                    || mptr->msize >= MZ_HUGE) {
                    if (force_mintrap && !Sokoban) {
                        /* openfallingtrap; not inescapable here */
                        if (in_sight) {
                            seetrap(trap);
                            if (tt == TRAPDOOR)
                                /*KR pline("A trap door opens, but %s doesn't
                                 * fall through.", mon_nam(mtmp)); */
                                pline(
                                    "다락문이 열렸지만, %s 떨어지지 않았다.",
                                    mon_nam(mtmp));
                            else /* (tt == HOLE) */
                                /*KR pline("%s doesn't fall through the
                                 * hole.", Monnam(mtmp)); */
                                pline("%s 구멍에 떨어지지 않았다.",
                                      Monnam(mtmp));
                        }
                        break; /* inescapable = FALSE; */
                    }
                    if (inescapable) { /* sokoban hole */
                        if (in_sight) {
                            /*KR pline("%s seems to be yanked down!",
                             * Monnam(mtmp)); */
                            pline("%s 아래로 끌려 내려간 것 같다!",
                                  Monnam(mtmp));
                            /* suppress message in mlevel_tele_trap() */
                            in_sight = FALSE;
                            seetrap(trap);
                        }
                    } else
                        break;
                }
                /*FALLTHRU*/
            case LEVEL_TELEP:
            case MAGIC_PORTAL: {
                int mlev_res;

                mlev_res =
                    mlevel_tele_trap(mtmp, trap, inescapable, in_sight);
                if (mlev_res)
                    return mlev_res;
                break;
            }
            case TELEP_TRAP:
                mtele_trap(mtmp, trap, in_sight);
                break;
            case WEB:
                /* Monster in a web. */
                if (webmaker(mptr))
                    break;
                if (mu_maybe_destroy_web(mtmp, in_sight, trap))
                    break;
                tear_web = FALSE;
                switch (monsndx(mptr)) {
                case PM_OWLBEAR: /* Eric Backus */
                case PM_BUGBEAR:
                    if (!in_sight) {
                        /*KR You_hear("the roaring of a confused bear!"); */
                        You("공황에 빠진 곰의 포효를 들었다!");
                        mtmp->mtrapped = 1;
                        break;
                    }
                    /*FALLTHRU*/
                default:
                    if (mptr->mlet == S_GIANT
                        /* exclude baby dragons and relatively short worms */
                        || (mptr->mlet == S_DRAGON && extra_nasty(mptr))
                        || (mtmp->wormno && count_wsegs(mtmp) > 5)) {
                        tear_web = TRUE;
                    } else if (in_sight) {
#if 0 /*KR: 원본*/
                    pline("%s is caught in %s spider web.", Monnam(mtmp),
                          a_your[trap->madeby_u]);
#else /*KR: KRNethack 맞춤 번역 */
                        pline("%s %s거미줄에 걸렸다.", Monnam(mtmp),
                              web_you[trap->madeby_u]);
#endif
                        seetrap(trap);
                    }
                    mtmp->mtrapped = tear_web ? 0 : 1;
                    break;
                /* this list is fairly arbitrary; it deliberately
                   excludes wumpus & giant/ettin zombies/mummies */
                case PM_TITANOTHERE:
                case PM_BALUCHITHERIUM:
                case PM_PURPLE_WORM:
                case PM_JABBERWOCK:
                case PM_IRON_GOLEM:
                case PM_BALROG:
                case PM_KRAKEN:
                case PM_MASTODON:
                case PM_ORION:
                case PM_NORN:
                case PM_CYCLOPS:
                case PM_LORD_SURTUR:
                    tear_web = TRUE;
                    break;
                }
                if (tear_web) {
                    if (in_sight)
#if 0 /*KR: 원본*/
                    pline("%s tears through %s spider web!", Monnam(mtmp),
                          a_your[trap->madeby_u]);
#else /*KR: KRNethack 맞춤 번역 */
                    pline("%s %s거미줄을 찢고 나왔다!", Monnam(mtmp),
                          web_you[trap->madeby_u]);
#endif
                    deltrap(trap);
                    newsym(mtmp->mx, mtmp->my);
                } else if (force_mintrap && !mtmp->mtrapped) {
                    if (in_sight) {
#if 0 /*KR: 원본*/
                    pline("%s avoids %s spider web!", Monnam(mtmp),
                          a_your[trap->madeby_u]);
#else /*KR: KRNethack 맞춤 번역 */
                    pline("%s %s거미줄을 피했다!", Monnam(mtmp),
                          web_you[trap->madeby_u]);
#endif
                        seetrap(trap);
                    }
                }
                break;
            case STATUE_TRAP:
                break;
            case MAGIC_TRAP:
                /* A magic trap.  Monsters usually immune. */
                if (!rn2(21))
                    goto mfiretrap;
                break;
            case ANTI_MAGIC:
                /* similar to hero's case, more or less */
                if (!resists_magm(mtmp)) { /* lose spell energy */
                    if (!mtmp->mcan
                        && (attacktype(mptr, AT_MAGC)
                            || attacktype(mptr, AT_BREA))) {
                        mtmp->mspec_used += d(2, 2);
                        if (in_sight) {
                            seetrap(trap);
                            /*KR pline("%s seems lethargic.", Monnam(mtmp));
                             */
                            pline("%s 무기력해 보인다.", Monnam(mtmp));
                        }
                    }
                } else { /* take some damage */
                    int dmgval2 = rnd(4);

                    if ((otmp = MON_WEP(mtmp)) != 0
                        && otmp->oartifact == ART_MAGICBANE)
                        dmgval2 += rnd(4);
                    for (otmp = mtmp->minvent; otmp; otmp = otmp->nobj)
                        if (otmp->oartifact
                            && defends_when_carried(AD_MAGM, otmp))
                            break;
                    if (otmp)
                        dmgval2 += rnd(4);
                    if (passes_walls(mptr))
                        dmgval2 = (dmgval2 + 3) / 4;

                    if (in_sight)
                        seetrap(trap);
                    mtmp->mhp -= dmgval2;
                    if (DEADMONSTER(mtmp))
                        monkilled(mtmp,
                                  in_sight
                         /*KR ? "compression from an anti-magic field" */
                                      ? "반마법 역장의 압축"
                                      : (const char *) 0,
                                  -AD_MAGM);
                    if (DEADMONSTER(mtmp))
                        trapkilled = TRUE;
                    if (see_it)
                        newsym(trap->tx, trap->ty);
                }
                break;
            case LANDMINE:
                if (rn2(3))
                    break; /* monsters usually don't set it off */
                if (is_flyer(mptr)) {
                    boolean already_seen = trap->tseen;

                    if (in_sight && !already_seen) {
                        /*KR pline("A trigger appears in a pile of soil below
                         * %s.", mon_nam(mtmp)); */
                        pline("%s 아래의 흙더미에서 기폭 장치가 나타났다.",
                              mon_nam(mtmp));
                        seetrap(trap);
                    }
                    if (rn2(3))
                        break;
                    if (in_sight) {
                        newsym(mtmp->mx, mtmp->my);
                        /*KR pline_The("air currents set it off!"); */
                        pline("기류가 작동시켰다!");
                    }
                } else if (in_sight) {
                    newsym(mtmp->mx, mtmp->my);
#if 0 /*KR: 원본*/
                pline("%s%s triggers %s land mine!",
                      !Deaf ? "KAABLAMM!!!  " : "", Monnam(mtmp),
                      a_your[trap->madeby_u]);
#else /*KR: KRNethack 맞춤 번역 */
                    pline("%s%s %s지뢰의 기폭 장치를 밟았다!",
                          !Deaf ? "콰쾅!!! " : "", Monnam(mtmp),
                          set_you[trap->madeby_u]);
#endif
                }
                if (!in_sight && !Deaf)
           /*KR pline("Kaablamm!  You hear an explosion in the distance!"); */
                    pline("콰쾅! 먼 곳에서 폭발음이 들렸다!");
                blow_up_landmine(trap);
                /* explosion might have destroyed a drawbridge; don't
                   dish out more damage if monster is already dead */
                if (DEADMONSTER(mtmp)
                    || thitm(0, mtmp, (struct obj *) 0, rnd(16), FALSE)) {
                    trapkilled = TRUE;
                } else {
                    /* monsters recursively fall into new pit */
                    if (mintrap(mtmp) == 2)
                        trapkilled = TRUE;
                }
                /* a boulder may fill the new pit, crushing monster */
                fill_pit(tx,
                         ty); /* thitm may have already destroyed the trap */
                if (DEADMONSTER(mtmp))
                    trapkilled = TRUE;
                if (unconscious()) {
                    multi = -1;
                    /*KR nomovemsg = "The explosion awakens you!"; */
                    nomovemsg = "폭발 소리에 잠이 깼다!";
                }
                break;
            case POLY_TRAP:
                if (resists_magm(mtmp)) {
                    shieldeff(mtmp->mx, mtmp->my);
                } else if (!resist(mtmp, WAND_CLASS, 0, NOTELL)) {
                    if (newcham(mtmp, (struct permonst *) 0, FALSE, FALSE))
                        /* we're done with mptr but keep it up to date */
                        mptr = mtmp->data;
                    if (in_sight)
                        seetrap(trap);
                }
                break;
            case ROLLING_BOULDER_TRAP:
                if (!is_flyer(mptr)) {
                    int style = ROLL | (in_sight ? 0 : LAUNCH_UNSEEN);

                    newsym(mtmp->mx, mtmp->my);
                    if (in_sight)
#if 0 /*KR: 원본*/
                    pline("Click!  %s triggers %s.", Monnam(mtmp),
                          trap->tseen ? "a rolling boulder trap" : "something");
#else /*KR: KRNethack 맞춤 번역 */
                        pline("딸깍! %s %s 작동시켰다.", append_josa(Monnam(mtmp), "이"),
                              trap->tseen ? "구르는 바위 함정을"
                                          : "무언가를");
#endif
                    if (launch_obj(BOULDER, trap->launch.x, trap->launch.y,
                                   trap->launch2.x, trap->launch2.y, style)) {
                        if (in_sight)
                            trap->tseen = TRUE;
                        if (DEADMONSTER(mtmp))
                            trapkilled = TRUE;
                    } else {
                        deltrap(trap);
                        newsym(mtmp->mx, mtmp->my);
                    }
                }
                break;
            case VIBRATING_SQUARE:
                if (see_it && !Blind) {
                    seetrap(trap); /* before messages */
                    if (in_sight) {
                        /*KR char buf[BUFSZ], *p, *monnm = mon_nam(mtmp); */
                        pline_The(
                            "%s의 발밑 지면이 이상하게 진동하는 것을 보았다.",
                            mon_nam(mtmp));
                    } else {
                        /* notice something (hearing uses a larger threshold
                           for 'nearby') */
#if 0 /*KR: 원본*/
                        pline_The("ground vibrate %s.", 
                         (distu(mtmp->mx,mtmp->my) <= 2 * 2) ? "nearby" 
                                                             : "in the distance"); 
#else
                        pline_The("지면이 %s 곳에서 진동하는 것을 보았다.",
                                  (distu(mtmp->mx, mtmp->my) <= 2 * 2)
                                      ? "가까운" : "먼");
#endif
                    }
                }
                break;
            default:
                impossible(
                    "Some monster encountered a strange trap of type %d.",
                    tt);
            }
        }
        if (trapkilled)
            return 2;
        return mtmp->mtrapped;
    }

    /* Combine cockatrice checks into single functions to avoid repeating
     * code. */
    void instapetrify(str) const char *str;
    {
        if (Stone_resistance)
            return;
        if (poly_when_stoned(youmonst.data) && polymon(PM_STONE_GOLEM))
            return;
        /*KR You("turn to stone..."); */
        You("돌로 변했다...");
        killer.format = KILLED_BY;
        if (str != killer.name)
            Strcpy(killer.name, str ? str : "");
        done(STONING);
    }

    void minstapetrify(mon, byplayer) struct monst *mon;
    boolean byplayer;
    {
        if (resists_ston(mon))
            return;
        if (poly_when_stoned(mon->data)) {
            mon_to_stone(mon);
            return;
        }
        if (!vamp_stone(mon))
            return;

        /* give a "<mon> is slowing down" message and also remove
           intrinsic speed (comparable to similar effect on the hero) */
        mon_adjust_speed(mon, -3, (struct obj *) 0);

        if (cansee(mon->mx, mon->my))
            /*KR pline("%s turns to stone.", Monnam(mon)); */
            pline("%s 돌로 변했다.", Monnam(mon));
        if (byplayer) {
            stoned = TRUE;
            xkilled(mon, XKILL_NOMSG);
        } else
            monstone(mon);
    }

    void selftouch(arg) const char *arg;
    {
        char kbuf[BUFSZ];

        if (uwep && uwep->otyp == CORPSE
            && touch_petrifies(&mons[uwep->corpsenm]) && !Stone_resistance) {
            /*KR pline("%s touch the %s corpse.", arg,
             * mons[uwep->corpsenm].mname); */
            pline("%s %s 시체를 만졌다.", arg, mons[uwep->corpsenm].mname);
            /*KR Sprintf(kbuf, "%s corpse", an(mons[uwep->corpsenm].mname));
             */
            Sprintf(kbuf, "%s 시체", mons[uwep->corpsenm].mname);
            instapetrify(kbuf);
            /* life-saved; unwield the corpse if we can't handle it */
            if (!uarmg && !Stone_resistance)
                uwepgone();
        }
        /* Or your secondary weapon, if wielded [hypothetical; we don't
           allow two-weapon combat when either weapon is a corpse] */
        if (u.twoweap && uswapwep && uswapwep->otyp == CORPSE
            && touch_petrifies(&mons[uswapwep->corpsenm])
            && !Stone_resistance) {
            /*KR pline("%s touch the %s corpse.", arg,
             * mons[uswapwep->corpsenm].mname); */
            pline("%s %s 시체를 만졌다.", arg,
                  mons[uswapwep->corpsenm].mname);
            /*KR Sprintf(kbuf, "%s corpse",
             * an(mons[uswapwep->corpsenm].mname)); */
            Sprintf(kbuf, "%s 시체", mons[uswapwep->corpsenm].mname);
            instapetrify(kbuf);
            /* life-saved; unwield the corpse */
            if (!uarmg && !Stone_resistance)
                uswapwepgone();
        }
    }

    void mselftouch(mon, arg, byplayer) struct monst *mon;
    const char *arg;
    boolean byplayer;
    {
        struct obj *mwep = MON_WEP(mon);

        if (mwep && mwep->otyp == CORPSE
            && touch_petrifies(&mons[mwep->corpsenm]) && !resists_ston(mon)) {
            if (cansee(mon->mx, mon->my)) {
                /*KR pline("%s%s touches %s.", arg ? arg : "", arg ?
                 * mon_nam(mon) : Monnam(mon), corpse_xname(mwep, (const char
                 * *) 0, CXN_PFX_THE)); */
                pline("%s%s %s 만졌다.", arg ? arg : "",
                      arg ? mon_nam(mon) : Monnam(mon),
                      corpse_xname(mwep, (const char *) 0, CXN_PFX_THE));
            }
            minstapetrify(mon, byplayer);
            /* if life-saved, might not be able to continue wielding */
            if (!DEADMONSTER(mon) && !which_armor(mon, W_ARMG)
                && !resists_ston(mon))
                mwepgone(mon);
        }
    }

    /* start levitating */
    void float_up()
    {
        context.botl = TRUE;
        if (u.utrap) {
            if (u.utraptype == TT_PIT) {
                reset_utrap(FALSE);
                /*KR You("float up, out of the pit!"); */
                You("구덩이에서 위로 올라갔다.");
                vision_full_recalc = 1; /* vision limits change */
                fill_pit(u.ux, u.uy);
            } else if (u.utraptype == TT_LAVA          /* molten lava */
                       || u.utraptype == TT_INFLOOR) { /* solidified lava */
                /*KR Your("body pulls upward, but your %s are still stuck.",
                 * makeplural(body_part(LEG))); */
                Your("몸은 떠올랐으나 %s는 여전히 끼어 있다.",
                     makeplural(body_part(LEG)));
            } else if (u.utraptype == TT_BURIEDBALL) { /* tethered */
                coord cc;

                cc.x = u.ux, cc.y = u.uy;
                /* caveat: this finds the first buried iron ball within
                   one step of the specified location, not necessarily the
                   buried [former] uball at the original anchor point */
                (void) buried_ball(&cc);
                /* being chained to the floor blocks levitation from floating
                   above that floor but not from enhancing carrying capacity
                 */
                /*KR You("feel lighter, but your %s is still chained to the
                 * %s.", body_part(LEG), IS_ROOM(levl[cc.x][cc.y].typ) ?
                 * "floor" : "ground"); */
                You("몸이 가벼워졌으나 %s는 여전히 %s에 사슬로 묶여 있다.",
                    body_part(LEG),
                    IS_ROOM(levl[cc.x][cc.y].typ) ? "바닥" : "땅");
            } else if (u.utraptype == WEB) {
                /*KR You("float up slightly, but you are still stuck in the
                 * web."); */
                You("위로 살짝 떠올랐으나 여전히 거미줄에 걸려 있다.");
            } else { /* bear trap */
                /*KR You("float up slightly, but your %s is still stuck.",
                 * body_part(LEG)); */
                You("위로 살짝 떠올랐으나 %s는 여전히 끼어 있다.",
                    body_part(LEG));
            }
            /* when still trapped, float_vs_flight() below will block
             * levitation */
#if 0 /*KR: 원본*/
        } else if (Is_waterlevel(&u.uz)) {
        pline("It feels as though you've lost some weight.");
#else /*KR: KRNethack 맞춤 번역*/
        } else if (Is_waterlevel(&u.uz)) {
            You("체중이 약간 줄어든 것 같은 느낌이 든다.");
#endif
        } else if (u.uinwater) {
            spoteffects(TRUE);
        } else if (u.uswallow) {
#if 0 /*KR: 원본*/
        You(is_animal(u.ustuck->data) ? "float away from the %s."
                                      : "spiral up into %s.",
            is_animal(u.ustuck->data) ? surface(u.ux, u.uy)
                                      : mon_nam(u.ustuck));
#else /*KR: KRNethack 맞춤 번역 */
            You(is_animal(u.ustuck->data)
                    ? "%s에서 둥실 떠올랐다."
                    : "%s 안에서 소용돌이치며 올라갔다.",
                is_animal(u.ustuck->data) ? surface(u.ux, u.uy)
                                          : mon_nam(u.ustuck));
#endif
        } else if (Hallucination) {
            /*KR pline("Up, up, and awaaaay!  You're walking on air!"); */
            pline("날아라, 날아라, 저 멀리! 당신은 허공을 걷고 있다!");
        } else if (Is_airlevel(&u.uz)) {
            /*KR You("gain control over your movements."); */
            You("움직임을 통제할 수 있게 되었다.");
        } else {
            /*KR You("start to float in the air!"); */
            You("허공에 떠오르기 시작했다!");
        }
        if (u.usteed && !is_floater(u.usteed->data)
            && !is_flyer(u.usteed->data)) {
            if (Lev_at_will) {
                /*KR pline("%s magically floats up!", Monnam(u.usteed)); */
                pline("%s 마법의 힘으로 위로 떠올랐다!", Monnam(u.usteed));
            } else {
                /*KR You("cannot stay on %s.", mon_nam(u.usteed)); */
                You("%s 위에 계속 탈 수 없다.", mon_nam(u.usteed));
                dismount_steed(DISMOUNT_GENERIC);
            }
        }
        if (Flying)
            /*KR You("are no longer able to control your flight."); */
            You("더 이상 비행을 통제할 수 없다.");
        float_vs_flight(); /* set BFlying, also BLevitation if still trapped
                            */
        /* levitation gives maximum carrying capacity, so encumbrance
           state might be reduced */
        (void) encumber_msg();
        return;
    }

    void fill_pit(x, y) int x, y;
    {
        struct obj *otmp;
        struct trap *t;

        if ((t = t_at(x, y)) && is_pit(t->ttyp)
            && (otmp = sobj_at(BOULDER, x, y))) {
            obj_extract_self(otmp);
            /*KR (void) flooreffects(otmp, x, y, "settle"); */
            (void) flooreffects(otmp, x, y, "박혔다");
        }
    }

    /* stop levitating */
    int float_down(hmask, emask) long hmask, emask; /* might cancel timeout */
    {
        register struct trap *trap = (struct trap *) 0;
        d_level current_dungeon_level;
        boolean no_msg = FALSE;

        HLevitation &= ~hmask;
        ELevitation &= ~emask;
        if (Levitation)
            return 0; /* maybe another ring/potion/boots */
        if (BLevitation) {
            /* if blocked by terrain, we haven't actually been levitating so
               we don't give any end-of-levitation feedback or side-effects,
               but if blocking is solely due to being trapped in/on floor,
               do give some feedback but skip other float_down() effects */
            boolean trapped = (BLevitation == I_SPECIAL);

            float_vs_flight();
            if (trapped && u.utrap) /* u.utrap => paranoia */
#if 0                               /*KR: 원본*/
            You("are no longer trying to float up from the %s.",
                (u.utraptype == TT_BEARTRAP) ? "trap's jaws"
                  : (u.utraptype == TT_WEB) ? "web"
                      : (u.utraptype == TT_BURIEDBALL) ? "chain"
                          : (u.utraptype == TT_LAVA) ? "lava"
                              : "ground"); /* TT_INFLOOR */
#else                               /*KR: KRNethack 맞춤 번역 */
                You("더 이상 %s 위로 떠오르려 하지 않는다.",
                    (u.utraptype == TT_BEARTRAP)     ? "함정의 아가리"
                    : (u.utraptype == TT_WEB)        ? "거미줄"
                    : (u.utraptype == TT_BURIEDBALL) ? "사슬"
                    : (u.utraptype == TT_LAVA)       ? "용암"
                                               : "지면"); /* TT_INFLOOR */
#endif
            (void) encumber_msg(); /* carrying capacity might have changed */
            return 0;
        }
        context.botl = TRUE;
        nomul(0); /* stop running or resting */
        if (BFlying) {
            /* controlled flight no longer overridden by levitation */
            float_vs_flight(); /* clears BFlying & I_SPECIAL
                                * unless hero is stuck in floor */
            if (Flying) {
                /*KR You("have stopped levitating and are now flying."); */
                You("공중부양을 멈추고 비행하기 시작했다.");
                (void)
                    encumber_msg(); /* carrying capacity might have changed */
                return 1;
            }
        }
        if (u.uswallow) {
#if 0 /*KR: 원본*/
        You("float down, but you are still %s.",
            is_animal(u.ustuck->data) ? "swallowed" : "engulfed");
#else /*KR: KRNethack 맞춤 번역 */
            You("착지했지만, 여전히 %s 상태다.",
                is_animal(u.ustuck->data) ? "삼켜진" : "휩싸인");
#endif
        (void) encumber_msg();
        return 1;
    }

    if (Punished && !carried(uball)
        && (is_pool(uball->ox, uball->oy)
            || ((trap = t_at(uball->ox, uball->oy))
                && (is_pit(trap->ttyp) || is_hole(trap->ttyp))))) {
        u.ux0 = u.ux;
        u.uy0 = u.uy;
        u.ux = uball->ox;
        u.uy = uball->oy;
        movobj(uchain, uball->ox, uball->oy);
        newsym(u.ux0, u.uy0);
        vision_full_recalc = 1; /* in case the hero moved. */
    }
    /* check for falling into pool - added by GAN 10/20/86 */
    if (!Flying) {
        if (!u.uswallow && u.ustuck) {
            if (sticks(youmonst.data))
#if 0 /*KR: 원본*/
                You("aren't able to maintain your hold on %s.",
                    mon_nam(u.ustuck));
#else
                You("더 이상 %s 붙잡고 있을 수 없다.", 
                    append_josa(mon_nam(u.ustuck), "를"));
#endif
            else
#if 0 /*KR:T*/
                pline("Startled, %s can no longer hold you!",
                      mon_nam(u.ustuck));
#else
                pline("깜짝 놀란 %s 더 이상 당신을 붙잡지 못했다!",
                      append_josa(mon_nam(u.ustuck), "가"));
#endif
            u.ustuck = 0;
        }
        /* kludge alert:
         * drown() and lava_effects() print various messages almost
         * every time they're called which conflict with the "fall
         * into" message below.  Thus, we want to avoid printing
         * confusing, duplicate or out-of-order messages.
         * Use knowledge of the two routines as a hack -- this
         * should really be handled differently -dlc
         */
        if (is_pool(u.ux, u.uy) && !Wwalking && !Swimming && !u.uinwater)
            no_msg = drown();

        if (is_lava(u.ux, u.uy)) {
            (void) lava_effects();
            no_msg = TRUE;
        }
    }
    if (!trap) {
        trap = t_at(u.ux, u.uy);
        if (Is_airlevel(&u.uz)) {
            /*KR You("begin to tumble in place."); */
            You("제자리에서 텀블링하기 시작했다.");
        } else if (Is_waterlevel(&u.uz) && !no_msg) {
            /*KR You_feel("heavier."); */
            You("몸이 무거워진 느낌이 든다.");
        /* u.uinwater msgs already in spoteffects()/drown() */
        } else if (!u.uinwater && !no_msg) {
            if (!(emask & W_SADDLE)) {
                if (Sokoban && trap) {
                    /* Justification elsewhere for Sokoban traps is based
                     * on air currents.  This is consistent with that.
                     * The unexpected additional force of the air currents
                     * once levitation ceases knocks you off your feet.
                     */
                    if (Hallucination)
                        /*KR pline("Bummer!  You've crashed."); */
                        pline("이런! 추락해버렸다.");
                    else
                        /*KR You("fall over."); */
                        You("넘어졌다.");
                    /*KR losehp(rnd(2), "dangerous winds", KILLED_BY); */
                    losehp(rnd(2), "위험한 바람", KILLED_BY);
                    if (u.usteed)
                        dismount_steed(DISMOUNT_FELL);
                    /*KR selftouch("As you fall, you"); */
                    selftouch("추락하면서, 당신은");
                } else if (u.usteed
                           && (is_floater(u.usteed->data)
                               || is_flyer(u.usteed->data))) {
                    /*KR You("settle more firmly in the saddle."); */
                    You("안장에 더 단단히 자리를 잡았다.");
                } else if (Hallucination) {
#if 0 /*KR: 원본*/
                    pline("Bummer!  You've %s.",
                          is_pool(u.ux, u.uy)
                             ? "splashed down"
                             : "hit the ground");
#else /*KR: KRNethack 맞춤 번역 */
                    pline("이런! %s.", is_pool(u.ux, u.uy)
                                           ? "풍덩 빠져버렸다"
                                           : "땅에 곤두박질쳤다");
#endif
                } else {
               /*KR You("float gently to the %s.", surface(u.ux, u.uy)); */
                    You("%s 위로 사뿐히 내려앉았다.", surface(u.ux, u.uy));
                }
            }
        }
    }

    /* levitation gives maximum carrying capacity, so having it end
       potentially triggers greater encumbrance; do this after
       'come down' messages, before trap activation or autopickup */
    (void) encumber_msg();

    /* can't rely on u.uz0 for detecting trap door-induced level change;
       it gets changed to reflect the new level before we can check it */
    assign_level(&current_dungeon_level, &u.uz);
    if (trap) {
        switch (trap->ttyp) {
        case STATUE_TRAP:
            break;
        case HOLE:
        case TRAPDOOR:
            if (!Can_fall_thru(&u.uz) || u.ustuck)
                break;
            /*FALLTHRU*/
        default:
            if (!u.utrap) /* not already in the trap */
                dotrap(trap, 0);
        }
    }
    if (!Is_airlevel(&u.uz) && !Is_waterlevel(&u.uz) && !u.uswallow
        /* falling through trap door calls goto_level,
           and goto_level does its own pickup() call */
        && on_level(&u.uz, &current_dungeon_level))
        (void) pickup(1);
    return 1;
}

/* shared code for climbing out of a pit */
void
climb_pit()
{
    if (!u.utrap || u.utraptype != TT_PIT)
        return;

    if (Passes_walls) {
        /* marked as trapped so they can pick things up */
        /*KR You("ascend from the pit."); */
        You("구덩이에서 올라왔다.");
        reset_utrap(FALSE);
        fill_pit(u.ux, u.uy);
        vision_full_recalc = 1; /* vision limits change */
    } else if (!rn2(2) && sobj_at(BOULDER, u.ux, u.uy)) {
#if 0 /*KR: 원본*/
        Your("%s gets stuck in a crevice.", body_part(LEG));
#else /*KR: KRNethack 맞춤 번역 */
        Your("%s 틈새에 끼어버렸다.", append_josa(body_part(LEG), "이"));
#endif
        display_nhwindow(WIN_MESSAGE, FALSE);
        clear_nhwindow(WIN_MESSAGE);
#if 0 /*KR: 원본*/
        You("free your %s.", body_part(LEG));
#else /*KR: KRNethack 맞춤 번역 */
        You("%s 빼냈다.", append_josa(body_part(LEG), "을"));
#endif
    } else if ((Flying || is_clinger(youmonst.data)) && !Sokoban) {
        /* eg fell in pit, then poly'd to a flying monster;           or used
         * '>' to deliberately enter it */
#if 0 /*KR: 원본*/
        You("%s from the pit.", Flying ? "fly" : "climb");
#else /*KR: KRNethack 맞춤 번역 */
        You("구덩이에서 %s 빠져나왔다.", Flying ? "날아서" : "기어서");
#endif
        reset_utrap(FALSE);
        fill_pit(u.ux, u.uy);
        vision_full_recalc = 1; /* vision limits change */
    } else if (!(--u.utrap)) {
        reset_utrap(FALSE);
#if 0 /*KR: 원본*/
        You("%s to the edge of the pit.",
            (Sokoban && Levitation)
                ? "struggle against the air currents and float"
                : u.usteed ? "ride" : "crawl");
#else /*KR: KRNethack 맞춤 번역 */
        You("%s 구덩이의 가장자리에 도달했다.",
            (Sokoban && Levitation) ? "기류에 맞서 발버둥치며 떠올라"
            : u.usteed              ? "타고"
                                    : "기어서");
#endif
        fill_pit(u.ux, u.uy);
        vision_full_recalc = 1; /* vision limits change */
    } else if (u.dz || flags.verbose) {
        if (u.usteed)
#if 0 /*KR: 원본*/
            Norep("%s is still in a pit.", upstart(y_monnam(u.usteed)));
#else /*KR: KRNethack 맞춤 번역 */
            Norep("%s 여전히 구덩이에 있다.",
                  append_josa(upstart(y_monnam(u.usteed)), "은"));
#endif
        else
#if 0 /*KR: 원본*/
            Norep((Hallucination && !rn2(5))
                      ? "You've fallen, and you can't get up."
                      : "You are still in a pit.");
#else /*KR: KRNethack 맞춤 번역 */
            Norep((Hallucination && !rn2(5))
                      ? "당신은 넘어졌고, 일어날 수 없다."
                      : "당신은 여전히 구덩이에 있다.");
#endif
    }
}
STATIC_OVL void dofiretrap(box) struct obj *box; /* null for floor trap */
{
    boolean see_it = !Blind;
    int num, alt;

    /* Bug: for box case, the equivalent of burn_floor_objects() ought     *
     * to be done upon its contents.     */

    if ((box && !carried(box)) ? is_pool(box->ox, box->oy) : Underwater) {
#if 0 /*KR: 원본*/
        pline("A cascade of steamy bubbles erupts from %s!",
              the(box ? xname(box) : surface(u.ux, u.uy)));
#else /*KR: KRNethack 맞춤 번역 */
        pline("%s에서 김이 나는 거품이 폭포수처럼 뿜어져 나왔다!",
              the(box ? xname(box) : surface(u.ux, u.uy)));
#endif
        if (Fire_resistance)
            /*KR You("are uninjured."); */
            You("다치지 않았다.");
        else
            /*KR losehp(rnd(3), "boiling water", KILLED_BY); */
            losehp(rnd(3), "끓는 물", KILLED_BY);
        return;
    }
#if 0 /*KR: 원본*/
    pline("A %s %s from %s!", tower_of_flame, box ? "bursts" : "erupts",
          the(box ? xname(box) : surface(u.ux, u.uy)));
#else /*KR: KRNethack 맞춤 번역 */
    pline("%s에서 %s이 %s!", the(box ? xname(box) : surface(u.ux, u.uy)),
          tower_of_flame, box ? "터져나왔다" : "솟구쳐 올랐다");
#endif
    if (Fire_resistance) {
        shieldeff(u.ux, u.uy);
        num = rn2(2);
    } else if (Upolyd) {
        num = d(2, 4);
        switch (u.umonnum) {
        case PM_PAPER_GOLEM:
            alt = u.mhmax;
            break;
        case PM_STRAW_GOLEM:
            alt = u.mhmax / 2;
            break;
        case PM_WOOD_GOLEM:
            alt = u.mhmax / 4;
            break;
        case PM_LEATHER_GOLEM:
            alt = u.mhmax / 8;
            break;
        default:
            alt = 0;
            break;
        }
        if (alt > num)
            num = alt;
        if (u.mhmax > mons[u.umonnum].mlevel)
            u.mhmax -= rn2(min(u.mhmax, num + 1)), context.botl = 1;
    } else {
        num = d(2, 4);
        if (u.uhpmax > u.ulevel)
            u.uhpmax -= rn2(min(u.uhpmax, num + 1)), context.botl = 1;
    }
    if (!num)
        /*KR You("are uninjured."); */
        You("다치지 않았다.");
    else
        losehp(num, tower_of_flame, KILLED_BY_AN); /* fire damage */
    burn_away_slime();

    if (burnarmor(&youmonst) || rn2(3)) {
        destroy_item(SCROLL_CLASS, AD_FIRE);
        destroy_item(SPBOOK_CLASS, AD_FIRE);
        destroy_item(POTION_CLASS, AD_FIRE);
    }
    if (!box && burn_floor_objects(u.ux, u.uy, see_it, TRUE) && !see_it)
        /*KR You("smell paper burning."); */
        You("종이 타는 냄새가 났다.");
    if (is_ice(u.ux, u.uy))
        melt_ice(u.ux, u.uy, (char *) 0);
}

STATIC_OVL void
domagictrap()
{
    register int fate = rnd(20);

    /* What happened to the poor sucker? */

    if (fate < 10) {
        /* Most of the time, it creates some monsters. */
        int cnt = rnd(4);

        /* blindness effects */
        if (!resists_blnd(&youmonst)) {
            /*KR You("are momentarily blinded by a flash of light!"); */
            You("번쩍이는 빛에 잠시 눈이 멀었다!");
            make_blinded((long) rn1(5, 10), FALSE);
            if (!Blind)
                Your1(vision_clears);
        } else if (!Blind) {
            /*KR You_see("a flash of light!"); */
            You("빛이 번쩍이는 것을 보았다!");
        }

        /* deafness effects */
        if (!Deaf) {
            /*KR You_hear("a deafening roar!"); */
            You("귀청이 터질 듯한 굉음이 들렸다!");
            incr_itimeout(&HDeaf, rn1(20, 30));
            context.botl = TRUE;
        } else {
            /* magic vibrations still hit you */
            /*KR You_feel("rankled."); */
            You("심기가 불편해짐을 느꼈다.");
            incr_itimeout(&HDeaf, rn1(5, 15));
            context.botl = TRUE;
        }
        while (cnt--)
            (void) makemon((struct permonst *) 0, u.ux, u.uy, NO_MM_FLAGS);
        /* roar: wake monsters in vicinity, after placing trap-created ones */
        wake_nearto(u.ux, u.uy, 7 * 7);
        /* [flash: should probably also hit nearby gremlins with light] */
    } else
        switch (fate) {
        case 10:
        case 11:
            /* sometimes nothing happens */
            break;
        case 12: /* a flash of fire */
            dofiretrap((struct obj *) 0);
            break;

        /* odd feelings */
        case 13:
            /*KR pline("A shiver runs up and down your %s!",
             * body_part(SPINE)); */
            pline("당신의 %s을 타고 소름이 쫙 돋았다!", body_part(SPINE));
            break;
        case 14:
#if 0 /*KR: 원본*/
            You_hear(Hallucination ? "the moon howling at you."
                                   : "distant howling.");
#else /*KR: KRNethack 맞춤 번역 */
            You(Hallucination ? "달이 당신에게 울부짖는 소리를 들었다."
                                   : "멀리서 울부짖는 소리를 들었다.");
#endif
            break;
        case 15:
            if (on_level(&u.uz, &qstart_level))
#if 0 /*KR: 원본*/
                You_feel(
                    "%slike the prodigal son.",
                    (flags.female || (Upolyd && is_neuter(youmonst.data)))
                        ? "oddly "
                        : "");
#else /*KR: KRNethack 맞춤 번역 */
                You("%s탕자가 된 듯한 기분이 들었다.",
                    (flags.female || (Upolyd && is_neuter(youmonst.data)))
                        ? "묘하게도 "
                        : "");
#endif
            else
#if 0 /*KR: 원본*/
                You("suddenly yearn for %s.",
                    Hallucination
                        ? "Cleveland"
                        : (In_quest(&u.uz) || at_dgn_entrance("The Quest"))
                              ? "your nearby homeland"
                              : "your distant homeland");
#else /*KR: KRNethack 맞춤 번역 */
                You("갑자기 %s 몹시 그리워졌다.",
                    Hallucination ? "클리블랜드가"
                    : (In_quest(&u.uz) || at_dgn_entrance("The Quest"))
                        ? "가까운 고향이"
                        : "머나먼 고향이");
#endif
            break;
        case 16:
            /*KR Your("pack shakes violently!"); */
            Your("배낭이 격렬하게 흔들렸다!");
            break;
        case 17:
            /*KR You(Hallucination ? "smell hamburgers." : "smell charred
             * flesh."); */
            You(Hallucination ? "햄버거 냄새를 맡았다."
                              : "새카맣게 탄 고기 냄새를 맡았다.");
            break;
        case 18:
            /*KR You_feel("tired."); */
            You("피곤해짐을 느꼈다.");
            break;

        /* very occasionally something nice happens. */
        case 19: { /* tame nearby monsters */
            int i, j;
            struct monst *mtmp;

            (void) adjattrib(A_CHA, 1, FALSE);
            for (i = -1; i <= 1; i++)
                for (j = -1; j <= 1; j++) {
                    if (!isok(u.ux + i, u.uy + j))
                        continue;
                    mtmp = m_at(u.ux + i, u.uy + j);
                    if (mtmp)
                        (void) tamedog(mtmp, (struct obj *) 0);
                }
            break;
        }
        case 20: { /* uncurse stuff */
            struct obj pseudo;
            long save_conf = HConfusion;

            pseudo = zeroobj; /* neither cursed nor blessed,
                                 and zero out oextra */
            pseudo.otyp = SCR_REMOVE_CURSE;
            HConfusion = 0L;
            (void) seffects(&pseudo);
            HConfusion = save_conf;
            break;
        }
        default:
            break;
        }
}

/* Set an item on fire.
 *   "force" means not to roll a luck-based protection check for the
 *     item.
 *   "x" and "y" are the coordinates to dump the contents of a
 *     container, if it burns up.
 *
 * Return whether the object was destroyed.
 */
boolean
fire_damage(obj, force, x, y)
struct obj *obj;
boolean force;
xchar x, y;
{
    int chance;
    struct obj *otmp, *ncobj;
    int in_sight = !Blind && couldsee(x, y); /* Don't care if it's lit */
    int dindx;

    /* object might light in a controlled manner */
    if (catch_lit(obj))
        return FALSE;

    if (Is_container(obj)) {
        switch (obj->otyp) {
        case ICE_BOX:
            return FALSE; /* Immune */
        case CHEST:
            chance = 40;
            break;
        case LARGE_BOX:
            chance = 30;
            break;
        default:
            chance = 20;
            break;
        }
        if ((!force && (Luck + 5) > rn2(chance))
            || (is_flammable(obj) && obj->oerodeproof))
            return FALSE;
        /* Container is burnt up - dump contents out */
        if (in_sight)
            /*KR pline("%s catches fire and burns.", Yname2(obj)); */
            pline("%s 불이 붙어 타버렸다.", Yname2(obj));
        if (Has_contents(obj)) {
            if (in_sight)
                /*KR pline("Its contents fall out."); */
                pline("그 안의 내용물이 쏟아졌다.");
            for (otmp = obj->cobj; otmp; otmp = ncobj) {
                ncobj = otmp->nobj;
                obj_extract_self(otmp);
                if (!flooreffects(otmp, x, y, ""))
                    place_object(otmp, x, y);
            }
        }
        setnotworn(obj);
        delobj(obj);
        return TRUE;
    } else if (!force && (Luck + 5) > rn2(20)) {
        /*  chance per item of sustaining damage:
          *     max luck (Luck==13):    10%
          *     avg luck (Luck==0):     75%
          *     awful luck (Luck<-4):  100%
          */
        return FALSE;
    } else if (obj->oclass == SCROLL_CLASS || obj->oclass == SPBOOK_CLASS) {
        if (obj->otyp == SCR_FIRE || obj->otyp == SPE_FIREBALL)
            return FALSE;
        if (obj->otyp == SPE_BOOK_OF_THE_DEAD) {
            if (in_sight)
                /*KR pline("Smoke rises from %s.", the(xname(obj))); */
                pline("%s에서 연기가 피어오른다.", the(xname(obj)));
            return FALSE;
        }
        dindx = (obj->oclass == SCROLL_CLASS) ? 3 : 4;
        if (in_sight)
            pline("%s %s.", Yname2(obj),
                  destroy_strings[dindx][(obj->quan > 1L)]);
        setnotworn(obj);
        delobj(obj);
        return TRUE;
    } else if (obj->oclass == POTION_CLASS) {
        dindx = (obj->otyp != POT_OIL) ? 1 : 2;
        if (in_sight)
            pline("%s %s.", Yname2(obj),
                  destroy_strings[dindx][(obj->quan > 1L)]);
        setnotworn(obj);
        delobj(obj);
        return TRUE;
    } else if (erode_obj(obj, (char *) 0, ERODE_BURN, EF_DESTROY)
               == ER_DESTROYED) {
        return TRUE;
    }
    return FALSE;
}

/*
 * Apply fire_damage() to an entire chain.
 *
 * Return number of objects destroyed. --ALI
 */
int
fire_damage_chain(chain, force, here, x, y)
struct obj *chain;
boolean force, here;
xchar x, y;
{
    struct obj *obj, *nobj;
    int num = 0;

    for (obj = chain; obj; obj = nobj) {
        nobj = here ? obj->nexthere : obj->nobj;
        if (fire_damage(obj, force, x, y))
            ++num;
    }

    if (num && (Blind && !couldsee(x, y)))
        /*KR You("smell smoke."); */
        You("연기 냄새가 난다.");
    return num;
}

/* obj has been thrown or dropped into lava; damage is worse than mere fire */
boolean
lava_damage(obj, x, y)
struct obj *obj;
xchar x, y;
{
    int otyp = obj->otyp, ocls = obj->oclass;

    /* the Amulet, invocation items, and Rider corpses are never destroyed
       (let Book of the Dead fall through to fire_damage() to get feedback) */
    if (obj_resists(obj, 0, 0) && otyp != SPE_BOOK_OF_THE_DEAD)
        return FALSE;
    /* destroy liquid (venom), wax, veggy, flesh, paper (except for scrolls
       and books--let fire damage deal with them), cloth, leather, wood, bone
       unless it's inherently or explicitly fireproof or contains something;
       note: potions are glass so fall through to fire_damage() and boil */
    if (objects[otyp].oc_material < DRAGON_HIDE
        && ocls != SCROLL_CLASS && ocls != SPBOOK_CLASS
        && objects[otyp].oc_oprop != FIRE_RES
        && otyp != WAN_FIRE && otyp != FIRE_HORN
        /* assumes oerodeproof isn't overloaded for some other purpose on
           non-eroding items */
        && !obj->oerodeproof
        /* fire_damage() knows how to deal with containers and contents */
        && !Has_contents(obj)) {
        if (cansee(x, y)) {
            /* this feedback is pretty clunky and can become very verbose
               when former contents of a burned container get here via
               flooreffects() */
            if (obj == thrownobj || obj == kickedobj)
#if 0 /*KR: 원본*/
                pline("%s %s up!", is_plural(obj) ? "They" : "It",
                      otense(obj, "burn"));
#else /*KR: KRNethack 맞춤 번역 */
                pline("%s 완전히 타버렸다!",
                      is_plural(obj) ? "그것들은" : "그것은");
#endif
            else
#if 0 /*KR: 원본*/
                You_see("%s hit lava and burn up!", doname(obj));
#else /*KR: KRNethack 맞춤 번역 */
                You("%s 용암에 떨어져 완전히 타버린 것을 보았다!",
                        append_josa(doname(obj), "이"));
#endif
        }
        if (carried(obj)) { /* shouldn't happen */
            remove_worn_item(obj, TRUE);
            useupall(obj);
        } else
            delobj(obj);
        return TRUE;
    }
    return fire_damage(obj, TRUE, x, y);
}

void acid_damage(obj) struct obj *obj;
{
    /* Scrolls but not spellbooks can be erased by acid. */
    struct monst *victim;
    boolean vismon;

    if (!obj)
        return;

    victim = carried(obj) ? &youmonst : mcarried(obj) ? obj->ocarry : NULL;
    vismon = victim && (victim != &youmonst) && canseemon(victim);

    if (obj->greased) {
        grease_protect(obj, (char *) 0, victim);
    } else if (obj->oclass == SCROLL_CLASS && obj->otyp != SCR_BLANK_PAPER) {
        if (obj->otyp != SCR_BLANK_PAPER
#ifdef MAIL
            && obj->otyp != SCR_MAIL
#endif
        ) {
            if (!Blind) {
                if (victim == &youmonst)
#if 0 /*KR: 원본*/
                    pline("Your %s.", aobjnam(obj, "fade"));
#else /*KR: KRNethack 맞춤 번역 */
                    pline("당신의 %s 글자가 바랬다.", xname(obj));
#endif
                else if (vismon)
#if 0 /*KR: 원본*/
                    pline("%s %s.", s_suffix(Monnam(victim)),
                          aobjnam(obj, "fade"));
#else /*KR: KRNethack 맞춤 번역 */
                    pline("%s의 %s 글자가 바랬다.", Monnam(victim),
                          xname(obj));
#endif
            }
        }
        obj->otyp = SCR_BLANK_PAPER;
        obj->spe = 0;
        obj->dknown = 0;
    } else
        erode_obj(obj, (char *) 0, ERODE_CORRODE, EF_GREASE | EF_VERBOSE);
}

/* context for water_damage(), managed by water_damage_chain();
   when more than one stack of potions of acid explode while processing
   a chain of objects, use alternate phrasing after the first message */
static struct h2o_ctx {
    int dkn_boom, unk_boom; /* track dknown, !dknown separately */
    boolean ctx_valid;
} acid_ctx = { 0, 0, FALSE };

/* Get an object wet and damage it appropriately.
 * "ostr", if present, is used instead of the object name in some
 * messages.
 * "force" means not to roll luck to protect some objects.
 * Returns an erosion return value (ER_*)
 */
int
water_damage(obj, ostr, force)
struct obj *obj;
const char *ostr;
boolean force;
{
    if (!obj)
        return ER_NOTHING;

    if (snuff_lit(obj))
        return ER_DAMAGED;

    if (!ostr)
        ostr = cxname(obj);

    if (obj->otyp == CAN_OF_GREASE && obj->spe > 0) {
        return ER_NOTHING;
    } else if (obj->otyp == TOWEL && obj->spe < 7) {
        wet_a_towel(obj, rnd(7), TRUE);
        return ER_NOTHING;
    } else if (obj->greased) {
        if (!rn2(2))
            obj->greased = 0;
        if (carried(obj))
            update_inventory();
        return ER_GREASED;
    } else if (Is_container(obj) && !Is_box(obj)
               && (obj->otyp != OILSKIN_SACK || (obj->cursed && !rn2(3)))) {
        if (carried(obj))
            /*KR pline("Water gets into your %s!", ostr); */
            pline("당신의 %s 안으로 물이 들어갔다!", ostr);

        water_damage_chain(obj->cobj, FALSE);
        return ER_DAMAGED; /* contents were damaged */
    } else if (obj->otyp == OILSKIN_SACK) {
        if (carried(obj))
            /*KR pline("Some water slides right off your %s.", ostr); */
            pline("물이 당신의 %s에서 미끄러져 떨어졌다.", ostr);
        makeknown(OILSKIN_SACK);
        /* not actually damaged, but because we /didn't/ get the "water
           gets into!" message, the player now has more information and
           thus we need to waste any potion they may have used (also,
           flavourwise the water is now on the floor) */
        return ER_DAMAGED;
    } else if (!force && (Luck + 5) > rn2(20)) {
        /*  chance per item of sustaining damage:
            *   max luck:               10%
            *   avg luck (Luck==0):     75%
            *   awful luck (Luck<-4):  100%
            */
        return ER_NOTHING;
    } else if (obj->oclass == SCROLL_CLASS) {
        if (obj->otyp == SCR_BLANK_PAPER
#ifdef MAIL
            || obj->otyp == SCR_MAIL
#endif
           ) return 0;
        if (carried(obj))
#if 0 /*KR: 원본*/
            pline("Your %s %s.", ostr, vtense(ostr, "fade"));
#else /*KR: KRNethack 맞춤 번역 */
            pline("당신의 %s 글자가 바랬다.", append_josa(ostr, "의"));
#endif

        obj->otyp = SCR_BLANK_PAPER;
        obj->dknown = 0;
        obj->spe = 0;
        if (carried(obj))
            update_inventory();
        return ER_DAMAGED;
    } else if (obj->oclass == SPBOOK_CLASS) {
        if (obj->otyp == SPE_BOOK_OF_THE_DEAD) {
#if 0 /*KR: 원본*/
            pline("Steam rises from %s.", the(xname(obj)));
#else /*KR: KRNethack 맞춤 번역 */
            pline("%s에서 증기가 피어올랐다.", the(xname(obj)));
#endif
            return 0;
        } else if (obj->otyp == SPE_BLANK_PAPER) {
            return 0;
        }
        if (carried(obj))
#if 0 /*KR: 원본*/
            pline("Your %s %s.", ostr, vtense(ostr, "fade"));
#else /*KR: KRNethack 맞춤 번역 */
            pline("당신의 %s 글자가 바랬다.", append_josa(ostr, "의"));
#endif

        if (obj->otyp == SPE_NOVEL) {
            obj->novelidx = 0;
            free_oname(obj);
        }

        obj->otyp = SPE_BLANK_PAPER;
        obj->dknown = 0;
        if (carried(obj))
            update_inventory();
        return ER_DAMAGED;
    } else if (obj->oclass == POTION_CLASS) {
        if (obj->otyp == POT_ACID) {
            char *bufp;
            boolean one = (obj->quan == 1L), update = carried(obj),
                    exploded = FALSE;

            if (Blind && !carried(obj))
                obj->dknown = 0;
            if (acid_ctx.ctx_valid)
                exploded =
                    ((obj->dknown ? acid_ctx.dkn_boom : acid_ctx.unk_boom)
                     > 0);
            /* First message is             * "a [potion|<color> potion|potion
             * of acid] explodes"             * depending on obj->dknown
             * (potion has been seen) and             *
             * objects[POT_ACID].oc_name_known (fully discovered), * or "some
             * {plural version} explode" when relevant.             * Second
             * and subsequent messages for same chain and             *
             * matching dknown status are             * "another
             * [potion|<color> &c] explodes" or plural             * variant.
             */
            bufp = simpleonames(obj);
#if 0                               /*KR: 원본*/
            pline("%s %s %s!", /* "A potion explodes!" */
                  !exploded ? (one ? "A" : "Some")
                            : (one ? "Another" : "More"),
                  bufp, vtense(bufp, "explode"));
#else                               /*KR: KRNethack 맞춤 번역 */
            pline("%s%s 폭발했다!", /* "A potion explodes!" */
                  !exploded ? "" : (one ? "또 다른 " : "더 많은 "),
                  append_josa(bufp, "이"));
#endif
            if (acid_ctx.ctx_valid) {
                if (obj->dknown)
                    acid_ctx.dkn_boom++;
                else
                    acid_ctx.unk_boom++;
            }
            setnotworn(obj);
            delobj(obj);
            if (update)
                update_inventory();
            return ER_DESTROYED;
        } else if (obj->odiluted) {
            if (carried(obj))
#if 0 /*KR: 원본*/
                pline("Your %s %s further.", ostr, vtense(ostr, "dilute"));
#else /*KR: KRNethack 맞춤 번역 */
                pline("당신의 %s 더욱 묽어졌다.", append_josa(ostr, "은"));
#endif

            obj->otyp = POT_WATER;
            obj->dknown = 0;
            obj->blessed = obj->cursed = 0;
            obj->odiluted = 0;
            if (carried(obj))
                update_inventory();
            return ER_DAMAGED;
        } else if (obj->otyp != POT_WATER) {
            if (carried(obj))
#if 0 /*KR: 원본*/
                pline("Your %s %s.", ostr, vtense(ostr, "dilute"));
#else /*KR: KRNethack 맞춤 번역 */
                pline("당신의 %s 묽어졌다.", append_josa(ostr, "은"));
#endif

            obj->odiluted++;
            if (carried(obj))
                update_inventory();
            return ER_DAMAGED;
        }
    } else {
        return erode_obj(obj, ostr, ERODE_RUST, EF_NONE);
    }
    return ER_NOTHING;
}

void
water_damage_chain(obj, here)
struct obj *obj;
boolean here;
{
    struct obj *otmp;

    /* initialize acid context: so far, neither seen (dknown) potions of
       acid nor unseen have exploded during this water damage sequence */
    acid_ctx.dkn_boom = acid_ctx.unk_boom = 0;
    acid_ctx.ctx_valid = TRUE;

    for (; obj; obj = otmp) {
        otmp = here ? obj->nexthere : obj->nobj;
        water_damage(obj, (char *) 0, FALSE);
    }

    /* reset acid context */
    acid_ctx.dkn_boom = acid_ctx.unk_boom = 0;
    acid_ctx.ctx_valid = FALSE;
}

/*
 * This function is potentially expensive - rolling
 * inventory list multiple times.  Luckily it's seldom needed.
 * Returns TRUE if disrobing made player unencumbered enough to
 * crawl out of the current predicament.
 */
STATIC_OVL boolean
emergency_disrobe(lostsome)
boolean *lostsome;
{
    int invc = inv_cnt(TRUE);

    while (near_capacity() > (Punished ? UNENCUMBERED : SLT_ENCUMBER)) {
        register struct obj *obj, *otmp = (struct obj *) 0;
        register int i;

        /* Pick a random object */
        if (invc > 0) {
            i = rn2(invc);
            for (obj = invent; obj; obj = obj->nobj) {
                /*
                 * Undroppables are: body armor, boots, gloves,
                 * amulets, and rings because of the time and effort
                 * in removing them + loadstone and other cursed stuff
                 * for obvious reasons.
                 */
                if (!((obj->otyp == LOADSTONE && obj->cursed) || obj == uamul
                      || obj == uleft || obj == uright || obj == ublindf
                      || obj == uarm || obj == uarmc || obj == uarmg
                      || obj == uarmf || obj == uarmu
                      || (obj->cursed && (obj == uarmh || obj == uarms))
                      || welded(obj)))
                    otmp = obj;
                /* reached the mark and found some stuff to drop? */
                if (--i < 0 && otmp)
                    break;

                /* else continue */
            }
        }
        if (!otmp)
            return FALSE; /* nothing to drop! */
        if (otmp->owornmask)
            remove_worn_item(otmp, FALSE);
        *lostsome = TRUE;
        dropx(otmp);
        invc--;
    }
    return TRUE;
}


/*  return TRUE iff player relocated */
boolean
drown()
{
    const char *pool_of_water;
    boolean inpool_ok = FALSE, crawl_ok;
    int i, x, y;

    feel_newsym(u.ux, u.uy); /* in case Blind, map the water here */
    /* happily wading in the same contiguous pool */
    if (u.uinwater && is_pool(u.ux - u.dx, u.uy - u.dy)
        && (Swimming || Amphibious)) {
        /* water effects on objects every now and then */
        if (!rn2(5))
            inpool_ok = TRUE;
        else
            return FALSE;
    }

if (!u.uinwater) {
#if 0 /*KR: 원본*/
        You("%s into the %s%c", Is_waterlevel(&u.uz) ? "plunge" : "fall",
            hliquid("water"),
            Amphibious || Swimming ? '.' : '!');
#else /*KR: KRNethack 맞춤 번역 */
        You("%s 안으로 %s%c", hliquid("물"),
            Is_waterlevel(&u.uz) ? "뛰어들었다" : "떨어졌다",
            Amphibious || Swimming ? '.' : '!');
#endif
        if (!Swimming && !Is_waterlevel(&u.uz))
#if 0 /*KR: 원본*/
            You("sink like %s.", Hallucination ? "the Titanic" : "a rock");
#else /*KR: KRNethack 맞춤 번역 */
            You("%s 가라앉았다.",
                Hallucination ? "타이타닉호처럼" : "돌멩이처럼");
#endif
    }

    water_damage_chain(invent, FALSE);

    if (u.umonnum == PM_GREMLIN && rn2(3))
        (void) split_mon(&youmonst, (struct monst *) 0);
    else if (u.umonnum == PM_IRON_GOLEM) {
        /*KR You("rust!"); */
        You("녹슬었다!");
        i = Maybe_Half_Phys(d(2, 6));
        if (u.mhmax > i)
            u.mhmax -= i;
        /*KR losehp(i, "rusting away", KILLED_BY); */
        losehp(i, "완전히 녹슬어서", KILLED_BY);
    }
    if (inpool_ok)
        return FALSE;

    if ((i = number_leashed()) > 0) {
#if 0 /*KR: 원본*/
        pline_The("leash%s slip%s loose.", (i > 1) ? "es" : "",
                  (i > 1) ? "" : "s");
#else /*KR: KRNethack 맞춤 번역 */
        pline("가죽끈이 느슨하게 풀렸다.");
#endif
        unleash_all();
    }

    if (Amphibious || Swimming) {
        if (Amphibious) {
            if (flags.verbose)
                /*KR pline("But you aren't drowning."); */
                pline("하지만 당신은 익사하지 않았다.");
            if (!Is_waterlevel(&u.uz)) {
                if (Hallucination)
                    /*KR Your("keel hits the bottom."); */
                    Your("용골이 바닥에 닿았다.");
                else
                    /*KR You("touch bottom."); */
                    You("바닥에 발이 닿았다.");
            }
        }
        if (Punished) {
            unplacebc();
            placebc();
        }
        vision_recalc(2); /* unsee old position */
        u.uinwater = 1;
        under_water(1);
        vision_full_recalc = 1;
        return FALSE;
    }
    if ((Teleportation || can_teleport(youmonst.data)) && !Unaware
        && (Teleport_control || rn2(3) < Luck + 2)) {
        /*KR You("attempt a teleport spell."); */ /* utcsri!carroll */
        You("순간이동 주문을 시도했다.");
        if (!level.flags.noteleport) {
            (void) dotele(FALSE);
            if (!is_pool(u.ux, u.uy))
                return TRUE;
        } else
            /*KR pline_The("attempted teleport spell fails."); */
            pline("시도했던 순간이동 주문이 실패했다.");
    }
    if (u.usteed) {
        dismount_steed(DISMOUNT_GENERIC);
        if (!is_pool(u.ux, u.uy))
            return TRUE;
    }
    crawl_ok = FALSE;
    x = y = 0; /* lint suppression */
    /* if sleeping, wake up now so that we don't crawl out of water while
     * still asleep; we can't do that the same way that waking       due to
     * combat is handled; note unmul() clears u.usleep */
    if (u.usleep)
        /*KR unmul("Suddenly you wake up!"); */
        unmul("갑자기 당신은 잠에서 깼다!");
    /* being doused will revive from fainting */
    if (is_fainted())
        reset_faint();
    /* can't crawl if unable to move (crawl_ok flag stays false) */
    if (multi < 0 || (Upolyd && !youmonst.data->mmove))
        goto crawl;
    /* look around for a place to crawl to */
    for (i = 0; i < 100; i++) {
        x = rn1(3, u.ux - 1);
        y = rn1(3, u.uy - 1);
        if (crawl_destination(x, y)) {
            crawl_ok = TRUE;
            goto crawl;
        }
    }
    /* one more scan */
    for (x = u.ux - 1; x <= u.ux + 1; x++)
        for (y = u.uy - 1; y <= u.uy + 1; y++)
            if (crawl_destination(x, y)) {
                crawl_ok = TRUE;
                goto crawl;
            }
crawl:
    if (crawl_ok) {
        boolean lost = FALSE;
        /* time to do some strip-tease... */
        boolean succ = Is_waterlevel(&u.uz) ? TRUE : emergency_disrobe(&lost);
        /*KR You("try to crawl out of the %s.", hliquid("water")); */
        You("%s에서 기어 나오려고 시도했다.", hliquid("물"));

        if (lost)
            /*KR You("dump some of your gear to lose weight..."); */
            You("무게를 줄이기 위해 짐의 일부를 버렸다...");

        if (succ) {
            /*KR pline("Pheew!  That was close."); */
            pline("휴우! 위험했다.");
            teleds(x, y, TRUE);
            return TRUE;
        }
        /* still too much weight */
        /*KR pline("But in vain."); */
        pline("그러나 헛수고였다.");
    }
    u.uinwater = 1;
    /*KR You("drown."); */
    You("익사했다.");
    for (i = 0; i < 5; i++) { /* arbitrary number of loops */
        /* killer format and name are reconstructed every iteration
           because lifesaving resets them */
        pool_of_water = waterbody_name(u.ux, u.uy);
        killer.format = KILLED_BY_AN;
        /* avoid "drowned in [a] water" */
        if (!strcmp(pool_of_water, "water"))
            pool_of_water = "deep water", killer.format = KILLED_BY;
        Strcpy(killer.name, pool_of_water);
        done(DROWNING);
        /* oops, we're still alive.  better get out of the water. */
        if (safe_teleds(TRUE))
            break; /* successful life-save */
                   /* nowhere safe to land; repeat drowning loop... */
        /*KR pline("You're still drowning."); */
        pline("당신은 아직도 익사 중이다.");
    }
    if (u.uinwater) {
        u.uinwater = 0;
#if 0 /*KR: 원본*/
        You("find yourself back %s.",
            Is_waterlevel(&u.uz) ? "in an air bubble" : "on land");
#else /*KR: KRNethack 맞춤 번역 */
        You("%s 돌아온 것을 깨달았다.",
            Is_waterlevel(&u.uz) ? "공기 방울 속으로" : "뭍으로");
#endif
    }
    return TRUE;
}

void drain_en(n) int n;
{
    if (!u.uenmax) {
        /* energy is completely gone */
        /*KR You_feel("momentarily lethargic."); */
        You("순간적으로 무기력해졌다.");
    } else {
        /* throttle further loss a bit when there's not much left to lose */
        if (n > u.uenmax || n > u.ulevel)
            n = rnd(n);

   /*KR You_feel("your magical energy drain away%c", (n > u.uen) ? '!' : '.'); */
        You("마력이 빠져나가는 것을 느꼈다%c", (n > u.uen) ? '!' : '.');
        u.uen -= n;
        if (u.uen < 0) {
            u.uenmax -= rnd(-u.uen);
            if (u.uenmax < 0)
                u.uenmax = 0;
            u.uen = 0;
        }
        context.botl = 1;
    }
}

/* disarm a trap */
int
dountrap()
{
    if (near_capacity() >= HVY_ENCUMBER) {
        /*KR pline("You're too strained to do that."); */
        pline("당신은 그렇게 하기에는 너무 무거운 짐을 지고 있다.");
        return 0;
    }
    if ((nohands(youmonst.data) && !webmaker(youmonst.data))
        || !youmonst.data->mmove) {
        /*KR pline("And just how do you expect to do that?"); */
        pline("대체 어떻게 그럴 수 있다고 생각하는 건가?");
        return 0;
    } else if (u.ustuck && sticks(youmonst.data)) {
#if 0 /*KR: 원본*/
        pline("You'll have to let go of %s first.", mon_nam(u.ustuck));
#else /*KR: KRNethack 맞춤 번역 */
        pline("먼저 %s 놓아주어야만 한다.",
              append_josa(mon_nam(u.ustuck), "을"));
#endif
        return 0;
    }
    if (u.ustuck || (welded(uwep) && bimanual(uwep))) {
#if 0 /*KR: 원본*/
        Your("%s seem to be too busy for that.", makeplural(body_part(HAND)));
#else /*KR: KRNethack 맞춤 번역 */
        Your("%s 그것을 하기엔 너무 바쁜 것 같다.",
             append_josa(makeplural(body_part(HAND)), "은"));
#endif
        return 0;
    }
    return untrap(FALSE);
}

/* Probability of disabling a trap.  Helge Hafting */
STATIC_OVL int
untrap_prob(ttmp)
struct trap *ttmp;
{
    int chance = 3;

    /* Only spiders know how to deal with webs reliably */
    if (ttmp->ttyp == WEB && !webmaker(youmonst.data))
        chance = 30;
    if (Confusion || Hallucination)
        chance++;
    if (Blind)
        chance++;
    if (Stunned)
        chance += 2;
    if (Fumbling)
        chance *= 2;
    /* Your own traps are better known than others. */
    if (ttmp && ttmp->madeby_u)
        chance--;
    if (Role_if(PM_ROGUE)) {
        if (rn2(2 * MAXULEV) < u.ulevel)
            chance--;
        if (u.uhave.questart && chance > 1)
            chance--;
    } else if (Role_if(PM_RANGER) && chance > 1)
        chance--;
    return rn2(chance);
}

/* Replace trap with object(s).  Helge Hafting */
void
cnv_trap_obj(otyp, cnt, ttmp, bury_it)
int otyp;
int cnt;
struct trap *ttmp;
boolean bury_it;
{
    struct obj *otmp = mksobj(otyp, TRUE, FALSE);

    otmp->quan = cnt;
    otmp->owt = weight(otmp);
    /* Only dart traps are capable of being poisonous */
    if (otyp != DART)
        otmp->opoisoned = 0;
    place_object(otmp, ttmp->tx, ttmp->ty);
    if (bury_it) {
        /* magical digging first disarms this trap, then will unearth it */
        (void) bury_an_obj(otmp, (boolean *) 0);
    } else {
        /* Sell your own traps only... */
        if (ttmp->madeby_u)
            sellobj(otmp, ttmp->tx, ttmp->ty);
        stackobj(otmp);
    }
    newsym(ttmp->tx, ttmp->ty);
    if (u.utrap && ttmp->tx == u.ux && ttmp->ty == u.uy)
        reset_utrap(TRUE);
    deltrap(ttmp);
}

/* while attempting to disarm an adjacent trap, we've fallen into it */
STATIC_OVL void
move_into_trap(ttmp)
struct trap *ttmp;
{
    int bc = 0;
    xchar x = ttmp->tx, y = ttmp->ty, bx, by, cx, cy;
    boolean unused;

    bx = by = cx = cy = 0; /* lint suppression */
    /* we know there's no monster in the way, and we're not trapped */
    if (!Punished
        || drag_ball(x, y, &bc, &bx, &by, &cx, &cy, &unused, TRUE)) {
        u.ux0 = u.ux, u.uy0 = u.uy;
        u.ux = x, u.uy = y;
        u.umoved = TRUE;
        newsym(u.ux0, u.uy0);
        vision_recalc(1);
        check_leash(u.ux0, u.uy0);
        if (Punished)
            move_bc(0, bc, bx, by, cx, cy);
        /* marking the trap unseen forces dotrap() to treat it like a new
           discovery and prevents pickup() -> look_here() -> check_here()
           from giving a redundant "there is a <trap> here" message when
           there are objects covering this trap */
        ttmp->tseen = 0; /* hack for check_here() */
        /* trigger the trap */
        iflags.failing_untrap++; /* spoteffects() -> dotrap(,FAILEDUNTRAP) */
        spoteffects(TRUE); /* pickup() + dotrap() */
        iflags.failing_untrap--;
        /* this should no longer be necessary; before the failing_untrap
           hack, Flying hero would not trigger an unseen bear trap and
           setting it not-yet-seen above resulted in leaving it hidden */
        if ((ttmp = t_at(u.ux, u.uy)) != 0)
            ttmp->tseen = 1;
        exercise(A_WIS, FALSE);
    }
}

/* 0: doesn't even try
 * 1: tries and fails
 * 2: succeeds
 */
STATIC_OVL int
try_disarm(ttmp, force_failure)
struct trap *ttmp;
boolean force_failure;
{
    struct monst *mtmp = m_at(ttmp->tx, ttmp->ty);
    int ttype = ttmp->ttyp;
    boolean under_u = (!u.dx && !u.dy);
    boolean holdingtrap = (ttype == BEAR_TRAP || ttype == WEB);

    /* Test for monster first, monsters are displayed instead of trap. */
    if (mtmp && (!mtmp->mtrapped || !holdingtrap)) {
#if 0 /*KR: 원본*/
        pline("%s is in the way.", Monnam(mtmp));
#else /*KR: KRNethack 맞춤 번역 */
        pline("%s 길을 막고 있다.", append_josa(Monnam(mtmp), "이"));
#endif
        return 0;
    }
    /* We might be forced to move onto the trap's location. */
    if (sobj_at(BOULDER, ttmp->tx, ttmp->ty) && !Passes_walls && !under_u) {
        /*KR There("is a boulder in your way."); */
        There("당신의 길에 바위가 있다.");
        return 0;
    }
    /* duplicate tight-space checks from test_move */
    if (u.dx && u.dy && bad_rock(youmonst.data, u.ux, ttmp->ty)
        && bad_rock(youmonst.data, ttmp->tx, u.uy)) {
        if ((invent && (inv_weight() + weight_cap() > 600))
            || bigmonst(youmonst.data)) {
            /* don't allow untrap if they can't get thru to it */
#if 0 /*KR: 원본*/
            You("are unable to reach the %s!",
                defsyms[trap_to_defsym(ttype)].explanation);
#else /*KR: KRNethack 맞춤 번역 */
            You("%s에 닿을 수 없다!",
                defsyms[trap_to_defsym(ttype)].explanation);
#endif
            return 0;
        }
    }
    /* untrappable traps are located on the ground. */
    if (!can_reach_floor(under_u)) {
        if (u.usteed && P_SKILL(P_RIDING) < P_BASIC)
            rider_cant_reach();
        else
#if 0 /*KR: 원본*/
            You("are unable to reach the %s!",
                defsyms[trap_to_defsym(ttype)].explanation);
#else /*KR: KRNethack 맞춤 번역 */
            You("%s에 닿을 수 없다!",
                defsyms[trap_to_defsym(ttype)].explanation);
#endif
        return 0;
    }

    /* Will our hero succeed? */
    if (force_failure || untrap_prob(ttmp)) {
        if (rnl(5)) {
            /*KR pline("Whoops..."); */
            pline("이런...");
            if (mtmp) { /* must be a trap that holds monsters */
                if (ttype == BEAR_TRAP) {
                    if (mtmp->mtame)
                        abuse_dog(mtmp);
                    mtmp->mhp -= rnd(4);
                    if (DEADMONSTER(mtmp))
                        killed(mtmp);
                } else if (ttype == WEB) {
                    if (!webmaker(youmonst.data)) {
                        struct trap *ttmp2 = maketrap(u.ux, u.uy, WEB);

                        if (ttmp2) {
                            /*KR pline_The("webbing sticks to you.  You're
                             * caught too!"); */
                            pline("거미줄이 당신에게 들러붙었다. 당신도 "
                                  "잡혔다!");
                            dotrap(ttmp2, NOWEBMSG);
                            if (u.usteed && u.utrap) {
                                /* you, not steed, are trapped */
                                dismount_steed(DISMOUNT_FELL);
                            }
                        }
                    } else
#if 0 /*KR: 원본*/
                        pline("%s remains entangled.", Monnam(mtmp));
#else /*KR: KRNethack 맞춤 번역 */
                        pline("%s 여전히 얽혀있다.",
                              append_josa(Monnam(mtmp), "은"));
#endif
                }
            } else if (under_u) {
                /* [don't need the iflags.failing_untrap hack here] */
                dotrap(ttmp, FAILEDUNTRAP);
            } else {
                move_into_trap(ttmp);
            }
        } else {
#if 0 /*KR: 원본*/
            pline("%s %s is difficult to %s.",
                  ttmp->madeby_u ? "Your" : under_u ? "This" : "That",
                  defsyms[trap_to_defsym(ttype)].explanation,
                  (ttype == WEB) ? "remove" : "disarm");
#else /*KR: KRNethack 맞춤 번역 */
            pline(
                "%s %s %s 어렵다.",
                ttmp->madeby_u ? "당신의"
                : under_u      ? "이"
                               : "저",
                append_josa(defsyms[trap_to_defsym(ttype)].explanation, "은"),
                (ttype == WEB) ? "제거하기" : "해제하기");
#endif
        }
        return 1;
    }
    return 2;
}

STATIC_OVL void reward_untrap(ttmp, mtmp) struct trap *ttmp;
struct monst *mtmp;
{
    if (!ttmp->madeby_u) {
        if (rnl(10) < 8 && !mtmp->mpeaceful && !mtmp->msleeping
            && !mtmp->mfrozen && !mindless(mtmp->data)
            && mtmp->data->mlet != S_HUMAN) {
            mtmp->mpeaceful = 1;
            set_malign(mtmp); /* reset alignment */
#if 0                         /*KR: 원본*/
            pline("%s is grateful.", Monnam(mtmp));
#else                         /*KR: KRNethack 맞춤 번역 */
            pline("%s 고마워한다.", append_josa(Monnam(mtmp), "은"));
#endif
        }
        /* Helping someone out of a trap is a nice thing to do,
         * A lawful may be rewarded, but not too often.  */
        if (!rn2(3) && !rnl(8) && u.ualign.type == A_LAWFUL) {
            adjalign(1);
            /*KR You_feel("that you did the right thing."); */
            You("옳은 일을 한 것 같다.");
        }
    }
}

STATIC_OVL int
disarm_holdingtrap(ttmp) /* Helge Hafting */
struct trap *ttmp;
{
    struct monst *mtmp;
    int fails = try_disarm(ttmp, FALSE);

    if (fails < 2)
        return fails;

    /* ok, disarm it. */

    /* untrap the monster, if any.
       There's no need for a cockatrice test, only the trap is touched */
    if ((mtmp = m_at(ttmp->tx, ttmp->ty)) != 0) {
        mtmp->mtrapped = 0;
#if 0 /*KR: 원본*/
        You("remove %s %s from %s.", the_your[ttmp->madeby_u],
            (ttmp->ttyp == BEAR_TRAP) ? "bear trap" : "webbing",
            mon_nam(mtmp));
#else /*KR: KRNethack 맞춤 번역 */
        You("%s에게서 %s%s 제거했다.", mon_nam(mtmp),
            ttmp->madeby_u ? "당신의 " : "",
            (ttmp->ttyp == BEAR_TRAP) ? "곰 덫을" : "거미줄을");
#endif
        reward_untrap(ttmp, mtmp);
    } else {
        if (ttmp->ttyp == BEAR_TRAP) {
#if 0 /*KR: 원본*/
            You("disarm %s bear trap.", the_your[ttmp->madeby_u]);
#else /*KR: KRNethack 맞춤 번역 */
            You("%s곰 덫을 해제했다.", ttmp->madeby_u ? "당신의 " : "");
#endif
            cnv_trap_obj(BEARTRAP, 1, ttmp, FALSE);
        } else /* if (ttmp->ttyp == WEB) */ {
#if 0 /*KR: 원본*/
            You("succeed in removing %s web.", the_your[ttmp->madeby_u]);
#else /*KR: KRNethack 맞춤 번역 */
            You("%s거미줄을 제거하는 데 성공했다.",
                ttmp->madeby_u ? "당신의 " : "");
#endif
            deltrap(ttmp);
        }
    }
    newsym(u.ux + u.dx, u.uy + u.dy);
    return 1;
}

STATIC_OVL int
disarm_landmine(ttmp) /* Helge Hafting */
struct trap *ttmp;
{
    int fails = try_disarm(ttmp, FALSE);

    if (fails < 2)
        return fails;
#if 0 /*KR: 원본*/
    You("disarm %s land mine.", the_your[ttmp->madeby_u]);
#else /*KR: KRNethack 맞춤 번역 */
    You("%s지뢰를 해제했다.", ttmp->madeby_u ? "당신의 " : "");
#endif
    cnv_trap_obj(LAND_MINE, 1, ttmp, FALSE);
    return 1;
}

/* getobj will filter down to cans of grease and known potions of oil */
static NEARDATA const char oil[] = { ALL_CLASSES, TOOL_CLASS, POTION_CLASS,
                                     0 };

/* it may not make much sense to use grease on floor boards, but so what? */
STATIC_OVL int
disarm_squeaky_board(ttmp)
struct trap *ttmp;
{
    struct obj *obj;
    boolean bad_tool;
    int fails;

    obj = getobj(oil, "untrap with");
    if (!obj)
        return 0;

    bad_tool = (obj->cursed
                || ((obj->otyp != POT_OIL || obj->lamplit)
                    && (obj->otyp != CAN_OF_GREASE || !obj->spe)));
    fails = try_disarm(ttmp, bad_tool);
    if (fails < 2)
        return fails;

    /* successfully used oil or grease to fix squeaky board */
    if (obj->otyp == CAN_OF_GREASE) {
        consume_obj_charge(obj, TRUE);
    } else {
        useup(obj); /* oil */
        makeknown(POT_OIL);
    }
    /*KR You("repair the squeaky board."); */ /* no madeby_u */
    You("삐걱거리는 판자를 고쳤다.");
    deltrap(ttmp);
    newsym(u.ux + u.dx, u.uy + u.dy);
    more_experienced(1, 5);
    newexplevel();
    return 1;
}

/* removes traps that shoot arrows, darts, etc. */
STATIC_OVL int
disarm_shooting_trap(ttmp, otyp)
struct trap *ttmp;
int otyp;
{
    int fails = try_disarm(ttmp, FALSE);

    if (fails < 2)
        return fails;
#if 0 /*KR: 원본*/
    You("disarm %s trap.", the_your[ttmp->madeby_u]);
#else /*KR: KRNethack 맞춤 번역 */
    You("%s함정을 해제했다.", ttmp->madeby_u ? "당신의 " : "");
#endif
    cnv_trap_obj(otyp, 50 - rnl(50), ttmp, FALSE);
    return 1;
}

/* Is the weight too heavy?
 * Formula as in near_capacity() & check_capacity() */
STATIC_OVL int
try_lift(mtmp, ttmp, wt, stuff)
struct monst *mtmp;
struct trap *ttmp;
int wt;
boolean stuff;
{
    int wc = weight_cap();

    if (((wt * 2) / wc) >= HVY_ENCUMBER) {
#if 0 /*KR: 원본*/
        pline("%s is %s for you to lift.", Monnam(mtmp),
              stuff ? "carrying too much" : "too heavy");
#else /*KR: KRNethack 맞춤 번역 */
        pline("%s 당신이 들어올리기에는 %s.", append_josa(Monnam(mtmp), "은"),
              stuff ? "너무 짐이 많다" : "너무 무겁다");
#endif
        if (!ttmp->madeby_u && !mtmp->mpeaceful && mtmp->mcanmove
            && !mindless(mtmp->data) && mtmp->data->mlet != S_HUMAN
            && rnl(10) < 3) {
            mtmp->mpeaceful = 1;
            set_malign(mtmp); /* reset alignment */
#if 0                         /*KR: 원본*/
            pline("%s thinks it was nice of you to try.", Monnam(mtmp));
#else                         /*KR: KRNethack 맞춤 번역 */
            pline("%s 시도해 준 것만으로도 고맙게 생각하는 것 같다.",
                  append_josa(Monnam(mtmp), "은"));
#endif
        }
        return 0;
    }
    return 1;
}

/* Help trapped monster (out of a (spiked) pit) */
STATIC_OVL int
help_monster_out(mtmp, ttmp)
struct monst *mtmp;
struct trap *ttmp;
{
    int wt;
    struct obj *otmp;
    boolean uprob;

    /*
     * This works when levitating too -- consistent with the ability
     * to hit monsters while levitating.
     *
     * Should perhaps check that our hero has arms/hands at the
     * moment.  Helping can also be done by engulfing...
     *
     * Test the monster first - monsters are displayed before traps.
     */
    if (!mtmp->mtrapped) {
#if 0 /*KR: 원본*/
        pline("%s isn't trapped.", Monnam(mtmp));
#else /*KR: KRNethack 맞춤 번역 */
        pline("%s 함정에 걸리지 않았다.", append_josa(Monnam(mtmp), "은"));
#endif
        return 0;
    }
    /* Do you have the necessary capacity to lift anything? */
    if (check_capacity((char *) 0))
        return 1;

    /* Will our hero succeed? */
    if ((uprob = untrap_prob(ttmp)) && !mtmp->msleeping && mtmp->mcanmove) {
#if 0 /*KR: 원본*/
        You("try to reach out your %s, but %s backs away skeptically.",
            makeplural(body_part(ARM)), mon_nam(mtmp));
#else /*KR: KRNethack 맞춤 번역 */
        You("%s 뻗어보려 했지만, %s 의심스러운 듯 뒤로 물러났다.",
            append_josa(makeplural(body_part(ARM)), "을"),
            append_josa(mon_nam(mtmp), "는"));
#endif
        return 1;
    }

    /* is it a cockatrice?... */
    if (touch_petrifies(mtmp->data) && !uarmg && !Stone_resistance) {
#if 0 /*KR: 원본*/
        You("grab the trapped %s using your bare %s.", mtmp->data->mname,
            makeplural(body_part(HAND)));
#else /*KR: KRNethack 맞춤 번역 */
        You("맨%s 함정에 걸린 %s 잡았다.",
            append_josa(makeplural(body_part(HAND)), "으로"),
            append_josa(mtmp->data->mname, "을"));
#endif

        if (poly_when_stoned(youmonst.data) && polymon(PM_STONE_GOLEM)) {
            display_nhwindow(WIN_MESSAGE, FALSE);
        } else {
            char kbuf[BUFSZ];

#if 0 /*KR: 원본*/
            Sprintf(kbuf, "trying to help %s out of a pit",
                    an(mtmp->data->mname));
#else /*KR: KRNethack 맞춤 번역 */
            Sprintf(kbuf, "구덩이에서 %s 꺼내려다",
                    append_josa(mtmp->data->mname, "을"));
#endif
            instapetrify(kbuf);
            return 1;
        }
    }
    /* need to do cockatrice check first if sleeping or paralyzed */
    if (uprob) {
#if 0 /*KR: 원본*/
        You("try to grab %s, but cannot get a firm grasp.", mon_nam(mtmp));
#else /*KR: KRNethack 맞춤 번역 */
        You("%s 잡으려 했으나, 단단히 움켜쥘 수 없었다.",
            append_josa(mon_nam(mtmp), "을"));
#endif
        if (mtmp->msleeping) {
            mtmp->msleeping = 0;
#if 0 /*KR: 원본*/
            pline("%s awakens.", Monnam(mtmp));
#else /*KR: KRNethack 맞춤 번역 */
            pline("%s 깨어났다.", append_josa(Monnam(mtmp), "이"));
#endif
        }
        return 1;
    }

#if 0 /*KR: 원본*/
    You("reach out your %s and grab %s.", makeplural(body_part(ARM)),
        mon_nam(mtmp));
#else /*KR: KRNethack 맞춤 번역 */
    You("%s 뻗어 %s 꽉 잡았다.",
        append_josa(makeplural(body_part(ARM)), "을"),
        append_josa(mon_nam(mtmp), "을"));
#endif

    if (mtmp->msleeping) {
        mtmp->msleeping = 0;
#if 0 /*KR: 원본*/
        pline("%s awakens.", Monnam(mtmp));
#else /*KR: KRNethack 맞춤 번역 */
        pline("%s 깨어났다.", append_josa(Monnam(mtmp), "이"));
#endif
    } else if (mtmp->mfrozen && !rn2(mtmp->mfrozen)) {
        /* After such manhandling, perhaps the effect wears off */
        mtmp->mcanmove = 1;
        mtmp->mfrozen = 0;
#if 0 /*KR: 원본*/
        pline("%s stirs.", Monnam(mtmp));
#else /*KR: KRNethack 맞춤 번역 */
        pline("%s 몸을 뒤척였다.", append_josa(Monnam(mtmp), "이"));
#endif
    }

    /* is the monster too heavy? */
    wt = inv_weight() + mtmp->data->cwt;
    if (!try_lift(mtmp, ttmp, wt, FALSE))
        return 1;

    /* is the monster with inventory too heavy? */
    for (otmp = mtmp->minvent; otmp; otmp = otmp->nobj)
        wt += otmp->owt;
    if (!try_lift(mtmp, ttmp, wt, TRUE))
        return 1;

#if 0 /*KR: 원본*/
    You("pull %s out of the pit.", mon_nam(mtmp));
#else /*KR: KRNethack 맞춤 번역 */
    You("%s 구덩이에서 끌어올렸다.", append_josa(mon_nam(mtmp), "을"));
#endif
    mtmp->mtrapped = 0;
    reward_untrap(ttmp, mtmp);
    fill_pit(mtmp->mx, mtmp->my);
    return 1;
}

int
untrap(force)
boolean force;
{
    register struct obj *otmp;
    register int x, y;
    int ch;
    struct trap *ttmp;
    struct monst *mtmp;
    const char *trapdescr;
    boolean here, useplural, deal_with_floor_trap,
        confused = (Confusion || Hallucination), trap_skipped = FALSE;
    int boxcnt = 0;
    char the_trap[BUFSZ], qbuf[QBUFSZ];

    if (!getdir((char *) 0))
        return 0;
    x = u.ux + u.dx;
    y = u.uy + u.dy;
    if (!isok(x, y)) {
        /*KR pline_The("perils lurking there are beyond your grasp."); */
        pline("그곳에 도사리고 있는 위험은 당신이 파악할 수 없다.");
        return 0;
    }
    /* 'force' is true for #invoke; make it be true for #untrap if
       carrying MKoT */
    if (!force && has_magic_key(&youmonst))
        force = TRUE;

    ttmp = t_at(x, y);
    if (ttmp && !ttmp->tseen)
        ttmp = 0;
    trapdescr = ttmp ? defsyms[trap_to_defsym(ttmp->ttyp)].explanation : 0;
    here = (x == u.ux && y == u.uy); /* !u.dx && !u.dy */

    if (here) /* are there are one or more containers here? */
        for (otmp = level.objects[x][y]; otmp; otmp = otmp->nexthere)
            if (Is_box(otmp)) {
                if (++boxcnt > 1)
                    break;
            }

    deal_with_floor_trap = can_reach_floor(FALSE);
    if (!deal_with_floor_trap) {
        *the_trap = '\0';
        if (ttmp)
            Strcat(the_trap, an(trapdescr));
        if (ttmp && boxcnt)
            Strcat(the_trap, " and ");
        if (boxcnt)
            Strcat(the_trap, (boxcnt == 1) ? "a container" : "containers");
        useplural = ((ttmp && boxcnt > 0) || boxcnt > 1);
        /* note: boxcnt and useplural will always be 0 for !here case */
        if (ttmp || boxcnt)
#if 0 /*KR: 원본*/
            There("%s %s %s but you can't reach %s%s.",
                  useplural ? "are" : "is", the_trap, here ? "here" : "there",
                  useplural ? "them" : "it",
                  u.usteed ? " while mounted" : "");
#else /*KR: KRNethack 맞춤 번역 */
            There("%s %s 있지만%s 닿을 수 없다.", here ? "여기에" : "저기에",
                  the_trap, u.usteed ? " 탑승한 상태로는" : "");
#endif
        trap_skipped = (ttmp != 0);
    } else { /* deal_with_floor_trap */

        if (ttmp) {
            Strcpy(the_trap, the(trapdescr));
            if (boxcnt) {
                if (is_pit(ttmp->ttyp)) {
#if 0 /*KR: 원본*/
                    You_cant("do much about %s%s.", the_trap,
                             u.utrap ? " that you're stuck in"
                                     : " while standing on the edge of it");
#else /*KR: KRNethack 맞춤 번역 */
                    You_cant("%s%s 어찌할 수 없다.",
                             u.utrap ? "당신이 빠져 있는 "
                                     : "가장자리에 서서는 ",
                             the_trap);
#endif
                    trap_skipped = TRUE;
                    deal_with_floor_trap = FALSE;
                } else {
#if 0 /*KR: 원본*/
                    Sprintf(
                        qbuf, "There %s and %s here.  %s %s?",
                        (boxcnt == 1) ? "is a container" : "are containers",
                        an(trapdescr),
                        (ttmp->ttyp == WEB) ? "Remove" : "Disarm", the_trap);
#else /*KR: KRNethack 맞춤 번역 */
                    Sprintf(qbuf, "여기에 %s와 %s 있다. %s %s?",
                            (boxcnt == 1) ? "상자" : "상자들", trapdescr,
                            the_trap,
                            (ttmp->ttyp == WEB) ? "제거하겠습니까"
                                                : "해제하겠습니까");
#endif
                    switch (ynq(qbuf)) {
                    case 'q':
                        return 0;
                    case 'n':
                        trap_skipped = TRUE;
                        deal_with_floor_trap = FALSE;
                        break;
                    }
                }
            }
            if (deal_with_floor_trap) {
                if (u.utrap) {
#if 0 /*KR: 원본*/
                    You("cannot deal with %s while trapped%s!", the_trap,
                        (x == u.ux && y == u.uy) ? " in it" : "");
#else /*KR: KRNethack 맞춤 번역 */
                    You("함정에 걸린 상태%s로는 %s 어찌할 수 없다!",
                        (x == u.ux && y == u.uy) ? "(그것도 바로 그 함정에!)"
                                                 : "",
                        append_josa(the_trap, "을"));
#endif
                    return 1;
                }
                if ((mtmp = m_at(x, y)) != 0
                    && (M_AP_TYPE(mtmp) == M_AP_FURNITURE
                        || M_AP_TYPE(mtmp) == M_AP_OBJECT)) {
                    stumble_onto_mimic(mtmp);
                    return 1;
                }
                switch (ttmp->ttyp) {
                case BEAR_TRAP:
                case WEB:
                    return disarm_holdingtrap(ttmp);
                case LANDMINE:
                    return disarm_landmine(ttmp);
                case SQKY_BOARD:
                    return disarm_squeaky_board(ttmp);
                case DART_TRAP:
                    return disarm_shooting_trap(ttmp, DART);
                case ARROW_TRAP:
                    return disarm_shooting_trap(ttmp, ARROW);
                case PIT:
                case SPIKED_PIT:
                    if (here) {
                        /*KR You("are already on the edge of the pit."); */
                        You("이미 구덩이의 가장자리에 있다.");
                        return 0;
                    }
                    if (!mtmp) {
                        /*KR pline("Try filling the pit instead."); */
                        pline("대신 구덩이를 메워 보아라.");
                        return 0;
                    }
                    return help_monster_out(mtmp, ttmp);
                default:
#if 0 /*KR: 원본*/
                    You("cannot disable %s trap.", !here ? "that" : "this");
#else /*KR: KRNethack 맞춤 번역 */
                    You("%s 함정을 무효화할 수 없다.", !here ? "저" : "이");
#endif
                    return 0;
                }
            }
        } /* end if */

        if (boxcnt) {
            for (otmp = level.objects[x][y]; otmp; otmp = otmp->nexthere)
                if (Is_box(otmp)) {
#if 0 /*KR: 원본*/
                    (void) safe_qbuf(qbuf, "There is ",
                                     " here.  Check it for traps?", otmp,
                                     doname, ansimpleoname, "a box");
#else /*KR: KRNethack 맞춤 번역 */
                    (void) safe_qbuf(qbuf, "여기에 ",
                                     " 있다. 함정이 있는지 조사하겠습니까?",
                                     otmp, doname, ansimpleoname, "상자가");
#endif
                    switch (ynq(qbuf)) {
                    case 'q':
                        return 0;
                    case 'n':
                        continue;
                    }

                    if ((otmp->otrapped
                         && (force
                             || (!confused
                                 && rn2(MAXULEV + 1 - u.ulevel) < 10)))
                        || (!force && confused && !rn2(3))) {
#if 0 /*KR: 원본*/
                        You("find a trap on %s!", the(xname(otmp)));
#else /*KR: KRNethack 맞춤 번역 */
                        You("%s에서 함정을 발견했다!", the(xname(otmp)));
#endif
                        if (!confused)
                            exercise(A_WIS, TRUE);

#if 0 /*KR: 원본*/
                        switch (ynq("Disarm it?")) {
#else /*KR: KRNethack 맞춤 번역 */
                        switch (ynq("해제하겠습니까?")) {
#endif
                        case 'q':
                            return 1;
                        case 'n':
                            trap_skipped = TRUE;
                            continue;
                        }

                        if (otmp->otrapped) {
                            exercise(A_DEX, TRUE);
                            ch = ACURR(A_DEX) + u.ulevel;
                            if (Role_if(PM_ROGUE))
                                ch *= 2;
                            if (!force
                                && (confused || Fumbling
                                    || rnd(75 + level_difficulty() / 2)
                                           > ch)) {
                                (void) chest_trap(otmp, FINGER, TRUE);
                            } else {
                                /*KR You("disarm it!"); */
                                You("해제했다!");
                                otmp->otrapped = 0;
                            }
                        } else
#if 0 /*KR: 원본*/
                            pline("That %s was not trapped.", xname(otmp));
#else /*KR: KRNethack 맞춤 번역 */
                            pline("그 %s에는 함정이 없었다.", xname(otmp));
#endif
                        return 1;
                    } else {
#if 0 /*KR: 원본*/
                        You("find no traps on %s.", the(xname(otmp)));
#else /*KR: KRNethack 맞춤 번역 */
                        You("%s에서 어떤 함정도 찾지 못했다.",
                            the(xname(otmp)));
#endif
                        return 1;
                    }
                }

#if 0 /*KR: 원본*/
            You(trap_skipped ? "find no other traps here."
                             : "know of no traps here.");
#else /*KR: KRNethack 맞춤 번역 */
            You(trap_skipped ? "여기서 다른 함정은 찾지 못했다."
                             : "여기에 함정이 있는지 모른다.");
#endif
            return 0;
        }

        if (stumble_on_door_mimic(x, y))
            return 1;

    } /* deal_with_floor_trap */
    /* doors can be manipulated even while levitating/unskilled riding */

    if (!IS_DOOR(levl[x][y].typ)) {
        if (!trap_skipped)
            /*KR You("know of no traps there."); */
            You("그곳에 함정이 있는지 모른다.");
        return 0;
    }

    switch (levl[x][y].doormask) {
    case D_NODOOR:
#if 0 /*KR: 원본*/
        You("%s no door there.", Blind ? "feel" : "see");
#else /*KR: KRNethack 맞춤 번역 */
        You("그곳에 문이 없는 것을 %s.", Blind ? "느꼈다" : "보았다");
#endif
        return 0;
    case D_ISOPEN:
        /*KR pline("This door is safely open."); */
        pline("이 문은 안전하게 열려 있다.");
        return 0;
    case D_BROKEN:
        /*KR pline("This door is broken."); */
        pline("이 문은 부서져 있다.");
        return 0;
    }

    if (((levl[x][y].doormask & D_TRAPPED) != 0
         && (force || (!confused && rn2(MAXULEV - u.ulevel + 11) < 10)))
        || (!force && confused && !rn2(3))) {
        /*KR You("find a trap on the door!"); */
        You("문에서 함정을 발견했다!");
        exercise(A_WIS, TRUE);
#if 0 /*KR: 원본*/
        if (ynq("Disarm it?") != 'y')
#else /*KR: KRNethack 맞춤 번역 */
        if (ynq("해제하겠습니까?") != 'y')
#endif
            return 1;
        if (levl[x][y].doormask & D_TRAPPED) {
            ch = 15 + (Role_if(PM_ROGUE) ? u.ulevel * 3 : u.ulevel);
            exercise(A_DEX, TRUE);
            if (!force
                && (confused || Fumbling
                    || rnd(75 + level_difficulty() / 2) > ch)) {
                /*KR You("set it off!"); */
                You("함정을 작동시켰다!");
                b_trapped("door", FINGER);
                levl[x][y].doormask = D_NODOOR;
                unblock_point(x, y);
                newsym(x, y);
                /* (probably ought to charge for this damage...) */
                if (*in_rooms(x, y, SHOPBASE))
                    add_damage(x, y, 0L);
            } else {
                /*KR You("disarm it!"); */
                You("해제했다!");
                levl[x][y].doormask &= ~D_TRAPPED;
            }
        } else
            /*KR pline("This door was not trapped."); */
            pline("이 문에는 함정이 없었다.");
        return 1;
    } else {
        /*KR You("find no traps on the door."); */
        You("문에서 어떤 함정도 찾지 못했다.");
        return 1;
    }
}

/* for magic unlocking; returns true if targetted monster (which might
   be hero) gets untrapped; the trap remains intact */
boolean
openholdingtrap(mon, noticed)
struct monst *mon;
boolean *noticed; /* set to true iff hero notices the effect; */
{                 /* otherwise left with its previous value intact */
    struct trap *t;
    char buf[BUFSZ], whichbuf[20];
    const char *trapdescr = 0, *which = 0;
    boolean ishero = (mon == &youmonst);

    if (!mon)
        return FALSE;
    if (mon == u.usteed)
        ishero = TRUE;

    t = t_at(ishero ? u.ux : mon->mx, ishero ? u.uy : mon->my);

    if (ishero && u.utrap) { /* all u.utraptype values are holding traps */
        which = the_your[(!t || !t->tseen || !t->madeby_u) ? 0 : 1];
        switch (u.utraptype) {
        case TT_LAVA:
            trapdescr = "molten lava";
            break;
        case TT_INFLOOR:
            /* solidified lava, so not "floor" even if within a room */
            trapdescr = "ground";
            break;
        case TT_BURIEDBALL:
            trapdescr = "your anchor";
            which = "";
            break;
        case TT_BEARTRAP:
        case TT_PIT:
        case TT_WEB:
            trapdescr = 0; /* use defsyms[].explanation */
            break;
        default:
            /* lint suppression in case 't' is unexpectedly Null
               or u.utraptype has new value we don't know about yet */
            trapdescr = "trap";
            break;
        }
    } else {
        /* if no trap here or it's not a holding trap, we're done */
        if (!t || (t->ttyp != BEAR_TRAP && t->ttyp != WEB))
            return FALSE;
    }

    if (!trapdescr)
        trapdescr = defsyms[trap_to_defsym(t->ttyp)].explanation;
    if (!which)
        which = t->tseen                    ? the_your[t->madeby_u]
                : index(vowels, *trapdescr) ? "an"
                                            : "a";
    if (*which)
        which = strcat(strcpy(whichbuf, which), " ");

    if (ishero) {
        if (!u.utrap)
            return FALSE;
        *noticed = TRUE;
#if 0 /*KR: 원본*/
        if (u.usteed)
            Sprintf(buf, "%s is", noit_Monnam(u.usteed));
        else
            Strcpy(buf, "You are");
        reset_utrap(TRUE);
        vision_full_recalc = 1; /* vision limits can change (pit escape) */
        pline("%s released from %s%s.", buf, which, trapdescr);
#else /*KR: KRNethack 맞춤 번역 */
        if (u.usteed)
            Sprintf(buf, "%s", append_josa(noit_Monnam(u.usteed), "는"));
        else
            Strcpy(buf, "당신은");
        reset_utrap(TRUE);
        vision_full_recalc = 1; /* vision limits can change (pit escape) */
        pline("%s %s%s에서 풀려났다.", buf, which, trapdescr);
#endif
    } else {
        if (!mon->mtrapped)
            return FALSE;
        mon->mtrapped = 0;
        if (canspotmon(mon)) {
            *noticed = TRUE;
#if 0 /*KR: 원본*/
            pline("%s is released from %s%s.", Monnam(mon), which,
                  trapdescr);
#else /*KR: KRNethack 맞춤 번역 */
            pline("%s %s%s에서 풀려났다.", append_josa(Monnam(mon), "은"),
                  which, trapdescr);
#endif
        } else if (cansee(t->tx, t->ty) && t->tseen) {
            *noticed = TRUE;
#if 0 /*KR: 원본*/
            if (t->ttyp == WEB)
                pline("%s is released from %s%s.", Something, which,
                      trapdescr);
            else /* BEAR_TRAP */
                pline("%s%s opens.", upstart(strcpy(buf, which)), trapdescr);
#else /*KR: KRNethack 맞춤 번역 */
            if (t->ttyp == WEB)
                pline("%s %s%s에서 풀려났다.", Something, which, trapdescr);
            else /* BEAR_TRAP */
                pline("%s%s 열렸다.", upstart(strcpy(buf, which)),
                      append_josa(trapdescr, "이"));
#endif
        }
        /* might pacify monster if adjacent */
        if (rn2(2) && distu(mon->mx, mon->my) <= 2)
            reward_untrap(t, mon);
    }
    return TRUE;
}

/* for magic locking; returns true if targetted monster (which might
   be hero) gets hit by a trap (might avoid actually becoming trapped) */
boolean
closeholdingtrap(mon, noticed)
struct monst *mon;
boolean *noticed; /* set to true iff hero notices the effect; */
{                 /* otherwise left with its previous value intact */
    struct trap *t;
    unsigned dotrapflags;
    boolean ishero = (mon == &youmonst), result;

    if (!mon)
        return FALSE;
    if (mon == u.usteed)
        ishero = TRUE;
    t = t_at(ishero ? u.ux : mon->mx, ishero ? u.uy : mon->my);
    /* if no trap here or it's not a holding trap, we're done */
    if (!t || (t->ttyp != BEAR_TRAP && t->ttyp != WEB))
        return FALSE;

    if (ishero) {
        if (u.utrap)
            return FALSE; /* already trapped */
        *noticed = TRUE;
        dotrapflags = FORCETRAP;
        /* dotrap calls mintrap when mounted hero encounters a web */
        if (u.usteed)
            dotrapflags |= NOWEBMSG;
        ++force_mintrap;
        dotrap(t, dotrapflags);
        --force_mintrap;
        result = (u.utrap != 0);
    } else {
        if (mon->mtrapped)
            return FALSE; /* already trapped */
        /* you notice it if you see the trap close/tremble/whatever
           or if you sense the monster who becomes trapped */
        *noticed = cansee(t->tx, t->ty) || canspotmon(mon);
        ++force_mintrap;
        result = (mintrap(mon) != 0);
        --force_mintrap;
    }
    return result;
}

/* for magic unlocking; returns true if targetted monster (which might
   be hero) gets hit by a trap (target might avoid its effect) */
boolean
openfallingtrap(mon, trapdoor_only, noticed)
struct monst *mon;
boolean trapdoor_only;
boolean *noticed; /* set to true iff hero notices the effect; */
{                 /* otherwise left with its previous value intact */
    struct trap *t;
    boolean ishero = (mon == &youmonst), result;

    if (!mon)
        return FALSE;
    if (mon == u.usteed)
        ishero = TRUE;
    t = t_at(ishero ? u.ux : mon->mx, ishero ? u.uy : mon->my);
    /* if no trap here or it's not a falling trap, we're done
       (note: falling rock traps have a trapdoor in the ceiling) */
    if (!t || ((t->ttyp != TRAPDOOR && t->ttyp != ROCKTRAP)
               && (trapdoor_only || (t->ttyp != HOLE && !is_pit(t->ttyp)))))
        return FALSE;

    if (ishero) {
        if (u.utrap)
            return FALSE; /* already trapped */
        *noticed = TRUE;
        dotrap(t, FORCETRAP);
        result = (u.utrap != 0);
    } else {
        if (mon->mtrapped)
            return FALSE; /* already trapped */
        /* you notice it if you see the trap close/tremble/whatever
           or if you sense the monster who becomes trapped */
        *noticed = cansee(t->tx, t->ty) || canspotmon(mon);
        /* monster will be angered; mintrap doesn't handle that */
        wakeup(mon, TRUE);
        ++force_mintrap;
        result = (mintrap(mon) != 0);
        --force_mintrap;
        /* mon might now be on the migrating monsters list */
    }
    return result;
}

/* only called when the player is doing something to the chest directly */
boolean
chest_trap(obj, bodypart, disarm)
register struct obj *obj;
register int bodypart;
boolean disarm;
{
    register struct obj *otmp = obj, *otmp2;
    char buf[80];
    const char *msg;
    coord cc;

    if (get_obj_location(obj, &cc.x, &cc.y, 0)) /* might be carried */
        obj->ox = cc.x, obj->oy = cc.y;

    otmp->otrapped = 0; /* trap is one-shot; clear flag first in case
                           chest kills you and ends up in bones file */
#if 0                   /*KR: 원본*/
    You(disarm ? "set it off!" : "trigger a trap!");
#else                   /*KR: KRNethack 맞춤 번역 */
    You(disarm ? "그것을 터뜨렸다!" : "함정을 작동시켰다!");
#endif
    display_nhwindow(WIN_MESSAGE, FALSE);
    if (Luck > -13 && rn2(13 + Luck) > 7) { /* saved by luck */
        /* trap went off, but good luck prevents damage */
        switch (rn2(13)) {
        case 12:
        case 11:
            /*KR msg = "explosive charge is a dud"; */
            msg = "폭약이 불발이었다";
            break;
        case 10:
        case 9:
            /*KR msg = "electric charge is grounded"; */
            msg = "전류가 땅으로 흘렀다";
            break;
        case 8:
        case 7:
            /*KR msg = "flame fizzles out"; */
            msg = "불꽃이 사그라들었다";
            break;
        case 6:
        case 5:
        case 4:
            /*KR msg = "poisoned needle misses"; */
            msg = "독침이 빗나갔다";
            break;
        case 3:
        case 2:
        case 1:
        case 0:
            /*KR msg = "gas cloud blows away"; */
            msg = "가스 구름이 날아가 버렸다";
            break;
        default:
            impossible("chest disarm bug");
            msg = (char *) 0;
            break;
        }
        if (msg)
            /*KR pline("But luckily the %s!", msg); */
            pline("하지만 다행히도 %s!", msg);
    } else {
        switch (rn2(20) ? ((Luck >= 13) ? 0 : rn2(13 - Luck)) : rn2(26)) {
        case 25:
        case 24:
        case 23:
        case 22:
        case 21: {
            struct monst *shkp = 0;
            long loss = 0L;
            boolean costly, insider;
            register xchar ox = obj->ox, oy = obj->oy;

            /* the obj location need not be that of player */
            costly = (costly_spot(ox, oy)
                      && (shkp = shop_keeper(*in_rooms(ox, oy, SHOPBASE)))
                             != (struct monst *) 0);
            insider = (*u.ushops && inside_shop(u.ux, u.uy)
                       && *in_rooms(ox, oy, SHOPBASE) == *u.ushops);

#if 0 /*KR: 원본*/
            pline("%s!", Tobjnam(obj, "explode"));
#else /*KR: KRNethack 맞춤 번역 */
            pline("%s 폭발했다!", append_josa(The(xname(obj)), "이"));
#endif
            /*KR Sprintf(buf, "exploding %s", xname(obj)); */
            Sprintf(buf, "폭발하는 %s", xname(obj));

            if (costly)
                loss += stolen_value(obj, ox, oy, (boolean) shkp->mpeaceful,
                                     TRUE);
            delete_contents(obj);
            /* unpunish() in advance if either ball or chain (or both) is
             * going to be destroyed */
            if (Punished
                && ((uchain->ox == u.ux && uchain->oy == u.uy)
                    || (uball->where == OBJ_FLOOR && uball->ox == u.ux
                        && uball->oy == u.uy)))
                unpunish();

            for (otmp = level.objects[u.ux][u.uy]; otmp; otmp = otmp2) {
                otmp2 = otmp->nexthere;
                if (costly)
                    loss += stolen_value(otmp, otmp->ox, otmp->oy,
                                         (boolean) shkp->mpeaceful, TRUE);
                delobj(otmp);
            }
            wake_nearby();
            losehp(Maybe_Half_Phys(d(6, 6)), buf, KILLED_BY_AN);
            exercise(A_STR, FALSE);
            if (costly && loss) {
                if (insider)
#if 0 /*KR: 원본*/
                    You("owe %ld %s for objects destroyed.", loss,
                        currency(loss));
#else /*KR: KRNethack 맞춤 번역 */
                    You("파괴된 물건값으로 %ld %s 빚졌다.", loss,
                        currency(loss));
#endif
                else {
#if 0 /*KR: 원본*/
                    You("caused %ld %s worth of damage!", loss,
                        currency(loss));
#else /*KR: KRNethack 맞춤 번역 */
                    You("%ld %s 상당의 피해를 입혔다!", loss, currency(loss));
#endif
                    make_angry_shk(shkp, ox, oy);
                }
            }
            return TRUE;
        } /* case 21 */
        case 20:
        case 19:
        case 18:
        case 17:
#if 0 /*KR: 원본*/
            pline("A cloud of noxious gas billows from %s.", the(xname(obj)));
            poisoned("gas cloud", A_STR, "cloud of poison gas", 15, FALSE);
#else /*KR: KRNethack 맞춤 번역 */
            pline("%s에서 유독 가스 구름이 피어올랐다.", the(xname(obj)));
            poisoned("가스 구름", A_STR, "독가스 구름", 15, FALSE);
#endif
            exercise(A_CON, FALSE);
            break;
        case 16:
        case 15:
        case 14:
        case 13:
#if 0 /*KR: 원본*/
            You_feel("a needle prick your %s.", body_part(bodypart));
            poisoned("needle", A_CON, "poisoned needle", 10, FALSE);
#else /*KR: KRNethack 맞춤 번역 */
            You("바늘이 당신의 %s 찌르는 것을 느꼈다.",
                     append_josa(body_part(bodypart), "을"));
            poisoned("바늘", A_CON, "독침", 10, FALSE);
#endif
            exercise(A_CON, FALSE);
            break;
        case 12:
        case 11:
        case 10:
        case 9:
            dofiretrap(obj);
            break;
        case 8:
        case 7:
        case 6: {
            int dmg;

            /*KR You("are jolted by a surge of electricity!"); */
            You("강한 전류에 감전되었다!");
            if (Shock_resistance) {
                shieldeff(u.ux, u.uy);
                /*KR You("don't seem to be affected."); */
                You("아무런 영향도 받지 않은 것 같다.");
                dmg = 0;
            } else
                dmg = d(4, 4);
            destroy_item(RING_CLASS, AD_ELEC);
            destroy_item(WAND_CLASS, AD_ELEC);
            if (dmg)
#if 0 /*KR: 원본*/
                losehp(dmg, "electric shock", KILLED_BY_AN);
#else /*KR: KRNethack 맞춤 번역 */
                losehp(dmg, "감전", KILLED_BY);
#endif
            break;
        } /* case 6 */
        case 5:
        case 4:
        case 3:
            if (!Free_action) {
                /*KR pline("Suddenly you are frozen in place!"); */
                pline("갑자기 그 자리에 얼어붙었다!");
                nomul(-d(5, 6));
                /*KR multi_reason = "frozen by a trap"; */
                multi_reason = "함정에 의해 얼어붙음";
                exercise(A_DEX, FALSE);
                nomovemsg = You_can_move_again;
            } else
                /*KR You("momentarily stiffen."); */
                You("순간적으로 몸이 굳었다.");
            break;
        case 2:
        case 1:
        case 0:
#if 0 /*KR: 원본*/
            pline("A cloud of %s gas billows from %s.",
                  Blind ? blindgas[rn2(SIZE(blindgas))] : rndcolor(),
                  the(xname(obj)));
#else /*KR: KRNethack 맞춤 번역 */
            pline("%s에서 %s 가스 구름이 피어올랐다.", the(xname(obj)),
                  Blind ? blindgas[rn2(SIZE(blindgas))] : rndcolor());
#endif
            if (!Stunned) {
                if (Hallucination)
                    /*KR pline("What a groovy feeling!"); */
                    pline("기분이 끝내주는데!");
                else
#if 0 /*KR: 원본*/
                    You("%s%s...", stagger(youmonst.data, "stagger"),
                        Halluc_resistance ? ""
                                          : Blind ? " and get dizzy"
                                                  : " and your vision blurs");
#else /*KR: KRNethack 맞춤 번역 */
                    You("%s%s...", stagger(youmonst.data, "비틀거렸다"),
                        Halluc_resistance ? ""
                        : Blind           ? " 그리고 현기증이 났다"
                                          : " 그리고 시야가 흐려졌다");
#endif
            }
            make_stunned((HStun & TIMEOUT) + (long) rn1(7, 16), FALSE);
            (void) make_hallucinated(
                (HHallucination & TIMEOUT) + (long) rn1(5, 16), FALSE, 0L);
            break;
        default:
            impossible("bad chest trap");
            break;
        }
        bot(); /* to get immediate botl re-display */
    }

    return FALSE;
}

struct trap *
t_at(x, y)
register int x, y;
{
    register struct trap *trap = ftrap;

    while (trap) {
        if (trap->tx == x && trap->ty == y)
            return trap;
        trap = trap->ntrap;
    }
    return (struct trap *) 0;
}

void
deltrap(trap)
register struct trap *trap;
{
    register struct trap *ttmp;

    clear_conjoined_pits(trap);
    if (trap == ftrap) {
        ftrap = ftrap->ntrap;
    } else {
        for (ttmp = ftrap; ttmp; ttmp = ttmp->ntrap)
            if (ttmp->ntrap == trap)
                break;
        if (!ttmp)
            panic("deltrap: no preceding trap!");
        ttmp->ntrap = trap->ntrap;
    }
    if (Sokoban && (trap->ttyp == PIT || trap->ttyp == HOLE))
        maybe_finish_sokoban();
    dealloc_trap(trap);
}

boolean
conjoined_pits(trap2, trap1, u_entering_trap2)
struct trap *trap2, *trap1;
boolean u_entering_trap2;
{
    int dx, dy, diridx, adjidx;

    if (!trap1 || !trap2)
        return FALSE;
    if (!isok(trap2->tx, trap2->ty) || !isok(trap1->tx, trap1->ty)
        || !is_pit(trap2->ttyp)
        || !is_pit(trap1->ttyp)
        || (u_entering_trap2 && !(u.utrap && u.utraptype == TT_PIT)))
        return FALSE;
    dx = sgn(trap2->tx - trap1->tx);
    dy = sgn(trap2->ty - trap1->ty);
    for (diridx = 0; diridx < 8; diridx++)
        if (xdir[diridx] == dx && ydir[diridx] == dy)
            break;
    /* diridx is valid if < 8 */
    if (diridx < 8) {
        adjidx = (diridx + 4) % 8;
        if ((trap1->conjoined & (1 << diridx))
            && (trap2->conjoined & (1 << adjidx)))
            return TRUE;
    }
    return FALSE;
}

STATIC_OVL void
clear_conjoined_pits(trap)
struct trap *trap;
{
    int diridx, adjidx, x, y;
    struct trap *t;

    if (trap && is_pit(trap->ttyp)) {
        for (diridx = 0; diridx < 8; ++diridx) {
            if (trap->conjoined & (1 << diridx)) {
                x = trap->tx + xdir[diridx];
                y = trap->ty + ydir[diridx];
                if (isok(x, y)
                    && (t = t_at(x, y)) != 0
                    && is_pit(t->ttyp)) {
                    adjidx = (diridx + 4) % 8;
                    t->conjoined &= ~(1 << adjidx);
                }
                trap->conjoined &= ~(1 << diridx);
            }
        }
    }
}

STATIC_OVL boolean
adj_nonconjoined_pit(adjtrap)
struct trap *adjtrap;
{
    struct trap *trap_with_u = t_at(u.ux0, u.uy0);

    if (trap_with_u && adjtrap && u.utrap && u.utraptype == TT_PIT
        && is_pit(trap_with_u->ttyp) && is_pit(adjtrap->ttyp)) {
        int idx;

        for (idx = 0; idx < 8; idx++) {
            if (xdir[idx] == u.dx && ydir[idx] == u.dy)
                return TRUE;
        }
    }
    return FALSE;
}

#if 0
/*
 * Mark all neighboring pits as conjoined pits.
 * (currently not called from anywhere)
 */
STATIC_OVL void
join_adjacent_pits(trap)
struct trap *trap;
{
    struct trap *t;
    int diridx, x, y;

    if (!trap)
        return;
    for (diridx = 0; diridx < 8; ++diridx) {
        x = trap->tx + xdir[diridx];
        y = trap->ty + ydir[diridx];
        if (isok(x, y)) {
            if ((t = t_at(x, y)) != 0 && is_pit(t->ttyp)) {
                trap->conjoined |= (1 << diridx);
                join_adjacent_pits(t);
            } else
                trap->conjoined &= ~(1 << diridx);
        }
    }
}
#endif /*0*/

/*
 * Returns TRUE if you escaped a pit and are standing on the precipice.
 */
boolean
uteetering_at_seen_pit(trap)
struct trap *trap;
{
    return (trap && is_pit(trap->ttyp) && trap->tseen
            && trap->tx == u.ux && trap->ty == u.uy
            && !(u.utrap && u.utraptype == TT_PIT));
}

/*
 * Returns TRUE if you didn't fall through a hole or didn't
 * release a trap door
 */
boolean
uescaped_shaft(trap)
struct trap *trap;
{
    return (trap && is_hole(trap->ttyp) && trap->tseen
            && trap->tx == u.ux && trap->ty == u.uy);
}

/* Destroy a trap that emanates from the floor. */
boolean
delfloortrap(ttmp)
register struct trap *ttmp;
{
    /* some of these are arbitrary -dlc */
    if (ttmp && ((ttmp->ttyp == SQKY_BOARD) || (ttmp->ttyp == BEAR_TRAP)
                 || (ttmp->ttyp == LANDMINE) || (ttmp->ttyp == FIRE_TRAP)
                 || is_pit(ttmp->ttyp)
                 || is_hole(ttmp->ttyp)
                 || (ttmp->ttyp == TELEP_TRAP) || (ttmp->ttyp == LEVEL_TELEP)
                 || (ttmp->ttyp == WEB) || (ttmp->ttyp == MAGIC_TRAP)
                 || (ttmp->ttyp == ANTI_MAGIC))) {
        register struct monst *mtmp;

        if (ttmp->tx == u.ux && ttmp->ty == u.uy) {
            if (u.utraptype != TT_BURIEDBALL)
                reset_utrap(TRUE);
        } else if ((mtmp = m_at(ttmp->tx, ttmp->ty)) != 0) {
            mtmp->mtrapped = 0;
        }
        deltrap(ttmp);
        return TRUE;
    }
    return FALSE;
}

/* used for doors (also tins).  can be used for anything else that opens. */
void
b_trapped(item, bodypart)
const char *item;
int bodypart;
{
    int lvl = level_difficulty(),
        dmg = rnd(5 + (lvl < 5 ? lvl : 2 + lvl / 2));

#if 0 /*KR: 원본*/
    pline("KABOOM!!  %s was booby-trapped!", The(item));
#else /*KR: KRNethack 맞춤 번역 */
    pline("콰쾅!! %s 함정이 설치되어 있었다!",
          append_josa(The(item), "에는"));
#endif
    wake_nearby();
#if 0 /*KR: 원본*/
    losehp(Maybe_Half_Phys(dmg), "explosion", KILLED_BY_AN);
#else /*KR: KRNethack 맞춤 번역 */
    losehp(Maybe_Half_Phys(dmg), "폭발", KILLED_BY);
#endif
    exercise(A_STR, FALSE);
    if (bodypart)
        exercise(A_CON, FALSE);
    make_stunned((HStun & TIMEOUT) + (long) dmg, TRUE);
} /* Monster is hit by trap. */ /* Note: doesn't work if both obj and
                                   d_override are null */
STATIC_OVL boolean
thitm(tlev, mon, obj, d_override, nocorpse)
int tlev;
struct monst *mon;
struct obj *obj;
int d_override;
boolean nocorpse;
{
    int strike;
    boolean trapkilled = FALSE;

    if (d_override)
        strike = 1;
    else if (obj)
        strike = (find_mac(mon) + tlev + obj->spe <= rnd(20));
    else
        strike = (find_mac(mon) + tlev <= rnd(20));

    /* Actually more accurate than thitu, which doesn't take     * obj->spe
     * into account.     */
    if (!strike) {
        if (obj && cansee(mon->mx, mon->my))
#if 0 /*KR: 원본*/
            pline("%s is almost hit by %s!", Monnam(mon), doname(obj));
#else /*KR: KRNethack 맞춤 번역 */
            pline("%s %s 거의 맞을 뻔했다!", append_josa(Monnam(mon), "은"),
                  append_josa(doname(obj), "에"));
#endif
    } else {
        int dam = 1;

        if (obj && cansee(mon->mx, mon->my))
#if 0 /*KR: 원본*/
            pline("%s is hit by %s!", Monnam(mon), doname(obj));
#else /*KR: KRNethack 맞춤 번역 */
            pline("%s %s 맞았다!", append_josa(Monnam(mon), "은"),
                  append_josa(doname(obj), "에"));
#endif
        if (d_override)
            dam = d_override;
        else if (obj) {
            dam = dmgval(obj, mon);
            if (dam < 1)
                dam = 1;
        }
        mon->mhp -= dam;
        if (DEADMONSTER(mon)) {
            int xx = mon->mx, yy = mon->my;

            monkilled(mon, "", nocorpse ? -AD_RBRE : AD_PHYS);
            if (DEADMONSTER(mon)) {
                newsym(xx, yy);
                trapkilled = TRUE;
            }
        }
    }
    if (obj && (!strike || d_override)) {
        place_object(obj, mon->mx, mon->my);
        stackobj(obj);
    } else if (obj)
        dealloc_obj(obj);

    return trapkilled;
}

boolean
unconscious()
{
    if (multi >= 0)
        return FALSE;

    return (boolean) (u.usleep
                      || (nomovemsg
                          && (!strncmp(nomovemsg, "You awake", 9)
                              || !strncmp(nomovemsg, "You regain con", 14)
                              || !strncmp(nomovemsg, "You are consci", 14))));
}
/*KR static const char lava_killer[] = "molten lava"; */
static const char lava_killer[] = "녹은 용암";

boolean
lava_effects()
{
    register struct obj *obj, *obj2;
    int dmg = d(6, 6); /* only applicable for water walking */
    boolean usurvive, boil_away;

    feel_newsym(u.ux, u.uy); /* in case Blind, map the lava here */
    burn_away_slime();
    if (likes_lava(youmonst.data))
        return FALSE;

    usurvive = Fire_resistance || (Wwalking && dmg < u.uhp);
    /* * A timely interrupt might manage to salvage your life     * but not
     * your gear.  For scrolls and potions this     * will destroy whole
     * stacks, where fire resistant hero     * survivor only loses partial
     * stacks via destroy_item().     * * Flag items to be destroyed before
     * any messages so     * that player causing hangup at --More-- won't get
     * an     * emergency save file created before item destruction.     */
    if (!usurvive)
        for (obj = invent; obj; obj = obj->nobj)
            if ((is_organic(obj) || obj->oclass == POTION_CLASS)
                && !obj->oerodeproof
                && objects[obj->otyp].oc_oprop != FIRE_RES
                && obj->otyp != SCR_FIRE && obj->otyp != SPE_FIREBALL
                && !obj_resists(obj, 0, 0)) /* for invocation items */
                obj->in_use = 1;

    /* Check whether we should burn away boots *first* so we know whether to
     * * make the player sink into the lava. Assumption: water walking only
     * * comes from boots.     */
    if (uarmf && is_organic(uarmf) && !uarmf->oerodeproof) {
        obj = uarmf;
#if 0 /*KR: 원본*/
        pline("%s into flame!", Yobjnam2(obj, "burst"));
#else /*KR: KRNethack 맞춤 번역 */
        pline("%s 불길에 휩싸였다!", append_josa(Yname2(obj), "이"));
#endif
        iflags.in_lava_effects++; /* (see above) */
        (void) Boots_off();
        useup(obj);
        iflags.in_lava_effects--;
    }

    if (!Fire_resistance) {
        if (Wwalking) {
#if 0 /*KR: 원본*/
            pline_The("%s here burns you!", hliquid("lava"));
#else /*KR: KRNethack 맞춤 번역 */
            pline("이곳의 %s 당신을 불태웠다!",
                  append_josa(hliquid("용암"), "이"));
#endif
            if (usurvive) {
                losehp(dmg, lava_killer, KILLED_BY); /* lava damage */
                goto burn_stuff;
            }
        } else
#if 0 /*KR: 원본*/
            You("fall into the %s!", hliquid("lava"));
#else /*KR: KRNethack 맞춤 번역 */
            You("%s 안으로 떨어졌다!", hliquid("용암"));
#endif

        usurvive = Lifesaved || discover;
        if (wizard)
            usurvive = TRUE;

        /* prevent remove_worn_item() -> Boots_off(WATER_WALKING_BOOTS) ->
         * spoteffects() -> lava_effects() recursion which would successfully
         * delete (via useupall) the no-longer-worn boots;           once
         * recursive call returned, we would try to delete them again here in
         * the outer call (and access stale memory, probably panic) */
        iflags.in_lava_effects++;

        for (obj = invent; obj; obj = obj2) {
            obj2 = obj->nobj;
            /* above, we set in_use for objects which are to be destroyed */
            if (obj->otyp == SPE_BOOK_OF_THE_DEAD && !Blind) {
                if (usurvive)
#if 0 /*KR: 원본*/
                    pline("%s glows a strange %s, but remains intact.",
                          The(xname(obj)), hcolor("dark red"));
#else /*KR: KRNethack 맞춤 번역 */
                    pline("%s 이상한 %s 빛을 냈지만, 온전하게 남았다.",
                          append_josa(The(xname(obj)), "은"),
                          hcolor("검붉은"));
#endif
            } else if (obj->in_use) {
                if (obj->owornmask) {
                    if (usurvive)
#if 0 /*KR: 원본*/
                        pline("%s into flame!", Yobjnam2(obj, "burst"));
#else /*KR: KRNethack 맞춤 번역 */
                        pline("%s 불길에 휩싸였다!",
                              append_josa(Yname2(obj), "이"));
#endif
                    remove_worn_item(obj, TRUE);
                }
                useupall(obj);
            }
        }

        iflags.in_lava_effects--;

        /* s/he died... */
        boil_away =
            (u.umonnum == PM_WATER_ELEMENTAL || u.umonnum == PM_STEAM_VORTEX
             || u.umonnum == PM_FOG_CLOUD);
        for (;;) {
            u.uhp = -1;
            /* killer format and name are reconstructed every iteration
             * because lifesaving resets them */
            killer.format = KILLED_BY;
            Strcpy(killer.name, lava_killer);
#if 0 /*KR: 원본*/
            You("%s...", boil_away ? "boil away" : "burn to a crisp");
#else /*KR: KRNethack 맞춤 번역 */
            You("%s...",
                boil_away ? "끓어올라 증발했다" : "새카맣게 타버렸다");
#endif
            done(BURNING);
            if (safe_teleds(TRUE))
                break; /* successful life-save */
            /* nowhere safe to land; repeat burning loop */
            /*KR pline("You're still burning."); */
            pline("당신은 여전히 불타고 있다.");
        }
#if 0 /*KR: 원본*/
        You("find yourself back on solid %s.", surface(u.ux, u.uy));
#else /*KR: KRNethack 맞춤 번역 */
        You("단단한 %s 위로 돌아와 있는 것을 깨달았다.", surface(u.ux, u.uy));
#endif
        return TRUE;
    } else if (!Wwalking && (!u.utrap || u.utraptype != TT_LAVA)) {
        boil_away = !Fire_resistance;
        /* if not fire resistant, sink_into_lava() will quickly be fatal; hero
         * needs to escape immediately */
        set_utrap(
            (unsigned) (rn1(4, 4) + ((boil_away ? 2 : rn1(4, 12)) << 8)),
            TT_LAVA);
#if 0 /*KR: 원본*/
        You("sink into the %s%s!", hliquid("lava"),
            !boil_away ? ", but it only burns slightly"
                       : " and are about to be immolated");
#else /*KR: KRNethack 맞춤 번역 */
        You("%s 속으로 가라앉았다! %s", hliquid("용암"),
            !boil_away ? "하지만 약간 화상을 입었을 뿐이다"
                       : "그리고 산채로 불태워지려 하고 있다");
#endif
        if (u.uhp > 1)
            losehp(!boil_away ? 1 : (u.uhp / 2), lava_killer,
                   KILLED_BY); /* lava damage */
    }

burn_stuff:
    destroy_item(SCROLL_CLASS, AD_FIRE);
    destroy_item(SPBOOK_CLASS, AD_FIRE);
    destroy_item(POTION_CLASS, AD_FIRE);
    return FALSE;
} /* called each turn when trapped in lava */
void
sink_into_lava()
{
    /*KR static const char sink_deeper[] = "You sink deeper into the lava.";
     */
    static const char sink_deeper[] = "용암 속으로 더 깊이 가라앉았다.";

    if (!u.utrap || u.utraptype != TT_LAVA) {
        ; /* do nothing; this usually won't happen but could after
           * polymorphing from a flier into a ceiling hider and then hiding;
           * allmain() only checks whether the hero is at a lava location,
           * not whether he or she is currently sinking */
    } else if (!is_lava(u.ux, u.uy)) {
        reset_utrap(FALSE); /* this shouldn't happen either */
    } else if (!u.uinvulnerable) {
        /* ordinarily we'd have to be fire resistant to survive long
           enough to become stuck in lava, but it can happen without
           resistance if water walking boots allow survival and then
           get burned up; u.utrap time will be quite short in that case */
        if (!Fire_resistance)
            u.uhp = (u.uhp + 2) / 3;

        u.utrap -= (1 << 8);
        if (u.utrap < (1 << 8)) {
            killer.format = KILLED_BY;
            /*KR Strcpy(killer.name, "molten lava"); */
            Strcpy(killer.name, "녹은 용암");
            /*KR You("sink below the surface and die."); */
            You("표면 아래로 가라앉아 죽었다.");
            burn_away_slime(); /* add insult to injury? */
            done(DISSOLVED);
            /* can only get here via life-saving; try to get away from lava */
            reset_utrap(TRUE);
            /* levitation or flight have become unblocked, otherwise Tport */
            if (!Levitation && !Flying)
                (void) safe_teleds(TRUE);
        } else if (!u.umoved) {
            /* can't fully turn into slime while in lava, but might not
               have it be burned away until you've come awfully close */
            if (Slimed && rnd(10 - 1) >= (int) (Slimed & TIMEOUT)) {
                pline(sink_deeper);
                burn_away_slime();
            } else {
                Norep(sink_deeper);
            }
            u.utrap += rnd(4);
        }
    }
}

/* called when something has been done (breaking a boulder, for instance)
   which entails a luck penalty if performed on a sokoban level */
void
sokoban_guilt()
{
    if (Sokoban) {
        change_luck(-1);
        /* TODO: issue some feedback so that player can learn that whatever
           he/she just did is a naughty thing to do in sokoban and should
           probably be avoided in future....
           Caveat: doing this might introduce message sequencing issues,
           depending upon feedback during the various actions which trigger
           Sokoban luck penalties. */
    }
}

/* called when a trap has been deleted or had its ttyp replaced */
STATIC_OVL void
maybe_finish_sokoban()
{
    struct trap *t;

    if (Sokoban && !in_mklev) {
        /* scan all remaining traps, ignoring any created by the hero;
           if this level has no more pits or holes, the current sokoban
           puzzle has been solved */
        for (t = ftrap; t; t = t->ntrap) {
            if (t->madeby_u)
                continue;
            if (t->ttyp == PIT || t->ttyp == HOLE)
                break;
        }
        if (!t) {
            /* we've passed the last trap without finding a pit or hole;
               clear the sokoban_rules flag so that luck penalties for
               things like breaking boulders or jumping will no longer
               be given, and restrictions on diagonal moves are lifted */
            Sokoban = 0; /* clear level.flags.sokoban_rules */
            /* TODO: give some feedback about solving the sokoban puzzle
               (perhaps say "congratulations" in Japanese?) */
        }
    }
}

/*trap.c*/
