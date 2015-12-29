// Justin Slone, Alex Kerr, Tim Munson
// distancevector.cpp
// Created 11/9/2015
// Last modified 11/11/2015

#include "distancevector.h"

using namespace std;

// Router constructor
Router::Router(string testdir, string routername)
{
	maxDescPlus1 = 0;
	
	// Initialize the router with the supplied directory and name.
	Init(testdir, routername);
}

// Loads the routing table from a configuration file
// associated with the input directory.
bool Router::Load(string testdir)
{
	string line;

	// Open "<routername>.cfg" in the supplied test directory.
	string cwd(get_current_dir_name());
	ifstream file(cwd + "/" + testdir + "/" + m_attribute->name + ".cfg");

	// Set all attributes to have infinite cost, no neighbor, and given name.
	for(auto *attribute : m_attributes)
	{
		if(m_attribute->name == attribute->name)
		{
			continue;
		}

		TableEntry* entry = new TableEntry(attribute->name, "", INFINITY);

		m_table.push_back(entry);
	}

	// Make sure file is open and stop the loading process if it's not.
	if(!file.is_open())
	{
		cout << "ERROR: Failed to create filestream." << endl;
		return false;
	}

	while(!file.eof())								// While not at the end of the file
	{
		getline(file, line);						// Get a line from the file.
		vector<string> tokens = TokenizeString(line);						// For tokenizing the line.

		if(4 == tokens.size())						// We're looking for four specific values from parsing.
		{
			TableEntry* entry = NULL;				// Set entry to NULL or else we'll segfault in later code...
			for(auto *te : m_table)
			{
				if(tokens[0] == te->destination)	// SGet the TableEntry with a destination value 
				{									// matching the router name, then quit looping.
					entry = te;
					break;
				}
			}

			if(NULL != entry)							// If we actually found a pre-existing entry
			{
				entry->next = tokens[0];				// Set entry's next to the given name from the current line. 
				entry->cost = atoi(tokens[1].c_str());	// Set entry's cost to the given cost from the current line.
	
				//keep track of our own distance to our neighbors, might be different than whats in the routing table				
				LinkCost* myNeighbor = new LinkCost(tokens[0], atoi(tokens[1].c_str()));
				m_neighborsCost.push_back(myNeighbor);

				PrintRTableEntry(entry->destination, entry->cost, entry->next);
			}
			else																		// Didn't find a pre-existing entry
			{
				entry = new TableEntry(tokens[0], tokens[0], atoi(tokens[1].c_str())); 	// Make a new one!
				m_table.push_back(entry);												// Add the new entry to the routing table.
				LinkCost* myNeighbor = new LinkCost(tokens[0], atoi(tokens[1].c_str()));
				m_neighborsCost.push_back(myNeighbor);
			}

			Attribute* remote_attribute = NULL;
			for(auto *a : m_attributes)					// Select the router attribute matching the given name from the current line.
			{
				if(tokens[0] == a->name)
				{
					remote_attribute = a;
					break;
				}
			}

			if(NULL == remote_attribute)				// We can't create a router connection if we don't know about that router.
			{
				cout << "ERROR: Remote attribute is NULL." << endl;
				return false;
			}

			// Make a new socket connection according to the tokens we obtained from the current line and remote router configuration.
			int sock = MakeSocket(m_attribute->name, atoi(tokens[2].c_str()) + m_attribute->port,
								  remote_attribute->name, atoi(tokens[3].c_str()) + remote_attribute->port);
			Connection* connection = new Connection(tokens[0], sock); 	// Create a new connection to the remote router.
			m_neighbors.push_back(connection);							// Add the connection to the router's list of neighbors.
		}
	}

	MakeDVTable();

	file.close();	// Don't forget to close the file.

	return true;
}

