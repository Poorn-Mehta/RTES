/*
*		File: Storage.c
*		Purpose: The source file containing functions to store all the images on local Flash memory
*		Owner: Poorn Mehta
*		Last Modified: 8/11/2019
*/

#include "main.h"
#include "Aux_Func.h"
#include "Cam_Func.h"
#include "Storage.h"

// Shared Variables
uint8_t Terminate_Flag;
uint8_t data_buffer[No_of_Buffers][Big_Buffer_Size];
strct_analyze Analysis;
uint32_t Deadline_ms;

// Local Variables
static store_struct file_in;
static mqd_t storage_queue;
static struct timespec curr_time;
static int q_recv_resp, dumpfd, total_bytes, written_bytes;
static uint32_t frames, store_index;
static float Storage_Stamp_1, Storage_Stamp_2;
static float Storage_Start_Stamp, Storage_End_Stamp;

// Function to setup Storage Queue in Read Only mode
// Parameter1: void
// Return: uint8_t result - 0: success
static uint8_t Storage_Q_Setup(void)
{
	struct mq_attr storage_queue_attr;
	storage_queue_attr.mq_maxmsg = queue_size;
	storage_queue_attr.mq_msgsize = sizeof(store_struct);

	storage_queue = mq_open(storage_q_name, O_CREAT | O_RDONLY | O_CLOEXEC, 0666, &storage_queue_attr);
	if(storage_queue == -1)
	{
		syslog(LOG_ERR, "<%.6fms>Queue opening error for Storage", Time_Stamp(Mode_ms));
		return 1;
	}
	return 0;
}

// Function that implements Storage Thread
void *Storage_Func(void *para_t)
{

	// Setup
	if(Storage_Q_Setup() != 0)
	{
		syslog(LOG_ERR, "<%.6fms>!!Storage!! Thread Setup Failed", Time_Stamp(Mode_ms));
		pthread_exit(0);
	}

	frames = 0;
	store_index = 0;

	syslog(LOG_INFO, "<%.6fms>!!Storage!! Thread Setup Completed on Core: %d", Time_Stamp(Mode_ms), sched_getcpu());

	while(frames < No_of_Frames)
	{
		// Set the timeout to 1 second for queue timed receive
		if(clock_gettime(CLOCK_REALTIME, &curr_time) != 0)
		{
			syslog(LOG_ERR, "<%.6fms>!!Storage!! Couldn't get time for curr_time", Time_Stamp(Mode_ms));
		}

		curr_time.tv_sec += 1;

		q_recv_resp = mq_timedreceive(storage_queue, (char *)&file_in, sizeof(store_struct), 0, &curr_time);

		if(store_index == 0)
		{
			Storage_Start_Stamp = Time_Stamp(Mode_ms);
		}

		Storage_Stamp_1 = Time_Stamp(Mode_ms);

		// Check the result of timed receive
		if((q_recv_resp < 0) && (errno != ETIMEDOUT))
		{
			syslog(LOG_ERR, "<%.6fms>!!Storage!! Queue receiving Error", Time_Stamp(Mode_ms));
			errno_exit("queue receive");
		}

		else if(q_recv_resp == sizeof(store_struct))
		{

			// Create the file
			dumpfd = open(&file_in.filename[0], O_WRONLY | O_CREAT, 00666);

			if(dumpfd == -1)
			{
				syslog(LOG_ERR, "<%.6fms>!!Storage!! Couldn't open file for storing image", Time_Stamp(Mode_ms));
			}

			// Write header in file
			written_bytes = write(dumpfd, &file_in.header[0], file_in.headersize);

			total_bytes = 0;

			// Write the entire buffer having the image information to the file
			do
			{
				written_bytes = write(dumpfd, file_in.dataptr, file_in.filesize);
				total_bytes += written_bytes;
			} while(total_bytes < file_in.filesize);

			// Close file
			if(close(dumpfd) != 0)
			{
				syslog(LOG_ERR, "<%.6fms>!!Storage!! Couldn't close File Descriptor dumpfd", Time_Stamp(Mode_ms));
			}

			syslog(LOG_INFO, "<%.6fms>!!Storage!! Successfully Stored Frame: %d", Time_Stamp(Mode_ms), frames);

			frames += 1;

			Storage_Stamp_2 = Time_Stamp(Mode_ms);

			Analysis.Exec_Analysis.Storage_Exec[store_index] = Storage_Stamp_2 - Storage_Stamp_1;

			store_index += 1;

		}
	}

	Storage_End_Stamp = Time_Stamp(Mode_ms);

	Analysis.Jitter_Analysis.Overall_Jitter[Storage_TID] = Storage_End_Stamp - (Storage_Start_Stamp + (No_of_Frames * Deadline_ms));

	// Log the exiting event
	syslog (LOG_INFO, "<%.6fms>!!Storage!! Exiting...", Time_Stamp(Mode_ms));

	// Close the queue
	if(mq_close(storage_queue) != 0)
	{
		syslog(LOG_ERR, "<%.6fms>!!Storage!! Couldn't close Storage Queue", Time_Stamp(Mode_ms));
	}

	// Unlink the queue
	if(mq_unlink(storage_q_name) != 0)
	{
		syslog(LOG_ERR, "<%.6fms>!!Storage!! Couldn't unlink Storage Queue", Time_Stamp(Mode_ms));
	}

	pthread_exit(0);
}
