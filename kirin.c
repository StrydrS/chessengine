/* 
      ,  ,
          \\ \\                 Kirin Chess Engine v0.2 (not officially released)
          ) \\ \\    _p_        created by Strydr Silverberg    
          )^\))\))  /  *\       sophomore year Colorado School of Mines
           \_|| || / /^`-' 
  __       -\ \\--/ /           special thanks to
<'  \\___/   ___. )'            Maxim Korzh                                 https://github.com/maksimKorzh
     `====\ )___/\\             ChessProgramming.org                        https://www.chessprogramming.org
          //     `"             Talk Chess (Computer Chess Club)            https://talkchess.com/
          \\    /  \            
          `"
*/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

/************ Define Bitboard Type ************/
#define U64 unsigned long long

//FEN debug positions
#define empty_board "8/8/8/8/8/8/8/8 b - - "
#define startPosition "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 "
#define tricky_position "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1 "
#define killer_position "rnbqkb1r/pp1p1pPp/8/2p1pP2/1P1P4/3P3P/P1P1P3/RNBQKBNR w KQkq e6 0 1"
#define cmk_position "r2q1rk1/ppp2ppp/2n1bn2/2b1p3/3pP3/3P1NPP/PPP1NPB1/R1BQ1RK1 b - - 0 9 "
#define new_pos "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10 "
#define repetitions "2r3k1/R7/8/1R6/8/8/P4KPP/8 w - - 0 40 "

/************ Board Squares ************/
enum {
  a8, b8, c8, d8, e8, f8, g8, h8,
  a7, b7, c7, d7, e7, f7, g7, h7,
  a6, b6, c6, d6, e6, f6, g6, h6,
  a5, b5, c5, d5, e5, f5, g5, h5,
  a4, b4, c4, d4, e4, f4, g4, h4,
  a3, b3, c3, d3, e3, f3, g3, h3,
  a2, b2, c2, d2, e2, f2, g2, h2,
  a1, b1, c1, d1, e1, f1, g1, h1, no_sq
};

enum { P, N, B, R, Q, K, p, n, b, r, q, k };

enum { white, black, both};

enum { rook, bishop };

/*

    bin  dec
    
   0001    1  white king can castle to the king side
   0010    2  white king can castle to the queen side
   0100    4  black king can castle to the king side
   1000    8  black king can castle to the queen side

   examples

   1111       both sides an castle both directions
   1001       black king => queen side
              white king => king side

*/

enum { wk = 1, wq = 2, bk = 4, bq = 8};

const char *squareCoordinates[] = {
  "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8",
  "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
  "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
  "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
  "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
  "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
  "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
  "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1"
};

char asciiPieces[12] = "PNBRQKpnbrqk";
char *unicodePieces[12]  = {"♙", "♘", "♗", "♖", "♕", "♔", "♟", "♞", "♝", "♜", "♛" ,"♚"};

//convert ascii character pieces to encoded constants
int charPieces[] = {
  ['P'] = P,
  ['N'] = N,
  ['B'] = B,
  ['R'] = R,
  ['Q'] = Q,
  ['K'] = K,
  ['p'] = p,
  ['n'] = n,
  ['b'] = b,
  ['r'] = r,
  ['q'] = q,
  ['k'] = k
};

char promotedPieces[] = { 
  [Q] = 'q', 
  [R] = 'r',
  [B] = 'b',
  [N] = 'n',
  [q] = 'q', 
  [r] = 'r',
  [b] = 'b',
  [n] = 'n',
};

/************ Board Variables ************/
//define piece bitboards, occupancy bitboards, side to move, enpassant square
U64 bitboards[12]; 
U64 occupancy[3];
int side;
int enpassant = no_sq;
int castle;

//"almost" unique position identifier -> hash key/position key
U64 hashKey;

U64 repetitionTable[1000]; 
int repetitionIndex; 
//half move counter
int ply;


/************ Time Control Variables ************/
int quit = 0;
int movesToGo = 30;
int moveTime = -1;
int timer = -1;
int inc = 0;
int startTime = 0;
int stopTime = 0;
int timeSet = 0;
int stopped = 0;


/************ UCI Time Control Functions from VICE by Richard Allbert ************/
int getTimeMS() { 
  //get time in ms
  struct timeval timeValue;
  gettimeofday(&timeValue, NULL);
  return timeValue.tv_sec * 1000 + timeValue.tv_usec / 1000;
}

int inputWaiting() {
  fd_set readfds;
  struct timeval tv;
  FD_ZERO (&readfds);
  FD_SET (fileno(stdin), &readfds);
  tv.tv_sec=0; tv.tv_usec=0;
  select(16, &readfds, 0, 0, &tv);

  return (FD_ISSET(fileno(stdin), &readfds));
}

void readInput() { 
  int bytes;
  char input[256] = "", *endc;

  if(inputWaiting()) { 
    stopped = 1;
    do {
      bytes = read(fileno(stdin), input, 256);
    } while(bytes < 0);

    endc = strchr(input, '\n');
    if(endc) *endc=0;

    if(strlen(input) > 0) { 
      if(!strncmp(input, "quit", 4)) quit = 1;
      else if(!strncmp(input, "stop", 4)) quit = 1;
    }
  }
}

static void communicate() { 
  if(timeSet == 1 && getTimeMS() > stopTime) stopped = 1;
  readInput();
}

/************ Random Magic Numbers! ************/
//from Tord Romstad's proposal to find magic numbers https://www.chessprogramming.org/Looking_for_Magics
//pseudo random number state
unsigned int randState = 1804289383;

//generate 32bit pseudo legal numbers
unsigned int getRandU32() {
  //get current state
  unsigned int num = randState;

  //XOR shift algorithm 
  num ^= num << 13;
  num ^= num >> 17;
  num ^= num << 5;

  randState = num; 
  return num;
}

//generate 64bity pseudo legal numbers
U64 getRandU64() {
  //define 4 random numbers
  U64 n1, n2, n3, n4;

  n1 = (U64)(getRandU32()) & 0xFFFF;
  n2 = (U64)(getRandU32()) & 0xFFFF;
  n3 = (U64)(getRandU32()) & 0xFFFF;
  n4 = (U64)(getRandU32()) & 0xFFFF;

  return n1 | (n2 << 16) | (n3 << 32) | (n4 << 48);
}

//generate magic number candidate
U64 generateMagicNum() {
  return getRandU64() & getRandU64() & getRandU64();
}

/************ Define Bit Macros ************/
#define setBit(bitboard, square) ((bitboard) |= (1ULL << (square)))
#define getBit(bitboard, square) ((bitboard) & (1ULL << (square)))
#define popBit(bitboard, square) ((bitboard) &= ~(1ULL << (square)))

//count bits within bitboard
static inline int countBits(U64 bitboard) {
  int count = 0;
  
  //consecutively reset least significant first bit in bitboard
  while(bitboard) {
    count++;
    bitboard &= bitboard - 1;
  }

  return count;
}

//get least significant first bit index 
static inline int getLSBindex(U64 bitboard) { 
  if(bitboard){
    //count trailing bits before lsb
    return countBits((bitboard &= -bitboard) -1);
  } else return -1;
}

/************ Zobrist Hashing ************/
//random piece keys [piece][square] enpassant[square]
U64 pieceKeys[12][64];
U64 enpassantKeys[64];
U64 castlingKeys[16];
U64 sideKey; 

void initRandKeys() { 
  randState = 1804289383;
  for(int piece = P; piece <= k; piece++) { 
    for(int square = 0; square < 64; square++) { 
      pieceKeys[piece][square] = getRandU64();
   }
  }

  for(int square = 0; square < 64; square++) { 
    enpassantKeys[square] = getRandU64();
  }

  for(int key = 0; key < 16; key++) { 
    castlingKeys[key] = getRandU64();
  }

  sideKey = getRandU64(); 
}


//generate "almost" unique position identifier aka hash key from scratch
U64 generateHashKey() { 
  U64 finalKey = 0ULL; 

  U64 bitboard;

  for(int piece = P; piece <= k; piece++) { 
    bitboard = bitboards[piece];
    while(bitboard) { 
      int square = getLSBindex(bitboard);
      finalKey ^= pieceKeys[piece][square];
      popBit(bitboard, square);
    }
  }

  if(enpassant != no_sq) finalKey ^= enpassantKeys[enpassant];

  finalKey ^= castlingKeys[castle];

  if(side == black) finalKey ^= sideKey;
  return finalKey;
}

/************ Input/Output ************/
void printBitboard(U64 bitboard) {
  printf("\n");
  
  for(int rank = 0; rank < 8; rank++) {
    for(int file = 0; file < 8; file++) {
      int square = rank * 8 + file;
      if(!file) printf("  %d ", 8 - rank);
      printf(" %d", getBit(bitboard, square) ? 1 : 0);
    }
    printf("\n");
  }
    printf("\n     a b c d e f g h\n\n");
    printf("     Bitboard: %llud\n\n", bitboard);
}

void printBoard() {
  printf("\n");
  for(int rank = 0; rank < 8; rank++) { 
    for(int file= 0; file < 8; file++) { 
      int square = rank * 8 + file;
      //define piece variable
      if(!file) printf(" %d ", 8 - rank);
      int piece = -1;
      
      for(int bbPiece = P; bbPiece <= k; bbPiece++) {
        if(getBit(bitboards[bbPiece], square)) piece = bbPiece;
      }
      printf(" %s", (piece == -1) ? "." : unicodePieces[piece]); 
    }
    printf("\n");
  }
  printf("\n    a b c d e f g h \n\n");

  printf("    STM:      %s\n", (!side && (side != -1)) ? "white" : "black");
  printf("    Enpassant:   %s\n", (enpassant != no_sq) ? squareCoordinates[enpassant] : "no");
  printf("    Castling:  %c%c%c%c\n\n", (castle & wk) ? 'K' : '-', 
                                        (castle & wq) ? 'Q' : '-', 
                                        (castle & bk) ? 'k' : '-',  
                                        (castle & bq) ? 'q' : '-'); 
  printf("    Hash Key: %llx\n", hashKey);
}

