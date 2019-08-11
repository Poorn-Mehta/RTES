#ifndef	__CAM_FUNC_H__
#define __CAM_FUNC_H__

#define MMAP_Buffers_Count	(uint8_t)2

#define Warmup_Select_Timeout_s	(uint8_t)2

uint8_t device_warmup(void);
void errno_exit(const char *s);
int xioctl(int fh, int request, void *arg);
void stop_capturing(void);
void start_capturing(void);
void uninit_device(void);
void init_mmap(uint32_t buffer_size);
void init_device(void);
void close_device(void);
void open_device(void);

#endif
