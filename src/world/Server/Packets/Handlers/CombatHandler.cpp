/*
Copyright (c) 2014-2021 AscEmu Team <http://www.ascemu.org>
This file is released under the MIT license. See README-MIT for more information.
*/


#include "Server/Packets/CmsgAttackSwing.h"
#include "Server/WorldSession.h"
#include "Objects/Units/Players/Player.h"
#include "Map/MapMgr.h"
#include "Management/Faction.h"

using namespace AscEmu::Packets;

void WorldSession::handleAttackSwingOpcode(WorldPacket& recvPacket)
{
    CmsgAttackSwing srlPacket;
    if (!srlPacket.deserialise(recvPacket))
        return;

    sLogger.debugFlag(AscEmu::Logging::LF_OPCODE, "Received CMSG_ATTACKSWING: %u (guidLow)", srlPacket.guid.getGuidLow());

    if (_player->IsFeared() || _player->IsStunned() || _player->IsPacified() || _player->isDead())
        return;

    const auto unitTarget = _player->GetMapMgr()->GetUnit(srlPacket.guid.getRawGuid());
    if (unitTarget == nullptr)
        return;

    if (!isAttackable(_player, unitTarget, false) || unitTarget->isDead())
        return;

    _player->smsg_AttackStart(unitTarget);
    _player->EventAttackStart();
}

void WorldSession::handleAttackStopOpcode(WorldPacket& /*recvPacket*/)
{
    const auto unitTarget = _player->GetMapMgr()->GetUnit(_player->getTargetGuid());
    if (unitTarget == nullptr)
        return;

    _player->EventAttackStop();
    _player->smsg_AttackStop(unitTarget);
}
