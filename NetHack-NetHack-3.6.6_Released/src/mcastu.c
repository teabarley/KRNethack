/* NetHack 3.6	mcastu.c	$NHDT-Date: 1567418129 2019/09/02 09:55:29 $  $NHDT-Branch: NetHack-3.6 $:$NHDT-Revision: 1.55 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2011. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

/* monster mage spells */
enum mcast_mage_spells {
    MGC_PSI_BOLT = 0,
    MGC_CURE_SELF,
    MGC_HASTE_SELF,
    MGC_STUN_YOU,
    MGC_DISAPPEAR,
    MGC_WEAKEN_YOU,
    MGC_DESTRY_ARMR,
    MGC_CURSE_ITEMS,
    MGC_AGGRAVATION,
    MGC_SUMMON_MONS,
    MGC_CLONE_WIZ,
    MGC_DEATH_TOUCH
};

/* monster cleric spells */
enum mcast_cleric_spells {
    CLC_OPEN_WOUNDS = 0,
    CLC_CURE_SELF,
    CLC_CONFUSE_YOU,
    CLC_PARALYZE,
    CLC_BLIND_YOU,
    CLC_INSECTS,
    CLC_CURSE_ITEMS,
    CLC_LIGHTNING,
    CLC_FIRE_PILLAR,
    CLC_GEYSER
};

STATIC_DCL void FDECL(cursetxt, (struct monst *, BOOLEAN_P));
STATIC_DCL int FDECL(choose_magic_spell, (int) );
STATIC_DCL int FDECL(choose_clerical_spell, (int) );
STATIC_DCL int FDECL(m_cure_self, (struct monst *, int) );
STATIC_DCL void FDECL(cast_wizard_spell, (struct monst *, int, int) );
STATIC_DCL void FDECL(cast_cleric_spell, (struct monst *, int, int) );
STATIC_DCL boolean FDECL(is_undirected_spell, (unsigned int, int) );
STATIC_DCL boolean FDECL(spell_would_be_useless,
                         (struct monst *, unsigned int, int) );

extern const char *const flash_types[]; /* from zap.c */

/* feedback when frustrated monster couldn't cast a spell */
STATIC_OVL
void cursetxt(mtmp, undirected) struct monst *mtmp;
boolean undirected;
{
    if (canseemon(mtmp) && couldsee(mtmp->mx, mtmp->my)) {
        const char *point_msg; /* spellcasting monsters are impolite */

        if (undirected)
            /*KR point_msg = "all around, then curses"; */
            point_msg = "사방을 가리키고는 저주를 퍼부었다";
        else if ((Invis && !perceives(mtmp->data)
                  && (mtmp->mux != u.ux || mtmp->muy != u.uy))
                 || is_obj_mappear(&youmonst, STRANGE_OBJECT)
                 || u.uundetected)
            /*KR point_msg = "and curses in your general direction"; */
            point_msg = "당신이 있는 쪽을 가리키고는 저주를 퍼부었다";
        else if (Displaced && (mtmp->mux != u.ux || mtmp->muy != u.uy))
            /*KR point_msg = "and curses at your displaced image"; */
            point_msg = "당신의 환영을 가리키고는 저주를 퍼부었다";
        else
            /*KR point_msg = "at you, then curses"; */
            point_msg = "당신을 가리키고는 저주를 퍼부었다";

        /*KR pline("%s points %s.", Monnam(mtmp), point_msg); */
        pline("%s %s.", append_josa(Monnam(mtmp), "은"), point_msg);
    } else if ((!(moves % 4) || !rn2(4))) {
        if (!Deaf)
            /*KR Norep("You hear a mumbled curse.");   */ /* Deaf-aware */
            Norep("누군가 웅얼거리며 저주하는 소리가 들린다.");
    }
}

/* convert a level based random selection into a specific mage spell;
   inappropriate choices will be screened out by spell_would_be_useless() */
STATIC_OVL int
choose_magic_spell(spellval)
int spellval;
{
    /* for 3.4.3 and earlier, val greater than 22 selected the default spell
     */
    while (spellval > 24 && rn2(25))
        spellval = rn2(spellval);

    switch (spellval) {
    case 24:
    case 23:
        if (Antimagic || Hallucination)
            return MGC_PSI_BOLT;
        /*FALLTHRU*/
    case 22:
    case 21:
    case 20:
        return MGC_DEATH_TOUCH;
    case 19:
    case 18:
        return MGC_CLONE_WIZ;
    case 17:
    case 16:
    case 15:
        return MGC_SUMMON_MONS;
    case 14:
    case 13:
        return MGC_AGGRAVATION;
    case 12:
    case 11:
    case 10:
        return MGC_CURSE_ITEMS;
    case 9:
    case 8:
        return MGC_DESTRY_ARMR;
    case 7:
    case 6:
        return MGC_WEAKEN_YOU;
    case 5:
    case 4:
        return MGC_DISAPPEAR;
    case 3:
        return MGC_STUN_YOU;
    case 2:
        return MGC_HASTE_SELF;
    case 1:
        return MGC_CURE_SELF;
    case 0:
    default:
        return MGC_PSI_BOLT;
    }
}

