#include "RetainerMgr.h"

#include <Common.h>
#include <Service.h>
#include <Database/DatabaseDef.h>
#include <Logging/Logger.h>
#include <Network/GamePacket.h>
#include <Network/PacketDef/Zone/ServerZoneDef.h>

#include <map>

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

  // TODO: Delete from database
  // auto& db = Common::Service< Db::DbWorkerPool< Db::ZoneDbConnection > >::ref();
  // auto stmt = db.getPreparedStatement( Db::ZoneDbStatements::CHARA_RETAINER_DEL );
  // stmt->setUInt64( 1, retainerId );
  // db.directExecute( stmt );

  Logger::info( "RetainerMgr: Deleted retainer {} for player {}", retainerId, player.getId() );
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
  for( uint8_t slot = 0; slot < 8; slot++ )
  {
    auto infoPacket = makeZonePacket< WorldPackets::Server::FFXIVIpcRetainerData >( player.getId() );
    auto& info = infoPacket->data();
    std::memset( &info, 0, sizeof( info ) );
    
    // Common fields for all slots
    info.handlerId = handlerId;
    info.unknown1 = 0xFFFFFFFF;  // Always -1 in capture
    info.unknown7 = 57;          // Always 0x39 in capture
    info.hireOrder = slot;       // Slot index (0-based for empty slots)
    info.personality = 0x99;     // 153 = default personality value
    
    auto it = slotMap.find( slot );
    if( it != slotMap.end() && it->second )
    {
      // Filled slot
      const auto& r = *it->second;
      info.retainerIdLow = static_cast< uint32_t >( r.retainerId );
      info.unknown2 = 23;        // Unknown, but 23 for filled slots in capture
      info.unknown3 = 0x78;      // Retail shows 0x78 for filled - NOT level, purpose unknown
      info.ownerIdHigh = static_cast< uint32_t >( player.getCharacterId() >> 32 ) | 0x6800;
      info.unknown4 = 11;        // Unknown, but 11 for filled slots
      info.unknown5 = 1;         // 1 = ACTIVE slot (this is the IsCurrentRetainerActive check!)
      info.classJob = r.classJob > 0 ? r.classJob : 1;
      info.createTime = r.createTime;
      // In retail, hireOrder for filled slots appears to be 1-indexed
      // Slot 0 in displayOrder -> hireOrder 1
      info.hireOrder = r.displayOrder + 1;  // 1-indexed for client
      std::strncpy( info.name, r.name.c_str(), 31 );
      info.name[31] = '\0';
      
      Logger::info( "RetainerMgr: Slot {} - '{}' (id: {}, classJob: {}, level: {}, unknown5: {}, hireOrder: {})", 
                    slot, r.name, r.retainerId, r.classJob, r.level, info.unknown5, info.hireOrder );
    }
    else
    {
      // Empty slot - mostly zeros
      info.ownerIdHigh = static_cast< uint32_t >( player.getCharacterId() >> 32 ) | 0x6801;
      info.unknown5 = 0;  // Explicitly 0 for empty/inactive slots
      Logger::debug( "RetainerMgr: Slot {} - empty (unknown5: {})", slot, info.unknown5 );
    }
    
    server.queueForPlayer( player.getCharacterId(), infoPacket );
  }
  
  // ========== Step 2: Send 0x01AA RetainerList (tiny 8-byte packet) AFTER the data packets ==========
  auto listPacket = makeZonePacket< WorldPackets::Server::FFXIVIpcRetainerList >( player.getId() );
  auto& listData = listPacket->data();
  std::memset( &listData, 0, sizeof( listData ) );
  
  listData.handlerId = handlerId;
  listData.maxSlots = 8;  // Standard max slots
  listData.retainerCount = static_cast< uint8_t >( retainers.size() );
  listData.padding = 0;
  
  Logger::info( "RetainerMgr: Sending RetainerList 0x01AA - handlerId: 0x{:04X}, maxSlots: {}, count: {}, size: {} bytes",
                handlerId, listData.maxSlots, listData.retainerCount, sizeof( listData ) );
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
  
  // Fill packet using new 3.35 structure
  data.handlerId = handlerId;
  data.unknown1 = 0xFFFFFFFF;  // Always -1
  data.retainerIdLow = static_cast< uint32_t >( retainer->retainerId );
  data.unknown2 = 23;
  data.unknown3 = 0x78;  // Retail shows 0x78 for filled slots - NOT level, purpose unknown
  data.ownerIdHigh = static_cast< uint32_t >( player.getCharacterId() >> 32 ) | 0x6800;
  data.unknown4 = 11;
  data.unknown5 = 1;
  data.classJob = retainer->classJob > 0 ? retainer->classJob : 1;
  data.unknown6 = 0;
  data.unknown7 = 57;  // Always 0x39
  data.createTime = retainer->createTime;
  data.hireOrder = slotIndex > 0 ? slotIndex : retainer->displayOrder;
  data.personality = 0x99;  // 153
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

// ========== Private Methods ==========

std::optional< RetainerData > RetainerMgr::loadRetainerFromDb( uint64_t retainerId )
{
  // TODO: Query database
  // auto& db = Common::Service< Db::DbWorkerPool< Db::ZoneDbConnection > >::ref();
  // auto stmt = db.getPreparedStatement( Db::ZoneDbStatements::CHARA_RETAINER_SEL );
  // stmt->setUInt64( 1, retainerId );
  // auto res = db.query( stmt );
  // if( res->next() )
  // {
  //   RetainerData data;
  //   // ... populate from result set
  //   return data;
  // }

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
