/* NetHack 3.6	write.c	$NHDT-Date: 1573346194 2019/11/10 00:36:34 $  $NHDT-Branch: NetHack-3.6 $:$NHDT-Revision: 1.20 $ */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

STATIC_DCL int FDECL(cost, (struct obj *) );
STATIC_DCL boolean FDECL(label_known, (int, struct obj *) );
STATIC_DCL char *FDECL(new_book_description, (int, char *) );

/*
 * returns basecost of a scroll or a spellbook
 */
STATIC_OVL int
cost(otmp)
register struct obj *otmp;
{
    if (otmp->oclass == SPBOOK_CLASS)
        return (10 * objects[otmp->otyp].oc_level);

    switch (otmp->otyp) {
#ifdef MAIL
    case SCR_MAIL:
        return 2;
#endif
    case SCR_LIGHT:
    case SCR_GOLD_DETECTION:
    case SCR_FOOD_DETECTION:
    case SCR_MAGIC_MAPPING:
    case SCR_AMNESIA:
    case SCR_FIRE:
    case SCR_EARTH:
        return 8;
    case SCR_DESTROY_ARMOR:
    case SCR_CREATE_MONSTER:
    case SCR_PUNISHMENT:
        return 10;
    case SCR_CONFUSE_MONSTER:
        return 12;
    case SCR_IDENTIFY:
        return 14;
    case SCR_ENCHANT_ARMOR:
    case SCR_REMOVE_CURSE:
    case SCR_ENCHANT_WEAPON:
    case SCR_CHARGING:
        return 16;
    case SCR_SCARE_MONSTER:
    case SCR_STINKING_CLOUD:
    case SCR_TAMING:
    case SCR_TELEPORTATION:
        return 20;
    case SCR_GENOCIDE:
        return 30;
    case SCR_BLANK_PAPER:
    default:
        /*KR impossible("You can't write such a weird scroll!"); */
        impossible("그런 이상한 두루마리는 쓸 수 없습니다!");
    }
    return 1000;
}

/* decide whether the hero knowns a particular scroll's label;
   unfortunately, we can't track things that haven't been added to
   the discoveries list and aren't present in current inventory,
   so some scrolls with ought to yield True will end up False */
STATIC_OVL boolean
label_known(scrolltype, objlist)
int scrolltype;
struct obj *objlist;
{
    struct obj *otmp;

    /* only scrolls */
    if (objects[scrolltype].oc_class != SCROLL_CLASS)
        return FALSE;
    /* type known implies full discovery; otherwise,
       user-assigned name implies partial discovery */
    if (objects[scrolltype].oc_name_known || objects[scrolltype].oc_uname)
        return TRUE;
    /* check inventory, including carried containers with known contents */
    for (otmp = objlist; otmp; otmp = otmp->nobj) {
        if (otmp->otyp == scrolltype && otmp->dknown)
            return TRUE;
        if (Has_contents(otmp) && otmp->cknown
            && label_known(scrolltype, otmp->cobj))
            return TRUE;
    }
    /* not found */
    return FALSE;
}

static NEARDATA const char write_on[] = { SCROLL_CLASS, SPBOOK_CLASS, 0 };

