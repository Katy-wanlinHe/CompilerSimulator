/*
 * LC4.c: Defines simulator functions for executing instructions
 */

#include "LC4.h"
#include <stdio.h>
#define INSN_OP(I) ((I) >> 12)
#define INSN_11_9(I) (((I) >> 9) & 0x7)
#define INSN_8_6(I) (((I) >> 6) & 0x7)
#define INSN_2_0(I) (((I) >> 0) & 0x7)
#define INSN_3_0(I) (((I) >> 0) & 0xF)
#define INSN_5_3(I) (((I) >> 3) & 0x7)
#define INSN_5_0(I) (((I) >> 0) & 0x3F)
#define INSN_6_0(I) (((I) >> 0) & 0x7F)
#define INSN_4_0(I) (((I) >> 0) & 0x1F)
#define INSN_5_4(I) (((I) >> 4) & 0x3)
#define INSN_8_0(I) (((I) >> 0) & 0x1FF)
#define INSN_7_0(I) (((I) >> 0) & 0xFF)
#define INSN_8_7(I) (((I) >> 7) & 0x3)
#define INSN_10_0(I) (((I) >> 0) & 0x7FF)
#define INSN_11(I) (((I) >> 11) & 0x1)

/*
 * Reset the machine state as Pennsim would do
 */
void Reset(MachineState* CPU)
{
    int count;
    
    CPU->PC = 0x8200;
    CPU->PSR = 0x8002;
    for (count = 0 ; count < 8; count++){
        CPU->R[count] = 0;
    }
    
    for (count = 0; count < 65536; count++){
        CPU->memory[count] = 0;
    }
    
    ClearSignals(CPU);
}


/*
 * Clear all of the control signals (set to 0)
 */
void ClearSignals(MachineState* CPU)
{
    CPU->rsMux_CTL = 0;
    CPU->rtMux_CTL = 0;
    CPU->rdMux_CTL = 0;
    
    CPU->regFile_WE = 0;;
    CPU->NZP_WE = 0;
    CPU->DATA_WE = 0;
    CPU->dmemAddr = 0;
    CPU->dmemValue = 0;
    CPU->regInputVal = 0;
    CPU->NZPVal = 0;
}


/*
 * This function should write out the current state of the CPU to the file output.
 */
void WriteOut(MachineState* CPU, FILE* output)
{
        //layout what in the instructions 0x80FF
        //check for equality
    int pc, instruction, reg_we, r, rv, nzp_we, nzp, data_we, data, dmem;
    int i = 0;
    int temp, mode;
    char instr[17];
    char input[100];
 
    pc = CPU->PC;
    instruction = CPU->memory[CPU->PC];
    mode = INSN_OP(instruction);
    reg_we = CPU->regFile_WE;
    if (reg_we == 1 && mode == 15){
        r = 7;
        rv = CPU->R[7];
    } else if (reg_we == 1 && mode != 15){
        r = INSN_11_9(instruction);
        rv = CPU->R[r];
    } else {
        r = 0;
        rv = 0;
    }
   
    data_we = CPU->DATA_WE;
    while (instruction > 0){
        temp = instruction%2;
        if (temp == 0){
           instr[15-i] = '0' ;
        } else {
           instr[15-i] = '1' ; 
        }
        instruction = instruction/2;
        i++;
    }
    
    while (i <= 15){
        instr[15-i] = '0';
        i++;
    }
    
    instr[16] = '\0';
    
    if (CPU->NZP_WE != 0) {
        nzp = INSN_2_0(CPU->PSR);
    } else {
        nzp = 0;
    }
    
    if (CPU->DATA_WE != 0){
        dmem = CPU -> dmemAddr;
        data = CPU -> dmemValue;
    } else if (CPU->DATA_WE == 0 && mode == 6){
        dmem = CPU -> dmemAddr;
        data = CPU -> dmemValue;
    }else {
        dmem = 0;
        data = 0;
    }
    
    snprintf(input, sizeof(char)*100, "%04X %s %d %d %04X %d %d %d %04X %04X\n", 
            pc, instr, reg_we, r, rv, CPU->NZP_WE, nzp, data_we, 
            dmem, data);
    //check whether should remain this way or set to 0 if the WE is low
    
    fputs(input, output);
}


