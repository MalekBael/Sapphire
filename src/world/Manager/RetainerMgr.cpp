#include "RetainerMgr.h"

#include <Common.h>
#include <Service.h>
#include <Database/DatabaseDef.h>
#include <Logging/Logger.h>
#include <Network/GamePacket.h>
#include <Network/PacketDef/Zone/ServerZoneDef.h>
#include <Network/PacketWrappers/RetainerSpawnPacket.h>
#include <Network/Util/PacketUtil.h>

#include <map>
#include <cmath>

#include "Actor/Player.h"
#include "Network/GameConnection.h"
#include "WorldServer.h"

using namespace Sapphire;
using namespace Sapphire::World::Manager;
using namespace Sapphire::Network::Packets;

bool RetainerMgr::init()
{
  Logger::info( "RetainerMgr: Initializing..." );
  // TODO: Any startup initialization needed
  return true;
}

// ========== Retainer CRUD Operations ==========

std::optional< uint64_t > RetainerMgr::createRetainer( Entity::Player& player,
                                                        const std::string& name,
                                                        RetainerPersonality personality,
                                                        const std::vector< uint8_t >& customize )
{
  // Validate name
  auto nameError = validateName( name );
  if( nameError != RetainerError::None )
  {
    Logger::warn( "RetainerMgr: Failed to create retainer, name validation failed: {}", name );
    return std::nullopt;
  }

  // Check if player has available slots
  auto currentCount = getRetainerCount( player );
  auto maxSlots = getMaxRetainerSlots( player );
  if( currentCount >= maxSlots )
  {
    Logger::warn( "RetainerMgr: Player {} has reached max retainer limit", player.getId() );
    return std::nullopt;
  }

  // Get next available slot
  uint8_t slot = getNextAvailableSlot( player );
  if( slot == 255 )
  {
    Logger::warn( "RetainerMgr: No available slots for player {}", player.getId() );
    return std::nullopt;
  }

  // Insert into database
  auto& db = Common::Service< Db::DbWorkerPool< Db::ZoneDbConnection > >::ref();
  auto stmt = db.getPreparedStatement( Db::ZoneDbStatements::CHARA_RETAINER_INS );
  stmt->setUInt( 1, player.getId() );               // CharacterId
  stmt->setUInt( 2, static_cast< uint32_t >( slot ) );   // DisplayOrder
  stmt->setString( 3, name );                       // Name
  stmt->setUInt( 4, static_cast< uint32_t >( personality ) );  // Personality
  stmt->setBinary( 5, customize );                  // Customize (26 bytes)
  stmt->setUInt( 6, static_cast< uint32_t >( time( nullptr ) ) );  // CreateTime
  
  try
  {
    db.directExecute( stmt );
  }
  catch( const std::exception& e )
  {
    Logger::error( "RetainerMgr: Database error creating retainer '{}' for player {}: {}", 
                   name, player.getId(), e.what() );
    return std::nullopt;
  }
  
  // Get the inserted retainer ID by querying for it
  auto selStmt = db.getPreparedStatement( Db::ZoneDbStatements::CHARA_RETAINER_SEL_BY_NAME );
  selStmt->setString( 1, name );
  auto res = db.query( selStmt );
  
  if( res && res->next() )
  {
    uint64_t retainerId = res->getUInt64( "RetainerId" );
    Logger::info( "RetainerMgr: Created retainer '{}' (ID: {}) for player {} in slot {} (personality: {})", 
                  name, retainerId, player.getId(), slot, static_cast< int >( personality ) );
    return retainerId;
  }
  
  Logger::error( "RetainerMgr: Failed to retrieve retainer ID after insert for '{}'", name );
  return std::nullopt;
}