/* convert a level based random selection into a specific cleric spell */
STATIC_OVL int
choose_clerical_spell(spellnum)
int spellnum;
{
    /* for 3.4.3 and earlier, num greater than 13 selected the default spell
     */
    while (spellnum > 15 && rn2(16))
        spellnum = rn2(spellnum);

    switch (spellnum) {
    case 15:
    case 14:
        if (rn2(3))
            return CLC_OPEN_WOUNDS;
        /*FALLTHRU*/
    case 13:
        return CLC_GEYSER;
    case 12:
        return CLC_FIRE_PILLAR;
    case 11:
        return CLC_LIGHTNING;
    case 10:
    case 9:
        return CLC_CURSE_ITEMS;
    case 8:
        return CLC_INSECTS;
    case 7:
    case 6:
        return CLC_BLIND_YOU;
    case 5:
    case 4:
        return CLC_PARALYZE;
    case 3:
    case 2:
        return CLC_CONFUSE_YOU;
    case 1:
        return CLC_CURE_SELF;
    case 0:
    default:
        return CLC_OPEN_WOUNDS;
    }
}

/* return values:
 * 1: successful spell
 * 0: unsuccessful spell
 */
int
castmu(mtmp, mattk, thinks_it_foundyou, foundyou)
register struct monst *mtmp;
register struct attack *mattk;
boolean thinks_it_foundyou;
boolean foundyou;
{
    int dmg, ml = mtmp->m_lev;
    int ret;
    int spellnum = 0;

    /* Three cases:
     * -- monster is attacking you.  Search for a useful spell.
     * -- monster thinks it's attacking you.  Search for a useful spell,
     * without checking for undirected.  If the spell found is directed,
     * it fails with cursetxt() and loss of mspec_used.
     * -- monster isn't trying to attack.  Select a spell once.  Don't keep
     * searching; if that spell is not useful (or if it's directed),
     * return and do something else.
     * Since most spells are directed, this means that a monster that isn't
     * attacking casts spells only a small portion of the time that an
     * attacking monster does.
     */
    if ((mattk->adtyp == AD_SPEL || mattk->adtyp == AD_CLRC) && ml) {
        int cnt = 40;

        do {
            spellnum = rn2(ml);
            if (mattk->adtyp == AD_SPEL)
                spellnum = choose_magic_spell(spellnum);
            else
                spellnum = choose_clerical_spell(spellnum);
            /* not trying to attack?  don't allow directed spells */
            if (!thinks_it_foundyou) {
                if (!is_undirected_spell(mattk->adtyp, spellnum)
                    || spell_would_be_useless(mtmp, mattk->adtyp, spellnum)) {
                    if (foundyou)
                        impossible("spellcasting monster found you and "
                                   "doesn't know it?");
                    return 0;
                }
                break;
            }
        } while (--cnt > 0
                 && spell_would_be_useless(mtmp, mattk->adtyp, spellnum));
        if (cnt == 0)
            return 0;
    }

    /* monster unable to cast spells? */
    if (mtmp->mcan || mtmp->mspec_used || !ml) {
        cursetxt(mtmp, is_undirected_spell(mattk->adtyp, spellnum));
        return (0);
    }

    if (mattk->adtyp == AD_SPEL || mattk->adtyp == AD_CLRC) {
        mtmp->mspec_used = 10 - mtmp->m_lev;
        if (mtmp->mspec_used < 2)
            mtmp->mspec_used = 2;
    }

    /* monster can cast spells, but is casting a directed spell at the
       wrong place?  If so, give a message, and return.  Do this *after*
       penalizing mspec_used. */
    if (!foundyou && thinks_it_foundyou
        && !is_undirected_spell(mattk->adtyp, spellnum)) {
#if 0 /*KR: 원본*/
        pline("%s casts a spell at %s!",
              canseemon(mtmp) ? Monnam(mtmp) : "Something",
              levl[mtmp->mux][mtmp->muy].typ == WATER ? "empty water"
                                                      : "thin air");
#else /*KR: KRNethack 맞춤 번역 */
        pline("%s %s에 대고 주문을 시전했다!",
              canseemon(mtmp) ? append_josa(Monnam(mtmp), "은")
                              : "무언가가",
              levl[mtmp->mux][mtmp->muy].typ == WATER ? "텅 빈 물속"
                                                      : "허공");
#endif
        return (0);
    }

    nomul(0);
    if (rn2(ml * 10) < (mtmp->mconf ? 100 : 20)) { /* fumbled attack */
        if (canseemon(mtmp) && !Deaf)
            /*KR pline_The("air crackles around %s.", mon_nam(mtmp)); */
            pline("%s 주변의 공기가 찌릿찌릿 소리를 낸다.",
                  append_josa(mon_nam(mtmp), "의"));
        return (0);
    }
    if (canspotmon(mtmp) || !is_undirected_spell(mattk->adtyp, spellnum)) {
#if 0 /*KR: 원본*/
        pline("%s casts a spell%s!",
              canspotmon(mtmp) ? Monnam(mtmp) : "Something",
              is_undirected_spell(mattk->adtyp, spellnum)
                  ? ""
                  : (Invis && !perceives(mtmp->data)
                     && (mtmp->mux != u.ux || mtmp->muy != u.uy))
                        ? " at a spot near you"
                        : (Displaced
                           && (mtmp->mux != u.ux || mtmp->muy != u.uy))
                              ? " at your displaced image"
                              : " at you");
#else /*KR: KRNethack 맞춤 번역 */
        const char *who = (canspotmon(mtmp) ? Monnam(mtmp) : "무언가");
        if (is_undirected_spell(mattk->adtyp, spellnum)) {
            pline("%s 주문을 시전했다!", append_josa(who, "은"));
        } else {
            pline("%s %s 향해 주문을 시전했다!", append_josa(who, "은"),
                  (Invis && !perceives(mtmp->data)
                   && (mtmp->mux != u.ux || mtmp->muy != u.uy))
                      ? "당신 근처의 한 지점을"
                  : (Displaced && (mtmp->mux != u.ux || mtmp->muy != u.uy))
                      ? "당신의 환영을"
                      : "당신을");
        }
#endif
    }

    /*
     * As these are spells, the damage is related to the level
     * of the monster casting the spell.
     */
    if (!foundyou) {
        dmg = 0;
        if (mattk->adtyp != AD_SPEL && mattk->adtyp != AD_CLRC) {
            impossible("%s casting non-hand-to-hand version of hand-to-hand "
                       "spell %d?",
                       Monnam(mtmp), mattk->adtyp);
            return (0);
        }
    } else if (mattk->damd)
        dmg = d((int) ((ml / 2) + mattk->damn), (int) mattk->damd);
    else
        dmg = d((int) ((ml / 2) + 1), 6);
    if (Half_spell_damage)
        dmg = (dmg + 1) / 2;

    ret = 1;

    switch (mattk->adtyp) {
    case AD_FIRE:
        /*KR pline("You're enveloped in flames."); */
        pline("당신은 화염에 휩싸였다.");
        if (Fire_resistance) {
            shieldeff(u.ux, u.uy);
            /*KR pline("But you resist the effects."); */
            pline("하지만 당신은 그 효과에 저항했다.");
            dmg = 0;
        }
        burn_away_slime();
        break;
    case AD_COLD:
        /*KR pline("You're covered in frost."); */
        pline("당신은 서리로 뒤덮였다.");
        if (Cold_resistance) {
            shieldeff(u.ux, u.uy);
            /*KR pline("But you resist the effects."); */
            pline("하지만 당신은 그 효과에 저항했다.");
            dmg = 0;
        }
        break;
    case AD_MAGM:
        /*KR You("are hit by a shower of missiles!"); */
        You("마법의 화살 세례를 맞았다!");
        if (Antimagic) {
            shieldeff(u.ux, u.uy);
            /*KR pline_The("missiles bounce off!"); */
            pline("화살들이 튕겨 나갔다!");
            dmg = 0;
        } else
            dmg = d((int) mtmp->m_lev / 2 + 1, 6);
        break;
    case AD_SPEL: /* wizard spell */
    case AD_CLRC: /* clerical spell */
    {
        if (mattk->adtyp == AD_SPEL)
            cast_wizard_spell(mtmp, dmg, spellnum);
        else
            cast_cleric_spell(mtmp, dmg, spellnum);
        dmg = 0; /* done by the spell casting functions */
        break;
    }
    }
    if (dmg)
        mdamageu(mtmp, dmg);
    return (ret);
}

