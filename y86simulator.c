//#EID MRR2578
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define EAX registers[0]
#define ECX registers[1]
#define EDX registers[2]
#define EBX registers[3]
#define ESP registers[4]
#define EBP registers[5]
#define ESE registers[6]
#define EDI registers[7]

//registers etc
int *memoryOne[0xFFFF];
int pc = -1;
int ZF = 0;
int SF = 0;
int OF = 0;
int stat = 1;
int registers[8];
char *opcodes[0xB1];
char *regStrings[8];
int maxMemAllocated = -1;

//prototypes
int fileCheck(FILE *input);
int getBlockInfo(FILE *input);
int fillMemory(FILE *input, int address, int numElements);
void initializeArrays();
void executeOpcode(char *operation, int op, int pc);
int runMemory();
void freeMemory();

int main(int argc, char *argv[]) {
  int i = 1;
  initializeArrays();
  //attempt to open first cmd line parameter
  if(argc > 1) {
      FILE *fp = fopen(argv[1], "r");
    if(fp == NULL)
      printf("No such file exists.\n\n");
    else {
      printf("%s\n", argv[1]);
      //beggining of load sequence
      if(fileCheck(fp) == 1)        
        runMemory();
      fclose(fp);
      freeMemory();
    }    
  }
  else
    printf("%s\n", "No file entered.");  
  return 0;
}

//puts value into memory via two level memory system
void setMemory(int address, int value) {
  int topBits = address >> 16;
  if(topBits > maxMemAllocated)
    maxMemAllocated = topBits;
  if(topBits > 0xFFFF)
    topBits = 0;
  int lowBits = address - (topBits << 16);  
  int *memoryTwo = memoryOne[topBits];
  if(memoryTwo == NULL) { 
    memoryOne[topBits] = (int *)calloc(0xFFFF, sizeof(int));
    memoryTwo = memoryOne[topBits];
  }
  memoryTwo[lowBits] = value;
}

//returns value from address in memory, if memory does not exist, creates it and sets to 0
int getMemory(int address) {
  if(address > 0xFFFFFFFF) {
    stat = 3;
    runMemory();
  }
  int topBits = address >> 16;
  int lowBits = address - (topBits << 16);
  int *memoryTwo = memoryOne[topBits];
  if(memoryTwo == NULL) {
    if(topBits > maxMemAllocated)
      maxMemAllocated = topBits;
    memoryOne[topBits] = (int *)calloc(0xFFFF, sizeof(int));
    memoryTwo = memoryOne[topBits];
  }
  return memoryTwo[lowBits];
}

void setFourBytes(int address, int value) {
  int x;
  for(x = 0; x < 4; x++) {
    setMemory(address + x, ((value >> (x * 8)) & 0xff));
  }
}

int getFourBytes(int address, int offset) {
  int x, memValue;
  for (x = 3 + offset; x >= 0 + offset; x--)
    memValue = memValue << 8 | getMemory(address + x);
  return memValue;
}

//goes from 0 to highest top level memory value used and frees malloced memory
void freeMemory() {
  int x, y;
  for(x = 0; x <= maxMemAllocated; x++) {
    int *memoryTwo = memoryOne[x];
    if(memoryTwo != NULL)
      free(memoryTwo);
  }
}

//checks magic number
int fileCheck(FILE *input) {
  int magicNum = (fgetc(input) << 8 | fgetc(input));
  if(magicNum != 0x7962) {
    printf("%s\n", "This is not a Y86 file.");
    return 0;
  }  
  return getBlockInfo(input);
}

//pulls out the address and num elements of a block
int getBlockInfo(FILE *input) {
  int address, numElements;
  address = (fgetc(input) << 8 | fgetc(input));
  numElements = (fgetc(input) << 8 | fgetc(input));
  //makes sure not end of file/not a block
  if(address != EOF && numElements != EOF) {
    if(pc == -1)
      pc = address;
    return fillMemory(input, address, numElements);
  }
  return 1;
}

//puts elements into memory
int fillMemory(FILE *input, int address, int numElements) {
  int counter = 0;
  int c;
  //while loop to go through elements
  while(counter < numElements && (c = fgetc(input)) != EOF) {
    setMemory(address + counter, c);
    //printf("0x%08X, 0x%08X\n", address + counter, c);
    counter++;    
  }
  //if end of file reached and not all elements found
  //reports corruption
  if(c == EOF && counter != numElements - 1) {
    printf("%s\n", "Corrupt file.");
    return 0;
  }
  //only reached if c != EOF, so makes sure all elements
  //were found, then moves to possible next block
  if(counter = numElements - 1)
    return getBlockInfo(input);
  return 1;
}

