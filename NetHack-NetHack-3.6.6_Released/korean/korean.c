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
    /* '으로/로'는 특이하게 'ㄹ' 받침일 때는 받침이 없는 것과 똑같이
     * '로'를 씁니다. (예: 칼로, 검으로) */
    if (strstr(josa, "으로") || strstr(josa, "로")) j =
        (has_jong && !is_rieul) ? "으로" : "로";
    else if (strstr(josa, "이라고") || strstr(josa, "라고")) j =
        has_jong ? "이라고" : "라고";
    else if (strstr(josa, "이라는") || strstr(josa, "라는")) j =
        has_jong ? "이라는" : "라는";
    else if (strstr(josa, "이라") || strstr(josa, "라")) j =
        has_jong ? "이라" : "라";
    else if (strstr(josa, "이야") || strstr(josa, "야")) j =
        has_jong ? "이야" : "야";
    else if (strstr(josa, "이다") || strstr(josa, "다")) j =
        has_jong ? "이다" : "다";
    /* 짧은 기본 조사는 마지막에 검사 */
    else if (strstr(josa, "은") || strstr(josa, "는"))
        j = has_jong ? "은" : "는";
    else if (strstr(josa, "이") || strstr(josa, "가"))
        j = has_jong ? "이" : "가";
    else if (strstr(josa, "을") || strstr(josa, "를"))
        j = has_jong ? "을" : "를";
    else if (strstr(josa, "과") || strstr(josa, "와"))
        j = has_jong ? "과" : "와";
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

