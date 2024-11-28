#include "spdlog/spdlog.h"
#include <Action/Action.h>
#include <Action/CommonAction.h>
#include <Actor/Chara.h>
#include <Script/NativeScriptApi.h>
#include <ScriptObject.h>
#include <algorithm>
#include <iostream>
#include <memory>
#include <vector>

using namespace Sapphire;
using namespace Sapphire::World::Action;
using namespace Sapphire::Entity;

// Action ID 100 corresponds to Venomous Bite
class ActionVenomousBite : public Sapphire::ScriptAPI::ActionScript
{
public:
  ActionVenomousBite() : Sapphire::ScriptAPI::ActionScript( 100 )
  {
  }

  static constexpr auto Potency = 100;

  void onExecute( Sapphire::World::Action::Action& action ) override
  {
    auto pSource = action.getSourceChara();
    auto pTarget = action.getHitChara();

    if( !pSource->isPlayer() )
      return;

    // Calculate damage
    auto dmg = action.calcDamage( Potency );

    // Apply damage (set magic damage to 0 if Venomous Bite is purely physical)
    action.getActionResultBuilder()->damage( pSource, pTarget, dmg.first, dmg.second );

    // Replace existing Venomous Bite status effect on the target
    pTarget->replaceSingleStatusEffectById( 124 );// Assuming 124 is Venomous Bite ID

    // Apply Venomous Bite status effect to the target
    action.getActionResultBuilder()->applyStatusEffect( pTarget, 124, 45000, 0 );

    // Optional: Log the application for debugging
    spdlog::info( "Venomous Bite applied to enemy {}", pTarget->getId() );
  }
};

// Expose the script to the scripting engine
EXPOSE_SCRIPT( ActionVenomousBite );
