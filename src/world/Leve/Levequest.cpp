#include "Levequest.h"
#include "Actor/Player.h"
#include "Logging/Logger.h"

using namespace Sapphire;

namespace Sapphire::World
{
  Levequest::Levequest( uint32_t leveId, Entity::PlayerPtr pPlayer ) : m_leveId( leveId ),
                                                                       m_pPlayer( pPlayer ),
                                                                       m_state( LeveState::Inactive ),
                                                                       m_objectiveProgress( 0 ),
                                                                       m_timeLimit( 1800 ),// Default to 30 minutes
                                                                       m_startTime( 0 ),
                                                                       m_isLinkedToQuest( false ),
                                                                       m_linkedQuestId( 0 ),
                                                                       m_linkedQuestStep( 0 )
  {
  }

  uint32_t Levequest::getId() const
  {
    return m_leveId;
  }

  Entity::PlayerPtr Levequest::getPlayer() const
  {
    return m_pPlayer;
  }

  Levequest::LeveState Levequest::getState() const
  {
    return m_state;
  }

  void Levequest::setState( LeveState state )
  {
    m_state = state;
  }

  uint8_t Levequest::getObjectiveProgress() const
  {
    return m_objectiveProgress;
  }

  void Levequest::setObjectiveProgress( uint8_t progress )
  {
    m_objectiveProgress = progress;
  }

  void Levequest::incrementObjectiveProgress()
  {
    m_objectiveProgress++;
  }

  uint32_t Levequest::getTimeRemaining() const
  {
    // Implement time calculation based on start time and current time
    return m_timeLimit;
  }

  uint32_t Levequest::getTimeLimit() const
  {
    return m_timeLimit;
  }

  void Levequest::linkToQuest( uint32_t questId, uint8_t stepToUpdate )
  {
    m_linkedQuestId = questId;
    m_linkedQuestStep = stepToUpdate;
    m_isLinkedToQuest = true;
  }

  bool Levequest::isLinkedToQuest() const
  {
    return m_isLinkedToQuest;
  }

  uint32_t Levequest::getLinkedQuestId() const
  {
    return m_linkedQuestId;
  }

  uint8_t Levequest::getLinkedQuestStep() const
  {
    return m_linkedQuestStep;
  }
}// namespace Sapphire::World
