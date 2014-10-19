unsigned short int ch_count,mask;


int CVICALLBACK Get_image(unsigned handle, int event, int error, void *callbackData);
int CVICALLBACK Get_control(unsigned handle, int event, int error, void *callbackData);
int CVICALLBACK Get_penguin(unsigned handle, int event, int error, void *callbackData);
int Request_image(void);
int Request_control_data(void);
int Request_oscillograms(unsigned short m_o);
int Shift_ud(int p);
int Shift_lr(int p);
int TCP_send_CDM_control(int control_server,unsigned short int n,double value);
int TCP_send_linac_control(int penguin_server,unsigned short int n,double val);
int TCP_send_ALFO(int control_server,unsigned int N,unsigned int A,unsigned int F,unsigned int Data);
int Set_oscillographs_parameters(int control_server,unsigned short int n_o,unsigned char n_p,unsigned char param);
