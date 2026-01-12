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
  stmt->setUInt( 1, player.getId() );                            // CharacterId
  stmt->setUInt( 2, static_cast< uint32_t >( slot ) );           // DisplayOrder
  stmt->setString( 3, name );                                    // Name
  stmt->setUInt( 4, static_cast< uint32_t >( personality ) );    // Personality
  stmt->setBinary( 5, customize );                               // Customize (26 bytes)
  stmt->setUInt( 6, static_cast< uint32_t >( time( nullptr ) ) );// CreateTime

  try
  {
    db.directExecute( stmt );
  } catch( const std::exception& e )
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
  } catch( const std::exception& e )
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

  // Retainers can have:
  // - Disciples of War: Gladiator(1), Pugilist(2), Marauder(3), Lancer(4), Archer(5)
  // - Disciples of Magic: Conjurer(6), Thaumaturge(7), Arcanist(26)
  // - Disciples of Land: Miner(16), Botanist(17), Fisher(18)
  // Classes 8-15 are crafters (not valid for retainers in 3.x)
  bool isValidClass = ( classJob >= 1 && classJob <= 7 ) ||  // DoW/DoM base classes
                      ( classJob >= 16 && classJob <= 18 ) ||// DoL classes
                      ( classJob == 26 );                    // Arcanist
  if( !isValidClass )
  {
    Logger::warn( "RetainerMgr::setRetainerClass - Invalid classJob {} for retainer {}", classJob, retainerId );
    return RetainerError::None;// Silently fail for invalid classes
  }

  // Update database
  auto& db = Common::Service< Db::DbWorkerPool< Db::ZoneDbConnection > >::ref();
  auto stmt = db.getPreparedStatement( Db::ZoneDbStatements::CHARA_RETAINER_UP_CLASSJOB );
  stmt->setUInt( 1, classJob );
  stmt->setUInt64( 2, retainerId );

  try
  {
    db.directExecute( stmt );
  } catch( const std::exception& e )
  {
    Logger::error( "RetainerMgr::setRetainerClass - DB error: {}", e.what() );
    return RetainerError::None;// Return success to not break client flow
  }

  // Reload retainer data from DB to get updated classJob
  auto updatedRetainer = getRetainer( retainerId );
  if( updatedRetainer.has_value() )
  {
    Logger::info( "RetainerMgr::setRetainerClass - DB updated, new classJob={}",
                  static_cast< int >( updatedRetainer->classJob ) );
  }

  // Send updated RetainerInfo packet to client so it knows the class changed
  // This is needed because the client caches classJob from the initial spawn packet
  // We also resend the full retainer list to update the client's menu cache
  sendRetainerInfo( player, retainerId, 0, updatedRetainer ? updatedRetainer->displayOrder : 0 );

  // Also resend the retainer inventory which triggers a refresh
  sendRetainerInventory( player, retainerId );

  Logger::info( "RetainerMgr::setRetainerClass - Set retainer {} '{}' classJob to {}",
                retainerId, updatedRetainer ? updatedRetainer->name : "?", classJob );
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
    return RetainerError::None;// No venture or not complete
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

