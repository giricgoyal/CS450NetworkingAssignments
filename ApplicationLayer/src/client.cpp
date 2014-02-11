/* client.cpp

    This program transfers a file to a remote server, possibly through an
    intermediary relay.  The server will respond with an acknowledgement.
    
    This program then calculates the round-trip time from the start of
    transmission to receipt of the acknowledgement, and reports that along
    with the average overall transmission rate.
    
    Written by Giric Goyal for CS 450 HW1 January 2014
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

#include "include/CS450Header.h"

using namespace std;


int main(int argc, char *argv[])
{
    // Variables
    string c_destinationIP = "127.0.0.1";
    int c_destinatinPortNumber = 54321;
    string c_relay = "";
    int c_relayPortNumber = 0;
    string c_fileName = "";
    int c_fd;

    struct stat c_st;
    void *c_addr = NULL;

    int c_tcpSocket = 0;

    sockaddr_in c_serverAddr;       // server address structure
    sockaddr_in c_clientAddr;       // client address structure

    struct hostent *c_hp;

    time_t c_startTime, c_endTime;
    double c_totalTime;

    char myAddress[ 100 ];

    CS450Header *c_header = (CS450Header *)malloc(sizeof(CS450Header));
    
    std::cout<<"\nAuthor\t:\tGiric Goyal\nID\t:\tggoyal2\nUIN\t:\t657440995\n\n";


    // User Input
    
    /* Check for the following from command-line args, or ask the user:
        
        Destination ( server ) name and port number
        Relay name and port number.  If none, communicate directly with server
        File to transfer.  Use OPEN(2) to verify that the file can be opened
        for reading, and ask again if necessary.
    */

    if (argc != 1) {
        if (argc == 2) {
            c_destinationIP = argv[1];
        }
        else if (argc == 3) {
            c_destinationIP = argv[1];
            c_destinatinPortNumber = atoi(argv[2]);
        }
        else if (argc == 4) {
            c_destinationIP = argv[1];
            c_destinatinPortNumber = atoi(argv[2]);
            c_relay = argv[3];
        }
        else if (argc == 5) {
            c_destinationIP = argv[1];
            c_destinatinPortNumber = atoi(argv[2]);
            c_relay = argv[3];
            c_relayPortNumber = atoi(argv[4]);
        }
    }
    std::cout<<"Using: \nDestination IP : "<<c_destinationIP<<"\tPort Number : "<<c_destinatinPortNumber<<endl;
    std::cout<<"Relay : "<<c_relay<<"\tPort Number : "<<c_relayPortNumber<<endl;

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

    if (c_addr == MAP_FAILED) {
        perror("Error mapping file to memory. Exiting");
        return 0;
    }
    else {
        std::cout<<"File mapped to memory."<<endl;
    }

    // Open a Connection to the server ( or relay )  TCP for the first HW
    // call SOCKET and CONNECT and verify that the connection opened.

    if ((c_tcpSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Cannot create socket. Exiting");
        return 0;
    }
    else {
        std::cout<<"Socket created."<<endl;
    }

    c_hp = gethostbyname(c_destinationIP.c_str());
    if (!c_hp) {
        perror("Error finding host address. Exiting.");
        return 0;
    }
    c_serverAddr.sin_family = AF_INET;
    c_serverAddr.sin_port = htons(c_destinatinPortNumber);
    /* put the host's address into the server address structure */
    memcpy((void *)&c_serverAddr.sin_addr, c_hp->h_addr_list[0], c_hp->h_length);
    
    if (connect(c_tcpSocket, (sockaddr *)&c_serverAddr, sizeof(c_serverAddr)) < 0) {
        perror("Error connecting to server. Exiting");
        return 0;
    }
    else {
        std::cout<<"Connected to server."<<endl;
    }
    // Note the time before beginning transmission
    c_startTime = time(0);

    // Create a CS450Header object, fill in the fields, and use SEND to write
    // it to the socket.

    gethostname( myAddress, 100 );
    if (!gethostbyname(myAddress)) {
        perror("Error finding host address. Exiting.");
        return 0;
    }
    
    c_header->version = htonl(2);
    c_header->UIN = htonl(657440995);
    c_header->HW_number = htonl(1);
    c_header->transactionNumber = htons(1);
    strcpy(c_header->ACCC,"ggoyal2");
    strcpy(c_header->filename,c_fileName.c_str());
    c_header->from_IP = htonl(gethostbyname(myAddress)->h_addr_list[0]);
    c_header->to_IP = htonl(c_serverAddr.sin_addr.s_addr);;
    c_header->packetType = htonl(1);
    c_header->nbytes = htonl(c_st.st_size);
    c_header->relayCommand = htonl(0);
    c_header->persistent = htonl(0);
    c_header->saveFile = htonl(0);

    // send the header
    if (send(c_tcpSocket, c_header, sizeof(CS450Header), 0) < 0) {
        perror("Error sending message.");
    }
    else {
        std::cout<<"Header sent."<<endl;
    }

    // Use SEND to send the data file.  If it is too big to send in one gulp
    // Then multiple SEND commands may be necessary.
    if (send(c_tcpSocket, c_addr, c_st.st_size, 0) < 0) {
        perror("Error sending message.");
    }
    else {
        std::cout<<"File sent."<<endl;
    }
    
    // Use RECV to read in a CS450Header from the server, which should contain
    // some acknowledgement information.  
    
    // Calculate the round-trip time and
    // bandwidth, and report them to the screen along with the size of the file
    // and output echo of all user input data.
    
    // When the job is done, either the client or the server must transmit a
    // CS450Header containing the command code to close the connection, and 
    // having transmitted it, close their end of the socket.  If a relay is 
    // involved, it will transmit the close command to the other side before
    // closing its connections.  Whichever process receives the close command
    // should close their end and also all open data files at that time.

    // Call RECV to read in one CS450Header struct
    if (recv(c_tcpSocket, c_header, sizeof(CS450Header), 0) < 0) {
        perror("Error receiving header.");
    }
    else {
        std::cout<<"Acknowledgement received."<<endl;
    }

    c_endTime = time(0);
    c_totalTime = difftime(c_endTime, c_startTime);

    std::cout<<"----------Stats Report----------\n";
    std::cout<<"Data sent by Client\t:\t"<<c_st.st_size<<" bytes"<<endl;
    std::cout<<"Data received by Server\t:\t"<<ntohl(c_header->nbytes)<<" bytes"<<endl;
    std::cout<<"Round trip time\t:\t"<<c_totalTime<<" secs"<<endl;
    std::cout<<"--------------------------------\n";
    
    
    // If it is desired to transmit another file, possibly using a different
    // destination, protocol, relay, and/or other parameters, get the
    // necessary input from the user and try again.
    
    // When done, report overall statistics and exit.
    
    shutdown(c_tcpSocket, SHUT_RDWR);
    free(c_header);
    //system("pause");
    return EXIT_SUCCESS;
}
