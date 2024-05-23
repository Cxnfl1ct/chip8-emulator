#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <string.h>
#include <windows.h>

#define ROM_OFFSET 0x200
#define SCREEN_X 64
#define SCREEN_Y 32
#define RAM_SIZE 4096
#define STACK_START 320
#define STACK_END 256
#define RAND_MAX 255

unsigned char reg[8];
short i;
unsigned char timer[2]; // Timer 0: Delay timer   Timer 1: Sound timer
unsigned char ram[RAM_SIZE];

char rompath[512];
char rom[4096];

const unsigned char fontset[80] = {
	0xF0, 0x90, 0x90, 0x90, 0xF0,		// 0
	0x20, 0x60, 0x20, 0x20, 0x70,		// 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0,		// 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0,		// 3
	0x90, 0x90, 0xF0, 0x10, 0x10,		// 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0,		// 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0,		// 6
	0xF0, 0x10, 0x20, 0x40, 0x40,		// 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0,		// 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0,		// 9
	0xF0, 0x90, 0xF0, 0x90, 0x90,		// A
	0xE0, 0x90, 0xE0, 0x90, 0xE0,		// B
	0xF0, 0x80, 0x80, 0x80, 0xF0,		// C
	0xE0, 0x90, 0x90, 0x90, 0xE0,		// D
	0xF0, 0x80, 0xF0, 0x80, 0xF0,		// E
	0xF0, 0x80, 0xF0, 0x80, 0x80		// F
};

int fb[SCREEN_X * SCREEN_Y];

int pc = 0;
int stack_size = 0;
int keys[16];

int scanKey()
{
	int n;
	char key = getchar();
	
	for (n=0; n<16; n++)
	{
		keys[n] = 0;
	}
	
	if (key == '1') keys[1]  = 1; // Scan for pressed keys and return corresponding value
	if (key == '2') keys[2]  = 1;
	if (key == '3') keys[3]  = 1;  // 1 2 3 4
	if (key == '4') keys[12] = 1;  // q w e r
	if (key == 'q') keys[4]  = 1;  // a s d f
	if (key == 'w') keys[5]  = 1;  // z x c v
	if (key == 'e') keys[6]  = 1;
	if (key == 'r') keys[13] = 1;  // 1 2 3 C
	if (key == 'a') keys[7]  = 1;  // 4 5 6 D
	if (key == 's') keys[8]  = 1;  // 7 8 9 E
	if (key == 'd') keys[9]  = 1;  // A 0 B F
	if (key == 'f') keys[14] = 1;
	if (key == 'z') keys[10] = 1;
	if (key == 'x') keys[0]  = 1;
	if (key == 'c') keys[11] = 1;
	if (key == 'v') keys[15] = 1;
}

int render()
{
	int i, j;
	printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"); // Clear screen (This will be probably removed after switching to GUI
	for (i=0; i<SCREEN_Y; i++) // Vertical loop
	{
		for (j=0; j<SCREEN_X; j++) // Horizontal loop
		{
			if (fb[j+(i*SCREEN_X)] = 0) // Check if currently pointed pixel is black
			{
				printf("□"); // Print black pixel
			} else {
				printf("■"); // Print white pixel
			}
		}
		printf("\n"); // New line
	}
}

void push(short value)
{
	unsigned char val = value >> 8;
	unsigned char val2 = value & 0x00FF;
	if (stack_size == 32) { printf("Error: Stack overflow, emulator cannot continue"); exit(1); }
	ram[STACK_START - stack_size] = val;
	ram[STACK_START - (stack_size - 1)] = val2;
	
	stack_size += 2;
}



int pop()
{
	short value;
	if (stack_size <= 0) { printf("Error: Stack underflow, emulator cannot continue"); exit(2); }
	value = ram[STACK_START - stack_size] << 8;
	value += ram[STACK_START - (stack_size - 1)];
	ram[STACK_START - stack_size] = '\0';
	ram[STACK_START - (stack_size - 1)] = '\0';
	
	return value;
}

void jump(int addr)
{
	pc = addr;
}

void resetScreen()
{
	for (i=0; i<SCREEN_X*SCREEN_Y; i++)
		fb[i] = 0;
}

int init(char rom[])
{
	int n;
	
	memcpy(&ram, &fontset, 80); // Copy fontset to RAM address 0x0-0x80 
	memcpy(&ram+ROM_OFFSET, &rom, sizeof(rom) ); // Copy program to RAM starting from address 0x200 (Adjusted using ROM_OFFSET)
	
	/*
	for (n=0; n<sizeof(fontset); n++)
	{
		ram[n] = fontset[n];
	}
	
	for (n=ROM_OFFSET; n<sizeof(rom)+ROM_OFFSET; n++)
	{
		ram[n] = rom[n];
	}*/
	
	resetScreen();
	
	// Commented, this is already present in main function
	
	/* 
	if (sizeof(rom) > RAM_SIZE-ROM_OFFSET)
	{
		printf("Error: ROM Size is bigger than %d bytes", RAM_SIZE-ROM_OFFSET);
		exit(2);
	} */
}

