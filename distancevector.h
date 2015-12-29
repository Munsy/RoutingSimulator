// Justin Slone, Alex Kerr, Tim Munson
// distancevector.h
// Created 11/9/2015
// Last modified 11/11/2015

#ifndef __DISTANCEVECTOR_H__
#define __DISTANCEVECTOR_H__

#include <sys/types.h>  // Types used in sys/socket.h and netinet/in.h
#include <netinet/in.h> // Internet domain address structures and functions
#include <sys/socket.h> // Structures and functions used for socket API
#include <netdb.h>      // Used for domain/DNS hostname lookup
#include <arpa/inet.h>  // Definitions for internet operations
#include <unistd.h>		// Standard symbolic constants and types
#include <errno.h>		// System error numbers
#include <iostream>		// Header that defines standard input/output stream objects
#include <cstdlib>		// For using the atoi() function
#include <cstdio>		// FILE operations (can remove probably?)
#include <vector>		// For using C++ vectors
#include <cstring>		// For manipulating C-style arrays and strings
#include <fstream>		// C++ filestream methods
#include <sstream>		// C++ Stringstream methods
#include <string>		// Introduces string types, character traits and a set of converting functions

#define INFINITY 64
#define TIMEOUT_TIME 10

using namespace std;

class Router
{
public:
	//Used by a router to store its own information as well as how to reach all of the other routers
	typedef struct Attribute {
		string name;
		string host;
		int port;

		Attribute(string the_name, string the_host, int the_port)
		{
			name = the_name;
			host = the_host;
			port = the_port;
		}
	} ATTRIBUTE;
	
	//Routing table
	typedef struct TableEntry {
		string destination;
		string next;
		int cost;

		TableEntry(string the_destination, string the_next, int the_cost)
		{
			destination = the_destination;
			next = the_next;
			cost = the_cost;
		}
	} TABLEENTRY;

	//Used to keep track of which socket is used to connect to which neighbor
	typedef struct Connection {
		string name;
		int socket;

		Connection(string the_name, int the_socket)
		{
			name = the_name;
			socket = the_socket;
		}
	} CONNECTION;

	
	//used in the DistanceVector struct to represent columns in the distance vector table
	typedef struct LinkCost {
		string name;
		int cost;

		LinkCost(string linkName, int linkCost) 	//*A*
		{
			name = linkName;
			cost = linkCost;
		}
	} LINKCOST;
	//represents a row in the Distcace Vector Table. Name represents the router who's distance vector we're storing
	// linkcosts vector represents each of the other nodes in the table and the cost to reach them from the node name
	typedef struct DistanceVector {
		string name;
		vector<LinkCost*> linkcosts;
	} DISTANCEVECTOR;

	Attribute* m_attribute;
	vector<Attribute*> m_attributes;
	vector<TableEntry*> m_table;
	vector<Connection*> m_neighbors;
	vector<DistanceVector*> m_DVtable;
	bool poisoned;

	Router(string testdir, string routername);
	void Start(bool poisoned);

private:
	bool Init(string testdir, string routername);
	bool Load(string testdir);
	bool MakeDVTable();
	int MakeSocket(string hostname, int hostport, string remotename, int remoteport);
	bool UpdateConnection(string info);
	bool UpdateTable(string update, Connection* source);
	bool TellNeighbors();
	void PrintTable(string name);
	bool isNeighbor(string name);
	
	bool UpdateDVTableRow(string name, string row);
	bool CheckMyDVTable(string name);
	int GetCost(string source, string destination);
	void UpdateRoutingTableEntry(string dest, string hop, int cost);
	bool ReadSocket(Connection *c);
	
	vector<LinkCost*> m_neighborsCost; 
	int maxDescPlus1; 
	vector<Connection*> m_allSockets;
	int m_sockets;	
	
	void PrintDVTable();
	void PrintDVTableRow(string destination);

	string GetNextHop(string dest);

	void PrintRTableEntry(string dest, int cost, string next);
	void UpdateNeighborCost(string neighbor, int newCost);

	vector<string> TokenizeString(string input);
};

#endif
