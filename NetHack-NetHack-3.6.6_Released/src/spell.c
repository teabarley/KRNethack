/* NetHack 3.6	spell.c	$NHDT-Date: 1546565814 2019/01/04 01:36:54 $  $NHDT-Branch: NetHack-3.6.2-beta01 $:$NHDT-Revision: 1.88 $ */
/* Copyright (c) M. Stephenson 1988                                  */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

/* spellmenu arguments; 0 thru n-1 used as spl_book[] index when swapping */
#define SPELLMENU_CAST (-2)
#define SPELLMENU_VIEW (-1)
#define SPELLMENU_SORT (MAXSPELL) /* special menu entry */

/* spell retention period, in turns; at 10% of this value, player becomes
   eligible to reread the spellbook and regain 100% retention (the threshold
   used to be 1000 turns, which was 10% of the original 10000 turn retention
   period but didn't get adjusted when that period got doubled to 20000) */
#define KEEN 20000
/* x: need to add 1 when used for reading a spellbook rather than for hero
   initialization; spell memory is decremented at the end of each turn,
   including the turn on which the spellbook is read; without the extra
   increment, the hero used to get cheated out of 1 turn of retention */
#define incrnknow(spell, x) (spl_book[spell].sp_know = KEEN + (x))

#define spellev(spell) spl_book[spell].sp_lev
#define spellname(spell) OBJ_NAME(objects[spellid(spell)])
#define spellet(spell) \
    ((char) ((spell < 26) ? ('a' + spell) : ('A' + spell - 26)))

STATIC_DCL int FDECL(spell_let_to_idx, (CHAR_P));
STATIC_DCL boolean FDECL(cursed_book, (struct obj * bp));
STATIC_DCL boolean FDECL(confused_book, (struct obj *) );
STATIC_DCL void FDECL(deadbook, (struct obj *) );
STATIC_PTR int NDECL(learn);
STATIC_DCL boolean NDECL(rejectcasting);
STATIC_DCL boolean FDECL(getspell, (int *) );
STATIC_PTR int FDECL(CFDECLSPEC spell_cmp,
                     (const genericptr, const genericptr));
STATIC_DCL void NDECL(sortspells);
STATIC_DCL boolean NDECL(spellsortmenu);
STATIC_DCL boolean FDECL(dospellmenu, (const char *, int, int *) );
STATIC_DCL int FDECL(percent_success, (int) );
STATIC_DCL char *FDECL(spellretention, (int, char *) );
STATIC_DCL int NDECL(throwspell);
STATIC_DCL void NDECL(cast_protection);
STATIC_DCL void FDECL(spell_backfire, (int) );
STATIC_DCL const char *FDECL(spelltypemnemonic, (int) );
STATIC_DCL boolean FDECL(spell_aim_step, (genericptr_t, int, int) );

/* The roles[] table lists the role-specific values for tuning
 * percent_success().
 *
 * Reasoning:
 * spelbase, spelheal:
 * Arc are aware of magic through historical research
 * Bar abhor magic (Conan finds it "interferes with his animal instincts")
 * Cav are ignorant to magic
 * Hea are very aware of healing magic through medical research
 * Kni are moderately aware of healing from Paladin training
 * Mon use magic to attack and defend in lieu of weapons and armor
 * Pri are very aware of healing magic through theological research
 * Ran avoid magic, preferring to fight unseen and unheard
 * Rog are moderately aware of magic through trickery
 * Sam have limited magical awareness, preferring meditation to conjuring
 * Tou are aware of magic from all the great films they have seen
 * Val have limited magical awareness, preferring fighting
 * Wiz are trained mages
 *
 * The arms penalty is lessened for trained fighters Bar, Kni, Ran,
 * Sam, Val -- the penalty is its metal interference, not encumbrance.
 * The `spelspec' is a single spell which is fundamentally easier
 * for that role to cast.
 *
 * spelspec, spelsbon:
 * Arc map masters (SPE_MAGIC_MAPPING)
 * Bar fugue/berserker (SPE_HASTE_SELF)
 * Cav born to dig (SPE_DIG)
 * Hea to heal (SPE_CURE_SICKNESS)
 * Kni to turn back evil (SPE_TURN_UNDEAD)
 * Mon to preserve their abilities (SPE_RESTORE_ABILITY)
 * Pri to bless (SPE_REMOVE_CURSE)
 * Ran to hide (SPE_INVISIBILITY)
 * Rog to find loot (SPE_DETECT_TREASURE)
 * Sam to be At One (SPE_CLAIRVOYANCE)
 * Tou to smile (SPE_CHARM_MONSTER)
 * Val control the cold (SPE_CONE_OF_COLD)
 * Wiz all really, but SPE_MAGIC_MISSILE is their party trick
 *
 * See percent_success() below for more comments.
 *
 * uarmbon, uarmsbon, uarmhbon, uarmgbon, uarmfbon:
 * Fighters find body armour & shield a little less limiting.
 * Headgear, Gauntlets and Footwear are not role-specific (but
 * still have an effect, except helm of brilliance, which is designed
 * to permit magic-use).
 */

#define uarmhbon 4 /* Metal helmets interfere with the mind */
#define uarmgbon 6 /* Casting channels through the hands */
#define uarmfbon 2 /* All metal interferes to some degree */

/* since the spellbook itself doesn't blow up, don't say just "explodes" */
#if 0 /*KR: 원본*/
static const char explodes[] = "radiates explosive energy";
#endif

/* convert a letter into a number in the range 0..51, or -1 if not a letter */
STATIC_OVL int
spell_let_to_idx(ilet)
char ilet;
{
    int indx;

    indx = ilet - 'a';
    if (indx >= 0 && indx < 26)
        return indx;
    indx = ilet - 'A';
    if (indx >= 0 && indx < 26)
        return indx + 26;
    return -1;
}

/* TRUE: book should be destroyed by caller */
STATIC_OVL boolean
cursed_book(bp)
struct obj *bp;
{
    boolean was_in_use;
    int lev = objects[bp->otyp].oc_level;
    int dmg = 0;

    switch (rn2(lev)) {
    case 0:
        /*KR You_feel("a wrenching sensation."); */
        You_feel("몸이 비틀어지는 듯한 감각을 느꼈다.");
        tele(); /* teleport him */
        break;
    case 1:
        /*KR You_feel("threatened."); */
        You_feel("위협받고 있는 것 같다.");
        aggravate();
        break;
    case 2:
        make_blinded(Blinded + rn1(100, 250), TRUE);
        break;
    case 3:
        take_gold();
        break;
    case 4:
        /*KR pline("These runes were just too much to comprehend."); */
        pline("이 룬 문자들은 이해하기에 너무 벅차다.");
        make_confused(HConfusion + rn1(7, 16), FALSE);
        break;
    case 5:
        /*KR pline_The("book was coated with contact poison!"); */
        pline("그 책은 접촉성 독으로 코팅되어 있었다!");
        if (uarmg) {
            /*KR erode_obj(uarmg, "gloves", ERODE_CORRODE, EF_GREASE |
             * EF_VERBOSE); */
            erode_obj(uarmg, "장갑", ERODE_CORRODE, EF_GREASE | EF_VERBOSE);
            break;
        }
        /* temp disable in_use; death should not destroy the book */
        was_in_use = bp->in_use;
        bp->in_use = FALSE;
        losestr(Poison_resistance ? rn1(2, 1) : rn1(4, 3));
        /*KR losehp(rnd(Poison_resistance ? 6 : 10), "contact-poisoned
           spellbook", KILLED_BY_AN); */
        losehp(rnd(Poison_resistance ? 6 : 10), "접촉성 독이 발린 마법서",
               KILLED_BY_AN);
        bp->in_use = was_in_use;
        break;
    case 6:
        if (Antimagic) {
            shieldeff(u.ux, u.uy);
            /*KR pline_The("book %s, but you are unharmed!", explodes); */
            pline("책이 폭발적인 에너지를 뿜어냈지만, 당신은 무사하다!");
        } else {
            /*KR pline("As you read the book, it %s in your %s!", explodes,
                  body_part(FACE)); */
            pline("책을 읽는 순간, 책이 당신의 %s 폭발적인 에너지를 "
                  "뿜어냈다!",
                  append_josa(body_part(FACE), "으로"));
            dmg = 2 * rnd(10) + 5;
            /*KR losehp(Maybe_Half_Phys(dmg), "exploding rune", KILLED_BY_AN);
             */
            losehp(Maybe_Half_Phys(dmg), "폭발하는 룬 문자", KILLED_BY_AN);
        }
        return TRUE;
    default:
        rndcurse();
        break;
    }
    return FALSE;
}

/* study while confused: returns TRUE if the book is destroyed */
STATIC_OVL boolean
confused_book(spellbook)
struct obj *spellbook;
{
    boolean gone = FALSE;

    if (!rn2(3) && spellbook->otyp != SPE_BOOK_OF_THE_DEAD) {
        spellbook->in_use = TRUE; /* in case called from learn */
        /*KR pline(
         "Being confused you have difficulties in controlling your actions.");
       */
        pline("혼란스러워서 행동을 제대로 통제하기가 어렵다.");
        display_nhwindow(WIN_MESSAGE, FALSE);
        /*KR You("accidentally tear the spellbook to pieces."); */
        You("실수로 그 마법서를 갈기갈기 찢어버렸다.");
        if (!objects[spellbook->otyp].oc_name_known
            && !objects[spellbook->otyp].oc_uname)
            docall(spellbook);
        useup(spellbook);
        gone = TRUE;
    } else {
#if 0 /*KR: 원본*/
        You("find yourself reading the %s line over and over again.",
            spellbook == context.spbook.book ? "next" : "first");
#else /*KR: KRNethack 맞춤 번역 */
        You("자신도 모르게 %s 줄을 계속해서 반복해 읽고 있다.",
            spellbook == context.spbook.book ? "다음" : "첫");
#endif
    }
    return gone;
}