STATIC_OVL int
m_cure_self(mtmp, dmg)
struct monst *mtmp;
int dmg;
{
    if (mtmp->mhp < mtmp->mhpmax) {
        if (canseemon(mtmp))
            /*KR pline("%s looks better.", Monnam(mtmp)); */
            pline("%s 상태가 좋아진 것 같다.",
                  append_josa(Monnam(mtmp), "의"));
        /* note: player healing does 6d4; this used to do 1d8 */
        if ((mtmp->mhp += d(3, 6)) > mtmp->mhpmax)
            mtmp->mhp = mtmp->mhpmax;
        dmg = 0;
    }
    return dmg;
}

/* monster wizard and cleric spellcasting functions */
/*
   If dmg is zero, then the monster is not casting at you.
   If the monster is intentionally not casting at you, we have previously
   called spell_would_be_useless() and spellnum should always be a valid
   undirected spell.
   If you modify either of these, be sure to change is_undirected_spell()
   and spell_would_be_useless().
 */
STATIC_OVL
void cast_wizard_spell(mtmp, dmg, spellnum) struct monst *mtmp;
int dmg;
int spellnum;
{
    if (dmg == 0 && !is_undirected_spell(AD_SPEL, spellnum)) {
        impossible("cast directed wizard spell (%d) with dmg=0?", spellnum);
        return;
    }

    switch (spellnum) {
    case MGC_DEATH_TOUCH:
        /*KR pline("Oh no, %s's using the touch of death!", mhe(mtmp)); */
        pline("오 이런, %s 죽음의 손길을 사용하고 있다!",
              append_josa(mhe(mtmp), "가"));
        if (nonliving(youmonst.data) || is_demon(youmonst.data)) {
            /*KR You("seem no deader than before."); */
            You("이전보다 더 죽은 것 같지는 않다.");
        } else if (!Antimagic && rn2(mtmp->m_lev) > 12) {
            if (Hallucination) {
                /*KR You("have an out of body experience."); */
                You("유체이탈을 경험했다.");
            } else {
                killer.format = KILLED_BY_AN;
                /*KR Strcpy(killer.name, "touch of death"); */
                Strcpy(killer.name, "죽음의 손길");
                done(DIED);
            }
        } else {
            if (Antimagic)
                shieldeff(u.ux, u.uy);
            /*KR pline("Lucky for you, it didn't work!"); */
            pline("다행히도, 아무런 효과가 없었다!");
        }
        dmg = 0;
        break;
    case MGC_CLONE_WIZ:
        if (mtmp->iswiz && context.no_of_wizards == 1) {
            /*KR pline("Double Trouble..."); */
            pline("골칫거리가 두 배가 되었다...");
            clonewiz();
            dmg = 0;
        } else
            impossible("bad wizard cloning?");
        break;
    case MGC_SUMMON_MONS: {
        int count;

        count = nasty(mtmp); /* summon something nasty */
        if (mtmp->iswiz) {
            /*KR verbalize("Destroy the thief, my pet%s!", plur(count)); */
            verbalize("도둑놈을 없애라, 나의 %s!",
                      count > 1 ? "부하들아" : "부하야");
        } else {
#if 0 /*KR: 원본*/
            const char *mappear = (count == 1) ? "A monster appears"
                                               : "Monsters appear";

            /* messages not quite right if plural monsters created but
               only a single monster is seen */
            if (Invis && !perceives(mtmp->data)
                && (mtmp->mux != u.ux || mtmp->muy != u.uy))
                pline("%s around a spot near you!", mappear);
            else if (Displaced && (mtmp->mux != u.ux || mtmp->muy != u.uy))
                pline("%s around your displaced image!", mappear);
            else
                pline("%s from nowhere!", mappear);
#else /*KR: KRNethack 맞춤 번역 */
            const char *mappear = (count == 1) ? "괴물이" : "괴물들이";

            if (Invis && !perceives(mtmp->data)
                && (mtmp->mux != u.ux || mtmp->muy != u.uy))
                pline("%s 당신 근처에 나타났다!", mappear);
            else if (Displaced && (mtmp->mux != u.ux || mtmp->muy != u.uy))
                pline("%s 당신의 환영 근처에 나타났다!", mappear);
            else
                pline("%s 어디선가 불쑥 나타났다!", mappear);
#endif
        }
        dmg = 0;
        break;
    }
    case MGC_AGGRAVATION:
        /*KR You_feel("that monsters are aware of your presence."); */
        You("괴물들이 당신의 존재를 알아챈 것 같다.");
        aggravate();
        dmg = 0;
        break;
    case MGC_CURSE_ITEMS:
        /*KR You_feel("as if you need some help."); */
        You("어딘가 도움이 필요할 것 같은 기분이 든다.");
        rndcurse();
        dmg = 0;
        break;
    case MGC_DESTRY_ARMR:
        if (Antimagic) {
            shieldeff(u.ux, u.uy);
            /*KR pline("A field of force surrounds you!"); */
            pline("신비한 역장이 당신을 감싼다!");
        } else if (!destroy_arm(some_armor(&youmonst))) {
            /*KR Your("skin itches."); */
            Your("피부가 가렵다.");
        }
        dmg = 0;
        break;
    case MGC_WEAKEN_YOU: /* drain strength */
        if (Antimagic) {
            shieldeff(u.ux, u.uy);
            /*KR You_feel("momentarily weakened."); */
            You("잠시 허약해진 느낌이 들었다.");
        } else {
            /*KR You("suddenly feel weaker!"); */
            You("갑자기 힘이 빠지는 것을 느꼈다!");
            dmg = mtmp->m_lev - 6;
            if (Half_spell_damage)
                dmg = (dmg + 1) / 2;
            losestr(rnd(dmg));
            if (u.uhp < 1)
                done_in_by(mtmp, DIED);
        }
        dmg = 0;
        break;
    case MGC_DISAPPEAR: /* makes self invisible */
        if (!mtmp->minvis && !mtmp->invis_blkd) {
            if (canseemon(mtmp))
#if 0 /*KR: 원본*/
                pline("%s suddenly %s!", Monnam(mtmp),
                      !See_invisible ? "disappears" : "becomes transparent");
#else /*KR: KRNethack 맞춤 번역 */
                pline("%s 갑자기 %s!", append_josa(Monnam(mtmp), "가"),
                      !See_invisible ? "사라졌다" : "투명해졌다");
#endif
            mon_set_minvis(mtmp);
            if (cansee(mtmp->mx, mtmp->my) && !canspotmon(mtmp))
                map_invisible(mtmp->mx, mtmp->my);
            dmg = 0;
        } else
            impossible("no reason for monster to cast disappear spell?");
        break;
    case MGC_STUN_YOU:
        if (Antimagic || Free_action) {
            shieldeff(u.ux, u.uy);
            if (!Stunned)
                /*KR You_feel("momentarily disoriented."); */
                You("순간적으로 방향 감각을 잃었다.");
            make_stunned(1L, FALSE);
        } else {
            /*KR You(Stunned ? "struggle to keep your balance." : "reel...");
             */
            You(Stunned ? "균형을 잡으려고 발버둥 친다." : "비틀거린다...");
            dmg = d(ACURR(A_DEX) < 12 ? 6 : 4, 4);
            if (Half_spell_damage)
                dmg = (dmg + 1) / 2;
            make_stunned((HStun & TIMEOUT) + (long) dmg, FALSE);
        }
        dmg = 0;
        break;
    case MGC_HASTE_SELF:
        mon_adjust_speed(mtmp, 1, (struct obj *) 0);
        dmg = 0;
        break;
    case MGC_CURE_SELF:
        dmg = m_cure_self(mtmp, dmg);
        break;
    case MGC_PSI_BOLT:
        /* prior to 3.4.0 Antimagic was setting the damage to 1--this
           made the spell virtually harmless to players with magic res. */
        if (Antimagic) {
            shieldeff(u.ux, u.uy);
            dmg = (dmg + 1) / 2;
        }
        if (dmg <= 5)
            /*KR You("get a slight %sache.", body_part(HEAD)); */
            You("가벼운 %s통을 느낀다.", body_part(HEAD));
        else if (dmg <= 10)
            /*KR Your("brain is on fire!"); */
            Your("뇌가 불타는 것 같다!");
        else if (dmg <= 20)
            /*KR Your("%s suddenly aches painfully!", body_part(HEAD)); */
            Your("%s 갑자기 고통스럽게 아프다!",
                 append_josa(body_part(HEAD), "가"));
        else
            /*KR Your("%s suddenly aches very painfully!", body_part(HEAD));
             */
            Your("%s 갑자기 몹시 고통스럽게 아프다!",
                 append_josa(body_part(HEAD), "가"));
        break;
    default:
        impossible("mcastu: invalid magic spell (%d)", spellnum);
        dmg = 0;
        break;
    }

    if (dmg)
        mdamageu(mtmp, dmg);
}

