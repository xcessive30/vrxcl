#include "../quake2/g_local.h"
#include "../gamemodes/ctf.h"

int JoinedPlayers();

void vrx_check_for_levelup(edict_t *ent);

char *LoPrint(char *text) {
    int i;

    if (!text)
        return NULL;
    for (i = 0; i < strlen(text); i++)
        if ((byte) text[i] > 127)
            text[i] = (byte) text[i] - 128;

    return text;
}

char *HiPrint(char *text) {
    int i;
    char *ReturnVal;

    ReturnVal = V_Malloc(strlen(text) + 1, TAG_LEVEL);

    strcpy(ReturnVal, text);

    if (!text)
        return NULL;
    for (i = 0; i < strlen(ReturnVal); i++)
        if ((byte) ReturnVal[i] <= 127)
            ReturnVal[i] = (byte) ReturnVal[i] + 128;
    return ReturnVal;
}

// this needs to match UpdateFreeAbilities() in v_utils.c
void vrx_add_levelup_boons(edict_t *ent) {
    if ((ent->myskills.level % 5) == 0) {
        if (ent->myskills.abilities[MAX_AMMO].level < ent->myskills.abilities[MAX_AMMO].max_level) {
            ent->myskills.abilities[MAX_AMMO].level++;
            ent->myskills.abilities[MAX_AMMO].current_level++;
        }
        if (ent->myskills.abilities[AMMO_REGEN].level < ent->myskills.abilities[MAX_AMMO].max_level) {
            ent->myskills.abilities[AMMO_REGEN].level++;
            ent->myskills.abilities[AMMO_REGEN].current_level++;
        }

        if (ent->myskills.abilities[POWER_REGEN].level < ent->myskills.abilities[MAX_AMMO].max_level) {
            ent->myskills.abilities[POWER_REGEN].level++;
            ent->myskills.abilities[POWER_REGEN].current_level++;
        } else ent->myskills.speciality_points++;

        if (ent->myskills.abilities[VITALITY].level < ent->myskills.abilities[VITALITY].max_level) {
            ent->myskills.abilities[VITALITY].level++;
            ent->myskills.abilities[VITALITY].current_level++;
        } else ent->myskills.speciality_points++;
    }

    // free scanner at level 10
    if (ent->myskills.level == 10) {
        if (!ent->myskills.abilities[SCANNER].level) {
            ent->myskills.abilities[SCANNER].level++;
            ent->myskills.abilities[SCANNER].current_level++;
        } else
            ent->myskills.speciality_points += 2;
    }

    //Give the player talents if they are eligible.
    //if(ent->myskills.level >= TALENT_MIN_LEVEL && ent->myskills.level <= TALENT_MAX_LEVEL)
    if (ent->myskills.level > 1 && !(ent->myskills.level % 2)) // Give a talent point every two levels.
        ent->myskills.talents.talentPoints++;
}

gitem_t *GetWeaponForNumber(int i) {
    switch (i) {
        case 2:
            return Fdi_SHOTGUN;
        case 3:
            return Fdi_SUPERSHOTGUN;
        case 4:
            return Fdi_MACHINEGUN;
        case 5:
            return Fdi_CHAINGUN;
        case 6:
            return Fdi_GRENADES;
        case 7:
            return Fdi_GRENADELAUNCHER;
        case 8:
            return Fdi_ROCKETLAUNCHER;
        case 9:
            return Fdi_HYPERBLASTER;
        case 10:
            return Fdi_RAILGUN;
        case 11:
            return Fdi_BFG;
    }
    return Fdi_ROCKETLAUNCHER;
}

double vrx_get_points_tnl(int level) {
    double lerp = pow( (level - 1) / 48.0, 1.3 );
    float level1XpTnl = 2500;
    float level49XpTnl = 180000;
    return (1 - lerp) * level1XpTnl + lerp * level49XpTnl;
}

