/* NetHack 3.6	cmd.c	$NHDT-Date: 1575245052 2019/12/02 00:04:12 $  $NHDT-Branch: NetHack-3.6 $:$NHDT-Revision: 1.350 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2013. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "lev.h"
#include "func_tab.h"

/* Macros for meta and ctrl modifiers:
 *   M and C return the meta/ctrl code for the given character;
 *     e.g., (C('c') is ctrl-c
 */
#ifndef M
#ifndef NHSTDC
#define M(c) (0x80 | (c))
#else
#define M(c) ((c) - 128)
#endif /* NHSTDC */
#endif

#ifndef C
#define C(c) (0x1f & (c))
#endif

#define unctrl(c) ((c) <= C('z') ? (0x60 | (c)) : (c))
#define unmeta(c) (0x7f & (c))

#ifdef ALTMETA
STATIC_VAR boolean alt_esc = FALSE;
#endif

struct cmd Cmd = { 0 }; /* flag.h */

extern const char *hu_stat[];  /* hunger status from eat.c */
extern const char *enc_stat[]; /* encumbrance status from botl.c */

#ifdef UNIX
/*
 * Some systems may have getchar() return EOF for various reasons, and
 * we should not quit before seeing at least NR_OF_EOFS consecutive EOFs.
 */
#if defined(SYSV) || defined(DGUX) || defined(HPUX)
#define NR_OF_EOFS 20
#endif
#endif

#define CMD_TRAVEL (char) 0x90
#define CMD_CLICKLOOK (char) 0x8F

#ifdef DEBUG
extern int NDECL(wiz_debug_cmd_bury);
#endif

#ifdef DUMB /* stuff commented out in extern.h, but needed here */
extern int NDECL(doapply);            /**/
extern int NDECL(dorub);              /**/
extern int NDECL(dojump);             /**/
extern int NDECL(doextlist);          /**/
extern int NDECL(enter_explore_mode); /**/
extern int NDECL(dodrop);             /**/
extern int NDECL(doddrop);            /**/
extern int NDECL(dodown);             /**/
extern int NDECL(doup);               /**/
extern int NDECL(donull);             /**/
extern int NDECL(dowipe);             /**/
extern int NDECL(docallcnd);          /**/
extern int NDECL(dotakeoff);          /**/
extern int NDECL(doremring);          /**/
extern int NDECL(dowear);             /**/
extern int NDECL(doputon);            /**/
extern int NDECL(doddoremarm);        /**/
extern int NDECL(dokick);             /**/
extern int NDECL(dofire);             /**/
extern int NDECL(dothrow);            /**/
extern int NDECL(doeat);              /**/
extern int NDECL(done2);              /**/
extern int NDECL(vanquished);         /**/
extern int NDECL(doengrave);          /**/
extern int NDECL(dopickup);           /**/
extern int NDECL(ddoinv);             /**/
extern int NDECL(dotypeinv);          /**/
extern int NDECL(dolook);             /**/
extern int NDECL(doprgold);           /**/
extern int NDECL(doprwep);            /**/
extern int NDECL(doprarm);            /**/
extern int NDECL(doprring);           /**/
extern int NDECL(dopramulet);         /**/
extern int NDECL(doprtool);           /**/
extern int NDECL(dosuspend);          /**/
extern int NDECL(doforce);            /**/
extern int NDECL(doopen);             /**/
extern int NDECL(doclose);            /**/
extern int NDECL(dosh);               /**/
extern int NDECL(dodiscovered);       /**/
extern int NDECL(doclassdisco);       /**/
extern int NDECL(doset);              /**/
extern int NDECL(dotogglepickup);     /**/
extern int NDECL(dowhatis);           /**/
extern int NDECL(doquickwhatis);      /**/
extern int NDECL(dowhatdoes);         /**/
extern int NDECL(dohelp);             /**/
extern int NDECL(dohistory);          /**/
extern int NDECL(doloot);             /**/
extern int NDECL(dodrink);            /**/
extern int NDECL(dodip);              /**/
extern int NDECL(dosacrifice);        /**/
extern int NDECL(dopray);             /**/
extern int NDECL(dotip);              /**/
extern int NDECL(doturn);             /**/
extern int NDECL(doredraw);           /**/
extern int NDECL(doread);             /**/
extern int NDECL(dosave);             /**/
extern int NDECL(dosearch);           /**/
extern int NDECL(doidtrap);           /**/
extern int NDECL(dopay);              /**/
extern int NDECL(dosit);              /**/
extern int NDECL(dotalk);             /**/
extern int NDECL(docast);             /**/
extern int NDECL(dovspell);           /**/
extern int NDECL(dotelecmd);          /**/
extern int NDECL(dountrap);           /**/
extern int NDECL(doversion);          /**/
extern int NDECL(doextversion);       /**/
extern int NDECL(doswapweapon);       /**/
extern int NDECL(dowield);            /**/
extern int NDECL(dowieldquiver);      /**/
extern int NDECL(dozap);              /**/
extern int NDECL(doorganize);         /**/
#endif /* DUMB */

static int NDECL((*timed_occ_fn));

STATIC_PTR int NDECL(dosuspend_core);
STATIC_PTR int NDECL(dosh_core);
STATIC_PTR int NDECL(doherecmdmenu);
STATIC_PTR int NDECL(dotherecmdmenu);
STATIC_PTR int NDECL(doprev_message);
STATIC_PTR int NDECL(timed_occupation);
STATIC_PTR int NDECL(doextcmd);
STATIC_PTR int NDECL(dotravel);
STATIC_PTR int NDECL(doterrain);
STATIC_PTR int NDECL(wiz_wish);
STATIC_PTR int NDECL(wiz_identify);
STATIC_PTR int NDECL(wiz_intrinsic);
STATIC_PTR int NDECL(wiz_map);
STATIC_PTR int NDECL(wiz_makemap);
STATIC_PTR int NDECL(wiz_genesis);
STATIC_PTR int NDECL(wiz_where);
STATIC_PTR int NDECL(wiz_detect);
STATIC_PTR int NDECL(wiz_panic);
STATIC_PTR int NDECL(wiz_polyself);
STATIC_PTR int NDECL(wiz_level_tele);
STATIC_PTR int NDECL(wiz_level_change);
STATIC_PTR int NDECL(wiz_show_seenv);
STATIC_PTR int NDECL(wiz_show_vision);
STATIC_PTR int NDECL(wiz_smell);
STATIC_PTR int NDECL(wiz_show_wmodes);
STATIC_DCL void NDECL(wiz_map_levltyp);
STATIC_DCL void NDECL(wiz_levltyp_legend);
#if defined(__BORLANDC__) && !defined(_WIN32)
extern void FDECL(show_borlandc_stats, (winid));
#endif
#ifdef DEBUG_MIGRATING_MONS
STATIC_PTR int NDECL(wiz_migrate_mons);
#endif
STATIC_DCL int FDECL(size_monst, (struct monst *, BOOLEAN_P));
STATIC_DCL int FDECL(size_obj, (struct obj *));
STATIC_DCL void FDECL(count_obj, (struct obj *, long *, long *,
                                  BOOLEAN_P, BOOLEAN_P));
STATIC_DCL void FDECL(obj_chain, (winid, const char *, struct obj *,
                                  BOOLEAN_P, long *, long *));
STATIC_DCL void FDECL(mon_invent_chain, (winid, const char *, struct monst *,
                                         long *, long *));
STATIC_DCL void FDECL(mon_chain, (winid, const char *, struct monst *,
                                  BOOLEAN_P, long *, long *));
STATIC_DCL void FDECL(contained_stats, (winid, const char *, long *, long *));
STATIC_DCL void FDECL(misc_stats, (winid, long *, long *));
STATIC_PTR int NDECL(wiz_show_stats);
STATIC_DCL boolean FDECL(accept_menu_prefix, (int NDECL((*))));
STATIC_PTR int NDECL(wiz_rumor_check);
STATIC_PTR int NDECL(doattributes);

STATIC_DCL void FDECL(enlght_out, (const char *));
STATIC_DCL void FDECL(enlght_line, (const char *, const char *, const char *,
                                    const char *));
STATIC_DCL char *FDECL(enlght_combatinc, (const char *, int, int, char *));
STATIC_DCL void FDECL(enlght_halfdmg, (int, int));
STATIC_DCL boolean NDECL(walking_on_water);
STATIC_DCL boolean FDECL(cause_known, (int));
STATIC_DCL char *FDECL(attrval, (int, int, char *));
STATIC_DCL void FDECL(background_enlightenment, (int, int));
STATIC_DCL void FDECL(basics_enlightenment, (int, int));
STATIC_DCL void FDECL(characteristics_enlightenment, (int, int));
STATIC_DCL void FDECL(one_characteristic, (int, int, int));
STATIC_DCL void FDECL(status_enlightenment, (int, int));
STATIC_DCL void FDECL(attributes_enlightenment, (int, int));

STATIC_DCL void FDECL(add_herecmd_menuitem, (winid, int NDECL((*)),
                                             const char *));
STATIC_DCL char FDECL(here_cmd_menu, (BOOLEAN_P));
STATIC_DCL char FDECL(there_cmd_menu, (BOOLEAN_P, int, int));
STATIC_DCL char *NDECL(parse);
STATIC_DCL void FDECL(show_direction_keys, (winid, CHAR_P, BOOLEAN_P));
STATIC_DCL boolean FDECL(help_dir, (CHAR_P, int, const char *));

static const char *readchar_queue = "";
static coord clicklook_cc;
/* for rejecting attempts to use wizard mode commands */
/*KR static const char unavailcmd[] = "Unavailable command '%s'."; */
static const char unavailcmd[] = "'%s' 명렁어는 사용할 수 없습니다.";
/* for rejecting #if !SHELL, !SUSPEND */
/*KR static const char cmdnotavail[] = "'%s' command not available."; */
static const char cmdnotavail[] = "'%s' 명령어를 사용할 수 없습니다.";

STATIC_PTR int
doprev_message(VOID_ARGS)
{
    return nh_doprev_message();
}

/* Count down by decrementing multi */
STATIC_PTR int
timed_occupation(VOID_ARGS)
{
    (*timed_occ_fn)();
    if (multi > 0)
        multi--;
    return multi > 0;
}

/* If you have moved since initially setting some occupations, they
 * now shouldn't be able to restart.
 *
 * The basic rule is that if you are carrying it, you can continue
 * since it is with you.  If you are acting on something at a distance,
 * your orientation to it must have changed when you moved.
 *
 * The exception to this is taking off items, since they can be taken
 * off in a number of ways in the intervening time, screwing up ordering.
 *
 *      Currently:      Take off all armor.
 *                      Picking Locks / Forcing Chests.
 *                      Setting traps.
 */
void
reset_occupations()
{
    reset_remarm();
    reset_pick();
    reset_trapset();
}

/* If a time is given, use it to timeout this function, otherwise the
 * function times out by its own means.
 */
void
set_occupation(fn, txt, xtime)
int NDECL((*fn));
const char *txt;
int xtime;
{
    if (xtime) {
        occupation = timed_occupation;
        timed_occ_fn = fn;
    } else
        occupation = fn;
    occtxt = txt;
    occtime = 0;
    return;
}

STATIC_DCL char NDECL(popch);

/* Provide a means to redo the last command.  The flag `in_doagain' is set
 * to true while redoing the command.  This flag is tested in commands that
 * require additional input (like `throw' which requires a thing and a
 * direction), and the input prompt is not shown.  Also, while in_doagain is
 * TRUE, no keystrokes can be saved into the saveq.
 */
#define BSIZE 20
static char pushq[BSIZE], saveq[BSIZE];
static NEARDATA int phead, ptail, shead, stail;

STATIC_OVL char
popch()
{
    /* If occupied, return '\0', letting tgetch know a character should
     * be read from the keyboard.  If the character read is not the
     * ABORT character (as checked in pcmain.c), that character will be
     * pushed back on the pushq.
     */
    if (occupation)
        return '\0';
    if (in_doagain)
        return (char) ((shead != stail) ? saveq[stail++] : '\0');
    else
        return (char) ((phead != ptail) ? pushq[ptail++] : '\0');
}

char
pgetchar() /* courtesy of aeb@cwi.nl */
{
    register int ch;

    if (iflags.debug_fuzzer)
        return randomkey();
    if (!(ch = popch()))
        ch = nhgetch();
    return (char) ch;
}

/* A ch == 0 resets the pushq */
void
pushch(ch)
char ch;
{
    if (!ch)
        phead = ptail = 0;
    if (phead < BSIZE)
        pushq[phead++] = ch;
    return;
}

/* A ch == 0 resets the saveq.  Only save keystrokes when not
 * replaying a previous command.
 */
void
savech(ch)
char ch;
{
    if (!in_doagain) {
        if (!ch)
            phead = ptail = shead = stail = 0;
        else if (shead < BSIZE)
            saveq[shead++] = ch;
    }
    return;
}

/* here after # - now read a full-word command */
STATIC_PTR int
doextcmd(VOID_ARGS)
{
    int idx, retval;
    int NDECL((*func));

    /* keep repeating until we don't run help or quit */
    do {
        idx = get_ext_cmd();
        if (idx < 0)
            return 0; /* quit */

        func = extcmdlist[idx].ef_funct;
        if (!wizard && (extcmdlist[idx].flags & WIZMODECMD)) {
            /*KR You("can't do that."); */
            pline("그럴 수는 없습니다.");
            return 0;
        }
        if (iflags.menu_requested && !accept_menu_prefix(func)) {
#if 0 /*KR:T*/
            pline("'%s' prefix has no effect for the %s command.",
                  visctrl(Cmd.spkeys[NHKF_REQMENU]),
                  extcmdlist[idx].ef_txt);
#else
            pline("'%s' 접두사는 %s 명령어에 대해 유효하지 않습니다.",
                  visctrl(Cmd.spkeys[NHKF_REQMENU]), 
                  extcmdlist[idx].ef_txt);
#endif
            iflags.menu_requested = FALSE;
        }
        retval = (*func)();
    } while (func == doextlist);

    return retval;
}

/* here after #? - now list all full-word commands and provid
   some navigation capability through the long list */
int
doextlist(VOID_ARGS)
{
    register const struct ext_func_tab *efp;
    char buf[BUFSZ], searchbuf[BUFSZ], promptbuf[QBUFSZ];
    winid menuwin;
    anything any;
    menu_item *selected;
    int n, pass;
    int menumode = 0, menushown[2], onelist = 0;
    boolean redisplay = TRUE, search = FALSE;
#if 0 /*KR:T*/
    static const char *headings[] = { "Extended commands",
                                      "Debugging Extended Commands" };
#else
    static const char *headings[] = { "확장 명령어",
                                      "디버깅 확장 명령어" };
#endif

    searchbuf[0] = '\0';
    menuwin = create_nhwindow(NHW_MENU);

    while (redisplay) {
        redisplay = FALSE;
        any = zeroany;
        start_menu(menuwin);
#if 0 /*KR:T*/
        add_menu(menuwin, NO_GLYPH, &any, 0, 0, ATR_NONE,
                 "Extended Commands List", MENU_UNSELECTED);
#else
        add_menu(menuwin, NO_GLYPH, &any, 0, 0, ATR_NONE, 
                 "확장 명령어 목록", MENU_UNSELECTED);
#endif
        add_menu(menuwin, NO_GLYPH, &any, 0, 0, ATR_NONE,
                 "", MENU_UNSELECTED);

#if 0 /*JP:T*/
        Strcpy(buf, menumode ? "Show" : "Hide");
        Strcat(buf, " commands that don't autocomplete");
        if (!menumode)
            Strcat(buf, " (those not marked with [A])");
#else
        Strcpy(buf, "자동 완성되지 않는 명령어를");
        Strcat(buf, menumode ? " 표시"
                             : " 표시하지 않음(이것들은[A] 표시가 붙지 않음)");
#endif
        any.a_int = 1;
        add_menu(menuwin, NO_GLYPH, &any, 'a', 0, ATR_NONE, buf,
                 MENU_UNSELECTED);

        if (!*searchbuf) {
            any.a_int = 2;
            /* was 's', but then using ':' handling within the interface
               would only examine the two or three meta entries, not the
               actual list of extended commands shown via separator lines;
               having ':' as an explicit selector overrides the default
               menu behavior for it; we retain 's' as a group accelerator */
#if 0 /*JP:T*/
            add_menu(menuwin, NO_GLYPH, &any, ':', 's', ATR_NONE,
                     "Search extended commands", MENU_UNSELECTED);
#else
            add_menu(menuwin, NO_GLYPH, &any, ':', 's', ATR_NONE,
                     "확장 명령어를 검색한다", MENU_UNSELECTED);
#endif
        } else {
            /*KR Strcpy(buf, "Show all, clear search"); */
            Strcpy(buf, "전부 표시, 검색 초기화");
            if (strlen(buf) + strlen(searchbuf) + strlen(" (\"\")") < QBUFSZ)
                Sprintf(eos(buf), " (\"%s\")", searchbuf);
            any.a_int = 3;
            /* specifying ':' as a group accelerator here is mostly a
               statement of intent (we'd like to accept it as a synonym but
               also want to hide it from general menu use) because it won't
               work for interfaces which support ':' to search; use as a
               general menu command takes precedence over group accelerator */
            add_menu(menuwin, NO_GLYPH, &any, 's', ':', ATR_NONE,
                     buf, MENU_UNSELECTED);
        }
        if (wizard) {
            any.a_int = 4;
#if 0 /*KR:T*/
            add_menu(menuwin, NO_GLYPH, &any, 'z', 0, ATR_NONE,
                     onelist ? "Show debugging commands in separate section"
                     : "Show all alphabetically, including debugging commands",
                     MENU_UNSELECTED);
#else
            add_menu(menuwin, NO_GLYPH, &any, 'z', 0, ATR_NONE,
                     onelist ? "디버깅 명령어를 별도의 섹션에 표시"
                             : "디버깅 명령어를 포함한 모든 명령어를 알파벳 순으로 표시",
                     MENU_UNSELECTED);
#endif
        }
        any = zeroany;
        add_menu(menuwin, NO_GLYPH, &any, 0, 0, ATR_NONE,
                 "", MENU_UNSELECTED);
        menushown[0] = menushown[1] = 0;
        n = 0;
        for (pass = 0; pass <= 1; ++pass) {
            /* skip second pass if not in wizard mode or wizard mode
               commands are being integrated into a single list */
            if (pass == 1 && (onelist || !wizard))
                break;
            for (efp = extcmdlist; efp->ef_txt; efp++) {
                int wizc;

                if ((efp->flags & CMD_NOT_AVAILABLE) != 0)
                    continue;
                /* if hiding non-autocomplete commands, skip such */
                if (menumode == 1 && (efp->flags & AUTOCOMPLETE) == 0)
                    continue;
                /* if searching, skip this command if it doesn't match */
                if (*searchbuf
                    /* first try case-insensitive substring match */
                    && !strstri(efp->ef_txt, searchbuf)
                    && !strstri(efp->ef_desc, searchbuf)
                    /* wildcard support; most interfaces use case-insensitve
                       pmatch rather than regexp for menu searching */
                    && !pmatchi(searchbuf, efp->ef_txt)
                    && !pmatchi(searchbuf, efp->ef_desc))
                    continue;
                /* skip wizard mode commands if not in wizard mode;
                   when showing two sections, skip wizard mode commands
                   in pass==0 and skip other commands in pass==1 */
                wizc = (efp->flags & WIZMODECMD) != 0;
                if (wizc && !wizard)
                    continue;
                if (!onelist && pass != wizc)
                    continue;

                /* We're about to show an item, have we shown the menu yet?
                   Doing menu in inner loop like this on demand avoids a
                   heading with no subordinate entries on the search
                   results menu. */
                if (!menushown[pass]) {
                    Strcpy(buf, headings[pass]);
                    add_menu(menuwin, NO_GLYPH, &any, 0, 0,
                             iflags.menu_headings, buf, MENU_UNSELECTED);
                    menushown[pass] = 1;
                }
                Sprintf(buf, " %-14s %-3s %s",
                        efp->ef_txt,
                        (efp->flags & AUTOCOMPLETE) ? "[A]" : " ",
                        efp->ef_desc);
                add_menu(menuwin, NO_GLYPH, &any, 0, 0, ATR_NONE,
                         buf, MENU_UNSELECTED);
                ++n;
            }
            if (n)
                add_menu(menuwin, NO_GLYPH, &any, 0, 0, ATR_NONE,
                         "", MENU_UNSELECTED);
        }
        if (*searchbuf && !n)
#if 0 /*KR:T*/
            add_menu(menuwin, NO_GLYPH, &any, 0, 0, ATR_NONE,
                     "no matches", MENU_UNSELECTED);
#else
            add_menu(menuwin, NO_GLYPH, &any, 0, 0, ATR_NONE, 
                     "일치하는 항목 없음", MENU_UNSELECTED);
#endif

        end_menu(menuwin, (char *) 0);
        n = select_menu(menuwin, PICK_ONE, &selected);
        if (n > 0) {
            switch (selected[0].item.a_int) {
            case 1: /* 'a': toggle show/hide non-autocomplete */
                menumode = 1 - menumode;  /* toggle 0 -> 1, 1 -> 0 */
                redisplay = TRUE;
                break;
            case 2: /* ':' when not searching yet: enable search */
                search = TRUE;
                break;
            case 3: /* 's' when already searching: disable search */
                search = FALSE;
                searchbuf[0] = '\0';
                redisplay = TRUE;
                break;
            case 4: /* 'z': toggle showing wizard mode commands separately */
                search = FALSE;
                searchbuf[0] = '\0';
                onelist = 1 - onelist;  /* toggle 0 -> 1, 1 -> 0 */
                redisplay = TRUE;
                break;
            }
            free((genericptr_t) selected);
        } else {
            search = FALSE;
            searchbuf[0] = '\0';
        }
        if (search) {
#if 0 /*KR:T*/
            Strcpy(promptbuf, "Extended command list search phrase");
            Strcat(promptbuf, "?");
#else
            Strcpy(promptbuf, "확장 명령어 검색 문자열은?");
#endif
            getlin(promptbuf, searchbuf);
            (void) mungspaces(searchbuf);
            if (searchbuf[0] == '\033')
                searchbuf[0] = '\0';
            if (*searchbuf)
                redisplay = TRUE;
            search = FALSE;
        }
    }
    destroy_nhwindow(menuwin);
    return 0;
}

#if defined(TTY_GRAPHICS) || defined(CURSES_GRAPHICS)
#define MAX_EXT_CMD 200 /* Change if we ever have more ext cmds */

/*
 * This is currently used only by the tty interface and is
 * controlled via runtime option 'extmenu'.  (Most other interfaces
 * already use a menu all the time for extended commands.)
 *
 * ``# ?'' is counted towards the limit of the number of commands,
 * so we actually support MAX_EXT_CMD-1 "real" extended commands.
 *
 * Here after # - now show pick-list of possible commands.
 */
int
extcmd_via_menu()
{
    const struct ext_func_tab *efp;
    menu_item *pick_list = (menu_item *) 0;
    winid win;
    anything any;
    const struct ext_func_tab *choices[MAX_EXT_CMD + 1];
    char buf[BUFSZ];
    char cbuf[QBUFSZ], prompt[QBUFSZ], fmtstr[20];
    int i, n, nchoices, acount;
    int ret, len, biggest;
    int accelerator, prevaccelerator;
    int matchlevel = 0;
    boolean wastoolong, one_per_line;

    ret = 0;
    cbuf[0] = '\0';
    biggest = 0;
    while (!ret) {
        i = n = 0;
        any = zeroany;
        /* populate choices */
        for (efp = extcmdlist; efp->ef_txt; efp++) {
            if ((efp->flags & CMD_NOT_AVAILABLE)
                || !(efp->flags & AUTOCOMPLETE)
                || (!wizard && (efp->flags & WIZMODECMD)))
                continue;
            if (!matchlevel || !strncmp(efp->ef_txt, cbuf, matchlevel)) {
                choices[i] = efp;
                if ((len = (int) strlen(efp->ef_desc)) > biggest)
                    biggest = len;
                if (++i > MAX_EXT_CMD) {
#if (NH_DEVEL_STATUS != NH_STATUS_RELEASED)
                    impossible(
      "Exceeded %d extended commands in doextcmd() menu; 'extmenu' disabled.",
                               MAX_EXT_CMD);
#endif /* NH_DEVEL_STATUS != NH_STATUS_RELEASED */
                    iflags.extmenu = 0;
                    return -1;
                }
            }
        }
        choices[i] = (struct ext_func_tab *) 0;
        nchoices = i;
        /* if we're down to one, we have our selection so get out of here */
        if (nchoices  <= 1) {
            ret = (nchoices == 1) ? (int) (choices[0] - extcmdlist) : -1;
            break;
        }

        /* otherwise... */
        win = create_nhwindow(NHW_MENU);
        start_menu(win);
        Sprintf(fmtstr, "%%-%ds", biggest + 15);
        prompt[0] = '\0';
        wastoolong = FALSE; /* True => had to wrap due to line width
                             * ('w' in wizard mode) */
        /* -3: two line menu header, 1 line menu footer (for prompt) */
        one_per_line = (nchoices < ROWNO - 3);
        accelerator = prevaccelerator = 0;
        acount = 0;
        for (i = 0; choices[i]; ++i) {
            accelerator = choices[i]->ef_txt[matchlevel];
            if (accelerator != prevaccelerator || one_per_line)
                wastoolong = FALSE;
            if (accelerator != prevaccelerator || one_per_line
                || (acount >= 2
                    /* +4: + sizeof " or " - sizeof "" */
                    && (strlen(prompt) + 4 + strlen(choices[i]->ef_txt)
                        /* -6: enough room for 1 space left margin
                         *   + "%c - " menu selector + 1 space right margin */
                        >= min(sizeof prompt, COLNO - 6)))) {
                if (acount) {
                    /* flush extended cmds for that letter already in buf */
                    Sprintf(buf, fmtstr, prompt);
                    any.a_char = prevaccelerator;
                    add_menu(win, NO_GLYPH, &any, any.a_char, 0, ATR_NONE,
                             buf, FALSE);
                    acount = 0;
                    if (!(accelerator != prevaccelerator || one_per_line))
                        wastoolong = TRUE;
                }
            }
            prevaccelerator = accelerator;
            if (!acount || one_per_line) {
#if 0 /*KR:T*/
                Sprintf(prompt, "%s%s [%s]", wastoolong ? "or " : "",
                        choices[i]->ef_txt, choices[i]->ef_desc);
            } else if (acount == 1) {
                Sprintf(prompt, "%s%s or %s", wastoolong ? "or " : "",
                        choices[i - 1]->ef_txt, choices[i]->ef_txt);
#else
                Sprintf(prompt, "%s%s [%s]", wastoolong ? "또는 " : "",
                        choices[i]->ef_txt, choices[i]->ef_desc);
            } else if (acount == 1) {
                Sprintf(prompt, "%s%s 또는 %s", wastoolong ? "또는 " : "",
                        choices[i - 1]->ef_txt, choices[i]->ef_txt);
#endif
            } else {
                /*KR Strcat(prompt, " or "); */
                Strcat(prompt, " 또는 ");
                Strcat(prompt, choices[i]->ef_txt);
            }
            ++acount;
        }
        if (acount) {
            /* flush buf */
            Sprintf(buf, fmtstr, prompt);
            any.a_char = prevaccelerator;
            add_menu(win, NO_GLYPH, &any, any.a_char, 0, ATR_NONE, buf,
                     FALSE);
        }
        /*KR Sprintf(prompt, "Extended Command: %s", cbuf); */
        Sprintf(prompt, "확장 명령어: %s", cbuf);
        end_menu(win, prompt);
        n = select_menu(win, PICK_ONE, &pick_list);
        destroy_nhwindow(win);
        if (n == 1) {
            if (matchlevel > (QBUFSZ - 2)) {
                free((genericptr_t) pick_list);
#if (NH_DEVEL_STATUS != NH_STATUS_RELEASED)
                impossible("Too many chars (%d) entered in extcmd_via_menu()",
                           matchlevel);
#endif
                ret = -1;
            } else {
                cbuf[matchlevel++] = pick_list[0].item.a_char;
                cbuf[matchlevel] = '\0';
                free((genericptr_t) pick_list);
            }
        } else {
            if (matchlevel) {
                ret = 0;
                matchlevel = 0;
            } else
                ret = -1;
        }
    }
    return ret;
}
#endif /* TTY_GRAPHICS */

/* #monster command - use special monster ability while polymorphed */
int
domonability(VOID_ARGS)
{
    if (can_breathe(youmonst.data))
        return dobreathe();
    else if (attacktype(youmonst.data, AT_SPIT))
        return dospit();
    else if (youmonst.data->mlet == S_NYMPH)
        return doremove();
    else if (attacktype(youmonst.data, AT_GAZE))
        return dogaze();
    else if (is_were(youmonst.data))
        return dosummon();
    else if (webmaker(youmonst.data))
        return dospinweb();
    else if (is_hider(youmonst.data))
        return dohide();
    else if (is_mind_flayer(youmonst.data))
        return domindblast();
    else if (u.umonnum == PM_GREMLIN) {
        if (IS_FOUNTAIN(levl[u.ux][u.uy].typ)) {
            if (split_mon(&youmonst, (struct monst *) 0))
                dryup(u.ux, u.uy, TRUE);
        } else
            /*KR There("is no fountain here."); */
            pline("여기에는 분수가 없다.");
    } else if (is_unicorn(youmonst.data)) {
        use_unicorn_horn((struct obj *) 0);
        return 1;
    } else if (youmonst.data->msound == MS_SHRIEK) {
        /*KR You("shriek."); */
        You("비명을 질렀다.");
        if (u.uburied)
            /*KR pline("Unfortunately sound does not carry well through rock."); */
            pline("불행히도 소리는 바위를 잘 통과하지 못한다.");
        else
            aggravate();
    } else if (youmonst.data->mlet == S_VAMPIRE)
        return dopoly();
    else if (Upolyd)
        /*KR pline("Any special ability you may have is purely reflexive."); */
        pline("당신이 가진 특수 능력은 모두 반사적으로만 발동된다.");
    else
        /*KR You("don't have a special ability in your normal form!"); */
        pline("원래 모습으로는 특수 능력이 없다!");
    return 0;
}

int
enter_explore_mode(VOID_ARGS)
{
    if (wizard) {
        /*KR You("are in debug mode."); */
        You("디버그 모드입니다.");
    } else if (discover) {
        /*KR You("are already in explore mode."); */
        You("이미 탐험 모드입니다.");
    } else {
#ifdef SYSCF
#if defined(UNIX)
        if (!sysopt.explorers || !sysopt.explorers[0]
            || !check_user_string(sysopt.explorers)) {
            /*KR You("cannot access explore mode."); */
            You("탐험 모드에 접근할 수 없습니다.");
            return 0;
        }
#endif
#endif
        pline(
        /*KR "Beware!  From explore mode there will be no return to normal game."); */
        "경고! 탐험 모드에 진입하면 일반 게임으로 돌아갈 수 없습니다.");
        if (paranoid_query(ParanoidQuit,
                      /*KR "Do you want to enter explore mode?")) { */
                           "탐험 모드에 진입하시겠습니까?")) {
            clear_nhwindow(WIN_MESSAGE);
            /*KR You("are now in non-scoring explore mode."); */
            You("이제 점수가 기록되지 않는 탐험 모드입니다.");
            discover = TRUE;
        } else {
            clear_nhwindow(WIN_MESSAGE);
            /*KR pline("Resuming normal game."); */
            pline("일반 게임을 계속합니다.");
        }
    }
    return 0;
}

/* ^W command - wish for something */
STATIC_PTR int
wiz_wish(VOID_ARGS) /* Unlimited wishes for debug mode by Paul Polderman */
{
    if (wizard) {
        boolean save_verbose = flags.verbose;

        flags.verbose = FALSE;
        makewish();
        flags.verbose = save_verbose;
        (void) encumber_msg();
    } else
        pline(unavailcmd, visctrl((int) cmd_from_func(wiz_wish)));
    return 0;
}

/* ^I command - reveal and optionally identify hero's inventory */
STATIC_PTR int
wiz_identify(VOID_ARGS)
{
    if (wizard) {
        iflags.override_ID = (int) cmd_from_func(wiz_identify);
        /* command remapping might leave #wizidentify as the only way
           to invoke us, in which case cmd_from_func() will yield NUL;
           it won't matter to display_inventory()/display_pickinv()
           if ^I invokes some other command--what matters is that
           display_pickinv() and xname() see override_ID as nonzero */
        if (!iflags.override_ID)
            iflags.override_ID = C('I');
        (void) display_inventory((char *) 0, FALSE);
        iflags.override_ID = 0;
    } else
        pline(unavailcmd, visctrl((int) cmd_from_func(wiz_identify)));
    return 0;
}

/* #wizmakemap - discard current dungeon level and replace with a new one */
STATIC_PTR int
wiz_makemap(VOID_ARGS)
{
    if (wizard) {
        struct monst *mtmp;
        boolean was_in_W_tower = In_W_tower(u.ux, u.uy, &u.uz);

        rm_mapseen(ledger_no(&u.uz));
        for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
            if (mtmp->isgd) { /* vault is going away; get rid of guard */
                mtmp->isgd = 0;
                mongone(mtmp);
            }
            if (DEADMONSTER(mtmp))
                continue;
            if (mtmp->isshk)
                setpaid(mtmp);
            /* TODO?
             *  Reduce 'born' tally for each monster about to be discarded
             *  by savelev(), otherwise replacing heavily populated levels
             *  tends to make their inhabitants become extinct.
             */
        }
        if (Punished) {
            ballrelease(FALSE);
            unplacebc();
        }
        /* reset lock picking unless it's for a carried container */
        maybe_reset_pick((struct obj *) 0);
        /* reset interrupted digging if it was taking place on this level */
        if (on_level(&context.digging.level, &u.uz))
            (void) memset((genericptr_t) &context.digging, 0,
                          sizeof (struct dig_info));
        /* reset cached targets */
        iflags.travelcc.x = iflags.travelcc.y = 0; /* travel destination */
        context.polearm.hitmon = (struct monst *) 0; /* polearm target */
        /* escape from trap */
        reset_utrap(FALSE);
        check_special_room(TRUE); /* room exit */
        u.ustuck = (struct monst *) 0;
        u.uswallow = 0;
        u.uinwater = 0;
        u.uundetected = 0; /* not hidden, even if means are available */
        dmonsfree(); /* purge dead monsters from 'fmon' */
        /* keep steed and other adjacent pets after releasing them
           from traps, stopping eating, &c as if hero were ascending */
        keepdogs(TRUE); /* (pets-only; normally we'd be using 'FALSE' here) */

        /* discard current level; "saving" is used to release dynamic data */
        savelev(-1, ledger_no(&u.uz), FREE_SAVE);
        /* create a new level; various things like bestowing a guardian
           angel on Astral or setting off alarm on Ft.Ludios are handled
           by goto_level(do.c) so won't occur for replacement levels */
        mklev();

        vision_reset();
        vision_full_recalc = 1;
        cls();
        /* was using safe_teleds() but that doesn't honor arrival region
           on levels which have such; we don't force stairs, just area */
        u_on_rndspot((u.uhave.amulet ? 1 : 0) /* 'going up' flag */
                     | (was_in_W_tower ? 2 : 0));
        losedogs();
        kill_genocided_monsters();
        /* u_on_rndspot() might pick a spot that has a monster, or losedogs()
           might pick the hero's spot (only if there isn't already a monster
           there), so we might have to move hero or the co-located monster */
        if ((mtmp = m_at(u.ux, u.uy)) != 0)
            u_collide_m(mtmp);
        initrack();
        if (Punished) {
            unplacebc();
            placebc();
        }
        docrt();
        flush_screen(1);
        deliver_splev_message(); /* level entry */
        check_special_room(FALSE); /* room entry */
#ifdef INSURANCE
        save_currentstate();
#endif
    } else {
        pline(unavailcmd, "#wizmakemap");
    }
    return 0;
}

/* ^F command - reveal the level map and any traps on it */
STATIC_PTR int
wiz_map(VOID_ARGS)
{
    if (wizard) {
        struct trap *t;
        long save_Hconf = HConfusion, save_Hhallu = HHallucination;

        HConfusion = HHallucination = 0L;
        for (t = ftrap; t != 0; t = t->ntrap) {
            t->tseen = 1;
            map_trap(t, TRUE);
        }
        do_mapping();
        HConfusion = save_Hconf;
        HHallucination = save_Hhallu;
    } else
        pline(unavailcmd, visctrl((int) cmd_from_func(wiz_map)));
    return 0;
}

/* ^G command - generate monster(s); a count prefix will be honored */
STATIC_PTR int
wiz_genesis(VOID_ARGS)
{
    if (wizard)
        (void) create_particular();
    else
        pline(unavailcmd, visctrl((int) cmd_from_func(wiz_genesis)));
    return 0;
}

/* ^O command - display dungeon layout */
STATIC_PTR int
wiz_where(VOID_ARGS)
{
    if (wizard)
        (void) print_dungeon(FALSE, (schar *) 0, (xchar *) 0);
    else
        pline(unavailcmd, visctrl((int) cmd_from_func(wiz_where)));
    return 0;
}

/* ^E command - detect unseen (secret doors, traps, hidden monsters) */
STATIC_PTR int
wiz_detect(VOID_ARGS)
{
    if (wizard)
        (void) findit();
    else
        pline(unavailcmd, visctrl((int) cmd_from_func(wiz_detect)));
    return 0;
}

/* ^V command - level teleport */
STATIC_PTR int
wiz_level_tele(VOID_ARGS)
{
    if (wizard)
        level_tele();
    else
        pline(unavailcmd, visctrl((int) cmd_from_func(wiz_level_tele)));
    return 0;
}

