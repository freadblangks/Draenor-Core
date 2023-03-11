#include "ScriptMgr.h"
#include "ScriptedGossip.h"
#include "PetBattle.h"
#include "PetBattleSystem.h"

enum
{
    PetBattleTrainerFightActionID = GOSSIP_ACTION_INFO_DEF + 0xABCD
};

class npc_PetBattleTrainer : public CreatureScript
{
public:
    npc_PetBattleTrainer() : CreatureScript("npc_PetBattleTrainer") { }

    struct npc_PetBattleTrainerAI : CreatureAI
    {
        bool isTrainer;
        std::set<uint32> questIDs{};

        npc_PetBattleTrainerAI(Creature* me) : CreatureAI(me)
        {
            isTrainer = false;

            if (!sObjectMgr->GetPetBattleTrainerTeam(me->GetEntry()).empty())
                return;

            for (auto const& v : sObjectMgr->GetQuestObjectivesByType(QUEST_OBJECTIVE_TYPE_PET_BATTLE_TAMER))
                if (v.ObjectID == me->GetEntry())
                {
                    isTrainer = true;
                    questIDs.insert(v.QuestID);
                }
        }

        void UpdateAI(uint32 /*diff*/) override { }

        void sGossipHello(Player* player) override
        {
            if (me->isQuestGiver())
                player->PrepareQuestMenu(me->GetGUID());

            //if (isTrainer)
            //{
               // bool check = false;
               // for (auto questID : questIDs)
                   // if (player->GetQuestStatus(questID) == QUEST_STATUS_INCOMPLETE)
                   // {
                        //check = true;
                        //break;
                    //}
                //if (check)
                    //if (BroadcastTextEntry const* bct = sBroadcastTextStore.LookupEntry(62660))
                        //player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, DB2Manager::GetBroadcastTextValue(bct, player->GetSession()->GetSessionDbLocaleIndex()), GOSSIP_SENDER_MAIN, PetBattleTrainerFightActionID);
            //}

            player->TalkedToCreature(me->GetEntry(), me->GetGUID());
            player->SEND_GOSSIP_MENU(player->GetGossipTextId(me), me->GetGUID());
        }
    };

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        player->PlayerTalkClass->ClearMenus();

