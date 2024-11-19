// created by Strydr Silverberg
// sophomore year Colorado School of Mines
// use as you wish 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

/************ Define Bitboard Type ************/
#define U64 unsigned long long

//FEN debug positions
#define empty_board "8/8/8/8/8/8/8/8 b - - "
#define start_position "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 "
#define tricky_position "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1 "
#define killer_position "rnbqkb1r/pp1p1pPp/8/2p1pP2/1P1P4/3P3P/P1P1P3/RNBQKBNR w KQkq e6 0 1"
#define cmk_position "r2q1rk1/ppp2ppp/2n1bn2/2b1p3/3pP3/3P1NPP/PPP1NPB1/R1BQ1RK1 b - - 0 9 "
#define new_pos "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10 "

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

const char *square_coordinates[] = {
  "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8",
  "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
  "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
  "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
  "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
  "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
  "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
  "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1"
};

char ascii_pieces[12] = "PNBRQKpnbrqk";
char *unicode_pieces[12]  = {"♙", "♘", "♗", "♖", "♕", "♔", "♟", "♞", "♝", "♜", "♛" ,"♚"};

//convert ascii character pieces to encoded constants
int char_pieces[] = {
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

char promoted_pieces[] = { 
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

/************ Random Magic Numbers! ************/
//from Tord Romstad's proposal to find magic numbers
//pseudo random number state
unsigned int rand_state = 1804289383;

//generate 32bit pseudo legal numbers
unsigned int get_randU32() {
  //get current state
  unsigned int num = rand_state;

  //XOR shift algorithm 
  num ^= num << 13;
  num ^= num >> 17;
  num ^= num << 5;

  rand_state = num; 
  return num;
}

//generate 64bity pseudo legal numbers
U64 get_randU64() {
  //define 4 random numbers
  U64 n1, n2, n3, n4;

  n1 = (U64)(get_randU32()) & 0xFFFF;
  n2 = (U64)(get_randU32()) & 0xFFFF;
  n3 = (U64)(get_randU32()) & 0xFFFF;
  n4 = (U64)(get_randU32()) & 0xFFFF;

  return n1 | (n2 << 16) | (n3 << 32) | (n4 << 48);
}

//generate magic number candidate
U64 generate_magic_num() {
  return get_randU64() & get_randU64() & get_randU64();
}

/************ Define Bit Macros ************/
#define set_bit(bitboard, square) ((bitboard) |= (1ULL << (square)))
#define get_bit(bitboard, square) ((bitboard) & (1ULL << (square)))
#define pop_bit(bitboard, square) ((bitboard) &= ~(1ULL << (square)))

//count bits within bitboard
static inline int count_bits(U64 bitboard) {
  int count = 0;
  
  //consecutively reset least significant first bit in bitboard
  while(bitboard) {
    count++;
    bitboard &= bitboard - 1;
  }

  return count;
}

//get least significant first bit index 
static inline int get_lsb_index(U64 bitboard) { 
  if(bitboard){
    //count trailing bits before lsb
    return count_bits((bitboard &= -bitboard) -1);
  } else return -1;
}

/************ Input/Output ************/
void print_bitboard(U64 bitboard) {
  printf("\n");
  
  for(int rank = 0; rank < 8; rank++) {
    for(int file = 0; file < 8; file++) {
      int square = rank * 8 + file;
      if(!file) printf("  %d ", 8 - rank);
      printf(" %d", get_bit(bitboard, square) ? 1 : 0);
    }
    printf("\n");
  }
    printf("\n     a b c d e f g h\n\n");
    printf("     Bitboard: %llud\n\n", bitboard);
}

void print_board() {
  printf("\n");
  for(int rank = 0; rank < 8; rank++) { 
    for(int file= 0; file < 8; file++) { 
      int square = rank * 8 + file;
      //define piece variable
      if(!file) printf(" %d ", 8 - rank);
      int piece = -1;
      
      for(int bb_piece = P; bb_piece <= k; bb_piece++) {
        if(get_bit(bitboards[bb_piece], square)) piece = bb_piece;
      }
      printf(" %s", (piece == -1) ? "." : unicode_pieces[piece]); 
    }
    printf("\n");
  }
  printf("\n    a b c d e f g h \n\n");

  printf("    STM:      %s\n", (!side && (side != -1)) ? "white" : "black");
  printf("    Enpassant:   %s\n", (enpassant != no_sq) ? square_coordinates[enpassant] : "no");
  printf("    Castling:  %c%c%c%c\n\n", (castle & wk) ? 'K' : '-', 
                                        (castle & wq) ? 'Q' : '-', 
                                        (castle & bk) ? 'k' : '-',  
                                        (castle & bq) ? 'q' : '-'); 
}

void parse_fen(char *fen) { 
  //reset board position and state variables
  memset(bitboards, 0ULL, sizeof(bitboards));
  memset(occupancy, 0ULL, sizeof(occupancy));
  side = 0; 
  enpassant = no_sq; 
  castle = 0;

  for(int rank = 0; rank < 8; rank++) {
    for(int file = 0; file < 8; file++) {
      int square = rank * 8 + file; 

      //match ascii pieces within FEN string
      if((*fen >= 'a' && *fen <= 'z') || (*fen >= 'A' && *fen <= 'Z')) {
        int piece = char_pieces[*fen]; 
        //set piece on corresponding bitboard
        set_bit(bitboards[piece], square);
        //increment pointer to FEN string
        fen++;
      }
      //match empty square numbers within FEN string
      if(*fen >= '0' && *fen <= '9') {
        //convert char 0 to int 0
        int offset = *fen - '0';

        int piece = -1;
        for(int bb_piece = P; bb_piece <= k; bb_piece++) {
          if(get_bit(bitboards[bb_piece], square)) piece = bb_piece;
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
}

/************ Attacks ************/

//not file constants
const U64 not_a_file = 18374403900871474942ULL;
const U64 not_h_file = 9187201950435737471ULL;
const U64 not_hg_file = 4557430888798830399ULL;
const U64 not_ab_file = 18229723555195321596ULL;

//relevant occupancy bit count for every square on board
const int bishop_relevant_bits[64] = {
  6, 5, 5, 5, 5, 5, 5, 6,
  5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 7, 7, 7, 7, 5, 5,
  5, 5, 7, 9, 9, 7, 5, 5,
  5, 5, 7, 9, 9, 7, 5, 5,
  5, 5, 7, 7, 7, 7, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5,
  6, 5, 5, 5, 5, 5, 5, 6 
};

const int rook_relevant_bits[64] = {
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
U64 rook_magic_nums[64] = {
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

U64 bishop_magic_nums[64] = { 
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
U64 knight_attacks[64];
U64 king_attacks[64];
U64 bishop_masks[64];
U64 rook_masks[64];

//32K and 256K
U64 bishop_attacks[64][512];
U64 rook_attacks[64][4096];

U64 mask_pawn_attacks(int side, int square) {
  //result attacks bitboard
  U64 attacks = 0ULL; 

  //piece bitboard
  U64 bitboard = 0ULL;
  
  set_bit(bitboard, square); 

  //white pawns 
  if(!side) {
    if((bitboard >> 7) & not_a_file) attacks |= (bitboard >> 7);
    if((bitboard >> 9) & not_h_file) attacks |= (bitboard >> 9);
    //black pawns
  } else {
    if((bitboard << 7) & not_h_file) attacks |= (bitboard << 7);
    if((bitboard << 9) & not_a_file) attacks |= (bitboard << 9);
  }
  //return attack map
  return attacks;
}

U64 mask_knight_attacks(int square) { 
  
  U64 attacks = 0ULL;
  U64 bitboard = 0ULL;
  
  set_bit(bitboard, square); 
  
  //knight attacks
  if((bitboard >> 15) & not_a_file) attacks |= (bitboard >> 15);
  if((bitboard >> 17) & not_h_file) attacks |= (bitboard >> 17);
  if((bitboard >> 10) & not_hg_file) attacks |= (bitboard >> 10);
  if((bitboard >> 6) & not_ab_file) attacks |= (bitboard >> 6);

  if((bitboard << 15) & not_h_file) attacks |= (bitboard << 15);
  if((bitboard << 17) & not_a_file) attacks |= (bitboard << 17);
  if((bitboard << 10) & not_ab_file) attacks |= (bitboard << 10);
  if((bitboard << 6) & not_hg_file) attacks |= (bitboard <<  6);
  return attacks;
}

U64 mask_king_attacks(int square) { 
  
  U64 attacks = 0ULL;
  U64 bitboard = 0ULL;
  
  set_bit(bitboard, square); 
  
  //king attacks
  if(bitboard >> 8) attacks |= (bitboard >> 8);
  if((bitboard >> 9) & not_h_file) attacks |= (bitboard >> 9);
  if((bitboard >> 7) & not_a_file) attacks |= (bitboard >> 7);
  if((bitboard >> 1) & not_h_file) attacks |= (bitboard >> 1);

  if(bitboard << 8) attacks |= (bitboard << 8);
  if((bitboard << 9) & not_a_file) attacks |= (bitboard << 9);
  if((bitboard << 7) & not_h_file) attacks |= (bitboard << 7);
  if((bitboard << 1) & not_a_file) attacks |= (bitboard << 1);
  return attacks;
}

U64 mask_bishop_attacks(int square) {

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

U64 bishop_attacks_otf(int square, U64 block) {

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

U64 mask_rook_attacks(int square) { 
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

U64 rook_attacks_otf(int square, U64 block) { 
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
void init_leaper_attacks() {
  //loop over 64 board squares 
  for(int square = 0; square < 64; square++) {
    //init pawn attacks
    pawn_attacks[white][square] = mask_pawn_attacks(white, square); 
    pawn_attacks[black][square] = mask_pawn_attacks(black, square); 

    //init knight attacks
    knight_attacks[square] = mask_knight_attacks(square);

    //init king attacks
    king_attacks[square] = mask_king_attacks(square);
  }
}

U64 set_occupancy(int index, int bits_in_mask, U64 attack_mask){
  U64 occupancy = 0ULL;

  //loop over the range of bits within attack mask
  for(int i = 0; i < bits_in_mask; i++) {
    int square = get_lsb_index(attack_mask);
    pop_bit(attack_mask, square);

    //make sure occupancy is on board
    if(index & (1 << i)) occupancy |= (1ULL << square);
  }
  return occupancy;
}

/************ Magics ************/
U64 find_magic_num(int square, int relevant_bits, int bishop) { 
  //init occupancies, attack tables, used attacks, attack mask, occupancy indicies
  U64 occupancies[4096];
  U64 attacks[4096];
  U64 used_attacks[4096];
  U64 attack_mask = bishop ? mask_bishop_attacks(square) : mask_rook_attacks(square);
  int occupancy_indicies = 1 << relevant_bits;

  for(int i = 0; i < occupancy_indicies; i++) { 
    occupancies[i] = set_occupancy(i, relevant_bits, attack_mask);
    attacks[i] = bishop ? bishop_attacks_otf(square, occupancies[i]) : rook_attacks_otf(square, occupancies[i]);
  }

  for(int rand = 0; rand < 100000000; rand++) {
    //generate magic number candidate
    U64 magic_num = generate_magic_num();

    //skip inappropriate magic numbers
    if(count_bits((attack_mask * magic_num) & 0xFF00000000000000) < 6) continue;
    
    //init used attacks
    memset(used_attacks, 0ULL, sizeof(used_attacks));

    //init index & fail flag
    int index, fail;

    //test magic index loop
    for(index = 0, fail = 0; !fail && index < occupancy_indicies; index++) {
      //init magic index
      int magic_index = (int)((occupancies[index] * magic_num) >> (64 - relevant_bits));
      
      //if magic index works, init used attacks
      if(used_attacks[magic_index] == 0ULL) used_attacks[magic_index] = attacks[index]; 
      else if(used_attacks[magic_index] != attacks[index]) fail = 1;
  }
    if(!fail) return magic_num;
  }
  printf("  Magic number fails!");
  return 0ULL;
}

//init magic numbers
void init_magic_num() {
  //loop over 64 board squares
  for(int square = 0; square < 64; square++) {
    rook_magic_nums[square] = find_magic_num(square, rook_relevant_bits[square], rook);
  }
  for(int square = 0; square < 64; square++) {
    bishop_magic_nums[square] = find_magic_num(square, bishop_relevant_bits[square], bishop);
  }
}

//init slider piece attack tables 
void init_slider_attacks(int bishop) { 
  for(int square = 0; square < 64; square++) {
    //init bishop and rook masks
    bishop_masks[square] = mask_bishop_attacks(square);
    rook_masks[square] = mask_rook_attacks(square);
    
    U64 attack_mask = bishop ? bishop_masks[square] : rook_masks[square];
    
    //init relevant occupancy bit count 
    int relevant_bits_count = count_bits(attack_mask);

    //init occupancy indicies
    int occupancy_indicies = (1 << relevant_bits_count);

    for(int i = 0; i < occupancy_indicies; i++) { 
      if(bishop) {
        U64 occupancy = set_occupancy(i, relevant_bits_count, attack_mask); 
        int magic_index = occupancy * bishop_magic_nums[square] >> (64-bishop_relevant_bits[square]);
        bishop_attacks[square][magic_index] = bishop_attacks_otf(square, occupancy);
      } else {
        U64 occupancy = set_occupancy(i, relevant_bits_count, attack_mask); 
        int magic_index = occupancy * rook_magic_nums[square] >> (64-rook_relevant_bits[square]);
        rook_attacks[square][magic_index] = rook_attacks_otf(square, occupancy);
      }
    }
  }
}

static inline U64 get_bishop_attacks(int square, U64 occupancy) { 
  //get bishop attacks assuming current board occupancy 
  occupancy &= bishop_masks[square];
  occupancy *= bishop_magic_nums[square];
  occupancy >>= 64 - bishop_relevant_bits[square];
  
  return bishop_attacks[square][occupancy];
}


static inline U64 get_rook_attacks(int square, U64 occupancy) { 
  //get rook attacks assuming current board occupancy 
  occupancy &= rook_masks[square];
  occupancy *= rook_magic_nums[square];
  occupancy >>= 64 - rook_relevant_bits[square];
  
  return rook_attacks[square][occupancy];
}

static inline U64 get_queen_attacks(int square, U64 occupancy) {
  
  U64 result = 0ULL;
  U64 bishop_occupancy = occupancy;
  U64 rook_occupancy = occupancy;

  bishop_occupancy &= bishop_masks[square];
  bishop_occupancy *= bishop_magic_nums[square];
  bishop_occupancy >>= 64 - bishop_relevant_bits[square];

  result = bishop_attacks[square][bishop_occupancy];

  rook_occupancy &= rook_masks[square];
  rook_occupancy *= rook_magic_nums[square];
  rook_occupancy >>= 64 - rook_relevant_bits[square];

  result |= rook_attacks[square][rook_occupancy];

  return result;
}

//is current given square attacked by the current given side
static inline int is_attacked(int square, int side) { 

  //attacked by pawns
  if((side == white) && (pawn_attacks[black][square] & bitboards[P])) return 1; 
  if((side == black) && (pawn_attacks[white][square] & bitboards[p])) return 1; 

  //attacked by knights
  if(knight_attacks[square] & ((side == white) ? bitboards[N] : bitboards[n])) return 1;

  //attacked by bishops
  if(get_bishop_attacks(square, occupancy[both]) & ((side == white) ? bitboards[B] : bitboards[b])) return 1;

  //attacked by rook
  if(get_rook_attacks(square, occupancy[both]) & ((side == white) ? bitboards[R] : bitboards[r])) return 1;

  //attacked by queens
  if(get_queen_attacks(square, occupancy[both]) & ((side == white) ? bitboards[Q] : bitboards[q])) return 1;

  //attacked by kings
  if(king_attacks[square] & ((side == white) ? bitboards[K] : bitboards[k])) return 1;

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
      printf(" %d", (is_attacked(square, side)) ? 1 : 0);
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
#define encode_move(source, target, piece, promoted, capture, dpp, enpassant, castle) \
(source) | (target << 6) | (piece << 12) | (promoted << 16) | \
(capture << 20) | (dpp << 21) | (enpassant << 22) | (castle << 23)\

//define macros to extract items from move
#define get_source(move) (move & 0x3f)
#define get_target(move) ((move & 0xfc0) >> 6)
#define get_piece(move) ((move & 0xf000) >> 12)
#define get_promoted(move) ((move & 0xf0000) >> 16)
#define get_capture(move) (move & 0x100000)
#define get_dpp(move) (move & 0x200000)
#define get_enpassant(move) (move & 0x400000)
#define get_castle(move) (move & 0x800000)

//move list structure
typedef struct {
  int moves[256]; 
  int count; 
} moves;

static inline void add_move(moves *move_list, int move) {
  move_list->moves[move_list->count] = move;

  //increment move count
  move_list->count++;
}

//print move
void print_move(int move) { 
  printf("%s%s%c", square_coordinates[get_source(move)], 
                     square_coordinates[get_target(move)],
                     promoted_pieces[get_promoted(move)]);
}

//print move
void print_move_list(moves *move_list) { 

  if(!move_list->count) {
    printf("    No moves in the move list!\n");
    return;
  }

  printf("\n    move    piece   capture   double    enpassant   castle\n\n");

  for(int numMoves = 0; numMoves < move_list->count; numMoves++) {
    int move = move_list->moves[numMoves];
    printf("    %s%s%c   %s       %d         %d         %d           %d\n", 
           square_coordinates[get_source(move)], 
           square_coordinates[get_target(move)],
           get_promoted(move) ? promoted_pieces[get_promoted(move)] : ' ',
           unicode_pieces[get_piece(move)],
           get_capture(move) ? 1 : 0,
           get_dpp(move) ? 1 : 0,
           get_enpassant(move) ? 1 : 0,
           get_castle(move) ? 1 : 0);
  }
  printf("\n\n    Total number of moves: %d\n\n", move_list->count);
}

//preserve board state
#define copy_board()                            \
  U64 bitboards_copy[12], occupancy_copy[3];    \
  int side_copy, enpassant_copy, castle_copy;   \
  memcpy(bitboards_copy, bitboards, 96);        \
  memcpy(occupancy_copy, occupancy, 24);        \
  side_copy = side;                             \
  enpassant_copy = enpassant;                   \
  castle_copy = castle;                         \

#define restore_board()                         \
  memcpy(bitboards, bitboards_copy, 96);        \
  memcpy(occupancy, occupancy_copy, 24);        \
  side = side_copy;                             \
  enpassant = enpassant_copy;                   \
  castle = castle_copy;                         \

enum { all_moves, only_captures };

//castling rights update contants
const int castling_rights[64] =  {
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
static inline int make_move(int move, int move_flag) { 
  //quiet moves
  if(move_flag == all_moves) {
    //preserve board state
    copy_board();

    //parse move
    int source_square = get_source(move);
    int target_square = get_target(move);
    int piece = get_piece(move);
    int promoted = get_promoted(move);
    int capture = get_capture(move);
    int dpp = get_dpp(move);
    int enpass = get_enpassant(move);
    int castling = get_castle(move);
    
    //move piece (only works for quiet moves)
    pop_bit(bitboards[piece], source_square);
    set_bit(bitboards[piece], target_square);
    
    //handling capture moves
    if(capture) { 
      int start_piece, end_piece;
      if(side == white) { 
        start_piece = p; 
        end_piece = k;
      } else {
        start_piece = P;
        end_piece = K;
      }

      for(int bb_piece = start_piece; bb_piece <= end_piece; bb_piece++) { 
        //if piece on target square, remove it from corresponding bitboard
        if(get_bit(bitboards[bb_piece], target_square)) {
          pop_bit(bitboards[bb_piece], target_square);
          break;
        }
        
      }
    }
    //handle pawn promotion
    if(promoted) {  
      //erase pawn from target square
      pop_bit(bitboards[(side == white) ? P : p], target_square);
      //setup promoted piece on board
      set_bit(bitboards[promoted], target_square);
    }
    //handle enpassant moves
    if(enpass) { 
      (side == white) ? pop_bit(bitboards[p], target_square + 8) : pop_bit(bitboards[P], target_square - 8);
    }
    enpassant = no_sq;
    //handle double pawn push
    if(dpp) { 
      (side == white) ? (enpassant = target_square + 8) : (enpassant = target_square - 8);
    }

    if(castling) { 
      switch(target_square) {
        case g1:
          pop_bit(bitboards[R], h1);
          set_bit(bitboards[R], f1); 
          break;
        case c1:
          pop_bit(bitboards[R], a1);
          set_bit(bitboards[R], d1); 
          break;
        case g8:
          pop_bit(bitboards[r], h8);
          set_bit(bitboards[r], f8); 
          break;
        case c8:
          pop_bit(bitboards[r], a8);
          set_bit(bitboards[r], d8); 
          break;
      }
    }

    //handle update castling rights
    castle &= castling_rights[source_square];
    castle &= castling_rights[target_square];

    //update occupancies
    memset(occupancy, 0ULL, 24);

    for(int bb_piece = P; bb_piece <= K; bb_piece++) { 
      occupancy[white] |= bitboards[bb_piece];
    }
    
    for(int bb_piece = p; bb_piece <= k; bb_piece++) { 
      occupancy[black] |= bitboards[bb_piece];
    }

    occupancy[both] |= occupancy[white];
    occupancy[both] |= occupancy[black];

    //change side
    side ^= 1;

    //make sure the king isn't in check
    if(is_attacked((side == white) ? get_lsb_index(bitboards[k]) : get_lsb_index(bitboards[K]) , side)) { 
      restore_board();
      return 0; 
    } else {
      return 1;
    }
  } else {
    if(get_capture(move)) make_move(move, all_moves);
    else return 0;
  }
}

//generate all moves
static inline void generate_moves(moves *move_list) { 
  //define initial variables
  move_list->count = 0;
  int source_square, target_square; 
  U64 bitboard, attacks;

  for(int piece = P; piece <= k; piece++) { 
    bitboard = bitboards[piece]; 
    //generate white pawn and white king castling moves
    if(side == white) { 
      if(piece==P) {
        //loop over white pawns within white pawn bb
        while(bitboard) {
          source_square = get_lsb_index(bitboard);
          target_square = source_square - 8;

          //generate quiet pawn moves (does not take)
          if(!(target_square < a8) && !get_bit(occupancy[both], target_square)) {
            //pawn promotion
            if(source_square >= a7 && source_square <= h7){
              add_move(move_list, encode_move(source_square, target_square, piece, Q, 0, 0, 0, 0));
              add_move(move_list, encode_move(source_square, target_square, piece, R, 0, 0, 0, 0));
              add_move(move_list, encode_move(source_square, target_square, piece, B, 0, 0, 0, 0));
              add_move(move_list, encode_move(source_square, target_square, piece, N, 0, 0, 0, 0));
            } else {
              //one square ahead pawn move
              add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));
              //two square ahead pawn move
              if((source_square >= a2 && source_square <= h2) && !get_bit(occupancy[both], target_square-8)) { 
              add_move(move_list, encode_move(source_square, (target_square - 8), piece, 0, 0, 1, 0, 0));
             
              }
            }
          }
          // init pawn attacks bitboard
          attacks = pawn_attacks[side][source_square] & occupancy[black];

          //generate pawn capture moves
          while(attacks) { 
            //init target square
            target_square = get_lsb_index(attacks); 
           if(source_square >= a7 && source_square <= h7){
              add_move(move_list, encode_move(source_square, target_square, piece, Q, 1, 0, 0, 0));
              add_move(move_list, encode_move(source_square, target_square, piece, R, 1, 0, 0, 0));
              add_move(move_list, encode_move(source_square, target_square, piece, B, 1, 0, 0, 0));
              add_move(move_list, encode_move(source_square, target_square, piece, N, 1, 0, 0, 0));
            } else {
              add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));
            }
              pop_bit(attacks, target_square); 
          }

          //generate enpassant captures
          if(enpassant != no_sq) { 
            U64 enpassant_attacks = pawn_attacks[side][source_square] & (1ULL << enpassant);

            //make sure enpassant capture is available
            if(enpassant_attacks) { 
              int target_enpassant = get_lsb_index(enpassant_attacks);
              add_move(move_list, encode_move(source_square, target_enpassant, piece, 0, 1, 0, 1, 0));
            }
          }
          pop_bit(bitboard, source_square);
        }
      }
      //castling moves
      if(piece == K) { 
        //king side castle is available
        if(castle & wk) {
          //squares between king and king's rook empty
          if((!get_bit(occupancy[both], f1)) && (!get_bit(occupancy[both], g1))) {      
            //make sure king and the f1 square are not being attacked
            if((!is_attacked(e1, black)) && !is_attacked(f1, black)) { 
              add_move(move_list, encode_move(e1, g1, piece, 0, 0, 0, 0, 1));
            }
          }
        }
        //queen side castle is available
        if(castle & wq) {
           //squares between king and king's rook empty
          if((!get_bit(occupancy[both], d1)) && (!get_bit(occupancy[both], c1)) && (!get_bit(occupancy[both], b1))) {      
            //make sure king and the f1 square are not being attacked
            if((!is_attacked(e1, black)) && !is_attacked(d1, black)) { 
              add_move(move_list, encode_move(e1, c1, piece, 0, 0, 0, 0, 1));
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
          source_square = get_lsb_index(bitboard);
          target_square = source_square + 8;

          //generate quiet pawn moves (does not take)
          if(!(target_square > h1) && !get_bit(occupancy[both], target_square)) {
            //pawn promotion
            if(source_square >= a2 && source_square <= h2){
              add_move(move_list, encode_move(source_square, target_square, piece, q, 0, 0, 0, 0));
              add_move(move_list, encode_move(source_square, target_square, piece, r, 0, 0, 0, 0));
              add_move(move_list, encode_move(source_square, target_square, piece, b, 0, 0, 0, 0));
              add_move(move_list, encode_move(source_square, target_square, piece, n, 0, 0, 0, 0));
            } else {
              //one square ahead pawn move
              add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));
              //two square ahead pawn move
              if((source_square >= a7 && source_square <= h7) && !get_bit(occupancy[both], target_square+8)) {
                add_move(move_list, encode_move(source_square, (target_square+8), piece, 0, 0, 1, 0, 0));
              }
            }
          }
          attacks = pawn_attacks[side][source_square] & occupancy[white];

          //generate pawn capture moves
          while(attacks) { 
            //init target square
            target_square = get_lsb_index(attacks); 
            
            if(source_square >= a2 && source_square <= h2){
              //move into a move list (tbd)
              add_move(move_list, encode_move(source_square, target_square, piece, q, 1, 0, 0, 0));
              add_move(move_list, encode_move(source_square, target_square, piece, r, 1, 0, 0, 0));
              add_move(move_list, encode_move(source_square, target_square, piece, b, 1, 0, 0, 0));
              add_move(move_list, encode_move(source_square, target_square, piece, n, 1, 0, 0, 0));
            } else {
              add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));
            }
              pop_bit(attacks, target_square); 
          }

          //generate enpassant captures
          if(enpassant != no_sq) { 
            U64 enpassant_attacks = pawn_attacks[side][source_square] & (1ULL << enpassant);

            //make sure enpassant capture is available
            if(enpassant_attacks) { 
              int target_enpassant = get_lsb_index(enpassant_attacks);
              add_move(move_list, encode_move(source_square, target_enpassant, piece, 0, 1, 0, 1, 0));
              
            }
          }
          pop_bit(bitboard, source_square);
        }
      }     
      //castling moves
      if(piece == k) { 
        //king side castle is available
        if(castle & bk) {
          //squares between king and king's rook empty
          if(!get_bit(occupancy[both], f8) && !get_bit(occupancy[both], g8)) {
            //make sure king and the f1 square are not being attacked
            if(!is_attacked(e8, white) && !is_attacked(f8, white)) { 
              add_move(move_list, encode_move(e8, g8, piece, 0, 0, 0, 0, 1));
            }
          }
        }
        //queen side castle is available
        if(castle & bq) {
           //squares between king and king's rook empty
          if(!get_bit(occupancy[both], d8) && !get_bit(occupancy[both], c8) && !get_bit(occupancy[both], b8)) {  
            //make sure king and the f8 square are not being attacked
            if(!is_attacked(e8, white) && !is_attacked(d8, white)) {
              add_move(move_list, encode_move(e8, c8, piece, 0, 0, 0, 0, 1));
            }
          }
        }
      }
    }

    //generate knight moves
    if((side == white) ? piece == N : piece == n){
      while(bitboard) {
        source_square = get_lsb_index(bitboard); 
        attacks = knight_attacks[source_square] & ((side==white) ? ~occupancy[white] : ~occupancy[black]);
        while(attacks) { 
          target_square = get_lsb_index(attacks);
          if(!get_bit(((side == white) ? occupancy[black] : occupancy[white]), target_square)) { 
              add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));
          } else {
              add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));
          }
          pop_bit(attacks, target_square);
        }
        pop_bit(bitboard, source_square);
      }
    }

    //generate bishop moves
    if((side == white) ? piece == B : piece == b){
      while(bitboard) {
        source_square = get_lsb_index(bitboard); 
        attacks = get_bishop_attacks(source_square, occupancy[both]) & ((side==white) ? ~occupancy[white] : ~occupancy[black]);
        while(attacks) { 
          target_square = get_lsb_index(attacks);
          if(!get_bit(((side == white) ? occupancy[black] : occupancy[white]), target_square)) { 
              add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));
          } else {
              add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));
          }
          pop_bit(attacks, target_square);
        }
        pop_bit(bitboard, source_square);
      }
    }

    //generate rook moves 
    if((side == white) ? piece == R : piece == r){
      while(bitboard) {
        source_square = get_lsb_index(bitboard); 
        attacks = get_rook_attacks(source_square, occupancy[both]) & ((side==white) ? ~occupancy[white] : ~occupancy[black]);
        while(attacks) { 
          target_square = get_lsb_index(attacks);
          if(!get_bit(((side == white) ? occupancy[black] : occupancy[white]), target_square)) { 
              add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));
          } else {
              add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));
          }
          pop_bit(attacks, target_square);
        }
        pop_bit(bitboard, source_square);
      }
    }

    //generate queen moves
    if((side == white) ? piece == Q : piece == q){
      while(bitboard) {
        source_square = get_lsb_index(bitboard); 
        attacks = get_queen_attacks(source_square, occupancy[both]) & ((side==white) ? ~occupancy[white] : ~occupancy[black]);
        while(attacks) { 
          target_square = get_lsb_index(attacks);
          if(!get_bit(((side == white) ? occupancy[black] : occupancy[white]), target_square)) { 
              add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));
          } else {
              add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));
          }
          pop_bit(attacks, target_square);
        }
        pop_bit(bitboard, source_square);
      }
    }

    //generate king moves
    if((side == white) ? piece == K : piece == k){
      while(bitboard) {
        source_square = get_lsb_index(bitboard); 
        attacks = king_attacks[source_square] & ((side==white) ? ~occupancy[white] : ~occupancy[black]);
        while(attacks) { 
          target_square = get_lsb_index(attacks);
          if(!get_bit(((side == white) ? occupancy[black] : occupancy[white]), target_square)) { 
              add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));
          } else {
              add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));
          }
          pop_bit(attacks, target_square);
        }
        pop_bit(bitboard, source_square);
      }
    }
  }
}