bool Router::MakeDVTable()
{
	//Make my row.
	DistanceVector *m_row = new DistanceVector();
	m_row->name = m_attribute->name;
	for(auto *n : m_neighborsCost)
	{
		LinkCost *lc = new LinkCost(n->name, n->cost);
		m_row->linkcosts.push_back(lc);
	}
	for(auto *a : m_attributes)
	{
		if(!isNeighbor(a->name))
		{
			LinkCost *temp = new LinkCost(a->name, 0);

			if(temp->name == m_attribute->name)
			{
			
				temp->cost = 0;
			}
			else
			{
				temp->cost = INFINITY;
			}
			m_row->linkcosts.push_back(temp);
		}
	}
	m_DVtable.push_back(m_row);
	

	//make all neighbors rows.
	for(auto *n : m_neighborsCost)
	{
		DistanceVector *n_row = new DistanceVector();
		n_row->name = n->name;
		for(auto *a : m_attributes)
		{
			LinkCost *lc = new LinkCost(a->name, 64);
			n_row->linkcosts.push_back(lc);
		}
		m_DVtable.push_back(n_row);
	}


	return true;
}

//checks if the router of the given name is a neighbor of this node
bool Router::isNeighbor(string name)
{
	for(auto *n : m_neighborsCost)
	{
		if(n->name == name)
		{
			return true;
		}
	}
	return false;
}

// Add names, hostnames, and ports to other routers 
// for other routers relative to this router.
bool Router::Init(string testdir, string routername)
{
	string line;
	string cwd(get_current_dir_name());
	ifstream file(cwd + "/" + testdir + "/" + "routers");

	while(!file.eof())
	{
		//string buffer;
		getline(file, line);
		//stringstream ss(line);
		vector<string> tokens = TokenizeString(line);

		//while(ss >> buffer)
		//{
		//	tokens.push_back(buffer);
		//}

		if(3 == tokens.size())
		{
			Attribute* attribute = new Attribute(tokens[0], tokens[1], atoi(tokens[2].c_str()));
			m_attributes.push_back(attribute);
		}
	}

	for(auto *attribute : m_attributes)
	{
		if(attribute->name == routername)
		{
			m_attribute = attribute;
			//m_attribute = new Attribute(routername, routername, 0);			// Initialize router attribute.
			int sock = MakeSocket(m_attribute->name, m_attribute->port, "", 0);	// Create a new socket.
			Connection* connection = new Connection(m_attribute->name, sock);	// Create a new connection.
			m_neighbors.push_back(connection);					// Add our connection to the list of neighbors.

			break;
		}
	}
	Load(testdir);
	return (NULL == m_attribute);
}

// Creates a UDP socket and returns the associated file descriptor.
int Router::MakeSocket(string hostname, int hostport, string remotename, int remoteport)
{
	struct sockaddr_in routeraddr;				// Our address
    struct sockaddr_in remoteaddr;				// Remote address
    int sock;                     	    		// Socket descriptor (return value)
    unsigned int length = 0;

	if(0 > (sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)))
	{
		cout << "ERROR: Invalid file descriptor when making socket." << endl;
		return -1;
	}

	// Initialize the sockaddr_in routeraddr structure.
	memset((char *)&routeraddr, 0, sizeof(routeraddr));
    routeraddr.sin_family = AF_INET;
    routeraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    routeraddr.sin_port = htons(hostport);

    // Bind syscall - binds the socket to server_addr info.
    if(0 > bind(sock, (struct sockaddr *)&routeraddr, sizeof(routeraddr)))
    {
        cout << "ERROR: Bind failed." << endl;
        return -1;
    }

    if("" != remotename && 0 != remoteport)
    {

    	memset(&remoteaddr, 0, sizeof(remoteaddr));

    	remoteaddr.sin_family = AF_INET;
    	remoteaddr.sin_port = htons(remoteport);

    	length = sizeof(remoteaddr);

    	connect(sock, (struct sockaddr *)&remoteaddr, length);
    }
	
	m_sockets++;

	if(sock > maxDescPlus1-1)
	{
		maxDescPlus1 = sock+1;
	}
    return sock;
}

