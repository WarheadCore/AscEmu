/*
Copyright (c) 2014-2021 AscEmu Team <http://www.ascemu.org>
This file is released under the MIT license. See README-MIT for more information.
*/

#pragma once

#include "ManagedPacket.h"
#include "WorldPacket.h"

namespace AscEmu::Packets
{
    class CmsgChannelModerate : public ManagedPacket
    {
    public:
        std::string name;

        CmsgChannelModerate() : CmsgChannelModerate("")
        {
        }

        CmsgChannelModerate(std::string name) :
            ManagedPacket(CMSG_CHANNEL_MODERATE, 0),
            name(name)
        {
        }

    protected:
        bool internalSerialise(WorldPacket& /*packet*/) override
        {
            return false;
        }

        bool internalDeserialise(WorldPacket& packet) override
        {
            packet >> name;
            return true;
        }
    };
}
