#include "disp.h"
#include "ai/minimax.h"

#include "bitset_handle.h"

// std
#include <iostream>

int main(int argc, char** argv)
{
    std::cout << "Chess AI" << std::endl;
    std::string fen = argc == 2 ? argv[1] : "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    Disp disp;
    Board board;
    board.load_fen(fen);

    auto moves = board.generate_moves((Color) board.get_turn());

    Move dothisback;

    for (auto x : moves)
    {
        auto split_moves = extract_moves(x);

        std::cout << "start: " << (int) split_moves[0].start_pos << " end: " << (int) split_moves[0].end_pos << std::endl;
        board.move(split_moves[0]);
        dothisback = split_moves[0];
        break;
    }

    print_bitset(board.all_pieces.to_ullong());

    board.unmove(dothisback);

    print_bitset(board.all_pieces.to_ullong());
}