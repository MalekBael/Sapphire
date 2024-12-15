#include <Action/CommonAction.h>
#include <Actor/Player.h>
#include <Script/NativeScriptApi.h>
#include <ScriptObject.h>
#include <StatusEffect/StatusEffect.h>

using namespace Sapphire;
using namespace Sapphire::World::Action;

// Tooltip: Increases damage dealt by 20%

class StatusEffectRagingStrikes : public Sapphire::ScriptAPI::StatusEffectScript
{
public:
  StatusEffectRagingStrikes() : Sapphire::ScriptAPI::StatusEffectScript( 125 )
  {
  }

  void onApply( Entity::Chara& actor ) override
  {
    if( auto status = actor.getStatusEffectById( Raging_Strikes ); status )
      status->setModifier( Common::ParamModifier::DamageDealtPercent, +20 );
  }
};

EXPOSE_SCRIPT( StatusEffectRagingStrikes );