/* special effects for The Book of the Dead */
STATIC_OVL void deadbook(book2) struct obj *book2;
{
    struct monst *mtmp, *mtmp2;
    coord mm;

    /*KR You("turn the pages of the Book of the Dead..."); */
    You("사자의 서의 페이지를 넘겼다...");
    makeknown(SPE_BOOK_OF_THE_DEAD);
    /* KMH -- Need ->known to avoid "_a_ Book of the Dead" */
    book2->known = 1;
    if (invocation_pos(u.ux, u.uy) && !On_stairs(u.ux, u.uy)) {
        register struct obj *otmp;
        register boolean arti1_primed = FALSE, arti2_primed = FALSE,
                         arti_cursed = FALSE;

        if (book2->cursed) {
            /*KR pline_The("runes appear scrambled.  You can't read them!");
             */
            pline("룬 문자들이 뒤죽박죽으로 섞여 있다. 읽을 수가 없다!");
            return;
        }

        if (!u.uhave.bell || !u.uhave.menorah) {
            /*KR pline("A chill runs down your %s.", body_part(SPINE)); */
            pline("당신의 %s 타고 오싹한 기운이 흘러내린다.",
                  append_josa(body_part(SPINE), "을"));
            if (!u.uhave.bell)
                /*KR You_hear("a faint chime..."); */
                You_hear("희미한 종소리가 들린다...");
            if (!u.uhave.menorah)
                /*KR pline("Vlad's doppelganger is amused."); */
                pline("블라드의 도플갱어가 즐거워한다.");
            return;
        }

        for (otmp = invent; otmp; otmp = otmp->nobj) {
            if (otmp->otyp == CANDELABRUM_OF_INVOCATION && otmp->spe == 7
                && otmp->lamplit) {
                if (!otmp->cursed)
                    arti1_primed = TRUE;
                else
                    arti_cursed = TRUE;
            }
            if (otmp->otyp == BELL_OF_OPENING
                && (moves - otmp->age) < 5L) { /* you rang it recently */
                if (!otmp->cursed)
                    arti2_primed = TRUE;
                else
                    arti_cursed = TRUE;
            }
        }

        if (arti_cursed) {
            /*KR pline_The("invocation fails!"); */
            pline("기원이 실패했다!");
            /*KR pline("At least one of your artifacts is cursed..."); */
            pline("적어도 당신의 아티팩트 중 하나는 저주받은 것 같다...");
        } else if (arti1_primed && arti2_primed) {
            unsigned soon =
                (unsigned) d(2, 6); /* time til next intervene() */

            /* successful invocation */
            mkinvokearea();
            u.uevent.invoked = 1;
            /* in case you haven't killed the Wizard yet, behave as if
               you just did */
            u.uevent.udemigod = 1; /* wizdead() */
            if (!u.udg_cnt || u.udg_cnt > soon)
                u.udg_cnt = soon;
        } else { /* at least one artifact not prepared properly */
            /*KR You("have a feeling that %s is amiss...", something); */
            You("%s 잘못되었다는 느낌이 든다...",
                append_josa(something, "이"));
            goto raise_dead;
        }
        return;
    }

    /* when not an invocation situation */
    if (book2->cursed) {
    raise_dead:

        /*KR You("raised the dead!"); */
        You("사망자들을 깨웠다!");
        /* first maybe place a dangerous adversary */
        if (!rn2(3)
            && ((mtmp =
                     makemon(&mons[PM_MASTER_LICH], u.ux, u.uy, NO_MINVENT))
                    != 0
                || (mtmp =
                        makemon(&mons[PM_NALFESHNEE], u.ux, u.uy, NO_MINVENT))
                       != 0)) {
            mtmp->mpeaceful = 0;
            set_malign(mtmp);
        }
        /* next handle the affect on things you're carrying */
        (void) unturn_dead(&youmonst);
        /* last place some monsters around you */
        mm.x = u.ux;
        mm.y = u.uy;
        mkundead(&mm, TRUE, NO_MINVENT);
    } else if (book2->blessed) {
        for (mtmp = fmon; mtmp; mtmp = mtmp2) {
            mtmp2 = mtmp->nmon; /* tamedog() changes chain */
            if (DEADMONSTER(mtmp))
                continue;

            if ((is_undead(mtmp->data) || is_vampshifter(mtmp))
                && cansee(mtmp->mx, mtmp->my)) {
                mtmp->mpeaceful = TRUE;
                if (sgn(mtmp->data->maligntyp) == sgn(u.ualign.type)
                    && distu(mtmp->mx, mtmp->my) < 4)
                    if (mtmp->mtame) {
                        if (mtmp->mtame < 20)
                            mtmp->mtame++;
                    } else
                        (void) tamedog(mtmp, (struct obj *) 0);
                else
                    monflee(mtmp, 0, FALSE, TRUE);
            }
        }
    } else {
        switch (rn2(3)) {
        case 0:
            /*KR Your("ancestors are annoyed with you!"); */
            Your("조상님들이 당신에게 짜증을 내신다!");
            break;
        case 1:
            /*KR pline_The("headstones in the cemetery begin to move!"); */
            pline("공동묘지의 묘비들이 움직이기 시작했다!");
            break;
        default:
            /*KR pline("Oh my!  Your name appears in the book!"); */
            pline("이런 세상에! 책에 당신의 이름이 적혀 있다!");
        }
    }
    return;
}

/* 'book' has just become cursed; if we're reading it and realize it is
   now cursed, interrupt */
void book_cursed(book) struct obj *book;
{
    if (occupation == learn && context.spbook.book == book && book->cursed
        && book->bknown && multi >= 0)
        stop_occupation();
}

STATIC_PTR int
learn(VOID_ARGS)
{
    int i;
    short booktype;
    char splname[BUFSZ];
    boolean costly = TRUE;
    struct obj *book = context.spbook.book;

    /* JDS: lenses give 50% faster reading; 33% smaller read time */
    if (context.spbook.delay && ublindf && ublindf->otyp == LENSES && rn2(2))
        context.spbook.delay++;
    if (Confusion) { /* became confused while learning */
        (void) confused_book(book);
        context.spbook.book = 0; /* no longer studying */
        context.spbook.o_id = 0;
        nomul(context.spbook.delay); /* remaining delay is uninterrupted */
        /*KR multi_reason = "reading a book"; */
        multi_reason = "책을 읽고 있어서";
        nomovemsg = 0;
        context.spbook.delay = 0;
        return 0;
    }
    if (context.spbook.delay) {
        /* not if (context.spbook.delay++), so at end delay == 0 */
        context.spbook.delay++;
        return 1; /* still busy */
    }
    exercise(A_WIS, TRUE); /* you're studying. */
    booktype = book->otyp;
    if (booktype == SPE_BOOK_OF_THE_DEAD) {
        deadbook(book);
        return 0;
    }

#if 0 /*KR: 원본*/
    Sprintf(splname,
            objects[booktype].oc_name_known ? "\"%s\"" : "the \"%s\" spell",
            OBJ_NAME(objects[booktype]));
#else /*KR: KRNethack 맞춤 번역 */
    Sprintf(splname, objects[booktype].oc_name_known ? "\"%s\"" : "\"%s\"",
            OBJ_NAME(objects[booktype]));
#endif
    for (i = 0; i < MAXSPELL; i++)
        if (spellid(i) == booktype || spellid(i) == NO_SPELL)
            break;

    if (i == MAXSPELL) {
        impossible("Too many spells memorized!");
    } else if (spellid(i) == booktype) {
        /* normal book can be read and re-read a total of 4 times */
        if (book->spestudied > MAX_SPELL_STUDY) {
            /*KR pline("This spellbook is too faint to be read any more."); */
            pline("이 마법서의 글씨는 너무 희미해서 더 이상 읽을 수 없다.");
            book->otyp = booktype = SPE_BLANK_PAPER;
            /* reset spestudied as if polymorph had taken place */
            book->spestudied = rn2(book->spestudied);
        } else if (spellknow(i) > KEEN / 10) {
            /*KR You("know %s quite well already.", splname); */
            You("이미 %s 잘 알고 있다.", append_josa(splname, "을"));
            costly = FALSE;
        } else { /* spellknow(i) <= KEEN/10 */
#if 0            /*KR: 원본*/
            Your("knowledge of %s is %s.", splname,
                 spellknow(i) ? "keener" : "restored");
#else            /*KR: KRNethack 맞춤 번역 */
            Your("%s 관한 지식이 %s.", append_josa(splname, "에"),
                 spellknow(i) ? "더욱 선명해졌다" : "회복되었다");
#endif
            incrnknow(i, 1);
            book->spestudied++;
            exercise(A_WIS, TRUE); /* extra study */
        }
        /* make book become known even when spell is already
           known, in case amnesia made you forget the book */
        makeknown((int) booktype);
    } else { /* (spellid(i) == NO_SPELL) */
        /* for a normal book, spestudied will be zero, but for
           a polymorphed one, spestudied will be non-zero and
           one less reading is available than when re-learning */
        if (book->spestudied >= MAX_SPELL_STUDY) {
            /* pre-used due to being the product of polymorph */
            /*KR pline("This spellbook is too faint to read even once."); */
            pline("이 마법서의 글씨는 너무 희미해서 한 번도 읽을 수가 없다.");
            book->otyp = booktype = SPE_BLANK_PAPER;
            /* reset spestudied as if polymorph had taken place */
            book->spestudied = rn2(book->spestudied);
        } else {
            spl_book[i].sp_id = booktype;
            spl_book[i].sp_lev = objects[booktype].oc_level;
            incrnknow(i, 1);
            book->spestudied++;
            /*KR You(i > 0 ? "add %s to your repertoire." : "learn %s.",
             * splname); */
            You(i > 0 ? "%s 당신의 레퍼토리에 추가했다." : "%s 배웠다.",
                append_josa(splname, "을"));
        }
        makeknown((int) booktype);
    }

    if (book->cursed) { /* maybe a demon cursed it */
        if (cursed_book(book)) {
            useup(book);
            context.spbook.book = 0;
            context.spbook.o_id = 0;
            return 0;
        }
    }
    if (costly)
        check_unpaid(book);
    context.spbook.book = 0;
    context.spbook.o_id = 0;
    return 0;
}

