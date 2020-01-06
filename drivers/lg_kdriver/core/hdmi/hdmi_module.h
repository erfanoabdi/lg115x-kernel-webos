/*
	SIC LABORATORY, LG ELECTRONICS INC., SEOUL, KOREA
	Copyright(c) 2013 by LG Electronics Inc.

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	version 2 as published by the Free Software Foundation.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.
*/

/*! \file HDMI_module.h
 * \brief VPORT ADC module header
 */

#ifndef LX_HDMI_MODULE
#define LX_HDMI_MODULE

#include <linux/interrupt.h>

 /*
 	define
 */
#define HDMI_THREAD_TIMEOUT		20	//20ms
#define HDMI_READ_COUNT		10
#define HDMI_SCDT_COUNT		 4
#define HDMI_CHEAK_COUNT	 4

#define HDMI_VIDEO_INIT_STATE						0	///<  Video ISR Initialize or No SDCT State.
#define HDMI_VIDEO_INTERRUPT_STATE					1	///<  Video ISR Assert State.
#define HDMI_VIDEO_STABLE_STATE					    2	///<  Video ISR Stable State.
#define HDMI_VIDEO_READ_STATE					    4	///<  Video Read Stable State.

#define HDMI_AVI_INIT_STATE							0	///<  AVI ISR Initialize or No SDCT State.
#define HDMI_AVI_INTERRUPT_STATE					1	///<  AVI ISR Assert State.
#define HDMI_AVI_CHANGE_STATE						2	///<  AVI CSC Change State.
#define HDMI_AVI_STABLE_STATE					    3	///<  AVI ISR Stable State.

#define HDMI_VSI_NO_DATA_STATE						0	///<  No VSI  State.
#define HDMI_VSI_INIT_STATE							1	///<  VSI ISR Initialize or No SDCT State.
#define HDMI_VSI_INTERRUPT_STATE					2	///<  VSI ISR Assert State.
#define HDMI_VSI_STABLE_STATE					    4	///<  VSI ISR Stable State.

#define HDMI_AVI_CSC_CHANGE_STATE					0	///<  AVI CSC Change State.
#define HDMI_AVI_CSC_TEMP_STATE						2	///<  AVI CSC Temp State.
#define HDMI_AVI_CSC_STABLE_STATE					3	///<  AVI CSC Stable State.

#define HDMI_SOURCE_MUTE_CLEAR_STATE				0	///<  Source Clear State.
#define HDMI_SOURCE_MUTE_STATE						1	///<  Source Mute State.

// define audio state
#define HDMI_AUDIO_INIT_STATE						0	///<  Audio ISR Initialize or No SCDT State.
#define HDMI_AUDIO_INTERRUPT_STATE					1	///<  Audio ISR Assert State.
#define HDMI_AUDIO_GET_INFO_STATE				   14	///<  Audio ISR and Get Info. State.(count value)
#define HDMI_AUDIO_STABLE_STATE					   15	///<  Audio ISR and sampling frequency Stable State.(count value)
#define HDMI_AUDIO_RECHECK_TIME			   		   20	///<  1 second, DDI calls a every 20 ms.
#define HDMI_AUDIO_BURST_INFO_RECHECK_80MS 		   80	///<  80 mili-second to check next burst info interrupt.

#define DEBUG_HDMI_AUDIO_MSG_PRINT_TIME			  500	///<  1 second, DDI calls a every 20 ms.

#define HDMI_AUDIO_RECHECK_TIME_500MS 			(500/HDMI_THREAD_TIMEOUT)	///<  0.5 second, Thread calls a every 20 ms.
#define HDMI_AUDIO_FREQ_ERROR_TIME_500MS 		(500/HDMI_THREAD_TIMEOUT)	///<  0.5 second, Thread calls a every 20 ms.
#define HDMI_AUDIO_RECHECK_TIME_1S 				(1000/HDMI_THREAD_TIMEOUT)	///<  1.0 second, Thread calls a every 20 ms.
#define HDMI_AUDIO_PORT_STABLE_TIME_5S 			(5000/HDMI_THREAD_TIMEOUT)	///<  5.0 second, Thread calls a every 20 ms.
#define DEBUG_HDMI_AUDIO_MSG_PRINT_TIME_1S		(1000/HDMI_THREAD_TIMEOUT)	///<  1.0 second, Thread calls a every 20 ms.
#define DEBUG_HDMI_AUDIO_MSG_PRINT_TIME_10S		(10000/HDMI_THREAD_TIMEOUT)	///<  10 seconds, Thread calls a every 20 ms.
#define DEBUG_HDMI_AUDIO_MSG_PRINT_TIME_100S	(100000/HDMI_THREAD_TIMEOUT)///<  100 seconds, Thread calls a every 20 ms.

