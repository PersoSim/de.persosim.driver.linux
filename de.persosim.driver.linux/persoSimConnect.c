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

// socket fd for the connection to the client
int clientSocket = -1;

int PSIMStartHandshakeServer(int port)
{
	//allocate handshake socket
	int handshakeSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (handshakeSocket < 0)
	{
		Log2(PCSC_LOG_ERROR, "Unable to allocate server socket for handshake (port %d)", port);
		return PSIM_COMMUNICATION_ERROR;
	}

	struct sockaddr_in serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(port);

	// bind handshake socket
	if (bind(handshakeSocket, (struct sockaddr *) &serverAddr,
			sizeof(serverAddr)) < 0)
	{
		Log2(PCSC_LOG_ERROR,
				"Unable to bind server socket for handshake to port %d",
				port);
		return PSIM_COMMUNICATION_ERROR;
	}

	if (listen(handshakeSocket, 5) < 0)
	{
		if (errno == EADDRINUSE) {
			Log1(PCSC_LOG_ERROR, "Handshake socket already in use");
		} else {
			Log1(PCSC_LOG_ERROR, "Unable to listen on handshake socket");
		}
		return PSIM_COMMUNICATION_ERROR;
	}

	//accept client connection
	clientSocket = accept(handshakeSocket, NULL, NULL);
	if (clientSocket < 0) {
		Log1(PCSC_LOG_ERROR, "Handshake socket failed to accept client connection");
		return PSIM_COMMUNICATION_ERROR;
	}


	//hardcoded handshake
	int msgBufferSize = 256;
	char msgBuffer[msgBufferSize];
	int len = 0;
	do {
		len += recv(clientSocket, msgBuffer + len, msgBufferSize - len, 0);
		// if len == 0 after the first loop the connection was closed
	} while (len > 0 && len < msgBufferSize && msgBuffer[len-1] != '\n');

	if (len > 0) {
		msgBuffer[len-1] = '\0';
	} else {
		Log1(PCSC_LOG_ERROR, "Client did not initiate a valid handshake");
		return PSIM_COMMUNICATION_ERROR;
	}
	Log2(PCSC_LOG_DEBUG, "Client initiated handshake: %s\n", msgBuffer);

	if (send(clientSocket, "HELLO_IFD|LUN:00\n",17, MSG_NOSIGNAL) < 0) {
		Log1(PCSC_LOG_ERROR, "Could not send response to handshake initialization");
		return PSIM_COMMUNICATION_ERROR;
	}

	len = 0;
	do {
		len += recv(clientSocket, msgBuffer + len, msgBufferSize - len, 0);
		// if len == 0 after the first loop the connection was closed
	} while (len > 0 && len < msgBufferSize && msgBuffer[len-1] != '\n');

	if (len > 0) {
		msgBuffer[len-1] = '\0';
	} else {
		Log1(PCSC_LOG_ERROR, "Client did not correctly finish handshake");
		return PSIM_COMMUNICATION_ERROR;
	}
	Log2(PCSC_LOG_DEBUG, "Client finished handshake: %s\n", msgBuffer);

	if (send(clientSocket, "DONE_IFD\n",9, MSG_NOSIGNAL) < 0) {
		Log1(PCSC_LOG_ERROR, "Could not send finish handshake response");
		return PSIM_COMMUNICATION_ERROR;
	}

	return PSIM_SUCCESS;
}

//TODO remove old code below
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
		if ((clientSocket = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			// socket could not be allocated, try next one
			continue;
		}

		// connect to server
		if (connect(clientSocket, p->ai_addr, p->ai_addrlen) == -1) {
			// connection failed, close socket and try next one
			close(clientSocket);
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

	Log3(PCSC_LOG_DEBUG, "Socket connected to %s:%s", hostname, port);
	return PSIM_SUCCESS;
}

int PSIMCloseConnection()
{
	//close socket connection
	if (clientSocket >= 0)
	{
		if (close(clientSocket) != 0)
		{
			Log2(PCSC_LOG_ERROR, "Closing socket failed with errno %d", errno);
			return PSIM_COMMUNICATION_ERROR;
		}
	}

	//store connection state
	clientSocket = -1;
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

	// transmit cmdApdu (ignoring errors, missing response will be handled later on)
	send(clientSocket, cmdApdu, strlen(cmdApdu), 0); 
	send(clientSocket, "\n", 1, 0); 

	// receive response
	int len = 0;
	do {
		len += recv(clientSocket, respApdu + len, respApduSize - len, 0);
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
	return clientSocket >= 0;
}
