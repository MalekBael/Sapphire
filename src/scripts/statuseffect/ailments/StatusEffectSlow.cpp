#include <Action/CommonAction.h>
#include <Actor/Player.h>
#include <Script/NativeScriptApi.h>
#include <ScriptObject.h>
#include <StatusEffect/StatusEffect.h>

using namespace Sapphire;
using namespace Sapphire::World::Action;

/*
Tooltip:
  Weaponskill cast time and recast time, spell cast time and recast time, and auto-attack delay are increased.
  Duration: 20s (needs checking)
*/

class StatusEffectSlow : public Sapphire::ScriptAPI::StatusEffectScript
{
public:
  StatusEffectSlow() : Sapphire::ScriptAPI::StatusEffectScript( 561 )
  {
  }

  void onApply( Entity::Chara& actor ) override
  {
    if( auto status = actor.getStatusEffectById( ActionStatus::Slow ) )
    {
      status->setModifier( Common::ParamModifier::AttackSpeed, -1500 );
      status->setModifier( Common::ParamModifier::SkillSpeed, -1500 );
      status->setModifier( Common::ParamModifier::SpellSpeed, -1500 );
    }
  }
};

EXPOSE_SCRIPT( StatusEffectSlow );