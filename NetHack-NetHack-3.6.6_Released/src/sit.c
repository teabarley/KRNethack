/* NetHack 3.6	sit.c	$NHDT-Date: 1559670609 2019/06/04 17:50:09 $  $NHDT-Branch: NetHack-3.6 $:$NHDT-Revision: 1.61 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2012. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "artifact.h"


/* take away the hero's money */
void
take_gold()
{
    struct obj *otmp, *nobj;
    int lost_money = 0;

    for (otmp = invent; otmp; otmp = nobj) {
        nobj = otmp->nobj;
        if (otmp->oclass == COIN_CLASS) {
            lost_money = 1;
            remove_worn_item(otmp, FALSE);
            delobj(otmp);
        }
    }
    if (!lost_money) {
        /*KR You_feel("a strange sensation."); */
        You("기묘한 감각을 느꼈다.");
    } else {
        /*KR You("notice you have no money!"); */
        You("돈이 하나도 없다는 것을 깨달았다!");
        context.botl = 1;
    }
}

/* #sit command */
int
dosit()
{
    /*KR static const char sit_message[] = "sit on the %s."; */
    static const char sit_message[] = "%s에 앉았다.";
    register struct trap *trap = t_at(u.ux, u.uy);
    register int typ = levl[u.ux][u.uy].typ;

    if (u.usteed) {
#if 0 /*KR: 원본*/
        You("are already sitting on %s.", mon_nam(u.usteed));
#else /*KR: KRNethack 맞춤 번역 */
        You("이미 %s 앉아 있다.", append_josa(mon_nam(u.usteed), "에"));
#endif
        return 0;
    }
    if (u.uundetected && is_hider(youmonst.data) && u.umonnum != PM_TRAPPER)
        u.uundetected = 0; /* no longer on the ceiling */

    if (!can_reach_floor(FALSE)) {
        if (u.uswallow)
            /*KR There("are no seats in here!"); */
            pline("여기에는 앉을 자리가 없다!");
        else if (Levitation)
            /*KR You("tumble in place."); */
            You("그 자리에서 공중제비를 돌았다.");
        else
            /*KR You("are sitting on air."); */
            You("공중에 앉았다.");
        return 0;
    } else if (u.ustuck && !sticks(youmonst.data)) {
        /* holding monster is next to hero rather than beneath, but
           hero is in no condition to actually sit at has/her own spot */
        if (humanoid(u.ustuck->data))
#if 0 /*KR: 원본*/
            pline("%s won't offer %s lap.", Monnam(u.ustuck), mhis(u.ustuck));
#else /*KR: KRNethack 맞춤 번역 */
            pline("%s 당신에게 무릎을 내주지 않는다.",
                  append_josa(Monnam(u.ustuck), "은"));
#endif
        else
#if 0 /*KR: 원본*/
            pline("%s has no lap.", Monnam(u.ustuck));
#else /*KR: KRNethack 맞춤 번역 */
            pline("%s 무릎이 없다.", append_josa(Monnam(u.ustuck), "은"));
#endif
        return 0;
    } else if (is_pool(u.ux, u.uy) && !Underwater) { /* water walking */
        goto in_water;
    }

    if (OBJ_AT(u.ux, u.uy)
        /* ensure we're not standing on the precipice */
        && !(uteetering_at_seen_pit(trap) || uescaped_shaft(trap))) {
        register struct obj *obj;

        obj = level.objects[u.ux][u.uy];
        if (youmonst.data->mlet == S_DRAGON && obj->oclass == COIN_CLASS) {
#if 0 /*KR: 원본*/
            You("coil up around your %shoard.",
                (obj->quan + money_cnt(invent) < u.ulevel * 1000) ? "meager "
                                                                  : "");
#else /*KR: KRNethack 맞춤 번역 */
            You("당신의 %s보물 위에 똬리를 틀었다.",
                (obj->quan + money_cnt(invent) < u.ulevel * 1000)
                    ? "보잘것없는 "
                    : "");
#endif
        } else {
#if 0 /*KR: 원본*/
            You("sit on %s.", the(xname(obj)));
#else /*KR: KRNethack 맞춤 번역 */
            You("%s 앉았다.", append_josa(the(xname(obj)), "에"));
#endif
            if (!(Is_box(obj) || objects[obj->otyp].oc_material == CLOTH))
                /*KR pline("It's not very comfortable..."); */
                pline("별로 편안하지 않다...");
        }
    } else if (trap != 0 || (u.utrap && (u.utraptype >= TT_LAVA))) {
        if (u.utrap) {
            exercise(A_WIS, FALSE); /* you're getting stuck longer */
            if (u.utraptype == TT_BEARTRAP) {
#if 0 /*KR: 원본*/
                You_cant("sit down with your %s in the bear trap.",
                         body_part(FOOT));
#else /*KR: KRNethack 맞춤 번역 */
                pline("%s 곰 덫에 갇힌 채로는 앉을 수 없다.",
                      append_josa(body_part(FOOT), "이"));
#endif
                u.utrap++;
            } else if (u.utraptype == TT_PIT) {
                if (trap && trap->ttyp == SPIKED_PIT) {
                    /*KR You("sit down on a spike.  Ouch!"); */
                    You("스파이크 위에 앉았다. 아얏!");
                    losehp(Half_physical_damage ? rn2(2) : 1,
                           /*KR "sitting on an iron spike", KILLED_BY); */
                           "철 스파이크 위에 앉아서", KILLED_BY);
                    exercise(A_STR, FALSE);
                } else
                    /*KR You("sit down in the pit."); */
                    You("구덩이 안에 앉았다.");
                u.utrap += rn2(5);
            } else if (u.utraptype == TT_WEB) {
                /*KR You("sit in the spider web and get entangled further!");
                 */
                You("거미줄에 앉아 더욱 깊이 얽매였다!");
                u.utrap += rn1(10, 5);
            } else if (u.utraptype == TT_LAVA) {
                /* Must have fire resistance or they'd be dead already */
#if 0 /*KR: 원본*/
                You("sit in the %s!", hliquid("lava"));
#else /*KR: KRNethack 맞춤 번역 */
                You("%s 앉았다!", append_josa(hliquid("용암"), "에"));
#endif
                if (Slimed)
                    burn_away_slime();
                u.utrap += rnd(4);
                /*KR losehp(d(2, 10), "sitting in lava", */
                losehp(d(2, 10), "용암 속에 앉아서",
                       KILLED_BY); /* lava damage */
            } else if (u.utraptype == TT_INFLOOR
                       || u.utraptype == TT_BURIEDBALL) {
                /*KR You_cant("maneuver to sit!"); */
                You_cant("앉으려고 몸을 움직일 수가 없다!");
                u.utrap++;
            }
        } else {
            /*KR You("sit down."); */
            You("앉았다.");
            dotrap(trap, VIASITTING);
        }
    } else if ((Underwater || Is_waterlevel(&u.uz))
               && !eggs_in_water(youmonst.data)) {
        if (Is_waterlevel(&u.uz))
            /*KR There("are no cushions floating nearby."); */
            pline("근처에 떠다니는 쿠션 같은 건 없다.");
        else
            /*KR You("sit down on the muddy bottom."); */
            You("진흙 바닥에 앉았다.");
    } else if (is_pool(u.ux, u.uy) && !eggs_in_water(youmonst.data)) {
    in_water:
#if 0 /*KR: 원본*/
        You("sit in the %s.", hliquid("water"));
#else /*KR: KRNethack 맞춤 번역 */
        You("%s 앉았다.", append_josa(hliquid("물"), "에"));
#endif
        if (!rn2(10) && uarm)
            /*KR (void) water_damage(uarm, "armor", TRUE); */
            (void) water_damage(uarm, "갑옷", TRUE);
        if (!rn2(10) && uarmf && uarmf->otyp != WATER_WALKING_BOOTS)
            /*KR (void) water_damage(uarm, "armor", TRUE); */
            (void) water_damage(uarm, "갑옷", TRUE);
    } else if (IS_SINK(typ)) {
        You(sit_message, defsyms[S_sink].explanation);
        /*KR Your("%s gets wet.", humanoid(youmonst.data) ? "rump" :
         * "underside"); */
        Your("%s 젖었다.",
             append_josa(humanoid(youmonst.data) ? "엉덩이" : "아랫부분",
                         "이"));
    } else if (IS_ALTAR(typ)) {
        You(sit_message, defsyms[S_altar].explanation);
        altar_wrath(u.ux, u.uy);
    } else if (IS_GRAVE(typ)) {
        You(sit_message, defsyms[S_grave].explanation);
    } else if (typ == STAIRS) {
        /*KR You(sit_message, "stairs"); */
        You(sit_message, "계단");
    } else if (typ == LADDER) {
        /*KR You(sit_message, "ladder"); */
        You(sit_message, "사다리");
    } else if (is_lava(u.ux, u.uy)) {
        /* must be WWalking */
        /*KR You(sit_message, hliquid("lava")); */
        You(sit_message, hliquid("용암"));
        burn_away_slime();
        if (likes_lava(youmonst.data)) {
            /*KR pline_The("%s feels warm.", hliquid("lava")); */
            pline("%s 따뜻하게 느껴진다.",
                  append_josa(hliquid("용암"), "이"));
            return 1;
        }
        /*KR pline_The("%s burns you!", hliquid("lava")); */
        pline("%s 당신을 태운다!", append_josa(hliquid("용암"), "이"));
        losehp(d((Fire_resistance ? 2 : 10), 10), /* lava damage */
               /*KR "sitting on lava", KILLED_BY); */
               "용암 위에 앉아서", KILLED_BY);
    } else if (is_ice(u.ux, u.uy)) {
        You(sit_message, defsyms[S_ice].explanation);
        if (!Cold_resistance)
            /*KR pline_The("ice feels cold."); */
            pline("얼음이 차갑게 느껴진다.");
    } else if (typ == DRAWBRIDGE_DOWN) {
        /*KR You(sit_message, "drawbridge"); */
        You(sit_message, "도개교");
    } else if (IS_THRONE(typ)) {
        You(sit_message, defsyms[S_throne].explanation);
        if (rnd(6) > 4) {
            switch (rnd(13)) {
            case 1:
                (void) adjattrib(rn2(A_MAX), -rn1(4, 3), FALSE);
                /*KR losehp(rnd(10), "cursed throne", KILLED_BY_AN); */
                losehp(rnd(10), "저주받은 옥좌에 앉아서", KILLED_BY);
                break;
            case 2:
                (void) adjattrib(rn2(A_MAX), 1, FALSE);
                break;
            case 3:
#if 0 /*KR: 원본*/
                pline("A%s electric shock shoots through your body!",
                      (Shock_resistance) ? "n" : " massive");
#else /*KR: KRNethack 맞춤 번역 */
                pline("%s전기 충격이 몸을 관통했다!",
                      (Shock_resistance) ? "" : "강력한 ");
#endif
                /*KR losehp(Shock_resistance ? rnd(6) : rnd(30), "electric
                   chair", KILLED_BY_AN); */
                losehp(Shock_resistance ? rnd(6) : rnd(30),
                       "전기 의자에 앉아서", KILLED_BY);
                exercise(A_CON, FALSE);
                break;
            case 4:
                /*KR You_feel("much, much better!"); */
                You("훨씬, 훨씬 더 상태가 좋아진 것을 느낀다!");
                if (Upolyd) {
                    if (u.mh >= (u.mhmax - 5))
                        u.mhmax += 4;
                    u.mh = u.mhmax;
                }
                if (u.uhp >= (u.uhpmax - 5))
                    u.uhpmax += 4;
                u.uhp = u.uhpmax;
                u.ucreamed = 0;
                make_blinded(0L, TRUE);
                make_sick(0L, (char *) 0, FALSE, SICK_ALL);
                heal_legs(0);
                context.botl = 1;
                break;
            case 5:
                take_gold();
                break;
            case 6:
                if (u.uluck + rn2(5) < 0) {
                    /*KR You_feel("your luck is changing."); */
                    You("당신의 운이 변하고 있는 것을 느낀다.");
                    change_luck(1);
                } else
                    makewish();
                break;
            case 7: {
                int cnt = rnd(10);

                /* Magical voice not affected by deafness */
                /*KR pline("A voice echoes:"); */
                pline("목소리가 메아리친다:");
#if 0 /*KR: 원본*/
                verbalize("Thy audience hath been summoned, %s!",
                          flags.female ? "Dame" : "Sire");
#else /*KR: KRNethack 맞춤 번역 */
                verbalize("그대의 청중이 소환되었도다, %s!",
                          flags.female ? "부인이여" : "전하여");
#endif
                while (cnt--)
                    (void) makemon(courtmon(), u.ux, u.uy, NO_MM_FLAGS);
                break;
            }
            case 8:
                /* Magical voice not affected by deafness */
                /*KR pline("A voice echoes:"); */
                pline("목소리가 메아리친다:");
#if 0 /*KR: 원본*/
                verbalize("By thine Imperious order, %s...",
                          flags.female ? "Dame" : "Sire");
#else /*KR: KRNethack 맞춤 번역 */
                verbalize("그대의 오만한 명령에 따라, %s...",
                          flags.female ? "부인이여" : "전하여");
#endif
                do_genocide(5); /* REALLY|ONTHRONE, see do_genocide() */
                break;
            case 9:
                /* Magical voice not affected by deafness */
                /*KR pline("A voice echoes:"); */
                pline("목소리가 메아리친다:");
                /*KR verbalize(
                 "A curse upon thee for sitting upon this most holy throne!");
               */
                verbalize(
                    "이 가장 신성한 옥좌에 앉은 그대에게 저주가 있을지어다!");
                if (Luck > 0) {
                    make_blinded(Blinded + rn1(100, 250), TRUE);
                    change_luck((Luck > 1) ? -rnd(2) : -1);
                } else
                    rndcurse();
                break;
            case 10:
                if (Luck < 0 || (HSee_invisible & INTRINSIC)) {
                    if (level.flags.nommap) {
                        /*KR pline("A terrible drone fills your head!"); */
                        pline(
                            "끔찍한 윙윙거리는 소리가 머릿속을 가득 채운다!");
                        make_confused((HConfusion & TIMEOUT) + (long) rnd(30),
                                      FALSE);
                    } else {
                        /*KR pline("An image forms in your mind."); */
                        pline("마음속에 어떤 형상이 떠오른다.");
                        do_mapping();
                    }
                } else {
                    /*KR Your("vision becomes clear."); */
                    Your("시야가 맑아졌다.");
                    HSee_invisible |= FROMOUTSIDE;
                    newsym(u.ux, u.uy);
                }
                break;
            case 11:
                if (Luck < 0) {
                    /*KR You_feel("threatened."); */
                    You("위협받는 느낌이 들었다.");
                    aggravate();
                } else {
                    /*KR You_feel("a wrenching sensation."); */
                    You("비틀리는 듯한 감각이 느껴졌다.");
                    tele(); /* teleport him */
                }
                break;
            case 12:
                /*KR You("are granted an insight!"); */
                You("통찰력을 얻었다!");
                if (invent) {
                    /* rn2(5) agrees w/seffects() */
                    identify_pack(rn2(5), FALSE);
                }
                break;
            case 13:
                /*KR Your("mind turns into a pretzel!"); */
                Your("정신이 프레첼처럼 꼬여버렸다!");
                make_confused((HConfusion & TIMEOUT) + (long) rn1(7, 16),
                              FALSE);
                break;
            default:
                impossible("throne effect");
                break;
            }
        } else {
            if (is_prince(youmonst.data))
                /*KR You_feel("very comfortable here."); */
                You("이곳이 매우 편안하게 느껴진다.");
            else
                /*KR You_feel("somehow out of place..."); */
                You("어쩐지 어울리지 않는 느낌이다...");
        }

        if (!rn2(3) && IS_THRONE(levl[u.ux][u.uy].typ)) {
            /* may have teleported */
            levl[u.ux][u.uy].typ = ROOM, levl[u.ux][u.uy].flags = 0;
            /*KR pline_The("throne vanishes in a puff of logic."); */
            pline("옥좌가 논리의 연기와 함께 펑 하고 사라졌다.");
            newsym(u.ux, u.uy);
        }
    } else if (lays_eggs(youmonst.data)) {
        struct obj *uegg;

        if (!flags.female) {
#if 0 /*KR: 원본*/
            pline("%s can't lay eggs!",
                  Hallucination
                      ? "You may think you are a platypus, but a male still"
                      : "Males");
#else /*KR: KRNethack 맞춤 번역 */
            pline("%s 알을 낳을 수 없다!",
                  Hallucination
                      ? "당신이 오리너구리라고 생각할지 몰라도, 수컷은 여전히"
                      : "수컷은");
#endif
            return 0;
        } else if (u.uhunger < (int) objects[EGG].oc_nutrition) {
            /*KR You("don't have enough energy to lay an egg."); */
            You("알을 낳을 만한 에너지가 없다.");
            return 0;
        } else if (eggs_in_water(youmonst.data)) {
            if (!(Underwater || Is_waterlevel(&u.uz))) {
                /*KR pline("A splash tetra you are not."); */
                pline("당신은 스플래시 테트라가 아니다.");
                return 0;
            }
            if (Upolyd
                && (youmonst.data == &mons[PM_GIANT_EEL]
                    || youmonst.data == &mons[PM_ELECTRIC_EEL])) {
                /*KR You("yearn for the Sargasso Sea."); */
                You("사르가소 해를 갈망한다.");
                return 0;
            }
        }
        uegg = mksobj(EGG, FALSE, FALSE);
        uegg->spe = 1;
        uegg->quan = 1L;
        uegg->owt = weight(uegg);
        /* this sets hatch timers if appropriate */
        set_corpsenm(uegg, egg_type_from_parent(u.umonnum, FALSE));
        uegg->known = uegg->dknown = 1;
#if 0 /*KR: 원본*/
        You("%s an egg.", eggs_in_water(youmonst.data) ? "spawn" : "lay");
#else /*KR: KRNethack 맞춤 번역 */
        You("알을 낳았다.");
#endif
        dropy(uegg);
        stackobj(uegg);
        morehungry((int) objects[EGG].oc_nutrition);
    } else {
        /*KR pline("Having fun sitting on the %s?", surface(u.ux, u.uy)); */
        pline("%s 앉아 있으니 재미있는가?",
              append_josa(surface(u.ux, u.uy), "에"));
    }
    return 1;
}