// Print the routing table for the given router. If a name is 
// supplied, only print the table entry corresponding to that name.
void Router::PrintTable(string name)
{
	//Print the routing table entry for the router passed
	if("" != name)
	{
		cout << "----Routing Table Entry in Router " << m_attribute->name << " for destination " << name << "----" <<endl;
		for(auto entry: m_table)
		{
			if(entry->destination == name)
			{
				cout << "dest: " << entry->destination << " cost: ";
				entry->cost < 64 ? cout << entry->cost : cout << "inf"; 
				cout << " nexthop: ";
				entry->next == "" ? cout << "NULL" : cout << entry->next;
				cout << endl;
			}
		}
	}
	else //print the entire routing table
	{
		cout << "----Routing Table for Router " << m_attribute->name << "----" << endl;
		for(auto *entry : m_table)
		{
			cout << "dest: " << entry->destination << " cost: ";
			entry->cost < 64 ? cout << entry->cost : cout << "inf"; 
			cout << " nexthop: ";
			entry->next == "" ? cout << "NULL" : cout << entry->next;
			cout << endl;
		}
	}
}

// Informs neighboring routers about changes to this
// router's routing table.
bool Router::TellNeighbors()
{
	DistanceVector* myDV;
		

	for(auto dv: m_DVtable)
	{
		if(dv->name == m_attribute->name)
		{
			myDV = dv;
		}
	}

	for(auto sock: m_neighbors)
	{
		string update = "U ";
		for(auto l : myDV->linkcosts)
		{
			update += l->name + " ";
			string nextHop = GetNextHop(l->name);
			if(nextHop == sock->name && poisoned== true)
			{
				update += to_string(INFINITY) + " ";
			}
			else
			{ 
				update += to_string(l->cost) + " ";
			}	
		}

		if(m_attribute->name != sock->name)
		{

				send(sock->socket, update.c_str(), strlen(update.c_str()), 0);

		}
	}
	return true;
}

//get the next hop from this router to the destination from its routing table
string Router::GetNextHop(string dest)
{
	string next = "";

	for(auto entry: m_table)
	{
		if(entry->destination == dest)
			next = entry->next;
	}	
	
	return next;
}

//takes in a string which was received from a neighbor and that neighbor's name to update that row in our DV table
bool Router::UpdateDVTableRow(string name, string row)
{
	vector<string> tokens = TokenizeString(row);
	DistanceVector* myDV;
	
	//get the correct row in the distance vector table
	for(auto dv: m_DVtable)
	{
		if(dv->name == name)
			myDV = dv;
	}
	
	//go through all the information sent by the neighbor and update the cost in the appropriate link	
	for(int i = 1; i < (int) tokens.size()-1; i+=2)
	{
		string col = tokens[i];
		int cost = atoi(tokens[i+1].c_str());
		
		for(auto rt: myDV->linkcosts)
		{
			if(rt->name == col)
				rt->cost = cost;
		}		
	}
	//check to see if we should update our own row in the distance vector table
	return CheckMyDVTable(name);
}