void parseFEN(char *fen) { 
  //reset board position and state variables
  memset(bitboards, 0ULL, sizeof(bitboards));
  memset(occupancy, 0ULL, sizeof(occupancy));
  side = 0; 
  enpassant = no_sq; 
  castle = 0;
  
  hashKey = 0ULL;
  repetitionIndex = 0;
  memset(repetitionTable, 0ULL, sizeof(repetitionTable));

  ply = 0;

  for(int rank = 0; rank < 8; rank++) {
    for(int file = 0; file < 8; file++) {
      int square = rank * 8 + file; 

      //match ascii pieces within FEN string
      if((*fen >= 'a' && *fen <= 'z') || (*fen >= 'A' && *fen <= 'Z')) {
        int piece = charPieces[*fen]; 
        //set piece on corresponding bitboard
        setBit(bitboards[piece], square);
        //increment pointer to FEN string
        fen++;
      }
      //match empty square numbers within FEN string
      if(*fen >= '0' && *fen <= '9') {
        //convert char 0 to int 0
        int offset = *fen - '0';

        int piece = -1;
        for(int bbPiece = P; bbPiece <= k; bbPiece++) {
          if(getBit(bitboards[bbPiece], square)) piece = bbPiece;
        }
        
        if(piece == -1) file--;

        file += offset; 
        fen++;
      }

      if(*fen == '/') fen++;
    }
  }
  //parse stm
  fen++;
  (*fen == 'w') ? (side = white) : (side = black);

  //parse castling rights
  fen += 2;
  while(*fen != ' ') {
 
    switch(*fen) {
      case 'K': castle |= wk; break;
      case 'Q': castle |= wq; break;
      case 'k': castle |= bk; break;
      case 'q': castle |= bq; break;
      case '-': break;
    }
    fen++;
  }

  //parse enpassant
  fen++;
  if(*fen != '-') {
    int file = fen[0] - 'a';
    int rank = 8 - (fen[1] - '0');
    enpassant = rank * 8 + file;
  } else enpassant = no_sq;
 
  for(int piece = P; piece <= K; piece++) {
    //populate white occupancy bitboard
    occupancy[white] |= bitboards[piece];
  }
  for(int piece = p; piece <= k; piece++) {
    occupancy[black] |= bitboards[piece];
  }

  occupancy[both] |= occupancy[white];
  occupancy[both] |= occupancy[black];

  hashKey = generateHashKey();

}

/************ Attacks ************/

//not file constants
const U64 notAfile = 18374403900871474942ULL;
const U64 notHfile = 9187201950435737471ULL;
const U64 notHGfile = 4557430888798830399ULL;
const U64 notABfile = 18229723555195321596ULL;

//relevant occupancy bit count for every square on board
const int bishopRelevantBits[64] = {
  6, 5, 5, 5, 5, 5, 5, 6,
  5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 7, 7, 7, 7, 5, 5,
  5, 5, 7, 9, 9, 7, 5, 5,
  5, 5, 7, 9, 9, 7, 5, 5,
  5, 5, 7, 7, 7, 7, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5,
  6, 5, 5, 5, 5, 5, 5, 6 
};

const int rookRelevantBits[64] = {
  12, 11, 11, 11, 11, 11, 11, 12,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  12, 11, 11, 11, 11, 11, 11, 12
};

//bishop, rook magic numbers
U64 rookMagicNums[64] = {
  0x8a80104000800020ULL,
  0x140002000100040ULL,
  0x2801880a0017001ULL,
  0x100081001000420ULL,
  0x200020010080420ULL,
  0x3001c0002010008ULL,
  0x8480008002000100ULL,
  0x2080088004402900ULL,
  0x800098204000ULL,
  0x2024401000200040ULL,
  0x100802000801000ULL,
  0x120800800801000ULL,
  0x208808088000400ULL,
  0x2802200800400ULL,
  0x2200800100020080ULL,
  0x801000060821100ULL,
  0x80044006422000ULL,
  0x100808020004000ULL,
  0x12108a0010204200ULL,
  0x140848010000802ULL,
  0x481828014002800ULL,
  0x8094004002004100ULL,
  0x4010040010010802ULL,
  0x20008806104ULL,
  0x100400080208000ULL,
  0x2040002120081000ULL,
  0x21200680100081ULL,
  0x20100080080080ULL,
  0x2000a00200410ULL,
  0x20080800400ULL,
  0x80088400100102ULL,
  0x80004600042881ULL,
  0x4040008040800020ULL,
  0x440003000200801ULL,
  0x4200011004500ULL,
  0x188020010100100ULL,
  0x14800401802800ULL,
  0x2080040080800200ULL,
  0x124080204001001ULL,
  0x200046502000484ULL,
  0x480400080088020ULL,
  0x1000422010034000ULL,
  0x30200100110040ULL,
  0x100021010009ULL,
  0x2002080100110004ULL,
  0x202008004008002ULL,
  0x20020004010100ULL,
  0x2048440040820001ULL,
  0x101002200408200ULL,
  0x40802000401080ULL,
  0x4008142004410100ULL,
  0x2060820c0120200ULL,
  0x1001004080100ULL,
  0x20c020080040080ULL,
  0x2935610830022400ULL,
  0x44440041009200ULL,
  0x280001040802101ULL,
  0x2100190040002085ULL,
  0x80c0084100102001ULL,
  0x4024081001000421ULL,
  0x20030a0244872ULL,
  0x12001008414402ULL,
  0x2006104900a0804ULL,
  0x1004081002402ULL
};

U64 bishopMagicNums[64] = { 
  0x40040844404084ULL,
  0x2004208a004208ULL,
  0x10190041080202ULL,
  0x108060845042010ULL,
  0x581104180800210ULL,
  0x2112080446200010ULL,
  0x1080820820060210ULL,
  0x3c0808410220200ULL,
  0x4050404440404ULL,
  0x21001420088ULL,
  0x24d0080801082102ULL,
  0x1020a0a020400ULL,
  0x40308200402ULL,
  0x4011002100800ULL,
  0x401484104104005ULL,
  0x801010402020200ULL,
  0x400210c3880100ULL,
  0x404022024108200ULL,
  0x810018200204102ULL,
  0x4002801a02003ULL,
  0x85040820080400ULL,
  0x810102c808880400ULL,
  0xe900410884800ULL,
  0x8002020480840102ULL,
  0x220200865090201ULL,
  0x2010100a02021202ULL,
  0x152048408022401ULL,
  0x20080002081110ULL,
  0x4001001021004000ULL,
  0x800040400a011002ULL,
  0xe4004081011002ULL,
  0x1c004001012080ULL,
  0x8004200962a00220ULL,
  0x8422100208500202ULL,
  0x2000402200300c08ULL,
  0x8646020080080080ULL,
  0x80020a0200100808ULL,
  0x2010004880111000ULL,
  0x623000a080011400ULL,
  0x42008c0340209202ULL,
  0x209188240001000ULL,
  0x400408a884001800ULL,
  0x110400a6080400ULL,
  0x1840060a44020800ULL,
  0x90080104000041ULL,
  0x201011000808101ULL,
  0x1a2208080504f080ULL,
  0x8012020600211212ULL,
  0x500861011240000ULL,
  0x180806108200800ULL,
  0x4000020e01040044ULL,
  0x300000261044000aULL,
  0x802241102020002ULL,
  0x20906061210001ULL,
  0x5a84841004010310ULL,
  0x4010801011c04ULL,
  0xa010109502200ULL,
  0x4a02012000ULL,
  0x500201010098b028ULL,
  0x8040002811040900ULL,
  0x28000010020204ULL,
  0x6000020202d0240ULL,
  0x8918844842082200ULL,
  0x4010011029020020ULL
};

//pawn attack table [side][square], //king, knight, bishop, rook attack table[square]
U64 pawn_attacks[2][64];
U64 knightAttacks[64];
U64 kingAttacks[64];
U64 bishopMasks[64];
U64 rookMasks[64];

//32K and 256K
U64 bishopAttacks[64][512];
U64 rookAttacks[64][4096];

U64 maskPawnAttacks(int side, int square) {
  //result attacks bitboard
  U64 attacks = 0ULL; 

  //piece bitboard
  U64 bitboard = 0ULL;
  
  setBit(bitboard, square); 

  //white pawns 
  if(!side) {
    if((bitboard >> 7) & notAfile) attacks |= (bitboard >> 7);
    if((bitboard >> 9) & notHfile) attacks |= (bitboard >> 9);
    //black pawns
  } else {
    if((bitboard << 7) & notHfile) attacks |= (bitboard << 7);
    if((bitboard << 9) & notAfile) attacks |= (bitboard << 9);
  }
  //return attack map
  return attacks;
}

U64 maskKnightAttacks(int square) { 
  
  U64 attacks = 0ULL;
  U64 bitboard = 0ULL;
  
  setBit(bitboard, square); 
  
  //knight attacks
  if((bitboard >> 15) & notAfile) attacks |= (bitboard >> 15);
  if((bitboard >> 17) & notHfile) attacks |= (bitboard >> 17);
  if((bitboard >> 10) & notHGfile) attacks |= (bitboard >> 10);
  if((bitboard >> 6) & notABfile) attacks |= (bitboard >> 6);

  if((bitboard << 15) & notHfile) attacks |= (bitboard << 15);
  if((bitboard << 17) & notAfile) attacks |= (bitboard << 17);
  if((bitboard << 10) & notABfile) attacks |= (bitboard << 10);
  if((bitboard << 6) & notHGfile) attacks |= (bitboard <<  6);
  return attacks;
}

