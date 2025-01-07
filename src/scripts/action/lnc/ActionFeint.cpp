#include <ScriptObject.h>
#include <Action/Action.h>
#include <Actor/Chara.h>
#include <Script/NativeScriptApi.h>
#include <memory>
#include <random>
#include <Action/CommonAction.h>

using namespace Sapphire;
using namespace Sapphire::World::Action;
using namespace Sapphire::Entity;

class ActionFeint : public Sapphire::ScriptAPI::ActionScript
{
public:
  ActionFeint() : Sapphire::ScriptAPI::ActionScript( 76 )
  {
  }

  static constexpr auto Potency = 120;

  void onExecute( Sapphire::World::Action::Action& action ) override
  {
    auto pSource = action.getSourceChara();
    auto pTarget = action.getHitChara();

    if( !pSource || !pTarget )
      return;

    auto dmg = action.calcDamage( Potency );
    action.getActionResultBuilder()->damage( pSource, pTarget, dmg.first, dmg.second );

    if( dmg.first > 0 )
    {
      pTarget->onActionHostile( pSource );
    }

    action.getActionResultBuilder()->damage( pSource, pTarget, dmg.first, dmg.second );

    pTarget->replaceSingleStatusEffectById( Slow );

    // Apply Slow - need to research the duration
    action.getActionResultBuilder()->applyStatusEffect( pTarget, Slow, 6000, 0 );
  }
};

EXPOSE_SCRIPT( ActionFeint );
