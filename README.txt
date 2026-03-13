<EN>
KRNethack. The Korean Translation project of the roguelike game, Nethack.
This project is based on JNethack (https://github.com/jnethack | https://scm.osdn.net/gitroot/jnethack/source.git ), the Japanese tranlation.
+ Nethack Devteam's official Nethack Git Repository (https://github.com/NetHack/NetHack)

<KR>
로그라이크 게임 'Nethack'의 한글화 프로젝트, KRNethack입니다.
이 프로젝트는 넷핵의 일본어 번역판인 Jnethack (https://github.com/jnethack | https://scm.osdn.net/gitroot/jnethack/source.git )을 기반으로 하고 있습니다.
+ 넷핵 개발팀의 공식 소스코드 (https://github.com/NetHack/NetHack)

취미 겸으로 부활시킨 개인 프로젝트입니다.
우선, 많은 관심 가져주셔서 정말 감사드립니다. 그리고 기대에 응해드리지 못해 죄송합니다.
프로그래밍, 더군다나 C에 대해 잘 알지도 못하면서 넷핵 번역에 손대는 것은 매우 어려운 
도전이었고, 결국 당시에 개인적인 사정이 생긴 것과 맞물려 제대로 된 자료를 남기지 못하고
여타 다른 시도처럼 소스코드를 남기는 데 그치게 되었습니다. 하지만 저는 이것이 계속
마음에 걸렸고, 이번에 다시 부활시킨 것도 '배포는 욕심을 너무 많이 냈다. 내 개인적으로
할 수 있는 데까지 해보고 만족해보자'라는 지극히 개인적인 생각에서 부활시킨 겁니다.
위에선 부활시켰다고 거창하게 적긴 했지만, 이번에는 주변에 알리는 일 없이 저 혼자 조용히 
진행해볼 생각입니다. 추가할 사항이 있으면 나중에 여기에 추가로 적겠습니다.

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
메모장: 파일 ➔ 다른 이름으로 저장 ➔ 우측 하단 인코딩을 ANSI로 선택 ➔ 덮어쓰기

Visual Studio: 파일 ➔ 다른 이름으로 저장 ➔ 저장 버튼 옆 화살표(▼) 클릭 ➔ 인코딩하여 저장 ➔ 한국어 - 코드 페이지 949 선택 ➔ 덮어쓰기

빌드하는 법: Nethack.sln (win/win32/vs2017) 눌러서 프로젝트 로드하고 하면 됨
위자드 모드 하는 법 : 바로가기 생성 - 속성 - 대상 - 끝에 -D -u wizard 추가

수정필요
do.c : 186, 651, 657, 1951 jpast(verb), jconj_adj, 다중파일 동시수정

아쉬운 점
do.c : 계단/사다리/부유 여부에 무관하게 그냥 '올라감'. 나중에 고치자.

연락처 : 4492kjh@naver.com
