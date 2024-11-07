// main chess engine file
// created by Strydr Silverberg
// sophomore year Colorado School of Mines
// use as you wish 

#include <stdio.h>
#include <string.h>

/************ Define Bitboard Type ************/
#define U64 unsigned long long

/************ Board Squares ************/
enum {
  a8, b8, c8, d8, e8, f8, g8, h8,
  a7, b7, c7, d7, e7, f7, g7, h7,
  a6, b6, c6, d6, e6, f6, g6, h6,
  a5, b5, c5, d5, e5, f5, g5, h5,
  a4, b4, c4, d4, e4, f4, g4, h4,
  a3, b3, c3, d3, e3, f3, g3, h3,
  a2, b2, c2, d2, e2, f2, g2, h2,
  a1, b1, c1, d1, e1, f1, g1, h1
};

enum { white, black };

enum { rook, bishop };

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
#define set_bit(bitboard, square) (bitboard |= (1ULL << square))
#define get_bit(bitboard, square) (bitboard & (1ULL << square))
#define pop_bit(bitboard, square) (get_bit(bitboard, square) ? bitboard ^= (1ULL << square) : 0)

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

/************ Init All ************/
//init all variables

void init_all() {
  init_leaper_attacks(); 
  
  // now hardcoded
  // init_magic_num();
}

/************ Main Driver ************/
int main() { 
  init_all(); 

  for(int square = 0; square < 64; square++) {
    printf(" 0x%llxULL,\n", rook_magic_nums[square]);
  }
  printf("\n\n");
  for(int square = 0; square < 64; square++) {
    printf(" 0x%llxULL,\n", bishop_magic_nums[square]);
  }

  return 0;
}
