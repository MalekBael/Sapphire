#pragma once

#include <Network/PacketDef/Zone/ServerZoneDef.h>
#include <Network/GamePacket.h>
#include <Util/Util.h>
#include <Common.h>
#include "Actor/Player.h"
#include "Forwards.h"
#include "Manager/RetainerMgr.h"

namespace Sapphire::Network::Packets::WorldPackets::Server
{

  /**
   * @brief Packet to spawn a retainer NPC in the world
   * 
   * Uses FFXIVIpcPlayerSpawn (opcode 0x0190) but with ObjKind=4 (Retainer)
   * Unlike NPCs, retainers have player-customized appearance and names.
   * 
   * Retainer spawns are temporary - they despawn when:
   * - Player dismisses them
   * - Player zones out
   * - Player logs out
   * - Summoning bell times out (2 minutes idle)
   */
  class RetainerSpawnPacket : public ZoneChannelPacket< FFXIVIpcPlayerSpawn >
  {
  public:
    /**
     * @brief Construct retainer spawn packet
     * 
     * @param retainerActorId Unique actor ID for this retainer spawn (from server.getNextActorId())
     * @param retainerData Retainer's persistent data (from database)
     * @param position World position to spawn retainer at
     * @param rotation Rotation in radians (facing direction)
     * @param target Player who is summoning the retainer
     */
    RetainerSpawnPacket( uint32_t retainerActorId,
                         const World::Manager::RetainerData& retainerData,
                         const Common::FFXIVARR_POSITION3& position,
                         float rotation,
                         Entity::Player& target ) :
      ZoneChannelPacket< FFXIVIpcPlayerSpawn >( retainerActorId, target.getId() )
    {
      initialize( retainerActorId, retainerData, position, rotation, target );
    };

  private:
    void initialize( uint32_t retainerActorId,
                     const World::Manager::RetainerData& retainerData,
                     const Common::FFXIVARR_POSITION3& position,
                     float rotation,
                     Entity::Player& target )
    {
      // Basic retainer properties
      m_data.ClassJob = retainerData.classJob;
      m_data.Lv = retainerData.level;
      
      // Retainers have minimal stats (they don't fight)
      m_data.Hp = 1000;
      m_data.Mp = 1000;
      m_data.HpMax = 1000;
      m_data.MpMax = 1000;

      // Copy customize array (26 bytes) - appearance data
      if( retainerData.customize.size() >= 26 )
      {
        memcpy( m_data.Customize, retainerData.customize.data(), 26 );
      }

      // Retainer name (max 32 bytes, null-terminated)
      const size_t nameLen = std::min( retainerData.name.size(), size_t( 31 ) );
      memcpy( m_data.Name, retainerData.name.c_str(), nameLen );
      m_data.Name[ nameLen ] = '\0';

      // Position and rotation
      m_data.Pos[ 0 ] = position.x;
      m_data.Pos[ 1 ] = position.y;
      m_data.Pos[ 2 ] = position.z;
      m_data.Dir = static_cast< uint16_t >( rotation * 32767.0f / 3.14159265f );

      // Object type: Retainer (not a player, not a combat NPC)
      // ObjKind=4 tells client this is a retainer/treasure/aetheryte/other object
      m_data.ObjKind = 4;  // Retainer
      m_data.ObjType = 5;   // Subtype (appears to be 5 for retainers)

      // Spawn index for this player's actor list
      m_data.Index = target.getSpawnIdForActorId( retainerActorId );
      if( !target.isActorSpawnIdValid( m_data.Index ) )
        return;

      // Owner relationship
      m_data.OwnerId = target.getId();  // Player who owns this retainer

      // Display properties
      m_data.Mode = 0;  // Standing idle
      m_data.ActiveType = 0;  // Not in combat stance
      m_data.Flag = 0;  // No special display flags

      // These fields should be zero for retainers
      m_data.MainTarget = 0;
      m_data.ParentId = 0;
      m_data.TriggerId = 0;
      m_data.ChannelingTarget = 0;
      m_data.ContentId = 0;
      m_data.LayoutId = 0;
      m_data.NpcId = 0;  // Retainers don't have BNpc IDs
      m_data.NameId = 0; // Custom name, not from game data

      // Equipment: Retainers can wear gear, but for initial spawn we'll use defaults
      // TODO: Load retainer equipment from inventory/gear set
      memset( m_data.Equipment, 0, sizeof( m_data.Equipment ) );
      m_data.MainWeapon = 0;
      m_data.SubWeapon = 0;

      // No status effects on spawn
      memset( m_data.Status, 0, sizeof( m_data.Status ) );

      // No mount/companion for retainers
      m_data.Companion = 0;
      m_data.Mount.Id = 0;

      // Retainer-specific fields
      // Note: Some of these may need adjustment based on testing
      m_data.PoseEmote = 0;  // Standing
      m_data.ModeArgs = 0;
      m_data.Voice = 0;
      m_data.Title = 0;
      m_data.OnlineStatus = 0;
      m_data.GMRank = 0;
      m_data.GrandCompany = 0;
      m_data.GrandCompanyRank = 0;
    };
  };

}
