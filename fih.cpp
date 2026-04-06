#include <cstdint>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>


// ================================= //
// |       fih chess engine        | //
// ================================= //
// This is my second attempt at      //
// writing a chess engine. This bot  //
// was made a learning oppurinity.   //
// Yes I know a lot of the code is   //
// horrible to say the least but its //
// fine. Hopefully this engine will  //
// become 2500 elo at some point :)  // 
// ================================= //


//             TO DO                 //
// ================================= //
// 1. Implement the new encoding system
// 2. Update makeMove() & unmakeMove() as needed
// 3. Write the generateMoves() function
// 4. Write a good solid fen parser
// 5. Write a preft test function to confirm makeMove() & unmakeMove() and generate moves work as intened
// 6. Write a basic search function using alpha beta and simple pruning
// 7. Add a very basic evaluation function using piece tables etc\
// 8. Add in basic UCI & threading
// 9. After this is the fun part

// ================================
// Move encoding (packed uint32_t)
// ================================
//
// Bit layout:
//  0- 5  from square
//  6-11  to square
// 12-15  piece     (stored as piece + 7, so -6..6 becomes 1..13)
// 16-19  captured  (stored as captured + 7, 0+7=7 means no capture)
// 20-23  promotion (stored as promotion + 7, 7 means no promotion)
// 24     enPassant flag
// 25     castle flag



static const int PIECE_OFFSET = 7; // offset added before storing signed piece values

inline uint32_t encodeMove(int from, int to, int piece, int captured, int promotion, bool enPassant, bool castle) {
    return  (uint32_t)(from)                          // bits 0-5
          | (uint32_t)(to)              << 6          // bits 6-11
          | (uint32_t)(piece    + PIECE_OFFSET) << 12 // bits 12-15
          | (uint32_t)(captured + PIECE_OFFSET) << 16 // bits 16-19
          | (uint32_t)(promotion+ PIECE_OFFSET) << 20 // bits 20-23
          | (uint32_t)(enPassant)                << 24 // bit 24
          | (uint32_t)(castle)                   << 25; // bit 25
}

inline int getFrom(uint32_t move) { return move & 0x3F; } // grab lowest 6 bits 
inline int getTo(uint32_t move) { return (move >> 6) & 0x3F; } // shift right 6, grab 6 bits
inline int getPiece(uint32_t move) { return ((move >> 12) & 0x0F) - PIECE_OFFSET; } // shift right 12, grab 4 bits, undo offset
inline int getCaptured(uint32_t move) { return ((move >> 16) & 0x0F) - PIECE_OFFSET; }
inline int getPromotion(uint32_t move) { return ((move >> 20) & 0x0F) - PIECE_OFFSET; }
inline bool getEnPassant(uint32_t move) { return (move >> 24) & 0x01; } // shift right 24, grab 1 bit 
inline bool getCastle(uint32_t move) { return (move >> 25) & 0x01; }
inline bool isCapture(uint32_t move) { return getCaptured(move) != 0; } // Helper to check if a move has no capture (captured field == 0 after undoing offset)
inline bool isPromotion(uint32_t move) { return getPromotion(move) != 0; } // Helper to check if a move is a promotion

struct BoardState {
    bool wkscr, wqscr, bkscr, bqscr;
    int enpassant_square;
    int side;
    int halfmove_clock;
};


class Board {
    public:
        // ========================= //
        // | Board class variables | //
        // ========================= //

        int side = 0;
        int halfmove_clock = 0;
        int enpassant_square = -1;
        bool wkscr = true;
        bool wqscr = true;
        bool bkscr = true;
        bool bqscr = true;

        uint64_t wPawns = 0;
        uint64_t wBishop = 0;
        uint64_t wKnight = 0;
        uint64_t wRook = 0;
        uint64_t wQueen = 0;
        uint64_t wKing = 0;

        uint64_t bPawns = 0;
        uint64_t bBishop = 0;
        uint64_t bKnight = 0;
        uint64_t bRook = 0;
        uint64_t bQueen = 0;
        uint64_t bKing = 0;

        int mailbox[64]; //learned that having something like this is recommened and is used almost by all engines?

        uint64_t pawnAttacksW[64];
        uint64_t pawnAttacksB[64];
        uint64_t knightAttacks[64];
        uint64_t kingAttacks[64];

        uint64_t rookMasks[64];
        uint64_t bishopMasks[64];

        int rookOffsets[64];
        int bishopOffsets[64];

        uint64_t rookAttacks[102400];
        uint64_t bishopAttacks[5248];

        int HistoryIndex = 0;   
        uint32_t moveHistory[4096];
        BoardState stateHistory[4096];

        bool initBoardRan = false;