RetainerError RetainerMgr::deleteRetainer( Entity::Player& player, uint64_t retainerId )
{
  auto retainer = getRetainer( retainerId );
  if( !retainer.has_value() )
  {
    return RetainerError::NotFound;
  }

  if( retainer->ownerId != player.getId() )
  {
    Logger::warn( "RetainerMgr: Player {} attempted to delete retainer {} they don't own", 
                  player.getId(), retainerId );
    return RetainerError::NotFound;
  }

  // Check if retainer has pending ventures or market listings
  if( retainer->ventureId > 0 && !retainer->ventureComplete )
  {
    return RetainerError::RetainerBusy;
  }

  if( retainer->sellingCount > 0 )
  {
    return RetainerError::RetainerBusy;
  }

  // Delete from database
  Logger::info( "RetainerMgr: Deleting retainer {} (name: '{}') for player {}", 
                retainerId, retainer->name, player.getId() );
  
  auto& db = Common::Service< Db::DbWorkerPool< Db::ZoneDbConnection > >::ref();
  
  try
  {
    db.directExecute( "DELETE FROM chararetainerinfo WHERE RetainerId = " + std::to_string( retainerId ) );
  }
  catch( const std::exception& e )
  {
    Logger::error( "RetainerMgr: DELETE failed with exception: {}", e.what() );
    return RetainerError::NotFound;
  }
  
  Logger::info( "RetainerMgr: Successfully deleted retainer {}", retainerId );
  return RetainerError::None;
}

std::vector< RetainerData > RetainerMgr::getRetainers( Entity::Player& player )
{
  std::vector< RetainerData > retainers;
  
  auto& db = Common::Service< Db::DbWorkerPool< Db::ZoneDbConnection > >::ref();
  auto stmt = db.getPreparedStatement( Db::ZoneDbStatements::CHARA_RETAINER_SEL_BY_CHARID );
  stmt->setUInt( 1, player.getId() );
  auto res = db.query( stmt );
  
  while( res && res->next() )
  {
    RetainerData data;
    data.retainerId = res->getUInt64( "RetainerId" );
    data.ownerId = res->getUInt( "CharacterId" );
    data.displayOrder = static_cast< uint8_t >( res->getUInt( "DisplayOrder" ) );
    data.name = res->getString( "Name" );
    data.personality = static_cast< RetainerPersonality >( res->getUInt( "Personality" ) );
    data.level = static_cast< uint8_t >( res->getUInt( "Level" ) );
    data.classJob = static_cast< uint8_t >( res->getUInt( "ClassJob" ) );
    data.exp = res->getUInt( "Exp" );
    data.cityId = static_cast< uint8_t >( res->getUInt( "CityId" ) );
    data.gil = res->getUInt( "Gil" );
    data.itemCount = static_cast< uint8_t >( res->getUInt( "ItemCount" ) );
    data.sellingCount = static_cast< uint8_t >( res->getUInt( "SellingCount" ) );
    data.ventureId = res->getUInt( "VentureId" );
    data.ventureComplete = res->getUInt( "VentureComplete" ) != 0;
    data.ventureStartTime = res->getUInt( "VentureStartTime" );
    data.ventureCompleteTime = res->getUInt( "VentureCompleteTime" );
    data.isActive = res->getUInt( "IsActive" ) != 0;
    data.isRename = res->getUInt( "IsRename" ) != 0;
    data.status = static_cast< uint8_t >( res->getUInt( "Status" ) );
    data.createTime = res->getUInt( "CreateTime" );
    
    // Get customize data if present
    if( !res->isNull( "Customize" ) )
    {
      auto blobData = res->getBlobVector( "Customize" );
      if( blobData.size() == 26 )
      {
        data.customize.assign( blobData.begin(), blobData.end() );
      }
    }
    
    retainers.push_back( data );
  }

  Logger::debug( "RetainerMgr: Loaded {} retainers for player {}", retainers.size(), player.getId() );
  return retainers;
}

std::optional< RetainerData > RetainerMgr::getRetainer( uint64_t retainerId )
{
  return loadRetainerFromDb( retainerId );
}

uint8_t RetainerMgr::getRetainerCount( Entity::Player& player )
{
  auto& db = Common::Service< Db::DbWorkerPool< Db::ZoneDbConnection > >::ref();
  auto stmt = db.getPreparedStatement( Db::ZoneDbStatements::CHARA_RETAINER_COUNT );
  stmt->setUInt( 1, player.getId() );
  auto res = db.query( stmt );
  
  if( res && res->next() )
  {
    return static_cast< uint8_t >( res->getUInt( "cnt" ) );
  }
  
  return 0;
}

