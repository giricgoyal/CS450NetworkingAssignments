/*  server.cpp

    This program receives a file from a remote client and sends back an 
    acknowledgement after the entire file has been received.
    
    Two possible options for closing the connection:
        (1) Send a close command along with the acknowledgement and then close.
        (2) Hold the connection open waiting for additional files / commands.
        The "persistent" field in the header indicates the client's preference.
        
    Written by Giric Goyal January 2014 for CS 450
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

#include "include/CS450Header.h"

using namespace std;

int main(int argc, char *argv[])
{
    // variables
    int s_portNumber = 54321;       // port number
    int s_tcpSocket = 0;            // socket
    int s_acceptSocket = 0;         // New Socket for data transfer

    sockaddr_in s_serverAddr;       // server address structure
    sockaddr_in s_clientAddr;       // client address structure

    struct hostent *s_hp;

    CS450Header *s_header = (CS450Header *)malloc(sizeof(CS450Header));

    char *s_dataAddr = NULL;
    char *s_tempAddr = NULL;

    socklen_t socklen;

    int s_fd;
    
    char myAddress[ 100 ];

    long int s_tempAddress;
    long int s_tempPort;

    std::cout<<"\nAuthor\t:\tGiric Goyal\nID\t:\tggoyal2\nUIN\t:\t657440995\n\n";

    // User Input 
    /* Check for the following from command-line args, or ask the user:
        
        Port number to listen to.  Default = 54321.
    */

    // if not specified port number as argument, use default 54321
    if (argc != 1) {
        s_portNumber = atoi(argv[1]);     // convert char* to int
        std::cout<<"Port Number : "<<s_portNumber<<endl;
    }
    else {
        std::cout<<"No port number specied. Using default 54321."<<endl;
    }

    //  Call SOCKET to create a listening socket
    if ((s_tcpSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
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

    if (bind(s_tcpSocket, (sockaddr *)&s_serverAddr, sizeof(s_serverAddr)) < 0) {
        perror("Cannot bind to socket:");
        return 0;
    }
    else {
        std::cout<<"Socket bound to port number : "<<s_portNumber<<" @ "<<myAddress<<endl;
    }

    //  Call LISTEN to set the socket to listening for new connection requests.
    if (listen(s_tcpSocket, 5) < 0) {
        perror("Listen failed.");
    }
    
    // For HW1 only handle one connection at a time
    
    // Call ACCEPT to accept a new incoming connection request.
    // The result will be a new socket, which will be used for all further
    // communications with this client.
    
    do {
        std::cout<<"Waiting for connection.\n"<<endl;
        if ((s_acceptSocket = accept(s_tcpSocket, (sockaddr *)&s_clientAddr, &socklen)) < 0) {
            perror("Failed to accept incoming connection.");
        }
        else {
            std::cout<<"Connected to client."<<endl;
        }

        // Call RECV to read in one CS450Header struct
        if (recv(s_acceptSocket, s_header, sizeof(CS450Header), 0) < 0) {
            perror("Error receiving message.");
        }
        else {
            std::cout<<"Header received."<<endl;
        }

        // Then call RECV again to read in the bytes of the incoming file.
        //      If "saveFile" is non-zero, save the file to disk under the name
        //      "filename".  Otherwise just read in the bytes and discard them.
        //      If the total # of bytes exceeds a limit, multiple RECVs are needed.
        
        if (s_dataAddr == NULL) {
            s_dataAddr = (char *)malloc(ntohl(s_header->nbytes));    
        }
        else {
            free(s_dataAddr);
            s_dataAddr = (char *)malloc(ntohl(s_header->nbytes));
        }
        if (s_tempAddr == NULL) {
            s_tempAddr = (char *)malloc(65536);    
        }
        else  {
            free(s_tempAddr);
            s_tempAddr = (char *)malloc(65536);
        }
        
        long int s_size = 0;
        long int s_tSize = 0;
        long int s_totalSize  = ntohl(s_header->nbytes);
        
        while (true) {
            if (s_totalSize - s_size >= 65536) {
                s_tSize = 65536;
            }
            else {
                s_tSize = s_totalSize - s_size;
                free(s_tempAddr);
                s_tempAddr = (char*)malloc(s_tSize);
            }
            if (recv(s_acceptSocket, s_tempAddr, s_tSize, MSG_WAITALL) < 0) {
                perror("Error getting file.");
            }
            else {
                //std::cout<<s_tSize<< " : "<<s_size<<" : "<<s_totalSize<<endl;
                strcat(s_dataAddr, s_tempAddr);
                s_size = s_size + s_tSize;
            }

            if (s_size == s_totalSize) {
                break;
            }
        }
        /*
        int s_bytesRead = 1;
        int s_counter = 0;
        while (s_bytesRead > 0) {
            if (s_totalSize - s_size >= 65536) {
                s_tSize = 65536;
            }
            else {
                s_tSize = s_totalSize - s_size;
                free(s_tempAddr);
                s_tempAddr = (char*)malloc(s_tSize);
            }
            if ((s_bytesRead = recv(s_acceptSocket, s_tempAddr, s_tSize, 0)) < 0) {
                perror("Error getting file.");
            }
            else {
                cout<<s_bytesRead<<" : " <<++s_counter<<endl;
                //std::cout<<s_tSize<< " : "<<s_size<<" : "<<s_totalSize<<endl;
                strcat(s_dataAddr, s_tempAddr);
                s_size = s_size + s_tSize;
            }

            if (s_bytesRead == 0) {
                break;
            }
        }
        */
    
        /*
        if (recv(s_acceptSocket, s_dataAddr, ntohl(s_header->nbytes), 0) < 0) {
            perror("Error receiving message.");
        }
        else {
            std::cout<<"File received."<<endl;
        }
        */

        if (ntohl(s_header->saveFile) != 0) {
            s_fd = creat("testnew.txt"/*s_header->filename*/, S_IRWXU);
            if (s_fd < 0) {
                perror("Error creating file.");
            }
            else {
                if (write(s_fd, s_dataAddr, ntohl(s_header->nbytes)) < 0) {
                    perror("Error writing file.");
                }
                else {
                    std::cout<<"File Saved."<<endl;
                    close(s_fd);
                }
            }
        }

        // Send back an acknowledgement to the client, indicating the number of 
        // bytes received.  Other information may also be included.

        s_tempAddress = s_header->from_IP;
        s_header->from_IP = s_header->to_IP;
        s_header->to_IP = s_tempAddress;

        //s_header->from_IP = s_serverAddr.sin_addr.s_addr;
        //s_header->to_IP = s_clientAddr.sin_addr.s_addr;
        
        s_header->packetType = htonl(2);
        s_header->nbytes = htonl(s_size);
        s_header->relayCommand = (ntohl(s_header->persistent) == 0)? htonl(1) : htonl(0);
        s_header->persistent = htonl(0);
        s_header->saveFile = htonl(0);

        s_tempAddress = s_header->trueFromIP;
        s_header->trueFromIP = s_header->trueToIP;
        s_header->trueToIP = s_tempAddress;

        //s_header->trueFromIP = s_serverAddr.sin_addr.s_addr;
        //s_header->trueToIP = s_clientAddr.sin_addr.s_addr;

        s_tempPort = s_header->from_Port;
        s_header->from_Port = s_header->to_Port;
        s_header->to_Port = s_tempPort;
        //s_header->from_Port = htons(s_portNumber);
        //s_header->to_Port = htons(s_portNumber);


        // send the header
        if (send(s_acceptSocket, s_header, sizeof(CS450Header), 0) < 0) {
            perror("Error sending acknowledgement header.");
        }
        else {
            std::cout<<"Acknowledgement sent."<<endl;
        }

        // If "persistent" is zero, then include a close command in the header
        // for the acknowledgement and close the socket.  Go back to ACCEPT to 
        // handle additional requests.  Otherwise keep the connection open and
        // read in another Header for another incoming file from this client.
        
    }while(ntohl(s_header->relayCommand) == 0);

    shutdown(s_tcpSocket, SHUT_RDWR);
    //free(s_hp);
    free(s_header);
    free(s_dataAddr);
    free(s_tempAddr);
    //system("PAUSE");
    return EXIT_SUCCESS;
}
