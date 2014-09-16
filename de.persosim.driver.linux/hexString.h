/* hexString.h */
#ifndef HEXSTRING_H
#define HEXSTRING_H

extern int HexChar2Int(char);

extern int HexNibble2Char(char, char*);

extern int HexByte2Chars(char, char*);

extern int HexByte2String(char, char*);

extern int HexInt2String(unsigned int, char*);

extern int HexByteArray2String(const char*, int, char*);

extern int HexString2CharArray(const char*, char*);

extern int HexString2Int(const char*);

#endif