int
study_book(spellbook)
register struct obj *spellbook;
{
    int booktype = spellbook->otyp;
    boolean confused = (Confusion != 0);
    boolean too_hard = FALSE;

    /* attempting to read dull book may make hero fall asleep */
    if (!confused
        && !Sleep_resistance
        /*KR && !strcmp(OBJ_DESCR(objects[booktype]), "dull")) { */
        && !strcmp(OBJ_DESCR(objects[booktype]), "따분한")) {
        const char *eyes;
        int dullbook = rnd(25) - ACURR(A_WIS);

        /* adjust chance if hero stayed awake, got interrupted, retries */
        if (context.spbook.delay && spellbook == context.spbook.book)
            dullbook -= rnd(objects[booktype].oc_level);

        if (dullbook > 0) {
            eyes = body_part(EYE);
            if (eyecount(youmonst.data) > 1)
                eyes = makeplural(eyes);
            /*KR pline("This book is so dull that you can't keep your %s
               open.", eyes); */
            pline(
                "이 책은 너무나 따분해서 당신은 %s 계속 뜨고 있을 수가 없다.",
                append_josa(eyes, "을"));
            dullbook += rnd(2 * objects[booktype].oc_level);
            fall_asleep(-dullbook, TRUE);
            return 1;
        }
    }

    if (context.spbook.delay && !confused
        && spellbook == context.spbook.book
        /* handle the sequence: start reading, get interrupted, have
           context.spbook.book become erased somehow, resume reading it */
        && booktype != SPE_BLANK_PAPER) {
#if 0 /*KR: 원본*/
        You("continue your efforts to %s.",
            (booktype == SPE_NOVEL) ? "read the novel" : "memorize the spell");
#else /*KR: KRNethack 맞춤 번역 */
        You("계속해서 %s 읽으려고 노력한다.",
            (booktype == SPE_NOVEL) ? "소설을" : "주문을 기억해 내려고");
#endif
    } else {
        /* KMH -- Simplified this code */
        if (booktype == SPE_BLANK_PAPER) {
            /*KR pline("This spellbook is all blank."); */
            pline("이 마법서는 온통 백지다.");
            makeknown(booktype);
            return 1;
        }

        /* 3.6 tribute */
        if (booktype == SPE_NOVEL) {
            /* Obtain current Terry Pratchett book title */
            const char *tribtitle = noveltitle(&spellbook->novelidx);

            if (read_tribute("books", tribtitle, 0, (char *) 0, 0,
                             spellbook->o_id)) {
                u.uconduct.literate++;
                check_unpaid(spellbook);
                makeknown(booktype);
                if (!u.uevent.read_tribute) {
                    /* give bonus of 20 xp and 4*20+0 pts */
                    more_experienced(20, 0);
                    newexplevel();
                    u.uevent.read_tribute = 1; /* only once */
                }
            }
            return 1;
        }

        switch (objects[booktype].oc_level) {
        case 1:
        case 2:
            context.spbook.delay = -objects[booktype].oc_delay;
            break;
        case 3:
        case 4:
            context.spbook.delay = -(objects[booktype].oc_level - 1)
                                   * objects[booktype].oc_delay;
            break;
        case 5:
        case 6:
            context.spbook.delay =
                -objects[booktype].oc_level * objects[booktype].oc_delay;
            break;
        case 7:
            context.spbook.delay = -8 * objects[booktype].oc_delay;
            break;
        default:
            impossible("Unknown spellbook level %d, book %d;",
                       objects[booktype].oc_level, booktype);
            return 0;
        }

        /* Books are often wiser than their readers (Rus.) */
        spellbook->in_use = TRUE;
        if (!spellbook->blessed && spellbook->otyp != SPE_BOOK_OF_THE_DEAD) {
            if (spellbook->cursed) {
                too_hard = TRUE;
            } else {
                /* uncursed - chance to fail */
                int read_ability =
                    ACURR(A_INT) + 4 + u.ulevel / 2
                    - 2 * objects[booktype].oc_level
                    + ((ublindf && ublindf->otyp == LENSES) ? 2 : 0);

                /* only wizards know if a spell is too difficult */
                if (Role_if(PM_WIZARD) && read_ability < 20 && !confused) {
                    char qbuf[QBUFSZ];

#if 0 /*KR: 원본*/
                    Sprintf(qbuf,
                    "This spellbook is %sdifficult to comprehend.  Continue?",
                            (read_ability < 12 ? "very " : ""));
#else /*KR: KRNethack 맞춤 번역 */
                    Sprintf(qbuf,
                            "이 마법서를 이해하기에는 %s어렵습니다. "
                            "계속하시겠습니까?",
                            (read_ability < 12 ? "매우 " : ""));
#endif
                    if (yn(qbuf) != 'y') {
                        spellbook->in_use = FALSE;
                        return 1;
                    }
                }
                /* its up to random luck now */
                if (rnd(20) > read_ability) {
                    too_hard = TRUE;
                }
            }
        }

        if (too_hard) {
            boolean gone = cursed_book(spellbook);

            nomul(context.spbook.delay); /* study time */
            /*KR multi_reason = "reading a book"; */
            multi_reason = "책을 읽고 있어서";
            nomovemsg = 0;
            context.spbook.delay = 0;
            if (gone || !rn2(3)) {
                if (!gone)
                    /*KR pline_The("spellbook crumbles to dust!"); */
                    pline("마법서가 바스라져 먼지가 되었다!");
                if (!objects[spellbook->otyp].oc_name_known
                    && !objects[spellbook->otyp].oc_uname)
                    docall(spellbook);
                useup(spellbook);
            } else
                spellbook->in_use = FALSE;
            return 1;
        } else if (confused) {
            if (!confused_book(spellbook)) {
                spellbook->in_use = FALSE;
            }
            nomul(context.spbook.delay);
            /*KR multi_reason = "reading a book"; */
            multi_reason = "책을 읽고 있어서";
            nomovemsg = 0;
            context.spbook.delay = 0;
            return 1;
        }
        spellbook->in_use = FALSE;

#if 0 /*KR: 원본*/
        You("begin to %s the runes.",
            spellbook->otyp == SPE_BOOK_OF_THE_DEAD ? "recite" : "memorize");
#else /*KR: KRNethack 맞춤 번역 */
        You("그 룬 문자들을 %s 시작했다.",
            spellbook->otyp == SPE_BOOK_OF_THE_DEAD ? "암송하기"
                                                    : "암기하기");
#endif
    }

    context.spbook.book = spellbook;
    if (context.spbook.book)
        context.spbook.o_id = context.spbook.book->o_id;
    /*KR set_occupation(learn, "studying", 0); */
    set_occupation(learn, "공부하기", 0);
    return 1;
}

/* a spellbook has been destroyed or the character has changed levels;
   the stored address for the current book is no longer valid */
void book_disappears(obj) struct obj *obj;
{
    if (obj == context.spbook.book) {
        context.spbook.book = (struct obj *) 0;
        context.spbook.o_id = 0;
    }
}

/* renaming an object usually results in it having a different address;
   so the sequence start reading, get interrupted, name the book, resume
   reading would read the "new" book from scratch */
void book_substitution(old_obj, new_obj) struct obj *old_obj, *new_obj;
{
    if (old_obj == context.spbook.book) {
        context.spbook.book = new_obj;
        if (context.spbook.book)
            context.spbook.o_id = context.spbook.book->o_id;
    }
}

/* called from moveloop() */
void
age_spells()
{
    int i;
    /*
     * The time relative to the hero (a pass through move
     * loop) causes all spell knowledge to be decremented.
     * The hero's speed, rest status, conscious status etc.
     * does not alter the loss of memory.
     */
    for (i = 0; i < MAXSPELL && spellid(i) != NO_SPELL; i++)
        if (spellknow(i))
            decrnknow(i);
    return;
}

/* return True if spellcasting is inhibited;
   only covers a small subset of reasons why casting won't work */
STATIC_OVL boolean
rejectcasting()
{
    /* rejections which take place before selecting a particular spell */
    if (Stunned) {
        /*KR You("are too impaired to cast a spell."); */
        You("너무 어지러워서 주문을 시전할 수 없다.");
        return TRUE;
    } else if (!can_chant(&youmonst)) {
        /*KR You("are unable to chant the incantation."); */
        You("주문을 읊조릴 수 없다.");
        return TRUE;
    } else if (!freehand()) {
        /* Note: !freehand() occurs when weapon and shield (or two-handed
         * weapon) are welded to hands, so "arms" probably doesn't need
         * to be makeplural(bodypart(ARM)).
         *
         * But why isn't lack of free arms (for gesturing) an issue when
         * poly'd hero has no limbs?
         */
        /*KR Your("arms are not free to cast!"); */
        Your("주문을 시전할 만큼 팔이 자유롭지 않다!");
        return TRUE;
    }
    return FALSE;
}

