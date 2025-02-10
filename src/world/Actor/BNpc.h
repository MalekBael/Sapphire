// File: src/world/Actor/BNpc.h

#pragma once

#include <Common.h>
#include "Chara.h"
#include "Forwards.h"
#include "ForwardsZone.h"
#include "Npc.h"
#include <map>
#include <queue>
#include <set>


namespace Sapphire::Entity
{
    struct HateListEntry {
        uint32_t m_hateAmount;
        CharaPtr m_pChara;
    };

    enum class BNpcState
    {
        Idle,
        Combat,
        Retreat,
        Roaming,
        JustDied,
        Dead,
    };

    enum BNpcFlag
      {
        Immobile = 0x001,
        TurningDisabled = 0x002,
        Invincible = 0x004,
        StayAlive = 0x008,
        NoDeaggro = 0x010,
        Untargetable = 0x020,
        AutoAttackDisabled = 0x040,
        Invisible = 0x080,
        NoRoam = 0x100,

        Intermission = 0x77// for transition phases to ensure boss only moves/acts when scripted
      };

    const std::array<uint32_t, 50> BnpcBaseHp =
    {
        // ... initialization ...
    };

    /*!
    \class BNpc
    \brief Base class for all BNpcs

    */
    class BNpc : public Npc
    {
    public:
        BNpc();

        BNpc(uint32_t id, std::shared_ptr<Common::BNPCInstanceObject> pInfo, const Territory& zone);
        BNpc(uint32_t id, std::shared_ptr<Common::BNPCInstanceObject> pInfo, const Territory& zone, uint32_t hp, Common::BNpcType type);

        void setIsRanged( bool isRanged ) { m_isRanged = isRanged; }
        bool isRanged() const { return m_isRanged; }
        void setAttackRange( float range ) { m_attackRange = range; }
        float getAttackRange() const { return m_attackRange; }

        // Add public setter methods
       void setBNpcBaseId(uint32_t baseId)
       {
           m_bNpcBaseId = baseId;
       }

       void setBNpcNameId(uint32_t nameId)
       {
           m_bNpcNameId = nameId;
       }

        virtual ~BNpc() override;

        void init();

        void spawn(PlayerPtr pTarget) override;
        void despawn(PlayerPtr pTarget) override;

        uint16_t getModelChara() const;
        uint8_t getLevel() const override;

        uint32_t getBNpcBaseId() const;
        uint32_t getBNpcNameId() const;

        uint8_t getEnemyType() const;

        uint64_t getWeaponMain() const;
        uint64_t getWeaponSub() const;

        uint8_t getAggressionMode() const;

        uint32_t getTriggerOwnerId() const;
        void setTriggerOwnerId(uint32_t triggerOwnerId);

        float getNaviTargetReachedDistance() const;

        // Return true if it reached the position
        bool moveTo(const Common::FFXIVARR_POSITION3& pos);

        bool moveTo(const Entity::Chara& targetChara);

        void sendPositionUpdate();

        BNpcState getState() const;
        void setState(BNpcState state);

        void hateListClear();
        uint32_t hateListGetValue(const Sapphire::Entity::CharaPtr& pChara);
        uint32_t hateListGetHighestValue();
        CharaPtr hateListGetHighest();
        void hateListAdd(const CharaPtr& pChara, int32_t hateAmount);
        void hateListAddDelayed(const CharaPtr& pChara, int32_t hateAmount);
        void hateListUpdate(const CharaPtr& pChara, int32_t hateAmount);
        void hateListRemove(const CharaPtr& pChara);
        bool hateListHasActor(const CharaPtr& pChara);

        void aggro(const CharaPtr& pChara);
        void deaggro(const CharaPtr& pChara);

        void update(uint64_t tickCount) override;
        void onTick() override;

        void onActionHostile(CharaPtr pSource) override;

        void onDeath() override;

        void autoAttack(CharaPtr pTarget) override;

        uint32_t getTimeOfDeath() const;
        void setTimeOfDeath(uint32_t timeOfDeath);

        void restHp();

        void checkAggro();

        void setOwner(const CharaPtr& m_pChara);

        void setLevelId(uint32_t levelId);
        uint32_t getLevelId() const;
        uint32_t getBoundInstanceId() const;

        bool hasFlag(uint32_t flag) const;
        void setFlag(uint32_t flags);
        void removeFlag( uint32_t flag );
        void clearFlags();

        void calculateStats() override;

        uint32_t getRank() const;

        Common::BNpcType getBNpcType() const;

        uint32_t getLayoutId() const;

        void processGambits(uint64_t tickCount);

        uint32_t getLastRoamTargetReachedTime() const;
        void setLastRoamTargetReachedTime(uint32_t time);

        std::shared_ptr<Common::BNPCInstanceObject> getInstanceObjectInfo() const;

        void setRoamTargetReached(bool reached);
        bool isRoamTargetReached() const;

        void setRoamTargetPos(const Common::FFXIVARR_POSITION3& targetPos);

        const Common::FFXIVARR_POSITION3& getRoamTargetPos() const;
        const Common::FFXIVARR_POSITION3& getSpawnPos() const;

        // Add a public setter for m_pGambitPack
        void setGambitPack(std::shared_ptr<World::AI::GambitPack> gambitPack)
        {
            m_pGambitPack = gambitPack;
        }

    private:
        uint32_t m_bNpcBaseId;
        uint32_t m_bNpcNameId;
        uint64_t m_weaponMain;
        uint64_t m_weaponSub;
        uint8_t m_aggressionMode;
        uint8_t m_enemyType;
        uint8_t m_onlineStatus;
        uint8_t m_pose;
        uint16_t m_modelChara;
        uint32_t m_displayFlags;
        uint8_t m_level;
        uint32_t m_levelId;
        uint32_t m_rank;
        uint32_t m_boundInstanceId;
        uint32_t m_layoutId;
        uint32_t m_triggerOwnerId;

        uint32_t m_flags;

        Common::BNpcType m_bnpcType;

        float m_naviTargetReachedDistance;
        bool m_isRanged = false;
        float m_attackRange = 3.0f;



        std::shared_ptr<Common::BNPCInstanceObject> m_pInfo;

        uint32_t m_timeOfDeath;
        uint32_t m_lastRoamTargetReachedTime;
        bool m_roamTargetReached{ false };

        Common::FFXIVARR_POSITION3 m_spawnPos;
        Common::FFXIVARR_POSITION3 m_roamPos;

        BNpcState m_state;
        std::set<std::shared_ptr<HateListEntry>> m_hateList;

        uint64_t m_naviLastUpdate;
        std::vector<Common::FFXIVARR_POSITION3> m_naviLastPath;
        uint8_t m_naviPathStep;
        Common::FFXIVARR_POSITION3 m_naviTarget;

        CharaPtr m_pOwner;
        World::AI::GambitPackPtr m_pGambitPack;

        std::shared_ptr<World::AI::Fsm::StateMachine> m_fsm;
    };

}// namespace Sapphire::Entity
