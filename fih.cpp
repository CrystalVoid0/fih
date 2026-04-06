#include <cstdint>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>


struct Move {
    int from      = 0;
    int to        = 0;
    int piece     = 0;
    int captured  = 0;
    int promotion = 0;
    bool enPassant = false;
    bool castle    = false;
};

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
        Move moveHistory[4096];
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

        void makeMove(Move move) {
            if (HistoryIndex >= 4096) { std::cerr << "History overflow\n"; return; }
            BoardState& currentState = stateHistory[HistoryIndex]; // Create a refence so modifying currentState will modify the stateHistory[]
            currentState.wqscr = wqscr;
            currentState.wkscr = wkscr;
            currentState.bqscr = bqscr;
            currentState.bkscr = bkscr;
            currentState.side = side;

            currentState.enpassant_square = enpassant_square;
            currentState.halfmove_clock = halfmove_clock;

            moveHistory[HistoryIndex] = move;
            HistoryIndex++;

            if (move.piece == 1 || move.piece == -1 || move.captured != 0) { halfmove_clock = 0;} 
            else {halfmove_clock++;}
            
            if (move.piece > 0) {side = 0;} 
            else {side = 1;}
 
            //yes i know its a lot of switch staments :(
            switch(move.piece) { //remove moving piece from its square
                case 1:  wPawns &= ~(1ULL << move.from); break;
                case -1: bPawns &= ~(1ULL << move.from); break;
                case 2:  wKnight &= ~(1ULL << move.from); break;
                case -2: bKnight &= ~(1ULL << move.from); break;
                case 3:  wBishop &= ~(1ULL << move.from); break;
                case -3: bBishop &= ~(1ULL << move.from); break;
                case 4:  wRook &= ~(1ULL << move.from); break;
                case -4: bRook &= ~(1ULL << move.from); break;
                case 5:  wQueen &= ~(1ULL << move.from); break;
                case -5: bQueen &= ~(1ULL << move.from); break;
                case 6:  wKing &= ~(1ULL << move.from); break;
                case -6: bKing &= ~(1ULL << move.from); break;
                case 0:   std::cerr << "Error in switch(move.piece) inside makeMove(), no piece type 0 why the fuk is this shi making a move with no piece?"; break;
                default:  std::cerr << "Error in switch(move.piece) inside makeMove(), function: Error move.piece is not between -6 or 1 value used -> " << move.piece << "\n"; break;
            }
            if (!move.enPassant) { //remove captured piece from square *if any* unless enPassant
                switch(move.captured) { 
                    case 0:  break;
                    case 1:  wPawns &= ~(1ULL << move.to); break;
                    case -1: bPawns &= ~(1ULL << move.to); break;
                    case 2:  wKnight &= ~(1ULL << move.to); break;
                    case -2: bKnight &= ~(1ULL << move.to); break;
                    case 3:  wBishop &= ~(1ULL << move.to); break;
                    case -3: bBishop &= ~(1ULL << move.to); break;
                    case 4:  wRook &= ~(1ULL << move.to); break;
                    case -4: bRook &= ~(1ULL << move.to); break;
                    case 5:  wQueen &= ~(1ULL << move.to); break;
                    case -5: bQueen &= ~(1ULL << move.to); break;
                    case 6:  wKing &= ~(1ULL << move.to); break;
                    case -6: bKing &= ~(1ULL << move.to); break;
                    default: std::cerr << "Uknown piece type in move.captured in makeMove(), value used -> " << move.captured << "\n";
                }
            } else { //remove pawn below enPassant square
                if (move.piece == 1) { bPawns &= ~(1ULL << (move.to - 8)); } 
                else { wPawns &= ~(1ULL << (move.to + 8)); }
            }
            if (!move.promotion) { //add moving piece to its move square
                switch(move.piece) {
                    case 1:  wPawns |= (1ULL << move.to); break;
                    case -1: bPawns |= (1ULL << move.to); break;
                    case 2:  wKnight |= (1ULL << move.to); break;
                    case -2: bKnight |= (1ULL << move.to); break;
                    case 3:  wBishop |= (1ULL << move.to); break;
                    case -3: bBishop |= (1ULL << move.to); break;
                    case 4:  wRook |= (1ULL << move.to); break;
                    case -4: bRook |= (1ULL << move.to); break;
                    case 5:  wQueen |= (1ULL << move.to); break;
                    case -5: bQueen |= (1ULL << move.to); break;
                    case 6:  wKing |= (1ULL << move.to); break;
                    case -6: bKing |= (1ULL << move.to); break;
                    case 0:  std::cerr << "Error in switch(move.piece) no piece type 0 why the fuk is this shi making a move with no piece?"; break;
                    default: std::cerr << "Error in switch(move.piece) inside makeMove() function: Error move.piece is not between -6 or 1 value used -> " << move.piece << "\n"; break;
                }
            } else { //if promotion add promoted piece to destination square
                switch(move.promotion) {
                    case 5:  wQueen |= (1ULL << move.to); break;
                    case -5: bQueen |= (1ULL << move.to); break;
                    case 4:  wRook |= (1ULL << move.to); break;
                    case -4: bRook |= (1ULL << move.to); break;
                    case 3:  wBishop |= (1ULL << move.to); break;
                    case -3: bBishop |= (1ULL << move.to); break;
                    case 2:  wKnight |= (1ULL << move.to); break;
                    case -2: bKnight |= (1ULL << move.to); break;
                    default: std::cerr << "Error in makeMove switch(move.promotion) unknown promotion value! {" << move.promotion << "}\n";
                }
            }

            //deal with castling
            if (move.castle == true) {
                if (side == 0) {
                    if (move.to - move.from > 0) {
                        wRook &= ~(1ULL << 7);
                        wRook |= (1ULL << 5);
                    }
                    else {
                        wRook &= ~(1ULL << 0);
                        wRook |= (1ULL << 3);
                    }
                    wqscr = false;
                    wkscr = false;
                }
                else {
                    if (move.to - move.from > 0) {
                        bRook &= ~(1ULL << 63);
                        bRook |= (1ULL << 61);
                    }
                    else {
                        bRook &= ~(1ULL << 56);
                        bRook |= (1ULL << 59);
                    }
                    bqscr = false;
                    bkscr = false;
                }
            }
            //update castling rules if rook or king move or rook taken
            if (move.from == 4)  { wkscr = false; wqscr = false; } // white king moved
            if (move.from == 60) { bkscr = false; bqscr = false; } // black king moved
            if (move.from == 0  || move.to == 0)  wqscr = false;   // a1 rook
            if (move.from == 7  || move.to == 7)  wkscr = false;   // h1 rook
            if (move.from == 56 || move.to == 56) bqscr = false;   // a8 rook
            if (move.from == 63 || move.to == 63) bkscr = false;   // h8 rook

            //update enpassant rules
            enpassant_square = -1;
            if ((move.piece == 1) && (move.to - move.from == 16)) { enpassant_square = move.from + 8; }
            else if ((move.piece == -1) && (move.from - move.to == 16)) { enpassant_square = move.from - 8; }

            if (side == 0) {side = 1;} 
            else if (side == 1) {side = 0;}
            else {std::cerr << "side variable is neither 1 or 0! \n";}
        }

        void unmakeMove() {
            if (HistoryIndex <= 0) { std::cerr << "History underflow\n"; return; }

            HistoryIndex--;
            Move& move = moveHistory[HistoryIndex];
            BoardState& previousState = stateHistory[HistoryIndex];

            wqscr = previousState.wqscr;
            wkscr = previousState.wkscr;
            bqscr = previousState.bqscr;
            bkscr = previousState.bkscr;
            side = previousState.side;

            halfmove_clock = previousState.halfmove_clock;
            enpassant_square = previousState.enpassant_square;

            
            if (move.promotion != 0) { //undo pawn promotion
                switch(move.promotion) {
                    case 5:  wQueen &= ~(1ULL << move.to); break;
                    case -5: bQueen &= ~(1ULL << move.to); break;
                    case 4:  wRook &= ~(1ULL << move.to); break;
                    case -4: bRook &= ~(1ULL << move.to); break;
                    case 3:  wBishop &= ~(1ULL << move.to); break;
                    case -3: bBishop &= ~(1ULL << move.to); break;
                    case 2:  wKnight &= ~(1ULL << move.to); break;
                    case -2: bKnight &= ~(1ULL << move.to); break;      
                }
                if (side == 0) wPawns |= (1ULL << move.from);
                else bPawns |= (1ULL << move.from);
            }
            else { //remove the piece from its destination square
                switch(move.piece) {
                    case 1:  wPawns &= ~(1ULL << move.to); break;
                    case -1: bPawns &= ~(1ULL << move.to); break;
                    case 2:  wKnight &= ~(1ULL << move.to); break;
                    case -2: bKnight &= ~(1ULL << move.to); break;
                    case 3:  wBishop &= ~(1ULL << move.to); break;
                    case -3: bBishop &= ~(1ULL << move.to); break;
                    case 4:  wRook &= ~(1ULL << move.to); break;
                    case -4: bRook &= ~(1ULL << move.to); break;
                    case 5:  wQueen &= ~(1ULL << move.to); break;
                    case -5: bQueen &= ~(1ULL << move.to); break;
                    case 6:  wKing &= ~(1ULL << move.to); break;
                    case -6: bKing &= ~(1ULL << move.to); break;
                    case 0:  std::cerr << "Error in switch(move.piece) in unmakeMove() no piece type 0 why the fuk is this shi making a move with no piece?"; break;
                    default: std::cerr << "Error in switch(move.piece) inside unmakeMove() function: Error move.piece is not between -6 or 1 value used -> " << move.piece << "\n"; break;
                }
            }
            switch(move.captured) { //add back any captured piece to the board
                case 0: break;
                case 1:  wPawns |= (1ULL << move.to); break;
                case -1: bPawns |= (1ULL << move.to); break;
                case 2:  wKnight |= (1ULL << move.to); break;
                case -2: bKnight |= (1ULL << move.to); break;
                case 3:  wBishop |= (1ULL << move.to); break;
                case -3: bBishop |= (1ULL << move.to); break;
                case 4:  wRook |= (1ULL << move.to); break;
                case -4: bRook |= (1ULL << move.to); break;
                case 5:  wQueen |= (1ULL << move.to); break;
                case -5: bQueen |= (1ULL << move.to); break;
                case 6:  wKing |= (1ULL << move.to); std::cerr << "tf why was the king captured?\n"; break; //I actually have no clue why this is here but might as well keep it just incase for now
                case -6: bKing |= (1ULL << move.to); std::cerr << "tf why was the king captured?\n"; break;
                default: std::cerr << "Uknown piece type in move.captured in unmakeMove(), value used -> " << move.captured << "\n";
            }                
            switch(move.piece) { //bring the moved piece back to its orginal square
                case 1:  wPawns |= (1ULL << move.from); break;
                case -1: bPawns |= (1ULL << move.from); break;
                case 2:  wKnight |= (1ULL << move.from); break;
                case -2: bKnight |= (1ULL << move.from); break;
                case 3:  wBishop |= (1ULL << move.from); break;
                case -3: bBishop |= (1ULL << move.from); break;
                case 4:  wRook |= (1ULL << move.from); break;
                case -4: bRook |= (1ULL << move.from); break;
                case 5:  wQueen |= (1ULL << move.from); break;
                case -5: bQueen |= (1ULL << move.from); break;
                case 6:  wKing |= (1ULL << move.from); break;
                case -6: bKing |= (1ULL << move.from); break;
                case 0:  std::cerr << "Error in switch(move.piece) inside unmakeMove() no piece type 0 why the fuk is this shi making a move with no piece?"; break;
                default: std::cerr << "Error in switch(move.piece) inside unmakeMove() function: Error move.piece is not between -6 or 1 value used -> " << move.piece << "\n"; break;   
            }
            if (move.enPassant) { //add back the pawn if en passant happened
                if (move.piece == 1) { bPawns |= (1ULL << (move.to - 8)); } 
                else { wPawns |= (1ULL << (move.to + 8)); }
            }
            if (move.castle == true) { //undo castling if it happened
                if (side == 0) {
                    if (move.to - move.from > 0) {
                        wRook |= (1ULL << 7);
                        wRook &= ~(1ULL << 5);
                    }
                    else {
                        wRook |= (1ULL << 0);
                        wRook &= ~(1ULL << 3);
                    }
                } else {
                    if (move.to - move.from > 0) {
                        bRook |= (1ULL << 63);
                        bRook &= ~(1ULL << 61);
                    }
                    else {
                        bRook |= (1ULL << 56);
                        bRook &= ~(1ULL << 59);
                    }
                }
            }
        }

        bool isInCheck(int side) {
            if (side == 0) { return isAttacked(__builtin_ctzll(wKing), side); }
            else if (side == 1) { return isAttacked(__builtin_ctzll(bKing), side); }
            if (!wKing) {
                std::cerr << "White king missing\n";
                return false;
            }
            if (!bKing) {
                std::cerr << "White king missing\n";
                return false;
            }
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

    private:
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
    UCI uci;
    Board board;
    Eval eval;
    moveGen moveGen;
    

    return 0;
}