/**
 * @file CmnDefRetainerCall.cpp
 * @brief Server-side implementation of the Retainer Call event (summoning a retainer)
 * 
 * Event ID: 720906 (0x000B000A)
 * Client Script: cmndefretainercall_00010.luab
 * 
 * This handles the summoning bell interaction where players summon their retainers.
 * The Lua script manages:
 * - Retainer selection and spawning
 * - Main menu (inventory, gil, market, ventures, equip, class/job)
 * - Venture management
 * - Class/Job assignment
 * 
 * Most UI interactions (dialogs, animations) are handled client-side in Lua.
 * The server handles data operations via yields.
 */

#include <ScriptObject.h>
#include <Actor/Player.h>
#include <Actor/EventObject.h>
#include <Manager/RetainerMgr.h>
#include <Manager/TerritoryMgr.h>
#include <Manager/PlayerMgr.h>
#include <Manager/QuestMgr.h>
#include <Service.h>
#include <cstdlib>
#include <vector>

using namespace Sapphire;

class CmnDefRetainerCall : public Sapphire::ScriptAPI::EventScript
{
private:
  // Per-player state (TODO: should be stored on player object for multi-player safety)
  uint64_t m_currentRetainerId{ 0 };
  uint8_t m_currentRetainerIndex{ 0 };

  // Yield IDs are client-script dependent; these are the ones observed/validated for the 3.35 client.
  static constexpr uint8_t YIELD_SELECT_RETAINER = 13;
  static constexpr uint8_t YIELD_CALL_RETAINER = 7;
  static constexpr uint8_t YIELD_DEPOP_RETAINER = 14;
  static constexpr uint8_t YIELD_RETAINER_MAIN_MENU = 34;

  // Additional yields used by the Lua flow (venture/tutorial flags, housing, etc.).
  static constexpr uint8_t YIELD_ACCEPT_RETAINER_TASK = 33;
  static constexpr uint8_t YIELD_SET_VENTURE_TUTORIAL_FLAG = 35;
  static constexpr uint8_t YIELD_IS_VENTURE_TUTORIAL_FLAG = 36;
  static constexpr uint8_t YIELD_IS_HOUSING_INDOOR = 37;
  static constexpr uint8_t YIELD_GET_RETAINER_FLAG = 38;
  static constexpr uint8_t YIELD_SET_RETAINER_FLAG = 39;
  static constexpr uint8_t YIELD_CANCEL_RETAINER_TASK = 41;

  static constexpr uint32_t QUEST_ILL_CONCEIVED_VENTURE_LIMSA = 66968;
  static constexpr uint32_t QUEST_ILL_CONCEIVED_VENTURE_GRIDANIA = 66969;
  static constexpr uint32_t QUEST_ILL_CONCEIVED_VENTURE_ULDAH = 66970;
  static constexpr uint32_t QUEST_SCIONS_OF_THE_SEVENTH_DAWN = 66045;

  static bool HasVentureUnlockQuestCompleted( const Entity::Player& player )
  {
    if( !player.isQuestCompleted( QUEST_SCIONS_OF_THE_SEVENTH_DAWN ) )
      return false;

    return player.isQuestCompleted( QUEST_ILL_CONCEIVED_VENTURE_LIMSA ) ||
           player.isQuestCompleted( QUEST_ILL_CONCEIVED_VENTURE_GRIDANIA ) ||
           player.isQuestCompleted( QUEST_ILL_CONCEIVED_VENTURE_ULDAH );
  }

public:
  CmnDefRetainerCall()
      : Sapphire::ScriptAPI::EventScript( 720906 )
  {
  }

  void onTalk( uint32_t eventId, Entity::Player& player, uint64_t actorId ) override
  {
    // Ensure the client has up-to-date quest completion flags. This matters for
    // retainer menu gating (ventures unlock quest) and avoids client-side false negatives.
    Common::Service< World::Manager::QuestMgr >::ref().sendQuestsInfo( player );

    // Retail 3.35 ordering: send retainer list/gating packets BEFORE the scene starts.
    // The client expects to receive the roster data before EventPlay, not during a yield.
    // NOTE: sendEventStart=false because the framework already sent EventStart (0x01CC)
    // via eventMgr.eventStart() in eventHandlerTalk before calling our onTalk.
    auto& retainerMgr = Common::Service< World::Manager::RetainerMgr >::ref();
    retainerMgr.sendRetainerList( player, 0, true, false );

    Scene00000( player );
  }

