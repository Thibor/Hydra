
#include <cassert>
#include <iostream>
#include <string>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif

using namespace std;

#define DO_NULL    1
#define NO_NULL    0
#define IS_PV      1
#define NO_PV      0
#define TIMEBUFFER 500
#define MOVESTOGO 24
#define INF 32001
#define MATE 32000
#define MAX_PLY 64
#define U64 unsigned __int64
#define U32 unsigned __int32
#define U16 unsigned __int16
#define U8  unsigned __int8
#define S64 signed   __int64
#define S32 signed   __int32
#define S16 signed   __int16
#define S8  signed   __int8
#define SQ  unsigned __int8
#define SORT_KING 400000000
#define SORT_HASH 200000000
#define SORT_CAPT 100000000
#define SORT_PROM  90000000
#define SORT_KILL  80000000
#define STARTFEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
#define NAME "Hydra"
#define VERSION "2026-02-03"

enum epiece {
	KING,
	QUEEN,
	ROOK,
	BISHOP,
	KNIGHT,
	PAWN,
	PIECE_EMPTY
};

enum ecolor {
	WHITE,
	BLACK,
	COLOR_EMPTY
};

enum esqare {
	A1 = 0, B1, C1, D1, E1, F1, G1, H1,
	A2 = 16, B2, C2, D2, E2, F2, G2, H2,
	A3 = 32, B3, C3, D3, E3, F3, G3, H3,
	A4 = 48, B4, C4, D4, E4, F4, G4, H4,
	A5 = 64, B5, C5, D5, E5, F5, G5, H5,
	A6 = 80, B6, C6, D6, E6, F6, G6, H6,
	A7 = 96, B7, C7, D7, E7, F7, G7, H7,
	A8 = 112, B8, C8, D8, E8, F8, G8, H8
};

enum ecastle {
	CASTLE_WK = 1,
	CASTLE_WQ = 2,
	CASTLE_BK = 4,
	CASTLE_BQ = 8
};

enum emflag {
	MFLAG_NORMAL = 0,
	MFLAG_CAPTURE = 1,
	MFLAG_EPCAPTURE = 2,
	MFLAG_CASTLE = 4,
	MFLAG_EP = 8,
	MFLAG_PROMOTION = 16,
	MFLAG_NULLMOVE = 32
};

struct s_Board {
	U8	 pieces[128];
	U8	 color[128];
	char stm;        // side to move: 0 = white,  1 = black
	char castle;     // 1 = shortW, 2 = longW, 4 = shortB, 8 = longB
	char ep;         // en passant square
	U8   ply;
	U64  hash;
	U64	 phash;
	int  rep_index;
	U64  rep_stack[1024];
	S8   king_loc[2];
	int  pcsq_mg[2];
	int  pcsq_eg[2];
	int piece_material[2];
	int pawn_material[2];
	U8 piece_cnt[2][6];
	U8 pawns_on_file[2][8];
	U8 pawns_on_rank[2][8];
	U8 pawn_ctrl[2][128];
};
extern s_Board board;


struct s_Move {
	char id;
	SQ from;
	SQ to;
	U8 piece_from;
	U8 piece_to;
	U8 piece_cap;
	char flags;
	char castle;
	char ply;
	char ep;
	int score;
};


struct s_SearchDriver {
	int myside;
	int history[2][128][128];
	int cutoff[2][128][128];
	s_Move killers[1024][2];
	char pv[2048];
	S16 score;
};
extern s_SearchDriver sd;

enum etimef {
	FTIME = 0x1,
	FINC = 0x2,
	FMOVESTOGO = 0x4,
	FDEPTH = 0x8,
	FNODES = 0x10,
	FMATE = 0x20,
	FMOVETIME = 0x40,
	FINFINITE = 0x80
};

struct s_SearchInfo {
	bool ponder;
	bool post;
	bool stop;
	int time[2];
	int inc[2];
	U64 nodes;
	int depthLimit;
	U64 nodesLimit;
	int timeLimit;
	U8 flags;
	U32 timeStart;
};
extern s_SearchInfo info;

struct s_Options
{
	bool ponder = true;
	int contempt = 10;
	int aspiration = 50;  // size of the aspiration window ( val-ASPITATION, val+ASPIRATION )
	int elo = 2500;
	int eloMin = 1000;
	int eloMax = 2500;
};
extern s_Options options;

struct s_eval_data
{
	int PIECE_VALUE[6];
	int SORT_VALUE[6];

	/* Piece-square tables - we use size of the board representation,
	not 0..63, to avoid re-indexing. Initialization routine, however,
	uses 0..63 format for clarity */
	int mgPst[6][2][128];
	int egPst[6][2][128];

	/* piece-square tables for pawn structure */

	int weak_pawn[2][128]; // isolated and backward pawns are scored in the same way
	int passed_pawn[2][128];
	int protected_passer[2][128];

	int sqNearK[2][128][128];

	/* single values - letter p before a name signifies a penalty */

	int BISHOP_PAIR;
	int P_KNIGHT_PAIR;
	int P_ROOK_PAIR;
	int ROOK_OPEN;
	int ROOK_HALF;
	int P_BISHOP_TRAPPED_A7;
	int P_BISHOP_TRAPPED_A6;
	int P_KNIGHT_TRAPPED_A8;
	int P_KNIGHT_TRAPPED_A7;
	int P_BLOCK_CENTRAL_PAWN;
	int P_KING_BLOCKS_ROOK;

	int SHIELD_2;
	int SHIELD_3;
	int P_NO_SHIELD;

	int RETURNING_BISHOP;
	int P_C3_KNIGHT;
	int P_NO_FIANCHETTO;
	int FIANCHETTO;
	int TEMPO;
	int ENDGAME_MAT;
};
extern s_eval_data e;

extern char vector[5][8];
extern bool slide[5];
extern char vectors[5];

void clearBoard();
void FillSq(U8 color, U8 piece, S8 sq);
void ClearSq(SQ sq);
int board_loadFromFen(char* fen);

void PrintBest();
void UciCommand(char* command);
bool CheckUp();

U8 movegen(s_Move* moves, U8 tt_move);
U8 movegen_qs(s_Move* moves);
void movegen_sort(U8 movecount, s_Move* m, U8 current);


void SquareToStr(SQ sq, char* a);
SQ StrToSquare(char* a);
char* MoveToStr(s_Move m, char* a);
bool algebraic_moves(char* a);


int move_make(s_Move move);
int move_unmake(s_Move move);
int move_makeNull();
int move_unmakeNull(char ep);

// the next couple of functions respond to questions about moves or move lists

int move_iscapt(s_Move m);
int move_isprom(s_Move m);
int move_canSimplify(s_Move m);
int move_countLegal();
bool move_isLegal(s_Move m);


s_Move StrToMove(char* a);

//transposition
struct szobrist {
	U64 piecesquare[6][2][128];
	U64 color;
	U64 castling[16];
	U64 ep[128];
};
extern szobrist zobrist;

enum ettflag {
	TT_EXACT,
	TT_ALPHA,
	TT_BETA
};

struct stt_entry {
	U64  hash;
	int  val;
	U8	 depthLimit;
	U8   flags;
	U8   bestmove;
};
extern stt_entry* tt;

struct spawntt_entry {
	U64  hash;
	int  val;
};
//extern class spawntt_entry* ptt;

struct sevaltt_entry {
	U64 hash;
	int val;
};
//extern sevaltt_entry* ett;

extern U64 tt_size;
extern int ptt_size;
extern int ett_size;

#define ROW_1   ( A1 >> 4 )
#define ROW_2   ( A2 >> 4 )
#define ROW_3   ( A3 >> 4 )
#define ROW_4   ( A4 >> 4 )
#define ROW_5   ( A5 >> 4 )
#define ROW_6   ( A6 >> 4 )
#define ROW_7   ( A7 >> 4 )
#define ROW_8   ( A8 >> 4 )

/* column identifiers */
#define COL_A  ( A1 & 7 )
#define COL_B  ( B1 & 7 )
#define COL_C  ( C1 & 7 )
#define COL_D  ( D1 & 7 )
#define COL_E  ( E1 & 7 )
#define COL_F  ( F1 & 7 )
#define COL_G  ( G1 & 7 )
#define COL_H  ( H1 & 7 )

/* vectors */
#define NORTH  16
#define NN    ( NORTH + NORTH )
#define SOUTH  -16
#define SS    ( SOUTH + SOUTH )
#define EAST  1
#define WEST  -1
#define NE    17
#define SW    -17
#define NW    15
#define SE    -15

/* generate square number from row and column */
#define SET_SQ(row,col) (row * 16 + col)

/* does a given number represent a square on the board? */
#define IS_SQ(x)  ( (x) & 0x88 ) ? (0) : (1)

/* get board column that a square is part of */
#define COL(sq)  ( (sq) & 7 )

/* get board row that a square is part of */
#define ROW(sq)  ( (sq) >> 4 )

/* determine if two squares lie on the same column */
#define SAME_COL(sq1,sq2) ( ( COL(sq1) == COL(sq2) ) ? (1) : (0) )

/* determine if two squares lie in the same row */
#define SAME_ROW(sq1,sq2) ( ( ROW(sq1) == ROW(sq2) ) ? (1) : (0) )

U64 rand64();
int tt_init();
int tt_setsize(int size);
int tt_probe(U8 depthLimit, int alpha, int beta, char* best);
void tt_save(U8 depthLimit, int val, char flags, char best);
int ttpawn_setsize(int size);
int ttpawn_probe();
void ttpawn_save(int val);
int tteval_setsize(int size);
int tteval_probe();
void tteval_save(int val);
U64 ttPermill();
void search_run();
void clearHistoryTable();
void setDefaultEval();
void setBasicValues();
void setSquaresNearKing();
void setPcsq();
int eval(int alpha, int beta, int use_hash);
int isPiece(U8 color, U8 piece, SQ sq);
int getTropism(int sq1, int sq2);
void PrintBoard();
void printEval();
void printEvalFactor(int wh, int bl);
int SearchQuiesce(int alpha, int beta);
bool badCapture(s_Move move);
bool Blind(s_Move move);
bool isAttacked(char byColor, SQ sq);
bool leaperAttack(char byColor, SQ sq, char byPiece);
bool straightAttack(char byColor, SQ sq, int vect);
bool diagAttack(int byColor, SQ sq, int vect);
bool bishAttack(int byColor, SQ sq, int vect);
void UciBench();
void UciPerformance();
void PerftDriver(U8 depthLimit);
void GetPv(char* pv);
unsigned int GetTimeMs();
bool isRepetition();
void PrintBenchHeader();
void PrintPerformanceHeader();
void PrintSummary(unsigned int time, unsigned long long nodes);
int wKingShield();
int bKingShield();
int getPawnScore();
int evalPawnStructure();
int EvalPawn(SQ sq, S8 side);
void EvalKnight(SQ sq, S8 side);
void EvalBishop(SQ sq, S8 side);
void EvalRook(SQ sq, S8 side);
void EvalQueen(SQ sq, S8 side);
bool isPawnSupported(SQ sq, S8 side);
void blockedPieces(int side);
void UciLoop();
void movegen_push(char from, char to, U8 piece_from, U8 piece_cap, char flags);
void movegen_pawn_move(SQ sq, bool promotion_only);
void movegen_pawn_capt(SQ sq);
void SearchIterate();
int SearchWiden(int depthLimit, int val);
void ResetInfo();
int SearchRoot(U8 depth, int alpha, int beta);
int SearchAlpha(U8 depth, U8 ply, int alpha, int beta, int can_null, int is_pv);
void setKillers(s_Move m, U8 ply);
void ReorderMoves(s_Move* m, U8 mcount, U8 ply);
void PrintInfo(int depth, int val);
void ageHistoryTable();
int Contempt();

/******************************************************************************
*  We want our eval to be color-independent, i.e. the same functions ought to *
*  be called for white and black pieces. This requires some way of converting *
*  row and square coordinates.                                                *
******************************************************************************/

static const int seventh[2] = { ROW(A7), ROW(A2) };
static const int eighth[2] = { ROW(A8), ROW(A1) };
static const int stepFwd[2] = { NORTH, SOUTH };
static const int stepBck[2] = { SOUTH, NORTH };

static const int inv_sq[128] = {
		A8, B8, C8, D8, E8, F8, G8, H8, -1, -1, -1, -1, -1, -1, -1, -1,
		A7, B7, C7, D7, E7, F7, G7, H7, -1, -1, -1, -1, -1, -1, -1, -1,
		A6, B6, C6, D6, E6, F6, G6, H6, -1, -1, -1, -1, -1, -1, -1, -1,
		A5, B5, C5, D5, E5, F5, G5, H5, -1, -1, -1, -1, -1, -1, -1, -1,
		A4, B4, C4, D4, E4, F4, G4, H4, -1, -1, -1, -1, -1, -1, -1, -1,
		A3, B3, C3, D3, E3, F3, G3, H3, -1, -1, -1, -1, -1, -1, -1, -1,
		A2, B2, C2, D2, E2, F2, G2, H2, -1, -1, -1, -1, -1, -1, -1, -1,
		A1, B1, C1, D1, E1, F1, G1, H1, -1, -1, -1, -1, -1, -1, -1, -1
};

#define REL_SQ(cl, sq)       ((cl) == (WHITE) ? (sq) : (inv_sq[sq]))

/* adjustements of piece value based on the number of own pawns */
int n_adj[9] = { -20, -16, -12, -8, -4,  0,  4,  8, 12 };
int r_adj[9] = { 15,  12,   9,  6,  3,  0, -3, -6, -9 };

static const int SafetyTable[100] = {
	 0,  0,   1,   2,   3,   5,   7,   9,  12,  15,
	18,  22,  26,  30,  35,  39,  44,  50,  56,  62,
	68,  75,  82,  85,  89,  97, 105, 113, 122, 131,
	140, 150, 169, 180, 191, 202, 213, 225, 237, 248,
	260, 272, 283, 295, 307, 319, 330, 342, 354, 366,
	377, 389, 401, 412, 424, 436, 448, 459, 471, 483,
	494, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	500, 500, 500, 500, 500, 500, 500, 500, 500, 500
};

/******************************************************************************
*  This struct holds data about certain aspects of evaluation, which allows   *
*  our program to print them if desired.                                      *
******************************************************************************/

struct eval_vector {
	int gamePhase;   // function of piece material: 24 in opening, 0 in endgame
	int mgMob[2];     // midgame mobility
	int egMob[2];     // endgame mobility
	int attCnt[2];    // no. of pieces attacking zone around enemy king
	int attWeight[2]; // weight of attacking pieces - index to SafetyTable
	int mgTropism[2]; // midgame king tropism score
	int egTropism[2]; // endgame king tropism score
	int kingShield[2];
	int adjustMaterial[2];
	int blockages[2];
	int positionalThemes[2];
} v;

s_eval_data e;

// tables used for translating piece/square tables to internal 0x88 representation

int index_white[64] = {
	A8, B8, C8, D8, E8, F8, G8, H8,
	A7, B7, C7, D7, E7, F7, G7, H7,
	A6, B6, C6, D6, E6, F6, G6, H6,
	A5, B5, C5, D5, E5, F5, G5, H5,
	A4, B4, C4, D4, E4, F4, G4, H4,
	A3, B3, C3, D3, E3, F3, G3, H3,
	A2, B2, C2, D2, E2, F2, G2, H2,
	A1, B1, C1, D1, E1, F1, G1, H1
};

int index_black[64] = {
	A1, B1, C1, D1, E1, F1, G1, H1,
	A2, B2, C2, D2, E2, F2, G2, H2,
	A3, B3, C3, D3, E3, F3, G3, H3,
	A4, B4, C4, D4, E4, F4, G4, H4,
	A5, B5, C5, D5, E5, F5, G5, H5,
	A6, B6, C6, D6, E6, F6, G6, H6,
	A7, B7, C7, D7, E7, F7, G7, H7,
	A8, B8, C8, D8, E8, F8, G8, H8
};

/******************************************************************************
*                           PAWN PCSQ                                         *
*                                                                             *
*  Unlike TSCP, CPW generally doesn't want to advance its pawns. Its piece/   *
*  square table for pawns takes into account the following factors:           *
*                                                                             *
*  - file-dependent component, encouraging program to capture                 *
*    towards the center                                                       *
*  - small bonus for staying on the 2nd rank                                  *
*  - small bonus for standing on a3/h3                                        *
*  - penalty for d/e pawns on their initial squares                           *
*  - bonus for occupying the center                                           *
******************************************************************************/

int org_pawn_pcsq_mg[64] = {
	 0,   0,   0,   0,   0,   0,   0,   0,
	-6,  -4,   1,   1,   1,   1,  -4,  -6,
	-6,  -4,   1,   2,   2,   1,  -4,  -6,
	-6,  -4,   2,   8,   8,   2,  -4,  -6,
	-6,  -4,   5,  10,  10,   5,  -4,  -6,
	-4,  -4,   1,   5,   5,   1,  -4,  -4,
	-6,  -4,   1, -24,  -24,  1,  -4,  -6,
	 0,   0,   0,   0,   0,   0,   0,   0
};

