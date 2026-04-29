/* NetHack 3.6	lock.c	$NHDT-Date: 1548978605 2019/01/31 23:50:05 $  $NHDT-Branch: NetHack-3.6.2-beta01 $:$NHDT-Revision: 1.84 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2011. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

/* at most one of `door' and `box' should be non-null at any given time */
STATIC_VAR NEARDATA struct xlock_s {
    struct rm *door;
    struct obj *box;
    int picktyp, /* key|pick|card for unlock, sharp vs blunt for #force */
        chance, usedtime;
    boolean magic_key;
} xlock;

/* occupation callbacks */
STATIC_PTR int NDECL(picklock);
STATIC_PTR int NDECL(forcelock);

STATIC_DCL const char *NDECL(lock_action);
STATIC_DCL boolean FDECL(obstructed, (int, int, BOOLEAN_P));
STATIC_DCL void FDECL(chest_shatter_msg, (struct obj *) );

boolean
picking_lock(x, y)
int *x, *y;
{
    if (occupation == picklock) {
        *x = u.ux + u.dx;
        *y = u.uy + u.dy;
        return TRUE;
    } else {
        *x = *y = 0;
        return FALSE;
    }
}

boolean
picking_at(x, y)
int x, y;
{
    return (boolean) (occupation == picklock && xlock.door == &levl[x][y]);
}

/* produce an occupation string appropriate for the current activity */
STATIC_OVL const char *
lock_action()
{
    /* "unlocking"+2 == "locking" */
    static const char *actions[] = {
#if 0 /*KR: 원본*/
        "unlocking the door",   /* [0] */
        "unlocking the chest",  /* [1] */
        "unlocking the box",    /* [2] */
        "picking the lock"      /* [3] */
#else /*KR: KRNethack 맞춤 번역 */
        "문의 잠금을 해제하는", "큰 궤짝의 잠금을 해제하는",
        "상자의 잠금을 해제하는", "자물쇠를 따는"
#endif
    };

    /* if the target is currently unlocked, we're trying to lock it now */
    if (xlock.door && !(xlock.door->doormask & D_LOCKED))
#if 0 /*KR: 원본*/
        return actions[0] + 2; /* "locking the door" */
#else /*KR: KRNethack 맞춤 번역 */
        /* 영어는 un 을 떼면 역의 의미가 되지만, 한국어는 다르므로 리터럴로
         * 반환 */
        return "문을 잠그는";
#endif
    else if (xlock.box && !xlock.box->olocked)
#if 0 /*KR: 원본*/
        return xlock.box->otyp == CHEST ? actions[1] + 2 : actions[2] + 2;
#else /*KR: KRNethack 맞춤 번역 */
        return xlock.box->otyp == CHEST ? "큰 궤짝을 잠그는"
                                        : "상자를 잠그는";
#endif
    /* otherwise we're trying to unlock it */
    else if (xlock.picktyp == LOCK_PICK)
        return actions[3]; /* "picking the lock" */
    else if (xlock.picktyp == CREDIT_CARD)
        return actions[3]; /* same as lock_pick */
    else if (xlock.door)
        return actions[0]; /* "unlocking the door" */
    else if (xlock.box)
        return xlock.box->otyp == CHEST ? actions[1] : actions[2];
    else
        return actions[3];
}

/* try to open/close a lock */
STATIC_PTR int
picklock(VOID_ARGS)
{
    if (xlock.box) {
        if (xlock.box->where != OBJ_FLOOR || xlock.box->ox != u.ux
            || xlock.box->oy != u.uy) {
            return ((xlock.usedtime = 0)); /* you or it moved */
        }
    } else { /* door */
        if (xlock.door != &(levl[u.ux + u.dx][u.uy + u.dy])) {
            return ((xlock.usedtime = 0)); /* you moved */
        }
        switch (xlock.door->doormask) {
        case D_NODOOR:
            /*KR pline("This doorway has no door."); */
            pline("이 통로에는 문이 없다.");
            return ((xlock.usedtime = 0));
        case D_ISOPEN:
            /*KR You("cannot lock an open door."); */
            You("열려 있는 문을 잠글 수는 없다.");
            return ((xlock.usedtime = 0));
        case D_BROKEN:
            /*KR pline("This door is broken."); */
            pline("이 문은 부서져 있다.");
            return ((xlock.usedtime = 0));
        }
    }

    if (xlock.usedtime++ >= 50 || nohands(youmonst.data)) {
        /*KR You("give up your attempt at %s.", lock_action()); */
        You("%s 것을 포기했다.", lock_action());
        exercise(A_DEX, TRUE); /* even if you don't succeed */
        return ((xlock.usedtime = 0));
    }

    if (rn2(100) >= xlock.chance)
        return 1; /* still busy */

    /* using the Master Key of Thievery finds traps if its bless/curse
       state is adequate (non-cursed for rogues, blessed for others;
       checked when setting up 'xlock') */
    if ((!xlock.door ? (int) xlock.box->otrapped
                     : (xlock.door->doormask & D_TRAPPED) != 0)
        && xlock.magic_key) {
        xlock.chance += 20; /* less effort needed next time */
        /* unfortunately we don't have a 'tknown' flag to record
           "known to be trapped" so declining to disarm and then
           retrying lock manipulation will find it all over again */
        /*KR if (yn("You find a trap!  Do you want to try to disarm it?") ==
         * 'y') { */
        if (yn("함정을 발견했다! 해제하겠습니까?") == 'y') {
            const char *what;
            boolean alreadyunlocked;

            /* disarming while using magic key always succeeds */
            if (xlock.door) {
                xlock.door->doormask &= ~D_TRAPPED;
                /*KR what = "door"; */
                what = "문";
                alreadyunlocked = !(xlock.door->doormask & D_LOCKED);
            } else {
                xlock.box->otrapped = 0;
                /*KR what = (xlock.box->otyp == CHEST) ? "chest" : "box"; */
                what = (xlock.box->otyp == CHEST) ? "큰 궤짝" : "상자";
                alreadyunlocked = !xlock.box->olocked;
            }
#if 0 /*KR: 원본*/
            You("succeed in disarming the trap.  The %s is still %slocked.",
                what, alreadyunlocked ? "un" : "");
#else /*KR: KRNethack 맞춤 번역 */
            You("함정 해제에 성공했다. %s 여전히 잠겨%s.",
                append_josa(what, "은"),
                alreadyunlocked ? " 있지 않다" : " 있다");
#endif
            exercise(A_WIS, TRUE);
        } else {
            /*KR You("stop %s.", lock_action()); */
            You("%s 것을 멈췄다.", lock_action());
            exercise(A_WIS, FALSE);
        }
        return ((xlock.usedtime = 0));
    }

    /*KR You("succeed in %s.", lock_action()); */
    You("%s 것에 성공했다.", lock_action());
    if (xlock.door) {
        if (xlock.door->doormask & D_TRAPPED) {
            /*KR b_trapped("door", FINGER); */
            b_trapped("문", FINGER);
            xlock.door->doormask = D_NODOOR;
            unblock_point(u.ux + u.dx, u.uy + u.dy);
            if (*in_rooms(u.ux + u.dx, u.uy + u.dy, SHOPBASE))
                add_damage(u.ux + u.dx, u.uy + u.dy, SHOP_DOOR_COST);
            newsym(u.ux + u.dx, u.uy + u.dy);
        } else if (xlock.door->doormask & D_LOCKED)
            xlock.door->doormask = D_CLOSED;
        else
            xlock.door->doormask = D_LOCKED;
    } else {
        xlock.box->olocked = !xlock.box->olocked;
        xlock.box->lknown = 1;
        if (xlock.box->otrapped)
            (void) chest_trap(xlock.box, FINGER, FALSE);
    }
    exercise(A_DEX, TRUE);
    return ((xlock.usedtime = 0));
}