// define audio interrupt mask
#define HDMI_AUDIO_INTERRUPT_BIT_MASK	   0x0000E000	///<  intr_2npcm_chg, intr_2pcm_chg, intr_fs_chg
//#define HDMI_AUDIO_INTERRUPT_BIT_MASK	   0x1000E000	///<  intr_achst_5b_chg_int, intr_2npcm_chg, intr_2pcm_chg, intr_fs_chg

// define HDMI Audio Sample Word PCM & non-PCM sample
#define HDMI_AUDIO_SAMPLE_NON_PCM_MASK			0x02	///< 0 : linear PCM, 1 : other purpose

// define HDMI Audio Copyright Cp Bit Mask(BYTE0)
#define HDMI_AUDIO_CP_BIT_MASK					 0x04	///< 0 : copyright, 1 : no copyright

// define HDMI Audio Copyright L Bit Mask(BYTE1)
#define HDMI_AUDIO_L_BIT_MASK					 0x80	///< 0 : Home copy, 1 : Pre-recorded

// define HDMI Audio Sample Frequency according to IEC60958 channel status BYTE3(IEC60958-3 Third edition, 2006-05 spec.)
#define HDMI_AUDIO_SAMPLE_BIT_MASK				 0x0F	///< Bit 0 ~ 3
#define HDMI_AUDIO_SAMPLE_22_05KHZ				    4	///< 22.05 Kbps
#define HDMI_AUDIO_SAMPLE_24_KHZ					6	///< 24 Kbps
#define HDMI_AUDIO_SAMPLE_32_KHZ					3	///< 32 Kbps
#define HDMI_AUDIO_SAMPLE_44_1KHZ					0	///< 44.1 Kbps
#define HDMI_AUDIO_SAMPLE_48_KHZ					2	///< 48 Kbps
#define HDMI_AUDIO_SAMPLE_88_2KHZ					8	///< 88.2 Kbps
#define HDMI_AUDIO_SAMPLE_96_KHZ				   10	///< 96 Kbps
#define HDMI_AUDIO_SAMPLE_176_4KHZ				   12	///< 176.4 Kbps
#define HDMI_AUDIO_SAMPLE_192_KHZ				   14	///< 192 Kbps
#define HDMI_AUDIO_SAMPLE_768_KHZ					9	///< 768 Kbps

// Used for high bite rate transmission using IEC60958 protocol defined in 61883-6.
#define HDMI_AUDIO_EXT_SAMPLE_BIT_MASK			 0x3F	///< Bit 0 ~ 3, Bit 6~7
#define HDMI_AUDIO_EXT_SAMPLE_384_KHZ				5	///< 384 Kbps
#define HDMI_AUDIO_EXT_SAMPLE_1536_KHZ			   21	///< 1536 Kbps
#define HDMI_AUDIO_EXT_SAMPLE_1024_KHZ			   53	///< 1024 Kbps
#define HDMI_AUDIO_EXT_SAMPLE_3528_KHZ			   13	///< 3528 Kbps
#define HDMI_AUDIO_EXT_SAMPLE_7056_KHZ			   45	///< 7056 Kbps
#define HDMI_AUDIO_EXT_SAMPLE_14112_KHZ			   29	///< 14112 Kbps
#define HDMI_AUDIO_EXT_SAMPLE_64_KHZ			   11	///< 64 Kbps
#define HDMI_AUDIO_EXT_SAMPLE_128_KHZ			   43	///< 128 Kbps
#define HDMI_AUDIO_EXT_SAMPLE_256_KHZ			   27	///< 256 Kbps
#define HDMI_AUDIO_EXT_SAMPLE_512_KHZ			   59	///< 512 Kbps

