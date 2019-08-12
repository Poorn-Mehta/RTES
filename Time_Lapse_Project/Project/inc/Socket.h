/*
*		File: Socket.h
*		Purpose: The header file containing useful defines and function prototypes for related source file
*		Owner: Poorn Mehta
*		Last Modified: 8/11/2019
*/

#ifndef	__SOCKET_H__
#define __SOCKET_H__

// Port number for socket
#define Port_Num		(uint32_t)8080

// The IP address of remote
#define Default_IP		"128.138.189.38"

// Length of IP address
#define IP_Addr_Len		(uint8_t)20

// Socket retries config
#define Mode_Infinite		(uint8_t)1
#define Mode_Limited		(uint8_t)0
#define Max_Retries		(uint8_t)10
#define Socket_Retry_Mode	Mode_Limited

// Socket timeout (currently not used)
#define Socket_Timeout_sec	(uint8_t)2

// Size of image segment that has to be sent in each transaction
#define Segment_Size		(uint32_t)1024

// Maximum number or retries to attempt resending the same frame
#define Frame_Socket_Max_Retries	(uint8_t)10

// Function prototype
void *Socket_Func(void *para_t);

#endif
