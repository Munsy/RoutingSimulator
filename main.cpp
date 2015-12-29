// Justin Slone, Alex Kerr, Tim Munson
// CptS 455
// Project 2: Distance Vector Routing
// main.cpp
// Created 11/9/2015
// Last modified 11/9/2015

#include "distancevector.h"

using std::cout;
using std::endl;

int main(int argc, char **argv)
{
	bool poisoned = false;
	Router *router;
	if(argc < 3)
	{
		cout << "Usage: 'router [testdir] [routername]'" << endl;
		return -1;
	}
	if(argc == 4)
	{
		char *str = argv[1];
		if(strcmp(str, "-p") == 0)
		{
			poisoned = true;
		}
		router = new Router(argv[2], argv[3]);
	}
	else
	{
		router = new Router(argv[1], argv[2]);
	}
	router->Start(poisoned);
}