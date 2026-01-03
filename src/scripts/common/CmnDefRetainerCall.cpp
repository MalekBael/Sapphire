/**
 * @file CmnDefRetainerCall.cpp
 * @brief Server-side implementation of the Retainer Call event (summoning a retainer)
 * 
 * Event ID: 720906 (0x000B000A)
 * Client Script: cmndefretainercall_00010.luab
 * 
 * This event is triggered when the player selects a retainer to summon from the
 * retainer selection menu (which was shown by CmnDefRetainerDesk).
 * 
 * Flow:
 *   Scene 0: Entry point
 *     - Yield 1: SelectRetainer - opens retainer selection widget
 *     - Returns selected index (1-8) or 444 if cancelled
 *   
 *   Scene 1: Spawn and interact with retainer
 *     - Yield 1: CallRetainer(index) - spawn retainer actor
 *     - Yield 2: BindRetainer(actorRef) - bind for cutscene control
 *     - Various greetings and menu handling
 *   
 *   Scene 2: Main menu loop (handled by client Lua)
 *   
 *   Scene 3: Dismiss retainer
 *     - DepopRetainer() - despawn retainer actor
 * 
 * TODO: Full implementation requires:
 *   - Spawning retainer as a temporary character actor
 *   - Cutscene binding system for actor control
 *   - Retainer inventory/market board/venture systems
 */

#include <ScriptObject.h>
#include <Actor/Player.h>
#include <Manager/PlayerMgr.h>
#include <Manager/RetainerMgr.h>
#include <Service.h>

using namespace Sapphire;

class CmnDefRetainerCall : public Sapphire::ScriptAPI::EventScript
{
public:
  CmnDefRetainerCall() : Sapphire::ScriptAPI::EventScript( 720906 )
  {
  }

  //////////////////////////////////////////////////////////////////////
  // Event Handlers
  //////////////////////////////////////////////////////////////////////

  void onTalk( uint32_t eventId, Entity::Player& player, uint64_t actorId ) override
  {
    playerMgr().sendDebug( player, "RetainerCall::onTalk - eventId: {}, actorId: {:X}", eventId, actorId );
    Scene00000( player );
  }

  /**
   * Client-to-Server Event Subtypes (yieldId values):
   * These are the subtype values from the client's native function calls.
   * Based on analysis of the 3.35 client binary:
   * 
   * Subtype  7: SelectRetainer       - Opens retainer selection widget
   * Subtype  8: DepopRetainer        - Dismiss/despawn retainer
   * Subtype 12: RetainerMainMenu     - Open retainer main menu
   * Subtype 13: RetainerMainMenuAfter - After main menu selection
   * Subtype 17: CallRetainer         - Summon retainer actor
   * Subtype 35: SetVentureTutorialFlag - Tutorial flag acknowledgement
   * 
   * Additional known subtypes (not all used in CmnDefRetainerCall):
   * Subtype  3: LoadRetainerCreation
   * Subtype  4: LoadRetainerRemake
   * Subtype  9: MarketRegister
   * Subtype 10: RetainerTaskLvRange
   * Subtype 14: RetainerClassSelect
   * Subtype 19: SetRetainerClassJob
   * Subtype 27: SendRetainerCharaMake
   * Subtype 31: CompleteRetainerTask
   * Subtype 32: AcceptRetainerTask
   * Subtype 33: CancelRetainerTask
   * Subtype 34: RemoveRetainer
   */
  static constexpr uint8_t YIELD_SELECT_RETAINER = 7;
  static constexpr uint8_t YIELD_DEPOP_RETAINER = 8;
  static constexpr uint8_t YIELD_RETAINER_MAIN_MENU = 12;
  static constexpr uint8_t YIELD_RETAINER_MAIN_MENU_AFTER = 13;
  static constexpr uint8_t YIELD_CALL_RETAINER = 17;
  static constexpr uint8_t YIELD_SET_VENTURE_TUTORIAL_FLAG = 35;

