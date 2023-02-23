#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>

/*	NOTES:
 *	- %llo means long long octal format
 *  - Leave space after parameter for scanf to avoid weird bug
 *	- You might wonder why I did things the way I did and I wish I had an answer
 *	- The only way you can modify memory or do any other simulator operations
 *		is to do them before you start running, once you hit a breakpoint
 *		or when running in single step mode by lifting the start lever
 *	- The starting pc octal number must be either 16 digits or have a ';' at the end
 */



// Global Variables

uint64_t memory[32768];
	
uint16_t PC;
uint16_t indexReg1, indexReg2, indexReg3, indexReg4, indexReg5, indexReg6;

int64_t AReg, QReg;

__int128 QAReg;
__int128 AQReg;

uint64_t IR;
uint64_t bBits;
uint64_t mkBits;

char instructionOrder = 'u';

int startLever = 0;
int stopLever = 0;
int jumpLever = 0;
int halted = 0;






// Get Instruction

uint64_t getInstruction(FILE *file)
{
    char line[200];
    char instruction[17];
    uint64_t octalInstruct;

    fgets(line, sizeof(line), file);

	int i, j = 0;
	for(i = 0; i < 100; i++)
	{
		//printf("line[%d] ==> %c\t", i, line[i]);
		// If empty line or line is only a comment return this and handle later
		if(line[i] == '\0' || line[0] == ';')
		{
			return -1;
		}
		// Exceeds length of an opcode
		else if(j > 16)
		{
			break;
		}
		// Hit a comment
		else if(line[i] == ';')
		{
			break;
		}
		// Is a valid digit
		else if(isdigit(line[i]))
		{
			instruction[j] = line[i];
			//printf("instruction[%d] ==> %c\n", j, instruction[j]);
			// Increment j because we were successfull
			// If none of these conditions were met (empty character/space)
			// Then we should not increment j
			j++;
		}
	}
	
	instruction[16] = '\0';
	//printf("INSTRUCTION ==> %s\n", instruction);

	octalInstruct = strtoll(instruction, NULL, 8);   // convert octal number

	//printf("Octal Instruction ==> %llo\n", octalInstruct);

    return octalInstruct;
}







// Print UI

void printUI()
{
	// Print UI
	
	printf("\n***************************\n");
	printf("o%o", PC);
	(instructionOrder=='u')? (printf(" Lower\n")) : (printf(" Upper\n")); 
	printf("Executing: %o %o %05o\n", (int)IR, (int)bBits, (int)mkBits);
	printf("AReg contains: %llo\n", AReg);
	printf("QReg contains: %llo\n", QReg);
	printf("I1 = %05llo I2 = %05llo I3 = %05llo\n", indexReg1, indexReg2, indexReg3);
	printf("I4 = %05llo I5 = %05llo I6 = %05llo\n", indexReg4, indexReg5, indexReg6);
	printf("***************************\n");
}



// Memory dump

void memoryDump(int startingAddy)
{
	FILE *myFile;

   	myFile = fopen("core.oct","w");

  	if(myFile == NULL)
   	{
    	  printf("Sorry something went wrong!");   
    	  exit(1);             
   	}
   	
   	int i;
   	for(i = 0; i < 50; i++)
   	{
   		fprintf(myFile, "%05llo\t %016llo %016llo %016llo %016llo\n", startingAddy, memory[startingAddy], memory[startingAddy + 1], memory[startingAddy + 2], memory[startingAddy + 3]);
		startingAddy += 4;
	}

   	fclose(myFile);
}
















// Handling of Opcodes



// 01 b k
void ARightShift(int k)
{
	AReg = AReg >> k;
}

// 02 b k
void QRightShift(int k)
{
	QReg = QReg >> k;
}

// 03 b k
void AQRightShift(int k)
{
	// Bring A into QA
	AQReg = AReg;
	// Move the A part to the upper 48 bits
	AQReg << 48;
	// Bitmask OR with the QReg to get it in the lower bits
	AQReg = AQReg | QReg;
	
	// Shift k times
	AQReg >> k;
	
	// Make Q be the lower half of QA
	QReg = AQReg & 077777777;
	// Make A be the upper half of QA
	AReg = AQReg & 07777777700000000;
	
}

// 04 b y
void enterQ(int y)
{
	QReg = y;
}

// 05 b k
void ALeftShift(int k)
{
	AReg = AReg << k;
}