/*
 * This function should execute one LC4 datapath cycle.
 */
int UpdateMachineState(MachineState* CPU, FILE* output)
{
    int instr;
    int mode, addr;
    int temp, num, signedn;
    unsigned int sn;
    int b1, i, t;
    int d, s;

    instr = CPU->memory[CPU->PC];
    mode = INSN_OP(instr);
    b1 = (CPU->PSR >> 15)& 1;
    if ((CPU->PC < 0 || CPU->PC > 0x2000) && b1 == 0){
        CPU->PC = 0x80FF;
        mode = 99; 
    }
    switch (mode){
        case 0:
            BranchOp(CPU, output);
            break;
        case 1:
            ArithmeticOp(CPU, output);
            break;
        case 2:
            ComparativeOp(CPU, output);
            break;
        case 5: 
            LogicalOp(CPU, output);
            break;
        case 12:
            JumpOp(CPU, output);
            break;
        case 4:
            JSROp(CPU, output);
            break;
        case 10:
            ShiftModOp(CPU, output);
            break;
        case 8: //RTI
            CPU->PSR &= ~(1 << 15);
            CPU->NZP_WE = 0;
            CPU ->regFile_WE = 0;
            WriteOut(CPU, output);
            CPU->PC = CPU->R[7];
            break;
        case 15: //TRAP
            temp = INSN_8_0(CPU->memory[CPU->PC]);
            CPU->PSR |= 1 << 15;
            CPU->regFile_WE = 1;
            CPU->R[7] = CPU->PC+1;
            SetNZP (CPU, CPU->R[7]);
            WriteOut(CPU, output); 
            CPU->PC = (0x8000|temp);
            break;
        case 7: //STR
            //to be completed
            CPU->DATA_WE = 1;
            CPU ->NZP_WE = 0;
            CPU->regFile_WE = 0;
            CPU->rdMux_CTL = 0;
            d = INSN_11_9(CPU->memory[CPU->PC]);
            CPU->rsMux_CTL = 0;
            s = INSN_8_6(CPU->memory[CPU->PC]);
            temp = INSN_5_0(CPU->memory[CPU->PC]);
            b1 = (num >> 5) && 1;
            if (b1 == 0){
                signedn = temp;
            } else {
               signedn = 0;
               for (i =0; i < 5; i++){
               b1 = temp%2;
                   if (b1==1){
                    signedn |= 0 << i;
                   } else{
                    signedn |= 1 << i;
                   }
               temp = temp/2;
               }
               signedn = (-1)*(signedn+1); 
            }
            CPU->dmemAddr = (CPU->R[s]) + signedn;
            b1 = (CPU->PSR >> 15)& 1;
            if ((CPU->dmemAddr < 0x2000 || CPU->dmemAddr >0x8000) && b1 ==0){
                CPU->PC = 0x80FF;
            } else {
                CPU->dmemValue = CPU->R[d];
                CPU->memory[CPU->dmemAddr] = CPU->R[d];
                WriteOut(CPU, output);
                CPU->PC += 1;
            }
            CPU->DATA_WE = 0;
            break;
        case 6: //LDR //switched
            CPU->DATA_WE = 0;
            CPU ->regFile_WE = 1;
            CPU->rtMux_CTL = 1;
            d = INSN_11_9(CPU->memory[CPU->PC]);
            CPU->rsMux_CTL = 0;
            s = INSN_8_6(CPU->memory[CPU->PC]);
            temp = INSN_5_0(CPU->memory[CPU->PC]);
            b1 = (num >> 5) && 1;
            if (b1 == 0){
                signedn = temp;
            } else {
               signedn = 0;
               for (i =0; i < 5; i++){
               b1 = temp%2;
                   if (b1==1){
                    signedn |= 0 << i;
                   } else{
                    signedn |= 1 << i;
                   }
               num = num/2;
               }
               signedn = (-1)*(signedn+1); 
            }
            addr = (CPU->R[s]) + signedn;
            b1 = (CPU->PSR >> 15)& 1;
            if ((addr < 0x2000 || addr >0x8000) && b1 == 0){
                CPU->PC = 0x80FF;
            } else {
                CPU->R[d] = CPU->memory[addr]; 
                //CPU ->dmemAddr = d;
                SetNZP (CPU, CPU->R[d]);
                WriteOut(CPU, output);
                CPU->PC += 1;
            }
            CPU->DATA_WE = 0;
            break;
        case 9: //CONST
            temp = INSN_11_9(CPU->memory[CPU->PC]);
            CPU->rsMux_CTL =1;
            CPU -> rdMux_CTL = 0;
            CPU ->regFile_WE = 1;
            num = INSN_8_0(CPU->memory[CPU->PC]);
            b1 = (num >> 8) && 1;
            if (b1 == 0){
                signedn = num;
                CPU->R[temp] = INSN_8_0(CPU->memory[CPU->PC]);
            } else {
               signedn = 0;
               for (i =0; i < 8; i++){
               b1 = num%2;
                   if (b1==1){
                    signedn |= 0 << i;
                   } else{
                    signedn |= 1 << i;
                   }
               num = num/2;
               }
               signedn = (-1)*(signedn+1); 

               CPU->R[temp] = signedn;
            }
            
            SetNZP(CPU, signedn);
            WriteOut(CPU, output);
            CPU->PC+=1;
            break;
        case 13:
           //HICONST
           //to be complete
           CPU->regFile_WE = 1;
           temp = INSN_7_0(CPU->memory[CPU->PC]);
           d = INSN_11_9(CPU->memory[CPU->PC]);
           CPU->R[d] = (CPU->R[d]& 0xFF)|(temp << 8);
           SetNZP (CPU, CPU->R[d]);
            WriteOut(CPU, output);
           CPU->PC+=1;
           break;
        case 99:
            break;
            
    }
    return 0;
}



