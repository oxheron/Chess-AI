#include "disp.h"

// std
#include <iostream>

int main(void)
{
    Disp disp;
    Board board;
    board.load_fen("8/8/8/4R3/4B3/8/8/8 w KQkq - 0 1");
    auto moves = board.generate_moves(Color::WHITE);

    for (auto move : moves)
    {
        std::cout << (int) move.end_pos << ", ";
    }
}