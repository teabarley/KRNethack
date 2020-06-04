/* NetHack 3.6	engrave.c	$NHDT-Date: 1570318925 2019/10/05 23:42:05 $  $NHDT-Branch: NetHack-3.6 $:$NHDT-Revision: 1.75 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2012. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "lev.h"

STATIC_VAR NEARDATA struct engr *head_engr;
STATIC_DCL const char *NDECL(blengr);

char *
random_engraving(outbuf)
char *outbuf;
{
    const char *rumor;

    /* a random engraving may come from the "rumors" file,
       or from the "engrave" file (formerly in an array here) */
    if (!rn2(4) || !(rumor = getrumor(0, outbuf, TRUE)) || !*rumor)
        (void) get_rnd_text(ENGRAVEFILE, outbuf, rn2);

    wipeout_text(outbuf, (int) (strlen(outbuf) / 4), 0);
    return outbuf;
}

/* Partial rubouts for engraving characters. -3. */
static const struct {
    char wipefrom;
    const char *wipeto;
} rubouts[] = { { 'A', "^" },
                { 'B', "Pb[" },
                { 'C', "(" },
                { 'D', "|)[" },
                { 'E', "|FL[_" },
                { 'F', "|-" },
                { 'G', "C(" },
                { 'H', "|-" },
                { 'I', "|" },
                { 'K', "|<" },
                { 'L', "|_" },
                { 'M', "|" },
                { 'N', "|\\" },
                { 'O', "C(" },
                { 'P', "F" },
                { 'Q', "C(" },
                { 'R', "PF" },
                { 'T', "|" },
                { 'U', "J" },
                { 'V', "/\\" },
                { 'W', "V/\\" },
                { 'Z', "/" },
                { 'b', "|" },
                { 'd', "c|" },
                { 'e', "c" },
                { 'g', "c" },
                { 'h', "n" },
                { 'j', "i" },
                { 'k', "|" },
                { 'l', "|" },
                { 'm', "nr" },
                { 'n', "r" },
                { 'o', "c" },
                { 'q', "c" },
                { 'w', "v" },
                { 'y', "v" },
                { ':', "." },
                { ';', ",:" },
                { ',', "." },
                { '=', "-" },
                { '+', "-|" },
                { '*', "+" },
                { '@', "0" },
                { '0', "C(" },
                { '1', "|" },
                { '6', "o" },
                { '7', "/" },
                { '8', "3o" } };

/* degrade some of the characters in a string */
void
wipeout_text(engr, cnt, seed)
char *engr;
int cnt;
unsigned seed; /* for semi-controlled randomization */
{
    char *s;
    int i, j, nxt, use_rubout, lth = (int) strlen(engr);

    if (lth && cnt > 0) {
        while (cnt--) {
            /* pick next character */
            if (!seed) {
                /* random */
                nxt = rn2(lth);
                use_rubout = rn2(4);
            } else {
                /* predictable; caller can reproduce the same sequence by
                   supplying the same arguments later, or a pseudo-random
                   sequence by varying any of them */
                nxt = seed % lth;
                seed *= 31, seed %= (BUFSZ - 1);
                use_rubout = seed & 3;
            }
            s = &engr[nxt];
            if (*s == ' ')
                continue;

            /* rub out unreadable & small punctuation marks */
            if (index("?.,'`-|_", *s)) {
                *s = ' ';
                continue;
            }

            if (!use_rubout)
                i = SIZE(rubouts);
            else
                for (i = 0; i < SIZE(rubouts); i++)
                    if (*s == rubouts[i].wipefrom) {
                        /*
                         * Pick one of the substitutes at random.
                         */
                        if (!seed)
                            j = rn2(strlen(rubouts[i].wipeto));
                        else {
                            seed *= 31, seed %= (BUFSZ - 1);
                            j = seed % (strlen(rubouts[i].wipeto));
                        }
                        *s = rubouts[i].wipeto[j];
                        break;
                    }

            /* didn't pick rubout; use '?' for unreadable character */
            if (i == SIZE(rubouts))
                *s = '?';
        }
    }

    /* trim trailing spaces */
    while (lth && engr[lth - 1] == ' ')
        engr[--lth] = '\0';
}

/* check whether hero can reach something at ground level */
boolean
can_reach_floor(check_pit)
boolean check_pit;
{
    struct trap *t;

    if (u.uswallow)
        return FALSE;
    /* Restricted/unskilled riders can't reach the floor */
    if (u.usteed && P_SKILL(P_RIDING) < P_BASIC)
        return FALSE;
    if (check_pit && !Flying
        && (t = t_at(u.ux, u.uy)) != 0
        && (uteetering_at_seen_pit(t) || uescaped_shaft(t)))
        return FALSE;

    return (boolean) ((!Levitation || Is_airlevel(&u.uz)
                       || Is_waterlevel(&u.uz))
                      && (!u.uundetected || !is_hider(youmonst.data)
                          || u.umonnum == PM_TRAPPER));
}

/* give a message after caller has determined that hero can't reach */
void
cant_reach_floor(x, y, up, check_pit)
int x, y;
boolean up, check_pit;
{
#if 0 /*KR:T*/
    You("can't reach the %s.",
        up ? ceiling(x, y)
           : (check_pit && can_reach_floor(FALSE))
               ? "bottom of the pit"
               : surface(x, y));
#else
    You("%s에 닿을 수 없다.",
        up ? ceiling(x, y)
        : (check_pit && can_reach_floor(FALSE))
        ? "구덩이의 바닥"
        : surface(x, y));
#endif
}

const char *
surface(x, y)
register int x, y;
{
    register struct rm *lev = &levl[x][y];

    if (x == u.ux && y == u.uy && u.uswallow && is_animal(u.ustuck->data))
        /*KR return "maw"; */
        return "위장";
    else if (IS_AIR(lev->typ) && Is_airlevel(&u.uz))
        /*KR return "air"; */
        return "공중";
    else if (is_pool(x, y))
#if 0 /*KR:T*/
        return (Underwater && !Is_waterlevel(&u.uz))
            ? "bottom" : hliquid("water");
#else
        return (Underwater && !Is_waterlevel(&u.uz))
        ? "물 밑바닥" : hliquid("수중");
#endif
    else if (is_ice(x, y))
        /*KR return "ice"; */
        return "얼음";
    else if (is_lava(x, y))
        /*KR return hliquid("lava"); */
        return hliquid("용암");
    else if (lev->typ == DRAWBRIDGE_DOWN)
        /*KR return "bridge"; */
        return "도개교";
    else if (IS_ALTAR(levl[x][y].typ))
        /*KR return "altar"; */
        return "제단";
    else if (IS_GRAVE(levl[x][y].typ))
        /*KR return "headstone"; */
        return "묘비";
    else if (IS_FOUNTAIN(levl[x][y].typ))
        /*KR return "fountain"; */
        return "분수";
    else if ((IS_ROOM(lev->typ) && !Is_earthlevel(&u.uz))
             || IS_WALL(lev->typ) || IS_DOOR(lev->typ) || lev->typ == SDOOR)
        /*KR return "floor"; */
        return "바닥";
    else
        /*KR return "ground"; */
        return "지면";
}

