#include "Script/NativeScriptApi.h"
#include "ScriptLogger.h"
#include <Actor/Player.h>
#include <Event/EventDefs.h>
#include <Manager/ChocoboMgr.h>// Include the new manager
#include <Manager/EventMgr.h>
#include <ScriptObject.h>
#include <Service.h>

using namespace Sapphire;
using namespace Sapphire::ScriptAPI;

class WarpRentalChocobo : public Sapphire::ScriptAPI::EventScript
{
public:
  WarpRentalChocobo() : Sapphire::ScriptAPI::EventScript( 131073 ) {}

  void onTalk( uint32_t eventId, Entity::Player& player, uint64_t actorId ) override
  {
    // Force eventId for testing
    eventId = 131073;

    ScriptLogger::debug( "WarpRentalChocobo::onTalk called with eventId: {}", eventId );
    if( eventId == 0 || eventId != 131073 )
    {
      ScriptLogger::error( "WarpRentalChocobo: Invalid eventId {}! Expected 131073.", eventId );
      return;
    }

    if( !player.getEvent( eventId ) )
    {
      eventMgr().eventStart( player, actorId, eventId, Event::EventHandler::Nest, 0, 0 );
    }

    // Start the main rental scene
    Scene00000( player, eventId );
  }

private:
  void Scene00000( Entity::Player& player, uint32_t eventId )
  {
    eventMgr().playScene( player, eventId, 0, HIDE_HOTBAR,
                          [ this, eventId ]( Entity::Player& player, const Event::SceneResult& result ) {
                            Scene00000Return( player, eventId, result );
                          } );
  }

  void Scene00000Return( Entity::Player& player, uint32_t eventId, const Event::SceneResult& result )
  {
    // Check if the Lua scene returned 1 (rental accepted and fade-out completed)
    if( result.getResult( 0 ) == 1 )
    {
      ScriptLogger::debug( "WarpRentalChocobo: Scene returned 1, triggering warp." );

      // Use the ChocoboMgr to handle the warp and mounting
      World::Manager::ChocoboMgr::triggerRentalWarp( player, 2040811 );
    }
    else
    {
      ScriptLogger::debug( "WarpRentalChocobo: Scene returned {}, no warp triggered.", result.getResult( 0 ) );
    }
  }
};

EXPOSE_SCRIPT( WarpRentalChocobo );