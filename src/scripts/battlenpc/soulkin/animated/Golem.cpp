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

json loadGolemJson()
{
  const std::string jsonPath = "data/battlenpc/soulin/animated/golem.json";
  std::ifstream file( jsonPath );

  if( !file.is_open() )
  {
    std::vector< std::string > alternativePaths = {
            "../data/battlenpc/soulin/animated/golem.json",
            "../../data/battlenpc/soulin/animated/golem.json",
            "./data/battlenpc/soulin/animated/golem.json" };

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

class GolemBase : public BattleNpcScript
{
public:
  GolemBase( const std::string& golemType )
      : BattleNpcScript( getBaseIdFromJson( golemType ) ),
        m_golemType( golemType )
  {
  }

  void onInit( Sapphire::Entity::BNpc& bnpc ) override
  {
    loadAndApplyGambitPack( bnpc );
  }

protected:
  std::string m_golemType;
  std::shared_ptr< GambitPack > m_gambitPack;

  static uint32_t getBaseIdFromJson( const std::string& golemType )
  {
    static json golemData = loadGolemJson();

    if( golemData.empty() ||
        !golemData.contains( "golem" ) ||
        !golemData[ "golem" ].contains( golemType ) ||
        !golemData[ "golem" ][ golemType ].contains( "baseId" ) )
    {
      return 0;
    }

    return golemData[ "golem" ][ golemType ][ "baseId" ];
  }

  void loadAndApplyGambitPack( Sapphire::Entity::BNpc& bnpc )
  {
    try
    {
      json data = loadGolemJson();
      if( data.empty() ) return;

      if( !data[ "golem" ].contains( m_golemType ) )
        return;

      const auto& golemData = data[ "golem" ][ m_golemType ];

      if( golemData.contains( "nameId" ) )
        bnpc.setBNpcNameId( golemData[ "nameId" ] );

      bool isRanged = golemData.value( "isRanged", false );
      float attackRange = golemData.value( "attackRange", 20.0f );

      bnpc.setIsRanged( isRanged );
      bnpc.setAttackRange( attackRange );
      bnpc.setIsMagical( false );

      if( !golemData.contains( "gambitPack" ) )
        return;

      int8_t loopCount = golemData[ "gambitPack" ].value( "loopCount", -1 );

      auto gambitPack = std::make_shared< GambitTimeLinePack >( loopCount );
      if( !gambitPack )
        return;

      for( const auto& timeline : golemData[ "gambitPack" ][ "timeLines" ] )
      {
        std::shared_ptr< GambitTargetCondition > condition;

        if( timeline[ "condition" ] == "TopHateTarget" )
          condition = make_TopHateTargetCondition();
        else
          condition = make_TopHateTargetCondition();

        auto action = Action::make_Action(
            bnpc.getAsChara(),
            timeline[ "actionId" ],
            timeline.value( "actionParam", 0 ) );

        gambitPack->addTimeLine(
                condition,
                action,
                timeline[ "timing" ] );
      }

      m_gambitPack = gambitPack;
      bnpc.setGambitPack( m_gambitPack );

    } catch( const std::exception& )
    {
    }
  }
};

////////////////////////////////////////////////////////////////////
// Specific Golem
////////////////////////////////////////////////////////////////////

class ClayGolem : public GolemBase
{
public:
  ClayGolem() : GolemBase( "ClayGolem" ) {}
};


EXPOSE_SCRIPT( ClayGolem );