/* #levelchange command - adjust hero's experience level */
STATIC_PTR int
wiz_level_change(VOID_ARGS)
{
    char buf[BUFSZ] = DUMMY;
    int newlevel = 0;
    int ret;

    /*KR getlin("To what experience level do you want to be set?", buf); */
    getlin("경험 레벨을 몇으로 설정하시겠습니까?", buf);
    (void) mungspaces(buf);
    if (buf[0] == '\033' || buf[0] == '\0')
        ret = 0;
    else
        ret = sscanf(buf, "%d", &newlevel);

    if (ret != 1) {
        pline1(Never_mind);
        return 0;
    }
    if (newlevel == u.ulevel) {
        /*KR You("are already that experienced."); */
        You("이미 그 경험 레벨입니다.");
    } else if (newlevel < u.ulevel) {
        if (u.ulevel == 1) {
            /*KR You("are already as inexperienced as you can get."); */
            You("이미 가능한 한 가장 낮은 경험 레벨입니다.");
            return 0;
        }
        if (newlevel < 1)
            newlevel = 1;
        while (u.ulevel > newlevel)
            /*KR losexp("#levelchange"); */
            losexp("#levelchange 명령어로");
    } else {
        if (u.ulevel >= MAXULEV) {
            /*KR You("are already as experienced as you can get."); */
            You("이미 가능한 한 가장 높은 경험 레벨입니다.");
            return 0;
        }
        if (newlevel > MAXULEV)
            newlevel = MAXULEV;
        while (u.ulevel < newlevel)
            pluslvl(FALSE);
    }
    u.ulevelmax = u.ulevel;
    return 0;
}

/* #panic command - test program's panic handling */
STATIC_PTR int
wiz_panic(VOID_ARGS)
{
    if (iflags.debug_fuzzer) {
        u.uhp = u.uhpmax = 1000;
        u.uen = u.uenmax = 1000;
        return 0;
    }
#if 0 /*KR:T*/
    if (paranoid_query(ParanoidQuit,
                       "Do you want to call panic() and end your game?"))
#else
    if (paranoid_query(ParanoidQuit,
                       "panic() 함수를 호출하여 게임을 종료하시겠습니까?"))
#endif
        panic("Crash test.");
    return 0;
}

/* #polyself command - change hero's form */
STATIC_PTR int
wiz_polyself(VOID_ARGS)
{
    polyself(1);
    return 0;
}

/* #seenv command */
STATIC_PTR int
wiz_show_seenv(VOID_ARGS)
{
    winid win;
    int x, y, v, startx, stopx, curx;
    char row[COLNO + 1];

    win = create_nhwindow(NHW_TEXT);
    /*
     * Each seenv description takes up 2 characters, so center
     * the seenv display around the hero.
     */
    startx = max(1, u.ux - (COLNO / 4));
    stopx = min(startx + (COLNO / 2), COLNO);
    /* can't have a line exactly 80 chars long */
    if (stopx - startx == COLNO / 2)
        startx++;

    for (y = 0; y < ROWNO; y++) {
        for (x = startx, curx = 0; x < stopx; x++, curx += 2) {
            if (x == u.ux && y == u.uy) {
                row[curx] = row[curx + 1] = '@';
            } else {
                v = levl[x][y].seenv & 0xff;
                if (v == 0)
                    row[curx] = row[curx + 1] = ' ';
                else
                    Sprintf(&row[curx], "%02x", v);
            }
        }
        /* remove trailing spaces */
        for (x = curx - 1; x >= 0; x--)
            if (row[x] != ' ')
                break;
        row[x + 1] = '\0';

        putstr(win, 0, row);
    }
    display_nhwindow(win, TRUE);
    destroy_nhwindow(win);
    return 0;
}

/* #vision command */
STATIC_PTR int
wiz_show_vision(VOID_ARGS)
{
    winid win;
    int x, y, v;
    char row[COLNO + 1];

    win = create_nhwindow(NHW_TEXT);
    Sprintf(row, "Flags: 0x%x could see, 0x%x in sight, 0x%x temp lit",
            COULD_SEE, IN_SIGHT, TEMP_LIT);
    putstr(win, 0, row);
    putstr(win, 0, "");
    for (y = 0; y < ROWNO; y++) {
        for (x = 1; x < COLNO; x++) {
            if (x == u.ux && y == u.uy)
                row[x] = '@';
            else {
                v = viz_array[y][x]; /* data access should be hidden */
                if (v == 0)
                    row[x] = ' ';
                else
                    row[x] = '0' + viz_array[y][x];
            }
        }
        /* remove trailing spaces */
        for (x = COLNO - 1; x >= 1; x--)
            if (row[x] != ' ')
                break;
        row[x + 1] = '\0';

        putstr(win, 0, &row[1]);
    }
    display_nhwindow(win, TRUE);
    destroy_nhwindow(win);
    return 0;
}

/* #wmode command */
STATIC_PTR int
wiz_show_wmodes(VOID_ARGS)
{
    winid win;
    int x, y;
    char row[COLNO + 1];
    struct rm *lev;
    boolean istty = WINDOWPORT("tty");

    win = create_nhwindow(NHW_TEXT);
    if (istty)
        putstr(win, 0, ""); /* tty only: blank top line */
    for (y = 0; y < ROWNO; y++) {
        for (x = 0; x < COLNO; x++) {
            lev = &levl[x][y];
            if (x == u.ux && y == u.uy)
                row[x] = '@';
            else if (IS_WALL(lev->typ) || lev->typ == SDOOR)
                row[x] = '0' + (lev->wall_info & WM_MASK);
            else if (lev->typ == CORR)
                row[x] = '#';
            else if (IS_ROOM(lev->typ) || IS_DOOR(lev->typ))
                row[x] = '.';
            else
                row[x] = 'x';
        }
        row[COLNO] = '\0';
        /* map column 0, levl[0][], is off the left edge of the screen */
        putstr(win, 0, &row[1]);
    }
    display_nhwindow(win, TRUE);
    destroy_nhwindow(win);
    return 0;
}

/* wizard mode variant of #terrain; internal levl[][].typ values in base-36 */
STATIC_OVL void
wiz_map_levltyp(VOID_ARGS)
{
    winid win;
    int x, y, terrain;
    char row[COLNO + 1];
    boolean istty = !strcmp(windowprocs.name, "tty");

    win = create_nhwindow(NHW_TEXT);
    /* map row 0, levl[][0], is drawn on the second line of tty screen */
    if (istty)
        putstr(win, 0, ""); /* tty only: blank top line */
    for (y = 0; y < ROWNO; y++) {
        /* map column 0, levl[0][], is off the left edge of the screen;
           it should always have terrain type "undiggable stone" */
        for (x = 1; x < COLNO; x++) {
            terrain = levl[x][y].typ;
            /* assumes there aren't more than 10+26+26 terrain types */
            row[x - 1] = (char) ((terrain == STONE && !may_dig(x, y))
                                    ? '*'
                                    : (terrain < 10)
                                       ? '0' + terrain
                                       : (terrain < 36)
                                          ? 'a' + terrain - 10
                                          : 'A' + terrain - 36);
        }
        x--;
        if (levl[0][y].typ != STONE || may_dig(0, y))
            row[x++] = '!';
        row[x] = '\0';
        putstr(win, 0, row);
    }

    {
        char dsc[BUFSZ];
        s_level *slev = Is_special(&u.uz);

        Sprintf(dsc, "D:%d,L:%d", u.uz.dnum, u.uz.dlevel);
        /* [dungeon branch features currently omitted] */
        /* special level features */
        if (slev) {
            Sprintf(eos(dsc), " \"%s\"", slev->proto);
            /* special level flags (note: dungeon.def doesn't set `maze'
               or `hell' for any specific levels so those never show up) */
            if (slev->flags.maze_like)
                Strcat(dsc, " mazelike");
            if (slev->flags.hellish)
                Strcat(dsc, " hellish");
            if (slev->flags.town)
                Strcat(dsc, " town");
            if (slev->flags.rogue_like)
                Strcat(dsc, " roguelike");
            /* alignment currently omitted to save space */
        }
        /* level features */
        if (level.flags.nfountains)
            Sprintf(eos(dsc), " %c:%d", defsyms[S_fountain].sym,
                    (int) level.flags.nfountains);
        if (level.flags.nsinks)
            Sprintf(eos(dsc), " %c:%d", defsyms[S_sink].sym,
                    (int) level.flags.nsinks);
        if (level.flags.has_vault)
            Strcat(dsc, " vault");
        if (level.flags.has_shop)
            Strcat(dsc, " shop");
        if (level.flags.has_temple)
            Strcat(dsc, " temple");
        if (level.flags.has_court)
            Strcat(dsc, " throne");
        if (level.flags.has_zoo)
            Strcat(dsc, " zoo");
        if (level.flags.has_morgue)
            Strcat(dsc, " morgue");
        if (level.flags.has_barracks)
            Strcat(dsc, " barracks");
        if (level.flags.has_beehive)
            Strcat(dsc, " hive");
        if (level.flags.has_swamp)
            Strcat(dsc, " swamp");
        /* level flags */
        if (level.flags.noteleport)
            Strcat(dsc, " noTport");
        if (level.flags.hardfloor)
            Strcat(dsc, " noDig");
        if (level.flags.nommap)
            Strcat(dsc, " noMMap");
        if (!level.flags.hero_memory)
            Strcat(dsc, " noMem");
        if (level.flags.shortsighted)
            Strcat(dsc, " shortsight");
        if (level.flags.graveyard)
            Strcat(dsc, " graveyard");
        if (level.flags.is_maze_lev)
            Strcat(dsc, " maze");
        if (level.flags.is_cavernous_lev)
            Strcat(dsc, " cave");
        if (level.flags.arboreal)
            Strcat(dsc, " tree");
        if (Sokoban)
            Strcat(dsc, " sokoban-rules");
        /* non-flag info; probably should include dungeon branching
           checks (extra stairs and magic portals) here */
        if (Invocation_lev(&u.uz))
            Strcat(dsc, " invoke");
        if (On_W_tower_level(&u.uz))
            Strcat(dsc, " tower");
        /* append a branch identifier for completeness' sake */
        if (u.uz.dnum == 0)
            Strcat(dsc, " dungeon");
        else if (u.uz.dnum == mines_dnum)
            Strcat(dsc, " mines");
        else if (In_sokoban(&u.uz))
            Strcat(dsc, " sokoban");
        else if (u.uz.dnum == quest_dnum)
            Strcat(dsc, " quest");
        else if (Is_knox(&u.uz))
            Strcat(dsc, " ludios");
        else if (u.uz.dnum == 1)
            Strcat(dsc, " gehennom");
        else if (u.uz.dnum == tower_dnum)
            Strcat(dsc, " vlad");
        else if (In_endgame(&u.uz))
            Strcat(dsc, " endgame");
        else {
            /* somebody's added a dungeon branch we're not expecting */
            const char *brname = dungeons[u.uz.dnum].dname;

            if (!brname || !*brname)
                brname = "unknown";
            if (!strncmpi(brname, "the ", 4))
                brname += 4;
            Sprintf(eos(dsc), " %s", brname);
        }
        /* limit the line length to map width */
        if (strlen(dsc) >= COLNO)
            dsc[COLNO - 1] = '\0'; /* truncate */
        putstr(win, 0, dsc);
    }

    display_nhwindow(win, TRUE);
    destroy_nhwindow(win);
    return;
}

/* temporary? hack, since level type codes aren't the same as screen
   symbols and only the latter have easily accessible descriptions */
static const char *levltyp[] = {
    "stone", "vertical wall", "horizontal wall", "top-left corner wall",
    "top-right corner wall", "bottom-left corner wall",
    "bottom-right corner wall", "cross wall", "tee-up wall", "tee-down wall",
    "tee-left wall", "tee-right wall", "drawbridge wall", "tree",
    "secret door", "secret corridor", "pool", "moat", "water",
    "drawbridge up", "lava pool", "iron bars", "door", "corridor", "room",
    "stairs", "ladder", "fountain", "throne", "sink", "grave", "altar", "ice",
    "drawbridge down", "air", "cloud",
    /* not a real terrain type, but used for undiggable stone
       by wiz_map_levltyp() */
    "unreachable/undiggable",
    /* padding in case the number of entries above is odd */
    ""
};

/* explanation of base-36 output from wiz_map_levltyp() */
STATIC_OVL void
wiz_levltyp_legend(VOID_ARGS)
{
    winid win;
    int i, j, last, c;
    const char *dsc, *fmt;
    char buf[BUFSZ];

    win = create_nhwindow(NHW_TEXT);
    putstr(win, 0, "#terrain encodings:");
    putstr(win, 0, "");
    fmt = " %c - %-28s"; /* TODO: include tab-separated variant for win32 */
    *buf = '\0';
    /* output in pairs, left hand column holds [0],[1],...,[N/2-1]
       and right hand column holds [N/2],[N/2+1],...,[N-1];
       N ('last') will always be even, and may or may not include
       the empty string entry to pad out the final pair, depending
       upon how many other entries are present in levltyp[] */
    last = SIZE(levltyp) & ~1;
    for (i = 0; i < last / 2; ++i)
        for (j = i; j < last; j += last / 2) {
            dsc = levltyp[j];
            c = !*dsc ? ' '
                   : !strncmp(dsc, "unreachable", 11) ? '*'
                      /* same int-to-char conversion as wiz_map_levltyp() */
                      : (j < 10) ? '0' + j
                         : (j < 36) ? 'a' + j - 10
                            : 'A' + j - 36;
            Sprintf(eos(buf), fmt, c, dsc);
            if (j > i) {
                putstr(win, 0, buf);
                *buf = '\0';
            }
        }
    display_nhwindow(win, TRUE);
    destroy_nhwindow(win);
    return;
}

/* #wizsmell command - test usmellmon(). */
STATIC_PTR int
wiz_smell(VOID_ARGS)
{
    int ans = 0;
    int mndx;  /* monster index */
    coord cc;  /* screen pos of unknown glyph */
    int glyph; /* glyph at selected position */

    cc.x = u.ux;
    cc.y = u.uy;
    mndx = 0; /* gcc -Wall lint */
    if (!olfaction(youmonst.data)) {
        /*KR You("are incapable of detecting odors in your present form."); */
        You("현재 모습으로는 냄새를 맡을 수 없습니다.");
        return 0;
    }

    /*KR pline("You can move the cursor to a monster that you want to
     * smell."); */
    pline("커서를 이동하여 냄새를 맡을 몬스터를 선택할 수 있습니다.");
    do {
        /*KR pline("Pick a monster to smell."); */
        pline("냄새를 맡을 몬스터를 선택하십시오.");
        /*KR ans = getpos(&cc, TRUE, "a monster"); */
        ans = getpos(&cc, TRUE, "몬스터");
        if (ans < 0 || cc.x < 0) {
            return 0; /* done */
        }
        /* Convert the glyph at the selected position to a mndxbol. */
        glyph = glyph_at(cc.x, cc.y);
        if (glyph_is_monster(glyph))
            mndx = glyph_to_mon(glyph);
        else
            mndx = 0;
        /* Is it a monster? */
        if (mndx) {
            if (!usmellmon(&mons[mndx]))
                /*KR pline("That monster seems to give off no smell."); */
                pline("그 몬스터는 아무런 냄새도 풍기지 않는 것 같다.");
        } else
            /*KR pline("That is not a monster."); */
            pline("그것은 몬스터가 아닙니다.");
    } while (TRUE);
    return 0;
}

/* #wizinstrinsic command to set some intrinsics for testing */
STATIC_PTR int
wiz_intrinsic(VOID_ARGS)
{
    if (wizard) {
        extern const struct propname {
            int prop_num;
            const char *prop_name;
        } propertynames[]; /* timeout.c */
        static const char wizintrinsic[] = "#wizintrinsic";
        static const char fmt[] = "You are%s %s.";
        winid win;
        anything any;
        char buf[BUFSZ];
        int i, j, n, p, amt, typ;
        long oldtimeout, newtimeout;
        const char *propname;
        menu_item *pick_list = (menu_item *) 0;

        any = zeroany;
        win = create_nhwindow(NHW_MENU);
        start_menu(win);
        for (i = 0; (propname = propertynames[i].prop_name) != 0; ++i) {
            p = propertynames[i].prop_num;
            if (p == HALLUC_RES) {
                /* Grayswandir vs hallucination; ought to be redone to
                   use u.uprops[HALLUC].blocked instead of being treated
                   as a separate property; letting in be manually toggled
                   even only in wizard mode would be asking for trouble... */
                continue;
            }
            if (p == FIRE_RES) {
                any.a_int = 0;
                add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE, "--", FALSE);
            }
            any.a_int = i + 1; /* +1: avoid 0 */
            oldtimeout = u.uprops[p].intrinsic & TIMEOUT;
            if (oldtimeout)
                Sprintf(buf, "%-27s [%li]", propname, oldtimeout);
            else
                Sprintf(buf, "%s", propname);
            add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, FALSE);
        }
        /*KR end_menu(win, "Which intrinsics?"); */
        end_menu(win, "어떤 내재적 능력을 설정하시겠습니까?");
        n = select_menu(win, PICK_ANY, &pick_list);
        destroy_nhwindow(win);

        amt = 30; /* TODO: prompt for duration */
        for (j = 0; j < n; ++j) {
            i = pick_list[j].item.a_int - 1; /* -1: reverse +1 above */
            p = propertynames[i].prop_num;
            oldtimeout = u.uprops[p].intrinsic & TIMEOUT;
            newtimeout = oldtimeout + (long) amt;
            switch (p) {
            case SICK:
            case SLIMED:
            case STONED:
                if (oldtimeout > 0L && newtimeout > oldtimeout)
                    newtimeout = oldtimeout;
                break;
            }

            switch (p) {
            case BLINDED:
                make_blinded(newtimeout, TRUE);
                break;
#if 0       /* make_confused() only gives feedback when confusion is
             * ending so use the 'default' case for it instead */
            case CONFUSION:
                make_confused(newtimeout, TRUE);
                break;
#endif /*0*/
            case DEAF:
                make_deaf(newtimeout, TRUE);
                break;
            case HALLUC:
                make_hallucinated(newtimeout, TRUE, 0L);
                break;
            case SICK:
                typ = !rn2(2) ? SICK_VOMITABLE : SICK_NONVOMITABLE;
                make_sick(newtimeout, wizintrinsic, TRUE, typ);
                break;
            case SLIMED:
#if 0 /*KR: 원본*/
                Sprintf(buf, fmt,
                        !Slimed ? "" : " still", "turning into slime");
#else /*KR: KRNethack 맞춤 번역 (영어 fmt 구조 무시)*/
                Sprintf(buf, "당신은%s 슬라임이 되어가고 있다.",
                        !Slimed ? "" : " 여전히");
#endif
                make_slimed(newtimeout, buf);
                break;
            case STONED:
#if 0 /*KR: 원본*/
                Sprintf(buf, fmt,
                        !Stoned ? "" : " still", "turning into stone");
#else
                Sprintf(buf, "당신은%s 돌이 되어가고 있다.",
                        !Stoned ? "" : " 여전히");
#endif
                make_stoned(newtimeout, buf, KILLED_BY, wizintrinsic);
                break;
            case STUNNED:
                make_stunned(newtimeout, TRUE);
                break;
            case VOMITING:
#if 0 /*KR: 원본*/
                Sprintf(buf, fmt, !Vomiting ? "" : " still", "vomiting");
#else
                Sprintf(buf, "당신은%s 구역질을 하고 있다.",
                        !Vomiting ? "" : " 여전히");
#endif
                make_vomiting(newtimeout, FALSE);
                pline1(buf);
                break;
            case WARN_OF_MON:
                if (!Warn_of_mon) {
                    context.warntype.speciesidx = PM_GRID_BUG;
                    context.warntype.species
                                         = &mons[context.warntype.speciesidx];
                }
                goto def_feedback;
            case GLIB:
                /* slippery fingers applies to gloves if worn at the time
                   so persistent inventory might need updating */
                make_glib((int) newtimeout);
                goto def_feedback;
            case LEVITATION:
            case FLYING:
                float_vs_flight();
                /*FALLTHRU*/
            default:
 def_feedback:
#if 0 /*KR: 원본*/
                pline("Timeout for %s %s %d.", propertynames[i].prop_name,
                      oldtimeout ? "increased by" : "set to", amt);
#else /*KR: KRNethack 맞춤 번역*/
                pline("%s의 제한 시간이 %d만큼 %s.",
                      propertynames[i].prop_name, amt,
                      oldtimeout ? "증가했습니다" : "설정되었습니다");
#endif
                if (p != GLIB)
                    incr_itimeout(&u.uprops[p].intrinsic, amt);
                break;
            }
            context.botl = 1; /* probably not necessary... */
        }
        if (n >= 1)
            free((genericptr_t) pick_list);
        doredraw();
    } else
        pline(unavailcmd, visctrl((int) cmd_from_func(wiz_intrinsic)));
    return 0;
}

/* #wizrumorcheck command - verify each rumor access */
STATIC_PTR int
wiz_rumor_check(VOID_ARGS)
{
    rumor_check();
    return 0;
}

/* #terrain command -- show known map, inspired by crawl's '|' command */
STATIC_PTR int
doterrain(VOID_ARGS)
{
    winid men;
    menu_item *sel;
    anything any;
    int n;
    int which;

    /*
     * normal play: choose between known map without mons, obj, and traps
     *  (to see underlying terrain only), or
     *  known map without mons and objs (to see traps under mons and objs), or
     *  known map without mons (to see objects under monsters);
     * explore mode: normal choices plus full map (w/o mons, objs, traps);
     * wizard mode: normal and explore choices plus
     *  a dump of the internal levl[][].typ codes w/ level flags, or
     *  a legend for the levl[][].typ codes dump
     */
    men = create_nhwindow(NHW_MENU);
    start_menu(men);
    any = zeroany;
    any.a_int = 1;
    add_menu(men, NO_GLYPH, &any, 0, 0, ATR_NONE,
        /*KR "known map without monsters, objects, and traps", */
             "알려진 지도 (몬스터, 아이템, 함정 없음)",
             MENU_SELECTED);
    any.a_int = 2;
    add_menu(men, NO_GLYPH, &any, 0, 0, ATR_NONE,
        /*KR "known map without monsters and objects", */
             "알려진 지도 (몬스터, 아이템 없음)",
             MENU_UNSELECTED);
    any.a_int = 3;
    add_menu(men, NO_GLYPH, &any, 0, 0, ATR_NONE,
        /*KR "known map without monsters", */
             "알려진 지도 (몬스터 없음)",
             MENU_UNSELECTED);
    if (discover || wizard) {
        any.a_int = 4;
        add_menu(men, NO_GLYPH, &any, 0, 0, ATR_NONE,
            /*KR "full map without monsters, objects, and traps", */
                 "전체 지도 (몬스터, 아이템, 함정 없음)",
                 MENU_UNSELECTED);
        if (wizard) {
            any.a_int = 5;
            add_menu(men, NO_GLYPH, &any, 0, 0, ATR_NONE,
                     "internal levl[][].typ codes in base-36",
                     MENU_UNSELECTED);
            any.a_int = 6;
            add_menu(men, NO_GLYPH, &any, 0, 0, ATR_NONE,
                     "legend of base-36 levl[][].typ codes",
                     MENU_UNSELECTED);
        }
    }
    /*KR end_menu(men, "View which?"); */
    end_menu(men, "어떤 것을 보시겠습니까?");

    n = select_menu(men, PICK_ONE, &sel);
    destroy_nhwindow(men);
    /*
     * n <  0: player used ESC to cancel;
     * n == 0: preselected entry was explicitly chosen and got toggled off;
     * n == 1: preselected entry was implicitly chosen via <space>|<enter>;
     * n == 2: another entry was explicitly chosen, so skip preselected one.
     */
    which = (n < 0) ? -1 : (n == 0) ? 1 : sel[0].item.a_int;
    if (n > 1 && which == 1)
        which = sel[1].item.a_int;
    if (n > 0)
        free((genericptr_t) sel);

    switch (which) {
    case 1: /* known map */
        reveal_terrain(0, TER_MAP);
        break;
    case 2: /* known map with known traps */
        reveal_terrain(0, TER_MAP | TER_TRP);
        break;
    case 3: /* known map with known traps and objects */
        reveal_terrain(0, TER_MAP | TER_TRP | TER_OBJ);
        break;
    case 4: /* full map */
        reveal_terrain(1, TER_MAP);
        break;
    case 5: /* map internals */
        wiz_map_levltyp();
        break;
    case 6: /* internal details */
        wiz_levltyp_legend();
        break;
    default:
        break;
    }
    return 0; /* no time elapses */
}

/* -enlightenment and conduct- */
static winid en_win = WIN_ERR;
static boolean en_via_menu = FALSE;
#if 0 /*KR: 원본*/
static const char You_[] = "You ", are[] = "are ", were[] = "were ",
                  have[] = "have ", had[] = "had ", can[] = "can ",
                  could[] = "could ";
#else /*KR: KRNethack 맞춤 번역 (SOV 어순을 위한 조사/어미 세팅)*/
static const char You_[] = "당신은 ", are[] = "이다", were[] = "이었다",
                  have[] = "을(를) 가지고 있다",
                  had[] = "을(를) 가지고 있었다", can[] = "할 수 있다",
                  could[] = "할 수 있었다", iru[] = "있다", ita[] = "있었다";
#endif
#if 0 /*KR: 미사용 */
static const char have_been[] = "have been ", have_never[] = "have never ",
                  never[] = "never ";
#endif

#if 0 /*KR: 원본*/
#define enl_msg(prefix, present, past, suffix, ps) \
    enlght_line(prefix, final ? past : present, suffix, ps)
#else /*KR: 문장 구조를 [주어] + [목적어/보어] + [동사] 순서로 재배치*/
#define enl_msg(prefix, present, past, suffix, ps) \
    enlght_line(prefix, ps, suffix, final ? past : present)
#endif
#define you_are(attr, ps) enl_msg(You_, are, were, attr, ps)
#define you_have(attr, ps) enl_msg(You_, have, had, attr, ps)
#define you_can(attr, ps) enl_msg(You_, can, could, attr, ps)
/*KR #define you_have_been(goodthing) enl_msg(You_, have_been, were, goodthing, "") */
#define you_have_been(goodthing) enl_msg(You_, are, were, goodthing, "")
#if 0 /*KR: 원본*/
#define you_have_never(badthing) \
    enl_msg(You_, have_never, never, badthing, "")
#define you_have_X(something) \
    enl_msg(You_, have, (const char *) "", something, "")
#else /*KR: KRNethack 맞춤 번역 (~한 적이 있다/없다 패턴)*/
#define you_have_never(badthing) \
    enl_msg(badthing, "적이 없다", "적이 없었다", "", "")
#define you_have_X(something) \
    enl_msg(something, "적이 있다", "적이 있었다", "", "")
#endif
#if 1 /*KR ~ing도 자연스럽게 출력하기 위함 */
#define you_are_ing(goodthing, ps) enl_msg(You_, iru, ita, goodthing, ps)
/*KR: 코드 작성 가독성을 위해 어순을 [주어, 목적어/수식어, 부가설명, 현재동사,
 * 과거동사]로 맞춘 래퍼 매크로 */
#define enl_msg_kr(prefix, suffix, ps, present, past) \
    enl_msg(prefix, present, past, suffix, ps)
#endif


static void
enlght_out(buf)
const char *buf;
{
    if (en_via_menu) {
        anything any;

        any = zeroany;
        add_menu(en_win, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, FALSE);
    } else
        putstr(en_win, 0, buf);
}

static void
enlght_line(start, middle, end, ps)
const char *start, *middle, *end, *ps;
{
    char buf[BUFSZ];

    Sprintf(buf, " %s%s%s%s.", start, middle, end, ps);
    enlght_out(buf);
}

/* format increased chance to hit or damage or defense (Protection) */
static char *
enlght_combatinc(inctyp, incamt, final, outbuf)
const char *inctyp;
int incamt, final;
char *outbuf;
{
    const char *modif, *bonus;
#if 0 /*KR: 원본 (한국어 어순에서는 사용하지 않음)*/
    boolean invrt;
#endif
    int absamt;

    absamt = abs(incamt);
    /* Protection amount is typically larger than damage or to-hit;
       reduce magnitude by a third in order to stretch modifier ranges
       (small:1..5, moderate:6..10, large:11..19, huge:20+) */
#if 0 /*KR: 원본*/
    if (!strcmp(inctyp, "defense"))
        absamt = (absamt * 2) / 3;

    if (absamt <= 3)
        modif = "small";
    else if (absamt <= 6)
        modif = "moderate";
    else if (absamt <= 12)
        modif = "large";
    else
        modif = "huge";

    modif = !incamt ? "no" : an(modif); /* ("no" case shouldn't happen) */
    bonus = (incamt >= 0) ? "bonus" : "penalty";
    /* "bonus <foo>" (to hit) vs "<bar> bonus" (damage, defense) */
    invrt = strcmp(inctyp, "to hit") ? TRUE : FALSE;

    Sprintf(outbuf, "%s %s %s", modif, invrt ? inctyp : bonus,
            invrt ? bonus : inctyp);
#else /*KR: KRNethack 맞춤 번역 (복잡한 영어 어순 로직 제거 및 한국어화)*/
    /* 이 함수를 호출할 때 "방어"라는 문자열을 넘겨줄 것임 */
    if (!strcmp(inctyp, "방어"))
        absamt = (absamt * 2) / 3;

    if (absamt <= 3)
        modif = "약간의";
    else if (absamt <= 6)
        modif = "적당한";
    else if (absamt <= 12)
        modif = "큰";
    else
        modif = "엄청난";

    bonus = (incamt > 0) ? "보너스" : "페널티";

    /* 한국어 어순: [능력치]에 [수식어] [보너스/페널티] */
    Sprintf(outbuf, "%s에 %s %s", inctyp, modif, bonus);
#endif
    if (final || wizard)
        Sprintf(eos(outbuf), " (%s%d)", (incamt > 0) ? "+" : "", incamt);

    return outbuf;
}

/* report half physical or half spell damage */
STATIC_OVL void
enlght_halfdmg(category, final)
int category;
int final;
{
    const char *category_name;
    char buf[BUFSZ];

    switch (category) {
    case HALF_PHDAM:
        /*KR category_name = "physical"; */
        category_name = "물리";
        break;
    case HALF_SPDAM:
        /*KR category_name = "spell"; */
        category_name = "주문";
        break;
    default:
        /*KR category_name = "unknown"; */
        category_name = "불명";
        break;
    }
#if 0 /*KR: 원본*/
    Sprintf(buf, " %s %s damage", (final || wizard) ? "half" : "reduced",
            category_name);
    enl_msg(You_, "take", "took", buf, from_what(category));
#else /*KR: KRNethack 맞춤 번역 */
    Sprintf(buf, "%s 데미지를 %s하여 ", category_name,
            (final || wizard) ? "반감" : "감소");
    enl_msg(You_, "받는다", "받았다", buf, from_what(category));
#endif
}

/* is hero actively using water walking capability on water (or lava)? */
STATIC_OVL boolean
walking_on_water()
{
    if (u.uinwater || Levitation || Flying)
        return FALSE;
    return (boolean) (Wwalking
                      && (is_pool(u.ux, u.uy) || is_lava(u.ux, u.uy)));
}

/* check whether hero is wearing something that player definitely knows
   confers the target property; item must have been seen and its type
   discovered but it doesn't necessarily have to be fully identified */
STATIC_OVL boolean
cause_known(propindx)
int propindx; /* index of a property which can be conveyed by worn item */
{
    register struct obj *o;
    long mask = W_ARMOR | W_AMUL | W_RING | W_TOOL;

    /* simpler than from_what()/what_gives(); we don't attempt to
       handle artifacts and we deliberately ignore wielded items */
    for (o = invent; o; o = o->nobj) {
        if (!(o->owornmask & mask))
            continue;
        if ((int) objects[o->otyp].oc_oprop == propindx
            && objects[o->otyp].oc_name_known && o->dknown)
            return TRUE;
    }
    return FALSE;
}

/* format a characteristic value, accommodating Strength's strangeness */
STATIC_OVL char *
attrval(attrindx, attrvalue, resultbuf)
int attrindx, attrvalue;
char resultbuf[]; /* should be at least [7] to hold "18/100\0" */
{
    if (attrindx != A_STR || attrvalue <= 18)
        Sprintf(resultbuf, "%d", attrvalue);
    else if (attrvalue > STR18(100)) /* 19 to 25 */
        Sprintf(resultbuf, "%d", attrvalue - 100);
    else /* simplify "18/ **" to be "18/100" */
        Sprintf(resultbuf, "18/%02d", attrvalue - 18);
    return resultbuf;
}

void
enlightenment(mode, final)
int mode;  /* BASICENLIGHTENMENT | MAGICENLIGHTENMENT (| both) */
int final; /* ENL_GAMEINPROGRESS:0, ENL_GAMEOVERALIVE, ENL_GAMEOVERDEAD */
{
    char buf[BUFSZ], tmpbuf[BUFSZ];

    en_win = create_nhwindow(NHW_MENU);
    en_via_menu = !final;
    if (en_via_menu)
        start_menu(en_win);

    Strcpy(tmpbuf, plname);
    *tmpbuf = highc(*tmpbuf); /* same adjustment as bottom line */
    /* as in background_enlightenment, when poly'd we need to use the saved
       gender in u.mfemale rather than the current you-as-monster gender */
#if 0 /*KR: 원본*/
    Sprintf(buf, "%s the %s's attributes:", tmpbuf,
            ((Upolyd ? u.mfemale : flags.female) && urole.name.f)
                ? urole.name.f
                : urole.name.m);
#else /*KR: KRNethack 맞춤 번역 (어순 변경: 직업 + 이름) */
    Sprintf(buf, "%s %s의 능력치:",
            ((Upolyd ? u.mfemale : flags.female) && urole.name.f)
                ? urole.name.f
                : urole.name.m,
            tmpbuf);
#endif

    /* title */
    enlght_out(buf); /* "Conan the Archeologist's attributes:" */
    /* background and characteristics; ^X or end-of-game disclosure */
    if (mode & BASICENLIGHTENMENT) {
        /* role, race, alignment, deities, dungeon level, time, experience */
        background_enlightenment(mode, final);
        /* hit points, energy points, armor class, gold */
        basics_enlightenment(mode, final);
        /* strength, dexterity, &c */
        characteristics_enlightenment(mode, final);
    }
    /* expanded status line information, including things which aren't
       included there due to space considerations--such as obvious
       alternative movement indicators (riding, levitation, &c), and
       various troubles (turning to stone, trapped, confusion, &c);
       shown for both basic and magic enlightenment */
    status_enlightenment(mode, final);
    /* remaining attributes; shown for potion,&c or wizard mode and
       explore mode ^X or end of game disclosure */
    if (mode & MAGICENLIGHTENMENT) {
        /* intrinsics and other traditional enlightenment feedback */
        attributes_enlightenment(mode, final);
    }

    if (!en_via_menu) {
        display_nhwindow(en_win, TRUE);
    } else {
        menu_item *selected = 0;

        end_menu(en_win, (char *) 0);
        if (select_menu(en_win, PICK_NONE, &selected) > 0)
            free((genericptr_t) selected);
        en_via_menu = FALSE;
    }
    destroy_nhwindow(en_win);
    en_win = WIN_ERR;
}

