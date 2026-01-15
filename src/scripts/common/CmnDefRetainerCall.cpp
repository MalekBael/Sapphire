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

public:
  CmnDefRetainerCall()
      : Sapphire::ScriptAPI::EventScript( 720906 )
  {
  }

  void onTalk( uint32_t eventId, Entity::Player& player, uint64_t actorId ) override
  {
    Scene00000( player );
  }

  // ========== Yield IDs from Lua script (cmndefretainercall_00010.luab) ==========
  // These correspond to server-side functions that require data operations
  // NOTE: RetainerBag, RetainerCharacterWidget are CLIENT-SIDE ONLY and do NOT yield!
  //       They are blocking calls that open UI modals using cached data.
  //
  // VERIFIED FROM BINARY (3.35):
  // - CallRetainer (sub_140634330) → yield 7
  // - SelectRetainer (sub_140634A50) → yield 13
  // - DepopRetainer (sub_140634EB0) → yield 14
  // - RetainerMainMenu (sub_1406348E0) → yield 34

  // Core retainer operations
  static constexpr uint8_t YIELD_CALL_RETAINER = 7;   // CallRetainer(slot) - spawns retainer NPC
  static constexpr uint8_t YIELD_SELECT_RETAINER = 13;// SelectRetainer() - opens retainer selection UI
  static constexpr uint8_t YIELD_DEPOP_RETAINER = 14; // DepopRetainer() - despawns retainer NPC

  // Menu operations
  static constexpr uint8_t YIELD_RETAINER_MAIN_MENU = 34;// RetainerMainMenu() - shows main menu, waits for server
  // RetainerMainMenuAfter does NOT yield - it returns the selection directly to Lua

  // Class/Job operations (CORRECTED based on actual Lua behavior)
  static constexpr uint8_t YIELD_RETAINER_CLASS_SELECT = 29; // RetainerClassSelect() - class selection menu
  static constexpr uint8_t YIELD_SET_RETAINER_CLASS_JOB = 30;// SetRetainerClassJob(classId) - assign class
  static constexpr uint8_t YIELD_RETAINER_JOB_SELECT = 20;   // RetainerJobSelect() - job selection menu

  // Data queries (these return values to Lua)
  static constexpr uint8_t YIELD_GET_RETAINER_NAME = 22;          // GetRetainerName() -> string
  static constexpr uint8_t YIELD_GET_RETAINER_CLASS_JOB = 23;     // GetRetainerClassJob() -> (classId, level)
  static constexpr uint8_t YIELD_GET_RETAINER_TASK_DETAILS = 24;  // GetRetainerTaskDetails() -> (taskId, isComplete)
  static constexpr uint8_t YIELD_IS_RETAINER_MANNEQUIN_EMPTY = 25;// IsRetainerMannequinEmpty() -> bool
  static constexpr uint8_t YIELD_CAN_RETAINER_JOB_CHANGE = 26;    // CanRetainerJobChange() -> bool
  static constexpr uint8_t YIELD_GET_RETAINER_EMPLOYED_COUNT = 27;// GetRetainerEmployedCount() -> count

  // Venture operations (renumbered - 28 stays, moved tasks to higher IDs)
  static constexpr uint8_t YIELD_RETAINER_TASK_LV_RANGE = 28;// RetainerTaskLvRange() - level range selection
  static constexpr uint8_t YIELD_RETAINER_TASK_SELECT = 18;  // RetainerTaskSelect(lvRange, isTreasure) - task selection
  static constexpr uint8_t YIELD_RETAINER_TASK_STATUS = 19;  // RetainerTaskStatus() - view current task
  static constexpr uint8_t YIELD_RETAINER_TASK_RESULT = 31;  // RetainerTaskResult(...) - show results
  static constexpr uint8_t YIELD_RETAINER_TASK_ASK = 32;     // RetainerTaskAsk() - confirm task
  static constexpr uint8_t YIELD_ACCEPT_RETAINER_TASK = 33;  // AcceptRetainerTask(taskId) - start task

  // Tutorial flags
  static constexpr uint8_t YIELD_SET_VENTURE_TUTORIAL_FLAG = 35;// SetVentureTutorialFlag(flag, value)
  static constexpr uint8_t YIELD_IS_VENTURE_TUTORIAL_FLAG = 36; // IsVentureTutorialFlag(flag) -> bool

  // Housing check
  static constexpr uint8_t YIELD_IS_HOUSING_INDOOR = 37;// IsHousingIndoorTerritory() -> bool

  // Retainer flags
  static constexpr uint8_t YIELD_GET_RETAINER_FLAG = 38;// GetRetainerFlag(flag) -> bool
  static constexpr uint8_t YIELD_SET_RETAINER_FLAG = 39;// SetRetainerFlag(flag, value)

  // Note: RetainerCharacterWidget doesn't yield - it's client-side only
  static constexpr uint8_t YIELD_CANCEL_RETAINER_TASK = 41;// CancelRetainerTask() - abort task

  // Menu constants from Lua
  enum RetainerMenuOption : uint8_t
  {
    RETAINER_MENU_INVENTORY = 0,
    RETAINER_MENU_GIL = 1,
    RETAINER_MENU_BUYBACK = 2,
    RETAINER_MENU_MARKET_1 = 3,
    RETAINER_MENU_MARKET_2 = 4,
    RETAINER_MENU_SELLHISTORY = 5,
    RETAINER_MENU_TASK_STATUS = 6,
    RETAINER_MENU_VENTURE = 7,
    RETAINER_MENU_EQUIP = 8,
    RETAINER_MENU_CLASS = 9,
    RETAINER_MENU_JOB = 10,
    RETAINER_MENU_CLOSE = 11
  };

  // Inventory mode constants
  enum RetainerBagMode : uint8_t
  {
    BAG_MODE_ITEM = 1,
    BAG_MODE_MONEY = 2,
    BAG_MODE_LIST_TO_PLAYER = 3,
    BAG_MODE_LIST_TO_RETAINER = 4,
    BAG_MODE_HISTORY = 5
  };

  void onYield( uint32_t eventId, uint16_t sceneId, uint8_t yieldId, Entity::Player& player,
                const std::string& resultString, uint64_t resultInt ) override
  {
    auto& retainerMgr = Common::Service< World::Manager::RetainerMgr >::ref();

    // Extract packed args: low 32 bits = arg#0, high 32 bits = arg#1
    uint32_t arg0 = static_cast< uint32_t >( resultInt & 0xFFFFFFFF );
    uint32_t arg1 = static_cast< uint32_t >( resultInt >> 32 );

    switch( yieldId )
    {
        // ========== Core Retainer Operations ==========

      case YIELD_SELECT_RETAINER:
      {
        uint8_t retainerCount = retainerMgr.getRetainerCount( player );
        if( retainerCount == 0 )
        {
          eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 444 } );
        }
        else
        {
          // Send the retainer list packets so client can display them
          retainerMgr.sendRetainerList( player );
          // Retail 3.35 capture: ResumeEventScene2(resumeId=13) returns numArgs=1, arg0=0 on success.
          eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 0 } );
        }
        break;
      }

      case YIELD_CALL_RETAINER:
      {
        // 3.35 observed shapes:
        // - Some scenes provide a slot index (displayOrder 0..7).
        // - Other scenes provide a retainerId directly (e.g., 0x0E / 0x0F).
        // Additionally, we've seen arg0==0 while arg1 carries the meaningful value.
        // Prefer matching a known retainerId before interpreting as a slot.

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

          // Prefer the high-dword slot when it is meaningful.
          if( slotCandidateB < 8 )
            ( void ) tryMatchSlot( slotCandidateB );
          if( retainerId == 0 && slotCandidateA < 8 )
            ( void ) tryMatchSlot( slotCandidateA );
        }

        if( retainerId == 0 || retainerSlot == 0xFF )
        {
          Logger::warn( "RetainerCall: CALL_RETAINER unable to resolve selection (arg0=0x{:08X} arg1=0x{:08X})",
                        arg0,
                        arg1 );
          eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 1, 0, 0 } );
          break;
        }

        // Store current retainer for subsequent operations.
        m_currentRetainerId = retainerId;
        m_currentRetainerIndex = retainerSlot;

        // Get the summoning bell's position from the event
        const Common::FFXIVARR_POSITION3* bellPos = nullptr;
        Common::FFXIVARR_POSITION3 bellPosition;

        auto pEvent = player.getEvent( eventId );
        if( pEvent )
        {
          uint64_t bellActorId = pEvent->getActorId();
          auto& teriMgr = Common::Service< World::Manager::TerritoryMgr >::ref();
          auto pZone = teriMgr.getTerritoryByGuId( player.getTerritoryId() );
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

        // Spawn the retainer NPC
        uint32_t actorId = retainerMgr.spawnRetainer( player, retainerId, bellPos );

        if( actorId == 0 )
        {
          eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 1, 0, 0 } );
        }
        else
        {
          // Send retainer info and inventory data so client can display bag/gil screens
          retainerMgr.sendRetainerInfo( player, retainerId );
          retainerMgr.sendRetainerInventory( player, retainerId );

          uint8_t greetingType = static_cast< uint8_t >( std::rand() % 3 );
          eventMgr().resumeScene( player, eventId, sceneId, yieldId,
                                  { 0, actorId, greetingType } );
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

        // Clear active retainer context for inventory operations (gil transfer, etc.).
        retainerMgr.clearActiveRetainer( player );

        // Retail ordering (3.35 capture) after client YieldEventScene2(yieldId=14):
        //   ResumeEventScene2 -> PopEventState (+Condition) -> Delete (ActorFreeSpawn) -> IsMarket -> MarketRetainerUpdate
        // The critical piece for avoiding the client getting stuck Busy is ensuring we actually finish the event,
        // which clears InNpcEvent and emits the Condition packet.
        eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 0 } );
        eventMgr().eventFinish( player, eventId, 1 );

        for( const uint32_t actorId : actorIdsToDespawn )
          retainerMgr.despawnRetainer( player, actorId );

        // Refresh market/retainer state after despawn.
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
        // RetainerMainMenu() yields to server before showing the menu
        // Server should send any necessary data updates, then resume with menu option count
        // The Lua script then displays the menu and waits for user selection
        // After selection, RetainerMainMenuAfter() is called (NO YIELD - client-side only)
        // Then based on selection, RetainerBag(mode) or other functions are called (also client-side)

        if( m_currentRetainerId != 0 )
        {
          // Send updated retainer info and inventory data before showing menu
          // This ensures the client has current data for any option the user might pick
          retainerMgr.sendRetainerInfo( player, m_currentRetainerId );
          retainerMgr.sendRetainerInventory( player, m_currentRetainerId );
        }

        // Retail 3.35 capture: ResumeEventScene2(resumeId=34) returns numArgs=1, arg0=0.
        // The client-side Lua then drives the actual menu UI.
        eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 0 } );
        break;
      }

        // ========== Data Queries ==========

      case YIELD_GET_RETAINER_NAME:
      {
        std::string name = "";
        if( m_currentRetainerId != 0 )
        {
          auto retainerOpt = retainerMgr.getRetainer( m_currentRetainerId );
          if( retainerOpt )
          {
            name = retainerOpt->name;
          }
        }
        // Return name via string result (if supported) or as placeholder
        eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 0 } );
        break;
      }

      case YIELD_GET_RETAINER_CLASS_JOB:
      {
        uint8_t classId = 0;
        uint8_t level = 1;
        if( m_currentRetainerId != 0 )
        {
          auto retainerOpt = retainerMgr.getRetainer( m_currentRetainerId );
          if( retainerOpt )
          {
            classId = retainerOpt->classJob;
            level = retainerOpt->level;
          }
        }
        eventMgr().resumeScene( player, eventId, sceneId, yieldId, { classId, level } );
        break;
      }

      case YIELD_GET_RETAINER_TASK_DETAILS:
      {
        uint32_t taskId = 0;
        bool isComplete = false;
        if( m_currentRetainerId != 0 )
        {
          auto retainerOpt = retainerMgr.getRetainer( m_currentRetainerId );
          if( retainerOpt && retainerOpt->ventureId != 0 )
          {
            taskId = retainerOpt->ventureId;
            // Check if venture is complete (ventureComplete flag or time check)
            isComplete = retainerOpt->ventureComplete;
          }
        }
        eventMgr().resumeScene( player, eventId, sceneId, yieldId,
                                { taskId, isComplete ? 1u : 0u } );
        break;
      }

      case YIELD_GET_RETAINER_EMPLOYED_COUNT:
      {
        uint8_t count = retainerMgr.getRetainerCount( player );
        eventMgr().resumeScene( player, eventId, sceneId, yieldId, { count } );
        break;
      }

      case YIELD_IS_RETAINER_MANNEQUIN_EMPTY:
      {
        // Check if retainer has any equipment
        // For now, assume empty (no equipment system implemented)
        eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 1 } );// 1 = empty
        break;
      }

      case YIELD_CAN_RETAINER_JOB_CHANGE:
      {
        // Check if retainer can change to a job (has appropriate level/items)
        // For now, return false
        eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 0 } );// 0 = cannot
        break;
      }

        // ========== Class/Job Operations ==========

      case YIELD_RETAINER_CLASS_SELECT:
      {
        // Show class selection menu
        // Return 0 to indicate cancelled, or class ID selected
        eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 0 } );
        break;
      }

      case YIELD_SET_RETAINER_CLASS_JOB:
      {
        // arg0 = class/job ID to set
        if( m_currentRetainerId != 0 && arg0 != 0 )
        {
          retainerMgr.setRetainerClass( player, m_currentRetainerId, static_cast< uint8_t >( arg0 ) );
          eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 0 } );// 0 = success
          break;
        }

        case YIELD_RETAINER_JOB_SELECT:
        {
          // Show job selection menu (for job crystal upgrade)
          eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 0 } );// 0 = cancelled
          break;
        }

          // ========== Venture Operations ==========

        case YIELD_RETAINER_TASK_LV_RANGE:
        {
          // Return the max level range available for ventures
          // Based on retainer level, return number of available level brackets
          uint8_t maxRange = 1;
          if( m_currentRetainerId != 0 )
          {
            auto retainerOpt = retainerMgr.getRetainer( m_currentRetainerId );
            if( retainerOpt )
            {
              // Each 5 levels opens a new range
              maxRange = ( retainerOpt->level / 5 ) + 1;
            }
          }
          eventMgr().resumeScene( player, eventId, sceneId, yieldId, { maxRange } );
          break;
        }

        case YIELD_RETAINER_TASK_SELECT:
        {
          // arg0 = level range, arg1 = isTreasure (exploration)
          // Return selected task ID or 0 for cancel
          eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 0 } );
          break;
        }

        case YIELD_RETAINER_TASK_STATUS:
        {
          // Show current venture status
          // Return 0 = don't cancel, 1 = cancel venture
          eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 0 } );
          break;
        }

        case YIELD_RETAINER_TASK_RESULT:
        {
          // Show venture results (called after completing venture)
          // Return 0 for success
          eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 0 } );
          break;
        }

        case YIELD_RETAINER_TASK_ASK:
        {
          // Confirm starting a venture
          // arg0 = task ID
          // Return 0 for no, task ID for yes
          eventMgr().resumeScene( player, eventId, sceneId, yieldId, { arg0 } );
          break;
        }

        case YIELD_ACCEPT_RETAINER_TASK:
        {
          // arg0 = task ID to start
          if( m_currentRetainerId != 0 && arg0 != 0 )
          {
            auto error = retainerMgr.startVenture( player, m_currentRetainerId, arg0 );
            if( error == World::Manager::RetainerError::None )
            {
              eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 0 } );// Success
            }
            else
            {
              eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 1 } );// Error
            }
          }
          else
          {
            eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 1 } );
          }
          break;
        }

        case YIELD_CANCEL_RETAINER_TASK:
        {
          if( m_currentRetainerId != 0 )
          {
            retainerMgr.cancelVenture( player, m_currentRetainerId );
          }
          eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 0 } );
          break;
        }

          // ========== Tutorial Flags ==========

        case YIELD_SET_VENTURE_TUTORIAL_FLAG:
        {
          // arg0 = flag, arg1 = value
          // TODO: Implement tutorial flag storage
          eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 1 } );
          break;
        }

        case YIELD_IS_VENTURE_TUTORIAL_FLAG:
        {
          // arg0 = flag to check
          // TODO: Implement tutorial flag checking
          // For now, return true (flag is set) to skip tutorials
          eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 1 } );
          break;
        }

          // ========== Misc ==========

        case YIELD_IS_HOUSING_INDOOR:
        {
          // Check if current territory is indoor housing
          // TODO: Proper housing territory detection
          eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 0 } );// 0 = not indoor
          break;
        }

        case YIELD_GET_RETAINER_FLAG:
        {
          // arg0 = flag ID
          // TODO: Implement retainer flags
          eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 0 } );// Flag not set
          break;
        }

        case YIELD_SET_RETAINER_FLAG:
        {
          // arg0 = flag ID, arg1 = value
          // TODO: Implement retainer flags
          eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 1 } );
          break;
        }

        default:
          eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 1 } );
          break;
      }
    }

    void Scene00000( Entity::Player & player )
    {
      eventMgr().playScene( player, getId(), 0, HIDE_HOTBAR, bindSceneReturn( &CmnDefRetainerCall::Scene00000Return ) );
    }

    void Scene00000Return( Entity::Player & player, const Event::SceneResult& result )
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
