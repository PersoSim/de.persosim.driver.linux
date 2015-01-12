#include "persoSimConnect.h"
#include "hexString.h"

#include <debuglog.h>
#include <ifdhandler.h>
#include <pcsclite.h>

#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

#include <limits.h>

// socket file descriptor for the connection to the client
struct psim_connection connectors[PSIM_MAX_READERS];

struct psim_connection * getReaderConnection(DWORD lun) {
	return &connectors[lun >> 16];
}

int closeReaderConnection(DWORD lun) {
	struct psim_connection * curReader = getReaderConnection(lun);

	int sockfd = curReader->clientSocket;
	curReader->clientSocket = 0;
	if (sockfd > 0) {
		close(sockfd);
	}

	return PSIM_SUCCESS;
}

int PSIMIsReaderAvailable(int lun) {
	int readerNum = lun >> 16;
	if (readerNum >= PSIM_MAX_READERS) return PSIM_NO_CONNECTION;
	return (connectors[readerNum].clientSocket > 0) ? PSIM_SUCCESS : PSIM_NO_CONNECTION;
}

// internal non-exported helper functions
int transmit (int sockfd, const char* msgBuffer) {
	//if no client connection is available return immediately
	if (sockfd < 0) {
		return PSIM_NO_CONNECTION;
	}

	int bufferLength = strlen(msgBuffer) +1 ;
	char transmitBuffer[bufferLength];
	strcpy(transmitBuffer, msgBuffer);
	strcat(transmitBuffer, "\n");
	Log2(PCSC_LOG_DEBUG, "Sending message to client: %s", transmitBuffer);

	if (send(sockfd, transmitBuffer, bufferLength, MSG_NOSIGNAL) < 0) {
			Log1(PCSC_LOG_ERROR, "Failure during transmit to client");
			return PSIM_COMMUNICATION_ERROR;
	}
	return PSIM_SUCCESS;
}

int receive(int sockfd, char* response, int respLength) {
	//if no client connection is available return immediately
	if (sockfd < 0) {
		return PSIM_NO_CONNECTION;
	}

	int offset = 0;
	do {
		offset += recv(sockfd, response + offset, respLength - offset, 0);
		// if len == 0 after the first loop the connection was closed
	} while (offset > 0 && offset < respLength && response[offset-1] != '\n');

	//FIXME handle Windows line endings
	if (offset > 0) {
		response[offset-1] = '\0';
	} else {
		Log1(PCSC_LOG_ERROR, "Failure during receive from client (no response received)");
		return PSIM_COMMUNICATION_ERROR;
	}
	Log2(PCSC_LOG_DEBUG, "Received from client: %s\n", response);

	return PSIM_SUCCESS;
}

int exchangePcscFunction(const char* function, DWORD lun, const char* params, char * response, int respLength) {
	// check if a reader is connected at given lun
//	if (PSIMIsReaderAvailable(lun) != PSIM_SUCCESS) {
//		return PSIM_NO_CONNECTION;
//	}
	struct psim_connection * curReader = getReaderConnection(lun);

	// build message buffer
	const int minLength = 12; // 2*function + divider + 8*lun + \0
	const int paramLength = strlen(params);
	const int bufferLength = (paramLength) ?  minLength + 1 + paramLength : minLength;
	char msgBuffer[bufferLength];
	strcpy(msgBuffer, function);
	strcat(msgBuffer, PSIM_MSG_DIVIDER);
	HexInt2String(lun, &msgBuffer[3]);
	msgBuffer[bufferLength-1] = '\0';
	if (bufferLength > minLength) {
		strcat(msgBuffer, PSIM_MSG_DIVIDER);
		strcat(msgBuffer, params);
	}

//	Log2(PCSC_LOG_DEBUG, "PSIM transmit PCSC function: %s", msgBuffer);


	int rv = transmit(curReader->clientSocket, msgBuffer);
	if (rv != PSIM_SUCCESS) {
		Log1(PCSC_LOG_ERROR, "Could not transmit PCSC function to PersoSim Connector");
		closeReaderConnection(lun);
		return rv;
	}

	rv = receive(curReader->clientSocket, response, respLength);
	if (rv != PSIM_SUCCESS) {
		Log1(PCSC_LOG_ERROR, "Could not receive PCSC response from PersoSim Connector");
		closeReaderConnection(lun);
		return rv;
	}

	return PSIM_SUCCESS;

}

RESPONSECODE extractPcscResponseCode(const char* response) {
	char responseCodeString[9];
	strncpy(responseCodeString, response, 8);
	responseCodeString[8] = '\0';

	return HexString2Int(responseCodeString);
}