U64 maskKingAttacks(int square) { 
  
  U64 attacks = 0ULL;
  U64 bitboard = 0ULL;
  
  setBit(bitboard, square); 
  
  //king attacks
  if(bitboard >> 8) attacks |= (bitboard >> 8);
  if((bitboard >> 9) & notHfile) attacks |= (bitboard >> 9);
  if((bitboard >> 7) & notAfile) attacks |= (bitboard >> 7);
  if((bitboard >> 1) & notHfile) attacks |= (bitboard >> 1);

  if(bitboard << 8) attacks |= (bitboard << 8);
  if((bitboard << 9) & notAfile) attacks |= (bitboard << 9);
  if((bitboard << 7) & notHfile) attacks |= (bitboard << 7);
  if((bitboard << 1) & notAfile) attacks |= (bitboard << 1);
  return attacks;
}

U64 maskBishopAttacks(int square) {

  U64 attacks = 0ULL;
  
  int rank, file;
  int targetRank = square / 8;
  int targetFile = square % 8;

  //mask relevant bishop occupancy bits
  for(rank = targetRank + 1, file = targetFile + 1; rank <= 6 && file <= 6; rank++, file++) attacks |= (1ULL << (rank * 8 + file));
  for(rank = targetRank - 1, file = targetFile + 1; rank >= 1 && file <= 6; rank--, file++) attacks |= (1ULL << (rank * 8 + file));
  for(rank = targetRank + 1, file = targetFile - 1; rank <= 6 && file >= 1; rank++, file--) attacks |= (1ULL << (rank * 8 + file));
  for(rank = targetRank - 1, file = targetFile - 1; rank >= 1 && file >= 1; rank--, file--) attacks |= (1ULL << (rank * 8 + file));

  return attacks; 
}

U64 bishopAttacksOtf(int square, U64 block) {

  U64 attacks = 0ULL;
  
  int rank, file;
  int targetRank = square / 8;
  int targetFile = square % 8;

  //mask relevant bishop occupancy bits
  for(rank = targetRank + 1, file = targetFile + 1; rank <= 7 && file <= 7; rank++, file++) {
    attacks |= (1ULL << (rank * 8 + file));
    if((1ULL << (rank * 8 + file)) & block) break;
  }

  for(rank = targetRank - 1, file = targetFile + 1; rank >= 0 && file <= 7; rank--, file++) {
    attacks |= (1ULL << (rank * 8 + file));
    if((1ULL << (rank * 8 + file)) & block) break;
  }

  for(rank = targetRank + 1, file = targetFile - 1; rank <= 7 && file >= 0; rank++, file--) {
    attacks |= (1ULL << (rank * 8 + file));
    if((1ULL << (rank * 8 + file)) & block) break;
  }

  for(rank = targetRank - 1, file = targetFile - 1; rank >= 0 && file >= 0; rank--, file--) {
    attacks |= (1ULL << (rank * 8 + file));
    if((1ULL << (rank * 8 + file)) & block) break;
  }

  return attacks; 
}

U64 maskRookAttacks(int square) { 
  U64 attacks = 0ULL; 

  int rank, file;
  int targetRank = square / 8;
  int targetFile = square % 8;
  
  for(rank = targetRank + 1; rank <=6; rank++) attacks |= (1ULL << (rank * 8 + targetFile));
  for(rank = targetRank - 1; rank >=1; rank--) attacks |= (1ULL << (rank * 8 + targetFile));
  for(file = targetFile + 1; file <=6; file++) attacks |= (1ULL << (targetRank * 8 + file));
  for(file = targetFile - 1; file >=1; file--) attacks |= (1ULL << (targetRank * 8 + file));
  
  return attacks;
}

U64 rookAttacksOtf(int square, U64 block) { 
  U64 attacks = 0ULL; 

  int rank, file;
  int targetRank = square / 8;
  int targetFile = square % 8;
  
  for(rank = targetRank + 1; rank <=7; rank++) {
    attacks |= (1ULL << (rank * 8 + targetFile));
    if((1ULL << (rank * 8 + targetFile)) & block) break;
  }

  for(rank = targetRank - 1; rank >=0; rank--) {
    attacks |= (1ULL << (rank * 8 + targetFile));
    if((1ULL << (rank * 8 + targetFile)) & block) break;
  }

  for(file = targetFile + 1; file <=7; file++) {
    attacks |= (1ULL << (targetRank * 8 + file));
    if((1ULL << (targetRank * 8 + file)) & block) break;
  }

  for(file = targetFile - 1; file >=0; file--) {
    attacks |= (1ULL << (targetRank * 8 + file));
    if((1ULL << (targetRank * 8 + file)) & block) break;
  }

  return attacks;
}

/************ Initialize Leaper Piece Attacks ************/
void initLeaperAttacks() {
  //loop over 64 board squares 
  for(int square = 0; square < 64; square++) {
    //init pawn attacks
    pawn_attacks[white][square] = maskPawnAttacks(white, square); 
    pawn_attacks[black][square] = maskPawnAttacks(black, square); 

    //init knight attacks
    knightAttacks[square] = maskKnightAttacks(square);

    //init king attacks
    kingAttacks[square] = maskKingAttacks(square);
  }
}

U64 setOccupancy(int index, int bitsInMask, U64 attackMask){
  U64 occupancy = 0ULL;

  //loop over the range of bits within attack mask
  for(int i = 0; i < bitsInMask; i++) {
    int square = getLSBindex(attackMask);
    popBit(attackMask, square);

    //make sure occupancy is on board
    if(index & (1 << i)) occupancy |= (1ULL << square);
  }
  return occupancy;
}

/************ Magics ************/
U64 findMagicNum(int square, int relevantBits, int bishop) { 
  //init occupancies, attack tables, used attacks, attack mask, occupancy indicies
  U64 occupancies[4096];
  U64 attacks[4096];
  U64 usedAttacks[4096];
  U64 attackMask = bishop ? maskBishopAttacks(square) : maskRookAttacks(square);
  int occupancyIndicies = 1 << relevantBits;

  for(int i = 0; i < occupancyIndicies; i++) { 
    occupancies[i] = setOccupancy(i, relevantBits, attackMask);
    attacks[i] = bishop ? bishopAttacksOtf(square, occupancies[i]) : rookAttacksOtf(square, occupancies[i]);
  }

  for(int rand = 0; rand < 100000000; rand++) {
    //generate magic number candidate
    U64 magicNum = generateMagicNum();

    //skip inappropriate magic numbers
    if(countBits((attackMask * magicNum) & 0xFF00000000000000) < 6) continue;
    
    //init used attacks
    memset(usedAttacks, 0ULL, sizeof(usedAttacks));

    //init index & fail flag
    int index, fail;

    //test magic index loop
    for(index = 0, fail = 0; !fail && index < occupancyIndicies; index++) {
      //init magic index
      int magicIndex = (int)((occupancies[index] * magicNum) >> (64 - relevantBits));
      
      //if magic index works, init used attacks
      if(usedAttacks[magicIndex] == 0ULL) usedAttacks[magicIndex] = attacks[index]; 
      else if(usedAttacks[magicIndex] != attacks[index]) fail = 1;
  }
    if(!fail) return magicNum;
  }
  printf("  Magic number fails!");
  return 0ULL;
}

//init magic numbers
void initMagicNum() {
  //loop over 64 board squares
  for(int square = 0; square < 64; square++) {
    rookMagicNums[square] = findMagicNum(square, rookRelevantBits[square], rook);
  }
  for(int square = 0; square < 64; square++) {
    bishopMagicNums[square] = findMagicNum(square, bishopRelevantBits[square], bishop);
  }
}

//init slider piece attack tables 
void initSliderAttacks(int bishop) { 
  for(int square = 0; square < 64; square++) {
    //init bishop and rook masks
    bishopMasks[square] = maskBishopAttacks(square);
    rookMasks[square] = maskRookAttacks(square);
    
    U64 attackMask = bishop ? bishopMasks[square] : rookMasks[square];
    
    //init relevant occupancy bit count 
    int relevantBitsCount = countBits(attackMask);

    //init occupancy indicies
    int occupancyIndicies = (1 << relevantBitsCount);

    for(int i = 0; i < occupancyIndicies; i++) { 
      if(bishop) {
        U64 occupancy = setOccupancy(i, relevantBitsCount, attackMask); 
        int magicIndex = occupancy * bishopMagicNums[square] >> (64-bishopRelevantBits[square]);
        bishopAttacks[square][magicIndex] = bishopAttacksOtf(square, occupancy);
      } else {
        U64 occupancy = setOccupancy(i, relevantBitsCount, attackMask); 
        int magicIndex = occupancy * rookMagicNums[square] >> (64-rookRelevantBits[square]);
        rookAttacks[square][magicIndex] = rookAttacksOtf(square, occupancy);
      }
    }
  }
}

static inline U64 getBishopAttacks(int square, U64 occupancy) { 
  //get bishop attacks assuming current board occupancy 
  occupancy &= bishopMasks[square];
  occupancy *= bishopMagicNums[square];
  occupancy >>= 64 - bishopRelevantBits[square];
  
  return bishopAttacks[square][occupancy];
}


static inline U64 getRookAttacks(int square, U64 occupancy) { 
  //get rook attacks assuming current board occupancy 
  occupancy &= rookMasks[square];
  occupancy *= rookMagicNums[square];
  occupancy >>= 64 - rookRelevantBits[square];
  
  return rookAttacks[square][occupancy];
}

