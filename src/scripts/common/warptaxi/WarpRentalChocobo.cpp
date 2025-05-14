//doesnt work currently
#include <Actor/Player.h>
#include <ScriptObject.h>

#include <datReader/DatCategories/bg/LgbTypes.h>
#include <datReader/DatCategories/bg/lgb.h>

#include "Territory/InstanceObjectCache.h"
#include "Territory/Territory.h"

#include <Exd/ExdData.h>
#include <Manager/PlayerMgr.h>
#include <Service.h>

using namespace Sapphire;

class WarpRentalChocobo : public Sapphire::ScriptAPI::EventScript
{
public:
  WarpRentalChocobo() : Sapphire::ScriptAPI::EventScript( 131073 )
  {
  }

  void onTalk( uint32_t eventId, Entity::Player& player, uint64_t actorId ) override
  {
    auto& exdData = Common::Service< Sapphire::Data::ExdData >::ref();

    auto warp = exdData.getRow< Excel::Warp >( eventId );
    if( !warp )
      return;

    // Check if player has enough Gil for the chocobo rental
    if( !checkGil( player, 80 ) )// 80 gil for rental
    {
      eventMgr().playScene( player, eventId, 2, HIDE_HOTBAR, {}, nullptr );
      return;
    }

    // Check if player already has a mount active
    if( player.getStatus() == Common::ActorStatus::Mounted )
    {
      eventMgr().playScene( player, eventId, 3, HIDE_HOTBAR, {}, nullptr );
      return;
    }

    auto warpChoice = [ this ]( Entity::Player& player, const Event::SceneResult& result ) {
      // If player confirmed the rental
      if( result.numOfResults > 0 && result.results[ 0 ] == 1 )
      {
        // Charge gil
        player.removeCurrency( Common::CurrencyType::Gil, 80 );

        auto warp = this->exdData().getRow< Excel::Warp >( result.eventId );
        if( warp )
        {
          // Get the destination info (outside town)
          auto popRangeInfo = instanceObjectCache().getPopRangeInfo( warp->data().PopRange );
          if( popRangeInfo )
          {
            auto pTeri = teriMgr().getTerritoryByTypeId( popRangeInfo->m_territoryTypeId );
            if( !pTeri )
              return;

            // Use directorId to store mount info temporarily
            // Store 1001 for mount ID 1 (we add 1000 to distinguish from other director IDs)
            player.setDirectorId( 1001 );

            // Request warp to outside town using the RENTAL_CHOCOBO warp type
            // Cast the 0x9 value to the WarpType enum to match the function signature
            warpMgr().requestMoveTerritory( player, static_cast< Common::WarpType >( 0x9 ),
                                            pTeri->getGuId(), popRangeInfo->m_pos, popRangeInfo->m_rotation );
          }
        }
      }
    };

    eventMgr().playScene( player, eventId, 0, HIDE_HOTBAR, { 1 }, warpChoice );
  }

  // Handle territory entry event
  void onEnterTerritory( Entity::Player& player, uint32_t eventId, uint16_t param1, uint16_t param2 ) override
  {
    // Check if this was a chocobo rental warp by checking the warp type
    // WARP_TYPE_RENTAL_CHOCOBO = 0x9
    if( param1 == static_cast< uint16_t >( Common::WarpType::WARP_TYPE_RENTAL_CHOCOBO ) && player.getDirectorId() >= 1000 )
    {
      // Extract mount ID from director ID (subtract 1000)
      uint16_t mountId = player.getDirectorId() - 1000;

      // Clear the director ID so it can be used for other purposes
      player.setDirectorId( 0 );

      if( mountId > 0 )
      {
        // Mount the player using the mount packet system
        player.setMount( mountId );
      }
    }
  }

private:
  bool checkGil( Entity::Player& player, uint32_t amount )
  {
    return player.getCurrency( Common::CurrencyType::Gil ) >= amount;
  }
};

EXPOSE_SCRIPT( WarpRentalChocobo );
