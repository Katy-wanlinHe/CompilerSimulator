/*
 * trace.c: location of main() to start the simulator
 */

#include "loader.h"

int main(int argc, char** argv)
{
    // this assignment does NOT require dynamic memory - do not use 'malloc() or free()'
    // create a local varible in main to represent the CPU and then pass it around
    // to subsequent functions using a pointer!
    
    /* notice: there are test cases for part 1 & part 2 here on codio
       they are located under folders called: p1_test_cases & p2_test_cases
    
       once you code main() to open up files, you can access the test cases by typing:
       ./trace p1_test_cases/divide.obj   - divide.obj is just an example
    
       please note part 1 test case files have an OBJ and a TXT file
       the .TXT shows you what's in the .OBJ files
    
       part 2 test case files have an OBJ, an ASM, and a TXT files
       the .OBJ is what you must read in
       the .ASM is the original assembly file that the OBJ was produced from
       the .TXT file is the expected output of your simulator for the given OBJ file
    */
    MachineState CP;
    MachineState* CPU = &CP;
    FILE *my_file ;
    
    if (argc < 2){
        printf("error1: usage: ./trace <trace_file.asm> \n");
        return -1;
    }
        
    char* output = argv[1] ;
    my_file = fopen (output, "w");
    if (my_file == NULL){
       printf("error2: error in opening \n");
        return -1; 
    }
    
    Reset(CPU);
    
    for (int j = 2; j < argc; j++){
        char* filename = argv[j] ;					// name of ASM file
        ReadObjectFile(filename, CPU);
    }
    
    int pc_loc = CPU->PC;
    while (pc_loc != 0x80FF){
        UpdateMachineState(CPU, my_file);
        pc_loc = CPU->PC;
    }

    fclose(my_file);
    return 0;
}