void vrx_check_for_levelup(edict_t *ent) {
    double points_needed;
    qboolean levelup = false;

    if (ent->ai.is_bot) // bots don't level up -az
        return;

    while (ent->myskills.experience >= ent->myskills.next_level) {
        levelup = true;

        // maximum level cap
        if (!ent->myskills.administrator && ent->myskills.level >= 50) // cap to 50
        {
            ent->myskills.next_level = ent->myskills.experience;
            return;
        }

        ent->myskills.level++;
        points_needed = vrx_get_points_tnl(ent->myskills.level);


        ent->myskills.next_level += points_needed;

        ent->myskills.speciality_points += 2;
        if (generalabmode->value && ent->myskills.class_num == CLASS_WEAPONMASTER)
            ent->myskills.weapon_points += 6;
        else // 4 points for everyone, only weaponmasters in generalabmode.
            ent->myskills.weapon_points += 5;

        vrx_add_levelup_boons(ent);//Add any special addons that should be there!
        vrx_update_all_character_maximums(ent);

        G_PrintGreenText(va("*****%s gained a level*****", ent->client->pers.netname));
        vrx_write_to_logfile(ent, va("Player reached level %d\n", ent->myskills.level));

        // maximum level cap
        if (!ent->myskills.administrator && ent->myskills.level >= 50) {
            ent->myskills.next_level = ent->myskills.experience;
            break;
        }
    }

    if (levelup) {
        gi.centerprintf(ent, "Welcome to level %d!\n You need %d experience \nto get to the next level.\n",
                        ent->myskills.level, ent->myskills.next_level - ent->myskills.experience);
        gi.sound(ent, CHAN_ITEM, gi.soundindex("misc/pc_up.wav"), 1, ATTN_STATIC, 0);
    }
}

int vrx_add_credits(edict_t *ent, float level_diff, int bonus, qboolean client) {
    int add_credits = 0;
    int streak;

    streak = ent->myskills.streak;

    if (client)
        add_credits = level_diff * (vrx_creditmult->value * vrx_pvpcreditmult->value * (CREDITS_PLAYER_BASE + streak));
    else
        add_credits = level_diff * (vrx_creditmult->value * vrx_pvmcreditmult->value * (CREDITS_OTHER_BASE));

    // vrxchile v1.5 no credit cap
    //if (add_credits > 250)
    //	add_credits = 250;

    add_credits += bonus;

    //FIXME: remove this after allocating more space
    if (ent->myskills.credits + add_credits > MAX_CREDITS)
        ent->myskills.credits = MAX_CREDITS;
    else
        ent->myskills.credits += add_credits;

    return add_credits;
}

void VortexSpreeAbilities(edict_t *attacker) {
    // NOTE: Spree MUST be incremented before calling this function
    // otherwise the player will keep getting 10 seconds of quad/invuln

    //New quad/invin duration variables
    int base_duration = 5 / FRAMETIME;    //5 seconds
    int kill_duration = 0.5 / FRAMETIME;    //0.5 seconds per kill

    if (!attacker->client)
        return;
    if (attacker->myskills.streak < 6)
        return;
    if (HasFlag(attacker))
        return;
    /*
    //Talent: Longer Powerups
    if(vrx_get_talent_slot(attacker, TALENT_LONG_POWERUPS) != -1)
    {
    int level = vrx_get_talent_level(attacker, TALENT_LONG_POWERUPS);
    int baseBonus, killBonus;

    switch(level)
    {
    case 1:		baseBonus = 0;		killBonus = 10;		break;
    case 2:		baseBonus = 0;		killBonus = 15;		break;
    case 3:		baseBonus = 0;		killBonus = 20;		break;
    default:	baseBonus = 0;		killBonus = 0;		break;
    }

    base_duration += baseBonus;
    kill_duration += killBonus;
    }
    */
    // special circumstances if they have both create quad and create invin
    if ((attacker->myskills.abilities[CREATE_QUAD].current_level > 0)
        && (attacker->myskills.abilities[CREATE_INVIN].current_level > 0)) {
        // if they already have it, give them another second
        if (attacker->client->quad_framenum > level.framenum)
            attacker->client->quad_framenum += kill_duration;
        else if (attacker->client->invincible_framenum > level.framenum)
            attacker->client->invincible_framenum += kill_duration;
        else if (!(attacker->myskills.streak % 10)) {
            // give them quad OR invin, not both!
            if (random() > 0.5)
                attacker->client->quad_framenum = level.framenum + base_duration;
            else
                // (apple)
                // invincible_framenum would add up level.framenum + base_duration
                attacker->client->invincible_framenum = level.framenum + base_duration;
        }
    }
        // does the attacker have create quad?
    else if (attacker->myskills.abilities[CREATE_QUAD].current_level > 0) {
        if (!attacker->myskills.abilities[CREATE_QUAD].disable) {
            // every 10 frags, give them 5 seconds of quad
            if (!(attacker->myskills.streak % 10))
                attacker->client->quad_framenum = level.framenum + base_duration;
                // if they already have quad, give them another second
            else if (attacker->client->quad_framenum > level.framenum)
                attacker->client->quad_framenum += kill_duration;
        }
    }
        // does the attacker have create invin?
    else if (attacker->myskills.abilities[CREATE_INVIN].current_level > 0) {
        if (!attacker->myskills.abilities[CREATE_INVIN].disable) {
            // every 10 frags, give them 5 seconds of invin
            if (!(attacker->myskills.streak % 10))
                attacker->client->invincible_framenum = level.framenum + base_duration;
                // if they already have invin, give them another second
            else if (attacker->client->invincible_framenum > level.framenum)
                attacker->client->invincible_framenum += kill_duration;
        }
    }
}

