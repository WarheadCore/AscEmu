/*
 * AscEmu Framework based on ArcEmu MMORPG Server
 * Copyright (c) 2014-2021 AscEmu Team <http://www.ascemu.org>
 * Copyright (C) 2008-2012 ArcEmu Team <http://www.ArcEmu.org/>
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


#include "Storage/MySQLDataStore.hpp"
#include "Map/MapMgr.h"
#include "Spell/SpellAuras.h"
#include "Spell/Definitions/PowerType.hpp"
#include "Server/Packets/SmsgControlVehicle.h"
#include "Server/Script/CreatureAIScript.h"

Vehicle::Vehicle() {}

Vehicle::~Vehicle()
{
    for (uint8_t i = 0; i < MAX_VEHICLE_SEATS; ++i)
        delete seats[i];

    installed_accessories.clear();
}


void Vehicle::Load(Unit* vehicleOwner, uint32 creatureEntry, uint32 vehicleid)
{
    if (vehicleOwner == nullptr)
    {
        sLogger.failure("Can't load vehicle without an owner.");
        return;
    }

    vehicle_info = sVehicleStore.LookupEntry(vehicleid);
    if (vehicle_info == nullptr)
    {
        sLogger.failure("Can't load a vehicle without vehicle id or data belonging to it.");
        return;
    }
    else
    {
        for (uint8 i = 0; i < MAX_VEHICLE_SEATS; i++)
        {
            uint32 seatid = vehicle_info->seatID[i];

            if (seatid != 0)
            {
                auto vehicle_seat = sVehicleSeatStore.LookupEntry(seatid);
                if (vehicle_seat == nullptr)
                {
                    sLogger.failure("Invalid seat id %u for seat %u for vehicle id %u", seatid, i, vehicleid);
                    continue;
                }

                seats[i] = new VehicleSeat(vehicle_seat);
            }
        }
    }

    this->creature_entry = creatureEntry;
    this->owner = vehicleOwner;

    if (vehicleOwner == nullptr || vehicle_info == nullptr)
        return;

    switch (vehicle_info->powerType)
    {
        case POWER_TYPE_STEAM:
        case POWER_TYPE_HEAT:
        case POWER_TYPE_BLOOD:
        case POWER_TYPE_OOZE:
        case POWER_TYPE_WRATH:
            vehicleOwner->setPowerType(POWER_TYPE_ENERGY);
            vehicleOwner->setMaxPower(POWER_TYPE_ENERGY, 100);
            vehicleOwner->setPower(POWER_TYPE_ENERGY, 100);
            break;

        case POWER_TYPE_PYRITE:
            vehicleOwner->setPowerType(POWER_TYPE_ENERGY);
            vehicleOwner->setMaxPower(POWER_TYPE_ENERGY, 50);
            vehicleOwner->setPower(POWER_TYPE_ENERGY, 50);
            break;
    }

    for (uint8 i = 0; i < MAX_VEHICLE_SEATS; i++)
        if ((seats[i] != nullptr) && seats[i]->Usable() && (!seats[i]->HasPassenger()))
            freeseats++;

}

bool Vehicle::HasEmptySeat()
{
    if (freeseats > 0)
        return true;
    else
        return false;
}

void Vehicle::AddPassenger(Unit* passenger)
{
    // find seat
    uint32 seatid = MAX_VEHICLE_SEATS;
    for (uint8 i = 0; i < MAX_VEHICLE_SEATS; i++)
        if ((seats[i] != nullptr) && seats[i]->Usable() && (!seats[i]->HasPassenger()))
        {
            seatid = i;
            break;
        }

    // There wasn't one :(
    if (seatid == MAX_VEHICLE_SEATS)
        return;

    AddPassengerToSeat(passenger, seatid);
}

void Vehicle::AddPassengerToSeat(Unit* passenger, uint32 seatid)
{
#if VERSION_STRING > TBC
    if (seats[seatid]->HasPassenger())
        return;

    if (!seats[seatid]->Usable())
        return;

    passenger->RemoveAllAuraType(SPELL_AURA_MOUNTED);

    if (passenger->isPlayer())
        static_cast<Player*>(passenger)->DismissActivePets();

    if (passenger->getCurrentVehicle() != nullptr)
        passenger->getCurrentVehicle()->EjectPassenger(passenger);

    // set moveflags
    // set movement info

    // root passenger
    passenger->setMoveRoot(true);

    passenger->SendPacket(AscEmu::Packets::SmsgControlVehicle().serialise().get());

    passenger->sendHopOnVehicle(owner, seatid);

    LocationVector v(owner->GetPosition());
    v.x += seats[seatid]->GetSeatInfo()->attachmentOffsetX;
    v.y += seats[seatid]->GetSeatInfo()->attachmentOffsetY;
    v.z += seats[seatid]->GetSeatInfo()->attachmentOffsetZ;

    passenger->SetPosition(v, false);

    // Player's client sets these
    if (passenger->isCreature())
    {
#if VERSION_STRING < Cata
        passenger->obj_movement_info.transport_guid = owner->getGuid();
        passenger->obj_movement_info.transport_seat = static_cast<uint8_t>(seatid);
#endif
    }

    if (passenger->isPlayer())
    {
        passenger->SendPacket(AscEmu::Packets::SmsgControlVehicle().serialise().get());

        passenger->addUnitFlags(UNIT_FLAG_PVP_ATTACKABLE);

        static_cast<Player*>(passenger)->setFarsightGuid(owner->getGuid());

        if (seats[seatid]->Controller())
        {
            WorldPacket pack(SMSG_CLIENT_CONTROL_UPDATE, 9);
            pack << owner->GetNewGUID() << uint8(1);
            static_cast<Player*>(passenger)->sendClientControlPacket(owner, 1);

            passenger->setCharmGuid(owner->getGuid());
            owner->setCharmedByGuid(passenger->getGuid());
            owner->addUnitFlags(UNIT_FLAG_PLAYER_CONTROLLED_CREATURE);

            WorldPacket spells(SMSG_PET_SPELLS, 100);
            owner->BuildPetSpellList(spells);
            passenger->SendPacket(&spells);

            static_cast<Player*>(passenger)->SetMover(owner);
        }
    }

    seats[seatid]->AddPassenger(passenger->getGuid());
    passenger->setCurrentVehicle(this);

    if (seats[seatid]->HidesPassenger())
        passenger->addUnitFlags(UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);

    passengercount++;
    freeseats--;

    if (passenger->isPlayer() && passengercount == 1)
    {
        if (owner->isCreature())
        {
            Creature* c = static_cast<Creature*>(owner);
            c->setFaction(passenger->getFactionTemplate());
        }
    }

    // remove spellclick flag if full
    if (!HasEmptySeat())
    {
        owner->removeNpcFlags(UNIT_NPC_FLAG_SPELLCLICK);
        owner->removeNpcFlags(UNIT_NPC_FLAG_PLAYER_VEHICLE);

    }

    if (passenger->isCreature())
    {
        Creature* c = static_cast<Creature*>(passenger);

        if (c->GetScript() != nullptr)
        {
            c->GetScript()->OnEnterVehicle();
        }
    }

    if (owner->isCreature())
    {
        Creature* c = static_cast<Creature*>(owner);

        if (c->GetScript() != nullptr)
        {
            if (passengercount == 1)
            {
                c->GetScript()->OnFirstPassengerEntered(passenger);
            }

            if (!HasEmptySeat())
                c->GetScript()->OnVehicleFull();
        }
    }
#endif
}

void Vehicle::EjectPassenger(Unit* passenger)
{
    if (passenger->getCurrentVehicle() == nullptr)
        return;

    if (passenger->getCurrentVehicle() != this)
        return;

    // find the seat the passenger is on
    uint32 seatid = MAX_VEHICLE_SEATS;
    for (uint8 i = 0; i < MAX_VEHICLE_SEATS; i++)
        if ((seats[i] != nullptr) && seats[i]->Usable() && seats[i]->HasPassenger() && (seats[i]->GetPassengerGUID() == passenger->getGuid()))
        {
            seatid = i;
            break;
        }

    if (seatid == MAX_VEHICLE_SEATS)
        return;

    EjectPassengerFromSeat(seatid);
}

void Vehicle::EjectPassengerFromSeat(uint32 seatid)
{
    if (!seats[seatid]->Usable())
        return;

    if (!seats[seatid]->HasPassenger())
        return;

    Unit* passenger = owner->GetMapMgrUnit(seats[seatid]->GetPassengerGUID());
    if (passenger == nullptr)
        return;

    // set moveflags
    // set movement info

    // remove charmed by if passenger was controller
    if (seats[seatid]->Controller())
    {
        passenger->setCharmGuid(0);
        owner->setCharmedByGuid(0);

        if (passenger->isPlayer())
        {
            owner->removeUnitFlags(UNIT_FLAG_PLAYER_CONTROLLED_CREATURE | UNIT_FLAG_PVP_ATTACKABLE);

            static_cast<Player*>(passenger)->sendClientControlPacket(owner, 0);

            // send null spells if needed
            static_cast<Player*>(passenger)->SendEmptyPetSpellList();
            static_cast<Player*>(passenger)->SetMover(passenger);
        }
    }

    if (passenger->isPlayer())
        static_cast<Player*>(passenger)->setFarsightGuid(0);

    // if we are on a flying vehicle, add a parachute!
    if (owner->hasAuraWithAuraEffect(SPELL_AURA_ENABLE_FLIGHT) || owner->hasAuraWithAuraEffect(SPELL_AURA_ENABLE_FLIGHT2))
        passenger->castSpell(passenger, 45472, false);

    // re-add spellclick flag if needed
    // despawn vehicle if it was spawned by spell?
    LocationVector landposition(owner->GetPosition());

    passenger->sendHopOffVehicle(owner, landposition);
    passenger->SetPosition(landposition);
    passenger->setMoveRoot(false);
    seats[seatid]->RemovePassenger();
    passenger->setCurrentVehicle(nullptr);
    passenger->removeUnitFlags(UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);

    passengercount--;
    freeseats++;

    if (HasEmptySeat())
    {
        if (owner->isPlayer())
            owner->addNpcFlags(UNIT_NPC_FLAG_PLAYER_VEHICLE);
        else
            owner->addNpcFlags(UNIT_NPC_FLAG_SPELLCLICK);
    }

    if (passenger->isPlayer())
        static_cast<Player*>(passenger)->SpawnActivePet();

    if (passenger->isCreature())
    {
        Creature* c = static_cast<Creature*>(passenger);

        if (c->GetScript() != nullptr)
        {
            c->GetScript()->OnExitVehicle();
        }
    }
    if (owner->isCreature())
    {
        Creature* c = static_cast<Creature*>(owner);

        if (c->GetScript() != nullptr)
        {
            if (passengercount == 0)
            {
                c->GetScript()->OnLastPassengerLeft(passenger);
            }
        }
        else{
            // The passenger summoned the vehicle, and we have no script to remove it, so we remove it here
            if ((passengercount == 0) && (c->getSummonedByGuid() == passenger->getGuid()))
                c->Despawn(1 * 1000, 0);
        }
    }
}

void Vehicle::EjectAllPassengers()
{
    for (uint8 i = 0; i < MAX_VEHICLE_SEATS; i++)
        if ((seats[i] != nullptr) && (seats[i]->GetPassengerGUID() != 0))
        {
            Unit* u = owner->GetMapMgr()->GetUnit(seats[i]->GetPassengerGUID());
            if (u == nullptr)
            {
                seats[i]->RemovePassenger();
                continue;
            }

            if (u->getVehicleComponent() != nullptr)
                u->getVehicleComponent()->EjectAllPassengers();
            else
                EjectPassengerFromSeat(i);
        }
}

void Vehicle::MovePassengerToSeat(Unit* passenger, uint32 seat)
{
    uint32 oldseatid = 0;
    for (uint8 i = 0; i < MAX_VEHICLE_SEATS; i++)
        if ((seats[i] != nullptr) && (seats[i]->GetPassengerGUID() == passenger->getGuid()))
        {
            oldseatid = i;
            break;
        }

    // Passenger is not in this vehicle
    /*if (oldseatid == MAX_VEHICLE_SEATS) oldseatid must be between 0 and 7 CID 53115
        return;*/

    if (seats[seat] == nullptr)
        return;

    if (!seats[seat]->Usable())
        return;

    if (seats[seat]->HasPassenger())
        return;

    EjectPassengerFromSeat(oldseatid);
    AddPassengerToSeat(passenger, seat);
}

