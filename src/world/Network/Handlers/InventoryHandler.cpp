#include <Common.h>
#include <Network/CommonNetwork.h>
#include <Network/GamePacket.h>
#include <Logging/Logger.h>
#include <Network/PacketContainer.h>
#include <Network/PacketDef/Zone/ClientZoneDef.h>

#include "Network/GameConnection.h"
#include "Network/PacketWrappers/ServerNoticePacket.h"

#include "Actor/Player.h"
#include "Manager/RetainerMgr.h"

#include "Session.h"
#include "WorldServer.h"
#include <Service.h>

#include <algorithm>
#include <iomanip>
#include <sstream>

using namespace Sapphire::Common;
using namespace Sapphire::Network::Packets;
using namespace Sapphire::Network::Packets::WorldPackets::Server;
using namespace Sapphire::Network::Packets::WorldPackets::Client;

namespace
{
  std::string HexDump( const uint8_t* data, const size_t size, const size_t maxBytes = 64 )
  {
    if( !data || size == 0 )
      return "";

    const size_t n = ( std::min )( size, maxBytes );
    std::ostringstream oss;
    oss << std::hex << std::setfill( '0' );
    for( size_t i = 0; i < n; ++i )
    {
      if( i != 0 )
        oss << ' ';
      oss << std::setw( 2 ) << static_cast< uint32_t >( data[ i ] );
    }

    if( n < size )
      oss << " ...";

    return oss.str();
  }
}// namespace