#define PLAYTIME_MIN_MINUTES        999.0    // minutes played before penalty begins
#define PLAYTIME_MAX_MINUTES        999.0    // minutes played before max penalty is reached
#define PLAYTIME_MAX_PENALTY        2.0        // reduce experience in half

int V_AddFinalExp(edict_t *player, int exp) {
    float mod, playtime_minutes;

    // reduce experience as play time increases
    playtime_minutes = player->myskills.playingtime / 60.0;
    if (playtime_minutes > PLAYTIME_MIN_MINUTES) {
        mod = 1.0 + playtime_minutes / PLAYTIME_MAX_MINUTES;
        if (mod >= PLAYTIME_MAX_PENALTY)
            mod = PLAYTIME_MAX_PENALTY;
        exp /= mod;
    }

    // player must pay back "hole" experience first
    if (player->myskills.nerfme > 0) {
        if (player->myskills.nerfme > exp) {
            player->myskills.nerfme -= exp;
            return 0;
        }

        exp -= player->myskills.nerfme;
    }

    if (player->myskills.level < 50) // hasn't reached the cap
    {
        if (!player->ai.is_bot) // not a bot? have exp
        {
            player->myskills.experience += exp;
        }
    }
    player->client->resp.score += exp;
    vrx_check_for_levelup(player);

    return exp;
}


void vrx_add_exp(edict_t *attacker, edict_t *targ);

void hw_deathcleanup(edict_t *targ, edict_t *attacker);

float vrx_get_nfer_bonus(edict_t *attacker, const edict_t *target, float bonus);

float vrx_get_level_difference_multiplier(const edict_t *attacker, const edict_t *targ, const edict_t *target);

float vrx_get_spree_bonus(const edict_t *attacker, float bonus);

float vrx_get_target_alliance_bonus(const edict_t *attacker, const edict_t *target, float bonus);

void getPlayerKillXp(edict_t *attacker, const edict_t *target, float level_diff, float dmgmod, const qboolean *is_mini,
                     int *base_exp, int *credits, int *break_points, float *bonus);

void vrx_do_nfer_effects(edict_t *attacker, edict_t *target);

void
awardMonsterXP(edict_t *attacker, const edict_t *targ, float dmgmod, int *base_exp,
               int *credits, float *level_diff);



int PVP_AwardKill(edict_t *attacker, edict_t *targ, edict_t *targetclient) {
    int max_points, base_exp;
    int credits = 0;
    int exp_points = 0;
    int break_points = 0;
    float level_diff = 0;
    float bonus = 1;
    float dmgmod = 1;
    float damage;
    char name[50];
    int clevel;
    qboolean is_mini = false;

    // sanity check
    if (!attacker || !attacker->inuse || !attacker->client)
        return 0;

    // don't give exp for friendly fire in invasion mode
    if ((targ->client ||
         (targ->owner && targ->owner->client) ||
         (targ->activator && targ->activator->client)) &&
        invasion->value > 0)
        return 0;

    // don't award points for monster that was just resurrected
    if (targ->monsterinfo.resurrected_time > level.time)
        return 0;

    name[0] = 0;// initialize

    // is this a mini-boss?
    if (IsNewbieBasher(targ))
        is_mini = true;

    // calculate damage modifier
    damage = GetPlayerBossDamage(attacker, targ);
    if (damage < 1)
        return 0;

    dmgmod = damage / GetTotalBossDamage(targ);

    // calculate level difference modifier
    level_diff = vrx_get_level_difference_multiplier(attacker, targ, targetclient);

    // calculate spree bonuses
    bonus = vrx_get_spree_bonus(attacker, bonus);

    // players get extra points for killing an allied teammate
    // if they are not allied with anyone themselves
    bonus = vrx_get_target_alliance_bonus(attacker, targetclient, bonus);

    // we killed another player
    if (targ->client) {
        getPlayerKillXp(attacker, targetclient, level_diff, dmgmod, &is_mini, &base_exp, &credits, &break_points, &bonus);

        strcat(name, targetclient->client->pers.netname);
        clevel = targetclient->myskills.level;
    }
        // we killed something else
    else {
        awardMonsterXP(attacker, targ, dmgmod, &base_exp, &credits, &level_diff);

        strcat(name, V_GetMonsterName(targ));
        clevel = targ->monsterinfo.level;
    }

    exp_points = dmgmod * (level_diff * vrx_pointmult->value * base_exp * bonus + break_points);

    if (G_GetClient(targ)) // chile v1.1: pvp has another value.
        exp_points *= vrx_pvppointmult->value;
    else
        exp_points *= vrx_pvmpointmult->value;

    if (hw->value && attacker->client->pers.inventory[halo_index])
        exp_points *= 1.5; // extra experience for halo user

    // award experience to allied players
    max_points = exp_points;

    if (!allies->value || ((exp_points = AddAllyExp(attacker, max_points)) < 1))
        // award experience to non-allied players
        exp_points = V_AddFinalExp(attacker, max_points);

    if (!attacker->ai.is_bot) {
        char *s1 = HiPrint(va("%dxp + $%d", exp_points, credits));
        char *s2 = HiPrint(va("%s (%d)", name, clevel));

        gi.cprintf(attacker,
                   PRINT_HIGH, "Gained %s (%.0f%% dmg. to %s) \n",
                   s1,
                   (dmgmod * 100), s2);

        V_Free(s1);
        V_Free(s2);
    }

    return exp_points;
}

