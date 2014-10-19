#include "toolbox.h"
#include "cdm.h"
#include <ansi_c.h>
#include <utility.h>
#include <tcpsupp.h>
#include <userint.h>
#include "all.h"



int CVICALLBACK Get_image(unsigned handle, int event, int error, void *callbackData) 
{
int  dataSize=width*height;
static unsigned short int buff[width*height];
double time_out;


    switch (event) {
        case TCP_DATAREADY:
        {
            SetCtrlVal(P_main, P_MAIN_LED_Y, 1);
            GetCtrlAttribute (P_main, P_MAIN_TIMER_1, ATTR_INTERVAL,&time_out);
			ResetTimer (P_main, P_MAIN_TIMER_1);

            if (count==0)
            {
            
                char bufff[RET_LEN_CAMERA_DATA];
                unsigned short *param = ( unsigned short * ) &bufff[HEAD_LEN];
                gusev_pkt_header *head= (gusev_pkt_header *) bufff;
            
	            SetCtrlAttribute (P_main, P_MAIN_TIMER_1, ATTR_ENABLED, 1);
            	p_data=buffer[0];
            	if(ClientTCPRead(handle, bufff, RET_LEN_CAMERA_DATA, 100)<0) 
            	{
                	MessagePopup("TCP Client", "Image header read error!");
	                SetCtrlVal(P_main, P_MAIN_LED_G, 0);
    	            break;
        	    }
        	    image_param.brightness = param[0];
        	    image_param.exposition = param[1];
        	    image_param.w		   = param[2];
        	    image_param.h		   = param[3];
        	    
        	    SetCtrlVal(P_image_set,P_IM_SET_BRIGHTNESS_IND,image_param.brightness);
        	    SetCtrlVal(P_image_set,P_IM_SET_EXPOSITION_IND,image_param.exposition);
            }
            if((dataSize = ClientTCPRead(handle, &buff, width*height, 10))<0) 
            {
                MessagePopup("TCP Client", "Image read error!");
                SetCtrlVal(P_main, P_MAIN_LED_G, 0);
                break;
            }  
			else	
			{
				if ((count+dataSize/2)<=width*height)
				{
					memcpy(p_data,buff,dataSize);
					p_data+=dataSize/2;
					count+=dataSize/2;	
					nnnm++; //hlam
					SetCtrlVal(P_main, P_MAIN_TRANSFER, (float)count/(width*height));
					SetCtrlVal(P_main, P_MAIN_LED_Y, 0);
				}
				if(count>=width*height)
				{
					count=0;
					SetCtrlAttribute (P_main, P_MAIN_TIMER_1, ATTR_ENABLED, 0);
		            SetCtrlVal(P_main, P_MAIN_LED_R, 1);
				
					
					processing();
	            	SetCtrlVal(P_main, P_MAIN_LED_R, 0);			
		            nnnm=0;
		            pict_count++;
	            
					Request_image();
				}	
        	}
        	ResetTimer (P_main, P_MAIN_TIMER_1);
       }   
            break;
        case TCP_DISCONNECT:
            MessagePopup("TCP Client", "Image server has closed connection!");
            image_server_connect=0;
            SetCtrlVal(P_main, P_MAIN_LED_G, 0);
            break;
    }
    return 0;
}

