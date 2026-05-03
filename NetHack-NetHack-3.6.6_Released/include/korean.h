#ifndef KOREAN_H
#define KOREAN_H

/* korean.c 에 있는 함수들의 목록표 (다른 파일에서 가져다 쓰기 위함) */

int get_jongseong(const char *str);
const char *append_josa(const char *word, const char *josa);
int cp949_char_bytes(unsigned char c);
int korean_strwidth(const char *str);
char *get_kr_name(const char *en_name);
void get_kr_strings(const char *word, const char **action_ptr, const char **prompt_ptr, char *fallback_act, char *fallback_prm);

/* --- KRNethack 번역 출력 가로채기 매크로 --- */
#undef OBJ_NAME
#undef OBJ_DESCR

/* 엔진은 영어 원본을 기억하게 합니다 */
#define RAW_OBJ_NAME(obj) (obj_descr[(obj).oc_name_idx].oc_name)
#define RAW_OBJ_DESCR(obj) (obj_descr[(obj).oc_descr_idx].oc_descr)

/* 화면에 뿌릴 때만 사전을 거쳐 한글로 냅니다 */
#define OBJ_NAME(obj) get_kr_name(RAW_OBJ_NAME(obj))
#define OBJ_DESCR(obj) get_kr_name(RAW_OBJ_DESCR(obj))
/* ------------------------------------------ */

#endif /* KOREAN_H */