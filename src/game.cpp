#include "game.h"

#include "bitset_handle.h"

// std
#include <iostream>
#include <set>
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

// Calculate the valid bits in the bitset for the given square and each direction for sliding pieces
std::array<std::array<uint64_t, 8>, 64> bit_tables;

// Calculate the valid bits in the bitset for knights on the given square
std::array<uint64_t, 64> knight_bit_tables;

// Calculate the valid bits in the bitset for pawn on the given square (and color)
std::array<std::array<uint64_t, 2>, 64> pawn_forward;
std::array<std::array<uint64_t, 2>, 64> pawn_captures;

// King allowed moves calculation and bitmaps (only for calculating attack squares)
std::array<uint64_t, 64> king_bit_tables;

// Pin tables, for a square and a PIN DIRECTION what table do you have
std::array<std::array<uint64_t, 5>, 64> pin_tables;

// King castle squares (bitmaps), add 2 if color is white
std::array<uint64_t, 4> king_castle({
    (uint64_t) 1 << 61, (uint64_t) 62, (uint64_t) 1 << 5, (uint64_t) 1 << 6
});

// Queen side castle king move square (bitmaps), to use add 2 if the color is white
std::array<uint64_t, 4> queen_castle({
    (uint64_t) 1 << 57, (uint64_t) 58, (uint64_t) 1 << 2, (uint64_t) 1 << 3
});

// Calculate the valid bits in a bitset for a certain EP file
std::array<std::array<uint64_t, 8>, 2> ep_bitmap({{
    {{(uint64_t)1 << 16, (uint64_t)1 << 17, (uint64_t)1 << 18, (uint64_t)1 << 19, (uint64_t)1 << 20, (uint64_t)1 << 21, (uint64_t)1 << 22, (uint64_t)1 << 23}},
    {{(uint64_t)1 << 40, (uint64_t)1 << 41, (uint64_t)1 << 42, (uint64_t)1 << 43, (uint64_t)1 << 44, (uint64_t)1 << 45, (uint64_t)1 << 46, (uint64_t)1 << 47}},
}});

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

// List of offsets for the pawn for each color
std::array<std::array<char, 3>, 2> pawn_offsets({{
    {{-8, -9, -7}},
    {{ 8,  9,  7}}
}});

// List of different offsets for pins and how the correspond to sliding offsets
// Pinned directions are 0 (nothing) 1 (horizontal) 2 (vertical) 3 (nwse) 4 (nesw)
std::unordered_map<char, char> pin_offsets({
    {-1, 1},
    {1, 1}, 
    {-8, 2},
    {8, 2},
    {-7, 3},
    {7, 3},
    {-9, 4},
    {9, 4}
});

// Reverse of pin offsets
std::unordered_map<char, std::pair<char, char>> rv_pin_offsets({
    {1, {1, -1}},
    {2, {8, -8}},
    {3, {7, -7}},
    {4, {9, -9}}
});

// Pawn starting squares
std::array<char, 2> start_y = {
    6, 1
};

// If a rank/file is in bounds it will return this
std::set<char> in_bounds({
    0, 1, 2, 3, 4, 5, 6, 7, 8
});

