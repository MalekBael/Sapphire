/**
 * @file CmnDefRetainerCall.cpp
 * @brief Server-side implementation of the Retainer Call event (summoning a retainer)
 * 
 * Event ID: 720906 (0x000B000A)
 * Client Script: cmndefretainercall_00010.luab
 */

#include <ScriptObject.h>
#include <Actor/Player.h>
#include <Manager/RetainerMgr.h>
#include <Service.h>
#include <cstdlib>

using namespace Sapphire;

class CmnDefRetainerCall : public Sapphire::ScriptAPI::EventScript
{
public:
  CmnDefRetainerCall() : Sapphire::ScriptAPI::EventScript( 720906 )
  {
  }

  void onTalk( uint32_t eventId, Entity::Player& player, uint64_t actorId ) override
  {
    Scene00000( player );
  }

  // Yield IDs from Lua script (cmndefretainercall_00010.luab)
  static constexpr uint8_t YIELD_CALL_RETAINER = 7;           // CallRetainer(slot) - spawns retainer
  static constexpr uint8_t YIELD_DEPOP_RETAINER = 8;          // DepopRetainer() - despawns retainer
  static constexpr uint8_t YIELD_RETAINER_MAIN_MENU = 12;     // RetainerMainMenu()
  static constexpr uint8_t YIELD_RETAINER_MAIN_MENU_AFTER = 13; // RetainerMainMenuAfter()
  static constexpr uint8_t YIELD_SELECT_RETAINER = 17;        // SelectRetainer(count)
  static constexpr uint8_t YIELD_SET_VENTURE_TUTORIAL_FLAG = 35; // SetVentureTutorialFlag()

  void onYield( uint32_t eventId, uint16_t sceneId, uint8_t yieldId, Entity::Player& player,
                const std::string& resultString, uint64_t resultInt ) override
  {
    auto& retainerMgr = Common::Service< World::Manager::RetainerMgr >::ref();

    switch( yieldId )
    {
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
          eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 1 } );
        }
        break;
      }

      case YIELD_CALL_RETAINER:
      {
        // resultInt packs both args: low 32 bits = slot, high 32 bits = retainerId
        uint8_t retainerSlot = static_cast< uint8_t >( resultInt & 0xFF );
        uint64_t retainerId = static_cast< uint64_t >( resultInt >> 32 );
        
        // If retainerId is provided directly, use it; otherwise fall back to slot lookup
        if( retainerId == 0 )
        {
          // Fall back to slot-based lookup
          auto retainers = retainerMgr.getRetainers( player );
          for( const auto& retainer : retainers )
          {
            if( retainer.displayOrder == retainerSlot )
            {
              retainerId = retainer.retainerId;
              break;
            }
          }
        }

        if( retainerId == 0 )
        {
          // No retainer found - Lua expects { result, actorId, greetingType }
          eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 1, 0, 0 } );
          break;
        }

        // Spawn the retainer NPC
        uint32_t actorId = retainerMgr.spawnRetainer( player, retainerId );
        
        if( actorId == 0 )
        {
          // Spawn failed - Lua expects { result, actorId, greetingType }
          eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 1, 0, 0 } );
        }
        else
        {
          // Spawn successful - Lua expects { result, actorId, greetingType }
          // result=0 (success), actorId for BindRetainer, greetingType 0-2 for greeting variant
          uint8_t greetingType = static_cast< uint8_t >( std::rand() % 3 ); // Random greeting (0, 1, or 2)
          eventMgr().resumeScene( player, eventId, sceneId, yieldId, 
            { 0, static_cast< uint64_t >( actorId ), greetingType } );
        }
        break;
      }

      case YIELD_DEPOP_RETAINER:
      {
        // Despawn any currently spawned retainers for this player
        // For now, we'll despawn all retainers - could be more specific based on resultInt
        auto retainers = retainerMgr.getRetainers( player );
        for( const auto& retainer : retainers )
        {
          uint32_t actorId = retainerMgr.getSpawnedRetainerActorId( player, retainer.retainerId );
          if( actorId != 0 )
          {
            retainerMgr.despawnRetainer( player, actorId );
          }
        }
        
        eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 0 } );
        break;
      }

      case YIELD_RETAINER_MAIN_MENU:
        eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 11 } );
        break;

      case YIELD_RETAINER_MAIN_MENU_AFTER:
        eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 1, 1 } );
        break;

      case YIELD_SET_VENTURE_TUTORIAL_FLAG:
        eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 1 } );
        break;

      default:
        eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 1 } );
        break;
    }
  }

  void Scene00000( Entity::Player& player )
  {
    eventMgr().playScene( player, getId(), 0, HIDE_HOTBAR, bindSceneReturn( &CmnDefRetainerCall::Scene00000Return ) );
  }

  void Scene00000Return( Entity::Player& player, const Event::SceneResult& result )
  {
    eventMgr().eventFinish( player, getId(), 1 );
  }
};

EXPOSE_SCRIPT( CmnDefRetainerCall );
