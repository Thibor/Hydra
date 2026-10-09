#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <immintrin.h>

#define MAX_PLY 64
#define MATE 32000
#define INF 32001
#define S8 signed __int8
#define U8 unsigned __int8
#define S16 signed __int16
#define U16 unsigned __int16
#define S32 signed __int32
#define S64 signed __int64
#define U64 unsigned __int64
#define FALSE 0
#define TRUE 1
#define NAME "Hydra"
#define VERSION "2026-08-02"
#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
#define FLIP(sq) ((sq)^0x38)
#define RANK_1_BB 0x00000000000000FFULL
#define RANK_2_BB 0x000000000000FF00ULL
#define RANK_3_BB 0x0000000000FF0000ULL
#define RANK_4_BB 0x00000000FF000000ULL
#define RANK_5_BB 0x000000FF00000000ULL
#define RANK_6_BB 0x0000FF0000000000ULL
#define RANK_7_BB 0x00FF000000000000ULL
#define RANK_8_BB 0xFF00000000000000ULL
#define FILE_A_BB 0x0101010101010101ULL
#define FILE_B_BB 0x0202020202020202ULL
#define FILE_C_BB 0x0404040404040404ULL
#define FILE_D_BB 0x0808080808080808ULL
#define FILE_E_BB 0x1010101010101010ULL
#define FILE_F_BB 0x2020202020202020ULL
#define FILE_G_BB 0x4040404040404040ULL
#define FILE_H_BB 0x8080808080808080ULL

enum Color { WHITE, BLACK, COLOR_NB };
enum Piece { WPAWN, WKNIGHT, WBISHOP, WROOK, WQUEEN, WKING, BPAWN, BKNIGHT, BBISHOP, BROOK, BQUEEN, BKING, PIECE_NB };
enum PieceType { PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, PT_NB };
enum Bound { UPPER, LOWER, EXACT };
enum File { FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H };
enum Rank { RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8 };
enum Castle { CASTLE_WK = 0b0001, CASTLE_WQ = 0b0010, CASTLE_BK = 0b0100, CASTLE_BQ = 0b1000 };
enum Square {
	SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
	SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
	SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
	SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
	SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
	SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6,
	SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
	SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8,
	SQ_NB
};

typedef struct {
	U8 color;
	U8 move50;
	U8 castleFlags;
	int historyCount;
	U64 pieces[PIECE_NB];
	U64 ep;
	U64 hash;
}Position;

typedef struct {
	U8 from;
	U8 to;
	U8 promo;
}Move;

typedef struct {
	Move move;
	Move killer1;
	Move killer2;
} Stack;

typedef struct {
	U64 hash;
	Move move;
	S16 score;
	U8 depth;
	U8 flag;
}TTEntry;

typedef struct {
	U64 hash;
	S16 score;
} TTEntryEval;

typedef struct {
	U8 post;
	U8 stop;
	U8 depthLimit;
	U64 timeStart;
	U64 timeLimit;
	U64 nodes;
	U64 nodesLimit;
}SearchInfo;

const int insufVal[PT_NB] = { 5,2,3,5,5,0 };
const int phaseVal[PT_NB] = { 0,1,1,2,4,0 };
U64 historyHash[1024];
U64 bbSquare[64];
U64 bbKnightAttack[64];
U64 bbKingAttack[64];
int hh[2][64][64];
U64 bishop_table[64][512];
U64 rook_table[64][4096];
U64 bishop_masks[64];
U64 rook_masks[64];
U64 zob_side;
U64 zob_castle[16];
U64 zob_piece[PIECE_NB][SQ_NB];
U64 ttSize;
U64 ttMask;
SearchInfo info;
Stack ss[128];
TTEntry* tt;
TTEntryEval* ttEval;
U64 bbFiles[8] = { FILE_A_BB, FILE_B_BB, FILE_C_BB, FILE_D_BB, FILE_E_BB, FILE_F_BB, FILE_G_BB, FILE_H_BB };
U64 bbRanks[8] = { RANK_1_BB, RANK_2_BB, RANK_3_BB, RANK_4_BB, RANK_5_BB, RANK_6_BB, RANK_7_BB, RANK_8_BB };
int mg_material[PT_NB] = { 82, 337, 365, 477, 1025, 0 };
int eg_material[PT_NB] = { 94, 281, 297, 512,  936, 0 };
int mx_material[PT_NB] = { 94, 337, 365, 512, 1025, 0 };

int boardCastle[64] = {
	13, 15, 15, 15, 12, 15, 15, 14,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	 7, 15, 15, 15,  3, 15, 15, 11
};

int mg_pawn_table[64] = {
	  0,   0,   0,   0,   0,   0,  0,   0,
	 98, 134,  61,  95,  68, 126, 34, -11,
	 -6,   7,  26,  31,  65,  56, 25, -20,
	-14,  13,   6,  21,  23,  12, 17, -23,
	-27,  -2,  -5,  12,  17,   6, 10, -25,
	-26,  -4,  -4, -10,   3,   3, 33, -12,
	-35,  -1, -20, -23, -15,  24, 38, -22,
	  0,   0,   0,   0,   0,   0,  0,   0,
};

int eg_pawn_table[64] = {
	  0,   0,   0,   0,   0,   0,   0,   0,
	178, 173, 158, 134, 147, 132, 165, 187,
	 94, 100,  85,  67,  56,  53,  82,  84,
	 32,  24,  13,   5,  -2,   4,  17,  17,
	 13,   9,  -3,  -7,  -7,  -8,   3,  -1,
	  4,   7,  -6,   1,   0,  -5,  -1,  -8,
	 13,   8,   8,  10,  13,   0,   2,  -7,
	  0,   0,   0,   0,   0,   0,   0,   0,
};

int mg_knight_table[64] = {
	-167, -89, -34, -49,  61, -97, -15, -107,
	 -73, -41,  72,  36,  23,  62,   7,  -17,
	 -47,  60,  37,  65,  84, 129,  73,   44,
	  -9,  17,  19,  53,  37,  69,  18,   22,
	 -13,   4,  16,  13,  28,  19,  21,   -8,
	 -23,  -9,  12,  10,  19,  17,  25,  -16,
	 -29, -53, -12,  -3,  -1,  18, -14,  -19,
	-105, -21, -58, -33, -17, -28, -19,  -23,
};

int eg_knight_table[64] = {
	-58, -38, -13, -28, -31, -27, -63, -99,
	-25,  -8, -25,  -2,  -9, -25, -24, -52,
	-24, -20,  10,   9,  -1,  -9, -19, -41,
	-17,   3,  22,  22,  22,  11,   8, -18,
	-18,  -6,  16,  25,  16,  17,   4, -18,
	-23,  -3,  -1,  15,  10,  -3, -20, -22,
	-42, -20, -10,  -5,  -2, -20, -23, -44,
	-29, -51, -23, -15, -22, -18, -50, -64,
};

int mg_bishop_table[64] = {
	-29,   4, -82, -37, -25, -42,   7,  -8,
	-26,  16, -18, -13,  30,  59,  18, -47,
	-16,  37,  43,  40,  35,  50,  37,  -2,
	 -4,   5,  19,  50,  37,  37,   7,  -2,
	 -6,  13,  13,  26,  34,  12,  10,   4,
	  0,  15,  15,  15,  14,  27,  18,  10,
	  4,  15,  16,   0,   7,  21,  33,   1,
	-33,  -3, -14, -21, -13, -12, -39, -21,
};