const char *
ceiling(x, y)
register int x, y;
{
    register struct rm *lev = &levl[x][y];
    const char *what;

    /* other room types will no longer exist when we're interested --
     * see check_special_room()
     */
    if (*in_rooms(x, y, VAULT))
        /*KR what = "vault's ceiling"; */
        what = "금고의 천장";
    else if (*in_rooms(x, y, TEMPLE))
        /*KR what = "temple's ceiling"; */
        what = "사원의 천장";
    else if (*in_rooms(x, y, SHOPBASE))
        /*KR what = "shop's ceiling"; */
        what = "상점의 천장";
    else if (Is_waterlevel(&u.uz))
        /* water plane has no surface; its air bubbles aren't below sky */
        /*KR what = "water above"; */
        what = "물 위";
    else if (IS_AIR(lev->typ))
        /*KR what = "sky"; */
        what = "하늘";
    else if (Underwater)
        /*KR what = "water's surface"; */
        what = "수면";
    else if ((IS_ROOM(lev->typ) && !Is_earthlevel(&u.uz))
             || IS_WALL(lev->typ) || IS_DOOR(lev->typ) || lev->typ == SDOOR)
        /*KR what = "ceiling"; */
        what = "천장";
    else
        /*KR what = "rock cavern"; */
        what = "동굴의 천장";

    return what;
}

struct engr *
engr_at(x, y)
xchar x, y;
{
    register struct engr *ep = head_engr;

    while (ep) {
        if (x == ep->engr_x && y == ep->engr_y)
            return ep;
        ep = ep->nxt_engr;
    }
    return (struct engr *) 0;
}

/* Decide whether a particular string is engraved at a specified
 * location; a case-insensitive substring match is used.
 * Ignore headstones, in case the player names herself "Elbereth".
 *
 * If strict checking is requested, the word is only considered to be
 * present if it is intact and is the entire content of the engraving.
 */
int
sengr_at(s, x, y, strict)
const char *s;
xchar x, y;
boolean strict;
{
    register struct engr *ep = engr_at(x, y);

    if (ep && ep->engr_type != HEADSTONE && ep->engr_time <= moves) {
        return strict ? (fuzzymatch(ep->engr_txt, s, "", TRUE))
                      : (strstri(ep->engr_txt, s) != 0);
    }

    return FALSE;
}

void
u_wipe_engr(cnt)
int cnt;
{
    if (can_reach_floor(TRUE))
        wipe_engr_at(u.ux, u.uy, cnt, FALSE);
}

void
wipe_engr_at(x, y, cnt, magical)
xchar x, y, cnt;
boolean magical;
{
    register struct engr *ep = engr_at(x, y);

    /* Headstones are indelible */
    if (ep && ep->engr_type != HEADSTONE) {
        debugpline1("asked to erode %d characters", cnt);
        if (ep->engr_type != BURN || is_ice(x, y) || (magical && !rn2(2))) {
            if (ep->engr_type != DUST && ep->engr_type != ENGR_BLOOD) {
                cnt = rn2(1 + 50 / (cnt + 1)) ? 0 : 1;
                debugpline1("actually eroding %d characters", cnt);
            }
            wipeout_text(ep->engr_txt, (int) cnt, 0);
            while (ep->engr_txt[0] == ' ')
                ep->engr_txt++;
            if (!ep->engr_txt[0])
                del_engr(ep);
        }
    }
}

void
read_engr_at(x, y)
int x, y;
{
    register struct engr *ep = engr_at(x, y);
    int sensed = 0;

    /* Sensing an engraving does not require sight,
     * nor does it necessarily imply comprehension (literacy).
     */
    if (ep && ep->engr_txt[0]) {
        switch (ep->engr_type) {
        case DUST:
            if (!Blind) {
                sensed = 1;
#if 0 /*KR:T*/
                pline("%s is written here in the %s.", Something,
                      is_ice(x, y) ? "frost" : "dust");
#else
                pline("무언가가 %s에 쓰여 있다.",
                    is_ice(x, y) ? "서리" : "먼지");
#endif
            }
            break;
        case ENGRAVE:
        case HEADSTONE:
            if (!Blind || can_reach_floor(TRUE)) {
                sensed = 1;
                /*KR pline("%s is engraved here on the %s.", Something, */
                pline("무언가가 %s에 새겨져 있다.",
                      surface(x, y));
            }
            break;
        case BURN:
            if (!Blind || can_reach_floor(TRUE)) {
                sensed = 1;
#if 0 /*KR:T*/
                pline("Some text has been %s into the %s here.",
                      is_ice(x, y) ? "melted" : "burned", surface(x, y));
#else
                pline("어떤 글이 %s %s 있다.",
                    surface(x, y), is_ice(x, y) ? "에 새겨져" : "에 눌어붙어");
#endif
            }
            break;
        case MARK:
            if (!Blind) {
                sensed = 1;
                /*KR pline("There's some graffiti on the %s here.", surface(x, y)); */
                pline("여기 %s에 그래피티가 있다.", surface(x, y));
            }
            break;
        case ENGR_BLOOD:
            /* "It's a message!  Scrawled in blood!"
             * "What's it say?"
             * "It says... `See you next Wednesday.'" -- Thriller
             */
            if (!Blind) {
                sensed = 1;
                /*KR You_see("a message scrawled in blood here."); */
                You("여기에 글자가 피로 휘갈겨져 있다.");
            }
            break;
        default:
            impossible("%s is written in a very strange way.", Something);
            sensed = 1;
        }

        if (sensed) {
            char *et, buf[BUFSZ];
            int maxelen = (int) (sizeof buf
                                 /* sizeof "literal" counts terminating \0 */
                            /*KR - sizeof "You feel the words: \"\"."); */
                                 - sizeof "당신은 다음의 단어를 느낀다: \"\".");

            if ((int) strlen(ep->engr_txt) > maxelen) {
                (void) strncpy(buf, ep->engr_txt, maxelen);
                buf[maxelen] = '\0';
                et = buf;
            } else {
                et = ep->engr_txt;
            }
            /*KR You("%s: \"%s\".", (Blind) ? "feel the words" : "read", et); */
            You("%s: \"%s\".", (Blind) ? "다음의 단어를 느낀다" : "읽었다", et);
            if (context.run > 0)
                nomul(0);
        }
    }
}

