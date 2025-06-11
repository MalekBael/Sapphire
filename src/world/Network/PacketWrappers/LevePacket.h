#ifndef _LEVEPACKET_H
#define _LEVEPACKET_H

#include <Network/GamePacket.h>
#include <Network/PacketDef/ServerIpcs.h>

namespace Sapphire::Network::Packets::Server
{
  /**
  * @brief The packet sent to inspect guildleves
  */
  struct FFXIVIpcInspectGuildleves : FFXIVIpcBasePacket< WorldPackets::Server::ServerZoneIpcType::InspectGuildleves > {
    uint32_t allowances;
    uint32_t leveIds[ 16 ];
    uint8_t leveTypes[ 16 ];
    uint8_t levelReqs[ 16 ];
    uint8_t isLocalLeve[ 16 ];
    uint8_t padding[ 48 ];// Add padding to ensure packet is properly aligned
  };

  /**
  * @brief The packet sent to set guildleve info and allowances
  */
  struct FFXIVIpcGuildleves : FFXIVIpcBasePacket< WorldPackets::Server::ServerZoneIpcType::Guildleves > {
    uint32_t allowances;
    uint8_t leveAllowances;
    uint8_t __padding1;
    uint8_t __padding2;
    uint8_t __padding3;
    uint8_t padding[ 16 ];// Add padding to ensure packet is properly aligned
  };
}// namespace Sapphire::Network::Packets::Server

#endif
