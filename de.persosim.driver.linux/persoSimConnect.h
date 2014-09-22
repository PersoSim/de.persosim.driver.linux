#ifndef PERSOSIMCONNECT_H
#define PERSOSIMCONNECT_H

#include <wintypes.h>
#include <ifdhandler.h>

//nr of supported readers (simultaneously connected connectors)
#define PSIM_MAX_READERS 5

struct psim_connection {
	int clientSocket;
};

//return values
#define PSIM_SUCCESS				0
#define PSIM_NO_CONNECTION			1
#define PSIM_COMMUNICATION_ERROR	2

//frequently used parts of PSIM connector messages
#define PSIM_MSG_DIVIDER "|"
#define PSIM_MSG_EMPTY_PARAMS ""

//PCSC function identifiers used in connector messages

#define PSIM_MSG_HANDSHAKE_ICC_HELLO "01"
#define PSIM_MSG_HANDSHAKE_ICC_STOP  "02"
#define PSIM_MSG_HANDSHAKE_ICC_ERROR "03"
#define PSIM_MSG_HANDSHAKE_ICC_DONE  "04"
#define PSIM_MSG_HANDSHAKE_IFD_HELLO "05"
#define PSIM_MSG_HANDSHAKE_IFD_ERROR "06"


#define PSIM_MSG_FUNCTION_DEVICE_CONTROL          "10"
#define PSIM_MSG_FUNCTION_DEVICE_LIST_DEVICES     "11"
#define PSIM_MSG_FUNCTION_GET_CAPABILITIES        "12"
#define PSIM_MSG_FUNCTION_SET_CAPABILITIES        "13"
#define PSIM_MSG_FUNCTION_POWER_ICC               "14"
#define PSIM_MSG_FUNCTION_TRANSMIT_TO_ICC         "15"
#define PSIM_MSG_FUNCTION_IS_ICC_PRESENT          "16"
#define PSIM_MSG_FUNCTION_IS_ICC_ABSENT           "17"
#define PSIM_MSG_FUNCTION_SWALLOW_ICC             "18"
#define PSIM_MSG_FUNCTION_SET_PROTOCOL_PARAMETERS "19"
#define PSIM_MSG_FUNCTION_LIST_INTERFACES         "1A"
#define PSIM_MSG_FUNCTION_LIST_CONTEXTS           "1B"
#define PSIM_MSG_FUNCTION_IS_CONTEXT_SUPPORTED    "1C"
#define PSIM_MSG_FUNCTION_GET_IFDSP               "1D"
#define PSIM_MSG_FUNCTION_EJECT_ICC               "1E"


/**
 * Start and fork of the handshake server, listening on the provided port number.
 *
 * Returns either PSIM_SUCCESS or PSIM_COMMUNICATION_ERROR
 */
int PSIMStartHandshakeServer(int);

/**
 * Check wheter a reader is connected on the given Lun.
 *
 * Returns either PSIM_SUCCESS or PSIM_NO_CONNECTION
 */
int PSIMIsReaderAvailable(int);

/**
 * Transmit a PCSC function call as properly encoded message to the PersoSim driver connector.
 *
 * @param[in] function is one of PCSIM_MSG_FUNCTION
 * @param[in] lun is encoded within the message and also also used to identify the correct socket/client to transmit the message to
 * @param[in] params HexString encoding of the defined parameters to the specific PCSC function as defined in the interface description.
 * 	 Multiple parameters should be separated by PSIM_MSG_DIVIDER. The calling code needs to ensure the correct structure of this parameter (including \0 termination)
 * @param[out] response allocated buffer the response should be written into.
 *   After the call finishes successfully this buffer will hold the received response according to the driver connector interface description.
 * @param[in] respLength size of the allocated response buffer.
 *   Its the responsibility of the calling code to ensure that the response buffer is large enough to hold
 * @param[in,out]
 *
 * Returns PSIM_SUCCESS if the messages were exchanged successfully or PSIM_COMMUNICATION_ERROR in any other case.
 */
int exchangePcscFunction(const char*, DWORD, const char*, char *, int);

//TODO document this method
RESPONSECODE extractPcscResponseCode(const char*);

#endif
