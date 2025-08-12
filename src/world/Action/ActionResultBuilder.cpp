#include "ActionResultBuilder.h"
#include "ActionResult.h"

#include <Actor/Player.h>

#include <Network/PacketWrappers/EffectPacket.h>
#include <Network/PacketWrappers/EffectPacket1.h>

#include <Territory/Territory.h>

#include <Util/Util.h>
#include <Util/UtilMath.h>
#include <Exd/ExdData.h>

#include <Logging/Logger.h>
#include <Manager/TerritoryMgr.h>
#include <Manager/MgrUtil.h>
#include <Service.h>

#include <Manager/TaskMgr.h>
#include <Task/ActionIntegrityTask.h>

using namespace Sapphire;
using namespace Sapphire::World::Action;
using namespace Sapphire::World::Manager;
using namespace Sapphire::Network::Packets;
using namespace Sapphire::Network::Packets::WorldPackets::Server;

ActionResultBuilder::ActionResultBuilder( Entity::CharaPtr source, uint32_t actionId, float aggroModifier, uint32_t resultId, uint16_t requestId ) :
  m_sourceChara( std::move( source ) ),
  m_actionId( actionId ),
  m_aggroModifier( aggroModifier ),
  m_resultId( resultId ),
  m_requestId( requestId )
{

}

void ActionResultBuilder::addResultToActor( Entity::CharaPtr& chara, ActionResultPtr result )
{
  auto it = m_actorResultsMap.find( chara );
  if( it == m_actorResultsMap.end() )
  {
    m_actorResultsMap[ chara ].push_back( std::move( result ) );
    return;
  }

  it->second.push_back( std::move( result ) );
}

void ActionResultBuilder::heal( Entity::CharaPtr& effectTarget, Entity::CharaPtr& healingTarget, uint32_t amount, Common::CalcResultType hitType, Common::ActionResultFlag flag )
{
  float aggro = m_aggroModifier;
  if( !m_applyHealAggro )
    aggro = 0;

  m_applyStatusAggro = false; // Status effects only apply aggro if there is no damage or heal from the action itself
  ActionResultPtr nextResult = make_ActionResult( m_sourceChara, healingTarget );
  auto& exdData = Common::Service< Data::ExdData >::ref();
  auto actionData = exdData.getRow< Excel::Action >( m_actionId );
  nextResult->heal( amount, hitType, std::abs( actionData->data().AttackType ), flag, aggro );
  addResultToActor( effectTarget, nextResult );
}

void ActionResultBuilder::restoreMP( Entity::CharaPtr& target, Entity::CharaPtr& restoringTarget, uint32_t amount, Common::ActionResultFlag flag )
{
  ActionResultPtr nextResult = make_ActionResult( m_sourceChara, restoringTarget );// restore mp source actor
  nextResult->restoreMP( amount, flag );
  addResultToActor( target, nextResult );
}

void ActionResultBuilder::damage( Entity::CharaPtr& effectTarget, Entity::CharaPtr& damagingTarget, uint32_t amount, Common::CalcResultType hitType, Common::ActionResultFlag flag )
{
  m_applyHealAggro = false; // Heal as a secondary effect does not apply aggro
  m_applyStatusAggro = false; // Status effects only apply aggro if there is no damage or heal from the action itself
  ActionResultPtr nextResult = make_ActionResult( m_sourceChara, damagingTarget );
  auto& exdData = Common::Service< Data::ExdData >::ref();
  auto actionData = exdData.getRow< Excel::Action >( m_actionId );
  nextResult->damage( amount, hitType, std::abs( actionData->data().AttackType ), flag, m_aggroModifier );
  addResultToActor( damagingTarget, nextResult );
}

void ActionResultBuilder::startCombo( Entity::CharaPtr& target, uint16_t actionId )
{
  ActionResultPtr nextResult = make_ActionResult( m_sourceChara, target );
  nextResult->startCombo( actionId );
  addResultToActor( target, nextResult );
}

void ActionResultBuilder::comboSucceed( Entity::CharaPtr& target )
{
  ActionResultPtr nextResult = make_ActionResult( m_sourceChara, target );
  nextResult->comboSucceed();
  addResultToActor( target, nextResult );
}

void ActionResultBuilder::applyStatusEffect( Entity::CharaPtr& target, uint16_t statusId, uint32_t duration, uint8_t param, bool shouldOverride )
{
  ActionResultPtr nextResult = make_ActionResult( m_sourceChara, target );
  nextResult->applyStatusEffect( statusId, duration, *m_sourceChara, param, m_applyStatusAggro, shouldOverride );
  addResultToActor( target, nextResult );
}

void ActionResultBuilder::applyStatusEffect( Entity::CharaPtr& target, uint16_t statusId, uint32_t duration, uint8_t param,
                                             const std::vector< World::Action::StatusModifier >& modifiers, uint32_t flag, bool statusToSource, bool shouldOverride )
{
  ActionResultPtr nextResult = make_ActionResult( m_sourceChara, target );
  nextResult->applyStatusEffect( statusId, duration, *m_sourceChara, param, modifiers, flag, statusToSource, m_applyStatusAggro, shouldOverride );
  addResultToActor( target, nextResult );
}

void ActionResultBuilder::applyStatusEffectSelf( uint16_t statusId, uint32_t duration, uint8_t param, bool shouldOverride )
{
  ActionResultPtr nextResult = make_ActionResult( m_sourceChara, m_sourceChara );
  nextResult->applyStatusEffectSelf( statusId, duration, param, m_applyStatusAggro, shouldOverride );
  addResultToActor( m_sourceChara, nextResult );
}

