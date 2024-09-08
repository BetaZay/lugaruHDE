/*
Copyright (C) 2010-2017 - Lugaru contributors (see AUTHORS file)

This file is part of Lugaru.

Lugaru is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

Lugaru is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Lugaru.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Level/Awards.hpp"

#include "Game.hpp"
#include "Objects/Person.hpp"

int bonus;
int bonusvalue;
int bonustotal;
int startbonustotal;
float bonustime;
float bonusnum[100];
float wonleveltime = 0;

const char* bonus_names[bonus_count] = {
#define DECLARE_BONUS(id, name, ...) name,
#include "Bonuses.def"
#undef DECLARE_BONUS
};

const char* award_names[award_count] = {
#define DECLARE_AWARD(id, name) name,
#include "Awards.def"
#undef DECLARE_AWARD
};

static const int bonus_values[bonus_count] = {
#define DECLARE_BONUS(id, name, value) value,
#include "Bonuses.def"
#undef DECLARE_BONUS
};

void award_bonus(int playerid, int bonusid, int alt_value)
{
    if (playerid != 0) {
        return;
    }
    bonus = bonusid;
    bonustime = 0;
    bonusvalue = alt_value ? alt_value : bonus_values[bonusid];
}

// FIXME: make these per-player
float damagetaken;
int numfalls;
int numflipfail;
int numseen;
int numresponded;
int numstaffattack;
int numswordattack;
int numknifeattack;
int numunarmedattack;
int numescaped;
int numflipped;
int numwallflipped;
int numthrowkill;
int numafterkill;
int numreversals;
int numattacks;
int maxalarmed;

int award_awards(int* awards, int time)
{
    int numawards = 0;

    // Flawless Award: No damage and no blood loss
    if (damagetaken == 0 && Person::players[0]->bloodloss == 0) {
        awards[numawards] = awardflawless;
        numawards++;
    }

    // All Dead Award: All enemies are dead
    bool alldead = true;
    for (unsigned i = 1; i < Person::players.size(); i++) {
        if (Person::players[i]->dead != 2) {
            alldead = false;
        }
    }
    if (alldead) {
        awards[numawards] = awardalldead;
        numawards++;
    }

    // No Dead Award: All allies are alive
    alldead = true;
    for (unsigned i = 1; i < Person::players.size(); i++) {
        if (Person::players[i]->dead != 1) {
            alldead = false;
        }
    }
    if (alldead) {
        awards[numawards] = awardnodead;
        numawards++;
    }

    // Stealth Award: No alarms triggered and no throw kills
    if (numresponded == 0 && !numthrowkill) {
        awards[numawards] = awardstealth;
        numawards++;
    }

    // Bojutsu Award: All attacks were with a staff
    if (numattacks == numstaffattack && numattacks > 0) {
        awards[numawards] = awardbojutsu;
        numawards++;
    }

    // Swordsman Award: All attacks were with a sword
    if (numattacks == numswordattack && numattacks > 0) {
        awards[numawards] = awardswordsman;
        numawards++;
    }

    // Knife Fighter Award: All attacks were with a knife
    if (numattacks == numknifeattack && numattacks > 0) {
        awards[numawards] = awardknifefighter;
        numawards++;
    }

    // Kung Fu Award: All attacks were unarmed, no throw kills
    if (numattacks == numunarmedattack && numthrowkill == 0 && weapons.size() > 0) {
        awards[numawards] = awardkungfu;
        numawards++;
    }

    // Evasion Award: The player escaped from battle
    if (numescaped > 0) {
        awards[numawards] = awardevasion;
        numawards++;
    }

    // Acrobat Award: No flip failures, and more than 20 flips
    if (numflipfail == 0 && numflipped + numwallflipped * 2 > 20) {
        awards[numawards] = awardacrobat;
        numawards++;
    }

    // Long Range Award: All enemies were killed with throw attacks
    if (numthrowkill == (int(Person::players.size()) - 1)) {
        awards[numawards] = awardlongrange;
        numawards++;
    }

    // Brutal Award: Kills were done after death
    alldead = true;
    for (unsigned i = 1; i < Person::players.size(); i++) {
        if (Person::players[i]->dead != 2) {
            alldead = false;
        }
    }
    if (numafterkill > 0 && alldead) {
        awards[numawards] = awardbrutal;
        numawards++;
    }

    // Aikido Award: High percentage of reversals
    if (numreversals > ((float)numattacks) * .8 && numreversals > 3) {
        awards[numawards] = awardaikido;
        numawards++;
    }

    // Strategy Award: Minimal alarms triggered
    if (maxalarmed == 1 && Person::players.size() > 2) {
        awards[numawards] = awardstrategy;
        numawards++;
    }

    // Klutz Award: Many flip failures
    if (numflipfail > 3) {
        awards[numawards] = awardklutz;
        numawards++;
    }

    // Coward Award: Escaped without fighting
    if (numattacks == 0) {
        awards[numawards] = awardcoward;
        numawards++;
    }

    // Calculate the number of enemies defeated
    int numEnemiesDefeated = 0;
    for (unsigned i = 1; i < Person::players.size(); i++) {
        if (Person::players[i]->dead > 0) {
            numEnemiesDefeated++;
        }
    }

    // Avoid division by zero
    if (numEnemiesDefeated > 0) {
        // Calculate average time per enemy
        float avgTimePerEnemy = time / numEnemiesDefeated;

        // Fast, Real Fast, and Damn Fast Awards based on avg time per enemy
        if (avgTimePerEnemy <= 5) {
            awards[numawards] = awardhedgehog;
            numawards++;
        } else if (avgTimePerEnemy <= 7) {
            awards[numawards] = awardrealfast;
            numawards++;
        } else if (avgTimePerEnemy <= 10) {
            awards[numawards] = awardfast;
            numawards++;
        }
    }

    return numawards;
}