//gathers next opcode, checks simulator status, 1 = ok, 2 = halt (see halt function), 3 = invalid address, 
//4 = inavlid register or opcode
int runMemory() {
  char *opcode = opcodes[getMemory(pc)];
  if(stat == 3) {
    printf("%s\n", "invalid address\n");
    freeMemory();
    exit(EXIT_FAILURE);
  }
  if(opcode == NULL) {
    stat = 4;
    printf("0x%08X: 0x%02X -- invalid opcode\n", pc, getMemory(pc));
    freeMemory();
    exit(EXIT_FAILURE);
  }
  if(stat == 4) {
    printf("0x%08X: 0x%08X -- invalid register in operation\n", pc, getFourBytes(pc, 0));
    freeMemory();
    exit(EXIT_FAILURE);  
  }   
  executeOpcode(opcode, getMemory(pc), pc);
}

char* codeString() {
  char ccString[4];
  ccString[0] = '.';
  ccString[1] = '.';
  ccString[2] = '.';
  ccString[3] = '\0';
  if(ZF)
    ccString[0] = 'Z';
  if(SF)
    ccString[1] = 'S';
  if(OF)
    ccString[2] = 'O';
  char* code;
  code = ccString;
  return code;
}

//operation functions begin here
void rrmovl() {
  int aReg = getMemory(pc + 1) >> 4;
  int bReg = getMemory(pc + 1) - ((aReg) << 4);
  int aValue = registers[aReg];
  int bValue = registers[bReg];
  if(aReg > 7 || bReg > 7) {stat = 4; runMemory();}
  printf("0x%08X: rrmovl %s,%s \t (%s -> 0x%08X, 0x%08X -> %s)\n", pc, regStrings[aReg], regStrings[bReg], regStrings[aReg], aValue, aValue, regStrings[bReg]);
  registers[bReg] = aValue;
  pc += 0x2;
  runMemory();
}

void irmovl() {
  int x, value;
  int bReg = getMemory(pc + 1) - ((getMemory(pc + 1) >> 4) << 4);
  if(bReg > 7) {stat = 4; runMemory();}
  value = getFourBytes(pc, 2);
  printf("0x%08X: irmovl 0x%08X,%s \t (0x%08X -> %s)\n", pc, value, regStrings[bReg], value, regStrings[bReg]);
  registers[bReg] = value;
  pc += 0x6;
  runMemory();
}

void rmmovl() {
  int x, displacement;
  int aReg = getMemory(pc + 1) >> 4;
  int bReg = getMemory(pc + 1) - ((aReg) << 4);
  int aValue = registers[aReg];
  int bValue = registers[bReg];
  if(aReg > 7 || bReg > 7) {stat = 4; runMemory();}
  displacement = getFourBytes(pc, 2);
  printf("0x%08X: rmmovl %s,0x%08X(%s) \t (%s -> 0x%08X, %s -> 0x%08X, 0x%08X -> [0x%08X])\n", pc, regStrings[aReg], displacement, regStrings[bReg], regStrings[bReg], bValue, regStrings[aReg], aValue, aValue, displacement + bValue);
  setFourBytes(registers[bReg] + displacement, registers[aReg]);
  pc += 0x6;
  runMemory();
}

void mrmovl() {
  int x, displacement, memValue;
  int aReg = getMemory(pc + 1) >> 4;
  int bReg = getMemory(pc + 1) - ((aReg) << 4);
  int aValue = registers[aReg];
  int bValue = registers[bReg];
  if(aReg > 7 || bReg > 7) {stat = 4; runMemory();}
  displacement = getFourBytes(pc, 2);
  memValue = getFourBytes(displacement + bValue, 0);
  printf("0x%08X: mrmovl 0x%08X(%s),%s \t (%s -> 0x%08X, [0x%08X] -> 0x%08X, 0x%08X -> %s)\n", pc, displacement, regStrings[bReg], regStrings[aReg], regStrings[bReg], bValue, bValue + displacement, memValue, memValue, regStrings[aReg]);
  registers[aReg] = memValue;
  pc += 0x6;
  runMemory();
}

