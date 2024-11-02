// main chess engine file
// created by Strydr Silverberg
// sophomore year Colorado School of Mines
// use as you wish 

//establish initial positions
#include <stdio.h>
char whiteKnight[64] = "0100001000000000000000000000000000000000000000000000000000000000";
char whiteBishop[64] = "0010010000000000000000000000000000000000000000000000000000000000";
char whiteRook[64] = "1000000100000000000000000000000000000000000000000000000000000000";
char whiteKing[64] = "0000100000000000000000000000000000000000000000000000000000000000";
char whitePawn[64] = "0000000011111111000000000000000000000000000000000000000000000000";
char whiteQueen[64] = "0001000000000000000000000000000000000000000000000000000000000000";

char blackKnight[64] = "0000000000000000000000000000000000000000000000000000000001000010";
char blackBishop[64] = "0000000000000000000000000000000000000000000000000000000000100100";
char blackRook[64] = "0000000000000000000000000000000000000000000000000000000010000001";
char blackKing[64] = "0000000000000000000000000000000000000000000000000000000000001000";
char blackPawn[64] = "0000000000000000000000000000000000000000000000001111111100000000";
char blackQueen[64] = "0000000000000000000000000000000000000000000000000000000000010000";

void printPositions(char *pieceType) { 
  int row = 0;
  char column = 'a';
  for(int i = 0; i < 64; i++) { 
    if(i%8 == 0) {
      row++;
      column = 'a';
    }
    if(pieceType[i] == '1') {
      printf("%d%c, ", row, column); 
    }
    column++;
  }
  printf("\n");
}

void printPos() {
  printf("whiteKnight at: ");
  printPositions(whiteKnight); 
  printf("whiteBishop at: ");
  printPositions(whiteBishop);
  printf("whitePawn at: ");
  printPositions(whitePawn);
  printf("whiteKing at: ");
  printPositions(whiteKing);
  printf("whiteQueen at: ");
  printPositions(whiteQueen);
  printf("whiteRook at: ");
  printPositions(whiteRook);

  printf("blackKnight at: ");
  printPositions(blackKnight);
  printf("blackBishop at: ");
  printPositions(blackBishop);
  printf("blackPawn at: ");
  printPositions(blackPawn);
  printf("blackKing at: ");
  printPositions(blackKing);
  printf("blackQueen at: ");
  printPositions(blackQueen);
  printf("blackRook at: ");
  printPositions(blackRook);

}
int main() { 

  printPos();

}