void breakchestlock(box, destroyit) struct obj *box;
boolean destroyit;
{
    if (!destroyit) { /* bill for the box but not for its contents */
        struct obj *hide_contents = box->cobj;

        box->cobj = 0;
        costly_alteration(box, COST_BRKLCK);
        box->cobj = hide_contents;
        box->olocked = 0;
        box->obroken = 1;
        box->lknown = 1;
    } else { /* #force has destroyed this box (at <u.ux,u.uy>) */
        struct obj *otmp;
        struct monst *shkp = (*u.ushops && costly_spot(u.ux, u.uy))
                                 ? shop_keeper(*u.ushops)
                                 : 0;
        boolean costly = (boolean) (shkp != 0),
                peaceful_shk = costly && (boolean) shkp->mpeaceful;
        long loss = 0L;

        /*KR pline("In fact, you've totally destroyed %s.", the(xname(box)));
         */
        pline("아뿔싸, 실수로 당신은 %s 완전히 파괴하고 말았다.",
              append_josa(the(xname(box)), "을"));
        /* Put the contents on ground at the hero's feet. */
        while ((otmp = box->cobj) != 0) {
            obj_extract_self(otmp);
            if (!rn2(3) || otmp->oclass == POTION_CLASS) {
                chest_shatter_msg(otmp);
                if (costly)
                    loss +=
                        stolen_value(otmp, u.ux, u.uy, peaceful_shk, TRUE);
                if (otmp->quan == 1L) {
                    obfree(otmp, (struct obj *) 0);
                    continue;
                }
                /* this works because we're sure to have at least 1 left;
                   otherwise it would fail since otmp is not in inventory */
                useup(otmp);
            }
            if (box->otyp == ICE_BOX && otmp->otyp == CORPSE) {
                otmp->age = monstermoves - otmp->age; /* actual age */
                start_corpse_timeout(otmp);
            }
            place_object(otmp, u.ux, u.uy);
            stackobj(otmp);
        }
        if (costly)
            loss += stolen_value(box, u.ux, u.uy, peaceful_shk, TRUE);
        if (loss)
            /*KR You("owe %ld %s for objects destroyed.", loss,
             * currency(loss)); */
            You("물건을 파괴한 대가로 %ld %s의 빚을 졌다.", loss,
                currency(loss));
        delobj(box);
    }
}

/* try to force a locked chest */
STATIC_PTR int
forcelock(VOID_ARGS)
{
    if ((xlock.box->ox != u.ux) || (xlock.box->oy != u.uy))
        return ((xlock.usedtime = 0)); /* you or it moved */

    if (xlock.usedtime++ >= 50 || !uwep || nohands(youmonst.data)) {
        /*KR You("give up your attempt to force the lock."); */
        You("자물쇠를 힘으로 열려는 시도를 포기했다.");
        if (xlock.usedtime >= 50) /* you made the effort */
            exercise((xlock.picktyp) ? A_DEX : A_STR, TRUE);
        return ((xlock.usedtime = 0));
    }

    if (xlock.picktyp) { /* blade */
        if (rn2(1000 - (int) uwep->spe) > (992 - greatest_erosion(uwep) * 10)
            && !uwep->cursed && !obj_resists(uwep, 0, 99)) {
            /* for a +0 weapon, probability that it survives an unsuccessful
             * attempt to force the lock is (.992)^50 = .67
             */
#if 0 /*KR: 원본*/
            pline("%sour %s broke!", (uwep->quan > 1L) ? "One of y" : "Y",
                  xname(uwep));
#else /*KR: KRNethack 맞춤 번역 */
            pline("당신의 %s 부서졌다!", append_josa(xname(uwep), "가"));
#endif
            useup(uwep);
            /*KR You("give up your attempt to force the lock."); */
            You("자물쇠를 힘으로 열려는 시도를 포기했다.");
            exercise(A_DEX, TRUE);
            return ((xlock.usedtime = 0));
        }
    } else             /* blunt */
        wake_nearby(); /* due to hammering on the container */

    if (rn2(100) >= xlock.chance)
        return 1; /* still busy */

    /*KR You("succeed in forcing the lock."); */
    You("자물쇠를 힘으로 여는 데 성공했다.");
    exercise(xlock.picktyp ? A_DEX : A_STR, TRUE);
    /* breakchestlock() might destroy xlock.box; if so, xlock context will
       be cleared (delobj -> obfree -> maybe_reset_pick); but it might not,
       so explicitly clear that manually */
    breakchestlock(xlock.box, (boolean) (!xlock.picktyp && !rn2(3)));
    reset_pick(); /* lock-picking context is no longer valid */

    return 0;
}

void
reset_pick()
{
    xlock.usedtime = xlock.chance = xlock.picktyp = 0;
    xlock.magic_key = FALSE;
    xlock.door = (struct rm *) 0;
    xlock.box = (struct obj *) 0;
}

