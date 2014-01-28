#include <pcsclite.h>
#include <ifdhandler.h>
#include <debuglog.h>
#include <string.h>
#include "hexString.h"
#include <sys/socket.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define TRUE	0
#define FALSE	1

#define PSIM_CMD_POWEROFF	"FF000000"
#define PSIM_CMD_POWERON	"FF010000"
#define PSIM_CMD_PING		"FF900000"
#define PSIM_CMD_SWITCHOFF	"FFEE0000"
#define PSIM_CMD_RESET		"FFFF0000"
#define PSIM_CMD_LENGTH		9

//BUFFERSIZE is maximum of extended length APDU plus 2 bytes for status word
#define BUFFERSIZE	0x10001
char intBuffer[BUFFERSIZE];

#define DEVICENAMESIZE 200
char Hostname[DEVICENAMESIZE];
char Port[DEVICENAMESIZE];
int simSocket = -1;

int CachedAtrLength = 0;
char CachedAtr[MAX_ATR_SIZE];

RESPONSECODE IFDHCreateChannelByName(DWORD Lun, LPSTR DeviceName)
{
	Log3(PCSC_LOG_DEBUG, "IFDHCreateChannelByName (Lun %d, DeviceName %s)",
	     Lun, DeviceName);
	// extract Hostname and Port from DeviceName
	char* colon = strchr(DeviceName, ':');
	if (colon)
	{
		colon[0] = '\0';
		strcpy(Hostname, DeviceName);
		colon++;
		strcpy(Port, colon);
	}
	else
	{
		strcpy(Hostname, "localhost");
		strcpy(Port, "9876");
		Log3(PCSC_LOG_INFO, "DEVICENAME malformed, using default %s:%s instead", Hostname, Port);
	} 
	Log2(PCSC_LOG_DEBUG, "Hostname _%s_", Hostname);
	Log2(PCSC_LOG_DEBUG, "Port _%s_", Port);
	
	return IFDHCreateChannel(Lun, 0x00);
}

RESPONSECODE
IFDHControl(DWORD Lun, DWORD dwControlCode, PUCHAR
	    TxBuffer, DWORD TxLength, PUCHAR RxBuffer, DWORD RxLength,
	    LPDWORD pdwBytesReturned)
{
	// not yet supported, will be needed to implement standard reader functionality
	// Log2(PCSC_LOG_DEBUG, "IFDHControl (Lun %d)", Lun);
	return IFD_NOT_SUPPORTED;
}

