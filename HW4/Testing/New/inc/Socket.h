#ifndef	__SOCKET_H__
#define __SOCKET_H__

#define Port_Num		(uint32_t)8080

#define Default_IP		"192.168.50.104"
#define IP_Addr_Len		(uint8_t)20

#define Mode_Infinite		(uint8_t)1
#define Mode_Limited		(uint8_t)0
#define Max_Retries		(uint8_t)10
#define Socket_Retry_Mode	Mode_Limited

#define Socket_Timeout_sec	(uint8_t)2

#define Segment_Size		(uint32_t)1024
#define Frame_Socket_Max_Retries	(uint8_t)10

void *Socket_Func(void *para_t);

#endif