  void onYield( uint32_t eventId, uint16_t sceneId, uint8_t yieldId, Entity::Player& player,
                const std::string& resultString, uint64_t resultInt ) override
  {
    playerMgr().sendDebug( player, "RetainerCall::onYield - scene: {}, yieldId: {}, str: '{}', int: {}",
                           sceneId, yieldId, resultString, resultInt );

    auto& retainerMgr = Common::Service< World::Manager::RetainerMgr >::ref();

    // The yieldId corresponds to the client's event subtype, not scene-specific IDs
    // Handle them globally regardless of scene
    switch( yieldId )
    {
      case YIELD_SELECT_RETAINER: // 7 - SelectRetainer - return selected retainer index
      {
        // For now, auto-select the first retainer (index 1)
        // In a full implementation, this would wait for UI selection
        uint8_t retainerCount = retainerMgr.getRetainerCount( player );
        if( retainerCount == 0 )
        {
          // No retainers - return 444 to indicate none available
          playerMgr().sendDebug( player, "RetainerCall: SelectRetainer - No retainers, returning 444" );
          eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 444 } );
        }
        else
        {
          // Return retainer index 1 (first retainer)
          playerMgr().sendDebug( player, "RetainerCall: SelectRetainer - Selecting retainer 1 of {}", retainerCount );
          eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 1 } );
        }
        break;
      }

      case YIELD_CALL_RETAINER: // 17 - CallRetainer(index) - spawn retainer actor
      {
        // TODO: Actually spawn retainer as character actor
        // For now, return fake actor ID and personality
        uint32_t fakeActorId = 0x10000001; // Placeholder actor ID
        uint8_t personality = 1; // Default personality
        
        playerMgr().sendDebug( player, "RetainerCall: CallRetainer - returning actorId {:X}, personality {}",
                               fakeActorId, personality );
        
        // Return: result (0=success), actorRef, personality
        eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 0, fakeActorId, personality } );
        break;
      }

      case YIELD_DEPOP_RETAINER: // 8 - DepopRetainer - despawn retainer actor
      {
        playerMgr().sendDebug( player, "RetainerCall: DepopRetainer - despawning" );
        // TODO: Actually despawn the retainer actor
        eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 0 } ); // 0 = success
        break;
      }

      case YIELD_RETAINER_MAIN_MENU: // 12 - RetainerMainMenu - open main menu
      {
        playerMgr().sendDebug( player, "RetainerCall: RetainerMainMenu - returning close (11)" );
        // Return cancel/close option to exit the menu
        eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 11 } ); // 11 = RETAINER_MENU_CLOSE
        break;
      }

      case YIELD_RETAINER_MAIN_MENU_AFTER: // 13 - Possibly IsCurrentRetainerActive
      {
        // The client might be checking if retainer is in a paid slot or active
        // Try returning two values like yieldId 35
        // First value might be "is active" (1 = yes, 0 = needs payment)
        // Second value might be slot info or display order
        playerMgr().sendDebug( player, "RetainerCall: yieldId 13 - returning (1, 1) for active retainer" );
        // Return: isActive=1, slotOrStatus=1
        eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 1, 1 } );
        break;
      }

      case YIELD_SET_VENTURE_TUTORIAL_FLAG: // 35 - SetVentureTutorialFlag
      {
        // Tutorial acknowledgement - just return success
        playerMgr().sendDebug( player, "RetainerCall: SetVentureTutorialFlag - acknowledged" );
        eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 1 } );
        break;
      }

      default:
      {
        // Unknown yield - log and return 1 (success/active)
        playerMgr().sendDebug( player, "RetainerCall: Unknown yieldId {} (scene {}) - returning 1", yieldId, sceneId );
        eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 1 } );
        break;
      }
    }
  }

  //////////////////////////////////////////////////////////////////////
  // Scenes
  //////////////////////////////////////////////////////////////////////

  void Scene00000( Entity::Player& player )
  {
    playerMgr().sendDebug( player, "RetainerCall: Playing Scene00000" );
    
    // Scene 0 is the entry point - the Lua script will call SelectRetainer()
    // which yields back to us to get the player's selection
    eventMgr().playScene( player, getId(), 0, HIDE_HOTBAR, bindSceneReturn( &CmnDefRetainerCall::Scene00000Return ) );
  }

  void Scene00000Return( Entity::Player& player, const Event::SceneResult& result )
  {
    auto returnValue = result.getResult( 0 );
    playerMgr().sendDebug( player, "RetainerCall: Scene00000Return - result: {}", returnValue );
    
    // The Lua script returns:
    //   -1 = Error/cancelled
    //    0 = Normal completion, proceed to Scene 1
    //   >0 = Other scene to chain to
    
    if( returnValue == static_cast< uint32_t >( -1 ) || returnValue == 0 )
    {
      // Event complete or cancelled
      playerMgr().sendDebug( player, "RetainerCall: Finishing event" );
      eventMgr().eventFinish( player, getId(), 1 );
    }
    else
    {
      // Chain to returned scene (usually Scene 1)
      playerMgr().sendDebug( player, "RetainerCall: Unexpected return value {}, finishing", returnValue );
      eventMgr().eventFinish( player, getId(), 1 );
    }
  }
};

EXPOSE_SCRIPT( CmnDefRetainerCall );