/* curse a few inventory items at random! */
void
rndcurse()
{
    int nobj = 0;
    int cnt, onum;
    struct obj *otmp;
    /*KR static const char mal_aura[] = "feel a malignant aura surround %s.";
     */
    static const char mal_aura[] = "사악한 기운이 %s 감싸는 것을 느꼈다.";

    if (uwep && (uwep->oartifact == ART_MAGICBANE) && rn2(20)) {
        /*KR You(mal_aura, "the magic-absorbing blade"); */
        You(mal_aura, "마력 흡수검을");
        return;
    }

    if (Antimagic) {
        shieldeff(u.ux, u.uy);
        /*KR You(mal_aura, "you"); */
        You(mal_aura, "당신을");
    }

    for (otmp = invent; otmp; otmp = otmp->nobj) {
        /* gold isn't subject to being cursed or blessed */
        if (otmp->oclass == COIN_CLASS)
            continue;
        nobj++;
    }
    if (nobj) {
        for (cnt = rnd(6 / ((!!Antimagic) + (!!Half_spell_damage) + 1));
             cnt > 0; cnt--) {
            onum = rnd(nobj);
            for (otmp = invent; otmp; otmp = otmp->nobj) {
                /* as above */
                if (otmp->oclass == COIN_CLASS)
                    continue;
                if (--onum == 0)
                    break; /* found the target */
            }
            /* the !otmp case should never happen; picking an already
               cursed item happens--avoid "resists" message in that case */
            if (!otmp || otmp->cursed)
                continue; /* next target */

            if (otmp->oartifact && spec_ability(otmp, SPFX_INTEL)
                && rn2(10) < 8) {
#if 0 /*KR: 원본*/
                pline("%s!", Tobjnam(otmp, "resist"));
#else /*KR: KRNethack 맞춤 번역 */
                pline("%s 저항했다!", append_josa(xname(otmp), "은"));
#endif
                continue;
            }

            if (otmp->blessed)
                unbless(otmp);
            else
                curse(otmp);
        }
        update_inventory();
    }

    /* treat steed's saddle as extended part of hero's inventory */
    if (u.usteed && !rn2(4) && (otmp = which_armor(u.usteed, W_SADDLE)) != 0
        && !otmp->cursed) { /* skip if already cursed */
        if (otmp->blessed)
            unbless(otmp);
        else
            curse(otmp);
        if (!Blind) {
#if 0 /*KR: 원본*/
            pline("%s %s.", Yobjnam2(otmp, "glow"),
                  hcolor(otmp->cursed ? NH_BLACK : (const char *) "brown"));
#else /*KR: KRNethack 맞춤 번역 */
            pline("%s %s 빛난다.", append_josa(xname(otmp), "이"),
                  hcolor(otmp->cursed ? NH_BLACK
                                          : (const char *) "갈색으로"));
#endif
            otmp->bknown = 1; /* ok to bypass set_bknown() here */
        }
    }
}

