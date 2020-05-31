/* NetHack 3.6	role.c	$NHDT-Date: 1547086250 2019/01/10 02:10:50 $  $NHDT-Branch: NetHack-3.6.2-beta01 $:$NHDT-Revision: 1.56 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985-1999. */
/*-Copyright (c) Robert Patrick Rankin, 2012. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

/*** Table of all roles ***/
/* According to AD&D, HD for some classes (ex. Wizard) should be smaller
 * (4-sided for wizards).  But this is not AD&D, and using the AD&D
 * rule here produces an unplayable character.  Thus I have used a minimum
 * of an 10-sided hit die for everything.  Another AD&D change: wizards get
 * a minimum strength of 4 since without one you can't teleport or cast
 * spells. --KAA
 *
 * As the wizard has been updated (wizard patch 5 jun '96) their HD can be
 * brought closer into line with AD&D. This forces wizards to use magic more
 * and distance themselves from their attackers. --LSZ
 *
 * With the introduction of races, some hit points and energy
 * has been reallocated for each race.  The values assigned
 * to the roles has been reduced by the amount allocated to
 * humans.  --KMH
 *
 * God names use a leading underscore to flag goddesses.
 */
const struct Role roles[] = {
#if 0 /*KR:T*/
    { { "Archeologist", 0 },
      { { "Digger", 0 },
        { "Field Worker", 0 },
        { "Investigator", 0 },
        { "Exhumer", 0 },
        { "Excavator", 0 },
        { "Spelunker", 0 },
        { "Speleologist", 0 },
        { "Collector", 0 },
        { "Curator", 0 } },
      "Quetzalcoatl", "Camaxtli", "Huhetotl", /* Central American */
#else
    { { "고고학자", 0 },
      { { "광부", 0 },
        { "노동자", 0 },
        { "조사관", 0 },
        { "채굴자", 0 },
        { "발굴자", 0 },
        { "동굴탐험가", 0 },
        { "동굴학자", 0 },
        { "수집가", 0 },
        { "학예사", 0 } },
      "케찰코아틀", "카막스틀리", "우에우에테오틀", /* Central American */
#endif
      "Arc",
#if 0 /*KR:T*/
      "the College of Archeology",
      "the Tomb of the Toltec Kings",
#else
      "고고학 대학",
      "톨텍 왕가의 무덤",
#endif
      PM_ARCHEOLOGIST,
      NON_PM,
      NON_PM,
      PM_LORD_CARNARVON,
      PM_STUDENT,
      PM_MINION_OF_HUHETOTL,
      NON_PM,
      PM_HUMAN_MUMMY,
      S_SNAKE,
      S_MUMMY,
      ART_ORB_OF_DETECTION,
      MH_HUMAN | MH_DWARF | MH_GNOME | ROLE_MALE | ROLE_FEMALE | ROLE_LAWFUL
          | ROLE_NEUTRAL,
      /* Str Int Wis Dex Con Cha */
      { 7, 10, 10, 7, 7, 7 },
      { 20, 20, 20, 10, 20, 10 },
      /* Init   Lower  Higher */
      { 11, 0, 0, 8, 1, 0 }, /* Hit points */
      { 1, 0, 0, 1, 0, 1 },
      14, /* Energy */
      10,
      5,
      0,
      2,
      10,
      A_INT,
      SPE_MAGIC_MAPPING,
      -4 },
#if 0 /*KR:T*/
    { { "Barbarian", 0 },
      { { "Plunderer", "Plunderess" },
        { "Pillager", 0 },
        { "Bandit", 0 },
        { "Brigand", 0 },
        { "Raider", 0 },
        { "Reaver", 0 },
        { "Slayer", 0 },
        { "Chieftain", "Chieftainess" },
        { "Conqueror", "Conqueress" } },
      "Mitra", "Crom", "Set", /* Hyborian */
#else
    { { "야만인", 0 },
      { { "약탈자", 0 },
        { "탈취자", 0 },
        { "무법자", 0 },
        { "산적", 0 },
        { "침략자", 0 },
        { "약탈자", 0 },
        { "살해자", 0 },
        { "수령", 0 },
        { "정복자", 0 } },
      "미트라", "크롬", "세트", /* Hyborian */
#endif
      "Bar",
#if 0 /*KR:T*/
      "the Camp of the Duali Tribe",
      "the Duali Oasis",
#else
      "Duali 족의 진영",
      "Duali 족의 오아시스",
#endif
      PM_BARBARIAN,
      NON_PM,
      NON_PM,
      PM_PELIAS,
      PM_CHIEFTAIN,
      PM_THOTH_AMON,
      PM_OGRE,
      PM_TROLL,
      S_OGRE,
      S_TROLL,
      ART_HEART_OF_AHRIMAN,
      MH_HUMAN | MH_ORC | ROLE_MALE | ROLE_FEMALE | ROLE_NEUTRAL
          | ROLE_CHAOTIC,
      /* Str Int Wis Dex Con Cha */
      { 16, 7, 7, 15, 16, 6 },
      { 30, 6, 7, 20, 30, 7 },
      /* Init   Lower  Higher */
      { 14, 0, 0, 10, 2, 0 }, /* Hit points */
      { 1, 0, 0, 1, 0, 1 },
      10, /* Energy */
      10,
      14,
      0,
      0,
      8,
      A_INT,
      SPE_HASTE_SELF,
      -4 },
#if 0 /*KR:T*/
    { { "Caveman", "Cavewoman" },
      { { "Troglodyte", 0 },
        { "Aborigine", 0 },
        { "Wanderer", 0 },
        { "Vagrant", 0 },
        { "Wayfarer", 0 },
        { "Roamer", 0 },
        { "Nomad", 0 },
        { "Rover", 0 },
        { "Pioneer", 0 } },
      "Anu", "_Ishtar", "Anshar", /* Babylonian */
#else
    { { "원시인", 0 },
      { { "혈거인", 0 },
        { "원주민", 0 },
        { "방랑자", 0 },
        { "부랑자", 0 },
        { "여행자", 0 },
        { "배회자", 0 },
        { "유목민", 0 },
        { "표류자", 0 },
        { "개척자", 0 } },
      "아누", "_이슈타르", "안샤르", /* 바빌로니아인 */
#endif
      "Cav",
#if 0 /*KR:T*/
      "the Caves of the Ancestors",
      "the Dragon's Lair",
#else
      "조상의 동굴",
      "용의 은신처",
#endif
      PM_CAVEMAN,
      PM_CAVEWOMAN,
      PM_LITTLE_DOG,
      PM_SHAMAN_KARNOV,
      PM_NEANDERTHAL,
      PM_CHROMATIC_DRAGON,
      PM_BUGBEAR,
      PM_HILL_GIANT,
      S_HUMANOID,
      S_GIANT,
      ART_SCEPTRE_OF_MIGHT,
      MH_HUMAN | MH_DWARF | MH_GNOME | ROLE_MALE | ROLE_FEMALE | ROLE_LAWFUL
          | ROLE_NEUTRAL,
      /* Str Int Wis Dex Con Cha */
      { 10, 7, 7, 7, 8, 6 },
      { 30, 6, 7, 20, 30, 7 },
      /* Init   Lower  Higher */
      { 14, 0, 0, 8, 2, 0 }, /* Hit points */
      { 1, 0, 0, 1, 0, 1 },
      10, /* Energy */
      0,
      12,
      0,
      1,
      8,
      A_INT,
      SPE_DIG,
      -4 },
#if 0 /*KR:T*/
    { { "Healer", 0 },
      { { "Rhizotomist", 0 },
        { "Empiric", 0 },
        { "Embalmer", 0 },
        { "Dresser", 0 },
        { "Medicus ossium", "Medica ossium" },
        { "Herbalist", 0 },
        { "Magister", "Magistra" },
        { "Physician", 0 },
        { "Chirurgeon", 0 } },
      "_Athena", "Hermes", "Poseidon", /* Greek */
#else
    { { "치유사", 0 },
      { { "약초 수집가", 0 },
        { "돌팔이 의사", 0 },
        { "장례지도사", 0 },
        { "붕대 담당자", 0 },
        { "정형외과 의사", 0 },
        { "한의사", 0 },
        { "의학석사", 0 },
        { "내과 의사", 0 },
        { "외과 의사", 0 } },
      "_아테나", "헤르메스", "포세이돈", /* 그리스인 */
#endif
      "Hea",
#if 0 /*KR:T*/
      "the Temple of Epidaurus",
      "the Temple of Coeus",
#else
      "에피다우로스의 사원",
      "코이오스의 사원",
#endif
      PM_HEALER,
      NON_PM,
      NON_PM,
      PM_HIPPOCRATES,
      PM_ATTENDANT,
      PM_CYCLOPS,
      PM_GIANT_RAT,
      PM_SNAKE,
      S_RODENT,
      S_YETI,
      ART_STAFF_OF_AESCULAPIUS,
      MH_HUMAN | MH_GNOME | ROLE_MALE | ROLE_FEMALE | ROLE_NEUTRAL,
      /* Str Int Wis Dex Con Cha */
      { 7, 7, 13, 7, 11, 16 },
      { 15, 20, 20, 15, 25, 5 },
      /* Init   Lower  Higher */
      { 11, 0, 0, 8, 1, 0 }, /* Hit points */
      { 1, 4, 0, 1, 0, 2 },
      20, /* Energy */
      10,
      3,
      -3,
      2,
      10,
      A_WIS,
      SPE_CURE_SICKNESS,
      -4 },
#if 0 /*KR:T*/
    { { "Knight", 0 },
      { { "Gallant", 0 },
        { "Esquire", 0 },
        { "Bachelor", 0 },
        { "Sergeant", 0 },
        { "Knight", 0 },
        { "Banneret", 0 },
        { "Chevalier", "Chevaliere" },
        { "Seignieur", "Dame" },
        { "Paladin", 0 } },
      "Lugh", "_Brigit", "Manannan Mac Lir", /* Celtic */
#else
    { { "기사", 0 },
      { { "사나이", 0 },
        { "향사", 0 },
        { "청년 기사", 0 },
        { "경사", 0 },
        { "기사", 0 },
        { "기령 기사", 0 },
        { "훈작사", 0 },
        { "귀족", 0 },
        { "궁정 기사", 0 } },
      "루", "_브리지트", "마난난 막 리르", /* 켈트족 */
#endif
      "Kni",
#if 0 /*KR:T*/
      "Camelot Castle",
      "the Isle of Glass",
#else
      "카멜롯 성",
      "유리의 섬",
#endif
      PM_KNIGHT,
      NON_PM,
      PM_PONY,
      PM_KING_ARTHUR,
      PM_PAGE,
      PM_IXOTH,
      PM_QUASIT,
      PM_OCHRE_JELLY,
      S_IMP,
      S_JELLY,
      ART_MAGIC_MIRROR_OF_MERLIN,
      MH_HUMAN | ROLE_MALE | ROLE_FEMALE | ROLE_LAWFUL,
      /* Str Int Wis Dex Con Cha */
      { 13, 7, 14, 8, 10, 17 },
      { 30, 15, 15, 10, 20, 10 },
      /* Init   Lower  Higher */
      { 14, 0, 0, 8, 2, 0 }, /* Hit points */
      { 1, 4, 0, 1, 0, 2 },
      10, /* Energy */
      10,
      8,
      -2,
      0,
      9,
      A_WIS,
      SPE_TURN_UNDEAD,
      -4 },
#if 0 /*KR:T*/
    { { "Monk", 0 },
      { { "Candidate", 0 },
        { "Novice", 0 },
        { "Initiate", 0 },
        { "Student of Stones", 0 },
        { "Student of Waters", 0 },
        { "Student of Metals", 0 },
        { "Student of Winds", 0 },
        { "Student of Fire", 0 },
        { "Master", 0 } },
      "Shan Lai Ching", "Chih Sung-tzu", "Huan Ti", /* Chinese */
#else /*연구[학문]을 (좋아)하는 사람, 연구가, 탐구자, 학자.학도,학구.*/
    { { "수도승", 0 },
      { { "지원자", 0 },
        { "수련 수사", 0 },
        { "전수자", 0 },
        { "흙의 탐구자", 0 },
        { "물의 탐구자", 0 },
        { "철의 탐구자", 0 },
        { "공기의 탐구자", 0 },
        { "불의 탐구자", 0 },
        { "달인", 0 } },
      "산해경", "적송자", "황제", /* 중국어 */
#endif
      "Mon",
#if 0 /*KR:T*/
      "the Monastery of Chan-Sune",
      "the Monastery of the Earth-Lord",
#else
      "참선의 수도원",
      "지왕의 수도원",
#endif
      PM_MONK,
      NON_PM,
      NON_PM,
      PM_GRAND_MASTER,
      PM_ABBOT,
      PM_MASTER_KAEN,
      PM_EARTH_ELEMENTAL,
      PM_XORN,
      S_ELEMENTAL,
      S_XORN,
      ART_EYES_OF_THE_OVERWORLD,
      MH_HUMAN | ROLE_MALE | ROLE_FEMALE | ROLE_LAWFUL | ROLE_NEUTRAL
          | ROLE_CHAOTIC,
      /* Str Int Wis Dex Con Cha */
      { 10, 7, 8, 8, 7, 7 },
      { 25, 10, 20, 20, 15, 10 },
      /* Init   Lower  Higher */
      { 12, 0, 0, 8, 1, 0 }, /* Hit points */
      { 2, 2, 0, 2, 0, 2 },
      10, /* Energy */
      10,
      8,
      -2,
      2,
      20,
      A_WIS,
      SPE_RESTORE_ABILITY,
      -4 },
#if 0
    { { "Priest", "Priestess" },
      { { "Aspirant", 0 },
        { "Acolyte", 0 },
        { "Adept", 0 },
        { "Priest", "Priestess" },
        { "Curate", 0 },
        { "Canon", "Canoness" },
        { "Lama", 0 },
        { "Patriarch", "Matriarch" },
        { "High Priest", "High Priestess" } },
      0, 0, 0, /* deities from a randomly chosen other role will be used */
#else
    { { "사제", "여사제" },
      { { "지망생", 0 },
        { "복사", 0 },
        { "시제", 0 },
        { "사제", "여사제" },
        { "조임 신부", 0 },
        { "참사회원", 0 },
        { "라마승", 0 },
        { "대주교", 0 },
        { "대사제", 0 } },
      0, 0, 0, /* 무작위로 선택된 다른 직업에서의 신이 사용될 것입니다 */
#endif
      "Pri",
#if 0 /*KR:T*/
      "the Great Temple",
      "the Temple of Nalzok",
#else
      "대사원",
      "나르조크의 사원",
#endif
      PM_PRIEST,
      PM_PRIESTESS,
      NON_PM,
      PM_ARCH_PRIEST,
      PM_ACOLYTE,
      PM_NALZOK,
      PM_HUMAN_ZOMBIE,
      PM_WRAITH,
      S_ZOMBIE,
      S_WRAITH,
      ART_MITRE_OF_HOLINESS,
      MH_HUMAN | MH_ELF | ROLE_MALE | ROLE_FEMALE | ROLE_LAWFUL | ROLE_NEUTRAL
          | ROLE_CHAOTIC,
      /* Str Int Wis Dex Con Cha */
      { 7, 7, 10, 7, 7, 7 },
      { 15, 10, 30, 15, 20, 10 },
      /* Init   Lower  Higher */
      { 12, 0, 0, 8, 1, 0 }, /* Hit points */
      { 4, 3, 0, 2, 0, 2 },
      10, /* Energy */
      0,
      3,
      -2,
      2,
      10,
      A_WIS,
      SPE_REMOVE_CURSE,
      -4 },
    /* Note:  Rogue precedes Ranger so that use of `-R' on the command line
       retains its traditional meaning. */
#if 0 /*KR:T*/
    { { "Rogue", 0 },
      { { "Footpad", 0 },
        { "Cutpurse", 0 },
        { "Rogue", 0 },
        { "Pilferer", 0 },
        { "Robber", 0 },
        { "Burglar", 0 },
        { "Filcher", 0 },
        { "Magsman", "Magswoman" },
        { "Thief", 0 } },
      "Issek", "Mog", "Kos", /* Nehwon */
#else
    { { "도적", 0 },
      { { "노상강도", 0 },
        { "소매치기", 0 },
        { "도적", 0 },
        { "강도", 0 },
        { "날강도", 0 },
        { "도둑", 0 },
        { "좀도둑", 0 },
        { "사기꾼", 0 },
        { "절도범", 0 } },
      "이섹", "모그", "코스", /* Nehwon(네원), 프리츠 라이버, AD&D. */
#endif
      "Rog",
#if 0 /*KR:T*/
      "the Thieves' Guild Hall",
      "the Assassins' Guild Hall",
#else /*중세 시대 기능인들의 조합을 일컫는 고유명사로 처리함.*/
      "도둑 길드 회관",
      "암살자 길드 회관",
#endif
      PM_ROGUE,
      NON_PM,
      NON_PM,
      PM_MASTER_OF_THIEVES,
      PM_THUG,
      PM_MASTER_ASSASSIN,
      PM_LEPRECHAUN,
      PM_GUARDIAN_NAGA,
      S_NYMPH,
      S_NAGA,
      ART_MASTER_KEY_OF_THIEVERY,
      MH_HUMAN | MH_ORC | ROLE_MALE | ROLE_FEMALE | ROLE_CHAOTIC,
      /* Str Int Wis Dex Con Cha */
      { 7, 7, 7, 10, 7, 6 },
      { 20, 10, 10, 30, 20, 10 },
      /* Init   Lower  Higher */
      { 10, 0, 0, 8, 1, 0 }, /* Hit points */
      { 1, 0, 0, 1, 0, 1 },
      11, /* Energy */
      10,
      8,
      0,
      1,
      9,
      A_INT,
      SPE_DETECT_TREASURE,
      -4 },
#if 0 /*KR:T*/
    { { "Ranger", 0 },
      {
#if 0 /* OBSOLETE */
        {"Edhel",   "Elleth"},
        {"Edhel",   "Elleth"},         /* elf-maid */
        {"Ohtar",   "Ohtie"},          /* warrior */
        {"Kano",    "Kanie"},          /* commander (Q.) ['a] educated guess,
                                          until further research- SAC */
        {"Arandur"," Aranduriel"}, /* king's servant, minister (Q.) - guess */
        {"Hir",         "Hiril"},      /* lord, lady (S.) ['ir] */
        {"Aredhel",     "Arwen"},      /* noble elf, maiden (S.) */
        {"Ernil",       "Elentariel"}, /* prince (S.), elf-maiden (Q.) */
        {"Elentar",     "Elentari"},   /* Star-king, -queen (Q.) */
        "Solonor Thelandira", "Aerdrie Faenya", "Lolth", /* Elven */
#endif
        { "Tenderfoot", 0 },
        { "Lookout", 0 },
        { "Trailblazer", 0 },
        { "Reconnoiterer", "Reconnoiteress" },
        { "Scout", 0 },
        { "Arbalester", 0 }, /* One skilled at crossbows */
        { "Archer", 0 },
        { "Sharpshooter", 0 },
        { "Marksman", "Markswoman" } },
      "Mercury", "_Venus", "Mars", /* Roman/planets */
#else /*OBSOLETE 부분은 톨킨 세계관의 요정어를 사용한 모양.*/
    { { "레인저", 0 },
      { { "신참", 0 },
        { "파수꾼", 0 },
        { "개척자", 0 },
        { "정찰꾼", 0 },
        { "척후병", 0 },
        { "석궁 사수", 0 }, /* One skilled at crossbows */
        { "궁수", 0 },
        { "저격병", 0 },
        { "명사수", 0 } },
      "머큐리", "_비너스", "마르스", /* Roman/planets */
#endif
      "Ran",
#if 0 /*KR:T*/
      "Orion's camp",
      "the cave of the wumpus",
#else
      "오리온의 야영지",
      "웜푸스의 동굴",
#endif
      PM_RANGER,
      NON_PM,
      PM_LITTLE_DOG /* Orion & canis major */,
      PM_ORION,
      PM_HUNTER,
      PM_SCORPIUS,
      PM_FOREST_CENTAUR,
      PM_SCORPION,
      S_CENTAUR,
      S_SPIDER,
      ART_LONGBOW_OF_DIANA,
      MH_HUMAN | MH_ELF | MH_GNOME | MH_ORC | ROLE_MALE | ROLE_FEMALE
          | ROLE_NEUTRAL | ROLE_CHAOTIC,
      /* Str Int Wis Dex Con Cha */
      { 13, 13, 13, 9, 13, 7 },
      { 30, 10, 10, 20, 20, 10 },
      /* Init   Lower  Higher */
      { 13, 0, 0, 6, 1, 0 }, /* Hit points */
      { 1, 0, 0, 1, 0, 1 },
      12, /* Energy */
      10,
      9,
      2,
      1,
      10,
      A_INT,
      SPE_INVISIBILITY,
      -4 },
#if 0 /*KR:T*/
    { { "Samurai", 0 },
      { { "Hatamoto", 0 },       /* Banner Knight */
        { "Ronin", 0 },          /* no allegiance */
        { "Ninja", "Kunoichi" }, /* secret society */
        { "Joshu", 0 },          /* heads a castle */
        { "Ryoshu", 0 },         /* has a territory */
        { "Kokushu", 0 },        /* heads a province */
        { "Daimyo", 0 },         /* a samurai lord */
        { "Kuge", 0 },           /* Noble of the Court */
        { "Shogun", 0 } },       /* supreme commander, warlord */
      "_Amaterasu Omikami", "Raijin", "Susanowo", /* Japanese */
#else
    { { "사무라이", 0 },
      { { "하타모토", 0 },       /* Banner Knight */
        { "로닌", 0 },          /* no allegiance */
        { "닌자", "쿠노이치" }, /* secret society */
        { "조슈", 0 },          /* heads a castle */
        { "료슈", 0 },         /* has a territory */
        { "코쿠슈", 0 },        /* heads a province */
        { "다이묘", 0 },         /* a samurai lord */
        { "쿠게", 0 },           /* Noble of the Court */
        { "쇼군", 0 } },       /* supreme commander, warlord */
      "_아마테라스 오미카미", "라이진", "스사노오", /* Japanese */
#endif
      "Sam",
#if 0 /*KR:T*/
      "the Castle of the Taro Clan",
      "the Shogun's Castle",
#else
      "타로 일족의 성",
      "쇼군의 성",
#endif
      PM_SAMURAI,
      NON_PM,
      PM_LITTLE_DOG,
      PM_LORD_SATO,
      PM_ROSHI,
      PM_ASHIKAGA_TAKAUJI,
      PM_WOLF,
      PM_STALKER,
      S_DOG,
      S_ELEMENTAL,
      ART_TSURUGI_OF_MURAMASA,
      MH_HUMAN | ROLE_MALE | ROLE_FEMALE | ROLE_LAWFUL,
      /* Str Int Wis Dex Con Cha */
      { 10, 8, 7, 10, 17, 6 },
      { 30, 10, 8, 30, 14, 8 },
      /* Init   Lower  Higher */
      { 13, 0, 0, 8, 1, 0 }, /* Hit points */
      { 1, 0, 0, 1, 0, 1 },
      11, /* Energy */
      10,
      10,
      0,
      0,
      8,
      A_INT,
      SPE_CLAIRVOYANCE,
      -4 },
#if 0 /*KR:T*/
    { { "Tourist", 0 },
      { { "Rambler", 0 },
        { "Sightseer", 0 },
        { "Excursionist", 0 },
        { "Peregrinator", "Peregrinatrix" },
        { "Traveler", 0 },
        { "Journeyer", 0 },
        { "Voyager", 0 },
        { "Explorer", 0 },
        { "Adventurer", 0 } },
      "Blind Io", "_The Lady", "Offler", /* Discworld */
#else
    { { "관광객", 0 },
      { { "산책인", 0 },
        { "유람객", 0 },
        { "유람 여행자", 0 },
        { "편력자", 0 },
        { "나그네", 0 },
        { "여행가", 0 },
        { "항해자", 0 },
        { "탐험가", 0 },
        { "모험가", 0 } },
      "실명의 이오", "_<여신>", "오플러", /* 디스크월드 */
#endif
      "Tou",
#if 0 /*KR:T*/
      "Ankh-Morpork",
      "the Thieves' Guild Hall",
#else
      "앙크-모포크",
      "도둑 길드 회관",
#endif
      PM_TOURIST,
      NON_PM,
      NON_PM,
      PM_TWOFLOWER,
      PM_GUIDE,
      PM_MASTER_OF_THIEVES,
      PM_GIANT_SPIDER,
      PM_FOREST_CENTAUR,
      S_SPIDER,
      S_CENTAUR,
      ART_YENDORIAN_EXPRESS_CARD,
      MH_HUMAN | ROLE_MALE | ROLE_FEMALE | ROLE_NEUTRAL,
      /* Str Int Wis Dex Con Cha */
      { 7, 10, 6, 7, 7, 10 },
      { 15, 10, 10, 15, 30, 20 },
      /* Init   Lower  Higher */
      { 8, 0, 0, 8, 0, 0 }, /* Hit points */
      { 1, 0, 0, 1, 0, 1 },
      14, /* Energy */
      0,
      5,
      1,
      2,
      10,
      A_INT,
      SPE_CHARM_MONSTER,
      -4 },
#if 0 /*KR:T*/
    { { "Valkyrie", 0 },
      { { "Stripling", 0 },
        { "Skirmisher", 0 },
        { "Fighter", 0 },
        { "Man-at-arms", "Woman-at-arms" },
        { "Warrior", 0 },
        { "Swashbuckler", 0 },
        { "Hero", "Heroine" },
        { "Champion", 0 },
        { "Lord", "Lady" } },
      "Tyr", "Odin", "Loki", /* Norse */
#else
    { { "발키리", 0 },
      { { "견습", 0 },
        { "보병", 0 },
        { "전사", 0 },
        { "중기병", 0 },
        { "전투병", 0 },
        { "장사", 0 },
        { "영웅", 0 },
        { "투사", 0 },
        { "로드", "레이디" } },
      "티르", "오딘", "로키", /* Norse */
#endif
      "Val",
#if 0 /*KR:T*/
      "the Shrine of Destiny",
      "the cave of Surtur",
#else
      "운명의 전당",
      "수르트의 동굴",
#endif
      PM_VALKYRIE,
      NON_PM,
      NON_PM /*PM_WINTER_WOLF_CUB*/,
      PM_NORN,
      PM_WARRIOR,
      PM_LORD_SURTUR,
      PM_FIRE_ANT,
      PM_FIRE_GIANT,
      S_ANT,
      S_GIANT,
      ART_ORB_OF_FATE,
      MH_HUMAN | MH_DWARF | ROLE_FEMALE | ROLE_LAWFUL | ROLE_NEUTRAL,
      /* Str Int Wis Dex Con Cha */
      { 10, 7, 7, 7, 10, 7 },
      { 30, 6, 7, 20, 30, 7 },
      /* Init   Lower  Higher */
      { 14, 0, 0, 8, 2, 0 }, /* Hit points */
      { 1, 0, 0, 1, 0, 1 },
      10, /* Energy */
      0,
      10,
      -2,
      0,
      9,
      A_WIS,
      SPE_CONE_OF_COLD,
      -4 },
#if 0 /*KR:T*/
    { { "Wizard", 0 },
      { { "Evoker", 0 },
        { "Conjurer", 0 },
        { "Thaumaturge", 0 },
        { "Magician", 0 },
        { "Enchanter", "Enchantress" },
        { "Sorcerer", "Sorceress" },
        { "Necromancer", 0 },
        { "Wizard", 0 },
        { "Mage", 0 } },
      "Ptah", "Thoth", "Anhur", /* Egyptian */
#else
    { { "마법사", 0 },
      { { "소환사", 0 },
        { "기술사", 0 },
        { "요술쟁이", 0 },
        { "마술사", 0 },
        { "요술사", 0 },
        { "주술사", 0 },
        { "사령술사", 0 },
        { "마법사", 0 },
        { "대마법사", 0 } },
      "프타", "토트", "안후르", /* 이집트인 */
#endif
      "Wiz",
#if 0 /*KR:T*/
      "the Lonely Tower",
      "the Tower of Darkness",
#else
      "고독한 탑",
      "어둠의 탑",
#endif
      PM_WIZARD,
      NON_PM,
      PM_KITTEN,
      PM_NEFERET_THE_GREEN,
      PM_APPRENTICE,
      PM_DARK_ONE,
      PM_VAMPIRE_BAT,
      PM_XORN,
      S_BAT,
      S_WRAITH,
      ART_EYE_OF_THE_AETHIOPICA,
      MH_HUMAN | MH_ELF | MH_GNOME | MH_ORC | ROLE_MALE | ROLE_FEMALE
          | ROLE_NEUTRAL | ROLE_CHAOTIC,
      /* Str Int Wis Dex Con Cha */
      { 7, 10, 7, 7, 7, 7 },
      { 10, 30, 10, 20, 20, 10 },
      /* Init   Lower  Higher */
      { 10, 0, 0, 8, 1, 0 }, /* Hit points */
      { 4, 3, 0, 2, 0, 3 },
      12, /* Energy */
      0,
      1,
      0,
      3,
      10,
      A_INT,
      SPE_MAGIC_MISSILE,
      -4 },
    /* Array terminator */
    { { 0, 0 } }
};

/* The player's role, created at runtime from initial
 * choices.  This may be munged in role_init().
 */
struct Role urole = {
    { "Undefined", 0 },
    { { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
      { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },
    "L", "N", "C",
    "Xxx", "home", "locate",
    NON_PM, NON_PM, NON_PM, NON_PM, NON_PM, NON_PM, NON_PM, NON_PM,
    0, 0, 0, 0,
    /* Str Int Wis Dex Con Cha */
    { 7, 7, 7, 7, 7, 7 },
    { 20, 15, 15, 20, 20, 10 },
    /* Init   Lower  Higher */
    { 10, 0, 0, 8, 1, 0 }, /* Hit points */
    { 2, 0, 0, 2, 0, 3 },
    14, /* Energy */
     0,
    10,
     0,
     0,
     4,
    A_INT,
     0,
    -3
};

/* Table of all races */
const struct Race races[] = {
    {
#if 0 /*KR:T*/
        "human",
        "human",
        "humanity",
#else
        "인간",
        "인간의",
        "인간",
#endif
        "Hum",
#if 0 /*KR:T*/
        { "man", "woman" },
#else
        { "남자", "여자" },
#endif
        PM_HUMAN,
        NON_PM,
        PM_HUMAN_MUMMY,
        PM_HUMAN_ZOMBIE,
        MH_HUMAN | ROLE_MALE | ROLE_FEMALE | ROLE_LAWFUL | ROLE_NEUTRAL
            | ROLE_CHAOTIC,
        MH_HUMAN,
        0,
        MH_GNOME | MH_ORC,
        /*    Str     Int Wis Dex Con Cha */
        { 3, 3, 3, 3, 3, 3 },
        { STR18(100), 18, 18, 18, 18, 18 },
        /* Init   Lower  Higher */
        { 2, 0, 0, 2, 1, 0 }, /* Hit points */
        { 1, 0, 2, 0, 2, 0 }  /* Energy */
    },
    {
#if 0 /*KR:T*/
        "elf",
        "elven",
        "elvenkind",
#else
        "엘프",
        "엘프의",
        "엘프",
#endif
        "Elf",
        { 0, 0 },
        PM_ELF,
        NON_PM,
        PM_ELF_MUMMY,
        PM_ELF_ZOMBIE,
        MH_ELF | ROLE_MALE | ROLE_FEMALE | ROLE_CHAOTIC,
        MH_ELF,
        MH_ELF,
        MH_ORC,
        /*  Str    Int Wis Dex Con Cha */
        { 3, 3, 3, 3, 3, 3 },
        { 18, 20, 20, 18, 16, 18 },
        /* Init   Lower  Higher */
        { 1, 0, 0, 1, 1, 0 }, /* Hit points */
        { 2, 0, 3, 0, 3, 0 }  /* Energy */
    },
    {
#if 0 /*KR:T*/
        "dwarf",
        "dwarven",
        "dwarvenkind",
#else
        "드워프",
        "드워프의",
        "드워프",
#endif
        "Dwa",
        { 0, 0 },
        PM_DWARF,
        NON_PM,
        PM_DWARF_MUMMY,
        PM_DWARF_ZOMBIE,
        MH_DWARF | ROLE_MALE | ROLE_FEMALE | ROLE_LAWFUL,
        MH_DWARF,
        MH_DWARF | MH_GNOME,
        MH_ORC,
        /*    Str     Int Wis Dex Con Cha */
        { 3, 3, 3, 3, 3, 3 },
        { STR18(100), 16, 16, 20, 20, 16 },
        /* Init   Lower  Higher */
        { 4, 0, 0, 3, 2, 0 }, /* Hit points */
        { 0, 0, 0, 0, 0, 0 }  /* Energy */
    },
    {
#if 0 /*KR:T*/
        "gnome",
        "gnomish",
        "gnomehood",
#else
        "노움",
        "노움의",
        "노움",
#endif
        "Gno",
        { 0, 0 },
        PM_GNOME,
        NON_PM,
        PM_GNOME_MUMMY,
        PM_GNOME_ZOMBIE,
        MH_GNOME | ROLE_MALE | ROLE_FEMALE | ROLE_NEUTRAL,
        MH_GNOME,
        MH_DWARF | MH_GNOME,
        MH_HUMAN,
        /*  Str    Int Wis Dex Con Cha */
        { 3, 3, 3, 3, 3, 3 },
        { STR18(50), 19, 18, 18, 18, 18 },
        /* Init   Lower  Higher */
        { 1, 0, 0, 1, 0, 0 }, /* Hit points */
        { 2, 0, 2, 0, 2, 0 }  /* Energy */
    },
    {
#if 0 /*KR:T*/
        "orc",
        "orcish",
        "orcdom",
#else
        "오크",
        "오크의",
        "오크",
#endif
        "Orc",
        { 0, 0 },
        PM_ORC,
        NON_PM,
        PM_ORC_MUMMY,
        PM_ORC_ZOMBIE,
        MH_ORC | ROLE_MALE | ROLE_FEMALE | ROLE_CHAOTIC,
        MH_ORC,
        0,
        MH_HUMAN | MH_ELF | MH_DWARF,
        /*  Str    Int Wis Dex Con Cha */
        { 3, 3, 3, 3, 3, 3 },
        { STR18(50), 16, 16, 18, 18, 16 },
        /* Init   Lower  Higher */
        { 1, 0, 0, 1, 0, 0 }, /* Hit points */
        { 1, 0, 1, 0, 1, 0 }  /* Energy */
    },
    /* Array terminator */
    { 0, 0, 0, 0 }
};

/* The player's race, created at runtime from initial
 * choices.  This may be munged in role_init().
 */
struct Race urace = {
    "something",
    "undefined",
    "something",
    "Xxx",
    { 0, 0 },
    NON_PM,
    NON_PM,
    NON_PM,
    NON_PM,
    0,
    0,
    0,
    0,
    /*    Str     Int Wis Dex Con Cha */
    { 3, 3, 3, 3, 3, 3 },
    { STR18(100), 18, 18, 18, 18, 18 },
    /* Init   Lower  Higher */
    { 2, 0, 0, 2, 1, 0 }, /* Hit points */
    { 1, 0, 2, 0, 2, 0 }  /* Energy */
};

/* Table of all genders */
const struct Gender genders[] = {
#if 0 /*KR:T*/
    { "male", "he", "him", "his", "Mal", ROLE_MALE },
    { "female", "she", "her", "her", "Fem", ROLE_FEMALE },
    { "neuter", "it", "it", "its", "Ntr", ROLE_NEUTER }
#else
    { "남성", "그", "그", "그의", "Mal", ROLE_MALE },
    { "여성", "그녀", "그녀", "그녀의", "Fem", ROLE_FEMALE },
    { "중성", "그것", "그것", "그것의", "Ntr", ROLE_NEUTER }
#endif
};

/* Table of all alignments */
const struct Align aligns[] = {
#if 0 /*KR:T*/
    { "law", "lawful", "Law", ROLE_LAWFUL, A_LAWFUL },
    { "balance", "neutral", "Neu", ROLE_NEUTRAL, A_NEUTRAL },
    { "chaos", "chaotic", "Cha", ROLE_CHAOTIC, A_CHAOTIC },
    { "evil", "unaligned", "Una", 0, A_NONE }
#else
    { "질서", "질서의", "Law", ROLE_LAWFUL, A_LAWFUL },
    { "중립", "중립의", "Neu", ROLE_NEUTRAL, A_NEUTRAL },
    { "혼돈", "혼돈의", "Cha", ROLE_CHAOTIC, A_CHAOTIC },
    { "무성향", "무성향의", "Una", 0, A_NONE }
#endif
};

/* Filters */
static struct {
    boolean roles[SIZE(roles)];
    short mask;
} rfilter;

STATIC_DCL int NDECL(randrole_filtered);
STATIC_DCL char *FDECL(promptsep, (char *, int));
STATIC_DCL int FDECL(role_gendercount, (int));
STATIC_DCL int FDECL(race_alignmentcount, (int));

/* used by str2XXX() */
/*KR static char NEARDATA randomstr[] = "random"; */
static char NEARDATA randomstr[] = "무작위";

boolean
validrole(rolenum)
int rolenum;
{
    return (boolean) (rolenum >= 0 && rolenum < SIZE(roles) - 1);
}

int
randrole(for_display)
boolean for_display;
{
    int res = SIZE(roles) - 1;

    if (for_display)
        res = rn2_on_display_rng(res);
    else
        res = rn2(res);
    return res;
}

STATIC_OVL int
randrole_filtered()
{
    int i, n = 0, set[SIZE(roles)];

    /* this doesn't rule out impossible combinations but attempts to
       honor all the filter masks */
    for (i = 0; i < SIZE(roles); ++i)
        if (ok_role(i, ROLE_NONE, ROLE_NONE, ROLE_NONE)
            && ok_race(i, ROLE_RANDOM, ROLE_NONE, ROLE_NONE)
            && ok_gend(i, ROLE_NONE, ROLE_RANDOM, ROLE_NONE)
            && ok_align(i, ROLE_NONE, ROLE_NONE, ROLE_RANDOM))
            set[n++] = i;
    return n ? set[rn2(n)] : randrole(FALSE);
}

int
str2role(str)
const char *str;
{
    int i, len;

    /* Is str valid? */
    if (!str || !str[0])
        return ROLE_NONE;

    /* Match as much of str as is provided */
    len = strlen(str);
    for (i = 0; roles[i].name.m; i++) {
        /* Does it match the male name? */
        if (!strncmpi(str, roles[i].name.m, len))
            return i;
        /* Or the female name? */
        if (roles[i].name.f && !strncmpi(str, roles[i].name.f, len))
            return i;
        /* Or the filecode? */
        if (!strcmpi(str, roles[i].filecode))
            return i;
    }

    if ((len == 1 && (*str == '*' || *str == '@'))
        || !strncmpi(str, randomstr, len))
        return ROLE_RANDOM;

    /* Couldn't find anything appropriate */
    return ROLE_NONE;
}

boolean
validrace(rolenum, racenum)
int rolenum, racenum;
{
    /* Assumes validrole */
    return (boolean) (racenum >= 0 && racenum < SIZE(races) - 1
                      && (roles[rolenum].allow & races[racenum].allow
                          & ROLE_RACEMASK));
}

int
randrace(rolenum)
int rolenum;
{
    int i, n = 0;

    /* Count the number of valid races */
    for (i = 0; races[i].noun; i++)
        if (roles[rolenum].allow & races[i].allow & ROLE_RACEMASK)
            n++;

    /* Pick a random race */
    /* Use a factor of 100 in case of bad random number generators */
    if (n)
        n = rn2(n * 100) / 100;
    for (i = 0; races[i].noun; i++)
        if (roles[rolenum].allow & races[i].allow & ROLE_RACEMASK) {
            if (n)
                n--;
            else
                return i;
        }

    /* This role has no permitted races? */
    return rn2(SIZE(races) - 1);
}

int
str2race(str)
const char *str;
{
    int i, len;

    /* Is str valid? */
    if (!str || !str[0])
        return ROLE_NONE;

    /* Match as much of str as is provided */
    len = strlen(str);
    for (i = 0; races[i].noun; i++) {
        /* Does it match the noun? */
        if (!strncmpi(str, races[i].noun, len))
            return i;
        /* Or the filecode? */
        if (!strcmpi(str, races[i].filecode))
            return i;
    }

    if ((len == 1 && (*str == '*' || *str == '@'))
        || !strncmpi(str, randomstr, len))
        return ROLE_RANDOM;

    /* Couldn't find anything appropriate */
    return ROLE_NONE;
}

boolean
validgend(rolenum, racenum, gendnum)
int rolenum, racenum, gendnum;
{
    /* Assumes validrole and validrace */
    return (boolean) (gendnum >= 0 && gendnum < ROLE_GENDERS
                      && (roles[rolenum].allow & races[racenum].allow
                          & genders[gendnum].allow & ROLE_GENDMASK));
}

int
randgend(rolenum, racenum)
int rolenum, racenum;
{
    int i, n = 0;

    /* Count the number of valid genders */
    for (i = 0; i < ROLE_GENDERS; i++)
        if (roles[rolenum].allow & races[racenum].allow & genders[i].allow
            & ROLE_GENDMASK)
            n++;

    /* Pick a random gender */
    if (n)
        n = rn2(n);
    for (i = 0; i < ROLE_GENDERS; i++)
        if (roles[rolenum].allow & races[racenum].allow & genders[i].allow
            & ROLE_GENDMASK) {
            if (n)
                n--;
            else
                return i;
        }

    /* This role/race has no permitted genders? */
    return rn2(ROLE_GENDERS);
}

int
str2gend(str)
const char *str;
{
    int i, len;

    /* Is str valid? */
    if (!str || !str[0])
        return ROLE_NONE;

    /* Match as much of str as is provided */
    len = strlen(str);
    for (i = 0; i < ROLE_GENDERS; i++) {
        /* Does it match the adjective? */
        if (!strncmpi(str, genders[i].adj, len))
            return i;
        /* Or the filecode? */
        if (!strcmpi(str, genders[i].filecode))
            return i;
    }
    if ((len == 1 && (*str == '*' || *str == '@'))
        || !strncmpi(str, randomstr, len))
        return ROLE_RANDOM;

    /* Couldn't find anything appropriate */
    return ROLE_NONE;
}

boolean
validalign(rolenum, racenum, alignnum)
int rolenum, racenum, alignnum;
{
    /* Assumes validrole and validrace */
    return (boolean) (alignnum >= 0 && alignnum < ROLE_ALIGNS
                      && (roles[rolenum].allow & races[racenum].allow
                          & aligns[alignnum].allow & ROLE_ALIGNMASK));
}

int
randalign(rolenum, racenum)
int rolenum, racenum;
{
    int i, n = 0;

    /* Count the number of valid alignments */
    for (i = 0; i < ROLE_ALIGNS; i++)
        if (roles[rolenum].allow & races[racenum].allow & aligns[i].allow
            & ROLE_ALIGNMASK)
            n++;

    /* Pick a random alignment */
    if (n)
        n = rn2(n);
    for (i = 0; i < ROLE_ALIGNS; i++)
        if (roles[rolenum].allow & races[racenum].allow & aligns[i].allow
            & ROLE_ALIGNMASK) {
            if (n)
                n--;
            else
                return i;
        }

    /* This role/race has no permitted alignments? */
    return rn2(ROLE_ALIGNS);
}

int
str2align(str)
const char *str;
{
    int i, len;

    /* Is str valid? */
    if (!str || !str[0])
        return ROLE_NONE;

    /* Match as much of str as is provided */
    len = strlen(str);
    for (i = 0; i < ROLE_ALIGNS; i++) {
        /* Does it match the adjective? */
        if (!strncmpi(str, aligns[i].adj, len))
            return i;
        /* Or the filecode? */
        if (!strcmpi(str, aligns[i].filecode))
            return i;
    }
    if ((len == 1 && (*str == '*' || *str == '@'))
        || !strncmpi(str, randomstr, len))
        return ROLE_RANDOM;

    /* Couldn't find anything appropriate */
    return ROLE_NONE;
}

/* is rolenum compatible with any racenum/gendnum/alignnum constraints? */
boolean
ok_role(rolenum, racenum, gendnum, alignnum)
int rolenum, racenum, gendnum, alignnum;
{
    int i;
    short allow;

    if (rolenum >= 0 && rolenum < SIZE(roles) - 1) {
        if (rfilter.roles[rolenum])
            return FALSE;
        allow = roles[rolenum].allow;
        if (racenum >= 0 && racenum < SIZE(races) - 1
            && !(allow & races[racenum].allow & ROLE_RACEMASK))
            return FALSE;
        if (gendnum >= 0 && gendnum < ROLE_GENDERS
            && !(allow & genders[gendnum].allow & ROLE_GENDMASK))
            return FALSE;
        if (alignnum >= 0 && alignnum < ROLE_ALIGNS
            && !(allow & aligns[alignnum].allow & ROLE_ALIGNMASK))
            return FALSE;
        return TRUE;
    } else {
        /* random; check whether any selection is possible */
        for (i = 0; i < SIZE(roles) - 1; i++) {
            if (rfilter.roles[i])
                continue;
            allow = roles[i].allow;
            if (racenum >= 0 && racenum < SIZE(races) - 1
                && !(allow & races[racenum].allow & ROLE_RACEMASK))
                continue;
            if (gendnum >= 0 && gendnum < ROLE_GENDERS
                && !(allow & genders[gendnum].allow & ROLE_GENDMASK))
                continue;
            if (alignnum >= 0 && alignnum < ROLE_ALIGNS
                && !(allow & aligns[alignnum].allow & ROLE_ALIGNMASK))
                continue;
            return TRUE;
        }
        return FALSE;
    }
}

/* pick a random role subject to any racenum/gendnum/alignnum constraints */
/* If pickhow == PICK_RIGID a role is returned only if there is  */
/* a single possibility */
int
pick_role(racenum, gendnum, alignnum, pickhow)
int racenum, gendnum, alignnum, pickhow;
{
    int i;
    int roles_ok = 0, set[SIZE(roles)];

    for (i = 0; i < SIZE(roles) - 1; i++) {
        if (ok_role(i, racenum, gendnum, alignnum)
            && ok_race(i, (racenum >= 0) ? racenum : ROLE_RANDOM,
                       gendnum, alignnum)
            && ok_gend(i, racenum,
                       (gendnum >= 0) ? gendnum : ROLE_RANDOM, alignnum)
            && ok_race(i, racenum,
                       gendnum, (alignnum >= 0) ? alignnum : ROLE_RANDOM))
            set[roles_ok++] = i;
    }
    if (roles_ok == 0 || (roles_ok > 1 && pickhow == PICK_RIGID))
        return ROLE_NONE;
    return set[rn2(roles_ok)];
}

/* is racenum compatible with any rolenum/gendnum/alignnum constraints? */
boolean
ok_race(rolenum, racenum, gendnum, alignnum)
int rolenum, racenum, gendnum, alignnum;
{
    int i;
    short allow;

    if (racenum >= 0 && racenum < SIZE(races) - 1) {
        if (rfilter.mask & races[racenum].selfmask)
            return FALSE;
        allow = races[racenum].allow;
        if (rolenum >= 0 && rolenum < SIZE(roles) - 1
            && !(allow & roles[rolenum].allow & ROLE_RACEMASK))
            return FALSE;
        if (gendnum >= 0 && gendnum < ROLE_GENDERS
            && !(allow & genders[gendnum].allow & ROLE_GENDMASK))
            return FALSE;
        if (alignnum >= 0 && alignnum < ROLE_ALIGNS
            && !(allow & aligns[alignnum].allow & ROLE_ALIGNMASK))
            return FALSE;
        return TRUE;
    } else {
        /* random; check whether any selection is possible */
        for (i = 0; i < SIZE(races) - 1; i++) {
            if (rfilter.mask & races[i].selfmask)
                continue;
            allow = races[i].allow;
            if (rolenum >= 0 && rolenum < SIZE(roles) - 1
                && !(allow & roles[rolenum].allow & ROLE_RACEMASK))
                continue;
            if (gendnum >= 0 && gendnum < ROLE_GENDERS
                && !(allow & genders[gendnum].allow & ROLE_GENDMASK))
                continue;
            if (alignnum >= 0 && alignnum < ROLE_ALIGNS
                && !(allow & aligns[alignnum].allow & ROLE_ALIGNMASK))
                continue;
            return TRUE;
        }
        return FALSE;
    }
}

/* Pick a random race subject to any rolenum/gendnum/alignnum constraints.
   If pickhow == PICK_RIGID a race is returned only if there is
   a single possibility. */
int
pick_race(rolenum, gendnum, alignnum, pickhow)
int rolenum, gendnum, alignnum, pickhow;
{
    int i;
    int races_ok = 0;

    for (i = 0; i < SIZE(races) - 1; i++) {
        if (ok_race(rolenum, i, gendnum, alignnum))
            races_ok++;
    }
    if (races_ok == 0 || (races_ok > 1 && pickhow == PICK_RIGID))
        return ROLE_NONE;
    races_ok = rn2(races_ok);
    for (i = 0; i < SIZE(races) - 1; i++) {
        if (ok_race(rolenum, i, gendnum, alignnum)) {
            if (races_ok == 0)
                return i;
            else
                races_ok--;
        }
    }
    return ROLE_NONE;
}

/* is gendnum compatible with any rolenum/racenum/alignnum constraints? */
/* gender and alignment are not comparable (and also not constrainable) */
boolean
ok_gend(rolenum, racenum, gendnum, alignnum)
int rolenum, racenum, gendnum;
int alignnum UNUSED;
{
    int i;
    short allow;

    if (gendnum >= 0 && gendnum < ROLE_GENDERS) {
        if (rfilter.mask & genders[gendnum].allow)
            return FALSE;
        allow = genders[gendnum].allow;
        if (rolenum >= 0 && rolenum < SIZE(roles) - 1
            && !(allow & roles[rolenum].allow & ROLE_GENDMASK))
            return FALSE;
        if (racenum >= 0 && racenum < SIZE(races) - 1
            && !(allow & races[racenum].allow & ROLE_GENDMASK))
            return FALSE;
        return TRUE;
    } else {
        /* random; check whether any selection is possible */
        for (i = 0; i < ROLE_GENDERS; i++) {
            if (rfilter.mask & genders[i].allow)
                continue;
            allow = genders[i].allow;
            if (rolenum >= 0 && rolenum < SIZE(roles) - 1
                && !(allow & roles[rolenum].allow & ROLE_GENDMASK))
                continue;
            if (racenum >= 0 && racenum < SIZE(races) - 1
                && !(allow & races[racenum].allow & ROLE_GENDMASK))
                continue;
            return TRUE;
        }
        return FALSE;
    }
}

/* pick a random gender subject to any rolenum/racenum/alignnum constraints */
/* gender and alignment are not comparable (and also not constrainable) */
/* If pickhow == PICK_RIGID a gender is returned only if there is  */
/* a single possibility */
int
pick_gend(rolenum, racenum, alignnum, pickhow)
int rolenum, racenum, alignnum, pickhow;
{
    int i;
    int gends_ok = 0;

    for (i = 0; i < ROLE_GENDERS; i++) {
        if (ok_gend(rolenum, racenum, i, alignnum))
            gends_ok++;
    }
    if (gends_ok == 0 || (gends_ok > 1 && pickhow == PICK_RIGID))
        return ROLE_NONE;
    gends_ok = rn2(gends_ok);
    for (i = 0; i < ROLE_GENDERS; i++) {
        if (ok_gend(rolenum, racenum, i, alignnum)) {
            if (gends_ok == 0)
                return i;
            else
                gends_ok--;
        }
    }
    return ROLE_NONE;
}

/* is alignnum compatible with any rolenum/racenum/gendnum constraints? */
/* alignment and gender are not comparable (and also not constrainable) */
boolean
ok_align(rolenum, racenum, gendnum, alignnum)
int rolenum, racenum;
int gendnum UNUSED;
int alignnum;
{
    int i;
    short allow;

    if (alignnum >= 0 && alignnum < ROLE_ALIGNS) {
        if (rfilter.mask & aligns[alignnum].allow)
            return FALSE;
        allow = aligns[alignnum].allow;
        if (rolenum >= 0 && rolenum < SIZE(roles) - 1
            && !(allow & roles[rolenum].allow & ROLE_ALIGNMASK))
            return FALSE;
        if (racenum >= 0 && racenum < SIZE(races) - 1
            && !(allow & races[racenum].allow & ROLE_ALIGNMASK))
            return FALSE;
        return TRUE;
    } else {
        /* random; check whether any selection is possible */
        for (i = 0; i < ROLE_ALIGNS; i++) {
            if (rfilter.mask & aligns[i].allow)
                return FALSE;
            allow = aligns[i].allow;
            if (rolenum >= 0 && rolenum < SIZE(roles) - 1
                && !(allow & roles[rolenum].allow & ROLE_ALIGNMASK))
                continue;
            if (racenum >= 0 && racenum < SIZE(races) - 1
                && !(allow & races[racenum].allow & ROLE_ALIGNMASK))
                continue;
            return TRUE;
        }
        return FALSE;
    }
}

/* Pick a random alignment subject to any rolenum/racenum/gendnum constraints;
   alignment and gender are not comparable (and also not constrainable).
   If pickhow == PICK_RIGID an alignment is returned only if there is
   a single possibility. */
int
pick_align(rolenum, racenum, gendnum, pickhow)
int rolenum, racenum, gendnum, pickhow;
{
    int i;
    int aligns_ok = 0;

    for (i = 0; i < ROLE_ALIGNS; i++) {
        if (ok_align(rolenum, racenum, gendnum, i))
            aligns_ok++;
    }
    if (aligns_ok == 0 || (aligns_ok > 1 && pickhow == PICK_RIGID))
        return ROLE_NONE;
    aligns_ok = rn2(aligns_ok);
    for (i = 0; i < ROLE_ALIGNS; i++) {
        if (ok_align(rolenum, racenum, gendnum, i)) {
            if (aligns_ok == 0)
                return i;
            else
                aligns_ok--;
        }
    }
    return ROLE_NONE;
}

void
rigid_role_checks()
{
    int tmp;

    /* Some roles are limited to a single race, alignment, or gender and
     * calling this routine prior to XXX_player_selection() will help
     * prevent an extraneous prompt that actually doesn't allow
     * you to choose anything further. Note the use of PICK_RIGID which
     * causes the pick_XX() routine to return a value only if there is one
     * single possible selection, otherwise it returns ROLE_NONE.
     *
     */
    if (flags.initrole == ROLE_RANDOM) {
        /* If the role was explicitly specified as ROLE_RANDOM
         * via -uXXXX-@ or OPTIONS=role:random then choose the role
         * in here to narrow down later choices.
         */
        flags.initrole = pick_role(flags.initrace, flags.initgend,
                                   flags.initalign, PICK_RANDOM);
        if (flags.initrole < 0)
            flags.initrole = randrole_filtered();
    }
    if (flags.initrace == ROLE_RANDOM
        && (tmp = pick_race(flags.initrole, flags.initgend,
                            flags.initalign, PICK_RANDOM)) != ROLE_NONE)
        flags.initrace = tmp;
    if (flags.initalign == ROLE_RANDOM
        && (tmp = pick_align(flags.initrole, flags.initrace,
                             flags.initgend, PICK_RANDOM)) != ROLE_NONE)
        flags.initalign = tmp;
    if (flags.initgend == ROLE_RANDOM
        && (tmp = pick_gend(flags.initrole, flags.initrace,
                            flags.initalign, PICK_RANDOM)) != ROLE_NONE)
        flags.initgend = tmp;

    if (flags.initrole != ROLE_NONE) {
        if (flags.initrace == ROLE_NONE)
            flags.initrace = pick_race(flags.initrole, flags.initgend,
                                       flags.initalign, PICK_RIGID);
        if (flags.initalign == ROLE_NONE)
            flags.initalign = pick_align(flags.initrole, flags.initrace,
                                         flags.initgend, PICK_RIGID);
        if (flags.initgend == ROLE_NONE)
            flags.initgend = pick_gend(flags.initrole, flags.initrace,
                                       flags.initalign, PICK_RIGID);
    }
}

boolean
setrolefilter(bufp)
const char *bufp;
{
    int i;
    boolean reslt = TRUE;

    if ((i = str2role(bufp)) != ROLE_NONE && i != ROLE_RANDOM)
        rfilter.roles[i] = TRUE;
    else if ((i = str2race(bufp)) != ROLE_NONE && i != ROLE_RANDOM)
        rfilter.mask |= races[i].selfmask;
    else if ((i = str2gend(bufp)) != ROLE_NONE && i != ROLE_RANDOM)
        rfilter.mask |= genders[i].allow;
    else if ((i = str2align(bufp)) != ROLE_NONE && i != ROLE_RANDOM)
        rfilter.mask |= aligns[i].allow;
    else
        reslt = FALSE;
    return reslt;
}

boolean
gotrolefilter()
{
    int i;

    if (rfilter.mask)
        return TRUE;
    for (i = 0; i < SIZE(roles); ++i)
        if (rfilter.roles[i])
            return TRUE;
    return FALSE;
}

void
clearrolefilter()
{
    int i;

    for (i = 0; i < SIZE(roles); ++i)
        rfilter.roles[i] = FALSE;
    rfilter.mask = 0;
}

#define BP_ALIGN 0
#define BP_GEND 1
#define BP_RACE 2
#define BP_ROLE 3
#define NUM_BP 4

STATIC_VAR char pa[NUM_BP], post_attribs;

STATIC_OVL char *
promptsep(buf, num_post_attribs)
char *buf;
int num_post_attribs;
{
#if 0 /*KR*/
    const char *conjuct = "and ";

    if (num_post_attribs > 1 && post_attribs < num_post_attribs
        && post_attribs > 1)
        Strcat(buf, ",");
    Strcat(buf, " ");
    --post_attribs;
    if (!post_attribs && num_post_attribs > 1)
        Strcat(buf, conjuct);
#else
    /*KR 한국어에는 'A.B. and C'라는 표현이 불필요함 */
    if (num_post_attribs > post_attribs) {
        Strcat(buf, ", ");
    }
    --post_attribs;
#endif
    return buf;
}

STATIC_OVL int
role_gendercount(rolenum)
int rolenum;
{
    int gendcount = 0;

    if (validrole(rolenum)) {
        if (roles[rolenum].allow & ROLE_MALE)
            ++gendcount;
        if (roles[rolenum].allow & ROLE_FEMALE)
            ++gendcount;
        if (roles[rolenum].allow & ROLE_NEUTER)
            ++gendcount;
    }
    return gendcount;
}

STATIC_OVL int
race_alignmentcount(racenum)
int racenum;
{
    int aligncount = 0;

    if (racenum != ROLE_NONE && racenum != ROLE_RANDOM) {
        if (races[racenum].allow & ROLE_CHAOTIC)
            ++aligncount;
        if (races[racenum].allow & ROLE_LAWFUL)
            ++aligncount;
        if (races[racenum].allow & ROLE_NEUTRAL)
            ++aligncount;
    }
    return aligncount;
}

char *
root_plselection_prompt(suppliedbuf, buflen, rolenum, racenum, gendnum,
                        alignnum)
char *suppliedbuf;
int buflen, rolenum, racenum, gendnum, alignnum;
{
    int k, gendercount = 0, aligncount = 0;
    char buf[BUFSZ];
    /*KR static char err_ret[] = " character's"; */
    static char err_ret[] = "캐릭터의";
#if 0 /*KR*/
    boolean donefirst = FALSE;
#endif

    if (!suppliedbuf || buflen < 1)
        return err_ret;

    /* initialize these static variables each time this is called */
    post_attribs = 0;
    for (k = 0; k < NUM_BP; ++k)
        pa[k] = 0;
    buf[0] = '\0';
    *suppliedbuf = '\0';

    /* How many alignments are allowed for the desired race? */
    if (racenum != ROLE_NONE && racenum != ROLE_RANDOM)
        aligncount = race_alignmentcount(racenum);

    if (alignnum != ROLE_NONE && alignnum != ROLE_RANDOM
        && ok_align(rolenum, racenum, gendnum, alignnum)) {
        /* if race specified, and multiple choice of alignments for it */
        if ((racenum >= 0) && (aligncount > 1)) {
#if 0 /*KR*/
            if (donefirst)
                Strcat(buf, " ");
#endif
            Strcat(buf, aligns[alignnum].adj);
#if 0 /*KR*/
            donefirst = TRUE;
#endif
        } else {
#if 0 /*KR*/
            if (donefirst)
                Strcat(buf, " ");
#endif
            Strcat(buf, aligns[alignnum].adj);
#if 0 /*KR*/
            donefirst = TRUE;
#endif
        }
    } else {
        /* in case we got here by failing the ok_align() test */
        if (alignnum != ROLE_RANDOM)
            alignnum = ROLE_NONE;
        /* if alignment not specified, but race is specified
           and only one choice of alignment for that race then
           don't include it in the later list */
        if ((((racenum != ROLE_NONE && racenum != ROLE_RANDOM)
              && ok_race(rolenum, racenum, gendnum, alignnum))
             && (aligncount > 1))
            || (racenum == ROLE_NONE || racenum == ROLE_RANDOM)) {
            pa[BP_ALIGN] = 1;
            post_attribs++;
        }
    }
    /* <your lawful> */

    /* How many genders are allowed for the desired role? */
    if (validrole(rolenum))
        gendercount = role_gendercount(rolenum);

    if (gendnum != ROLE_NONE && gendnum != ROLE_RANDOM) {
        if (validrole(rolenum)) {
            /* if role specified, and multiple choice of genders for it,
               and name of role itself does not distinguish gender */
            if ((rolenum != ROLE_NONE) && (gendercount > 1)
                && !roles[rolenum].name.f) {
#if 0 /*KR*/
                if (donefirst)
                    Strcat(buf, " ");
#endif
                Strcat(buf, genders[gendnum].adj);
#if 0 /*KR*/
                donefirst = TRUE;
#endif
            }
        } else {
#if 0 /*KR*/
            if (donefirst)
                Strcat(buf, " ");
#endif
            Strcat(buf, genders[gendnum].adj);
#if 0 /*KR*/
            donefirst = TRUE;
#endif
        }
    } else {
        /* if gender not specified, but role is specified
                and only one choice of gender then
                don't include it in the later list */
        if ((validrole(rolenum) && (gendercount > 1))
            || !validrole(rolenum)) {
            pa[BP_GEND] = 1;
            post_attribs++;
        }
    }
    /* <your lawful female> */

    if (racenum != ROLE_NONE && racenum != ROLE_RANDOM) {
        if (validrole(rolenum)
            && ok_race(rolenum, racenum, gendnum, alignnum)) {
#if 0 /*KR*/
            if (donefirst)
                Strcat(buf, " ");
#endif
            Strcat(buf, (rolenum == ROLE_NONE) ? races[racenum].noun
                                               : races[racenum].adj);
#if 0 /*KR*/
            donefirst = TRUE;
#endif
        } else if (!validrole(rolenum)) {
#if 0 /*KR*/
            if (donefirst)
                Strcat(buf, " ");
#endif
            Strcat(buf, races[racenum].noun);
#if 0 /*KR*/
            donefirst = TRUE;
#endif
        } else {
            pa[BP_RACE] = 1;
            post_attribs++;
        }
    } else {
        pa[BP_RACE] = 1;
        post_attribs++;
    }
    /* <your lawful female gnomish> || <your lawful female gnome> */

    if (validrole(rolenum)) {
#if 0 /*KR*/
        if (donefirst)
            Strcat(buf, " ");
#endif
        if (gendnum != ROLE_NONE) {
            if (gendnum == 1 && roles[rolenum].name.f)
                Strcat(buf, roles[rolenum].name.f);
            else
                Strcat(buf, roles[rolenum].name.m);
        } else {
            if (roles[rolenum].name.f) {
                Strcat(buf, roles[rolenum].name.m);
                Strcat(buf, "/");
                Strcat(buf, roles[rolenum].name.f);
            } else
                Strcat(buf, roles[rolenum].name.m);
        }
#if 0 /*KR*/
        donefirst = TRUE;
#endif
    } else if (rolenum == ROLE_NONE) {
        pa[BP_ROLE] = 1;
        post_attribs++;
    }

    if ((racenum == ROLE_NONE || racenum == ROLE_RANDOM)
        && !validrole(rolenum)) {
#if 0 /*KR*/
        if (donefirst)
            Strcat(buf, " ");
#endif
        /*KR Strcat(buf, "character"); */
        Strcat(buf, "캐릭터");
#if 0 /*KR*/
        donefirst = TRUE;
#endif
    }
    /* <your lawful female gnomish cavewoman> || <your lawful female gnome>
     *    || <your lawful female character>
     */
    if (buflen > (int) (strlen(buf) + 1)) {
        Strcpy(suppliedbuf, buf);
        return suppliedbuf;
    } else
        return err_ret;
}

char *
build_plselection_prompt(buf, buflen, rolenum, racenum, gendnum, alignnum)
char *buf;
int buflen, rolenum, racenum, gendnum, alignnum;
{
    /*KR const char *defprompt = "Shall I pick a character for you? [ynaq] "; */
    const char *defprompt = "제가 캐릭터를 대신 골라 드릴까요? [ynaq] ";
    int num_post_attribs = 0;
#if 0 /*KR*/
    char tmpbuf[BUFSZ], *p;
#else
    char tmpbuf[BUFSZ];
#endif
    if (buflen < QBUFSZ)
        return (char *) defprompt;
#if 0 /*KR*/
    Strcpy(tmpbuf, "Shall I pick ");
    if (racenum != ROLE_NONE || validrole(rolenum))
        Strcat(tmpbuf, "your ");
    else
        Strcat(tmpbuf, "a ");
#else
    Strcpy(tmpbuf, "");
#endif
    /* <your> */

    (void) root_plselection_prompt(eos(tmpbuf), buflen - strlen(tmpbuf),
                                   rolenum, racenum, gendnum, alignnum);
#if 0 /*KR*/ /*한국어에서는 불필요*/
    /* "Shall I pick a character's role, race, gender, and alignment for you?"
       plus " [ynaq] (y)" is a little too long for a conventional 80 columns;
       also, "pick a character's <anything>" sounds a bit stilted */
    strsubst(tmpbuf, "pick a character", "pick character");
#endif
#if 0 /*KR*/
    Sprintf(buf, "%s", s_suffix(tmpbuf));
#else 
    Sprintf(buf, "%s의", tmpbuf);
#endif
#if 0 /*KR:한국어에서는 불필요*/
    /* don't bother splitting caveman/cavewoman or priest/priestess
       in order to apply possessive suffix to both halves, but do
       change "priest/priestess'" to "priest/priestess's" */
    if ((p = strstri(buf, "priest/priestess'")) != 0
        && p[sizeof "priest/priestess'" - sizeof ""] == '\0')
        strkitten(buf, 's');
#endif

    /* buf should now be:
     *    <your lawful female gnomish cavewoman's>
     * || <your lawful female gnome's>
     * || <your lawful female character's>
     *
     * Now append the post attributes to it
     */
    num_post_attribs = post_attribs;
    if (!num_post_attribs) {
        /* some constraints might have been mutually exclusive, in which case
           some prompting that would have been omitted is needed after all */
        if (flags.initrole == ROLE_NONE && !pa[BP_ROLE])
            pa[BP_ROLE] = ++post_attribs;
        if (flags.initrace == ROLE_NONE && !pa[BP_RACE])
            pa[BP_RACE] = ++post_attribs;
        if (flags.initalign == ROLE_NONE && !pa[BP_ALIGN])
            pa[BP_ALIGN] = ++post_attribs;
        if (flags.initgend == ROLE_NONE && !pa[BP_GEND])
            pa[BP_GEND] = ++post_attribs;
        num_post_attribs = post_attribs;
    }
    if (num_post_attribs) {
        if (pa[BP_RACE]) {
            (void) promptsep(eos(buf), num_post_attribs);
            /*KR Strcat(buf, "race"); */
            Strcat(buf, "종족");
        }
        if (pa[BP_ROLE]) {
            (void) promptsep(eos(buf), num_post_attribs);
            /*KR Strcat(buf, "role"); */
            Strcat(buf, "직업");
        }
        if (pa[BP_GEND]) {
            (void) promptsep(eos(buf), num_post_attribs);
            /*KR Strcat(buf, "gender"); */
            Strcat(buf, "성별");
        }
        if (pa[BP_ALIGN]) {
            (void) promptsep(eos(buf), num_post_attribs);
            /*KR Strcat(buf, "alignment"); */
            Strcat(buf, "성향");
        }
    }
    /*KR Strcat(buf, " for you? [ynaq] "); */
    Strcat(buf, "을 골라 드릴까요? [ynaq] ");
    return buf;
}

#undef BP_ALIGN
#undef BP_GEND
#undef BP_RACE
#undef BP_ROLE
#undef NUM_BP

void
plnamesuffix()
{
    char *sptr, *eptr;
    int i;

    /* some generic user names will be ignored in favor of prompting */
    if (sysopt.genericusers) {
        if (*sysopt.genericusers == '*') {
            *plname = '\0';
        } else {
            i = (int) strlen(plname);
            if ((sptr = strstri(sysopt.genericusers, plname)) != 0
                && (sptr == sysopt.genericusers || sptr[-1] == ' ')
                && (sptr[i] == ' ' || sptr[i] == '\0'))
                *plname = '\0'; /* call askname() */
        }
    }

    do {
        if (!*plname)
            askname(); /* fill plname[] if necessary, or set defer_plname */

        /* Look for tokens delimited by '-' */
        if ((eptr = index(plname, '-')) != (char *) 0)
            *eptr++ = '\0';
        while (eptr) {
            /* Isolate the next token */
            sptr = eptr;
            if ((eptr = index(sptr, '-')) != (char *) 0)
                *eptr++ = '\0';

            /* Try to match it to something */
            if ((i = str2role(sptr)) != ROLE_NONE)
                flags.initrole = i;
            else if ((i = str2race(sptr)) != ROLE_NONE)
                flags.initrace = i;
            else if ((i = str2gend(sptr)) != ROLE_NONE)
                flags.initgend = i;
            else if ((i = str2align(sptr)) != ROLE_NONE)
                flags.initalign = i;
        }
    } while (!*plname && !iflags.defer_plname);

    /* commas in the plname confuse the record file, convert to spaces */
    for (sptr = plname; *sptr; sptr++) {
        if (*sptr == ',')
            *sptr = ' ';
    }
}

/* show current settings for name, role, race, gender, and alignment
   in the specified window */
void
role_selection_prolog(which, where)
int which;
winid where;
{
    /*KR static const char NEARDATA choosing[] = " choosing now", */
    static const char NEARDATA choosing[] = " 현재 선택중",
                            /*KR not_yet[] = " not yet specified",*/         
                               not_yet[] = " 아직 정하지 않음",
                            /*KR rand_choice[] = " random"; */
                               rand_choice[] = " 무작위";
    char buf[BUFSZ];
    int r, c, g, a, allowmask;

    r = flags.initrole;
    c = flags.initrace;
    g = flags.initgend;
    a = flags.initalign;
    if (r >= 0) {
        allowmask = roles[r].allow;
        if ((allowmask & ROLE_RACEMASK) == MH_HUMAN)
            c = 0; /* races[human] */
        else if (c >= 0 && !(allowmask & ROLE_RACEMASK & races[c].allow))
            c = ROLE_RANDOM;
        if ((allowmask & ROLE_GENDMASK) == ROLE_MALE)
            g = 0; /* role forces male (hypothetical) */
        else if ((allowmask & ROLE_GENDMASK) == ROLE_FEMALE)
            g = 1; /* role forces female (valkyrie) */
        if ((allowmask & ROLE_ALIGNMASK) == AM_LAWFUL)
            a = 0; /* aligns[lawful] */
        else if ((allowmask & ROLE_ALIGNMASK) == AM_NEUTRAL)
            a = 1; /* aligns[neutral] */
        else if ((allowmask & ROLE_ALIGNMASK) == AM_CHAOTIC)
            a = 2; /* alings[chaotic] */
    }
    if (c >= 0) {
        allowmask = races[c].allow;
        if ((allowmask & ROLE_ALIGNMASK) == AM_LAWFUL)
            a = 0; /* aligns[lawful] */
        else if ((allowmask & ROLE_ALIGNMASK) == AM_NEUTRAL)
            a = 1; /* aligns[neutral] */
        else if ((allowmask & ROLE_ALIGNMASK) == AM_CHAOTIC)
            a = 2; /* alings[chaotic] */
        /* [c never forces gender] */
    }
    /* [g and a don't constrain anything sufficiently
       to narrow something done to a single choice] */

    /*KR Sprintf(buf, "%12s ", "name:"); */
    Sprintf(buf, "%12s ", "이름:");
    Strcat(buf, (which == RS_NAME) ? choosing : !*plname ? not_yet : plname);
    putstr(where, 0, buf);
    /*KR Sprintf(buf, "%12s ", "name:"); */
    Sprintf(buf, "%12s ", "role:");
    Strcat(buf, (which == RS_ROLE) ? choosing : (r == ROLE_NONE)
                                                    ? not_yet
                                                    : (r == ROLE_RANDOM)
                                                          ? rand_choice
                                                          : roles[r].name.m);
    if (r >= 0 && roles[r].name.f) {
        /* distinct female name [caveman/cavewoman, priest/priestess] */
        if (g == 1)
            /* female specified; replace male role name with female one */
            Sprintf(index(buf, ':'), ": %s", roles[r].name.f);
        else if (g < 0)
            /* gender unspecified; append slash and female role name */
            Sprintf(eos(buf), "/%s", roles[r].name.f);
    }
    putstr(where, 0, buf);
    /*KR Sprintf(buf, "%12s ", "race:"); */
    Sprintf(buf, "%12s ", "종족:");
    Strcat(buf, (which == RS_RACE) ? choosing : (c == ROLE_NONE)
                                                    ? not_yet
                                                    : (c == ROLE_RANDOM)
                                                          ? rand_choice
                                                          : races[c].noun);
    putstr(where, 0, buf);
    /*KR Sprintf(buf, "%12s ", "gender:"); */
    Sprintf(buf, "%12s ", "성별:");
    Strcat(buf, (which == RS_GENDER) ? choosing : (g == ROLE_NONE)
                                                      ? not_yet
                                                      : (g == ROLE_RANDOM)
                                                            ? rand_choice
                                                            : genders[g].adj);
    putstr(where, 0, buf);
    /*KR Sprintf(buf, "%12s ", "alignment:"); */
    Sprintf(buf, "%12s ", "성향:");
#if 0 /*KR: '일본어에서 명사는 자동' 이라는 주석이 있음 */
    Strcat(buf, (which == RS_ALGNMNT) ? choosing : (a == ROLE_NONE)
                                                       ? not_yet
                                                       : (a == ROLE_RANDOM)
                                                             ? rand_choice
                                                             : aligns[a].adj);
#else
    Strcat(buf, (which == RS_ALGNMNT) ? choosing : (a == ROLE_NONE)
                                                 ? not_yet
                                                 : (a == ROLE_RANDOM)
                                                       ? rand_choice
                                                       : aligns[a].noun);
#endif
    putstr(where, 0, buf);
}

/* add a "pick alignment first"-type entry to the specified menu */
void
role_menu_extra(which, where, preselect)
int which;
winid where;
boolean preselect;
{
    static NEARDATA const char RS_menu_let[] = {
        '=',  /* name */
        '?',  /* role */
        '/',  /* race */
        '\"', /* gender */
        '[',  /* alignment */
    };
    anything any;
    char buf[BUFSZ];
    const char *what = 0, *constrainer = 0, *forcedvalue = 0;
    int f = 0, r, c, g, a, i, allowmask;

    r = flags.initrole;
    c = flags.initrace;
    switch (which) {
    case RS_NAME:
        /*KR what = "name"; */
        what = "이름";
        break;
    case RS_ROLE:
        /*KR what = "role"; */
        what = "직업";
        f = r;
        for (i = 0; i < SIZE(roles); ++i)
            if (i != f && !rfilter.roles[i])
                break;
        if (i == SIZE(roles)) {
            /*KR constrainer = "filter"; */
            constrainer = "필터";
            /*KR forcedvalue = "filter"; */
            forcedvalue = "직업";
        }
        break;
    case RS_RACE:
        /*KR what = "race"; */
        what = "종족";
        f = flags.initrace;
        c = ROLE_NONE; /* override player's setting */
        if (r >= 0) {
            allowmask = roles[r].allow & ROLE_RACEMASK;
            if (allowmask == MH_HUMAN)
                c = 0; /* races[human] */
            if (c >= 0) {
                /*KR constrainer = "role"; */
                constrainer = "직업";
                forcedvalue = races[c].noun;
            } else if (f >= 0
                       && (allowmask & ~rfilter.mask) == races[f].selfmask) {
                /* if there is only one race choice available due to user
                   options disallowing others, race menu entry is disabled */
                /*KR constrainer = "filter"; */
                constrainer = "필터";
                /*KR forcedvalue = "race"; */
                forcedvalue = "종족";
            }
        }
        break;
    case RS_GENDER:
        /*KR what = "gender"; */
        what = "성별";
        f = flags.initgend;
        g = ROLE_NONE;
        if (r >= 0) {
            allowmask = roles[r].allow & ROLE_GENDMASK;
            if (allowmask == ROLE_MALE)
                g = 0; /* genders[male] */
            else if (allowmask == ROLE_FEMALE)
                g = 1; /* genders[female] */
            if (g >= 0) {
                /*KR constrainer = "role"; */
                constrainer = "직업";
                forcedvalue = genders[g].adj;
            } else if (f >= 0
                       && (allowmask & ~rfilter.mask) == genders[f].allow) {
                /* if there is only one gender choice available due to user
                   options disallowing other, gender menu entry is disabled */
                /*KR constrainer = "filter"; */
                constrainer = "필터";
                /*KR forcedvalue = "gender"; */
                forcedvalue = "성별";
            }
        }
        break;
    case RS_ALGNMNT:
        /*KR what = "alignment"; */
        what = "성향";
        f = flags.initalign;
        a = ROLE_NONE;
        if (r >= 0) {
            allowmask = roles[r].allow & ROLE_ALIGNMASK;
            if (allowmask == AM_LAWFUL)
                a = 0; /* aligns[lawful] */
            else if (allowmask == AM_NEUTRAL)
                a = 1; /* aligns[neutral] */
            else if (allowmask == AM_CHAOTIC)
                a = 2; /* aligns[chaotic] */
            if (a >= 0)
                /*KR constrainer = "role"; */
                constrainer = "직업";
        }
        if (c >= 0 && !constrainer) {
            allowmask = races[c].allow & ROLE_ALIGNMASK;
            if (allowmask == AM_LAWFUL)
                a = 0; /* aligns[lawful] */
            else if (allowmask == AM_NEUTRAL)
                a = 1; /* aligns[neutral] */
            else if (allowmask == AM_CHAOTIC)
                a = 2; /* aligns[chaotic] */
            if (a >= 0)
                /*KR constrainer = "race"; */
                constrainer = "종족";
        }
        if (f >= 0 && !constrainer
            && (ROLE_ALIGNMASK & ~rfilter.mask) == aligns[f].allow) {
            /* if there is only one alignment choice available due to user
               options disallowing others, algn menu entry is disabled */
            /*KR constrainer = "filter"; */
            constrainer = "필터";
            /*KR forcedvalue = "alignment"; */
            forcedvalue = "성향";
        }
        if (a >= 0)
#if 0 /*KR: '일본어에서는 명사가 자동' 이라는 주석이 있음 */
            forcedvalue = aligns[a].adj;
#else
            forcedvalue = aligns[a].noun;
#endif
        break;
    }

    any = zeroany; /* zero out all bits */
    if (constrainer) {
        any.a_int = 0;
        /* use four spaces of padding to fake a grayed out menu choice */
        /*KR Sprintf(buf, "%4s%s forces %s", "", constrainer, forcedvalue); */
        Sprintf(buf, "%4s 이 %s로는 %s뿐입니다", "", constrainer, forcedvalue);
        add_menu(where, NO_GLYPH, &any, 0, 0, ATR_NONE, buf,
                 MENU_UNSELECTED);
    } else if (what) {
        any.a_int = RS_menu_arg(which);
        /*KR Sprintf(buf, "Pick%s %s first", (f >= 0) ? " another" : "", what); */
        Sprintf(buf, "%s %s를 먼저 고른다", (f >= 0) ? "다른" : "", what);
        add_menu(where, NO_GLYPH, &any, RS_menu_let[which], 0, ATR_NONE, buf,
                 MENU_UNSELECTED);
    } else if (which == RS_filter) {
        any.a_int = RS_menu_arg(RS_filter);
#if 0 /*KR:T*/
        add_menu(where, NO_GLYPH, &any, '~', 0, ATR_NONE,
                 "Reset role/race/&c filtering", MENU_UNSELECTED);
#else
        add_menu(where, NO_GLYPH, &any, '~', 0, ATR_NONE,
            "직업/종족 등의 필터를 초기화", MENU_UNSELECTED);
#endif
    } else if (which == ROLE_RANDOM) {
        any.a_int = ROLE_RANDOM;
#if 0 /*KR:T*/
        add_menu(where, NO_GLYPH, &any, '*', 0, ATR_NONE, "Random",
                 preselect ? MENU_SELECTED : MENU_UNSELECTED);
#else
        add_menu(where, NO_GLYPH, &any, '*', 0, ATR_NONE, "무작위",
                 MENU_UNSELECTED);
#endif
    } else if (which == ROLE_NONE) {
        any.a_int = ROLE_NONE;
#if 0 /*KR:T*/
        add_menu(where, NO_GLYPH, &any, 'q', 0, ATR_NONE, "Quit",
                 preselect ? MENU_SELECTED : MENU_UNSELECTED);
#else
        add_menu(where, NO_GLYPH, &any, 'q', 0, ATR_NONE, "종료",
            MENU_UNSELECTED);
#endif
    } else {
        impossible("role_menu_extra: bad arg (%d)", which);
    }
}

/*
 *      Special setup modifications here:
 *
 *      Unfortunately, this is going to have to be done
 *      on each newgame or restore, because you lose the permonst mods
 *      across a save/restore.  :-)
 *
 *      1 - The Rogue Leader is the Tourist Nemesis.
 *      2 - Priests start with a random alignment - convert the leader and
 *          guardians here.
 *      3 - Priests also get their of deities from a randomly chosen role.
 *      4 - [obsolete] Elves can have one of two different leaders,
 *          but can't work it out here because it requires hacking the
 *          level file data (see sp_lev.c).
 *
 * This code also replaces quest_init().
 */
void
role_init()
{
    int alignmnt;
    struct permonst *pm;

    /* Strip the role letter out of the player name.
     * This is included for backwards compatibility.
     */
    plnamesuffix();

    /* Check for a valid role.  Try flags.initrole first. */
    if (!validrole(flags.initrole)) {
        /* Try the player letter second */
        if ((flags.initrole = str2role(pl_character)) < 0)
            /* None specified; pick a random role */
            flags.initrole = randrole_filtered();
    }

    /* We now have a valid role index.  Copy the role name back. */
    /* This should become OBSOLETE */
    Strcpy(pl_character, roles[flags.initrole].name.m);
    pl_character[PL_CSIZ - 1] = '\0';

    /* Check for a valid race */
    if (!validrace(flags.initrole, flags.initrace))
        flags.initrace = randrace(flags.initrole);

    /* Check for a valid gender.  If new game, check both initgend
     * and female.  On restore, assume flags.female is correct. */
    if (flags.pantheon == -1) { /* new game */
        if (!validgend(flags.initrole, flags.initrace, flags.female))
            flags.female = !flags.female;
    }
    if (!validgend(flags.initrole, flags.initrace, flags.initgend))
        /* Note that there is no way to check for an unspecified gender. */
        flags.initgend = flags.female;

    /* Check for a valid alignment */
    if (!validalign(flags.initrole, flags.initrace, flags.initalign))
        /* Pick a random alignment */
        flags.initalign = randalign(flags.initrole, flags.initrace);
    alignmnt = aligns[flags.initalign].value;

    /* Initialize urole and urace */
    urole = roles[flags.initrole];
    urace = races[flags.initrace];

    /* Fix up the quest leader */
    if (urole.ldrnum != NON_PM) {
        pm = &mons[urole.ldrnum];
        pm->msound = MS_LEADER;
        pm->mflags2 |= (M2_PEACEFUL);
        pm->mflags3 |= M3_CLOSE;
        pm->maligntyp = alignmnt * 3;
        /* if gender is random, we choose it now instead of waiting
           until the leader monster is created */
        quest_status.ldrgend =
            is_neuter(pm) ? 2 : is_female(pm) ? 1 : is_male(pm)
                                                        ? 0
                                                        : (rn2(100) < 50);
    }

    /* Fix up the quest guardians */
    if (urole.guardnum != NON_PM) {
        pm = &mons[urole.guardnum];
        pm->mflags2 |= (M2_PEACEFUL);
        pm->maligntyp = alignmnt * 3;
    }

    /* Fix up the quest nemesis */
    if (urole.neminum != NON_PM) {
        pm = &mons[urole.neminum];
        pm->msound = MS_NEMESIS;
        pm->mflags2 &= ~(M2_PEACEFUL);
        pm->mflags2 |= (M2_NASTY | M2_STALK | M2_HOSTILE);
        pm->mflags3 &= ~(M3_CLOSE);
        pm->mflags3 |= M3_WANTSARTI | M3_WAITFORU;
        /* if gender is random, we choose it now instead of waiting
           until the nemesis monster is created */
        quest_status.nemgend = is_neuter(pm) ? 2 : is_female(pm) ? 1
                                   : is_male(pm) ? 0 : (rn2(100) < 50);
    }

    /* Fix up the god names */
    if (flags.pantheon == -1) {             /* new game */
        flags.pantheon = flags.initrole;    /* use own gods */
        while (!roles[flags.pantheon].lgod) /* unless they're missing */
            flags.pantheon = randrole(FALSE);
    }
    if (!urole.lgod) {
        urole.lgod = roles[flags.pantheon].lgod;
        urole.ngod = roles[flags.pantheon].ngod;
        urole.cgod = roles[flags.pantheon].cgod;
    }
    /* 0 or 1; no gods are neuter, nor is gender randomized */
    /*KR quest_status.godgend = !strcmpi(align_gtitle(alignmnt), "goddess"); */
    quest_status.godgend = !strcmpi(align_gtitle(alignmnt), "여신");

#if 0
/*
 * Disable this fixup so that mons[] can be const.  The only
 * place where it actually matters for the hero is in set_uasmon()
 * and that can use mons[race] rather than mons[role] for this
 * particular property.  Despite the comment, it is checked--where
 * needed--via instrinsic 'Infravision' which set_uasmon() manages.
 */
    /* Fix up infravision */
    if (mons[urace.malenum].mflags3 & M3_INFRAVISION) {
        /* although an infravision intrinsic is possible, infravision
         * is purely a property of the physical race.  This means that we
         * must put the infravision flag in the player's current race
         * (either that or have separate permonst entries for
         * elven/non-elven members of each class).  The side effect is that
         * all NPCs of that class will have (probably bogus) infravision,
         * but since infravision has no effect for NPCs anyway we can
         * ignore this.
         */
        mons[urole.malenum].mflags3 |= M3_INFRAVISION;
        if (urole.femalenum != NON_PM)
            mons[urole.femalenum].mflags3 |= M3_INFRAVISION;
    }
#endif /*0*/

    /* Artifacts are fixed in hack_artifacts() */

    /* Success! */
    return;
}

const char *
Hello(mtmp)
struct monst *mtmp;
{
#if 0 /*KR*/
    switch (Role_switch) {
    case PM_KNIGHT:
        return "Salutations"; /* Olde English */
    case PM_SAMURAI:
        return (mtmp && mtmp->data == &mons[PM_SHOPKEEPER])
            ? "Irasshaimase"
            : "Konnichi wa"; /* Japanese */
    case PM_TOURIST:
        return "Aloha"; /* Hawaiian */
    case PM_VALKYRIE:
        return
#ifdef MAIL
        (mtmp && mtmp->data == &mons[PM_MAIL_DAEMON]) ? "Hallo" :
#endif
            "Velkommen"; /* Norse */
    default:
        return "Hello";
    }
#else
    /* 인사는 한국어로써 자연스러워지도록 대폭 변형  */
    static char helo_buf[BUFSZ];

    switch (Role_switch) {
    case PM_KNIGHT:
        Sprintf(helo_buf, "어서 오시게, %s여", plname);
        break;
    case PM_SAMURAI:
        Sprintf(helo_buf, "어서 오시게나, %s여", plname);
        break;
    case PM_TOURIST:
        Sprintf(helo_buf, "알로하, %s!", plname);
        break;
    case PM_VALKYRIE:
        Sprintf(helo_buf, "어서 오게나, %s", plname);
        break;
    default:
        Sprintf(helo_buf, "어서 오세요, %s", plname);
        break;
    }
    return helo_buf;
#endif
}

const char *
Goodbye()
{
#if 0 /*KR*/
    switch (Role_switch) {
    case PM_KNIGHT:
        return "Fare thee well"; /* Olde English */
    case PM_SAMURAI:
        return "Sayonara"; /* Japanese */
    case PM_TOURIST:
        return "Aloha"; /* Hawaiian */
    case PM_VALKYRIE:
        return "Farvel"; /* Norse */
    default:
        return "Goodbye";
    }
#else
    static char helo_buf[BUFSZ];

    switch (Role_switch) {
    case PM_KNIGHT:
        Sprintf(helo_buf, "안녕히 가시게");
        break;
    case PM_SAMURAI:
        Sprintf(helo_buf, "안녕히 가시게나");
        break;
    case PM_TOURIST:
        Sprintf(helo_buf, "알로하");
        break;
    case PM_VALKYRIE:
        Sprintf(helo_buf, "안녕히 가게나");
        break;
    default:
        Sprintf(helo_buf, "안녕히 가세요");
        break;
    }
    return helo_buf;
#endif
}

/* role.c */
