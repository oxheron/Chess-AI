#pragma once

#include "game.h"

// The minimax search function
// For now, it will just count up every possible position from a given board
size_t search(int depth, Board& board);