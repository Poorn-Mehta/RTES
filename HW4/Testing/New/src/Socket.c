#include "main.h"

extern char target_ip[20];
extern strct_analyze Analysis;
extern uint32_t Deadline_ms;

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

static uint8_t Socket_Q_Setup(void)
{
	// Queue setup
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

static uint8_t Connect_Socket(void)
{
	while(retry_count < Max_Retries)
	{
		if(connect(new_socket, (struct sockaddr *)&client, sizeof(client)) == 0)
		{
			syslog(LOG_INFO, "<%.6fms>!!Socket!! *EVENT* Connection to the Remote Succeeded", Time_Stamp(Mode_ms));
			return 0;
		}
		
		else
		{
			syslog(LOG_ERR, "<%.6fms>!!Socket!! *EVENT* Connection Attempt to the Remote Failed", Time_Stamp(Mode_ms));
		}

		retry_count += 1;
	}
	return 1;
}

static uint8_t Socket_Init(void)
{
	retry_count = 0;

	// socket init on client end
	new_socket = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if(new_socket < 0)
	{
		syslog(LOG_ERR, "<%.6fms>!!Socket!! *EVENT* Socket Creation Failed", Time_Stamp(Mode_ms));
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

	if(inet_pton(AF_INET, target_ip, &client.sin_addr)<=0)
	{
		syslog(LOG_ERR, "<%.6fms>!!Socket!! *EVENT* Error in IP Address", Time_Stamp(Mode_ms));
		return 1;
	}

	client.sin_port = htons(Port_Num);

	return 0;

}

void *Socket_Func(void *para_t)
{

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

	syslog(LOG_INFO, "<%.6fms>!!Socket!! *EVENT* Thread Setup Completed on Core: %d", Time_Stamp(Mode_ms), sched_getcpu());

	if(Connect_Socket() != 0)
	{
		syslog(LOG_ERR, "<%.6fms>!!Socket!! Thread Setup Failed", Time_Stamp(Mode_ms));
		pthread_exit(0);
	}

	while(frames < No_of_Frames)
	{
		clock_gettime(CLOCK_REALTIME, &curr_time);
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
			Frame_Attempt = 0;
			while(Frame_Attempt < Frame_Socket_Max_Retries)
			{
				total_bytes = 0;
				do
				{
					written_bytes = write(new_socket, &file_in, sizeof(store_struct));
					if(written_bytes > 0)
					{
						total_bytes += written_bytes;
					}
				} while(total_bytes < sizeof(store_struct));

				memcpy(&frame_data[0], file_in.dataptr, file_in.filesize);

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
					syslog(LOG_INFO, "<%.6fms>!!Socket!! Successfully Sent Frame: %d", Time_Stamp(Mode_ms), frames);
					frames += 1;
					break;
				}
			}

			Socket_Stamp_2 = Time_Stamp(Mode_ms);

			Analysis.Exec_Analysis.Socket_Exec[sock_index] = Socket_Stamp_2 - Socket_Stamp_1;

			sock_index += 1;

		}
	}

	Socket_End_Stamp = Time_Stamp(Mode_ms);

	Analysis.Jitter_Analysis.Overall_Jitter[Socket_TID] = Socket_End_Stamp - (Socket_Start_Stamp + (No_of_Frames * Deadline_ms));

	syslog (LOG_INFO, "<%.6fms>!!Socket!! Exiting...", Time_Stamp(Mode_ms));
	mq_close(socket_queue);
	mq_unlink(socket_q_name);
	close(new_socket);
	return 0;
}