uint32_t RetainerMgr::registerToMarket( Entity::Player& player, uint64_t retainerId, uint8_t cityId )
{
  auto retainer = getRetainer( retainerId );
  if( !retainer.has_value() )
  {
    Logger::warn( "RetainerMgr::registerToMarket - Retainer {} not found", retainerId );
    return 1;// Generic error
  }

  if( retainer->ownerId != player.getId() )
  {
    Logger::warn( "RetainerMgr::registerToMarket - Player {} doesn't own retainer {}",
                  player.getId(), retainerId );
    return 1;// Generic error
  }

  // Check if already registered to this city
  if( retainer->cityId == cityId && cityId != 0 )
  {
    Logger::debug( "RetainerMgr::registerToMarket - Retainer {} already registered to city {}",
                   retainerId, cityId );
    return 463;// Already registered at this city
  }

  // Update cityId in database
  auto& db = Common::Service< Db::DbWorkerPool< Db::ZoneDbConnection > >::ref();
  auto stmt = db.getPreparedStatement( Db::ZoneDbStatements::CHARA_RETAINER_UP_CITY );
  stmt->setUInt( 1, cityId );
  stmt->setUInt64( 2, retainerId );

  try
  {
    db.directExecute( stmt );
  } catch( const std::exception& e )
  {
    Logger::error( "RetainerMgr::registerToMarket - DB error: {}", e.what() );
    return 1;// Generic error
  }

  Logger::info( "RetainerMgr::registerToMarket - Registered retainer {} '{}' to city {}",
                retainerId, retainer->name, cityId );
  return 0;// Success
}

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

  const auto retainers = getRetainers( player );
  Logger::info( "RetainerMgr: getRetainers returned {} retainers", retainers.size() );

  auto& server = Common::Service< World::WorldServer >::ref();
  const uint8_t maxSlots = getMaxRetainerSlots( player );
  const uint8_t employedCount = static_cast< uint8_t >( retainers.size() );
  const uint8_t availableSlots = maxSlots > employedCount ? static_cast< uint8_t >( maxSlots - employedCount ) : 0;

  // If handlerId not provided, generate one based on player ID.
  // Retail captures show a value in the 0x1A** range.
  if( handlerId == 0 )
    handlerId = ( player.getId() & 0xFFFF ) | 0x1A00;

  // Step 0: Tell the client how many retainers are currently employed.
  // This updates GetRetainerEmployedCount() client-side.
  Network::Util::Packet::sendActorControlSelf( player, player.getId(), 0x98 /* SetRetainerCount */,
                                               static_cast< uint32_t >( employedCount ) );

  // Build a fixed slot index (0-7) lookup.
  std::array< const RetainerData*, 8 > slotToRetainer{};
  slotToRetainer.fill( nullptr );
  for( const auto& r : retainers )
  {
    if( r.displayOrder < slotToRetainer.size() )
      slotToRetainer[ r.displayOrder ] = &r;
  }

  // Step 1: Send 8x 0x01AB RetainerData packets (one per slot).
  // NOTE: In retail captures, these commonly arrive before the 0x01AA list.
  for( uint8_t slot = 0; slot < slotToRetainer.size(); ++slot )
  {
    auto infoPacket = makeZonePacket< WorldPackets::Server::FFXIVIpcRetainerData >( player.getId() );
    auto& info = infoPacket->data();
    std::memset( &info, 0, sizeof( info ) );

    info.handlerId = handlerId;
    info.unknown1 = 0xFFFFFFFF;
    info.slotIndex = slot;
    info.personality = 0x99;

    // Retail 3.35 capture shows these bytes are non-zero constants even for empty slots.
    info.padding1[ 0 ] = 0x68;
    info.padding1[ 1 ] = 0x0E;
    info.padding1[ 2 ] = 0xE2;
    info.padding2[ 0 ] = 0x00;
    info.padding2[ 1 ] = 0x39;
    info.tail16 = 0xE20E;
    info.tail32 = 0xE20E6B70;

    if( const RetainerData* r = slotToRetainer[ slot ] )
    {
      info.retainerId = r->retainerId;
      // Number of items currently listed on the market for this retainer.
      // This value is shown in the retainer list UI as "selling X/20".
      info.sellingCount = r->sellingCount;
      info.activeFlag = 1;
      info.classJob = r->classJob > 0 ? r->classJob : 1;
      info.unknown3 = r->createTime;
      info.unknown4 = 1;

      std::strncpy( info.name, r->name.c_str(), 31 );
      info.name[ 31 ] = '\0';

      Logger::debug( "RetainerMgr: Slot {} - '{}' (retainerId: {}, classJob: {}, tail32: {:08X})",
                     slot, r->name, r->retainerId, static_cast< int >( info.classJob ), info.tail32 );
    }
    else
    {
      info.retainerId = 0;
      info.activeFlag = 0;
      info.unknown4 = 0;
      Logger::debug( "RetainerMgr: Slot {} - empty (retainerId: 0)", slot );
    }

    server.queueForPlayer( player.getCharacterId(), infoPacket );
  }

  // Step 2: Send 0x01AA RetainerList.
  // IMPORTANT: Despite its name, byte +5 is treated by the client as "available slots".
  auto listPacket = makeZonePacket< WorldPackets::Server::FFXIVIpcRetainerList >( player.getId() );
  auto& listData = listPacket->data();
  std::memset( &listData, 0, sizeof( listData ) );

  listData.handlerId = handlerId;
  listData.maxSlots = maxSlots;
  listData.retainerCount = availableSlots;
  listData.padding = 0;

  Logger::debug( "RetainerMgr: Sending RetainerList - maxSlots: {}, availableSlots: {} (employed: {})",
                 maxSlots, availableSlots, employedCount );
  server.queueForPlayer( player.getCharacterId(), listPacket );

  // Step 3: Send 0x01EF IsMarket so the client enables market-related retainer options.
  // Retail capture shows this packet sent twice back-to-back.
  sendIsMarket( player );
  sendIsMarket( player );

  Logger::info( "RetainerMgr: Sent 8x RetainerData + 1x RetainerList + 2x IsMarket (employed: {}, maxSlots: {})",
                employedCount, maxSlots );
}

