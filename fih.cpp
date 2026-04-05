#include <cstdint>
#include <iostream>
#include <unordered_map>


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
    int enpassent_square;
    int moving;
    int halfmove_clock;
};

class Board {
    public:
        //Fen game varibles
        int board[8][8] = {};
        int moving; //0 = whites turn to move, 1 = blacks turn to move
        bool wkscr = false; // white king side castling rights
        bool wqscr = false; // white queen side castling rights
        bool bkscr = false; // black king side castling rights
        bool bqscr = false; // black queen side castling rights
        int enpassent_square = -1; //-1 mean no enpassent
        int halfmove_clock = 0; //half move clock i guess?

        //Attack BitBoards
        uint64_t pawnAttacksW[64] = {0};
        uint64_t pawnAttacksB[64] = {0};
        uint64_t knightAttacks[64] = {0};
        uint64_t kingAttacks[64] = {0};

        //Masks
        uint64_t rookMasks[64] = {0};
        uint64_t bishopMasks[64] = {0};

        //Rook and bishops offset bit
        int rookOffsets[64];
        int bishopOffsets[64];

        //Rook, Bishops bitboard look ups
        uint64_t rookAttacks[102400];
        uint64_t bishopAttacks[5248];

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

        //Piece Bitboards
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

        //Move Histoy related variables
        int HistoryIndex = 0;   
        Move moveHistory[512];
        BoardState stateHistory[512];

        //Saftey feature probably will get romved later on
        bool initABBran = false;

        //init function for Board class
        void initBoard() {
            initPieces();
            initAttackBitBoards();
            initSlidingPieceAttackTB();
        }

        bool isInCheck(int moving) {
            int sq = 0;
            uint64_t occ = getOccupancy();
            if (moving == 0) {
                sq = __builtin_ctzll(wKing);
                if ((getrookAttacks(sq, occ) | getbishopAttacks(sq, occ)) & bQueen > 0) { return true; }
                else if ((getrookAttacks(sq, occ)) & bRook > 0) { return true; }
                else if ((getbishopAttacks(sq, occ)) & bBishop > 0) { return true; }
                else if ((knightAttacks[sq] & bKnight) > 0) { return true; }
                else if ((pawnAttacksW[sq] & bPawns) > 0) { return true; }
            }
            else if (moving == 1) {
                sq = __builtin_ctzll(bKing);
                if ((getrookAttacks(sq, occ) | getbishopAttacks(sq, occ)) & wQueen > 0) { return true; }
                else if ((getrookAttacks(sq, occ)) & wRook > 0) { return true; }
                else if ((getbishopAttacks(sq, occ)) & wBishop > 0) { return true; }
                else if ((knightAttacks[sq] & wKnight) > 0) { return true; }
                else if ((pawnAttacksB[sq] & wPawns) > 0) { return true; }
            }
            else { std::cerr << "Error in isInCheck moving is " << moving << "\n";}
            return false;
        }

        bool isAttacked(int sq, int moving) {
            uint64_t occ = getOccupancy();
            if (moving == 0) {
                if ((getrookAttacks(sq, occ) | getbishopAttacks(sq, occ)) & bQueen > 0) { return true; }
                else if ((getrookAttacks(sq, occ)) & bRook > 0) { return true; }
                else if ((getbishopAttacks(sq, occ)) & bBishop > 0) { return true; }
                else if ((knightAttacks[sq] & bKnight) > 0) { return true; }
                else if ((pawnAttacksW[sq] & bPawns) > 0) { return true; }
            }
            else if (moving == 1) {
                if ((getrookAttacks(sq, occ) | getbishopAttacks(sq, occ)) & wQueen > 0) { return true; }
                else if ((getrookAttacks(sq, occ)) & wRook > 0) { return true; }
                else if ((getbishopAttacks(sq, occ)) & wBishop > 0) { return true; }
                else if ((knightAttacks[sq] & wKnight) > 0) { return true; }
                else if ((pawnAttacksB[sq] & wPawns) > 0) { return true; }
            }
            else { std::cerr << "Error in isAttacked() moving is " << moving << "\n";}
            return false;
        }

