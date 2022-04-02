/*
 * loader.c : Defines loader functions for opening and loading object files
 */

#include "loader.h"

// memory array location
unsigned short memoryAddress;

/*
 * Read an object file and modify the machine state as described in the writeup
 */
int ReadObjectFile(char* filename, MachineState* CPU)
{
    int prg_type;          //1 = .CODE or .DATA, 2 = symbol 3 = FILE NAME, 4 = line number                        
    int address;
    int byte1, byte2;
    int num;
    int i;
    FILE *src_file ; 

    //clears memory
    
    src_file = fopen (filename, "rb");
    if (src_file == NULL){
        printf("error1: Invalid File Name \n");
        return -1;
    }
    
    //read in 2 byte of code
    byte1 = fgetc(src_file) ;
    byte2 = fgetc(src_file) ;
    prg_type = (byte1 << 8)| byte2;
    
    while (prg_type == 0xCADE || prg_type == 0xDADA || prg_type == 0xC3B7 || prg_type == 0xF17E || prg_type == 0x715E ){
        if (prg_type == 0xCADE || prg_type == 0xDADA) {
            byte1 = fgetc(src_file) ;
            byte2 = fgetc(src_file) ;
            address = (byte1 << 8)| byte2;
            
            byte1 = fgetc(src_file) ;
            byte2 = fgetc(src_file) ;
            num = (byte1 << 8)| byte2;
            
            for (i=0; i<num; i++){
                byte1 = fgetc(src_file) ;
                byte2 = fgetc(src_file) ;
                CPU->memory[address + i] = (byte1 << 8) | byte2;
            }
        } else if (prg_type == 0xC3B7){
            byte1 = fgetc(src_file) ;
            byte2 = fgetc(src_file) ;
            address = (byte1 << 8)| byte2;
            
            byte1 = fgetc(src_file) ;
            byte2 = fgetc(src_file) ;
            num = (byte1 << 8)| byte2;
            
            for (i=0; i<num; i++){
                byte1 = fgetc(src_file) ;
            }
        } else if ( prg_type == 0xF17E){
            byte1 = fgetc(src_file) ;
            byte2 = fgetc(src_file) ;
            num = (byte1 << 8)| byte2;
            
             for (i=0; i<num; i++){
                byte1 = fgetc(src_file) ;
            }
        } else if (prg_type == 0x715E){
             for (i = 0; i < 3; i++){
                byte1 = fgetc(src_file) ;
                byte2 = fgetc(src_file) ;
            }
        }
        
        byte1 = fgetc(src_file) ;
        if (byte1 == EOF) {
            break;
        }
        byte2 = fgetc(src_file) ;
        if (byte2 == EOF){
            break;
        }
    
        prg_type = (byte1 << 8)| byte2;
    }
    
    // close file
    fclose(src_file);
    return 0;
}