int pawn_pcsq_eg[64] = {
	 0,   0,   0,   0,   0,   0,   0,   0,
	-6,  -4,   1,   1,   1,   1,  -4,  -6,
	-6,  -4,   1,   2,   2,   1,  -4,  -6,
	-6,  -4,   2,   8,   8,   2,  -4,  -6,
	-6,  -4,   5,  10,  10,   5,  -4,  -6,
	-4,  -4,   1,   5,   5,   1,  -4,  -4,
	-6,  -4,   1, -24,  -24,  1,  -4,  -6,
	 0,   0,   0,   0,   0,   0,   0,   0
};
int pawn_pcsq_mg[64] = {};

/******************************************************************************
*    KNIGHT PCSQ                                                              *
*                                                                             *
*   - centralization bonus                                                    *
*   - rim and back rank penalty, including penalty for not being developed    *
******************************************************************************/

int org_knight_pcsq_mg[64] = {
	-8,  -8,  -8,  -8,  -8,  -8,  -8,  -8,
	-8,   0,   0,   0,   0,   0,   0,  -8,
	-8,   0,   4,   6,   6,   4,   0,  -8,
	-8,   0,   6,   8,   8,   6,   0,  -8,
	-8,   0,   6,   8,   8,   6,   0,  -8,
	-8,   0,   4,   6,   6,   4,   0,  -8,
	-8,   0,   1,   2,   2,   1,   0,  -8,
   -16, -12,  -8,  -8,  -8,  -8, -12,  -16
};

int knight_pcsq_eg[64] = {
	-8,  -8,  -8,  -8,  -8,  -8,  -8,  -8,
	-8,   0,   0,   0,   0,   0,   0,  -8,
	-8,   0,   4,   6,   6,   4,   0,  -8,
	-8,   0,   6,   8,   8,   6,   0,  -8,
	-8,   0,   6,   8,   8,   6,   0,  -8,
	-8,   0,   4,   6,   6,   4,   0,  -8,
	-8,   0,   1,   2,   2,   1,   0,  -8,
   -16, -12,  -8,  -8,  -8,  -8, -12,  -16
};
int knight_pcsq_mg[64] = {};
/******************************************************************************
*                BISHOP PCSQ                                                  *
*                                                                             *
*   - centralization bonus, smaller than for knight                           *
*   - penalty for not being developed                                         *
*   - good squares on the own half of the board                               *
******************************************************************************/

int org_bishop_pcsq_mg[64] = {
	-4,  -4,  -4,  -4,  -4,  -4,  -4,  -4,
	-4,   0,   0,   0,   0,   0,   0,  -4,
	-4,   0,   2,   4,   4,   2,   0,  -4,
	-4,   0,   4,   6,   6,   4,   0,  -4,
	-4,   0,   4,   6,   6,   4,   0,  -4,
	-4,   1,   2,   4,   4,   2,   1,  -4,
	-4,   2,   1,   1,   1,   1,   2,  -4,
	-4,  -4, -12,  -4,  -4, -12,  -4,  -4
};

int bishop_pcsq_eg[64] = {
	-4,  -4,  -4,  -4,  -4,  -4,  -4,  -4,
	-4,   0,   0,   0,   0,   0,   0,  -4,
	-4,   0,   2,   4,   4,   2,   0,  -4,
	-4,   0,   4,   6,   6,   4,   0,  -4,
	-4,   0,   4,   6,   6,   4,   0,  -4,
	-4,   1,   2,   4,   4,   2,   1,  -4,
	-4,   2,   1,   1,   1,   1,   2,  -4,
	-4,  -4, -12,  -4,  -4, -12,  -4,  -4
};
int bishop_pcsq_mg[64] = {};
/******************************************************************************
*                        ROOK PCSQ                                            *
*                                                                             *
*    - bonus for 7th and 8th ranks                                            *
*    - penalty for a/h columns                                                *
*    - small centralization bonus                                             *
******************************************************************************/

int org_rook_pcsq_mg[64] = {
	 5,   5,   5,   5,   5,   5,   5,   5,
	-5,   0,   0,   0,   0,   0,   0,  -5,
	-5,   0,   0,   0,   0,   0,   0,  -5,
	-5,   0,   0,   0,   0,   0,   0,  -5,
	-5,   0,   0,   0,   0,   0,   0,  -5,
	-5,   0,   0,   0,   0,   0,   0,  -5,
	-5,   0,   0,   0,   0,   0,   0,  -5,
	 0,   0,   0,   2,   2,   0,   0,   0
};

int rook_pcsq_eg[64] = {
	 5,   5,   5,   5,   5,   5,   5,   5,
	-5,   0,   0,   0,   0,   0,   0,  -5,
	-5,   0,   0,   0,   0,   0,   0,  -5,
	-5,   0,   0,   0,   0,   0,   0,  -5,
	-5,   0,   0,   0,   0,   0,   0,  -5,
	-5,   0,   0,   0,   0,   0,   0,  -5,
	-5,   0,   0,   0,   0,   0,   0,  -5,
	 0,   0,   0,   2,   2,   0,   0,   0
};
int rook_pcsq_mg[64] = {};
/******************************************************************************
*                     QUEEN PCSQ                                              *
*                                                                             *
* - small bonus for centralization in the endgame                             *
* - penalty for staying on the 1st rank, between rooks in the midgame         *
******************************************************************************/

int org_queen_pcsq_mg[64] = {
	 0,   0,   0,   0,   0,   0,   0,   0,
	 0,   0,   1,   1,   1,   1,   0,   0,
	 0,   0,   1,   2,   2,   1,   0,   0,
	 0,   0,   2,   3,   3,   2,   0,   0,
	 0,   0,   2,   3,   3,   2,   0,   0,
	 0,   0,   1,   2,   2,   1,   0,   0,
	 0,   0,   1,   1,   1,   1,   0,   0,
	-5,  -5,  -5,  -5,  -5,  -5,  -5,  -5
};

int queen_pcsq_eg[64] = {
	 0,   0,   0,   0,   0,   0,   0,   0,
	 0,   0,   1,   1,   1,   1,   0,   0,
	 0,   0,   1,   2,   2,   1,   0,   0,
	 0,   0,   2,   3,   3,   2,   0,   0,
	 0,   0,   2,   3,   3,   2,   0,   0,
	 0,   0,   1,   2,   2,   1,   0,   0,
	 0,   0,   1,   1,   1,   1,   0,   0,
	-5,  -5,  -5,  -5,  -5,  -5,  -5,  -5
};
int queen_pcsq_mg[64] = {};

int king_pcsq_mg[64] = {
   -40, -30, -50, -70, -70, -50, -30, -40,
   -30, -20, -40, -60, -60, -40, -20, -30,
   -20, -10, -30, -50, -50, -30, -10, -20,
   -10,   0, -20, -40, -40, -20,   0, -10,
	 0,  10, -10, -30, -30, -10,  10,   0,
	10,  20,   0, -20, -20,   0,  20,  10,
	30,  40,  20,   0,   0,  20,  40,  30,
	40,  50,  30,  10,  10,  30,  50,  40
};

int king_pcsq_eg[64] = {
   -72, -48, -36, -24, -24, -36, -48, -72,
   -48, -24, -12,   0,   0, -12, -24, -48,
   -36, -12,   0,  12,  12,   0, -12, -36,
   -24,   0,  12,  24,  24,  12,   0, -24,
   -24,   0,  12,  24,  24,  12,   0, -24,
   -36, -12,   0,  12,  12,   0, -12, -36,
   -48, -24, -12,   0,   0, -12, -24, -48,
   -72, -48, -36, -24, -24, -36, -48, -72
};

/******************************************************************************
*                     WEAK PAWNS PCSQ                                         *
*                                                                             *
*  Current version of CPW-engine does not differentiate between isolated and  *
*  backward pawns, using one  generic  cathegory of  weak pawns. The penalty  *
*  is bigger in the center, on the assumption that weak central pawns can be  *
*  attacked  from many  directions. If the penalty seems too low, please note *
*  that being on a semi-open file will come into equation, too.               *
******************************************************************************/

int weak_pawn_pcsq[64] = {
	 0,   0,   0,   0,   0,   0,   0,   0,
   -10, -12, -14, -16, -16, -14, -12, -10,
   -10, -12, -14, -16, -16, -14, -12, -10,
   -10, -12, -14, -16, -16, -14, -12, -10,
   -10, -12, -14, -16, -16, -14, -12, -10,
   -10, -12, -14, -16, -16, -14, -12, -10,
   -10, -12, -14, -16, -16, -14, -12, -10,
	 0,   0,   0,   0,   0,   0,   0,   0
};

int passed_pawn_pcsq[64] = {
	 0,   0,   0,   0,   0,   0,   0,   0,
   140, 140, 140, 140, 140, 140, 140, 140,
	92,  92,  92,  92,  92,  92,  92,  92,
	56,  56,  56,  56,  56,  56,  56,  56,
	32,  32,  32,  32,  32,  32,  32,  32,
	20,  20,  20,  20,  20,  20,  20,  20,
	20,  20,  20,  20,  20,  20,  20,  20,
	 0,   0,   0,   0,   0,   0,   0,   0
};

int material[6] = { 0,975,500,335,325,100 };
#pragma endregion

int EloRnd(int range) {
	return rand() % (range + 1) - (range >> 1);
}

static void SetElo() {
	int elo = options.elo;
	if (elo < options.eloMin)
		elo = options.eloMin;
	if (elo > options.eloMax)
		elo = options.eloMax;
	elo -= options.eloMin;
	int eloRange = options.eloMax - options.eloMin;
	int range = 800 - (elo * 800) / eloRange;
	for (int n = 0; n < 64; n++) {
		pawn_pcsq_mg[n] = org_pawn_pcsq_mg[n] + EloRnd(range);
		knight_pcsq_mg[n] = org_knight_pcsq_mg[n] + EloRnd(range);
		bishop_pcsq_mg[n] = org_bishop_pcsq_mg[n] + EloRnd(range);
		rook_pcsq_mg[n] = org_rook_pcsq_mg[n] + EloRnd(range);
		queen_pcsq_mg[n] = org_queen_pcsq_mg[n] + EloRnd(range);
	}
}

void setDefaultEval() {
	SetElo();
	setBasicValues();
	setSquaresNearKing();
	setPcsq();
}

void setBasicValues() {
	for (int n = 0; n < 6; n++)
		e.PIECE_VALUE[n] = material[n];
	e.BISHOP_PAIR = 30;
	e.P_KNIGHT_PAIR = 8;
	e.P_ROOK_PAIR = 16;

	/*************************************************
	* Values used for sorting captures are the same  *
	* as normal piece values, except for a king.     *
	*************************************************/

	for (int i = 0; i < 6; ++i) {
		e.SORT_VALUE[i] = e.PIECE_VALUE[i];
	}
	e.SORT_VALUE[KING] = SORT_KING;

	/* trapped and blocked pieces */
	e.P_KING_BLOCKS_ROOK = 24;
	e.P_BLOCK_CENTRAL_PAWN = 24;
	e.P_BISHOP_TRAPPED_A7 = 150;
	e.P_BISHOP_TRAPPED_A6 = 50;
	e.P_KNIGHT_TRAPPED_A8 = 150;
	e.P_KNIGHT_TRAPPED_A7 = 100;

	/* minor penalties */
	e.P_C3_KNIGHT = 5;
	e.P_NO_FIANCHETTO = 4;

	/* king's defence */
	e.SHIELD_2 = 10;
	e.SHIELD_3 = 5;
	e.P_NO_SHIELD = 10;

	/* minor bonuses */
	e.ROOK_OPEN = 10;
	e.ROOK_HALF = 5;
	e.RETURNING_BISHOP = 20;
	e.FIANCHETTO = 4;
	e.TEMPO = 10;

	e.ENDGAME_MAT = 1300;
}

void setSquaresNearKing() {
	for (int i = 0; i < 128; ++i)
		for (int j = 0; j < 128; ++j)
		{

			e.sqNearK[WHITE][i][j] = 0;
			e.sqNearK[BLACK][i][j] = 0;

			if (IS_SQ(i) && IS_SQ(j)) {

				/* squares constituting the ring around both kings */
				if (j == i + NORTH || j == i + SOUTH
					|| j == i + EAST || j == i + WEST
					|| j == i + NW || j == i + NE
					|| j == i + SW || j == i + SE) {

					e.sqNearK[WHITE][i][j] = 1;
					e.sqNearK[BLACK][i][j] = 1;
				}

				/* squares in front of the white king ring */
				if (j == i + NORTH + NORTH
					|| j == i + NORTH + NE
					|| j == i + NORTH + NW)
					e.sqNearK[WHITE][i][j] = 1;

				/* squares in front og the black king ring */
				if (j == i + SOUTH + SOUTH
					|| j == i + SOUTH + SE
					|| j == i + SOUTH + SW)
					e.sqNearK[WHITE][i][j] = 1;
			}
		}
}


void setPcsq() {

	for (int i = 0; i < 64; ++i) {

		e.weak_pawn[WHITE][index_white[i]] = weak_pawn_pcsq[i];
		e.weak_pawn[BLACK][index_black[i]] = weak_pawn_pcsq[i];
		e.passed_pawn[WHITE][index_white[i]] = passed_pawn_pcsq[i];
		e.passed_pawn[BLACK][index_black[i]] = passed_pawn_pcsq[i];

		/* protected passers are slightly stronger than ordinary passers */

		e.protected_passer[WHITE][index_white[i]] = (passed_pawn_pcsq[i] * 10) / 8;
		e.protected_passer[BLACK][index_black[i]] = (passed_pawn_pcsq[i] * 10) / 8;

		/* now set the piece/square tables for each color and piece type */

		e.mgPst[PAWN][WHITE][index_white[i]] = pawn_pcsq_mg[i];
		e.mgPst[PAWN][BLACK][index_black[i]] = pawn_pcsq_mg[i];
		e.mgPst[KNIGHT][WHITE][index_white[i]] = knight_pcsq_mg[i];
		e.mgPst[KNIGHT][BLACK][index_black[i]] = knight_pcsq_mg[i];
		e.mgPst[BISHOP][WHITE][index_white[i]] = bishop_pcsq_mg[i];
		e.mgPst[BISHOP][BLACK][index_black[i]] = bishop_pcsq_mg[i];
		e.mgPst[ROOK][WHITE][index_white[i]] = rook_pcsq_mg[i];
		e.mgPst[ROOK][BLACK][index_black[i]] = rook_pcsq_mg[i];
		e.mgPst[QUEEN][WHITE][index_white[i]] = queen_pcsq_mg[i];
		e.mgPst[QUEEN][BLACK][index_black[i]] = queen_pcsq_mg[i];
		e.mgPst[KING][WHITE][index_white[i]] = king_pcsq_mg[i];
		e.mgPst[KING][BLACK][index_black[i]] = king_pcsq_mg[i];

		e.egPst[PAWN][WHITE][index_white[i]] = pawn_pcsq_eg[i];
		e.egPst[PAWN][BLACK][index_black[i]] = pawn_pcsq_eg[i];
		e.egPst[KNIGHT][WHITE][index_white[i]] = knight_pcsq_eg[i];
		e.egPst[KNIGHT][BLACK][index_black[i]] = knight_pcsq_eg[i];
		e.egPst[BISHOP][WHITE][index_white[i]] = bishop_pcsq_eg[i];
		e.egPst[BISHOP][BLACK][index_black[i]] = bishop_pcsq_eg[i];
		e.egPst[ROOK][WHITE][index_white[i]] = rook_pcsq_eg[i];
		e.egPst[ROOK][BLACK][index_black[i]] = rook_pcsq_eg[i];
		e.egPst[QUEEN][WHITE][index_white[i]] = queen_pcsq_eg[i];
		e.egPst[QUEEN][BLACK][index_black[i]] = queen_pcsq_eg[i];
		e.egPst[KING][WHITE][index_white[i]] = king_pcsq_eg[i];
		e.egPst[KING][BLACK][index_black[i]] = king_pcsq_eg[i];
	}
}

