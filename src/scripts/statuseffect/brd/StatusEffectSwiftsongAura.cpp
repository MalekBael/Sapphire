#include <Script/NativeScriptApi.h>
#include <ScriptObject.h>
#include <Actor/Player.h>
#include <Actor/BNpc.h>
#include <Action/CommonAction.h>
#include <Action/Action.h>
#include <StatusEffect/StatusEffect.h>

#include <Network/Util/PacketUtil.h>

#include "Manager/PartyMgr.h"

#include "Util/UtilMath.h"
#include <algorithm>


using namespace Sapphire;
using namespace Sapphire::World::Action;

class StatusEffectSwiftsongAura : public Sapphire::ScriptAPI::StatusEffectScript
{
public:
  StatusEffectSwiftsongAura() : Sapphire::ScriptAPI::StatusEffectScript( SwiftsongAura )
  {
  }

  static constexpr uint32_t Flags = static_cast< uint32_t >( Common::StatusEffectFlag::BuffCategory ) |
                                    static_cast< uint32_t >( Common::StatusEffectFlag::Permanent );

  void onTick( Entity::Chara& actor, Sapphire::StatusEffect::StatusEffect& effect ) override
  {
    if( !actor.isAlive() )
    {
      actor.removeSingleStatusEffectById( SwiftsongAura );
      return;
    }

    auto pPlayer = actor.getAsPlayer();
    if( !pPlayer )
      return;

    if( pPlayer->isInCombat() )
    {
      pPlayer->removeSingleStatusEffectById( SwiftsongAura );
      return;
    }

    if( pPlayer->getPartyId() == 0 )
      return;

    auto& partyMgr = Common::Service< World::Manager::PartyMgr >::ref();
    auto pParty = partyMgr.getParty( pPlayer->getPartyId() );
    auto membersIds = pParty->MemberId;
    for( auto& target : actor.getInRangeActors( false ) )
    {
      if( std::find( membersIds.begin(), membersIds.end(), target->getId() ) != membersIds.end() )
      {
        if ( ( target->isPlayer() && target->getAsPlayer()->isInCombat() ) ||
          ( target->isBattleNpc() && target->getAsBNpc()->getState() == Sapphire::Entity::BNpcState::Combat ) )
        {
          continue;
        }


        auto distance = Sapphire::Common::Util::distance( actor.getPos(), target->getPos() );
        if( distance <= 20.0f )
        {
          auto targetChara = target->getAsChara();
          if( targetChara->hasStatusEffect( SwiftsongStatus ) )
          {
            auto effect = targetChara->getStatusEffectById( SwiftsongStatus );
            effect->refresh( 5000 );
          }
          else
          {
            statusEffectMgr().applyStatusEffect( actor.getAsChara(), targetChara->getAsChara(), SwiftsongStatus, 5000, 20, { }, Flags, true );
          }
        }
      }
    }


    
  }
};

EXPOSE_SCRIPT( StatusEffectSwiftsongAura );