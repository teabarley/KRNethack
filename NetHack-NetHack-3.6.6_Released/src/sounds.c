/* NetHack 3.6	sounds.c	$NHDT-Date: 1570844005 2019/10/12 01:33:25 $  $NHDT-Branch: NetHack-3.6 $:$NHDT-Revision: 1.83 $ */
/* Copyright (c) 1989 Janet Walz, Mike Threepoint */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

STATIC_DCL boolean FDECL(mon_is_gecko, (struct monst *) );
STATIC_DCL int FDECL(domonnoise, (struct monst *) );
STATIC_DCL int NDECL(dochat);
STATIC_DCL int FDECL(mon_in_room, (struct monst *, int) );

/* this easily could be a macro, but it might overtax dumb compilers */
STATIC_OVL int
mon_in_room(mon, rmtyp)
struct monst *mon;
int rmtyp;
{
    int rno = levl[mon->mx][mon->my].roomno;
    if (rno >= ROOMOFFSET)
        return rooms[rno - ROOMOFFSET].rtype == rmtyp;
    return FALSE;
}

void
dosounds()
{
    register struct mkroom *sroom;
    register int hallu, vx, vy;
#if defined(AMIGA) && defined(AZTEC_C_WORKAROUND)
    int xx;
#endif
    struct monst *mtmp;

    if (Deaf || !flags.acoustics || u.uswallow || Underwater)
        return;

    hallu = Hallucination ? 1 : 0;

    if (level.flags.nfountains && !rn2(400)) {
#if 0 /*KR: 원본*/
        static const char *const fountain_msg[4] = {
            "bubbling water.", "water falling on coins.",
            "the splashing of a naiad.", "a soda fountain!",
#else /*KR: KRNethack 맞춤 번역 */
        static const char *const fountain_msg[4] = {
            "물이 부글거리는 소리를 들었다.",
            "동전 위에 물이 떨어지는 소리를 들었다.",
            "물의 요정이 물장구치는 소리를 들었다.",
            "탄산음료의 보글거리는 소리를 들었다!",
#endif
        };
        You_hear1(fountain_msg[rn2(3) + hallu]);
    }
    if (level.flags.nsinks && !rn2(300)) {
#if 0 /*KR: 원본*/
        static const char *const sink_msg[3] = {
            "a slow drip.", "a gurgling noise.", "dishes being washed!",
#else /*KR: KRNethack 맞춤 번역 */
        static const char *const sink_msg[3] = {
            "물이 천천히 똑똑 떨어지는 소리를 들었다.",
            "물이 꿀럭꿀럭 넘어가는 소리를 들었다.",
            "접시 닦는 소리를 들었다!",
#endif
        };
        You_hear1(sink_msg[rn2(2) + hallu]);
    }
    if (level.flags.has_court && !rn2(200)) {
#if 0 /*KR: 원본*/
        static const char *const throne_msg[4] = {
            "the tones of courtly conversation.",
            "a sceptre pounded in judgment.",
            "Someone shouts \"Off with %s head!\"", "Queen Beruthiel's cats!",
#else /*KR: KRNethack 맞춤 번역 */
        static const char *const throne_msg[4] = {
            "궁정에서 오가는 대화 소리를 들었다.",
            "재판 중에 지휘봉을 두드리는 소리를 들었다.",
            "누군가가 \"%s 목을 쳐라!\"라고 외치는 소리를 들었다.",
            "베루시엘 왕비의 고양이들이 내는 소리를 들었다!",
#endif
        };
        for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
            if (DEADMONSTER(mtmp))
                continue;
            if ((mtmp->msleeping || is_lord(mtmp->data)
                 || is_prince(mtmp->data))
                && !is_animal(mtmp->data) && mon_in_room(mtmp, COURT)) {
                /* finding one is enough, at least for now */
                int which = rn2(3) + hallu;

                if (which != 2)
                    You_hear1(throne_msg[which]);
                else
                    pline(throne_msg[2], uhis());
                return;
            }
        }
    }
    if (level.flags.has_swamp && !rn2(200)) {
#if 0 /*KR: 원본*/
        static const char *const swamp_msg[3] = {
            "hear mosquitoes!", "smell marsh gas!", /* so it's a smell...*/
            "hear Donald Duck!",
#else /*KR: KRNethack 맞춤 번역 */
        static const char *const swamp_msg[3] = {
            "모기 소리를 들었다!",
            "늪지 가스 냄새를 맡았다!", /* so it's a smell...*/
            "도널드 덕의 소리를 들었다!",
#endif
        };
        You1(swamp_msg[rn2(2) + hallu]);
        return;
    }
    if (level.flags.has_vault && !rn2(200)) {
        if (!(sroom = search_special(VAULT))) {
            /* strange ... */
            level.flags.has_vault = 0;
            return;
        }
        if (gd_sound())
            switch (rn2(2) + hallu) {
            case 1: {
                boolean gold_in_vault = FALSE;

                for (vx = sroom->lx; vx <= sroom->hx; vx++)
                    for (vy = sroom->ly; vy <= sroom->hy; vy++)
                        if (g_at(vx, vy))
                            gold_in_vault = TRUE;
#if defined(AMIGA) && defined(AZTEC_C_WORKAROUND)
                /* Bug in aztec assembler here. Workaround below */
                xx = ROOM_INDEX(sroom) + ROOMOFFSET;
                xx = (xx != vault_occupied(u.urooms));
                if (xx)
#else
            if (vault_occupied(u.urooms) != (ROOM_INDEX(sroom) + ROOMOFFSET))
#endif /* AZTEC_C_WORKAROUND */
                {
                    if (gold_in_vault)
#if 0 /*KR: 원본*/
                        You_hear(!hallu
                                     ? "someone counting money."
                                     : "the quarterback calling the play.");
#else /*KR: KRNethack 맞춤 번역 */
                        You(
                            !hallu
                                ? "누군가 돈을 세는 소리를 들었다."
                                : "쿼터백이 작전을 지시하는 소리를 들었다.");
#endif
                    else
                        /*KR You_hear("someone searching."); */
                        You("누군가 찾는 소리를 들었다.");
                    break;
                }
            }
                /*FALLTHRU*/
            case 0:
                /*KR You_hear("the footsteps of a guard on patrol."); */
                You("경비병이 순찰하는 발소리를 들었다.");
                break;
            case 2:
                /*KR You_hear("Ebenezer Scrooge!"); */
                You("에비니저 스크루지의 소리를 들었다!");
                break;
            }
        return;
    }
    if (level.flags.has_beehive && !rn2(200)) {
        for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
            if (DEADMONSTER(mtmp))
                continue;
            if ((mtmp->data->mlet == S_ANT && is_flyer(mtmp->data))
                && mon_in_room(mtmp, BEEHIVE)) {
                switch (rn2(2) + hallu) {
                case 0:
                    /*KR You_hear("a low buzzing."); */
                    You("낮게 웅웅거리는 소리를 들었다.");
                    break;
                case 1:
                    /*KR You_hear("an angry drone."); */
                    You("성난 윙윙거리는 소리를 들었다.");
                    break;
                case 2:
#if 0 /*KR: 원본*/
                    You_hear("bees in your %sbonnet!",
                             uarmh ? "" : "(nonexistent) ");
#else /*KR: KRNethack 맞춤 번역 */
                    You("당신의 %s모자 안에 벌이 있는 소리를 들었다!",
                             uarmh ? "" : "(존재하지도 않는) ");
#endif
                    break;
                }
                return;
            }
        }
    }
    if (level.flags.has_morgue && !rn2(200)) {
        for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
            if (DEADMONSTER(mtmp))
                continue;
            if ((is_undead(mtmp->data) || is_vampshifter(mtmp))
                && mon_in_room(mtmp, MORGUE)) {
#if 0 /*KR: 원본*/
                const char *hair = body_part(HAIR); /* hair/fur/scales */
#else
                const char *hair = body_part(HAIR);
#endif

                switch (rn2(2) + hallu) {
                case 0:
                    /*KR You("suddenly realize it is unnaturally quiet."); */
                    You("갑자기 부자연스러울 정도로 조용하다는 것을 "
                        "깨달았다.");
                    break;
                case 1:
#if 0 /*KR: 원본*/
                    pline_The("%s on the back of your %s %s up.", hair,
                              body_part(NECK), vtense(hair, "stand"));
#else /*KR: KRNethack 맞춤 번역 */
                    pline("당신 %s 뒤쪽의 %s 곤두섰다.", body_part(NECK),
                          append_josa(body_part(HAIR), "이"));
#endif
                    break;
                case 2:
#if 0 /*KR: 원본*/
                    pline_The("%s on your %s %s to stand up.", hair,
                              body_part(HEAD), vtense(hair, "seem"));
#else /*KR: KRNethack 맞춤 번역 */
                    pline("당신 %s의 %s 쭈뼛 서는 것 같다.", body_part(HEAD),
                          append_josa(body_part(HAIR), "이"));
#endif
                    break;
                }
                return;
            }
        }
    }
    if (level.flags.has_barracks && !rn2(200)) {
        static const char *const barracks_msg[4] = {
#if 0 /*KR: 원본*/
            "blades being honed.", "loud snoring.", "dice being thrown.",
            "General MacArthur!",
#else /*KR: KRNethack 맞춤 번역 */
            "칼을 가는 소리를 들었다.",
            "큰 코골이 소리를 들었다.",
            "주사위를 던지는 소리를 들었다.",
            "맥아더 장군의 목소리를 들었다!",
#endif
        };
        int count = 0;

        for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
            if (DEADMONSTER(mtmp))
                continue;
            if (is_mercenary(mtmp->data)
#if 0 /* don't bother excluding these */
                && !strstri(mtmp->data->mname, "watch")
                && !strstri(mtmp->data->mname, "guard")
#endif
                && mon_in_room(mtmp, BARRACKS)
                /* sleeping implies not-yet-disturbed (usually) */
                && (mtmp->msleeping || ++count > 5)) {
                You_hear1(barracks_msg[rn2(3) + hallu]);
                return;
            }
        }
    }
    if (level.flags.has_zoo && !rn2(200)) {
        static const char *const zoo_msg[3] = {
#if 0 /*KR: 원본*/
            "a sound reminiscent of an elephant stepping on a peanut.",
            "a sound reminiscent of a seal barking.", "Doctor Dolittle!",
#else /*KR: KRNethack 맞춤 번역 */
            "코끼리가 땅콩을 밟는 듯한 소리를 들었다.",
            "물개가 짖는 듯한 소리를 들었다.",
            "두리틀 선생의 소리를 들었다!",
#endif
        };
        for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
            if (DEADMONSTER(mtmp))
                continue;
            if ((mtmp->msleeping || is_animal(mtmp->data))
                && mon_in_room(mtmp, ZOO)) {
                You_hear1(zoo_msg[rn2(2) + hallu]);
                return;
            }
        }
    }
    if (level.flags.has_shop && !rn2(200)) {
        if (!(sroom = search_special(ANY_SHOP))) {
            /* strange... */
            level.flags.has_shop = 0;
            return;
        }
        if (tended_shop(sroom)
            && !index(u.ushops, (int) (ROOM_INDEX(sroom) + ROOMOFFSET))) {
            static const char *const shop_msg[3] = {
#if 0 /*KR: 원본*/
                "someone cursing shoplifters.",
                "the chime of a cash register.", "Neiman and Marcus arguing!",
#else /*KR: KRNethack 맞춤 번역 */
                "누군가 좀도둑을 욕하는 소리를 들었다.",
                "금전등록기가 짤랑거리는 소리를 들었다.",
                "니만과 마커스가 말다툼하는 소리를 들었다!",
#endif
            };
            You_hear1(shop_msg[rn2(2) + hallu]);
        }
        return;
    }
    if (level.flags.has_temple && !rn2(200)
        && !(Is_astralevel(&u.uz) || Is_sanctum(&u.uz))) {
        for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
            if (DEADMONSTER(mtmp))
                continue;
            if (mtmp->ispriest
                && inhistemple(mtmp)
                /* priest must be active */
                && mtmp->mcanmove
                && !mtmp->msleeping
                /* hero must be outside this temple */
                && temple_occupied(u.urooms) != EPRI(mtmp)->shroom)
                break;
        }
        if (mtmp) {
            /* Generic temple messages; no attempt to match topic or tone
               to the pantheon involved, let alone to the specific deity.
               These are assumed to be coming from the attending priest;
               asterisk means that the priest must be capable of speech;
               pound sign (octathorpe,&c--don't go there) means that the
               priest and the altar must not be directly visible (we don't
               care if telepathy or extended detection reveals that the
               priest is not currently standing on the altar; he's mobile). */
            static const char *const temple_msg[] = {
#if 0 /*KR: 원본*/
                "*someone praising %s.", "*someone beseeching %s.",
                "#an animal carcass being offered in sacrifice.",
                "*a strident plea for donations.",
#else /*KR: KRNethack 맞춤 번역 */
                "*누군가 %s에게 찬양하는 소리를 들었다.",
                "*누군가 %s에게 간청하는 소리를 들었다.",
                "#동물 사체를 제물로 바치는 소리를 들었다.",
                "*귀에 거슬리게 기부를 부탁하는 소리를 들었다.",
#endif
            };
            const char *msg;
            int trycount = 0, ax = EPRI(mtmp)->shrpos.x,
                ay = EPRI(mtmp)->shrpos.y;
            boolean speechless = (mtmp->data->msound <= MS_ANIMAL),
                    in_sight = canseemon(mtmp) || cansee(ax, ay);

            do {
                msg = temple_msg[rn2(SIZE(temple_msg) - 1 + hallu)];
                if (index(msg, '*') && speechless)
                    continue;
                if (index(msg, '#') && in_sight)
                    continue;
                break; /* msg is acceptable */
            } while (++trycount < 50);
            while (*msg == '*' || *msg == '#')
                ++msg; /* skip control flags */
            if (index(msg, '%'))
                You_hear(msg, halu_gname(EPRI(mtmp)->shralign));
            else
                You_hear1(msg);
            return;
        }
    }
    if (Is_oracle_level(&u.uz) && !rn2(400)) {
        /* make sure the Oracle is still here */
        for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
            if (DEADMONSTER(mtmp))
                continue;
            if (mtmp->data == &mons[PM_ORACLE])
                break;
        }
        /* and don't produce silly effects when she's clearly visible */
        if (mtmp && (hallu || !canseemon(mtmp))) {
            static const char *const ora_msg[5] = {
#if 0 /*KR: 원본*/
                "a strange wind.",     /* Jupiter at Dodona */
                "convulsive ravings.", /* Apollo at Delphi */
                "snoring snakes.",     /* AEsculapius at Epidaurus */
                "someone say \"No more woodchucks!\"",
                "a loud ZOT!" /* both rec.humor.oracle */
#else /*KR: KRNethack 맞춤 번역 */
                "기묘한 바람 소리를 들었다.",
                "발작적으로 헛소리하는 것을 들었다.",
                "뱀이 코 고는 소리를 들었다.",
                "누군가 \"마멋은 이제 그만!\"이라고 말하는 것을 들었다.",
                "큰 ZOT 소리를 들었다!"
#endif
            };
            You_hear1(ora_msg[rn2(3) + hallu * 2]);
        }
        return;
    }
}