static inline U64 getQueenAttacks(int square, U64 occupancy) {
  
  U64 result = 0ULL;
  U64 bishopOccupancy = occupancy;
  U64 rookOccupancy = occupancy;

  bishopOccupancy &= bishopMasks[square];
  bishopOccupancy *= bishopMagicNums[square];
  bishopOccupancy >>= 64 - bishopRelevantBits[square];

  result = bishopAttacks[square][bishopOccupancy];

  rookOccupancy &= rookMasks[square];
  rookOccupancy *= rookMagicNums[square];
  rookOccupancy >>= 64 - rookRelevantBits[square];

  result |= rookAttacks[square][rookOccupancy];

  return result;
}

//is current given square attacked by the current given side
static inline int isAttacked(int square, int side) { 

  //attacked by pawns
  if((side == white) && (pawn_attacks[black][square] & bitboards[P])) return 1; 
  if((side == black) && (pawn_attacks[white][square] & bitboards[p])) return 1; 

  //attacked by knights
  if(knightAttacks[square] & ((side == white) ? bitboards[N] : bitboards[n])) return 1;

  //attacked by bishops
  if(getBishopAttacks(square, occupancy[both]) & ((side == white) ? bitboards[B] : bitboards[b])) return 1;

  //attacked by rook
  if(getRookAttacks(square, occupancy[both]) & ((side == white) ? bitboards[R] : bitboards[r])) return 1;

  //attacked by queens
  if(getQueenAttacks(square, occupancy[both]) & ((side == white) ? bitboards[Q] : bitboards[q])) return 1;

  //attacked by kings
  if(kingAttacks[square] & ((side == white) ? bitboards[K] : bitboards[k])) return 1;

  //by default, returns false
  return 0;
}


//print attacked squares
void print_attacked(int side) {

  printf("\n");
  for(int rank = 0; rank < 8; rank++) {
    for(int file = 0; file < 8; file++) { 
      int square = rank * 8 + file;
      if(!file) printf("  %d ", 8 - rank);
      printf(" %d", (isAttacked(square, side)) ? 1 : 0);
    }
    printf("\n");
  }
   printf("\n     a b c d e f g h\n\n");
}

//binary move representation encoding                       hexadecimal constants
//0000 0000 0000 0000 0011 1111 source square               0x3f
//0000 0000 0000 1111 1100 0000 target square               0xfc0
//0000 0000 1111 0000 0000 0000 piece                       0xf000
//0000 1111 0000 0000 0000 0000 promoted piece              0xf0000
//0001 0000 0000 0000 0000 0000 capture flag                0x100000
//0010 0000 0000 0000 0000 0000 double pawn push flag       0x200000
//0100 0000 0000 0000 0000 0000 enpassant capture flag      0x400000
//1000 0000 0000 0000 0000 0000 castle flag                 0x800000

//encode move macro 
#define encodeMove(source, target, piece, promoted, capture, dpp, enpassant, castle) \
(source) | (target << 6) | (piece << 12) | (promoted << 16) | \
(capture << 20) | (dpp << 21) | (enpassant << 22) | (castle << 23)\

//define macros to extract items from move
#define getSource(move) (move & 0x3f)
#define getTarget(move) ((move & 0xfc0) >> 6)
#define getPiece(move) ((move & 0xf000) >> 12)
#define getPromoted(move) ((move & 0xf0000) >> 16)
#define getCapture(move) (move & 0x100000)
#define getDpp(move) (move & 0x200000)
#define getEnpassant(move) (move & 0x400000)
#define getCastle(move) (move & 0x800000)

//move list structure
typedef struct {
  int moves[256]; 
  int count; 
} moves;



static inline void addMove(moves *moveList, int move) {
  moveList->moves[moveList->count] = move;

  //increment move count
  moveList->count++;
}

//print move
void printMove(int move) { 
  if(getPromoted(move)) {
    printf("%s%s%c", squareCoordinates[getSource(move)], 
                     squareCoordinates[getTarget(move)],
                     promotedPieces[getPromoted(move)]);
  } else {
  printf("%s%s", squareCoordinates[getSource(move)], 
                     squareCoordinates[getTarget(move)]);
  }
}

//print move
void printMove_list(moves *moveList) { 

  if(!moveList->count) {
    printf("    No moves in the move list!\n");
    return;
  }

  printf("\n    move    piece   capture   double    enpassant   castle\n\n");

  for(int numMoves = 0; numMoves < moveList->count; numMoves++) {
    int move = moveList->moves[numMoves];
    printf("    %s%s%c   %s       %d         %d         %d           %d\n", 
           squareCoordinates[getSource(move)], 
           squareCoordinates[getTarget(move)],
           getPromoted(move) ? promotedPieces[getPromoted(move)] : ' ',
           unicodePieces[getPiece(move)],
           getCapture(move) ? 1 : 0,
           getDpp(move) ? 1 : 0,
           getEnpassant(move) ? 1 : 0,
           getCastle(move) ? 1 : 0);
  }
  printf("\n\n    Total number of moves: %d\n\n", moveList->count);
}

//preserve board state
#define copyBoard()                            \
  U64 bitboardsCopy[12], occupancyCopy[3];    \
  int sideCopy, enpassantCopy, castleCopy;   \
  memcpy(bitboardsCopy, bitboards, 96);        \
  memcpy(occupancyCopy, occupancy, 24);        \
  sideCopy = side;                             \
  enpassantCopy = enpassant;                   \
  castleCopy = castle;                         \
  U64 hashKeyCopy = hashKey;                   \

#define restoreBoard()                         \
  memcpy(bitboards, bitboardsCopy, 96);        \
  memcpy(occupancy, occupancyCopy, 24);        \
  side = sideCopy;                             \
  enpassant = enpassantCopy;                   \
  castle = castleCopy;                         \
  hashKey = hashKeyCopy;                       \

enum { allMoves, onlyCaptures };

//castling rights update contants
const int castlingRights[64] =  {
    7, 15, 15, 15,  3, 15, 15, 11,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    13, 15, 15, 15, 12, 15, 15, 14 
};

//make move on board
static inline int makeMove(int move, int move_flag) { 
  //quiet moves
  if(move_flag == allMoves) {
    //preserve board state
    copyBoard();

    //parse move
    int sourceSquare = getSource(move);
    int targetSquare = getTarget(move);
    int piece = getPiece(move);
    int promoted = getPromoted(move);
    int capture = getCapture(move);
    int dpp = getDpp(move);
    int enpass = getEnpassant(move);
    int castling = getCastle(move);
    
    //move piece (only works for quiet moves)
    popBit(bitboards[piece], sourceSquare);
    setBit(bitboards[piece], targetSquare);
    
    //hash piece  (remove piece from source square, add piece to target square)
    hashKey ^= pieceKeys[piece][sourceSquare]; 
    hashKey ^= pieceKeys[piece][targetSquare]; 

    //handling capture moves
    if(capture) { 
      int startPiece, endPiece;
      if(side == white) { 
        startPiece = p; 
        endPiece = k;
      } else {
        startPiece = P;
        endPiece = K;
      }

      for(int bbPiece = startPiece; bbPiece <= endPiece; bbPiece++) { 
        //if piece on target square, remove it from corresponding bitboard
        if(getBit(bitboards[bbPiece], targetSquare)) {
          popBit(bitboards[bbPiece], targetSquare);
          //remove piece from hash key
          hashKey ^= pieceKeys[bbPiece][targetSquare];
          break;
        }
        
      }
    }

    //handle pawn promotion
    if(promoted) {  
      //erase pawn from target square
      //popBit(bitboards[(side == white) ? P : p], targetSquare);
      if(side == white) {
        popBit(bitboards[P], targetSquare);
        hashKey ^= pieceKeys[P][targetSquare];
      } else {
        popBit(bitboards[p], targetSquare);
        hashKey ^= pieceKeys[p][targetSquare];
      }
      //setup promoted piece on board
      setBit(bitboards[promoted], targetSquare);
      hashKey ^= pieceKeys[promoted][targetSquare];
    }

    //handle enpassant moves
    if(enpass) { 
      (side == white) ? popBit(bitboards[p], targetSquare + 8) : popBit(bitboards[P], targetSquare - 8);
      if(side == white) { 
        popBit(bitboards[p], targetSquare + 8);
        hashKey ^= pieceKeys[p][targetSquare + 8];
      } else {
        popBit(bitboards[P], targetSquare - 8);
        hashKey ^= pieceKeys[P][targetSquare - 8];
      }
    }

    if(enpassant != no_sq) hashKey ^= enpassantKeys[enpassant];
    enpassant = no_sq;

    //handle double pawn push
    if(dpp) { 
      (side == white) ? (enpassant = targetSquare + 8) : (enpassant = targetSquare - 8);
      if(side == white) { 
        enpassant = targetSquare + 8;
        hashKey ^= enpassantKeys[targetSquare + 8];
      } else {
        enpassant = targetSquare - 8;
        hashKey ^= enpassantKeys[targetSquare - 8];
      }
    }

    if(castling) { 
      switch(targetSquare) {
        case g1:
          popBit(bitboards[R], h1);
          setBit(bitboards[R], f1); 

          hashKey ^= pieceKeys[R][h1];
          hashKey ^= pieceKeys[R][f1];
          break;
        case c1:
          popBit(bitboards[R], a1);
          setBit(bitboards[R], d1);

          hashKey ^= pieceKeys[R][a1];
          hashKey ^= pieceKeys[R][d1];
          break;
        case g8:
          popBit(bitboards[r], h8);
          setBit(bitboards[r], f8); 

          hashKey ^= pieceKeys[r][h8];
          hashKey ^= pieceKeys[r][f8];
          break;
        case c8:
          popBit(bitboards[r], a8);
          setBit(bitboards[r], d8); 

          hashKey ^= pieceKeys[r][a8];
          hashKey ^= pieceKeys[r][d8];
          break;
      }
    }

    //hash castling rights
    hashKey ^= castlingKeys[castle];

    //handle update castling rights
    castle &= castlingRights[sourceSquare];
    castle &= castlingRights[targetSquare];

    hashKey ^= castlingKeys[castle];

    //update occupancies
    memset(occupancy, 0ULL, 24);

    for(int bbPiece = P; bbPiece <= K; bbPiece++) { 
      occupancy[white] |= bitboards[bbPiece];
    }
    
    for(int bbPiece = p; bbPiece <= k; bbPiece++) { 
      occupancy[black] |= bitboards[bbPiece];
    }

    occupancy[both] |= occupancy[white];
    occupancy[both] |= occupancy[black];

    //change side
    side ^= 1;

    //hash side
    hashKey ^= sideKey;


    //debug hash key incremental update
    //U64 hashFromScratch = generateHashKey();
  
    ////in case the hash key built from scratch doesn't match
    ////the one that was incrementally updated we interrupt execution
    //if(hashKey != hashFromScratch) {
    //  printf("\n\nMake move\n");
    //  printf("move: "); printMove(move);
    //  printBoard();
    //  printf("Hash key should be: %llx\n", hashFromScratch);
    //  getchar();
    //}

    //make sure the king isn't in check
    if(isAttacked((side == white) ? getLSBindex(bitboards[k]) : getLSBindex(bitboards[K]) , side)) { 
      restoreBoard();
      return 0; 
    } else {
      return 1;
    }
  } else {
    if(getCapture(move)) makeMove(move, allMoves);
    else return 0;
  }
}