void RetainerMgr::sendRetainerInfo( Entity::Player& player, uint64_t retainerId,
                                    uint32_t handlerId, uint8_t slotIndex )
{
  const auto retainer = getRetainer( retainerId );
  if( !retainer.has_value() )
  {
    Logger::warn( "RetainerMgr: Cannot send info for unknown retainer {}", retainerId );
    return;
  }

  auto& server = Common::Service< World::WorldServer >::ref();

  if( handlerId == 0 )
    handlerId = ( player.getId() & 0xFFFF ) | 0x1A00;

  auto packet = makeZonePacket< WorldPackets::Server::FFXIVIpcRetainerData >( player.getId() );
  auto& data = packet->data();
  std::memset( &data, 0, sizeof( data ) );

  data.handlerId = handlerId;
  data.unknown1 = 0xFFFFFFFF;
  data.retainerId = retainer->retainerId;
  data.slotIndex = slotIndex > 0 ? slotIndex : retainer->displayOrder;
  // Number of items currently listed on the market for this retainer.
  data.sellingCount = retainer->sellingCount;
  data.activeFlag = 1;
  data.classJob = retainer->classJob > 0 ? retainer->classJob : 1;
  data.unknown3 = retainer->createTime;
  data.unknown4 = 1;
  data.personality = 0x99;

  // Retail 3.35 capture shows these bytes are non-zero constants.
  data.padding1[ 0 ] = 0x68;
  data.padding1[ 1 ] = 0x0E;
  data.padding1[ 2 ] = 0xE2;
  data.padding2[ 0 ] = 0x00;
  data.padding2[ 1 ] = 0x39;
  data.tail16 = 0xE20E;
  data.tail32 = 0xE20E6B70;

  std::strncpy( data.name, retainer->name.c_str(), 31 );
  data.name[ 31 ] = '\0';

  server.queueForPlayer( player.getCharacterId(), packet );

  // Also provide IsMarket state during direct retainer interactions.
  sendIsMarket( player );

  Logger::info( "RetainerMgr: Sent RetainerInfo for '{}' (classJob={}) + IsMarket to player {}",
                retainer->name, static_cast< int >( data.classJob ), player.getId() );
}

