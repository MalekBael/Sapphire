#ifndef _LEVEQUEST_H_
#define _LEVEQUEST_H_

#include "ForwardsZone.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Sapphire::World
{
  class Levequest
  {
  public:
    Levequest( uint32_t leveId, Entity::PlayerPtr pPlayer );
    ~Levequest() = default;

    uint32_t getId() const;
    Entity::PlayerPtr getPlayer() const;

    enum LeveState : uint8_t
    {
      Inactive = 0,
      Active = 1,
      Complete = 2,
      Failed = 3
    };

    LeveState getState() const;
    void setState( LeveState state );

    uint8_t getObjectiveProgress() const;
    void setObjectiveProgress( uint8_t progress );
    void incrementObjectiveProgress();

    uint32_t getTimeRemaining() const;
    uint32_t getTimeLimit() const;

    // Add this method to the public interface
  
    /**
     * @brief Connect a levequest to a quest
     * @param questId The quest ID to update when this leve is accepted
     * @param stepToUpdate The quest step to set when accepted
     */
    void linkToQuest( uint32_t questId, uint8_t stepToUpdate );
  
    /**
     * @brief Check if this levequest is linked to a quest
     */
    bool isLinkedToQuest() const;
  
    /**
     * @brief Get the linked quest ID
     */
    uint32_t getLinkedQuestId() const;
  
    /**
     * @brief Get the step to update when accepted
     */
    uint8_t getLinkedQuestStep() const;

    // Add more methods as needed

  private:
    uint32_t m_leveId;
    Entity::PlayerPtr m_pPlayer;
    LeveState m_state;
    uint8_t m_objectiveProgress;
    uint32_t m_timeLimit;
    uint32_t m_startTime;

    // Add these members to the private section
    uint32_t m_linkedQuestId = 0;
    uint8_t m_linkedQuestStep = 0;
    bool m_isLinkedToQuest = false;
  };
}// namespace Sapphire::World

#endif// _LEVEQUEST_H_
