#pragma once

#include "game.h"

// SDL
#include <SDL2/SDL.h>
// std

struct Disp
{
    std::list<Piece> white;
    std::list<Piece> black;

    Disp();

    void updatePieces(std::list<Piece> white, std::list<Piece> black);
    void display();
};