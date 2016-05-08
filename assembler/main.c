//
//  main.c
//  avm_assembler
//
//  Created by Marco Cuciniello on 03/05/16.
//  Copyright © 2016 Arm4x. All rights reserved.
//

// Opcode syntax is documented in avm/avm/main.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// write 4 bit number at position cbit from
void write_4bit(uint16_t* instr, short num, short* cbit) {
    int c, k;
    for (c = 3; c >= 0; c--)
    {
        k = num >> c;
        
        if (k & 1)
            *instr |= 1 << *cbit;
        else
            *instr |= 0 << *cbit;
        
        *cbit = *cbit - 1;
    }
}

// write 8 bit number at position cbit
void write_8bit(uint16_t* instr, short num, short* cbit) {
    int c, k;
    for (c = 7; c >= 0; c--)
    {
        k = num >> c;
        
        if (k & 1)
            *instr |= 1 << *cbit;
        else
            *instr |= 0 << *cbit;
        
        *cbit = *cbit - 1;
    }
}

// write 16 bit number at position cbit
void write_16bit(uint16_t* instr, short num, short* cbit) {
    int c, k;
    for (c = 15; c >= 0; c--)
    {
        k = num >> c;
        
        if (k & 1)
            *instr |= 1 << *cbit;
        else
            *instr |= 0 << *cbit;
        
        *cbit = *cbit - 1;
    }
}

// used in jmp (linked list of labels)
typedef struct s_label {
    char name[50];
    int line;
    struct s_label* next;
} label;

label* create_label(char* namex, int linex) {
    label* newLabel = malloc(sizeof(label));
    if(newLabel != NULL) {
        strncpy(newLabel->name, namex, 50);
        newLabel->line = linex;
        newLabel->next = NULL;
    }
    return newLabel;
}

void delete_label(label* labelx) {
    if(labelx->next != NULL) {
        delete_label(labelx->next);
    }
    free(labelx);
}

label* add_label(label* labellist, char* namex, int linex) {
    label* newLabel = create_label(namex, linex);
    if(newLabel != NULL) {
        newLabel->next = labellist;
    }
    return newLabel;
}

int search_label(label* labellist, char* namex) {
    label* x;
    for(x=labellist; x != NULL; x=x->next) {
        if(strcmp(x->name,namex)==0) {
            return x->line;
        }
    }
    return -1;
}

int is_a_register(char* str) {
    char* registers[6] = {"r0","r1","r2","r3","r4","r5"};
    
    for(int x=0; x<6; x++) {
        if(strcmp(registers[x],str)==0) {
            return 1;
        }
    }
    return 0;
}