/* remove a random INTRINSIC ability */
void
attrcurse()
{
    switch (rnd(11)) {
    case 1:
        if (HFire_resistance & INTRINSIC) {
            HFire_resistance &= ~INTRINSIC;
            /*KR You_feel("warmer."); */
            You("따뜻해진 것을 느낀다.");
            break;
        }
        /*FALLTHRU*/
    case 2:
        if (HTeleportation & INTRINSIC) {
            HTeleportation &= ~INTRINSIC;
            /*KR You_feel("less jumpy."); */
            You("덜 들썩거리는 기분을 느낀다.");
            break;
        }
        /*FALLTHRU*/
    case 3:
        if (HPoison_resistance & INTRINSIC) {
            HPoison_resistance &= ~INTRINSIC;
            /*KR You_feel("a little sick!"); */
            You("약간 역겨운 기분을 느낀다!");
            break;
        }
        /*FALLTHRU*/
    case 4:
        if (HTelepat & INTRINSIC) {
            HTelepat &= ~INTRINSIC;
            if (Blind && !Blind_telepat)
                see_monsters(); /* Can't sense mons anymore! */
            /*KR Your("senses fail!"); */
            Your("감각이 마비되었다!");
            break;
        }
        /*FALLTHRU*/
    case 5:
        if (HCold_resistance & INTRINSIC) {
            HCold_resistance &= ~INTRINSIC;
            /*KR You_feel("cooler."); */
            You("시원해진 것을 느낀다.");
            break;
        }
        /*FALLTHRU*/
    case 6:
        if (HInvis & INTRINSIC) {
            HInvis &= ~INTRINSIC;
            /*KR You_feel("paranoid."); */
            You("피해망상적인 기분을 느낀다.");
            break;
        }
        /*FALLTHRU*/
    case 7:
        if (HSee_invisible & INTRINSIC) {
            HSee_invisible &= ~INTRINSIC;
#if 0 /*KR: 원본*/
            You("%s!", Hallucination ? "tawt you taw a puttie tat"
                                     : "thought you saw something");
#else /*KR: KRNethack 맞춤 번역 */
            You("%s!", Hallucination
                           ? "기여운 고냥이를 봉 거 가따고 생가케따"
                           : "무언가 본 것 같다고 생각했다");
#endif
            break;
        }
        /*FALLTHRU*/
    case 8:
        if (HFast & INTRINSIC) {
            HFast &= ~INTRINSIC;
            /*KR You_feel("slower."); */
            You("느려진 것을 느낀다.");
            break;
        }
        /*FALLTHRU*/
    case 9:
        if (HStealth & INTRINSIC) {
            HStealth &= ~INTRINSIC;
            /*KR You_feel("clumsy."); */
            You("어설퍼진 것을 느낀다.");
            break;
        }
        /*FALLTHRU*/
    case 10:
        /* intrinsic protection is just disabled, not set back to 0 */
        if (HProtection & INTRINSIC) {
            HProtection &= ~INTRINSIC;
            /*KR You_feel("vulnerable."); */
            You("취약해진 것을 느낀다.");
            break;
        }
        /*FALLTHRU*/
    case 11:
        if (HAggravate_monster & INTRINSIC) {
            HAggravate_monster &= ~INTRINSIC;
            /*KR You_feel("less attractive."); */
            You("매력이 떨어진 것을 느낀다.");
            break;
        }
        /*FALLTHRU*/
    default:
        break;
    }
}

/*sit.c*/
