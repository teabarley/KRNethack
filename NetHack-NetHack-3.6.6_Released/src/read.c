/* NetHack 3.6	read.c	$NHDT-Date: 1561485713 2019/06/25 18:01:53 $  $NHDT-Branch: NetHack-3.6 $:$NHDT-Revision: 1.172 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2012. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

#define Your_Own_Role(mndx)  \
    ((mndx) == urole.malenum \
     || (urole.femalenum != NON_PM && (mndx) == urole.femalenum))
#define Your_Own_Race(mndx)  \
    ((mndx) == urace.malenum \
     || (urace.femalenum != NON_PM && (mndx) == urace.femalenum))

boolean known;

static NEARDATA const char readable[] = { ALL_CLASSES, SCROLL_CLASS,
                                          SPBOOK_CLASS, 0 };
static const char all_count[] = { ALLOW_COUNT, ALL_CLASSES, 0 };

STATIC_DCL boolean FDECL(learnscrolltyp, (SHORT_P));
STATIC_DCL char *FDECL(erode_obj_text, (struct obj *, char *) );
STATIC_DCL char *FDECL(apron_text, (struct obj *, char *buf));
STATIC_DCL void FDECL(stripspe, (struct obj *) );
STATIC_DCL void FDECL(p_glow1, (struct obj *) );
STATIC_DCL void FDECL(p_glow2, (struct obj *, const char *) );
STATIC_DCL void FDECL(forget_single_object, (int) );
#if 0 /* not used */
STATIC_DCL void FDECL(forget_objclass, (int));
#endif
STATIC_DCL void FDECL(randomize, (int *, int) );
STATIC_DCL void FDECL(forget, (int) );
STATIC_DCL int FDECL(maybe_tame, (struct monst *, struct obj *) );
STATIC_DCL boolean FDECL(get_valid_stinking_cloud_pos, (int, int) );
STATIC_DCL boolean FDECL(is_valid_stinking_cloud_pos, (int, int, BOOLEAN_P));
STATIC_PTR void FDECL(display_stinking_cloud_positions, (int) );
STATIC_PTR void FDECL(set_lit, (int, int, genericptr));
STATIC_DCL void NDECL(do_class_genocide);

STATIC_OVL boolean
learnscrolltyp(scrolltyp)
short scrolltyp;
{
    if (!objects[scrolltyp].oc_name_known) {
        makeknown(scrolltyp);
        more_experienced(0, 10);
        return TRUE;
    } else
        return FALSE;
}

/* also called from teleport.c for scroll of teleportation */
void learnscroll(sobj) struct obj *sobj;
{
    /* it's implied that sobj->dknown is set;
       we couldn't be reading this scroll otherwise */
    if (sobj->oclass != SPBOOK_CLASS)
        (void) learnscrolltyp(sobj->otyp);
}

STATIC_OVL char *
erode_obj_text(otmp, buf)
struct obj *otmp;
char *buf;
{
    int erosion = greatest_erosion(otmp);

    if (erosion)
        wipeout_text(buf, (int) (strlen(buf) * erosion / (2 * MAX_ERODE)),
                     otmp->o_id ^ (unsigned) ubirthday);
    return buf;
}

char *
tshirt_text(tshirt, buf)
struct obj *tshirt;
char *buf;
{
#if 0 /*KR: 원본*/
    static const char *shirt_msgs[] = {
        /* Scott Bigham */
      "I explored the Dungeons of Doom and all I got was this lousy T-shirt!",
        "Is that Mjollnir in your pocket or are you just happy to see me?",
      "It's not the size of your sword, it's how #enhance'd you are with it.",
        "Madame Elvira's House O' Succubi Lifetime Customer",
        "Madame Elvira's House O' Succubi Employee of the Month",
        "Ludios Vault Guards Do It In Small, Dark Rooms",
        "Yendor Military Soldiers Do It In Large Groups",
        "I survived Yendor Military Boot Camp",
        "Ludios Accounting School Intra-Mural Lacrosse Team",
        "Oracle(TM) Fountains 10th Annual Wet T-Shirt Contest",
        "Hey, black dragon!  Disintegrate THIS!",
        "I'm With Stupid -->",
        "Don't blame me, I voted for Izchak!",
        "Don't Panic", /* HHGTTG */
        "Furinkan High School Athletic Dept.",                /* Ranma 1/2 */
        "Hel-LOOO, Nurse!",                                   /* Animaniacs */
        "=^.^=",
        "100% goblin hair - do not wash",
        "Aberzombie and Fitch",
        "cK -- Cockatrice touches the Kop",
        "Don't ask me, I only adventure here",
        "Down with pants!",
        "d, your dog or a killer?",
        "FREE PUG AND NEWT!",
        "Go team ant!",
        "Got newt?",
        "Hello, my darlings!", /* Charlie Drake */
        "Hey!  Nymphs!  Steal This T-Shirt!",
        "I <3 Dungeon of Doom",
        "I <3 Maud",
        "I am a Valkyrie.  If you see me running, try to keep up.",
        "I am not a pack rat - I am a collector",
        "I bounced off a rubber tree",         /* Monkey Island */
        "Plunder Island Brimstone Beach Club", /* Monkey Island */
        "If you can read this, I can hit you with my polearm",
        "I'm confused!",
        "I scored with the princess",
        "I want to live forever or die in the attempt.",
        "Lichen Park",
        "LOST IN THOUGHT - please send search party",
        "Meat is Mordor",
        "Minetown Better Business Bureau",
        "Minetown Watch",
 "Ms. Palm's House of Negotiable Affection -- A Very Reputable House Of Disrepute",
        "Protection Racketeer",
        "Real men love Crom",
        "Somebody stole my Mojo!",
        "The Hellhound Gang",
        "The Werewolves",
        "They Might Be Storm Giants",
        "Weapons don't kill people, I kill people",
        "White Zombie",
        "You're killing me!",
        "Anhur State University - Home of the Fighting Fire Ants!",
        "FREE HUGS",
        "Serial Ascender",
        "Real men are valkyries",
        "Young Men's Cavedigging Association",
        "Occupy Fort Ludios",
        "I couldn't afford this T-shirt so I stole it!",
        "Mind flayers suck",
        "I'm not wearing any pants",
        "Down with the living!",
        "Pudding farmer",
        "Vegetarian",
        "Hello, I'm War!",
        "It is better to light a candle than to curse the darkness",
        "It is easier to curse the darkness than to light a candle",
        /* expanded "rock--paper--scissors" featured in TV show "Big Bang
           Theory" although they didn't create it (and an actual T-shirt
           with pentagonal diagram showing which choices defeat which) */
        "rock--paper--scissors--lizard--Spock!",
        /* "All men must die -- all men must serve" challange and response
           from book series _A_Song_of_Ice_and_Fire_ by George R.R. Martin,
           TV show "Game of Thrones" (probably an actual T-shirt too...) */
        "/Valar morghulis/ -- /Valar dohaeris/",
    };
#else /*KR: KRNethack 맞춤 번역 */
    static const char *shirt_msgs[] = {
        /* Scott Bigham */
        "파멸의 미궁을 탐험하고 얻은 거라곤 이 형편없는 티셔츠뿐!",
        "주머니에 든 거 묠니르인가요, 아니면 날 봐서 기쁜 건가요?",
        "중요한 건 검의 크기가 아니라, 검과 얼마나 #enhance(동기화) "
        "되었는가입니다.",
        "마담 엘비라의 서큐버스관 평생 고객",
        "마담 엘비라의 서큐버스관 이달의 우수 직원",
        "루디오스 금고 경비병들은 작고 어두운 방에서 '그것'을 합니다",
        "옌더 군대 병사들은 떼거지로 '그것'을 합니다",
        "나는 옌더 군대 신병 훈련소에서 살아남았다",
        "루디오스 회계학교 교내 라크로스 팀",
        "오라클(TM) 분수 제10회 연례 젖은 티셔츠 경연대회",
        "이봐, 블랙 드래곤! '이거나' 분해시켜 보시지!",
        "저는 멍청이랑 일행입니다 -->",
        "날 탓하지 마, 난 이자크에게 투표했다고!",
        "당황하지 마시오",           /* HHGTTG */
        "풍림관 고등학교 체육부",    /* Ranma 1/2 */
        "안녕하세-용, 간호사 누나!", /* Animaniacs */
        "=^.^=",
        "고블린 털 100% - 세탁하지 마시오",
        "아버좀비 & 피치",
        "cK -- 코카트리스가 캅을 만지다",
        "내게 묻지 마시오, 난 여기서 모험만 할 뿐",
        "바지 타도!",
        "d, 당신의 개입니까, 아니면 살인마입니까?",
        "퍼그와 뉴트에게 자유를!",
        "가자, 개미 팀!",
        "뉴트 있수?",
        "안녕, 내 사랑!", /* Charlie Drake */
        "이봐! 님프들! 이 티셔츠를 훔쳐가!",
        "나는 파멸의 미궁을 <3 (사랑)해",
        "나는 모드를 <3 (사랑)해",
        "나는 발키리다. 내가 뛰는 걸 본다면, 뒤처지지 않게 따라와라.",
        "나는 짐꾼 쥐가 아니다 - 수집가다",
        "나는 고무나무에서 튕겨 나갔다", /* Monkey Island */
        "약탈의 섬 유황 해변 클럽",      /* Monkey Island */
        "이 글씨가 보인다면, 내 미늘창이 닿는 거리라는 뜻이다",
        "난 혼란스럽다!",
        "공주랑 진도 좀 뺐지",
        "영원히 살거나, 그러다 죽거나.",
        "라이켄 파크",
        "생각에 잠김 - 수색대를 보내주시오",
        "고기는 모르도르다",
        "광산 마을 우수 기업 협회",
        "광산 마을 경비대",
        "팜 부인의 거래 가능한 애정의 집 -- 매우 평판이 좋은 매음굴",
        "보호비 갈취단",
        "진정한 남자는 크롬을 사랑한다",
        "누군가 내 모조를 훔쳐갔어!",
        "헬하운드 갱",
        "웨어울프스",
        "그들은 스톰 자이언트일지도 모른다",
        "무기가 사람을 죽이는 게 아니다, 내가 사람을 죽이는 거다",
        "화이트 좀비",
        "당신이 날 죽이고 있어요!",
        "안후르 주립 대학교 - 파이팅 파이어 앤츠의 고향!",
        "프리 허그",
        "연쇄 승천마",
        "진정한 남자는 발키리를 한다",
        "YMCA (청년 동굴탐험가 협회)",
        "루디오스 요새를 점령하라",
        "이 티셔츠를 살 돈이 없어서 그냥 훔쳤다!",
        "마인드 플레이어는 밥맛이다",
        "난 지금 바지를 입고 있지 않다",
        "산 자들을 타도하라!",
        "푸딩 농부",
        "채식주의자",
        "안녕, 난 '전쟁'이야!",
        "어둠을 저주하느니 촛불 하나를 켜는 것이 낫다",
        "촛불 하나를 켜는 것보다 어둠을 저주하는 게 더 쉽다",
        /* expanded "rock--paper--scissors" featured in TV show "Big Bang
           Theory" although they didn't create it (and an actual T-shirt
           with pentagonal diagram showing which choices defeat which) */
        "가위--바위--보--도마뱀--스팍!",
        /* "All men must die -- all men must serve" challange and response
           from book series _A_Song_of_Ice_and_Fire_ by George R.R. Martin,
           TV show "Game of Thrones" (probably an actual T-shirt too...) */
        "/발라 모굴리스/ -- /발라 도하에리스/",
    };
#endif

    Strcpy(buf, shirt_msgs[tshirt->o_id % SIZE(shirt_msgs)]);
    return erode_obj_text(tshirt, buf);
}

STATIC_OVL char *
apron_text(apron, buf)
struct obj *apron;
char *buf;
{
#if 0 /*KR: 원본*/
    static const char *apron_msgs[] = {
        "Kiss the cook",
        "I'm making SCIENCE!",
        "Don't mess with the chef",
        "Don't make me poison you",
        "Gehennom's Kitchen",
        "Rat: The other white meat",
        "If you can't stand the heat, get out of Gehennom!",
        "If we weren't meant to eat animals, why are they made out of meat?",
        "If you don't like the food, I'll stab you",
    };
#else /*KR: KRNethack 맞춤 번역 */
    static const char *apron_msgs[] = {
        "요리사에게 키스를",
        "나는 지금 '과학'을 하고 있다구!",
        "주방장한테 까불지 마라",
        "음식에 독 타게 만들지 마",
        "게헨놈의 주방",
        "쥐고기: 또 다른 백색육",
        "열기를 견딜 수 없다면, 게헨놈에서 나가라!",
        "동물을 먹지 말아야 한다면, 왜 동물들은 고기로 만들어져 있는 거지?",
        "음식이 마음에 안 들면, 널 찔러버릴 테다",
    };
#endif

    Strcpy(buf, apron_msgs[apron->o_id % SIZE(apron_msgs)]);
    return erode_obj_text(apron, buf);
}

