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

class StatusEffectPhlebotomize : public ScriptAPI::StatusEffectScript
{
public:
  StatusEffectPhlebotomize() : ScriptAPI::StatusEffectScript( 119 ) {}

  static constexpr uint32_t Potency = 30;

  void onApply( Entity::Chara& actor ) override
  {
    spdlog::debug( "Phlebotomize applied to target ID {}", actor.getId() );
  }

  void onTick( Entity::Chara& actor ) override
  {
    if( !actor.isAlive() )
      return;

    auto statusEffect = actor.getStatusEffectById( 119 );
    if( !statusEffect )
      return;

    auto pSource = statusEffect->getSrcActor();
    auto pTarget = &actor;

    if( !pSource->isPlayer() )
      return;

    Action action( pSource, 119, 0 );// Action ID and request ID can be adjusted as needed

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

EXPOSE_SCRIPT( StatusEffectPhlebotomize );