        //Magic bits
        uint64_t rookMagics[64] = { //Rook magic numbers [64]
            0x8a80104000800020ULL, 0x140002000100040ULL,  0x2801880a0017001ULL,  0x100081001000420ULL,
            0x200020010080420ULL,  0x3001c0002010008ULL,  0x8480008002000100ULL, 0x2080088004402900ULL,
            0x800098204000ULL,     0x2024401000200040ULL, 0x100802000801000ULL,  0x120800800801000ULL,
            0x208808088000400ULL,  0x2802200800400ULL,    0x2200800100020080ULL, 0x801000060821100ULL,
            0x80044006422000ULL,   0x100808020004000ULL,  0x12108a0010204200ULL, 0x140848010000802ULL,
            0x481828014002800ULL,  0x8094004002004100ULL, 0x4010040010010802ULL, 0x20008806104ULL,
            0x100400080208000ULL,  0x2040002120081000ULL, 0x21200680100081ULL,   0x20100080080080ULL,
            0x2000a00200410ULL,    0x20080800400ULL,      0x80088400100102ULL,   0x80004600042881ULL,
            0x4040008040800020ULL, 0x440003000200801ULL,  0x4200011004500ULL,    0x188020010100100ULL,
            0x14800401802800ULL,   0x2080040080800200ULL, 0x124080204001001ULL,  0x200046502000484ULL,
            0x480400080088020ULL,  0x1000422010034000ULL, 0x30200100110040ULL,   0x100021010009ULL,
            0x2002080100110004ULL, 0x202008004008002ULL,  0x20020004010100ULL,   0x2048440040820001ULL,
            0x101002200408200ULL,  0x40802000401080ULL,   0x4008142004410100ULL, 0x2060820c0120200ULL,
            0x1001004080100ULL,    0x20c020080040080ULL,  0x2935610830022400ULL, 0x44440041009200ULL,
            0x280001040802101ULL,  0x2100190040002085ULL, 0x80c0084100102001ULL, 0x4024081001000421ULL,
            0x20030a0244872ULL,    0x12001008414402ULL,   0x2006104900a0804ULL,  0x1004081002402ULL
        };

        uint64_t bishopMagics[64] = { // Bishop magic numbers [64]
            0x40040844404084ULL,   0x2004208a004208ULL,   0x10190041080202ULL,   0x108060845042010ULL,
            0x581104180800210ULL,  0x2112080446200010ULL, 0x1080820820060210ULL, 0x3c0808410220200ULL,
            0x4050404440404ULL,    0x21001420088ULL,       0x24d0080801082102ULL, 0x1020a0a020400ULL,
            0x40308200402ULL,      0x4011002100800ULL,     0x401484104104005ULL,  0x801010402020200ULL,
            0x400210c3880100ULL,   0x404022024108200ULL,   0x810018200204102ULL,  0x4002801a02003ULL,
            0x85040820080400ULL,   0x810102c808880400ULL,  0xe900410884800ULL,    0x8002020480840102ULL,
            0x220200865090201ULL,  0x2010100a02021202ULL,  0x152048408022401ULL,  0x20080002081110ULL,
            0x4001001021004000ULL, 0x800040400a011002ULL,  0xe4004081011002ULL,   0x1c004001012080ULL,
            0x8004200962a00220ULL, 0x8422100208500202ULL,  0x2000402200300c08ULL, 0x8646020080080080ULL,
            0x80020a0200100808ULL, 0x2010004880111000ULL,  0x623000a080011400ULL, 0x42008c0340209202ULL,
            0x209188240001000ULL,  0x400408a884001800ULL,  0x110400a6080400ULL,   0x1840060a44020800ULL,
            0x90080104000041ULL,   0x201011000808101ULL,   0x1a2208080504f080ULL, 0x8012020600211212ULL,
            0x500861011240000ULL,  0x180806108200800ULL,   0x4000020e01040044ULL, 0x300000261044000aULL,
            0x802241102020002ULL,  0x20906061210001ULL,    0x5a84841004010310ULL, 0x4010801011c04ULL,
            0xa010109502200ULL,    0x4a02012000ULL,         0x500201010098b028ULL, 0x8040002811040900ULL,
            0x28000010020204ULL,   0x6000020202d0240ULL,   0x8918844842082200ULL, 0x4010011029020020ULL
        };
        
        int rookShifts[64] = { //Rook shift amounts [64] (64 - number of relevant occupancy bits)
            52, 53, 53, 53, 53, 53, 53, 52,
            53, 54, 54, 54, 54, 54, 54, 53,
            53, 54, 54, 54, 54, 54, 54, 53,
            53, 54, 54, 54, 54, 54, 54, 53,
            53, 54, 54, 54, 54, 54, 54, 53,
            53, 54, 54, 54, 54, 54, 54, 53,
            53, 54, 54, 54, 54, 54, 54, 53,
            52, 53, 53, 53, 53, 53, 53, 52
        };

        int bishopShifts[64] = { //Bishop shift amounts [64] (64 - number of relevant occupancy bits)
            58, 59, 59, 59, 59, 59, 59, 58,
            59, 59, 59, 59, 59, 59, 59, 59,
            59, 59, 57, 57, 57, 57, 59, 59,
            59, 59, 57, 55, 55, 57, 59, 59,
            59, 59, 57, 55, 55, 57, 59, 59,
            59, 59, 57, 57, 57, 57, 59, 59,
            59, 59, 59, 59, 59, 59, 59, 59,
            58, 59, 59, 59, 59, 59, 59, 58
        };
    
        // ========================= //
        // | Board class functions | //
        // ========================= //