/*ARGSUSED*/
/* display role, race, alignment and such to en_win */
STATIC_OVL void
background_enlightenment(unused_mode, final)
int unused_mode UNUSED;
int final;
{
    const char *role_titl, *rank_titl;
    int innategend, difgend, difalgn;
    char buf[BUFSZ], tmpbuf[BUFSZ];

    /* note that if poly'd, we need to use u.mfemale instead of flags.female
       to access hero's saved gender-as-human/elf/&c rather than current one */
    innategend = (Upolyd ? u.mfemale : flags.female) ? 1 : 0;
    role_titl = (innategend && urole.name.f) ? urole.name.f : urole.name.m;
    rank_titl = rank_of(u.ulevel, Role_switch, innategend);

    enlght_out(""); /* separator after title */
    /*KR enlght_out("Background:"); */
    enlght_out("배경 정보:");

    /* if polymorphed, report current shape before underlying role;
       will be repeated as first status: "you are transformed" and also
       among various attributes: "you are in beast form" (after being
       told about lycanthropy) or "you are polymorphed into <a foo>"
       (with countdown timer appended for wizard mode); we really want
       the player to know he's not a samurai at the moment... */
    if (Upolyd) {
        struct permonst *uasmon = youmonst.data;

        tmpbuf[0] = '\0';
        /* here we always use current gender, not saved role gender */
        if (!is_male(uasmon) && !is_female(uasmon) && !is_neuter(uasmon))
#if 0 /*KR: 원본*/
            Sprintf(tmpbuf, "%s ", genders[flags.female ? 1 : 0].adj);
        Sprintf(buf, "%sin %s%s form", !final ? "currently " : "", tmpbuf,
                uasmon->mname);
        you_are(buf, "");
#else /*KR: KRNethack 맞춤 번역*/
            Sprintf(tmpbuf, "%s ", genders[flags.female ? 1 : 0].adj);
        {
            /* 몬스터 이름을 한글로 가져오기 위한 사전 함수 선언 (C89 에러
             * 방지를 위해 중괄호로 묶음) */
            extern char *get_kr_name(const char *);
            Sprintf(buf, "%s%s%s의 모습", !final ? "현재 " : "", tmpbuf,
                    get_kr_name(uasmon->mname));
            you_are(buf, "");
        }
#endif
    }

    /* report role; omit gender if it's redundant (eg, "female priestess") */
    tmpbuf[0] = '\0';
    if (!urole.name.f
        && ((urole.allow & ROLE_GENDMASK) == (ROLE_MALE | ROLE_FEMALE)
            || innategend != flags.initgend))
        Sprintf(tmpbuf, "%s ", genders[innategend].adj);
    buf[0] = '\0';
    if (Upolyd)
#if 0 /*KR:T*/
        Strcpy(buf, "actually "); /* "You are actually a ..." */
#else
        Strcpy(buf, "실제로는"); /* "당신은 실제로는 ..." */
#endif
    if (!strcmpi(rank_titl, role_titl)) {
        /* omit role when rank title matches it */
#if 0 /*KR:T (랭크와 직업이 다를 때) */
        Sprintf(eos(buf), "%s, level %d %s%s", an(rank_titl), u.ulevel,
                tmpbuf, urace.noun);
    } else {
        Sprintf(eos(buf), "%s, a level %d %s%s %s", an(rank_titl), u.ulevel,
                tmpbuf, urace.adj, role_titl);
#else
        /* 예: 레벨 5의 여성 인간 기사 */
        Sprintf(eos(buf), "레벨 %d의 %s%s %s", u.ulevel, tmpbuf, urace.adj,
                role_titl);
    } else {
        /* 예: 레벨 5의 여성 엘프 레인저인 탐색꾼 */
        Sprintf(eos(buf), "레벨 %d의 %s%s %s인 %s", u.ulevel, tmpbuf,
                urace.adj, role_titl, rank_titl);
#endif
    }
    you_are(buf, "");

    /* report alignment (bypass you_are() in order to omit ending period);
       adverb is used to distinguish between temporary change (helm of opp.
       alignment), permanent change (one-time conversion), and original */
#if 0 /*KR: 원본*/
    Sprintf(buf, " %s%s%s, %son a mission for %s",
            You_, !final ? are : were,
            align_str(u.ualign.type),
            /* helm of opposite alignment (might hide conversion) */
            (u.ualign.type != u.ualignbase[A_CURRENT])
               /* what's the past tense of "currently"? if we used "formerly"
                  it would sound like a reference to the original alignment */
               ? (!final ? "currently " : "temporarily ")
               /* permanent conversion */
               : (u.ualign.type != u.ualignbase[A_ORIGINAL])
                  /* and what's the past tense of "now"? certainly not "then"
                     in a context like this...; "belatedly" == weren't that
                     way sooner (in other words, didn't start that way) */
                  ? (!final ? "now " : "belatedly ")
                  /* atheist (ignored in very early game) */
                  : (!u.uconduct.gnostic && moves > 1000L)
                     ? "nominally "
                     /* lastly, normal case */
                     : "",
            u_gname());
#else /*KR: KRNethack 맞춤 번역*/
    Sprintf(buf, "당신은 %s이며, %s%s 위한 임무를 수행하고 %s.",
            align_str(u.ualign.type),
            /* helm of opposite alignment (반대 성향의 투구 착용 시) */
            (u.ualign.type != u.ualignbase[A_CURRENT]) ? "일시적으로 "
            /* permanent conversion (영구적인 성향 전환 시) */
            : (u.ualign.type != u.ualignbase[A_ORIGINAL]) ? "현재 "
            /* atheist (무신론자 - 극초반엔 무시됨) */
            : (!u.uconduct.gnostic && moves > 1000L)
                ? "명목상 "
                /* lastly, normal case (기본 상태) */
                : "",
            append_josa(u_gname(), "을"), !final ? iru : ita);
#endif
    enlght_out(buf);
    /* show the rest of this game's pantheon (finishes previous sentence)
       [appending "also Moloch" at the end would allow for straightforward
       trailing "and" on all three aligned entries but looks too verbose] */
#if 0 /*KR:T*/
    Sprintf(buf, " who %s opposed by", !final ? "is" : "was");
    if (u.ualign.type != A_LAWFUL)
        Sprintf(eos(buf), " %s (%s) and", align_gname(A_LAWFUL),
                align_str(A_LAWFUL));
    if (u.ualign.type != A_NEUTRAL)
        Sprintf(eos(buf), " %s (%s)%s", align_gname(A_NEUTRAL),
                align_str(A_NEUTRAL),
                (u.ualign.type != A_CHAOTIC) ? " and" : "");
    if (u.ualign.type != A_CHAOTIC)
        Sprintf(eos(buf), " %s (%s)", align_gname(A_CHAOTIC),
                align_str(A_CHAOTIC));
    Strcat(buf, "."); /* terminate sentence */
#else /* (새로운 문장으로 분리) */
    Strcpy(buf, "당신은 ");
    if (u.ualign.type != A_LAWFUL)
        Sprintf(eos(buf), "%s(%s) 및 ", align_gname(A_LAWFUL),
                align_str(A_LAWFUL));
    if (u.ualign.type != A_NEUTRAL)
        Sprintf(eos(buf), "%s(%s)%s", align_gname(A_NEUTRAL),
                align_str(A_NEUTRAL),
                (u.ualign.type != A_CHAOTIC) ? " 및 " : "");
    if (u.ualign.type != A_CHAOTIC)
        Sprintf(eos(buf), "%s(%s)", align_gname(A_CHAOTIC),
                align_str(A_CHAOTIC));
    /* 항상 2명의 적대 신이 존재하므로 "신들과" 로 묶어서 마무리 */
    Sprintf(eos(buf), " 신들과 대립하고 %s.", !final ? iru : ita);
#endif
    enlght_out(buf);

    /* show original alignment,gender,race,role if any have been changed;
       giving separate message for temporary alignment change bypasses need
       for tricky phrasing otherwise necessitated by possibility of having
       helm of opposite alignment mask a permanent alignment conversion */
    difgend = (innategend != flags.initgend);
    difalgn = (((u.ualign.type != u.ualignbase[A_CURRENT]) ? 1 : 0)
               + ((u.ualignbase[A_CURRENT] != u.ualignbase[A_ORIGINAL])
                  ? 2 : 0));
    if (difalgn & 1) { /* have temporary alignment so report permanent one */
        /*KR Sprintf(buf, "actually %s", align_str(u.ualignbase[A_CURRENT])); */
        Sprintf(buf, "실제로는 %s", align_str(u.ualignbase[A_CURRENT]));
#if 0 /*KR: 원본*/
        you_are(buf, "");
#else /* (상태/과거 시제 분리) */
        enl_msg(buf, "이다", "이었다", "", "");
#endif
        difalgn &= ~1; /* suppress helm from "started out <foo>" message */
    }
    if (difgend || difalgn) { /* sex change or perm align change or both */
#if 0 /*KR: 원본*/
        Sprintf(buf, " You started out %s%s%s.",
                difgend ? genders[flags.initgend].adj : "",
                (difgend && difalgn) ? " and " : "",
                difalgn ? align_str(u.ualignbase[A_ORIGINAL]) : "");
#else /*KR: KRNethack 맞춤 번역*/
        Sprintf(buf, "당신은 처음에는 %s%s%s 상태로 시작했다.",
                difgend ? genders[flags.initgend].adj : "",
                (difgend && difalgn) ? "이고 " : "",
                difalgn ? align_str(u.ualignbase[A_ORIGINAL]) : "");
#endif
        enlght_out(buf);
    }

    /* As of 3.6.2: dungeon level, so that ^X really has all status info as
       claimed by the comment below; this reveals more information than
       the basic status display, but that's one of the purposes of ^X;
       similar information is revealed by #overview; the "You died in
       <location>" given by really_done() is more rudimentary than this */
    *buf = *tmpbuf = '\0';
    if (In_endgame(&u.uz)) {
        int egdepth = observable_depth(&u.uz);

        (void) endgamelevelname(tmpbuf, egdepth);
#if 0 /*KR: 원본*/
        Sprintf(buf, "in the endgame, on the %s%s",
                !strncmp(tmpbuf, "Plane", 5) ? "Elemental " : "", tmpbuf);
#else /*KR: KRNethack 맞춤 번역*/
        Sprintf(buf, "최종 시련의 %s에", tmpbuf);
#endif
    } else if (Is_knox(&u.uz)) {
        /* this gives away the fact that the knox branch is only 1 level */
        /*KR Sprintf(buf, "on the %s level", dungeons[u.uz.dnum].dname); */
        Sprintf(buf, "%s에", dungeons[u.uz.dnum].dname);
        /* TODO? maybe phrase it differently when actually inside the fort,
           if we're able to determine that (not trivial) */
    } else {
        char dgnbuf[QBUFSZ];

        Strcpy(dgnbuf, dungeons[u.uz.dnum].dname);
#if 0 /*KR: 원본 (The ~ dgnbuf는 그냥 없는 처리) */
        if (!strncmpi(dgnbuf, "The ", 4))
            *dgnbuf = lowc(*dgnbuf);
        Sprintf(tmpbuf, "level %d",
                In_quest(&u.uz) ? dunlev(&u.uz) : depth(&u.uz));
#else /*KR: KRNethack 맞춤 번역 (층수/계층 구분)*/
        if (In_quest(&u.uz)) {
            Sprintf(tmpbuf, "제%d계층", dunlev(&u.uz));
        } else {
            Sprintf(tmpbuf, "지하 %d층", depth(&u.uz));
        }
#endif
        /* TODO? maybe extend this bit to include various other automatic
           annotations from the dungeon overview code */
#if 0 /*KR: 원본*/
        if (Is_rogue_level(&u.uz))
            Strcat(tmpbuf, ", a primitive area");
        else if (Is_bigroom(&u.uz) && !Blind)
            Strcat(tmpbuf, ", a very big room");
        Sprintf(buf, "in %s, on %s", dgnbuf, tmpbuf);
#else
        if (Is_rogue_level(&u.uz))
            Strcat(tmpbuf, ", 원시적인 구역");
        else if (Is_bigroom(&u.uz) && !Blind)
            Strcat(tmpbuf, ", 아주 거대한 방");
        Sprintf(buf, "%s %s에", dgnbuf, tmpbuf);
#endif
    }
#if 0 /*KR: 원본*/
    you_are(buf, "");
#else /*KR: ("~에 있다" 로 마무리) */
    enl_msg(You_, "있다", "있었다", buf, "");
#endif

    /* this is shown even if the 'time' option is off */
    if (moves == 1L) {
#if 0 /*KR*/
        you_have("just started your adventure", "");
#else
        enlght_line(You_, "", "모험을 시작한 지 얼마 되지 않았다", "");
#endif
    } else {
        /* 'turns' grates on the nerves in this context... */
        /*KR Sprintf(buf, "the dungeon %ld turn%s ago", moves, plur(moves)); */
        Sprintf(buf, "%ld턴 전에 미궁에 들어섰다", moves);
        /* same phrasing for current and final: "entered" is unconditional */
#if 0 /*KR*/
        enlght_line(You_, "entered ", buf, "");
#else
        enlght_line(You_, "", buf, "");
#endif
    }

    /* for gameover, these have been obtained in really_done() so that they
       won't vary if user leaves a disclosure prompt or --More-- unanswered
       long enough for the dynamic value to change between then and now */
#if 0 /*KR:T*/
    if (final ? iflags.at_midnight : midnight()) {
        enl_msg("It ", "is ", "was ", "the midnight hour", "");
    } else if (final ? iflags.at_night : night()) {
        enl_msg("It ", "is ", "was ", "nighttime", "");
#else
    if (final ? iflags.at_midnight : midnight()) {
        enl_msg("시간대는 깊은 밤", "이다", "이었다", "", "");
    } else if (final ? iflags.at_night : night()) {
        enl_msg("시간대는 밤", "이다", "이었다", "", "");
#endif
    }
    /* other environmental factors */
    if (flags.moonphase == FULL_MOON || flags.moonphase == NEW_MOON) {
        /* [This had "tonight" but has been changed to "in effect".
           There is a similar issue to Friday the 13th--it's the value
           at the start of the current session but that session might
           have dragged on for an arbitrary amount of time.  We want to
           report the values that currently affect play--or affected
           play when game ended--rather than actual outside situation.] */
#if 0 /*KR:T*/
        Sprintf(buf, "a %s moon in effect%s",
                (flags.moonphase == FULL_MOON) ? "full"
                : (flags.moonphase == NEW_MOON) ? "new"
                  /* showing these would probably just lead to confusion
                     since they have no effect on game play... */
                  : (flags.moonphase < FULL_MOON) ? "first quarter"
                    : "last quarter",
                /* we don't have access to 'how' here--aside from survived
                   vs died--so settle for general platitude */
                final ? " when your adventure ended" : "");
        enl_msg("There ", "is ", "was ", buf, "");
#else /* (모험을 끝냈을 때) 하늘에는 (보름~하현)달이 떠 있다(있었다) */
        Sprintf(buf, "%s%s달",
                /* we don't have access to 'how' here--aside from survived
                   vs died--so settle for general platitude */
                final ? "모험을 끝냈을 때 하늘에는 " : "하늘에는 ",
                (flags.moonphase == FULL_MOON)  ? "보름"
                : (flags.moonphase == NEW_MOON) ? "그믐"
                /* showing these would probably just lead to confusion
                   since they have no effect on game play... */
                : (flags.moonphase < FULL_MOON) ? "상현"
                                                : "하현");
        enl_msg(buf, "이 떠 있다", "이 떠 있었다", "", "");
#endif
    }
    if (flags.friday13) {
        /* let player know that friday13 penalty is/was in effect;
           we don't say "it is/was Friday the 13th" because that was at
           the start of the session and it might be past midnight (or
           days later if the game has been paused without save/restore),
           so phrase this similar to the start up message */
#if 0 /*KR:T*/
        Sprintf(buf, " Bad things %s on Friday the 13th.",
                !final ? "can happen"
                : (final == ENL_GAMEOVERALIVE) ? "could have happened"
                  /* there's no may to tell whether -1 Luck made a
                     difference but hero has died... */
                  : "happened");
#else
        Sprintf(buf, "13일의 금요일에는 좋지 않은 일이 %s．",
                !final ? "일어날 수 있습니다"
                : (final == ENL_GAMEOVERALIVE)
                    ? "일어났을 수도 있습니다"
                    /* there's no may to tell whether -1 Luck made a
                       difference but hero has died... */
                    : "일어났습니다");
#endif
        enlght_out(buf);
    }

    if (!Upolyd) {
        int ulvl = (int) u.ulevel;
        /* [flags.showexp currently does not matter; should it?] */

        /* experience level is already shown above */
#if 0 /*KR: 원본*/
        Sprintf(buf, "%-1ld experience point%s", u.uexp, plur(u.uexp));
        /* TODO?
         * Remove wizard-mode restriction since patient players can
         * determine the numbers needed without resorting to spoilers
         * (even before this started being disclosed for 'final';
         * just enable 'showexp' and look at normal status lines
         * after drinking gain level potions or eating wraith corpses
         * or being level-drained by vampires).
         */
        if (ulvl < 30 && (final || wizard)) {
            long nxtlvl = newuexp(ulvl), delta = nxtlvl - u.uexp;

            Sprintf(eos(buf), ", %ld %s%sneeded %s level %d",
                    delta, (u.uexp > 0) ? "more " : "",
                    /* present tense=="needed", past tense=="were needed" */
                    !final ? "" : (delta == 1L) ? "was " : "were ",
                    /* "for": grammatically iffy but less likely to wrap */
                    (ulvl < 18) ? "to attain" : "for", (ulvl + 1));
        }
        you_have(buf, "");
#else /*KR: KRNethack 맞춤 번역 (복잡한 영어 조건문 제거)*/
        Sprintf(buf, "경험치 %-1ld 포인트", u.uexp);
        if (ulvl < 30 && (final || wizard)) {
            long nxtlvl = newuexp(ulvl), delta = nxtlvl - u.uexp;
            Sprintf(eos(buf), " (레벨 %d까지 %ld 포인트 %s)", (ulvl + 1),
                    delta, !final ? "남음" : "남았었음");
        }
        you_have(buf, "");
        /* 결과: "당신은 경험치 150 포인트 (레벨 5까지 20 포인트 남음)을(를)
         * 가지고 있다." */
#endif
    }
#ifdef SCORE_ON_BOTL
    if (flags.showscore) {
        /* describes what's shown on status line, which is an approximation;
           only show it here if player has the 'showscore' option enabled */
#if 0 /*KR: 원본*/
        Sprintf(buf, "%ld%s", botl_score(),
                !final ? "" : " before end-of-game adjustments");
        enl_msg("Your score ", "is ", "was ", buf, "");
#else /*KR: 결과: "당신의 점수는 게임 종료 조정 전 5000점이었다." */
        Sprintf(buf, "%s%ld점", !final ? "" : "게임 종료 조정 전 ",
                botl_score());
        enl_msg("당신의 점수는 ", "이다", "이었다", buf, "");
#endif
    }
#endif
}

/* hit points, energy points, armor class -- essential information which
   doesn't fit very well in other categories */
/*ARGSUSED*/
STATIC_OVL void
basics_enlightenment(mode, final)
int mode UNUSED;
int final;
{
#if 0 /*KR: 원본 (한국어에서는 사용하지 않음)*/
    static char Power[] = "energy points (spell power)";
#endif
    char buf[BUFSZ];
    int pw = u.uen, hp = (Upolyd ? u.mh : u.uhp),
        pwmax = u.uenmax, hpmax = (Upolyd ? u.mhmax : u.uhpmax);

    enlght_out(""); /* separator after background */
    /*KR enlght_out("Basics:"); */
    enlght_out("기본 정보:");

    if (hp < 0)
        hp = 0;
    /* "1 out of 1" rather than "all" if max is only 1; should never happen */
#if 0 /*KR: 원본*/
    if (hp == hpmax && hpmax > 1)
        Sprintf(buf, "all %d hit points", hpmax);
    else
        Sprintf(buf, "%d out of %d hit point%s", hp, hpmax, plur(hpmax));
#else /*KR: KRNethack 맞춤 번역 (영어의 단복수/상태 조건문 단순화)*/
    Sprintf(buf, "%d 체력 (최대: %d)", hp, hpmax);
#endif
    you_have(buf, "");

    /* low max energy is feasible, so handle couple of extra special cases */
#if 0 /*KR: 원본*/
    if (pwmax == 0 || (pw == pwmax && pwmax == 2)) /* both: "all 2" is silly */
        Sprintf(buf, "%s %s", !pwmax ? "no" : "both", Power);
    else if (pw == pwmax && pwmax > 2)
        Sprintf(buf, "all %d %s", pwmax, Power);
    else
        Sprintf(buf, "%d out of %d %s", pw, pwmax, Power);
#else /*KR: KRNethack 맞춤 번역*/
    Sprintf(buf, "%d 마력 (최대: %d)", pw, pwmax);
#endif
    you_have(buf, "");

    if (Upolyd) {
        switch (mons[u.umonnum].mlevel) {
        case 0:
            /* status line currently being explained shows "HD:0" */
            /*KR Strcpy(buf, "0 hit dice (actually 1/2)"); */
            Strcpy(buf, "HD0 (실제로는 1/2)");
            break;
        case 1:
            /*KR Strcpy(buf, "1 hit die"); */
            Strcpy(buf, "HD1");
            break;
        default:
            /*KR Sprintf(buf, "%d hit dice", mons[u.umonnum].mlevel); */
            Sprintf(buf, "HD%d", mons[u.umonnum].mlevel);
            break;
        }
        you_have(buf, "");
    }

    Sprintf(buf, "%d", u.uac);
    /*KR enl_msg("Your armor class ", "is ", "was ", buf, ""); */
    enl_msg("당신의 AC는 ", "이다", "이었다", buf, "");

    /* gold; similar to doprgold(#seegold) but without shop billing info;
       same amount as shown on status line which ignores container contents */
    {
        /*KR static const char Your_wallet[] = "Your wallet "; */
        static const char Your_wallet[] = "당신의 지갑";
        long umoney = money_cnt(invent);

#if 0 /*KR: 원본*/
        if (!umoney) {
            enl_msg(Your_wallet, "is ", "was ", "empty", "");
        } else {
            Sprintf(buf, "%ld %s", umoney, currency(umoney));
            enl_msg(Your_wallet, "contains ", "contained ", buf, "");
#else /*KR: KRNethack 맞춤 번역 */
        if (!umoney) {
            enl_msg(Your_wallet, " 텅 비어 있다", " 텅 비어 있었다", "은",
                    "");
        } else {
            char coinbuf[BUFSZ];
            /* 금액과 단위를 먼저 합침 (예: "61 조크미드") */
            Sprintf(coinbuf, "%ld %s", umoney, currency(umoney));
            /* 합친 단어에 자연스러운 조사를 붙임 (예: "61 조크미드가") */
            Sprintf(buf, "에는 %s", append_josa(coinbuf, "이"));

            /* 동사 앞에 공백을 넣어서 띄어쓰기가 무시되는 것을 원천 차단 ("
             * 들어 있다") */
            enl_msg(Your_wallet, " 들어 있다", " 들어 있었다", buf, "");
#endif
        }
    }

    if (flags.pickup) {
        char ocl[MAXOCLASSES + 1];

#if 0 /*KR: 원본 (ON 출력을 뒤로 미룸)*/
        Strcpy(buf, "on");
#endif
        oc_to_str(flags.pickup_types, ocl);
#if 0 /*KR: 원본*/
        Sprintf(eos(buf), " for %s%s%s",
                *ocl ? "'" : "", *ocl ? ocl : "all types", *ocl ? "'" : "");
#else /*KR: KRNethack 맞춤 번역*/
        Sprintf(buf, "%s%s%s", *ocl ? "'" : "", *ocl ? ocl : "모든 종류",
                *ocl ? "'" : "");
#endif
        if (flags.pickup_thrown && *ocl) /* *ocl: don't show if 'all types' */
            /*KR Strcat(buf, " plus thrown"); */
            Strcat(buf, " 및 던진 물건");
        if (apelist)
            /*KR Strcat(buf, ", with exceptions"); */
            Strcat(buf, "(예외 있음)");
#if 1 /*KR: 뒤로 미뤘던 ON(켜짐) 텍스트를 이제서야 출력*/
        Strcat(buf, "에 대해 켜져 ");
#endif
    } else {
        /*KR Strcpy(buf, "off"); */
        Strcpy(buf, "꺼져 ");
    }
#if 0 /*KR: 원본*/
    enl_msg("Autopickup ", "is ", "was ", buf, "");
#else /*KR: KRNethack 맞춤 번역 (enl_msg_kr 사용)*/
    /* 결과 예시: "자동 줍기 설정은 모든 종류에 대해 켜져 있다." */
    enl_msg_kr("자동 줍기 설정은 ", buf, "", "있다", "있었다");
#endif
}

/* characteristics: expanded version of bottom line strength, dexterity, &c */
STATIC_OVL void
characteristics_enlightenment(mode, final)
int mode;
int final;
{
    char buf[BUFSZ];

    enlght_out("");
    /*KR Sprintf(buf, "%s Characteristics:", !final ? "Current" : "Final"); */
    Sprintf(buf, "%s 특성:", !final ? "현재" : "최종");
    enlght_out(buf);

    /* bottom line order */
    one_characteristic(mode, final, A_STR); /* strength */
    one_characteristic(mode, final, A_DEX); /* dexterity */
    one_characteristic(mode, final, A_CON); /* constitution */
    one_characteristic(mode, final, A_INT); /* intelligence */
    one_characteristic(mode, final, A_WIS); /* wisdom */
    one_characteristic(mode, final, A_CHA); /* charisma */
}

/* display one attribute value for characteristics_enlightenment() */
STATIC_OVL void
one_characteristic(mode, final, attrindx)
int mode, final, attrindx;
{
    extern const char *const attrname[]; /* attrib.c */
    boolean hide_innate_value = FALSE, interesting_alimit;
    int acurrent, abase, apeak, alimit;
    const char *paren_pfx;
    char subjbuf[BUFSZ], valubuf[BUFSZ], valstring[32];

    /* being polymorphed or wearing certain cursed items prevents
       hero from reliably tracking changes to characteristics so
       we don't show base & peak values then; when the items aren't
       cursed, hero could take them off to check underlying values
       and we show those in such case so that player doesn't need
       to actually resort to doing that */
    if (Upolyd) {
        hide_innate_value = TRUE;
    } else if (Fixed_abil) {
        if (stuck_ring(uleft, RIN_SUSTAIN_ABILITY)
            || stuck_ring(uright, RIN_SUSTAIN_ABILITY))
            hide_innate_value = TRUE;
    }
    switch (attrindx) {
    case A_STR:
        if (uarmg && uarmg->otyp == GAUNTLETS_OF_POWER && uarmg->cursed)
            hide_innate_value = TRUE;
        break;
    case A_DEX:
        break;
    case A_CON:
        if (uwep && uwep->oartifact == ART_OGRESMASHER && uwep->cursed)
            hide_innate_value = TRUE;
        break;
    case A_INT:
        if (uarmh && uarmh->otyp == DUNCE_CAP && uarmh->cursed)
            hide_innate_value = TRUE;
        break;
    case A_WIS:
        if (uarmh && uarmh->otyp == DUNCE_CAP && uarmh->cursed)
            hide_innate_value = TRUE;
        break;
    case A_CHA:
        break;
    default:
        return; /* impossible */
    };
    /* note: final disclosure includes MAGICENLIGHTENTMENT */
    if ((mode & MAGICENLIGHTENMENT) && !Upolyd)
        hide_innate_value = FALSE;

    acurrent = ACURR(attrindx);
    (void) attrval(attrindx, acurrent, valubuf); /* Sprintf(valubuf,"%d",) */
    /*KR Sprintf(subjbuf, "Your %s ", attrname[attrindx]); */
    Sprintf(subjbuf, "당신의 %s ", append_josa(attrname[attrindx], "은"));

    if (!hide_innate_value) {
        /* show abase, amax, and/or attrmax if acurr doesn't match abase
           (a magic bonus or penalty is in effect) or abase doesn't match
           amax (some points have been lost to poison or exercise abuse
           and are restorable) or attrmax is different from normal human
           (while game is in progress; trying to reduce dependency on
           spoilers to keep track of such stuff) or attrmax was different
           from abase (at end of game; this attribute wasn't maxed out) */
        abase = ABASE(attrindx);
        apeak = AMAX(attrindx);
        alimit = ATTRMAX(attrindx);
        /* criterium for whether the limit is interesting varies */
        interesting_alimit =
            final ? TRUE /* was originally `(abase != alimit)' */
                  : (alimit != (attrindx != A_STR ? 18 : STR18(100)));
        /*KR paren_pfx = final ? " (" : " (current; "; */
        paren_pfx = final ? " (" : " (현재; ";
        if (acurrent != abase) {
#if 0 /*KR: 원본*/
            Sprintf(eos(valubuf), "%sbase:%s", paren_pfx,
                    attrval(attrindx, abase, valstring));
#else
            Sprintf(eos(valubuf), "%s기본: %s", paren_pfx,
                    attrval(attrindx, abase, valstring));
#endif
            paren_pfx = ", ";
        }
        if (abase != apeak) {
#if 0 /*KR: 원본*/
            Sprintf(eos(valubuf), "%speak:%s", paren_pfx,
                    attrval(attrindx, apeak, valstring));
#else
            Sprintf(eos(valubuf), "%s최대: %s", paren_pfx,
                    attrval(attrindx, apeak, valstring));
#endif
            paren_pfx = ", ";
        }
        if (interesting_alimit) {
#if 0 /*KR: 원본*/
            Sprintf(eos(valubuf), "%s%slimit:%s", paren_pfx,
                    /* more verbose if exceeding 'limit' due to magic bonus */
                    (acurrent > alimit) ? "innate " : "",
                    attrval(attrindx, alimit, valstring));
#else
            Sprintf(
                eos(valubuf), "%s%s상한: %s", paren_pfx,
                /* 마법 보너스 등으로 한도를 초과했을 때 '본래의' 한도 표시 */
                (acurrent > alimit) ? "본래의 " : "",
                attrval(attrindx, alimit, valstring));
#endif
            /* paren_pfx = ", "; */
        }
        if (acurrent != abase || abase != apeak || interesting_alimit)
            Strcat(valubuf, ")");
    }
#if 0 /*KR: 원본*/
    enl_msg(subjbuf, "is ", "was ", valubuf, "");
#else /*KR: KRNethack 맞춤 번역 (가독성을 위한 래퍼 매크로 활용) */
    enl_msg_kr(subjbuf, valubuf, "", "이다", "이었다");
#endif
}

/* status: selected obvious capabilities, assorted troubles */
STATIC_OVL void
status_enlightenment(mode, final)
int mode;
int final;
{
    boolean magic = (mode & MAGICENLIGHTENMENT) ? TRUE : FALSE;
    int cap, wtype;
    char buf[BUFSZ], youtoo[BUFSZ];
    boolean Riding = (u.usteed
                      /* if hero dies while dismounting, u.usteed will still
                         be set; we want to ignore steed in that situation */
                      && !(final == ENL_GAMEOVERDEAD
                          /*KR && !strcmp(killer.name, "riding accident"))); */
                           && !strcmp(killer.name, "승마 사고로")));
    const char *steedname = (!Riding ? (char *) 0
                      : x_monnam(u.usteed,
                                 u.usteed->mtame ? ARTICLE_YOUR : ARTICLE_THE,
                                 (char *) 0,
                                 (SUPPRESS_SADDLE | SUPPRESS_HALLUCINATION),
                                 FALSE));

    /*\
     * Status (many are abbreviated on bottom line; others are or
     *     should be discernible to the hero hence to the player)
    \*/
    enlght_out(""); /* separator after title or characteristics */
    /*KR enlght_out(final ? "Final Status:" : "Current Status:"); */
    enlght_out(final ? "최종 상태:" : "현재 상태:");

    Strcpy(youtoo, You_);
    /* not a traditional status but inherently obvious to player; more
       detail given below (attributes section) for magic enlightenment */
    if (Upolyd) {
#if 0 /*KR: 원본*/
        Strcpy(buf, "transformed");
        if (ugenocided())
            Sprintf(eos(buf), " and %s %s inside",
                    final ? "felt" : "feel", udeadinside());
        you_are(buf, "");
#else
        Strcpy(buf, "변신한 상태");
        if (ugenocided())
            
            Sprintf(eos(buf), "이며, 속이 %s 느낌", udeadinside());
        enl_msg_kr(You_, buf, "", "이다", "이었다");
#endif
    }
    /* not a trouble, but we want to display riding status before maybe
       reporting steed as trapped or hero stuck to cursed saddle */
    if (Riding) {
#if 0 /*KR: 원본*/
        Sprintf(buf, "riding %s", steedname);
        you_are(buf, "");
        Sprintf(eos(youtoo), "and %s ", steedname);
#else /*KR: KRNethack 맞춤 번역*/
        Sprintf(buf, "%s(을)를 타고 ", steedname);
        enl_msg_kr(You_, buf, "", "있다", "있었다");
        Sprintf(youtoo, "당신과 %s(은)는 ", steedname);
#endif
    }
    /* other movement situations that hero should always know */
    if (Levitation) {
        if (Lev_at_will && magic)
#if 0 /*KR: 원본*/
            you_are("levitating, at will", "");
        else
            enl_msg(youtoo, are, were, "levitating", from_what(LEVITATION));
#else /*KR: KRNethack 맞춤 번역*/
            enl_msg_kr(youtoo, "자유자재로 부유하고 ", "", "있다", "있었다");
        else
            enl_msg_kr(youtoo, "부유하고 ", from_what(LEVITATION), "있다",
                       "있었다");
#endif
    } else if (Flying) { /* can only fly when not levitating */
#if 0                    /*KR: 원본*/
        enl_msg(youtoo, are, were, "flying", from_what(FLYING));
#else
        enl_msg_kr(youtoo, "날고 ", from_what(FLYING), "있다", "있었다");
#endif
    }
    if (Underwater) {
#if 0 /*KR: 원본*/
        you_are("underwater", "");
    } else if (u.uinwater) {
        you_are(Swimming ? "swimming" : "in water", from_what(SWIMMING));
#else
        enl_msg_kr(You_, "물속에 ", "", "있다", "있었다");
    } else if (u.uinwater) {
        enl_msg_kr(You_, Swimming ? "수영하고 " : "물에 빠져 ",
                   from_what(SWIMMING), "있다", "있었다");
#endif
    } else if (walking_on_water()) {
        /* show active Wwalking here, potential Wwalking elsewhere */
#if 0 /*KR: 원본*/
        Sprintf(buf, "walking on %s",
                is_pool(u.ux, u.uy) ? "water"
                : is_lava(u.ux, u.uy) ? "lava"
                  : surface(u.ux, u.uy)); /* catchall; shouldn't happen */
        you_are(buf, from_what(WWALKING));
#else
        Sprintf(buf, "%s 위를 걷고 ",
                is_pool(u.ux, u.uy) ? "물"
                : is_lava(u.ux, u.uy) ? "용암"
                    : surface(u.ux, u.uy)); /* catchall; shouldn't happen */
        enl_msg_kr(You_, buf, from_what(WWALKING), "있다", "있었다");
#endif
    }
    if (Upolyd && (u.uundetected || U_AP_TYPE != M_AP_NOTHING))
        youhiding(TRUE, final);

    /* internal troubles, mostly in the order that prayer ranks them */
    if (Stoned) {
        if (final && (Stoned & I_SPECIAL))
#if 0 /*KR: 원본*/
            enlght_out(" You turned into stone.");
        else
            you_are("turning to stone", "");
#else /*KR: KRNethack 맞춤 번역*/
            enlght_out(" 당신은 돌이 되었다.");
        else
            enl_msg_kr(You_, "돌이 되어가고 ", "", "있다", "있었다");
#endif
    }
    if (Slimed) {
        if (final && (Slimed & I_SPECIAL))
#if 0 /*KR: 원본*/
            enlght_out(" You turned into slime.");
        else
            you_are("turning into slime", "");
#else /*KR: KRNethack 맞춤 번역*/
            enlght_out(" 당신은 슬라임이 되었다.");
        else
            enl_msg_kr(You_, "슬라임이 되어가고 ", "", "있다", "있었다");
#endif
    }
    if (Strangled) {
        if (u.uburied) {
#if 0 /*KR: 원본*/
            you_are("buried", "");
#else /*KR: KRNethack 맞춤 번역*/
            enl_msg_kr(You_, "파묻혀 ", "", "있다", "있었다");
#endif
        } else {
            if (final && (Strangled & I_SPECIAL)) {
                /*KR enlght_out(" You died from strangulation."); */
                enlght_out(" 당신은 질식사했다.");
            } else {
#if 0 /*KR: 원본*/
                Strcpy(buf, "being strangled");
                if (wizard)
                    Sprintf(eos(buf), " (%ld)", (Strangled & TIMEOUT));
                you_are(buf, from_what(STRANGLED));
#else /*KR: KRNethack 맞춤 번역*/
                Strcpy(buf, "목이 졸리고");
                if (wizard)
                    Sprintf(eos(buf), " (%ld)", (Strangled & TIMEOUT));
                Strcat(buf, " ");
                enl_msg_kr(You_, buf, from_what(STRANGLED), "있다", "있었다");
#endif
            }
        }
    }
    if (Sick) {
        /* the two types of sickness are lumped together; hero can be
           afflicted by both but there is only one timeout; botl status
           puts TermIll before FoodPois and death due to timeout reports
           terminal illness if both are in effect, so do the same here */
        if (final && (Sick & I_SPECIAL)) {
#if 0 /*KR: 원본*/
            Sprintf(buf, " %sdied from %s.", You_, /* has trailing space */
                    (u.usick_type & SICK_NONVOMITABLE)
                    ? "terminal illness" : "food poisoning");
#else /*KR: KRNethack 맞춤 번역*/
            /* You_ 변수 안에 이미 "당신은 "이라는 공백이 포함됨 */
            Sprintf(buf, " %s%s 죽었다.", You_,
                    (u.usick_type & SICK_NONVOMITABLE) ? "불치병으로"
                                                       : "식중독으로");
#endif
            enlght_out(buf);
        } else {
            /* unlike death due to sickness, report the two cases separately
               because it is possible to cure one without curing the other */
            if (u.usick_type & SICK_NONVOMITABLE)
#if 0 /*KR: 원본*/
                you_are("terminally sick from illness", "");
#else
                enl_msg_kr(You_, "불치병에 걸려 ", "", "있다", "있었다");
#endif
            if (u.usick_type & SICK_VOMITABLE)
#if 0 /*KR: 원본*/
                you_are("terminally sick from food poisoning", "");
#else
                enl_msg_kr(You_, "식중독에 걸려 ", "", "있다", "있었다");
#endif
        }
    }
    if (Vomiting)
#if 0 /*KR: 원본*/
        you_are("nauseated", "");
#else
        enl_msg_kr(You_, "구역질이 ", "", "난다", "났다");
#endif
    if (Stunned)
#if 0 /*KR: 원본*/
        you_are("stunned", "");
#else
        enl_msg_kr(You_, "어질어질한 상태", "", "이다", "이었다");
#endif
    if (Confusion)
#if 0 /*KR: 원본*/
        you_are("confused", "");
#else
        enl_msg_kr(You_, "혼란스러운 상태", "", "이다", "이었다");
#endif
    if (Hallucination)
#if 0 /*KR: 원본*/
        you_are("hallucinating", "");
#else
        enl_msg_kr(You_, "환각 상태", "", "이다", "이었다");
#endif
    if (Blind) {
        /* from_what() (currently wizard-mode only) checks !haseyes()
           before u.uroleplay.blind, so we should too */
#if 0 /*KR: 원본*/
        Sprintf(buf, "%s blind",
                !haseyes(youmonst.data) ? "innately"
                : u.uroleplay.blind ? "permanently"
                  /* better phrasing desperately wanted... */
                  : Blindfolded_only ? "deliberately"
                    : "temporarily");
#else /*KR: KRNethack 맞춤 번역*/
        Sprintf(buf, "%s 눈이 먼 상태",
                !haseyes(youmonst.data) ? "선천적으로"
                : u.uroleplay.blind     ? "영구적으로"
                /* better phrasing desperately wanted... */
                : Blindfolded_only ? "고의적으로"
                                   : "일시적으로");
#endif
        if (wizard && (Blinded & TIMEOUT) != 0L
            && !u.uroleplay.blind && haseyes(youmonst.data))
            Sprintf(eos(buf), " (%ld)", (Blinded & TIMEOUT));
        /* !haseyes: avoid "you are innately blind innately" */
        you_are(buf, !haseyes(youmonst.data) ? "" : from_what(BLINDED));
    }
    if (Deaf)
#if 0 /*KR: 원본*/
        you_are("deaf", from_what(DEAF));
#else /*KR: KRNethack 맞춤 번역*/
        enl_msg_kr(You_, "귀가 들리지 않는 상태", from_what(DEAF), "이다",
                   "이었다");
#endif

    /* external troubles, more or less */
    if (Punished) {
        if (uball) {
#if 0 /*KR: 원본*/
            Sprintf(buf, "chained to %s", ansimpleoname(uball));
#else
            Sprintf(buf, "%s에 사슬로 묶여 ", ansimpleoname(uball));
#endif
        } else {
            impossible("Punished without uball?");
#if 0 /*KR: 원본*/
            Strcpy(buf, "punished");
#else
            Strcpy(buf, "벌을 받고 ");
#endif
        }
#if 0 /*KR: 원본*/
        you_are(buf, "");
#else
        enl_msg_kr(You_, buf, "", "있다", "있었다");
#endif
    }
    if (u.utrap) {
        char predicament[BUFSZ];
        struct trap *t;
        boolean anchored = (u.utraptype == TT_BURIEDBALL);

        if (anchored) {
#if 0 /*KR: 원본*/
            Strcpy(predicament, "tethered to something buried");
#else
            Strcpy(predicament, "파묻힌 무언가에 묶여 ");
#endif
        } else if (u.utraptype == TT_INFLOOR || u.utraptype == TT_LAVA) {
#if 0 /*KR: 원본*/
            Sprintf(predicament, "stuck in %s", the(surface(u.ux, u.uy)));
#else
            Sprintf(predicament, "%s에 빠져 ", surface(u.ux, u.uy));
#endif
        } else {
#if 0 /*KR: 원본*/
            Strcpy(predicament, "trapped");
            if ((t = t_at(u.ux, u.uy)) != 0)
                Sprintf(eos(predicament), " in %s",
                        an(defsyms[trap_to_defsym(t->ttyp)].explanation));
#else
            predicament[0] = '\0';
            if ((t = t_at(u.ux, u.uy)) != 0)
                Sprintf(predicament, "%s에 ",
                        defsyms[trap_to_defsym(t->ttyp)].explanation);
            Strcat(predicament, "걸려 ");
#endif
        }
        if (u.usteed) { /* not `Riding' here */
#if 0                   /*KR: 원본*/
            Sprintf(buf, "%s%s ", anchored ? "you and " : "", steedname);
            *buf = highc(*buf);
            enl_msg(buf, (anchored ? "are " : "is "),
                    (anchored ? "were " : "was "), predicament, "");
#else
            /* 말과 함께 함정/무거운 쇠구슬에 걸렸을 때의 주어 처리 */
            Sprintf(buf, "%s%s(은)는 ", anchored ? "당신과 " : "", steedname);
            enl_msg_kr(buf, predicament, "", "있다", "있었다");
#endif
        } else
#if 0 /*KR: 원본*/
            you_are(predicament, "");
#else
            enl_msg_kr(You_, predicament, "", "있다", "있었다");
#endif
    } /* (u.utrap) */
    if (u.uswallow) {
#if 0 /*KR: 원본*/
        Sprintf(buf, "swallowed by %s", a_monnam(u.ustuck));
        if (wizard)
            Sprintf(eos(buf), " (%u)", u.uswldtim);
        you_are(buf, "");
#else
        Sprintf(buf, "%s에게 삼켜져 ", a_monnam(u.ustuck));
        if (wizard)
            Sprintf(eos(buf), " (%u)", u.uswldtim);
        enl_msg_kr(You_, buf, "", "있다", "있었다");
#endif
    } else if (u.ustuck) {
#if 0 /*KR: 원본*/
        Sprintf(buf, "%s %s",
                (Upolyd && sticks(youmonst.data)) ? "holding" : "held by",
                a_monnam(u.ustuck));
        you_are(buf, "");
#else
        /* 내가 몬스터를 붙잡고 있는지, 몬스터에게 붙잡힌 건지 구분 */
        Sprintf(buf, "%s%s ", a_monnam(u.ustuck),
                (Upolyd && sticks(youmonst.data)) ? "(을)를 붙잡고"
                                                  : "에게 붙잡혀");
        enl_msg_kr(You_, buf, "", "있다", "있었다");
#endif
    }
    if (Riding) {
        struct obj *saddle = which_armor(u.usteed, W_SADDLE);

        if (saddle && saddle->cursed) {
#if 0 /*KR: 원본*/
            Sprintf(buf, "stuck to %s %s", s_suffix(steedname),
                    simpleonames(saddle));
            you_are(buf, "");
#else
            Sprintf(buf, "%s의 %s에 들러붙어 ", steedname,
                    simpleonames(saddle));
            enl_msg_kr(You_, buf, "", "있다", "있었다");
#endif
        }
    }
    if (Wounded_legs) {
        /* when mounted, Wounded_legs applies to steed rather than to
           hero; we only report steed's wounded legs in wizard mode */
        if (u.usteed) { /* not `Riding' here */
            if (wizard && steedname) {
                Strcpy(buf, steedname);
                *buf = highc(*buf);
#if 0 /*KR: 원본*/
                enl_msg(buf, " has", " had", " wounded legs", "");
#else /*KR: KRNethack 맞춤 번역*/
                enl_msg_kr(buf, " 다리를 다친 상태", "", "이다", "이었다");
#endif
            }
        } else {
#if 0 /*KR: 원본*/
            Sprintf(buf, "wounded %s", makeplural(body_part(LEG)));
            you_have(buf, "");
#else
            Sprintf(buf, "다친 %s(을)를", makeplural(body_part(LEG)));
            enl_msg_kr(You_, buf, "", "가지고 있다", "가지고 있었다");
#endif
        }
    }
    if (Glib) {
#if 0 /*KR: 원본*/
        Sprintf(buf, "slippery %s", fingers_or_gloves(TRUE));
        if (wizard)
            Sprintf(eos(buf), " (%ld)", (Glib & TIMEOUT));
        you_have(buf, "");
#else
        Sprintf(buf, "미끄러운 %s(을)를", fingers_or_gloves(TRUE));
        if (wizard)
            Sprintf(eos(buf), " (%ld)", (Glib & TIMEOUT));
        enl_msg_kr(You_, buf, "", "가지고 있다", "가지고 있었다");
#endif
    }
    if (Fumbling) {
        if (magic || cause_known(FUMBLING))
#if 0 /*KR: 원본*/
            enl_msg(You_, "fumble", "fumbled", "", from_what(FUMBLING));
#else
            enl_msg_kr(You_, "어설프게 ", from_what(FUMBLING), "행동한다",
                       "행동했다");
#endif
    }
    if (Sleepy) {
        if (magic || cause_known(SLEEPY)) {
#if 0 /*KR: 원본*/
            Strcpy(buf, from_what(SLEEPY));
            if (wizard)
                Sprintf(eos(buf), " (%ld)", (HSleepy & TIMEOUT));
            enl_msg("You ", "fall", "fell", " asleep uncontrollably", buf);
#else
            Strcpy(buf, "통제할 수 없이 ");
            if (wizard)
                Sprintf(eos(buf), " (%ld) ", (HSleepy & TIMEOUT));
            enl_msg_kr(You_, buf, from_what(SLEEPY), "잠든다", "잠들었다");
#endif
        }
    }
    /* hunger/nutrition */
    if (Hunger) {
        if (magic || cause_known(HUNGER))
#if 0 /*KR: 원본*/
            enl_msg(You_, "hunger", "hungered", " rapidly",
                    from_what(HUNGER));
#else
            enl_msg_kr(You_, "매우 빠르게 ", from_what(HUNGER), "배고파진다",
                       "배고파졌다");
#endif
    }
#if 0 /*KR: 원본*/
    Strcpy(buf, hu_stat[u.uhs]); /* hunger status; omitted if "normal" */
    mungspaces(buf);             /* strip trailing spaces */
    if (*buf) {
        *buf = lowc(*buf); /* override capitalization */
        if (!strcmp(buf, "weak"))
            Strcat(buf, " from severe hunger");
        else if (!strncmp(buf, "faint", 5)) /* fainting, fainted */
            Strcat(buf, " due to starvation");
        you_are(buf, "");
    }
#else
    /* hunger status; omitted if "normal" */
    /* hu_stat(배고픔 상태창 문자열)이 이미 번역되었다고 가정 */
    Strcpy(buf, hu_stat[u.uhs]);
    mungspaces(buf);
    if (*buf) {
        if (!strcmp(buf, "허약"))
            Strcat(buf, " (심각한 굶주림으로 인해)");
        else if (!strncmp(buf, "기절", 6)) /* 기절 직전, 기절함 등 */
            Strcat(buf, " (아사 직전이므로)");
        enl_msg_kr(You_, buf, "", "이다", "이었다");
    }
#endif

    /* encumbrance */
    if ((cap = near_capacity()) > UNENCUMBERED) {
#if 0 /*KR: 원본*/
        const char *adj = "?_?"; /* (should always get overridden) */

        Strcpy(buf, enc_stat[cap]);
        *buf = lowc(*buf);
        switch (cap) {
        case SLT_ENCUMBER:
            adj = "slightly";
            break; /* burdened */
        case MOD_ENCUMBER:
            adj = "moderately";
            break; /* stressed */
        case HVY_ENCUMBER:
            adj = "very";
            break; /* strained */
        case EXT_ENCUMBER:
            adj = "extremely";
            break; /* overtaxed */
        case OVERLOADED:
            adj = "not possible";
            break;
        }
        Sprintf(eos(buf), "; movement %s %s%s", !final ? "is" : "was", adj,
                (cap < OVERLOADED) ? " slowed" : "");
        you_are(buf, "");
#else
        const char *adj = "?_?";

        /* enc_stat(무게 상태창 문자열)이 이미 번역되었다고 가정 */
        Strcpy(buf, enc_stat[cap]);
        switch (cap) {
        case SLT_ENCUMBER:
            adj = "약간";
            break; /* 무거움 */
        case MOD_ENCUMBER:
            adj = "적당히";
            break; /* 힘겨움 */
        case HVY_ENCUMBER:
            adj = "매우";
            break; /* 벅참 */
        case EXT_ENCUMBER:
            adj = "극도로";
            break; /* 짓눌림 */
        case OVERLOADED:
            adj = "불가능";
            break;
        }
        Sprintf(eos(buf), " 상태 (이동이 %s %s)", adj,
                (cap < OVERLOADED) ? "느려짐" : "함");
        enl_msg_kr(You_, buf, "", "이다", "이었다");
#endif
    } else {
        /* last resort entry, guarantees Status section is non-empty
           (no longer needed for that purpose since weapon status added;
           still useful though) */
#if 0 /*KR: 원본*/
        you_are("unencumbered", "");
#else
        enl_msg_kr(You_, "짐이 가벼운 상태", "", "이다", "이었다");
#endif
    }

    /* report being weaponless; distinguish whether gloves are worn */
    if (!uwep) {
#if 0 /*KR: 원본*/
        you_are(uarmg ? "empty handed" /* gloves imply hands */
                      : humanoid(youmonst.data)
                         /* hands but no weapon and no gloves */
                         ? "bare handed"
                         /* alternate phrasing for paws or lack of hands */
                         : "not wielding anything",
                "");
#else
        enl_msg_kr(You_,
                   uarmg                     ? "맨손 상태"
                   : humanoid(youmonst.data) ? "맨주먹 상태"
                                             : "아무것도 쥐고 있지 않은 상태",
                   "", "이다", "이었다");
#endif
        /* two-weaponing implies hands (can't be polymorphed) and
           a weapon or wep-tool (not other odd stuff) in each hand */
    } else if (u.twoweap) {
#if 0 /*KR: 원본*/
        you_are("wielding two weapons at once", "");
#else
        enl_msg_kr(You_, "두 개의 무기를 동시에 쥐고 ", "", "있다", "있었다");
#endif
        /* report most weapons by their skill class (so a katana will be
           described as a long sword, for instance; mattock and hook are
           exceptions), or wielded non-weapon item by its object class */
    } else {
#if 0 /*KR: 원본*/
        const char *what = weapon_descr(uwep);

        if (!strcmpi(what, "armor") || !strcmpi(what, "food")
            || !strcmpi(what, "venom"))
            Sprintf(buf, "wielding some %s", what);
        else
            Sprintf(buf, "wielding %s",
                    (uwep->quan == 1L) ? an(what) : makeplural(what));
        you_are(buf, "");
#else
        /* get_kr_name을 사용해 무기 묘사를 한글로 받아옵니다. */
        extern char *get_kr_name(const char *);
        const char *what = get_kr_name(weapon_descr(uwep));

        /* 한국어는 복수형을 강제하지 않으므로 단순화 가능 */
        Sprintf(buf, "%s(을)를 장비하고 ", what);
        enl_msg_kr(You_, buf, "", "있다", "있었다");
#endif
    }
    /*
     * Skill with current weapon.  Might help players who've never
     * noticed #enhance or decided that it was pointless.
     *
     * TODO?  Maybe merge wielding line and skill line into one sentence.
     */
    if ((wtype = uwep_skill_type()) != P_NONE) {
        char sklvlbuf[20];
        int sklvl = P_SKILL(wtype);
        boolean hav = (sklvl != P_UNSKILLED && sklvl != P_SKILLED);

#if 0 /*KR: 원본*/
        if (sklvl == P_ISRESTRICTED)
            Strcpy(sklvlbuf, "no");
        else
            (void) lcase(skill_level_name(wtype, sklvlbuf));
        /* "you have no/basic/expert/master/grand-master skill with <skill>"
           or "you are unskilled/skilled in <skill>" */
        Sprintf(buf, "%s %s %s", sklvlbuf,
                hav ? "skill with" : "in", skill_name(wtype));
        if (can_advance(wtype, FALSE))
            Sprintf(eos(buf), " and %s that",
                    !final ? "can enhance" : "could have enhanced");
        if (hav)
            you_have(buf, "");
        else
            you_are(buf, "");
#else
        if (sklvl == P_ISRESTRICTED)
            Strcpy(sklvlbuf, "제한됨");
        else
            (void) lcase(skill_level_name(wtype, sklvlbuf));

        /* skill_name 한글화 가설 */
        extern char *get_kr_name(const char *);
        Sprintf(buf, "%s 숙련도가 %s 상태", get_kr_name(skill_name(wtype)),
                sklvlbuf);

        if (can_advance(wtype, FALSE))
            Sprintf(eos(buf), "이며 %s",
                    !final ? "숙련도를 올릴 수 있다"
                           : "숙련도를 올릴 수 있었다");
        else
            Sprintf(eos(buf), "%s", !final ? "이다" : "이었다");

        /* "당신은 단검 숙련도가 기본 상태이며 숙련도를 올릴 수 있다" */
        enl_msg_kr(You_, buf, "", "", ""); /* 이미 buf 안에 동사가 포함됨 */
#endif
    }
    /* report 'nudity' */
    if (!uarm && !uarmu && !uarmc && !uarms && !uarmg && !uarmf && !uarmh) {
#if 0 /*KR: 원본*/
        if (u.uroleplay.nudist)
            enl_msg(You_, "do", "did", " not wear any armor", "");
        else
            you_are("not wearing any armor", "");
#else
        if (u.uroleplay.nudist)
            enl_msg_kr(You_, "어떤 방어구도 입지 ", "", "않는다", "않았다");
        else
            enl_msg_kr(You_, "어떤 방어구도 입고 있지 ", "", "않다",
                       "않았다");
#endif
    }
}

/* attributes: intrinsics and the like, other non-obvious capabilities */
STATIC_OVL void
attributes_enlightenment(unused_mode, final)
int unused_mode UNUSED;
int final;
{
#if 0 /*KR: 원본 (한국어에서는 사용하지 않음)*/
    static NEARDATA const char if_surroundings_permitted[] =
        " if surroundings permitted";
#endif
    int ltmp, armpro;
    char buf[BUFSZ];

    /*\
     * Attributes
    \*/
    enlght_out("");
#if 0 /*KR: 원본*/
    enlght_out(final ? "Final Attributes:" : "Current Attributes:");
#else /*KR: KRNethack 맞춤 번역*/
    enlght_out(final ? "최종 능력치:" : "현재 능력치:");
#endif

    if (u.uevent.uhand_of_elbereth) {
#if 0 /*KR: 원본*/
        static const char *const hofe_titles[3] = { "the Hand of Elbereth",
                                                    "the Envoy of Balance",
                                                    "the Glory of Arioch" };
#else /*KR: KRNethack 맞춤 번역*/
        static const char *const hofe_titles[3] = { "엘베레스의 수호자",
                                                    "조화의 사자",
                                                    "아리오치의 영광" };
#endif
        you_are(hofe_titles[u.uevent.uhand_of_elbereth - 1], "");
    }

#if 0 /*KR: 원본*/
    Sprintf(buf, "%s", piousness(TRUE, "aligned"));
#else
    Sprintf(buf, "%s", piousness(TRUE, "신앙심"));
#endif
    if (u.ualign.record >= 0)
        you_are(buf, "");
    else
        you_have(buf, "");

    if (wizard) {
#if 0 /*KR: 원본*/
        Sprintf(buf, " %d", u.ualign.record);
        enl_msg("Your alignment ", "is", "was", buf, "");
#else
        Sprintf(buf, "당신의 성향치는 %d", u.ualign.record);
        enl_msg_kr(buf, "", "", "이다", "이었다");
#endif
    }

    /*** Resistances to troubles ***/
    if (Invulnerable)
#if 0 /*KR: 원본*/
        you_are("invulnerable", from_what(INVULNERABLE));
#else
        you_are("무적", from_what(INVULNERABLE));
#endif
    if (Antimagic)
#if 0 /*KR: 원본*/
        you_are("magic-protected", from_what(ANTIMAGIC));
#else
        you_have("마법 방어 능력", from_what(ANTIMAGIC));
#endif
    if (Fire_resistance)
#if 0 /*KR: 원본*/
        you_are("fire resistant", from_what(FIRE_RES));
#else
        you_have("불에 대한 내성", from_what(FIRE_RES));
#endif
    if (Cold_resistance)
#if 0 /*KR: 원본*/
        you_are("cold resistant", from_what(COLD_RES));
#else
        you_have("추위에 대한 내성", from_what(COLD_RES));
#endif
    if (Sleep_resistance)
#if 0 /*KR: 원본*/
        you_are("sleep resistant", from_what(SLEEP_RES));
#else
        you_have("수면에 대한 내성", from_what(SLEEP_RES));
#endif
    if (Disint_resistance)
#if 0 /*KR: 원본*/
        you_are("disintegration-resistant", from_what(DISINT_RES));
#else
        you_have("분해에 대한 내성", from_what(DISINT_RES));
#endif
    if (Shock_resistance)
#if 0 /*KR: 원본*/
        you_are("shock resistant", from_what(SHOCK_RES));
#else
        you_have("전격에 대한 내성", from_what(SHOCK_RES));
#endif
    if (Poison_resistance)
#if 0 /*KR: 원본*/
        you_are("poison resistant", from_what(POISON_RES));
#else
        you_have("독에 대한 내성", from_what(POISON_RES));
#endif
    if (Acid_resistance)
#if 0 /*KR: 원본*/
        you_are("acid resistant", from_what(ACID_RES));
#else
        you_have("산에 대한 내성", from_what(ACID_RES));
#endif
    if (Drain_resistance)
#if 0 /*KR: 원본*/
        you_are("level-drain resistant", from_what(DRAIN_RES));
#else
        you_have("레벨 흡수에 대한 내성", from_what(DRAIN_RES));
#endif
    if (Sick_resistance)
#if 0 /*KR: 원본*/
        you_are("immune to sickness", from_what(SICK_RES));
#else
        you_have("질병에 대한 면역", from_what(SICK_RES));
#endif
    if (Stone_resistance)
#if 0 /*KR: 원본*/
        you_are("petrification resistant", from_what(STONE_RES));
#else
        you_have("석화에 대한 내성", from_what(STONE_RES));
#endif
    if (Halluc_resistance)
#if 0 /*KR: 원본*/
        enl_msg(You_, "resist", "resisted", " hallucinations",
                from_what(HALLUC_RES));
#else
        you_have("환각에 대한 내성", from_what(HALLUC_RES));
#endif
    if (u.uedibility)
#if 0 /*KR: 원본*/
        you_can("recognize detrimental food", "");
#else
        you_can("유해한 음식을 식별", "");
#endif

    /*** Vision and senses ***/
    if (!Blind && (Blinded || !haseyes(youmonst.data)))
#if 0 /*KR: 원본*/
        you_can("see", from_what(-BLINDED)); /* Eyes of the Overworld */
#else
        you_can("사물을 볼", from_what(-BLINDED));
#endif
    if (See_invisible) {
        if (!Blind)
#if 0 /*KR: 원본*/
            enl_msg(You_, "see", "saw", " invisible", from_what(SEE_INVIS));
#else
            enl_msg_kr(You_, "투명한 것을 볼 ", from_what(SEE_INVIS),
                       "수 있다", "수 있었다");
#endif
        else
#if 0 /*KR: 원본*/
            enl_msg(You_, "will see", "would have seen",
                    " invisible when not blind", from_what(SEE_INVIS));
#else
            enl_msg_kr(You_, "눈이 멀지 않았다면 투명한 것을 볼 ",
                       from_what(SEE_INVIS), "수 있다", "수 있었다");
#endif
    }
    if (Blind_telepat)
#if 0 /*KR: 원본*/
        you_are("telepathic", from_what(TELEPAT));
#else /*KR: KRNethack 맞춤 번역*/
        you_have("텔레파시 능력", from_what(TELEPAT));
#endif
    if (Warning)
#if 0 /*KR: 원본*/
        you_are("warned", from_what(WARNING));
#else
        you_have("경계 능력", from_what(WARNING));
#endif
    if (Warn_of_mon && context.warntype.obj) {
#if 0 /*KR: 원본*/
        Sprintf(buf, "aware of the presence of %s",
                (context.warntype.obj & M2_ORC) ? "orcs"
                : (context.warntype.obj & M2_ELF) ? "elves"
                : (context.warntype.obj & M2_DEMON) ? "demons" : something);
        you_are(buf, from_what(WARN_OF_MON));
#else
        Sprintf(buf, "%s의 존재를 감지하는 능력",
                (context.warntype.obj & M2_ORC)     ? "오크"
                : (context.warntype.obj & M2_ELF)   ? "엘프"
                : (context.warntype.obj & M2_DEMON) ? "악마"
                                                    : "무언가");
        you_have(buf, "");
#endif
    }
    if (Warn_of_mon && context.warntype.polyd) {
#if 0 /*KR: 원본*/
        Sprintf(buf, "aware of the presence of %s",
                ((context.warntype.polyd & (M2_HUMAN | M2_ELF))
                 == (M2_HUMAN | M2_ELF))
                    ? "humans and elves"
                    : (context.warntype.polyd & M2_HUMAN)
                          ? "humans"
                          : (context.warntype.polyd & M2_ELF)
                                ? "elves"
                                : (context.warntype.polyd & M2_ORC)
                                      ? "orcs"
                                      : (context.warntype.polyd & M2_DEMON)
                                            ? "demons"
                                            : "certain monsters");
        you_are(buf, "");
#else
        Sprintf(buf, "%s의 존재를 감지하는 능력",
                ((context.warntype.polyd & (M2_HUMAN | M2_ELF))
                 == (M2_HUMAN | M2_ELF))
                    ? "인간과 엘프"
                : (context.warntype.polyd & M2_HUMAN) ? "인간"
                : (context.warntype.polyd & M2_ELF)   ? "엘프"
                : (context.warntype.polyd & M2_ORC)   ? "오크"
                : (context.warntype.polyd & M2_DEMON) ? "악마"
                                                      : "특정 몬스터");
        you_have(buf, "");
#endif
    }
    if (Warn_of_mon && context.warntype.speciesidx >= LOW_PM) {
#if 0 /*KR: 원본*/
        Sprintf(buf, "aware of the presence of %s",
                makeplural(mons[context.warntype.speciesidx].mname));
        you_are(buf, from_what(WARN_OF_MON));
#else
        /* get_kr_name을 사용해 몬스터 이름을 한글로 받아옵니다. */
        extern char *get_kr_name(const char *);
        Sprintf(buf, "%s의 존재를 감지하는 능력",
                get_kr_name(mons[context.warntype.speciesidx].mname));
        you_have(buf, from_what(WARN_OF_MON));
#endif
    }
    if (Undead_warning)
#if 0 /*KR: 원본*/
        you_are("warned of undead", from_what(WARN_UNDEAD));
#else
        you_have("언데드에 대한 경계 능력", from_what(WARN_UNDEAD));
#endif
    if (Searching)
#if 0 /*KR: 원본*/
        you_have("automatic searching", from_what(SEARCHING));
#else
        you_have("자동 탐색 능력", from_what(SEARCHING));
#endif
    if (Clairvoyant)
#if 0 /*KR: 원본*/
        you_are("clairvoyant", from_what(CLAIRVOYANT));
#else
        you_have("천리안 능력", from_what(CLAIRVOYANT));
#endif
    else if ((HClairvoyant || EClairvoyant) && BClairvoyant) {
        Strcpy(buf, from_what(-CLAIRVOYANT));
#if 0 /*KR: 원본 (위험한 문자열 조작 로직 제거)*/
        if (!strncmp(buf, " because of ", 12))
            /* overwrite substring; strncpy doesn't add terminator */
            (void) strncpy(buf, " if not for ", 12);
        enl_msg(You_, "could be", "could have been", " clairvoyant", buf);
#else /*KR: KRNethack 맞춤 번역 (자연스럽고 안전한 의역)*/
        enl_msg_kr(You_, "천리안 능력이 차단된 상태", buf, "이다", "이었다");
#endif
    }
    if (Infravision)
#if 0 /*KR: 원본*/
        you_have("infravision", from_what(INFRAVISION));
#else
        you_have("적외선 시야", from_what(INFRAVISION));
#endif
    if (Detect_monsters)
#if 0 /*KR: 원본*/
        you_are("sensing the presence of monsters", "");
#else
        you_have("몬스터의 존재를 감지하는 능력", "");
#endif
    if (u.umconf)
#if 0 /*KR: 원본*/
        you_are("going to confuse monsters", "");
#else
        you_have("몬스터를 혼란시키는 능력", "");
#endif

    /*** Appearance and behavior ***/
    if (Adornment) {
        int adorn = 0;

        if (uleft && uleft->otyp == RIN_ADORNMENT)
            adorn += uleft->spe;
        if (uright && uright->otyp == RIN_ADORNMENT)
            adorn += uright->spe;
        /* the sum might be 0 (+0 ring or two which negate each other);
           that yields "you are charismatic" (which isn't pointless
           because it potentially impacts seduction attacks) */
#if 0 /*KR: 원본*/
        Sprintf(buf, "%scharismatic",
                (adorn > 0) ? "more " : (adorn < 0) ? "less " : "");
        you_are(buf, from_what(ADORNED));
#else
        Sprintf(buf, "매력이 %s 상태",
                (adorn > 0)   ? "증가한" : (adorn < 0) ? "감소한"
                                                       : "돋보이는");
        enl_msg_kr(You_, buf, from_what(ADORNED), "이다", "이었다");
#endif
    }
    if (Invisible)
#if 0 /*KR: 원본*/
        you_are("invisible", from_what(INVIS));
#else
        you_are("투명한 상태", from_what(INVIS));
#endif
    else if (Invis)
#if 0 /*KR: 원본*/
        you_are("invisible to others", from_what(INVIS));
#else
        you_are("타인에게 투명하게 보이는 상태", from_what(INVIS));
#endif
    /* ordinarily "visible" is redundant; this is a special case for
       the situation when invisibility would be an expected attribute */
    else if ((HInvis || EInvis) && BInvis)
#if 0 /*KR: 원본*/
        you_are("visible", from_what(-INVIS));
#else
        you_are("투명화가 차단된 상태", from_what(-INVIS));
#endif
    if (Displaced)
        /*KR you_are("displaced", from_what(DISPLACED)); */
        you_have("환영을 만드는 능력", from_what(DISPLACED));
    if (Stealth)
        /*KR you_are("stealthy", from_what(STEALTH)); */
        you_have("은밀하게 행동하는 능력", from_what(STEALTH));
    if (Aggravate_monster)
#if 0 /*KR: 원본*/
        enl_msg("You aggravate", "", "d", " monsters",
                from_what(AGGRAVATE_MONSTER));
#else
        enl_msg_kr(You_, "몬스터들을 자극하고 ", from_what(AGGRAVATE_MONSTER),
                   "있다", "있었다");
#endif
    if (Conflict)
        /*KR enl_msg("You cause", "", "d", " conflict", from_what(CONFLICT));
         */
        enl_msg_kr(You_, "분쟁을 일으키고 ", from_what(CONFLICT), "있다",
                   "있었다");

    /*** Transportation ***/
    if (Jumping)
        /*KR you_can("jump", from_what(JUMPING)); */
        you_can("도약할", from_what(JUMPING));
    if (Teleportation)
        /*KR you_can("teleport", from_what(TELEPORT)); */
        you_can("순간이동을 할", from_what(TELEPORT));
    if (Teleport_control)
        /*KR you_have("teleport control", from_what(TELEPORT_CONTROL)); */
        you_have("순간이동 제어 능력", from_what(TELEPORT_CONTROL));
    /* actively levitating handled earlier as a status condition */
    if (BLevitation) { /* levitation is blocked */
        long save_BLev = BLevitation;

        BLevitation = 0L;
        if (Levitation) {
            /* either trapped in the floor or inside solid rock
               (or both if chained to buried iron ball and have
               moved one step into solid rock somehow) */
#if 0 /*KR: 원본*/
            boolean trapped = (save_BLev & I_SPECIAL) != 0L,
                    terrain = (save_BLev & FROMOUTSIDE) != 0L;

            Sprintf(buf, "%s%s%s",
                    trapped ? " if not trapped" : "",
                    (trapped && terrain) ? " and" : "",
                    terrain ? if_surroundings_permitted : "");
            enl_msg(You_, "would levitate", "would have levitated", buf, "");
#else /*KR: KRNethack 맞춤 번역 (상황이 허락한다면 부유할 수 있음)*/
            enl_msg_kr(You_, "상황이 허락했다면 부유할 ", "", "수 있다",
                       "수 있었다");
#endif
        }
        BLevitation = save_BLev;
    }
    /* actively flying handled earlier as a status condition */
    if (BFlying) { /* flight is blocked */
        long save_BFly = BFlying;

        BFlying = 0L;
        if (Flying) {
#if 0 /*KR: 원본*/
            enl_msg(You_, "would fly", "would have flown",
                    /* wording quibble: for past tense, "hadn't been"
                       would sound better than "weren't" (and
                       "had permitted" better than "permitted"), but
                       "weren't" and "permitted" are adequate so the
                       extra complexity to handle that isn't worth it */
                    Levitation
                       ? " if you weren't levitating"
                       : (save_BFly == I_SPECIAL)
                          /* this is an oversimpliction; being trapped
                             might also be blocking levitation so flight
                             would still be blocked after escaping trap */
                          ? " if you weren't trapped"
                          : (save_BFly == FROMOUTSIDE)
                             ? if_surroundings_permitted
                             /* two or more of levitation, surroundings,
                                and being trapped in the floor */
                             : " if circumstances permitted",
                    "");
#else
            enl_msg_kr(You_, "날 ",
                       /* 번역: 부유 중이 아니었다면 / 갇혀 있지 않았다면 /
                          주변 환경이 허락했다면 / 상황이 허락했다면 */
                       Levitation                 ? "부유 중이 아니었다면 "
                       : (save_BFly == I_SPECIAL) ? "갇혀 있지 않았다면 "
                       : (save_BFly == FROMOUTSIDE)
                           ? "주변 환경이 허락했다면 "
                           : "상황이 허락했다면 ",
                       "수 있다", "수 있었다");
#endif
        }
        BFlying = save_BFly;
    }
    /* actively walking on water handled earlier as a status condition */
    if (Wwalking && !walking_on_water())
        /*KR you_can("walk on water", from_what(WWALKING)); */
        you_can("물 위를 걸을", from_what(WWALKING));
    /* actively swimming (in water but not under it) handled earlier */
    if (Swimming && (Underwater || !u.uinwater))
        /*KR you_can("swim", from_what(SWIMMING)); */
        you_can("수영할", from_what(SWIMMING));
    if (Breathless)
        /*KR you_can("survive without air", from_what(MAGICAL_BREATHING)); */
        you_can("공기 없이도 생존할", from_what(MAGICAL_BREATHING));
    else if (Amphibious)
        /*KR you_can("breathe water", from_what(MAGICAL_BREATHING)); */
        you_can("물속에서 호흡할", from_what(MAGICAL_BREATHING));
    if (Passes_walls)
        /*KR you_can("walk through walls", from_what(PASSES_WALLS)); */
        you_can("벽을 통과할", from_what(PASSES_WALLS));

    /*** Physical attributes ***/
    if (Regeneration)
        /*KR enl_msg("You regenerate", "", "d", "", from_what(REGENERATION)); */
        you_have("재생 능력", from_what(REGENERATION));
    if (Slow_digestion)
        /*KR you_have("slower digestion", from_what(SLOW_DIGESTION)); */
        you_have("느린 소화 능력", from_what(SLOW_DIGESTION));
    if (u.uhitinc)
        /*KR you_have(enlght_combatinc("to hit", u.uhitinc, final, buf), ""); */
        you_have(enlght_combatinc("명중률", u.uhitinc, final, buf), "");
    if (u.udaminc)
        /*KR you_have(enlght_combatinc("damage", u.udaminc, final, buf), ""); */
        you_have(enlght_combatinc("데미지", u.udaminc, final, buf), "");
    if (u.uspellprot || Protection) {
        int prot = 0;

        if (uleft && uleft->otyp == RIN_PROTECTION)
            prot += uleft->spe;
        if (uright && uright->otyp == RIN_PROTECTION)
            prot += uright->spe;
        if (HProtection & INTRINSIC)
            prot += u.ublessed;
        prot += u.uspellprot;
        if (prot)
            /*KR you_have(enlght_combatinc("defense", prot, final, buf), "");
             */
            you_have(enlght_combatinc("방어", prot, final, buf), "");
    }
    if ((armpro = magic_negation(&youmonst)) > 0) {
        /* magic cancellation factor, conferred by worn armor */
#if 0 /*KR: 원본*/
        static const char *const mc_types[] = {
            "" /*ordinary*/, "warded", "guarded", "protected",
        };
#else
        static const char *const mc_types[] = {
            "" /*ordinary*/,
            "마법을 튕겨내는 상태",
            "마법을 방어하는 상태",
            "마법으로부터 보호받는 상태",
        };
#endif
        /* sanity check */
        if (armpro >= SIZE(mc_types))
            armpro = SIZE(mc_types) - 1;
        you_are(mc_types[armpro], "");
    }
    if (Half_physical_damage)
        enlght_halfdmg(HALF_PHDAM, final);
    if (Half_spell_damage)
        enlght_halfdmg(HALF_SPDAM, final);
    /* polymorph and other shape change */
    if (Protection_from_shape_changers)
        /*KR you_are("protected from shape changers",
         * from_what(PROT_FROM_SHAPE_CHANGERS)); */
        you_are("변신 생물로부터 보호받는 상태",
                from_what(PROT_FROM_SHAPE_CHANGERS));
    if (Unchanging) {
        const char *what = 0;

        if (!Upolyd) /* Upolyd handled below after current form */
            /*KR you_can("not change from your current form",
             * from_what(UNCHANGING)); */
            you_can("현재의 모습에서 변하지 않을", from_what(UNCHANGING));
        /* blocked shape changes */
        if (Polymorph)
#if 0 /*KR: 원본*/
            what = !final ? "polymorph" : "have polymorphed";
#else
            what = !final ? "폴리모프를 해야" : "폴리모프를 했어야";
#endif
        else if (u.ulycn >= LOW_PM)
#if 0 /*KR: 원본*/
            what = !final ? "change shape" : "have changed shape";
#else
            what = !final ? "모습이 변해야" : "모습이 변했어야";
#endif
        if (what) {
#if 0 /*KR: 원본*/
            Sprintf(buf, "would %s periodically", what);
            /* omit from_what(UNCHANGING); too verbose */
            enl_msg(You_, buf, buf, " if not locked into your current form",
                    "");
#else /*KR: KRNethack 맞춤 번역 (구조 변경)*/
            Sprintf(buf, "주기적으로 %s 하지만, 현재 모습으로 고정되어 ",
                    what);
            enl_msg_kr(You_, buf, "", "있다", "있었다");
#endif
        }
    } else if (Polymorph) {
        /*KR you_are("polymorphing periodically", from_what(POLYMORPH)); */
        you_are("주기적으로 폴리모프하는 상태", from_what(POLYMORPH));
    }
    if (Polymorph_control)
        /*KR you_have("polymorph control", from_what(POLYMORPH_CONTROL)); */
        you_have("폴리모프 제어 능력", from_what(POLYMORPH_CONTROL));
    if (Upolyd
        && u.umonnum != u.ulycn
        /* if we've died from turning into slime, we're polymorphed
           right now but don't want to list it as a temporary attribute
           [we need a more reliable way to detect this situation] */
        && !(final == ENL_GAMEOVERDEAD && u.umonnum == PM_GREEN_SLIME
             && !Unchanging)) {
        /* foreign shape (except were-form which is handled below) */
#if 0 /*KR: 원본 (변수 호출 구조 변경)*/
        Sprintf(buf, "polymorphed into %s", an(youmonst.data->mname));
#else
        extern char *get_kr_name(const char *);
        Sprintf(buf, "%s(으)로 폴리모프된 상태",
                get_kr_name(youmonst.data->mname));
#endif
        if (wizard)
            Sprintf(eos(buf), " (%d)", u.mtimedone);
        you_are(buf, "");
    }
    if (lays_eggs(youmonst.data) && flags.female) /* Upolyd */
        /*KR you_can("lay eggs", ""); */
        you_can("산란", "");
    if (u.ulycn >= LOW_PM) {
        /* "you are a werecreature [in beast form]" */
#if 0 /*KR: 원본 (변수 호출 구조 변경)*/
        Strcpy(buf, an(mons[u.ulycn].mname));
        if (u.umonnum == u.ulycn) {
            Strcat(buf, " in beast form");
#else
        extern char *get_kr_name(const char *);
        Strcpy(buf, get_kr_name(mons[u.ulycn].mname));
        if (u.umonnum == u.ulycn) {
            Strcat(buf, " 야수 형태");
#endif
            if (wizard)
                Sprintf(eos(buf), " (%d)", u.mtimedone);
        }
        you_are(buf, "");
    }
    if (Unchanging && Upolyd) /* !Upolyd handled above */
        /*KR you_can("not change from your current form",
         * from_what(UNCHANGING)); */
        you_can("현재 모습에서 변하지 않을", from_what(UNCHANGING));
    if (Hate_silver)
        /*KR you_are("harmed by silver", ""); */
        you_are("은에 피해를 입는 상태", "");
    /* movement and non-armor-based protection */
    if (Fast)
#if 0 /*KR: 원본 (삼항 연산자 내부 텍스트 길이로 인한 분리)*/
        you_are(Very_fast ? "very fast" : "fast", from_what(FAST));
#else
        you_are(Very_fast ? "매우 빠른 상태" : "빠른 상태", from_what(FAST));
#endif
    if (Reflecting)
        /*KR you_have("reflection", from_what(REFLECTING)); */
        you_have("반사 능력", from_what(REFLECTING));
    if (Free_action)
        /*KR you_have("free action", from_what(FREE_ACTION)); */
        you_have("자유 행동 능력", from_what(FREE_ACTION));
    if (Fixed_abil)
        /*KR you_have("fixed abilities", from_what(FIXED_ABIL)); */
        you_have("능력치 유지 속성", from_what(FIXED_ABIL));
    if (Lifesaved)
#if 0 /*KR: 원본 (함수 변경 및 구조 변경)*/
        enl_msg("Your life ", "will be", "would have been", " saved", "");
#else
        enl_msg_kr("당신의 생명은 ", "구원받을 ", "", "것이다", "것이었다");
#endif

    /*** Miscellany ***/
    if (Luck) {
        ltmp = abs((int) Luck);
#if 0 /*KR: 원본*/
        Sprintf(buf, "%s%slucky",
                ltmp >= 10 ? "extremely " : ltmp >= 5 ? "very " : "",
                Luck < 0 ? "un" : "");
        if (wizard)
            Sprintf(eos(buf), " (%d)", Luck);
        you_are(buf, "");
#else
        Sprintf(buf, "%s%s 상태",
                ltmp >= 10  ? "극도로 "
                : ltmp >= 5 ? "매우 "
                            : "",
                Luck < 0 ? "불운한" : "운이 좋은");
        if (wizard)
            Sprintf(eos(buf), " (%d)", Luck);
        you_are(buf, "");
#endif
    } else if (wizard)
#if 0 /*KR: 원본*/
        enl_msg("Your luck ", "is", "was", " zero", "");
#else
        enl_msg_kr("당신의 운은 ", "0", "", "이다", "이었다");
#endif
    if (u.moreluck > 0)
        /*KR you_have("extra luck", ""); */
        you_have("추가적인 운", "");
    else if (u.moreluck < 0)
        /*KR you_have("reduced luck", ""); */
        you_have("감소된 운", "");
    if (carrying(LUCKSTONE) || stone_luck(TRUE)) {
        ltmp = stone_luck(FALSE);
#if 0 /*KR: 원본*/
        if (ltmp <= 0)
            enl_msg("Bad luck ", "does", "did", " not time out for you", "");
        if (ltmp >= 0)
            enl_msg("Good luck ", "does", "did", " not time out for you", "");
#else /*KR: KRNethack 맞춤 번역 (함수 변경)*/
        if (ltmp <= 0)
            enl_msg_kr("불운의 기한이 ", "사라지지 ", "", "않는다", "않았다");
        if (ltmp >= 0)
            enl_msg_kr("행운의 기한이 ", "사라지지 ", "", "않는다", "않았다");
#endif
    }

    if (u.ugangr) {
#if 0 /*KR: 원본*/
        Sprintf(buf, " %sangry with you",
                u.ugangr > 6 ? "extremely " : u.ugangr > 3 ? "very " : "");
        if (wizard)
            Sprintf(eos(buf), " (%d)", u.ugangr);
        enl_msg(u_gname(), " is", " was", buf, "");
#else
        Sprintf(buf, "당신에게 %s분노한 상태",
                u.ugangr > 6   ? "극도로 "
                : u.ugangr > 3 ? "매우 "
                               : "");
        if (wizard)
            Sprintf(eos(buf), " (%d)", u.ugangr);
        enl_msg_kr(u_gname(), buf, "", "이다", "이었다");
#endif
    } else {
        /*
         * We need to suppress this when the game is over, because death
         * can change the value calculated by can_pray(), potentially
         * resulting in a false claim that you could have prayed safely.
         */
        if (!final) {
#if 0 /*KR: 원본*/
            Sprintf(buf, "%ssafely pray", can_pray(FALSE) ? "" : "not ");
            if (wizard)
                Sprintf(eos(buf), " (%d)", u.ublesscnt);
            you_can(buf, "");
#else /*KR: KRNethack 맞춤 번역 (구조 및 함수 변경)*/
            Sprintf(buf, "안전하게 기도할 ");
            if (wizard)
                Sprintf(eos(buf), " (%d)", u.ublesscnt);
            enl_msg_kr(You_, buf, "", can_pray(FALSE) ? "수 있다" : "수 없다",
                       can_pray(FALSE) ? "수 있었다" : "수 없었다");
#endif
        }
    }

#ifdef DEBUG
    /* named fruit debugging (doesn't really belong here...); to enable,
       include 'fruit' in DEBUGFILES list (even though it isn't a file...) */
    if (wizard && explicitdebug("fruit")) {
        struct fruit *f;

        reorder_fruit(TRUE); /* sort by fruit index, from low to high;
                              * this modifies the ffruit chain, so could
                              * possibly mask or even introduce a problem,
                              * but it does useful sanity checking */
        for (f = ffruit; f; f = f->nextf) {
#if 0 /*KR: 원본*/
            Sprintf(buf, "Fruit #%d ", f->fid);
            enl_msg(buf, "is ", "was ", f->fname, "");
#else /*KR: KRNethack 맞춤 번역*/
            Sprintf(buf, "과일 #%d(은)는 ", f->fid);
            enl_msg_kr(buf, f->fname, "", "이다", "이었다");
#endif
        }
#if 0 /*KR: 원본*/
        enl_msg("The current fruit ", "is ", "was ", pl_fruit, "");
        Sprintf(buf, "%d", flags.made_fruit);
        enl_msg("The made fruit flag ", "is ", "was ", buf, "");
#else /*KR: KRNethack 맞춤 번역*/
        enl_msg_kr("현재 과일은 ", pl_fruit, "", "이다", "이었다");
        Sprintf(buf, "%d", flags.made_fruit);
        enl_msg_kr("만들어진 과일 플래그는 ", buf, "", "이다", "이었다");
#endif
    }
#endif

    {
        const char *p;

        buf[0] = '\0';
        if (final < 2) { /* still in progress, or quit/escaped/ascended */
#if 0                    /*KR: 원본*/
            p = "survived after being killed ";
#else
            p = "번 죽고 부활했다"; /* "N 번 죽고 부활했다"로 활용 */
#endif
            switch (u.umortality) {
            case 0:
#if 0 /*KR: 원본*/
                p = !final ? (char *) 0 : "survived";
#else
                p = !final ? (char *) 0 : "생존했다";
#endif
                break;
            case 1:
#if 0 /*KR: 원본*/
                Strcpy(buf, "once");
#else
                Strcpy(buf, "한");
#endif
                break;
            case 2:
#if 0 /*KR: 원본*/
                Strcpy(buf, "twice");
#else
                Strcpy(buf, "두");
#endif
                break;
            case 3:
#if 0 /*KR: 원본*/
                Strcpy(buf, "thrice");
#else
                Strcpy(buf, "세");
#endif
                break;
            default:
#if 0 /*KR: 원본*/
                Sprintf(buf, "%d times", u.umortality);
#else
                Sprintf(buf, "%d", u.umortality);
#endif
                break;
            }
        } else { /* game ended in character's death */
#if 0            /*KR: 원본*/
            p = "are dead";
#else
            p = "사망했다";
#endif
            switch (u.umortality) {
            case 0:
                impossible("dead without dying?");
            case 1:
                break; /* just "are dead" */
            default:
#if 0 /*KR: 원본*/
                Sprintf(buf, " (%d%s time!)", u.umortality,
                        ordin(u.umortality));
#else
                Sprintf(buf, " (%d번째 죽음!)", u.umortality);
#endif
                break;
            }
        }
        if (p) {
#if 0 /*KR: 원본 (함수 및 구조 변경)*/
            enl_msg(You_, "have been killed ", p, buf, "");
#else /*KR: KRNethack 맞춤 번역*/
            if (final < 2) {
                if (u.umortality == 0) {
                    /* "당신은 생존했다." */
                    enl_msg_kr(You_, p, "", "", "");
                } else {
                    /* "당신은 한 번 죽고 부활했다." */
                    Sprintf(buf + strlen(buf), " %s", p);
                    enl_msg_kr(You_, buf, "", "", "");
                }
            } else {
                if (u.umortality <= 1) {
                    /* "당신은 사망했다." */
                    enl_msg_kr(You_, p, "", "", "");
                } else {
                    /* "당신은 사망했다. (N번째 죽음!)" */
                    Sprintf(buf + strlen(buf), " %s", p);
                    enl_msg_kr(You_, buf, "", "", "");
                }
            }
#endif
        }
    }
}

#if 0  /* no longer used */
STATIC_DCL boolean NDECL(minimal_enlightenment);

/*
 * Courtesy function for non-debug, non-explorer mode players
 * to help refresh them about who/what they are.
 * Returns FALSE if menu cancelled (dismissed with ESC), TRUE otherwise.
 */
STATIC_OVL boolean
minimal_enlightenment()
{
    winid tmpwin;
    menu_item *selected;
    anything any;
    int genidx, n;
    char buf[BUFSZ], buf2[BUFSZ];
    static const char untabbed_fmtstr[] = "%-15s: %-12s";
    static const char untabbed_deity_fmtstr[] = "%-17s%s";
    static const char tabbed_fmtstr[] = "%s:\t%-12s";
    static const char tabbed_deity_fmtstr[] = "%s\t%s";
    static const char *fmtstr;
    static const char *deity_fmtstr;

    fmtstr = iflags.menu_tab_sep ? tabbed_fmtstr : untabbed_fmtstr;
    deity_fmtstr = iflags.menu_tab_sep ? tabbed_deity_fmtstr
                                       : untabbed_deity_fmtstr;
    any = zeroany;
    buf[0] = buf2[0] = '\0';
    tmpwin = create_nhwindow(NHW_MENU);
    start_menu(tmpwin);
/*KR add_menu(tmpwin, NO_GLYPH, &any, 0, 0, iflags.menu_headings, "Starting", FALSE); */
    add_menu(tmpwin, NO_GLYPH, &any, 0, 0, iflags.menu_headings,
             "시작", FALSE);

    /* Starting name, race, role, gender */
    /*KR Sprintf(buf, fmtstr, "name", plname); */
    Sprintf(buf, fmtstr, "이름", plname);
    add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, FALSE);
    /*KR Sprintf(buf, fmtstr, "race", urace.noun); */
    Sprintf(buf, fmtstr, "종족", urace.noun);
    add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, FALSE);
    /*KR Sprintf(buf, fmtstr, "role", (flags.initgend && urole.name.f) ? urole.name.f : urole.name.m); */
    Sprintf(buf, fmtstr, "직업",
            (flags.initgend && urole.name.f) ? urole.name.f : urole.name.m);
    add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, FALSE);
    /*KR Sprintf(buf, fmtstr, "gender", genders[flags.initgend].adj); */
    Sprintf(buf, fmtstr, "성별", genders[flags.initgend].adj);
    add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, FALSE);

    /* Starting alignment */
    /*KR Sprintf(buf, fmtstr, "alignment", align_str(u.ualignbase[A_ORIGINAL])); */
    Sprintf(buf, fmtstr, "성향", align_str(u.ualignbase[A_ORIGINAL]));
    add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, FALSE);

    /* Current name, race, role, gender */
    add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, "", FALSE);
    /*KR add_menu(tmpwin, NO_GLYPH, &any, 0, 0, iflags.menu_headings, "Current", FALSE); */
    add_menu(tmpwin, NO_GLYPH, &any, 0, 0, iflags.menu_headings,
             "현재", FALSE);
    /*KR Sprintf(buf, fmtstr, "race", Upolyd ? youmonst.data->mname : urace.noun); */
    Sprintf(buf, fmtstr, "종족", Upolyd ? youmonst.data->mname : urace.noun);
    add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, FALSE);
    if (Upolyd) {
        /*KR Sprintf(buf, fmtstr, "role (base)", (u.mfemale && urole.name.f) ? urole.name.f : urole.name.m); */
        Sprintf(buf, fmtstr, "직업 (기본)",
                (u.mfemale && urole.name.f) ? urole.name.f
                                            : urole.name.m);
        add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, FALSE);
    } else {
        /*KR Sprintf(buf, fmtstr, "role", (flags.female && urole.name.f) ? urole.name.f : urole.name.m); */
        Sprintf(buf, fmtstr, "직업",
                (flags.female && urole.name.f) ? urole.name.f
                                               : urole.name.m);
        add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, FALSE);
    }
    /* don't want poly_gender() here; it forces `2' for non-humanoids */
    genidx = is_neuter(youmonst.data) ? 2 : flags.female;
    /*KR Sprintf(buf, fmtstr, "gender", genders[genidx].adj); */
    Sprintf(buf, fmtstr, "성별", genders[genidx].adj);
    add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, FALSE);
    if (Upolyd && (int) u.mfemale != genidx) {
        /*KR Sprintf(buf, fmtstr, "gender (base)", genders[u.mfemale].adj); */
        Sprintf(buf, fmtstr, "성별 (기본)", genders[u.mfemale].adj);
        add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, FALSE);
    }

    /* Current alignment */
    /*KR Sprintf(buf, fmtstr, "alignment", align_str(u.ualign.type)); */
    Sprintf(buf, fmtstr, "성향", align_str(u.ualign.type));
    add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, FALSE);

    /* Deity list */
    add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, "", FALSE);
    /*KR add_menu(tmpwin, NO_GLYPH, &any, 0, 0, iflags.menu_headings, "Deities", FALSE); */
    add_menu(tmpwin, NO_GLYPH, &any, 0, 0, iflags.menu_headings,
             "모시는 신", FALSE);