uint8_t RetainerMgr::getMaxRetainerSlots( Entity::Player& player )
{
  // Base: 2 retainers
  // Additional retainers can be purchased from Mog Station
  // Check player's account flags for additional slots
  // For now, return default
  return 2;
}

// ========== Name Validation ==========

bool RetainerMgr::isNameAvailable( const std::string& name )
{
  auto& db = Common::Service< Db::DbWorkerPool< Db::ZoneDbConnection > >::ref();
  auto stmt = db.getPreparedStatement( Db::ZoneDbStatements::CHARA_RETAINER_SEL_BY_NAME );
  stmt->setString( 1, name );
  auto res = db.query( stmt );
  
  // Name is available if no results found
  return !( res && res->next() );
}

RetainerError RetainerMgr::validateName( const std::string& name )
{
  // Check length (1-20 characters, matching client limit from RETAINER_DATABASE_FLOW.md)
  if( name.empty() || name.length() > 20 )
  {
    return RetainerError::InvalidName;
  }

  // Check for valid characters (alphanumeric and some special chars)
  for( char c : name )
  {
    // Allow letters, numbers, hyphens, apostrophes
    if( !std::isalnum( static_cast< unsigned char >( c ) ) && c != '-' && c != '\'' )
    {
      return RetainerError::InvalidName;
    }
  }

  // Check availability
  if( !isNameAvailable( name ) )
  {
    return RetainerError::NameTaken;
  }

  // TODO: Add profanity filter check

  return RetainerError::None;
}

RetainerError RetainerMgr::renameRetainer( Entity::Player& player, uint64_t retainerId, const std::string& newName )
{
  auto retainer = getRetainer( retainerId );
  if( !retainer.has_value() || retainer->ownerId != player.getId() )
  {
    return RetainerError::NotFound;
  }

  auto nameError = validateName( newName );
  if( nameError != RetainerError::None )
  {
    return nameError;
  }

  // TODO: Check if player has rename token
  // TODO: Update database
  // TODO: Consume rename token

  return RetainerError::None;
}

// ========== Class/Job Management ==========

RetainerError RetainerMgr::setRetainerClass( Entity::Player& player, uint64_t retainerId, uint8_t classJob )
{
  auto retainer = getRetainer( retainerId );
  if( !retainer.has_value() || retainer->ownerId != player.getId() )
  {
    return RetainerError::NotFound;
  }

  // Retainers can only have Disciples of War/Magic or Disciples of Land classes
  // Validate classJob ID here
  // TODO: Validate against ClassJob Excel sheet

  // TODO: Update database

  return RetainerError::None;
}

void RetainerMgr::addRetainerExp( uint64_t retainerId, uint32_t exp )
{
  // TODO: Add exp and handle level ups
}

// ========== Venture Management ==========

RetainerError RetainerMgr::startVenture( Entity::Player& player, uint64_t retainerId, uint32_t ventureId )
{
  auto retainer = getRetainer( retainerId );
  if( !retainer.has_value() || retainer->ownerId != player.getId() )
  {
    return RetainerError::NotFound;
  }

  if( retainer->ventureId > 0 )
  {
    return RetainerError::RetainerBusy;
  }

  // TODO: Look up venture duration from RetainerTask Excel
  // TODO: Check if retainer meets level/class requirements
  // TODO: Check if player has venture tokens
  // TODO: Consume venture token
  // TODO: Update database with venture info

  return RetainerError::None;
}

RetainerError RetainerMgr::completeVenture( Entity::Player& player, uint64_t retainerId )
{
  auto retainer = getRetainer( retainerId );
  if( !retainer.has_value() || retainer->ownerId != player.getId() )
  {
    return RetainerError::NotFound;
  }

  if( retainer->ventureId == 0 || !retainer->ventureComplete )
  {
    return RetainerError::None; // No venture or not complete
  }

  // TODO: Generate rewards based on ventureId
  // TODO: Add items to retainer inventory
  // TODO: Add exp to retainer
  // TODO: Clear venture data in database

  return RetainerError::None;
}

