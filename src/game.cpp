#include "game.h"

// std
#include <iostream>
#include <unordered_map>
#include <cctype>

// Convert characters to piecetypes
std::unordered_map<char, PieceType> ctop_t({
    {'k', PieceType::KING},
    {'q', PieceType::QUEEN},
    {'r', PieceType::ROOK},
    {'b', PieceType::BISHOP},
    {'n', PieceType::KNIGHT},
    {'p', PieceType::PAWN}
});

// Map of opposite colors for generating moves
std::unordered_map<Color, Color> opposite_color({
    {Color::WHITE, Color::BLACK},
    {Color::BLACK, Color::WHITE},
    {Color::NONE, Color::NONE}
});

// Calculate the number of squares to the edge for the starting square and each direction
std::array<std::array<char, 8>, 64> num_sq_to_edge;

// List of sliding piece offsets based on direction
std::array<char, 8> sliding_offsets({
    -1, 1, 8, -8, 7, -9, 9, -7
});

// Generate a map of how far a square is from the edge
// Left, Right, Top, Bottom, northwest, southwest, northeast, southeast
void gen_num_sq_to_edge()
{
    for (size_t sq = 0; sq < 64; sq++)
    {
        char x = sq % 8;
        char y = sq / 8;
        
        char numNorth = 8 - y;
        char numSouth = y + 1;
        char numWest = x + 1;
        char numEast = 8 - x;

        char sq_idx = y * 8 + x;
        num_sq_to_edge[sq_idx] = 
        {
            numWest, numEast, numNorth, numSouth, 
            std::min(numWest, numNorth), std::min(numWest, numSouth), std::min(numEast, numNorth), std::min(numEast, numSouth)
        };
    }
}

// Initalizes board with empty squares
Board::Board()
{
    gen_num_sq_to_edge();
    for (auto& p : board) p = Piece();
}

// Load a fen string into the board
void Board::load_fen(std::string fen)
{
    size_t split_idx = fen.find(' ', 0);
    std::string turn = fen.substr(split_idx + 1);
    fen.erase(split_idx);

    for (size_t i = 0; i < 8; i++)
    {
        size_t slash_split = fen.find('/', 0);
        std::string next;
        if (slash_split != std::string::npos)
        {
            next = fen.substr(slash_split + 1);
            fen.erase(slash_split);
        }

        for (size_t j = 0, fen_idx = 0; j < 8; j++, fen_idx++)
        {
            if (ctop_t.contains(std::tolower(fen[fen_idx]))) 
            {
                if (std::isupper(fen[fen_idx])) 
                {
                    // Square isnt being saved somehow
                    Piece p(ctop_t[std::tolower(fen[fen_idx])], Color::WHITE, i * 8 + j);

                    board[i * 8 + j] = p;
                    white.push_back(p);
                }
                else
                {
                    Piece p(ctop_t[fen[fen_idx]], Color::BLACK, i * 8 + j);
                    board[i * 8 + j] = p;
                    black.push_back(p);
                }
            }
            else
            {
                j += fen[fen_idx] - 49;
            }
        }
        fen = next;
    }

    split_idx = turn.find(' ', 0);
    std::string castle = turn.substr(split_idx + 1);
    turn.erase(split_idx);

    if (turn[0] == 'w') this->turn = 1;
    else this->turn = 0;

    split_idx = castle.find(' ', 0);
    castle.erase(split_idx);

    std::unordered_map<char, bool*> castle_set({
        {'K', &white_KC},
        {'Q', &white_QC},
        {'k', &black_KC},
        {'q', &black_QC},
    });

    for (auto [c, varp] : castle_set) if (castle.find(c) != std::string::npos) *varp = true;

    std::cout << "White KC: " << white_KC << "\nWhite QC: " << white_QC  << "\nBlack KC: " << black_KC << "\nBlack QC: " << black_QC << std::endl;   
}

std::vector<Move> Board::generate_moves(Color color)
{
    std::vector<Move> return_v;
    for (auto piece : (color == Color::WHITE ? white : black))
    {
        if (piece.piece_t == PieceType::BISHOP) bishop_moves(piece, return_v);
        else if (piece.piece_t == PieceType::ROOK) rook_moves(piece, return_v);
        else if (piece.piece_t == PieceType::QUEEN) bishop_moves(piece, return_v);
        else if (piece.piece_t == PieceType::KING) king_moves(piece, return_v);
    }
    return return_v;
}

void Board::print_pieces()
{
    for (auto p : white)
    {
        std::cout << (size_t) p.piece_t << ", " << (int) p.square << std::endl;
    }

    for (auto p : black)
    {
        std::cout << (size_t) p.piece_t << ", " << (int) p.square << std::endl;
    }
}

void Board::move()
{

}

// Calculate all possible rook moves for a piece
void Board::rook_moves(Piece piece, std::vector<Move>& moves)
{
    for (size_t dir = 0; dir < 4; dir++)
    {
        for (size_t i = 1; i < num_sq_to_edge[piece.square][dir]; i++)
        {
            char t_square = piece.square + sliding_offsets[dir] * i; 
            Color t_color = board[t_square].color;
            if (t_color == piece.color) break;
            moves.push_back({piece.square, t_square});
            if (t_color == opposite_color[piece.color]) break;
        }
    }
}

// Calculate all possible bishop moves for a piece
void Board::bishop_moves(Piece piece, std::vector<Move>& moves)
{
    for (size_t dir = 0; dir < 4; dir++)
    {
        for (size_t i = 1; i < num_sq_to_edge[piece.square][dir + 4]; i++)
        {
            char t_square = piece.square + sliding_offsets[dir + 4] * i; 
            Color t_color = board[t_square].color;
            if (t_color == piece.color) break;
            moves.push_back({piece.square, t_square});
            if (t_color == opposite_color[piece.color]) break;
        }
    }
}

// Combine bishop + rook moves, generate all queen moves
void Board::queen_moves(Piece piece, std::vector<Move>& moves)
{
    rook_moves(piece, moves);
    bishop_moves(piece, moves);
}

// Calculate all valid king moves
void Board::king_moves(Piece piece, std::vector<Move>& moves)
{
    for (size_t dir = 0; dir < 8; dir++)
    {
        char t_square = piece.square + sliding_offsets[dir]; 
        Color t_color = board[t_square].color;
        if (t_color == piece.color) break;
        moves.push_back({piece.square, t_square});
        if (t_color == opposite_color[piece.color]) break;
    }
}