void
make_engr_at(x, y, s, e_time, e_type)
int x, y;
const char *s;
long e_time;
xchar e_type;
{
    struct engr *ep;
    unsigned smem = strlen(s) + 1;

    if ((ep = engr_at(x, y)) != 0)
        del_engr(ep);
    ep = newengr(smem);
    (void) memset((genericptr_t)ep, 0, smem + sizeof(struct engr));
    ep->nxt_engr = head_engr;
    head_engr = ep;
    ep->engr_x = x;
    ep->engr_y = y;
    ep->engr_txt = (char *) (ep + 1);
    Strcpy(ep->engr_txt, s);
    /* engraving Elbereth shows wisdom */
    if (!in_mklev && !strcmp(s, "Elbereth"))
        exercise(A_WIS, TRUE);
    ep->engr_time = e_time;
    ep->engr_type = e_type > 0 ? e_type : rnd(N_ENGRAVE - 1);
    ep->engr_lth = smem;
}

/* delete any engraving at location <x,y> */
void
del_engr_at(x, y)
int x, y;
{
    register struct engr *ep = engr_at(x, y);

    if (ep)
        del_engr(ep);
}

/*
 * freehand - returns true if player has a free hand
 */
int
freehand()
{
    return (!uwep || !welded(uwep)
            || (!bimanual(uwep) && (!uarms || !uarms->cursed)));
}

static NEARDATA const char styluses[] = { ALL_CLASSES, ALLOW_NONE,
                                          TOOL_CLASS,  WEAPON_CLASS,
                                          WAND_CLASS,  GEM_CLASS,
                                          RING_CLASS,  0 };

/* Mohs' Hardness Scale:
 *  1 - Talc             6 - Orthoclase
 *  2 - Gypsum           7 - Quartz
 *  3 - Calcite          8 - Topaz
 *  4 - Fluorite         9 - Corundum
 *  5 - Apatite         10 - Diamond
 *
 * Since granite is an igneous rock hardness ~ 7, anything >= 8 should
 * probably be able to scratch the rock.
 * Devaluation of less hard gems is not easily possible because obj struct
 * does not contain individual oc_cost currently. 7/91
 *
 * steel      - 5-8.5   (usu. weapon)
 * diamond    - 10                      * jade       -  5-6      (nephrite)
 * ruby       -  9      (corundum)      * turquoise  -  5-6
 * sapphire   -  9      (corundum)      * opal       -  5-6
 * topaz      -  8                      * glass      - ~5.5
 * emerald    -  7.5-8  (beryl)         * dilithium  -  4-5??
 * aquamarine -  7.5-8  (beryl)         * iron       -  4-5
 * garnet     -  7.25   (var. 6.5-8)    * fluorite   -  4
 * agate      -  7      (quartz)        * brass      -  3-4
 * amethyst   -  7      (quartz)        * gold       -  2.5-3
 * jasper     -  7      (quartz)        * silver     -  2.5-3
 * onyx       -  7      (quartz)        * copper     -  2.5-3
 * moonstone  -  6      (orthoclase)    * amber      -  2-2.5
 */

