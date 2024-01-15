#ifndef __BESK_H__
#define __BESK_H__

#include <stdio.h>
#include <stdint.h>

#include "helord.h"
#include "halvord.h"

typedef uint8_t oktet_t;    // 8-bit

#define NUM_HALF_CELLS (2048)      // number of halfcells in core memory

#define KONTROLL_BIT 0x80          // kontroll utskrift if switch is set
#define ARZERO_BIT   0x40
#define HELORD_BIT   0x20

// instruction w:12 op:8 = 20 bit
#define W(ins)  (((ins) >> 8) & 0xFFF)  // get W word pointer
#define O(ins)  ((ins) & 0xFF)          // operation 
#define Z(ins)  ((ins) & ARZERO_BIT)    // set AR=0 before operation
#define H(ins)  ((ins) & HELORD_BIT)    // helord operation (pointed to by w)
#define N(ins)  ((ins) & 0x1F)          // operation number

#define MAKE_OP(W,Z,H,N) (((W)<<8)|(((Z)&1)<<6)|(((H)&1)<<5)|((N)&0x1F))

typedef struct
{
    helord_t  MD;     // multiplikand. MDV+MDH, MD0,MD1...MD39
    helord_t  MR;     // multiplikator    
    helord_t  AR;     // ackumulator.  ARV+ARH  AR0,AR1,....,AR39 (spill)
    helord_t  ARP;    // previous AR (used when AR may be set=0 by op)
    oktet_t   AR00;   // store in AR?
    oktet_t   AR40;   // store in AR?
    oktet_t   SI;     // spillindikation

    halvord_t KR;     // Kontrollregister
    halvord_t INS;    // Instruktion AS + OP
    helord_t  BR;     // Binärräknare (???)
    // 2048 halfword memory cells left cells vhac is located on even addresses
    // and hhac are located in odd addresses
    void* user_data;  // emulator etc
    // paper tape / printer
    FILE* in;         // inremsa
    FILE* ut;         // utremsa
    int page;         // telex page code (0=undefined)    
    // drum memory
    FILE* drum;
    // Function display
    uint8_t  Fpos_x;   // 1,2,3,4,5,6,8 (scale factor x)
    uint8_t  Fpos_y;   // 1,2,3,4,5,6,8 (scale factor y)
    helord_t Fx;
    helord_t Fy;
    helord_t Fop;  // 0=no ouput, 1=dot, 2=circle
    int utmatning_pos;          // 0...35 = 0,10,20...,350 degree
    int kontroll_utskrift_pos;  // 0..35
    int gang_pos;               // 0..35
    int         running; // 0 = stopped, 1 = runnnig
    int         trace;    // instruction trace output
    int         quit;     // terminate
    halvord_t   MEM[NUM_HALF_CELLS];
} besk_t;

#define GANG_STEP        7
#define GANG_RUN         10
#define GANG_VARIABLE    2

#define KONTROLL_UTSKRIFT_OFF         2
#define KONTROLL_UTSKRIFT_E2_UTSKRIFT 4
#define KONTROLL_UTSKRIFT_STEGVIS     0

#define UTMATNING_STANS       0
#define UTMATNING_ORDER       2
#define UTMATNING_SKRIVMASKIN 5

// Mnemonic - names are my, change when I find som standard...
#define OP_BAND    0x00
#define OP_MOVMR   0x01
#define OP_MUL     0x02
#define OP_MULR    0x03
#define OP_ASHR    0x04
#define OP_SHR     0x44
#define OP_SHL     0x05
#define OP_SHL40   0x45
#define OP_ADDST   0x06
#define OP_INCST   0x46
#define OP_STORA   0x07
#define OP_ADDMR   0x08
#define OP_SUBMR   0x09
#define OP_JC      0x0A
#define OP_SUB     0x0B
#define OP_NEG     0x4B
#define OP_JMP     0x0C
#define OP_AADD    0x0D
#define OP_JGE     0x0E
#define OP_JLT     0x4E
#define OP_ASUB    0x0F
#define OP_LOAD    0x70  // zero and add [as]
#define OP_ADD     0x10
#define OP_STORE   0x11
#define OP_DIV     0x12
#define OP_REV     0x13
#define OP_READ5_  0x14
#define OP_READ5   0x74
#define OP_NORM    0x15
#define OP_NORM40  0x35
#define OP_UNDEF16 0x16
#define OP_UNDEF17 0x17
#define OP_FUNC    0x18
#define OP_READ4x10 0x19
#define OP_READ4x1 0x39
#define OP_UNDEF1A 0x1A
#define OP_RD      0x1B
#define OP_WRITE4  0x1C
#define OP_WRITE   0x1D
#define OP_UNDEF1E 0x1E
#define OP_WD      0x1F


// drum memory size = 256 * 32 * 5 = 40960 bytes
#define DRUM_CHANNEL_SIZE  0x40  // 64 halfwords per channel
#define DRUM_CHANNEL_BYTES ((DRUM_CHANNEL_SIZE/2)*5)  // 32*5 = 160 bytes per channel
#define DRUM_NUM_CHANNELS       0x100  // 256
#define DRUM_MAX_CHANNEL_NUMBER 0x1FE  // 510

#endif