int CVICALLBACK Get_control(unsigned handle, int event, int error, void *callbackData) 
{
int i,dataSize,bytes_read;
unsigned short nc; 
double val,i_v;
unsigned int Data_rb,Data_x,Data_q;
unsigned char buff[1000],buff_work[1000];
gusev_pkt_header *gus_header_r_tmp=(gusev_pkt_header *) buff;
char mesg[200];
    switch (event) {
        case TCP_DATAREADY:
        {
			bytes_read=0;        
            if((dataSize=ClientTCPRead(handle, buff,sizeof(buff), 100))<0) 
   			{
        		MessagePopup("TCP Client", "Control server read error!");
                SetCtrlVal(P_control, P_CONTROL_LED_G, 0);
   		       	break;
	       	}
       		if (dataSize>=sizeof(gusev_pkt_header))
       		{
       			//memcpy(&gus_header_r_tmp,buff,HEAD_LEN);
    		   	if( (gus_header_r_tmp->magic==GUS_MAGIC)&&
    		   		(gus_header_r_tmp->len>=HEAD_LEN)&&
    		   		(gus_header_r_tmp->len<=MAX_HEAD_LEN))
    		   	{
			    	memcpy(&gus_header_r,gus_header_r_tmp,HEAD_LEN);
			    	memcpy(buff_work,&buff[HEAD_LEN],dataSize-HEAD_LEN);
			    	dataSize-=HEAD_LEN;
					count_oscill=0;
			    }
			    else
			    	memcpy(buff_work,buff,dataSize);
			}

       			
			switch(gus_header_r.OpC)
			{
				case 1: //return confirmation change control data
				{
				char control_label[20];
					if(dataSize<sizeof(unsigned short)+sizeof(double))
					{
        				MessagePopup("TCP Client", "Control server read error:\n missing confirmation data");
						break;
					}
	       			
	       			SetCtrlAttribute (P_control, P_LINAC_TIMER, ATTR_ENABLED, 1);
    	   			SetCtrlAttribute (P_control, P_LINAC_TIMER, ATTR_INTERVAL, LED_Y_TIME_LIGHT);

					nc=*((unsigned short*)&buff_work[bytes_read]);
					bytes_read+=sizeof(unsigned short);
					control_data[nc]=*((double*)&buff_work[bytes_read]);
					bytes_read+=sizeof(double);

					GetCtrlVal(P_control,controls[nc],&val);
					GetCtrlAttribute(P_control,controls[nc],ATTR_INCR_VALUE,&i_v);
					SetCtrlVal(P_control, controls_c[nc], control_data[nc]);
					
					GetCtrlAttribute (P_control, controls[nc],ATTR_LABEL_TEXT, control_label);
					
					strcpy(mesg,control_label);
					strcat(mesg," was changing by ");
					strcat(mesg,gus_header_r.ID);
					Message_str(mesg);
					
					
					if (fabs(val-control_data[nc])>i_v/2.0)
						SetCtrlAttribute (P_control, controls_c[nc], ATTR_TEXT_BGCOLOR,diff_color);
					else
						SetCtrlAttribute (P_control, controls_c[nc], ATTR_TEXT_BGCOLOR,def_color);
					
					SetCtrlVal(P_control, P_CONTROL_LED_Y, 0);
					break;
				}
				case 2: //return control data
				{
					if(dataSize<device_number_of_controls[current_device]*sizeof(double))
					{
        				MessagePopup("TCP Client", "Control server read error:\n missing control data");
						break;
					}
					for (i=0;i<device_number_of_controls[current_device];i++)
					{
						control_data[i]=*((double*)&buff_work[bytes_read]);
						bytes_read+=sizeof(double);
						SetCtrlVal(P_control,controls[i],control_data[i]);
						SetCtrlVal(P_control,controls_c[i],control_data[i]);
						SetCtrlAttribute (P_control, controls_c[i], ATTR_TEXT_BGCOLOR,def_color);					
						
					}
					SetCtrlVal(P_control, P_CONTROL_LED_Y, 0);
					break;
				}
				case 4:	// return oscillograms
				{
					if(count_oscill==0)
					{
						mask=*((unsigned short*)&buff_work[bytes_read]);
						oscillograms_mask=mask;
						bytes_read+=sizeof(unsigned short);
						dataSize-=sizeof(unsigned short);
						p_oscill_data=oscillograms_data;
						ch_count=0;
						
						for(i=0;i<4;i++)
							ch_count+=(mask>>i)&1;
					}
           			if ((count_oscill+dataSize)<=(1036*ch_count))
					{
						memcpy(p_oscill_data,&buff_work[bytes_read],dataSize);
						p_oscill_data+=dataSize;
						count_oscill+=dataSize;	
					}
					if(count_oscill>=(1036*ch_count))
					{
						count_oscill=0;
            			Plot_oscillograms(mask);
					}
					break;
				}
				case 5:	// ALFO confirmation/answer
				{
					Data_rb = *((unsigned int *) &buff_work[4]);
					Data_x = ((gus_header_r_tmp->Status)>>10)&1;
					Data_q = ((gus_header_r_tmp->Status)>>11)&1;
					SetCtrlVal(P_alfo, P_ALFO_DATA_C, Data_rb);
					SetCtrlVal(P_alfo, P_ALFO_LED_X,Data_x);
					SetCtrlVal(P_alfo, P_ALFO_LED_Q,Data_q);
					break;
				}
			}
			break;
		}
        case TCP_DISCONNECT:
            MessagePopup("TCP Client", "Control server has closed connection!");
            SetCtrlVal(P_control, P_CONTROL_LED_G, 0);
            control_server_connect=0;
            break;
    }
    return 0;

}
int CVICALLBACK Get_penguin(unsigned handle, int event, int error, void *callbackData) 
{
int i,j,k,r_offset;
staromakh_pkthdr_t penguin_data;
double val,i_v;
char str[20];

    switch (event) {
        case TCP_DATAREADY:
        {
            if((ClientTCPRead(handle, &penguin_data,sizeof(penguin_data), 100))<0) 
            {
            	//error!!
				break;
			}
       		if((penguin_data.code==STAROMAKH_CODE_MEASURED)&&(penguin_data.packet_size>sizeof(penguin_data)))
	   		{
       			SetCtrlAttribute (P_linac, P_LINAC_TIMER, ATTR_ENABLED, 1);
       			SetCtrlAttribute (P_linac, P_LINAC_TIMER, ATTR_INTERVAL, LED_Y_TIME_LIGHT);
       			SetCtrlVal(P_linac, P_LINAC_LED_Y, 1);
       			SetCtrlVal(P_mag_sys, P_MAG_SYS_LED_Y, 1);
       			SetCtrlVal(P_ipp, P_IPP_LED_Y, 1);
	            if((ClientTCPRead(handle, l_control_data,penguin_data.packet_size-sizeof(penguin_data), 100))!=(penguin_data.packet_size-sizeof(penguin_data))) 
    	        {
        	    	//error!!
					break;
				}
				for(j=0;j<STAROMAKH_NUM_SUBG;j++) 
				{
					GetCtrlVal(P_linac,P_LINAC_PHASE_1+j*2,&val);
					GetCtrlAttribute(P_linac,P_LINAC_PHASE_1+j*2,ATTR_INCR_VALUE,&i_v);
					
					SetCtrlVal(P_linac,P_LINAC_PHASE_1_C+j*2,l_control_data[j]);
					if (fabs(val-l_control_data[j])>i_v/2.0)
						SetCtrlAttribute (P_linac, P_LINAC_PHASE_1_C+j*2, ATTR_TEXT_BGCOLOR,diff_color);
					else
						SetCtrlAttribute (P_linac, P_LINAC_PHASE_1_C+j*2, ATTR_TEXT_BGCOLOR,def_color);					
				}
				for(j=0;j<STAROMAKH_NUM_MAG_SYS;j++) 
				{
					GetCtrlVal(P_mag_sys,P_MAG_SYS_N_1+j*2,&val);
					GetCtrlAttribute(P_mag_sys,P_MAG_SYS_N_1+j*2,ATTR_INCR_VALUE,&i_v);
				
					SetCtrlVal(P_mag_sys,P_MAG_SYS_C_1+j*2,l_control_data[j*2+1+STAROMAKH_NUM_SUBG]);

					if (fabs(val-l_control_data[j*2+STAROMAKH_NUM_SUBG])>i_v/2.0)
					{
						SetCtrlAttribute (P_mag_sys, P_MAG_SYS_N_1+j*2, ATTR_TEXT_BGCOLOR,diff_color);
						sprintf(str,"%f",l_control_data[j*2+STAROMAKH_NUM_SUBG]);
						SetCtrlToolTipAttribute (P_mag_sys, P_MAG_SYS_N_1+j*2,CTRL_TOOLTIP_ATTR_TEXT,str);
					}
					else
					{
						SetCtrlAttribute (P_mag_sys, P_MAG_SYS_N_1+j*2, ATTR_TEXT_BGCOLOR,0xffffff);
						SetCtrlToolTipAttribute (P_mag_sys, P_MAG_SYS_N_1+j*2,CTRL_TOOLTIP_ATTR_TEXT,"");
					}
				}
				r_offset=STAROMAKH_NUM_SUBG+STAROMAKH_NUM_MAG_SYS*2;
				for(i=0;i<NUM_IPP;i++)
				{
					ipp_data[i].param=(int)l_control_data[r_offset];
					r_offset++;
					for(j=0;j<NUM_COLS;j++) 				
					{
						ipp_data[i].x[j]=IPPvalue2int(l_control_data[r_offset]);
						r_offset++;
					}
					for(j=0;j<NUM_COLS;j++)
					{
						ipp_data[i].y[j]=IPPvalue2int(l_control_data[r_offset]);
						r_offset++;
					}
					if (must_get_cosmo)
					{
						for(j=0;j<NUM_COLS;j++)
						{
							acc_cosmo_x[i][j]+=ipp_data[i].x[j];
							acc_cosmo_y[i][j]+=ipp_data[i].y[j];
						}
						if (must_get_cosmo==1)
						{
							for(j=0;j<NUM_COLS;j++)
							{
								ipp_data[i].x_cosmo[j]=acc_cosmo_x[i][j]/num_cosmo;
								ipp_data[i].y_cosmo[j]=acc_cosmo_y[i][j]/num_cosmo;
								acc_cosmo_x[i][j]=0;
								acc_cosmo_y[i][j]=0;
							}
							write_bg_to_file(i,"ipp_bg.dat");
							SetCtrlVal(P_ipp,P_IPP_LED_R,0);
							if(TCP_send_linac_control(penguin_server,control_num_for_get_cosmo,control_val_for_get_cosmo)<0)
								MessagePopup("Penguin server", "Can't restore!");
							
						}
					}
				}
				r_offset=STAROMAKH_NUM_SUBG+STAROMAKH_NUM_MAG_SYS*2+STAROMAKH_NUM_IPP;
				for(j=0;j<STAROMAKH_NUM_VACUUM/2;j++) 
				{
					SetCtrlVal(P_vacuum,v_controls_u[j],l_control_data[j*2+r_offset]);
					SetCtrlVal(P_vacuum,v_controls_i[j],l_control_data[j*2+1+r_offset]);
					
				}
				
				draw_ipp();
				plot_ipp();
				if (must_get_cosmo) must_get_cosmo--;
       		}
        }   
            break;
        case TCP_DISCONNECT:
            MessagePopup("TCP Client", "Penguin server has closed connection!");
            SetCtrlVal(P_linac, P_LINAC_LED_G, 0);
            SetCtrlVal(P_mag_sys, P_MAG_SYS_LED_G, 0);
            SetCtrlVal(P_ipp, P_IPP_LED_G, 0);
            penguin_server_connect=0;
            break;
    }
    return 0;

}

