#include "ScriptLogger.h"
#include <Actor/Player.h>
#include <Exd/ExdData.h>
#include <Network/Util/PacketUtil.h>
#include <ScriptObject.h>
#include <Service.h>

// Namespace with utility functions for chocobo rental scripts
using namespace Sapphire::ScriptAPI;
namespace Sapphire::ScriptAPI::ChocoboRental
{
  // Constants that match the Lua implementation
  constexpr uint8_t CLASS_LEVEL = 10;// Minimum level requirement
  constexpr uint32_t PRICE = 80;     // Gil cost for rental
  constexpr uint8_t MOUNT_ID = 1;    // Standard company chocobo mount ID

  // Scene IDs
  constexpr uint16_t SCENE0 = 0;// Main dialog scene
  constexpr uint16_t SCENE1 = 1;// Post-warp scene (handles fade in)
  constexpr uint16_t SCENE2 = 2;// Error scene (not enough gil, etc.)

  /**
   * Check if player meets the class level requirements for chocobo rental
   * Matches checkClassLevel in cmndefrentalchocobo_00012.luab.luab
   */
  inline bool checkClassLevel( Entity::Player& player )
  {
    // In the Lua script, it checks GetClassLevelMax() >= CLASS_LEVEL
    return player.getLevel() >= CLASS_LEVEL;
  }

  /**
   * Check if player has enough gil for chocobo rental
   * Matches checkGil in cmndefrentalchocobo_00012.luab.luab
   */
  inline bool checkGil( Entity::Player& player )
  {
    return player.getCurrency( Common::CurrencyType::Gil ) >= PRICE;
  }

  /**
   * Check if player is qualified for chocobo rental (level and gil)
   * Matches isQualified in cmndefrentalchocobo_00012.luab.luab
   */
  inline bool isQualified( Entity::Player& player )
  {
    return checkClassLevel( player ) && checkGil( player );
  }

  /**
   * Process payment for chocobo rental
   * Helper function for rental scripts
   */
  inline void processPayment( Entity::Player& player )
  {
    if( checkGil( player ) )
    {
      player.removeCurrency( Common::CurrencyType::Gil, PRICE );
      ScriptLogger::debug( "[ChocoboRental] Charged player {0} gil for rental", PRICE );
    }
  }

  /**
   * Apply mount state to player
   * Helper function for post-warp mounting
   */
  inline void applyMountState( Entity::Player& player )
  {
    ScriptLogger::debug( "[ChocoboRental] Applying mount state to player" );

    // First reset to idle state for clean mounting
    player.setStatus( static_cast< Common::ActorStatus >( 1 ) );// ActorStatus::Idle

    // Apply mount state
    player.setMount( MOUNT_ID );
    player.setStatus( static_cast< Common::ActorStatus >( 4 ) );// ActorStatus::Mounted

    // Send mount packets
    Network::Util::Packet::sendMount( player );
  }

  /**
   * Set the director ID flag for post-warp mounting
   * Helper function for marking players for mounting after zone transition
   */
  inline void setMountFlag( Entity::Player& player )
  {
    // Using 1001 as a custom flag to indicate rental chocobo
    player.setDirectorId( 1001 );
  }

  /**
   * Check if player has the mount flag set
   */
  inline bool hasMountFlag( Entity::Player& player )
  {
    return player.getDirectorId() == 1001;
  }

  /**
   * Clear the mount flag
   */
  inline void clearMountFlag( Entity::Player& player )
  {
    player.setDirectorId( 0 );
  }
}// namespace Sapphire::ScriptAPI::ChocoboRental

// The actual script class that will be registered with EXPOSE_SCRIPT
// Change class name to match the EXPOSE_SCRIPT name (all lowercase)
class cmndefrentalchocobo : public Sapphire::ScriptAPI::EventScript
{
public:
  cmndefrentalchocobo() : Sapphire::ScriptAPI::EventScript( 1179650 )
  {
    ScriptLogger::debug( "[cmndefrentalchocobo] Utility script initialized" );
  }
};

// Script name is now consistent with the class name
EXPOSE_SCRIPT( cmndefrentalchocobo );