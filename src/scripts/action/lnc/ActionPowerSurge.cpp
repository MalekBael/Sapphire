#include <Action/Action.h>
#include <Actor/Chara.h>
#include <Script/NativeScriptApi.h>
#include <ScriptObject.h>
#include <memory>

using namespace Sapphire;
using namespace Sapphire::World::Action;
using namespace Sapphire::Entity;

class ActionPowerSurge : public Sapphire::ScriptAPI::ActionScript
{
public:
  ActionPowerSurge() : Sapphire::ScriptAPI::ActionScript( 93 )
  {
  }

  void onExecute( Sapphire::World::Action::Action& action ) override
  {
    // Placeholder implementation
  }
};

EXPOSE_SCRIPT( ActionPowerSurge );