        void loadFEN(const std::string& fen) { //yes this was coded with claude since I also hate handling inputs and dealing with strings in c++ if it where python I code code this but its not :|
            wPawns = wKnight = wBishop = wRook = wQueen = wKing = 0;
            bPawns = bKnight = bBishop = bRook = bQueen = bKing = 0;
            for (int i = 0; i < 64; i++) mailbox[i] = 0;
            wkscr = wqscr = bkscr = bqscr = false;
            enpassant_square = -1;
            halfmove_clock = 0;
            HistoryIndex = 0;

            std::string parts[6];
            int partIndex = 0;
            for (char c : fen) {
                if (c == ' ') { partIndex++; if (partIndex >= 6) break; }
                else parts[partIndex] += c;
            }

            int sq = 56; 
            for (char c : parts[0]) {
                if (c == '/') {
                    sq -= 16;
                } else if (c >= '1' && c <= '8') {
                    sq += (c - '0');
                } else {
                    int piece = 0;
                    switch(c) {
                        case 'P': piece =  1; break;
                        case 'p': piece = -1; break;
                        case 'N': piece =  2; break;
                        case 'n': piece = -2; break;
                        case 'B': piece =  3; break;
                        case 'b': piece = -3; break;
                        case 'R': piece =  4; break;
                        case 'r': piece = -4; break;
                        case 'Q': piece =  5; break;
                        case 'q': piece = -5; break;
                        case 'K': piece =  6; break;
                        case 'k': piece = -6; break;
                    }
                    placePiece(sq, piece);
                    sq++;
                }
            }

            side = (parts[1] == "w") ? 0 : 1;

            for (char c : parts[2]) {
                switch(c) {
                    case 'K': wkscr = true; break;
                    case 'Q': wqscr = true; break;
                    case 'k': bkscr = true; break;
                    case 'q': bqscr = true; break;
                    case '-': break;
                }
            }

            if (parts[3] != "-") {
                int file = parts[3][0] - 'a'; // 'a'=0 ... 'h'=7
                int rank = parts[3][1] - '1'; // '1'=0 ... '8'=7
                enpassant_square = rank * 8 + file;
            }

            halfmove_clock = std::stoi(parts[4]);
        }

        void makeMove(uint32_t move) {
            if (HistoryIndex >= 4096) { std::cerr << "History overflow\n"; return; }

            stateHistory[HistoryIndex].enpassant_square = enpassant_square;
            stateHistory[HistoryIndex].halfmove_clock = halfmove_clock;
            stateHistory[HistoryIndex].wkscr = wkscr;
            stateHistory[HistoryIndex].wqscr = wqscr;
            stateHistory[HistoryIndex].bkscr = bkscr;
            stateHistory[HistoryIndex].bqscr = bqscr;
            moveHistory[HistoryIndex] = move;
            HistoryIndex++;

            int from      = getFrom(move);
            int to        = getTo(move);
            int piece     = getPiece(move);
            int captured  = getCaptured(move);
            int promotion = getPromotion(move);
            bool enPassantMove = getEnPassant(move);
            bool castleMove = getCastle(move);

            if (piece == 1 || piece == -1 || captured != 0) { halfmove_clock = 0;} 
            else {halfmove_clock++;}
 
            removePiece(from, piece);

            if (captured != 0 && !enPassantMove) {
                removePiece(to, captured);
            }

            if (enPassantMove) {
                if (piece == 1) removePiece(to - 8, -1);
                else removePiece(to + 8, 1);
            }
            
            if (promotion != 0) placePiece(to, promotion);
            else placePiece(to, piece);
            
            if (castleMove) {
                if (piece == 6) {
                    if (to == 6) { removePiece(7, 4);  placePiece(5, 4);  } // kingside
                    else { removePiece(0, 4);  placePiece(3, 4);  } // queenside
                } else {
                    if (to == 62) { removePiece(63, -4); placePiece(61, -4); } // kingside
                    else { removePiece(56, -4); placePiece(59, -4); } // queenside
                }
            }   

            if (from == 4)  { wkscr = false; wqscr = false; } // white king moved
            if (from == 60) { bkscr = false; bqscr = false; } // black king moved
            if (from == 0  || to == 0)  wqscr = false;   // a1 rook
            if (from == 7  || to == 7)  wkscr = false;   // h1 rook
            if (from == 56 || to == 56) bqscr = false;   // a8 rook
            if (from == 63 || to == 63) bkscr = false;   // h8 rook

            enpassant_square = -1;
            if ((piece == 1) && (to - from == 16)) enpassant_square = from + 8; 
            else if ((piece == -1) && (from - to == 16)) enpassant_square = from - 8; 

            side ^= 1;
        }

        void unmakeMove() {
            if (HistoryIndex <= 0) { std::cerr << "History underflow\n"; return; }

            HistoryIndex--;
            uint32_t move = moveHistory[HistoryIndex];

            int from      = getFrom(move);
            int to        = getTo(move);
            int piece     = getPiece(move);
            int captured  = getCaptured(move);
            int promotion = getPromotion(move);

            enpassant_square = stateHistory[HistoryIndex].enpassant_square;
            halfmove_clock   = stateHistory[HistoryIndex].halfmove_clock;
            wkscr = stateHistory[HistoryIndex].wkscr;
            wqscr = stateHistory[HistoryIndex].wqscr;
            bkscr = stateHistory[HistoryIndex].bkscr;
            bqscr = stateHistory[HistoryIndex].bqscr;

            side ^= 1;

            if (promotion != 0) {
                removePiece(to, promotion);
                placePiece(from, piece);
            } else {
                removePiece(to, piece);
                placePiece(from, piece);
            }

            if (captured != 0 && !getEnPassant(move)) {
                placePiece(to, captured);
            }

            if (getEnPassant(move)) {
                if (piece == 1) placePiece(to - 8, -1);
                else            placePiece(to + 8,  1);
            }

            if (getCastle(move)) {
                if (piece == 6) {
                    if (to == 6) { removePiece(5, 4);  placePiece(7, 4);  }
                    else { removePiece(3, 4);  placePiece(0, 4);  }
                } else {
                    if (to == 62) { removePiece(61, -4); placePiece(63, -4); }
                    else { removePiece(59, -4); placePiece(56, -4); }
                }
            }
        }

