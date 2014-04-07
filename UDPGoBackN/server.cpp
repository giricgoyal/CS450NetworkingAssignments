/*  server.cpp

    This program receives a file from a remote client and sends back an 
    acknowledgement after the entire file has been received.
    
    Two possible options for closing the connection:
        (1) Send a close command along with the acknowledgement and then close.
        (2) Hold the connection open waiting for additional files / commands.
        The "persistent" field in the header indicates the client's preference.
        
    Written by Giric Goyal April 2014 for CS 450 HW 3
*/


#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "CS450Header.h"
#include "450UtilsUDP.h"

using namespace std;

#define RECVBUFFER 4096

int main(int argc, char *argv[])
{
    // variables
    int s_portNumber = 54323;       // port number
    int s_udpSocket = 0;            // socket
    
    sockaddr_in s_serverAddr;       // server address structure
    sockaddr_in s_clientAddr;       // client address structure

    struct hostent *s_hp;

 
    int s_fd;
    
    char myAddress[ 100 ];

    //uint32_t s_tempAddress;
    //uint16_t s_tempPort;
    uint16_t checksum = 0;
   
    int recvLen = -1;
    Packet *recvBuffer = (Packet *)malloc(4096);
    Packet *tempBuffer = (Packet *)malloc(4096);
    
    unsigned int fileSize = 0;
    unsigned int transactionNumber = 0;
    int totalFileReceived = 0;

    int s_expectedSeqnum = 1;
    int s_lastAcknowledged = 0;

    std::cout<<"\nAuthor\t:\tGiric Goyal\nID\t:\tggoyal2\nUIN\t:\t657440995\n\n";

    // User Input 
    /* Check for the following from command-line args, or ask the user:
        
        Port number to listen to.  Default = 54323.
    */

    // if not specified port number as argument, use default 54321
    if (argc != 1) {
        s_portNumber = atoi(argv[1]);     // convert char* to int
        std::cout<<"Port Number : "<<s_portNumber<<endl;
    }
    else {
        std::cout<<"No port number specied. Using default "<<s_portNumber<<endl;
    }

    //  Call SOCKET to create a listening socket
    if ((s_udpSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Cannot create socket.");
        return 0;
    }
    else {
        std::cout<<"Socket created."<<endl;
    }

    //  Call BIND to bind the socket to a particular port number

    gethostname( myAddress, 100 );
    s_serverAddr.sin_family = AF_INET;
    s_serverAddr.sin_port = htons(s_portNumber);


    s_hp = gethostbyname(myAddress);
    if (!s_hp) {
        perror("Error finding host address. Exiting.");
        return 0;
    }
    /* put the host's address into the server address structure */
    memcpy((void *)&s_serverAddr.sin_addr, s_hp->h_addr_list[0], s_hp->h_length);

    if (bind(s_udpSocket, (sockaddr *)&s_serverAddr, sizeof(s_serverAddr)) < 0) {
        perror("Cannot bind to socket:");
        return 0;
    }
    else {
        std::cout<<"Socket bound to port number : "<<s_portNumber<<" @ "<<myAddress<<endl;
    }



    do { 

        std::cout<<"Waiting to receive on port : "<<s_portNumber<<endl;

        socklen_t addrLen = sizeof(s_clientAddr);
        if ((recvLen = recvfrom(s_udpSocket, recvBuffer, 4096, 0, (sockaddr *)&s_clientAddr, &addrLen)) <= 0) {
             perror("No header received");
        }
        else {

            std::cout<<"Received Bytes: "<<recvLen<<endl;

            // create initial ack packet for each new transaction.
            totalFileReceived += recvLen;



            recvBuffer->header.checksum = ntohs(recvBuffer->header.checksum);
            checksum = calcChecksum((void*)recvBuffer, ntohl(recvBuffer->header.nbytes) + 512);
            recvBuffer->header.checksum = htons(recvBuffer->header.checksum);


            uint16_t temp2 = recvBuffer->header.from_Port;
            recvBuffer->header.from_Port = recvBuffer->header.to_Port;
            recvBuffer->header.to_Port = temp2;

            uint32_t temp = recvBuffer->header.from_IP;
            recvBuffer->header.from_IP = recvBuffer->header.to_IP;
            recvBuffer->header.to_IP = temp;

            temp = recvBuffer->header.trueFromIP;
            recvBuffer->header.trueFromIP = recvBuffer->header.trueToIP;
            recvBuffer->header.trueToIP = temp;


            if (checksum == 0 && transactionNumber != ntohl(recvBuffer->header.transactionNumber)) {
                std::cout<<"\nNew transaction! \n";
                transactionNumber = ntohl(recvBuffer->header.transactionNumber);
                fileSize = 0;
                totalFileReceived = 0;
                s_expectedSeqnum = 1;
                s_lastAcknowledged = 0;
                tempBuffer->header.ackNumber = htonl(0);
            }

            // deliver data and send ACK
            if (checksum == 0 && ntohl(recvBuffer->header.sequenceNumber) == s_expectedSeqnum) {

                std::cout<<"Received Packet with seq number : "<<ntohl(recvBuffer->header.sequenceNumber)<<" correctly."<<endl;

                // deliver data
                fileSize += ntohl(recvBuffer->header.nbytes);
                
                if (recvBuffer->header.saveFile != 0) {
                    if (s_lastAcknowledged == 0) {
                        s_fd = creat(recvBuffer->header.filename, S_IRWXU);

                        if (s_fd < 0) {
                            perror("Error creating file.");
                        }
                        else {
                            if (write(s_fd, recvBuffer->data, ntohl(recvBuffer->header.nbytes)) < 0) {
                                perror("Error writing file.");
                            }
                            else {
                                close(s_fd);
                                std::cout<<"File Created!\n";
                            }
                        }
                    }
                    else {
                        s_fd = open(recvBuffer->header.filename, O_APPEND | O_CREAT | O_WRONLY, 0600);

                        if (s_fd < 0) {
                            perror("Error creating file.");
                        }
                        else {
                            if (write(s_fd, recvBuffer->data, ntohl(recvBuffer->header.nbytes)) < 0) {
                                perror("Error writing file.");
                            }
                            else {
                                close(s_fd);
                                std::cout<<"File updated\n";
                            }
                        }
                    }
                }

                // send ACK
                s_lastAcknowledged = ntohl(recvBuffer->header.sequenceNumber);
                recvBuffer->header.ackNumber = recvBuffer->header.sequenceNumber;
                recvBuffer->header.sequenceNumber = htonl(s_expectedSeqnum);
                
                strcpy(recvBuffer->data, "");

                recvBuffer->header.nbytes = htonl(0);

                recvBuffer->header.checksum = htons(0);
                checksum = calcChecksum((void*)recvBuffer, 512);
                recvBuffer->header.checksum = htons(checksum);


                if (sendto(s_udpSocket, recvBuffer, 4096, 0, (sockaddr *)&s_clientAddr, sizeof(s_clientAddr)) < 0) {
                    perror("Sending Ack Failed.");
                }
                else {
                    std::cout<<"Ack : "<<ntohl(recvBuffer->header.ackNumber)<<" sent"<<endl;
                    memcpy((void*)tempBuffer, recvBuffer, 4096);
                }

                // increment expected sequence number
                s_expectedSeqnum += 1;

            }
            else {
                if (checksum != 0) {
                    std::cout<<"Received Corrupt Packet. "<<endl;
                }
                else if (checksum == 0 && ntohl(recvBuffer->header.sequenceNumber) != s_expectedSeqnum) {
                    std::cout<<"Received out of Order Packet. "<<endl;
                }

                if (s_lastAcknowledged != 0) {
                    if (sendto(s_udpSocket, tempBuffer, 4096, 0, (sockaddr *)&s_clientAddr, sizeof(s_clientAddr)) < 0) {
                        perror("Sending Ack Failed.");
                    }
                    else {
                        std::cout<<"Ack : "<<ntohl(tempBuffer->header.ackNumber)<<" RESENT"<<endl;
                    }
                }
            }


            if (fileSize == ntohl(recvBuffer->header.nTotalBytes)) {
                if (recvBuffer->header.saveFile != 0) {
                    std::cout<<"File Saved."<<endl;
                }
                std::cout<<"\nTotal bytes received : "<<totalFileReceived<<endl;
            }
        }
          
    }while(true);
    close(s_udpSocket);  
    //system("PAUSE");
    return EXIT_SUCCESS;
}
