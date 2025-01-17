////////////////////////////////////////////////////////////////////////////////
//
// Project-Hellscream https://hellscream.org
// Copyright (C) 2018-2020 Project-Hellscream-6.2
//  All Rights Reserved.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef TRINITY_OBJECTACCESSOR_H
#define TRINITY_OBJECTACCESSOR_H

#include "Common.h"
#include "Define.h"
#include "UpdateData.h"
#include "GridDefines.h"
#include "Object.h"
#include "Player.h"
#include "Transport.h"

class Creature;
class Corpse;
class Unit;
class GameObject;
class DynamicObject;
class WorldObject;
class Vehicle;
class Map;
class WorldRunnable;
class Transport;

template <class T>
class HashMapHolder
{
    public:

        typedef std::unordered_map<uint64, T*> MapType;
        typedef ACE_RW_Thread_Mutex LockType;

        static void Insert(T* o)
        {
            TRINITY_WRITE_GUARD(LockType, i_lock);
            m_objectMap[o->GetGUID()] = o;
        }

        static void Remove(T* o)
        {
            TRINITY_WRITE_GUARD(LockType, i_lock);
            m_objectMap.erase(o->GetGUID());
        }

        static T* Find(uint64 guid)
        {
            TRINITY_READ_GUARD(LockType, i_lock);
            typename MapType::iterator itr = m_objectMap.find(guid);
            return (itr != m_objectMap.end()) ? itr->second : NULL;
        }

        static MapType& GetContainer() { return m_objectMap; }

        static LockType* GetLock() { return &i_lock; }

    private:

        //Non instanceable only static
        HashMapHolder() {}

        static LockType i_lock;
        static MapType  m_objectMap;
};

class ObjectAccessor
{
    friend class ACE_Singleton<ObjectAccessor, ACE_Null_Mutex>;
    private:
        ObjectAccessor();
        ~ObjectAccessor();
        ObjectAccessor(const ObjectAccessor&);
        ObjectAccessor& operator=(const ObjectAccessor&);

    public:
        // TODO: override these template functions for each holder type and add assertions

        template<class T> static T* GetObjectInOrOutOfWorld(uint64 guid, T* /*typeSpecifier*/)
        {
            return HashMapHolder<T>::Find(guid);
        }

        static Unit* GetObjectInOrOutOfWorld(uint64 guid, Unit* /*typeSpecifier*/)
        {
            if (IS_PLAYER_GUID(guid))
                return (Unit*)GetObjectInOrOutOfWorld(guid, (Player*)NULL);

            if (IS_PET_GUID(guid))
                return (Unit*)GetObjectInOrOutOfWorld(guid, (Pet*)NULL);

            return (Unit*)GetObjectInOrOutOfWorld(guid, (Creature*)NULL);
        }

        // returns object if is in world
        template<class T> static T* GetObjectInWorld(ObjectGuid guid, T* /*typeSpecifier*/)
        {
            return HashMapHolder<T>::Find(guid);
        }

        // Player may be not in world while in ObjectAccessor
        static Player* GetObjectInWorld(ObjectGuid guid, Player* /*typeSpecifier*/)
        {
            uint32 l_LowPart = GUID_LOPART(guid);
            Player* player   = nullptr;

            if (l_LowPart < k_PlayerCacheMaxGuid)
                player = m_PlayersCache[l_LowPart];
            else
                player = HashMapHolder<Player>::Find(guid);

            if (player && player->IsInWorld())
                return player;

            return nullptr;
        }

        static Creature* GetObjectInWorld(uint64 guid, Creature* /*typeSpecifier*/)
        {
            uint32 l_LowPart = GUID_LOPART(guid);

            if (l_LowPart <  k_CreaturesCacheMaxGuid)
                return m_CreaturesCache[l_LowPart];

            return HashMapHolder<Creature>::Find(guid);
        }

