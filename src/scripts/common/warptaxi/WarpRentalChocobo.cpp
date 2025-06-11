#include "Manager/TerritoryMgr.h"
#include "ScriptLogger.h"
#include "Territory/InstanceObjectCache.h"
#include "Territory/Territory.h"
#include <Actor/Player.h>
#include <Network/Util/PacketUtil.h>
#include <ScriptObject.h>
#include "Network/PacketWrappers/ActorControlSelfPacket.h"
#include "Network/CommonActorControl.h"

using namespace Sapphire;
using namespace Sapphire::ScriptAPI;

void logPlayerMountState(const Entity::Player& player, const char* context)
{
    ScriptLogger::debug("[{0}] {1}: mount={2}, status={3}, directorId={4}",
        player.getId(), context, player.getCurrentMount(), static_cast<int>(player.getStatus()), player.getDirectorId());
}

class WarpRentalChocobo : public EventScript
{
public:
  WarpRentalChocobo() : EventScript( 131073 )//1179650
  {
    ScriptLogger::debug( "WarpRentalChocobo script initialized" );
  }

  void onTalk( uint32_t eventId, Entity::Player& player, uint64_t actorId ) override
  {
    ScriptLogger::debug( "WarpRentalChocobo::onTalk triggered for player {0}, eventId: {1}", player.getId(), eventId );

    logPlayerMountState(player, "Before rental logic");

    uint32_t originalEventId = eventId;

    eventMgr().playScene( player, eventId, 0, HIDE_HOTBAR,
                          [ this, originalEventId, &player ]( Entity::Player& player, const Event::SceneResult& result ) {
                            ScriptLogger::debug( "Scene callback received: numResults={0}, using originalEventId={1}",
                                                 result.numOfResults, originalEventId );

                            if( result.numOfResults > 0 )
                              ScriptLogger::debug( "Result[0]={0}", result.results[ 0 ] );

                            if( result.numOfResults > 0 && result.results[ 0 ] == 1 )
                            {
                              auto& exdData = Common::Service< Sapphire::Data::ExdData >::ref();
                              auto warp = exdData.getRow< Excel::Warp >( originalEventId );
                              if( !warp )
                              {
                                ScriptLogger::error( "Failed to get warp data for eventId: {0}", originalEventId );
                                return;
                              }

                              ScriptLogger::debug( "Charging player {0} gil", 80 );
                              player.removeCurrency( Common::CurrencyType::Gil, 80 );

                              auto popRangeInfo = instanceObjectCache().getPopRangeInfo( warp->data().PopRange );
                              if( !popRangeInfo )
                              {
                                ScriptLogger::error( "Failed to get popRangeInfo" );
                                return;
                              }

                              auto pTeri = teriMgr().getTerritoryByTypeId( popRangeInfo->m_territoryTypeId );
                              if( !pTeri )
                              {
                                ScriptLogger::error( "Failed to get territory, id: {0}", popRangeInfo->m_territoryTypeId );
                                return;
                              }

                              logPlayerMountState(player, "Before setMount");

                              player.setPersistentMount(1);
                              player.setMount(1); // Company Chocobo
                              player.setStatus(static_cast<Common::ActorStatus>(Common::ActorStatus::Mounted));

                              logPlayerMountState(player, "After setMount");

                              ScriptLogger::debug( "Setting directorId flag to 1001 for post-warp mounting" );
                              player.setDirectorId( 1001 );

                              ScriptLogger::debug( "Requesting warp to territory {0} at position ({1}, {2}, {3})",
                                                   popRangeInfo->m_territoryTypeId,
                                                   popRangeInfo->m_pos.x,
                                                   popRangeInfo->m_pos.y,
                                                   popRangeInfo->m_pos.z );

                              warpMgr().requestMoveTerritory(
                                      player,
                                      Common::WarpType::WARP_TYPE_RENTAL_CHOCOBO,
                                      pTeri->getGuId(),
                                      popRangeInfo->m_pos,
                                      popRangeInfo->m_rotation );

                              ScriptLogger::debug( "Warp requested, waiting for completion" );
                            }
                            else
                            {
                              ScriptLogger::debug( "Player declined chocobo rental or dialog was cancelled" );
                            }
                          } );
  }

  void onEnterTerritory(Entity::Player& player, uint32_t eventId, uint16_t param1, uint16_t param2) override
  {
    // Just log the entry, don't try to mount here
    logPlayerMountState(player, "onEnterTerritory");
  }
};

EXPOSE_SCRIPT( WarpRentalChocobo );