//////////////// PARSING HELPER FUNCTIONS ///////////////////////////



/*
 * Parses rest of branch operation and updates state of machine.
 */
void BranchOp(MachineState* CPU, FILE* output)
{
    int nzp, benchmark;
    int state;
    int offset;
    //set write enable
    
    nzp = CPU->PSR;
    state = INSN_11_9(CPU->memory[CPU->PC]);
    offset = INSN_8_0(CPU->memory[CPU->PC]);
    CPU->NZP_WE = 0;                  // update based on input 
    //printf ("PSR = %d \n", CPU->PSR);
    CPU->regFile_WE = 0;
   
    switch (state){
        case 0: //do nothing
            //printf ("running do nothing \n");
            CPU->PC = CPU->PC+1;
            break;
        case 1:
            //printf ("running BRp \n");
            WriteOut(CPU, output);
            if (nzp == 1) {
                CPU->PC = CPU->PC+1+offset;
            } else {
                CPU->PC = CPU->PC+1;
            }
            break;
        case 3:
            //printf ("running BRzp \n");
            WriteOut(CPU, output);
            if (nzp == 1 || nzp == 2) {
                CPU->PC = CPU->PC+1+offset;
            } else {
                CPU->PC = CPU->PC+1;
            }
            break;
        case 2:
           //printf ("running BRz \n");
           WriteOut(CPU, output);
           if (nzp == 2) {
                CPU->PC = CPU->PC+1+offset;
            } else {
                CPU->PC = CPU->PC+1;
            }
            break;
        case 4:
           //printf ("running BRn \n");
           WriteOut(CPU, output);
           if (nzp == 4) {
                CPU->PC = CPU->PC+1+offset;
            } else {
                CPU->PC = CPU->PC+1;
            }
            break;
       case 5:
           //printf ("running BRnp \n");
           WriteOut(CPU, output);
           if (nzp == 4 || nzp == 1) {
                CPU->PC = CPU->PC+1+offset;
            } else {
                CPU->PC = CPU->PC+1;
            }
            break;
       case 6:
           //printf ("running BRnz \n");
           WriteOut(CPU, output);
           if (nzp == 4 || nzp == 2) {
                CPU->PC = CPU->PC+1+offset;
            } else {
                CPU->PC = CPU->PC+1;
            }
            break;
       case 7:
           //printf ("running BRnzp \n");
           WriteOut(CPU, output);
           CPU->PC = CPU->PC+1+offset;
           break;  
    }

}