#if 0  /*KR: 원본*/
    Sprintf(buf2, deity_fmtstr, align_gname(A_CHAOTIC),
            (u.ualignbase[A_ORIGINAL] == u.ualign.type
             && u.ualign.type == A_CHAOTIC)               ? " (s,c)"
                : (u.ualignbase[A_ORIGINAL] == A_CHAOTIC) ? " (s)"
                : (u.ualign.type   == A_CHAOTIC)          ? " (c)" : "");
#else
    Sprintf(buf2, deity_fmtstr, align_gname(A_CHAOTIC),
            (u.ualignbase[A_ORIGINAL] == u.ualign.type
             && u.ualign.type == A_CHAOTIC)               ? " (시작, 현재)"
                : (u.ualignbase[A_ORIGINAL] == A_CHAOTIC) ? " (시작)"
                : (u.ualign.type   == A_CHAOTIC)          ? " (현재)" : "");
#endif
    /*KR Sprintf(buf, fmtstr, "Chaotic", buf2); */
    Sprintf(buf, fmtstr, "혼돈", buf2);
    add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, FALSE);

#if 0 /*KR: 원본*/
    Sprintf(buf2, deity_fmtstr, align_gname(A_NEUTRAL),
            (u.ualignbase[A_ORIGINAL] == u.ualign.type
             && u.ualign.type == A_NEUTRAL)               ? " (s,c)"
                : (u.ualignbase[A_ORIGINAL] == A_NEUTRAL) ? " (s)"
                : (u.ualign.type   == A_NEUTRAL)          ? " (c)" : "");
