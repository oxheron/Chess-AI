#include "disp.h"

// std
#include <iostream>

int main(void)
{
    Disp disp;
    Board board;
    board.load_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    auto moves = board.generate_moves(Color::WHITE);

    // for (auto move : moves)
    // {
    //     std::cout << (int) move.end_pos << ", ";
    // }

    board.print_pieces();
    board.move({11, 27});
    std::cout << "after move" << std::endl;
    board.print_pieces();
}