#ifndef KOREAN_H
#define KOREAN_H

/* korean.c 에 있는 함수들의 목록표 (다른 파일에서 가져다 쓰기 위함) */

int get_jongseong(const char *str);
const char *append_josa(const char *word, const char *josa);
int utf8_char_bytes(unsigned char c);
int korean_strwidth(const char *str);
char *get_kr_name(const char *en_name);

/* 이번에 새로 추가할 함수 */
void get_kr_strings(const char *word, const char **action_ptr, const char **prompt_ptr, char *fallback_act, char *fallback_prm);

#endif /* KOREAN_H */