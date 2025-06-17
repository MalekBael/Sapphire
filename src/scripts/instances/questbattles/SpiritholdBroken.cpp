#include <ScriptObject.h>
#include <Territory/QuestBattle.h>
#include <Actor/Player.h>
#include <Actor/GameObject.h>
#include <Actor/BNpc.h>
#include <AI/GambitPack.h>
#include <AI/GambitTargetCondition.h>
#include <Action/Action.h>
#include <Manager/EventMgr.h>
#include <Service.h>
#include <fstream>
#include <nlohmann/json.hpp>

#include "ScriptLogger.h"

using namespace Sapphire;
using json = nlohmann::json;
using namespace Sapphire::ScriptAPI;

// Function to load the golem.json file
json loadGolemJson()
{
  const std::string jsonPath = "data/battlenpc/soulkin/animated/Golem.json";
  std::ifstream file(jsonPath);

  if (!file.is_open())
  {
    std::vector<std::string> alternativePaths = {
            "../data/battlenpc/soulkin/animated/Golem.json",
            "../../data/battlenpc/soulkin/animated/Golem.json",
            "./data/battlenpc/soulkin/animated/Golem.json"};

    for (const auto& path : alternativePaths)
    {
      file.open(path);
      if (file.is_open())
        break;
    }

    if (!file.is_open())
      return json();
  }

  json data;
  try
  {
    file >> data;
    return data;
  }
  catch (const std::exception&)
  {
    return json();
  }
}

class SpiritholdBroken : public Sapphire::ScriptAPI::QuestBattleScript
{
private:
  static constexpr auto INIT_POP_01 = 3869528;
  static constexpr auto LOC_POS_ACTOR0 = 3884691;
  static constexpr auto LOC_POS_ACTOR1 = 3884694;
  static constexpr auto LOC_ACTOR0 = 1003064;
  static constexpr auto BNPC_NAME_01 = 750;
  static constexpr auto DEF_ACTION_SAND_BREATH = 445;
  static constexpr auto DEF_BLIND = 15;
  static constexpr auto CUT_SCENE_01 = 69;
  static constexpr auto LOC_TALKSHAPE1 = 8;

  // Define the golem HP constant
  static constexpr uint32_t GOLEM_HP = 5000000; // Set your desired HP value

  // Battle talk message IDs
  static constexpr auto MASKEDMAGUS_BATTLETALK_01 = 152;
  static constexpr auto MASKEDMAGUS_BATTLETALK_02 = 153;
  static constexpr auto MASKEDMAGUS_BATTLETALK_03 = 154;
  
  // Battle talk flags
  bool m_sentBattleTalk1 = false;
  bool m_sentBattleTalk2 = false;
  bool m_sentBattleTalk3 = false;
  