        bool isInCheck(int side) {
            if (!wKing) { std::cerr << "White king missing\n"; return false; }
            if (!bKing) { std::cerr << "White king missing\n"; return false; }
            if (side == 0) { return isAttacked(__builtin_ctzll(wKing), side); }
            else if (side == 1) { return isAttacked(__builtin_ctzll(bKing), side); }
            else { std::cerr << "King is in check returned neither in check or not in check!\n"; return false; }
        }

        bool isAttacked(int sq, int side) {
            uint64_t occ = getOccupancy();
            if (side == 0) {
                uint64_t rook = getrookAttacks(sq, occ);
                uint64_t bishop = getbishopAttacks(sq, occ);
                if ((((rook) | (bishop)) & bQueen) > 0) { return true; }
                else if (((rook) & bRook) > 0) { return true; }
                else if (((bishop) & bBishop) > 0) { return true; }
                else if (((knightAttacks[sq]) & bKnight) > 0) { return true; }
                else if (((pawnAttacksW[sq]) & bPawns) > 0) { return true; }
                else if (((kingAttacks[sq]) & bKing) > 0) { return true; }
            }
            else if (side == 1) {
                uint64_t rook = getrookAttacks(sq, occ);
                uint64_t bishop = getbishopAttacks(sq, occ);
                if ((((rook) | (bishop)) & wQueen) > 0) { return true; }
                else if (((rook) & wRook) > 0) { return true; }
                else if (((bishop) & wBishop) > 0) { return true; }
                else if (((knightAttacks[sq]) & wKnight) > 0) { return true; }
                else if (((pawnAttacksB[sq]) & wPawns) > 0) { return true; }
                else if (((kingAttacks[sq]) & wKing) > 0) { return true; }
            }
            else { std::cerr << "Error in isAttacked() side is " << side << "\n";}
            return false;
        }

        void LongPrint(uint64_t board) { //dont use if running in uci mode
            std::cout << "------------------\n";
            for (int row = 7; row >= 0; row--) {
                for (int col = 0; col < 8; col++) {
                    int sq = row * 8 + col;
                    std::cout << ((board >> sq) & 1) << " ";
                }
                std::cout << "\n";
            }
            std::cout << "\n";
        }