/* level change or object deletion; context may no longer be valid */
void maybe_reset_pick(
    container) struct obj *container; /* passed from obfree() */
{
    /*
     * If a specific container, only clear context if it is for that
     * particular container (which is being deleted).  Other stuff on
     * the current dungeon level remains valid.
     * However if 'container' is Null, clear context if not carrying
     * xlock.box (which might be Null if context is for a door).
     * Used for changing levels, where a floor container or a door is
     * being left behind and won't be valid on the new level but a
     * carried container will still be.  There might not be any context,
     * in which case redundantly clearing it is harmless.
     */
    if (container ? (container == xlock.box)
                  : (!xlock.box || !carried(xlock.box)))
        reset_pick();
}

/* for doapply(); if player gives a direction or resumes an interrupted
   previous attempt then it costs hero a move even if nothing ultimately
   happens; when told "can't do that" before being asked for direction
   or player cancels with ESC while giving direction, it doesn't */
#define PICKLOCK_LEARNED_SOMETHING (-1) /* time passes */
#define PICKLOCK_DID_NOTHING 0          /* no time passes */
#define PICKLOCK_DID_SOMETHING 1

/* player is applying a key, lock pick, or credit card */
int
pick_lock(pick)
struct obj *pick;
{
    int picktyp, c, ch;
    coord cc;
    struct rm *door;
    struct obj *otmp;
    char qbuf[QBUFSZ];

    picktyp = pick->otyp;

    /* check whether we're resuming an interrupted previous attempt */
    if (xlock.usedtime && picktyp == xlock.picktyp) {
        /*KR static char no_longer[] = "Unfortunately, you can no longer %s
         * %s."; */
        static char no_longer[] = "안타깝게도, 당신은 더 이상 %s %s.";

        if (nohands(youmonst.data)) {
            /*KR const char *what = (picktyp == LOCK_PICK) ? "pick" : "key";
             */
            const char *what =
                (picktyp == LOCK_PICK) ? "만능 열쇠를" : "열쇠를";

            if (picktyp == CREDIT_CARD)
                /*KR what = "card"; */
                what = "카드를";
            /*KR pline(no_longer, "hold the", what); */
            pline(no_longer, what, "쥘 수 없다");
            reset_pick();
            return PICKLOCK_LEARNED_SOMETHING;
        } else if (u.uswallow || (xlock.box && !can_reach_floor(TRUE))) {
            /*KR pline(no_longer, "reach the", "lock"); */
            pline(no_longer, "자물쇠에", "닿지 않는다");
            reset_pick();
            return PICKLOCK_LEARNED_SOMETHING;
        } else {
            const char *action = lock_action();

            /*KR You("resume your attempt at %s.", action); */
            You("%s 시도를 재개했다.", action);
            xlock.magic_key = is_magic_key(&youmonst, pick);
            set_occupation(picklock, action, 0);
            return PICKLOCK_DID_SOMETHING;
        }
    }

    if (nohands(youmonst.data)) {
        /*KR You_cant("hold %s -- you have no hands!", doname(pick)); */
        You_cant("%s 쥘 수 없다 -- 손이 없으니까!",
                 append_josa(doname(pick), "을"));
        return PICKLOCK_DID_NOTHING;
    } else if (u.uswallow) {
#if 0 /*KR: 원본*/
        You_cant("%sunlock %s.", (picktyp == CREDIT_CARD) ? "" : "lock or ",
                 mon_nam(u.ustuck));
#else /*KR: KRNethack 맞춤 번역 */
        You_cant("%s %s 없다.", append_josa(mon_nam(u.ustuck), "을"),
                 (picktyp == CREDIT_CARD) ? "잠금 해제할 수"
                                          : "잠그거나 해제할 수");
#endif
        return PICKLOCK_DID_NOTHING;
    }

    if (picktyp != LOCK_PICK && picktyp != CREDIT_CARD
        && picktyp != SKELETON_KEY) {
        impossible("picking lock with object %d?", picktyp);
        return PICKLOCK_DID_NOTHING;
    }
    ch = 0; /* lint suppression */

    /*KR if (!get_adjacent_loc((char *) 0, "Invalid location!", u.ux, u.uy,
     * &cc)) */
    if (!get_adjacent_loc((char *) 0, "유효하지 않은 위치다!", u.ux, u.uy,
                          &cc))
        return PICKLOCK_DID_NOTHING;