  // Helper method to setup the golem's behavior using JSON configuration
  void setupGolemBehavior(Entity::BNpc& boss)
  {
    ScriptLogger::debug("SpiritholdBroken: Setting up gambit behavior for Clay Golem");

    json golemJson = loadGolemJson();
    if (golemJson.empty() || !golemJson.contains("golem") ||
        !golemJson["golem"].contains("ClayGolem"))
      return;

    auto& clayGolemData = golemJson["golem"]["ClayGolem"];
    if (!clayGolemData.contains("gambitPack"))
      return;

    int32_t loopCount = clayGolemData["gambitPack"].value("loopCount", -1);
    auto gambitPack = std::make_shared<Sapphire::World::AI::GambitTimeLinePack>(loopCount);

    if (clayGolemData["gambitPack"].contains("timeLines"))
    {
      ScriptLogger::debug("SpiritholdBroken: Setting up gambit timeline for Clay Golem");

      // Track the first action timing for debugging
      uint32_t firstActionTiming = 0;
      bool isFirstAction = true;

      for (const auto& timeline : clayGolemData["gambitPack"]["timeLines"])
      {
        // Create the condition
        std::shared_ptr<Sapphire::World::AI::GambitTargetCondition> condition;
        if (timeline.contains("condition") && timeline["condition"] == "TopHateTarget")
          condition = Sapphire::World::AI::make_TopHateTargetCondition();
        else
          condition = Sapphire::World::AI::make_TopHateTargetCondition(); // Default

        // Get action data from timeline
        uint32_t actionId = timeline.value("actionId", 0);
        uint32_t actionParam = timeline.value("actionParam", 0);
        uint32_t timing = timeline.value("timing", 0);

        // Track first action timing for debugging
        if (isFirstAction)
        {
          firstActionTiming = timing;
          isFirstAction = false;
          ScriptLogger::debug("SpiritholdBroken: First action (ID: {}) has timing set to {} seconds",
                           actionId, timing);
        }

        ScriptLogger::debug("SpiritholdBroken: Adding action ID {} to timeline with timing {} seconds",
                         actionId, timing);

        // Create the action
        auto action = Sapphire::World::Action::make_Action(boss.getAsChara(), actionId, actionParam);

        // Apply cast type if specified
        if (timeline.contains("castType"))
        {
          auto castType = static_cast<Common::CastType>(timeline["castType"]);
          action->setCastType(castType);
          ScriptLogger::debug("SpiritholdBroken: Setting castType {} for action {}",
                           static_cast<uint8_t>(castType), actionId);
        }

        // Add timeline entry
        gambitPack->addTimeLine(condition, action, timing);
      }
    }

    // Apply the gambit pack to the boss
    boss.setGambitPack(gambitPack);

    // Start the timeline to initialize m_startTimeMs
    if (auto timelinePack = gambitPack->getAsTimeLine())
    {
      ScriptLogger::debug("SpiritholdBroken: Starting gambit timeline with startTime at {}",
                       Common::Util::getTimeMs());
      timelinePack->start();
    }
  }

public:
  SpiritholdBroken() : Sapphire::ScriptAPI::QuestBattleScript(15)
  { }