void call() {
  int x, temp;
  for(x = 4; x >= 1; x--)
    temp = temp << 8 | getMemory(pc + x);
  int nextESP = ESP - 4;
  printf("0x%08X: call 0x%08X \t (%s -> 0x%08X, 0x%08X -> [0x%08X], 0x%08X -> %s, 0x%08X -> PC)\n", pc, temp, "\%ESP", ESP, pc + 5, nextESP, nextESP, "\%ESP", temp);  
  setFourBytes(nextESP, pc + 5);
  ESP = nextESP;  
  pc = temp;
  runMemory();  
}

void ret() {
  int nextESP = ESP + 4;
  printf("0x%08X: ret \t (%s -> 0x%08X, [0x%08X] -> 0x%08X, 0x%08X -> %s, 0x%08X -> PC)\n", pc, "\%ESP", ESP, ESP, getMemory(ESP), nextESP, "\%ESP", getMemory(ESP));
  pc = getMemory(ESP);
  ESP = nextESP;  
  runMemory();
}

void pushl() {
  int aReg = getMemory(pc + 1) >> 4;
  int aValue = registers[aReg];
  if(aReg > 7) {stat = 4; runMemory();}
  int nextESP = ESP - 4;
  printf("0x%08X: pushl %s \t (%s -> 0x%08X, %s -> 0x%08X, 0x%08X -> [0x%08X], 0x%08X -> %s)\n", pc, regStrings[aReg], regStrings[aReg], aValue, "\%ESP", ESP, aValue, nextESP, nextESP, "%ESP");
  setFourBytes(nextESP, aValue);  
  ESP = nextESP;  
  pc += 0x2;
  runMemory();
}

void popl() {
  int aReg = getMemory(pc + 1) >> 4;
  if(aReg > 7) {stat = 4; runMemory();}
  int nextESP = ESP + 4;
  printf("0x%08X: popl %s \t (%s -> 0x%08X, [0x%08X] -> 0x%08X, 0x%08X -> %s, 0x%08X -> %s)\n", pc, regStrings[aReg], "\%ESP", ESP, ESP, getFourBytes(ESP, 0), nextESP, "\%ESP", getFourBytes(ESP, 0), regStrings[aReg]);
  registers[aReg] = getFourBytes(ESP, 0);    
  ESP = nextESP;
  pc += 0x2;
  runMemory();
}

void addl() {
  int aReg = getMemory(pc + 1) >> 4;
  int bReg = getMemory(pc + 1) - ((aReg) << 4);
  int aValue = registers[aReg];
  int bValue = registers[bReg];
  if(aReg > 7 || bReg > 7) {stat = 4; runMemory();}
  int t = bValue + aValue;  
  OF = (aValue < 0 == bValue < 0) && (t < 0 != aValue < 0);  
  ZF = (t == 0);
  SF = (t < 0);  
  printf("0x%08X: addl %s,%s \t (%s -> 0x%08X, %s -> 0x%08X, 0x%08X-> %s, %s -> CC)\n", pc, regStrings[aReg], regStrings[bReg], regStrings[aReg], aValue, regStrings[bReg], bValue, t, regStrings[bReg], codeString());  
  registers[bReg] = t;
  pc += 0x2;
  runMemory();
}

void subl() {
  int aReg = getMemory(pc + 1) >> 4;
  int bReg = getMemory(pc + 1) - ((aReg) << 4);
  int aValue = registers[aReg];
  int bValue = registers[bReg];
  if(aReg > 7 || bReg > 7) {stat = 4; runMemory();}
  int t = bValue - aValue;  
  OF = (-aValue< 0 == bValue < 0) && (t < 0 != -aValue < 0);  
  if(-aValue == aValue && OF == 0)
    OF = 1;
  ZF = (t == 0);
  SF = (t < 0);
  printf("0x%08X: subl %s,%s \t (%s -> 0x%08X, %s -> 0x%08X, 0x%08X-> %s, %s -> CC)\n", pc, regStrings[aReg], regStrings[bReg], regStrings[aReg], aValue, regStrings[bReg], bValue, t, regStrings[bReg], codeString());  
  registers[bReg] = t;
  pc += 0x2;
  runMemory();
}

