#include "StateCombat.h"
#include "Actor/BNpc.h"
#include "Logging/Logger.h"
#include <Service.h>

#include <Manager/TerritoryMgr.h>
#include <Territory/Territory.h>
#include <Navi/NaviProvider.h>

using namespace Sapphire::World;

void AI::Fsm::StateCombat::onUpdate( Entity::BNpc& bnpc, uint64_t tickCount )
{

  auto& teriMgr = Common::Service< World::Manager::TerritoryMgr >::ref();
  auto pZone = teriMgr.getTerritoryByGuId( bnpc.getTerritoryId() );
  if( !pZone )
  {
    Logger::debug( "Territory not found for BNPC" );
    return;
  }
  auto pNaviProvider = pZone->getNaviProvider();
  bool hasQueuedAction = bnpc.hasAction();

  if( hasQueuedAction )
    return;

  auto pHatedActor = bnpc.hateListGetHighest();
  if( !pHatedActor )
    return;

  if( pNaviProvider && bnpc.pathingActive() )
  {
    auto state = bnpc.getState();
    bool isRunning = state == Entity::BNpcState::Retreat || state == Entity::BNpcState::Combat;
    pNaviProvider->updateAgentParameters( bnpc.getAgentId(), bnpc.getRadius(), isRunning, isRunning ? bnpc.getRunSpeed() : bnpc.getWalkSpeed() );
  }

  auto distanceOrig = Common::Util::distance( bnpc.getPos(), bnpc.getSpawnPos() );

  if( !pHatedActor->isAlive() || bnpc.getTerritoryId() != pHatedActor->getTerritoryId() )
  {
    bnpc.deaggro( pHatedActor );
    pHatedActor = bnpc.hateListGetHighest();
  }

  if( !pHatedActor )
    return;

  auto distance = Common::Util::distance( bnpc.getPos(), pHatedActor->getPos() );

  // All possibilities to automatically lose aggro go here
  // todo: use status flags here instead of bnpc flags?
  if( !bnpc.hasFlag( Entity::NoDeaggro ) )
  {
    if( bnpc.hasFlag( Entity::Immobile ) && distance > 40.0f )
    {
      bnpc.deaggro( pHatedActor );
    }
    else if( distance > 80.0f )
    {
      bnpc.deaggro( pHatedActor );
    }
  }

  auto dtMove = tickCount - m_lastMoveTime;
  auto dtRot = tickCount - m_lastRotTime;

  if( dtRot == tickCount )
    dtRot = 300;

  if( bnpc.pathingActive() && !hasQueuedAction &&
      !bnpc.hasFlag( Entity::Immobile ) && dtMove >= 1500 &&
      distance > ( bnpc.getNaviTargetReachedDistance() + pHatedActor->getRadius() ) )
  {

    if( pNaviProvider )
      pNaviProvider->setMoveTarget( bnpc.getAgentId(), pHatedActor->getPos() );

    bnpc.moveTo( *pHatedActor );
    m_lastMoveTime = tickCount;
  }
  else
  {
    // todo: turn speed per bnpc since retail doesn't have spin2win on newer mobs
    if( !bnpc.hasFlag( Entity::TurningDisabled ) && !bnpc.isFacingTarget( *pHatedActor, 1.0f ) && dtRot >= 300 )
    {
      float oldRot = bnpc.getRot();
      float rot = Common::Util::calcAngFrom( bnpc.getPos().x, bnpc.getPos().z, pHatedActor->getPos().x, pHatedActor->getPos().z );

      // Convert to facing direction
      float newRot = rot + ( PI / 2 );

      // Normalize to [-π, π] range
      newRot = -fmod( newRot + PI, 2 * PI ) - PI;

      volatile auto dRot = newRot - oldRot;
      dRot = dRot * ( dtRot / 300.f );

      bnpc.setRot( oldRot + dRot );

      //bnpc.face( pHatedActor->getPos() );
      bnpc.sendPositionUpdate( tickCount );
      m_lastRotTime = tickCount;
    }
  }

  if( bnpc.getAgentId() != -1 )
  {
    auto pos = pNaviProvider->getAgentPos( bnpc.getAgentId() );
    auto myPos = bnpc.getPos();
    if( ( pos.x != myPos.x || pos.y != myPos.y || pos.z != myPos.z ) )
    {
      bnpc.setPos( pos );
    }
  }

  // todo: there are mobs that ignore aggro and continue their path
  //       such as Labyrinth of The Ancients adds in Thanatos boss fight, account for those
  if( !hasQueuedAction && distance <= ( bnpc.getNaviTargetReachedDistance() + pHatedActor->getRadius() + 3.0f ) )
  {
    bnpc.processGambits( tickCount );

    // in combat range. ATTACK!
    if( !bnpc.hasFlag( Entity::BNpcFlag::AutoAttackDisabled ) && bnpc.isFacingTarget( *pHatedActor, 1.0f ) )
      bnpc.autoAttack( pHatedActor );

    if( bnpc.getNaviIsPathing() )
    {
      pNaviProvider->resetMoveTarget( bnpc.getAgentId() );
      bnpc.setNaviIsPathing( false );
    }
  }
  m_lastTick = tickCount;
}

void AI::Fsm::StateCombat::onEnter( Entity::BNpc& bnpc )
{
}

void AI::Fsm::StateCombat::onExit( Entity::BNpc& bnpc )
{
  bnpc.hateListClear();
  bnpc.changeTarget( Common::INVALID_GAME_OBJECT_ID64 );
  bnpc.setStance( Common::Stance::Passive );
  bnpc.setOwner( nullptr );
}

