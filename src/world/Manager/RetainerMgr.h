#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <unordered_map>
#include <mutex>
#include "ForwardsZone.h"

namespace Sapphire::World::Manager
{

  /**
   * @brief Personality types for retainers
   * 
   * These affect dialogue and behavior in cutscenes
   */
  enum class RetainerPersonality : uint8_t
  {
    Cheerful = 1,
    Wild = 2,
    Coolheaded = 3,
    Carefree = 4,
    Problematic = 5,
    Playful = 6
  };

  /**
   * @brief Error codes for retainer operations
   */
  enum class RetainerError : uint32_t
  {
    None = 0,
    NameTaken = 0x15A,
    InvalidName = 0x15B,
    MaxRetainers = 0x15C,
    NotFound = 0x15D,
    DatabaseError = 0x15E,
    InvalidSlot = 0x15F,
    RetainerBusy = 0x160// On venture, has market listings, etc.
  };

  /**
   * @brief Data structure representing a retainer
   */
  struct RetainerData {
    uint64_t retainerId{ 0 };
    uint32_t ownerId{ 0 };
    uint8_t displayOrder{ 0 };
    std::string name;
    RetainerPersonality personality{ RetainerPersonality::Cheerful };
    uint8_t level{ 1 };
    uint8_t classJob{ 0 };
    uint32_t exp{ 0 };
    uint8_t cityId{ 0 };
    uint32_t gil{ 0 };
    uint8_t itemCount{ 0 };
    uint8_t sellingCount{ 0 };
    uint32_t ventureId{ 0 };
    bool ventureComplete{ false };
    uint32_t ventureStartTime{ 0 };
    uint32_t ventureCompleteTime{ 0 };
    std::vector< uint8_t > customize;// 26 bytes
    bool isActive{ true };
    bool isRename{ false };
    uint8_t status{ 0 };
    uint32_t createTime{ 0 };
  };

  /**
   * @brief Manager class for retainer-related operations
   * 
   * Handles:
   * - Creating/deleting retainers
   * - Loading/saving retainer data from database
   * - Venture management
   * - Market board integration
   * - Sending retainer packets to clients
   * 
   * @note Register this manager with ServiceManager during world server initialization
   */
  class RetainerMgr
  {
  public:
    RetainerMgr() = default;
    ~RetainerMgr() = default;

    // ========== Initialization ==========

    /**
     * @brief Initialize the retainer manager
     * 
     * Called during world server startup
     */
    bool init();

    // ========== Retainer CRUD Operations ==========

    /**
     * @brief Create a new retainer for a player
     * 
     * @param player The player creating the retainer
     * @param name The retainer's name (1-20 characters)
     * @param personality Personality type (1-6)
     * @param customize Character appearance data (26 bytes)
     * @return RetainerId on success, or empty optional on failure
     */
    std::optional< uint64_t > createRetainer( Entity::Player& player,
                                              const std::string& name,
                                              RetainerPersonality personality,
                                              const std::vector< uint8_t >& customize );

    /**
     * @brief Delete a retainer
     * 
     * @param player The retainer's owner
     * @param retainerId The retainer to delete
     * @return Error code (None on success)
     */
    RetainerError deleteRetainer( Entity::Player& player, uint64_t retainerId );

    /**
     * @brief Get all retainers for a player
     * 
     * @param player The player
     * @return Vector of retainer data
     */
    std::vector< RetainerData > getRetainers( Entity::Player& player );

    /**
     * @brief Get a specific retainer by ID
     * 
     * @param retainerId The retainer ID
     * @return Retainer data if found
     */
    std::optional< RetainerData > getRetainer( uint64_t retainerId );

    /**
     * @brief Get a player's retainer count
     * 
     * @param player The player
     * @return Number of hired retainers
     */
    uint8_t getRetainerCount( Entity::Player& player );

    /**
     * @brief Get the maximum retainer slots for a player
     * 
     * Default is 2, can be expanded up to 10 with additional retainer purchases
     * 
     * @param player The player
     * @return Maximum retainer slots
     */
    uint8_t getMaxRetainerSlots( Entity::Player& player );

    // ========== Name Validation ==========

    /**
     * @brief Check if a retainer name is available
     * 
     * @param name The name to check
     * @return true if name is available
     */
    bool isNameAvailable( const std::string& name );

    /**
     * @brief Validate a retainer name
     * 
     * Checks length, characters, profanity filter, etc.
     * 
     * @param name The name to validate
     * @return Error code (None if valid)
     */
    RetainerError validateName( const std::string& name );