/************ Perft Testing ************/
int get_time_ms() { 
  //get time in ms
  struct timeval time;
  gettimeofday(&time, NULL);
  return time.tv_sec * 1000 + time.tv_usec / 1000;
}

//leaf nodes (num of positions reached during perft at a given depth)
long nodes; 

//perft driver
static inline void perft_driver(int depth) { 
  
  //recursion escape mechanic
  if(depth == 0) {
    nodes++;
    return; 
  }

  moves move_list[1];
  generate_moves(move_list); 
 
  //loop over generated moves
  for(int i = 0; i < move_list->count; i++) { 
    //preserve board state
    copy_board(); 

    if(!make_move(move_list->moves[i], all_moves)) continue;

    //call perft driver recursively
    perft_driver(depth - 1);
    restore_board();
  }
}

//perftest
void perft_test(int depth) { 
  
  printf("\nPerformance Test\n");
  moves move_list[1];
  generate_moves(move_list); 
  long start = get_time_ms();

  //loop over generated moves
  for(int i = 0; i < move_list->count; i++) { 
    //preserve board state
    copy_board(); 

    if(!make_move(move_list->moves[i], all_moves)) continue;

    //cumulative nodes
    long cumulative_nodes = nodes; 

    //call perft driver recursively
    perft_driver(depth - 1);

    //old nodes
    long old_nodes = nodes - cumulative_nodes;

    restore_board();

    printf("move: %s%s%c  nodes: %ld\n", square_coordinates[get_source(move_list->moves[i])], 
                                         square_coordinates[get_target(move_list->moves[i])],
                                         promoted_pieces[get_promoted(move_list->moves[i])],
                                         old_nodes);
  }

  printf("\nDepth: %d", depth);
  printf("\nNodes: %ld", nodes);
  printf("\nTime: %ld", get_time_ms() - start);


}

