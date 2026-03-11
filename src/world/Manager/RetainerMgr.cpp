#include "RetainerMgr.h"

#include <Common.h>
#include <Service.h>
#include <Database/DatabaseDef.h>
#include <Logging/Logger.h>
#include <Network/GamePacket.h>
#include <Network/PacketDef/Zone/ServerZoneDef.h>
#include <Network/PacketWrappers/RetainerSpawnPacket.h>
#include <Network/Util/PacketUtil.h>
#include <Network/CommonActorControl.h>

#include "Manager/TerritoryMgr.h"
#include "Territory/Territory.h"

#include <map>
#include <cmath>
#include <algorithm>
#include <ctime>
#include <mutex>

#include "Actor/Player.h"
#include "Inventory/Item.h"
#include "Inventory/ItemContainer.h"
#include "Manager/InventoryMgr.h"
#include "Manager/ItemMgr.h"
#include "Network/GameConnection.h"
#include "WorldServer.h"

using namespace Sapphire;
using namespace Sapphire::World::Manager;
using namespace Sapphire::Network::Packets;
using namespace Sapphire::Network::ActorControl;

namespace
{
  void LogRetainerPacketOffsetsOnce()
  {
    static std::once_flag s_once;
    std::call_once( s_once, []() {
      Logger::info( "RetainerPacketLayout: FFXIVIpcNormalItem size={} contextId@{} unknown1@{} item@{} storageId@{} containerIndex@{} stack@{} catalogId@{}",
                    sizeof( WorldPackets::Server::FFXIVIpcNormalItem ),
                    offsetof( WorldPackets::Server::FFXIVIpcNormalItem, contextId ),
                    offsetof( WorldPackets::Server::FFXIVIpcNormalItem, unknown1 ),
                    offsetof( WorldPackets::Server::FFXIVIpcNormalItem, item ),
                    offsetof( WorldPackets::Server::ZoneProtoDownNormalItem, storageId ),
                    offsetof( WorldPackets::Server::ZoneProtoDownNormalItem, containerIndex ),
                    offsetof( WorldPackets::Server::ZoneProtoDownNormalItem, stack ),
                    offsetof( WorldPackets::Server::ZoneProtoDownNormalItem, catalogId ) );

      Logger::info( "RetainerPacketLayout: FFXIVIpcItemSize size={} contextId@{} size@{} storageId@{} unknown1@{}",
                    sizeof( WorldPackets::Server::FFXIVIpcItemSize ),
                    offsetof( WorldPackets::Server::FFXIVIpcItemSize, contextId ),
                    offsetof( WorldPackets::Server::FFXIVIpcItemSize, size ),
                    offsetof( WorldPackets::Server::FFXIVIpcItemSize, storageId ),
                    offsetof( WorldPackets::Server::FFXIVIpcItemSize, unknown1 ) );

      Logger::info( "RetainerPacketLayout: FFXIVIpcRetainerData size={} handlerId@{} unknown1@{} retainerId@{} slotIndex@{} sellingCount@{} activeFlag@{} classJob@{} unknown3@{} unknown4@{} personality@{} name@{} tail16@{} tail32@{}",
                    sizeof( WorldPackets::Server::FFXIVIpcRetainerData ),
                    offsetof( WorldPackets::Server::FFXIVIpcRetainerData, handlerId ),
                    offsetof( WorldPackets::Server::FFXIVIpcRetainerData, unknown1 ),
                    offsetof( WorldPackets::Server::FFXIVIpcRetainerData, retainerId ),
                    offsetof( WorldPackets::Server::FFXIVIpcRetainerData, slotIndex ),
                    offsetof( WorldPackets::Server::FFXIVIpcRetainerData, sellingCount ),
                    offsetof( WorldPackets::Server::FFXIVIpcRetainerData, activeFlag ),
                    offsetof( WorldPackets::Server::FFXIVIpcRetainerData, classJob ),
                    offsetof( WorldPackets::Server::FFXIVIpcRetainerData, unknown3 ),
                    offsetof( WorldPackets::Server::FFXIVIpcRetainerData, unknown4 ),
                    offsetof( WorldPackets::Server::FFXIVIpcRetainerData, personality ),
                    offsetof( WorldPackets::Server::FFXIVIpcRetainerData, name ),
                    offsetof( WorldPackets::Server::FFXIVIpcRetainerData, tail16 ),
                    offsetof( WorldPackets::Server::FFXIVIpcRetainerData, tail32 ) );

      Logger::info( "RetainerPacketLayout: FFXIVIpcMarketRetainerUpdate size={} retainerId@{} unknown15@{} classJob@{} name@{}",
                    sizeof( WorldPackets::Server::FFXIVIpcMarketRetainerUpdate ),
                    offsetof( WorldPackets::Server::FFXIVIpcMarketRetainerUpdate, retainerId ),
                    offsetof( WorldPackets::Server::FFXIVIpcMarketRetainerUpdate, unknown15 ),
                    offsetof( WorldPackets::Server::FFXIVIpcMarketRetainerUpdate, classJob ),
                    offsetof( WorldPackets::Server::FFXIVIpcMarketRetainerUpdate, name ) );
    } );
  }

  bool IsRetainerStorageId( const uint16_t storageId )
  {
    return storageId >= static_cast< uint16_t >( Common::InventoryType::RetainerBag0 ) &&
           storageId <= static_cast< uint16_t >( Common::InventoryType::RetainerMarket );
  }

  uint32_t ResolveRetainerCreateTime( uint32_t createTime )
  {
    if( createTime != 0 )
      return createTime;

    const auto now = static_cast< uint32_t >( std::time( nullptr ) );
    return now != 0 ? now : 1u;
  }

  void InitializeRetainerDataPacket( WorldPackets::Server::FFXIVIpcRetainerData& data,
                                     uint32_t handlerId,
                                     uint8_t slotIndex )
  {
    std::memset( &data, 0, sizeof( data ) );
    data.handlerId = handlerId;
    data.unknown1 = 0xFFFFFFFF;
    data.slotIndex = slotIndex;
    data.personality = 0x99;
    data.padding1[ 0 ] = 0x68;
    data.padding1[ 1 ] = 0x0E;
    data.padding1[ 2 ] = 0xE2;
    data.padding2[ 0 ] = 0x00;
    data.padding2[ 1 ] = 0x39;
    data.tail16 = 0xE20E;
    data.tail32 = 0xE20E6B70;
  }

  void PopulateRetainerDataPacket( WorldPackets::Server::FFXIVIpcRetainerData& data, const RetainerData& retainer )
  {
    data.retainerId = retainer.retainerId;
    data.sellingCount = retainer.sellingCount;
    data.activeFlag = 1;
    data.classJob = retainer.classJob;
    data.unknown3 = ResolveRetainerCreateTime( retainer.createTime );
    data.unknown4 = retainer.level > 0 ? retainer.level : 1;
    data.personality = static_cast< uint8_t >( retainer.personality );

    std::strncpy( data.name, retainer.name.c_str(), 31 );
    data.name[ 31 ] = '\0';
  }

