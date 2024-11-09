// created by Strydr Silverberg
// sophomore year Colorado School of Mines
// use as you wish 

#include <stdio.h>
#include <string.h>

/************ Define Bitboard Type ************/
#define U64 unsigned long long

//FEN debug positions
#define empty_board "8/8/8/8/8/8/8/8 w - - "
#define start_position "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1 "
#define tricky_position "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1 "
#define killer_position "rnbqkb1r/pp1p1pPp/8/2p1pP2/1P1P4/3P3P/P1P1P3/RNBQKBNR w KQkq e6 0 1"
#define cmk_position "r2q1rk1/ppp2ppp/2n1bn2/2b1p3/3pP3/3P1NPP/PPP1NPB1/R1BQ1RK1 b - - 0 9 "

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

enum { wk = 1, wq = 2, bk = 4, bq = 8};

const char* square_coordinates[] = {
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

//define piece bitboards, occupancy bitboards, side to move, enpassant square
U64 bitboards[12]; 
U64 occupancy[3];
int side;
int enpassant = no_sq;
int castle;

/************ Random Magic Numbers! ************/
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
//from Tord Romstad's proposal to find magic numbers
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
  //init bit count
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
  } else {
    return -1;
  }
}
/************ Print Bitboard ************/
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

U64 bishop_masks[64];
U64 rook_masks[64];

//32K and 256K
U64 bishop_attacks[64][512];
U64 rook_attacks[64][4096];

//pawn attack table [side][square]
U64 pawn_attacks[2][64];
//knight attack table[square]
U64 knight_attacks[64];
//king attack table[square]
U64 king_attacks[64];

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
  if((knight_attacks[square]) & ((side == white) ? bitboards[N] : bitboards[n])) return 1;

  //attacked by kings
  if((king_attacks[square]) & ((side == white) ? bitboards[K] : bitboards[k])) return 1;

  //attacked by bishops
  if((get_bishop_attacks(square, occupancy[both])) & ((side == white) ? bitboards[B] : bitboards[b])) return 1;

  //attacked by rook
  if((get_rook_attacks(square, occupancy[both])) & ((side == white) ? bitboards[R] : bitboards[r])) return 1;

  //attacked by queens
  if((get_queen_attacks(square, occupancy[both])) & ((side == white) ? bitboards[Q] : bitboards[q])) return 1;

  //by default, returns false
  return 0;
}

