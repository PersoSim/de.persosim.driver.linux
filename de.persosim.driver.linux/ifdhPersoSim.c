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


RESPONSECODE
IFDHCreateChannelByName(DWORD Lun, LPSTR DeviceName)
{
	Log3(PCSC_LOG_DEBUG, "IFDHCreateChannelByName (Lun 0x%08lX, DeviceName %s)",
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
	Log3(PCSC_LOG_DEBUG, "IFDHControl (Lun 0x%08lX, controlCode 0x%lX)", Lun, dwControlCode);

	//prepare params buffer
	int pLength = 19 + TxLength * 2; // 8(controlCode) + divider + 2*TxLenght(TxBuffer) + divider + 8(RxLength) + '\0'
	char params[pLength];
	HexInt2String(dwControlCode,params);
	strcat(params, PSIM_MSG_DIVIDER);
	HexByteArray2String(TxBuffer, TxLength, &params[9]);
	strcat(params, PSIM_MSG_DIVIDER);
	HexInt2String(RxLength, &params[pLength-9]);

	//prepare response buffer
	long respBufferSize = RxLength * 2 + 10;
	char response[respBufferSize];

	//forward pcsc function to client
	int rv = exchangePcscFunction(PSIM_MSG_FUNCTION_DEVICE_CONTROL, Lun, params, response, respBufferSize);
	if (rv != PSIM_SUCCESS) {
		return IFD_COMMUNICATION_ERROR;
	}

	//extract response
	if (strlen(response) > 9) {
		*pdwBytesReturned = HexString2CharArray(&response[9], RxBuffer);
	} else {
		*pdwBytesReturned = 0;
	}

	//return the response code
	return extractPcscResponseCode(response);
}

RESPONSECODE
IFDHCreateChannel(DWORD Lun, DWORD Channel)
{
	Log3(PCSC_LOG_DEBUG, "IFDHCreateChannel (Lun 0x%08lX, Channel 0x%lX)", Lun,
	     Channel);

	if (Lun == 0) {
		// start the handshake server
		int portNo = (int) strtol(Port, (char **)NULL, 10);
		int rv = PSIMStartHandshakeServer(portNo);

		return (rv == PSIM_SUCCESS) ? IFD_SUCCESS : IFD_COMMUNICATION_ERROR;
	} else {
		return (PSIMIsReaderAvailable(Lun) == PSIM_SUCCESS) ? IFD_SUCCESS : IFD_NO_SUCH_DEVICE;
	}

}


RESPONSECODE
IFDHCloseChannel(DWORD Lun)
{
	Log2(PCSC_LOG_DEBUG, "IFDHCloseChannel (Lun 0x%08lX)", Lun);
	
	// powerOff the simulator (also closes connection if needed)
	DWORD AtrLength = MAX_ATR_SIZE;
	IFDHPowerICC(Lun, IFD_POWER_DOWN, intBuffer, &AtrLength);

	return IFD_SUCCESS;
}

RESPONSECODE
IFDHGetCapabilities(DWORD Lun, DWORD Tag, PDWORD Length, PUCHAR Value)
{
	Log3(PCSC_LOG_DEBUG, "IFDHGetCapabilities (Lun 0x%08lX, Tag 0x%lX)", Lun, Tag);
	//Check if Tag should be answered by the driver instead of the connector
	switch (Tag) {
		case TAG_IFD_SIMULTANEOUS_ACCESS:
			*Value = PSIM_MAX_READERS;
			*Length = 1;
			return IFD_SUCCESS;
		case TAG_IFD_SLOTS_NUMBER:
			if (Lun == 0) {
				*Value = 1;//TODO only one slot for the moment
				*Length = 1;
				return IFD_SUCCESS;
			} else {
				break;
			}
		case TAG_IFD_SLOT_THREAD_SAFE:
			*Value = 0; //TODO check whether this can be set to true
			*Length = 1;
			return IFD_SUCCESS;
		}

	// if reader not connected return IFD_COMMUNICATION_ERROR
	if (PSIMIsReaderAvailable(Lun) != PSIM_SUCCESS) return IFD_COMMUNICATION_ERROR;

	//forward other requests to the connector

	//prepare params buffer
	char params[18]; // 8(Tag) + divider + 8(Length) + '\0'
	HexInt2String(Tag,params);
	strcat(params, PSIM_MSG_DIVIDER);
	HexInt2String(*Length, &params[9]);

	//prepare response buffer
	long respBufferSize = *Length * 2 + 10;
	char response[respBufferSize];

	//forward pcsc function to client
	int rv = exchangePcscFunction(PSIM_MSG_FUNCTION_GET_CAPABILITIES, Lun, params, response, respBufferSize);
	if (rv != PSIM_SUCCESS) {
		return IFD_COMMUNICATION_ERROR;
	}

	//extract returned value
	if (strlen(response) > 9) {
		*Length = HexString2CharArray(&response[9], Value);
	} else {
		*Length = 0;
	}

	//return the response code
	return extractPcscResponseCode(response);
}

RESPONSECODE
IFDHSetCapabilities(DWORD Lun, DWORD Tag, DWORD Length, PUCHAR Value)
{
	Log3(PCSC_LOG_DEBUG, "IFDHSetCapabilities (Lun 0x%08lX, Tag 0x%lX)", Lun, Tag);

	// if reader not connected return IFD_COMMUNICATION_ERROR
	if (PSIMIsReaderAvailable(Lun) != PSIM_SUCCESS) return IFD_COMMUNICATION_ERROR;

	//prepare params buffer
	int pLenght = 10 + 2 * Length; // 8(Tag) + divider + 2*Length(Value) + '\0'
	char params[pLenght];
	HexInt2String(Tag,params);
	strcat(params, PSIM_MSG_DIVIDER);
	HexByteArray2String(Value, Length, &params[9]);

	//prepare response buffer
	char respBufferSize = 9;
	char response[respBufferSize];

	//forward pcsc function to client
	int rv = exchangePcscFunction(PSIM_MSG_FUNCTION_SET_CAPABILITIES, Lun, params, response, respBufferSize);
	if (rv != PSIM_SUCCESS) {
		return IFD_COMMUNICATION_ERROR;
	}

	//return the response code
	return extractPcscResponseCode(response);
}

RESPONSECODE
IFDHSetProtocolParameters(DWORD Lun, DWORD Protocol, UCHAR Flags,
			  UCHAR PTS1, UCHAR PTS2, UCHAR PTS3)
{
	Log3(PCSC_LOG_DEBUG, "IFDHSetProtocolParameters (Lun 0x%08lX, Protocol %ld)", Lun, Protocol);
	Log5(PCSC_LOG_DEBUG, "using Flags 0x%X, PTS1 0x%X, PTS2 0x%X, PTS3 0x%X)", Flags, PTS1, PTS2, PTS3);

	// if reader not connected return IFD_COMMUNICATION_ERROR
	if (PSIMIsReaderAvailable(Lun) != PSIM_SUCCESS) return IFD_COMMUNICATION_ERROR;

	//prepare params buffer
	char params[21]; // 8(Protocol) + 4 * ( divider + flag/ptsx) + '\0'
	HexInt2String(Protocol,params);
	strcat(params, PSIM_MSG_DIVIDER);
	HexByte2String(Flags, &params[9]);
	strcat(params, PSIM_MSG_DIVIDER);
	HexByte2String(PTS1, &params[12]);
	strcat(params, PSIM_MSG_DIVIDER);
	HexByte2String(PTS2, &params[15]);
	strcat(params, PSIM_MSG_DIVIDER);
	HexByte2String(PTS3, &params[18]);

	//prepare response buffer
	char respBufferSize = 9;
	char response[respBufferSize];

	//forward pcsc function to client
	int rv = exchangePcscFunction(PSIM_MSG_FUNCTION_SET_PROTOCOL_PARAMETERS, Lun, params, response, respBufferSize);
	if (rv != PSIM_SUCCESS) {
		return IFD_COMMUNICATION_ERROR;
	}

	//return the response code
	return extractPcscResponseCode(response);
}

RESPONSECODE
IFDHPowerICC(DWORD Lun, DWORD Action, PUCHAR Atr, PDWORD AtrLength)
{
	Log3(PCSC_LOG_DEBUG, "IFDHPowerICC (Lun 0x%08lX, Action %ld)", Lun, Action);

	// if reader not connected return IFD_COMMUNICATION_ERROR
	if (PSIMIsReaderAvailable(Lun) != PSIM_SUCCESS) return IFD_COMMUNICATION_ERROR;

	//prepare params buffer
	char params[18]; // 8(Action) + divider + 8(AtrLength) + '\0'
	HexInt2String(Action,params);
	strcat(params, PSIM_MSG_DIVIDER);
	HexInt2String(*AtrLength, &params[9]);

	//prepare response buffer
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
	Log2(PCSC_LOG_DEBUG, "IFDHTransmitToICC (Lun 0x%08lX)", Lun);

	// if reader not connected return IFD_COMMUNICATION_ERROR
	if (PSIMIsReaderAvailable(Lun) != PSIM_SUCCESS) return IFD_COMMUNICATION_ERROR;

	//prepare params buffer
	char params[10 + TxLength * 2];// 8(controlCode) + divider + 2*TxLenght(TxBuffer) + '\0'
	HexByteArray2String(TxBuffer, TxLength, params);
	strcat(params, PSIM_MSG_DIVIDER); //XXX why include the RESP length twice???
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
	//Log2(PCSC_LOG_DEBUG, "IFDHICCPresence (Lun 0x%08lX)", Lun);

	// if reader not connected return IFD_COMMUNICATION_ERROR
	if (PSIMIsReaderAvailable(Lun) != PSIM_SUCCESS) return IFD_ICC_NOT_PRESENT;
	
	//prepare response buffer
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