int
doread()
{
    register struct obj *scroll;
    boolean confused, nodisappear;

    known = FALSE;
    if (check_capacity((char *) 0))
        return 0;
    scroll = getobj(readable, "read");
    if (!scroll)
        return 0;

    /* outrumor has its own blindness check */
    if (scroll->otyp == FORTUNE_COOKIE) {
        if (flags.verbose)
            /*KR You("break up the cookie and throw away the pieces."); */
            You("쿠키를 부수고 조각들을 버렸다.");
        outrumor(bcsign(scroll), BY_COOKIE);
        if (!Blind)
            u.uconduct.literate++;
        useup(scroll);
        return 1;
    } else if (scroll->otyp == T_SHIRT || scroll->otyp == ALCHEMY_SMOCK) {
        char buf[BUFSZ], *mesg;
        const char *endpunct;

        if (Blind) {
            /*KR You_cant("feel any Braille writing."); */
            You_cant("점자 같은 것은 만져지지 않는다.");
            return 0;
        }
        /* can't read shirt worn under suit (under cloak is ok though) */
        if (scroll->otyp == T_SHIRT && uarm && scroll == uarmu) {
#if 0 /*KR: 원본*/
            pline("%s shirt is obscured by %s%s.",
                  scroll->unpaid ? "That" : "Your", shk_your(buf, uarm),
                  suit_simple_name(uarm));
#else /*KR: KRNethack 맞춤 번역 */
            pline("%s 셔츠는 %s에 가려져 있다.",
                  scroll->unpaid ? "그" : "당신의",
                  append_josa(suit_simple_name(uarm), "에"));
#endif
            return 0;
        }
        u.uconduct.literate++;
        /* populate 'buf[]' */
        mesg = (scroll->otyp == T_SHIRT) ? tshirt_text(scroll, buf)
                                         : apron_text(scroll, buf);
        endpunct = "";
        if (flags.verbose) {
            int ln = (int) strlen(mesg);

            /* we will be displaying a sentence; need ending punctuation */
            if (ln > 0 && !index(".!?", mesg[ln - 1]))
                endpunct = ".";
            /*KR pline("It reads:"); */
            pline("이렇게 쓰여 있다:");
        }
        pline("\"%s\"%s", mesg, endpunct);
        return 1;
    } else if (scroll->otyp == CREDIT_CARD) {
#if 0 /*KR: 원본*/
        static const char *card_msgs[] = {
            "Leprechaun Gold Tru$t - Shamrock Card",
            "Magic Memory Vault Charge Card",
            "Larn National Bank",                /* Larn */
            "First Bank of Omega",               /* Omega */
            "Bank of Zork - Frobozz Magic Card", /* Zork */
            "Ankh-Morpork Merchant's Guild Barter Card",
            "Ankh-Morpork Thieves' Guild Unlimited Transaction Card",
            "Ransmannsby Moneylenders Association",
            "Bank of Gehennom - 99% Interest Card",
            "Yendorian Express - Copper Card",
            "Yendorian Express - Silver Card",
            "Yendorian Express - Gold Card",
            "Yendorian Express - Mithril Card",
            "Yendorian Express - Platinum Card", /* must be last */
        };
#else /*KR: KRNethack 맞춤 번역 */
        static const char *card_msgs[] = {
            "레프리콘 황금 신탁 - 샴록 카드",
            "마법 기억 금고 차지 카드",
            "란 국립 은행",                   /* Larn */
            "오메가 제일 은행",               /* Omega */
            "조크 은행 - 프로보즈 마법 카드", /* Zork */
            "앙크모포크 상인 길드 물물교환 카드",
            "앙크모포크 도둑 길드 무제한 거래 카드",
            "란스만스비 대금업자 협회",
            "게헨놈 은행 - 99% 이자 카드",
            "옌더리안 익스프레스 - 구리 카드",
            "옌더리안 익스프레스 - 은 카드",
            "옌더리안 익스프레스 - 금 카드",
            "옌더리안 익스프레스 - 미스릴 카드",
            "옌더리안 익스프레스 - 백금 카드", /* must be last */
        };
#endif

        if (Blind) {
            /*KR You("feel the embossed numbers:"); */
            You("양각된 숫자들을 만져보았다:");
        } else {
            if (flags.verbose)
                /*KR pline("It reads:"); */
                pline("이렇게 쓰여 있다:");
            pline("\"%s\"",
                  scroll->oartifact
                      ? card_msgs[SIZE(card_msgs) - 1]
                      : card_msgs[scroll->o_id % (SIZE(card_msgs) - 1)]);
        }
        /* Make a credit card number */
        pline("\"%d0%d %ld%d1 0%d%d0\"%s", (((int) scroll->o_id % 89) + 10),
              ((int) scroll->o_id % 4),
              ((((long) scroll->o_id * 499L) % 899999L) + 100000L),
              ((int) scroll->o_id % 10), (!((int) scroll->o_id % 3)),
              (((int) scroll->o_id * 7) % 10),
              (flags.verbose || Blind) ? "." : "");
        u.uconduct.literate++;
        return 1;
    } else if (scroll->otyp == CAN_OF_GREASE) {
        /*KR pline("This %s has no label.", singular(scroll, xname)); */
        pline("이 %s에는 라벨이 없다.", singular(scroll, xname));
        return 0;
    } else if (scroll->otyp == MAGIC_MARKER) {
        if (Blind) {
            /*KR You_cant("feel any Braille writing."); */
            You_cant("점자 같은 것은 만져지지 않는다.");
            return 0;
        }
        if (flags.verbose)
            /*KR pline("It reads:"); */
            pline("이렇게 쓰여 있다:");
        /*KR pline("\"Magic Marker(TM) Red Ink Marker Pen.  Water
         * Soluble.\""); */
        pline("\"매직 마커(TM) 붉은색 잉크 마커 펜. 수용성.\"");
        u.uconduct.literate++;
        return 1;
    } else if (scroll->oclass == COIN_CLASS) {
        if (Blind)
            /*KR You("feel the embossed words:"); */
            You("양각된 글자들을 만져보았다:");
        else if (flags.verbose)
            /*KR You("read:"); */
            You("읽었다:");
        /*KR pline("\"1 Zorkmid.  857 GUE.  In Frobs We Trust.\""); */
        pline("\"1 조크미드. 857 GUE. 우리는 프로브를 믿는다.\"");
        u.uconduct.literate++;
        return 1;
    } else if (scroll->oartifact == ART_ORB_OF_FATE) {
        if (Blind)
            /*KR You("feel the engraved signature:"); */
            You("새겨진 서명을 만져보았다:");
        else
            /*KR pline("It is signed:"); */
            pline("이렇게 서명되어 있다:");
        /*KR pline("\"Odin.\""); */
        pline("\"오딘.\"");
        u.uconduct.literate++;
        return 1;
    } else if (scroll->otyp == CANDY_BAR) {
#if 0 /*KR: 원본*/
        static const char *wrapper_msgs[] = {
            "Apollo",       /* Lost */
            "Moon Crunchy", /* South Park */
            "Snacky Cake",    "Chocolate Nuggie", "The Small Bar",
            "Crispy Yum Yum", "Nilla Crunchie",   "Berry Bar",
            "Choco Nummer",   "Om-nom", /* Cat Macro */
            "Fruity Oaty",              /* Serenity */
            "Wonka Bar" /* Charlie and the Chocolate Factory */
        };
#else /*KR: KRNethack 맞춤 번역 */
        static const char *wrapper_msgs[] = {
            "아폴로",    /* Lost */
            "문 크런치", /* South Park */
            "스내키 케이크", "초콜릿 너기", "스몰 바",
            "크리스피 얌얌", "닐라 크런치", "베리 바",
            "초코 넘머",     "옴-뇸", /* Cat Macro */
            "프루티 오티",            /* Serenity */
            "웡카 바"                 /* Charlie and the Chocolate Factory */
        };
#endif

        if (Blind) {
            /*KR You_cant("feel any Braille writing."); */
            You_cant("점자 같은 것은 만져지지 않는다.");
            return 0;
        }
        /*KR pline("The wrapper reads: \"%s\".",
         * wrapper_msgs[scroll->o_id % SIZE(wrapper_msgs)]); */
        pline("포장지에 쓰여 있다: \"%s\".",
              wrapper_msgs[scroll->o_id % SIZE(wrapper_msgs)]);
        u.uconduct.literate++;
        return 1;
    } else if (scroll->oclass != SCROLL_CLASS
               && scroll->oclass != SPBOOK_CLASS) {
        /*KR pline(silly_thing_to, "read"); */
        pline(silly_thing_to, "읽기에는");
        return 0;
    } else if (Blind && (scroll->otyp != SPE_BOOK_OF_THE_DEAD)) {
        const char *what = 0;

        if (scroll->oclass == SPBOOK_CLASS)
            what = "mystic runes";
        else if (!scroll->dknown)
            what = "formula on the scroll";
        if (what) {
            /*KR pline("Being blind, you cannot read the %s.", what); */
            pline("당신은 눈이 멀어서 %s 읽을 수 없다.",
                  append_josa(what, "을"));
            return 0;
        }
    }

    confused = (Confusion != 0);
#ifdef MAIL
    if (scroll->otyp == SCR_MAIL) {
        confused = FALSE; /* override */
        /* reading mail is a convenience for the player and takes
           place outside the game, so shouldn't affect gameplay;
           on the other hand, it starts by explicitly making the
           hero actively read something, which is pretty hard
           to simply ignore; as a compromise, if the player has
           maintained illiterate conduct so far, and this mail
           scroll didn't come from bones, ask for confirmation */
        if (!u.uconduct.literate) {
            /*KR if (!scroll->spe && yn(
             "Reading mail will violate \"illiterate\" conduct.  Read anyway?"
                                   ) != 'y') */
            if (!scroll->spe
                && yn("편지를 읽으면 \"문맹\" 서약을 깨게 됩니다. 그래도 "
                      "읽으시겠습니까?")
                       != 'y')
                return 0;
        }
    }
#endif

    /* Actions required to win the game aren't counted towards conduct */
    /* Novel conduct is handled in read_tribute so exclude it too*/
    if (scroll->otyp != SPE_BOOK_OF_THE_DEAD
        && scroll->otyp != SPE_BLANK_PAPER && scroll->otyp != SCR_BLANK_PAPER
        && scroll->otyp != SPE_NOVEL)
        u.uconduct.literate++;

    if (scroll->oclass == SPBOOK_CLASS) {
        return study_book(scroll);
    }
    scroll->in_use = TRUE; /* scroll, not spellbook, now being read */
    if (scroll->otyp != SCR_BLANK_PAPER) {
        boolean silently = !can_chant(&youmonst);

        /* a few scroll feedback messages describe something happening
           to the scroll itself, so avoid "it disappears" for those */
        nodisappear =
            (scroll->otyp == SCR_FIRE
             || (scroll->otyp == SCR_REMOVE_CURSE && scroll->cursed));
#if 0 /*KR: 원본*/
        if (Blind)
            pline(nodisappear
                      ? "You %s the formula on the scroll."
                      : "As you %s the formula on it, the scroll disappears.",
                  silently ? "cogitate" : "pronounce");
        else
            pline(nodisappear ? "You read the scroll."
                              : "As you read the scroll, it disappears.");
        if (confused) {
            if (Hallucination)
                pline("Being so trippy, you screw up...");
            else
                pline("Being confused, you %s the magic words...",
                      silently ? "misunderstand" : "mispronounce");
        }
#else /*KR: KRNethack 맞춤 번역 */
        if (Blind)
            pline(nodisappear ? "당신은 두루마리에 적힌 주문을 %s."
                              : "당신이 주문을 %s, 두루마리가 사라졌다.",
                  silently ? "생각했다" : "발음하자");
        else
            pline(nodisappear ? "당신은 두루마리를 읽었다."
                              : "당신이 두루마리를 읽자, 그것이 사라졌다.");
        if (confused) {
            if (Hallucination)
                pline("너무 몽롱해서, 실수하고 말았다...");
            else
                pline("혼란스러워서, 당신은 마법의 단어를 %s...",
                      silently ? "오해했다" : "잘못 발음했다");
        }
#endif
    }
    if (!seffects(scroll)) {
        if (!objects[scroll->otyp].oc_name_known) {
            if (known)
                learnscroll(scroll);
            else if (!objects[scroll->otyp].oc_uname)
                docall(scroll);
        }
        scroll->in_use = FALSE;
        if (scroll->otyp != SCR_BLANK_PAPER)
            useup(scroll);
    }
    return 1;
}

STATIC_OVL void stripspe(obj) register struct obj *obj;
{
    if (obj->blessed || obj->spe <= 0) {
        pline1(nothing_happens);
    } else {
        /* order matters: message, shop handling, actual transformation */
#if 0 /*KR: 원본*/
        pline("%s briefly.", Yobjnam2(obj, "vibrate"));
#else /*KR: KRNethack 맞춤 번역 */
        pline("%s 짧게 진동했다.", append_josa(Yname2(obj), "이"));
#endif
        costly_alteration(obj, COST_UNCHRG);
        obj->spe = 0;
        if (obj->otyp == OIL_LAMP || obj->otyp == BRASS_LANTERN)
            obj->age = 0;
    }
}

STATIC_OVL void p_glow1(otmp) register struct obj *otmp;
{
#if 0 /*KR: 원본*/
    pline("%s briefly.", Yobjnam2(otmp, Blind ? "vibrate" : "glow"));
#else /*KR: KRNethack 맞춤 번역 */
    pline("%s 짧게 %s.", append_josa(Yname2(otmp), "이"),
          Blind ? "진동했다" : "빛났다");
#endif
}

STATIC_OVL void p_glow2(otmp, color) register struct obj *otmp;
register const char *color;
{
#if 0 /*KR: 원본*/
    pline("%s%s%s for a moment.", Yobjnam2(otmp, Blind ? "vibrate" : "glow"),
          Blind ? "" : " ", Blind ? "" : hcolor(color));
#else /*KR: KRNethack 맞춤 번역 */
    pline("%s 잠시 동안 %s%s.", append_josa(Yname2(otmp), "이"),
          Blind ? "" : hcolor(color), Blind ? "진동했다" : " 빛났다");
#endif
}

/* Is the object chargeable?  For purposes of inventory display; it is
   possible to be able to charge things for which this returns FALSE. */
boolean
is_chargeable(obj)
struct obj *obj;
{
    if (obj->oclass == WAND_CLASS)
        return TRUE;
    /* known && !oc_name_known is possible after amnesia/mind flayer */
    if (obj->oclass == RING_CLASS)
        return (boolean) (objects[obj->otyp].oc_charged
                          && (obj->known
                              || (obj->dknown
                                  && objects[obj->otyp].oc_name_known)));
    if (is_weptool(obj)) /* specific check before general tools */
        return FALSE;
    if (obj->oclass == TOOL_CLASS)
        return (boolean) objects[obj->otyp].oc_charged;
    return FALSE; /* why are weapons/armor considered charged anyway? */
}

/* recharge an object; curse_bless is -1 if the recharging implement
   was cursed, +1 if blessed, 0 otherwise. */