int eval(int alpha, int beta, int use_hash) {
	int result = 0, mgScore = 0, egScore = 0;
	int stronger, weaker;

	/**************************************************************************
	*  Probe the evaluatinon hashtable, unless we call eval() only in order   *
	*  to display detailed result                                             *
	**************************************************************************/

	int probeval = tteval_probe();
	if (probeval != INF && use_hash)
		return probeval;

	/**************************************************************************
	*  Clear all eval data                                                    *
	**************************************************************************/

	v.gamePhase = board.piece_cnt[WHITE][KNIGHT] + board.piece_cnt[WHITE][BISHOP] + 2 * board.piece_cnt[WHITE][ROOK] + 4 * board.piece_cnt[WHITE][QUEEN]
		+ board.piece_cnt[BLACK][KNIGHT] + board.piece_cnt[BLACK][BISHOP] + 2 * board.piece_cnt[BLACK][ROOK] + 4 * board.piece_cnt[BLACK][QUEEN];

	for (int side = 0; side <= 1; side++) {
		v.mgMob[side] = 0;
		v.egMob[side] = 0;
		v.attCnt[side] = 0;
		v.attWeight[side] = 0;
		v.mgTropism[side] = 0;
		v.egTropism[side] = 0;
		v.adjustMaterial[side] = 0;
		v.blockages[side] = 0;
		v.positionalThemes[side] = 0;
		v.kingShield[side] = 0;
	}

	/**************************************************************************
	*  Sum the incrementally counted material and piece/square table values   *
	**************************************************************************/

	mgScore = board.piece_material[WHITE] + board.pawn_material[WHITE] + board.pcsq_mg[WHITE]
		- board.piece_material[BLACK] - board.pawn_material[BLACK] - board.pcsq_mg[BLACK];
	egScore = board.piece_material[WHITE] + board.pawn_material[WHITE] + board.pcsq_eg[WHITE]
		- board.piece_material[BLACK] - board.pawn_material[BLACK] - board.pcsq_eg[BLACK];

	/**************************************************************************
	* add king's pawn shield score and evaluate part of piece blockage score  *
	* (the rest of the latter will be done via piece eval)                    *
	**************************************************************************/

	v.kingShield[WHITE] = wKingShield();
	v.kingShield[BLACK] = bKingShield();
	blockedPieces(WHITE);
	blockedPieces(BLACK);
	mgScore += (v.kingShield[WHITE] - v.kingShield[BLACK]);

	/* tempo bonus */
	if (board.stm == WHITE) result += e.TEMPO;
	else				  result -= e.TEMPO;

	/**************************************************************************
	*  Adjusting material value for the various combinations of pieces.       *
	*  Currently it scores bishop, knight and rook pairs. The first one       *
	*  gets a bonus, the latter two - a penalty. Beside that knights lose     *
	*  value as pawns disappear, whereas rooks gain.                          *
	**************************************************************************/

	if (board.piece_cnt[WHITE][BISHOP] > 1) v.adjustMaterial[WHITE] += e.BISHOP_PAIR;
	if (board.piece_cnt[BLACK][BISHOP] > 1) v.adjustMaterial[BLACK] += e.BISHOP_PAIR;
	if (board.piece_cnt[WHITE][KNIGHT] > 1) v.adjustMaterial[WHITE] -= e.P_KNIGHT_PAIR;
	if (board.piece_cnt[BLACK][KNIGHT] > 1) v.adjustMaterial[BLACK] -= e.P_KNIGHT_PAIR;
	if (board.piece_cnt[WHITE][ROOK] > 1) v.adjustMaterial[WHITE] -= e.P_ROOK_PAIR;
	if (board.piece_cnt[BLACK][ROOK] > 1) v.adjustMaterial[BLACK] -= e.P_ROOK_PAIR;

	v.adjustMaterial[WHITE] += n_adj[board.piece_cnt[WHITE][PAWN]] * board.piece_cnt[WHITE][KNIGHT];
	v.adjustMaterial[BLACK] += n_adj[board.piece_cnt[BLACK][PAWN]] * board.piece_cnt[BLACK][KNIGHT];
	v.adjustMaterial[WHITE] += r_adj[board.piece_cnt[WHITE][PAWN]] * board.piece_cnt[WHITE][ROOK];
	v.adjustMaterial[BLACK] += r_adj[board.piece_cnt[BLACK][PAWN]] * board.piece_cnt[BLACK][ROOK];

	result += getPawnScore();

	/**************************************************************************
	*  Evaluate pieces                                                        *
	**************************************************************************/

	for (U8 row = 0; row < 8; row++)
		for (U8 col = 0; col < 8; col++) {

			S8 sq = SET_SQ(row, col);

			if (board.color[sq] != COLOR_EMPTY) {
				switch (board.pieces[sq]) {
				case PAWN: // pawns are evaluated separately
					break;
				case KNIGHT:
					EvalKnight(sq, board.color[sq]);
					break;
				case BISHOP:
					EvalBishop(sq, board.color[sq]);
					break;
				case ROOK:
					EvalRook(sq, board.color[sq]);
					break;
				case QUEEN:
					EvalQueen(sq, board.color[sq]);
					break;
				case KING:
					break;
				}
			}
		}

	/**************************************************************************
	*  Merge  midgame  and endgame score. We interpolate between  these  two  *
	*  values, using a gamePhase value, based on remaining piece material on  *
	*  both sides. With less pieces, endgame score becomes more influential.  *
	**************************************************************************/

	mgScore += (v.mgMob[WHITE] - v.mgMob[BLACK]);
	egScore += (v.egMob[WHITE] - v.egMob[BLACK]);
	mgScore += (v.mgTropism[WHITE] - v.mgTropism[BLACK]);
	egScore += (v.egTropism[WHITE] - v.egTropism[BLACK]);
	if (v.gamePhase > 24) v.gamePhase = 24;
	int mgWeight = v.gamePhase;
	int egWeight = 24 - mgWeight;
	result += ((mgScore * mgWeight) + (egScore * egWeight)) / 24;

	/**************************************************************************
	*  Add phase-independent score components.                                *
	**************************************************************************/

	result += (v.blockages[WHITE] - v.blockages[BLACK]);
	result += (v.positionalThemes[WHITE] - v.positionalThemes[BLACK]);
	result += (v.adjustMaterial[WHITE] - v.adjustMaterial[BLACK]);

	/**************************************************************************
	*  Merge king attack score. We don't apply this value if there are less   *
	*  than two attackers or if the attacker has no queen.                    *
	**************************************************************************/

	if (v.attCnt[WHITE] < 2 || board.piece_cnt[WHITE][QUEEN] == 0) v.attWeight[WHITE] = 0;
	if (v.attCnt[BLACK] < 2 || board.piece_cnt[BLACK][QUEEN] == 0) v.attWeight[BLACK] = 0;
	result += SafetyTable[v.attWeight[WHITE]];
	result -= SafetyTable[v.attWeight[BLACK]];

	/**************************************************************************
	*  Low material correction - guarding against an illusory material advan- *
	*  tage. Full blown program should have more such rules, but the current  *
	*  set ought to be useful enough. Please note that our code  assumes      *
	*  different material values for bishop and  knight.                      *
	*                                                                         *
	*  - a single minor piece cannot win                                      *
	*  - two knights cannot checkmate bare king                               *
	*  - bare rook vs minor piece is drawish                                  *
	*  - rook and minor vs rook is drawish                                    *
	**************************************************************************/

	if (result > 0) {
		stronger = WHITE;
		weaker = BLACK;
	}
	else {
		stronger = BLACK;
		weaker = WHITE;
	}

	if (board.pawn_material[stronger] == 0) {

		if (board.piece_material[stronger] < 400) return 0;

		if (board.pawn_material[weaker] == 0
			&& (board.piece_material[stronger] == 2 * e.PIECE_VALUE[KNIGHT]))
			return 0;

		if (board.piece_material[stronger] == e.PIECE_VALUE[ROOK]
			&& board.piece_material[weaker] == e.PIECE_VALUE[BISHOP]) result /= 2;

		if (board.piece_material[stronger] == e.PIECE_VALUE[ROOK]
			&& board.piece_material[weaker] == e.PIECE_VALUE[KNIGHT]) result /= 2;

		if (board.piece_material[stronger] == e.PIECE_VALUE[ROOK] + e.PIECE_VALUE[BISHOP]
			&& board.piece_material[weaker] == e.PIECE_VALUE[ROOK]) result /= 2;

		if (board.piece_material[stronger] == e.PIECE_VALUE[ROOK] + e.PIECE_VALUE[KNIGHT]
			&& board.piece_material[weaker] == e.PIECE_VALUE[ROOK]) result /= 2;
	}

	/**************************************************************************
	*  Finally return the score relative to the side to move.                 *
	**************************************************************************/

	if (board.stm == BLACK) result = -result;

	tteval_save(result);

	return result;
}

void EvalKnight(SQ sq, S8 side) {
	int att = 0;
	int mob = 0;
	int pos;

	/**************************************************************************
	*  Collect data about mobility and king attacks. This resembles move      *
	*  generation code, except that we are just incrementing the counters     *
	*  instead of adding actual moves.                                        *
	**************************************************************************/

	for (U8 dir = 0; dir < 8; dir++) {
		pos = sq + vector[KNIGHT][dir];
		if (IS_SQ(pos) && board.color[pos] != side) {
			// we exclude mobility to squares controlled by enemy pawns
			// but don't penalize possible captures
			if (!board.pawn_ctrl[!side][pos]) ++mob;
			if (e.sqNearK[!side][board.king_loc[!side]][pos])
				++att; // this knight is attacking zone around enemy king
		}
	}

	/**************************************************************************
	*  Evaluate mobility. We try to do it in such a way that zero represents  *
	*  average mobility, but  our formula of doing so is a puer guess.        *
	**************************************************************************/

	v.mgMob[side] += 4 * (mob - 4);
	v.egMob[side] += 4 * (mob - 4);

	/**************************************************************************
	*  Save data about king attacks                                           *
	**************************************************************************/

	if (att) {
		v.attCnt[side]++;
		v.attWeight[side] += 2 * att;
	}

	/**************************************************************************
	* Evaluate king tropism                                                   *
	**************************************************************************/

	int tropism = getTropism(sq, board.king_loc[!side]);
	v.mgTropism[side] += 3 * tropism;
	v.egTropism[side] += 3 * tropism;
}

void EvalBishop(SQ sq, S8 side) {

	int att = 0;
	int mob = 0;

	/**************************************************************************
	*  Collect data about mobility and king attacks                           *
	**************************************************************************/

	for (char dir = 0; dir < vectors[BISHOP]; dir++) {

		for (char pos = sq;;) {

			pos = pos + vector[BISHOP][dir];
			if (!IS_SQ(pos)) break;

			if (board.pieces[pos] == PIECE_EMPTY) {
				if (!board.pawn_ctrl[!side][pos]) mob++;
				// we exclude mobility to squares controlled by enemy pawns
				if (e.sqNearK[!side][board.king_loc[!side]][pos]) ++att;
			}
			else {                                // non-empty square
				if (board.color[pos] != side) {         // opponent's piece
					mob++;
					if (e.sqNearK[!side][board.king_loc[!side]][pos]) ++att;
				}
				break;                              // own piece
			}
		}
	}

	v.mgMob[side] += 3 * (mob - 7);
	v.egMob[side] += 3 * (mob - 7);

	if (att) {
		v.attCnt[side]++;
		v.attWeight[side] += 2 * att;
	}

	int tropism = getTropism(sq, board.king_loc[!side]);
	v.mgTropism[side] += 2 * tropism;
	v.egTropism[side] += 1 * tropism;
}

void EvalRook(SQ sq, S8 side) {

	int att = 0;
	int mob = 0;

	/**************************************************************************
	*  Bonus for rook on the seventh rank. It is applied when there are pawns *
	*  to attack along that rank or if enemy king is cut off on 8th rank      *
	/*************************************************************************/

	if (ROW(sq) == seventh[side]
		&& (board.pawns_on_rank[!side][seventh[side]] || ROW(board.king_loc[!side]) == eighth[side])) {
		v.mgMob[side] += 20;
		v.egMob[side] += 30;
	}

	/**************************************************************************
	*  Bonus for open and half-open files is merged with mobility score.      *
	*  Bonus for open files targetting enemy king is added to attWeight[]     *
	/*************************************************************************/

	if (board.pawns_on_file[side][COL(sq)] == 0) {
		if (board.pawns_on_file[!side][COL(sq)] == 0) { // fully open file
			v.mgMob[side] += e.ROOK_OPEN;
			v.egMob[side] += e.ROOK_OPEN;
			if (abs(COL(sq) - COL(board.king_loc[!side])) < 2)
				v.attWeight[side] += 1;
		}
		else {                                    // half open file
			v.mgMob[side] += e.ROOK_HALF;
			v.egMob[side] += e.ROOK_HALF;
			if (abs(COL(sq) - COL(board.king_loc[!side])) < 2)
				v.attWeight[side] += 2;
		}
	}

	/**************************************************************************
	*  Collect data about mobility and king attacks                           *
	**************************************************************************/

	for (char dir = 0; dir < vectors[ROOK]; dir++) {

		for (char pos = sq;;) {

			pos = pos + vector[ROOK][dir];
			if (!IS_SQ(pos)) break;

			if (board.pieces[pos] == PIECE_EMPTY) {
				mob++;
				if (e.sqNearK[!side][board.king_loc[!side]][pos]) ++att;
			}
			else {                                // non-empty square
				if (board.color[pos] != side) {         // opponent's piece
					mob++;
					if (e.sqNearK[!side][board.king_loc[!side]][pos]) ++att;
				}
				break;                              // own piece
			}
		}
	}

	v.mgMob[side] += 2 * (mob - 7);
	v.egMob[side] += 4 * (mob - 7);

	if (att) {
		v.attCnt[side]++;
		v.attWeight[side] += 3 * att;
	}

	int tropism = getTropism(sq, board.king_loc[!side]);
	v.mgTropism[side] += 2 * tropism;
	v.egTropism[side] += 1 * tropism;
}

void EvalQueen(SQ sq, S8 side) {

	int att = 0;
	int mob = 0;

	if (ROW(sq) == seventh[side]
		&& (board.pawns_on_rank[!side][seventh[side]] || ROW(board.king_loc[!side]) == eighth[side])) {
		v.mgMob[side] += 5;
		v.egMob[side] += 10;
	}

	/**************************************************************************
	*  A queen should not be developed too early                              *
	**************************************************************************/

	if ((side == WHITE && ROW(sq) > ROW_2) || (side == BLACK && ROW(sq) < ROW_7)) {
		if (isPiece(side, KNIGHT, REL_SQ(side, B1))) v.positionalThemes[side] -= 2;
		if (isPiece(side, BISHOP, REL_SQ(side, C1))) v.positionalThemes[side] -= 2;
		if (isPiece(side, BISHOP, REL_SQ(side, F1))) v.positionalThemes[side] -= 2;
		if (isPiece(side, KNIGHT, REL_SQ(side, G1))) v.positionalThemes[side] -= 2;
	}

	/**************************************************************************
	*  Collect data about mobility and king attacks                           *
	**************************************************************************/

	for (char dir = 0; dir < vectors[QUEEN]; dir++) {

		for (char pos = sq;;) {

			pos = pos + vector[QUEEN][dir];
			if (!IS_SQ(pos)) break;

			if (board.pieces[pos] == PIECE_EMPTY) {
				mob++;
				if (e.sqNearK[!side][board.king_loc[!side]][pos]) ++att;
			}
			else {                                 // non-empty square
				if (board.color[pos] != side) {          // opponent's piece
					mob++;
					if (e.sqNearK[!side][board.king_loc[!side]][pos]) ++att;
				}
				break;                               // own piece
			}
		}
	}

	v.mgMob[side] += 1 * (mob - 14);
	v.egMob[side] += 2 * (mob - 14);

	if (att) {
		v.attCnt[side]++;
		v.attWeight[side] += 4 * att;
	}

	int tropism = getTropism(sq, board.king_loc[!side]);
	v.mgTropism[side] += 2 * tropism;
	v.egTropism[side] += 4 * tropism;
}

int wKingShield() {

	int result = 0;

	/* king on the kingside */
	if (COL(board.king_loc[WHITE]) > COL_E) {

		if (isPiece(WHITE, PAWN, F2))  result += e.SHIELD_2;
		else if (isPiece(WHITE, PAWN, F3))  result += e.SHIELD_3;

		if (isPiece(WHITE, PAWN, G2))  result += e.SHIELD_2;
		else if (isPiece(WHITE, PAWN, G3))  result += e.SHIELD_3;

		if (isPiece(WHITE, PAWN, H2))  result += e.SHIELD_2;
		else if (isPiece(WHITE, PAWN, H3))  result += e.SHIELD_3;
	}

	/* king on the queenside */
	else if (COL(board.king_loc[WHITE]) < COL_D) {

		if (isPiece(WHITE, PAWN, A2))  result += e.SHIELD_2;
		else if (isPiece(WHITE, PAWN, A3))  result += e.SHIELD_3;

		if (isPiece(WHITE, PAWN, B2))  result += e.SHIELD_2;
		else if (isPiece(WHITE, PAWN, B3))  result += e.SHIELD_3;

		if (isPiece(WHITE, PAWN, C2))  result += e.SHIELD_2;
		else if (isPiece(WHITE, PAWN, C3))  result += e.SHIELD_3;
	}

	return result;
}