//generate all moves
static inline void generateMoves(moves *moveList) { 
  //define initial variables
  moveList->count = 0;
  int sourceSquare, targetSquare; 
  U64 bitboard, attacks;

  for(int piece = P; piece <= k; piece++) { 
    bitboard = bitboards[piece]; 
    //generate white pawn and white king castling moves
    if(side == white) { 
      if(piece==P) {
        //loop over white pawns within white pawn bb
        while(bitboard) {
          sourceSquare = getLSBindex(bitboard);
          targetSquare = sourceSquare - 8;

          //generate quiet pawn moves (does not take)
          if(!(targetSquare < a8) && !getBit(occupancy[both], targetSquare)) {
            //pawn promotion
            if(sourceSquare >= a7 && sourceSquare <= h7){
              addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, Q, 0, 0, 0, 0));
              addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, R, 0, 0, 0, 0));
              addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, B, 0, 0, 0, 0));
              addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, N, 0, 0, 0, 0));
            } else {
              //one square ahead pawn move
              addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, 0, 0, 0, 0, 0));
              //two square ahead pawn move
              if((sourceSquare >= a2 && sourceSquare <= h2) && !getBit(occupancy[both], targetSquare-8)) { 
              addMove(moveList, encodeMove(sourceSquare, (targetSquare - 8), piece, 0, 0, 1, 0, 0));
             
              }
            }
          }
          // init pawn attacks bitboard
          attacks = pawn_attacks[side][sourceSquare] & occupancy[black];

          //generate pawn capture moves
          while(attacks) { 
            //init target square
            targetSquare = getLSBindex(attacks); 
           if(sourceSquare >= a7 && sourceSquare <= h7){
              addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, Q, 1, 0, 0, 0));
              addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, R, 1, 0, 0, 0));
              addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, B, 1, 0, 0, 0));
              addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, N, 1, 0, 0, 0));
            } else {
              addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, 0, 1, 0, 0, 0));
            }
              popBit(attacks, targetSquare); 
          }

          //generate enpassant captures
          if(enpassant != no_sq) { 
            U64 enpassantAttacks = pawn_attacks[side][sourceSquare] & (1ULL << enpassant);

            //make sure enpassant capture is available
            if(enpassantAttacks) { 
              int targetEnpassant = getLSBindex(enpassantAttacks);
              addMove(moveList, encodeMove(sourceSquare, targetEnpassant, piece, 0, 1, 0, 1, 0));
            }
          }
          popBit(bitboard, sourceSquare);
        }
      }
      //castling moves
      if(piece == K) { 
        //king side castle is available
        if(castle & wk) {
          //squares between king and king's rook empty
          if((!getBit(occupancy[both], f1)) && (!getBit(occupancy[both], g1))) {      
            //make sure king and the f1 square are not being attacked
            if((!isAttacked(e1, black)) && !isAttacked(f1, black)) { 
              addMove(moveList, encodeMove(e1, g1, piece, 0, 0, 0, 0, 1));
            }
          }
        }
        //queen side castle is available
        if(castle & wq) {
           //squares between king and king's rook empty
          if((!getBit(occupancy[both], d1)) && (!getBit(occupancy[both], c1)) && (!getBit(occupancy[both], b1))) {      
            //make sure king and the f1 square are not being attacked
            if((!isAttacked(e1, black)) && !isAttacked(d1, black)) { 
              addMove(moveList, encodeMove(e1, c1, piece, 0, 0, 0, 0, 1));
            }
          }
        }
      }
    }
    //generate black pawn and black king castling moves
    else {
       if(piece==p) {
        //loop over black pawns within black pawn bb
        while(bitboard) {
          sourceSquare = getLSBindex(bitboard);
          targetSquare = sourceSquare + 8;

          //generate quiet pawn moves (does not take)
          if(!(targetSquare > h1) && !getBit(occupancy[both], targetSquare)) {
            //pawn promotion
            if(sourceSquare >= a2 && sourceSquare <= h2){
              addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, q, 0, 0, 0, 0));
              addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, r, 0, 0, 0, 0));
              addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, b, 0, 0, 0, 0));
              addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, n, 0, 0, 0, 0));
            } else {
              //one square ahead pawn move
              addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, 0, 0, 0, 0, 0));
              //two square ahead pawn move
              if((sourceSquare >= a7 && sourceSquare <= h7) && !getBit(occupancy[both], targetSquare+8)) {
                addMove(moveList, encodeMove(sourceSquare, (targetSquare+8), piece, 0, 0, 1, 0, 0));
              }
            }
          }
          attacks = pawn_attacks[side][sourceSquare] & occupancy[white];

          //generate pawn capture moves
          while(attacks) { 
            //init target square
            targetSquare = getLSBindex(attacks); 
            
            if(sourceSquare >= a2 && sourceSquare <= h2){
              //move into a move list (tbd)
              addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, q, 1, 0, 0, 0));
              addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, r, 1, 0, 0, 0));
              addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, b, 1, 0, 0, 0));
              addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, n, 1, 0, 0, 0));
            } else {
              addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, 0, 1, 0, 0, 0));
            }
              popBit(attacks, targetSquare); 
          }

          //generate enpassant captures
          if(enpassant != no_sq) { 
            U64 enpassantAttacks = pawn_attacks[side][sourceSquare] & (1ULL << enpassant);

            //make sure enpassant capture is available
            if(enpassantAttacks) { 
              int targetEnpassant = getLSBindex(enpassantAttacks);
              addMove(moveList, encodeMove(sourceSquare, targetEnpassant, piece, 0, 1, 0, 1, 0));
              
            }
          }
          popBit(bitboard, sourceSquare);
        }
      }     
      //castling moves
      if(piece == k) { 
        //king side castle is available
        if(castle & bk) {
          //squares between king and king's rook empty
          if(!getBit(occupancy[both], f8) && !getBit(occupancy[both], g8)) {
            //make sure king and the f1 square are not being attacked
            if(!isAttacked(e8, white) && !isAttacked(f8, white)) { 
              addMove(moveList, encodeMove(e8, g8, piece, 0, 0, 0, 0, 1));
            }
          }
        }
        //queen side castle is available
        if(castle & bq) {
           //squares between king and king's rook empty
          if(!getBit(occupancy[both], d8) && !getBit(occupancy[both], c8) && !getBit(occupancy[both], b8)) {  
            //make sure king and the f8 square are not being attacked
            if(!isAttacked(e8, white) && !isAttacked(d8, white)) {
              addMove(moveList, encodeMove(e8, c8, piece, 0, 0, 0, 0, 1));
            }
          }
        }
      }
    }

    //generate knight moves
    if((side == white) ? piece == N : piece == n){
      while(bitboard) {
        sourceSquare = getLSBindex(bitboard); 
        attacks = knightAttacks[sourceSquare] & ((side==white) ? ~occupancy[white] : ~occupancy[black]);
        while(attacks) { 
          targetSquare = getLSBindex(attacks);
          if(!getBit(((side == white) ? occupancy[black] : occupancy[white]), targetSquare)) { 
              addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, 0, 0, 0, 0, 0));
          } else {
              addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, 0, 1, 0, 0, 0));
          }
          popBit(attacks, targetSquare);
        }
        popBit(bitboard, sourceSquare);
      }
    }

    //generate bishop moves
    if((side == white) ? piece == B : piece == b){
      while(bitboard) {
        sourceSquare = getLSBindex(bitboard); 
        attacks = getBishopAttacks(sourceSquare, occupancy[both]) & ((side==white) ? ~occupancy[white] : ~occupancy[black]);
        while(attacks) { 
          targetSquare = getLSBindex(attacks);
          if(!getBit(((side == white) ? occupancy[black] : occupancy[white]), targetSquare)) { 
              addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, 0, 0, 0, 0, 0));
          } else {
              addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, 0, 1, 0, 0, 0));
          }
          popBit(attacks, targetSquare);
        }
        popBit(bitboard, sourceSquare);
      }
    }

    //generate rook moves 
    if((side == white) ? piece == R : piece == r){
      while(bitboard) {
        sourceSquare = getLSBindex(bitboard); 
        attacks = getRookAttacks(sourceSquare, occupancy[both]) & ((side==white) ? ~occupancy[white] : ~occupancy[black]);
        while(attacks) { 
          targetSquare = getLSBindex(attacks);
          if(!getBit(((side == white) ? occupancy[black] : occupancy[white]), targetSquare)) { 
              addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, 0, 0, 0, 0, 0));
          } else {
              addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, 0, 1, 0, 0, 0));
          }
          popBit(attacks, targetSquare);
        }
        popBit(bitboard, sourceSquare);
      }
    }

    //generate queen moves
    if((side == white) ? piece == Q : piece == q){
      while(bitboard) {
        sourceSquare = getLSBindex(bitboard); 
        attacks = getQueenAttacks(sourceSquare, occupancy[both]) & ((side==white) ? ~occupancy[white] : ~occupancy[black]);
        while(attacks) { 
          targetSquare = getLSBindex(attacks);
          if(!getBit(((side == white) ? occupancy[black] : occupancy[white]), targetSquare)) { 
              addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, 0, 0, 0, 0, 0));
          } else {
              addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, 0, 1, 0, 0, 0));
          }
          popBit(attacks, targetSquare);
        }
        popBit(bitboard, sourceSquare);
      }
    }

    //generate king moves
    if((side == white) ? piece == K : piece == k){
      while(bitboard) {
        sourceSquare = getLSBindex(bitboard); 
        attacks = kingAttacks[sourceSquare] & ((side==white) ? ~occupancy[white] : ~occupancy[black]);
        while(attacks) { 
          targetSquare = getLSBindex(attacks);
          if(!getBit(((side == white) ? occupancy[black] : occupancy[white]), targetSquare)) { 
              addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, 0, 0, 0, 0, 0));
          } else {
              addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, 0, 1, 0, 0, 0));
          }
          popBit(attacks, targetSquare);
        }
        popBit(bitboard, sourceSquare);
      }
    }
  }
}

