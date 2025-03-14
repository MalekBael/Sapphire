#include <Action/Action.h>
#include <Action/CommonAction.h>
#include <Actor/Chara.h>
#include <Script/NativeScriptApi.h>
#include <ScriptObject.h>
#include <StatusEffect/StatusEffect.h>
#include <memory>

using namespace Sapphire;
using namespace Sapphire::World::Action;

/*
Tooltip:
  Chance to parry is increased
*/

class StatusEffectKeenFlurry : public Sapphire::ScriptAPI::StatusEffectScript
{
public:
  StatusEffectKeenFlurry() : Sapphire::ScriptAPI::StatusEffectScript( 114 )
  {
  }

  void onApply( Entity::Chara& actor ) override
  {
    if( auto status = actor.getStatusEffectById( ActionStatus::Keen_Flurry ) )
    {
      status->setModifier( Common::ParamModifier::ParryPercent, +40 );
    }
  }
};

EXPOSE_SCRIPT( StatusEffectKeenFlurry );