    if (cc.x == u.ux && cc.y == u.uy) { /* pick lock on a container */
        const char *verb;
        char qsfx[QBUFSZ];
#if 0 /*KR: 원본*/
        boolean it;
#endif
        int count;

        if (u.dz < 0) {
#if 0 /*KR: 원본*/
            There("isn't any sort of lock up %s.",
                  Levitation ? "here" : "there");
#else /*KR: KRNethack 맞춤 번역 */
            pline("%s에는 그 어떤 종류의 자물쇠도 없다.",
                  Levitation ? "이곳" : "그 위");
#endif
            return PICKLOCK_LEARNED_SOMETHING;
        } else if (is_lava(u.ux, u.uy)) {
            /*KR pline("Doing that would probably melt %s.", yname(pick)); */
            pline("그러면 아마도 %s 녹아버릴 것이다.",
                  append_josa(yname(pick), "가"));
            return PICKLOCK_LEARNED_SOMETHING;
        } else if (is_pool(u.ux, u.uy) && !Underwater) {
            /*KR pline_The("%s has no lock.", hliquid("water")); */
            pline("%s에는 자물쇠가 없다.", append_josa(hliquid("물"), "에"));
            return PICKLOCK_LEARNED_SOMETHING;
        }

        count = 0;
        c = 'n'; /* in case there are no boxes here */
        for (otmp = level.objects[cc.x][cc.y]; otmp; otmp = otmp->nexthere)
            if (Is_box(otmp)) {
                ++count;
                if (!can_reach_floor(TRUE)) {
                    /*KR You_cant("reach %s from up here.", the(xname(otmp)));
                     */
                    You_cant("이 위에서는 %s 닿지 않는다.",
                             append_josa(the(xname(otmp)), "에"));
                    return PICKLOCK_LEARNED_SOMETHING;
                }
#if 0 /*KR*/
                it = 0;
#endif
                if (otmp->obroken)
                    /*KR verb = "fix"; */
                    verb = "고치겠습니까";
                else if (!otmp->olocked)
                    /*KR verb = "lock", it = 1; */
                    verb = "잠그겠습니까";
                else if (picktyp != LOCK_PICK)
                    /*KR verb = "unlock", it = 1; */
                    verb = "잠금 해제하겠습니까";
                else
                    /*KR verb = "pick"; */
                    verb = "자물쇠를 따겠습니까";

                /* "There is <a box> here; <verb> <it|its lock>?" */
                /*KR Sprintf(qsfx, " here; %s %s?", verb, it ? "it" : "its
                 * lock"); */
                Sprintf(qsfx, " 있습니다. %s?", verb);
#if 0 /*KR: 원본*/
                (void) safe_qbuf(qbuf, "There is ", qsfx, otmp, doname,
                                 ansimpleoname, "a box");
#else /*KR: KRNethack 맞춤 번역 */
                (void) safe_qbuf(qbuf, "이곳에 ", qsfx, otmp, doname,
                                 ansimpleoname, "상자가");
#endif
                otmp->lknown = 1;

                c = ynq(qbuf);
                if (c == 'q')
                    return 0;
                if (c == 'n')
                    continue;

                if (otmp->obroken) {
                    /*KR You_cant("fix its broken lock with %s.",
                     * doname(pick)); */
                    You_cant("%s 부서진 자물쇠를 고칠 수 없다.",
                             append_josa(doname(pick), "으로"));
                    return PICKLOCK_LEARNED_SOMETHING;
                } else if (picktyp == CREDIT_CARD && !otmp->olocked) {
                /* credit cards are only good for unlocking */
#if 0 /*KR: 원본*/
                    You_cant("do that with %s.",
                             an(simple_typename(picktyp)));
#else /*KR: KRNethack 맞춤 번역 */
                    You_cant(
                        "%s는 그런 일은 할 수 없다.",
                        append_josa(simple_typename(picktyp), "으로"));
#endif
                    return PICKLOCK_LEARNED_SOMETHING;
                }
                switch (picktyp) {
                case CREDIT_CARD:
                    ch = ACURR(A_DEX) + 20 * Role_if(PM_ROGUE);
                    break;
                case LOCK_PICK:
                    ch = 4 * ACURR(A_DEX) + 25 * Role_if(PM_ROGUE);
                    break;
                case SKELETON_KEY:
                    ch = 75 + ACURR(A_DEX);
                    break;
                default:
                    ch = 0;
                }
                if (otmp->cursed)
                    ch /= 2;

                xlock.box = otmp;
                xlock.door = 0;
                break;
            }
        if (c != 'y') {
            if (!count)
                /*KR There("doesn't seem to be any sort of lock here."); */
                pline("여기에는 어떤 종류의 자물쇠도 없는 것 같다.");
            return PICKLOCK_LEARNED_SOMETHING; /* decided against all boxes */
        }
    } else { /* pick the lock in a door */
        struct monst *mtmp;

        if (u.utrap && u.utraptype == TT_PIT) {
            /*KR You_cant("reach over the edge of the pit."); */
            You_cant("구덩이 밖으로는 손이 닿지 않는다.");
            return PICKLOCK_LEARNED_SOMETHING;
        }

        door = &levl[cc.x][cc.y];
        mtmp = m_at(cc.x, cc.y);
        if (mtmp && canseemon(mtmp) && M_AP_TYPE(mtmp) != M_AP_FURNITURE
            && M_AP_TYPE(mtmp) != M_AP_OBJECT) {
            if (picktyp == CREDIT_CARD
                && (mtmp->isshk || mtmp->data == &mons[PM_ORACLE]))
                /*KR verbalize("No checks, no credit, no problem."); */
                verbalize("수표도 카드도 안 받습니다. 현금만 가능합니다.");
            else
#if 0 /*KR: 원본*/
                pline("I don't think %s would appreciate that.",
                      mon_nam(mtmp));
#else /*KR: KRNethack 맞춤 번역 */
                pline("%s 그것을 달가워하지 않을 것이다.",
                      append_josa(mon_nam(mtmp), "가"));
#endif
            return PICKLOCK_LEARNED_SOMETHING;
        } else if (mtmp && is_door_mappear(mtmp)) {
            /* "The door actually was a <mimic>!" */
            stumble_onto_mimic(mtmp);
            /* mimic might keep the key (50% chance, 10% for PYEC or MKoT) */
            maybe_absorb_item(mtmp, pick, 50, 10);
            return PICKLOCK_LEARNED_SOMETHING;
        }
        if (!IS_DOOR(door->typ)) {
            if (is_drawbridge_wall(cc.x, cc.y) >= 0)
                /*KR You("%s no lock on the drawbridge.", Blind ? "feel" :
                 * "see"); */
                You("도개교에는 자물쇠가 없는 것을 %s.",
                    Blind ? "알았다" : "보았다");
            else
                /*KR You("%s no door there.", Blind ? "feel" : "see"); */
                You("그곳에 문이 없는 것을 %s.", Blind ? "알았다" : "보았다");
            return PICKLOCK_LEARNED_SOMETHING;
        }
        switch (door->doormask) {
        case D_NODOOR:
            /*KR pline("This doorway has no door."); */
            pline("이 통로에는 문이 없다.");
            return PICKLOCK_LEARNED_SOMETHING;
        case D_ISOPEN:
            /*KR You("cannot lock an open door."); */
            You("열려 있는 문을 잠글 수는 없다.");
            return PICKLOCK_LEARNED_SOMETHING;
        case D_BROKEN:
            /*KR pline("This door is broken."); */
            pline("이 문은 부서져 있다.");
            return PICKLOCK_LEARNED_SOMETHING;
        default:
            /* credit cards are only good for unlocking */
            if (picktyp == CREDIT_CARD && !(door->doormask & D_LOCKED)) {
                /*KR You_cant("lock a door with a credit card."); */
                You_cant("신용카드로 문을 잠글 수는 없다.");
                return PICKLOCK_LEARNED_SOMETHING;
            }

#if 0 /*KR: 원본*/
            Sprintf(qbuf, "%s it?",
                    (door->doormask & D_LOCKED) ? "Unlock" : "Lock");
#else /*KR: KRNethack 맞춤 번역 */
            Sprintf(qbuf, "이것을 %s니까?",
                    (door->doormask & D_LOCKED) ? "잠금 해제하겠습"
                                                : "잠그겠습");
#endif

            c = yn(qbuf);
            if (c == 'n')
                return 0;

            switch (picktyp) {
            case CREDIT_CARD:
                ch = 2 * ACURR(A_DEX) + 20 * Role_if(PM_ROGUE);
                break;
            case LOCK_PICK:
                ch = 3 * ACURR(A_DEX) + 30 * Role_if(PM_ROGUE);
                break;
            case SKELETON_KEY:
                ch = 70 + ACURR(A_DEX);
                break;
            default:
                ch = 0;
            }
            xlock.door = door;
            xlock.box = 0;
        }
    }
    context.move = 0;
    xlock.chance = ch;
    xlock.picktyp = picktyp;
    xlock.magic_key = is_magic_key(&youmonst, pick);
    xlock.usedtime = 0;
    set_occupation(picklock, lock_action(), 0);
    return PICKLOCK_DID_SOMETHING;
}

