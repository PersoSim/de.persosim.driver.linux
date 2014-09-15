#include "hexString.h"
#include <string.h>
#include <stdio.h>


int testHexChar2Int();
int testHexNibble2Char();
int testHexByte2Chars();
int testHexInt2String();
int testHexByteArray2String();
int testHexString2CharArray();
int testHexString2Int();
int assertMatchInt(int, int, char*);
int assertMatchCharArray(char*, char*, int, char*);

int main ()
{
	int nrOfErrors = 0;

	nrOfErrors += testHexChar2Int();
	nrOfErrors += testHexNibble2Char();
	nrOfErrors += testHexByte2Chars();
	nrOfErrors += testHexInt2String();
	nrOfErrors += testHexByteArray2String();
	nrOfErrors += testHexString2CharArray();
	nrOfErrors += testHexString2Int();

	printf("\nAll tests executed, %d errors found\n", nrOfErrors);
	return nrOfErrors;
}

int testHexChar2Int()
{
	printf("testHexChar2Int\n");
	int errors = 0;
	
	errors += assertMatchInt(HexChar2Int('0'),0, "HexChar2Int('0')");
	errors += assertMatchInt(HexChar2Int('1'),1, "HexChar2Int('1')");
	errors += assertMatchInt(HexChar2Int('9'),9, "HexChar2Int('9')");

	errors += assertMatchInt(HexChar2Int('a'),10, "HexChar2Int('a')");
	errors += assertMatchInt(HexChar2Int('c'),12, "HexChar2Int('c')");
	errors += assertMatchInt(HexChar2Int('f'),15, "HexChar2Int('f')");

	errors += assertMatchInt(HexChar2Int('A'),10, "HexChar2Int('A')");
	errors += assertMatchInt(HexChar2Int('C'),12, "HexChar2Int('C')");
	errors += assertMatchInt(HexChar2Int('F'),15, "HexChar2Int('F')");
 
	return errors;	
}

int testHexNibble2Char()
{
	printf("testHexNibble2Char\n");
	int errors = 0;

	char hex[1];

	int len = HexNibble2Char(0, hex);
	errors += assertMatchInt(1, len, "HexNibble2Char(0) - len ");
	errors += assertMatchInt('0', hex[0], "HexNibble2Char(0) - value");

	len = HexNibble2Char(9, hex);
	errors += assertMatchInt(1, len, "HexNibble2Char(9) - len ");
	errors += assertMatchInt('9', hex[0], "HexNibble2Char(9) - value");

	len = HexNibble2Char(10, hex);
	errors += assertMatchInt(1, len, "HexNibble2Char(10) - len ");
	errors += assertMatchInt('A', hex[0], "HexNibble2Char(10) - value");

	len = HexNibble2Char(15, hex);
	errors += assertMatchInt(1, len, "HexNibble2Char(15) - len ");
	errors += assertMatchInt('F', hex[0], "HexNibble2Char(15) - value");

	len = HexNibble2Char(16, hex);
	errors += assertMatchInt(-1, len, "HexNibble2Char(16) - len ");

	len = HexNibble2Char(-1, hex);
	errors += assertMatchInt(-1, len, "HexNibble2Char(-1) - len ");

	return errors;	
}

int testHexByte2Chars()
{
	printf("testHexByte2Chars\n");
	int errors = 0;

	char hex[2];

	int len = HexByte2Chars(0x00, hex);
	errors += assertMatchInt(2, len, "HexByte2Chars(00) - len ");
	errors += assertMatchInt('0', hex[0], "HexByte2Chars(00) - high nibble");
	errors += assertMatchInt('0', hex[1], "HexByte2Chars(00) - low nibble");

	len = HexByte2Chars(0x0F, hex);
	errors += assertMatchInt(2, len, "HexByte2Chars(0F) - len ");
	errors += assertMatchInt('0', hex[0], "HexByte2Chars(0F) - high nibble");
	errors += assertMatchInt('F', hex[1], "HexByte2Chars(0F) - low nibble");

	len = HexByte2Chars(0xF0, hex);
	errors += assertMatchInt(2, len, "HexByte2Chars(F0) - len ");
	errors += assertMatchInt('F', hex[0], "HexByte2Chars(F0) - high nibble");
	errors += assertMatchInt('0', hex[1], "HexByte2Chars(F0) - low nibble");

	len = HexByte2Chars(0xFF, hex);
	errors += assertMatchInt(2, len, "HexByte2Chars(FF) - len ");
	errors += assertMatchInt('F', hex[0], "HexByte2Chars(FF) - high nibble");
	errors += assertMatchInt('F', hex[1], "HexByte2Chars(FF) - low nibble");

	return errors;	
}