  void onInit(QuestBattle& instance) override
  {
    // Add all your EObj definitions as before
    instance.addEObj("Entrance", 2000182, 5021407, 5021409, 5, { 623.000000f, 23.872311f, 94.505638f }, 1.000000f, 0.000000f, 0);
    instance.addEObj("Exit", 2000139, 0, 5022207, 4, { 623.000000f, 23.656260f, 61.956181f }, 1.000000f, 0.000000f, 0);
    instance.addEObj("unknown_0", 2000412, 1824621, 3671487, 4, { 41.519749f, 31.998119f, -243.976501f }, 0.991789f, 0.000048f, 0);
    instance.addEObj("unknown_1", 2000411, 2263931, 3671491, 4, { 12.924340f, 24.612749f, -195.452896f }, 0.991789f, 0.000048f, 0);
    instance.addEObj("unknown_2", 2000410, 2263930, 3671493, 4, { 59.367592f, 22.826191f, -188.589005f }, 1.000000f, 0.000000f, 0);
    instance.addEObj("Entrypoint", 2001278, 0, 3915596, 4, { 57.856419f, 22.763201f, -178.983093f }, 1.000000f, -1.095531f, 0);
    instance.addEObj("Entrypoint_1", 2001278, 0, 3915613, 4, { 18.769621f, 24.894320f, -193.620804f }, 1.000000f, 0.000000f, 0);
    instance.addEObj("Entrypoint_2", 2001278, 0, 3915615, 4, { 37.416389f, 32.348339f, -239.311096f }, 1.000000f, -0.652703f, 0);
    instance.addEObj("Entrance", 2001710, 4138604, 4138602, 5, { -78.116699f, 3.688327f, -39.524689f }, 1.000000f, 0.000000f, 0);
    instance.addEObj("Entrance_1", 2001710, 4138624, 4138622, 5, { 194.548599f, 13.313300f, 46.466358f }, 1.000000f, 0.000000f, 0);
    instance.addEObj("unknown_3", 2004356, 0, 4876927, 4, { 659.983582f, 21.846149f, 70.454659f }, 0.991760f, 0.000048f, 0);
    instance.addEObj("Entrance_2", 2003673, 0, 4621850, 4, { -362.233490f, 1.217181f, 456.390015f }, 1.000000f, 0.000000f, 0);
    instance.addEObj("Entrance_3", 2005123, 0, 5584700, 4, { 614.997070f, 22.127399f, 83.073334f }, 1.000000f, 0.000000f, 0);
    instance.addEObj("Hulkinghogcarcass", 2002893, 0, 4499354, 4, { 467.401306f, 2.212447f, 154.063599f }, 0.991760f, 0.433121f, 0);
    instance.addEObj("unknown_4", 2004660, 0, 5099944, 4, { 170.997192f, 16.160179f, 59.052219f }, 1.000000f, 0.000000f, 0);
    instance.addEObj("Destination", 2005065, 0, 5607524, 4, { -225.261093f, 13.156470f, 56.919300f }, 0.991760f, 0.000048f, 0);
    instance.addEObj("Destination_1", 2005066, 0, 5607529, 4, { 109.694603f, 24.166731f, 174.784805f }, 1.000000f, 0.000000f, 0);
    instance.addEObj("Destination_2", 2005067, 0, 5607536, 4, { -338.428711f, -0.067347f, 453.487488f }, 1.000000f, 0.000000f, 0);
    instance.addEObj("Destination_3", 2005068, 0, 5607547, 4, { 604.605286f, 24.055861f, 105.129700f }, 1.000000f, 0.000000f, 0);
    instance.addEObj("unknown_5", 2006322, 0, 5925629, 4, { 177.604706f, 14.795990f, 55.369701f }, 1.000000f, 0.000000f, 0);
    instance.addEObj("unknown_6", 2006313, 0, 5926377, 4, { 185.645096f, 14.040300f, 50.517399f }, 0.991760f, 0.000048f, 0);
    instance.addEObj("Overloadedwain", 2002403, 0, 4306922, 4, { 151.561203f, 18.129620f, -127.292297f }, 1.000000f, -1.500076f, 0);
    instance.addEObj("unknown_7", 2002407, 0, 4306923, 4, { 151.300507f, 17.340410f, -127.860397f }, 1.000000f, 0.553560f, 0);
    instance.addEObj("unknown_8", 2000726, 0, 3776233, 4, { 42.628422f, 4.222789f, 48.897511f }, 1.000000f, 0.000000f, 0);
    instance.addEObj("Destination_4", 2000731, 0, 3777873, 4, { 87.151657f, 8.191072f, -74.345261f }, 1.000000f, 0.000000f, 0);
    instance.addEObj("unknown_9", 2001651, 0, 4101229, 4, { -254.810394f, 15.060610f, 4.379272f }, 0.991760f, 0.000048f, 0);
    instance.addEObj("unknown_10", 2001652, 0, 4101230, 4, { -255.790100f, 15.141020f, 3.643538f }, 1.000000f, 0.000000f, 0);
    instance.addEObj("unknown_11", 2001653, 0, 4101231, 4, { -255.597107f, 14.962130f, 2.632248f }, 1.000000f, 0.000000f, 0);
    instance.addEObj("unknown_12", 2001654, 0, 4101232, 4, { -255.676697f, 15.242050f, 4.474526f }, 1.000000f, 0.000000f, 0);
    instance.addEObj("Destination_5", 2001359, 0, 3951346, 4, { -274.069214f, 22.196470f, -174.910995f }, 1.000000f, 0.000000f, 0);
    instance.addEObj("Destination_6", 2001360, 0, 3951353, 4, { -240.589096f, 7.755447f, -92.976707f }, 1.000000f, 0.000000f, 0);
    instance.addEObj("Destination_7", 2001361, 0, 3951356, 4, { -206.772797f, 8.545876f, -81.818642f }, 1.000000f, 0.000000f, 0);
    instance.addEObj("Destination_8", 2001362, 0, 3951357, 4, { -146.683807f, 7.302510f, -44.484200f }, 1.000000f, 0.000000f, 0);
    instance.addEObj("Destination_9", 2001363, 0, 3951361, 4, { -122.599602f, 4.438381f, -50.170559f }, 1.000000f, 0.000000f, 0);
    instance.addEObj("Destination_10", 2001364, 0, 3951364, 4, { -175.622894f, 6.913669f, 4.099701f }, 1.000000f, 0.000000f, 0);
    instance.addEObj("Destination_11", 2001366, 0, 3951383, 4, { -105.779198f, 2.364898f, -57.375599f }, 1.000000f, 0.000000f, 0);
    instance.addEObj("Destination_12", 2001367, 0, 3951398, 4, { -209.033203f, 9.881900f, 111.748001f }, 1.000000f, 0.000000f, 0);
    instance.addEObj("unknown_13", 2004686, 0, 5021388, 4, { -335.539886f, 20.119431f, 616.384216f }, 1.000000f, 0.000000f, 0);
    instance.addEObj("???", 2004113, 0, 4722129, 4, { 446.995392f, 1.130214f, -110.054604f }, 1.000000f, 0.000000f, 0);
    instance.addEObj("???_1", 2004124, 0, 4722813, 4, { -185.961700f, -0.060072f, 444.449493f }, 0.991760f, 0.000048f, 0);
    instance.addEObj("Destination_13", 2004282, 0, 4757269, 4, { 261.585297f, 15.432220f, -125.871803f }, 0.991760f, 0.000048f, 0);
    instance.addEObj("Destination_14", 2004581, 0, 5101542, 4, { 354.733185f, 5.612688f, -30.140120f }, 0.991760f, 0.000048f, 0);
    instance.addEObj("Destination_15", 2004582, 0, 5101543, 4, { 343.292694f, 2.437127f, 8.920555f }, 0.991760f, 0.000048f, 0);
    instance.addEObj("Destination_16", 2004845, 0, 5101545, 4, { 362.531494f, 1.671068f, -18.454340f }, 0.991760f, 0.000048f, 0);
    instance.addEObj("Suspiciousthicket", 2004846, 0, 5101564, 4, { -181.445099f, 8.093226f, 101.304604f }, 0.991760f, 0.000048f, 0);

    // Pre-spawn the Golem during initialization WITH THE CORRECT HP
    uint32_t baseId = 549;
    ScriptLogger::debug("SpiritholdBroken: Creating Clay Golem in onInit with HP %u", GOLEM_HP);

    json golemJson = loadGolemJson();
    if (!golemJson.empty() && golemJson.contains("golem") &&
        golemJson["golem"].contains("ClayGolem"))
    {
      auto& clayGolemData = golemJson["golem"]["ClayGolem"];
      if (clayGolemData.contains("baseId"))
        baseId = clayGolemData["baseId"];
    }

    // Create golem with the proper HP during init
    auto boss = instance.createBNpcFromLayoutId(INIT_POP_01, GOLEM_HP, Common::BNpcType::Enemy);

    if (boss)
    {
      // Log actual HP after creation
      ScriptLogger::debug("SpiritholdBroken: Actual Golem HP after creation: %u/%u", 
                         boss->getHp(), boss->getMaxHp());
                       
      if (!golemJson.empty() && golemJson.contains("golem") &&
          golemJson["golem"].contains("ClayGolem"))
      {
        auto& clayGolemData = golemJson["golem"]["ClayGolem"];
        if (clayGolemData.contains("nameId"))
          boss->setBNpcNameId(clayGolemData["nameId"]);
        else
          boss->setBNpcNameId(30);

        bool isRanged = clayGolemData.value("isRanged", false);
        float attackRange = clayGolemData.value("attackRange", 3.0f);

        boss->setIsRanged(isRanged);
        boss->setAttackRange(attackRange);
      }

      // Note: DON'T setup gambits here, as they should be initialized in onDutyCommence
      // DON'T start combat state here either
    }
    else
    {
      ScriptLogger::error("SpiritholdBroken: Failed to create Clay Golem in onInit");
    }
  }

