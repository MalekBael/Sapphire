#include <Action/CommonAction.h>
#include <Actor/Player.h>
#include <Script/NativeScriptApi.h>
#include <ScriptObject.h>
#include <StatusEffect/StatusEffect.h>

using namespace Sapphire;
using namespace Sapphire::World::Action;

/*
Tooltip: Reduces damage receved by 20%.
*/

class StatusEffectRampart : public Sapphire::ScriptAPI::StatusEffectScript
{
public:
  StatusEffectRampart() : Sapphire::ScriptAPI::StatusEffectScript( 71 )
  {
  }

  void onApply( Entity::Chara& actor ) override
  {
    if( auto status = actor.getStatusEffectById( Status_Rampart ); status )
      status->setModifier( Common::ParamModifier::DamageTakenPercent, -20 );
  }
};

EXPOSE_SCRIPT( StatusEffectRampart );