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

class ActionClericStance : public Sapphire::ScriptAPI::ActionScript
{
public:
  ActionClericStance() : Sapphire::ScriptAPI::ActionScript( ClericStance )
  {
  }

  void onExecute( Sapphire::World::Action::Action& action ) override
  {
    auto pSource = action.getSourceChara();
    auto pActionBuilder = action.getActionResultBuilder();

    if( pSource->hasStatusEffect( ClericStanceStatus ) )
      pSource->removeSingleStatusEffectById( ClericStanceStatus );
    else
      pActionBuilder->applyStatusEffect( pSource, ClericStanceStatus, 0, 0, false );
  }
};

EXPOSE_SCRIPT( ActionClericStance );