static const char *const h_sounds[] = {
#if 0 /*KR: 원본*/
    "beep",   "boing",   "sing",   "belche", "creak",   "cough",
    "rattle", "ululate", "pop",    "jingle", "sniffle", "tinkle",
    "eep",    "clatter", "hum",    "sizzle", "twitter", "wheeze",
    "rustle", "honk",    "lisp",   "yodel",  "coo",     "burp",
    "moo",    "boom",    "murmur", "oink",   "quack",   "rumble",
    "twang",  "bellow",  "toot",   "gargle", "hoot",    "warble"
#else /*KR: KRNethack 맞춤 번역 */
    "삑 소리를 내",   "팅 소리를 내", "노래하",
    "트림하",         "삐걱거리",     "기침하",
    "덜그럭거리",     "울부짖",       "펑 소리를 내",
    "딸랑거리",       "훌쩍거리",     "짤랑거리",
    "이입 소리를 내", "달그락거리",   "콧노래를 부르",
    "지글거리",       "지저귀",       "씨근거리",
    "바스락거리",     "빵빵거리",     "혀 짧은 소리를 내",
    "요들송을 부르",  "구구거리",     "트림하",
    "음메 하고 우",   "쾅 소리를 내", "중얼거리",
    "꿀꿀거리",       "꽥꽥거리",     "우르릉거리",
    "핑 소리를 내",   "고함치",       "뚜 소리를 내",
    "가글하",         "부엉부엉 우",  "지저귀"
#endif
};