void RetainerMgr::sendIsMarket( Entity::Player& player, uint32_t marketContext, uint32_t isMarket )
{
  auto& server = Common::Service< World::WorldServer >::ref();

  auto pkt = makeZonePacket< WorldPackets::Server::FFXIVIpcIsMarket >( player.getId() );
  auto& data = pkt->data();
  std::memset( &data, 0, sizeof( data ) );
  data.marketContext = marketContext;
  data.isMarket = isMarket;
  server.queueForPlayer( player.getCharacterId(), pkt );
}

void RetainerMgr::sendRetainerInventory( Entity::Player& player, uint64_t retainerId )
{
  Logger::info( "RetainerMgr::sendRetainerInventory - Called for retainerId {} player {}", retainerId, player.getId() );

  // Track the currently open retainer so inventory operations (e.g., gil transfer)
  // can resolve RetainerGil(12000) updates to a concrete retainerId.
  setActiveRetainer( player, retainerId );

  if( const auto* soldItems = player.getSoldItems() )
  {
    Logger::debug( "RetainerMgr::sendRetainerInventory - player {} soldItemsCount={} (buyback diagnostic)",
                   player.getId(), soldItems->size() );
  }

  // Send player's full inventory (including gil in Currency container) so the client
  // has the player's gil cached when opening the gil transfer UI.
  player.sendInventory();

  const auto retainerOpt = getRetainer( retainerId );
  if( !retainerOpt.has_value() )
  {
    Logger::error( "RetainerMgr::sendRetainerInventory - Retainer {} not found in database!", retainerId );
    return;
  }

  const auto& retainer = *retainerOpt;
  Logger::info( "RetainerMgr::sendRetainerInventory - Found retainer '{}' with {} gil", retainer.name, retainer.gil );

  auto& server = Common::Service< World::WorldServer >::ref();

  // Retail sends 0x010B early in the retainer inventory/gil flow.
  sendMarketRetainerUpdate( player, retainer );

  Logger::info( "RetainerMgr::sendRetainerInventory - Sending inventory packets (no ItemStorage - following retail)" );

  // ============================================================================
  // RETAIL PACKET ANALYSIS (3.35 capture - retainer_summon_gil_view_class.xml):
  // Retail does NOT send ItemStorage (0x01AE) packets for retainer containers!
  // It only sends: GilItem/NormalItem + ItemSize directly.
  //
  // The client appears to handle container initialization implicitly when
  // receiving item data packets with a storageId it hasn't seen before.
  // ============================================================================

  // ============================================================================
  // RETAIL DISCOVERY: The client uses ItemSize.unknown1 to determine the player's
  // gil amount for the RetainerBag(2) gil transfer UI. This field must contain
  // the player's current gil in ALL retainer container ItemSize packets!
  // ============================================================================
  const uint32_t playerGil = player.getCurrency( Common::CurrencyType::Gil );

  // ============================================================================
  // Send retainer's gil (container ID 12000)
  //
  // RETAIL PACKET ANALYSIS (3.35 capture):
  // Retail does NOT send ItemStorage (0x01AE) for RetainerGil containers!
  // It only sends: GilItem (0x01B3) + ItemSize (0x01B0) directly.
  //
  // From capture: GilItem contextId=0x1ae5, storageId=12000, stack=0, subquarity=3, catalogId=1
  //               ItemSize contextId=0x1ae5, size=1, storageId=12000, unknown1=playerGil
  // ============================================================================

  // Send retainer bag containers (10000-10006) FIRST - matching retail order
  for( int i = 0; i < 7; ++i )
  {
    auto bagSequence = player.getNextInventorySequence();

    auto sizePacket = makeZonePacket< WorldPackets::Server::FFXIVIpcItemSize >( player.getId() );
    sizePacket->data().contextId = bagSequence;
    sizePacket->data().size = 0;// Empty bag (TODO: send actual items when implemented)
    sizePacket->data().storageId = static_cast< uint32_t >( Common::InventoryType::RetainerBag0 ) + i;
    sizePacket->data().unknown1 = playerGil;
    server.queueForPlayer( player.getCharacterId(), sizePacket );
  }

  // Send retainer's gil (container ID 12000)
  {
    auto gilSequence = player.getNextInventorySequence();

    // GilItem packet with the actual gil amount (NO ItemStorage packet!)
    auto gilPacket = makeZonePacket< WorldPackets::Server::FFXIVIpcGilItem >( player.getId() );
    gilPacket->data().contextId = gilSequence;
    gilPacket->data().item.catalogId = 1; // Gil item ID
    gilPacket->data().item.subquarity = 3;// Retail uses 3, not 1
    gilPacket->data().item.stack = retainer.gil;
    gilPacket->data().item.storageId = static_cast< uint16_t >( Common::InventoryType::RetainerGil );
    gilPacket->data().item.containerIndex = 0;
    gilPacket->data().padding = 0;
    server.queueForPlayer( player.getCharacterId(), gilPacket );

    // ItemSize to finalize - unknown1 MUST contain player's gil!
    auto sizePacket = makeZonePacket< WorldPackets::Server::FFXIVIpcItemSize >( player.getId() );
    sizePacket->data().contextId = gilSequence;
    sizePacket->data().size = 1;
    sizePacket->data().storageId = static_cast< uint32_t >( Common::InventoryType::RetainerGil );
    sizePacket->data().unknown1 = playerGil;
    server.queueForPlayer( player.getCharacterId(), sizePacket );

    Logger::info( "  -> RetainerGil(12000): contextId={} retainerGil={} playerGil={}",
                  gilSequence, retainer.gil, playerGil );
  }

  // Send retainer crystals (container ID 12001)
  {
    auto crystalSequence = player.getNextInventorySequence();

    auto sizePacket = makeZonePacket< WorldPackets::Server::FFXIVIpcItemSize >( player.getId() );
    sizePacket->data().contextId = crystalSequence;
    sizePacket->data().size = 0;// No crystals
    sizePacket->data().storageId = static_cast< uint32_t >( Common::InventoryType::RetainerCrystal );
    sizePacket->data().unknown1 = playerGil;
    server.queueForPlayer( player.getCharacterId(), sizePacket );
  }

  // Send retainer market listings (container ID 12002)
  {
    auto marketSequence = player.getNextInventorySequence();

    auto sizePacket = makeZonePacket< WorldPackets::Server::FFXIVIpcItemSize >( player.getId() );
    sizePacket->data().contextId = marketSequence;
    sizePacket->data().size = 0;// No market listings
    sizePacket->data().storageId = static_cast< uint32_t >( Common::InventoryType::RetainerMarket );
    sizePacket->data().unknown1 = playerGil;
    server.queueForPlayer( player.getCharacterId(), sizePacket );

    // Retail sends a burst of 0x01AD entries followed by a 0x01AC header.
    // This appears to prime the market-related retainer UI paths even when empty.
    sendMarketPriceSnapshot( player, marketSequence );
  }

  // Send retainer equipped gear (container ID 11000)
  {
    auto gearSequence = player.getNextInventorySequence();

    auto sizePacket = makeZonePacket< WorldPackets::Server::FFXIVIpcItemSize >( player.getId() );
    sizePacket->data().contextId = gearSequence;
    sizePacket->data().size = 0;// No equipped gear
    sizePacket->data().storageId = static_cast< uint32_t >( Common::InventoryType::RetainerEquippedGear );
    sizePacket->data().unknown1 = playerGil;
    server.queueForPlayer( player.getCharacterId(), sizePacket );
  }

  Logger::debug( "RetainerMgr::sendRetainerInventory - Sent inventory for retainer {} (retainerGil: {}, playerGil: {})",
                 retainerId, retainer.gil, playerGil );
}