/* return 1 if action took 1 (or more) moves, 0 if error or aborted */
int
doengrave()
{
    boolean dengr = FALSE;    /* TRUE if we wipe out the current engraving */
    boolean doblind = FALSE;  /* TRUE if engraving blinds the player */
    boolean doknown = FALSE;  /* TRUE if we identify the stylus */
    boolean eow = FALSE;      /* TRUE if we are overwriting oep */
    boolean jello = FALSE;    /* TRUE if we are engraving in slime */
    boolean ptext = TRUE;     /* TRUE if we must prompt for engrave text */
    boolean teleengr = FALSE; /* TRUE if we move the old engraving */
    boolean zapwand = FALSE;  /* TRUE if we remove a wand charge */
    xchar type = DUST;        /* Type of engraving made */
    xchar oetype = 0;         /* will be set to type of current engraving */
    char buf[BUFSZ];          /* Buffer for final/poly engraving text */
    char ebuf[BUFSZ];         /* Buffer for initial engraving text */
    char fbuf[BUFSZ];         /* Buffer for "your fingers" */
    char qbuf[QBUFSZ];        /* Buffer for query text */
    char post_engr_text[BUFSZ]; /* Text displayed after engraving prompt */
    const char *everb;          /* Present tense of engraving type */
    const char *eloc; /* Where the engraving is (ie dust/floor/...) */
    char *sp;         /* Place holder for space count of engr text */
    int len;          /* # of nonspace chars of new engraving text */
    int maxelen;      /* Max allowable length of engraving text */
    struct engr *oep = engr_at(u.ux, u.uy);
    /* The current engraving */
    struct obj *otmp; /* Object selected with which to engrave */
    char *writer;

    multi = 0;              /* moves consumed */
    nomovemsg = (char *) 0; /* occupation end message */

    buf[0] = (char) 0;
    ebuf[0] = (char) 0;
    post_engr_text[0] = (char) 0;
    maxelen = BUFSZ - 1;
    if (oep)
        oetype = oep->engr_type;
    if (is_demon(youmonst.data) || youmonst.data->mlet == S_VAMPIRE)
        type = ENGR_BLOOD;

    /* Can the adventurer engrave at all? */

    if (u.uswallow) {
        if (is_animal(u.ustuck->data)) {
            /*KR pline("What would you write?  \"Jonah was here\"?"); */
            pline("뭐라고 쓰고 싶은 거야? \"요나가 여기 있었다\"?");
            return 0;
        } else if (is_whirly(u.ustuck->data)) {
            cant_reach_floor(u.ux, u.uy, FALSE, FALSE);
            return 0;
        } else
            jello = TRUE;
    } else if (is_lava(u.ux, u.uy)) {
        /*KR You_cant("write on the %s!", surface(u.ux, u.uy)); */
        You("%s에 쓸 수 없다!", surface(u.ux, u.uy));
        return 0;
    } else if (is_pool(u.ux, u.uy) || IS_FOUNTAIN(levl[u.ux][u.uy].typ)) {
        /*KR You_cant("write on the %s!", surface(u.ux, u.uy)); */
        You("%s에 쓸 수 없다!", surface(u.ux, u.uy));
        return 0;
    }
    if (Is_airlevel(&u.uz) || Is_waterlevel(&u.uz) /* in bubble */) {
        /*KR You_cant("write in thin air!"); */
        You("공중에 쓸 수 없다!");
        return 0;
    } else if (!accessible(u.ux, u.uy)) {
        /* stone, tree, wall, secret corridor, pool, lava, bars */
        /*KR You_cant("write here."); */
        You_cant("이곳에는 쓸 수 없다.");
        return 0;
    }
    if (cantwield(youmonst.data)) {
        /*KR You_cant("even hold anything!"); */
        You("무언가를 쥐고 있을 수조차 없다!");
        return 0;
    }
    if (check_capacity((char *) 0))
        return 0;

    /* One may write with finger, or weapon, or wand, or..., or...
     * Edited by GAN 10/20/86 so as not to change weapon wielded.
     */

    otmp = getobj(styluses, "write with");
    if (!otmp) /* otmp == zeroobj if fingers */
        return 0;

    if (otmp == &zeroobj) {
        /*KR Strcat(strcpy(fbuf, "your "), body_part(FINGERTIP)); */
        Strcat(strcpy(fbuf, "당신의 "), body_part(FINGERTIP));
        writer = fbuf;
    } else
        writer = yname(otmp);

    /* There's no reason you should be able to write with a wand
     * while both your hands are tied up.
     */
    if (!freehand() && otmp != uwep && !otmp->owornmask) {
        /*KR You("have no free %s to write with!", body_part(HAND)); */
        You("자유롭게 쓸 빈 %s이 없다!", body_part(HAND));
        return 0;
    }

    if (jello) {
        /*KR You("tickle %s with %s.", mon_nam(u.ustuck), writer); 
        Your("message dissolves..."); */
        You("%s(으)로 %s을/를 간지럽혔다.", writer, mon_nam(u.ustuck));
        Your("메시지가 녹아내렸다...");
        return 0;
    }
    if (otmp->oclass != WAND_CLASS && !can_reach_floor(TRUE)) {
        cant_reach_floor(u.ux, u.uy, FALSE, TRUE);
        return 0;
    }
    if (IS_ALTAR(levl[u.ux][u.uy].typ)) {
        /*KR You("make a motion towards the altar with %s.", writer); */
        You("%s을/를 사용해서 제단에 쓰려고 했다.", writer);
        altar_wrath(u.ux, u.uy);
        return 0;
    }
    if (IS_GRAVE(levl[u.ux][u.uy].typ)) {
        if (otmp == &zeroobj) { /* using only finger */
            /*KR You("would only make a small smudge on the %s.", */
            You("%s에 작고 더러운 자국을 남길 뿐이었다.",
                surface(u.ux, u.uy));
            return 0;
        } else if (!levl[u.ux][u.uy].disturbed) {
            /*KR You("disturb the undead!"); */
            You("망자의 영면을 방해했다!");
            levl[u.ux][u.uy].disturbed = 1;
            (void) makemon(&mons[PM_GHOUL], u.ux, u.uy, NO_MM_FLAGS);
            exercise(A_WIS, FALSE);
            return 1;
        }
    }

    /* SPFX for items */

    switch (otmp->oclass) {
    default:
    case AMULET_CLASS:
    case CHAIN_CLASS:
    case POTION_CLASS:
    case COIN_CLASS:
        break;
    case RING_CLASS:
        /* "diamond" rings and others should work */
    case GEM_CLASS:
        /* diamonds & other hard gems should work */
        if (objects[otmp->otyp].oc_tough) {
            type = ENGRAVE;
            break;
        }
        break;
    case ARMOR_CLASS:
        if (is_boots(otmp)) {
            type = DUST;
            break;
        }
        /*FALLTHRU*/
    /* Objects too large to engrave with */
    case BALL_CLASS:
    case ROCK_CLASS:
        /*KR You_cant("engrave with such a large object!"); */
        pline("그렇게 큰 물체로 새길 수는 없다!");
        ptext = FALSE;
        break;
    /* Objects too silly to engrave with */
    case FOOD_CLASS:
    case SCROLL_CLASS:
    case SPBOOK_CLASS:
#if 0 /*KR:T*/
        pline("%s would get %s.", Yname2(otmp),
            is_ice(u.ux, u.uy) ? "all frosty" : "too dirty");
#else
        Your("%s은/는 %s.", xname(otmp),
            is_ice(u.ux, u.uy) ? "서리로 뒤덮였다" : "더러워졌다");
#endif
        ptext = FALSE;
        break;
    case RANDOM_CLASS: /* This should mean fingers */
        break;

    /* The charge is removed from the wand before prompting for
     * the engraving text, because all kinds of setup decisions
     * and pre-engraving messages are based upon knowing what type
     * of engraving the wand is going to do.  Also, the player
     * will have potentially seen "You wrest .." message, and
     * therefore will know they are using a charge.
     */
    case WAND_CLASS:
        if (zappable(otmp)) {
            check_unpaid(otmp);
            if (otmp->cursed && !rn2(WAND_BACKFIRE_CHANCE)) {
                wand_explode(otmp, 0);
                return 1;
            }
            zapwand = TRUE;
            if (!can_reach_floor(TRUE))
                ptext = FALSE;

            switch (otmp->otyp) {
            /* DUST wands */
            default:
                break;
            /* NODIR wands */
            case WAN_LIGHT:
            case WAN_SECRET_DOOR_DETECTION:
            case WAN_CREATE_MONSTER:
            case WAN_WISHING:
            case WAN_ENLIGHTENMENT:
                zapnodir(otmp);
                break;
            /* IMMEDIATE wands */
            /* If wand is "IMMEDIATE", remember to affect the
             * previous engraving even if turning to dust.
             */
            case WAN_STRIKING:
                Strcpy(post_engr_text,
                    /*KR "The wand unsuccessfully fights your attempt to write!"); */
                    "지팡이가 당신의 쓰려는 시도에 저항을 실패했다!");
                break;
            case WAN_SLOW_MONSTER:
                if (!Blind) {
                    /*KR Sprintf(post_engr_text, "The bugs on the %s slow down!", */
                    Sprintf(post_engr_text, "%s 위의 벌레들이 느려진다!",
                            surface(u.ux, u.uy));
                }
                break;
            case WAN_SPEED_MONSTER:
                if (!Blind) {
                    /*KR Sprintf(post_engr_text, "The bugs on the %s speed up!", */
                    Sprintf(post_engr_text, "%s 위의 벌레들이 빨라진다!",
                            surface(u.ux, u.uy));
                }
                break;
            case WAN_POLYMORPH:
                if (oep) {
                    if (!Blind) {
                        type = (xchar) 0; /* random */
                        (void) random_engraving(buf);
                    } else {
                        /* keep the same type so that feels don't
                           change and only the text is altered,
                           but you won't know anyway because
                           you're a _blind writer_ */
                        if (oetype)
                            type = oetype;
                        xcrypt(blengr(), buf);
                    }
                    dengr = TRUE;
                }
                break;
            case WAN_NOTHING:
            case WAN_UNDEAD_TURNING:
            case WAN_OPENING:
            case WAN_LOCKING:
            case WAN_PROBING:
                break;
            /* RAY wands */
            case WAN_MAGIC_MISSILE:
                ptext = TRUE;
                if (!Blind) {
                    Sprintf(post_engr_text,
                        /*KR "The %s is riddled by bullet holes!", */
                            "%s이 총알 구멍으로 벌집이 되었다!",
                            surface(u.ux, u.uy));
                }
                break;
            /* can't tell sleep from death - Eric Backus */
            case WAN_SLEEP:
            case WAN_DEATH:
                if (!Blind) {
                    /*KR Sprintf(post_engr_text, "The bugs on the %s stop moving!", */
                    Sprintf(post_engr_text, "%s 위의 벌레들이 움직임을 멈췄다!",
                            surface(u.ux, u.uy));
                }
                break;
            case WAN_COLD:
                if (!Blind)
                    Strcpy(post_engr_text,
                        /*KR "A few ice cubes drop from the wand."); */
                           "지팡이에서 각얼음이 조금 떨어진다.");
                if (!oep || (oep->engr_type != BURN))
                    break;
                /*FALLTHRU*/
            case WAN_CANCELLATION:
            case WAN_MAKE_INVISIBLE:
                if (oep && oep->engr_type != HEADSTONE) {
                    if (!Blind)
                        /*KR pline_The("engraving on the %s vanishes!", */
                        pline("%s 위의 글자가 사라진다!",
                                  surface(u.ux, u.uy));
                    dengr = TRUE;
                }
                break;
            case WAN_TELEPORTATION:
                if (oep && oep->engr_type != HEADSTONE) {
                    if (!Blind)
                        /*KR pline_The("engraving on the %s vanishes!", */
                        pline("%s 위의 글자가 사라진다!",
                                  surface(u.ux, u.uy));
                    teleengr = TRUE;
                }
                break;
            /* type = ENGRAVE wands */
            case WAN_DIGGING:
                ptext = TRUE;
                type = ENGRAVE;
                if (!objects[otmp->otyp].oc_name_known) {
                    if (flags.verbose)
                        /*KR pline("This %s is a wand of digging!", xname(otmp)); */
                        pline("이것은 굴삭의 지팡이다!");
                    doknown = TRUE;
                }
#if 0 /*KR:T*/
                Strcpy(post_engr_text,
                       (Blind && !Deaf)
                          ? "You hear drilling!"    /* Deaf-aware */
                          : Blind
                             ? "You feel tremors."
                             : IS_GRAVE(levl[u.ux][u.uy].typ)
                                 ? "Chips fly out from the headstone."
                                 : is_ice(u.ux, u.uy)
                                    ? "Ice chips fly up from the ice surface!"
                                    : (level.locations[u.ux][u.uy].typ
                                       == DRAWBRIDGE_DOWN)
                                       ? "Splinters fly up from the bridge."
                                       : "Gravel flies up from the floor.");
#else
                Strcpy(post_engr_text,
                       (Blind && !Deaf)
                          ? "구멍을 파는 소리가 들린다!"    /* Deaf-aware */
                          : Blind
                             ? "당신은 진동을 느꼈다."
                             : IS_GRAVE(levl[u.ux][u.uy].typ)
                                 ? "묘비에서 파편이 튀었다."
                                 : is_ice(u.ux, u.uy)
                                    ? "얼어붙은 표면에서 얼음 조각이 튀었다!"
                                    : (level.locations[u.ux][u.uy].typ
                                       == DRAWBRIDGE_DOWN)
                                       ? "도개교에서 파편이 튀었다."
                                       : "바닥에서 자갈이 튀었다.");
#endif
                break;
            /* type = BURN wands */
            case WAN_FIRE:
                ptext = TRUE;
                type = BURN;
                if (!objects[otmp->otyp].oc_name_known) {
                    if (flags.verbose)
                        /*KR pline("This %s is a wand of fire!", xname(otmp)); */
                        pline("이 %s는 화염의 지팡이다!", xname(otmp));
                    doknown = TRUE;
                }
           /*KR Strcpy(post_engr_text, Blind ? "You feel the wand heat up."
                                             : "Flames fly from the wand."); */
                Strcpy(post_engr_text, Blind ? "당신은 지팡이가 뜨거워진다고 느낀다."
                                             : "지팡이에서 불똥이 튀었다.");
                break;
            case WAN_LIGHTNING:
                ptext = TRUE;
                type = BURN;
                if (!objects[otmp->otyp].oc_name_known) {
                    if (flags.verbose)
                        /*KR pline("This %s is a wand of lightning!", xname(otmp)); */
                        pline("이 %s는 번개의 지팡이다!", xname(otmp));
                    doknown = TRUE;
                }
                if (!Blind) {
                    /*KR Strcpy(post_engr_text, "Lightning arcs from the wand."); */
                    Strcpy(post_engr_text, "지팡이에서 불꽃이 튀었다.");
                    doblind = TRUE;
                } else
                    Strcpy(post_engr_text, !Deaf
                        /*KR    ? "You hear crackling!"  /* Deaf-aware */
                           /*   : "Your hair stands up!"); */
                                ? "당신은 빠직빠직 소리를 듣는다!"  /* Deaf-aware */
                                : "당신의 머리가 곤두섰다!");
                break;

            /* type = MARK wands */
            /* type = ENGR_BLOOD wands */
            }
        } else { /* end if zappable */
            /* failing to wrest one last charge takes time */
            ptext = FALSE; /* use "early exit" below, return 1 */
            /* give feedback here if we won't be getting the
               "can't reach floor" message below */
            if (can_reach_floor(TRUE)) {
                /* cancelled wand turns to dust */
                if (otmp->spe < 0)
                    zapwand = TRUE;
                /* empty wand just doesn't write */
                else
                    /*KR pline_The("wand is too worn out to engrave."); */
                    pline_The("지팡이는 새기기에는 너무 낡았다.");
            }
        }
        break;

    case WEAPON_CLASS:
        if (is_blade(otmp)) {
            if ((int) otmp->spe > -3)
                type = ENGRAVE;
            else
                /*KR pline("%s too dull for engraving.", Yobjnam2(otmp, "are")); */
                pline("%s은/는 새기기에는 너무 무디다.", xname(otmp));
        }
        break;

    case TOOL_CLASS:
        if (otmp == ublindf) {
            pline(
                /*KR "That is a bit difficult to engrave with, don't you think?"); */
                "그걸로는 좀 새기기 어려울 것 같아. 그렇게 생각하지 않니?");
            return 0;
        }
        switch (otmp->otyp) {
        case MAGIC_MARKER:
            if (otmp->spe <= 0)
                /*KR Your("marker has dried out."); */
                Your("마커펜이 말라 버렸다.");
            else
                type = MARK;
            break;
        case TOWEL:
            /* Can't really engrave with a towel */
            ptext = FALSE;
            if (oep)
                if (oep->engr_type == DUST
                    || oep->engr_type == ENGR_BLOOD
                    || oep->engr_type == MARK) {
                    if (is_wet_towel(otmp))
                        dry_a_towel(otmp, -1, TRUE);
                    if (!Blind)
                        /*KR You("wipe out the message here."); */
                        You("여기 있던 메시지를 닦아냈다.");
                    else
#if 0 /*KR:T*/
                        pline("%s %s.", Yobjnam2(otmp, "get"),
                              is_ice(u.ux, u.uy) ? "frosty" : "dusty");
#else
                        pline("%s은/는 %s이/가 되었다.", xname(otmp),
                            is_ice(u.ux, u.uy) ? "서리 투성이" : "먼지 범벅");
#endif
                    dengr = TRUE;
                } else
                    /*KR pline("%s can't wipe out this engraving.", Yname2(otmp)); */
                    pline("이 글자는 %s(으)로는 닦아낼 수 없다.", xname(otmp));
            else
#if 0 /*KR:T*/
                pline("%s %s.", Yobjnam2(otmp, "get"),
                      is_ice(u.ux, u.uy) ? "frosty" : "dusty");
#else
                pline("%s은/는 %s이/가 되었다.", xname(otmp),
                    is_ice(u.ux, u.uy) ? "서리 투성이" : "먼지 범벅");
#endif
            break;
        default:
            break;
        }
        break;

    case VENOM_CLASS:
        if (wizard) {
            /*KR pline("Writing a poison pen letter??"); */
            pline("호오. 이거야말로 진짜 '독이 되는 말'이지.");
            break;
        }
        /*FALLTHRU*/
    case ILLOBJ_CLASS:
        impossible("You're engraving with an illegal object!");
        break;
    }

    if (IS_GRAVE(levl[u.ux][u.uy].typ)) {
        if (type == ENGRAVE || type == 0) {
            type = HEADSTONE;
        } else {
            /* ensures the "cannot wipe out" case */
            type = DUST;
            dengr = FALSE;
            teleengr = FALSE;
            buf[0] = '\0';
        }
    }

    /*
     * End of implement setup
     */

    /* Identify stylus */
    if (doknown) {
        learnwand(otmp);
        if (objects[otmp->otyp].oc_name_known)
            more_experienced(0, 10);
    }
    if (teleengr) {
        rloc_engr(oep);
        oep = (struct engr *) 0;
    }
    if (dengr) {
        del_engr(oep);
        oep = (struct engr *) 0;
    }
    /* Something has changed the engraving here */
    if (*buf) {
        make_engr_at(u.ux, u.uy, buf, moves, type);
        if (!Blind)
            /*KR pline_The("engraving now reads: \"%s\".", buf); */
            pline("새긴 글자는 이제 이렇게 읽힌다: \"%s\".", buf);
        ptext = FALSE;
    }
    if (zapwand && (otmp->spe < 0)) {
#if 0 /*KR:T*/
        pline("%s %sturns to dust.", The(xname(otmp)),
              Blind ? "" : "glows violently, then ");
#else
        pline("%s이/가 %s가루로 변했다.", The(xname(otmp)),
            Blind ? "" : "막 빛나더니, ");
#endif
        if (!IS_GRAVE(levl[u.ux][u.uy].typ))
#if 0 /*KR:T*/
            You(
    "are not going to get anywhere trying to write in the %s with your dust.",
                is_ice(u.ux, u.uy) ? "frost" : "dust");
#else
            You(
                "먼지로 %s에 무언가 쓰려고 했지만, 그럴 수 없었다.",
                is_ice(u.ux, u.uy) ? "서리" : "먼지");
#endif
        useup(otmp);
        otmp = 0; /* wand is now gone */
        ptext = FALSE;
    }
    /* Early exit for some implements. */
    if (!ptext) {
        if (otmp && otmp->oclass == WAND_CLASS && !can_reach_floor(TRUE))
            cant_reach_floor(u.ux, u.uy, FALSE, TRUE);
        return 1;
    }
    /*
     * Special effects should have deleted the current engraving (if
     * possible) by now.
     */
    if (oep) {
        register char c = 'n';

        /* Give player the choice to add to engraving. */
        if (type == HEADSTONE) {
            /* no choice, only append */
            c = 'y';
        } else if (type == oep->engr_type
                   && (!Blind || oep->engr_type == BURN
                       || oep->engr_type == ENGRAVE)) {
            /*KR c = yn_function("Do you want to add to the current engraving?", */
            c = yn_function("현재 새겨진 글자에 추가하시겠습니까?",
                            ynqchars, 'y');
            if (c == 'q') {
                pline1(Never_mind);
                return 0;
            }
        }

        if (c == 'n' || Blind) {
            if (oep->engr_type == DUST
                || oep->engr_type == ENGR_BLOOD
                || oep->engr_type == MARK) {
                if (!Blind) {
#if 0 /*KR:T*/
                    You("wipe out the message that was %s here.",
                        (oep->engr_type == DUST)
                            ? "written in the dust"
                            : (oep->engr_type == ENGR_BLOOD)
                                ? "scrawled in blood"
                                : "written");
#else
                    You("%s메시지를 지워버렸다.",
                        (oep->engr_type == DUST)
                        ? "먼지에 쓰인 "
                        : (oep->engr_type == ENGR_BLOOD)
                        ? "피로 휘갈겨진 "
                        : "쓰여져 있는 ");
#endif
                    del_engr(oep);
                    oep = (struct engr *) 0;
                } else
                    /* Don't delete engr until after we *know* we're engraving
                     */
                    eow = TRUE;
            } else if (type == DUST || type == MARK || type == ENGR_BLOOD) {
#if 0 /*KR:T*/
                You("cannot wipe out the message that is %s the %s here.",
                    oep->engr_type == BURN
                        ? (is_ice(u.ux, u.uy) ? "melted into" : "burned into")
                        : "engraved in",
                    surface(u.ux, u.uy));
#else
                You("%s에 %s 메시지를 지울 수는 없다.",
                    surface(u.ux, u.uy),
                    oep->engr_type == BURN
                    ? (is_ice(u.ux, u.uy) ? "녹아 있는" : "눌어붙어 있는")
                    : "새겨진");
#endif
                return 1;
            } else if (type != oep->engr_type || c == 'n') {
                if (!Blind || can_reach_floor(TRUE))
                    /*KR You("will overwrite the current message."); */
                    You("메시지를 덮어쓰려고 했다.");
                eow = TRUE;
            }
        }
    }

    eloc = surface(u.ux, u.uy);
    switch (type) {
    default:
   /*KR everb = (oep && !eow ? "add to the weird writing on"
                             : "write strangely on"); */
        everb = (oep && !eow ? "이상한 글자에 더했다"
                             : "이상한 글자를 썼다");
        break;
    case DUST:
   /*KR everb = (oep && !eow ? "add to the writing in" : "write in");
        eloc = is_ice(u.ux, u.uy) ? "frost" : "dust"; */
        everb = (oep && !eow ? "덧붙여 썼다" : "썼다");
        eloc = is_ice(u.ux, u.uy) ? "서리" : "먼지";
        break;
    case HEADSTONE:
   /*KR everb = (oep && !eow ? "add to the epitaph on" : "engrave on"); */
        everb = (oep && !eow ? "묘비문에 글자를 더했다" : "묘비문을 새겼다");
        break;
    case ENGRAVE:
        /*KR everb = (oep && !eow ? "add to the engraving in" : "engrave in"); */
        everb = (oep && !eow ? "글자를 덧붙여 썼다" : "새겼다");
        break;
    case BURN:
        everb = (oep && !eow
                /*KR ? (is_ice(u.ux, u.uy) ? "add to the text melted into"
                                           : "add to the text burned into")
                     : (is_ice(u.ux, u.uy) ? "melt into" : "burn into")); */
                     ? (is_ice(u.ux, u.uy) ? "녹은 글씨에 더했다"
                                           : "눌어붙은 글씨에 더했다")
                     : (is_ice(u.ux, u.uy) ? "녹였다" : "눌어붙게 헸다"));
        break;
    case MARK:
        /*KR everb = (oep && !eow ? "add to the graffiti on" : "scribble on"); */
        everb = (oep && !eow ? "그래피티에 덧붙여 썼다" : "휘갈겨 썼다");
        break;
    case ENGR_BLOOD:
        /*KR everb = (oep && !eow ? "add to the scrawl on" : "scrawl on"); */
        everb = (oep && !eow ? "덧붙여 휘갈겨 썼다" : "휘갈겨 썼다");
        break;
    }

    /* Tell adventurer what is going on */
    if (otmp != &zeroobj)
        /*KR You("%s the %s with %s.", everb, eloc, doname(otmp)); */
        /*KR You write on(everb) the floor(eloc) with your finger(doname(otmp)) */
        /*JP jpast(everb)를 사용. 수정필요*/
        You("%s으로 %s에 %s.", doname(otmp), eloc, everb);
    else
        /*KR You("%s the %s with your %s.", everb, eloc, body_part(FINGERTIP)); */
        /*KR You write on(everb) the floor(eloc) with your finger(body_part(FINGERTIP))*/
        You("%s으로 %s에 %s.", body_part(FINGERTIP), eloc, everb);

    /* Prompt for engraving! */
    /*KR Sprintf(qbuf, "What do you want to %s the %s here?", everb, eloc); */
    /*KR What do you want to write on(everb) the floor(eloc) here? */
    /*KR 여기 있는 %s(바닥)에 무엇을 %s? */
    Sprintf(qbuf, "당신은 %s에 %s. 뭐라고 쓰시겠습니까?", eloc, everb);
    getlin(qbuf, ebuf);
    /* convert tabs to spaces and condense consecutive spaces to one */
    mungspaces(ebuf);

    /* Count the actual # of chars engraved not including spaces */
    len = strlen(ebuf);
    for (sp = ebuf; *sp; sp++)
        if (*sp == ' ')
            len -= 1;

    if (len == 0 || index(ebuf, '\033')) {
        if (zapwand) {
            if (!Blind)
#if 0 /*KR:T*/
                pline("%s, then %s.", Tobjnam(otmp, "glow"),
                      otense(otmp, "fade"));
#else
                pline("%s이/가 빛났지만, 이내 사라졌다.", xname(otmp));
#endif
            return 1;
        } else {
            pline1(Never_mind);
            return 0;
        }
    }

    /* A single `x' is the traditional signature of an illiterate person */
    if (len != 1 || (!index(ebuf, 'x') && !index(ebuf, 'X')))
        u.uconduct.literate++;

    /* Mix up engraving if surface or state of mind is unsound.
       Note: this won't add or remove any spaces. */
    for (sp = ebuf; *sp; sp++) {
        if (*sp == ' ')
            continue;
        if (((type == DUST || type == ENGR_BLOOD) && !rn2(25))
            || (Blind && !rn2(11)) || (Confusion && !rn2(7))
            || (Stunned && !rn2(4)) || (Hallucination && !rn2(2)))
            /*JP 일본어에서는 jrndm_replace라는 것으로 랜덤화시킴. 수정필요*/
            *sp = ' ' + rnd(96 - 2); /* ASCII '!' thru '~'
                                        (excludes ' ' and DEL) */
    }

    /* Previous engraving is overwritten */
    if (eow) {
        del_engr(oep);
        oep = (struct engr *) 0;
    }

    /* Figure out how long it took to engrave, and if player has
     * engraved too much.
     */
    switch (type) {
    default:
        multi = -(len / 10);
        if (multi)
            /*KR nomovemsg = "You finish your weird engraving."; */
            nomovemsg = "You finish your weird engraving.";
        break;
    case DUST:
        multi = -(len / 10);
        if (multi)
            /*KR nomovemsg = "You finish writing in the dust."; */
            nomovemsg = "You finish writing in the dust.";
        break;
    case HEADSTONE:
    case ENGRAVE:
        multi = -(len / 10);
        if (otmp->oclass == WEAPON_CLASS
            && (otmp->otyp != ATHAME || otmp->cursed)) {
            multi = -len;
            maxelen = ((otmp->spe + 3) * 2) + 1;
            /* -2 => 3, -1 => 5, 0 => 7, +1 => 9, +2 => 11
             * Note: this does not allow a +0 anything (except an athame)
             * to engrave "Elbereth" all at once.
             * However, you can engrave "Elb", then "ere", then "th".
             */
            /*KR pline("%s dull.", Yobjnam2(otmp, "get")); */
            Your("%s이/가 무뎌졌다.", xname(otmp));
            costly_alteration(otmp, COST_DEGRD);
            if (len > maxelen) {
                multi = -maxelen;
                otmp->spe = -3;
            } else if (len > 1)
                otmp->spe -= len >> 1;
            else
                otmp->spe -= 1; /* Prevent infinite engraving */
        } else if (otmp->oclass == RING_CLASS || otmp->oclass == GEM_CLASS) {
            multi = -len;
        }
        if (multi)
            /*KR nomovemsg = "You finish engraving."; */
            nomovemsg = "당신은 새기기를 마쳤다.";
        break;
    case BURN:
        multi = -(len / 10);
        if (multi)
            nomovemsg = is_ice(u.ux, u.uy)
                     /*KR ? "You finish melting your message into the ice."
                          : "You finish burning your message into the floor."; */
                          ? "당신은 얼음에 메시지 녹이기를 마쳤다."
                          : "당신은 바닥에 메시지 눌어붙게 만들기를 마쳤다.";
        break;
    case MARK:
        multi = -(len / 10);
        if (otmp->otyp == MAGIC_MARKER) {
            maxelen = otmp->spe * 2; /* one charge / 2 letters */
            if (len > maxelen) {
                /*KR Your("marker dries out."); */
                Your("마커펜이 말라 버렸다.");
                otmp->spe = 0;
                multi = -(maxelen / 10);
            } else if (len > 1)
                otmp->spe -= len >> 1;
            else
                otmp->spe -= 1; /* Prevent infinite graffiti */
        }
        if (multi)
            /*KR nomovemsg = "You finish defacing the dungeon."; */
            nomovemsg = "당신은 던전 벽에 흠집 내기를 마쳤다.";
        break;
    case ENGR_BLOOD:
        multi = -(len / 10);
        if (multi)
            /*KR nomovemsg = "You finish scrawling."; */
            nomovemsg = "당신은 휘갈겨 쓰기를 마쳤다.";
        break;
    }

    /* Chop engraving down to size if necessary */
    if (len > maxelen) {
        for (sp = ebuf; maxelen && *sp; sp++)
            if (*sp == ' ')
                maxelen--;
        if (!maxelen && *sp) {
            /*JP 한자의 1바이트분 만이 남지 않도록 */
            /* is_kanji2 함수. 수정필요*/
            *sp = '\0';
            if (multi)
                /*KR nomovemsg = "You cannot write any more.";
            You("are only able to write \"%s\".", ebuf); */
                nomovemsg = "당신은 더 이상 쓸 수 없다.";
            You("\"%s\"(이)라고만 쓸 수 있었다.", ebuf);
        }
    }

    if (oep) /* add to existing engraving */
        Strcpy(buf, oep->engr_txt);
    (void) strncat(buf, ebuf, BUFSZ - (int) strlen(buf) - 1);
    /* Put the engraving onto the map */
    make_engr_at(u.ux, u.uy, buf, moves - multi, type);

    if (post_engr_text[0])
        pline("%s", post_engr_text);
    if (doblind && !resists_blnd(&youmonst)) {
        /*KR You("are blinded by the flash!"); */
        You("섬광으로 눈이 멀었다!");
        make_blinded((long) rnd(50), FALSE);
        if (!Blind)
            Your1(vision_clears);
    }
    return 1;
}