int material_score[12] = {
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

const int pawn_score[64] = 
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

const int knight_score[64] = 
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

const int bishop_score[64] = 
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

const int rook_score[64] =
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

const int king_score[64] = 
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

const int mirror_score[128] =
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
  
  for(int bb_piece = P; bb_piece <=k; bb_piece++) { 
    bitboard = bitboards[bb_piece];
    while(bitboard) { 
      piece = bb_piece;
      square = get_lsb_index(bitboard);
      
      //score material weights
      score += material_score[piece];
      switch(piece) {
        //evaluate white piece positional scores
        case P:  score += pawn_score[square]; break;
        case N:  score += knight_score[square]; break;
        case B:  score += bishop_score[square]; break;
        case R:  score += rook_score[square]; break;
        case K:  score += king_score[square]; break; 
        //evaluate black piece positional scores
        case p:  score -= pawn_score[mirror_score[square]]; break;
        case n:  score -= knight_score[mirror_score[square]]; break;
        case b:  score -= bishop_score[mirror_score[square]]; break;
        case r:  score -= rook_score[mirror_score[square]]; break;
        case k:  score -= king_score[mirror_score[square]]; break; 
      }
      pop_bit(bitboard, square);
    }
  }
  //return final evaluation based on side
  return (side == white) ? score : -score;
}