    /**
     * @brief Rename a retainer
     * 
     * Requires rename token or fantasia equivalent
     * 
     * @param player The retainer's owner
     * @param retainerId The retainer to rename
     * @param newName The new name
     * @return Error code (None on success)
     */
    RetainerError renameRetainer( Entity::Player& player, uint64_t retainerId, const std::string& newName );

    // ========== Class/Job Management ==========

    /**
     * @brief Set a retainer's class/job
     * 
     * @param player The retainer's owner
     * @param retainerId The retainer
     * @param classJob ClassJob ID from the ClassJob Excel sheet
     * @return Error code (None on success)
     */
    RetainerError setRetainerClass( Entity::Player& player, uint64_t retainerId, uint8_t classJob );

    /**
     * @brief Add experience to a retainer
     * 
     * @param retainerId The retainer
     * @param exp Experience points to add
     */
    void addRetainerExp( uint64_t retainerId, uint32_t exp );

    // ========== Venture Management ==========

    /**
     * @brief Start a venture for a retainer
     * 
     * @param player The retainer's owner
     * @param retainerId The retainer
     * @param ventureId The venture ID from RetainerTask Excel
     * @return Error code (None on success)
     */
    RetainerError startVenture( Entity::Player& player, uint64_t retainerId, uint32_t ventureId );

    /**
     * @brief Complete a venture and collect rewards
     * 
     * @param player The retainer's owner
     * @param retainerId The retainer
     * @return Error code (None on success)
     */
    RetainerError completeVenture( Entity::Player& player, uint64_t retainerId );

    /**
     * @brief Cancel an ongoing venture
     * 
     * @param player The retainer's owner
     * @param retainerId The retainer
     * @return Error code (None on success)
     */
    RetainerError cancelVenture( Entity::Player& player, uint64_t retainerId );

    // ========== Market Board ==========

    /**
     * @brief Register a retainer to sell at a city's market
     * 
     * @param player The retainer's owner
     * @param retainerId The retainer
     * @param cityId City ID (1=Limsa, 2=Gridania, 3=Ul'dah, 4=Ishgard)
     * @return 0 on success, 463 if already registered to this city, other error codes for failures
     */
    uint32_t registerToMarket( Entity::Player& player, uint64_t retainerId, uint8_t cityId );

    /**
     * @brief List an item on the market board
     * 
     * @param player The retainer's owner
     * @param retainerId The retainer
     * @param itemId Item instance ID from retainer inventory
     * @param price Gil price per unit
     * @param stack Number of items (for stackables)
     * @return Error code (None on success)
     */
    RetainerError listItem( Entity::Player& player, uint64_t retainerId,
                            uint32_t itemId, uint32_t price, uint32_t stack = 1 );

    /**
     * @brief Remove an item from the market board
     * 
     * @param player The retainer's owner
     * @param retainerId The retainer
     * @param listingId The market listing ID
     * @return Error code (None on success)
     */
    RetainerError unlistItem( Entity::Player& player, uint64_t retainerId, uint64_t listingId );

    /**
     * @brief Adjust the price of a listed item
     * 
     * @param player The retainer's owner
     * @param retainerId The retainer
     * @param listingId The market listing ID
     * @param newPrice The new price
     * @return Error code (None on success)
     */
    RetainerError adjustPrice( Entity::Player& player, uint64_t retainerId,
                               uint64_t listingId, uint32_t newPrice );

    // ========== Packet Sending ==========

    /**
     * @brief Send the RetainerList packet to the client
     * 
     * Opcode: 0x01AA (RetainerList) - 8 bytes
     * Opcode: 0x01AB (RetainerInfo) - 72 bytes x 8 slots
     * Should be sent when player interacts with Summoning Bell
     * 
     * @param player The player to send to
     * @param handlerId Event handler ID for context (default 0 = auto-generate)
     */
    void sendRetainerList( Entity::Player& player, uint32_t handlerId = 0 );

    /**
     * @brief Send detailed RetainerInfo packet for a specific retainer
     * 
     * Opcode: 0x01AB (RetainerInfo) - 72 bytes
     * 
     * @param player The player to send to
     * @param retainerId The retainer
     * @param handlerId Event handler ID for context
     * @param slotIndex Slot index (0-7)
     */
    void sendRetainerInfo( Entity::Player& player, uint64_t retainerId,
                           uint32_t handlerId = 0, uint8_t slotIndex = 0 );