//generate all moves
static inline void generate_moves() { 
  //define initial variables
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
              //move into a move list (tbd)
              printf("pawn promotion: %s%sq\n", square_coordinates[source_square], square_coordinates[target_square]);
              printf("pawn promotion: %s%sr\n", square_coordinates[source_square], square_coordinates[target_square]);
              printf("pawn promotion: %s%sb\n", square_coordinates[source_square], square_coordinates[target_square]);
              printf("pawn promotion: %s%sn\n", square_coordinates[source_square], square_coordinates[target_square]);
            } else {
              //one square ahead pawn move
              printf("pawn push: %s%s\n", square_coordinates[source_square], square_coordinates[target_square]);
              //two square ahead pawn move
              if((source_square >= a2 && source_square <= h2) && !get_bit(occupancy[both], target_square-8)) {
                
                printf("double pawn push: %s%s\n", square_coordinates[source_square], square_coordinates[target_square - 8]);
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
              //move into a move list (tbd)
              printf("pawn capture promotion: %s%sq\n", square_coordinates[source_square], square_coordinates[target_square]);
              printf("pawn capture promotion: %s%sr\n", square_coordinates[source_square], square_coordinates[target_square]);
              printf("pawn capture promotion: %s%sb\n", square_coordinates[source_square], square_coordinates[target_square]);
              printf("pawn capture promotion: %s%sn\n", square_coordinates[source_square], square_coordinates[target_square]);
            } else {
              printf("pawn capture: %s%s\n", square_coordinates[source_square], square_coordinates[target_square]);
            }
              pop_bit(attacks, target_square); 
          }

          //generate enpassant captures
          if(enpassant != no_sq) { 
            U64 enpassant_attacks = pawn_attacks[side][source_square] & (1ULL << enpassant);

            //make sure enpassant capture is available
            if(enpassant_attacks) { 
              int target_enpassant = get_lsb_index(enpassant_attacks);
              printf("pawn enpassant capture: %s%s\n", square_coordinates[source_square], square_coordinates[target_enpassant]);
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
          if((!get_bit(occupancy[both], f1)) & (!get_bit(occupancy[both], g1))) {      
            //make sure king and the f1 square are not being attacked
            if((!is_attacked(e1, black)) && !is_attacked(g1, black)) { 
              printf("castling move: e1g1\n");
            }
          }
        }
        //queen side castle is available
        if(castle & wq) {
           //squares between king and king's rook empty
          if((!get_bit(occupancy[both], b1)) & (!get_bit(occupancy[both], c1)) & (!get_bit(occupancy[both], d1))) {      
            //make sure king and the f1 square are not being attacked
            if((!is_attacked(e1, black)) && !is_attacked(d1, black)) { 
              printf("castling move: e1c1\n");
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
              //move into a move list (tbd)
              printf("pawn promotion: %s%sq\n", square_coordinates[source_square], square_coordinates[target_square]);
              printf("pawn promotion: %s%sr\n", square_coordinates[source_square], square_coordinates[target_square]);
              printf("pawn promotion: %s%sb\n", square_coordinates[source_square], square_coordinates[target_square]);
              printf("pawn promotion: %s%sn\n", square_coordinates[source_square], square_coordinates[target_square]);
            } else {
              //one square ahead pawn move
              printf("pawn push: %s%s\n", square_coordinates[source_square], square_coordinates[target_square]);
              //two square ahead pawn move
              if((source_square >= a7 && source_square <= h7) && !get_bit(occupancy[both], target_square+8)) {
                
                printf("double pawn push: %s%s\n", square_coordinates[source_square], square_coordinates[target_square + 8]);
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
              printf("pawn capture promotion: %s%sq\n", square_coordinates[source_square], square_coordinates[target_square]);
              printf("pawn capture promotion: %s%sr\n", square_coordinates[source_square], square_coordinates[target_square]);
              printf("pawn capture promotion: %s%sb\n", square_coordinates[source_square], square_coordinates[target_square]);
              printf("pawn capture promotion: %s%sn\n", square_coordinates[source_square], square_coordinates[target_square]);
            } else {
              printf("pawn capture: %s%s\n", square_coordinates[source_square], square_coordinates[target_square]);
            }
              pop_bit(attacks, target_square); 
          }

          //generate enpassant captures
          if(enpassant != no_sq) { 
            U64 enpassant_attacks = pawn_attacks[side][source_square] & (1ULL << enpassant);

            //make sure enpassant capture is available
            if(enpassant_attacks) { 
              int target_enpassant = get_lsb_index(enpassant_attacks);
              printf("pawn enpassant capture: %s%s\n", square_coordinates[source_square], square_coordinates[target_enpassant]);
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
          if((!get_bit(occupancy[both], f8)) & (!get_bit(occupancy[both], g8))) {
            //make sure king and the f1 square are not being attacked
            if((!is_attacked(e8, white)) && !is_attacked(g8, white)) { 
              printf("castling move: e8g8\n");
            }
          }
        }
        //queen side castle is available
        if(castle & bq) {
           //squares between king and king's rook empty
          if((!get_bit(occupancy[both], b8)) & (!get_bit(occupancy[both], c8)) & (!get_bit(occupancy[both], d8))) {      
            //make sure king and the f8 square are not being attacked
            if((!is_attacked(e8, white)) && !is_attacked(d8, white)) { 
              printf("castling move: e8c8\n");
            }
          }
        }
      }
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

//print attacked squares
void print_attacked(int side) {

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

/************ Main Driver ************/
int main() { 
  init_all(); 

  parse_fen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPrBPPrP/R3K2R b KQkq - 0 1 ");
  print_board();
  
  generate_moves();
  
  return 0;
}