void RetainerMgr::setActiveRetainer( Entity::Player& player, uint64_t retainerId )
{
  std::scoped_lock lock( m_activeRetainerMutex );
  m_activeRetainerByPlayer[ player.getId() ] = retainerId;
}

void RetainerMgr::clearActiveRetainer( Entity::Player& player )
{
  std::scoped_lock lock( m_activeRetainerMutex );
  m_activeRetainerByPlayer.erase( player.getId() );
}

std::optional< uint64_t > RetainerMgr::getActiveRetainerId( const Entity::Player& player ) const
{
  std::scoped_lock lock( m_activeRetainerMutex );
  const auto it = m_activeRetainerByPlayer.find( player.getId() );
  if( it == m_activeRetainerByPlayer.end() )
    return std::nullopt;
  return it->second;
}

bool RetainerMgr::updateRetainerGil( uint64_t retainerId, uint32_t gil )
{
  auto& db = Common::Service< Db::DbWorkerPool< Db::ZoneDbConnection > >::ref();

  try
  {
    db.directExecute( "UPDATE chararetainerinfo SET Gil = " + std::to_string( gil ) +
                      " WHERE RetainerId = " + std::to_string( retainerId ) );
    return true;
  } catch( const std::exception& e )
  {
    Logger::error( "RetainerMgr: Failed to update retainer gil for {}: {}", retainerId, e.what() );
    return false;
  }
}