/*
 * Parses rest of arithmetic operation and prints out.
 */
void ArithmeticOp(MachineState* CPU, FILE* output)
{
   int state;
   int d,s,t;
   int i;
   int b1, b, temp;
   
   state = INSN_5_3(CPU->memory[CPU->PC]);
   d = INSN_11_9(CPU->memory[CPU->PC]);
   s = INSN_8_6(CPU->memory[CPU->PC]);
   CPU->rsMux_CTL =0;
   CPU -> rdMux_CTL = 0;
   CPU ->regFile_WE = 1;
    
   if (state == 0){
       t = INSN_2_0(CPU->memory[CPU->PC]);
       CPU->R[d] = (CPU->R[s])+(CPU->R[t]);
       CPU->NZP_WE = 1;                  // update based on input 
   } else if (state == 1){
       t = INSN_2_0(CPU->memory[CPU->PC]);
       CPU->R[d] = (CPU->R[s])*(CPU->R[t]);
       CPU->NZP_WE = 1;
   } else if (state == 2){
       t = INSN_2_0(CPU->memory[CPU->PC]);
       CPU->R[d] = (CPU->R[s])-(CPU->R[t]);
       CPU->NZP_WE = 1;
   } else if (state == 3){
       t = INSN_2_0(CPU->memory[CPU->PC]);
       CPU->R[d] = (CPU->R[s])/(CPU->R[t]);
        CPU->NZP_WE = 1;
   } else if (state >= 4) {
       t = INSN_4_0(CPU->memory[CPU->PC]);
       b1 = (t << 15);
       if (b1 == 0){
          CPU->R[d] = (CPU->R[s])+t;
       } else {
           temp = 0;
           for (i =0; i < 4; i++){
               b = t%2;
               if (b==1){
                    temp |= 0 << i;
               } else{
                    temp |= 1 << i;
               }
               t = t/2;
           }
           CPU->R[d] = (CPU->R[s])-temp-1;
       }
       printf("****the result is: %d \n", CPU->R[d]);
   }
   
    SetNZP(CPU, CPU->R[d]);
    WriteOut(CPU, output);
    CPU->PC+=1;
}

/*
 * Parses rest of comparative operation and prints out.
 */