const char *
growl_sound(mtmp)
register struct monst *mtmp;
{
    const char *ret;

    switch (mtmp->data->msound) {
    case MS_MEW:
    case MS_HISS:
        /*KR ret = "hiss"; */
        ret = "하악거렸다";
        break;
    case MS_BARK:
    case MS_GROWL:
        /*KR ret = "growl"; */
        ret = "으르렁거렸다";
        break;
    case MS_ROAR:
        /*KR ret = "roar"; */
        ret = "포효했다";
        break;
    case MS_BUZZ:
        /*KR ret = "buzz"; */
        ret = "윙윙거렸다";
        break;
    case MS_SQEEK:
        /*KR ret = "squeal"; */
        ret = "꽥꽥거렸다";
        break;
    case MS_SQAWK:
        /*KR ret = "screech"; */
        ret = "날카로운 비명을 질렀다";
        break;
    case MS_NEIGH:
        /*KR ret = "neigh"; */
        ret = "히힝 하고 울었다";
        break;
    case MS_WAIL:
        /*KR ret = "wail"; */
        ret = "울부짖었다";
        break;
    case MS_SILENT:
        /*KR ret = "commotion"; */
        ret = "소동을 일으켰다";
        break;
    default:
        /*KR ret = "scream"; */
        ret = "비명을 질렀다";
    }
    return ret;
}

/* the sounds of a seriously abused pet, including player attacking it */
void growl(mtmp) register struct monst *mtmp;
{
    register const char *growl_verb = 0;

    if (mtmp->msleeping || !mtmp->mcanmove || !mtmp->data->msound)
        return;

    /* presumably nearness and soundok checks have already been made */
    if (Hallucination)
        growl_verb = h_sounds[rn2(SIZE(h_sounds))];
    else
        growl_verb = growl_sound(mtmp);
    if (growl_verb) {
#if 0 /*KR: 원본*/
        pline("%s %s!", Monnam(mtmp), vtense((char *) 0, growl_verb));
#else /*KR: KRNethack 맞춤 번역 */
        pline("%s %s!", append_josa(Monnam(mtmp), "이"), growl_verb);
#endif
        if (context.run)
            nomul(0);
        wake_nearto(mtmp->mx, mtmp->my, mtmp->data->mlevel * 18);
    }
}

/* the sounds of mistreated pets */
void yelp(mtmp) register struct monst *mtmp;
{
    register const char *yelp_verb = 0;

    if (mtmp->msleeping || !mtmp->mcanmove || !mtmp->data->msound)
        return;

    /* presumably nearness and soundok checks have already been made */
    if (Hallucination)
        yelp_verb = h_sounds[rn2(SIZE(h_sounds))];
    else
        switch (mtmp->data->msound) {
        case MS_MEW:
            /*KR yelp_verb = (!Deaf) ? "yowl" : "arch"; */
            yelp_verb = (!Deaf) ? "길게 울부짖었다" : "몸을 구부렸다";
            break;
        case MS_BARK:
        case MS_GROWL:
            /*KR yelp_verb = (!Deaf) ? "yelp" : "recoil"; */
            yelp_verb = (!Deaf) ? "깽깽거렸다" : "움찔했다";
            break;
        case MS_ROAR:
            /*KR yelp_verb = (!Deaf) ? "snarl" : "bluff"; */
            yelp_verb = (!Deaf) ? "으르렁거렸다" : "허세를 부렸다";
            break;
        case MS_SQEEK:
            /*KR yelp_verb = (!Deaf) ? "squeal" : "quiver"; */
            yelp_verb = (!Deaf) ? "꽥꽥거렸다" : "떨었다";
            break;
        case MS_SQAWK:
            /*KR yelp_verb = (!Deaf) ? "screak" : "thrash"; */
            yelp_verb = (!Deaf) ? "끼익 소리를 냈다" : "요동쳤다";
            break;
        case MS_WAIL:
            /*KR yelp_verb = (!Deaf) ? "wail" : "cringe"; */
            yelp_verb = (!Deaf) ? "울부짖었다" : "움츠러들었다";
            break;
        }
    if (yelp_verb) {
#if 0 /*KR: 원본*/
        pline("%s %s!", Monnam(mtmp), vtense((char *) 0, yelp_verb));
#else /*KR: KRNethack 맞춤 번역 */
        pline("%s %s!", append_josa(Monnam(mtmp), "이"), yelp_verb);
#endif
        if (context.run)
            nomul(0);
        wake_nearto(mtmp->mx, mtmp->my, mtmp->data->mlevel * 12);
    }
}

/* the sounds of distressed pets */
void whimper(mtmp) register struct monst *mtmp;
{
    register const char *whimper_verb = 0;

    if (mtmp->msleeping || !mtmp->mcanmove || !mtmp->data->msound)
        return;

    /* presumably nearness and soundok checks have already been made */
    if (Hallucination)
        whimper_verb = h_sounds[rn2(SIZE(h_sounds))];
    else
        switch (mtmp->data->msound) {
        case MS_MEW:
        case MS_GROWL:
            /*KR whimper_verb = "whimper"; */
            whimper_verb = "낑낑거렸다";
            break;
        case MS_BARK:
            /*KR whimper_verb = "whine"; */
            whimper_verb = "끙끙거렸다";
            break;
        case MS_SQEEK:
            /*KR whimper_verb = "squeal"; */
            whimper_verb = "꽥꽥거렸다";
            break;
        }
    if (whimper_verb) {
#if 0 /*KR: 원본*/
        pline("%s %s.", Monnam(mtmp), vtense((char *) 0, whimper_verb));
#else /*KR: KRNethack 맞춤 번역 */
        pline("%s %s.", append_josa(Monnam(mtmp), "이"), whimper_verb);
#endif
        if (context.run)
            nomul(0);
        wake_nearto(mtmp->mx, mtmp->my, mtmp->data->mlevel * 6);
    }
}

