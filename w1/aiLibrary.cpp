#include "aiLibrary.h"
#include <flecs.h>
#include "ecsTypes.h"
#include "raylib.h"
#include <cfloat>
#include <cmath>
#include <algorithm>

class AttackEnemyState : public State
{
public:
  void enter() const override {}
  void exit() const override {}
  void act(float/* dt*/, flecs::world &/*ecs*/, flecs::entity /*entity*/) const override {}
};

template<typename T>
T sqr(T a){ return a*a; }

template<typename T, typename U>
static float dist_sq(const T &lhs, const U &rhs) { return float(sqr(lhs.x - rhs.x) + sqr(lhs.y - rhs.y)); }

template<typename T, typename U>
static float dist(const T &lhs, const U &rhs) { return sqrtf(dist_sq(lhs, rhs)); }

template<typename T, typename U>
static int move_towards(const T &from, const U &to)
{
  int deltaX = to.x - from.x;
  int deltaY = to.y - from.y;
  if (abs(deltaX) > abs(deltaY))
    return deltaX > 0 ? EA_MOVE_RIGHT : EA_MOVE_LEFT;
  return deltaY < 0 ? EA_MOVE_UP : EA_MOVE_DOWN;
}

static int inverse_move(int move)
{
  return move == EA_MOVE_LEFT ? EA_MOVE_RIGHT :
         move == EA_MOVE_RIGHT ? EA_MOVE_LEFT :
         move == EA_MOVE_UP ? EA_MOVE_DOWN :
         move == EA_MOVE_DOWN ? EA_MOVE_UP : move;
}


template<typename Callable>
static void on_closest_enemy_pos(flecs::world &ecs, flecs::entity entity, Callable c)
{
  static auto enemiesQuery = ecs.query<const Position, const Team>();
  entity.set([&](const Position &pos, const Team &t, Action &a)
  {
    flecs::entity closestEnemy;
    float closestDist = FLT_MAX;
    Position closestPos;
    enemiesQuery.each([&](flecs::entity enemy, const Position &epos, const Team &et)
    {
      if (t.team == et.team)
        return;
      float curDist = dist(epos, pos);
      if (curDist < closestDist)
      {
        closestDist = curDist;
        closestPos = epos;
        closestEnemy = enemy;
      }
    });
    if (ecs.is_valid(closestEnemy))
      c(a, pos, closestPos);
  });
}

class MoveToEnemyState : public State
{
public:
  void enter() const override {}
  void exit() const override {}
  void act(float/* dt*/, flecs::world &ecs, flecs::entity entity) const override
  {
    on_closest_enemy_pos(ecs, entity, [&](Action &a, const Position &pos, const Position &enemy_pos)
    {
      a.action = move_towards(pos, enemy_pos);
    });
  }
};

class FleeFromEnemyState : public State
{
public:
  FleeFromEnemyState() {}
  void enter() const override {}
  void exit() const override {}
  void act(float/* dt*/, flecs::world &ecs, flecs::entity entity) const override
  {
    on_closest_enemy_pos(ecs, entity, [&](Action &a, const Position &pos, const Position &enemy_pos)
    {
      a.action = inverse_move(move_towards(pos, enemy_pos));
    });
  }
};

template<typename Callable>
static void on_closest_ally_pos(flecs::world &ecs, flecs::entity entity, Callable c)
{
  static auto alliesQuery = ecs.query<const Position, const Team>();
  entity.set([&](const Position &pos, const Team &t, Action &a)
  {
    flecs::entity closestAlly;
    float closestDist = FLT_MAX;
    Position closestPos;
    alliesQuery.each([&](flecs::entity ally, const Position &epos, const Team &et)
    {
      if (t.team != et.team || entity == ally)
        return;
      float curDist = dist(epos, pos);
      if (curDist < closestDist)
      {
        closestDist = curDist;
        closestPos = epos;
        closestAlly = ally;
      }
    });
    if (ecs.is_valid(closestAlly))
      c(a, pos, closestPos);
  });
}

class FollowAllyState : public State
{
public:
  void enter() const override {}
  void exit() const override {}
  void act(float /* dt*/, flecs::world &ecs, flecs::entity entity) const override {
    on_closest_ally_pos(ecs, entity, [&](Action &a, const Position &pos, const Position &ally_pos)
    {
      a.action = move_towards(pos, ally_pos);
    });
  }
};

