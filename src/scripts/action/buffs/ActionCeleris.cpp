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

class ActionCeleris : public Sapphire::ScriptAPI::ActionScript
{
public:
  ActionCeleris() : Sapphire::ScriptAPI::ActionScript( 404 )
  {
  }

  static constexpr uint32_t Haste = 651;

  void onExecute( Sapphire::World::Action::Action& action ) override
  {
    auto pSource = action.getSourceChara();
    auto pTarget = action.getHitChara();

    if( !pSource->isPlayer() )
      return;

    pSource->replaceSingleStatusEffectById( Haste );
    action.getActionResultBuilder()->applyStatusEffectSelf( Haste, 6000, 0 );
  }
};

EXPOSE_SCRIPT( ActionCeleris );
