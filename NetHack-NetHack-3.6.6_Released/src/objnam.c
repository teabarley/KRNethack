/* NetHack 3.6	objnam.c	$NHDT-Date: 1583315888 2020/03/04 09:58:08 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.293 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2011. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

/* "an uncursed greased partly eaten guardian naga hatchling [corpse]" */
#define PREFIX 80 /* (56) */
#define SCHAR_LIM 127
#define NUMOBUF 12

STATIC_DCL char *FDECL(strprepend, (char *, const char *));
STATIC_DCL short FDECL(rnd_otyp_by_wpnskill, (SCHAR_P));
STATIC_DCL short FDECL(rnd_otyp_by_namedesc, (const char *, CHAR_P, int));
STATIC_DCL boolean FDECL(wishymatch, (const char *, const char *, BOOLEAN_P));
STATIC_DCL char *NDECL(nextobuf);
STATIC_DCL void FDECL(releaseobuf, (char *));
STATIC_DCL char *FDECL(minimal_xname, (struct obj *));
STATIC_DCL void FDECL(add_erosion_words, (struct obj *, char *));
STATIC_DCL char *FDECL(doname_base, (struct obj *obj, unsigned));
STATIC_DCL char *FDECL(just_an, (char *str, const char *));
STATIC_DCL boolean FDECL(singplur_lookup, (char *, char *, BOOLEAN_P,
                                           const char *const *));
STATIC_DCL char *FDECL(singplur_compound, (char *));
STATIC_DCL char *FDECL(xname_flags, (struct obj *, unsigned));
STATIC_DCL boolean FDECL(badman, (const char *, BOOLEAN_P));
STATIC_DCL char *FDECL(globwt, (struct obj *, char *, boolean *));

#define BSTRCMPI(base, ptr, str) ((ptr) < base || strcmpi((ptr), str))
#define BSTRNCMPI(base, ptr, str, num) \
    ((ptr) < base || strncmpi((ptr), str, num))
#define Strcasecpy(dst, src) (void) strcasecpy(dst, src)

/*KR: 한글 파싱 매크로 추가 (문자열 일치 시 l에 길이를 자동 할당) */
#ifndef STRNCMPEX
#define STRNCMPEX(s, match) strncmp((s), (match), l = strlen(match))
#define STRNCMP2(s, match) strncmp((s), (match), strlen(match))
#endif

/* true for gems/rocks that should have " stone" appended to their names */
#define GemStone(typ)                                                  \
    (typ == FLINT                                                      \
     || (objects[typ].oc_material == GEMSTONE                          \
         && (typ != DILITHIUM_CRYSTAL && typ != RUBY && typ != DIAMOND \
             && typ != SAPPHIRE && typ != BLACK_OPAL && typ != EMERALD \
             && typ != OPAL)))



STATIC_OVL char *
strprepend(s, pref)
register char *s;
register const char *pref;
{
    register int i = (int) strlen(pref);

    if (i > PREFIX) {
        impossible("PREFIX too short (for %d).", i);
        return s;
    }
    s -= i;
    (void) strncpy(s, pref, i); /* do not copy trailing 0 */
    return s;
}

/* manage a pool of BUFSZ buffers, so callers don't have to */
static char NEARDATA obufs[NUMOBUF][BUFSZ];
static int obufidx = 0;

STATIC_OVL char *
nextobuf()
{
    obufidx = (obufidx + 1) % NUMOBUF;
    return obufs[obufidx];
}

/* put the most recently allocated buffer back if possible */
STATIC_OVL void
releaseobuf(bufp)
char *bufp;
{
    /* caller may not know whether bufp is the most recently allocated
       buffer; if it isn't, do nothing; note that because of the somewhat
       obscure PREFIX handling for object name formatting by xname(),
       the pointer our caller has and is passing to us might be into the
       middle of an obuf rather than the address returned by nextobuf() */
    if (bufp >= obufs[obufidx]
        && bufp < obufs[obufidx] + sizeof obufs[obufidx]) /* obufs[][BUFSZ] */
        obufidx = (obufidx - 1 + NUMOBUF) % NUMOBUF;
}

char *
obj_typename(otyp)
register int otyp;
{
    char *buf = nextobuf();
    struct objclass *ocl = &objects[otyp];
    const char *actualn = OBJ_NAME(*ocl);
    const char *dn = OBJ_DESCR(*ocl);
    const char *un = ocl->oc_uname;
    int nn = ocl->oc_name_known;

    switch (ocl->oc_class) {
    case COIN_CLASS:
        /*KR Strcpy(buf, "coin"); */
        Strcpy(buf, "금화");
        break;
    case POTION_CLASS:
        /*KR Strcpy(buf, "potion"); */
        Strcpy(buf, "물약");
        break;
    case SCROLL_CLASS:
        /*KR Strcpy(buf, "scroll"); */
        Strcpy(buf, "두루마리");
        break;
    case WAND_CLASS:
        /*KR Strcpy(buf, "wand"); */
        Strcpy(buf, "지팡이");
        break;
    case SPBOOK_CLASS:
        if (otyp != SPE_NOVEL) {
            /*KR Strcpy(buf, "spellbook"); */
            Strcpy(buf, "주문서");
        } else {
            /*KR Strcpy(buf, !nn ? "book" : "novel"); */
            Strcpy(buf, !nn ? "책" : "소설");
            nn = 0;
        }
        break;
    case RING_CLASS:
        /*KR Strcpy(buf, "ring"); */
        Strcpy(buf, "반지");
        break;
    case AMULET_CLASS:
#if 0 /*KR*/
        if (nn)
            Strcpy(buf, actualn);
        else
            Strcpy(buf, "amulet");
        if (un)
            Sprintf(eos(buf), " called %s", un);
        if (dn)
            Sprintf(eos(buf), " (%s)", dn);
        return buf;
#else
        if (nn)
            Strcpy(buf, actualn);
        else if(un)
            Strcpy(buf, "부적");
        break;
#endif
#if 1 /*KR*/
    case GEM_CLASS:
        if (nn)
            Strcat(buf, actualn);
        else if (un)
            Strcat(buf, "보석");
        break;
#endif
    default:
        if (nn) {
#if 0 /*KR*/
            Strcpy(buf, actualn);
            if (GemStone(otyp))
                Strcat(buf, " stone");
            if (un)
                Sprintf(eos(buf), " called %s", un);
            if (dn)
                Sprintf(eos(buf), " (%s)", dn);
#else
            Strcat(buf, actualn);
#endif
        } else {
#if 0 /*KR*/
            Strcpy(buf, dn ? dn : actualn);
            if (ocl->oc_class == GEM_CLASS)
                Strcat(buf,
                       (ocl->oc_material == MINERAL) ? " stone" : " gem");
            if (un)
                Sprintf(eos(buf), " called %s", un);
#else
            Strcat(buf, dn ? dn : actualn);
#endif
        }
#if 0 /*KR*/
        return buf;
#else
        break;
#endif
    }
    /* here for ring/scroll/potion/wand */
    if (nn) {
#if 0 /*KR*/
        if (ocl->oc_unique)
            Strcpy(buf, actualn); /* avoid spellbook of Book of the Dead */
        else
            Sprintf(eos(buf), " of %s", actualn);
#else
        Strcpy(buf, actualn);
#endif
    }
#if 0 /*KR*/
    if (un)
        Sprintf(eos(buf), " called %s", un);
#endif
    if (dn)
        /*KR Sprintf(eos(buf), " (%s)", dn); */
        Sprintf(eos(buf), "(%s)", dn);

#if 1 /*KR:T (유저가 지어준 이름 복구) */
    if (un)
        Sprintf(eos(buf), " [%s고 불림]", 
                append_josa(un, "이라"));
#endif

    return buf;
}

/* less verbose result than obj_typename(); either the actual name
   or the description (but not both); user-assigned name is ignored */
char *
simple_typename(otyp)
int otyp;
{
    char *bufp, *pp, *save_uname = objects[otyp].oc_uname;

    objects[otyp].oc_uname = 0; /* suppress any name given by user */
    bufp = obj_typename(otyp);
    objects[otyp].oc_uname = save_uname;
    if ((pp = strstri(bufp, " (")) != 0)
        *pp = '\0'; /* strip the appended description */
    return bufp;
}

/* typename for debugging feedback where data involved might be suspect */
char *
safe_typename(otyp)
int otyp;
{
    unsigned save_nameknown;
    char *res = 0;

    if (otyp < STRANGE_OBJECT || otyp >= NUM_OBJECTS
        || !OBJ_NAME(objects[otyp])) {
        res = nextobuf();
        Sprintf(res, "glorkum[%d]", otyp);
    } else {
        /* force it to be treated as fully discovered */
        save_nameknown = objects[otyp].oc_name_known;
        objects[otyp].oc_name_known = 1;
        res = simple_typename(otyp);
        objects[otyp].oc_name_known = save_nameknown;
    }
    return res;
}

boolean
obj_is_pname(obj)
struct obj *obj;
{
    if (!obj->oartifact || !has_oname(obj))
        return FALSE;
    if (!program_state.gameover && !iflags.override_ID) {
        if (not_fully_identified(obj))
            return FALSE;
    }
    return TRUE;
}

/* used by distant_name() to pass extra information to xname_flags();
   it would be much cleaner if this were a parameter, but that would
   require all of the xname() and doname() calls to be modified */
static int distantname = 0;

/* Give the name of an object seen at a distance.  Unlike xname/doname,
 * we don't want to set dknown if it's not set already.
 */
char *
distant_name(obj, func)
struct obj *obj;
char *FDECL((*func), (OBJ_P));
{
    char *str;

    /* 3.6.1: this used to save Blind, set it, make the call, then restore
     * the saved value; but the Eyes of the Overworld override blindness
     * and let characters wearing them get dknown set for distant items.
     *
     * TODO? if the hero is wearing those Eyes, figure out whether the
     * object is within X-ray radius and only treat it as distant when
     * beyond that radius.  Logic is iffy but result might be interesting.
     */
    ++distantname;
    str = (*func)(obj);
    --distantname;
    return str;
}

/* convert player specified fruit name into corresponding fruit juice name
   ("slice of pizza" -> "pizza juice" rather than "slice of pizza juice") */
char *
fruitname(juice)
boolean juice; /* whether or not to append " juice" to the name */
{
#if 0 /*KR*/
    char *buf = nextobuf();
    const char *fruit_nam = strstri(pl_fruit, " of ");

    if (fruit_nam)
        fruit_nam += 4; /* skip past " of " */
    else
        fruit_nam = pl_fruit; /* use it as is */

    Sprintf(buf, "%s%s", makesingular(fruit_nam), juice ? " juice" : "");
    return buf;
#else /*한글에서는 여기까지 하지는 않음*/
    char* buf = nextobuf();
    Sprintf(buf, "%s %s", pl_fruit, juice ? " 주스" : "");
    return buf;
#endif
}

/* look up a named fruit by index (1..127) */
struct fruit *
fruit_from_indx(indx)
int indx;
{
    struct fruit *f;

    for (f = ffruit; f; f = f->nextf)
        if (f->fid == indx)
            break;
    return f;
}

/* look up a named fruit by name */
struct fruit *
fruit_from_name(fname, exact, highest_fid)
const char *fname;
boolean exact; /* False => prefix or exact match, True = exact match only */
int *highest_fid; /* optional output; only valid if 'fname' isn't found */
{
    struct fruit *f, *tentativef;
    char *altfname;
    unsigned k;
    /*
     * note: named fruits are case-senstive...
     */

    if (highest_fid)
        *highest_fid = 0;
    /* first try for an exact match */
    for (f = ffruit; f; f = f->nextf)
        if (!strcmp(f->fname, fname))
            return f;
        else if (highest_fid && f->fid > *highest_fid)
            *highest_fid = f->fid;

    /* didn't match as-is; if caller is willing to accept a prefix
       match, try to find one; we want to find the longest prefix that
       matches, not the first */
    if (!exact) {
        tentativef = 0;
        for (f = ffruit; f; f = f->nextf) {
            k = strlen(f->fname);
            if (!strncmp(f->fname, fname, k)
                && (!fname[k] || fname[k] == ' ')
                && (!tentativef || k > strlen(tentativef->fname)))
                tentativef = f;
        }
        f = tentativef;
    }
    /* if we still don't have a match, try singularizing the target;
       for exact match, that's trivial, but for prefix, it's hard */
    if (!f) {
        altfname = makesingular(fname);
        for (f = ffruit; f; f = f->nextf) {
            if (!strcmp(f->fname, altfname))
                break;
        }
        releaseobuf(altfname);
    }
    if (!f && !exact) {
        char fnamebuf[BUFSZ], *p;
        unsigned fname_k = strlen(fname); /* length of assumed plural fname */

        tentativef = 0;
        for (f = ffruit; f; f = f->nextf) {
            k = strlen(f->fname);
            /* reload fnamebuf[] each iteration in case it gets modified;
               there's no need to recalculate fname_k */
            Strcpy(fnamebuf, fname);
            /* bug? if singular of fname is longer than plural,
               failing the 'fname_k > k' test could skip a viable
               candidate; unfortunately, we can't singularize until
               after stripping off trailing stuff and we can't get
               accurate fname_k until fname has been singularized;
               compromise and use 'fname_k >= k' instead of '>',
               accepting 1 char length discrepancy without risking
               false match (I hope...) */
            if (fname_k >= k && (p = index(&fnamebuf[k], ' ')) != 0) {
                *p = '\0'; /* truncate at 1st space past length of f->fname */
                altfname = makesingular(fnamebuf);
                k = strlen(altfname); /* actually revised 'fname_k' */
                if (!strcmp(f->fname, altfname)
                    && (!tentativef || k > strlen(tentativef->fname)))
                    tentativef = f;
                releaseobuf(altfname); /* avoid churning through all obufs */
            }
        }
        f = tentativef;
    }
    return f;
}

/* sort the named-fruit linked list by fruit index number */
void
reorder_fruit(forward)
boolean forward;
{
    struct fruit *f, *allfr[1 + 127];
    int i, j, k = SIZE(allfr);

    for (i = 0; i < k; ++i)
        allfr[i] = (struct fruit *) 0;
    for (f = ffruit; f; f = f->nextf) {
        /* without sanity checking, this would reduce to 'allfr[f->fid]=f' */
        j = f->fid;
        if (j < 1 || j >= k) {
            impossible("reorder_fruit: fruit index (%d) out of range", j);
            return; /* don't sort after all; should never happen... */
        } else if (allfr[j]) {
            impossible("reorder_fruit: duplicate fruit index (%d)", j);
            return;
        }
        allfr[j] = f;
    }
    ffruit = 0; /* reset linked list; we're rebuilding it from scratch */
    /* slot [0] will always be empty; must start 'i' at 1 to avoid
       [k - i] being out of bounds during first iteration */
    for (i = 1; i < k; ++i) {
        /* for forward ordering, go through indices from high to low;
           for backward ordering, go from low to high */
        j = forward ? (k - i) : i;
        if (allfr[j]) {
            allfr[j]->nextf = ffruit;
            ffruit = allfr[j];
        }
    }
}

char *
xname(obj)
struct obj *obj;
{
    return xname_flags(obj, CXN_NORMAL);
}

