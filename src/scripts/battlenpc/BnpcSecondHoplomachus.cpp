// File: src/scripts/battlenpc/BnpcSecondHoplomachus.cpp

#include <AI/GambitPack.h>
#include <AI/GambitTargetCondition.h>
#include <Action/Action.h>
#include <Actor/BNpc.h>
#include <Logging/Logger.h>
#include <ScriptObject.h>

using namespace Sapphire::World;
using namespace Sapphire::ScriptAPI;
using namespace Sapphire::World::AI;
using namespace Sapphire::Entity;

   // In BnpcSecondHoplomachus.cpp
class BnpcSecondHoplomachus : public BattleNpcScript
{
public:
  BnpcSecondHoplomachus() : BattleNpcScript( 55 ) {}

  void onInit( Sapphire::Entity::BNpc& bnpc ) override
  {
    bnpc.setBNpcNameId( 1821 );
    // Initialize the GambitPack as needed
    auto gambitPack = std::make_shared< Sapphire::World::AI::GambitTimeLinePack >( -1 );

    gambitPack->addTimeLine(
            Sapphire::World::AI::make_TopHateTargetCondition(),
            Sapphire::World::Action::make_Action( bnpc.getAsChara(), 10, 0 ),
            0 );

    gambitPack->addTimeLine(
            AI::make_TopHateTargetCondition(),
            Action::make_Action( bnpc.getAsChara(), 7, 0 ),
            2 );

    gambitPack->addTimeLine(
            AI::make_TopHateTargetCondition(),
            Action::make_Action( bnpc.getAsChara(), 404, 0 ),
            4 );

    gambitPack->addTimeLine(
            AI::make_TopHateTargetCondition(),
            Action::make_Action( bnpc.getAsChara(), 9, 0 ),
            6 );

    gambitPack->addTimeLine(
            AI::make_TopHateTargetCondition(),
            Action::make_Action( bnpc.getAsChara(), 7, 0 ),
            8 );

    gambitPack->addTimeLine(
            AI::make_TopHateTargetCondition(),
            Action::make_Action( bnpc.getAsChara(), 9, 0 ),
            10 );

    gambitPack->addTimeLine(
            AI::make_TopHateTargetCondition(),
            Action::make_Action( bnpc.getAsChara(), 7, 0 ),
            12 );

    m_gambitPack = gambitPack;
    bnpc.setGambitPack( m_gambitPack );
  }

private:
  std::shared_ptr< Sapphire::World::AI::GambitPack > m_gambitPack;
};

EXPOSE_SCRIPT( BnpcSecondHoplomachus );