void *renderer(void* arg)
{
	for (;;) render();
}

int eval(short inst)
{
	// Check below link to check full list of CHIP-8 instructions
	// Shoutout to Michael Wales for list of instructions
	// 
	// https://github.com/mwales/chip8/blob/master/customRom/all_instructions.md
	
	int d1 = inst & 0xF000;
	int d2 = inst & 0x0F00;
	int d3 = inst & 0x00F0;
	int d34 = inst & 0x00FF;
	int d4 = inst & 0x000F;
	int d24 = inst & 0x0FFF;
	
	scanKey();
	
	if ( d1 == 0x0000)
	{
		if (d34 == 0x00E0) resetScreen();
		else if (d34 == 0x00EE) jump( pop() );
		else eval(ram[d24]);
	}
	if ( d1 == 0x1000) jump(d24);
	if ( d1 == 0x2000) { push(pc+2); jump(d24); }
	if ( d1 == 0x3000) { 
		if (reg[d2] == d34) pc+=2;
	}
	if ( d1 == 0x4000) { 
		if (reg[d2] != d34) pc+=2;
	}
	if ( d1 == 0x5000) { 
		if (reg[d2] == reg[d3]) pc+=2;
	}
	if ( d1 == 0x6000) { 
		reg[d2] == d34;
	}
	if ( d1 == 0x7000) { 
		reg[d2] += d34;
	}
	if ( d1 == 0x8000) {
		if (d4 == 0x0) reg[d2] = reg[d3];
		if (d4 == 0x1) reg[d2] = reg[d2] | reg[d3];
		if (d4 == 0x2) reg[d2] = reg[d2] & reg[d3];
		if (d4 == 0x3) reg[d2] = reg[d2] ^ reg[d3];
		if (d4 == 0x4) {
			if (reg[d2] > (reg[d2] ^ reg[d3]))
			{
				reg[d2] = 0x01;
			} else {
				reg[d2] = 0x00;
			}
		}
		if (d4 == 0x5) {
			if (reg[d2] < (reg[d2] ^ reg[d3]))
			{
				reg[d2] = 0x01;
			} else {
				reg[d2] = 0x00;
			}
		}
		if (d4 == 0x6) {
			reg[15] = reg[d3] & 0x01;
			reg[d2] = reg[d3] >> 1;
		}
		if (d4 == 0x7)
		{
			reg[d2] = reg[d2] - reg[d3];
			if (reg[d2] < (reg[d2] ^ reg[d3]))
			{
				reg[d2] = 0x01;
			} else {
				reg[d2] = 0x00;
			}
		}
		if (d4 == 0xE)
		{
			reg[15] = reg[d3] & 0x01;
			reg[d2] = reg[d3] << 1;
		}
	}
	if (d1 == 0x9000) {
		if (reg[d2] != reg[d3]) pc+=2; // Increase pc by 2 to skip instruction
	}
	if (d1 == 0xA000) {
		i = inst & 0x0FFF; // Set I to address NNN
	}
	if (d1 == 0xB000) {
		jump((int) reg[0] + d24); // Jump to V0 + imm
	}
	if (d1 == 0xC000) {
		reg[d2] = rand() & d34; // Set VX to random number bit masked by NN
	}
	if (d1 == 0xD000) {
		int j, k, offset, fo; // Draw sprite at X, Y with N bytes (Maximum is 8)
		char byte;
		
		for (j=d3; j<(d4 > 8 ? 8 : d4); j++) // This has to be cleaned up later
		{
			offset = d3*SCREEN_X+d2;
			fo = offset + (j * 8); // 'fo' stands for final offset
			
			byte = ram[i+j];
			if ((byte & 0x80) != 0) fb[fo]   = fb[fo]   ^ 1; // Check for positive bit and toggles appropriate pixel for it
			if ((byte & 0x40) != 0) fb[fo+1] = fb[fo+1] ^ 1; // XOR Drawing
			if ((byte & 0x20) != 0) fb[fo+2] = fb[fo+2] ^ 1;
			if ((byte & 0x10) != 0) fb[fo+3] = fb[fo+3] ^ 1;
			if ((byte & 0x08) != 0) fb[fo+4] = fb[fo+4] ^ 1;
			if ((byte & 0x04) != 0) fb[fo+5] = fb[fo+5] ^ 1;
			if ((byte & 0x02) != 0) fb[fo+6] = fb[fo+6] ^ 1;
			if ((byte & 0x01) != 0) fb[fo+7] = fb[fo+7] ^ 1;
		}
	}
	if (d1 == 0xE000) {
		if (d3 == 0x0090)
		{
			if (keys[reg[d2] ] == 1) pc += 2;
		}
		if (d3 == 0x00A0)
		{
			if (keys[reg[d2] ] == 0) pc += 2;
		}
	}
	if (d1 == 0xF000) {
		if (d34 == 0x0007)
		{
			reg[d2] = timer[0]; // 
		}
		if (d34 == 0x000A)
		{
			for (;;) {
				scanKey(); // Scan key
				
				if (keys[0] == 1)  { reg[d2] = 0;  break;} // Check all of those keys and break if one of them is pressed
				if (keys[1] == 1)  { reg[d2] = 1;  break;} // yes this is dumb af
				if (keys[2] == 1)  { reg[d2] = 2;  break;}
				if (keys[3] == 1)  { reg[d2] = 3;  break;}
				if (keys[4] == 1)  { reg[d2] = 4;  break;}
				if (keys[5] == 1)  { reg[d2] = 5;  break;}
				if (keys[6] == 1)  { reg[d2] = 6;  break;}
				if (keys[7] == 1)  { reg[d2] = 7;  break;}
				if (keys[8] == 1)  { reg[d2] = 8;  break;}
				if (keys[9] == 1)  { reg[d2] = 9;  break;}
				if (keys[10] == 1) { reg[d2] = 10; break;}
				if (keys[11] == 1) { reg[d2] = 11; break;}
				if (keys[12] == 1) { reg[d2] = 12; break;}
				if (keys[13] == 1) { reg[d2] = 13; break;}
				if (keys[14] == 1) { reg[d2] = 14; break;}
				if (keys[15] == 1) { reg[d2] = 15; break;}
			}
		}
		if (d34 == 0x0015)
		{
			timer[0] = reg[d2];
		}
		if (d34 == 0x0018)
		{
			timer[1] = reg[d2];
		}
		if (d34 == 0x001E)
		{
			i += reg[d2];
		}
		if (d34 == 0x0029)
		{
			i = (reg[d2] % 16) * 5;
		}
		if (d34 == 0x0033)
		{
			int mod100 = reg[d2] % 100;
			int hundred = (reg[d2] - mod100) / 100;
			int one = reg[d2] % 10;
			int ten = (reg[d2] - hundred - one) / 10;
			ram[i]   = hundred;
			ram[i+1] = ten;
			ram[i+2] = one;
		}
		if (d34 == 0x0055)
		{
			int n;
			
			for (n=0; n<(d2 >> 8); n++)
			{
				ram[i+n] = reg[n];
			}
		}
		if (d34 == 0x0065)
		{
			int n;
			
			for (n=0; n<(d2 >> 8); n++)
			{
				reg[n] = ram[i+n];
			}
		}
	}
}

