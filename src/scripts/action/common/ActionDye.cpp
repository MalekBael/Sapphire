#include <ScriptObject.h>
#include <Actor/Player.h>
#include <Action/Action.h>
#include <Inventory/Item.h>

using namespace Sapphire;

class ActionDye : public Sapphire::ScriptAPI::ActionScript
{
public:
  ActionDye() : Sapphire::ScriptAPI::ActionScript( Dye )
  {
  }

  void onExecute( Sapphire::World::Action::Action& action ) override
  {
    auto sourceChara = action.getSourceChara();

    if( !sourceChara->isPlayer() )
      return;

    //TODO: Effect
    sourceChara->getAsPlayer()->dyeItemFromDyeingInfo();
  }
};

EXPOSE_SCRIPT( ActionDye );