    /**
     * @brief Send retainer inventory data to the client
     * 
     * @param player The player to send to
     * @param retainerId The retainer
     */
    void sendRetainerInventory( Entity::Player& player, uint64_t retainerId );

    /**
     * @brief Send IsMarket (0x01EF) to the client
     *
     * Retail commonly emits this around retainer UI transitions.
     */
    void sendIsMarket( Entity::Player& player, uint32_t marketContext = 0x6E, uint32_t isMarket = 1 );

    /**
     * @brief Send GetRetainerListResult (0x0106) to the client
     *
     * Retail sends this in response to client GetRetainerList (0x0106) when
     * opening retainer inventory/gil related UIs.
     */
    void sendGetRetainerListResult( Entity::Player& player );

    /**
     * @brief Send market listings for a retainer
     * 
     * @param player The player to send to
     * @param retainerId The retainer
     */
    void sendMarketListings( Entity::Player& player, uint64_t retainerId );

    /**
     * @brief Send sales history for a retainer
     * 
     * @param player The player to send to
     * @param retainerId The retainer
     */
    void sendSalesHistory( Entity::Player& player, uint64_t retainerId );

    // ========== NPC Spawning ==========

    /**
     * @brief Spawn a retainer NPC in the world
     * 
     * Creates a temporary retainer actor visible to the summoning player.
     * The retainer will be positioned near the summoning bell (if provided)
     * or near the player as fallback.
     * 
     * @param player The player summoning the retainer
     * @param retainerId The retainer to spawn
     * @param bellPos Optional position of the summoning bell (retainer spawns here)
     * @return Actor ID of spawned retainer, or 0 on failure
     */
    uint32_t spawnRetainer( Entity::Player& player, uint64_t retainerId,
                            const Common::FFXIVARR_POSITION3* bellPos = nullptr );

    /**
     * @brief Despawn a retainer NPC
     * 
     * Removes the retainer actor from the world and sends despawn packet.
     * Called when player dismisses retainer, logs out, or zones.
     * 
     * @param player The player who summoned the retainer
     * @param retainerActorId The actor ID of the spawned retainer
     */
    void despawnRetainer( Entity::Player& player, uint32_t retainerActorId );

    /**
     * @brief Get the spawned actor ID for a retainer
     * 
     * @param player The player who summoned it
     * @param retainerId The retainer database ID
     * @return Actor ID if spawned, or 0 if not currently spawned
     */
    uint32_t getSpawnedRetainerActorId( Entity::Player& player, uint64_t retainerId );

    void sendMarketRetainerUpdate( Entity::Player& player, const RetainerData& retainer );

    // ========== Active Retainer Context ==========

    /**
     * @brief Track which retainer is currently active/open for a given player.
     *
     * This is used by inventory operation handlers (e.g., gil transfer) to know
     * which retainer DB row to update when the client operates on RetainerGil (12000).
     */
    void setActiveRetainer( Entity::Player& player, uint64_t retainerId );
    void clearActiveRetainer( Entity::Player& player );
    std::optional< uint64_t > getActiveRetainerId( const Entity::Player& player ) const;

    /**
     * @brief Persist retainer gil to the database.
     */
    bool updateRetainerGil( uint64_t retainerId, uint32_t gil );

  private:
    void sendMarketPriceSnapshot( Entity::Player& player, uint32_t requestId );

    /**
     * @brief Load retainer data from database
     * 
     * @param retainerId The retainer to load
     * @return Retainer data if found
     */
    std::optional< RetainerData > loadRetainerFromDb( uint64_t retainerId );

    /**
     * @brief Save retainer data to database
     * 
     * @param data The retainer data to save
     * @return true on success
     */
    bool saveRetainerToDb( const RetainerData& data );

    /**
     * @brief Get the next available display order slot for a player
     * 
     * @param player The player
     * @return Next available slot (0-9), or 255 if full
     */
    uint8_t getNextAvailableSlot( Entity::Player& player );

    // Tracking of spawned retainers per player
    // Map: PlayerId -> Map: RetainerId -> ActorId
    std::map< uint32_t, std::map< uint64_t, uint32_t > > m_spawnedRetainers;

    mutable std::mutex m_activeRetainerMutex;
    std::unordered_map< uint32_t, uint64_t > m_activeRetainerByPlayer;
  };

}// namespace Sapphire::World::Manager
