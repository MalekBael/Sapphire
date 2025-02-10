#include <AI/GambitPack.h>
#include <AI/GambitTargetCondition.h>
#include <Action/Action.h>
#include <Actor/BNpc.h>
#include <Script/NativeScriptApi.h>
#include <ScriptObject.h>

using namespace Sapphire::ScriptAPI;
using namespace Sapphire;


//////////////////////////////////////////////////////////////////////////////////////////////////////
// Water Sprite Script | Need to find correct water spell id
//////////////////////////////////////////////////////////////////////////////////////////////////////

class WaterSprite : public BattleNpcScript
{
public:
  WaterSprite() : BattleNpcScript( 59 )// baseId = 59
  {
  }

  void onInit( Sapphire::Entity::BNpc& bnpc ) override
  {
    if( !&bnpc )
    {
      return;
    }

    try
    {
      bnpc.setBNpcNameId( 56 );
      //defining range
      bnpc.setIsRanged( true );
      bnpc.setAttackRange( 20.0f );

      auto gambitPack = std::make_shared< Sapphire::World::AI::GambitTimeLinePack >( -1 );
      if( !gambitPack )
      {
        return;
      }

      gambitPack->addTimeLine(
              Sapphire::World::AI::make_TopHateTargetCondition(),
              Sapphire::World::Action::make_Action( bnpc.getAsChara(), 134, 0 ),
              0 );

      gambitPack->addTimeLine(
              Sapphire::World::AI::make_TopHateTargetCondition(),
              Sapphire::World::Action::make_Action( bnpc.getAsChara(), 971, 0 ),
              2 );


      m_gambitPack = gambitPack;
      bnpc.setGambitPack( m_gambitPack );
    } catch( const std::exception& )
    {
    }
  }

private:
  std::shared_ptr< Sapphire::World::AI::GambitPack > m_gambitPack;
};

// End of Water Sprite Script


//////////////////////////////////////////////////////////////////////////////////////////////////////
// Fire Sprite Script
//////////////////////////////////////////////////////////////////////////////////////////////////////

class FireSprite : public BattleNpcScript
{
public:
  FireSprite() : BattleNpcScript( 134 )// baseId = 134
  {
  }

  void onInit( Sapphire::Entity::BNpc& bnpc ) override
  {
    if( !&bnpc )
    {
      return;
    }

    try
    {
      bnpc.setBNpcNameId( 116 );
      //defining range
      bnpc.setIsRanged( true );
      bnpc.setAttackRange( 20.0f );

      auto gambitPack = std::make_shared< Sapphire::World::AI::GambitTimeLinePack >( -1 );
      if( !gambitPack )
      {
        return;
      }

      gambitPack->addTimeLine(
              Sapphire::World::AI::make_TopHateTargetCondition(),
              Sapphire::World::Action::make_Action( bnpc.getAsChara(), 141, 0 ),
              0 );

      gambitPack->addTimeLine(
              Sapphire::World::AI::make_TopHateTargetCondition(),
              Sapphire::World::Action::make_Action( bnpc.getAsChara(), 5325, 0 ),
              4 );

      gambitPack->addTimeLine(
              Sapphire::World::AI::make_TopHateTargetCondition(),
              Sapphire::World::Action::make_Action( bnpc.getAsChara(), 3497, 0 ),
              8 );

      m_gambitPack = gambitPack;
      bnpc.setGambitPack( m_gambitPack );
    } catch( const std::exception& )
    {
    }
  }

private:
  std::shared_ptr< Sapphire::World::AI::GambitPack > m_gambitPack;
};

// End of Fire Sprite Script


//////////////////////////////////////////////////////////////////////////////////////////////////////
// Earth Sprite Script
//////////////////////////////////////////////////////////////////////////////////////////////////////

class EarthSprite : public BattleNpcScript
{
public:
  EarthSprite() : BattleNpcScript( 131 )// baseId = 131
  {
  }

  void onInit( Sapphire::Entity::BNpc& bnpc ) override
  {
    if( !&bnpc )
    {
      return;
    }

    try
    {
      bnpc.setBNpcNameId( 113 );
      //defining range
      bnpc.setIsRanged( true );
      bnpc.setAttackRange( 20.0f );

      auto gambitPack = std::make_shared< Sapphire::World::AI::GambitTimeLinePack >( -1 );
      if( !gambitPack )
      {
        return;
      }

      gambitPack->addTimeLine(
              Sapphire::World::AI::make_TopHateTargetCondition(),
              Sapphire::World::Action::make_Action( bnpc.getAsChara(), 119, 0 ),
              0 );

      gambitPack->addTimeLine(
              Sapphire::World::AI::make_TopHateTargetCondition(),
              Sapphire::World::Action::make_Action( bnpc.getAsChara(), 119, 0 ),
              2 );

      m_gambitPack = gambitPack;
      bnpc.setGambitPack( m_gambitPack );
    } catch( const std::exception& )
    {
    }
  }

private:
  std::shared_ptr< Sapphire::World::AI::GambitPack > m_gambitPack;
};

// End of Earth Sprite Script

//////////////////////////////////////////////////////////////////////////////////////////////////////
// Expose Each Script | Add these to src/scripts/battlenpc/Scriptloader.cpp.in
//////////////////////////////////////////////////////////////////////////////////////////////////////

EXPOSE_SCRIPT( WaterSprite );
EXPOSE_SCRIPT( FireSprite );
EXPOSE_SCRIPT( EarthSprite );