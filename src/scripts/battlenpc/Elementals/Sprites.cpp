#include <AI/GambitPack.h>
#include <AI/GambitTargetCondition.h>
#include <Action/Action.h>
#include <Actor/BNpc.h>
#include <Script/NativeScriptApi.h>
#include <ScriptObject.h>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using namespace Sapphire::World;
using namespace Sapphire::ScriptAPI;
using namespace Sapphire::World::AI;
using json = nlohmann::json;

// Base class with common implementation for all sprite types
class SpriteBase : public BattleNpcScript
{
public:
  // Constructor registering for a specific baseId
  SpriteBase( uint32_t baseId ) : BattleNpcScript( baseId ) {}

  void onInit( Sapphire::Entity::BNpc& bnpc ) override
  {
    loadAndApplyGambitPack( bnpc );
  }

protected:
  std::shared_ptr< GambitPack > m_gambitPack;

  void loadAndApplyGambitPack( Sapphire::Entity::BNpc& bnpc )
  {
    try
    {
      // Get BNpc baseId to determine which sprite type we're dealing with
      uint32_t baseId = bnpc.getBNpcBaseId();

      // Load JSON configuration
      const std::string jsonPath = "data/battlenpc/elementals/sprites.json";
      std::ifstream file( jsonPath );

      // Try alternate paths if needed
      if( !file.is_open() )
      {
        std::vector< std::string > alternativePaths = {
                "../data/battlenpc/elementals/sprites.json",
                "../../data/battlenpc/elementals/sprites.json",
                "./data/battlenpc/elementals/sprites.json" };

        for( const auto& path : alternativePaths )
        {
          file.open( path );
          if( file.is_open() )
          {
            std::cerr << "Found sprites.json at: " << path << std::endl;
            break;
          }
        }

        if( !file.is_open() )
        {
          std::cerr << "Failed to open sprites.json from any path" << std::endl;
          return;
        }
      }

      // Properly parse JSON data
      json data;
      file >> data;

      std::string spriteType;

      // Determine sprite type from baseId
      if( baseId == 59 ) spriteType = "WaterSprite";
      else if( baseId == 133 ) spriteType = "WindSprite";
      else if( baseId == 135 ) spriteType = "LightningSprite";
      else if( baseId == 134 ) spriteType = "FireSprite";
      else if( baseId == 131 ) spriteType = "EarthSprite";
      else
      {
        std::cerr << "Unknown sprite baseId: " << baseId << std::endl;
        return;
      }

      if( !data[ "sprites" ].contains( spriteType ) )
      {
        std::cerr << "Sprite type not found in JSON: " << spriteType << std::endl;
        return;
      }

      const auto& spriteData = data[ "sprites" ][ spriteType ];

      // Configure the BNpc
      bnpc.setBNpcNameId( spriteData[ "nameId" ] );

      // Read isRanged and attackRange from JSON
      bool isRanged = spriteData.value( "isRanged", true );
      float attackRange = spriteData.value( "attackRange", 20.0f );

      bnpc.setIsRanged( isRanged );
      bnpc.setAttackRange( attackRange );
      bnpc.setIsMagical( true );



      // Get loop count from JSON or use default -1 (infinite)
      int8_t loopCount = spriteData[ "gambitPack" ].value( "loopCount", -1 );

      // Create the gambit pack with the loop count from JSON
      auto gambitPack = std::make_shared< GambitTimeLinePack >( loopCount );
      if( !gambitPack )
      {
        std::cerr << "Failed to create gambit pack" << std::endl;
        return;
      }

      // Add all timelines from JSON
      for( const auto& timeline : spriteData[ "gambitPack" ][ "timeLines" ] )
      {
        std::shared_ptr< GambitTargetCondition > condition;

        // Create the appropriate condition
        if( timeline[ "condition" ] == "TopHateTarget" )
          condition = make_TopHateTargetCondition();
        else
          condition = make_TopHateTargetCondition();// Default

        // Add timeline to gambit pack
        gambitPack->addTimeLine(
                condition,
                Action::make_Action(
                        bnpc.getAsChara(),
                        timeline[ "actionId" ],
                        timeline[ "actionParam" ] ),
                timeline[ "timing" ] );
      }

      // Store and set the gambit pack
      m_gambitPack = gambitPack;
      bnpc.setGambitPack( m_gambitPack );

      //std::cerr << "Successfully initialized sprite " << spriteType << " with baseId " << baseId << std::endl;

    } catch( const std::exception& e )
    {
      std::cerr << "Error processing sprite data: " << e.what() << std::endl;
    }
  }
};

////////////////////////////////////////////////////////////////////
// Specific Sprites
////////////////////////////////////////////////////////////////////
class WaterSprite : public SpriteBase
{
public:
  WaterSprite() : SpriteBase( 59 ) {}
};

class FireSprite : public SpriteBase
{
public:
  FireSprite() : SpriteBase( 134 ) {}
};

class EarthSprite : public SpriteBase
{
public:
  EarthSprite() : SpriteBase( 131 ) {}
};

class WindSprite : public SpriteBase
{
public:
  WindSprite() : SpriteBase( 133 ) {}
};

class LightningSprite : public SpriteBase
{
public:
  LightningSprite() : SpriteBase( 135 ) {}
};


// Register all three sprite classes
EXPOSE_SCRIPT( WaterSprite );
EXPOSE_SCRIPT( FireSprite );
EXPOSE_SCRIPT( EarthSprite );
EXPOSE_SCRIPT( WindSprite );
EXPOSE_SCRIPT( LightningSprite );
