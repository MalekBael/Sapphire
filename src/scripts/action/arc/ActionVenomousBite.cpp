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

    auto dmg = action.calcDamage( Potency );

    if( dmg.first > 0 )
    {
      pTarget->onActionHostile( pSource );
    }

    action.getActionResultBuilder()->damage( pSource, pTarget, dmg.first, dmg.second );

    pTarget->replaceSingleStatusEffectById( Venomous_Bite );

    // Apply Venomous Bite status effect to the target
    action.getActionResultBuilder()->applyStatusEffect( pTarget, Venomous_Bite, 45000, 0 );
  }
};
EXPOSE_SCRIPT( ActionVenomousBite );
