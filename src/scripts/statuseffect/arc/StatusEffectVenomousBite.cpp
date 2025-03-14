// In src/scripts/statuseffect/arc/StatusEffectVenomousBite.cpp

#include <Action/Action.h>
#include <Actor/Chara.h>
#include <Network/Util/PacketUtil.h>
#include <Script/NativeScriptApi.h>
#include <ScriptObject.h>
#include <StatusEffect/StatusEffect.h>
#include <spdlog/spdlog.h>

// Include necessary namespaces
using namespace Sapphire;
using namespace Sapphire::Common;// For Common enums and types
using namespace Sapphire::World::Action;
using namespace Sapphire::Entity;

// StatusEffect ID 124 corresponds to Venomous Bite
class StatusEffectVenomousBite : public ScriptAPI::StatusEffectScript
{
public:
  StatusEffectVenomousBite() : ScriptAPI::StatusEffectScript( 124 ) {}

  static constexpr uint32_t Potency = 40;

  // Called when the status effect is applied
  void onApply( Entity::Chara& actor ) override
  {
    spdlog::debug( "Venomous Bite applied to target ID {}", actor.getId() );
    // No additional actions needed on apply
  }

  // Called every tick (e.g., every 3 seconds)
  void onTick( Entity::Chara& actor ) override
  {
    // Ensure the actor is alive
    if( !actor.isAlive() )
      return;

    // Get the status effect instance
    auto statusEffect = actor.getStatusEffectById( 124 );
    if( !statusEffect )
      return;

    // Get the source actor who applied the status effect
    auto pSource = statusEffect->getSrcActor();
    if( !pSource )
      return;

    spdlog::debug( "Venomous Bite tick for target ID {}", actor.getId() );

    // Create an Action object to calculate damage
    Action action( pSource, 124, 0 );// Action ID and request ID can be adjusted as needed

    // Calculate damage using the action's calcDamage method
    auto dmgPair = action.calcDamage( Potency );

    // dmgPair.first is the damage amount
    uint32_t damage = dmgPair.first;
    CalcResultType hitType = dmgPair.second;

    // Apply damage to the actor
    actor.takeDamage( damage );

    // Update the HUD to reflect the damage
    Network::Util::Packet::sendHudParam( actor );

    // Establish aggro if damage was dealt
    if( damage > 0 )
    {
      actor.onActionHostile( pSource );
    }
  }

  // Called when the status effect is removed or expires
  void onExpire( Entity::Chara& actor ) override
  {
    spdlog::debug( "Venomous Bite expired on target ID {}", actor.getId() );
    // No additional actions needed on expire
  }
};

EXPOSE_SCRIPT( StatusEffectVenomousBite );