void ComparativeOp(MachineState* CPU, FILE* output)
{
    int mode;
    int i, s, t, result;
    int signs, signt, orgt, orgs, temp;
    int b1s, b1t;
    int temp1, temp2;
    
    //set s and c
    CPU->regFile_WE = 0;
    CPU->rsMux_CTL =2;
    s = INSN_11_9(CPU->memory[CPU->PC]);
    mode = INSN_8_7(CPU->memory[CPU->PC]);
    
    switch(mode){
        case 0:
           CPU->rtMux_CTL = 1;
           t = INSN_2_0(CPU->memory[CPU->PC]);
           signs = CPU->R[s];
           signt = CPU->R[t];
            //convert R[s]
           b1s = (CPU->R[s] >> 15) & 1;
           if (b1s == 0){
              signs = CPU->R[s];
            } else {
               signs = 0;
               orgs = CPU->R[s];
               for (i =0; i < 15; i++){
                   temp1 = orgs%2;
                   if (temp1 == 1){
                        signs |= 0 << i;
                   } else{
                        signs |= 1 << i;
                   }
                       orgs = orgs/2;
                   }
               signs = (-1)*(signs + 1);
            }
             //convert R[t]
            b1t = (CPU->R[t]  >> 15) & 1;
           if (b1t == 0){
              signt = CPU->R[t];
            } else {
               signt = 0;
               orgt = CPU->R[t];
               for (i =0; i < 15; i++){
                   temp1 = orgt%2;
                   if (temp1 == 1){
                        signt |= 0 << i;
                   } else{
                        signt |= 1 << i;
                   }
                       orgt = orgt/2;
                   }
               signt = (-1)*(signt + 1);
            }
           
            result = signs-signt;
            
            break;
        case 1:
            CPU->rtMux_CTL = 1;
            t = INSN_2_0(CPU->memory[CPU->PC]);
            result = (CPU->R[s])-(CPU->R[t]);
            break;
        case 2: 
           t = INSN_6_0(CPU->memory[CPU->PC]);
           //convert R[s]
           //b1s = (CPU->R[s] << 15);
           b1s = (CPU->R[s]  >> 15) & 1;
           if (b1s == 0){
              signs = CPU->R[s];
            } else {
               signs = 0;
               orgs = CPU->R[s];
               for (i =0; i < 15; i++){
                   temp1 = orgs%2;
                   if (temp1 == 1){
                        signs |= 0 << i;
                   } else{
                        signs |= 1 << i;
                   }
                       orgs = orgs/2;
                   }
               signs = (-1)*(signs + 1);
            }

             //b1t = (t << 15);
            b1t = (t  >> 15) & 1;
           if (b1t == 0){
              signt = t;
            } else {
               signt = 0;
               orgt = t;
               for (i =0; i < 15; i++){
                   temp1 = orgt%2;
                   if (temp1 == 1){
                        signt |= 0 << i;
                   } else{
                        signt |= 1 << i;
                   }
                       orgt = orgt/2;
                   }
               signt = (-1)*(signt + 1);
            }
            
            result = signs-signt;
            break;
        case 3:
            t = INSN_6_0(CPU->memory[CPU->PC]);
            result = (CPU->R[s])-t;
            break;
    }
    
    SetNZP(CPU, result);
    WriteOut(CPU, output);
    CPU->PC+=1;
  
}

/*
 * Parses rest of logical operation and prints out.
 */
void LogicalOp(MachineState* CPU, FILE* output)
{
    int mode;
    int d, s;
    unsigned short int signs, signt, result, t;
    int b1, b1s, b1t;
    int b, i, temp;
    
    //open NZP_WE, set s and c
    CPU->rdMux_CTL = 0;
    d = INSN_11_9(CPU->memory[CPU->PC]);
    CPU->rsMux_CTL = 0;
    s = INSN_8_6(CPU->memory[CPU->PC]);
    signs = CPU->R[s] ;
    mode = INSN_5_3(CPU->memory[CPU->PC]);
    
    if (mode == 0){
        CPU->rtMux_CTL = 0;
        t = INSN_2_0(CPU->memory[CPU->PC]);
        signt = CPU->R[t];
          
        for (i =0; i < 16; i++){
            b1s = (signs >> i) && 1;
            b1t = (signt >> i) && 1;
            result |= (b1s & b1t) << i;
        }
        CPU->R[d] = result;
    }  else if (mode == 1){
        result = 0;
        for (i =0; i < 16; i++){
            b1s = (signs >> i) && 1;
            result |= (~ b1s) << i;
        }
        CPU->R[d] = result;
    } else if (mode == 2){
        CPU->rtMux_CTL = 0;
        t = INSN_2_0(CPU->memory[CPU->PC]);
        signt = CPU->R[t];
        result = 0;
        for (i =0; i < 16; i++){
            b1s = (signs >> i) && 1;
            b1t = (signt >> i) && 1;
            result |= (b1t | b1s) << i;
        }
        CPU->R[d] = result;
            
    } else if (mode >=4){
       t = INSN_4_0(CPU->memory[CPU->PC]);
       b1 = (t >> 4) & 1;
       if (b1 != 0){
           t |= 0xFFE0;
       }
        result = 0;
        for (i =0; i < 16; i++){
            b1s = (signs >> i) && 1;
            b1t = (t >> i) && 1;
            result |= (b1s && b1t) << i;
        }
        CPU->R[d] = result;
  }   
    SetNZP(CPU, result);
    WriteOut(CPU, output);
    CPU->PC+=1;
}
  



/*
 * Parses rest of jump operation and prints out.
 */