/************ Perft Testing ************/

//leaf nodes (num of positions reached during perft at a given depth)
long nodes; 

//perft driver
static inline void perftDriver(int depth) { 
  
  //recursion escape mechanic
  if(depth == 0) {
    nodes++;
    return; 
  }

  moves moveList[1];
  generateMoves(moveList); 
 
  //loop over generated moves
  for(int i = 0; i < moveList->count; i++) { 
    //preserve board state
    copyBoard(); 

    if(!makeMove(moveList->moves[i], allMoves)) continue;

    //call perft driver recursively
    perftDriver(depth - 1);
    restoreBoard();
  
  }
}

//perftest
void perftTest(int depth) { 
  
  printf("\nPerformance Test\n");
  moves moveList[1];
  generateMoves(moveList); 
  long start = getTimeMS();

  //loop over generated moves
  for(int i = 0; i < moveList->count; i++) { 
    //preserve board state
    copyBoard(); 

    if(!makeMove(moveList->moves[i], allMoves)) continue;

    //cumulative nodes
    long cumulativeNodes = nodes; 

    //call perft driver recursively
    perftDriver(depth - 1);

    //old nodes
    long oldNodes = nodes - cumulativeNodes;

    restoreBoard();

    printf("move: %s%s%c  nodes: %ld\n", squareCoordinates[getSource(moveList->moves[i])], 
                                         squareCoordinates[getTarget(moveList->moves[i])],
                                         promotedPieces[getPromoted(moveList->moves[i])],
                                         oldNodes);
  }

  printf("\nDepth: %d", depth);
  printf("\nNodes: %ld", nodes);
  printf("\nTime: %ld", getTimeMS() - start);


}

int materialScore[12] = {
  100, //wp
  300, //wn
  350, //wb 
  500, //wr
  1000, //wq
  10000, //wk 
  -100, //bp
  -300, //bn
  -350, //bb 
  -500, //br
  -1000, //bq
  -10000, //bk
  };

const int pawnScore[64] = 
{
    90,  90,  90,  90,  90,  90,  90,  90,
    30,  30,  30,  40,  40,  30,  30,  30,
    20,  20,  20,  30,  30,  30,  20,  20,
    10,  10,  10,  20,  20,  10,  10,  10,
     5,   5,  10,  20,  20,   5,   5,   5,
     0,   0,   0,   5,   5,   0,   0,   0,
     0,   0,   0, -10, -10,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0
};

const int knightScore[64] = 
{
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,  10,  10,   0,   0,  -5,
    -5,   5,  20,  20,  20,  20,   5,  -5,
    -5,  10,  20,  30,  30,  20,  10,  -5,
    -5,  10,  20,  30,  30,  20,  10,  -5,
    -5,   5,  20,  10,  10,  20,   5,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5, -10,   0,   0,   0,   0, -10,  -5
};

const int bishopScore[64] = 
{
     0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,  10,  10,   0,   0,   0,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,  10,   0,   0,   0,   0,  10,   0,
     0,  30,   0,   0,   0,   0,  30,   0,
     0,   0, -10,   0,   0, -10,   0,   0

};

const int rookScore[64] =
{
    50,  50,  50,  50,  50,  50,  50,  50,
    50,  50,  50,  50,  50,  50,  50,  50,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,   0,   0,  20,  20,   0,   0,   0

};

const int kingScore[64] = 
{
     0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   5,   5,   5,   5,   0,   0,
     0,   5,   5,  10,  10,   5,   5,   0,
     0,   5,  10,  20,  20,  10,   5,   0,
     0,   5,  10,  20,  20,  10,   5,   0,
     0,   0,   5,  10,  10,   5,   0,   0,
     0,   5,   5,  -5,  -5,   0,   5,   0,
     0,   0,   5,   0, -15,   0,  10,   0
};

const int mirrorScore[128] =
{
	a1, b1, c1, d1, e1, f1, g1, h1,
	a2, b2, c2, d2, e2, f2, g2, h2,
	a3, b3, c3, d3, e3, f3, g3, h3,
	a4, b4, c4, d4, e4, f4, g4, h4,
	a5, b5, c5, d5, e5, f5, g5, h5,
	a6, b6, c6, d6, e6, f6, g6, h6,
	a7, b7, c7, d7, e7, f7, g7, h7,
	a8, b8, c8, d8, e8, f8, g8, h8
};

static inline int evaluate() { 
  //static evaluation score
  int score = 0; 

  U64 bitboard;

  int piece, square; 
  
  for(int bbPiece = P; bbPiece <=k; bbPiece++) { 
    bitboard = bitboards[bbPiece];
    while(bitboard) { 
      piece = bbPiece;
      square = getLSBindex(bitboard);
      
      //score material weights
      score += materialScore[piece];
      switch(piece) {
        //evaluate white piece positional scores
        case P:  score += pawnScore[square]; break;
        case N:  score += knightScore[square]; break;
        case B:  score += bishopScore[square]; break;
        case R:  score += rookScore[square]; break;
        case K:  score += kingScore[square]; break; 
        //evaluate black piece positional scores
        case p:  score -= pawnScore[mirrorScore[square]]; break;
        case n:  score -= knightScore[mirrorScore[square]]; break;
        case b:  score -= bishopScore[mirrorScore[square]]; break;
        case r:  score -= rookScore[mirrorScore[square]]; break;
        case k:  score -= kingScore[mirrorScore[square]]; break; 
      }
      popBit(bitboard, square);
    }
  }
  //return final evaluation based on side
  return (side == white) ? score : -score;
}

/************ Search Position ************/
//score bounds for range of mating score
#define infinity 50000
#define mateValue 49000
#define mateScore 48000
/*
                          
    (Victims) Pawn Knight Bishop   Rook  Queen   King
  (Attackers)
        Pawn   105    205    305    405    505    605
      Knight   104    204    304    404    504    604
      Bishop   103    203    303    403    503    603
        Rook   102    202    302    402    502    602
       Queen   101    201    301    401    501    601
        King   100    200    300    400    500    600

*/

// MVV LVA [attacker][victim]
static int mvvLVA[12][12] = {
 	105, 205, 305, 405, 505, 605,  105, 205, 305, 405, 505, 605,
	104, 204, 304, 404, 504, 604,  104, 204, 304, 404, 504, 604,
	103, 203, 303, 403, 503, 603,  103, 203, 303, 403, 503, 603,
	102, 202, 302, 402, 502, 602,  102, 202, 302, 402, 502, 602,
	101, 201, 301, 401, 501, 601,  101, 201, 301, 401, 501, 601,
	100, 200, 300, 400, 500, 600,  100, 200, 300, 400, 500, 600,

	105, 205, 305, 405, 505, 605,  105, 205, 305, 405, 505, 605,
	104, 204, 304, 404, 504, 604,  104, 204, 304, 404, 504, 604,
	103, 203, 303, 403, 503, 603,  103, 203, 303, 403, 503, 603,
	102, 202, 302, 402, 502, 602,  102, 202, 302, 402, 502, 602,
	101, 201, 301, 401, 501, 601,  101, 201, 301, 401, 501, 601,
	100, 200, 300, 400, 500, 600,  100, 200, 300, 400, 500, 600
};

#define maxPly 64

int killerMoves[2][maxPly];
int historyMoves[12][64];

