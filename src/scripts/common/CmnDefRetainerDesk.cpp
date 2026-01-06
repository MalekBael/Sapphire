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
 * 
 * Yield IDs (Scene 2 - Release Retainer):
 *   7  = SelectRetainer - opens retainer selection UI, returns index (1-8) or 444
 *   8  = SetCurrentRetainerIndex - sets the selected retainer context
 *   9  = IsCurrentRetainerActive - checks if retainer is active (not suspended)
 *   10 = GetRetainerName - returns name, sex, hasHouse
 *   34 = RemoveRetainer - deletes the retainer, returns 0 success or error code
 * 
 * Yield IDs (Scene 7 - Market Tax Rates):
 *   38 = GetMarketSellTaxRates - returns 5 values: limsa, gridania, uldah, ishgard, taxInterval
 * 
 * NOTE: Yield IDs marked with (?) need verification via debug logging
 */

#include <ScriptObject.h>
#include <Actor/Player.h>
#include <Manager/PlayerMgr.h>
#include <Manager/RetainerMgr.h>
#include <Service.h>

using namespace Sapphire;

class CmnDefRetainerDesk : public Sapphire::ScriptAPI::EventScript
{
private:
  // Per-player state for retainer operations (stored on player object would be better, but this works for now)
  // In a real implementation, this should be stored per-player in the event handler context
  uint8_t m_selectedRetainerIndex{ 0 };
  uint64_t m_selectedRetainerId{ 0 };

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
    else if( sceneId == 2 )
    {
      // Scene 2: Release (dismiss) retainer flow
      handleScene02Yield( eventId, yieldId, player, resultString, resultInt );
    }
    else if( sceneId == 7 )
    {
      // Scene 7: View market tax rates
      handleScene07Yield( eventId, yieldId, player, resultString, resultInt );
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
    
    switch( selection )
    {
      case 1: // Hire retainer
        Scene00001( player );
        break;
      
      case 3: // Release (dismiss) retainer
        Scene00002( player );
        break;
      
      case 5: // View market tax rates
        Scene00007( player );
        break;
      
      case 6: // Cancel / Nothing
      case 0: // Escape
        playerMgr().sendDebug( player, "RetainerDesk: Finishing event (cancel)" );
        eventMgr().eventFinish( player, getId(), 1 );
        break;
      
      default:
        // Other options not implemented yet - just finish
        playerMgr().sendDebug( player, "RetainerDesk: Unhandled selection {} - finishing event", selection );
        eventMgr().eventFinish( player, getId(), 1 );
        break;
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

  //////////////////////////////////////////////////////////////////////
  // Scene 00002 - Release (Dismiss) Retainer
  //////////////////////////////////////////////////////////////////////

  void Scene00002( Entity::Player& player )
  {
    playerMgr().sendDebug( player, "RetainerDesk: Playing Scene00002 (release retainer)" );
    eventMgr().playScene( player, getId(), 2, HIDE_HOTBAR, bindSceneReturn( &CmnDefRetainerDesk::Scene00002Return ) );
  }

  void Scene00002Return( Entity::Player& player, const Event::SceneResult& result )
  {
    auto status = result.getResult( 0 );
    playerMgr().sendDebug( player, "RetainerDesk: Scene00002Return - status={}", status );
    
    // 0 = success (retainer released), -1 = cancelled/error
    // Return to main menu regardless
    Scene00000( player );
  }

  /**
   * @brief Handle yields for Scene 2 (Release Retainer)
   * 
   * The Lua flow is:
   * 1. SelectRetainer() - opens retainer list UI
   * 2. SetCurrentRetainerIndex(idx) - sets context
   * 3. IsCurrentRetainerActive() - checks status
   * 4. GetRetainerName() - gets name for confirmation
   * 5. RemoveRetainer(idx) - actually deletes
   */
  void handleScene02Yield( uint32_t eventId, uint8_t yieldId, Entity::Player& player,
                           const std::string& resultString, uint64_t resultInt )
  {
    auto& retainerMgr = Common::Service< World::Manager::RetainerMgr >::ref();
    
    // Log all yields for discovery
    playerMgr().sendDebug( player, "RetainerDesk Scene02: yieldId={} resultInt={}", yieldId, resultInt );
    
    switch( yieldId )
    {
      case 7:  // SelectRetainer - open retainer selection UI
      {
        // Send fresh retainer list so the UI has data
        retainerMgr.sendRetainerList( player );
        
        uint8_t retainerCount = retainerMgr.getRetainerCount( player );
        if( retainerCount == 0 )
        {
          // No retainers to select - return 444 (special "none available" code)
          playerMgr().sendDebug( player, "RetainerDesk: SelectRetainer - no retainers, returning 444" );
          eventMgr().resumeScene( player, eventId, 2, yieldId, { 444 } );
        }
        else
        {
          // For now, auto-select first retainer (index 1 in Lua is 1-based)
          // TODO: This should open a UI and wait for player selection
          // The client likely handles this via RetainerList UI interaction
          playerMgr().sendDebug( player, "RetainerDesk: SelectRetainer - {} retainers available, returning 1", retainerCount );
          eventMgr().resumeScene( player, eventId, 2, yieldId, { 1 } );
        }
        break;
      }

      case 8:  // SetCurrentRetainerIndex - store the selected retainer
      {
        m_selectedRetainerIndex = static_cast< uint8_t >( resultInt );
        playerMgr().sendDebug( player, "RetainerDesk: SetCurrentRetainerIndex - idx={}", m_selectedRetainerIndex );
        
        // Get the retainer ID for this index
        auto retainers = retainerMgr.getRetainers( player );
        if( m_selectedRetainerIndex > 0 && m_selectedRetainerIndex <= retainers.size() )
        {
          m_selectedRetainerId = retainers[ m_selectedRetainerIndex - 1 ].retainerId;
          playerMgr().sendDebug( player, "RetainerDesk: Selected retainer ID={}", m_selectedRetainerId );
        }
        
        eventMgr().resumeScene( player, eventId, 2, yieldId, { 0 } ); // Success
        break;
      }

      case 9:  // IsCurrentRetainerActive - check if retainer is active
      {
        // Return 1 = active, 0 = suspended/inactive
        auto retainer = retainerMgr.getRetainer( m_selectedRetainerId );
        uint32_t isActive = ( retainer.has_value() && retainer->isActive ) ? 1 : 0;
        playerMgr().sendDebug( player, "RetainerDesk: IsCurrentRetainerActive - id={} active={}", 
                               m_selectedRetainerId, isActive );
        eventMgr().resumeScene( player, eventId, 2, yieldId, { isActive } );
        break;
      }

      case 10: // GetRetainerName - returns name, sex(?), hasHouse
      {
        auto retainer = retainerMgr.getRetainer( m_selectedRetainerId );
        if( retainer.has_value() )
        {
          // The Lua expects: name, sex(?), hasHouse
          // For now: name = string result, sex = 0 (unknown), hasHouse = false
          playerMgr().sendDebug( player, "RetainerDesk: GetRetainerName - name='{}' id={}", 
                                 retainer->name, m_selectedRetainerId );
          // This should be a string yield, not integer - return 0/0/0 for the integer parts
          // The name is passed via resultString in reverse direction
          // For now, just return success - the client probably gets the name from RetainerInfo packet
          eventMgr().resumeScene( player, eventId, 2, yieldId, { 0, 0, 0 } );
        }
        else
        {
          playerMgr().sendDebug( player, "RetainerDesk: GetRetainerName - retainer not found!" );
          eventMgr().resumeScene( player, eventId, 2, yieldId, { 0, 0, 0 } );
        }
        break;
      }

      case 34: // RemoveRetainer - actually delete the retainer
      {
        playerMgr().sendDebug( player, "RetainerDesk: RemoveRetainer - deleting retainer idx={} id={}", 
                               m_selectedRetainerIndex, m_selectedRetainerId );
        
        auto error = retainerMgr.deleteRetainer( player, m_selectedRetainerId );
        
        if( error == World::Manager::RetainerError::None )
        {
          playerMgr().sendDebug( player, "RetainerDesk: RemoveRetainer - success" );
          // Send updated retainer list
          retainerMgr.sendRetainerList( player );
          eventMgr().resumeScene( player, eventId, 2, yieldId, { 0 } ); // Success
        }
        else if( error == World::Manager::RetainerError::RetainerBusy )
        {
          // Error codes 1879048199-1879048204 in Lua indicate inventory not empty
          playerMgr().sendDebug( player, "RetainerDesk: RemoveRetainer - retainer busy (has items/ventures)" );
          eventMgr().resumeScene( player, eventId, 2, yieldId, { 1879048199 } );
        }
        else
        {
          playerMgr().sendDebug( player, "RetainerDesk: RemoveRetainer - failed with error {}", 
                                 static_cast< uint32_t >( error ) );
          eventMgr().resumeScene( player, eventId, 2, yieldId, { 1 } ); // Generic error
        }
        
        // Clear selection
        m_selectedRetainerIndex = 0;
        m_selectedRetainerId = 0;
        break;
      }

      default:
        playerMgr().sendDebug( player, "RetainerDesk: Unknown Scene02 yieldId {} - returning 1", yieldId );
        eventMgr().resumeScene( player, eventId, 2, yieldId, { 1 } );
        break;
    }
  }

  //////////////////////////////////////////////////////////////////////
  // Scene 00007 - View Market Tax Rates
  //////////////////////////////////////////////////////////////////////

  void Scene00007( Entity::Player& player )
  {
    playerMgr().sendDebug( player, "RetainerDesk: Playing Scene00007 (market tax rates)" );
    eventMgr().playScene( player, getId(), 7, HIDE_HOTBAR, bindSceneReturn( &CmnDefRetainerDesk::Scene00007Return ) );
  }

  void Scene00007Return( Entity::Player& player, const Event::SceneResult& result )
  {
    auto selection = result.getResult( 0 );
    playerMgr().sendDebug( player, "RetainerDesk: Scene00007Return - selection={}", selection );
    
    // The menu returns:
    // 1 = Limsa selected
    // 2 = Gridania selected
    // 3 = Ul'dah selected
    // 4 = Ishgard selected
    // 5 = About tax rates (explanation)
    // 6 = Back
    
    if( selection >= 1 && selection <= 5 )
    {
      // Stay in the market tax menu (loops in Lua)
      Scene00007( player );
    }
    else
    {
      // Back to main menu
      Scene00000( player );
    }
  }

  /**
   * @brief Handle yields for Scene 7 (Market Tax Rates)
   * 
   * The Lua calls GetMarketSellTaxRates() which returns 5 values:
   * - limsa rate (0-5%, -1 if not registered)
   * - gridania rate
   * - uldah rate  
   * - ishgard rate
   * - tax interval (hours until reset)
   */
  void handleScene07Yield( uint32_t eventId, uint8_t yieldId, Entity::Player& player,
                           const std::string& resultString, uint64_t resultInt )
  {
    playerMgr().sendDebug( player, "RetainerDesk Scene07: yieldId={}", yieldId );
    
    switch( yieldId )
    {
      case 38: // GetMarketSellTaxRates (assumed yield ID - verify via debug log)
      {
        // Return 5 values: limsa, gridania, uldah, ishgard, taxInterval
        // Tax rate 0-4 = that percentage, 5+ or -1 = not registered
        // For now, return default 5% tax (max) for all cities = "no discount"
        // TODO: Implement actual tax rate tracking based on player's sales volume
        
        uint32_t limsaTax = 5;      // 5% = no discount
        uint32_t gridaniaTax = 5;   // 5% = no discount
        uint32_t uldahTax = 5;      // 5% = no discount
        uint32_t ishgardTax = 5;    // 5% = no discount
        uint32_t taxInterval = 168; // Hours until weekly reset (7 days)
        
        playerMgr().sendDebug( player, "RetainerDesk: GetMarketSellTaxRates - returning {}/{}/{}/{} interval={}", 
                               limsaTax, gridaniaTax, uldahTax, ishgardTax, taxInterval );
        eventMgr().resumeScene( player, eventId, 7, yieldId, { limsaTax, gridaniaTax, uldahTax, ishgardTax, taxInterval } );
        break;
      }

      default:
        playerMgr().sendDebug( player, "RetainerDesk: Unknown Scene07 yieldId {} - returning default tax values", yieldId );
        // Return default tax rates anyway
        eventMgr().resumeScene( player, eventId, 7, yieldId, { 5, 5, 5, 5, 168 } );
        break;
    }
  }
};

EXPOSE_SCRIPT( CmnDefRetainerDesk );