  enum vars
  {
    SUCCESS_CALLED,
    DIALOGUE_SHOWN,
  };

  void onPlayerSetup(Sapphire::QuestBattle& instance, Entity::Player& player) override
  {
    // Change rotation to 0.f to face the boss (180 degrees from original 3.f)
    player.setRot( 4.71f );
    player.setPos({259.981f, 24.000f, 191.259f});
  }

  void onDutyCommence(QuestBattle& instance, Entity::Player& player) override
  {
    // Don't create a new golem - get the one we already created
    auto boss = instance.getActiveBNpcByLayoutId(INIT_POP_01);
    
    if (!boss)
    {
      ScriptLogger::error("SpiritholdBroken: Could not find Clay Golem in onDutyCommence");
      return;
    }
    
    // Log the HP at duty commence
    ScriptLogger::debug("SpiritholdBroken: Clay Golem HP at duty start: %u/%u", 
                     boss->getHp(), boss->getMaxHp());
    
    // Now setup the gambit behavior
    setupGolemBehavior(*boss);
    
    // Setup combat state
    boss->hateListClear();
    boss->hateListAdd(player.getAsPlayer(), 10000);
    boss->setState(Entity::BNpcState::Combat);
    boss->changeTarget(player.getId());
    boss->setStance(Common::Stance::Active);
    boss->aggro(player.getAsPlayer());
  }