/* write -- applying a magic marker */
int
dowrite(pen)
register struct obj *pen;
{
    register struct obj *paper;
#if 0 /*KR: 미사용 */
    char namebuf[BUFSZ] = DUMMY, *nm, *bp;
#else
    char namebuf[BUFSZ] = DUMMY, *nm;
#endif
    register struct obj *new_obj;
    int basecost, actualcost;
    int curseval;
    char qbuf[QBUFSZ];
    int first, last, i, deferred, deferralchance;
    boolean by_descr = FALSE;
    const char *typeword;

    if (nohands(youmonst.data)) {
        /*KR You("need hands to be able to write!"); */
        You("글을 쓰려면 손이 필요하다!");
        return 0;
    } else if (Glib) {
#if 0 /*KR: 원본 (영어 동사형 매크로 제거 및 어순 변경)*/
        pline("%s from your %s.", Tobjnam(pen, "slip"),
              fingers_or_gloves(FALSE));
#else
        pline("%s 당신의 %s에서 미끄러졌다.", append_josa(xname(pen), "이"),
              fingers_or_gloves(FALSE));
#endif
        dropx(pen);
        return 1;
    }

    /* get paper to write on */
    /*KR paper = getobj(write_on, "write on"); */
    paper = getobj(write_on, "글씨를 쓸");
    if (!paper)
        return 0;
    /* can't write on a novel (unless/until it's been converted into a blank
       spellbook), but we want messages saying so to avoid "spellbook" */
#if 0 /*KR: 원본*/
    typeword = (paper->otyp == SPE_NOVEL)
                  ? "book"
                  : (paper->oclass == SPBOOK_CLASS)
                     ? "spellbook"
                     : "scroll";
#else
    typeword = (paper->otyp == SPE_NOVEL)        ? "책"
               : (paper->oclass == SPBOOK_CLASS) ? "마법서"
                                                 : "두루마리";
#endif

    if (Blind) {
        if (!paper->dknown) {
#if 0 /*KR: 원본*/
            You("don't know if that %s is blank or not.", typeword);
#else
            You("그 %s 백지인지 아닌지 모른다.", append_josa(typeword, "이"));
#endif
            return 0;
        } else if (paper->oclass == SPBOOK_CLASS) {
            /* can't write a magic book while blind */
#if 0 /*KR: 원본*/
            pline("%s can't create braille text.",
                  upstart(ysimple_name(pen)));
#else
            pline("%s 점자를 만들어낼 수는 없다.",
                  append_josa(upstart(ysimple_name(pen)), "은"));
#endif
            return 0;
        }
    }
    paper->dknown = 1;
    if (paper->otyp != SCR_BLANK_PAPER && paper->otyp != SPE_BLANK_PAPER) {
#if 0 /*KR: 원본*/
        pline("That %s is not blank!", typeword);
#else
        pline("그 %s 백지가 아니다!", append_josa(typeword, "은"));
#endif
        exercise(A_WIS, FALSE);
        return 1;
    }

    /* what to write */
#if 0 /*KR: 원본*/
    Sprintf(qbuf, "What type of %s do you want to write?", typeword);
#else
    Sprintf(qbuf, "어떤 종류의 %s 쓰시겠습니까?",
            append_josa(typeword, "를"));
#endif
    getlin(qbuf, namebuf);
    (void) mungspaces(namebuf); /* remove any excess whitespace */
    if (namebuf[0] == '\033' || !namebuf[0])
        return 1;
    nm = namebuf;

#if 0 /*KR: 원본 (한국어 어순에서는 "scroll " 등의 영어 접두사/접미사 필터링 \
         로직이 불필요함)*/
    if (!strncmpi(nm, "scroll ", 7))
        nm += 7;
    else if (!strncmpi(nm, "spellbook ", 10))
        nm += 10;
    if (!strncmpi(nm, "of ", 3))
        nm += 3;

    if ((bp = strstri(nm, " armour")) != 0) {
        (void) strncpy(bp, " armor ", 7); /* won't add '\0' */
        (void) mungspaces(bp + 1);        /* remove the extra space */
    }
#endif

    deferred = 0;       /* not any scroll or book */
    deferralchance = 0; /* incremented for each oc_uname match */
    first = bases[(int) paper->oclass];
    last = bases[(int) paper->oclass + 1] - 1;
    for (i = first; i <= last; i++) {
        /* extra shufflable descr not representing a real object */
        if (!OBJ_NAME(objects[i]))
            continue;

        if (!strcmpi(OBJ_NAME(objects[i]), nm))
            goto found;
        if (!strcmpi(OBJ_DESCR(objects[i]), nm)) {
            by_descr = TRUE;
            goto found;
        }
        /* user-assigned name might match real name of a later
           entry, so we don't simply use first match with it;
           also, player might assign same name multiple times
           and if so, we choose one of those matches randomly */
        if (objects[i].oc_uname
            && !strcmpi(objects[i].oc_uname, nm)
            /*
             * First match: chance incremented to 1,
             * !rn2(1) is 1, we remember i;
             * second match: chance incremented to 2,
             * !rn2(2) has 1/2 chance to replace i;
             * third match: chance incremented to 3,
             * !rn2(3) has 1/3 chance to replace i
             * and 2/3 chance to keep previous 50:50
             * choice; so on for higher match counts.
             */
            && !rn2(++deferralchance))
            deferred = i;
    }
    /* writing by user-assigned name is same as by description:
       fails for books, works for scrolls (having an assigned
       type name guarantees presence on discoveries list) */
    if (deferred) {
        i = deferred;
        by_descr = TRUE;
        goto found;
    }

#if 0 /*KR: 원본*/
    There("is no such %s!", typeword);
#else
    pline("그런 %s 없다!", append_josa(typeword, "은"));
#endif
    return 1;