int testHexInt2String()
{
	printf("testHexInt2String\n");
	int errors = 0;

	char hex[20];
	char bin[10]; 
	int binLen;

	// simple single byte
	int bin00 = 0x00;
	int len = HexInt2String(bin00, hex);
	errors += assertMatchInt(8, len, "HexInt2String(\"00\") returned length");
	errors += assertMatchInt(8, strlen(hex), "HexInt2String(\"00\") string length");
	errors += assertMatchCharArray("00000000", hex, 3, "HexInt2String(\"00\")");
 
	// single byte, highest bit set
	int binF5 = 0xF5;
	len = HexInt2String(binF5, hex);
	errors += assertMatchInt(8, len, "HexInt2String(\"F5\") returned length");
	errors += assertMatchInt(8, strlen(hex), "HexInt2String(\"F5\") string length");
	errors += assertMatchCharArray("000000F5", hex, 3, "HexInt2String(\"F5\")");
 
	// multiple bytes
	int bin012345 = 0x012345;
	len = HexInt2String(bin012345, hex);
	errors += assertMatchInt(8, len, "HexInt2String(\"012345\") returned length");
	errors += assertMatchInt(8, strlen(hex), "HexInt2String(\"012345\") string length");
	errors += assertMatchCharArray("00012345", hex, 7, "HexInt2String(\"012345\")");

	return errors;
}

int testHexByteArray2String()
{
	printf("testHexByteArray2String\n");
	int errors = 0;

	char hex[20];
	char bin[10];
	int binLen;

	// simple single byte
	char bin00[] = {0x00};
	int len = HexByteArray2String(bin00, 1, hex);
	errors += assertMatchInt(2, len, "HexByteArray2String(\"00\") returned length");
	errors += assertMatchInt(2, strlen(hex), "HexByteArray2String(\"00\") string length");
	errors += assertMatchCharArray("00", hex, 3, "HexByteArray2String(\"00\")");

	// single byte, highest bit set
	char binF5[] = {0xF5};
	len = HexByteArray2String(binF5, 1, hex);
	errors += assertMatchInt(2, len, "HexByteArray2String(\"F5\") returned length");
	errors += assertMatchInt(2, strlen(hex), "HexByteArray2String(\"F5\") string length");
	errors += assertMatchCharArray("F5", hex, 3, "HexByteArray2String(\"F5\")");

	// multiple bytes
	char bin012345[] = {0x01, 0x23, 0x45};
	len = HexByteArray2String(bin012345, 3, hex);
	errors += assertMatchInt(6, len, "HexByteArray2String(\"012345\") returned length");
	errors += assertMatchInt(6, strlen(hex), "HexByteArray2String(\"012345\") string length");
	errors += assertMatchCharArray("012345", hex, 7, "HexByteArray2String(\"012345\")");

	return errors;
}	

int testHexString2CharArray()
{
	printf("testHexString2CharArray\n");
	int errors = 0;

	char hex[20];
	char bin[10]; 
	int binLen;

	// simple single byte
	strcpy(hex, "00");
	char exp00[] = {0x00}; 
	binLen = HexString2CharArray(hex, bin);	
	errors += assertMatchInt(binLen, 1, "HexString2CharArray(\"00\") returned length");
	errors += assertMatchCharArray(exp00, bin, binLen, "HexString2CharArray(\"00\")");
 
	// single byte, highest bit set
	strcpy(hex, "F5");
	char expF5[] = {0xF5}; 
	binLen = HexString2CharArray(hex, bin);	
	errors += assertMatchInt(binLen, 1, "HexString2CharArray(\"F5\") returned length");
	errors += assertMatchCharArray(expF5, bin, binLen, "HexString2CharArray(\"F5\")");
 
	// multiple bytes
	strcpy(hex, "012345");
	char exp012345[] = {0x01, 0x23, 0x45}; 
	binLen = HexString2CharArray(hex, bin);	
	errors += assertMatchInt(binLen, 3, "HexString2CharArray(\"012345\") returned length");
	errors += assertMatchCharArray(exp012345, bin, binLen, "HexString2CharArray(\"012345\")");

	// odd number of input chars
	strcpy(hex, "ABC");
	char expABC[] = {0x0A, 0xBC}; 
	binLen = HexString2CharArray(hex, bin);	
	errors += assertMatchInt(binLen, 2, "HexString2CharArray(\"ABC\") returned length");
	errors += assertMatchCharArray(expABC, bin, binLen, "HexString2CharArray(\"ABC\")");

	return errors;	
}

int testHexString2Int()
{
	printf("testHexString2Int\n");
	int errors = 0;

	char hex[20];
	char bin[10];
	int binLen;


	errors += assertMatchInt(HexString2Int("00"),0, "HexString2Int(\"00\")"); // simple single byte
	errors += assertMatchInt(HexString2Int("F5"),0xF5, "HexString2Int(\"F5\")"); // single byte, highest bit set
	errors += assertMatchInt(HexString2Int("012345"), 0x012345, "HexString2Int(\"012345\")"); // multiple bytes
	errors += assertMatchInt(HexString2Int("ABC"),0xABC, "HexString2Int(\"ABC\")"); // odd number of input chars

	return errors;
}

int assertMatchInt(int exp, int recv, char* msg)
{
	if (exp != recv)
	{
		printf("Testfailure. %s expected %d, received %d\n", msg, exp, recv);
		return 1;
	}
	return 0;
}

int assertMatchCharArray(char* exp, char* recv, int length, char* msg)
{
	for (int i=0; i<length; i++)
	{
		if (exp[i] != recv[i])
		{
			printf("Testfailure. %s arrays differ at position %d, expected %hhX, received %hhX\n", msg, i, exp[i], recv[i]);
			printf("Expected string: %s \n", exp);
			printf("Received string: %s \n", recv);
			return 1;
		}
	}
	return 0;
}

