#include "hack.h"
#include <windows.h>
#include <stdio.h>

/* 1. 문자열의 마지막 글자 '받침(종성)' 코드를 알아내는 함수 */
int
get_jongseong(const char *str)
{
    int len, wlen;
    wchar_t *wstr;
    wchar_t last_char;
    int jongseong = 0;

    if (!str || !*str)
        return 0;

    /* 1) 문자열을 윈도우 유니코드(UTF-16)로 임시 변환하여 마지막 글자를
     * 추출합니다. */
    len = strlen(str);
    wlen = MultiByteToWideChar(CP_ACP, 0, str, len, NULL, 0);
    if (wlen == 0)
        return 0;

    wstr = (wchar_t *) malloc(wlen * sizeof(wchar_t));
    MultiByteToWideChar(CP_ACP, 0, str, len, wstr, wlen);

    last_char = wstr[wlen - 1]; /* 문자열의 진짜 마지막 한 글자 */
    free(wstr);

    /* 2) 마지막 글자가 한글인지 확인하고 받침 공식을 적용합니다. */
    if (last_char >= 0xAC00 && last_char <= 0xD7A3) {
        jongseong = (last_char - 0xAC00) % 28;
    }
    /* (옵션) 숫자 1, 3, 6, 7, 8 등 받침 소리가 나는 숫자 예외 처리 */
    else if (last_char >= '0' && last_char <= '9') {
        if (last_char == '1' || last_char == '7' || last_char == '8')
            jongseong = 8; // 'ㄹ' 받침 소리
        else if (last_char == '0' || last_char == '3' || last_char == '6')
            jongseong = 1; // 일반 받침 소리
    }

    return jongseong; /* 0이면 받침 없음, 8이면 'ㄹ' 받침, 그 외는 일반 받침
                       */
}

/* 2. 단어 뒤에 알맞은 조사를 자동으로 붙여서 반환하는 함수 */
const char *
append_josa(const char *word, const char *josa)
{
    /* * 주의: pline("%s %s", append_josa(A), append_josa(B)) 처럼 한 줄에
     * 여러 번 호출될 때 버퍼가 덮어씌워지는 것을 막기 위해 4개의 순환
     * 버퍼(Circular Buffer)를 사용합니다.
     */
    static char bufs[4][256];
    static int buf_idx = 0;
    char *buf;
    int jong, has_jong, is_rieul;
    const char *j = "";

    buf_idx = (buf_idx + 1) % 4;
    buf = bufs[buf_idx];

    if (!word || !*word)
        return "";

    jong = get_jongseong(word);
    has_jong = (jong > 0);
    is_rieul = (jong == 8); /* 'ㄹ' 받침 (예: 활, 칼) */

    /* 입력된 조사 형태에 따라 알맞은 짝을 찾습니다. */
    if (strstr(josa, "은") || strstr(josa, "는"))
        j = has_jong ? "은" : "는";
    else if (strstr(josa, "이") || strstr(josa, "가"))
        j = has_jong ? "이" : "가";
    else if (strstr(josa, "을") || strstr(josa, "를"))
        j = has_jong ? "을" : "를";
    else if (strstr(josa, "과") || strstr(josa, "와"))
        j = has_jong ? "과" : "와";
    else if (strstr(josa, "으로") || strstr(josa, "로"))
        /* '으로/로'는 특이하게 'ㄹ' 받침일 때는 받침이 없는 것과 똑같이
         * '로'를 씁니다. (예: 칼로, 검으로) */
        j = (has_jong && !is_rieul) ? "으로" : "로";
    else
        j = josa; /* 알 수 없는 조사면 그냥 그대로 붙임 */

    snprintf(buf, 256, "%s%s", word, j);
    return buf;
}

/* 1. 현재 바이트가 몇 바이트짜리 문자의 시작인지 판별하는 함수 (UTF-8 규칙)
 */
int
utf8_char_bytes(unsigned char c)
{
    if (c < 0x80)
        return 1; // 1바이트 (영어/숫자)
    if ((c & 0xE0) == 0xC0)
        return 2; // 2바이트 (유럽어 등)
    if ((c & 0xF0) == 0xE0)
        return 3; // 3바이트 (한글/한자/일본어)
    if ((c & 0xF8) == 0xF0)
        return 4; // 4바이트 (이모지 등)
    return 1;     // 알 수 없는 경우 1바이트 처리
}

/* 2. 문자열의 '바이트 수'가 아닌 '화면 출력 칸 수(Display Width)'를 계산하는
   함수 원본 넷핵의 strlen()을 대체하게 됩니다. */
int
korean_strwidth(const char *str)
{
    int width = 0;
    unsigned const char *p = (unsigned const char *) str;

    while (*p) {
        if (*p < 0x80) {
            width += 1; // 영어나 띄어쓰기는 1칸
            p += 1;
        } else if ((*p & 0xE0) == 0xC0) {
            width += 1; // 2바이트 문자도 보통 화면에선 1칸
            p += 2;
        } else if ((*p & 0xF0) == 0xE0) {
            width += 2; // 한글(3바이트)은 화면에서 2칸(전각)을 차지함
            p += 3;
        } else if ((*p & 0xF8) == 0xF0) {
            width += 2; // 특수문자/이모지도 2칸
            p += 4;
        } else {
            p++; // 에러 방지용 전진
        }
    }
    return width;
}

