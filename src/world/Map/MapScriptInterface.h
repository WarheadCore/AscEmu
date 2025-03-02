/*
 * AscEmu Framework based on ArcEmu MMORPG Server
 * Copyright (c) 2014-2021 AscEmu Team <http://www.ascemu.org>
 * Copyright (C) 2008-2012 ArcEmu Team <http://www.ArcEmu.org/>
 * Copyright (C) 2005-2007 Ascent Team
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef MAP_SCRIPT_INTERFACE_H
#define MAP_SCRIPT_INTERFACE_H

#include "Objects/ObjectDefines.h"
#include "Objects/Object.h"
#include "Objects/Units/Creatures/Creature.h"

#include "Map/MapCell.h"
#include "Map/MapMgr.h"

class GameObject;
class Creature;
class Unit;
class Player;


//////////////////////////////////////////////////////////////////////////////////////////
/// Class MapScriptInterface
/// Provides an interface to mapmgr for scripts, to obtain objects, get players, etc.
//////////////////////////////////////////////////////////////////////////////////////////
class SERVER_DECL MapScriptInterface
{
    public:
        MapScriptInterface(MapMgr & mgr);
        ~MapScriptInterface();

        template<class T, uint32 TypeId> T* GetObjectNearestCoords(uint32 Entry, float x, float y, float z = 0.0f)
        {
            MapCell* pCell = mapMgr.GetCell(mapMgr.GetPosX(x), mapMgr.GetPosY(y));
            if (pCell == 0)
                return 0;

            T* ClosestObject = NULL;
            float ClosestDist = 999999.0f;
            float CurrentDist = 0;

            ObjectSet::const_iterator iter = pCell->Begin();
            for (; iter != pCell->End(); ++iter)
            {
                CurrentDist = (*iter)->CalcDistance(x, y, (z != 0.0f ? z : (*iter)->GetPositionZ()));
                if (CurrentDist < ClosestDist && (*iter)->getObjectTypeId() == TypeId)
                {
                    if ((Entry && (*iter)->getEntry() == Entry) || !Entry)
                    {
                        ClosestDist = CurrentDist;
                        ClosestObject = ((T*)(*iter));
                    }
                }
            }

            return ClosestObject;
        }

        inline GameObject* GetGameObjectNearestCoords(float x, float y, float z = 0.0f, uint32 Entry = 0)
        {
            return GetObjectNearestCoords<GameObject, TYPEID_GAMEOBJECT>(Entry, x, y, z);
        }

        inline Creature* GetCreatureNearestCoords(float x, float y, float z = 0.0f, uint32 Entry = 0)
        {
            return GetObjectNearestCoords<Creature, TYPEID_UNIT>(Entry, x, y, z);
        }

        inline Creature* findNearestCreature(Object* pObject, uint32_t entry, float maxSearchRange /*= 250.0f*/) const
        {
            MapCell* pCell = mapMgr.GetCell(mapMgr.GetPosX(pObject->GetPositionX()), mapMgr.GetPosY(pObject->GetPositionY()));
            if (pCell == 0)
                return nullptr;

            float CurrentDist = 0;
            Creature* target = nullptr;

            ObjectSet::const_iterator iter = pCell->Begin();
            for (; iter != pCell->End(); ++iter)
            {
                if ((*iter)->isCreature() && (*iter)->getEntry() == entry)
                {
                    target = static_cast<Creature*>((*iter));
                    CurrentDist = (*iter)->CalcDistance(pObject);
                    if (CurrentDist <= maxSearchRange)
                        return target;
                }
            }
            return nullptr;
        }

        inline void GetCreatureListWithEntryInGrid(Creature* pCreature, std::list<Creature*>& container, uint32_t entry, float maxSearchRange /*= 250.0f*/) const
        {
            MapCell* pCell = mapMgr.GetCell(mapMgr.GetPosX(pCreature->GetPositionX()), mapMgr.GetPosY(pCreature->GetPositionY()));
            if (pCell == 0)
                return;

            float CurrentDist = 0;
            Creature* target = nullptr;

            ObjectSet::const_iterator iter = pCell->Begin();
            for (; iter != pCell->End(); ++iter)
            {
                if ((*iter)->isCreature() && (*iter)->getEntry() == entry)
                {
                    target = static_cast<Creature*>((*iter));
                    CurrentDist = (*iter)->CalcDistance(pCreature);
                    if (CurrentDist <= maxSearchRange)
                        container.push_back(target);
                }
            }
        }

        inline Creature* getNearestAssistCreatureInGrid(Creature* pCreature, Unit* enemy, float range /*= 250.0f*/) const
        {
            MapCell* pCell = mapMgr.GetCell(mapMgr.GetPosX(pCreature->GetPositionX()), mapMgr.GetPosY(pCreature->GetPositionY()));
            if (pCell == 0)
                return nullptr;

            float CurrentDist = 0;
            ObjectSet::const_iterator iter = pCell->Begin();
            for (; iter != pCell->End(); ++iter)
            {
                if ((*iter)->isCreature())
                {
                    Creature* helper = (*iter)->ToCreature();
                    if (pCreature != helper)
                    {
                        CurrentDist = (*iter)->CalcDistance(pCreature);
                        if (CurrentDist <= range)
                        {
                            if (helper->getAIInterface()->canAssistTo(pCreature, enemy))
                            {
                                return helper;
                            }
                        }
                    }
                }
            }
            return nullptr;
        }

        inline void GetGameObjectListWithEntryInGrid(Creature* pCreature, std::list<GameObject*>& container, uint32_t entry, float maxSearchRange /*= 250.0f*/) const
        {
            MapCell* pCell = mapMgr.GetCell(mapMgr.GetPosX(pCreature->GetPositionX()), mapMgr.GetPosY(pCreature->GetPositionY()));
            if (pCell == 0)
                return;

            float CurrentDist = 0;
            GameObject* target = nullptr;

            ObjectSet::const_iterator iter = pCell->Begin();
            for (; iter != pCell->End(); ++iter)
            {
                if ((*iter)->isGameObject() && (*iter)->getEntry() == entry)
                {
                    target = reinterpret_cast<GameObject*>((*iter));
                    CurrentDist = (*iter)->CalcDistance(pCreature);
                    if (CurrentDist <= maxSearchRange)
                        container.push_back(target);
                }
            }
        }

        inline Player* GetPlayerNearestCoords(float x, float y, float z = 0.0f, uint32 Entry = 0)
        {
            return GetObjectNearestCoords<Player, TYPEID_PLAYER>(Entry, x, y, z);
        }

        uint32 GetPlayerCountInRadius(float x, float y, float z = 0.0f, float radius = 5.0f);

        GameObject* SpawnGameObject(uint32 Entry, float cX, float cY, float cZ, float cO, bool AddToWorld, uint32 Misc1, uint32 Misc2, uint32 phase = 0xFFFFFFF);
        GameObject* SpawnGameObject(MySQLStructure::GameobjectSpawn* gs, bool AddToWorld);
        Creature* SpawnCreature(uint32 Entry, float cX, float cY, float cZ, float cO, bool AddToWorld, bool tmplate, uint32 Misc1, uint32 Misc2, uint32 phase = 0xFFFFFFF);
        Creature* SpawnCreature(MySQLStructure::CreatureSpawn* sp, bool AddToWorld);

        void DeleteGameObject(GameObject* ptr);
        void DeleteCreature(Creature* ptr);

        MapScriptInterface & operator=(MapScriptInterface & msi)
        {
            if (this != &msi)
            {
                this->mapMgr = msi.mapMgr;
            }

            return *this;
        }

    private:

        MapMgr & mapMgr;
};


class SERVER_DECL StructFactory
{
    private:

        StructFactory() = default;
        ~StructFactory() = default;

    public:

        static StructFactory& getInstance();

        StructFactory(StructFactory&&) = delete;
        StructFactory(StructFactory const&) = delete;
        StructFactory& operator=(StructFactory&&) = delete;
        StructFactory& operator=(StructFactory const&) = delete;
};

#define sStructFactory StructFactory::getInstance()

#endif // MAP_SCRIPT_INTERFACE_H
