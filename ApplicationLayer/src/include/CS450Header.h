/*  CS450Header.h

    This file defines the format of the header block that is to precede each
    block of data transmitted for the CS 450 HW in Spring 2014.
    
*/

#ifndef CS450HEADER_H
#define CS450HEADER_H
// ALL NUMBERS LARGER THAN ONE BYTE SHOULD BE STORED IN NETWORK ORDER

typedef struct{
    int version;  // Set to 4.  Later versions may have more fields
    long int UIN; // Packets with unrecognized UINs will be dropped
    int HW_number; // e.g. 1 for HW1, etc. 
    int transactionNumber;  // To identify multiple parts of one transaction.
    char ACCC[ 10 ],  // your ACCC user ID name, e.g. astudent42
        filename[ 20 ];  // the name of the file being transmitted.
                         // i.e. the name where the server should store it.
    uint32_t from_IP, to_IP;  // Ultimate destination, not the relay
    int packetType; // 1 = file transfer; 2 = acknowledgement
    unsigned long nbytes; // Number of bytes of data to follow the header
    // float garbleChance, dropChance, dupeChance; // For later HW
    int relayCommand; // 1 = close connection
    int persistent;  // non-zero:  Hold connection open after this file 
    int saveFile;  // non-zero:  Save incoming file to disk.  Else discard it.
    uint16_t from_Port, to_Port; // Ultimate source & destination.  Not relay.
    uint32_t trueFromIP, trueToIP; // AWS may change public IP vs private IP
    
    // Students may change the following, so long as the total overall size
    // of the struct does not change.
    // I.e. you can add additional fields, but if you do, reduce the size of the
    // reserved array by an equal number of bytes.
    
    char reserved[ 988 ]; // Unused padding
    
    
} CS450Header;

#endif