/* ========================================== *
     * 무기 (Weapons) 번역 사전
     * ========================================== */

    /* [단검류 - Daggers] */
    if (!strcmp(en_name, "dagger"))
        return "단검";
    if (!strcmp(en_name, "elven dagger"))
        return "엘프제 단검";
    if (!strcmp(en_name, "orcish dagger"))
        return "오크제 단검";
    if (!strcmp(en_name, "silver dagger"))
        return "은 단검";
    if (!strcmp(en_name, "athame"))
        return "아테임";
    if (!strcmp(en_name, "scalpel"))
        return "메스";

    /* [검류 - Swords] */
    if (!strcmp(en_name, "short sword"))
        return "짧은 검"; /* 숏 소드 */
    if (!strcmp(en_name, "elven short sword"))
        return "엘프제 짧은 검";
    if (!strcmp(en_name, "orcish short sword"))
        return "오크제 짧은 검";
    if (!strcmp(en_name, "dwarvish short sword"))
        return "드워프제 짧은 검";
    if (!strcmp(en_name, "wakizashi"))
        return "와키자시";
    if (!strcmp(en_name, "broadsword"))
        return "브로드소드";
    if (!strcmp(en_name, "elven broadsword"))
        return "엘프제 브로드소드";
    if (!strcmp(en_name, "long sword"))
        return "롱 소드";
    if (!strcmp(en_name, "two-handed sword"))
        return "양손검";
    if (!strcmp(en_name, "katana"))
        return "카타나";
    if (!strcmp(en_name, "tsurugi"))
        return "츠루기";
    if (!strcmp(en_name, "runesword"))
        return "룬소드";

    /* [도끼류 - Axes] */
    if (!strcmp(en_name, "axe"))
        return "도끼";
    if (!strcmp(en_name, "battle-axe"))
        return "전투도끼";

    /* [곡괭이류 - Pick-axes] */
    if (!strcmp(en_name, "pick-axe"))
        return "곡괭이";
    if (!strcmp(en_name, "dwarvish mattock"))
        return "드워프제 곡괭이";

    /* [둔기/곤봉류 - Maces & Clubs] */
    if (!strcmp(en_name, "club"))
        return "곤봉";
    if (!strcmp(en_name, "aklys"))
        return "아클리스";
    if (!strcmp(en_name, "mace"))
        return "메이스";
    if (!strcmp(en_name, "morning star"))
        return "모닝스타";
    if (!strcmp(en_name, "flail"))
        return "도리깨";
    if (!strcmp(en_name, "war hammer"))
        return "워해머";
    if (!strcmp(en_name, "quarterstaff"))
        return "육척봉";

    /* [장창/미늘창류 - Polearms] */
    if (!strcmp(en_name, "partisan"))
        return "파르티잔";
    if (!strcmp(en_name, "ranseur"))
        return "랜서";
    if (!strcmp(en_name, "spetum"))
        return "스페툼";
    if (!strcmp(en_name, "glaive"))
        return "글레이브";
    if (!strcmp(en_name, "lance"))
        return "랜스";
    if (!strcmp(en_name, "halberd"))
        return "할버드";
    if (!strcmp(en_name, "bardiche"))
        return "바디슈";
    if (!strcmp(en_name, "voulge"))
        return "불쥬";
    if (!strcmp(en_name, "fauchard"))
        return "포샤드";
    if (!strcmp(en_name, "guisarme"))
        return "기자름";
    if (!strcmp(en_name, "bill-guisarme"))
        return "빌기자름";
    if (!strcmp(en_name, "lucern hammer"))
        return "루체른 해머";
    if (!strcmp(en_name, "bec de corbin"))
        return "벡 드 코방";

    /* [창/투창류 - Spears & Javelins] */
    if (!strcmp(en_name, "spear"))
        return "창";
    if (!strcmp(en_name, "elven spear"))
        return "엘프제 창";
    if (!strcmp(en_name, "orcish spear"))
        return "오크제 창";
    if (!strcmp(en_name, "dwarvish spear"))
        return "드워프제 창";
    if (!strcmp(en_name, "silver spear"))
        return "은 창";
    if (!strcmp(en_name, "javelin"))
        return "투창";
    if (!strcmp(en_name, "trident"))
        return "삼지창";

    /* [활과 화살 - Bows & Arrows] */
    if (!strcmp(en_name, "bow"))
        return "활";
    if (!strcmp(en_name, "elven bow"))
        return "엘프제 활";
    if (!strcmp(en_name, "orcish bow"))
        return "오크제 활";
    if (!strcmp(en_name, "yumi"))
        return "유미";
    if (!strcmp(en_name, "arrow"))
        return "화살";
    if (!strcmp(en_name, "elven arrow"))
        return "엘프제 화살";
    if (!strcmp(en_name, "orcish arrow"))
        return "오크제 화살";
    if (!strcmp(en_name, "silver arrow"))
        return "은 화살";
    if (!strcmp(en_name, "ya"))
        return "야"; /* 일본식 화살 */

    /* [투척 무기류 - Missiles] */
    if (!strcmp(en_name, "sling"))
        return "새총";
    if (!strcmp(en_name, "crossbow"))
        return "석궁";
    if (!strcmp(en_name, "crossbow bolt"))
        return "석궁 볼트";
    if (!strcmp(en_name, "dart"))
        return "다트";
    if (!strcmp(en_name, "shuriken"))
        return "수리검";
    if (!strcmp(en_name, "boomerang"))
        return "부메랑";

    /* [기타 무기 - Whips etc.] */
    if (!strcmp(en_name, "bullwhip"))
        return "채찍";
    if (!strcmp(en_name, "silver saber"))
        return "은 사벨";
    if (!strcmp(en_name, "worm tooth"))
        return "벌레의 이빨";
    if (!strcmp(en_name, "crysknife"))
        return "크리스나이프";
    if (!strcmp(en_name, "rubber hose"))
        return "고무 호스";

    /* ========================================== *
     * 방어구 (Armor)
     * ========================================== */
    /* 셔츠 */
    if (!strcmp(en_name, "Hawaiian shirt"))
        return "하와이안 셔츠";
    if (!strcmp(en_name, "T-shirt"))
        return "티셔츠";

    /* 갑옷 */
    if (!strcmp(en_name, "leather armor"))
        return "가죽 갑옷";
    if (!strcmp(en_name, "orcish ring mail"))
        return "오크제 링 메일";
    if (!strcmp(en_name, "orcish chain mail"))
        return "오크제 사슬 갑옷";
    if (!strcmp(en_name, "studded leather armor"))
        return "스터디드 레더 아머";
    if (!strcmp(en_name, "ring mail"))
        return "링 메일";
    if (!strcmp(en_name, "scale mail"))
        return "스케일 메일";
    if (!strcmp(en_name, "plate mail"))
        return "판금 갑옷";
    if (!strcmp(en_name, "bronze plate mail"))
        return "청동 판금 갑옷";
    if (!strcmp(en_name, "splint mail"))
        return "스플린트 메일";
    if (!strcmp(en_name, "banded mail"))
        return "밴디드 메일";
    if (!strcmp(en_name, "dwarvish mithril-coat"))
        return "드워프제 미스릴 코트";
    if (!strcmp(en_name, "elven mithril-coat"))
        return "엘프제 미스릴 코트";
    if (!strcmp(en_name, "crystal plate mail"))
        return "수정 판금 갑옷";

    /* 용 비늘 & 용 비늘 갑옷 */
    if (!strcmp(en_name, "gray dragon scales"))
        return "회색 용 비늘";
    if (!strcmp(en_name, "silver dragon scales"))
        return "은색 용 비늘";
    if (!strcmp(en_name, "shimmering dragon scales"))
        return "일렁이는 용 비늘";
    if (!strcmp(en_name, "red dragon scales"))
        return "붉은 용 비늘";
    if (!strcmp(en_name, "white dragon scales"))
        return "하얀 용 비늘";
    if (!strcmp(en_name, "orange dragon scales"))
        return "주황색 용 비늘";
    if (!strcmp(en_name, "black dragon scales"))
        return "검은 용 비늘";
    if (!strcmp(en_name, "blue dragon scales"))
        return "푸른 용 비늘";
    if (!strcmp(en_name, "green dragon scales"))
        return "초록색 용 비늘";
    if (!strcmp(en_name, "yellow dragon scales"))
        return "노란색 용 비늘";
    if (!strcmp(en_name, "gray dragon scale mail"))
        return "회색 용 비늘 갑옷";
    if (!strcmp(en_name, "silver dragon scale mail"))
        return "은색 용 비늘 갑옷";
    if (!strcmp(en_name, "shimmering dragon scale mail"))
        return "일렁이는 용 비늘 갑옷";
    if (!strcmp(en_name, "red dragon scale mail"))
        return "붉은 용 비늘 갑옷";
    if (!strcmp(en_name, "white dragon scale mail"))
        return "하얀 용 비늘 갑옷";
    if (!strcmp(en_name, "orange dragon scale mail"))
        return "주황색 용 비늘 갑옷";
    if (!strcmp(en_name, "black dragon scale mail"))
        return "검은 용 비늘 갑옷";
    if (!strcmp(en_name, "blue dragon scale mail"))
        return "푸른 용 비늘 갑옷";
    if (!strcmp(en_name, "green dragon scale mail"))
        return "초록색 용 비늘 갑옷";
    if (!strcmp(en_name, "yellow dragon scale mail"))
        return "노란색 용 비늘 갑옷";

    /* 망토 */
    if (!strcmp(en_name, "mummy wrapping"))
        return "미라의 붕대";
    if (!strcmp(en_name, "elven cloak"))
        return "엘프제 망토";
    if (!strcmp(en_name, "orcish cloak"))
        return "오크제 망토";
    if (!strcmp(en_name, "dwarvish cloak"))
        return "드워프제 망토";
    if (!strcmp(en_name, "oilskin cloak"))
        return "방수포 망토";
    if (!strcmp(en_name, "robe"))
        return "로브";
    if (!strcmp(en_name, "alchemy smock"))
        return "연금술사의 작업복";
    if (!strcmp(en_name, "leather cloak"))
        return "가죽 망토";
    if (!strcmp(en_name, "cloak of protection"))
        return "보호의 망토";
    if (!strcmp(en_name, "cloak of invisibility"))
        return "투명의 망토";
    if (!strcmp(en_name, "cloak of magic resistance"))
        return "마법 저항의 망토";
    if (!strcmp(en_name, "cloak of displacement"))
        return "잔상의 망토";

    /* 투구 */
    if (!strcmp(en_name, "elven leather helm"))
        return "엘프제 가죽 투구";
    if (!strcmp(en_name, "orcish helm"))
        return "오크제 투구";
    if (!strcmp(en_name, "dwarvish iron helm"))
        return "드워프제 철 투구";
    if (!strcmp(en_name, "fedora"))
        return "페도라";
    if (!strcmp(en_name, "cornuthaum"))
        return "고깔모자";
    if (!strcmp(en_name, "dunce cap"))
        return "바보 모자";
    if (!strcmp(en_name, "dented pot"))
        return "찌그러진 냄비";
    if (!strcmp(en_name, "helmet"))
        return "투구";
    if (!strcmp(en_name, "helm of brilliance"))
        return "총명의 투구";
    if (!strcmp(en_name, "helm of opposite alignment"))
        return "반대 성향의 투구";
    if (!strcmp(en_name, "helm of telepathy"))
        return "텔레파시의 투구";

    /* 장갑 */
    if (!strcmp(en_name, "leather gloves"))
        return "가죽 장갑";
    if (!strcmp(en_name, "gauntlets of fumbling"))
        return "실수의 건틀릿";
    if (!strcmp(en_name, "gauntlets of power"))
        return "힘의 건틀릿";
    if (!strcmp(en_name, "gauntlets of dexterity"))
        return "민첩의 건틀릿";

    /* 방패 */
    if (!strcmp(en_name, "small shield"))
        return "작은 방패";
    if (!strcmp(en_name, "elven shield"))
        return "엘프제 방패";
    if (!strcmp(en_name, "Uruk-hai shield"))
        return "우르크하이 방패";
    if (!strcmp(en_name, "orcish shield"))
        return "오크제 방패";
    if (!strcmp(en_name, "large shield"))
        return "큰 방패";
    if (!strcmp(en_name, "dwarvish roundshield"))
        return "드워프제 둥근 방패";
    if (!strcmp(en_name, "shield of reflection"))
        return "반사의 방패";

    /* 신발 */
    if (!strcmp(en_name, "low boots"))
        return "짧은 장화";
    if (!strcmp(en_name, "iron shoes"))
        return "철 신발";
    if (!strcmp(en_name, "high boots"))
        return "긴 장화";
    if (!strcmp(en_name, "speed boots"))
        return "가속의 장화";
    if (!strcmp(en_name, "water walking boots"))
        return "수면 보행 장화";
    if (!strcmp(en_name, "jumping boots"))
        return "도약의 장화";
    if (!strcmp(en_name, "elven boots"))
        return "엘프제 장화";
    if (!strcmp(en_name, "kicking boots"))
        return "발차기의 장화";
    if (!strcmp(en_name, "fumble boots"))
        return "실수의 장화";
    if (!strcmp(en_name, "levitation boots"))
        return "부유의 장화";

    /* ========================================== *
     * 반지 (Rings)
     * ========================================== */
    if (!strcmp(en_name, "adornment"))
        return "장식";
    if (!strcmp(en_name, "hunger"))
        return "허기";
    if (!strcmp(en_name, "protection"))
        return "보호";
    if (!strcmp(en_name, "protection from shape changers"))
        return "변신 방지";
    if (!strcmp(en_name, "stealth"))
        return "은밀";
    if (!strcmp(en_name, "sustain ability"))
        return "능력 유지";
    if (!strcmp(en_name, "warning"))
        return "경고";
    if (!strcmp(en_name, "aggravate monster"))
        return "몬스터 도발";
    if (!strcmp(en_name, "cold resistance"))
        return "냉기 저항";
    if (!strcmp(en_name, "gain constitution"))
        return "건강 증가";
    if (!strcmp(en_name, "gain strength"))
        return "힘 증가";
    if (!strcmp(en_name, "increase accuracy"))
        return "명중률 증가";
    if (!strcmp(en_name, "increase damage"))
        return "피해량 증가";
    if (!strcmp(en_name, "slow digestion"))
        return "소화 지연";
    if (!strcmp(en_name, "invisibility"))
        return "투명화";
    if (!strcmp(en_name, "poison resistance"))
        return "독 저항";
    if (!strcmp(en_name, "see invisible"))
        return "투명 감지";
    if (!strcmp(en_name, "shock resistance"))
        return "전격 저항";
    if (!strcmp(en_name, "fire resistance"))
        return "화염 저항";
    if (!strcmp(en_name, "free action"))
        return "행동의 자유";
    if (!strcmp(en_name, "levitation"))
        return "부유";
    if (!strcmp(en_name, "regeneration"))
        return "재생";
    if (!strcmp(en_name, "searching"))
        return "탐색";
    if (!strcmp(en_name, "teleportation"))
        return "순간이동";
    if (!strcmp(en_name, "conflict"))
        return "분쟁";
    if (!strcmp(en_name, "polymorph"))
        return "폴리모프";
    if (!strcmp(en_name, "polymorph control"))
        return "폴리모프 제어";
    if (!strcmp(en_name, "teleport control"))
        return "순간이동 제어";

    /* ========================================== *
     * 부적 (Amulets)
     * ========================================== */
    if (!strcmp(en_name, "amulet of change"))
        return "변화의 부적";
    if (!strcmp(en_name, "amulet of ESP"))
        return "텔레파시의 부적";
    if (!strcmp(en_name, "amulet of life saving"))
        return "구명의 부적";
    if (!strcmp(en_name, "amulet of magical breathing"))
        return "마법 호흡의 부적";
    if (!strcmp(en_name, "amulet of reflection"))
        return "반사의 부적";
    if (!strcmp(en_name, "amulet of restful sleep"))
        return "숙면의 부적";
    if (!strcmp(en_name, "amulet of strangulation"))
        return "교살의 부적";
    if (!strcmp(en_name, "amulet of unchanging"))
        return "불변의 부적";
    if (!strcmp(en_name, "amulet versus poison"))
        return "독 저항의 부적";
    if (!strcmp(en_name, "Amulet of Yendor"))
        return "옌더의 부적";
    if (!strcmp(en_name, "fake Amulet of Yendor"))
        return "가짜 옌더의 부적";

    /* ========================================== *
     * 도구 (Tools)
     * ========================================== */
    if (!strcmp(en_name, "large box"))
        return "커다란 상자";
    if (!strcmp(en_name, "chest"))
        return "상자";
    if (!strcmp(en_name, "ice box"))
        return "얼음 상자";
    if (!strcmp(en_name, "sack"))
        return "자루";
    if (!strcmp(en_name, "oilskin sack"))
        return "방수포 자루";
    if (!strcmp(en_name, "bag of holding"))
        return "보존의 배낭";
    if (!strcmp(en_name, "bag of tricks"))
        return "속임수의 배낭";
    if (!strcmp(en_name, "skeleton key"))
        return "만능 열쇠";
    if (!strcmp(en_name, "lock pick"))
        return "락픽";
    if (!strcmp(en_name, "credit card"))
        return "신용카드";
    if (!strcmp(en_name, "tallow candle"))
        return "양초(동물성)";
    if (!strcmp(en_name, "wax candle"))
        return "양초(밀랍)";
    if (!strcmp(en_name, "brass lantern"))
        return "황동 랜턴";
    if (!strcmp(en_name, "oil lamp"))
        return "기름 램프";
    if (!strcmp(en_name, "magic lamp"))
        return "마법의 램프";
    if (!strcmp(en_name, "tin whistle"))
        return "양철 호루라기";
    if (!strcmp(en_name, "magic whistle"))
        return "마법의 호루라기";
    if (!strcmp(en_name, "wooden flute"))
        return "나무 피리";
    if (!strcmp(en_name, "magic flute"))
        return "마법의 피리";
    if (!strcmp(en_name, "tooled horn"))
        return "세공된 뿔피리";
    if (!strcmp(en_name, "frost horn"))
        return "서리의 뿔피리";
    if (!strcmp(en_name, "fire horn"))
        return "화염의 뿔피리";
    if (!strcmp(en_name, "horn of plenty"))
        return "풍요의 뿔피리";
    if (!strcmp(en_name, "wooden harp"))
        return "나무 하프";
    if (!strcmp(en_name, "magic harp"))
        return "마법의 하프";
    if (!strcmp(en_name, "bell"))
        return "종";
    if (!strcmp(en_name, "bugle"))
        return "나팔";
    if (!strcmp(en_name, "leather drum"))
        return "가죽 북";
    if (!strcmp(en_name, "drum of earthquake"))
        return "지진의 북";
    if (!strcmp(en_name, "pick-axe"))
        return "곡괭이";
    if (!strcmp(en_name, "grappling hook"))
        return "갈고리 밧줄";
    if (!strcmp(en_name, "unicorn horn"))
        return "유니콘의 뿔";
    if (!strcmp(en_name, "expensive camera"))
        return "비싼 카메라";
    if (!strcmp(en_name, "towel"))
        return "수건";
    if (!strcmp(en_name, "blindfold"))
        return "눈가리개";
    if (!strcmp(en_name, "lenses"))
        return "렌즈";
    if (!strcmp(en_name, "stethoscope"))
        return "청진기";
    if (!strcmp(en_name, "tinning kit"))
        return "통조림 도구";
    if (!strcmp(en_name, "tin opener"))
        return "통조림 따개";
    if (!strcmp(en_name, "leash"))
        return "가죽끈";
    if (!strcmp(en_name, "saddle"))
        return "안장";
    if (!strcmp(en_name, "magic marker"))
        return "마법의 마커";
    if (!strcmp(en_name, "land mine"))
        return "지뢰";
    if (!strcmp(en_name, "beartrap"))
        return "곰덫";
    if (!strcmp(en_name, "Candelabrum of Invocation"))
        return "기원의 촛대";
    if (!strcmp(en_name, "Bell of Opening"))
        return "개방의 종";
    if (!strcmp(en_name, "mirror"))
        return "거울";
    if (!strcmp(en_name, "crystal ball"))
        return "수정 구슬";

    /* ========================================== *
     * 음식 (Food)
     * ========================================== */
    if (!strcmp(en_name, "tripe ration"))
        return "내장 배급식";
    if (!strcmp(en_name, "corpse"))
        return "사체";
    if (!strcmp(en_name, "egg"))
        return "알";
    if (!strcmp(en_name, "meatball"))
        return "고기 완자";
    if (!strcmp(en_name, "meat ring"))
        return "고기 반지";
    if (!strcmp(en_name, "meat stick"))
        return "고기 막대기";
    if (!strcmp(en_name, "meat chunk"))
        return "고기 덩어리";
    if (!strcmp(en_name, "huge chunk of meat"))
        return "거대한 고기 덩어리";
    if (!strcmp(en_name, "kelp frond"))
        return "다시마 잎";
    if (!strcmp(en_name, "eucalyptus leaf"))
        return "유칼립투스 잎";
    if (!strcmp(en_name, "apple"))
        return "사과";
    if (!strcmp(en_name, "orange"))
        return "오렌지";
    if (!strcmp(en_name, "pear"))
        return "배";
    if (!strcmp(en_name, "melon"))
        return "멜론";
    if (!strcmp(en_name, "banana"))
        return "바나나";
    if (!strcmp(en_name, "carrot"))
        return "당근";
    if (!strcmp(en_name, "sprig of wolfsbane"))
        return "투구꽃 가지";
    if (!strcmp(en_name, "clove of garlic"))
        return "마늘 한 쪽";
    if (!strcmp(en_name, "slime mold"))
        return "점액질 곰팡이";
    if (!strcmp(en_name, "lump of royal jelly"))
        return "로열 젤리 덩어리";
    if (!strcmp(en_name, "cream pie"))
        return "크림 파이";
    if (!strcmp(en_name, "candy bar"))
        return "캔디 바";
    if (!strcmp(en_name, "fortune cookie"))
        return "포춘 쿠키";
    if (!strcmp(en_name, "pancake"))
        return "팬케이크";
    if (!strcmp(en_name, "lembas wafer"))
        return "렘바스 웨하스";
    if (!strcmp(en_name, "cram ration"))
        return "크램 배급식";
    if (!strcmp(en_name, "food ration"))
        return "식량 배급식";
    if (!strcmp(en_name, "K-ration"))
        return "K-레이션";
    if (!strcmp(en_name, "C-ration"))
        return "C-레이션";
    if (!strcmp(en_name, "tin"))
        return "통조림";

    /* ========================================== *
     * 마법 물품 Base Names
     * (Potion/Scroll/Wand/Spellbook 공유 이름)
     * ========================================== */
    if (!strcmp(en_name, "booze"))
        return "술";
    if (!strcmp(en_name, "fruit juice"))
        return "과일 주스";
    if (!strcmp(en_name, "see invisible"))
        return "투명 감지";
    if (!strcmp(en_name, "sickness"))
        return "질병";
    if (!strcmp(en_name, "confusion"))
        return "혼란";
    if (!strcmp(en_name, "extra healing"))
        return "추가 치료";
    if (!strcmp(en_name, "hallucination"))
        return "환각";
    if (!strcmp(en_name, "healing"))
        return "치료";
    if (!strcmp(en_name, "restore ability"))
        return "능력 복구";
    if (!strcmp(en_name, "gain level"))
        return "레벨 상승";
    if (!strcmp(en_name, "sleeping"))
        return "수면";
    if (!strcmp(en_name, "water"))
        return "물";
    if (!strcmp(en_name, "blindness"))
        return "실명";
    if (!strcmp(en_name, "paralysis"))
        return "마비";
    if (!strcmp(en_name, "speed"))
        return "가속";
    if (!strcmp(en_name, "monster detection"))
        return "몬스터 감지";
    if (!strcmp(en_name, "object detection"))
        return "물건 감지";
    if (!strcmp(en_name, "enlightenment"))
        return "깨달음";
    if (!strcmp(en_name, "full healing"))
        return "완전 치료";
    if (!strcmp(en_name, "mutability"))
        return "돌연변이";
    if (!strcmp(en_name, "acid"))
        return "산성";
    if (!strcmp(en_name, "oil"))
        return "기름";
    if (!strcmp(en_name, "amnesia"))
        return "기억상실";
    if (!strcmp(en_name, "mail"))
        return "우편";
    if (!strcmp(en_name, "enchant armor"))
        return "방어구 강화";
    if (!strcmp(en_name, "destroy armor"))
        return "방어구 파괴";
    if (!strcmp(en_name, "confuse monster"))
        return "몬스터 혼란";
    if (!strcmp(en_name, "scare monster"))
        return "몬스터 공포";
    if (!strcmp(en_name, "remove curse"))
        return "저주 해제";
    if (!strcmp(en_name, "enchant weapon"))
        return "무기 강화";
    if (!strcmp(en_name, "create monster"))
        return "몬스터 소환";
    if (!strcmp(en_name, "taming"))
        return "몬스터 길들이기";
    if (!strcmp(en_name, "genocide"))
        return "학살";
    if (!strcmp(en_name, "earth"))
        return "대지";
    if (!strcmp(en_name, "punishment"))
        return "처벌";
    if (!strcmp(en_name, "charging"))
        return "충전";
    if (!strcmp(en_name, "stinking cloud"))
        return "냄새나는 구름";
    if (!strcmp(en_name, "blank paper"))
        return "백지";
    if (!strcmp(en_name, "dig"))
        return "파기";
    if (!strcmp(en_name, "magic missile"))
        return "매직 미사일";
    if (!strcmp(en_name, "fireball"))
        return "파이어볼";
    if (!strcmp(en_name, "sleep"))
        return "수면";
    if (!strcmp(en_name, "cone of cold"))
        return "냉기 원뿔";
    if (!strcmp(en_name, "finger of death"))
        return "죽음의 손가락";
    if (!strcmp(en_name, "light"))
        return "빛";
    if (!strcmp(en_name, "detect monsters"))
        return "몬스터 감지";
    if (!strcmp(en_name, "knock"))
        return "열기";
    if (!strcmp(en_name, "force bolt"))
        return "포스 볼트";
    if (!strcmp(en_name, "cure blindness"))
        return "실명 치료";
    if (!strcmp(en_name, "drain life"))
        return "생명력 흡수";
    if (!strcmp(en_name, "slow monster"))
        return "몬스터 둔화";
    if (!strcmp(en_name, "wizard lock"))
        return "마법 잠금";
    if (!strcmp(en_name, "detect food"))
        return "음식 감지";
    if (!strcmp(en_name, "cause fear"))
        return "공포 유발";
    if (!strcmp(en_name, "clairvoyance"))
        return "천리안";
    if (!strcmp(en_name, "cure sickness"))
        return "질병 치료";
    if (!strcmp(en_name, "charm monster"))
        return "몬스터 매혹";
    if (!strcmp(en_name, "haste self"))
        return "자기 가속";
    if (!strcmp(en_name, "detect unseen"))
        return "투명 감지";
    if (!strcmp(en_name, "detect treasure"))
        return "보물 감지";
    if (!strcmp(en_name, "magic mapping"))
        return "마법의 지도";
    if (!strcmp(en_name, "identify"))
        return "식별";
    if (!strcmp(en_name, "turn undead"))
        return "언데드 퇴치";
    if (!strcmp(en_name, "teleport away"))
        return "멀리 순간이동";
    if (!strcmp(en_name, "create familiar"))
        return "사역마 소환";
    if (!strcmp(en_name, "cancellation"))
        return "무효화";
    if (!strcmp(en_name, "jumping"))
        return "도약";
    if (!strcmp(en_name, "stone to flesh"))
        return "돌을 살로";
    if (!strcmp(en_name, "Book of the Dead"))
        return "사자의 서";
    if (!strcmp(en_name, "secret door detection"))
        return "비밀 문 감지";
    if (!strcmp(en_name, "wishing"))
        return "소원";
    if (!strcmp(en_name, "nothing"))
        return "아무것도 아님";
    if (!strcmp(en_name, "striking"))
        return "타격";
    if (!strcmp(en_name, "make invisible"))
        return "투명화 부여";
    if (!strcmp(en_name, "speed monster"))
        return "몬스터 가속";
    if (!strcmp(en_name, "opening"))
        return "열기";
    if (!strcmp(en_name, "locking"))
        return "잠그기";
    if (!strcmp(en_name, "probing"))
        return "탐지";
    if (!strcmp(en_name, "digging"))
        return "파기";
    if (!strcmp(en_name, "cold"))
        return "냉기";
    if (!strcmp(en_name, "death"))
        return "죽음";
    if (!strcmp(en_name, "lightning"))
        return "번개";
    if (!strcmp(en_name, "gold detection"))
        return "금 감지";
    /* (주문/두루마리는 turn undead | 지팡이는 undead turning) */
    if (!strcmp(en_name, "undead turning"))
        return "언데드 퇴치"; 
    if (!strcmp(en_name, "novel"))
        return "소설책";

    /* ========================================== *
     * 보석 및 바위 (Gems & Rocks)
     * ========================================== */
    if (!strcmp(en_name, "dilithium crystal"))
        return "다이리튬 크리스탈";
    if (!strcmp(en_name, "diamond"))
        return "다이아몬드";
    if (!strcmp(en_name, "ruby"))
        return "루비";
    if (!strcmp(en_name, "jacinth"))
        return "자신스";
    if (!strcmp(en_name, "sapphire"))
        return "사파이어";
    if (!strcmp(en_name, "black opal"))
        return "블랙 오팔";
    if (!strcmp(en_name, "emerald"))
        return "에메랄드";
    if (!strcmp(en_name, "turquoise"))
        return "터키석";
    if (!strcmp(en_name, "citrine"))
        return "황수정";
    if (!strcmp(en_name, "aquamarine"))
        return "아쿠아마린";
    if (!strcmp(en_name, "amber"))
        return "호박석";
    if (!strcmp(en_name, "topaz"))
        return "토파즈";
    if (!strcmp(en_name, "jet"))
        return "흑옥";
    if (!strcmp(en_name, "opal"))
        return "오팔";
    if (!strcmp(en_name, "chrysoberyl"))
        return "금록석";
    if (!strcmp(en_name, "garnet"))
        return "가닛";
    if (!strcmp(en_name, "amethyst"))
        return "자수정";
    if (!strcmp(en_name, "agate"))
        return "마노";
    if (!strcmp(en_name, "fluorite"))
        return "형석";
    if (!strcmp(en_name, "jade"))
        return "옥";
    if (!strcmp(en_name, "worthless piece of white glass"))
        return "가치 없는 하얀 유리 조각";
    if (!strcmp(en_name, "worthless piece of blue glass"))
        return "가치 없는 푸른 유리 조각";
    if (!strcmp(en_name, "worthless piece of red glass"))
        return "가치 없는 붉은 유리 조각";
    if (!strcmp(en_name, "worthless piece of yellowish brown glass"))
        return "가치 없는 황갈색 유리 조각";
    if (!strcmp(en_name, "worthless piece of orange glass"))
        return "가치 없는 주황색 유리 조각";
    if (!strcmp(en_name, "worthless piece of yellow glass"))
        return "가치 없는 노란 유리 조각";
    if (!strcmp(en_name, "worthless piece of black glass"))
        return "가치 없는 검은 유리 조각";
    if (!strcmp(en_name, "worthless piece of green glass"))
        return "가치 없는 초록색 유리 조각";
    if (!strcmp(en_name, "worthless piece of violet glass"))
        return "가치 없는 보라색 유리 조각";
    if (!strcmp(en_name, "luckstone"))
        return "행운의 돌";
    if (!strcmp(en_name, "loadstone"))
        return "무거운 돌";
    if (!strcmp(en_name, "touchstone"))
        return "시금석";
    if (!strcmp(en_name, "flint"))
        return "부싯돌";
    if (!strcmp(en_name, "rock"))
        return "돌";
    if (!strcmp(en_name, "boulder"))
        return "바위";
    if (!strcmp(en_name, "statue"))
        return "조각상";
    if (!strcmp(en_name, "heavy iron ball"))
        return "무거운 철구";
    if (!strcmp(en_name, "iron chain"))
        return "쇠사슬";

    /* ========================================== *
     * 독 (Venoms)
     * ========================================== */
    if (!strcmp(en_name, "blinding venom"))
        return "실명 독액";
    if (!strcmp(en_name, "acid venom"))
        return "산성 독액";

