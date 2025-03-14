#include <Action/Action.h>
#include <Action/CommonAction.h>
#include <Actor/Chara.h>
#include <Script/NativeScriptApi.h>
#include <ScriptObject.h>
#include <memory>
#include <random>

using namespace Sapphire;
using namespace Sapphire::World::Action;
using namespace Sapphire::Entity;

class ActionHeavyShot : public Sapphire::ScriptAPI::ActionScript
{
public:
  ActionHeavyShot() : Sapphire::ScriptAPI::ActionScript( 97 ), gen( std::random_device{}() ), distr( 1, 100 )
  {
  }

  static constexpr auto Potency = 150;

  void onExecute( Sapphire::World::Action::Action& action ) override
  {
    auto pSource = action.getSourceChara();
    auto pTarget = action.getHitChara();

    if( !pSource->isPlayer() )
      return;

    auto dmg = action.calcDamage( Potency );
    action.getActionResultBuilder()->damage( pSource, pTarget, dmg.first, dmg.second );

    if( dmg.first > 0 )
    {
      pTarget->onActionHostile( pSource );
    }

    if( distr( gen ) <= 20 )
    {
      uint32_t duration = 10000;

      pSource->replaceSingleStatusEffectById( StraighterShot );
      action.getActionResultBuilder()->applyStatusEffectSelf( StraighterShot, duration, 0 );
    }
  }

private:
  std::mt19937 gen;
  std::uniform_int_distribution<> distr;
};

EXPOSE_SCRIPT( ActionHeavyShot );