        void initBoard() { 
            if (initBoardRan) {
                std::cerr << "initBoard already ran\n";
                return;
            }
            initBoardRan = true;
            uint64_t temp_val = 1;
            //Pawn White
            for (int i = 0; i < 64; i++ ) {
                temp_val = 1ULL << i;
                if(i >= 56) {pawnAttacksW[i] = 0;}
                else if(i % 8 == 0 || i % 8 == 7) { 
                    if(i % 8 == 0) { pawnAttacksW[i] = temp_val << 9; }
                    else if(i % 8 == 7) { pawnAttacksW[i] = temp_val << 7; }
                } 
                else{ pawnAttacksW[i] = (temp_val << 7 | temp_val << 9); } 
            }
            //Pawn Black
            for (int i = 0; i < 64; i++) {
                temp_val = 1ULL << i;
                if(i <= 7) {pawnAttacksB[i] = 0;}
                else if(i % 8 == 0 || i % 8 == 7) { 
                    if(i % 8 == 0) { pawnAttacksB[i] = temp_val >> 7; }
                    else if(i % 8 == 7) { pawnAttacksB[i] = temp_val >> 9; }
                } 
                else{ pawnAttacksB[i] = (temp_val >> 7 | temp_val >> 9); } 
            }
            //King
            for (int i = 0; i < 64; i++) {
                temp_val = 1ULL << i;
                if (i % 8 == 0 || i % 8 == 7 || i <= 7 || i >= 56) {
                    if (i % 8 == 0 ) {
                        if (i == 7) { kingAttacks[i] = temp_val << 1 | (temp_val << 8 | temp_val << 9); }
                        else if (i == 56) { kingAttacks[i] = temp_val << 1 | (temp_val >> 7 | temp_val >> 8); }
                        else { kingAttacks[i] = temp_val << 1 | ((temp_val << 8 | temp_val << 9) | (temp_val >> 7 | temp_val >> 8)); }
                    }
                    else if (i % 8 == 7) {
                        if (i == 56) { kingAttacks[i] = temp_val >> 1 | (temp_val >> 8 | temp_val >> 9); }
                        else if (i == 7) { kingAttacks[i] = temp_val >> 1 | (temp_val << 7 | temp_val << 8); }
                        else { kingAttacks[i] = temp_val >> 1 | ((temp_val << 7 | temp_val << 8) | (temp_val >> 8 | temp_val >> 9)); }
                    }
                    else if (i >= 1 && i <= 7 || i >= 56 && i <= 63) { 
                        if (i >= 1 && i <= 7 ) { kingAttacks[i] = (temp_val << 1 | temp_val >> 1) | (temp_val << 7 | temp_val << 8 | temp_val << 9); }
                        else { kingAttacks[i] = (temp_val << 1 | temp_val >> 1) | (temp_val >> 7 | temp_val >> 8 | temp_val >> 9); }
                    }
                }
                else{ kingAttacks[i] = (temp_val >> 1 | temp_val << 1) | ((temp_val << 7 | temp_val << 8 | temp_val << 9) | (temp_val >> 7 | temp_val >> 8 | temp_val >> 9)); }
            }
            //Knight
            for (int sq = 0; sq < 64; sq++) {
                knightAttacks[sq] = 0;
                int r = sq / 8;
                int f = sq % 8;

                int moves[8][2] = {
                    {2, 1}, {2, -1}, {-2, 1}, {-2, -1},
                    {1, 2}, {1, -2}, {-1, 2}, {-1, -2}
                };

                for (int i = 0; i < 8; i++) {
                    int tr = r + moves[i][0];
                    int tf = f + moves[i][1];
                    if (tr >= 0 && tr <= 7 && tf >= 0 && tf <= 7) {
                        knightAttacks[sq] |= (1ULL << (tr * 8 + tf));
                    }
                }
            }

            //Rook, Bishops Masks
            for (int i = 0; i < 64; i++) { //Rooks
                int r = i / 8;
                int f = i % 8;  
                uint64_t temp_mask = 0;
                for (int up    = r + 1; up <= 6;   up++)   temp_mask |= 1ULL << (up * 8 + f);
                for (int down  = r - 1; down >= 1; down--) temp_mask |= 1ULL << (down * 8 + f);
                for (int right = f + 1; right <= 6; right++) temp_mask |= 1ULL << (r * 8 + right);
                for (int left  = f - 1; left >= 1;  left--) temp_mask |= 1ULL << (r * 8 + left);
                rookMasks[i] = temp_mask;
            }
            for (int i = 0; i < 64; i++) { //Bishops
                int r = i / 8;
                int f = i % 8;
                uint64_t temp_mask = 0;
                for (int tr = r + 1, tf = f + 1; tr <= 6 && tf <= 6; tr++, tf++) temp_mask |= 1ULL << (tr * 8 + tf); 
                for (int tr = r + 1, tf = f - 1; tr <= 6 && tf >= 1; tr++, tf--) temp_mask |= 1ULL << (tr * 8 + tf); 
                for (int tr = r - 1, tf = f + 1; tr >= 1 && tf <= 6; tr--, tf++) temp_mask |= 1ULL << (tr * 8 + tf);
                for (int tr = r - 1, tf = f - 1; tr >= 1 && tf >= 1; tr--, tf--) temp_mask |= 1ULL << (tr * 8 + tf);
                bishopMasks[i] = temp_mask;
            }

            rookOffsets[0] = 0;
            bishopOffsets[0] = 0;
            for (int i = 1; i < 64; i++) { rookOffsets[i] = rookOffsets[i - 1] + (1ULL << (64 - rookShifts[i - 1])); }
            for (int i = 1; i < 64; i++) { bishopOffsets[i] = bishopOffsets[i - 1] + (1ULL << (64 - bishopShifts[i - 1])); }

            fillBishopAttacks();
            fillRookAttacks();
        }

        uint64_t getOccupancy() {
            return (wPawns | wBishop | wKnight | wRook | wQueen | wKing) | (bPawns | bBishop | bKnight | bRook | bQueen | bKing);
        }

        uint64_t getOccupancyWhite() {
            return wPawns | wBishop | wKnight | wRook | wQueen | wKing;
        }

        uint64_t getOccupancyBlack() {
            return bPawns | bBishop | bKnight | bRook | bQueen | bKing;
        }

        uint64_t getrookAttacks(int sq, uint64_t occ) {
            occ &= rookMasks[sq];
            occ *= rookMagics[sq];
            occ >>= rookShifts[sq];
            return rookAttacks[rookOffsets[sq] + occ];
        }

        uint64_t getbishopAttacks(int sq, uint64_t occ) {
            occ &= bishopMasks[sq];
            occ *= bishopMagics[sq];
            occ >>= bishopShifts[sq];
            return bishopAttacks[bishopOffsets[sq] + occ];
        }

        int getPieceAt(int sq) {
            uint64_t bit = 1ULL << sq;
            if (wPawns & bit)  return 1;
            if (wKnight & bit) return 2;
            if (wBishop & bit) return 3;
            if (wRook & bit)   return 4;
            if (wQueen & bit)  return 5;
            if (wKing & bit)   return 6;
            if (bPawns & bit)  return -1;
            if (bKnight & bit) return -2;
            if (bBishop & bit) return -3;
            if (bRook & bit)   return -4;
            if (bQueen & bit)  return -5;
            if (bKing & bit)   return -6;
            return 0;
        }

    private:
        void removePiece(int sq, int piece) {
            mailbox[sq] = 0;
            switch(piece) {
                case  1: wPawns  &= ~(1ULL << sq); break;
                case -1: bPawns  &= ~(1ULL << sq); break;
                case  2: wKnight &= ~(1ULL << sq); break;
                case -2: bKnight &= ~(1ULL << sq); break;
                case  3: wBishop &= ~(1ULL << sq); break;
                case -3: bBishop &= ~(1ULL << sq); break;
                case  4: wRook   &= ~(1ULL << sq); break;
                case -4: bRook   &= ~(1ULL << sq); break;
                case  5: wQueen  &= ~(1ULL << sq); break;
                case -5: bQueen  &= ~(1ULL << sq); break;
                case  6: wKing   &= ~(1ULL << sq); break;
                case -6: bKing   &= ~(1ULL << sq); break;
            }
        }

