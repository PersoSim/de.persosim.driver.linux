/* hexString.c */
#include <string.h>
#include <stdio.h>

#include "hexString.h"

#define MASK_NIBBLE 0x0F

int HexChar2Int(char input)
{
	if (input >= '0' && input <= '9')
		return input - '0';
	else if (input >= 'A' && input <= 'F')
		return input - 'A' + 10;
	else if (input >= 'a' && input <= 'f')
		return input - 'a' + 10;
	else
		return 0;
}

int HexNibble2Char(char nibble, char* hex)
{
	if (0 <= nibble && nibble < 10)
		hex[0] = nibble + '0';
	else if (10 <= nibble && nibble < 16)
		hex[0] = nibble - 10 + 'A';
	else
		return -1;

	return 1;
}

int HexByte2Chars(char byte, char* chars)
{
	// process low nibble
	HexNibble2Char((byte >> 4) & MASK_NIBBLE, chars);
	
	// process low nibble
	HexNibble2Char(byte & MASK_NIBBLE, chars+1);
	return 2;
}

int HexByte2String(char byte, char* chars)
{
	// process content
	HexByte2Chars(byte, chars);

	//append \0
	chars[2] = '\0';

	return 2;
}

int HexInt2String(unsigned int input, char* hexString)
{

	HexByte2Chars(input >> 24, hexString);
	HexByte2Chars(input >> 16, hexString + 2);
	HexByte2Chars(input >> 8, hexString + 4);
	HexByte2Chars(input     , hexString + 6);
	hexString[8] = '\0';

	return 8;
}

int HexByteArray2String(const char* bytes, int bytesLength, char* hexString)
{
	for (int i=0; i < bytesLength; i++)
	{
		HexByte2Chars(bytes[i], hexString + 2*i);
	}

	hexString[2 * bytesLength] = '\0';
	return 2 * bytesLength;
}

int HexString2CharArray(const char* hexString, char* bytes)
{
	int hexLength = strlen(hexString);
	int binLength = hexLength / 2;
	int i, highNibbleOffset, lowNibbleOffset;

	// handle odd string length
	if (hexLength % 2)
	{
		binLength++;
		i=1;
		highNibbleOffset = -1;
		lowNibbleOffset = 0;

		bytes[0] = HexChar2Int(hexString[0]);
	}
	else
	{
		i = 0;
		highNibbleOffset = 0;
		lowNibbleOffset = 1;
	}

	for (; i < binLength; i++)
	{
		bytes[i] = HexChar2Int(hexString[2*i+highNibbleOffset]) << 4;
		bytes[i] += HexChar2Int(hexString[2*i+lowNibbleOffset]);
	}

	return binLength;
}

int HexString2Int(const char* hexString)
{
	int retVal = 0;
	int hexStringLength = strlen(hexString);

	int i = 0;
	for (i=0; i < hexStringLength; i++)
	{
		retVal = retVal << 4;
		retVal |= (HexChar2Int(hexString[i]) & 0x0F);

	}

	return retVal;
}

