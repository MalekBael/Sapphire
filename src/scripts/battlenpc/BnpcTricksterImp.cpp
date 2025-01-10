// File: src/scripts/battlenpc/BnpcTricksterImp.cpp

#include <AI/GambitPack.h>
#include <AI/GambitTargetCondition.h>
#include <Action/Action.h>
#include <Actor/BNpc.h>
#include <ScriptObject.h>
#include <Logging/Logger.h>

// Avoid using broad 'using namespace' to prevent ambiguity
using namespace Sapphire::World;
using namespace Sapphire::ScriptAPI;
using namespace Sapphire::World::AI;
using namespace Sapphire::Entity;

class BnpcTricksterImp : public BattleNpcScript
{
public:
  BnpcTricksterImp() : BattleNpcScript( 21 ) {}

  void onInit( Sapphire::Entity::BNpc& bnpc ) override
  {
    // Initialize GambitTimeLinePack
    auto gambitPack = std::make_shared< Sapphire::World::AI::GambitTimeLinePack >( -1 );

    // Add timeline actions
    gambitPack->addTimeLine(
            Sapphire::World::AI::make_TopHateTargetCondition(),
            Action::make_Action( bnpc.getAsChara(), 142, 0 ),
            0// Execute at timeline tick 0
    );

    gambitPack->addTimeLine(
            Sapphire::World::AI::make_TopHateTargetCondition(),
            Action::make_Action( bnpc.getAsChara(), 142, 0 ),
            2// Execute at timeline tick 2
    );

    m_gambitPack = gambitPack;
    bnpc.setGambitPack( m_gambitPack );
  }

  void onAction( Sapphire::Entity::BNpc& bnpc, Sapphire::Entity::Chara& target, uint32_t actionId ) override
  {
    const uint32_t CASTING_ACTION_ID = 142;// Replace with actual casting action ID if different

    if( actionId == CASTING_ACTION_ID )
    {
      // Set the Immobile flag when casting starts
      bnpc.setFlag( static_cast< uint32_t >( Sapphire::Entity::BNpcFlag::Immobile ) );
    }
    else
    {
      // Remove the Immobile flag when casting ends or another action occurs
      bnpc.removeFlag( static_cast< uint32_t >( Sapphire::Entity::BNpcFlag::Immobile ) );
    }

    // Optionally, handle additional actions or logic here
  }

private:
  std::shared_ptr< Sapphire::World::AI::GambitPack > m_gambitPack;
};

EXPOSE_SCRIPT( BnpcTricksterImp );