int bKingShield() {
	int result = 0;

	/* king on the kingside */
	if (COL(board.king_loc[BLACK]) > COL_E) {
		if (isPiece(BLACK, PAWN, F7))  result += e.SHIELD_2;
		else if (isPiece(BLACK, PAWN, F6))  result += e.SHIELD_3;

		if (isPiece(BLACK, PAWN, G7))  result += e.SHIELD_2;
		else if (isPiece(BLACK, PAWN, G6))  result += e.SHIELD_3;

		if (isPiece(BLACK, PAWN, H7))  result += e.SHIELD_2;
		else if (isPiece(BLACK, PAWN, H6))  result += e.SHIELD_3;
	}

	/* king on the queenside */
	else if (COL(board.king_loc[BLACK]) < COL_D) {
		if (isPiece(BLACK, PAWN, A7))  result += e.SHIELD_2;
		else if (isPiece(BLACK, PAWN, A6))  result += e.SHIELD_3;

		if (isPiece(BLACK, PAWN, B7))  result += e.SHIELD_2;
		else if (isPiece(BLACK, PAWN, B6))  result += e.SHIELD_3;

		if (isPiece(BLACK, PAWN, C7))  result += e.SHIELD_2;
		else if (isPiece(BLACK, PAWN, C6))  result += e.SHIELD_3;
	}
	return result;
}

/******************************************************************************
*                            Pawn structure evaluaton                         *
******************************************************************************/

int getPawnScore() {
	int result;

	/**************************************************************************
	*  This function wraps hashing mechanism around evalPawnStructure().      *
	*  Please note  that since we use the pawn hashtable, evalPawnStructure() *
	*  must not take into account the piece position.  In a more elaborate    *
	*  program, pawn hashtable would contain only the characteristics of pawn *
	*  structure,  and scoring them in conjunction with the piece position    *
	*  would have been done elsewhere.                                        *
	**************************************************************************/

	int probeval = ttpawn_probe();
	if (probeval != INF)
		return probeval;

	result = evalPawnStructure();
	ttpawn_save(result);
	return result;
}

int evalPawnStructure() {
	int result = 0;

	for (U8 row = 0; row < 8; row++)
		for (U8 col = 0; col < 8; col++) {

			S8 sq = SET_SQ(row, col);

			if (board.pieces[sq] == PAWN) {
				if (board.color[sq] == WHITE) result += EvalPawn(sq, WHITE);
				else                      result -= EvalPawn(sq, BLACK);
			}
		}

	return result;
}

int EvalPawn(SQ sq, S8 side) {
	int result = 0;
	int flagIsPassed = 1; // we will be trying to disprove that
	int flagIsWeak = 1;   // we will be trying to disprove that
	int flagIsOpposed = 0;

	/**************************************************************************
	*   We have only very basic data structures that do not update informa-   *
	*   tion about pawns incrementally, so we have to calculate everything    *
	*   here.  The loop below detects doubled pawns, passed pawns and sets    *
	*   a flag on finding that our pawn is opposed by enemy pawn.             *
	**************************************************************************/

	if (board.pawn_ctrl[!side][sq]) // if a pawn is attacked by a pawn, it is not
		flagIsPassed = 0;       // passed (not sure if it's the best decision)

	S8 nextSq = sq + stepFwd[side];

	while (IS_SQ(nextSq)) {

		if (board.pieces[nextSq] == PAWN) { // either opposed by enemy pawn or doubled
			flagIsPassed = 0;
			if (board.color[nextSq] == side)
				result -= 20;       // doubled pawn penalty
			else
				flagIsOpposed = 1;  // flag our pawn as opposed
		}

		if (board.pawn_ctrl[!side][nextSq])
			flagIsPassed = 0;

		nextSq += stepFwd[side];
	}

	/**************************************************************************
	*   Another loop, going backwards and checking whether pawn has support.  *
	*   Here we can at least break out of it for speed optimization.          *
	**************************************************************************/

	nextSq = sq + stepFwd[side]; // so that a pawn in a duo will not be considered weak

	while (IS_SQ(nextSq)) {

		if (board.pawn_ctrl[side][nextSq]) {
			flagIsWeak = 0;
			break;
		}

		nextSq += stepBck[side];
	}

	/**************************************************************************
	*  Evaluate passed pawns, scoring them higher if they are protected       *
	*  or if their advance is supported by friendly pawns                     *
	**************************************************************************/

	if (flagIsPassed) {
		if (isPawnSupported(sq, side)) result += e.protected_passer[side][sq];
		else							 result += e.passed_pawn[side][sq];
	}

	/**************************************************************************
	*  Evaluate weak pawns, increasing the penalty if they are situated       *
	*  on a half-open file                                                    *
	**************************************************************************/

	if (flagIsWeak) {
		result += e.weak_pawn[side][sq];
		if (!flagIsOpposed)
			result -= 4;
	}

	return result;
}

bool isPawnSupported(SQ sq, S8 side)
{
	int step = side == WHITE ? SOUTH : NORTH;
	if (IS_SQ(sq + WEST) && isPiece(side, PAWN, sq + WEST)) return 1;
	if (IS_SQ(sq + EAST) && isPiece(side, PAWN, sq + EAST)) return 1;
	if (IS_SQ(sq + step + WEST) && isPiece(side, PAWN, sq + step + WEST)) return 1;
	if (IS_SQ(sq + step + EAST) && isPiece(side, PAWN, sq + step + EAST)) return 1;
	return 0;
}

/******************************************************************************
*                             Pattern detection                               *
******************************************************************************/
void blockedPieces(int side)
{
	unsigned char oppo = side ^ 1;
	// central pawn blocked, bishop hard to develop
	if (isPiece(side, BISHOP, REL_SQ(side, C1))
		&& isPiece(side, PAWN, REL_SQ(side, D2))
		&& board.color[REL_SQ(side, D3)] != COLOR_EMPTY)
		v.blockages[side] -= e.P_BLOCK_CENTRAL_PAWN;

	if (isPiece(side, BISHOP, REL_SQ(side, F1))
		&& isPiece(side, PAWN, REL_SQ(side, E2))
		&& board.color[REL_SQ(side, E3)] != COLOR_EMPTY)
		v.blockages[side] -= e.P_BLOCK_CENTRAL_PAWN;

	// trapped knight
	if (isPiece(side, KNIGHT, REL_SQ(side, A8))
		&& (isPiece(oppo, PAWN, REL_SQ(side, A7)) || isPiece(oppo, PAWN, REL_SQ(side, C7))))
		v.blockages[side] -= e.P_KNIGHT_TRAPPED_A8;

	if (isPiece(side, KNIGHT, REL_SQ(side, H8))
		&& (isPiece(oppo, PAWN, REL_SQ(side, H7)) || isPiece(oppo, PAWN, REL_SQ(side, F7))))
		v.blockages[side] -= e.P_KNIGHT_TRAPPED_A8;

	if (isPiece(side, KNIGHT, REL_SQ(side, A7))
		&& isPiece(oppo, PAWN, REL_SQ(side, A6))
		&& isPiece(oppo, PAWN, REL_SQ(side, B7)))
		v.blockages[side] -= e.P_KNIGHT_TRAPPED_A7;

	if (isPiece(side, KNIGHT, REL_SQ(side, H7))
		&& isPiece(oppo, PAWN, REL_SQ(side, H6))
		&& isPiece(oppo, PAWN, REL_SQ(side, G7)))
		v.blockages[side] -= e.P_KNIGHT_TRAPPED_A7;

	// knight blocking queenside pawns
	if (isPiece(side, KNIGHT, REL_SQ(side, C3))
		&& isPiece(side, PAWN, REL_SQ(side, C2))
		&& isPiece(side, PAWN, REL_SQ(side, D4))
		&& !isPiece(side, PAWN, REL_SQ(side, E4)))
		v.blockages[side] -= e.P_C3_KNIGHT;

	// trapped bishop
	if (isPiece(side, BISHOP, REL_SQ(side, A7))
		&& isPiece(oppo, PAWN, REL_SQ(side, B6)))
		v.blockages[side] -= e.P_BISHOP_TRAPPED_A7;

	if (isPiece(side, BISHOP, REL_SQ(side, H7))
		&& isPiece(oppo, PAWN, REL_SQ(side, G6)))
		v.blockages[side] -= e.P_BISHOP_TRAPPED_A7;

	if (isPiece(side, BISHOP, REL_SQ(side, B8))
		&& isPiece(oppo, PAWN, REL_SQ(side, C7)))
		v.blockages[side] -= e.P_BISHOP_TRAPPED_A7;

	if (isPiece(side, BISHOP, REL_SQ(side, G8))
		&& isPiece(oppo, PAWN, REL_SQ(side, F7)))
		v.blockages[side] -= e.P_BISHOP_TRAPPED_A7;

	if (isPiece(side, BISHOP, REL_SQ(side, A6))
		&& isPiece(oppo, PAWN, REL_SQ(side, B5)))
		v.blockages[side] -= e.P_BISHOP_TRAPPED_A6;

	if (isPiece(side, BISHOP, REL_SQ(side, H6))
		&& isPiece(oppo, PAWN, REL_SQ(side, G5)))
		v.blockages[side] -= e.P_BISHOP_TRAPPED_A6;

	// bishop on initial sqare supporting castled king
	if (isPiece(side, BISHOP, REL_SQ(side, F1))
		&& isPiece(side, KING, REL_SQ(side, G1)))
		v.positionalThemes[side] += e.RETURNING_BISHOP;

	if (isPiece(side, BISHOP, REL_SQ(side, C1))
		&& isPiece(side, KING, REL_SQ(side, B1)))
		v.positionalThemes[side] += e.RETURNING_BISHOP;

	// uncastled king blocking own rook
	if ((isPiece(side, KING, REL_SQ(side, F1)) || isPiece(side, KING, REL_SQ(side, G1)))
		&& (isPiece(side, ROOK, REL_SQ(side, H1)) || isPiece(side, ROOK, REL_SQ(side, G1))))
		v.blockages[side] -= e.P_KING_BLOCKS_ROOK;

	if ((isPiece(side, KING, REL_SQ(side, C1)) || isPiece(side, KING, REL_SQ(side, B1)))
		&& (isPiece(side, ROOK, REL_SQ(side, A1)) || isPiece(side, ROOK, REL_SQ(side, B1))))
		v.blockages[side] -= e.P_KING_BLOCKS_ROOK;
}

int isPiece(U8 color, U8 piece, SQ sq)
{
	return ((board.pieces[sq] == piece) && (board.color[sq] == color));
}

/******************************************************************************
*                             Printing eval results                           *
******************************************************************************/
void printEval() {
	PrintBoard();
	printf("------------------------------------------\n");
	printf("Total value (for side to move): %d \n", eval(-MATE, MATE, 0));
	printf("Material balance       : %d \n", board.piece_material[WHITE] + board.pawn_material[WHITE] - board.piece_material[BLACK] - board.pawn_material[BLACK]);
	printf("Material adjustement   : ");
	printEvalFactor(v.adjustMaterial[WHITE], v.adjustMaterial[BLACK]);
	printf("Mg Piece/square tables : ");
	printEvalFactor(board.pcsq_mg[WHITE], board.pcsq_mg[BLACK]);
	printf("Eg Piece/square tables : ");
	printEvalFactor(board.pcsq_eg[WHITE], board.pcsq_eg[BLACK]);
	printf("Mg Mobility            : ");
	printEvalFactor(v.mgMob[WHITE], v.mgMob[BLACK]);
	printf("Eg Mobility            : ");
	printEvalFactor(v.egMob[WHITE], v.egMob[BLACK]);
	printf("Mg Tropism             : ");
	printEvalFactor(v.mgTropism[WHITE], v.mgTropism[BLACK]);
	printf("Eg Tropism             : ");
	printEvalFactor(v.egTropism[WHITE], v.egTropism[BLACK]);
	printf("Pawn structure         : %d \n", evalPawnStructure());
	printf("Blockages              : ");
	printEvalFactor(v.blockages[WHITE], v.blockages[BLACK]);
	printf("Positional themes      : ");
	printEvalFactor(v.positionalThemes[WHITE], v.positionalThemes[BLACK]);
	printf("King Shield            : ");
	printEvalFactor(v.kingShield[WHITE], v.kingShield[BLACK]);
	printf("Tempo                  : ");
	if (board.stm == WHITE) printf("%d", e.TEMPO);
	else printf("%d", -e.TEMPO);
	printf("\n");
	printf("------------------------------------------\n");
}

void printEvalFactor(int wh, int bl)
{
	printf("white %4d, black %4d, total: %4d \n", wh, bl, wh - bl);
}

int getTropism(int sq1, int sq2)
{
	return 7 - (abs(ROW(sq1) - ROW(sq2)) + abs(COL(sq1) - COL(sq2)));
}

U8 bestmove;          // move id passed between iterations for sorting purposes
s_Move move_to_make;  // move to be returned when search runs out of time
s_Move move_to_ponder;//last ponder move

unsigned int GetTimeMs() {
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	return (unsigned int)((((U64)ft.dwHighDateTime << 32) | ft.dwLowDateTime) / 10000);
}

bool isAttacked(char byColor, SQ sq) {

	/* pawns */
	if (byColor == WHITE && board.pawn_ctrl[WHITE][sq])
		return true;

	if (byColor == BLACK && board.pawn_ctrl[BLACK][sq])
		return true;

	/* knights */
	if (leaperAttack(byColor, sq, KNIGHT))
		return true;

	/* kings */
	if (leaperAttack(byColor, sq, KING))
		return true;

	/* straight line sliders */
	if (straightAttack(byColor, sq, NORTH)
		|| straightAttack(byColor, sq, SOUTH)
		|| straightAttack(byColor, sq, EAST)
		|| straightAttack(byColor, sq, WEST))
		return true;

	/* diagonal sliders */
	if (diagAttack(byColor, sq, NE)
		|| diagAttack(byColor, sq, SE)
		|| diagAttack(byColor, sq, NW)
		|| diagAttack(byColor, sq, SW))
		return true;
	return false;
}

bool leaperAttack(char byColor, SQ sq, char byPiece)
{
	S8 nextSq;
	for (int dir = 0; dir < 8; dir++)
	{
		nextSq = sq + vector[byPiece][dir];
		if (IS_SQ(nextSq) && isPiece(byColor, byPiece, nextSq))
			return true;
	}
	return false;
}

bool straightAttack(char byColor, SQ sq, int vect)
{
	int nextSq = sq + vect;
	while (IS_SQ(nextSq))
	{
		if (board.color[nextSq] != COLOR_EMPTY)
			return (board.color[nextSq] == byColor) && (board.pieces[nextSq] == ROOK || board.pieces[nextSq] == QUEEN);
		nextSq = nextSq + vect;
	}
	return false;
}

bool diagAttack(int byColor, SQ sq, int vect)
{
	int nextSq = sq + vect;
	while (IS_SQ(nextSq))
	{
		if (board.color[nextSq] != COLOR_EMPTY)
			return (board.color[nextSq] == byColor) && (board.pieces[nextSq] == BISHOP || board.pieces[nextSq] == QUEEN);
		nextSq = nextSq + vect;
	}
	return false;
}

bool bishAttack(int byColor, SQ sq, int vect)
{
	int nextSq = sq + vect;
	while (IS_SQ(nextSq))
	{
		if (board.color[nextSq] != COLOR_EMPTY)
		{
			if (board.color[nextSq] == byColor
				&& board.pieces[nextSq] == BISHOP)
				return true;
			return false;
		}
		nextSq = nextSq + vect;
	}
	return false;
}

//retrieving pv from hash table
void GetPv(char* pv) {
	s_Board rootb = board;
	char best;
	s_Move moves[256];
	int mcount = 0;
	move_to_ponder = {};
	for (U8 depth = 1; depth <= info.depthLimit; depth++) {
		best = -1;
		if (depth == 1)
			best = bestmove;
		else
			tt_probe(0, 0, 0, &best);
		if (best == -1)
			break;
		mcount = movegen(moves, 0xFF);
		for (int i = 0; i < mcount; i++) {
			if (moves[i].id == best) {
				if (depth == 2)
					move_to_ponder = moves[i];
				move_make(moves[i]);
				pv = MoveToStr(moves[i], pv);
				pv[0] = ' ';
				pv++;
				break;
			}
		}
	}
	pv[0] = 0;
	board = rootb;
}

int SearchQuiesce(int alpha, int beta)
{
	if (CheckUp())
		return 0;

	/* get a "stand pat" score */
	int val = eval(alpha, beta, 1);
	int stand_pat = val;

	/* check if stand-pat score causes a beta cutoff */
	if (val >= beta)
		return beta;

	/* check if stand-pat score may become a new alpha */
	if (alpha < val)
		alpha = val;

	/**************************************************************************
	*  We have taken into account the stand pat score, and it didn't let us   *
	*  come to a definite conclusion about the position. So we have to search *
	**************************************************************************/

	s_Move movelist[256];
	U8 mcount = movegen_qs(movelist);

	for (U8 i = 0; i < mcount; i++) {

		movegen_sort(mcount, movelist, i);

		if (movelist[i].piece_cap == KING) return MATE;

		/**********************************************************************
		*  Delta cutoff - a move guarentees the score well below alpha, so    *
		*  there's no point in searching it. We don't use his heuristic in    *
		*  the endgame, because of the insufficient material issues.          *
		**********************************************************************/

		if ((stand_pat + e.PIECE_VALUE[movelist[i].piece_cap] + 200 < alpha)
			&& (board.piece_material[!board.stm] - e.PIECE_VALUE[movelist[i].piece_cap] > e.ENDGAME_MAT)
			&& (!move_isprom(movelist[i])))
			continue;

		/**********************************************************************
		*  badCapture() replaces a cutoff based on the Static Exchange Evalu- *
		*  ation, marking the place where it ought to be coded. Despite being *
		*  just a hack, it saves quite a few nodes.                           *
		**********************************************************************/

		if (badCapture(movelist[i])
			&& !move_canSimplify(movelist[i])
			&& !move_isprom(movelist[i]))
			continue;

		/**********************************************************************
		*  Cutoffs  misfired, we have to search the current move              *
		**********************************************************************/

		move_make(movelist[i]);
		val = -SearchQuiesce(-beta, -alpha);
		move_unmake(movelist[i]);

		if (info.stop)
			return 0;

		if (val > alpha) {
			if (val >= beta) return beta;
			alpha = val;
		}
	}
	return alpha;
}