pthread_mutex_t handshakeServerMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_t handshakeThread;
int handshakeSocket = 0;
int acceptHandshakeConnections = 1;

void * handleHandshakeConnections(void * param) {
	while(acceptHandshakeConnections) {

		Log1(PCSC_LOG_DEBUG, "Handshake server accepting connections");

		//accept new client connection
		int clientSocket = accept(handshakeSocket, NULL, NULL);
		if (clientSocket < 0) {
			Log1(PCSC_LOG_ERROR, "Handshake socket failed to accept client connection, terminating handshake server");
			break;
		}


		//hardcoded handshake
		int msgBufferSize = 256;
		char msgBuffer[msgBufferSize];
		int rv = receive(clientSocket, msgBuffer, msgBufferSize);
		if (rv != PSIM_SUCCESS) {
			Log1(PCSC_LOG_ERROR, "Client did not initiate a valid handshake");
			close(clientSocket);
			continue;
		}
		Log2(PCSC_LOG_DEBUG, "Client initiated handshake: %s\n", msgBuffer);

		//TODO handle different handshake types, e.g. connection close by client

		//TODO select lun
		int lun = 0x010000;
		lun = 0;
		struct psim_connection * curReader = getReaderConnection(lun);

		strcpy(msgBuffer, PSIM_MSG_HANDSHAKE_IFD_HELLO);
		strcat(msgBuffer, PSIM_MSG_DIVIDER);
		HexInt2String(lun, &msgBuffer[3]);
		strcat(msgBuffer, "\n");
		rv = transmit(clientSocket, msgBuffer);
		if (rv != PSIM_SUCCESS) {
			Log1(PCSC_LOG_ERROR, "Could not send response to handshake initialization");
			close(clientSocket);
			continue;
		}

		rv = receive(clientSocket, msgBuffer, msgBufferSize);
		if (rv != PSIM_SUCCESS) {
			Log1(PCSC_LOG_ERROR, "Client did not correctly finish handshake");
			close(clientSocket);
			continue;
		}
		Log2(PCSC_LOG_DEBUG, "Client finished handshake: %s\n", msgBuffer);

		// enable connector as new reader
		curReader->clientSocket = clientSocket;

		//let pcsc rescan readers
		raise(SIGUSR1);


		//XXX implement closing and disposal of client sockets (through handshake as well as on errors)
	}

	//must return something
	return NULL;
}


int PSIMStartHandshakeServer(int port)
{
	pthread_mutex_lock(&handshakeServerMutex);

	if (handshakeSocket > 0) {
		//handshakeServer already running
		pthread_mutex_unlock(&handshakeServerMutex);
		return PSIM_SUCCESS;
	}

	//allocate handshake socket
	handshakeSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (handshakeSocket < 0)
	{
		Log2(PCSC_LOG_ERROR, "Unable to allocate server socket for handshake (port %d)", port);
		pthread_mutex_unlock(&handshakeServerMutex);
		return PSIM_COMMUNICATION_ERROR;
	}

	//allow SO_REUSEADDR to allow faster binding of the socket after a crash
	int reuse = 1;
	setsockopt(handshakeSocket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof reuse);

	struct sockaddr_in serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(port);

	// bind handshake socket
	if (bind(handshakeSocket, (struct sockaddr *) &serverAddr,
			sizeof(serverAddr)) < 0)
	{
		Log3(PCSC_LOG_ERROR,
				"Unable to bind server socket for handshake to port %d (errno=%d)",
				port, errno);
		pthread_mutex_unlock(&handshakeServerMutex);
		return PSIM_COMMUNICATION_ERROR;
	}

	if (listen(handshakeSocket, 5) < 0)
	{
		if (errno == EADDRINUSE) {
			Log1(PCSC_LOG_ERROR, "Handshake socket already in use");
		} else {
			Log1(PCSC_LOG_ERROR, "Unable to listen on handshake socket");
		}
		pthread_mutex_unlock(&handshakeServerMutex);
		return PSIM_COMMUNICATION_ERROR;
	}

	int rv;
	if( (rv=pthread_create( &handshakeThread, NULL, &handleHandshakeConnections, NULL)) ){
		Log2(PCSC_LOG_ERROR, "Unable to execute handshake handler thread (error=%d)", rv);

		pthread_mutex_unlock(&handshakeServerMutex);
		return PSIM_COMMUNICATION_ERROR;
	}

	pthread_mutex_unlock(&handshakeServerMutex);

	return PSIM_SUCCESS;
}
