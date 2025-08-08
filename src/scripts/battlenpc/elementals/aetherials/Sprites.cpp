#include <AI/GambitPack.h>
#include <AI/GambitTargetCondition.h>
#include <Action/Action.h>
#include <Actor/BNpc.h>
#include <Script/NativeScriptApi.h>
#include <ScriptObject.h>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

#ifdef _WIN32
#include <direct.h>
#define getcwd _getcwd
#else
#include <unistd.h>
#endif

using namespace Sapphire::World;
using namespace Sapphire::ScriptAPI;
using namespace Sapphire::World::AI;
using json = nlohmann::json;

json loadSpriteJson()
{
  // Add this to see the current working directory
  char currentPath[ FILENAME_MAX ];
  if( getcwd( currentPath, sizeof( currentPath ) ) )
  {
    std::cout << "[Sprites] Current working directory: " << currentPath << std::endl;
  }

  // The correct path should be the first one since JSON is copied to bin/data/...
  std::vector< std::string > allPaths = {
          "data/battlenpc/elementals/aetherials/sprites.json",// This should work now!
          "./data/battlenpc/elementals/aetherials/sprites.json",
          "../data/battlenpc/elementals/aetherials/sprites.json",
          "../../data/battlenpc/elementals/aetherials/sprites.json" };

  std::ifstream file;
  std::string successPath;

  for( const auto& path : allPaths )
  {
    file.close();
    file.clear();
    file.open( path );
    if( file.is_open() )
    {
      successPath = path;
      std::cout << "[Sprites] Successfully opened JSON at: " << path << std::endl;
      break;
    }
    else
    {
      std::cout << "[Sprites] Failed to open JSON at: " << path << std::endl;
    }
  }

  if( !file.is_open() )
  {
    std::cout << "[Sprites] ERROR: Could not find sprites.json at any of these paths:" << std::endl;
    for( const auto& path : allPaths )
    {
      std::cout << "  - " << path << std::endl;
    }
    return json();
  }

  json data;
  try
  {
    file >> data;
    std::cout << "[Sprites] Successfully loaded JSON with " << data[ "sprites" ].size() << " sprite types" << std::endl;
    return data;
  } catch( const std::exception& e )
  {
    std::cout << "[Sprites] ERROR parsing JSON: " << e.what() << std::endl;
    return json();
  }
}

// Base class with common implementation for all sprite types
class SpriteBase : public BattleNpcScript
{
public:
  // Constructor taking spriteType string and using baseId from JSON
  SpriteBase( const std::string& spriteType )
      : BattleNpcScript( getBaseIdFromJson( spriteType ) ),
        m_spriteType( spriteType )
  {
    uint32_t baseId = getBaseIdFromJson( spriteType );
    std::cout << "[Sprites] Created " << spriteType << " with baseId: " << baseId << std::endl;
  }

  void onInit( Sapphire::Entity::BNpc& bnpc ) override
  {
    std::cout << "[Sprites] onInit called for " << m_spriteType << std::endl;
    loadAndApplyGambitPack( bnpc );
  }

protected:
  std::string m_spriteType;
  std::shared_ptr< GambitPack > m_gambitPack;

  static uint32_t getBaseIdFromJson( const std::string& spriteType )
  {
    static json spriteData = loadSpriteJson();

    if( spriteData.empty() )
    {
      std::cout << "[Sprites] ERROR: Sprite data is empty for " << spriteType << std::endl;
      return 0;
    }

    if( !spriteData.contains( "sprites" ) )
    {
      std::cout << "[Sprites] ERROR: No 'sprites' key in JSON for " << spriteType << std::endl;
      return 0;
    }

    if( !spriteData[ "sprites" ].contains( spriteType ) )
    {
      std::cout << "[Sprites] ERROR: Sprite type '" << spriteType << "' not found in JSON" << std::endl;
      return 0;
    }

    if( !spriteData[ "sprites" ][ spriteType ].contains( "baseId" ) )
    {
      std::cout << "[Sprites] ERROR: No 'baseId' for sprite type " << spriteType << std::endl;
      return 0;
    }

    uint32_t baseId = spriteData[ "sprites" ][ spriteType ][ "baseId" ];
    std::cout << "[Sprites] Found baseId " << baseId << " for " << spriteType << std::endl;
    return baseId;
  }