bool badCapture(s_Move move) {

	/* captures by pawn do not lose material */
	if (move.piece_from == PAWN)
		return false;

	/* Captures "lower takes higher" (as well as BxN) are good by definition. */
	if (e.PIECE_VALUE[move.piece_cap] >= e.PIECE_VALUE[move.piece_from] - 50)
		return false;

	/**************************************************************************
	*   When the enemy piece is defended by a pawn, in the quiescence search  *
	*   we  will  accept rook takes minor, but not minor takes pawn. ( More   *
	*   exact  version  should accept B/N x P if (a) the pawn  is  the  sole  *
	*   defender and (b) there is more than one attacker.                     *
	**************************************************************************/

	if (board.pawn_ctrl[board.color[move.from] ^ 1][move.to]
		&& e.PIECE_VALUE[move.piece_cap] + 200 < e.PIECE_VALUE[move.piece_from])
		return true;

	if (e.PIECE_VALUE[move.piece_cap] + 500 < e.PIECE_VALUE[move.piece_from]) {
		if (leaperAttack(board.color[move.from] ^ 1, move.to, KNIGHT)) return true;
		if (bishAttack(board.color[move.from] ^ 1, move.to, NE)) return true;
		if (bishAttack(board.color[move.from] ^ 1, move.to, NW)) return true;
		if (bishAttack(board.color[move.from] ^ 1, move.to, SE)) return true;
		if (bishAttack(board.color[move.from] ^ 1, move.to, SW)) return true;
	}

	/* if a capture is not processed, it cannot be considered bad */
	return false;
}

/******************************************************************************
*  search_run() is the only function called outside search.cpp, so it acts as *
*  an interface. After some preparatory work it calls search_iterate();       *
******************************************************************************/

void search_run() {
	sd.myside = board.stm;
	ageHistoryTable();
	SearchIterate();
}

//calls SearchRoot() with increasing depth until allocated time is exhausted
void SearchIterate() {
	int move_count = move_countLegal();
	int val = SearchRoot(1, -MATE, MATE);
	for (int depth = 2; depth <= info.depthLimit; depth++) {
		val = SearchWiden(depth, val);
		if (info.stop)
			break;
		if (move_count == 1)
			break;
		if (info.timeLimit && GetTimeMs() - info.timeStart > info.timeLimit / 2)
			break;
	}
	PrintBest();
}

//this function deals with aspiration window
int SearchWiden(int depth, int val) {
	int temp = val,
		alpha = val - options.aspiration,
		beta = val + options.aspiration;
	temp = SearchRoot(depth, alpha, beta);
	if (temp <= alpha || temp >= beta)
		temp = SearchRoot(depth, -MATE, MATE);
	return temp;
}

int SearchRoot(U8 depth, int alpha, int beta) {
	s_Move movelist[256];
	int val = 0;
	int best = -MATE;

	U8 currmove_legal = 0;

	/* Check  extension is done also at  the  root */
	bool inCheck = isAttacked(board.stm ^ 1, board.king_loc[board.stm]);
	if (inCheck) ++depth;

	U8 mcount = movegen(movelist, bestmove);

	for (U8 i = 0; i < mcount; i++) {

		int cl = board.stm;
		movegen_sort(mcount, movelist, i);

		if (movelist[i].piece_cap == KING) {
			alpha = MATE;
			bestmove = movelist[i].id;
		}

		move_make(movelist[i]);

		// filter out illegal moves
		if (isAttacked(board.stm, board.king_loc[!board.stm])) {
			move_unmake(movelist[i]);
			continue;
		}
		sd.cutoff[cl][movelist[i].from][movelist[i].to] -= 1;
		currmove_legal++;

		/* the "if" clause introduces PVS at root */
		if (best == -MATE)
			val = -SearchAlpha(depth - 1, 0, -beta, -alpha, DO_NULL, IS_PV);
		else
		{
			val = -SearchAlpha(depth - 1, 0, -alpha - 1, -alpha, DO_NULL, NO_PV);
			if (val > alpha)
				val = -SearchAlpha(depth - 1, 0, -beta, -alpha, DO_NULL, IS_PV);
		}

		if (val > best) best = val;
		move_unmake(movelist[i]);

		if (info.stop)
			return 0;

		if (val > alpha) {

			bestmove = movelist[i].id;
			move_to_make = movelist[i];

			if (val > beta) {
				tt_save(depth, beta, TT_BETA, bestmove);
				PrintInfo(depth, beta);
				return beta;
			}

			alpha = val;
			tt_save(depth, alpha, TT_ALPHA, bestmove);

			PrintInfo(depth, val);
		}
	}
	tt_save(depth, alpha, TT_EXACT, bestmove);
	return alpha;
}

int SearchAlpha(U8 depth, U8 ply, int alpha, int beta, int can_null, int is_pv)
{
	int  val = -MATE;
	char bestmove;
	char tt_move_index = (char)-1;
	char tt_flag = TT_ALPHA;
	int  flagInCheck;
	int  raised_alpha = 0;
	int  f_prune = 0;
	int  reduction_depth = 0;
	int  moves_tried = 0;
	int  new_depth;
	int  mate_value = MATE - ply; // will be used in mate distance pruning
	s_Move movelist[256];         // move list
	s_Move move;                  // current move

	/**************************************************************************
	*  Probably later we will want to probe the transposition table. Here we  *
	*  tell  the  cpu to prepare for that event. This is just a minor  speed  *
	*  optimization and program would run fine without that.                  *
	**************************************************************************/

	_mm_prefetch((char*)&tt[board.hash & tt_size], _MM_HINT_NTA);



	/**************************************************************************
	* MATE DISTANCE PRUNING, a minor improvement that helps to shave off some *
	* some nodes when the checkmate is near. Basically it prevents looking    *
	* for checkmates taking longer than one we have already found. No Elo     *
	* gain expected, but it's a nice feature. Don't use it at the root,       *
	* since  this code  doesn't return a move, only a value.                  *
	**************************************************************************/
	if (alpha < -mate_value) alpha = -mate_value;
	if (beta > mate_value - 1) beta = mate_value - 1;
	if (alpha >= beta) return alpha;
	/**************************************************************************
	*  Are we in check? If so, extend. It also means that program will never  *
	*  never enter quiescence search while in check.                          *
	**************************************************************************/
	flagInCheck = (isAttacked(board.stm ^ 1, board.king_loc[board.stm]));
	if (flagInCheck)
		depth += 1;
	/**************************************************************************
	*  At leaf nodes we do quiescence search (captures only) to make sure     *
	*  that only relatively quiet positions with no hanging pieces will be    *
	*  evaluated.                                                             *
	**************************************************************************/
	if (depth < 1)
		return SearchQuiesce(alpha, beta);
	if (CheckUp())
		return 0;
	if (isRepetition())
		return Contempt();
	/**************************************************************************
	*  Read the transposition table. We may have already searched current     *
	*  position. If depth was sufficient, then we might use the score         *
	*  of that search. If not, hash move still is expected to be good         *
	*  and should be sorted first.                                            *
	*                                                                         *
	*  NOTE: current implementation is sub-standard, since tt_move is just    *
	*  an index showing move's location on a move list. We should be able     *
	*  to retrieve move without generating full move list instead.            *
	**************************************************************************/
	if ((val = tt_probe(depth, alpha, beta, &tt_move_index)) != INF) {
		// in pv nodes we return only in case of an exact hash hit
		if (!is_pv || (val > alpha && val < beta)) {

			/******************************************************************
			*  Here we must be careful about checkmate scoring. "Mate in n"   *
			*  returned by transposition table means "mate in n if we start   *
			*  counting n right now". Yet search always returns mate scores   *
			*  as distance from root, so we must convert to that metric.      *
			*  Other programs might hide similar code within tt_probe() and   *
			*  tt_save() functions.                                           *
			******************************************************************/

			if (abs(val) > MATE - 100) {
				if (val > 0) val -= ply;
				else         val += ply;
			}

			return val;
		}
	}
	/**************************************************************************
	* EVAL PRUNING / STATIC NULL MOVE                                         *
	**************************************************************************/
	if (depth < 3
		&& !is_pv
		&& !flagInCheck
		&& abs(beta - 1) > -MATE + 100)
	{
		int static_eval = eval(alpha, beta, 1);

		int eval_margin = 120 * depth;
		if (static_eval - eval_margin >= beta)
			return static_eval - eval_margin;
	}
	/**************************************************************************
	*  Here  we introduce  NULL MOVE PRUNING. It  means  allowing opponent    *
	*  to execute two moves in a row, i.e. capturing something and escaping   *
	*  a recapture. If this cannot  wreck our position, then it is so good    *
	*  that there's  no  point in searching further. The flag "can_null"      *
	*  ensures we don't do  two null moves in a row. Null move is not used    *
	*  in  the endgame because of the risk of zugzwang.                       *
	**************************************************************************/

	if (depth > 2
		&& can_null
		&& !is_pv
		&& eval(alpha, beta, 1) > beta
		&& board.piece_material[board.stm] > e.ENDGAME_MAT
		&& !flagInCheck)
	{
		char ep_old = board.ep;
		move_makeNull();

		/**********************************************************************
		*  We use so-called adaptative null move pruning. Size of reduction   *
		*  depends on remaining  depth.                                       *
		**********************************************************************/

		char R = 2;
		if (depth > 6) R = 3;

		val = -SearchAlpha(depth - R - 1, ply + 1, -beta, -beta + 1, NO_NULL, NO_PV);

		move_unmakeNull(ep_old);

		if (info.stop)
			return 0;
		if (val >= beta) return beta;
	}   // end of null move code

	/**************************************************************************
	*  RAZORING - if a node is close to the leaf and its static score is low, *
	*  we drop directly to the quiescence search.                             *
	**************************************************************************/

	if (!is_pv && !flagInCheck && tt_move_index == -1 && can_null && depth <= 3)
	{
		int threshold = alpha - 300 - (depth - 1) * 60;
		if (eval(alpha, beta, 1) < threshold) {
			val = SearchQuiesce(alpha, beta);
			if (val < threshold) return alpha;
		}
	} // end of razoring code

	/**************************************************************************
	*  Decide  if FUTILITY PRUNING  is  applicable. If we are not in check,   *
	*  not searching for a checkmate and eval is below (alpha - margin), it   *
	*  might  mean that searching non-tactical moves at low depths is futile  *
	*  so we set a flag allowing this pruning.                                *
	**************************************************************************/

	int fmargin[4] = { 0, 200, 300, 500 };

	if (depth <= 3
		&& !is_pv
		&& !flagInCheck
		&& abs(alpha) < 9000
		&& eval(alpha, beta, 1) + fmargin[depth] <= alpha)
		f_prune = 1;

	/**************************************************************************
	*  Generate moves, then place special cases higher on the list            *
	**************************************************************************/

	U8 mcount = movegen(movelist, tt_move_index);
	ReorderMoves(movelist, mcount, ply);
	bestmove = movelist[0].id;

	/**************************************************************************
	*  Loop through the move list, trying them one by one.                    *
	**************************************************************************/

	for (int i = 0; i < mcount; i++) {

		int cl = board.stm;
		movegen_sort(mcount, movelist, i); // pick the best of untried moves
		move = movelist[i];
		move_make(move);

		// filter out illegal moves
		if (isAttacked(board.stm, board.king_loc[!board.stm])) {
			move_unmake(move);
			continue;
		}
		/**********************************************************************
		*  When the futility pruning flag is set, prune moves which do not    *
		*  give  check and do not change material balance.  Some  programs    *
		*  prune insufficient captures as well, but that seems too risky.     *
		**********************************************************************/

		if (f_prune
			&& moves_tried
			&& !move_iscapt(move)
			&& !move_isprom(move)
			&& !isAttacked(board.stm ^ 1, board.king_loc[board.stm])) {
			move_unmake(move);
			continue;
		}

		sd.cutoff[cl][move.from][move.to] -= 1;
		moves_tried++;
		reduction_depth = 0;       // this move has not been reduced yet
		new_depth = depth - 1;     // decrease depth by one ply

		/**********************************************************************
		*  Late move reduction. Typically a cutoff occurs on trying one of    *
		*  the first moves. If it doesn't, we are probably in an all-node,    *
		*  which means that all moves will fail low. So we might as well      *
		*  spare some effort, searching to reduced depth. Of course this is   *
		*  not a foolproof method, but it works more often than not. Still,   *
		*  we  need to exclude certain moves from reduction, in  order  to    *
		*  filter out tactical moves that may cause a late cutoff.            *
		**********************************************************************/

		if (!is_pv
			&& new_depth > 3
			&& moves_tried > 3
			&& !isAttacked(board.stm ^ 1, board.king_loc[board.stm])
			&& !flagInCheck
			&& sd.cutoff[cl][move.from][move.to] < 50
			&& (move.from != sd.killers[0][ply].from || move.to != sd.killers[0][ply].to)
			&& (move.from != sd.killers[1][ply].from || move.to != sd.killers[1][ply].to)
			&& !move_iscapt(move)
			&& !move_isprom(move)) {

			/******************************************************************
			* Real programs tend to use more advanced formulas to calculate   *
			* reduction depth. Typically they calculate it from both remai-   *
			* ning depth and move count. Formula used here is very basic and  *
			* gives only a minimal improvement over uniform one ply reduction,*
			* and is included for the sake of completeness only.              *
			******************************************************************/

			sd.cutoff[cl][move.from][move.to] = 50;
			reduction_depth = 1;
			if (moves_tried > 6) reduction_depth += 1;
			new_depth -= reduction_depth;
		}
		/**********************************************************************
		*  The code below introduces principal variation search. It  means    *
		*  that once we are in a PV-node (indicated by IS_PV flag) and  we    *
		*  have  found a move that raises alpha, we assume that  the  rest    *
		*  of moves ought to be refuted. This is done  relatively  cheaply    *
		*  by using  a null-window search centered around alpha.  Only  if    *
		*  this search fails high, we are forced repeat it with full window.  *
		*                                                                     *
		*  Understanding the shorthand in the first two lines is a bit tricky *
		*  If alpha has not been raised, we might be either in a zero window  *
		*  (scout) node or in an open window (pv) node, entered after a scout *
		*  search failed high. In both cases, we need to search with the same *
		*  alpha, the same beta AND the same node type.                       *                                            *
		**********************************************************************/

		if (!raised_alpha)
			val = -SearchAlpha(new_depth, ply + 1, -beta, -alpha, DO_NULL, is_pv);
		else
		{
			val = -SearchAlpha(new_depth, ply + 1, -alpha - 1, -alpha, DO_NULL, NO_PV);
			if (val > alpha)
				val = -SearchAlpha(new_depth, ply + 1, -beta, -alpha, DO_NULL, IS_PV);
		}
		/**********************************************************************
		*  Sometimes reduced search brings us above alpha. This is unusual,   *
		*  since we expected reduced move to be bad in first place. It is     *
		*  not certain now, so let's search to the full, unreduced depth.     *
		**********************************************************************/

		if (reduction_depth && val > alpha)
		{
			new_depth += reduction_depth;
			reduction_depth = 0;
			if (!raised_alpha)
				val = -SearchAlpha(new_depth, ply + 1, -beta, -alpha, DO_NULL, is_pv);
			else
			{
				val = -SearchAlpha(new_depth, ply + 1, -alpha - 1, -alpha, DO_NULL, NO_PV);
				if (val > alpha)
					val = -SearchAlpha(new_depth, ply + 1, -beta, -alpha, DO_NULL, IS_PV);
			}
		}
		move_unmake(move);
		if (info.stop)
			return 0;
		/**********************************************************************
		*  We can improve over alpha, so we change the node value together    *
		*  with  the expected move. Also the raised_alpha flag, needed  to    *
		*  control PVS, is set. In case of a beta cuoff, when our position    *
		*  is  so good that the score will not be accepted one ply before,    *
		*  we return it immediately.                                          *
		**********************************************************************/
		if (val > alpha) {
			bestmove = movelist[i].id;
			sd.cutoff[cl][move.from][move.to] += 6;
			if (val >= beta) {
				/**************************************************************
				*  On a quiet move update killer moves and history table      *
				*  in order to enhance move ordering.                         *
				**************************************************************/

				if (!move_iscapt(move)
					&& !move_isprom(move)) {
					setKillers(movelist[i], ply);
					sd.history[board.stm][move.from][move.to] += depth * depth;

					/**********************************************************
					*  With super deep search history table would overflow    *
					*  - let's prevent it.                                    *
					**********************************************************/

					if (sd.history[board.stm][move.from][move.to] > SORT_KILL) {
						for (int cl = 0; cl < 2; cl++)
							for (int a = 0; a < 128; a++)
								for (int b = 0; b < 128; b++) {
									sd.history[cl][a][b] = sd.history[cl][a][b] / 2;
								}
					}
				}
				tt_flag = TT_BETA;
				alpha = beta;
				break; // no need to search any further
			}
			raised_alpha = 1;
			tt_flag = TT_EXACT;
			alpha = val;
		} // changing the node value is finished
	}   // end of looping through the moves

	/**************************************************************************
	*  Checkmate and stalemate detection: if we can't find a legal move in    *
	*  the current position, we test if we are in check. If so, mate score    *
	*  relative to search depth is returned. If not, we use draw score pro-   *
	*  vided by contempt() function.                                          *
	**************************************************************************/
	if (!moves_tried)
	{
		bestmove = -1;
		if (flagInCheck)
			alpha = -MATE + ply;
		else
			alpha = Contempt();
	}
	/* tt_save() does not save anything when the search is timed out */
	tt_save(depth, alpha, tt_flag, bestmove);
	return alpha;
}