RetainerError RetainerMgr::cancelVenture( Entity::Player& player, uint64_t retainerId )
{
  auto retainer = getRetainer( retainerId );
  if( !retainer.has_value() || retainer->ownerId != player.getId() )
  {
    return RetainerError::NotFound;
  }

  if( retainer->ventureId == 0 )
  {
    return RetainerError::None;
  }

  // TODO: Clear venture data in database
  // Note: Venture token is NOT refunded

  return RetainerError::None;
}

// ========== Market Board ==========

RetainerError RetainerMgr::listItem( Entity::Player& player, uint64_t retainerId,
                                     uint32_t itemId, uint32_t price, uint32_t stack )
{
  auto retainer = getRetainer( retainerId );
  if( !retainer.has_value() || retainer->ownerId != player.getId() )
  {
    return RetainerError::NotFound;
  }

  // TODO: Verify item exists in retainer inventory
  // TODO: Check if retainer has available market slots (max 20)
  // TODO: Move item from inventory to market storage
  // TODO: Create market listing in database

  return RetainerError::None;
}

RetainerError RetainerMgr::unlistItem( Entity::Player& player, uint64_t retainerId, uint64_t listingId )
{
  // TODO: Implement
  return RetainerError::None;
}

RetainerError RetainerMgr::adjustPrice( Entity::Player& player, uint64_t retainerId,
                                        uint64_t listingId, uint32_t newPrice )
{
  // TODO: Implement
  return RetainerError::None;
}

// ========== Packet Sending ==========

