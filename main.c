// main chess engine file
// created by Strydr Silverberg
// sophomore year Colorado School of Mines
// use as you wish 

#include <stdio.h>

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

/************ Define Bit Macros ************/
#define set_bit(bitboard, square) (bitboard |= (1ULL << square))
#define get_bit(bitboard, square) (bitboard & (1ULL << square))
#define pop_bit(bitboard, square) (get_bit(bitboard, square) ? bitboard ^= (1ULL << square) : 0)

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
/************ Main Driver ************/
int main() { 
  //init leaper piece attacks
  init_leaper_attacks(); 

  for(int square = 0; square < 64; square++) { 
    print_bitboard(king_attacks[square]);
  }
  
  return 0;
}
