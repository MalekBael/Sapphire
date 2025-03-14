#include <Action/CommonAction.h>
#include <Actor/Player.h>
#include <Script/NativeScriptApi.h>
#include <ScriptObject.h>
#include <StatusEffect/StatusEffect.h>

using namespace Sapphire;
using namespace Sapphire::World::Action;

class StatusEffectDisembowel : public Sapphire::ScriptAPI::StatusEffectScript
{
public:
  StatusEffectDisembowel() : Sapphire::ScriptAPI::StatusEffectScript( 121 )
  {
  }

  void onApply( Entity::Chara& actor ) override
  {
    // Placeholder implementation
  }
};

EXPOSE_SCRIPT( StatusEffectDisembowel );