int eg_bishop_table[64] = {
	-14, -21, -11,  -8, -7,  -9, -17, -24,
	 -8,  -4,   7, -12, -3, -13,  -4, -14,
	  2,  -8,   0,  -1, -2,   6,   0,   4,
	 -3,   9,  12,   9, 14,  10,   3,   2,
	 -6,   3,  13,  19,  7,  10,  -3,  -9,
	-12,  -3,   8,  10, 13,   3,  -7, -15,
	-14, -18,  -7,  -1,  4,  -9, -15, -27,
	-23,  -9, -23,  -5, -9, -16,  -5, -17,
};

int mg_rook_table[64] = {
	 32,  42,  32,  51, 63,  9,  31,  43,
	 27,  32,  58,  62, 80, 67,  26,  44,
	 -5,  19,  26,  36, 17, 45,  61,  16,
	-24, -11,   7,  26, 24, 35,  -8, -20,
	-36, -26, -12,  -1,  9, -7,   6, -23,
	-45, -25, -16, -17,  3,  0,  -5, -33,
	-44, -16, -20,  -9, -1, 11,  -6, -71,
	-19, -13,   1,  17, 16,  7, -37, -26,
};

int eg_rook_table[64] = {
	13, 10, 18, 15, 12,  12,   8,   5,
	11, 13, 13, 11, -3,   3,   8,   3,
	 7,  7,  7,  5,  4,  -3,  -5,  -3,
	 4,  3, 13,  1,  2,   1,  -1,   2,
	 3,  5,  8,  4, -5,  -6,  -8, -11,
	-4,  0, -5, -1, -7, -12,  -8, -16,
	-6, -6,  0,  2, -9,  -9, -11,  -3,
	-9,  2,  3, -1, -5, -13,   4, -20,
};

int mg_queen_table[64] = {
	-28,   0,  29,  12,  59,  44,  43,  45,
	-24, -39,  -5,   1, -16,  57,  28,  54,
	-13, -17,   7,   8,  29,  56,  47,  57,
	-27, -27, -16, -16,  -1,  17,  -2,   1,
	 -9, -26,  -9, -10,  -2,  -4,   3,  -3,
	-14,   2, -11,  -2,  -5,   2,  14,   5,
	-35,  -8,  11,   2,   8,  15,  -3,   1,
	 -1, -18,  -9,  10, -15, -25, -31, -50,
};

int eg_queen_table[64] = {
	 -9,  22,  22,  27,  27,  19,  10,  20,
	-17,  20,  32,  41,  58,  25,  30,   0,
	-20,   6,   9,  49,  47,  35,  19,   9,
	  3,  22,  24,  45,  57,  40,  57,  36,
	-18,  28,  19,  47,  31,  34,  39,  23,
	-16, -27,  15,   6,   9,  17,  10,   5,
	-22, -23, -30, -16, -16, -23, -36, -32,
	-33, -28, -22, -43,  -5, -32, -20, -41,
};

int mg_king_table[64] = {
	-65,  23,  16, -15, -56, -34,   2,  13,
	 29,  -1, -20,  -7,  -8,  -4, -38, -29,
	 -9,  24,   2, -16, -20,   6,  22, -22,
	-17, -20, -12, -27, -30, -25, -14, -36,
	-49,  -1, -27, -39, -46, -44, -33, -51,
	-14, -14, -22, -46, -44, -30, -15, -27,
	  1,   7,  -8, -64, -43, -16,   9,   8,
	-15,  36,  12, -54,   8, -28,  24,  14,
};

int eg_king_table[64] = {
	-74, -35, -18, -18, -11,  15,   4, -17,
	-12,  17,  14,  17,  17,  38,  23,  11,
	 10,  17,  23,  15,  20,  45,  44,  13,
	 -8,  22,  24,  27,  26,  33,  26,   3,
	-18,  -4,  21,  24,  27,  23,   9, -11,
	-19,  -3,  11,  21,  23,  16,   7,  -9,
	-27, -11,   4,  13,  14,   4,  -5, -17,
	-53, -34, -21, -11, -28, -14, -24, -43
};

int* mg_table[PT_NB] = {
	mg_pawn_table,
	mg_knight_table,
	mg_bishop_table,
	mg_rook_table,
	mg_queen_table,
	mg_king_table
};

int* eg_table[PT_NB] = {
	eg_pawn_table,
	eg_knight_table,
	eg_bishop_table,
	eg_rook_table,
	eg_queen_table,
	eg_king_table
};

int mg_pst[PIECE_NB][64];
int eg_pst[PIECE_NB][64];

void UciCommand(Position* pos, char* line);

static inline void TTClear() { memset(tt, 0, sizeof(tt)); }
static inline void HHClear() { memset(hh, 0, sizeof(hh)); }
static inline void SSClear() { memset(ss, 0, sizeof(ss)); }
static inline U64 GetTimeMs() { return GetTickCount64(); }
//least significant bit (LSB) is the rightmost bit in a binary number
static inline U64 LSB(const U64 bb) { return _tzcnt_u64(bb); }
static inline U64 Count(const U64 bb) { return _mm_popcnt_u64(bb); }
static inline U64 East(const U64 bb) { return (bb << 1) & ~FILE_A_BB; }
static inline U64 West(const U64 bb) { return (bb >> 1) & ~FILE_H_BB; }
static inline U64 North(const U64 bb) { return bb << 8; }
static inline U64 South(const U64 bb) { return bb >> 8; }
static inline U64 NW(const U64 bb) { return (bb << 7) & ~FILE_H_BB; }
static inline U64 NE(const U64 bb) { return (bb << 9) & ~FILE_A_BB; }
static inline U64 SW(const U64 bb) { return (bb >> 9) & ~FILE_H_BB; }
static inline U64 SE(const U64 bb) { return (bb >> 7) & ~FILE_A_BB; }
static inline int PieceColorOf(int piece) { return piece / PT_NB; }
static inline int PieceTypeOf(int piece) { return piece % PT_NB; }
static inline int FileOf(int sq) { return sq % 8; }
static inline int RankOf(int sq) { return sq / 8; }
static inline int Center(int rank, int file) { return -abs(rank * 2 - 7) / 2 - abs(file * 2 - 7) / 2; }
static inline int CenterSq(int sq) { return Center(RankOf(sq), FileOf(sq)); }
static inline int Equal(const Move lhs, const Move rhs) { return !memcmp(&rhs, &lhs, sizeof(Move)); }
static inline int MakePiece(int color, int pt) { return color * PT_NB + pt; }
static inline U64 BishopAttack(int square, U64 occupancy) {return bishop_table[square][_pext_u64(occupancy, bishop_masks[square])];}
static inline U64 RookAttack(int square, U64 occupancy) {return rook_table[square][_pext_u64(occupancy, rook_masks[square])];}

static int IsValid(int square, int delta) {
	int r = RankOf(square);
	int f = FileOf(square);
	int nr = RankOf(square + delta);
	int nf = FileOf(square + delta);
	if (nr < 0 || nr > 7 || nf < 0 || nf > 7) return 0;
	if (delta == 1 || delta == -1) return r == nr;
	if (delta == 9 || delta == -9 || delta == 7 || delta == -7) {
		return (r - nr == 1 || r - nr == -1) && (f - nf == 1 || f - nf == -1);
	}
	return 1;
}

