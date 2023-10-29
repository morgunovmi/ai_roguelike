#include "dijkstraMapGen.h"
#include "ecsTypes.h"
#include "dungeonUtils.h"
#include "aiUtils.h"

template<typename Callable>
static void query_dungeon_data(flecs::world &ecs, Callable c)
{
  static auto dungeonDataQuery = ecs.query<const DungeonData>();

  dungeonDataQuery.each(c);
}

template<typename Callable>
static void query_characters_positions(flecs::world &ecs, Callable c)
{
  static auto characterPositionQuery = ecs.query<const Position, const Team>();

  characterPositionQuery.each(c);
}

constexpr float invalid_tile_value = 1e5f;

static void init_tiles(std::vector<float> &map, const DungeonData &dd)
{
  map.resize(dd.width * dd.height);
  for (float &v : map)
    v = invalid_tile_value;
}

// scan version, could be implemented as Dijkstra version as well
static void process_dmap(std::vector<float> &map, const DungeonData &dd)
{
  bool done = false;
  auto getMapAt = [&](size_t x, size_t y, float def)
  {
    if (x < dd.width && y < dd.width && dd.tiles[y * dd.width + x] == dungeon::floor)
      return map[y * dd.width + x];
    return def;
  };
  auto getMinNei = [&](size_t x, size_t y)
  {
    float val = map[y * dd.width + x];
    val = std::min(val, getMapAt(x - 1, y + 0, val));
    val = std::min(val, getMapAt(x + 1, y + 0, val));
    val = std::min(val, getMapAt(x + 0, y - 1, val));
    val = std::min(val, getMapAt(x + 0, y + 1, val));
    return val;
  };
  while (!done)
  {
    done = true;
    for (size_t y = 0; y < dd.height; ++y)
      for (size_t x = 0; x < dd.width; ++x)
      {
        const size_t i = y * dd.width + x;
        if (dd.tiles[i] != dungeon::floor)
          continue;
        const float myVal = getMapAt(x, y, invalid_tile_value);
        const float minVal = getMinNei(x, y);
        if (minVal < myVal - 1.f)
        {
          map[i] = minVal + 1.f;
          done = false;
        }
      }
  }
}

void dmaps::gen_player_approach_map(flecs::world &ecs, std::vector<float> &map)
{
  query_dungeon_data(ecs, [&](const DungeonData &dd)
  {
    init_tiles(map, dd);
    query_characters_positions(ecs, [&](const Position &pos, const Team &t)
    {
      if (t.team == 0) // player team hardcode
        map[pos.y * dd.width + pos.x] = 0.f;
    });
    process_dmap(map, dd);
  });
}

void dmaps::gen_player_flee_map(flecs::world &ecs, std::vector<float> &map)
{
  gen_player_approach_map(ecs, map);
  for (float &v : map)
    if (v < invalid_tile_value)
      v *= -1.2f;
  query_dungeon_data(ecs, [&](const DungeonData &dd)
  {
    process_dmap(map, dd);
  });
}

void dmaps::gen_hive_pack_map(flecs::world &ecs, std::vector<float> &map)
{
  static auto hiveQuery = ecs.query<const Position, const Hive>();
  query_dungeon_data(ecs, [&](const DungeonData &dd)
  {
    init_tiles(map, dd);
    hiveQuery.each([&](const Position &pos, const Hive &)
    {
      map[pos.y * dd.width + pos.x] = 0.f;
    });
    process_dmap(map, dd);
  });
}

void dmaps::gen_exploration_map(flecs::world &ecs, std::vector<float> &map)
{
  static auto explorationQuery = ecs.query<const ExplorationData>();
  query_dungeon_data(ecs, [&](const DungeonData &dd) {
    init_tiles(map, dd);
    explorationQuery.each([&](const ExplorationData &data) {
      for (int y = 0; y < data.height; ++y)
        for (int x = 0; x < data.width; ++x)
        {
          if (dd.tiles[data.width * y + x] != dungeon::wall && !data.data[data.width * y + x])
            map[dd.width * y + x] = 0.f;
        }
    });
    process_dmap(map, dd);
  });
}

void dmaps::gen_ally_map(flecs::world &ecs, std::vector<float> &map, flecs::entity e, const Team &t)
{
  query_dungeon_data(ecs, [&](const DungeonData &dd)
  {
    init_tiles(map, dd);
    query_characters_positions(ecs, [&](flecs::entity ee, const Position &epos, const Team &et) {
      if (et.team == t.team && ee != e)
      {
        map[epos.y * dd.width + epos.x] = 0.f;
      }
    });
    process_dmap(map, dd);
  });
}

void dmaps::gen_mage_approach_map(flecs::world &ecs, std::vector<float> &map)
{
  query_dungeon_data(ecs, [&](const DungeonData &dd)
  {
    init_tiles(map, dd);
    query_characters_positions(ecs, [&](const Position &pos, const Team &t) {
      if (t.team == 0) // player team hardcode
      {
        const int dist = 4;
        for (int i = -dist; i <= dist; ++i)
        {
          for (int j = -dist; j <= dist; ++j)
          {
            if (i == -dist || i == dist || j == -dist || j == dist)
            {
              auto moveCount = 0;
              auto curPos = pos;
              auto prevPos = curPos;
              while (curPos.x >= 0 && curPos.x < dd.width
                     && curPos.y >= 0 && curPos.y < dd.height
                     && !(curPos.x == pos.x + i && curPos.y == pos.y + j)
                     && dd.tiles[dd.width * curPos.y + curPos.x] != dungeon::wall
                     && moveCount <= 4)
              {
                prevPos = curPos;
                curPos = move_pos(curPos, move_towards(curPos, Position{pos.x + i, pos.y + j}));
                ++moveCount;
              }
              if (dd.tiles[dd.width * curPos.y + curPos.x] != dungeon::wall
                  || !(curPos.x >= 0 && curPos.x < dd.width)
                  || !(curPos.y >= 0 && curPos.y < dd.height))
                curPos = prevPos;

              map[dd.width * curPos.y + curPos.x] = 0.f;
            }
          }
        }
      }
    });
    process_dmap(map, dd);
  });
}

