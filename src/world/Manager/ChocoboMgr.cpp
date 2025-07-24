#include "ChocoboMgr.h"
#include "PlayerMgr.h"
#include "TerritoryMgr.h"
#include "WarpMgr.h"

#include "Territory/Territory.h"
#include <Exd/ExdData.h>
#include <Logging/Logger.h>
#include <Network/CommonActorControl.h>
#include <Network/GamePacket.h>
#include <Network/PacketDef/Zone/ServerZoneDef.h>
#include <Network/PacketWrappers/ActorControlPacket.h>
#include <Network/PacketWrappers/ActorControlSelfPacket.h>
#include <Network/PacketWrappers/PlayerSpawnPacket.h>
#include <Network/Util/PacketUtil.h>
#include <Service.h>
#include <WorldServer.h>

#include "Manager/ChocoboMgr.h" 

using namespace Sapphire;
using namespace Sapphire::World::Manager;
using namespace Sapphire::Network::Packets::WorldPackets::Server;

// Static member definition
std::unordered_set< uint32_t > ChocoboMgr::s_pendingRentalMounts;
std::unordered_map< uint32_t, uint32_t > ChocoboMgr::s_persistentMounts;

void ChocoboMgr::addPendingRentalMount( uint32_t playerId )
{
  s_pendingRentalMounts.insert( playerId );
  Logger::debug( "ChocoboMgr: Added pending rental mount for player {}", playerId );
}

bool ChocoboMgr::hasPendingRentalMount( uint32_t playerId )
{
  return s_pendingRentalMounts.count( playerId ) > 0;
}

void ChocoboMgr::removePendingRentalMount( uint32_t playerId )
{
  s_pendingRentalMounts.erase( playerId );
  Logger::debug( "ChocoboMgr: Removed pending rental mount for player {}", playerId );
}

void ChocoboMgr::addPersistentMount( uint32_t playerId, uint32_t mountId )
{
  s_persistentMounts[ playerId ] = mountId;
  Logger::debug( "ChocoboMgr: Added persistent mount {} for player {}", mountId, playerId );
}

bool ChocoboMgr::hasPersistentMount( uint32_t playerId )
{
  return s_persistentMounts.count( playerId ) > 0;
}

uint32_t ChocoboMgr::getPersistentMount( uint32_t playerId )
{
  auto it = s_persistentMounts.find( playerId );
  if( it != s_persistentMounts.end() )
    return it->second;
  return 0;
}

void ChocoboMgr::removePersistentMount( uint32_t playerId )
{
  s_persistentMounts.erase( playerId );
  Logger::debug( "ChocoboMgr: Removed persistent mount for player {}", playerId );
}

void ChocoboMgr::checkAndMountPlayer( Entity::Player& player )
{
  // Check for pending rental mount first (from rental warp)
  if( hasPendingRentalMount( player.getCharacterId() ) )
  {
    Logger::debug( "ChocoboMgr: Player {} has pending rental mount, waiting for FINISH_LOADING", player.getCharacterId() );
    return;
  }

  // Check for persistent mount (from zone change)
  if( hasPersistentMount( player.getCharacterId() ) )
  {
    Logger::debug( "ChocoboMgr: Player {} has persistent mount, waiting for FINISH_LOADING", player.getCharacterId() );
    return;
  }
}

void ChocoboMgr::checkAndMountPlayerAfterLoading( Entity::Player& player )
{
  // Check for pending rental mount first (from rental warp)
  if( hasPendingRentalMount( player.getCharacterId() ) )
  {
    Logger::debug( "ChocoboMgr: FINISH_LOADING received - mounting pending rental chocobo for player {}", player.getCharacterId() );
    mountRentalChocobo( player );
    removePendingRentalMount( player.getCharacterId() );

    // Add to persistent mounts so it persists through future zone changes
    addPersistentMount( player.getCharacterId(), 1 );
    return;
  }

  // Check for persistent mount (from zone change)
  if( hasPersistentMount( player.getCharacterId() ) )
  {
    uint32_t mountId = getPersistentMount( player.getCharacterId() );
    Logger::debug( "ChocoboMgr: FINISH_LOADING received - restoring persistent mount {} for player {}", mountId, player.getCharacterId() );
    mountRentalChocobo( player, mountId );
    return;
  }
}

void ChocoboMgr::onPlayerZoneChange( Entity::Player& player )
{
  // Called when a player is about to change zones
  // Check if they have a mount that should persist
  if( player.getCurrentMount() != 0 )
  {
    uint32_t currentMount = player.getCurrentMount();
    Logger::debug( "ChocoboMgr: Player {} changing zones with mount {}, adding to persistent mounts",
                   player.getCharacterId(), currentMount );

    // Add to persistent mounts so it gets restored after zone change
    addPersistentMount( player.getCharacterId(), currentMount );
  }
}