/* while loading bones, clean up text which might accidentally
   or maliciously disrupt player's terminal when displayed */
void
sanitize_engravings()
{
    struct engr *ep;

    for (ep = head_engr; ep; ep = ep->nxt_engr) {
        sanitize_name(ep->engr_txt);
    }
}

void
save_engravings(fd, mode)
int fd, mode;
{
    struct engr *ep, *ep2;
    unsigned no_more_engr = 0;

    for (ep = head_engr; ep; ep = ep2) {
        ep2 = ep->nxt_engr;
        if (ep->engr_lth && ep->engr_txt[0] && perform_bwrite(mode)) {
            bwrite(fd, (genericptr_t) &ep->engr_lth, sizeof ep->engr_lth);
            bwrite(fd, (genericptr_t) ep, sizeof (struct engr) + ep->engr_lth);
        }
        if (release_data(mode))
            dealloc_engr(ep);
    }
    if (perform_bwrite(mode))
        bwrite(fd, (genericptr_t) &no_more_engr, sizeof no_more_engr);
    if (release_data(mode))
        head_engr = 0;
}

void
rest_engravings(fd)
int fd;
{
    struct engr *ep;
    unsigned lth;

    head_engr = 0;
    while (1) {
        mread(fd, (genericptr_t) &lth, sizeof lth);
        if (lth == 0)
            return;
        ep = newengr(lth);
        mread(fd, (genericptr_t) ep, sizeof (struct engr) + lth);
        ep->nxt_engr = head_engr;
        head_engr = ep;
        ep->engr_txt = (char *) (ep + 1); /* Andreas Bormann */
        /* Mark as finished for bones levels -- no problem for
         * normal levels as the player must have finished engraving
         * to be able to move again.
         */
        ep->engr_time = moves;
    }
}

