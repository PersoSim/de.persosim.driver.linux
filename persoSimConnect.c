#include "persoSimConnect.h"

#include <string.h>
#include <debuglog.h>

#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>


// connection state and used socket
int simConnected = 0;
int simSocket = -1;

int PSIMOpenConnection(const char* hostname, const char* port)
{
	// find the remote address (candidates)
	struct addrinfo hints, *servinfo, *p;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if (getaddrinfo(hostname, port, &hints, &servinfo) != 0) {
		Log3(PCSC_LOG_ERROR, "Unable to resolve %s:%d", hostname, port);
		return PSIM_CAN_NOT_RESOLVE;
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
		Log3(PCSC_LOG_ERROR, "Unable to connect to %s:%s", hostname, port);
		return PSIM_COMMUNICATION_ERROR;
	}
	//free all found addresses
	freeaddrinfo(servinfo);

	//store connection state
	simConnected = 1;

	Log3(PCSC_LOG_DEBUG, "Socket connected to %s:%s", hostname, port);
	return PSIM_SUCCESS;
}

int PSIMCloseConnection()
{
	//close socket connection
	if (simSocket >= 0)
	{
		if (close(simSocket) != 0)
		{
			Log2(PCSC_LOG_ERROR, "Closing socket failed with errno %d", errno);
			return PSIM_COMMUNICATION_ERROR;
		}
	}

	//store connection state
	simConnected = 0;
	simSocket = -1;
	Log1(PCSC_LOG_DEBUG, "Socket disconnected");

	return PSIM_SUCCESS;
}

/*
 * Expects cmdApdu to contain a HexString and respApdu to be large enough to hold the complete response HexString both including \0 termination
 */
void exchangeApdu(const char* cmdApdu, char* respApdu, int respApduSize)
{
	//Log2(PCSC_LOG_DEBUG, "exchangeApdu command APDU\n%s\n", cmdApdu);

	//ignore SIGPIPE for this function
	struct sigaction ignAct, oldAct;
	memset(&ignAct, '\0', sizeof(ignAct));
	ignAct.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &ignAct, &oldAct);

	// transmit cmdApdu (ignoring errors, missing response wil be handeld later on)
	send(simSocket, cmdApdu, strlen(cmdApdu), 0); 
	send(simSocket, "\n", 1, 0); 

	// receive response
	int len = 0;
	do {
		len += recv(simSocket, respApdu + len, respApduSize - len, 0);
		// if len == 0 after the first loop the connection was closed
	} while (len > 0 && len < respApduSize && respApdu[len-1] != '\n');

	if (len > 4) {
		respApdu[len-1] = '\0';
	}
	else
	{
		// no valid response was received (did not even contain SW)
		// include fake SW
		strcpy(respApdu, "6FFF");
	}

	//Log2(PCSC_LOG_DEBUG, "exchangeApdu response APDU\n%s\n", respApdu);
	
	// reset handler for SIGPIPE
	sigaction(SIGPIPE, &oldAct, NULL);
}

int PSIMIsConnected()
{
	return simConnected;
}
