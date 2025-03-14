#include <Action/CommonAction.h>
#include <Actor/Player.h>
#include <Script/NativeScriptApi.h>
#include <ScriptObject.h>
#include <StatusEffect/StatusEffect.h>

using namespace Sapphire;
using namespace Sapphire::World::Action;

/*
Tooltip:
  Weaponskill cast/recast time, spell cast/recast time, and auto-attack delay are reduced.
  Duration: 20s (needs checking)
*/

class StatusEffectHaste : public Sapphire::ScriptAPI::StatusEffectScript
{
public:
  StatusEffectHaste() : Sapphire::ScriptAPI::StatusEffectScript( 651 )
  {
  }

  void onApply( Entity::Chara& actor ) override
  {
    if( auto status = actor.getStatusEffectById( ActionStatus::Haste ) )
    {
      status->setModifier( Common::ParamModifier::AttackSpeed, +500 );
      status->setModifier( Common::ParamModifier::SkillSpeed, +500 );
      status->setModifier( Common::ParamModifier::SpellSpeed, +500 );
    }
  }
};

EXPOSE_SCRIPT( StatusEffectHaste );