void Vehicle::MovePassengerToNextSeat(Unit* passenger)
{
    uint32 oldseatid = 0;
    for (uint8 i = 0; i < MAX_VEHICLE_SEATS; i++)
        if ((seats[i] != nullptr) && (seats[i]->GetPassengerGUID() == passenger->getGuid()))
        {
            oldseatid = i;
            break;
        }

    // Passenger is not in this vehicle
    /*if (oldseatid == MAX_VEHICLE_SEATS) oldseatid must be between 0 and 7 CID 53174
        return;*/

    // Now find a next seat if possible
    uint32 newseatid = oldseatid;
    for (uint32 i = oldseatid + 1; i < MAX_VEHICLE_SEATS; i++)
        if ((seats[i] != nullptr) && (seats[i]->Usable()) && (!seats[i]->HasPassenger()))
        {
            newseatid = i;
            break;
        }

    // There's no suitable seat :(
    if (newseatid == oldseatid)
        return;

    EjectPassengerFromSeat(oldseatid);
    AddPassengerToSeat(passenger, newseatid);
}

void Vehicle::MovePassengerToPrevSeat(Unit* passenger)
{
    uint32 oldseatid = MAX_VEHICLE_SEATS;
    for (uint8 i = 0; i < MAX_VEHICLE_SEATS; i++)
        if ((seats[i] != nullptr) && (seats[i]->GetPassengerGUID() == passenger->getGuid()))
        {
            oldseatid = i;
            break;
        }

    // Passenger is not in this vehicle
    if (oldseatid == MAX_VEHICLE_SEATS)
        return;

    // Now find a previous seat if possible
    uint32 newseatid = oldseatid;
    for (int32 i = static_cast<int32>(oldseatid)-1; i >= 0; i--)
        if ((seats[i] != nullptr) && (seats[i]->Usable()) && (!seats[i]->HasPassenger()))
        {
            newseatid = static_cast<uint32>(i);
            break;
        }

    // There's no suitable seat :(
    if (newseatid == oldseatid)
        return;

    EjectPassengerFromSeat(oldseatid);
    AddPassengerToSeat(passenger, newseatid);
}

