
#include "program.h"

using namespace std;

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
}

static void UciGo(char* command) {
	ResetInfo();
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
		converted = sscanf(token, "%*s %d", &info.movestogo);
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
		converted = sscanf(token, "%*s %d", &info.nodesLimit);
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
		info.timeLimit = min(time / info.movestogo + inc, time / 2);
	search_run();
}

void UciStop() {
	info.stop = true;
}

void UciPonderhit(){
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
		printf("id name RapCpw\n");
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
	while (true)
	{
		string line;
		getline(cin, line);
		UciCommand((char*)line.c_str());
	}
}