double getOwnLevelBaseBonus(int level, float xpPerKill) {
    /* const float gainPow = 1.2;
    const float xpPerMin = 250;
    const float xpPerMinIncreasePerLevel = 11;


    float ratio = xpPerKill / xpPerMin;
    float growth = ratio * xpPerMinIncreasePerLevel;

    return growth * pow( level - 1, gainPow ); // depends on level cap -az
     */

    return 0;
}


void
awardMonsterXP(edict_t *attacker, const edict_t *targ, float dmgmod, int *base_exp,
               int *credits, float *level_diff) {
    (*base_exp) = EXP_WORLD_MONSTER + getOwnLevelBaseBonus(attacker->myskills.level, EXP_PLAYER_BASE);

//4.5 monster bonus flags
    if (targ->monsterinfo.bonus_flags & BF_UNIQUE_FIRE
        || targ->monsterinfo.bonus_flags & BF_UNIQUE_LIGHTNING) {
        (*level_diff) *= 15.0;
    } else if (targ->monsterinfo.bonus_flags & BF_CHAMPION) {
        (*level_diff) *= 3.0;
    }

    if (targ->monsterinfo.bonus_flags & BF_GHOSTLY || targ->monsterinfo.bonus_flags & BF_FANATICAL
        || targ->monsterinfo.bonus_flags & BF_BERSERKER || targ->monsterinfo.bonus_flags & BF_GHOSTLY
        || targ->monsterinfo.bonus_flags & BF_STYGIAN)
        (*level_diff) *= 1.5;

// control cost bonus (e.g. tanks)
    if (targ->monsterinfo.control_cost > 33)
        (*level_diff) *= (0.75 * targ->monsterinfo.control_cost) / 30;

    // award credits for kill
    (*credits) = vrx_add_credits(attacker, (dmgmod * (*level_diff)), 0, false);


// increment spree counter in invasion/pvm mode
    if (invasion->value || pvm->value) {
        attacker->myskills.streak++;
        VortexSpreeAbilities(attacker);
    }
}


void getPlayerKillXp(edict_t *attacker, const edict_t *target, float level_diff, float dmgmod, const qboolean *is_mini,
                     int *base_exp, int *credits, int *break_points, float *bonus) {

    // spree break bonus points
    if (target->myskills.streak >= SPREE_START)
        (*break_points) = SPREE_BREAKBONUS;
        // you get the same bonus for killing a newbie basher as you would breaking a spree war
    else if ((*is_mini) || (target->myskills.streak >= SPREE_WARS_START && SPREE_WARS))
        (*break_points) = SPREE_WARS_BONUS;

    // award 2fer bonus
    (*bonus) *= vrx_get_nfer_bonus(attacker, target, (*bonus));

    (*base_exp) = EXP_PLAYER_BASE + getOwnLevelBaseBonus(attacker->myskills.level, EXP_PLAYER_BASE);

// award credits for kill
    (*credits) = dmgmod * vrx_add_credits(attacker, level_diff, 0, true);
}

