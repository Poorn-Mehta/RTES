#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// Useful macros
#define GET_BIT_0(x)	(x & (0x01 << 0)) >> 0
#define GET_BIT_1(x)	(x & (0x01 << 1)) >> 1
#define GET_BIT_2(x)	(x & (0x01 << 2)) >> 2
#define GET_BIT_3(x)	(x & (0x01 << 3)) >> 3
#define GET_BIT_4(x)	(x & (0x01 << 4)) >> 4
#define GET_BIT_5(x)	(x & (0x01 << 5)) >> 5
#define GET_BIT_6(x)	(x & (0x01 << 6)) >> 6
#define GET_BIT_7(x)	(x & (0x01 << 7)) >> 7

#define CALCULATE_POS(x1, x2, x3, x4)	(x1 << 0) | (x2 << 1) | (x3 << 2) | (x4 << 3)

#define GET_HEX_FROM_BITS(x0, x1, x2, x3, x4, x5, x6, x7)	(x0 << 0) | (x1 << 1) | (x2 << 2) | (x3 << 3) | (x4 << 4) | (x5 << 5) | (x6 << 6) | (x7 << 7)

// variables
uint8_t P1, P2, P3, P4;
uint8_t DB0, DB1, DB2, DB3, DB4, DB5, DB6, DB7;
uint8_t PW;
uint8_t D0, D1, D2, D3, D4, D5, D6, D7, D8, D9, D10, D11, D12;
uint8_t C1, C2, C3, C4;
uint8_t CPW;
uint8_t pos;
uint8_t data;

uint8_t ip;

// Setting the correct data, and generating the correct encoded bit stream
void test_start(void)
{
	ip = 0x23;

	DB0 = GET_BIT_0(ip);
	DB1 = GET_BIT_1(ip);
	DB2 = GET_BIT_2(ip);
	DB3 = GET_BIT_3(ip);
	DB4 = GET_BIT_4(ip);
	DB5 = GET_BIT_5(ip);
	DB6 = GET_BIT_6(ip);
	DB7 = GET_BIT_7(ip);
	
	D3 = DB0;
	D5 = DB1;
	D6 = DB2;
	D7 = DB3;
	D9 = DB4;
	D10 = DB5;
	D11 = DB6;
	D12 = DB7;

	// Calculating parity bits
	P1 = D3 ^ D5 ^ D7 ^ D9 ^ D11;
	P2 = D3 ^ D6 ^ D7 ^ D10;
	P3 = D5 ^ D6 ^ D7 ^ D12;
	P4 = D9 ^ D10 ^ D11 ^ D12;
	
	D1 = P1;
	D2 = P2;
	D4 = P3;
	D8 = P4;
	
	// Calculating PW bit
	PW = D1 ^ D2 ^ D3 ^ D4 ^ D5 ^ D6 ^ D7 ^ D8 ^ D9 ^ D10 ^ D11 ^ D12;
	
	D0 = PW;
	
	printf("\nGenerated Bit Stream: %d %d %d %d %d %d %d %d %d %d %d %d %d", D0, D1, D2, D3, D4, D5, D6, D7, D8, D9, D10, D11, D12);
}