uint32 Vehicle::GetSeatEntryForPassenger(Unit* passenger)
{
    for (uint8 i = 0; i < MAX_VEHICLE_SEATS; i++)
        if ((seats[i] != nullptr) && (seats[i]->GetPassengerGUID() == passenger->getGuid()))
            return seats[i]->GetSeatInfo()->ID;

    return 0;
}

bool Vehicle::IsControler(Unit* aura)
{
    for (uint8 i = 0; i < MAX_VEHICLE_SEATS; i++)
        if ((seats[i] != nullptr) && (seats[i]->GetPassengerGUID() == aura->getGuid()))
            return seats[i]->GetSeatInfo()->IsController();

    return 0;
}

void Vehicle::MovePassengers(float x, float y, float z, float o)
{
    for (uint8 i = 0; i < MAX_VEHICLE_SEATS; i++)
    {
        if ((seats[i] != nullptr) && (seats[i]->GetPassengerGUID() != 0))
        {
            Unit* passenger = owner->GetMapMgrUnit(seats[i]->GetPassengerGUID());
            if (passenger == nullptr)
                continue;

            passenger->SetPosition(x, y, z, o);
        }
    }
}

uint32 Vehicle::GetPassengerCount() const{
    uint32 count = 0;

    for (uint8 i = 0; i < MAX_VEHICLE_SEATS; i++)
    {
        if ((seats[i] != nullptr) && (seats[i]->GetPassengerGUID() != 0))
        {
            Unit* passenger = owner->GetMapMgrUnit(seats[i]->GetPassengerGUID());
            if (passenger == nullptr)
                continue;

            if (passenger->getVehicleComponent() == nullptr)
                count++;
            else
                count += passenger->getVehicleComponent()->GetPassengerCount();
        }
    }

    return count;
}