void ChocoboMgr::onPlayerDismount( Entity::Player& player )
{
  // Called when a player manually dismounts
  // Remove from persistent mounts so it doesn't get restored
  if( hasPersistentMount( player.getCharacterId() ) )
  {
    Logger::debug( "ChocoboMgr: Player {} manually dismounted, removing from persistent mounts",
                   player.getCharacterId() );
    removePersistentMount( player.getCharacterId() );
  }
}

void ChocoboMgr::mountRentalChocobo( Entity::Player& player, uint32_t chocoboId )
{
  Logger::debug( "ChocoboMgr: Starting mount process for player {} after FINISH_LOADING", player.getCharacterId() );

  // Clear any existing mount first
  if( player.getCurrentMount() != 0 )
  {
    Logger::debug( "ChocoboMgr: Clearing existing mount {}", player.getCurrentMount() );
    player.setMount( 0 );
  }

  // Set the correct mount equipment for rental chocobo
  if( chocoboId == 1 )
  {
    // Use correct equipment values for rental chocobo
    player.setMountEquipment( 1, 0, 1, 0 );// head=1, body=0, leg=1, stain=0
    Logger::debug( "ChocoboMgr: Set rental chocobo equipment (1,0,1,0)" );
  }

  // Set player status to Mounted
  player.setStatus( Common::ActorStatus::Mounted );
  Logger::debug( "ChocoboMgr: Set player status to Mounted" );

  // Set the mount ID
  player.setMount( chocoboId );
  Logger::debug( "ChocoboMgr: Set mount ID to {} - Current mount: {}", chocoboId, player.getCurrentMount() );

  auto& server = Common::Service< World::WorldServer >::ref();

  // Send mount packet to the player
  auto mountPacket = Sapphire::Network::Packets::makeZonePacket< FFXIVIpcMount >( player.getId() );
  mountPacket->data().id = chocoboId;
  server.queueForPlayer( player.getCharacterId(), mountPacket );
  Logger::debug( "ChocoboMgr: Sent FFXIVIpcMount packet with id: {}", chocoboId );

  // Send status update to the player
  auto setStatusPacket = std::make_shared< ActorControlPacket >( player.getId(),
                                                                 Sapphire::Network::ActorControl::SetStatus, static_cast< uint8_t >( Common::ActorStatus::Mounted ) );
  server.queueForPlayer( player.getCharacterId(), setStatusPacket );
  Logger::debug( "ChocoboMgr: Sent SetStatus ActorControl packet to player" );

  Logger::debug( "ChocoboMgr: Successfully mounted chocobo {} for player {}", chocoboId, player.getCharacterId() );
}

void ChocoboMgr::triggerRentalWarp( Entity::Player& player, uint32_t levelId )
{
  auto& warpMgr = Common::Service< WarpMgr >::ref();
  auto& terriMgr = Common::Service< TerritoryMgr >::ref();
  auto& exdData = Common::Service< Data::ExdData >::ref();

  auto levelRow = exdData.getRow< Excel::Level >( levelId );
  if( !levelRow )
  {
    Logger::error( "ChocoboMgr: Level row not found for id {}!", levelId );
    return;
  }

  auto mapRow = exdData.getRow< Excel::Map >( levelRow->data().Map );
  if( !mapRow )
  {
    Logger::error( "ChocoboMgr: Map row not found for id {}!", levelRow->data().Map );
    return;
  }

  // Use raw world coordinates for warping
  double clientX = levelRow->data().TransX;
  double clientY = levelRow->data().TransY;
  double clientZ = levelRow->data().TransZ;

  Common::FFXIVARR_POSITION3 pos{ static_cast< float >( clientX ), static_cast< float >( clientY ), static_cast< float >( clientZ ) };
  float rot = static_cast< float >( levelRow->data().RotY );

  // Use the territory ID from the Level sheet
  auto territoryInstance = terriMgr.getTerritoryByTypeId( levelRow->data().TerritoryType );
  if( territoryInstance )
  {
    auto territoryGuid = territoryInstance->getGuId();
    warpMgr.requestMoveTerritory( player, Common::WarpType::WARP_TYPE_RENTAL_CHOCOBO, territoryGuid, pos, rot );

    // Mark the player for mounting after warp
    addPendingRentalMount( player.getCharacterId() );

    Logger::debug( "ChocoboMgr: Warp requested to territory {} for player {}", levelRow->data().TerritoryType, player.getCharacterId() );
  }
  else
  {
    Logger::error( "ChocoboMgr: Could not find territory instance for type {}", levelRow->data().TerritoryType );
  }
}

void ChocoboMgr::cleanup()
{
  Logger::debug( "ChocoboMgr: Cleanup called, {} pending mounts, {} persistent mounts",
                 s_pendingRentalMounts.size(), s_persistentMounts.size() );
}
