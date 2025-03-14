#include <Action/Action.h>
#include <Action/CommonAction.h>
#include <Actor/Chara.h>
#include <Script/NativeScriptApi.h>
#include <ScriptObject.h>
#include <memory>

using namespace Sapphire;
using namespace Sapphire::World::Action;
using namespace Sapphire::Entity;

class ActionHeavyThrust : public Sapphire::ScriptAPI::ActionScript
{
public:
  ActionHeavyThrust() : Sapphire::ScriptAPI::ActionScript( 79 )
  {
  }

  /*Need to research how to implement positionals. Heavy Thrust has a 170 flank potency*/

  static constexpr auto Potency = 100;

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

    uint32_t duration = 30000;

    pSource->replaceSingleStatusEffectById( ActionStatus::Heavy_Thrust );
    action.getActionResultBuilder()->applyStatusEffectSelf( ActionStatus::Heavy_Thrust, duration, 0 );
  }
};

EXPOSE_SCRIPT( ActionHeavyThrust );