STATIC_OVL
void cast_cleric_spell(mtmp, dmg, spellnum) struct monst *mtmp;
int dmg;
int spellnum;
{
    if (dmg == 0 && !is_undirected_spell(AD_CLRC, spellnum)) {
        impossible("cast directed cleric spell (%d) with dmg=0?", spellnum);
        return;
    }

    switch (spellnum) {
    case CLC_GEYSER:
        /* this is physical damage (force not heat),
         * not magical damage or fire damage
         */
        /*KR pline("A sudden geyser slams into you from nowhere!"); */
        pline("어디선가 갑자기 튀어나온 간헐천이 당신을 강타했다!");
        dmg = d(8, 6);
        if (Half_physical_damage)
            dmg = (dmg + 1) / 2;
        break;
    case CLC_FIRE_PILLAR:
        /*KR pline("A pillar of fire strikes all around you!"); */
        pline("당신 주변에 불기둥이 솟아올랐다!");
        if (Fire_resistance) {
            shieldeff(u.ux, u.uy);
            dmg = 0;
        } else
            dmg = d(8, 6);
        if (Half_spell_damage)
            dmg = (dmg + 1) / 2;
        burn_away_slime();
        (void) burnarmor(&youmonst);
        destroy_item(SCROLL_CLASS, AD_FIRE);
        destroy_item(POTION_CLASS, AD_FIRE);
        destroy_item(SPBOOK_CLASS, AD_FIRE);
        (void) burn_floor_objects(u.ux, u.uy, TRUE, FALSE);
        break;
    case CLC_LIGHTNING: {
        boolean reflects;

        /*KR pline("A bolt of lightning strikes down at you from above!"); */
        pline("위에서 번개가 당신을 향해 내리쳤다!");
        /*KR reflects = ureflects("It bounces off your %s%s.", ""); */
        reflects = ureflects("그것은 당신의 %s%s 튕겨 나갔다.", "에서");
        if (reflects || Shock_resistance) {
            shieldeff(u.ux, u.uy);
            dmg = 0;
            if (reflects)
                break;
        } else
            dmg = d(8, 6);
        if (Half_spell_damage)
            dmg = (dmg + 1) / 2;
        destroy_item(WAND_CLASS, AD_ELEC);
        destroy_item(RING_CLASS, AD_ELEC);
        (void) flashburn((long) rnd(100));
        break;
    }
    case CLC_CURSE_ITEMS:
        /*KR You_feel("as if you need some help."); */
        You("어딘가 도움이 필요할 것 같은 기분이 든다.");
        rndcurse();
        dmg = 0;
        break;
    case CLC_INSECTS: {
        /* Try for insects, and if there are none
           left, go for (sticks to) snakes.  -3. */
        struct permonst *pm = mkclass(S_ANT, 0);
        struct monst *mtmp2 = (struct monst *) 0;
        char let = (pm ? S_ANT : S_SNAKE);
        boolean success = FALSE, seecaster;
        int i, quan, oldseen, newseen;
        coord bypos;
        const char *fmt;

        oldseen = monster_census(TRUE);
        quan = (mtmp->m_lev < 2) ? 1 : rnd((int) mtmp->m_lev / 2);
        if (quan < 3)
            quan = 3;
        for (i = 0; i <= quan; i++) {
            if (!enexto(&bypos, mtmp->mux, mtmp->muy, mtmp->data))
                break;
            if ((pm = mkclass(let, 0)) != 0
                && (mtmp2 = makemon(pm, bypos.x, bypos.y, MM_ANGRY)) != 0) {
                success = TRUE;
                mtmp2->msleeping = mtmp2->mpeaceful = mtmp2->mtame = 0;
                set_malign(mtmp2);
            }
        }
        newseen = monster_census(TRUE);

        /* not canspotmon(), which includes unseen things sensed via warning
         */
        seecaster = canseemon(mtmp) || tp_sensemon(mtmp) || Detect_monsters;

        fmt = 0;
        if (!seecaster) {
            char *arg; /* [not const: upstart(N==1 ? an() : makeplural())] */
            /*KR const char *what = (let == S_SNAKE) ? "snake" : "insect"; */
            const char *what = (let == S_SNAKE) ? "뱀" : "벌레";

            if (newseen <= oldseen || Unaware) {
                /* unseen caster fails or summons unseen critters,
                   or unconscious hero ("You dream that you hear...") */
                /*KR You_hear("someone summoning %s.", makeplural(what)); */
                You("누군가 %s 소환하는 소리를 들었다.",
                         append_josa(what, "을"));
            } else {
                /* unseen caster summoned seen critter(s) */
#if 0 /*KR: 원본*/
                arg = (newseen == oldseen + 1) ? an(what) : makeplural(what);
                if (!Deaf)
                    You_hear("someone summoning something, and %s %s.", arg,
                             vtense(arg, "appear"));
                else
                    pline("%s %s.", upstart(arg), vtense(arg, "appear"));
#else /*KR: KRNethack 맞춤 번역 */
                arg = (char *) what;
                if (!Deaf)
                    You("누군가 무언가를 소환하는 소리를 들었고, %s 나타났다.",
                             append_josa(arg, "가"));
                else
                    pline("%s 나타났다.", append_josa(arg, "가"));
#endif
            }

            /* seen caster, possibly producing unseen--or just one--critters;
               hero is told what the caster is doing and doesn't necessarily
               observe complete accuracy of that caster's results (in other
               words, no need to fuss with visibility or singularization;
               player is told what's happening even if hero is unconscious) */
        } else if (!success)
            /*KR fmt = "%s casts at a clump of sticks, but nothing happens.";
             */
            fmt = "%s 나뭇가지 더미를 향해 주문을 외웠지만, 아무 일도 "
                  "일어나지 않았다.";
        else if (let == S_SNAKE)
            /*KR fmt = "%s transforms a clump of sticks into snakes!"; */
            fmt = "%s 나뭇가지 더미를 뱀들로 바꿔놓았다!";
        else if (Invis && !perceives(mtmp->data)
                 && (mtmp->mux != u.ux || mtmp->muy != u.uy))
            /*KR fmt = "%s summons insects around a spot near you!"; */
            fmt = "%s 당신 주변의 한 지점에 벌레들을 소환했다!";
        else if (Displaced && (mtmp->mux != u.ux || mtmp->muy != u.uy))
            /*KR fmt = "%s summons insects around your displaced image!"; */
            fmt = "%s 당신의 환영 주변에 벌레들을 소환했다!";
        else
            /*KR fmt = "%s summons insects!"; */
            fmt = "%s 벌레들을 소환했다!";
        if (fmt)
            pline(fmt, append_josa(Monnam(mtmp), "은"));

        dmg = 0;
        break;
    }
    case CLC_BLIND_YOU:
        /* note: resists_blnd() doesn't apply here */
        if (!Blinded) {
            int num_eyes = eyecount(youmonst.data);
#if 0 /*KR: 원본*/
            pline("Scales cover your %s!", (num_eyes == 1)
                                               ? body_part(EYE)
                                               : makeplural(body_part(EYE)));
#else /*KR: KRNethack 맞춤 번역 */
            pline("비늘이 당신의 %s 덮었다!",
                  append_josa(body_part(EYE), "을"));
#endif
            make_blinded(Half_spell_damage ? 100L : 200L, FALSE);
            if (!Blind)
                Your1(vision_clears);
            dmg = 0;
        } else
            impossible("no reason for monster to cast blindness spell?");
        break;
    case CLC_PARALYZE:
        if (Antimagic || Free_action) {
            shieldeff(u.ux, u.uy);
            if (multi >= 0)
                /*KR You("stiffen briefly."); */
                You("잠시 몸이 굳어졌다.");
            nomul(-1);
            /*KR multi_reason = "paralyzed by a monster"; */
            multi_reason = "괴물에 의해 마비되어";
        } else {
            if (multi >= 0)
                /*KR You("are frozen in place!"); */
                You("그 자리에 얼어붙었다!");
            dmg = 4 + (int) mtmp->m_lev;
            if (Half_spell_damage)
                dmg = (dmg + 1) / 2;
            nomul(-dmg);
            /*KR multi_reason = "paralyzed by a monster"; */
            multi_reason = "괴물에 의해 마비되어";
        }
        nomovemsg = 0;
        dmg = 0;
        break;
    case CLC_CONFUSE_YOU:
        if (Antimagic) {
            shieldeff(u.ux, u.uy);
            /*KR You_feel("momentarily dizzy."); */
            You("순간적으로 어지러움을 느꼈다.");
        } else {
            boolean oldprop = !!Confusion;

            dmg = (int) mtmp->m_lev;
            if (Half_spell_damage)
                dmg = (dmg + 1) / 2;
            make_confused(HConfusion + dmg, TRUE);
            if (Hallucination)
                /*KR You_feel("%s!", oldprop ? "trippier" : "trippy"); */
                You("%s몽롱하다!", oldprop ? "더욱 " : "");
            else
                /*KR You_feel("%sconfused!", oldprop ? "more " : ""); */
                You("%s혼란스럽다!", oldprop ? "더욱 " : "");
        }
        dmg = 0;
        break;
    case CLC_CURE_SELF:
        dmg = m_cure_self(mtmp, dmg);
        break;
    case CLC_OPEN_WOUNDS:
        if (Antimagic) {
            shieldeff(u.ux, u.uy);
            dmg = (dmg + 1) / 2;
        }
        if (dmg <= 5)
            /*KR Your("skin itches badly for a moment."); */
            Your("피부가 잠시 심하게 가려웠다.");
        else if (dmg <= 10)
            /*KR pline("Wounds appear on your body!"); */
            pline("몸에 상처가 생겼다!");
        else if (dmg <= 20)
            /*KR pline("Severe wounds appear on your body!"); */
            pline("몸에 심각한 상처가 생겼다!");
        else
            /*KR Your("body is covered with painful wounds!"); */
            Your("몸이 고통스러운 상처들로 뒤덮였다!");
        break;
    default:
        impossible("mcastu: invalid clerical spell (%d)", spellnum);
        dmg = 0;
        break;
    }

    if (dmg)
        mdamageu(mtmp, dmg);
}

