#pragma once

#include "stateMachine.h"

// states
State *create_attack_enemy_state();
State *create_move_to_enemy_state();
State *create_flee_from_enemy_state();
State *create_follow_ally_state();
State *create_patrol_state(float patrol_dist);
State *create_heal_state(float heal_amount);
State *create_heal_closest_ally_state(float heal_amount);
State *create_nop_state();

// transitions
StateTransition *create_enemy_available_transition(float dist);
StateTransition *create_ally_available_transition(float dist);
StateTransition *create_enemy_reachable_transition();
StateTransition *create_heal_available_transition();
StateTransition *create_hitpoints_less_than_transition(float thres);
StateTransition *create_closest_ally_hitpoints_less_than_transition(float thres);
StateTransition *create_negate_transition(StateTransition *in);
StateTransition *create_and_transition(StateTransition *lhs, StateTransition *rhs);
StateTransition *create_or_transition(StateTransition *lhs, StateTransition *rhs);