void Sapphire::Network::GameConnection::itemOperation( const Packets::FFXIVARR_PACKET_RAW& inPacket,
                                                       Entity::Player& player )
{
  auto& server = Common::Service< World::WorldServer >::ref();
  const auto packet = ZoneChannelPacket< FFXIVIpcClientInventoryItemOperation >( inPacket );

  const auto& req = packet.data();

  const auto operationType = req.OperationType;
  const auto contextId = req.ContextId;

  const auto pad1 = req.__padding1;
  const auto pad2 = req.__padding2;
  const auto pad3 = req.__padding3;

  const auto srcActorId = req.SrcActorId;
  const auto dstActorId = req.DstActorId;

  const auto fromSlot = req.SrcContainerIndex;
  const auto fromContainer = req.SrcStorageId;
  const auto toSlot = req.DstContainerIndex;
  const auto toContainer = req.DstStorageId;

  const auto pad4 = req.__padding4;
  const auto pad5 = req.__padding5;
  const auto pad6 = req.__padding6;
  const auto pad7 = req.__padding7;

  const auto srcStack = req.SrcStack;
  const auto dstStack = req.DstStack;
  const auto srcCatalogId = req.SrcCatalogId;
  const auto dstCatalogId = req.DstCatalogId;

  uint8_t errorType = Common::ITEM_OPERATION_ERROR_TYPE::ITEM_OPERATION_ERROR_TYPE_NONE;

  Logger::debug( "ItemOperation: opType={} ctx={} srcActor={} srcStorage={} srcSlot={} srcStack={} srcItem={} dstActor={} dstStorage={} dstSlot={} dstStack={} dstItem={}",
                 static_cast< uint32_t >( operationType ),
                 contextId,
                 srcActorId,
                 fromContainer,
                 fromSlot,
                 srcStack,
                 srcCatalogId,
                 dstActorId,
                 toContainer,
                 toSlot,
                 dstStack,
                 dstCatalogId );

  // Extra diagnostics for the retainer-gil case: for some 3.35 flows we appear to receive 0 storage/actor ids.
  if( operationType == Common::ITEM_OPERATION_TYPE::ITEM_OPERATION_TYPE_SETRETAINERGIL )
  {
    const auto ipcHdrSize = sizeof( Packets::FFXIVARR_IPC_HEADER );
    const auto payloadOffset = ( std::min )( ipcHdrSize, inPacket.data.size() );
    const auto payloadSize = inPacket.data.size() - payloadOffset;
    const uint8_t* payload = payloadSize > 0 ? ( inPacket.data.data() + payloadOffset ) : nullptr;

    Logger::debug( "ItemOperation SETRETAINERGIL: raw sizes segSize={} dataSize={} payloadSize={} pads={:02X} {:02X} {:02X} extraPads={:02X} {:02X} {:02X} {:02X}",
                   inPacket.segHdr.size,
                   inPacket.data.size(),
                   payloadSize,
                   static_cast< uint32_t >( pad1 ),
                   static_cast< uint32_t >( pad2 ),
                   static_cast< uint32_t >( pad3 ),
                   static_cast< uint32_t >( pad4 ),
                   static_cast< uint32_t >( pad5 ),
                   static_cast< uint32_t >( pad6 ),
                   static_cast< uint32_t >( pad7 ) );

    if( payload )
      Logger::debug( "ItemOperation SETRETAINERGIL: payload[0..] {}", HexDump( payload, payloadSize, 80 ) );
  }

  // TODO: other inventory operations need to be implemented
  switch( operationType )
  {
    case Common::ITEM_OPERATION_TYPE::ITEM_OPERATION_TYPE_DELETEITEM:// discard item action
      player.discardItem( fromContainer, fromSlot );
      break;

    case Common::ITEM_OPERATION_TYPE::ITEM_OPERATION_TYPE_MOVEITEM:// move item action
      player.moveItem( fromContainer, fromSlot, toContainer, toSlot );
      break;

    case Common::ITEM_OPERATION_TYPE::ITEM_OPERATION_TYPE_SWAPITEM:// swap item action
      player.swapItem( fromContainer, fromSlot, toContainer, toSlot );
      break;

    case Common::ITEM_OPERATION_TYPE::ITEM_OPERATION_TYPE_MERGEITEM:// merge stack action
      player.mergeItem( fromContainer, fromSlot, toContainer, toSlot );
      break;

    case Common::ITEM_OPERATION_TYPE::ITEM_OPERATION_TYPE_SPLITITEM:// split stack action
      player.splitItem( fromContainer, fromSlot, toContainer, toSlot, static_cast< uint16_t >( dstStack ) );
      break;

    case Common::ITEM_OPERATION_TYPE::ITEM_OPERATION_TYPE_SETRETAINERGIL:// gil transfer between Currency <-> RetainerGil
    {
      auto& retainerMgr = Common::Service< World::Manager::RetainerMgr >::ref();
      const auto activeRetainerId = retainerMgr.getActiveRetainerId( player );

      if( !activeRetainerId.has_value() )
      {
        Logger::warn( "ItemOperation SETRETAINERGIL: no active retainer context for player {}", player.getId() );
        errorType = Common::ITEM_OPERATION_ERROR_TYPE::ITEM_OPERATION_ERROR_TYPE_INVALIDGIL;
        break;
      }

      const auto retainerOpt = retainerMgr.getRetainer( *activeRetainerId );
      if( !retainerOpt.has_value() )
      {
        Logger::warn( "ItemOperation SETRETAINERGIL: active retainer {} not found", *activeRetainerId );
        errorType = Common::ITEM_OPERATION_ERROR_TYPE::ITEM_OPERATION_ERROR_TYPE_LOADINGDATA;
        break;
      }

      constexpr uint32_t kMaxGil = 999999999;

      const uint32_t currentPlayerGil = player.getCurrency( Common::CurrencyType::Gil );
      const uint32_t currentRetainerGil = retainerOpt->gil;

      // Sanity: gil item id is 1.
      if( ( srcCatalogId != 0 && srcCatalogId != 1 ) || ( dstCatalogId != 0 && dstCatalogId != 1 ) )
      {
        Logger::warn( "ItemOperation SETRETAINERGIL: unexpected catalog ids src={} dst={} (expected 1)", srcCatalogId, dstCatalogId );
      }

      uint32_t newPlayerGil = currentPlayerGil;
      uint32_t newRetainerGil = currentRetainerGil;

      // NOTE: On 3.35 we observe multiple on-wire shapes for SETRETAINERGIL:
      // - "Normal" shape: storage IDs present (Currency <-> RetainerGil) and stacks can be interpreted as
      //   either absolute post-operation balances or transfer amount.
      // - "Missing storage" shape: Src/DstStorageId and sometimes Src/DstActorId are 0, but stacks may
      //   contain *absolute post-operation balances* (often only the source stack is populated).
      // We must NOT treat absolute balances as transfer amount, or the entered amount will appear "scaled".

      uint32_t transferAmount = 0;
      bool usedDesiredRetainerGil = false;

      bool currencyToRetainer = ( fromContainer == Common::InventoryType::Currency ) &&
                                ( toContainer == Common::InventoryType::RetainerGil );
      bool retainerToCurrency = ( fromContainer == Common::InventoryType::RetainerGil ) &&
                                ( toContainer == Common::InventoryType::Currency );

      // Some clients/flows appear to send SETRETAINERGIL with Src/DstStorageId unset (0).
      // In that case, infer direction from Src/DstActorId and treat stack fields as transfer amount.
      const bool storagesMissing = ( fromContainer == 0 && toContainer == 0 );
      const bool actorsMissing = ( srcActorId == 0 && dstActorId == 0 );

      // Common 3.35 variant (observed in logs): storages are 0 and SrcStack == DstStack.
      // In this case the stack value represents the *retainer's post-operation gil balance*,
      // not the entered transfer amount.
      //
      // Examples:
      // - Retainer 50, player 1572, entrust 500 -> client sends stacks=550 (new retainer gil)
      // - Retainer 600, withdraw 600 -> client sends stacks=0 (new retainer gil)
      if( storagesMissing && srcStack == dstStack )
      {
        const uint32_t desiredRetainerGil = srcStack;

        if( desiredRetainerGil != currentRetainerGil )
        {
          if( desiredRetainerGil > currentRetainerGil )
          {
            const uint32_t delta = desiredRetainerGil - currentRetainerGil;
            if( delta <= currentPlayerGil && desiredRetainerGil <= kMaxGil )
            {
              currencyToRetainer = true;
              retainerToCurrency = false;

              newRetainerGil = desiredRetainerGil;
              newPlayerGil = currentPlayerGil - delta;
              transferAmount = delta;
              usedDesiredRetainerGil = true;
            }
          }
          else
          {
            const uint32_t delta = currentRetainerGil - desiredRetainerGil;
            if( delta <= currentRetainerGil && desiredRetainerGil <= kMaxGil && ( currentPlayerGil + delta ) <= kMaxGil )
            {
              currencyToRetainer = false;
              retainerToCurrency = true;

              newRetainerGil = desiredRetainerGil;
              newPlayerGil = currentPlayerGil + delta;
              transferAmount = delta;
              usedDesiredRetainerGil = true;
            }
          }

          if( transferAmount != 0 )
          {
            Logger::debug( "SETRETAINERGIL decode: inferred from desiredRetainerGil={} delta={} dir={} (playerGil={} retainerGil={} ctx={} extraPad4={:02X})",
                           desiredRetainerGil,
                           transferAmount,
                           currencyToRetainer ? "Currency->Retainer" : "Retainer->Currency",
                           currentPlayerGil,
                           currentRetainerGil,
                           contextId,
                           static_cast< uint32_t >( pad4 ) );
          }
        }
      }

      if( storagesMissing && !currencyToRetainer && !retainerToCurrency )
      {
        const uint32_t spawnedRetainerActorId = retainerMgr.getSpawnedRetainerActorId( player, *activeRetainerId );

        const bool srcIsPlayer = ( srcActorId == 0 ) || ( srcActorId == player.getId() );
        const bool dstIsPlayer = ( dstActorId == 0 ) || ( dstActorId == player.getId() );
        const bool srcIsRetainer = ( spawnedRetainerActorId != 0 ) && ( srcActorId == spawnedRetainerActorId );
        const bool dstIsRetainer = ( spawnedRetainerActorId != 0 ) && ( dstActorId == spawnedRetainerActorId );

        currencyToRetainer = srcIsPlayer && dstIsRetainer;
        retainerToCurrency = srcIsRetainer && dstIsPlayer;

        Logger::debug( "ItemOperation SETRETAINERGIL: storages missing; inferred direction currencyToRetainer={} retainerToCurrency={} (srcActor={} dstActor={} retainerActor={})",
                       currencyToRetainer ? 1 : 0,
                       retainerToCurrency ? 1 : 0,
                       srcActorId,
                       dstActorId,
                       spawnedRetainerActorId );
      }

      // If we didn't decode the common desiredRetainerGil variant, fall back to interpreting stacks
      // as a transfer amount.
      if( !usedDesiredRetainerGil && transferAmount == 0 )
        transferAmount = dstStack != 0 ? dstStack : srcStack;

      if( storagesMissing && !usedDesiredRetainerGil && !currencyToRetainer && !retainerToCurrency )
      {
        Logger::warn( "ItemOperation SETRETAINERGIL: storages missing and direction could not be inferred (srcActor={} dstActor={} srcStack={} dstStack={})",
                      srcActorId,
                      dstActorId,
                      srcStack,
                      dstStack );
        errorType = Common::ITEM_OPERATION_ERROR_TYPE::ITEM_OPERATION_ERROR_TYPE_STORAGENOTFOUND;
        break;
      }

      if( transferAmount == 0 )
        break;

      if( currencyToRetainer )
      {
        if( transferAmount > currentPlayerGil )
        {
          errorType = Common::ITEM_OPERATION_ERROR_TYPE::ITEM_OPERATION_ERROR_TYPE_INVALIDGIL;
          break;
        }
        newPlayerGil = currentPlayerGil - transferAmount;
        newRetainerGil = currentRetainerGil + transferAmount;
      }
      else if( retainerToCurrency )
      {
        if( transferAmount > currentRetainerGil )
        {
          errorType = Common::ITEM_OPERATION_ERROR_TYPE::ITEM_OPERATION_ERROR_TYPE_INVALIDGIL;
          break;
        }
        newRetainerGil = currentRetainerGil - transferAmount;
        newPlayerGil = currentPlayerGil + transferAmount;
      }
      else
      {
        Logger::warn( "ItemOperation SETRETAINERGIL: unexpected routing srcStorage={} dstStorage={} srcActor={} dstActor={} (expected Currency<->RetainerGil)",
                      fromContainer,
                      toContainer,
                      srcActorId,
                      dstActorId );
        errorType = Common::ITEM_OPERATION_ERROR_TYPE::ITEM_OPERATION_ERROR_TYPE_STORAGENOTFOUND;
        break;
      }

      if( newPlayerGil > kMaxGil || newRetainerGil > kMaxGil )
      {
        errorType = Common::ITEM_OPERATION_ERROR_TYPE::ITEM_OPERATION_ERROR_TYPE_GILFULL;
        break;
      }

      // Apply player gil delta via currency helpers (persists + notifies client).
      if( newPlayerGil > currentPlayerGil )
        player.addCurrency( Common::CurrencyType::Gil, newPlayerGil - currentPlayerGil );
      else if( newPlayerGil < currentPlayerGil )
        player.removeCurrency( Common::CurrencyType::Gil, currentPlayerGil - newPlayerGil );

      // Persist retainer gil and refresh retainer inventory so the modal updates immediately.
      if( !retainerMgr.updateRetainerGil( *activeRetainerId, newRetainerGil ) )
      {
        errorType = Common::ITEM_OPERATION_ERROR_TYPE::ITEM_OPERATION_ERROR_TYPE_LOADINGDATA;
        break;
      }

      Logger::debug( "SETRETAINERGIL decode: dir={} usedDesiredRetainerGil={} transferAmount={} (srcStack={} dstStack={} storagesMissing={} actorsMissing={})",
                     currencyToRetainer ? "Currency->Retainer" : "Retainer->Currency",
                     usedDesiredRetainerGil ? 1 : 0,
                     transferAmount,
                     srcStack,
                     dstStack,
                     storagesMissing ? 1 : 0,
                     actorsMissing ? 1 : 0 );

      Logger::info( "SETRETAINERGIL: player {} retainer {} playerGil {}->{} retainerGil {}->{}",
                    player.getId(), *activeRetainerId, currentPlayerGil, newPlayerGil, currentRetainerGil, newRetainerGil );

      retainerMgr.sendRetainerInventory( player, *activeRetainerId );

      break;
    }

    default:
      break;
  }

  auto ackPacket = makeZonePacket< FFXIVIpcItemOperationBatch >( player.getId() );
  ackPacket->data().contextId = contextId;
  ackPacket->data().operationId = contextId;
  ackPacket->data().operationType = operationType;
  ackPacket->data().errorType = errorType;
  ackPacket->data().packetNum = 0;
  server.queueForPlayer( player.getCharacterId(), ackPacket );
}
