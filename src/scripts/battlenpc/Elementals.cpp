// File: src/scripts/battlenpc/Elementals.cpp

#include <AI/GambitPack.h>
#include <AI/GambitTargetCondition.h>
#include <Action/Action.h>
#include <Actor/BNpc.h>
#include <Script/NativeScriptApi.h>
#include <ScriptObject.h>

using namespace Sapphire::ScriptAPI;
using namespace Sapphire;

// Water Sprite Script
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
      bnpc.setBNpcNameId( 56 );// nameId = 56

      auto gambitPack = std::make_shared< Sapphire::World::AI::GambitTimeLinePack >( -1 );
      if( !gambitPack )
      {
        return;
      }

      gambitPack->addTimeLine(
              Sapphire::World::AI::make_TopHateTargetCondition(),
              Sapphire::World::Action::make_Action( bnpc.getAsChara(), 1120, 0 ),
              0 );

      gambitPack->addTimeLine(
              Sapphire::World::AI::make_TopHateTargetCondition(),
              Sapphire::World::Action::make_Action( bnpc.getAsChara(), 1120, 0 ),
              2 );



      m_gambitPack = gambitPack;
      bnpc.setGambitPack( m_gambitPack );
    } catch( const std::exception& )
    {
      // Exception handling without logging
    }
  }

private:
  std::shared_ptr< Sapphire::World::AI::GambitPack > m_gambitPack;
};

// Fire Sprite Script
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
      bnpc.setBNpcNameId( 116 );// nameId = 116

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
              Sapphire::World::Action::make_Action( bnpc.getAsChara(), 141, 0 ),
              2 );

      m_gambitPack = gambitPack;
      bnpc.setGambitPack( m_gambitPack );
    } catch( const std::exception& )
    {
      // Exception handling without logging
    }
  }

private:
  std::shared_ptr< Sapphire::World::AI::GambitPack > m_gambitPack;
};

// Earth Sprite Script
class EarthSprite : public BattleNpcScript
{
public:
  EarthSprite() : BattleNpcScript( 131 )// baseId = 131
  {
    // Constructor body without Logger
  }

  void onInit( Sapphire::Entity::BNpc& bnpc ) override
  {
    if( !&bnpc )
    {
      return;
    }

    try
    {
      bnpc.setBNpcNameId( 113 );// nameId = 113

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
      // Exception handling without logging
    }
  }

private:
  std::shared_ptr< Sapphire::World::AI::GambitPack > m_gambitPack;
};

// Expose Each Script
EXPOSE_SCRIPT( WaterSprite );
EXPOSE_SCRIPT( FireSprite );
EXPOSE_SCRIPT( EarthSprite );
