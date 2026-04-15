#include "program.h"

using namespace std;

s_SearchDriver sd = {};
s_Options options = {};
s_SearchInfo info = {};
s_Board board = {};

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