void *runtime(void* arg)
{
	for (;;)
	{
		eval(ram[pc]); // Evaluate instruction on ram address pc
		pc += 2; // We will increase pc by 2 since chip 8 instructions are 2 bytes long
	}
}

int main(int argc, char *argv[])
{
	// devcpp lame asf
	
	/* Update May 23, 2024
	   Fixed argv to make program function properly */
	
	pthread_t pthread[2];
	char p1[] = "Renderer";
	char p2[] = "Runtime";
	char rompath[512];
	int args;
	const char *fucc_u_invalid_conversion = "r";
	int n, thr_id, stat, f_size; // Init variables
	
	srand(time(NULL)); // Random seed
	
	if (strlen(argv[1]) <= 2)
	{
		printf("Error: No ROM file is provided"); // Print error and exit if no ROM files are provided
		exit(3);
	}
	
	FILE* f; // Init file pointer
	
	_fullpath(rompath, argv[1], strlen(argv[1])); // Convert relative path to absolute path
	
	f = fopen(rompath, "r"); // Open file
	
	printf("%s", argv[1]); // For debugging purposes, has to be removed or commented later
	
	f_size = GetFileSize(f, NULL); // Get size of ROM file
	
	if (f_size == 0) {
		printf("Error: Provided ROM file is either corrupted or doesn\'t exist"); // Print error and exit if size of ROM file is 0 bytes
		exit(1);
	} else if (f_size > RAM_SIZE-ROM_OFFSET) {
		printf("Error: Size of ROM file is bigger than %d bytes", RAM_SIZE-ROM_OFFSET); // Print error and exit if size of ROM file is greater than available RAM space defined on top
		exit(2);
	}
	
	char rom[f_size]; 
	
	for (n=0; n<f_size; n++)
	{
		rom[f_size] = fgetc(f); // Read ROM file until end
	} 
	
	/*
	thr_id = pthread_create(&pthread[0], NULL, renderer, (void *) p1);
	if (thr_id < 0)
	{
		perror("Failed to create renderer thread!");
		exit(EXIT_FAILURE);
	}
	
	
	thr_id = pthread_create(&pthread[1], NULL, runtime, (void*)p2);
	if (thr_id < 0)
	{
		perror("Failed to create runtime thread!");
		exit(EXIT_FAILURE);
	}*/
	
	//renderer((void *) args);
	
	// ^^^ Will be uncommented later ^^^
	
	init(rom); // Initialize emulator
	
	return 0;
}