/*
 * Return TRUE if a spell was picked, with the spell index in the return
 * parameter.  Otherwise return FALSE.
 */
STATIC_OVL boolean
getspell(spell_no)
int *spell_no;
{
    int nspells, idx;
    char ilet, lets[BUFSZ], qbuf[QBUFSZ];

    if (spellid(0) == NO_SPELL) {
        /*KR You("don't know any spells right now."); */
        You("지금은 아무런 주문도 모른다.");
        return FALSE;
    }
    if (rejectcasting())
        return FALSE; /* no spell chosen */

    if (flags.menu_style == MENU_TRADITIONAL) {
        /* we know there is at least 1 known spell */
        for (nspells = 1; nspells < MAXSPELL && spellid(nspells) != NO_SPELL;
             nspells++)
            continue;

        if (nspells == 1)
            Strcpy(lets, "a");
        else if (nspells < 27)
            Sprintf(lets, "a-%c", 'a' + nspells - 1);
        else if (nspells == 27)
            Sprintf(lets, "a-zA");
        /* this assumes that there are at most 52 spells... */
        else
            Sprintf(lets, "a-zA-%c", 'A' + nspells - 27);

        for (;;) {
            /*KR Sprintf(qbuf, "Cast which spell? [%s *?]", lets); */
            Sprintf(qbuf, "어떤 주문을 시전하시겠습니까? [%s *?]", lets);
            ilet = yn_function(qbuf, (char *) 0, '\0');
            if (ilet == '*' || ilet == '?')
                break; /* use menu mode */
            if (index(quitchars, ilet))
                return FALSE;

            idx = spell_let_to_idx(ilet);
            if (idx < 0 || idx >= nspells) {
                /*KR You("don't know that spell."); */
                You("그런 주문은 모른다.");
                continue; /* ask again */
            }
            *spell_no = idx;
            return TRUE;
        }
    }
    /*KR return dospellmenu("Choose which spell to cast", SPELLMENU_CAST,
                       spell_no); */
    return dospellmenu("시전할 주문을 선택하세요", SPELLMENU_CAST, spell_no);
}

/* the 'Z' command -- cast a spell */
int
docast()
{
    int spell_no;

    if (getspell(&spell_no))
        return spelleffects(spell_no, FALSE);
    return 0;
}

STATIC_OVL const char *
spelltypemnemonic(skill)
int skill;
{
    switch (skill) {
    case P_ATTACK_SPELL:
        /*KR return "attack"; */
        return "공격";
    case P_HEALING_SPELL:
        /*KR return "healing"; */
        return "치유";
    case P_DIVINATION_SPELL:
        /*KR return "divination"; */
        return "점술";
    case P_ENCHANTMENT_SPELL:
        /*KR return "enchantment"; */
        return "부여";
    case P_CLERIC_SPELL:
        /*KR return "clerical"; */
        return "성직";
    case P_ESCAPE_SPELL:
        /*KR return "escape"; */
        return "탈출";
    case P_MATTER_SPELL:
        /*KR return "matter"; */
        return "물질";
    default:
        impossible("Unknown spell skill, %d;", skill);
        return "";
    }
}

int
spell_skilltype(booktype)
int booktype;
{
    return objects[booktype].oc_skill;
}

STATIC_OVL void
cast_protection()
{
    int l = u.ulevel, loglev = 0, gain, natac = u.uac + u.uspellprot;
    /* note: u.uspellprot is subtracted when find_ac() factors it into u.uac,
       so adding here factors it back out
       (versions prior to 3.6 had this backwards) */

    /* loglev=log2(u.ulevel)+1 (1..5) */
    while (l) {
        loglev++;
        l /= 2;
    }

    /* The more u.uspellprot you already have, the less you get,
     * and the better your natural ac, the less you get.
     *
     * LEVEL AC    SPELLPROT from successive SPE_PROTECTION casts
     * 1     10    0,  1,  2,  3,  4
     * 1      0    0,  1,  2,  3
     * 1    -10    0,  1,  2
     * 2-3   10    0,  2,  4,  5,  6,  7,  8
     * 2-3    0    0,  2,  4,  5,  6
     * 2-3  -10    0,  2,  3,  4
     * 4-7   10    0,  3,  6,  8,  9, 10, 11, 12
     * 4-7    0    0,  3,  5,  7,  8,  9
     * 4-7  -10    0,  3,  5,  6
     * 7-15 -10    0,  3,  5,  6
     * 8-15  10    0,  4,  7, 10, 12, 13, 14, 15, 16
     * 8-15   0    0,  4,  7,  9, 10, 11, 12
     * 8-15 -10    0,  4,  6,  7,  8
     * 16-30  10    0,  5,  9, 12, 14, 16, 17, 18, 19, 20
     * 16-30   0    0,  5,  9, 11, 13, 14, 15
     * 16-30 -10    0,  5,  8,  9, 10
     */
    natac = (10 - natac) / 10; /* convert to positive and scale down */
    gain = loglev - (int) u.uspellprot / (4 - min(3, natac));

    if (gain > 0) {
        if (!Blind) {
            int rmtyp;
            const char *hgolden = hcolor(NH_GOLDEN), *atmosphere;

            if (u.uspellprot) {
                /*KR pline_The("%s haze around you becomes more dense.",
                 * hgolden); */
                pline("당신 주변의 %s 아지랑이가 더 짙어졌다.", hgolden);
            } else {
                rmtyp = levl[u.ux][u.uy].typ;
                atmosphere = u.uswallow
                                 ? ((u.ustuck->data == &mons[PM_FOG_CLOUD])
                                        /*KR ? "mist" */
                                        ? "안개"
                                        : is_whirly(u.ustuck->data)
                                              /*KR ? "maelstrom" */
                                              ? "소용돌이"
                                              : is_animal(u.ustuck->data)
                                                    /*KR ? "maw" */
                                                    ? "위장"
                                                    /*KR : "ooze") */
                                                    : "진흙")
                                 : (u.uinwater
                                        /*KR ? hliquid("water") */
                                        ? hliquid("물")
                                        : (rmtyp == CLOUD)
                                              /*KR ? "cloud" */
                                              ? "구름"
                                              : IS_TREE(rmtyp)
                                                    /*KR ? "vegetation" */
                                                    ? "초목"
                                                    : IS_STWALL(rmtyp)
                                                          /*KR ? "stone" */
                                                          ? "암석"
                                                          /*KR : "air"); */
                                                          : "공기");
#if 0 /*KR: 원본*/
                pline_The("%s around you begins to shimmer with %s haze.",
                          atmosphere, an(hgolden));
#else /*KR: KRNethack 맞춤 번역 */
                pline("당신 주변의 %s %s 아지랑이로 일렁이기 시작했다.",
                      append_josa(atmosphere, "이"), an(hgolden));
#endif
            }
        }
        u.uspellprot += gain;
        u.uspmtime =
            (P_SKILL(spell_skilltype(SPE_PROTECTION)) == P_EXPERT) ? 20 : 10;
        if (!u.usptime)
            u.usptime = u.uspmtime;
        find_ac();
    } else {
        /*KR Your("skin feels warm for a moment."); */
        Your("피부가 잠시 따뜻해지는 느낌이다.");
    }
}

/* attempting to cast a forgotten spell will cause disorientation */
STATIC_OVL void spell_backfire(spell) int spell;
{
    long duration = (long) ((spellev(spell) + 1) * 3), /* 6..24 */
        old_stun = (HStun & TIMEOUT), old_conf = (HConfusion & TIMEOUT);

    /* Prior to 3.4.1, only effect was confusion; it still predominates.
     *
     * 3.6.0: this used to override pre-existing confusion duration
     * (cases 0..8) and pre-existing stun duration (cases 4..9);
     * increase them instead.   (Hero can no longer cast spells while
     * Stunned, so the potential increment to stun duration here is
     * just hypothetical.)
     */
    switch (rn2(10)) {
    case 0:
    case 1:
    case 2:
    case 3:
        make_confused(old_conf + duration, FALSE); /* 40% */
        break;
    case 4:
    case 5:
    case 6:
        make_confused(old_conf + 2L * duration / 3L, FALSE); /* 30% */
        make_stunned(old_stun + duration / 3L, FALSE);
        break;
    case 7:
    case 8:
        make_stunned(old_stun + 2L * duration / 3L, FALSE); /* 20% */
        make_confused(old_conf + duration / 3L, FALSE);
        break;
    case 9:
        make_stunned(old_stun + duration, FALSE); /* 10% */
        break;
    }
    return;
}