void andl() {
  int aReg = getMemory(pc + 1) >> 4;
  int bReg = getMemory(pc + 1) - ((aReg) << 4);
  int aValue = registers[aReg];
  int bValue = registers[bReg];
  if(aReg > 7 || bReg > 7) {stat = 4; runMemory();}
  int t = bValue & aValue;  
  ZF = (t == 0);
  SF = (t < 0);
  OF = 0;  
  printf("0x%08X: andl %s,%s \t (%s -> 0x%08X, %s -> 0x%08X, 0x%08X-> %s, %s -> CC)\n", pc, regStrings[aReg], regStrings[bReg], regStrings[aReg], aValue, regStrings[bReg], bValue, t, regStrings[bReg], codeString());  
  registers[bReg] = t;
  pc += 0x2;
  runMemory();
}

void xorl() {
  int aReg = getMemory(pc + 1) >> 4;
  int bReg = getMemory(pc + 1) - ((aReg) << 4);
  int aValue = registers[aReg];
  int bValue = registers[bReg];
  if(aReg > 7 || bReg > 7) {stat = 4; runMemory();}
  int t = bValue ^ aValue;  
  ZF = (t == 0);
  SF = (t < 0);
  OF = 0;
  printf("0x%08X: xorl %s,%s \t (%s -> 0x%08X, %s -> 0x%08X, 0x%08X-> %s, %s -> CC)\n", pc, regStrings[aReg], regStrings[bReg], regStrings[aReg], aValue, regStrings[bReg], bValue, t, regStrings[bReg], codeString());  
  registers[bReg] = t;
  pc += 0x2;
  runMemory();
}

void jmp() {
  int x, temp;
  temp = getFourBytes(pc, 1);
  printf("0x%08X: jmp 0x%08X \t (0x%08X -> PC)\n", pc, temp, temp);
  pc = temp;  
  runMemory();
}

void jle() {
  int x, temp;
  temp = getFourBytes(pc, 1);
  if((SF ^ OF) || ZF) {
    printf("0x%08X: jle 0x%08X \t (ZF->%d, SF->%d, OF->%d, 0x%08X -> PC)\n", pc, temp, ZF, SF, OF, temp);
    pc = temp;
  }
  else {
    printf("0x%08X: jle 0x%08X \t (ZF->%d, SF->%d, OF->%d, no jump)\n", pc, temp, ZF, SF, OF);
    pc += 0x5;
  }
  runMemory();
}

void jl() {
  int x, temp;
  temp = getFourBytes(pc, 1);
  if(SF ^ OF) {
    printf("0x%08X: jl 0x%08X \t (SF->%d, OF->%d, 0x%08X -> PC)\n", pc, temp, SF, OF, temp);
    pc = temp;
  }
  else {
    printf("0x%08X: jl 0x%08X \t (SF->%d, OF->%d, no jump)\n", pc, temp, SF, OF);
    pc += 0x5;
  }
  runMemory();
}

void je() {
  int x, temp;
  temp = getFourBytes(pc, 1);
  if(ZF) {
    printf("0x%08X: je 0x%08X \t (ZF->%d, 0x%08X -> PC)\n", pc, temp, ZF, temp);
    pc = temp;
  }
  else {
    printf("0x%08X: je 0x%08X \t (ZF->%d, no jump)\n", pc, temp, ZF);
    pc += 0x5;
  }
  runMemory();
}

void jne() {
  int x, temp;
  temp = getFourBytes(pc, 1);
  if(!ZF) {
    printf("0x%08X: jne 0x%08X \t (ZF->%d, 0x%08X -> PC)\n", pc, temp, ZF, temp);
    pc = temp;
  }
  else {
    printf("0x%08X: jne 0x%08X \t (ZF->%d, no jump)\n", pc, temp, ZF);
    pc += 0x5;
  }
  runMemory();
}

void jge() {
  int x, temp;
  temp = getFourBytes(pc, 1);
  if(!(SF ^ OF)) {
    printf("0x%08X: jge 0x%08X \t (SF->%d, OF->%d, 0x%08X -> PC)\n", pc, temp, SF, OF, temp);
    pc = temp;
  }
  else {
    printf("0x%08X: jge 0x%08X \t (SF->%d, OF->%d, no jump)\n", pc, temp, SF, OF);
    pc += 0x5;
  }
  runMemory();
}

void jg() {
  int x, temp;
  temp = getFourBytes(pc, 1);
  if(!(SF ^ OF) & !ZF) {
    printf("0x%08X: jg 0x%08X \t (ZF->%d, SF->%d, OF->%d, 0x%08X -> PC)\n", pc, temp, ZF, SF, OF, temp);
    pc = temp;
  }
  else {
    printf("0x%08X: jg 0x%08X \t (ZF->%d, SF->%d, OF->%d, no jump)\n", pc, temp, ZF, SF, OF);
    pc += 0x5;
  }
  runMemory();
}

