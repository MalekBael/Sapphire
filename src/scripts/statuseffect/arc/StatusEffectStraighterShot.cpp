#include <Script/NativeScriptApi.h>
#include <ScriptObject.h>
#include <Actor/Player.h>
#include <Action/CommonAction.h>
#include <StatusEffect/StatusEffect.h>

using namespace Sapphire;
using namespace Sapphire::World::Action;

// Tooltip: Next Straight Shot will result in a critical hit.

class StatusEffectStraighterShot : public Sapphire::ScriptAPI::StatusEffectScript
{
public:
  StatusEffectStraighterShot() : Sapphire::ScriptAPI::StatusEffectScript( 122 )
  {
  }

  void onApply( Entity::Chara& actor ) override
  {
    if( auto status = actor.getStatusEffectById( StraighterShot ); status )
      status->setModifier( Common::ParamModifier::CriticalHitPowerPercent, +100 );
  }
};

EXPOSE_SCRIPT( StatusEffectStraighterShot );