/*
      ================================
            Triangular PV table
      --------------------------------
        PV line: e2e4 e7e5 g1f3 b8c6
      ================================

           0    1    2    3    4    5
      
      0    m1   m2   m3   m4   m5   m6
      
      1    0    m2   m3   m4   m5   m6 
      
      2    0    0    m3   m4   m5   m6
      
      3    0    0    0    m4   m5   m6
       
      4    0    0    0    0    m5   m6
      
      5    0    0    0    0    0    m6
*/

int pvLength[maxPly];
int pvTable[maxPly][maxPly];

int followPV, scorePV;

/************ Transposition Table ************/
#define hashSize 0x400000
#define noHashEntry 100000
#define hashFlagExact 0
#define hashFlagAlpha 1
#define hashFlagBeta 2

typedef struct {
  U64 key; 
  int depth;
  int flag;
  int score;
} tt; //transposition table

tt transpositionTable[hashSize];

void clearTranspositionTable() { 
  for(int index = 0; index < hashSize; index++) { 
    transpositionTable[index].key = 0;
    transpositionTable[index].depth = 0;
    transpositionTable[index].flag = 0;
    transpositionTable[index].score = 0;
  }
}

static inline int readHashEntry(int alpha, int beta, int depth) { 
  //create a transposition table instance pointer to point to the hash table entry 
  //responsible for storing particular hash entry
  tt *hashEntry = &transpositionTable[hashKey % hashSize];
  if(hashEntry->key == hashKey) { 
    if(hashEntry->depth >= depth) {
      int score = hashEntry->score;
      
      if(score < -mateScore) score += ply;
      if(score > mateScore) score -= ply;
      if(hashEntry->flag == hashFlagExact) { 
        return score;
      }
      if((hashEntry->flag == hashFlagAlpha) && (score <= alpha)) { 
        return alpha;
      }
      if((hashEntry->flag == hashFlagBeta) && (score >= beta)) { 
        return beta;
      }
    }
  }
  return noHashEntry;
}

static inline void recordHash(int score, int depth, int hashFlag) {
  tt *hashEntry = &transpositionTable[hashKey % hashSize];
  
  if(score < -mateScore) score -= ply;
  if(score > mateScore) score += ply;
  hashEntry->key = hashKey;
  hashEntry->depth = depth;
  hashEntry->flag = hashFlag;
  hashEntry->score = score;
  
}
static inline void enablePvScoring(moves *moveList) {
  
  followPV = 0; 
  for(int i = 0; i < moveList->count; i++) { 
    if(pvTable[0][ply] == moveList->moves[i]) {
      scorePV = 1;
      followPV = 1;
    }
  }
}

// move ordering 
// 1. PV move
// 2. Captures in MVV/LVA
// 3. 1st Killer Move
// 4. 2nd Killer Move
// 5. History Moves
// 6. Unsorted Moves

static inline int scoreMove(int move) { 
  
  if(scorePV) { 
    if(pvTable[0][ply] == move) {
      scorePV = 0;
      return 20000;
    }
  }

  if(getCapture(move)) { 
      int targetPiece = P;
      int startPiece, endPiece;
      if(side == white) { startPiece = p; endPiece = k; } 
      else { startPiece = P; endPiece = K; }

      for(int bbPiece = startPiece; bbPiece <= endPiece; bbPiece++) { 
        //if piece on target square, remove it from corresponding bitboard
        if(getBit(bitboards[bbPiece], getTarget(move))) {
          targetPiece = bbPiece;
          break;
        }
        
      }
    return mvvLVA[getPiece(move)][targetPiece] + 10000;
  } else { 
    if(killerMoves[0][ply] == move) return 9000;   
    else if(killerMoves[1][ply] == move) return 8000;   
    else return historyMoves[getPiece(move)][getTarget(move)];
  }

  return 0;
}

static inline int sortMoves(moves *moveList) { 
  
  int moveScores[moveList->count]; 

  for(int i = 0; i < moveList->count; i++) { 
    moveScores[i] = scoreMove(moveList->moves[i]);
  }

  for(int current = 0; current < moveList->count; current++) { 
    for(int next = current + 1; next < moveList->count; next++) { 
      //compare current and next move scores
      if(moveScores[current] < moveScores[next]) { 
        int tempScore = moveScores[current];
        moveScores[current] = moveScores[next];
        moveScores[next] = tempScore;

        int tempMove = moveList->moves[current];
        moveList->moves[current] = moveList->moves[next];
        moveList->moves[next] = tempMove;
      }
    } 
  }

}
  
void printMoveScores(moves *moveList) { 
  printf("Move Scores\n\n");
  for(int i = 0; i < moveList->count; i++) { 
    printf("    move: "); 
    printMove(moveList->moves[i]);
    printf(" score: %d\n", scoreMove(moveList->moves[i]));
  }
}

static inline int isRepeating() { 
  for(int i = 0; i < repetitionIndex; i++) { 
    if(repetitionTable[i] == hashKey) {
      return 1;
    }
  }

  return 0;
}
static inline int quiescence(int alpha, int beta) { 
  
  if((nodes & 2047) == 0) communicate();

  nodes++;
  
  if(ply > maxPly - 1) return evaluate();

  int eval = evaluate();
  if(eval >= beta) return beta;
    //found a better move (PV node)
  if(eval > alpha) alpha = eval;

  moves moveList[1]; 
  generateMoves(moveList);

  sortMoves(moveList);

  //loop over moves within movelist
  for(int i = 0; i < moveList->count; i++) {
    copyBoard();
    ply++;
    
    repetitionTable[repetitionIndex] = hashKey;
    repetitionIndex++;

    //if illegal move
    if(makeMove(moveList->moves[i], onlyCaptures) == 0) { 
      ply--;
      repetitionIndex--;
      continue;
    }

       //score current move
    int score = -quiescence(-beta, -alpha);
    ply--;
    repetitionIndex--;
    restoreBoard();

    if(stopped == 1) return 0;

    //found a better move (PV node)
    if(score > alpha) {
      alpha = score;
      //fail-hard beta cutoff (node (move) fails high)
      if(score >= beta) return beta;
    }
  }
  return alpha;
}

const int fullDepthMoves = 4;
const int reductionLimit = 3;

//negamax alpha beta search
static inline int negamax(int alpha, int beta, int depth) { 
  int score;
  int hashFlag = hashFlagAlpha;

  int pvNode = (beta - alpha > 1);
  
  if(ply && isRepeating()) return 0;

  if(ply && (score = readHashEntry(alpha, beta, depth)) != noHashEntry && !pvNode) {
    return score; 
  }

  if((nodes & 2047) == 0) communicate();
  pvLength[ply] = ply;
  //recursion escape condition
  if(depth == 0) return quiescence(alpha, beta);
  //too deep in search -> overflow of arrays relying on max ply const
  if(ply > maxPly - 1) return evaluate();

  nodes++; 
  
  int inCheck = isAttacked((side == white) ? getLSBindex(bitboards[K]) : getLSBindex(bitboards[k]), side ^ 1);
  
  //increase search depth if king is in check 
  if(inCheck) depth++;
  int legalMoves = 0;
 
  // null move pruning
  if(depth >= 3 && inCheck == 0 && ply) {
    copyBoard();
    ply++;

    repetitionTable[repetitionIndex] = hashKey;
    repetitionIndex++;

    if(enpassant != no_sq) hashKey ^= enpassantKeys[enpassant];
    enpassant = no_sq;
    side ^= 1;
    hashKey ^= sideKey;
    //depth - 1 - R where R is the reduction limit
    score = -negamax(-beta, -beta + 1, depth - 3);
    
    ply--;
    repetitionIndex--;
    restoreBoard();
    if(stopped == 1) return 0;
    if(score >= beta) return beta;
  }

  moves moveList[1]; 
  generateMoves(moveList);

  if(followPV) enablePvScoring(moveList);

  sortMoves(moveList);

  int movesSearched = 0;

  //loop over moves within movelist
  for(int i = 0; i < moveList->count; i++) {
    copyBoard();
    ply++;
    
    repetitionTable[repetitionIndex] = hashKey;
    repetitionIndex++;

    //if illegal move
    if(makeMove(moveList->moves[i], allMoves) == 0) { 
      ply--;
      repetitionIndex--;
      continue;
    }

    //increment legal moves
    legalMoves++;

    //score current move
    if(movesSearched == 0) score = -negamax(-beta, -alpha, depth - 1);
    else {
      //condition to consider late move reduction
      if(movesSearched >= fullDepthMoves 
        && depth >= reductionLimit 
        && inCheck == 0 
        && getCapture(moveList->moves[i]) == 0 
        && getPromoted(moveList->moves[i]) == 0) {
        score = -negamax(-alpha - 1, -alpha, depth - 2);
      } else {
        score = alpha + 1;
      }
      //weak search to determine that all other moves are bad       
      if(score > alpha) { 
        score = -negamax(-alpha - 1, -alpha, depth - 1);
        //if algorithm finds out it was wrong, (one of the other moves was better than first PV move), 
        //research the move that has failed to be proved to be bad
        if((score > alpha) && (score < beta)) {
          score = -negamax(-beta, -alpha, depth - 1);
        }
      }
    }

    ply--;
    repetitionIndex--;
    restoreBoard();

    if(stopped == 1) return 0;
    movesSearched++; 

   //found a better move (PV node)
    if(score > alpha) {
      hashFlag = hashFlagExact;
      if(getCapture(moveList->moves[i]) == 0) { 
        historyMoves[getPiece(moveList->moves[i])][getTarget(moveList->moves[i])] += depth;
      }
      alpha = score;

      pvTable[ply][ply] = moveList->moves[i];
      //copy move from deeper ply into current ply line
      for(int nextPly = ply + 1; nextPly < pvLength[ply+1]; nextPly ++) {
        pvTable[ply][nextPly] = pvTable[ply+1][nextPly];
      }
      pvLength[ply] = pvLength[ply+1];
      //fail-hard beta cutoff (node (move) fails high)
      if(score >= beta) { 
        recordHash(beta, depth, hashFlagBeta);
        if(getCapture(moveList->moves[i]) == 0) { 
          killerMoves[1][ply] = killerMoves[0][ply];
          killerMoves[0][ply] = moveList->moves[i];
        }
        return beta;
      }
    }
  }

  if(legalMoves == 0) {
    //king is in check
    if(inCheck) {
      return -mateValue + ply;
    }

    else return 0;
  }

  recordHash(alpha, depth, hashFlag);
 //node fails low
  return alpha;

}

