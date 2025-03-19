#include <AI/GambitPack.h>
#include <AI/GambitTargetCondition.h>
#include <Action/Action.h>
#include <Actor/BNpc.h>
#include <Script/NativeScriptApi.h>
#include <ScriptObject.h>
#include <fstream>
#include <nlohmann/json.hpp>

using namespace Sapphire::World;
using namespace Sapphire::ScriptAPI;
using namespace Sapphire::World::AI;
using json = nlohmann::json;

json loadSpriteJson()
{
  const std::string jsonPath = "data/battlenpc/elementals/sprites.json";
  std::ifstream file( jsonPath );

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
        break;
    }

    if( !file.is_open() )
      return json();
  }

  json data;
  try
  {
    file >> data;
    return data;
  } catch( const std::exception& )
  {
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
  }

  void onInit( Sapphire::Entity::BNpc& bnpc ) override
  {
    loadAndApplyGambitPack( bnpc );
  }

protected:
  std::string m_spriteType;
  std::shared_ptr< GambitPack > m_gambitPack;

  static uint32_t getBaseIdFromJson( const std::string& spriteType )
  {
    static json spriteData = loadSpriteJson();

    if( spriteData.empty() ||
        !spriteData.contains( "sprites" ) ||
        !spriteData[ "sprites" ].contains( spriteType ) ||
        !spriteData[ "sprites" ][ spriteType ].contains( "baseId" ) )
    {
      return 0;
    }

    return spriteData[ "sprites" ][ spriteType ][ "baseId" ];
  }

  void loadAndApplyGambitPack( Sapphire::Entity::BNpc& bnpc )
  {
    try
    {
      json data = loadSpriteJson();
      if( data.empty() ) return;

      if( !data[ "sprites" ].contains( m_spriteType ) )
        return;

      const auto& spriteData = data[ "sprites" ][ m_spriteType ];

      // Configure the BNpc
      if( spriteData.contains( "nameId" ) )
        bnpc.setBNpcNameId( spriteData[ "nameId" ] );

      // Read isRanged and attackRange from JSON
      bool isRanged = spriteData.value( "isRanged", true );
      float attackRange = spriteData.value( "attackRange", 20.0f );

      bnpc.setIsRanged( isRanged );
      bnpc.setAttackRange( attackRange );
      bnpc.setIsMagical( true );

      // Set up gambit pack if present
      if( !spriteData.contains( "gambitPack" ) )
        return;

      // Get loop count from JSON or use default -1 (infinite)
      int8_t loopCount = spriteData[ "gambitPack" ].value( "loopCount", -1 );

      // Create the gambit pack with the loop count from JSON
      auto gambitPack = std::make_shared< GambitTimeLinePack >( loopCount );
      if( !gambitPack )
        return;

      // Add all timelines from JSON
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
      }

      // Store and set the gambit pack
      m_gambitPack = gambitPack;
      bnpc.setGambitPack( m_gambitPack );

    } catch( const std::exception& )
    {
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
