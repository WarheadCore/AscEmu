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
 */

#ifndef PET_H
#define PET_H

#include "Creature.h"
#include "Spell/SpellInfo.hpp"
#include "Spell/SpellMgr.hpp"
#include "Macros/PetMacros.hpp"

/* Taken from ItemPetFood.dbc
 * Each value is equal to a flag
 * so 1 << PET_FOOD_BREAD for example
 * will result in a usable value.
 */
enum PET_FOOD
{
    PET_FOOD_NONE,
    PET_FOOD_MEAT,
    PET_FOOD_FISH,
    PET_FOOD_CHEESE,
    PET_FOOD_BREAD,
    PET_FOOD_FUNGUS,
    PET_FOOD_FRUIT,
    PET_FOOD_RAW_MEAT,      /// not used in pet diet
    PET_FOOD_RAW_FISH       /// not used in pet diet
};

enum PetReactState
{
    PET_STATE_PASSIVE       = 0,
    PET_STATE_DEFENSIVE     = 1,
    PET_STATE_AGGRESSIVE    = 2
};

enum PetCommands
{
    PET_ACTION_STAY     = 0,
    PET_ACTION_FOLLOW   = 1,
    PET_ACTION_ATTACK   = 2,
    PET_ACTION_DISMISS  = 3,
    PET_ACTION_CASTING  = 4
};

enum PetActionFeedback : uint8_t
{
    PET_FEEDBACK_NONE,
    PET_FEEDBACK_PET_DEAD,
    PET_FEEDBACK_NOTHING_TO_ATTACK,
    PET_FEEDBACK_CANT_ATTACK_TARGET
};

enum PET_RENAME
{
    PET_RENAME_NOT_ALLOWED    = 0x02,
    PET_RENAME_ALLOWED        = 0x03
};

enum PetSpells
{
    PET_SPELL_PASSIVE   = 0x06000000,
    PET_SPELL_DEFENSIVE,
    PET_SPELL_AGRESSIVE,
    PET_SPELL_STAY      = 0x07000000,
    PET_SPELL_FOLLOW,
    PET_SPELL_ATTACK
};

enum StableState
{
    STABLE_STATE_ACTIVE     = 1,
    STABLE_STATE_PASSIVE    = 2
};

enum HappinessState
{
    UNHAPPY     = 1,
    CONTENT     = 2,
    HAPPY       = 3
};

enum AutoCastEvents
{
    AUTOCAST_EVENT_NONE                 = 0,
    AUTOCAST_EVENT_ATTACK               = 1,
    AUTOCAST_EVENT_ON_SPAWN             = 2,
    AUTOCAST_EVENT_OWNER_ATTACKED       = 3,
    AUTOCAST_EVENT_LEAVE_COMBAT         = 4,
    AUTOCAST_EVENT_COUNT                = 5
};

enum PetType
{
    HUNTERPET   = 1,
    WARLOCKPET  = 2,
};

typedef std::map<SpellInfo const*, uint16> PetSpellMap;
struct PlayerPet;


class SERVER_DECL Pet : public Creature
{
    friend class Player;
    friend class Creature;
    friend class WorldSession;

    // MIT START
public:
    Pet(uint64_t guid);
    ~Pet();

    //////////////////////////////////////////////////////////////////////////////////////////
    // Essential functions

    void Update(unsigned long /*time_passed*/);         // hides function Creature::Update
    // void AddToWorld();                               // not used
    // void AddToWorld(MapMgr* pMapMgr);                // not used
    // void PushToWorld(MapMgr*);                       // not used
    void RemoveFromWorld(bool free_guid);               // hides function Creature::RemoveFromWorld
    // void OnPrePushToWorld();                         // not used
    void OnPushToWorld();                               // hides function Creature::OnPushToWorld
    // void OnPreRemoveFromWorld();                     // not used
    void OnRemoveFromWorld();                           // hides function Object::OnRemoveFromWorld

    //////////////////////////////////////////////////////////////////////////////////////////
    // Owner
    Player* getPlayerOwner() override;

protected:
    Player* m_Owner = nullptr;

public:
    //////////////////////////////////////////////////////////////////////////////////////////
    // Misc
    void giveXp(uint32_t xp);

    // MIT END

        // Override superclass method that returns false
        bool isPet() const override { return true; }

        void LoadFromDB(Player* owner, PlayerPet* pi);
        /// returns false if an error occurred. The caller MUST delete us.
        bool CreateAsSummon(uint32 entry, CreatureProperties const* properties_, Creature* created_from_creature, Player* owner, SpellInfo const* created_by_spell, uint32 type, uint32 expiretime, LocationVector* Vec = NULL, bool dismiss_old_pet = true);

        void InitializeSpells();
        void InitializeMe(bool first);
        void SendSpellsToOwner();
        void SendCastFailed(uint32 spellid, uint8 fail);
        void SendActionFeedback(PetActionFeedback value);
        void BuildPetSpellList(WorldPacket& data);

        inline void SetPetAction(uint32 act) { m_Action = act; }
        inline uint32 GetPetAction(void) { return m_Action; }

        inline void SetPetState(uint32 state) { m_State = state; }
        inline uint32 GetPetState(void) { return m_State; }

