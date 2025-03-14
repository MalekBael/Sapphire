#include <Action/Action.h>
#include <Actor/Chara.h>
#include <Script/NativeScriptApi.h>
#include <ScriptObject.h>
#include <memory>

using namespace Sapphire;
using namespace Sapphire::World::Action;
using namespace Sapphire::Entity;

class ActionElusiveJump : public Sapphire::ScriptAPI::ActionScript
{
public:
  ActionElusiveJump() : Sapphire::ScriptAPI::ActionScript( 94 )
  {
  }

  void onExecute( Sapphire::World::Action::Action& action ) override
  {
    // Placeholder implementation
  }
};

EXPOSE_SCRIPT( ActionElusiveJump);