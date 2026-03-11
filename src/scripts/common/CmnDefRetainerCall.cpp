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
#include <Inventory/ItemContainer.h>
#include <Manager/RetainerMgr.h>
#include <Manager/TerritoryMgr.h>
#include <Manager/PlayerMgr.h>
#include <Manager/QuestMgr.h>
#include <Common.h>
#include <Service.h>
#include <Logging/Logger.h>
#include <cstdlib>
#include <optional>
#include <unordered_map>
#include <vector>

using namespace Sapphire;

class CmnDefRetainerCall : public Sapphire::ScriptAPI::EventScript
{
private:
  uint64_t m_currentRetainerId{ 0 };
  uint8_t m_currentRetainerIndex{ 0 };
  bool m_sentInventoryThisSession{ false };
  uint32_t m_ventureTutorialFlags{ 0 };
  std::unordered_map< uint32_t, uint32_t > m_retainerFlags;

  static constexpr uint8_t YIELD_SELECT_RETAINER = 13;
  static constexpr uint8_t YIELD_CALL_RETAINER = 7;
  static constexpr uint8_t YIELD_DEPOP_RETAINER = 14;
  static constexpr uint8_t YIELD_RETAINER_MAIN_MENU = 34;

  static constexpr uint8_t YIELD_CLASS_CATEGORY_INTRO = 29;
  static constexpr uint8_t YIELD_SET_RETAINER_CLASS = 30;

  static constexpr uint8_t YIELD_COMPLETE_RETAINER_TASK = 31;
  static constexpr uint8_t YIELD_ACCEPT_RETAINER_TASK = 32;
  static constexpr uint8_t YIELD_CANCEL_RETAINER_TASK = 33;

  static constexpr uint8_t YIELD_SET_VENTURE_TUTORIAL_FLAG = 35;
  static constexpr uint8_t YIELD_IS_VENTURE_TUTORIAL_FLAG = 36;
  static constexpr uint8_t YIELD_IS_HOUSING_INDOOR = 37;
  static constexpr uint8_t YIELD_GET_RETAINER_FLAG = 38;
  static constexpr uint8_t YIELD_SET_RETAINER_FLAG = 39;

  static constexpr uint8_t YIELD_CANCEL_RETAINER_TASK_LEGACY = 41;

public:
  CmnDefRetainerCall()
      : Sapphire::ScriptAPI::EventScript( 720906 )
  {
  }

  void onTalk( uint32_t eventId, Entity::Player& player, uint64_t actorId ) override
  {
    m_currentRetainerId = 0;
    m_currentRetainerIndex = 0;
    m_sentInventoryThisSession = false;
    m_ventureTutorialFlags = 0;
    m_retainerFlags.clear();

    auto& retainerMgr = Common::Service< World::Manager::RetainerMgr >::ref();
    retainerMgr.sendRetainerList( player, 0, true, false );

    Scene00000( player );
  }

