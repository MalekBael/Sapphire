#include <Action/CommonAction.h>
#include <Actor/Player.h>
#include <Script/NativeScriptApi.h>
#include <ScriptObject.h>
#include <StatusEffect/StatusEffect.h>

using namespace Sapphire;
using namespace Sapphire::World::Action;

/*
Tooltip:
  Increases damage dealt by 10% and damage suffered by 25%.
  Duration: 20s
*/

class StatusEffectBloodForBlood : public Sapphire::ScriptAPI::StatusEffectScript
{
public:
  StatusEffectBloodForBlood() : Sapphire::ScriptAPI::StatusEffectScript( 117 )
  {
  }

  void onApply( Entity::Chara& actor ) override
  {
    if( auto status = actor.getStatusEffectById( ActionStatus::Blood_For_Blood ) )
    {
      status->setModifier( Common::ParamModifier::DamageDealtPercent, +10 );
      status->setModifier( Common::ParamModifier::DamageTakenPercent, +25 );
    }
  }
};

EXPOSE_SCRIPT( StatusEffectBloodForBlood );