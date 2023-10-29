#pragma once

#include <flecs.h>

constexpr float tile_size = 512.f;

void init_roguelike(flecs::world &ecs);
void init_dungeon(flecs::world &ecs, char *tiles, size_t w, size_t h);
void update_exploration_data(flecs::world &ecs);
void update_mage_ally_maps(flecs::world &ecs);
void process_turn(flecs::world &ecs);
void print_stats(flecs::world &ecs);