void RetainerMgr::sendGetRetainerListResult( Entity::Player& player )
{
  const auto retainers = getRetainers( player );
  const RetainerData* chosen = retainers.empty() ? nullptr : &retainers.front();

  auto& server = Common::Service< World::WorldServer >::ref();

  auto pkt = makeZonePacket< WorldPackets::Server::FFXIVIpcGetRetainerListResult >( player.getId() );
  auto& data = pkt->data();
  std::memset( &data, 0, sizeof( data ) );

  if( chosen )
  {
    data.retainerId = chosen->retainerId;
    data.createTime = chosen->createTime;
    data.unknown12 = 3;
    data.unknown14 = 3;
    std::strncpy( data.name, chosen->name.c_str(), sizeof( data.name ) - 1 );
    data.name[ sizeof( data.name ) - 1 ] = '\0';
  }

  server.queueForPlayer( player.getCharacterId(), pkt );
}

void RetainerMgr::sendMarketRetainerUpdate( Entity::Player& player, const RetainerData& retainer )
{
  auto& server = Common::Service< World::WorldServer >::ref();

  auto pkt = makeZonePacket< WorldPackets::Server::FFXIVIpcMarketRetainerUpdate >( player.getId() );
  auto& data = pkt->data();
  std::memset( &data, 0, sizeof( data ) );

  data.retainerId = retainer.retainerId;
  data.unknown15 = 3;
  data.classJob = retainer.classJob;
  std::strncpy( data.name, retainer.name.c_str(), sizeof( data.name ) - 1 );
  data.name[ sizeof( data.name ) - 1 ] = '\0';

  server.queueForPlayer( player.getCharacterId(), pkt );
}