/* ========================================== *
     * 미식별 묘사 / 외형 (Descriptions)
     * ========================================== */

    /* [무기 외형] */
    if (!strcmp(en_name, "runed arrow"))
        return "룬이 새겨진 화살";
    if (!strcmp(en_name, "crude arrow"))
        return "조잡한 화살";
    if (!strcmp(en_name, "bamboo arrow"))
        return "대나무 화살";
    if (!strcmp(en_name, "throwing star"))
        return "표창";
    if (!strcmp(en_name, "throwing spear"))
        return "투창";
    if (!strcmp(en_name, "runed spear"))
        return "룬이 새겨진 창";
    if (!strcmp(en_name, "crude spear"))
        return "조잡한 창";
    if (!strcmp(en_name, "stout spear"))
        return "튼튼한 창";
    if (!strcmp(en_name, "scimitar"))
        return "곡도";
    if (!strcmp(en_name, "runed dagger"))
        return "룬이 새겨진 단검";
    if (!strcmp(en_name, "crude dagger"))
        return "조잡한 단검";
    if (!strcmp(en_name, "double-headed axe"))
        return "양날 도끼";
    if (!strcmp(en_name, "runed short sword"))
        return "룬이 새겨진 짧은 검";
    if (!strcmp(en_name, "crude short sword"))
        return "조잡한 짧은 검";
    if (!strcmp(en_name, "broad short sword"))
        return "넓고 짧은 검";
    if (!strcmp(en_name, "curved sword"))
        return "휘어진 검";
    if (!strcmp(en_name, "runed broadsword"))
        return "룬이 새겨진 브로드소드";
    if (!strcmp(en_name, "samurai sword"))
        return "사무라이 검";
    if (!strcmp(en_name, "long samurai sword"))
        return "긴 사무라이 검";
    if (!strcmp(en_name, "vulgar polearm"))
        return "조잡한 미늘창";
    if (!strcmp(en_name, "hilted polearm"))
        return "자루가 있는 미늘창";
    if (!strcmp(en_name, "forked polearm"))
        return "갈라진 미늘창";
    if (!strcmp(en_name, "single-edged polearm"))
        return "외날 미늘창";
    if (!strcmp(en_name, "angled poleaxe"))
        return "각진 폴액스";
    if (!strcmp(en_name, "long poleaxe"))
        return "긴 폴액스";
    if (!strcmp(en_name, "pole cleaver"))
        return "장대 고기칼";
    if (!strcmp(en_name, "broad pick"))
        return "넓은 곡괭이";
    if (!strcmp(en_name, "pole sickle"))
        return "장대 낫";
    if (!strcmp(en_name, "pruning hook"))
        return "전지용 갈고리";
    if (!strcmp(en_name, "hooked polearm"))
        return "갈고리 달린 미늘창";
    if (!strcmp(en_name, "pronged polearm"))
        return "갈래가 있는 미늘창";
    if (!strcmp(en_name, "beaked polearm"))
        return "부리 모양 미늘창";
    if (!strcmp(en_name, "thonged club"))
        return "가죽끈 달린 곤봉";
    if (!strcmp(en_name, "runed bow"))
        return "룬이 새겨진 활";
    if (!strcmp(en_name, "crude bow"))
        return "조잡한 활";
    if (!strcmp(en_name, "long bow"))
        return "장궁";
    if (!strcmp(en_name, "staff"))
        return "지팡이";
    if (!strcmp(en_name, "iron hook"))
        return "철 갈고리";

    /* [방어구 외형] */
    if (!strcmp(en_name, "leather hat"))
        return "가죽 모자";
    if (!strcmp(en_name, "iron skull cap"))
        return "철제 둥근 모자";
    if (!strcmp(en_name, "hard hat"))
        return "단단한 모자";
    if (!strcmp(en_name, "conical hat"))
        return "원뿔형 모자";
    if (!strcmp(en_name, "plumed helmet"))
        return "깃장식 투구";
    if (!strcmp(en_name, "etched helmet"))
        return "식각된 투구";
    if (!strcmp(en_name, "crested helmet"))
        return "볏이 있는 투구";
    if (!strcmp(en_name, "visored helmet"))
        return "얼굴 가리개가 있는 투구";
    if (!strcmp(en_name, "crude chain mail"))
        return "조잡한 사슬 갑옷";
    if (!strcmp(en_name, "chain mail"))
        return "사슬 갑옷";
    if (!strcmp(en_name, "crude ring mail"))
        return "조잡한 고리 갑옷";
    if (!strcmp(en_name, "faded pall"))
        return "빛바랜 관의";
    if (!strcmp(en_name, "coarse mantelet"))
        return "거친 망토";
    if (!strcmp(en_name, "hooded cloak"))
        return "후드 달린 망토";
    if (!strcmp(en_name, "slippery cloak"))
        return "미끄러운 망토";
    if (!strcmp(en_name, "apron"))
        return "앞치마";
    if (!strcmp(en_name, "tattered cape"))
        return "너덜너덜한 망토";
    if (!strcmp(en_name, "opera cloak"))
        return "오페라 망토";
    if (!strcmp(en_name, "ornamental cope"))
        return "장식된 망토";
    if (!strcmp(en_name, "piece of cloth"))
        return "천 조각";
    if (!strcmp(en_name, "blue and green shield"))
        return "파랗고 초록색인 방패";
    if (!strcmp(en_name, "white-handed shield"))
        return "하얀 손이 그려진 방패";
    if (!strcmp(en_name, "red-eyed shield"))
        return "붉은 눈이 그려진 방패";
    if (!strcmp(en_name, "large round shield"))
        return "크고 둥근 방패";
    if (!strcmp(en_name, "polished silver shield"))
        return "광택나는 은 방패";
    if (!strcmp(en_name, "old gloves"))
        return "낡은 장갑";
    if (!strcmp(en_name, "padded gloves"))
        return "패딩 장갑";
    if (!strcmp(en_name, "riding gloves"))
        return "승마용 장갑";
    if (!strcmp(en_name, "fencing gloves"))
        return "펜싱 장갑";
    if (!strcmp(en_name, "walking shoes"))
        return "산책용 신발";
    if (!strcmp(en_name, "hard shoes"))
        return "단단한 신발";
    if (!strcmp(en_name, "jackboots"))
        return "가죽 장화";
    if (!strcmp(en_name, "combat boots"))
        return "전투화";
    if (!strcmp(en_name, "jungle boots"))
        return "정글 장화";
    if (!strcmp(en_name, "hiking boots"))
        return "등산화";
    if (!strcmp(en_name, "mud boots"))
        return "진흙 장화";
    if (!strcmp(en_name, "buckled boots"))
        return "버클이 달린 장화";
    if (!strcmp(en_name, "riding boots"))
        return "승마 장화";
    if (!strcmp(en_name, "snow boots"))
        return "설원 장화";

    /* [부적 외형] */
    if (!strcmp(en_name, "circular"))
        return "원형";
    if (!strcmp(en_name, "spherical"))
        return "구형";
    if (!strcmp(en_name, "oval"))
        return "타원형";
    if (!strcmp(en_name, "triangular"))
        return "삼각형";
    if (!strcmp(en_name, "pyramidal"))
        return "피라미드형";
    if (!strcmp(en_name, "square"))
        return "정사각형";
    if (!strcmp(en_name, "concave"))
        return "오목한";
    if (!strcmp(en_name, "hexagonal"))
        return "육각형";
    if (!strcmp(en_name, "octagonal"))
        return "팔각형";

    /* [도구 외형] */
    if (!strcmp(en_name, "bag"))
        return "자루";
    if (!strcmp(en_name, "key"))
        return "열쇠";
    if (!strcmp(en_name, "candle"))
        return "양초";
    if (!strcmp(en_name, "lamp"))
        return "램프";
    if (!strcmp(en_name, "looking glass"))
        return "손거울";
    if (!strcmp(en_name, "glass orb"))
        return "유리 구슬";
    if (!strcmp(en_name, "whistle"))
        return "호루라기";
    if (!strcmp(en_name, "flute"))
        return "피리";
    if (!strcmp(en_name, "horn"))
        return "뿔피리";
    if (!strcmp(en_name, "harp"))
        return "하프";
    if (!strcmp(en_name, "drum"))
        return "북";
    if (!strcmp(en_name, "candelabrum"))
        return "촛대";
    if (!strcmp(en_name, "silver bell"))
        return "은 종";

    /* [물약/보석/기타 색상 묘사] */
    /* ruby, aquamarine은 이미 추가하셨으므로 생략 */
    if (!strcmp(en_name, "pink"))
        return "분홍색";
    if (!strcmp(en_name, "orange"))
        return "주황색";
    if (!strcmp(en_name, "yellow"))
        return "노란색";
    if (!strcmp(en_name, "emerald"))
        return "에메랄드";
    if (!strcmp(en_name, "dark green"))
        return "진초록색";
    if (!strcmp(en_name, "cyan"))
        return "청록색";
    if (!strcmp(en_name, "sky blue"))
        return "하늘색";
    if (!strcmp(en_name, "brilliant blue"))
        return "선명한 파란색";
    if (!strcmp(en_name, "magenta"))
        return "자홍색";
    if (!strcmp(en_name, "purple-red"))
        return "적자색";
    if (!strcmp(en_name, "puce"))
        return "암갈색";
    if (!strcmp(en_name, "milky"))
        return "우윳빛";
    if (!strcmp(en_name, "swirly"))
        return "소용돌이치는";
    if (!strcmp(en_name, "bubbly"))
        return "거품이 나는";
    if (!strcmp(en_name, "smoky"))
        return "연기가 나는";
    if (!strcmp(en_name, "cloudy"))
        return "구름 낀";
    if (!strcmp(en_name, "effervescent"))
        return "거품이 끓어오르는";
    if (!strcmp(en_name, "black"))
        return "검은색";
    if (!strcmp(en_name, "golden"))
        return "황금색";
    if (!strcmp(en_name, "brown"))
        return "갈색";
    if (!strcmp(en_name, "fizzy"))
        return "탄산이 있는";
    if (!strcmp(en_name, "dark"))
        return "어두운색";
    if (!strcmp(en_name, "white"))
        return "하얀색";
    if (!strcmp(en_name, "murky"))
        return "탁한";
    if (!strcmp(en_name, "clear"))
        return "투명한";
    if (!strcmp(en_name, "yellowish brown"))
        return "황갈색";
    if (!strcmp(en_name, "gray"))
        return "회색";

    /* [주문서 외형] */
    if (!strcmp(en_name, "parchment"))
        return "양피지";
    if (!strcmp(en_name, "vellum"))
        return "피지";
    if (!strcmp(en_name, "ragged"))
        return "너덜너덜한";
    if (!strcmp(en_name, "dog eared"))
        return "모서리가 접힌";
    if (!strcmp(en_name, "mottled"))
        return "얼룩덜룩한";
    if (!strcmp(en_name, "stained"))
        return "때묻은";
    if (!strcmp(en_name, "cloth"))
        return "천";
    if (!strcmp(en_name, "leathery"))
        return "가죽 같은";
    if (!strcmp(en_name, "velvet"))
        return "벨벳";
    if (!strcmp(en_name, "light green"))
        return "연초록색";
    if (!strcmp(en_name, "turquoise"))
        return "청록색";
    if (!strcmp(en_name, "light blue"))
        return "연파란색";
    if (!strcmp(en_name, "dark blue"))
        return "진파란색";
    if (!strcmp(en_name, "indigo"))
        return "남색";
    if (!strcmp(en_name, "purple"))
        return "보라색";
    if (!strcmp(en_name, "violet"))
        return "청자색";
    if (!strcmp(en_name, "tan"))
        return "황갈색";
    if (!strcmp(en_name, "plaid"))
        return "격자무늬";
    if (!strcmp(en_name, "light brown"))
        return "연갈색";
    if (!strcmp(en_name, "dark brown"))
        return "진갈색";
    if (!strcmp(en_name, "wrinkled"))
        return "구겨진";
    if (!strcmp(en_name, "dusty"))
        return "먼지 쌓인";
    if (!strcmp(en_name, "bronze"))
        return "청동";
    if (!strcmp(en_name, "copper"))
        return "구리";
    if (!strcmp(en_name, "silver"))
        return "은";
    if (!strcmp(en_name, "gold"))
        return "금";
    if (!strcmp(en_name, "glittering"))
        return "반짝이는";
    if (!strcmp(en_name, "shining"))
        return "빛나는";
    if (!strcmp(en_name, "dull"))
        return "탁한";
    if (!strcmp(en_name, "thin"))
        return "얇은";
    if (!strcmp(en_name, "thick"))
        return "두꺼운";
    if (!strcmp(en_name, "canvas"))
        return "캔버스";
    if (!strcmp(en_name, "hardcover"))
        return "양장본";
    if (!strcmp(en_name, "plain"))
        return "평범한";
    if (!strcmp(en_name, "paperback"))
        return "문고판";
    if (!strcmp(en_name, "papyrus"))
        return "파피루스";

    /* [지팡이 재질] */
    if (!strcmp(en_name, "glass"))
        return "유리";
    if (!strcmp(en_name, "balsa"))
        return "발사나무";
    if (!strcmp(en_name, "crystal"))
        return "수정";
    if (!strcmp(en_name, "maple"))
        return "단풍나무";
    if (!strcmp(en_name, "pine"))
        return "소나무";
    if (!strcmp(en_name, "oak"))
        return "참나무";
    if (!strcmp(en_name, "ebony"))
        return "흑단나무";
    if (!strcmp(en_name, "marble"))
        return "대리석";
    if (!strcmp(en_name, "tin"))
        return "주석";
    if (!strcmp(en_name, "platinum"))
        return "백금";
    if (!strcmp(en_name, "iridium"))
        return "이리듐";
    if (!strcmp(en_name, "zinc"))
        return "아연";
    if (!strcmp(en_name, "aluminum"))
        return "알루미늄";
    if (!strcmp(en_name, "uranium"))
        return "우라늄";
    if (!strcmp(en_name, "iron"))
        return "철";
    if (!strcmp(en_name, "steel"))
        return "강철";
    if (!strcmp(en_name, "short"))
        return "짧은";
    if (!strcmp(en_name, "long"))
        return "긴";
    if (!strcmp(en_name, "curved"))
        return "휘어진";
    if (!strcmp(en_name, "forked"))
        return "갈라진";
    if (!strcmp(en_name, "spiked"))
        return "가시 돋친";
    if (!strcmp(en_name, "jeweled"))
        return "보석이 박힌";

    /* [반지 재질] */
    if (!strcmp(en_name, "wooden"))
        return "나무";
    if (!strcmp(en_name, "granite"))
        return "화강암";
    if (!strcmp(en_name, "clay"))
        return "점토";
    if (!strcmp(en_name, "coral"))
        return "산호";
    if (!strcmp(en_name, "black onyx"))
        return "블랙 오닉스";
    if (!strcmp(en_name, "moonstone"))
        return "문스톤";
    if (!strcmp(en_name, "tiger eye"))
        return "호안석";
    if (!strcmp(en_name, "jade"))
        return "옥";
    if (!strcmp(en_name, "agate"))
        return "마노";
    if (!strcmp(en_name, "topaz"))
        return "토파즈";
    if (!strcmp(en_name, "sapphire"))
        return "사파이어";
    if (!strcmp(en_name, "diamond"))
        return "다이아몬드";
    if (!strcmp(en_name, "pearl"))
        return "진주";
    if (!strcmp(en_name, "twisted"))
        return "꼬인";
    if (!strcmp(en_name, "ivory"))
        return "상아";
    if (!strcmp(en_name, "wire"))
        return "철사";
    if (!strcmp(en_name, "engagement"))
        return "약혼";
    if (!strcmp(en_name, "shiny"))
        return "빛나는";

    /* [두루마리 글자 (Gibberish)]
     * (마법 두루마리 글자는 대개 기호처럼 쓰여 번역하지 않는 경우가 많지만
     * "stamped" 등 상태를 나타내는 단어는 번역해야 합니다.) */
    if (!strcmp(en_name, "stamped"))
        return "우표가 붙은";
    if (!strcmp(en_name, "unlabeled"))
        return "아무것도 안 적힌";

    /* [기타 외형/특수 아이템] */
    if (!strcmp(en_name, "splash of venom"))
        return "독액 방울";
    if (!strcmp(en_name, "cheap plastic imitation of the Amulet of Yendor"))
        return "옌더의 부적의 싸구려 플라스틱 모조품";

    /* 빼먹은 것 */
    if (!strcmp(en_name, "knife"))
        return "칼";
    if (!strcmp(en_name, "stiletto"))
        return "스틸레토";
    if (!strcmp(en_name, "leather jacket"))
        return "가죽 재킷";
    if (!strcmp(en_name, "can of grease"))
        return "윤활유 캔";
    if (!strcmp(en_name, "figurine"))
        return "작은 조각상";
    if (!strcmp(en_name, "glob of gray ooze"))
        return "회색 점액의 방울";
    if (!strcmp(en_name, "glob of brown pudding"))
        return "갈색 푸딩의 방울";
    if (!strcmp(en_name, "glob of green slime"))
        return "초록 슬라임의 방울";
    if (!strcmp(en_name, "glob of black pudding"))
        return "검은 푸딩의 방울";
    if (!strcmp(en_name, "gain ability"))
        return "능력 획득";
    if (!strcmp(en_name, "gain energy"))
        return "에너지 획득";
    if (!strcmp(en_name, "gold piece"))
        return "금화";

    /* [신체 부위 - Body Parts] */
    if (!strcmp(en_name, "head"))
        return "머리";
    if (!strcmp(en_name, "eye"))
        return "눈";
    if (!strcmp(en_name, "face"))
        return "얼굴";
    if (!strcmp(en_name, "jaw"))
        return "턱";
    if (!strcmp(en_name, "nose"))
        return "코";
    if (!strcmp(en_name, "neck"))
        return "목";
    if (!strcmp(en_name, "stomach"))
        return "위장";
    if (!strcmp(en_name, "spine"))
        return "척추";
    if (!strcmp(en_name, "arm"))
        return "팔";
    if (!strcmp(en_name, "hand"))
        return "손";
    if (!strcmp(en_name, "fingertip"))
        return "손가락 끝";
    if (!strcmp(en_name, "finger"))
        return "손가락";
    if (!strcmp(en_name, "leg"))
        return "다리";
    if (!strcmp(en_name, "foot"))
        return "발";
    if (!strcmp(en_name, "fingertips"))
        return "손가락 끝";
    if (!strcmp(en_name, "fingers"))
        return "손가락";
    if (!strcmp(en_name, "legs"))
        return "다리";
    if (!strcmp(en_name, "feet"))
        return "발";
    if (!strcmp(en_name, "arms"))
        return "팔";
    if (!strcmp(en_name, "hands"))
        return "손";
    if (!strcmp(en_name, "eyes"))
        return "눈";

    return (char *) en_name; /* 사전에 없으면 원래 영어 반환 */
}
/* --- KRNethack 번역 사전 끝 --- */

