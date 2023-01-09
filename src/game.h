#pragma once

#include "piece.h"

// std
#include <array>
#include <string>
#include <list> 
#include <vector>

// Holds data for a move, returned in the move piece function
struct Move
{
    char start_pos;
    char end_pos;
};

// The chess board, has functions about making moves and loading fen strings
class Board
{
private:
    // Board for checking moves
    std::array<Piece, 64> board;

    // Lists of pieces 
    std::list<Piece> white;
    std::list<Piece> black;

    // Stuff about castling
    bool white_KC = false;
    bool white_QC = false;
    bool black_KC = false;
    bool black_QC = false;

    // Info about the games progress
    bool turn;
    size_t moves;

public:
    // Initializes the board with empty squares
    Board();

    // Sets up the board with a fen string, so it can have any postition
    void load_fen(std::string fen);

    // A debugging function until I have a working screen
    void print_pieces();

    // The most important function, generates all of the possible moves for the ai to choose from
    std::vector<Move> generate_moves(Color color);

    // Change the position of a piece, without rules (execpt a piece cannot occupy the same space as a piece)
    void change_pos();

    // A legal move
    void move();

private:
    // Generate all legal rook moves from a piece, puts result in moves
    void rook_moves(Piece piece, std::vector<Move>& moves);
    void bishop_moves(Piece piece, std::vector<Move>& moves);
    void queen_moves(Piece piece, std::vector<Move>& moves);
    void king_moves(Piece piece, std::vector<Move>& moves);
};
