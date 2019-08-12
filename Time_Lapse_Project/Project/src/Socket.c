/*
*		File: Socket.c
*		Purpose: The source file containing functions to send all images to remote computer using TCP socket
*		Owner: Poorn Mehta
*		Last Modified: 8/11/2019
*/

#include "main.h"
#include "Aux_Func.h"
#include "Cam_Func.h"
#include "Socket.h"

// Shared Variables
char target_ip[20];
strct_analyze Analysis;
uint32_t Deadline_ms;

// Local Variables
static store_struct file_in;
static uint8_t frame_data[Big_Buffer_Size];
static int new_socket, q_recv_resp, total_bytes, written_bytes, read_bytes;
static struct sockaddr_in client;
static uint32_t retry_count, frames, buf_index, frame_confirm, Frame_Attempt, sock_index;
//static struct timeval sock_timeout;
static mqd_t socket_queue;
static struct timespec curr_time;
static float Socket_Stamp_1, Socket_Stamp_2;
static float Socket_Start_Stamp, Socket_End_Stamp;

// Function to setup Socket Queue in Read Only mode
// Parameter1: void
// Return: uint8_t result - 0: success
static uint8_t Socket_Q_Setup(void)
{
	struct mq_attr socket_queue_attr;
	socket_queue_attr.mq_maxmsg = queue_size;
	socket_queue_attr.mq_msgsize = sizeof(store_struct);

	socket_queue = mq_open(socket_q_name, O_CREAT | O_RDONLY | O_CLOEXEC, 0666, &socket_queue_attr);
	if(socket_queue == -1)
	{
		syslog(LOG_ERR, "<%.6fms>Queue opening error for Storage", Time_Stamp(Mode_ms));
		return 1;
	}
	return 0;
}

// Function to setup connect to the socket
// Parameter1: void
// Return: uint8_t result - 0: success
static uint8_t Connect_Socket(void)
{
	while(retry_count < Max_Retries)
	{
		if(connect(new_socket, (struct sockaddr *)&client, sizeof(client)) == 0)
		{
			syslog(LOG_INFO, "<%.6fms>!!Socket!! Connection to the Remote Succeeded", Time_Stamp(Mode_ms));
			return 0;
		}
		
		else
		{
			syslog(LOG_ERR, "<%.6fms>!!Socket!! Connection Attempt to the Remote Failed", Time_Stamp(Mode_ms));
		}

		retry_count += 1;
	}
	return 1;
}

// Function to setup the socket connection
// Parameter1: void
// Return: uint8_t result - 0: success
static uint8_t Socket_Init(void)
{
	retry_count = 0;

	// socket init on client end
	new_socket = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if(new_socket < 0)
	{
		syslog(LOG_ERR, "<%.6fms>!!Socket!! Socket Creation Failed", Time_Stamp(Mode_ms));
		return 1;
	}

/*	sock_timeout.tv_sec = Socket_Timeout_sec;
	sock_timeout.tv_usec = 0;
	if(setsockopt(new_socket, SOL_SOCKET, SO_RCVTIMEO, &sock_timeout, sizeof(struct timeval)) != 0)
	{
		syslog(LOG_ERR, "<%.6fms>!!Socket!! *EVENT* Error in Setsockopt", Time_Stamp(Mode_ms));
		return 1;
	}*/

	client.sin_family = AF_INET;

	if(inet_pton(AF_INET, target_ip, &client.sin_addr) <= 0)
	{
		syslog(LOG_ERR, "<%.6fms>!!Socket!! Error in IP Address", Time_Stamp(Mode_ms));
		return 1;
	}

	client.sin_port = htons(Port_Num);

	return 0;

}

