/*  CS450Header.h

    This file defines the format of the header block that is to precede each
    block of data transmitted for the CS 450 HW in Spring 2014.
    
    This is version 5 of this header, with fields added to support reliable
    data transfer over unreliable networks.  Any fields that are not currently 
    in use should be set to zero.
    
    Also, all loosely defined types, such as long int, that could vary from
    machine to machine have been converted to uint32_t or something like it,
    as described in 
    http://www.nongnu.org/avr-libc/user-manual/group__avr__stdint.html.
    
*/

#ifndef CS450HEADER_H
    #define CS450HEADER_H
    
    // ALL NUMBERS LARGER THAN ONE BYTE SHOULD BE STORED IN NETWORK ORDER
    
    typedef struct{
        // First all the 32-bit numbers, for packing purposes
        int32_t version;  // Set to 7.  Later versions may have more fields
        int32_t UIN; // Packets with unrecognized UINs will be dropped
        int32_t transactionNumber;  // To identify  parts of one transaction.
        int32_t sequenceNumber; 
        int32_t ackNumber;  // Acknowledgement number
        uint32_t from_IP, to_IP;  // Ultimate destination, not the relay
        uint32_t trueFromIP, trueToIP; // AWS may change public IP vs private IP
        uint32_t nbytes; // Number of bytes of data to follow in this packet
        uint32_t nTotalBytes; // Total number of bytes in the file
        char filename[ 32 ];  // the name of the file being transmitted.
                             // i.e. the name where the server should store it.
        // Then the 16-bit ones - An even number of these.
        uint16_t from_Port, to_Port; // Ultimate source & destination, Not relay
        uint16_t checksum;  // One's complement of sum of remaining bytes
        // Then some 8-bit numbers.  Hopefully a multiple of 4 of these
        int8_t HW_number; // e.g. 1 for HW1, etc. 
        int8_t packetType; // 1 = file transfer; 2 = acknowledgement; 3 = both
        int8_t relayCommand; // 1 = close connection?
        int8_t saveFile;  // non-zero:  Save to disk.  Else discard.
        char ACCC[ 8 ];  // your ACCC user ID name, e.g. astudent42
        int8_t dropChance; // 0 to 100, as an integral percentage
        int8_t dupeChance; // 0 to 100, as an integral percentage
        int8_t garbleChance; // 0 to 100, as an integral percentage
        int8_t delayChance; // 0 to 100, as an integral percentage
        uint8_t windowSize; // 0 to 255.  0 for Stop and Wait
        int8_t protocol; // * 10.  E.g. 22 => 2.2, 30 => 3.0, etc.
            // 30 for RDT 3.0 stop-and-wait, 31 for RDT 3.0 Go-Back-N,
            // and 32 for RDT 3.0 Selective_Repeat 
        
        // Students may change the following, so long as the total overall size
        // of the struct does not change.
        // I.e. you can add additional fields, but if you do, reduce the size of
        // the reserved array by an equal number of bytes.
        
        char unused[ 409 ]; // Unused padding  Total header size should be 512
        
    } CS450Header;
    
    const static int BLOCKSIZE = 3584; // 3.5k, so total packet size = 4k
    
    typedef struct { 
        CS450Header header; // 512 bytes
        char data[ BLOCKSIZE ];  // 3.5k - Total Packet Size = 4k
    } Packet;
    
    const static int PacketSize = sizeof( Packet );

#endif