int test_end_calculate(void)
{
	printf("\nReceived Bit Stream: %d %d %d %d %d %d %d %d %d %d %d %d %d", D0, D1, D2, D3, D4, D5, D6, D7, D8, D9, D10, D11, D12);
	
	// Calculating received PW
	CPW = D0 ^ D1 ^ D2 ^ D3 ^ D4 ^ D5 ^ D6 ^ D7 ^ D8 ^ D9 ^ D10 ^ D11 ^ D12;
	
	// Calculating expected parity bits
	C1 = D1 ^ D3 ^ D5 ^ D7 ^ D9 ^ D11;
	C2 = D2 ^ D3 ^ D6 ^ D7 ^ D10;
	C3 = D4 ^ D5 ^ D6 ^ D7 ^ D12;
	C4 = D8 ^ D9 ^ D10 ^ D11 ^ D12;
	
	// Handling all test cases
	if(CPW == 0)
	{
		if((C1 != 0) || (C2 != 0) || (C3 != 0) || (C4 != 0))
		{
			printf("\nBAD NEWS: Double Bit Error Detected. Should go in checkstop. I will just return with error");
			return (-1);
		}
		else
		{
			printf("\nNO ERROR. ENJOY YOUR GOOD DATA (while it lasts :P)");
		}
	}

	else
	{
		if((C1 != 0) || (C2 != 0) || (C3 != 0) || (C4 != 0))
		{
			printf("\nOkay so there is SBE but don't freak out, I can correct it :D");
			pos = CALCULATE_POS(C1, C2, C3, C4);
			switch(pos)
			{
				case 1:
					D1 ^= 0x01;
					printf("\nHeh! So the Parity Bit 1 was flipped.");
					break;
				case 2:
					D2 ^= 0x01;
					printf("\nHeh! So the Parity Bit 2 was flipped.");
					break;
				case 3:
					D3 ^= 0x01;
					printf("\nOkay so the Data Bit 0 was flipped.");
					break;
				case 4:
					D4 ^= 0x01;
					printf("\nHeh! So the Parity Bit 3 was flipped.");
					break;
				case 5:
					D5 ^= 0x01;
					printf("\nOkay so the Data Bit 1 was flipped.");
					break;
				case 6:
					D6 ^= 0x01;
					printf("\nOkay so the Data Bit 2 was flipped.");
					break;
				case 7:
					D7 ^= 0x01;
					printf("\nOkay so the Data Bit 3 was flipped.");
					break;
				case 8:
					D8 ^= 0x01;
					printf("\nHeh! So the Parity Bit 4 was flipped.");
					break;
				case 9:
					D9 ^= 0x01;
					printf("\nOkay so the Data Bit 4 was flipped.");
					break;
				case 10:
					D10 ^= 0x01;
					printf("\nOkay so the Data Bit 5 was flipped.");
					break;
				case 11:
					D11 ^= 0x01;
					printf("\nOkay so the Data Bit 6 was flipped.");
					break;
				case 12:
					D12 ^= 0x01;
					printf("\nOkay so the Data Bit 7 was flipped.");
					break;
				default:
					// Can only come in MBE
					printf("\nOkay I don't know what the heck is going on");
					printf("\n### This was a multiple bit error. Could only figure that out based on testing and observations. ###");
					return (-1);
			}
			
			printf("\nDon't wanna brag here but I HAVE CORRECTED THAT SBE");
			printf("\nRefined Bit Stream: %d %d %d %d %d %d %d %d %d %d %d %d %d", D0, D1, D2, D3, D4, D5, D6, D7, D8, D9, D10, D11, D12);
		}
		else
		{
			printf("\nOkay so the PW bit was flipped. Yeah I know, who cares? XD");
			
			D0 ^= 0x01;
			
			printf("\nI still corrected it anyway");
			printf("\nRefined Bit Stream: %d %d %d %d %d %d %d %d %d %d %d %d %d", D0, D1, D2, D3, D4, D5, D6, D7, D8, D9, D10, D11, D12);
			
		}	
		
		DB0 = D3; 
		DB1 = D5;
		DB2 = D6;
		DB3 = D7;
		DB4 = D9;
		DB5 = D10;
		DB6 = D11;
		DB7 = D12;
		
		data = GET_HEX_FROM_BITS(DB0, DB1, DB2, DB3, DB4, DB5, DB6, DB7);
		
		printf("\nAlso, as a Bonus - I've got your HEX Data Back as Well - it's: 0x%x", data);	
		
		// Can only come in MBE
		if(data != ip)
		{
			printf("\n### This was a multiple bit error. Could only figure that out based on testing and observations. ###");
		}
	}


	return 0;
}