/* try to force a chest with your weapon */
int
doforce()
{
    register struct obj *otmp;
    register int c, picktyp;
    char qbuf[QBUFSZ];

    if (u.uswallow) {
        /*KR You_cant("force anything from inside here."); */
        You_cant("이 안쪽에서는 억지로 무언가를 열 수 없다.");
        return 0;
    }
    if (!uwep /* proper type test */
        || ((uwep->oclass == WEAPON_CLASS || is_weptool(uwep))
                ? (objects[uwep->otyp].oc_skill < P_DAGGER
                   || objects[uwep->otyp].oc_skill == P_FLAIL
                   || objects[uwep->otyp].oc_skill > P_LANCE)
                : uwep->oclass != ROCK_CLASS)) {
#if 0 /*KR: 원본*/
        You_cant("force anything %s weapon.",
                 !uwep ? "when not wielding a"
                       : (uwep->oclass != WEAPON_CLASS && !is_weptool(uwep))
                             ? "without a proper"
                             : "with that");
#else /*KR: KRNethack 맞춤 번역 */
        You_cant("%s 억지로 무언가를 열 수는 없다.",
                 !uwep ? "무기를 들고 있지 않으면"
                 : (uwep->oclass != WEAPON_CLASS && !is_weptool(uwep))
                     ? "적절한 무기가 없으면"
                     : "그 무기로는");
#endif
        return 0;
    }
    if (!can_reach_floor(TRUE)) {
        cant_reach_floor(u.ux, u.uy, FALSE, TRUE);
        return 0;
    }

    picktyp = is_blade(uwep) && !is_pick(uwep);
    if (xlock.usedtime && xlock.box && picktyp == xlock.picktyp) {
        /*KR You("resume your attempt to force the lock."); */
        You("자물쇠를 힘으로 열려는 시도를 재개했다.");
        /*KR set_occupation(forcelock, "forcing the lock", 0); */
        set_occupation(forcelock, "억지로 자물쇠를 열기", 0);
        return 1;
    }

    /* A lock is made only for the honest man, the thief will break it. */
    xlock.box = (struct obj *) 0;
    for (otmp = level.objects[u.ux][u.uy]; otmp; otmp = otmp->nexthere)
        if (Is_box(otmp)) {
            if (otmp->obroken || !otmp->olocked) {
                /* force doname() to omit known "broken" or "unlocked"
                   prefix so that the message isn't worded redundantly;
                   since we're about to set lknown, there's no need to
                   remember and then reset its current value */
                otmp->lknown = 0;
#if 0 /*KR: 원본*/
                There("is %s here, but its lock is already %s.",
                      doname(otmp), otmp->obroken ? "broken" : "unlocked");
#else /*KR: KRNethack 맞춤 번역 */
                pline("이곳에 %s 있지만, 자물쇠는 이미 %s.",
                      append_josa(doname(otmp), "가"),
                      otmp->obroken ? "부서졌다" : "열려 있다");
#endif
                otmp->lknown = 1;
                continue;
            }
#if 0 /*KR: 원본*/
            (void) safe_qbuf(qbuf, "There is ", " here; force its lock?",
                             otmp, doname, ansimpleoname, "a box");
#else /*KR: KRNethack 맞춤 번역 */
            (void) safe_qbuf(qbuf, "이곳에 ",
                             " 있습니다. 억지로 자물쇠를 여시겠습니까?", otmp,
                             doname, ansimpleoname, "상자가");
#endif
            otmp->lknown = 1;

            c = ynq(qbuf);
            if (c == 'q')
                return 0;
            if (c == 'n')
                continue;

            if (picktyp)
                /*KR You("force %s into a crack and pry.", yname(uwep)); */
                You("%s 틈새에 밀어넣고 억지로 비틀었다.",
                    append_josa(yname(uwep), "을"));
            else
                /*KR You("start bashing it with %s.", yname(uwep)); */
                You("%s(으)로 그것을 내려치기 시작했다.", yname(uwep));
            xlock.box = otmp;
            xlock.chance = objects[uwep->otyp].oc_wldam * 2;
            xlock.picktyp = picktyp;
            xlock.magic_key = FALSE;
            xlock.usedtime = 0;
            break;
        }

    if (xlock.box)
        /*KR set_occupation(forcelock, "forcing the lock", 0); */
        set_occupation(forcelock, "자물쇠를 억지로 열기", 0);
    else
        /*KR You("decide not to force the issue."); */
        You("억지로 여는 것을 관두기로 했다.");
    return 1;
}

boolean
stumble_on_door_mimic(x, y)
int x, y;
{
    struct monst *mtmp;

    if ((mtmp = m_at(x, y)) && is_door_mappear(mtmp)
        && !Protection_from_shape_changers) {
        stumble_onto_mimic(mtmp);
        return TRUE;
    }
    return FALSE;
}

/* the 'O' command - try to open a door */
int
doopen()
{
    return doopen_indir(0, 0);
}

