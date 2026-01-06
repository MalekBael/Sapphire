/**
 * @file CmnDefRetainerDesk.cpp
 * @brief Server-side implementation of the Retainer Vocate NPC event
 * 
 * Event ID: 720905 (0x000B0009)
 * Client Script: cmndefretainerdesk_00009.luab
 * 
 * Yield IDs (Scene 0):
 *   37 = Get retainer list / menu data
 * 
 * Yield IDs (Scene 1 - Retainer Creation):
 *   3  = LoadRetainerCreation - request to start CharaMake
 *   5  = CreateRetainer - submit retainer name and customize data  
 *   24 = GetContentFinderStatus - check if in duty finder
 *   25 = SetCharaMakeCondition - setup CharaMake restrictions
 *   36 = PromptName / CreateRetainer final submit with name
 */

#include <ScriptObject.h>
#include <Actor/Player.h>
#include <Manager/PlayerMgr.h>
#include <Manager/RetainerMgr.h>
#include <Service.h>

using namespace Sapphire;

class CmnDefRetainerDesk : public Sapphire::ScriptAPI::EventScript
{
public:
  CmnDefRetainerDesk() : Sapphire::ScriptAPI::EventScript( 720905 )
  {
  }

  //////////////////////////////////////////////////////////////////////
  // Event Handlers
  //////////////////////////////////////////////////////////////////////

  void onTalk( uint32_t eventId, Entity::Player& player, uint64_t actorId ) override
  {
    Scene00000( player );
  }

  void onYield( uint32_t eventId, uint16_t sceneId, uint8_t yieldId, Entity::Player& player,
                const std::string& resultString, uint64_t resultInt ) override
  {
    playerMgr().sendDebug( player, "RetainerDesk::onYield - scene: {}, yieldId: {}, resultStr: '{}', resultInt: {}",
                           sceneId, yieldId, resultString, resultInt );

    if( sceneId == 0 && yieldId == 37 )
    {
      // Client is requesting retainer list/menu data; send the list packet now so it is fresh.
      auto& retainerMgr = Common::Service< World::Manager::RetainerMgr >::ref();
      retainerMgr.sendRetainerList( player );

      // Scene 0, yieldId 37: Get retainer list data
      // Returns: maxRetainers, currentRetainerCount (Lua expects max first)
      uint8_t maxRetainers = retainerMgr.getMaxRetainerSlots( player );
      uint8_t currentCount = retainerMgr.getRetainerCount( player );
      // Use player debug channel; Logger is not available in script DLLs
      playerMgr().sendDebug( player, "RetainerDesk: yieldId 37 - max: {} current: {}", maxRetainers, currentCount );
      eventMgr().resumeScene( player, eventId, sceneId, yieldId, { maxRetainers, currentCount } );
    }
    else if( sceneId == 1 )
    {
      // Scene 1: Retainer creation flow
      switch( yieldId )
      {
        case 3:  // LoadRetainerCreation - start the creation flow
          playerMgr().sendDebug( player, "RetainerDesk: yieldId 3 - LoadRetainerCreation - returning 0 (success)" );
          eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 0 } );
          break;

        case 5:  // CreateRetainer - called after CharaMake with name and personality
        {
          playerMgr().sendDebug( player, "RetainerDesk: yieldId 5 - CreateRetainer - name: '{}', personality: {}",
                         resultString, resultInt );
          
          // Get the customize data stored from RetainerCustomize packet
          if( !player.hasPendingRetainerCustomize() )
          {
            playerMgr().sendDebug( player, "RetainerDesk: ERROR - No pending customize data!" );
            eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 1 } ); // Return error
            break;
          }
          
          // Convert customize data to vector
          const uint8_t* customizePtr = player.getPendingRetainerCustomize();
          std::vector< uint8_t > customize( customizePtr, customizePtr + 26 );
          
          // Get personality from resultInt (1-6)
          auto personality = static_cast< World::Manager::RetainerPersonality >( 
            resultInt > 0 && resultInt <= 6 ? resultInt : 1 );
          
          // Create the retainer
          auto& retainerMgr = Common::Service< World::Manager::RetainerMgr >::ref();
          auto retainerId = retainerMgr.createRetainer( player, resultString, personality, customize );
          
          // Clear pending data
          player.clearPendingRetainerCustomize();
          