float vrx_get_target_alliance_bonus(const edict_t *attacker, const edict_t *target, float bonus) {
    if (allies->value && !V_GetNumAllies(attacker) && target)
        bonus += (float) 0.5 * V_GetNumAllies(target);

    if (attacker->myskills.boss)
        bonus += ceil(attacker->myskills.boss * 0.2);
    return bonus;
}

float vrx_get_spree_bonus(const edict_t *attacker, float bonus) {
    if (attacker->myskills.streak >= SPREE_START && !SPREE_WAR)
        bonus += 0.2;
    else if (attacker->myskills.streak >= SPREE_WARS_START && SPREE_WARS)
        bonus += 0.5;
    return bonus;
}

float vrx_get_level_difference_multiplier(const edict_t *attacker, const edict_t *targ, const edict_t *target) {
    float level_diff;
    if (targ->client) // target is a player
        level_diff = (float) (target->myskills.level + 1) / (attacker->myskills.level + 1);
    else // target is a monster/summon
        level_diff = (float) (targ->monsterinfo.level + 1) / (attacker->myskills.level + 1);
    return level_diff;
}

float vrx_get_nfer_bonus(edict_t *attacker, const edict_t *target, float bonus) {
    if (attacker->lastkill >= level.time) {

        if (attacker->nfer < 2)
            attacker->nfer = 2;
        else
            attacker->nfer++;

        bonus += sqrtf(attacker->nfer / 2.f);
        attacker->myskills.num_2fers++;

        vrx_do_nfer_effects(attacker, target);
    }
    return bonus;
}

void vrx_do_nfer_effects(edict_t *attacker, edict_t *target) {
    if (attacker->nfer == 4) {
        gi.sound(attacker, CHAN_VOICE, gi.soundindex("misc/assasin.wav"), 1, ATTN_NORM, 0);    //listo
    } else if (attacker->nfer == 5) {
        gi.sound(attacker, CHAN_VOICE, gi.soundindex("speech/hey.wav"), 1, ATTN_NORM, 0);    //listo
    } else if (attacker->nfer >= 3 && attacker->nfer <= 4) {
        gi.sound(target, CHAN_VOICE, gi.soundindex("misc/excellent.wav"), 1, ATTN_NORM, 0); //listo
    } else if (attacker->nfer == 10) {
        gi.sound(attacker, CHAN_VOICE, gi.soundindex("misc/godlike.wav"), 1, ATTN_NORM, 0);//listo
    }
}

void vrx_add_exp(edict_t *attacker, edict_t *targ) {
    int i, exp_points;
    edict_t *targetclient, *player = NULL;

    // this is a player-monster boss
    if (IsABoss(attacker)) {
        AddBossExp(attacker, targ);
        return;
    }

    if (domination->value) {
        dom_fragaward(attacker, targ);
        return;
    }

    if (ctf->value) {
        CTF_AwardFrag(attacker, targ);
        return;
    }

    if (hw->value) {
        hw_deathcleanup(targ, attacker);
        return;
    }

    attacker = G_GetClient(attacker);
    targetclient = G_GetClient(targ);

    // world monster boss kill
    if (!targetclient && targ->monsterinfo.control_cost >= 100) {
        if (targ->mtype == M_JORG)
            return;

        if (attacker)
            G_PrintGreenText(va("%s puts the smackdown on a world-spawned boss!", attacker->client->pers.netname));

        AwardBossKill(targ);
        return;
    }

    // sanity check
    //if (attacker == targetclient || attacker == NULL || targetclient == NULL || !attacker->inuse || !targetclient->inuse)
    //	return;

    // make sure targetclient is valid
    //if (!targetclient || !targetclient->inuse)
    //	return;

    // award experience and credits to anyone that has hurt the targetclient
    for (i = 0; i < game.maxclients; i++) {
        player = g_edicts + 1 + i;

        // award experience and credits to non-spectator clients
        if (!player->inuse || G_IsSpectator(player) || player == targetclient || player->flags & FL_CHATPROTECT)
            continue;

        PVP_AwardKill(player, targ, targetclient);
    }

    // give your team some experience
    if ((int) (dmflags->value) & (DF_MODELTEAMS | DF_SKINTEAMS)) {
        exp_points = PVP_AwardKill(attacker, targ, targetclient);
        Add_ctfteam_exp(attacker, (int) (0.5 * exp_points));
        return;
    }
}