/* pet makes "I'm hungry" noises */
void beg(mtmp) register struct monst *mtmp;
{
    if (mtmp->msleeping || !mtmp->mcanmove
        || !(carnivorous(mtmp->data) || herbivorous(mtmp->data)))
        return;

    /* presumably nearness and soundok checks have already been made */
    if (!is_silent(mtmp->data) && mtmp->data->msound <= MS_ANIMAL)
        (void) domonnoise(mtmp);
    else if (mtmp->data->msound >= MS_HUMANOID) {
        if (!canspotmon(mtmp))
            map_invisible(mtmp->mx, mtmp->my);
        /*KR verbalize("I'm hungry."); */
        verbalize("배고파.");
    }
}

/* return True if mon is a gecko or seems to look like one (hallucination) */
STATIC_OVL boolean
mon_is_gecko(mon)
struct monst *mon;
{
    int glyph;

    /* return True if it is actually a gecko */
    if (mon->data == &mons[PM_GECKO])
        return TRUE;
    /* return False if it is a long worm; we might be chatting to its tail
       (not strictly needed; long worms are MS_SILENT so won't get here) */
    if (mon->data == &mons[PM_LONG_WORM])
        return FALSE;
    /* result depends upon whether map spot shows a gecko, which will
       be due to hallucination or to mimickery since mon isn't one */
    glyph = glyph_at(mon->mx, mon->my);
    return (boolean) (glyph_to_mon(glyph) == PM_GECKO);
}

