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

class ActionKeenFlurry : public Sapphire::ScriptAPI::ActionScript
{
public:
  ActionKeenFlurry() : Sapphire::ScriptAPI::ActionScript( 77 )
  {
  }

  void onExecute( Sapphire::World::Action::Action& action ) override
  {
    auto pSource = action.getSourceChara();
    auto pTarget = action.getHitChara();

    if( !pSource->isPlayer() )
      return;

    pSource->replaceSingleStatusEffectById( Keen_Flurry );
    action.getActionResultBuilder()->applyStatusEffectSelf( Keen_Flurry, 20000, 0 );
  }
};

EXPOSE_SCRIPT( ActionKeenFlurry );
