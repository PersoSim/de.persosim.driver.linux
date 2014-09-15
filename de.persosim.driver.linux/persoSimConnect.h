#ifndef PERSOSIMCONNECT_H
#define PERSOSIMCONNECT_H

//return values
#define PSIM_SUCCESS				0
#define PSIM_NO_CONNECTION			1
#define PSIM_COMMUNICATION_ERROR	2

//frequently used parts of PSIM connector messages
#define PSIM_MSG_DIVIDER "|"
#define PSIM_MSG_EMPTY_PARAMS ""

//PCSC function identifiers used in connector messages
//#define PSIM_FUNCTION_DEVICE_CONTROL = 0;
//#define PSIM_FUNCTION_DEVICE_LIST_DEVICES = 1;
//#define PSIM_FUNCTION_GET_CAPABILITIES = 2;
//#define PSIM_FUNCTION_SET_CAPABILITIES = 3;
//#define PSIM_FUNCTION_POWER_ICC = 4;
//#define PSIM_FUNCTION_TRANSMIT_TO_ICC = 5;
#define PSIM_MSG_FUNCTION_IS_ICC_PRESENT "06"
//#define PSIM_FUNCTION_IS_ICC_ABSENT = 7;
//#define PSIM_FUNCTION_SWALLOW_ICC = 8;
//#define PSIM_FUNCTION_SET_PROTOCOL_PARAMETERS = 9;
//#define PSIM_FUNCTION_LIST_INTERFACES = 10;
//#define PSIM_FUNCTION_LIST_CONTEXTS = 11;
//#define PSIM_FUNCTION_IS_CONTEXT_SUPPORTED = 12;
//#define PSIM_FUNCTION_GET_IFDSP = 13;
//#define PSIM_FUNCTION_EJECT_ICC = 14;


/**
 * Start and fork of the handshake server, listening on the provided port number.
 *
 * Returns either PSIM_SUCCESS or PSIM_COMMUNICATION_ERROR
 */
int PSIMStartHandshakeServer(int port);

/**
 * Checks for presence of a simulated card.
 *
 * Returns either PSIM_SUCCESS or PSIM_NO_CARD_PRESENT. Any error case (e.g. disconnected sockets) is returned as PSIM_NO_CARD_PRESENT
 */
int PSIMIsIccPresent(int lun);

//TODO remove old definitions below
//Special command APDUs supported by PersoSim
#define PSIM_CMD_POWEROFF	"FF000000"
#define PSIM_CMD_POWERON	"FF010000"
#define PSIM_CMD_PING		"FF900000"
#define PSIM_CMD_SWITCHOFF	"FFEE0000"
#define PSIM_CMD_RESET		"FFFF0000"
#define PSIM_CMD_LENGTH		9

extern int PSIMCloseConnection();

extern int PSIMIsConnected();

extern void exchangeApdu(const char*, char*, int);

#endif