/* try to open a door in direction u.dx/u.dy */
int
doopen_indir(x, y)
int x, y;
{
    coord cc;
    register struct rm *door;
    boolean portcullis;
    int res = 0;

    if (nohands(youmonst.data)) {
        /*KR You_cant("open anything -- you have no hands!"); */
        You_cant("아무것도 열 수 없다 -- 손이 없으니까!");
        return 0;
    }

    if (u.utrap && u.utraptype == TT_PIT) {
        /*KR You_cant("reach over the edge of the pit."); */
        You_cant("구덩이 밖으로는 손이 닿지 않는다.");
        return 0;
    }

    if (x > 0 && y > 0) {
        cc.x = x;
        cc.y = y;
    } else if (!get_adjacent_loc((char *) 0, (char *) 0, u.ux, u.uy, &cc))
        return 0;

    /* open at yourself/up/down */
    if ((cc.x == u.ux) && (cc.y == u.uy))
        return doloot();

    if (stumble_on_door_mimic(cc.x, cc.y))
        return 1;

    /* when choosing a direction is impaired, use a turn
       regardless of whether a door is successfully targetted */
    if (Confusion || Stunned)
        res = 1;

    door = &levl[cc.x][cc.y];
    portcullis = (is_drawbridge_wall(cc.x, cc.y) >= 0);
    if (Blind) {
        int oldglyph = door->glyph;
        schar oldlastseentyp = lastseentyp[cc.x][cc.y];

        feel_location(cc.x, cc.y);
        if (door->glyph != oldglyph
            || lastseentyp[cc.x][cc.y] != oldlastseentyp)
            res = 1; /* learned something */
    }

    if (portcullis || !IS_DOOR(door->typ)) {
        /* closed portcullis or spot that opened bridge would span */
        if (is_db_wall(cc.x, cc.y) || door->typ == DRAWBRIDGE_UP)
            /*KR There("is no obvious way to open the drawbridge."); */
            pline("도개교를 여는 확실한 방법은 없는 것 같다.");
        else if (portcullis || door->typ == DRAWBRIDGE_DOWN)
            /*KR pline_The("drawbridge is already open."); */
            pline("도개교는 이미 열려 있다.");
        else if (container_at(cc.x, cc.y, TRUE))
#if 0 /*KR: 원본*/
            pline("%s like something lootable over there.",
                  Blind ? "Feels" : "Seems");
#else /*KR: KRNethack 맞춤 번역 */
            pline("저기 %s 무언가 들어있을 법한 상자 같은 게 있다.",
                  Blind ? "만져보니" : "보아하니");
#endif
        else
            /*KR You("%s no door there.", Blind ? "feel" : "see"); */
            You("그곳에 문이 없는 것을 %s.", Blind ? "알았다" : "보았다");
        return res;
    }

    if (!(door->doormask & D_CLOSED)) {
        const char *mesg;

        switch (door->doormask) {
        case D_BROKEN:
            /*KR mesg = " is broken"; */
            mesg = " 부서져 있다";
            break;
        case D_NODOOR:
            /*KR mesg = "way has no door"; */
            mesg = "는 문이 없다";
            break;
        case D_ISOPEN:
            /*KR mesg = " is already open"; */
            mesg = " 이미 열려 있다";
            break;
        default:
            /*KR mesg = " is locked"; */
            mesg = " 잠겨 있다";
            break;
        }
        /*KR pline("This door%s.", mesg); */
        pline("이 문%s.", mesg);
        return res;
    }

    if (verysmall(youmonst.data)) {
        /*KR pline("You're too small to pull the door open."); */
        pline("당신은 너무 작아서 문을 당겨 열 수 없다.");
        return res;
    }

    /* door is known to be CLOSED */
    if (rnl(20) < (ACURRSTR + ACURR(A_DEX) + ACURR(A_CON)) / 3) {
        /*KR pline_The("door opens."); */
        pline("문이 열렸다.");
        if (door->doormask & D_TRAPPED) {
            /*KR b_trapped("door", FINGER); */
            b_trapped("문", FINGER);
            door->doormask = D_NODOOR;
            if (*in_rooms(cc.x, cc.y, SHOPBASE))
                add_damage(cc.x, cc.y, SHOP_DOOR_COST);
        } else
            door->doormask = D_ISOPEN;
        feel_newsym(cc.x, cc.y);   /* the hero knows she opened it */
        unblock_point(cc.x, cc.y); /* vision: new see through there */
    } else {
        exercise(A_STR, TRUE);
        /*KR pline_The("door resists!"); */
        pline("문이 꿈쩍도 하지 않는다!");
    }

    return 1;
}

STATIC_OVL boolean
obstructed(x, y, quietly)
register int x, y;
boolean quietly;
{
    register struct monst *mtmp = m_at(x, y);

    if (mtmp && M_AP_TYPE(mtmp) != M_AP_FURNITURE) {
        if (M_AP_TYPE(mtmp) == M_AP_OBJECT)
            goto objhere;
        if (!quietly) {
            if ((mtmp->mx != x) || (mtmp->my != y)) {
                /* worm tail */
#if 0 /*KR: 원본*/
                pline("%s%s blocks the way!",
                      !canspotmon(mtmp) ? Something : s_suffix(Monnam(mtmp)),
                      !canspotmon(mtmp) ? "" : " tail");
#else /*KR: KRNethack 맞춤 번역 */
                pline("%s%s 길을 막고 있다!",
                      !canspotmon(mtmp) ? "무언가"
                                        : append_josa(Monnam(mtmp), "의"),
                      !canspotmon(mtmp) ? "가" : " 꼬리가");
#endif
            } else {
#if 0 /*KR: 원본*/
                pline("%s blocks the way!",
                      !canspotmon(mtmp) ? "Some creature" : Monnam(mtmp));
#else /*KR: KRNethack 맞춤 번역 */
                pline("%s 길을 막고 있다!",
                      !canspotmon(mtmp)
                          ? "어떤 생물이"
                          : append_josa(Monnam(mtmp), "가"));
#endif
            }
        }
        if (!canspotmon(mtmp))
            map_invisible(x, y);
        return TRUE;
    }
    if (OBJ_AT(x, y)) {
    objhere:
        if (!quietly)
            /*KR pline("%s's in the way.", Something); */
            pline("무언가가 길을 막고 있다.");
        return TRUE;
    }
    return FALSE;
}