        static Unit* GetObjectInWorld(ObjectGuid guid, Unit* /*typeSpecifier*/)
        {
            if (IS_PLAYER_GUID(guid))
                return static_cast<Unit*>(GetObjectInWorld(guid, static_cast<Player*>(nullptr)));

            if (IS_PET_GUID(guid))
                return static_cast<Unit*>(GetObjectInWorld(guid, static_cast<Pet*>(nullptr)));

            return static_cast<Unit*>(GetObjectInWorld(guid, static_cast<Creature*>(nullptr)));
        }

        // returns object if is in map
        template<class T> static T* GetObjectInMap(ObjectGuid guid, Map* map, T* /*typeSpecifier*/)
        {
            if (!map)
                return nullptr;

            if (T* obj = GetObjectInWorld(guid, static_cast<T*>(nullptr)))
                if (obj->GetMap() == map && !obj->IsPreDelete())
                    return obj;

            return nullptr;
        }

        template<class T> static T* GetObjectInWorld(uint32 mapid, float x, float y, uint64 guid, T* /*fake*/)
        {
            T* obj = HashMapHolder<T>::Find(guid);
            if (!obj || obj->GetMapId() != mapid)
                return NULL;

            CellCoord p = Trinity::ComputeCellCoord(x, y);
            if (!p.IsCoordValid())
            {
                TC_LOG_ERROR("server.worldserver", "ObjectAccessor::GetObjectInWorld: invalid coordinates supplied X:%f Y:%f grid cell [%u:%u]", x, y, p.x_coord, p.y_coord);
                return NULL;
            }

            CellCoord q = Trinity::ComputeCellCoord(obj->GetPositionX(), obj->GetPositionY());
            if (!q.IsCoordValid())
            {
                TC_LOG_ERROR("server.worldserver", "ObjectAccessor::GetObjecInWorld: object (GUID: %u TypeId: %u) has invalid coordinates X:%f Y:%f grid cell [%u:%u]", obj->GetGUIDLow(), obj->GetTypeId(), obj->GetPositionX(), obj->GetPositionY(), q.x_coord, q.y_coord);
                return NULL;
            }

            int32 dx = int32(p.x_coord) - int32(q.x_coord);
            int32 dy = int32(p.y_coord) - int32(q.y_coord);

            if (dx > -2 && dx < 2 && dy > -2 && dy < 2)
                return obj;
            else
                return NULL;
        }

        // these functions return objects only if in map of specified object
        static WorldObject* GetWorldObject(WorldObject const&, uint64);
        static Object* GetObjectByTypeMask(WorldObject const&, uint64, uint32 typemask);
        static Corpse* GetCorpse(WorldObject const& u, uint64 guid);
        static GameObject* GetGameObject(WorldObject const& u, uint64 guid);
        static DynamicObject* GetDynamicObject(WorldObject const& u, uint64 guid);
        static AreaTrigger* GetAreaTrigger(WorldObject const& u, uint64 guid);
        static Conversation* GetConversation(WorldObject const& p_U, uint64 p_Guid);
        static Unit* GetUnit(WorldObject const&, uint64 guid);
        static Creature* GetCreature(WorldObject const& u, uint64 guid);
        static Pet* GetPet(WorldObject const&, uint64 guid);
        static Player* GetPlayer(WorldObject const&, uint64 guid);
        static Creature* GetCreatureOrPetOrVehicle(WorldObject const&, uint64);
        static Transport* GetTransport(WorldObject const& u, uint64 guid);

        // these functions return objects if found in whole world
        // ACCESS LIKE THAT IS NOT THREAD SAFE
        static Pet* FindPet(ObjectGuid const& g);
        /// Find player /!\ IN WORLD !!!!
        static Player* FindPlayer(uint64);

        static Player* FindPlayerInOrOutOfWorld(uint64);

        /// Find creature /!\ IN WORLD !!!!
        static Creature* FindCreature(uint64);
        /// Find unit /!\ IN WORLD !!!!
        static Unit* FindUnit(ObjectGuid const& g);
        static Player* FindPlayerByName(const char* name);