int Request_image(void)
{
	static char request[REQ_LEN_CAMERA_DATA];
	static int  not_inited = 1;
	gusev_pkt_header *head = ( gusev_pkt_header * ) request;
	
	if ( not_inited ) {

		char comp_name[100];
		
		GetCompName(comp_name);
		head->len	= REQ_LEN_CAMERA_DATA;
		head->magic = GUS_MAGIC;
		memcpy(head->ID, comp_name, sizeof( head->ID));
		head->SqN 	= 0;
		head->OpC	= 12;
		not_inited  = 0;
	}
	
	head->SqN++;
	return ClientTCPWrite (image_server, request, head->len, 1000); 
}

int Request_oscillograms(unsigned short m_o)
{
static char request[REQ_LEN_OSCILL];
static int  not_inited = 1;
gusev_pkt_header *head = ( gusev_pkt_header * ) request;

	if ( not_inited ) 
	{
	char comp_name[100];

		GetCompName (comp_name);
		head->len	= REQ_LEN_OSCILL;
		head->magic = GUS_MAGIC;
		memcpy(head->ID,comp_name,sizeof( head->ID));
		head->SqN 	= 0;
		head->OpC	= 4;
	}
	
	*( ( unsigned short * ) &request[HEAD_LEN] ) = m_o;
	head->SqN++;
	
	return ClientTCPWrite (control_server, request, head->len, 1000); 
}