STATIC_OVL int
domonnoise(mtmp)
register struct monst *mtmp;
{
    char verbuf[BUFSZ];
    register const char *pline_msg = 0, /* Monnam(mtmp) will be prepended */
        *verbl_msg = 0,                 /* verbalize() */
            *verbl_msg_mcan = 0;        /* verbalize() if cancelled */
    struct permonst *ptr = mtmp->data;
    int msound = ptr->msound, gnomeplan = 0;

    /* presumably nearness and sleep checks have already been made */
    if (Deaf)
        return 0;
    if (is_silent(ptr))
        return 0;

    /* leader might be poly'd; if he can still speak, give leader speech */
    if (mtmp->m_id == quest_status.leader_m_id && msound > MS_ANIMAL)
        msound = MS_LEADER;
    /* make sure it's your role's quest guardian; adjust if not */
    else if (msound == MS_GUARDIAN && ptr != &mons[urole.guardnum])
        msound = mons[genus(monsndx(ptr), 1)].msound;
    /* some normally non-speaking types can/will speak if hero is similar */
    else if (msound == MS_ORC /* note: MS_ORC is same as MS_GRUNT */
             && ((same_race(ptr, youmonst.data)          /* current form, */
                  || same_race(ptr, &mons[Race_switch])) /* unpoly'd form */
                 || Hallucination))
        msound = MS_HUMANOID;
    /* silliness, with slight chance to interfere with shopping */
    else if (Hallucination && mon_is_gecko(mtmp))
        msound = MS_SELL;

    /* be sure to do this before talking; the monster might teleport away, in
     * which case we want to check its pre-teleport position
     */
    if (!canspotmon(mtmp))
        map_invisible(mtmp->mx, mtmp->my);

    switch (msound) {
    case MS_ORACLE:
        return doconsult(mtmp);
    case MS_PRIEST:
        priest_talk(mtmp);
        break;
    case MS_LEADER:
    case MS_NEMESIS:
    case MS_GUARDIAN:
        quest_chat(mtmp);
        break;
    case MS_SELL: /* pitch, pay, total */
        if (!Hallucination || (mtmp->isshk && !rn2(2))) {
            shk_chat(mtmp);
        } else {
            /* approximation of GEICO's advertising slogan (it actually
               concludes with "save you 15% or more on car insurance.") */
#if 0 /*KR: 원본*/
            Sprintf(verbuf, "15 minutes could save you 15 %s.",
                    currency(15L)); /* "zorkmids" */
#else /*KR: KRNethack 맞춤 번역 */
            Sprintf(verbuf, "15분이면 15 %s 아낄 수 있습니다.",
                    append_josa(currency(15L), "을")); /* "zorkmids" */
#endif
            verbl_msg = verbuf;
        }
        break;
    case MS_VAMPIRE: {
        /* vampire messages are varied by tameness, peacefulness, and time of
         * night */
        boolean isnight = night();
        boolean kindred =
            (Upolyd
             && (u.umonnum == PM_VAMPIRE || u.umonnum == PM_VAMPIRE_LORD));
        boolean nightchild =
            (Upolyd
             && (u.umonnum == PM_WOLF || u.umonnum == PM_WINTER_WOLF
                 || u.umonnum == PM_WINTER_WOLF_CUB));
#if 0 /*KR: 원본*/
        const char *racenoun =
            (flags.female && urace.individual.f)
                ? urace.individual.f
                : (urace.individual.m) ? urace.individual.m : urace.noun;
#else /*KR: KRNethack 맞춤 번역 */
        const char *racenoun = (flags.female) ? "계집" : "사내";
#endif

        if (mtmp->mtame) {
            if (kindred) {
#if 0 /*KR: 원본*/
                Sprintf(verbuf, "Good %s to you Master%s",
                        isnight ? "evening" : "day",
                        isnight ? "!" : ".  Why do we not rest?");
#else /*KR: KRNethack 맞춤 번역 */
                Sprintf(verbuf, "주인님, 좋은 %s%s",
                        isnight ? "저녁입니다" : "하루입니다",
                        isnight ? "!" : ". 어째서 쉬지 않으십니까?");
#endif
                verbl_msg = verbuf;
            } else {
#if 0 /*KR: 원본*/
                Sprintf(verbuf, "%s%s",
                        nightchild ? "Child of the night, " : "",
                        midnight()
                         ? "I can stand this craving no longer!"
                         : isnight
                          ? "I beg you, help me satisfy this growing craving!"
                          : "I find myself growing a little weary.");
#else /*KR: KRNethack 맞춤 번역 */
                Sprintf(verbuf, "%s%s", nightchild ? "밤의 아이여, " : "",
                        midnight() ? "이 갈망을 더 이상 참을 수 없네!"
                        : isnight
                            ? "부탁이니, 이 커져가는 갈망을 채우게 도와다오!"
                            : "조금 지치는 것 같군.");
#endif
                verbl_msg = verbuf;
            }
        } else if (mtmp->mpeaceful) {
            if (kindred && isnight) {
#if 0 /*KR: 원본*/
                Sprintf(verbuf, "Good feeding %s!",
                        flags.female ? "sister" : "brother");
#else /*KR: KRNethack 맞춤 번역 */
                Sprintf(verbuf, "식사 잘하게, %s!",
                        flags.female ? "자매여" : "형제여");
#endif
                verbl_msg = verbuf;
            } else if (nightchild && isnight) {
                /*KR Sprintf(verbuf, "How nice to hear you, child of the
                 * night!"); */
                Sprintf(verbuf, "네 소리를 들으니 참 좋구나, 밤의 아이여!");
                verbl_msg = verbuf;
            } else
                /*KR verbl_msg = "I only drink... potions."; */
                verbl_msg = "나는 그저... 물약만 마신다.";
        } else {
            static const char *const vampmsg[] = {
                /* These first two (0 and 1) are specially handled below */
                /*KR "I vant to suck your %s!", */
                "너의 피를 빨아먹고 싶다!",
                /*KR "I vill come after %s without regret!", */
                "후회 없이 %s 쫓아가겠다!",
                /* other famous vampire quotes can follow here if desired */
            };
            int vampindex;

            if (kindred)
                /*KR verbl_msg = "This is my hunting ground that you dare to
                 * prowl!"; */
                verbl_msg =
                    "이곳은 내 사냥터인데 네가 감히 어슬렁거리는구나!";
            else if (youmonst.data == &mons[PM_SILVER_DRAGON]
                     || youmonst.data == &mons[PM_BABY_SILVER_DRAGON]) {
                /* Silver dragons are silver in color, not made of silver */
#if 0 /*KR: 원본*/
                Sprintf(verbuf, "%s!  Your silver sheen does not frighten me!",
                        youmonst.data == &mons[PM_SILVER_DRAGON]
                            ? "Fool"
                            : "Young Fool");
                verbl_msg = verbuf;
#else /*KR: KRNethack 맞춤 번역 */
                Sprintf(verbuf, "%s! 네놈의 은빛 광채 따위는 두렵지 않다!",
                        youmonst.data == &mons[PM_SILVER_DRAGON]
                            ? "어리석은 놈"
                            : "어리석은 애송이");
                verbl_msg = verbuf;
#endif
            } else {
                vampindex = rn2(SIZE(vampmsg));
                if (vampindex == 0) {
                    Sprintf(verbuf, vampmsg[vampindex], body_part(BLOOD));
                    verbl_msg = verbuf;
                } else if (vampindex == 1) {
                    Sprintf(verbuf, vampmsg[vampindex],
                            Upolyd ? an(mons[u.umonnum].mname)
                                   : an(racenoun));
                    verbl_msg = verbuf;
                } else
                    verbl_msg = vampmsg[vampindex];
            }
        }
    } break;
    case MS_WERE:
        if (flags.moonphase == FULL_MOON && (night() ^ !rn2(13))) {
#if 0 /*KR: 원본*/
            pline("%s throws back %s head and lets out a blood curdling %s!",
                  Monnam(mtmp), mhis(mtmp),
                  ptr == &mons[PM_HUMAN_WERERAT] ? "shriek" : "howl");
#else /*KR: KRNethack 맞춤 번역 */
            pline("%s 고개를 뒤로 젖히더니 피가 얼어붙을 듯한 %s 내질렀다!",
                  append_josa(Monnam(mtmp), "은"),
                  ptr == &mons[PM_HUMAN_WERERAT] ? "비명을" : "울부짖음을");
#endif
            wake_nearto(mtmp->mx, mtmp->my, 11 * 11);
        } else
            /*KR pline_msg = "whispers inaudibly.  All you can make out is
             * \"moon\"."; */
            pline_msg = "알아들을 수 없게 속삭였다. 들리는 것이라고는 "
                        "\"달\"뿐이었다.";
        break;
    case MS_BARK:
        if (flags.moonphase == FULL_MOON && night()) {
            /*KR pline_msg = "howls."; */
            pline_msg = "울부짖었다.";
        } else if (mtmp->mpeaceful) {
            if (mtmp->mtame
                && (mtmp->mconf || mtmp->mflee || mtmp->mtrapped
                    || moves > EDOG(mtmp)->hungrytime || mtmp->mtame < 5))
                /*KR pline_msg = "whines."; */
                pline_msg = "끙끙거렸다.";
            else if (mtmp->mtame && EDOG(mtmp)->hungrytime > moves + 1000)
                /*KR pline_msg = "yips."; */
                pline_msg = "깽깽거렸다.";
            else {
                if (mtmp->data
                    != &mons[PM_DINGO]) /* dingos do not actually bark */
                    /*KR pline_msg = "barks."; */
                    pline_msg = "짖었다.";
            }
        } else {
            /*KR pline_msg = "growls."; */
            pline_msg = "으르렁거렸다.";
        }
        break;
    case MS_MEW:
        if (mtmp->mtame) {
            if (mtmp->mconf || mtmp->mflee || mtmp->mtrapped
                || mtmp->mtame < 5)
                /*KR pline_msg = "yowls."; */
                pline_msg = "울부짖었다.";
            else if (moves > EDOG(mtmp)->hungrytime)
                /*KR pline_msg = "meows."; */
                pline_msg = "야옹거렸다.";
            else if (EDOG(mtmp)->hungrytime > moves + 1000)
                /*KR pline_msg = "purrs."; */
                pline_msg = "가르랑거렸다.";
            else
                /*KR pline_msg = "mews."; */
                pline_msg = "야옹하고 울었다.";
            break;
        }
        /*FALLTHRU*/
    case MS_GROWL:
        /*KR pline_msg = mtmp->mpeaceful ? "snarls." : "growls!"; */
        pline_msg =
            mtmp->mpeaceful ? "으르렁거렸다." : "사납게 으르렁거렸다!";
        break;
    case MS_ROAR:
        /*KR pline_msg = mtmp->mpeaceful ? "snarls." : "roars!"; */
        pline_msg = mtmp->mpeaceful ? "으르렁거렸다." : "포효했다!";
        break;
    case MS_SQEEK:
        /*KR pline_msg = "squeaks."; */
        pline_msg = "찍찍거렸다.";
        break;
    case MS_SQAWK:
        if (ptr == &mons[PM_RAVEN] && !mtmp->mpeaceful)
            /*KR verbl_msg = "Nevermore!"; */
            verbl_msg = "다시는 그러지 않으리!";
        else
            /*KR pline_msg = "squawks."; */
            pline_msg = "꽥꽥거렸다.";
        break;
    case MS_HISS:
        if (!mtmp->mpeaceful)
            /*KR pline_msg = "hisses!"; */
            pline_msg = "쉿 소리를 냈다!";
        else
            return 0; /* no sound */
        break;
    case MS_BUZZ:
        /*KR pline_msg = mtmp->mpeaceful ? "drones." : "buzzes angrily."; */
        pline_msg = mtmp->mpeaceful ? "윙윙거렸다." : "화난 듯이 윙윙거렸다.";
        break;
    case MS_GRUNT:
        /*KR pline_msg = "grunts."; */
        pline_msg = "꿀꿀거렸다.";
        break;
    case MS_NEIGH:
        if (mtmp->mtame < 5)
            /*KR pline_msg = "neighs."; */
            pline_msg = "이힝 하고 울었다.";
        else if (moves > EDOG(mtmp)->hungrytime)
            /*KR pline_msg = "whinnies."; */
            pline_msg = "히힝 하고 울었다.";
        else
            /*KR pline_msg = "whickers."; */
            pline_msg = "푸루루 하고 울었다.";
        break;
    case MS_WAIL:
        /*KR pline_msg = "wails mournfully."; */
        pline_msg = "구슬프게 울부짖었다.";
        break;
    case MS_GURGLE:
        /*KR pline_msg = "gurgles."; */
        pline_msg = "꾸르륵 소리를 냈다.";
        break;
    case MS_BURBLE:
        /*KR pline_msg = "burbles."; */
        pline_msg = "웅얼거렸다.";
        break;
    case MS_SHRIEK:
        /*KR pline_msg = "shrieks."; */
        pline_msg = "새된 비명을 질렀다.";
        aggravate();
        break;
    case MS_IMITATE:
        /*KR pline_msg = "imitates you."; */
        pline_msg = "당신을 흉내 냈다.";
        break;
    case MS_BONES:
#if 0 /*KR: 원본*/
        pline("%s rattles noisily.", Monnam(mtmp));
#else /*KR: KRNethack 맞춤 번역 */
        pline("%s 시끄럽게 덜그럭거린다.",
              append_josa(Monnam(mtmp), "이"));
#endif
        /*KR You("freeze for a moment."); */
        You("잠시 얼어붙었다.");
        nomul(-2);
        /*KR multi_reason = "scared by rattling"; */
        multi_reason = "덜그럭거리는 소리에 놀라서";
        nomovemsg = 0;
        break;
    case MS_LAUGH: {
        static const char *const laugh_msg[4] = {
#if 0 /*KR: 원본*/
            "giggles.", "chuckles.", "snickers.", "laughs.",
#else /*KR: KRNethack 맞춤 번역 */
            "킥킥 웃었다.",
            "빙그레 웃었다.",
            "숨죽여 웃었다.",
            "웃었다.",
#endif
        };
        pline_msg = laugh_msg[rn2(4)];
    } break;
    case MS_MUMBLE:
        /*KR pline_msg = "mumbles incomprehensibly."; */
        pline_msg = "알아들을 수 없게 중얼거렸다.";
        break;
    case MS_DJINNI:
        if (mtmp->mtame) {
            /*KR verbl_msg = "Sorry, I'm all out of wishes."; */
            verbl_msg = "미안하지만, 내게 소원은 더 이상 안 남았다네.";
        } else if (mtmp->mpeaceful) {
            if (ptr == &mons[PM_WATER_DEMON])
                /*KR pline_msg = "gurgles."; */
                pline_msg = "꾸르륵거렸다.";
            else
                /*KR verbl_msg = "I'm free!"; */
                verbl_msg = "자유다!";
        } else {
            if (ptr != &mons[PM_PRISONER])
                /*KR verbl_msg = "This will teach you not to disturb me!"; */
                verbl_msg =
                    "이걸로 나를 방해하지 말아야 한다는 걸 배우게 될 거다!";
#if 0
            else
                verbl_msg = "??????????";
#endif
        }
        break;
    case MS_BOAST: /* giants */
        if (!mtmp->mpeaceful) {
            switch (rn2(4)) {
            case 0:
#if 0 /*KR: 원본*/
                pline("%s boasts about %s gem collection.", Monnam(mtmp),
                      mhis(mtmp));
#else /*KR: KRNethack 맞춤 번역 */
                pline("%s 자신의 보석 컬렉션을 자랑했다.",
                      append_josa(Monnam(mtmp), "은"));
#endif
                break;
            case 1:
                /*KR pline_msg = "complains about a diet of mutton."; */
                pline_msg = "양고기뿐인 식단에 대해 불평했다.";
                break;
            default:
                /*KR pline_msg = "shouts \"Fee Fie Foe Foo!\" and guffaws.";
                 */
                pline_msg = "\"피 파이 포 푸!\" 하고 외치더니 껄껄 웃었다.";
                wake_nearto(mtmp->mx, mtmp->my, 7 * 7);
                break;
            }
            break;
        }
        /*FALLTHRU*/
    case MS_HUMANOID:
        if (!mtmp->mpeaceful) {
            if (In_endgame(&u.uz) && is_mplayer(ptr))
                mplayer_talk(mtmp);
            else
                /*KR pline_msg = "threatens you."; */
                pline_msg = "당신을 위협했다.";
            break;
        }
        /* Generic peaceful humanoid behaviour. */
        if (mtmp->mflee)
            /*KR pline_msg = "wants nothing to do with you."; */
            pline_msg = "당신과 엮이고 싶어 하지 않는다.";
        else if (mtmp->mhp < mtmp->mhpmax / 4)
            /*KR pline_msg = "moans."; */
            pline_msg = "신음했다.";
        else if (mtmp->mconf || mtmp->mstun)
            /*KR verbl_msg = !rn2(3) ? "Huh?" : rn2(2) ? "What?" : "Eh?"; */
            verbl_msg = !rn2(3) ? "엉?" : rn2(2) ? "뭐?" : "에?";
        else if (!mtmp->mcansee)
            /*KR verbl_msg = "I can't see!"; */
            verbl_msg = "안 보여!";
        else if (mtmp->mtrapped) {
            struct trap *t = t_at(mtmp->mx, mtmp->my);

            if (t)
                t->tseen = 1;
            /*KR verbl_msg = "I'm trapped!"; */
            verbl_msg = "함정에 빠졌어!";
        } else if (mtmp->mhp < mtmp->mhpmax / 2)
            /*KR pline_msg = "asks for a potion of healing."; */
            pline_msg = "회복 물약을 부탁했다.";
        else if (mtmp->mtame && !mtmp->isminion
                 && moves > EDOG(mtmp)->hungrytime)
            /*KR verbl_msg = "I'm hungry."; */
            verbl_msg = "배고파.";
        /* Specific monsters' interests */
        else if (is_elf(ptr))
            /*KR pline_msg = "curses orcs."; */
            pline_msg = "오크를 저주했다.";
        else if (is_dwarf(ptr))
            /*KR pline_msg = "talks about mining."; */
            pline_msg = "채광에 대해 이야기했다.";
        else if (likes_magic(ptr))
            /*KR pline_msg = "talks about spellcraft."; */
            pline_msg = "주문 제작에 대해 이야기했다.";
        else if (ptr->mlet == S_CENTAUR)
            /*KR pline_msg = "discusses hunting."; */
            pline_msg = "사냥에 대해 논했다.";
        else if (is_gnome(ptr) && Hallucination && (gnomeplan = rn2(4)) % 2)
        /* skipped for rn2(4) result of 0 or 2;
           gag from an early episode of South Park called "Gnomes";
           initially, Tweek (introduced in that episode) is the only
           one aware of the tiny gnomes after spotting them sneaking
           about; they are embarked upon a three-step business plan;
           a diagram of the plan shows:
                     Phase 1         Phase 2      Phase 3
               Collect underpants       ?         Profit
           and they never verbalize step 2 so we don't either */
#if 0 /*KR: 원본*/
            verbl_msg = (gnomeplan == 1) ? "Phase one, collect underpants."
                                         : "Phase three, profit!";
#else /*KR: KRNethack 맞춤 번역 */
            verbl_msg = (gnomeplan == 1) ? "1단계, 팬티를 모은다."
                                         : "3단계, 수익 창출!";
#endif
        else
            switch (monsndx(ptr)) {
            case PM_HOBBIT:
                pline_msg = (mtmp->mhpmax - mtmp->mhp >= 10)
                                /*KR ? "complains about unpleasant dungeon
                                   conditions." */
                                ? "불쾌한 던전 환경에 대해 불평했다."
                                /*KR : "asks you about the One Ring."; */
                                : "당신에게 절대반지에 대해 물었다.";
                break;
            case PM_ARCHEOLOGIST:
                /*KR pline_msg = "describes a recent article in \"Spelunker
                 * Today\" magazine."; */
                pline_msg = "\"투데이 동굴 탐험가\" 잡지의 최근 기사에 대해 "
                            "설명했다.";
                break;
            case PM_TOURIST:
                /*KR verbl_msg = "Aloha."; */
                verbl_msg = "알로하.";
                break;
            default:
                /*KR pline_msg = "discusses dungeon exploration."; */
                pline_msg = "던전 탐험에 대해 논했다.";
                break;
            }
        break;
    case MS_SEDUCE: {
        int swval;

        if (SYSOPT_SEDUCE) {
            if (ptr->mlet != S_NYMPH
                && could_seduce(mtmp, &youmonst, (struct attack *) 0) == 1) {
                (void) doseduce(mtmp);
                break;
            }
            swval = ((poly_gender() != (int) mtmp->female) ? rn2(3) : 0);
        } else
            swval = ((poly_gender() == 0) ? rn2(3) : 0);
        switch (swval) {
        case 2:
#if 0 /*KR: 원본*/
            verbl_msg = "Hello, sailor.";
#else /*KR: KRNethack 맞춤 번역 */
            switch (poly_gender()) {
            case 0:
                verbl_msg = "안녕, 멋진 선원분.";
                break;
            case 1:
                verbl_msg = "안녕, 아가씨.";
                break;
            default:
                verbl_msg = "안녕하세요.";
                break;
            }
#endif
            break;
        case 1:
            /*KR pline_msg = "comes on to you."; */
            pline_msg = "당신에게 수작을 부렸다.";
            break;
        default:
            /*KR pline_msg = "cajoles 구슬렸다."; */
            pline_msg = "당신을 구슬렸다.";
        }
    } break;
    case MS_ARREST:
        if (mtmp->mpeaceful)
#if 0 /*KR: 원본*/
            verbalize("Just the facts, %s.", flags.female ? "Ma'am" : "Sir");
#else /*KR: KRNethack 맞춤 번역 */
            verbalize("사실만 말하십시오, %s.",
                      flags.female ? "부인" : "선생");
#endif
        else {
            static const char *const arrest_msg[3] = {
#if 0 /*KR: 원본*/
                "Anything you say can be used against you.",
                "You're under arrest!", "Stop in the name of the Law!",
#else /*KR: KRNethack 맞춤 번역 */
                "당신의 발언은 불리하게 작용할 수 있다.",
                "당신을 체포하겠다!",
                "법의 이름으로 멈춰라!",
#endif
            };
            verbl_msg = arrest_msg[rn2(3)];
        }
        break;
    case MS_BRIBE:
        if (mtmp->mpeaceful && !mtmp->mtame) {
            (void) demon_talk(mtmp);
            break;
        }
    /* fall through */
    case MS_CUSS:
        if (!mtmp->mpeaceful)
            cuss(mtmp);
        else if (is_lminion(mtmp))
            /*KR verbl_msg = "It's not too late."; */
            verbl_msg = "아직 늦지 않았다.";
        else
            /*KR verbl_msg = "We're all doomed."; */
            verbl_msg = "우린 다 끝났어.";
        break;
    case MS_SPELL:
        /* deliberately vague, since it's not actually casting any spell */
        /*KR pline_msg = "seems to mutter a cantrip."; */
        pline_msg = "주문을 중얼거리는 것 같다.";
        break;
    case MS_NURSE:
        /*KR verbl_msg_mcan = "I hate this job!"; */
        verbl_msg_mcan = "이 직업 정말 싫어!";
        if (uwep && (uwep->oclass == WEAPON_CLASS || is_weptool(uwep)))
            /*KR verbl_msg = "Put that weapon away before you hurt someone!";
             */
            verbl_msg = "누굴 다치게 하기 전에 그 무기 치우세요!";
        else if (uarmc || uarm || uarmh || uarms || uarmg || uarmf)
            verbl_msg =
                Role_if(PM_HEALER)
                    /*KR ? "Doc, I can't help you unless you cooperate." */
                    ? "선생님, 협조하지 않으시면 도와드릴 수가 없습니다."
                    /*KR : "Please undress so I can examine you."; */
                    : "진찰할 수 있게 옷을 벗어주세요.";
        else if (uarmu)
            /*KR verbl_msg = "Take off your shirt, please."; */
            verbl_msg = "셔츠를 벗어주세요.";
        else
            /*KR verbl_msg = "Relax, this won't hurt a bit."; */
            verbl_msg = "긴장 푸세요, 하나도 안 아플 겁니다.";
        break;
    case MS_GUARD:
        if (money_cnt(invent))
            /*KR verbl_msg = "Please drop that gold and follow me."; */
            verbl_msg = "그 금화를 내려놓고 따라오십시오.";
        else
            /*KR verbl_msg = "Please follow me."; */
            verbl_msg = "저를 따라오십시오.";
        break;
    case MS_SOLDIER: {
        static const char
            *const soldier_foe_msg[3] =
                {
#if 0 /*KR: 원본*/
                  "Resistance is useless!", "You're dog meat!", "Surrender!",
#else /*KR: KRNethack 맞춤 번역 */
                  "저항은 무의미하다!", "넌 개밥이 될 거다!", "항복해라!",
#endif
                },
                   *const soldier_pax_msg[3] = {
#if 0 /*KR: 원본*/
                       "What lousy pay we're getting here!",
                       "The food's not fit for Orcs!",
                       "My feet hurt, I've been on them all day!",
#else /*KR: KRNethack 맞춤 번역 */
                       "여기 급여가 너무 형편없군!",
                       "음식이 오크도 안 먹을 수준이야!",
                       "하루 종일 서 있었더니 발이 아프네!",
#endif
                   };
        verbl_msg = mtmp->mpeaceful ? soldier_pax_msg[rn2(3)]
                                    : soldier_foe_msg[rn2(3)];
        break;
    }
    case MS_RIDER: {
        const char *tribtitle;
        struct obj *book = 0;
        boolean ms_Death = (ptr == &mons[PM_DEATH]);

        /* 3.6 tribute */
        if (ms_Death && !context.tribute.Deathnotice
            && (book = u_have_novel()) != 0) {
            if ((tribtitle = noveltitle(&book->novelidx)) != 0) {
                /*KR Sprintf(verbuf, "Ah, so you have a copy of /%s/.",
                 * tribtitle); */
                Sprintf(verbuf, "아, /%s/의 사본을 가지고 있구나.",
                        tribtitle);
                /* no Death featured in these two, so exclude them */
                if (strcmpi(tribtitle, "Snuff")
                    && strcmpi(tribtitle, "The Wee Free Men"))
                    /*KR Strcat(verbuf, "  I may have been misquoted there.");
                     */
                    Strcat(verbuf,
                           "  거기서 내 말이 잘못 인용되었을 수도 있지.");
                verbl_msg = verbuf;
            }
            context.tribute.Deathnotice = 1;
        } else if (ms_Death && rn2(3) && Death_quote(verbuf, sizeof verbuf)) {
            verbl_msg = verbuf;
            /* end of tribute addition */

        } else if (ms_Death && !rn2(10)) {
            /*KR pline_msg = "is busy reading a copy of Sandman #8."; */
            pline_msg = "샌드맨 8권을 읽느라 바쁘다.";
        } else
            /*KR verbl_msg = "Who do you think you are, War?"; */
            verbl_msg = "네가 전쟁이라도 된다고 생각하느냐?";
        break;
    } /* case MS_RIDER */
    } /* switch */

    if (pline_msg) {
#if 0 /*KR: 원본*/
        pline("%s %s", Monnam(mtmp), pline_msg);
#else /*KR: KRNethack 맞춤 번역 */
        pline("%s %s", append_josa(Monnam(mtmp), "은"), pline_msg);
#endif
    } else if (mtmp->mcan && verbl_msg_mcan) {
        verbalize1(verbl_msg_mcan);
    } else if (verbl_msg) {
#if 0 /*KR: 원본*/
        /* more 3.6 tribute */
        if (ptr == &mons[PM_DEATH]) {
            /* Death talks in CAPITAL LETTERS
               and without quotation marks */
            char tmpbuf[BUFSZ];

            pline1(ucase(strcpy(tmpbuf, verbl_msg)));
        } else {
            verbalize1(verbl_msg);
        }
#else /*KR: KRNethack 맞춤 번역 */
        verbalize1(verbl_msg);
#endif
    }
    return 1;
}

