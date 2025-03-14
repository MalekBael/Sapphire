#include <AI/GambitPack.h>
#include <AI/GambitTargetCondition.h>
#include <Action/Action.h>
#include <Actor/BNpc.h>
#include <ScriptObject.h>

using namespace Sapphire::World;
using namespace Sapphire::ScriptAPI;
using namespace Sapphire::World::AI;

class BnpcChigoe : public BattleNpcScript
{
public:
  BnpcChigoe() : BattleNpcScript( 43 ) {}

  void onInit( Sapphire::Entity::BNpc& bnpc ) override
  {
    auto gambitPack = std::make_shared< AI::GambitTimeLinePack >( -1 );

    gambitPack->addTimeLine(
            AI::make_TopHateTargetCondition(),
            Action::make_Action( bnpc.getAsChara(), 7, 0 ),
            2 );

    gambitPack->addTimeLine(
            AI::make_TopHateTargetCondition(),
            Action::make_Action( bnpc.getAsChara(), 7, 0 ),
            6 );

    // Additional timelines can be uncommented and customized as needed
    /*
        gambitPack->addTimeLine(
            AI::make_TopHateTargetCondition(),
            Action::make_Action(bnpc.getAsChara(), 163, 0),
            6
        );
        gambitPack->addTimeLine(
            AI::make_TopHateTargetCondition(),
            Action::make_Action(bnpc.getAsChara(), 7, 0),
            8
        );
        gambitPack->addTimeLine(
            AI::make_TopHateTargetCondition(),
            Action::make_Action(bnpc.getAsChara(), 7, 0),
            10
        );
        gambitPack->addTimeLine(
            AI::make_TopHateTargetCondition(),
            Action::make_Action(bnpc.getAsChara(), 7, 0),
            12
        );
        gambitPack->addTimeLine(
            AI::make_TopHateTargetCondition(),
            Action::make_Action(bnpc.getAsChara(), 7, 0),
            14
        );
        */

    m_gambitPack = gambitPack;

    bnpc.setGambitPack( m_gambitPack );
  }

private:
  std::shared_ptr< AI::GambitPack > m_gambitPack;
};

EXPOSE_SCRIPT( BnpcChigoe );