// define HDMI Audio Data-Types according to IEC61937-2 Burst Info Preamble C(Pc)(IEC61937-2 First edition, 2004-03 spec.)
#define  BURST_INFO_AUDIO_TYPE_BIT_MASK		   0x001F	///< Bit 4 - 0
#define  BURST_INFO_AUDIO_TYPE_NULL					0	///<Null Data
#define  BURST_INFO_AUDIO_TYPE_AC3					1	///<AC-3 data
#define  BURST_INFO_AUDIO_TYPE_SMPTE_338M			2	///<Refer to SMPTE 338M
#define  BURST_INFO_AUDIO_TYPE_PAUSE				3	///<Pause
#define  BURST_INFO_AUDIO_TYPE_MPEG1_L1				4	///<MPEG-1 layer-1 data
#define  BURST_INFO_AUDIO_TYPE_MPEG1_L23			5	///<MPEG-1 layer-2 or -3 data or MPEG-2 without extension
#define  BURST_INFO_AUDIO_TYPE_MPEG2_EXT			6	///<MPEG-2 data with extension
#define  BURST_INFO_AUDIO_TYPE_MPEG2_AAC			7	///<MPEG-2 AAC
#define  BURST_INFO_AUDIO_TYPE_MPEG2_L1				8	///<MPEG-2, layer-1 low sampling frequency
#define  BURST_INFO_AUDIO_TYPE_MPEG2_L2				9	///<MPEG-2, layer-2 low sampling frequency
#define  BURST_INFO_AUDIO_TYPE_MPEG2_L3			   10	///<MPEG-2, layer-3 low sampling frequency
#define  BURST_INFO_AUDIO_TYPE_DTS_I			   11	///<DTS type I
#define  BURST_INFO_AUDIO_TYPE_DTS_II			   12	///<DTS type II
#define  BURST_INFO_AUDIO_TYPE_DTS_III			   13	///<DTS type III
#define  BURST_INFO_AUDIO_TYPE_ATRAC			   14	///<ATRAC
#define  BURST_INFO_AUDIO_TYPE_ATRAC_23			   15	///<ATRAC 2/3
#define  BURST_INFO_AUDIO_TYPE_ATRAC_X			   16	///<ATRAC-X
#define  BURST_INFO_AUDIO_TYPE_DTS_IV			   17	///<DTS type IV
#define  BURST_INFO_AUDIO_TYPE_WMA_I_IV			   18	///<WMA type I ~ IV
#define  BURST_INFO_AUDIO_TYPE_MPEG2_AAC_LOW	   19	///<MPEG-2 AAC low sampling frequency
#define  BURST_INFO_AUDIO_TYPE_MPEG4_AAC	       20	///<MPEG-4 AAC
#define  BURST_INFO_AUDIO_TYPE_AC3_ENHANCED		   21	///<AC-3 Enhanced
#define  BURST_INFO_AUDIO_TYPE_MAT				   22	///<MAT
														///<23-26 Reserved
														///<27-30 Refer to SMPTE 338M
														///<31 Extended data-type

#define  BURST_INFO_PAYLOAD_ERROR_BIT_MASK	   0x0080	///<Error-flag indicationg that the burst-payload may contain errors
#define  BURST_INFO_DEPENDENT_INFO_BIT_MASK	   0x1F00	///<Data-type-dependent info.
#define  BURST_INFO_STREAM_NUMBER_BIT_MASK	   0xD000	///<Bitstream number

#define HDMI_DEEP_COLOR_8BIT						0	///<  Deep Color 8bit
#define HDMI_DEEP_COLOR_10BIT						1	///<  Deep Color 8bit
#define HDMI_DEEP_COLOR_12BIT						2	///<  Deep Color 8bit

/*
	Device inform
*/


/*
	Enum define
*/
typedef enum{
	HDMI_PHY0_PORT		= 0,
	HDMI_PHY1_PORT		= 1,
	HDMI_PHY2_PORT		= 2,
	HDMI_PHY3_PORT		= 3,
	HDMI_ALL_PHY_PORT	= 4,
} HDMI_PHY_TYPE_T;

/*
	Enum define for connection
*/
typedef enum{
	HDMI_PORT_NOT_CONNECTED,
	HDMI_PORT_CONNECTED,
	HDMI_PORT_CONNECT_MAX,
} HDMI_PORT_CONNECTION_TYPE_T;

typedef struct EXT_TIMING_ENUM {
	UINT16 hAct_info;		///< Horizontal active pixel
	UINT16 vAct_info; 		///< Vertical active lines
	UINT16 scan_info;		///< Scan type (0 : interlace, 1 : progressive)

	UINT16 hAct_buf;			///< Horizontal active pixel
	UINT16 vAct_buf; 		///< Vertical active lines
	UINT16 scan_buf; 		///< Scan type (0 : interlace, 1 : progressive)
	LX_HDMI_EXT_FORMAT_INFO_T  extInfo_buf;		///< Ext format Information (3D, 4Kx2K)
} sEXT_TIMING_ENUM;

typedef struct O_ENUM {
	int 	PreEnum;
	int	PostEnum;
} sO_ENUM;

/*
	Enum define for HDCP Encryption Status
*/
typedef enum{
	HDMI_HDCP_NOT_ENCRYPTED,
	HDMI_HDCP_ENCRYPTED,
	HDMI_HDCP_AUTH_DONE,
	HDMI_HDCP_STATUS_MAX,
} HDMI_HDCP_STATUS_T;