  void Scene00000( Entity::Player& player )
  {
    eventMgr().playScene( player, getId(), 0, 0,
                          [ & ]( Entity::Player& player, const Event::SceneResult& result ) {
                          } );
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
    auto resumeOk = [ & ]() {
      resumeDefault( { 0 } );
    };
    auto resumeError = [ & ]() {
      resumeDefault( { 1 } );
    };
    auto resumeResult = [ & ]( bool ok ) {
      resumeDefault( { ok ? 0u : 1u } );
    };

    struct RetainerSelection {
      uint64_t retainerId{ 0 };
      uint8_t slot{ 0xFF };
    };

    auto resolveSelection = [ & ]( const std::vector< World::Manager::RetainerData >& retainers )
            -> RetainerSelection {
      RetainerSelection selection{};

      auto matchById = [ & ]( uint32_t candidate ) -> bool {
        for( const auto& retainer : retainers )
        {
          if( retainer.retainerId == static_cast< uint64_t >( candidate ) )
          {
            selection.retainerId = retainer.retainerId;
            selection.slot = retainer.displayOrder;
            return true;
          }
        }
        return false;
      };

      if( arg1 != 0 )
        ( void ) matchById( arg1 );
      if( selection.retainerId == 0 && arg0 != 0 )
        ( void ) matchById( arg0 );

      if( selection.retainerId != 0 )
        return selection;

      const uint8_t slotCandidateA = static_cast< uint8_t >( arg0 & 0xFF );
      const uint8_t slotCandidateB = static_cast< uint8_t >( arg1 & 0xFF );

      auto matchBySlot = [ & ]( uint8_t candidate ) -> bool {
        for( const auto& retainer : retainers )
        {
          if( retainer.displayOrder == candidate )
          {
            selection.retainerId = retainer.retainerId;
            selection.slot = candidate;
            return true;
          }
        }
        return false;
      };

      if( slotCandidateB < 8 )
        ( void ) matchBySlot( slotCandidateB );
      if( selection.retainerId == 0 && slotCandidateA < 8 )
        ( void ) matchBySlot( slotCandidateA );

      return selection;
    };

    auto resolveBellPosition = [ & ]() -> std::optional< Common::FFXIVARR_POSITION3 > {
      auto pEvent = player.getEvent( eventId );
      if( !pEvent )
        return std::nullopt;

      const uint64_t bellActorId = pEvent->getActorId();
      auto pZone = teriMgr().getTerritoryByGuId( player.getTerritoryId() );
      if( !pZone )
        return std::nullopt;

      auto pBell = pZone->getEObj( static_cast< uint32_t >( bellActorId ) );
      if( !pBell )
        return std::nullopt;

      return pBell->getPos();
    };

    auto sendSelectionGating = [ & ]() {
      // Retail order: ResumeEventScene2 (0x01D8) -> 0x0143 -> inventory burst.
      retainerMgr.sendSetRetainerSubscription( player );
      if( m_currentRetainerId != 0 && !m_sentInventoryThisSession )
      {
        retainerMgr.sendRetainerInventory( player, m_currentRetainerId );
        m_sentInventoryThisSession = true;
      }
    };

    switch( yieldId )
    {
      case YIELD_SELECT_RETAINER:
      {
        const uint8_t retainerCount = retainerMgr.getRetainerCount( player );
        resumeDefault( { retainerCount == 0 ? 444u : 0u } );
        break;
      }

      case YIELD_CALL_RETAINER:
      {
        const auto retainers = retainerMgr.getRetainers( player );
        const auto selection = resolveSelection( retainers );

        if( selection.retainerId == 0 || selection.slot == 0xFF )
        {
          Logger::debug( "RetainerCall: CALL_RETAINER unable to resolve selection (arg0=0x{:08X} arg1=0x{:08X})",
                         arg0,
                         arg1 );
          resumeDefault( { 1, 0, 0 } );
          break;
        }

        m_currentRetainerId = selection.retainerId;
        m_currentRetainerIndex = selection.slot;
        m_sentInventoryThisSession = false;

        const auto bellPosition = resolveBellPosition();
        const Common::FFXIVARR_POSITION3* bellPos = bellPosition ? &*bellPosition : nullptr;

        const uint32_t actorId = retainerMgr.spawnRetainer( player, selection.retainerId, bellPos );
        if( actorId == 0 )
        {
          resumeDefault( { 1, 0, 0 } );
          break;
        }

        const uint8_t greetingType = static_cast< uint8_t >( std::rand() % 3 );
        resumeDefault( { 0, actorId, greetingType } );

        sendSelectionGating();
        break;
      }

      case YIELD_DEPOP_RETAINER:
      {
        const uint64_t closingRetainerId = m_currentRetainerId;

        std::vector< uint32_t > actorIdsToDespawn;
        actorIdsToDespawn.reserve( 8 );
        for( const auto& retainer : retainerMgr.getRetainers( player ) )
        {
          const uint32_t actorId = retainerMgr.getSpawnedRetainerActorId( player, retainer.retainerId );
          if( actorId != 0 )
            actorIdsToDespawn.push_back( actorId );
        }

        m_currentRetainerId = 0;
        m_currentRetainerIndex = 0;
        m_sentInventoryThisSession = false;

        retainerMgr.clearActiveRetainer( player );

        resumeOk();
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

        retainerMgr.sendRetainerSessionMarker( player );
        break;
      }

      case YIELD_RETAINER_MAIN_MENU:
      {
        if( m_currentRetainerId != 0 && !m_sentInventoryThisSession )
        {
          retainerMgr.sendRetainerInventory( player, m_currentRetainerId );
          m_sentInventoryThisSession = true;
        }

        resumeOk();
        break;
      }

      case YIELD_CLASS_CATEGORY_INTRO:
      {
        resumeOk();
        break;
      }

      case YIELD_SET_RETAINER_CLASS:
      {
        if( m_currentRetainerId == 0 || arg0 == 0 )
        {
          resumeError();
          break;
        }

        bool hasEquipment = false;
        if( auto container = player.getInventoryContainer(
                    static_cast< uint16_t >( Common::InventoryType::RetainerEquippedGear ) ) )
        {
          for( uint8_t slot = 0; slot < container->getMaxSize(); ++slot )
          {
            if( container->getItem( slot ) )
            {
              hasEquipment = true;
              break;
            }
          }
        }

        if( hasEquipment )
        {
          resumeError();
          break;
        }

        const auto error = retainerMgr.setRetainerClass( player, m_currentRetainerId, static_cast< uint8_t >( arg0 ) );
        resumeResult( error == World::Manager::RetainerError::None );
        break;
      }

      case YIELD_COMPLETE_RETAINER_TASK:
      {
        if( m_currentRetainerId == 0 )
        {
          resumeError();
          break;
        }

        const auto error = retainerMgr.completeVenture( player, m_currentRetainerId );
        resumeResult( error == World::Manager::RetainerError::None );
        break;
      }

      case YIELD_ACCEPT_RETAINER_TASK:
      {
        const uint32_t ventureId = arg0;
        if( m_currentRetainerId == 0 || ventureId == 0 )
        {
          resumeError();
          break;
        }

        const auto error = retainerMgr.startVenture( player, m_currentRetainerId, ventureId );
        resumeResult( error == World::Manager::RetainerError::None );
        break;
      }

      case YIELD_SET_VENTURE_TUTORIAL_FLAG:
      {
        // Lua: SetVentureTutorialFlag(flagBit, bool)
        if( arg0 != 0 )
        {
          if( arg1 != 0 )
            m_ventureTutorialFlags |= arg0;
          else
            m_ventureTutorialFlags &= ~arg0;
        }

        resumeOk();
        break;
      }

      case YIELD_IS_VENTURE_TUTORIAL_FLAG:
      {
        // Lua: IsVentureTutorialFlag(flagBit) -> bool
        const bool isSet = ( arg0 != 0 ) && ( ( m_ventureTutorialFlags & arg0 ) != 0 );
        resumeDefault( { isSet ? 1u : 0u } );
        break;
      }

      case YIELD_IS_HOUSING_INDOOR:
      {
        resumeOk();
        break;
      }

      case YIELD_GET_RETAINER_FLAG:
      {
        const auto it = m_retainerFlags.find( arg0 );
        const uint32_t flagValue = ( it != m_retainerFlags.end() ) ? it->second : 0u;
        resumeDefault( { flagValue } );
        break;
      }

      case YIELD_SET_RETAINER_FLAG:
      {
        m_retainerFlags[ arg0 ] = arg1;
        resumeOk();
        break;
      }

      case YIELD_CANCEL_RETAINER_TASK:
      case YIELD_CANCEL_RETAINER_TASK_LEGACY:
      {
        if( m_currentRetainerId != 0 )
          ( void ) retainerMgr.cancelVenture( player, m_currentRetainerId );
        resumeOk();
        break;
      }

      default:
      {
        resumeOk();
        break;
      }
    }
  }
};

EXPOSE_SCRIPT( CmnDefRetainerCall );