/* #chat command */
int
dotalk()
{
    int result;

    result = dochat();
    return result;
}

STATIC_OVL int
dochat()
{
    struct monst *mtmp;
    int tx, ty;
    struct obj *otmp;

    if (is_silent(youmonst.data)) {
#if 0 /*KR: 원본*/
        pline("As %s, you cannot speak.", an(youmonst.data->mname));
#else /*KR: KRNethack 맞춤 번역 */
        pline("%s 상태로는 말할 수 없다.", youmonst.data->mname);
#endif
        return 0;
    }
    if (Strangled) {
        /*KR You_cant("speak.  You're choking!"); */
        You_cant("말할 수 없다. 숨이 막힌다!");
        return 0;
    }
    if (u.uswallow) {
        /*KR pline("They won't hear you out there."); */
        pline("밖에서는 네 목소리가 들리지 않을 거다.");
        return 0;
    }
    if (Underwater) {
        /*KR Your("speech is unintelligible underwater."); */
        Your("물속에서는 말이 알아들을 수 없다.");
        return 0;
    }
    if (Deaf) {
        /*KR pline("How can you hold a conversation when you cannot hear?");
         */
        pline("들을 수 없는데 어떻게 대화를 나눌 수 있겠는가?");
        return 0;
    }

    if (!Blind && (otmp = shop_object(u.ux, u.uy)) != (struct obj *) 0) {
        /* standing on something in a shop and chatting causes the shopkeeper
           to describe the price(s).  This can inhibit other chatting inside
           a shop, but that shouldn't matter much.  shop_object() returns an
           object iff inside a shop and the shopkeeper is present and willing
           (not angry) and able (not asleep) to speak and the position
           contains any objects other than just gold.
        */
        price_quote(otmp);
        return 1;
    }

    /*KR if (!getdir("Talk to whom? (in what direction)")) { */
    if (!getdir("누구와 대화하겠습니까? (방향 지정)")) {
        /* decided not to chat */
        return 0;
    }

    if (u.usteed && u.dz > 0) {
        if (!u.usteed->mcanmove || u.usteed->msleeping) {
#if 0 /*KR: 원본*/
            pline("%s seems not to notice you.", Monnam(u.usteed));
#else /*KR: KRNethack 맞춤 번역 */
            pline("%s 당신을 알아차리지 못한 것 같다.",
                  append_josa(Monnam(u.usteed), "은"));
#endif
            return 1;
        } else
            return domonnoise(u.usteed);
    }

    if (u.dz) {
        /*KR pline("They won't hear you %s there.", u.dz < 0 ? "up" : "down");
         */
        pline("거기 %s 당신의 목소리가 들리지 않을 것이다.",
              u.dz < 0 ? "위에서는" : "아래에서는");
        return 0;
    }

    if (u.dx == 0 && u.dy == 0) {
        /*
         * Let's not include this.
         * It raises all sorts of questions: can you wear
         * 2 helmets, 2 amulets, 3 pairs of gloves or 6 rings as a marilith,
         * etc...  --KAA
         if (u.umonnum == PM_ETTIN) {
             You("discover that your other head makes boring conversation.");
             return 1;
         }
         */
        /*KR pline("Talking to yourself is a bad habit for a dungeoneer."); */
        pline("혼잣말은 던전 탐험가에게 나쁜 습관이다.");
        return 0;
    }

    tx = u.ux + u.dx;
    ty = u.uy + u.dy;

    if (!isok(tx, ty))
        return 0;

    mtmp = m_at(tx, ty);

    if ((!mtmp || mtmp->mundetected) && (otmp = vobj_at(tx, ty)) != 0
        && otmp->otyp == STATUE) {
        /* Talking to a statue */
        if (!Blind) {
#if 0 /*KR: 원본*/
            pline_The("%s seems not to notice you.",
                      /* if hallucinating, you can't tell it's a statue */
                      Hallucination ? rndmonnam((char *) 0) : "statue");
#else /*KR: KRNethack 맞춤 번역 */
            pline("%s 당신을 알아차리지 못한 것 같다.",
                  append_josa(Hallucination ? rndmonnam((char *) 0) : "석상",
                              "은"));
#endif
        }
        return 0;
    }

    if (!mtmp || mtmp->mundetected || M_AP_TYPE(mtmp) == M_AP_FURNITURE
        || M_AP_TYPE(mtmp) == M_AP_OBJECT)
        return 0;

    /* sleeping monsters won't talk, except priests (who wake up) */
    if ((!mtmp->mcanmove || mtmp->msleeping) && !mtmp->ispriest) {
        /* If it is unseen, the player can't tell the difference between
           not noticing him and just not existing, so skip the message. */
        if (canspotmon(mtmp))
#if 0 /*KR: 원본*/
            pline("%s seems not to notice you.", Monnam(mtmp));
#else /*KR: KRNethack 맞춤 번역 */
            pline("%s 당신을 알아차리지 못한 것 같다.",
                  append_josa(Monnam(mtmp), "은"));
#endif
        return 0;
    }

    /* if this monster is waiting for something, prod it into action */
    mtmp->mstrategy &= ~STRAT_WAITMASK;

    if (mtmp->mtame && mtmp->meating) {
        if (!canspotmon(mtmp))
            map_invisible(mtmp->mx, mtmp->my);
#if 0 /*KR: 원본*/
        pline("%s is eating noisily.", Monnam(mtmp));
#else /*KR: KRNethack 맞춤 번역 */
        pline("%s 시끄럽게 먹고 있다.", append_josa(Monnam(mtmp), "은"));
#endif
        return 0;
    }

    return domonnoise(mtmp);
}

