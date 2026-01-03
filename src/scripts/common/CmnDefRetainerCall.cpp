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

  static constexpr uint8_t YIELD_SELECT_RETAINER = 7;
  static constexpr uint8_t YIELD_DEPOP_RETAINER = 8;
  static constexpr uint8_t YIELD_RETAINER_MAIN_MENU = 12;
  static constexpr uint8_t YIELD_RETAINER_MAIN_MENU_AFTER = 13;
  static constexpr uint8_t YIELD_CALL_RETAINER = 17;
  static constexpr uint8_t YIELD_SET_VENTURE_TUTORIAL_FLAG = 35;

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
          eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 444 } );
        else
          eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 1 } );
        break;
      }

      case YIELD_CALL_RETAINER:
      {
        // TODO: Spawn retainer actor with PlayerSpawn packet (0x0190)
        eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 1 } ); // 1 = failure
        break;
      }

      case YIELD_DEPOP_RETAINER:
        eventMgr().resumeScene( player, eventId, sceneId, yieldId, { 0 } );
        break;

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
