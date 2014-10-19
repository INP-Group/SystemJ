#ifndef __TSYCAMLIB_H
#define __TSYCAMLIB_H


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


enum {DEFAULT_TSYCAM_PORT = 4001};

/* Library-wide behaviour control */
int   tc_set_io_mode(int mode);
void  tc_SIGIO_handler(int sig);

/* Basic functionality */
void *tc_create_camera_object(const char *host, short port,
                              int w, int h,
                              void *framebuffer, size_t fbusize);
int   tc_set_parameter    (void *ref, const char *name, int value);
int   tc_request_new_frame(void *ref, int *timeout_p);
int   tc_decode_frame     (void *ref);
int   tc_missing_count    (void *ref);

/* Asynchronous functionality */
int   tc_fd_of_object(void *ref);
int   tc_fdio_proc   (void *ref, int *timeout_p);
int   tc_timeout_proc(void *ref, int *timeout_p);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __TSYCAMLIB_H */