int Request_control_data(void)
{
char comp_name[100];

	GetCompName (comp_name);
	gus_header.len=sizeof(gus_header);
	gus_header.magic=GUS_MAGIC;
	memcpy(gus_header.ID,comp_name,sizeof(gus_header.ID));
	gus_header.SqN=888;
	gus_header.OpC=2;
	return ClientTCPWrite (control_server, &gus_header,gus_header.len, 1000); 
}

int Set_camera_parameters(void)
{
	static char		  request[REQ_LEN_CAMERA_SET];
	static int		  not_inited = 1;
	gusev_pkt_header *head = ( gusev_pkt_header * ) request;
	unsigned short	 *buff = ( unsigned short * ) &request[HEAD_LEN];

	if ( not_inited ) {

		char comp_name[100];
		
		GetCompName(comp_name);
		head->len	= REQ_LEN_CAMERA_SET;
		head->magic = GUS_MAGIC;
		memcpy(head->ID, comp_name, sizeof( head->ID));
		head->SqN 	= 333;
		head->OpC	= 11;
		not_inited = 0;
	}
	
	GetCtrlVal(P_image_set,P_IM_SET_BRIGHTNESS,&buff[0]);
	GetCtrlVal(P_image_set,P_IM_SET_EXPOSITION,&buff[1]);
	return ClientTCPWrite (image_server, request, head->len, 1000); 
};
int Start_camera(void)
{
	static char request[REQ_LEN_CAMERA_START];
	static int  not_inited = 1;
	gusev_pkt_header *head = ( gusev_pkt_header * ) request;
	
	if ( not_inited ) {

		char comp_name[100];
		
		GetCompName(comp_name);
		head->len	= REQ_LEN_CAMERA_START;
		head->magic = GUS_MAGIC;
		memcpy(head->ID, comp_name, sizeof( head->ID));
		head->SqN 	= 777;
		head->OpC	= 10;
		not_inited = 0;
	}
	return ClientTCPWrite (image_server, request, head->len, 1000); 
};


