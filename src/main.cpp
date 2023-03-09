#include "disp.h"
#include "ai/minimax.h"

#include "bitset_handle.h"

// std
#include <iostream>

int main(int argc, char** argv)
{
    // Pins dont work 
    std::cout << "Chess AI" << std::endl;
    // std::string fen = argc == 2 ? argv[1] : "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    std::string fen = "K7/8/8/3R4/8/8/6b1/k7 w - - 0 1";
    Disp disp;
    Board board;
    board.load_fen(fen);
    auto moves = board.generate_moves((Color) board.get_turn());

    for (auto x : moves)
    {
        print_bitset(x.bitboard);
    }
}