        inline void SetPetDiet(uint32 diet) { m_Diet = diet; }
        inline void SetPetDiet()
        { 
            if (myFamily)
                m_Diet = myFamily->petdietflags;
            else
                m_Diet = 0;
        }
        inline uint32 GetPetDiet(void) { return m_Diet; }

        inline AI_Spell* GetAISpellForSpellId(uint32 spellid)
        {
            std::map<uint32, AI_Spell*>::iterator itr = m_AISpellStore.find(spellid);
            if (itr != m_AISpellStore.end())
                return itr->second;
            else
                return NULL;
        }

        void UpdatePetInfo(bool bSetToOffline);
        void Remove(bool bUpdate, bool bSetOffline);
        void Dismiss();
        void setDeathState(DeathState s);

        void PrepareForRemove(bool bUpdate, bool bSetOffline);
        void DelayedRemove(bool bTime, bool dismiss = false, uint32 delay = PET_DELAYED_REMOVAL_TIME);
        void Despawn(uint32 delay, uint32 respawntime);

        bool CanGainXP();
        uint32 GetNextLevelXP(uint32 currentlevel);
        void ApplyStatsForLevel();
        void ApplySummonLevelAbilities();
        void ApplyPetLevelAbilities();
        void UpdateAP();
        void LoadPetAuras(int32 id);
        void SetDefaultActionbar();
        void SetActionBarSlot(uint32 slot, uint32 spell) { ActionBar[slot] = spell; }

        void AddSpell(SpellInfo const* sp, bool learning, bool showLearnSpell = true);
        void RemoveSpell(SpellInfo const* sp, bool showUnlearnSpell = true);
        void WipeTalents();
        uint32 GetUntrainCost();
        void SetSpellState(SpellInfo const* sp, uint16 State);
        uint16 GetSpellState(SpellInfo const* sp);
        bool HasSpell(uint32 SpellID)
        {
            const auto sp = sSpellMgr.getSpellInfo(SpellID);
            if (sp)
                return mSpells.find(sp) != mSpells.end();
            return false;
        }
        inline void RemoveSpell(uint32 SpellID)
        {
            const auto sp = sSpellMgr.getSpellInfo(SpellID);
            if (sp) RemoveSpell(sp);
        }
        inline void SetSpellState(uint32 SpellID, uint16 State)
        {
            const auto sp = sSpellMgr.getSpellInfo(SpellID);
            if (sp) SetSpellState(sp, State);
        }
        inline uint16 GetSpellState(uint32 SpellID)
        {
            if (SpellID == 0)
                return DEFAULT_SPELL_STATE;

            const auto sp = sSpellMgr.getSpellInfo(SpellID);
            if (sp)
                return GetSpellState(sp);
            return DEFAULT_SPELL_STATE;
        }

        AI_Spell* CreateAISpell(SpellInfo const* info);
        inline PetSpellMap* GetSpells() { return &mSpells; }
        inline bool IsSummonedPet() { return Summon; }

        void SetAutoCastSpell(AI_Spell* sp);
        void Rename(std::string NewName);
        inline std::string & GetName() { return m_name; }
        uint32 CanLearnSpell(SpellInfo const* sp);
        void UpdateSpellList(bool showLearnSpells = true);

        // talents
        void SendTalentsToOwner();        // Send talentpoints and talent spells to owner
        inline uint8 GetTPsForLevel(uint32 level) { return (level >= 20) ? uint8(level - 16) >> 2 : 0; }    // pet gain first talent point at lvl 20, then every 4 lvls another point
        inline uint8 GetSpentTPs() { return GetTPsForLevel(getLevel()) - this->getPetTalentPoints(); }    // returns amount of spent talent points

        void HandleAutoCastEvent(AutoCastEvents Type);
        AI_Spell* HandleAutoCastEvent();
        void SetPetSpellState(uint32 spell, uint16 state);
        void SetAutoCast(AI_Spell* sp, bool on);
        float GetHappinessDmgMod() { return 0.25f * GetHappinessState() + 0.5f; };
        bool IsBeingDeleted() { return ScheduledForDeletion; }

        Group* getGroup();

        void Die(Unit* pAttacker, uint32 damage, uint32 spellid);

    protected:

        PetSpellMap mSpells;
        PlayerPet* mPi = nullptr;
        uint32 ActionBar[10] = {0};

        std::map<uint32, AI_Spell*> m_AISpellStore;

        uint32 m_AutoCombatSpell = 0;

        uint32 m_HappinessTimer = PET_HAPPINESS_UPDATE_TIMER;
        uint32 m_PetNumber = 0;
        uint32 m_Action = PET_ACTION_FOLLOW;
        uint32 m_State = PET_STATE_DEFENSIVE;
        uint32 m_ExpireTime = 0;
        uint32 m_Diet = 0;
        time_t reset_time = 0;
        uint32 reset_cost = 0;
        bool bExpires = false;
        bool Summon = false;
        bool ScheduledForDeletion = false;
        std::string m_name;
        HappinessState GetHappinessState();
        void SetNameForEntry(uint32 entry);
        uint32 GetAutoCastTypeForSpell(SpellInfo const* ent);
        void SafeDelete();

    std::list<AI_Spell*> m_autoCastSpells[AUTOCAST_EVENT_COUNT];
};

#endif // _PET_H
