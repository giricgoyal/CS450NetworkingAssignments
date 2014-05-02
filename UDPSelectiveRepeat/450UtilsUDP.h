/*  450UtilsUDP.h

    This file contains a number of useful functions for use in CS 450 HW
        
    Written by John T. Bell January 2014 for CS 450

    Edited by Giric Goyal for CS450 HW4
*/

#ifndef CS450UTILSUDP_H
#define CS450UTILSUDP_H

#include <cstdlib>
#include <cstdio>

#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>

#include <netinet/in.h>

#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>


#include "CS450Header.h"

using namespace std;

void printIP( ostream & out, uint32_t IP );

ostream & operator << ( ostream & out, const CS450Header & h );
ostream & operator << ( ostream & out, const struct sockaddr_in & addr );

void packHeader( CS450Header & h, int32_t version, int32_t UIN, 
    int32_t transactionNumber, int32_t sequenceNumber, 
    int32_t ackNumber, uint32_t from_IP, uint32_t to_IP, 
    uint32_t trueFromIP, uint32_t trueToIP, uint32_t nbytes,
    uint32_t nTotalBytes, const char * filename, uint16_t from_Port,
    uint16_t to_Port, uint16_t checksum, int8_t HW_number, int8_t packetType, 
    int8_t relayCommand, int8_t saveFile, const char * ACCC, int8_t dropChance, 
    int8_t dupeChance, int8_t garbleChance, int8_t delayChance, 
    uint8_t windowSize, int8_t protocol ) {

    h.version = version;
    h.UIN = UIN;
    h.transactionNumber = transactionNumber;
    h.sequenceNumber = sequenceNumber;
    h.ackNumber = ackNumber;
    h.from_IP = from_IP;
    h.to_IP = to_IP;
    h.trueFromIP = trueFromIP;
    h.trueToIP = trueToIP;
    h.nbytes = nbytes;
    h.nTotalBytes = nTotalBytes;
    strcpy(h.filename, filename);
    h.from_Port = from_Port;
    h.to_Port = to_Port;
    h.checksum = checksum;
    h.HW_number = HW_number;
    h.packetType = packetType;
    h.relayCommand = relayCommand;
    h.saveFile = saveFile;
    strcpy(h.ACCC, ACCC);
    h.dropChance = dropChance;
    h.dupeChance = dupeChance;
    h.garbleChance = garbleChance;
    h.delayChance = delayChance;
    h.windowSize = windowSize;
    h.protocol = protocol;
}


void unpackHeader( const CS450Header & h, int32_t & version, int32_t & UIN, 
    int32_t & transactionNumber, int32_t & sequenceNumber, 
    int32_t & ackNumber, uint32_t & from_IP, uint32_t & to_IP, 
    uint32_t & trueFromIP, uint32_t & trueToIP, uint32_t & nbytes,
    uint32_t & nTotalBytes, char * filename, uint16_t & from_Port,
    uint16_t & to_Port, uint16_t & checksum, int8_t & HW_number, 
    int8_t & packetType, int8_t & relayCommand, int8_t & saveFile, char * ACCC,
    int8_t & dropChance, int8_t & dupeChance, int8_t & garbleChance, int8_t & delayChance,
    uint8_t & windowSize, int8_t & protocol ) {


    version = h.version;
    UIN = h.UIN;
    transactionNumber = h.transactionNumber;
    sequenceNumber = h.sequenceNumber;
    ackNumber = h.ackNumber;
    from_IP = h.from_IP;
    to_IP = h.to_IP;
    trueFromIP = h.trueFromIP;
    trueToIP = h.trueToIP;
    nbytes = h.nbytes;
    nTotalBytes = h.nTotalBytes;
    strcpy(filename, h.filename);
    from_Port = h.from_Port;
    to_Port = h.to_Port;
    checksum = h.checksum;
    HW_number = h.HW_number;
    packetType = h.packetType;
    relayCommand = h.relayCommand;
    saveFile = h.saveFile;
    strcpy(ACCC, h.ACCC);
    dropChance = h.dropChance;
    dupeChance = h.dupeChance;
    garbleChance = h.garbleChance;
    delayChance = h.delayChance;
    windowSize = h.windowSize;
    protocol = h.protocol;
}


uint16_t calcChecksum( void *data, int nbytes ) {
/* Algorithm:
    1. Typecast data into a uint16_t * data16, and also into a uint8_t * data8
    2. Set uint32_t sum = 0, max16 = 0xffff;
    3. Loop for i = 0 to nbytes / 2
        A. add data16[ i ] to sum
        B. if sum > max16, subtract max16 from sum
    4. if nbytes is odd
        A. add data8[ nbytes - 1 ] to sum
        B. if sum > max16, subtract max16 from sum
    5. Return the one's complement of sum.
    
    To use:
        1. Set checksum field of header to 0
        2. Calculate checksum of Packet 
        3. Store result in checksum field of header.
        4. Now if you calculate the checksum of the packet,
            the result should be zero.
*/
    uint32_t checksum = 0;
    uint16_t *data_16bit = (uint16_t *)data;
    uint8_t *data_8bit = (uint8_t *)data;

    while(nbytes > 1) {
        checksum = checksum + *(data_16bit)++;  
        nbytes = nbytes - 2;
    }
    if (nbytes > 0) {
        checksum = checksum + *(data_8bit);
    }
    while (checksum >> 16) {
        checksum = (checksum & 0xFFFF) + (checksum >> 16);
    }
    return (~checksum);
}

#endif












