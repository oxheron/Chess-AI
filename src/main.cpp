#include "disp.h"

// std
#include <iostream>

int main(void)
{
    Disp disp;
    Board board;
    board.load_fen("8/8/2k1b1R1/4K3/8/8/8/8 w - - 0 1");
    auto moves = board.generate_moves(Color::WHITE);

    std::cout << moves.size() << std::endl;
}