#else
    Sprintf(buf2, deity_fmtstr, align_gname(A_NEUTRAL),
            (u.ualignbase[A_ORIGINAL] == u.ualign.type
             && u.ualign.type == A_NEUTRAL)               ? " (시작, 현재)"
                : (u.ualignbase[A_ORIGINAL] == A_NEUTRAL) ? " (시작)"
                : (u.ualign.type   == A_NEUTRAL)          ? " (현재)" : "");
#endif
    /*KR Sprintf(buf, fmtstr, "Neutral", buf2); */
    Sprintf(buf, fmtstr, "중립", buf2);
    add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, FALSE);

#if 0 /*KR: 원본*/
    Sprintf(buf2, deity_fmtstr, align_gname(A_LAWFUL),
            (u.ualignbase[A_ORIGINAL] == u.ualign.type
             && u.ualign.type == A_LAWFUL)                ? " (s,c)"
                : (u.ualignbase[A_ORIGINAL] == A_LAWFUL)  ? " (s)"
                : (u.ualign.type   == A_LAWFUL)           ? " (c)" : "");
#else
    Sprintf(buf2, deity_fmtstr, align_gname(A_LAWFUL),
            (u.ualignbase[A_ORIGINAL] == u.ualign.type
             && u.ualign.type == A_LAWFUL)                ? " (시작, 현재)"
                : (u.ualignbase[A_ORIGINAL] == A_LAWFUL)  ? " (시작)"
                : (u.ualign.type   == A_LAWFUL)           ? " (현재)" : "");
#endif
    /*KR Sprintf(buf, fmtstr, "Lawful", buf2); */
    Sprintf(buf, fmtstr, "질서", buf2);
    add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, FALSE);

    /*KR end_menu(tmpwin, "Base Attributes"); */
    end_menu(tmpwin, "기본 속성");
    n = select_menu(tmpwin, PICK_NONE, &selected);
    destroy_nhwindow(tmpwin);
    return (boolean) (n != -1);
}
#endif /*0*/

/* ^X command */
STATIC_PTR int
doattributes(VOID_ARGS)
{
    int mode = BASICENLIGHTENMENT;

    /* show more--as if final disclosure--for wizard and explore modes */
    if (wizard || discover)
        mode |= MAGICENLIGHTENMENT;

    enlightenment(mode, ENL_GAMEINPROGRESS);
    return 0;
}

void
youhiding(via_enlghtmt, msgflag)
boolean via_enlghtmt; /* englightment line vs topl message */
int msgflag;          /* for variant message phrasing */
{
    char *bp, buf[BUFSZ];

#if 0 /*KR: 원본*/
    Strcpy(buf, "hiding");
#else
    buf[0] = '\0'; /* KR: 한국어 어순을 위해 초기화 */
#endif
    if (U_AP_TYPE != M_AP_NOTHING) {
        /* mimic; hero is only able to mimic a strange object or gold
           or hallucinatory alternative to gold, so we skip the details
           for the hypothetical furniture and monster cases */
#if 0 /*KR: 원본*/
        bp = eos(strcpy(buf, "mimicking"));
        if (U_AP_TYPE == M_AP_OBJECT) {
            Sprintf(bp, " %s", an(simple_typename(youmonst.mappearance)));
        } else if (U_AP_TYPE == M_AP_FURNITURE) {
            Strcpy(bp, " something");
        } else if (U_AP_TYPE == M_AP_MONSTER) {
            Strcpy(bp, " someone");
        } else {
            ; /* something unexpected; leave 'buf' as-is */
        }
#else /*KR: KRNethack 맞춤 번역 (어순 재배치)*/
        if (U_AP_TYPE == M_AP_OBJECT) {
            extern char *get_kr_name(const char *);
            Strcpy(buf, get_kr_name(simple_typename(youmonst.mappearance)));
        } else if (U_AP_TYPE == M_AP_FURNITURE) {
            Strcpy(buf, "무언가");
        } else if (U_AP_TYPE == M_AP_MONSTER) {
            Strcpy(buf, "누군가");
        } else {
            ; /* something unexpected; leave 'buf' as-is */
        }
        if (buf[0])
            Strcat(buf, "의 모습으로 위장하고 ");
#endif
    } else if (u.uundetected) {
#if 0             /*KR: 원본*/
        bp = eos(buf); /* points past "hiding" */
        if (youmonst.data->mlet == S_EEL) {
            if (is_pool(u.ux, u.uy))
                Sprintf(bp, " in the %s", waterbody_name(u.ux, u.uy));
        } else if (hides_under(youmonst.data)) {
            struct obj *o = level.objects[u.ux][u.uy];

            if (o)
                Sprintf(bp, " underneath %s", ansimpleoname(o));
        } else if (is_clinger(youmonst.data) || Flying) {
            /* Flying: 'lurker above' hides on ceiling but doesn't cling */
            Sprintf(bp, " on the %s", ceiling(u.ux, u.uy));
        } else {
            /* on floor; is_hider() but otherwise not special: 'trapper' */
            if (u.utrap && u.utraptype == TT_PIT) {
                struct trap *t = t_at(u.ux, u.uy);

                Sprintf(bp, " in a %spit",
                        (t && t->ttyp == SPIKED_PIT) ? "spiked " : "");
            } else
                Sprintf(bp, " on the %s", surface(u.ux, u.uy));
        }
#else             /*KR: KRNethack 맞춤 번역*/
        bp = buf; /* 버퍼 시작점부터 덮어씀 */
        if (youmonst.data->mlet == S_EEL) {
            if (is_pool(u.ux, u.uy))
                Sprintf(bp, "%s 안에", waterbody_name(u.ux, u.uy));
        } else if (hides_under(youmonst.data)) {
            struct obj *o = level.objects[u.ux][u.uy];

            if (o)
                Sprintf(bp, "%s 아래에", ansimpleoname(o));
        } else if (is_clinger(youmonst.data) || Flying) {
            /* Flying: 'lurker above' hides on ceiling but doesn't cling */
            Sprintf(bp, "%s에", ceiling(u.ux, u.uy));
        } else {
            /* on floor; is_hider() but otherwise not special: 'trapper' */
            if (u.utrap && u.utraptype == TT_PIT) {
                struct trap *t = t_at(u.ux, u.uy);

                Sprintf(bp, "%s함정 속에",
                        (t && t->ttyp == SPIKED_PIT) ? "가시 " : "구덩이 ");
            } else
                Sprintf(bp, "%s에", surface(u.ux, u.uy));
        }
        Strcat(buf, " 숨어 ");
#endif
    } else {
#if 0 /*KR: 원본*/
        ; /* shouldn't happen; will result in generic "you are hiding" */
#else
        Strcpy(buf, "숨어 ");
#endif
    }

    if (via_enlghtmt) {
        int final = msgflag; /* 'final' is used by you_are() macro */

#if 0 /*KR: 원본 (함수 변경)*/
        you_are(buf, "");
#else
        enl_msg_kr(You_, buf, "", "있다", "있었다");
#endif
    } else {
        /* for dohide(), when player uses '#monster' command */
#if 0 /*KR: 원본 (구조 변경)*/
        You("are %s %s.", msgflag ? "already" : "now", buf);
#else
        if (msgflag) {
            You("이미 %s있다.", buf);
        } else {
            You("이제 %s있다.", buf);
        }
#endif
    }
}

/* KMH, #conduct
 * (shares enlightenment's tense handling)
 */
int
doconduct(VOID_ARGS)
{
    show_conduct(0);
    return 0;
}

void
show_conduct(final)
int final;
{
    char buf[BUFSZ];
    int ngenocided;

/* Create the conduct window */
    en_win = create_nhwindow(NHW_MENU);
    /*KR putstr(en_win, 0, "Voluntary challenges:"); */
    putstr(en_win, 0, "자발적 도전 과제:");

    if (u.uroleplay.blind)
        /*KR you_have_been("blind from birth"); */
        you_have_been("태어날 때부터 맹인");
    if (u.uroleplay.nudist)
        /*KR you_have_been("faithfully nudist"); */
        you_have_been("철저한 나체주의자");

    if (!u.uconduct.food)
#if 0 /*KR: 원본 (함수 변경 및 의미 의역)*/
        enl_msg(You_, "have gone", "went", " without food", "");
#else
        you_have_never("음식을 먹은 ");
#endif
    /* but beverages are okay */
    else if (!u.uconduct.unvegan)
#if 0 /*KR: 원본 (함수 변경: you_have_X -> you_have_been)*/
        you_have_X("followed a strict vegan diet");
#else
        you_have_been("철저한 비건(완전 채식주의자)");
#endif
    else if (!u.uconduct.unvegetarian)
        /*KR you_have_been("vegetarian"); */
        you_have_been("채식주의자");

    if (!u.uconduct.gnostic)
        /*KR you_have_been("an atheist"); */
        you_have_been("무신론자");

    if (!u.uconduct.weaphit) {
        /*KR you_have_never("hit with a wielded weapon"); */
        you_have_never("장비한 무기로 공격한 ");
    } else if (wizard) {
#if 0 /*KR: 원본 (plur 함수 제거)*/
        Sprintf(buf, "used a wielded weapon %ld time%s", u.uconduct.weaphit,
                plur(u.uconduct.weaphit));
        you_have_X(buf);
#else
        Sprintf(buf, "장비한 무기를 %ld번 사용한 ", u.uconduct.weaphit);
        you_have_X(buf);
#endif
    }
    if (!u.uconduct.killer)
        /*KR you_have_been("a pacifist"); */
        you_have_been("평화주의자");

    if (!u.uconduct.literate) {
        /*KR you_have_been("illiterate"); */
        you_have_been("문맹");
    } else if (wizard) {
#if 0 /*KR: 원본 (plur 함수 제거)*/
        Sprintf(buf, "read items or engraved %ld time%s", u.uconduct.literate,
                plur(u.uconduct.literate));
        you_have_X(buf);
#else
        Sprintf(buf, "물건을 읽거나 글씨를 %ld번 새긴 ", u.uconduct.literate);
        you_have_X(buf);
#endif
    }

    ngenocided = num_genocides();
    if (ngenocided == 0) {
        /*KR you_have_never("genocided any monsters"); */
        you_have_never("몬스터를 학살한 ");
    } else {
#if 0 /*KR: 원본 (plur 함수 제거)*/
        Sprintf(buf, "genocided %d type%s of monster%s", ngenocided,
                plur(ngenocided), plur(ngenocided));
        you_have_X(buf);
#else
        Sprintf(buf, "%d종류의 몬스터를 학살한 ", ngenocided);
        you_have_X(buf);
#endif
    }

    if (!u.uconduct.polypiles) {
        /*KR you_have_never("polymorphed an object"); */
        you_have_never("물건을 폴리모프한 ");
    } else if (wizard) {
#if 0 /*KR: 원본 (plur 함수 제거)*/
        Sprintf(buf, "polymorphed %ld item%s", u.uconduct.polypiles,
                plur(u.uconduct.polypiles));
        you_have_X(buf);
#else
        Sprintf(buf, "%ld개의 물건을 폴리모프한 ", u.uconduct.polypiles);
        you_have_X(buf);
#endif
    }

    if (!u.uconduct.polyselfs) {
        /*KR you_have_never("changed form"); */
        you_have_never("모습을 바꾼 ");
    } else if (wizard) {
#if 0 /*KR: 원본 (plur 함수 제거)*/
        Sprintf(buf, "changed form %ld time%s", u.uconduct.polyselfs,
                plur(u.uconduct.polyselfs));
        you_have_X(buf);
#else
        Sprintf(buf, "%ld번 모습을 바꾼 ", u.uconduct.polyselfs);
        you_have_X(buf);
#endif
    }

    if (!u.uconduct.wishes) {
#if 0 /*KR: 원본 (함수 변경: you_have_X -> you_have_never)*/
        you_have_X("used no wishes");
#else
        you_have_never("소원을 빈 ");
#endif
    } else {
#if 0 /*KR: 원본*/
        Sprintf(buf, "used %ld wish%s", u.uconduct.wishes,
                (u.uconduct.wishes > 1L) ? "es" : "");
        if (u.uconduct.wisharti) {
            /* if wisharti == wishes
             * 1 wish (for an artifact)
             * 2 wishes (both for artifacts)
             * N wishes (all for artifacts)
             * else (N is at least 2 in order to get here; M < N)
             * N wishes (1 for an artifact)
             * N wishes (M for artifacts)
             */
            if (u.uconduct.wisharti == u.uconduct.wishes)
                Sprintf(eos(buf), " (%s",
                        (u.uconduct.wisharti > 2L) ? "all "
                          : (u.uconduct.wisharti == 2L) ? "both " : "");
            else
                Sprintf(eos(buf), " (%ld ", u.uconduct.wisharti);

            Sprintf(eos(buf), "for %s)",
                    (u.uconduct.wisharti == 1L) ? "an artifact"
                                                : "artifacts");
        }
#else /*KR: KRNethack 맞춤 번역 (단복수 처리 제거 및 한국어 최적화)*/
        Sprintf(buf, "%ld번 소원을 빈 ", u.uconduct.wishes);
        if (u.uconduct.wisharti) {
            if (u.uconduct.wisharti == u.uconduct.wishes)
                Sprintf(eos(buf), " (전부 아티팩트)");
            else
                Sprintf(eos(buf), " (그 중 %ld번은 아티팩트)",
                        u.uconduct.wisharti);
        }
#endif
        you_have_X(buf);

        if (!u.uconduct.wisharti)
#if 0 /*KR: 원본 (함수 변경: enl_msg -> you_have_never)*/
            enl_msg(You_, "have not wished", "did not wish",
                    " for any artifacts", "");
#else
            you_have_never("아티팩트를 소원한 ");
#endif
    }

    /* Pop up the window and wait for a key */
    display_nhwindow(en_win, TRUE);
    destroy_nhwindow(en_win);
    en_win = WIN_ERR;
}

