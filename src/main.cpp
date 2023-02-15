#include "disp.h"
#include "ai/minimax.h"

// std
#include <iostream>

int main(int argc, char** argv)
{
    std::cout << "Chess AI" << std::endl;
    std::string fen = argc == 2 ? argv[1] : "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    Disp disp;
    Board board;
    board.load_fen(fen);
    std::cout << search(2, board) << std::endl;
    std::cout << "susus" << std::endl;
}