void RetainerMgr::sendRetainerList( Entity::Player& player, uint32_t handlerId )
{
  Logger::info( "RetainerMgr::sendRetainerList called for player {}", player.getId() );
  
  auto retainers = getRetainers( player );
  Logger::info( "RetainerMgr: getRetainers returned {} retainers", retainers.size() );
  
  auto& server = Common::Service< World::WorldServer >::ref();
  const auto maxSlots = getMaxRetainerSlots( player );
  const auto currentCount = static_cast< uint8_t >( retainers.size() );
  
  // ========== Step 0: Send ActorControl to set retainer count ==========
  // This updates the client's internal count for GetRetainerEmployedCount()
  Network::Util::Packet::sendActorControlSelf( player, player.getId(), 0x98 /* SetRetainerCount */,
                                                static_cast<uint32_t>(retainers.size()) );
  
  // Build a map of slot -> retainer for quick lookup
  std::map< uint8_t, const RetainerData* > slotMap;
  for( const auto& r : retainers )
  {
    slotMap[r.displayOrder] = &r;
  }
  
  // If handlerId not provided, generate one based on player ID
  // In the capture, handlerId = 0x1ADD = 6877
  if( handlerId == 0 )
  {
    // Use lower 16 bits of player ID as a simple handler ID
    handlerId = ( player.getId() & 0xFFFF ) | 0x1A00;
  }
  
  // ========== Step 1: Send 8x 0x01AB RetainerData packets (one per slot) ==========
  // NOTE: Retail sends RetainerData BEFORE RetainerList (based on packet capture)
  // BINARY ANALYSIS: GetRetainerEmployedCount() counts slots where retainerId != 0 at offset 8472
  for( uint8_t slot = 0; slot < 8; slot++ )
  {
    auto infoPacket = makeZonePacket< WorldPackets::Server::FFXIVIpcRetainerData >( player.getId() );
    auto& info = infoPacket->data();
    std::memset( &info, 0, sizeof( info ) );
    
    // Common fields for all slots
    info.handlerId = handlerId;
    info.unknown1 = 0xFFFFFFFF;  // Always -1 in capture
    info.slotIndex = slot;        // Slot index 0-7 (client uses this for array indexing!)
    info.personality = 0x99;      // 153 = default personality value
    
    auto it = slotMap.find( slot );
    if( it != slotMap.end() && it->second )
    {
      // Filled slot - retainerId MUST be non-zero for client to count as employed!
      const auto& r = *it->second;
      info.retainerId = r.retainerId;  // Full 64-bit retainer ID (client counts non-zero as employed)
      info.unknown2 = 11;              // Unknown, 11 for filled slots
      info.activeFlag = 1;             // 1 = ACTIVE slot
      info.classJob = r.classJob > 0 ? r.classJob : 1;
      info.unknown3 = r.createTime;    // Create time or level?
      info.unknown4 = r.displayOrder + 1;  // 1-indexed display order
      std::strncpy( info.name, r.name.c_str(), 31 );
      info.name[31] = '\0';
      
      Logger::debug( "RetainerMgr: Slot {} - '{}' (retainerId: {}, classJob: {})", 
                     slot, r.name, r.retainerId, static_cast<int>(info.classJob) );
    }
    else
    {
      // Empty slot - retainerId must be 0 (client won't count this as employed)
      info.retainerId = 0;
      info.activeFlag = 0;
      Logger::debug( "RetainerMgr: Slot {} - empty (retainerId: 0)", slot );
    }
    
    server.queueForPlayer( player.getCharacterId(), infoPacket );
  }
  
  // ========== Step 2: Send 0x01AA RetainerList (tiny 8-byte packet) AFTER the data packets ==========
  auto listPacket = makeZonePacket< WorldPackets::Server::FFXIVIpcRetainerList >( player.getId() );
  auto& listData = listPacket->data();
  std::memset( &listData, 0, sizeof( listData ) );
  
  listData.handlerId = handlerId;
  
  // BINARY ANALYSIS FINDINGS:
  // The client stores packet byte +5 (our "retainerCount" field) into byte_1417A12B2
  // GetAvailableRetainerSlots() reads byte_1417A12B2 directly (it's the AVAILABLE slots, not employed count!)
  // Lua Scene 1 checks: if employedCount <= availableSlots then show "hire up to [availableSlots]"
  // 
  // So this field should contain: maxSlots - currentlyEmployed = available slots remaining
  uint8_t currentEmployed = static_cast< uint8_t >( retainers.size() );
  uint8_t availableSlots = maxSlots > currentEmployed ? maxSlots - currentEmployed : 0;
  
  listData.maxSlots = maxSlots;         // Maximum allowed retainer slots (2)
  listData.retainerCount = availableSlots;  // ACTUALLY available slots (maxSlots - employed)
  listData.padding = 0;

  Logger::debug( "RetainerMgr: Sending RetainerList - maxSlots: {}, availableSlots: {} (employed: {})",
                 maxSlots, availableSlots, currentEmployed );
  server.queueForPlayer( player.getCharacterId(), listPacket );
  
  Logger::info( "RetainerMgr: Sent 8x RetainerData + 1x RetainerList packets (count: {})", retainers.size() );
}

void RetainerMgr::sendRetainerInfo( Entity::Player& player, uint64_t retainerId, 
                                    uint32_t handlerId, uint8_t slotIndex )
{
  auto retainer = getRetainer( retainerId );
  if( !retainer.has_value() )
  {
    Logger::warn( "RetainerMgr: Cannot send info for unknown retainer {}", retainerId );
    return;
  }
  
  auto& server = Common::Service< World::WorldServer >::ref();
  auto packet = makeZonePacket< WorldPackets::Server::FFXIVIpcRetainerData >( player.getId() );
  auto& data = packet->data();
  std::memset( &data, 0, sizeof( data ) );
  
  // If handlerId not provided, generate one
  if( handlerId == 0 )
  {
    handlerId = ( player.getId() & 0xFFFF ) | 0x1A00;
  }
  
  // Fill packet using corrected 3.35 structure (based on binary analysis)
  data.handlerId = handlerId;
  data.unknown1 = 0xFFFFFFFF;  // Always -1
  data.retainerId = retainer->retainerId;  // Full 64-bit retainer ID
  data.slotIndex = slotIndex > 0 ? slotIndex : retainer->displayOrder;
  data.unknown2 = 11;          // Filled slot marker
  data.activeFlag = 1;         // Active slot
  data.classJob = retainer->classJob > 0 ? retainer->classJob : 1;
  data.unknown3 = retainer->createTime;
  data.unknown4 = retainer->displayOrder + 1;
  data.personality = 0x99;     // 153
  std::strncpy( data.name, retainer->name.c_str(), 31 );
  data.name[31] = '\0';
  
  server.queueForPlayer( player.getCharacterId(), packet );
  Logger::debug( "RetainerMgr: Sent RetainerInfo for '{}' to player {} ({} bytes)", 
                 retainer->name, player.getId(), sizeof( data ) );
}