void cmovle() {
  int aReg = getMemory(pc + 1) >> 4;
  int bReg = getMemory(pc + 1) - ((aReg) << 4);
  if(aReg > 7 || bReg > 7) {stat = 4; runMemory();}
  if((SF ^ OF) || ZF) {
    registers[bReg] = registers[aReg];
    printf("0x%08X: cmovle %s,%s \t (ZF->%d, SF->%d, OF->%d, %s -> 0x%08X, 0x%08X -> %s)\n", pc, regStrings[aReg], regStrings[bReg], ZF, SF, OF, regStrings[aReg], registers[aReg], registers[aReg], regStrings[bReg]);
  }
  else printf("0x%08X: cmovle %s,%s \t (ZF->%d, SF->%d, OF->%d, no move)\n", pc, regStrings[aReg], regStrings[bReg], ZF, SF, OF);
  pc += 0x2;
  runMemory();
}

void cmovl() {
  int aReg = getMemory(pc + 1) >> 4;
  int bReg = getMemory(pc + 1) - ((aReg) << 4);
  if(aReg > 7 || bReg > 7) {stat = 4; runMemory();}
  if(SF ^ OF) {
    registers[bReg] = registers[aReg];
    printf("0x%08X: cmovl %s,%s \t (SF->%d, OF->%d, %s -> 0x%08X, 0x%08X -> %s)\n", pc, regStrings[aReg], regStrings[bReg], SF, OF, regStrings[aReg], registers[aReg], registers[aReg], regStrings[bReg]);
  }
  else printf("0x%08X: cmovl %s,%s \t (SF->%d, OF->%d, no move)\n", pc, regStrings[aReg], regStrings[bReg], SF, OF);
  pc += 0x2;
  runMemory();
}

void cmove() {
  int aReg = getMemory(pc + 1) >> 4;
  int bReg = getMemory(pc + 1) - ((aReg) << 4);
  if(aReg > 7 || bReg > 7) {stat = 4; runMemory();}
  if(ZF) {
    registers[bReg] = registers[aReg];
    printf("0x%08X: cmove %s,%s \t (ZF->%d, %s -> 0x%08X, 0x%08X -> %s)\n", pc, regStrings[aReg], regStrings[bReg], ZF, regStrings[aReg], registers[aReg], registers[aReg], regStrings[bReg]);
  }
  else printf("0x%08X: cmove %s,%s \t (ZF->%d, no move)\n", pc, regStrings[aReg], regStrings[bReg], ZF);
  pc += 0x2;
  runMemory();
}

void cmovne() {
  int aReg = getMemory(pc + 1) >> 4;
  int bReg = getMemory(pc + 1) - ((aReg) << 4);
  if(aReg > 7 || bReg > 7) {stat = 4; runMemory();}
  if(!ZF) {
    registers[bReg] = registers[aReg];
    printf("0x%08X: cmovne %s,%s \t (ZF->%d, %s -> 0x%08X, 0x%08X -> %s)\n", pc, regStrings[aReg], regStrings[bReg], ZF, regStrings[aReg], registers[aReg], registers[aReg], regStrings[bReg]);
  }
  else printf("0x%08X: cmovne %s,%s \t (ZF->%d, no move)\n", pc, regStrings[aReg], regStrings[bReg], ZF);
  pc += 0x2;
  runMemory();
}

void cmovge() {
  int aReg = getMemory(pc + 1) >> 4;
  int bReg = getMemory(pc + 1) - ((aReg) << 4);
  if(aReg > 7 || bReg > 7) {stat = 4; runMemory();}
  if(!(SF ^ OF)) {
    registers[bReg] = registers[aReg];
    printf("0x%08X: cmovge %s,%s \t (SF->%d, OF->%d, %s -> 0x%08X, 0x%08X -> %s)\n", pc, regStrings[aReg], regStrings[bReg], SF, OF, regStrings[aReg], registers[aReg], registers[aReg], regStrings[bReg]);
  }
  else printf("0x%08X: cmovge %s,%s \t (SF->%d, OF->%d, no move)\n", pc, regStrings[aReg], regStrings[bReg], SF, OF);
  pc += 0x2;
  runMemory();
}