        void makeMove(Move move) {
            BoardState& currentState = stateHistory[HistoryIndex]; // Create a refence so modifying currentState will modify the stateHistory[]
            currentState.wqscr = wqscr;
            currentState.wkscr = wkscr;
            currentState.bqscr = bqscr;
            currentState.bkscr = bkscr;
            currentState.moving = moving;

            currentState.enpassent_square = enpassent_square;
            currentState.halfmove_clock = halfmove_clock;

            moveHistory[HistoryIndex] = move;
            HistoryIndex++;

            if (move.piece == 1 || move.piece == -1 || move.captured != 0) { halfmove_clock = 0;} 
            else {halfmove_clock++;}

            //remove moving piece from its square
            switch(move.piece) {
                case 1:
                    wPawns &= ~(1ULL << move.from); break;
                case -1:
                    bPawns &= ~(1ULL << move.from); break;
                case 2:
                    wKnight &= ~(1ULL << move.from); break;
                case -2:
                    bKnight &= ~(1ULL << move.from); break;
                case 3:
                    wBishop &= ~(1ULL << move.from); break;
                case -3:
                    bBishop &= ~(1ULL << move.from); break;
                case 4:
                    wRook &= ~(1ULL << move.from); break;
                case -4:
                    bRook &= ~(1ULL << move.from); break;
                case 5:
                    wQueen &= ~(1ULL << move.from); break;
                case -5:
                    bQueen &= ~(1ULL << move.from); break;
                case 6:
                    wKing &= ~(1ULL << move.from); break;
                case -6:
                    bKing &= ~(1ULL << move.from); break;
                case 0:
                    std::cerr << "Error in switch(move.piece) inside makeMove(), no piece type 0 why the fuk is this shi making a move with no piece?"; break;
                default:
                    std::cerr << "Error in switch(move.piece) inside makeMove(), function: Error move.piece is not between -6 or 1 value used -> " << move.piece << "\n";
                    break;
            }
            //removing captured piece from square *if any* 
            switch(move.captured) {
                case 0:
                    break;
                case 1:
                    wPawns &= ~(1ULL << move.to); break;
                case -1:
                    bPawns &= ~(1ULL << move.to); break;
                case 2:
                    wKnight &= ~(1ULL << move.to); break;
                case -2:
                    bKnight &= ~(1ULL << move.to); break;
                case 3:
                    wBishop &= ~(1ULL << move.to); break;
                case -3:
                    bBishop &= ~(1ULL << move.to); break;
                case 4:
                    wRook &= ~(1ULL << move.to); break;
                case -4:
                    bRook &= ~(1ULL << move.to); break;
                case 5:
                    wQueen &= ~(1ULL << move.to); break;
                case -5:
                    bQueen &= ~(1ULL << move.to); break;
                case 6:
                    wKing &= ~(1ULL << move.to); break;
                case -6:
                    bKing &= ~(1ULL << move.to); break;
                default:
                    std::cerr << "Uknown piece type in move.captured in makeMove(), value used -> " << move.captured << "\n";
            }
            //moving piece to its move square
            switch(move.piece) {
                case 1:
                    wPawns |= (1ULL << move.to); break;
                case -1:
                    bPawns |= (1ULL << move.to); break;
                case 2:
                    wKnight |= (1ULL << move.to); break;
                case -2:
                    bKnight |= (1ULL << move.to); break;
                case 3:
                    wBishop |= (1ULL << move.to); break;
                case -3:
                    bBishop |= (1ULL << move.to); break;
                case 4:
                    wRook |= (1ULL << move.to); break;
                case -4:
                    bRook |= (1ULL << move.to); break;
                case 5:
                    wQueen |= (1ULL << move.to); break;
                case -5:
                    bQueen |= (1ULL << move.to); break;
                case 6:
                    wKing |= (1ULL << move.to); break;
                case -6:
                    bKing |= (1ULL << move.to); break;
                case 0:
                    std::cerr << "Error in switch(move.piece) no piece type 0 why the fuk is this shi making a move with no piece?"; break;
                default:
                    std::cerr << "Error in switch(move.piece) inside makeMove() function: Error move.piece is not between -6 or 1 value used -> " << move.piece << "\n";
                    break;
            }
            //deal with en passent (at least some of the logic)
            if (move.enPassant == true) {
                if (moving == 0) { bPawns &= ~(1ULL << (move.to - 8)); } 
                else { wPawns &= ~(1ULL << (move.to + 8)); }
            }

            //deal with castling
            if (move.castle == true) {
                if (moving == 0) {
                    if (move.to - move.from > 0) {
                        wRook &= ~(1ULL << 7);
                        wRook |= (1ULL << 5);
                    }
                    else {
                        wRook &= ~(1ULL);
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

            //handle promtion pieces
            if (move.promotion != 0) {
                if (moving == 0) { wPawns &= ~(1ULL << move.to); }
                else { bPawns &= ~(1ULL << move.to); }
                switch(move.promotion) {
                        case 5:
                            wQueen |= (1ULL << move.to); break;
                        case -5:
                            bQueen |= (1ULL << move.to); break;
                        case 4:
                            wRook |= (1ULL << move.to); break;
                        case -4:
                            bRook |= (1ULL << move.to); break;
                        case 3:
                            wBishop |= (1ULL << move.to); break;
                        case -3:
                            bBishop |= (1ULL << move.to); break;
                        case 2:
                            wKnight |= (1ULL << move.to); break;
                        case -2:
                            bKnight |= (1ULL << move.to); break;      
                }
            }
            //update enpassant rules
            enpassent_square = -1;
            if ((move.piece == 1) && (move.to - move.from == 16)) {
                enpassent_square = move.from + 8;
            }
            else if ((move.piece == -1) && (move.from - move.to == 16)) {
                enpassent_square = move.from - 8;
            }

            if (moving == 0) {moving = 1;} 
            else if (moving == 1) {moving = 0;}
            else {std::cerr << "moving variable is neither 1 or 0! \n";}
        }

        void unmakeMove() {
            HistoryIndex--;
            Move& move = moveHistory[HistoryIndex];
            BoardState& previousState = stateHistory[HistoryIndex];

            wqscr = previousState.wqscr;
            wkscr = previousState.wkscr;
            bqscr = previousState.bqscr;
            bkscr = previousState.bkscr;
            moving = previousState.moving;

            halfmove_clock = previousState.halfmove_clock;
            enpassent_square = previousState.enpassent_square;


            //undo pawn promotion
            if (move.promotion != 0) {
                switch(move.promotion) {
                        case 5:
                            wQueen &= ~(1ULL << move.to); break;
                        case -5:
                            bQueen &= ~(1ULL << move.to); break;
                        case 4:
                            wRook &= ~(1ULL << move.to); break;
                        case -4:
                            bRook &= ~(1ULL << move.to); break;
                        case 3:
                            wBishop &= ~(1ULL << move.to); break;
                        case -3:
                            bBishop &= ~(1ULL << move.to); break;
                        case 2:
                            wKnight &= ~(1ULL << move.to); break;
                        case -2:
                            bKnight &= ~(1ULL << move.to); break;      
                }
                if (moving == 0) wPawns |= (1ULL << move.from);
                else bPawns |= (1ULL << move.from);
            }
            else {
                //remove the piece from its destination square
                switch(move.piece) {
                    case 1:
                        wPawns &= ~(1ULL << move.to); break;
                    case -1:
                        bPawns &= ~(1ULL << move.to); break;
                    case 2:
                        wKnight &= ~(1ULL << move.to); break;
                    case -2:
                        bKnight &= ~(1ULL << move.to); break;
                    case 3:
                        wBishop &= ~(1ULL << move.to); break;
                    case -3:
                        bBishop &= ~(1ULL << move.to); break;
                    case 4:
                        wRook &= ~(1ULL << move.to); break;
                    case -4:
                        bRook &= ~(1ULL << move.to); break;
                    case 5:
                        wQueen &= ~(1ULL << move.to); break;
                    case -5:
                        bQueen &= ~(1ULL << move.to); break;
                    case 6:
                        wKing &= ~(1ULL << move.to); break;
                    case -6:
                        bKing &= ~(1ULL << move.to); break;
                    case 0:
                        std::cerr << "Error in switch(move.piece) in unmakeMove() no piece type 0 why the fuk is this shi making a move with no piece?"; break;
                    default:
                        std::cerr << "Error in switch(move.piece) inside unmakeMove() function: Error move.piece is not between -6 or 1 value used -> " << move.piece << "\n";
                        break;
                }

                //add back any captured piece to the board
                switch(move.captured) {
                    case 0:
                        break;
                    case 1:
                        wPawns |= (1ULL << move.to); break;
                    case -1:
                        bPawns |= (1ULL << move.to); break;
                    case 2:
                        wKnight |= (1ULL << move.to); break;
                    case -2:
                        bKnight |= (1ULL << move.to); break;
                    case 3:
                        wBishop |= (1ULL << move.to); break;
                    case -3:
                        bBishop |= (1ULL << move.to); break;
                    case 4:
                        wRook |= (1ULL << move.to); break;
                    case -4:
                        bRook |= (1ULL << move.to); break;
                    case 5:
                        wQueen |= (1ULL << move.to); break;
                    case -5:
                        bQueen |= (1ULL << move.to); break;
                    case 6:
                        wKing |= (1ULL << move.to); break;
                    case -6:
                        bKing |= (1ULL << move.to); break;
                    default:
                        std::cerr << "Uknown piece type in move.captured in unmakeMove(), value used -> " << move.captured << "\n";
                }

                //bring the moved piece back to its orginal square
                switch(move.piece) {
                    case 1:
                        wPawns |= (1ULL << move.from); break;
                    case -1:
                        bPawns |= (1ULL << move.from); break;
                    case 2:
                        wKnight |= (1ULL << move.from); break;
                    case -2:
                        bKnight |= (1ULL << move.from); break;
                    case 3:
                        wBishop |= (1ULL << move.from); break;
                    case -3:
                        bBishop |= (1ULL << move.from); break;
                    case 4:
                        wRook |= (1ULL << move.from); break;
                    case -4:
                        bRook |= (1ULL << move.from); break;
                    case 5:
                        wQueen |= (1ULL << move.from); break;
                    case -5:
                        bQueen |= (1ULL << move.from); break;
                    case 6:
                        wKing |= (1ULL << move.from); break;
                    case -6:
                        bKing |= (1ULL << move.from); break;
                    case 0:
                        std::cerr << "Error in switch(move.piece) inside unmakeMove() no piece type 0 why the fuk is this shi making a move with no piece?"; break;
                    default:
                        std::cerr << "Error in switch(move.piece) inside unmakeMove() function: Error move.piece is not between -6 or 1 value used -> " << move.piece << "\n";
                        break;
                }

                //add back the pawn if en passant happened
                if (move.enPassant == true) {
                if (moving == 0) { bPawns |= (1ULL << (move.to - 8)); } 
                else { wPawns |= (1ULL << (move.to + 8)); }
                }
            }
            //undo castling if it happened
            if (move.castle == true) {
                if (moving == 0) {
                    if (move.to - move.from > 0) {
                        wRook |= (1ULL << 7);
                        wRook &= ~(1ULL << 5);
                    }
                    else {
                        wRook |= (1ULL);
                        wRook &= ~(1ULL << 3);
                    }
                }
                else {
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

        uint64_t getOccupancy() {
            return (wPawns | wBishop | wKnight | wRook | wQueen | wKing) | (bPawns | bBishop | bKnight | bRook | bQueen | bKing);
        }

        uint64_t getOccupancyWhite() {
            return wPawns | wBishop | wKnight | wRook | wQueen | wKing;
        }

        uint64_t getOccupancyBlack() {
            return bPawns | bBishop | bKnight | bRook | bQueen | bKing;
        }

        void initPieces() {
            wPawns = wBishop = wKnight = wRook = wQueen = wKing = 0;
            bPawns = bBishop = bKnight = bRook = bQueen = bKing = 0;
            boardToBitBoard(board); //Converet the int[8][8] board to 12 different bit boards
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

        void initSlidingPieceAttackTB() {
            //Make the two offest tables
            rookOffsets[0] = 0;
            bishopOffsets[0] = 0;
            for (int i = 1; i < 64; i++) { rookOffsets[i] = rookOffsets[i - 1] + (1ULL << (64 - rookShifts[i - 1])); }
            for (int i = 1; i < 64; i++) { bishopOffsets[i] = bishopOffsets[i - 1] + (1ULL << (64 - bishopShifts[i - 1])); }

            fillRookAttacks();
            fillBishopAttacks();
        }

        void initAttackBitBoards() {
            if (initABBran == false) {initABBran = true;}
            else {std::cerr << "Function initAttackBitBoard has already been ran before!\n";}
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
            //Knight 0 = TLCT || 1 = TRCT || 2 = TRCB || 3 = BRCT || 4 = BRCB || 5 = BLCB || 6 = BLCT || 7 = TLCB
            for (int i = 0; i < 64; i++) {
                temp_val = 1ULL << i;
                int squares_knight[8] = {1, 1, 1, 1, 1, 1, 1, 1};
                if (i >= 48 || i % 8 == 0) { squares_knight[0] = 0; }
                if (i >= 48 || i % 8 == 7) { squares_knight[1] = 0; }
                if (i >= 56 || i % 8 == 6 || i % 8 == 7) { squares_knight[2] = 0; }
                if (i <= 15 || i % 8 == 7) { squares_knight[3] = 0; }
                if (i <= 7  || i % 8 == 6 || i % 8 == 7) { squares_knight[4] = 0; }
                if (i <= 15 || i % 8 == 7) { squares_knight[5] = 0; }
                if (i <= 7  || i % 8 == 0 || i % 8 == 1) { squares_knight[6] = 0; }
                if (i >= 56 || i % 8 == 0 || i % 8 == 1) { squares_knight[7] = 0; }
                temp_knight(i, squares_knight);
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
        }

        void boardToBitBoard(int board[8][8]) {
            for (int x = 0; x < 8; x++) {
                for (int y = 0; y < 8; y++) {
                    int piece = board[x][y];
                    if (piece == 0) { continue; }
                    else if (piece == 1) { wPawns |= (1ULL << (x*8 + y)); }
                    else if (piece == 2) { wBishop |= (1ULL << (x*8 + y)); }
                    else if (piece == 3) { wKnight |= (1ULL << (x*8 + y)); }
                    else if (piece == 4) { wRook |= (1ULL << (x*8 + y)); }
                    else if (piece == 5) { wQueen |= (1ULL << (x*8 + y)); }
                    else if (piece == 6) { wKing |= (1ULL << (x*8 + y)); }

                    else if (piece == -1) { bPawns |= (1ULL << (x*8 + y)); }
                    else if (piece == -2) { bBishop |= (1ULL << (x*8 + y)); }
                    else if (piece == -3) { bKnight |= (1ULL << (x*8 + y)); }
                    else if (piece == -4) { bRook |= (1ULL << (x*8 + y)); }
                    else if (piece == -5) { bQueen |= (1ULL << (x*8 + y)); }
                    else if (piece == -6) { bKing |= (1ULL << (x*8 + y)); }
                }
            }
        }

        void parseFen(std::string fen) {
            const std::unordered_map<char, int> fenMap =
            {
                {'P', 1},
                {'N', 2},
                {'B', 3},
                {'R', 4},
                {'Q', 5},
                {'K', 6},
                {'p', -1},
                {'n', -2},
                {'b', -3},
                {'r', -4},
                {'q', -5},
                {'k', -6},
            };
            int col = 0;
            int row = 0;
            enpassent_square = 0;
            for (int i = 0; i < 8; i++)
                for (int j = 0; j < 8; j++)
                    board[i][j] = 0;
            bool done_with_board = false;
            int space_counter = 0;
            for (int i = 0; i < fen.length(); i++) {
                char c = fen[i];
                if (done_with_board == false) {
                    if (c == '/') {row += 1; col = 0; continue;}
                    if (c >= '0' && c <= '9') { //only simple way to convert char to nums without a libary
                        int num = c - '0';
                        for (int j = 0; j < num; j++) {
                            board[7 - row][col] = 0;
                            col++;
                        }
                        continue;
                    }
                    if (fenMap.count(c)) { board[7- row][col] = fenMap.at(c); col++; }
                }
                if (c == ' ') {space_counter++; continue;}
                if (col == 8 && row == 7) { done_with_board = true; continue; }
                if (space_counter == 1) {
                    if (c == 'w') { moving = 0; }
                    else if (c == 'b') { moving = 1; }
                    else { std::cerr << "Invalid Fen String!" << std::endl; }
                }
                else if (space_counter == 2) {
                    if (c == 'K') wkscr = true;
                    if (c == 'Q') wqscr = true;
                    if (c == 'k') bkscr = true;
                    if (c == 'q') bqscr = true;
                }
                else if (space_counter == 3) {
                    if (c == '-') {
                        enpassent_square = -1;
                    } else if (c >= 'a' && c <= 'h') {
                        // store file temporarily
                        int file = c - 'a'; // 0–7
                        i++; // move to next char, which should be rank
                        if (i < fen.size() && fen[i] >= '1' && fen[i] <= '8') {
                            int rank = fen[i] - '1';       // 0–7
                            enpassent_square = (7 - rank) * 8 + file;
                        } else {
                            std::cerr << "Invalid en passant square in FEN!\n";
                            enpassent_square = -1;
                        }
                    } else {
                        std::cerr << "Invalid en passant character in FEN!\n";
                        enpassent_square = -1;
                    }
                }
                else if (space_counter == 4) { halfmove_clock = halfmove_clock * 10 + (c - '0'); }
                else if (space_counter == 5) { break; }
            }
        }

        // printBoard is the only function that was vibecoded since I hate messing with ascii :] cry all you want
        void printBoard(int board_temp_val[8][8]) {
            static const std::unordered_map<int, const char*> pieces = {
                {1, "P"}, 
                {2, "N"}, 
                {3, "B"}, 
                {4, "R"}, 
                {5, "Q"}, 
                {6, "K"},
                {-1, "P"}, 
                {-2, "N"}, 
                {-3, "B"}, 
                {-4, "R"}, 
                {-5, "Q"}, 
                {-6, "K"},
                {0, " "}
            };

            const std::string LIGHT_ORANGE = "\033[38;2;255;200;130m";
            const std::string DARK_ORANGE  = "\033[38;2;180;90;20m";
            const std::string BORDER       = "\033[38;2;255;179;102m";
            const std::string RESET        = "\033[0m";

            std::cout << BORDER << "   a   b   c   d   e   f   g   h\n";
            std::cout << "  ╔═══╦═══╦═══╦═══╦═══╦═══╦═══╦═══╗\n" << RESET;

            for (int row = 0; row < 8; row++) {
                std::cout << BORDER << (8 - row) << " ║" << RESET;
                for (int col = 0; col < 8; col++) {
                    int piece = board[7- row][col];
                    int absPiece = piece < 0 ? -piece : piece;
                    if (piece > 0)      std::cout << LIGHT_ORANGE << " " << pieces.at(absPiece) << " " << RESET;
                    else if (piece < 0) std::cout << DARK_ORANGE  << " " << pieces.at(absPiece) << " " << RESET;
                    else                std::cout << "   ";
                    std::cout << BORDER << "║" << RESET;
                }
                std::cout << BORDER << " " << (8 - row) << "\n";
                if (row < 7)
                    std::cout << BORDER << "  ╠═══╬═══╬═══╬═══╬═══╬═══╬═══╬═══╣\n" << RESET;
            }

            std::cout << BORDER << "  ╚═══╩═══╩═══╩═══╩═══╩═══╩═══╩═══╝\n";
            std::cout << "   a   b   c   d   e   f   g   h\n" << RESET;
        }

        void LongPrint(uint64_t board) {
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

    private:
        void temp_knight(int i, int squares_knight_temp[8]) {
            //ungoldy readability ik right :}                                  i reallyyyyyy dont feel like fixing this
            uint64_t temp_val_knight_function = 1ULL << i;
            uint64_t temp_val_temp = temp_val_knight_function;
            if (squares_knight_temp[0] == 1) {temp_val_temp = temp_val_temp | temp_val_knight_function << 15; }
            temp_val_knight_function = 1ULL << i;
            if (squares_knight_temp[1] == 1) {temp_val_temp = temp_val_temp | temp_val_knight_function << 17; }
            temp_val_knight_function = 1ULL << i;
            if (squares_knight_temp[2] == 1) {temp_val_temp = temp_val_temp | temp_val_knight_function << 10; }
            temp_val_knight_function = 1ULL << i;
            if (squares_knight_temp[3] == 1) {temp_val_temp = temp_val_temp | temp_val_knight_function >> 6; }
            temp_val_knight_function = 1ULL << i;
            if (squares_knight_temp[4] == 1) {temp_val_temp = temp_val_temp | temp_val_knight_function >> 15; }
            temp_val_knight_function = 1ULL << i;
            if (squares_knight_temp[5] == 1) {temp_val_temp = temp_val_temp | temp_val_knight_function >> 17; }
            temp_val_knight_function = 1ULL << i;
            if (squares_knight_temp[6] == 1) {temp_val_temp = temp_val_temp | temp_val_knight_function >> 10; }
            temp_val_knight_function = 1ULL << i;
            if (squares_knight_temp[7] == 1) {temp_val_temp = temp_val_temp | temp_val_knight_function << 6; }
            knightAttacks[i] = temp_val_temp & ~temp_val_knight_function;
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

class genMove {
    public:
        Board& board;
        genMove(Board& board) : board(board) {}


        int perft(int depth) {
            if (depth == 0) return 1;
            
            int count = generateMoves();
            int nodes = 0;

            for (int i = 0; i < count; i++) {
                board.makeMove(board.moveHistory[i]);
                
                if (!board.isInCheck(1 - board.moving)) {
                    nodes += perft(depth - 1);
                }

                board.unmakeMove();
            }
            return nodes;
        }

        int generateMoves() {
            int count = 0;
            uint64_t rank3Mask = 0x0000000000FF0000ULL;
            uint64_t rank6Mask = 0x0000FF0000000000ULL;
            uint64_t occ = board.getOccupancy();
            if (board.moving == 0) {
                uint64_t pawns = board.wPawns;
                uint64_t bishops = board.wBishop; //done
                uint64_t knights = board.wKnight; //done
                uint64_t rooks = board.wRook;   //done
                uint64_t queens = board.wQueen; //done
                uint64_t king = board.wKing;  //done

                while (pawns) {
                    int from = __builtin_ctzll(pawns);
                    pawns &= pawns - 1;

                    uint64_t singlePush = (1ULL << from << 8) & ~board.getOccupancy();
                    uint64_t doublePush = ((singlePush & rank3Mask) << 8) & ~board.getOccupancy();
                    uint64_t attacks = board.pawnAttacksW[from] & board.getOccupancyBlack();

                    uint64_t ep = 0;
                    if (board.enpassent_square != -1) {
                        ep = board.pawnAttacksW[from] & (1ULL << board.enpassent_square);
                        if (ep != 0) {
                            Move move;
                            move.from = from;
                            move.to = board.enpassent_square;
                            move.piece = 1;
                            move.captured = -1;
                            move.promotion = 0;
                            move.enPassant = true;
                            move.castle = false;

                            board.moveHistory[count++] = move;
                        }
                    }

                    while (attacks) {
                        int to = __builtin_ctzll(attacks);
                        attacks &= attacks - 1;

                        if (to / 8 != 7) {
                            Move move;
                            move.from = from;
                            move.to = to;
                            move.piece = 1;
                            move.captured = getPieceAt(to);
                            move.promotion = 0;
                            move.enPassant = false;
                            move.castle = false;

                            board.moveHistory[count++] = move;
                        }
                        //promotion logic
                        else if (to / 8 == 7){
                            {Move move;
                            move.from = from;
                            move.to = to;
                            move.piece = 1;
                            move.captured = getPieceAt(to);
                            move.promotion = 2;
                            move.enPassant = false;
                            move.castle = false;

                            board.moveHistory[count++] = move;}

                            {Move move;
                            move.from = from;
                            move.to = to;
                            move.piece = 1;
                            move.captured = getPieceAt(to);
                            move.promotion = 3;
                            move.enPassant = false;
                            move.castle = false;

                            board.moveHistory[count++] = move;}

                            {Move move;
                            move.from = from;
                            move.to = to;
                            move.piece = 1;
                            move.captured = getPieceAt(to);
                            move.promotion = 4;
                            move.enPassant = false;
                            move.castle = false;

                            board.moveHistory[count++] = move;}

                            {Move move;
                            move.from = from;
                            move.to = to;
                            move.piece = 1;
                            move.captured = getPieceAt(to);
                            move.promotion = 5;
                            move.enPassant = false;
                            move.castle = false;

                            board.moveHistory[count++] = move;}
                        }
                    }

                    while (singlePush) {
                        int to = __builtin_ctzll(singlePush);
                        singlePush &= singlePush - 1;
                        
                        if (to / 8 != 7) {
                            Move move;
                            move.from = from;
                            move.to = to;
                            move.piece = 1;
                            move.captured = 0;
                            move.promotion = 0;
                            move.enPassant = false;
                            move.castle = false;

                            board.moveHistory[count++] = move;
                        }

                        //promotion logic
                        else if (to / 8 == 7) {
                            {Move move;
                            move.from = from;
                            move.to = to;
                            move.piece = 1;
                            move.captured = 0;
                            move.promotion = 2;
                            move.enPassant = false;
                            move.castle = false;

                            board.moveHistory[count++] = move;}

                            {Move move;
                            move.from = from;
                            move.to = to;
                            move.piece = 1;
                            move.captured = 0;
                            move.promotion = 3;
                            move.enPassant = false;
                            move.castle = false;

                            board.moveHistory[count++] = move;}

                            {Move move;
                            move.from = from;
                            move.to = to;
                            move.piece = 1;
                            move.captured = 0;
                            move.promotion = 4;
                            move.enPassant = false;
                            move.castle = false;

                            board.moveHistory[count++] = move;}

                            {Move move;
                            move.from = from;
                            move.to = to;
                            move.piece = 1;
                            move.captured = 0;
                            move.promotion = 5;
                            move.enPassant = false;
                            move.castle = false;

                            board.moveHistory[count++] = move;}
                        }
                    }

                    while (doublePush) {
                        int to = __builtin_ctzll(doublePush);
                        doublePush &= doublePush - 1;

                        Move move;
                        move.from = from;
                        move.to = to;
                        move.piece = 1;
                        move.captured = 0;
                        move.promotion = 0;
                        move.enPassant = false;
                        move.castle = false;

                        board.moveHistory[count++] = move;
                    }
                }
                while (knights) {
                    int from = __builtin_ctzll(knights);
                    knights &= knights - 1;

                    uint64_t attacks = board.knightAttacks[from] & ~board.getOccupancyWhite();
                    while (attacks) {
                        int to = __builtin_ctzll(attacks);
                        attacks &= attacks - 1;

                        Move move;
                        move.from = from;
                        move.to = to;
                        move.piece = 2;
                        move.captured = getPieceAt(to);
                        move.promotion = 0;
                        move.enPassant = false;
                        move.castle = false;

                        board.moveHistory[count++] = move;
                    }
                }
                while (bishops) {
                    int from = __builtin_ctzll(bishops);
                    bishops &= bishops - 1;

                    uint64_t attacks = board.getbishopAttacks(from, occ) & ~board.getOccupancyWhite();
                    while (attacks) {
                        int to = __builtin_ctzll(attacks);
                        attacks &= attacks - 1;
                        
                        Move move;
                        move.from = from;
                        move.to = to;
                        move.piece = 3;
                        move.captured = getPieceAt(to);
                        move.promotion = 0;
                        move.enPassant = false;
                        move.castle = false;

                        board.moveHistory[count++] = move;
                    }
                }
                while (rooks) {
                    int from = __builtin_ctzll(rooks);
                    rooks &= rooks - 1;

                    uint64_t attacks = board.getrookAttacks(from, occ) & ~board.getOccupancyWhite();
                    while (attacks) {
                        int to = __builtin_ctzll(attacks);
                        attacks &= attacks - 1;
                        
                        Move move;
                        move.from = from;
                        move.to = to;
                        move.piece = 4;
                        move.captured = getPieceAt(to);
                        move.promotion = 0;
                        move.enPassant = false;
                        move.castle = false;

                        board.moveHistory[count++] = move;
                    }
                }
                while (queens) {
                    int from = __builtin_ctzll(queens);
                    queens &= queens - 1;

                    uint64_t attacks = (board.getrookAttacks(from, occ) | board.getbishopAttacks(from, occ)) & ~board.getOccupancyWhite();
                    while (attacks) {
                        int to = __builtin_ctzll(attacks);
                        attacks &= attacks - 1;
                        
                        Move move;
                        move.from = from;
                        move.to = to;
                        move.piece = 5;
                        move.captured = getPieceAt(to);
                        move.promotion = 0;
                        move.enPassant = false;
                        move.castle = false;

                        board.moveHistory[count++] = move;
                    }
                }
                while (king) {
                    int from = __builtin_ctzll(king);
                    king &= king - 1;

                    uint64_t attacks = board.kingAttacks[from] & ~board.getOccupancyWhite();
                    while (attacks) {
                        int to = __builtin_ctzll(attacks);
                        attacks &= attacks - 1;

                        Move move;
                        move.from = from;
                        move.to = to;
                        move.piece = 6;
                        move.captured = getPieceAt(to);
                        move.promotion = 0;
                        move.enPassant = false;
                        move.castle = false;

                        board.moveHistory[count++] = move;
                    }

                    //handle kingside castle
                    if (board.wkscr &&
                        getPieceAt(5) == 0 && getPieceAt(6) == 0 &&
                        !board.isAttacked(4, 1) && !board.isAttacked(5, 1) && !board.isAttacked(6, 1)) {
                        Move move;
                        move.from = from;
                        move.to = 6;
                        move.piece = 6;
                        move.captured = 0;
                        move.promotion = 0;
                        move.enPassant = false;
                        move.castle = true;

                        board.moveHistory[count++] = move;
                    }
                    //handle queenside castle
                    if (board.wqscr &&
                        getPieceAt(1) == 0 && getPieceAt(2) == 0 && getPieceAt(3) == 0 &&
                        !board.isAttacked(2, 1) && !board.isAttacked(3, 1) && !board.isAttacked(4, 1)) {
                        
                        Move move;
                        move.from = from;
                        move.to = 2;
                        move.piece = 6;
                        move.captured = 0;
                        move.promotion = 0;
                        move.enPassant = false;
                        move.castle = true;

                        board.moveHistory[count++] = move;
                    }
                }
            }

            //black
            else if (board.moving == 1) {
                uint64_t pawns = board.bPawns; //done
                uint64_t bishops = board.bBishop; //done
                uint64_t knights = board.bKnight; //done
                uint64_t rooks = board.bRook;   //done
                uint64_t queens = board.bQueen; //done
                uint64_t king = board.bKing;  //done

                while (pawns) {
                    int from = __builtin_ctzll(pawns);
                    pawns &= pawns - 1;

                    uint64_t singlePush = (1ULL >> from << 8) & ~board.getOccupancy();
                    uint64_t doublePush = ((singlePush & rank6Mask) >> 8) & ~board.getOccupancy();
                    uint64_t attacks = board.pawnAttacksB[from] & board.getOccupancyWhite();

                    uint64_t ep = 0;
                    if (board.enpassent_square != -1) {
                        ep = board.pawnAttacksB[from] & (1ULL << board.enpassent_square);
                        if (ep != 0) {
                            Move move;
                            move.from = from;
                            move.to = board.enpassent_square;
                            move.piece = -1;
                            move.captured = 1;
                            move.promotion = 0;
                            move.enPassant = true;
                            move.castle = false;

                            board.moveHistory[count++] = move;
                        }
                    }

                    while (attacks) {
                        int to = __builtin_ctzll(attacks);
                        attacks &= attacks - 1;

                        if (to / 8 != 0) {
                            Move move;
                            move.from = from;
                            move.to = to;
                            move.piece = -1;
                            move.captured = getPieceAt(to);
                            move.promotion = 0;
                            move.enPassant = false;
                            move.castle = false;

                            board.moveHistory[count++] = move;
                        }
                        //promotion logic
                        else if (to / 8 == 0){
                            {Move move;
                            move.from = from;
                            move.to = to;
                            move.piece = -1;
                            move.captured = getPieceAt(to);
                            move.promotion = -2;
                            move.enPassant = false;
                            move.castle = false;

                            board.moveHistory[count++] = move;}

                            {Move move;
                            move.from = from;
                            move.to = to;
                            move.piece = -1;
                            move.captured = getPieceAt(to);
                            move.promotion = -3;
                            move.enPassant = false;
                            move.castle = false;

                            board.moveHistory[count++] = move;}

                            {Move move;
                            move.from = from;
                            move.to = to;
                            move.piece = -1;
                            move.captured = getPieceAt(to);
                            move.promotion = -4;
                            move.enPassant = false;
                            move.castle = false;

                            board.moveHistory[count++] = move;}

                            {Move move;
                            move.from = from;
                            move.to = to;
                            move.piece = -1;
                            move.captured = getPieceAt(to);
                            move.promotion = -5;
                            move.enPassant = false;
                            move.castle = false;

                            board.moveHistory[count++] = move;}
                        }
                    }

                    while (singlePush) {
                        int to = __builtin_ctzll(singlePush);
                        singlePush &= singlePush - 1;
                        
                        if (to / 8 != 0) {
                            Move move;
                            move.from = from;
                            move.to = to;
                            move.piece = -1;
                            move.captured = 0;
                            move.promotion = 0;
                            move.enPassant = false;
                            move.castle = false;

                            board.moveHistory[count++] = move;
                        }

                        //promotion logic
                        else if (to / 8 == 0) {
                            {Move move;
                            move.from = from;
                            move.to = to;
                            move.piece = -1;
                            move.captured = 0;
                            move.promotion = -2;
                            move.enPassant = false;
                            move.castle = false;

                            board.moveHistory[count++] = move;}

                            {Move move;
                            move.from = from;
                            move.to = to;
                            move.piece = -1;
                            move.captured = 0;
                            move.promotion = -3;
                            move.enPassant = false;
                            move.castle = false;

                            board.moveHistory[count++] = move;}

                            {Move move;
                            move.from = from;
                            move.to = to;
                            move.piece = -1;
                            move.captured = 0;
                            move.promotion = -4;
                            move.enPassant = false;
                            move.castle = false;

                            board.moveHistory[count++] = move;}

                            {Move move;
                            move.from = from;
                            move.to = to;
                            move.piece = -1;
                            move.captured = 0;
                            move.promotion = -5;
                            move.enPassant = false;
                            move.castle = false;

                            board.moveHistory[count++] = move;}
                        }
                    }

                    while (doublePush) {
                        int to = __builtin_ctzll(doublePush);
                        doublePush &= doublePush - 1;

                        Move move;
                        move.from = from;
                        move.to = to;
                        move.piece = -1;
                        move.captured = 0;
                        move.promotion = 0;
                        move.enPassant = false;
                        move.castle = false;

                        board.moveHistory[count++] = move;
                    }
                }
                while (knights) {
                    int from = __builtin_ctzll(knights);
                    knights &= knights - 1;

                    uint64_t attacks = board.knightAttacks[from] & ~board.getOccupancyBlack();
                    while (attacks) {
                        int to = __builtin_ctzll(attacks);
                        attacks &= attacks - 1;

                        Move move;
                        move.from = from;
                        move.to = to;
                        move.piece = -2;
                        move.captured = getPieceAt(to);
                        move.promotion = 0;
                        move.enPassant = false;
                        move.castle = false;

                        board.moveHistory[count++] = move;
                    }
                }
                while (bishops) {
                    int from = __builtin_ctzll(bishops);
                    bishops &= bishops - 1;

                    uint64_t attacks = board.getbishopAttacks(from, occ) & ~board.getOccupancyBlack();
                    while (attacks) {
                        int to = __builtin_ctzll(attacks);
                        attacks &= attacks - 1;
                        
                        Move move;
                        move.from = from;
                        move.to = to;
                        move.piece = -3;
                        move.captured = getPieceAt(to);
                        move.promotion = 0;
                        move.enPassant = false;
                        move.castle = false;

                        board.moveHistory[count++] = move;
                    }
                }
                while (rooks) {
                    int from = __builtin_ctzll(rooks);
                    rooks &= rooks - 1;

                    uint64_t attacks = board.getrookAttacks(from, occ) & ~board.getOccupancyBlack();
                    while (attacks) {
                        int to = __builtin_ctzll(attacks);
                        attacks &= attacks - 1;
                        
                        Move move;
                        move.from = from;
                        move.to = to;
                        move.piece = -4;
                        move.captured = getPieceAt(to);
                        move.promotion = 0;
                        move.enPassant = false;
                        move.castle = false;

                        board.moveHistory[count++] = move;
                    }
                }
                while (queens) {
                    int from = __builtin_ctzll(queens);
                    queens &= queens - 1;

                    uint64_t attacks = (board.getrookAttacks(from, occ) | board.getbishopAttacks(from, occ)) & ~board.getOccupancyBlack();
                    while (attacks) {
                        int to = __builtin_ctzll(attacks);
                        attacks &= attacks - 1;
                        
                        Move move;
                        move.from = from;
                        move.to = to;
                        move.piece = -5;
                        move.captured = getPieceAt(to);
                        move.promotion = 0;
                        move.enPassant = false;
                        move.castle = false;

                        board.moveHistory[count++] = move;
                    }
                }
                while (king) {
                    int from = __builtin_ctzll(king);
                    king &= king - 1;

                    uint64_t attacks = board.kingAttacks[from] & ~board.getOccupancyBlack();
                    while (attacks) {
                        int to = __builtin_ctzll(attacks);
                        attacks &= attacks - 1;

                        Move move;
                        move.from = from;
                        move.to = to;
                        move.piece = -6;
                        move.captured = getPieceAt(to);
                        move.promotion = 0;
                        move.enPassant = false;
                        move.castle = false;

                        board.moveHistory[count++] = move;
                    }

                    //handle kingside castle
                    if (board.bkscr &&
                        getPieceAt(61) == 0 && getPieceAt(62) == 0 &&
                        !board.isAttacked(60, 0) && !board.isAttacked(61, 0) && !board.isAttacked(62, 0)) {
                        Move move;
                        move.from = from;
                        move.to = 62;
                        move.piece = -6;
                        move.captured = 0;
                        move.promotion = 0;
                        move.enPassant = false;
                        move.castle = true;

                        board.moveHistory[count++] = move;
                    }
                    //handle queenside castle
                    if (board.bqscr &&
                        getPieceAt(57) == 0 && getPieceAt(58) == 0 && getPieceAt(59) == 0 &&
                        !board.isAttacked(58, 0) && !board.isAttacked(59, 0) && !board.isAttacked(60, 0)) {
                        
                        Move move;
                        move.from = from;
                        move.to = 58;
                        move.piece = -6;
                        move.captured = 0;
                        move.promotion = 0;
                        move.enPassant = false;
                        move.castle = true;

                        board.moveHistory[count++] = move;
                    }
                }
            }
            return count;
        }

        int getPieceAt(int sq) {
            uint64_t bit = 1ULL << sq;
            if (board.wPawns & bit)  return 1;
            if (board.wKnight & bit) return 2;
            if (board.wBishop & bit) return 3;
            if (board.wRook & bit)   return 4;
            if (board.wQueen & bit)  return 5;
            if (board.wKing & bit)   return 6;
            if (board.bPawns & bit)  return -1;
            if (board.bKnight & bit) return -2;
            if (board.bBishop & bit) return -3;
            if (board.bRook & bit)   return -4;
            if (board.bQueen & bit)  return -5;
            if (board.bKing & bit)   return -6;
            return 0;
        }

    private:

};


int main() {
    Board board;
    
    std::cout << "\033[38;2;255;179;102m";
    std::cout << "           ███████╗██╗██╗  ██╗\n";
    std::cout << "           ██╔════╝██║██║  ██║\n";
    std::cout << "           █████╗  ██║███████║\n";
    std::cout << "           ██╔══╝  ██║██╔══██║\n";
    std::cout << "           ██║     ██║██║  ██║\n";
    std::cout << "           ╚═╝     ╚═╝╚═╝  ╚═╝\n";
    std::cout << "===========================================\n";
    std::cout << "| A Chess bot | elo: -1000 | version: 0.3 |\n";
    std::cout << "===========================================\n";

    std::string input_fen_string;

    // rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
    std::cout << "Enter Fen String (Press enter for default): "; std::getline(std::cin, input_fen_string);
    if (input_fen_string == "") { input_fen_string = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"; }
    std::cout << "\n";
    board.parseFen(input_fen_string);
    board.printBoard(board.board);

    board.initBoard();
    genMove genMove(board);
    std::cout << "perft 1: " << genMove.perft(1) << "\n";
    std::cout << "perft 2: " << genMove.perft(2) << "\n";
    std::cout << "perft 3: " << genMove.perft(3) << "\n";
    std::cout << "perft 4: " << genMove.perft(4) << "\n";
    std::cout << "perft 5: " << genMove.perft(5) << "\n";
    std::cout << "perft 6: " << genMove.perft(6) << "\n";
    //std::cout << genMove.generateMoves() << std::endl;

    return 0;
}
