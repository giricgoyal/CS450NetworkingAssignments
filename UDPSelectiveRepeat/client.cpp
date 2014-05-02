/* client.cpp

    This program transfers a file to a remote server, possibly through an
    intermediary relay.  The server will respond with an acknowledgement.
    
    This program then calculates the round-trip time from the start of
    transmission to receipt of the acknowledgement, and reports that along
    with the average overall transmission rate.
    
    Written by Giric Goyal for CS 450 HW4 April 2014
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
#include <signal.h>
#include <math.h>
#include <list>


#include "CS450Header.h"
#include "450UtilsUDP.h"

using namespace std;

bool c_timerFlag = false;

void timeHandler(int sig) {
    c_timerFlag = true;
}

int main(int argc, char *argv[])
{
    // Variables
    string c_destinationIP = "127.0.0.1";
    uint32_t c_destinationPortNumber = 54323;
    string c_relay = "none";
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

    int32_t transactionNumber = -1;
    uint32_t nbytes = 0;
    uint32_t nTotalBytes = 0;
    uint16_t checksum = 0;
    int packetType = 1;
    int relayCommand = 0;
    int saveFile = 0;
    int dropChance = 0;
    int dupeChance = 0;
    int garbleChance = 0;
    int delayChance = 0; 
    int recvLen = -1;   

    Packet *c_packet = (Packet *)malloc(4096);


    int c_windowSize = 8;
    int c_basePointer = 0;
    int c_nextSeqNumber = 0;

    unsigned long c_totalPackets = 0;

    int c_timerValue = 3;

    struct sigaction c_sact;

    fd_set c_readfds;
    struct timeval c_tv;


    sigemptyset(&c_sact.sa_mask);
    c_sact.sa_flags = 0;
    c_sact.sa_handler = timeHandler;
    sigaction(SIGALRM, &c_sact, NULL);


    list<Packet> mymap1;
    list<Packet>::iterator it1;
    list<int> mymap2;
    list<int>::iterator it2;
    
    int tfd = 0;
    char tbuff[100];
    if ((tfd = open("transactionNumber.txt", O_RDONLY)) < 0) {
        perror("Error reading in Transaction Number.");
    }
    else {
        if (read(tfd, tbuff, sizeof(tbuff)) > 0) 
            transactionNumber = atoi(tbuff);
        close(tfd);
    }

    if (argc == 1) {
        std::cout<<"\nUse the command line arguments to provide the destination ip and port number.";
        std::cout<<"\n\n./client [destination-ip] [destination-port] [relay-ip] [relay-port] [client-port] [garble-chance] [window-size]";
        std::cout<<"\n\nThe first argument [destination-ip] is mandatory. Rest of the values are used default if omitted.";
        std::cout<<"\nDefault destination port: "<<c_destinationPortNumber;
        std::cout<<"\nDefault relay: "<<c_relay;
        std::cout<<"\nDefault relay port: "<<c_relayPortNumber;
        std::cout<<"\nDefault client port: "<<c_clientPortNumber;
        std::cout<<"\nDefault Garble Chance: "<<garbleChance;
        std::cout<<"\nDefault Window Size: "<<c_windowSize<<"\n\n\n";
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
        else if (argc == 7) {
            c_destinationIP = argv[1];
            c_destinationPortNumber = atoi(argv[2]);
            c_relay = argv[3];
            c_relayPortNumber = atoi(argv[4]);
            c_clientPortNumber = atoi(argv[5]);
            garbleChance = atoi(argv[6]);
        }
        else if (argc == 8) {
            c_destinationIP = argv[1];
            c_destinationPortNumber = atoi(argv[2]);
            c_relay = argv[3];
            c_relayPortNumber = atoi(argv[4]);
            c_clientPortNumber = atoi(argv[5]);
            garbleChance = atoi(argv[6]);
            c_windowSize = atoi(argv[7]);
        }
    }

    do {

        std::cout<<"Transaction No : "<<transactionNumber<<endl;

        dupeChance = delayChance = dropChance = garbleChance;

        std::cout<<"Using: \nDestination IP : "<<c_destinationIP<<"\tPort Number : "<<c_destinationPortNumber<<endl;
        std::cout<<"Relay : "<<c_relay<<"\tPort Number : "<<c_relayPortNumber<<endl;
        std::cout<<"Client Port Number : "<<c_clientPortNumber<<endl;
        std::cout<<"Window Size : "<<c_windowSize<<endl;
        std::cout<<"Garble Chance : "<<garbleChance<<endl;
        std::cout<<"Dupe Chance : "<<dupeChance<<endl;
        std::cout<<"Drop Chance : "<<dropChance<<endl;
        std::cout<<"Delay Chance : "<<delayChance<<endl;

        // File to transfer
        std::cout<<"\nEnter file name with path : ";
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
            //fcntl(c_udpSocket, F_SETFL, O_NONBLOCK);
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
        if (bind(c_udpSocket, (sockaddr *)&c_clientAddr, sizeof(c_clientAddr)) < 0) {
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
                return 0;
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

        char *c_tempString = (char*)malloc(BLOCKSIZE);
        
        c_totalReceived = 0;


        c_basePointer = 1;
        c_nextSeqNumber = 1;

        c_totalReceived = 0;
        c_totalSent = 0;
        
        c_totalPackets = int(ceil(float(c_st.st_size)/float(BLOCKSIZE)));

        std::cout<<"\nStarting sending process ... \n\n";


        FD_ZERO(&c_readfds);
        FD_SET(c_udpSocket, &c_readfds);

        c_tv.tv_sec = 1;
        c_tv.tv_usec = 0;


        while (true) {
            FD_ZERO(&c_readfds);
            FD_SET(c_udpSocket, &c_readfds);

            c_tv.tv_sec = 1;
            c_tv.tv_usec = 0;

            int dataSize = BLOCKSIZE;

            // send packet with next sequence number
            if (c_nextSeqNumber < c_basePointer + c_windowSize && c_nextSeqNumber <= c_totalPackets) {


                if (c_nextSeqNumber == c_totalPackets) {
                    dataSize = c_st.st_size - ((c_nextSeqNumber - 1) * BLOCKSIZE);
                }
                 
                strncpy(c_tempString, (char*)c_addr + ((c_nextSeqNumber - 1) * BLOCKSIZE), dataSize);

                nbytes = dataSize;
                nTotalBytes = c_st.st_size;
                packetType = 1;
                relayCommand = 0;
                checksum = 0;

                saveFile = 0;
                if (c_saveFile == 'Y' || c_saveFile == 'y') {
                    saveFile = 1;
                }

                dropChance = dupeChance = delayChance = garbleChance;
                
                //cout<<c_nextSeqNumber<<"\t"<<c_basePointer<<"\t"<<c_windowSize<<endl;
                
                // create packet
                packHeader(c_header, htonl(7) /*version*/, htonl(657440995), htonl(transactionNumber),
                htonl(c_nextSeqNumber), htonl(0), c_clientAddr.sin_addr.s_addr, c_serverAddr.sin_addr.s_addr,
                c_clientAddr.sin_addr.s_addr, c_serverAddr.sin_addr.s_addr, htonl(nbytes), htonl(nTotalBytes), 
                c_fileName.c_str(), c_clientAddr.sin_port, c_serverAddr.sin_port, htons(checksum), 4 /*HW no*/, packetType, 
                relayCommand,  saveFile, "ggoyal2", dropChance, dupeChance, garbleChance, delayChance, c_windowSize, 32 /*version*/);
           
                c_packet->header = c_header;
                strcpy(c_packet->data,c_tempString);

                checksum = calcChecksum((void*)c_packet, nbytes + 512);
                c_packet->header.checksum = htons(checksum);

                // send packet

                int sent = 0;
               
                if (c_relay != "" && strcmp(c_relay.c_str(),"none") != 0) {
                    sent = sendto(c_udpSocket, c_packet, 4096, 0, (sockaddr *)&c_relayAddr, sizeof(c_relayAddr));
                }
                else {
                    sent = sendto(c_udpSocket, c_packet, 4096, 0, (sockaddr *)&c_serverAddr, sizeof(c_serverAddr));
                }            

                if (sent < 0) {
                    perror("Error sending Packet.");
                    return(0);
                }
                else {
                    std::cout<<"Packet with seq number : "<<c_nextSeqNumber<<" sent.\n";
                    c_totalSent += sent;
                    mymap1.push_back(*c_packet);
                    mymap2.push_back(time(0) - c_startTime);
                }

                if (c_nextSeqNumber == 1) {
                    alarm(c_timerValue);
                }

                // check for timers
                if (c_basePointer == c_nextSeqNumber) {
                    // start timer
                    alarm(c_timerValue);
                }

                // increment next seq number
                c_nextSeqNumber ++;

            }

            
            // check for incoming packet
            int c_retVal = select(c_udpSocket + 1, &c_readfds, NULL, NULL, &c_tv);
            if (c_retVal == -1) {
                perror("Select Error.");
                //return 0;
            }
            else if (c_retVal > 0) {
                socklen_t sockLen = sizeof(c_clientAddr);
                if (c_relay != "" && strcmp(c_relay.c_str(),"none") != 0) {
                    recvLen = recvfrom(c_udpSocket, c_packet, 4096, 0, (sockaddr *)&c_relayAddr, &sockLen);
                }
                else {
                    recvLen = recvfrom(c_udpSocket, c_packet, 4096, 0, (sockaddr *)&c_serverAddr, &sockLen);
                }

                if (recvLen < 0) {
                    perror("Error receiving Ack.");
                }
                else {
                    c_packet->header.checksum = ntohs(c_packet->header.checksum);
                    checksum = calcChecksum((void*)c_packet, ntohl(c_packet->header.nbytes) + 512);
                    c_packet->header.checksum = htons(c_packet->header.checksum);
                    
                    c_totalReceived += recvLen;

                    if (checksum != 0) {
                        // if corrupt, do nothing, ignore
                        std::cout<<"Corrupt packet received."<<endl;
                    }
                    else if (checksum == 0 && ntohl(c_packet->header.ackNumber) < c_basePointer + c_windowSize) {

                        std::cout<<"\nAck number : "<<ntohl(c_packet->header.ackNumber)<<" received.\n";

                        int t = 0;
                        for (it1 = mymap1.begin(), it2 = mymap2.begin() ; it1 != mymap1.end(); ++it1, ++it2) {
                            Packet c_tempPacket = *it1;
                            //cout<<"1 : "<<ntohl(c_tempPacket->header.sequenceNumber)<<endl;
                            if (ntohl(c_tempPacket.header.sequenceNumber) == ntohl(c_packet->header.ackNumber)) {
                                mymap1.erase(it1);
                                t = *it2;
                                mymap2.erase(it2);
                                break;
                            }
                        }

                        if (c_basePointer == c_nextSeqNumber) {
                            // stop timer
                            alarm(0);
                        }

                        if (ntohl(c_packet->header.ackNumber) == c_basePointer){
                            if (!mymap1.empty()) {
                                int min = c_totalPackets + 1;
                                for (it1 = mymap1.begin(), it2 = mymap2.begin() ; it1 != mymap1.end(); ++it1, ++it2) {
                                    Packet c_packet = *it1;
                                    //cout<<"2 : "<<ntohl(c_packet->header.sequenceNumber)<<endl;
                                    if (ntohl(c_packet.header.sequenceNumber) < min) {
                                        min = ntohl(c_packet.header.sequenceNumber);
                                    }
                                }
                                if (min != c_totalPackets + 1) {
                                    c_basePointer = min;    
                                }
                                cout<<"Ack timer: "<<mymap2.front() - t<<endl;
                                alarm(abs(mymap2.front() - t) == 0? 1 : abs(mymap2.front() - t));
                            }
                            else {
                                c_basePointer = c_nextSeqNumber;
                                alarm(c_timerValue);
                            }
                        }
                        //cout<<ntohl(c_packet->header.ackNumber)<<" : "<<c_basePointer<<" : "<<c_nextSeqNumber<<endl;
                    }
                }
                
            }
            else {
                //perror("nothing to recv.");
            }

            for (it1 = mymap1.begin(), it2 = mymap2.begin() ; it1 != mymap1.end(); ++it1, ++it2) {
                Packet p = *it1;
                cout<<ntohl(p.header.sequenceNumber)<<" : ";
            }
            cout<<endl;
            cout<<"Base: "<<c_basePointer<<" Next: "<<c_nextSeqNumber<<endl;
            

            // timer up? resend packet and set timer for the next packet
            if (c_timerFlag) {

                Packet c_tpacket = mymap1.front();
                mymap1.pop_front();
                int t = mymap2.front();
                mymap2.pop_front();

                mymap1.push_back(c_tpacket);
                mymap2.push_back(time(0) - c_startTime);

                int sent = 0;

                if (c_relay != "" && strcmp(c_relay.c_str(),"none") != 0) {
                    sent = sendto(c_udpSocket, & c_tpacket, 4096, 0, (sockaddr *)&c_relayAddr, sizeof(c_relayAddr));
                }
                else {
                    sent = sendto(c_udpSocket, & c_tpacket, 4096, 0, (sockaddr *)&c_serverAddr, sizeof(c_relayAddr));
                }            
                
                if (sent < 0) {
                    perror("Error sending Packet.");
                    return(0);
                }
                else {
                    std::cout<<"Packet with seq number : "<<ntohl(c_tpacket.header.sequenceNumber)<<" RESENT.\n";
                }
                
                while (mymap2.front() == t) {
                    Packet c_packet = mymap1.front();
                    mymap1.pop_front();
                    mymap2.pop_front();
                    mymap1.push_back(c_packet);
                    mymap2.push_back(time(0) - c_startTime);

                    int sent = 0;

                    if (c_relay != "" && strcmp(c_relay.c_str(),"none") != 0) {
                        sent = sendto(c_udpSocket, &c_packet, 4096, 0, (sockaddr *)&c_relayAddr, sizeof(c_relayAddr));
                    }
                    else {
                        sent = sendto(c_udpSocket, &c_packet, 4096, 0, (sockaddr *)&c_serverAddr, sizeof(c_relayAddr));
                    }            
                    
                    if (sent < 0) {
                        perror("Error sending Packet.");
                        return(0);
                    }
                    else {
                        std::cout<<"Packet with seq number : "<<ntohl(c_packet.header.sequenceNumber)<<" RESENT.\n";
                    }
                    
                }
                
                cout<<"Timer: "<<mymap2.front() - t<<endl;
                alarm(abs(mymap2.front() - t) == 0? 1 : abs(mymap2.front() - t));

                c_timerFlag = false;
            }
            // exit from loop when all sent.
            if (c_basePointer >= c_totalPackets) {
                break;
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
            std::cout<<"Enter Garble Chance : ";
            std::cin>>garbleChance;
            std::cout<<"Enter Window Size : ";
            std::cin>>c_windowSize;
        }

    }while(c_sendMore == 'y' || c_sendMore == 'Y');

    if ((tfd = open("transactionNumber.txt", O_WRONLY)) < 0) {
        perror("Error writing Transaction Number to file.");
    }
    else {
        sprintf(tbuff, "%d", transactionNumber);
        if (write(tfd, tbuff, sizeof(tbuff)) < 0) {
            perror("Error writing Transaction Number to file.");
        }
        close(tfd);
    }


    return EXIT_SUCCESS;
}
