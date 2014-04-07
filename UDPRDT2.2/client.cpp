/* client.cpp

    This program transfers a file to a remote server, possibly through an
    intermediary relay.  The server will respond with an acknowledgement.
    
    This program then calculates the round-trip time from the start of
    transmission to receipt of the acknowledgement, and reports that along
    with the average overall transmission rate.
    
    Written by Giric Goyal for CS 450 HW2 March 2014
*/

#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctime>
#include <string.h>
#include <unistd.h>

#include "CS450Header.h"
#include "450UtilsUDP.h"

using namespace std;


int main(int argc, char *argv[])
{
    // Variables
    string c_destinationIP = "127.0.0.1";
    uint32_t c_destinationPortNumber = 54321;
    string c_relay = "";
    uint32_t c_relayPortNumber = 54320;
    uint32_t c_clientPortNumber = 54329;
    string c_fileName = "";
    int c_fd;

    struct stat c_st;
    void *c_addr = NULL;

    int c_udpSocket = 0;

    sockaddr_in c_serverAddr;       // server address structure
    sockaddr_in c_relayAddr;        // relay address structure
    sockaddr_in c_clientAddr;       // client address structure

    struct hostent *c_hp;
    struct hostent *c_rp;

    time_t c_startTime, c_endTime;
    double c_totalTime;

    char c_myAddress[ 100 ];

    CS450Header c_header;
    
    std::cout<<"\nAuthor\t:\tGiric Goyal\nID\t:\tggoyal2\nUIN\t:\t657440995\n\n";

    char c_sendMore = 'n';
    char c_saveFile = 'n';

    long int c_totalSent = 0;
    long int c_totalReceived = 0;
    long int c_totalTimeElapsed = 0;
    string c_allFiles = "";

    int32_t transactionNumber = 0;
    int32_t sequenceNumber = 0;
    int32_t acknowledgement = 1;
    uint32_t nbytes = 0;
    uint32_t nTotalBytes = 0;
    uint16_t checksum = 0;
    int packetType = 1;
    int relayCommand = 0;
    int saveFile = 0;
    int dropChance = 0;
    int dupeChance = 0;
    int garbleChance = 0; 
    int recvLen = -1;   

    Packet *c_packet = (Packet *)malloc(4096);


    //int c_logFile;
   // string c_log = "";

    // User Input
    
    /* Check for the following from command-line args, or ask the user:
        
        Destination ( server ) name and port number
        Relay name and port number.  If none, communicate directly with server
        File to transfer.  Use OPEN(2) to verify that the file can be opened
        for reading, and ask again if necessary.
    */

    if (argc == 1) {
        std::cout<<"\nUse the command line arguments to provide the destination ip and port number.";
        std::cout<<"\n\n./client [destination-ip] [destination-port] [relay-ip] [relay-port] [client-port]";
        std::cout<<"\n\nThe first argument [destination-ip] is mandatory. Rest of the values are used default if omitted.";
        std::cout<<"\nDefault destination port: 54321";
        std::cout<<"\nDefault relay: none";
        std::cout<<"\nDefault relay port: 54320";
        std::cout<<"\nDefault client port: 54329\n\n\n";
        return 0;
    }

    else {
        if (argc == 2) {
            c_destinationIP = argv[1];
        }
        else if (argc == 3) {
            c_destinationIP = argv[1];
            c_destinationPortNumber = atoi(argv[2]);
        }
        else if (argc == 4) {
            c_destinationIP = argv[1];
            c_destinationPortNumber = atoi(argv[2]);
            c_relay = argv[3];
        }
        else if (argc == 5) {
            c_destinationIP = argv[1];
            c_destinationPortNumber = atoi(argv[2]);
            c_relay = argv[3];
            c_relayPortNumber = atoi(argv[4]);
        }
        else if (argc == 6) {
            c_destinationIP = argv[1];
            c_destinationPortNumber = atoi(argv[2]);
            c_relay = argv[3];
            c_relayPortNumber = atoi(argv[4]);
            c_clientPortNumber = atoi(argv[5]);
        }
    }

    do {

        c_totalReceived = 0;
        c_totalSent = 0;

        std::cout<<"Using: \nDestination IP : "<<c_destinationIP<<"\tPort Number : "<<c_destinationPortNumber<<endl;
        std::cout<<"Relay : "<<c_relay<<"\tPort Number : "<<c_relayPortNumber<<endl;
        std::cout<<"Client Port Number : "<<c_clientPortNumber<<endl;

        // File to transfer
        std::cout<<"Enter file name with path : ";
        std::cin>>c_fileName;
        
        while ((c_fd = open(c_fileName.c_str(), O_RDONLY)) < 0) {
            perror("Error Reading File.");
            std::cout<<"Enter file name again : ";
            std::cin>>c_fileName;
        }
        std::cout<<"File Read Successful"<<endl;
        
        // Use FSTAT and MMAP to map the file to a memory buffer.  That will let the
        // virtual memory system page it in as needed instead of reading it byte
        // by byte.
        if (fstat(c_fd, &c_st) < 0) {
            perror("Error getting file stats.");
        }

        c_addr = mmap(NULL, c_st.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, c_fd, 0);
        close(c_fd);

        if (c_addr == MAP_FAILED) {
            perror("Error mapping file to memory. Exiting");
            return 0;
        }
        else {
            std::cout<<"File mapped to memory."<<endl;
        }

        std::cout<<"Save File? (y/n) : ";
        std::cin>>c_saveFile;

        std::cout<<"Send more after this file? (y/n) : ";
        std::cin>>c_sendMore;

        // Create a socket
        if ((c_udpSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            perror("Cannot create socket. Exiting");
            return 0;
        }
        else {
            std::cout<<"Socket created. "<<endl;
        }

        // get the name of the host
        gethostname( c_myAddress, 100 );
        if (!gethostbyname(c_myAddress)) {
            perror("Error finding host address. Exiting.");
            return 0;
        }

        // fill in struct sockaddr
        c_clientAddr.sin_family = AF_INET;
        c_clientAddr.sin_port = htons(c_clientPortNumber);
        memcpy((void *)&c_clientAddr.sin_addr, gethostbyname(c_myAddress)->h_addr_list[0], gethostbyname(c_myAddress)->h_length);
        

        // bind to the socket
        if (bind(c_udpSocket, (sockaddr *)&c_clientAddr, sizeof(c_serverAddr)) < 0) {
            perror("Error binding to port. Exiting");
            return 0;
        }
        else {
            std::cout<<"Socket Bound."<<endl;
        }

        // get the name of the server
        c_hp = gethostbyname(c_destinationIP.c_str());
        if (!c_hp) {
            perror("Error finding host address. Exiting.");
            return 0;
        }

        // fill in server sockaddr struct
        c_serverAddr.sin_family = AF_INET;
        c_serverAddr.sin_port = htons(c_destinationPortNumber);
        /* put the host's address into the server address structure */
        memcpy((void *)&c_serverAddr.sin_addr, c_hp->h_addr_list[0], c_hp->h_length);
        
        // if relay specified
        if (c_relay != "" && strcmp(c_relay.c_str(),"none") != 0) {
            // get the name of the relay
            c_rp = gethostbyname(c_relay.c_str());
            if (!c_rp) {
                perror("Error finding relay address.");
            }

            // fill in relay sockaddr struct
            c_relayAddr.sin_family = AF_INET;
            c_relayAddr.sin_port = htons(c_relayPortNumber);
            memcpy((void *)&c_relayAddr.sin_addr, c_rp->h_addr_list[0], c_rp->h_length);
            

        }

        // Note the time before beginning transmission
        c_startTime = time(0);

        // Create a CS450Header object, fill in the fields, and use SEND to write
        // it to the socket.

        transactionNumber += 1;

        int c_startIndex = 0;
        int c_endIndex = 0;
        char *c_tempString = (char*)malloc(BLOCKSIZE);
        bool sendNext = true;

        c_totalReceived = 0;
        
        while (c_endIndex <= c_st.st_size) {
            if (sendNext) {
                if (c_endIndex + BLOCKSIZE <= c_st.st_size) {
                    c_endIndex = c_endIndex + BLOCKSIZE;
                }
                else {
                    c_endIndex = c_st.st_size;
                    free(c_tempString);
                    c_tempString = (char*) malloc (c_endIndex - c_startIndex);
                } 
                strncpy(c_tempString, (char*)c_addr + c_startIndex, c_endIndex - c_startIndex);
            }

            nbytes = c_endIndex - c_startIndex;
            nTotalBytes = c_st.st_size;
            packetType = 1;
            relayCommand = 0;
            checksum = 0;

            saveFile = 0;
            if (c_saveFile == 'Y' || c_saveFile == 'y') {
                saveFile = 1;
            }

            dropChance = 0;
            dupeChance = 0;
            garbleChance = 50;

            packHeader(c_header, htonl(5) /*version*/, htonl(657440995), htonl(transactionNumber),
                htonl(sequenceNumber), htonl(acknowledgement), c_clientAddr.sin_addr.s_addr, c_serverAddr.sin_addr.s_addr,
                c_clientAddr.sin_addr.s_addr, c_serverAddr.sin_addr.s_addr, htonl(nbytes), htonl(nTotalBytes), 
                c_fileName.c_str(), c_clientAddr.sin_port, c_serverAddr.sin_port, htons(checksum), 2 /*HW no*/, packetType, 
                relayCommand,  saveFile, "ggoyal2", dropChance, dupeChance, garbleChance, 22 /*version*/);
           
            c_packet->header = c_header;
            strcpy(c_packet->data,c_tempString);

            checksum = calcChecksum((void*)c_packet, nbytes + 512);
            c_packet->header.checksum = htons(checksum);

            int sent = 0;

            if (c_relay != "" && strcmp(c_relay.c_str(),"none") != 0) {
                sent = sendto(c_udpSocket, c_packet, 4096, 0, (sockaddr *)&c_relayAddr, sizeof(c_relayAddr));
            }
            else {
                sent = sendto(c_udpSocket, c_packet, 4096, 0, (sockaddr *)&c_serverAddr, sizeof(c_relayAddr));
            }            
            
            if (sent < 0) {
                perror("Error sending Packet.");
            }
            else {
                std::cout<<"Packet Seq : "<<ntohl(c_packet->header.sequenceNumber)<<" sent"<<endl;
                socklen_t sockLen = sizeof(c_relayAddr);
                c_totalSent += 4096;
                
                if (c_relay != "" && strcmp(c_relay.c_str(),"none") != 0) {
                    recvLen = recvfrom(c_udpSocket, c_packet, 4096, 0, (sockaddr *)&c_relayAddr, &sockLen);
                }
                else {
                    recvLen = recvfrom(c_udpSocket, c_packet, 4096, 0, (sockaddr *)&c_serverAddr, &sockLen);
                }

                if (recvLen <= 0) {
                    perror("Error receiving Ack.");
                }
                else {
                    c_totalReceived += recvLen;
                    std::cout<<"Packet Ack : "<<ntohl(c_packet->header.ackNumber)<<" received"<<endl;
                    if (ntohl(c_packet->header.ackNumber) == ntohl(c_packet->header.sequenceNumber)) {
                        sequenceNumber = (sequenceNumber == 1)? 0 : 1;
                        acknowledgement = ntohl(c_packet->header.ackNumber);
                    }

                    if ((sequenceNumber - acknowledgement) != 0) {
                        sendNext = true;
                    }
                    else {
                        sendNext = false;
                    }

                    if (sendNext) {
                        if (c_startIndex + BLOCKSIZE <= c_st.st_size) {
                            c_startIndex = c_startIndex + BLOCKSIZE;
                        }
                        else {
                            c_startIndex = c_st.st_size - c_startIndex;
                        }
                    }
                    if (c_endIndex == c_st.st_size) {
                        break;
                    }
                }
            }
        }

        c_endTime = time(0);
        c_totalTime = difftime(c_endTime, c_startTime);

        c_totalTimeElapsed += c_totalTime;
        
        std::cout<<"----------Stats Report----------\n";
        std::cout<<"File sent\t\t:\t"<<c_allFiles<<endl;
        std::cout<<"Data sent by Client\t:\t"<<c_totalSent<<" bytes"<<endl;
        std::cout<<"Data received by Server\t:\t"<<c_totalReceived<<" bytes"<<endl;
        std::cout<<"Round trip time\t\t:\t"<<c_totalTimeElapsed<<" secs"<<endl;
        std::cout<<"--------------------------------\n";

        
        close(c_udpSocket);


        if (c_sendMore == 'y' || c_sendMore == 'Y') {
            std::cout<<"\nEnter destination IP and Port Number : ";
            std::cin>>c_destinationIP>>c_destinationPortNumber;
            std::cout<<"Enter relay IP and Port Number : ";
            std::cin>>c_relay>>c_relayPortNumber;
            std::cout<<"Enter client Port Number : ";
            std::cin>>c_clientPortNumber;
        }

    }while(c_sendMore == 'y' || c_sendMore == 'Y');

    return EXIT_SUCCESS;
}