  uint32_t GetRetainerNormalItemUnknown1( const uint16_t storageId )
  {
    if( storageId >= static_cast< uint16_t >( Common::InventoryType::RetainerBag0 ) &&
        storageId <= static_cast< uint16_t >( Common::InventoryType::RetainerBag6 ) )
    {
      // Retail 3.35 capture shows 0x77 for retainer bag NormalItem entries.
      return 0x77u;
    }

    if( storageId == static_cast< uint16_t >( Common::InventoryType::RetainerMarket ) ||
        storageId == static_cast< uint16_t >( Common::InventoryType::RetainerEquippedGear ) )
    {
      // Retail 3.35 capture uses RetainerGil (12000) for market/equip entries.
      return static_cast< uint32_t >( Common::InventoryType::RetainerGil );
    }

    return 0u;
  }

  uint32_t GetRetainerIsMarketFlag( const RetainerData& retainer )
  {
    // Retail 3.35 capture shows isMarket = 0 when the retainer is not registered to a city market.
    return retainer.cityId != 0 ? 1u : 0u;
  }

  void SendRetainerContainer( Entity::Player& player, const ItemContainerPtr& container, const uint32_t sequence )
  {
    if( !container )
      return;

    auto& server = Common::Service< World::WorldServer >::ref();

    const uint16_t containerId = container->getId();
    const auto& itemMap = container->getItemMap();

    const bool isCrystalContainer = containerId == Common::InventoryType::RetainerCrystal;
    const bool isCurrencyOrCrystal = isCrystalContainer;

    const uint32_t normalItemUnknown1 = GetRetainerNormalItemUnknown1( containerId );
    const uint32_t maxSize = container->getMaxSize();

    Logger::debug( "RetainerMgr::SendRetainerContainer - seq={} storageId={} unknown1=0x{:08X} maxSize={} items={}",
                   sequence,
                   static_cast< unsigned int >( containerId ),
                   normalItemUnknown1,
                   maxSize,
                   static_cast< unsigned int >( itemMap.size() ) );

    uint32_t itemCount = 0;
    for( const auto& [ slotIndex, item ] : itemMap )
    {
      if( !item )
        continue;

      ++itemCount;

      if( isCurrencyOrCrystal )
      {
        auto currencyInfoPacket = makeZonePacket< WorldPackets::Server::FFXIVIpcGilItem >( player.getId() );
        auto& data = currencyInfoPacket->data();
        std::memset( &data, 0, sizeof( data ) );
        data.contextId = sequence;
        data.item.catalogId = item->getId();
        data.item.subquarity = 1;
        data.item.stack = item->getStackSize();
        data.item.storageId = containerId;
        data.item.containerIndex = static_cast< uint16_t >( slotIndex );

        server.queueForPlayer( player.getCharacterId(), currencyInfoPacket );
        continue;
      }

      auto itemInfoPacket = makeZonePacket< WorldPackets::Server::FFXIVIpcNormalItem >( player.getId() );
      auto& data = itemInfoPacket->data();
      std::memset( &data, 0, sizeof( data ) );
      data.contextId = sequence;
      data.unknown1 = normalItemUnknown1;
      data.item.storageId = containerId;
      data.item.containerIndex = static_cast< uint16_t >( slotIndex );
      data.item.stack = item->getStackSize();
      data.item.catalogId = item->getId();
      data.item.durability = item->getDurability();
      data.item.flags = static_cast< uint8_t >( item->isHq() ? 1 : 0 );
      data.item.stain = static_cast< uint8_t >( item->getStain() );
      data.item.pattern = item->getPattern();
      data.item.signatureId = 0;
      data.item.refine = item->getSpiritbond();

      Logger::debug( "RetainerMgr::SendRetainerContainer - seq={} storageId={} idx={} stack={} catalogId={} unknown1=0x{:08X}",
                     sequence,
                     static_cast< unsigned int >( containerId ),
                     static_cast< unsigned int >( slotIndex ),
                     item->getStackSize(),
                     item->getId(),
                     normalItemUnknown1 );

      server.queueForPlayer( player.getCharacterId(), itemInfoPacket );
    }

    auto itemSizePacket = makeZonePacket< WorldPackets::Server::FFXIVIpcItemSize >( player.getId() );
    auto& sizeData = itemSizePacket->data();
    std::memset( &sizeData, 0, sizeof( sizeData ) );
    sizeData.contextId = sequence;
    sizeData.size = static_cast< int32_t >( maxSize );
    sizeData.storageId = containerId;
    sizeData.unknown1 = 0;

    Logger::debug( "RetainerMgr::SendRetainerContainer - seq={} storageId={} itemCount={} sizeSent={}",
                   sequence,
                   static_cast< unsigned int >( containerId ),
                   itemCount,
                   maxSize );

    server.queueForPlayer( player.getCharacterId(), itemSizePacket );
  }

  uint32_t GetRetainerCountForOwner( Db::DbWorkerPool< Db::ZoneDbConnection >& db, uint64_t ownerId )
  {
    auto stmt = db.getPreparedStatement( Db::ZoneDbStatements::CHARA_RETAINER_COUNT );
    stmt->setUInt64( 1, ownerId );
    auto res = db.query( stmt );

    if( res && res->next() )
      return res->getUInt( "cnt" );

    return 0u;
  }

  uint64_t ResolveRetainerOwnerCharacterId( Entity::Player& player, Db::DbWorkerPool< Db::ZoneDbConnection >& db )
  {
    const uint64_t characterId = player.getCharacterId();
    const uint64_t legacyOwnerId = static_cast< uint64_t >( player.getId() );

    if( GetRetainerCountForOwner( db, characterId ) > 0 )
      return characterId;

    if( legacyOwnerId == characterId )
      return characterId;

    if( GetRetainerCountForOwner( db, legacyOwnerId ) == 0 )
      return characterId;

    try
    {
      db.directExecute( "UPDATE chararetainerinfo SET CharacterId = " + std::to_string( characterId ) +
                        " WHERE CharacterId = " + std::to_string( legacyOwnerId ) + ";" );
      Logger::info( "RetainerMgr: Migrated retainer ownership from legacy CharacterId {} to {}", legacyOwnerId, characterId );
    } catch( const std::exception& e )
    {
      Logger::error( "RetainerMgr: Failed migrating retainer ownership from {} to {}: {}", legacyOwnerId, characterId, e.what() );
      return characterId;
    }

    return characterId;
  }
}// namespace