RESPONSECODE IFDHCreateChannel(DWORD Lun, DWORD Channel)
{
	Log3(PCSC_LOG_DEBUG, "IFDHCreateChannel (Lun %d, Channel 0x%x)", Lun,
	     Channel);

	// find the remote address (candidates)
	struct addrinfo hints, *servinfo, *p;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if (getaddrinfo(Hostname, Port, &hints, &servinfo) != 0) {
		Log3(PCSC_LOG_ERROR, "Unable to resolve %s:%d", Hostname, Port);
		return IFD_NO_SUCH_DEVICE;
	}

	// loop through all the results and connect to the first that works
	for(p = servinfo; p != NULL; p = p->ai_next) {
		// allocate socket
		if ((simSocket = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			// socket could not be allocated, try next one
			continue;
		}

		// connect to server
		if (connect(simSocket, p->ai_addr, p->ai_addrlen) == -1) {
			// connection failed, close socket and try next one
			close(simSocket);
			continue;
		}

		break;
	}

	if (p == NULL) {
		freeaddrinfo(servinfo);
		Log3(PCSC_LOG_ERROR, "Unable to connect to %s:%d", Hostname, Port);
		return IFD_COMMUNICATION_ERROR;
	}
	//free all found addresses
	freeaddrinfo(servinfo);


	// powerOn the simulator (in order to keep the connection alive)
	int AtrLength = MAX_ATR_SIZE;
	IFDHPowerICC(Lun, IFD_POWER_UP, intBuffer, &AtrLength);


	Log3(PCSC_LOG_DEBUG, "Socket connected to %s:%d", Hostname,
	     Port);
	return IFD_SUCCESS;
}

RESPONSECODE IFDHCloseChannel(DWORD Lun)
{
	Log2(PCSC_LOG_DEBUG, "IFDHCloseChannel (Lun %d)", Lun);
	
	// powerOff the simulator
	int AtrLength = MAX_ATR_SIZE;
	IFDHPowerICC(Lun, IFD_POWER_DOWN, intBuffer, &AtrLength);

	//close socket connection
	if (simSocket >= 0)
	{
		if (!close(simSocket))
			return IFD_COMMUNICATION_ERROR;
	}

	return IFD_SUCCESS;
}

RESPONSECODE
IFDHGetCapabilities(DWORD Lun, DWORD Tag, PDWORD Length, PUCHAR Value)
{

	switch (Tag) {
	case TAG_IFD_ATR:
		// return the ATR and its size

		// restrict returned bytes to minimum of *Length and CachedAtrLength
		if (*Length > CachedAtrLength) 
		{
			*Length = CachedAtrLength;
		}	

		memcpy(Value, CachedAtr, *Length);
		Log2(PCSC_LOG_DEBUG,
		     "IFDHGetCapabilities with tag TAG_IFD_ATR called, returned %d bytes as atr", *Length);
		return IFD_SUCCESS;
		break;
	case TAG_IFD_SIMULTANEOUS_ACCESS:
		*Value = 1;
		*Length = 1;
		break;
	case TAG_IFD_SLOTS_NUMBER:
		*Value = 1;
		*Length = 1;
		break;
	case TAG_IFD_SLOT_THREAD_SAFE:
		*Value = 0;
		*Length = 1;
		break;
	default:
		Log2(PCSC_LOG_DEBUG,
		     "IFDHGetCapabilities with unknown tag (%0#X) called", Tag);
		return IFD_ERROR_TAG;
	}

}

RESPONSECODE
IFDHSetCapabilities(DWORD Lun, DWORD Tag, DWORD Length, PUCHAR Value)
{
	// not yet supported
	// Log2(PCSC_LOG_DEBUG, "IFDHSetCapabilities (Lun %d)", Lun);
	return IFD_NOT_SUPPORTED;
}

RESPONSECODE
IFDHSetProtocolParameters(DWORD Lun, DWORD Protocol, UCHAR Flags,
			  UCHAR PTS1, UCHAR PTS2, UCHAR PTS3)
{
	// not yet supported
	// Log2(PCSC_LOG_DEBUG, "IFDHSetProtocolParameters (Lun %d)", Lun);
	return IFD_NOT_SUPPORTED;
}

RESPONSECODE IFDHPowerICC(DWORD Lun, DWORD Action, PUCHAR Atr, PDWORD AtrLength)
{
	Log2(PCSC_LOG_DEBUG, "IFDHPowerICC (Lun %d)", Lun);
	char cmdApdu[PSIM_CMD_LENGTH];
	switch (Action) {
	case IFD_POWER_DOWN:
		//TODO send PowerOff to simulator
		strcpy(cmdApdu, PSIM_CMD_POWEROFF);
		// exchangeApdu(cmdApdu, intBuffer, BUFFERSIZE);
		
		// unset cached atr
		CachedAtrLength = 0;
		memset(CachedAtr, 0, MAX_ATR_SIZE);
		
		break;
	case IFD_POWER_UP:
		// send PowerOn to simulator
		strcpy(cmdApdu, PSIM_CMD_POWERON);
		exchangeApdu(cmdApdu, intBuffer, BUFFERSIZE);

		// cache ATR from response
		CachedAtrLength = HexString2CharArray(intBuffer, CachedAtr);
		
		break;
	case IFD_RESET:
		// send Reset to simulator
		strcpy(cmdApdu, PSIM_CMD_RESET);
		exchangeApdu(cmdApdu, intBuffer, BUFFERSIZE);

		// cache ATR from response
		CachedAtrLength = HexString2CharArray(intBuffer, CachedAtr);
		break;
	default:
		Log3(PCSC_LOG_ERROR, "IFDHPowerICC (Lun %d) - unsupported Action 0x%X", Lun, Action);
		return IFD_NOT_SUPPORTED;
	}

	//return the ATR and its size (as cached above)
	return IFDHGetCapabilities(Lun, TAG_IFD_ATR, AtrLength, Atr);
}

RESPONSECODE
IFDHTransmitToICC(DWORD Lun, SCARD_IO_HEADER SendPci,
		  PUCHAR TxBuffer, DWORD TxLength, PUCHAR RxBuffer, PDWORD
		  RxLength, PSCARD_IO_HEADER RecvPci)
{
	Log2(PCSC_LOG_DEBUG, "IFDHTransmitToICC (Lun %d)", Lun);
	
	//convert APDU tho HexString
	int cmdApduSize = TxLength * 2 + 1; // 2 chars per byte plus \0
	char cmdApdu[cmdApduSize];
	HexByteArray2String(TxBuffer, TxLength, cmdApdu);

	//prepare buffer for response APDU
	int respApduSize = *RxLength * 2 + 1; // 2 chars per byte plus \0
	char respApdu[respApduSize];
	
	//perform the exchange
	exchangeApdu(cmdApdu, respApdu, respApduSize);
	

	//copy response to RxBuffer
	*RxLength = HexString2CharArray(respApdu, RxBuffer);

	return IFD_SUCCESS; 
}

RESPONSECODE IFDHICCPresence(DWORD Lun)
{
	//Log2(PCSC_LOG_DEBUG, "IFDHICCPresence (Lun %d)", Lun); 
	// send PresencePing to simulator
	char cmdApdu[PSIM_CMD_LENGTH];
	strcpy(cmdApdu, PSIM_CMD_PING);
	exchangeApdu(cmdApdu, intBuffer, BUFFERSIZE);

	if (strcmp(intBuffer, "9000") == 0)
	{
		return IFD_SUCCESS; 
	}
	else
	{
		return IFD_ICC_NOT_PRESENT;
	}
}

/*
 * Expects cmdApdu to contain a HexString and respApdu to be large enough to hold the complete response HexString both including \0 termination
 */
void exchangeApdu(const char* cmdApdu, char* respApdu, int respApduSize)
{
	Log2(PCSC_LOG_DEBUG, "exchangeApdu command APDU\n%s\n", cmdApdu);

	// transmit cmdApdu
	int len = send(simSocket, cmdApdu, strlen(cmdApdu), 0); 
	len = send(simSocket, "\n", 1, 0); 

	// receive response
	len = 0;
	do {
		len += recv(simSocket, respApdu + len, respApduSize - len, 0);
	} while (len < respApduSize && respApdu[len-1] != '\n');
	respApdu[len-1] = '\0';

	Log2(PCSC_LOG_DEBUG, "exchangeApdu response APDU\n%s\n", respApdu);
}