// Generate all of the maps used
void gen_bit_tables()
{
    for (size_t sq = 0; sq < 64; sq++)
    {
        char x = sq % 8;
        char y = sq / 8;
        
        char numNorth = 8 - y;
        char numSouth = y + 1;
        char numWest = x + 1;
        char numEast = 8 - x;

        std::array<char, 8> num_to_edge = 
        {
            numWest, numEast, numNorth, numSouth, std::min(numWest, numNorth), std::min(numWest, numSouth), std::min(numEast, numNorth), std::min(numEast, numSouth)
        };

        for (size_t i = 0; i < 8; i++)
        {
            std::bitset<64> save = 0;
            for (size_t j = sq + sliding_offsets[i]; j < num_to_edge[i]; j += sliding_offsets[i])
            {
                save[j] = 1;
            }
            bit_tables[sq][i] = save.to_ullong();
        }

        std::bitset<64> save = 0;
        for (auto off : knight_offsets)
        {
            if (in_bounds.contains(x + off.first) || in_bounds.contains(y + off.second)) save[x + off.first + (y + off.second) * 8] = 1;
        }

        knight_bit_tables[sq] = save.to_ullong();

        save = 0;
        save[sq + pawn_offsets[0][0]] = 1;
        pawn_forward[sq][0] = save.to_ullong();
        save = 0;
        save[sq + pawn_offsets[1][0]] = 1;
        pawn_forward[sq][0] = save.to_ullong();
        save = 0;
        if (in_bounds.contains((sq + pawn_offsets[0][1]) / 8) && in_bounds.contains((sq + pawn_offsets[0][1]) % 8)) save[sq + pawn_offsets[0][1]] = 1;
        if (in_bounds.contains((sq + pawn_offsets[0][2]) / 8) && in_bounds.contains((sq + pawn_offsets[0][2]) % 8)) save[sq + pawn_offsets[0][2]] = 1;
        pawn_captures[sq][0] = save.to_ullong();
        save = 0;
        if (in_bounds.contains((sq + pawn_offsets[1][1]) / 8) && in_bounds.contains((sq + pawn_offsets[1][1]) % 8)) save[sq + pawn_offsets[1][1]] = 1;
        if (in_bounds.contains((sq + pawn_offsets[1][2]) / 8) && in_bounds.contains((sq + pawn_offsets[1][2]) % 8)) save[sq + pawn_offsets[1][2]] = 1;
        pawn_captures[sq][1] = save.to_ullong();
        save = 0;


        for (size_t i = 0; i < 8; i++)
        {
            if (in_bounds.contains((sq + sliding_offsets[i]) / 8) && in_bounds.contains((sq + sliding_offsets[i]) % 8)) save[sq + sliding_offsets[i]] = 1;
        }

        king_bit_tables[sq] = save.to_ullong();

        pin_tables[0][sq] = UINT64_MAX;

        for (size_t i = 1; i < 5; i++)
        {
            pin_tables[sq][i] = bit_tables[sq][rv_pin_offsets[i].first] | bit_tables[sq][rv_pin_offsets[i].second];
        }
    }
}

// Initalizes board with empty squares
Board::Board()
{
    gen_bit_tables();
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

        for (char j = 0, fen_idx = 0; j < 8; j++, fen_idx++)
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
    std::string ep = castle.substr(split_idx + 1);
    castle.erase(split_idx);

    std::unordered_map<char, bool*> castle_set({
        {'K', &white_KC},
        {'Q', &white_QC},
        {'k', &black_KC},
        {'q', &black_QC},
    });

    for (auto [c, varp] : castle_set) if (castle.find(c) != std::string::npos) *varp = true;
    
    update_board((Color) this->turn);

    // Update ep
    if (ep[0] != '-') 
    {
        capture_type = 0b111;
        ep_file = ep[0] - 64;
    }
}

