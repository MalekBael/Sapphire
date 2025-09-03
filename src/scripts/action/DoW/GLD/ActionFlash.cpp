#include <Script/NativeScriptApi.h>
#include <ScriptObject.h>
#include <Actor/Chara.h>
#include <Action/CommonAction.h>
#include <Action/Action.h>
#include <StatusEffect/StatusEffect.h>
#include <Math/CalcStats.h>
#include <Actor/BNpc.h>
#include <Util/UtilMath.h>

using namespace Sapphire;
using namespace Sapphire::World::Action;

class ActionFlash : public Sapphire::ScriptAPI::ActionScript
{
public:
  ActionFlash() : Sapphire::ScriptAPI::ActionScript( Flash )
  {
  }

  static constexpr auto Radius = 5;
  static constexpr auto Duration = 12;

  void onExecute( Sapphire::World::Action::Action& action ) override
  {
    auto pSource = action.getSourceChara();
    auto pActionBuilder = action.getActionResultBuilder();

    if( !pActionBuilder )
      return;

    for( auto& pActor : pSource->getInRangeActors() )
    {
      auto pTarget = pActor->getAsChara();

      if( pTarget->isFriendly( *pSource ) )
        continue;

      if( Common::Util::distance( pSource->getPos(), pTarget->getPos() ) > Radius )
        continue;

      if( pSource->getLevel() >= 20 )
        pActionBuilder->applyStatusEffect( pTarget, Blind, ( Duration * 1000 ), 0, true );

      pActor->getAsBNpc()->hateListUpdate( pSource, 30 );
    }
  }
};

EXPOSE_SCRIPT( ActionFlash );