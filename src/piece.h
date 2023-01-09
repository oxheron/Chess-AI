#pragma once

enum class PieceType
{
    EMPTY, KING, QUEEN, ROOK, BISHOP, KNIGHT, PAWN
};

enum class Color
{
    BLACK, WHITE, NONE
};

struct Piece
{
    PieceType piece_t;
    Color color;
    char square;
    bool alive;

    Piece()
        : piece_t(PieceType::EMPTY), color(Color::NONE), square(0), alive(true)
    {}
    
    Piece(PieceType piece_t, Color color, char square)
        : piece_t(piece_t), color(color), square(square), alive(false)
    {}

    PieceType get_piece() { return alive ? piece_t : PieceType::EMPTY; }
    Color get_color() { return alive ? color : Color::NONE; }

};