#include <Action/Action.h>
#include <Action/CommonAction.h>
#include <Actor/BNpc.h>
#include <Actor/Player.h>
#include <Script/NativeScriptApi.h>
#include <ScriptObject.h>
#include <StatusEffect/StatusEffect.h>

#include <Network/Util/PacketUtil.h>

#include "Manager/PartyMgr.h"

#include <Math/CalcStats.h>
#include "Util/UtilMath.h"
#include <algorithm>

using namespace Sapphire;
using namespace Sapphire::World::Action;

class StatusEffectMagesBalladAura : public Sapphire::ScriptAPI::StatusEffectScript
{
public:
  StatusEffectMagesBalladAura() : Sapphire::ScriptAPI::StatusEffectScript( MagesBalladAura )
  {
  }

  static constexpr auto DrainPotency = 60;
  static constexpr auto PotencySelf = 20;
  static constexpr auto PotencyParty = 30;
  static constexpr uint32_t Flags = static_cast< uint32_t >( Common::StatusEffectFlag::BuffCategory ) |
                                    static_cast< uint32_t >( Common::StatusEffectFlag::Permanent );

  void onApply( Entity::Chara& actor )
  {
    actor.removeSingleStatusEffectById( MagesBalladStatus );
    actor.removeSingleStatusEffectById( ArmysPaeonAura );
    actor.removeSingleStatusEffectById( FoeRequiemAura );
  }

  void onTick( Entity::Chara& actor, Sapphire::StatusEffect::StatusEffect& effect ) override
  {
    auto potencyParty = PotencyParty;

    if( actor.hasStatusEffect( BattleVoiceStatus ) )
    {
      potencyParty *= 2;
    }

    uint32_t mp = actor.getMp() + Math::CalcStats::calcMpRefresh( PotencySelf, actor.getLevel() );
    uint32_t mpDrained = Math::CalcStats::calcMpRefresh( DrainPotency, actor.getLevel() );
    if( mp <= mpDrained )
    {
      actor.setMp( 0 );
      actor.removeSingleStatusEffectById( MagesBalladAura );
      // We refresh the buff on other players one last time
    }
    else
    {
      actor.setMp( mp - mpDrained );
    }

    auto pPlayer = actor.getAsPlayer();
    if( !pPlayer )
      return;

    if( pPlayer->getPartyId() == 0 )
      return;

    auto& partyMgr = Common::Service< World::Manager::PartyMgr >::ref();
    auto pParty = partyMgr.getParty( pPlayer->getPartyId() );
    auto membersIds = pParty->MemberId;
    for( auto& target : actor.getInRangeActors( false ) )
    {
      if( std::find( membersIds.begin(), membersIds.end(), target->getId() ) != membersIds.end() )
      {
        auto distance = Sapphire::Common::Util::distance( actor.getPos(), target->getPos() );
        if( distance <= 20.0f )
        {
          auto targetChara = target->getAsChara();
          if( targetChara->hasStatusEffect( MagesBalladAura ) )
          {
            continue;
          }
          else if( targetChara->hasStatusEffect( MagesBalladStatus ) )
          {
            auto effect = targetChara->getStatusEffectById( MagesBalladStatus );
            effect->setModifier( Common::ParamModifier::Refresh, potencyParty );
            effect->refresh( 5000 );
          }
          else
          {
            statusEffectMgr().applyStatusEffect( actor.getAsChara(), targetChara->getAsChara(), MagesBalladStatus, 5000, 0, { StatusModifier{ Common::ParamModifier::Refresh, potencyParty } }, Flags, true );
          }
        }
      }
    }


  }
};

EXPOSE_SCRIPT( StatusEffectMagesBalladAura );