int CVICALLBACK Timer_tik (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
int status,i,summa;
	switch (event)
		{
		case EVENT_TIMER_TICK:
			summa=0;
			for (i=0;i<4;i++)
			{
				GetCtrlVal(P_oscillograms,P_OSCILL_AUTO_REFRESH_0_0+i,&status);
				summa+=status<<i;
			}
			SetCtrlAttribute(P_oscillograms,P_OSCILL_TIMER,ATTR_ENABLED,summa);
			Request_oscillograms(summa);
			break;
		}
	return 0;
}

int Shift_ud(int p)
{
double coeff=0.0001,value;
int cc;
unsigned short int cn;
	GetCtrlVal(P_calibration,P_CALIBR_CN,&cn);
	GetCtrlVal(P_control,controls[cn],&value);
	value+=coeff*p;
	SetCtrlVal(P_control,controls[cn],value);

	if (control_server_connect==0)
	{
		GetCtrlVal(P_main,P_MAIN_CDM_UN,&cc);			
	    if (ConnectToTCPServer (&control_server, control_server_port[cc], control_server_name[cc], Get_control,0, 1000)<0) 
		{
			SetCtrlVal(P_control, P_CONTROL_LED_G, 0);
    	    MessagePopup("TCP Client", "Connection to control server failed !");
    	    control_server_connect=0;
	    } 
	    else 
	    {
			SetCtrlVal(P_control, P_CONTROL_LED_G, 1);
			control_server_connect=1;
		}
	}
	return TCP_send_CDM_control(control_server,cn,value);
}

int TCP_send_CDM_control(int control_server,unsigned short int n,double value)
{
static char request[REQ_LEN_UPDATE];
static char comp_name[100];
gusev_pkt_header *head = ( gusev_pkt_header * ) request;

	GetCompName (comp_name);
	head->len	= REQ_LEN_UPDATE;
	head->magic = GUS_MAGIC;
	memcpy(head->ID,comp_name,sizeof( head->ID));
	head->SqN 	= 100;
	head->OpC	= 1;
	*( ( unsigned short * ) &request[HEAD_LEN] ) = n;
	*( ( double * ) &request[HEAD_LEN + sizeof( unsigned short ) ] ) = value;
	return ClientTCPWrite (control_server, request, head->len, 1000); 
};

int TCP_send_linac_control(int penguin_server,unsigned short int n,double val)
{
staromakh_setvalue_rec_t s;

	s.magicnumber=STAROMAKH_MAGIC;
	s.packet_size=sizeof(s);
	s.code=STAROMAKH_CODE_WR_REQ;
	s._reserved=0;
	s.number=n;
	s.value=val;
	return	ClientTCPWrite (penguin_server, &s, sizeof(s), 1000);	
};
int TCP_send_ALFO(int control_server,unsigned int N,unsigned int A,unsigned int F,unsigned int Data)
{
static char request[REQ_LEN_ALFO];
static char comp_name[100];
gusev_pkt_header *head = ( gusev_pkt_header * ) request;

	GetCompName (comp_name);
	head->len	= REQ_LEN_ALFO;
	head->magic = GUS_MAGIC;
	memcpy(head->ID,comp_name,sizeof( head->ID));
	head->SqN 	= 111;
	head->OpC	= 5;
	request[HEAD_LEN + 0] = N;
	request[HEAD_LEN + 1] = A;
	request[HEAD_LEN + 2] = F;
	*( ( unsigned int * ) &request[HEAD_LEN + 4] ) = Data;
	return ClientTCPWrite (control_server, request, head->len, 1000); 
};


int Set_oscillographs_parameters(int control_server,unsigned short int n_o,unsigned char n_p,unsigned char param)
{
static char request[REQ_LEN_SET_OSCILL];
static char comp_name[100];
gusev_pkt_header *head = ( gusev_pkt_header * ) request;
char *buff = ( char * ) &request[HEAD_LEN];

	GetCompName (comp_name);
	head->len	= REQ_LEN_SET_OSCILL;
	head->magic = GUS_MAGIC;
	memcpy(head->ID,comp_name,sizeof( head->ID));
	head->SqN 	= 100;
	head->OpC	= 3;
	*( ( unsigned short * ) buff ) = n_o;
	buff += sizeof( unsigned short );
	buff[0] = n_p;
	buff[1] = param;
	return ClientTCPWrite (control_server, request, head->len, 1000);
}