static U64 GenerateSliderAttacks(int square, U64 occupancy, int *deltas) {
	U64 attacks = 0ULL;
	for (int i = 0; i < 4; i++) {
		int target = square;
		while (IsValid(target, deltas[i])) {
			target += deltas[i];
			attacks |= (1ULL << target);
			if (occupancy & (1ULL << target)) break;
		}
	}
	return attacks;
}

static void InitSlidersBmi2() {
	const int bishop_deltas[4] = { 9, -9, 7, -7 };
	const int rook_deltas[4] = { 8, -8, 1, -1 };
	for (int sq = 0; sq < 64; sq++) {
		U64 bishop_mask = 0ULL;
		for (int i = 0; i < 4; i++) {
			int target = sq;
			while (IsValid(target, bishop_deltas[i]) && IsValid(target + bishop_deltas[i], bishop_deltas[i])) {
				target += bishop_deltas[i];
				bishop_mask |= (1ULL << target);
			}
		}
		bishop_masks[sq] = bishop_mask;

		U64 rook_mask = 0ULL;
		for (int i = 0; i < 4; i++) {
			int target = sq;
			while (IsValid(target, rook_deltas[i]) && IsValid(target + rook_deltas[i], rook_deltas[i])) {
				target += rook_deltas[i];
				rook_mask |= (1ULL << target);
			}
		}
		rook_masks[sq] = rook_mask;

		int bishop_bits = Count(bishop_mask);
		int bishop_permutations = 1 << bishop_bits;
		for (int i = 0; i < bishop_permutations; i++) {
			U64 occupancy = _pdep_u64(i, bishop_mask);
			U64 attacks = GenerateSliderAttacks(sq, occupancy,bishop_deltas);
			U64 index = _pext_u64(occupancy, bishop_mask);
			bishop_table[sq][index] = attacks;
		}

		int rook_bits = Count(rook_mask);
		int rook_permutations = 1 << rook_bits;
		for (int i = 0; i < rook_permutations; i++) {
			U64 occupancy = _pdep_u64(i, rook_mask);
			U64 attacks = GenerateSliderAttacks(sq, occupancy,rook_deltas);

			U64 index = _pext_u64(occupancy, rook_mask);
			rook_table[sq][index] = attacks;
		}
	}
}

static U64 Rand64() {
	static U64 next = 1;
	next = next * 1103515245 + 12345;
	return next;
}

static void Swap(U64* a, U64* b) {
	U64 temp = *a;
	*a = *b;
	*b = temp;
}

//Return occupancy mask for a color
static inline U64 ColorOccupancy(const Position* pos, int color) {
	int start = color * PT_NB;
	U64 occ = 0;
	for (int pt = 0; pt < PT_NB; ++pt)
		occ |= pos->pieces[start + pt];
	return occ;
}

//Return occupancy mask for all pieces
static inline U64 AllOccupancy(const Position* pos) {
	return ColorOccupancy(pos, 0) | ColorOccupancy(pos, 1);
}

//Determine piece type on square (ignoring color). Returns PT_NB if empty
static int PieceTypeOnSquare(Position* pos, int sq) {
	for (int pt = PAWN; pt < PT_NB; ++pt)
		if ((pos->pieces[pt] | pos->pieces[PT_NB + pt]) & bbSquare[sq])
			return pt;
	return PT_NB;
}

static int PieceOnSquare(Position* pos, int sq) {
	for (int piece = 0; piece < PIECE_NB; ++piece)
		if (pos->pieces[piece] & bbSquare[sq])
			return piece;
	return PIECE_NB;
}

//Raycast helper
static U64 Ray(const U64 bb, const U64 blockers, U64(*f)(U64)) {
	U64 mask = f(bb);
	mask |= f(mask & ~blockers);
	mask |= f(mask & ~blockers);
	mask |= f(mask & ~blockers);
	mask |= f(mask & ~blockers);
	mask |= f(mask & ~blockers);
	mask |= f(mask & ~blockers);
	mask |= f(mask & ~blockers);
	return mask;
}

static inline U64 KnightAttackBB(const U64 bb) {
	return (((bb << 15) | (bb >> 17)) & 0x7F7F7F7F7F7F7F7FULL) | (((bb << 17) | (bb >> 15)) & 0xFEFEFEFEFEFEFEFEULL) |
		(((bb << 10) | (bb >> 6)) & 0xFCFCFCFCFCFCFCFCULL) | (((bb << 6) | (bb >> 10)) & 0x3F3F3F3F3F3F3F3FULL);
}

static inline U64 KnightAttack(const int sq) {
	return bbKnightAttack[sq];
}

static inline U64 BishopAttackBB(U64 bb, U64 blockers) {
	return Ray(bb, blockers, NW) | Ray(bb, blockers, NE) | Ray(bb, blockers, SW) | Ray(bb, blockers, SE);
}

static inline U64 RookAttackBB(U64 bb, U64 blockers) {
	return Ray(bb, blockers, North) | Ray(bb, blockers, East) | Ray(bb, blockers, South) | Ray(bb, blockers, West);
}

static inline U64 KingAttackBB(const U64 bb) {
	return (bb << 8) | (bb >> 8) | (((bb >> 1) | (bb >> 9) | (bb << 7)) & 0x7F7F7F7F7F7F7F7FULL) |
		(((bb << 1) | (bb << 9) | (bb >> 7)) & 0xFEFEFEFEFEFEFEFEULL);
}

static inline U64 KingAttack(const int sq) {
	return bbKingAttack[sq];
}

static void PrintBoard(Position* pos) {
	const char* s = "   +---+---+---+---+---+---+---+---+\n";
	const char* t = "     A   B   C   D   E   F   G   H\n";
	printf(t);
	for (int r = 7; r >= 0; r--) {
		printf(s);
		printf(" %d |", r + 1);
		for (int f = 0; f < 8; f++) {
			int sq = r * 8 + f;
			int piece = PieceOnSquare(pos, sq);
			printf(" %c |", "ANBRQKanbrqk "[piece]);
		}
		printf(" %d \n", r + 1);
	}
	printf(s);
	printf(t);
	char castling[5] = "KQkq";
	for (int n = 0; n < 4; n++)
		if (!(pos->castleFlags & (1 << n)))
			castling[n] = '-';
	printf("side     : %16s\n", pos->color ? "black" : "white");
	printf("castling : %16s\n", castling);
	printf("score    : %16d\n", EvalPosition(pos));
}

static int CheckUp() {
	if ((++info.nodes & 0xffff) == 0) {
		if (info.timeLimit && GetTimeMs() - info.timeStart > info.timeLimit)
			info.stop = TRUE;
		if (info.nodesLimit && info.nodes > info.nodesLimit)
			info.stop = TRUE;
	}
	return info.stop;
}

