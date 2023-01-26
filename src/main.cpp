#include "disp.h"

// std
#include <iostream>

int main(int argc, char** argv)
{
    std::string fen = argc == 2 ? argv[1] : "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    Disp disp;
    Board board;
    board.load_fen(fen);
    auto moves = board.generate_moves((Color) board.get_turn());
    std::cout << moves.size() << std::endl;
}