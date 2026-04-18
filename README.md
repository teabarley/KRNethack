# KRNethack - 한국어 NetHack (Windows GUI)
![Version](https://img.shields.io/badge/version-3.6.6-blue)
![License](https://img.shields.io/badge/license-NGPL-green)
![Translation](https://img.shields.io/badge/translation-WIP-yellow)

NetHack 3.6.6 Windows Graphical Interface 버전을 기반으로 하는 한국어 번역 개인 프로젝트입니다. 일본어 번역판인 JNetHack의 코드 구조를 일부 참고하여, 한국어의 어순과 문법에 맞게 엔진 단에서부터 텍스트 출력 구조를 재설계하고 있습니다.

> **Disclaimer** : 이 프로젝트는 넷핵을 사랑하는 개인 팬에 의해 진행되는 비공식 번역 프로젝트이며, 공식 NetHack DevTeam 및 JNetHack 개발팀과 어떠한 공식적인 관련도 없습니다.
(This is an unofficial fan translation project, not affiliated with the NetHack DevTeam or the JNetHack team.)

## 번역 구조 및 기술적 특징 (Translation Structure)
단순히 외부 텍스트 파일만 번역하는 것이 아닌, C언어 소스 코드를 직접 수정하여 한국어에 완벽하게 호환되는 출력 시스템을 구축하고 있습니다.

### 코드 레벨 어순 재배치 (Code-level Word Order Fixing)
영어의 SVO(주어-동사-목적어) 어순으로 하드코딩된 넷핵의 출력 매크로와 pline 함수들을 SOV(주어-목적어-동사) 어순으로 마개조합니다. 원본 코드를 보존하기 위해 전처리기 지시자를 사용합니다.

|
#if 0 /*KR: 원본*/
    pline("%s into the moat.", E_phrase(etmp, "fall"));
#else /*KR: KRNethack 맞춤 번역*/
    pline("%s 해자에 빠졌다.", append_josa(E_phrase(etmp, ""), "는"));
#endif
|

### 동적 조사 처리 시스템 (Dynamic Postposition)
은/는, 이/가, 을/를 등 앞 단어의 종성(받침) 유무에 따라 조사가 동적으로 변환되도록 append_josa 등의 커스텀 함수를 자체 구현하여 자연스러운 한국어 문장을 생성합니다.

### 엔진 보호를 위한 사전 후킹 (Dictionary Hooking)
게임의 세이브 파일이나 내부 로직은 영어를 기반으로 작동합니다. 엔진이 몬스터나 아이템을 생성하고 검색할 때는 원본 영어를 그대로 사용하게 두고, 플레이어의 화면에 출력되기 직전에만 get_kr_name() 함수가 개입하여 한글로 번역해 줍니다. 이를 통해 게임의 치명적인 오류를 방지합니다.

## 향후 프로젝트 진행 구조 (Roadmap)
본 로드맵은 Gemini의 도움을 받아 작성된 것으로, 추후 진행과 차이가 날 수 있는 점 양해 바랍니다.

[x] Phase 0: 번역 기반 시스템 구축 - 조사 처리 함수, 한글 이름 반환 사전(hacklib.c) 등 뼈대 작업.

[ ] Phase 1: src/ (핵심 소스 코드 번역) - ← 현재 진행 중

게임 내 상호작용, 전투, 마법, 특수 맵 생성(sp_lev.c), 명령어(cmd.c) 등 핵심 로직에서 출력되는 하드코딩 텍스트 및 어순 번역.

[ ] Phase 2: dat/ (데이터 및 시나리오 번역)

게임 백과사전(Encyclopedia), 소문(Rumors), 아티팩트 설명, 퀘스트 대사 및 신탁 등 정적 텍스트 데이터 번역.

[ ] Phase 3: win/ & sys/ (Windows GUI 번역)

Windows Graphical Interface에 종속적인 메뉴, 팝업창, 인벤토리 UI, 시스템 메시지 번역. (게임 내 한글 직접 입력기 등 윈도우 API 관련 연구 포함)

[ ] Phase 4: 최적화 및 배포

한글 폰트 렌더링 최적화, 오역/미번역 검수 및 최종 Windows 실행 파일(.exe) 빌드 릴리즈.

## 버그 및 오역 제보 (Reporting Issues)
아직 번역이 진행 중이므로 어색한 문장이나 오역, 혹은 게임 플레이 중 튕김 현상(Crash)이 발생할 수 있습니다.
문제나 개선 사항을 발견하시면 언제든 Issues에 제보해 주세요!

## 라이선스 (License)
NetHack General Public License (NGPL) 을 따릅니다.
자세한 사항은 소스 코드 내의 dat/license 파일을 참고해 주십시오.

## 링크 (Credits & Links)
- [Original NetHack](https://nethack.org/)
- [NetHack GitHub](https://github.com/NetHack/NetHack)

- [JNetHack Project](http://jnethack.sourceforge.jp/)
- [JNetHack GitHub](https://github.com/jnethack/jnethack-release)

- [Google Gemini](gemini.google.com)

README.txt 재작성에는 HanNetHack(https://github.com/timfr09/HanNetHack) 의 README를 많이 참고했습니다. 이 자리를 빌어 감사를 표합니다.

## 메모 (Note)
wand = 지팡이	spellbook = 주문서	
scroll = 두루마리		potion = 물약

메모장: 파일 ➔ 다른 이름으로 저장 ➔ 우측 하단 인코딩을 ANSI로 선택 ➔ 덮어쓰기

Visual Studio: 파일 ➔ 다른 이름으로 저장 ➔ 저장 버튼 옆 화살표(▼) 클릭 ➔ 인코딩하여 저장 ➔ 한국어 - 코드 페이지 949 선택 ➔ 덮어쓰기

빌드하는 법: Nethack.sln (win/win32/vs2017) 눌러서 프로젝트 로드하고 하면 됨
위자드 모드 하는 법 : 바로가기 생성 - 속성 - 대상 - 끝에 -D -u wizard 추가

## 연락처 (Contact)
4492kjh@naver.com