  void onUpdate(QuestBattle& instance, uint64_t tickCount) override
  {
    auto boss = instance.getActiveBNpcByLayoutId(INIT_POP_01);
    auto successCalled = instance.getDirectorVar(SUCCESS_CALLED);
    auto dialogueShown = instance.getDirectorVar(DIALOGUE_SHOWN);
    auto pPlayer = instance.getPlayerPtr();

    uint32_t bossHpPercent = 0;
    if (boss)
    {
      bossHpPercent = boss->getHpPercent();
      
      // Ensure boss keeps targeting the player
      if (pPlayer && boss->getTargetId() != pPlayer->getId())
      {
        boss->changeTarget(pPlayer->getId());
      }
      
      // Process the gambit pack (crucial for abilities)
      boss->processGambits(tickCount);
      
      // Battle talk at 80% HP
      if (bossHpPercent <= 80 && !m_sentBattleTalk1 && pPlayer)
      {
        playerMgr().sendBattleTalk(*pPlayer,
                                  MASKEDMAGUS_BATTLETALK_01,
                                  instance.getDirectorId(),
                                  0x4,
                                  LOC_ACTOR0,
                                  boss->getId(),
                                  5000);
        m_sentBattleTalk1 = true;
      }
      
      // Battle talk at 60% HP
      if (bossHpPercent <= 60 && !m_sentBattleTalk2 && pPlayer)
      {
        playerMgr().sendBattleTalk(*pPlayer,
                                  MASKEDMAGUS_BATTLETALK_02,
                                  instance.getDirectorId(),
                                  0x4,
                                  LOC_ACTOR0,
                                  boss->getId(),
                                  5000);
        m_sentBattleTalk2 = true;
      }
      
      // Battle talk at 20% HP
      if (bossHpPercent <= 20 && !m_sentBattleTalk3 && pPlayer)
      {
        playerMgr().sendBattleTalk(*pPlayer,
                                  MASKEDMAGUS_BATTLETALK_03,
                                  instance.getDirectorId(),
                                  0x4,
                                  LOC_ACTOR0,
                                  boss->getId(),
                                  5000);
        m_sentBattleTalk3 = true;
      }
    }

    if (pPlayer && !pPlayer->isAlive())
    {
      instance.fail();
      return;
    }

    if (instance.getCountEnemyBNpc() == 0 && successCalled == 0)
    {
      instance.setDirectorVar(SUCCESS_CALLED, 1);
      instance.success();
      return;
    }
  }

  void onEnterTerritory(QuestBattle& instance, Entity::Player& player, uint32_t eventId, uint16_t param1,
                       uint16_t param2) override
  {
    auto& eventMgr = Common::Service<Sapphire::World::Manager::EventMgr>::ref();

    eventMgr.playScene(player, instance.getDirectorId(), 1,
                      NO_DEFAULT_CAMERA | CONDITION_CUTSCENE | SILENT_ENTER_TERRI_ENV |
                      HIDE_HOTBAR | SILENT_ENTER_TERRI_BGM | SILENT_ENTER_TERRI_SE |
                      DISABLE_STEALTH | 0x00100000 | LOCK_HUD | LOCK_HOTBAR |
                      DISABLE_CANCEL_EMOTE, [&](Entity::Player& player, const Event::SceneResult& result)
                      {
                        player.setOnEnterEventDone(true);
                      });
  }

  void onDutyComplete(QuestBattle& instance, Entity::Player& player) override
  {
    auto idx = player.getQuestIndex(instance.getQuestId());
    if (idx == -1)
      return;
    auto& quest = player.getQuestByIndex(idx);
    quest.setSeq(4);
  }
};

EXPOSE_SCRIPT(SpiritholdBroken);