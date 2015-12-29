#!/bin/bash

if [ -e distancevector.h.gch ]
	then
		rm distancevector.h.gch
fi

g++ -Wall -o router -std=c++11 main.cpp distancevector.h distancevector.cpp