// 06 b k
void QLeftShift(int k)
{
	QReg = QReg << k;
}

// 07 b k
void AQLeftShift(int k)
{
	// Bring A into QA
	AQReg = AReg;
	// Move the A part to the upper 48 bits
	AQReg << 48;
	// Bitmask OR with the QReg to get it in the lower bits
	AQReg = AQReg | QReg;
	
	// Shift k times
	AQReg << k;
	
	// Make Q be the lower half of QA
	QReg = AQReg & 077777777;
	// Make A be the upper half of QA
	AReg = AQReg & 07777777700000000;
}

// 10 b y
void enterA(int y)
{
	AReg = y;
}

// 11 b y
void increaseA(int y)
{
	AReg = AReg + y;
}

// 12 b m
void loadA(int b, int m)
{
	switch(b)
	{
		case 1:
			AReg = memory[indexReg1] + m;
			break;
		case 2:
			AReg = memory[indexReg2] + m;
			break;
		case 3:
			AReg = memory[indexReg3] + m;
			break;
		case 4:
			AReg = memory[indexReg4] + m;
			break;
		case 5:
			AReg = memory[indexReg5] + m;
			break;
		case 6:
			AReg = memory[indexReg6] + m;
			break;
		default:
			AReg = memory[m];
	}
}

// 13 b m
void loadAComp(int b, int m)
{
	switch(b)
	{
		case 1:
			AReg = ~(memory[indexReg1] + m);
			break;
		case 2:
			AReg = ~(memory[indexReg2] + m);
			break;
		case 3:
			AReg = ~(memory[indexReg3] + m);
			break;
		case 4:
			AReg = ~(memory[indexReg4] + m);
			break;
		case 5:
			AReg = ~(memory[indexReg5] + m);
			break;
		case 6:
			AReg = ~(memory[indexReg6] + m);
			break;
		default:
			AReg = ~(memory[m]);
	}
}

// 14 b m
void add(int m)
{
	AReg = AReg + memory[m];
}

// 15 b m
void subtract(int m)
{
	AReg = AReg - memory[m];
}

// 16 b m
void loadQ(int b, int m)
{
	switch(b)
	{
		case 1:
			QReg = memory[indexReg1] + m;
			break;
		case 2:
			QReg = memory[indexReg2] + m;
			break;
		case 3:
			QReg = memory[indexReg3] + m;
			break;
		case 4:
			QReg = memory[indexReg4] + m;
			break;
		case 5:
			QReg = memory[indexReg5] + m;
			break;
		case 6:
			QReg = memory[indexReg6] + m;
			break;
		default:
			QReg = memory[m];
	}
}

// 17 b m
void loadQComp(int b, int m)
{
	switch(b)
	{
		case 1:
			QReg = ~(memory[indexReg1] + m);
			break;
		case 2:
			QReg = ~(memory[indexReg2] + m);
			break;
		case 3:
			QReg = ~(memory[indexReg3] + m);
			break;
		case 4:
			QReg = ~(memory[indexReg4] + m);
			break;
		case 5:
			QReg = ~(memory[indexReg5] + m);
			break;
		case 6:
			QReg = ~(memory[indexReg6] + m);
			break;
		default:
			QReg = ~(memory[m]);
	}
}

// 20 b m
void storeA(int m)
{
	memory[m] = AReg;
}

// 21 b m
void storeQ(int m)
{
	memory[m] = QReg;
}

// 22 j m
void AJump(int j, int m)
{
	if((j == 0) && (AReg == 0))
	{
		PC = m;
	}
	else if((j == 1) && (AReg != 0))
	{
		PC = m;
	}
	else if((j == 2) && (AReg > 0))
	{
		PC = m;
	}
	else if((j == 3) && (AReg < 0))
	{
		PC = m;
	}
	// RETURN JUMPS COMING LATER
}

// 23 j m
void QJump(int j, int m)
{
	if((j == 0) && (QReg == 0))
	{
		PC = m;
	}
	else if((j == 1) && (QReg != 0))
	{
		PC = m;
	}
	else if((j == 2) && (QReg > 0))
	{
		PC = m;
	}
	else if((j == 3) && (QReg < 0))
	{
		PC = m;
	}
}

// 24 b m
void multiplyInt(int m)
{
	QAReg = AReg * m;
	AReg = QAReg & 077777777;
	QReg = QAReg & 07777777700000000;
}