int
spelleffects(spell, atme)
int spell;
boolean atme;
{
    int energy, damage, chance, n, intell;
    int otyp, skill, role_skill, res = 0;
    boolean confused = (Confusion != 0);
    boolean physical_damage = FALSE;
    struct obj *pseudo;
    coord cc;

    /*
     * Reject attempting to cast while stunned or with no free hands.
     * Already done in getspell() to stop casting before choosing
     * which spell, but duplicated here for cases where spelleffects()
     * gets called directly for ^T without intrinsic teleport capability
     * or #turn for non-priest/non-knight.
     * (There's no duplication of messages; when the rejection takes
     * place in getspell(), we don't get called.)
     */
    if (rejectcasting()) {
        return 0; /* no time elapses */
    }

    /*
     * Spell casting no longer affects knowledge of the spell. A
     * decrement of spell knowledge is done every turn.
     */
    if (spellknow(spell) <= 0) {
        /*KR Your("knowledge of this spell is twisted."); */
        Your("이 주문에 대한 당신의 지식이 뒤틀렸다.");
        /*KR pline("It invokes nightmarish images in your mind..."); */
        pline("당신의 마음에 악몽 같은 심상들을 불러일으킨다...");
        spell_backfire(spell);
        return 1;
    } else if (spellknow(spell) <= KEEN / 200) { /* 100 turns left */
        /*KR You("strain to recall the spell."); */
        You("그 주문을 떠올리려고 몹시 애쓴다.");
    } else if (spellknow(spell) <= KEEN / 40) { /* 500 turns left */
        /*KR You("have difficulty remembering the spell."); */
        You("그 주문을 기억해 내기가 어렵다.");
    } else if (spellknow(spell) <= KEEN / 20) { /* 1000 turns left */
        /*KR Your("knowledge of this spell is growing faint."); */
        Your("이 주문에 대한 당신의 지식이 점점 흐릿해지고 있다.");
    } else if (spellknow(spell) <= KEEN / 10) { /* 2000 turns left */
        /*KR Your("recall of this spell is gradually fading."); */
        Your("이 주문에 대한 당신의 기억이 서서히 옅어지고 있다.");
    }
    /*
     * Note: dotele() also calculates energy use and checks nutrition
     * and strength requirements; it any of these change, update it too.
     */
    energy = (spellev(spell) * 5); /* 5 <= energy <= 35 */

    if (u.uhunger <= 10 && spellid(spell) != SPE_DETECT_FOOD) {
        /*KR You("are too hungry to cast that spell."); */
        You("너무 배가 고파서 그 주문을 시전할 수 없다.");
        return 0;
    } else if (ACURR(A_STR) < 4 && spellid(spell) != SPE_RESTORE_ABILITY) {
        /*KR You("lack the strength to cast spells."); */
        You("주문을 시전할 힘이 부족하다.");
        return 0;
    } else if (check_capacity(
                   /*KR "Your concentration falters while carrying so much
                      stuff.")) { */
                   "너무 많은 물건을 들고 있어 집중이 흐트러진다.")) {
        return 1;
    }

    /* if the cast attempt is already going to fail due to insufficient
       energy (ie, u.uen < energy), the Amulet's drain effect won't kick
       in and no turn will be consumed; however, when it does kick in,
       the attempt may fail due to lack of energy after the draining, in
       which case a turn will be used up in addition to the energy loss */
    if (u.uhave.amulet && u.uen >= energy) {
        /*KR You_feel("the amulet draining your energy away."); */
        You_feel("부적이 당신의 에너지를 앗아가는 것을 느낀다.");
        /* this used to be 'energy += rnd(2 * energy)' (without 'res'),
           so if amulet-induced cost was more than u.uen, nothing
           (except the "don't have enough energy" message) happened
           and player could just try again (and again and again...);
           now we drain some energy immediately, which has a
           side-effect of not increasing the hunger aspect of casting */
        u.uen -= rnd(2 * energy);
        if (u.uen < 0)
            u.uen = 0;
        context.botl = 1;
        res = 1; /* time is going to elapse even if spell doesn't get cast */
    }

    if (energy > u.uen) {
        /*KR You("don't have enough energy to cast that spell."); */
        You("그 주문을 시전할 마력이 부족하다.");
        return res;
    } else {
        if (spellid(spell) != SPE_DETECT_FOOD) {
            int hungr = energy * 2;

            /* If hero is a wizard, their current intelligence
             * (bonuses + temporary + current)
             * affects hunger reduction in casting a spell.
             * 1. int = 17-18 no reduction
             * 2. int = 16    1/4 hungr
             * 3. int = 15    1/2 hungr
             * 4. int = 1-14  normal reduction
             * The reason for this is:
             * a) Intelligence affects the amount of exertion
             * in thinking.
             * b) Wizards have spent their life at magic and
             * understand quite well how to cast spells.
             */
            intell = acurr(A_INT);
            if (!Role_if(PM_WIZARD))
                intell = 10;
            switch (intell) {
            case 25:
            case 24:
            case 23:
            case 22:
            case 21:
            case 20:
            case 19:
            case 18:
            case 17:
                hungr = 0;
                break;
            case 16:
                hungr /= 4;
                break;
            case 15:
                hungr /= 2;
                break;
            }
            /* don't put player (quite) into fainting from
             * casting a spell, particularly since they might
             * not even be hungry at the beginning; however,
             * this is low enough that they must eat before
             * casting anything else except detect food
             */
            if (hungr > u.uhunger - 3)
                hungr = u.uhunger - 3;
            morehungry(hungr);
        }
    }

    chance = percent_success(spell);
    if (confused || (rnd(100) > chance)) {
        /*KR You("fail to cast the spell correctly."); */
        You("주문을 올바르게 시전하는 데 실패했다.");
        u.uen -= energy / 2;
        context.botl = 1;
        return 1;
    }

    u.uen -= energy;
    context.botl = 1;
    exercise(A_WIS, TRUE);
    /* pseudo is a temporary "false" object containing the spell stats */
    pseudo = mksobj(spellid(spell), FALSE, FALSE);
    pseudo->blessed = pseudo->cursed = 0;
    pseudo->quan = 20L; /* do not let useup get it */
    /*
     * Find the skill the hero has in a spell type category.
     * See spell_skilltype for categories.
     */
    otyp = pseudo->otyp;
    skill = spell_skilltype(otyp);
    role_skill = P_SKILL(skill);

    switch (otyp) {
    /*
     * At first spells act as expected.  As the hero increases in skill
     * with the appropriate spell type, some spells increase in their
     * effects, e.g. more damage, further distance, and so on, without
     * additional cost to the spellcaster.
     */
    case SPE_FIREBALL:
    case SPE_CONE_OF_COLD:
        if (role_skill >= P_SKILLED) {
            if (throwspell()) {
                cc.x = u.dx;
                cc.y = u.dy;
                n = rnd(8) + 1;
                while (n--) {
                    if (!u.dx && !u.dy && !u.dz) {
                        if ((damage = zapyourself(pseudo, TRUE)) != 0) {
                            char buf[BUFSZ];
#if 0 /*KR: 원본*/
                            Sprintf(buf, "zapped %sself with a spell",
                                    uhim());
                            losehp(damage, buf, NO_KILLER_PREFIX);
#else /*KR: KRNethack 맞춤 번역 */
                            Strcpy(buf, "자신의 마법을 맞고");
                            losehp(damage, buf, KILLED_BY);
#endif
                        }
                    } else {
                        explode(u.dx, u.dy, otyp - SPE_MAGIC_MISSILE + 10,
                                spell_damage_bonus(u.ulevel / 2 + 1), 0,
                                (otyp == SPE_CONE_OF_COLD) ? EXPL_FROSTY
                                                           : EXPL_FIERY);
                    }
                    u.dx = cc.x + rnd(3) - 2;
                    u.dy = cc.y + rnd(3) - 2;
                    if (!isok(u.dx, u.dy) || !cansee(u.dx, u.dy)
                        || IS_STWALL(levl[u.dx][u.dy].typ) || u.uswallow) {
                        /* Spell is reflected back to center */
                        u.dx = cc.x;
                        u.dy = cc.y;
                    }
                }
            }
            break;
        } /* else */
        /*FALLTHRU*/

    /* these spells are all duplicates of wand effects */
    case SPE_FORCE_BOLT:
        physical_damage = TRUE;
    /*FALLTHRU*/
    case SPE_SLEEP:
    case SPE_MAGIC_MISSILE:
    case SPE_KNOCK:
    case SPE_SLOW_MONSTER:
    case SPE_WIZARD_LOCK:
    case SPE_DIG:
    case SPE_TURN_UNDEAD:
    case SPE_POLYMORPH:
    case SPE_TELEPORT_AWAY:
    case SPE_CANCELLATION:
    case SPE_FINGER_OF_DEATH:
    case SPE_LIGHT:
    case SPE_DETECT_UNSEEN:
    case SPE_HEALING:
    case SPE_EXTRA_HEALING:
    case SPE_DRAIN_LIFE:
    case SPE_STONE_TO_FLESH:
        if (objects[otyp].oc_dir != NODIR) {
            if (otyp == SPE_HEALING || otyp == SPE_EXTRA_HEALING) {
                /* healing and extra healing are actually potion effects,
                   but they've been extended to take a direction like wands */
                if (role_skill >= P_SKILLED)
                    pseudo->blessed = 1;
            }
            if (atme) {
                u.dx = u.dy = u.dz = 0;
            } else if (!getdir((char *) 0)) {
                /* getdir cancelled, re-use previous direction */
                /*
                 * FIXME:  reusing previous direction only makes sense
                 * if there is an actual previous direction.  When there
                 * isn't one, the spell gets cast at self which is rarely
                 * what the player intended.  Unfortunately, the way
                 * spelleffects() is organized means that aborting with
                 * "nevermind" is not an option.
                 */
                /*KR pline_The("magical energy is released!"); */
                pline("마법의 에너지가 풀려났다!");
            }
            if (!u.dx && !u.dy && !u.dz) {
                if ((damage = zapyourself(pseudo, TRUE)) != 0) {
                    char buf[BUFSZ];

                    /*KR Sprintf(buf, "zapped %sself with a spell", uhim());
                     */
                    Strcpy(buf, "자신의 마법을 맞고");
                    if (physical_damage)
                        damage = Maybe_Half_Phys(damage);
#if 0 /*KR: 원본*/
                    losehp(damage, buf, NO_KILLER_PREFIX);
#else /*KR: KRNethack 맞춤 번역 */
                    losehp(damage, buf, KILLED_BY);
#endif
                }
            } else
                weffects(pseudo);
        } else
            weffects(pseudo);
        update_inventory(); /* spell may modify inventory */
        break;

    /* these are all duplicates of scroll effects */
    case SPE_REMOVE_CURSE:
    case SPE_CONFUSE_MONSTER:
    case SPE_DETECT_FOOD:
    case SPE_CAUSE_FEAR:
    case SPE_IDENTIFY:
        /* high skill yields effect equivalent to blessed scroll */
        if (role_skill >= P_SKILLED)
            pseudo->blessed = 1;
    /*FALLTHRU*/
    case SPE_CHARM_MONSTER:
    case SPE_MAGIC_MAPPING:
    case SPE_CREATE_MONSTER:
        (void) seffects(pseudo);
        break;

    /* these are all duplicates of potion effects */
    case SPE_HASTE_SELF:
    case SPE_DETECT_TREASURE:
    case SPE_DETECT_MONSTERS:
    case SPE_LEVITATION:
    case SPE_RESTORE_ABILITY:
        /* high skill yields effect equivalent to blessed potion */
        if (role_skill >= P_SKILLED)
            pseudo->blessed = 1;
    /*FALLTHRU*/
    case SPE_INVISIBILITY:
        (void) peffects(pseudo);
        break;
        /* end of potion-like spells */

    case SPE_CURE_BLINDNESS:
        healup(0, 0, FALSE, TRUE);
        break;
    case SPE_CURE_SICKNESS:
        if (Sick)
            /*KR You("are no longer ill."); */
            You("더 이상 아프지 않다.");
        if (Slimed)
            /*KR make_slimed(0L, "The slime disappears!"); */
            make_slimed(0L, "슬라임이 사라졌다!");
        healup(0, 0, TRUE, FALSE);
        break;
    case SPE_CREATE_FAMILIAR:
        (void) make_familiar((struct obj *) 0, u.ux, u.uy, FALSE);
        break;
    case SPE_CLAIRVOYANCE:
        if (!BClairvoyant) {
            if (role_skill >= P_SKILLED)
                pseudo->blessed = 1; /* detect monsters as well as map */
            do_vicinity_map(pseudo);
            /* at present, only one thing blocks clairvoyance */
        } else if (uarmh && uarmh->otyp == CORNUTHAUM)
            /*KR You("sense a pointy hat on top of your %s.",
             * body_part(HEAD)); */
            You("당신의 %s 위로 뾰족한 모자가 느껴진다.", body_part(HEAD));
        break;
    case SPE_PROTECTION:
        cast_protection();
        break;
    case SPE_JUMPING:
        if (!jump(max(role_skill, 1)))
            pline1(nothing_happens);
        break;
    default:
        impossible("Unknown spell %d attempted.", spell);
        obfree(pseudo, (struct obj *) 0);
        return 0;
    }

    /* gain skill for successful cast */
    use_skill(skill, spellev(spell));

    obfree(pseudo, (struct obj *) 0); /* now, get rid of it */
    return 1;
}