/* to support '#stats' wizard-mode command */
void
engr_stats(hdrfmt, hdrbuf, count, size)
const char *hdrfmt;
char *hdrbuf;
long *count, *size;
{
    struct engr *ep;

    Sprintf(hdrbuf, hdrfmt, (long) sizeof (struct engr));
    *count = *size = 0L;
    for (ep = head_engr; ep; ep = ep->nxt_engr) {
        ++*count;
        *size += (long) sizeof *ep + (long) ep->engr_lth;
    }
}

void
del_engr(ep)
register struct engr *ep;
{
    if (ep == head_engr) {
        head_engr = ep->nxt_engr;
    } else {
        register struct engr *ept;

        for (ept = head_engr; ept; ept = ept->nxt_engr)
            if (ept->nxt_engr == ep) {
                ept->nxt_engr = ep->nxt_engr;
                break;
            }
        if (!ept) {
            impossible("Error in del_engr?");
            return;
        }
    }
    dealloc_engr(ep);
}

/* randomly relocate an engraving */
void
rloc_engr(ep)
struct engr *ep;
{
    int tx, ty, tryct = 200;

    do {
        if (--tryct < 0)
            return;
        tx = rn1(COLNO - 3, 2);
        ty = rn2(ROWNO);
    } while (engr_at(tx, ty) || !goodpos(tx, ty, (struct monst *) 0, 0));