void RetainerMgr::sendMarketPriceSnapshot( Entity::Player& player, uint32_t requestId )
{
  auto& server = Common::Service< World::WorldServer >::ref();

  // Send a small set of empty-ish price entries to mirror the retail pattern.
  for( int i = 0; i < 11; ++i )
  {
    auto entry = makeZonePacket< WorldPackets::Server::FFXIVIpcMarketPrice >( player.getId() );
    auto& e = entry->data();
    std::memset( &e, 0, sizeof( e ) );
    e.requestId = requestId;
    e.itemCatalogId = static_cast< uint32_t >( Common::InventoryType::RetainerMarket );
    e.unknown12 = 0x68;
    e.unknown16 = 0x12C;
    server.queueForPlayer( player.getCharacterId(), entry );
  }

  auto hdr = makeZonePacket< WorldPackets::Server::FFXIVIpcMarketPriceHeader >( player.getId() );
  auto& h = hdr->data();
  std::memset( &h, 0, sizeof( h ) );
  h.requestId = requestId;
  server.queueForPlayer( player.getCharacterId(), hdr );
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

uint32_t RetainerMgr::spawnRetainer( Entity::Player& player, uint64_t retainerId,
                                     const Common::FFXIVARR_POSITION3* bellPos )
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

  // Check if already spawned - if so, despawn first (client may have despawned without telling server)
  auto existingActor = getSpawnedRetainerActorId( player, retainerId );
  if( existingActor != 0 )
  {
    Logger::debug( "RetainerMgr::spawnRetainer - Retainer {} was tracked as spawned, clearing and respawning",
                   retainerId );
    despawnRetainer( player, existingActor );
  }

  // Generate unique actor ID for this retainer spawn
  // Retainer actor IDs use pattern: 0x40000000 + (player low 16 bits << 8) + retainer slot
  // This ensures they're unique per player and don't clash with other actors
  uint32_t retainerActorId = 0x40000000 | ( ( player.getId() & 0xFFFF ) << 8 ) | ( retainerData.displayOrder & 0xFF );

  // Calculate spawn position
  // If bell position is provided, spawn near the bell; otherwise spawn near player
  auto playerPos = player.getPos();
  auto playerRot = player.getRot();

  Common::FFXIVARR_POSITION3 spawnPos;
  float retainerRot;

  if( bellPos != nullptr )
  {
    // Spawn at the bell's position
    // The retainer will "walk in" via the Lua script animation
    spawnPos = *bellPos;

    // Calculate rotation to face the player
    float dx = playerPos.x - bellPos->x;
    float dz = playerPos.z - bellPos->z;
    retainerRot = std::atan2( dx, dz );

    Logger::debug( "RetainerMgr::spawnRetainer - Using bell position ({:.2f}, {:.2f}, {:.2f})",
                   bellPos->x, bellPos->y, bellPos->z );
  }
  else
  {
    // Fallback: spawn 2 yalms in front of player (legacy behavior)
    spawnPos.x = playerPos.x + ( std::sin( playerRot ) * 2.0f );
    spawnPos.y = playerPos.y;
    spawnPos.z = playerPos.z + ( std::cos( playerRot ) * 2.0f );

    // Face retainer towards player
    retainerRot = playerRot + 3.14159265f;// 180 degrees opposite

    Logger::debug( "RetainerMgr::spawnRetainer - No bell position, using player-relative position" );
  }

  // Send spawn packet
  auto& server = Common::Service< World::WorldServer >::ref();
  auto spawnPacket = std::make_shared< WorldPackets::Server::RetainerSpawnPacket >(
          retainerActorId,
          retainerData,
          spawnPos,
          retainerRot,
          player );

  server.queueForPlayer( player.getCharacterId(), spawnPacket );

  // Track spawned retainer
  m_spawnedRetainers[ player.getId() ][ retainerId ] = retainerActorId;

  Logger::info( "RetainerMgr::spawnRetainer - Spawned retainer {} '{}' as actor {} for player {}",
                retainerId, retainerData.name, retainerActorId, player.getId() );

  return retainerActorId;
}

void RetainerMgr::despawnRetainer( Entity::Player& player, uint32_t retainerActorId )
{
  auto playerIt = m_spawnedRetainers.find( player.getId() );
  if( playerIt == m_spawnedRetainers.end() )
    return;

  // Find and remove the retainer
  for( auto it = playerIt->second.begin(); it != playerIt->second.end(); ++it )
  {
    if( it->second == retainerActorId )
    {
      Logger::info( "RetainerMgr::despawnRetainer - Despawning retainer actor {} for player {}",
                    retainerActorId, player.getId() );

      // Send ActorFreeSpawn packet to client and free spawn index
      player.freePlayerSpawnId( retainerActorId );

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

  return 255;// All slots used
}
