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

json loadEnemyHumanoidsJson()
{
  const std::string jsonPath = "data/battlenpc/spoken/enemyhumanoids.json";
  std::ifstream file( jsonPath );

  if( !file.is_open() )
  {
    std::vector< std::string > alternativePaths = {
            "../data/battlenpc/spoken/enemyhumanoids.json",
            "../../data/battlenpc/spoken/enemyhumanoids.json",
            "./data/battlenpc/spoken/enemyhumanoids.json" };

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

// Base class with common implementation for all EnemyHumanoid types
class EnemyHumanoidBase : public BattleNpcScript
{
public:
  // Constructor taking enemyHumanoidType string and using baseId from JSON
  EnemyHumanoidBase( const std::string& enemyHumanoidType )
      : BattleNpcScript( getBaseIdFromJson( enemyHumanoidType ) ),
        m_enemyHumanoidType( enemyHumanoidType )
  {
  }

  void onInit( Sapphire::Entity::BNpc& bnpc ) override
  {
    loadAndApplyGambitPack( bnpc );
  }

protected:
  std::string m_enemyHumanoidType;
  std::shared_ptr< GambitPack > m_gambitPack;

  static uint32_t getBaseIdFromJson( const std::string& enemyHumanoidType )
  {
    static json enemyHumanoidData = loadEnemyHumanoidsJson();

    if( enemyHumanoidData.empty() ||
        !enemyHumanoidData.contains( "enemyHumanoids" ) ||
        !enemyHumanoidData[ "enemyHumanoids" ].contains( enemyHumanoidType ) ||
        !enemyHumanoidData[ "enemyHumanoids" ][ enemyHumanoidType ].contains( "baseId" ) )
    {
      return 0;
    }

    return enemyHumanoidData[ "enemyHumanoids" ][ enemyHumanoidType ][ "baseId" ];
  }

  void loadAndApplyGambitPack( Sapphire::Entity::BNpc& bnpc )
  {
    try
    {
      json data = loadEnemyHumanoidsJson();
      if( data.empty() ) return;

      if( !data[ "enemyHumanoids" ].contains( m_enemyHumanoidType ) )
        return;

      const auto& enemyHumanoidData = data[ "enemyHumanoids" ][ m_enemyHumanoidType ];

      // Configure the BNpc
      if( enemyHumanoidData.contains( "nameId" ) )
        bnpc.setBNpcNameId( enemyHumanoidData[ "nameId" ] );

      // Read isRanged from JSON (and use default value false for humanoids)
      bool isRanged = enemyHumanoidData.value( "isRanged", false );

      // Set the BNpc properties
      bnpc.setIsRanged( isRanged );
      // Don't set attackRange if not ranged - will use the default 3.0f from BNpc class
      if( isRanged && enemyHumanoidData.contains( "attackRange" ) )
      {
        float attackRange = enemyHumanoidData.value( "attackRange", 20.0f );
        bnpc.setAttackRange( attackRange );
      }

      // Read isMagical from JSON (and use default value false for humanoids)
      bool isMagical = enemyHumanoidData.value( "isMagical", false );
      bnpc.setIsMagical( isMagical );

      // Set up gambit pack if present
      if( !enemyHumanoidData.contains( "gambitPack" ) )
        return;

      // Get loop count from JSON or use default -1 (infinite)
      int8_t loopCount = enemyHumanoidData[ "gambitPack" ].value( "loopCount", -1 );

      // Create the gambit pack with the loop count from JSON
      auto gambitPack = std::make_shared< GambitTimeLinePack >( loopCount );
      if( !gambitPack )
        return;

      // Add all timelines from JSON
      for( const auto& timeline : enemyHumanoidData[ "gambitPack" ][ "timeLines" ] )
      {
        std::shared_ptr< GambitTargetCondition > condition;

        // Create the appropriate condition based on JSON setting
        if( timeline[ "condition" ] == "TopHateTarget" )
          condition = make_TopHateTargetCondition();
        else if( timeline[ "condition" ] == "Self" )
          condition = std::make_shared< HPSelfPctLessThanTargetCondition >( 101 );// Always true for self-targeting
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
// Specific Humanoids
////////////////////////////////////////////////////////////////////
class YellowJacketPatrol : public EnemyHumanoidBase
{
public:
  YellowJacketPatrol() : EnemyHumanoidBase( "YellowJacketPatrol" ) {}
};

class ThirdCohortSignifer : public EnemyHumanoidBase
{
public:
  ThirdCohortSignifer() : EnemyHumanoidBase( "ThirdCohortSignifer" ) {}
};

EXPOSE_SCRIPT( YellowJacketPatrol );
EXPOSE_SCRIPT( ThirdCohortSignifer );