int main(int argc, const char * argv[]) {
    if(argc < 2) {
        printf("error: no input file\n");
        return 0;
    }
    
    FILE *fp = fopen(argv[1], "rb");
    label* detectedLabels = create_label("init",0);
    uint16_t instr;                                     // encoded instruction
    uint16_t program[32765];                            // bytearray of assembled program
    int pc;                                             // point where we are in the program buffer
    int templine;                                       // temp line jump
    short cbit = 15;                                    // current bit
    short line = 0;                                     // line of .a4x file
    short skip_lt = 0;                                  // skip last token call
    char* buffer = 0;                                   // read file content
    char* tok = 0;                                      // token
    long filelen;                                       // file length
    
    if (!fp) {
        printf("error: no such file or directory: '%s'\n", argv[1]);
        delete_label(detectedLabels);
        return 0;
    }
    
    fseek (fp, 0, SEEK_END);
    filelen = ftell (fp);
    fseek (fp, 0, SEEK_SET);
    buffer = malloc(filelen);
    fread (buffer, 1, filelen, fp);
    
    // ye I know, there are better ways instead of strtok but is a 16 bit instruction set...
    tok = strtok (buffer,"  \t\n,;");
    
    // Creating labels structure
    while(tok!=NULL) {
        if(tok[(strlen(tok))-1] == ':') {
            tok[(strlen(tok))-1] = '\0';
            printf("[debug] detected label: %s\n", tok);
            detectedLabels = add_label(detectedLabels, tok, line);
        }
        
        if(strcmp(tok,"jmp")==0 || strcmp(tok,"jz")==0) {
            line = line + 4;
        }
        
        if(strcmp(tok,"push")==0) {
            tok = strtok (NULL,"  \t\n,;");
            if(is_a_register(tok)) {
                line = line + 2;
            }
            else {
                line = line + 4;
            }
        }
        if(strcmp(tok,"ps")==0  ||
           strcmp(tok,"pop")==0 ||
           strcmp(tok,"lr")==0  ||
           strcmp(tok,"not")==0 ||
           strcmp(tok,"cmp")==0 ||
           strcmp(tok,"mul")==0 ||
           strcmp(tok,"mv")==0  ||
           strcmp(tok,"ad")==0  ||
           strcmp(tok,"sb")==0  ||
           strcmp(tok,"and")==0 ||
           strcmp(tok,"or")==0  ||
           strcmp(tok,"xor")==0
           ) {
            line = line + 2;
        }
        
        tok = strtok (NULL,"  \t\n,;");
    }
    
    line = 0;
    fseek (fp, 0, SEEK_SET);
    fread (buffer, 1, filelen, fp);
    tok = strtok (buffer,"  \t\n,;");
    
    // parsing
    while(tok != NULL)
    {
        instr = 0;
        skip_lt = 0;
        
        // ps: pause
        if(strcmp(tok,"ps") == 0) {
            write_8bit(&instr, 0, &cbit);
            write_8bit(&instr, 0, &cbit);
            
            printf("[debug] encoded instr: %04x\n", instr);
            program[pc] = instr;
            pc++;
            cbit = 15;
            line++;
        }
        
        // lr: load register
        else if(strcmp(tok,"lr") == 0) {
            // first 4 bit opcode
            write_4bit(&instr, 1, &cbit);
            
            // second 4 bit register
            tok = strtok (NULL,"   \t\n,;");
            write_4bit(&instr, atoi(&tok[1]), &cbit);
            
            // last 8 bit value
            tok = strtok (NULL,"   \t\n,;");
            
            // decimal value
            if(tok[0] == '#') {
                tok++;
                if(atoi(tok) <= 255) {
                    write_8bit(&instr, atoi(tok), &cbit);
                }
                else {
                    printf("Syntax error, line: %d value greater then 255\n", line);
                    free(buffer);
                    delete_label(detectedLabels);
                    return 0;
                }
            }
            
            // Hexadecimal value
            else if(tok[0] == '0' && tok[1] == 'x') {
                tok++;
                tok++; // tok = tok + 2 is less sneaky >_>
                int number = (int)strtol(tok, NULL, 16);
                if(number <= 65535) {
                    write_16bit(&instr, number, &cbit);
                }
                else {
                    printf("Syntax error, line: %d invalid memory address\n", line);
                    free(buffer);
                    delete_label(detectedLabels);
                    return 0;
                }
            }
            
            // Dunno value
            else {
                printf("Syntax error, line: %d invalid value format\n", line);
                free(buffer);
                delete_label(detectedLabels);
                return 0;
            }

            
            // check 15 bit write
            if(cbit>-1) {
                printf("Syntax error, line: %d\n", line);
                free(buffer);
                delete_label(detectedLabels);
                return 0;
            }
            
            // Cleaning and updating
            printf("[debug] encoded instr: %04x\n", instr);
            program[pc] = instr;
            pc++;
            cbit = 15;
            line++;
        }
        
        else if(strcmp(tok,"not") == 0) {
            write_4bit(&instr, 6, &cbit);
            
            tok = strtok (NULL,"  \t\n,;");
            write_4bit(&instr, atoi(&tok[1]), &cbit);
            
            tok = strtok (NULL,"  \t\n,;");
            write_4bit(&instr, atoi(&tok[1]), &cbit);
            
            tok = strtok (NULL,"  \t\n,;");
            write_4bit(&instr, 0, &cbit);
            
            if(cbit>-1) {
                printf("Syntax error, line: %d\n", line);
                free(buffer);
                delete_label(detectedLabels);
                return 0;
            }
            
            printf("[debug] encoded instr: %04x\n", instr);
            program[pc] = instr;
            pc++;
            cbit = 15;
            line++;
        }
        
        else if(strcmp(tok,"ad")==0) {
            write_4bit(&instr, 2, &cbit);
            
            tok = strtok (NULL,"  \t\n,;");
            write_4bit(&instr, atoi(&tok[1]), &cbit);
            
            tok = strtok (NULL,"  \t\n,;");
            write_4bit(&instr, atoi(&tok[1]), &cbit);
            
            tok = strtok (NULL,"  \t\n,;");
            write_4bit(&instr, atoi(&tok[1]), &cbit);
            
            if(cbit>-1) {
                printf("Syntax error, line: %d\n", line);
                free(buffer);
                delete_label(detectedLabels);
                return 0;
            }
            
            printf("[debug] encoded instr: %04x\n", instr);
            program[pc] = instr;
            pc++;
            cbit = 15;
            line++;
        }
        
        else if(strcmp(tok,"sb")==0) {
            write_4bit(&instr, 3, &cbit);
            
            tok = strtok (NULL,"  \t\n,;");
            write_4bit(&instr, atoi(&tok[1]), &cbit);
            
            tok = strtok (NULL,"  \t\n,;");
            write_4bit(&instr, atoi(&tok[1]), &cbit);
            
            tok = strtok (NULL,"  \t\n,;");
            write_4bit(&instr, atoi(&tok[1]), &cbit);
            
            if(cbit>-1) {
                printf("Syntax error, line: %d\n", line);
                free(buffer);
                delete_label(detectedLabels);
                return 0;
            }
            
            printf("[debug] encoded instr: %04x\n", instr);
            program[pc] = instr;
            pc++;
            cbit = 15;
            line++;
        }
        
        else if(strcmp(tok,"and")==0) {
            write_4bit(&instr, 4, &cbit);
            
            tok = strtok (NULL,"  \t\n,;");
            write_4bit(&instr, atoi(&tok[1]), &cbit);
            
            tok = strtok (NULL,"  \t\n,;");
            write_4bit(&instr, atoi(&tok[1]), &cbit);
            
            tok = strtok (NULL,"  \t\n,;");
            write_4bit(&instr, atoi(&tok[1]), &cbit);
            
            if(cbit>-1) {
                printf("Syntax error, line: %d\n", line);
                free(buffer);
                delete_label(detectedLabels);
                return 0;
            }
            
            printf("[debug] encoded instr: %04x\n", instr);
            program[pc] = instr;
            pc++;
            cbit = 15;
            line++;
        }
        
        else if(strcmp(tok,"or")==0) {
            write_4bit(&instr, 5, &cbit);
            
            tok = strtok (NULL,"  \t\n,;");
            write_4bit(&instr, atoi(&tok[1]), &cbit);
            
            tok = strtok (NULL,"  \t\n,;");
            write_4bit(&instr, atoi(&tok[1]), &cbit);
            
            tok = strtok (NULL,"  \t\n,;");
            write_4bit(&instr, atoi(&tok[1]), &cbit);
            
            if(cbit>-1) {
                printf("Syntax error, line: %d\n", line);
                free(buffer);
                delete_label(detectedLabels);
                return 0;
            }
            
            printf("[debug] encoded instr: %04x\n", instr);
            program[pc] = instr;
            pc++;
            cbit = 15;
            line++;
        }
        
        else if(strcmp(tok,"xor")==0) {
            write_4bit(&instr, 7, &cbit);
            
            tok = strtok (NULL,"  \t\n,;");
            write_4bit(&instr, atoi(&tok[1]), &cbit);
            
            tok = strtok (NULL,"  \t\n,;");
            write_4bit(&instr, atoi(&tok[1]), &cbit);
            
            tok = strtok (NULL,"  \t\n,;");
            write_4bit(&instr, atoi(&tok[1]), &cbit);
            
            if(cbit>-1) {
                printf("Syntax error, line: %d\n", line);
                free(buffer);
                delete_label(detectedLabels);
                return 0;
            }
            
            printf("[debug] encoded instr: %04x\n", instr);
            program[pc] = instr;
            pc++;
            cbit = 15;
            line++;
        }
        
        else if(strcmp(tok,"jmp")==0) {
            write_4bit(&instr, 8, &cbit);
            write_4bit(&instr, 0, &cbit);
            write_8bit(&instr, 0, &cbit);
            printf("[debug] encoded instr: %04x\n", instr);
            // Cleaning in order to write address argument
            program[pc] = instr;
            pc++;
            cbit = 15;
            instr = 0;
            
            tok = strtok (NULL,"  \t\n,;");
            
            // Decimal value
            if(tok[0] == '#') {
                tok++;
                if(atoi(tok) <= 65535) {
                    write_16bit(&instr, atoi(tok), &cbit);
                }
                else {
                    printf("Syntax error, line: %d invalid memory address\n", line);
                    free(buffer);
                    delete_label(detectedLabels);
                    return 0;
                }
            }
            
            // Hexadecimal value
            else if(tok[0] == '0' && tok[1] == 'x') {
                tok++;
                tok++; // tok = tok + 2 is less sneaky >_>
                int number = (int)strtol(tok, NULL, 16);
                if(number <= 65535) {
                    write_16bit(&instr, number, &cbit);
                }
                else {
                    printf("Syntax error, line: %d invalid memory address\n", line);
                    free(buffer);
                    delete_label(detectedLabels);
                    return 0;
                }
            }
            
            // Label
            else {
                templine = search_label(detectedLabels, tok);
                if(templine != -1) {
                    write_16bit(&instr, templine, &cbit);
                }
                else {
                    printf("Syntax error, line: %d invalid label\n", line);
                    free(buffer);
                    delete_label(detectedLabels);
                    return 0;
                }
            }
            
            
            if(cbit>-1) {
                printf("Syntax error, line: %d\n", line);
                free(buffer);
                delete_label(detectedLabels);
                return 0;
            }
            
            printf("[debug] encoded jump address: 0x%04x\n", instr);
            program[pc] = instr;
            pc++;
            cbit = 15;
            line++;
        }
        
        else if(strcmp(tok,"push")==0) {
            tok = strtok (NULL,"  \t\n,;");
            if(is_a_register(tok)==1) {
                write_4bit(&instr, 9, &cbit);
                write_4bit(&instr, atoi(&tok[1]), &cbit);
                write_8bit(&instr, 1, &cbit);               // this is used as a flag
            }
            else {
                write_4bit(&instr, 9, &cbit);
                write_4bit(&instr, 0, &cbit);
                write_8bit(&instr, 0, &cbit);
                
                // Cleaning in order to write address argument
                program[pc] = instr;
                pc++;
                cbit = 15;
                instr = 0;
                
                // Decimal value
                if(tok[0] == '#') {
                    tok++;
                    if(atoi(tok) <= 65535) {
                        write_16bit(&instr, atoi(tok), &cbit);
                    }
                    else {
                        printf("Syntax error, line: %d invalid memory address\n", line);
                        free(buffer);
                        delete_label(detectedLabels);
                        return 0;
                    }
                }
                
                // Hexadecimal value
                else if(tok[0] == '0' && tok[1] == 'x') {
                    tok++;
                    tok++; // tok = tok + 2 is less sneaky >_>
                    int number = (int)strtol(tok, NULL, 16);
                    if(number <= 65535) {
                        write_16bit(&instr, number, &cbit);
                    }
                    else {
                        printf("Syntax error, line: %d invalid memory address\n", line);
                        free(buffer);
                        delete_label(detectedLabels);
                        return 0;
                    }
                }
                
                // Dunno value
                else {
                    printf("Syntax error, line: %d invalid value format\n", line);
                    free(buffer);
                    delete_label(detectedLabels);
                    return 0;
                }
            }
            
            if(cbit>-1) {
                printf("Syntax error, line: %d\n", line);
                free(buffer);
                delete_label(detectedLabels);
                return 0;
            }
            
            printf("[debug] encoded instr: 0x%04x\n", instr);
            program[pc] = instr;
            pc++;
            cbit = 15;
            line++;
        }
        
        else if(strcmp(tok,"pop")==0) {
            tok = strtok (NULL,"  \t\n,;");
            if(is_a_register(tok)==1) {
                write_4bit(&instr, 10, &cbit);
                write_4bit(&instr, atoi(&tok[1]), &cbit);
                write_8bit(&instr, 1, &cbit);                   // this is used as a flag
            }
            else {
                skip_lt = 1;
                write_4bit(&instr, 10, &cbit);
                write_4bit(&instr, 0, &cbit);
                write_8bit(&instr, 0, &cbit);
            }
            
            if(cbit>-1) {
                printf("Syntax error, line: %d\n", line);
                free(buffer);
                delete_label(detectedLabels);
                return 0;
            }
            
            printf("[debug] encoded instr: 0x%04x\n", instr);
            program[pc] = instr;
            pc++;
            cbit = 15;
            line++;
        }
        
        else if(strcmp(tok,"jz")==0) {
            write_4bit(&instr, 11, &cbit);
            write_4bit(&instr, 0, &cbit);
            write_8bit(&instr, 0, &cbit);
            printf("[debug] encoded instr: %04x\n", instr);
            // Cleaning in order to write address argument
            program[pc] = instr;
            pc++;
            cbit = 15;
            instr = 0;
            
            tok = strtok (NULL,"  \t\n,;");
            
            // Decimal value
            if(tok[0] == '#') {
                tok++;
                if(atoi(tok) <= 65535) {
                    write_16bit(&instr, atoi(tok), &cbit);
                }
                else {
                    printf("Syntax error, line: %d invalid memory address\n", line);
                    free(buffer);
                    delete_label(detectedLabels);
                    return 0;
                }
            }
            
            // Label
            else {
                templine = search_label(detectedLabels, tok);
                if(templine != -1) {
                    write_16bit(&instr, templine, &cbit);
                }
                else {
                    printf("Syntax error, line: %d invalid label\n", line);
                    free(buffer);
                    delete_label(detectedLabels);
                    return 0;
                }
            }
            
            
            if(cbit>-1) {
                printf("Syntax error, line: %d\n", line);
                free(buffer);
                delete_label(detectedLabels);
                return 0;
            }
            
            printf("[debug] encoded jump address: 0x%04x\n", instr);
            program[pc] = instr;
            pc++;
            cbit = 15;
            line++;
        }
        
        else if(strcmp(tok,"cmp")==0) {
            write_4bit(&instr, 12, &cbit);
            
            tok = strtok (NULL,"  \t\n,;");
            write_4bit(&instr, atoi(&tok[1]), &cbit);
            
            tok = strtok (NULL,"  \t\n,;");
            write_4bit(&instr, atoi(&tok[1]), &cbit);
            
            write_4bit(&instr, 0, &cbit);
            
            if(cbit>-1) {
                printf("Syntax error, line: %d\n", line);
                free(buffer);
                delete_label(detectedLabels);
                return 0;
            }
            
            printf("[debug] encoded instr: %04x\n", instr);
            program[pc] = instr;
            pc++;
            cbit = 15;
            line++;
        }
        
        else if(strcmp(tok,"mul")==0) {
            write_4bit(&instr, 13, &cbit);
            
            tok = strtok (NULL,"  \t\n,;");
            write_4bit(&instr, atoi(&tok[1]), &cbit);
            
            tok = strtok (NULL,"  \t\n,;");
            write_4bit(&instr, atoi(&tok[1]), &cbit);
            
            tok = strtok (NULL,"  \t\n,;");
            write_4bit(&instr, atoi(&tok[1]), &cbit);
            
            if(cbit>-1) {
                printf("Syntax error, line: %d\n", line);
                free(buffer);
                delete_label(detectedLabels);
                return 0;
            }
            
            printf("[debug] encoded instr: %04x\n", instr);
            program[pc] = instr;
            pc++;
            cbit = 15;
            line++;
        }
        
        else if(strcmp(tok,"mv")==0) {
            write_4bit(&instr, 14, &cbit);
            
            tok = strtok (NULL,"  \t\n,;");
            write_4bit(&instr, atoi(&tok[1]), &cbit);
            
            tok = strtok (NULL,"  \t\n,;");
            write_4bit(&instr, atoi(&tok[1]), &cbit);
            
            write_4bit(&instr, 0, &cbit);
            
            if(cbit>-1) {
                printf("Syntax error, line: %d\n", line);
                free(buffer);
                delete_label(detectedLabels);
                return 0;
            }
            
            printf("[debug] encoded instr: %04x\n", instr);
            program[pc] = instr;
            pc++;
            cbit = 15;
            line++;
        }
        
        // Label, just skip
        else if(tok[(strlen(tok))-1] == ':') {
            line++;
        }
        
        else {
            printf("Syntax error, line: %d %s is not a valid instruction\n", line, tok);
            free(buffer);
            delete_label(detectedLabels);
            return 0;
        }
        
        if(skip_lt == 0) {
            tok = strtok (NULL,"  \t\n,;");
        }
    }
    
    FILE *out = NULL;
    
    if(argc>2) {
        out = fopen(argv[2],"wb");
    }
    else {
        out = fopen("a.out","wb");
    }
    
    for(int x=0; x<pc; x++) {
        program[x] = (program[x]>>8) | (program[x]<<8);
        fwrite( &program[x], sizeof(uint16_t), 1, out );
    }
    fclose(out);
    
    printf("[i] Done!\n");
    free(buffer);
    delete_label(detectedLabels);
    return 0;
}