/*ARGSUSED*/
STATIC_OVL boolean
spell_aim_step(arg, x, y)
genericptr_t arg UNUSED;
int x, y;
{
    if (!isok(x, y))
        return FALSE;
    if (!ZAP_POS(levl[x][y].typ)
        && !(IS_DOOR(levl[x][y].typ) && (levl[x][y].doormask & D_ISOPEN)))
        return FALSE;
    return TRUE;
}

/* Choose location where spell takes effect. */
STATIC_OVL int
throwspell()
{
    coord cc, uc;
    struct monst *mtmp;

    if (u.uinwater) {
        /*KR pline("You're joking!  In this weather?"); */
        pline("농담하는 거지! 이런 날씨에?");
        return 0;
    } else if (Is_waterlevel(&u.uz)) {
        /*KR You("had better wait for the sun to come out."); */
        You("해가 뜰 때까지 기다리는 게 좋겠다.");
        return 0;
    }

    /*KR pline("Where do you want to cast the spell?"); */
    pline("어디에 주문을 시전하시겠습니까?");
    cc.x = u.ux;
    cc.y = u.uy;
    /*KR if (getpos(&cc, TRUE, "the desired position") < 0) */
    if (getpos(&cc, TRUE, "원하는 위치") < 0)
        return 0; /* user pressed ESC */
    /* The number of moves from hero to where the spell drops.*/
    if (distmin(u.ux, u.uy, cc.x, cc.y) > 10) {
        /*KR pline_The("spell dissipates over the distance!"); */
        pline("거리가 멀어 주문이 흩어져버렸다!");
        return 0;
    } else if (u.uswallow) {
        /*KR pline_The("spell is cut short!"); */
        pline("주문이 끊어져버렸다!");
        exercise(A_WIS, FALSE); /* What were you THINKING! */
        u.dx = 0;
        u.dy = 0;
        return 1;
    } else if ((!cansee(cc.x, cc.y)
                && (!(mtmp = m_at(cc.x, cc.y)) || !canspotmon(mtmp)))
               || IS_STWALL(levl[cc.x][cc.y].typ)) {
        /*KR Your("mind fails to lock onto that location!"); */
        Your("정신이 그 위치를 포착하는 데 실패했다!");
        return 0;
    }

    uc.x = u.ux;
    uc.y = u.uy;

    walk_path(&uc, &cc, spell_aim_step, (genericptr_t) 0);

    u.dx = cc.x;
    u.dy = cc.y;
    return 1;
}

/* add/hide/remove/unhide teleport-away on behalf of dotelecmd() to give
   more control to behavior of ^T when used in wizard mode */
int
tport_spell(what)
int what;
{
    static struct tport_hideaway {
        struct spell savespell;
        int tport_indx;
    } save_tport;
    int i;
/* also defined in teleport.c */
#define NOOP_SPELL 0
#define HIDE_SPELL 1
#define ADD_SPELL 2
#define UNHIDESPELL 3
#define REMOVESPELL 4

    for (i = 0; i < MAXSPELL; i++)
        if (spellid(i) == SPE_TELEPORT_AWAY || spellid(i) == NO_SPELL)
            break;
    if (i == MAXSPELL) {
        impossible("tport_spell: spellbook full");
        /* wizard mode ^T is not able to honor player's menu choice */
    } else if (spellid(i) == NO_SPELL) {
        if (what == HIDE_SPELL || what == REMOVESPELL) {
            save_tport.tport_indx = MAXSPELL;
        } else if (what == UNHIDESPELL) {
            /*assert( save_tport.savespell.sp_id == SPE_TELEPORT_AWAY );*/
            spl_book[save_tport.tport_indx] = save_tport.savespell;
            save_tport.tport_indx = MAXSPELL; /* burn bridge... */
        } else if (what == ADD_SPELL) {
            save_tport.savespell = spl_book[i];
            save_tport.tport_indx = i;
            spl_book[i].sp_id = SPE_TELEPORT_AWAY;
            spl_book[i].sp_lev = objects[SPE_TELEPORT_AWAY].oc_level;
            spl_book[i].sp_know = KEEN;
            return REMOVESPELL; /* operation needed to reverse */
        }
    } else { /* spellid(i) == SPE_TELEPORT_AWAY */
        if (what == ADD_SPELL || what == UNHIDESPELL) {
            save_tport.tport_indx = MAXSPELL;
        } else if (what == REMOVESPELL) {
            /*assert( i == save_tport.tport_indx );*/
            spl_book[i] = save_tport.savespell;
            save_tport.tport_indx = MAXSPELL;
        } else if (what == HIDE_SPELL) {
            save_tport.savespell = spl_book[i];
            save_tport.tport_indx = i;
            spl_book[i].sp_id = NO_SPELL;
            return UNHIDESPELL; /* operation needed to reverse */
        }
    }
    return NOOP_SPELL;
}

/* forget a random selection of known spells due to amnesia;
   they used to be lost entirely, as if never learned, but now we
   just set the memory retention to zero so that they can't be cast */
void
losespells()
{
    int n, nzap, i;

    /* in case reading has been interrupted earlier, discard context */
    context.spbook.book = 0;
    context.spbook.o_id = 0;
    /* count the number of known spells */
    for (n = 0; n < MAXSPELL; ++n)
        if (spellid(n) == NO_SPELL)
            break;

    /* lose anywhere from zero to all known spells;
       if confused, use the worse of two die rolls */
    nzap = rn2(n + 1);
    if (Confusion) {
        i = rn2(n + 1);
        if (i > nzap)
            nzap = i;
    }
    /* good Luck might ameliorate spell loss */
    if (nzap > 1 && !rnl(7))
        nzap = rnd(nzap);

    /*
     * Forget 'nzap' out of 'n' known spells by setting their memory
     * retention to zero.  Every spell has the same probability to be
     * forgotten, even if its retention is already zero.
     *
     * Perhaps we should forget the corresponding book too?
     *
     * (3.4.3 removed spells entirely from the list, but always did
     * so from its end, so the 'nzap' most recently learned spells
     * were the ones lost by default.  Player had sort control over
     * the list, so could move the most useful spells to front and
     * only lose them if 'nzap' turned out to be a large value.
     *
     * Discarding from the end of the list had the virtue of making
     * casting letters for lost spells become invalid and retaining
     * the original letter for the ones which weren't lost, so there
     * was no risk to the player of accidentally casting the wrong
     * spell when using a letter that was in use prior to amnesia.
     * That wouldn't be the case if we implemented spell loss spread
     * throughout the list of known spells; every spell located past
     * the first lost spell would end up with new letter assigned.)
     */
    for (i = 0; nzap > 0; ++i) {
        /* when nzap is small relative to the number of spells left,
           the chance to lose spell [i] is small; as the number of
           remaining candidates shrinks, the chance per candidate
           gets bigger; overall, exactly nzap entries are affected */
        if (rn2(n - i) < nzap) {
            /* lose access to spell [i] */
            spellknow(i) = 0;
#if 0
            /* also forget its book */
            forget_single_object(spellid(i));
#endif
            /* and abuse wisdom */
            exercise(A_WIS, FALSE);
            /* there's now one less spell slated to be forgotten */
            --nzap;
        }
    }
}