void RetainerMgr::sendRetainerInventory( Entity::Player& player, uint64_t retainerId )
{
  // TODO: Query retainer inventory from database
  // TODO: Send item packets similar to player inventory
}

void RetainerMgr::sendMarketListings( Entity::Player& player, uint64_t retainerId )
{
  // TODO: Query market listings from database
  // TODO: Send market listing packets
}

void RetainerMgr::sendSalesHistory( Entity::Player& player, uint64_t retainerId )
{
  // TODO: Query sales history from database (last 20)
  // TODO: Send sales history packets
}

// ========== NPC Spawning ==========

uint32_t RetainerMgr::spawnRetainer( Entity::Player& player, uint64_t retainerId )
{
  // Load retainer data
  auto retainerOpt = getRetainer( retainerId );
  if( !retainerOpt )
  {
    Logger::error( "RetainerMgr::spawnRetainer - Retainer {} not found", retainerId );
    return 0;
  }

  auto& retainerData = *retainerOpt;

  // Verify ownership
  if( retainerData.ownerId != player.getId() )
  {
    Logger::error( "RetainerMgr::spawnRetainer - Player {} tried to spawn retainer {} owned by {}",
                   player.getId(), retainerId, retainerData.ownerId );
    return 0;
  }

  // Check if already spawned
  auto existingActor = getSpawnedRetainerActorId( player, retainerId );
  if( existingActor != 0 )
  {
    Logger::warn( "RetainerMgr::spawnRetainer - Retainer {} already spawned as actor {}",
                  retainerId, existingActor );
    return existingActor;
  }

  // Generate unique actor ID for this retainer spawn
  // Retainer actor IDs use pattern: 0x40000000 + (player low 16 bits << 8) + retainer slot
  // This ensures they're unique per player and don't clash with other actors
  uint32_t retainerActorId = 0x40000000 | ( ( player.getId() & 0xFFFF ) << 8 ) | ( retainerData.displayOrder & 0xFF );

  // Calculate spawn position (2 yalms in front of player)
  auto playerPos = player.getPos();
  auto playerRot = player.getRot();
  
  Common::FFXIVARR_POSITION3 spawnPos;
  spawnPos.x = playerPos.x + ( std::sin( playerRot ) * 2.0f );
  spawnPos.y = playerPos.y;
  spawnPos.z = playerPos.z + ( std::cos( playerRot ) * 2.0f );

  // Face retainer towards player
  float retainerRot = playerRot + 3.14159265f; // 180 degrees opposite

  // Send spawn packet
  auto& server = Common::Service< World::WorldServer >::ref();
  auto spawnPacket = std::make_shared< WorldPackets::Server::RetainerSpawnPacket >(
    retainerActorId,
    retainerData,
    spawnPos,
    retainerRot,
    player
  );

  server.queueForPlayer( player.getCharacterId(), spawnPacket );

  // Track spawned retainer
  m_spawnedRetainers[ player.getId() ][ retainerId ] = retainerActorId;

  Logger::info( "RetainerMgr::spawnRetainer - Spawned retainer {} '{}' as actor {} for player {}",
                retainerId, retainerData.name, retainerActorId, player.getId() );

  return retainerActorId;
}