int main(void)
{


	printf("\nProgram to Demonstrate Hamming Code using 8 bit data");
	printf("\nTest Byte: 0x23");
	
	printf("\n\n>>>>>>>>>> NO ERROR TEST CASE(S) <<<<<<<<<<");
	
	printf("\n\n******TEST CASE 1 - NO ERROR******");
	
	test_start();
	
	test_end_calculate();
	
	printf("\n******TEST CASE 1 END******");
	
///////////////////////////////////////////////////////////////////////////

	printf("\n\n>>>>>>>>>> SBE TEST CASE(S) <<<<<<<<<<");
	
	printf("\n\n******TEST CASE 2 - Data Bit 2 Flipped******");
	
	test_start();
	
	D6 ^= 0x01;
	
	test_end_calculate();
	
	printf("\n******TEST CASE 2 END******");
	

///////////////////////////////////////////////////////////////////////////////

	printf("\n\n******TEST CASE 3 - Parity Bit 3 Flipped******");
	
	test_start();
	
	D4 ^= 0x01;
	
	test_end_calculate();
	
	printf("\n******TEST CASE 3 END******");
	
///////////////////////////////////////////////////////////////////////////////

	printf("\n\n******TEST CASE 4 - PW Bit Flipped******");
	
	test_start();
	
	D0 ^= 0x01;
	
	test_end_calculate();
	
	printf("\n******TEST CASE 4 END******");
	
///////////////////////////////////////////////////////////////////////////

	printf("\n\n>>>>>>>>>> DBE TEST CASE(S) <<<<<<<<<<");
	
	printf("\n\n******TEST CASE 5 - Data Bit 3 & Data Bit 6 Flipped******");
	
	test_start();
	
	D7 ^= 0x01;
	D11 ^= 0x01;
	
	test_end_calculate();
	
	printf("\n******TEST CASE 5 END******");
	
///////////////////////////////////////////////////////////////////////////////

	printf("\n\n******TEST CASE 6 - Parity Bit 1 & Parity Bit 4 Flipped******");
	
	test_start();
	
	D1 ^= 0x01;
	D8 ^= 0x01;
	
	test_end_calculate();
	
	printf("\n******TEST CASE 6 END******");
	
///////////////////////////////////////////////////////////////////////////////

	printf("\n\n******TEST CASE 7 - PW & Data Bit 0 Flipped******");
	
	test_start();
	
	D0 ^= 0x01;
	D3 ^= 0x01;
	
	test_end_calculate();
	
	printf("\n******TEST CASE 7 END******");
	
///////////////////////////////////////////////////////////////////////////

	printf("\n\n>>>>>>>>>> MBE TEST CASE(S) <<<<<<<<<<");
	
	printf("\n\n******TEST CASE 8 - Data Bit 1 & Data Bit 3 & Data Bit 6 Flipped******");
	
	test_start();
	
	D5 ^= 0x01;
	D7 ^= 0x01;
	D11 ^= 0x01;
	
	test_end_calculate();
	
	printf("\n******TEST CASE 8 END******");
	
///////////////////////////////////////////////////////////////////////////////

	printf("\n\n******TEST CASE 9 - Parity Bit 1 & Parity Bit 2 & Data Bit 3 & Data Bit 6 Flipped******");
	
	test_start();
	
	D1 ^= 0x01;
	D2 ^= 0x01;
	D7 ^= 0x01;
	D11 ^= 0x01;
	
	test_end_calculate();
	
	printf("\n******TEST CASE 9 END******");
	
///////////////////////////////////////////////////////////////////////////////

	printf("\n\n******TEST CASE 10 - All Bits Flipped******");
	
	test_start();
	
	D0 ^= 0x01;
	D1 ^= 0x01;
	D2 ^= 0x01;
	D3 ^= 0x01;
	D4 ^= 0x01;
	D5 ^= 0x01;
	D6 ^= 0x01;
	D7 ^= 0x01;
	D8 ^= 0x01;
	D9 ^= 0x01;
	D10 ^= 0x01;
	D11 ^= 0x01;
	D12 ^= 0x01;
	
	test_end_calculate();
	
	printf("\n******TEST CASE 10 END******\n");
	
	
	return 0;

}
