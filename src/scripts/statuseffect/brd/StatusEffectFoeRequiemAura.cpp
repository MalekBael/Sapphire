#include <Action/Action.h>
#include <Action/CommonAction.h>
#include <Actor/BNpc.h>
#include <Actor/Player.h>
#include <Script/NativeScriptApi.h>
#include <ScriptObject.h>
#include <StatusEffect/StatusEffect.h>

#include <Network/Util/PacketUtil.h>

#include "Manager/PartyMgr.h"
#include "Manager/StatusEffectMgr.h"

#include <Math/CalcStats.h>
#include "Util/UtilMath.h"
#include <algorithm>

using namespace Sapphire;
using namespace Sapphire::World::Action;

class StatusEffectFoeRequiemAura : public Sapphire::ScriptAPI::StatusEffectScript
{
public:
  StatusEffectFoeRequiemAura() : Sapphire::ScriptAPI::StatusEffectScript( FoeRequiemAura )
  {
  }

  static constexpr auto DrainPotency = 60;
  static constexpr int32_t ModifierValue = -10;
  static constexpr uint32_t Flags = static_cast< uint32_t >( Common::StatusEffectFlag::DebuffCategory ) |
                                    static_cast< uint32_t >( Common::StatusEffectFlag::Permanent );

  void onApply( Entity::Chara& actor )
  {
    actor.removeSingleStatusEffectById( ArmysPaeonAura );
    actor.removeSingleStatusEffectById( MagesBalladAura );
  }

  void onTick( Entity::Chara& actor, Sapphire::StatusEffect::StatusEffect& effect ) override
  {
    uint32_t mp = actor.getMp();
    uint32_t mpDrained = Math::CalcStats::calcMpRefresh( DrainPotency, actor.getLevel() );
    if( mp <= mpDrained )
    {
      actor.setMp( 0 );
      actor.removeSingleStatusEffectById( ArmysPaeonAura );
      // We refresh the buff on enemies one last time
    }
    else
    {
      actor.setMp( mp - mpDrained );
    }

    int32_t modifierVal = ModifierValue;
    if( actor.hasStatusEffect( BattleVoiceStatus ) )
    {
      modifierVal *= 2;
    }

    for( auto& target : actor.getInRangeActors( false ) )
    {
      if( actor.isHostile( *target->getAsChara() ) )
      {
        auto distance = Sapphire::Common::Util::distance( actor.getPos(), target->getPos() );
        if( distance <= 20.0f )
        {
          auto targetChara = target->getAsChara();
          if( targetChara->hasStatusEffect( FoeRequiemStatus ) )
          {
            auto effect = targetChara->getStatusEffectById( FoeRequiemStatus );
            effect->setModifier( Common::ParamModifier::MagicDefensePercent, modifierVal );
            effect->refresh( 5000 );
          }
          else
          {
            statusEffectMgr().applyStatusEffect( actor.getAsChara(), target->getAsChara(), FoeRequiemStatus, 5000, 0, { StatusModifier{ Common::ParamModifier::MagicDefensePercent, modifierVal } }, Flags, true );
          }
        }
      }
    }


  }
};

EXPOSE_SCRIPT( StatusEffectFoeRequiemAura );