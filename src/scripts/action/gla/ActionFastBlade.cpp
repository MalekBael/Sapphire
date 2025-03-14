#include <Action/Action.h>
#include <Action/CommonAction.h>
#include <Actor/Chara.h>
#include <Script/NativeScriptApi.h>
#include <ScriptObject.h>
#include <memory>
#include <random>

using namespace Sapphire;
using namespace Sapphire::World::Action;
using namespace Sapphire::Entity;

class ActionFastBlade : public Sapphire::ScriptAPI::ActionScript
{
public:
  ActionFastBlade() : Sapphire::ScriptAPI::ActionScript( 9 )
  {
  }

  static constexpr auto Potency = 150;

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

EXPOSE_SCRIPT( ActionFastBlade );