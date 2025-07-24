#pragma once

#include <Actor/Player.h>
#include <unordered_map>
#include <unordered_set>

namespace Sapphire::World::Manager
{
  /*!
   * @brief Manager class for handling rental chocobo functionality
   */
  class ChocoboMgr
  {
  private:
    static std::unordered_set< uint32_t > s_pendingRentalMounts;
    static std::unordered_map< uint32_t, uint32_t > s_persistentMounts;// playerId -> mountId

  public:
    /*!
     * @brief Adds a player to the pending rental mounts list
     * @param playerId The character ID of the player
     */
    static void addPendingRentalMount( uint32_t playerId );

    /*!
     * @brief Checks if a player has a pending rental mount
     * @param playerId The character ID of the player
     * @return true if the player has a pending mount, false otherwise
     */
    static bool hasPendingRentalMount( uint32_t playerId );

    /*!
     * @brief Removes a player from the pending rental mounts list
     * @param playerId The character ID of the player
     */
    static void removePendingRentalMount( uint32_t playerId );

    /*!
     * @brief Adds a player to the persistent mounts list for zone changes
     * @param playerId The character ID of the player
     * @param mountId The mount ID to persist
     */
    static void addPersistentMount( uint32_t playerId, uint32_t mountId );

    /*!
     * @brief Checks if a player has a persistent mount
     * @param playerId The character ID of the player
     * @return true if the player has a persistent mount, false otherwise
     */
    static bool hasPersistentMount( uint32_t playerId );

    /*!
     * @brief Gets the persistent mount ID for a player
     * @param playerId The character ID of the player
     * @return The mount ID, or 0 if none
     */
    static uint32_t getPersistentMount( uint32_t playerId );

    /*!
     * @brief Removes a player from the persistent mounts list
     * @param playerId The character ID of the player
     */
    static void removePersistentMount( uint32_t playerId );

    /*!
     * @brief Checks for pending/persistent mount on zone-in (does NOT mount immediately)
     * Called from Territory::onPlayerZoneIn
     * @param player The player to check
     */
    static void checkAndMountPlayer( Entity::Player& player );

    /*!
     * @brief Mounts player after FINISH_LOADING when client is ready
     * Called after client sends FINISH_LOADING command
     * @param player The player to check and mount
     */
    static void checkAndMountPlayerAfterLoading( Entity::Player& player );

    /*!
     * @brief Called when a player is changing zones to preserve mount state
     * @param player The player changing zones
     */
    static void onPlayerZoneChange( Entity::Player& player );

    /*!
     * @brief Called when a player manually dismounts to clear persistence
     * @param player The player who dismounted
     */
    static void onPlayerDismount( Entity::Player& player );

    /*!
     * @brief Performs the actual mounting of a rental chocobo for a player
     * @param player The player to mount
     * @param chocoboId The ID of the chocobo to mount (default: 1 for rental chocobo)
     */
    static void mountRentalChocobo( Entity::Player& player, uint32_t chocoboId = 1 );

    /*!
     * @brief Triggers a warp with rental chocobo for the specified player
     * @param player The player to warp
     * @param levelId The level ID to warp to (from Level.exd)
     */
    static void triggerRentalWarp( Entity::Player& player, uint32_t levelId );

    /*!
     * @brief Cleanup function to remove disconnected players from pending list
     * Called periodically to prevent memory leaks
     */
    static void cleanup();
  };
}// namespace Sapphire::World::Manager