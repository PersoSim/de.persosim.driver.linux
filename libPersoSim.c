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

//BUFFERSIZE is maximum of extended length APDU plus 2 bytes for status word
#define BUFFERSIZE	0x10001

char *Hostname = "localhost";
long Port = 9876;
int simSocket = -1;

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




#define BUFFERSIZE = 500
#define PORT = 9876

char buffer[500 + 1]; //PP BUFFER
char* msg = "0084000008\n";












	//TODO implement socket setup


	Log3(PCSC_LOG_DEBUG, "create socket connection to %s:%d", Hostname,
	     Port);
	return IFD_SUCCESS;
}

RESPONSECODE IFDHCloseChannel(DWORD Lun)
{
	Log2(PCSC_LOG_DEBUG, "IFDHCloseChannel (Lun %d)", Lun);
	
	// TODO powerOff the simulator
	// transmitControlAPDU("00FF0000");

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
		//TODO get ATR from simulator
		Log1(PCSC_LOG_DEBUG,
		     "IFDHGetCapabilities with tag TAG_IFD_ATR called");
		
		const char* atr = "3BE800008131FE00506572736F53696D";
		//TODO check if lenght fits

		*Length = HexString2CharArray(atr, Value);
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
	//TODO
	Log2(PCSC_LOG_DEBUG, "IFDHPowerICC (Lun %d)", Lun);
	switch (Action) {
	case IFD_POWER_DOWN:
		//TODO send PowerDown to Socket
		//TODO unset cached atr
	case IFD_POWER_UP:
		//TODO send PowerUp to Socket
		//TODO set atr according to response
	case IFD_RESET:
		//TODO send Reset to Socket
		//TODO update atr according to response
	default:
		Log3(PCSC_LOG_ERROR, "IFDHPowerICC (Lun %d) - unsupported Action 0x%X", Lun, Action);
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
	int cmdApduSize = TxLength * 2 + 2; // 2 chars per byte plus \n\0
	char cmdApdu[cmdApduSize];
	HexByteArray2String(TxBuffer, TxLength, cmdApdu);
	Log2(PCSC_LOG_DEBUG, "APDU %s", cmdApdu);

	//prepare buffer for response APDU
	int respApduSize = *RxLength * 2 + 2; // 2 chars per byte plus \n\0
	char respApdu[respApduSize];
	
	//perform the exchange
	exchangeApdu(cmdApdu, respApdu, &respApduSize);
	

	//copy response to RxBuffer
	*RxLength = HexString2CharArray(respApdu, RxBuffer);

	return IFD_SUCCESS; 
}

RESPONSECODE IFDHICCPresence(DWORD Lun)
{
	//TODO
	//Log2(PCSC_LOG_DEBUG, "IFDHICCPresence (Lun %d)", Lun); 
}

/*
 * The allocated buffers cmdApdu and respApdu need to be large enough to append at least \n
 */
void exchangeApdu(char* cmdApdu, char* respApdu, int* respApduSize)
{
	Log2(PCSC_LOG_DEBUG, "exchangeApdu command APDU\n %s", cmdApdu);

	//append \n to cmdApdu
	int len = strlen(cmdApdu);
	cmdApdu[len++] = '\n';
	cmdApdu[len] = '\0';
	

	//TODO	exchange APDU
	
	// transmit an APDU
	char* msg = "0084000008\n";
//	send(mysocket, msg, strlen(msg), 0); 
//	send(simSocket, msg, strlen(msg), 0); 

	// transmit an APDU
	len = send(simSocket, cmdApdu, len, 0); 

	// receive response
	len = recv(simSocket, respApdu, *respApduSize, 0);

	// remove trailing \n and terminate string with \0
	len--;
	respApdu[len] = '\0';

//	respApdu[0] = '1';
//	respApdu[1] = '2';
//	respApdu[2] = '9';
//	respApdu[3] = '0';
//	respApdu[4] = '0';
//	respApdu[5] = '0';
//	respApdu[6] = '\n';
//	respApdu[7] = '\0';

	//remove trailing newline 
//	len = strlen(respApdu);
//	cmdApdu[len-1] = '\0';
	
	Log2(PCSC_LOG_DEBUG, "exchangeApdu response APDU\n %s", respApdu);
}
