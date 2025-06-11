#pragma once

#include "Common.h"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace Sapphire::World::Action
{
  struct StatusModifier {
    Common::ParamModifier modifier;
    int32_t value;
  };

  struct StatusEntry {
    uint16_t id;
    uint32_t duration;
    uint32_t flag;
    std::vector< StatusModifier > modifiers;
  };

  struct StatusEffect {
    std::vector< StatusEntry > caster;
    std::vector< StatusEntry > target;
  };

  struct ActionEntry {
    uint16_t potency = 0;
    uint16_t comboPotency = 0;
    uint16_t flankPotency = 0;
    uint16_t frontPotency = 0;
    uint16_t rearPotency = 0;
    uint16_t curePotency = 0;
    uint16_t restoreMPPercentage = 0;
    std::vector< uint32_t > nextCombo;
    StatusEffect statuses;
  };

  class ActionLut
  {
  public:
    using Lut = std::unordered_map< uint16_t, ActionEntry >;

    static bool validEntryExists( uint16_t actionId );
    static const ActionEntry& getEntry( uint16_t actionId );

    static Lut m_actionLut;
  };
}// fixed C26495 (variable not initialized)