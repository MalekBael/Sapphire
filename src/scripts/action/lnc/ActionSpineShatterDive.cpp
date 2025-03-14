#include <Action/Action.h>
#include <Actor/Chara.h>
#include <Script/NativeScriptApi.h>
#include <ScriptObject.h>
#include <memory>

using namespace Sapphire;
using namespace Sapphire::World::Action;
using namespace Sapphire::Entity;

class ActionSpineShatterDive : public Sapphire::ScriptAPI::ActionScript
{
public:
  ActionSpineShatterDive() : Sapphire::ScriptAPI::ActionScript( 95 )
  {
  }

  void onExecute( Sapphire::World::Action::Action& action ) override
  {
    // Placeholder implementation
  }
};

EXPOSE_SCRIPT( ActionSpineShatterDive );