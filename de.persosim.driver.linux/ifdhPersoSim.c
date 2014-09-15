#include <stdlib.h>
#include <string.h>

#include <debuglog.h>
#include <ifdhandler.h>
#include <pcsclite.h>

#include "hexString.h"
#include "persoSimConnect.h"


//BUFFERSIZE is maximum of extended length APDU plus 2 bytes for status word
#define BUFFERSIZE	0x10001
char intBuffer[BUFFERSIZE];

#define DEVICENAMESIZE 200
char Hostname[DEVICENAMESIZE];
char Port[DEVICENAMESIZE];

int CachedAtrLength = 0;
char CachedAtr[MAX_ATR_SIZE];


RESPONSECODE
IFDHCreateChannelByName(DWORD Lun, LPSTR DeviceName)
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
		strcpy(Port, "5678");
		Log3(PCSC_LOG_INFO, "DEVICENAME malformed, using default %s:%s instead", Hostname, Port);
	} 
	
	return IFDHCreateChannel(Lun, 0x00);
}

RESPONSECODE
IFDHControl(DWORD Lun, DWORD dwControlCode, PUCHAR
	    TxBuffer, DWORD TxLength, PUCHAR RxBuffer, DWORD RxLength,
	    LPDWORD pdwBytesReturned)
{
	//XXX forward this call to driver connector
	// not yet supported, will be needed to implement standard reader functionality
	// Log2(PCSC_LOG_DEBUG, "IFDHControl (Lun %d)", Lun);
	return IFD_NOT_SUPPORTED;
}

RESPONSECODE
IFDHCreateChannel(DWORD Lun, DWORD Channel)
{
	Log3(PCSC_LOG_DEBUG, "IFDHCreateChannel (Lun %d, Channel 0x%x)", Lun,
	     Channel);

	// start the handshake server
	int portNo = (int) strtol(Port, (char **)NULL, 10);
	int rv = PSIMStartHandshakeServer(portNo);

	return (rv == PSIM_SUCCESS) ? IFD_SUCCESS : IFD_COMMUNICATION_ERROR;
}


RESPONSECODE
IFDHCloseChannel(DWORD Lun)
{
	Log2(PCSC_LOG_DEBUG, "IFDHCloseChannel (Lun %d)", Lun);
	
	// powerOff the simulator (also closes connection if needed)
	DWORD AtrLength = MAX_ATR_SIZE;
	IFDHPowerICC(Lun, IFD_POWER_DOWN, intBuffer, &AtrLength);

	//TODO kill handshake server after all channels are closed?

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

	return IFD_SUCCESS;
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

RESPONSECODE
IFDHPowerICC(DWORD Lun, DWORD Action, PUCHAR Atr, PDWORD AtrLength)
{
	Log2(PCSC_LOG_DEBUG, "IFDHPowerICC (Lun %d)", Lun);
	//prepare params and response
	char params[14];
	HexInt2String(Action,params);
	strcat(params, PSIM_MSG_DIVIDER);
	HexInt2String(*AtrLength, &params[9]);
	char respBufferSize = 74;
	char response[respBufferSize];

	//forward pcsc function to client
	int rv = exchangePcscFunction(PSIM_MSG_FUNCTION_POWER_ICC, Lun, params, response, respBufferSize);
	if (rv != PSIM_SUCCESS) {
		return IFD_COMMUNICATION_ERROR;
	}

	//extract ATR
	if (strlen(response) > 9) {
		*AtrLength = HexString2CharArray(&response[9], Atr);
	} else {
		*AtrLength = 0;
	}

	//return the response code
	return extractPcscResponseCode(response);
}

RESPONSECODE
IFDHTransmitToICC(DWORD Lun, SCARD_IO_HEADER SendPci,
		  PUCHAR TxBuffer, DWORD TxLength, PUCHAR RxBuffer, PDWORD
		  RxLength, PSCARD_IO_HEADER RecvPci)
{
	Log2(PCSC_LOG_DEBUG, "IFDHTransmitToICC (Lun %d)", Lun);
	
	//prepare params buffer
	char params[TxLength * 2 + 10];
	HexByteArray2String(TxBuffer, TxLength, params);
	strcat(params, PSIM_MSG_DIVIDER);
	HexInt2String(*RxLength, &params[TxLength * 2 + 1]);

	//prepare response buffer
	long respBufferSize = *RxLength * 2 + 10;
	char response[respBufferSize];

	//forward pcsc function to client
	int rv = exchangePcscFunction(PSIM_MSG_FUNCTION_TRANSMIT_TO_ICC, Lun, params, response, respBufferSize);
	if (rv != PSIM_SUCCESS) {
		return IFD_COMMUNICATION_ERROR;
	}

	//extract response
	if (strlen(response) > 9) {
		*RxLength = HexString2CharArray(&response[9], RxBuffer);
	} else {
		*RxLength = 0;
	}

	//return the response code
	return extractPcscResponseCode(response);
}

RESPONSECODE
IFDHICCPresence(DWORD Lun)
{
	//Log2(PCSC_LOG_DEBUG, "IFDHICCPresence (Lun %d)", Lun);
	
	//prepare buffer for response
	char respBufferSize = 9;
	char response[respBufferSize];

	//forward pcsc function to client
	int rv = exchangePcscFunction(PSIM_MSG_FUNCTION_IS_ICC_PRESENT, Lun, PSIM_MSG_EMPTY_PARAMS, response, respBufferSize);
	if (rv != PSIM_SUCCESS) {
		return IFD_ICC_NOT_PRESENT;
	}

	//return the response code
	return extractPcscResponseCode(response);
}