          if( retainerId.has_value() )
          {
            playerMgr().sendDebug( player, "RetainerDesk: Created retainer '{}' with ID {}", 
                           resultString, retainerId.value() );
            eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 0 } ); // Success
          }
          else
          {
            playerMgr().sendDebug( player, "RetainerDesk: Failed to create retainer '{}'", resultString );
            eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 1 } ); // Error
          }
          break;
        }

        case 24: // GetContentFinderStatus - check if player is in duty finder queue
          // Return 0 = not in queue, allows creation to proceed
          playerMgr().sendDebug( player, "RetainerDesk: yieldId 24 - GetContentFinderStatus - returning 0 (not in queue)" );
          eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 0 } );
          break;

        case 25: // SetCharaMakeCondition - setup CharaMake mode
          // Return 1 = success, 0 = failure (would abort)
          playerMgr().sendDebug( player, "RetainerDesk: yieldId 25 - SetCharaMakeCondition - returning 1 (success)" );
          eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 1 } );
          break;

        case 36: // PromptName - client calls GetAvailableRetainerSlotsSync() which yields here
        {
          // The client's GetAvailableRetainerSlotsSync() yields with ID 36 and expects
          // the return value to be the number of available retainer slots.
          // The Lua check is: if GetAvailableRetainerSlotsSync() <= GetRetainerEmployedCount() then FAIL
          // So we must return a value > currentCount to allow hiring.
          auto& retainerMgr = Common::Service< World::Manager::RetainerMgr >::ref();
          uint8_t maxRetainers = retainerMgr.getMaxRetainerSlots( player );
          uint8_t currentCount = retainerMgr.getRetainerCount( player );
          
          // Return maxRetainers so that: maxRetainers > currentCount allows hiring
          playerMgr().sendDebug( player, "RetainerDesk: yieldId 36 - GetAvailableRetainerSlotsSync - returning {} (max) to pass check vs employed={}",
                         maxRetainers, currentCount );
          eventMgr().resumeScene( player, eventId, sceneId, yieldId, { maxRetainers } );
          break;
        }

        default:
          // Unknown yield in Scene 1 - return 1 for success
          playerMgr().sendDebug( player, "RetainerDesk: Unknown yieldId {} in Scene 1 - returning 1", yieldId );
          eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 1 } );
          break;
      }
    }
    else
    {
      // Fallback for other scenes
      playerMgr().sendDebug( player, "RetainerDesk: Unhandled yield - scene: {}, yieldId: {} - returning 1", sceneId, yieldId );
      eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 1 } );
    }
  }

  //////////////////////////////////////////////////////////////////////
  // Scenes
  //////////////////////////////////////////////////////////////////////

  void Scene00000( Entity::Player& player )
  {
    eventMgr().playScene( player, getId(), 0, HIDE_HOTBAR, bindSceneReturn( &CmnDefRetainerDesk::Scene00000Return ) );
  }

  void Scene00000Return( Entity::Player& player, const Event::SceneResult& result )
  {
    auto selection = result.getResult( 0 );
    playerMgr().sendDebug( player, "RetainerDesk: Scene00000Return - selection={}", selection );
    
    if( selection == 1 )
    {
      // Hire retainer
      Scene00001( player );
    }
    else if( selection == 6 || selection == 0 )
    {
      // Cancel (selection 6 is "Nothing" in menu, 0 is cancel/escape)
      playerMgr().sendDebug( player, "RetainerDesk: Finishing event (cancel)" );
      eventMgr().eventFinish( player, getId(), 1 );
    }
    else
    {
      // Other options not implemented yet - just finish
      playerMgr().sendDebug( player, "RetainerDesk: Unhandled selection {} - finishing event", selection );
      eventMgr().eventFinish( player, getId(), 1 );
    }
  }

  //////////////////////////////////////////////////////////////////////

  void Scene00001( Entity::Player& player )
  {
    // Pass { 1 } to Scene 1 to indicate we want CharaMake mode
    eventMgr().playScene( player, getId(), 1, HIDE_HOTBAR, { 1 }, bindSceneReturn( &CmnDefRetainerDesk::Scene00001Return ) );
  }

  void Scene00001Return( Entity::Player& player, const Event::SceneResult& result )
  {
    auto status = result.getResult( 0 );
    playerMgr().sendDebug( player, "RetainerDesk: Scene00001Return - status={}", status );
    
    // The Lua script returns a scene number to chain to:
    // 11 = Creation success - show success dialogue
    // -1 = Cancelled or error
    // 0  = Generic success
    if( status == 11 )
    {
      // Play Scene 11 - retainer creation success dialogue
      Scene00011( player );
    }
    else
    {
      // Cancelled or error - finish the event
      eventMgr().eventFinish( player, getId(), 1 );
    }
  }

  //////////////////////////////////////////////////////////////////////

  void Scene00011( Entity::Player& player )
  {
    // Scene 11: Retainer creation success dialogue
    // The Lua script will display success messages and log the hire
    playerMgr().sendDebug( player, "RetainerDesk: Playing Scene00011 (creation success)" );
    eventMgr().playScene( player, getId(), 11, HIDE_HOTBAR, bindSceneReturn( &CmnDefRetainerDesk::Scene00011Return ) );
  }

  void Scene00011Return( Entity::Player& player, const Event::SceneResult& result )
  {
    playerMgr().sendDebug( player, "RetainerDesk: Scene00011Return - finishing event" );
    // After success dialogue, return to main menu
    Scene00000( player );
  }
};

EXPOSE_SCRIPT( CmnDefRetainerDesk );
