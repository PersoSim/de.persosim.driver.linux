#include <string.h>
#include <sys/socket.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>

#define BUFFERSIZE 	500
#define PORT 		9876

int main ()
{
	char buffer[500 + 1]; //PP BUFFER
	char* msg = "0084000008";
	char* msgPowerOn = "FF010000";

	// create the socket
	int mysocket = socket(AF_INET, SOCK_STREAM, 0);

	// define the remote address
	struct sockaddr_in dest;
	memset(&dest, 0, sizeof(dest));
	dest.sin_family = AF_INET;
	dest.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	dest.sin_port = htons(9876); // PP PORT

	// connect
	connect(mysocket, (struct sockadr *)&dest, sizeof(struct sockaddr));

	// transmit an APDU
	int len = send(mysocket, msgPowerOn, strlen(msgPowerOn), 0); 
	len = send(mysocket, "\n", 1, 0); 
	printf("Transmitted APDU: %s\n", msgPowerOn);

	// receive response
	len = 0;
	do {
		len += recv(mysocket, buffer + len, BUFFERSIZE - len, 0); // PP BUFFERSIZE
	} while (len < BUFFERSIZE && buffer[len-1] != '\n');
	buffer[len-1] = '\0';
	printf("Received response: %s\n", buffer);
	
	// transmit another APDU
	len = send(mysocket, msg, strlen(msg), 0); 
	len = send(mysocket, "\n", 1, 0); 
	printf("Transmitted APDU: %s\n", msg);

	// receive response
	len = 0;
	do {
		len += recv(mysocket, buffer + len, BUFFERSIZE - len, 0); // PP BUFFERSIZE
	} while (len < BUFFERSIZE && buffer[len-1] != '\n');
	buffer[len-1] = '\0';
	printf("Received response: %s\n", buffer);
	
	// transmit another APDU
	len = send(mysocket, msg, strlen(msg), 0); 
	len = send(mysocket, "\n", 1, 0); 
	printf("Transmitted APDU: %s\n", msg);

	// receive response
	len = 0;
	do {
		len += recv(mysocket, buffer + len, BUFFERSIZE - len, 0); // PP BUFFERSIZE
	} while (len < BUFFERSIZE && buffer[len-1] != '\n');
	buffer[len-1] = '\0';
	printf("Received response: %s\n", buffer);

	// close connection
	close(mysocket);

	return 0;
}


