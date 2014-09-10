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
		strcpy(Port, "9876");
		Log3(PCSC_LOG_INFO, "DEVICENAME malformed, using default %s:%s instead", Hostname, Port);
	} 
	
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
	char cmdApdu[PSIM_CMD_LENGTH];
	switch (Action) {
	case IFD_POWER_DOWN:
		if (PSIMIsConnected())
		{
			// send PowerOff to simulator
			strcpy(cmdApdu, PSIM_CMD_POWEROFF);
			exchangeApdu(cmdApdu, intBuffer, BUFFERSIZE);

			// close connection
			PSIMCloseConnection();
		}
		
		// unset cached atr
		CachedAtrLength = 0;
		memset(CachedAtr, 0, MAX_ATR_SIZE);
		
		break;
	case IFD_POWER_UP:
		if (!PSIMIsConnected())
		{
			// open connection
			PSIMOpenConnection(Hostname, Port);
		}
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

RESPONSECODE
IFDHICCPresence(DWORD Lun)
{
	//Log2(PCSC_LOG_DEBUG, "IFDHICCPresence (Lun %d)", Lun);
	
	// ensure that a connection exists
	int closeConnection = 0; //true if connection needs to be closed after test
	if (!PSIMIsConnected())
	{
		switch (PSIMOpenConnection(Hostname, Port)) {
		case PSIM_SUCCESS:
			closeConnection = 1; //make sure connection is closed after the ping
			break;
		default:
			return IFD_ICC_NOT_PRESENT;
		}
	}

	// send PresencePing to simulator
	char cmdApdu[PSIM_CMD_LENGTH];
	strcpy(cmdApdu, PSIM_CMD_PING);
	exchangeApdu(cmdApdu, intBuffer, BUFFERSIZE);

	// ensure that a connection established by this method is also closed here
	if (closeConnection)
	{
		PSIMCloseConnection();
	}

	if (strcmp(intBuffer, "9000") == 0)
	{
		return IFD_SUCCESS; 
	}
	else
	{
		return IFD_ICC_NOT_PRESENT;
	}
}

