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

class ActionBloodForBlood : public Sapphire::ScriptAPI::ActionScript
{
public:
  ActionBloodForBlood() : Sapphire::ScriptAPI::ActionScript( 85 )
  {
  }

  static constexpr uint32_t Blood_For_Blood = 117;

  void onExecute( Sapphire::World::Action::Action& action ) override
  {
    auto pSource = action.getSourceChara();
    auto pTarget = action.getHitChara();

    if( !pSource->isPlayer() )
      return;

    pSource->replaceSingleStatusEffectById( Blood_For_Blood );
    action.getActionResultBuilder()->applyStatusEffectSelf( Blood_For_Blood, 20000, 0 );
  }
};

EXPOSE_SCRIPT( ActionBloodForBlood );