/************ Search Position ************/
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
static int mvv_lva[12][12] = {
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

//half move counter
int ply;
int best_move; 

static inline int score_move(int move) { 
  
  if(get_capture(move)) { 
      int target_piece = P;
      int start_piece, end_piece;
      if(side == white) { start_piece = p; end_piece = k; } 
      else { start_piece = P; end_piece = K; }

      for(int bb_piece = start_piece; bb_piece <= end_piece; bb_piece++) { 
        //if piece on target square, remove it from corresponding bitboard
        if(get_bit(bitboards[bb_piece], get_target(move))) {
          target_piece = bb_piece;
          break;
        }
        
      }
    return mvv_lva[get_piece(move)][target_piece];
  } else { 
    
  }

  return 0;
}

void print_move_scores(moves *move_list) { 
      printf("Move Scores\n\n");
      for(int i = 0; i < move_list->count; i++) { 
      printf("    move: "); 
      print_move(move_list->moves[i]);
      printf(" score: %d\n", score_move(move_list->moves[i]));
    }
}

static inline int quiescence(int alpha, int beta) { 
  
  nodes++;

  int eval = evaluate();
  if(eval >= beta) return beta;
    //found a better move (PV node)
  if(eval > alpha) alpha = eval;

  moves move_list[1]; 
  generate_moves(move_list);

  //loop over moves within movelist
  for(int i = 0; i < move_list->count; i++) {
    copy_board();
    ply++;
    
    //if illegal move
    if(make_move(move_list->moves[i], only_captures) == 0) { 
      ply--;
      continue;
    }

       //score current move
    int score = -quiescence(-beta, -alpha);
    ply--;
    restore_board();

    //fail-hard beta cutoff (node (move) fails high)
    if(score >= beta) return beta;
    //found a better move (PV node)
    if(score > alpha) alpha = score;
  }
  return alpha;
}

//negamax alpha beta search
static inline int negamax(int alpha, int beta, int depth) { 
  
  //recursion escape condition
  if(depth == 0) return quiescence(alpha, beta);
  
  nodes++; 
  
  int check_state = is_attacked((side == white) ? get_lsb_index(bitboards[K]) : get_lsb_index(bitboards[k]), side ^ 1);
  int legal_moves = 0;
  int current_best;
  int old_alpha = alpha;

  moves move_list[1]; 
  generate_moves(move_list);

  //loop over moves within movelist
  for(int i = 0; i < move_list->count; i++) {
    copy_board();
    ply++;
    
    //if illegal move
    if(make_move(move_list->moves[i], all_moves) == 0) { 
      ply--;
      continue;
    }

    //increment legal moves
    legal_moves++;

    //score current move
    int score = -negamax(-beta, -alpha, depth - 1);
    ply--;
    restore_board();

    //fail-hard beta cutoff (node (move) fails high)
    if(score >= beta) return beta;
    //found a better move (PV node)
    if(score > alpha) {
      alpha = score;
      //if root move, associate best move with best score
      if(ply==0) current_best = move_list->moves[i];
    }
  }

  if(legal_moves == 0) {
    //king is in check
    if(check_state) return -49000 + ply;
    else return 0;
  }

  if(old_alpha != alpha) best_move = current_best;
  //node fails low
  return alpha;

}

void search_position(int depth) { 
  //find best move within a given position (using negamax algorithm)
  int score = negamax(-50000, 50000, depth);
  if(best_move) { 
    printf("info score cp %d depth %d nodes %ld\n", score, depth, nodes);
    printf("bestmove ");
    print_move(best_move);
    printf("\n");
  }
}

/************ UCI ************/
//parse user/GUI move string input (ex. e7e8q)
int parse_move(char *move_string) { 
  moves move_list[1];
  generate_moves(move_list);
  
  int source_square = (move_string[0] - 'a') + (8 - (move_string[1] - '0')) * 8;
  int target_square = (move_string[2] - 'a') + (8 - (move_string[3] - '0')) * 8;

  for(int i = 0; i < move_list->count; i++) { 
    int move = move_list->moves[i];

    //make sure source & target squares are available within the generated move
    if(source_square == get_source(move) && target_square == get_target(move)) { 
      int promoted_piece = get_promoted(move);
      if(promoted_piece) {  
        if((promoted_piece == Q || promoted_piece == q) && move_string[4] == 'q') { 
          return move;
        } else if((promoted_piece == R || promoted_piece == r) && move_string[4] == 'r') {
          return move; 
        } else if((promoted_piece == B || promoted_piece == b) && move_string[4] == 'b') {
          return move; 
        } else if((promoted_piece == N || promoted_piece == n) && move_string[4] == 'n') {
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
void parse_position(char *command) { 
  //shift pointer to the right where the next token begins
  command += 9;

  //init pointer to current character in command string
  char *current_char = command;
  
  if(strncmp(command, "startpos", 8) == 0) parse_fen(start_position);
  else { 
    current_char = strstr(command, "fen");
    if(current_char == NULL) { 
      parse_fen(start_position);
    } else {
      current_char += 4;
      parse_fen(current_char);
    }
  }
  current_char = strstr(command, "moves");
  if(current_char != NULL) { 
    current_char += 6; 
    //loop over moves within move string
    while(*current_char) { 
      int move = parse_move(current_char);
      if(move == 0) break; 
      make_move(move, all_moves);

      //move current char pointer to end of current move
      while(*current_char && *current_char != ' ') current_char++;

      //go to the next move
      current_char++;
    }
  }
  print_board();
}

//parse UCI "go" command, ex. "go depth 6"
void parse_go(char *command) { 
  int depth = -1;
  char *current_depth = NULL;
  if((current_depth = strstr(command, "depth"))) depth = atoi(current_depth + 6);
  
  //different time controls placeholder 
  else {
    depth = 5;
  }
  search_position(depth);
}

// GUI -> isready   
// readyok <-Engine 
// GUI -> ucinewgame
// "handshake" protocol within UCI

void uci_loop() { 
  //reset stdin & stdout buffers
  setbuf(stdin, NULL);
  setbuf(stdout, NULL);

  //define USER/GUI input buff(large for FEN strings and move strings"
  char input[2000];

  printf("id name ce\n");
  printf("author name Strydr Silverberg\n");
  printf("uciok\n");

  //main "game" loop
  while(1) { 
    memset(input, 0, sizeof(input));
    fflush(stdout);
    if(!fgets(input, 2000, stdin)) continue;
    if(input[0] == '\n') continue;
    if(!strncmp(input, "isready", 7)) {
      printf("readyok\n");
      continue;
    }
    else if(!strncmp(input, "position", 8)) parse_position(input);
    else if(!strncmp(input, "ucinewgame", 10)) parse_position("position startpos");
    else if(!strncmp(input, "go", 2)) parse_go(input);
    else if(!strncmp(input, "quit", 4)) break;
    else if(!strncmp(input, "uci", 3)) {
      printf("id name ce\n");
      printf("author name Strydr Silverberg\n");
      printf("uciok\n");
    }
  }
}

/************ Init All ************/
//init all variables
void init_all() {
  init_leaper_attacks(); 
  init_slider_attacks(bishop);  
  init_slider_attacks(rook);
}

/************ Main Driver ************/

int main() { 
  init_all(); 
  
  int debug = 1;
  
  if(debug) { 
    parse_fen(tricky_position);
    print_board(); 
    
    moves move_list[1];
    generate_moves(move_list);

    print_move_scores(move_list);
 
    //search_position(3);
  } else uci_loop();
  return 0;
}