/* ordered by command name */
struct ext_func_tab extcmdlist[] = {
#if 0 /*KR: 원본*/
    { '#', "#", "perform an extended command",
            doextcmd, IFBURIED | GENERALCMD },
    { M('?'), "?", "list all extended commands",
            doextlist, IFBURIED | AUTOCOMPLETE | GENERALCMD },
    { M('a'), "adjust", "adjust inventory letters",
            doorganize, IFBURIED | AUTOCOMPLETE },
    { M('A'), "annotate", "name current level",
            donamelevel, IFBURIED | AUTOCOMPLETE },
    { 'a', "apply", "apply (use) a tool (pick-axe, key, lamp...)",
            doapply },
    { C('x'), "attributes", "show your attributes",
            doattributes, IFBURIED },
    { '@', "autopickup", "toggle the pickup option on/off",
            dotogglepickup, IFBURIED },
    { 'C', "call", "call (name) something", docallcmd, IFBURIED },
    { 'Z', "cast", "zap (cast) a spell", docast, IFBURIED },
    { M('c'), "chat", "talk to someone", dotalk, IFBURIED | AUTOCOMPLETE },
    { 'c', "close", "close a door", doclose },
    { M('C'), "conduct", "list voluntary challenges you have maintained",
            doconduct, IFBURIED | AUTOCOMPLETE },
    { M('d'), "dip", "dip an object into something", dodip, AUTOCOMPLETE },
    { '>', "down", "go down a staircase", dodown },
    { 'd', "drop", "drop an item", dodrop },
    { 'D', "droptype", "drop specific item types", doddrop },
    { 'e', "eat", "eat something", doeat },
    { 'E', "engrave", "engrave writing on the floor", doengrave },
    { M('e'), "enhance", "advance or check weapon and spell skills",
            enhance_weapon_skill, IFBURIED | AUTOCOMPLETE },
    { '\0', "exploremode", "enter explore (discovery) mode",
            enter_explore_mode, IFBURIED },
    { 'f', "fire", "fire ammunition from quiver", dofire },
    { M('f'), "force", "force a lock", doforce, AUTOCOMPLETE },
    { ';', "glance", "show what type of thing a map symbol corresponds to",
            doquickwhatis, IFBURIED | GENERALCMD },
    { '?', "help", "give a help message", dohelp, IFBURIED | GENERALCMD },
    { '\0', "herecmdmenu", "show menu of commands you can do here",
            doherecmdmenu, IFBURIED },
    { 'V', "history", "show long version and game history",
            dohistory, IFBURIED | GENERALCMD },
    { 'i', "inventory", "show your inventory", ddoinv, IFBURIED },
    { 'I', "inventtype", "inventory specific item types",
            dotypeinv, IFBURIED },
    { M('i'), "invoke", "invoke an object's special powers",
            doinvoke, IFBURIED | AUTOCOMPLETE },
    { M('j'), "jump", "jump to another location", dojump, AUTOCOMPLETE },
    { C('d'), "kick", "kick something", dokick },
    { '\\', "known", "show what object types have been discovered",
            dodiscovered, IFBURIED | GENERALCMD },
    { '`', "knownclass", "show discovered types for one class of objects",
            doclassdisco, IFBURIED | GENERALCMD },
    { '\0', "levelchange", "change experience level",
            wiz_level_change, IFBURIED | AUTOCOMPLETE | WIZMODECMD },
    { '\0', "lightsources", "show mobile light sources",
            wiz_light_sources, IFBURIED | AUTOCOMPLETE | WIZMODECMD },
    { ':', "look", "look at what is here", dolook, IFBURIED },
    { M('l'), "loot", "loot a box on the floor", doloot, AUTOCOMPLETE },
#ifdef DEBUG_MIGRATING_MONS
    { '\0', "migratemons", "migrate N random monsters",
            wiz_migrate_mons, IFBURIED | AUTOCOMPLETE | WIZMODECMD },
#endif
    { M('m'), "monster", "use monster's special ability",
            domonability, IFBURIED | AUTOCOMPLETE },
    { 'N', "name", "name a monster or an object",
            docallcmd, IFBURIED | AUTOCOMPLETE },
    { M('o'), "offer", "offer a sacrifice to the gods",
            dosacrifice, AUTOCOMPLETE },
    { 'o', "open", "open a door", doopen },
    { 'O', "options", "show option settings, possibly change them",
            doset, IFBURIED | GENERALCMD },
    { C('o'), "overview", "show a summary of the explored dungeon",
            dooverview, IFBURIED | AUTOCOMPLETE },
    { '\0', "panic", "test panic routine (fatal to game)",
            wiz_panic, IFBURIED | AUTOCOMPLETE | WIZMODECMD },
    { 'p', "pay", "pay your shopping bill", dopay },
    { ',', "pickup", "pick up things at the current location", dopickup },
    { '\0', "polyself", "polymorph self",
            wiz_polyself, IFBURIED | AUTOCOMPLETE | WIZMODECMD },
    { M('p'), "pray", "pray to the gods for help",
            dopray, IFBURIED | AUTOCOMPLETE },
    { C('p'), "prevmsg", "view recent game messages",
            doprev_message, IFBURIED | GENERALCMD },
    { 'P', "puton", "put on an accessory (ring, amulet, etc)", doputon },
    { 'q', "quaff", "quaff (drink) something", dodrink },
    { M('q'), "quit", "exit without saving current game",
            done2, IFBURIED | AUTOCOMPLETE | GENERALCMD },
    { 'Q', "quiver", "select ammunition for quiver", dowieldquiver },
    { 'r', "read", "read a scroll or spellbook", doread },
    { C('r'), "redraw", "redraw screen", doredraw, IFBURIED | GENERALCMD },
    { 'R', "remove", "remove an accessory (ring, amulet, etc)", doremring },
    { M('R'), "ride", "mount or dismount a saddled steed",
            doride, AUTOCOMPLETE },
    { M('r'), "rub", "rub a lamp or a stone", dorub, AUTOCOMPLETE },
    { 'S', "save", "save the game and exit", dosave, IFBURIED | GENERALCMD },
    { 's', "search", "search for traps and secret doors",
            dosearch, IFBURIED, "searching" },
    { '*', "seeall", "show all equipment in use", doprinuse, IFBURIED },
    { AMULET_SYM, "seeamulet", "show the amulet currently worn",
            dopramulet, IFBURIED },
    { ARMOR_SYM, "seearmor", "show the armor currently worn",
            doprarm, IFBURIED },
    { GOLD_SYM, "seegold", "count your gold", doprgold, IFBURIED },
    { '\0', "seenv", "show seen vectors",
            wiz_show_seenv, IFBURIED | AUTOCOMPLETE | WIZMODECMD },
    { RING_SYM, "seerings", "show the ring(s) currently worn",
            doprring, IFBURIED },
    { SPBOOK_SYM, "seespells", "list and reorder known spells",
            dovspell, IFBURIED },
    { TOOL_SYM, "seetools", "show the tools currently in use",
            doprtool, IFBURIED },
    { '^', "seetrap", "show the type of adjacent trap", doidtrap, IFBURIED },
    { WEAPON_SYM, "seeweapon", "show the weapon currently wielded",
            doprwep, IFBURIED },
    { '!', "shell", "do a shell escape",
            dosh_core, IFBURIED | GENERALCMD
#ifndef SHELL
                       | CMD_NOT_AVAILABLE
#endif /* SHELL */
    },
    { M('s'), "sit", "sit down", dosit, AUTOCOMPLETE },
    { '\0', "stats", "show memory statistics",
            wiz_show_stats, IFBURIED | AUTOCOMPLETE | WIZMODECMD },
    { C('z'), "suspend", "suspend the game",
            dosuspend_core, IFBURIED | GENERALCMD
#ifndef SUSPEND
                            | CMD_NOT_AVAILABLE
#endif /* SUSPEND */
    },
    { 'x', "swap", "swap wielded and secondary weapons", doswapweapon },
    { 'T', "takeoff", "take off one piece of armor", dotakeoff },
    { 'A', "takeoffall", "remove all armor", doddoremarm },
    { C('t'), "teleport", "teleport around the level", dotelecmd, IFBURIED },
    { '\0', "terrain", "show map without obstructions",
            doterrain, IFBURIED | AUTOCOMPLETE },
    { '\0', "therecmdmenu",
            "menu of commands you can do from here to adjacent spot",
            dotherecmdmenu },
    { 't', "throw", "throw something", dothrow },
    { '\0', "timeout", "look at timeout queue and hero's timed intrinsics",
            wiz_timeout_queue, IFBURIED | AUTOCOMPLETE | WIZMODECMD },
    { M('T'), "tip", "empty a container", dotip, AUTOCOMPLETE },
    { '_', "travel", "travel to a specific location on the map", dotravel },
    { M('t'), "turn", "turn undead away", doturn, IFBURIED | AUTOCOMPLETE },
    { 'X', "twoweapon", "toggle two-weapon combat",
            dotwoweapon, AUTOCOMPLETE },
    { M('u'), "untrap", "untrap something", dountrap, AUTOCOMPLETE },
    { '<', "up", "go up a staircase", doup },
    { '\0', "vanquished", "list vanquished monsters",
            dovanquished, IFBURIED | AUTOCOMPLETE | WIZMODECMD },
    { M('v'), "version",
            "list compile time options for this version of NetHack",
            doextversion, IFBURIED | AUTOCOMPLETE | GENERALCMD },
    { 'v', "versionshort", "show version", doversion, IFBURIED | GENERALCMD },
    { '\0', "vision", "show vision array",
            wiz_show_vision, IFBURIED | AUTOCOMPLETE | WIZMODECMD },
    { '.', "wait", "rest one move while doing nothing",
            donull, IFBURIED, "waiting" },
    { 'W', "wear", "wear a piece of armor", dowear },
    { '&', "whatdoes", "tell what a command does", dowhatdoes, IFBURIED },
    { '/', "whatis", "show what type of thing a symbol corresponds to",
            dowhatis, IFBURIED | GENERALCMD },
    { 'w', "wield", "wield (put in use) a weapon", dowield },
    { M('w'), "wipe", "wipe off your face", dowipe, AUTOCOMPLETE },
#ifdef DEBUG
    { '\0', "wizbury", "bury objs under and around you",
            wiz_debug_cmd_bury, IFBURIED | AUTOCOMPLETE | WIZMODECMD },
#endif
    { C('e'), "wizdetect", "reveal hidden things within a small radius",
            wiz_detect, IFBURIED | AUTOCOMPLETE | WIZMODECMD },
    { C('g'), "wizgenesis", "create a monster",
            wiz_genesis, IFBURIED | AUTOCOMPLETE | WIZMODECMD },
    { C('i'), "wizidentify", "identify all items in inventory",
            wiz_identify, IFBURIED | AUTOCOMPLETE | WIZMODECMD },
    { '\0', "wizintrinsic", "set an intrinsic",
            wiz_intrinsic, IFBURIED | AUTOCOMPLETE | WIZMODECMD },
    { C('v'), "wizlevelport", "teleport to another level",
            wiz_level_tele, IFBURIED | AUTOCOMPLETE | WIZMODECMD },
    { '\0', "wizmakemap", "recreate the current level",
            wiz_makemap, IFBURIED | WIZMODECMD },
    { C('f'), "wizmap", "map the level",
            wiz_map, IFBURIED | AUTOCOMPLETE | WIZMODECMD },
    { '\0', "wizrumorcheck", "verify rumor boundaries",
            wiz_rumor_check, IFBURIED | AUTOCOMPLETE | WIZMODECMD },
    { '\0', "wizsmell", "smell monster",
            wiz_smell, IFBURIED | AUTOCOMPLETE | WIZMODECMD },
    { '\0', "wizwhere", "show locations of special levels",
            wiz_where, IFBURIED | AUTOCOMPLETE | WIZMODECMD },
    { C('w'), "wizwish", "wish for something",
            wiz_wish, IFBURIED | AUTOCOMPLETE | WIZMODECMD },
    { '\0', "wmode", "show wall modes",
            wiz_show_wmodes, IFBURIED | AUTOCOMPLETE | WIZMODECMD },
    { 'z', "zap", "zap a wand", dozap },
#else /*KR: KRNethack 맞춤 번역*/
    { '#', "#", "확장 명령어를 실행한다", doextcmd, IFBURIED | GENERALCMD },
    { M('?'), "?", "모든 확장 명령어 목록을 본다", doextlist,
      IFBURIED | AUTOCOMPLETE | GENERALCMD },
    { M('a'), "adjust", "소지품의 단축키 알파벳을 조정한다", doorganize,
      IFBURIED | AUTOCOMPLETE },
    { M('A'), "annotate", "현재 층에 이름을 붙인다", donamelevel,
      IFBURIED | AUTOCOMPLETE },
    { 'a', "apply", "도구를 사용한다 (곡괭이, 열쇠, 램프 등)", doapply },
    { C('x'), "attributes", "당신의 능력치를 본다", doattributes, IFBURIED },
    { '@', "autopickup", "자동 줍기 옵션을 켜거나 끈다", dotogglepickup,
      IFBURIED },
    { 'C', "call", "무언가에 이름을 붙인다", docallcmd, IFBURIED },
    { 'Z', "cast", "마법을 시전한다", docast, IFBURIED },
    { M('c'), "chat", "누군가와 대화한다", dotalk, IFBURIED | AUTOCOMPLETE },
    { 'c', "close", "문을 닫는다", doclose },
    { M('C'), "conduct", "현재 유지 중인 자발적 도전 과제 목록을 본다",
      doconduct, IFBURIED | AUTOCOMPLETE },
    { M('d'), "dip", "무언가에 물건을 담근다", dodip, AUTOCOMPLETE },
    { '>', "down", "계단을 내려간다", dodown },
    { 'd', "drop", "물건을 내려놓는다", dodrop },
    { 'D', "droptype", "특정 종류의 물건들을 내려놓는다", doddrop },
    { 'e', "eat", "무언가를 먹는다", doeat },
    { 'E', "engrave", "바닥에 글씨를 새긴다", doengrave },
    { M('e'), "enhance", "무기 및 마법 숙련도를 확인하거나 올린다",
      enhance_weapon_skill, IFBURIED | AUTOCOMPLETE },
    { '\0', "exploremode", "탐험(발견) 모드로 진입한다", enter_explore_mode,
      IFBURIED },
    { 'f', "fire", "화살통에 장전된 탄약을 발사한다", dofire },
    { M('f'), "force", "억지로 잠금을 푼다", doforce, AUTOCOMPLETE },
    { ';', "glance", "지도의 기호가 어떤 사물에 해당하는지 확인한다",
      doquickwhatis, IFBURIED | GENERALCMD },
    { '?', "help", "도움말을 본다", dohelp, IFBURIED | GENERALCMD },
    { '\0', "herecmdmenu", "현재 위치에서 할 수 있는 행동 메뉴를 띄운다",
      doherecmdmenu, IFBURIED },
    { 'V', "history", "상세 버전 및 게임 기록을 본다", dohistory,
      IFBURIED | GENERALCMD },
    { 'i', "inventory", "소지품을 본다", ddoinv, IFBURIED },
    { 'I', "inventtype", "특정 종류의 소지품만 본다", dotypeinv, IFBURIED },
    { M('i'), "invoke", "물건의 특수 능력을 기원(발동)한다", doinvoke,
      IFBURIED | AUTOCOMPLETE },
    { M('j'), "jump", "다른 위치로 도약한다", dojump, AUTOCOMPLETE },
    { C('d'), "kick", "무언가를 발로 찬다", dokick },
    { '\\', "known", "지금까지 발견한 물건의 종류들을 본다", dodiscovered,
      IFBURIED | GENERALCMD },
    { '`', "knownclass", "특정 종류 중 발견한 물건들을 본다", doclassdisco,
      IFBURIED | GENERALCMD },
    { '\0', "levelchange", "경험치 레벨을 변경한다", wiz_level_change,
      IFBURIED | AUTOCOMPLETE | WIZMODECMD },
    { '\0', "lightsources", "이동하는 광원을 확인한다", wiz_light_sources,
      IFBURIED | AUTOCOMPLETE | WIZMODECMD },
    { ':', "look", "이곳에 무엇이 있는지 살핀다", dolook, IFBURIED },
    { M('l'), "loot", "바닥에 있는 용기에서 물건을 꺼낸다(루팅)", doloot,
      AUTOCOMPLETE },
#ifdef DEBUG_MIGRATING_MONS
    { '\0', "migratemons", "무작위 몬스터 N마리를 이주시킨다",
      wiz_migrate_mons, IFBURIED | AUTOCOMPLETE | WIZMODECMD },
#endif
    { M('m'), "monster", "몬스터의 특수 능력을 사용한다", domonability,
      IFBURIED | AUTOCOMPLETE },
    { 'N', "name", "몬스터나 물건에 이름을 붙인다", docallcmd,
      IFBURIED | AUTOCOMPLETE },
    { M('o'), "offer", "신에게 제물을 바친다", dosacrifice, AUTOCOMPLETE },
    { 'o', "open", "문을 연다", doopen },
    { 'O', "options", "옵션 설정을 보고 변경한다", doset,
      IFBURIED | GENERALCMD },
    { C('o'), "overview", "탐험한 미궁의 개요를 본다", dooverview,
      IFBURIED | AUTOCOMPLETE },
    { '\0', "panic", "패닉 루틴을 테스트한다 (게임에 치명적임)", wiz_panic,
      IFBURIED | AUTOCOMPLETE | WIZMODECMD },
    { 'p', "pay", "상점의 청구서를 지불한다", dopay },
    { ',', "pickup", "현재 위치에 있는 물건들을 줍는다", dopickup },
    { '\0', "polyself", "자신을 폴리모프(변신)한다", wiz_polyself,
      IFBURIED | AUTOCOMPLETE | WIZMODECMD },
    { M('p'), "pray", "신에게 도움을 기도한다", dopray,
      IFBURIED | AUTOCOMPLETE },
    { C('p'), "prevmsg", "최근 게임 메시지를 다시 본다", doprev_message,
      IFBURIED | GENERALCMD },
    { 'P', "puton", "장신구를 착용한다 (반지, 목걸이 등)", doputon },
    { 'q', "quaff", "무언가를 마신다", dodrink },
    { M('q'), "quit", "저장하지 않고 게임을 종료한다", done2,
      IFBURIED | AUTOCOMPLETE | GENERALCMD },
    { 'Q', "quiver", "화살통에 넣을 탄약을 선택한다", dowieldquiver },
    { 'r', "read", "두루마리나 마법서를 읽는다", doread },
    { C('r'), "redraw", "화면을 다시 그린다", doredraw,
      IFBURIED | GENERALCMD },
    { 'R', "remove", "장신구를 벗는다 (반지, 목걸이 등)", doremring },
    { M('R'), "ride", "안장이 얹힌 탈것에 타거나 내린다", doride,
      AUTOCOMPLETE },
    { M('r'), "rub", "램프나 돌을 문지른다", dorub, AUTOCOMPLETE },
    { 'S', "save", "게임을 저장하고 종료한다", dosave,
      IFBURIED | GENERALCMD },
    { 's', "search", "함정과 숨겨진 문을 찾는다", dosearch, IFBURIED,
      "탐색하는" },
    { '*', "seeall", "사용 중인 모든 장비를 본다", doprinuse, IFBURIED },
    { AMULET_SYM, "seeamulet", "착용 중인 목걸이를 본다", dopramulet,
      IFBURIED },
    { ARMOR_SYM, "seearmor", "착용 중인 방어구를 본다", doprarm, IFBURIED },
    { GOLD_SYM, "seegold", "가진 금화의 수를 센다", doprgold, IFBURIED },
    { '\0', "seenv", "시야 벡터를 본다", wiz_show_seenv,
      IFBURIED | AUTOCOMPLETE | WIZMODECMD },
    { RING_SYM, "seerings", "착용 중인 반지를 본다", doprring, IFBURIED },
    { SPBOOK_SYM, "seespells", "아는 마법 목록을 보고 재배열한다", dovspell,
      IFBURIED },
    { TOOL_SYM, "seetools", "사용 중인 도구를 본다", doprtool, IFBURIED },
    { '^', "seetrap", "인접한 함정의 종류를 본다", doidtrap, IFBURIED },
    { WEAPON_SYM, "seeweapon", "장비한 무기를 본다", doprwep, IFBURIED },
    { '!', "shell", "쉘(Shell)로 빠져나간다", dosh_core,
      IFBURIED | GENERALCMD
#ifndef SHELL
          | CMD_NOT_AVAILABLE
#endif /* SHELL */
    },
    { M('s'), "sit", "자리에 앉는다", dosit, AUTOCOMPLETE },
    { '\0', "stats", "메모리 통계를 본다", wiz_show_stats,
      IFBURIED | AUTOCOMPLETE | WIZMODECMD },
    { C('z'), "suspend", "게임을 일시 중지한다", dosuspend_core,
      IFBURIED | GENERALCMD
#ifndef SUSPEND
          | CMD_NOT_AVAILABLE
#endif /* SUSPEND */
    },
    { 'x', "swap", "주 무기와 보조 무기를 바꾼다", doswapweapon },
    { 'T', "takeoff", "방어구를 하나 벗는다", dotakeoff },
    { 'A', "takeoffall", "모든 방어구를 벗는다", doddoremarm },
    { C('t'), "teleport", "현재 층 내에서 순간이동한다", dotelecmd,
      IFBURIED },
    { '\0', "terrain", "방해물 없이 지도를 본다", doterrain,
      IFBURIED | AUTOCOMPLETE },
    { '\0', "therecmdmenu", "인접한 칸에 대해 할 수 있는 명령 메뉴를 띄운다",
      dotherecmdmenu },
    { 't', "throw", "무언가를 던진다", dothrow },
    { '\0', "timeout", "시간 제한 큐와 시간제 능력치를 본다",
      wiz_timeout_queue, IFBURIED | AUTOCOMPLETE | WIZMODECMD },
    { M('T'), "tip", "용기를 비운다", dotip, AUTOCOMPLETE },
    { '_', "travel", "지도의 특정 위치로 이동한다", dotravel },
    { M('t'), "turn", "언데드를 퇴치한다", doturn, IFBURIED | AUTOCOMPLETE },
    { 'X', "twoweapon", "쌍수 무기 전투를 켜거나 끈다", dotwoweapon,
      AUTOCOMPLETE },
    { M('u'), "untrap", "함정을 해제한다", dountrap, AUTOCOMPLETE },
    { '<', "up", "계단을 올라간다", doup },
    { '\0', "vanquished", "물리친 몬스터 목록을 본다", dovanquished,
      IFBURIED | AUTOCOMPLETE | WIZMODECMD },
    { M('v'), "version", "넷핵의 컴파일 옵션을 본다", doextversion,
      IFBURIED | AUTOCOMPLETE | GENERALCMD },
    { 'v', "versionshort", "버전을 본다", doversion, IFBURIED | GENERALCMD },
    { '\0', "vision", "시야 배열을 본다", wiz_show_vision,
      IFBURIED | AUTOCOMPLETE | WIZMODECMD },
    { '.', "wait", "한 턴 동안 아무것도 하지 않고 쉰다", donull, IFBURIED,
      "쉬는" },
    { 'W', "wear", "방어구를 입는다", dowear },
    { '&', "whatdoes", "명령어가 어떤 역할을 하는지 확인한다", dowhatdoes,
      IFBURIED },
    { '/', "whatis", "기호가 어떤 사물에 해당하는지 확인한다", dowhatis,
      IFBURIED | GENERALCMD },
    { 'w', "wield", "무기를 장비한다", dowield },
    { M('w'), "wipe", "얼굴을 닦는다", dowipe, AUTOCOMPLETE },
#ifdef DEBUG
    { '\0', "wizbury", "발밑과 주변의 물건들을 파묻는다", wiz_debug_cmd_bury,
      IFBURIED | AUTOCOMPLETE | WIZMODECMD },
#endif
    { C('e'), "wizdetect", "좁은 반경 내의 숨겨진 것들을 드러낸다",
      wiz_detect, IFBURIED | AUTOCOMPLETE | WIZMODECMD },
    { C('g'), "wizgenesis", "몬스터를 생성한다", wiz_genesis,
      IFBURIED | AUTOCOMPLETE | WIZMODECMD },
    { C('i'), "wizidentify", "소지품의 모든 물건을 감정한다", wiz_identify,
      IFBURIED | AUTOCOMPLETE | WIZMODECMD },
    { '\0', "wizintrinsic", "내재적 능력을 설정한다", wiz_intrinsic,
      IFBURIED | AUTOCOMPLETE | WIZMODECMD },
    { C('v'), "wizlevelport", "다른 층으로 순간이동한다", wiz_level_tele,
      IFBURIED | AUTOCOMPLETE | WIZMODECMD },
    { '\0', "wizmakemap", "현재 층을 다시 만든다", wiz_makemap,
      IFBURIED | WIZMODECMD },
    { C('f'), "wizmap", "층의 지도를 밝힌다", wiz_map,
      IFBURIED | AUTOCOMPLETE | WIZMODECMD },
    { '\0', "wizrumorcheck", "소문의 경계를 검증한다", wiz_rumor_check,
      IFBURIED | AUTOCOMPLETE | WIZMODECMD },
    { '\0', "wizsmell", "몬스터의 냄새를 맡는다", wiz_smell,
      IFBURIED | AUTOCOMPLETE | WIZMODECMD },
    { '\0', "wizwhere", "특수 층들의 위치를 본다", wiz_where,
      IFBURIED | AUTOCOMPLETE | WIZMODECMD },
    { C('w'), "wizwish", "무언가를 소원한다", wiz_wish,
      IFBURIED | AUTOCOMPLETE | WIZMODECMD },
    { '\0', "wmode", "벽 모드를 본다", wiz_show_wmodes,
      IFBURIED | AUTOCOMPLETE | WIZMODECMD },
    { 'z', "zap", "지팡이를 휘두른다", dozap },
#endif
    { '\0', (char *) 0, (char *) 0, donull, 0, (char *) 0 } /* sentinel */
};

/* for key2extcmddesc() to support dowhatdoes() */
struct movcmd {
    uchar k1, k2, k3, k4; /* 'normal', 'qwertz', 'numpad', 'phone' */
    const char *txt, *alt; /* compass direction, screen direction */
};
#if 0 /*KR: 원본*/
static const struct movcmd movtab[] = {
    { 'h', 'h', '4', '4', "west",      "left" },
    { 'j', 'j', '2', '8', "south",     "down" },
    { 'k', 'k', '8', '2', "north",     "up" },
    { 'l', 'l', '6', '6', "east",      "right" },
    { 'b', 'b', '1', '7', "southwest", "lower left" },
    { 'n', 'n', '3', '9', "southeast", "lower right" },
    { 'u', 'u', '9', '3', "northeast", "upper right" },
    { 'y', 'z', '7', '1', "northwest", "upper left" },
    {   0,   0,   0,   0,  (char *) 0, (char *) 0 }
};
#else
static const struct movcmd movtab[] = {
    { 'h', 'h', '4', '4', "서쪽", "왼쪽" },
    { 'j', 'j', '2', '8', "남쪽", "아래쪽" },
    { 'k', 'k', '8', '2', "북쪽", "위쪽" },
    { 'l', 'l', '6', '6', "동쪽", "오른쪽" },
    { 'b', 'b', '1', '7', "남서쪽", "좌측 하단" },
    { 'n', 'n', '3', '9', "남동쪽", "우측 하단" },
    { 'u', 'u', '9', '3', "북동쪽", "우측 상단" },
    { 'y', 'z', '7', '1', "북서쪽", "좌측 상단" },
    { 0, 0, 0, 0, (char *) 0, (char *) 0 }
};
#endif

int extcmdlist_length = SIZE(extcmdlist) - 1;

const char *
key2extcmddesc(key)
uchar key;
{
    static char key2cmdbuf[48];
    const struct movcmd *mov;
    int k, c;
    uchar M_5 = (uchar) M('5'), M_0 = (uchar) M('0');

    /* need to check for movement commands before checking the extended
       commands table because it contains entries for number_pad commands
       that match !number_pad movement (like 'j' for "jump") */
    key2cmdbuf[0] = '\0';
    if (movecmd(k = key))
        /* "move or attack"? */
        /*KR Strcpy(key2cmdbuf, "move"); */ 
        Strcpy(key2cmdbuf, "이동");
    else if (movecmd(k = unctrl(key)))
        /*KR Strcpy(key2cmdbuf, "rush"); */
        Strcpy(key2cmdbuf, "돌진");
    else if (movecmd(k = (Cmd.num_pad ? unmeta(key) : lowc(key))))
        /*KR Strcpy(key2cmdbuf, "run"); */
        Strcpy(key2cmdbuf, "달리기");
    if (*key2cmdbuf) {
        for (mov = &movtab[0]; mov->k1; ++mov) {
            c = !Cmd.num_pad ? (!Cmd.swap_yz ? mov->k1 : mov->k2)
                             : (!Cmd.phone_layout ? mov->k3 : mov->k4);
            if (c == k) {
#if 0 /*KR: 원본*/
                Sprintf(eos(key2cmdbuf), " %s (screen %s)",
                        mov->txt, mov->alt);
#else
                Sprintf(eos(key2cmdbuf), ": %s (화면 %s)", mov->txt,
                        mov->alt);
#endif
                return key2cmdbuf;
            }
        }
    } else if (digit(key) || (Cmd.num_pad && digit(unmeta(key)))) {
        key2cmdbuf[0] = '\0';
        if (!Cmd.num_pad)
       /*KR Strcpy(key2cmdbuf, "start of, or continuation of, a count"); */
            Strcpy(key2cmdbuf, "반복 횟수 입력 시작 또는 계속");
        else if (key == '5' || key == M_5)
#if 0 /*KR: 원본*/
            Sprintf(key2cmdbuf, "%s prefix",
                    (!!Cmd.pcHack_compat ^ (key == M_5)) ? "run" : "rush");
#else
            Sprintf(key2cmdbuf, "%s 접두사",
                    (!!Cmd.pcHack_compat ^ (key == M_5)) ? "달리기" : "돌진");
#endif
        else if (key == '0' || (Cmd.pcHack_compat && key == M_0))
            /*KR Strcpy(key2cmdbuf, "synonym for 'i'"); */
            Strcpy(key2cmdbuf, "'i'와 동일");
        if (*key2cmdbuf)
            return key2cmdbuf;
    }
    if (Cmd.commands[key]) {
        if (Cmd.commands[key]->ef_txt)
            return Cmd.commands[key]->ef_desc;

    }
    return (char *) 0;
}

boolean
bind_key(key, command)
uchar key;
const char *command;
{
    struct ext_func_tab *extcmd;

    /* special case: "nothing" is reserved for unbinding */
    if (!strcmp(command, "nothing")) {
        Cmd.commands[key] = (struct ext_func_tab *) 0;
        return TRUE;
    }

    for (extcmd = extcmdlist; extcmd->ef_txt; extcmd++) {
        if (strcmp(command, extcmd->ef_txt))
            continue;
        Cmd.commands[key] = extcmd;
#if 0 /* silently accept key binding for unavailable command (!SHELL,&c) */
        if ((extcmd->flags & CMD_NOT_AVAILABLE) != 0) {
            char buf[BUFSZ];

            Sprintf(buf, cmdnotavail, extcmd->ef_txt);
            config_error_add("%s", buf);
        }
#endif
        return TRUE;
    }

    return FALSE;
}

/* initialize all keyboard commands */
void
commands_init()
{
    struct ext_func_tab *extcmd;

    for (extcmd = extcmdlist; extcmd->ef_txt; extcmd++)
        if (extcmd->key)
            Cmd.commands[extcmd->key] = extcmd;

    (void) bind_key(C('l'), "redraw"); /* if number_pad is set */
    /*       'b', 'B' : go sw */
    /*       'F' : fight (one time) */
    /*       'g', 'G' : multiple go */
    /*       'h', 'H' : go west */
    (void) bind_key('h',    "help"); /* if number_pad is set */
    (void) bind_key('j',    "jump"); /* if number_pad is on */
    /*       'j', 'J', 'k', 'K', 'l', 'L', 'm', 'M', 'n', 'N' move commands */
    (void) bind_key('k',    "kick"); /* if number_pad is on */
    (void) bind_key('l',    "loot"); /* if number_pad is on */
    (void) bind_key(C('n'), "annotate"); /* if number_pad is on */
    (void) bind_key(M('n'), "name");
    (void) bind_key(M('N'), "name");
    (void) bind_key('u',    "untrap"); /* if number_pad is on */

    /* alt keys: */
    (void) bind_key(M('O'), "overview");
    (void) bind_key(M('2'), "twoweapon");

    /* wait_on_space */
    (void) bind_key(' ',    "wait");
}

int
dokeylist_putcmds(datawin, docount, cmdflags, exflags, keys_used)
winid datawin;
boolean docount;
int cmdflags, exflags;
boolean *keys_used; /* boolean keys_used[256] */
{
    int i;
    char buf[BUFSZ];
    char buf2[QBUFSZ];
    int count = 0;

    for (i = 0; i < 256; i++) {
        const struct ext_func_tab *extcmd;
        uchar key = (uchar) i;

        if (keys_used[i])
            continue;
        if (key == ' ' && !flags.rest_on_space)
            continue;
        if ((extcmd = Cmd.commands[i]) != (struct ext_func_tab *) 0) {
            if ((cmdflags && !(extcmd->flags & cmdflags))
                || (exflags && (extcmd->flags & exflags)))
                continue;
            if (docount) {
                count++;
                continue;
            }
            Sprintf(buf, "%-8s %-12s %s", key2txt(key, buf2),
                    extcmd->ef_txt,
                    extcmd->ef_desc);
            putstr(datawin, 0, buf);
            keys_used[i] = TRUE;
        }
    }
    return count;
}

/* list all keys and their bindings, like dat/hh but dynamic */
void
dokeylist(VOID_ARGS)
{
    char buf[BUFSZ], buf2[BUFSZ];
    uchar key;
    boolean keys_used[256] = { 0 };
    winid datawin;
    int i;
    static const char
#if 0 /*KR: 원본*/
        run_desc[] = "Prefix: run until something very interesting is seen",
        forcefight_desc[] =
                     "Prefix: force fight even if you don't see a monster";
#else
        run_desc[] = "접두사: 무언가 아주 흥미로운 것을 발견할 때까지 달린다",
        forcefight_desc[] = "접두사: 몬스터가 보이지 않더라도 강제 공격한다";
#endif
    static const struct {
        int nhkf;
        const char *desc;
        boolean numpad;
    } misc_keys[] = {
        /*KR { NHKF_ESC, "escape from the current query/action", FALSE }, */
        { NHKF_ESC, "현재 질문/행동에서 벗어난다(취소)", FALSE },
        /*KR { NHKF_RUSH, "Prefix: rush until something interesting is seen",
           FALSE }, */
        { NHKF_RUSH, "접두사: 무언가 흥미로운 것을 발견할 때까지 돌진한다",
          FALSE },
        { NHKF_RUN, run_desc, FALSE },
        { NHKF_RUN2, run_desc, TRUE },
        { NHKF_FIGHT, forcefight_desc, FALSE },
        { NHKF_FIGHT2, forcefight_desc, TRUE },
        /*KR { NHKF_NOPICKUP, "Prefix: move without picking up
           objects/fighting", FALSE }, */
        { NHKF_NOPICKUP, "접두사: 물건을 줍거나 싸우지 않고 이동한다",
          FALSE },
        /*KR { NHKF_RUN_NOPICKUP, "Prefix: run without picking up
           objects/fighting", FALSE }, */
        { NHKF_RUN_NOPICKUP, "접두사: 물건을 줍거나 싸우지 않고 달린다",
          FALSE },
        /*KR { NHKF_DOINV, "view inventory", TRUE }, */
        { NHKF_DOINV, "소지품을 본다", TRUE },
        /*KR { NHKF_REQMENU, "Prefix: request a menu", FALSE }, */
        { NHKF_REQMENU, "접두사: 메뉴를 요청한다", FALSE },
#ifdef REDO
        /*KR { NHKF_DOAGAIN , "re-do: perform the previous command again",
           FALSE }, */
        { NHKF_DOAGAIN, "재실행: 이전 명령어를 다시 수행한다", FALSE },
#endif
        { 0, (const char *) 0, FALSE }
    };

    datawin = create_nhwindow(NHW_TEXT);
    putstr(datawin, 0, "");
    /*KR putstr(datawin, 0, "            Full Current Key Bindings List"); */
    putstr(datawin, 0, "            현재 전체 키 바인딩 목록");

    /* directional keys */
    putstr(datawin, 0, "");
    /*KR putstr(datawin, 0, "Directional keys:"); */
    putstr(datawin, 0, "방향 키:");
    show_direction_keys(datawin, '.',
                        FALSE); /* '.'==self in direction grid */

    keys_used[(uchar) Cmd.move_NW] = keys_used[(uchar) Cmd.move_N] =
        keys_used[(uchar) Cmd.move_NE] = keys_used[(uchar) Cmd.move_W] =
            keys_used[(uchar) Cmd.move_E] = keys_used[(uchar) Cmd.move_SW] =
                keys_used[(uchar) Cmd.move_S] =
                    keys_used[(uchar) Cmd.move_SE] = TRUE;

    if (!iflags.num_pad) {
        keys_used[(uchar) highc(Cmd.move_NW)] =
            keys_used[(uchar) highc(Cmd.move_N)] =
                keys_used[(uchar) highc(Cmd.move_NE)] =
                    keys_used[(uchar) highc(Cmd.move_W)] =
                        keys_used[(uchar) highc(Cmd.move_E)] =
                            keys_used[(uchar) highc(Cmd.move_SW)] =
                                keys_used[(uchar) highc(Cmd.move_S)] =
                                    keys_used[(uchar) highc(Cmd.move_SE)] =
                                        TRUE;
        keys_used[(uchar) C(Cmd.move_NW)] = keys_used[(uchar) C(Cmd.move_N)] =
            keys_used[(uchar) C(Cmd.move_NE)] =
                keys_used[(uchar) C(Cmd.move_W)] =
                    keys_used[(uchar) C(Cmd.move_E)] =
                        keys_used[(uchar) C(Cmd.move_SW)] =
                            keys_used[(uchar) C(Cmd.move_S)] =
                                keys_used[(uchar) C(Cmd.move_SE)] = TRUE;
        putstr(datawin, 0, "");
#if 0 /*KR: 원본*/
        putstr(datawin, 0,
          "Shift-<direction> will move in specified direction until you hit");
        putstr(datawin, 0, "        a wall or run into something.");
#else
        putstr(datawin, 0,
               "Shift-<방향>은 벽에 부딪히거나 무언가와 마주칠 때까지");
        putstr(datawin, 0, "        지정된 방향으로 이동합니다.");
#endif
#if 0 /*KR: 원본*/
        putstr(datawin, 0,
          "Ctrl-<direction> will run in specified direction until something");
        putstr(datawin, 0, "        very interesting is seen.");
#else
        putstr(datawin, 0,
               "Ctrl-<방향>은 무언가 아주 흥미로운 것을 발견할 때까지");
        putstr(datawin, 0, "        지정된 방향으로 달립니다.");
#endif
    }

    putstr(datawin, 0, "");
    /*KR putstr(datawin, 0, "Miscellaneous keys:"); */
    putstr(datawin, 0, "기타 키:");
    for (i = 0; misc_keys[i].desc; i++) {
        key = Cmd.spkeys[misc_keys[i].nhkf];
        if (key
            && ((misc_keys[i].numpad && iflags.num_pad)
                || !misc_keys[i].numpad)) {
            keys_used[(uchar) key] = TRUE;
            Sprintf(buf, "%-8s %s", key2txt(key, buf2), misc_keys[i].desc);
            putstr(datawin, 0, buf);
        }
    }
#ifndef NO_SIGNAL
    /*KR putstr(datawin, 0, "^c        break out of NetHack (SIGINT)"); */
    putstr(datawin, 0, "^c        넷핵을 강제 종료합니다 (SIGINT)");
    keys_used[(uchar) C('c')] = TRUE;
#endif

    putstr(datawin, 0, "");
    show_menu_controls(datawin, TRUE);

    if (dokeylist_putcmds(datawin, TRUE, GENERALCMD, WIZMODECMD, keys_used)) {
        putstr(datawin, 0, "");
        /*KR putstr(datawin, 0, "General commands:"); */
        putstr(datawin, 0, "일반 명령어:");
        (void) dokeylist_putcmds(datawin, FALSE, GENERALCMD, WIZMODECMD,
                                 keys_used);
    }

    if (dokeylist_putcmds(datawin, TRUE, 0, WIZMODECMD, keys_used)) {
        putstr(datawin, 0, "");
        /*KR putstr(datawin, 0, "Game commands:"); */
        putstr(datawin, 0, "게임 명령어:");
        (void) dokeylist_putcmds(datawin, FALSE, 0, WIZMODECMD, keys_used);
    }

    if (wizard
        && dokeylist_putcmds(datawin, TRUE, WIZMODECMD, 0, keys_used)) {
        putstr(datawin, 0, "");
        /*KR putstr(datawin, 0, "Wizard-mode commands:"); */
        putstr(datawin, 0, "마법사 모드 명령어:");
        (void) dokeylist_putcmds(datawin, FALSE, WIZMODECMD, 0, keys_used);
    }

    display_nhwindow(datawin, FALSE);
    destroy_nhwindow(datawin);
}

char
cmd_from_func(fn)
int NDECL((*fn));
{
    int i;

    for (i = 0; i < 256; ++i)
        if (Cmd.commands[i] && Cmd.commands[i]->ef_funct == fn)
            return (char) i;
    return '\0';
}

/*
 * wizard mode sanity_check code
 */

static const char template[] = "%-27s  %4ld  %6ld";
static const char stats_hdr[] = "                             count  bytes";
static const char stats_sep[] = "---------------------------  ----- -------";

STATIC_OVL int
size_obj(otmp)
struct obj *otmp;
{
    int sz = (int) sizeof (struct obj);

    if (otmp->oextra) {
        sz += (int) sizeof (struct oextra);
        if (ONAME(otmp))
            sz += (int) strlen(ONAME(otmp)) + 1;
        if (OMONST(otmp))
            sz += size_monst(OMONST(otmp), FALSE);
        if (OMID(otmp))
            sz += (int) sizeof (unsigned);
        if (OLONG(otmp))
            sz += (int) sizeof (long);
        if (OMAILCMD(otmp))
            sz += (int) strlen(OMAILCMD(otmp)) + 1;
    }
    return sz;
}

STATIC_OVL void
count_obj(chain, total_count, total_size, top, recurse)
struct obj *chain;
long *total_count;
long *total_size;
boolean top;
boolean recurse;
{
    long count, size;
    struct obj *obj;

    for (count = size = 0, obj = chain; obj; obj = obj->nobj) {
        if (top) {
            count++;
            size += size_obj(obj);
        }
        if (recurse && obj->cobj)
            count_obj(obj->cobj, total_count, total_size, TRUE, TRUE);
    }
    *total_count += count;
    *total_size += size;
}

STATIC_OVL void
obj_chain(win, src, chain, force, total_count, total_size)
winid win;
const char *src;
struct obj *chain;
boolean force;
long *total_count;
long *total_size;
{
    char buf[BUFSZ];
    long count = 0L, size = 0L;

    count_obj(chain, &count, &size, TRUE, FALSE);

    if (count || size || force) {
        *total_count += count;
        *total_size += size;
        Sprintf(buf, template, src, count, size);
        putstr(win, 0, buf);
    }
}

STATIC_OVL void
mon_invent_chain(win, src, chain, total_count, total_size)
winid win;
const char *src;
struct monst *chain;
long *total_count;
long *total_size;
{
    char buf[BUFSZ];
    long count = 0, size = 0;
    struct monst *mon;

    for (mon = chain; mon; mon = mon->nmon)
        count_obj(mon->minvent, &count, &size, TRUE, FALSE);

    if (count || size) {
        *total_count += count;
        *total_size += size;
        Sprintf(buf, template, src, count, size);
        putstr(win, 0, buf);
    }
}

STATIC_OVL void
contained_stats(win, src, total_count, total_size)
winid win;
const char *src;
long *total_count;
long *total_size;
{
    char buf[BUFSZ];
    long count = 0, size = 0;
    struct monst *mon;

    count_obj(invent, &count, &size, FALSE, TRUE);
    count_obj(fobj, &count, &size, FALSE, TRUE);
    count_obj(level.buriedobjlist, &count, &size, FALSE, TRUE);
    count_obj(migrating_objs, &count, &size, FALSE, TRUE);
    /* DEADMONSTER check not required in this loop since they have no
     * inventory */
    for (mon = fmon; mon; mon = mon->nmon)
        count_obj(mon->minvent, &count, &size, FALSE, TRUE);
    for (mon = migrating_mons; mon; mon = mon->nmon)
        count_obj(mon->minvent, &count, &size, FALSE, TRUE);

    if (count || size) {
        *total_count += count;
        *total_size += size;
        Sprintf(buf, template, src, count, size);
        putstr(win, 0, buf);
    }
}

STATIC_OVL int
size_monst(mtmp, incl_wsegs)
struct monst *mtmp;
boolean incl_wsegs;
{
    int sz = (int) sizeof (struct monst);

    if (mtmp->wormno && incl_wsegs)
        sz += size_wseg(mtmp);

    if (mtmp->mextra) {
        sz += (int) sizeof (struct mextra);
        if (MNAME(mtmp))
            sz += (int) strlen(MNAME(mtmp)) + 1;
        if (EGD(mtmp))
            sz += (int) sizeof (struct egd);
        if (EPRI(mtmp))
            sz += (int) sizeof (struct epri);
        if (ESHK(mtmp))
            sz += (int) sizeof (struct eshk);
        if (EMIN(mtmp))
            sz += (int) sizeof (struct emin);
        if (EDOG(mtmp))
            sz += (int) sizeof (struct edog);
        /* mextra->mcorpsenm doesn't point to more memory */
    }
    return sz;
}

STATIC_OVL void
mon_chain(win, src, chain, force, total_count, total_size)
winid win;
const char *src;
struct monst *chain;
boolean force;
long *total_count;
long *total_size;
{
    char buf[BUFSZ];
    long count, size;
    struct monst *mon;
    /* mon->wormno means something different for migrating_mons and mydogs */
    boolean incl_wsegs = !strcmpi(src, "fmon");

    count = size = 0L;
    for (mon = chain; mon; mon = mon->nmon) {
        count++;
        size += size_monst(mon, incl_wsegs);
    }
    if (count || size || force) {
        *total_count += count;
        *total_size += size;
        Sprintf(buf, template, src, count, size);
        putstr(win, 0, buf);
    }
}

