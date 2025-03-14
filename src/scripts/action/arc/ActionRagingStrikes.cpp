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

class ActionRagingStrikes : public Sapphire::ScriptAPI::ActionScript
{
public:
  ActionRagingStrikes() : Sapphire::ScriptAPI::ActionScript( 101 )
  {
  }

  static constexpr uint32_t Raging_Strikes = 125;

  void onExecute( Sapphire::World::Action::Action& action ) override
  {
    auto pSource = action.getSourceChara();
    auto pTarget = action.getHitChara();

    if( !pSource->isPlayer() )
      return;

    pSource->replaceSingleStatusEffectById( Raging_Strikes );
    action.getActionResultBuilder()->applyStatusEffectSelf( Raging_Strikes, 20000, 0 );
  }
};

EXPOSE_SCRIPT( ActionRagingStrikes );