uint32_t RetainerMgr::MakeRetainerInventoryOwnerKey( const uint64_t retainerId )
{
  // Keep retainer inventory rows out of the normal CharacterId namespace.
  // We only need per-retainer uniqueness for storage persistence.
  // Mask to 31 bits to keep it in-range for typical INT columns.
  return 0x40000000u | static_cast< uint32_t >( retainerId & 0x3FFFFFFFu );
}

static void ClearContainerItems( const ItemContainerPtr& container )
{
  if( !container )
    return;
  // Important: clear the map entirely.
  // Resetting shared_ptrs leaves null entries behind, which can confuse
  // code paths that rely on getEntryCount()/map size for UI refresh logic.
  container->getItemMap().clear();
}

static void ClearRetainerContainers( Entity::Player& player )
{
  for( uint16_t i = 0; i < 7; ++i )
    ClearContainerItems( player.getInventoryContainer( static_cast< uint16_t >( Common::InventoryType::RetainerBag0 ) + i ) );

  ClearContainerItems( player.getInventoryContainer( static_cast< uint16_t >( Common::InventoryType::RetainerEquippedGear ) ) );
  ClearContainerItems( player.getInventoryContainer( static_cast< uint16_t >( Common::InventoryType::RetainerCrystal ) ) );
  ClearContainerItems( player.getInventoryContainer( static_cast< uint16_t >( Common::InventoryType::RetainerMarket ) ) );
}

static void LoadRetainerContainersFromDb( Entity::Player& player, const uint64_t retainerId )
{
  auto& itemMgr = Common::Service< World::Manager::ItemMgr >::ref();
  auto& db = Common::Service< Db::DbWorkerPool< Db::ZoneDbConnection > >::ref();

  const uint32_t ownerKey = RetainerMgr::MakeRetainerInventoryOwnerKey( retainerId );

  const std::string query =
          "SELECT storageId, "
          "container_0, "
          "container_1, "
          "container_2, "
          "container_3, "
          "container_4, "
          "container_5, "
          "container_6, "
          "container_7, "
          "container_8, "
          "container_9, "
          "container_10, "
          "container_11, "
          "container_12, "
          "container_13, "
          "container_14, "
          "container_15, "
          "container_16, "
          "container_17, "
          "container_18, "
          "container_19, "
          "container_20, "
          "container_21, "
          "container_22, "
          "container_23, "
          "container_24, "
          "container_25, "
          "container_26, "
          "container_27, "
          "container_28, "
          "container_29, "
          "container_30, "
          "container_31, "
          "container_32, "
          "container_33, "
          "container_34 "
          "FROM charaiteminventory WHERE CharacterId = " +
          std::to_string( ownerKey ) +
          " ORDER BY storageId ASC;";

  auto res = db.query( query );

  while( res->next() )
  {
    const uint16_t storageId = res->getUInt16( 1 );
    if( !IsRetainerStorageId( storageId ) )
      continue;

    auto container = player.getInventoryContainer( storageId );
    if( !container )
      continue;

    uint32_t loadedCount = 0;

    for( uint32_t i = 1; i <= container->getMaxSize(); ++i )
    {
      const uint64_t uItemId = res->getUInt64( i + 1 );
      if( uItemId == 0 )
        continue;

      ItemPtr item = itemMgr.loadItem( uItemId );
      if( !item )
        continue;

      container->getItemMap()[ i - 1 ] = item;
      ++loadedCount;
    }
  }
}

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
  const uint64_t characterId = player.getCharacterId();

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
    Logger::warn( "RetainerMgr: Character {} has reached max retainer limit", characterId );
    return std::nullopt;
  }

  // Get next available slot
  uint8_t slot = getNextAvailableSlot( player );
  if( slot == 255 )
  {
    Logger::warn( "RetainerMgr: No available slots for character {}", characterId );
    return std::nullopt;
  }

  // Insert into database
  auto& db = Common::Service< Db::DbWorkerPool< Db::ZoneDbConnection > >::ref();
  auto stmt = db.getPreparedStatement( Db::ZoneDbStatements::CHARA_RETAINER_INS );
  stmt->setUInt64( 1, characterId );                             // CharacterId
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
                   name, characterId, e.what() );
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
                  name, retainerId, characterId, slot, static_cast< int >( personality ) );
    return retainerId;
  }

  Logger::error( "RetainerMgr: Failed to retrieve retainer ID after insert for '{}'", name );
  return std::nullopt;
}

RetainerError RetainerMgr::deleteRetainer( Entity::Player& player, uint64_t retainerId )
{
  const uint64_t characterId = player.getCharacterId();
  auto retainer = getRetainer( retainerId );
  if( !retainer.has_value() )
  {
    return RetainerError::NotFound;
  }

  if( retainer->ownerId != characterId )
  {
    Logger::warn( "RetainerMgr: Player {} attempted to delete retainer {} they don't own",
                  characterId, retainerId );
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
                retainerId, retainer->name, characterId );

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

  const uint64_t characterId = ResolveRetainerOwnerCharacterId( player, db );

  auto stmt = db.getPreparedStatement( Db::ZoneDbStatements::CHARA_RETAINER_SEL_BY_CHARID );
  stmt->setUInt64( 1, characterId );
  auto res = db.query( stmt );

  while( res && res->next() )
  {
    RetainerData data;
    data.retainerId = res->getUInt64( "RetainerId" );
    data.ownerId = res->getUInt64( "CharacterId" );
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

    if( !res->isNull( "Customize" ) )
    {
      auto blobData = res->getBlobVector( "Customize" );
      if( blobData.size() == 26 )
        data.customize.assign( blobData.begin(), blobData.end() );
    }

    retainers.push_back( data );
  }

  Logger::debug( "RetainerMgr: Loaded {} retainers for character {}", retainers.size(), characterId );
  return retainers;
}

std::optional< RetainerData > RetainerMgr::getRetainer( uint64_t retainerId )
{
  return loadRetainerFromDb( retainerId );
}

