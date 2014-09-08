/* hexString.h */
#ifndef HEXSTRING_H
#define HEXSTRING_H

extern int HexChar2Int(char);

extern int HexNibble2Char(char, char*);

extern int HexByte2Chars(char, char*);

extern int HexByteArray2String(const char*, int, char*);

extern int HexString2CharArray(const char*, char*);

#endif