void setKillers(s_Move m, U8 ply)
{
	/* if a move isn't a capture, save it as a killer move */
	if (m.piece_cap == PIECE_EMPTY) {

		/* make sure killer moves will be different
		before saving secondary killer move */
		if (m.from != sd.killers[ply][0].from
			|| m.to != sd.killers[ply][0].to)
			sd.killers[ply][1] = sd.killers[ply][0];

		/* save primary killer move */
		sd.killers[ply][0] = m;
	}
}

void ReorderMoves(s_Move* m, U8 mcount, U8 ply) {

	for (int j = 0; j < mcount; j++) {
		if ((m[j].from == sd.killers[ply][1].from)
			&& (m[j].to == sd.killers[ply][1].to)
			&& (m[j].score < SORT_KILL - 1)) {
			m[j].score = SORT_KILL - 1;
		}

		if ((m[j].from == sd.killers[ply][0].from)
			&& (m[j].to == sd.killers[ply][0].to)
			&& (m[j].score < SORT_KILL)) {
			m[j].score = SORT_KILL;
		}
	}
}

void PrintInfo(int depth, int val) {
	sd.score = val;
	char score[10];
	std::fill(std::begin(sd.pv), std::end(sd.pv), 0);
	if (abs(val) < MATE - 2000)
		sprintf(score, "cp %d", val);
	else if (val > 0)
		sprintf(score, "mate %d", (MATE - val) / 2 + 1);
	else
		sprintf(score, "mate %d", -(MATE + val) / 2 - 1);
	U32 time = GetTimeMs() - info.timeStart;
	GetPv(sd.pv);
	if (info.post)
		printf("info depth %u score %s time %u nodes %u hashfull %llu pv %s\n", depth, score, time, info.nodes, ttPermill(), sd.pv);
}

/******************************************************************************
*  Checking if the current position has been already encountered on the cur-  *
*  rent search path. Function does NOT check the number of repetitions.       *
******************************************************************************/
bool isRepetition()
{
	for (int i = 0; i < board.rep_index; i++)
		if (board.rep_stack[i] == board.hash)
			return true;
	return false;
}

/******************************************************************************
*  Clearing the history table is needed at the beginning of a search starting *
*  from a new position, like at the beginning of a new game.                  *
******************************************************************************/
void clearHistoryTable()
{
	for (int cl = 0; cl < 2; cl++)
		for (int i = 0; i < 128; i++)
			for (int j = 0; j < 128; j++)
			{
				sd.history[cl][i][j] = 0;
				sd.cutoff[cl][i][j] = 100;
			}
}

/******************************************************************************
* ageHistoryTable() is run between searches to decrease the history values    *
* used for move sorting. This  causes obsolete information to disappear gra-  *
* dually. Clearing the table was worse for the move ordering.                 *
******************************************************************************/

void ageHistoryTable()
{
	for (int cl = 0; cl < 2; cl++)
		for (int i = 0; i < 128; i++)
			for (int j = 0; j < 128; j++)
			{
				sd.history[cl][i][j] = sd.history[cl][i][j] / 8;
				sd.cutoff[cl][i][j] = 100;
			}
}

/******************************************************************************
*  contempt() returns a draw value (which may be non-zero) relative to the    *
*  side to move and to the  game  stage. This  way  we may make our program   *
*  play for a  draw  or strive to avoid it.                                   *
******************************************************************************/
int Contempt() {
	int value = board.piece_material[sd.myside] < e.ENDGAME_MAT ? 0 : options.contempt;
	if (board.stm == sd.myside)
		return -value;
	else
		return value;
}

static bool InputAvailable() {
	static HANDLE hstdin = 0;
	static bool pipe = false;
	unsigned long dw = 0;
	if (!hstdin) {
		hstdin = GetStdHandle(STD_INPUT_HANDLE);
		pipe = !GetConsoleMode(hstdin, &dw);
		if (!pipe)
		{
			SetConsoleMode(hstdin, dw & ~(ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT));
			FlushConsoleInputBuffer(hstdin);
		}
		else
		{
			setvbuf(stdin, NULL, _IONBF, 0);
			setvbuf(stdout, NULL, _IONBF, 0);
		}
	}
	if (pipe)
		PeekNamedPipe(hstdin, 0, 0, 0, &dw, 0);
	else
		GetNumberOfConsoleInputEvents(hstdin, &dw);
	return dw > 1;
}

bool CheckUp() {
	if (!(++info.nodes & 0xffff)) {
		if (info.timeLimit && GetTimeMs() - info.timeStart > info.timeLimit)
			info.stop = true;
		if (info.nodesLimit && info.nodes > info.nodesLimit)
			info.stop = true;
		if (InputAvailable()) {
			string line;
			getline(cin, line);
			UciCommand((char*)line.c_str());
		}
	}
	return info.stop;
}

void PrintBest() {
	if (info.ponder || !info.post)
		return;
	char make[6]{};
	MoveToStr(move_to_make, make);
	if (options.ponder && (move_to_ponder.from != move_to_ponder.to)) {
		char ponder[6]{};
		MoveToStr(move_to_ponder, ponder);
		printf("bestmove %s ponder %s\n", make, ponder);
	}
	else
		printf("bestmove %s\n", make);
}

s_SearchDriver sd = {};
s_Options options = {};
s_SearchInfo info = {};
s_Board board = {};

szobrist zobrist;

stt_entry* tt;
spawntt_entry* ptt;
sevaltt_entry* ett;

U64 tt_size = 0;
int ptt_size = 0;
int ett_size = 0;
U64 tt_used = 0;


U8 movecount;

s_Move* m;

bool slide[5] = { 0, 1, 1, 1, 0 };
char vectors[5] = { 8, 8, 4, 4, 8 };
char vector[5][8] = {
	{ SW, SOUTH, SE, WEST, EAST, NW, NORTH, NE },
	{ SW, SOUTH, SE, WEST, EAST, NW, NORTH, NE },
	{ SOUTH, WEST, EAST, NORTH                 },
	{ SW, SE, NW, NE                           },
	{ -33, -31, -18, -14, 14, 18, 31, 33       }
};

int move_makeNull() {
	board.stm ^= 1;
	board.hash ^= zobrist.color;
	board.ply++;
	if (board.ep != -1) {
		board.hash ^= zobrist.ep[board.ep];
		board.ep = -1;
	}
	return 0;
}

int move_unmakeNull(char ep) {
	board.stm ^= 1;
	board.hash ^= zobrist.color;
	board.ply--;
	if (ep != -1) {
		board.hash ^= zobrist.ep[ep];
		board.ep = ep;
	}
	return 0;
}

int move_make(s_Move move) {

	/* switch the side to move */
	board.stm ^= 1;
	board.hash ^= zobrist.color;

	/* a capture or a pawn move clears b.ply */
	board.ply++;
	if ((move.piece_from == PAWN) || move_iscapt(move))
		board.ply = 0;

	/* in case of a capture, the "to" square must be cleared,
	   else incrementally updated stuff gets blown up */
	if (board.pieces[move.to] != PIECE_EMPTY)
		ClearSq(move.to);

	/* a piece vacates its initial square */
	ClearSq(move.from);

	/* a piece arrives to its destination square */
	FillSq(board.stm ^ 1, move.piece_to, move.to);

	/**************************************************************************
	*  Reset the castle flags. If either a king or a rook leaves its initial  *
	*  square, the side looses the castling rights. The same happens when     *
	*  a rook on its initial square gets captured.                            *
	**************************************************************************/

	switch (move.from) {
	case H1:
		board.castle &= ~CASTLE_WK;
		break;
	case E1:
		board.castle &= ~(CASTLE_WK | CASTLE_WQ);
		break;
	case A1:
		board.castle &= ~CASTLE_WQ;
		break;
	case H8:
		board.castle &= ~CASTLE_BK;
		break;
	case E8:
		board.castle &= ~(CASTLE_BK | CASTLE_BQ);
		break;
	case A8:
		board.castle &= ~CASTLE_BQ;
		break;
	}
	switch (move.to) {
	case H1:
		board.castle &= ~CASTLE_WK;
		break;
	case E1:
		board.castle &= ~(CASTLE_WK | CASTLE_WQ);
		break;
	case A1:
		board.castle &= ~CASTLE_WQ;
		break;
	case H8:
		board.castle &= ~CASTLE_BK;
		break;
	case E8:
		board.castle &= ~(CASTLE_BK | CASTLE_BQ);
		break;
	case A8:
		board.castle &= ~CASTLE_BQ;
		break;
	}
	board.hash ^= zobrist.castling[move.castle];
	board.hash ^= zobrist.castling[board.castle];

	/**************************************************************************
	*   Finish the castling move. It is represented as the king move (e1g1    *
	*   = White castles short), which has already been executed above. Now    *
	*   we must move the rook to complete castling.                           *
	**************************************************************************/

	if (move.flags & MFLAG_CASTLE) {
		if (move.to == G1) {
			ClearSq(H1);
			FillSq(WHITE, ROOK, F1);
		}
		else if (move.to == C1) {
			ClearSq(A1);
			FillSq(WHITE, ROOK, D1);
		}
		else if (move.to == G8) {
			ClearSq(H8);
			FillSq(BLACK, ROOK, F8);
		}
		else if (move.to == C8) {
			ClearSq(A8);
			FillSq(BLACK, ROOK, D8);
		}
	}

	/**************************************************************************
	*  Erase the current state of the ep-flag, then set it again if a pawn    *
	*  jump that allows such capture has been made. 1.e4 in the initial po-   *
	*  sition will not set the en passant flag, because there are no black    *
	*  pawns on d4 and f4. This soluion helps with opening book and increa-   *
	*  ses the number of transposition table hits.                            *
	**************************************************************************/

	if (board.ep != -1) {
		board.hash ^= zobrist.ep[board.ep];
		board.ep = -1;
	}
	if ((move.piece_from == PAWN) && (abs(move.from - move.to) == 32)
		&& (board.pawn_ctrl[board.stm][(move.from + move.to) / 2])
		) {
		board.ep = (move.from + move.to) / 2;
		board.hash ^= zobrist.ep[board.ep];
	}

	/**************************************************************************
	*  Remove a pawn captured en passant                                      *
	**************************************************************************/

	if (move.flags & MFLAG_EPCAPTURE) {
		if (!board.stm == WHITE) {
			ClearSq(move.to - 16);
		}
		else {
			ClearSq(move.to + 16);
		}
	}

	++board.rep_index;
	board.rep_stack[board.rep_index] = board.hash;

	return 0;
}

int move_unmake(s_Move move) {

	board.stm ^= 1;
	board.hash ^= zobrist.color;

	board.ply = move.ply;

	/* set en passant square */
	if (board.ep != -1)
		board.hash ^= zobrist.ep[board.ep];
	if (move.ep != -1)
		board.hash ^= zobrist.ep[move.ep];
	board.ep = move.ep;

	/* Move the piece back */
	ClearSq(move.to);
	FillSq(board.stm, move.piece_from, move.from);

	/* Un-capture: in case of a capture, put the captured piece back */
	if (move_iscapt(move))
		FillSq((char)!board.stm, move.piece_cap, move.to);

	/* Un-castle: the king has already been moved, now move the rook */
	if (move.flags & MFLAG_CASTLE) {
		if (move.to == G1) {
			ClearSq(F1);
			FillSq(WHITE, ROOK, H1);
		}
		else if (move.to == C1) {
			ClearSq(D1);
			FillSq(WHITE, ROOK, A1);
		}
		else if (move.to == G8) {
			ClearSq(F8);
			FillSq(BLACK, ROOK, H8);
		}
		else if (move.to == C8) {
			ClearSq(D8);
			FillSq(BLACK, ROOK, A8);
		}
	}

	/* adjust castling flags */
	board.hash ^= zobrist.castling[move.castle];
	board.hash ^= zobrist.castling[board.castle];
	board.castle = move.castle;

	/* Put the pawn captured en passant back to its initial square */
	if (move.flags & MFLAG_EPCAPTURE) {
		if (board.stm == WHITE) {
			FillSq(BLACK, PAWN, move.to - 16);
		}
		else {
			FillSq(WHITE, PAWN, move.to + 16);
		}
	}

	--board.rep_index;

	return 0;
}

int move_iscapt(s_Move m) {
	return (m.piece_cap != PIECE_EMPTY);
}

int move_isprom(s_Move m) {
	return (m.piece_from != m.piece_to);
}

int move_canSimplify(s_Move m) {
	if (m.piece_cap == PAWN
		|| board.piece_material[!board.stm] - e.PIECE_VALUE[m.piece_cap] > e.ENDGAME_MAT)
		return 0;
	else
		return 1;
}

int move_countLegal() {
	s_Move mlist[256];
	int mcount = movegen(mlist, 0xFF);
	int result = 0;

	for (int i = 0; i < mcount; i++) {

		/* try a move... */
		move_make(mlist[i]);

		/* ...then increase the counter if it did not leave us in check */
		if (!isAttacked(board.stm, board.king_loc[!board.stm])) ++result;

		move_unmake(mlist[i]);
	}

	/* return number of legal moves in the current position */
	return result;
}

bool move_isLegal(s_Move m)
{
	s_Move movelist[256] = {};
	int movecount = movegen(movelist, 0xFF);
	for (int i = 0; i < movecount; i++)
		if (movelist[i].from == m.from && movelist[i].to == m.to)
		{
			bool result = true;
			move_make(movelist[i]);
			if (isAttacked(board.stm, board.king_loc[!board.stm]))
				result = false;
			move_unmake(movelist[i]);
			return result;
		}
	return false;
}

/******************************************************************************
*  This is not yet proper static exchange evaluation, but an approximation    *
*  proposed by Harm Geert Mueller under the acronym BLIND (better, or lower   *
*  if not defended. As the name indicates, it detects only obviously good     *
*  captures, but it seems enough to improve move ordering.                    *
******************************************************************************/

bool Blind(s_Move move)
{
	int sq_to = move.to;
	int sq_fr = move.from;
	int pc_fr = board.pieces[sq_fr];
	int pc_to = board.pieces[sq_to];
	int val = e.SORT_VALUE[pc_to];
	/* captures by pawn do not lose material */
	if (pc_fr == PAWN)
		return true;
	/* Captures "lower takes higher" (as well as BxN) are good by definition. */
	if (e.SORT_VALUE[pc_to] >= e.SORT_VALUE[pc_fr] - 50)
		return true;
	/* Make the first capture, so that X-ray defender show up*/
	ClearSq(sq_fr);
	/* Captures of undefended pieces are good by definition */
	if (!isAttacked(board.stm ^ 1, sq_to))
	{
		FillSq(board.stm, pc_fr, sq_fr);
		return true;
	}
	FillSq(board.stm, pc_fr, sq_fr);
	return false; // of other captures we know nothing, Jon Snow!
}