        if (action == PetBattleTrainerFightActionID)
        {
            player->CLOSE_GOSSIP_MENU();

            static float distance = 10.0f;

            Position playerPosition;
            {
                float l_X = creature->m_positionX + (std::cos(creature->m_orientation) * distance);
                float l_Y = creature->m_positionY + (std::sin(creature->m_orientation) * distance);
                float l_Z = player->GetMap()->GetHeight(l_X, l_Y, MAX_HEIGHT);

                playerPosition.m_positionX = l_X;
                playerPosition.m_positionY = l_Y;
                playerPosition.m_positionZ = l_Z;

                playerPosition.m_orientation = atan2(creature->m_positionY - l_Y, creature->m_positionX - l_X);
                playerPosition.m_orientation = (playerPosition.m_orientation >= 0) ? playerPosition.m_orientation : 2 * M_PI + playerPosition.m_orientation;
            }

            Position trainerPosition;
            {
                trainerPosition.m_positionX = creature->m_positionX;
                trainerPosition.m_positionY = creature->m_positionY;
                trainerPosition.m_positionZ = creature->m_positionZ;
                trainerPosition.m_orientation = creature->m_orientation;
            }

            Position battleCenterPosition;
            {
                battleCenterPosition.m_positionX = (playerPosition.m_positionX + trainerPosition.m_positionX) / 2;
                battleCenterPosition.m_positionY = (playerPosition.m_positionY + trainerPosition.m_positionY) / 2;
                battleCenterPosition.m_positionZ = player->GetMap()->GetHeight(battleCenterPosition.m_positionX, battleCenterPosition.m_positionY, MAX_HEIGHT);
                battleCenterPosition.m_orientation = trainerPosition.m_orientation + M_PI;
            }

            PetBattleRequest* battleRequest = sPetBattleSystem->CreateRequest(player->GetGUID());
            battleRequest->OpponentGuid = creature->GetGUID();

            battleRequest->PetBattleCenterPosition[0] = battleCenterPosition.m_positionX;
            battleRequest->PetBattleCenterPosition[1] = battleCenterPosition.m_positionY;
            battleRequest->PetBattleCenterPosition[2] = battleCenterPosition.m_positionZ;
            battleRequest->BattleFacing = battleCenterPosition.m_orientation;

            battleRequest->TeamPosition[PETBATTLE_TEAM_1][0] = playerPosition.m_positionX;
            battleRequest->TeamPosition[PETBATTLE_TEAM_1][1] = playerPosition.m_positionY;
            battleRequest->TeamPosition[PETBATTLE_TEAM_1][2] = playerPosition.m_positionZ;

            battleRequest->TeamPosition[PETBATTLE_TEAM_2][0] = trainerPosition.m_positionX;
            battleRequest->TeamPosition[PETBATTLE_TEAM_2][1] = trainerPosition.m_positionY;
            battleRequest->TeamPosition[PETBATTLE_TEAM_2][2] = trainerPosition.m_positionZ;

            battleRequest->RequestType = PETBATTLE_TYPE_PVE;

            eBattlePetRequests canEnterResult = sPetBattleSystem->CanPlayerEnterInPetBattle(player, battleRequest);
            if (canEnterResult != PETBATTLE_REQUEST_OK)
            {
                player->GetSession()->SendPetBattleRequestFailed(canEnterResult);
                sPetBattleSystem->RemoveRequest(battleRequest->RequesterGuid);
                return true;
            }

            std::shared_ptr<BattlePetInstance> playerPets[MAX_PETBATTLE_SLOTS];
            std::shared_ptr<BattlePetInstance> wildBattlePets[MAX_PETBATTLE_SLOTS];

            for (size_t i = 0; i < MAX_PETBATTLE_SLOTS; ++i)
            {
                playerPets[i] = nullptr;
                wildBattlePets[i] = nullptr;
            }

            std::vector<BattlePetNpcTeamMember> npcteam = sObjectMgr->GetPetBattleTrainerTeam(creature->GetEntry());

            uint32 wildsPetCount = 0;
            for (BattlePetNpcTeamMember l_Current : npcteam)
            {
                if (wildsPetCount >= MAX_PETBATTLE_SLOTS)
                    break;

                auto battlePetInstance = std::make_shared<BattlePetInstance>();

                uint32 l_DisplayID = 0;

                if (sBattlePetSpeciesStore.LookupEntry(l_Current.Specie) && sObjectMgr->GetCreatureTemplate(sBattlePetSpeciesStore.LookupEntry(l_Current.Specie)->entry))
                {
                    l_DisplayID = sObjectMgr->GetCreatureTemplate(sBattlePetSpeciesStore.LookupEntry(l_Current.Specie)->entry)->Modelid1;

                    if (!l_DisplayID)
                    {
                        l_DisplayID = sObjectMgr->GetCreatureTemplate(sBattlePetSpeciesStore.LookupEntry(l_Current.Specie)->entry)->Modelid2;

                        if (!l_DisplayID)
                        {
                            l_DisplayID = sObjectMgr->GetCreatureTemplate(sBattlePetSpeciesStore.LookupEntry(l_Current.Specie)->entry)->Modelid3;
                            if (!l_DisplayID)
                                l_DisplayID = sObjectMgr->GetCreatureTemplate(sBattlePetSpeciesStore.LookupEntry(l_Current.Specie)->entry)->Modelid4;
                        }
                    }
                }

                battlePetInstance->JournalID.Clear();
                battlePetInstance->Slot = 0;
                battlePetInstance->NameTimeStamp = 0;
                battlePetInstance->Species = l_Current.Specie;
                battlePetInstance->DisplayModelID = l_DisplayID;
                battlePetInstance->XP = 0;
                battlePetInstance->Flags = 0;
                battlePetInstance->Health = 20000;

                for (size_t i = 0; i < MAX_PETBATTLE_ABILITIES; ++i)
                    battlePetInstance->Abilities[i] = l_Current.Ability[i];

                wildBattlePets[wildsPetCount] = battlePetInstance;
                wildBattlePets[wildsPetCount]->OriginalCreature.Clear();
                wildsPetCount++;
            }

            size_t playerPetCount = 0;
            std::shared_ptr<BattlePet>* petSlots = player->GetBattlePetCombatTeam();
            for (size_t i = 0; i < MAX_PETBATTLE_SLOTS; ++i)
            {
                if (!petSlots[i])
                    continue;

                if (playerPetCount >= MAX_PETBATTLE_SLOTS || playerPetCount >= player->GetUnlockedPetBattleSlot())
                    break;

                playerPets[playerPetCount] = std::make_shared<BattlePetInstance>();
                playerPets[playerPetCount]->CloneFrom(petSlots[i]);
                playerPets[playerPetCount]->Slot = playerPetCount;
                playerPets[playerPetCount]->OriginalBattlePet = petSlots[i];

                ++playerPetCount;
            }

            player->GetSession()->SendPetBattleFinalizeLocation(battleRequest);

            PetBattle* petBattle = sPetBattleSystem->CreateBattle();

            petBattle->Teams[PETBATTLE_TEAM_1]->OwnerGuid = player->GetGUID();
            petBattle->Teams[PETBATTLE_TEAM_1]->PlayerGuid = player->GetGUID();
            petBattle->Teams[PETBATTLE_TEAM_2]->OwnerGuid = creature->GetGUID();

            for (size_t i = 0; i < MAX_PETBATTLE_SLOTS; ++i)
            {
                if (playerPets[i])
                    petBattle->AddPet(PETBATTLE_TEAM_1, playerPets[i]);

                if (wildBattlePets[i])
                    petBattle->AddPet(PETBATTLE_TEAM_2, wildBattlePets[i]);
            }

            petBattle->BattleType = battleRequest->RequestType;
            petBattle->PveBattleType = PVE_PETBATTLE_TRAINER;

            player->_petBattleId = petBattle->ID;

            sPetBattleSystem->RemoveRequest(battleRequest->RequesterGuid);

            for (size_t i = 0; i < MAX_PETBATTLE_SLOTS; ++i)
            {
                if (playerPets[i])
                    playerPets[i] = nullptr;

                if (wildBattlePets[i])
                    wildBattlePets[i] = nullptr;
            }

            //player->GetMotionMaster()->MovePointWithRot(PETBATTLE_ENTER_MOVE_SPLINE_ID, playerPosition.m_positionX, playerPosition.m_positionY, playerPosition.m_positionZ, playerPosition.m_orientation);
        }
        else
            player->CLOSE_GOSSIP_MENU();

        return true;
    }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_PetBattleTrainerAI(creature);
    }
};

void AddSC_npc_PetBattleTrainer()
{
    new npc_PetBattleTrainer;
}