static U64 Attacked(Position* pos, int sq, int byColor) {
	const U64 bb = bbSquare[sq];
	const U64 N = pos->pieces[MakePiece(byColor, KNIGHT)];
	const U64 BQ = pos->pieces[MakePiece(byColor, BISHOP)] | pos->pieces[MakePiece(byColor, QUEEN)];
	const U64 RQ = pos->pieces[MakePiece(byColor, ROOK)] | pos->pieces[MakePiece(byColor, QUEEN)];
	const U64 pawns = pos->pieces[MakePiece(byColor, PAWN)];
	const U64 pawn_attacks = byColor ? SW(pawns) | SE(pawns) : NW(pawns) | NE(pawns);
	U64 blockers = AllOccupancy(pos);
	return (pawn_attacks & bb) | (N & KnightAttack(sq)) |
		(BishopAttack(sq, blockers) & BQ) |
		(RookAttack(sq, blockers) & RQ) |
		(KingAttack(sq) & pos->pieces[MakePiece(byColor, KING)]);
}

static void AddMove(Move* const moveList, int* num_moves, const int from, const int to, const int promo) {
	Move* m = &moveList[(*num_moves)++];
	m->from = from;
	m->to = to;
	m->promo = promo;
}

/* Pawn move generation for the side-to-move */
static void GeneratePawnMoves(Move* const moveList, int* num_moves, U64 to_mask, const int offset) {
	while (to_mask) {
		const U64 to = LSB(to_mask);
		to_mask &= to_mask - 1;
		if (to >= 56 || to <= 7) {
			AddMove(moveList, num_moves, to + offset, to, KNIGHT);
			AddMove(moveList, num_moves, to + offset, to, BISHOP);
			AddMove(moveList, num_moves, to + offset, to, ROOK);
			AddMove(moveList, num_moves, to + offset, to, QUEEN);
		}
		else
			AddMove(moveList, num_moves, to + offset, to, PT_NB);
	}
}

/* Generate moves for piece type for side-to-move */
static void GeneratePieceMoves(Move* const moveList, int* num_moves, const Position* pos, const U64 blockers, const int piece, const U64 to_mask, U64(*func)(int, U64)) {
	U64 copy = pos->pieces[piece]; // piece bitboard for side-to-move
	while (copy) {
		const int fr = (int)LSB(copy);
		copy &= copy - 1;
		U64 moves = func(fr, blockers) & to_mask;
		while (moves) {
			const int to = (int)LSB(moves);
			moves &= moves - 1;
			AddMove(moveList, num_moves, fr, to, PT_NB);
		}
	}
}

static int GenerateMovesWhite(const Position* pos, Move* const moveList, int only_captures) {
	int num_moves = 0;
	const U64 bbUs = ColorOccupancy(pos, WHITE);
	const U64 bbThem = ColorOccupancy(pos, BLACK);
	const U64 all = bbUs | bbThem;
	const U64 to_mask = only_captures ? bbThem : ~bbUs;
	const U64 pawnsUs = pos->pieces[WPAWN];
	U64 maskToPawn = North(pawnsUs) & ~all & (only_captures ? 0xFF00000000000000ULL : 0xFFFFFFFFFFFF0000ULL);
	GeneratePawnMoves(moveList, &num_moves, maskToPawn, -8);
	if (!only_captures)
		GeneratePawnMoves(moveList, &num_moves, North(North(pawnsUs & 0xFF00ULL) & ~all) & ~all, -16);
	GeneratePawnMoves(moveList, &num_moves, NW(pawnsUs) & (bbThem | pos->ep), -7);
	GeneratePawnMoves(moveList, &num_moves, NE(pawnsUs) & (bbThem | pos->ep), -9);
	GeneratePieceMoves(moveList, &num_moves, pos, all, WKNIGHT, to_mask, KnightAttack);
	GeneratePieceMoves(moveList, &num_moves, pos, all, WBISHOP, to_mask, BishopAttack);
	GeneratePieceMoves(moveList, &num_moves, pos, all, WQUEEN, to_mask, BishopAttack);
	GeneratePieceMoves(moveList, &num_moves, pos, all, WROOK, to_mask, RookAttack);
	GeneratePieceMoves(moveList, &num_moves, pos, all, WQUEEN, to_mask, RookAttack);
	GeneratePieceMoves(moveList, &num_moves, pos, all, WKING, to_mask, KingAttack);
	if (!only_captures && (pos->castleFlags & CASTLE_WK) && !(all & 0x60ULL) && !Attacked(pos, SQ_E1, BLACK) && !Attacked(pos, SQ_F1, BLACK))
		AddMove(moveList, &num_moves, SQ_E1, SQ_G1, PT_NB);
	if (!only_captures && (pos->castleFlags & CASTLE_WQ) && !(all & 0xEULL) && !Attacked(pos, SQ_E1, BLACK) && !Attacked(pos, SQ_D1, BLACK))
		AddMove(moveList, &num_moves, SQ_E1, SQ_C1, PT_NB);
	return num_moves;
}

static int GenerateMovesBlack(const Position* pos, Move* const moveList, int only_captures) {
	int num_moves = 0;
	const U64 bbUs = ColorOccupancy(pos, BLACK);
	const U64 bbThem = ColorOccupancy(pos, WHITE);
	const U64 all = bbUs | bbThem;
	const U64 to_mask = only_captures ? bbThem : ~bbUs;
	const U64 pawnsUs = pos->pieces[BPAWN];
	U64 maskToPawn = South(pawnsUs) & ~all & (only_captures ? 0x00000000000000FFULL : 0x0000FFFFFFFFFFFFULL);
	GeneratePawnMoves(moveList, &num_moves, maskToPawn, 8);
	if (!only_captures)
		GeneratePawnMoves(moveList, &num_moves, South(South(pawnsUs & 0x00FF000000000000ULL) & ~all) & ~all, 16);
	GeneratePawnMoves(moveList, &num_moves, SW(pawnsUs) & (bbThem | pos->ep), 9);
	GeneratePawnMoves(moveList, &num_moves, SE(pawnsUs) & (bbThem | pos->ep), 7);
	GeneratePieceMoves(moveList, &num_moves, pos, all, BKNIGHT, to_mask, KnightAttack);
	GeneratePieceMoves(moveList, &num_moves, pos, all, BBISHOP, to_mask, BishopAttack);
	GeneratePieceMoves(moveList, &num_moves, pos, all, BQUEEN, to_mask, BishopAttack);
	GeneratePieceMoves(moveList, &num_moves, pos, all, BROOK, to_mask, RookAttack);
	GeneratePieceMoves(moveList, &num_moves, pos, all, BQUEEN, to_mask, RookAttack);
	GeneratePieceMoves(moveList, &num_moves, pos, all, BKING, to_mask, KingAttack);
	if (!only_captures && (pos->castleFlags & CASTLE_BK) && !(all & 0x6000000000000000ULL) && !Attacked(pos, SQ_E8, WHITE) && !Attacked(pos, SQ_F8, WHITE))
		AddMove(moveList, &num_moves, SQ_E8, SQ_G8, PT_NB);
	if (!only_captures && (pos->castleFlags & CASTLE_BQ) && !(all & 0x0E00000000000000ULL) && !Attacked(pos, SQ_E8, WHITE) && !Attacked(pos, SQ_D8, WHITE))
		AddMove(moveList, &num_moves, SQ_E8, SQ_C8, PT_NB);
	return num_moves;
}

static int GenerateMoves(const Position* pos, Move* const moveList, int only_captures) {
	return pos->color ? GenerateMovesBlack(pos, moveList, only_captures) : GenerateMovesWhite(pos, moveList, only_captures);
}