//returns movecount
U8 movegen(s_Move* moves, U8 tt_move)
{
	m = moves;
	movecount = 0;
	//Castling
	if (board.stm == WHITE) {
		if (board.castle & CASTLE_WK) {
			if ((board.pieces[F1] == PIECE_EMPTY)
				&& (board.pieces[G1] == PIECE_EMPTY)
				&& (!isAttacked((char)!board.stm, E1))
				&& (!isAttacked((char)!board.stm, F1))
				&& (!isAttacked((char)!board.stm, G1)))
				movegen_push(E1, G1, KING, PIECE_EMPTY, MFLAG_CASTLE);
		}
		if (board.castle & CASTLE_WQ) {
			if ((board.pieces[B1] == PIECE_EMPTY)
				&& (board.pieces[C1] == PIECE_EMPTY)
				&& (board.pieces[D1] == PIECE_EMPTY)
				&& (!isAttacked((char)!board.stm, E1))
				&& (!isAttacked((char)!board.stm, D1))
				&& (!isAttacked((char)!board.stm, C1)))
				movegen_push(E1, C1, KING, PIECE_EMPTY, MFLAG_CASTLE);
		}
	}
	else {
		if (board.castle & CASTLE_BK) {
			if ((board.pieces[F8] == PIECE_EMPTY)
				&& (board.pieces[G8] == PIECE_EMPTY)
				&& (!isAttacked((char)!board.stm, E8))
				&& (!isAttacked((char)!board.stm, F8))
				&& (!isAttacked((char)!board.stm, G8)))
				movegen_push(E8, G8, KING, PIECE_EMPTY, MFLAG_CASTLE);
		}
		if (board.castle & CASTLE_BQ) {
			if ((board.pieces[B8] == PIECE_EMPTY)
				&& (board.pieces[C8] == PIECE_EMPTY)
				&& (board.pieces[D8] == PIECE_EMPTY)
				&& (!isAttacked((char)!board.stm, E8))
				&& (!isAttacked((char)!board.stm, D8))
				&& (!isAttacked((char)!board.stm, C8)))
				movegen_push(E8, C8, KING, PIECE_EMPTY, MFLAG_CASTLE);
		}
	}
	for (S8 sq = 0; sq < 120; sq++)
	{
		if (board.color[sq] == board.stm)
		{
			if (board.pieces[sq] == PAWN)
			{
				movegen_pawn_move(sq, 0);
				movegen_pawn_capt(sq);
			}
			else
			{
				assert(board.pieces[sq] < (sizeof vectors / sizeof vectors[0]) && board.pieces[sq] >= 0);
				for (char dir = 0; dir < vectors[board.pieces[sq]]; dir++)
				{
					for (char pos = sq;;)
					{
						pos = pos + vector[board.pieces[sq]][dir];
						if (!IS_SQ(pos))
							break;
						if (board.pieces[pos] == PIECE_EMPTY)
							movegen_push(sq, pos, board.pieces[sq], PIECE_EMPTY, MFLAG_NORMAL);
						else
						{
							if (board.color[pos] != board.stm)
								movegen_push(sq, pos, board.pieces[sq], board.pieces[pos], MFLAG_CAPTURE);
							break; // we're hitting a piece, so looping is over
						}
						if (!slide[board.pieces[sq]])
							break;
					}
				}
			}
		}
	}
	/* if we have a best-move fed into movegen(), then increase its score */
	if ((tt_move != -1) && (tt_move < movecount)) moves[tt_move].score = SORT_HASH;
	return movecount;
}

U8 movegen_qs(s_Move* moves)
{
	m = moves;
	movecount = 0;
	for (S8 sq = 0; sq < 120; sq++)
	{
		if (board.color[sq] == board.stm)
		{
			if (board.pieces[sq] == PAWN)
			{
				movegen_pawn_move(sq, 1);
				movegen_pawn_capt(sq);
			}
			else
			{
				assert(board.pieces[sq] < (sizeof vectors / sizeof vectors[0]) && board.pieces[sq] >= 0);
				for (char dir = 0; dir < vectors[board.pieces[sq]]; dir++)
				{
					for (char pos = sq;;)
					{
						pos = pos + vector[board.pieces[sq]][dir];
						if (!IS_SQ(pos))
							break;
						if (board.pieces[pos] != PIECE_EMPTY)
						{
							if (board.color[pos] != board.stm)
								movegen_push(sq, pos, board.pieces[sq], board.pieces[pos], MFLAG_CAPTURE);
							break; // we're hitting a piece, so looping is over
						}
						if (!slide[board.pieces[sq]]) break;
					}
				}
			}
		}
	}
	return movecount;
}


void movegen_pawn_move(SQ sq, bool promotion_only)
{
	if (board.stm == WHITE)
	{
		if (promotion_only && (ROW(sq) != ROW_7))
			return;
		if (board.pieces[sq + NORTH] == PIECE_EMPTY)
		{
			movegen_push(sq, sq + NORTH, PAWN, PIECE_EMPTY, MFLAG_NORMAL);
			if ((ROW(sq) == ROW_2) && (board.pieces[sq + NN] == PIECE_EMPTY))
				movegen_push(sq, sq + NN, PAWN, PIECE_EMPTY, MFLAG_EP);
		}
	}
	else
	{
		if (promotion_only && (ROW(sq) != ROW_2))
			return;
		if (board.pieces[sq + SOUTH] == PIECE_EMPTY)
		{
			movegen_push(sq, sq + SOUTH, PAWN, PIECE_EMPTY, MFLAG_NORMAL);
			if ((ROW(sq) == ROW_7) && (board.pieces[sq + SS] == PIECE_EMPTY))
				movegen_push(sq, sq + SS, PAWN, PIECE_EMPTY, MFLAG_EP);
		}
	}
}

void movegen_pawn_capt(SQ sq)
{
	if (board.stm == WHITE) {
		if (IS_SQ(sq + NW) && ((board.ep == sq + NW) || (board.color[sq + NW] == (board.stm ^ 1)))) {
			movegen_push(sq, sq + NW, PAWN, board.pieces[sq + NW], MFLAG_CAPTURE);
		}
		if (IS_SQ(sq + NE) && ((board.ep == sq + NE) || (board.color[sq + NE] == (board.stm ^ 1)))) {
			movegen_push(sq, sq + 17, PAWN, board.pieces[sq + NE], MFLAG_CAPTURE);
		}
	}
	else {
		if (IS_SQ(sq + SE) && ((board.ep == sq + SE) || (board.color[sq + SE] == (board.stm ^ 1)))) {
			movegen_push(sq, sq + SE, PAWN, board.pieces[sq + SE], MFLAG_CAPTURE);
		}
		if (IS_SQ(sq + SW) && ((board.ep == sq + SW) || (board.color[sq + SW] == (board.stm ^ 1)))) {
			movegen_push(sq, sq + SW, PAWN, board.pieces[sq + SW], MFLAG_CAPTURE);
		}
	}
}

void movegen_push(char from, char to, U8 piece_from, U8 piece_cap, char flags)
{
	m[movecount].from = from;
	m[movecount].to = to;
	m[movecount].piece_from = piece_from;
	m[movecount].piece_to = piece_from;
	m[movecount].piece_cap = piece_cap;
	m[movecount].flags = flags;
	m[movecount].ply = board.ply;
	m[movecount].castle = board.castle;
	m[movecount].ep = board.ep;
	m[movecount].id = movecount;

	/**************************************************************************
	* Quiet moves are sorted by history score.                                *
	**************************************************************************/

	m[movecount].score = sd.history[board.stm][from][to];

	/**************************************************************************
	* Score for captures: add the value of the captured piece and the id      *
	* of the attacking piece. If two pieces attack the same target, the one   *
	* with the higher id (eg. Pawn=5) gets searched first. En passant gets    *
	* the same score as pawn takes pawn. Good captures are put at the front   *
	* of the list, bad captures - after ordinary moves.                       *
	**************************************************************************/

	if (piece_cap != PIECE_EMPTY) {
		if (Blind(m[movecount]) == 0) m[movecount].score = e.SORT_VALUE[piece_cap] + piece_from;
		else                          m[movecount].score = SORT_CAPT + e.SORT_VALUE[piece_cap] + piece_from;
	}

	if ((piece_from == PAWN) && (to == board.ep)) {
		m[movecount].score = SORT_CAPT + e.SORT_VALUE[PAWN] + 5;
		m[movecount].flags = MFLAG_EPCAPTURE;
	}

	/**************************************************************************
	* Put all four possible promotion moves on the list and score them.       *
	**************************************************************************/
	if ((piece_from == PAWN) && ((ROW(to) == ROW_1) || (ROW(to) == ROW_8)))
	{
		m[movecount].flags |= MFLAG_PROMOTION;
		for (char prompiece = QUEEN; prompiece <= KNIGHT; prompiece++)
		{
			m[movecount + prompiece - 1] = m[movecount];
			m[movecount + prompiece - 1].piece_to = prompiece;
			m[movecount + prompiece - 1].score += SORT_PROM + e.SORT_VALUE[prompiece];
			m[movecount + prompiece - 1].id = movecount + prompiece - 1;
		}
		movecount += 3;
	}
	movecount++;
}

void movegen_sort(U8 movecount, s_Move* m, U8 current)
{
	//find the move with the highest score - hoping for an early cutoff
	int high = current;
	for (int i = current + 1; i < movecount; i++)
		if (m[i].score > m[high].score)
			high = i;
	s_Move temp = m[high];
	m[high] = m[current];
	m[current] = temp;
}

U64 rand64() {
	static U64 next = 1;

	next = next * 1103515245 + 12345;
	return next;
}

int tt_init() {

	/* fill the zobrist struct with random numbers */

	for (int pnr = 0; pnr <= 5; pnr++) {
		for (int cnr = 0; cnr <= 1; cnr++) {
			for (int snr = 0; snr <= 127; snr++) {
				zobrist.piecesquare[pnr][cnr][snr] = rand64();
			}
		}
	}

	zobrist.color = rand64();

	for (int castling = 0; castling <= 15; castling++) {
		zobrist.castling[castling] = rand64();
	}

	for (int ep = 0; ep <= 127; ep++) {
		zobrist.ep[ep] = rand64();
	}

	return 0;
}

int tt_setsize(int size) {

	/**************************************************************************
	*  Check if size is a power of 2. If not, make it the next lower power    *
	*  of 2. This allows for a faster access of the entry needed:             *
	*  as sizeof(stt_entry) in our case is 16 Bytes long (see definition of   *
	*  stt_entry), we are creating size / 16 tt entries. The idea of making   *
	*  the size a power of 2 is important for accessing the table. By 'anding'*
	*  the hash value and the number of entries -1 (tt_size), we get a number *
	*  in the range between 0 and the number of entries very quickly. This    *
	*  number is used to index the entry.                                     *
	**************************************************************************/

	free(tt);

	if (size & (size - 1)) {

		size--;
		for (int i = 1; i < 32; i = i * 2)
			size |= size >> i;
		size++;
		size >>= 1;

	}

	if (size < 16) {
		tt_size = 0;
		return 0;
	}

	tt_size = (size / sizeof(stt_entry)) - 1;
	tt = (stt_entry*)calloc(tt_size + 1, sizeof(stt_entry));
	return 0;
}

int tt_probe(U8 depthLimit, int alpha, int beta, char* best) {

	if (!tt_size) return INF;

	/**************************************************************************
	*   Before  searching  a certain position, look whether we have  done  so *
	*   before. This is done by comparing the hashkey of the current position *
	*   to the hashkey of the specific tt entry. If they are the same, we may *
	*   use the move stored in the hash table to enhance move ordering.  When *
	*   the previous search was not shallower then the one needed now, we may *
	*   use  the  information present in the transposition table  to  replace *
	*   search altogether. We do it only if the value found is in the  proper *
	*   relation  to alpha and beta, i.e. when it would cause a cutoff.  Some *
	*	programs  do  use these informations to narrow the window,  but  then *
	*   you have to be extra careful to avoid search instability.             *
	**************************************************************************/

	stt_entry* phashe = &tt[board.hash & tt_size];

	if (phashe->hash == board.hash) {

		/***************************************************
		*   The  position  matches, so  we  may  retrieve  *
		*   a move that will be used for sorting purposes  *
		***************************************************/

		*best = phashe->bestmove;

		/***************************************************
		*   Now test if we can retrieve position value     *
		*  ( saved depth greater than current depth )      *
		***************************************************/

		if (phashe->depthLimit >= depthLimit) {

			if (phashe->flags == TT_EXACT)
				return phashe->val;

			if ((phashe->flags == TT_ALPHA) && (phashe->val <= alpha))
				return alpha;

			if ((phashe->flags == TT_BETA) && (phashe->val >= beta))
				return beta;

		}

	}

	return INF;

}

void tt_save(U8 depthLimit, int val, char flags, char best) {

	if (!tt_size)
		return;
	if (info.stop)
		return;

	stt_entry* phashe = &tt[board.hash & tt_size];

	if ((phashe->hash == board.hash) && (phashe->depthLimit > depthLimit)) return;
	if (!phashe->hash)
		tt_used++;
	phashe->hash = board.hash;
	phashe->val = val;
	phashe->flags = flags;
	phashe->depthLimit = depthLimit;
	phashe->bestmove = best;
}

int ttpawn_setsize(int size) {

	/* see tt_setsize for more details */

	free(ptt);

	if (size & (size - 1)) {

		size--;
		for (int i = 1; i < 32; i = i * 2)
			size |= size >> i;
		size++;
		size >>= 1;

	}

	if (size < 8) {
		ptt_size = 0;
		return 0;
	}

	ptt_size = (size / sizeof(spawntt_entry)) - 1;
	ptt = (spawntt_entry*)malloc(size);

	return 0;
}

int ttpawn_probe() {

	if (!ptt_size) return INF;

	spawntt_entry* phashe = &ptt[board.phash & ptt_size];

	if (phashe->hash == board.phash) return phashe->val;

	return INF;

}

void ttpawn_save(int val) {

	if (!ptt_size) return;

	spawntt_entry* phashe = &ptt[board.phash & ptt_size];

	phashe->hash = board.phash;
	phashe->val = val;
}

int tteval_setsize(int size)
{
	tt_used = 0;
	free(ett);
	if (size & (size - 1))
	{
		size--;
		for (int i = 1; i < 32; i = i * 2)
			size |= size >> i;
		size++;
		size >>= 1;
	}
	if (size < 16)
	{
		ett_size = 0;
		return 0;
	}
	ett_size = (size / sizeof(sevaltt_entry)) - 1;
	ett = (sevaltt_entry*)malloc(size);
	return 0;
}

int tteval_probe() {
	if (!ett_size)
		return INF;
	sevaltt_entry* phashe = &ett[board.hash & ett_size];
	if (phashe->hash == board.hash)
		return phashe->val;
	return INF;

}

void tteval_save(int val) {

	if (!ett_size) return;

	sevaltt_entry* phashe = &ett[board.hash & ett_size];

	phashe->hash = board.hash;
	phashe->val = val;
}

U64 ttPermill() {
	return (tt_used * 1000ull) / tt_size;
}

void clearBoard()
{
	for (int sq = 0; sq < 128; sq++)
	{
		board.pieces[sq] = PIECE_EMPTY;
		board.color[sq] = COLOR_EMPTY;
		for (int cl = 0; cl < 2; cl++) {
			board.pawn_ctrl[cl][sq] = 0;
		}
	}
	board.castle = 0;
	board.ep = -1;
	board.ply = 0;
	board.hash = 0;
	board.phash = 0;
	board.stm = 0;
	board.rep_index = 0;

	// reset perceived values

	board.piece_material[WHITE] = 0;
	board.piece_material[BLACK] = 0;
	board.pawn_material[WHITE] = 0;
	board.pawn_material[BLACK] = 0;
	board.pcsq_mg[WHITE] = 0;
	board.pcsq_mg[BLACK] = 0;
	board.pcsq_eg[WHITE] = 0;
	board.pcsq_eg[BLACK] = 0;


	// reset counters

	for (int i = 0; i < 6; i++) {
		board.piece_cnt[WHITE][i] = 0;
		board.piece_cnt[BLACK][i] = 0;
	}

	for (int i = 0; i < 8; i++) {
		board.pawns_on_file[WHITE][i] = 0;
		board.pawns_on_file[BLACK][i] = 0;
		board.pawns_on_rank[WHITE][i] = 0;
		board.pawns_on_rank[BLACK][i] = 0;
	}
}

/******************************************************************************
* fillSq() and clearSq(), beside placing a piece on a given square or erasing *
* it,  must  take care for all the incrementally updated  stuff:  hash  keys, *
* piece counters, material and pcsq values, pawn-related data, king location. *
******************************************************************************/

void FillSq(U8 color, U8 piece, S8 sq) {

	// place a piece on the board
	board.pieces[sq] = piece;
	board.color[sq] = color;

	// update king location
	if (piece == KING)
		board.king_loc[color] = sq;

	/**************************************************************************
	* Pawn structure changes slower than piece position, which allows reusing *
	* some data, both in pawn and piece evaluation. For that reason we do     *
	* some extra work here, expecting to gain extra speed elsewhere.          *
	**************************************************************************/

	if (piece == PAWN) {
		// update pawn material
		board.pawn_material[color] += e.PIECE_VALUE[piece];

		// update pawn hashkey
		board.phash ^= zobrist.piecesquare[piece][color][sq];

		// update counter of pawns on a given rank and file
		++board.pawns_on_file[color][COL(sq)];
		++board.pawns_on_rank[color][ROW(sq)];

		// update squares controlled by pawns
		if (color == WHITE) {
			if (IS_SQ(sq + NE)) board.pawn_ctrl[WHITE][sq + NE]++;
			if (IS_SQ(sq + NW)) board.pawn_ctrl[WHITE][sq + NW]++;
		}
		else {
			if (IS_SQ(sq + SE)) board.pawn_ctrl[BLACK][sq + SE]++;
			if (IS_SQ(sq + SW)) board.pawn_ctrl[BLACK][sq + SW]++;
		}
	}
	else {
		// update piece material
		board.piece_material[color] += e.PIECE_VALUE[piece];
	}

	// update piece counter
	board.piece_cnt[color][piece]++;

	// update piece-square value
	board.pcsq_mg[color] += e.mgPst[piece][color][sq];
	board.pcsq_eg[color] += e.egPst[piece][color][sq];

	// update hash key
	board.hash ^= zobrist.piecesquare[piece][color][sq];
}


