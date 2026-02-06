#include "program.h"

using namespace std;

s_SearchDriver sd = {};
s_Options options = {};
s_SearchInfo info = {};

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

s_Move strToMove(char* a)
{
	s_Move m = {};
	m.from = convert_a_0x88(a);
	m.to = convert_a_0x88(a + 2);

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
		m = strToMove(a);
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


char* algebraic_writemove(s_Move m, char* a)
{
	char parray[5] = { 0,'q','r','b','n' };
	convert_0x88_a(m.from, a);
	convert_0x88_a(m.to, a + 2);
	a += 4;
	if (m.piece_to != m.piece_from) {
		a[0] = parray[m.piece_to];
		a++;
	}
	a[0] = 0;
	return a;
}

void convert_0x88_a(SQ sq, char* a)
{
	a[0] = COL(sq) + 'a';
	a[1] = ROW(sq) + '1';
	a[2] = 0;
}

SQ convert_a_0x88(char* a)
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