void RetainerMgr::despawnRetainer( Entity::Player& player, uint32_t retainerActorId )
{
  // TODO: Send ActorFreeSpawn packet to remove retainer from world
  // For now, just remove from tracking
  
  auto playerIt = m_spawnedRetainers.find( player.getId() );
  if( playerIt == m_spawnedRetainers.end() )
    return;

  // Find and remove the retainer
  for( auto it = playerIt->second.begin(); it != playerIt->second.end(); ++it )
  {
    if( it->second == retainerActorId )
    {
      Logger::info( "RetainerMgr::despawnRetainer - Despawned retainer actor {} for player {}",
                    retainerActorId, player.getId() );
      playerIt->second.erase( it );
      break;
    }
  }

  // Clean up player entry if no more spawned retainers
  if( playerIt->second.empty() )
  {
    m_spawnedRetainers.erase( playerIt );
  }
}

uint32_t RetainerMgr::getSpawnedRetainerActorId( Entity::Player& player, uint64_t retainerId )
{
  auto playerIt = m_spawnedRetainers.find( player.getId() );
  if( playerIt == m_spawnedRetainers.end() )
    return 0;

  auto retainerIt = playerIt->second.find( retainerId );
  if( retainerIt == playerIt->second.end() )
    return 0;

  return retainerIt->second;
}

// ========== Private Methods ==========

std::optional< RetainerData > RetainerMgr::loadRetainerFromDb( uint64_t retainerId )
{
  auto& db = Common::Service< Db::DbWorkerPool< Db::ZoneDbConnection > >::ref();
  auto stmt = db.getPreparedStatement( Db::ZoneDbStatements::CHARA_RETAINER_SEL );
  stmt->setUInt64( 1, retainerId );
  auto res = db.query( stmt );
  
  if( res && res->next() )
  {
    RetainerData data;
    data.retainerId = res->getUInt64( "RetainerId" );
    data.ownerId = res->getUInt( "CharacterId" );
    data.displayOrder = static_cast< uint8_t >( res->getUInt( "DisplayOrder" ) );
    data.name = res->getString( "Name" );
    data.personality = static_cast< RetainerPersonality >( res->getUInt( "Personality" ) );
    data.level = static_cast< uint8_t >( res->getUInt( "Level" ) );
    data.classJob = static_cast< uint8_t >( res->getUInt( "ClassJob" ) );
    data.exp = res->getUInt( "Exp" );
    data.cityId = static_cast< uint8_t >( res->getUInt( "CityId" ) );
    data.gil = res->getUInt( "Gil" );
    data.itemCount = static_cast< uint8_t >( res->getUInt( "ItemCount" ) );
    data.sellingCount = static_cast< uint8_t >( res->getUInt( "SellingCount" ) );
    data.ventureId = res->getUInt( "VentureId" );
    data.ventureComplete = res->getUInt( "VentureComplete" ) != 0;
    data.ventureStartTime = res->getUInt( "VentureStartTime" );
    data.ventureCompleteTime = res->getUInt( "VentureCompleteTime" );
    data.isActive = res->getUInt( "IsActive" ) != 0;
    data.isRename = res->getUInt( "IsRename" ) != 0;
    data.status = static_cast< uint8_t >( res->getUInt( "Status" ) );
    data.createTime = res->getUInt( "CreateTime" );
    
    // Get customize data if present
    if( !res->isNull( "Customize" ) )
    {
      auto blobData = res->getBlobVector( "Customize" );
      if( blobData.size() == 26 )
      {
        data.customize.assign( blobData.begin(), blobData.end() );
      }
    }
    
    return data;
  }

  return std::nullopt;
}

bool RetainerMgr::saveRetainerToDb( const RetainerData& data )
{
  // TODO: Update database
  return false;
}

uint8_t RetainerMgr::getNextAvailableSlot( Entity::Player& player )
{
  auto retainers = getRetainers( player );
  std::set< uint8_t > usedSlots;
  
  for( const auto& r : retainers )
  {
    usedSlots.insert( r.displayOrder );
  }

  // Find first unused slot
  for( uint8_t i = 0; i < 10; i++ )
  {
    if( usedSlots.find( i ) == usedSlots.end() )
    {
      return i;
    }
  }

  return 255; // All slots used
}