STATIC_OVL char *
xname_flags(obj, cxn_flags)
register struct obj *obj;
unsigned cxn_flags; /* bitmask of CXN_xxx values */
{
    register char *buf;
    register int typ = obj->otyp;
    register struct objclass *ocl = &objects[typ];
    int nn = ocl->oc_name_known, omndx = obj->corpsenm;
    const char *actualn = OBJ_NAME(*ocl);
    const char *dn = OBJ_DESCR(*ocl);
    const char *un = ocl->oc_uname;
    boolean pluralize = (obj->quan != 1L) && !(cxn_flags & CXN_SINGULAR);
    boolean known, dknown, bknown;

    buf = nextobuf() + PREFIX; /* leave room for "17 -3 " */
    /* As of 3.6.2: this used to be part of 'dn's initialization, but it
       needs to come after possibly overriding 'actualn' */
#if 0 /*KR: 원본*/
    if (!dn)
        dn = actualn;
#else /*KR: KRNethack 맞춤 번역 */
    if (!dn)
        dn = actualn;
    else
        dn = get_kr_name(dn); /* 미식별 시의 묘사(runed 등) 번역 적용 */
#endif

    buf[0] = '\0';
    /*
     * clean up known when it's tied to oc_name_known, eg after AD_DRIN
     * This is only required for unique objects since the article
     * printed for the object is tied to the combination of the two
     * and printing the wrong article gives away information.
     */
    if (!nn && ocl->oc_uses_known && ocl->oc_unique)
        obj->known = 0;
    if (!Blind && !distantname)
        obj->dknown = 1;
    if (Role_if(PM_PRIEST))
        obj->bknown = 1; /* actively avoid set_bknown();
                          * we mustn't call update_inventory() now because
                          * it would call xname() (via doname()) recursively
                          * and could end up clobbering all the obufs... */

    if (iflags.override_ID) {
        known = dknown = bknown = TRUE;
        nn = 1;
    } else {
        known = obj->known;
        dknown = obj->dknown;
        bknown = obj->bknown;
    }

    if (obj_is_pname(obj))
#if 0 /*KR*/
        goto nameit;
#else
    {
        Strcat(buf, get_kr_name(ONAME(obj)));
        goto nameit;
    }
#endif
#if 1 /*KR*/
    if (has_oname(obj) && dknown) {
        /* 사체(CORPSE)일 때는 여기서 이름을 붙이지 않고, *
         * doname_base에서 자연스럽게 조립하도록 미룸.    */
        if (obj->otyp != CORPSE) {
            Sprintf(eos(buf), "%s 이름붙인 ",
                    append_josa(get_kr_name(ONAME(obj)), "이라"));
        }
    }
#endif
    switch (obj->oclass) {
    case AMULET_CLASS:
        if (!dknown)
            /*KR Strcpy(buf, "amulet"); */
            Strcpy(buf, "부적");
        else if (typ == AMULET_OF_YENDOR || typ == FAKE_AMULET_OF_YENDOR)
            /* each must be identified individually */
            Strcpy(buf, known ? actualn : dn);
        else if (nn)
            Strcpy(buf, actualn);
        else if (un)
            /*KR Sprintf(buf, "amulet called %s", un); */
            Sprintf(buf, "%s라 불리는 부적", un);
        else
            /*KR Sprintf(buf, "%s amulet", dn); */
            Sprintf(eos(buf), "%s 부적", dn);
        break;
    case WEAPON_CLASS:
        if (is_poisonable(obj) && obj->opoisoned)
            /*KR Strcpy(buf, "poisoned "); */
            Strcpy(buf, "독이 발린 ");
        /*FALLTHRU*/
    case VENOM_CLASS:
    case TOOL_CLASS:
#if 1 /*KR*/
        if (typ == FIGURINE)
            Sprintf(eos(buf), "%s의 ", get_kr_name(mons[obj->corpsenm].mname));
#endif
        if (typ == LENSES)
            /*KR Strcpy(buf, "pair of "); */
            Strcpy(buf, "한 쌍의 ");
        else if (is_wet_towel(obj))
            /*KR Strcpy(buf, (obj->spe < 3) ? "moist " : "wet "); */
            Strcpy(buf, (obj->spe < 3) ? "축축한 " : "젖은 ");

        if (!dknown)
            Strcat(buf, dn);
        else if (nn)
            Strcat(buf, actualn);
        else if (un) {
#if 0 /*KR*/
            Strcat(buf, dn);
            Strcat(buf, " called ");
            Strcat(buf, un);
#else
            Strcat(buf, dn);
            Strcat(buf, " 라 불리는 ");
            Strcat(buf, un);
#endif
        } else
            Strcat(buf, dn);

#if 0 /*KR*/ /*이 부분은 어순의 관계에서 상단에 이미 정의함*/
        if (typ == FIGURINE && omndx != NON_PM) {
            char anbuf[10]; /* [4] would be enough: 'a','n',' ','\0' */

            Sprintf(eos(buf), " of %s%s",
                    just_an(anbuf, mons[omndx].mname),
                    mons[omndx].mname);
        } else if (is_wet_towel(obj)) {
#else
        if (is_wet_towel(obj)) {
#endif
            if (wizard)
                Sprintf(eos(buf), " (%d)", obj->spe);
        }
        break;
    case ARMOR_CLASS:
        /* depends on order of the dragon scales objects */
        if (typ >= GRAY_DRAGON_SCALES && typ <= YELLOW_DRAGON_SCALES) {
            /*KR Sprintf(buf, "set of %s", actualn); */
            Sprintf(buf, "%s 한 벌", actualn);
            break;
        }
        if (is_boots(obj) || is_gloves(obj))
            /*KR Strcpy(buf, "pair of "); */
            Strcpy(buf, "한 쌍의 ");

        if (obj->otyp >= ELVEN_SHIELD && obj->otyp <= ORCISH_SHIELD
            && !dknown) {
            /*KR Strcpy(buf, "shield"); */
            Strcpy(buf, "방패");
            break;
        }
        if (obj->otyp == SHIELD_OF_REFLECTION && !dknown) {
            /*KR Strcpy(buf, "smooth shield"); */
            Strcpy(buf, "매끈한 방패");
            break;
        }

        if (nn) {
            Strcat(buf, actualn);
        } else if (un) {
#if 0 /*KR*/
            if (is_boots(obj))
                Strcat(buf, "boots");
            else if (is_gloves(obj))
                Strcat(buf, "gloves");
            else if (is_cloak(obj))
                Strcpy(buf, "cloak");
            else if (is_helmet(obj))
                Strcpy(buf, "helmet");
            else if (is_shield(obj))
                Strcpy(buf, "shield");
            else
                Strcpy(buf, "armor");
            Strcat(buf, " called ");
            Strcat(buf, un);
#else
            const char *p;
            if (is_boots(obj))
                p = "장화";
            else if (is_gloves(obj))
                p = "장갑";
            else if (is_cloak(obj))
                p = "망토";
            else if (is_helmet(obj))
                p = "투구";
            else if (is_shield(obj))
                p = "방패";
            else
                p = "갑옷";
            Sprintf(eos(buf), "%s 불리는 %s", 
                    append_josa(un, "이라"), p);
#endif
        } else
            Strcat(buf, dn);
        break;
    case FOOD_CLASS:
        if (typ == SLIME_MOLD) {
            struct fruit *f = fruit_from_indx(obj->spe);

            if (!f) {
                impossible("Bad fruit #%d?", obj->spe);
                Strcpy(buf, "fruit");
            } else {
                Strcpy(buf, f->fname);
                if (pluralize) {
                    /* ick; already pluralized fruit names
                       are allowed--we want to try to avoid
                       adding a redundant plural suffix */
                    Strcpy(buf, makeplural(makesingular(buf)));
                    pluralize = FALSE;
                }
            }
            break;
        }
        if (obj->globby) {
            Sprintf(buf, "%s%s",
                    (obj->owt <= 100)
                  /*KR ? "small " */
                       ? "작은 "
                       : (obj->owt > 500)
                     /*KR ? "very large " */
                          ? "매우 큰 "
                          : (obj->owt > 300)
                        /*KR ? "large " */
                             ? "커다란 "
                             : "",
                    actualn);
            break;
        }

#if 0 /*KR*/
        Strcpy(buf, actualn);
        if (typ == TIN && known)
            tin_details(obj, omndx, buf);
#else
        if (typ == TIN && known)
            /*KR "~의 고기" */
            tin_details(obj, omndx, buf);
        Strcat(buf, actualn);
#endif
        break;
    case COIN_CLASS:
    case CHAIN_CLASS:
#if 0 /*KR*/
        Strcpy(buf, actualn);
#else
        Strcat(buf, actualn);
#endif
        break;
    case ROCK_CLASS:
        if (typ == STATUE && omndx != NON_PM) {
#if 0 /*KR*/
            char anbuf[10];

            Sprintf(buf, "%s%s of %s%s",
                    (Role_if(PM_ARCHEOLOGIST) && (obj->spe & STATUE_HISTORIC))
                       ? "historic "
                       : "",
                    actualn,
                    type_is_pname(&mons[omndx])
                       ? ""
                       : the_unique_pm(&mons[omndx])
                          ? "the "
                          : just_an(anbuf, mons[omndx].mname),
                    mons[omndx].mname);
#else
            Sprintf(eos(buf), "%s %s의 %s",
                (Role_if(PM_ARCHEOLOGIST) && (obj->spe& STATUE_HISTORIC))
                ? "역사적인"
                : "",
                get_kr_name(mons[obj->corpsenm].mname), actualn);
#endif
        } else
#if 0 /*KR*/
            Strcpy(buf, actualn);
#else
            Strcat(buf, actualn);
#endif
        break;
    case BALL_CLASS:
#if 0 /*KR*/
        Sprintf(buf, "%sheavy iron ball",
                (obj->owt > ocl->oc_weight) ? "very " : "");
#else
        Sprintf(eos(buf), "%s 무거운 철구",
                (obj->owt > ocl->oc_weight) ? "엄청" : "");
#endif
        break;

case POTION_CLASS:
#if 0 /*KR: 원본 코드 (영어) */
        if (dknown && obj->odiluted)
            Strcpy(buf, "diluted ");
        if (nn || un || !dknown) {
            Strcat(buf, "potion");
            if (!dknown)
                break;
            if (nn) {
                Strcat(buf, " of ");
                if (typ == POT_WATER && bknown
                    && (obj->blessed || obj->cursed)) {
                    Strcat(buf, obj->blessed ? "holy " : "unholy ");
                }
                Strcat(buf, actualn);
            } else {
                Strcat(buf, " called ");
                Strcat(buf, un);
            }
        } else {
            Strcat(buf, dn);
            Strcat(buf, " potion");
        }
#else /*KR: KRNethack 맞춤 번역 */
        if (dknown && obj->odiluted)
            Strcat(buf, "희석된 ");

        if (!dknown) {
            Strcat(buf, "물약");
        } else if (nn) {
            if (typ == POT_WATER && bknown && (obj->blessed || obj->cursed)) {
                Strcat(buf, obj->blessed ? "신성한 " : "부정한 ");
            }
            /* 완전 식별되었을 때 (예: 치료 물약) */
            Sprintf(eos(buf), "%s 물약", actualn);
        } else if (un) {
            /* 유저가 이름을 지어줬을 때 */
            Sprintf(eos(buf), "%s고 불리는 물약", append_josa(un, "이라"));
        } else {
            /* 묘사만 알 때 (예: 루비 물약) */
            Sprintf(eos(buf), "%s 물약", get_kr_name(dn));
        }
#endif
        break;

    case SCROLL_CLASS:
#if 0 /*KR: 원본 코드 (영어) */
        Strcpy(buf, "scroll");
        if (!dknown)
            break;
        if (nn) {
            Strcat(buf, " of ");
            Strcat(buf, actualn);
        } else if (un) {
            Strcat(buf, " called ");
            Strcat(buf, un);
        } else if (ocl->oc_magic) {
            Strcat(buf, " labeled ");
            Strcat(buf, dn);
        } else {
            Strcpy(buf, dn);
            Strcat(buf, " scroll");
        }
#else /*KR: KRNethack 맞춤 번역 */
        if (!dknown) {
            Strcat(buf, "두루마리");
        } else if (nn) {
            Sprintf(eos(buf), "%s 두루마리", actualn);
        } else if (un) {
            Sprintf(eos(buf), "%s고 불리는 두루마리", 
                    append_josa(un, "이라"));
        } else if (ocl->oc_magic) {
            /* 마법 두루마리일 때 (예: "ZELGO MER"라고 적힌 두루마리) */
            Sprintf(eos(buf), "\"%s\"라고 적힌 두루마리", get_kr_name(dn));
        } else {
            Sprintf(eos(buf), "%s 두루마리", get_kr_name(dn));
        }
#endif
        break;

    case WAND_CLASS:
#if 0 /*KR: 원본 코드 (영어) */
        if (!dknown)
            Strcpy(buf, "wand");
        else if (nn)
            Sprintf(buf, "wand of %s", actualn);
        else if (un)
            Sprintf(buf, "wand called %s", un);
        else
            Sprintf(buf, "%s wand", dn);
#else /*KR: KRNethack 맞춤 번역 */
        if (!dknown) {
            Strcat(buf, "지팡이");
        } else if (nn) {
            Sprintf(eos(buf), "%s 지팡이", actualn);
        } else if (un) {
            Sprintf(eos(buf), "%s고 불리는 지팡이", 
                    append_josa(un, "이라"));
        } else {
#if 1 /*KR: tin(주석/통조림) 사전 충돌 예외 처리 */
            const char *wand_desc = dn;
            if (RAW_OBJ_DESCR(*ocl) && !strcmp(RAW_OBJ_DESCR(*ocl), "tin")) {
                wand_desc =
                    "주석"; /* 영어 원문이 tin이라면 주석으로 강제 고정 */
            }
            Sprintf(eos(buf), "%s 지팡이", get_kr_name(wand_desc));
#else
            Sprintf(eos(buf), "%s 지팡이", get_kr_name(dn));
#endif
        }
#endif
        break;

    case SPBOOK_CLASS:
#if 0 /*KR: 원본 코드 (영어) */
        if (typ == SPE_NOVEL) { /* 3.6 tribute */
            if (!dknown)
                Strcpy(buf, "book");
            else if (nn)
                Strcpy(buf, actualn);
            else if (un)
                Sprintf(buf, "novel called %s", un);
            else
                Sprintf(buf, "%s book", dn);
            break;
            /* end of tribute */
        } else if (!dknown) {
            Strcpy(buf, "spellbook");
        } else if (nn) {
            if (typ != SPE_BOOK_OF_THE_DEAD)
                Strcpy(buf, "spellbook of ");
            Strcat(buf, actualn);
        } else if (un) {
            Sprintf(buf, "spellbook called %s", un);
        } else
            Sprintf(buf, "%s spellbook", dn);
#else /*KR: KRNethack 맞춤 번역 */
        if (typ == SPE_NOVEL) {
            if (!dknown)
                Strcpy(buf, "책");
            else if (nn)
                Sprintf(buf, "%s 소설", actualn);
            else if (un)
                Sprintf(buf, "%s고 불리는 소설", 
                        append_josa(un, "이라"));
            else
                Sprintf(buf, "%s 책", get_kr_name(dn));
        } else {
            if (!dknown)
                Strcpy(buf, "주문서");
            else if (nn)
                Sprintf(eos(buf), "%s 주문서", actualn);
            else if (un)
                Sprintf(eos(buf), "%s고 불리는 주문서", 
                        append_josa(un, "이라"));
            else
                Sprintf(eos(buf), "%s 주문서", get_kr_name(dn));
        }
#endif
        break;

    case RING_CLASS:
#if 0 /*KR: 원본 코드 (영어) */
        if (!dknown)
            Strcpy(buf, "ring");
        else if (nn)
            Sprintf(buf, "ring of %s", actualn);
        else if (un)
            Sprintf(buf, "ring called %s", un);
        else
            Sprintf(buf, "%s ring", dn);
#else /*KR: KRNethack 맞춤 번역 */
        if (!dknown) {
            Strcat(buf, "반지");
        } else if (nn) {
            Sprintf(eos(buf), "%s 반지", actualn);
        } else if (un) {
            Sprintf(eos(buf), "%s고 불리는 반지", 
                    append_josa(un, "이라"));
        } else {
            Sprintf(eos(buf), "%s 반지", get_kr_name(dn));
        }
#endif
        break;

    case GEM_CLASS: {
        const char *rock = (ocl->oc_material == MINERAL) ? "stone" : "gem";
#if 0 /*KR: 원본 코드 (영어) */
        if (!dknown) {
            Strcpy(buf, rock);
        } else if (!nn) {
            if (un)
                Sprintf(buf, "%s called %s", rock, un);
            else
                Sprintf(buf, "%s %s", dn, rock);
        } else {
            Strcpy(buf, actualn);
            if (GemStone(typ))
                Strcat(buf, " stone");
        }
#else /*KR: KRNethack 맞춤 번역 */
        const char *kr_rock = (ocl->oc_material == MINERAL) ? "돌" : "보석";

        if (!dknown) {
            Strcat(buf, kr_rock);
        } else if (nn) {
     /* 보석은 보통 번역명 자체에 '보석'이나 '돌'이 
      * 포함되어 있으므로 actualn만 붙입니다 */
            Strcpy(buf, actualn);
        } else if (un) {
            Sprintf(eos(buf), "%s고 불리는 %s", 
                    append_josa(un, "이라"), kr_rock);
        } else {
            Sprintf(eos(buf), "%s %s", get_kr_name(dn), kr_rock);
        }
#endif
        break;
    }
    default:
        Sprintf(buf, "glorkum %d %d %d", obj->oclass, typ, obj->spe);
    }
#if 0 /*KR*/
    if (pluralize)
        Strcpy(buf, makeplural(buf));
#endif

    if (obj->otyp == T_SHIRT && program_state.gameover) {
        char tmpbuf[BUFSZ];

        /*KR Sprintf(eos(buf), " with text \"%s\"", tshirt_text(obj, tmpbuf)); */
        Sprintf(eos(buf), "(\"%s\" 라고 쓰여 있다)", tshirt_text(obj, tmpbuf));
    }

#if 0 /*KR*/
    if (has_oname(obj) && dknown) {
        Strcat(buf, " named ");
 nameit:
        Strcat(buf, ONAME(obj));
    }

    if (!strncmpi(buf, "the ", 4))
        buf += 4;
#else
    nameit:
#endif
    return buf;
}

/* similar to simple_typename but minimal_xname operates on a particular
   object rather than its general type; it formats the most basic info:
     potion                     -- if description not known
     brown potion               -- if oc_name_known not set
     potion of object detection -- if discovered
 */
STATIC_OVL char *
minimal_xname(obj)
struct obj *obj;
{
    char *bufp;
    struct obj bareobj;
    struct objclass saveobcls;
    int otyp = obj->otyp;

    /* suppress user-supplied name */
    saveobcls.oc_uname = objects[otyp].oc_uname;
    objects[otyp].oc_uname = 0;
    /* suppress actual name if object's description is unknown */
    saveobcls.oc_name_known = objects[otyp].oc_name_known;
    if (!obj->dknown)
        objects[otyp].oc_name_known = 0;

    /* caveat: this makes a lot of assumptions about which fields
       are required in order for xname() to yield a sensible result */
    bareobj = zeroobj;
    bareobj.otyp = otyp;
    bareobj.oclass = obj->oclass;
    bareobj.dknown = obj->dknown;
    /* suppress known except for amulets (needed for fakes and real A-of-Y) */
    bareobj.known = (obj->oclass == AMULET_CLASS)
                        ? obj->known
                        /* default is "on" for types which don't use it */
                        : !objects[otyp].oc_uses_known;
    bareobj.quan = 1L;         /* don't want plural */
    bareobj.corpsenm = NON_PM; /* suppress statue and figurine details */
    /* but suppressing fruit details leads to "bad fruit #0"
       [perhaps we should force "slime mold" rather than use xname?] */
    if (obj->otyp == SLIME_MOLD)
        bareobj.spe = obj->spe;

    bufp = distant_name(&bareobj, xname); /* xname(&bareobj) */
#if 0 /*KR:T*/
    if (!strncmp(bufp, "uncursed ", 9))
        bufp += 9; /* Role_if(PM_PRIEST) */
#else
    if (!strncmp(bufp, "저주받지 않은 ", 14))
        bufp += 14; /* Role_if(PM_PRIEST) */
#endif

    objects[otyp].oc_uname = saveobcls.oc_uname;
    objects[otyp].oc_name_known = saveobcls.oc_name_known;
    return bufp;
}

/* xname() output augmented for multishot missile feedback */
char *
mshot_xname(obj)
struct obj *obj;
{
    char tmpbuf[BUFSZ];
    char *onm = xname(obj);

    if (m_shot.n > 1 && m_shot.o == obj->otyp) {
#if 0 /*KR: 원본*/
        /* "the Nth arrow"; value will eventually be passed to an() or
           The(), both of which correctly handle this "the " prefix */
        Sprintf(tmpbuf, "the %d%s ", m_shot.i, ordin(m_shot.i));
#else /*KR: 일괄적으로 '%d번째 '를 아이템 이름 앞에 붙임 */
        /* (예: 1번째 화살) */
        Sprintf(tmpbuf, "%d번째 ", m_shot.i);
#endif
        onm = strprepend(onm, tmpbuf);
    }
    return onm;
}

/* used for naming "the unique_item" instead of "a unique_item" */
boolean
the_unique_obj(obj)
struct obj *obj;
{
#if 0 /*KR*/
    boolean known = (obj->known || iflags.override_ID);

    if (!obj->dknown && !iflags.override_ID)
        return FALSE;
    else if (obj->otyp == FAKE_AMULET_OF_YENDOR && !known)
        return TRUE; /* lie */
    else
        return (boolean) (objects[obj->otyp].oc_unique
                          && (known || obj->otyp == AMULET_OF_YENDOR));
#else
    return FALSE;
#endif

}

/* should monster type be prefixed with "the"? (mostly used for corpses) */
boolean
the_unique_pm(ptr)
struct permonst *ptr;
{
    boolean uniq;

    /* even though monsters with personal names are unique, we want to
       describe them as "Name" rather than "the Name" */
    if (type_is_pname(ptr))
        return FALSE;

    uniq = (ptr->geno & G_UNIQ) ? TRUE : FALSE;
    /* high priest is unique if it includes "of <deity>", otherwise not
       (caller needs to handle the 1st possibility; we assume the 2nd);
       worm tail should be irrelevant but is included for completeness */
    if (ptr == &mons[PM_HIGH_PRIEST] || ptr == &mons[PM_LONG_WORM_TAIL])
        uniq = FALSE;
    /* Wizard no longer needs this; he's flagged as unique these days */
    if (ptr == &mons[PM_WIZARD_OF_YENDOR])
        uniq = TRUE;
    return uniq;
}

STATIC_OVL void
add_erosion_words(obj, prefix)
struct obj *obj;
char *prefix;
{
    boolean iscrys = (obj->otyp == CRYSKNIFE);
    boolean rknown;

    rknown = (iflags.override_ID == 0) ? obj->rknown : TRUE;

    if (!is_damageable(obj) && !iscrys)
        return;

    /* The only cases where any of these bits do double duty are for
     * rotted food and diluted potions, which are all not is_damageable().
     */
    if (obj->oeroded && !iscrys) {
        switch (obj->oeroded) {
        case 2:
            /*KR Strcat(prefix, "very "); */
            Strcat(prefix, "매우 ");
            break;
        case 3:
            /*KR Strcat(prefix, "thoroughly "); */
            Strcat(prefix, "완전히 ");
            break;
        }
        /*KR Strcat(prefix, is_rustprone(obj) ? "rusty " : "burnt "); */
        Strcat(prefix, is_rustprone(obj) ? "녹슨 " : "불에 탄 ");
    }
    if (obj->oeroded2 && !iscrys) {
        switch (obj->oeroded2) {
        case 2:
            /*KR Strcat(prefix, "very "); */
            Strcat(prefix, "매우 ");
            break;
        case 3:
            /*KR Strcat(prefix, "thoroughly "); */
            Strcat(prefix, "완전히 ");
            break;
        }
        /*KR Strcat(prefix, is_corrodeable(obj) ? "corroded " : "rotted "); */
        Strcat(prefix, is_corrodeable(obj) ? "부식된 " : "녹슨 ");
    }
    if (rknown && obj->oerodeproof)
#if 0 /*KR:T*/
        Strcat(prefix, iscrys
                          ? "fixed "
                          : is_rustprone(obj)
                             ? "rustproof "
                             : is_corrodeable(obj)
                                ? "corrodeproof " /* "stainless"? */
                                : is_flammable(obj)
                                   ? "fireproof "
                                   : "");
#else
        Strcat(prefix, iscrys
            ? "고정된 "
            : is_rustprone(obj)
            ? "방녹의 "
            : is_corrodeable(obj)
            ? "방부식의 " /* "stainless"? */
            : is_flammable(obj)
            ? "방염의 "
            : "");
#endif
}

/* used to prevent rust on items where rust makes no difference */
boolean
erosion_matters(obj)
struct obj *obj;
{
    switch (obj->oclass) {
    case TOOL_CLASS:
        /* it's possible for a rusty weptool to be polymorphed into some
           non-weptool iron tool, in which case the rust implicitly goes
           away, but it's also possible for it to be polymorphed into a
           non-iron tool, in which case rust also implicitly goes away,
           so there's no particular reason to try to handle the first
           instance differently [this comment belongs in poly_obj()...] */
        return is_weptool(obj) ? TRUE : FALSE;
    case WEAPON_CLASS:
    case ARMOR_CLASS:
    case BALL_CLASS:
    case CHAIN_CLASS:
        return TRUE;
    default:
        break;
    }
    return FALSE;
}

#define DONAME_WITH_PRICE 1
#define DONAME_VAGUE_QUAN 2

STATIC_OVL char *
doname_base(obj, doname_flags)
struct obj *obj;
unsigned doname_flags;
{
    boolean ispoisoned = FALSE,
            with_price = (doname_flags & DONAME_WITH_PRICE) != 0,
            vague_quan = (doname_flags & DONAME_VAGUE_QUAN) != 0,
            weightshown = FALSE;
    boolean known, dknown, cknown, bknown, lknown;
    int omndx = obj->corpsenm;
    char prefix[PREFIX], globbuf[QBUFSZ];
#if 0 /*KR*/
    char tmpbuf[PREFIX + 1]; /* for when we have to add something at
                                the start of prefix instead of the
                                end (Strcat is used on the end) */
#endif
    register char *bp = xname(obj);
#if 1 /*KR: KRNethack 맞춤 번역 (어순 조립용 변수)*/
    /* 수식어를 앞으로 빼내기 위한 버퍼 */
    char preprefix[PREFIX];
    preprefix[0] = '\0';
#endif

    if (iflags.override_ID) {
        known = dknown = cknown = bknown = lknown = TRUE;
    } else {
        known = obj->known;
        dknown = obj->dknown;
        cknown = obj->cknown;
        bknown = obj->bknown;
        lknown = obj->lknown;
    }

    /* When using xname, we want "poisoned arrow", and when using
     * doname, we want "poisoned +0 arrow".  This kludge is about the only
     * way to do it, at least until someone overhauls xname() and doname(),
     * combining both into one function taking a parameter.
     */
    /* must check opoisoned--someone can have a weirdly-named fruit */
#if 0 /*KR:T*/
    if (!strncmp(bp, "poisoned ", 9) && obj->opoisoned) {
        bp += 9;
        ispoisoned = TRUE;
    }
#else
    {
        /* 숫자 12 미사용. strlen을 사용해 인코딩 버그 차단. */
        int p_len = strlen("독이 발린 ");
        if (!strncmp(bp, "독이 발린 ", p_len) && obj->opoisoned) {
            bp += p_len;
            ispoisoned = TRUE;
        }
    }
#endif

#if 1 /*KR:T*/
    /* xname()에서 이미 "타마(이)라고 불리는 새끼 고양이의 사체"처럼 넘어올
       경우, "타마(이)라고 불리는 " 부분만 안전하게 잘라내어 preprefix에
       보관하고, bp(본체 이름)는 그 뒤부터 시작하도록 포인터를 옮겨줍니다. */
    {
        char *tp;
        /* 하드코딩된 숫자 12 대신 strlen을 사용. */
        const char *keyword = "불리는 ";
        if ((tp = strstr(bp, keyword)) != NULL) {
            /* (tp - bp)는 타마(이)라고" 까지의 길이입니다. */
            int split_len = (tp - bp) + strlen(keyword);

            /* [안전장치] 배열 크기를 넘지 않도록 제한 */
            if (split_len >= PREFIX)
                split_len = PREFIX - 1;

            strncpy(preprefix, bp, split_len);
            preprefix[split_len] = '\0';
            bp += split_len;
        }
    }
#endif

    if (obj->quan != 1L) {
        if (dknown || !vague_quan)
            /*KR Sprintf(prefix, "%ld ", obj->quan); */
            Sprintf(prefix, "%ld개의 ", obj->quan);
        else
            /*KR Strcpy(prefix, "some "); */
            Strcpy(prefix, "얼마간의 ");
    } else if (obj->otyp == CORPSE) {
        /* skip article prefix for corpses [else corpse_xname()
           would have to be taught how to strip it off again] */
        *prefix = '\0';
#if 0 /*KR 관사(the) 미사용. prefix의 초기화 */
    } else if (obj_is_pname(obj) || the_unique_obj(obj)) {
        if (!strncmpi(bp, "the ", 4))
            bp += 4;
        Strcpy(prefix, "the ");
    } else {
        Strcpy(prefix, "a ");
#else
    } else {
        Strcpy(prefix, "");
#endif
    }

    /* "empty" goes at the beginning, but item count goes at the end */
    if (cknown
        /* bag of tricks: include "empty" prefix if it's known to
           be empty but its precise number of charges isn't known
           (when that is known, suffix of "(n:0)" will be appended,
           making the prefix be redundant; note that 'known' flag
           isn't set when emptiness gets discovered because then
           charging magic would yield known number of new charges) */
        && ((obj->otyp == BAG_OF_TRICKS)
             ? (obj->spe == 0 && !obj->known)
             /* not bag of tricks: empty if container which has no contents */
             : ((Is_container(obj) || obj->otyp == STATUE)
                && !Has_contents(obj))))

        /*KR Strcat(prefix, "empty "); */
        Strcat(prefix, "비어있는 ");

    if (bknown && obj->oclass != COIN_CLASS
        && (obj->otyp != POT_WATER || !objects[POT_WATER].oc_name_known
            || (!obj->cursed && !obj->blessed))) {
        /* allow 'blessed clear potion' if we don't know it's holy water;
         * always allow "uncursed potion of water"
         */
        if (obj->cursed)
            /*KR Strcat(prefix, "cursed "); */
            Strcat(prefix, "저주받은 ");
        else if (obj->blessed)
            /*KR Strcat(prefix, "blessed "); */
            Strcat(prefix, "축복받은 ");
        else if (!iflags.implicit_uncursed
            /* For most items with charges or +/-, if you know how many
             * charges are left or what the +/- is, then you must have
             * totally identified the item, so "uncursed" is unnecessary,
             * because an identified object not described as "blessed" or
             * "cursed" must be uncursed.
             *
             * If the charges or +/- is not known, "uncursed" must be
             * printed to avoid ambiguity between an item whose curse
             * status is unknown, and an item known to be uncursed.
             */
                 || ((!known || !objects[obj->otyp].oc_charged
                      || obj->oclass == ARMOR_CLASS
                      || obj->oclass == RING_CLASS)
#ifdef MAIL
                     && obj->otyp != SCR_MAIL
#endif
                     && obj->otyp != FAKE_AMULET_OF_YENDOR
                     && obj->otyp != AMULET_OF_YENDOR
                     && !Role_if(PM_PRIEST)))
            /*KR Strcat(prefix, "uncursed "); */
            Strcat(prefix, "저주받지 않은 ");
    }

    if (lknown && Is_box(obj)) {
        if (obj->obroken)
            /* 3.6.0 used "unlockable" here but that could be misunderstood
               to mean "capable of being unlocked" rather than the intended
               "not capable of being locked" */
            /*KR Strcat(prefix, "broken "); */
            Strcat(prefix, "부서진 ");
        else if (obj->olocked)
            /*KR Strcat(prefix, "locked "); */
            Strcat(prefix, "잠긴 ");
        else
            /*KR Strcat(prefix, "unlocked "); */
            Strcat(prefix, "잠기지 않은 ");
    }

    if (obj->greased)
        /*KR Strcat(prefix, "greased "); */
        Strcat(prefix, "기름을 바른 ");

    if (cknown && Has_contents(obj)) {
        /* we count the number of separate stacks, which corresponds
           to the number of inventory slots needed to be able to take
           everything out if no merges occur */
        long itemcount = count_contents(obj, FALSE, FALSE, TRUE, FALSE);

#if 0 /*JP*/
        Sprintf(eos(bp), " containing %ld item%s", itemcount,
            plur(itemcount));
#else
        Sprintf(eos(bp), " (아이템 %ld개 들어 있음)", itemcount);
#endif
    }

    switch (is_weptool(obj) ? WEAPON_CLASS : obj->oclass) {
    case AMULET_CLASS:
        if (obj->owornmask & W_AMUL)
            /*KR Strcat(bp, " (being worn)"); */
            Strcat(bp, " (몸에 걸치고 있음)");
        break;
    case ARMOR_CLASS:
        if (obj->owornmask & W_ARMOR) {
            /*KR Strcat(bp, (obj == uskin) ? " (embedded in your skin)" */
            Strcat(bp, (obj == uskin) ? " (몸에 박혀 있음)"
                       /* in case of perm_invent update while Wear/Takeoff
                          is in progress; check doffing() before donning()
                          because donning() returns True for both cases */
#if 0 /*KR:T*/
                                      : doffing(obj) ? " (being doffed)"
                                      : donning(obj) ? " (being donned)"
                                                     : " (being worn)"); */
#else
                                      : doffing(obj) ? " (몸에 걸치는 도중)"
                                      : donning(obj) ? " (벗고 있는 도중)"
                                                     : " (몸에 걸치고 있음)");
#endif
            /* slippery fingers is an intrinsic condition of the hero
               rather than extrinsic condition of objects, but gloves
               are described as slippery when hero has slippery fingers */
            if (obj == uarmg && Glib) /* just appended "(something)",
                                       * change to "(something; slippery)" */
                /*KR Strcpy(rindex(bp, ')'), "; slippery)"); */
                Strcpy(rindex(bp, ')'), "; 미끄러운)");
        }
        /*FALLTHRU*/
    case WEAPON_CLASS:
        if (ispoisoned)
            /*KR Strcat(prefix, "poisoned "); */
            Strcat(prefix, "독이 발린 ");
        add_erosion_words(obj, prefix);
        if (known) {
            Strcat(prefix, sitoa(obj->spe));
            Strcat(prefix, " ");
        }
        break;
    case TOOL_CLASS:
        if (obj->owornmask & (W_TOOL | W_SADDLE)) { /* blindfold */
            /*KR Strcat(bp, " (being worn)"); */
            Strcat(bp, " (몸에 걸치고 있음)");
            break;
        }
        if (obj->otyp == LEASH && obj->leashmon != 0) {
            struct monst *mlsh = find_mid(obj->leashmon, FM_FMON);

            if (!mlsh) {
                impossible("leashed monster not on this level");
                obj->leashmon = 0;
            } else {
#if 0 /*KR:T*/
                Sprintf(eos(bp), " (attached to %s)",
                    noit_mon_nam(mlsh));
#else
                Sprintf(eos(bp), " (%s에 줄로 연결되어 있음)",
                    noit_mon_nam(mlsh));
#endif
            }
            break;
        }
        if (obj->otyp == CANDELABRUM_OF_INVOCATION) {
#if 0 /*KR*/
            if (!obj->spe)
                Strcpy(tmpbuf, "no");
            else
                Sprintf(tmpbuf, "%d", obj->spe);
            Sprintf(eos(bp), " (%s candle%s%s)", tmpbuf, plur(obj->spe),
                !obj->lamplit ? " attached" : ", lit");
#else
            if (!obj->spe)
                Sprintf(eos(bp), "(한 개도 꽂혀있지 않음)");
            else {
                if (!obj->lamplit)
                    Sprintf(eos(bp), "(%d개 꽂혀 있음)", obj->spe);
                else
                    Sprintf(eos(bp), "(%d개 불이 켜져 있음)", obj->spe);
            }
#endif
            break;
        } else if (obj->otyp == OIL_LAMP || obj->otyp == MAGIC_LAMP
                   || obj->otyp == BRASS_LANTERN || Is_candle(obj)) {
            if (Is_candle(obj)
                && obj->age < 20L * (long)objects[obj->otyp].oc_cost)
                /*KR Strcat(prefix, "partly used "); */
                Strcat(prefix, "일부 사용한");
            if (obj->lamplit)
                /*KR Strcat(bp, " (lit)"); */
                Strcat(bp, "(불이 켜져 있음)");
            break;
        }
        if (objects[obj->otyp].oc_charged)
            goto charges;
        break;
    case WAND_CLASS:
 charges:
        if (known)
            Sprintf(eos(bp), " (%d:%d)", (int) obj->recharged, obj->spe);
        break;
    case POTION_CLASS:
        if (obj->otyp == POT_OIL && obj->lamplit)
            /*KR Strcat(bp, " (lit)"); */
            Strcat(bp, " (불이 켜져 있음)");
        break;
    case RING_CLASS:
 ring:
        if (obj->owornmask & W_RINGR)
            /*KR Strcat(bp, " (on right "); */
            Strcat(bp, " (오른쪽 ");
        if (obj->owornmask & W_RINGL)
            Strcat(bp, " (왼쪽 ");
        if (obj->owornmask & W_RING) {
            Strcat(bp, body_part(HAND));
            Strcat(bp, ")");
        }
        if (known && objects[obj->otyp].oc_charged) {
#if 1 /*KR*/
            Strcat(prefix, " ");
#endif
            Strcat(prefix, sitoa(obj->spe));
            Strcat(prefix, " ");
        }
        break;
    case FOOD_CLASS:
        if (obj->oeaten)
            /*KR Strcat(prefix, "partly eaten "); */
            Strcat(prefix, "일부 먹힌 ");
        if (obj->otyp == CORPSE) {
            /* (quan == 1) => want corpse_xname() to supply article,
               (quan != 1) => already have count or "some" as prefix;
               "corpse" is already in the buffer returned by xname() */
            unsigned cxarg = (((obj->quan != 1L) ? 0 : CXN_ARTICLE)
                              | CXN_NOCORPSE);
            char *cxstr = corpse_xname(obj, prefix, cxarg);

#if 0 /*KR*/
            Sprintf(prefix, "%s ", cxstr);
#else
            if (has_oname(obj) && dknown) {
                /* 예: Zaz(이)라는 이름의 저주받은 언덕 오크 */
                Sprintf(prefix, "%s는 이름의 %s ",
                        append_josa(get_kr_name(ONAME(obj)), "이라"), cxstr);
            } else {
                /* 예: 저주받은 언덕 오크의 */
                Sprintf(prefix, "%s의 ", cxstr);
            }
#endif
            /* avoid having doname(corpse) consume an extra obuf */
            releaseobuf(cxstr);
        } else if (obj->otyp == EGG) {
#if 0 /* corpses don't tell if they're stale either */
            if (known && stale_egg(obj))
                Strcat(prefix, "stale ");
#endif
            if (omndx >= LOW_PM
                && (known || (mvitals[omndx].mvflags & MV_KNOWS_EGG))) {
#if 0 /*KR:T*/
                Strcat(prefix, mons[omndx].mname);
                Strcat(prefix, " ");
#else
                Strcat(prefix, mons[omndx].mname);
                Strcat(prefix, "의");
#endif
                if (obj->spe)
                    /*KR Strcat(bp, " (laid by you)"); */
                    Strcat(bp, " (당신이 낳음)");
            }
        }
        if (obj->otyp == MEAT_RING)
            goto ring;
        break;
    case BALL_CLASS:
    case CHAIN_CLASS:
        add_erosion_words(obj, prefix);
        if (obj->owornmask & W_BALL)
            /*KR Strcat(bp, " (chained to you)"); */
            Strcat(bp, " (당신에게 연결되어 있음)");
        break;
    }

    if ((obj->owornmask & W_WEP) && !mrg_to_wielded) {
        if (obj->quan != 1L) {
            /*KR Strcat(bp, " (wielded)"); */
            Strcat(bp, " (장비중)");
        } else {
            const char *hand_s = body_part(HAND);

            if (bimanual(obj))
                hand_s = makeplural(hand_s);
            /* note: Sting's glow message, if added, will insert text
               in front of "(weapon in hand)"'s closing paren */
#if 0 /*KR*/
            Sprintf(eos(bp), " (%sweapon in %s)",
                (obj->otyp == AKLYS) ? "tethered " : "", hand_s);
#else /*aklys를 특별 취급하지 않음*/
            Sprintf(eos(bp), "(%s에 쥐고 있음)", hand_s);
#endif

            if (warn_obj_cnt && obj == uwep && (EWarn_of_mon & W_WEP) != 0L) {
                if (!Blind) /* we know bp[] ends with ')'; overwrite that */
#if 0 /*KR:T 한국어 어순(색상 -> 동사)에 맞춰 순서 변경. */
                    Sprintf(eos(bp) - 1, ", %s %s)",
                            glow_verb(warn_obj_cnt, TRUE),
                            glow_color(obj->oartifact));
#else /*KR glow_color [붉은색 등] glow_verb [빛나는 등] 이라 가정 */
                    Sprintf(eos(bp) - 1, ", %s으로 %s)",
                            glow_color(obj->oartifact),
                            glow_verb(warn_obj_cnt, TRUE));
#endif /*KR: "붉은색으로 빛나는" 등으로 표시됨을 상정 */
            }
        }
    }
    if (obj->owornmask & W_SWAPWEP) {
        if (u.twoweap)
       /*KR Sprintf(eos(bp), " (wielded in other %s)", body_part(HAND));
        else
            Strcat(bp, " (alternate weapon; not wielded)"); */
            Sprintf(eos(bp), " (왼%s에 쥐고 있음)", body_part(HAND));
        else
            Strcat(bp, " (예비 무기; 장비하지 않음)");
    }
    if (obj->owornmask & W_QUIVER) {
        switch (obj->oclass) {
        case WEAPON_CLASS:
            if (is_ammo(obj)) {
                if (objects[obj->otyp].oc_skill == -P_BOW) {
                    /* Ammo for a bow */
                    /*KR Strcat(bp, " (in quiver)"); */
                    Strcat(bp, " (화살통에 들어 있음)");
                    break;
                } else {
                    /* Ammo not for a bow */
                    /*KR Strcat(bp, " (in quiver pouch)"); */
                    Strcat(bp, " (탄주머니에 들어 있음)");
                    break;
                }
            } else {
                /* Weapons not considered ammo */
                /*KR Strcat(bp, " (at the ready)"); */
                Strcat(bp, " (준비하고 있음)");
                break;
            }
        /* Small things and ammo not for a bow */
        case RING_CLASS:
        case AMULET_CLASS:
        case WAND_CLASS:
        case COIN_CLASS:
        case GEM_CLASS:
            /*KR Strcat(bp, " (in quiver pouch)"); */
            Strcat(bp, " (탄주머니에 들어 있음)");
            break;
        default: /* odd things */
            /*KR Strcat(bp, " (at the ready)"); */
            Strcat(bp, " (준비하고 있음)");
        }
    }
    /* treat 'restoring' like suppress_price because shopkeeper and
       bill might not be available yet while restore is in progress
       (objects won't normally be formatted during that time, but if
       'perm_invent' is enabled then they might be) */
    if (iflags.suppress_price || restoring) {
        ; /* don't attempt to obtain any stop pricing, even if 'with_price' */
    } else if (is_unpaid(obj)) { /* in inventory or in container in invent */
        long quotedprice = unpaid_cost(obj, TRUE);

#if 0 /*KR:T*/
        Sprintf(eos(bp), " (%s, %s%ld %s)",
                obj->unpaid ? "unpaid" : "contents",
                globwt(obj, globbuf, &weightshown),
                quotedprice, currency(quotedprice));
#else
        Sprintf(eos(bp), " (%s, %s%ld %s)",
                obj->unpaid ? "미지불" : "내용물",
                globwt(obj, globbuf, &weightshown), 
                quotedprice, currency(quotedprice));
#endif
    } else if (with_price) { /* on floor or in container on floor */
        int nochrg = 0;
        long price = get_cost_of_shop_item(obj, &nochrg);

        if (price > 0L)
#if 0 /*KR:T*/
            Sprintf(eos(bp), " (%s, %s%ld %s)",
                    nochrg ? "contents" : "for sale",
                    globwt(obj, globbuf, &weightshown),
                    price, currency(price));
#else
            Sprintf(eos(bp), " (%s, %s%ld %s)", 
                    nochrg ? "내용물" : "상품",
                    globwt(obj, globbuf, &weightshown), 
                    price, currency(price));
#endif
        else if (nochrg > 0)
#if 0 /*KR:T*/
            Sprintf(eos(bp), " (%sno charge)",
                    globwt(obj, globbuf, &weightshown));
#else
            Sprintf(eos(bp), " (%s무료)",
                    globwt(obj, globbuf, &weightshown));
#endif
    }
#if 0 /*KR*//*한국어에서는 미사용*/
    if (!strncmp(prefix, "a ", 2)) {
        /* save current prefix, without "a "; might be empty */
        Strcpy(tmpbuf, prefix + 2);
        /* set prefix[] to "", "a ", or "an " */
        (void)just_an(prefix, *tmpbuf ? tmpbuf : bp);
        /* append remainder of original prefix */
        Strcat(prefix, tmpbuf);
    }
#endif

    /* show weight for items (debug tourist info);
       "aum" is stolen from Crawl's "Arbitrary Unit of Measure" */
    if (wizard && iflags.wizweight) {
        /* wizard mode user has asked to see object weights;
           globs with shop pricing attached already include it */
        if (!weightshown)
            Sprintf(eos(bp), " (%u aum)", obj->owt);
    }
#if 0 /*KR*/
    bp = strprepend(bp, prefix);
#else /*KR: '이름 붙인'을 되돌린다 */
    Strcat(preprefix, prefix);
    bp = strprepend(bp, preprefix);
#endif
    return bp;
}