    ep->engr_x = tx;
    ep->engr_y = ty;
}

/* Create a headstone at the given location.
 * The caller is responsible for newsym(x, y).
 */
void
make_grave(x, y, str)
int x, y;
const char *str;
{
    char buf[BUFSZ];

    /* Can we put a grave here? */
    if ((levl[x][y].typ != ROOM && levl[x][y].typ != GRAVE) || t_at(x, y))
        return;
    /* Make the grave */
    levl[x][y].typ = GRAVE;
    /* Engrave the headstone */
    del_engr_at(x, y);
    if (!str)
        str = get_rnd_text(EPITAPHFILE, buf, rn2);
    make_engr_at(x, y, str, 0L, HEADSTONE);
    return;
}

static const char blind_writing[][21] = {
    {0x44, 0x66, 0x6d, 0x69, 0x62, 0x65, 0x22, 0x45, 0x7b, 0x71,
     0x65, 0x6d, 0x72, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    {0x51, 0x67, 0x60, 0x7a, 0x7f, 0x21, 0x40, 0x71, 0x6b, 0x71,
     0x6f, 0x67, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x49, 0x6d, 0x73, 0x69, 0x62, 0x65, 0x22, 0x4c, 0x61, 0x7c,
     0x6d, 0x67, 0x24, 0x42, 0x7f, 0x69, 0x6c, 0x77, 0x67, 0x7e, 0x00},
    {0x4b, 0x6d, 0x6c, 0x66, 0x30, 0x4c, 0x6b, 0x68, 0x7c, 0x7f,
     0x6f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x51, 0x67, 0x70, 0x7a, 0x7f, 0x6f, 0x67, 0x68, 0x64, 0x71,
     0x21, 0x4f, 0x6b, 0x6d, 0x7e, 0x72, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x4c, 0x63, 0x76, 0x61, 0x71, 0x21, 0x48, 0x6b, 0x7b, 0x75,
     0x67, 0x63, 0x24, 0x45, 0x65, 0x6b, 0x6b, 0x65, 0x00, 0x00, 0x00},
    {0x4c, 0x67, 0x68, 0x6b, 0x78, 0x68, 0x6d, 0x76, 0x7a, 0x75,
     0x21, 0x4f, 0x71, 0x7a, 0x75, 0x6f, 0x77, 0x00, 0x00, 0x00, 0x00},
    {0x44, 0x66, 0x6d, 0x7c, 0x78, 0x21, 0x50, 0x65, 0x66, 0x65,
     0x6c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x44, 0x66, 0x73, 0x69, 0x62, 0x65, 0x22, 0x56, 0x7d, 0x63,
     0x69, 0x76, 0x6b, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
};

STATIC_OVL const char *
blengr(VOID_ARGS)
{
    return blind_writing[rn2(SIZE(blind_writing))];
}

/*engrave.c*/
