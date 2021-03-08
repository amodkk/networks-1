#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring> 
#include <vector>
#include <sstream>
#include <fstream>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <stdint.h>

//close
#include <unistd.h>

#define BUFSIZE 128
#define DATASIZE 126
#define HEADERSIZE 2
//#define PORT 10035
#define PORT 10016
#define MAXLINE 1024

using namespace std;

typedef struct {
    uint8_t Sequence;
    unsigned char Checksum;
    char Data[DATASIZE];
} Packet;	

void usage();
char generateChecksum(char*, int);
bool gremlin(float, float, Packet*);
char* loadFileToBuffer();
Packet* constructPacket(char*, int);
bool sendPacket(const Packet*, bool); 

int fd;
int slen;
socklen_t slt;
struct sockaddr_in serverAddress; 

int main(int argc, char *argv[])
{
    //Socket variables
    struct hostent *hp;
    unsigned char buf[BUFSIZE];
    slen = sizeof(serverAddress);
    slt = sizeof(serverAddress);

    //Command line arguments
    string sServerAddress;
    float fDamaged = 0;
    float fLost = 0;

	bool bSent = false;
	bool bGremlin = false;

    //Sending variables
    Packet *pPacket;
	Packet *pTemp = new Packet;

    for(int i=1;i < argc; i+= 2)
    {
		//debug 
		cout << "input argv[1] " << argv[i] << endl; 
		cout << "input argv[2] " << argv[i+1] << endl;
		//sServerAddress = argv[2]; 

		//added def- atof: Parses the C string str, interpreting its content as a floating point number and returns its value as a double.
        switch (argv[i] [1])
        {
            case 'd':
            {
                fDamaged = atof(argv[i+1]);
				//debug 
				//cout << "here1" << endl; 
            };
            break;

            case 'l':
            {
                fLost = atof(argv[i+1]);
				//debug 
				//cout << "here2" << endl; 
            };
            break;

            case 's':
            {
                sServerAddress = argv[i+1];
				//debug
				//cout << "sServerAddress c_str()" << sServerAddress.c_str() << endl;
            };
            break;

            case 'h':
            {
				//debug
				cout << "At case 'h' " << endl;
                usage();
                return 0;
            }
        }
    }

    //Loop forever
    string ln;
    for(;;)
    {
		//Open socket
		if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
			cerr << "Unable to open socket. Exiting..." << endl;
			return 0;
		}

		//added: The bzero() function copies n bytes, each with a value of zero, into string s.
		bzero(&serverAddress, sizeof(serverAddress));
		serverAddress.sin_family = AF_INET;
		serverAddress.sin_port = htons(PORT);
		//added here
		//serverAddress.sin_addr = 
		//debug
		//cout << sServerAddress.c_str() << endl; 

		//try to open port, cout if error
		if (inet_aton(sServerAddress.c_str(), &serverAddress.sin_addr)==0)
		{
			cerr << "There was a problem with the server IP address. Exiting..." << endl;
			return 0;
		}

		//here if successfully opened port
		cout << "Type command or quit to quit: ";
		cin.clear();

		//debug 
		//cout << "ln should be cleared. ln: " << ln; 

		cin >> ln;

		//debug
		//cout << "ln is " << ln << endl; 

		if(ln.compare("quit") == 0)
			break;
		if(ln.compare("PUT") == 0 || ln.compare("put") == 0)
		{
			
			cin >> ln;
			cout << "input file name is: '" << ln << "'" << endl;
			//Now ln is our filename to send
			ifstream putfile;
			putfile.open(ln.c_str());
			unsigned char csum = 0x00;
			unsigned char lost = 0x00;
			char buff[DATASIZE];
			if(putfile.is_open())
			{
			
				//Now we send a 1 packet overhead for the filename
				bSent = false;
				pPacket = constructPacket((char*)"PUT TestFile", strlen("PUT TestFile"));
				while(!bSent ) {
					memcpy(pTemp, pPacket, sizeof( Packet ));
					bGremlin = gremlin(fDamaged, fLost, pTemp);
					bSent = sendPacket(pTemp, bGremlin);
					cout << "Sending put..." << endl;
				}	
			
				while(!putfile.eof())
				{

					/************************************************/
					// This part of the code reads in from the      //
					// open file and fills up the data part of      //
					// the packet. It also calculates the checksum. //
					/************************************************/
					for(int i = 0; i < DATASIZE; i++)
					{
						if(!putfile.eof())
						{
							if( i < 48 ) {
								cout << buff[i];
							}
							buff[i] = putfile.get();
						}
						else
						{
							if( i != 0 ) {
								buff[i-1]= '\0';
							}
							buff[i] = '\0';
						}
					}
					cout << endl;
				

					/*************************************************/
					// This part of the code looks at the input      //
					// parameters and determines if a packet should  //
					// be simulated lost or damaged. This is also    //
	  				// known as the GREMLIN function   		 //
					/*************************************************/

					//Send
					bSent = false;
					pPacket = constructPacket(buff, strlen(buff));
					while( !bSent ) {
						memcpy(pTemp, pPacket, sizeof( Packet ));
						bGremlin = gremlin(fDamaged, fLost, pTemp);
						bSent = sendPacket(pTemp, bGremlin); 
					}
				
				
				}
			
				sendto(fd, "\0", 1, 0, (struct sockaddr*)&serverAddress, slen);

				cout << "Sending Complete!\n";
				putfile.close();
			}
			else
			{
				cout << "Could not open file name.\n";
			}

		}
		close(fd);
    }

    return 0;
}

