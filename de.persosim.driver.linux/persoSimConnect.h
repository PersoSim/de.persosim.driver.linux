#ifndef PERSOSIMCONNECT_H
#define PERSOSIMCONNECT_H

//Special command APDUs supported by PersoSim
#define PSIM_CMD_POWEROFF	"FF000000"
#define PSIM_CMD_POWERON	"FF010000"
#define PSIM_CMD_PING		"FF900000"
#define PSIM_CMD_SWITCHOFF	"FFEE0000"
#define PSIM_CMD_RESET		"FFFF0000"
#define PSIM_CMD_LENGTH		9

//return values
#define PSIM_SUCCESS			0
#define PSIM_COMMUNICATION_ERROR	1
#define PSIM_CAN_NOT_RESOLVE		2

extern int PSIMOpenConnection(const char*, const char*);

extern int PSIMCloseConnection();

extern int PSIMIsConnected();

extern void exchangeApdu(const char*, char*, int);

#endif