void searchPosition(int depth) { 
  
  int score = 0;
  nodes = 0;
  stopped = 0;

  followPV = 0;
  scorePV = 0; 

  //clear helper data structures for search
  memset(killerMoves, 0, sizeof(killerMoves));
  memset(historyMoves, 0, sizeof(historyMoves));
  memset(pvTable, 0, sizeof(pvTable));
  memset(pvLength, 0, sizeof(pvLength));

  //clearTranspositionTable();

  int alpha = -infinity;
  int beta = infinity;

  //iterative deepening
  for(int currentDepth = 1; currentDepth <= depth; currentDepth++) { 
    if(stopped == 1) break;
    followPV = 1;
    score = negamax(alpha, beta, currentDepth);
    
    //fell outside of the window, so try again with a full depth window 
    if((score <= alpha) || (score >= beta)) {
      alpha = -infinity;
      beta = infinity;
      continue;
    }
    
    //setup the window for the next iteration
    alpha = score - 50;
    beta = score + 50;
  
    if(score > -mateValue && score < -mateScore) {
      printf("info score mate %d depth %d nodes %ld time %d pv ", -(score + mateValue) / 2 - 1, currentDepth, nodes, getTimeMS() - startTime);
    } else if(score > mateScore && score < mateValue) {
      printf("info score mate %d depth %d nodes %ld time %d pv ", (mateValue - score) / 2 + 1, currentDepth, nodes, getTimeMS() - startTime);   
    } else
      printf("info score cp %d depth %d nodes %ld time %d pv ", score, currentDepth, nodes, getTimeMS() - startTime);
        
    for(int i = 0; i < pvLength[0]; i++) {
      printMove(pvTable[0][i]);
      printf(" ");
    }
    printf("\n");
 }

  printf("bestmove ");
  printMove(pvTable[0][0]);
  printf("\n");
  fflush(stdout);
}

/************ UCI ************/
//parse user/GUI move string input (ex. e7e8q)
int parseMove(char *moveString) { 
  moves moveList[1];
  generateMoves(moveList);
  
  int sourceSquare = (moveString[0] - 'a') + (8 - (moveString[1] - '0')) * 8;
  int targetSquare = (moveString[2] - 'a') + (8 - (moveString[3] - '0')) * 8;

  for(int i = 0; i < moveList->count; i++) { 
    int move = moveList->moves[i];

    //make sure source & target squares are available within the generated move
    if(sourceSquare == getSource(move) && targetSquare == getTarget(move)) { 
      int promotedPiece = getPromoted(move);
      if(promotedPiece) {  
        if((promotedPiece == Q || promotedPiece == q) && moveString[4] == 'q') { 
          return move;
        } else if((promotedPiece == R || promotedPiece == r) && moveString[4] == 'r') {
          return move; 
        } else if((promotedPiece == B || promotedPiece == b) && moveString[4] == 'b') {
          return move; 
        } else if((promotedPiece == N || promotedPiece == n) && moveString[4] == 'n') {
          return move; 
        }
        //loop on possible wrong promotion (ex. e7e8p, e7e8k, etc.) 
        continue;
      }
      return move;
    }
  }

  //return illegal move 
  return 0;
}

//parse UCI position command
void parsePosition(char *command) { 
  //shift pointer to the right where the next token begins
  command += 9;

  //init pointer to current character in command string
  char *currentChar = command;
  
  if(strncmp(command, "startpos", 8) == 0) parseFEN(startPosition);
  else { 
    currentChar = strstr(command, "fen");
    if(currentChar == NULL) { 
      parseFEN(startPosition);
    } else {
      currentChar += 4;
      parseFEN(currentChar);
    }
  }
  currentChar = strstr(command, "moves");
  if(currentChar != NULL) { 
    currentChar += 6; 
    //loop over moves within move string
    while(*currentChar) { 
      int move = parseMove(currentChar);
      if(move == 0) break; 
      repetitionIndex++;
      repetitionTable[repetitionIndex] = hashKey;

      makeMove(move, allMoves);

      //move current char pointer to end of current move
      while(*currentChar && *currentChar != ' ') currentChar++;

      //go to the next move
      currentChar++;
    }
  }
  //printBoard();
}
// reset time control variables
void reset_time_control()
{
    // reset timing
    quit = 0;
    movesToGo = 30;
    moveTime = -1;
    timer = -1;
    inc = 0;
    startTime = 0;
    stopTime = 0;
    timeSet = 0;
    stopped = 0;
}

// parse UCI command "go"
void parseGo(char *command)
{
    // reset time control
    reset_time_control();
    
    // init parameters
    int depth = -1;

    // init argument
    char *argument = NULL;

    // infinite search
    if ((argument = strstr(command,"infinite"))) {}

    // match UCI "binc" command
    if ((argument = strstr(command,"binc")) && side == black)
        // parse black time increment
        inc = atoi(argument + 5);

    // match UCI "winc" command
    if ((argument = strstr(command,"winc")) && side == white)
        // parse white time increment
        inc = atoi(argument + 5);

    // match UCI "wtime" command
    if ((argument = strstr(command,"wtime")) && side == white)
        // parse white time limit
        timer = atoi(argument + 6);

    // match UCI "btime" command
    if ((argument = strstr(command,"btime")) && side == black)
        // parse black time limit
        timer = atoi(argument + 6);

    // match UCI "movestogo" command
    if ((argument = strstr(command,"movestogo")))
        // parse number of moves to go
        movesToGo = atoi(argument + 10);

    // match UCI "movetime" command
    if ((argument = strstr(command,"movetime")))
        // parse amount of time allowed to spend to make a move
        moveTime = atoi(argument + 9);

    // match UCI "depth" command
    if ((argument = strstr(command,"depth")))
        // parse search depth
        depth = atoi(argument + 6);
    
    // run perft at given depth
    if ((argument = strstr(command,"perft"))) {
        // parse search depth
        depth = atoi(argument + 6);
        
        // run perft
        perftTest(depth);
        
        return;
    }

    // if move time is not available
    if(moveTime != -1)
    {
        // set time equal to move time
        timer = moveTime;

        // set moves to go to 1
        movesToGo = 1;
    }

    // init start time
    startTime = getTimeMS();

    // init search depth
    depth = depth;

    // if time control is available
    if(timer != -1)
    {
        // flag we're playing with time control
        timeSet = 1;

        // set up timing
        timer /= movesToGo;
        
        // lag compensation
        timer -= 50;
        
        // if time is up
        if (timer < 0)
        {
            // restore negative time to 0
            timer = 0;
            
            // inc lag compensation on 0+inc time controls
            inc -= 50;
            
            // timing for 0 seconds left and no inc
            if (inc < 0) inc = 1;
        }
        
        // init stoptime
        stopTime = startTime + timer + inc;        
    }

    // if depth is not available
    if(depth == -1)
        // set depth to 64 plies (takes ages to complete...)
        depth = 64;

    // print debug info
    printf("time: %d  inc: %d  start: %u  stop: %u  depth: %d  timeset:%d\n",
            timer, inc, startTime, stopTime, depth, timeSet);

    // search position
    searchPosition(depth);
}// GUI -> isready   
// readyok <-Engine 
// GUI -> ucinewgame
// "handshake" protocol within UCI

void uciLoop() {
    setbuf(stdin, NULL);
    setbuf(stdout, NULL);
    
    char input[2000];
    
    // print engine info
    printf("id name Kirin\n");
    printf("author name Strydr Silverberg\n");
    printf("uciok\n");
    
    while (1)
    {
        memset(input, 0, sizeof(input));
        fflush(stdout);
        
        if(!fgets(input, 2000, stdin))
            continue;
      
        if(input[0] == '\n')
            continue;
      
        if(strncmp(input, "isready", 7) == 0)
        {
            printf("readyok\n");
            continue;
        }
        
        else if(strncmp(input, "position", 8) == 0) { 
            parsePosition(input);
            clearTranspositionTable();
        }
        else if(strncmp(input, "ucinewgame", 10) == 0) {
            parsePosition("position startpos");
            clearTranspositionTable();
        }
        else if(strncmp(input, "go", 2) == 0)
            parseGo(input);
        
        else if(strncmp(input, "quit", 4) == 0)
            break;
        
        else if (strncmp(input, "uci", 3) == 0)
        {
          printf("id name Kirin\n");
          printf("author name Strydr Silverberg\n");
          printf("uciok\n");
        }
    }
}

/************ Init All ************/
//init all variables
void initAll() {
  initLeaperAttacks(); 
  initSliderAttacks(bishop);  
  initSliderAttacks(rook);
  initRandKeys();
  
  clearTranspositionTable();
}

/************ Main Driver ************/

int main() { 
  initAll(); 
  
  int debug = 0;
  
  if(debug) { 
    parseFEN(repetitions);
    printBoard(); 
    searchPosition(10);
    
    //info score cp 20 depth 7 nodes 56762 pv b1c3 d7d5 d2d4 b8c6 g1f3 g8f6 c1f4
  } else uciLoop();
  return 0;
}