static int IsRepetition(Position* pos) {
	int limit = max(0, pos->historyCount - pos->move50);
	for (int n = pos->historyCount - 4; n >= limit; n -= 2)
		if (historyHash[n] == pos->hash)
			return TRUE;
	return FALSE;
}

static void PrintBitboard(U64 bb) {
	const char* s = "   +---+---+---+---+---+---+---+---+\n";
	const char* t = "     A   B   C   D   E   F   G   H\n";
	printf(t);
	for (int r = 7; r >= 0; r--) {
		printf(s);
		printf(" %d |", r + 1);
		for (int f = 0; f < 8; f++) {
			int sq = r * 8 + f;
			printf(" %c |", bb & 1ull << sq ? 'x' : ' ');
		}
		printf(" %d \n", r + 1);
	}
	printf(s);
	printf(t);
}

static void SetFen(Position* pos, char* fen) {
	memset(pos, 0, sizeof(Position));
	int sq = 56;
	while (*fen && *fen != ' ') {
		U64 bb = 1ull << sq;
		switch (*fen) {
		case '1': sq += 1; break;
		case '2': sq += 2; break;
		case '3': sq += 3; break;
		case '4': sq += 4; break;
		case '5': sq += 5; break;
		case '6': sq += 6; break;
		case '7': sq += 7; break;
		case '8': sq += 8; break;
		case 'P': pos->pieces[WPAWN] |= bb; ++sq; break;
		case 'N': pos->pieces[WKNIGHT] |= bb; ++sq; break;
		case 'B': pos->pieces[WBISHOP] |= bb; ++sq; break;
		case 'R': pos->pieces[WROOK] |= bb; ++sq; break;
		case 'Q': pos->pieces[WQUEEN] |= bb; ++sq; break;
		case 'K': pos->pieces[WKING] |= bb; ++sq; break;
		case 'p': pos->pieces[BPAWN] |= bb; ++sq; break;
		case 'n': pos->pieces[BKNIGHT] |= bb; ++sq; break;
		case 'b': pos->pieces[BBISHOP] |= bb; ++sq; break;
		case 'r': pos->pieces[BROOK] |= bb; ++sq; break;
		case 'q': pos->pieces[BQUEEN] |= bb; ++sq; break;
		case 'k': pos->pieces[BKING] |= bb; ++sq; break;
		case '/': sq -= 16; break;
		}
		fen++;
	}
	fen++;
	pos->color = *fen == 'b';
	while (*fen && *fen != ' ')
		fen++;
	fen++;
	while (*fen && *fen != ' ') {
		switch (*fen) {
		case 'K': pos->castleFlags |= CASTLE_WK; break;
		case 'Q': pos->castleFlags |= CASTLE_WQ; break;
		case 'k': pos->castleFlags |= CASTLE_BK; break;
		case 'q': pos->castleFlags |= CASTLE_BQ; break;
		}
		fen++;
	}
	fen++;
	if (*fen != '-') {
		const int sq = (fen[0] - 'a') + 8 * (fen[1] - '1');
		pos->ep = bbSquare[sq];
	}
	while (*fen && *fen != ' ') fen++; fen++;
	pos->move50 = atoi(fen);
	pos->hash = Rand64();
}

static char* ParseToken(char* string, char* token) {
	while (*string == ' ')
		string++;
	while (*string != ' ' && *string != '\0' && *string != '\n')
		*token++ = *string++;
	*token = '\0';
	return string;
}

static char* MoveToUci(Move move) {
	static char str[6] = { 0 };
	str[0] = 'a' + FileOf(move.from);
	str[1] = '1' + RankOf(move.from);
	str[2] = 'a' + FileOf(move.to);
	str[3] = '1' + RankOf(move.to);
	str[4] = "\0nbrq\0\0"[move.promo];
	return str;
}

static Move UciToMove(char* s) {
	Move m;
	m.from = (s[0] - 'a');
	int f = (s[1] - '1');
	m.from += 8 * f;
	m.to = (s[2] - 'a');
	f = (s[3] - '1');
	m.to += 8 * f;
	m.promo = PT_NB;
	switch (s[4]) {
	case 'N':
	case 'n':
		m.promo = KNIGHT;
		break;
	case 'B':
	case 'b':
		m.promo = BISHOP;
		break;
	case 'R':
	case 'r':
		m.promo = ROOK;
		break;
	case 'Q':
	case 'q':
		m.promo = QUEEN;
		break;
	}
	return m;
}

/* Make move for side-to-move representation (pieces[0..5] = our pieces) */
/* Returns 1 if move is legal (not leaving king in check), otherwise 0 */
static int MakeMove(Position* pos, const Move* move) {
	historyHash[pos->historyCount++] = pos->hash;
	const int pt = PieceTypeOnSquare(pos, move->from);
	const int piece = MakePiece(pos->color, pt);
	const int ptCaptured = PieceTypeOnSquare(pos, move->to);
	const U64 bbFrom = bbSquare[move->from];
	const U64 bbTo = bbSquare[move->to];
	pos->move50++;
	if (ptCaptured != PT_NB || pt == PAWN)
		pos->move50 = 0;
	// Move our piece bitboard (side-to-move is in first half)
	pos->pieces[piece] ^= bbFrom | bbTo;
	pos->hash ^= zob_piece[piece][move->from] ^ zob_piece[piece][move->to];
	// Handle en-passant capture
	if (pt == PAWN && bbTo == pos->ep)
		if (pos->color) {
			U64 epTo = bbTo << 8;
			pos->pieces[WPAWN] ^= epTo;
			pos->hash ^= zob_piece[WPAWN][LSB(epTo)];
		}
		else {
			U64 epTo = bbTo >> 8;
			pos->pieces[BPAWN] ^= epTo;
			pos->hash ^= zob_piece[BPAWN][LSB(epTo)];
		}
	if (pos->ep) {
		pos->hash ^= pos->ep;
		pos->ep = 0x0ULL;
	}
	// Set ep if double pawn push
	if (pt == PAWN && abs(move->to - move->from) == 16) {
		pos->ep = pos->color ? bbTo << 8 : bbTo >> 8;
		pos->hash ^= pos->ep;
	}
	// Capture
	if (ptCaptured != PT_NB) {
		int pieceCaptured = MakePiece(!pos->color, ptCaptured);
		pos->pieces[pieceCaptured] ^= bbTo;
		pos->hash ^= zob_piece[pieceCaptured][move->to];
	}
	// King move may imply rook move for castling (operate on our rook bitboard)
	if (pt == KING) {
		int del = move->to - move->from;
		if (del == 2) {
			int pieceRook = MakePiece(pos->color, ROOK);
			int fsq = move->from + 3;
			int tsq = move->to - 1;
			const U64 bb = pos->color ? 0xA000000000000000ULL : 0xa0ULL;
			pos->pieces[pieceRook] ^= bb;
			pos->hash ^= zob_piece[pieceRook][fsq] ^ zob_piece[pieceRook][tsq];
		}
		else if (del == -2) {
			int pieceRook = MakePiece(pos->color, ROOK);
			int fsq = move->from - 4;
			int tsq = move->to + 1;
			const U64 bb = pos->color ? 0x0900000000000000ULL : 0x9ULL;
			pos->pieces[MakePiece(pos->color, ROOK)] ^= bb;
			pos->hash ^= zob_piece[pieceRook][fsq] ^ zob_piece[pieceRook][tsq];
		}
	}
	// Promotion
	if (pt == PAWN && (move->to > 55 || move->to < 8)) {
		int pp = MakePiece(pos->color, move->promo);
		pos->pieces[piece] ^= bbTo;
		pos->pieces[pp] ^= bbTo;
		pos->hash ^= zob_piece[piece][move->to] ^ zob_piece[pp][move->to];
	}
	pos->hash ^= zob_castle[pos->castleFlags];
	pos->castleFlags &= boardCastle[move->from] & boardCastle[move->to];
	pos->hash ^= zob_castle[pos->castleFlags];
	pos->color = !pos->color;
	pos->hash ^= zob_side;
	return !Attacked(pos, (int)LSB(pos->pieces[MakePiece(!pos->color, KING)]), pos->color);
}