uint8_t RetainerMgr::getRetainerCount( Entity::Player& player )
{
  auto& db = Common::Service< Db::DbWorkerPool< Db::ZoneDbConnection > >::ref();
  const uint64_t characterId = ResolveRetainerOwnerCharacterId( player, db );

  const uint32_t count = GetRetainerCountForOwner( db, characterId );
  return static_cast< uint8_t >( count );
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
  if( !retainer.has_value() || retainer->ownerId != player.getCharacterId() )
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
  const auto retainer = getRetainer( retainerId );
  if( !retainer || retainer->ownerId != player.getCharacterId() )
    return RetainerError::NotFound;

  const bool isValidClass = ( classJob >= 1 && classJob <= 7 ) ||  // DoW/DoM base classes
                            ( classJob >= 16 && classJob <= 18 ) ||// DoL classes
                            ( classJob == 26 );                    // Arcanist
  if( !isValidClass )
  {
    Logger::warn( "RetainerMgr::setRetainerClass - Invalid classJob {} for retainer {}", classJob, retainerId );
    return RetainerError::None;
  }

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
    return RetainerError::None;
  }

  const auto updatedRetainer = getRetainer( retainerId );
  if( updatedRetainer )
  {
    Logger::info( "RetainerMgr::setRetainerClass - DB updated, new classJob={}",
                  static_cast< int >( updatedRetainer->classJob ) );
  }
  else
  {
    Logger::warn( "RetainerMgr::setRetainerClass - DB updated but retainer reload failed for {}", retainerId );
  }

  const uint32_t retainerActorId = getSpawnedRetainerActorId( player, retainerId );
  if( retainerActorId != 0 )
  {
    Network::Util::Packet::sendActorControl( player, retainerActorId,
                                             Network::ActorControl::ClassJobChange, classJob );
    Logger::info( "RetainerMgr::setRetainerClass - Sent ClassJobChange ActorControl for actor {:08X} -> classJob={}",
                  retainerActorId, classJob );
  }

  if( updatedRetainer )
  {
    sendRetainerInfo( player, retainerId );
    // Note: Do NOT send GetRetainerListResult proactively here. Retail only sends
    // 0x0106 (GetRetainerListResult) in response to client's 0x0106 request, not
    // as a result of class changes. The packet handler in PacketHandlers.cpp
    // already handles the request/response flow correctly.
  }

  Logger::info( "RetainerMgr::setRetainerClass - Set retainer {} '{}' classJob to {}",
                retainerId, updatedRetainer ? updatedRetainer->name : "?", classJob );
  return RetainerError::None;
}

void RetainerMgr::addRetainerExp( uint64_t retainerId, uint32_t exp )
{
  auto retainer = getRetainer( retainerId );
  if( !retainer.has_value() )
    return;

  retainer->exp += exp;
  ( void ) saveRetainerToDb( *retainer );
}

// ========== Venture Management ==========

RetainerError RetainerMgr::startVenture( Entity::Player& player, uint64_t retainerId, uint32_t ventureId )
{
  auto retainer = getRetainer( retainerId );
  if( !retainer.has_value() || retainer->ownerId != player.getCharacterId() )
    return RetainerError::NotFound;

  if( retainer->ventureId != 0 && !retainer->ventureComplete )
    return RetainerError::RetainerBusy;

  if( ventureId == 0 )
    return RetainerError::InvalidSlot;

  const auto now = static_cast< uint32_t >( time( nullptr ) );
  constexpr uint32_t defaultDurationSeconds = 60 * 60;

  retainer->ventureId = ventureId;
  retainer->ventureComplete = false;
  retainer->ventureStartTime = now;
  retainer->ventureCompleteTime = now + defaultDurationSeconds;

  if( !saveRetainerToDb( *retainer ) )
  {
    Logger::error( "RetainerMgr::startVenture - Failed to persist venture (retainerId={}, ventureId={})",
                   retainerId,
                   ventureId );
    return RetainerError::DatabaseError;
  }

  sendRetainerInfo( player, retainerId );
  return RetainerError::None;
}

RetainerError RetainerMgr::completeVenture( Entity::Player& player, uint64_t retainerId )
{
  auto retainer = getRetainer( retainerId );
  if( !retainer.has_value() || retainer->ownerId != player.getCharacterId() )
    return RetainerError::NotFound;

  if( retainer->ventureId == 0 )
    return RetainerError::None;

  const auto now = static_cast< uint32_t >( time( nullptr ) );
  if( retainer->ventureCompleteTime == 0 || now < retainer->ventureCompleteTime )
    return RetainerError::None;

  if( retainer->ventureComplete )
    return RetainerError::None;

  retainer->ventureComplete = true;

  if( !saveRetainerToDb( *retainer ) )
  {
    Logger::error( "RetainerMgr::completeVenture - Failed to persist completion (retainerId={})", retainerId );
    return RetainerError::DatabaseError;
  }

  sendRetainerInfo( player, retainerId );
  return RetainerError::None;
}

RetainerError RetainerMgr::cancelVenture( Entity::Player& player, uint64_t retainerId )
{
  auto retainer = getRetainer( retainerId );
  if( !retainer.has_value() || retainer->ownerId != player.getCharacterId() )
    return RetainerError::NotFound;

  retainer->ventureId = 0;
  retainer->ventureComplete = false;
  retainer->ventureStartTime = 0;
  retainer->ventureCompleteTime = 0;

  if( !saveRetainerToDb( *retainer ) )
  {
    Logger::error( "RetainerMgr::cancelVenture - Failed to persist cancel (retainerId={})", retainerId );
    return RetainerError::DatabaseError;
  }

  sendRetainerInfo( player, retainerId );
  return RetainerError::None;
}

// ========== Market Board ==========

uint32_t RetainerMgr::registerToMarket( Entity::Player& player, uint64_t retainerId, uint8_t cityId )
{
  auto retainer = getRetainer( retainerId );
  if( !retainer.has_value() || retainer->ownerId != player.getCharacterId() )
    return 1;

  if( retainer->cityId == cityId )
    return 463;

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
    return 1;
  }

  Logger::info( "RetainerMgr::registerToMarket - Registered retainer {} '{}' to city {}",
                retainerId, retainer->name, cityId );
  return 0;
}

RetainerError RetainerMgr::listItem( Entity::Player& player, uint64_t retainerId,
                                     uint32_t itemId, uint32_t price, uint32_t stack )
{
  auto retainer = getRetainer( retainerId );
  if( !retainer.has_value() || retainer->ownerId != player.getCharacterId() )
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
  ( void ) player;
  ( void ) retainerId;
  ( void ) listingId;
  return RetainerError::None;
}