/* the 'C' command - try to close a door */
int
doclose()
{
    register int x, y;
    register struct rm *door;
    boolean portcullis;
    int res = 0;

    if (nohands(youmonst.data)) {
        /*KR You_cant("close anything -- you have no hands!"); */
        You_cant("아무것도 닫을 수 없다 -- 손이 없으니까!");
        return 0;
    }

    if (u.utrap && u.utraptype == TT_PIT) {
        /*KR You_cant("reach over the edge of the pit."); */
        You_cant("구덩이 밖으로는 손이 닿지 않는다.");
        return 0;
    }

    if (!getdir((char *) 0))
        return 0;

    x = u.ux + u.dx;
    y = u.uy + u.dy;
    if ((x == u.ux) && (y == u.uy)) {
        /*KR You("are in the way!"); */
        You("당신이 길을 막고 있다!");
        return 1;
    }

    if (!isok(x, y))
        goto nodoor;

    if (stumble_on_door_mimic(x, y))
        return 1;

    /* when choosing a direction is impaired, use a turn
       regardless of whether a door is successfully targetted */
    if (Confusion || Stunned)
        res = 1;

    door = &levl[x][y];
    portcullis = (is_drawbridge_wall(x, y) >= 0);
    if (Blind) {
        int oldglyph = door->glyph;
        schar oldlastseentyp = lastseentyp[x][y];

        feel_location(x, y);
        if (door->glyph != oldglyph || lastseentyp[x][y] != oldlastseentyp)
            res = 1; /* learned something */
    }

    if (portcullis || !IS_DOOR(door->typ)) {
        /* is_db_wall: closed portcullis */
        if (is_db_wall(x, y) || door->typ == DRAWBRIDGE_UP)
            /*KR pline_The("drawbridge is already closed."); */
            pline("도개교는 이미 닫혀 있다.");
        else if (portcullis || door->typ == DRAWBRIDGE_DOWN)
            /*KR There("is no obvious way to close the drawbridge."); */
            pline("도개교를 닫을 확실한 방법은 없는 것 같다.");
        else {
        nodoor:
            /*KR You("%s no door there.", Blind ? "feel" : "see"); */
            You("그곳에 문이 없는 것을 %s.", Blind ? "알았다" : "보았다");
        }
        return res;
    }

    if (door->doormask == D_NODOOR) {
        /*KR pline("This doorway has no door."); */
        pline("이 통로에는 문이 없다.");
        return res;
    } else if (obstructed(x, y, FALSE)) {
        return res;
    } else if (door->doormask == D_BROKEN) {
        /*KR pline("This door is broken."); */
        pline("이 문은 부서져 있다.");
        return res;
    } else if (door->doormask & (D_CLOSED | D_LOCKED)) {
        /*KR pline("This door is already closed."); */
        pline("이 문은 이미 닫혀 있다.");
        return res;
    }

    if (door->doormask == D_ISOPEN) {
        if (verysmall(youmonst.data) && !u.usteed) {
            /*KR pline("You're too small to push the door closed."); */
            pline("당신은 너무 작아서 문을 밀어 닫을 수 없다.");
            return res;
        }
        if (u.usteed
            || rn2(25) < (ACURRSTR + ACURR(A_DEX) + ACURR(A_CON)) / 3) {
            /*KR pline_The("door closes."); */
            pline("문이 닫혔다.");
            door->doormask = D_CLOSED;
            feel_newsym(x, y); /* the hero knows she closed it */
            block_point(x, y); /* vision:  no longer see there */
        } else {
            exercise(A_STR, TRUE);
            /*KR pline_The("door resists!"); */
            pline("문이 닫히지 않는다!");
        }
    }

    return 1;
}

/* box obj was hit with spell or wand effect otmp;
   returns true if something happened */
boolean
boxlock(obj, otmp)
struct obj *obj, *otmp; /* obj *is* a box */
{
    boolean res = 0;

    switch (otmp->otyp) {
    case WAN_LOCKING:
    case SPE_WIZARD_LOCK:
        if (!obj->olocked) { /* lock it; fix if broken */
            /*KR pline("Klunk!"); */
            pline("철컥!");
            obj->olocked = 1;
            obj->obroken = 0;
            if (Role_if(PM_WIZARD))
                obj->lknown = 1;
            else
                obj->lknown = 0;
            res = 1;
        } /* else already closed and locked */
        break;
    case WAN_OPENING:
    case SPE_KNOCK:
        if (obj->olocked) { /* unlock; couldn't be broken */
            /*KR pline("Klick!"); */
            pline("딸깍!");
            obj->olocked = 0;
            res = 1;
            if (Role_if(PM_WIZARD))
                obj->lknown = 1;
            else
                obj->lknown = 0;
        } else /* silently fix if broken */
            obj->obroken = 0;
        break;
    case WAN_POLYMORPH:
    case SPE_POLYMORPH:
        /* maybe start unlocking chest, get interrupted, then zap it;
           we must avoid any attempt to resume unlocking it */
        if (xlock.box == obj)
            reset_pick();
        break;
    }
    return res;
}

/* Door/secret door was hit with spell or wand effect otmp;
   returns true if something happened */