#ifdef USER_SOUNDS

extern void FDECL(play_usersound, (const char *, int) );

typedef struct audio_mapping_rec {
    struct nhregex *regex;
    char *filename;
    int volume;
    struct audio_mapping_rec *next;
} audio_mapping;

static audio_mapping *soundmap = 0;

char *sounddir = ".";

/* adds a sound file mapping, returns 0 on failure, 1 on success */
int add_sound_mapping(mapping) const char *mapping;
{
    char text[256];
    char filename[256];
    char filespec[256];
    int volume;

    if (sscanf(mapping, "MESG \"%255[^\"]\"%*[\t ]\"%255[^\"]\" %d", text,
               filename, &volume)
        == 3) {
        audio_mapping *new_map;

        if (strlen(sounddir) + strlen(filename) > 254) {
            raw_print("sound file name too long");
            return 0;
        }
        Sprintf(filespec, "%s/%s", sounddir, filename);

        if (can_read_file(filespec)) {
            new_map = (audio_mapping *) alloc(sizeof(audio_mapping));
            new_map->regex = regex_init();
            new_map->filename = dupstr(filespec);
            new_map->volume = volume;
            new_map->next = soundmap;

            if (!regex_compile(text, new_map->regex)) {
                raw_print(regex_error_desc(new_map->regex));
                regex_free(new_map->regex);
                free(new_map->filename);
                free(new_map);
                return 0;
            } else {
                soundmap = new_map;
            }
        } else {
            Sprintf(text, "cannot read %.243s", filespec);
            raw_print(text);
            return 0;
        }
    } else {
        raw_print("syntax error in SOUND");
        return 0;
    }

    return 1;
}

void play_sound_for_message(msg) const char *msg;
{
    audio_mapping *cursor = soundmap;

    while (cursor) {
        if (regex_match(msg, cursor->regex)) {
            play_usersound(cursor->filename, cursor->volume);
        }
        cursor = cursor->next;
    }
}

#endif /* USER_SOUNDS */

/*sounds.c*/