// Function that implements Socket Thread
void *Socket_Func(void *para_t)
{

	// Setup
	if(Socket_Init() != 0)
	{
		syslog(LOG_ERR, "<%.6fms>!!Socket!! Thread Setup Failed", Time_Stamp(Mode_ms));
		pthread_exit(0);
	}

	if(Socket_Q_Setup() != 0)
	{
		syslog(LOG_ERR, "<%.6fms>!!Socket!! Thread Setup Failed", Time_Stamp(Mode_ms));
		pthread_exit(0);
	}

	frames = 0;
	sock_index = 0;

	syslog(LOG_INFO, "<%.6fms>!!Socket!! Thread Setup Completed on Core: %d", Time_Stamp(Mode_ms), sched_getcpu());

	if(Connect_Socket() != 0)
	{
		syslog(LOG_ERR, "<%.6fms>!!Socket!! Thread Setup Failed", Time_Stamp(Mode_ms));
		pthread_exit(0);
	}

	while(frames < No_of_Frames)
	{
		// Set the timeout to 1 second for queue timed receive
		if(clock_gettime(CLOCK_REALTIME, &curr_time) != 0)
		{
			syslog(LOG_ERR, "<%.6fms>!!Socket!! Couldn't get time for curr_time", Time_Stamp(Mode_ms));
		}

		curr_time.tv_sec += 1;

		q_recv_resp = mq_timedreceive(socket_queue, (char *)&file_in, sizeof(store_struct), 0, &curr_time);

		if(sock_index == 0)
		{
			Socket_Start_Stamp = Time_Stamp(Mode_ms);
		}

		Socket_Stamp_1 = Time_Stamp(Mode_ms);

		// Check the result of timed receive
		if((q_recv_resp < 0) && (errno != ETIMEDOUT))
		{
			syslog(LOG_ERR, "<%.6fms>!!Socket!! Queue receiving Error", Time_Stamp(Mode_ms));
			errno_exit("queue receive");
		}

		else if(q_recv_resp == sizeof(store_struct))
		{
			// Reset frame attempt counter
			Frame_Attempt = 0;

			while(Frame_Attempt < Frame_Socket_Max_Retries)
			{
				// Send frame information structure first
				total_bytes = 0;
				do
				{
					written_bytes = write(new_socket, &file_in, sizeof(store_struct));
					if(written_bytes > 0)
					{
						total_bytes += written_bytes;
					}
				} while(total_bytes < sizeof(store_struct));

				// Copy converted RGB image from RAM circular buffer to local RAM buffer
				memcpy(&frame_data[0], file_in.dataptr, file_in.filesize);

				// Send all of the pixels
				total_bytes = 0;
				buf_index = 0;
				do
				{
					written_bytes = write(new_socket, &frame_data[buf_index], Segment_Size);
					if(written_bytes > 0)
					{
						buf_index += written_bytes;
						total_bytes += written_bytes;
					}
				} while(total_bytes < Big_Buffer_Size);

				// Wait for confirmation from remote
				total_bytes = 0;
				do
				{
					read_bytes = read(new_socket, &frame_confirm, sizeof(uint32_t));
					if(read_bytes > 0)
					{
						total_bytes += read_bytes;
					}
				} while(total_bytes < sizeof(uint32_t));

				Frame_Attempt += 1;

				if(frame_confirm == frames)
				{
					syslog(LOG_INFO, "<%.6fms>!!Socket!! Successfully Sent Frame Over Network: %d", Time_Stamp(Mode_ms), frames);
					frames += 1;
					break;
				}
			}

			// Log error if frame wasn't received
			if(Frame_Attempt == Frame_Socket_Max_Retries)
			{
				syslog(LOG_ERR, "<%.6fms>!!Socket!! Failed to Send Frame over Network: %d", Time_Stamp(Mode_ms), frames);
				frames += 1;
			}

			Socket_Stamp_2 = Time_Stamp(Mode_ms);

			Analysis.Exec_Analysis.Socket_Exec[sock_index] = Socket_Stamp_2 - Socket_Stamp_1;

			sock_index += 1;

		}
	}

	Socket_End_Stamp = Time_Stamp(Mode_ms);

	Analysis.Jitter_Analysis.Overall_Jitter[Socket_TID] = Socket_End_Stamp - (Socket_Start_Stamp + (No_of_Frames * Deadline_ms));

	// Log exit event
	syslog (LOG_INFO, "<%.6fms>!!Socket!! Exiting...", Time_Stamp(Mode_ms));

	// Close queue
	if(mq_close(socket_queue) != 0)
	{
		syslog(LOG_ERR, "<%.6fms>!!Socket!! Couldn't close Socket Queue", Time_Stamp(Mode_ms));
	}

	// Unlink queue
	if(mq_unlink(socket_q_name) != 0)
	{
		syslog(LOG_ERR, "<%.6fms>!!Socket!! Couldn't unlink Socket Queue", Time_Stamp(Mode_ms));
	}

	// Close socket
	if(close(new_socket) != 0)
	{
		syslog(LOG_ERR, "<%.6fms>!!Socket!! Couldn't close File Descriptor new_socket", Time_Stamp(Mode_ms));
	}

	return 0;
}