// 25 b m
void divideInt(int m)
{
	QAReg = AReg / m;
	AReg = QAReg & 077777777;
	QReg = QAReg & 07777777700000000;
}

// Skipping the floating and fraction for now

// 36 b m
void storageSkip(int m)
{
	if(memory[m] < 0)
	{
		PC++;
	}
	else if(memory[m] > 0)
	{
		if(instructionOrder == 'u')
		{
			instructionOrder = 'l';
		}
		else
		{
			instructionOrder = 'u';
		}
	}
}

// 37 b m
void storageShift(int m)
{
	if(memory[m] < 0)
	{
		PC++;
	}
	else if(memory[m] > 0)
	{
		if(instructionOrder == 'u')
		{
			instructionOrder = 'l';
		}
		else
		{
			instructionOrder = 'u';
		}
	}
	
	memory[m] = memory[m] << 1;
}

// 40 b m
void selectiveSet(int m)
{
	AReg = AReg | memory[m];
}

// 41 b m
void selectiveClear(int m)
{	
	int64_t temp = memory[m];
	
	temp = ~temp;
	AReg = ~AReg;
	AReg = AReg | memory[m];
	
	AReg = ~AReg;
}

// 42 b m
void selectiveComplement(int m)
{
	AReg = AReg ^ memory[m];
}

// 43 b m
void selectiveSub(int m)
{
	AReg = QReg & memory[m];
}

// 44 b m
void loadLogical(int m)
{
	AReg = QReg & memory[m];
}

// 45 b m
void addLogical(int m)
{
	AReg = AReg + (QReg & memory[m]);
}

// 46 b m
void subtractLogical(int m)
{
	AReg = AReg - (QReg & memory[m]);
}

// 47 b m
void storeLogical(int m)
{
	memory[m] = QReg & AReg;
}

// 50 b y
void enterIndex(int b, int y)
{
	
	switch(b)
	{
		case 1:
			indexReg1 = y;
			break;
		case 2:
			indexReg2 = y;
			break;
		case 3:
			indexReg3 = y;
			break;
		case 4:
			indexReg4 = y;
			break;
		case 5:
			indexReg5 = y;
			break;
		default:
			indexReg6 = y;
			
	}
}

// 51 b y
void increaseIndex(int b, int y)
{
	switch(b)
	{
		case 1:
			indexReg1 += y;
			break;
		case 2:
			indexReg2 += y;
			break;
		case 3:
			indexReg3 += y;
			break;
		case 4:
			indexReg4 += y;
			break;
		case 5:
			indexReg5 += y;
			break;
		default:
			indexReg6 += y;
	}
}

// 52 b m
void loadIndexUpper(int b, int m)
{
	// Get the upper 15 m bits from memory[m]
	int64_t temp = memory[m] & 07777700000000;
	
	// Shift into the leading 15 bits
	temp = temp >> 24;
	
	// Store into the desired index register
	switch(b)
	{
		case 1:
			indexReg1 = temp;
			break;
		case 2:
			indexReg2 = temp;
			break;
		case 3:
			indexReg3 = temp;
			break;
		case 4:
			indexReg4 = temp;
			break;
		case 5:
			indexReg5 = temp;
			break;
		default:
			indexReg6 = temp;
	}
}

// 53 b m
void loadIndexLower(int b, int m)
{
	// Get the lower 15 m bits from memory[m]
	int64_t temp = memory[m] & 077777;
	
	// Store into the desired index register
	switch(b)
	{
		case 1:
			indexReg1 = temp;
			break;
		case 2:
			indexReg2 = temp;
			break;
		case 3:
			indexReg3 = temp;
			break;
		case 4:
			indexReg4 = temp;
			break;
		case 5:
			indexReg5 = temp;
			break;
		default:
			indexReg6 = temp;
	}
}