//Checks to see if we should update our own row in the distance vector table
bool Router::CheckMyDVTable(string sender)
{
	bool updated = false; //used to verify we need to tell our neighbors
	DistanceVector *myDV;
	DistanceVector *destDV;
		
	//get our own row out of the distance vector table
	for(auto dv: m_DVtable)
	{
		if(dv->name == m_attribute->name)
			myDV = dv;
		else if(dv->name == sender)
			destDV = dv;
	}

	if(sender != "")
	{
		//Go through your DV table
		for(auto lc : myDV->linkcosts)
		{
			//Get the next hop of each column
			string nextHop = GetNextHop(lc->name);

			//If next hop of current column is the sender
			if(nextHop == sender)
			{
				int total = 0;
				//Add the cost of getting from here to the sender
				total += GetCost(m_attribute->name, nextHop);

				//Go through the senders row
				for(auto destLC : destDV->linkcosts)
				{
					//Find the column that matches our current column
					if(destLC->name == lc->name)
					{
						//Add the cost of getting from the sender to the final destination
						total += destLC->cost;

					
						//If they don't match we need to update our routing table
						if(lc->cost != total)
						{
							lc->cost = total;

							UpdateRoutingTableEntry(lc->name, GetNextHop(nextHop), lc->cost);

							updated = true;
						}
						break;
					}
				}
				
			}
		}
	}
	//go through all the router to see if we need to update
	for(auto atr: m_attributes)
	{	
		//don't check if we should update cost to ourseleves - its always 0
		if(atr->name != m_attribute->name)
		{
			
			//get the current cost to reach the router atr
			int minCost = GetCost(m_attribute->name, atr->name);
			string next = "";
			bool updatedCost = false; //used to verify we need to update our distance vector row
			for(auto n: m_neighborsCost)
			{
				int myCostToN = n->cost; //cost from this router to its neighbor
				int NCostToDest = GetCost(n->name, atr->name); //cost from neighbor to the destination router
				int totalCost = myCostToN + NCostToDest; 
				//check if there is a better way to reach the destination 
				if(totalCost < minCost)
				{
					//set the minCost and note which neighbor we use to get there
					minCost = totalCost;
					next = n->name;
					//set the flags so we will update our DV table row, routing table, and tell our neighbors
					updatedCost = true;
					updated = true;
				}
			}
			if(updatedCost)
			{
				//need to update my row
				for(auto rt: myDV->linkcosts)
				{
					if(rt->name == atr->name)
					{
						rt->cost = minCost;
					}
				}
				//need to update my routing table
				UpdateRoutingTableEntry(atr->name, next, minCost);
			}
		}
	}
	
	if(updated)
	{
		TellNeighbors();
	}
	return updated;
}

//goes through the distance vector table and gets the cost from the router name source to the router named destination
int Router::GetCost(string source, string destination)
{
	int cost = INFINITY;

	DistanceVector* myDV = NULL;
	
	//get the row for the source router in the distance vector table
	for(auto dv: m_DVtable)
	{
		if(dv->name == source)
			myDV = dv;
	}

	//get the cost from source router to destination
	for(auto rt: myDV->linkcosts)
	{
		if(rt->name == destination)
		{
			cost = rt->cost;
		}
	}
	return cost;
}

//updates the routing table entry for the router given as dest
void Router::UpdateRoutingTableEntry(string dest, string hop, int cost)
{
	//find the appropriate entry in the routing table
	for(auto entry: m_table)
	{
		if(entry->destination == dest)
		{
			//update the next hop and cost
			entry->next = hop;
			entry->cost = cost;
			//print the change to std out
			PrintRTableEntry(entry->destination, entry->cost, entry->next);
			break;
		}
	}
}

void Router::PrintRTableEntry(string dest, int cost, string next)
{
	cout << "(" << m_attribute->name << " - dest: " << dest << " cost: " << cost << " nexthop: " << next << ")" << endl;
}


void Router::UpdateNeighborCost(string neighbor, int newCost)
{
	for(auto n: m_neighborsCost)
	{
		if(n->name == neighbor)
		{
			n->cost = newCost;

			string nextHop = GetNextHop(n->name);

			if(nextHop == n->name)
			{
				for(auto dvRow: m_DVtable)
				{
					if(dvRow->name == m_attribute->name)
					{
						for(auto dvCol: dvRow->linkcosts)
						{
							if(dvCol->name == n->name)
							{
								dvCol->cost = n->cost;
							}
						}
					}
				}
			}

			CheckMyDVTable("");
			return;
		}
	}
}


