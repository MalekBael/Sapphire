#include <ScriptObject.h>
#include <Actor/Player.h>
#include <Action/Action.h>

class ActionFightOrFlight : public Sapphire::ScriptAPI::ActionScript
{
public:
  ActionFightOrFlight() : Sapphire::ScriptAPI::ActionScript( FightOrFlight )
  {
  }

  static constexpr auto Duration = 30;
  static constexpr uint32_t Flags = static_cast< uint32_t >( Common::StatusEffectFlag::BuffCategory );

  void onExecute( Sapphire::World::Action::Action& action ) override
  {
    auto pSource = action.getSourceChara();
    auto pActionBuilder = action.getActionResultBuilder();

    pActionBuilder->applyStatusEffect( pSource, FightOrFlightStatus, ( Duration * 1000 ), 0, {}, Flags, false, true );
  }
};

EXPOSE_SCRIPT( ActionFightOrFlight );