void recharge(obj, curse_bless) struct obj *obj;
int curse_bless;
{
    register int n;
    boolean is_cursed, is_blessed;

    is_cursed = curse_bless < 0;
    is_blessed = curse_bless > 0;

    if (obj->oclass == WAND_CLASS) {
        int lim = (obj->otyp == WAN_WISHING)             ? 3
                  : (objects[obj->otyp].oc_dir != NODIR) ? 8
                                                         : 15;

        /* undo any prior cancellation, even when is_cursed */
        if (obj->spe == -1)
            obj->spe = 0;

        /*
         * Recharging might cause wands to explode.
         * v = number of previous recharges
         * v = percentage chance to explode on this attempt
         * v = cumulative odds for exploding
         * 0 :   0        0
         * 1 :   0.29     0.29
         * 2 :   2.33     2.62
         * 3 :   7.87    10.28
         * 4 :  18.66    27.02
         * 5 :  36.44    53.62
         * 6 :  62.97    82.83
         * 7 : 100      100
         */
        n = (int) obj->recharged;
        if (n > 0
            && (obj->otyp == WAN_WISHING
                || (n * n * n > rn2(7 * 7 * 7)))) { /* recharge_limit */
            wand_explode(obj, rnd(lim));
            return;
        }
        /* didn't explode, so increment the recharge count */
        obj->recharged = (unsigned) (n + 1);

        /* now handle the actual recharging */
        if (is_cursed) {
            stripspe(obj);
        } else {
            n = (lim == 3) ? 3 : rn1(5, lim + 1 - 5);
            if (!is_blessed)
                n = rnd(n);

            if (obj->spe < n)
                obj->spe = n;
            else
                obj->spe++;
            if (obj->otyp == WAN_WISHING && obj->spe > 3) {
                wand_explode(obj, 1);
                return;
            }
            if (obj->spe >= lim)
                p_glow2(obj, NH_BLUE);
            else
                p_glow1(obj);
#if 0 /*[shop price doesn't vary by charge count]*/
            /* update shop bill to reflect new higher price */
            if (obj->unpaid)
                alter_cost(obj, 0L);
#endif
        }

    } else if (obj->oclass == RING_CLASS && objects[obj->otyp].oc_charged) {
        /* charging does not affect ring's curse/bless status */
        int s = is_blessed ? rnd(3) : is_cursed ? -rnd(2) : 1;
        boolean is_on = (obj == uleft || obj == uright);

#if 0 /*KR: 원본*/
        /* destruction depends on current state, not adjustment */
        if (obj->spe > rn2(7) || obj->spe <= -5) {
            pline("%s momentarily, then %s!", Yobjnam2(obj, "pulsate"),
                  otense(obj, "explode"));
            if (is_on)
                Ring_gone(obj);
            s = rnd(3 * abs(obj->spe)); /* amount of damage */
            useup(obj);
            losehp(Maybe_Half_Phys(s), "exploding ring", KILLED_BY_AN);
        } else {
            long mask = is_on ? (obj == uleft ? LEFT_RING : RIGHT_RING) : 0L;

            pline("%s spins %sclockwise for a moment.", Yname2(obj),
                  s < 0 ? "counter" : "");
#else /*KR: KRNethack 맞춤 번역 */
        /* destruction depends on current state, not adjustment */
        if (obj->spe > rn2(7) || obj->spe <= -5) {
            pline("%s 순간적으로 맥동하더니, 이내 폭발했다!",
                  append_josa(Yname2(obj), "이"));
            if (is_on)
                Ring_gone(obj);
            s = rnd(3 * abs(obj->spe)); /* amount of damage */
            useup(obj);
            losehp(Maybe_Half_Phys(s), "폭발하는 반지", KILLED_BY_AN);
        } else {
            long mask = is_on ? (obj == uleft ? LEFT_RING : RIGHT_RING) : 0L;

            pline("%s 잠시 %s시계 방향으로 돌았다.",
                  append_josa(Yname2(obj), "이"), s < 0 ? "반" : "");
#endif
            if (s < 0)
                costly_alteration(obj, COST_DECHNT);
            /* cause attributes and/or properties to be updated */
            if (is_on)
                Ring_off(obj);
            obj->spe += s; /* update the ring while it's off */
            if (is_on)
                setworn(obj, mask), Ring_on(obj);
            /* oartifact: if a touch-sensitive artifact ring is
               ever created the above will need to be revised  */
            /* update shop bill to reflect new higher price */
            if (s > 0 && obj->unpaid)
                alter_cost(obj, 0L);
        }

    } else if (obj->oclass == TOOL_CLASS) {
        int rechrg = (int) obj->recharged;

        if (objects[obj->otyp].oc_charged) {
            /* tools don't have a limit, but the counter used does */
            if (rechrg < 7) /* recharge_limit */
                obj->recharged++;
        }
        switch (obj->otyp) {
        case BELL_OF_OPENING:
            if (is_cursed)
                stripspe(obj);
            else if (is_blessed)
                obj->spe += rnd(3);
            else
                obj->spe += 1;
            if (obj->spe > 5)
                obj->spe = 5;
            break;
        case MAGIC_MARKER:
        case TINNING_KIT:
        case EXPENSIVE_CAMERA:
            if (is_cursed)
                stripspe(obj);
            else if (rechrg
                     && obj->otyp
                            == MAGIC_MARKER) { /* previously recharged */
                obj->recharged = 1; /* override increment done above */
                if (obj->spe < 3)
                    /*KR Your("marker seems permanently dried out."); */
                    Your("마커가 영구적으로 말라버린 것 같다.");
                else
                    pline1(nothing_happens);
            } else if (is_blessed) {
                n = rn1(16, 15); /* 15..30 */
                if (obj->spe + n <= 50)
                    obj->spe = 50;
                else if (obj->spe + n <= 75)
                    obj->spe = 75;
                else {
                    int chrg = (int) obj->spe;
                    if ((chrg + n) > 127)
                        obj->spe = 127;
                    else
                        obj->spe += n;
                }
                p_glow2(obj, NH_BLUE);
            } else {
                n = rn1(11, 10); /* 10..20 */
                if (obj->spe + n <= 50)
                    obj->spe = 50;
                else {
                    int chrg = (int) obj->spe;
                    if ((chrg + n) > 127)
                        obj->spe = 127;
                    else
                        obj->spe += n;
                }
                p_glow2(obj, NH_WHITE);
            }
            break;
        case OIL_LAMP:
        case BRASS_LANTERN:
            if (is_cursed) {
                stripspe(obj);
                if (obj->lamplit) {
                    if (!Blind)
                        /*KR pline("%s out!", Tobjnam(obj, "go")); */
                        pline("%s 꺼졌다!", append_josa(Yname2(obj), "이"));
                    end_burn(obj, TRUE);
                }
            } else if (is_blessed) {
                obj->spe = 1;
                obj->age = 1500;
                p_glow2(obj, NH_BLUE);
            } else {
                obj->spe = 1;
                obj->age += 750;
                if (obj->age > 1500)
                    obj->age = 1500;
                p_glow1(obj);
            }
            break;
        case CRYSTAL_BALL:
            if (is_cursed) {
                stripspe(obj);
            } else if (is_blessed) {
                obj->spe = 6;
                p_glow2(obj, NH_BLUE);
            } else {
                if (obj->spe < 5) {
                    obj->spe++;
                    p_glow1(obj);
                } else
                    pline1(nothing_happens);
            }
            break;
        case HORN_OF_PLENTY:
        case BAG_OF_TRICKS:
        case CAN_OF_GREASE:
            if (is_cursed) {
                stripspe(obj);
            } else if (is_blessed) {
                if (obj->spe <= 10)
                    obj->spe += rn1(10, 6);
                else
                    obj->spe += rn1(5, 6);
                if (obj->spe > 50)
                    obj->spe = 50;
                p_glow2(obj, NH_BLUE);
            } else {
                obj->spe += rnd(5);
                if (obj->spe > 50)
                    obj->spe = 50;
                p_glow1(obj);
            }
            break;
        case MAGIC_FLUTE:
        case MAGIC_HARP:
        case FROST_HORN:
        case FIRE_HORN:
        case DRUM_OF_EARTHQUAKE:
            if (is_cursed) {
                stripspe(obj);
            } else if (is_blessed) {
                obj->spe += d(2, 4);
                if (obj->spe > 20)
                    obj->spe = 20;
                p_glow2(obj, NH_BLUE);
            } else {
                obj->spe += rnd(4);
                if (obj->spe > 20)
                    obj->spe = 20;
                p_glow1(obj);
            }
            break;
        default:
            goto not_chargable;
            /*NOTREACHED*/
            break;
        } /* switch */

    } else {
    not_chargable:
        /*KR You("have a feeling of loss."); */
        You("상실감이 느껴진다.");
    }
}

/* Forget known information about this object type. */
STATIC_OVL void forget_single_object(obj_id) int obj_id;
{
    objects[obj_id].oc_name_known = 0;
    objects[obj_id].oc_pre_discovered = 0; /* a discovery when relearned */
    if (objects[obj_id].oc_uname) {
        free((genericptr_t) objects[obj_id].oc_uname);
        objects[obj_id].oc_uname = 0;
    }
    undiscover_object(obj_id); /* after clearing oc_name_known */

    /* clear & free object names from matching inventory items too? */
}

#if 0 /* here if anyone wants it.... */
/* Forget everything known about a particular object class. */
STATIC_OVL void
forget_objclass(oclass)
int oclass;
{
    int i;

    for (i = bases[oclass];
         i < NUM_OBJECTS && objects[i].oc_class == oclass; i++)
        forget_single_object(i);
}
#endif

/* randomize the given list of numbers  0 <= i < count */
STATIC_OVL void randomize(indices, count) int *indices;
int count;
{
    int i, iswap, temp;

    for (i = count - 1; i > 0; i--) {
        if ((iswap = rn2(i + 1)) == i)
            continue;
        temp = indices[i];
        indices[i] = indices[iswap];
        indices[iswap] = temp;
    }
}

/* Forget % of known objects. */
void forget_objects(percent) int percent;
{
    int i, count;
    int indices[NUM_OBJECTS];

    if (percent == 0)
        return;
    if (percent <= 0 || percent > 100) {
        impossible("forget_objects: bad percent %d", percent);
        return;
    }

    indices[0] = 0; /* lint suppression */
    for (count = 0, i = 1; i < NUM_OBJECTS; i++)
        if (OBJ_DESCR(objects[i])
            && (objects[i].oc_name_known || objects[i].oc_uname))
            indices[count++] = i;

    if (count > 0) {
        randomize(indices, count);

        /* forget first % of randomized indices */
        count = ((count * percent) + rn2(100)) / 100;
        for (i = 0; i < count; i++)
            forget_single_object(indices[i]);
    }
}

/* Forget some or all of map (depends on parameters). */
void forget_map(howmuch) int howmuch;
{
    register int zx, zy;

    if (Sokoban)
        return;

    known = TRUE;
    for (zx = 0; zx < COLNO; zx++)
        for (zy = 0; zy < ROWNO; zy++)
            if (howmuch & ALL_MAP || rn2(7)) {
                /* Zonk all memory of this location. */
                levl[zx][zy].seenv = 0;
                levl[zx][zy].waslit = 0;
                levl[zx][zy].glyph = cmap_to_glyph(S_stone);
                lastseentyp[zx][zy] = STONE;
            }
    /* forget overview data for this level */
    forget_mapseen(ledger_no(&u.uz));
}

/* Forget all traps on the level. */
void
forget_traps()
{
    register struct trap *trap;

    /* forget all traps (except the one the hero is in :-) */
    for (trap = ftrap; trap; trap = trap->ntrap)
        if ((trap->tx != u.ux || trap->ty != u.uy) && (trap->ttyp != HOLE))
            trap->tseen = 0;
}

/*
 * Forget given % of all levels that the hero has visited and not forgotten,
 * except this one.
 */
void forget_levels(percent) int percent;
{
    int i, count;
    xchar maxl, this_lev;
    int indices[MAXLINFO];

    if (percent == 0)
        return;

    if (percent <= 0 || percent > 100) {
        impossible("forget_levels: bad percent %d", percent);
        return;
    }

    this_lev = ledger_no(&u.uz);
    maxl = maxledgerno();

    /* count & save indices of non-forgotten visited levels */
    /* Sokoban levels are pre-mapped for the player, and should stay
     * so, or they become nearly impossible to solve.  But try to
     * shift the forgetting elsewhere by fiddling with percent
     * instead of forgetting fewer levels.
     */
    indices[0] = 0; /* lint suppression */
    for (count = 0, i = 0; i <= maxl; i++)
        if ((level_info[i].flags & VISITED)
            && !(level_info[i].flags & FORGOTTEN) && i != this_lev) {
            if (ledger_to_dnum(i) == sokoban_dnum)
                percent += 2;
            else
                indices[count++] = i;
        }

    if (percent > 100)
        percent = 100;

    if (count > 0) {
        randomize(indices, count);

        /* forget first % of randomized indices */
        count = ((count * percent) + 50) / 100;
        for (i = 0; i < count; i++) {
            level_info[indices[i]].flags |= FORGOTTEN;
            forget_mapseen(indices[i]);
        }
    }
}

/*
 * Forget some things (e.g. after reading a scroll of amnesia).  When called,
 * the following are always forgotten:
 * - felt ball & chain
 * - traps
 * - part (6 out of 7) of the map
 *
 * Other things are subject to flags:
 * howmuch & ALL_MAP       = forget whole map
 * howmuch & ALL_SPELLS    = forget all spells
 */
STATIC_OVL void forget(howmuch) int howmuch;
{
    if (Punished)
        u.bc_felt = 0; /* forget felt ball&chain */

    forget_map(howmuch);
    forget_traps();

    /* 1 in 3 chance of forgetting some levels */
    if (!rn2(3))
        forget_levels(rn2(25));

    /* 1 in 3 chance of forgetting some objects */
    if (!rn2(3))
        forget_objects(rn2(25));

    if (howmuch & ALL_SPELLS)
        losespells();
    /*
     * Make sure that what was seen is restored correctly.  To do this,
     * we need to go blind for an instant --- turn off the display,
     * then restart it.  All this work is needed to correctly handle
     * walls which are stone on one side and wall on the other.  Turning
     * off the seen bits above will make the wall revert to stone,  but
     * there are cases where we don't want this to happen.  The easiest
     * thing to do is to run it through the vision system again, which
     * is always correct.
     */
    docrt(); /* this correctly will reset vision */
}

/* monster is hit by scroll of taming's effect */
STATIC_OVL int
maybe_tame(mtmp, sobj)
struct monst *mtmp;
struct obj *sobj;
{
    int was_tame = mtmp->mtame;
    unsigned was_peaceful = mtmp->mpeaceful;

    if (sobj->cursed) {
        setmangry(mtmp, FALSE);
        if (was_peaceful && !mtmp->mpeaceful)
            return -1;
    } else {
        if (mtmp->isshk)
            make_happy_shk(mtmp, FALSE);
        else if (!resist(mtmp, sobj->oclass, 0, NOTELL))
            (void) tamedog(mtmp, (struct obj *) 0);
        if ((!was_peaceful && mtmp->mpeaceful) || (!was_tame && mtmp->mtame))
            return 1;
    }
    return 0;
}

STATIC_OVL boolean
get_valid_stinking_cloud_pos(x, y)
int x, y;
{
    return (!(!isok(x, y) || !cansee(x, y) || !ACCESSIBLE(levl[x][y].typ)
              || distu(x, y) >= 32));
}

STATIC_OVL boolean
is_valid_stinking_cloud_pos(x, y, showmsg)
int x, y;
boolean showmsg;
{
    if (!get_valid_stinking_cloud_pos(x, y)) {
        if (showmsg)
            /*KR You("smell rotten eggs."); */
            You("썩은 달걀 냄새가 난다.");
        return FALSE;
    }
    return TRUE;
}

STATIC_PTR void display_stinking_cloud_positions(state) int state;
{
    if (state == 0) {
        tmp_at(DISP_BEAM, cmap_to_glyph(S_goodpos));
    } else if (state == 1) {
        int x, y, dx, dy;
        int dist = 6;

        for (dx = -dist; dx <= dist; dx++)
            for (dy = -dist; dy <= dist; dy++) {
                x = u.ux + dx;
                y = u.uy + dy;
                if (get_valid_stinking_cloud_pos(x, y))
                    tmp_at(x, y);
            }
    } else {
        tmp_at(DISP_END, 0);
    }
}

/* scroll effects; return 1 if we use up the scroll and possibly make it
   become discovered, 0 if caller should take care of those side-effects */
int
seffects(sobj)
struct obj *sobj; /* scroll, or fake spellbook object for scroll-like spell */
{
    int cval, otyp = sobj->otyp;
    boolean confused = (Confusion != 0), sblessed = sobj->blessed,
            scursed = sobj->cursed, already_known, old_erodeproof,
            new_erodeproof;
    struct obj *otmp;

    if (objects[otyp].oc_magic)
        exercise(A_WIS, TRUE);                    /* just for trying */
    already_known = (sobj->oclass == SPBOOK_CLASS /* spell */
                     || objects[otyp].oc_name_known);

    switch (otyp) {
#ifdef MAIL
    case SCR_MAIL:
        known = TRUE;
        if (sobj->spe == 2)
            /* "stamped scroll" created via magic marker--without a stamp */
            /*KR pline("This scroll is marked \"postage due\"."); */
            pline("이 두루마리에는 \"우편 요금 부족\"이라고 적혀 있다.");
        else if (sobj->spe)
            /* scroll of mail obtained from bones file or from wishing;
             * note to the puzzled: the game Larn actually sends you junk
             * mail if you win!
             */
            /*KR pline(
    "This seems to be junk mail addressed to the finder of the Eye of Larn.");
  */
            pline("이것은 란의 눈 발견자에게 보내는 스팸 메일인 것 같다.");
        else
            readmail(sobj);
        break;
#endif
    case SCR_ENCHANT_ARMOR: {
        register schar s;
        boolean special_armor;
        boolean same_color;

        otmp = some_armor(&youmonst);
#if 0 /*KR: 원본*/
        if (!otmp) {
            strange_feeling(sobj, !Blind
                                      ? "Your skin glows then fades."
                                      : "Your skin feels warm for a moment.");
#else /*KR: KRNethack 맞춤 번역 */
        if (!otmp) {
            strange_feeling(sobj, !Blind ? "피부가 빛나더니 이내 사그라졌다."
                                         : "피부가 잠시 따뜻하게 느껴졌다.");
#endif
            sobj = 0; /* useup() in strange_feeling() */
            exercise(A_CON, !scursed);
            exercise(A_STR, !scursed);
            break;
        }
        if (confused) {
            old_erodeproof = (otmp->oerodeproof != 0);
            new_erodeproof = !scursed;
            otmp->oerodeproof = 0; /* for messages */
#if 0                              /*KR: 원본*/
            if (Blind) {
                otmp->rknown = FALSE;
                pline("%s warm for a moment.", Yobjnam2(otmp, "feel"));
            } else {
                otmp->rknown = TRUE;
                pline("%s covered by a %s %s %s!", Yobjnam2(otmp, "are"),
                      scursed ? "mottled" : "shimmering",
                      hcolor(scursed ? NH_BLACK : NH_GOLDEN),
                      scursed ? "glow"
                              : (is_shield(otmp) ? "layer" : "shield"));
            }
            if (new_erodeproof && (otmp->oeroded || otmp->oeroded2)) {
                otmp->oeroded = otmp->oeroded2 = 0;
                pline("%s as good as new!",
                      Yobjnam2(otmp, Blind ? "feel" : "look"));
            }
#else                              /*KR: KRNethack 맞춤 번역 */
            if (Blind) {
                otmp->rknown = FALSE;
                pline("%s 잠시 따뜻하게 느껴졌다.",
                      append_josa(Yname2(otmp), "이"));
            } else {
                otmp->rknown = TRUE;
                pline("%s %s %s %s 덮였다!", append_josa(Yname2(otmp), "이"),
                      scursed ? "얼룩덜룩한" : "어른거리는",
                      hcolor(scursed ? NH_BLACK : NH_GOLDEN),
                      scursed ? "빛으로"
                              : (is_shield(otmp) ? "막으로" : "방패막으로"));
            }
            if (new_erodeproof && (otmp->oeroded || otmp->oeroded2)) {
                otmp->oeroded = otmp->oeroded2 = 0;
                pline("%s 새것처럼 %s!", append_josa(Yname2(otmp), "이"),
                      Blind ? "느껴진다" : "보인다");
            }
#endif
            if (old_erodeproof && !new_erodeproof) {
                /* restore old_erodeproof before shop charges */
                otmp->oerodeproof = 1;
                costly_alteration(otmp, COST_DEGRD);
            }
            otmp->oerodeproof = new_erodeproof ? 1 : 0;
            break;
        }
        /* elven armor vibrates warningly when enchanted beyond a limit */
        special_armor = is_elven_armor(otmp)
                        || (Role_if(PM_WIZARD) && otmp->otyp == CORNUTHAUM);
        if (scursed)
            same_color = (otmp->otyp == BLACK_DRAGON_SCALE_MAIL
                          || otmp->otyp == BLACK_DRAGON_SCALES);
        else
            same_color = (otmp->otyp == SILVER_DRAGON_SCALE_MAIL
                          || otmp->otyp == SILVER_DRAGON_SCALES
                          || otmp->otyp == SHIELD_OF_REFLECTION);
        if (Blind)
            same_color = FALSE;

        /* KMH -- catch underflow */
        s = scursed ? -otmp->spe : otmp->spe;
        if (s > (special_armor ? 5 : 3) && rn2(s)) {
#if 0 /*KR: 원본*/
            otmp->in_use = TRUE;
            pline("%s violently %s%s%s for a while, then %s.", Yname2(otmp),
                  otense(otmp, Blind ? "vibrate" : "glow"),
                  (!Blind && !same_color) ? " " : "",
                  (Blind || same_color) ? "" : hcolor(scursed ? NH_BLACK
                                                              : NH_SILVER),
                  otense(otmp, "evaporate"));
#else /*KR: KRNethack 맞춤 번역 */
            otmp->in_use = TRUE;
            pline("%s 한동안 맹렬하게 %s%s, 이내 증발했다.",
                  append_josa(Yname2(otmp), "이"),
                  (Blind || same_color)
                      ? ""
                      : hcolor(scursed ? NH_BLACK : NH_SILVER),
                  Blind ? " 진동하더니" : " 빛나더니");
#endif
            remove_worn_item(otmp, FALSE);
            useup(otmp);
            break;
        }
        s = scursed            ? -1
            : (otmp->spe >= 9) ? (rn2(otmp->spe) == 0)
            : sblessed         ? rnd(3 - otmp->spe / 3)
                               : 1;
        if (s >= 0 && Is_dragon_scales(otmp)) {
            /* dragon scales get turned into dragon scale mail */
#if 0 /*KR: 원본*/
            pline("%s merges and hardens!", Yname2(otmp));
#else /*KR: KRNethack 맞춤 번역 */
            pline("%s 융합되며 단단해졌다!", append_josa(Yname2(otmp), "이"));
#endif
            setworn((struct obj *) 0, W_ARM);
            /* assumes same order */
            otmp->otyp += GRAY_DRAGON_SCALE_MAIL - GRAY_DRAGON_SCALES;
            if (sblessed) {
                otmp->spe++;
                if (!otmp->blessed)
                    bless(otmp);
            } else if (otmp->cursed)
                uncurse(otmp);
            otmp->known = 1;
            setworn(otmp, W_ARM);
            if (otmp->unpaid)
                alter_cost(otmp, 0L); /* shop bill */
            break;
        }
#if 0 /*KR: 원본*/
        pline("%s %s%s%s%s for a %s.", Yname2(otmp),
              s == 0 ? "violently " : "",
              otense(otmp, Blind ? "vibrate" : "glow"),
              (!Blind && !same_color) ? " " : "",
              (Blind || same_color)
                 ? "" : hcolor(scursed ? NH_BLACK : NH_SILVER),
              (s * s > 1) ? "while" : "moment");
#else /*KR: KRNethack 맞춤 번역 */
        pline("%s %s %s%s%s.", append_josa(Yname2(otmp), "이"),
              (s * s > 1) ? "잠시 동안" : "잠깐", s == 0 ? "격렬하게 " : "",
              (Blind || same_color) ? ""
                                    : hcolor(scursed ? NH_BLACK : NH_SILVER),
              Blind ? " 진동했다" : " 빛났다");
#endif
        /* [this cost handling will need updating if shop pricing is
           ever changed to care about curse/bless status of armor] */
        if (s < 0)
            costly_alteration(otmp, COST_DECHNT);
        if (scursed && !otmp->cursed)
            curse(otmp);
        else if (sblessed && !otmp->blessed)
            bless(otmp);
        else if (!scursed && otmp->cursed)
            uncurse(otmp);
        if (s) {
            otmp->spe += s;
            adj_abon(otmp, s);
            known = otmp->known;
            /* update shop bill to reflect new higher price */
            if (s > 0 && otmp->unpaid)
                alter_cost(otmp, 0L);
        }

        if ((otmp->spe > (special_armor ? 5 : 3))
            && (special_armor || !rn2(7)))
            /*KR pline("%s %s.", Yobjnam2(otmp, "suddenly vibrate"),
             * Blind ? "again" : "unexpectedly"); */
            pline("%s %s 진동했다.", append_josa(Yname2(otmp), "이"),
                  Blind ? "다시" : "갑자기");
        break;
    }
    case SCR_DESTROY_ARMOR: {
        otmp = some_armor(&youmonst);
        if (confused) {
            if (!otmp) {
                /*KR strange_feeling(sobj, "Your bones itch."); */
                strange_feeling(sobj, "뼈가 가려운 느낌이 든다.");
                sobj = 0; /* useup() in strange_feeling() */
                exercise(A_STR, FALSE);
                exercise(A_CON, FALSE);
                break;
            }
            old_erodeproof = (otmp->oerodeproof != 0);
            new_erodeproof = scursed;
            otmp->oerodeproof = 0; /* for messages */
            p_glow2(otmp, NH_PURPLE);
            if (old_erodeproof && !new_erodeproof) {
                /* restore old_erodeproof before shop charges */
                otmp->oerodeproof = 1;
                costly_alteration(otmp, COST_DEGRD);
            }
            otmp->oerodeproof = new_erodeproof ? 1 : 0;
            break;
        }
        if (!scursed || !otmp || !otmp->cursed) {
            if (!destroy_arm(otmp)) {
                /*KR strange_feeling(sobj, "Your skin itches."); */
                strange_feeling(sobj, "피부가 가려운 느낌이 든다.");
                sobj = 0; /* useup() in strange_feeling() */
                exercise(A_STR, FALSE);
                exercise(A_CON, FALSE);
                break;
            } else
                known = TRUE;
        } else { /* armor and scroll both cursed */
            /*KR pline("%s.", Yobjnam2(otmp, "vibrate")); */
            pline("%s 진동했다.", append_josa(Yname2(otmp), "이"));
            if (otmp->spe >= -6) {
                otmp->spe += -1;
                adj_abon(otmp, -1);
            }
            make_stunned((HStun & TIMEOUT) + (long) rn1(10, 10), TRUE);
        }
    } break;
    case SCR_CONFUSE_MONSTER:
    case SPE_CONFUSE_MONSTER:
        if (youmonst.data->mlet != S_HUMAN || scursed) {
            if (!HConfusion)
                /*KR You_feel("confused."); */
                You("혼란스러움을 느낀다.");
            make_confused(HConfusion + rnd(100), FALSE);
        } else if (confused) {
#if 0 /*KR: 원본*/
            if (!sblessed) {
                Your("%s begin to %s%s.", makeplural(body_part(HAND)),
                     Blind ? "tingle" : "glow ",
                     Blind ? "" : hcolor(NH_PURPLE));
                make_confused(HConfusion + rnd(100), FALSE);
            } else {
                pline("A %s%s surrounds your %s.",
                      Blind ? "" : hcolor(NH_RED),
                      Blind ? "faint buzz" : " glow", body_part(HEAD));
                make_confused(0L, TRUE);
            }
#else /*KR: KRNethack 맞춤 번역 */
            if (!sblessed) {
                if (Blind)
                    Your("%s 따끔거리기 시작했다.",
                         append_josa(makeplural(body_part(HAND)), "이"));
                else
                    Your("%s %s 빛으로 빛나기 시작했다.",
                         append_josa(makeplural(body_part(HAND)), "이"),
                         hcolor(NH_PURPLE));
                make_confused(HConfusion + rnd(100), FALSE);
            } else {
                if (Blind)
                    pline("희미한 윙윙거림이 당신의 %s 감쌌다.",
                          append_josa(body_part(HEAD), "을"));
                else
                    pline("%s 빛이 당신의 %s 감쌌다.", hcolor(NH_RED),
                          append_josa(body_part(HEAD), "을"));
                make_confused(0L, TRUE);
            }
#endif
        } else {
#if 0 /*KR: 원본*/
            if (!sblessed) {
                Your("%s%s %s%s.", makeplural(body_part(HAND)),
                     Blind ? "" : " begin to glow",
                     Blind ? (const char *) "tingle" : hcolor(NH_RED),
                     u.umconf ? " even more" : "");
                u.umconf++;
            } else {
                if (Blind)
                    Your("%s tingle %s sharply.", makeplural(body_part(HAND)),
                         u.umconf ? "even more" : "very");
                else
                    Your("%s glow a%s brilliant %s.",
                         makeplural(body_part(HAND)),
                         u.umconf ? "n even more" : "", hcolor(NH_RED));
                /* after a while, repeated uses become less effective */
                if (u.umconf >= 40)
                    u.umconf++;
                else
                    u.umconf += rn1(8, 2);
            }
#else /*KR: KRNethack 맞춤 번역 */
            if (!sblessed) {
                if (Blind)
                    Your("%s %s따끔거린다.",
                         append_josa(makeplural(body_part(HAND)), "이"),
                         u.umconf ? "더욱 더 " : "");
                else
                    Your("%s %s%s 빛으로 빛나기 시작했다.",
                         append_josa(makeplural(body_part(HAND)), "이"),
                         u.umconf ? "더욱 더 " : "", hcolor(NH_RED));
                u.umconf++;
            } else {
                if (Blind)
                    Your("%s %s 따끔거린다.",
                         append_josa(makeplural(body_part(HAND)), "이"),
                         u.umconf ? "더욱 더 날카롭게" : "매우 날카롭게");
                else
                    Your("%s %s찬란한 %s 빛으로 빛난다.",
                         append_josa(makeplural(body_part(HAND)), "이"),
                         u.umconf ? "더욱 더 " : "", hcolor(NH_RED));

                /* after a while, repeated uses become less effective */
                if (u.umconf >= 40)
                    u.umconf++;
                else
                    u.umconf += rn1(8, 2);
            }
#endif
        }
        break;
    case SCR_SCARE_MONSTER:
    case SPE_CAUSE_FEAR: {
        register int ct = 0;
        register struct monst *mtmp;

        for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
            if (DEADMONSTER(mtmp))
                continue;
            if (cansee(mtmp->mx, mtmp->my)) {
                if (confused || scursed) {
                    mtmp->mflee = mtmp->mfrozen = mtmp->msleeping = 0;
                    mtmp->mcanmove = 1;
                } else if (!resist(mtmp, sobj->oclass, 0, NOTELL))
                    monflee(mtmp, 0, FALSE, FALSE);
                if (!mtmp->mtame)
                    ct++; /* pets don't laugh at you */
            }
        }
        if (otyp == SCR_SCARE_MONSTER || !ct)
            /*KR You_hear("%s %s.", (confused || scursed) ? "sad wailing"
             * : "maniacal laughter",
             * !ct ? "in the distance" : "close by"); */
            You("%s %s 들었다.", !ct ? "멀리서" : "가까이에서",
                     (confused || scursed) ? "슬픈 통곡 소리를"
                                           : "광기 어린 웃음소리를");
        break;
    }
    case SCR_BLANK_PAPER:
        if (Blind)
            /*KR You("don't remember there being any magic words on this
             * scroll."); */
            You("이 두루마리에 마법의 단어가 있었는지 기억나지 않는다.");
        else
            /*KR pline("This scroll seems to be blank."); */
            pline("이 두루마리는 백지인 것 같다.");
        known = TRUE;
        break;
    case SCR_REMOVE_CURSE:
    case SPE_REMOVE_CURSE: {
        register struct obj *obj;

#if 0 /*KR: 원본*/
        You_feel(!Hallucination
                 ? (!confused ? "like someone is helping you."
                              : "like you need some help.")
                 : (!confused ? "in touch with the Universal Oneness."
                              : "the power of the Force against you!"));
#else
        You(!Hallucination
                     ? (!confused ? "누군가 당신을 돕고 있는 것 같다."
                                  : "누군가의 도움이 필요한 것 같다.")
                     : (!confused ? "우주의 일체감과 맞닿은 것 같다."
                                  : "포스의 힘이 당신을 거스르는 것 같다!"));
#endif

        if (scursed) {
            /*KR pline_The("scroll disintegrates."); */
            pline("두루마리가 산산조각 났다.");
        } else {
            for (obj = invent; obj; obj = obj->nobj) {
                long wornmask;

                /* gold isn't subject to cursing and blessing */
                if (obj->oclass == COIN_CLASS)
                    continue;
                /* hide current scroll from itself so that perm_invent won't
                   show known blessed scroll losing bknown when confused */
                if (obj == sobj && obj->quan == 1L)
                    continue;
                wornmask = (obj->owornmask & ~(W_BALL | W_ART | W_ARTI));
                if (wornmask && !sblessed) {
                    /* handle a couple of special cases; we don't
                       allow auxiliary weapon slots to be used to
                       artificially increase number of worn items */
                    if (obj == uswapwep) {
                        if (!u.twoweap)
                            wornmask = 0L;
                    } else if (obj == uquiver) {
                        if (obj->oclass == WEAPON_CLASS) {
                            /* mergeable weapon test covers ammo,
                               missiles, spears, daggers & knives */
                            if (!objects[obj->otyp].oc_merge)
                                wornmask = 0L;
                        } else if (obj->oclass == GEM_CLASS) {
                            /* possibly ought to check whether
                               alternate weapon is a sling... */
                            if (!uslinging())
                                wornmask = 0L;
                        } else {
                            /* weptools don't merge and aren't
                               reasonable quivered weapons */
                            wornmask = 0L;
                        }
                    }
                }
                if (sblessed || wornmask || obj->otyp == LOADSTONE
                    || (obj->otyp == LEASH && obj->leashmon)) {
                    /* water price varies by curse/bless status */
                    boolean shop_h2o =
                        (obj->unpaid && obj->otyp == POT_WATER);

                    if (confused) {
                        blessorcurse(obj, 2);
                        /* lose knowledge of this object's curse/bless
                           state (even if it didn't actually change) */
                        obj->bknown = 0;
                        /* blessorcurse() only affects uncursed items
                           so no need to worry about price of water
                           going down (hence no costly_alteration) */
                        if (shop_h2o && (obj->cursed || obj->blessed))
                            alter_cost(obj, 0L); /* price goes up */
                    } else if (obj->cursed) {
                        if (shop_h2o)
                            costly_alteration(obj, COST_UNCURS);
                        uncurse(obj);
                    }
                }
            }
        }
        if (Punished && !confused)
            unpunish();
        if (u.utrap && u.utraptype == TT_BURIEDBALL) {
            buried_ball_to_freedom();
            /*KR pline_The("clasp on your %s vanishes.", body_part(LEG)); */
            pline("%s에 채워져 있던 수갑이 사라졌다.", body_part(LEG));
        }
        update_inventory();
        break;
    }
    case SCR_CREATE_MONSTER:
    case SPE_CREATE_MONSTER:
        if (create_critters(1 + ((confused || scursed) ? 12 : 0)
                                + ((sblessed || rn2(73)) ? 0 : rnd(4)),
                            confused ? &mons[PM_ACID_BLOB]
                                     : (struct permonst *) 0,
                            FALSE))
            known = TRUE;
        /* no need to flush monsters; we ask for identification only if the
         * monsters are not visible
         */
        break;
    case SCR_ENCHANT_WEAPON:
        /* [What about twoweapon mode?  Proofing/repairing/enchanting both
           would be too powerful, but shouldn't we choose randomly between
           primary and secondary instead of always acting on primary?] */
        if (confused && uwep && erosion_matters(uwep)
            && uwep->oclass != ARMOR_CLASS) {
            old_erodeproof = (uwep->oerodeproof != 0);
            new_erodeproof = !scursed;
            uwep->oerodeproof = 0; /* for messages */
            if (Blind) {
                uwep->rknown = FALSE;
                /*KR Your("weapon feels warm for a moment."); */
                Your("무기가 잠시 따뜻하게 느껴진다.");
            } else {
                uwep->rknown = TRUE;
#if 0 /*KR: 원본*/
                pline("%s covered by a %s %s %s!", Yobjnam2(uwep, "are"),
                      scursed ? "mottled" : "shimmering",
                      hcolor(scursed ? NH_PURPLE : NH_GOLDEN),
                      scursed ? "glow" : "shield");
#else /*KR: KRNethack 맞춤 번역 */
                pline("%s %s %s %s 덮였다!", append_josa(Yname2(uwep), "이"),
                      scursed ? "얼룩덜룩한" : "어른거리는",
                      hcolor(scursed ? NH_PURPLE : NH_GOLDEN),
                      scursed ? "빛으로" : "방패막으로");
#endif
            }
            if (new_erodeproof && (uwep->oeroded || uwep->oeroded2)) {
                uwep->oeroded = uwep->oeroded2 = 0;
                /*KR pline("%s as good as new!",
                 * Yobjnam2(uwep, Blind ? "feel" : "look")); */
                pline("%s 새것처럼 %s!", append_josa(Yname2(uwep), "이"),
                      Blind ? "느껴진다" : "보인다");
            }
            if (old_erodeproof && !new_erodeproof) {
                /* restore old_erodeproof before shop charges */
                uwep->oerodeproof = 1;
                costly_alteration(uwep, COST_DEGRD);
            }
            uwep->oerodeproof = new_erodeproof ? 1 : 0;
            break;
        }
        if (!chwepon(sobj, scursed            ? -1
                           : !uwep            ? 1
                           : (uwep->spe >= 9) ? !rn2(uwep->spe)
                           : sblessed         ? rnd(3 - uwep->spe / 3)
                                              : 1))
            sobj = 0; /* nothing enchanted: strange_feeling -> useup */
        break;
    case SCR_TAMING:
    case SPE_CHARM_MONSTER: {
        int candidates, res, results, vis_results;

        if (u.uswallow) {
            candidates = 1;
            results = vis_results = maybe_tame(u.ustuck, sobj);
        } else {
            int i, j, bd = confused ? 5 : 1;
            struct monst *mtmp;

            /* note: maybe_tame() can return either positive or
               negative values, but not both for the same scroll */
            candidates = results = vis_results = 0;
            for (i = -bd; i <= bd; i++)
                for (j = -bd; j <= bd; j++) {
                    if (!isok(u.ux + i, u.uy + j))
                        continue;
                    if ((mtmp = m_at(u.ux + i, u.uy + j)) != 0
                        || (!i && !j && (mtmp = u.usteed) != 0)) {
                        ++candidates;
                        res = maybe_tame(mtmp, sobj);
                        results += res;
                        if (canspotmon(mtmp))
                            vis_results += res;
                    }
                }
        }
        if (!results) {
            /*KR pline("Nothing interesting %s.",
             * !candidates ? "happens" : "seems to happen"); */
            pline("흥미로운 일은 %s.",
                  !candidates ? "일어나지 않았다" : "일어나지 않은 것 같다");
        } else {
#if 0 /*KR: 원본*/
            pline_The("neighborhood %s %sfriendlier.",
                      vis_results ? "is" : "seems",
                      (results < 0) ? "un" : "");
#else /*KR: KRNethack 맞춤 번역 */
            pline("주변이 %s %s.",
                  (results < 0) ? "적대적으로 변한 것" : "우호적으로 변한 것",
                  vis_results ? "같다" : "같다");
#endif
            if (vis_results > 0)
                known = TRUE;
        }
        break;
    }
    case SCR_GENOCIDE:
        if (!already_known)
            /*KR You("have found a scroll of genocide!"); */
            You("학살의 두루마리를 발견했다!");
        known = TRUE;
        if (sblessed)
            do_class_genocide();
        else
            do_genocide((!scursed) | (2 * !!Confusion));
        break;
    case SCR_LIGHT:
        if (!confused || rn2(5)) {
            if (!Blind)
                known = TRUE;
            litroom(!confused && !scursed, sobj);
            if (!confused && !scursed) {
                if (lightdamage(sobj, TRUE, 5))
                    known = TRUE;
            }
        } else {
            /* could be scroll of create monster, don't set known ...*/
            (void) create_critters(
                1, !scursed ? &mons[PM_YELLOW_LIGHT] : &mons[PM_BLACK_LIGHT],
                TRUE);
        }
        break;
    case SCR_TELEPORTATION:
        if (confused || scursed) {
            level_tele();
        } else {
            known = scrolltele(sobj);
        }
        break;
    case SCR_GOLD_DETECTION:
        if ((confused || scursed) ? trap_detect(sobj) : gold_detect(sobj))
            sobj = 0; /* failure: strange_feeling() -> useup() */
        break;
    case SCR_FOOD_DETECTION:
    case SPE_DETECT_FOOD:
        if (food_detect(sobj))
            sobj = 0; /* nothing detected: strange_feeling -> useup */
        break;
    case SCR_IDENTIFY:
        /* known = TRUE; -- handled inline here */
        /* use up the scroll first, before makeknown() performs a
           perm_invent update; also simplifies empty invent check */
        useup(sobj);
        sobj = 0; /* it's gone */
        if (confused)
            /*KR You("identify this as an identify scroll."); */
            You("이것이 식별의 두루마리라는 것을 식별해 냈다.");
        else if (!already_known || !invent)
            /* force feedback now if invent became
               empty after using up this scroll */
            /*KR pline("This is an identify scroll."); */
            pline("이것은 식별의 두루마리다.");
        if (!already_known)
            (void) learnscrolltyp(SCR_IDENTIFY);
        /*FALLTHRU*/
    case SPE_IDENTIFY:
        cval = 1;
        if (sblessed || (!scursed && !rn2(5))) {
            cval = rn2(5);
            /* note: if cval==0, identify all items */
            if (cval == 1 && sblessed && Luck > 0)
                ++cval;
        }
        if (invent && !confused) {
            identify_pack(cval, !already_known);
        } else if (otyp == SPE_IDENTIFY) {
            /* when casting a spell we know we're not confused,
               so inventory must be empty (another message has
               already been given above if reading a scroll) */
            /*KR pline("You're not carrying anything to be identified."); */
            pline("식별할 물건을 하나도 가지고 있지 않다.");
        }
        break;
    case SCR_CHARGING:
        if (confused) {
            if (scursed) {
                /*KR You_feel("discharged."); */
                You("방전된 기분이다.");
                u.uen = 0;
            } else {
                /*KR You_feel("charged up!"); */
                You("충전된 기분이다!");
                u.uen += d(sblessed ? 6 : 4, 4);
                if (u.uen > u.uenmax) /* if current energy is already at   */
                    u.uenmax = u.uen; /* or near maximum, increase maximum */
                else
                    u.uen = u.uenmax; /* otherwise restore current to max  */
            }
            context.botl = 1;
            break;
        }
        /* known = TRUE; -- handled inline here */
        if (!already_known) {
            /*KR pline("This is a charging scroll."); */
            pline("이것은 충전의 두루마리다.");
            learnscroll(sobj);
        }
        /* use it up now to prevent it from showing in the
           getobj picklist because the "disappears" message
           was already delivered */
        useup(sobj);
        sobj = 0; /* it's gone */
        otmp = getobj(all_count, "charge");
        if (otmp)
            recharge(otmp, scursed ? -1 : sblessed ? 1 : 0);
        break;
    case SCR_MAGIC_MAPPING:
        if (level.flags.nommap) {
            /*KR Your("mind is filled with crazy lines!"); */
            Your("머릿속이 미친 듯한 선들로 가득 찼다!");
            if (Hallucination)
                /*KR pline("Wow!  Modern art."); */
                pline("와! 현대 미술이네.");
            else
                /*KR Your("%s spins in bewilderment.", body_part(HEAD)); */
                Your("%s 당혹감에 빙빙 돈다.",
                     append_josa(body_part(HEAD), "이"));
            make_confused(HConfusion + rnd(30), FALSE);
            break;
        }
        if (sblessed) {
            register int x, y;

            for (x = 1; x < COLNO; x++)
                for (y = 0; y < ROWNO; y++)
                    if (levl[x][y].typ == SDOOR)
                        cvt_sdoor_to_door(&levl[x][y]);
            /* do_mapping() already reveals secret passages */
        }
        known = TRUE;
        /*FALLTHRU*/
    case SPE_MAGIC_MAPPING:
        if (level.flags.nommap) {
            /*KR Your("%s spins as %s blocks the spell!", body_part(HEAD),
             * something); */
            Your("%s 마법을 막아내어 %s 빙빙 돈다!",
                 append_josa(something, "이"),
                 append_josa(body_part(HEAD), "이"));
            make_confused(HConfusion + rnd(30), FALSE);
            break;
        }
        /*KR pline("A map coalesces in your mind!"); */
        pline("머릿속에서 지도가 그려진다!");
        cval = (scursed && !confused);
        if (cval)
            HConfusion = 1; /* to screw up map */
        do_mapping();
        if (cval) {
            HConfusion = 0; /* restore */
            /*KR pline("Unfortunately, you can't grasp the details."); */
            pline("불행히도, 세부적인 것까지는 파악할 수 없다.");
        }
        break;
    case SCR_AMNESIA:
        known = TRUE;
        forget((!sblessed ? ALL_SPELLS : 0)
               | (!confused || scursed ? ALL_MAP : 0));
#if 0                      /*KR: 원본*/
        if (Hallucination) /* Ommmmmm! */
            Your("mind releases itself from mundane concerns.");
        else if (!strncmpi(plname, "Maud", 4))
            pline(
          "As your mind turns inward on itself, you forget everything else.");
        else if (rn2(2))
            pline("Who was that Maud person anyway?");
        else
            pline("Thinking of Maud you forget everything else.");
#else                      /*KR: KRNethack 맞춤 번역 */
        if (Hallucination) /* Ommmmmm! */
            Your("마음이 세속적인 번뇌에서 해방되었다.");
        else if (!strncmp(plname, "Maud", 4))
            pline("마음이 내면으로 향하면서, 당신은 다른 모든 것을 "
                  "잊어버렸다.");
        else if (rn2(2))
            pline("도대체 그 모드 라는 사람은 누구였지?");
        else
            pline("모드를 생각하느라 당신은 다른 모든 것을 잊어버렸다.");
#endif
        exercise(A_WIS, FALSE);
        break;
    case SCR_FIRE: {
        coord cc;
        int dam;

        cc.x = u.ux;
        cc.y = u.uy;
        cval = bcsign(sobj);
        dam = (2 * (rn1(3, 3) + 2 * cval) + 1) / 3;
        useup(sobj);
        sobj = 0; /* it's gone */
        if (!already_known)
            (void) learnscrolltyp(SCR_FIRE);
        if (confused) {
            if (Fire_resistance) {
                shieldeff(u.ux, u.uy);
                if (!Blind)
                    /*KR pline("Oh, look, what a pretty fire in your %s.",
                     * makeplural(body_part(HAND))); */
                    pline("오, 이런, %s에 참 예쁜 불이 붙었네.",
                          makeplural(body_part(HAND)));
                else
                    /*KR You_feel("a pleasant warmth in your %s.",
                     * makeplural(body_part(HAND))); */
                    You("%s에서 기분 좋은 온기를 느꼈다.",
                             makeplural(body_part(HAND)));
            } else {
                /*KR pline_The("scroll catches fire and you burn your %s.",
                 * makeplural(body_part(HAND))); */
                pline("두루마리에 불이 붙어 %s 데었다.",
                      makeplural(body_part(HAND)));
                losehp(1, "불의 두루마리", KILLED_BY_AN);
            }
            break;
        }
        if (Underwater) {
            /*KR pline_The("%s around you vaporizes violently!",
             * hliquid("water")); */
            pline("당신 주변의 %s 격렬하게 증발했다!",
                  append_josa(hliquid("물"), "이"));
        } else {
            if (sblessed) {
                if (!already_known)
                    /*KR pline("This is a scroll of fire!"); */
                    pline("이것은 불의 두루마리다!");
                dam *= 5;
                /*KR pline("Where do you want to center the explosion?"); */
                pline("어디를 중심으로 폭발시키겠습니까?");
                getpos_sethilite(display_stinking_cloud_positions,
                                 get_valid_stinking_cloud_pos);
                /*KR (void) getpos(&cc, TRUE, "the desired position"); */
                (void) getpos(&cc, TRUE, "원하는 위치");
                if (!is_valid_stinking_cloud_pos(cc.x, cc.y, FALSE)) {
                    /* try to reach too far, get burned */
                    cc.x = u.ux;
                    cc.y = u.uy;
                }
            }
            if (cc.x == u.ux && cc.y == u.uy) {
                /*KR pline_The("scroll erupts in a tower of flame!"); */
                pline("두루마리가 화염의 탑으로 분출했다!");
                iflags.last_msg = PLNMSG_TOWER_OF_FLAME; /* for explode() */
                burn_away_slime();
            }
        }
        explode(cc.x, cc.y, 11, dam, SCROLL_CLASS, EXPL_FIERY);
        break;
    }
    case SCR_EARTH:
        /* TODO: handle steeds */
        if (!Is_rogue_level(&u.uz) && has_ceiling(&u.uz)
            && (!In_endgame(&u.uz) || Is_earthlevel(&u.uz))) {
            register int x, y;
            int nboulders = 0;

            /* Identify the scroll */
            if (u.uswallow)
                /*KR You_hear("rumbling."); */
                You("우르릉거리는 소리를 들었다.");
            else
#if 0 /*KR: 원본*/
                pline_The("%s rumbles %s you!", ceiling(u.ux, u.uy),
                          sblessed ? "around" : "above");
#else /*KR: KRNethack 맞춤 번역 */
                pline("%s 당신 %s 우르릉거린다!",
                      append_josa(ceiling(u.ux, u.uy), "이"),
                      sblessed ? "주변에서" : "위에서");
#endif
            known = 1;
            sokoban_guilt();

            /* Loop through the surrounding squares */
            if (!scursed)
                for (x = u.ux - 1; x <= u.ux + 1; x++) {
                    for (y = u.uy - 1; y <= u.uy + 1; y++) {
                        /* Is this a suitable spot? */
                        if (isok(x, y) && !closed_door(x, y)
                            && !IS_ROCK(levl[x][y].typ)
                            && !IS_AIR(levl[x][y].typ)
                            && (x != u.ux || y != u.uy)) {
                            nboulders +=
                                drop_boulder_on_monster(x, y, confused, TRUE);
                        }
                    }
                }
            /* Attack the player */
            if (!sblessed) {
                drop_boulder_on_player(confused, !scursed, TRUE, FALSE);
            } else if (!nboulders)
                /*KR pline("But nothing else happens."); */
                pline("하지만 그 외엔 아무 일도 일어나지 않았다.");
        }
        break;
    case SCR_PUNISHMENT:
        known = TRUE;
        if (confused || sblessed) {
            /*KR You_feel("guilty."); */
            You("죄책감이 든다.");
            break;
        }
        punish(sobj);
        break;
    case SCR_STINKING_CLOUD: {
        coord cc;

        if (!already_known)
            /*KR You("have found a scroll of stinking cloud!"); */
            You("악취나는 구름의 두루마리를 찾아냈다!");
        known = TRUE;
        /*KR pline("Where do you want to center the %scloud?",
         * already_known ? "stinking " : ""); */
        pline("어디를 중심으로 %s구름을 피우겠습니까?",
              already_known ? "악취나는 " : "");
        cc.x = u.ux;
        cc.y = u.uy;
        getpos_sethilite(display_stinking_cloud_positions,
                         get_valid_stinking_cloud_pos);
        if (getpos(&cc, TRUE, "원하는 위치") < 0) {
            pline1(Never_mind);
            break;
        }
        if (!is_valid_stinking_cloud_pos(cc.x, cc.y, TRUE))
            break;
        (void) create_gas_cloud(cc.x, cc.y, 3 + bcsign(sobj),
                                8 + 4 * bcsign(sobj));
        break;
    }
    default:
        impossible("What weird effect is this? (%u)", otyp);
    }
    /* if sobj is gone, we've already called useup() above and the
       update_inventory() that it performs might have come too soon
       (before charging an item, for instance) */
    if (!sobj)
        update_inventory();
    return sobj ? 0 : 1;
}

void drop_boulder_on_player(confused, helmet_protects, byu,
                            skip_uswallow) boolean confused,
    helmet_protects, byu, skip_uswallow;
{
    int dmg;
    struct obj *otmp2;

    /* hit monster if swallowed */
    if (u.uswallow && !skip_uswallow) {
        drop_boulder_on_monster(u.ux, u.uy, confused, byu);
        return;
    }

    otmp2 = mksobj(confused ? ROCK : BOULDER, FALSE, FALSE);
    if (!otmp2)
        return;
    otmp2->quan = confused ? rn1(5, 2) : 1;
    otmp2->owt = weight(otmp2);
    if (!amorphous(youmonst.data) && !Passes_walls
        && !noncorporeal(youmonst.data) && !unsolid(youmonst.data)) {
#if 0 /*KR: 원본*/
        You("are hit by %s!", doname(otmp2));
        dmg = dmgval(otmp2, &youmonst) * otmp2->quan;
        if (uarmh && helmet_protects) {
            if (is_metallic(uarmh)) {
                pline("Fortunately, you are wearing a hard helmet.");
                if (dmg > 2)
                    dmg = 2;
            } else if (flags.verbose) {
                pline("%s does not protect you.", Yname2(uarmh));
            }
        }
#else /*KR: KRNethack 맞춤 번역 */
        You("%s 맞았다!", append_josa(doname(otmp2), "에"));
        dmg = dmgval(otmp2, &youmonst) * otmp2->quan;
        if (uarmh && helmet_protects) {
            if (is_metallic(uarmh)) {
                pline("다행히도, 당신은 단단한 투구를 쓰고 있다.");
                if (dmg > 2)
                    dmg = 2;
            } else if (flags.verbose) {
                pline("%s 당신을 보호해주지 못했다.",
                      append_josa(Yname2(uarmh), "은"));
            }
        }
#endif
    } else
        dmg = 0;
    wake_nearto(u.ux, u.uy, 4 * 4);
    /* Must be before the losehp(), for bones files */
    if (!flooreffects(otmp2, u.ux, u.uy, "fall")) {
        place_object(otmp2, u.ux, u.uy);
        stackobj(otmp2);
        newsym(u.ux, u.uy);
    }
    if (dmg)
        losehp(Maybe_Half_Phys(dmg), "대지의 두루마리", KILLED_BY_AN);
}

boolean
drop_boulder_on_monster(x, y, confused, byu)
int x, y;
boolean confused, byu;
{
    register struct obj *otmp2;
    register struct monst *mtmp;

    /* Make the object(s) */
    otmp2 = mksobj(confused ? ROCK : BOULDER, FALSE, FALSE);
    if (!otmp2)
        return FALSE; /* Shouldn't happen */
    otmp2->quan = confused ? rn1(5, 2) : 1;
    otmp2->owt = weight(otmp2);

    /* Find the monster here (won't be player) */
    mtmp = m_at(x, y);
    if (mtmp && !amorphous(mtmp->data) && !passes_walls(mtmp->data)
        && !noncorporeal(mtmp->data) && !unsolid(mtmp->data)) {
        struct obj *helmet = which_armor(mtmp, W_ARMH);
        int mdmg;

#if 0 /*KR: 원본*/
        if (cansee(mtmp->mx, mtmp->my)) {
            pline("%s is hit by %s!", Monnam(mtmp), doname(otmp2));
            if (mtmp->minvis && !canspotmon(mtmp))
                map_invisible(mtmp->mx, mtmp->my);
        } else if (u.uswallow && mtmp == u.ustuck)
            You_hear("something hit %s %s over your %s!",
                     s_suffix(mon_nam(mtmp)), mbodypart(mtmp, STOMACH),
                     body_part(HEAD));

        mdmg = dmgval(otmp2, mtmp) * otmp2->quan;
        if (helmet) {
            if (is_metallic(helmet)) {
                if (canspotmon(mtmp))
                    pline("Fortunately, %s is wearing a hard helmet.",
                          mon_nam(mtmp));
                else if (!Deaf)
                    You_hear("a clanging sound.");
                if (mdmg > 2)
                    mdmg = 2;
            } else {
                if (canspotmon(mtmp))
                    pline("%s's %s does not protect %s.", Monnam(mtmp),
                          xname(helmet), mhim(mtmp));
            }
        }
#else /*KR: KRNethack 맞춤 번역 */
        if (cansee(mtmp->mx, mtmp->my)) {
            pline("%s %s 맞았다!", append_josa(Monnam(mtmp), "은"),
                  append_josa(doname(otmp2), "에"));
            if (mtmp->minvis && !canspotmon(mtmp))
                map_invisible(mtmp->mx, mtmp->my);
        } else if (u.uswallow && mtmp == u.ustuck)
            You("당신의 %s 위에서 무언가가 %s %s 때리는 소리를 들었다!",
                     body_part(HEAD), s_suffix(mon_nam(mtmp)),
                     mbodypart(mtmp, STOMACH));

        mdmg = dmgval(otmp2, mtmp) * otmp2->quan;
        if (helmet) {
            if (is_metallic(helmet)) {
                if (canspotmon(mtmp))
                    pline("다행히도, %s 단단한 투구를 쓰고 있다.",
                          append_josa(mon_nam(mtmp), "은"));
                else if (!Deaf)
                    You("쨍그랑거리는 소리를 들었다.");
                if (mdmg > 2)
                    mdmg = 2;
            } else {
                if (canspotmon(mtmp))
                    pline("%s %s %s 보호해주지 못했다.",
                          s_suffix(Monnam(mtmp)),
                          append_josa(xname(helmet), "은"),
                          append_josa(mhim(mtmp), "를"));
            }
        }
#endif
        mtmp->mhp -= mdmg;
        if (DEADMONSTER(mtmp)) {
            if (byu) {
                killed(mtmp);
            } else {
                /*KR pline("%s is killed.", Monnam(mtmp)); */
                pline("%s 죽었다.", append_josa(Monnam(mtmp), "은"));
                mondied(mtmp);
            }
        } else {
            wakeup(mtmp, byu);
        }
        wake_nearto(x, y, 4 * 4);
    } else if (u.uswallow && mtmp == u.ustuck) {
        obfree(otmp2, (struct obj *) 0);
        /* fall through to player */
        drop_boulder_on_player(confused, TRUE, FALSE, TRUE);
        return 1;
    }
    /* Drop the rock/boulder to the floor */
    if (!flooreffects(otmp2, x, y, "fall")) {
        place_object(otmp2, x, y);
        stackobj(otmp2);
        newsym(x, y); /* map the rock */
    }
    return TRUE;
}

/* overcharging any wand or zapping/engraving cursed wand */
void wand_explode(obj, chg) struct obj *obj;
int chg; /* recharging */
{
#if 0 /*KR: 원본*/
    const char *expl = !chg ? "suddenly" : "vibrates violently and";
#else /*KR: KRNethack 맞춤 번역 */
    const char *expl = !chg ? "갑자기" : "격렬하게 진동하더니";
#endif
    int dmg, n, k;

    /* number of damage dice */
    if (!chg)
        chg = 2; /* zap/engrave adjustment */
    n = obj->spe + chg;
    if (n < 2)
        n = 2; /* arbitrary minimum */
    /* size of damage dice */
    switch (obj->otyp) {
    case WAN_WISHING:
        k = 12;
        break;
    case WAN_CANCELLATION:
    case WAN_DEATH:
    case WAN_POLYMORPH:
    case WAN_UNDEAD_TURNING:
        k = 10;
        break;
    case WAN_COLD:
    case WAN_FIRE:
    case WAN_LIGHTNING:
    case WAN_MAGIC_MISSILE:
        k = 8;
        break;
    case WAN_NOTHING:
        k = 4;
        break;
    default:
        k = 6;
        break;
    }
    /* inflict damage and destroy the wand */
    dmg = d(n, k);
    obj->in_use = TRUE; /* in case losehp() is fatal (or --More--^C) */
#if 0                   /*KR: 원본*/
    pline("%s %s explodes!", Yname2(obj), expl);
    losehp(Maybe_Half_Phys(dmg), "exploding wand", KILLED_BY_AN);
#else                   /*KR: KRNethack 맞춤 번역 */
    pline("%s %s 폭발했다!", append_josa(Yname2(obj), "이"), expl);
    losehp(Maybe_Half_Phys(dmg), "폭발하는 마법 막대", KILLED_BY_AN);
#endif
    useup(obj);
    /* obscure side-effect */
    exercise(A_STR, FALSE);
}

/* used to collect gremlins being hit by light so that they can be processed
   after vision for the entire lit area has been brought up to date */
struct litmon {
    struct monst *mon;
    struct litmon *nxt;
};
STATIC_VAR struct litmon *gremlins = 0;

/*
 * Low-level lit-field update routine.
 */
STATIC_PTR void set_lit(x, y, val) int x, y;
genericptr_t val;
{
    struct monst *mtmp;
    struct litmon *gremlin;

    if (val) {
        levl[x][y].lit = 1;
        if ((mtmp = m_at(x, y)) != 0 && mtmp->data == &mons[PM_GREMLIN]) {
            gremlin = (struct litmon *) alloc(sizeof *gremlin);
            gremlin->mon = mtmp;
            gremlin->nxt = gremlins;
            gremlins = gremlin;
        }
    } else {
        levl[x][y].lit = 0;
        snuff_light_source(x, y);
    }
}

void litroom(on, obj) register boolean on;
struct obj *obj;
{
    char is_lit; /* value is irrelevant; we use its address
                    as a `not null' flag for set_lit() */

    /* first produce the text (provided you're not blind) */
    if (!on) {
        register struct obj *otmp;

        if (!Blind) {
            if (u.uswallow) {
                /*KR pline("It seems even darker in here than before."); */
                pline("여기 안이 전보다 더 어두워진 것 같다.");
            } else {
                if (uwep && artifact_light(uwep) && uwep->lamplit)
#if 0 /*KR: 원본*/
                    pline("Suddenly, the only light left comes from %s!",
                          the(xname(uwep)));
#else /*KR: KRNethack 맞춤 번역 */
                    pline("갑자기, 유일하게 남은 빛은 %s 나온다!",
                          append_josa(the(xname(uwep)), "에서"));
#endif
                else
                    /*KR You("are surrounded by darkness!"); */
                    You("어둠에 휩싸였다!");
            }
        }

        /* the magic douses lamps, et al, too */
        for (otmp = invent; otmp; otmp = otmp->nobj)
            if (otmp->lamplit)
                (void) snuff_lit(otmp);
    } else { /* on */
        if (u.uswallow) {
            if (Blind)
                ; /* no feedback */
            else if (is_animal(u.ustuck->data))
                /*KR pline("%s %s is lit.", s_suffix(Monnam(u.ustuck)),
                 * mbodypart(u.ustuck, STOMACH)); */
                pline("%s %s 안이 밝아졌다.", s_suffix(Monnam(u.ustuck)),
                      mbodypart(u.ustuck, STOMACH));
            else if (is_whirly(u.ustuck->data))
#if 0 /*KR: 원본*/
                pline("%s shines briefly.", Monnam(u.ustuck));
#else /*KR: KRNethack 맞춤 번역 */
                pline("%s 잠시 빛났다.", append_josa(Monnam(u.ustuck), "이"));
#endif
            else
#if 0 /*KR: 원본*/
                pline("%s glistens.", Monnam(u.ustuck));
#else /*KR: KRNethack 맞춤 번역 */
                pline("%s 반짝였다.", append_josa(Monnam(u.ustuck), "이"));
#endif
        } else if (!Blind)
            /*KR pline("A lit field surrounds you!"); */
            pline("빛의 장막이 당신을 둘러쌌다!");
    }

    /* No-op when swallowed or in water */
    if (u.uswallow || Underwater || Is_waterlevel(&u.uz))
        return;
    /*
     * If we are darkening the room and the hero is punished but not
     * blind, then we have to pick up and replace the ball and chain so
     * that we don't remember them if they are out of sight.
     */
    if (Punished && !on && !Blind)
        move_bc(1, 0, uball->ox, uball->oy, uchain->ox, uchain->oy);

    if (Is_rogue_level(&u.uz)) {
        /* Can't use do_clear_area because MAX_RADIUS is too small */
        /* rogue lighting must light the entire room */
        int rnum = levl[u.ux][u.uy].roomno - ROOMOFFSET;
        int rx, ry;

        if (rnum >= 0) {
            for (rx = rooms[rnum].lx - 1; rx <= rooms[rnum].hx + 1; rx++)
                for (ry = rooms[rnum].ly - 1; ry <= rooms[rnum].hy + 1; ry++)
                    set_lit(rx, ry,
                            (genericptr_t) (on ? &is_lit : (char *) 0));
            rooms[rnum].rlit = on;
        }
        /* hallways remain dark on the rogue level */
    } else
        do_clear_area(
            u.ux, u.uy,
            (obj && obj->oclass == SCROLL_CLASS && obj->blessed) ? 9 : 5,
            set_lit, (genericptr_t) (on ? &is_lit : (char *) 0));

    /*
     * If we are not blind, then force a redraw on all positions in sight
     * by temporarily blinding the hero.  The vision recalculation will
     * correctly update all previously seen positions *and* correctly
     * set the waslit bit [could be messed up from above].
     */
    if (!Blind) {
        vision_recalc(2);

        /* replace ball&chain */
        if (Punished && !on)
            move_bc(0, 0, uball->ox, uball->oy, uchain->ox, uchain->oy);
    }

    vision_full_recalc = 1; /* delayed vision recalculation */
    if (gremlins) {
        struct litmon *gremlin;

        /* can't delay vision recalc after all */
        vision_recalc(0);
        /* after vision has been updated, monsters who are affected
           when hit by light can now be hit by it */
        do {
            gremlin = gremlins;
            gremlins = gremlin->nxt;
            light_hits_gremlin(gremlin->mon, rnd(5));
            free((genericptr_t) gremlin);
        } while (gremlins);
    }
}

STATIC_OVL void
do_class_genocide()
{
    int i, j, immunecnt, gonecnt, goodcnt, class, feel_dead = 0;
    char buf[BUFSZ] = DUMMY;
    boolean gameover = FALSE; /* true iff killed self */

    for (j = 0;; j++) {
        if (j >= 5) {
            pline1(thats_enough_tries);
            return;
        }
        do {
            /*KR getlin("What class of monsters do you wish to genocide?",
             * buf); */
            getlin("어떤 종류의 몬스터를 멸종시키겠습니까?", buf);
            (void) mungspaces(buf);
        } while (!*buf);
        /* choosing "none" preserves genocideless conduct */
        if (*buf == '\033' || !strcmpi(buf, "none")
            || !strcmpi(buf, "nothing"))
            return;

        class = name_to_monclass(buf, (int *) 0);
        if (class == 0 && (i = name_to_mon(buf)) != NON_PM)
            class = mons[i].mlet;
        immunecnt = gonecnt = goodcnt = 0;
        for (i = LOW_PM; i < NUMMONS; i++) {
            if (mons[i].mlet == class) {
                if (!(mons[i].geno & G_GENO))
                    immunecnt++;
                else if (mvitals[i].mvflags & G_GENOD)
                    gonecnt++;
                else
                    goodcnt++;
            }
        }
        if (!goodcnt && class != mons[urole.malenum].mlet
            && class != mons[urace.malenum].mlet) {
#if 0 /*KR: 원본*/
            if (gonecnt)
                pline("All such monsters are already nonexistent.");
            else if (immunecnt || class == S_invisible)
                You("aren't permitted to genocide such monsters.");
            else if (wizard && buf[0] == '*') {
                register struct monst *mtmp, *mtmp2;

                gonecnt = 0;
                for (mtmp = fmon; mtmp; mtmp = mtmp2) {
                    mtmp2 = mtmp->nmon;
                    if (DEADMONSTER(mtmp))
                        continue;
                    mongone(mtmp);
                    gonecnt++;
                }
                pline("Eliminated %d monster%s.", gonecnt, plur(gonecnt));
                return;
            } else
                pline("That %s does not represent any monster.",
                      strlen(buf) == 1 ? "symbol" : "response");
#else /*KR: KRNethack 맞춤 번역 */
            if (gonecnt)
                pline("그러한 몬스터들은 이미 존재하지 않는다.");
            else if (immunecnt || class == S_invisible)
                You("그러한 몬스터들을 멸종시키는 것은 허용되지 않는다.");
            else if (wizard && buf[0] == '*') {
                register struct monst *mtmp, *mtmp2;

                gonecnt = 0;
                for (mtmp = fmon; mtmp; mtmp = mtmp2) {
                    mtmp2 = mtmp->nmon;
                    if (DEADMONSTER(mtmp))
                        continue;
                    mongone(mtmp);
                    gonecnt++;
                }
                pline("몬스터 %d마리를 제거했다.", gonecnt);
                return;
            } else
                pline("그 %s 어떤 몬스터도 나타내지 않는다.",
                      strlen(buf) == 1 ? "기호는" : "응답은");
#endif
            continue;
        }

        for (i = LOW_PM; i < NUMMONS; i++) {
            if (mons[i].mlet == class) {
                char nam[BUFSZ];

                Strcpy(nam, makeplural(mons[i].mname));
                /* Although "genus" is Latin for race, the hero benefits
                 * from both race and role; thus genocide affects either.
                 */
                if (Your_Own_Role(i) || Your_Own_Race(i)
                    || ((mons[i].geno & G_GENO)
                        && !(mvitals[i].mvflags & G_GENOD))) {
                    /* This check must be first since player monsters might
                     * have G_GENOD or !G_GENO.
                     */
                    mvitals[i].mvflags |= (G_GENOD | G_NOCORPSE);
                    reset_rndmonst(i);
                    kill_genocided_monsters();
                    update_inventory(); /* eggs & tins */
#if 0                                   /*KR: 원본*/
                    pline("Wiped out all %s.", nam);
#else                                   /*KR: KRNethack 맞춤 번역 */
                    pline("모든 %s 싹 쓸어버렸다.", append_josa(nam, "을"));
#endif
                    if (Upolyd && i == u.umonnum) {
                        u.mh = -1;
                        if (Unchanging) {
                            if (!feel_dead++)
                                /*KR You("die."); */
                                You("죽었다.");
                            /* finish genociding this class of
                               monsters before ultimately dying */
                            gameover = TRUE;
                        } else
                            rehumanize();
                    }
                    /* Self-genocide if it matches either your race
                       or role.  Assumption:  male and female forms
                       share same monster class. */
                    if (i == urole.malenum || i == urace.malenum) {
                        u.uhp = -1;
                        if (Upolyd) {
                            if (!feel_dead++)
                                /*KR You_feel("%s inside.", udeadinside()); */
                                You("내면이 %s.", udeadinside());
                        } else {
                            if (!feel_dead++)
                                /*KR You("die."); */
                                You("죽었다.");
                            gameover = TRUE;
                        }
                    }
                } else if (mvitals[i].mvflags & G_GENOD) {
                    if (!gameover)
#if 0 /*KR: 원본*/
                        pline("All %s are already nonexistent.", nam);
#else /*KR: KRNethack 맞춤 번역 */
                        pline("모든 %s 이미 존재하지 않는다.",
                              append_josa(nam, "은"));
#endif
                } else if (!gameover) {
                    /* suppress feedback about quest beings except
                       for those applicable to our own role */
                    if ((mons[i].msound != MS_LEADER
                         || quest_info(MS_LEADER) == i)
                        && (mons[i].msound != MS_NEMESIS
                            || quest_info(MS_NEMESIS) == i)
                        && (mons[i].msound != MS_GUARDIAN
                            || quest_info(MS_GUARDIAN) == i)
                        /* non-leader/nemesis/guardian role-specific monster
                         */
                        && (i != PM_NINJA /* nuisance */
                            || Role_if(PM_SAMURAI))) {
                        boolean named, uniq;

                        named = type_is_pname(&mons[i]) ? TRUE : FALSE;
                        uniq = (mons[i].geno & G_UNIQ) ? TRUE : FALSE;
                        /* one special case */
                        if (i == PM_HIGH_PRIEST)
                            uniq = FALSE;

#if 0 /*KR: 원본*/
                        You("aren't permitted to genocide %s%s.",
                            (uniq && !named) ? "the " : "",
                            (uniq || named) ? mons[i].mname : nam);
#else /*KR: KRNethack 맞춤 번역 */
                        You("%s 멸종시키는 것은 허용되지 않는다.",
                            append_josa((uniq || named) ? mons[i].mname : nam,
                                        "을"));
#endif
                    }
                }
            }
        }
        if (gameover || u.uhp == -1) {
            killer.format = KILLED_BY_AN;
            /*KR Strcpy(killer.name, "scroll of genocide"); */
            Strcpy(killer.name, "학살의 두루마리");
            if (gameover)
                done(GENOCIDED);
        }
        return;
    }
}

#define REALLY 1
#define PLAYER 2
#define ONTHRONE 4
void do_genocide(how) int how;
/* 0 = no genocide; create monsters (cursed scroll) */
/* 1 = normal genocide */
/* 3 = forced genocide of player */
/* 5 (4 | 1) = normal genocide from throne */
{
    char buf[BUFSZ] = DUMMY;
    register int i, killplayer = 0;
    register int mndx;
    register struct permonst *ptr;
    const char *which;

    if (how & PLAYER) {
        mndx = u.umonster; /* non-polymorphed mon num */
        ptr = &mons[mndx];
        Strcpy(buf, ptr->mname);
        killplayer++;
    } else {
        for (i = 0;; i++) {
            if (i >= 5) {
                /* cursed effect => no free pass (unless rndmonst() fails) */
                if (!(how & REALLY) && (ptr = rndmonst()) != 0)
                    break;

                pline1(thats_enough_tries);
                return;
            }
            /*KR getlin("What monster do you want to genocide? [type the
             * name]", buf); */
            getlin("어떤 몬스터를 멸종시키겠습니까? [이름을 입력하세요]",
                   buf);
            (void) mungspaces(buf);
            /* choosing "none" preserves genocideless conduct */
            if (*buf == '\033' || !strcmpi(buf, "none")
                || !strcmpi(buf, "nothing")) {
                /* ... but no free pass if cursed */
                if (!(how & REALLY) && (ptr = rndmonst()) != 0)
                    break; /* remaining checks don't apply */

                return;
            }

            mndx = name_to_mon(buf);
            if (mndx == NON_PM || (mvitals[mndx].mvflags & G_GENOD)) {
#if 0 /*KR: 원본*/
                pline("Such creatures %s exist in this world.",
                      (mndx == NON_PM) ? "do not" : "no longer");
#else /*KR: KRNethack 맞춤 번역 */
                pline("그런 생물은 이 세상에 %s.",
                      (mndx == NON_PM) ? "존재하지 않는다"
                                       : "더 이상 존재하지 않는다");
#endif
                continue;
            }
            ptr = &mons[mndx];
            /* Although "genus" is Latin for race, the hero benefits
             * from both race and role; thus genocide affects either.
             */
            if (Your_Own_Role(mndx) || Your_Own_Race(mndx)) {
                killplayer++;
                break;
            }
            if (is_human(ptr))
                adjalign(-sgn(u.ualign.type));
            if (is_demon(ptr))
                adjalign(sgn(u.ualign.type));

            if (!(ptr->geno & G_GENO)) {
                if (!Deaf) {
                    /* FIXME: unconditional "caverns" will be silly in some
                     * circumstances.  Who's speaking?  Divine pronouncements
                     * aren't supposed to be hampered by deafness....
                     */
#if 0 /*KR: 원본*/
                    if (flags.verbose)
                        pline("A thunderous voice booms through the caverns:");
                    verbalize("No, mortal!  That will not be done.");
#else /*KR: KRNethack 맞춤 번역 */
                    if (flags.verbose)
                        pline("우레와 같은 목소리가 동굴을 울린다:");
                    verbalize(
                        "안 된다, 필멸자여! 그 일은 이루어지지 않을 것이다.");
#endif
                }
                continue;
            }
            /* KMH -- Unchanging prevents rehumanization */
            if (Unchanging && ptr == youmonst.data)
                killplayer++;
            break;
        }
        mndx = monsndx(ptr); /* needed for the 'no free pass' cases */
    }

#if 0 /*KR: 원본*/
    which = "all ";
    if (Hallucination) {
        if (Upolyd)
            Strcpy(buf, youmonst.data->mname);
        else {
            Strcpy(buf, (flags.female && urole.name.f) ? urole.name.f
                                                       : urole.name.m);
            buf[0] = lowc(buf[0]);
        }
    } else {
        Strcpy(buf, ptr->mname); /* make sure we have standard singular */
        if ((ptr->geno & G_UNIQ) && ptr != &mons[PM_HIGH_PRIEST])
            which = !type_is_pname(ptr) ? "the " : "";
    }
    if (how & REALLY) {
        /* setting no-corpse affects wishing and random tin generation */
        mvitals[mndx].mvflags |= (G_GENOD | G_NOCORPSE);
        pline("Wiped out %s%s.", which,
              (*which != 'a') ? buf : makeplural(buf));
#else /*KR: KRNethack 맞춤 번역 */
    which = "모든 ";
    if (Hallucination) {
        if (Upolyd)
            Strcpy(buf, youmonst.data->mname);
        else {
            Strcpy(buf, (flags.female && urole.name.f) ? urole.name.f
                                                       : urole.name.m);
            buf[0] = lowc(buf[0]);
        }
    } else {
        Strcpy(buf, ptr->mname); /* make sure we have standard singular */
        if ((ptr->geno & G_UNIQ) && ptr != &mons[PM_HIGH_PRIEST])
            which = !type_is_pname(ptr) ? "그 " : "";
    }
    if (how & REALLY) {
        /* setting no-corpse affects wishing and random tin generation */
        mvitals[mndx].mvflags |= (G_GENOD | G_NOCORPSE);
        /* 한국어에서는 복수형 변환이 필요 없으며, which를 번역하여 붙임 */
        pline("%s%s 싹 쓸어버렸다.", which, append_josa(buf, "을"));
#endif

        if (killplayer) {
            /* might need to wipe out dual role */
            if (urole.femalenum != NON_PM && mndx == urole.malenum)
                mvitals[urole.femalenum].mvflags |= (G_GENOD | G_NOCORPSE);
            if (urole.femalenum != NON_PM && mndx == urole.femalenum)
                mvitals[urole.malenum].mvflags |= (G_GENOD | G_NOCORPSE);
            if (urace.femalenum != NON_PM && mndx == urace.malenum)
                mvitals[urace.femalenum].mvflags |= (G_GENOD | G_NOCORPSE);
            if (urace.femalenum != NON_PM && mndx == urace.femalenum)
                mvitals[urace.malenum].mvflags |= (G_GENOD | G_NOCORPSE);

            u.uhp = -1;
            if (how & PLAYER) {
                killer.format = KILLED_BY;
                /*KR Strcpy(killer.name, "genocidal confusion"); */
                Strcpy(killer.name, "학살적 혼란");
            } else if (how & ONTHRONE) {
                /* player selected while on a throne */
                killer.format = KILLED_BY_AN;
                /*KR Strcpy(killer.name, "imperious order"); */
                Strcpy(killer.name, "거만한 명령");
            } else { /* selected player deliberately, not confused */
                killer.format = KILLED_BY_AN;
                /*KR Strcpy(killer.name, "scroll of genocide"); */
                Strcpy(killer.name, "학살의 두루마리");
            }

            /* Polymorphed characters will die as soon as they're rehumanized.
             */
            /* KMH -- Unchanging prevents rehumanization */
            if (Upolyd && ptr != youmonst.data) {
                delayed_killer(POLYMORPH, killer.format, killer.name);
                /*KR You_feel("%s inside.", udeadinside()); */
                You("내면이 %s.", udeadinside());
            } else
                done(GENOCIDED);
        } else if (ptr == youmonst.data) {
            rehumanize();
        }
        reset_rndmonst(mndx);
        kill_genocided_monsters();
        update_inventory(); /* in case identified eggs were affected */
    } else {
        int cnt = 0, census = monster_census(FALSE);

        if (!(mons[mndx].geno & G_UNIQ)
            && !(mvitals[mndx].mvflags & (G_GENOD | G_EXTINCT)))
            for (i = rn1(3, 4); i > 0; i--) {
                if (!makemon(ptr, u.ux, u.uy, NO_MINVENT))
                    break; /* couldn't make one */
                ++cnt;
                if (mvitals[mndx].mvflags & G_EXTINCT)
                    break; /* just made last one */
            }
#if 0 /*KR: 원본*/
        if (cnt) {
            /* accumulated 'cnt' doesn't take groups into account;
               assume bringing in new mon(s) didn't remove any old ones */
            cnt = monster_census(FALSE) - census;
            pline("Sent in %s%s.", (cnt > 1) ? "some " : "",
                  (cnt > 1) ? makeplural(buf) : an(buf));
        } else
#else /*KR: KRNethack 맞춤 번역 */
        if (cnt) {
            /* accumulated 'cnt' doesn't take groups into account;
               assume bringing in new mon(s) didn't remove any old ones */
            cnt = monster_census(FALSE) - census;
            pline("%s %d마리를 들여보냈다.", buf, cnt);
        } else
#endif
            pline1(nothing_happens);
    }
}

void punish(sobj) struct obj *sobj;
{
    struct obj *reuse_ball =
        (sobj && sobj->otyp == HEAVY_IRON_BALL) ? sobj : (struct obj *) 0;

    /* KMH -- Punishment is still okay when you are riding */
#if 0 /*KR: 원본*/
    if (!reuse_ball)
        You("are being punished for your misbehavior!");
    if (Punished) {
        Your("iron ball gets heavier.");
        uball->owt += IRON_BALL_W_INCR * (1 + sobj->cursed);
        return;
    }
    if (amorphous(youmonst.data) || is_whirly(youmonst.data)
        || unsolid(youmonst.data)) {
        if (!reuse_ball) {
            pline("A ball and chain appears, then falls away.");
            dropy(mkobj(BALL_CLASS, TRUE));
        } else {
            dropy(reuse_ball);
        }
        return;
    }
#else /*KR: KRNethack 맞춤 번역 */
    if (!reuse_ball)
        You("비행을 저질러 벌을 받게 되었다!");
    if (Punished) {
        Your("철구가 더 무거워졌다.");
        uball->owt += IRON_BALL_W_INCR * (1 + sobj->cursed);
        return;
    }
    if (amorphous(youmonst.data) || is_whirly(youmonst.data)
        || unsolid(youmonst.data)) {
        if (!reuse_ball) {
            pline("쇠사슬과 철구가 나타났다가, 이내 떨어져 나갔다.");
            dropy(mkobj(BALL_CLASS, TRUE));
        } else {
            dropy(reuse_ball);
        }
        return;
    }
#endif
    setworn(mkobj(CHAIN_CLASS, TRUE), W_CHAIN);
    if (!reuse_ball)
        setworn(mkobj(BALL_CLASS, TRUE), W_BALL);
    else
        setworn(reuse_ball, W_BALL);
    uball->spe = 1; /* special ball (see save) */

    /*
     * Place ball & chain if not swallowed.  If swallowed, the ball & chain
     * variables will be set at the next call to placebc().
     */
    if (!u.uswallow) {
        placebc();
        if (Blind)
            set_bc(1);      /* set up ball and chain variables */
        newsym(u.ux, u.uy); /* see ball&chain if can't see self */
    }
}

/* remove the ball and chain */
void
unpunish()
{
    struct obj *savechain = uchain;

    /* chain goes away */
    obj_extract_self(uchain);
    newsym(uchain->ox, uchain->oy);
    setworn((struct obj *) 0, W_CHAIN); /* sets 'uchain' to Null */
    dealloc_obj(savechain);
    /* ball persists */
    uball->spe = 0;
    setworn((struct obj *) 0, W_BALL); /* sets 'uball' to Null */
}

/* some creatures have special data structures that only make sense in their
 * normal locations -- if the player tries to create one elsewhere, or to
 * revive one, the disoriented creature becomes a zombie
 */
boolean
cant_revive(mtype, revival, from_obj)
int *mtype;
boolean revival;
struct obj *from_obj;
{
    /* SHOPKEEPERS can be revived now */
    if (*mtype == PM_GUARD || (*mtype == PM_SHOPKEEPER && !revival)
        || *mtype == PM_HIGH_PRIEST || *mtype == PM_ALIGNED_PRIEST
        || *mtype == PM_ANGEL) {
        *mtype = PM_HUMAN_ZOMBIE;
        return TRUE;
    } else if (*mtype == PM_LONG_WORM_TAIL) { /* for create_particular() */
        *mtype = PM_LONG_WORM;
        return TRUE;
    } else if (unique_corpstat(&mons[*mtype])
               && (!from_obj || !has_omonst(from_obj))) {
        /* unique corpses (from bones or wizard mode wish) or
           statues (bones or any wish) end up as shapechangers */
        *mtype = PM_DOPPELGANGER;
        return TRUE;
    }
    return FALSE;
}

struct _create_particular_data {
    int which;
    int fem;
    char monclass;
    boolean randmonst;
    boolean maketame, makepeaceful, makehostile;
    boolean sleeping, saddled, invisible, hidden;
};

boolean
create_particular_parse(str, d)
char *str;
struct _create_particular_data *d;
{
    char *bufp = str;
    char *tmpp;

    d->monclass = MAXMCLASSES;
    d->which = urole.malenum; /* an arbitrary index into mons[] */
    d->fem = -1;              /* gender not specified */
    d->randmonst = FALSE;
    d->maketame = d->makepeaceful = d->makehostile = FALSE;
    d->sleeping = d->saddled = d->invisible = d->hidden = FALSE;

    if ((tmpp = strstri(bufp, "saddled ")) != 0) {
        d->saddled = TRUE;
        (void) memset(tmpp, ' ', sizeof "saddled " - 1);
    }
    if ((tmpp = strstri(bufp, "sleeping ")) != 0) {
        d->sleeping = TRUE;
        (void) memset(tmpp, ' ', sizeof "sleeping " - 1);
    }
    if ((tmpp = strstri(bufp, "invisible ")) != 0) {
        d->invisible = TRUE;
        (void) memset(tmpp, ' ', sizeof "invisible " - 1);
    }
    if ((tmpp = strstri(bufp, "hidden ")) != 0) {
        d->hidden = TRUE;
        (void) memset(tmpp, ' ', sizeof "hidden " - 1);
    }
    /* check "female" before "male" to avoid false hit mid-word */
    if ((tmpp = strstri(bufp, "female ")) != 0) {
        d->fem = 1;
        (void) memset(tmpp, ' ', sizeof "female " - 1);
    }
    if ((tmpp = strstri(bufp, "male ")) != 0) {
        d->fem = 0;
        (void) memset(tmpp, ' ', sizeof "male " - 1);
    }
    bufp = mungspaces(bufp); /* after potential memset(' ') */
    /* allow the initial disposition to be specified */
#if 0 /*KR: 원본*/
    if (!strncmpi(bufp, "tame ", 5)) {
        bufp += 5;
#else /*KR: KRNethack 맞춤 번역 */
    if (!strncmp(bufp, "길들여진 ", strlen("길들여진 "))) {
        bufp += strlen("길들여진 ");
#endif
        d->maketame = TRUE;
#if 0 /*KR: 원본*/
    } else if (!strncmpi(bufp, "peaceful ", 9)) {
        bufp += 9;
#else /*KR: KRNethack 맞춤 번역 */
    } else if (!strncmp(bufp, "우호적인 ", strlen("우호적인 "))) {
        bufp += strlen("우호적인 ");
#endif
        d->makepeaceful = TRUE;
#if 0 /*KR: 원본*/
    } else if (!strncmpi(bufp, "hostile ", 8)) {
        bufp += 8;
#else /*KR: KRNethack 맞춤 번역 */
    } else if (!strncmp(bufp, "적대적인 ", strlen("적대적인 "))) {
        bufp += strlen("적대적인 ");
#endif
        d->makehostile = TRUE;
    }
    /* decide whether a valid monster was chosen */
#if 0 /*KR: 원본*/
    if (wizard && (!strcmp(bufp, "*") || !strcmp(bufp, "random"))) {
#else /*KR: KRNethack 맞춤 번역 */
    if (wizard && (!strcmp(bufp, "*") || !strcmp(bufp, "랜덤"))) {
#endif
        d->randmonst = TRUE;
        return TRUE;
    }
    d->which = name_to_mon(bufp);
    if (d->which >= LOW_PM)
        return TRUE; /* got one */
    d->monclass = name_to_monclass(bufp, &d->which);

    if (d->which >= LOW_PM) {
        d->monclass = MAXMCLASSES; /* matters below */
        return TRUE;
    } else if (d->monclass == S_invisible) { /* not an actual monster class */
        d->which = PM_STALKER;
        d->monclass = MAXMCLASSES;
        return TRUE;
    } else if (d->monclass == S_WORM_TAIL) { /* empty monster class */
        d->which = PM_LONG_WORM;
        d->monclass = MAXMCLASSES;
        return TRUE;
    } else if (d->monclass > 0) {
        d->which = urole.malenum; /* reset from NON_PM */
        return TRUE;
    }
    return FALSE;
}

boolean
create_particular_creation(d)
struct _create_particular_data *d;
{
    struct permonst *whichpm = NULL;
    int i, mx, my, firstchoice = NON_PM;
    struct monst *mtmp;
    boolean madeany = FALSE;

    if (!d->randmonst) {
        firstchoice = d->which;
        if (cant_revive(&d->which, FALSE, (struct obj *) 0)
            && firstchoice != PM_LONG_WORM_TAIL) {
            /* wizard mode can override handling of special monsters */
            char buf[BUFSZ];

#if 0 /*KR: 원본*/
            Sprintf(buf, "Creating %s instead; force %s?",
                    mons[d->which].mname, mons[firstchoice].mname);
#else /*KR: KRNethack 맞춤 번역 */
            Sprintf(buf, "대신 %s 생성합니다; %s 강제하겠습니까?",
                    mons[d->which].mname,
                    append_josa(mons[firstchoice].mname, "을"));
#endif
            if (yn(buf) == 'y')
                d->which = firstchoice;
        }
        whichpm = &mons[d->which];
    }
    for (i = 0; i <= multi; i++) {
        if (d->monclass != MAXMCLASSES)
            whichpm = mkclass(d->monclass, 0);
        else if (d->randmonst)
            whichpm = rndmonst();
        mtmp = makemon(whichpm, u.ux, u.uy, NO_MM_FLAGS);
        if (!mtmp) {
            /* quit trying if creation failed and is going to repeat */
            if (d->monclass == MAXMCLASSES && !d->randmonst)
                break;
            /* otherwise try again */
            continue;
        }
        mx = mtmp->mx, my = mtmp->my;
        /* 'is_FOO()' ought to be called 'always_FOO()' */
        if (d->fem != -1 && !is_male(mtmp->data) && !is_female(mtmp->data))
            mtmp->female = d->fem; /* ignored for is_neuter() */
        if (d->maketame) {
            (void) tamedog(mtmp, (struct obj *) 0);
        } else if (d->makepeaceful || d->makehostile) {
            mtmp->mtame = 0; /* sanity precaution */
            mtmp->mpeaceful = d->makepeaceful ? 1 : 0;
            set_malign(mtmp);
        }
        if (d->saddled && can_saddle(mtmp) && !which_armor(mtmp, W_SADDLE)) {
            struct obj *otmp = mksobj(SADDLE, TRUE, FALSE);

            put_saddle_on_mon(otmp, mtmp);
        }
        if (d->invisible) {
            mon_set_minvis(mtmp);
            if (does_block(mx, my, &levl[mx][my]))
                block_point(mx, my);
            else
                unblock_point(mx, my);
        }
        if (d->hidden
            && ((is_hider(mtmp->data) && mtmp->data->mlet != S_MIMIC)
                || (hides_under(mtmp->data) && OBJ_AT(mx, my))
                || (mtmp->data->mlet == S_EEL && is_pool(mx, my))))
            mtmp->mundetected = 1;
        if (d->sleeping)
            mtmp->msleeping = 1;
        /* iff asking for 'hidden', show locaton of every created monster
           that can't be seen--whether that's due to successfully hiding
           or vision issues (line-of-sight, invisibility, blindness) */
        if (d->hidden && !canspotmon(mtmp)) {
            int count = couldsee(mx, my) ? 8 : 4;
            char saveviz = viz_array[my][mx];

            if (!flags.sparkle)
                count /= 2;
            viz_array[my][mx] |= (IN_SIGHT | COULD_SEE);
            flash_glyph_at(mx, my, mon_to_glyph(mtmp, newsym_rn2), count);
            viz_array[my][mx] = saveviz;
            newsym(mx, my);
        }
        madeany = TRUE;
        /* in case we got a doppelganger instead of what was asked
           for, make it start out looking like what was asked for */
        if (mtmp->cham != NON_PM && firstchoice != NON_PM
            && mtmp->cham != firstchoice)
            (void) newcham(mtmp, &mons[firstchoice], FALSE, FALSE);
    }
    return madeany;
}

/*
 * Make a new monster with the type controlled by the user.
 *
 * Note:  when creating a monster by class letter, specifying the
 * "strange object" (']') symbol produces a random monster rather
 * than a mimic.  This behavior quirk is useful so don't "fix" it
 * (use 'm'--or "mimic"--to create a random mimic).
 *
 * Used in wizard mode only (for ^G command and for scroll or spell
 * of create monster).  Once upon a time, an earlier incarnation of
 * this code was also used for the scroll/spell in explore mode.
 */
boolean
create_particular()
{
    char buf[BUFSZ] = DUMMY, *bufp;
    int  tryct = 5;
    struct _create_particular_data d;

#if 0 /*KR: 원본*/
    do {
        getlin("Create what kind of monster? [type the name or symbol]", buf);
        bufp = mungspaces(buf);
        if (*bufp == '\033')
            return FALSE;

        if (create_particular_parse(bufp, &d))
            break;

        /* no good; try again... */
        pline("I've never heard of such monsters.");
    } while (--tryct > 0);
#else /*KR: KRNethack 맞춤 번역 */
    do {
        getlin("어떤 몬스터를 생성하겠습니까? [이름이나 기호를 입력하세요]",
               buf);
        bufp = mungspaces(buf);
        if (*bufp == '\033')
            return FALSE;

        if (create_particular_parse(bufp, &d))
            break;

        /* no good; try again... */
        pline("그런 몬스터는 들어본 적이 없다.");
    } while (--tryct > 0);
#endif

    if (!tryct)
        pline1(thats_enough_tries);
    else
        return create_particular_creation(&d);

    return FALSE;
}

/*read.c*/