        void placePiece(int sq, int piece) {
            mailbox[sq] = piece;
            switch(piece) {
                case  1: wPawns  |= (1ULL << sq); break;
                case -1: bPawns  |= (1ULL << sq); break;
                case  2: wKnight |= (1ULL << sq); break;
                case -2: bKnight |= (1ULL << sq); break;
                case  3: wBishop |= (1ULL << sq); break;
                case -3: bBishop |= (1ULL << sq); break;
                case  4: wRook   |= (1ULL << sq); break;
                case -4: bRook   |= (1ULL << sq); break;
                case  5: wQueen  |= (1ULL << sq); break;
                case -5: bQueen  |= (1ULL << sq); break;
                case  6: wKing   |= (1ULL << sq); break;
                case -6: bKing   |= (1ULL << sq); break;
            }
        }

        uint64_t indexToOccupancy(int index, int numBits, uint64_t mask) {
            uint64_t occupancy = 0;
            for (int i = 0; i < numBits; i++) {
                int bitPos = __builtin_ctzll(mask);
                mask &= mask - 1;
                if (index & (1 << i)) { occupancy |= (1ULL << bitPos); }
            }
            return occupancy;
        }
        
        uint64_t computeRookAttacks(int sq, uint64_t occ) {
            uint64_t attacks = 0;
            int r = sq / 8;
            int f = sq % 8;

            for (int i = r + 1; i <= 7; i++) { attacks |= (1ULL << (i * 8 + f)); if (occ & (1ULL << (i * 8 + f))) break; }
            for (int i = r - 1; i >= 0; i--) { attacks |= (1ULL << (i * 8 + f)); if (occ & (1ULL << (i * 8 + f))) break; }
            for (int j = f + 1; j <= 7; j++) { attacks |= (1ULL << (r * 8 + j)); if (occ & (1ULL << (r * 8 + j))) break; }
            for (int j = f - 1; j >= 0; j--) { attacks |= (1ULL << (r * 8 + j)); if (occ & (1ULL << (r * 8 + j))) break; }

            return attacks;
        }

        uint64_t computeBishopAttacks(int sq, uint64_t occ) {
            uint64_t attacks = 0;
            int r = sq / 8;
            int f = sq % 8;

            for (int tr = r + 1, tf = f + 1; tr <= 7 && tf <= 7; tr++, tf++) { attacks |= 1ULL << (tr * 8 + tf); if (occ & (1ULL << (tr * 8 + tf))) break; }
            for (int tr = r + 1, tf = f - 1; tr <= 7 && tf >= 0; tr++, tf--) { attacks |= 1ULL << (tr * 8 + tf); if (occ & (1ULL << (tr * 8 + tf))) break; }
            for (int tr = r - 1, tf = f + 1; tr >= 0 && tf <= 7; tr--, tf++) { attacks |= 1ULL << (tr * 8 + tf); if (occ & (1ULL << (tr * 8 + tf))) break; }
            for (int tr = r - 1, tf = f - 1; tr >= 0 && tf >= 0; tr--, tf--) { attacks |= 1ULL << (tr * 8 + tf); if (occ & (1ULL << (tr * 8 + tf))) break; }

            return attacks;
        }

        void fillRookAttacks() {
            for (int sq = 0; sq < 64; sq++) {
                int numBits = 64 - rookShifts[sq];
                int numConfigs = 1 << numBits;

                for (int index = 0; index < numConfigs; index++) {
                    uint64_t occ = indexToOccupancy(index, numBits, rookMasks[sq]);
                    uint64_t magicIndex = (occ * rookMagics[sq]) >> rookShifts[sq];
                    uint64_t attacks = computeRookAttacks(sq, occ);
                    rookAttacks[rookOffsets[sq] + magicIndex] = attacks;
                }
            }
        }

        void fillBishopAttacks() {
            for (int sq = 0; sq < 64; sq++) {
                int numBits = 64 - bishopShifts[sq];
                int numConfigs = 1 << numBits;

                for (int index = 0; index < numConfigs; index++) {
                    uint64_t occ = indexToOccupancy(index, numBits, bishopMasks[sq]);
                    uint64_t magicIndex = (occ * bishopMagics[sq]) >> bishopShifts[sq];
                    uint64_t attacks = computeBishopAttacks(sq, occ);
                    bishopAttacks[bishopOffsets[sq] + magicIndex] = attacks;
                }
            }
        }
};

class moveGen {
    public:
        Board& board;
        moveGen(Board& b) : board(b) {}

        uint64_t perft(int depth) {
            if (depth == 0) return 1;

            uint32_t moves[256];
            int count = generateMoves(moves);
            uint64_t nodes = 0;

            for (int i = 0; i < count; i++) {
                board.makeMove(moves[i]);

                int movedSide = board.side ^ 1;
                if (!board.isInCheck(movedSide)) {
                    nodes += perft(depth - 1);
                }

                board.unmakeMove();
            }

            return nodes;
        }
        
