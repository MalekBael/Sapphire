#pragma once

#include <cstdint>
#include <AI/TargetHelper.h>

#include <nlohmann/json.hpp>

namespace Sapphire::Encounter
{
  class Selector
  {
  private:
    std::string m_name;
    bool m_fillWithRandom{ true };
    uint32_t m_count{ 0 };
    std::vector< World::AI::TargetSelectFilterPtr > m_filters;
    World::AI::Snapshot m_snapshot;

  public:
    Selector(){}
    void createSnapshot( Entity::CharaPtr pSrc, const std::vector< uint32_t >& exclude = {} );
    const World::AI::Snapshot::Results& getResults();
    const World::AI::Snapshot::TargetIds& getTargetIds();
    void clearResults();
    void from_json( const nlohmann::json& json );
  };
};// namespace Sapphire::Encounter