static U64 Attacks(int pt, int sq, U64 blockers) {
	switch (pt) {
	case ROOK:
		return RookAttack(sq, blockers);
	case BISHOP:
		return BishopAttack(sq, blockers);
	case QUEEN:
		return RookAttack(sq, blockers) | BishopAttack(sq, blockers);
	case KNIGHT:
		return KnightAttack(sq);
	case KING:
		return KingAttack(sq);
	default:
		return 0;
	}
}

static int EvalPosition(Position* pos) {
	int phase = 0;
	int score = 0;
	int scoreMg = 0;
	int scoreEg = 0;
	int insufficient[2] = { 0 };
	U64 bbBlockers = AllOccupancy(pos);
	for (int c = WHITE; c < COLOR_NB; ++c) {
		U64 occUs = ColorOccupancy(pos, c);
		U64 bbStart1 = pos->pieces[MakePiece(!c, PAWN)];
		U64 bbControl1 = c ? NW(bbStart1) | NE(bbStart1) : SW(bbStart1) | SE(bbStart1);
		for (int pt = PAWN; pt < PT_NB; ++pt) {
			int piece = MakePiece(c, pt);
			U64 copy = pos->pieces[piece];
			while (copy) {
				const int fr = (int)LSB(copy);
				copy &= copy - 1;
				scoreMg += mg_pst[piece][fr];
				scoreEg += eg_pst[piece][fr];
				phase += phaseVal[pt];
				insufficient[c] += insufVal[pt];
				if (pt > PAWN && pt < KING) {
					U64 bbAttack = Attacks(pt, fr, bbBlockers);
					score += Count(bbAttack & ~bbControl1 & ~occUs);
				}
			}
		}
		U64 bbStart0 = pos->pieces[MakePiece(c, KING)];
		U64 file0 = bbFiles[FileOf(LSB(bbStart0))];
		file0 |= East(file0) | West(file0);
		U64 bbAttack0 = file0 & ~(FILE_D_BB | FILE_E_BB);
		bbAttack0 &= pos->pieces[MakePiece(c, PAWN)];
		scoreMg += Count(bbAttack0 & (c ? RANK_7_BB : RANK_2_BB)) * 16;
		scoreMg += Count(bbAttack0 & (c ? RANK_6_BB : RANK_3_BB)) * 8;
		score = -score;
		scoreMg = -scoreMg;
		scoreEg = -scoreEg;
	}
	if (phase > 24) phase = 24;
	score += (scoreMg * phase + scoreEg * (24 - phase)) / 24;
	if (max(insufficient[0], insufficient[1]) < 5)return 0;
	if (insufficient[score < 0] < 4)return 0;
	return pos->color ? -score : score;
}

static int EvalHash(Position* pos, U64 hash) {
	TTEntryEval* ee = &ttEval[hash & ttMask];
	if (ee->hash != hash) {
		ee->hash = hash;
		ee->score = EvalPosition(pos);
	}
	return (ee->score * (100 - pos->move50)) / 100;
}

static int IsPseudolegalMove(const Position* pos, const Move move) {
	Move moves[256];
	const int num_moves = GenerateMoves((Position*)pos, moves, 0);
	for (int i = 0; i < num_moves; ++i)
		if (moves[i].from == move.from && moves[i].to == move.to)
			return 1;
	return 0;
}

static void PrintPv(const Position* pos, const Move move) {
	if (!IsPseudolegalMove(pos, move))
		return;
	const Position npos = *pos;
	if (!MakeMove((Position*)&npos, &move))
		return;
	printf(" %s", MoveToUci(move));
	TTEntry* ttEntry = tt + (npos.hash & ttMask);
	if (ttEntry->hash != npos.hash || IsRepetition((Position*)&npos))
		return;
	PrintPv(&npos, ttEntry->move);
}

static int Permill() {
	int pm = 0;
	for (int n = 0; n < 1000; n++)
		if (tt[n].hash)
			pm++;
	return pm;
}

static void PrintInfo(Position* pos, int depth, int score) {
	printf("info depth %d score ", depth);
	if (abs(score) < MATE - MAX_PLY)
		printf("cp %d", score);
	else
		printf("mate %d", (score > 0 ? (MATE - score + 1) >> 1 : -(MATE + score) >> 1));
	printf(" time %lld", GetTimeMs() - info.timeStart);
	printf(" nodes %lld", info.nodes);
	printf(" hashfull %d pv", Permill());
	PrintPv(pos, ss[0].move);
	printf("\n");
}