bool Router::ReadSocket(Connection *c)
{
	char buffer[100];
	memset(buffer, 0, 100);

	recv(c->socket, buffer, 100, 0);

	string str(buffer);
	if(str != "" && c->name != m_attribute->name)
	{
		bool r = UpdateDVTableRow(c->name, str);
		//PrintDVTable();
		return r;
	}
	else if(c->name == m_attribute->name && str != "")
	{
		vector<string> tokens = TokenizeString(str);
		
		if(tokens[0] == "L")
		{
			//cout << "Took in L. Passing " << tokens[1] << " and " << tokens[2] << " to UpdateNeighborCost" << endl;
			UpdateNeighborCost(tokens[1], atoi(tokens[2].c_str()));
		}
		else if(tokens[0] == "P")
		{
			if(tokens.size() == 1)
			{
				PrintTable("");				
			}
			else
			{
				PrintTable(tokens[1]);
			}
		}
		
	}
	return false;
}

void Router::PrintDVTable()
{
	cout << "----DV Table----" << endl;
	for(auto *dv : m_DVtable)
	{
		cout << dv->name;
		for(auto *lc : dv->linkcosts)
		{
			cout << "\t" << lc->name << ": ";
			lc->cost < 64 ? cout << lc->cost : cout << "inf";
		}
		cout << endl;
	}
}

void Router::PrintDVTableRow(string destination)
{
	cout << "----Row " << destination << " in DV Table----" <<endl;
	for(auto *lc : m_DVtable[0]->linkcosts)
	{
		cout << "\t" << lc->name;
	}
	cout << endl;

	for(auto dv: m_DVtable)
	{
		if(dv->name == destination)
		{
			cout << dv->name;
			for(auto *lc : dv->linkcosts)
			{
				cout << "\t";
				lc->cost < 64 ? cout << lc->cost : cout << "inf";
			}
			cout << endl;
		}
	}
		
}

vector<string> Router::TokenizeString(string input)
{
	vector<string> tokens;
	string buffer;
	stringstream ss(input);

	while(ss >> buffer)
	{
		tokens.push_back(buffer);
	}

	return tokens;
}

// Start broadcasting and listening on this router.
void Router::Start(bool poisoned)
{
	this->poisoned = poisoned;
//	cout << "#sockets = " << m_sockets << endl;
//	cout << "maxDescPlus1: " << maxDescPlus1 << endl;
//	PrintTable("B");
//	PrintTable("");
	
	fd_set readFDs;
	timeval timeout;
	int r;

/*	for(auto serv: m_neighbors)
	{
		cout<<"Socket: " << serv->name << " Port#: " << serv->socket << endl;
	}
*/


	// HOW THIS SHOULD WORK:
	// http://beej.us/guide/bgnet/output/html/multipage/advanced.html#select
	// 
	int sec = TIMEOUT_TIME;
	int usec = 0;
	while(true)
	{
		FD_ZERO(&readFDs);
		for(auto *servSock: m_neighbors)
		{
			FD_SET(servSock->socket, &readFDs);
		}

		timeout.tv_sec = sec;
		timeout.tv_usec = usec;

		r = select(maxDescPlus1, &readFDs, NULL, NULL, &timeout);

		if(r < 0)
		{
			cout << "Error: Select returned an error. Code - " << r << endl;
		}
		else if(r == 0)
		{
			sec = TIMEOUT_TIME;
			usec = 0;
			//PrintTable("");
			TellNeighbors();
		}
		else
		{
			for(auto servSock: m_neighbors)
			{
				if(FD_ISSET(servSock->socket, &readFDs))
				{
					//do something
					bool changed = ReadSocket(servSock);
					if(changed != true)
					{
						sec = timeout.tv_sec;
						usec = timeout.tv_usec;
					}
					else {
						sec = TIMEOUT_TIME;
						usec = 0;
					}
				}
			}
		}
	}
}
