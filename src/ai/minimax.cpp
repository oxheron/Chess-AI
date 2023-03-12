#include "minimax.h"

#include <iostream>

size_t search(int depth, Board& board)
{
    if (depth == 0) return 0;
    size_t pos_c = 0;
    auto piece_moves = board.generate_moves((Color) board.get_turn());

    for (auto x : piece_moves)
    {
        auto moves = extract_moves(x);
        std::cout << "here" << std::endl;
        // Crash on this line or entering moves loop
        if (depth == 1) pos_c += moves.size();
        std::cout << "her2e";
        for (auto m : moves)
        {
            std::cout << "starting ";
            board.move(m);
            std::cout << "after moves ";
            pos_c += search(depth - 1, board);
            std::cout << "after search "; 
            board.unmove(m);
            std::cout << "after unmove ";
        }
    }

    return pos_c;
}