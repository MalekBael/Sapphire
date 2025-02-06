// File: src/scripts/battlenpc/BnpcBNPC Name 1821.cpp

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

class BnpcBNPC Name 1821 : public BattleNpcScript
{
public:
    BnpcBNPC Name 1821() : BattleNpcScript( 54 ) {}

    void onInit( Sapphire::Entity::BNpc& bnpc ) override
    {
        bnpc.setBNpcNameId( 1821 );
        auto gambitPack = std::make_shared< Sapphire::World::AI::GambitTimeLinePack >( -1 );

        gambitPack->addTimeLine(
                Sapphire::World::AI::make_TopHateTargetCondition(),
                Sapphire::World::Action::make_Action( bnpc.getAsChara(), 404, 0 ),
                0 );

        gambitPack->addTimeLine(
                Sapphire::World::AI::make_TopHateTargetCondition(),
                Sapphire::World::Action::make_Action( bnpc.getAsChara(), 10, 0 ),
                0 );

        gambitPack->addTimeLine(
                Sapphire::World::AI::make_TopHateTargetCondition(),
                Sapphire::World::Action::make_Action( bnpc.getAsChara(), 999, 0 ),
                0 );

        gambitPack->addTimeLine(
                Sapphire::World::AI::make_TopHateTargetCondition(),
                Sapphire::World::Action::make_Action( bnpc.getAsChara(), 999, 0 ),
                0 );

        gambitPack->addTimeLine(
                Sapphire::World::AI::make_TopHateTargetCondition(),
                Sapphire::World::Action::make_Action( bnpc.getAsChara(), 404, 0 ),
                0 );

        m_gambitPack = gambitPack;
        bnpc.setGambitPack( m_gambitPack );
    }

private:
    std::shared_ptr< Sapphire::World::AI::GambitPack > m_gambitPack;
};

EXPOSE_SCRIPT( BnpcBNPC Name 1821 );