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

class ActionRampart : public Sapphire::ScriptAPI::ActionScript
{
public:
  ActionRampart() : Sapphire::ScriptAPI::ActionScript( 10 )
  {
  }

  static constexpr uint32_t Status_Rampart = 71;

  void onExecute( Sapphire::World::Action::Action& action ) override
  {
    auto pSource = action.getSourceChara();
    auto pTarget = action.getHitChara();

    if( !pSource->isPlayer() )
      return;

    pSource->replaceSingleStatusEffectById( Status_Rampart );
    action.getActionResultBuilder()->applyStatusEffectSelf( Status_Rampart, 20000, 0 );
  }
};

EXPOSE_SCRIPT( ActionRampart );