// 54 b y
void indexSkip(int b, int y)
{
	if(b == 1)
	{
		if(indexReg1 == y)
		{
			indexReg1 = 0;
			PC++;
		}
		else
		{
			indexReg1++;
			if(instructionOrder == 'u')
			{
				instructionOrder = 'l';
			}
			else
			{
				instructionOrder = 'u';
			}
		}
	}
	else if(b == 2)
	{
		if(indexReg2 == y)
		{
			indexReg2 = 0;
			PC++;
		}
		else
		{
			indexReg2++;
			if(instructionOrder == 'u')
			{
				instructionOrder = 'l';
			}
			else
			{
				instructionOrder = 'u';
			}
		}
	}
	if(b == 3)
	{
		if(indexReg3 == y)
		{
			indexReg3 = 0;
			PC++;
		}
		else
		{
			indexReg3++;
			if(instructionOrder == 'u')
			{
				instructionOrder = 'l';
			}
			else
			{
				instructionOrder = 'u';
			}
		}
	}
	if(b == 4)
	{
		if(indexReg4 == y)
		{
			indexReg4 = 0;
			PC++;
		}
		else
		{
			indexReg4++;
			if(instructionOrder == 'u')
			{
				instructionOrder = 'l';
			}
			else
			{
				instructionOrder = 'u';
			}
		}
	}
	if(b == 5)
	{
		if(indexReg5 == y)
		{
			indexReg5 = 0;
			PC++;
		}
		else
		{
			indexReg5++;
			if(instructionOrder == 'u')
			{
				instructionOrder = 'l';
			}
			else
			{
				instructionOrder = 'u';
			}
		}
	}
	if(b == 6)
	{
		if(indexReg6 == y)
		{
			indexReg6 = 0;
			PC++;
		}
		else
		{
			indexReg6++;
			if(instructionOrder == 'u')
			{
				instructionOrder = 'l';
			}
			else
			{
				instructionOrder = 'u';
			}
		}
	}
}

// 55 b m
void indexJump(int b, int m)
{
	if(b == 1 && indexReg1 != 0)
	{
		indexReg1--;
		PC = m;
	}
	else if(b == 2 && indexReg2 != 0)
	{
		indexReg2--;
		PC = m;
	}
	else if(b == 3 && indexReg3 != 0)
	{
		indexReg3--;
		PC = m;
	}
	else if(b == 4 && indexReg4 != 0)
	{
		indexReg4--;
		PC = m;
	}
	else if(b == 5 && indexReg5 != 0)
	{
		indexReg5--;
		PC = m;
	}
	else if(b == 6 && indexReg6 != 0)
	{
		indexReg6--;
		PC = m;
	}
}

// 56 b mu
void storeUpper(int b, int m)
{
	int64_t temp;
	
	// Get the bits from the desired index register
	switch(b)
	{
		case 1:
			temp = indexReg1;
			break;
		case 2:
			temp = indexReg2;
			break;
		case 3:
			temp = indexReg3;
			break;
		case 4:
			temp = indexReg4;
			break;
		case 5:
			temp = indexReg5;
			break;
		default:
			temp = indexReg6;
	}
	
	// Move to the upper 15 bits location of a word
	temp = temp << 24;
	
	// Fill Upper 15 bits with 1's
	memory[m] = memory[m] | 07777700000000;
	
	// Replace memory[m] bits with lowest 15 bits of A
	memory[m] = memory[m] & temp;
}

// 57 b ml 
void storeLower(int b, int m)
{
	int64_t temp;
	
	// Get the bits from the desired index register
	switch(b)
	{
		case 1:
			temp = indexReg1;
			break;
		case 2:
			temp = indexReg2;
			break;
		case 3:
			temp = indexReg3;
			break;
		case 4:
			temp = indexReg4;
			break;
		case 5:
			temp = indexReg5;
			break;
		default:
			temp = indexReg6;
	}
	
	// Fill Lower 15 bits with 1's
	memory[m] = memory[m] | 077777;
	
	// Replace memory[m] bits with lowest 15 bits of A
	memory[m] = memory[m] & temp;
}

// Skip a few

// 60 b mu
void substituteUpper(int m)
{
	int64_t temp;
	
	// Extract the lowest 15 bits from A
	temp = AReg & 077777;
	
	// Move to the upper 15 bits location of a word
	temp = temp << 24;
	
	// Fill Upper 15 bits with 1's
	memory[m] = memory[m] | 07777700000000;
	
	// Replace memory[m] bits with lowest 15 bits of A
	memory[m] = memory[m] & temp;
}

// 61 b ml
void substituteLower(int m)
{
	int64_t temp;
	
	// Extract the lowest 15 bits from A
	temp = AReg & 077777;
	
	// Fill Upper 15 bits with 1's
	memory[m] = memory[m] | 077777;
	
	// Replace memory[m] bits with lowest 15 bits of A
	memory[m] = memory[m] & temp;
}