/* ========================================== *
 * getobj / ggetobj 용 한국어 동사 매핑
 * ========================================== */
static const struct {
    const char *en_verb;
    const char *kr_prompt;
    const char *kr_action;
} kr_verb_map[] = {
    { "eat", "무엇을 먹겠습니까?", "먹을" },
    { "drop", "어떤 것을 버리시겠습니까?", "버릴" },
    { "drink", "무엇을 마시겠습니까?", "마실" },
    { "read", "어떤 것을 읽으시겠습니까?", "읽을" },
    { "wield", "어떤 것을 무기로 쥐겠습니까?", "무기로 쥘" },
    { "wear", "무엇을 입으시겠습니까?", "입을" },
    { "take off", "무엇을 벗으시겠습니까?", "벗을" },
    { "put on", "어떤 것을 착용하시겠습니까?", "착용할" },
    { "remove", "어떤 것을 빼시겠습니까?", "뺄" },
    { "rub", "어디에 문지르시겠습니까?", "문지를" },
    { "write on", "어디에 쓰시겠습니까?", "글을 쓸" },
    { "write with", "무엇으로 쓰시겠습니까?", "글을 쓸" },
    { "look at", "무엇을 살펴보시겠습니까?", "살펴볼" },
    { "use or apply", "어떤 것을 사용하시겠습니까?", "사용할" },
    { "throw", "무엇을 던지시겠습니까?", "던질" },
    { "fire", "무엇을 쏘시겠습니까?", "쏠" },
    { "dip", "무엇을 담그시겠습니까?", "담글" },
    { "untrap", "어떤 것으로 함정을 해제하시겠습니까?", "함정을 해제할" },
    { "offer", "무엇을 제물로 바치시겠습니까?", "제물로 바칠" },
    { "sacrifice", "무엇을 제물로 바치시겠습니까?", "제물로 바칠" },
    { "invoke", "어떤 것의 힘을 끌어내시겠습니까?", "힘을 끌어낼" },
    { "open", "무엇을 여시겠습니까?", "열" },
    { "close", "무엇을 닫으시겠습니까?", "닫을" },
    { "loot", "무엇을 뒤지시겠습니까?", "뒤질" },
    { "tin", "무엇을 통조림으로 만드시겠습니까?", "통조림으로 만들" },
    { "charge", "무엇을 충전하시겠습니까?", "충전할" },
    { "call", "무엇에 이름을 붙이시겠습니까?", "이름을 붙일" },
    { "adjust", "무엇을 조정하시겠습니까?", "조정할" },
    { "rub on the stone", "돌에 무엇을 문지르시겠습니까?", "문지를" },
    { "identify", "어떤 종류의 물건을 감정하시겠습니까?", "감정할" },
    { "pay", "무엇에 대한 값을 지불하시겠습니까?", "값을 지불할" },
    { "take", "어떤 것을 꺼내시겠습니까?", "꺼낼" },
    { "put in", "어떤 것을 넣으시겠습니까?", "넣을" },
    /* "name"은 아이템 | "call"은 몬스터 이름 붙이기 */
    { "name", "무엇에 이름을 붙이시겠습니까?", "이름을 붙일" }, 
    { "cast", "어떤 마법을 시전하시겠습니까?", "시전할" },
    { "pay for", "어떤 물건의 값을 지불하시겠습니까?", "값을 지불할" },
    { 0, 0, 0 }
};

/* 배열을 검색해서 포인터에 값을 넣어주는 메인 함수 */
void get_kr_strings(word, action_ptr, prompt_ptr, fallback_act,
                    fallback_prm) const char *word;
const char **action_ptr;
const char **prompt_ptr;
char *fallback_act;
char *fallback_prm;
{
    int k;
    *action_ptr = (const char *) 0;
    *prompt_ptr = (const char *) 0;

    for (k = 0; kr_verb_map[k].en_verb; k++) {
        if (!strcmp(word, kr_verb_map[k].en_verb)) {
            *prompt_ptr = kr_verb_map[k].kr_prompt;
            *action_ptr = kr_verb_map[k].kr_action;
            break;
        }
    }

    if (!*prompt_ptr) {
        Sprintf(fallback_prm, "어떤 것을 %s 하시겠습니까?", word);
        *prompt_ptr = fallback_prm;
        Sprintf(fallback_act, "%s 할", word);
        *action_ptr = fallback_act;
    }
}