STATIC_DCL
boolean
is_undirected_spell(adtyp, spellnum)
unsigned int adtyp;
int spellnum;
{
    if (adtyp == AD_SPEL) {
        switch (spellnum) {
        case MGC_CLONE_WIZ:
        case MGC_SUMMON_MONS:
        case MGC_AGGRAVATION:
        case MGC_DISAPPEAR:
        case MGC_HASTE_SELF:
        case MGC_CURE_SELF:
            return TRUE;
        default:
            break;
        }
    } else if (adtyp == AD_CLRC) {
        switch (spellnum) {
        case CLC_INSECTS:
        case CLC_CURE_SELF:
            return TRUE;
        default:
            break;
        }
    }
    return FALSE;
}

/* Some spells are useless under some circumstances. */
STATIC_DCL
boolean
spell_would_be_useless(mtmp, adtyp, spellnum)
struct monst *mtmp;
unsigned int adtyp;
int spellnum;
{
    /* Some spells don't require the player to really be there and can be cast
     * by the monster when you're invisible, yet still shouldn't be cast when
     * the monster doesn't even think you're there.
     * This check isn't quite right because it always uses your real position.
     * We really want something like "if the monster could see mux, muy".
     */
    boolean mcouldseeu = couldsee(mtmp->mx, mtmp->my);

    if (adtyp == AD_SPEL) {
        /* aggravate monsters, etc. won't be cast by peaceful monsters */
        if (mtmp->mpeaceful
            && (spellnum == MGC_AGGRAVATION || spellnum == MGC_SUMMON_MONS
                || spellnum == MGC_CLONE_WIZ))
            return TRUE;
        /* haste self when already fast */
        if (mtmp->permspeed == MFAST && spellnum == MGC_HASTE_SELF)
            return TRUE;
        /* invisibility when already invisible */
        if ((mtmp->minvis || mtmp->invis_blkd) && spellnum == MGC_DISAPPEAR)
            return TRUE;
        /* peaceful monster won't cast invisibility if you can't see
           invisible,
           same as when monsters drink potions of invisibility.  This doesn't
           really make a lot of sense, but lets the player avoid hitting
           peaceful monsters by mistake */
        if (mtmp->mpeaceful && !See_invisible && spellnum == MGC_DISAPPEAR)
            return TRUE;
        /* healing when already healed */
        if (mtmp->mhp == mtmp->mhpmax && spellnum == MGC_CURE_SELF)
            return TRUE;
        /* don't summon monsters if it doesn't think you're around */
        if (!mcouldseeu
            && (spellnum == MGC_SUMMON_MONS
                || (!mtmp->iswiz && spellnum == MGC_CLONE_WIZ)))
            return TRUE;
        if ((!mtmp->iswiz || context.no_of_wizards > 1)
            && spellnum == MGC_CLONE_WIZ)
            return TRUE;
        /* aggravation (global wakeup) when everyone is already active */
        if (spellnum == MGC_AGGRAVATION) {
            /* if nothing needs to be awakened then this spell is useless
               but caster might not realize that [chance to pick it then
               must be very small otherwise caller's many retry attempts
               will eventually end up picking it too often] */
            if (!has_aggravatables(mtmp))
                return rn2(100) ? TRUE : FALSE;
        }
    } else if (adtyp == AD_CLRC) {
        /* summon insects/sticks to snakes won't be cast by peaceful monsters
         */
        if (mtmp->mpeaceful && spellnum == CLC_INSECTS)
            return TRUE;
        /* healing when already healed */
        if (mtmp->mhp == mtmp->mhpmax && spellnum == CLC_CURE_SELF)
            return TRUE;
        /* don't summon insects if it doesn't think you're around */
        if (!mcouldseeu && spellnum == CLC_INSECTS)
            return TRUE;
        /* blindness spell on blinded player */
        if (Blinded && spellnum == CLC_BLIND_YOU)
            return TRUE;
    }
    return FALSE;
}