static S16 SearchAlpha(Position* pos, int alpha, int beta, int depth, int ply, Stack* ss, int doNull) {
	if (CheckUp())
		return 0;
	int  mate_value = MATE - ply;
	if (alpha < -mate_value)
		alpha = -mate_value;
	if (beta > mate_value - 1)
		beta = mate_value - 1;
	if (alpha >= beta)
		return alpha;
	const int staticEval = EvalHash(pos, pos->hash);
	if (ply >= MAX_PLY)
		return staticEval;
	const U64 inCheck = Attacked(pos, (int)LSB(pos->pieces[MakePiece(pos->color, KING)]), !pos->color);
	if (inCheck)
		depth = max(1, depth + 1);
	int inQuiescence = depth < 1;
	if (ply && !inQuiescence)
		if (pos->move50 >= 100 || IsRepetition(pos))
			return 0;
	TTEntry* ttEntry = tt + (pos->hash & ttMask);
	Move ttMove = { 0 };
	int inPv = beta - alpha > 1;
	if (ttEntry->hash == pos->hash) {
		ttMove = ttEntry->move;
		if (!inPv && ttEntry->depth >= depth) {
			if (ttEntry->flag == EXACT)return ttEntry->score;
			if (ttEntry->flag == LOWER && ttEntry->score <= alpha)return ttEntry->score;
			if (ttEntry->flag == UPPER && ttEntry->score >= beta)return ttEntry->score;
		}
	}
	else
		depth -= depth > 3;
	if (inQuiescence && alpha < staticEval) {
		alpha = staticEval;
		if (alpha >= beta)
			return beta;
	}
	if (!inQuiescence && !inPv && !inCheck && ply && beta < MATE - MAX_PLY) {
		// REVERSE FUTILITY PRUNING
		if (depth < 8 && staticEval - 70 * depth >= beta)
			return staticEval - 70 * depth;
		// RAZORING: if eval is far below alpha even after the best capture,there is no point searching -- drop straight into qsearch.
		if (depth <= 3 && staticEval + 300 + 60 * depth < alpha)
			return SearchAlpha(pos, alpha, beta, 0, ply, ss, doNull);
		// NULL MOVE PRUNING
		if (depth > 2 && staticEval >= beta && doNull && (pos->pieces[KNIGHT] || pos->pieces[BISHOP] || pos->pieces[ROOK] || pos->pieces[QUEEN])) {
			Position npos = *pos;
			npos.color = !npos.color;
			npos.hash ^= zob_side;
			npos.ep = 0x0ULL;
			int R = depth >= 7 ? 4 : 3;
			int score = -SearchAlpha(&npos, -beta, -beta + 1, depth - R - 1, ply + 1, ss, 0);
			if (score >= beta)
				return score;
		}
	}
	U8 tt_flag = LOWER;
	Move movesList[256];
	int quietMoves = 0;
	Move qList[256];
	const int movesCount = GenerateMoves(pos, movesList, inQuiescence);
	S64 scoreList[256];
	for (int j = 0; j < movesCount; ++j) {
		Move m = movesList[j];
		const int ptSou = PieceTypeOnSquare((Position*)pos, m.from);
		int ptDes = m.promo == PT_NB ? PieceTypeOnSquare((Position*)pos, m.to) : m.promo;
		if (Equal(m, ttMove))
			scoreList[j] = 1LL << 62;
		else if (ptDes != PT_NB)
			scoreList[j] = ((ptDes + 1) * (1LL << 54)) - ptSou;
		else if (Equal(m, ss[ply].killer1))
			scoreList[j] = 1LL << 50;
		else if (Equal(m, ss[ply].killer2))
			scoreList[j] = 1LL << 48;
		else
			scoreList[j] = hh[pos->color][m.from][m.to];
	}
	S16 score;
	int legalMoves = 0;
	for (int i = 0; i < movesCount; ++i) {
		int bstIdx = i;
		for (int j = i + 1; j < movesCount; ++j)
			if (scoreList[bstIdx] < scoreList[j])
				bstIdx = j;
		Move move = movesList[bstIdx];
		scoreList[bstIdx] = scoreList[i];
		movesList[bstIdx] = movesList[i];
		//printf("%s\n", MoveToUci(move));
		// Delta pruning
		if (inQuiescence && !inCheck &&
			staticEval + 50 + mx_material[PieceTypeOnSquare(pos, move.to)] < alpha)
			break;

		// Forward futility pruning
		/*if (!inQuiescence && !inCheck && (move.from != ttMove.from || move.to != ttMove.to) &&
			staticEval + 150 * depth + mx_material[PieceTypeOnSquare(pos, move.to)] < alpha)
			break;*/

		Position npos = *pos;
		if (!MakeMove(&npos, &move))
			continue;
		if (!legalMoves || depth < 4)
			score = -SearchAlpha(&npos, -beta, -alpha, depth - 1, ply + 1, ss, 1);
		else {
			int r = !inPv;
			score = -SearchAlpha(&npos, -alpha - 1, -alpha, depth - 1 - r, ply + 1, ss, 1);
			if (r && score > alpha)
				score = -SearchAlpha(&npos, -alpha - 1, -alpha, depth - 1, ply + 1, ss, 1);
			if (score > alpha && score < beta)
				score = -SearchAlpha(&npos, -beta, -alpha, depth - 1, ply + 1, ss, 1);
		}
		//if (!ply)printf("%s %d\n",MoveToUci(move),score);
		if (info.stop)
			break;
		legalMoves++;
		int isQuiet = move.promo == PT_NB && PieceTypeOnSquare((Position*)pos, move.to) == PT_NB && (PieceTypeOnSquare((Position*)pos, move.from) != PAWN || bbSquare[move.to] != pos->ep);
		if (isQuiet)
			qList[quietMoves++] = move;
		if (alpha < score) {
			alpha = score;
			tt_flag = EXACT;
			ss[ply].move = move;
			if (!ply && info.post)
				PrintInfo(pos, depth, score);
			if (alpha >= beta) {
				tt_flag = UPPER;
				if (isQuiet) {
					ss[ply].killer2 = ss[ply].killer1;
					ss[ply].killer1 = move;
				}
				int bonus = depth * depth;
				int h = hh[pos->color][move.from][move.to];
				h += bonus - h / 1024;
				hh[pos->color][move.from][move.to] = h;
				for (int i = 0; i < quietMoves; i++) {
					Move* m = &qList[i];
					int hm = hh[pos->color][m->from][m->to];
					hm -= bonus - hm / 1024;
					hh[pos->color][m->from][m->to] = hm;
				}
				break;
			}
		}
	}
	if (info.stop)
		return 0;
	if (!legalMoves)
		return inQuiescence ? alpha : inCheck ? ply - MATE : 0;
	ttEntry->hash = pos->hash;
	ttEntry->move = ss[ply].move;
	ttEntry->depth = max(0, depth);
	ttEntry->score = alpha;
	ttEntry->flag = tt_flag;
	return alpha;
}

static void SearchIteratively(Position* pos) {
	TTClear();
	SSClear();
	int score = 0;
	int alpha = -MATE;
	int beta = MATE;
	for (int depth = 1; depth <= info.depthLimit; ++depth) {
		int aspH = 16, aspL = 16;
		do {
			if (depth > 4) {
				alpha = score - aspL;
				beta = score + aspH;
			}
			score = SearchAlpha(pos, alpha, beta, depth, 0, ss, 1);
			if (score <= alpha) {
				alpha -= aspL;
				aspL *= 2;
			}
			else if (score >= beta) {
				beta += aspH;
				aspH *= 2;
			}
			else
				break;
		} while (!info.stop);
		if (info.stop)
			break;
		if (info.timeLimit && GetTimeMs() - info.timeStart > info.timeLimit / 2)
			break;
	}
	if (info.post) {
		char* uci = MoveToUci(ss[0].move);
		printf("bestmove %s\n", uci);
		fflush(stdout);
	}
}

static void ResetInfo() {
	info.timeStart = GetTimeMs();
	info.timeLimit = 0;
	info.depthLimit = MAX_PLY;
	info.nodesLimit = 0;
	info.nodes = 0;
	info.stop = FALSE;
	info.post = TRUE;
}

static inline void PerftDriver(Position* pos, int depth) {
	Move moves[256];
	const int num_moves = GenerateMoves(pos, moves, 0);
	for (int n = 0; n < num_moves; n++) {
		Position npos = *pos;
		if (!MakeMove(&npos, &moves[n])) {
			//PrintBoard(&npos);
			//printf("%s\n",MoveToUci(moves[n]));
			continue;
		}
		if (depth)
			PerftDriver(&npos, depth - 1);
		else
			info.nodes++;
	}
}

static int ShrinkNumber(U64 n) {
	if (n < 10000)
		return 0;
	if (n < 10000000)
		return 1;
	if (n < 10000000000)
		return 2;
	return 3;
}

