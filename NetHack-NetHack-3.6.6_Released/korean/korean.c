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