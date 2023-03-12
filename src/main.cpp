#include "disp.h"
#include "ai/minimax.h"

#include "bitset_handle.h"

// std
#include <iostream>

int main(int argc, char** argv)
{
    std::cout << "Chess AI" << std::endl;
    std::string fen = argc == 2 ? argv[1] : "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8";

    Disp disp;
    Board board;
    board.load_fen(fen);
    
    std::cout << search(2, board) << std::endl;
    std::cout << "done" << std::endl;
}