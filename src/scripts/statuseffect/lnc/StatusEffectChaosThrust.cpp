#include <Action/Action.h>
#include <Actor/Chara.h>
#include <Network/Util/PacketUtil.h>
#include <Script/NativeScriptApi.h>
#include <ScriptObject.h>
#include <StatusEffect/StatusEffect.h>
#include <spdlog/spdlog.h>

using namespace Sapphire;
using namespace Sapphire::Common;
using namespace Sapphire::World::Action;
using namespace Sapphire::Entity;


class StatusEffectChaosThrust : public ScriptAPI::StatusEffectScript
{
public:
  StatusEffectChaosThrust() : ScriptAPI::StatusEffectScript( 118 ) {}

  static constexpr uint32_t Potency = 35;

  void onApply( Entity::Chara& actor ) override
  {
    spdlog::debug( "Chaos Thrust applied to target ID {}", actor.getId() );
  }

  void onTick( Entity::Chara& actor ) override
  {
    if( !actor.isAlive() )
      return;

    auto statusEffect = actor.getStatusEffectById( 118 );
    if( !statusEffect )
      return;

    auto pSource = statusEffect->getSrcActor();
    if( !pSource )
      return;

    Action action( pSource, 118, 0 );

    auto dmgPair = action.calcDamage( Potency );

    uint32_t damage = dmgPair.first;
    CalcResultType hitType = dmgPair.second;

    actor.takeDamage( damage );

    Network::Util::Packet::sendHudParam( actor );

    if( damage > 0 )
    {
      actor.onActionHostile( pSource );
    }
  }

};

EXPOSE_SCRIPT( StatusEffectChaosThrust );