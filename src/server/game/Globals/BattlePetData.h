/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _BATTLE_PET_DATA_STORE_H
#define _BATTLE_PET_DATA_STORE_H

typedef std::map<uint32, BattlePetTemplate*> BattlePetTemplateMap;

class BattlePetDataStoreMgr
{
    BattlePetDataStoreMgr();
    ~BattlePetDataStoreMgr();

public:
    static BattlePetDataStoreMgr* instance();

    void Initialize();
    void LoadBattlePetTemplate();
    void LoadBattlePetNpcTeamMember();
    void ComputeBattlePetSpawns();

    BattlePetTemplate const* GetBattlePetTemplate(uint32 species) const;
    BattlePetTemplate const* GetBattlePetTemplateByEntry(uint32 CreatureID) const;
    uint16 GetRandomBreedID(std::set<uint32> BreedIDs);
    uint8 GetWeightForBreed(uint16 breedID);
    uint8 GetRandomQuailty();
    std::vector<BattlePetNpcTeamMember> GetPetBattleTrainerTeam(uint32 npcID);
private:
    BattlePetTemplateContainer _battlePetTemplateStore;
    BattlePetTemplateMap _battlePetTemplate;
    BattlePetNpcTeamMembers _battlePetNpcTeamMembers;
};

#define sBattlePetDataStore BattlePetDataStoreMgr::instance()

#endif