#include "ScriptLogger.h"
#include <Actor/Player.h>
#include <Network/Util/PacketUtil.h>
#include <ScriptObject.h>

using namespace Sapphire;
using namespace Sapphire::ScriptAPI;

class RentalChocoboCommandHandler : public CommandScript
{
public:
  RentalChocoboCommandHandler() : CommandScript( 0xC9 )// FINISH_LOADING command
  {
    ScriptLogger::debug( "RentalChocoboCommandHandler initialized" );
  }

  void onCommand( Entity::Player& player, uint16_t commandId, uint64_t param1, uint64_t param2, uint64_t param3, uint64_t param4 ) override
  {
    // Only handle FINISH_LOADING command (0xC9)
    if( commandId != 0xC9 )
      return;

    ScriptLogger::debug( "FINISH_LOADING command handler for player {0}, persistentMount={1}",
                         player.getId(), player.getPersistentMount() );

    uint32_t persistentMount = player.getPersistentMount();
    if( persistentMount > 0 )
    {
      ScriptLogger::debug( "Found persistent mount ID: {0}, applying mount after zone load", persistentMount );

      // Make sure to set status to Mounted first
      player.setStatus( static_cast< Common::ActorStatus >( Common::ActorStatus::Mounted ) );

      // Apply the mount - this will send the necessary packets
      player.setMount( persistentMount );

      // Optional: clear persistent mount if it should only apply once
      // player.setPersistentMount(0);

      ScriptLogger::debug( "Mount applied after zone load: mountId={0}, status={1}",
                           player.getCurrentMount(), static_cast< int >( player.getStatus() ) );
    }
  }
};

EXPOSE_SCRIPT( RentalChocoboCommandHandler );