class PatrolState : public State
{
  float patrolDist;
public:
  PatrolState(float dist) : patrolDist(dist) {}
  void enter() const override {}
  void exit() const override {}
  void act(float/* dt*/, flecs::world &ecs, flecs::entity entity) const override
  {
    entity.set([&](const Position &pos, const PatrolPos &ppos, Action &a)
    {
      if (dist(pos, ppos) > patrolDist)
        a.action = move_towards(pos, ppos); // do a recovery walk
      else
      {
        // do a random walk
        a.action = GetRandomValue(EA_MOVE_START, EA_MOVE_END - 1);
      }
    });
  }
};

class HealState : public State
{
private:
  float healAmount;
public:
  HealState(float _healAmount) : healAmount(_healAmount) {}
  void enter() const override {}
  void exit() const override {}
  void act(float /* dt*/, flecs::world &ecs, flecs::entity entity) const override {
    entity.set([&](Hitpoints &hp) {
      hp.hitpoints = std::clamp(hp.hitpoints + healAmount, 0.0f, 100.0f);
    });
  }
};

template<typename Callable>
static void on_closest_ally(flecs::world &ecs, flecs::entity entity, Callable c)
{
  static auto alliesQuery = ecs.query<const Position, const Team>();
  entity.set([&](const Position &pos, const Team &t, Action &a)
  {
    flecs::entity closestAlly;
    float closestDist = FLT_MAX;
    Position closestPos;
    alliesQuery.each([&](flecs::entity ally, const Position &epos, const Team &et)
    {
      if (t.team != et.team || entity == ally)
        return;
      float curDist = dist(epos, pos);
      if (curDist < closestDist)
      {
        closestDist = curDist;
        closestPos = epos;
        closestAlly = ally;
      }
    });
    if (ecs.is_valid(closestAlly))
      c(a, closestAlly);
  });
}

class HealClosestAlly : public State
{
private:
  float healAmount;
public:
  HealClosestAlly(float _healAmount) : healAmount(_healAmount) {}
  void enter() const override {}
  void exit() const override {}
  void act(float /* dt*/, flecs::world &ecs, flecs::entity entity) const override {
    on_closest_ally(ecs, entity, [&](Action &a, flecs::entity closestAlly)
    {
      closestAlly.set([&](Hitpoints &hp) {
        hp.hitpoints += healAmount;
      });
    });

    entity.set([](HealCooldown &cd)
    {
      printf("Set cur to 0\n");
      cd.cur = 0;
    });
  }
};

class NopState : public State
{
public:
  void enter() const override {}
  void exit() const override {}
  void act(float/* dt*/, flecs::world &ecs, flecs::entity entity) const override {}
};

class EnemyAvailableTransition : public StateTransition
{
  float triggerDist;
public:
  EnemyAvailableTransition(float in_dist) : triggerDist(in_dist) {}
  bool isAvailable(flecs::world &ecs, flecs::entity entity) const override
  {
    static auto enemiesQuery = ecs.query<const Position, const Team>();
    bool enemiesFound = false;
    entity.get([&](const Position &pos, const Team &t)
    {
      enemiesQuery.each([&](flecs::entity enemy, const Position &epos, const Team &et)
      {
        if (t.team == et.team)
          return;
        float curDist = dist(epos, pos);
        enemiesFound |= curDist <= triggerDist;
      });
    });
    return enemiesFound;
  }
};

class AllyAvailableTransition : public StateTransition
{
  float triggerDist;
public:
  AllyAvailableTransition(float in_dist) : triggerDist(in_dist) {}
  bool isAvailable(flecs::world &ecs, flecs::entity entity) const override
  {
    static auto alliesQuery = ecs.query<const Position, const Team>();
    bool alliesFound = false;
    entity.get([&](const Position &pos, const Team &t)
    {
      alliesQuery.each([&](flecs::entity ally, const Position &epos, const Team &et)
      {
        if (t.team != et.team || entity == ally)
          return;
        float curDist = dist(epos, pos);
        alliesFound |= curDist <= triggerDist;
      });
    });
    return alliesFound;
  }
};

class HealAvailableTransition : public StateTransition
{
public:
  bool isAvailable(flecs::world &/*ecs*/, flecs::entity entity) const override
  {
    bool isAvailable = false;
    entity.get([&](const HealCooldown &cd)
    {
      isAvailable |= cd.cur == cd.cooldown;
    });
    return isAvailable;
  }
};

