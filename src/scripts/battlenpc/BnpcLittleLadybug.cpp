#include "../src/scripts/ScriptObject.h"
#include "../src/world/Action/CommonAction.h"
#include "../src/world/Actor/GameObject.h"
#include "../src/world/Script/NativeScriptApi.h"
#include "../src/world/Script/NativeScriptMgr.h"
#include <AI/Fsm/Condition.h>
#include <AI/Fsm/StateCombat.h>
#include <AI/Fsm/StateDead.h>
#include <AI/Fsm/StateIdle.h>
#include <AI/Fsm/StateMachine.h>
#include <AI/Fsm/StateRetreat.h>
#include <AI/Fsm/StateRoam.h>
#include <AI/GambitPack.h>
#include <AI/GambitRule.h>
#include <AI/GambitTargetCondition.h>
#include <Action/Action.h>
#include <Actor/BNpc.h>

using namespace Sapphire::World;
using namespace Sapphire::ScriptAPI;
using namespace Sapphire::World::AI;

class BnpcLittleLadybug : public BattleNpcScript
{
public:
  BnpcLittleLadybug() : BattleNpcScript( 49 ) {}

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

EXPOSE_SCRIPT( BnpcLittleLadybug );
