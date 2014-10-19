
typedef struct {
    unsigned short n;
    unsigned short a;
    unsigned short f;
    unsigned short status;
    unsigned *buf;
} cam_header_t;

#define CAM_IOCTL_LAMENABLE	1
#define CAM_IOCTL_LAMDISABLE	2
#define CAM_IOCTL_EXC 		3
#define CAM_IOCTL_EXZ 		4
#define CAM_IOCTL_SETI 		5
#define CAM_IOCTL_GETI 		6
#define CAM_IOCTL_LAMPENDING 	7

#ifdef MODULE

typedef void (*LAMHANDLER) (int);

extern int cam_bus_read (char *buf, int len);
extern int cam_bus_write (const char *buf, int len);
extern void cam_execute_C (void);
extern void cam_execute_Z (void);
extern int cam_get_I(void);
extern void cam_set_I(int);
extern int cam_bus_lamenable(int lam, LAMHANDLER handler);
extern int cam_bus_lamdisable(int lam);
extern int cam_bus_lampending(void);
#endif