void ActionResultBuilder::applyStatusEffectSelf( uint16_t statusId, uint32_t duration, uint8_t param, const std::vector< World::Action::StatusModifier >& modifiers,
                                                 uint32_t flag, bool shouldOverride )
{
  ActionResultPtr nextResult = make_ActionResult( m_sourceChara, m_sourceChara );
  nextResult->applyStatusEffectSelf( statusId, duration, param, modifiers, flag, m_applyStatusAggro, shouldOverride );
  addResultToActor( m_sourceChara, nextResult );
}

void ActionResultBuilder::replaceStatusEffect( Sapphire::StatusEffect::StatusEffectPtr& pOldStatus, Entity::CharaPtr& target, uint16_t statusId, uint32_t duration, uint8_t param,
                                               const std::vector< World::Action::StatusModifier >& modifiers, uint32_t flag, bool statusToSource )
{
  ActionResultPtr nextResult = make_ActionResult( m_sourceChara, target );
  nextResult->replaceStatusEffect( pOldStatus, statusId, duration, *m_sourceChara, param, modifiers, flag, statusToSource, m_applyStatusAggro );
  addResultToActor( target, nextResult );
}

void ActionResultBuilder::replaceStatusEffectSelf( Sapphire::StatusEffect::StatusEffectPtr& pOldStatus, uint16_t statusId, uint32_t duration, uint8_t param,
                                                   const std::vector< World::Action::StatusModifier >& modifiers, uint32_t flag )
{
  ActionResultPtr nextResult = make_ActionResult( m_sourceChara, m_sourceChara );
  nextResult->replaceStatusEffectSelf( pOldStatus, statusId, duration, param, modifiers, flag, m_applyStatusAggro );
  addResultToActor( m_sourceChara, nextResult );
}

void ActionResultBuilder::mount( Entity::CharaPtr& target, uint16_t mountId )
{
  ActionResultPtr nextResult = make_ActionResult( m_sourceChara, target );
  nextResult->mount( mountId );
  addResultToActor( target, nextResult );
}

void ActionResultBuilder::sendActionResults( const std::vector< Entity::CharaPtr >& targetList )
{
  Logger::debug( "EffectBuilder result: " );
  Logger::debug( "Targets afflicted: {}", targetList.size() );

  do // we want to send at least one packet even nothing is hit so other players can see
  {
    auto packet = createActionResultPacket( targetList );
    server().queueForPlayers( m_sourceChara->getInRangePlayerIds( true ), packet );
  }
  while( !m_actorResultsMap.empty() );
}

std::shared_ptr< FFXIVPacketBase > ActionResultBuilder::createActionResultPacket( const std::vector< Entity::CharaPtr >& targetList )
{
  auto targetCount = targetList.size();
  auto& taskMgr = Common::Service< World::Manager::TaskMgr >::ref();
  auto& teriMgr = Common::Service< Sapphire::World::Manager::TerritoryMgr >::ref();
  auto zone = teriMgr.getTerritoryByGuId( m_sourceChara->getTerritoryId() );

  // need to get actionData
  auto& exdData = Common::Service< Data::ExdData >::ref();

  auto actionData = exdData.getRow< Excel::Action >( m_actionId );
  if( !actionData )
    throw std::runtime_error( "No actiondata found!" );

  if( targetCount > 1 || actionData->data().EffectType != static_cast< uint8_t >( Common::CastType::SingleTarget ) ) // use AoeEffect packets
  {
    // todo: maintarget logic is lackluster. There needs to be some criteria for when a main target is required.
    uint32_t mainTarget = Common::INVALID_GAME_OBJECT_ID;
    if( !targetList.empty() )
      mainTarget = targetList[ 0 ]->getId();
    auto actionResult = makeEffectPacket( m_sourceChara->getId(), mainTarget, m_actionId );
    actionResult->setRotation( Common::Util::floatToUInt16Rot( m_sourceChara->getRot() ) );
    actionResult->setRequestId( m_requestId );
    actionResult->setResultId( m_resultId );
    actionResult->setTargetPosition( m_sourceChara->getPos() );


    uint8_t targetIndex = 0;
    for( auto& [ actor, actorResultList ] : m_actorResultsMap )
    {

      if( actor )
        taskMgr.queueTask( World::makeActionIntegrityTask( m_resultId, actor, actorResultList, 300 ) );

      for( auto& result : actorResultList )
      {
        auto effect = result->getCalcResultParam();
        if( result->getTarget() == m_sourceChara )
          actionResult->addSourceEffect( effect );
        else
          actionResult->addTargetEffect( effect, result->getTarget()->getId() );
      }
      targetIndex++;

      if( targetIndex == 15 )
        break;
    }

    m_actorResultsMap.clear();
    return actionResult;
  }
  else  // use Effect for single target
  {
    uint32_t mainTargetId = targetList.empty() ? m_sourceChara->getId() : targetList[ 0 ]->getId();
    auto actionResult = makeEffectPacket1( m_sourceChara->getId(), mainTargetId, m_actionId );
    actionResult->setRotation( Common::Util::floatToUInt16Rot( m_sourceChara->getRot() ) );
    actionResult->setRequestId( m_requestId );
    actionResult->setResultId( m_resultId );

    for( auto& [ actor, actorResultList ] : m_actorResultsMap )
    {
      if( actor )
        taskMgr.queueTask( World::makeActionIntegrityTask( m_resultId, actor, actorResultList, 300 ) );

      for( auto& result : actorResultList )
      {
        auto effect = result->getCalcResultParam();
        if( result->getTarget() == m_sourceChara &&
            result->getCalcResultParam().Type != Common::CalcResultType::TypeSetStatusMe )
          actionResult->addSourceEffect( effect );
        else
          actionResult->addTargetEffect( effect );
      }
    }

    m_actorResultsMap.clear();
    return actionResult;
  }
}