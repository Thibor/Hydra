#include "program.h"

using namespace std;

bool pipe;
HANDLE hstdin;

int InputInit()
{
	unsigned long dw;
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
	return 0;
}

bool InputAvailable() {
	unsigned long dw = 0;
	if (pipe)
		PeekNamedPipe(hstdin, 0, 0, 0, &dw, 0);
	else
		GetNumberOfConsoleInputEvents(hstdin, &dw);
	return dw > 1;
}

void ReadLine(char* str, int n) {
	char* ptr;
	if (fgets(str, n, stdin) == NULL)
		exit(0);
	if ((ptr = strchr(str, '\n')) != NULL)
		*ptr = '\0';
}

bool GetInput(string &s){
	char command[4000];
	if (!InputAvailable())
		return false;
	ReadLine(command, sizeof command);
	s= string(command);
	return true;
}