/* convert 1..10 to 0..9; add 10 for second group (spell casting) */
#define ad_to_typ(k) (10 + (int) k - 1)

/* monster uses spell (ranged) */
int
buzzmu(mtmp, mattk)
register struct monst *mtmp;
register struct attack *mattk;
{
    /* don't print constant stream of curse messages for 'normal'
       spellcasting monsters at range */
    if (mattk->adtyp > AD_SPC2)
        return (0);

    if (mtmp->mcan) {
        cursetxt(mtmp, FALSE);
        return (0);
    }
    if (lined_up(mtmp) && rn2(3)) {
        nomul(0);
        if (mattk->adtyp && (mattk->adtyp < 11)) { /* no cf unsigned >0 */
            if (canseemon(mtmp))
#if 0 /*KR: 원본*/
                pline("%s zaps you with a %s!", Monnam(mtmp),
                      flash_types[ad_to_typ(mattk->adtyp)]);
#else /*KR: KRNethack 맞춤 번역 */
                pline("%s 당신에게 %s 발사했다!",
                      append_josa(Monnam(mtmp), "가"),
                      append_josa(flash_types[ad_to_typ(mattk->adtyp)],
                                  "을"));
#endif
            buzz(-ad_to_typ(mattk->adtyp), (int) mattk->damn, mtmp->mx,
                 mtmp->my, sgn(tbx), sgn(tby));
        } else
            impossible("Monster spell %d cast", mattk->adtyp - 1);
    }
    return (1);
}

/*mcastu.c*/
