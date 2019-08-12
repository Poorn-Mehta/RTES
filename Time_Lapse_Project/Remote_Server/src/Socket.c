#include "main.h"

static store_struct frame_info;
static uint8_t frame_data[Big_Buffer_Size], frame_info_buffer[500];
static int new_socket, custom_socket, total_w_bytes, written_bytes, total_r_bytes, read_bytes, dumpfd;
static struct sockaddr_in custom_server;
static uint32_t frames, buf_index, total_frames;
static int tmp_opt = 1;

static uint8_t Socket_Init(void)
{

	// socket init on server end
	new_socket = socket(AF_INET, SOCK_STREAM, 0);
	if(new_socket < 0)
	{
		syslog(LOG_ERR, "<%.6fms>!!Socket!! Socket Creation Failed", Time_Stamp(Mode_ms));
		return 1;
	}

	// Forcefully attaching socket to the port 8080 
	if (setsockopt(new_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &tmp_opt, sizeof(tmp_opt))) 
	{ 
		syslog(LOG_ERR, "<%.6fms>!!Socket!! Error in Setsockopt", Time_Stamp(Mode_ms));
		return 1;
	} 

	custom_server.sin_family = AF_INET;
	custom_server.sin_addr.s_addr = INADDR_ANY;
	custom_server.sin_port = htons(Port_Num);

	if(bind(new_socket, (struct sockaddr *)&custom_server, sizeof(custom_server)) < 0)
	{
		syslog(LOG_ERR, "<%.6fms>!!Socket!! Error in Bind", Time_Stamp(Mode_ms));
		return 1;
	}

	if(listen(new_socket, Listen_Queue_Length) < 0)
	{
		syslog(LOG_ERR, "<%.6fms>!!Socket!! Error in Listen", Time_Stamp(Mode_ms));
		return 1;
	}

	return 0;
}

void *Socket_Func(void *para_t)
{

	if(Socket_Init() != 0)
	{
		syslog(LOG_ERR, "<%.6fms>!!Socket!! Thread Setup Failed", Time_Stamp(Mode_ms));
		pthread_exit(0);
	}

	syslog(LOG_INFO, "<%.6fms>!!Socket!! Now Blocking on Accept Call", Time_Stamp(Mode_ms));

	custom_socket = accept(new_socket, (struct sockaddr *)0, 0);

	if(custom_socket < 0)
	{
		syslog(LOG_ERR, "<%.6fms>!!Socket!! Error in Accept", Time_Stamp(Mode_ms));
		pthread_exit(0);
	}

	else
	{
		syslog(LOG_INFO, "<%.6fms>!!Socket!! Accept Succeeded", Time_Stamp(Mode_ms));
	}

	frames = 0;
	total_frames = Default_Frames;

	syslog(LOG_INFO, "<%.6fms>!!Socket!! Setup Completed", Time_Stamp(Mode_ms));

	while(frames < total_frames) 
	{

		total_r_bytes = 0;
		read_bytes = 0;
		buf_index = 0;

		do
		{
			read_bytes = read(custom_socket, &frame_info_buffer[buf_index], sizeof(store_struct));
			if(read_bytes > 0)
			{
				buf_index += read_bytes;
				total_r_bytes += read_bytes;
			}
		} while(total_r_bytes < sizeof(store_struct));

		memcpy(&frame_info, &frame_info_buffer[0], sizeof(store_struct));
//		read_bytes = read(custom_socket, &frame_info, sizeof(store_struct));

		total_r_bytes = 0;
		read_bytes = 0;
		buf_index = 0;

		do
		{
			read_bytes = read(custom_socket, &frame_data[buf_index], Segment_Size);
			if(read_bytes > 0)
			{
				buf_index += read_bytes;
				total_r_bytes += read_bytes;
			}
		} while(total_r_bytes < Big_Buffer_Size);

		syslog(LOG_INFO, "<%.6fms>!!Socket!! Successfully Received Frame: %d", Time_Stamp(Mode_ms), frames);

		total_frames = frame_info.total_frames;

		dumpfd = open(frame_info.filename, O_WRONLY | O_CREAT, 00666);

		// Write header in file
		written_bytes = write(dumpfd, frame_info.header, frame_info.headersize);

		total_w_bytes = 0;

		// Write the entire buffer having the image information to the file
		do
		{
			written_bytes = write(dumpfd, &frame_data[0], frame_info.filesize);
			total_w_bytes += written_bytes;
		} while(total_w_bytes < frame_info.filesize);

		// Close file
		close(dumpfd);

		syslog(LOG_INFO, "<%.6fms>!!Socket!! Successfully Stored Frame: %d", Time_Stamp(Mode_ms), frames);

		total_w_bytes = 0;

		do
		{
			written_bytes = write(custom_socket, &frames, sizeof(uint32_t));
			if(written_bytes > 0)
			{
				total_w_bytes += written_bytes;
			}
		} while(total_w_bytes < sizeof(uint32_t));

		frames += 1;
	}
	syslog (LOG_INFO, "<%.6fms>!!Socket!! Exiting...", Time_Stamp(Mode_ms));
	close(custom_socket);
	pthread_exit(0);
}
