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
// Left, right, north, south, nw, sw, ne, se
std::array<char, 8> sliding_offsets({
    -1, 1, 8, -8, 7, -9, 9, -7
});

// List of offsets for the knight, in x,y form
std::array<std::pair<char, char>, 8> knight_offsets({{
    {-1, 2},
    { 1, 2},
    { 2, 1},
    { 2,-1},
    { 1,-2},
    {-1,-2},
    {-2,-1},
    {-2, 1}
}});

// List of pawn offsets based on direction and color
std::array<std::array<char, 3>, 2> pawn_offset = {{
    {-8, -9, -7}, 
    { 8,  9,  7}
}};

// Pawn starting squares
std::array<char, 2> start_y = {
    6, 1
};

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
    for (char i = 0; i < board.size(); i++)
    {
        board[i] = std::make_shared<Piece>(PieceType::EMPTY, Color::NONE, i);
    }
}

// Load a fen string into the board
void Board::load_fen(std::string fen)
{
    size_t split_idx = fen.find(' ', 0);
    std::string turn = fen.substr(split_idx + 1);
    fen.erase(split_idx);

    for (char i = 7; i >= 0; i--)
    {
        size_t slash_split = fen.find('/', 0);
        std::string next;
        if (slash_split != std::string::npos)
        {
            next = fen.substr(slash_split + 1);
            fen.erase(slash_split);
        }

        for (char j = 7, fen_idx = 0; j >= 0; j--, fen_idx++)
        {
            if (ctop_t.contains(std::tolower(fen[fen_idx]))) 
            {
                if (std::isupper(fen[fen_idx])) 
                {
                    char sq = i * 8 + j;
                    board[sq]->piece_t = ctop_t[std::tolower(fen[fen_idx])];
                    board[sq]->square = sq;
                    board[sq]->color = Color::WHITE;
                    white.push_back(board[sq]);
                }
                else
                {
                    char sq = i * 8 + j;
                    board[sq]->piece_t = ctop_t[fen[fen_idx]];
                    board[sq]->square = sq;
                    board[sq]->color = Color::BLACK;
                    black.push_back(board[sq]);
                }
            }
            else
            {
                j -= fen[fen_idx] - 49;
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

    // Generate attack squares
    pinned_squares[0].fill(0);
    pinned_squares[1].fill(0);

    // Too lazy to put this in a function 
    // Find opposing king square
    char k_sq;
    for (auto p : this->turn ? white : black)
    {
        if (p->piece_t == PieceType::KING)
        {
            k_sq = p->square;
            break;
        }  
    }

    attacked[this->turn].fill(0);

    // Do attacking squares right ie the 8 squares around the king, the king, and castling squares 
    // Generate attack squares
    // Squares adjacent to king
    for (size_t i = 0; i < 8; i++)
    {
        std::cout << i << std::endl;
        attacked[this->turn][k_sq + sliding_offsets[i]] = calc_attacks((Color) !this->turn, k_sq + sliding_offsets[i]);
    }
    
    in_check = calc_attacks((Color) !this->turn, k_sq);

    // Hardcoded castle squares
    attacked[this->turn][k_sq - 2] = calc_attacks((Color) !this->turn, k_sq - 2);
    attacked[this->turn][k_sq + 2] = calc_attacks((Color) !this->turn, k_sq + 2);

    // If 2 pieces, king must move
    // If 1 piece, based on the piece, generate a list of squares that stops the check (such as taking the checking piece or blocking)
    // Go through all pieces
    // Check if they attack the king (copy of attack_sq loop)
    // If they do, put king in check, and while raycasting (only for sliding pieces) add squares to list
    // Add piece square to list of stop check moves

    if (in_check)
    {
        for (auto piece : this->turn ? black : white)
        {
            if (piece->piece_t == PieceType::PAWN) 
            {
                // Calculate a capture to the left (if its not on the leftmost file)
                if (piece->square % 8 != 0)
                {
                    char target_sq = piece->square + pawn_offset[this->turn][1];
                    if (target_sq == k_sq) 
                    {
                        if (stop_check.size() != 0) stop_check.clear();
                        else stop_check.push_back(piece->square);
                    }
                }

                // Calculate a capture to the right (if its not on the rightmost file)
                if (piece->square % 8 != 7)
                {
                    char target_sq = piece->square + pawn_offset[this->turn][2];
                    if (target_sq == k_sq) 
                    {
                        if (stop_check.size() != 0) stop_check.clear();
                        else stop_check.push_back(piece->square);
                    }
                }
            }
            else if (piece->piece_t == PieceType::KNIGHT) 
            {
                char offset = k_sq - piece->square;
                char ox = offset % 8;
                char oy = offset / 8;
                if (std::find(knight_offsets.begin(), knight_offsets.end(), std::pair<char, char>(ox, oy)) != knight_offsets.end())
                {
                    if (stop_check.size() != 0) stop_check.clear();
                    else stop_check.push_back(piece->square);
                }
            }
            else if (piece->piece_t == PieceType::BISHOP) 
            {
                char p_offset = k_sq - piece->square;
                char offset;
                if (p_offset % 7 == 0) 
                {
                    if (p_offset > 0) offset = 7;
                    else offset = -7;
                }
                else if (p_offset % 9 == 0) 
                {
                    if (p_offset > 0) offset = 9;
                    else offset = 9;
                }
                else continue;

                stop_check.push_back(piece->square);

                for (size_t i = 1; ; i++)
                {
                    if (board[piece->square + i * offset]->piece_t != PieceType::EMPTY)
                    {
                        stop_check.clear();
                        break;
                    }
                    stop_check.push_back(piece->square + i * offset);
                    if (board[piece->square + (i + 1) * offset]->square == k_sq) break;
                }
            }
            else if (piece->piece_t == PieceType::ROOK) 
            {
                char p_offset = k_sq - piece->square;
                char offset;
                if (p_offset % 8 == 0) 
                {
                    if (p_offset > 0) offset = 8;
                    else offset = -8;
                }
                else if (p_offset < 8) 
                {
                    if (p_offset > 0) offset = 1;
                    else offset = -1;
                }
                else continue;

                stop_check.push_back(piece->square);

                for (size_t i = 1; ; i++)
                {
                    if (board[piece->square + i * offset]->piece_t != PieceType::EMPTY)
                    {
                        stop_check.clear();
                        break;
                    }
                    stop_check.push_back(piece->square + i * offset);
                    if (board[piece->square + (i + 1) * offset]->square == k_sq) break;
                } 
            }
            else if (piece->piece_t == PieceType::QUEEN)
            {
                char p_offset = k_sq - piece->square;
                char offset;
                if (p_offset % 7 == 0) 
                {
                    if (p_offset > 0) offset = 7;
                    else offset = -7;
                }
                else if (p_offset % 9 == 0) 
                {
                    if (p_offset > 0) offset = 9;
                    else offset = 9;
                }
                else if (p_offset % 8 == 0) 
                {
                    if (p_offset > 0) offset = 8;
                    else offset = -8;
                }
                else if (p_offset < 8) 
                {
                    if (p_offset > 0) offset = 1;
                    else offset = -1;
                }
                else continue;

                stop_check.push_back(piece->square);

                for (size_t i = 1; ; i++)
                {
                    if (board[piece->square + i * offset]->piece_t != PieceType::EMPTY)
                    {
                        stop_check.clear();
                        break;
                    }
                    stop_check.push_back(piece->square + i * offset);
                    if (board[piece->square + (i + 1) * offset]->square == k_sq) break;
                }
            }
        }
    }
        
    calc_pins(Color::WHITE);
    calc_pins(Color::BLACK);
}

std::vector<Move> Board::generate_moves(Color color, bool check)
{
    std::vector<Move> return_v;
    // Go through every piece of the color and generate moves for it
    for (auto piece : (color == Color::WHITE ? white : black))
    {
        if (piece->piece_t == PieceType::PAWN) 
        {
            size_t s = return_v.size();
            std::cout << "pawn" << std::endl;
            pawn_moves(*piece, return_v);
            std::cout << return_v.size() - s << std::endl;
        }
        else if (piece->piece_t == PieceType::KNIGHT) 
        {
            size_t s = return_v.size();
            std::cout << "knight" << std::endl;
            knight_moves(*piece, return_v);
            std::cout << return_v.size() - s << std::endl;
        }
        else if (piece->piece_t == PieceType::BISHOP) 
        {
            size_t s = return_v.size();
            std::cout << "bishop" << std::endl;
            bishop_moves(*piece, return_v);
            std::cout << return_v.size() - s << std::endl;
        }
        else if (piece->piece_t == PieceType::ROOK) 
        {
            size_t s = return_v.size();
            std::cout << "rook" << std::endl;
            rook_moves(*piece, return_v);
            std::cout << return_v.size() - s << std::endl;
        }
        else if (piece->piece_t == PieceType::QUEEN)
        {
            size_t s = return_v.size();
            std::cout << "queen" << std::endl;
            queen_moves(*piece, return_v);
            std::cout << return_v.size() - s << std::endl;
        }
        else if (piece->piece_t == PieceType::KING) 
        {
            size_t s = return_v.size();
            std::cout << "king" << std::endl;
            king_moves(*piece, return_v, check);
            std::cout << return_v.size() - s << std::endl;
        }
    }

    // Special moves
    // Castling
    if (color == Color::WHITE)
    {
        if (white_KC)
        {
            if (!in_check && !attacked[1][5] && !attacked[1][6] && board[5]->color == Color::NONE && board[6]->color == Color::NONE) return_v.push_back({4, 6});
        }   
        if (white_QC)
        {
            if (!in_check && !attacked[1][2] && !attacked[1][3] && board[2]->color == Color::NONE && board[3]->color == Color::NONE) return_v.push_back({4, 2});
        }
    }
    else
    {
        if (black_KC)
        {
            if (!in_check && !attacked[0][61] && !attacked[0][62] && board[61]->color == Color::NONE && board[62]->color == Color::NONE) return_v.push_back({60, 62});
        }   
        if (black_QC)
        {
            if (!in_check && !attacked[0][58] && !attacked[0][59] && board[58]->color == Color::NONE && board[59]->color == Color::NONE) return_v.push_back({60, 58});
        }
    }

    return return_v;
}

void Board::print_pieces()
{
    std::cout << "white" << std::endl;

    for (auto p : white)
    {
        std::cout << (size_t) p->piece_t << ", " << (int) p->square << std::endl;
    }

    std::cout << "black" << std::endl;

    for (auto p : black)
    {
        std::cout << (size_t) p->piece_t << ", " << (int) p->square << std::endl;
    }
}

void Board::update_history()
{
    short history = 0;
    history |= white_KC;
    history |= (short) white_QC << 1;
    history |= (short) black_KC << 2;
    history |= (short) black_QC << 3;
    history |= (0b00000111 & ep_file) << 4;
    history |= (0b00000111 & capture_type) << 7;
    history |= (0b00111111 & fifty_mover) << 10;
    game_history.push(history);
}

void Board::undo_history()
{
    short history = game_history.top();
    white_KC = history & (short) 0b00000001;
    white_QC = history & (short) 0b00000010;
    black_KC = history & (short) 0b00000100;
    black_QC = history & (short) 0b00001000;
    ep_file = (history & (short) 0b01110000) >> 4;
    capture_type = (history & (short) 0b01110000000) >> 7;
    fifty_mover = (history & (short) 0b1111110000000000) >> 10;
    game_history.pop();
}

void Board::calc_pins(Color color)
{
    for (auto p : (bool) color ? black : white)
    {
        if (p->piece_t == PieceType::KING)
        {
            // Calculate pins: go through each sliding piece
            for (auto p2 : ((bool) color ? white : black))
            {
                // Diagonal pins
                if (p2->piece_t == PieceType::BISHOP)
                {
                    // Go through nsw and nse (by doing a abs ksq - bsq % westoffset == 0 (or eastoffset))
                    // Figure which direction it is by positive ksq - bsq is north
                    // Loop through that ray until you hit the king, or 2 pieces (save a piece, and if you hit a second one break)
                    // Continue the loop when done
                    
                    char p_offset = p->square - p2->square;
                    char offset;
                    if (p_offset % 7 == 0) 
                    {
                        if (p_offset > 0) offset = 7;
                        else offset = -7;
                    }
                    else if (p_offset % 9 == 0) 
                    {
                        if (p_offset > 0) offset = 9;
                        else offset = 9;
                    }
                    else continue;

                    char pinned_sq = -1;
                    for (size_t i = 1; ; i++)
                    {
                        if (board[p2->square + i * offset]->piece_t != PieceType::EMPTY) 
                        {
                            if (board[p2->square + i * offset]->piece_t == PieceType::KING) break;
                            if (pinned_sq != -1)
                            {
                                pinned_sq = -1;
                                break;
                            }
                            pinned_sq = p2->square + i * offset;
                        }
                    }

                    if (pinned_sq != -1) 
                    {
                        pinned_squares[!(bool)color][pinned_sq] = abs(offset);
                    }
                }
                // Straight pins
                if (p2->piece_t == PieceType::ROOK)
                {
                    // Go through x and y (by doing a abs ksq - bsq % 1 == 0 or 8 instead of 1)
                    // Figure which direction it is by positive ksq - bsq is up or right, negative is down or left
                    // Loop through that ray until you hit the king, or 2 pieces (save a piece, and if you hit a second one break)
                    // Continue the loop when done

                    char p_offset = p->square - p2->square;
                    char offset;
                    if (p_offset % 8 == 0) 
                    {
                        if (p_offset > 0) offset = 8;
                        else offset = -8;
                    }
                    else if (p_offset < 8) 
                    {
                        if (p_offset > 0) offset = 1;
                        else offset = -1;
                    }
                    else continue;

                    char pinned_sq = -1;
                    for (size_t i = 1; ; i++)
                    {
                        if (board[p2->square + i * offset]->piece_t != PieceType::EMPTY) 
                        {
                            if (board[p2->square + i * offset]->piece_t == PieceType::KING) break;
                            if (pinned_sq != -1)
                            {
                                pinned_sq = -1;
                                break;
                            }
                            pinned_sq = p2->square + i * offset;
                        }
                    }

                    if (pinned_sq != -1) 
                    {
                        pinned_squares[!(bool) color][pinned_sq] = abs(offset); 
                    }
                }
                if (p2->piece_t == PieceType::QUEEN)
                {
                    char p_offset = p->square - p2->square;
                    char offset;
                    if (p_offset % 7 == 0) 
                    {
                        if (p_offset > 0) offset = 7;
                        else offset = -7;
                    }
                    else if (p_offset % 9 == 0) 
                    {
                        if (p_offset > 0) offset = 9;
                        else offset = 9;
                    }
                    else if (p_offset % 8 == 0) 
                    {
                        if (p_offset > 0) offset = 8;
                        else offset = -8;
                    }
                    else if (p_offset < 8) 
                    {
                        if (p_offset > 0) offset = 1;
                        else offset = -1;
                    }
                    else continue;

                    char pinned_sq = -1;
                    for (size_t i = 1; ; i++)
                    {
                        if (board[p2->square + i * offset]->piece_t != PieceType::EMPTY) 
                        {
                            if (board[p2->square + i * offset]->piece_t == PieceType::KING) break;
                            if (pinned_sq != -1)
                            {
                                pinned_sq = -1;
                                break;
                            }
                            pinned_sq = p2->square + i * offset;
                        }
                    }

                    if (pinned_sq != -1) 
                    {
                        pinned_squares[!(bool) color][pinned_sq] = abs(offset); 
                    }
                }
            }
        } 
    }    
}

// Calculate if a square is attacked by the color
bool Board::calc_attacks(Color color, char square)
{
    std::cout << "call" << std::endl;
    for (auto piece : (color == Color::WHITE ? white : black))
    {
        if (piece->piece_t == PieceType::PAWN) 
        {
            // Calculate a capture to the left (if its not on the leftmost file)
            if (piece->square % 8 != 0)
            {
                char target_sq = piece->square + pawn_offset[(bool) color][1];
                if (target_sq == square) return 1;
            }

            // Calculate a capture to the right (if its not on the rightmost file)
            if (piece->square % 8 != 7)
            {
                char target_sq = piece->square + pawn_offset[(bool) color][2];
                if (target_sq == square) return 1;
            }
        }
        else if (piece->piece_t == PieceType::KNIGHT) 
        {
            char offset = square - piece->square;
            char ox = offset % 8;
            char oy = offset / 8;
            if (std::find(knight_offsets.begin(), knight_offsets.end(), std::pair<char, char>(ox, oy)) != knight_offsets.end()) return 1;
        }
        else if (piece->piece_t == PieceType::BISHOP) 
        {
            char p_offset = square - piece->square;
            char offset;
            if (p_offset % 7 == 0) 
            {
                if (p_offset > 0) offset = 7;
                else offset = -7;
            }
            else if (p_offset % 9 == 0) 
            {
                if (p_offset > 0) offset = 9;
                else offset = -9;
            }
            else continue;

            if (p_offset == 0) continue;

            bool np_empty = 1;
            for (size_t i = 1; ; i++)
            {
                std::cout << "square" << (int) square << std::endl;
                std::cout << piece->square + i * offset << std::endl;
                std::cout << "i + 1" << piece->square + (i + 1) * offset << std::endl;
                std::cout << "boardpiece" << (int) board[piece->square + (i + 1) * offset]->square << std::endl;
                if (board[piece->square + i * offset]->piece_t != PieceType::EMPTY && board[piece->square + i * offset]->piece_t != PieceType::KING)
                {
                    np_empty = 0;
                    break;
                }
                if (piece->square + i * offset == square) break;
            }

            if (np_empty) return 1;    
        }
        else if (piece->piece_t == PieceType::ROOK) 
        {
            char p_offset = square - piece->square;
            char offset;
            if (p_offset % 8 == 0) 
            {
                if (p_offset > 0) offset = 8;
                else offset = -8;
            }
            else if (p_offset < 8) 
            {
                if (p_offset > 0) offset = 1;
                else offset = -1;
            }
            else continue;

            bool np_empty = 1;
            for (size_t i = 1; ; i++)
            {
                if (board[piece->square + i * offset]->piece_t != PieceType::EMPTY && board[piece->square + i * offset]->piece_t != PieceType::KING)
                {
                        np_empty = 0;
                        break;
                }
                if (board[piece->square + (i + 1) * offset]->square == square) break;
            }

            if (np_empty) return 1;    
        }
        else if (piece->piece_t == PieceType::QUEEN)
        {
            char p_offset = square - piece->square;
            char offset;
            if (p_offset % 7 == 0) 
            {
                if (p_offset > 0) offset = 7;
                else offset = -7;
            }
            else if (p_offset % 9 == 0) 
            {
                if (p_offset > 0) offset = 9;
                else offset = 9;
            }
            else if (p_offset % 8 == 0) 
            {
                if (p_offset > 0) offset = 8;
                else offset = -8;
            }
            else if (p_offset < 8) 
            {
                if (p_offset > 0) offset = 1;
                else offset = -1;
            }
            else continue;

            bool np_empty = 1;
            for (size_t i = 1; ; i++)
            {
                if (board[piece->square + i * offset]->piece_t != PieceType::EMPTY && board[piece->square + i * offset]->piece_t != PieceType::KING)
                {
                        np_empty = 0;
                        break;
                }
                if (board[piece->square + (i + 1) * offset]->square == square) break;
            }
            
            if (np_empty) return 1;    
        }
        else if (piece->piece_t == PieceType::KING) 
        {
            for (char dir = 0; dir < 8; dir++)
            {
                if (square == piece->square + sliding_offsets[dir]) return 1;
            }
        }
    }

    return 0;
}

// Does a move on the board (doesn't have to be legal, but if the move isn't legal it is undefined behavior)
void Board::move(Move move)
{
    // For piece pinning, figure out if each opposite piece that can pin is on line with the king
    // Once the ray is found, look for a piece blocking the ray to the king (or not, if it is a check)
    // If there is more than 1 piece blocking, disregard it
    // If there is only one piece, add the square to the is_pinned map for its color, and the direction (based on the ray)

    // Update the history with last moves history
    // If a pawn move square change abs value is 16, activate en passent for that file
    // If the move isn't a pawn move or a capture, add a tick to the 50 move rule
    // Else reset it
    // If it is a capture, add the piece to the capture map
    // If the move is a rook move or a king move, and castling is enabled for that side (or at all for the king), disable the flags, and add it to the flags

    update_history();

    fifty_mover++;

    // Do move on board
    Piece start_p = *board[move.start_pos];
    if (start_p.color != (Color) turn) return;

    if (board[move.end_pos]->color == opposite_color[start_p.color]) 
    {
        capture_type = (char) board[move.end_pos]->piece_t;
        for (auto p : (((bool) board[move.end_pos]->color) ? white : black))
        {
            if (p->square == move.end_pos) (((bool) board[move.end_pos]->color) ? white : black).remove(p);
        }
        board[move.end_pos]->color = Color::NONE;
        board[move.end_pos]->piece_t = PieceType::EMPTY;

        fifty_mover = 0;
    }

    board[move.end_pos]->square = move.start_pos;
    board[move.start_pos]->square = move.end_pos;
    board[move.start_pos].swap(board[move.end_pos]);

    // Clear castle flags
    if (start_p.piece_t == PieceType::KING)
    {
        // Is a castle
        if (abs(move.start_pos - move.end_pos) == 2)
        {
            // Move rook 
            // KC
            if (move.start_pos - move.end_pos < 0) 
            {
                board[7 + ((int) !(bool) start_p.color) * 56]->square = 5 + ((int) !(bool) start_p.color) * 56;
                board[5 + ((int) !(bool) start_p.color) * 56]->square = 7 + ((int) !(bool) start_p.color) * 56;
                board[7 + ((int) !(bool) start_p.color) * 56].swap(board[5 + ((int) !(bool) start_p.color) * 56]);
            }
            // QC
            else 
            {
                board[3 + ((int) !(bool) start_p.color) * 56]->square =((int) !(bool) start_p.color) * 56;
                board[((int) !(bool) start_p.color) * 56]->square = 3 + ((int) !(bool) start_p.color) * 56;
                board[3 + ((int) !(bool) start_p.color) * 56].swap(board[((int) !(bool) start_p.color) * 56]);
            }
        }

        // Clear castling
        if (start_p.color == Color::WHITE)
        {
            white_KC = 0;
            white_QC = 0;
        }
        else
        {
            black_KC = 0;
            black_QC = 0;
        }
    }

    if (start_p.piece_t == PieceType::ROOK && (start_p.square % 8) == 0) 
    {
        if (start_p.color == Color::WHITE) white_QC = 0;
        else black_QC = 0;
    }

    if (start_p.piece_t == PieceType::ROOK && (start_p.square % 8) == 7)
    {
        if (start_p.color == Color::WHITE) white_KC = 0;
        else white_QC = 0;
    }

    // Generate attack squares
    pinned_squares[0].fill(0);
    pinned_squares[1].fill(0);

    // Find opposing king square
    char k_sq;
    for (auto p : ((bool) start_p.color) ? black : white)
    {
        if (p->piece_t == PieceType::KING)
        {
            k_sq = p->square;
            break;
        }  
    }

    attacked[!(bool) start_p.color].fill(0);

    // Do attacking squares right ie the 8 squares around the king, the king, and castling squares 
    // Generate attack squares
    // Squares adjacent to king
    for (size_t i = 0; i < 8; i++)
    {
        attacked[!(bool) start_p.color][k_sq + sliding_offsets[i]] = calc_attacks(start_p.color, k_sq + sliding_offsets[i]);
        std::cout << k_sq + sliding_offsets[i] << ", " << attacked[!(bool) start_p.color][k_sq + sliding_offsets[i]] << std::endl;
    }
    
    in_check = calc_attacks(start_p.color, k_sq);

    // Hardcoded castle squares
    attacked[!(bool) start_p.color][k_sq - 2] = calc_attacks(start_p.color, k_sq - 2);
    attacked[!(bool) start_p.color][k_sq + 2] = calc_attacks(start_p.color, k_sq + 2);

    // If 2 pieces, king must move
    // If 1 piece, based on the piece, generate a list of squares that stops the check (such as taking the checking piece or blocking)
    // Go through all pieces
    // Check if they attack the king (copy of attack_sq loop)
    // If they do, put king in check, and while raycasting (only for sliding pieces) add squares to list
    // Add piece square to list of stop check moves

    if (in_check)
    {
        for (auto piece : (start_p.color == Color::WHITE ? white : black))
        {
            if (piece->piece_t == PieceType::PAWN) 
            {
                // Calculate a capture to the left (if its not on the leftmost file)
                if (piece->square % 8 != 0)
                {
                    char target_sq = piece->square + pawn_offset[(bool) start_p.color][1];
                    if (target_sq == k_sq) 
                    {
                        if (stop_check.size() != 0) stop_check.clear();
                        else stop_check.push_back(piece->square);
                    }
                }

                // Calculate a capture to the right (if its not on the rightmost file)
                if (piece->square % 8 != 7)
                {
                    char target_sq = piece->square + pawn_offset[(bool) start_p.color][2];
                    if (target_sq == k_sq) 
                    {
                        if (stop_check.size() != 0) stop_check.clear();
                        else stop_check.push_back(piece->square);
                    }
                }
            }
            else if (piece->piece_t == PieceType::KNIGHT) 
            {
                char offset = k_sq - piece->square;
                char ox = offset % 8;
                char oy = offset / 8;
                if (std::find(knight_offsets.begin(), knight_offsets.end(), std::pair<char, char>(ox, oy)) != knight_offsets.end())
                {
                    if (stop_check.size() != 0) stop_check.clear();
                    else stop_check.push_back(piece->square);
                }
            }
            else if (piece->piece_t == PieceType::BISHOP) 
            {
                char p_offset = k_sq - piece->square;
                char offset;
                if (p_offset % 7 == 0) 
                {
                    if (p_offset > 0) offset = 7;
                    else offset = -7;
                }
                else if (p_offset % 9 == 0) 
                {
                    if (p_offset > 0) offset = 9;
                    else offset = 9;
                }
                else continue;

                stop_check.push_back(piece->square);

                for (size_t i = 1; ; i++)
                {
                    if (board[piece->square + i * offset]->piece_t != PieceType::EMPTY)
                    {
                        stop_check.clear();
                        break;
                    }
                    stop_check.push_back(piece->square + i * offset);
                    if (board[piece->square + (i + 1) * offset]->square == k_sq) break;
                }
            }
            else if (piece->piece_t == PieceType::ROOK) 
            {
                char p_offset = k_sq - piece->square;
                char offset;
                if (p_offset % 8 == 0) 
                {
                    if (p_offset > 0) offset = 8;
                    else offset = -8;
                }
                else if (p_offset < 8) 
                {
                    if (p_offset > 0) offset = 1;
                    else offset = -1;
                }
                else continue;

                stop_check.push_back(piece->square);

                for (size_t i = 1; ; i++)
                {
                    if (board[piece->square + i * offset]->piece_t != PieceType::EMPTY)
                    {
                        stop_check.clear();
                        break;
                    }
                    stop_check.push_back(piece->square + i * offset);
                    if (board[piece->square + (i + 1) * offset]->square == k_sq) break;
                } 
            }
            else if (piece->piece_t == PieceType::QUEEN)
            {
                char p_offset = k_sq - piece->square;
                char offset;
                if (p_offset % 7 == 0) 
                {
                    if (p_offset > 0) offset = 7;
                    else offset = -7;
                }
                else if (p_offset % 9 == 0) 
                {
                    if (p_offset > 0) offset = 9;
                    else offset = 9;
                }
                else if (p_offset % 8 == 0) 
                {
                    if (p_offset > 0) offset = 8;
                    else offset = -8;
                }
                else if (p_offset < 8) 
                {
                    if (p_offset > 0) offset = 1;
                    else offset = -1;
                }
                else continue;

                stop_check.push_back(piece->square);

                for (size_t i = 1; ; i++)
                {
                    if (board[piece->square + i * offset]->piece_t != PieceType::EMPTY)
                    {
                        stop_check.clear();
                        break;
                    }
                    stop_check.push_back(piece->square + i * offset);
                    if (board[piece->square + (i + 1) * offset]->square == k_sq) break;
                }
            }
        }
    }
        
    calc_pins(Color::WHITE);
    calc_pins(Color::BLACK);

    for (auto p : (bool) start_p.color ? black : white) if (p->piece_t == PieceType::KING && attacked[!(bool) start_p.color][p->square]) in_check = true; 

    // Wrap up stuff
    ep_file = 0;
    turn = !turn;
    if (board[move.end_pos]->piece_t == PieceType::PAWN)
    {
        fifty_mover = 0;
        if (abs(move.start_pos - move.end_pos) == 16) 
        {
            ep_file = move.start_pos % 8;
            capture_type = 0b111;
        }
    } 

    // Do castling
    if (turn) this->moves++;
}

// Undoes a move (has to be saved)
// Regen flag is if the search is going back multiple moves it wont regenerate attack squares, check, and pinned pieces
void Board::unmove(Move move, bool regen)
{   
    // Update some flags 
    turn = !turn;
    if (turn) this->moves--;

    // Undo the move on board
    Piece start_p = *board[move.end_pos];

    board[move.end_pos]->square = move.start_pos;
    board[move.start_pos]->square = move.end_pos;
    board[move.start_pos].swap(board[move.end_pos]);

    // Undo captures
    if (capture_type != 0b000 && capture_type != 0b111) 
    {
        board[move.end_pos]->color = opposite_color[board[move.start_pos]->color];
        board[move.end_pos]->piece_t = (PieceType) capture_type;
        ((board[move.end_pos]->color == Color::WHITE) ? white : black).push_back(board[move.end_pos]);
    }

    // Generate attack squares, pinned pieces, and check
    if (regen)
    {
        // Generate attack squares
        pinned_squares[0].fill(0);
        pinned_squares[1].fill(0);
        std::vector<Move> moves = generate_moves(start_p.color, false);
        attacked[!(bool)start_p.color].fill(0);
    
        for (auto m : moves)
        {
            if (!attacked[!(bool)start_p.color][m.end_pos]) attacked[!(bool)start_p.color][m.end_pos] = 1;
        }

        calc_pins(Color::WHITE);
        calc_pins(Color::BLACK);
    }

    undo_history();
}

// Calculate all possible rook moves for a piece
void Board::rook_moves(Piece piece, std::vector<Move>& moves)
{
    if (pinned_squares[(bool)piece.color][piece.square] != 0)
    {
        if (pinned_squares[(bool)piece.color][piece.square] % 2 == 1) return;
        std::vector<char> dirs = {pinned_squares[(bool)piece.color][piece.square], (char) -pinned_squares[(bool)piece.color][piece.square]};
        for (size_t dir = 0; dir < dirs.size(); dir++)
        {
            for (size_t i = 1; ; i++)
            {
                char t_square = piece.square + dirs[dir] * i; 
                Color t_color = board[t_square]->color;
                if (t_color == piece.color) break;
                if (!in_check || std::find(stop_check.begin(), stop_check.end(), (char) t_square) != stop_check.end()) moves.push_back({piece.square, t_square});
                if (t_color == opposite_color[piece.color]) break;
            }
        }
    }
    else
    {
        for (size_t dir = 0; dir < 4; dir++)
        {
            for (size_t i = 1; i < num_sq_to_edge[piece.square][dir]; i++)
            {
                char t_square = piece.square + sliding_offsets[dir] * i; 
                Color t_color = board[t_square]->color;
                if (t_color == piece.color) break;
                if (!in_check || std::find(stop_check.begin(), stop_check.end(), (char) t_square) != stop_check.end()) moves.push_back({piece.square, t_square});
                if (t_color == opposite_color[piece.color]) break;
            }
        }
    }
}

// Calculate all possible bishop moves for a bishop
void Board::bishop_moves(Piece piece, std::vector<Move>& moves)
{
    if (pinned_squares[(bool)piece.color][piece.square] != 0)
    {
        if (pinned_squares[(bool)piece.color][piece.square] % 2 == 0) return;
        std::vector<char> dirs = {pinned_squares[(bool)piece.color][piece.square], (char) -pinned_squares[(bool)piece.color][piece.square]};
        for (size_t dir = 0; dir < dirs.size(); dir++)
        {
            for (size_t i = 1; ; i++)
            {
                char t_square = piece.square + dirs[dir] * i; 
                Color t_color = board[t_square]->color;
                if (t_color == piece.color) break;
                if (!in_check || std::find(stop_check.begin(), stop_check.end(), (char) t_square) != stop_check.end()) moves.push_back({piece.square, t_square});
                if (t_color == opposite_color[piece.color]) break;
            }
        }
    }
    else
    {
        for (size_t dir = 0; dir < 4; dir++)
        {
            for (size_t i = 1; i < num_sq_to_edge[piece.square][dir + 4]; i++)
            {
                char t_square = piece.square + sliding_offsets[dir + 4] * i; 
                Color t_color = board[t_square]->color;
                if (t_color == piece.color) break;
                if (!in_check || std::find(stop_check.begin(), stop_check.end(), (char) t_square) != stop_check.end()) moves.push_back({piece.square, t_square});
                if (t_color == opposite_color[piece.color]) break;
            }
        }
    }
}

// Combine bishop + rook moves, generate all queen moves
void Board::queen_moves(Piece piece, std::vector<Move>& moves)
{
    rook_moves(piece, moves);
    bishop_moves(piece, moves);
}

// Calculate all valid knight moves from a table
void Board::knight_moves(Piece piece, std::vector<Move>& moves)
{
    if (pinned_squares[(bool) piece.color][piece.square] != 0) return;
    char kx = piece.square % 8;
    char ky = piece.square / 8;

    for (auto [x, y] : knight_offsets)
    {
        char tx = x + kx;
        char ty = y + ky;
        if (tx > 7 || tx < 0 || ty > 7 || ty < 0) continue;
        char new_sq = tx + ty * 8;
        if (board[new_sq]->color == piece.color) continue;
        if (!in_check || std::find(stop_check.begin(), stop_check.end(), (char) new_sq) != stop_check.end()) moves.push_back({piece.square, new_sq});
    }
}

// Calculate all valid king moves
void Board::king_moves(Piece piece, std::vector<Move>& moves, bool check)
{
    // Loop through all directions
    for (size_t dir = 0; dir < 8; dir++)
    {
        // Calculate target square
        char t_square = piece.square + sliding_offsets[dir]; 
        // Calculate target square color (none, for an empty piece, or black/white)
        Color t_color = board[t_square]->color;
        // Check if it is piece of the same color, break if it is
        if (t_color == piece.color) continue;
        // Check if it an attacked square
        if (attacked[(bool) piece.color][t_square] && check) continue;
        // Add the move
        moves.push_back({piece.square, t_square});
        // Check if it is a different color piece, break if it is
        if (t_color == opposite_color[piece.color]) continue;
    }
}

// Calculate all valid pawn moves
void Board::pawn_moves(Piece piece, std::vector<Move>& moves)
{
    // Calculate pawn pins
    if (pinned_squares[(bool) piece.color][piece.square] == 1) return;
 
    // Calculate rank and file
    char px = piece.square % 8;
    char py = piece.square / 8;

    if (pinned_squares[(bool) piece.color][piece.square] == 0 || pinned_squares[(bool) piece.color][piece.square] == 8)
    {
        // Calculate if a piece can move forward 1
        char pawn_oy = pawn_offset[(bool)piece.color][0];
        if (board[piece.square + pawn_oy]->piece_t == PieceType::EMPTY)
        {
            if (!in_check || std::find(stop_check.begin(), stop_check.end(), (char) piece.square + pawn_oy) != stop_check.end()) moves.push_back({piece.square, (char) (piece.square + pawn_oy)});
            // Calculate if a piece can move forward 2 (only on starting square)
            if (board[piece.square + pawn_oy * 2]->piece_t == PieceType::EMPTY && py == start_y[(bool) piece.color] && (!in_check || std::find(stop_check.begin(), stop_check.end(), (char) piece.square + pawn_oy * 2) != stop_check.end())) moves.push_back({piece.square, (char) (piece.square + pawn_oy * 2)});
        }
    }
    
    // Calculate a capture to the left (if its not on the leftmost file)
    if (px != 0 && (pinned_squares[(bool) piece.color][piece.square] == 0 || pinned_squares[(bool) piece.color][piece.square] == 9))
    {
        char target_sq = piece.square + pawn_offset[(bool)piece.color][1];
        if (opposite_color[board[target_sq]->color] == piece.color && (!in_check || std::find(stop_check.begin(), stop_check.end(), target_sq) != stop_check.end())) moves.push_back({piece.square, target_sq});
    }

    // Calculate a capture to the right (if its not on the rightmost file)
    if (px != 7 && (pinned_squares[(bool) piece.color][piece.square] == 0 || pinned_squares[(bool) piece.color][piece.square] == 7))
    {
        char target_sq = piece.square + pawn_offset[(bool)piece.color][2];
        if (opposite_color[board[target_sq]->color] == piece.color && (!in_check || std::find(stop_check.begin(), stop_check.end(), target_sq) != stop_check.end())) moves.push_back({piece.square, target_sq});
    }
}
