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

class ActionStraightShot : public Sapphire::ScriptAPI::ActionScript
{
public:
  ActionStraightShot() : Sapphire::ScriptAPI::ActionScript( 98 )
  {
  }

  static constexpr auto Potency = 140;

  void onExecute( Sapphire::World::Action::Action& action ) override
  {
    auto pSource = action.getSourceChara();
    auto pTarget = action.getHitChara();

    if( !pSource || !pTarget )
      return;

    auto dmg = action.calcDamage( Potency );
    action.getActionResultBuilder()->damage( pSource, pTarget, dmg.first, dmg.second );
    pSource->removeSingleStatusEffectById( StraighterShot );


    if( dmg.first > 0 )
    {
      pTarget->onActionHostile( pSource );
    }
  }
};

EXPOSE_SCRIPT( ActionStraightShot );