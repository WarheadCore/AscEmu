/*
Copyright (c) 2014-2021 AscEmu Team <http://www.ascemu.org>
This file is released under the MIT license. See README-MIT for more information.
*/

#pragma once

#include "Management/Arenas.h"
#include "Management/Battleground/Battleground.h"

class CircleOfBlood : public Arena
{
public:
    CircleOfBlood(MapMgr* mgr, uint32_t id, uint32_t lgroup, uint32_t t, uint32_t players_per_side);
    ~CircleOfBlood();

    static CBattleground* Create(MapMgr* m, uint32_t i, uint32_t l, uint32_t t, uint32_t players_per_side)
    {
        return new CircleOfBlood(m, i, l, t, players_per_side);
    }

    void OnCreate();
    void HookOnShadowSight();
    LocationVector GetStartingCoords(uint32_t Team);
    bool HookHandleRepop(Player *plr);
};