  void onYield( uint32_t eventId,
                uint16_t sceneId,
                uint8_t yieldId,
                Entity::Player& player,
                const std::string& /*resultString*/,
                uint64_t returnInt ) override
  {
    auto& retainerMgr = Common::Service< World::Manager::RetainerMgr >::ref();

    const uint32_t arg0 = static_cast< uint32_t >( returnInt & 0xFFFFFFFFu );
    const uint32_t arg1 = static_cast< uint32_t >( ( returnInt >> 32 ) & 0xFFFFFFFFu );

    auto resumeDefault = [ & ]( std::initializer_list< uint32_t > args ) {
      eventMgr().resumeScene( player, eventId, sceneId, yieldId, std::vector< uint32_t >( args ) );
    };

    playerMgr().sendDebug( player,
                           "RetainerCall::onYield sceneId={} yieldId={} arg0=0x{:08X} arg1=0x{:08X}",
                           sceneId,
                           yieldId,
                           arg0,
                           arg1 );

    switch( yieldId )
    {
        // ========== Core Retainer Operations ==========

      case YIELD_SELECT_RETAINER:
      {
        // Retainer list was already sent in onTalk (before scene started), per retail 3.35 ordering.
        // Just send the resume with the retainer count (0 = success with retainers, 444 = no retainers).
        const uint8_t retainerCount = retainerMgr.getRetainerCount( player );
        resumeDefault( { retainerCount == 0 ? 444u : 0u } );
        break;
      }

      case YIELD_CALL_RETAINER:
      {
        const auto retainers = retainerMgr.getRetainers( player );

        uint64_t retainerId = 0;
        uint8_t retainerSlot = 0xFF;

        auto tryMatchRetainerId = [ & ]( uint32_t candidate ) -> bool {
          for( const auto& retainer : retainers )
          {
            if( retainer.retainerId == static_cast< uint64_t >( candidate ) )
            {
              retainerId = retainer.retainerId;
              retainerSlot = retainer.displayOrder;
              return true;
            }
          }
          return false;
        };

        if( arg1 != 0 )
          ( void ) tryMatchRetainerId( arg1 );
        if( retainerId == 0 && arg0 != 0 )
          ( void ) tryMatchRetainerId( arg0 );

        if( retainerId == 0 )
        {
          const uint8_t slotCandidateA = static_cast< uint8_t >( arg0 & 0xFF );
          const uint8_t slotCandidateB = static_cast< uint8_t >( arg1 & 0xFF );

          auto tryMatchSlot = [ & ]( uint8_t candidate ) -> bool {
            for( const auto& retainer : retainers )
            {
              if( retainer.displayOrder == candidate )
              {
                retainerId = retainer.retainerId;
                retainerSlot = candidate;
                return true;
              }
            }
            return false;
          };

          if( slotCandidateB < 8 )
            ( void ) tryMatchSlot( slotCandidateB );
          if( retainerId == 0 && slotCandidateA < 8 )
            ( void ) tryMatchSlot( slotCandidateA );
        }

        if( retainerId == 0 || retainerSlot == 0xFF )
        {
          playerMgr().sendDebug( player,
                                 "RetainerCall: CALL_RETAINER unable to resolve selection (arg0=0x{:08X} arg1=0x{:08X})",
                                 arg0,
                                 arg1 );
          resumeDefault( { 1, 0, 0 } );
          break;
        }

        m_currentRetainerId = retainerId;
        m_currentRetainerIndex = retainerSlot;

        const Common::FFXIVARR_POSITION3* bellPos = nullptr;
        Common::FFXIVARR_POSITION3 bellPosition;

        if( auto pEvent = player.getEvent( eventId ) )
        {
          const uint64_t bellActorId = pEvent->getActorId();
          auto pZone = teriMgr().getTerritoryByGuId( player.getTerritoryId() );
          if( pZone )
          {
            auto pBell = pZone->getEObj( static_cast< uint32_t >( bellActorId ) );
            if( pBell )
            {
              bellPosition = pBell->getPos();
              bellPos = &bellPosition;
            }
          }
        }

        const uint32_t actorId = retainerMgr.spawnRetainer( player, retainerId, bellPos );

        if( actorId == 0 )
        {
          resumeDefault( { 1, 0, 0 } );
        }
        else
        {
          retainerMgr.sendRetainerInfo( player, retainerId );
          retainerMgr.sendRetainerInventory( player, retainerId );

          const uint8_t greetingType = static_cast< uint8_t >( std::rand() % 3 );
          resumeDefault( { 0, actorId, greetingType } );
        }
        break;
      }

      case YIELD_DEPOP_RETAINER:
      {
        const uint64_t closingRetainerId = m_currentRetainerId;

        std::vector< uint32_t > actorIdsToDespawn;
        {
          const auto retainers = retainerMgr.getRetainers( player );
          actorIdsToDespawn.reserve( retainers.size() );
          for( const auto& retainer : retainers )
          {
            const uint32_t actorId = retainerMgr.getSpawnedRetainerActorId( player, retainer.retainerId );
            if( actorId != 0 )
              actorIdsToDespawn.push_back( actorId );
          }
        }

        m_currentRetainerId = 0;
        m_currentRetainerIndex = 0;

        retainerMgr.clearActiveRetainer( player );

        resumeDefault( { 0 } );
        // Retail ordering (3.35 capture): client sends 0x01E0, then server emits:
        //   0x01D8 (ResumeEventScene2) -> 0x01CD (PopEventState) -> 0x01A3 (Condition) -> 0x0143 (OrderMySelf close)
        // Keep our sequence aligned to avoid leaving the player in a "busy" state.
        retainerMgr.sendRetainerSessionClose( player );
        eventMgr().eventFinish( player, eventId, 1 );
        retainerMgr.sendRetainerConditionClose( player );
        retainerMgr.sendRetainerActorControlCloseGate( player );
        retainerMgr.sendRetainerOrderMySelfClose( player );

        for( const uint32_t actorId : actorIdsToDespawn )
          retainerMgr.despawnRetainer( player, actorId );

        retainerMgr.sendIsMarket( player );
        if( closingRetainerId != 0 )
        {
          if( const auto retainerOpt = retainerMgr.getRetainer( closingRetainerId ) )
            retainerMgr.sendMarketRetainerUpdate( player, *retainerOpt );
        }

        break;
      }

        // ========== Menu Operations ==========

      case YIELD_RETAINER_MAIN_MENU:
      {
        if( m_currentRetainerId != 0 )
        {
          retainerMgr.sendRetainerInfo( player, m_currentRetainerId );
          retainerMgr.sendRetainerInventory( player, m_currentRetainerId );
        }

        resumeDefault( { 0 } );
        break;
      }

        // ========== Venture / Tutorial / Flags ==========

      case YIELD_ACCEPT_RETAINER_TASK:
      {
        // arg0 = venture/task id.
        if( !HasVentureUnlockQuestCompleted( player ) )
        {
          resumeDefault( { 1 } );
          break;
        }

        if( m_currentRetainerId != 0 && arg0 != 0 )
        {
          const auto error = retainerMgr.startVenture( player, m_currentRetainerId, arg0 );
          resumeDefault( { error == World::Manager::RetainerError::None ? 0u : 1u } );
        }
        else
        {
          resumeDefault( { 1 } );
        }
        break;
      }

      case YIELD_CANCEL_RETAINER_TASK:
      {
        if( m_currentRetainerId != 0 )
          retainerMgr.cancelVenture( player, m_currentRetainerId );
        resumeDefault( { 0 } );
        break;
      }

      case YIELD_SET_VENTURE_TUTORIAL_FLAG:
      case YIELD_IS_VENTURE_TUTORIAL_FLAG:
        // TODO: Persist actual tutorial flags; for now, behave as "flag is set" to avoid repeated tutorials.
        resumeDefault( { 1 } );
        break;

      case YIELD_IS_HOUSING_INDOOR:
        // TODO: Proper housing territory detection.
        resumeDefault( { 0 } );
        break;

      case YIELD_GET_RETAINER_FLAG:
        // TODO: Implement per-retainer flags.
        resumeDefault( { 0 } );
        break;

      case YIELD_SET_RETAINER_FLAG:
        // TODO: Implement per-retainer flags.
        resumeDefault( { 1 } );
        break;

      default:
        resumeDefault( { 1 } );
        break;
    }
  }

  void Scene00000( Entity::Player& player )
  {
    eventMgr().playScene( player, getId(), 0, HIDE_HOTBAR, bindSceneReturn( &CmnDefRetainerCall::Scene00000Return ) );
  }

  void Scene00000Return( Entity::Player& player, const Event::SceneResult& result )
  {
    // Despawn any spawned retainers when the event ends
    auto& retainerMgr = Common::Service< World::Manager::RetainerMgr >::ref();
    auto retainers = retainerMgr.getRetainers( player );
    for( const auto& retainer : retainers )
    {
      uint32_t actorId = retainerMgr.getSpawnedRetainerActorId( player, retainer.retainerId );
      if( actorId != 0 )
      {
        retainerMgr.despawnRetainer( player, actorId );
      }
    }

    eventMgr().eventFinish( player, getId(), 1 );
  }
};

EXPOSE_SCRIPT( CmnDefRetainerCall );
