#include "minimax.h"

#include <iostream>
#include <fstream>

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
                // size_t save = board.all_pieces.to_ullong();
                // size_t save2;
                print_square(m.start_pos);
                print_square(m.end_pos);
                std::cout << ": ";
                board.move(m);
                std::cout << "after move" << std::endl;
                // save2 = board.all_pieces.to_ullong();
                pos_c += search(depth - 1, board);
                board.unmove(m);
                // if (board.all_pieces.to_ullong() != save) 
                // {
                //     std::cout << "move: " << (int) m.start_pos << ", " << (int) m.end_pos << ", " << (int) m.special << std::endl;
                //     print_bitset(x.bitboard);
                //     std::cout << "before move" << std::endl;
                //     print_bitset(save);
                //     std::cout << "after move" << std::endl;
                //     print_bitset(save2);
                //     std::cout << "after unmove" << std::endl;
                //     print_bitset(board.all_pieces.to_ullong());
                // }
            }
        }
    }

    // Miscalculation when promoting
    // Move gen is wrong (I think pin calc)

    std::cout << pos_c << std::endl;

    return pos_c;
}