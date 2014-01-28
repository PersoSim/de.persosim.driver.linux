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

char *Hostname = "localhost";
long Port = 9876;
int simSocket = -1;

int CachedAtrLength = 0;
char CachedAtr[MAX_ATR_SIZE];

RESPONSECODE IFDHCreateChannelByName(DWORD Lun, LPSTR DeviceName)
{
	Log3(PCSC_LOG_DEBUG, "IFDHCreateChannelByName (Lun %d, DeviceName %s)",
	     Lun, DeviceName);
	//TODO extract Hostname and Port from DeviceName
	return IFDHCreateChannel(Lun, 0x00);
}

RESPONSECODE
IFDHControl(DWORD Lun, DWORD dwControlCode, PUCHAR
	    TxBuffer, DWORD TxLength, PUCHAR RxBuffer, DWORD RxLength,
	    LPDWORD pdwBytesReturned)
{
	//TODO
	Log2(PCSC_LOG_DEBUG, "IFDHControl (Lun %d)", Lun);
}

RESPONSECODE IFDHCreateChannel(DWORD Lun, DWORD Channel)
{
	Log3(PCSC_LOG_DEBUG, "IFDHCreateChannel (Lun %d, Channel 0x%x)", Lun,
	     Channel);

	// allocate socket
	simSocket = socket(AF_INET, SOCK_STREAM, 0);

	// define the remote address
	struct sockaddr_in dest;
	memset(&dest, 0, sizeof(dest));
	dest.sin_family = AF_INET;
	dest.sin_addr.s_addr = htonl(INADDR_LOOPBACK); //TODO use Hostname
	dest.sin_port = htons(Port);

	// connect
	connect(simSocket, (struct sockadr *)&dest, sizeof(struct sockaddr));
	
	// powerOn the simulator (in order to keep the connection alive)
	int AtrLength = MAX_ATR_SIZE;
	IFDHPowerICC(Lun, IFD_POWER_UP, intBuffer, &AtrLength);


	Log3(PCSC_LOG_DEBUG, "socket connected to %s:%d", Hostname,
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
	//TODO
	Log2(PCSC_LOG_DEBUG, "IFDHSetCapabilities (Lun %d)", Lun);
}

RESPONSECODE
IFDHSetProtocolParameters(DWORD Lun, DWORD Protocol, UCHAR Flags,
			  UCHAR PTS1, UCHAR PTS2, UCHAR PTS3)
{
	//TODO
	Log2(PCSC_LOG_DEBUG, "IFDHSetProtocolParameters (Lun %d)", Lun);
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
	// send PowerOn to simulator
//	char cmdApdu[PSIM_CMD_LENGTH];
//	strcpy(cmdApdu, PSIM_CMD_PING);
//	exchangeApdu(cmdApdu, intBuffer, BUFFERSIZE);

//	if (strcmp(intBuffer, "9000"))
//	{
		return IFD_SUCCESS; 
//	}
//	else
//	{
//		return IFD_ICC_NOT_PRESENT;
//	}
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