char *
doname(obj)
struct obj *obj;
{
    return doname_base(obj, (unsigned) 0);
}

/* Name of object including price. */
char *
doname_with_price(obj)
struct obj *obj;
{
    return doname_base(obj, DONAME_WITH_PRICE);
}

/* "some" instead of precise quantity if obj->dknown not set */
char *
doname_vague_quan(obj)
struct obj *obj;
{
    /* Used by farlook.
     * If it hasn't been seen up close and quantity is more than one,
     * use "some" instead of the quantity: "some gold pieces" rather
     * than "25 gold pieces".  This is suboptimal, to put it mildly,
     * because lookhere and pickup report the precise amount.
     * Picking the item up while blind also shows the precise amount
     * for inventory display, then dropping it while still blind leaves
     * obj->dknown unset so the count reverts to "some" for farlook.
     *
     * TODO: add obj->qknown flag for 'quantity known' on stackable
     * items; it could overlay obj->cknown since no containers stack.
     */
    return doname_base(obj, DONAME_VAGUE_QUAN);
}

/* used from invent.c */
boolean
not_fully_identified(otmp)
struct obj *otmp;
{
    /* gold doesn't have any interesting attributes [yet?] */
    if (otmp->oclass == COIN_CLASS)
        return FALSE; /* always fully ID'd */
    /* check fundamental ID hallmarks first */
    if (!otmp->known || !otmp->dknown
#ifdef MAIL
        || (!otmp->bknown && otmp->otyp != SCR_MAIL)
#else
        || !otmp->bknown
#endif
        || !objects[otmp->otyp].oc_name_known)
        return TRUE;
    if ((!otmp->cknown && (Is_container(otmp) || otmp->otyp == STATUE))
        || (!otmp->lknown && Is_box(otmp)))
        return TRUE;
    if (otmp->oartifact && undiscovered_artifact(otmp->oartifact))
        return TRUE;
    /* otmp->rknown is the only item of interest if we reach here */
    /*
     *  Note:  if a revision ever allows scrolls to become fireproof or
     *  rings to become shockproof, this checking will need to be revised.
     *  `rknown' ID only matters if xname() will provide the info about it.
     */
    if (otmp->rknown
        || (otmp->oclass != ARMOR_CLASS && otmp->oclass != WEAPON_CLASS
            && !is_weptool(otmp)            /* (redundant) */
            && otmp->oclass != BALL_CLASS)) /* (useless) */
        return FALSE;
    else /* lack of `rknown' only matters for vulnerable objects */
        return (boolean) (is_rustprone(otmp) || is_corrodeable(otmp)
                          || is_flammable(otmp));
}

/* format a corpse name (xname() omits monster type; doname() calls us);
   eatcorpse() also uses us for death reason when eating tainted glob */
