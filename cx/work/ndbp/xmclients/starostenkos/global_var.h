#ifndef __GLOBAL_VAR_H
#define __GLOBAL_VAR_H

// __STAROMAKH_PROTO_H

enum {STAROMAKH_PORT = 8008}; // Heh, that's where Itus lived...

enum {STAROMAKH_MAGIC = 0x486b414d};

enum {STAROMAKH_NUM_SUBG = 6};
enum {STAROMAKH_NUM_MAG_SYS = 50}; 
enum {STAROMAKH_NUM_IPP = 31*6};
enum {STAROMAKH_NUM_VACUUM = 32*2};

enum
{
    STAROMAKH_CODE_MEASURED = 0xDA1AFEED,
    STAROMAKH_CODE_IMMREQ   = 0xDA1ADA1A,
    STAROMAKH_CODE_WR_REQ   = 0x5E10DA1A,
////    STAROMAKH_CODE_WR_RESP  = 0xDA1A5E10,
};


typedef struct
{
    int  magicnumber;
    int  packet_size;
    int  code;
    int  _reserved;
    char data[0];
} staromakh_pkthdr_t;

typedef struct
{
    int  magicnumber;
    int  packet_size;
    int  code;
    int  _reserved;
    double  value;
    int     number;
} staromakh_setvalue_rec_t;

// __STAROMAKH_PROTO_H



#define height 494
#define width 659
//#define height 493
//#define width 654


#define LED_Y_TIME_LIGHT 0.05


#define data_type unsigned short int

//typedef data_type array_dt_trans[width][height];
typedef data_type array_dt[height][width];




array_dt buffer_bg,buffer,buffer_w,buffer_s,bbb;
char tmp_buff[400];
int mesg_count;

int P_main,P_pref,P_control,P_options,P_options_set,P_oscillograms,P_image_set,P_calibration,P_calibration_data[5],P_scan,P_linac,P_mag_sys,P_ipp,P_set_ipp,P_profil,P_constant,P_alfo,P_vacuum,P_vepp4;
int P_main_vis,P_pref_vis,P_control_vis,P_options_set_vis,P_options_vis, P_oscillograms_vis,P_image_set_vis,P_calibration_vis,P_calibration_data_vis,P_scan_vis,P_linac_vis,P_mag_sys_vis,P_ipp_vis,P_set_ipp_vis,P_profil_vis,P_constant_vis,P_alfo_vis,P_vacuum_vis,P_vepp4_vis;

int calibration_data_panels_count;
double scanx[width];
int count,count_oscill,nnnm,pict_count;

unsigned short *p_data;//указатель на массив с изображением buffer
unsigned char *p_oscill_data;//указатель на массив с графиками oscillograms_data


#define max_device_num 10
#define max_server_name_len 100
#define max_controls_num 20

unsigned int image_server,control_server,penguin_server;
char device_name[max_device_num][max_server_name_len],
	image_server_name[max_device_num][max_server_name_len],
	control_server_name[max_device_num][max_server_name_len],
	penguin_server_name[max_device_num][max_server_name_len],
	controls_name[max_controls_num][100];
char device_name_tmp[max_server_name_len],
	image_server_name_tmp[max_server_name_len],
	control_server_name_tmp[max_server_name_len],
	penguin_server_name_tmp[max_server_name_len];
int image_server_port[max_device_num],
	control_server_port[max_device_num],
	penguin_server_port[max_device_num];
int image_server_port_tmp,control_server_port_tmp,penguin_server_port_tmp,
	device_number_of_controls_tmp;
int image_server_connect,control_server_connect,penguin_server_connect;
int current_device;
int device_number_of_controls[max_device_num];

int controls[max_controls_num],controls_c[max_controls_num];
int v_controls_u[STAROMAKH_NUM_VACUUM/2],v_controls_i[STAROMAKH_NUM_VACUUM/2];

typedef struct 
{
	char name[10];
	double min;
	double max;
	double inc;
	int prec;
}controls_properties;

controls_properties controls_current_device[max_controls_num],controls_vacuum[STAROMAKH_NUM_VACUUM/2];

#define max_undo_num 1000
typedef struct 
{
	double data[max_undo_num];
	int control[max_undo_num];
	int count;
	int max;
}undo;
undo undo_controls,undo_linac_controls,undo_buncher_controls;

#define GUS_MAGIC 0x55555555


typedef struct
{
	unsigned short int brightness;
	unsigned short int exposition;
	unsigned short int w;
	unsigned short int h;
}gusev_image_param;

typedef struct
{
	unsigned int len;
	unsigned int magic;
	char ID[10];
	unsigned short int SqN;
	unsigned short int OpC;
	unsigned short int Status;
	//char data[0];
}gusev_pkt_header;

typedef struct
{
	gusev_pkt_header h;
	unsigned short int usi;
}gusev_pkt_req_oscill;

typedef struct
{
	gusev_pkt_header h;
	unsigned short int usi;
	double value;
}gusev_pkt_set_control;

enum IPP_Param
{
	NUM_IPP = 6,
	NUM_COLS =15,
	MAX_COSMO = 100
};
int must_get_cosmo,num_cosmo;

typedef struct
{
	int param;
	int x[NUM_COLS];
	int y[NUM_COLS];
	int x_cosmo[NUM_COLS];
	int y_cosmo[NUM_COLS];
}ipp_data_type;



enum requstLen 
{
	HEAD_LEN		= sizeof( gusev_pkt_header ),
	MAX_HEAD_LEN =4154+4*4,
	REQ_LEN_UPDATE	= HEAD_LEN +  sizeof(unsigned short) + sizeof(double),
	REQ_LEN_DATA = HEAD_LEN,
	REQ_LEN_OSCILL	= HEAD_LEN +  sizeof(unsigned short),
	REQ_LEN_SET_OSCILL=HEAD_LEN + sizeof(unsigned short)+2*sizeof(unsigned char),
	REQ_LEN_CAMERA_START=HEAD_LEN,
	REQ_LEN_CAMERA_SET=HEAD_LEN + 2*sizeof(unsigned short),
	REQ_LEN_CAMERA_DATA = HEAD_LEN,
	RET_LEN_CAMERA_DATA = HEAD_LEN + 4*sizeof(unsigned short),
	REQ_LEN_ALFO = HEAD_LEN+2*sizeof(unsigned int)
};

gusev_pkt_set_control gusev_set_one_control;
gusev_pkt_req_oscill gusev_oscill;
gusev_pkt_header gus_header,gus_header_r;
gusev_image_param image_param;

int oscillograms_mask;


int acc_cosmo_x[MAX_COSMO][NUM_COLS],acc_cosmo_y[MAX_COSMO][NUM_COLS];

ipp_data_type ipp_data[NUM_COLS];

double x_ipp[NUM_COLS],y_ipp[NUM_COLS];

double control_val_for_get_cosmo;
int    control_num_for_get_cosmo;
int zzzlll,zzzlll_2;



#endif