        void perftDivide(int depth) {
            uint32_t moves[256];
            int count = generateMoves(moves);
            uint64_t total = 0;

            for (int i = 0; i < count; i++) {
                board.makeMove(moves[i]);

                int movedSide = board.side ^ 1;
                uint64_t nodes = 0;
                if (!board.isInCheck(movedSide)) {
                    nodes = perft(depth - 1);
                }

                board.unmakeMove();

                if (nodes > 0) {
                    int from = getFrom(moves[i]);
                    int to   = getTo(moves[i]);
                    char fromFile = 'a' + (from % 8);
                    char fromRank = '1' + (from / 8);
                    char toFile   = 'a' + (to % 8);
                    char toRank   = '1' + (to / 8);
                    std::cout << fromFile << fromRank << toFile << toRank << ": " << nodes << "\n";
                    total += nodes;
                }
            }
            std::cout << "Total: " << total << "\n";
        }
        
        int generateMoves(uint32_t* moves) {
            int nodes = 0;
            int side = board.side;

            uint64_t occ = board.getOccupancy();
            uint64_t ownPieces = (side == 0) ? board.getOccupancyWhite() : board.getOccupancyBlack();   //just found out I can do stuff like this in one line c++ is crazy
            uint64_t enemyPieces = (side == 0) ? board.getOccupancyBlack() : board.getOccupancyWhite();

            uint64_t pawns = (side == 0) ? board.wPawns : board.bPawns;
            uint64_t bishops = (side == 0) ? board.wBishop : board.bBishop;
            uint64_t knights = (side == 0) ? board.wKnight : board.bKnight;
            uint64_t rooks = (side == 0) ? board.wRook : board.bRook;
            uint64_t queens = (side == 0) ? board.wQueen : board.bQueen;
            uint64_t king = (side == 0) ? board.wKing : board.bKing;

            while (pawns > 0) {
                int from = __builtin_ctzll(pawns);
                pawns &= pawns - 1;
                uint64_t bit = 1ULL << from;

                if (side == 0) {
                    // single push
                    if (!((occ) & (bit << 8))) {
                        // promotion
                        if (from >= 48) {
                            moves[nodes++] = encodeMove(from, from + 8, 1, 0,  5, false, false); // queen
                            moves[nodes++] = encodeMove(from, from + 8, 1, 0,  4, false, false); // rook
                            moves[nodes++] = encodeMove(from, from + 8, 1, 0,  3, false, false); // bishop
                            moves[nodes++] = encodeMove(from, from + 8, 1, 0,  2, false, false); // knight
                        } else {
                            moves[nodes++] = encodeMove(from, from + 8, 1, 0, 0, false, false);
                        }
                        // double push (only possible if single push was clear)
                        if (from >= 8 && from <= 15 && !(occ & (bit << 16))) {
                            moves[nodes++] = encodeMove(from, from + 16, 1, 0, 0, false, false);
                        }
                    }

                    // captures
                    uint64_t captures = board.pawnAttacksW[from] & enemyPieces;
                    while (captures) {
                        int to = __builtin_ctzll(captures);
                        captures &= captures - 1;
                        if (from >= 48) { // promotion capture
                            moves[nodes++] = encodeMove(from, to, 1, board.getPieceAt(to),  5, false, false);
                            moves[nodes++] = encodeMove(from, to, 1, board.getPieceAt(to),  4, false, false);
                            moves[nodes++] = encodeMove(from, to, 1, board.getPieceAt(to),  3, false, false);
                            moves[nodes++] = encodeMove(from, to, 1, board.getPieceAt(to),  2, false, false);
                        } else {
                            moves[nodes++] = encodeMove(from, to, 1, board.getPieceAt(to), 0, false, false);
                        }
                    }

                    // en passant
                    if (board.enpassant_square != -1 && (board.pawnAttacksW[from] & (1ULL << board.enpassant_square))) {
                        moves[nodes++] = encodeMove(from, board.enpassant_square, 1, -1, 0, true, false);
                    }

                } else {
                    // single push
                    if (!(occ & (bit >> 8))) {
                        // promotion
                        if (from >= 8 && from <= 15) {
                            moves[nodes++] = encodeMove(from, from - 8, -1, 0, -5, false, false);
                            moves[nodes++] = encodeMove(from, from - 8, -1, 0, -4, false, false);
                            moves[nodes++] = encodeMove(from, from - 8, -1, 0, -3, false, false);
                            moves[nodes++] = encodeMove(from, from - 8, -1, 0, -2, false, false);
                        } else {
                            moves[nodes++] = encodeMove(from, from - 8, -1, 0, 0, false, false);
                        }
                        // double push
                        if (from >= 48 && from <= 55 && !(occ & (bit >> 16))) {
                            moves[nodes++] = encodeMove(from, from - 16, -1, 0, 0, false, false);
                        }
                    }

                    // captures
                    uint64_t captures = board.pawnAttacksB[from] & enemyPieces;
                    while (captures) {
                        int to = __builtin_ctzll(captures);
                        captures &= captures - 1;
                        if (from >= 8 && from <= 15) { // promotion capture
                            moves[nodes++] = encodeMove(from, to, -1, board.getPieceAt(to), -5, false, false);
                            moves[nodes++] = encodeMove(from, to, -1, board.getPieceAt(to), -4, false, false);
                            moves[nodes++] = encodeMove(from, to, -1, board.getPieceAt(to), -3, false, false);
                            moves[nodes++] = encodeMove(from, to, -1, board.getPieceAt(to), -2, false, false);
                        } else {
                            moves[nodes++] = encodeMove(from, to, -1, board.getPieceAt(to), 0, false, false);
                        }
                    }

                    // en passant
                    if (board.enpassant_square != -1 && (board.pawnAttacksB[from] & (1ULL << board.enpassant_square))) {
                        moves[nodes++] = encodeMove(from, board.enpassant_square, -1, 1, 0, true, false);
                    }
                }
            }

            int piece = (side == 0) ? 2 : -2;
            while (knights > 0) {
                int from = __builtin_ctzll(knights);
                knights &= knights - 1;

                uint64_t attacks = board.knightAttacks[from] & ~ownPieces;
                while (attacks) {
                    int to = __builtin_ctzll(attacks);
                    attacks &= attacks - 1;

                    moves[nodes++] = encodeMove(from, to, piece, board.getPieceAt(to), 0, false, false);
                }
            }
        
            piece = (side == 0) ? 3 : -3;
            while (bishops > 0) {
                int from = __builtin_ctzll(bishops);
                bishops &= bishops - 1;

                uint64_t attacks = board.getbishopAttacks(from, occ) & ~ownPieces;
                while (attacks) {
                    int to = __builtin_ctzll(attacks);
                    attacks &= attacks - 1;

                    moves[nodes++] = encodeMove(from, to, piece, board.getPieceAt(to), 0, false, false);
                }
            }

            piece = (side == 0) ? 4 : -4;
            while (rooks > 0) {
                int from = __builtin_ctzll(rooks);
                rooks &= rooks - 1;

                uint64_t attacks = board.getrookAttacks(from, occ) & ~ownPieces;
                while (attacks) {
                    int to = __builtin_ctzll(attacks);
                    attacks &= attacks - 1;

                    moves[nodes++] = encodeMove(from, to, piece, board.getPieceAt(to), 0, false, false);
                }
            }

            piece = (side == 0) ? 5 : -5;
            while (queens > 0) {
                int from = __builtin_ctzll(queens);
                queens &= queens - 1;

                uint64_t attacks = (board.getrookAttacks(from, occ) | board.getbishopAttacks(from, occ)) & ~ownPieces;
                while (attacks) {
                    int to = __builtin_ctzll(attacks);
                    attacks &= attacks - 1;

                    moves[nodes++] = encodeMove(from, to, piece, board.getPieceAt(to), 0, false, false);
                }
            }

            piece = (side == 0) ? 6 : -6;
            while (king > 0) {
                int from = __builtin_ctzll(king);
                king &= king - 1;

                uint64_t attacks = board.kingAttacks[from] & ~ownPieces;
                while (attacks) {
                    int to = __builtin_ctzll(attacks);
                    attacks &= attacks - 1;

                    moves[nodes++] = encodeMove(from, to, piece, board.getPieceAt(to), 0, false, false);
                }
            }
            
            // castling stuff
            if (side == 0) {
                if (board.wkscr && !(occ & (1ULL << 5)) && !(occ & (1ULL << 6))
                        && !board.isAttacked(4, 0) && !board.isAttacked(5, 0) && !board.isAttacked(6, 0))
                    moves[nodes++] = encodeMove(4, 6, 6, 0, 0, false, true);

                if (board.wqscr && !(occ & (1ULL << 3)) && !(occ & (1ULL << 2)) && !(occ & (1ULL << 1))
                        && !board.isAttacked(4, 0) && !board.isAttacked(3, 0) && !board.isAttacked(2, 0))
                    moves[nodes++] = encodeMove(4, 2, 6, 0, 0, false, true);
            } 
            else {
                if (board.bkscr && !(occ & (1ULL << 61)) && !(occ & (1ULL << 62))
                        && !board.isAttacked(60, 1) && !board.isAttacked(61, 1) && !board.isAttacked(62, 1))
                    moves[nodes++] = encodeMove(60, 62, -6, 0, 0, false, true);

                if (board.bqscr && !(occ & (1ULL << 59)) && !(occ & (1ULL << 58)) && !(occ & (1ULL << 57))
                        && !board.isAttacked(60, 1) && !board.isAttacked(59, 1) && !board.isAttacked(58, 1))
                    moves[nodes++] = encodeMove(60, 58, -6, 0, 0, false, true);
            }
            
            return nodes;
        }
    private:
    
};

class Search {
    public:

    private:
    
};

class Eval {
    public:

    private:
    
};

class UCI {
    public:
        std::string line;
        void uciUP() {
            while (true) {
                std::getline(std::cin, line);
                if (line == "uci") {
                    std::cout << "id name fih" << std::endl;
                    std::cout << "id author Angelicide" << std::endl;
                    std::cout << "uciok" << std::endl;
                }
                else if (line  == "isready") {
                    std::cout << "readyok" << std::endl;
                }
            }   
        }
};

int main() {
    Board board;
    board.initBoard();
    board.loadFEN("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10 ");

    moveGen mg(board);

    // expected: depth1=20, depth2=400, depth3=8902, depth4=197281
    for (int d = 1; d <= 10; d++) {
        uint64_t result = mg.perft(d);
        std::cout << "Depth " << d << ": " << result << "\n";
    }
    return 0;
}