found:

    if (i == SCR_BLANK_PAPER || i == SPE_BLANK_PAPER) {
        /*KR You_cant("write that!"); */
        You_cant("그것을 쓸 수는 없다!");
        /*KR pline("It's obscene!"); */
        pline("그건 외설스럽다!");
        return 1;
    } else if (i == SPE_BOOK_OF_THE_DEAD) {
        /*KR pline("No mere dungeon adventurer could write that."); */
        pline("일개 미궁 탐험가 따위가 적을 수 있는 것이 아니다.");
        return 1;
    } else if (by_descr && paper->oclass == SPBOOK_CLASS
               && !objects[i].oc_name_known) {
        /* can't write unknown spellbooks by description */
        /*KR pline("Unfortunately you don't have enough information to go
         * on."); */
        pline("불행히도 계속 써내려가기에 충분한 정보가 없다.");
        return 1;
    }

    /* KMH, conduct */
    u.uconduct.literate++;

    new_obj = mksobj(i, FALSE, FALSE);
    new_obj->bknown = (paper->bknown && pen->bknown);

    /* shk imposes a flat rate per use, not based on actual charges used */
    check_unpaid(pen);

    /* see if there's enough ink */
    basecost = cost(new_obj);
    if (pen->spe < basecost / 2) {
        /*KR Your("marker is too dry to write that!"); */
        Your("마커가 너무 말라붙어서 그것을 쓸 수 없다!");
        obfree(new_obj, (struct obj *) 0);
        return 1;
    }

    /* we're really going to write now, so calculate cost
     */
    actualcost = rn1(basecost / 2, basecost / 2);
    curseval = bcsign(pen) + bcsign(paper);
    exercise(A_WIS, TRUE);
    /* dry out marker */
    if (pen->spe < actualcost) {
        pen->spe = 0;
        /*KR Your("marker dries out!"); */
        Your("마커가 말라붙어버렸다!");
        /* scrolls disappear, spellbooks don't */
        if (paper->oclass == SPBOOK_CLASS) {
            /*KR pline_The("spellbook is left unfinished and your writing
             * fades."); */
            pline_The("마법서는 미완성으로 남았고, 당신이 쓴 글씨는 흐릿해져 "
                      "사라졌다.");
            update_inventory(); /* pen charges */
        } else {
            /*KR pline_The("scroll is now useless and disappears!"); */
            pline_The("두루마리는 이제 쓸모가 없어졌고, 사라져버렸다!");
            useup(paper);
        }
        obfree(new_obj, (struct obj *) 0);
        return 1;
    }
    pen->spe -= actualcost;

    /*
     * Writing by name requires that the hero knows the scroll or
     * book type.  One has previously been read (and its effect
     * was evident) or been ID'd via scroll/spell/throne and it
     * will be on the discoveries list.
     * (Previous versions allowed scrolls and books to be written
     * by type name if they were on the discoveries list via being
     * given a user-assigned name, even though doing the latter
     * doesn't--and shouldn't--make the actual type become known.)
     *
     * Writing by description requires that the hero knows the
     * description (a scroll's label, that is, since books by_descr
     * are rejected above).  BUG:  We can only do this for known
     * scrolls and for the case where the player has assigned a
     * name to put it onto the discoveries list; we lack a way to
     * track other scrolls which have been seen closely enough to
     * read the label without then being ID'd or named.  The only
     * exception is for currently carried inventory, where we can
     * check for one [with its dknown bit set] of the same type.
     *
     * Normal requirements can be overridden if hero is Lucky.
     */

    /* if known, then either by-name or by-descr works */
    if (!objects[new_obj->otyp].oc_name_known
        /* else if named, then only by-descr works */
        && !(by_descr && label_known(new_obj->otyp, invent))
        /* and Luck might override after both checks have failed */
        && rnl(Role_if(PM_WIZARD) ? 5 : 15)) {
#if 0 /*KR: 원본*/
        You("%s to write that.", by_descr ? "fail" : "don't know how");
#else
        You("그것을 %s.",
            by_descr ? "쓰는 데 실패했다" : "쓰는 방법을 모른다");
#endif
        /* scrolls disappear, spellbooks don't */
        if (paper->oclass == SPBOOK_CLASS) {
            /*KR You("write in your best handwriting:  \"My Diary\", but it
             * quickly fades."); */
            You("최고의 명필로 \"나의 일기\" 라고 적었지만, 곧 흐릿해져 "
                "사라졌다.");
            update_inventory(); /* pen charges */
        } else {
            if (by_descr) {
                Strcpy(namebuf, OBJ_DESCR(objects[new_obj->otyp]));
                wipeout_text(namebuf, (6 + MAXULEV - u.ulevel) / 6, 0);
            } else
                /*KR Sprintf(namebuf, "%s was here!", plname); */
                Sprintf(namebuf, "%s 다녀감!", plname);
            /*KR You("write \"%s\" and the scroll disappears.", namebuf); */
            You("\"%s\" 라고 적자, 두루마리가 사라졌다.", namebuf);
            useup(paper);
        }
        obfree(new_obj, (struct obj *) 0);
        return 1;
    }
    /* can write scrolls when blind, but requires luck too;
       attempts to write books when blind are caught above */
    if (Blind && rnl(3)) {
        /* writing while blind usually fails regardless of
           whether the target scroll is known; even if we
           have passed the write-an-unknown scroll test
           above we can still fail this one, so it's doubly
           hard to write an unknown scroll while blind */
        /*KR You("fail to write the scroll correctly and it disappears."); */
        You("두루마리를 제대로 작성하는 데 실패했고, 두루마리가 "
            "사라져버렸다.");
        useup(paper);
        obfree(new_obj, (struct obj *) 0);
        return 1;
    }

    /* useup old scroll / spellbook */
    useup(paper);

    /* success */
    if (new_obj->oclass == SPBOOK_CLASS) {
        /* acknowledge the change in the object's description... */
        /*KR pline_The("spellbook warps strangely, then turns %s.",
                  new_book_description(new_obj->otyp, namebuf)); */
        pline_The("마법서가 기묘하게 뒤틀리더니, %s 변했다.",
                  new_book_description(new_obj->otyp, namebuf));
    }
    new_obj->blessed = (curseval > 0);
    new_obj->cursed = (curseval < 0);