void ClearSq(SQ sq) {

	// set intermediate variables, then do the same
	// as in fillSq(), only backwards

	U8 color = board.color[sq];
	U8 piece = board.pieces[sq];

	board.hash ^= zobrist.piecesquare[piece][color][sq];

	if (piece == PAWN) {
		// update squares controlled by pawns
		if (color == WHITE) {
			if (IS_SQ(sq + NE)) board.pawn_ctrl[WHITE][sq + NE]--;
			if (IS_SQ(sq + NW)) board.pawn_ctrl[WHITE][sq + NW]--;
		}
		else {
			if (IS_SQ(sq + SE)) board.pawn_ctrl[BLACK][sq + SE]--;
			if (IS_SQ(sq + SW)) board.pawn_ctrl[BLACK][sq + SW]--;
		}

		--board.pawns_on_file[color][COL(sq)];
		--board.pawns_on_rank[color][ROW(sq)];
		board.pawn_material[color] -= e.PIECE_VALUE[piece];
		board.phash ^= zobrist.piecesquare[piece][color][sq];
	}
	else
		board.piece_material[color] -= e.PIECE_VALUE[piece];

	board.pcsq_mg[color] -= e.mgPst[piece][color][sq];
	board.pcsq_eg[color] -= e.egPst[piece][color][sq];

	board.piece_cnt[color][piece]--;

	board.pieces[sq] = PIECE_EMPTY;
	board.color[sq] = COLOR_EMPTY;
}


int board_loadFromFen(char* fen) {

	clearBoard();
	clearHistoryTable();

	char* f = fen;

	char col = 0;
	char row = 7;

	do {
		switch (f[0]) {
		case 'K':
			FillSq(WHITE, KING, SET_SQ(row, col));
			col++;
			break;
		case 'Q':
			FillSq(WHITE, QUEEN, SET_SQ(row, col));
			col++;
			break;
		case 'R':
			FillSq(WHITE, ROOK, SET_SQ(row, col));
			col++;
			break;
		case 'B':
			FillSq(WHITE, BISHOP, SET_SQ(row, col));
			col++;
			break;
		case 'N':
			FillSq(WHITE, KNIGHT, SET_SQ(row, col));
			col++;
			break;
		case 'P':
			FillSq(WHITE, PAWN, SET_SQ(row, col));
			col++;
			break;
		case 'k':
			FillSq(BLACK, KING, SET_SQ(row, col));
			col++;
			break;
		case 'q':
			FillSq(BLACK, QUEEN, SET_SQ(row, col));
			col++;
			break;
		case 'r':
			FillSq(BLACK, ROOK, SET_SQ(row, col));
			col++;
			break;
		case 'b':
			FillSq(BLACK, BISHOP, SET_SQ(row, col));
			col++;
			break;
		case 'n':
			FillSq(BLACK, KNIGHT, SET_SQ(row, col));
			col++;
			break;
		case 'p':
			FillSq(BLACK, PAWN, SET_SQ(row, col));
			col++;
			break;
		case '/':
			row--;
			col = 0;
			break;
		case '1':
			col += 1;
			break;
		case '2':
			col += 2;
			break;
		case '3':
			col += 3;
			break;
		case '4':
			col += 4;
			break;
		case '5':
			col += 5;
			break;
		case '6':
			col += 6;
			break;
		case '7':
			col += 7;
			break;
		case '8':
			col += 8;
			break;
		};

		f++;
	} while (f[0] != ' ');

	f++;

	if (f[0] == 'w') {
		board.stm = WHITE;
	}
	else {
		board.stm = BLACK;
		board.hash ^= zobrist.color;
	}

	f += 2;

	do {
		switch (f[0]) {
		case 'K':
			board.castle |= CASTLE_WK;
			break;
		case 'Q':
			board.castle |= CASTLE_WQ;
			break;
		case 'k':
			board.castle |= CASTLE_BK;
			break;
		case 'q':
			board.castle |= CASTLE_BQ;
			break;
		}

		f++;
	} while (f[0] != ' ');

	board.hash ^= zobrist.castling[board.castle];

	f++;

	if (f[0] != '-') {
		board.ep = StrToSquare(f);
		board.hash ^= zobrist.ep[board.ep];
	}

	do {
		f++;
	} while (f[0] != ' ');
	f++;
	int ply = 0;
	int converted;
	converted = sscanf(f, "%d", &ply);
	board.ply = (unsigned char)ply;

	board.rep_index = 0;
	board.rep_stack[board.rep_index] = board.hash;

	return 0;
}

static inline void PerftDriver(U8 depth) {
	s_Move m[256];
	int mcount = movegen(m, 0xFF);
	for (int i = 0; i < mcount; i++){
		move_make(m[i]);
		if (!isAttacked(board.stm, board.king_loc[board.stm ^ 1]))
			if (depth)
				PerftDriver(depth - 1);
			else
				info.nodes++;
		move_unmake(m[i]);
	}
}

void ResetInfo() {
	info.ponder = false;
	info.post = true;
	info.stop = false;
	info.flags = 0;
	info.nodes = 0;
	info.nodesLimit = 0;
	info.depthLimit = MAX_PLY;
	info.timeLimit = 0;
	info.timeStart = GetTimeMs();
}

void UciPerformance() {
	ResetInfo();
	PrintPerformanceHeader();
	info.depthLimit = 0;
	info.flags = FDEPTH;
	U64 elapsed = 0;
	while (elapsed < 3000) {
		PerftDriver(info.depthLimit++);
		elapsed = GetTimeMs() - info.timeStart;
		printf(" %2d. %8llu %12llu\n", info.depthLimit,elapsed,info.nodes);
	}
	PrintSummary(elapsed, info.nodes);
}

void UciBench() {
	ResetInfo();
	PrintBenchHeader();
	info.post = false;
	info.depthLimit = 0;
	info.flags = FDEPTH;
	U64 elapsed = 0;
	while (elapsed < 3000) {
		info.depthLimit++;
		search_run();
		elapsed = GetTimeMs() - info.timeStart;
		printf(" %2d. %8llu %12llu %5d %s\n", info.depthLimit,elapsed,info.nodes, sd.score, sd.pv);
	}
	PrintSummary(elapsed, info.nodes);
}

s_Move StrToMove(char* a)
{
	s_Move m = {};
	m.from = StrToSquare(a);
	m.to = StrToSquare(a + 2);

	m.piece_from = board.pieces[m.from];
	m.piece_to = board.pieces[m.from];
	m.piece_cap = board.pieces[m.to];

	m.flags = 0;
	m.castle = 0;
	m.ep = -1;
	m.ply = 0;
	m.score = 0;

	/* default promotion to queen */

	if ((m.piece_to == PAWN) &&
		(ROW(m.to) == ROW_1 || ROW(m.to) == ROW_8))
		m.piece_to = QUEEN;


	switch (a[4]) {
	case 'q':
		m.piece_to = QUEEN;
		a++;
		break;
	case 'r':
		m.piece_to = ROOK;
		a++;
		break;
	case 'b':
		m.piece_to = BISHOP;
		a++;
		break;
	case 'n':
		m.piece_to = KNIGHT;
		a++;
		break;
	}

	//castling
	if ((m.piece_from == KING) &&
		((m.from == E1 && (m.to == G1 || m.to == C1)) ||
			(m.from == E8 && (m.to == G8 || m.to == C8)))) {
		m.flags = MFLAG_CASTLE;
	}

	/* ep
		if the moving-piece is a Pawn, the square it moves to is empty and
		it was a diagonal move it has to be an en-passant capture.
	*/
	if ((m.piece_from == PAWN) &&
		(m.piece_cap == PIECE_EMPTY) &&
		((abs(m.from - m.to) == 15) || (abs(m.from - m.to) == 17))) {
		m.flags = MFLAG_EPCAPTURE;
	}

	if ((m.piece_from == PAWN) && (abs(m.from - m.to) == 32)) {
		m.flags |= MFLAG_EP;
	}

	return m;
}

bool algebraic_moves(char* a)
{
	s_Move m = {};
	bool found_match = false;
	while (a[0]) {

		if (!((a[0] >= 'a') && (a[0] <= 'h'))) {
			a++;
			continue;
		}
		m = StrToMove(a);
		found_match = move_isLegal(m);
		if (found_match)
		{
			move_make(m);

			if ((m.piece_from == PAWN) ||
				(move_iscapt(m)) ||
				(m.flags == MFLAG_CASTLE))
				board.rep_index = 0;
		}
		else
			break;
		a += 4;
		if (a[0] == 0) break;
		if (a[0] != ' ') a++;
	}
	return found_match;
}


char* MoveToStr(s_Move m, char* a)
{
	char parray[5] = { 0,'q','r','b','n' };
	SquareToStr(m.from, a);
	SquareToStr(m.to, a + 2);
	a += 4;
	if (m.piece_to != m.piece_from) {
		a[0] = parray[m.piece_to];
		a++;
	}
	a[0] = 0;
	return a;
}

void SquareToStr(SQ sq, char* a)
{
	a[0] = COL(sq) + 'a';
	a[1] = ROW(sq) + '1';
	a[2] = 0;
}

SQ StrToSquare(char* a)
{
	return a[0] - 'a' | ((a[1] - '1') << 4);
}

static void PrintWelcome() {
	cout << NAME << " " << VERSION << endl;
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

void PrintSummary(unsigned int time, unsigned long long nodes) {
	if (time < 1)
		time = 1;
	U64 nps = (nodes * 1000) / time;
	const char* units[] = { "", "k", "m", "g" };
	int sn = ShrinkNumber(nps);
	U64 p = pow(10, sn * 3);
	printf("-----------------------------\n");
	printf("Time        : %d\n", time);
	printf("Nodes       : %llu\n", nodes);
	printf("Nps         : %llu (%llu%s/s)\n", nps, nps / p, units[sn]);
	printf("-----------------------------\n");
}

void PrintBenchHeader()
{
	printf("-------------------------------------------------------\n");
	printf("ply      time        nodes score pv\n");
	printf("-------------------------------------------------------\n");
}

void PrintPerformanceHeader()
{
	printf("-------------------------------------------------------\n");
	printf("ply      time        nodes\n");
	printf("-------------------------------------------------------\n");
}

void PrintBoard() {
	string uw = "KQRBNAXX";
	string ub = "kqrbnaxx";
	string s = "   +---+---+---+---+---+---+---+---+";
	string t = "     A   B   C   D   E   F   G   H";
	cout << t << endl;
	for (int r = 7; r >= 0; r--) {
		cout << s << endl;
		printf(" %d |", r + 1);
		for (int f = 0; f <= 7; f++) {
			int sq = SET_SQ(r, f);
			int piece = board.pieces[sq];
			if (piece == PIECE_EMPTY)
				printf("   |");
			else if (board.color[sq] == WHITE)
				printf(" %c |", uw[piece & 0x7]);
			else if (board.color[sq] == BLACK)
				printf(" %c |", ub[piece & 0x7]);
		}
		cout << " " << r + 1 << endl;
	}

	cout << s << endl;
	cout << t << endl;
	cout << "side to move: " << (board.stm == WHITE ? "white" : "black") << endl;
}

static void UciGo(char* command) {
	ResetInfo();
	int movestogo = 32;
	char* token;
	if (strstr(command, "infinite"))
		info.flags |= FINFINITE;
	if (strstr(command, "ponder")) {
		info.ponder = true;
		info.flags |= FINFINITE;
	}
	int converted;
	token = strstr(command, "wtime");
	if (token > 0)
	{
		info.flags |= FTIME;
		converted = sscanf(token, "%*s %d", &info.time[WHITE]);
	}
	token = strstr(command, "btime");
	if (token > 0)
	{
		info.flags |= FTIME;
		converted = sscanf(token, "%*s %d", &info.time[BLACK]);
	}
	token = strstr(command, "winc");
	if (token > 0)
	{
		info.flags |= FINC;
		converted = sscanf(token, "%*s %d", &info.inc[WHITE]);
	}
	token = strstr(command, "binc");
	if (token > 0)
	{
		info.flags |= FINC;
		converted = sscanf(token, "%*s %d", &info.inc[BLACK]);
	}
	token = strstr(command, "movestogo");
	if (token > 0)
	{
		info.flags |= FMOVESTOGO;
		converted = sscanf(token, "%*s %d", &movestogo);
	}
	token = strstr(command, "depth");
	if (token > 0)
	{
		info.flags |= FDEPTH;
		converted = sscanf(token, "%*s %d", &info.depthLimit);
	}
	token = strstr(command, "nodes");
	if (token > 0)
	{
		info.flags |= FNODES;
		converted = sscanf(token, "%*s %ull", &info.nodesLimit);
	}
	token = strstr(command, "movetime");
	if (token > 0)
	{
		info.flags |= FMOVETIME;
		converted = sscanf(token, "%*s %d", &info.timeLimit);
	}
	if (info.flags == 0)
		info.flags |= FINFINITE;
	int time = board.stm ? info.time[BLACK] : info.time[WHITE];
	int inc = board.stm ? info.inc[BLACK] : info.inc[WHITE];
	if (time)
		info.timeLimit = min(time / movestogo + inc, time / 2);
	search_run();
}

void UciStop() {
	info.stop = true;
}

void UciPonderhit() {
	info.ponder = false;
	info.flags &= ~FINFINITE;
	info.timeStart = GetTimeMs();
}

void UciQuit() {
	exit(0);
}

void UciCommand(char* command)
{
	if (!strcmp(command, "uci"))
	{
		printf("id name %s\n", NAME);
		printf("option name hash type spin default 64 min 1 max 1024\n");
		printf("option name aspiration type spin default 50 min 0 max 100\n");
		printf("option name draw_opening type spin default -10 min -100 max 100\n");
		printf("option name draw_endgame type spin default 0 min -100 max 100\n");
		printf("option name UCI_Elo type spin default %d min %d max %d\n", options.eloMax, options.eloMin, options.eloMax);
		printf("option name ponder type check default %s\n", options.ponder ? "true" : "false");
		printf("uciok\n");
	}
	if (!strcmp(command, "isready"))
		printf("readyok\n");
	if (!strncmp(command, "setoption", 9))
	{
		char name[256];
		char value[256];
		if (strstr(command, "setoption name Ponder value"))
			options.ponder = (strstr(command, "value true") != 0);
		int converted = sscanf(command, "setoption name %s value %s", name, value);
		name[255] = 0;
		if (!strcmp(name, "Hash"))
		{
			int val = 64;
			converted = sscanf(value, "%d", &val);
			if (converted < 0)
			{
				tt_setsize(val << 20);
				ttpawn_setsize(val << 18);
			}
		}
		if (!strcmp(name, "aspiration"))
			converted = sscanf(value, "%d", &options.aspiration);
		if (!strcmp(name, "contempt"))
			converted = sscanf(value, "%d", &options.contempt);
		if (!strcmp(name, "UCI_Elo"))
			if (sscanf(value, "%d", &options.elo) > 0)
				setDefaultEval();
	}
	if (!strcmp(command, "ucinewgame")) {}
	if (!strncmp(command, "position", 8))
	{
		if (!strncmp(command, "position fen", 12))
			board_loadFromFen(command + 13);
		else
			board_loadFromFen(STARTFEN);
		char* moves = strstr(command, "moves");
		if (moves)
			if (!algebraic_moves(moves + 6))
				printf("wrong moves\n");
	}
	if (!strncmp(command, "go", 2))
		UciGo(command);
	if (!strcmp(command, "stop"))
		UciStop();
	if (!strcmp(command, "ponderhit"))
		UciPonderhit();
	if (!strcmp(command, "quit"))
		UciQuit();
	if (!strncmp(command, "bench", 5))
		UciBench();
	if (!strncmp(command, "perft", 5))
		UciPerformance();
	if (!strcmp(command, "eval"))
		printEval();
	if (!strcmp(command, "print"))
		PrintBoard();
}

void UciLoop() {
	while (true) {
		string line;
		getline(cin, line);
		UciCommand((char*)line.c_str());
	}
}

int main()
{
	PrintWelcome();
	setDefaultEval();
	tt_init();
	tt_setsize(0x4000000);     //64m
	ttpawn_setsize(0x1000000); //16m
	tteval_setsize(0x2000000); //32m
	board_loadFromFen(STARTFEN);
	UciLoop();
}