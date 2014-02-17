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

#include "CS450Header.h"

using namespace std;


int main(int argc, char *argv[])
{
    // Variables
    string c_destinationIP = "127.0.0.1";
    int c_destinatinPortNumber = 54321;
    string c_relay = "";
    int c_relayPortNumber = 54320;
    string c_fileName = "";
    int c_fd;

    struct stat c_st;
    void *c_addr = NULL;

    int c_tcpSocket = 0;

    sockaddr_in c_serverAddr;       // server address structure
    sockaddr_in c_relayAddr;        // relay address structure
    sockaddr_in c_clientAddr;       // client address structure

    struct hostent *c_hp;
    struct hostent *c_rp;

    time_t c_startTime, c_endTime;
    double c_totalTime;

    char c_myAddress[ 100 ];

    CS450Header *c_header = (CS450Header *)malloc(sizeof(CS450Header));
    
    std::cout<<"\nAuthor\t:\tGiric Goyal\nID\t:\tggoyal2\nUIN\t:\t657440995\n\n";

    char c_sendMore = 'n';
    char c_saveFile = 'n';

    int c_fileCounter = 0;
    long int c_totalSent = 0;
    long int c_totalReceived = 0;
    long int c_totalTimeElapsed = 0;
    string c_allFiles = "";


    //int c_logFile;
   // string c_log = "";

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
    else {
        std::cout<<"\nUse the command line arguments to provide the destination ip and port number.";
        std::cout<<"\n\n./client [destination-ip] [destination-port] [relay-ip] [relay-port]";
        std::cout<<"\n\nThe first argument [destination-ip] is mandatory. Rest of the values are used default.";
        std::cout<<"\nDefault destination port: 54321";
        std::cout<<"\nDefault relay: none";
        std::cout<<"\nDefault relay port: 54320\n\n\n";
        return 0;
    }

    do {

        if (c_sendMore == 'y' || c_sendMore == 'Y') {
            std::cout<<"\nEnter destination IP and Port Number : ";
            std::cin>>c_destinationIP>>c_destinatinPortNumber;
            std::cout<<"Enter relay IP and Port Number : ";
            std::cin>>c_relay>>c_relayPortNumber;
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

        // Open a Connection to the server ( or relay )  TCP for the first HW
        // call SOCKET and CONNECT and verify that the connection opened.

        if ((c_tcpSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("Cannot create socket. Exiting");
            return 0;
        }
        else {
            std::cout<<"Socket created. "<<endl;
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

        if (c_relay != "" && strcmp(c_relay.c_str(),"none") != 0) {
            c_rp = gethostbyname(c_relay.c_str());
            if (!c_rp) {
                perror("Error finding relay address.");
            }
            c_relayAddr.sin_family = AF_INET;
            c_relayAddr.sin_port = htons(c_relayPortNumber);

            memcpy((void *)&c_relayAddr.sin_addr, c_rp->h_addr_list[0], c_rp->h_length);
            
            if (connect(c_tcpSocket, (sockaddr *)&c_relayAddr, sizeof(c_relayAddr)) < 0) {
                perror("Error connecting to relay. Exiting");
                return 0;
            }
            else {
                std::cout<<"Connected to relay."<<endl;
            }

        }
        else {
            if (connect(c_tcpSocket, (sockaddr *)&c_serverAddr, sizeof(c_serverAddr)) < 0) {
                perror("Error connecting to server. Exiting");
                return 0;
            }
            else {
                std::cout<<"Connected to server."<<endl;
            }
        }
        // Note the time before beginning transmission
        c_startTime = time(0);

        // Create a CS450Header object, fill in the fields, and use SEND to write
        // it to the socket.

        gethostname( c_myAddress, 100 );
        if (!gethostbyname(c_myAddress)) {
            perror("Error finding host address. Exiting.");
            return 0;
        }

        c_clientAddr.sin_family = AF_INET;
        c_clientAddr.sin_port = htons(c_destinatinPortNumber);
        memcpy((void *)&c_clientAddr.sin_addr, gethostbyname(c_myAddress)->h_addr_list[0], gethostbyname(c_myAddress)->h_length);

        c_header->version = htonl(4);
        c_header->UIN = htonl(657440995);
        c_header->HW_number = htonl(1);
        c_header->transactionNumber = htonl(1);
        strcpy(c_header->ACCC,"ggoyal2");
        strcpy(c_header->filename,c_fileName.c_str());
        c_header->from_IP = c_clientAddr.sin_addr.s_addr;
        c_header->to_IP = c_serverAddr.sin_addr.s_addr;
        c_header->packetType = htonl(1);
        c_header->nbytes = htonl(c_st.st_size);
        c_header->relayCommand = htonl(0);
        c_header->persistent = (c_sendMore == 'y' || c_sendMore == 'Y')? htonl(1) : htonl(0);
        c_header->saveFile = (c_saveFile == 'y' || c_saveFile == 'Y')? htonl(1) : htonl(0);
        c_header->from_Port = htons(c_destinatinPortNumber);
        c_header->to_Port = htons(c_destinatinPortNumber);
        c_header->trueFromIP = (c_clientAddr.sin_addr.s_addr);
        c_header->trueToIP = (c_serverAddr.sin_addr.s_addr);


        // send the header
        if (send(c_tcpSocket, c_header, sizeof(CS450Header), 0) < 0) {
            perror("Error sending message.");
        }
        else {
            std::cout<<"Header sent."<<endl;
        }
        // Use SEND to send the data file.  If it is too big to send in one gulp
        // Then multiple SEND commands may be necessary.
        
        int c_startIndex = 0;
        int c_endIndex = 0;
        char *c_tempString = (char*)malloc(65536);
        int c_sentBytes = 0;
        //int c_counter = 0;
        while(c_endIndex <= c_st.st_size) {
            if (c_endIndex + 65536 <= c_st.st_size) {
                c_endIndex = c_endIndex + 65536;
            }
            else {
                c_endIndex = c_st.st_size;
                free(c_tempString);
                c_tempString = (char*) malloc (c_endIndex - c_startIndex);
            } 
            //std::cout<<c_startIndex<<":"<<c_endIndex<<":"<<c_st.st_size<<":"<<c_endIndex - c_startIndex<<endl;
            strncpy(c_tempString, (char*)c_addr + c_startIndex, c_endIndex - c_startIndex);

            if ((c_sentBytes = send(c_tcpSocket, c_tempString, c_endIndex - c_startIndex, 0)) < 0) {
                perror("Error sending message.");
            }
            else {
                //std::cout<<"File sent : "<<c_sentBytes<<" : "<<++c_counter<<endl;
            }


            if (c_startIndex + 65536 <= c_st.st_size) {
                c_startIndex = c_startIndex + 65536;
            }
            else {
                c_startIndex = c_st.st_size - c_startIndex;
            }
            if (c_endIndex == c_st.st_size) {
                break;
            }
        }
        
        /*
        if (send(c_tcpSocket, c_addr, c_st.st_size, 0) < 0) {
            perror("Error sending message.");
        }
        else {
            std::cout<<"File sent."<<endl;
        }
        */
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

        c_totalTimeElapsed += c_totalTime;
        c_totalSent += c_st.st_size;
        c_totalReceived += ntohl(c_header->nbytes);
        c_fileCounter ++;
        c_allFiles += c_fileName + " ";
        /*
        c_logFile = open("client_log.txt", O_WRONLY | O_APPEND, S_IRWXU);
        if (c_logFile < 0) {
            c_logFile = open("client_log.txt", O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU);
        }
        if (c_logFile < 0) {
            perror("Error Opening/Creating Log File.");
        }
        else {
            if (write(c_logFile, "test", 5) < 0) {
                perror("Error writing log file.");
            }
            else {
                std::cout<<"Log Saved."<<endl;
                close(c_logFile);
            }
        }
        */
        // If it is desired to transmit another file, possibly using a different
        // destination, protocol, relay, and/or other parameters, get the
        // necessary input from the user and try again.
        
        // When done, report overall statistics and exit.

    }while(c_sendMore == 'y' || c_sendMore == 'Y');

    shutdown(c_tcpSocket, SHUT_RDWR);
    free(c_header);

    std::cout<<"----------Stats Report----------\n";
    std::cout<<"Files sent("<<c_fileCounter<<")\t\t:\t"<<c_allFiles<<endl;
    std::cout<<"Data sent by Client\t:\t"<<c_totalSent<<" bytes"<<endl;
    std::cout<<"Data received by Server\t:\t"<<c_totalReceived<<" bytes"<<endl;
    std::cout<<"Round trip time\t\t:\t"<<c_totalTimeElapsed<<" secs"<<endl;
    std::cout<<"--------------------------------\n";
    

    //system("pause");
    return EXIT_SUCCESS;
}