#ifdef MAIL
    if (new_obj->otyp == SCR_MAIL)
        /* 0: delivered in-game via external event (or randomly for fake
           mail); 1: from bones or wishing; 2: written with marker */
        new_obj->spe = 2;
#endif
    /* unlike alchemy, for example, a successful result yields the
       specifically chosen item so hero recognizes it even if blind;
       the exception is for being lucky writing an undiscovered scroll,
       where the label associated with the type-name isn't known yet */
    new_obj->dknown = label_known(new_obj->otyp, invent) ? 1 : 0;

#if 0 /*KR: 원본*/
    new_obj = hold_another_object(new_obj, "Oops!  %s out of your grasp!",
                                  The(aobjnam(new_obj, "slip")),
                                  (const char *) 0);
#else /*KR: KRNethack 맞춤 번역 (영어 매크로 대신 한국어 조사 사용)*/
    new_obj = hold_another_object(new_obj, "앗! %s 당신의 손에서 미끄러졌다!",
                                  append_josa(The(xname(new_obj)), "이"),
                                  (const char *) 0);
#endif
    nhUse(new_obj); /* try to avoid complaint about dead assignment */
    return 1;
}

/* most book descriptions refer to cover appearance, so we can issue a
   message for converting a plain book into one of those with something
   like "the spellbook turns red" or "the spellbook turns ragged";
   but some descriptions refer to composition and "the book turns vellum"
   looks funny, so we want to insert "into " prior to such descriptions;
   even that's rather iffy, indicating that such descriptions probably
   ought to be eliminated (especially "cloth"!) */
STATIC_OVL char *
new_book_description(booktype, outbuf)
int booktype;
char *outbuf;
{
#if 0 /*KR: 원본 (한국어 "으로" 구문에서는 "into" 예외 처리가 불필요함) */
    /* subset of description strings from objects.c; if it grows
       much, we may need to add a new flag field to objects[] instead */
    static const char *const compositions[] = {
        "parchment",
        "vellum",
        "cloth",
#if 0
        "canvas", "hardcover", /* not used */
        "papyrus", /* not applicable--can't be produced via writing */
#endif /*0*/
        0
    };
    const char *descr, *const *comp_p;

    descr = OBJ_DESCR(objects[booktype]);
    for (comp_p = compositions; *comp_p; ++comp_p)
        if (!strcmpi(descr, *comp_p))
            break;

    Sprintf(outbuf, "%s%s", *comp_p ? "into " : "", descr);
#else  /*KR: 으로 or 로 유동 적용 */
    Strcpy(outbuf, append_josa(OBJ_DESCR(objects[booktype]), "으로"));
#endif
    return outbuf;
}

/*write.c*/