void cmovg() {
  int aReg = getMemory(pc + 1) >> 4;
  int bReg = getMemory(pc + 1) - ((aReg) << 4);
  if(aReg > 7 || bReg > 7) {stat = 4; runMemory();}
  if(!(SF ^ OF) & !ZF) {
    registers[bReg] = registers[aReg];
    printf("0x%08X: cmovg %s,%s \t (ZF->%d, SF->%d, OF->%d, %s -> 0x%08X, 0x%08X -> %s)\n", pc, regStrings[aReg], regStrings[bReg], ZF, SF, OF, regStrings[aReg], registers[aReg], registers[aReg], regStrings[bReg]);
  }
  else printf("0x%08X: cmovg %s,%s \t (ZF->%d, SF->%d, OF->%d, no move)\n", pc, regStrings[aReg], regStrings[bReg], ZF, SF, OF);
  pc += 0x2;
  runMemory();
}

void nop() {
  printf("0x%08X: nop\n", pc);
  pc += 0x1;
  runMemory();
}

void halt() {
  printf("0x%08X: halt\n", pc);
  stat = 2;
}
//end of operations

//calls correct operation
void executeOpcode(char *operation, int op, int pc) {
  if(strcmp("rrmovl", operation) == 0)
    rrmovl();
  else if(strcmp("irmovl", operation) == 0)
    irmovl();
  else if(strcmp("rmmovl", operation) == 0)
    rmmovl();
  else if(strcmp("mrmovl", operation) == 0)
    mrmovl();
  else if(strcmp("call", operation) == 0)
    call();
  else if(strcmp("ret", operation) == 0)
    ret();  
  else if(strcmp("pushl", operation) == 0)
    pushl();
  else if(strcmp("popl", operation) == 0)
    popl();  
  else if(strcmp("addl", operation) == 0)
    addl();
  else if(strcmp("subl", operation) == 0)
    subl();
  else if(strcmp("andl", operation) == 0)
    andl();
  else if(strcmp("xorl", operation) == 0)
    xorl();  
  else if(strcmp("jmp", operation) == 0)
    jmp();
  else if(strcmp("jle", operation) == 0)
    jle();
  else if(strcmp("jl", operation) == 0)
    jl();
  else if(strcmp("je", operation) == 0)
    je();
  else if(strcmp("jne", operation) == 0)
    jne();
  else if(strcmp("jge", operation) == 0)
    jge();
  else if(strcmp("jg", operation) == 0)
    jg();
  else if(strcmp("cmovle", operation) == 0)
    cmovle();
  else if(strcmp("cmovl", operation) == 0)
    cmovl();
  else if(strcmp("cmove", operation) == 0)
    cmove();
  else if(strcmp("cmovne", operation) == 0)
    cmovne();
  else if(strcmp("cmovge", operation) == 0)
    cmovge();
  else if(strcmp("cmovg", operation) == 0)
    cmovg();  
  else if(strcmp("nop", operation) == 0)
    nop();  
  else if(strcmp("halt", operation) == 0)
    halt();  
}

//opcodes and registers strings
void initializeArrays() {
  opcodes[0x00] = "halt";
  opcodes[0x10] = "nop";
  opcodes[0x20] = "rrmovl";
  opcodes[0x30] = "irmovl";
  opcodes[0x40] = "rmmovl";
  opcodes[0x50] = "mrmovl";
  opcodes[0x80] = "call";
  opcodes[0x90] = "ret";
  opcodes[0xA0] = "pushl";
  opcodes[0xB0] = "popl";
  opcodes[0x20] = "rrmovl";
  opcodes[0x21] = "cmovle";
  opcodes[0x22] = "cmovl";
  opcodes[0x23] = "cmove";
  opcodes[0x24] = "cmovne";
  opcodes[0x25] = "cmovge";
  opcodes[0x26] = "cmovg";
  opcodes[0x60] = "addl";
  opcodes[0x61] = "subl";
  opcodes[0x62] = "andl";
  opcodes[0x63] = "xorl";
  opcodes[0x70] = "jmp";
  opcodes[0x71] = "jle";
  opcodes[0x72] = "jl";
  opcodes[0x73] = "je";
  opcodes[0x74] = "jne";
  opcodes[0x75] = "jge";
  opcodes[0x76] = "jg";  
  regStrings[0] = "\%EAX";
  regStrings[1] = "\%ECX";
  regStrings[2] = "\%EDX";
  regStrings[3] = "\%EBX";
  regStrings[4] = "\%ESP";
  regStrings[5] = "\%EBP";
  regStrings[6] = "\%ESI";
  regStrings[7] = "\%EDI";
}