  void loadAndApplyGambitPack( Sapphire::Entity::BNpc& bnpc )
  {
    try
    {
      json data = loadSpriteJson();
      if( data.empty() )
      {
        std::cout << "[Sprites] ERROR: JSON data is empty for " << m_spriteType << std::endl;
        return;
      }

      if( !data[ "sprites" ].contains( m_spriteType ) )
      {
        std::cout << "[Sprites] ERROR: Sprite type '" << m_spriteType << "' not found in JSON" << std::endl;
        return;
      }

      const auto& spriteData = data[ "sprites" ][ m_spriteType ];
      std::cout << "[Sprites] Configuring " << m_spriteType << std::endl;

      // Configure the BNpc
      if( spriteData.contains( "nameId" ) )
      {
        uint32_t nameId = spriteData[ "nameId" ];
        bnpc.setBNpcNameId( nameId );
        std::cout << "[Sprites] Set nameId: " << nameId << std::endl;
      }

      // Read isRanged and attackRange from JSON
      bool isRanged = spriteData.value( "isRanged", true );
      float attackRange = spriteData.value( "attackRange", 20.0f );

      bnpc.setIsRanged( isRanged );
      bnpc.setAttackRange( attackRange );
      bnpc.setIsMagical( true );

      std::cout << "[Sprites] Set isRanged: " << isRanged << ", attackRange: " << attackRange << std::endl;

      // Set up gambit pack if present
      if( !spriteData.contains( "gambitPack" ) )
      {
        std::cout << "[Sprites] No gambit pack data for " << m_spriteType << std::endl;
        return;
      }

      // Get loop count from JSON or use default -1 (infinite)
      int8_t loopCount = spriteData[ "gambitPack" ].value( "loopCount", -1 );

      // Create the gambit pack with the loop count from JSON
      auto gambitPack = std::make_shared< GambitTimeLinePack >( loopCount );
      if( !gambitPack )
      {
        std::cout << "[Sprites] ERROR: Failed to create gambit pack for " << m_spriteType << std::endl;
        return;
      }

      std::cout << "[Sprites] Created gambit pack with loop count: " << ( int ) loopCount << std::endl;

      // Add all timelines from JSON
      int timelineCount = 0;
      for( const auto& timeline : spriteData[ "gambitPack" ][ "timeLines" ] )
      {
        std::shared_ptr< GambitTargetCondition > condition;

        // Create the appropriate condition
        if( timeline[ "condition" ] == "TopHateTarget" )
          condition = make_TopHateTargetCondition();
        else
          condition = make_TopHateTargetCondition();// Default

        gambitPack->addTimeLine(
                condition,
                Action::make_Action(
                        bnpc.getAsChara(),
                        timeline[ "actionId" ],
                        timeline[ "actionParam" ] ),
                timeline[ "timing" ] );

        std::cout << "[Sprites] Added timeline " << timelineCount++ << ": actionId="
                  << timeline[ "actionId" ] << ", timing=" << timeline[ "timing" ] << std::endl;
      }

      // Store and set the gambit pack
      m_gambitPack = gambitPack;
      bnpc.setGambitPack( m_gambitPack );

      std::cout << "[Sprites] Successfully configured " << m_spriteType << " with " << timelineCount << " timelines" << std::endl;
    } catch( const std::exception& e )
    {
      std::cout << "[Sprites] ERROR in loadAndApplyGambitPack for " << m_spriteType << ": " << e.what() << std::endl;
    }
  }
};

////////////////////////////////////////////////////////////////////
// Specific Sprites
////////////////////////////////////////////////////////////////////
class WaterSprite : public SpriteBase
{
public:
  WaterSprite() : SpriteBase( "WaterSprite" ) {}
};

class TaintedWaterSprite : public SpriteBase
{
public:
  TaintedWaterSprite() : SpriteBase( "TaintedWaterSprite" ) {}
};

class DeepStainedWaterSprite : public SpriteBase
{
public:
  DeepStainedWaterSprite() : SpriteBase( "DeepStainedWaterSprite" ) {}
};

class FireSprite : public SpriteBase
{
public:
  FireSprite() : SpriteBase( "FireSprite" ) {}
};

class EarthSprite : public SpriteBase
{
public:
  EarthSprite() : SpriteBase( "EarthSprite" ) {}
};

class TaintedEarthSprite : public SpriteBase
{
public:
  TaintedEarthSprite() : SpriteBase( "TaintedEarthSprite" ) {}
};

class WindSprite : public SpriteBase
{
public:
  WindSprite() : SpriteBase( "WindSprite" ) {}
};

class CrazedSprite : public SpriteBase
{
public:
  CrazedSprite() : SpriteBase( "CrazedSprite" ) {}
};

class TaintedWindSprite : public SpriteBase
{
public:
  TaintedWindSprite() : SpriteBase( "TaintedWindSprite" ) {}
};

class DeepStainedWindSprite : public SpriteBase
{
public:
  DeepStainedWindSprite() : SpriteBase( "DeepStainedWindSprite" ) {}
};

class LightningSprite : public SpriteBase
{
public:
  LightningSprite() : SpriteBase( "LightningSprite" ) {}
};

class DeepStainedLightningSprite : public SpriteBase
{
public:
  DeepStainedLightningSprite() : SpriteBase( "DeepStainedLightningSprite" ) {}
};

class IceSprite : public SpriteBase
{
public:
  IceSprite() : SpriteBase( "IceSprite" ) {}
};

class FlameShiver : public SpriteBase
{
public:
  FlameShiver() : SpriteBase( "FlameShiver" ) {}
};

class StoneShiver : public SpriteBase
{
public:
  StoneShiver() : SpriteBase( "StoneShiver" ) {}
};

// Register all sprite classes
EXPOSE_SCRIPT( WaterSprite );
EXPOSE_SCRIPT( TaintedWaterSprite );
EXPOSE_SCRIPT( DeepStainedWaterSprite );
EXPOSE_SCRIPT( FireSprite );
EXPOSE_SCRIPT( EarthSprite );
EXPOSE_SCRIPT( TaintedEarthSprite );
EXPOSE_SCRIPT( WindSprite );
EXPOSE_SCRIPT( CrazedSprite );
EXPOSE_SCRIPT( DeepStainedWindSprite );
EXPOSE_SCRIPT( TaintedWindSprite );
EXPOSE_SCRIPT( LightningSprite );
EXPOSE_SCRIPT( DeepStainedLightningSprite );
EXPOSE_SCRIPT( IceSprite );
EXPOSE_SCRIPT( FlameShiver );
EXPOSE_SCRIPT( StoneShiver );