uint16 Vehicle::GetMoveFlags2() const{
    uint16 flags2 = 0;

    if (vehicle_info->flags & VEHICLE_FLAG_NO_STRAFE)
        flags2 |= MOVEFLAG2_NO_STRAFING;

    if (vehicle_info->flags & VEHICLE_FLAG_NO_JUMPING)
        flags2 |= MOVEFLAG2_NO_JUMPING;
    if (vehicle_info->flags & VEHICLE_FLAG_FULLSPEEDTURNING)
        flags2 |= MOVEFLAG2_FULLSPEED_TURNING;
    if (vehicle_info->flags & VEHICLE_FLAG_ALLOW_PITCHING)
        flags2 |= MOVEFLAG2_ALLOW_PITCHING;
    if (vehicle_info->flags & VEHICLE_FLAG_FULLSPEEDPITCHING)
        flags2 |= MOVEFLAG2_FULLSPEED_PITCHING;

    return flags2;
}


void Vehicle::InstallAccessories()
{
    if (!installed_accessories.empty())
        return;

    std::vector< VehicleAccessoryEntry* >* v = sObjectMgr.GetVehicleAccessories(creature_entry);
    if (v == nullptr)
        return;

    for (std::vector< VehicleAccessoryEntry* >::iterator itr = v->begin(); itr != v->end(); ++itr)
    {
        VehicleAccessoryEntry *accessory = *itr;

        if (seats[accessory->seat] == nullptr)
            continue;

        if (seats[accessory->seat]->HasPassenger())
            EjectPassengerFromSeat(accessory->seat);

        CreatureProperties const* cp = sMySQLStore.getCreatureProperties(accessory->accessory_entry);
        if (cp == nullptr)
            continue;

        Creature* c = owner->GetMapMgr()->CreateCreature(accessory->accessory_entry);
        c->Load(cp, owner->GetPositionX(), owner->GetPositionY(), owner->GetPositionZ(), owner->GetOrientation());
#if VERSION_STRING < Cata
        c->obj_movement_info.transport_guid = owner->getGuid();
#ifdef FT_VEHICLES
        c->obj_movement_info.transport_seat = static_cast<uint8_t>(accessory->seat);
#endif
#endif
        c->setPhase(PHASE_SET, owner->GetPhase());
        c->setFaction(owner->getFactionTemplate());
        c->PushToWorld(owner->GetMapMgr());

        AddPassengerToSeat(c, accessory->seat);
        installed_accessories.push_back(c->getGuid());
    }
}

