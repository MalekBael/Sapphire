// code credit to Kooper @ Sapphire Discord

#include "spdlog/spdlog.h"
#include <Action/Action.h>
#include <Action/CommonAction.h>
#include <Actor/Chara.h>
#include <Script/NativeScriptApi.h>
#include <ScriptObject.h>
#include <algorithm>
#include <iostream>
#include <memory>
#include <random>
#include <vector>

using namespace Sapphire;
using namespace Sapphire::World::Action;
using namespace Sapphire::Entity;

// Action ID 97 corresponds to Heavy Shot
class ActionHeavyShot : public Sapphire::ScriptAPI::ActionScript
{
public:
  ActionHeavyShot() : Sapphire::ScriptAPI::ActionScript( 97 )
  {
  }

  static constexpr auto Potency = 150;
  static constexpr uint32_t StraighterShot = 122;

  void onExecute( Sapphire::World::Action::Action& action ) override
  {
    auto pSource = action.getSourceChara();
    auto pTarget = action.getHitChara();

    if( !pSource->isPlayer() )
      return;

    // Calculate damage
    auto dmg = action.calcDamage( Potency );
    action.getActionResultBuilder()->damage( pSource, pTarget, dmg.first, dmg.second );

    // Establish aggro if damage was dealt
    if( dmg.first > 0 )
    {
      pTarget->onActionHostile( pSource );
    }

    // 20% chance to apply "Straighter Shot" status effect
    std::random_device rd;
    std::mt19937 gen( rd() );
    std::uniform_int_distribution<> distr( 1, 100 );

    if( distr( gen ) <= 20 )
    {
      uint32_t duration = 10000;

      pSource->replaceSingleStatusEffectById( StraighterShot );
      action.getActionResultBuilder()->applyStatusEffectSelf( StraighterShot, duration, 0 );
    }
  }
};

// Expose the script to the scripting engine
EXPOSE_SCRIPT( ActionHeavyShot );
