#include <Action/CommonAction.h>
#include <Actor/Player.h>
#include <Script/NativeScriptApi.h>
#include <ScriptObject.h>
#include <StatusEffect/StatusEffect.h>

using namespace Sapphire;
using namespace Sapphire::World::Action;

// Tooltip: Increases damage dealt by 15%

class StatusEffectHeavyThrust : public Sapphire::ScriptAPI::StatusEffectScript
{
public:
  StatusEffectHeavyThrust() : Sapphire::ScriptAPI::StatusEffectScript( 115 )
  {
  }

  void onApply( Entity::Chara& actor ) override
  {
    if( auto status = actor.getStatusEffectById( Heavy_Thrust ); status )
      status->setModifier( Common::ParamModifier::DamageDealtPercent, +15 );
  }
};

EXPOSE_SCRIPT( StatusEffectHeavyThrust );