static void PrintSummary(U64 time, U64 nodes) {
	U64 nps = (nodes * 1000) / max(time, 1);
	const char* units[] = { "", "k", "m", "g" };
	int sn = ShrinkNumber(nps);
	int p = pow(10, sn * 3);
	int b = pow(10, 3);
	printf("-----------------------------\n");
	printf("Time        : %llu\n", time);
	printf("Nodes       : %llu\n", nodes);
	printf("Nps         : %llu (%llu%s/s)\n", nps, nps / p, units[sn]);
	printf("-----------------------------\n");
}

static void PrintPerformanceHeader() {
	printf("-----------------------------\n");
	printf("ply      time        nodes\n");
	printf("-----------------------------\n");
}

//performance test
static void UciPerformance(Position* pos) {
	ResetInfo();
	PrintPerformanceHeader();
	info.depthLimit = 0;
	U64 elapsed = 0;
	while (elapsed < 4000) {
		PerftDriver(pos, info.depthLimit++);
		elapsed = GetTimeMs() - info.timeStart;
		printf(" %2d. %8llu %12llu\n", info.depthLimit, elapsed, info.nodes);
	}
	PrintSummary(elapsed, info.nodes);
}

//start benchmark
static void UciBench(Position* pos) {
	ResetInfo();
	PrintPerformanceHeader();
	info.depthLimit = 0;
	info.post = FALSE;
	U64 elapsed = 0;
	while (elapsed < 8000) {
		++info.depthLimit;
		SearchIteratively(pos);
		elapsed = GetTimeMs() - info.timeStart;
		printf(" %2d. %8llu %12llu\n", info.depthLimit, elapsed, info.nodes);
	}
	PrintSummary(elapsed, info.nodes);
	//printf("eok: %f\n",(eok*100.0)/info.nodes);
}

static void ParsePosition(Position* pos, char* ptr) {
	char token[80], fen[80];
	ptr = ParseToken(ptr, token);
	if (strcmp(token, "fen") == 0) {
		fen[0] = '\0';
		while (1) {
			ptr = ParseToken(ptr, token);
			if (*token == '\0' || strcmp(token, "moves") == 0)
				break;
			strcat(fen, token);
			strcat(fen, " ");
		}
		SetFen(pos, fen);
	}
	else {
		ptr = ParseToken(ptr, token);
		SetFen(pos, START_FEN);
	}
	pos->historyCount = 0;
	if (strcmp(token, "moves") == 0) {
		while (1) {
			ptr = ParseToken(ptr, token);
			if (*token == '\0')
				break;
			Move m = UciToMove(token);
			MakeMove(pos, &m);
			if (!pos->move50)
				pos->historyCount = 0;
		}
	}
}

static void ParseGo(Position* pos, char* command) {
	ResetInfo();
	int wtime = 0;
	int btime = 0;
	int winc = 0;
	int binc = 0;
	int movestogo = 32;
	char* argument = NULL;
	if (argument = strstr(command, "binc"))
		binc = atoi(argument + 5);
	if (argument = strstr(command, "winc"))
		winc = atoi(argument + 5);
	if (argument = strstr(command, "wtime"))
		wtime = max(1, atoi(argument + 6));
	if (argument = strstr(command, "btime"))
		btime = max(1, atoi(argument + 6));
	if ((argument = strstr(command, "movestogo")))
		movestogo = atoi(argument + 10);
	if ((argument = strstr(command, "movetime")))
		info.timeLimit = atoi(argument + 9);
	if ((argument = strstr(command, "depth")))
		info.depthLimit = atoi(argument + 6);
	if (argument = strstr(command, "nodes"))
		info.nodesLimit = atoi(argument + 5);
	int time = pos->color ? btime : wtime;
	int inc = pos->color ? binc : winc;
	if (time)
		info.timeLimit = max(1, min(time / movestogo + inc, time / 2));
	SearchIteratively(pos);
}

void UciCommand(Position* pos, char* line) {
	if (!strncmp(line, "ucinewgame", 10))HHClear();
	else if (!strncmp(line, "uci", 3)) {
		printf("id name %s\nuciok\n", NAME);
		fflush(stdout);
	}
	else if (!strncmp(line, "isready", 7)) {
		printf("readyok\n");
		fflush(stdout);
	}
	else if (!strncmp(line, "go", 2))ParseGo(pos, line + 2);
	else if (!strncmp(line, "position", 8))ParsePosition(pos, line + 8);
	else if (!strncmp(line, "print", 5))PrintBoard(pos);
	else if (!strncmp(line, "perft", 5))UciPerformance(pos);
	else if (!strncmp(line, "bench", 5))UciBench(pos);
}

static void UciLoop(Position* pos) {
	//PrintBitboard(0x60ULL);
	//PrintBitboard(0xEULL);
	//PrintBitboard(0x6000000000000000ULL);
	//PrintBitboard(0x0E00000000000000ULL);
	//PrintBitboard(0xA000000000000000ULL);
	//PrintBitboard(0xA0ULL);
	//PrintBitboard(0x0900000000000000ULL);
	//PrintBitboard(0x9ULL);
	//UciCommand(pos, "position startpos moves d2d4 g8f6 c2c4 e7e6 b1c3 f8b4 g1f3 c7c5 e2e3 e8g8 a2a3");
	//UciCommand(pos, "position fen 2r1k2r/1b1p1ppp/p3p3/2R1P3/1nKN4/2b1BN1P/1q3PP1/3Q1B1R b k - 9 19");
	//UciCommand(pos,"print");
	//UciCommand(pos, "perft");
	//UciCommand(pos, "go depth 1");
	char line[4000];
	while (fgets(line, sizeof(line), stdin))
		UciCommand(pos, line);
}

static void Init() {
	zob_side = Rand64();
	for (int p = 0; p < PIECE_NB; ++p)
		for (int sq = 0; sq < SQ_NB; ++sq)
			zob_piece[p][sq] = Rand64();
	for (int i = 0; i < 16; ++i)
		zob_castle[i] = Rand64();
	for (int sq = 0; sq < 64; ++sq) {
		U64 bb = 1ULL << sq;
		bbSquare[sq] = bb;
		bbKnightAttack[sq] = KnightAttackBB(bb);
		bbKingAttack[sq] = KingAttackBB(bb);
	}
	for (int pt = PAWN; pt <= KING; pt++)
		for (int sq = 0; sq < 64; sq++) {
			int mg = mg_material[pt] + mg_table[pt][FLIP(sq)];
			int eg = eg_material[pt] + eg_table[pt][FLIP(sq)];
			mg_pst[pt][sq] = mg;
			eg_pst[pt][sq] = eg;
			mg_pst[PT_NB + pt][FLIP(sq)] = mg;
			eg_pst[PT_NB + pt][FLIP(sq)] = eg;
		}
}

static void InitTT(U64 mb) {
	ttSize = 1;
	while (ttSize < mb * 1e6 / (sizeof(TTEntry) + sizeof(TTEntryEval)))
		ttSize <<= 1;
	ttMask = ttSize - 1;
	free(tt);
	free(ttEval);
	tt = (TTEntry*)malloc(sizeof(TTEntry) * ttSize);
	ttEval = (TTEntryEval*)malloc(sizeof(TTEntryEval) * ttSize);
	TTClear();
}

int main(const int argc, const char** argv) {
	Position pos;
	Init();
	InitSlidersBmi2();
	InitTT(32);
	printf("%s %s\n", NAME, VERSION);
	SetFen(&pos, START_FEN);
	UciLoop(&pos);
}