boolean
doorlock(otmp, x, y)
struct obj *otmp;
int x, y;
{
    register struct rm *door = &levl[x][y];
    boolean res = TRUE;
    int loudness = 0;
    const char *msg = (const char *) 0;
    /*KR const char *dustcloud = "A cloud of dust"; */
    const char *dustcloud = "먼지구름이";
    /*KR const char *quickly_dissipates = "quickly dissipates"; */
    const char *quickly_dissipates = "순식간에 흩어졌다";
    boolean mysterywand = (otmp->oclass == WAND_CLASS && !otmp->dknown);

    if (door->typ == SDOOR) {
        switch (otmp->otyp) {
        case WAN_OPENING:
        case SPE_KNOCK:
        case WAN_STRIKING:
        case SPE_FORCE_BOLT:
            door->typ = DOOR;
            door->doormask = D_CLOSED | (door->doormask & D_TRAPPED);
            newsym(x, y);
            if (cansee(x, y))
                /*KR pline("A door appears in the wall!"); */
                pline("벽에서 문이 나타났다!");
            if (otmp->otyp == WAN_OPENING || otmp->otyp == SPE_KNOCK)
                return TRUE;
            break; /* striking: continue door handling below */
        case WAN_LOCKING:
        case SPE_WIZARD_LOCK:
        default:
            return FALSE;
        }
    }

    switch (otmp->otyp) {
    case WAN_LOCKING:
    case SPE_WIZARD_LOCK:
        if (Is_rogue_level(&u.uz)) {
            boolean vis = cansee(x, y);
            /* Can't have real locking in Rogue, so just hide doorway */
            if (vis)
                /*KR pline("%s springs up in the older, more primitive
                   doorway.", dustcloud); */
                pline("오래되고 원시적인 통로에 %s 일어났다.", dustcloud);
            else
                /*KR You_hear("a swoosh."); */
                You_hear("쉿 하는 소리를 들었다.");
            if (obstructed(x, y, mysterywand)) {
                if (vis)
                    /*KR pline_The("cloud %s.", quickly_dissipates); */
                    pline("그 구름은 %s.", quickly_dissipates);
                return FALSE;
            }
            block_point(x, y);
            door->typ = SDOOR, door->doormask = D_NODOOR;
            if (vis)
                /*KR pline_The("doorway vanishes!"); */
                pline("통로가 사라졌다!");
            newsym(x, y);
            return TRUE;
        }
        if (obstructed(x, y, mysterywand))
            return FALSE;
        /* Don't allow doors to close over traps.  This is for pits */
        /* & trap doors, but is it ever OK for anything else? */
        if (t_at(x, y)) {
            /* maketrap() clears doormask, so it should be NODOOR */
#if 0 /*KR: 원본*/
            pline("%s springs up in the doorway, but %s.", dustcloud,
                  quickly_dissipates);
#else /*KR: KRNethack 맞춤 번역 */
            pline("통로에 %s 일어났지만, %s.", dustcloud, quickly_dissipates);
#endif
            return FALSE;
        }

        switch (door->doormask & ~D_TRAPPED) {
        case D_CLOSED:
            /*KR msg = "The door locks!"; */
            msg = "문이 잠겼다!";
            break;
        case D_ISOPEN:
            /*KR msg = "The door swings shut, and locks!"; */
            msg = "문이 쾅 닫히더니, 잠겼다!";
            break;
        case D_BROKEN:
            /*KR msg = "The broken door reassembles and locks!"; */
            msg = "부서졌던 문이 다시 합쳐지더니 잠겼다!";
            break;
        case D_NODOOR:
            /*KR msg =
                "A cloud of dust springs up and assembles itself into a
               door!"; */
            msg = "먼지구름이 일어나더니 모여서 문이 되었다!";
            break;
        default:
            res = FALSE;
            break;
        }
        block_point(x, y);
        door->doormask = D_LOCKED | (door->doormask & D_TRAPPED);
        newsym(x, y);
        break;
    case WAN_OPENING:
    case SPE_KNOCK:
        if (door->doormask & D_LOCKED) {
            /*KR msg = "The door unlocks!"; */
            msg = "문 잠금이 풀렸다!";
            door->doormask = D_CLOSED | (door->doormask & D_TRAPPED);
        } else
            res = FALSE;
        break;
    case WAN_STRIKING:
    case SPE_FORCE_BOLT:
        if (door->doormask & (D_LOCKED | D_CLOSED)) {
            if (door->doormask & D_TRAPPED) {
                if (MON_AT(x, y))
                    (void) mb_trapped(m_at(x, y));
                else if (flags.verbose) {
                    if (cansee(x, y))
                        /*KR pline("KABOOM!!  You see a door explode."); */
                        pline("콰앙!! 문이 폭발하는 것을 보았다.");
                    else
                        /*KR You_hear("a distant explosion."); */
                        You_hear("멀리서 폭발음이 들린다.");
                }
                door->doormask = D_NODOOR;
                unblock_point(x, y);
                newsym(x, y);
                loudness = 40;
                break;
            }
            door->doormask = D_BROKEN;
            if (flags.verbose) {
                if (cansee(x, y))
                    /*KR pline_The("door crashes open!"); */
                    pline("문이 박살 나며 열렸다!");
                else
                    /*KR You_hear("a crashing sound."); */
                    You_hear("무언가 박살 나는 소리가 들린다.");
            }
            unblock_point(x, y);
            newsym(x, y);
            /* force vision recalc before printing more messages */
            if (vision_full_recalc)
                vision_recalc(0);
            loudness = 20;
        } else
            res = FALSE;
        break;
    default:
        impossible("magic (%d) attempted on door.", otmp->otyp);
        break;
    }
    if (msg && cansee(x, y))
        pline1(msg);
    if (loudness > 0) {
        /* door was destroyed */
        wake_nearto(x, y, loudness);
        if (*in_rooms(x, y, SHOPBASE))
            add_damage(x, y, 0L);
    }

    if (res && picking_at(x, y)) {
        /* maybe unseen monster zaps door you're unlocking */
        stop_occupation();
        reset_pick();
    }
    return res;
}

STATIC_OVL void chest_shatter_msg(otmp) struct obj *otmp;
{
    const char *disposition;
    const char *thing;
    long save_Blinded;

    if (otmp->oclass == POTION_CLASS) {
#if 0 /*KR: 원본*/
        You("%s %s shatter!", Blind ? "hear" : "see", an(bottlename()));
#else /*KR: KRNethack 맞춤 번역 */
        You("%s 산산조각 나는 것을 %s!", append_josa(bottlename(), "가"),
            Blind ? "들었다" : "보았다");
#endif
        if (!breathless(youmonst.data) || haseyes(youmonst.data))
            potionbreathe(otmp);
        return;
    }
    /* We have functions for distant and singular names, but not one */
    /* which does _both_... */
    save_Blinded = Blinded;
    Blinded = 1;
    thing = singular(otmp, xname);
    Blinded = save_Blinded;
    switch (objects[otmp->otyp].oc_material) {
    case PAPER:
        /*KR disposition = "is torn to shreds"; */
        disposition = "갈기갈기 찢겼다";
        break;
    case WAX:
        /*KR disposition = "is crushed"; */
        disposition = "부서졌다";
        break;
    case VEGGY:
        /*KR disposition = "is pulped"; */
        disposition = "짓이겨졌다";
        break;
    case FLESH:
        /*KR disposition = "is mashed"; */
        disposition = "으깨졌다";
        break;
    case GLASS:
        /*KR disposition = "shatters"; */
        disposition = "산산조각 났다";
        break;
    case WOOD:
        /*KR disposition = "splinters to fragments"; */
        disposition = "파편으로 부서졌다";
        break;
    default:
        /*KR disposition = "is destroyed"; */
        disposition = "파괴되었다";
        break;
    }
    /*KR pline("%s %s!", An(thing), disposition); */
    pline("%s %s!", append_josa(thing, "은"), disposition);
}

/*lock.c*/
