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
    int s_fd;
    
    char myAddress[ 100 ];

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
        std::cout<<"Socket bound to port number : "<<s_portNumber<<" @ : "<<myAddress<<endl;
    }

    //  Call LISTEN to set the socket to listening for new connection requests.
    if (listen(s_tcpSocket, 5) < 0) {
        perror("Listen failed.");
    }
    
    // For HW1 only handle one connection at a time
    
    // Call ACCEPT to accept a new incoming connection request.
    // The result will be a new socket, which will be used for all further
    // communications with this client.
    socklen_t socklen;
    std::cout<<"Waiting for connection."<<endl;
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
    s_dataAddr = (char *)malloc(ntohl(s_header->nbytes));
    if (recv(s_acceptSocket, s_dataAddr, ntohl(s_header->nbytes), 0) < 0) {
        perror("Error receiving message.");
    }
    else {
        std::cout<<"File received."<<endl;
    }

     if (ntohs(s_header->saveFile) != 0) {
        s_fd = creat(s_header->filename, S_IRWXU);
        if (s_fd < 0) {
            perror("Error creating file.");
        }
        else {
            if (write(s_fd, s_dataAddr, ntohl(s_header->nbytes)) < 0) {
                perror("Error writing file.");
            }
            else {
                std::cout<<"File Saved."<<endl;
            }
        }
    }

    // Send back an acknowledgement to the client, indicating the number of 
    // bytes received.  Other information may also be included.

    struct stat s_st;
    if (stat(s_dataAddr, &s_st) <0) {
        perror ("Error getting file stats.");
    }
    cout<<endl<<s_st.st_size<<endl;

    s_header->from_IP = 0;
    s_header->to_IP = 0;
    s_header->packetType = htonl(2);
    s_header->nbytes = htonl(strlen(s_dataAddr));
    s_header->relayCommand = htonl(1);
    s_header->persistent = htonl(0);
    s_header->saveFile = htonl(0);

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
    
    shutdown(s_tcpSocket, SHUT_RDWR);
    //free(s_hp);
    free(s_header);
    free(s_dataAddr);
    //system("PAUSE");
    return EXIT_SUCCESS;
}