STATIC_OVL void
misc_stats(win, total_count, total_size)
winid win;
long *total_count;
long *total_size;
{
    char buf[BUFSZ], hdrbuf[QBUFSZ];
    long count, size;
    int idx;
    struct trap *tt;
    struct damage *sd; /* shop damage */
    struct kinfo *k; /* delayed killer */
    struct cemetery *bi; /* bones info */

    /* traps and engravings are output unconditionally;
     * others only if nonzero
     */
    count = size = 0L;
    for (tt = ftrap; tt; tt = tt->ntrap) {
        ++count;
        size += (long) sizeof *tt;
    }
    *total_count += count;
    *total_size += size;
    Sprintf(hdrbuf, "traps, size %ld", (long) sizeof (struct trap));
    Sprintf(buf, template, hdrbuf, count, size);
    putstr(win, 0, buf);

    count = size = 0L;
    engr_stats("engravings, size %ld+text", hdrbuf, &count, &size);
    *total_count += count;
    *total_size += size;
    Sprintf(buf, template, hdrbuf, count, size);
    putstr(win, 0, buf);

    count = size = 0L;
    light_stats("light sources, size %ld", hdrbuf, &count, &size);
    if (count || size) {
        *total_count += count;
        *total_size += size;
        Sprintf(buf, template, hdrbuf, count, size);
        putstr(win, 0, buf);
    }

    count = size = 0L;
    timer_stats("timers, size %ld", hdrbuf, &count, &size);
    if (count || size) {
        *total_count += count;
        *total_size += size;
        Sprintf(buf, template, hdrbuf, count, size);
        putstr(win, 0, buf);
    }

    count = size = 0L;
    for (sd = level.damagelist; sd; sd = sd->next) {
        ++count;
        size += (long) sizeof *sd;
    }
    if (count || size) {
        *total_count += count;
        *total_size += size;
        Sprintf(hdrbuf, "shop damage, size %ld",
                (long) sizeof (struct damage));
        Sprintf(buf, template, hdrbuf, count, size);
        putstr(win, 0, buf);
    }

    count = size = 0L;
    region_stats("regions, size %ld+%ld*rect+N", hdrbuf, &count, &size);
    if (count || size) {
        *total_count += count;
        *total_size += size;
        Sprintf(buf, template, hdrbuf, count, size);
        putstr(win, 0, buf);
    }

    count = size = 0L;
    for (k = killer.next; k; k = k->next) {
        ++count;
        size += (long) sizeof *k;
    }
    if (count || size) {
        *total_count += count;
        *total_size += size;
        Sprintf(hdrbuf, "delayed killer%s, size %ld",
                plur(count), (long) sizeof (struct kinfo));
        Sprintf(buf, template, hdrbuf, count, size);
        putstr(win, 0, buf);
    }

    count = size = 0L;
    for (bi = level.bonesinfo; bi; bi = bi->next) {
        ++count;
        size += (long) sizeof *bi;
    }
    if (count || size) {
        *total_count += count;
        *total_size += size;
        Sprintf(hdrbuf, "bones history, size %ld",
                (long) sizeof (struct cemetery));
        Sprintf(buf, template, hdrbuf, count, size);
        putstr(win, 0, buf);
    }

    count = size = 0L;
    for (idx = 0; idx < NUM_OBJECTS; ++idx)
        if (objects[idx].oc_uname) {
            ++count;
            size += (long) (strlen(objects[idx].oc_uname) + 1);
        }
    if (count || size) {
        *total_count += count;
        *total_size += size;
        Strcpy(hdrbuf, "object type names, text");
        Sprintf(buf, template, hdrbuf, count, size);
        putstr(win, 0, buf);
    }
}

/*
 * Display memory usage of all monsters and objects on the level.
 */
static int
wiz_show_stats()
{
    char buf[BUFSZ];
    winid win;
    long total_obj_size, total_obj_count,
         total_mon_size, total_mon_count,
         total_ovr_size, total_ovr_count,
         total_misc_size, total_misc_count;

    win = create_nhwindow(NHW_TEXT);
    putstr(win, 0, "Current memory statistics:");

    total_obj_count = total_obj_size = 0L;
    putstr(win, 0, stats_hdr);
    Sprintf(buf, "  Objects, base size %ld", (long) sizeof (struct obj));
    putstr(win, 0, buf);
    obj_chain(win, "invent", invent, TRUE, &total_obj_count, &total_obj_size);
    obj_chain(win, "fobj", fobj, TRUE, &total_obj_count, &total_obj_size);
    obj_chain(win, "buried", level.buriedobjlist, FALSE,
              &total_obj_count, &total_obj_size);
    obj_chain(win, "migrating obj", migrating_objs, FALSE,
              &total_obj_count, &total_obj_size);
    obj_chain(win, "billobjs", billobjs, FALSE,
              &total_obj_count, &total_obj_size);
    mon_invent_chain(win, "minvent", fmon, &total_obj_count, &total_obj_size);
    mon_invent_chain(win, "migrating minvent", migrating_mons,
                     &total_obj_count, &total_obj_size);
    contained_stats(win, "contained", &total_obj_count, &total_obj_size);
    putstr(win, 0, stats_sep);
    Sprintf(buf, template, "  Obj total", total_obj_count, total_obj_size);
    putstr(win, 0, buf);

    total_mon_count = total_mon_size = 0L;
    putstr(win, 0, "");
    Sprintf(buf, "  Monsters, base size %ld", (long) sizeof (struct monst));
    putstr(win, 0, buf);
    mon_chain(win, "fmon", fmon, TRUE, &total_mon_count, &total_mon_size);
    mon_chain(win, "migrating", migrating_mons, FALSE,
              &total_mon_count, &total_mon_size);
    /* 'mydogs' is only valid during level change or end of game disclosure,
       but conceivably we've been called from within debugger at such time */
    if (mydogs) /* monsters accompanying hero */
        mon_chain(win, "mydogs", mydogs, FALSE,
                  &total_mon_count, &total_mon_size);
    putstr(win, 0, stats_sep);
    Sprintf(buf, template, "  Mon total", total_mon_count, total_mon_size);
    putstr(win, 0, buf);

    total_ovr_count = total_ovr_size = 0L;
    putstr(win, 0, "");
    putstr(win, 0, "  Overview");
    overview_stats(win, template, &total_ovr_count, &total_ovr_size);
    putstr(win, 0, stats_sep);
    Sprintf(buf, template, "  Over total", total_ovr_count, total_ovr_size);
    putstr(win, 0, buf);

    total_misc_count = total_misc_size = 0L;
    putstr(win, 0, "");
    putstr(win, 0, "  Miscellaneous");
    misc_stats(win, &total_misc_count, &total_misc_size);
    putstr(win, 0, stats_sep);
    Sprintf(buf, template, "  Misc total", total_misc_count, total_misc_size);
    putstr(win, 0, buf);

    putstr(win, 0, "");
    putstr(win, 0, stats_sep);
    Sprintf(buf, template, "  Grand total",
            (total_obj_count + total_mon_count
             + total_ovr_count + total_misc_count),
            (total_obj_size + total_mon_size
             + total_ovr_size + total_misc_size));
    putstr(win, 0, buf);

#if defined(__BORLANDC__) && !defined(_WIN32)
    show_borlandc_stats(win);
#endif

    display_nhwindow(win, FALSE);
    destroy_nhwindow(win);
    return 0;
}

void
sanity_check()
{
    obj_sanity_check();
    timer_sanity_check();
    mon_sanity_check();
    light_sources_sanity_check();
    bc_sanity_check();
}

#ifdef DEBUG_MIGRATING_MONS
static int
wiz_migrate_mons()
{
    int mcount = 0;
    char inbuf[BUFSZ] = DUMMY;
    struct permonst *ptr;
    struct monst *mtmp;
    d_level tolevel;

    getlin("How many random monsters to migrate? [0]", inbuf);
    if (*inbuf == '\033')
        return 0;
    mcount = atoi(inbuf);
    if (mcount < 0 || mcount > (COLNO * ROWNO) || Is_botlevel(&u.uz))
        return 0;
    while (mcount > 0) {
        if (Is_stronghold(&u.uz))
            assign_level(&tolevel, &valley_level);
        else
            get_level(&tolevel, depth(&u.uz) + 1);
        ptr = rndmonst();
        mtmp = makemon(ptr, 0, 0, NO_MM_FLAGS);
        if (mtmp)
            migrate_to_level(mtmp, ledger_no(&tolevel), MIGR_RANDOM,
                             (coord *) 0);
        mcount--;
    }
    return 0;
}
#endif

struct {
    int nhkf;
    char key;
    const char *name;
} const spkeys_binds[] = {
    { NHKF_ESC,              '\033', (char *) 0 }, /* no binding */
    { NHKF_DOAGAIN,          DOAGAIN, "repeat" },
    { NHKF_REQMENU,          'm', "reqmenu" },
    { NHKF_RUN,              'G', "run" },
    { NHKF_RUN2,             '5', "run.numpad" },
    { NHKF_RUSH,             'g', "rush" },
    { NHKF_FIGHT,            'F', "fight" },
    { NHKF_FIGHT2,           '-', "fight.numpad" },
    { NHKF_NOPICKUP,         'm', "nopickup" },
    { NHKF_RUN_NOPICKUP,     'M', "run.nopickup" },
    { NHKF_DOINV,            '0', "doinv" },
    { NHKF_TRAVEL,           CMD_TRAVEL, (char *) 0 }, /* no binding */
    { NHKF_CLICKLOOK,        CMD_CLICKLOOK, (char *) 0 }, /* no binding */
    { NHKF_REDRAW,           C('r'), "redraw" },
    { NHKF_REDRAW2,          C('l'), "redraw.numpad" },
    { NHKF_GETDIR_SELF,      '.', "getdir.self" },
    { NHKF_GETDIR_SELF2,     's', "getdir.self2" },
    { NHKF_GETDIR_HELP,      '?', "getdir.help" },
    { NHKF_COUNT,            'n', "count" },
    { NHKF_GETPOS_SELF,      '@', "getpos.self" },
    { NHKF_GETPOS_PICK,      '.', "getpos.pick" },
    { NHKF_GETPOS_PICK_Q,    ',', "getpos.pick.quick" },
    { NHKF_GETPOS_PICK_O,    ';', "getpos.pick.once" },
    { NHKF_GETPOS_PICK_V,    ':', "getpos.pick.verbose" },
    { NHKF_GETPOS_SHOWVALID, '$', "getpos.valid" },
    { NHKF_GETPOS_AUTODESC,  '#', "getpos.autodescribe" },
    { NHKF_GETPOS_MON_NEXT,  'm', "getpos.mon.next" },
    { NHKF_GETPOS_MON_PREV,  'M', "getpos.mon.prev" },
    { NHKF_GETPOS_OBJ_NEXT,  'o', "getpos.obj.next" },
    { NHKF_GETPOS_OBJ_PREV,  'O', "getpos.obj.prev" },
    { NHKF_GETPOS_DOOR_NEXT, 'd', "getpos.door.next" },
    { NHKF_GETPOS_DOOR_PREV, 'D', "getpos.door.prev" },
    { NHKF_GETPOS_UNEX_NEXT, 'x', "getpos.unexplored.next" },
    { NHKF_GETPOS_UNEX_PREV, 'X', "getpos.unexplored.prev" },
    { NHKF_GETPOS_VALID_NEXT, 'z', "getpos.valid.next" },
    { NHKF_GETPOS_VALID_PREV, 'Z', "getpos.valid.prev" },
    { NHKF_GETPOS_INTERESTING_NEXT, 'a', "getpos.all.next" },
    { NHKF_GETPOS_INTERESTING_PREV, 'A', "getpos.all.prev" },
    { NHKF_GETPOS_HELP,      '?', "getpos.help" },
    { NHKF_GETPOS_LIMITVIEW, '"', "getpos.filter" },
    { NHKF_GETPOS_MOVESKIP,  '*', "getpos.moveskip" },
    { NHKF_GETPOS_MENU,      '!', "getpos.menu" }
};

boolean
bind_specialkey(key, command)
uchar key;
const char *command;
{
    int i;
    for (i = 0; i < SIZE(spkeys_binds); i++) {
        if (!spkeys_binds[i].name || strcmp(command, spkeys_binds[i].name))
            continue;
        Cmd.spkeys[spkeys_binds[i].nhkf] = key;
        return TRUE;
    }
    return FALSE;
}

/* returns a one-byte character from the text (it may massacre the txt
 * buffer) */
char
txt2key(txt)
char *txt;
{
    txt = trimspaces(txt);
    if (!*txt)
        return '\0';

    /* simple character */
    if (!txt[1])
        return txt[0];

    /* a few special entries */
    if (!strcmp(txt, "<enter>"))
        return '\n';
    if (!strcmp(txt, "<space>"))
        return ' ';
    if (!strcmp(txt, "<esc>"))
        return '\033';

    /* control and meta keys */
    switch (*txt) {
    case 'm': /* can be mx, Mx, m-x, M-x */
    case 'M':
        txt++;
        if (*txt == '-' && txt[1])
            txt++;
        if (txt[1])
            return '\0';
        return M(*txt);
    case 'c': /* can be cx, Cx, ^x, c-x, C-x, ^-x */
    case 'C':
    case '^':
        txt++;
        if (*txt == '-' && txt[1])
            txt++;
        if (txt[1])
            return '\0';
        return C(*txt);
    }

    /* ascii codes: must be three-digit decimal */
    if (*txt >= '0' && *txt <= '9') {
        uchar key = 0;
        int i;

        for (i = 0; i < 3; i++) {
            if (txt[i] < '0' || txt[i] > '9')
                return '\0';
            key = 10 * key + txt[i] - '0';
        }
        return key;
    }

    return '\0';
}

/* returns the text for a one-byte encoding;
 * must be shorter than a tab for proper formatting */
char *
key2txt(c, txt)
uchar c;
char *txt; /* sufficiently long buffer */
{
    /* should probably switch to "SPC", "ESC", "RET"
       since nethack's documentation uses ESC for <escape> */
    if (c == ' ')
        Sprintf(txt, "<space>");
    else if (c == '\033')
        Sprintf(txt, "<esc>");
    else if (c == '\n')
        Sprintf(txt, "<enter>");
    else if (c == '\177')
        Sprintf(txt, "<del>"); /* "<delete>" won't fit */
    else
        Strcpy(txt, visctrl((char) c));
    return txt;
}


void
parseautocomplete(autocomplete, condition)
char *autocomplete;
boolean condition;
{
    struct ext_func_tab *efp;
    register char *autoc;

    /* break off first autocomplete from the rest; parse the rest */
    if ((autoc = index(autocomplete, ',')) != 0
        || (autoc = index(autocomplete, ':')) != 0) {
        *autoc++ = '\0';
        parseautocomplete(autoc, condition);
    }

    /* strip leading and trailing white space */
    autocomplete = trimspaces(autocomplete);

    if (!*autocomplete)
        return;

    /* take off negation */
    if (*autocomplete == '!') {
        /* unlike most options, a leading "no" might actually be a part of
         * the extended command.  Thus you have to use ! */
        autocomplete++;
        autocomplete = trimspaces(autocomplete);
        condition = !condition;
    }

    /* find and modify the extended command */
    for (efp = extcmdlist; efp->ef_txt; efp++) {
        if (!strcmp(autocomplete, efp->ef_txt)) {
            if (condition)
                efp->flags |= AUTOCOMPLETE;
            else
                efp->flags &= ~AUTOCOMPLETE;
            return;
        }
    }

    /* not a real extended command */
    raw_printf("Bad autocomplete: invalid extended command '%s'.",
               autocomplete);
    wait_synch();
}

/* called at startup and after number_pad is twiddled */
void
reset_commands(initial)
boolean initial;
{
    static const char sdir[] = "hykulnjb><",
                      sdir_swap_yz[] = "hzkulnjb><",
                      ndir[] = "47896321><",
                      ndir_phone_layout[] = "41236987><";
    static const int ylist[] = {
        'y', 'Y', C('y'), M('y'), M('Y'), M(C('y'))
    };
    static struct ext_func_tab *back_dir_cmd[8];
    const struct ext_func_tab *cmdtmp;
    boolean flagtemp;
    int c, i, updated = 0;
    static boolean backed_dir_cmd = FALSE;

    if (initial) {
        updated = 1;
        Cmd.num_pad = FALSE;
        Cmd.pcHack_compat = Cmd.phone_layout = Cmd.swap_yz = FALSE;
        for (i = 0; i < SIZE(spkeys_binds); i++)
            Cmd.spkeys[spkeys_binds[i].nhkf] = spkeys_binds[i].key;
        commands_init();
    } else {

        if (backed_dir_cmd) {
            for (i = 0; i < 8; i++) {
                Cmd.commands[(uchar) Cmd.dirchars[i]] = back_dir_cmd[i];
            }
        }

        /* basic num_pad */
        flagtemp = iflags.num_pad;
        if (flagtemp != Cmd.num_pad) {
            Cmd.num_pad = flagtemp;
            ++updated;
        }
        /* swap_yz mode (only applicable for !num_pad); intended for
           QWERTZ keyboard used in Central Europe, particularly Germany */
        flagtemp = (iflags.num_pad_mode & 1) ? !Cmd.num_pad : FALSE;
        if (flagtemp != Cmd.swap_yz) {
            Cmd.swap_yz = flagtemp;
            ++updated;
            /* Cmd.swap_yz has been toggled;
               perform the swap (or reverse previous one) */
            for (i = 0; i < SIZE(ylist); i++) {
                c = ylist[i] & 0xff;
                cmdtmp = Cmd.commands[c];              /* tmp = [y] */
                Cmd.commands[c] = Cmd.commands[c + 1]; /* [y] = [z] */
                Cmd.commands[c + 1] = cmdtmp;          /* [z] = tmp */
            }
        }
        /* MSDOS compatibility mode (only applicable for num_pad) */
        flagtemp = (iflags.num_pad_mode & 1) ? Cmd.num_pad : FALSE;
        if (flagtemp != Cmd.pcHack_compat) {
            Cmd.pcHack_compat = flagtemp;
            ++updated;
            /* pcHack_compat has been toggled */
            c = M('5') & 0xff;
            cmdtmp = Cmd.commands['5'];
            Cmd.commands['5'] = Cmd.commands[c];
            Cmd.commands[c] = cmdtmp;
            c = M('0') & 0xff;
            Cmd.commands[c] = Cmd.pcHack_compat ? Cmd.commands['I'] : 0;
        }
        /* phone keypad layout (only applicable for num_pad) */
        flagtemp = (iflags.num_pad_mode & 2) ? Cmd.num_pad : FALSE;
        if (flagtemp != Cmd.phone_layout) {
            Cmd.phone_layout = flagtemp;
            ++updated;
            /* phone_layout has been toggled */
            for (i = 0; i < 3; i++) {
                c = '1' + i;             /* 1,2,3 <-> 7,8,9 */
                cmdtmp = Cmd.commands[c];              /* tmp = [1] */
                Cmd.commands[c] = Cmd.commands[c + 6]; /* [1] = [7] */
                Cmd.commands[c + 6] = cmdtmp;          /* [7] = tmp */
                c = (M('1') & 0xff) + i; /* M-1,M-2,M-3 <-> M-7,M-8,M-9 */
                cmdtmp = Cmd.commands[c];              /* tmp = [M-1] */
                Cmd.commands[c] = Cmd.commands[c + 6]; /* [M-1] = [M-7] */
                Cmd.commands[c + 6] = cmdtmp;          /* [M-7] = tmp */
            }
        }
    } /*?initial*/

    if (updated)
        Cmd.serialno++;
    Cmd.dirchars = !Cmd.num_pad
                       ? (!Cmd.swap_yz ? sdir : sdir_swap_yz)
                       : (!Cmd.phone_layout ? ndir : ndir_phone_layout);
    Cmd.alphadirchars = !Cmd.num_pad ? Cmd.dirchars : sdir;

    Cmd.move_W = Cmd.dirchars[0];
    Cmd.move_NW = Cmd.dirchars[1];
    Cmd.move_N = Cmd.dirchars[2];
    Cmd.move_NE = Cmd.dirchars[3];
    Cmd.move_E = Cmd.dirchars[4];
    Cmd.move_SE = Cmd.dirchars[5];
    Cmd.move_S = Cmd.dirchars[6];
    Cmd.move_SW = Cmd.dirchars[7];

    if (!initial) {
        for (i = 0; i < 8; i++) {
            back_dir_cmd[i] =
                (struct ext_func_tab *) Cmd.commands[(uchar) Cmd.dirchars[i]];
            Cmd.commands[(uchar) Cmd.dirchars[i]] = (struct ext_func_tab *) 0;
        }
        backed_dir_cmd = TRUE;
        for (i = 0; i < 8; i++)
            (void) bind_key(Cmd.dirchars[i], "nothing");
    }
}

/* non-movement commands which accept 'm' prefix to request menu operation */
STATIC_OVL boolean
accept_menu_prefix(cmd_func)
int NDECL((*cmd_func));
{
    if (cmd_func == dopickup || cmd_func == dotip
        /* eat, #offer, and apply tinning-kit all use floorfood() to pick
           an item on floor or in invent; 'm' skips picking from floor
           (ie, inventory only) rather than request use of menu operation */
        || cmd_func == doeat || cmd_func == dosacrifice || cmd_func == doapply
        /* 'm' for removing saddle from adjacent monster without checking
           for containers at <u.ux,u.uy> */
        || cmd_func == doloot
        /* travel: pop up a menu of interesting targets in view */
        || cmd_func == dotravel
        /* wizard mode ^V and ^T */
        || cmd_func == wiz_level_tele || cmd_func == dotelecmd
        /* 'm' prefix allowed for some extended commands */
        || cmd_func == doextcmd || cmd_func == doextlist)
        return TRUE;
    return FALSE;
}

char
randomkey()
{
    static unsigned i = 0;
    char c;

    switch (rn2(16)) {
    default:
        c = '\033';
        break;
    case 0:
        c = '\n';
        break;
    case 1:
    case 2:
    case 3:
    case 4:
        c = (char) rn1('~' - ' ' + 1, ' ');
        break;
    case 5:
        c = (char) (rn2(2) ? '\t' : ' ');
        break;
    case 6:
        c = (char) rn1('z' - 'a' + 1, 'a');
        break;
    case 7:
        c = (char) rn1('Z' - 'A' + 1, 'A');
        break;
    case 8:
        c = extcmdlist[i++ % SIZE(extcmdlist)].key;
        break;
    case 9:
        c = '#';
        break;
    case 10:
    case 11:
    case 12:
        c = Cmd.dirchars[rn2(8)];
        if (!rn2(7))
            c = !Cmd.num_pad ? (!rn2(3) ? C(c) : (c + 'A' - 'a')) : M(c);
        break;
    case 13:
        c = (char) rn1('9' - '0' + 1, '0');
        break;
    case 14:
        /* any char, but avoid '\0' because it's used for mouse click */
        c = (char) rnd(iflags.wc_eight_bit_input ? 255 : 127);
        break;
    }

    return c;
}

void
random_response(buf, sz)
char *buf;
int sz;
{
    char c;
    int count = 0;

    for (;;) {
        c = randomkey();
        if (c == '\n')
            break;
        if (c == '\033') {
            count = 0;
            break;
        }
        if (count < sz - 1)
            buf[count++] = c;
    }
    buf[count] = '\0';
}

int
rnd_extcmd_idx(VOID_ARGS)
{
    return rn2(extcmdlist_length + 1) - 1;
}

int
ch2spkeys(c, start, end)
char c;
int start,end;
{
    int i;

    for (i = start; i <= end; i++)
        if (Cmd.spkeys[i] == c)
            return i;
    return NHKF_ESC;
}

void
rhack(cmd)
register char *cmd;
{
    int spkey;
    boolean prefix_seen, bad_command,
        firsttime = (cmd == 0);

    iflags.menu_requested = FALSE;
#ifdef SAFERHANGUP
    if (program_state.done_hup)
        end_of_input();
#endif
    if (firsttime) {
        context.nopick = 0;
        cmd = parse();
    }
    if (*cmd == Cmd.spkeys[NHKF_ESC]) {
        context.move = FALSE;
        return;
    }
    if (*cmd == DOAGAIN && !in_doagain && saveq[0]) {
        in_doagain = TRUE;
        stail = 0;
        rhack((char *) 0); /* read and execute command */
        in_doagain = FALSE;
        return;
    }
    /* Special case of *cmd == ' ' handled better below */
    if (!*cmd || *cmd == (char) 0377) {
        nhbell();
        context.move = FALSE;
        return; /* probably we just had an interrupt */
    }

    /* handle most movement commands */
    prefix_seen = FALSE;
    context.travel = context.travel1 = 0;
    spkey = ch2spkeys(*cmd, NHKF_RUN, NHKF_CLICKLOOK);

    switch (spkey) {
    case NHKF_RUSH:
        if (movecmd(cmd[1])) {
            context.run = 2;
            domove_attempting |= DOMOVE_RUSH;
        } else
            prefix_seen = TRUE;
        break;
    case NHKF_RUN2:
        if (!Cmd.num_pad)
            break;
        /*FALLTHRU*/
    case NHKF_RUN:
        if (movecmd(lowc(cmd[1]))) {
            context.run = 3;
            domove_attempting |= DOMOVE_RUSH;
        } else
            prefix_seen = TRUE;
        break;
    case NHKF_FIGHT2:
        if (!Cmd.num_pad)
            break;
        /*FALLTHRU*/
    /* Effects of movement commands and invisible monsters:
     * m: always move onto space (even if 'I' remembered)
     * F: always attack space (even if 'I' not remembered)
     * normal movement: attack if 'I', move otherwise.
     */
    case NHKF_FIGHT:
        if (movecmd(cmd[1])) {
            context.forcefight = 1;
            domove_attempting |= DOMOVE_WALK;
        } else
            prefix_seen = TRUE;
        break;
    case NHKF_NOPICKUP:
        if (movecmd(cmd[1]) || u.dz) {
            context.run = 0;
            context.nopick = 1;
            if (!u.dz)
                domove_attempting |= DOMOVE_WALK;
            else
                cmd[0] = cmd[1]; /* "m<" or "m>" */
        } else
            prefix_seen = TRUE;
        break;
    case NHKF_RUN_NOPICKUP:
        if (movecmd(lowc(cmd[1]))) {
            context.run = 1;
            context.nopick = 1;
            domove_attempting |= DOMOVE_RUSH;
        } else
            prefix_seen = TRUE;
        break;
    case NHKF_DOINV:
        if (!Cmd.num_pad)
            break;
        (void) ddoinv(); /* a convenience borrowed from the PC */
        context.move = FALSE;
        multi = 0;
        return;
    case NHKF_CLICKLOOK:
        if (iflags.clicklook) {
            context.move = FALSE;
            do_look(2, &clicklook_cc);
        }
        return;
    case NHKF_TRAVEL:
        if (flags.travelcmd) {
            context.travel = 1;
            context.travel1 = 1;
            context.run = 8;
            context.nopick = 1;
            domove_attempting |= DOMOVE_RUSH;
            break;
        }
        /*FALLTHRU*/
    default:
        if (movecmd(*cmd)) { /* ordinary movement */
            context.run = 0; /* only matters here if it was 8 */
            domove_attempting |= DOMOVE_WALK;
        } else if (movecmd(Cmd.num_pad ? unmeta(*cmd) : lowc(*cmd))) {
            context.run = 1;
            domove_attempting |= DOMOVE_RUSH;
        } else if (movecmd(unctrl(*cmd))) {
            context.run = 3;
            domove_attempting |= DOMOVE_RUSH;
        }
        break;
    }

    /* some special prefix handling */
    /* overload 'm' prefix to mean "request a menu" */
    if (prefix_seen && cmd[0] == Cmd.spkeys[NHKF_REQMENU]) {
        /* (for func_tab cast, see below) */
        const struct ext_func_tab *ft = Cmd.commands[cmd[1] & 0xff];
        int NDECL((*func)) = ft ? ((struct ext_func_tab *) ft)->ef_funct : 0;

        if (func && accept_menu_prefix(func)) {
            iflags.menu_requested = TRUE;
            ++cmd;
        }
    }

    if (((domove_attempting & (DOMOVE_RUSH | DOMOVE_WALK)) != 0L)
                            && !context.travel && !dxdy_moveok()) {
        /* trying to move diagonally as a grid bug;
           this used to be treated by movecmd() as not being
           a movement attempt, but that didn't provide for any
           feedback and led to strangeness if the key pressed
           ('u' in particular) was overloaded for num_pad use */
        /*KR You_cant("get there from here..."); */
        You_cant("여기서 거기로는 갈 수 없다...");
        context.run = 0;
        context.nopick = context.forcefight = FALSE;
        context.move = context.mv = FALSE;
        multi = 0;
        return;
    }

    if ((domove_attempting & DOMOVE_WALK) != 0L) {
        if (multi)
            context.mv = TRUE;
        domove();
        context.forcefight = 0;
        return;
    } else if ((domove_attempting & DOMOVE_RUSH) != 0L) {
        if (firsttime) {
            if (!multi)
                multi = max(COLNO, ROWNO);
            u.last_str_turn = 0;
        }
        context.mv = TRUE;
        domove();
        return;
    } else if (prefix_seen && cmd[1] == Cmd.spkeys[NHKF_ESC]) {
        /* <prefix><escape> */
        /* don't report "unknown command" for change of heart... */
        bad_command = FALSE;
    } else if (*cmd == ' ' && !flags.rest_on_space) {
        bad_command = TRUE; /* skip cmdlist[] loop */

    /* handle all other commands */
    } else {
        register const struct ext_func_tab *tlist;
        int res, NDECL((*func));

        /* current - use *cmd to directly index cmdlist array */
        if ((tlist = Cmd.commands[*cmd & 0xff]) != 0) {
            if (!wizard && (tlist->flags & WIZMODECMD)) {
                /*KR You_cant("do that!"); */
                pline("그럴 수는 없다!");
                res = 0;
            } else if (u.uburied && !(tlist->flags & IFBURIED)) {
                /*KR You_cant("do that while you are buried!"); */
                You("파묻힌 상태에서는 그럴 수 없다!");
                res = 0;
            } else {
                /* we discard 'const' because some compilers seem to have
                   trouble with the pointer passed to set_occupation() */
                func = ((struct ext_func_tab *) tlist)->ef_funct;
                if (tlist->f_text && !occupation && multi)
                    set_occupation(func, tlist->f_text, multi);
                res = (*func)(); /* perform the command */
            }
            if (!res) {
                context.move = FALSE;
                multi = 0;
            }
            return;
        }
        /* if we reach here, cmd wasn't found in cmdlist[] */
        bad_command = TRUE;
    }

    if (bad_command) {
        char expcmd[20]; /* we expect 'cmd' to point to 1 or 2 chars */
        char c, c1 = cmd[1];

        expcmd[0] = '\0';
        while ((c = *cmd++) != '\0')
            Strcat(expcmd, visctrl(c)); /* add 1..4 chars plus terminator */

   /*KR if (!prefix_seen || !help_dir(c1, spkey, "Invalid direction key!")) */
        if (!prefix_seen || !help_dir(c1, spkey, "잘못된 방향키입니다!"))
            /*KR Norep("Unknown command '%s'.", expcmd); */
            Norep("알 수 없는 명령어 '%s'.", expcmd);
    }
    /* didn't move */
    context.move = FALSE;
    multi = 0;
    return;
}

/* convert an x,y pair into a direction code */
int
xytod(x, y)
schar x, y;
{
    register int dd;

    for (dd = 0; dd < 8; dd++)
        if (x == xdir[dd] && y == ydir[dd])
            return dd;
    return -1;
}

/* convert a direction code into an x,y pair */
void
dtoxy(cc, dd)
coord *cc;
register int dd;
{
    cc->x = xdir[dd];
    cc->y = ydir[dd];
    return;
}

/* also sets u.dz, but returns false for <> */
int
movecmd(sym)
char sym;
{
    register const char *dp = index(Cmd.dirchars, sym);

    u.dz = 0;
    if (!dp || !*dp)
        return 0;
    u.dx = xdir[dp - Cmd.dirchars];
    u.dy = ydir[dp - Cmd.dirchars];
    u.dz = zdir[dp - Cmd.dirchars];
#if 0 /* now handled elsewhere */
    if (u.dx && u.dy && NODIAG(u.umonnum)) {
        u.dx = u.dy = 0;
        return 0;
    }
#endif
    return !u.dz;
}

/* grid bug handling which used to be in movecmd() */
int
dxdy_moveok()
{
    if (u.dx && u.dy && NODIAG(u.umonnum))
        u.dx = u.dy = 0;
    return u.dx || u.dy;
}

/* decide whether a character (user input keystroke) requests screen repaint */
boolean
redraw_cmd(c)
char c;
{
    return (boolean) (c == Cmd.spkeys[NHKF_REDRAW]
                      || (Cmd.num_pad && c == Cmd.spkeys[NHKF_REDRAW2]));
}

boolean
prefix_cmd(c)
char c;
{
    return (c == Cmd.spkeys[NHKF_RUSH]
            || c == Cmd.spkeys[NHKF_RUN]
            || c == Cmd.spkeys[NHKF_NOPICKUP]
            || c == Cmd.spkeys[NHKF_RUN_NOPICKUP]
            || c == Cmd.spkeys[NHKF_FIGHT]
            || (Cmd.num_pad && (c == Cmd.spkeys[NHKF_RUN2]
                                || c == Cmd.spkeys[NHKF_FIGHT2])));
}

/*
 * uses getdir() but unlike getdir() it specifically
 * produces coordinates using the direction from getdir()
 * and verifies that those coordinates are ok.
 *
 * If the call to getdir() returns 0, Never_mind is displayed.
 * If the resulting coordinates are not okay, emsg is displayed.
 *
 * Returns non-zero if coordinates in cc are valid.
 */
int
get_adjacent_loc(prompt, emsg, x, y, cc)
const char *prompt, *emsg;
xchar x, y;
coord *cc;
{
    xchar new_x, new_y;
    if (!getdir(prompt)) {
        pline1(Never_mind);
        return 0;
    }
    new_x = x + u.dx;
    new_y = y + u.dy;
    if (cc && isok(new_x, new_y)) {
        cc->x = new_x;
        cc->y = new_y;
    } else {
        if (emsg)
            pline1(emsg);
        return 0;
    }
    return 1;
}

int
getdir(s)
const char *s;
{
    char dirsym;
    int is_mov;

 retry:
    if (in_doagain || *readchar_queue)
        dirsym = readchar();
    else
   /*KR dirsym = yn_function((s && *s != '^') ? s : "In what direction?", */
        dirsym = yn_function((s && *s != '^') ? s : "어느 방향으로?",
                             (char *) 0, '\0');
    /* remove the prompt string so caller won't have to */
    clear_nhwindow(WIN_MESSAGE);

    if (redraw_cmd(dirsym)) { /* ^R */
        docrt();              /* redraw */
        goto retry;
    }
    savech(dirsym);

    if (dirsym == Cmd.spkeys[NHKF_GETDIR_SELF]
        || dirsym == Cmd.spkeys[NHKF_GETDIR_SELF2]) {
        u.dx = u.dy = u.dz = 0;
    } else if (!(is_mov = movecmd(dirsym)) && !u.dz) {
        boolean did_help = FALSE, help_requested;

        if (!index(quitchars, dirsym)) {
            help_requested = (dirsym == Cmd.spkeys[NHKF_GETDIR_HELP]);
            if (help_requested || iflags.cmdassist) {
                did_help =
                    help_dir((s && *s == '^') ? dirsym : '\0', NHKF_ESC,
                             help_requested
                                 ? (const char *) 0
                                 /*KR : "Invalid direction key!"); */
                                 : "잘못된 방향키입니다!");
                if (help_requested)
                    goto retry;
            }
            if (!did_help)
                /*KR pline("What a strange direction!"); */
                pline("정말 이상한 방향이다!");
        }
        return 0;
    } else if (is_mov && !dxdy_moveok()) {
        /*KR You_cant("orient yourself that direction."); */
        You_cant("그 방향으로는 몸을 틀 수 없다.");
        return 0;
    }
    if (!u.dz && (Stunned || (Confusion && !rn2(5))))
        confdir();
    return 1;
}

STATIC_OVL void
show_direction_keys(win, centerchar, nodiag)
winid win; /* should specify a window which is using a fixed-width font... */
char centerchar; /* '.' or '@' or ' ' */
boolean nodiag;
{
    char buf[BUFSZ];

    if (!centerchar)
        centerchar = ' ';

    if (nodiag) {
        Sprintf(buf, "             %c   ", Cmd.move_N);
        putstr(win, 0, buf);
        putstr(win, 0, "             |   ");
        Sprintf(buf, "          %c- %c -%c",
                Cmd.move_W, centerchar, Cmd.move_E);
        putstr(win, 0, buf);
        putstr(win, 0, "             |   ");
        Sprintf(buf, "             %c   ", Cmd.move_S);
        putstr(win, 0, buf);
    } else {
        Sprintf(buf, "          %c  %c  %c",
                Cmd.move_NW, Cmd.move_N, Cmd.move_NE);
        putstr(win, 0, buf);
        putstr(win, 0, "           \\ | / ");
        Sprintf(buf, "          %c- %c -%c",
                Cmd.move_W, centerchar, Cmd.move_E);
        putstr(win, 0, buf);
        putstr(win, 0, "           / | \\ ");
        Sprintf(buf, "          %c  %c  %c",
                Cmd.move_SW, Cmd.move_S, Cmd.move_SE);
        putstr(win, 0, buf);
    };
}

/* explain choices if player has asked for getdir() help or has given
   an invalid direction after a prefix key ('F', 'g', 'm', &c), which
   might be bogus but could be up, down, or self when not applicable */
