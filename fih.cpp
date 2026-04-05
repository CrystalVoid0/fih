#include <cstdint>
#include <iostream>
#include <string>
#include <thread>


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
        int side = 0;
        int halfmove_clock = 0;
        int enpassent_square = -1;
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

        int HistorySize = 4096;
        int HistoryIndex = 0;   
        Move moveHistory[HistorySize];
        BoardState stateHistory[HistorySize];

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

        void initBoard() {
            if (initBoardRand == false) { initBoardRan = true; }
            else {std::cerr << "Function initBoard has already been ran before!\n";}
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




    private:

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

    


    return 0;
}