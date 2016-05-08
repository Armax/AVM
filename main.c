//
//  main.c
//  avm
//
//  Created by Marco Cuciniello on 02/05/16.
//  Copyright © 2016 Arm4x. All rights reserved.
//

/*
 Specifics
 
 Structure (16 bit):
 
 3x type: 
 
 4bit   4bit   8bit
 0001 | 0001 | 00001010         [ ldr r1, #16 ]
  Op     r1     Value
 
 4x type:
 4bit   4bit   4bit   4bit
 0001 | 0011 | 0001 | 0010      [ add r3, r1, r2 ]
  Op     r1     r2     r3
 
 2x32 type:
 First instruction is the instruction itself, second is the argument
 
 4bit      16 bit               16 bit
 1000 | 000000000000       1001101101011111     [ jmp 0x9B5F ]
  Op   ignored or flag          address
 
 Instructions:
    1x:
        0 = ps          pause           : stop the execution
    2x32:
        8 = jmp         jump            : jump to a given address es: jmp 0x9B5F
        9 = push        push            : push a 16 bit value to the stack es: push 0x7A4F or a register es: push r1
       10 = pop         pop             : pop a 16 bit value from the stack, if no register is specified r5 will be used es: pop (value is in r5), pop r1
       11 = jz          jump flag: zf   : jump to a given address if zf is not 0 es: jz 0x9B5F
    3x:
        1 = lr          load register   : load a register with a value es: ldr r1, #16
        6 = not         not             : not operator es: not r0,r1 result will be in r0               (!r1)
       12 = cmp         cmp             : compare es: cmp r1, r2 if r1=r2 zf flag will be 1
       13 = mul         mul             : perform multiplication es: mul r3, r1, r2 result is in r3     (r1*r2)
       14 = mv          mov             : copy a value from a register to an other es: mv r3 r1
    4x:
        2 = ad          add             : perform addition es: ad r3, r1, r2 result is in r3            (r1+r2)
        3 = sb          sub             : perform substraction es: sb r3, r1, r2 result is in r3        (r1-r2)
        4 = and         and             : and operator es: and r3, r1, r2 result is in r3               (r1&r2)
        5 = or          or              : or operator es: or r3, r1, r2 result is in r3                 (r1||r2)
        7 = xor         xor             : xor operator es: xor r3, r1, r2 result is in r3               (r1^r2)
 
 Registers (16 bit):
    
    r0, r1, r2, r3, r4, r5 multipurpose registers
    sp, ns, zf
 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Registers
uint16_t r0         = 0;
uint16_t r1         = 0;
uint16_t r2         = 0;
uint16_t r3         = 0;
uint16_t r4         = 0;
uint16_t r5         = 0;
uint16_t sp         = 0; // stack pointer
uint16_t ns         = 0; // negative sub
uint16_t zf         = 0; // multi purpose flag

// still 7 free register maybe I will implement them later

uint16_t* r[9] = { &r0, &r1, &r2, &r3, &r4, &r5, &sp, &ns, &zf };

// Temp value / registers / address
uint16_t t_instr    = 0;
uint16_t argument   = 0;
int opnum           = 0;
int p1              = 0;
int p2              = 0;
int p3              = 0;
int val             = 0;

// Program counter
int pc = 0;

// State
short running = 1;

// Memory
unsigned char memory[65535];

// Stack
uint16_t stack[256];

int is_a_register(char* str) {
    char* registers[6] = {"r0","r1","r2","r3","r4","r5"};
    
    for(int x=0; x<6; x++) {
        if(strcmp(registers[x],str)==0) {
            return 1;
        }
    }
    return 0;
}

short fetch() {
    t_instr = 0;
    memcpy(&t_instr, &memory[pc], sizeof(t_instr));     // reading 2 byte
    t_instr = (t_instr>>8) | (t_instr<<8);              // endianess
    pc = pc+2;
    return t_instr;
}

void decode(short op) {
    opnum   = (op & 0xF000) >> 12;
    p1      = (op & 0xF00 ) >>  8;
    p2      = (op & 0xF0  ) >>  4;
    p3      = (op & 0xF   );
    val     = (op & 0xFF  );
}

void execute() {
    switch(opnum) {
        case 0:
            // ps
            printf("waiting...\n");
            running = 0;
            break;
        case 1:
            // ldr
            if(p1 > 8 || p1 < 0) {
                exit(0);
            }
            printf("lr r%d #%d\n", p1, val);
            *r[p1] = val;
            break;
        case 2:
            // add
            if(p1 > 8 || p1 < 0) {
                exit(0);
            }
            printf("add r%d r%d r%d\n", p1, p2, p3);
            *r[p1] = *r[p2] + *r[p3];
            printf("[debug] result from add: %d\n", *r[p1]);
            break;
        case 3:
            // sb
            if(p1 > 8 || p1 < 0 || p2 > 8 || p2 < 0 || p2 > 8 || p2 < 0) {
                exit(0);
            }
            printf("sb r%d r%d r%d\n", p1, p2, p3);
            *r[p1] = *r[p2] - *r[p3];
            break;
        case 4:
            // and
            if(p1 > 8 || p1 < 0 || p2 > 8 || p2 < 0 || p2 > 8 || p2 < 0) {
                exit(0);
            }
            printf("and r%d r%d r%d\n", p1, p2, p3);
            *r[p1] = *r[p2] & *r[p3];
            break;
        case 5:
            // or
            if(p1 > 8 || p1 < 0 || p2 > 8 || p2 < 0 || p2 > 8 || p2 < 0) {
                exit(0);
            }
            printf("or r%d r%d r%d\n", p1, p2, p3);
            *r[p1] = *r[p2] || *r[p3];
            break;
        case 6:
            // not
            if(p1 > 8 || p1 < 0) {
                exit(0);
            }
            printf("not r%d", p1);
            *r[p1] = !*r[p1];
            break;
        case 7:
            // or
            if(p1 > 8 || p1 < 0 || p2 > 8 || p2 < 0 || p2 > 8 || p2 < 0) {
                exit(0);
            }
            printf("xor r%d r%d r%d\n", p1, p2, p3);
            *r[p1] = *r[p2] ^ *r[p3];
            break;
        case 8:
            // jmp
            argument = fetch();
            printf("jmp 0x%04x\n", argument);
            pc = argument;
            break;
        case 9:
            // push
            if(val == 1) {
                if(p1 > 8 || p1 < 0) {
                    exit(0);
                }
                printf("push r%d\n", p1);
                stack[sp] = *r[p1];
            }
            else {
                argument = fetch();
                printf("push 0x%04x\n", argument);
                stack[sp] = argument;
            }
            sp++;
            break;
        case 10:
            // pop
            sp--;
            *r[p1] = stack[sp];
            break;
        case 11:
            // jz
            argument = fetch();
            printf("jz 0x%04x\n", argument);
            if(zf==1) {
                pc = argument;
            }
            break;
        case 12:
            // cmp
            if(p1 > 8 || p1 < 0 || p2 > 8 || p2 < 0) {
                exit(0);
            }
            printf("cmp r%d r%d\n", p1, p2);
            if(*r[p1]==*r[p2]) {
                zf = 1;
            }
            else {
                zf = 0;
            }
            break;
        case 13:
            // mul
            if(p1 > 8 || p1 < 0 || p2 > 8 || p2 < 0 || p2 > 8 || p2 < 0) {
                exit(0);
            }
            printf("mul r%d r%d r%d\n", p1, p2, p3);
            *r[p1] = *r[p2] * *r[p3];
            printf("[debug] result from mul: %d\n", *r[p1]);
            break;
        case 14:
            // mv
            if(p1 > 8 || p1 < 0 || p2 > 8 || p2 < 0) {
                exit(0);
            }
            printf("mv r%d r%d\n", p1, p2);
            *r[p1] = *r[p2];
            break;
        default:
            // not implemented
            printf("unexpected instruction: %d\n", opnum);
            break;
            
    }
    
}

void ir() {
    printf( "regs = " );
    for(int x=0; x<6; x++) {
        printf("r%d: %016x ", x, *r[x]);
    }
    printf("sp: %016x ", sp);
    printf("zf: %016x ", zf);
    printf( "\n" );
}

void p_raw_memory() {
    printf( "memory = " );
    for(int x=0; x<65535; x++) {
        printf("%02x", memory[x]);
    }
    printf("\n");
}

void p_stack() {
    printf( "stack = " );
    for(int x=0; x<256; x++) {
        printf("%04x", stack[x]);
    }
    printf("\n");
}

void run() {
    while(running) {
        short op = fetch();     // fetch
        decode(op);
        // decode
        execute();              // execute
        if(running) {
            ir();               // uncomment to show registers after every instruction
            //p_stack();          // uncomment to show stack contents after every instruction
            //p_raw_memory()    // uncomment to show memory contents after every instruction
        }
    }
}

// Mapping binary into memory
void exec(char filepath[]) {
    FILE *fp = NULL;
    long filelen;
    
    fp=fopen(filepath, "rb");
    
    if(fp != NULL) {
        fseek(fp, 0, SEEK_END);
        filelen = ftell(fp);
        rewind(fp);
        
        if(filelen > 65535) {
            printf("File too large\n");
            fclose(fp);
        }
        else {
            fread(memory, 65535, filelen, fp);
            fclose(fp);
            run();
        }
    }
    else {
        printf("Error: %s doesn't exist\n", filepath);
    }
    
    fclose(fp);
    return;
}

int main(int argc, const char * argv[]) {
    char cmd[50];
    
    printf("Welcome to Arm4x VM\n\n");
    printf("[avm]$ ");
    scanf("%50s", cmd);
    exec(cmd);
    
    //run();
    return 0;
}
