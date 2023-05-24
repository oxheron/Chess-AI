#include "minimax.h"

#include <iostream>
#include <fstream>
#include <stdexcept>

std::ofstream loggger("out.text", std::ios_base::ate);

// size_t search(int depth, Board& board)
// {
//     if (depth == 0) return 0;
//     size_t pos_c = 0;
//     auto piece_moves = board.generate_moves((Color) board.get_turn());

//     for (auto x : piece_moves)
//     {
//         auto moves = extract_moves(x);
//         if (depth == 1) pos_c += moves.size();
//         for (auto m : moves)
//         {
//             board.move(m);
//             pos_c += search(depth - 1, board);
//             board.unmove(m);
//         }
//     }

//     return pos_c;
// }

void print_square(int sq)
{
    std::cout << (char) ('a' + sq % 8) << (sq / 8) + 1;
}

size_t search(int depth, Board& board)
{
    size_t pos_c = 0;
    auto piece_moves = board.generate_moves((Color) board.get_turn());

    // SOMETHINGS WRONG WITH CAPTURES
    // PIECES BEING CHANGED SOMEHOW
    // std::cout << piece_moves.size() << std::endl;

    for (auto x : piece_moves)
    {
        auto moves = extract_moves(x);
        if (depth == 1) pos_c += moves.size();
        else 
        {
            for (auto m : moves)
            {
                size_t save = board.all_pieces.to_ullong();
                auto pins = board.pins;
                board.update_history();
                short history = board.game_history.top();
                board.game_history.pop();
                size_t save2;
                if (depth == 4) print_square(m.start_pos);
                if (depth == 4) print_square(m.end_pos);
                if (depth == 4) std::cout << ", " << (int) m.special << ": "; 
                board.move(m);
                save2 = board.all_pieces.to_ullong();
                pos_c += search(depth - 1, board);
                board.unmove(m);
                board.update_history();
                if (board.all_pieces.to_ullong() != save || pins != board.pins || history != board.game_history.top()) 
                {
                    std::cout << "before move" << std::endl;
                    print_bitset(save);
                    std::cout << "after move" << std::endl;
                    print_bitset(save2);
                    std::cout << "after unmove" << std::endl;
                    print_bitset(board.all_pieces.to_ullong());
                    throw std::runtime_error("move/unmove error");
                }
                board.game_history.pop();
            }
        }
    }

    // Miscalculation when promoting
    // Move gen is wrong (I think pin calc)

    if (depth == 3) std::cout << pos_c << std::endl;

    return pos_c;
}