        static Player* FindPlayerByNameInOrOutOfWorld(const char* name);

        /// Find gameobject /!\ IN WORLD !!!!
        static GameObject* FindGameObject(uint64);

        static Player* FindPlayerByNameAndRealmId(std::string const& name, uint32 realmId);

        // when using this, you must use the hashmapholder's lock
        static HashMapHolder<Player>::MapType const& GetPlayers()
        {
            return HashMapHolder<Player>::GetContainer();
        }

        // when using this, you must use the hashmapholder's lock
        static HashMapHolder<Creature>::MapType const& GetCreatures()
        {
            return HashMapHolder<Creature>::GetContainer();
        }

        // when using this, you must use the hashmapholder's lock
        static HashMapHolder<GameObject>::MapType const& GetGameObjects()
        {
            return HashMapHolder<GameObject>::GetContainer();
        }

        static void AddObject(Player* object)
        {
            if (object->GetGUIDLow() < k_PlayerCacheMaxGuid)
                m_PlayersCache[object->GetGUIDLow()] = object;

            HashMapHolder<Player>::Insert(object);
        }

        static void AddObject(Creature* object)
        {
            if (object->GetGUIDLow() < k_CreaturesCacheMaxGuid)
                m_CreaturesCache[object->GetGUIDLow()] = object;

            HashMapHolder<Creature>::Insert(object);
        }

        template<class T> static void AddObject(T* object)
        {
            HashMapHolder<T>::Insert(object);
        }

        static void RemoveObject(Player* object)
        {
            if (object->GetGUIDLow() < k_PlayerCacheMaxGuid)
                m_PlayersCache[object->GetGUIDLow()] = nullptr;

            HashMapHolder<Player>::Remove(object);
        }

        static void RemoveObject(Creature* object)
        {
            if (object->GetGUIDLow() < k_CreaturesCacheMaxGuid)
                m_CreaturesCache[object->GetGUIDLow()] = nullptr;

            HashMapHolder<Creature>::Remove(object);
        }

        template<class T> static void RemoveObject(T* object)
        {
            HashMapHolder<T>::Remove(object);
        }

        static void SaveAllPlayers();

        //non-static functions
        void AddUpdateObject(Object* obj)
        {
            TRINITY_GUARD(ACE_Thread_Mutex, i_objectLock);
            i_objects.insert(obj);
        }

        void RemoveUpdateObject(Object* obj)
        {
            TRINITY_GUARD(ACE_Thread_Mutex, i_objectLock);
            i_objects.erase(obj);
        }

        //Thread safe
        Corpse* GetCorpseForPlayerGUID(uint64 guid);
        void RemoveCorpse(Corpse* corpse);
        void AddCorpse(Corpse* corpse);
        void AddCorpsesToGrid(GridCoord const& gridpair, GridType& grid, Map* map);
        Corpse* ConvertCorpseForPlayer(uint64 player_guid, bool insignia = false);

        //Thread unsafe
        void Update(uint32 diff);
        void RemoveOldCorpses();
        void UnloadAll();

    private:
        static void _buildChangeObjectForPlayer(WorldObject*, UpdateDataMapType&);
        static void _buildPacket(Player*, Object*, UpdateDataMapType&);
        void _update();

        typedef std::unordered_map<uint64, Corpse*> Player2CorpsesMapType;
        typedef std::unordered_map<Player*, UpdateData>::value_type UpdateDataValueType;

        std::set<Object*> i_objects;
        Player2CorpsesMapType i_player2corpse;

        ACE_Thread_Mutex i_objectLock;
        ACE_RW_Thread_Mutex i_corpseLock;

        static uint32 k_PlayerCacheMaxGuid;
        static Player** m_PlayersCache;

        static uint32 k_CreaturesCacheMaxGuid;
        static Creature** m_CreaturesCache;
};

#define sObjectAccessor ACE_Singleton<ObjectAccessor, ACE_Null_Mutex>::instance()
#endif
