/*
 * main.cxx 12/01/29
 *
 * Miquel Ferran <mikilaguna@gmail.com>
 */

#include <cstdlib>
#include <cstring>
#include <iostream>

#include "common.h"

using namespace std;

int main(int argc, char* argv[])
{
	if (argc > 1)
	{
		if (strcmp(argv[1], "send") == 0 and argc == 6)
			return sender_main(argv[2], argv[3], argv[4], argv[5], 0);
		if (strcmp(argv[1], "send") == 0 and argc == 7)
			return sender_main(argv[3], argv[4], argv[5], argv[6], argv[2]);
		if (strcmp(argv[1], "receive") == 0 and argc == 4)
			return receiver_main(argv[2], argv[3]);
	}
	
	cout << "Usage: gloox-ft-test" << endl << endl;
	cout << "  send [integrated proxy port, if any] \\" << endl;
	cout << "       <sender user@server/resource> <sender password> \\" << endl;
	cout << "       <receiver user@server/resource> <filename>" << endl << endl;
	cout << "  receive <receiver user@server/resource> <receiver password>";
	cout << endl << endl;
	return EXIT_FAILURE;
}
