#pragma once
#include <vector>
#include <flecs.h>
#include "ecsTypes.h"

namespace dmaps
{
  void gen_player_approach_map(flecs::world &ecs, std::vector<float> &map);
  void gen_player_flee_map(flecs::world &ecs, std::vector<float> &map);
  void gen_hive_pack_map(flecs::world &ecs, std::vector<float> &map);
  void gen_exploration_map(flecs::world &ecs, std::vector<float> &map);
  void gen_ally_map(flecs::world &ecs, std::vector<float> &map, flecs::entity e, const Team &t);
  void gen_mage_approach_map(flecs::world &ecs, std::vector<float> &map);
};