/*
 * Allow player to sort the list of known spells.  Manually swapping
 * pairs of them becomes very tedious once the list reaches two pages.
 *
 * Possible extensions:
 * provide means for player to control ordering of skill classes;
 * provide means to supply value N such that first N entries stick
 * while rest of list is being sorted;
 * make chosen sort order be persistent such that when new spells
 * are learned, they get inserted into sorted order rather than be
 * appended to the end of the list?
 */
enum spl_sort_types {
    SORTBY_LETTER = 0,
    SORTBY_ALPHA,
    SORTBY_LVL_LO,
    SORTBY_LVL_HI,
    SORTBY_SKL_AL,
    SORTBY_SKL_LO,
    SORTBY_SKL_HI,
    SORTBY_CURRENT,
    SORTRETAINORDER,

    NUM_SPELL_SORTBY
};

static const char *spl_sortchoices[NUM_SPELL_SORTBY] = {
    /*KR "by casting letter", */
    "시전 문자 순으로",
    /*KR "alphabetically", */
    "알파벳 순으로",
    /*KR "by level, low to high", */
    "레벨 순으로 (낮은 순서)",
    /*KR "by level, high to low", */
    "레벨 순으로 (높은 순서)",
    /*KR "by skill group, alphabetized within each group", */
    "스킬 그룹별로, 그룹 내 알파벳 순",
    /*KR "by skill group, low to high level within group", */
    "스킬 그룹별로, 그룹 내 레벨 낮은 순",
    /*KR "by skill group, high to low level within group", */
    "스킬 그룹별로, 그룹 내 레벨 높은 순",
    /*KR "maintain current ordering", */
    "현재 정렬 순서 유지",
    /* a menu choice rather than a sort choice */
    /*KR "reassign casting letters to retain current order", */
    "현재 순서를 유지하면서 시전 문자 재할당",
};
static int spl_sortmode = 0;   /* index into spl_sortchoices[] */
static int *spl_orderindx = 0; /* array of spl_book[] indices */

/* qsort callback routine */
STATIC_PTR int CFDECLSPEC spell_cmp(vptr1, vptr2) const genericptr vptr1;
const genericptr vptr2;
{
    /*
     * gather up all of the possible parameters except spell name
     * in advance, even though some might not be needed:
     * indx. = spl_orderindx[] index into spl_book[];
     * otyp. = spl_book[] index into objects[];
     * levl. = spell level;
     * skil. = skill group aka spell class.
     */
    int indx1 = *(int *) vptr1, indx2 = *(int *) vptr2,
        otyp1 = spl_book[indx1].sp_id, otyp2 = spl_book[indx2].sp_id,
        levl1 = objects[otyp1].oc_level, levl2 = objects[otyp2].oc_level,
        skil1 = objects[otyp1].oc_skill, skil2 = objects[otyp2].oc_skill;

    switch (spl_sortmode) {
    case SORTBY_LETTER:
        return indx1 - indx2;
    case SORTBY_ALPHA:
        break;
    case SORTBY_LVL_LO:
        if (levl1 != levl2)
            return levl1 - levl2;
        break;
    case SORTBY_LVL_HI:
        if (levl1 != levl2)
            return levl2 - levl1;
        break;
    case SORTBY_SKL_AL:
        if (skil1 != skil2)
            return skil1 - skil2;
        break;
    case SORTBY_SKL_LO:
        if (skil1 != skil2)
            return skil1 - skil2;
        if (levl1 != levl2)
            return levl1 - levl2;
        break;
    case SORTBY_SKL_HI:
        if (skil1 != skil2)
            return skil1 - skil2;
        if (levl1 != levl2)
            return levl2 - levl1;
        break;
    case SORTBY_CURRENT:
    default:
        return (vptr1 < vptr2) ? -1
                               : (vptr1 > vptr2); /* keep current order */
    }
    /* tie-breaker for most sorts--alphabetical by spell name */
    return strcmpi(OBJ_NAME(objects[otyp1]), OBJ_NAME(objects[otyp2]));
}

/* sort the index used for display order of the "view known spells"
   list (sortmode == SORTBY_xxx), or sort the spellbook itself to make
   the current display order stick (sortmode == SORTRETAINORDER) */
STATIC_OVL void
sortspells()
{
    int i;
#if defined(SYSV) || defined(DGUX)
    unsigned n;
#else
    int n;
#endif

    if (spl_sortmode == SORTBY_CURRENT)
        return;
    for (n = 0; n < MAXSPELL && spellid(n) != NO_SPELL; ++n)
        continue;
    if (n < 2)
        return; /* not enough entries to need sorting */

    if (!spl_orderindx) {
        /* we haven't done any sorting yet; list is in casting order */
        if (spl_sortmode == SORTBY_LETTER /* default */
            || spl_sortmode == SORTRETAINORDER)
            return;
        /* allocate enough for full spellbook rather than just N spells */
        spl_orderindx = (int *) alloc(MAXSPELL * sizeof(int));
        for (i = 0; i < MAXSPELL; i++)
            spl_orderindx[i] = i;
    }

    if (spl_sortmode == SORTRETAINORDER) {
        struct spell tmp_book[MAXSPELL];

        /* sort spl_book[] rather than spl_orderindx[];
           this also updates the index to reflect the new ordering (we
           could just free it since that ordering becomes the default) */
        for (i = 0; i < MAXSPELL; i++)
            tmp_book[i] = spl_book[spl_orderindx[i]];
        for (i = 0; i < MAXSPELL; i++)
            spl_book[i] = tmp_book[i], spl_orderindx[i] = i;
        spl_sortmode = SORTBY_LETTER; /* reset */
        return;
    }

    /* usual case, sort the index rather than the spells themselves */
    qsort((genericptr_t) spl_orderindx, n, sizeof *spl_orderindx, spell_cmp);
    return;
}

/* called if the [sort spells] entry in the view spells menu gets chosen */
STATIC_OVL boolean
spellsortmenu()
{
    winid tmpwin;
    menu_item *selected;
    anything any;
    char let;
    int i, n, choice;

    tmpwin = create_nhwindow(NHW_MENU);
    start_menu(tmpwin);
    any = zeroany; /* zero out all bits */

    for (i = 0; i < SIZE(spl_sortchoices); i++) {
        if (i == SORTRETAINORDER) {
            let = 'z'; /* assumes fewer than 26 sort choices... */
            /* separate final choice from others with a blank line */
            any.a_int = 0;
            add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, "",
                     MENU_UNSELECTED);
        } else {
            let = 'a' + i;
        }
        any.a_int = i + 1;
        add_menu(tmpwin, NO_GLYPH, &any, let, 0, ATR_NONE, spl_sortchoices[i],
                 (i == spl_sortmode) ? MENU_SELECTED : MENU_UNSELECTED);
    }
    /*KR end_menu(tmpwin, "View known spells list sorted"); */
    end_menu(tmpwin, "알고 있는 주문 목록 정렬 방식");

    n = select_menu(tmpwin, PICK_ONE, &selected);
    destroy_nhwindow(tmpwin);
    if (n > 0) {
        choice = selected[0].item.a_int - 1;
        /* skip preselected entry if we have more than one item chosen */
        if (n > 1 && choice == spl_sortmode)
            choice = selected[1].item.a_int - 1;
        free((genericptr_t) selected);
        spl_sortmode = choice;
        return TRUE;
    }
    return FALSE;
}

/* the '+' command -- view known spells */
int
dovspell()
{
    char qbuf[QBUFSZ];
    int splnum, othnum;
    struct spell spl_tmp;

    if (spellid(0) == NO_SPELL) {
        /*KR You("don't know any spells right now."); */
        You("지금은 아무런 주문도 모른다.");
    } else {
        /*KR while (dospellmenu("Currently known spells", */
        while (dospellmenu("현재 알고 있는 주문 목록", SPELLMENU_VIEW,
                           &splnum)) {
            if (splnum == SPELLMENU_SORT) {
                if (spellsortmenu())
                    sortspells();
            } else {
#if 0 /*KR: 원본*/
                Sprintf(qbuf, "Reordering spells; swap '%c' with",
                        spellet(splnum));
#else /*KR: KRNethack 맞춤 번역 */
                {
                    char temp_let[10];
                    /* 1. 단일 문자를 작은따옴표로 감싼 문자열로 만듭니다. */
                    Sprintf(temp_let, "'%c'", spellet(splnum));

                    /* 2. 문자열이 되었으므로 append_josa에 넣고 %s로
                     * 출력합니다. */
                    Sprintf(qbuf, "주문 재정렬: %s 바꿀 문자는?",
                            append_josa(temp_let, "와"));
                }
#endif
                if (!dospellmenu(qbuf, splnum, &othnum))
                    break;

                spl_tmp = spl_book[splnum];
                spl_book[splnum] = spl_book[othnum];
                spl_book[othnum] = spl_tmp;
            }
        }
    }
    if (spl_orderindx) {
        free((genericptr_t) spl_orderindx);
        spl_orderindx = 0;
    }
    spl_sortmode = SORTBY_LETTER; /* 0 */
    return 0;
}

STATIC_OVL boolean dospellmenu(prompt, splaction, spell_no) const
    char *prompt;
