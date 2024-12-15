// Action script template

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


class ActionStraightShot : public Sapphire::ScriptAPI::ActionScript
{
public:
  ActionStraightShot() : Sapphire::ScriptAPI::ActionScript( 98 )
  {
  }

  // Replace `ActionPotency` with the appropriate potency value
  static constexpr auto Potency = 140;

  void onExecute( Sapphire::World::Action::Action& action ) override
  {
    auto pSource = action.getSourceChara();
    auto pTarget = action.getHitChara();

    // Optional: Include any checks specific to your action
    if( !pSource || !pTarget )
      return;

    // Calculate damage
    auto dmg = action.calcDamage( Potency );
    action.getActionResultBuilder()->damage( pSource, pTarget, dmg.first, dmg.second );
    pSource->removeSingleStatusEffectById( StraighterShot );

    // Establish aggro if damage was dealt
    if( dmg.first > 0 )
    {
      pTarget->onActionHostile( pSource );
    }

    // Include any additional logic specific to your action here
    // For example, applying status effects, combo logic, etc.
  }
};

// Expose the script to the scripting engine
EXPOSE_SCRIPT( ActionStraightShot );