void Vehicle::RemoveAccessories()
{
    for (std::vector< uint64 >::iterator itr = installed_accessories.begin(); itr != installed_accessories.end(); ++itr)
    {
        Unit* u = owner->GetMapMgr()->GetUnit(*itr);
        if (u == nullptr)
            continue;

        if (u->getVehicleComponent() != nullptr)
            u->getVehicleComponent()->EjectAllPassengers();

        EjectPassenger(u);
        u->Delete();
    }

    installed_accessories.clear();
}

bool Vehicle::HasAccessoryWithGUID(uint64 guid)
{
    std::vector< uint64 >::iterator itr =
        std::find(installed_accessories.begin(), installed_accessories.end(), guid);

    if (itr == installed_accessories.end())
        return false;
    else
        return true;
}

uint32 Vehicle::GetPassengerSeatId(uint64 guid)
{
    for (uint8 i = 0; i < MAX_VEHICLE_SEATS; i++)
        if ((seats[i] != nullptr && seats[i]->GetPassengerGUID() == guid))
            return seats[i]->GetSeatInfo()->ID;
    return 0;
}

VehicleSeat::VehicleSeat(DBC::Structures::VehicleSeatEntry const* info)
{
    passenger = 0;
    seat_info = info;
}

bool VehicleSeat::Controller() const
{
    return seat_info->IsController();
}

bool VehicleSeat::Usable() const
{
    return seat_info->IsUsable();
}

bool VehicleSeat::HidesPassenger() const
{
    return seat_info->HidesPassenger();
}
