<EN>
KRNethack. The Korean Translation project of the roguelike game, Nethack.
This project is based on JNethack (https://github.com/jnethack | https://scm.osdn.net/gitroot/jnethack/source.git ), the Japanese tranlation.
+ Nethack Devteam's official Nethack Git Repository (https://github.com/NetHack/NetHack)

<KR>
로그라이크 게임 'Nethack'의 한글화 프로젝트, KRNethack입니다.
이 프로젝트는 넷핵의 일본어 번역판인 Jnethack (https://github.com/jnethack | https://scm.osdn.net/gitroot/jnethack/source.git )을 기반으로 하고 있습니다.
+ 넷핵 개발팀의 공식 소스코드 (https://github.com/NetHack/NetHack)

번역완료 (Completed)
allmain.c		role.c 	pline.c		dog.c
eat.c		fountain.c

번역 불필요 - src 폴더 (Not needed - src folder)
alloc.c	display.c		dlb.c	extralev.c
isaac64.c	mkmap.c		mkroom.c 	rect.c
rnd.c	sp_lev.c		sys.c		track.c
u_init.c	vision.c		windows.c

번역 불필요 - include 폴더 (Not needed - include folder)

용어 정리 - 아이템
wand = 지팡이
spellbook = 주문서	scroll = 두루마리		potion = 물약
용어 정리 - 몬스터

용어 정리 - 지형

메모
위자드 모드 하는 법 : -D -u wizard

수정필요
do.c : 186, 651, 657, 1951 jpast(verb), jconj_adj, 다중파일 동시수정

아쉬운 점
do.c : 계단/사다리/부유 여부에 무관하게 그냥 '올라감'. 나중에 고치자.

연락처 : 4492kjh@naver.com