bool gremlin(float fDamaged, float fLost, Packet* ppacket)
{
	if(fLost > (1.0 * rand()) / (1.0 * RAND_MAX))
	{
		return true;
	}

	if(fDamaged > (1.0 * rand()) / (1.0 * RAND_MAX))
	{
		int numDamaged = rand() % 10;
		int byteNum = rand() % BUFSIZE;
		if(numDamaged == 9)
		{
			if(numDamaged > 1)
			{
				ppacket->Data[numDamaged-HEADERSIZE]+= 8;	
			}
			else if(numDamaged == 1)
			{
				ppacket->Checksum+= 8;
			}
			else
			{
				ppacket->Sequence+= 8;
			}
			
			numDamaged = rand() % 10;
			if(numDamaged > 1)
			{
				ppacket->Data[numDamaged-HEADERSIZE]+= 4;	
			}
			else if(numDamaged == 1)
			{
				ppacket->Checksum+= 4;
			}
			else
			{
				ppacket->Sequence+= 4;
			}

			numDamaged = rand() % 10;
			if(numDamaged > 1)
			{
				ppacket->Data[numDamaged-HEADERSIZE]+= 2;	
			}
			else if(numDamaged == 1)
			{
				ppacket->Checksum+= 2;
			}
			else
			{
				ppacket->Sequence+= 2;
			}
		}
		else if(numDamaged > 6)
		{
			if(numDamaged > 1)
			{
				ppacket->Data[numDamaged-HEADERSIZE]+= 8;	
			}
			else if(numDamaged == 1)
			{
				ppacket->Checksum+= 8;
			}
			else
			{
				ppacket->Sequence+= 8;
			}

			numDamaged = rand() % 10;
			if(numDamaged > 1)
			{
				ppacket->Data[numDamaged-HEADERSIZE]+= 4;	
			}
			else if(numDamaged == 1)
			{
				ppacket->Checksum+= 4;
			}
			else
			{
				ppacket->Sequence+= 4;
			}
		}
		else
		{
			if(numDamaged > 1)
			{
				ppacket->Data[numDamaged-HEADERSIZE]+=8;	
			}
			else if(numDamaged == 1)
			{
				ppacket->Checksum+=4;
			}
			else
			{
				ppacket->Sequence+= 2;
			}
		}

	}
	return false;
}

void usage() {
    cout << "Use the following syntax: \nproject1 -l <lost packets> -d <damaged packets>" << endl;
}

unsigned char generateChecksum( Packet* pPacket ) {
    unsigned char retVal = 0x00;

    retVal = pPacket->Sequence;

    for( int i=0; i < DATASIZE; i++ ) {
        retVal += pPacket->Data[i];
    }

    retVal = ~retVal;

    return retVal;
}

Packet* constructPacket(char* data, int length) {
    Packet* pPacket = new Packet;
    static uint8_t sequenceNum = 0;

    pPacket->Sequence = sequenceNum;

    sequenceNum = 1 - sequenceNum;

    for( int i=0; i < DATASIZE; i++ ) {
        if( i < length )
            pPacket->Data[i] = data[i];
        else
            pPacket->Data[i] = '\0';
    }

    pPacket->Checksum = generateChecksum(pPacket);

    return pPacket;
}


bool sendPacket(const Packet* pPacket, bool bLost) {
    bool bReturn = false;

	//Send packet
	if(!bLost)
	{
		if (sendto(fd, pPacket, BUFSIZE, 0, (struct sockaddr*)&serverAddress, slen) == -1) {
			cerr << "Problem sending packet with sequence #" << pPacket->Sequence << "..." << endl;
			bReturn = false;
		} 
	}


	int recvPollVal = 0;
	int iLength = 0;
	struct pollfd ufds;
	unsigned char recvline[MAXLINE + 1];
	time_t timer;

	//Wait on return
	ufds.fd = fd;
	ufds.events = POLLIN;
	recvPollVal = poll(&ufds, 1, 20);

	if( recvPollVal == -1 ) {            //If error occurs
		cerr << "Error polling socket..." << endl;
		bReturn = false;
	} else if( recvPollVal == 0 ) {        //If timeout occurs
		cerr << "Timeout... Lost Packet, Sequence Number - "  << (int)pPacket->Sequence << endl;
		bReturn = false;
	} else {
		iLength = recvfrom(fd, recvline, MAXLINE, 0, (struct sockaddr*)&serverAddress, &slt);

		if( recvline[0] == '1') {          //If ACK Received, return true
			cout << "ACK - " << (int)recvline[1] << endl;
			//cout << pPacket->Data << endl;
		    bReturn = true;
		} else if( recvline[0] == '0' ) {     //Else if NAK, return false
			cout << "NAK - " << (int)recvline[1] << endl;
		    bReturn = false;
		} else {                             //Else bad stuff, return false
		    bReturn = false;
		}
	}
  
	return bReturn;
}

