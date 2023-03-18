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
    // board.move({3, 43, 0});

    // int total = 0;

    // auto movegend = board.generate_moves((Color) board.get_turn());
    // for (auto piecemove : movegend)
    // {
    //     std::cout << (int) piecemove.start_pos << std::endl;
    //     print_bitset(piecemove.bitboard);

    //     total += extract_moves(piecemove).size();
    // }

    // std::cout << total << std::endl;
    

    std::cout << search(2, board) << std::endl;
    std::cout << "done" << std::endl;
}

// Errors b2b3 should be 34
// d1d6 should be 28