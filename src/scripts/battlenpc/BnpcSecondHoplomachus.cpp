#include <AI/GambitPack.h>
#include <AI/GambitTargetCondition.h>
#include <Action/Action.h>
#include <Actor/BNpc.h>
#include <ScriptObject.h>

using namespace Sapphire::World;
using namespace Sapphire::ScriptAPI;
using namespace Sapphire::World::AI;

class BnpcSecondHoplomachus : public BattleNpcScript
{
public:
  BnpcSecondHoplomachus() : BattleNpcScript( 1821 ) {}

  void onInit( Sapphire::Entity::BNpc& bnpc ) override
  {
    std::cout << "Initializing BnpcSecondHoplomachus with BNpc ID: " << bnpc.getBNpcBaseId() << std::endl;
    auto gambitPack = std::make_shared< AI::GambitTimeLinePack >( -1 );

    gambitPack->addTimeLine(
            AI::make_TopHateTargetCondition(),
            Action::make_Action( bnpc.getAsChara(), 10, 0 ),
            0 );

    gambitPack->addTimeLine(
            AI::make_TopHateTargetCondition(),
            Action::make_Action( bnpc.getAsChara(), 7, 0 ),
            2 );
    
    gambitPack->addTimeLine(
            AI::make_TopHateTargetCondition(),
            Action::make_Action(bnpc.getAsChara(), 309, 0),
            4 );
    gambitPack->addTimeLine(
            AI::make_TopHateTargetCondition(),
            Action::make_Action(bnpc.getAsChara(), 9, 0),
            6 );
    gambitPack->addTimeLine(
            AI::make_TopHateTargetCondition(),
            Action::make_Action(bnpc.getAsChara(), 7, 0),
            8 );
    gambitPack->addTimeLine(
            AI::make_TopHateTargetCondition(),
            Action::make_Action(bnpc.getAsChara(), 9, 0),
            10 );
    gambitPack->addTimeLine(
            AI::make_TopHateTargetCondition(),
            Action::make_Action(bnpc.getAsChara(), 7, 0),
            12 );
        

    m_gambitPack = gambitPack;

    bnpc.setGambitPack( m_gambitPack );
  }

private:
  std::shared_ptr< AI::GambitPack > m_gambitPack;
};

EXPOSE_SCRIPT( BnpcSecondHoplomachus );