// 70 b m
void replaceAdd(int m)
{
	memory[m] = memory[m] + AReg;
}

// 71 b m
void replaceSub(int m)
{
	memory[m] = memory[m] - AReg;
}

// 72 b m
void replaceAddOne(int m)
{
	memory[m] = memory[m] + 1;
}

// 73 b m
void replaceSubOne(int m)
{
	int temp = memory[m];
	temp--;
	memory[m] = temp;
}

// 75 j m
void selectiveJump(int j, int m)
{
	if(j == 0)
	{
		PC = m;
	}
	if(j == 1 && stopLever == 1)
	{
		PC = m;	
	}
}

// 76 j m
void selectiveStop(int j, int m)
{
	if(j == 0)
	{
		halted = 1;
		startLever = 0;
		PC = m;
	}
	if(j == 1 && stopLever == 1)
	{
		halted = 1;
		startLever = 0;
		PC = m;	
	}
}





















// Main

int main()
{	
	char simulatorOperation;
	uint16_t givenMemory;
	
	int breakpoint = 0;
	
	char fileName[30];
	
	char cont = ' ';
	char ssMode = 'n';
	
	FILE* fileInput;

	// Start of UI
	
	printf("Please enter the name of a file you wish to load\n");
	printf("Ensure the file is in the same directory as the program \n");
	printf("and ends with .oct when providing the name \n\n");
	
	do
	{
		printf("Enter file name ==> ");
		scanf(" %29s", fileName);
		
		// Load file into the array
		
		fileInput = fopen(fileName, "r");
		
		if(!fileInput)
		{
			printf("Something went wrong \a\n");
			exit(101);
		}
		
		// Aquire the first line of the file to set the PC
		PC = getInstruction(fileInput);

		// Loop the entire file until we hit the end
		while(!feof(fileInput))
		{
			memory[PC] = getInstruction(fileInput);
			// This essentially checks if the file had an empty line
			// Kinda a weird approach but it works
			if(memory[PC] != -1)
			{
				PC++;
			}
		}
		
		fclose(fileInput);
		
		printf("\nWe have successfully loaded in %s \n\n", fileName);
		
		printf("Would you like to load another file? y/n ==> ");
		scanf(" %c", &simulatorOperation);
		puts("");
		
	} while(simulatorOperation != 'n');
	
	// Set Initial PC
	
	printf("Where would you like the program counter (PC) to start? \n");
	printf("Enter in octal ==> ");
	scanf(" %llo", &PC); 
	printf("\n");
	
	// Simulated Levers of the 1604
	
	printf("Please select an operation to perform: \n");
	printf("   g: put the start/step lever key down \n");
	printf("   G: put the start/step lever key up \n");
	printf("   e: put the first selective stop lever key down \n");
	printf("   E: put the first selective stop lever key up \n");	
	printf("   u: put the first selective jump lever key down \n");
	printf("   U: put the first selective jump lever key up \n");
	printf("   b: set the breakpoint \n");
	printf("   m: display memory contents \n");
	printf("   d: dump memory contents \n");
	printf("   s: set memory contents \n");
	printf("   a: set the A register \n");
	printf("   q: set the Q register \n");
	printf("   p: set the PC \n");
	printf("1..6: set the corresponding index register \n");
	printf("   H: Stop the simulator \n");
	
	do
	{
		// Check if we are at a breakpoint
		if(PC == breakpoint)
		{
			printf("--Breakpoint Hit--\n");
			printf("Lifted the start lever\n");
			printf("Press 'g' to resume the execution\n");
			// When we hit a breakpoint allow the user
			// to do simulator options before putting the lever down
			startLever = 0;
		}
		
		/* Allow simulator controls while not startLever
		 * (Alternative to non-blocking IO)
		 * When a breakpoint is hit or the program is halted
		 * Users gain access to controls again
		 */
		if(startLever == 0)
		{
			printf("\nEnter ==> ");
			scanf(" %c", &simulatorOperation);
		}
	
		// Start and Stop Levers
		if(simulatorOperation == 'g')
		{
			startLever = 1;
			simulatorOperation = ' ';
			
			printf("Would you like to run in single step mode? y/n ==> ");
			scanf(" %c", &ssMode);
		}
		
		// Put start lever up
		if(simulatorOperation == 'G')
		{
			startLever = 0;
		}
		
		// Put selective stop lever down
		if(simulatorOperation == 'e')
		{
			stopLever = 1;
		}
		// Put selective stop lever up
		if(simulatorOperation == 'E')
		{
			stopLever = 0;
		}
		
		// Put selective jump lever down
		if(simulatorOperation == 'u')
		{
			jumpLever = 1;
		}
		// Put selective jump lever up
		if(simulatorOperation == 'U')
		{
			jumpLever = 0;
		}
		
		// Set a breakpoint
		if(simulatorOperation == 'b')
		{
			printf("Please enter a 5-digit octal value: ");
			scanf(" %llo", &breakpoint);

			printf("Breakpoint is now %llo\n\n", breakpoint);
		}
		
		// Print out a given memory location and what's coming next
		if(simulatorOperation == 'm')
		{	
			printf("Please enter a 5-digit octal value: ");
			scanf(" %llo", &givenMemory);
			
			int i;
			for(i = 0; i < 4; i++)
			{
				printf("M[%05llo] ==> ", givenMemory + i);
				printf("%016llo \n", memory[givenMemory+i]);
			}
			puts("");
			
		}
		
		// Memory dump to core.oct
		if(simulatorOperation == 'd')
		{
			printf("Please enter a 5-digit octal value: ");
			scanf(" %llo", &givenMemory);
			
			memoryDump(givenMemory);
		}
			
		// Set a memory location to a value
		if(simulatorOperation == 's')
		{
			printf("Please enter a 5-digit octal address: ");
			scanf(" %llo", &givenMemory);

			printf("Please enter a 16-digit octal value: ");
			scanf(" %llo", &memory[givenMemory]);
			
		}
			
		// Enter to A Register
		if(simulatorOperation == 'a')
		{
			printf("Please enter a 16-digit octal value: ");
			scanf(" %llo", &AReg);

			printf("AReg now contains %llo\n\n", AReg);

		}
			
		// Enter to Q Register
		if(simulatorOperation == 'q')
		{
			printf("Please enter a 16-digit octal value: ");
			scanf(" %llo", &QReg);

			printf("QReg now contains %016llo\n\n", QReg);
			
		}
			
		// Set the PC
		if(simulatorOperation == 'p')
		{
			printf("Please enter a 5-digit octal value: ");
			scanf(" %llo", &PC);

			printf("PC now contains %llo\n\n", PC);
			
		}
			
		// Set the Index registers
		if(simulatorOperation == '1')
		{
			printf("Please enter a 5-digit octal value: ");
			scanf(" %llo", &indexReg1);

			printf("Index Register 1 now contains %llo\n\n", indexReg1);
			
		}
		if(simulatorOperation == '2')
		{		
			printf("Please enter a 5-digit octal value: ");
			scanf(" %llo", &indexReg2);

			printf("Index Register 2 now contains %llo\n\n", indexReg2);
		}
		if(simulatorOperation == '3')
		{
			printf("Please enter a 5-digit octal value: ");
			scanf(" %llo", &indexReg3);

			printf("Index Register 3 now contains %llo\n\n", indexReg3);
		}
		if(simulatorOperation == '4')
		{
			printf("Please enter a 5-digit octal value: ");
			scanf(" %llo", &indexReg4);

			printf("Index Register 4 now contains %llo\n\n", indexReg4);
		}
		if(simulatorOperation == '5')
		{
			printf("Please enter a 5-digit octal value: ");
			scanf(" %llo", &indexReg5);

			printf("Index Register 5 now contains %llo\n\n", indexReg5);
			
		}
		if(simulatorOperation == '6')
		{
			printf("Please enter a 5-digit octal value: ");
			scanf(" %llo", &indexReg6);

			printf("Index Register 6 now contains %llo\n\n", indexReg6);
		}	
				
		
		// Pulling instructions
		
		if(startLever == 1)
		{
			if(instructionOrder == 'u')
			{
				IR = (memory[PC] & 0xFC0000000000); // Extract upper opcode
				IR = IR >> 42;
				
				bBits = (memory[PC] & 0x38000000000);
				bBits = bBits >> 39;
				
				mkBits = (memory[PC] & 0x7FFF000000);
				mkBits = mkBits >> 24;
				
			//	printf("Upper Opcode is %o \n", IR);
			//	printf("Upper b Bits is %o \n", bBits);
			//	printf("Upper m/k Bits is %05o \n", mkBits);
				instructionOrder = 'l';
				printUI();
			}
			else if(instructionOrder == 'l')
			{
				IR = (memory[PC] & 0xFC0000); // Extract upper opcode
				IR = IR >> 18;
				
				bBits = (memory[PC] & 0x38000);
				bBits = bBits >> 15;
				
				mkBits = (memory[PC] & 0x7FFF);
				
			//	printf("Lower Opcode is %o \n", IR);
			//	printf("Lower b Bits is %o \n", bBits);
			//	printf("Lower m/k bits is %05o \n", mkBits);
				instructionOrder = 'u';
				printUI();
				PC++;
			}
			
			
			
			// Choose what to do based on opcode in IR
			
			switch(IR)
			{
				// NO-OP
				case 000:
					break;
				case 001:
					ARightShift(mkBits);
					break;
				case 002:
					QRightShift(mkBits);
					break;
				case 003:
					AQRightShift(mkBits);
					break;
				case 004:
					enterQ(mkBits);
					break;
				case 005:
					ALeftShift(mkBits);
					break;
				case 006:
					QLeftShift(mkBits);
					break;
				case 007:
					AQLeftShift(mkBits); 
					break;
				case 010:
					enterA(mkBits);
					break;
				case 011:
					increaseA(mkBits);
					break;
				case 012:
					loadA(bBits, mkBits);
					break;
				case 013:
					loadAComp(bBits, mkBits);
					break;
				case 014:
					add(mkBits);
					break;
				case 015:
					subtract(mkBits);
					break;
				case 016:
					loadQ(bBits, mkBits);
					break;
				case 017:
					loadQComp(bBits, mkBits);
					break;
				case 020:
					storeA(mkBits);
					break;
				case 021:
					storeQ(mkBits);
					break;
				case 022:
					AJump(bBits, mkBits);
					break;
				case 023:
					QJump(bBits, mkBits);
					break;
				case 024:
					multiplyInt(mkBits);
					break;
				case 025:
					divideInt(mkBits);
					break;
				case 036:
					storageSkip(mkBits);
					break;
				case 037:
					storageShift(mkBits);
					break;
				case 040:
					selectiveSet(mkBits);
					break;
				case 041:
					selectiveClear(mkBits);
					break;
				case 042:
					selectiveComplement(mkBits);
					break;
				case 043:
					selectiveSub(mkBits);
					break;
				case 044:
					loadLogical(mkBits);
					break;
				case 045:
					addLogical(mkBits);
					break;
				case 046:
					subtractLogical(mkBits);
					break;
				case 047:
					storeLogical(mkBits);
					break;
				case 050:
					enterIndex(bBits, mkBits);
					break;
				case 051:
					increaseIndex(bBits, mkBits);
					break;
				case 052:
					loadIndexUpper(bBits, mkBits);
					break;
				case 053:
					loadIndexLower(bBits, mkBits);
					break;
				case 054:
					indexSkip(bBits, mkBits);
					break;
				case 055:
					indexJump(bBits, mkBits);
					break;
				case 056:
					storeUpper(bBits, mkBits);
					break;
				case 057:
					storeLower(bBits, mkBits);
					break;
				case 060:
					substituteUpper(mkBits);
					break;
				case 061:
					substituteLower(mkBits);
					break;
				case 070:
					replaceAdd(mkBits);
					break;
				case 071:
					replaceSub(mkBits);
					break;
				case 072:
					replaceAddOne(mkBits);
					break;
				case 073:
					replaceSubOne(mkBits);
					break;
				case 075:
					selectiveJump(bBits, mkBits);
					break;
				case 076:
					selectiveStop(bBits, mkBits);
					break;
				default:
					printf("INVALID OPCODE ==> %llo\n", IR);
					startLever = 0;
			}
		
		}
		
		// CHECK - Wait for input if single step is on
		if(ssMode == 'y' && startLever == 1)
		{	
			printf("Please enter 'c' to continue \n");
			printf("Or enter 'G' to lift start lever and access simulator controls\n");
			scanf(" %c", &cont);
			if(cont == 'G')
			{
				startLever = 0;
			}
		}
		
		if(halted == 1)
		{
			printf("HALTED\n");
			halted = 0;
		}
		
	} while(simulatorOperation != 'H');
	
	return 0;
}
