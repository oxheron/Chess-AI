#pragma once

#include "piece.h"
#include "bitset_handle.h"

// std
#include <array>
#include <string>
#include <list> 
#include <memory>
#include <stack>
#include <bitset>
#include <vector>
#include <unordered_map>

// Holds data for a bitboard of moves for a piece, returned in the generate moves function
struct Moves
{
    char start_pos;
    uint64_t bitboard;

    // Special data, such as rook square for castling or what piece to promote to
    char special;

    inline Moves()
    {
        start_pos = 0;
        bitboard  = 0;
        special = 0;
    }

    inline Moves(char start_pos, uint64_t bitboard)
    {
        this->start_pos = start_pos;
        this->bitboard = bitboard;
        this->special = 0;
    }

    inline Moves(Piece start_pos, uint64_t bitboard)
    {
        this->start_pos = start_pos.square;
        this->bitboard = bitboard;
        this->special = 0;
    }

    inline Moves(char start_pos, uint64_t bitboard, char promote)
    {
        this->start_pos = start_pos;
        this->bitboard = bitboard;
        this->special = promote;
    }
    
    inline Moves(Piece start_pos, uint64_t bitboard, char promote)
    {
        this->start_pos = start_pos.square;
        this->bitboard = bitboard;
        this->special = promote;
    }
};

// Holds data for a move, the input of move and unmove in the board class
struct Move
{
    char start_pos;
    char end_pos;
    // Holds special data such as promotion or castling
    char special;
};

inline void extract_moves(Moves moves, std::vector<Move>& output)
{
    // Use recursion of this function
    // Find forward set bit, push back a move with the forward set bit and the rest of the piece info, call this function again with the same std::vector and a move where the bitboard is anded with a ~ 1 << fsb

    if (moves.bitboard == 0) return;    
    int fsb = bit_scan_fw(moves.bitboard);
    output.emplace_back(Move{moves.start_pos, (char) fsb, moves.special});
    extract_moves(Moves{moves.start_pos, moves.bitboard & ~(uint64_t) 1 << fsb, moves.special}, output);
    return;
}

// Turns a bitboard of grouped moves for a piece into individual moves
inline std::vector<Move> extract_moves(Moves moves)
{
    std::vector<Move> output;

    // Use recursion and passing a std::vector forward
    extract_moves(moves, output);

    return output;
}

// The chess board, has functions about making moves and loading fen strings
class Board
{
private:
public:
    // Board for checking moves
    std::array<std::shared_ptr<Piece>, 64> board;
    
    // Bitboard for fast generation
    std::bitset<64> white_pieces;
    std::bitset<64> black_pieces;
    std::bitset<64> all_pieces;
public:
    // Lists of pieces
    std::list<std::shared_ptr<Piece>> white;
    std::list<std::shared_ptr<Piece>> black;

    // Current game flags
    bool white_KC = false;
    bool white_QC = false;
    bool black_KC = false;
    bool black_QC = true;
    char capture_type = 0;
    char ep_file = 0;
    char fifty_mover = 0;

    // Info about the games progress
    bool turn = 1;
    size_t moves;

    // Info about what king squares are not attacked and are legal moves (for both colors), at least for now done by move gen without pins 
    uint64_t attacked;

    // Is the opposing king in check? 
    bool in_check;
    // Could it castle safely (not through check)
    bool KC_safe;
    bool QC_safe;
    // Squares to block or take to stop check
    uint64_t stop_check;

    // What pieces are pinned (square and 1 of the 8 directions, for next turns color)
    std::array<char, 64> pins; 

    // Info about past captures
    // Bits 0-3 are castle state 0 is white_KC, 3 is black_QC
    // Bits 4-6 are EP file bits
    // Bits 7-9 are capture bits, if there was a capture, the piece type is in 7-9 
    // Also stores info about EP(see below)
    // 000 = EMPTY (no EP)
    // 001 = KING (never used)
    // 010 = QUEEN
    // 011 = ROOK
    // 100 = BISHOP
    // 101 = KNIGHT
    // 110 = PAWN
    // 111 = EMPTY (ep)
    // You cant have a capture and a move that enables EP at the same time, so if it isn't 111 bits 4-6 are disregarderd
    // Bits 10-15 are 50 move rule
    std::stack<short> game_history;

public:
    // Initializes the board with empty squares
    Board();

    // Sets up the board with a fen string, so it can have any postition
    void load_fen(std::string fen);

    // A debugging function until I have a working screen
    void print_pieces();

    // The most important function, generates all of the possible moves for the ai to choose from
    std::vector<Moves> generate_moves(Color color);

    // A move
    void move(Move move);

    // Do a move backwards
    void unmove(Move move);

    // Get turn for generating moves
    bool get_turn() { return this->turn; }
private:
    // Generate all legal rook moves from a piece, puts result in moves
    uint64_t sliding_moves(Piece piece);
    uint64_t knight_moves(Piece piece);
    uint64_t pawn_moves(Piece piece);

    // Update the game_history stack
    void update_history();
    // Undo the game history stack (for unmoving)
    void undo_history();

    // Calculate pins
    void calc_pins(Color color, char king_sq);
    // Calculate attacks
    uint64_t all_attacks(Color color, uint64_t no_king);
    // Do all updates that has to be done for a move
    void update_board(Color color);
};