class HitpointsLessThanTransition : public StateTransition
{
  float threshold;
public:
  HitpointsLessThanTransition(float in_thres) : threshold(in_thres) {}
  bool isAvailable(flecs::world &ecs, flecs::entity entity) const override
  {
    bool hitpointsThresholdReached = false;
    entity.get([&](const Hitpoints &hp)
    {
      hitpointsThresholdReached |= hp.hitpoints < threshold;
    });
    return hitpointsThresholdReached;
  }
};

class ClosestAllyHitpointsLessThanTransition : public StateTransition
{
  float threshold;
public:
  ClosestAllyHitpointsLessThanTransition(float thres) : threshold(thres) {}
  bool isAvailable(flecs::world &ecs, flecs::entity entity) const override
  {
    bool hitpointsThresholdReached = false;
    on_closest_ally(ecs, entity, [&](Action &a, flecs::entity closestAlly)
    {
      closestAlly.get([&](const Hitpoints &hp) {
        hitpointsThresholdReached |= hp.hitpoints < threshold;
      });
    });
    return hitpointsThresholdReached;
  }
};

class EnemyReachableTransition : public StateTransition
{
public:
  bool isAvailable(flecs::world &ecs, flecs::entity entity) const override
  {
    return false;
  }
};

class NegateTransition : public StateTransition
{
  const StateTransition *transition; // we own it
public:
  NegateTransition(const StateTransition *in_trans) : transition(in_trans) {}
  ~NegateTransition() override { delete transition; }

  bool isAvailable(flecs::world &ecs, flecs::entity entity) const override
  {
    return !transition->isAvailable(ecs, entity);
  }
};

class AndTransition : public StateTransition
{
  const StateTransition *lhs; // we own it
  const StateTransition *rhs; // we own it
public:
  AndTransition(const StateTransition *in_lhs, const StateTransition *in_rhs) : lhs(in_lhs), rhs(in_rhs) {}
  ~AndTransition() override
  {
    delete lhs;
    delete rhs;
  }

  bool isAvailable(flecs::world &ecs, flecs::entity entity) const override
  {
    return lhs->isAvailable(ecs, entity) && rhs->isAvailable(ecs, entity);
  }
};

class OrTransition : public StateTransition
{
  const StateTransition *lhs; // we own it
  const StateTransition *rhs; // we own it
public:
  OrTransition(const StateTransition *in_lhs, const StateTransition *in_rhs) : lhs(in_lhs), rhs(in_rhs) {}
  ~OrTransition() override
  {
    delete lhs;
    delete rhs;
  }

  bool isAvailable(flecs::world &ecs, flecs::entity entity) const override
  {
    return lhs->isAvailable(ecs, entity) || rhs->isAvailable(ecs, entity);
  }
};


// states
State *create_attack_enemy_state()
{
  return new AttackEnemyState();
}
State *create_move_to_enemy_state()
{
  return new MoveToEnemyState();
}

State *create_flee_from_enemy_state()
{
  return new FleeFromEnemyState();
}

State *create_follow_ally_state()
{
  return new FollowAllyState();
}

State *create_patrol_state(float patrol_dist)
{
  return new PatrolState(patrol_dist);
}

State *create_heal_state(float heal_amount)
{
  return new HealState(heal_amount);
}

State *create_heal_closest_ally_state(float heal_amount)
{
  return new HealClosestAlly(heal_amount);
}

State *create_nop_state()
{
  return new NopState();
}

// transitions
StateTransition *create_enemy_available_transition(float dist)
{
  return new EnemyAvailableTransition(dist);
}

StateTransition *create_ally_available_transition(float dist)
{
  return new AllyAvailableTransition(dist);
}

StateTransition *create_enemy_reachable_transition()
{
  return new EnemyReachableTransition();
}

StateTransition *create_heal_available_transition()
{
  return new HealAvailableTransition();
}

StateTransition *create_hitpoints_less_than_transition(float thres)
{
  return new HitpointsLessThanTransition(thres);
}

StateTransition *create_closest_ally_hitpoints_less_than_transition(float thres)
{
  return new ClosestAllyHitpointsLessThanTransition(thres);
}

StateTransition *create_negate_transition(StateTransition *in)
{
  return new NegateTransition(in);
}
StateTransition *create_and_transition(StateTransition *lhs, StateTransition *rhs)
{
  return new AndTransition(lhs, rhs);
}

StateTransition *create_or_transition(StateTransition *lhs, StateTransition *rhs)
{
  return new OrTransition(lhs, rhs);
}