char *
corpse_xname(otmp, adjective, cxn_flags)
struct obj *otmp;
const char *adjective;
unsigned cxn_flags; /* bitmask of CXN_xxx values */
{
    /* some callers [aobjnam()] rely on prefix area that xname() sets aside */
    char *nambuf = nextobuf() + PREFIX;
    int omndx = otmp->corpsenm;
#if 0 /*KR*/
    boolean ignore_quan = (cxn_flags & CXN_SINGULAR) != 0,
            /* suppress "the" from "the unique monster corpse" */
#else
    boolean
#endif
        no_prefix = (cxn_flags & CXN_NO_PFX) != 0,
            /* include "the" for "the woodchuck corpse */
        the_prefix = (cxn_flags & CXN_PFX_THE) != 0,
            /* include "an" for "an ogre corpse */
        any_prefix = (cxn_flags & CXN_ARTICLE) != 0,
            /* leave off suffix (do_name() appends "corpse" itself) */
        omit_corpse = (cxn_flags & CXN_NOCORPSE) != 0,
        possessive = FALSE,
        glob = (otmp->otyp != CORPSE && otmp->globby);
    const char *mname;

    if (glob) {
        mname = OBJ_NAME(objects[otmp->otyp]); /* "glob of <monster>" */
    } else if (omndx == NON_PM) { /* paranoia */
        /*KR mname = "thing"; */
        mname = "무언가";
        /* [Possible enhancement:  check whether corpse has monster traits
            attached in order to use priestname() for priests and minions.] */
    } else if (omndx == PM_ALIGNED_PRIEST) {
        /* avoid "aligned priest"; it just exposes internal details */
        /*KR mname = "priest"; */
        mname = "사제";
    } else {
#if 0 /*KR*/
        mname = mons[omndx].mname;
        if (the_unique_pm(&mons[omndx]) || type_is_pname(&mons[omndx])) {
            mname = s_suffix(mname);
            possessive = TRUE;
#else
        /* 몬스터 이름을 한글로 변환합니다. */
        mname = get_kr_name(mons[omndx].mname);
        if (the_unique_pm(&mons[omndx]) || type_is_pname(&mons[omndx])) {
            /* 한국어는 뒤에서 "의 사체"를 일괄적으로 붙일 것이므로
             * s_suffix("~'s")를 씌우지 않으며, 어순을 꼬는 possessive도 쓰지
             * 않습니다. */
            possessive = FALSE;
#endif
            /* don't precede personal name like "Medusa" with an article */
            if (type_is_pname(&mons[omndx]))
                no_prefix = TRUE;
            /* always precede non-personal unique monster name like
               "Oracle" with "the" unless explicitly overridden */
            else if (the_unique_pm(&mons[omndx]) && !no_prefix)
                the_prefix = TRUE;
        }
    }
    if (no_prefix)
        the_prefix = any_prefix = FALSE;
    else if (the_prefix)
        any_prefix = FALSE; /* mutually exclusive */

    *nambuf = '\0';
    /* can't use the() the way we use an() below because any capitalized
       Name causes it to assume a personal name and return Name as-is;
       that's usually the behavior wanted, but here we need to force "the"
       to precede capitalized unique monsters (pnames are handled above) */
#if 0 /*KR 한국어에서는 정관사가 불필요 */
    if (the_prefix)
        Strcat(nambuf, "the ");
#endif

    if (!adjective || !*adjective) {
        /* normal case:  newt corpse */
        Strcat(nambuf, mname);
    } else {
        /* adjective positioning depends upon format of monster name */
        if (possessive) /* Medusa's cursed partly eaten corpse */
            Sprintf(eos(nambuf), "%s %s", mname, adjective);
        else /* cursed partly eaten troll corpse */
            Sprintf(eos(nambuf), "%s %s", adjective, mname);
        /* in case adjective has a trailing space, squeeze it out */
        mungspaces(nambuf);
        /* doname() might include a count in the adjective argument;
           if so, don't prepend an article */
        if (digit(*adjective))
            any_prefix = FALSE;
    }

    if (glob) {
        ; /* omit_corpse doesn't apply; quantity is always 1 */
    } else if (!omit_corpse) {
#if 0 /*KR*/
        Strcat(nambuf, " corpse");
        /* makeplural(nambuf) => append "s" to "corpse" */
        if (otmp->quan > 1L && !ignore_quan) {
            Strcat(nambuf, "s");
            any_prefix = FALSE; /* avoid "a newt corpses" */
        }
#else
        Strcat(nambuf, "의 사체");
#endif
    }

    /* it's safe to overwrite our nambuf after an() has copied
       its old value into another buffer */
    if (any_prefix)
        Strcpy(nambuf, an(nambuf));

    return nambuf;
}

/* xname doesn't include monster type for "corpse"; cxname does */
char *
cxname(obj)
struct obj *obj;
{
    if (obj->otyp == CORPSE)
        return corpse_xname(obj, (const char *) 0, CXN_NORMAL);
    return xname(obj);
}

/* like cxname, but ignores quantity */
char *
cxname_singular(obj)
struct obj *obj;
{
    if (obj->otyp == CORPSE)
        return corpse_xname(obj, (const char *) 0, CXN_SINGULAR);
    return xname_flags(obj, CXN_SINGULAR);
}

/* treat an object as fully ID'd when it might be used as reason for death */
char *
killer_xname(obj)
struct obj *obj;
{
    struct obj save_obj;
    unsigned save_ocknown;
    char *buf, *save_ocuname, *save_oname = (char *) 0;

    /* bypass object twiddling for artifacts */
    if (obj->oartifact)
        return bare_artifactname(obj);

    /* remember original settings for core of the object;
       oextra structs other than oname don't matter here--since they
       aren't modified they don't need to be saved and restored */
    save_obj = *obj;
    if (has_oname(obj))
        save_oname = ONAME(obj);

    /* killer name should be more specific than general xname; however, exact
       info like blessed/cursed and rustproof makes things be too verbose */
    obj->known = obj->dknown = 1;
    obj->bknown = obj->rknown = obj->greased = 0;
    /* if character is a priest[ess], bknown will get toggled back on */
    if (obj->otyp != POT_WATER)
        obj->blessed = obj->cursed = 0;
    else
        obj->bknown = 1; /* describe holy/unholy water as such */
    /* "killed by poisoned <obj>" would be misleading when poison is
       not the cause of death and "poisoned by poisoned <obj>" would
       be redundant when it is, so suppress "poisoned" prefix */
    obj->opoisoned = 0;
    /* strip user-supplied name; artifacts keep theirs */
    if (!obj->oartifact && save_oname)
        ONAME(obj) = (char *) 0;
    /* temporarily identify the type of object */
    save_ocknown = objects[obj->otyp].oc_name_known;
    objects[obj->otyp].oc_name_known = 1;
    save_ocuname = objects[obj->otyp].oc_uname;
    objects[obj->otyp].oc_uname = 0; /* avoid "foo called bar" */

    /* format the object */
    if (obj->otyp == CORPSE) {
        buf = corpse_xname(obj, (const char *) 0, CXN_NORMAL);
    } else if (obj->otyp == SLIME_MOLD) {
        /* concession to "most unique deaths competition" in the annual
           devnull tournament, suppress player supplied fruit names because
           those can be used to fake other objects and dungeon features */
        buf = nextobuf();
        /*KR Sprintf(buf, "deadly slime mold%s", plur(obj->quan)); */
        Strcpy(buf, "치명적인 점액질 곰팡이");
    } else {
        buf = xname(obj);
    }
    /* apply an article if appropriate; caller should always use KILLED_BY */
#if 0 /*KR 한국어에서는 불필요 */
    if (obj->quan == 1L && !strstri(buf, "'s ") && !strstri(buf, "s' "))
        buf = (obj_is_pname(obj) || the_unique_obj(obj)) ? the(buf) : an(buf);
#endif

    objects[obj->otyp].oc_name_known = save_ocknown;
    objects[obj->otyp].oc_uname = save_ocuname;
    *obj = save_obj; /* restore object's core settings */
    if (!obj->oartifact && save_oname)
        ONAME(obj) = save_oname;

    return buf;
}

/* xname,doname,&c with long results reformatted to omit some stuff */
char *
short_oname(obj, func, altfunc, lenlimit)
struct obj *obj;
char *FDECL((*func), (OBJ_P)),    /* main formatting routine */
     *FDECL((*altfunc), (OBJ_P)); /* alternate for shortest result */
unsigned lenlimit;
{
    struct obj save_obj;
    char unamebuf[12], onamebuf[12], *save_oname, *save_uname, *outbuf;

    outbuf = (*func)(obj);
    if ((unsigned) strlen(outbuf) <= lenlimit)
        return outbuf;

    /* shorten called string to fairly small amount */
    save_uname = objects[obj->otyp].oc_uname;
    if (save_uname && strlen(save_uname) >= sizeof unamebuf) {
        (void) strncpy(unamebuf, save_uname, sizeof unamebuf - 4);
        Strcpy(unamebuf + sizeof unamebuf - 4, "...");
        objects[obj->otyp].oc_uname = unamebuf;
        releaseobuf(outbuf);
        outbuf = (*func)(obj);
        objects[obj->otyp].oc_uname = save_uname; /* restore called string */
        if ((unsigned) strlen(outbuf) <= lenlimit)
            return outbuf;
    }

    /* shorten named string to fairly small amount */
    save_oname = has_oname(obj) ? ONAME(obj) : 0;
    if (save_oname && strlen(save_oname) >= sizeof onamebuf) {
        (void) strncpy(onamebuf, save_oname, sizeof onamebuf - 4);
        Strcpy(onamebuf + sizeof onamebuf - 4, "...");
        ONAME(obj) = onamebuf;
        releaseobuf(outbuf);
        outbuf = (*func)(obj);
        ONAME(obj) = save_oname; /* restore named string */
        if ((unsigned) strlen(outbuf) <= lenlimit)
            return outbuf;
    }

    /* shorten both called and named strings;
       unamebuf and onamebuf have both already been populated */
    if (save_uname && strlen(save_uname) >= sizeof unamebuf && save_oname
        && strlen(save_oname) >= sizeof onamebuf) {
        objects[obj->otyp].oc_uname = unamebuf;
        ONAME(obj) = onamebuf;
        releaseobuf(outbuf);
        outbuf = (*func)(obj);
        if ((unsigned) strlen(outbuf) <= lenlimit) {
            objects[obj->otyp].oc_uname = save_uname;
            ONAME(obj) = save_oname;
            return outbuf;
        }
    }

    /* still long; strip several name-lengthening attributes;
       called and named strings are still in truncated form */
    save_obj = *obj;
    obj->bknown = obj->rknown = obj->greased = 0;
    obj->oeroded = obj->oeroded2 = 0;
    releaseobuf(outbuf);
    outbuf = (*func)(obj);
    if (altfunc && (unsigned) strlen(outbuf) > lenlimit) {
        /* still long; use the alternate function (usually one of
           the jackets around minimal_xname()) */
        releaseobuf(outbuf);
        outbuf = (*altfunc)(obj);
    }
    /* restore the object */
    *obj = save_obj;
    if (save_oname)
        ONAME(obj) = save_oname;
    if (save_uname)
        objects[obj->otyp].oc_uname = save_uname;

    /* use whatever we've got, whether it's too long or not */
    return outbuf;
}

/*
 * Used if only one of a collection of objects is named (e.g. in eat.c).
 */
const char *
singular(otmp, func)
register struct obj *otmp;
char *FDECL((*func), (OBJ_P));
{
    long savequan;
    char *nam;

    /* using xname for corpses does not give the monster type */
    if (otmp->otyp == CORPSE && func == xname)
        func = cxname;

    savequan = otmp->quan;
    otmp->quan = 1L;
    nam = (*func)(otmp);
    otmp->quan = savequan;
    return nam;
}

/* pick "", "a ", or "an " as article for 'str'; used by an() and doname() */
STATIC_OVL char *
just_an(outbuf, str)
char *outbuf;
const char *str;
{
#if 0 /*KR 한국어에서는 부정관사는 불필요 */
    char c0;

    *outbuf = '\0';
    c0 = lowc(*str);
    if (!str[1]) {
        /* single letter; might be used for named fruit */
        Strcpy(outbuf, index("aefhilmnosx", c0) ? "an " : "a ");
    } else if (!strncmpi(str, "the ", 4) || !strcmpi(str, "molten lava")
               || !strcmpi(str, "iron bars") || !strcmpi(str, "ice")) {
        ; /* no article */
    } else {
        if ((index(vowels, c0) && strncmpi(str, "one-", 4)
             && strncmpi(str, "eucalyptus", 10) && strncmpi(str, "unicorn", 7)
             && strncmpi(str, "uranium", 7) && strncmpi(str, "useful", 6))
            || (index("x", c0) && !index(vowels, lowc(str[1]))))
            Strcpy(outbuf, "an ");
        else
            Strcpy(outbuf, "a ");
    }
#else
    *outbuf = '\0';
#endif
    return outbuf;
}

char *an(str) const char *str;
{
    char *buf = nextobuf();

    if (!str || !*str) {
        impossible("Alphabet soup: 'an(%s)'.", str ? "\"\"" : "<null>");
        return strcpy(buf, "an []");
    }
    (void) just_an(buf, str);
    return strcat(buf, str);
}

char *An(str) const char *str;
{
    char *tmp = an(str);

#if 0 /*KR 대문자화하지 않는다 */
    *tmp = highc(*tmp);
#endif
    return tmp;
}

/*
 * Prepend "the" if necessary; assumes str is a subject derived from xname.
 * Use type_is_pname() for monster names, not the().  the() is idempotent.
 */
char *the(str) const char *str;
{
#if 0 /*KR*/
    char *buf = nextobuf();
    boolean insert_the = FALSE;

    if (!str || !*str) {
        impossible("Alphabet soup: 'the(%s)'.", str ? "\"\"" : "<null>");
        return strcpy(buf, "the []");
    }
    if (!strncmpi(str, "the ", 4)) {
        buf[0] = lowc(*str);
        Strcpy(&buf[1], str + 1);
        return buf;
    } else if (*str < 'A' || *str > 'Z'
               /* treat named fruit as not a proper name, even if player
                  has assigned a capitalized proper name as his/her fruit */
               || fruit_from_name(str, TRUE, (int *) 0)) {
        /* not a proper name, needs an article */
        insert_the = TRUE;
    } else {
        /* Probably a proper name, might not need an article */
        register char *tmp, *named, *called;
        int l;

        /* some objects have capitalized adjectives in their names */
        if (((tmp = rindex(str, ' ')) != 0 || (tmp = rindex(str, '-')) != 0)
            && (tmp[1] < 'A' || tmp[1] > 'Z')) {
            insert_the = TRUE;
        } else if (tmp && index(str, ' ') < tmp) { /* has spaces */
            /* it needs an article if the name contains "of" */
            tmp = strstri(str, " of ");
            named = strstri(str, " named ");
            called = strstri(str, " called ");
            if (called && (!named || called < named))
                named = called;

            if (tmp && (!named || tmp < named)) /* found an "of" */
                insert_the = TRUE;
            /* stupid special case: lacks "of" but needs "the" */
            else if (!named && (l = strlen(str)) >= 31
                     && !strcmp(&str[l - 31],
                                "Platinum Yendorian Express Card"))
                insert_the = TRUE;
        }
    }
    if (insert_the)
        Strcpy(buf, "the ");
    else
        buf[0] = '\0';
    Strcat(buf, str);

    return buf;
#else /*KR 한국어는 정관사(the)를 사용하지 않습니다. */
    char *buf = nextobuf();
    if (str)
        Strcpy(buf, str);
    else
        buf[0] = '\0';
    return buf;
#endif
}

char *
The(str)
const char *str;
{
#if 0 /*KR*/
    char *tmp = the(str);

    *tmp = highc(*tmp);
    return tmp;
#else
    char *buf = nextobuf();
    if (str)
        Strcpy(buf, str);
    else
        buf[0] = '\0';
    return buf;
#endif
}

#if 0 /*KR:T*/
/* returns "count cxname(otmp)" or just cxname(otmp) if count == 1 */
char *
aobjnam(otmp, verb)
struct obj *otmp;
const char *verb;
{
    char prefix[PREFIX];
    char *bp = cxname(otmp);

    if (otmp->quan != 1L) {
        Sprintf(prefix, "%ld ", otmp->quan);
        bp = strprepend(bp, prefix);
    }
    if (verb) {
        Strcat(bp, " ");
        Strcat(bp, otense(otmp, verb));
    }
    return bp;
}
#else
char *
aobjnam(otmp, verb)
register struct obj *otmp;
register const char *verb;
{
    return xname(otmp);
}
#endif

/* combine yname and aobjnam eg "your count cxname(otmp)" */
char *
yobjnam(obj, verb)
struct obj *obj;
const char *verb;
{
    char *s = aobjnam(obj, verb);

    /* leave off "your" for most of your artifacts, but prepend
     * "your" for unique objects and "foo of bar" quest artifacts */
    if (!carried(obj) || !obj_is_pname(obj)
        || obj->oartifact >= ART_ORB_OF_DETECTION) {
        char *outbuf = shk_your(nextobuf(), obj);
        int space_left = BUFSZ - 1 - strlen(outbuf);

        s = strncat(outbuf, s, space_left);
    }
    return s;
}

/* combine Yname2 and aobjnam eg "Your count cxname(otmp)" */
char *
Yobjnam2(obj, verb)
struct obj *obj;
const char *verb;
{
    register char *s = yobjnam(obj, verb);

#if 0 /*KR*/
    *s = highc(*s);
#endif
    return s;
}

/* like aobjnam, but prepend "The", not count, and use xname */
char *
Tobjnam(otmp, verb)
struct obj *otmp;
const char *verb;
{
    char *bp = The(xname(otmp));

#if 0 /*KR 한국어에는 3인칭 단수 현재형 s가 없다 */
    if (verb) {
        Strcat(bp, " ");
        Strcat(bp, otense(otmp, verb));
    }
#endif
    return bp;
}

/* capitalized variant of doname() */
char *
Doname2(obj)
struct obj *obj;
{
    char *s = doname(obj);

    *s = highc(*s);
    return s;
}

#if 0 /* stalled-out work in progress */
/* Doname2() for itemized buying of 'obj' from a shop */
char *
payDoname(obj)
struct obj *obj;
{
    static const char and_contents[] = " and its contents";
    char *p = doname(obj);

    if (Is_container(obj) && !obj->cknown) {
        if (obj->unpaid) {
            if ((int) strlen(p) + sizeof and_contents - 1 < BUFSZ - PREFIX)
                Strcat(p, and_contents);
            *p = highc(*p);
        } else {
            p = strprepend(p, "Contents of ");
        }
    } else {
        *p = highc(*p);
    }
    return p;
}
#endif /*0*/

/* returns "[your ]xname(obj)" or "Foobar's xname(obj)" or "the xname(obj)" */
char *
yname(obj)
struct obj *obj;
{
    char *s = cxname(obj);

    /* leave off "your" for most of your artifacts, but prepend
     * "your" for unique objects and "foo of bar" quest artifacts */
    if (!carried(obj) || !obj_is_pname(obj)
        || obj->oartifact >= ART_ORB_OF_DETECTION) {
        char *outbuf = shk_your(nextobuf(), obj);
        int space_left = BUFSZ - 1 - strlen(outbuf);

        s = strncat(outbuf, s, space_left);
    }

    return s;
}

/* capitalized variant of yname() */
char *
Yname2(obj)
struct obj *obj;
{
    char *s = yname(obj);

#if 0 /*KR*/
    *s = highc(*s);
#endif
    return s;
}

/* returns "your minimal_xname(obj)"
 * or "Foobar's minimal_xname(obj)"
 * or "the minimal_xname(obj)"
 */
char *
ysimple_name(obj)
struct obj *obj;
{
    char *outbuf = nextobuf();
    char *s = shk_your(outbuf, obj); /* assert( s == outbuf ); */
#if 0 /*KR*/
    int space_left = BUFSZ - 1 - strlen(s);

    return strncat(s, minimal_xname(obj), space_left);
#else
    int space_left = BUFSZ - strlen(s);

    return strncat(s, minimal_xname(obj), space_left);
#endif
}

/* capitalized variant of ysimple_name() */
char *
Ysimple_name2(obj)
struct obj *obj;
{
    char *s = ysimple_name(obj);

#if 0 /*KR*/
    *s = highc(*s);
#endif
    return s;
}

/* "scroll" or "scrolls" */
char *
simpleonames(obj)
struct obj *obj;
{
    char *simpleoname = minimal_xname(obj);

#if 0 /*KR 한국어는 단수 복수가 동일한 형태 */
    if (obj->quan != 1L)
        simpleoname = makeplural(simpleoname);
#endif
    return simpleoname;
}

/* "a scroll" or "scrolls"; "a silver bell" or "the Bell of Opening" */
char *
ansimpleoname(obj)
struct obj *obj;
{
    char *simpleoname = simpleonames(obj);
    int otyp = obj->otyp;

    /* prefix with "the" if a unique item, or a fake one imitating same,
       has been formatted with its actual name (we let typename() handle
       any `known' and `dknown' checking necessary) */
    if (otyp == FAKE_AMULET_OF_YENDOR)
        otyp = AMULET_OF_YENDOR;
    if (objects[otyp].oc_unique
        && !strcmp(simpleoname, OBJ_NAME(objects[otyp])))
        return the(simpleoname);

    /* simpleoname is singular if quan==1, plural otherwise */
    if (obj->quan == 1L)
        simpleoname = an(simpleoname);
    return simpleoname;
}

/* "the scroll" or "the scrolls" */
char *
thesimpleoname(obj)
struct obj *obj;
{
    char *simpleoname = simpleonames(obj);

    return the(simpleoname);
}

/* artifact's name without any object type or known/dknown/&c feedback */
char *
bare_artifactname(obj)
struct obj *obj;
{
    char *outbuf;

    if (obj->oartifact) {
        outbuf = nextobuf();
#if 0 /*KR: 원본*/
        Strcpy(outbuf, artiname(obj->oartifact));
#else /*KR: KRNethack 맞춤 번역 */
        Strcpy(outbuf, get_kr_name(artiname(obj->oartifact)));
#endif
#if 0 /*KR*/
        if (!strncmp(outbuf, "The ", 4))
            outbuf[0] = lowc(outbuf[0]);
#endif
    } else {
        outbuf = xname(obj);
    }
    return outbuf;
}


static const char *wrp[] = {
    "wand",   "ring",      "potion",     "scroll", "gem",
    "amulet", "spellbook", "spell book",
    /* for non-specific wishes */
    "weapon", "armor",     "tool",       "food",   "comestible",
};

static const char wrpsym[] = { WAND_CLASS,   RING_CLASS,   POTION_CLASS,
                               SCROLL_CLASS, GEM_CLASS,    AMULET_CLASS,
                               SPBOOK_CLASS, SPBOOK_CLASS, WEAPON_CLASS,
                               ARMOR_CLASS,  TOOL_CLASS,   FOOD_CLASS,
                               FOOD_CLASS };

#if 0 /*KR 한국어에는 3인칭 단수 현재형 s가 없다 */
/* return form of the verb (input plural) if xname(otmp) were the subject */
char *
otense(otmp, verb)
struct obj *otmp;
const char *verb;
{
    char *buf;

    /*
     * verb is given in plural (without trailing s).  Return as input
     * if the result of xname(otmp) would be plural.  Don't bother
     * recomputing xname(otmp) at this time.
     */
    if (!is_plural(otmp))
        return vtense((char *) 0, verb);

    buf = nextobuf();
    Strcpy(buf, verb);
    return buf;
}

/* various singular words that vtense would otherwise categorize as plural;
   also used by makesingular() to catch some special cases */
static const char *const special_subjs[] = {
    "erinys",  "manes", /* this one is ambiguous */
    "Cyclops", "Hippocrates",     "Pelias",    "aklys",
    "amnesia", "detect monsters", "paralysis", "shape changers",
    "nemesis", 0
    /* note: "detect monsters" and "shape changers" are normally
       caught via "<something>(s) of <whatever>", but they can be
       wished for using the shorter form, so we include them here
       to accommodate usage by makesingular during wishing */
};

/* return form of the verb (input plural) for present tense 3rd person subj */
char *
vtense(subj, verb)
register const char *subj;
register const char *verb;
{
    char *buf = nextobuf(), *bspot;
    int len, ltmp;
    const char *sp, *spot;
    const char *const *spec;

    /*
     * verb is given in plural (without trailing s).  Return as input
     * if subj appears to be plural.  Add special cases as necessary.
     * Many hard cases can already be handled by using otense() instead.
     * If this gets much bigger, consider decomposing makeplural.
     * Note: monster names are not expected here (except before corpse).
     *
     * Special case: allow null sobj to get the singular 3rd person
     * present tense form so we don't duplicate this code elsewhere.
     */
    if (subj) {
        if (!strncmpi(subj, "a ", 2) || !strncmpi(subj, "an ", 3))
            goto sing;
        spot = (const char *) 0;
        for (sp = subj; (sp = index(sp, ' ')) != 0; ++sp) {
            if (!strncmpi(sp, " of ", 4) || !strncmpi(sp, " from ", 6)
                || !strncmpi(sp, " called ", 8) || !strncmpi(sp, " named ", 7)
                || !strncmpi(sp, " labeled ", 9)) {
                if (sp != subj)
                    spot = sp - 1;
                break;
            }
        }
        len = (int) strlen(subj);
        if (!spot)
            spot = subj + len - 1;

        /*
         * plural: anything that ends in 's', but not '*us' or '*ss'.
         * Guess at a few other special cases that makeplural creates.
         */
        if ((lowc(*spot) == 's' && spot != subj
             && !index("us", lowc(*(spot - 1))))
            || !BSTRNCMPI(subj, spot - 3, "eeth", 4)
            || !BSTRNCMPI(subj, spot - 3, "feet", 4)
            || !BSTRNCMPI(subj, spot - 1, "ia", 2)
            || !BSTRNCMPI(subj, spot - 1, "ae", 2)) {
            /* check for special cases to avoid false matches */
            len = (int) (spot - subj) + 1;
            for (spec = special_subjs; *spec; spec++) {
                ltmp = strlen(*spec);
                if (len == ltmp && !strncmpi(*spec, subj, len))
                    goto sing;
                /* also check for <prefix><space><special_subj>
                   to catch things like "the invisible erinys" */
                if (len > ltmp && *(spot - ltmp) == ' '
                    && !strncmpi(*spec, spot - ltmp + 1, ltmp))
                    goto sing;
            }

            return strcpy(buf, verb);
        }
        /*
         * 3rd person plural doesn't end in telltale 's';
         * 2nd person singular behaves as if plural.
         */
        if (!strcmpi(subj, "they") || !strcmpi(subj, "you"))
            return strcpy(buf, verb);
    }

 sing:
    Strcpy(buf, verb);
    len = (int) strlen(buf);
    bspot = buf + len - 1;

    if (!strcmpi(buf, "are")) {
        Strcasecpy(buf, "is");
    } else if (!strcmpi(buf, "have")) {
        Strcasecpy(bspot - 1, "s");
    } else if (index("zxs", lowc(*bspot))
               || (len >= 2 && lowc(*bspot) == 'h'
                   && index("cs", lowc(*(bspot - 1))))
               || (len == 2 && lowc(*bspot) == 'o')) {
        /* Ends in z, x, s, ch, sh; add an "es" */
        Strcasecpy(bspot + 1, "es");
    } else if (lowc(*bspot) == 'y' && !index(vowels, lowc(*(bspot - 1)))) {
        /* like "y" case in makeplural */
        Strcasecpy(bspot, "ies");
    } else {
        Strcasecpy(bspot + 1, "s");
    }

    return buf;
}
#else /*KR: KRNethack 맞춤 번역 */
/* (외부 파일의 호출 에러 방지를 위해 원형 반환으로 복구) */
char *
otense(otmp, verb)
struct obj *otmp;
const char *verb;
{
    char *buf = nextobuf();
    Strcpy(buf, verb);
    return buf;
}

char *
vtense(subj, verb)
register const char *subj;
register const char *verb;
{
    char *buf = nextobuf();
    Strcpy(buf, verb);
    return buf;
}
#endif

#if 0 /*KR*/
struct sing_plur {
    const char *sing, *plur;
};

/* word pairs that don't fit into formula-based transformations;
   also some suffices which have very few--often one--matches or
   which aren't systematically reversible (knives, staves) */
static struct sing_plur one_off[] = {
    { "child",
      "children" },      /* (for wise guys who give their food funny names) */
    { "cubus", "cubi" }, /* in-/suc-cubus */
    { "culus", "culi" }, /* homunculus */
    { "djinni", "djinn" },
    { "erinys", "erinyes" },
    { "foot", "feet" },
    { "fungus", "fungi" },
    { "goose", "geese" },
    { "knife", "knives" },
    { "labrum", "labra" }, /* candelabrum */
    { "louse", "lice" },
    { "mouse", "mice" },
    { "mumak", "mumakil" },
    { "nemesis", "nemeses" },
    { "ovum", "ova" },
    { "ox", "oxen" },
    { "passerby", "passersby" },
    { "rtex", "rtices" }, /* vortex */
    { "serum", "sera" },
    { "staff", "staves" },
    { "tooth", "teeth" },
    { 0, 0 }
};

static const char *const as_is[] = {
    /* makesingular() leaves these plural due to how they're used */
    "boots",   "shoes",     "gloves",    "lenses",   "scales",
    "eyes",    "gauntlets", "iron bars",
    /* both singular and plural are spelled the same */
    "bison",   "deer",      "elk",       "fish",      "fowl",
    "tuna",    "yaki",      "-hai",      "krill",     "manes",
    "moose",   "ninja",     "sheep",     "ronin",     "roshi",
    "shito",   "tengu",     "ki-rin",    "Nazgul",    "gunyoki",
    "piranha", "samurai",   "shuriken", 0,
    /* Note:  "fish" and "piranha" are collective plurals, suitable
       for "wiped out all <foo>".  For "3 <foo>", they should be
       "fishes" and "piranhas" instead.  We settle for collective
       variant instead of attempting to support both. */
};

/* singularize/pluralize decisions common to both makesingular & makeplural */
STATIC_OVL boolean
singplur_lookup(basestr, endstring, to_plural, alt_as_is)
char *basestr, *endstring;    /* base string, pointer to eos(string) */
boolean to_plural;            /* true => makeplural, false => makesingular */
const char *const *alt_as_is; /* another set like as_is[] */
{
    const struct sing_plur *sp;
    const char *same, *other, *const *as;
    int al;
    int baselen = strlen(basestr);

    for (as = as_is; *as; ++as) {
        al = (int) strlen(*as);
        if (!BSTRCMPI(basestr, endstring - al, *as))
            return TRUE;
    }
    if (alt_as_is) {
        for (as = alt_as_is; *as; ++as) {
            al = (int) strlen(*as);
            if (!BSTRCMPI(basestr, endstring - al, *as))
                return TRUE;
        }
    }

   /* Leave "craft" as a suffix as-is (aircraft, hovercraft);
      "craft" itself is (arguably) not included in our likely context */
   if ((baselen > 5) && (!BSTRCMPI(basestr, endstring - 5, "craft")))
       return TRUE;
   /* avoid false hit on one_off[].plur == "lice" or .sing == "goose";
       if more of these turn up, one_off[] entries will need to flagged
       as to which are whole words and which are matchable as suffices
       then matching in the loop below will end up becoming more complex */
    if (!strcmpi(basestr, "slice")
        || !strcmpi(basestr, "mongoose")) {
        if (to_plural)
            Strcasecpy(endstring, "s");
        return TRUE;
    }
    /* skip "ox" -> "oxen" entry when pluralizing "<something>ox"
       unless it is muskox */
    if (to_plural && baselen > 2 && !strcmpi(endstring - 2, "ox")
        && !(baselen > 5 && !strcmpi(endstring - 6, "muskox"))) {
        /* "fox" -> "foxes" */
        Strcasecpy(endstring, "es");
        return TRUE;
    }
    if (to_plural) {
        if (baselen > 2 && !strcmpi(endstring - 3, "man")
            && badman(basestr, to_plural)) {
            Strcasecpy(endstring, "s");
            return TRUE;
        }
    } else {
        if (baselen > 2 && !strcmpi(endstring - 3, "men")
            && badman(basestr, to_plural))
            return TRUE;
    }
    for (sp = one_off; sp->sing; sp++) {
        /* check whether endstring already matches */
        same = to_plural ? sp->plur : sp->sing;
        al = (int) strlen(same);
        if (!BSTRCMPI(basestr, endstring - al, same))
            return TRUE; /* use as-is */
        /* check whether it matches the inverse; if so, transform it */
        other = to_plural ? sp->sing : sp->plur;
        al = (int) strlen(other);
        if (!BSTRCMPI(basestr, endstring - al, other)) {
            Strcasecpy(endstring - al, same);
            return TRUE; /* one_off[] transformation */
        }
    }
    return FALSE;
}

/* searches for common compounds, ex. lump of royal jelly */
STATIC_OVL char *
singplur_compound(str)
char *str;
{
    /* if new entries are added, be sure to keep compound_start[] in sync */
    static const char *const compounds[] =
        {
          " of ",     " labeled ", " called ",
          " named ",  " above", /* lurkers above */
          " versus ", " from ",    " in ",
          " on ",     " a la ",    " with", /* " with "? */
          " de ",     " d'",       " du ",
          "-in-",     "-at-",      0
        }, /* list of first characters for all compounds[] entries */
        compound_start[] = " -";

    const char *const *cmpd;
    char *p;

    for (p = str; *p; ++p) {
        /* substring starting at p can only match if *p is found
           within compound_start[] */
        if (!index(compound_start, *p))
            continue;

        /* check current substring against all words in the compound[] list */
        for (cmpd = compounds; *cmpd; ++cmpd)
            if (!strncmpi(p, *cmpd, (int) strlen(*cmpd)))
                return p;
    }
    /* wasn't recognized as a compound phrase */
    return 0;
}
#endif

/* Plural routine; once upon a time it may have been chiefly used for
 * user-defined fruits, but it is now used extensively throughout the
 * program.
 *
 * For fruit, we have to try to account for everything reasonable the
 * player has; something unreasonable can still break the code.
 * However, it's still a lot more accurate than "just add an 's' at the
 * end", which Rogue uses...
 *
 * Also used for plural monster names ("Wiped out all homunculi." or the
 * vanquished monsters list) and body parts.  A lot of unique monsters have
 * names which get mangled by makeplural and/or makesingular.  They're not
 * genocidable, and vanquished-mon handling does its own special casing
 * (for uniques who've been revived and re-killed), so we don't bother
 * trying to get those right here.
 *
 * Also misused by muse.c to convert 1st person present verbs to 2nd person.
 * 3.6.0: made case-insensitive.
 */
char *makeplural(oldstr) const char *oldstr;
{
#if 0 /*KR*/
    register char *spot;
    char lo_c, *str = nextobuf();
    const char *excess = (char *) 0;
    int len;

    if (oldstr)
        while (*oldstr == ' ')
            oldstr++;
    if (!oldstr || !*oldstr) {
        impossible("plural of null?");
        Strcpy(str, "s");
        return str;
    }
    Strcpy(str, oldstr);

    /*
     * Skip changing "pair of" to "pairs of".  According to Webster, usual
     * English usage is use pairs for humans, e.g. 3 pairs of dancers,
     * and pair for objects and non-humans, e.g. 3 pair of boots.  We don't
     * refer to pairs of humans in this game so just skip to the bottom.
     */
    if (!strncmpi(str, "pair of ", 8))
        goto bottom;

    /* look for "foo of bar" so that we can focus on "foo" */
    if ((spot = singplur_compound(str)) != 0) {
        excess = oldstr + (int) (spot - str);
        *spot = '\0';
    } else
        spot = eos(str);

    spot--;
    while (spot > str && *spot == ' ')
        spot--; /* Strip blanks from end */
    *(spot + 1) = '\0';
    /* Now spot is the last character of the string */

    len = strlen(str);

    /* Single letters */
    if (len == 1 || !letter(*spot)) {
        Strcpy(spot + 1, "'s");
        goto bottom;
    }

    /* dispense with some words which don't need pluralization */
    {
        static const char *const already_plural[] = {
            "ae",  /* algae, larvae, &c */
            "matzot", 0,
        };

        /* spot+1: synch up with makesingular's usage */
        if (singplur_lookup(str, spot + 1, TRUE, already_plural))
            goto bottom;

        /* more of same, but not suitable for blanket loop checking */
        if ((len == 2 && !strcmpi(str, "ya"))
            || (len >= 3 && !strcmpi(spot - 2, " ya")))
            goto bottom;
    }

    /* man/men ("Wiped out all cavemen.") */
    if (len >= 3 && !strcmpi(spot - 2, "man")
        /* exclude shamans and humans etc */
        && !badman(str, TRUE)) {
        Strcasecpy(spot - 1, "en");
        goto bottom;
    }
    if (lowc(*spot) == 'f') { /* (staff handled via one_off[]) */
        lo_c = lowc(*(spot - 1));
        if (len >= 3 && !strcmpi(spot - 2, "erf")) {
            /* avoid "nerf" -> "nerves", "serf" -> "serves" */
            ; /* fall through to default (append 's') */
        } else if (index("lr", lo_c) || index(vowels, lo_c)) {
            /* [aeioulr]f to [aeioulr]ves */
            Strcasecpy(spot, "ves");
            goto bottom;
        }
    }
    /* ium/ia (mycelia, baluchitheria) */
    if (len >= 3 && !strcmpi(spot - 2, "ium")) {
        Strcasecpy(spot - 2, "ia");
        goto bottom;
    }
    /* algae, larvae, hyphae (another fungus part) */
    if ((len >= 4 && !strcmpi(spot - 3, "alga"))
        || (len >= 5
            && (!strcmpi(spot - 4, "hypha") || !strcmpi(spot - 4, "larva")))
        || (len >= 6 && !strcmpi(spot - 5, "amoeba"))
        || (len >= 8 && (!strcmpi(spot - 7, "vertebra")))) {
        /* a to ae */
        Strcasecpy(spot + 1, "e");
        goto bottom;
    }
    /* fungus/fungi, homunculus/homunculi, but buses, lotuses, wumpuses */
    if (len > 3 && !strcmpi(spot - 1, "us")
        && !((len >= 5 && !strcmpi(spot - 4, "lotus"))
             || (len >= 6 && !strcmpi(spot - 5, "wumpus")))) {
        Strcasecpy(spot - 1, "i");
        goto bottom;
    }
    /* sis/ses (nemesis) */
    if (len >= 3 && !strcmpi(spot - 2, "sis")) {
        Strcasecpy(spot - 1, "es");
        goto bottom;
    }
    /* matzoh/matzot, possible food name */
    if (len >= 6
        && (!strcmpi(spot - 5, "matzoh") || !strcmpi(spot - 5, "matzah"))) {
        Strcasecpy(spot - 1, "ot"); /* oh/ah -> ot */
        goto bottom;
    }
    if (len >= 5
        && (!strcmpi(spot - 4, "matzo") || !strcmpi(spot - 4, "matza"))) {
        Strcasecpy(spot, "ot"); /* o/a -> ot */
        goto bottom;
    }

    /* note: -eau/-eaux (gateau, bordeau...) */
    /* note: ox/oxen, VAX/VAXen, goose/geese */

    lo_c = lowc(*spot);

    /* codex/spadix/neocortex and the like */
    if (len >= 5
        && (!strcmpi(spot - 2, "dex")
            ||!strcmpi(spot - 2, "dix")
            ||!strcmpi(spot - 2, "tex"))
           /* indices would have been ok too, but stick with indexes */
        && (strcmpi(spot - 4,"index") != 0)) {
        Strcasecpy(spot - 1, "ices"); /* ex|ix -> ices */
        goto bottom;
    }
    /* Ends in z, x, s, ch, sh; add an "es" */
    if (index("zxs", lo_c)
        || (len >= 2 && lo_c == 'h' && index("cs", lowc(*(spot - 1)))
            /* 21st century k-sound */
            && !(len >= 4 &&
                 ((lowc(*(spot - 2)) == 'e'
                    && index("mt", lowc(*(spot - 3)))) ||
                  (lowc(*(spot - 2)) == 'o'
                    && index("lp", lowc(*(spot - 3)))))))
        /* Kludge to get "tomatoes" and "potatoes" right */
        || (len >= 4 && !strcmpi(spot - 2, "ato"))
        || (len >= 5 && !strcmpi(spot - 4, "dingo"))) {
        Strcasecpy(spot + 1, "es"); /* append es */
        goto bottom;
    }
    /* Ends in y preceded by consonant (note: also "qu") change to "ies" */
    if (lo_c == 'y' && !index(vowels, lowc(*(spot - 1)))) {
        Strcasecpy(spot, "ies"); /* y -> ies */
        goto bottom;
    }
    /* Default: append an 's' */
    Strcasecpy(spot + 1, "s");

 bottom:
    if (excess)
        Strcat(str, excess);
    return str;
#else
    /* 한국어는 복수형을 처리하지 않으므로 원본 문자열을 그대로 반환합니다. */
    char *str = nextobuf();
    if (oldstr)
        Strcpy(str, oldstr);
    else
        str[0] = '\0';
    return str;
#endif
}

/*
 * Singularize a string the user typed in; this helps reduce the complexity
 * of readobjnam, and is also used in pager.c to singularize the string
 * for which help is sought.
 *
 * "Manes" is ambiguous: monster type (keep s), or horse body part (drop s)?
 * Its inclusion in as_is[]/special_subj[] makes it get treated as the former.
 *
 * A lot of unique monsters have names ending in s; plural, or singular
 * from plural, doesn't make much sense for them so we don't bother trying.
 * 3.6.0: made case-insensitive.
 */
char *
makesingular(oldstr)
const char *oldstr;
{
#if 0 /*KR 한국어는 단수 복수가 동일한 형태 */
    register char *p, *bp;
    const char *excess = 0;
    char *str = nextobuf();

    if (oldstr)
        while (*oldstr == ' ')
            oldstr++;
    if (!oldstr || !*oldstr) {
        impossible("singular of null?");
        str[0] = '\0';
        return str;
    }

    bp = strcpy(str, oldstr);

    /* check for "foo of bar" so that we can focus on "foo" */
    if ((p = singplur_compound(bp)) != 0) {
        excess = oldstr + (int) (p - bp);
        *p = '\0';
    } else
        p = eos(bp);

    /* dispense with some words which don't need singularization */
    if (singplur_lookup(bp, p, FALSE, special_subjs))
        goto bottom;

    /* remove -s or -es (boxes) or -ies (rubies) */
    if (p >= bp + 1 && lowc(p[-1]) == 's') {
        if (p >= bp + 2 && lowc(p[-2]) == 'e') {
            if (p >= bp + 3 && lowc(p[-3]) == 'i') { /* "ies" */
                if (!BSTRCMPI(bp, p - 7, "cookies")
                    || (!BSTRCMPI(bp, p - 4, "pies")
                        /* avoid false match for "harpies" */
                        && (p - 4 == bp || p[-5] == ' '))
                    /* alternate djinni/djinn spelling; not really needed */
                    || (!BSTRCMPI(bp, p - 6, "genies")
                        /* avoid false match for "progenies" */
                        && (p - 6 == bp || p[-7] == ' '))
                    || !BSTRCMPI(bp, p - 5, "mbies") /* zombie */
                    || !BSTRCMPI(bp, p - 5, "yries")) /* valkyrie */
                    goto mins;
                Strcasecpy(p - 3, "y"); /* ies -> y */
                goto bottom;
            }
            /* wolves, but f to ves isn't fully reversible */
            if (p - 4 >= bp && (index("lr", lowc(*(p - 4)))
                                || index(vowels, lowc(*(p - 4))))
                && !BSTRCMPI(bp, p - 3, "ves")) {
                if (!BSTRCMPI(bp, p - 6, "cloves")
                    || !BSTRCMPI(bp, p - 6, "nerves"))
                    goto mins;
                Strcasecpy(p - 3, "f"); /* ves -> f */
                goto bottom;
            }
            /* note: nurses, axes but boxes, wumpuses */
            if (!BSTRCMPI(bp, p - 4, "eses")
                || !BSTRCMPI(bp, p - 4, "oxes") /* boxes, foxes */
                || !BSTRCMPI(bp, p - 4, "nxes") /* lynxes */
                || !BSTRCMPI(bp, p - 4, "ches")
                || !BSTRCMPI(bp, p - 4, "uses") /* lotuses */
                || !BSTRCMPI(bp, p - 4, "sses") /* priestesses */
                || !BSTRCMPI(bp, p - 5, "atoes") /* tomatoes */
                || !BSTRCMPI(bp, p - 7, "dingoes")
                || !BSTRCMPI(bp, p - 7, "Aleaxes")) {
                *(p - 2) = '\0'; /* drop es */
                goto bottom;
            } /* else fall through to mins */

            /* ends in 's' but not 'es' */
        } else if (!BSTRCMPI(bp, p - 2, "us")) { /* lotus, fungus... */
            if (BSTRCMPI(bp, p - 6, "tengus") /* but not these... */
                && BSTRCMPI(bp, p - 7, "hezrous"))
                goto bottom;
        } else if (!BSTRCMPI(bp, p - 2, "ss")
                   || !BSTRCMPI(bp, p - 5, " lens")
                   || (p - 4 == bp && !strcmpi(p - 4, "lens"))) {
            goto bottom;
        }
 mins:
        *(p - 1) = '\0'; /* drop s */

    } else { /* input doesn't end in 's' */

        if (!BSTRCMPI(bp, p - 3, "men")
            && !badman(bp, FALSE)) {
            Strcasecpy(p - 2, "an");
            goto bottom;
        }
        /* matzot -> matzo, algae -> alga */
        if (!BSTRCMPI(bp, p - 6, "matzot") || !BSTRCMPI(bp, p - 2, "ae")) {
            *(p - 1) = '\0'; /* drop t/e */
            goto bottom;
        }
        /* balactheria -> balactherium */
        if (p - 4 >= bp && !strcmpi(p - 2, "ia")
            && index("lr", lowc(*(p - 3))) && lowc(*(p - 4)) == 'e') {
            Strcasecpy(p - 1, "um"); /* a -> um */
        }

        /* here we cannot find the plural suffix */
    }

 bottom:
    /* if we stripped off a suffix (" of bar" from "foo of bar"),
       put it back now [strcat() isn't actually 100% safe here...] */
    if (excess)
        Strcat(bp, excess);

    return bp;
#else /*KR 새로운 버퍼는 필요 */
    char *str = nextobuf();
    Strcpy(str, oldstr);
    return str;
#endif
}

#if 0 /*KR*/
STATIC_OVL boolean
badman(basestr, to_plural)
const char *basestr;
boolean to_plural;            /* true => makeplural, false => makesingular */
{
    /* these are all the prefixes for *man that don't have a *men plural */
    static const char *no_men[] = {
        "albu", "antihu", "anti", "ata", "auto", "bildungsro", "cai", "cay",
        "ceru", "corner", "decu", "des", "dura", "fir", "hanu", "het",
        "infrahu", "inhu", "nonhu", "otto", "out", "prehu", "protohu",
        "subhu", "superhu", "talis", "unhu", "sha",
        "hu", "un", "le", "re", "so", "to", "at", "a",
    };
    /* these are all the prefixes for *men that don't have a *man singular */
    static const char *no_man[] = {
        "abdo", "acu", "agno", "ceru", "cogno", "cycla", "fleh", "grava",
        "hegu", "preno", "sonar", "speci", "dai", "exa", "fla", "sta", "teg",
        "tegu", "vela", "da", "hy", "lu", "no", "nu", "ra", "ru", "se", "vi",
        "ya", "o", "a",
    };
    int i, al;
    const char *endstr, *spot;

    if (!basestr || strlen(basestr) < 4)
        return FALSE;

    endstr = eos((char *) basestr);

    if (to_plural) {
        for (i = 0; i < SIZE(no_men); i++) {
            al = (int) strlen(no_men[i]);
            spot = endstr - (al + 3);
            if (!BSTRNCMPI(basestr, spot, no_men[i], al)
                && (spot == basestr || *(spot - 1) == ' '))
                return TRUE;
        }
    } else {
        for (i = 0; i < SIZE(no_man); i++) {
            al = (int) strlen(no_man[i]);
            spot = endstr - (al + 3);
            if (!BSTRNCMPI(basestr, spot, no_man[i], al)
                && (spot == basestr || *(spot - 1) == ' '))
                return TRUE;
        }
    }
    return FALSE;
}
#endif

/* compare user string against object name string using fuzzy matching */
STATIC_OVL boolean
wishymatch(u_str, o_str, retry_inverted)
const char *u_str;      /* from user, so might be variant spelling */
const char *o_str;      /* from objects[], so is in canonical form */
boolean retry_inverted; /* optional extra "of" handling */
{
    static NEARDATA const char detect_SP[] = "detect ",
                               SP_detection[] = " detection";
    char *p, buf[BUFSZ];

    /* ignore spaces & hyphens and upper/lower case when comparing */
    if (fuzzymatch(u_str, o_str, " -", TRUE))
        return TRUE;

    if (retry_inverted) {
        const char *u_of, *o_of;

        /* when just one of the strings is in the form "foo of bar",
           convert it into "bar foo" and perform another comparison */
        u_of = strstri(u_str, " of ");
        o_of = strstri(o_str, " of ");
        if (u_of && !o_of) {
            Strcpy(buf, u_of + 4);
            p = eos(strcat(buf, " "));
            while (u_str < u_of)
                *p++ = *u_str++;
            *p = '\0';
            return fuzzymatch(buf, o_str, " -", TRUE);
        } else if (o_of && !u_of) {
            Strcpy(buf, o_of + 4);
            p = eos(strcat(buf, " "));
            while (o_str < o_of)
                *p++ = *o_str++;
            *p = '\0';
            return fuzzymatch(u_str, buf, " -", TRUE);
        }
    }

    /* [note: if something like "elven speed boots" ever gets added, these
       special cases should be changed to call wishymatch() recursively in
       order to get the "of" inversion handling] */
    if (!strncmp(o_str, "dwarvish ", 9)) {
        if (!strncmpi(u_str, "dwarven ", 8))
            return fuzzymatch(u_str + 8, o_str + 9, " -", TRUE);
    } else if (!strncmp(o_str, "elven ", 6)) {
        if (!strncmpi(u_str, "elvish ", 7))
            return fuzzymatch(u_str + 7, o_str + 6, " -", TRUE);
        else if (!strncmpi(u_str, "elfin ", 6))
            return fuzzymatch(u_str + 6, o_str + 6, " -", TRUE);
    } else if (!strncmp(o_str, detect_SP, sizeof detect_SP - 1)) {
        /* check for "detect <foo>" vs "<foo> detection" */
        if ((p = strstri(u_str, SP_detection)) != 0
            && !*(p + sizeof SP_detection - 1)) {
            /* convert "<foo> detection" into "detect <foo>" */
            *p = '\0';
            Strcat(strcpy(buf, detect_SP), u_str);
            /* "detect monster" -> "detect monsters" */
            if (!strcmpi(u_str, "monster"))
                Strcat(buf, "s");
            *p = ' ';
            return fuzzymatch(buf, o_str, " -", TRUE);
        }
    } else if (strstri(o_str, SP_detection)) {
        /* and the inverse, "<foo> detection" vs "detect <foo>" */
        if (!strncmpi(u_str, detect_SP, sizeof detect_SP - 1)) {
            /* convert "detect <foo>s" into "<foo> detection" */
            p = makesingular(u_str + sizeof detect_SP - 1);
            Strcat(strcpy(buf, p), SP_detection);
            /* caller may be looping through objects[], so avoid
               churning through all the obufs */
            releaseobuf(p);
            return fuzzymatch(buf, o_str, " -", TRUE);
        }
    } else if (strstri(o_str, "ability")) {
        /* when presented with "foo of bar", makesingular() used to
           singularize both foo & bar, but now only does so for foo */
        /* catch "{potion(s),ring} of {gain,restore,sustain} abilities" */
        if ((p = strstri(u_str, "abilities")) != 0
            && !*(p + sizeof "abilities" - 1)) {
            (void) strncpy(buf, u_str, (unsigned) (p - u_str));
            Strcpy(buf + (p - u_str), "ability");
            return fuzzymatch(buf, o_str, " -", TRUE);
        }
    } else if (!strcmp(o_str, "aluminum")) {
        /* this special case doesn't really fit anywhere else... */
        /* (note that " wand" will have been stripped off by now) */
        if (!strcmpi(u_str, "aluminium"))
            return fuzzymatch(u_str + 9, o_str + 8, " -", TRUE);
    }

    return FALSE;
}

struct o_range {
    const char *name, oclass;
    int f_o_range, l_o_range;
};

#if 0 /*KR 특정 범주를 지정해서 소원을 빌 때. 한국어에선 일단 안함 */
/* wishable subranges of objects */
STATIC_OVL NEARDATA const struct o_range o_ranges[] = {
    { "bag", TOOL_CLASS, SACK, BAG_OF_TRICKS },
    { "lamp", TOOL_CLASS, OIL_LAMP, MAGIC_LAMP },
    { "candle", TOOL_CLASS, TALLOW_CANDLE, WAX_CANDLE },
    { "horn", TOOL_CLASS, TOOLED_HORN, HORN_OF_PLENTY },
    { "shield", ARMOR_CLASS, SMALL_SHIELD, SHIELD_OF_REFLECTION },
    { "hat", ARMOR_CLASS, FEDORA, DUNCE_CAP },
    { "helm", ARMOR_CLASS, ELVEN_LEATHER_HELM, HELM_OF_TELEPATHY },
    { "gloves", ARMOR_CLASS, LEATHER_GLOVES, GAUNTLETS_OF_DEXTERITY },
    { "gauntlets", ARMOR_CLASS, LEATHER_GLOVES, GAUNTLETS_OF_DEXTERITY },
    { "boots", ARMOR_CLASS, LOW_BOOTS, LEVITATION_BOOTS },
    { "shoes", ARMOR_CLASS, LOW_BOOTS, IRON_SHOES },
    { "cloak", ARMOR_CLASS, MUMMY_WRAPPING, CLOAK_OF_DISPLACEMENT },
    { "shirt", ARMOR_CLASS, HAWAIIAN_SHIRT, T_SHIRT },
    { "dragon scales", ARMOR_CLASS, GRAY_DRAGON_SCALES,
      YELLOW_DRAGON_SCALES },
    { "dragon scale mail", ARMOR_CLASS, GRAY_DRAGON_SCALE_MAIL,
      YELLOW_DRAGON_SCALE_MAIL },
    { "sword", WEAPON_CLASS, SHORT_SWORD, KATANA },
    { "venom", VENOM_CLASS, BLINDING_VENOM, ACID_VENOM },
    { "gray stone", GEM_CLASS, LUCKSTONE, FLINT },
    { "grey stone", GEM_CLASS, LUCKSTONE, FLINT },
};
#endif

#if 0 /*KR 미사용 */
/* alternate spellings; if the difference is only the presence or
   absence of spaces and/or hyphens (such as "pickaxe" vs "pick axe"
   vs "pick-axe") then there is no need for inclusion in this list;
   likewise for ``"of" inversions'' ("boots of speed" vs "speed boots") */
static const struct alt_spellings {
    const char *sp;
    int ob;
} spellings[] = {
    { "pickax", PICK_AXE },
    { "whip", BULLWHIP },
    { "saber", SILVER_SABER },
    { "silver sabre", SILVER_SABER },
    { "smooth shield", SHIELD_OF_REFLECTION },
    { "grey dragon scale mail", GRAY_DRAGON_SCALE_MAIL },
    { "grey dragon scales", GRAY_DRAGON_SCALES },
    { "iron ball", HEAVY_IRON_BALL },
    { "lantern", BRASS_LANTERN },
    { "mattock", DWARVISH_MATTOCK },
    { "amulet of poison resistance", AMULET_VERSUS_POISON },
    { "potion of sleep", POT_SLEEPING },
    { "stone", ROCK },
    { "camera", EXPENSIVE_CAMERA },
    { "tee shirt", T_SHIRT },
    { "can", TIN },
    { "can opener", TIN_OPENER },
    { "kelp", KELP_FROND },
    { "eucalyptus", EUCALYPTUS_LEAF },
    { "royal jelly", LUMP_OF_ROYAL_JELLY },
    { "lembas", LEMBAS_WAFER },
    { "cookie", FORTUNE_COOKIE },
    { "pie", CREAM_PIE },
    { "marker", MAGIC_MARKER },
    { "hook", GRAPPLING_HOOK },
    { "grappling iron", GRAPPLING_HOOK },
    { "grapnel", GRAPPLING_HOOK },
    { "grapple", GRAPPLING_HOOK },
    { "protection from shape shifters", RIN_PROTECTION_FROM_SHAPE_CHAN },
    /* if we ever add other sizes, move this to o_ranges[] with "bag" */
    { "box", LARGE_BOX },
    /* normally we wouldn't have to worry about unnecessary <space>, but
       " stone" will get stripped off, preventing a wishymatch; that actually
       lets "flint stone" be a match, so we also accept bogus "flintstone" */
    { "luck stone", LUCKSTONE },
    { "load stone", LOADSTONE },
    { "touch stone", TOUCHSTONE },
    { "flintstone", FLINT },
    { (const char *) 0, 0 },
};

STATIC_OVL short
rnd_otyp_by_wpnskill(skill)
schar skill;
{
    int i, n = 0;
    short otyp = STRANGE_OBJECT;

    for (i = bases[WEAPON_CLASS];
         i < NUM_OBJECTS && objects[i].oc_class == WEAPON_CLASS; i++)
        if (objects[i].oc_skill == skill) {
            n++;
            otyp = i;
        }
    if (n > 0) {
        n = rn2(n);
        for (i = bases[WEAPON_CLASS];
             i < NUM_OBJECTS && objects[i].oc_class == WEAPON_CLASS; i++)
            if (objects[i].oc_skill == skill)
                if (--n < 0)
                    return i;
    }
    return otyp;
}
#endif

STATIC_OVL short
rnd_otyp_by_namedesc(name, oclass, xtra_prob)
const char *name;
char oclass;
int xtra_prob; /* to force 0% random generation items to also be considered */
{
    int i, n = 0;
    short validobjs[NUM_OBJECTS];
    register const char *zn;
    int prob, maxprob = 0;

    if (!name || !*name)
        return STRANGE_OBJECT;

    memset((genericptr_t) validobjs, 0, sizeof validobjs);

    /* FIXME:
     * When this spans classes (the !oclass case), the item
     * probabilities are not very useful because they don't take
     * the class generation probability into account.  [If 10%
     * of spellbooks were blank and 1% of scrolls were blank,
     * "blank" would have 10/11 chance to yield a book even though
     * scrolls are supposed to be much more common than books.]
     */


for (i = oclass ? bases[(int) oclass] : STRANGE_OBJECT + 1;
         i < NUM_OBJECTS && (!oclass || objects[i].oc_class == oclass); ++i) {
#if 0 /*KR: 원본*/
        /* don't match extra descriptions (w/o real name) */
        if ((zn = OBJ_NAME(objects[i])) == 0)
            continue;
        if (wishymatch(name, zn, TRUE)
            || ((zn = OBJ_DESCR(objects[i])) != 0
                && wishymatch(name, zn, FALSE))
            || ((zn = objects[i].oc_uname) != 0
                && wishymatch(name, zn, FALSE))) {
            validobjs[n++] = (short) i;
            maxprob += (objects[i].oc_prob + xtra_prob);
        }
#else /*KR: (영어 검색 지원 및 가짜 옌더의 부적 예외 처리) */
        const char *raw_zn;
        /* don't match extra descriptions (w/o real name) */
        if ((zn = OBJ_NAME(objects[i])) == 0)
            continue;

        /* "옌더의 부적"을 소원으로 빌었을 때 여기서 가짜 부적이 검색되지
         * 않도록 함. (비-위자드 모드일 때 진짜를 가짜로 바꾸는 처리는 나중에
         * 수행됨) */
        if (i == FAKE_AMULET_OF_YENDOR)
            continue;

        raw_zn = RAW_OBJ_NAME(objects[i]); /* 영어 원본 이름 추출 */

        /* 한글(zn)과 영어(raw_zn) 둘 다 검색을 허용! */
        if (wishymatch(name, zn, TRUE)
            || (raw_zn && wishymatch(name, raw_zn, TRUE))
            || ((zn = OBJ_DESCR(objects[i])) != 0
                && wishymatch(name, zn, FALSE))
            || ((raw_zn = RAW_OBJ_DESCR(objects[i])) != 0
                && wishymatch(name, raw_zn, FALSE))
            || ((zn = objects[i].oc_uname) != 0
                && wishymatch(name, zn, FALSE))) {
            validobjs[n++] = (short) i;
            maxprob += (objects[i].oc_prob + xtra_prob);
        }
#endif
    }


    if (n > 0 && maxprob) {
        prob = rn2(maxprob);
        for (i = 0; i < n - 1; i++)
            if ((prob -= (objects[validobjs[i]].oc_prob + xtra_prob)) < 0)
                break;
        return validobjs[i];
    }
    return STRANGE_OBJECT;
}

int
shiny_obj(oclass)
char oclass;
{
    return (int) rnd_otyp_by_namedesc("shiny", oclass, 0);
}

/*
 * Return something wished for.  Specifying a null pointer for
 * the user request string results in a random object.  Otherwise,
 * if asking explicitly for "nothing" (or "nil") return no_wish;
 * if not an object return &zeroobj; if an error (no matching object),
 * return null.
 */
struct obj *
readobjnam(bp, no_wish)
register char *bp;
struct obj *no_wish;
{
    register char *p;
    register int i;
    register struct obj *otmp;
    int cnt, spe, spesgn, typ, very, rechrg;
    int blessed, uncursed, iscursed, ispoisoned, isgreased;
    int eroded, eroded2, erodeproof, locked, unlocked, broken;
    int halfeaten, mntmp, contents;
    int islit, unlabeled, ishistoric, isdiluted, trapped;
#if 0 /*KR*/
    int tmp, tinv, tvariety;
#else
    int tvariety;
#endif
    int wetness, gsize = 0;
    struct fruit *f;
    int ftype = context.current_fruit;
#if 0 /*KR*/
    char fruitbuf[BUFSZ], globbuf[BUFSZ];
#else
    char fruitbuf[BUFSZ];
#endif
    /* Fruits may not mess up the ability to wish for real objects (since
     * you can leave a fruit in a bones file and it will be added to
     * another person's game), so they must be checked for last, after
     * stripping all the possible prefixes and seeing if there's a real
     * name in there.  So we have to save the full original name.  However,
     * it's still possible to do things like "uncursed burnt Alaska",
     * or worse yet, "2 burned 5 course meals", so we need to loop to
     * strip off the prefixes again, this time stripping only the ones
     * possible on food.
     * We could get even more detailed so as to allow food names with
     * prefixes that _are_ possible on food, so you could wish for
     * "2 3 alarm chilis".  Currently this isn't allowed; options.c
     * automatically sticks 'candied' in front of such names.
     */
    char oclass;
    char *un, *dn, *actualn, *origbp = bp;
    const char *name = 0;

    cnt = spe = spesgn = typ = 0;
    very = rechrg = blessed = uncursed = iscursed = ispoisoned =
        isgreased = eroded = eroded2 = erodeproof = halfeaten =
        islit = unlabeled = ishistoric = isdiluted = trapped =
        locked = unlocked = broken = 0;
    tvariety = RANDOM_TIN;
    mntmp = NON_PM;
#define UNDEFINED 0
#define EMPTY 1
#define SPINACH 2
    contents = UNDEFINED;
    oclass = 0;
    actualn = dn = un = 0;
    wetness = 0;

    if (!bp)
        goto any;
    /* first, remove extra whitespace they may have typed */
    (void) mungspaces(bp);
    /* allow wishing for "nothing" to preserve wishless conduct...
       [now requires "wand of nothing" if that's what was really wanted] */
#if 0 /*KR 영어 원문 유지 + 한국어 추가 */
    if (!strcmpi(bp, "nothing") || !strcmpi(bp, "nil")
        || !strcmpi(bp, "none"))
#else
    if (!strcmpi(bp, "nothing") || !strcmpi(bp, "nil") || !strcmpi(bp, "none")
        || !strcmp(bp, "없음") || !strcmp(bp, "아무것도 없음")
        || !strcmp(bp, "아무것도") || !strcmp(bp, "무"))
#endif
        return no_wish;
    /* save the [nearly] unmodified choice string */
    Strcpy(fruitbuf, bp);

    for (;;) {
        register int l;

        if (!bp || !*bp)
            goto any;
        if (!strncmpi(bp, "an ", l = 3) || !strncmpi(bp, "a ", l = 2)) {
            cnt = 1;
        } else if (!strncmpi(bp, "the ", l = 4)) {
            ; /* just increment `bp' by `l' below */
        } else if (!cnt && digit(*bp) && strcmp(bp, "0")) {
            cnt = atoi(bp);
            while (digit(*bp))
                bp++;
            while (*bp == ' ')
                bp++;
            l = 0;
#if 1 /*KR 한국어의 수량사 뒤에 붙는 단위+조사 건너뛰기 */            
            if (!STRNCMPEX(bp, "권의")    || !STRNCMPEX(bp, "자루의")
                || !STRNCMPEX(bp, "벌의") || !STRNCMPEX(bp, "개의")
                || !STRNCMPEX(bp, "장의") || !STRNCMPEX(bp, "마리의")
                || !STRNCMPEX(bp, "의"))
                ; /* 일치하면 포인터를 다음으로 넘김 */
            else
                l = 0;
#endif

        } else if (*bp == '+' || *bp == '-') {
            spesgn = (*bp++ == '+') ? 1 : -1;
            spe = atoi(bp);
            while (digit(*bp))
                bp++;
            while (*bp == ' ')
                bp++;
            l = 0;
#if 0 /*KR: 원본*/
        } else if (!strncmpi(bp, "blessed ", l = 8)
                   || !strncmpi(bp, "holy ", l = 5)) {
            blessed = 1;
        } else if (!strncmpi(bp, "moist ", l = 6)
                   || !strncmpi(bp, "wet ", l = 4)) {
            if (!strncmpi(bp, "wet ", 4))
                wetness = rn2(3) + 3;
            else
                wetness = rnd(2);
        } else if (!strncmpi(bp, "cursed ", l = 7)
                   || !strncmpi(bp, "unholy ", l = 7)) {
            iscursed = 1;
        } else if (!strncmpi(bp, "uncursed ", l = 9)) {
            uncursed = 1;
#else /*KR: KRNethack 맞춤 번역 (영어+한글 동시 지원) */
        } else if (!strncmpi(bp, "blessed ", l = 8)
                   || !strncmpi(bp, "holy ", l = 5)
                   || !STRNCMPEX(bp, "축복받은 ")
                   || !STRNCMPEX(bp, "성스러운 ")) {
            blessed = 1;
        } else if (!strncmpi(bp, "moist ", l = 6)
                   || !strncmpi(bp, "wet ", l = 4)
                   || !STRNCMPEX(bp, "축축한 ") || !STRNCMPEX(bp, "젖은 ")) {
            if (!strncmpi(bp, "wet ", 4) || !STRNCMP2(bp, "젖은 "))
                wetness = rn2(3) + 3;
            else
                wetness = rnd(2);
        } else if (!strncmpi(bp, "cursed ", l = 7)
                   || !strncmpi(bp, "unholy ", l = 7)
                   || !STRNCMPEX(bp, "저주받은 ")
                   || !STRNCMPEX(bp, "불경한 ")) {
            iscursed = 1;
        } else if (!strncmpi(bp, "uncursed ", l = 9)
                   || !STRNCMPEX(bp, "저주받지 않은 ")) {
            uncursed = 1;
#endif
#if 0 /*KR: 원본*/
        } else if (!strncmpi(bp, "rustproof ", l = 10)
                   || !strncmpi(bp, "erodeproof ", l = 11)
                   || !strncmpi(bp, "corrodeproof ", l = 13)
                   || !strncmpi(bp, "fixed ", l = 6)
                   || !strncmpi(bp, "fireproof ", l = 10)
                   || !strncmpi(bp, "rotproof ", l = 9)) {
#else /*KR: KRNethack 맞춤 번역 (영어+한글 동시 지원) */
        } else if (!strncmpi(bp, "rustproof ", l = 10)
                   || !strncmpi(bp, "erodeproof ", l = 11)
                   || !strncmpi(bp, "corrodeproof ", l = 13)
                   || !strncmpi(bp, "fixed ", l = 6)
                   || !strncmpi(bp, "fireproof ", l = 10)
                   || !strncmpi(bp, "rotproof ", l = 9)
                   || !STRNCMPEX(bp, "방녹의 ")
                   || !STRNCMPEX(bp, "방부식의 ")
                   || !STRNCMPEX(bp, "방염의 ")
                   || !STRNCMPEX(bp, "방부패의 ")) {
#endif
            erodeproof = 1;

#if 0 /*KR: 원본*/
        } else if (!strncmpi(bp, "lit ", l = 4)
                   || !strncmpi(bp, "burning ", l = 8)) {
#else /*KR: KRNethack 맞춤 번역 */
        } else if (!strncmpi(bp, "lit ", l = 4)
                   || !strncmpi(bp, "burning ", l = 8)
                   || !STRNCMPEX(bp, "빛나는 ") || !STRNCMPEX(bp, "타오르는 ")
                   || !STRNCMPEX(bp, "켜진 ")) {
#endif
            islit = 1;

#if 0 /*KR: 원본*/
        } else if (!strncmpi(bp, "unlit ", l = 6)
                   || !strncmpi(bp, "extinguished ", l = 13)) {
#else /*KR: KRNethack 맞춤 번역 */
        } else if (!strncmpi(bp, "unlit ", l = 6)
                   || !strncmpi(bp, "extinguished ", l = 13)
                   || !STRNCMPEX(bp, "꺼진 ")
                   || !STRNCMPEX(bp, "빛나지 않는 ")) {
#endif
            islit = 0;
            /* "unlabeled" and "blank" are synonymous */

#if 0 /*KR: 원본*/
        } else if (!strncmpi(bp, "unlabeled ", l = 10)
                   || !strncmpi(bp, "unlabelled ", l = 11)
                   || !strncmpi(bp, "blank ", l = 6)) {
#else /*KR: KRNethack 맞춤 번역 */
        } else if (!strncmpi(bp, "unlabeled ", l = 10)
                   || !strncmpi(bp, "unlabelled ", l = 11)
                   || !strncmpi(bp, "blank ", l = 6)
                   || !STRNCMPEX(bp, "라벨 없는 ") || !STRNCMPEX(bp, "백지의 ")
                   || !STRNCMPEX(bp, "아무것도 안 적힌 ")) {
#endif
            unlabeled = 1;

#if 0 /*KR: 원본*/
        } else if (!strncmpi(bp, "poisoned ", l = 9)) {
#else /*KR: KRNethack 맞춤 번역 */
        } else if (!strncmpi(bp, "poisoned ", l = 9)
                   || !STRNCMPEX(bp, "독이 발라진 ")
                   || !STRNCMPEX(bp, "독이 묻은 ")) {
#endif
            ispoisoned = 1;
            /* "trapped" recognized but not honored outside wizard mode */
        } else if (!strncmpi(bp, "trapped ", l = 8)) {
            trapped = 0; /* undo any previous "untrapped" */
            if (wizard)
                trapped = 1;
        } else if (!strncmpi(bp, "untrapped ", l = 10)) {
            trapped = 2; /* not trapped */
        /* locked, unlocked, broken: box/chest lock states */
#if 0                    /*KR: 원본*/
        } else if (!strncmpi(bp, "locked ", l = 7)) {
#else                    /*KR: KRNethack 맞춤 번역 (영어+한글 동시 지원) */
        } else if (!strncmpi(bp, "locked ", l = 7) || !STRNCMPEX(bp, "잠긴 ")
                   || !STRNCMPEX(bp, "자물쇠가 채워진 ")) {
#endif
            locked = 1, unlocked = broken = 0;

#if 0 /*KR: 원본*/
        } else if (!strncmpi(bp, "unlocked ", l = 9)) {
#else /*KR: KRNethack 맞춤 번역 */
        } else if (!strncmpi(bp, "unlocked ", l = 9)
                   || !STRNCMPEX(bp, "잠기지 않은 ")
                   || !STRNCMPEX(bp, "자물쇠가 안 채워진 ")) {
#endif
            unlocked = 1, locked = broken = 0;

#if 0 /*KR: 원본*/
        } else if (!strncmpi(bp, "broken ", l = 7)) {
#else /*KR: KRNethack 맞춤 번역 */
        } else if (!strncmpi(bp, "broken ", l = 7)
                   || !STRNCMPEX(bp, "부서진 ")
                   || !STRNCMPEX(bp, "망가진 ")) {
#endif
            broken = 1, locked = unlocked = 0;

#if 0 /*KR: 원본*/
        } else if (!strncmpi(bp, "greased ", l = 8)) {
#else /*KR: KRNethack 맞춤 번역 */
        } else if (!strncmpi(bp, "greased ", l = 8)
                   || !STRNCMPEX(bp, "기름칠 된 ")
                   || !STRNCMPEX(bp, "기름이 발라진 ")) {
#endif
            isgreased = 1;

#if 0 /*KR: 원본*/
        } else if (!strncmpi(bp, "very ", l = 5)) {
#else /*KR: KRNethack 맞춤 번역 */
        } else if (!strncmpi(bp, "very ", l = 5) || !STRNCMPEX(bp, "매우 ")
                   || !STRNCMPEX(bp, "아주 ")) {
#endif
            /* very rusted very heavy iron ball */
            very = 1;

#if 0 /*KR: 원본*/
        } else if (!strncmpi(bp, "thoroughly ", l = 11)) {
#else /*KR: KRNethack 맞춤 번역 */
        } else if (!strncmpi(bp, "thoroughly ", l = 11)
                   || !STRNCMPEX(bp, "철저하게 ")
                   || !STRNCMPEX(bp, "완전히 ")) {
#endif
            very = 2;

#if 0 /*KR: 원본*/
        } else if (!strncmpi(bp, "rusty ", l = 6)
                   || !strncmpi(bp, "rusted ", l = 7)
                   || !strncmpi(bp, "burnt ", l = 6)
                   || !strncmpi(bp, "burned ", l = 7)) {
#else /*KR: KRNethack 맞춤 번역 */
        } else if (!strncmpi(bp, "rusty ", l = 6)
                   || !strncmpi(bp, "rusted ", l = 7)
                   || !strncmpi(bp, "burnt ", l = 6)
                   || !strncmpi(bp, "burned ", l = 7)
                   || !STRNCMPEX(bp, "녹슨 ") || !STRNCMPEX(bp, "불탄 ")
                   || !STRNCMPEX(bp, "탄 ")) {
#endif
            eroded = 1 + very;
            very = 0;

#if 0 /*KR: 원본*/
        } else if (!strncmpi(bp, "corroded ", l = 9)
                   || !strncmpi(bp, "rotted ", l = 7)) {
#else /*KR: KRNethack 맞춤 번역 */
        } else if (!strncmpi(bp, "corroded ", l = 9)
                   || !strncmpi(bp, "rotted ", l = 7)
                   || !STRNCMPEX(bp, "부식된 ") || !STRNCMPEX(bp, "썩은 ")) {
#endif
            eroded2 = 1 + very;
            very = 0;

#if 0 /*KR: 원본*/
        } else if (!strncmpi(bp, "partly eaten ", l = 13)
                   || !strncmpi(bp, "partially eaten ", l = 16)) {
#else /*KR: KRNethack 맞춤 번역 */
        } else if (!strncmpi(bp, "partly eaten ", l = 13)
                   || !strncmpi(bp, "partially eaten ", l = 16)
                   || !STRNCMPEX(bp, "먹다 만 ")
                   || !STRNCMPEX(bp, "일부 먹은 ")) {
#endif
            halfeaten = 1;

#if 0 /*KR: 원본*/
        } else if (!strncmpi(bp, "historic ", l = 9)) {
#else /*KR: KRNethack 맞춤 번역 */
        } else if (!strncmpi(bp, "historic ", l = 9)
                   || !STRNCMPEX(bp, "역사적인 ")
                   || !STRNCMPEX(bp, "유서 깊은 ")) {
#endif
            ishistoric = 1;

#if 0 /*KR: 원본*/
        } else if (!strncmpi(bp, "diluted ", l = 8)) {
#else /*KR: KRNethack 맞춤 번역 */
        } else if (!strncmpi(bp, "diluted ", l = 8)
                   || !STRNCMPEX(bp, "희석된 ")
                   || !STRNCMPEX(bp, "묽어진 ")) {
#endif
            isdiluted = 1;

#if 0 /*KR: 원본*/
        } else if (!strncmpi(bp, "empty ", l = 6)) {
#else /*KR: KRNethack 맞춤 번역 */
        } else if (!strncmpi(bp, "empty ", l = 6) || !STRNCMPEX(bp, "빈 ")
                   || !STRNCMPEX(bp, "비어있는 ")
                   || !STRNCMPEX(bp, "비어 있는 ")) {
#endif
            contents = EMPTY;

#if 0 /*KR: 원본*/
        } else if (!strncmpi(bp, "small ", l = 6)) { /* glob sizes */
#else /*KR: KRNethack 맞춤 번역 */
        } else if (!strncmpi(bp, "small ", l = 6)
                   || !STRNCMPEX(bp, "작은 ")) { /* glob sizes */
#endif
            /* "small" might be part of monster name (mimic, if wishing
               for its corpse) rather than prefix for glob size; when
               used for globs, it might be either "small glob of <foo>" or
               "small <foo> glob" and user might add 's' even though plural
               doesn't accomplish anything because globs don't stack */
            if (strncmpi(bp + l, "glob", 4) && !strstri(bp + l, " glob"))
                break;
            gsize = 1;

#if 0 /*KR: 원본*/
        } else if (!strncmpi(bp, "medium ", l = 7)) {
#else /*KR: KRNethack 맞춤 번역 */
        } else if (!strncmpi(bp, "medium ", l = 7) || !STRNCMPEX(bp, "중간 ")
                   || !STRNCMPEX(bp, "중간 크기 ")) {
#endif
            /* xname() doesn't display "medium" but without this
               there'd be no way to ask for the intermediate size
               ("glob" without size prefix yields smallest one) */
            gsize = 2;

#if 0 /*KR: 원본*/
        } else if (!strncmpi(bp, "large ", l = 6)) {
#else /*KR: KRNethack 맞춤 번역 */
        } else if (!strncmpi(bp, "large ", l = 6) || !STRNCMPEX(bp, "큰 ")
                   || !STRNCMPEX(bp, "커다란 ")) {
#endif
            /* "large" might be part of monster name (dog, cat, koboold,
               mimic) or object name (box, round shield) rather than
               prefix for glob size */
            if (strncmpi(bp + l, "glob", 4) && !strstri(bp + l, " glob"))
                break;
            /* "very large " had "very " peeled off on previous iteration */
            gsize = (very != 1) ? 3 : 4;
        } else
            break;
        bp += l;
    }
    if (!cnt)
        cnt = 1; /* will be changed to 2 if makesingular() changes string */
    if (strlen(bp) > 1 && (p = rindex(bp, '(')) != 0) {
        boolean keeptrailingchars = TRUE;

        p[(p > bp && p[-1] == ' ') ? -1 : 0] = '\0'; /*terminate bp */
        ++p; /* advance past '(' */
        if (!strncmpi(p, "lit)", 4)) {
            islit = 1;
            p += 4 - 1; /* point at ')' */
        } else {
            spe = atoi(p);
            while (digit(*p))
                p++;
            if (*p == ':') {
                p++;
                rechrg = spe;
                spe = atoi(p);
                while (digit(*p))
                    p++;
            }
            if (*p != ')') {
                spe = rechrg = 0;
                /* mis-matched parentheses; rest of string will be ignored
                 * [probably we should restore everything back to '('
                 * instead since it might be part of "named ..."]
                 */
                keeptrailingchars = FALSE;
            } else {
                spesgn = 1;
            }
        }
        if (keeptrailingchars) {
            char *pp = eos(bp);

            /* 'pp' points at 'pb's terminating '\0',
               'p' points at ')' and will be incremented past it */
            do {
                *pp++ = *++p;
            } while (*p);
        }
    }
    /*
     * otmp->spe is type schar, so we don't want spe to be any bigger or
     * smaller.  Also, spe should always be positive --some cheaters may
     * try to confuse atoi().
     */
    if (spe < 0) {
        spesgn = -1; /* cheaters get what they deserve */
        spe = abs(spe);
    }
    if (spe > SCHAR_LIM)
        spe = SCHAR_LIM;
    if (rechrg < 0 || rechrg > 7)
        rechrg = 7; /* recharge_limit */

    /* now we have the actual name, as delivered by xname, say
     *  green potions called whisky
     *  scrolls labeled "QWERTY"
     *  egg
     *  fortune cookies
     *  very heavy iron ball named hoei
     *  wand of wishing
     *  elven cloak
     */
    if ((p = strstri(bp, " named ")) != 0) {
        *p = 0;
        name = p + 7;
    }
    if ((p = strstri(bp, " called ")) != 0) {
        *p = 0;
        un = p + 8;
#if 0 /*KR 타입별로는 일단 하지 않음 */
        /* "helmet called telepathy" is not "helmet" (a specific type)
         * "shield called reflection" is not "shield" (a general type)
         */
        for (i = 0; i < SIZE(o_ranges); i++)
            if (!strcmpi(bp, o_ranges[i].name)) {
                oclass = o_ranges[i].oclass;
                goto srch;
            }
#endif
    }
    if ((p = strstri(bp, " labeled ")) != 0) {
        *p = 0;
        dn = p + 9;
    } else if ((p = strstri(bp, " labelled ")) != 0) {
        *p = 0;
        dn = p + 10;
    }
    if ((p = strstri(bp, " of spinach")) != 0) {
        *p = 0;
        contents = SPINACH;
    }

#if 0 /*KR 한국어에서는 처리하지 않는다 */
    /*
     * Skip over "pair of ", "pairs of", "set of" and "sets of".
     *
     * Accept "3 pair of boots" as well as "3 pairs of boots".  It is
     * valid English either way.  See makeplural() for more on pair/pairs.
     *
     * We should only double count if the object in question is not
     * referred to as a "pair of".  E.g. We should double if the player
     * types "pair of spears", but not if the player types "pair of
     * lenses".  Luckily (?) all objects that are referred to as pairs
     * -- boots, gloves, and lenses -- are also not mergable, so cnt is
     * ignored anyway.
     */
    if (!strncmpi(bp, "pair of ", 8)) {
        bp += 8;
        cnt *= 2;
    } else if (!strncmpi(bp, "pairs of ", 9)) {
        bp += 9;
        if (cnt > 1)
            cnt *= 2;
    } else if (!strncmpi(bp, "set of ", 7)) {
        bp += 7;
    } else if (!strncmpi(bp, "sets of ", 8)) {
        bp += 8;
    }
#endif

#if 0 /*KR ('~의 덩어리' 및 '~의' 처리) */
    /* intercept pudding globs here; they're a valid wish target,
     * but we need them to not get treated like a corpse.
     *
     * also don't let player wish for multiple globs.
     */
    i = (int) strlen(bp);
    p = (char *) 0;
    /* check for "glob", "<foo> glob", and "glob of <foo>" */
    if (!strcmpi(bp, "glob") || !BSTRCMPI(bp, bp + i - 5, " glob")
        || !strcmpi(bp, "globs") || !BSTRCMPI(bp, bp + i - 6, " globs")
        || (p = strstri(bp, "glob of ")) != 0
        || (p = strstri(bp, "globs of ")) != 0) {
        mntmp = name_to_mon(!p ? bp : (strstri(p, " of ") + 4));
        /* if we didn't recognize monster type, pick a valid one at random */
        if (mntmp == NON_PM)
            mntmp = rn1(PM_BLACK_PUDDING - PM_GRAY_OOZE, PM_GRAY_OOZE);
        /* construct canonical spelling in case name_to_mon() recognized a
           variant (grey ooze) or player used inverted syntax (<foo> glob);
           if player has given a valid monster type but not valid glob type,
           object name lookup won't find it and wish attempt will fail */
        Sprintf(globbuf, "glob of %s", mons[mntmp].mname);
        bp = globbuf;
        mntmp = NON_PM; /* not useful for "glob of <foo>" object lookup */
        cnt = 0; /* globs don't stack */
        oclass = FOOD_CLASS;
        actualn = bp, dn = 0;
        goto srch;
    } else {
        /*
         * Find corpse type using "of" (figurine of an orc, tin of orc meat)
         * Don't check if it's a wand or spellbook.
         * (avoid "wand/finger of death" confusion).
         */
        if (!strstri(bp, "wand ") && !strstri(bp, "spellbook ")
            && !strstri(bp, "finger ")) {
            if ((p = strstri(bp, "tin of ")) != 0) {
                if (!strcmpi(p + 7, "spinach")) {
                    contents = SPINACH;
                    mntmp = NON_PM;
                } else {
                    tmp = tin_variety_txt(p + 7, &tinv);
                    tvariety = tinv;
                    mntmp = name_to_mon(p + 7 + tmp);
                }
                typ = TIN;
                goto typfnd;
            } else if ((p = strstri(bp, " of ")) != 0
                       && (mntmp = name_to_mon(p + 4)) >= LOW_PM)
                *p = 0;
        }
    }
    /* Find corpse type w/o "of" (red dragon scale mail, yeti corpse) */
    if (strncmpi(bp, "samurai sword", 13)  /* not the "samurai" monster! */
        && strncmpi(bp, "wizard lock", 11) /* not the "wizard" monster! */
        && strncmpi(bp, "ninja-to", 8)     /* not the "ninja" rank */
        && strncmpi(bp, "master key", 10)  /* not the "master" rank */
        && strncmpi(bp, "magenta", 7)) {   /* not the "mage" rank */
        if (mntmp < LOW_PM && strlen(bp) > 2
            && (mntmp = name_to_mon(bp)) >= LOW_PM) {
            int mntmptoo, mntmplen; /* double check for rank title */
            char *obp = bp;

            mntmptoo = title_to_mon(bp, (int *) 0, &mntmplen);
            bp += (mntmp != mntmptoo) ? (int) strlen(mons[mntmp].mname)
                                      : mntmplen;
            if (*bp == ' ') {
                bp++;
            } else if (!strncmpi(bp, "s ", 2)) {
                bp += 2;
            } else if (!strncmpi(bp, "es ", 3)) {
                bp += 3;
            } else if (!*bp && !actualn && !dn && !un && !oclass) {
                /* no referent; they don't really mean a monster type */
                bp = obp;
                mntmp = NON_PM;
            }
        }
    }
#else /*KR:T "괴물명+의 덩어리"는 각각 고유 ID가 있으므로 별도 처리  */
    {
        int l = strlen(bp);
        int l2 = strlen("의 덩어리");
        if (l > 4 && strncmp(bp + l - l2, "의 덩어리", l2) == 0) {
            if ((mntmp = name_to_mon(bp)) >= PM_GRAY_OOZE
                && mntmp <= PM_BLACK_PUDDING) {
                mntmp = NON_PM; /* lie to ourselves */
                cnt = 0;        /* force only one */
            }
        } else {
            /* "(괴물명)의 (아이템)" 형태 대응 */
            if ((mntmp = name_to_mon(bp)) >= LOW_PM) {
                const char *mp = mons[mntmp].mname;
                bp = strstri(bp, mp) + strlen(mp);
                /* "의 " 부분을 건너뛰기 위한 처리 */
                if (!strncmp(bp, "의 ", 3)) {
                    bp += 3;
                } else if (!strncmp(bp, "의", 2)) {
                    bp += 2;
                }
            }
        }
    }
#endif

#if 0 /*KR 단수화는 하지 않는다 */
    /* first change to singular if necessary */
    if (*bp) {
        char *sng = makesingular(bp);

        if (strcmp(bp, sng)) {
            if (cnt == 1)
                cnt = 2;
            Strcpy(bp, sng);
        }
    }
#endif

#if 0 /*KR 스펠 발음·어법 공존 처리는 하지 않는다 */
    /* Alternate spellings (pick-ax, silver sabre, &c) */
    {
        const struct alt_spellings *as = spellings;

        while (as->sp) {
            if (fuzzymatch(bp, as->sp, " -", TRUE)) {
                typ = as->ob;
                goto typfnd;
            }
            as++;
        }
        /* can't use spellings list for this one due to shuffling */
        if (!strncmpi(bp, "grey spell", 10))
            *(bp + 2) = 'a';

        if ((p = strstri(bp, "armour")) != 0) {
            /* skip past "armo", then copy remainder beyond "u" */
            p += 4;
            while ((*p = *(p + 1)) != '\0')
                ++p; /* self terminating */
        }
    }
#endif

#if 0 /*KR*/
    /* dragon scales - assumes order of dragons */
    if (!strcmpi(bp, "scales") && mntmp >= PM_GRAY_DRAGON
        && mntmp <= PM_YELLOW_DRAGON) {
        typ = GRAY_DRAGON_SCALES + mntmp - PM_GRAY_DRAGON;
        mntmp = NON_PM; /* no monster */
        goto typfnd;
    }
#else
    /*JP: 「비늘 갑옷」을 먼저 처리해 둔다 */
    if (!strcmpi(bp, "비늘 갑옷") && mntmp >= PM_GRAY_DRAGON
        && mntmp <= PM_YELLOW_DRAGON) {
        typ = GRAY_DRAGON_SCALE_MAIL + mntmp - PM_GRAY_DRAGON;
        mntmp = NON_PM; /* no monster */
        goto typfnd;
    }

    if (!strcmpi(bp, "비늘") && mntmp >= PM_GRAY_DRAGON
        && mntmp <= PM_YELLOW_DRAGON) {
        typ = GRAY_DRAGON_SCALES + mntmp - PM_GRAY_DRAGON;
        mntmp = NON_PM; /* no monster */
        goto typfnd;
    }
#endif

    p = eos(bp);
#if 0 /*KR*/
    if (!BSTRCMPI(bp, p - 10, "holy water")) {
        typ = POT_WATER;
        if ((p - bp) >= 12 && *(p - 12) == 'u')
            iscursed = 1; /* unholy water */
        else
            blessed = 1;
        goto typfnd;
    }
#else /* 성수와 부정한 물을 별도로 판정 */
    if (!BSTRCMPI(bp, p - 4, "성수")) {
        typ = POT_WATER;
        blessed = 1;
        goto typfnd;
    }
    if (!BSTRCMPI(bp, p - 8, "부정한 물")) {
        typ = POT_WATER;
        iscursed = 1;
        goto typfnd;
    }
#endif
#if 0 /*KR*/
    if (unlabeled && !BSTRCMPI(bp, p - 6, "scroll")) {
#else
    if (unlabeled && !BSTRCMPI(bp, p - 4, "두루마리")) {
#endif
        typ = SCR_BLANK_PAPER;
        goto typfnd;
    }
#if 0 /*JP*/
    if (unlabeled && !BSTRCMPI(bp, p - 9, "spellbook")) {
#else
    if (unlabeled && !BSTRCMPI(bp, p - 6, "주문서")) {
#endif
        typ = SPE_BLANK_PAPER;
        goto typfnd;
    }
    /* specific food rather than color of gem/potion/spellbook[/scales] */
    if (!BSTRCMPI(bp, p - 6, "orange") && mntmp == NON_PM) {
        typ = ORANGE;
        goto typfnd;
    }
    /*
     * NOTE: Gold pieces are handled as objects nowadays, and therefore
     * this section should probably be reconsidered as well as the entire
     * gold/money concept.  Maybe we want to add other monetary units as
     * well in the future. (TH)
     */
#if 0 /*KR: 원본*/
    if (!BSTRCMPI(bp, p - 10, "gold piece")
        || !BSTRCMPI(bp, p - 7, "zorkmid")
        || !strcmpi(bp, "gold") || !strcmpi(bp, "money")
        || !strcmpi(bp, "coin") || *bp == GOLD_SYM) {
#else /*KR: (영어+한글 동시 지원 및 길이 자동계산) */
    if (!BSTRCMPI(bp, p - 10, "gold piece") || !BSTRCMPI(bp, p - 7, "zorkmid")
        || !strcmpi(bp, "gold") || !strcmpi(bp, "money")
        || !strcmpi(bp, "coin") || !BSTRCMPI(bp, p - strlen("금화"), "금화")
        || !BSTRCMPI(bp, p - strlen("조크미드"), "조크미드")
        || !strcmp(bp, "금") || !strcmp(bp, "돈") || !strcmp(bp, "동전")
        || *bp == GOLD_SYM) {
#endif
        if (cnt > 5000 && !wizard)
            cnt = 5000;
        else if (cnt < 1)
            cnt = 1;
        otmp = mksobj(GOLD_PIECE, FALSE, FALSE);
        otmp->quan = (long) cnt;
        otmp->owt = weight(otmp);
        context.botl = 1;
        return otmp;
    }

    /* check for single character object class code ("/" for wand, &c) */
    if (strlen(bp) == 1 && (i = def_char_to_objclass(*bp)) < MAXOCLASSES
        && i > ILLOBJ_CLASS && (i != VENOM_CLASS || wizard)) {
        oclass = i;
        goto any;
    }

#if 0 /*KR: 원본*/
    /*KR 영어에서는 XXXXX potion은 불확정명,
     * potion of XXXXX는 확정명이라고 한다.
     * 구분은 가능하지만, 한국어에서는 둘 다
     * "XXXXX의 약"이므로 여기서는 구분하지 않는다 */
    /* Search for class names: XXXXX potion, scroll of XXXXX.  Avoid */
    /* false hits on, e.g., rings for "ring mail". */
    if (strncmpi(bp, "enchant ", 8)
        && strncmpi(bp, "destroy ", 8)
        && strncmpi(bp, "detect food", 11)
        && strncmpi(bp, "food detection", 14)
        && strncmpi(bp, "ring mail", 9)
        && strncmpi(bp, "studded leather armor", 21)
        && strncmpi(bp, "leather armor", 13)
        && strncmpi(bp, "tooled horn", 11)
        && strncmpi(bp, "food ration", 11)
        && strncmpi(bp, "meat ring", 9))
        for (i = 0; i < (int) (sizeof wrpsym); i++) {
            register int j = strlen(wrp[i]);

            /* check for "<class> [ of ] something" */
            if (!strncmpi(bp, wrp[i], j)) {
                oclass = wrpsym[i];
                if (oclass != AMULET_CLASS) {
                    bp += j;
                    if (!strncmpi(bp, " of ", 4))
                        actualn = bp + 4;
                    /* else if(*bp) ?? */
                } else
                    actualn = bp;
                goto srch;
            }
            /* check for "something <class>" */
            if (!BSTRCMPI(bp, p - j, wrp[i])) {
                oclass = wrpsym[i];
                /* for "foo amulet", leave the class name so that
                   wishymatch() can do "of inversion" to try matching
                   "amulet of foo"; other classes don't include their
                   class name in their full object names (where
                   "potion of healing" is just "healing", for instance) */
                if (oclass != AMULET_CLASS) {
                    p -= j;
                    *p = '\0';
                    if (p > bp && p[-1] == ' ')
                        p[-1] = '\0';
                } else {
                    /* amulet without "of"; convoluted wording but better a
                       special case that's handled than one that's missing */
                    if (!strncmpi(bp, "versus poison ", 14)) {
                        typ = AMULET_VERSUS_POISON;
                        goto typfnd;
                    }
                }
                actualn = dn = bp;
                goto srch;
            }
        }
#else /*KR: KRNethack 맞춤 번역 - 영/한 클래스명 동시 추출 지원 */
    /* Search for class names: XXXXX potion, scroll of XXXXX.  Avoid */
    /* false hits on, e.g., rings for "ring mail". */
    if (strncmpi(bp, "enchant ", 8) && strncmpi(bp, "destroy ", 8)
        && strncmpi(bp, "detect food", 11)
        && strncmpi(bp, "food detection", 14) && strncmpi(bp, "ring mail", 9)
        && strncmpi(bp, "studded leather armor", 21)
        && strncmpi(bp, "leather armor", 13)
        && strncmpi(bp, "tooled horn", 11) && strncmpi(bp, "food ration", 11)
        && strncmpi(bp, "meat ring", 9)) {
        /* 1. 영문 어순 처리 (scroll of genocide, healing potion 등 원본 기능)
         */
        for (i = 0; i < (int) (sizeof wrpsym); i++) {
            register int j = strlen(wrp[i]);

            if (!strncmpi(bp, wrp[i], j)) {
                oclass = wrpsym[i];
                if (oclass != AMULET_CLASS) {
                    bp += j;
                    if (!strncmpi(bp, " of ", 4))
                        actualn = bp + 4;
                } else
                    actualn = bp;
                goto srch;
            }
            if (!BSTRCMPI(bp, p - j, wrp[i])) {
                oclass = wrpsym[i];
                if (oclass != AMULET_CLASS) {
                    p -= j;
                    *p = '\0';
                    if (p > bp && p[-1] == ' ')
                        p[-1] = '\0';
                } else {
                    if (!strncmpi(bp, "versus poison ", 14)) {
                        typ = AMULET_VERSUS_POISON;
                        goto typfnd;
                    }
                }
                actualn = dn = bp;
                goto srch;
            }
        }
    }

/* 2. 한국어 어순 처리 ("학살의 두루마리", "치료 물약" 등 추가 기능) */
    /* 한국어 고유 명사 예외 처리 (절단기에 남아있는 클래스명과 겹치는 고유
     * 아이템들만 추가) */
    if (strcmp(bp, "소설책") != 0
        && strcmp(bp, "아스클레피오스의 지팡이") != 0
        && strcmp(bp, "지팡이") != 0) {
        static const struct {
            const char *kname;
            char ksym;
        } kr_wrp[] = {
            /* 주의: 부적, 갑옷, 무기는 korean.c에 이미 풀네임으로
               번역되었으므로 절대 여기서 자르지 않도록 배열에서 삭제했습니다!
             */
            { "의 지팡이", WAND_CLASS },     { " 지팡이", WAND_CLASS },
            { "의 반지", RING_CLASS },       { " 반지", RING_CLASS },
            { "의 물약", POTION_CLASS },     { " 물약", POTION_CLASS },
            { "의 두루마리", SCROLL_CLASS }, { " 두루마리", SCROLL_CLASS },
            { "의 보석", GEM_CLASS },        { " 보석", GEM_CLASS },
            { "의 주문서", SPBOOK_CLASS },   { " 주문서", SPBOOK_CLASS },
            { "의 책", SPBOOK_CLASS },       { " 책", SPBOOK_CLASS }
        };

        int k, j;
        for (k = 0; k < SIZE(kr_wrp); k++) {
            j = strlen(kr_wrp[k].kname);

            /* 안전장치: 입력 문자열이 접미사보다 짧으면 비교하지 않음 (메모리
             * 크래시 완벽 방어) */
            if ((p - bp) >= j && !BSTRCMPI(bp, p - j, kr_wrp[k].kname)) {
                oclass = kr_wrp[k].ksym; /* SCROLL_CLASS 등 설정 */
                p -= j;                  /* "~의 두루마리" 등 꼬리 자르기 */
                *p = '\0';

                /* 혹시 앞에 공백이 남았다면 깔끔하게 정리 (예: "학살 " ->
                 * "학살") */
                if (p > bp && p[-1] == ' ') {
                    p[-1] = '\0';
                }
                actualn = dn = bp; /* 순수한 알맹이만 남김 */
                goto srch;         /* 검색으로 직행 */
            }
        }
    }
#endif


    /* Wishing in wizard mode can create traps and furniture.
     * Part I:  distinguish between trap and object for the two
     * types of traps which have corresponding objects:  bear trap
     * and land mine.  "beartrap" (object) and "bear trap" (trap)
     * have a difference in spelling which we used to exploit by
     * adding a special case in wishymatch(), but "land mine" is
     * spelled the same either way so needs different handing.
     * Since we need something else for land mine, we've dropped
     * the bear trap hack so that both are handled exactly the
     * same.  To get an armed trap instead of a disarmed object,
     * the player can prefix either the object name or the trap
     * name with "trapped " (which ordinarily applies to chests
     * and tins), or append something--anything at all except for
     * " object", but " trap" is suggested--to either the trap
     * name or the object name.
     */
    if (wizard && (!strncmpi(bp, "bear", 4) || !strncmpi(bp, "land", 4))) {
        boolean beartrap = (lowc(*bp) == 'b');
        char *zp = bp + 4; /* skip "bear"/"land" */

        if (*zp == ' ')
            ++zp; /* embedded space is optional */
        if (!strncmpi(zp, beartrap ? "trap" : "mine", 4)) {
            zp += 4;
            if (trapped == 2 || !strcmpi(zp, " object")) {
                /* "untrapped <foo>" or "<foo> object" */
                typ = beartrap ? BEARTRAP : LAND_MINE;
                goto typfnd;
            } else if (trapped == 1 || *zp != '\0') {
                /* "trapped <foo>" or "<foo> trap" (actually "<foo>*") */
                int idx = trap_to_defsym(beartrap ? BEAR_TRAP : LANDMINE);

                /* use canonical trap spelling, skip object matching */
                Strcpy(bp, defsyms[idx].explanation);
                goto wiztrap;
            }
            /* [no prefix or suffix; we're going to end up matching
               the object name and getting a disarmed trap object] */
        }
    }

 retry:

#if 0 /*KR 타입별로는 일단 하지 않는다 */
    /* "grey stone" check must be before general "stone" */
    for (i = 0; i < SIZE(o_ranges); i++)
        if (!strcmpi(bp, o_ranges[i].name)) {
            typ = rnd_class(o_ranges[i].f_o_range, o_ranges[i].l_o_range);
            goto typfnd;
        }
#endif

#if 0 /*KR 돌의 특별 처리는 불필요 */
    if (!BSTRCMPI(bp, p - 6, " stone") || !BSTRCMPI(bp, p - 4, " gem")) {
        p[!strcmpi(p - 4, " gem") ? -4 : -6] = '\0';
        oclass = GEM_CLASS;
        dn = actualn = bp;
        goto srch;
    } else if (!strcmpi(bp, "looking glass")) {
        ; /* avoid false hit on "* glass" */
    } else if (!BSTRCMPI(bp, p - 6, " glass") || !strcmpi(bp, "glass")) {
        register char *g = bp;

        /* treat "broken glass" as a non-existent item; since "broken" is
           also a chest/box prefix it might have been stripped off above */
        if (broken || strstri(g, "broken"))
            return (struct obj *) 0;
        if (!strncmpi(g, "worthless ", 10))
            g += 10;
        if (!strncmpi(g, "piece of ", 9))
            g += 9;
        if (!strncmpi(g, "colored ", 8))
            g += 8;
        else if (!strncmpi(g, "coloured ", 9))
            g += 9;
        if (!strcmpi(g, "glass")) { /* choose random color */
            /* 9 different kinds */
            typ = LAST_GEM + rnd(9);
            if (objects[typ].oc_class == GEM_CLASS)
                goto typfnd;
            else
                typ = 0; /* somebody changed objects[]? punt */
        } else { /* try to construct canonical form */
            char tbuf[BUFSZ];

            Strcpy(tbuf, "worthless piece of ");
            Strcat(tbuf, g); /* assume it starts with the color */
            Strcpy(bp, tbuf);
        }
    }
#endif

    actualn = bp;
    if (!dn)
        dn = actualn; /* ex. "skull cap" */

 srch:

    /* check real names of gems first */
    if (!oclass && actualn) {
        for (i = bases[GEM_CLASS]; i <= LAST_GEM; i++) {
            register const char *zn;

            if ((zn = OBJ_NAME(objects[i])) != 0 && !strcmpi(actualn, zn)) {
                typ = i;
                goto typfnd;
            }
        }
#if 0 /*KR: 원본*/
        /* "tin of foo" would be caught above, but plain "tin" has
           a random chance of yielding "tin wand" unless we do this */
        if (!strcmpi(actualn, "tin")) {
#else /*KR: KRNethack 맞춤 번역 (영어 지원을 위해 원본 복구 + 한글 추가) */
        /* 영문 입력 시 'tin(주석)' 지팡이와 혼동되는 것을 막는 필수 코드 */
        if (!strcmpi(actualn, "tin") || !strcmp(actualn, "통조림")) {
#endif
            typ = TIN;
            goto typfnd;
        }
    }

    if (((typ = rnd_otyp_by_namedesc(actualn, oclass, 1)) != STRANGE_OBJECT)
        || ((typ = rnd_otyp_by_namedesc(dn, oclass, 1)) != STRANGE_OBJECT)
        || ((typ = rnd_otyp_by_namedesc(un, oclass, 1)) != STRANGE_OBJECT)
        || ((typ = rnd_otyp_by_namedesc(origbp, oclass, 1)) != STRANGE_OBJECT))
        goto typfnd;
    typ = 0;

    /* if we've stripped off "armor" and failed to match anything
       in objects[], append "mail" and try again to catch misnamed
       requests like "plate armor" and "yellow dragon scale armor" */
    if (oclass == ARMOR_CLASS && !strstri(bp, "mail")) {
        /* modifying bp's string is ok; we're about to resort
           to random armor if this also fails to match anything */
        Strcat(bp, " mail");
        goto retry;
    }
#if 0 /*KR: 원본*/
    if (!strcmpi(bp, "spinach")) {
#else /*KR: KRNethack 맞춤 번역 (영어+한글 동시 지원) */
    if (!strcmpi(bp, "spinach") || !strcmp(bp, "시금치")) {
#endif
        contents = SPINACH;
        typ = TIN;
        goto typfnd;
    }
    /* Note: not strcmpi.  2 fruits, one capital, one not, are possible.
       Also not strncmp.  We used to ignore trailing text with it, but
       that resulted in "grapefruit" matching "grape" if the latter came
       earlier than the former in the fruit list. */
    {
        char *fp;
        int l, cntf;
        int blessedf, iscursedf, uncursedf, halfeatenf;

        blessedf = iscursedf = uncursedf = halfeatenf = 0;
        cntf = 0;

        fp = fruitbuf;
        for (;;) {
            if (!fp || !*fp)
                break;
            if (!strncmpi(fp, "an ", l = 3) || !strncmpi(fp, "a ", l = 2)) {
                cntf = 1;
            } else if (!cntf && digit(*fp)) {
                cntf = atoi(fp);
                while (digit(*fp))
                    fp++;
                while (*fp == ' ')
                    fp++;
                l = 0;
            } else if (!strncmpi(fp, "blessed ", l = 8)) {
                blessedf = 1;
            } else if (!strncmpi(fp, "cursed ", l = 7)) {
                iscursedf = 1;
            } else if (!strncmpi(fp, "uncursed ", l = 9)) {
                uncursedf = 1;
            } else if (!strncmpi(fp, "partly eaten ", l = 13)
                       || !strncmpi(fp, "partially eaten ", l = 16)) {
                halfeatenf = 1;
            } else
                break;
            fp += l;
        }

        for (f = ffruit; f; f = f->nextf) {
            /* match type: 0=none, 1=exact, 2=singular, 3=plural */
            int ftyp = 0;

            if (!strcmp(fp, f->fname))
                ftyp = 1;
            else if (!strcmp(fp, makesingular(f->fname)))
                ftyp = 2;
            else if (!strcmp(fp, makeplural(f->fname)))
                ftyp = 3;
            if (ftyp) {
                typ = SLIME_MOLD;
                blessed = blessedf;
                iscursed = iscursedf;
                uncursed = uncursedf;
                halfeaten = halfeatenf;
                /* adjust count if user explicitly asked for
                   singular amount (can't happen unless fruit
                   has been given an already pluralized name)
                   or for plural amount */
                if (ftyp == 2 && !cntf)
                    cntf = 1;
                else if (ftyp == 3 && !cntf)
                    cntf = 2;
                cnt = cntf;
                ftype = f->fid;
                goto typfnd;
            }
        }
    }

    if (!oclass && actualn) {
        short objtyp;

        /* Perhaps it's an artifact specified by name, not type */
        name = artifact_name(actualn, &objtyp);
        if (name) {
            typ = objtyp;
            goto typfnd;
        }
    }

    /*
     * Let wizards wish for traps and furniture.
     * Must come after objects check so wizards can still wish for
     * trap objects like beartraps.
     * Disallow such topology tweaks for WIZKIT startup wishes.
     */
 wiztrap:
    if (wizard && !program_state.wizkit_wishing) {
        struct rm *lev;
        boolean madeterrain = FALSE;
        int trap, x = u.ux, y = u.uy;

        for (trap = NO_TRAP + 1; trap < TRAPNUM; trap++) {
            struct trap *t;
            const char *tname;

            tname = defsyms[trap_to_defsym(trap)].explanation;
            if (strncmpi(tname, bp, strlen(tname)))
                continue;
            /* found it; avoid stupid mistakes */
            if (is_hole(trap) && !Can_fall_thru(&u.uz))
                trap = ROCKTRAP;
            if ((t = maketrap(x, y, trap)) != 0) {
                trap = t->ttyp;
                tname = defsyms[trap_to_defsym(trap)].explanation;
                pline("%s%s.", An(tname),
                      (trap != MAGIC_PORTAL) ? "" : " to nowhere");
            } else
                pline("Creation of %s failed.", an(tname));
            return (struct obj *) &zeroobj;
        }

        /* furniture and terrain (use at your own risk; can clobber stairs
           or place furniture on existing traps which shouldn't be allowed) */
        lev = &levl[x][y];
        p = eos(bp);
        if (!BSTRCMPI(bp, p - 8, "fountain")) {
            lev->typ = FOUNTAIN;
            level.flags.nfountains++;
            if (!strncmpi(bp, "magic ", 6))
                lev->blessedftn = 1;
            pline("A %sfountain.", lev->blessedftn ? "magic " : "");
            madeterrain = TRUE;
        } else if (!BSTRCMPI(bp, p - 6, "throne")) {
            lev->typ = THRONE;
            pline("A throne.");
            madeterrain = TRUE;
        } else if (!BSTRCMPI(bp, p - 4, "sink")) {
            lev->typ = SINK;
            level.flags.nsinks++;
            pline("A sink.");
            madeterrain = TRUE;

        /* ("water" matches "potion of water" rather than terrain) */
        } else if (!BSTRCMPI(bp, p - 4, "pool")
                   || !BSTRCMPI(bp, p - 4, "moat")) {
            lev->typ = !BSTRCMPI(bp, p - 4, "pool") ? POOL : MOAT;
            del_engr_at(x, y);
            pline("A %s.", (lev->typ == POOL) ? "pool" : "moat");
            /* Must manually make kelp! */
            water_damage_chain(level.objects[x][y], TRUE);
            madeterrain = TRUE;

        /* also matches "molten lava" */
        } else if (!BSTRCMPI(bp, p - 4, "lava")) {
            lev->typ = LAVAPOOL;
            del_engr_at(x, y);
            pline("A pool of molten lava.");
            if (!(Levitation || Flying))
                pooleffects(FALSE);
            madeterrain = TRUE;
        } else if (!BSTRCMPI(bp, p - 5, "altar")) {
            aligntyp al;

            lev->typ = ALTAR;
            if (!strncmpi(bp, "chaotic ", 8))
                al = A_CHAOTIC;
            else if (!strncmpi(bp, "neutral ", 8))
                al = A_NEUTRAL;
            else if (!strncmpi(bp, "lawful ", 7))
                al = A_LAWFUL;
            else if (!strncmpi(bp, "unaligned ", 10))
                al = A_NONE;
            else /* -1 - A_CHAOTIC, 0 - A_NEUTRAL, 1 - A_LAWFUL */
                al = !rn2(6) ? A_NONE : (rn2((int) A_LAWFUL + 2) - 1);
            lev->altarmask = Align2amask(al);
            pline("%s altar.", An(align_str(al)));
            madeterrain = TRUE;
        } else if (!BSTRCMPI(bp, p - 5, "grave")
                   || !BSTRCMPI(bp, p - 9, "headstone")) {
            make_grave(x, y, (char *) 0);
            pline("%s.", IS_GRAVE(lev->typ) ? "A grave"
                                            : "Can't place a grave here");
            madeterrain = TRUE;
        } else if (!BSTRCMPI(bp, p - 4, "tree")) {
            lev->typ = TREE;
            pline("A tree.");
            block_point(x, y);
            madeterrain = TRUE;
        } else if (!BSTRCMPI(bp, p - 4, "bars")) {
            lev->typ = IRONBARS;
            pline("Iron bars.");
            madeterrain = TRUE;
        }

        if (madeterrain) {
            feel_newsym(x, y); /* map the spot where the wish occurred */
            /* hero started at <x,y> but might not be there anymore (create
               lava, decline to die, and get teleported away to safety) */
            if (u.uinwater && !is_pool(u.ux, u.uy)) {
                u.uinwater = 0; /* leave the water */
                docrt();
                vision_full_recalc = 1;
            } else if (u.utrap && u.utraptype == TT_LAVA
                       && !is_lava(u.ux, u.uy)) {
                reset_utrap(FALSE);
            }
            /* cast 'const' away; caller won't modify this */
            return (struct obj *) &zeroobj;
        }
    } /* end of wizard mode traps and terrain */

#if 0 /*KR 타입별은 어쨌든 하지 않는다 */
    if (!oclass && !typ) {
        if (!strncmpi(bp, "polearm", 7)) {
            typ = rnd_otyp_by_wpnskill(P_POLEARMS);
            goto typfnd;
        } else if (!strncmpi(bp, "hammer", 6)) {
            typ = rnd_otyp_by_wpnskill(P_HAMMER);
            goto typfnd;
        }
    }
#endif

    if (!oclass)
        return ((struct obj *) 0);
 any:
    if (!oclass)
        oclass = wrpsym[rn2((int) sizeof wrpsym)];
 typfnd:
    if (typ)
        oclass = objects[typ].oc_class;

    /* handle some objects that are only allowed in wizard mode */
    if (typ && !wizard) {
        switch (typ) {
        case AMULET_OF_YENDOR:
            typ = FAKE_AMULET_OF_YENDOR;
            break;
        case CANDELABRUM_OF_INVOCATION:
            typ = rnd_class(TALLOW_CANDLE, WAX_CANDLE);
            break;
        case BELL_OF_OPENING:
            typ = BELL;
            break;
        case SPE_BOOK_OF_THE_DEAD:
            typ = SPE_BLANK_PAPER;
            break;
        case MAGIC_LAMP:
            typ = OIL_LAMP;
            break;
        default:
            /* catch any other non-wishable objects (venom) */
            if (objects[typ].oc_nowish)
                return (struct obj *) 0;
            break;
        }
    }

    /*
     * Create the object, then fine-tune it.
     */
    otmp = typ ? mksobj(typ, TRUE, FALSE) : mkobj(oclass, FALSE);
    typ = otmp->otyp, oclass = otmp->oclass; /* what we actually got */

    if (islit && (typ == OIL_LAMP || typ == MAGIC_LAMP || typ == BRASS_LANTERN
                  || Is_candle(otmp) || typ == POT_OIL)) {
        place_object(otmp, u.ux, u.uy); /* make it viable light source */
        begin_burn(otmp, FALSE);
        obj_extract_self(otmp); /* now release it for caller's use */
    }

    /* if player specified a reasonable count, maybe honor it */
    if (cnt > 0 && objects[typ].oc_merge
        && (wizard || cnt < rnd(6) || (cnt <= 7 && Is_candle(otmp))
            || (cnt <= 20 && ((oclass == WEAPON_CLASS && is_ammo(otmp))
                              || typ == ROCK || is_missile(otmp)))))
        otmp->quan = (long) cnt;

    if (oclass == VENOM_CLASS)
        otmp->spe = 1;

    if (spesgn == 0) {
        spe = otmp->spe;
    } else if (wizard) {
        ; /* no alteration to spe */
    } else if (oclass == ARMOR_CLASS || oclass == WEAPON_CLASS
               || is_weptool(otmp)
               || (oclass == RING_CLASS && objects[typ].oc_charged)) {
        if (spe > rnd(5) && spe > otmp->spe)
            spe = 0;
        if (spe > 2 && Luck < 0)
            spesgn = -1;
    } else {
        if (oclass == WAND_CLASS) {
            if (spe > 1 && spesgn == -1)
                spe = 1;
        } else {
            if (spe > 0 && spesgn == -1)
                spe = 0;
        }
        if (spe > otmp->spe)
            spe = otmp->spe;
    }

    if (spesgn == -1)
        spe = -spe;

    /* set otmp->spe.  This may, or may not, use spe... */
    switch (typ) {
    case TIN:
        if (contents == EMPTY) {
            otmp->corpsenm = NON_PM;
            otmp->spe = 0;
        } else if (contents == SPINACH) {
            otmp->corpsenm = NON_PM;
            otmp->spe = 1;
        }
        break;
    case TOWEL:
        if (wetness)
            otmp->spe = wetness;
        break;
    case SLIME_MOLD:
        otmp->spe = ftype;
    /* Fall through */
    case SKELETON_KEY:
    case CHEST:
    case LARGE_BOX:
    case HEAVY_IRON_BALL:
    case IRON_CHAIN:
    case STATUE:
        /* otmp->cobj already done in mksobj() */
        break;
#ifdef MAIL
    case SCR_MAIL:
        /* 0: delivered in-game via external event (or randomly for fake mail);
           1: from bones or wishing; 2: written with marker */
        otmp->spe = 1;
        break;
#endif
    case WAN_WISHING:
        if (!wizard) {
            otmp->spe = (rn2(10) ? -1 : 0);
            break;
        }
        /*FALLTHRU*/
    default:
        otmp->spe = spe;
    }

    /* set otmp->corpsenm or dragon scale [mail] */
    if (mntmp >= LOW_PM) {
        if (mntmp == PM_LONG_WORM_TAIL)
            mntmp = PM_LONG_WORM;

        switch (typ) {
        case TIN:
            otmp->spe = 0; /* No spinach */
            if (dead_species(mntmp, FALSE)) {
                otmp->corpsenm = NON_PM; /* it's empty */
            } else if ((!(mons[mntmp].geno & G_UNIQ) || wizard)
                       && !(mvitals[mntmp].mvflags & G_NOCORPSE)
                       && mons[mntmp].cnutrit != 0) {
                otmp->corpsenm = mntmp;
            }
            break;
        case CORPSE:
            if ((!(mons[mntmp].geno & G_UNIQ) || wizard)
                && !(mvitals[mntmp].mvflags & G_NOCORPSE)) {
                if (mons[mntmp].msound == MS_GUARDIAN)
                    mntmp = genus(mntmp, 1);
                set_corpsenm(otmp, mntmp);
            }
            break;
        case EGG:
            mntmp = can_be_hatched(mntmp);
            /* this also sets hatch timer if appropriate */
            set_corpsenm(otmp, mntmp);
            break;
        case FIGURINE:
            if (!(mons[mntmp].geno & G_UNIQ) && !is_human(&mons[mntmp])
#ifdef MAIL
                && mntmp != PM_MAIL_DAEMON
#endif
                )
                otmp->corpsenm = mntmp;
            break;
        case STATUE:
            otmp->corpsenm = mntmp;
            if (Has_contents(otmp) && verysmall(&mons[mntmp]))
                delete_contents(otmp); /* no spellbook */
            otmp->spe = ishistoric ? STATUE_HISTORIC : 0;
            break;
        case SCALE_MAIL:
            /* Dragon mail - depends on the order of objects & dragons. */
            if (mntmp >= PM_GRAY_DRAGON && mntmp <= PM_YELLOW_DRAGON)
                otmp->otyp = GRAY_DRAGON_SCALE_MAIL + mntmp - PM_GRAY_DRAGON;
            break;
        }
    }

    /* set blessed/cursed -- setting the fields directly is safe
     * since weight() is called below and addinv() will take care
     * of luck */
    if (iscursed) {
        curse(otmp);
    } else if (uncursed) {
        otmp->blessed = 0;
        otmp->cursed = (Luck < 0 && !wizard);
    } else if (blessed) {
        otmp->blessed = (Luck >= 0 || wizard);
        otmp->cursed = (Luck < 0 && !wizard);
    } else if (spesgn < 0) {
        curse(otmp);
    }

    /* set eroded and erodeproof */
    if (erosion_matters(otmp)) {
        if (eroded && (is_flammable(otmp) || is_rustprone(otmp)))
            otmp->oeroded = eroded;
        if (eroded2 && (is_corrodeable(otmp) || is_rottable(otmp)))
            otmp->oeroded2 = eroded2;
        /*
         * 3.6.1: earlier versions included `&& !eroded && !eroded2' here,
         * but damageproof combined with damaged is feasible (eroded
         * armor modified by confused reading of cursed destroy armor)
         * so don't prevent player from wishing for such a combination.
         */
        if (erodeproof && (is_damageable(otmp) || otmp->otyp == CRYSKNIFE))
            otmp->oerodeproof = (Luck >= 0 || wizard);
    }

    /* set otmp->recharged */
    if (oclass == WAND_CLASS) {
        /* prevent wishing abuse */
        if (otmp->otyp == WAN_WISHING && !wizard)
            rechrg = 1;
        otmp->recharged = (unsigned) rechrg;
    }

    /* set poisoned */
    if (ispoisoned) {
        if (is_poisonable(otmp))
            otmp->opoisoned = (Luck >= 0);
        else if (oclass == FOOD_CLASS)
            /* try to taint by making it as old as possible */
            otmp->age = 1L;
    }
    /* and [un]trapped */
    if (trapped) {
        if (Is_box(otmp) || typ == TIN)
            otmp->otrapped = (trapped == 1);
    }
    /* empty for containers rather than for tins */
    if (contents == EMPTY) {
        if (otmp->otyp == BAG_OF_TRICKS || otmp->otyp == HORN_OF_PLENTY) {
            if (otmp->spe > 0)
                otmp->spe = 0;
        } else if (Has_contents(otmp)) {
            /* this assumes that artifacts can't be randomly generated
               inside containers */
            delete_contents(otmp);
            otmp->owt = weight(otmp);
        }
    }
    /* set locked/unlocked/broken */
    if (Is_box(otmp)) {
        if (locked) {
            otmp->olocked = 1, otmp->obroken = 0;
        } else if (unlocked) {
            otmp->olocked = 0, otmp->obroken = 0;
        } else if (broken) {
            otmp->olocked = 0, otmp->obroken = 1;
        }
    }

    if (isgreased)
        otmp->greased = 1;

    if (isdiluted && otmp->oclass == POTION_CLASS && otmp->otyp != POT_WATER)
        otmp->odiluted = 1;

    /* set tin variety */
    if (otmp->otyp == TIN && tvariety >= 0 && (rn2(4) || wizard))
        set_tin_variety(otmp, tvariety);

    if (name) {
        const char *aname;
        short objtyp;

        /* an artifact name might need capitalization fixing */
        aname = artifact_name(name, &objtyp);
        if (aname && objtyp == otmp->otyp)
            name = aname;

        /* 3.6 tribute - fix up novel */
        if (otmp->otyp == SPE_NOVEL) {
            const char *novelname;

            novelname = lookup_novel(name, &otmp->novelidx);
            if (novelname)
                name = novelname;
        }

        otmp = oname(otmp, name);
        /* name==aname => wished for artifact (otmp->oartifact => got it) */
        if (otmp->oartifact || name == aname) {
            otmp->quan = 1L;
            u.uconduct.wisharti++; /* KMH, conduct */
        }
    }

    /* more wishing abuse: don't allow wishing for certain artifacts */
    /* and make them pay; charge them for the wish anyway! */
    if ((is_quest_artifact(otmp)
         || (otmp->oartifact && rn2(nartifact_exist()) > 1)) && !wizard) {
        artifact_exists(otmp, safe_oname(otmp), FALSE);
        obfree(otmp, (struct obj *) 0);
        otmp = (struct obj *) &zeroobj;
#if 0 /*KR: 원본*/
        pline("For a moment, you feel %s in your %s, but it disappears!",
              something, makeplural(body_part(HAND)));
#else
        pline("잠시 %s %s 안에 있는 것 같은 느낌이 들었지만, 곧 사라졌다!",
              append_josa(something, "이"), makeplural(body_part(HAND)));
#endif
        return otmp;
    }

    if (halfeaten && otmp->oclass == FOOD_CLASS) {
        if (otmp->otyp == CORPSE)
            otmp->oeaten = mons[otmp->corpsenm].cnutrit;
        else
            otmp->oeaten = objects[otmp->otyp].oc_nutrition;
        /* (do this adjustment before setting up object's weight) */
        consume_oeaten(otmp, 1);
    }
    otmp->owt = weight(otmp);
    if (very && otmp->otyp == HEAVY_IRON_BALL)
        otmp->owt += IRON_BALL_W_INCR;
    else if (gsize > 1 && otmp->globby)
        /* 0: unspecified => small; 1: small => keep default owt of 20;
           2: medium => 120; 3: large => 320; 4: very large => 520 */
        otmp->owt += 100 + (gsize - 2) * 200;

    return otmp;
}

int
rnd_class(first, last)
int first, last;
{
    int i, x, sum = 0;

    if (first == last)
        return first;
    for (i = first; i <= last; i++)
        sum += objects[i].oc_prob;
    if (!sum) /* all zero */
        return first + rn2(last - first + 1);
    x = rnd(sum);
    for (i = first; i <= last; i++)
        if (objects[i].oc_prob && (x -= objects[i].oc_prob) <= 0)
            return i;
    return 0;
}

const char *
suit_simple_name(suit)
struct obj *suit;
{
    const char *suitnm, *esuitp;

    if (suit) {
        if (Is_dragon_mail(suit))
#if 0 /*KR:T*/
            return "dragon mail"; /* <color> dragon scale mail */
#else
            return "용 비늘 갑옷"; /* <color> dragon scale mail */
#endif
        else if (Is_dragon_scales(suit))
            /*KR return "dragon scales"; */
            return "용 비늘";
        suitnm = OBJ_NAME(objects[suit->otyp]);
        esuitp = eos((char *) suitnm);
#if 0 /*KR: 원본*/
        if (strlen(suitnm) > 5 && !strcmp(esuitp - 5, " mail"))
            return "mail"; /* most suits fall into this category */
        else if (strlen(suitnm) > 7 && !strcmp(esuitp - 7, " jacket"))
            return "jacket"; /* leather jacket */
#else /*KR: KRNethack 맞춤 번역 */
        /* UTF-8에서 "갑옷"은 6바이트이므로 길이를 6으로 체크합니다. */
        if (strlen(suitnm) > 6 && !strcmp(esuitp - 6, "갑옷"))
            return "갑옷";
        /* 영문판의 jacket 예외처리를 한글판에 맞게 복구합니다. */
        else if (strlen(suitnm) > 6 && !strcmp(esuitp - 6, "재킷"))
            return "재킷"; /* 가죽 재킷 */
#endif
    }
    /* "suit" is lame but "armor" is ambiguous and "body armor" is absurd */
    /*KR return "suit"; */
    return "갑옷";
}

const char *
cloak_simple_name(cloak)
struct obj *cloak;
{
    if (cloak) {
        switch (cloak->otyp) {
        case ROBE:
            /*KR return "robe"; */
            return "로브";
        case MUMMY_WRAPPING:
            /*KR return "wrapping"; */
            return "미라 붕대";
        case ALCHEMY_SMOCK:
#if 0 /*KR: 원본*/
            return (objects[cloak->otyp].oc_name_known && cloak->dknown)
                       ? "smock" : "apron";
#else /*KR: KRNethack 맞춤 번역 */
            return (objects[cloak->otyp].oc_name_known && cloak->dknown)
                       ? "작업복" : "앞치마";
#endif
        default:
            break;
        }
    }
    /*KR return "cloak"; */
    return "망토";
}

/* helm vs hat for messages */
const char *
helm_simple_name(helmet)
struct obj *helmet;
{
    /*
     *  There is some wiggle room here; the result has been chosen
     *  for consistency with the "protected by hard helmet" messages
     *  given for various bonks on the head:  headgear that provides
     *  such protection is a "helm", that which doesn't is a "hat".
     *
     *      elven leather helm / leather hat    -> hat
     *      dwarvish iron helm / hard hat       -> helm
     *  The rest are completely straightforward:
     *      fedora, cornuthaum, dunce cap       -> hat
     *      all other types of helmets          -> helm
     */
    /*KR return (helmet && !is_metallic(helmet)) ? "hat" : "helm";*/
    return (helmet && !is_metallic(helmet)) ? "모자" : "투구";
}

/* gloves vs gauntlets; depends upon discovery state */
const char *
gloves_simple_name(gloves)
struct obj *gloves;
{
    /*KR static const char gauntlets[] = "gauntlets"; */
    static const char gauntlets[] = "건틀릿";

    if (gloves && gloves->dknown) {
        int otyp = gloves->otyp;
        struct objclass *ocl = &objects[otyp];
        const char *actualn = OBJ_NAME(*ocl),
                   *descrpn = OBJ_DESCR(*ocl);

        if (strstri(objects[otyp].oc_name_known ? actualn : descrpn,
                    gauntlets))
            return gauntlets;
    }
    /*KR return "gloves"; */
    return "장갑";
}

const char *
mimic_obj_name(mtmp)
struct monst *mtmp;
{
    if (M_AP_TYPE(mtmp) == M_AP_OBJECT) {
        if (mtmp->mappearance == GOLD_PIECE)
            /*KR return "gold"; */
            return "금화";
        if (mtmp->mappearance != STRANGE_OBJECT)
            return simple_typename(mtmp->mappearance);
    }
    /*KR return "whatcha-may-callit"; */
    return "정체불명의 물건";
}

/*
 * Construct a query prompt string, based around an object name, which is
 * guaranteed to fit within [QBUFSZ].  Takes an optional prefix, three
 * choices for filling in the middle (two object formatting functions and a
 * last resort literal which should be very short), and an optional suffix.
 */
char *
safe_qbuf(qbuf, qprefix, qsuffix, obj, func, altfunc, lastR)
char *qbuf; /* output buffer */
const char *qprefix, *qsuffix;
struct obj *obj;
char *FDECL((*func), (OBJ_P)), *FDECL((*altfunc), (OBJ_P));
const char *lastR;
{
    char *bufp, *endp;
    /* convert size_t (or int for ancient systems) to ordinary unsigned */
    unsigned len, lenlimit,
        len_qpfx = (unsigned) (qprefix ? strlen(qprefix) : 0),
        len_qsfx = (unsigned) (qsuffix ? strlen(qsuffix) : 0),
        len_lastR = (unsigned) strlen(lastR);

    lenlimit = QBUFSZ - 1;
    endp = qbuf + lenlimit;
    /* sanity check, aimed mainly at paniclog (it's conceivable for
       the result of short_oname() to be shorter than the length of
       the last resort string, but we ignore that possibility here) */
    if (len_qpfx > lenlimit)
        impossible("safe_qbuf: prefix too long (%u characters).", len_qpfx);
    else if (len_qpfx + len_qsfx > lenlimit)
        impossible("safe_qbuf: suffix too long (%u + %u characters).",
                   len_qpfx, len_qsfx);
    else if (len_qpfx + len_lastR + len_qsfx > lenlimit)
        impossible("safe_qbuf: filler too long (%u + %u + %u characters).",
                   len_qpfx, len_lastR, len_qsfx);

    /* the output buffer might be the same as the prefix if caller
       has already partially filled it */
    if (qbuf == qprefix) {
        /* prefix is already in the buffer */
        *endp = '\0';
    } else if (qprefix) {
        /* put prefix into the buffer */
        (void) strncpy(qbuf, qprefix, lenlimit);
        *endp = '\0';
    } else {
        /* no prefix; output buffer starts out empty */
        qbuf[0] = '\0';
    }
    len = (unsigned) strlen(qbuf);

    if (len + len_lastR + len_qsfx > lenlimit) {
        /* too long; skip formatting, last resort output is truncated */
        if (len < lenlimit) {
            (void) strncpy(&qbuf[len], lastR, lenlimit - len);
            *endp = '\0';
            len = (unsigned) strlen(qbuf);
            if (qsuffix && len < lenlimit) {
                (void) strncpy(&qbuf[len], qsuffix, lenlimit - len);
                *endp = '\0';
                /* len = (unsigned) strlen(qbuf); */
            }
        }
    } else {
        /* suffix and last resort are guaranteed to fit */
        len += len_qsfx; /* include the pending suffix */
        /* format the object */
        bufp = short_oname(obj, func, altfunc, lenlimit - len);
        if (len + strlen(bufp) <= lenlimit)
            Strcat(qbuf, bufp); /* formatted name fits */
        else
            Strcat(qbuf, lastR); /* use last resort */
        releaseobuf(bufp);

        if (qsuffix)
            Strcat(qbuf, qsuffix);
    }
    /* assert( strlen(qbuf) < QBUFSZ ); */
    return qbuf;
}

STATIC_OVL char *
globwt(otmp, buf, weightformatted_p)
struct obj *otmp;
char *buf;
boolean *weightformatted_p;
{
    *buf = '\0';
    if (otmp->globby) {
        Sprintf(buf, "%u aum, ", otmp->owt);
        *weightformatted_p = TRUE;
    } else {
        *weightformatted_p = FALSE;
    }
    return buf;
}

/*objnam.c*/