void RetainerMgr::sendRetainerList( Entity::Player& player, uint32_t handlerId, bool sendGatingPackets, bool sendEventStart )
{
  const auto retainers = getRetainers( player );
  auto& server = Common::Service< World::WorldServer >::ref();

  constexpr uint8_t kWireMaxSlots = 8;
  const uint8_t employedCount = static_cast< uint8_t >( ( std::min )( retainers.size(), static_cast< size_t >( kWireMaxSlots ) ) );
  const uint8_t maxAllowedSlots = ( std::min )( getMaxRetainerSlots( player ), kWireMaxSlots );

  if( handlerId == 0 )
    handlerId = ( player.getId() & 0xFFFF ) | 0x1A00;

  if( sendGatingPackets )
  {
    // Note: The 0142 ActorControl gating packet is now sent from EventHandlers.cpp
    // before eventStart() to match retail packet order (0142 -> 01CC -> 01A3).
    // The 01A3 Condition packet is sent by eventStart() via setCondition().

    if( sendEventStart )
    {
      auto eventStartPkt = makeZonePacket< WorldPackets::Server::FFXIVIpcEventStart >( player.getId() );
      auto& eventData = eventStartPkt->data();
      std::memset( &eventData, 0, sizeof( eventData ) );
      eventData.targetId = player.getId();
      eventData.handlerId = 0x000B000A;
      eventData.event = 0x01;
      eventData.flags = 0x00;
      eventData.eventArg = 0xdc6d0de2;
      server.queueForPlayer( player.getCharacterId(), eventStartPkt );
    }
  }

  std::array< const RetainerData*, 8 > slotToRetainer{};
  slotToRetainer.fill( nullptr );
  for( const auto& r : retainers )
  {
    if( r.displayOrder < slotToRetainer.size() )
      slotToRetainer[ r.displayOrder ] = &r;
  }

  for( uint8_t slot = 0; slot < slotToRetainer.size(); ++slot )
  {
    auto infoPacket = makeZonePacket< WorldPackets::Server::FFXIVIpcRetainerData >( player.getId() );
    auto& info = infoPacket->data();
    InitializeRetainerDataPacket( info, handlerId, slot );

    if( const RetainerData* r = slotToRetainer[ slot ] )
      PopulateRetainerDataPacket( info, *r );

    server.queueForPlayer( player.getCharacterId(), infoPacket );
  }

  auto listPacket = makeZonePacket< WorldPackets::Server::FFXIVIpcRetainerList >( player.getId() );
  auto& listData = listPacket->data();
  std::memset( &listData, 0, sizeof( listData ) );

  listData.handlerId = handlerId;
  listData.maxSlots = kWireMaxSlots;
  // Retail 3.35 capture shows this byte reflects the account's allowed retainer slots
  // (base 2 + any purchased extras), not (maxSlots - employed) as previously assumed.
  // The client can derive employed count from the 0x01AB slot table.
  ( void ) employedCount;
  listData.retainerCount = maxAllowedSlots;
  listData.padding = 0;

  server.queueForPlayer( player.getCharacterId(), listPacket );

  // Note: SetRetainerCount (0x0143/0x98) is sent from CmnDefRetainerCall::onYield
  // at YIELD_CALL_RETAINER to match retail packet order (after scene yield).
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
  InitializeRetainerDataPacket( data, handlerId, slotIndex > 0 ? slotIndex : retainer->displayOrder );
  PopulateRetainerDataPacket( data, *retainer );

  server.queueForPlayer( player.getCharacterId(), packet );
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

void RetainerMgr::sendRetainerConditionOpen( Entity::Player& player )
{
  auto& server = Common::Service< World::WorldServer >::ref();

  // Retail 3.35 capture (#00006): flags[12]={02 00 00 00 00 04 00 00 00 00 04 B3}, padding=0x8EE56B90
  static constexpr std::array< uint8_t, 12 > kFlags{ 0x02, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x04, 0xB3 };
  static constexpr uint32_t kPadding = 0x8EE56B90;

  auto pkt = makeZonePacket< WorldPackets::Server::FFXIVIpcCondition >( player.getId() );
  auto& data = pkt->data();
  std::memset( &data, 0, sizeof( data ) );
  std::memcpy( data.flags, kFlags.data(), kFlags.size() );
  data.padding = kPadding;
  server.queueForPlayer( player.getCharacterId(), pkt );
}

void RetainerMgr::sendRetainerConditionClose( Entity::Player& player )
{
  auto& server = Common::Service< World::WorldServer >::ref();

  // Retail 3.35 capture (#00700): flags[12]={02 00 00 00 00 00 00 00 00 00 29 00}, padding=0x30
  static constexpr std::array< uint8_t, 12 > kFlags{ 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x29, 0x00 };
  static constexpr uint32_t kPadding = 0x00000030;

  auto pkt = makeZonePacket< WorldPackets::Server::FFXIVIpcCondition >( player.getId() );
  auto& data = pkt->data();
  std::memset( &data, 0, sizeof( data ) );
  std::memcpy( data.flags, kFlags.data(), kFlags.size() );
  data.padding = kPadding;
  server.queueForPlayer( player.getCharacterId(), pkt );
}

void RetainerMgr::sendRetainerSessionClose( Entity::Player& player, uint32_t handlerId )
{
  // Retail 3.35 close sequence starts with ResumeEventScene2 (0x01D8) in the *event* context.
  // This handlerId is the RetainerCall event (0x000B000A), not the 0x1Axx handler used by 0x01AA/0x01AB.
  if( handlerId == 0 )
    handlerId = 0x000B000A;

  // Retail capture shows resumeId=0x0E and a non-zero arg0 in this packet.
  // This value appears constant across observed captures.
  sendRetainerResumeScene2( player, handlerId, 0x0E, 0xE20B2858, 0 );
}

void RetainerMgr::sendRetainerSessionMarker( Entity::Player& player )
{
  auto& server = Common::Service< World::WorldServer >::ref();

  auto pkt = makeZonePacket< WorldPackets::Server::FFXIVIpcSessionMarker >( player.getId() );
  auto& data = pkt->data();
  std::memset( &data, 0, sizeof( data ) );

  // Retail capture observed taskCount = 4 in this packet near retainer close.
  data.taskCount = 4;

  server.queueForPlayer( player.getCharacterId(), pkt );
}

void RetainerMgr::sendRetainerActorControlCloseGate( Entity::Player& player )
{
  auto& server = Common::Service< World::WorldServer >::ref();

  // Retail 3.35 capture (#00701): msg=0x0142 payload
  //   category=0x01F8 padding=0xE20C param1=0 param2=0 param3=0 param4=0 param5=0x20
  auto pkt = makeZonePacket< WorldPackets::Server::FFXIVIpcActorControl >( player.getId() );
  auto& data = pkt->data();
  std::memset( &data, 0, sizeof( data ) );
  data.category = 0x01F8;
  data.padding = 0xE20C;
  data.param5 = 0x20;
  server.queueForPlayer( player.getCharacterId(), pkt );
}

void RetainerMgr::sendRetainerOrderMySelfAttributes( Entity::Player& player )
{
  auto& server = Common::Service< World::WorldServer >::ref();

  // Retail 3.35 capture: an IsMarket (0x01EF) precedes a burst of 0x0143 packets
  // before the ResumeEventScene4/Inspect flow.
  sendIsMarket( player );

  auto emitOrderMySelf = [ & ]() {
    auto pkt = makeZonePacket< WorldPackets::Server::FFXIVIpcActorControlSelf >( player.getId() );
    auto& data = pkt->data();
    std::memset( &data, 0, sizeof( data ) );
    data.category = 0x0018;
    data.padding = 0x8E50;
    data.param1 = 0x00093A80;
    data.param6 = 0x8E50A690;
    server.queueForPlayer( player.getCharacterId(), pkt );
  };

  // Retail sends seven consecutive 0x0143 packets with identical payloads.
  for( int i = 0; i < 7; ++i )
    emitOrderMySelf();
}

void RetainerMgr::sendRetainerOrderMySelfClose( Entity::Player& player )
{
  auto& server = Common::Service< World::WorldServer >::ref();

  // Retail 3.35 capture (#00703): msg=0x0143 payload used during close/unbusy
  //   category=0x0207 padding=0xE20C param1=0xEFBEADDE param2=0 param3=0x00000400 param6=0xE20C1738
  auto pkt = makeZonePacket< WorldPackets::Server::FFXIVIpcActorControlSelf >( player.getId() );
  auto& data = pkt->data();
  std::memset( &data, 0, sizeof( data ) );
  data.category = 0x0207;
  data.padding = 0xE20C;
  data.param1 = 0xEFBEADDE;
  data.param3 = 0x00000400;
  data.param6 = 0xE20C1738;
  server.queueForPlayer( player.getCharacterId(), pkt );
}

void RetainerMgr::sendRetainerResumeScene2( Entity::Player& player, uint32_t handlerId, uint8_t resumeId,
                                            uint32_t arg0, uint32_t arg1 )
{
  auto& server = Common::Service< World::WorldServer >::ref();

  if( handlerId == 0 )
    handlerId = 0x000B000A;

  auto resumePkt = makeZonePacket< WorldPackets::Server::FFXIVIpcResumeEventScene2 >( player.getId() );
  auto& data = resumePkt->data();
  std::memset( &data, 0, sizeof( data ) );
  data.handlerId = handlerId;
  data.sceneId = 0;
  data.resumeId = resumeId;
  data.numOfArgs = arg1 != 0 ? 2 : 1;
  data.args[ 0 ] = arg0;
  if( data.numOfArgs > 1 )
    data.args[ 1 ] = arg1;
  server.queueForPlayer( player.getCharacterId(), resumePkt );
}

void RetainerMgr::sendRetainerEquip( Entity::Player& player, const RetainerData& retainer )
{
  // Send retainer's currently equipped gear as 0x01A5 (Equip packet).
  const uint32_t retainerActorId = getSpawnedRetainerActorId( player, retainer.retainerId );
  if( retainerActorId == 0 )
  {
    Logger::warn( "RetainerMgr::sendRetainerEquip - Retainer {} not spawned for player {}",
                  retainer.retainerId, player.getId() );
    return;
  }

  auto& server = Common::Service< World::WorldServer >::ref();

  auto equipPkt = makeZonePacket< WorldPackets::Server::FFXIVIpcEquip >( retainerActorId, player.getId() );
  auto& data = equipPkt->data();
  std::memset( &data, 0, sizeof( data ) );

  const auto equipContainer = player.getInventoryContainer( static_cast< uint16_t >( Common::InventoryType::RetainerEquippedGear ) );

  const auto mainItem = equipContainer ? equipContainer->getItem( Common::GearSetSlot::MainHand ) : nullptr;
  const auto offItem = equipContainer ? equipContainer->getItem( Common::GearSetSlot::OffHand ) : nullptr;

  data.MainWeapon = mainItem ? static_cast< uint64_t >( mainItem->getId() ) : 0;
  data.SubWeapon = offItem ? static_cast< uint64_t >( offItem->getId() ) : 0;
  data.CrestEnable = 0;
  data.PatternInvalid = 0;

  // Equipment array (10 slots): Head, Body, Hands, Waist, Legs, Feet, Ear, Neck, Wrist, Ring1
  const Common::GearSetSlot equipSlots[ 10 ] = {
          Common::GearSetSlot::Head,
          Common::GearSetSlot::Body,
          Common::GearSetSlot::Hands,
          Common::GearSetSlot::Waist,
          Common::GearSetSlot::Legs,
          Common::GearSetSlot::Feet,
          Common::GearSetSlot::Ear,
          Common::GearSetSlot::Neck,
          Common::GearSetSlot::Wrist,
          Common::GearSetSlot::Ring1 };

  for( int i = 0; i < 10; ++i )
  {
    const auto item = equipContainer ? equipContainer->getItem( equipSlots[ i ] ) : nullptr;
    data.Equipment[ i ] = item ? item->getId() : 0;
  }

  server.queueForPlayer( player.getCharacterId(), equipPkt );
}

void RetainerMgr::sendRetainerInspect( Entity::Player& player, const RetainerData& retainer )
{
  const uint32_t retainerActorId = getSpawnedRetainerActorId( player, retainer.retainerId );
  if( retainerActorId == 0 )
  {
    Logger::warn( "RetainerMgr::sendRetainerInspect - Retainer {} not spawned for player {}",
                  retainer.retainerId, player.getId() );
    return;
  }

  auto& server = Common::Service< World::WorldServer >::ref();

  auto inspectPkt = makeZonePacket< WorldPackets::Server::FFXIVIpcInspect >( retainerActorId, player.getId() );
  auto& data = inspectPkt->data();
  std::memset( &data, 0, sizeof( data ) );

  data.ObjType = 0;
  data.Sex = 0;
  data.ClassJob = retainer.classJob;
  data.Lv = retainer.level > 0 ? retainer.level : 1;
  data.LvSync = 0;
  data.Title = 0;
  data.GrandCompany = 0;
  data.GrandCompanyRank = 0;
  data.Flag = 0;

  if( retainer.customize.size() == sizeof( data.Customize ) )
    std::memcpy( data.Customize, retainer.customize.data(), sizeof( data.Customize ) );

  std::strncpy( data.Name, retainer.name.c_str(), sizeof( data.Name ) - 1 );
  data.Name[ sizeof( data.Name ) - 1 ] = '\0';

  const auto equipContainer = player.getInventoryContainer( static_cast< uint16_t >( Common::InventoryType::RetainerEquippedGear ) );
  const uint8_t equipMax = equipContainer ? equipContainer->getMaxSize() : 0;

  auto getEquippedItem = [ & ]( Common::GearSetSlot slot ) -> ItemPtr {
    const auto idx = static_cast< uint8_t >( slot );
    if( !equipContainer || idx >= equipMax )
      return nullptr;
    return equipContainer->getItem( idx );
  };

  const auto mainItem = getEquippedItem( Common::GearSetSlot::MainHand );
  const auto offItem = getEquippedItem( Common::GearSetSlot::OffHand );

  data.MainWeaponModelId = mainItem ? mainItem->getModelId1() : 0;
  data.SubWeaponModelId = offItem ? offItem->getModelId1() : 0;

  for( int i = 0; i < 10; ++i )
    data.ModelId[ i ] = 0;

  if( equipContainer )
  {
    const Common::GearModelSlot modelSlots[ 10 ] = {
            Common::GearModelSlot::ModelHead,
            Common::GearModelSlot::ModelBody,
            Common::GearModelSlot::ModelHands,
            Common::GearModelSlot::ModelLegs,
            Common::GearModelSlot::ModelFeet,
            Common::GearModelSlot::ModelEar,
            Common::GearModelSlot::ModelNeck,
            Common::GearModelSlot::ModelWrist,
            Common::GearModelSlot::ModelRing1,
            Common::GearModelSlot::ModelRing2 };

    const Common::GearSetSlot equipSlots[ 10 ] = {
            Common::GearSetSlot::Head,
            Common::GearSetSlot::Body,
            Common::GearSetSlot::Hands,
            Common::GearSetSlot::Legs,
            Common::GearSetSlot::Feet,
            Common::GearSetSlot::Ear,
            Common::GearSetSlot::Neck,
            Common::GearSetSlot::Wrist,
            Common::GearSetSlot::Ring1,
            Common::GearSetSlot::Ring2 };

    for( int i = 0; i < 10; ++i )
    {
      const auto item = getEquippedItem( equipSlots[ i ] );
      if( item )
        data.ModelId[ modelSlots[ i ] ] = item->getModelId1();
    }

    const uint8_t equipLimit = static_cast< uint8_t >( ( std::min )( equipMax,
                                                                     static_cast< uint8_t >( Common::GearSetSlot::SoulCrystal + 1 ) ) );
    for( uint8_t i = 0; i < equipLimit; ++i )
    {
      const auto item = equipContainer->getItem( i );
      if( !item )
        continue;

      auto& entry = data.Equipment[ i ];
      entry.CatalogId = item->getId();
      entry.Pattern = item->getPattern();
      entry.Signature = 0;
      entry.HQ = static_cast< uint8_t >( item->isHq() ? 1 : 0 );
      entry.Stain = static_cast< uint8_t >( item->getStain() );
    }
  }

  std::memset( data.PSNId, 0, sizeof( data.PSNId ) );
  std::memset( data.MasterName, 0, sizeof( data.MasterName ) );
  std::memset( data.SkillLv, 0, sizeof( data.SkillLv ) );
  for( int i = 0; i < 50; ++i )
    data.BaseParam[ i ] = 0;

  server.queueForPlayer( player.getCharacterId(), inspectPkt );
}

void RetainerMgr::sendRetainerEventScene4( Entity::Player& player, uint32_t handlerId )
{
  // Send ResumeEventScene4 (0x01D9) - appears late in retail flow
  // Variant with 4 arguments for extended event handling
  auto& server = Common::Service< World::WorldServer >::ref();

  auto scenePkt = makeZonePacket< WorldPackets::Server::FFXIVIpcResumeEventScene4 >( player.getId() );
  auto& data = scenePkt->data();
  std::memset( &data, 0, sizeof( data ) );

  data.handlerId = handlerId;
  data.sceneId = 0;
  data.resumeId = 0x03;// From retail capture (0x0a000b00 with 0x03 byte)
  data.numOfArgs = 4;
  // args[0..3] default to 0
  for( int i = 0; i < 4; ++i )
    data.args[ i ] = 0;

  server.queueForPlayer( player.getCharacterId(), scenePkt );
}


void RetainerMgr::sendRetainerInventory( Entity::Player& player, uint64_t retainerId )
{
  LogRetainerPacketOffsetsOnce();

  // Track the currently open retainer so inventory operations (e.g., gil transfer)
  // can resolve RetainerGil(12000) updates to a concrete retainerId.
  setActiveRetainer( player, retainerId );

  const auto retainerOpt = getRetainer( retainerId );
  if( !retainerOpt.has_value() )
  {
    Logger::error( "RetainerMgr::sendRetainerInventory - Retainer {} not found in database!", retainerId );
    return;
  }

  const auto& retainer = *retainerOpt;

  // Ensure the player's retainer containers reflect the currently selected retainer.
  {
    std::scoped_lock lock( m_loadedRetainerMutex );
    const auto it = m_loadedRetainerInventoryByPlayer.find( player.getId() );
    const bool needsLoad = it == m_loadedRetainerInventoryByPlayer.end() || it->second != retainerId;
    if( needsLoad )
    {
      ClearRetainerContainers( player );
      LoadRetainerContainersFromDb( player, retainerId );
      m_loadedRetainerInventoryByPlayer[ player.getId() ] = retainerId;
    }
  }

  auto& server = Common::Service< World::WorldServer >::ref();

  // Retail: 0x01EF (IsMarket) precedes 0x010B (MarketRetainerUpdate) and the
  // subsequent inventory container burst.
  const uint32_t isMarket = GetRetainerIsMarketFlag( retainer );
  sendIsMarket( player, 0x6E, isMarket );
  sendMarketRetainerUpdate( player, retainer );

  constexpr uint32_t kRetainerItemSizeUnknown1 = 0;

  auto sendEmptyItemSize = [ & ]( const uint16_t storageId, const uint32_t sequence ) {
    auto sizePacket = makeZonePacket< WorldPackets::Server::FFXIVIpcItemSize >( player.getId() );
    auto& sizeData = sizePacket->data();
    std::memset( &sizeData, 0, sizeof( sizeData ) );
    sizeData.contextId = sequence;
    sizeData.size = 0;
    sizeData.storageId = storageId;
    sizeData.unknown1 = kRetainerItemSizeUnknown1;
    server.queueForPlayer( player.getCharacterId(), sizePacket );
  };

  // Send retainer bag containers (10000-10006) FIRST - matching retail order.
  for( uint16_t i = 0; i < 7; ++i )
  {
    const uint16_t storageId = static_cast< uint16_t >( Common::InventoryType::RetainerBag0 ) + i;
    const uint32_t sequence = player.getNextInventorySequence();

    if( auto container = player.getInventoryContainer( storageId ) )
    {
      SendRetainerContainer( player, container, sequence );
      continue;
    }

    Logger::warn( "RetainerMgr::sendRetainerInventory - Missing retainer bag container storageId={} for player {}",
                  storageId, player.getId() );
    sendEmptyItemSize( storageId, sequence );
  }

  // Send retainer's gil (container ID 12000)
  {
    const uint32_t gilSequence = player.getNextInventorySequence();

    auto gilPacket = makeZonePacket< WorldPackets::Server::FFXIVIpcGilItem >( player.getId() );
    auto& gilData = gilPacket->data();
    std::memset( &gilData, 0, sizeof( gilData ) );
    gilData.contextId = gilSequence;
    gilData.item.catalogId = 1;
    gilData.item.subquarity = 3;
    gilData.item.stack = retainer.gil;
    gilData.item.storageId = static_cast< uint16_t >( Common::InventoryType::RetainerGil );
    gilData.item.containerIndex = 0;
    gilData.padding = 0;
    server.queueForPlayer( player.getCharacterId(), gilPacket );

    auto sizePacket = makeZonePacket< WorldPackets::Server::FFXIVIpcItemSize >( player.getId() );
    auto& sizeData = sizePacket->data();
    std::memset( &sizeData, 0, sizeof( sizeData ) );
    sizeData.contextId = gilSequence;
    sizeData.size = 1;
    sizeData.storageId = static_cast< uint32_t >( Common::InventoryType::RetainerGil );
    sizeData.unknown1 = kRetainerItemSizeUnknown1;
    server.queueForPlayer( player.getCharacterId(), sizePacket );
  }

  // Retainer crystals (12001)
  {
    const uint16_t storageId = static_cast< uint16_t >( Common::InventoryType::RetainerCrystal );
    const uint32_t sequence = player.getNextInventorySequence();
    if( auto container = player.getInventoryContainer( storageId ) )
      SendRetainerContainer( player, container, sequence );
    else
      sendEmptyItemSize( storageId, sequence );
  }

  // Retainer market listings (12002)
  {
    const uint16_t storageId = static_cast< uint16_t >( Common::InventoryType::RetainerMarket );
    const uint32_t marketSequence = player.getNextInventorySequence();

    if( auto container = player.getInventoryContainer( storageId ) )
      SendRetainerContainer( player, container, marketSequence );
    else
      sendEmptyItemSize( storageId, marketSequence );

    sendMarketPriceSnapshot( player, marketSequence );
  }

  // Retainer equipped gear (11000)
  {
    const uint16_t storageId = static_cast< uint16_t >( Common::InventoryType::RetainerEquippedGear );
    const uint32_t sequence = player.getNextInventorySequence();
    if( auto container = player.getInventoryContainer( storageId ) )
      SendRetainerContainer( player, container, sequence );
    else
      sendEmptyItemSize( storageId, sequence );
  }

  // Send 0x01A5 (Equip) - retainer's currently equipped gear
  sendRetainerEquip( player, retainer );
}

void RetainerMgr::setActiveRetainer( Entity::Player& player, uint64_t retainerId )
{
  std::scoped_lock lock( m_activeRetainerMutex );
  m_activeRetainerByPlayer[ player.getId() ] = retainerId;
}

void RetainerMgr::clearActiveRetainer( Entity::Player& player )
{
  // When the retainer UI closes, drop both the active retainer context and any
  // cached "loaded retainer" marker so the next open always reloads from DB.
  {
    std::scoped_lock lock( m_activeRetainerMutex );
    m_activeRetainerByPlayer.erase( player.getId() );
  }

  {
    std::scoped_lock lock( m_loadedRetainerMutex );
    m_loadedRetainerInventoryByPlayer.erase( player.getId() );
  }
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
  auto& server = Common::Service< World::WorldServer >::ref();

  constexpr uint8_t kMaxSlots = 8;

  auto pkt = makeZonePacket< WorldPackets::Server::FFXIVIpcGetRetainerListResult >( player.getId() );
  auto& data = pkt->data();
  std::memset( &data, 0, sizeof( data ) );

  // Populate entries by slot/displayOrder so the client index aligns with the
  // 0x01AB/0x01AA retainer slot table. If we fill sequentially, the client can
  // end up reading class/job/level for the wrong slot.
  unsigned int populatedCount = 0;
  for( const auto& retainer : retainers )
  {
    const uint8_t slot = retainer.displayOrder;
    if( slot >= kMaxSlots )
    {
      // Should never happen; displayOrder is a byte and expected 0-7.
      Logger::warn( "RetainerMgr: Ignoring retainer {} '{}' with out-of-range displayOrder {} for player {}",
                    static_cast< unsigned long long >( retainer.retainerId ),
                    retainer.name,
                    static_cast< unsigned int >( slot ),
                    player.getId() );
      continue;
    }

    auto& entry = data.entries[ slot ];
    entry.retainerId = retainer.retainerId;
    entry.createTime = ResolveRetainerCreateTime( retainer.createTime );
    entry.level = retainer.level;
    entry.classJob = retainer.classJob;
    std::strncpy( entry.name, retainer.name.c_str(), sizeof( entry.name ) - 1 );
    entry.name[ sizeof( entry.name ) - 1 ] = '\0';
    ++populatedCount;
  }

  if( populatedCount == 0 )
    Logger::debug( "RetainerMgr: Sending GetRetainerListResult (0x0106) to player {}: no retainers", player.getId() );
  else
    Logger::debug( "RetainerMgr: Sending GetRetainerListResult (0x0106) to player {}: {} retainers",
                   player.getId(),
                   populatedCount );

  for( size_t i = 0; i < 8; ++i )
  {
    const auto& entry = data.entries[ i ];
    Logger::debug( "RetainerMgr: 0x0106 entry[{}]: retainerId={} name='{}' level={} classJob={} createTime={} ",
                   static_cast< unsigned int >( i ),
                   static_cast< unsigned long long >( entry.retainerId ),
                   entry.name,
                   entry.level,
                   entry.classJob,
                   entry.createTime );
  }

  server.queueForPlayer( player.getCharacterId(), pkt );
}

void RetainerMgr::sendSetRetainerSubscription( Entity::Player& player )
{
  auto& server = Common::Service< World::WorldServer >::ref();

  // Retail 3.35 capture: msg=0x0143 (OrderMySelf) with category=0x0018.
  // Observed constant payload:
  //   padding=0x8E50, param1=0x00093A80 (604800 seconds), param6=0x8E50A690
  auto pkt = makeZonePacket< WorldPackets::Server::FFXIVIpcActorControlSelf >( player.getId() );
  auto& data = pkt->data();
  std::memset( &data, 0, sizeof( data ) );
  data.category = 0x0018;
  data.padding = 0x8E50;
  data.param1 = 0x00093A80;
  data.param6 = 0x8E50A690;
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
  const uint64_t characterId = player.getCharacterId();
  // Load retainer data
  auto retainerOpt = getRetainer( retainerId );
  if( !retainerOpt )
  {
    Logger::error( "RetainerMgr::spawnRetainer - Retainer {} not found", retainerId );
    return 0;
  }

  auto& retainerData = *retainerOpt;

  // Verify ownership
  if( retainerData.ownerId != characterId )
  {
    Logger::error( "RetainerMgr::spawnRetainer - Player {} tried to spawn retainer {} owned by {}",
                   characterId, retainerId, retainerData.ownerId );
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

  // Get actor ID from the territory's actor ID pool (like BNpcs and other spawned entities)
  // This ensures the actor ID matches what's in the spawn packet header,
  // which is critical for ClassJobChange ActorControl to update the correct actor.
  auto& teriMgr = Common::Service< World::Manager::TerritoryMgr >::ref();
  auto pZone = teriMgr.getTerritoryByGuId( player.getTerritoryId() );
  if( !pZone )
  {
    Logger::error( "RetainerMgr::spawnRetainer - Could not get territory for player {}", player.getId() );
    return 0;
  }
  uint32_t retainerActorId = pZone->getNextActorId();

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
    data.ownerId = res->getUInt64( "CharacterId" );
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