/* --- KRNethack 번역 사전 시작 --- */
/* 번역 사전 함수 (여기에 단어를 계속 추가하시면 됩니다) */
char *
get_kr_name(const char *en_name)
{
    if (!en_name)
        return (char *) en_name;

    /* 몬스터 */
    /* ========================================== *
     * 몬스터 번역 사전              *
     * ========================================== */

    /* [개미류 - Ants] */
    if (!strcmp(en_name, "giant ant"))
        return "거대 개미";
    if (!strcmp(en_name, "killer bee"))
        return "살인벌";
    if (!strcmp(en_name, "soldier ant"))
        return "병정개미";
    if (!strcmp(en_name, "fire ant"))
        return "불개미";
    if (!strcmp(en_name, "giant beetle"))
        return "거대 딱정벌레";
    if (!strcmp(en_name, "queen bee"))
        return "여왕벌";

    /* [방울류 - Blobs] */
    if (!strcmp(en_name, "acid blob"))
        return "산성 방울";
    if (!strcmp(en_name, "quivering blob"))
        return "진동하는 방울";
    if (!strcmp(en_name, "gelatinous cube"))
        return "젤라틴 큐브";

    /* [코카트리스 - Cockatrices] */
    if (!strcmp(en_name, "chickatrice"))
        return "치카트리스";
    if (!strcmp(en_name, "cockatrice"))
        return "코카트리스";
    if (!strcmp(en_name, "pyrolisk"))
        return "파이로리스크";

    /* [개과 동물 - Dogs & Canines] */
    if (!strcmp(en_name, "jackal"))
        return "자칼";
    if (!strcmp(en_name, "fox"))
        return "여우";
    if (!strcmp(en_name, "coyote"))
        return "코요테";
    if (!strcmp(en_name, "werejackal"))
        return "자칼인간";
    if (!strcmp(en_name, "little dog"))
        return "작은 개";
    if (!strcmp(en_name, "dingo"))
        return "딩고";
    if (!strcmp(en_name, "dog"))
        return "개";
    if (!strcmp(en_name, "large dog"))
        return "커다란 개";
    if (!strcmp(en_name, "wolf"))
        return "늑대";
    if (!strcmp(en_name, "werewolf"))
        return "늑대인간";
    if (!strcmp(en_name, "winter wolf cub"))
        return "새끼 겨울늑대";
    if (!strcmp(en_name, "warg"))
        return "와르그";
    if (!strcmp(en_name, "winter wolf"))
        return "겨울늑대";
    if (!strcmp(en_name, "hell hound pup"))
        return "새끼 지옥견";
    if (!strcmp(en_name, "hell hound"))
        return "지옥견";
    if (!strcmp(en_name, "Cerberus"))
        return "케르베로스";

    /* [눈알류 - Eyes] */
    if (!strcmp(en_name, "gas spore"))
        return "가스 포자";
    if (!strcmp(en_name, "floating eye"))
        return "떠다니는 눈알";
    if (!strcmp(en_name, "freezing sphere"))
        return "동결 구체";
    if (!strcmp(en_name, "flaming sphere"))
        return "화염 구체";
    if (!strcmp(en_name, "shocking sphere"))
        return "전격 구체";
    if (!strcmp(en_name, "beholder"))
        return "비홀더";

    /* [고양이과 동물 - Felines] */
    if (!strcmp(en_name, "kitten"))
        return "새끼 고양이";
    if (!strcmp(en_name, "housecat"))
        return "집고양이";
    if (!strcmp(en_name, "jaguar"))
        return "재규어";
    if (!strcmp(en_name, "lynx"))
        return "스라소니";
    if (!strcmp(en_name, "panther"))
        return "표범";
    if (!strcmp(en_name, "large cat"))
        return "커다란 고양이";
    if (!strcmp(en_name, "tiger"))
        return "호랑이";

    /* [그렘린과 가고일 - Gremlins & Gargoyles] */
    if (!strcmp(en_name, "gremlin"))
        return "그렘린";
    if (!strcmp(en_name, "gargoyle"))
        return "가고일";
    if (!strcmp(en_name, "winged gargoyle"))
        return "날개달린 가고일";

    /* [인간형 - Humanoids] */
    if (!strcmp(en_name, "hobbit"))
        return "호빗";
    if (!strcmp(en_name, "dwarf"))
        return "드워프";
    if (!strcmp(en_name, "bugbear"))
        return "버그베어";
    if (!strcmp(en_name, "dwarf lord"))
        return "드워프 군주";
    if (!strcmp(en_name, "dwarf king"))
        return "드워프 왕";
    if (!strcmp(en_name, "mind flayer"))
        return "정신 강탈자";
    if (!strcmp(en_name, "master mind flayer"))
        return "우두머리 정신 강탈자";

    /* [임프와 악마 - Imps & Demons] */
    if (!strcmp(en_name, "manes"))
        return "마네스";
    if (!strcmp(en_name, "homunculus"))
        return "호문클루스";
    if (!strcmp(en_name, "imp"))
        return "임프";
    if (!strcmp(en_name, "lemure"))
        return "레뮤르";
    if (!strcmp(en_name, "quasit"))
        return "콰짓";
    if (!strcmp(en_name, "tengu"))
        return "텐구";

    /* [젤리류 - Jellies] */
    if (!strcmp(en_name, "blue jelly"))
        return "파란 젤리";
    if (!strcmp(en_name, "spotted jelly"))
        return "점박이 젤리";
    if (!strcmp(en_name, "ochre jelly"))
        return "황토색 젤리";

    /* [코볼트 - Kobolds] */
    if (!strcmp(en_name, "kobold"))
        return "코볼트";
    if (!strcmp(en_name, "large kobold"))
        return "커다란 코볼트";
    if (!strcmp(en_name, "kobold lord"))
        return "코볼트 군주";
    if (!strcmp(en_name, "kobold shaman"))
        return "코볼트 주술사";

    /* [레프리콘 - Leprechauns] */
    if (!strcmp(en_name, "leprechaun"))
        return "레프리콘";

    /* [미믹 - Mimics] */
    if (!strcmp(en_name, "small mimic"))
        return "작은 미믹";
    if (!strcmp(en_name, "large mimic"))
        return "커다란 미믹";
    if (!strcmp(en_name, "giant mimic"))
        return "거대한 미믹";

    /* [님프 - Nymphs] */
    if (!strcmp(en_name, "wood nymph"))
        return "나무 님프";
    if (!strcmp(en_name, "water nymph"))
        return "물 님프";
    if (!strcmp(en_name, "mountain nymph"))
        return "산 님프";

    /* [오크 - Orcs] */
    if (!strcmp(en_name, "goblin"))
        return "고블린";
    if (!strcmp(en_name, "hobgoblin"))
        return "홉고블린";
    if (!strcmp(en_name, "orc"))
        return "오크";
    if (!strcmp(en_name, "hill orc"))
        return "언덕 오크";
    if (!strcmp(en_name, "Mordor orc"))
        return "모르도르 오크";
    if (!strcmp(en_name, "Uruk-hai"))
        return "우르크하이";
    if (!strcmp(en_name, "orc shaman"))
        return "오크 주술사";
    if (!strcmp(en_name, "orc-captain"))
        return "오크 대장";

    /* [피어서 - Piercers] */
    if (!strcmp(en_name, "rock piercer"))
        return "바위 피어서";
    if (!strcmp(en_name, "iron piercer"))
        return "철 피어서";
    if (!strcmp(en_name, "glass piercer"))
        return "유리 피어서";

    /* [네발짐승 - Quadrupeds] */
    if (!strcmp(en_name, "rothe"))
        return "로스";
    if (!strcmp(en_name, "mumak"))
        return "무막";
    if (!strcmp(en_name, "leocrotta"))
        return "레오크로타";
    if (!strcmp(en_name, "wumpus"))
        return "움푸스";
    if (!strcmp(en_name, "titanothere"))
        return "티타노테어";
    if (!strcmp(en_name, "baluchitherium"))
        return "발루키테리움";
    if (!strcmp(en_name, "mastodon"))
        return "마스토돈";

    /* [설치류 - Rodents] */
    if (!strcmp(en_name, "sewer rat"))
        return "하수구 쥐";
    if (!strcmp(en_name, "giant rat"))
        return "거대 쥐";
    if (!strcmp(en_name, "rabid rat"))
        return "광견병 쥐";
    if (!strcmp(en_name, "wererat"))
        return "쥐인간";
    if (!strcmp(en_name, "rock mole"))
        return "바위 두더지";
    if (!strcmp(en_name, "woodchuck"))
        return "우드척";

    /* [거미류 - Spiders] */
    if (!strcmp(en_name, "cave spider"))
        return "동굴 거미";
    if (!strcmp(en_name, "centipede"))
        return "지네";
    if (!strcmp(en_name, "giant spider"))
        return "거대 거미";
    if (!strcmp(en_name, "scorpion"))
        return "전갈";

    /* [함정 괴물 - Trappers & Lurkers] */
    if (!strcmp(en_name, "lurker above"))
        return "천장의 럴커";
    if (!strcmp(en_name, "trapper"))
        return "트래퍼";

    /* [유니콘과 말 - Unicorns & Horses] */
    if (!strcmp(en_name, "pony"))
        return "조랑말";
    if (!strcmp(en_name, "white unicorn"))
        return "하얀 유니콘";
    if (!strcmp(en_name, "gray unicorn"))
        return "회색 유니콘";
    if (!strcmp(en_name, "black unicorn"))
        return "검은 유니콘";
    if (!strcmp(en_name, "horse"))
        return "말";
    if (!strcmp(en_name, "warhorse"))
        return "군마";

    /* [소용돌이 - Vortices] */
    if (!strcmp(en_name, "fog cloud"))
        return "안개 구름";
    if (!strcmp(en_name, "dust vortex"))
        return "먼지 소용돌이";
    if (!strcmp(en_name, "ice vortex"))
        return "얼음 소용돌이";
    if (!strcmp(en_name, "energy vortex"))
        return "에너지 소용돌이";
    if (!strcmp(en_name, "steam vortex"))
        return "증기 소용돌이";
    if (!strcmp(en_name, "fire vortex"))
        return "화염 소용돌이";

    /* [지렁이 - Worms] */
    if (!strcmp(en_name, "baby long worm"))
        return "새끼 긴 지렁이";
    if (!strcmp(en_name, "baby purple worm"))
        return "새끼 보라색 지렁이";
    if (!strcmp(en_name, "long worm"))
        return "긴 지렁이";
    if (!strcmp(en_name, "purple worm"))
        return "보라색 지렁이";

    /* [기타 곤충 - Xan, &c] */
    if (!strcmp(en_name, "grid bug"))
        return "격자 벌레";
    if (!strcmp(en_name, "xan"))
        return "잔";

    /* [빛 무리 - Lights] */
    if (!strcmp(en_name, "yellow light"))
        return "노란 빛";
    if (!strcmp(en_name, "black light"))
        return "검은 빛";

    /* [즈루티 - Zruty] */
    if (!strcmp(en_name, "zruty"))
        return "즈루티";

    /* [천사 - Angels] */
    if (!strcmp(en_name, "couatl"))
        return "코아틀";
    if (!strcmp(en_name, "Aleax"))
        return "알레악스";
    if (!strcmp(en_name, "Angel"))
        return "천사";
    if (!strcmp(en_name, "ki-rin"))
        return "기린";
    if (!strcmp(en_name, "Archon"))
        return "아콘";

    /* [박쥐 - Bats] */
    if (!strcmp(en_name, "bat"))
        return "박쥐";
    if (!strcmp(en_name, "giant bat"))
        return "거대 박쥐";
    if (!strcmp(en_name, "raven"))
        return "까마귀";
    if (!strcmp(en_name, "vampire bat"))
        return "흡혈 박쥐";

    /* [켄타우로스 - Centaurs] */
    if (!strcmp(en_name, "plains centaur"))
        return "평원 켄타우로스";
    if (!strcmp(en_name, "forest centaur"))
        return "숲 켄타우로스";
    if (!strcmp(en_name, "mountain centaur"))
        return "산 켄타우로스";

    /* [용 - Dragons] */
    if (!strcmp(en_name, "baby gray dragon"))
        return "새끼 회색 용";
    if (!strcmp(en_name, "baby silver dragon"))
        return "새끼 은색 용";
    if (!strcmp(en_name, "baby shimmering dragon"))
        return "새끼 일렁이는 용";
    if (!strcmp(en_name, "baby red dragon"))
        return "새끼 붉은색 용";
    if (!strcmp(en_name, "baby white dragon"))
        return "새끼 하얀색 용";
    if (!strcmp(en_name, "baby orange dragon"))
        return "새끼 주황색 용";
    if (!strcmp(en_name, "baby black dragon"))
        return "새끼 검은색 용";
    if (!strcmp(en_name, "baby blue dragon"))
        return "새끼 푸른색 용";
    if (!strcmp(en_name, "baby green dragon"))
        return "새끼 초록색 용";
    if (!strcmp(en_name, "baby yellow dragon"))
        return "새끼 노란색 용";
    if (!strcmp(en_name, "gray dragon"))
        return "회색 용";
    if (!strcmp(en_name, "silver dragon"))
        return "은색 용";
    if (!strcmp(en_name, "shimmering dragon"))
        return "일렁이는 용";
    if (!strcmp(en_name, "red dragon"))
        return "붉은색 용";
    if (!strcmp(en_name, "white dragon"))
        return "하얀색 용";
    if (!strcmp(en_name, "orange dragon"))
        return "주황색 용";
    if (!strcmp(en_name, "black dragon"))
        return "검은색 용";
    if (!strcmp(en_name, "blue dragon"))
        return "푸른색 용";
    if (!strcmp(en_name, "green dragon"))
        return "초록색 용";
    if (!strcmp(en_name, "yellow dragon"))
        return "노란색 용";

    /* [원소 - Elementals] */
    if (!strcmp(en_name, "stalker"))
        return "스토커";
    if (!strcmp(en_name, "air elemental"))
        return "공기 원소";
    if (!strcmp(en_name, "fire elemental"))
        return "화염 원소";
    if (!strcmp(en_name, "earth elemental"))
        return "대지 원소";
    if (!strcmp(en_name, "water elemental"))
        return "물 원소";

    /* [균류 - Fungi] */
    if (!strcmp(en_name, "lichen"))
        return "이끼";
    if (!strcmp(en_name, "brown mold"))
        return "갈색 곰팡이";
    if (!strcmp(en_name, "yellow mold"))
        return "노란 곰팡이";
    if (!strcmp(en_name, "green mold"))
        return "초록 곰팡이";
    if (!strcmp(en_name, "red mold"))
        return "붉은 곰팡이";
    if (!strcmp(en_name, "shrieker"))
        return "쉬리커";
    if (!strcmp(en_name, "violet fungus"))
        return "보라색 곰팡이";

    /* [노움 - Gnomes] */
    if (!strcmp(en_name, "gnome"))
        return "노움";
    if (!strcmp(en_name, "gnome lord"))
        return "노움 군주";
    if (!strcmp(en_name, "gnomish wizard"))
        return "노움 마법사";
    if (!strcmp(en_name, "gnome king"))
        return "노움 왕";

    /* [거인 - Giants] */
    if (!strcmp(en_name, "giant"))
        return "거인";
    if (!strcmp(en_name, "stone giant"))
        return "바위 거인";
    if (!strcmp(en_name, "hill giant"))
        return "언덕 거인";
    if (!strcmp(en_name, "fire giant"))
        return "화염 거인";
    if (!strcmp(en_name, "frost giant"))
        return "서리 거인";
    if (!strcmp(en_name, "ettin"))
        return "에틴";
    if (!strcmp(en_name, "storm giant"))
        return "폭풍 거인";
    if (!strcmp(en_name, "titan"))
        return "타이탄";
    if (!strcmp(en_name, "minotaur"))
        return "미노타우로스";
    if (!strcmp(en_name, "jabberwock"))
        return "재버워크";
    if (!strcmp(en_name, "vorpal jabberwock"))
        return "보팔 재버워크";

    /* [경찰 - Kops] */
    if (!strcmp(en_name, "Keystone Kop"))
        return "키스톤 경찰";
    if (!strcmp(en_name, "Kop Sergeant"))
        return "경찰 경사";
    if (!strcmp(en_name, "Kop Lieutenant"))
        return "경찰 부관";
    if (!strcmp(en_name, "Kop Kaptain"))
        return "경찰 서장";

    /* [리치 - Liches] */
    if (!strcmp(en_name, "lich"))
        return "리치";
    if (!strcmp(en_name, "demilich"))
        return "데미리치";
    if (!strcmp(en_name, "master lich"))
        return "마스터 리치";
    if (!strcmp(en_name, "arch-lich"))
        return "아크 리치";

    /* [미라 - Mummies] */
    if (!strcmp(en_name, "kobold mummy"))
        return "코볼트 미라";
    if (!strcmp(en_name, "gnome mummy"))
        return "노움 미라";
    if (!strcmp(en_name, "orc mummy"))
        return "오크 미라";
    if (!strcmp(en_name, "dwarf mummy"))
        return "드워프 미라";
    if (!strcmp(en_name, "elf mummy"))
        return "엘프 미라";
    if (!strcmp(en_name, "human mummy"))
        return "인간 미라";
    if (!strcmp(en_name, "ettin mummy"))
        return "에틴 미라";
    if (!strcmp(en_name, "giant mummy"))
        return "거인 미라";

    /* [나가 - Nagas] */
    if (!strcmp(en_name, "red naga hatchling"))
        return "붉은 나가 유체";
    if (!strcmp(en_name, "black naga hatchling"))
        return "검은 나가 유체";
    if (!strcmp(en_name, "golden naga hatchling"))
        return "황금 나가 유체";
    if (!strcmp(en_name, "guardian naga hatchling"))
        return "수호자 나가 유체";
    if (!strcmp(en_name, "red naga"))
        return "붉은 나가";
    if (!strcmp(en_name, "black naga"))
        return "검은 나가";
    if (!strcmp(en_name, "golden naga"))
        return "황금 나가";
    if (!strcmp(en_name, "guardian naga"))
        return "수호자 나가";

    /* [오우거 - Ogres] */
    if (!strcmp(en_name, "ogre"))
        return "오우거";
    if (!strcmp(en_name, "ogre lord"))
        return "오우거 군주";
    if (!strcmp(en_name, "ogre king"))
        return "오우거 왕";

    /* [푸딩 - Puddings] */
    if (!strcmp(en_name, "gray ooze"))
        return "회색 점액";
    if (!strcmp(en_name, "brown pudding"))
        return "갈색 푸딩";
    if (!strcmp(en_name, "green slime"))
        return "초록 슬라임";
    if (!strcmp(en_name, "black pudding"))
        return "검은 푸딩";

    /* [기타 특수 - Others] */
    if (!strcmp(en_name, "quantum mechanic"))
        return "양자역학자";
    if (!strcmp(en_name, "rust monster"))
        return "녹 괴물";
    if (!strcmp(en_name, "disenchanter"))
        return "디스인챈터";

    /* [뱀 - Snakes] */
    if (!strcmp(en_name, "garter snake"))
        return "얼룩뱀";
    if (!strcmp(en_name, "snake"))
        return "뱀";
    if (!strcmp(en_name, "water moccasin"))
        return "물뱀";
    if (!strcmp(en_name, "python"))
        return "비단뱀";
    if (!strcmp(en_name, "pit viper"))
        return "살무사";
    if (!strcmp(en_name, "cobra"))
        return "코브라";

    /* [트롤 - Trolls] */
    if (!strcmp(en_name, "troll"))
        return "트롤";
    if (!strcmp(en_name, "ice troll"))
        return "얼음 트롤";
    if (!strcmp(en_name, "rock troll"))
        return "바위 트롤";
    if (!strcmp(en_name, "water troll"))
        return "물 트롤";
    if (!strcmp(en_name, "Olog-hai"))
        return "올로그하이";
    if (!strcmp(en_name, "umber hulk"))
        return "움버 헐크";

    /* [흡혈귀 - Vampires] */
    if (!strcmp(en_name, "vampire"))
        return "흡혈귀";
    if (!strcmp(en_name, "vampire lord"))
        return "흡혈귀 군주";
    if (!strcmp(en_name, "vampire mage"))
        return "흡혈귀 마법사";
    if (!strcmp(en_name, "Vlad the Impaler"))
        return "가시 공작 블라드";

    /* [망령 - Wraiths] */
    if (!strcmp(en_name, "barrow wight"))
        return "배로우 와이트";
    if (!strcmp(en_name, "wraith"))
        return "레이스";
    if (!strcmp(en_name, "Nazgul"))
        return "나즈굴";
    if (!strcmp(en_name, "xorn"))
        return "쏜";

    /* [유인원 - Apelike] */
    if (!strcmp(en_name, "monkey"))
        return "원숭이";
    if (!strcmp(en_name, "ape"))
        return "유인원";
    if (!strcmp(en_name, "owlbear"))
        return "아울베어";
    if (!strcmp(en_name, "yeti"))
        return "예티";
    if (!strcmp(en_name, "carnivorous ape"))
        return "육식 유인원";
    if (!strcmp(en_name, "sasquatch"))
        return "사스콰치";

    /* [좀비 - Zombies] */
    if (!strcmp(en_name, "kobold zombie"))
        return "코볼트 좀비";
    if (!strcmp(en_name, "gnome zombie"))
        return "노움 좀비";
    if (!strcmp(en_name, "orc zombie"))
        return "오크 좀비";
    if (!strcmp(en_name, "dwarf zombie"))
        return "드워프 좀비";
    if (!strcmp(en_name, "elf zombie"))
        return "엘프 좀비";
    if (!strcmp(en_name, "human zombie"))
        return "인간 좀비";
    if (!strcmp(en_name, "ettin zombie"))
        return "에틴 좀비";
    if (!strcmp(en_name, "ghoul"))
        return "구울";
    if (!strcmp(en_name, "giant zombie"))
        return "거인 좀비";
    if (!strcmp(en_name, "skeleton"))
        return "해골";

    /* [골렘 - Golems] */
    if (!strcmp(en_name, "straw golem"))
        return "밀짚 골렘";
    if (!strcmp(en_name, "paper golem"))
        return "종이 골렘";
    if (!strcmp(en_name, "rope golem"))
        return "밧줄 골렘";
    if (!strcmp(en_name, "gold golem"))
        return "황금 골렘";
    if (!strcmp(en_name, "leather golem"))
        return "가죽 골렘";
    if (!strcmp(en_name, "wood golem"))
        return "나무 골렘";
    if (!strcmp(en_name, "flesh golem"))
        return "살점 골렘";
    if (!strcmp(en_name, "clay golem"))
        return "점토 골렘";
    if (!strcmp(en_name, "stone golem"))
        return "돌 골렘";
    if (!strcmp(en_name, "glass golem"))
        return "유리 골렘";
    if (!strcmp(en_name, "iron golem"))
        return "철 골렘";

    /* [인간/엘프 등 - Humans & Elves] */
    if (!strcmp(en_name, "human"))
        return "인간";
    if (!strcmp(en_name, "elf"))
        return "엘프";
    if (!strcmp(en_name, "Woodland-elf"))
        return "숲 엘프";
    if (!strcmp(en_name, "Green-elf"))
        return "초록 엘프";
    if (!strcmp(en_name, "Grey-elf"))
        return "회색 엘프";
    if (!strcmp(en_name, "elf-lord"))
        return "엘프 군주";
    if (!strcmp(en_name, "Elvenking"))
        return "엘프 왕";
    if (!strcmp(en_name, "doppelganger"))
        return "도플겡어";
    if (!strcmp(en_name, "shopkeeper"))
        return "상점 주인";
    if (!strcmp(en_name, "guard"))
        return "경비병";
    if (!strcmp(en_name, "prisoner"))
        return "죄수";
    if (!strcmp(en_name, "Oracle"))
        return "현자";
    if (!strcmp(en_name, "aligned priest"))
        return "동종 사제";
    if (!strcmp(en_name, "high priest"))
        return "고위 사제";
    if (!strcmp(en_name, "soldier"))
        return "병사";
    if (!strcmp(en_name, "sergeant"))
        return "하사관";
    if (!strcmp(en_name, "nurse"))
        return "간호사";
    if (!strcmp(en_name, "lieutenant"))
        return "부관";
    if (!strcmp(en_name, "captain"))
        return "대장";
    if (!strcmp(en_name, "watchman"))
        return "경비대원";
    if (!strcmp(en_name, "watch captain"))
        return "경비대장";
    if (!strcmp(en_name, "Medusa"))
        return "메두사";
    if (!strcmp(en_name, "Wizard of Yendor"))
        return "옌더의 마법사";
    if (!strcmp(en_name, "Croesus"))
        return "크로이소스";
    if (!strcmp(en_name, "Charon"))
        return "카론";
    if (!strcmp(en_name, "ghost"))
        return "유령";
    if (!strcmp(en_name, "shade"))
        return "그림자 망령";

    /* [악마 - Demons (Unique/Bosses)] */
    if (!strcmp(en_name, "water demon"))
        return "물의 악마";
    if (!strcmp(en_name, "succubus"))
        return "서큐버스";
    if (!strcmp(en_name, "horned devil"))
        return "뿔난 악마";
    if (!strcmp(en_name, "incubus"))
        return "인큐버스";
    if (!strcmp(en_name, "erinys"))
        return "에리니에스";
    if (!strcmp(en_name, "barbed devil"))
        return "가시 돋친 악마";
    if (!strcmp(en_name, "marilith"))
        return "마릴리스";
    if (!strcmp(en_name, "vrock"))
        return "브록";
    if (!strcmp(en_name, "hezrou"))
        return "헤즈로우";
    if (!strcmp(en_name, "bone devil"))
        return "뼈 악마";
    if (!strcmp(en_name, "ice devil"))
        return "얼음 악마";
    if (!strcmp(en_name, "nalfeshnee"))
        return "날페쉬니";
    if (!strcmp(en_name, "pit fiend"))
        return "구덩이 악마";
    if (!strcmp(en_name, "sandestin"))
        return "산데스틴";
    if (!strcmp(en_name, "balrog"))
        return "발록";
    if (!strcmp(en_name, "Juiblex"))
        return "주블렉스";
    if (!strcmp(en_name, "Yeenoghu"))
        return "예노구";
    if (!strcmp(en_name, "Orcus"))
        return "오르쿠스";
    if (!strcmp(en_name, "Geryon"))
        return "게리온";
    if (!strcmp(en_name, "Dispater"))
        return "디스파테르";
    if (!strcmp(en_name, "Baalzebub"))
        return "바알제붑";
    if (!strcmp(en_name, "Asmodeus"))
        return "아스모데우스";
    if (!strcmp(en_name, "Demogorgon"))
        return "데모고르곤";
    if (!strcmp(en_name, "Death"))
        return "죽음";
    if (!strcmp(en_name, "Pestilence"))
        return "질병";
    if (!strcmp(en_name, "Famine"))
        return "기아";
    if (!strcmp(en_name, "mail daemon"))
        return "우편 데몬";
    if (!strcmp(en_name, "djinni"))
        return "지니";

    /* [해양 괴물 - Sea Monsters] */
    if (!strcmp(en_name, "jellyfish"))
        return "해파리";
    if (!strcmp(en_name, "piranha"))
        return "피라냐";
    if (!strcmp(en_name, "shark"))
        return "상어";
    if (!strcmp(en_name, "giant eel"))
        return "거대 뱀장어";
    if (!strcmp(en_name, "electric eel"))
        return "전기 뱀장어";
    if (!strcmp(en_name, "kraken"))
        return "크라켄";

    /* [기타 파충류 - Lizards] */
    if (!strcmp(en_name, "newt"))
        return "뉴트";
    if (!strcmp(en_name, "gecko"))
        return "게코";
    if (!strcmp(en_name, "iguana"))
        return "이구아나";
    if (!strcmp(en_name, "baby crocodile"))
        return "새끼 악어";
    if (!strcmp(en_name, "lizard"))
        return "도마뱀";
    if (!strcmp(en_name, "chameleon"))
        return "카멜레온";
    if (!strcmp(en_name, "crocodile"))
        return "악어";
    if (!strcmp(en_name, "salamander"))
        return "샐러맨더";
    if (!strcmp(en_name, "long worm tail"))
        return "긴 지렁이 꼬리";

    /* [플레이어 직업 - Classes] */
    if (!strcmp(en_name, "archeologist"))
        return "고고학자";
    if (!strcmp(en_name, "barbarian"))
        return "야만인";
    if (!strcmp(en_name, "caveman"))
        return "원시인 (남)";
    if (!strcmp(en_name, "cavewoman"))
        return "원시인 (여)";
    if (!strcmp(en_name, "healer"))
        return "치유사";
    if (!strcmp(en_name, "knight"))
        return "기사";
    if (!strcmp(en_name, "monk"))
        return "수도승";
    if (!strcmp(en_name, "priest"))
        return "사제 (남)";
    if (!strcmp(en_name, "priestess"))
        return "사제 (여)";
    if (!strcmp(en_name, "ranger"))
        return "레인저";
    if (!strcmp(en_name, "rogue"))
        return "도적";
    if (!strcmp(en_name, "samurai"))
        return "사무라이";
    if (!strcmp(en_name, "tourist"))
        return "관광객";
    if (!strcmp(en_name, "valkyrie"))
        return "발키리";
    if (!strcmp(en_name, "wizard"))
        return "마법사";

    /* [퀘스트 리더 & 적 - Quest] */
    if (!strcmp(en_name, "Lord Carnarvon"))
        return "카나본 경";
    if (!strcmp(en_name, "Pelias"))
        return "펠리아스";
    if (!strcmp(en_name, "Shaman Karnov"))
        return "주술사 카르노프";
    if (!strcmp(en_name, "Earendil"))
        return "에아렌딜";
    if (!strcmp(en_name, "Elwing"))
        return "엘윙";
    if (!strcmp(en_name, "Hippocrates"))
        return "히포크라테스";
    if (!strcmp(en_name, "King Arthur"))
        return "아서 왕";
    if (!strcmp(en_name, "Grand Master"))
        return "그랜드 마스터";
    if (!strcmp(en_name, "Arch Priest"))
        return "대사제";
    if (!strcmp(en_name, "Orion"))
        return "오리온";
    if (!strcmp(en_name, "Master of Thieves"))
        return "도적 마스터";
    if (!strcmp(en_name, "Lord Sato"))
        return "사토 영주";
    if (!strcmp(en_name, "Twoflower"))
        return "투플라워";
    if (!strcmp(en_name, "Norn"))
        return "노른";
    if (!strcmp(en_name, "Neferet the Green"))
        return "녹색의 네페레트";
    if (!strcmp(en_name, "Minion of Huhetotl"))
        return "후헤토틀의 하수인";
    if (!strcmp(en_name, "Thoth Amon"))
        return "토트 아몬";
    if (!strcmp(en_name, "Chromatic Dragon"))
        return "오색 용";
    if (!strcmp(en_name, "Goblin King"))
        return "고블린 왕";
    if (!strcmp(en_name, "Cyclops"))
        return "사이클롭스";
    if (!strcmp(en_name, "Ixoth"))
        return "익소스";
    if (!strcmp(en_name, "Master Kaen"))
        return "카엔 사범";
    if (!strcmp(en_name, "Nalzok"))
        return "날족";
    if (!strcmp(en_name, "Scorpius"))
        return "스콜피우스";
    if (!strcmp(en_name, "Master Assassin"))
        return "마스터 어새신";
    if (!strcmp(en_name, "Ashikaga Takauji"))
        return "아시카가 타카우지";
    if (!strcmp(en_name, "Lord Surtur"))
        return "수르트 영주";
    if (!strcmp(en_name, "Dark One"))
        return "다크 원";
    if (!strcmp(en_name, "student"))
        return "학생";
    if (!strcmp(en_name, "chieftain"))
        return "족장";
    if (!strcmp(en_name, "neanderthal"))
        return "네안데르탈인";
    if (!strcmp(en_name, "High-elf"))
        return "하이 엘프";
    if (!strcmp(en_name, "attendant"))
        return "수행원";
    if (!strcmp(en_name, "page"))
        return "시종";
    if (!strcmp(en_name, "abbot"))
        return "수도원장";
    if (!strcmp(en_name, "acolyte"))
        return "수사";
    if (!strcmp(en_name, "hunter"))
        return "사냥꾼";
    if (!strcmp(en_name, "thug"))
        return "폭력배";
    if (!strcmp(en_name, "ninja"))
        return "닌자";
    if (!strcmp(en_name, "roshi"))
        return "도사";
    if (!strcmp(en_name, "guide"))
        return "안내인";
    if (!strcmp(en_name, "warrior"))
        return "전사";
    if (!strcmp(en_name, "apprentice"))
        return "수습생";

    /* 무기 */
    if (!strcmp(en_name, "long sword"))
        return "롱 소드";
    if (!strcmp(en_name, "dagger"))
        return "단검";

    /* 물약 색상 (a색 물약 문제 해결용) */
    if (!strcmp(en_name, "ruby"))
        return "루비";
    if (!strcmp(en_name, "aquamarine"))
        return "아쿠아마린";

    /* 필요한 단어를 계속 추가하세요 */

    return (char *) en_name; /* 사전에 없으면 원래 영어 반환 */
}
/* --- KRNethack 번역 사전 끝 --- */