std::vector<Move> Board::generate_moves(Color color)
{
    std::vector<Move> return_v;
    // Go through every piece of the color and generate moves for it
    for (auto piece : (color == Color::WHITE ? white : black))
    {
        if (piece->piece_t == PieceType::PAWN) 
        {
            uint64_t move_bmap = pawn_moves(*piece);
            if ((move_bmap & 0xFF) == 0xFF)
            {
                return_v.push_back(Move{*piece, move_bmap & ~(uint64_t)0xFF, PieceType::KNIGHT});
                return_v.push_back(Move{*piece, move_bmap & ~(uint64_t)0xFF, PieceType::BISHOP});
                return_v.push_back(Move{*piece, move_bmap & ~(uint64_t)0xFF, PieceType::ROOK});
                return_v.push_back(Move{*piece, move_bmap & ~(uint64_t)0xFF, PieceType::QUEEN});
            }
            else if ((move_bmap & 0xFF00000000000000) == 0xFF00000000000000)
            {
                return_v.push_back(Move{*piece, move_bmap & ~0xFF00000000000000, PieceType::KNIGHT});
                return_v.push_back(Move{*piece, move_bmap & ~0xFF00000000000000, PieceType::BISHOP});
                return_v.push_back(Move{*piece, move_bmap & ~0xFF00000000000000, PieceType::ROOK});
                return_v.push_back(Move{*piece, move_bmap & ~0xFF00000000000000, PieceType::QUEEN});
            }
            else
            {
                return_v.push_back(Move{*piece, move_bmap});
            }

        }
        else if (piece->piece_t == PieceType::KNIGHT) 
        {
            return_v.push_back(Move{*piece, knight_moves(*piece)});
        }
        else if (piece->piece_t == PieceType::KING) 
        {
            return_v.push_back(Move{*piece, attacked});
        }
        else
        {
            return_v.push_back(Move{*piece, sliding_moves(*piece)});
        }
    }

    // Castling
    // if (color == Color::WHITE)
    // {
    //     if (white_KC)
    //     {
    //         if (!in_check && !attacked[1][5] && !attacked[1][6] && board[5]->color == Color::NONE && board[6]->color == Color::NONE) return_v.push_back({4, 6});
    //     }   
    //     if (white_QC)
    //     {
    //         if (!in_check && !attacked[1][2] && !attacked[1][3] && board[2]->color == Color::NONE && board[3]->color == Color::NONE) return_v.push_back({4, 2});
    //     }
    // }
    // else
    // {
    //     if (black_KC)
    //     {
    //         if (!in_check && !attacked[0][61] && !attacked[0][62] && board[61]->color == Color::NONE && board[62]->color == Color::NONE) return_v.push_back({60, 62});
    //     }   
    //     if (black_QC)
    //     {
    //         if (!in_check && !attacked[0][58] && !attacked[0][59] && board[58]->color == Color::NONE && board[59]->color == Color::NONE) return_v.push_back({60, 58});
    //     }
    // }

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

// Generates a bitmap of every square the pieces attack (if the opposing king didn't exist)
// Used for calculating attacks
uint64_t Board::all_attacks(Color color, uint64_t no_king)
{
    uint64_t all_attacked = 0;
    for (auto x : color == Color::WHITE ? white : black)
    {
        switch (x->piece_t)
        {
            case PieceType::PAWN:
            {
                all_attacked |= pawn_captures[(bool) color][x->square];
                break;
            }
            case PieceType::KNIGHT:
            {
                all_attacked |= knight_bit_tables[x->square];
                break;
            }
            case PieceType::KING: 
            {
                all_attacked |= king_bit_tables[x->square];
                break;
            }
            default: 
            {
                size_t start = x->piece_t == PieceType::BISHOP ? 4 : 0;
                size_t end = x->piece_t == PieceType::QUEEN ? 8 : start + 4;

                for (; start < end; start++)
                {
                    uint64_t masked_blockers = bit_tables[x->square][start] & no_king;
                    char closest = sliding_offsets[start] > 0 ? bit_scan_fw(masked_blockers) : bit_scan_rv(masked_blockers);
                    if (board[closest]->color == x->color) closest -= sliding_offsets[start];
                    all_attacked |= bit_tables[x->square][start] & ~bit_tables[closest][start];
                }
            }
        }
    }

    return all_attacked;
}

void Board::calc_pins(Color color, char king_sq)
{
    // Go through every sliding piece
    // For each sliding piece direction see if the opposing king is in that line (without blockers)
    // If so, and the board with that line (removing all parts past the king) and call checkBit 
    // Use found direction and the returned value in the pin map

    for (auto x : color == Color::WHITE ? white : black)
    {
        if (x->piece_t == PieceType::BISHOP || x->piece_t == PieceType::ROOK || x->piece_t == PieceType::QUEEN)
        {
            size_t start = x->piece_t == PieceType::BISHOP ? 4 : 0;
            size_t end = x->piece_t == PieceType::QUEEN ? 8 : start + 4;

            for (; start < end; start++)
            {
                if (bit_tables[x->square][start] & (uint64_t) 1 << king_sq) 
                {
                    size_t bit_loc = find_set_bit(bit_tables[x->square][start] & ~bit_tables[king_sq - sliding_offsets[start]][start]);
                    if (bit_loc) pins[bit_loc] = pin_offsets[start];
                    break;
                }
            }
        }
    }
}

void Board::update_board(Color color)
{
    // Clear the pins
    pins.fill(0);

    // Find the king (this is the most annoying thing)
    char king_sq;
    for (auto x : color == Color::WHITE ? black : white)
    {
        if (x->piece_t == PieceType::KING) 
        {
            king_sq = x->square;
            break;
        }
    }
    
    // Calculate safe squares around the king (using generate attacks method but xor all pieces with king square (so the king doesnt exist in terms of seeing if the square is attacked)
    uint64_t attacked_sqs = all_attacks(color, all_pieces.to_ullong() ^ ((uint64_t) 1 << king_sq));
    attacked = attacked_sqs & king_bit_tables[king_sq];

    // See if the king is in check
    in_check = attacked_sqs & (uint64_t) 1 << king_sq;

    // Castling squares check
    KC_safe = (king_castle[(int) color * 2] & attacked_sqs) && (king_castle[(int) color * 2 + 1] & attacked_sqs);
    QC_safe = (queen_castle[(int) color * 2] & attacked_sqs) && (queen_castle[(int) color * 2 + 1] & attacked_sqs);

    // Calculate pins 
    calc_pins(color, king_sq);
 
    // If king is in check, if so generate stop check squares (EVENTUALLY DO THIS IN CALC ATTACKS)
    if (in_check) 
    {
        for (auto x : color == Color::WHITE ? white : black)
        {
            switch (x->piece_t)
            {
                case PieceType::PAWN:
                {
                    if (!(pawn_captures[(bool) color][x->square] & (uint64_t) 1 << king_sq)) break;

                    if (stop_check != 0) 
                    { 
                        stop_check = UINT64_MAX; 
                        break;
                    }

                    stop_check = 1 << x->square;

                    break;
                }
                case PieceType::KNIGHT:
                {
                    if (!(knight_bit_tables[x->square] & (uint64_t) 1 << king_sq)) break;

                    if (stop_check != 0) 
                    { 
                        stop_check = UINT64_MAX; 
                        break;
                    }

                    stop_check = 1 << x->square;

                    break;
                }
                case PieceType::KING: break;
                default: 
                {
                    if (stop_check != 0) 
                    { 
                        stop_check = UINT64_MAX; 
                        break;
                    }

                    size_t start = x->piece_t == PieceType::BISHOP ? 4 : 0;
                    size_t end = x->piece_t == PieceType::QUEEN ? 8 : start + 4;

                    for (; start < end; start++)
                    {
                        uint64_t masked_blockers = bit_tables[x->square][start] & all_pieces.to_ullong();
                        char closest = sliding_offsets[start] > 0 ? bit_scan_fw(masked_blockers) : bit_scan_rv(masked_blockers);
                        if (board[closest]->color == x->color) closest -= sliding_offsets[start];
                        uint64_t result = bit_tables[x->square][start] & ~bit_tables[closest][start];
                        if (result & (uint64_t) 1 << king_sq) 
                        {
                            stop_check = result | (uint64_t) 1 << x->square;
                            break;
                        }
                    }
                }
            }

            if (stop_check == UINT64_MAX)
            {
                stop_check = 0;
                break;
            }
        }
    }
}

// Move generation
// For pins there will be a direction that the pinned piece is allowed to move
// For checks there will be a table that has all of the legal squares a normal piece is allowed to go (ie. blocking or capturing)
// King moves are based on calculating all of the squares around the king, and if they are not attacked by the opponent they are safe
// If no moves in normal piece table then only king moves and vice versa
// If no moves at all then it is mate
// ADD FLAG FOR ATTACKING SQUARES + PIN CHECKING to turn of pin checking for pin generation moves

uint64_t Board::sliding_moves(Piece piece)
{
    size_t start = piece.piece_t == PieceType::BISHOP ? 4 : 0;
    size_t end = piece.piece_t == PieceType::QUEEN ? 8 : start + 4;

    uint64_t output = 0;

    for (; start < end; start++)
    {
        // Pin logic, this is a better solution im just too lazy
        // if (pinned_direction[pins[piece.square]][start]) continue;

        uint64_t masked_blockers = bit_tables[piece.square][start] & all_pieces.to_ullong();
        char closest = sliding_offsets[start] > 0 ? bit_scan_fw(masked_blockers) : bit_scan_rv(masked_blockers);
        if (board[closest]->color == piece.color) closest -= sliding_offsets[start];
        output |= bit_tables[piece.square][start] & ~bit_tables[closest][start];
    }

    return output & stop_check & pin_tables[piece.square][pins[piece.square]];
}

uint64_t Board::knight_moves(Piece piece)
{
    if (pins[piece.square] != 0) return 0;
    return knight_bit_tables[piece.square] & ~(piece.color == Color::WHITE ? white_pieces.to_ullong() : black_pieces.to_ullong()) & stop_check;
} 

uint64_t Board::pawn_moves(Piece piece)
{
    // Calculate moving 1 forward and capturing (from pawn map)
    // If the move forward promote at bits at opposite side for what it is promoting to 
    // Use hardcoded logic for 2 squares 

    // Do ep using ep_bitmap
    uint64_t output = (pawn_captures[piece.square][(bool) piece.color] & ((piece.color == Color::WHITE ? black_pieces.to_ullong() : white_pieces.to_ullong()) || ep_bitmap[(bool) piece.color][ep_file])) || (pawn_forward[piece.square][(bool) piece.square] & ~all_pieces.to_ullong());
    output &= pin_tables[piece.square][pins[piece.square]]; 

    // Check for promotion  
    if (piece.square < 8) output |= 0xFF;
    if (piece.square > 56) output |= 0xFF00000000000000;

    // Do proper checking of start squares 
    // if (!all_pieces[piece.square += pawn_offset[piece.color]] && !all_pieces[piece.square += 2 * pawn_offset[piece.color]]) output |= 1 << (piece.square += pawn_offset[piece.color])

    return output & stop_check;
}