int splaction; /* SPELLMENU_CAST, SPELLMENU_VIEW, or spl_book[] index */
int *spell_no;
{
    winid tmpwin;
    int i, n, how, splnum;
    char buf[BUFSZ], retentionbuf[24];
    const char *fmt;
    menu_item *selected;
    anything any;

    tmpwin = create_nhwindow(NHW_MENU);
    start_menu(tmpwin);
    any = zeroany; /* zero out all bits */

    /*
     * The correct spacing of the columns when not using
     * tab separation depends on the following:
     * (1) that the font is monospaced, and
     * (2) that selection letters are pre-pended to the
     * given string and are of the form "a - ".
     */
    if (!iflags.menu_tab_sep) {
#if 0 /*KR: 원본*/
        Sprintf(buf, "%-20s      Level %-12s Fail Retention", "    Name",
                "Category");
#else /*KR: KRNethack 맞춤 번역 */
        Sprintf(buf, "%-20s      레벨 %-12s 실패 지속", "    이름", "분류");
#endif
        fmt = "%-20s  %2d   %-12s %3d%% %9s";
    } else {
        /*KR Sprintf(buf, "Name\tLevel\tCategory\tFail\tRetention"); */
        Sprintf(buf, "이름\t레벨\t분류\t실패율\t지속도");
        fmt = "%s\t%-d\t%s\t%-d%%\t%s";
    }
    add_menu(tmpwin, NO_GLYPH, &any, 0, 0, iflags.menu_headings, buf,
             MENU_UNSELECTED);
    for (i = 0; i < MAXSPELL && spellid(i) != NO_SPELL; i++) {
        splnum = !spl_orderindx ? i : spl_orderindx[i];
        Sprintf(buf, fmt, spellname(splnum), spellev(splnum),
                spelltypemnemonic(spell_skilltype(spellid(splnum))),
                100 - percent_success(splnum),
                spellretention(splnum, retentionbuf));

        any.a_int = splnum + 1; /* must be non-zero */
        add_menu(tmpwin, NO_GLYPH, &any, spellet(splnum), 0, ATR_NONE, buf,
                 (splnum == splaction) ? MENU_SELECTED : MENU_UNSELECTED);
    }
    how = PICK_ONE;
    if (splaction == SPELLMENU_VIEW) {
        if (spellid(1) == NO_SPELL) {
            /* only one spell => nothing to swap with */
            how = PICK_NONE;
        } else {
            /* more than 1 spell, add an extra menu entry */
            any.a_int = SPELLMENU_SORT + 1;
            add_menu(tmpwin, NO_GLYPH, &any, '+', 0, ATR_NONE,
                     /*KR "[sort spells]", MENU_UNSELECTED); */
                     "[주문 정렬]", MENU_UNSELECTED);
        }
    }
    end_menu(tmpwin, prompt);

    n = select_menu(tmpwin, how, &selected);
    destroy_nhwindow(tmpwin);
    if (n > 0) {
        *spell_no = selected[0].item.a_int - 1;
        /* menu selection for `PICK_ONE' does not
           de-select any preselected entry */
        if (n > 1 && *spell_no == splaction)
            *spell_no = selected[1].item.a_int - 1;
        free((genericptr_t) selected);
        /* default selection of preselected spell means that
           user chose not to swap it with anything */
        if (*spell_no == splaction)
            return FALSE;
        return TRUE;
    } else if (splaction >= 0) {
        /* explicit de-selection of preselected spell means that
           user is still swapping but not for the current spell */
        *spell_no = splaction;
        return TRUE;
    }
    return FALSE;
}

STATIC_OVL int
percent_success(spell)
int spell;
{
    /* Intrinsic and learned ability are combined to calculate
     * the probability of player's success at cast a given spell.
     */
    int chance, splcaster, special, statused;
    int difficulty;
    int skill;

    /* Calculate intrinsic ability (splcaster) */

    splcaster = urole.spelbase;
    special = urole.spelheal;
    statused = ACURR(urole.spelstat);

    if (uarm && is_metallic(uarm))
        splcaster += (uarmc && uarmc->otyp == ROBE) ? urole.spelarmr / 2
                                                    : urole.spelarmr;
    else if (uarmc && uarmc->otyp == ROBE)
        splcaster -= urole.spelarmr;
    if (uarms)
        splcaster += urole.spelshld;

    if (uarmh && is_metallic(uarmh) && uarmh->otyp != HELM_OF_BRILLIANCE)
        splcaster += uarmhbon;
    if (uarmg && is_metallic(uarmg))
        splcaster += uarmgbon;
    if (uarmf && is_metallic(uarmf))
        splcaster += uarmfbon;

    if (spellid(spell) == urole.spelspec)
        splcaster += urole.spelsbon;

    /* `healing spell' bonus */
    if (spellid(spell) == SPE_HEALING || spellid(spell) == SPE_EXTRA_HEALING
        || spellid(spell) == SPE_CURE_BLINDNESS
        || spellid(spell) == SPE_CURE_SICKNESS
        || spellid(spell) == SPE_RESTORE_ABILITY
        || spellid(spell) == SPE_REMOVE_CURSE)
        splcaster += special;

    if (splcaster > 20)
        splcaster = 20;

    /* Calculate learned ability */

    /* Players basic likelihood of being able to cast any spell
     * is based of their `magic' statistic. (Int or Wis)
     */
    chance = 11 * statused / 2;

    /*
     * High level spells are harder.  Easier for higher level casters.
     * The difficulty is based on the hero's level and their skill level
     * in that spell type.
     */
    skill = P_SKILL(spell_skilltype(spellid(spell)));
    skill = max(skill, P_UNSKILLED) - 1; /* unskilled => 0 */
    difficulty =
        (spellev(spell) - 1) * 4 - ((skill * 6) + (u.ulevel / 3) + 1);

    if (difficulty > 0) {
        /* Player is too low level or unskilled. */
        chance -= isqrt(900 * difficulty + 2000);
    } else {
        /* Player is above level.  Learning continues, but the
         * law of diminishing returns sets in quickly for
         * low-level spells.  That is, a player quickly gains
         * no advantage for raising level.
         */
        int learning = 15 * -difficulty / spellev(spell);
        chance += learning > 20 ? 20 : learning;
    }

    /* Clamp the chance: >18 stat and advanced learning only help
     * to a limit, while chances below "hopeless" only raise the
     * specter of overflowing 16-bit ints (and permit wearing a
     * shield to raise the chances :-).
     */
    if (chance < 0)
        chance = 0;
    if (chance > 120)
        chance = 120;

    /* Wearing anything but a light shield makes it very awkward
     * to cast a spell.  The penalty is not quite so bad for the
     * player's role-specific spell.
     */
    if (uarms && weight(uarms) > (int) objects[SMALL_SHIELD].oc_weight) {
        if (spellid(spell) == urole.spelspec) {
            chance /= 2;
        } else {
            chance /= 4;
        }
    }

    /* Finally, chance (based on player intell/wisdom and level) is
     * combined with ability (based on player intrinsics and
     * encumbrances).  No matter how intelligent/wise and advanced
     * a player is, intrinsics and encumbrance can prevent casting;
     * and no matter how able, learning is always required.
     */
    chance = chance * (20 - splcaster) / 15 - splcaster;

    /* Clamp to percentile */
    if (chance > 100)
        chance = 100;
    if (chance < 0)
        chance = 0;

    return chance;
}

STATIC_OVL char *
spellretention(idx, outbuf)
int idx;
char *outbuf;
{
    long turnsleft, percent, accuracy;
    int skill;

    skill = P_SKILL(spell_skilltype(spellid(idx)));
    skill = max(skill, P_UNSKILLED); /* restricted same as unskilled */
    turnsleft = spellknow(idx);
    *outbuf = '\0'; /* lint suppression */

    if (turnsleft < 1L) {
        /* spell has expired; hero can't successfully cast it anymore */
        /*KR Strcpy(outbuf, "(gone)"); */
        Strcpy(outbuf, "(망각함)");
    } else if (turnsleft >= (long) KEEN) {
        /* full retention, first turn or immediately after reading book */
        Strcpy(outbuf, "100%");
    } else {
        /*
         * Retention is displayed as a range of percentages of
         * amount of time left until memory of the spell expires;
         * the precision of the range depends upon hero's skill
         * in this spell.
         * expert:  2% intervals; 1-2,  3-4,  ...,  99-100;
         * skilled:  5% intervals; 1-5,  6-10, ...,  95-100;
         * basic: 10% intervals; 1-10, 11-20, ...,  91-100;
         * unskilled: 25% intervals; 1-25, 26-50, 51-75, 76-100.
         *
         * At the low end of each range, a value of N% really means
         * (N-1)%+1 through N%; so 1% is "greater than 0, at most 200".
         * KEEN is a multiple of 100; KEEN/100 loses no precision.
         */
        percent = (turnsleft - 1L) / ((long) KEEN / 100L) + 1L;
        accuracy = (skill == P_EXPERT)    ? 2L
                   : (skill == P_SKILLED) ? 5L
                   : (skill == P_BASIC)   ? 10L
                                          : 25L;
        /* round up to the high end of this range */
        percent = accuracy * ((percent - 1L) / accuracy + 1L);
        Sprintf(outbuf, "%ld%%-%ld%%", percent - accuracy + 1L, percent);
    }
    return outbuf;
}

/* Learn a spell during creation of the initial inventory */
void initialspell(obj) struct obj *obj;
{
    int i, otyp = obj->otyp;

    for (i = 0; i < MAXSPELL; i++)
        if (spellid(i) == NO_SPELL || spellid(i) == otyp)
            break;

    if (i == MAXSPELL) {
        impossible("Too many spells memorized!");
    } else if (spellid(i) != NO_SPELL) {
        /* initial inventory shouldn't contain duplicate spellbooks */
        impossible("Spell %s already known.", OBJ_NAME(objects[otyp]));
    } else {
        spl_book[i].sp_id = otyp;
        spl_book[i].sp_lev = objects[otyp].oc_level;
        incrnknow(i, 0);
    }
    return;
}

/*spell.c*/