STATIC_OVL boolean
help_dir(sym, spkey, msg)
char sym;
int spkey; /* NHKF_ code for prefix key, if one was used, or for ESC */
const char *msg;
{
    static const char wiz_only_list[] = "EFGIVW";
    char ctrl;
    winid win;
    char buf[BUFSZ], buf2[BUFSZ], *explain;
    const char *dothat, *how;
    boolean prefixhandling, viawindow;

    /* NHKF_ESC indicates that player asked for help at getdir prompt */
    viawindow = (spkey == NHKF_ESC || iflags.cmdassist);
    prefixhandling = (spkey != NHKF_ESC);
    /*
     * Handling for prefix keys that don't want special directions.
     * Delivered via pline if 'cmdassist' is off, or instead of the
     * general message if it's on.
     */
    /*KR dothat = "do that"; */
    dothat = "그렇게 할";
    /*KR how = " at"; */
    how = "을 향해"; /* for "<action> at yourself"; not used for up/down */
    switch (spkey) {
    case NHKF_NOPICKUP:
        /*KR dothat = "move"; */
        dothat = "이동할";
        break;
    case NHKF_RUSH:
        /*KR dothat = "rush"; */
        dothat = "돌진할";
        break;
    case NHKF_RUN2:
        if (!Cmd.num_pad)
            break;
        /*FALLTHRU*/
    case NHKF_RUN:
    case NHKF_RUN_NOPICKUP:
        /*KR dothat = "run"; */
        dothat = "달릴";
        break;
    case NHKF_FIGHT2:
        if (!Cmd.num_pad)
            break;
        /*FALLTHRU*/
    case NHKF_FIGHT:
        /*KR dothat = "fight"; */
        dothat = "공격할";
        /*KR how = ""; */
        how = "을"; /* avoid "fight at yourself" -> 자신'을' 공격 */
        break;
    default:
        prefixhandling = FALSE;
        break;
    }

    buf[0] = '\0';
    /* for movement prefix followed by '.' or (numpad && 's') to mean 'self';
       note: '-' for hands (inventory form of 'self') is not handled here */
    if (prefixhandling
        && (sym == Cmd.spkeys[NHKF_GETDIR_SELF]
            || (Cmd.num_pad && sym == Cmd.spkeys[NHKF_GETDIR_SELF2]))) {
#if 0 /*KR: 원본*/
        Sprintf(buf, "You can't %s%s yourself.", dothat, how);
#else
        Sprintf(buf, "자신%s %s 수는 없다.", how, dothat);
#endif
        /* for movement prefix followed by up or down */
    } else if (prefixhandling && (sym == '<' || sym == '>')) {
#if 0 /*KR: 원본*/
        Sprintf(buf, "You can't %s %s.", dothat,
                /* was "upwards" and "downwards", but they're considered
                   to be variants of canonical "upward" and "downward" */
                (sym == '<') ? "upward" : "downward");
#else
        Sprintf(buf, "%s %s 수는 없다.", (sym == '<') ? "위로" : "아래로",
                dothat);
#endif
    }

    /* if '!cmdassist', display via pline() and we're done (note: asking
       for help at getdir() prompt forces cmdassist for this operation) */
    if (!viawindow) {
        if (prefixhandling) {
            if (!*buf)
#if 0 /*KR: 원본*/
                Sprintf(buf, "Invalid direction for '%s' prefix.",
                        visctrl(Cmd.spkeys[spkey]));
#else
                Sprintf(buf, "'%s' 접두사에 대해 잘못된 방향입니다.",
                        visctrl(Cmd.spkeys[spkey]));
#endif
            pline("%s", buf);
            return TRUE;
        }
        /* when 'cmdassist' is off and caller doesn't insist, do nothing */
        return FALSE;
    }

win = create_nhwindow(NHW_TEXT);
    if (!win)
        return FALSE;

    if (*buf) {
        /* show bad-prefix message instead of general invalid-direction one */
        putstr(win, 0, buf);
        putstr(win, 0, "");
    } else if (msg) {
        Sprintf(buf, "cmdassist: %s", msg);
        putstr(win, 0, buf);
        putstr(win, 0, "");
    }

    if (!prefixhandling && (letter(sym) || sym == '[')) {
        /* '[': old 'cmdhelp' showed ESC as ^[ */
        sym = highc(sym);       /* @A-Z[ (note: letter() accepts '@') */
        ctrl = (sym - 'A') + 1; /* 0-27 (note: 28-31 aren't applicable) */
        if ((explain = dowhatdoes_core(ctrl, buf2)) != 0
            && (!index(wiz_only_list, sym) || wizard)) {
#if 0 /*KR: 원본 (한국어 어순을 위해 매개변수 순서 변경)*/
            Sprintf(buf, "Are you trying to use ^%c%s?", sym,
                    index(wiz_only_list, sym) ? ""
                        : " as specified in the Guidebook");
#else
            Sprintf(buf, "%s^%c 명령어를 사용하려고 하셨습니까?",
                    index(wiz_only_list, sym) ? "" : "가이드북에 명시된 ",
                    sym);
#endif
            putstr(win, 0, buf);
            putstr(win, 0, "");
            putstr(win, 0, explain);
            putstr(win, 0, "");
            /*KR putstr(win, 0, "To use that command, hold down the <Ctrl> key
             * as a shift"); */
            putstr(win, 0,
                   "해당 명령어를 사용하려면 <Ctrl> 키를 누른 상태에서");
            /*KR Sprintf(buf, "and press the <%c> key.", sym); */
            Sprintf(buf, "<%c> 키를 누르십시오.", sym);
            putstr(win, 0, buf);
            putstr(win, 0, "");
        }
    }

#if 0 /*KR: 원본*/
    Sprintf(buf, "Valid direction keys%s%s%s are:",
            prefixhandling ? " to " : "", prefixhandling ? dothat : "",
            NODIAG(u.umonnum) ? " in your current form" : "");
#else /*KR: KRNethack 맞춤 번역 (어순 재배치)*/
    Sprintf(buf, "%s%s 유효한 방향키는 다음과 같습니다:",
            NODIAG(u.umonnum) ? "현재 모습에서 " : "",
            prefixhandling ? dothat : "");
#endif
    putstr(win, 0, buf);
    show_direction_keys(win, !prefixhandling ? '.' : ' ', NODIAG(u.umonnum));

    if (!prefixhandling || spkey == NHKF_NOPICKUP) {
        /* NOPICKUP: unlike the other prefix keys, 'm' allows up/down for
           stair traversal; we won't get here when "m<" or "m>" has been
           given but we include up and down for 'm'+invalid_direction;
           self is excluded as a viable direction for every prefix */
        putstr(win, 0, "");
        /*KR putstr(win, 0, "          <  up"); */
        putstr(win, 0, "          <  위로");
        /*KR putstr(win, 0, "          >  down"); */
        putstr(win, 0, "          >  아래로");
        if (!prefixhandling) {
            int selfi = Cmd.num_pad ? NHKF_GETDIR_SELF2 : NHKF_GETDIR_SELF;

#if 0 /*KR: 원본*/
            Sprintf(buf,   "       %4s  direct at yourself",
                    visctrl(Cmd.spkeys[selfi]));
#else
            Sprintf(buf, "       %4s  자신을 향함",
                    visctrl(Cmd.spkeys[selfi]));
#endif
            putstr(win, 0, buf);
        }
    }

    if (msg) {
        /* non-null msg means that this wasn't an explicit user request */
        putstr(win, 0, "");
#if 0 /*KR: 원본*/
        putstr(win, 0,
               "(Suppress this message with !cmdassist in config file.)");
#else
        putstr(win, 0,
               "(이 메시지를 끄려면 설정 파일에 !cmdassist를 추가하세요.)");
#endif
    }
    display_nhwindow(win, FALSE);
    destroy_nhwindow(win);
    return TRUE;
}

void
confdir()
{
    register int x = NODIAG(u.umonnum) ? 2 * rn2(4) : rn2(8);

    u.dx = xdir[x];
    u.dy = ydir[x];
    return;
}

const char *
directionname(dir)
int dir;
{
#if 0 /*KR: 원본*/
    static NEARDATA const char *const dirnames[] = {
        "west",      "northwest", "north",     "northeast", "east",
        "southeast", "south",     "southwest", "down",      "up",
    };
#else
    static NEARDATA const char *const dirnames[] = {
        "서쪽",   "북서쪽", "북쪽",   "북동쪽", "동쪽",
        "남동쪽", "남쪽",   "남서쪽", "아래",   "위",
    };
#endif

    if (dir < 0 || dir >= SIZE(dirnames))
        /*KR return "invalid"; */
        return "잘못된 방향";
    return dirnames[dir];
}

int
isok(x, y)
register int x, y;
{
    /* x corresponds to curx, so x==1 is the first column. Ach. %% */
    return x >= 1 && x <= COLNO - 1 && y >= 0 && y <= ROWNO - 1;
}

/* #herecmdmenu command */
STATIC_PTR int
doherecmdmenu(VOID_ARGS)
{
    char ch = here_cmd_menu(TRUE);

    return ch ? 1 : 0;
}

/* #therecmdmenu command, a way to test there_cmd_menu without mouse */
STATIC_PTR int
dotherecmdmenu(VOID_ARGS)
{
    char ch;

    if (!getdir((const char *) 0) || !isok(u.ux + u.dx, u.uy + u.dy))
        return 0;

    if (u.dx || u.dy)
        ch = there_cmd_menu(TRUE, u.ux + u.dx, u.uy + u.dy);
    else
        ch = here_cmd_menu(TRUE);

    return ch ? 1 : 0;
}

STATIC_OVL void
add_herecmd_menuitem(win, func, text)
winid win;
int NDECL((*func));
const char *text;
{
    char ch;
    anything any;

    if ((ch = cmd_from_func(func)) != '\0') {
        any = zeroany;
        any.a_nfunc = func;
        add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE, text, MENU_UNSELECTED);
    }
}

STATIC_OVL char
there_cmd_menu(doit, x, y)
boolean doit;
int x, y;
{
    winid win;
    char ch;
    char buf[BUFSZ];
    schar typ = levl[x][y].typ;
    int npick, K = 0;
    menu_item *picks = (menu_item *) 0;
    struct trap *ttmp;
    struct monst *mtmp;

    win = create_nhwindow(NHW_MENU);
    start_menu(win);

    if (IS_DOOR(typ)) {
        boolean key_or_pick, card;
        int dm = levl[x][y].doormask;

        if ((dm & (D_CLOSED | D_LOCKED))) {
            /*KR add_herecmd_menuitem(win, doopen, "Open the door"), ++K; */
            add_herecmd_menuitem(win, doopen, "문을 연다"), ++K;
            /* unfortunately there's no lknown flag for doors to
               remember the locked/unlocked state */
            key_or_pick = (carrying(SKELETON_KEY) || carrying(LOCK_PICK));
            card = (carrying(CREDIT_CARD) != 0);
            if (key_or_pick || card) {
#if 0 /*KR: 원본*/
                Sprintf(buf, "%sunlock the door",
                        key_or_pick ? "lock or " : "");
                add_herecmd_menuitem(win, doapply, upstart(buf)), ++K;
#else 
                Sprintf(buf, "문을 %s",
                        key_or_pick ? "잠그거나 딴다" : "딴다");
                add_herecmd_menuitem(win, doapply, buf), ++K;
#endif
            }
            /* unfortunately there's no tknown flag for doors (or chests)
               to remember whether a trap had been found */
#if 0 /*KR: 원본*/
            add_herecmd_menuitem(win, dountrap,
                                 "Search the door for a trap"), ++K;
            /* [what about #force?] */
            add_herecmd_menuitem(win, dokick, "Kick the door"), ++K;
        } else if ((dm & D_ISOPEN)) {
            add_herecmd_menuitem(win, doclose, "Close the door"), ++K;
#else
            add_herecmd_menuitem(win, dountrap,
                                 "문에 함정이 있는지 조사한다"), ++K;
            /* [what about #force?] */
            add_herecmd_menuitem(win, dokick, "문을 발로 찬다"), ++K;
        } else if ((dm & D_ISOPEN)) {
            add_herecmd_menuitem(win, doclose, "문을 닫는다"), ++K;
#endif
        }
    }

if (typ <= SCORR)
        /*KR add_herecmd_menuitem(win, dosearch, "Search for secret doors"),
         * ++K; */
        add_herecmd_menuitem(win, dosearch, "비밀 문을 찾는다"), ++K;

    if ((ttmp = t_at(x, y)) != 0 && ttmp->tseen) {
        /*KR add_herecmd_menuitem(win, doidtrap, "Examine trap"), ++K; */
        add_herecmd_menuitem(win, doidtrap, "함정을 조사한다"), ++K;
        if (ttmp->ttyp != VIBRATING_SQUARE)
            /*KR add_herecmd_menuitem(win, dountrap, "Attempt to disarm
             * trap"), ++K; */
            add_herecmd_menuitem(win, dountrap, "함정 해제를 시도한다"), ++K;
    }

    mtmp = m_at(x, y);
    if (mtmp && !canspotmon(mtmp))
        mtmp = 0;
    if (mtmp && which_armor(mtmp, W_SADDLE)) {
        char *mnam =
            x_monnam(mtmp, ARTICLE_THE, (char *) 0, SUPPRESS_SADDLE, FALSE);

        if (!u.usteed) {
            /*KR Sprintf(buf, "Ride %s", mnam); */
            Sprintf(buf, "%s(을)를 탄다", mnam);
            add_herecmd_menuitem(win, doride, buf), ++K;
        }
        /*KR Sprintf(buf, "Remove saddle from %s", mnam); */
        Sprintf(buf, "%s에게서 안장을 벗긴다", mnam);
        add_herecmd_menuitem(win, doloot, buf), ++K;
    }
    if (mtmp && can_saddle(mtmp) && !which_armor(mtmp, W_SADDLE)
        && carrying(SADDLE)) {
        /*KR Sprintf(buf, "Put saddle on %s", mon_nam(mtmp)), ++K; */
        Sprintf(buf, "%s에게 안장을 얹는다", mon_nam(mtmp)), ++K;
        add_herecmd_menuitem(win, doapply, buf);
    }
#if 0
    if (mtmp || glyph_is_invisible(glyph_at(x, y))) {
        /* "Attack %s", mtmp ? mon_nam(mtmp) : "unseen creature" */
    } else {
        /* "Move %s", direction */
    }
#endif

    if (K) {
        /*KR end_menu(win, "What do you want to do?"); */
        end_menu(win, "무엇을 하시겠습니까?");
        npick = select_menu(win, PICK_ONE, &picks);
    } else {
        /*KR pline("No applicable actions."); */
        pline("가능한 행동이 없습니다.");
        npick = 0;
    }
    destroy_nhwindow(win);
    ch = '\0';
    if (npick > 0) {
        int NDECL((*func)) = picks->item.a_nfunc;
        free((genericptr_t) picks);

        if (doit) {
            int ret = (*func)();

            ch = (char) ret;
        } else {
            ch = cmd_from_func(func);
        }
    }
    return ch;
}

STATIC_OVL char
here_cmd_menu(doit)
boolean doit;
{
    winid win;
    char ch;
    char buf[BUFSZ];
    schar typ = levl[u.ux][u.uy].typ;
    int npick;
    menu_item *picks = (menu_item *) 0;

    win = create_nhwindow(NHW_MENU);
    start_menu(win);

if (IS_FOUNTAIN(typ) || IS_SINK(typ)) {
#if 0 /*KR: 원본*/
        Sprintf(buf, "Drink from the %s",
                defsyms[IS_FOUNTAIN(typ) ? S_fountain : S_sink].explanation);
#else
        Sprintf(buf, "%s에서 물을 마신다",
                defsyms[IS_FOUNTAIN(typ) ? S_fountain : S_sink].explanation);
#endif
        add_herecmd_menuitem(win, dodrink, buf);
    }
    if (IS_FOUNTAIN(typ))
        /*KR add_herecmd_menuitem(win, dodip, "Dip something into the
         * fountain"); */
        add_herecmd_menuitem(win, dodip, "분수에 무언가를 담근다");
    if (IS_THRONE(typ))
        /*KR add_herecmd_menuitem(win, dosit, "Sit on the throne"); */
        add_herecmd_menuitem(win, dosit, "왕좌에 앉는다");

    if ((u.ux == xupstair && u.uy == yupstair)
        || (u.ux == sstairs.sx && u.uy == sstairs.sy && sstairs.up)
        || (u.ux == xupladder && u.uy == yupladder)) {
#if 0 /*KR: 원본*/
        Sprintf(buf, "Go up the %s",
                (u.ux == xupladder && u.uy == yupladder)
                ? "ladder" : "stairs");
#else
        Sprintf(buf, "%s(을)를 올라간다",
                (u.ux == xupladder && u.uy == yupladder) ? "사다리" : "계단");
#endif
        add_herecmd_menuitem(win, doup, buf);
    }
    if ((u.ux == xdnstair && u.uy == ydnstair)
        || (u.ux == sstairs.sx && u.uy == sstairs.sy && !sstairs.up)
        || (u.ux == xdnladder && u.uy == ydnladder)) {
#if 0 /*KR: 원본*/
        Sprintf(buf, "Go down the %s",
                (u.ux == xupladder && u.uy == yupladder)
                ? "ladder" : "stairs");
#else
        Sprintf(buf, "%s(을)를 내려간다",
                (u.ux == xupladder && u.uy == yupladder) ? "사다리" : "계단");
#endif
        add_herecmd_menuitem(win, dodown, buf);
    }
    if (u.usteed) { /* another movement choice */
#if 0               /*KR: 원본*/
        Sprintf(buf, "Dismount %s",
                x_monnam(u.usteed, ARTICLE_THE, (char *) 0,
                         SUPPRESS_SADDLE, FALSE));
#else
        Sprintf(buf, "%s에게서 내린다",
                x_monnam(u.usteed, ARTICLE_THE, (char *) 0, SUPPRESS_SADDLE,
                         FALSE));
#endif
        add_herecmd_menuitem(win, doride, buf);
    }
#if 0
    if (Upolyd) { /* before objects */
        Sprintf(buf, "Use %s special ability",
                s_suffix(mons[u.umonnum].mname));
        add_herecmd_menuitem(win, domonability, buf);
    }
#endif

    if (OBJ_AT(u.ux, u.uy)) {
        struct obj *otmp = level.objects[u.ux][u.uy];

#if 0 /*KR: 원본*/
        Sprintf(buf, "Pick up %s", otmp->nexthere ? "items" : doname(otmp));
#else
        Sprintf(buf, "%s(을)를 줍는다",
                otmp->nexthere ? "물건들" : doname(otmp));
#endif
        add_herecmd_menuitem(win, dopickup, buf);

        if (Is_container(otmp)) {
            /*KR Sprintf(buf, "Loot %s", doname(otmp)); */
            Sprintf(buf, "%s(을)를 뒤진다", doname(otmp));
            add_herecmd_menuitem(win, doloot, buf);
        }
        if (otmp->oclass == FOOD_CLASS) {
            /*KR Sprintf(buf, "Eat %s", doname(otmp)); */
            Sprintf(buf, "%s(을)를 먹는다", doname(otmp));
            add_herecmd_menuitem(win, doeat, buf);
        }
    }

    if (invent)
        /*KR add_herecmd_menuitem(win, dodrop, "Drop items"); */
        add_herecmd_menuitem(win, dodrop, "물건들을 내려놓는다");

    /*KR add_herecmd_menuitem(win, donull, "Rest one turn"); */
    add_herecmd_menuitem(win, donull, "한 턴 쉰다");
    /*KR add_herecmd_menuitem(win, dosearch, "Search around you"); */
    add_herecmd_menuitem(win, dosearch, "주변을 탐색한다");
    /*KR add_herecmd_menuitem(win, dolook, "Look at what is here"); */
    add_herecmd_menuitem(win, dolook, "이곳에 무엇이 있는지 살핀다");

    /*KR end_menu(win, "What do you want to do?"); */
    end_menu(win, "무엇을 하시겠습니까?");
    npick = select_menu(win, PICK_ONE, &picks);
    destroy_nhwindow(win);
    ch = '\0';
    if (npick > 0) {
        int NDECL((*func)) = picks->item.a_nfunc;
        free((genericptr_t) picks);

        if (doit) {
            int ret = (*func)();

            ch = (char) ret;
        } else {
            ch = cmd_from_func(func);
        }
    }
    return ch;
}


static NEARDATA int last_multi;

/*
 * convert a MAP window position into a movecmd
 */
const char *
click_to_cmd(x, y, mod)
int x, y, mod;
{
    int dir;
    static char cmd[4];
    cmd[1] = 0;

    if (iflags.clicklook && mod == CLICK_2) {
        clicklook_cc.x = x;
        clicklook_cc.y = y;
        cmd[0] = Cmd.spkeys[NHKF_CLICKLOOK];
        return cmd;
    }

    x -= u.ux;
    y -= u.uy;

    if (flags.travelcmd) {
        if (abs(x) <= 1 && abs(y) <= 1) {
            x = sgn(x), y = sgn(y);
        } else {
            u.tx = u.ux + x;
            u.ty = u.uy + y;
            cmd[0] = Cmd.spkeys[NHKF_TRAVEL];
            return cmd;
        }

        if (x == 0 && y == 0) {
            if (iflags.herecmd_menu) {
                cmd[0] = here_cmd_menu(FALSE);
                return cmd;
            }

            /* here */
            if (IS_FOUNTAIN(levl[u.ux][u.uy].typ)
                || IS_SINK(levl[u.ux][u.uy].typ)) {
                cmd[0] = cmd_from_func(mod == CLICK_1 ? dodrink : dodip);
                return cmd;
            } else if (IS_THRONE(levl[u.ux][u.uy].typ)) {
                cmd[0] = cmd_from_func(dosit);
                return cmd;
            } else if ((u.ux == xupstair && u.uy == yupstair)
                       || (u.ux == sstairs.sx && u.uy == sstairs.sy
                           && sstairs.up)
                       || (u.ux == xupladder && u.uy == yupladder)) {
                cmd[0] = cmd_from_func(doup);
                return cmd;
            } else if ((u.ux == xdnstair && u.uy == ydnstair)
                       || (u.ux == sstairs.sx && u.uy == sstairs.sy
                           && !sstairs.up)
                       || (u.ux == xdnladder && u.uy == ydnladder)) {
                cmd[0] = cmd_from_func(dodown);
                return cmd;
            } else if (OBJ_AT(u.ux, u.uy)) {
                cmd[0] = cmd_from_func(Is_container(level.objects[u.ux][u.uy])
                                       ? doloot : dopickup);
                return cmd;
            } else {
                cmd[0] = cmd_from_func(donull); /* just rest */
                return cmd;
            }
        }

        /* directional commands */

        dir = xytod(x, y);

        if (!m_at(u.ux + x, u.uy + y)
            && !test_move(u.ux, u.uy, x, y, TEST_MOVE)) {
            cmd[1] = Cmd.dirchars[dir];
            cmd[2] = '\0';
            if (iflags.herecmd_menu) {
                cmd[0] = there_cmd_menu(FALSE, u.ux + x, u.uy + y);
                if (cmd[0] == '\0')
                    cmd[1] = '\0';
                return cmd;
            }

            if (IS_DOOR(levl[u.ux + x][u.uy + y].typ)) {
                /* slight assistance to the player: choose kick/open for them
                 */
                if (levl[u.ux + x][u.uy + y].doormask & D_LOCKED) {
                    cmd[0] = cmd_from_func(dokick);
                    return cmd;
                }
                if (levl[u.ux + x][u.uy + y].doormask & D_CLOSED) {
                    cmd[0] = cmd_from_func(doopen);
                    return cmd;
                }
            }
            if (levl[u.ux + x][u.uy + y].typ <= SCORR) {
                cmd[0] = cmd_from_func(dosearch);
                cmd[1] = 0;
                return cmd;
            }
        }
    } else {
        /* convert without using floating point, allowing sloppy clicking */
        if (x > 2 * abs(y))
            x = 1, y = 0;
        else if (y > 2 * abs(x))
            x = 0, y = 1;
        else if (x < -2 * abs(y))
            x = -1, y = 0;
        else if (y < -2 * abs(x))
            x = 0, y = -1;
        else
            x = sgn(x), y = sgn(y);

        if (x == 0 && y == 0) {
            /* map click on player to "rest" command */
            cmd[0] = cmd_from_func(donull);
            return cmd;
        }
        dir = xytod(x, y);
    }

    /* move, attack, etc. */
    cmd[1] = 0;
    if (mod == CLICK_1) {
        cmd[0] = Cmd.dirchars[dir];
    } else {
        cmd[0] = (Cmd.num_pad
                     ? M(Cmd.dirchars[dir])
                     : (Cmd.dirchars[dir] - 'a' + 'A')); /* run command */
    }

    return cmd;
}

char
get_count(allowchars, inkey, maxcount, count, historical)
char *allowchars;
char inkey;
long maxcount;
long *count;
boolean historical; /* whether to include in message history: True => yes */
{
    char qbuf[QBUFSZ];
    int key;
    long cnt = 0L;
    boolean backspaced = FALSE;
    /* this should be done in port code so that we have erase_char
       and kill_char available; we can at least fake erase_char */
#define STANDBY_erase_char '\177'

    for (;;) {
        if (inkey) {
            key = inkey;
            inkey = '\0';
        } else
            key = readchar();

        if (digit(key)) {
            cnt = 10L * cnt + (long) (key - '0');
            if (cnt < 0)
                cnt = 0;
            else if (maxcount > 0 && cnt > maxcount)
                cnt = maxcount;
        } else if (cnt && (key == '\b' || key == STANDBY_erase_char)) {
            cnt = cnt / 10;
            backspaced = TRUE;
        } else if (key == Cmd.spkeys[NHKF_ESC]) {
            break;
        } else if (!allowchars || index(allowchars, key)) {
            *count = cnt;
            break;
        }

if (cnt > 9 || backspaced) {
            clear_nhwindow(WIN_MESSAGE);
            if (backspaced && !cnt) {
                /*KR Sprintf(qbuf, "Count: "); */
                Sprintf(qbuf, "횟수: ");
            } else {
                /*KR Sprintf(qbuf, "Count: %ld", cnt); */
                Sprintf(qbuf, "횟수: %ld", cnt);
                backspaced = FALSE;
            }
            custompline(SUPPRESS_HISTORY, "%s", qbuf);
            mark_synch();
        }
    }

    if (historical) {
        /*KR Sprintf(qbuf, "Count: %ld ", *count); */
        Sprintf(qbuf, "횟수: %ld ", *count);
        (void) key2txt((uchar) key, eos(qbuf));
        putmsghistory(qbuf, FALSE);
    }

    return key;
}


STATIC_OVL char *
parse()
{
#ifdef LINT /* static char in_line[COLNO]; */
    char in_line[COLNO];
#else
    static char in_line[COLNO];
#endif
    register int foo;

    iflags.in_parse = TRUE;
    multi = 0;
    context.move = 1;
    flush_screen(1); /* Flush screen buffer. Put the cursor on the hero. */

#ifdef ALTMETA
    alt_esc = iflags.altmeta; /* readchar() hack */
#endif
    if (!Cmd.num_pad || (foo = readchar()) == Cmd.spkeys[NHKF_COUNT]) {
        long tmpmulti = multi;

        foo = get_count((char *) 0, '\0', LARGEST_INT, &tmpmulti, FALSE);
        last_multi = multi = tmpmulti;
    }
#ifdef ALTMETA
    alt_esc = FALSE; /* readchar() reset */
#endif

    if (iflags.debug_fuzzer /* if fuzzing, override '!' and ^Z */
        && (Cmd.commands[foo & 0x0ff]
            && (Cmd.commands[foo & 0x0ff]->ef_funct == dosuspend_core
                || Cmd.commands[foo & 0x0ff]->ef_funct == dosh_core)))
        foo = Cmd.spkeys[NHKF_ESC];

    if (foo == Cmd.spkeys[NHKF_ESC]) { /* esc cancels count (TH) */
        clear_nhwindow(WIN_MESSAGE);
        multi = last_multi = 0;
    } else if (foo == Cmd.spkeys[NHKF_DOAGAIN] || in_doagain) {
        multi = last_multi;
    } else {
        last_multi = multi;
        savech(0); /* reset input queue */
        savech((char) foo);
    }

    if (multi) {
        multi--;
        save_cm = in_line;
    } else {
        save_cm = (char *) 0;
    }
    /* in 3.4.3 this was in rhack(), where it was too late to handle M-5 */
    if (Cmd.pcHack_compat) {
        /* This handles very old inconsistent DOS/Windows behaviour
           in a different way: earlier, the keyboard handler mapped
           these, which caused counts to be strange when entered
           from the number pad. Now do not map them until here. */
        switch (foo) {
        case '5':
            foo = Cmd.spkeys[NHKF_RUSH];
            break;
        case M('5'):
            foo = Cmd.spkeys[NHKF_RUN];
            break;
        case M('0'):
            foo = Cmd.spkeys[NHKF_DOINV];
            break;
        default:
            break; /* as is */
        }
    }

    in_line[0] = foo;
    in_line[1] = '\0';
    if (prefix_cmd(foo)) {
        foo = readchar();
        savech((char) foo);
        in_line[1] = foo;
        in_line[2] = 0;
    }
    clear_nhwindow(WIN_MESSAGE);

    iflags.in_parse = FALSE;
    return in_line;
}

#ifdef HANGUPHANDLING
/* some very old systems, or descendents of such systems, expect signal
   handlers to have return type `int', but they don't actually inspect
   the return value so we should be safe using `void' unconditionally */
/*ARGUSED*/
void
hangup(sig_unused) /* called as signal() handler, so sent at least one arg */
int sig_unused UNUSED;
{
    if (program_state.exiting)
        program_state.in_moveloop = 0;
    nhwindows_hangup();
#ifdef SAFERHANGUP
    /* When using SAFERHANGUP, the done_hup flag it tested in rhack
       and a couple of other places; actual hangup handling occurs then.
       This is 'safer' because it disallows certain cheats and also
       protects against losing objects in the process of being thrown,
       but also potentially riskier because the disconnected program
       must continue running longer before attempting a hangup save. */
    program_state.done_hup++;
    /* defer hangup iff game appears to be in progress */
    if (program_state.in_moveloop && program_state.something_worth_saving)
        return;
#endif /* SAFERHANGUP */
    end_of_input();
}

void
end_of_input()
{
#ifdef NOSAVEONHANGUP
#ifdef INSURANCE
    if (flags.ins_chkpt && program_state.something_worth_saving)
        program_state.preserve_locks = 1; /* keep files for recovery */
#endif
    program_state.something_worth_saving = 0; /* don't save */
#endif

#ifndef SAFERHANGUP
    if (!program_state.done_hup++)
#endif
        if (program_state.something_worth_saving)
            (void) dosave0();
    if (iflags.window_inited)
        exit_nhwindows((char *) 0);
    clearlocks();
    nh_terminate(EXIT_SUCCESS);
    /*NOTREACHED*/ /* not necessarily true for vms... */
    return;
}
#endif /* HANGUPHANDLING */

char
readchar()
{
    register int sym;
    int x = u.ux, y = u.uy, mod = 0;

    if (iflags.debug_fuzzer)
        return randomkey();
    if (*readchar_queue)
        sym = *readchar_queue++;
    else
        sym = in_doagain ? pgetchar() : nh_poskey(&x, &y, &mod);

#ifdef NR_OF_EOFS
    if (sym == EOF) {
        register int cnt = NR_OF_EOFS;
        /*
         * Some SYSV systems seem to return EOFs for various reasons
         * (?like when one hits break or for interrupted systemcalls?),
         * and we must see several before we quit.
         */
        do {
            clearerr(stdin); /* omit if clearerr is undefined */
            sym = pgetchar();
        } while (--cnt && sym == EOF);
    }
#endif /* NR_OF_EOFS */

    if (sym == EOF) {
#ifdef HANGUPHANDLING
        hangup(0); /* call end_of_input() or set program_state.done_hup */
#endif
        sym = '\033';
#ifdef ALTMETA
    } else if (sym == '\033' && alt_esc) {
        /* iflags.altmeta: treat two character ``ESC c'' as single `M-c' */
        sym = *readchar_queue ? *readchar_queue++ : pgetchar();
        if (sym == EOF || sym == 0)
            sym = '\033';
        else if (sym != '\033')
            sym |= 0200; /* force 8th bit on */
#endif /*ALTMETA*/
    } else if (sym == 0) {
        /* click event */
        readchar_queue = click_to_cmd(x, y, mod);
        sym = *readchar_queue++;
    }
    return (char) sym;
}

/* '_' command, #travel, via keyboard rather than mouse click */
STATIC_PTR int
dotravel(VOID_ARGS)
{
    static char cmd[2];
    coord cc;

    /* [FIXME?  Supporting the ability to disable traveling via mouse
       click makes some sense, depending upon overall mouse usage.
       Disabling '_' on a user by user basis makes no sense at all since
       even if it is typed by accident, aborting when picking a target
       destination is trivial.  Travel via mouse predates travel via '_',
       and this use of OPTION=!travel is probably just a mistake....] */
    if (!flags.travelcmd)
        return 0;

    cmd[1] = 0;
    cc.x = iflags.travelcc.x;
    cc.y = iflags.travelcc.y;
    if (cc.x == 0 && cc.y == 0) {
        /* No cached destination, start attempt from current position */
        cc.x = u.ux;
        cc.y = u.uy;
    }
    iflags.getloc_travelmode = TRUE;
    if (iflags.menu_requested) {
        int gf = iflags.getloc_filter;
        iflags.getloc_filter = GFILTER_VIEW;
        if (!getpos_menu(&cc, GLOC_INTERESTING)) {
            iflags.getloc_filter = gf;
            iflags.getloc_travelmode = FALSE;
            return 0;
        }
        iflags.getloc_filter = gf;
    } else {
        /*KR pline("Where do you want to travel to?"); */
        pline("어디로 이동하시겠습니까?");
        /*KR if (getpos(&cc, TRUE, "the desired destination") < 0) { */
        if (getpos(&cc, TRUE, "목적지") < 0) {
            /* user pressed ESC */
            iflags.getloc_travelmode = FALSE;
            return 0;
        }
    }
    iflags.getloc_travelmode = FALSE;
    iflags.travelcc.x = u.tx = cc.x;
    iflags.travelcc.y = u.ty = cc.y;
    cmd[0] = Cmd.spkeys[NHKF_TRAVEL];
    readchar_queue = cmd;
    return 0;
}

/*
 *   Parameter validator for generic yes/no function to prevent
 *   the core from sending too long a prompt string to the
 *   window port causing a buffer overflow there.
 */
char
yn_function(query, resp, def)
const char *query, *resp;
char def;
{
    char res, qbuf[QBUFSZ];
#ifdef DUMPLOG
    extern unsigned saved_pline_index; /* pline.c */
    unsigned idx = saved_pline_index;
    /* buffer to hold query+space+formatted_single_char_response */
    char dumplog_buf[QBUFSZ + 1 + 15]; /* [QBUFSZ+1+7] should suffice */
#endif

    iflags.last_msg = PLNMSG_UNKNOWN; /* most recent pline is clobbered */

    /* maximum acceptable length is QBUFSZ-1 */
    if (strlen(query) >= QBUFSZ) {
        /* caller shouldn't have passed anything this long */
        paniclog("Query truncated: ", query);
        (void) strncpy(qbuf, query, QBUFSZ - 1 - 3);
        Strcpy(&qbuf[QBUFSZ - 1 - 3], "...");
        query = qbuf;
    }
    res = (*windowprocs.win_yn_function)(query, resp, def);
#ifdef DUMPLOG
    if (idx == saved_pline_index) {
        /* when idx is still the same as saved_pline_index, the interface
           didn't put the prompt into saved_plines[]; we put a simplified
           version in there now (without response choices or default) */
        Sprintf(dumplog_buf, "%s ", query);
        (void) key2txt((uchar) res, eos(dumplog_buf));
        dumplogmsg(dumplog_buf);
    }
#endif
    return res;
}

/* for paranoid_confirm:quit,die,attack prompting */
boolean
paranoid_query(be_paranoid, prompt)
boolean be_paranoid;
const char *prompt;
{
    boolean confirmed_ok;

    /* when paranoid, player must respond with "yes" rather than just 'y'
       to give the go-ahead for this query; default is "no" unless the
       ParanoidConfirm flag is set in which case there's no default */
    if (be_paranoid) {
        char pbuf[BUFSZ], qbuf[QBUFSZ], ans[BUFSZ];
        const char *promptprefix = "",
           /*KR *responsetype = ParanoidConfirm ? "(yes|no)" : "(yes) [no]"; */
                       *responsetype =
                           ParanoidConfirm ? "(yes|no로 입력)" : "(yes) [no]";
        int k, trylimit = 6; /* 1 normal, 5 more with "Yes or No:" prefix */

        copynchars(pbuf, prompt, BUFSZ - 1);
        /* in addition to being paranoid about this particular
           query, we might be even more paranoid about all paranoia
           responses (ie, ParanoidConfirm is set) in which case we
           require "no" to reject in addition to "yes" to confirm
           (except we won't loop if response is ESC; it means no) */
        do {
            /* make sure we won't overflow a QBUFSZ sized buffer */
            k = (int) (strlen(promptprefix) + 1 + strlen(responsetype));
            if ((int) strlen(pbuf) + k > QBUFSZ - 1) {
                /* chop off some at the end */
                Strcpy(pbuf + (QBUFSZ - 1) - k - 4, "...?"); /* -4: "...?" */
            }

            Sprintf(qbuf, "%s%s %s", promptprefix, pbuf, responsetype);
            *ans = '\0';
            getlin(qbuf, ans);
            (void) mungspaces(ans);
            confirmed_ok = !strcmpi(ans, "yes");
            if (confirmed_ok || *ans == '\033')
                break;
            /*KR promptprefix = "\"Yes\" or \"No\": "; */
            promptprefix = "\"yes\" 또는 \"no\"로 입력하세요: ";
        } while (ParanoidConfirm && strcmpi(ans, "no") && --trylimit);
    } else
        confirmed_ok = (yn(prompt) == 'y');

    return confirmed_ok;
}

/* ^Z command, #suspend */
STATIC_PTR int
dosuspend_core(VOID_ARGS)
{
#ifdef SUSPEND
    /* Does current window system support suspend? */
    if ((*windowprocs.win_can_suspend)()) {
        /* NB: SYSCF SHELLERS handled in port code. */
        dosuspend();
    } else
#endif
        Norep(cmdnotavail, "#suspend");
    return 0;
}

/* '!' command, #shell */
STATIC_PTR int
dosh_core(VOID_ARGS)
{
#ifdef SHELL
    /* access restrictions, if any, are handled in port code */
    dosh();
#else
    Norep(cmdnotavail, "#shell");
#endif
    return 0;
}

/*cmd.c*/