void JumpOp(MachineState* CPU, FILE* output) 
{
    int mode;
    int s, t, offset;
    int b1, temp;
    int i, b;
    
    mode = INSN_11(CPU->memory[CPU->PC]);
    CPU->regFile_WE = 0;
    CPU->NZP_WE = 0;
    
    switch (mode){
        case 0:
            CPU->rsMux_CTL = 1;
            s = INSN_8_6(CPU->memory[CPU->PC]);
            WriteOut(CPU, output);
            CPU->PC = CPU->R[s];
            break;
        case 1:
         temp = INSN_10_0(CPU->memory[CPU->PC]);
            WriteOut (CPU, output);
            CPU->PC = (CPU->PC)+1+temp;
            break;
       }
    }
    


/*
 * Parses rest of JSR operation and prints out.
 */
void JSROp(MachineState* CPU, FILE* output)
{
    int mode;
    int s, offset;
    int b1, temp;
    int b, i;
    
    mode = INSN_11(CPU->memory[CPU->PC]);
    CPU->regFile_WE = 1;
    CPU->rdMux_CTL = 1;
    CPU->R[7] = (CPU->PC)+1;
    
    switch (mode){
        case 0:
            CPU->rsMux_CTL = 1;
            s = INSN_8_6(CPU->memory[CPU->PC]);
            WriteOut (CPU, output);
            CPU->PC = CPU->R[s];
            break;
        case 1:
            temp = INSN_10_0(CPU->memory[CPU->PC]);
            b1 = temp << 15;
            b1 = (temp >> 15) & 1;
            if (b1 == 0){
                offset = temp;
            } else {
                offset = 0;
               for (i =0; i < 10; i++){
                   b = temp %2;
                   if (b==1) {
                        offset |= 0 << i;
                   } else{
                        offset |= 1 << i;
                   }
                   temp = temp/2;
               }
               offset = (-1)*(offset + 1);
           }
            WriteOut (CPU, output);
            CPU->PC = (CPU->PC)+1+temp;
            break;
       }
    }
    
    
    


/*
 * Parses rest of shift/mod operations and prints out.
 */
void ShiftModOp(MachineState* CPU, FILE* output)
{
    int mode;
    int d, s, t;
    int result;
    
    CPU->rdMux_CTL = 0;
    d = INSN_11_9(CPU->memory[CPU->PC]);
    CPU->rsMux_CTL = 0;
    CPU->regFile_WE =1;
    s = INSN_8_6(CPU->memory[CPU->PC]);
    mode = INSN_5_4(CPU->memory[CPU->PC]);
    
    if (mode == 0){
       t = INSN_3_0(CPU->memory[CPU->PC]); 
       result = (CPU->R[s]) << t;
    }else if (mode == 1){
        t = INSN_3_0(CPU->memory[CPU->PC]); 
        result = (CPU->R[s]) >> t;              //look up arithmetic shift
    } else if (mode == 2){
        t = INSN_3_0(CPU->memory[CPU->PC]); 
        result = (CPU->R[s]) >> t;
    } else if (mode == 3){
        t = INSN_2_0(CPU->memory[CPU->PC]); 
        result = (CPU->R[s]) % (CPU->R[t]);
    }
  
    CPU->R[d] = result;
    SetNZP(CPU, result);
    WriteOut(CPU, output);
    CPU->PC+=1;
}

/*
 * Set the NZP bits in the PSR.
 */
void SetNZP(MachineState* CPU, short result)
{
    //set NZP_WE
    CPU->NZP_WE = 1;
    
    if (CPU->PSR >= 0x8000){
        if (result > 0) {
          CPU->PSR = 0x8000+1; 
          CPU->NZPVal =1;
       } else if (result == 0) {
          CPU->PSR = 0x8000+2; 
          CPU->NZPVal =2;
       } else if (result < 0) {
          CPU->PSR = 0x8000+4; 
          CPU->NZPVal =4;
       }
    }else {
        if (result > 0) {
          CPU->PSR = 1; 
          CPU->NZPVal =1;
       } else if (result == 0) {
          CPU->PSR = 2; 
          CPU->NZPVal =2;
       } else if (result < 0) {
          CPU->PSR = 4; 
          CPU->NZPVal =4;
       }
    }
    
    
}