/*
	Struct for phy workaround and test
	*/
typedef struct hdmi_phy_control {
	int all_port_pdb_enable;		///< all phy pdb on for faster port switching
	int link_reset_control;			///< link reset workaround for fast HDCP Authenti
	int check_an_data;				///< check AN data valid before port change
	int port_change_start;			///< port change sequence started
	int port_to_change;				///< port number to change
	UINT8	AN_Data_Prt0[8];		///< AN Data Port 0
	UINT8	AN_Data_Prt1[8];		///< AN Data Port 1
	UINT8	AN_Data_Prt2[8];		///< AN Data Port 2
	UINT8	AN_Data_Prt3[8];		///< AN Data Port 3

} HDMI_PHY_CONTROL_T;
/*
	global variable
*/


/*
	function prototype
*/



// function pointer
int HDMI_Initialize(LX_HDMI_INIT_T *param);
int HDMI_SetPort(UINT32 *port);
int HDMI_GetMode(LX_HDMI_MODE_T *mode);
int HDMI_GetAspectRatio(LX_HDMI_ASPECTRATIO_T *ratio);

int HDMI_GetColorDomain(LX_HDMI_COLOR_DOMAIN_T *color);
int HDMI_GetTimingInfo(LX_HDMI_TIMING_INFO_T *info);
int HDMI_GetAviPacket(LX_HDMI_INFO_PACKET_T *packet);
int HDMI_GetVsiPacket(LX_HDMI_INFO_PACKET_T *packet);
int HDMI_GetSpdPacket(LX_HDMI_INFO_PACKET_T *packet);
int HDMI_GetInfoPacket(LX_HDMI_INFO_PACKET_T *packet);

int HDMI_GetStatus(LX_HDMI_STATUS_T *status);
int HDMI_SetHPD(LX_HDMI_HPD_T *hpd);

int HDMI_PowerConsumption(UINT32 power);

//audio related
int HDMI_GetAudioInfo(LX_HDMI_AUDIO_INFO_T *pAudioInfo);
int HDMI_GetAudioCopyInfo(LX_HDMI_AUDIO_COPY_T *pCopyInfo);
int HDMI_SetArc(LX_HDMI_ARC_CTRL_T *pArcCtrl);
int HDMI_SetMute(LX_HDMI_MUTE_CTRL_T *pMuteCtrl);
int HDMI_GetAudioDebugInfo(LX_HDMI_DEBUG_AUDIO_INFO_T *pAudioDebugInfo);

int HDMI_GetRegister(UINT32 addr , UINT32 *value);
int HDMI_SetRegister(UINT32 addr , UINT32 value);

void HDMI_Periodic_Task(void);
int HDMI_DisableInterrupt(void);

#ifdef	KDRV_CONFIG_PM
int HDMI_RunSuspend(void);
int HDMI_RunResume(void);
#endif

// from hdmi_module.c
int HDMI_exit(void);

int HDMI_GetI2CData(unsigned int sub , unsigned int addr , unsigned int size );
int HDMI_SetI2CData(unsigned int sub , unsigned int addr , unsigned int size , unsigned int value);

int HDMI_Read_EDID_Data(int port_num , UINT8 *pedid_data);
int HDMI_Write_EDID_Data(int port_num , UINT8 *pedid_data);
int HDMI_Get_Phy_Status(LX_HDMI_PHY_INFORM_T *sp_hdmi_phy_status);
int HDMI_Read_HDCP_Key(UINT8 *phdcp_key);
int HDMI_Write_HDCP_Key(UINT8 *phdcp_key);

int HDMI_Thread_Control(int sleep_enable);
int HDMI_Get_Aksv_Data(int port_num, UINT8 *pAksv_Data);
int HDMI_MHL_Send_RCP(UINT8 rcp_data);
int HDMI_MHL_Send_WriteBurst(LX_HDMI_MHL_WRITEBURST_DATA_T *spWriteburst_data);
int HDMI_MHL_Read_WriteBurst(LX_HDMI_MHL_WRITEBURST_DATA_T *spWriteburst_data);
int HDMI_Module_Call_Type(LX_HDMI_CALL_TYPE_T	hdmi_call_type);
int HDMI_Reset_Internal_Edid(int port_num, int edid_resetn);
int HDMI_Enable_External_DDC_Access(int port_num, int enable);
int HDMI_MHL_Receive_RCP(LX_HDMI_RCP_RECEIVE_T *sp_MHL_RCP_rcv_msg);
int HDMI_Get_Driver_Status(LX_HDMI_DRIVER_STATUS_T *sp_hdmi_driver_status);
#endif