void VortexDeathCleanup(edict_t *attacker, edict_t *targ) {
    int lose_points = 0;
    float level_diff;

    if (IsABoss(attacker)) {
        targ->myskills.streak = 0;
        return; // bosses don't get frags
    }

    attacker = G_GetClient(attacker);
    targ = G_GetClient(targ);

    //GHz: Ignore invalid data
    if (attacker == NULL || targ == NULL) {
        if (targ)
            targ->myskills.streak = 0;
        return;
    }

    //GHz: Handle suicides
    if (targ == attacker) {
        targ->myskills.streak = 0;
        targ->myskills.suicides++;

        // players don't lose points in PvM mode since it is easy to kill yourself
        if (!pvm->value) {
            lose_points = 0;

            // cap max exp lost at 50 points
            if (lose_points > 50)
                lose_points = 50;

            if (targ->client->resp.score - lose_points > 0) {
                targ->client->resp.score -= lose_points;
                targ->myskills.experience -= lose_points;
            } else {
                targ->myskills.experience -= targ->client->resp.score;
                targ->client->resp.score = 0;
            }
        }

        return;
    }

    if (invasion->value < 2) {
        targ->myskills.fragged++;

        attacker->myskills.frags++;
        attacker->client->resp.frags++;
        attacker->lastkill = level.time + 2;
    }

    if (!ptr->value && !domination->value && !pvm->value && !ctf->value
        && (targ->myskills.streak >= SPREE_START)) {
        //GHz: Reset spree properties for target and credit the attacker
        if (SPREE_WAR == true && targ == SPREE_DUDE) {
            SPREE_WAR = false;
            SPREE_DUDE = NULL;
            attacker->myskills.break_spree_wars++;
        }
        attacker->myskills.break_sprees++;
        gi.bprintf(PRINT_HIGH, "%s broke %s's %d frag killing spree!\n", attacker->client->pers.netname,
                   targ->client->pers.netname, targ->myskills.streak);
    }

    targ->myskills.streak = 0;

    if (IsNewbieBasher(targ))
        gi.bprintf(PRINT_HIGH, "%s wasted a mini-boss!\n", attacker->client->pers.netname);

    level_diff = (float) (targ->myskills.level + 1) / (attacker->myskills.level + 1);
    // don't let 'em spree off players that offer no challenge!
    if (!(IsNewbieBasher(attacker) && (level_diff <= 0.5))) {
        attacker->myskills.streak++;

        if ((ffa->value || V_IsPVP()) && attacker->myskills.streak > 15) {
            tech_dropall(attacker); // you can't use techs on a spree.
        }

        VortexSpreeAbilities(attacker);
    } else
        return;

    if (ptr->value)
        return;
    if (domination->value)
        return;
    if (pvm->value)
        return;
    if (ctf->value)
        return;

    if (attacker->myskills.streak >= SPREE_START) {
        if (attacker->myskills.streak > attacker->myskills.max_streak)
            attacker->myskills.max_streak = attacker->myskills.streak;
        if (attacker->myskills.streak == SPREE_START)
            attacker->myskills.num_sprees++;

        if (attacker->myskills.streak == SPREE_WARS_START && SPREE_WARS > 0)
            attacker->myskills.spree_wars++;

        if ((attacker->myskills.streak >= SPREE_WARS_START) && SPREE_WARS && (!V_GetNumAllies(attacker))
            && !attacker->myskills.boss && !IsNewbieBasher(attacker)) {
            if (SPREE_WAR == false) {
                SPREE_DUDE = attacker;
                SPREE_WAR = true;
                SPREE_TIME = level.time;
                safe_cprintf(attacker, PRINT_HIGH, "You have 2 minutes to war. Get as many frags as you can!\n");
            }

            if (attacker == SPREE_DUDE) {
                G_PrintGreenText(
                        va("%s SPREE WAR: %d frag spree!", attacker->client->pers.netname, attacker->myskills.streak));

            }
        } else if (attacker->myskills.streak >= 10 && GetRandom(1, 2) == 1) {
            G_PrintGreenText(
                    va("%s rampage: %d frag spree!", attacker->client->pers.netname, attacker->myskills.streak));
        } else if (attacker->myskills.streak >= 10) {
            G_PrintGreenText(
                    va("%s is god-like: %d frag spree!", attacker->client->pers.netname, attacker->myskills.streak));
        } else if (attacker->myskills.streak >= SPREE_START) {
            G_PrintGreenText(
                    va("%s is on a %d frag spree!", attacker->client->pers.netname, attacker->myskills.streak));
        }
    }
}

