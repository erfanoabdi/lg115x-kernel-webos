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



/** @file
 *
 * main driver implementation for HDMI device.
 * HDMI device will teach you how to make device driver with new platform.
 *
 * author     sunghyun.myoung (sh.myoung@lge.com)
 * version    1.0
 * date       2010.02.19
 * note       Additional information.
 *
 * @addtogroup lg115x_hdmi
 * @{
 */

/*----------------------------------------------------------------------------------------
 *   File Inclusions
 *---------------------------------------------------------------------------------------*/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>	/**< printk() */
#include <linux/slab.h> 	/**< kmalloc() */
#include <linux/fs.h> 		/**< everything\ldots{} */
#include <linux/types.h> 	/**< size_t */
#include <linux/fcntl.h>	/**< O_ACCMODE */
#include <asm/uaccess.h>
#include <linux/ioport.h>	/**< For request_region, check_region etc */
#include <asm/io.h>			/**< For ioremap_nocache */
#include <linux/workqueue.h>/**< For working queue */
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <asm/div64.h> 		//do_div
#include <linux/kthread.h>
#include <linux/wait.h>		/**< Wait queue */
#include <linux/semaphore.h>

#include "os_util.h"
#include "hdmi_hw_m14b0.h"
#include "hdmi_reg_m14.h"
#include "sys_regs.h"

//#include "hdmi_hdcp_key_m14.h"
//#include "hdmi_edid_data_m14.h"

/******************************************************************************
 *				DEFINES
 *****************************************************************************/
#define u16CHK_RANGE(X,Y,offset)		((UINT16) ((X) + (offset) - (Y)) <= (offset*2))
#define TBL_NUM(X)						(sizeof(X) /sizeof((X)[0]))

#define CHK_UHD_BD()	(lx_chip_plt() != LX_CHIP_PLT_FHD)
#define CHK_FHD_BD()	(lx_chip_plt() == LX_CHIP_PLT_FHD)

//#define M14B0_MHL_CONNECTION_BUG_WORKAROUND
#define M14_DISABLE_AUDIO_INTERRUPT

#define M14_THREAD_READ_PHY_REG_VALUE

#define FOR_EDISON_XBOX360

#define M14_CODE_FOR_MHL_CTS
#define M14_CBUS_PDB_CTRL
//#define M14_HUMAX_SETTOP_HDCP_WORKAROUND
//#define M14_HDMI_SEMI_AUTO_EQ_CONTROL
#define ODT_PDB_OFF_ON_WORKAROUND	//for Dvico player re-read EDID data ( Some Dvico player(TVIX M-6510A) reads EDID only when TMDS termination is on)
//#define HPD_OFF_WORKAROUND_FOR_EXT_EDID	// long HPD off time for LTE2 and MHL to HDMI gender
//#define HPD_ON_OFF_CONTROL_WORKAROUND	// for Indian settop box dishtv ( no AVI packet )
#define HDMI_MODE_WHEN_NO_AVI_WORKAROUND	// for Indian settop box dishtv ( no AVI packet )
/**
 *	Global variables of the driver
 */
extern OS_SEM_T	g_HDMI_Sema;
extern LX_HDMI_REG_T *pM14B0HdmiRegCfg;
extern HDMI_DATA_T *gM14BootData;

static HDMI_PORT_CONNECTION_TYPE_T _gM14B0HDMIConnectState = HDMI_PORT_NOT_CONNECTED;
static LX_HDMI_TIMING_INFO_T _gM14B0PrevTiming = {0,};
static UINT32 _gM14B0TimingReadCnt = 0;
static UINT32 _gM14B0HDMIState = HDMI_STATE_DISABLE;

static LX_HDMI_AVI_COLORSPACE_T	_gM14B0PrevPixelEncoding = LX_HDMI_AVI_COLORSPACE_RGB;
static LX_HDMI_INFO_PACKET_T _gM14B0PrevAVIPacket = {0, };
static LX_HDMI_INFO_PACKET_T _gM14B0PrevVSIPacket = {0, };
static BOOLEAN _gM14B0AVIReadState 	  = FALSE;
static BOOLEAN _gM14B0VSIState 	  = FALSE;
static BOOLEAN _gM14B0AVIChangeState 	  = FALSE;
static BOOLEAN _gM14B0PortSelected 	  = FALSE;
static UINT32 _gM14B0MHLContentOff = 0;	// MHL RAP support(if content off is '1' set timing info to '0' to enable AV mute.

//audio
static BOOLEAN _gM14B0AudioMuteState 	  = FALSE;
static BOOLEAN _gM14B0AudioArcStatus 	  = FALSE;	//This variable is used ARC Enabled Status to recover ARC mode after PHY reset.
static UINT32  _gM14B0IntrAudioState 	  = HDMI_AUDIO_INIT_STATE;
static UINT32  _gM14B0HdmiPortStableCount  = 0;	//This count is used to detect if HDMI Switch port is changed.
static UINT32  _gM14B0HdmiFreqErrorCount   = 0;	//This count is used to detect if HDMI TMDS Frequency and Frequency is different.
static UINT32  _gM14B0HdmiAudioThreadCount = 0;	//_gM14B0HdmiAudioThreadCount is increased in _HDMI_M14B0_Periodic_Audio_Task function
static UINT64  _gM14B0IntrBurstInfoCount   = 0;	//This count is used to detect if burst info. interrupt is asserted count.
static UINT64  _gM14B0TaskBurstInfoCount   = 0;	//This count is used to save _gM14B0IntrBurstInfoCount in hdmi task.
static UINT64  _gM14B0BurstInfoPrevJiffies = 0;	//This Jiffies is used to save previous burst info. interrupt jiffies.
static LX_HDMI_AUDIO_INFO_T _gM14B0HdmiAudioInfo = {LX_HDMI_AUDIO_DEFAULT, LX_HDMI_SAMPLING_FREQ_NONE};

//interrupt variables
static UINT32 _gM14B0Intr_vf_stable = HDMI_VIDEO_INIT_STATE;
static UINT32 _gM14B0Intr_avi = HDMI_AVI_INIT_STATE;
static UINT32 _gM14B0Intr_src_mute = HDMI_SOURCE_MUTE_CLEAR_STATE;
static UINT32 _gM14B0Intr_vsi = HDMI_VSI_INIT_STATE;

static UINT32 _gM14B0Force_thread_sleep  = 0;	//enable  = 2, disable  =0
static UINT32 _gM14B0HDMI_thread_running = 0;	//enable  = 1, disable  =0
static UINT32 _gM14B0HDMI_interrupt_registered = 0;

static UINT32 _gM14B0ChipPlatform = 0;

//static int _gPathen_set_count = 0;
static int _gM14B0CHG_AVI_count_MHL = -1;
static int _gM14B0CHG_AVI_count_EQ = -1;
static int _gM14B0_TMDS_ERROR_EQ = -1;
static int _gM14B0_TMDS_ERROR_EQ_2nd = -1;
static int _gM14B0_TMDS_ERROR_intr_count = 0;
static int _gM14B0_TMDS_ERROR_count = 0;

static int _HDMI_M14B0_ResetPortControl(int reset_enable);
static int _HDMI_M14B0_TMDS_ResetPort(int port_num);
static int _HDMI_M14B0_HDCP_ResetPort(int port_num);
#ifdef NOT_USED_NOW
static int _HDMI_M14B0_TMDS_HDCP_ResetPort(int port_num);
static int _HDMI_M14B0_TMDS_HDCP_ResetPort_Control(int port_num, int reset);
#endif	//#ifdef NOT_USED_NOW
static int _HDMI_M14B0_TMDS_ResetPort_Control(int port_num, int reset);
static int _HDMI_M14B0_HDCP_ResetPort_Control(int port_num, int reset);
static int _HDMI_M14B0_SetPortConnection(void);
static int _HDMI_M14B0_EnablePort(int prt_num, int enable);
static int _HDMI_M14B0_Get_ManMHLMode(void);
static int _HDMI_M14B0_Set_ManMHLMode(int man_mhl_mode, int man_mhl_value);
static int _HDMI_M14B0_Disable_TMDS_Error_Interrupt(void);
static int _HDMI_M14B0_Enable_TMDS_Error_Interrupt(void);
#ifdef NOT_USED_NOW
static int _HDMI_M14B0_Set_HDCP_Unauth(int port_num, int unauth_nosync, int unauth_mode_chg);
#endif	//#ifdef NOT_USED_NOW
static int _HDMI_M14B0_Check_RAP_Content(int clear);
static int _HDMI_M14B0_MHL_Check_Status(int init);
static int _HDMI_M14B0_HDCP_Check_Status(void);

#ifdef ODT_PDB_OFF_ON_WORKAROUND
static int _HDMI_M14B0_Phy_Enable_Register_Access(int port_num);
static int _HDMI_M14B0_EQ_PDB_control(int reset);
#endif

static	UINT8 HDCP_AN_Data_Zero[8] ={0,} ;
static	UINT8 HDCP_AKSV_Data_Zero[5] ={0,} ;
static LX_HDMI_PHY_INFORM_T _gM14B0HDMIPhyInform = {0,};

static HDMI_PHY_CONTROL_T	gHDMI_Phy_Control;
static unsigned int _Port_Change_StartTime = 0;
static int _SCDT_Fall_Detected[4] = {0,};
static unsigned int _TCS_Done_Time[4] = {0,};
static LX_HDMI_RCP_RECEIVE_T _gMHL_RCP_RCV_MSG;
static unsigned int _HDMI_Check_State_StartTime = 0;
static unsigned int _HDMI_Check_State_ElaspedTime = 0;

//wait_queue_head_t	WaitQueue_HDMI;
static DECLARE_WAIT_QUEUE_HEAD(WaitQueue_HDMI_M14B0);
static DECLARE_WAIT_QUEUE_HEAD(WaitQueue_MHL_M14B0);
static DECLARE_WAIT_QUEUE_HEAD(WaitQueue_MHL_PDB_M14B0);

static DEFINE_SPINLOCK(_gIntrHdmiM14B0VideoLock);
#ifdef NOT_USED_NOW
static DEFINE_SPINLOCK(_gHdmiM14B0RegReadLock);
#endif	//#ifdef NOT_USED_NOW

//HDMI Audio Variables
static DEFINE_SPINLOCK(_gIntrHdmiM14B0AudioLock);

static const sEXT_TIMING_ENUM TBL_EXT_INFO[ ] =
{	/// hAct_info	vAct_info	scan_info	hAct_buf	vAct_buf	scan_buf	extInfo_buf
	{	1280,	1470,	1,		1280, 	720, 	1,		LX_HDMI_EXT_3D_FRAMEPACK }, 		//720p FP
	{	1920,	2205,	1,		1920, 	1080, 	1,		LX_HDMI_EXT_3D_FRAMEPACK },		//1080p FP
	{	1920,	2228,	1,		1920, 	1080, 	0,		LX_HDMI_EXT_3D_FRAMEPACK },		//1080i FP

	{	2560,	720,	1,		1280, 	720, 	1,		LX_HDMI_EXT_3D_SBSFULL }, 			//720p SSF
	{	3840,	1080,	1,		1920, 	1080, 	1,		LX_HDMI_EXT_3D_SBSFULL },			//1080p SSF
	{	3840,	1080,	0,		1920, 	1080, 	0,		LX_HDMI_EXT_3D_SBSFULL },			//1080i SSF

	{	1280,	1440,	1,		1280, 	720, 	1,		LX_HDMI_EXT_3D_LINE_ALTERNATIVE }, 	//720p LA
	{	1920,	2160,	1,		1920, 	1080, 	1,		LX_HDMI_EXT_3D_LINE_ALTERNATIVE },	//1080p LA

	{	1920,	1103,	1,		1920, 	1080, 	0,		LX_HDMI_EXT_3D_FIELD_ALTERNATIVE },	//1080i FA
};
static const sO_ENUM EXT_3D_TYPE[ ] =
{	/// LX_HDMI_VSI_3D_STRUCTURE_T 						LX_HDMI_EXT_FORMAT_INFO_T
	{	LX_HDMI_VSI_3D_STRUCTURE_FRAME_PACKING,			LX_HDMI_EXT_3D_FRAMEPACK},
	{	LX_HDMI_VSI_3D_STRUCTURE_FIELD_ALTERNATIVE,		LX_HDMI_EXT_3D_FIELD_ALTERNATIVE},
	{	LX_HDMI_VSI_3D_STRUCTURE_LINE_ALTERNATIVE,		LX_HDMI_EXT_3D_LINE_ALTERNATIVE},
	{	LX_HDMI_VSI_3D_STRUCTURE_SIDEBYSIDE_FULL,		LX_HDMI_EXT_3D_SBSFULL},
	{	LX_HDMI_VSI_3D_STRUCTURE_L_DEPTH,				LX_HDMI_EXT_3D_L_DEPTH},
	{	LX_HDMI_VSI_3D_STRUCTURE_L_DEPTH_GRAPHICS,		LX_HDMI_EXT_3D_L_GRAPHICS},
	{	LX_HDMI_VSI_3D_STRUCTURE_TOP_BOTTOM,			LX_HDMI_EXT_3D_TNB},
	{	LX_HDMI_VSI_3D_STRUCTURE_SIDEBYSIDE_HALF,		LX_HDMI_EXT_3D_SBS},
	{	LX_HDMI_VSI_3D_STRUCTURE_MAX,					LX_HDMI_EXT_2D_FORMAT},
};

static int	_gM14B0_HDMI_MHLAutoModeCnt = 0;
static int _gHdmi_no_connection_count = 0;
static unsigned int _g_ODT_PDB_Off_Time;
static unsigned int _g_ODT_PDB_On_Time;

#ifdef HPD_OFF_WORKAROUND_FOR_EXT_EDID
static unsigned int _g_HPD_Off_Time[4] = {0,};
static unsigned int _g_HPD_On_Time[4] = {0,};
static unsigned int _g_HPD_Elasped_Time[4] = {0,};
#endif

static int _g_rcp_send_buffer = 2;

static int _g_abnormal_3d_format = 0;

static int _g_tcs_min_max_zero_count[4] = {0,};

static UINT32	_Prev_TMDS_Clock[4] = {0,};
static UINT32	_Curr_TMDS_Clock[4] = {0,};
/******************************************************************************
 *				DATA STRUCTURES
 *****************************************************************************/
/**
 *	Structure that declares the usual file
 *	access functions
 */
static struct task_struct	*stHDMI_Thread;
static struct task_struct	*stMHL_Reconnect_Thread;
static struct task_struct	*stMHL_PDB_Thread;

PHY_REG_CTRL_M14B0_T  gPHY_REG_CTRL_M14B0;
static volatile HDMI_LINK_REG_M14B0_T *pstLinkReg;
static volatile HDMI_LINK_REG_M14B0_T *pstLinkShadowReg;


static volatile unsigned char *pHDCP_Key_addr;
static volatile unsigned char *pEDID_data_addr_port0;
static volatile unsigned char *pEDID_data_addr_port1;
static volatile unsigned char *pEDID_data_addr_port2;
static volatile unsigned char *pEDID_data_addr_port3;
/******************************************************************************
 *				Local function
 *****************************************************************************/
static int _HDMI_M14B0_GetExtendTimingInfo(LX_HDMI_TIMING_INFO_T *info);
static int _HDMI_M14B0_ClearSWResetAll(void);			//M14B0D Chip - Ctop control
static int _HDMI_M14B0_PhyRunReset(void);
static int _HDMI_M14B0_RunReset(void);
static int _HDMI_M14B0_GetMHLConection(void);
static int _HDMI_M14B0_GetPortConnection(void);
static int _HDMI_M14B0_SetInternalMute(LX_HDMI_MUTE_CTRL_T interMute);
static int _HDMI_M14B0_SetVideoBlank(LX_HDMI_AVI_COLORSPACE_T space);
static int _HDMI_M14B0_EnableInterrupt(void);
static int _HDMI_M14B0_Thread(void *data);
irqreturn_t _HDMI_M14B0_IRQHandler(int irq, void *dev);
static void _HDMI_M14B0_Periodic_Task(void);

//audio related.
static int _HDMI_M14B0_SetAudio(void);
static int _HDMI_M14B0_GetAudioInfo(void);
static int _HDMI_M14B0_GetAudioTypeAndFreq(LX_HDMI_AUDIO_TYPE_T *audioType, LX_HDMI_SAMPLING_FREQ_T *samplingFreq);
static int _HDMI_M14B0_GetAudioFreqFromTmdsClock(LX_HDMI_SAMPLING_FREQ_T *samplingFreqFromTmds);
static int _HDMI_M14B0_DebugAudioInfo(LX_HDMI_DEBUG_AUDIO_INFO_T *pAudioDebugInfo);

static int _HDMI_M14B0_PhyOn(int port_num);
static int _HDMI_M14B0_PhyOn_5V(int port_num);
static int _HDMI_M14B0_PhyOff(int port_num);
#ifdef NOT_USED_NOW
static int _HDMI_M14B0_ResetPort(int port_num);
#endif	//#ifdef NOT_USED_NOW
static int _HDMI_M14B0_Get_EDID_Rd(int port_num , int *edid_rd_done, int *edid_rd_cnt);
static int _HDMI_M14B0_Get_HDCP_info(int port_num , UINT32 *hdcp_authed, UINT32 *hdcp_enc_en);
#ifdef NOT_USED_NOW
static int _HDMI_M14B0_Write_EDID_Data_A4P(UINT32 *pedid_data);
#endif	//#ifdef NOT_USED_NOW
static int _HDMI_M14B0_Get_HDMI5V_Info_A4P(int *pHDMI5V_Status_PRT0, int *pHDMI5V_Status_PRT1 ,int *pHDMI5V_Status_PRT2, int *pHDMI5V_Status_PRT3);
static int _HDMI_M14B0_Get_HDMI5V_Info(int port_num, int *pHDMI5V_Status);
static int _HDMI_M14B0_Set_HPD_Out(int prt_sel, int hpd_out_value);
static int _HDMI_M14B0_Set_HPD_Out_A4P(int HPD_Out_PRT0, int HPD_Out_PRT1 ,int HPD_Out_PRT2, int HPD_Out_PRT3);
static int _HDMI_M14B0_Get_HPD_Out_A4P(int *pHPD_Out_PRT0, int *pHPD_Out_PRT1 ,int *pHPD_Out_PRT2, int *pHPD_Out_PRT3);

static int _HDMI_M14B0_Get_AN_Data(int port_num, UINT8 *AN_Data);
static int _HDMI_M14B0_MHL_Reconnect_Thread(void *data);
static int _HDMI_M14B0_MHL_PDB_Thread(void *data);
static int _HDMI_M14B0_Phy_Reset(int port_num);
static int _HDMI_M14B0_Check_Aksv_Exist(int port_num);
static int _HDMI_M14B0_swrst_TMDS_sel_control(int reset);

int HDMI_M14B0_HWInitial(void)
{
	gPHY_REG_CTRL_M14B0.shdw.addr = (UINT32 *)OS_KMalloc(sizeof(PHY_REG_M14B0_T));

	gHDMI_Phy_Control.all_port_pdb_enable = 1;		// should not be used due to power issue
	gHDMI_Phy_Control.link_reset_control = 0;
	gHDMI_Phy_Control.check_an_data = 0;

#ifndef	USE_QEMU_SYSTEM
	pstLinkReg =(HDMI_LINK_REG_M14B0_T *)ioremap(pM14B0HdmiRegCfg->link_reg_base_addr, pM14B0HdmiRegCfg->link_reg_size);
#else
	pstLinkReg =(HDMI_LINK_REG_M14B0_T *)ioremap(pM14B0HdmiRegCfg->link_qemu_base_addr, pM14B0HdmiRegCfg->link_reg_size);
#endif

	HDMI_PRINT("HDMI LINK REG CFG \n");
	HDMI_PRINT("LINK REG Base Address	0x%08x\n", pM14B0HdmiRegCfg->link_reg_base_addr);
	HDMI_PRINT("LINK REG Size 			0x%08x\n", pM14B0HdmiRegCfg->link_reg_size);
	HDMI_PRINT("LINK REG Qumu Address 	0x%08x\n", pM14B0HdmiRegCfg->link_qemu_base_addr);

	if (pstLinkReg == NULL)
	{
		HDMI_ERROR("ERROR : can't allocate for register\n");
		return RET_ERROR;
	}

	pstLinkShadowReg = (HDMI_LINK_REG_M14B0_T *)kmalloc(sizeof(HDMI_LINK_REG_M14B0_T), GFP_KERNEL);
	if (pstLinkShadowReg == NULL)
	{
		HDMI_ERROR("ERROR : can't allocate for shadow register\n");
		return RET_ERROR;
	}
	_gM14B0HDMIPhyInform.phy_status = HDMI_PHY_STATUS_DISCONNECTED;

	_gM14B0HDMIPhyInform.hpd_pol[0] = 1;
	_gM14B0HDMIPhyInform.hpd_pol[1] = 1;
	_gM14B0HDMIPhyInform.hpd_pol[2] = 1;
	_gM14B0HDMIPhyInform.hpd_pol[3] = 1;

	 pHDCP_Key_addr = (volatile unsigned char *)&(pstLinkReg->hdcp_key_00);
	 pEDID_data_addr_port0 = (volatile unsigned char *)&(pstLinkReg->edid_00);
	 pEDID_data_addr_port1 = (volatile unsigned char *)&(pstLinkReg->edid_64);
	 pEDID_data_addr_port2 = (volatile unsigned char *)&(pstLinkReg->edid_128);
	 pEDID_data_addr_port3 = (volatile unsigned char *)&(pstLinkReg->edid_192);

	_gM14B0ChipPlatform = lx_chip_plt() + 0x4;	
	//140514 : MHL CT 
	//140430 for driver version

	stHDMI_Thread = kthread_create(_HDMI_M14B0_Thread, (void*)NULL, "hdmi_thread");
	if (stHDMI_Thread)
	{
		wake_up_process(stHDMI_Thread);
		_gM14B0HDMI_thread_running = 1;
		HDMI_DEBUG("HDMI Thread [%d]\n", stHDMI_Thread->pid);
	}
	else
	{
		HDMI_ERROR("HDMI Thread Already Created\n");
	}

	stMHL_Reconnect_Thread = kthread_create(_HDMI_M14B0_MHL_Reconnect_Thread, (void*)NULL, "mhl_reconnect_thread");
	if (stMHL_Reconnect_Thread)
	{
		wake_up_process(stMHL_Reconnect_Thread);
		HDMI_DEBUG("MHL Reconnect Thread [%d]\n", stMHL_Reconnect_Thread->pid);
	}
	else
	{
		HDMI_ERROR("MHL Reconnect Thread Already Created\n");
	}

	stMHL_PDB_Thread = kthread_create(_HDMI_M14B0_MHL_PDB_Thread, (void*)NULL, "mhl_pdb_thread");
	if (stMHL_PDB_Thread)
	{
		wake_up_process(stMHL_PDB_Thread);
		HDMI_DEBUG("MHL PDB Thread [%d]\n", stMHL_PDB_Thread->pid);
	}
	else
	{
		HDMI_ERROR("MHL Reconnect Thread Already Created\n");
	}

//	HDMI_M14B0_SetPowerControl(0);

	HDMI_TRACE("M14B0 HDMI Initialize \n" );
	return RET_OK;
}

int HDMI_M14B0_SetPort(UINT32 *port)
{
	int ret = RET_OK;
	int port_selected;
//	int loop, port_authed, port_enc_en;
	int hdmi5v_status = 0;
	int prev_port_an_data_zero = -1;
	ULONG	flags = 0;

	LINK_M14_RdFL(system_control_00);
	LINK_M14_Rd01(system_control_00, reg_prt_sel, port_selected);

	//spin lock for protection : lock
	spin_lock_irqsave(&_gIntrHdmiM14B0AudioLock, flags);

	//Clears a global value for audio info.
	_gM14B0HdmiAudioInfo.audioType	  = LX_HDMI_AUDIO_NO_AUDIO;
	_gM14B0HdmiAudioInfo.samplingFreq = LX_HDMI_SAMPLING_FREQ_NONE;

	//Clear _gM14B0IntrAudioState
	_gM14B0IntrAudioState = HDMI_AUDIO_INIT_STATE;

	//Reset _gM14B0IntrBurstInfoCount
	_gM14B0IntrBurstInfoCount = 0;

	//Clear _gM14B0HdmiFreqErrorCount
	_gM14B0HdmiFreqErrorCount = 0;

	_gM14B0HDMIState = HDMI_STATE_IDLE;

	if( port_selected != *port)
	{
		_HDMI_M14B0_swrst_TMDS_sel_control(1);
		_gM14B0Intr_vf_stable = HDMI_VIDEO_INIT_STATE;
		_gM14B0Intr_avi = HDMI_AVI_INIT_STATE;
		_gM14B0Intr_vsi = HDMI_VSI_INIT_STATE;
		_gM14B0Intr_src_mute = HDMI_SOURCE_MUTE_CLEAR_STATE;
	}
	_gM14B0TimingReadCnt = 0;
	_gM14B0AVIReadState = FALSE;
	_gM14B0VSIState = FALSE;
	_gM14B0AVIChangeState = FALSE;

	//spin lock for protection : unlock
	spin_unlock_irqrestore(&_gIntrHdmiM14B0AudioLock, flags);

	OS_LockMutex(&g_HDMI_Sema);

	//added 131127 : force timing info to zero when port change


	do {
		if (*port > HDMI_PHY3_PORT)	{ret = RET_ERROR;	break;}

		_Port_Change_StartTime = jiffies_to_msecs(jiffies);
		//_gM14B0CHG_AVI_count_EQ = 0;
		_gM14B0_TMDS_ERROR_EQ = 0;
		_gM14B0_TMDS_ERROR_EQ_2nd = -1;
//		_HDMI_M14B0_Enable_TMDS_Error_Interrupt();
		_TCS_Done_Time[*port] = jiffies_to_msecs(jiffies);


#ifdef M14_CODE_FOR_MHL_CTS
		_HDMI_M14B0_Check_RAP_Content(1);
#endif

		if (gHDMI_Phy_Control.check_an_data)
		{
			if (port_selected == 0)
				_HDMI_M14B0_Get_AN_Data(port_selected, gHDMI_Phy_Control.AN_Data_Prt0);
			else if (port_selected == 1)
				_HDMI_M14B0_Get_AN_Data(port_selected, gHDMI_Phy_Control.AN_Data_Prt1);
			else if (port_selected == 2)
				_HDMI_M14B0_Get_AN_Data(port_selected, gHDMI_Phy_Control.AN_Data_Prt2);
			else if (port_selected == 3)
				_HDMI_M14B0_Get_AN_Data(port_selected, gHDMI_Phy_Control.AN_Data_Prt3);

			if (*port == 0)
				prev_port_an_data_zero = memcmp(gHDMI_Phy_Control.AN_Data_Prt0, HDCP_AN_Data_Zero, sizeof(HDCP_AN_Data_Zero) );
			else if (*port == 1)
				prev_port_an_data_zero = memcmp(gHDMI_Phy_Control.AN_Data_Prt1, HDCP_AN_Data_Zero, sizeof(HDCP_AN_Data_Zero) );
			else if (*port == 2)
				prev_port_an_data_zero = memcmp(gHDMI_Phy_Control.AN_Data_Prt2, HDCP_AN_Data_Zero, sizeof(HDCP_AN_Data_Zero) );
			else if (*port == 3)
				prev_port_an_data_zero = memcmp(gHDMI_Phy_Control.AN_Data_Prt3, HDCP_AN_Data_Zero, sizeof(HDCP_AN_Data_Zero) );
		}

		if (gHDMI_Phy_Control.link_reset_control)
		{
			if (gHDMI_Phy_Control.check_an_data)
				_HDMI_M14B0_TMDS_ResetPort(port_selected);
			else
				_HDMI_M14B0_TMDS_ResetPort(port_selected);
		}
		if (!gHDMI_Phy_Control.all_port_pdb_enable)
		{
			if (*port != 0)
				_HDMI_M14B0_PhyOff(0);
			if (*port != 1)
				_HDMI_M14B0_PhyOff(1);
			if (*port != 2)
				_HDMI_M14B0_PhyOff(2);
			if (*port != 3)
				_HDMI_M14B0_PhyOff(3);
		}
#ifdef M14_HUMAX_SETTOP_HDCP_WORKAROUND
		/* HUMAX IR1020HD : snow noise when resolution change */
		/* Restore Default values for NOT selected port */
		if (*port != 0)
			_HDMI_M14B0_Set_HDCP_Unauth(0, 1, 1);
		if (*port != 1)
			_HDMI_M14B0_Set_HDCP_Unauth(1, 1, 1);
		if (*port != 2)
			_HDMI_M14B0_Set_HDCP_Unauth(2, 1, 1);
		if (*port != 3)
			_HDMI_M14B0_Set_HDCP_Unauth(3, 1, 1);
#endif

#ifdef M14_CODE_FOR_MHL_CTS
		if (_gM14B0HDMIPhyInform.cd_sense)
		{
#if 0
			if ( (port_selected == 3) && (*port != 3) )
			{
				LINK_M14_RdFL(cbus_05);
				LINK_M14_Wr01(cbus_05, reg_man_msc_wrt_stat_pathen_clr, 0x1);
				LINK_M14_WrFL(cbus_05);
				HDMI_DEBUG("---- MHL pathen clr : 0x1 \n");
			}
			else if ( (port_selected != 3) && (*port == 3) )
			{
				LINK_M14_RdFL(cbus_04);
				LINK_M14_Wr01(cbus_04, reg_man_msc_wrt_stat_pathen_set, 0x1);
				LINK_M14_WrFL(cbus_04);
				HDMI_DEBUG("---- MHL pathen set : 0x1 \n");
				_gPathen_set_count = 0;
			}
#endif
			if ( (port_selected == 3) || (*port == 3) )
			{
				LINK_M14_RdFL(cbus_04);
				LINK_M14_Wr01(cbus_04, reg_man_msc_wrt_stat_pathen_set, 0x1);
				LINK_M14_WrFL(cbus_04);
				HDMI_DEBUG("---- MHL pathen set : 0x1 \n");
				OS_MsecSleep(5);	// ms delay
				LINK_M14_RdFL(cbus_04);
				LINK_M14_Wr01(cbus_04, reg_man_msc_wrt_stat_pathen_set, 0x0);
				LINK_M14_WrFL(cbus_04);
				HDMI_DEBUG("---- MHL pathen set : 0x0 \n");
			}
		}
#endif

		LINK_M14_RdFL(system_control_00);
		LINK_M14_Wr01(system_control_00, reg_prt_sel, *port);
		LINK_M14_WrFL(system_control_00);

		_gM14B0PortSelected = TRUE;

		// Added for CHG VSI VFORMAT interrupt to occure
		LINK_M14_RdFL(packet_00);
		LINK_M14_Wr01(packet_00, reg_pkt_clr, 1);
		LINK_M14_WrFL(packet_00);

		OS_MsecSleep(5);	// ms delay

		LINK_M14_RdFL(packet_00);
		LINK_M14_Wr01(packet_00, reg_pkt_clr, 0);
		LINK_M14_WrFL(packet_00);

		_HDMI_M14B0_Get_HDMI5V_Info(*port, &hdmi5v_status);

		if (gHDMI_Phy_Control.check_an_data && hdmi5v_status && (prev_port_an_data_zero) )
		{
			HDMI_DEBUG("Start Port Change to [%d] checking AN Data\n", *port);
			spin_lock_irqsave(&_gIntrHdmiM14B0VideoLock, flags);
			gHDMI_Phy_Control.port_to_change = *port;
			gHDMI_Phy_Control.port_change_start = 1;
			spin_unlock_irqrestore(&_gIntrHdmiM14B0VideoLock, flags);
#if 0
			for (loop=0;loop<100;loop++)
			{
				_HDMI_M14B0_Get_AN_Data(*port, HDCP_AN_Data);
				HDMI_DEBUG("Port #[%d] AN[0x%02x0x%02x0x%02x0x%02x0x%02x0x%02x0x%02x0x%02x], loo[%d]\n",\
						*port, HDCP_AN_Data[0], HDCP_AN_Data[1], HDCP_AN_Data[2], HDCP_AN_Data[3], HDCP_AN_Data[4], \
						HDCP_AN_Data[5], HDCP_AN_Data[6], HDCP_AN_Data[7], loop);
				if ( HDCP_AN_Data[0] || HDCP_AN_Data[1] || HDCP_AN_Data[2] || HDCP_AN_Data[3] || \
						HDCP_AN_Data[4] || HDCP_AN_Data[5] || HDCP_AN_Data[6] || HDCP_AN_Data[7])
					break;
				/*
				   _HDMI_M14B0_Get_HDCP_info(*port , &port_authed, &port_enc_en);
				   HDMI_DEBUG("Port #[%d] Authed[%d], loop[%d]\n", *port, port_authed, loop);
				   if (port_authed ==0)
				   break;
				 */
				OS_MsecSleep(50);	// ms delay
			}
			_HDMI_M14B0_PhyOn(*port);
#endif
		}
		else
		{
			HDMI_DEBUG("Direct Port Change to [%d] check_an_data[%d], hdmi5v[%d], prev_port_AN_Data_Zero[%d]\n", \
					*port, gHDMI_Phy_Control.check_an_data ,hdmi5v_status, prev_port_an_data_zero);
			if (!gHDMI_Phy_Control.all_port_pdb_enable)
				_HDMI_M14B0_PhyOn(*port);
			else
			{
				/* All Phy Disabled except port_num */
				if (*port != 0)
					_HDMI_M14B0_EnablePort(0, 0);
				if (*port != 1)
					_HDMI_M14B0_EnablePort(1, 0);
				if (*port != 2)
					_HDMI_M14B0_EnablePort(2, 0);
				if (*port != 3)
					_HDMI_M14B0_EnablePort(3, 0);

				/* Enable port_num */
				_HDMI_M14B0_EnablePort(*port, 1);
			}

		}

		HDMI_DEBUG("[%s] Entered with [%d] \n",__func__, *port);

	}while (0);

	//Check a ARC Enabled Status for Audio Return Channel.
	if (_gM14B0AudioArcStatus == TRUE)
	{
		//ARC source
		LINK_M14_RdFL(edid_heac_00);
		LINK_M14_Wr01(edid_heac_00, reg_arc_src, 0x0);
		LINK_M14_WrFL(edid_heac_00);

		//Enable or Disable ARC port
		LINK_M14_RdFL(phy_link_00);
		LINK_M14_Wr01(phy_link_00, phy_arc_odt_sel_prt0, 0x2);	///< Port1 PHY ARC Temination 저항값 설정 (Default 'b10 Setting 요청!)
		LINK_M14_Wr01(phy_link_00, phy_arc_pdb_prt0, (UINT32)_gM14B0AudioArcStatus);		///< Port1 PHY ARC PDB
		LINK_M14_WrFL(phy_link_00);
	}

	OS_UnlockMutex(&g_HDMI_Sema);

	if( port_selected != *port)
		_HDMI_M14B0_swrst_TMDS_sel_control(0);

	HDMI_DEBUG("[%d] %s :  ARC = %s \n", __L__, __F__, _gM14B0AudioArcStatus? "On":"Off");
	return ret;
}

int HDMI_M14B0_Thread_Control(int sleep_enable)
{
	if (sleep_enable)
	{
		_gM14B0Force_thread_sleep = 2;
		_gM14B0HDMIState = HDMI_STATE_DISABLE;
		HDMI_NOTI("M14B0 sleeping HDMI Thread \n");
	}
	else
	{
		wake_up_interruptible(&WaitQueue_HDMI_M14B0);
		_gM14B0HDMIState = HDMI_STATE_IDLE;
		_gM14B0Force_thread_sleep = 0;
		_gM14B0HDMI_thread_running = 1;
		HDMI_NOTI("M14B0 Starting HDMI Thread\n");
	}

	return RET_OK;
}
int HDMI_M14B0_GetMode(LX_HDMI_MODE_T *mode)
{
	mode->bHDMI =  gM14BootData->mode;

	HDMI_PRINT("[%d] %s : Mode = [%s] \n", __L__, __F__,  mode->bHDMI? "HDMI":"DVI");
	return RET_OK;
}

int HDMI_M14B0_GetAspectRatio(LX_HDMI_ASPECTRATIO_T *ratio)
{
	ratio->eAspectRatio = (((_gM14B0PrevAVIPacket.dataBytes[0] &0xff0000)>>16) &0x30)>>4;		// M1M0

	HDMI_PRINT("[%d] %s : [0] NoData, [1]4:3, [2]16:9 [3] Future  ==> [%d] \n", __L__, __F__, ratio->eAspectRatio);
	return RET_OK;
}

int HDMI_M14B0_GetColorDomain(LX_HDMI_COLOR_DOMAIN_T *color)
{
	UINT32 temp = 0;
	memset((void *)color , 0 , sizeof(LX_HDMI_COLOR_DOMAIN_T));

	if (_gM14B0Intr_vf_stable == HDMI_VIDEO_INIT_STATE || _gM14B0Intr_avi == HDMI_AVI_INIT_STATE)
	{
		return RET_OK;
	}

	if (_gM14B0Intr_avi > HDMI_AVI_INIT_STATE)
	{
		color->bHdmiMode =  gM14BootData->mode;

		if (color->bHdmiMode)
		{
			if (_gM14B0AVIReadState)
				temp = _gM14B0PrevAVIPacket.dataBytes[0];
			else
			{
				LINK_M14_RdFL(packet_18);
				temp = LINK_M14_Rd(packet_18);
			}

			color->ePixelEncoding = (((temp &0xff00)>>8) &0x60)>>5;				// Y1Y0
			color->eColorimetry = (((temp &0xff0000)>>16) &0xc0)>>6;			// C1C0
			color->eExtColorimetry = (((temp &0xff000000)>>24) &0x70)>>4;		// EC2EC1EC0
			color->eITContent = (((temp &0xff000000)>>24) &0x80)>>7;			// ITC
			color->eRGBQuantizationRange = (((temp &0xff000000)>>24) &0x0c)>>2;	// Q1Q0
		}
	}

	return RET_OK;
}

int HDMI_M14B0_GetTimingInfo(LX_HDMI_TIMING_INFO_T *info)
{
	UINT32 tmp32 = 0;
	UINT32 tmdsClock = 0,	tmpClock = 0;
#ifndef M14_THREAD_READ_PHY_REG_VALUE
	UINT32 up_freq = 0,	down_freq = 0;
#endif
	UINT32 pixelRepet = 0, colorDepth = 0;

	LX_HDMI_TIMING_INFO_T 	bufTiming;

	static int abnormal_FP_count = 0;

	memset(&bufTiming , 0 , sizeof(LX_HDMI_TIMING_INFO_T));

	do{
		if (_gM14B0HDMI_thread_running == 1 && _gM14B0HDMIState < HDMI_STATE_READ)
			break;

		if (_gM14B0HDMIState < HDMI_STATE_SCDT)	// 140408 : added for condition when no connection, but prev timing exist
			break;

		if (_gM14B0PortSelected == FALSE)
			break;

		if (_gM14B0HDMI_thread_running == 1 && _gM14B0HDMIState == HDMI_STATE_STABLE && _gM14B0PrevTiming.vFreq > 0)
			goto func_exit;

		LINK_M14_RdFL(video_04);
		LINK_M14_Rd01(video_04, reg_h_tot, bufTiming.hTotal);		///< Horizontal total pixels
		LINK_M14_Rd01(video_04, reg_v_tot, bufTiming.vTotal);		///< Vertical total lines

		LINK_M14_RdFL(video_05);
		LINK_M14_Rd01(video_05, reg_h_av, bufTiming.hActive);		///< Horizontal active pixel
		LINK_M14_Rd01(video_05, reg_v_av, bufTiming.vActive);		///< Vertical active lines
#if 0
		LINK_M14_RdFL(video_06);
		LINK_M14_Rd01(video_06, reg_h_fp, bufTiming.hStart);		///< Horizontal start pixel (Front Porch)
		LINK_M14_Rd01(video_06, reg_v_fp, bufTiming.vStart);		///< Vertical start lines (Front Porch)
#endif
		LINK_M14_RdFL(video_07);
		LINK_M14_Rd01(video_07, reg_h_bp, bufTiming.hStart);	///< Horizontal start pixel (Back Porch)
		LINK_M14_Rd01(video_07, reg_v_bp, bufTiming.vStart);	///< Vertical start lines (Back Porch)

		LINK_M14_RdFL(video_10);
		LINK_M14_Rd01(video_10, reg_intrl, bufTiming.scanType);
		bufTiming.scanType^=1; 							///< Scan type (0 : interlace, 1 : progressive) 	info->scanType ^= 1;

		bufTiming.extInfo = 0; 	///< Full 3D Timing

		_HDMI_M14B0_GetExtendTimingInfo(&bufTiming);		// get extend information

		LINK_M14_RdFL(video_02);
		LINK_M14_Rd01(video_02, reg_cmode_tx, colorDepth);
		colorDepth = (colorDepth &0x60)>>5;

		if ( (bufTiming.extInfo == LX_HDMI_EXT_4K_2K) && (colorDepth > 0))
		{
			HDMI_VIDEO("[%d] %s : not support 4Kx2K color bit over 8bit[%d]\n", __L__, __F__,colorDepth);
			memset(&bufTiming , 0 , sizeof(LX_HDMI_TIMING_INFO_T));
			goto func_cnt;
		}
		// InValid Format Check
		if ( (bufTiming.hActive > 4096) || (bufTiming.vActive > 2415) ||
			(bufTiming.hActive < 320) || (bufTiming.vActive < 240) )
		{
			//memset(&bufTiming , 0 , sizeof(LX_HDMI_TIMING_INFO_T));
			HDMI_VIDEO("[%d] %s : InValid Format for Active Size,  hActive[%d] vActive [%d]\n", __L__, __F__, bufTiming.hActive, bufTiming.vActive);
			goto func_exit;
		}

		//* Support 2D 1280x1024i@86  by 20120202
		//* Not support Master #333 - because MASTER timing issue
		if (bufTiming.hActive == 1280 && bufTiming.vActive == 512 && bufTiming.scanType == 1)
		{
			HDMI_VIDEO("[%d] %s : not support 2D 1280x1024!@86 of Master #333\n", __L__, __F__);
			memset(&bufTiming , 0 , sizeof(LX_HDMI_TIMING_INFO_T));
			goto func_cnt;
		}

	//	_HDMI_M14B0_GetExtendTimingInfo(&bufTiming);		// get extend information

		if (bufTiming.vActive == 0 || bufTiming.hActive == 0 )
		{
			HDMI_PRINT("[%d] %s : _HDMI_M14B0_GetExtendTimingInfo hActive, vActive  = 0\n", __L__, __F__);
			//memset(&bufTiming , 0 , sizeof(LX_HDMI_TIMING_INFO_T));
			goto func_exit;
		}

		//* Support 2D 1920x1080i@50 of EIA-861D  for any PC Card  by 20111010
		//* Not support Master #840 - because MASTER timing issue
		if (bufTiming.hTotal == 2304)// && bufTiming.hActive == 1920 && bufTiming.scanType == 1)
		{
			if (bufTiming.vTotal == 1250)
			{
				HDMI_VIDEO("[%d] %s : Support 2D 1920x1080!@50 of EIA-861D \n", __L__, __F__);
			}
			else
			{
				HDMI_VIDEO("[%d] %s : not support 2D 1920x1080!@50 of EIA-861D Master #840\n", __L__, __F__);
				memset(&bufTiming , 0 , sizeof(LX_HDMI_TIMING_INFO_T));
				goto func_cnt;
			}
		}

		if (bufTiming.hActive %2 == 1)
		{
			HDMI_VIDEO("[%d] %s : hActive modification [%d]\n", __L__, __F__, bufTiming.hActive);
			if ( u16CHK_RANGE(bufTiming.hActive, 4096, 2) )		bufTiming.hActive = 4096;
			else if ( u16CHK_RANGE(bufTiming.hActive, 3840, 2) )	bufTiming.hActive = 3840;
			else if ( u16CHK_RANGE(bufTiming.hActive, 1920, 2) )	bufTiming.hActive = 1920;
			else if ( u16CHK_RANGE(bufTiming.hActive, 1440, 2) )	bufTiming.hActive = 1440;
			else if ( u16CHK_RANGE(bufTiming.hActive, 1280, 2) )	bufTiming.hActive = 1280;
			else if ( u16CHK_RANGE(bufTiming.hActive, 720, 2) )		bufTiming.hActive = 720;
			else if ( u16CHK_RANGE(bufTiming.hActive, 640, 2) )		bufTiming.hActive = 640;
		}

		if (bufTiming.vActive %2 == 1)
		{
			HDMI_VIDEO("[%d] %s : vActive modification [%d]\n", __L__, __F__, bufTiming.vActive);
			if ( u16CHK_RANGE(bufTiming.vActive, 2160, 2) )		bufTiming.vActive = 2160;
			else if ( u16CHK_RANGE(bufTiming.vActive, 1080, 2) )	bufTiming.vActive = 1080;
			else if ( u16CHK_RANGE(bufTiming.vActive, 720, 2) )		bufTiming.vActive = 720;
			else if ( u16CHK_RANGE(bufTiming.vActive, 576, 2) )		bufTiming.vActive = 576;
			else if ( u16CHK_RANGE(bufTiming.vActive, 480, 2) )		bufTiming.vActive = 480;
			else if ( u16CHK_RANGE(bufTiming.vActive, 288, 2) )		bufTiming.vActive = 288;
			else if ( u16CHK_RANGE(bufTiming.vActive, 240, 2) )		bufTiming.vActive = 240;
		}

		///* Start  Horizontal & Vertical frequency *///
#ifdef M14_THREAD_READ_PHY_REG_VALUE
		tmdsClock = _gM14B0HDMIPhyInform.tmds_clock; 	// XXX.XX KHz
#else
		PHY_REG_M14B0_RdFL(tmds_freq_1);
		PHY_REG_M14B0_RdFL(tmds_freq_2);

		PHY_REG_M14B0_Rd01(tmds_freq_1,tmds_freq,up_freq);
		PHY_REG_M14B0_Rd01(tmds_freq_2,tmds_freq,down_freq);
		tmdsClock = ((up_freq << 8) + down_freq); 	// XXX.XX KHz
#endif
		if ( tmdsClock > 29600 )
		{
			HDMI_VIDEO("[%d] %s : not support 4Kx2K over 296Mhz [%d0 Khz]\n", __L__, __F__,tmdsClock);
			memset(&bufTiming , 0 , sizeof(LX_HDMI_TIMING_INFO_T));
			goto func_cnt;
		}

		LINK_M14_RdFL(video_00);
		LINK_M14_Rd01(video_00, reg_pr_tx, pixelRepet);

		tmpClock = tmdsClock * 1000;

		if (tmdsClock != 0 && bufTiming.vTotal > 0 && bufTiming.hTotal > 0 )		// XXX.XX KHz
		{
			//LINK_M14_RdFL(video_02);
			//LINK_M14_Rd01(video_02, reg_cmode_tx, colorDepth);
			//colorDepth = (colorDepth &0x60)>>5;

			if (colorDepth)
			{
				tmdsClock= tmdsClock * 100;
				if (colorDepth == 1) tmdsClock = tmdsClock / 125;					// colorDepth = 10bit
				else				tmdsClock = tmdsClock / 150;					// colorDepth = 12bit
			}
			tmdsClock = tmdsClock * 1000;			// XX,XXX KHz
			//bufTiming.hFreq = tmdsClock / bufTiming.hTotal;		///< Horizontal frequency(100 Hz unit) = Dot Freq / hTotal

			bufTiming.vFreq = tmdsClock / bufTiming.vTotal;		//XX.X KHz
			tmpClock = bufTiming.vFreq * 100;					//X,XX0 KHz
			bufTiming.vFreq = tmpClock / bufTiming.hTotal;		//XX.X Hz	///< Veritical frequency(1/10 Hz unit) = hFreq / vTotal

			if ( bufTiming.scanType == 0)		// 0 : interlace
				bufTiming.vFreq = bufTiming.vFreq + bufTiming.vFreq;

			if (bufTiming.extInfo == LX_HDMI_EXT_3D_FIELD_ALTERNATIVE || pixelRepet > 0)
			{
				bufTiming.vFreq = bufTiming.vFreq >> 1;
			}

			if (bufTiming.vFreq > 1000)							bufTiming.vFreq = 1000;
			else if ( u16CHK_RANGE(bufTiming.vFreq, 240, 5) )		bufTiming.vFreq = 240;
			else if ( u16CHK_RANGE(bufTiming.vFreq, 250, 5) )		bufTiming.vFreq = 250;
			else if ( u16CHK_RANGE(bufTiming.vFreq, 300, 5) )		bufTiming.vFreq = 300;
			else if ( u16CHK_RANGE(bufTiming.vFreq, 500, 5) )		bufTiming.vFreq = 500;
			else if ( u16CHK_RANGE(bufTiming.vFreq, 600, 5) )		bufTiming.vFreq = 600;
			else if ( u16CHK_RANGE(bufTiming.vFreq, 660, 5) )		bufTiming.vFreq = 660;
			else if ( u16CHK_RANGE(bufTiming.vFreq, 700, 5) )		bufTiming.vFreq = 700;
			else if ( u16CHK_RANGE(bufTiming.vFreq, 720, 5) )		bufTiming.vFreq = 720;
			else if ( u16CHK_RANGE(bufTiming.vFreq, 750, 5) )		bufTiming.vFreq = 750;
			else if ( u16CHK_RANGE(bufTiming.vFreq, 760, 5) )		bufTiming.vFreq = 760;
			else if ( u16CHK_RANGE(bufTiming.vFreq, 800, 5) )		bufTiming.vFreq = 800;
			else if ( u16CHK_RANGE(bufTiming.vFreq, 850, 5) )		bufTiming.vFreq = 850;
			else
			{
				tmp32 = bufTiming.vFreq % 10;
				if ( tmp32 != 0 )
				{
					bufTiming.vFreq = bufTiming.vFreq / 10;
					if (tmp32 < 5)
						bufTiming.vFreq = bufTiming.vFreq * 10;
					else
						bufTiming.vFreq = (bufTiming.vFreq + 1) * 10;
				}
			}
		}
		///* End Horizontal & Vertical frequency *///

		if ((_gM14B0HDMIState > HDMI_STATE_NO_SCDT && _gM14B0HDMIState <= HDMI_STATE_READ) ||_gM14B0HDMI_thread_running == 0)
		{
			if (_gM14B0Intr_vf_stable > HDMI_VIDEO_INIT_STATE)
			{
				CTOP_CTRL_M14B0_RdFL(DEI, ctr00);
				CTOP_CTRL_M14B0_Rd01(DEI, ctr00, phy_ppll_sel, tmp32);

				if (pixelRepet > 0)
				{
					if (tmp32 == 0)
					{
						CTOP_CTRL_M14B0_Wr01(DEI, ctr00, phy_ppll_sel, 0x3);
						CTOP_CTRL_M14B0_WrFL(DEI, ctr00);
						HDMI_DEBUG("[%d] %s : change CTOP Clock Divide[1/%d]  for pixel repet \n", __L__, __F__, pixelRepet);
					}
				}
				else if (bufTiming.extInfo == LX_HDMI_EXT_4K_2K)
				{
					if (tmp32 == 0)
					{
						CTOP_CTRL_M14B0_Wr01(DEI, ctr00, phy_ppll_sel, 0x3);
						CTOP_CTRL_M14B0_WrFL(DEI, ctr00);
						HDMI_DEBUG("[%d] %s : change CTOP Clock Divide[1/2]  for UD\n", __L__, __F__);
					}
				}
				else
				{
					if (tmp32 != 0)
					{
						CTOP_CTRL_M14B0_Wr01(DEI, ctr00, phy_ppll_sel, 0x0);
						CTOP_CTRL_M14B0_WrFL(DEI, ctr00);
						HDMI_DEBUG("[%d] %s : change CTOP Clock Divide[1/1]  for default\n", __L__, __F__);
					}
				}
			}

			if (pixelRepet > 1)
			{
				memset(&bufTiming , 0 , sizeof(LX_HDMI_TIMING_INFO_T));
				HDMI_ERROR("[%d] %s : not support Pixel Repetation [%d]\n", __L__, __F__, pixelRepet);
				break;
			}
		}

func_cnt:
		abnormal_FP_count = 0;

		//if ( _gM14B0TimingReadCnt > 100) 	goto func_exit;

		if ( memcmp(&_gM14B0PrevTiming, &bufTiming, sizeof(LX_HDMI_TIMING_INFO_T)) == 0 )
		{
			if (_gM14B0TimingReadCnt <= 100)
				_gM14B0TimingReadCnt++;

			if (_gM14B0HDMIState == HDMI_STATE_READ && _gM14B0TimingReadCnt < 9)
				HDMI_VIDEO("[%d] %s : hActive[%d] vActive [%d] ReadCnt[%d] \n", __L__, __F__, bufTiming.hActive, bufTiming.vActive,_gM14B0TimingReadCnt);
		}

		break;

func_exit:

		memcpy(&bufTiming, &_gM14B0PrevTiming, sizeof(LX_HDMI_TIMING_INFO_T));
	} while(0);

	info->hFreq 	= _gM14B0PrevTiming.hFreq 	= bufTiming.hFreq; 			///< Horizontal frequency(100 Hz unit) = Dot Freq / hTotal
	info->vFreq 	= _gM14B0PrevTiming.vFreq 	= bufTiming.vFreq; 			///< Veritical frequency(1/10 Hz unit) = hFreq / vTotal
	info->hTotal	= _gM14B0PrevTiming.hTotal 	= bufTiming.hTotal; 		///< Horizontal total pixels
	info->vTotal 	= _gM14B0PrevTiming.vTotal	= bufTiming.vTotal; 		///< Vertical total lines
	info->hStart	= _gM14B0PrevTiming.hStart	= bufTiming.hStart; 		///< Horizontal start pixel (Back Porch)
	info->vStart 	= _gM14B0PrevTiming.vStart	= bufTiming.vStart;			///< Vertical start lines (Back Porch)
	info->hActive 	= _gM14B0PrevTiming.hActive	= bufTiming.hActive;		///< Horizontal active pixel
	info->vActive 	= _gM14B0PrevTiming.vActive	= bufTiming.vActive; 		///< Vertical active lines
	info->scanType 	= _gM14B0PrevTiming.scanType	= bufTiming.scanType; 		///< Scan type (0 : interlace, 1 : progressive) 	info->scanType ^= 1;
	info->extInfo	= _gM14B0PrevTiming.extInfo	= bufTiming.extInfo; 	///< Full 3D Timing
	info->state = _gM14B0HDMIState;

	/* Workaround Code for FP vactive timiming with Non-FP 3D information */
	if ( _gM14B0Intr_vsi >= HDMI_VSI_STABLE_STATE)
	{
		int vactive_reg_value;

		LINK_M14_RdFL(video_05);
		LINK_M14_Rd01(video_05, reg_v_av, vactive_reg_value);		///< Vertical active lines

		if( (info->vActive > 1300) ||\
				( ( info->extInfo == LX_HDMI_EXT_3D_FIELD_ALTERNATIVE) && (vactive_reg_value != 1103)  && (vactive_reg_value != 601)
				  && (vactive_reg_value != 503) ) )
		{
			abnormal_FP_count ++;

			if(abnormal_FP_count == 3)
			{

				HDMI_NOTI("#########################################\n");
				HDMI_NOTI("#########################################\n");
				HDMI_NOTI("## 3D format and resolution not match ###\n");
				HDMI_NOTI("#### Port[%d], 3D[%d], Vactive[%d], Vactive_reg[%d] #\n", \
						_gM14B0HDMIPhyInform.prt_sel, info->extInfo, info->vActive, vactive_reg_value);
				HDMI_NOTI("#########################################\n");
				HDMI_NOTI("#########################################\n");

				_g_abnormal_3d_format = 1;
				/*
				_HDMI_M14B0_TMDS_HDCP_ResetPort_Control(_gM14B0HDMIPhyInform.prt_sel, 0);

				PHY_REG_M14B0_RdFL(cr_mode_sel_resetb);
				PHY_REG_M14B0_Wr01(cr_mode_sel_resetb, cr_mode_sel_resetb,0x0);
				PHY_REG_M14B0_WrFL(cr_mode_sel_resetb);

				OS_MsecSleep(10);	// ms delay

				PHY_REG_M14B0_RdFL(cr_mode_sel_resetb);
				PHY_REG_M14B0_Wr01(cr_mode_sel_resetb, cr_mode_sel_resetb,0x1);
				PHY_REG_M14B0_WrFL(cr_mode_sel_resetb);

				OS_MsecSleep(10);	// ms delay

				_HDMI_M14B0_TMDS_HDCP_ResetPort_Control(_gM14B0HDMIPhyInform.prt_sel, 1);
				*/
			}
		}
		else
			abnormal_FP_count = 0;
	}
	else
		abnormal_FP_count = 0;

	if (_gM14B0MHLContentOff && (_gM14B0HDMIPhyInform.prt_sel == 3) && (_gM14B0HDMIPhyInform.cd_sense == 1) )
		memset(info , 0 , sizeof(LX_HDMI_TIMING_INFO_T));

	/*	test for abnormal source
	if (bufTiming.vFreq > 600 && _gM14B0HDMIState == HDMI_STATE_READ)
	{
		HDMI_VIDEO("[%d] %s : VFreq [%d] ReadCnt[%d] \n", __L__, __F__, bufTiming.vFreq, _gM14B0TimingReadCnt);
		memset(info , 0 , sizeof(LX_HDMI_TIMING_INFO_T));
	}
	*/

	HDMI_PRINT("[%d] %s abnormal_FP_count[%d] \n", __L__, __F__, abnormal_FP_count);
	return RET_OK;
}

int HDMI_M14B0_GetAviPacket(LX_HDMI_INFO_PACKET_T *packet)
{
	ULONG flags = 0;
	UINT32 header;
	UINT32 data[8];
	static UINT32 gAVIReadCnt = 0;
	LX_HDMI_AVI_COLORSPACE_T	csc = LX_HDMI_AVI_COLORSPACE_RGB;

	memset((void *)packet , 0 , sizeof(LX_HDMI_INFO_PACKET_T));
	//packet->InfoFrameType = LX_HDMI_INFO_AVI;

	if (_gM14B0Intr_vf_stable == HDMI_VIDEO_INIT_STATE || _gM14B0Intr_avi == HDMI_AVI_INIT_STATE)
	{
		gAVIReadCnt = 0;
		memset(&_gM14B0PrevAVIPacket, 0 , sizeof(LX_HDMI_INFO_PACKET_T));
		//_gM14B0PrevAVIPacket.InfoFrameType = LX_HDMI_INFO_AVI;
		//HDMI_VIDEO("[%d] %s : _gM14B0Intr_vf_stable AVI  / _gM14B0Intr_vf_stable \n", __L__, __F__ );
#ifdef HDMI_MODE_WHEN_NO_AVI_WORKAROUND
		if ( _gM14B0HDMI_thread_running == 1)
			packet->dataBytes[7] = gM14BootData->mode;		//HDMI Mode
		else
		{
			UINT32 prt_selected;
			LINK_M14_RdFL(system_control_00);

			LINK_M14_Rd01(system_control_00, reg_prt_sel, prt_selected);

			if (prt_selected == 0)
				LINK_M14_Rd01(system_control_00, reg_hdmi_mode_prt0, data[7]);
			else if (prt_selected == 1)
				LINK_M14_Rd01(system_control_00, reg_hdmi_mode_prt1, data[7]);
			else if (prt_selected == 2)
				LINK_M14_Rd01(system_control_00, reg_hdmi_mode_prt2, data[7]);
			else
				LINK_M14_Rd01(system_control_00, reg_hdmi_mode_prt3, data[7]);

			//LINK_M14_Rd01(system_control_00, reg_hdmi_mode, data[7]);
			packet->dataBytes[7] = gM14BootData->mode = data[7];		//HDMI Mode
		}
#endif
		return RET_OK;
	}

	if ( _gM14B0HDMIPhyInform.tmds_clock > 29600 )
	{
		gAVIReadCnt = 0;
		HDMI_VIDEO("[%d] %s : not support 4Kx2K over 296Mhz [%d0 Khz]\n", __L__, __F__,_gM14B0HDMIPhyInform.tmds_clock);
		memset(&_gM14B0PrevAVIPacket, 0 , sizeof(LX_HDMI_INFO_PACKET_T));
		return RET_OK;
	}

	if ( _gM14B0HDMI_thread_running == 1 && _gM14B0Intr_avi == HDMI_AVI_STABLE_STATE)
		goto func_exit;

	if (_gM14B0Intr_avi == HDMI_AVI_CHANGE_STATE)
	{
		//gAVIReadCnt = 0;
		spin_lock_irqsave(&_gIntrHdmiM14B0VideoLock, flags);
		_gM14B0Intr_avi = HDMI_AVI_INTERRUPT_STATE;
		_gM14B0AVIReadState = FALSE;
		spin_unlock_irqrestore(&_gIntrHdmiM14B0VideoLock, flags);
	}

	if (gAVIReadCnt > 300)
	{
		spin_lock_irqsave(&_gIntrHdmiM14B0VideoLock, flags);
		_gM14B0Intr_avi = HDMI_AVI_STABLE_STATE;
		spin_unlock_irqrestore(&_gIntrHdmiM14B0VideoLock, flags);
	}

	LINK_M14_RdFL(packet_17);
	header = LINK_M14_Rd(packet_17);		// reg_pkt_avi_hdr_0 (AVI Packet Version), reg_pkt_avi_hdr_1 (AVI Packet Length)
	packet->header = (header &0xffff);

	LINK_M14_RdFL(packet_18);
	LINK_M14_RdFL(packet_19);
	LINK_M14_RdFL(packet_20);
	LINK_M14_RdFL(packet_21);
	LINK_M14_RdFL(packet_22);
	LINK_M14_RdFL(packet_23);
	LINK_M14_RdFL(packet_24);

	data[0] = LINK_M14_Rd(packet_18);
	data[1] = LINK_M14_Rd(packet_19);
	data[2] = LINK_M14_Rd(packet_20);
	data[3] = LINK_M14_Rd(packet_21);
	data[4] = LINK_M14_Rd(packet_22);
	data[5] = LINK_M14_Rd(packet_23);
	data[6] = LINK_M14_Rd(packet_24);

	packet->dataBytes[0] = data[0];
	packet->dataBytes[1] = data[1];
	packet->dataBytes[2] = data[2];
	packet->dataBytes[3] = data[3];
	packet->dataBytes[4] = data[4];
	packet->dataBytes[5] = data[5];
	packet->dataBytes[6] = data[6];

	if ( _gM14B0HDMI_thread_running == 1)
		packet->dataBytes[7] = gM14BootData->mode;		//HDMI Mode
	else
	{
		UINT32 prt_selected;
		LINK_M14_RdFL(system_control_00);

		LINK_M14_Rd01(system_control_00, reg_prt_sel, prt_selected);

		if (prt_selected == 0)
			LINK_M14_Rd01(system_control_00, reg_hdmi_mode_prt0, data[7]);
		else if (prt_selected == 1)
			LINK_M14_Rd01(system_control_00, reg_hdmi_mode_prt1, data[7]);
		else if (prt_selected == 2)
			LINK_M14_Rd01(system_control_00, reg_hdmi_mode_prt2, data[7]);
		else
			LINK_M14_Rd01(system_control_00, reg_hdmi_mode_prt3, data[7]);

		//LINK_M14_Rd01(system_control_00, reg_hdmi_mode, data[7]);
		packet->dataBytes[7] = gM14BootData->mode = data[7];		//HDMI Mode
	}

	csc = (((data[0] &0xff00)>>8) &0x60)>>5;

	if ((csc == 3) && ((data[0]>>8)  == 0x60))		//data[0] 0xff ==> Cheaksum, data[0] 0x6000 ==> Y!Y0
	{
		csc = 0;
		packet->dataBytes[0] = data[0] = data[0] &0xff;
	}

	if (_gM14B0PrevPixelEncoding != csc)
	{
		HDMI_NOTI("CSC change [%d] => [%d]\n", _gM14B0PrevPixelEncoding , csc);
		_HDMI_M14B0_SetVideoBlank(csc);
		_gM14B0PrevPixelEncoding = csc;
	}

	_gM14B0AVIReadState = TRUE;

	if ( memcmp(&_gM14B0PrevAVIPacket, packet, sizeof(LX_HDMI_INFO_PACKET_T)) != 0 )
	{
		gAVIReadCnt = 0;
		HDMI_VIDEO("[%d] %s : changed AVI Packet / InfoFrameType gPre[%d] /packet[%d] \n", __L__, __F__, _gM14B0PrevAVIPacket.InfoFrameType, packet->InfoFrameType);
		HDMI_VIDEO("AVI header	gPre[%d] /packet[%d] \n", _gM14B0PrevAVIPacket.header, packet->header);
		HDMI_VIDEO("AVI data	gPre[0x%x][0x%x][0x%x][0x%x][0x%x][0x%x][0x%x][0x%x] \n", _gM14B0PrevAVIPacket.dataBytes[0], _gM14B0PrevAVIPacket.dataBytes[1], _gM14B0PrevAVIPacket.dataBytes[2], \
			_gM14B0PrevAVIPacket.dataBytes[3], _gM14B0PrevAVIPacket.dataBytes[4], _gM14B0PrevAVIPacket.dataBytes[5], _gM14B0PrevAVIPacket.dataBytes[6], _gM14B0PrevAVIPacket.dataBytes[7]);
		HDMI_VIDEO("AVI data	packet[0x%x][0x%x][0x%x][0x%x][0x%x][0x%x][0x%x][0x%x] \n", 	packet->dataBytes[0], packet->dataBytes[1], packet->dataBytes[2], \
			packet->dataBytes[3], packet->dataBytes[4], packet->dataBytes[5], packet->dataBytes[6], packet->dataBytes[7]);

		memcpy(&_gM14B0PrevAVIPacket, packet, sizeof(LX_HDMI_INFO_PACKET_T));
		return RET_OK;
	}
	else
	{
		if (gAVIReadCnt < 350) 		gAVIReadCnt++;
		return RET_OK;
	}

func_exit:
	memcpy(packet, &_gM14B0PrevAVIPacket,  sizeof(LX_HDMI_INFO_PACKET_T));
	HDMI_PRINT("[%d] %s \n", __L__, __F__);

	return RET_OK;
}

int HDMI_M14B0_GetVsiPacket(LX_HDMI_INFO_PACKET_T *packet)
{
	ULONG flags = 0;
	UINT32 header;
	UINT32 data[9];
	static UINT32 gVSIReadCnt = 0;

	memset((void *)packet , 0 , sizeof(LX_HDMI_INFO_PACKET_T));
	packet->InfoFrameType = LX_HDMI_INFO_VSI;

	//HDMI_DEBUG("%s, %d _gM14B0Intr_vf_stable= %d,_gM14B0Intr_vsi = %d, \n\n",__F__, __L__, _gM14B0Intr_vf_stable, _gM14B0Intr_vsi);

	if (_gM14B0Intr_vf_stable == HDMI_VIDEO_INIT_STATE ||_gM14B0Intr_vsi < HDMI_VSI_INTERRUPT_STATE)
	{
		//HDMI_VIDEO("%s, %d _gM14B0Intr_vf_stable= %d,_gM14B0Intr_vsi = %d, \n\n",__F__, __L__, _gM14B0Intr_vf_stable, _gM14B0Intr_vsi);
		gVSIReadCnt = 0;
		memset(&_gM14B0PrevVSIPacket, 0 , sizeof(LX_HDMI_INFO_PACKET_T));
		_gM14B0PrevVSIPacket.InfoFrameType = LX_HDMI_INFO_VSI;
		return RET_OK;
	}

	if ( _gM14B0HDMIPhyInform.tmds_clock > 29600 )
	{
		gVSIReadCnt = 0;
		HDMI_VIDEO("[%d] %s : not support 4Kx2K over 296Mhz [%d0 Khz]\n", __L__, __F__,_gM14B0HDMIPhyInform.tmds_clock);
		memset(&_gM14B0PrevVSIPacket, 0 , sizeof(LX_HDMI_INFO_PACKET_T));
		_gM14B0PrevVSIPacket.InfoFrameType = LX_HDMI_INFO_VSI;
		return RET_OK;
	}

	if (_gM14B0VSIState == TRUE)
	{
		{
			LINK_M14_RdFL(interrupt_enable_01);
			LINK_M14_Wr01(interrupt_enable_01, intr_no_vsi_enable, 0x1);			///< 20 intr_no_vsi_enable
			LINK_M14_WrFL(interrupt_enable_01);
			_gM14B0VSIState = FALSE;
			HDMI_DEBUG("[%d] %s : No VSI intra enable  \n", __LINE__, __func__);
		}
	}

	if ( _gM14B0HDMI_thread_running == 1 && _gM14B0Intr_vsi == HDMI_VSI_STABLE_STATE)
		goto func_exit;

	if (gVSIReadCnt > 200)
	{
		spin_lock_irqsave(&_gIntrHdmiM14B0VideoLock, flags);
		_gM14B0Intr_vsi = HDMI_VSI_STABLE_STATE;
		spin_unlock_irqrestore(&_gIntrHdmiM14B0VideoLock, flags);
	}

	LINK_M14_RdFL(packet_74);
	header = LINK_M14_Rd(packet_74);		// reg_pkt_avi_hdr_0 (AVI Packet Version), reg_pkt_avi_hdr_1 (AVI Packet Length)
	packet->header = (header &0xffff);

	LINK_M14_RdFL(packet_75);
	LINK_M14_RdFL(packet_76);
	LINK_M14_RdFL(packet_77);
	LINK_M14_RdFL(packet_78);
	LINK_M14_RdFL(packet_79);
	LINK_M14_RdFL(packet_80);
	LINK_M14_RdFL(packet_81);

	data[0] = LINK_M14_Rd(packet_75);
	data[1] = LINK_M14_Rd(packet_76);
	data[2] = LINK_M14_Rd(packet_77);
	data[3] = LINK_M14_Rd(packet_78);
	data[4] = LINK_M14_Rd(packet_79);
	data[5] = LINK_M14_Rd(packet_80);
	data[6] = LINK_M14_Rd(packet_81);

	packet->dataBytes[0] = data[0];
	packet->dataBytes[1] = data[1];
	packet->dataBytes[2] = data[2];
	packet->dataBytes[3] = data[3];
	packet->dataBytes[4] = data[4];
	packet->dataBytes[5] = data[5];
	packet->dataBytes[6] = data[6];
	packet->dataBytes[7] = 0;

	if ( memcmp(&_gM14B0PrevVSIPacket, packet, sizeof(LX_HDMI_INFO_PACKET_T)) != 0 )
	{
		gVSIReadCnt = 0;
		HDMI_VIDEO("[%d] %s : changed VSI Packet / InfoFrameType gPre[%d] /packet[%d] \n", __L__, __F__, _gM14B0PrevVSIPacket.InfoFrameType, packet->InfoFrameType);
		HDMI_VIDEO("VSI header	gPre[%d] /packet[%d] \n", _gM14B0PrevVSIPacket.header, packet->header);
		HDMI_VIDEO("VSI data	gPre[0x%x][0x%x][0x%x][0x%x][0x%x][0x%x][0x%x][0x%x] \n", _gM14B0PrevVSIPacket.dataBytes[0], _gM14B0PrevVSIPacket.dataBytes[1], _gM14B0PrevVSIPacket.dataBytes[2], \
			_gM14B0PrevVSIPacket.dataBytes[3], _gM14B0PrevVSIPacket.dataBytes[4], _gM14B0PrevVSIPacket.dataBytes[5], _gM14B0PrevVSIPacket.dataBytes[6], _gM14B0PrevVSIPacket.dataBytes[7]);
		HDMI_VIDEO("VSI data	packet[0x%x][0x%x][0x%x][0x%x][0x%x][0x%x][0x%x][0x%x] \n", 	packet->dataBytes[0], packet->dataBytes[1], packet->dataBytes[2], \
			packet->dataBytes[3], packet->dataBytes[4], packet->dataBytes[5], packet->dataBytes[6], packet->dataBytes[7]);

		memcpy(&_gM14B0PrevVSIPacket, packet, sizeof(LX_HDMI_INFO_PACKET_T));
		return RET_OK;
	}
	else
	{
		if (gVSIReadCnt < 250) 		gVSIReadCnt++;
		return RET_OK;
	}

func_exit:
	memcpy(packet, &_gM14B0PrevVSIPacket, sizeof(LX_HDMI_INFO_PACKET_T));
	HDMI_PRINT("[%d] %s \n", __L__, __F__);

	return RET_OK;
}

int HDMI_M14B0_GetSpdPacket(LX_HDMI_INFO_PACKET_T *packet)
{
	UINT32 header;
	UINT32 data[9];
	static LX_HDMI_INFO_PACKET_T gPrevSPDPacket = {0, };

	memset((void *)packet , 0 , sizeof(LX_HDMI_INFO_PACKET_T));
	packet->InfoFrameType = LX_HDMI_INFO_SPD;

	if (_gM14B0Intr_vf_stable == HDMI_VIDEO_INIT_STATE)
	{
		memset(&gPrevSPDPacket, 0 , sizeof(LX_HDMI_INFO_PACKET_T));
		gPrevSPDPacket.InfoFrameType = LX_HDMI_INFO_SPD;
		return RET_OK;
	}

	LINK_M14_RdFL(packet_58);
	header = LINK_M14_Rd(packet_58);
	packet->header = (header &0xffff);

	LINK_M14_RdFL(packet_59);
	LINK_M14_RdFL(packet_60);
	LINK_M14_RdFL(packet_61);
	LINK_M14_RdFL(packet_62);
	LINK_M14_RdFL(packet_63);
	LINK_M14_RdFL(packet_64);
	LINK_M14_RdFL(packet_65);

	data[0] = LINK_M14_Rd(packet_59);
	data[1] = LINK_M14_Rd(packet_60);
	data[2] = LINK_M14_Rd(packet_61);
	data[3] = LINK_M14_Rd(packet_62);
	data[4] = LINK_M14_Rd(packet_63);
	data[5] = LINK_M14_Rd(packet_64);
	data[6] = LINK_M14_Rd(packet_65);

	packet->dataBytes[0] = data[0];
	packet->dataBytes[1] = data[1];
	packet->dataBytes[2] = data[2];
	packet->dataBytes[3] = data[3];
	packet->dataBytes[4] = data[4];
	packet->dataBytes[5] = data[5];
	packet->dataBytes[6] = data[6];
	packet->dataBytes[7] = 0;

	if ( memcmp(&gPrevSPDPacket, packet, sizeof(LX_HDMI_INFO_PACKET_T)) != 0 )
	{
		HDMI_VIDEO("[%d] %s : changed SPD Packet / InfoFrameType gPre[%d] /packet[%d] \n", __L__, __F__, gPrevSPDPacket.InfoFrameType, packet->InfoFrameType);
		HDMI_VIDEO("SPD header	gPre[%d] /packet[%d] \n", gPrevSPDPacket.header, packet->header);
		HDMI_VIDEO("SPD data	gPre[0x%x][0x%x][0x%x][0x%x][0x%x][0x%x][0x%x][0x%x] \n", gPrevSPDPacket.dataBytes[0], gPrevSPDPacket.dataBytes[1], gPrevSPDPacket.dataBytes[2], \
			gPrevSPDPacket.dataBytes[3], gPrevSPDPacket.dataBytes[4], gPrevSPDPacket.dataBytes[5], gPrevSPDPacket.dataBytes[6], gPrevSPDPacket.dataBytes[7]);
		HDMI_VIDEO("SPD data	packet[0x%x][0x%x][0x%x][0x%x][0x%x][0x%x][0x%x][0x%x] \n", 	packet->dataBytes[0], packet->dataBytes[1], packet->dataBytes[2], \
			packet->dataBytes[3], packet->dataBytes[4], packet->dataBytes[5], packet->dataBytes[6], packet->dataBytes[7]);

		memcpy(&gPrevSPDPacket, packet, sizeof(LX_HDMI_INFO_PACKET_T));
	}

	return RET_OK;
}

int HDMI_M14B0_GetInfoPacket(LX_HDMI_INFO_PACKET_T *packet)
{
	//not implemented now!!!
	memset((void *)packet, 0, sizeof(LX_HDMI_INFO_PACKET_T));

	if (_gM14B0Intr_vf_stable == HDMI_VIDEO_INIT_STATE)
		return RET_OK;

	return RET_OK;
}

int HDMI_M14B0_SetPowerControl(UINT32 power)
{
	/*
	if (power)
	{
		_gM14B0HDMIPhyInform.power_control = 1;
	}
	else if (!gHDMI_Phy_Control.all_port_pdb_enable)
	{
		_gM14B0HDMIPhyInform.power_control = 0;

		_HDMI_M14B0_PhyOff(0);
		_HDMI_M14B0_PhyOff(1);
		_HDMI_M14B0_PhyOff(2);
		_HDMI_M14B0_PhyOff(3);

		_HDMI_M14B0_Set_HPD_Out_A4P(0,0,0,0);
	}

	*/
	HDMI_PRINT("[%d] %s :  %s \n", __L__, __F__, (power ? "On" : "Off"));

	return RET_OK;
}

int HDMI_M14B0_SetHPD(LX_HDMI_HPD_T *hpd)
{
	HDMI_NOTI("HPD enable [%s]\n",(hpd->bEnable ? "On" : "Off"));

	OS_LockMutex(&g_HDMI_Sema);

	if (hpd->bEnable)
		_gM14B0HDMIPhyInform.hpd_enable = 1;
	else
	{
		_gM14B0HDMIPhyInform.hpd_enable = 0;

			_HDMI_M14B0_PhyOff(0);
			_HDMI_M14B0_PhyOff(1);
			_HDMI_M14B0_PhyOff(2);
			_HDMI_M14B0_PhyOff(3);

			_HDMI_M14B0_Set_HPD_Out_A4P(!_gM14B0HDMIPhyInform.hpd_pol[0], \
					!_gM14B0HDMIPhyInform.hpd_pol[1], \
					!_gM14B0HDMIPhyInform.hpd_pol[2], \
					!_gM14B0HDMIPhyInform.hpd_pol[3]);
	}

	OS_UnlockMutex(&g_HDMI_Sema);

	return RET_OK;
}


int HDMI_M14B0_GetStatus(LX_HDMI_STATUS_T *status)
{
	UINT32 tmdsClock = 0;
#ifndef M14_THREAD_READ_PHY_REG_VALUE
	UINT32 up_freq = 0,	down_freq = 0;
#endif
	LX_HDMI_AVI_COLORSPACE_T csc = 0;		    ///< PixelEncoding

	memset((void *)status , 0 , sizeof(LX_HDMI_STATUS_T));

	do {
		if (_gM14B0HDMIState < HDMI_STATE_SCDT)
			break;

		status->bHdmiMode = gM14BootData->mode;

		status->eHotPlug = _gM14B0HDMIConnectState;

		LINK_M14_RdFL(video_02);
		LINK_M14_Rd01(video_02, reg_cmode_tx, status->eColorDepth);

		status->eColorDepth = (status->eColorDepth &0x60)>>5;

		LINK_M14_RdFL(video_00);
		LINK_M14_Rd01(video_00, reg_pr_tx, status->pixelRepet);

		status->csc = _gM14B0PrevPixelEncoding;
		csc = (((_gM14B0PrevAVIPacket.dataBytes[0] &0xff00)>>8) &0x60)>>5;
		if (_gM14B0PrevPixelEncoding == csc)		HDMI_DEBUG(" _gM14B0PrevPixelEncoding = %d , csc = %d \n", _gM14B0PrevPixelEncoding, csc);

#ifdef M14_THREAD_READ_PHY_REG_VALUE
		tmdsClock = _gM14B0HDMIPhyInform.tmds_clock; 	// XXX.XX KHz
#else
		PHY_REG_M14B0_RdFL(tmds_freq_1);
		PHY_REG_M14B0_RdFL(tmds_freq_2);

		PHY_REG_M14B0_Rd01(tmds_freq_1,tmds_freq,up_freq);
		PHY_REG_M14B0_Rd01(tmds_freq_2,tmds_freq,down_freq);
		tmdsClock = ((up_freq << 8) + down_freq); 	// XXX.XX KHz
#endif

		HDMI_VIDEO("HDMI_M14B0_GetStatus  depth = 0x%x tmds =%d\n", status->eColorDepth, tmdsClock);
		HDMI_VIDEO("[HDMI] %d HDMI  State  = %d _gM14B0Force_thread_sleep= %d  _gM14B0HDMI_thread_running =%d \n",\
			__L__, _gM14B0HDMIState,_gM14B0Force_thread_sleep, _gM14B0HDMI_thread_running);
		HDMI_VIDEO("[HDMI]  AVI State[%d] VSI State[%d]  SRC Mute State[%d] \n",  _gM14B0Intr_avi, _gM14B0Intr_vsi, _gM14B0Intr_src_mute );
	} while (0);

	return RET_OK;
}

/**
 *  HDMI_M14B0_GetAudioInfo
 *
 *  @parm LX_HDMI_AUDIO_INFO_T *
 *  @return int
*/
int HDMI_M14B0_GetAudioInfo(LX_HDMI_AUDIO_INFO_T *pAudioInfo)
{
	int ret = RET_OK;

	OS_LockMutex(&g_HDMI_Sema);

	//Return a previous global value.
	pAudioInfo->audioType	 = _gM14B0HdmiAudioInfo.audioType;
	pAudioInfo->samplingFreq = _gM14B0HdmiAudioInfo.samplingFreq;

	//Debug print
	if ((_gM14B0HdmiAudioThreadCount % DEBUG_HDMI_AUDIO_MSG_PRINT_TIME_1S) == 0)
	{
		HDMI_AUDIO("HDMI_GetAudioInfo : type = %d, freq = %d, state = %d, mute = %d\n",	\
					pAudioInfo->audioType, pAudioInfo->samplingFreq, _gM14B0IntrAudioState, _gM14B0AudioMuteState);
	}
	else
	{
		HDMI_PRINT("HDMI_GetAudioInfo : type = %d, freq = %d, state = %d, mute = %d\n",	\
					pAudioInfo->audioType, pAudioInfo->samplingFreq, _gM14B0IntrAudioState, _gM14B0AudioMuteState);
	}

	OS_UnlockMutex(&g_HDMI_Sema);

	return ret;
}

/**
 *  HDMI_M14B0_GetAudioCopyInfo
 *
 *  @parm LX_HDMI_AUDIO_COPY_T *
 *  @return int
*/
int HDMI_M14B0_GetAudioCopyInfo(LX_HDMI_AUDIO_COPY_T *pCopyInfo)
{
	int ret = RET_OK;

	UINT32	prt_selected, reg_hdmi_mode, reg_achst_byte0, reg_achst_byte1;
	UINT32	CpBit, LBit;

	OS_LockMutex(&g_HDMI_Sema);

	//Check HDMI /DVI Mode, 0 : DVI, 1 : HDMI
	LINK_M14_RdFL(system_control_00);
	//LINK_M14_Rd01(system_control_00, reg_hdmi_mode, reg_hdmi_mode);	//H13
	LINK_M14_Rd01(system_control_00, reg_prt_sel, prt_selected);

	if (prt_selected == 0)
		LINK_M14_Rd01(system_control_00, reg_hdmi_mode_prt0, reg_hdmi_mode);
	else if (prt_selected == 1)
		LINK_M14_Rd01(system_control_00, reg_hdmi_mode_prt1, reg_hdmi_mode);
	else if (prt_selected == 2)
		LINK_M14_Rd01(system_control_00, reg_hdmi_mode_prt2, reg_hdmi_mode);
	else
		LINK_M14_Rd01(system_control_00, reg_hdmi_mode_prt3, reg_hdmi_mode);

	//Get a hdmi audio copyright information.
	if (reg_hdmi_mode == 1)	//HDMI Mode
	{
		//Read  reg_achst_byte0 reg.
		LINK_M14_RdFL(audio_07);
		LINK_M14_Rd01(audio_07, reg_achst_byte0, reg_achst_byte0);

		//Set a CpBit
		if (reg_achst_byte0 & HDMI_AUDIO_CP_BIT_MASK)
			CpBit = 1;
		else
			CpBit = 0;

		//Read  reg_achst_byte1reg.
		LINK_M14_RdFL(audio_08);
		LINK_M14_Rd01(audio_08, reg_achst_byte1, reg_achst_byte1);

		//Set a LBit
		if (reg_achst_byte1 & HDMI_AUDIO_L_BIT_MASK)
			LBit = 1;
		else
			LBit = 0;

		//Set a Copyright Info. by CpBit and LBit
		if (CpBit == 0 && LBit == 0)
			*pCopyInfo = LX_HDMI_AUDIO_COPY_ONCE;
		else if (CpBit == 0 && LBit == 1)
			*pCopyInfo = LX_HDMI_AUDIO_COPY_NO_MORE;	//same with LX_HDMI_AUDIO_COPY_NEVER
		else
			*pCopyInfo = LX_HDMI_AUDIO_COPY_FREE;

		//Debug print
		if ((_gM14B0HdmiAudioThreadCount % DEBUG_HDMI_AUDIO_MSG_PRINT_TIME_1S) == 0)
		{
			HDMI_AUDIO("HDMI_M14B0_GetAudioCopyInfo : CopyInfo = %d(mode = %d)(0x%X, 0x%X)\n", *pCopyInfo, reg_hdmi_mode, reg_achst_byte0, reg_achst_byte1);
		}
		else
		{
			HDMI_PRINT("HDMI_M14B0_GetAudioCopyInfo : CopyInfo = %d(mode = %d)(0x%X, 0x%X)\n", *pCopyInfo, reg_hdmi_mode, reg_achst_byte0, reg_achst_byte1);
		}
	}
	else	//DVI Mode
	{
		*pCopyInfo = LX_HDMI_AUDIO_COPY_FREE;
	}

	OS_UnlockMutex(&g_HDMI_Sema);

	HDMI_PRINT("HDMI_M14B0_GetAudioCopyInfo : CopyInfo = %d(mode = %d)\n", *pCopyInfo, reg_hdmi_mode);
	return ret;
}

/**
 *  HDMI_M14B0_SetArc
 *
 *  @parm LX_HDMI_ARC_CTRL_T *
 *  @return int
*/
int HDMI_M14B0_SetArc(LX_HDMI_ARC_CTRL_T *pArcCtrl)
{
	OS_LockMutex(&g_HDMI_Sema);

	//ARC source
	LINK_M14_RdFL(edid_heac_00);
	LINK_M14_Wr01(edid_heac_00, reg_arc_src, 0x0);
	LINK_M14_WrFL(edid_heac_00);

	//Enable or Disable ARC port
	LINK_M14_RdFL(phy_link_00);
	LINK_M14_Wr01(phy_link_00, phy_arc_odt_sel_prt0, 0x2);	///< Port1 PHY ARC Temination 저항값 설정 (Default 'b10 Setting 요청!)
	LINK_M14_Wr01(phy_link_00, phy_arc_pdb_prt0, (UINT32)pArcCtrl->bARCEnable);		///< Port1 PHY ARC PDB
	LINK_M14_WrFL(phy_link_00);

	//Update _gAudioArcStatus
	_gM14B0AudioArcStatus = pArcCtrl->bARCEnable;

	OS_UnlockMutex(&g_HDMI_Sema);

	HDMI_PRINT("[%d] %s :  %s \n", __L__, __F__, pArcCtrl->bARCEnable? "On":"Off");
	return RET_OK;
}

/**
 *  HDMI_M14B0_SetMute
 *
 *  @parm LX_HDMI_MUTE_CTRL_T *
 *  @return int
*/
int HDMI_M14B0_SetMute(LX_HDMI_MUTE_CTRL_T *pMuteCtrl)
{
	BOOLEAN 	videoMuteState;

	OS_LockMutex(&g_HDMI_Sema);

	LINK_M14_RdFL(system_control_01);
	LINK_M14_Rd01(system_control_01, reg_mute_vid, videoMuteState);

	//video related.
	if (pMuteCtrl->eMode == LX_HDMI_VIDEO_MUTE || pMuteCtrl->eMode == LX_HDMI_AV_MUTE)
	{
		if (pMuteCtrl->bVideoMute != videoMuteState)
		{
			LINK_M14_Wr01(system_control_01, reg_mute_vid, pMuteCtrl->bVideoMute);
			LINK_M14_WrFL(system_control_01);

			HDMI_VIDEO("[%d] %s : bVideoMute = %s \n", __L__, __F__, (pMuteCtrl->bVideoMute ? "On" : "Off"));
		}
	}

	//audio related.
	if (pMuteCtrl->eMode == LX_HDMI_AUDIO_MUTE || pMuteCtrl->eMode == LX_HDMI_AV_MUTE)
	{
		//Check a previous state
		if (_gM14B0AudioMuteState != pMuteCtrl->bAudioMute)
		{
			if (pMuteCtrl->bAudioMute == TRUE)
			{
				//Mute audio data
				LINK_M14_RdFL(audio_05);
				LINK_M14_Wr01(audio_05, reg_i2s_sd_en, 0x0);	//I2S SD Output Disable(4 Channel)
				LINK_M14_WrFL(audio_05);
			}
			else
			{
				//Un-mute audio data
				LINK_M14_RdFL(audio_05);
				LINK_M14_Wr01(audio_05, reg_i2s_sd_en, 0xF);	//I2S SD Output Enable(4 Channel)
				LINK_M14_WrFL(audio_05);
			}

			//Update _gM14B0AudioMuteState
			_gM14B0AudioMuteState = pMuteCtrl->bAudioMute;

			//For debug print
			if (pMuteCtrl->bAudioMute == FALSE)
			{
				HDMI_AUDIO("[%d] %s : bAudioMute = %s \n", __L__, __F__, (pMuteCtrl->bAudioMute ? "On" : "Off"));
			}
			else
			{
				HDMI_DEBUG("[%d] %s : bAudioMute = %s \n", __L__, __F__, (pMuteCtrl->bAudioMute ? "On" : "Off"));
			}
		}

		//For debug print
		if (pMuteCtrl->bAudioMute == FALSE)
		{
			HDMI_DEBUG("[%d]SetMute : type = %d, freq = %d, mute = %d, PSC = %d\n", \
						__L__, _gM14B0HdmiAudioInfo.audioType, _gM14B0HdmiAudioInfo.samplingFreq, _gM14B0AudioMuteState, _gM14B0HdmiPortStableCount);
		}
	}

	OS_UnlockMutex(&g_HDMI_Sema);

	return RET_OK;
}

/**
 *  HDMI_M14B0_GetAudioDebugInfo
 *
 *  @parm LX_HDMI_AUDIO_INFO_T *
 *  @return int
*/
int HDMI_M14B0_GetAudioDebugInfo(LX_HDMI_DEBUG_AUDIO_INFO_T *pAudioDebugInfo)
{
	int ret = RET_OK;

	OS_LockMutex(&g_HDMI_Sema);

	//Copy and Print HDMI Audio Debug Info.
	(void)_HDMI_M14B0_DebugAudioInfo(pAudioDebugInfo);

	HDMI_DEBUG("HDMI_M14B0_GetAudioDebugInfo : type = %d, freq = %d, state = %d, mute = %d, PSC = %d, video = %d\n", \
				_gM14B0HdmiAudioInfo.audioType, _gM14B0HdmiAudioInfo.samplingFreq, _gM14B0IntrAudioState, _gM14B0AudioMuteState, _gM14B0HdmiPortStableCount, _gM14B0HDMIState);

	OS_UnlockMutex(&g_HDMI_Sema);

	return ret;
}

static int _HDMI_M14B0_GetExtendTimingInfo(LX_HDMI_TIMING_INFO_T *info)
{
	UINT8 	tmp8 =0;
	UINT32	tmp32 = 0, i = 0;
	//UINT32 	videoIdCode = 0;
	LX_HDMI_TIMING_INFO_T 	bufTiming = {0,};
	LX_HDMI_VSI_VIDEO_FORMAT_T eVideoFormat = 0;	/**< HDMI VSI info */
	LX_HDMI_VSI_3D_STRUCTURE_T e3DStructure = 0;	/**< HDMI VSI info */
	const sEXT_TIMING_ENUM *pTbl;
	const sO_ENUM *p3DTbl;

	do {
		memcpy(&bufTiming , info , sizeof(LX_HDMI_TIMING_INFO_T));
		bufTiming.extInfo = LX_HDMI_EXT_2D_FORMAT;		///< 2D format

		if (_gM14B0Intr_vsi < HDMI_VSI_INTERRUPT_STATE)
		{
			pTbl = &TBL_EXT_INFO[0];
			for (i = 0; i < TBL_NUM(TBL_EXT_INFO) ; i++, pTbl++)
			{
				if ((info->hActive == pTbl->hAct_info) && (info->vActive == pTbl->vAct_info) && (info->scanType== pTbl->scan_info) )
				{
					bufTiming.hActive 		= pTbl->hAct_buf;
					bufTiming.vActive 		= pTbl->vAct_buf;
					bufTiming.scanType	= pTbl->scan_buf;
					bufTiming.extInfo		= pTbl->extInfo_buf;
					HDMI_VIDEO(" check Extend TimingInfo for NO VSI \n");
					break;
				}
			}
			break;
		}

		/**< HDMI VSI info */
		LINK_M14_RdFL(packet_76);
		tmp32 = LINK_M14_Rd(packet_76);
		eVideoFormat = ((tmp32 &0xff))>> 5;

		if (eVideoFormat == LX_FORMAT_EXTENDED_RESOLUTION_FORMAT)		// 4Kx2K
		{
			bufTiming.extInfo = LX_HDMI_EXT_4K_2K;				///< 4kx2K format
		}
		else if (eVideoFormat == LX_FORMAT_3D_FORMAT)
		{
			e3DStructure = ((tmp32 &0xff00)>>8)>> 4;

			p3DTbl = &EXT_3D_TYPE[0];
			for (i = 0; i < TBL_NUM(EXT_3D_TYPE) ; i++, p3DTbl++)
			{
				if (e3DStructure == p3DTbl->PreEnum)
				{
					bufTiming.extInfo = p3DTbl->PostEnum;
					break;
				}
				bufTiming.extInfo = p3DTbl->PostEnum;
			}
		}
		else //if (eVideoFormat == LX_FORMAT_NO_ADDITIONAL_FORMAT)
			break;

		switch(bufTiming.extInfo)		//if (eVideoFormat == LX_FORMAT_3D_FORMAT)
		{
			case LX_HDMI_EXT_3D_FRAMEPACK:
			{
				// Russia  STB Issue
				// VSI Info is 3D F/P and Source is 2D
				if ( (bufTiming.hActive == 640 && bufTiming.vActive == 480) \
					|| (bufTiming.hActive == 720 && bufTiming.vActive == 480) \
					|| (bufTiming.hActive == 720 && bufTiming.vActive == 576) \
					|| (bufTiming.hActive == 1280 && bufTiming.vActive == 720) \
					|| (bufTiming.hActive == 1920 && bufTiming.vActive == 1080) )
				{
					bufTiming.extInfo = LX_HDMI_EXT_2D_FORMAT;		///< 2D format
					HDMI_VIDEO("[%d] %s : 3D info is F/P, but Timing Info is 2D format for Russia STB Issue \n",  __L__, __F__);
					break;
				}
#if 1
				if ( (bufTiming.vActive == 2228) ||(bufTiming.vActive == 1028) )		//1080i & 480i
				{
					bufTiming.scanType = 0;
					tmp8 = 68; //23+22+23
					bufTiming.vActive = (bufTiming.vActive - tmp8) >> 1;
				}
				else if (bufTiming.vActive == 1226)		//576i
				{
					bufTiming.scanType = 0;
					tmp8 = 74; // 25+24+25
					bufTiming.vActive = (bufTiming.vActive - tmp8) >> 1;
				}
				else if (bufTiming.vActive == 1103)		// interace
				{
					bufTiming.scanType = 0;
					tmp8 = 23; //23
					bufTiming.vActive = bufTiming.vActive - tmp8;
				}
				else			//progressive
				{
					tmp8 = (bufTiming.vTotal - bufTiming.vActive);
					bufTiming.vActive = (bufTiming.vActive - tmp8) >> 1;
				}

#else
				/// detected progressive for Frame Packing Interlace format, check scan type.
				LINK_M14_RdFL(packet_20);		//AVI
				tmp32 = LINK_M14_Rd(packet_20);
				videoIdCode = (((tmp32 &0xff)) &0x7f);				// VIC6~VIC0

				if (videoIdCode == 5 || videoIdCode == 6 ||videoIdCode == 7 ||videoIdCode == 10 || videoIdCode == 11 ||
					videoIdCode == 20 || videoIdCode == 21 ||videoIdCode == 22 || videoIdCode == 25 || videoIdCode == 26 ||
					videoIdCode == 39 || videoIdCode == 40 || videoIdCode == 44 || videoIdCode == 45)		// 1080!@60Hz & 1080!@50Hz
					bufTiming.scanType = 0;

				HDMI_VIDEO(" VIC  = %d \n", videoIdCode);

				if (videoIdCode == 0)
				{
					if ( (bufTiming.vActive == 2228) ||(bufTiming.vActive == 1028) )		//1080i & 480i
					{
						bufTiming.scanType = 0;
						tmp8 = 68; //23+22+23
						bufTiming.vActive = (bufTiming.vActive - tmp8) >> 1;
					}
					else if (bufTiming.vActive == 1226)	//576i
					{
						bufTiming.scanType = 0;
						tmp8 = 74; //23+22+23
						bufTiming.vActive = (bufTiming.vActive - tmp8) >> 1;
					}
				}
				else
				{
					if (bufTiming.scanType)		//progressive
					{
						tmp8 = (bufTiming.vTotal - bufTiming.vActive);
						bufTiming.vActive = (bufTiming.vActive - tmp8) >> 1;
					}
					else					//interlace
					{
						if ( (bufTiming.vActive == 2228) ||(bufTiming.vActive == 1028) )		//1080i & 480i
						{
							tmp8 = 68; //23+22+23
							bufTiming.vActive = (bufTiming.vActive - tmp8) >> 1;
						}
						else if (bufTiming.vActive == 1226)		//576i
						{
							tmp8 = 74; // 25+24+25
							bufTiming.vActive = (bufTiming.vActive - tmp8) >> 1;
						}
						else if (bufTiming.vActive == 1103)
						{
							tmp8 = 23; //23
							bufTiming.vActive = bufTiming.vActive - tmp8;
						}
					}
				}
#endif
			} break;

			case LX_HDMI_EXT_3D_SBSFULL:
				bufTiming.hActive = bufTiming.hActive >> 1;
				break;

			case LX_HDMI_EXT_3D_FIELD_ALTERNATIVE:
			{
				bufTiming.scanType = 0;

				if (bufTiming.vActive == 1103)		tmp8 = 23; //1080! - 22.5
				else if (bufTiming.vActive == 601) 	tmp8 = 25; //576! - 24.5
				else if (bufTiming.vActive == 503)	tmp8 = 23; //480! - 22.5
				else 								tmp8 = 0;

				bufTiming.vActive = bufTiming.vActive - tmp8;

			} break;

			case LX_HDMI_EXT_3D_LINE_ALTERNATIVE:
				bufTiming.vActive = bufTiming.vActive >> 1;
				break;

			case LX_HDMI_EXT_2D_FORMAT:
			case LX_HDMI_EXT_3D_L_DEPTH:
			case LX_HDMI_EXT_3D_L_GRAPHICS:
			case LX_HDMI_EXT_3D_TNB:
			case LX_HDMI_EXT_3D_SBS:
			case LX_HDMI_EXT_4K_2K:
			case LX_HDMI_EXT_MAX:
			default:
				break;
		}

		/* Not support 3D 1920x1080!@50 of EIA-861D */
		if (eVideoFormat == LX_FORMAT_3D_FORMAT && (bufTiming.hTotal == 2304 ||bufTiming.hTotal == 4608))
		{
			memset(&bufTiming , 0 , sizeof(LX_HDMI_TIMING_INFO_T));
			HDMI_VIDEO("[%d] %s : Not support 3D 1920x1080i@50(1250)  \n",  __L__, __F__);
		}
	} while(0);

	if ( ( _gM14B0HDMI_thread_running == 0 &&  _gM14B0TimingReadCnt < 23 && _gM14B0TimingReadCnt > 20) ||
		(_gM14B0HDMIState >= HDMI_STATE_READ && _gM14B0TimingReadCnt < 6 && _gM14B0TimingReadCnt > 3) )
	{
		//HDMI_VIDEO(" VIC  = %d [gpreAVI - %d] \n", videoIdCode, __gM14B0PrevAVIPacket.eAviPacket.VideoIdCode);
		HDMI_VIDEO(" Timinginfo   buf hActive = %d // 	hActive = %d \n", bufTiming.hActive, info->hActive);
		HDMI_VIDEO(" Timinginfo   buf vActive = %d // 	vActive = %d \n", bufTiming.vActive, info->vActive);
		HDMI_VIDEO(" Timinginfo   buf scanType = %d //  scanType = %d \n", bufTiming.scanType, info->scanType);
		HDMI_VIDEO(" Timinginfo   buf full_3d = %d //  full_3d = %d \n", bufTiming.extInfo, info->extInfo);
		HDMI_VIDEO(" Timinginfo   Extend info([0]Normal[1]FP[2]FA[3]LA[4]SSF[5]LD[6]LDG[7]TNB[8]SBS[9]4K = %d \n", bufTiming.extInfo);
	}

	info->hTotal			= bufTiming.hTotal; 		///< Horizontal total pixels
	info->vTotal 			= bufTiming.vTotal; 		///< Vertical total lines
	info->hStart			= bufTiming.hStart; 		///< Horizontal start pixel (Back Porch)
	info->vStart 			= bufTiming.vStart;		///< Vertical start lines (Back Porch)
	info->hActive 		= bufTiming.hActive;		///< Horizontal active pixel
	info->vActive 		= bufTiming.vActive; 		///< Vertical active lines
	info->scanType 		= bufTiming.scanType; 		///< Scan type (0 : interlace, 1 : progressive) 	info->scanType ^= 1;
	info->extInfo			= bufTiming.extInfo; 		///< Ext format info

	HDMI_PRINT("[%d] %s : VideoFormat[%d]  3D Structure[%d] \n",  __L__, __F__, eVideoFormat, e3DStructure);

	return RET_OK;
}

static int _HDMI_M14B0_SetInternalMute(LX_HDMI_MUTE_CTRL_T interMute)
{
	BOOLEAN 	videoMuteState;

	LINK_M14_RdFL(system_control_01);
	LINK_M14_Rd01(system_control_01, reg_mute_vid, videoMuteState);

	//video related.
	if (interMute.eMode == LX_HDMI_VIDEO_MUTE || interMute.eMode == LX_HDMI_AV_MUTE)
	{
		if (interMute.bVideoMute != videoMuteState)
		{
			LINK_M14_Wr01(system_control_01, reg_mute_vid, interMute.bVideoMute);
			LINK_M14_WrFL(system_control_01);
			HDMI_VIDEO("[%d] %s : bVideoMute = %s \n", __L__, __F__, (interMute.bVideoMute ? "On" : "Off"));
		}
		else
			HDMI_VIDEO("[%d] %s : skip bVideoMute = %s \n", __L__, __F__, (interMute.bVideoMute ? "On" : "Off"));
	}

	//audio related.
	if (interMute.eMode == LX_HDMI_AUDIO_MUTE || interMute.eMode == LX_HDMI_AV_MUTE)
	{
		//Check a previous state
		if (_gM14B0AudioMuteState != interMute.bAudioMute)
		{
			if (interMute.bAudioMute == TRUE)
			{
				//Mute audio data
				LINK_M14_RdFL(audio_05);
				LINK_M14_Wr01(audio_05, reg_i2s_sd_en, 0x0);	//I2S SD Output Disable(4 Channel)
				LINK_M14_WrFL(audio_05);
			}
			else
			{
				//Un-mute audio data
				LINK_M14_RdFL(audio_05);
				LINK_M14_Wr01(audio_05, reg_i2s_sd_en, 0xF);	//I2S SD Output Enable(4 Channel)
				LINK_M14_WrFL(audio_05);
			}

			//Update _gM14B0AudioMuteState
			_gM14B0AudioMuteState = interMute.bAudioMute;

			HDMI_AUDIO("[%d] %s : bAudioMute = %s \n", __L__, __F__, (interMute.bAudioMute ? "On" : "Off"));
		}
	}

	return RET_OK;
}

static int _HDMI_M14B0_SetVideoBlank(LX_HDMI_AVI_COLORSPACE_T space)
{
	UINT32 blank_r = 0x0;
	UINT32 blank_b = 0x0;

	switch(space)
	{
		case LX_HDMI_AVI_COLORSPACE_YCBCR422:
		case LX_HDMI_AVI_COLORSPACE_YCBCR444:
		{
			blank_r = 0x800;
			blank_b = 0x800;
		}	break;

		case LX_HDMI_AVI_COLORSPACE_RGB:
		default:
		{
			blank_r = 0x0;
			blank_b = 0x0;
		}	break;
	}

	/* Blank Red */
	LINK_M14_RdFL(video_09);
	LINK_M14_Wr01(video_09, reg_vid_blank_r, blank_r);	//ACR Enable(Audio Clock Generation Function Activation)
	LINK_M14_WrFL(video_09);

	/* Blank Blue */
	LINK_M14_RdFL(video_10);
	LINK_M14_Wr01(video_10, reg_vid_blank_b, blank_b);	//ACR Enable(Audio Clock Generation Function Activation)
	LINK_M14_WrFL(video_10);

	HDMI_VIDEO("[%d] %s : changed video blank color space [%d]  \n",  __L__, __F__, space);
	return RET_OK;
}

static int _HDMI_M14B0_ClearSWResetAll(void)
{
	CTOP_CTRL_M14B0_RdFL(DEI, ctr02);
	CTOP_CTRL_M14B0_Wr01(DEI, ctr02, swrst_hdmi_dto, 0x1);
	CTOP_CTRL_M14B0_Wr01(DEI, ctr02, swrst_sys, 0x1);
	CTOP_CTRL_M14B0_Wr01(DEI, ctr02, swrst_hdcp, 0x1);
	CTOP_CTRL_M14B0_Wr01(DEI, ctr02, swrst_tmds, 0x1);
	CTOP_CTRL_M14B0_Wr01(DEI, ctr02, swrst_tmds_sel, 0x1);
	CTOP_CTRL_M14B0_Wr01(DEI, ctr02, swrst_vfifo_w, 0x1);
	CTOP_CTRL_M14B0_Wr01(DEI, ctr02, swrst_afifo_w, 0x1);
	CTOP_CTRL_M14B0_Wr01(DEI, ctr02, swrst_pix_pip, 0x1);
	CTOP_CTRL_M14B0_Wr01(DEI, ctr02, swrst_vfifo_r, 0x1);
	CTOP_CTRL_M14B0_Wr01(DEI, ctr02, swrst_aud, 0x1);
	CTOP_CTRL_M14B0_Wr01(DEI, ctr02, swrst_afifo_r, 0x1);
	CTOP_CTRL_M14B0_Wr01(DEI, ctr02, swrst_edid, 0x0);
	CTOP_CTRL_M14B0_WrFL(DEI, ctr02);

	OS_MsecSleep(1);	// ms delay

	CTOP_CTRL_M14B0_RdFL(DEI, ctr02);
	CTOP_CTRL_M14B0_Wr01(DEI, ctr02, swrst_hdmi_dto, 0x0);
	CTOP_CTRL_M14B0_Wr01(DEI, ctr02, swrst_sys, 0x0);
	CTOP_CTRL_M14B0_Wr01(DEI, ctr02, swrst_hdcp, 0x0);
	CTOP_CTRL_M14B0_Wr01(DEI, ctr02, swrst_tmds, 0x0);
	CTOP_CTRL_M14B0_Wr01(DEI, ctr02, swrst_tmds_sel, 0x0);
	CTOP_CTRL_M14B0_Wr01(DEI, ctr02, swrst_vfifo_w, 0x0);
	CTOP_CTRL_M14B0_Wr01(DEI, ctr02, swrst_afifo_w, 0x0);
	CTOP_CTRL_M14B0_Wr01(DEI, ctr02, swrst_pix_pip, 0x0);
	CTOP_CTRL_M14B0_Wr01(DEI, ctr02, swrst_vfifo_r, 0x0);
	CTOP_CTRL_M14B0_Wr01(DEI, ctr02, swrst_aud, 0x0);
	CTOP_CTRL_M14B0_Wr01(DEI, ctr02, swrst_afifo_r, 0x0);
	CTOP_CTRL_M14B0_Wr01(DEI, ctr02, swrst_edid, 0x1);		// release reset
	CTOP_CTRL_M14B0_WrFL(DEI, ctr02);

	OS_MsecSleep(1);	// ms delay

	/* select HDMI PHY i2c clock : 0(3Mhz), 1(24Mhz) */
	CTOP_CTRL_M14B0_RdFL(TOP, ctr06);
	CTOP_CTRL_M14B0_Wr01(TOP, ctr06,phy_i2c_clk_sel,1);
	CTOP_CTRL_M14B0_WrFL(TOP, ctr06);

	return RET_OK;
}

static int _HDMI_M14B0_SetPortConnection(void)
{
	ULONG	flags = 0;
	static int port_num;
	static int count_port_change = 0;
	UINT8 HDCP_AN_Data[8] ={0,} ;
	static UINT8 *Prev_HDCP_AN_Data;

	if (gHDMI_Phy_Control.port_change_start)	// port change detected
	{
		port_num = gHDMI_Phy_Control.port_to_change;
		spin_lock_irqsave(&_gIntrHdmiM14B0VideoLock, flags);
		gHDMI_Phy_Control.port_change_start = 0;
		spin_unlock_irqrestore(&_gIntrHdmiM14B0VideoLock, flags);

		if (port_num == 0)
			Prev_HDCP_AN_Data = gHDMI_Phy_Control.AN_Data_Prt0;
		else if (port_num == 1)
			Prev_HDCP_AN_Data = gHDMI_Phy_Control.AN_Data_Prt1;
		else if (port_num == 2)
			Prev_HDCP_AN_Data = gHDMI_Phy_Control.AN_Data_Prt2;
		else if (port_num == 3)
			Prev_HDCP_AN_Data = gHDMI_Phy_Control.AN_Data_Prt3;
		else
			return RET_ERROR;

		HDMI_TRACE("Port Chnage Sequence port num[%d] count_port_change[%d]\n", port_num, count_port_change);

		count_port_change = 300;	// init an data check count

		HDMI_DEBUG("Port #[%d] PREV AN[0x%02x%02x%02x%02x%02x%02x%02x%02x]\n",\
				port_num, Prev_HDCP_AN_Data[0], Prev_HDCP_AN_Data[1], Prev_HDCP_AN_Data[2], Prev_HDCP_AN_Data[3],\
				Prev_HDCP_AN_Data[4], Prev_HDCP_AN_Data[5], Prev_HDCP_AN_Data[6], Prev_HDCP_AN_Data[7]);
	}

	if (count_port_change > 0)
	{
		_HDMI_M14B0_Get_AN_Data(port_num, HDCP_AN_Data);
		HDMI_DEBUG("Port #[%d] AN[0x%02x%02x%02x%02x%02x%02x%02x%02x], count[%d]\n",\
				port_num, HDCP_AN_Data[0], HDCP_AN_Data[1], HDCP_AN_Data[2], HDCP_AN_Data[3], HDCP_AN_Data[4], \
				HDCP_AN_Data[5], HDCP_AN_Data[6], HDCP_AN_Data[7], count_port_change);
		/*
		if ( memcmp(HDCP_AN_Data, HDCP_AN_Data_Zero, sizeof(HDCP_AN_Data) ) || (count_port_change == 1))
				*/
		if ( memcmp(HDCP_AN_Data, Prev_HDCP_AN_Data, sizeof(HDCP_AN_Data) ) || (count_port_change == 1))
		{
			if (count_port_change == 1)
				_HDMI_M14B0_HDCP_ResetPort(port_num);

			count_port_change = 1;
			_HDMI_M14B0_PhyOn(port_num);
		}
		count_port_change --;
	}
	return RET_OK;
}

static int _HDMI_M14B0_GetPortConnection(void)
{
	static UINT32 portCnt = 0;
	UINT32 scdt = 0;
	UINT32 ctrl_repeat = 0;
	UINT32 noVsi_Intra = 0;
	BOOLEAN mode = 0;
//	ULONG	flags = 0;
	LX_HDMI_MUTE_CTRL_T 		muteCtrl = {FALSE, FALSE, LX_HDMI_VIDEO_MUTE};
	int	port_num;
	UINT32 temp;
	UINT32 up_freq = 0,	down_freq = 0;
	int	access_phy[4];
	UINT32	hpd_out_polarity;
	int i;
	static int tmds_clock_abnormal_count = 0;
	unsigned int elasped_time_eq;
	static unsigned int cs_man_changed_time_eq;
	unsigned int elasped_2nd_time_eq;
	unsigned int current_time_eq;

	static BOOLEAN prev_hdmi_mode_prt0 = 0;
	static BOOLEAN prev_hdmi_mode_prt1 = 0;
	static BOOLEAN prev_hdmi_mode_prt2 = 0;
	static BOOLEAN prev_hdmi_mode_prt3 = 0;
//	int hdcp_status;
//	UINT8 aksv_data[5];
	/*
	UINT32 hdcp_ri_0, hdcp_ri_1, hdcp_ri_all;
	static UINT32 prev_hdcp_ri_all;
	UINT8 HDCP_AN_Data[8] ={0,} ;
	static UINT8 Prev_HDCP_AN_Data[8] ={0,} ;

	LINK_M14_RdFL(hdcp_05);
	LINK_M14_Rd01(hdcp_05, reg_hdcp_ri_0_prt0, hdcp_ri_0);
	LINK_M14_Rd01(hdcp_05, reg_hdcp_ri_1_prt0, hdcp_ri_1);

	hdcp_ri_all = hdcp_ri_1 << 8 | hdcp_ri_1;

	if (hdcp_ri_all != prev_hdcp_ri_all)
	{
		HDMI_DEBUG("---- HDMI_RI CHG [0x%x] => [0x%x]\n", prev_hdcp_ri_all, hdcp_ri_all);
		prev_hdcp_ri_all = hdcp_ri_all;
	}
	_HDMI_M14B0_Get_AN_Data(0, HDCP_AN_Data);
	if ( memcmp(Prev_HDCP_AN_Data, HDCP_AN_Data, sizeof(HDCP_AN_Data) ) )
	{
		HDMI_DEBUG("---- HDMI_CHG AN[0x%02x%02x%02x%02x%02x%02x%02x%02x]\n",\
			HDCP_AN_Data[0], HDCP_AN_Data[1], HDCP_AN_Data[2], HDCP_AN_Data[3], HDCP_AN_Data[4], \
			HDCP_AN_Data[5], HDCP_AN_Data[6], HDCP_AN_Data[7]);
		memcpy(Prev_HDCP_AN_Data, HDCP_AN_Data, sizeof(HDCP_AN_Data));
	}
	*/
	_HDMI_M14B0_Get_HPD_Out_A4P(&_gM14B0HDMIPhyInform.hpd_out[0], &_gM14B0HDMIPhyInform.hpd_out[1] ,&_gM14B0HDMIPhyInform.hpd_out[2], &_gM14B0HDMIPhyInform.hpd_out[3]);
	_HDMI_M14B0_Get_HDMI5V_Info_A4P(&_gM14B0HDMIPhyInform.hdmi5v[0], &_gM14B0HDMIPhyInform.hdmi5v[1] ,&_gM14B0HDMIPhyInform.hdmi5v[2], &_gM14B0HDMIPhyInform.hdmi5v[3]);

//	if ( memcmp(_gM14B0HDMIPhyInform.hpd_out, _gM14B0HDMIPhyInform.hdmi5v, sizeof(_gM14B0HDMIPhyInform.hdmi5v)) )
	if ( ( _gM14B0HDMIPhyInform.hpd_out[0] != !(_gM14B0HDMIPhyInform.hdmi5v[0] ^ _gM14B0HDMIPhyInform.hpd_pol[0]) )\
	|| ( _gM14B0HDMIPhyInform.hpd_out[1] != !(_gM14B0HDMIPhyInform.hdmi5v[1] ^ _gM14B0HDMIPhyInform.hpd_pol[1]) )\
	|| ( _gM14B0HDMIPhyInform.hpd_out[2] != !(_gM14B0HDMIPhyInform.hdmi5v[2] ^ _gM14B0HDMIPhyInform.hpd_pol[2]) )\
	|| ( _gM14B0HDMIPhyInform.hpd_out[3] != !(_gM14B0HDMIPhyInform.hdmi5v[3] ^ _gM14B0HDMIPhyInform.hpd_pol[3]) ) )
	{
		for (port_num = 0; port_num < 4;port_num ++)
		{
#ifdef HPD_OFF_WORKAROUND_FOR_EXT_EDID
			if( (port_num == 1) && ( _gM14B0HDMIPhyInform.hdmi5v[1] == 1) &&  ( _g_HPD_Off_Time[1] != 0) )
			{
//				HDMI_DEBUG("---- HDMI_PHY:Port[%d] Ext EDID and cable connected !!! : HPD off", port_num);
				_g_HPD_On_Time[1] = jiffies_to_msecs(jiffies);
				_g_HPD_Elasped_Time[1] = _g_HPD_On_Time[1] - _g_HPD_Off_Time[1];
				if( _g_HPD_Elasped_Time[1] < 1500) 
					continue;
				else
				{
					_g_HPD_Off_Time[1] = 0;
					HDMI_DEBUG("---- HDMI_PHY:Port[%d] HPD off for [%u]msec\n", port_num, _g_HPD_Elasped_Time[1] );
				}
			}
#endif

			hpd_out_polarity = !( _gM14B0HDMIPhyInform.hdmi5v[port_num] ^ _gM14B0HDMIPhyInform.hpd_pol[port_num])  ;	// hpd_out considering polarity
			if ( _gM14B0HDMIPhyInform.hpd_out[port_num] != hpd_out_polarity )
			{
				_gM14B0HDMIPhyInform.tcs_done[port_num] = 0;

				if (!_gM14B0HDMIPhyInform.hdmi5v[port_num])		// Cable Disconnected !!!
				{
					HDMI_NOTI("---- HDMI_PHY:DisConn port[%d] = [%d] => [%d]\n", port_num, _gM14B0HDMIPhyInform.hpd_out[port_num], hpd_out_polarity);
//					if (gHDMI_Phy_Control.link_reset_control)
//						_HDMI_M14B0_TMDS_HDCP_ResetPort(port_num);
						_HDMI_M14B0_HDCP_ResetPort(port_num);
						_HDMI_M14B0_TMDS_ResetPort_Control(port_num ,0);

					if (gHDMI_Phy_Control.all_port_pdb_enable)
						_HDMI_M14B0_PhyOff(port_num);

					_HDMI_M14B0_Set_HPD_Out(port_num, hpd_out_polarity);	// disable HPD

#ifdef M14_HUMAX_SETTOP_HDCP_WORKAROUND
					/* HUMAX IR1020HD : snow noise when resolution change */
					/* Restore Default values for detached port */
					_HDMI_M14B0_Set_HDCP_Unauth(port_num, 1, 1);
#endif
				}
				else	// cable connected
				{
					_Port_Change_StartTime = jiffies_to_msecs(jiffies);

					//if (gHDMI_Phy_Control.all_port_pdb_enable && _gM14B0HDMIPhyInform.module_open)
					if (_gM14B0HDMIPhyInform.hpd_enable)
					{
						HDMI_NOTI("---- HDMI_PHY:Conn port[%d] = [%d] => [%d]\n", port_num, _gM14B0HDMIPhyInform.hpd_out[port_num], hpd_out_polarity);
						/* Phy Port Enable Can Be Changed HERE */

						/* All Phy Disabled except port_num */
						if (port_num != 0)
							_HDMI_M14B0_EnablePort(0, 0);
						if (port_num != 1)
							_HDMI_M14B0_EnablePort(1, 0);
						if (port_num != 2)
							_HDMI_M14B0_EnablePort(2, 0);
						if (port_num != 3)
							_HDMI_M14B0_EnablePort(3, 0);

						/* Enable port_num */
						_HDMI_M14B0_EnablePort(port_num, 1);

						_HDMI_M14B0_Set_HPD_Out(port_num, hpd_out_polarity);	// Enable HPD

						OS_MsecSleep(10); // delay after enable port and phy register write

						/* Phy ON & phy run-reset */
						_HDMI_M14B0_PhyOn_5V(port_num);

						/* if port_num is not current selected port , disable port */
						if (port_num != _gM14B0HDMIPhyInform.prt_sel)
							_HDMI_M14B0_EnablePort(port_num, 0);

						/* Enable selected port */
						_HDMI_M14B0_EnablePort(_gM14B0HDMIPhyInform.prt_sel, 1);

						if ( ( port_num == 3) && (_HDMI_M14B0_Get_ManMHLMode() == 1) && ( _gM14B0_HDMI_MHLAutoModeCnt == 0) )// MHL port
						{
							_gM14B0_HDMI_MHLAutoModeCnt = 20; // MHL Mode to HDMI for 400msec
						}
#if 0
#ifdef M14_CODE_FOR_MHL_CTS
#ifdef M14_CBUS_PDB_CTRL
						if ( (port_num == 3) && (_gM14B0HDMIPhyInform.cd_sense == 0) )
						{
							int phy_en_prt0, phy_en_prt1, phy_en_prt2, phy_en_prt3;
							HDMI_DEBUG("---- no CD_SENSE : HDMI Mode \n");

							LINK_M14_RdFL(phy_link_00);
							LINK_M14_Rd01(phy_link_00, phy_enable_prt0, phy_en_prt0);		//PHY Enable

							LINK_M14_RdFL(phy_link_02);
							LINK_M14_Rd01(phy_link_02, phy_enable_prt1, phy_en_prt1);		//PHY Enable

							LINK_M14_RdFL(phy_link_04);
							LINK_M14_Rd01(phy_link_04, phy_enable_prt2, phy_en_prt2);		//PHY Enable

							LINK_M14_RdFL(phy_link_06);
							LINK_M14_Rd01(phy_link_06, phy_enable_prt3, phy_en_prt3);		//PHY Enable

							if (!phy_en_prt3)	// IF port3(C-BUS) is not enabled?
							{
								_HDMI_M14B0_EnablePort(0, 0);
								_HDMI_M14B0_EnablePort(1, 0);
								_HDMI_M14B0_EnablePort(2, 0);
								_HDMI_M14B0_EnablePort(3, 1);
							}

							{
								/*
								HDMI_DEBUG("---- MHL Set ODT PDB Mode to 0x00\n");
								PHY_REG_M14B0_RdFL(eq_i2c_odt_pdb_mode);
								PHY_REG_M14B0_Wr01(eq_i2c_odt_pdb_mode,eq_i2c_odt_pdb,0x0);
								PHY_REG_M14B0_Wr01(eq_i2c_odt_pdb_mode,eq_i2c_odt_pdb_mode,0x0);
								PHY_REG_M14B0_WrFL(eq_i2c_odt_pdb_mode);
								*/
								HDMI_DEBUG("---- NO CD_SENSE for MHL,  PDB_D0_MAN_SEL :0x00, PDB_DCK_MAN_SEL :0x00\n");
								PHY_REG_M14B0_RdFL(pdb_d0_man_sel);
								PHY_REG_M14B0_Wr01(pdb_d0_man_sel,pdb_d0_man_sel,0x0);
								PHY_REG_M14B0_Wr01(pdb_d0_man_sel,pdb_d0_man,0x0);
								PHY_REG_M14B0_WrFL(pdb_d0_man_sel);

								PHY_REG_M14B0_RdFL(pdb_dck_man_sel);
								PHY_REG_M14B0_Wr01(pdb_dck_man_sel,pdb_dck_man_sel,0x0);
								PHY_REG_M14B0_Wr01(pdb_dck_man_sel,pdb_dck_man,0x0);
								PHY_REG_M14B0_WrFL(pdb_dck_man_sel);

							}

							if (!phy_en_prt3)
							{
								_HDMI_M14B0_EnablePort(0, phy_en_prt0);
								_HDMI_M14B0_EnablePort(1, phy_en_prt1);
								_HDMI_M14B0_EnablePort(2, phy_en_prt2);
								_HDMI_M14B0_EnablePort(3, phy_en_prt3);
							}
						}
#endif
#endif
#endif
					}
					else
						HDMI_TASK("HDMI module is not enabled !!!\n");
				}
//				_gM14B0HDMIPhyInform.hpd_out[port_num] = _gM14B0HDMIPhyInform.hdmi5v[port_num];
			}
		}

		//memcpy(_gM14B0HDMIPhyInform.hpd_out, _gM14B0HDMIPhyInform.hdmi5v, sizeof(_gM14B0HDMIPhyInform.hdmi5v));

		//_HDMI_M14B0_Set_HPD_Out_A4P(_gM14B0HDMIPhyInform.hpd_out[0], _gM14B0HDMIPhyInform.hpd_out[1],_gM14B0HDMIPhyInform.hpd_out[2], _gM14B0HDMIPhyInform.hpd_out[3]);
	}

	LINK_M14_RdFL(system_control_00);
	LINK_M14_Rd01(system_control_00, reg_prt_sel, _gM14B0HDMIPhyInform.prt_sel);

	LINK_M14_RdFL(phy_link_00);
	LINK_M14_Rd01(phy_link_00, phy_pdb_prt0, _gM14B0HDMIPhyInform.phy_pdb[0]);			//PHY PDB ON
	LINK_M14_Rd01(phy_link_00, phy_enable_prt0, _gM14B0HDMIPhyInform.phy_enable[0]);		//PHY Enable
	LINK_M14_Rd01(phy_link_00, phy_rstn_prt0, _gM14B0HDMIPhyInform.phy_rstn[0]);			//PHY RESET
	LINK_M14_Rd01(phy_link_00, hpd_in_prt0, _gM14B0HDMIPhyInform.hpd_in[0]);			//PHY HPD IN

	LINK_M14_RdFL(phy_link_02);
	LINK_M14_Rd01(phy_link_02, phy_pdb_prt1, _gM14B0HDMIPhyInform.phy_pdb[1]);			//PHY PDB ON
	LINK_M14_Rd01(phy_link_02, phy_enable_prt1, _gM14B0HDMIPhyInform.phy_enable[1]);		//PHY Enable
	LINK_M14_Rd01(phy_link_02, phy_rstn_prt1, _gM14B0HDMIPhyInform.phy_rstn[1]);			//PHY RESET
	LINK_M14_Rd01(phy_link_02, hpd_in_prt1, _gM14B0HDMIPhyInform.hpd_in[1]);			//PHY HPD IN

	LINK_M14_RdFL(phy_link_04);
	LINK_M14_Rd01(phy_link_04, phy_pdb_prt2, _gM14B0HDMIPhyInform.phy_pdb[2]);			//PHY PDB ON
	LINK_M14_Rd01(phy_link_04, phy_enable_prt2, _gM14B0HDMIPhyInform.phy_enable[2]);		//PHY Enable
	LINK_M14_Rd01(phy_link_04, phy_rstn_prt2, _gM14B0HDMIPhyInform.phy_rstn[2]);			//PHY RESET
	LINK_M14_Rd01(phy_link_04, hpd_in_prt2, _gM14B0HDMIPhyInform.hpd_in[2]);			//PHY HPD IN

	LINK_M14_RdFL(phy_link_06);
	LINK_M14_Rd01(phy_link_06, phy_pdb_prt3, _gM14B0HDMIPhyInform.phy_pdb[3]);			//PHY PDB ON
	LINK_M14_Rd01(phy_link_06, phy_enable_prt3, _gM14B0HDMIPhyInform.phy_enable[3]);		//PHY Enable
	LINK_M14_Rd01(phy_link_06, phy_rstn_prt3, _gM14B0HDMIPhyInform.phy_rstn[3]);			//PHY RESET
	LINK_M14_Rd01(phy_link_06, hpd_in_prt3, _gM14B0HDMIPhyInform.hpd_in[3]);			//PHY HPD IN

	LINK_M14_Rd01(system_control_00, reg_scdt_prt0, _gM14B0HDMIPhyInform.scdt[0]);
	LINK_M14_Rd01(system_control_00, reg_scdt_prt1, _gM14B0HDMIPhyInform.scdt[1]);
	LINK_M14_Rd01(system_control_00, reg_scdt_prt2, _gM14B0HDMIPhyInform.scdt[2]);
	LINK_M14_Rd01(system_control_00, reg_scdt_prt3, _gM14B0HDMIPhyInform.scdt[3]);

	LINK_M14_Rd01(system_control_00, reg_hdmi_mode_sel, _gM14B0HDMIPhyInform.hdmi_mode);

	for (i=0;i<4;i++)
		access_phy[i] = _gM14B0HDMIPhyInform.phy_pdb[i] && _gM14B0HDMIPhyInform.phy_enable[i] && _gM14B0HDMIPhyInform.phy_rstn[i] ;

	_HDMI_M14B0_Get_HPD_Out_A4P(&_gM14B0HDMIPhyInform.hpd_out[0], &_gM14B0HDMIPhyInform.hpd_out[1] ,&_gM14B0HDMIPhyInform.hpd_out[2], &_gM14B0HDMIPhyInform.hpd_out[3]);
	_HDMI_M14B0_Get_HDMI5V_Info_A4P(&_gM14B0HDMIPhyInform.hdmi5v[0], &_gM14B0HDMIPhyInform.hdmi5v[1] ,&_gM14B0HDMIPhyInform.hdmi5v[2], &_gM14B0HDMIPhyInform.hdmi5v[3]);

	if (access_phy[0] || access_phy[1] ||access_phy[2] ||access_phy[3] )
	{
		PHY_REG_M14B0_RdFL(tmds_freq_1);
		PHY_REG_M14B0_RdFL(tmds_freq_2);

		PHY_REG_M14B0_Rd01(tmds_freq_1,tmds_freq,up_freq);
		PHY_REG_M14B0_Rd01(tmds_freq_2,tmds_freq,down_freq);

		_gM14B0HDMIPhyInform.tmds_clock = ((up_freq << 8) + down_freq); 	// XXX.XX KHz

		if ( _gM14B0HDMIPhyInform.hpd_enable && (_gM14B0HDMIPhyInform.tmds_clock == 0) && (_gM14B0HDMIPhyInform.hdmi5v[_gM14B0HDMIPhyInform.prt_sel] ))	// hdmi 5v , not no tmds clock
		{
			int cr_done, cr_lock;

			PHY_REG_M14B0_RdFL(cr_lock_done);
			PHY_REG_M14B0_Rd01(cr_lock_done, cr_lock, cr_lock);
			PHY_REG_M14B0_Rd01(cr_lock_done, cr_done, cr_done);

			if (cr_lock && cr_done)	// no tmds clock, but CR lock done ????
				tmds_clock_abnormal_count++;
			else
				tmds_clock_abnormal_count = 0;

			if (tmds_clock_abnormal_count > 50)	//clock abnormal for long time
			{
				HDMI_NOTI("#########################################\n");
				HDMI_NOTI("#### CR LOCK DONE, but NO TMDS CLOCK ####\n");
				HDMI_NOTI("#### HPD out off/on Port [%d] ############\n", _gM14B0HDMIPhyInform.prt_sel);
				HDMI_NOTI("#########################################\n");

				_HDMI_M14B0_Set_HPD_Out(_gM14B0HDMIPhyInform.prt_sel, !_gM14B0HDMIPhyInform.hpd_pol[_gM14B0HDMIPhyInform.prt_sel]);	// disable HPD
				_HDMI_M14B0_Phy_Reset(_gM14B0HDMIPhyInform.prt_sel);

				OS_MsecSleep(100);
				/*
				PHY_REG_M14B0_RdFL(resetb_pdb_all);
				PHY_REG_M14B0_Wr01(resetb_pdb_all,resetb_all,0x0);		//active low reset
				PHY_REG_M14B0_WrFL(resetb_pdb_all);

				OS_MsecSleep(10);

				PHY_REG_M14B0_RdFL(resetb_pdb_all);
				PHY_REG_M14B0_Wr01(resetb_pdb_all,resetb_all,0x1);		//active low reset
				PHY_REG_M14B0_WrFL(resetb_pdb_all);
				*/

				tmds_clock_abnormal_count = 0;
			}
		}
		else
			tmds_clock_abnormal_count = 0;

		if( _g_abnormal_3d_format > 0)
		{
			HDMI_NOTI("#########################################\n");
			HDMI_NOTI("#### ABNORMAL 3D Format Detected     ####\n");
			HDMI_NOTI("#### HPD out off/on Port [%d] ############\n", _gM14B0HDMIPhyInform.prt_sel);
			HDMI_NOTI("#########################################\n");

			_HDMI_M14B0_Set_HPD_Out(_gM14B0HDMIPhyInform.prt_sel, !_gM14B0HDMIPhyInform.hpd_pol[_gM14B0HDMIPhyInform.prt_sel]);	// disable HPD

			PHY_REG_M14B0_RdFL(cr_mode_sel_resetb);
			PHY_REG_M14B0_Wr01(cr_mode_sel_resetb, cr_mode_sel_resetb,0x0);
			PHY_REG_M14B0_WrFL(cr_mode_sel_resetb);

			OS_MsecSleep(10);	// ms delay

			PHY_REG_M14B0_RdFL(cr_mode_sel_resetb);
			PHY_REG_M14B0_Wr01(cr_mode_sel_resetb, cr_mode_sel_resetb,0x1);
			PHY_REG_M14B0_WrFL(cr_mode_sel_resetb);

			OS_MsecSleep(100);
			/*
			   PHY_REG_M14B0_RdFL(resetb_pdb_all);
			   PHY_REG_M14B0_Wr01(resetb_pdb_all,resetb_all,0x0);		//active low reset
			   PHY_REG_M14B0_WrFL(resetb_pdb_all);

			   OS_MsecSleep(10);

			   PHY_REG_M14B0_RdFL(resetb_pdb_all);
			   PHY_REG_M14B0_Wr01(resetb_pdb_all,resetb_all,0x1);		//active low reset
			   PHY_REG_M14B0_WrFL(resetb_pdb_all);
			 */

			_g_abnormal_3d_format = 0;
		}

	}
	else
		_gM14B0HDMIPhyInform.tmds_clock = 0;

	if (_gM14B0HDMIPhyInform.prt_sel == 0)
	{
		LINK_M14_Rd01(system_control_00, reg_scdt_prt0, scdt);
		LINK_M14_Rd01(system_control_00, reg_hdmi_mode_prt0, mode);
		if ( prev_hdmi_mode_prt0 != mode)
		{
			HDMI_DEBUG("#### HDMI_Mode : Port [%d] Mode Changed [%d]=>[%d] \n",\
					_gM14B0HDMIPhyInform.prt_sel, prev_hdmi_mode_prt0, mode );
			prev_hdmi_mode_prt0 = mode;
		}
	}
	else if (_gM14B0HDMIPhyInform.prt_sel == 1)
	{
		LINK_M14_Rd01(system_control_00, reg_scdt_prt1, scdt);
		LINK_M14_Rd01(system_control_00, reg_hdmi_mode_prt1, mode);
		if ( prev_hdmi_mode_prt1 != mode)
		{
			HDMI_DEBUG("#### HDMI_Mode : Port [%d] Mode Changed [%d]=>[%d] \n",\
					_gM14B0HDMIPhyInform.prt_sel, prev_hdmi_mode_prt1, mode );
			prev_hdmi_mode_prt1 = mode;
		}
	}
	else if (_gM14B0HDMIPhyInform.prt_sel == 2)
	{
		LINK_M14_Rd01(system_control_00, reg_scdt_prt2, scdt);
		LINK_M14_Rd01(system_control_00, reg_hdmi_mode_prt2, mode);
		if ( prev_hdmi_mode_prt2 != mode)
		{
			HDMI_DEBUG("#### HDMI_Mode : Port [%d] Mode Changed [%d]=>[%d] \n",\
					_gM14B0HDMIPhyInform.prt_sel, prev_hdmi_mode_prt2, mode );
			prev_hdmi_mode_prt2 = mode;
		}
	}
	else
	{
		LINK_M14_Rd01(system_control_00, reg_scdt_prt3, scdt);
		LINK_M14_Rd01(system_control_00, reg_hdmi_mode_prt3, mode);
		if ( prev_hdmi_mode_prt3 != mode)
		{
			HDMI_DEBUG("#### HDMI_Mode : Port [%d] Mode Changed [%d]=>[%d] \n",\
					_gM14B0HDMIPhyInform.prt_sel, prev_hdmi_mode_prt3, mode );
			prev_hdmi_mode_prt3 = mode;
		}
	}
	//LINK_M14_Rd01(system_control_00, reg_scdt, scdt);
	//LINK_M14_Rd01(system_control_00, reg_hdmi_mode, mode);

	for (port_num=0;port_num<4;port_num++)
	{
		/* Enabled SW Adaptive EQ for MHL */
		/*
		if ( (port_num ==3) && ( _gM14B0HDMIPhyInform.cd_sense) )	// for MHL
			break;
			*/

		if ( _SCDT_Fall_Detected[port_num] )
		{
			//_HDMI_M14B0_HDCP_ResetPort(port_num);
			 _g_tcs_min_max_zero_count[port_num] = 0;

			/* Polling SCDT sometimes miss detection of short SDCT fall */
			if( _gM14B0HDMIPhyInform.prt_sel == port_num)
				scdt = 0;

			_gM14B0HDMIPhyInform.tcs_done[port_num] = 0;
			_SCDT_Fall_Detected[port_num] = 0;
			if (_gM14B0HDMIPhyInform.phy_pdb[port_num] && _gM14B0HDMIPhyInform.phy_rstn[port_num])
			{
				/* All Phy Disabled except port_num */
				if (port_num != 0)
					_HDMI_M14B0_EnablePort(0, 0);
				if (port_num != 1)
					_HDMI_M14B0_EnablePort(1, 0);
				if (port_num != 2)
					_HDMI_M14B0_EnablePort(2, 0);
				if (port_num != 3)
					_HDMI_M14B0_EnablePort(3, 0);

				/* Enable port_num */
				_HDMI_M14B0_EnablePort(port_num, 1);

				PHY_REG_M14B0_RdFL(eq_cs_rs_sel);
				PHY_REG_M14B0_Wr01(eq_cs_rs_sel, eq_cs_man_sel,0x0);
				PHY_REG_M14B0_WrFL(eq_cs_rs_sel);

				HDMI_NOTI("---- HDMI_PHY : port [%d] Sync Lost : Enable TCS \n",port_num );

				/* if port_num is not current selected port , disable port */
				if (port_num != _gM14B0HDMIPhyInform.prt_sel)
					_HDMI_M14B0_EnablePort(port_num, 0);

				/* Enable selected port */
				_HDMI_M14B0_EnablePort(_gM14B0HDMIPhyInform.prt_sel, 1);
			}
		}

		if ( _gM14B0HDMIPhyInform.hdmi5v[port_num] && (_gM14B0HDMIPhyInform.tcs_done[port_num] == 0) \
			&& _gM14B0HDMIPhyInform.phy_pdb[port_num] && _gM14B0HDMIPhyInform.phy_rstn[port_num])		// port detected but TCS not done yet
		{
			int tcs_done_value, tcs_min_value, tcs_max_value;

			//HDMI_DEBUG("---- HDMI_PHY : port [%d] TCS not done\n",port_num );

			/* All Phy Disabled except port_num */
			if (port_num != 0)
				_HDMI_M14B0_EnablePort(0, 0);
			if (port_num != 1)
				_HDMI_M14B0_EnablePort(1, 0);
			if (port_num != 2)
				_HDMI_M14B0_EnablePort(2, 0);
			if (port_num != 3)
				_HDMI_M14B0_EnablePort(3, 0);

			/* Enable port_num */
			_HDMI_M14B0_EnablePort(port_num, 1);

			PHY_REG_M14B0_RdFL(tcs_done);
			PHY_REG_M14B0_Rd01(tcs_done, tcs_done, tcs_done_value);

			if (tcs_done_value)
			{

				PHY_REG_M14B0_RdFL(tcs_min);
				PHY_REG_M14B0_Rd01(tcs_min, tcs_min, tcs_min_value);
				
				PHY_REG_M14B0_RdFL(tcs_max);
				PHY_REG_M14B0_Rd01(tcs_max, tcs_max, tcs_max_value);

		//		if( (tcs_min_value == 0) && (tcs_max_value == 0) && (_g_tcs_min_max_zero_count[port_num] < 10 ) )
				if( ( abs(tcs_max_value - tcs_min_value) <= 2 ) && (_g_tcs_min_max_zero_count[port_num] < 10 ) )	//140515 : MHL CT min/max almost same value
				{
					_g_tcs_min_max_zero_count[port_num] ++;

					//HDMI_NOTI("---- HDMI_PHY : port [%d] TCS Min/Max all zero [%d] !!!!! : Mode_Sel RESET !!! \n",port_num, _g_tcs_min_max_zero_count[port_num] );
					HDMI_NOTI("---- HDMI_PHY : port [%d] TCS Min[%d] ~= Max[%d] count [%d] !!!!! : Mode_Sel RESET !!! \n",port_num, tcs_min_value, tcs_max_value, _g_tcs_min_max_zero_count[port_num] );

					/* Continouse TMDS Error After eq_cs_man setting !!! : 15M Cable */
					PHY_REG_M14B0_RdFL(cr_mode_sel_resetb);
					PHY_REG_M14B0_Wr01(cr_mode_sel_resetb, cr_mode_sel_resetb,0x0);
					PHY_REG_M14B0_WrFL(cr_mode_sel_resetb);

					OS_MsecSleep(10);	// ms delay

					PHY_REG_M14B0_RdFL(cr_mode_sel_resetb);
					PHY_REG_M14B0_Wr01(cr_mode_sel_resetb, cr_mode_sel_resetb,0x1);
					PHY_REG_M14B0_WrFL(cr_mode_sel_resetb);

					OS_MsecSleep(30);	// ms delay
				}
				else
				{
					_g_tcs_min_max_zero_count[port_num] = 0;

					_gM14B0HDMIPhyInform.tcs_done[port_num] = tcs_done_value;

					//_gM14B0CHG_AVI_count_EQ = 0;
					_gM14B0_TMDS_ERROR_EQ = 0;
					_gM14B0_TMDS_ERROR_EQ_2nd = -1;

					//				PHY_REG_M14B0_RdFL(reg_tcs_man_mea_1);
					//				PHY_REG_M14B0_Wr01(reg_tcs_man_mea_1, reg_tcs_man_mea_1, 0x0);
					//				PHY_REG_M14B0_WrFL(reg_tcs_man_mea_1);

					PHY_REG_M14B0_RdFL(eq_cs_man);
					if (tcs_min_value + 10 < 0x1f)		// for 15m cable
					{
						if(tcs_min_value == 0)
							PHY_REG_M14B0_Wr01(eq_cs_man, eq_cs_man,tcs_min_value + 12);
						else if(tcs_min_value == 4)		// 140409 : BD370V-N 1080P60Hz noise (to set cs value to 0x10(from 0xe))
							PHY_REG_M14B0_Wr01(eq_cs_man, eq_cs_man,tcs_min_value + 12);
						else
							PHY_REG_M14B0_Wr01(eq_cs_man, eq_cs_man,tcs_min_value + 10);
					}
					else
						PHY_REG_M14B0_Wr01(eq_cs_man, eq_cs_man, 0x1f);

					PHY_REG_M14B0_WrFL(eq_cs_man);
					PHY_REG_M14B0_RdFL(eq_cs_rs_sel);
					PHY_REG_M14B0_Wr01(eq_cs_rs_sel, eq_cs_man_sel,0x3);
					PHY_REG_M14B0_WrFL(eq_cs_rs_sel);

					PHY_REG_M14B0_RdFL(reset_pdb_sel);
					PHY_REG_M14B0_Wr01(reset_pdb_sel, reset_sel,0x1);	// Reset Manual Control
					PHY_REG_M14B0_WrFL(reset_pdb_sel);

					PHY_REG_M14B0_RdFL(bert_dtb_resetb);
					PHY_REG_M14B0_Wr01(bert_dtb_resetb, dtb_resetb,0x0);	// Data Retimer/BERT/TMDS Decoder reset
					PHY_REG_M14B0_WrFL(bert_dtb_resetb);

					//OS_MsecSleep(5);	// ms delay

					PHY_REG_M14B0_RdFL(bert_dtb_resetb);
					PHY_REG_M14B0_Wr01(bert_dtb_resetb, dtb_resetb,0x1);	// Data Retimer/BERT/TMDS Decoder reset
					PHY_REG_M14B0_WrFL(bert_dtb_resetb);

					PHY_REG_M14B0_RdFL(reset_pdb_sel);
					PHY_REG_M14B0_Wr01(reset_pdb_sel, reset_sel,0x0);	// Reset Manual Control
					PHY_REG_M14B0_WrFL(reset_pdb_sel);

					//_HDMI_M14B0_Enable_TMDS_Error_Interrupt();
					_TCS_Done_Time[port_num] = jiffies_to_msecs(jiffies);

					_SCDT_Fall_Detected[port_num] = 0;	// clear SCDT interrupt

					_HDMI_M14B0_TMDS_ResetPort_Control(port_num ,1 );

					HDMI_DEBUG("---- HDMI_PHY : port [%d] TCS Done [%d]=>[%d]\n",port_num, _gM14B0HDMIPhyInform.tcs_done[port_num], tcs_done_value );
					if(tcs_min_value == 0)
						HDMI_NOTI("---- HDMI_PHY : port [%d] TCS MIN [0x%x]+12 => [0x%x]\n",port_num, tcs_min_value, tcs_min_value + 12 );
					else if(tcs_min_value == 4) // 140409 : BD370V-N 1080P60Hz noise (to set cs value to 0x10(from 0xe))
						HDMI_NOTI("---- HDMI_PHY : port [%d] TCS MIN [0x%x]+12 => [0x%x]\n",port_num, tcs_min_value, tcs_min_value + 12 );
					else
						HDMI_NOTI("---- HDMI_PHY : port [%d] TCS MIN [0x%x]+10 => [0x%x]\n",port_num, tcs_min_value, tcs_min_value + 10 );
					HDMI_NOTI("---- HDMI_PHY : port [%d] TCS MAX [0x%x]\n",port_num, tcs_max_value );
					HDMI_DEBUG("---- HDMI_PHY : DTB reset port [%d] \n",port_num);
				}
			}
			
			/* Restart TCS when TMDS Frequency is changes more than 100KHz : 140409 */

			PHY_REG_M14B0_RdFL(tmds_freq_1);
			PHY_REG_M14B0_RdFL(tmds_freq_2);

			PHY_REG_M14B0_Rd01(tmds_freq_1,tmds_freq,up_freq);
			PHY_REG_M14B0_Rd01(tmds_freq_2,tmds_freq,down_freq);

			_Curr_TMDS_Clock[port_num] = ((up_freq << 8) + down_freq); 	// XXX.XX KHz

			if( abs( _Curr_TMDS_Clock[port_num] - _Prev_TMDS_Clock[port_num]) > 10 ) //100kHz
			{
				HDMI_NOTI("---- HDMI_PHY port [%d] : Clock Changed [%d]=>[%d] \n", port_num, _Prev_TMDS_Clock[port_num], _Curr_TMDS_Clock[port_num] );
					_Prev_TMDS_Clock[port_num] =  _Curr_TMDS_Clock[port_num];

				if (_gM14B0HDMIPhyInform.tcs_done[port_num] == 1)
				{
					HDMI_NOTI("---- HDMI_PHY port [%d] : Restart TCS \n", port_num );
					PHY_REG_M14B0_RdFL(eq_cs_rs_sel);
					PHY_REG_M14B0_Wr01(eq_cs_rs_sel, eq_cs_man_sel,0x0);
					PHY_REG_M14B0_WrFL(eq_cs_rs_sel);

					PHY_REG_M14B0_RdFL(bert_tmds_sel);
					PHY_REG_M14B0_Wr01(bert_tmds_sel,tcs_en,0x0);
					PHY_REG_M14B0_WrFL(bert_tmds_sel);

					PHY_REG_M14B0_RdFL(bert_tmds_sel);
					PHY_REG_M14B0_Wr01(bert_tmds_sel,tcs_en,0x1);
					PHY_REG_M14B0_WrFL(bert_tmds_sel);

					_gM14B0HDMIPhyInform.tcs_done[port_num] = 0;

					_gM14B0_TMDS_ERROR_EQ = -1;
					_gM14B0_TMDS_ERROR_EQ_2nd = -1;
				}
			}

			/* if port_num is not current selected port , disable port */
			if (port_num != _gM14B0HDMIPhyInform.prt_sel)
				_HDMI_M14B0_EnablePort(port_num, 0);

			/* Enable selected port */
			_HDMI_M14B0_EnablePort(_gM14B0HDMIPhyInform.prt_sel, 1);

		}
	}

	if (_gM14B0_HDMI_MHLAutoModeCnt > 0)
	{
		_gM14B0_HDMI_MHLAutoModeCnt--;
		if ( _gM14B0_HDMI_MHLAutoModeCnt == 0)
		{
			_HDMI_M14B0_Set_ManMHLMode(0, 0);
			HDMI_DEBUG("---- HDMI_PHY : set MHL auto mode\n");
		}
	}

	for (port_num = 0; port_num < 4;port_num ++)
	{
		_HDMI_M14B0_Get_EDID_Rd(port_num , &_gM14B0HDMIPhyInform.edid_rd_done[port_num], &temp);
		_HDMI_M14B0_Get_HDCP_info(port_num , &_gM14B0HDMIPhyInform.hdcp_authed[port_num], &_gM14B0HDMIPhyInform.hdcp_enc_en[port_num]);
	}

	/*
	if (_gM14B0HDMIPhyInform.hdcp_authed[_gM14B0HDMIPhyInform.prt_sel] && _gM14B0HDMIPhyInform.hdcp_enc_en[_gM14B0HDMIPhyInform.prt_sel] )
		hdcp_status = HDMI_HDCP_AUTH_DONE;
	else
	{
		HDMI_M14B0_Get_Aksv_Data(_gM14B0HDMIPhyInform.prt_sel, aksv_data);
		if ( (memcmp(aksv_data, (void*){0,}, sizeof(aksv_data)) ) && _gM14B0HDMIPhyInform.hdcp_authed[_gM14B0HDMIPhyInform.prt_sel])
			memcmp
	}
	*/
	current_time_eq = jiffies_to_msecs(jiffies);
	elasped_time_eq =  current_time_eq - _TCS_Done_Time[_gM14B0HDMIPhyInform.prt_sel];

	if ( (elasped_time_eq > 3000) && ( ( _gM14B0_TMDS_ERROR_EQ != -1) || ( _gM14B0_TMDS_ERROR_EQ_2nd != -1) ) )
	{
//		_HDMI_M14B0_Disable_TMDS_Error_Interrupt();
		HDMI_DEBUG("---- HDMI_PHY Stable??? for [%d]msec : Disable ECC ERROR check\n", elasped_time_eq);
		HDMI_DEBUG("---- 1st EQ ERROR[%d], 2nd EQ ERROR[%d]", _gM14B0_TMDS_ERROR_EQ, _gM14B0_TMDS_ERROR_EQ_2nd);
		_gM14B0_TMDS_ERROR_EQ = -1;
		_gM14B0_TMDS_ERROR_EQ_2nd = -1;

	}

	_HDMI_M14B0_Enable_TMDS_Error_Interrupt();

	if(_gM14B0_TMDS_ERROR_intr_count > 0)
	{
		_gM14B0_TMDS_ERROR_intr_count = 0;
		_gM14B0_TMDS_ERROR_count ++;
		if(_gM14B0_TMDS_ERROR_count > 3)
		{
			HDMI_NOTI("!!! TMDS Errors [%d] : force scdt to zero !!!\n", _gM14B0_TMDS_ERROR_count);
			_gM14B0_TMDS_ERROR_count = 0;
			scdt = 0;
		}

	}
	else
		_gM14B0_TMDS_ERROR_count = 0;

	/*
	if ( ( elasped_time_eq < 5000 ) && (_gM14B0_TMDS_ERROR_EQ != -1) )
		_HDMI_M14B0_Enable_TMDS_Error_Interrupt();
		*/

	/* more than 8 ECC Error interrrupt occured during 1.8 second,
	   change EQ CS value to 0xf */
	//if ( (_gM14B0_TMDS_ERROR_EQ > 30) && ( elasped_time_eq < 3000 ) )
	//if ( (_gM14B0_TMDS_ERROR_EQ > 8) && ( elasped_time_eq < 1800 ) )
	if ( (_gM14B0_TMDS_ERROR_EQ > 16) && ( elasped_time_eq < 3000 ) )	// 140403
	{
		/* Enable port_num */
		_HDMI_M14B0_EnablePort(_gM14B0HDMIPhyInform.prt_sel, 1);

		PHY_REG_M14B0_RdFL(eq_cs_man);
		//PHY_REG_M14B0_Wr01(eq_cs_man, eq_cs_man, 0xf);
		PHY_REG_M14B0_Wr01(eq_cs_man, eq_cs_man, 0x11);		// 140403 : changed cs_man value for BP540 and 15M Cable
		PHY_REG_M14B0_WrFL(eq_cs_man);

		/* Continouse TMDS Error After eq_cs_man setting !!! : 15M Cable */
		PHY_REG_M14B0_RdFL(cr_mode_sel_resetb);
		PHY_REG_M14B0_Wr01(cr_mode_sel_resetb, cr_mode_sel_resetb,0x0);
		PHY_REG_M14B0_WrFL(cr_mode_sel_resetb);

		OS_MsecSleep(10);	// ms delay

		PHY_REG_M14B0_RdFL(cr_mode_sel_resetb);
		PHY_REG_M14B0_Wr01(cr_mode_sel_resetb, cr_mode_sel_resetb,0x1);
		PHY_REG_M14B0_WrFL(cr_mode_sel_resetb);

		OS_MsecSleep(30);	// ms delay

		_HDMI_M14B0_TMDS_ResetPort_Control(_gM14B0HDMIPhyInform.prt_sel ,1 );
		_SCDT_Fall_Detected[_gM14B0HDMIPhyInform.prt_sel] = 0;	// clear SCDT interrupt from above mode_sel reset

/*
		PHY_REG_M14B0_RdFL(reset_pdb_sel);
		PHY_REG_M14B0_Wr01(reset_pdb_sel, reset_sel,0x1);	// Reset Manual Control
		PHY_REG_M14B0_WrFL(reset_pdb_sel);

		PHY_REG_M14B0_RdFL(bert_dtb_resetb);
		PHY_REG_M14B0_Wr01(bert_dtb_resetb, dtb_resetb,0x0);	// Data Retimer/BERT/TMDS Decoder reset
		PHY_REG_M14B0_WrFL(bert_dtb_resetb);

		//OS_MsecSleep(5);	// ms delay

		PHY_REG_M14B0_RdFL(bert_dtb_resetb);
		PHY_REG_M14B0_Wr01(bert_dtb_resetb, dtb_resetb,0x1);	// Data Retimer/BERT/TMDS Decoder reset
		PHY_REG_M14B0_WrFL(bert_dtb_resetb);

		PHY_REG_M14B0_RdFL(reset_pdb_sel);
		PHY_REG_M14B0_Wr01(reset_pdb_sel, reset_sel,0x0);	// Reset Manual Control
		PHY_REG_M14B0_WrFL(reset_pdb_sel);
*/
//		_HDMI_M14B0_Disable_TMDS_Error_Interrupt();
		HDMI_NOTI("---- HDMI_PHY port[%d] : TMDS Error EQ intr [%d] in [%d]msec : EQ_CS_MAN to [0x%x]\n",_gM14B0HDMIPhyInform.prt_sel, _gM14B0_TMDS_ERROR_EQ, elasped_time_eq, 0x11 );

		_gM14B0_TMDS_ERROR_EQ = -1;
		_gM14B0_TMDS_ERROR_EQ_2nd = 0;
		cs_man_changed_time_eq = jiffies_to_msecs(jiffies);

//		OS_MsecSleep(10);	// ms delay
//		_SCDT_Fall_Detected[_gM14B0HDMIPhyInform.prt_sel] = 0;	// clear SCDT interrupt
	}

	if(_gM14B0_TMDS_ERROR_EQ_2nd != -1)
	{
		HDMI_DEBUG("---- HDMI 2ND EQ Check [%d]\n", _gM14B0_TMDS_ERROR_EQ_2nd);
		current_time_eq = jiffies_to_msecs(jiffies);
		elasped_2nd_time_eq =  current_time_eq - cs_man_changed_time_eq;
	}

	/* more than 4 ECC Error interrrupt occured during .9 second,
	   change EQ CS value to 0x6 */
	//if ( (_gM14B0_TMDS_ERROR_EQ_2nd > 4) && ( elasped_2nd_time_eq < 900 ) )
	if ( (_gM14B0_TMDS_ERROR_EQ_2nd > 8) && ( elasped_2nd_time_eq < 1800 ) )	//140404
	{
		int cs_man_value;
		
		/* Enable port_num */
		_HDMI_M14B0_EnablePort(_gM14B0HDMIPhyInform.prt_sel, 1);

		PHY_REG_M14B0_RdFL(eq_cs_man);
		PHY_REG_M14B0_Rd01(eq_cs_man, eq_cs_man, cs_man_value);
		
		for(;cs_man_value > 0x3; cs_man_value--)
		{
			PHY_REG_M14B0_RdFL(eq_cs_man);
			PHY_REG_M14B0_Wr01(eq_cs_man, eq_cs_man, cs_man_value);
			PHY_REG_M14B0_WrFL(eq_cs_man);
		}

		/* Continouse TMDS Error After eq_cs_man setting !!! : 15M Cable */
		PHY_REG_M14B0_RdFL(cr_mode_sel_resetb);
		PHY_REG_M14B0_Wr01(cr_mode_sel_resetb, cr_mode_sel_resetb,0x0);
		PHY_REG_M14B0_WrFL(cr_mode_sel_resetb);

		OS_MsecSleep(10);	// ms delay

		PHY_REG_M14B0_RdFL(cr_mode_sel_resetb);
		PHY_REG_M14B0_Wr01(cr_mode_sel_resetb, cr_mode_sel_resetb,0x1);
		PHY_REG_M14B0_WrFL(cr_mode_sel_resetb);

		OS_MsecSleep(30);	// ms delay

		_HDMI_M14B0_TMDS_ResetPort_Control(_gM14B0HDMIPhyInform.prt_sel ,1 );
		_SCDT_Fall_Detected[_gM14B0HDMIPhyInform.prt_sel] = 0;	// clear SCDT interrupt from above mode_sel reset

/*
		PHY_REG_M14B0_RdFL(reset_pdb_sel);
		PHY_REG_M14B0_Wr01(reset_pdb_sel, reset_sel,0x1);	// Reset Manual Control
		PHY_REG_M14B0_WrFL(reset_pdb_sel);

		PHY_REG_M14B0_RdFL(bert_dtb_resetb);
		PHY_REG_M14B0_Wr01(bert_dtb_resetb, dtb_resetb,0x0);	// Data Retimer/BERT/TMDS Decoder reset
		PHY_REG_M14B0_WrFL(bert_dtb_resetb);

		//OS_MsecSleep(5);	// ms delay

		PHY_REG_M14B0_RdFL(bert_dtb_resetb);
		PHY_REG_M14B0_Wr01(bert_dtb_resetb, dtb_resetb,0x1);	// Data Retimer/BERT/TMDS Decoder reset
		PHY_REG_M14B0_WrFL(bert_dtb_resetb);

		PHY_REG_M14B0_RdFL(reset_pdb_sel);
		PHY_REG_M14B0_Wr01(reset_pdb_sel, reset_sel,0x0);	// Reset Manual Control
		PHY_REG_M14B0_WrFL(reset_pdb_sel);
*/
//		_HDMI_M14B0_Disable_TMDS_Error_Interrupt();
		HDMI_NOTI("---- HDMI_PHY port[%d] : TMDS Error EQ 2ND intr [%d] in [%d]msec : EQ_CS_MAN to [0x%x]\n",_gM14B0HDMIPhyInform.prt_sel, _gM14B0_TMDS_ERROR_EQ_2nd, elasped_2nd_time_eq, cs_man_value+1 );

		_gM14B0_TMDS_ERROR_EQ = -1;
		_gM14B0_TMDS_ERROR_EQ_2nd = -1;

//		OS_MsecSleep(10);	// ms delay
//		_SCDT_Fall_Detected[_gM14B0HDMIPhyInform.prt_sel] = 0;	// clear SCDT interrupt

	}

	/* more than 5 avi change interrupt occured during 5second,
	   change EQ CS value to 0xf */
	if ( (_gM14B0CHG_AVI_count_EQ > 5) && ( elasped_time_eq < 5000 ) )
	{
		/* Enable port_num */
		_HDMI_M14B0_EnablePort(_gM14B0HDMIPhyInform.prt_sel, 1);

		PHY_REG_M14B0_RdFL(eq_cs_man);
		PHY_REG_M14B0_Wr01(eq_cs_man, eq_cs_man, 0xf);
		PHY_REG_M14B0_WrFL(eq_cs_man);

		HDMI_NOTI("---- HDMI_PHY port[%d] : AVI_CHG intr [%d] in [%d]msec : EQ_CS_MAN to [%d]\n",_gM14B0HDMIPhyInform.prt_sel, _gM14B0CHG_AVI_count_EQ, elasped_time_eq, 0xf );

		_gM14B0CHG_AVI_count_EQ = -1;
	}

	if (scdt == HDMI_PORT_CONNECTED)
	{
		if (portCnt != 0)		HDMI_DEBUG("[%d] %s : Connection \n", __LINE__, __func__);
		portCnt = 0;
	}
	else
	{
		mode = 0;
		if (portCnt < 3)
		{
			/*
			spin_lock_irqsave(&_gIntrHdmiM14B0VideoLock, flags);
			_gM14B0Intr_vf_stable = HDMI_VIDEO_INIT_STATE;
			_gM14B0Intr_avi = HDMI_AVI_INIT_STATE;
			_gM14B0Intr_vsi = HDMI_VSI_INIT_STATE;
			_gM14B0Intr_src_mute = HDMI_SOURCE_MUTE_CLEAR_STATE;
			_gM14B0TimingReadCnt = 0;
			_gM14B0AVIReadState = FALSE;
			_gM14B0VSIState = FALSE;
			_gM14B0AVIChangeState = FALSE;
			spin_unlock_irqrestore(&_gIntrHdmiM14B0VideoLock, flags);
			*/

			_HDMI_M14B0_SetInternalMute(muteCtrl);	// Video All Clear

			///* Reset phy pll for pixel repetition  *///
			CTOP_CTRL_M14B0_RdFL(DEI, ctr00);
			CTOP_CTRL_M14B0_Rd01(DEI, ctr00, phy_ppll_sel, ctrl_repeat);

			if (ctrl_repeat)
			{
				CTOP_CTRL_M14B0_Wr01(DEI, ctr00, phy_ppll_sel, 0x0);
				CTOP_CTRL_M14B0_WrFL(DEI, ctr00);
			}

			LINK_M14_RdFL(interrupt_enable_01);
			LINK_M14_Rd01(interrupt_enable_01, intr_no_vsi_enable, noVsi_Intra);			///< 20 intr_no_vsi_enable

			if (noVsi_Intra != 0)
			{
				LINK_M14_Wr01(interrupt_enable_01, intr_no_vsi_enable, 0x0);			///< 20 intr_no_vsi_enable
				LINK_M14_WrFL(interrupt_enable_01);
				HDMI_DEBUG("[%d] %s : No VSI intra disable \n", __LINE__, __func__);
			}

			HDMI_DEBUG("[%d] %s : Dis Connection  [%d] \n", __LINE__, __func__,   portCnt);
			portCnt++;
		}
	}

// 130926 : wonsik.do spin lock needed ???
//	spin_lock_irqsave(&_gIntrHdmiM14B0VideoLock, flags);
	_gM14B0HDMIConnectState = scdt;
	gM14BootData->mode = mode;
//	spin_unlock_irqrestore(&_gIntrHdmiM14B0VideoLock, flags);

	return RET_OK;
}

static int _HDMI_M14B0_PhyRunReset(void)
{
	int phy_enable_prt3, prt_selected, cd_sense;

	HDMI_DEBUG("[%s] Entered \n",__func__);

	PHY_REG_M14B0_RdFL(cr_mode_sel_resetb);
	PHY_REG_M14B0_Wr01(cr_mode_sel_resetb, cr_mode_sel_resetb,0x0);
	PHY_REG_M14B0_WrFL(cr_mode_sel_resetb);

	/* M14B0 New HDMI Phy Register Settings */
	PHY_REG_M14B0_RdFL(resetb_pdb_all);
	PHY_REG_M14B0_Wr01(resetb_pdb_all,resetb_all,0x1);
	PHY_REG_M14B0_WrFL(resetb_pdb_all);

	PHY_REG_M14B0_RdFL(dr_clk_inv_ch);
	PHY_REG_M14B0_Wr01(dr_clk_inv_ch,dr_clk_inv_ch0,0x1);
	PHY_REG_M14B0_Wr01(dr_clk_inv_ch,dr_clk_inv_ch1,0x1);
	PHY_REG_M14B0_Wr01(dr_clk_inv_ch,dr_clk_inv_ch2,0x1);
	PHY_REG_M14B0_WrFL(dr_clk_inv_ch);

	/* M14 New HDMI Phy Register Settings */
	PHY_REG_M14B0_RdFL(eq_cs_rs_sel);
	PHY_REG_M14B0_Wr01(eq_cs_rs_sel,eq_rs_man_sel,0x1);
#ifdef M14_HDMI_SEMI_AUTO_EQ_CONTROL
	PHY_REG_M14B0_Wr01(eq_cs_rs_sel,eq_cs_man_sel,0x1);
#endif
	PHY_REG_M14B0_WrFL(eq_cs_rs_sel);

#ifdef M14_HDMI_SEMI_AUTO_EQ_CONTROL
	PHY_REG_M14B0_RdFL(ck_mode_cs_man_00);
	PHY_REG_M14B0_Wr01(ck_mode_cs_man_00,ck_mode_cs_man_00,0x6);
	PHY_REG_M14B0_WrFL(ck_mode_cs_man_00);
	PHY_REG_M14B0_RdFL(ck_mode_cs_man_01);
	PHY_REG_M14B0_Wr01(ck_mode_cs_man_01,ck_mode_cs_man_01,0x8);
	PHY_REG_M14B0_WrFL(ck_mode_cs_man_01);
	PHY_REG_M14B0_RdFL(ck_mode_cs_man_10);
	PHY_REG_M14B0_Wr01(ck_mode_cs_man_10,ck_mode_cs_man_10,0xA);
	PHY_REG_M14B0_WrFL(ck_mode_cs_man_10);
	PHY_REG_M14B0_RdFL(ck_mode_cs_man_11);
	PHY_REG_M14B0_Wr01(ck_mode_cs_man_11,ck_mode_cs_man_11,0xC);
	PHY_REG_M14B0_WrFL(ck_mode_cs_man_11);
#endif

	// For edison room XBOX360
	PHY_REG_M14B0_RdFL(idr_adj);
#ifdef FOR_EDISON_XBOX360
	PHY_REG_M14B0_Wr01(idr_adj,idr_adj,0x3); //0x2
#else
	PHY_REG_M14B0_Wr01(idr_adj,idr_adj,0x2); //0x2
#endif
	PHY_REG_M14B0_WrFL(idr_adj);

#ifdef FOR_EDISON_XBOX360
	PHY_REG_M14B0_RdFL(cr_vbgr_ctrl);
	PHY_REG_M14B0_Wr01(cr_vbgr_ctrl,cr_vbgr_ctrl,0x5);	//140403 : OLED 15M Cable (from 0x4)
	PHY_REG_M14B0_WrFL(cr_vbgr_ctrl);

	PHY_REG_M14B0_RdFL(cr_tmr_scale);
	PHY_REG_M14B0_Wr01(cr_tmr_scale,cr_tmr_scale,0x02);
	PHY_REG_M14B0_WrFL(cr_tmr_scale);

	PHY_REG_M14B0_RdFL(eq_eval_time_1);
	PHY_REG_M14B0_Wr01(eq_eval_time_1,eq_eval_time_1,0xff);
	PHY_REG_M14B0_WrFL(eq_eval_time_1);

	PHY_REG_M14B0_RdFL(eq_eval_time_2);
	PHY_REG_M14B0_Wr01(eq_eval_time_2,eq_eval_time_2,0xff);
	PHY_REG_M14B0_WrFL(eq_eval_time_2);

	PHY_REG_M14B0_RdFL(eq_filter);
	PHY_REG_M14B0_Wr01(eq_filter,eq_filter,0x0f);	//0x1f : MSPG-4233 HDCP fail issue //0x3f // default 0x0f
	PHY_REG_M14B0_WrFL(eq_filter);
#endif
	// End of For edison room XBOX360

	PHY_REG_M14B0_RdFL(tmds_errec_detect);
	PHY_REG_M14B0_Wr01(tmds_errec_detect,tmds_errec_detect,0x1);
	PHY_REG_M14B0_WrFL(tmds_errec_detect);

	PHY_REG_M14B0_RdFL(bert_tmds_sel);
	PHY_REG_M14B0_Wr01(bert_tmds_sel,tcs_en,0x1);
	PHY_REG_M14B0_WrFL(bert_tmds_sel);

	PHY_REG_M14B0_RdFL(tmds_clk_inv);
	PHY_REG_M14B0_Wr01(tmds_clk_inv,tmds_clk_inv,0x1);
	PHY_REG_M14B0_WrFL(tmds_clk_inv);

	PHY_REG_M14B0_RdFL(reg_tcs_man_pre);
	PHY_REG_M14B0_Wr01(reg_tcs_man_pre,reg_tcs_man_pre,0x40);
	PHY_REG_M14B0_WrFL(reg_tcs_man_pre);

	/*
	PHY_REG_M14B0_RdFL(cr_mode_sel_resetb);
	PHY_REG_M14B0_Wr01(cr_mode_sel_resetb,cr_mode_sel_resetb,0x01);
	PHY_REG_M14B0_WrFL(cr_mode_sel_resetb);
	*/

	// set eq at 2012-11-17
	//PHY_REG_M14B0_RdFL(dr_n1);
	//PHY_REG_M14B0_Wr01(dr_n1,dr_n1,0x7);
	//PHY_REG_M14B0_WrFL(dr_n1);

	//PHY_REG_M14B0_RdFL(eq_rs);
	//PHY_REG_M14B0_Wr01(eq_rs,eq_rs,0x5);		//2012-11-17 change 3 -> 5
	//PHY_REG_M14B0_WrFL(eq_rs);

	//PHY_REG_M14B0_RdFL(cr_kvco_offset);
	//PHY_REG_M14B0_Wr01(cr_kvco_offset,cr_kvco_offset,0x1);		//2012-11-22 change 2 -> 1
	//PHY_REG_M14B0_WrFL(cr_kvco_offset);

/*	// HDCP Problem to the Master for L9 -- need cheak  : default value 0x1
	PHY_REG_M14B0_RdFL(eq_af_en_avg_width);
	PHY_REG_M14B0_Wr01(eq_af_en_avg_width,eq_avg_width,0x0);
	PHY_REG_M14B0_WrFL(eq_af_en_avg_width);
*/
	LINK_M14_RdFL(phy_link_06);
	LINK_M14_Rd01(phy_link_06, phy_enable_prt3, phy_enable_prt3);

	LINK_M14_RdFL(system_control_00);
	LINK_M14_Rd01(system_control_00, reg_prt_sel, prt_selected);
	LINK_M14_Rd01(system_control_00, reg_cd_sense_prt3, cd_sense);

	PHY_REG_M14B0_RdFL(dr_filter_ch0);
	PHY_REG_M14B0_RdFL(cr_icp_adj);
	PHY_REG_M14B0_RdFL(eq_rs_man);

#if 0
#ifdef M14_CODE_FOR_MHL_CTS
#ifdef M14_CBUS_PDB_CTRL
	if ( (gHDMI_Phy_Control.all_port_pdb_enable && phy_enable_prt3) || (!gHDMI_Phy_Control.all_port_pdb_enable && (prt_selected==0x3) ) )
	{
		/*
		   HDMI_DEBUG("---- MHL port : Set ODT PDB Mode to 0x10\n");
		   PHY_REG_M14B0_RdFL(eq_i2c_odt_pdb_mode);
		   PHY_REG_M14B0_Wr01(eq_i2c_odt_pdb_mode,eq_i2c_odt_pdb,0x0);
		   PHY_REG_M14B0_Wr01(eq_i2c_odt_pdb_mode,eq_i2c_odt_pdb_mode,0x1);
		   PHY_REG_M14B0_WrFL(eq_i2c_odt_pdb_mode);
		 */
		HDMI_DEBUG("---- PHY INIT for MHL,  PDB_D0_MAN_SEL :0x10, PDB_DCK_MAN_SEL :0x10\n");
		PHY_REG_M14B0_RdFL(pdb_d0_man_sel);
		PHY_REG_M14B0_Wr01(pdb_d0_man_sel,pdb_d0_man_sel,0x1);
		PHY_REG_M14B0_Wr01(pdb_d0_man_sel,pdb_d0_man,0x0);
		PHY_REG_M14B0_WrFL(pdb_d0_man_sel);

		PHY_REG_M14B0_RdFL(pdb_dck_man_sel);
		PHY_REG_M14B0_Wr01(pdb_dck_man_sel,pdb_dck_man_sel,0x1);
		PHY_REG_M14B0_Wr01(pdb_dck_man_sel,pdb_dck_man,0x0);
		PHY_REG_M14B0_WrFL(pdb_dck_man_sel);
	}
#endif
#endif
#endif

	if ( ( (gHDMI_Phy_Control.all_port_pdb_enable && phy_enable_prt3) || (!gHDMI_Phy_Control.all_port_pdb_enable && (prt_selected==0x3) ) ) && cd_sense)
	{
// GetMHLConnection also execute these settings 
#if 0
#ifdef M14_CODE_FOR_MHL_CTS
#ifdef M14_CBUS_PDB_CTRL
		HDMI_DEBUG("---- PHY INIT for MHL,  PDB_D0_MAN_SEL :0x10, PDB_DCK_MAN_SEL :0x10\n");
		PHY_REG_M14B0_RdFL(pdb_d0_man_sel);
		PHY_REG_M14B0_Wr01(pdb_d0_man_sel,pdb_d0_man_sel,0x1);
		PHY_REG_M14B0_Wr01(pdb_d0_man_sel,pdb_d0_man,0x0);
		PHY_REG_M14B0_WrFL(pdb_d0_man_sel);
		PHY_REG_M14B0_RdFL(pdb_dck_man_sel);
		PHY_REG_M14B0_Wr01(pdb_dck_man_sel,pdb_dck_man_sel,0x1);
		PHY_REG_M14B0_Wr01(pdb_dck_man_sel,pdb_dck_man,0x0);
		PHY_REG_M14B0_WrFL(pdb_dck_man_sel);
#endif
#endif
#endif

// GetMHLConnection also execute these settings 
#if 0	// GetMHLConnection also execute these settings 
		HDMI_DEBUG("---- MHL port : cd sense ON [%s]\n", __func__);
		HDMI_DEBUG("---- MHL CD Sense dr_filter_ch0 : [%d]\n", 0x1);
		PHY_REG_M14B0_Wr01(dr_filter_ch0,dr_filter_ch0,0x1);
		HDMI_DEBUG("---- MHL CR_PLL Charge Pump Current Adjust : [%d]\n", 0x3);
		PHY_REG_M14B0_Wr01(cr_icp_adj,cr_icp_adj,0x2);	// from 0x3 (A0)
		HDMI_DEBUG("---- MHL EQ_RS_MAN  : [%d]\n", 0x5);
		PHY_REG_M14B0_Wr01(eq_rs_man,eq_rs_man,0x5);	//from 0x3(default)
#ifdef M14_CODE_FOR_MHL_CTS
		HDMI_DEBUG("---- MHL ODC CTRL  : [%d]\n", 0x0);
		PHY_REG_M14B0_RdFL(odt_ctrl);
		PHY_REG_M14B0_Wr01(odt_ctrl,odt_ctrl,0x0);
		PHY_REG_M14B0_WrFL(odt_ctrl);
#endif
#endif

	}
	else
	{
		HDMI_DEBUG("---- HDMI Mode dr_filter_ch0 : [%d]\n", 0x7);
		PHY_REG_M14B0_Wr01(dr_filter_ch0,dr_filter_ch0,0x7);
		HDMI_DEBUG("---- HDMI Mode  CR_PLL Charge Pump Current Adjust : [%d]\n", 0x4);
		PHY_REG_M14B0_Wr01(cr_icp_adj,cr_icp_adj,0x4);
#ifdef M14_HDMI_SEMI_AUTO_EQ_CONTROL
		HDMI_DEBUG("---- HDMI EQ_RS_MAN  : [%d]\n", 0x3);
		PHY_REG_M14B0_Wr01(eq_rs_man,eq_rs_man,0x3);
#else
		HDMI_DEBUG("---- HDMI EQ_RS_MAN  : [%d]\n", 0x3);
		/* BD570 EQ error : EQ RS value 0x7 (default 0x3) , 131114 */
		PHY_REG_M14B0_Wr01(eq_rs_man,eq_rs_man,0x3);
#endif
#ifdef M14_CODE_FOR_MHL_CTS
		HDMI_DEBUG("---- HDMI ODC CTRL  : [%d]\n", 0x1);
		PHY_REG_M14B0_RdFL(odt_ctrl);
		PHY_REG_M14B0_Wr01(odt_ctrl,odt_ctrl,0x1);
		PHY_REG_M14B0_WrFL(odt_ctrl);
#endif

		HDMI_DEBUG("---- HDMI DR_N1  : [%d]\n", 0x3);
		PHY_REG_M14B0_RdFL(dr_n1);
		PHY_REG_M14B0_Wr01(dr_n1,dr_n1,0x3);	// for 15m cable (0x38/0x93)
		PHY_REG_M14B0_WrFL(dr_n1);
	}

	PHY_REG_M14B0_WrFL(dr_filter_ch0);
	PHY_REG_M14B0_WrFL(cr_icp_adj);
	PHY_REG_M14B0_WrFL(eq_rs_man);

	OS_MsecSleep(100);	// ms delay

	PHY_REG_M14B0_RdFL(cr_mode_sel_resetb);
	PHY_REG_M14B0_Wr01(cr_mode_sel_resetb, cr_mode_sel_resetb,0x1);
	PHY_REG_M14B0_WrFL(cr_mode_sel_resetb);

	return RET_OK;
}

static int _HDMI_M14B0_RunReset(void)
{
	// Clock Divide for Pixel Repetition format
	LINK_M14_RdFL(video_00);
	LINK_M14_Wr01(video_00, reg_pr_cmu_sync, 0x1);
	LINK_M14_WrFL(video_00);

	// Negative polarity for M14 UD and 3D_SS_Full format
	// Support positive and negative polarity for M14 DE HDMI data bridge
	LINK_M14_RdFL(video_03);
	LINK_M14_Wr01(video_03, reg_neg_pol_en,0x1);
	LINK_M14_WrFL(video_03);

	/* MHL 2.0 900mA */
	LINK_M14_RdFL(cbus_15);
	LINK_M14_Wr01(cbus_15, reg_sink_cap_dev_cat, 0x31);
	LINK_M14_WrFL(cbus_15);

	/* HDCP 1.1 not support*/
	LINK_M14_RdFL(hdcp_05);
	LINK_M14_Wr01(hdcp_05, reg_hdcp_bcaps_prt0, 0x81);
	LINK_M14_WrFL(hdcp_05);

	LINK_M14_RdFL(hdcp_11);
	LINK_M14_Wr01(hdcp_11, reg_hdcp_bcaps_prt1, 0x81);
	LINK_M14_WrFL(hdcp_11);

	LINK_M14_RdFL(hdcp_17);
	LINK_M14_Wr01(hdcp_17, reg_hdcp_bcaps_prt2, 0x81);
	LINK_M14_WrFL(hdcp_17);

	LINK_M14_RdFL(hdcp_23);
	LINK_M14_Wr01(hdcp_23, reg_hdcp_bcaps_prt3, 0x81);
	LINK_M14_WrFL(hdcp_23);

	LINK_M14_RdFL(cbus_12);
	LINK_M14_Wr01(cbus_12, reg_man_src_support_rcp, 0x1);	//manual RCP support for Vu/LTE1 phones
	LINK_M14_WrFL(cbus_12);

#ifdef M14_CODE_FOR_MHL_CTS
	LINK_M14_RdFL(cbus_02);
	LINK_M14_Wr01(cbus_02, reg_cbus_bi_n_retry, 0x1F);	//0x1E
	LINK_M14_WrFL(cbus_02);

	//0x298
	LINK_M14_RdFL(cbus_15);
	LINK_M14_Wr(cbus_15, 0x00213102);	//mhl version 2.1
	LINK_M14_WrFL(cbus_15);

	//0x29c
	LINK_M14_RdFL(cbus_16);
	LINK_M14_Wr(cbus_16, 0x64370100);
	LINK_M14_WrFL(cbus_16);

	//0x2A0
	LINK_M14_RdFL(cbus_17);
	LINK_M14_Wr(cbus_17, 0x410F0700);
	LINK_M14_WrFL(cbus_17);

	LINK_M14_RdFL(cbus_18);
	LINK_M14_Wr(cbus_18, 0x00001033);
	LINK_M14_WrFL(cbus_18);

	LINK_M14_RdFL(cbus_42);
	LINK_M14_Wr(cbus_42, 0x001F0201);
	LINK_M14_WrFL(cbus_42);

	LINK_M14_RdFL(cbus_29);
	LINK_M14_Wr(cbus_29, 0x01100110);
	LINK_M14_WrFL(cbus_29);

	LINK_M14_RdFL(cbus_30);
	LINK_M14_Wr(cbus_30, 0x01000000);
	LINK_M14_WrFL(cbus_30);

	LINK_M14_RdFL(cbus_36);
	LINK_M14_Wr(cbus_36, 0x120311);
	LINK_M14_WrFL(cbus_36);

	LINK_M14_RdFL(cbus_35);
	LINK_M14_Wr01(cbus_35, reg_cbus_wake_to_discover_min_2, 0x50);	//0x63 : MHL CTS C-Bus Wakeup fix
	LINK_M14_WrFL(cbus_35);

	LINK_M14_RdFL(cbus_33);
	LINK_M14_Wr01(cbus_33, reg_cbus_wake_pulse_width_1_min, 0x0D);	//0x11 : min pulse width (17msec => 13msec) //meizu phone
	LINK_M14_Wr01(cbus_33, reg_cbus_wake_pulse_width_1_max, 0x21);	//0x17 : max pulse width (23msec => 33msec)	//HTC One S Phone
	LINK_M14_WrFL(cbus_33);

	LINK_M14_RdFL(cbus_34);
	LINK_M14_Wr01(cbus_34, reg_cbus_wake_pulse_width_2_max, 0x4D);	//0x43 : max pulse width (67msec => 77msec)	//HTC One S Phone
	LINK_M14_WrFL(cbus_34);
#endif
	// HPD PIN Control Enable
	//LINK_M14_RdFL(phy_link_00);
	//LINK_M14_Wr01(phy_link_00, hpd0_oen, 0x0);
	//LINK_M14_WrFL(phy_link_00);

/*	// set internal test hdcp key
	LINK_M14_RdFL((UINT32*)&glink_818_bx);
	LINK_M14_Wr01.reg_dbg_hdcp_key_bak_en,0x0)
	LINK_M14_WrFL((UINT32*)&glink_818_bx);
*/
	/*
	LINK_M14_RdFL(hdcp_00);
	LINK_M14_Wr02(hdcp_00, reg_hdcp_unauth_mode_chg_prt0, 0, reg_hdcp_unauth_nosync_prt0, 0);
	LINK_M14_WrFL(hdcp_00);
	LINK_M14_RdFL(hdcp_06);
	LINK_M14_Wr02(hdcp_06, reg_hdcp_unauth_mode_chg_prt1, 0, reg_hdcp_unauth_nosync_prt1, 0);
	LINK_M14_WrFL(hdcp_06);
	LINK_M14_RdFL(hdcp_12);
	LINK_M14_Wr02(hdcp_12, reg_hdcp_unauth_mode_chg_prt2, 0, reg_hdcp_unauth_nosync_prt2, 0);
	LINK_M14_WrFL(hdcp_12);
	LINK_M14_RdFL(hdcp_18);
	LINK_M14_Wr02(hdcp_18, reg_hdcp_unauth_mode_chg_prt3, 0, reg_hdcp_unauth_nosync_prt3, 0);
	LINK_M14_WrFL(hdcp_18);
	*/

	return RET_OK;
}

/**
* _HDMI_M14B0_SetAudio
*
* @parm void
* @return int
*/
static int _HDMI_M14B0_SetAudio(void)
{
	//ARC source
	LINK_M14_RdFL(edid_heac_00);
	LINK_M14_Wr01(edid_heac_00, reg_arc_src, 0x1);
	LINK_M14_WrFL(edid_heac_00);

	//Channel 0
	LINK_M14_RdFL(system_control_01);
	LINK_M14_Wr01(system_control_01, reg_aac_en, 0x1);	//Auto Audio Path Configuration Enable(N, CTS value is auto configured.)
	LINK_M14_WrFL(system_control_01);

	LINK_M14_RdFL(audio_00);
	LINK_M14_Wr01(audio_00, reg_acr_en, 		 0x0);	//ACR Enable(Audio Clock Generation Function Activation)
	LINK_M14_Wr01(audio_00, reg_acr_n_fs, 		 0x1);	//0 : 128Fs, 1 : 256 Fs, 2 : 512Fs(default : 256Fs)
	LINK_M14_Wr01(audio_00, reg_acr_clk_aud_div, 0x1);	//0 : 128Fs, 1 : 256 Fs, 2 : 512Fs(default : 256Fs)
	LINK_M14_Wr01(audio_00, reg_acr_ncts_rx_en,  0x0);	//Use N, CTS value for audio clock generation
	LINK_M14_Wr01(audio_00, reg_acr_adj_thr, 	 0x3);	//Threshold Value for Clock Frequency Auto Adjustment for proper FIFO running, not required
	LINK_M14_Wr01(audio_00, reg_acr_adj_en, 	 0x1);	//Enable Clock Frequency Auto Adjustment for proper FIFO running
	LINK_M14_WrFL(audio_00);

	LINK_M14_RdFL(audio_05);
	LINK_M14_Wr01(audio_05, reg_i2s_sck_edge, 1);	//I2S Format for falling edge
	LINK_M14_Wr01(audio_05, reg_i2s_sd_en,  0xF);	//I2S SD Output Enable(4 Channel)
	LINK_M14_Wr01(audio_05, reg_i2s_out_en,   1);	//I2S Output Enable
	LINK_M14_Wr01(audio_05, reg_i2s_sd0_map,  0);	//I2S SD 0 Output Channel Mappings
	LINK_M14_Wr01(audio_05, reg_i2s_sd1_map,  1);	//I2S SD 1 Output Channel Mappings
	LINK_M14_Wr01(audio_05, reg_i2s_sd2_map,  2);	//I2S SD 2 Output Channel Mappings
	LINK_M14_Wr01(audio_05, reg_i2s_sd3_map,  3);	//I2S SD 3 Output Channel Mappings
	LINK_M14_WrFL(audio_05);

	//Set audio mute state
	_gM14B0AudioMuteState = FALSE;

	HDMI_AUDIO("[%d] %s\n", __L__, __F__);
	return RET_OK;
}


/**
 *  _HDMI_M14B0_GetAudioTypeAndFreq
 *
 *  @return int
*/static int _HDMI_M14B0_GetAudioInfo(void)
{
	int 	ret = RET_OK;

	ULONG	flags = 0;
	BOOLEAN bDebugEnabled = FALSE;	//< HDMI Debug Print Enabled

	UINT32	prt_selected, reg_scdt, reg_hdmi_mode, reg_burst_pc_0;

	UINT16	ui16VActive;		//< HDMI Vertical Active Size
	UINT16	ui16HActive;		//< HDMI horizontal Active Size
	UINT32	intrVideoState		= HDMI_STATE_DISABLE;
	UINT32	intrAudioState		= HDMI_AUDIO_INIT_STATE;
	UINT32	audioRecheckTime 	= HDMI_AUDIO_RECHECK_TIME_500MS;

	UINT8	ui8TmdsClockHigh = 0;		//< HDMI measured clock value of TMDS clock for upper 8 bit
	UINT8	ui8TmdsClockLow  = 0;		//< HDMI measured clock value of TMDS clock for low 8 bit
	UINT64	ui64TmdsClock 	 = 0;		//< HDMI measured clock value of TMDS clock

	LX_HDMI_AUDIO_TYPE_T		audioType		= LX_HDMI_AUDIO_NO_AUDIO;		///< HDMI Audio Type.
	LX_HDMI_SAMPLING_FREQ_T 	samplingFreq	= LX_HDMI_SAMPLING_FREQ_NONE; 	///< Sampling Frequency

	LX_HDMI_SAMPLING_FREQ_T 	samplingFreqFromTmds = LX_HDMI_SAMPLING_FREQ_NONE;	//< HDMI sampling frequency from TMDS clock
	LX_HDMI_MUTE_CTRL_T 		muteCtrl = {FALSE, FALSE, LX_HDMI_AUDIO_MUTE};

	//Increase _gM14B0HdmiAudioThreadCount;
	_gM14B0HdmiAudioThreadCount++;

	//Check HDMI port connection
	LINK_M14_RdFL(system_control_00);

	//LINK_M14_Rd01(system_control_00, reg_scdt, reg_scdt);
	LINK_M14_Rd01(system_control_00, reg_prt_sel, prt_selected);

	//Check a selected HDMI Port.
	if (prt_selected == 0)
	{
		LINK_M14_Rd01(system_control_00, reg_scdt_prt0, reg_scdt);
		LINK_M14_Rd01(system_control_00, reg_hdmi_mode_prt0, reg_hdmi_mode);
	}
	else if (prt_selected == 1)
	{
		LINK_M14_Rd01(system_control_00, reg_scdt_prt1, reg_scdt);
		LINK_M14_Rd01(system_control_00, reg_hdmi_mode_prt1, reg_hdmi_mode);
	}
	else if (prt_selected == 2)
	{
		LINK_M14_Rd01(system_control_00, reg_scdt_prt2, reg_scdt);
		LINK_M14_Rd01(system_control_00, reg_hdmi_mode_prt2, reg_hdmi_mode);
	}
	else
	{
		LINK_M14_Rd01(system_control_00, reg_scdt_prt3, reg_scdt);
		LINK_M14_Rd01(system_control_00, reg_hdmi_mode_prt3, reg_hdmi_mode);
	}

	if (reg_scdt == 0)	//Check HDMI, DVI Sync Detect
	{
		//spin lock for protection : lock
		spin_lock_irqsave(&_gIntrHdmiM14B0AudioLock, flags);

		//Clear _gM14B0IntrAudioState
		_gM14B0IntrAudioState = HDMI_AUDIO_INIT_STATE;

		//Clear _gM14B0HdmiFreqErrorCount
		_gM14B0HdmiFreqErrorCount = 0;

		//spin lock for protection : unlock
		spin_unlock_irqrestore(&_gIntrHdmiM14B0AudioLock, flags);

		if ((_gM14B0HdmiAudioThreadCount % DEBUG_HDMI_AUDIO_MSG_PRINT_TIME_10S) == 0)
			HDMI_AUDIO("[%d]HDMI_GetAudioInfo : HDMI / DVI Not Connected(state = %d)!!!\n", __L__, _gM14B0IntrAudioState);

		goto func_exit;
	}

	//Get measured frequency value of TMDS clock.
#ifdef M14_THREAD_READ_PHY_REG_VALUE
	ui64TmdsClock = (UINT64)(_gM14B0HDMIPhyInform.tmds_clock ) * 10000;
#else
	PHY_REG_M14B0_RdFL(tmds_freq_1);
	PHY_REG_M14B0_RdFL(tmds_freq_2);

	PHY_REG_M14B0_Rd01(tmds_freq_1, tmds_freq, ui8TmdsClockHigh);
	PHY_REG_M14B0_Rd01(tmds_freq_2, tmds_freq, ui8TmdsClockLow);

	ui64TmdsClock = (UINT64)((ui8TmdsClockHigh << 8) | ui8TmdsClockLow) * 10000;
#endif
	HDMI_AUDIO("Get TDMS Clock : ui64TmdsClock = %llu, ui8TmdsClockHigh = 0x%X, ui8TmdsClockLow = %d\n", ui64TmdsClock, ui8TmdsClockHigh, ui8TmdsClockLow);

	//check a UD format(do not support M14 chip upper 25MHz TDMS Clock)
	if (ui64TmdsClock > 250*1000*1000)
	{
		if ((_gM14B0HdmiAudioThreadCount % DEBUG_HDMI_AUDIO_MSG_PRINT_TIME_10S) == 0)
			HDMI_AUDIO("[%d]HDMI_GetAudioInfo : HDMI UD Format(ui64TmdsClock = %llu)!!!\n", __L__, ui64TmdsClock);

		goto func_exit;
	}
	//check a MHL Contents Off Status
	else if ((_gM14B0MHLContentOff == TRUE) && (_gM14B0HDMIPhyInform.prt_sel == 3) && (_gM14B0HDMIPhyInform.cd_sense == 1))
	{
		if ((_gM14B0HdmiAudioThreadCount % DEBUG_HDMI_AUDIO_MSG_PRINT_TIME_10S) == 0)
			HDMI_AUDIO("[%d]HDMI_GetAudioInfo : MHL Contents Off(_gM14B0MHLContentOff = %d)!!!\n", __L__, _gM14B0MHLContentOff);

		goto func_exit;
	}

	//spin lock for protection : lock
	spin_lock_irqsave(&_gIntrHdmiM14B0AudioLock, flags);

	//copy video interrupt state
	intrVideoState = _gM14B0HDMIState;
	//intrVideoState = HDMI_STATE_READ;	//temp for test

	//copy audio interrupt state
	intrAudioState = _gM14B0IntrAudioState;

	//spin lock for protection : unlock
	spin_unlock_irqrestore(&_gIntrHdmiM14B0AudioLock, flags);

	//Check _gM14B0IntrAudioState is stably changed.
	if ( (intrAudioState == HDMI_AUDIO_STABLE_STATE && intrVideoState >= HDMI_STATE_READ)
	  ||(intrAudioState == HDMI_AUDIO_STABLE_STATE && _gM14B0HdmiPortStableCount > HDMI_AUDIO_PORT_STABLE_TIME_5S) )	//To protect a abnormal or transient video and audio info drop state.
	{
		if (_gM14B0HdmiAudioInfo.audioType == LX_HDMI_AUDIO_AC3_IEC60958)
			audioRecheckTime = HDMI_AUDIO_RECHECK_TIME_1S;
		else
			audioRecheckTime = HDMI_AUDIO_RECHECK_TIME_500MS;

		//Recheck for HDMI Audio Format and sampling frequency
		if ((_gM14B0HdmiAudioThreadCount % audioRecheckTime) == 0)	// 0.5 or 1 seconds, Thread calls a every 20 ms.
		{
			//Check a current mute status for workaround
			if (_gM14B0AudioMuteState == TRUE)
			{
				//Unmute audio data for abnormal state
				muteCtrl.eMode		= LX_HDMI_AUDIO_MUTE;
				muteCtrl.bAudioMute = FALSE;
				_HDMI_M14B0_SetInternalMute(muteCtrl);

				//Set a debug print status
				bDebugEnabled = TRUE;
			}

			//Get HDMI Audio Type and Sampling Frequency
			(void)_HDMI_M14B0_GetAudioTypeAndFreq(&audioType, &samplingFreq);

			//Get a sampling frequency from TMDS clock
			(void)_HDMI_M14B0_GetAudioFreqFromTmdsClock(&samplingFreqFromTmds);

			//M14 A0 IP Bug : non-PCM interrupt is not triggerred if non-PCM(AC-3) is changed to non-PCM(DTS).
			if ( (audioType			  != _gM14B0HdmiAudioInfo.audioType)
			  ||(samplingFreqFromTmds != _gM14B0HdmiAudioInfo.samplingFreq) )
			{
				HDMI_DEBUG("[%d]HDMI_GetAudioInfo : type = %d<-%d, tmds freq = %d<-%d, freq = %d, state = %d, mute = %d, PSC = %d(%d)\n", \
							__L__, audioType, _gM14B0HdmiAudioInfo.audioType, samplingFreqFromTmds, _gM14B0HdmiAudioInfo.samplingFreq,	\
							samplingFreq, _gM14B0IntrAudioState, _gM14B0AudioMuteState, _gM14B0HdmiPortStableCount, audioRecheckTime);

				//spin lock for protection for audio
				spin_lock_irqsave(&_gIntrHdmiM14B0AudioLock, flags);
				_gM14B0IntrAudioState = HDMI_AUDIO_INTERRUPT_STATE;
				_gM14B0HdmiFreqErrorCount = 0;
				spin_unlock_irqrestore(&_gIntrHdmiM14B0AudioLock, flags);

				//Mute audio data
				muteCtrl.eMode		= LX_HDMI_AUDIO_MUTE;
				muteCtrl.bAudioMute = TRUE;
				_HDMI_M14B0_SetInternalMute(muteCtrl);

				//Reset _gM14B0IntrBurstInfoCount
				_gM14B0IntrBurstInfoCount = 0;

				goto func_exit;
			}
		}

		//Increase _gM14B0HdmiPortStableCount(If HDMI port is changed, this count is cleared.)
		spin_lock_irqsave(&_gIntrHdmiM14B0AudioLock, flags);
		_gM14B0HdmiPortStableCount++;
		spin_unlock_irqrestore(&_gIntrHdmiM14B0AudioLock, flags);

		//Debug print
		if ((_gM14B0HdmiAudioThreadCount % DEBUG_HDMI_AUDIO_MSG_PRINT_TIME_10S) == 0)
		{
			HDMI_AUDIO("[%d]HDMI_GetAudioInfo : type = %d, freq = %d, state = %d, PSC = %d. mute = %d\n",	\
						__L__, _gM14B0HdmiAudioInfo.audioType, _gM14B0HdmiAudioInfo.samplingFreq,	\
						_gM14B0IntrAudioState, _gM14B0HdmiPortStableCount, _gM14B0AudioMuteState);
		}

		return ret;
	}
	else if (intrAudioState == HDMI_AUDIO_STABLE_STATE && intrVideoState < HDMI_STATE_READ)
	{
		HDMI_AUDIO("[%d]HDMI_GetAudioInfo : HDMI_VIDEO_INTERRUPT_STATE(state A = %d, V = %d)!!!\n", __L__, _gM14B0IntrAudioState, _gM14B0Intr_vf_stable);

		//spin lock for protection for audio
		spin_lock_irqsave(&_gIntrHdmiM14B0AudioLock, flags);
		_gM14B0IntrAudioState = HDMI_AUDIO_INTERRUPT_STATE;
		_gM14B0HdmiFreqErrorCount = 0;
		spin_unlock_irqrestore(&_gIntrHdmiM14B0AudioLock, flags);
	}
	else if (intrAudioState == HDMI_AUDIO_GET_INFO_STATE && intrVideoState >= HDMI_STATE_READ)
	{
		if ((_gM14B0HdmiAudioThreadCount % DEBUG_HDMI_AUDIO_MSG_PRINT_TIME_10S) == 0)
			HDMI_AUDIO("[%d]HDMI_GetAudioInfo : HDMI_AUDIO_GET_INFO_STATE(state A = %d, V = %d)!!!\n", __L__, _gM14B0IntrAudioState, _gM14B0Intr_vf_stable);
	}
	else if (intrAudioState < HDMI_AUDIO_GET_INFO_STATE && intrVideoState >= HDMI_STATE_READ)
	{
		//spin lock for protection : lock
		spin_lock_irqsave(&_gIntrHdmiM14B0AudioLock, flags);

		//increase _gM14B0IntrAudioState
		_gM14B0IntrAudioState++;

		//spin lock for protection : unlock
		spin_unlock_irqrestore(&_gIntrHdmiM14B0AudioLock, flags);

		if ((_gM14B0HdmiAudioThreadCount % DEBUG_HDMI_AUDIO_MSG_PRINT_TIME_10S) == 0)
			HDMI_AUDIO("[%d]HDMI_GetAudioInfo : HDMI_AUDIO_UNSTABLE_STATE(state A = %d, V = %d)!!!\n", __L__, _gM14B0IntrAudioState, _gM14B0Intr_vf_stable);

		goto func_exit;
	}
	else
	{
		if ((_gM14B0HdmiAudioThreadCount % DEBUG_HDMI_AUDIO_MSG_PRINT_TIME_10S) == 0)
			HDMI_AUDIO("[%d]HDMI_GetAudioInfo : HDMI / DVI Not Connected(state = %d)!!!\n", __L__, _gM14B0IntrAudioState);

		goto func_exit;
	}

	//Get video vertical & horizontal active size
	LINK_M14_RdFL(video_05);
	LINK_M14_Rd01(video_05, reg_h_av, ui16HActive);
	LINK_M14_Rd01(video_05, reg_v_av, ui16VActive);

	//Check video active size
	if ( (ui16VActive <	240) || (ui16HActive <	320)	\
	  ||(ui16VActive > 2300) || (ui16HActive > 4096) )
	{
		HDMI_AUDIO("[%d]HDMI_GetAudioInfo : Video Active Size Error(v = %d, h = %d)!!!\n", \
					__L__, ui16VActive, ui16HActive);

		goto func_exit;
	}

	//Check HDMI /DVI Mode, 0 : DVI, 1 : HDMI
	//LINK_M14_RdFL(system_control_00);
	//LINK_M14_Rd01(system_control_00, reg_hdmi_mode, reg_hdmi_mode);

	if (reg_hdmi_mode)
	{
		//Get HDMI Audio Type and Sampling Frequency
		(void)_HDMI_M14B0_GetAudioTypeAndFreq(&audioType, &samplingFreq);

		//Get a sampling frequency from TMDS clock
		(void)_HDMI_M14B0_GetAudioFreqFromTmdsClock(&samplingFreqFromTmds);

		//Get HDMI Audio Sampling Frequency from TMDS clock when audio sample is PCM
		if (audioType == LX_HDMI_AUDIO_PCM)
		{
			//Check a sampling frequency from status byte to TMDS clock
			//Channel status byte 0 value is 44.2Khz normal case or abnormal case.
			if ( (samplingFreq == LX_HDMI_SAMPLING_FREQ_44_1KHZ)
			   &&(samplingFreqFromTmds != LX_HDMI_SAMPLING_FREQ_44_1KHZ) )
			{
				//Check a HDMI Port Stable Count
				if (_gM14B0HdmiFreqErrorCount < HDMI_AUDIO_FREQ_ERROR_TIME_500MS)
				{
					//Increase _gM14B0HdmiFreqErrorCount.
					spin_lock_irqsave(&_gIntrHdmiM14B0AudioLock, flags);
					_gM14B0HdmiFreqErrorCount++;
					spin_unlock_irqrestore(&_gIntrHdmiM14B0AudioLock, flags);

					//Set a no audio mode
					audioType = LX_HDMI_AUDIO_NO_AUDIO;

					if ((_gM14B0HdmiAudioThreadCount % DEBUG_HDMI_AUDIO_MSG_PRINT_TIME_100S) == 0)
					{
						HDMI_ERROR("[%d]HDMI_GetAudioInfo : Freq. Error(freq = %d, tmds freq = %d)!!!\n",	\
									__L__, samplingFreq, samplingFreqFromTmds);
					}

					goto func_exit;
				}
				else
				{
					if ((_gM14B0HdmiAudioThreadCount % DEBUG_HDMI_AUDIO_MSG_PRINT_TIME_100S) == 0)
					{
						HDMI_ERROR("[%d]HDMI_GetAudioInfo : Freq. Error => Force 48KHz(freq = %d, tmds freq = %d)!!!\n",	\
									__L__, samplingFreq, samplingFreqFromTmds);
					}
				}
			}

			//Set a audio output mute when TMDS Fs is zero or not support.
			if ( (samplingFreqFromTmds == LX_HDMI_SAMPLING_FREQ_NONE)
			   ||(samplingFreqFromTmds == LX_HDMI_SAMPLING_FREQ_22_05KHZ)
			   ||(samplingFreqFromTmds == LX_HDMI_SAMPLING_FREQ_24_KHZ)
			   ||(samplingFreqFromTmds == LX_HDMI_SAMPLING_FREQ_768_KHZ))
			{
				//Set a no audio mode
				audioType = LX_HDMI_AUDIO_NO_AUDIO;

				if ((_gM14B0HdmiAudioThreadCount % DEBUG_HDMI_AUDIO_MSG_PRINT_TIME_100S) == 0)
					HDMI_ERROR("[%d]HDMI_GetAudioInfo : TMDS Fs Error(%d)!!!\n", __L__, samplingFreqFromTmds);

				goto func_exit;
			}
		}

		//Set a sampling frequency from TMDS clock
		samplingFreq = samplingFreqFromTmds;

		//Set a audio output mute when TMDS Fs is zero or not support.
		if ( (samplingFreqFromTmds == LX_HDMI_SAMPLING_FREQ_NONE)
		   ||(samplingFreqFromTmds == LX_HDMI_SAMPLING_FREQ_22_05KHZ)
		   ||(samplingFreqFromTmds == LX_HDMI_SAMPLING_FREQ_24_KHZ)
		   ||(samplingFreqFromTmds == LX_HDMI_SAMPLING_FREQ_768_KHZ))
		{
			//Set a no audio mode
			audioType = LX_HDMI_AUDIO_NO_AUDIO;

			if ((_gM14B0HdmiAudioThreadCount % DEBUG_HDMI_AUDIO_MSG_PRINT_TIME_100S) == 0)
				HDMI_ERROR("[%d]HDMI_GetAudioInfo : TMDS Fs Error(%d)!!!\n", __L__, samplingFreqFromTmds);

			goto func_exit;
		}

		//For debug print
		if ((_gM14B0HdmiAudioThreadCount % DEBUG_HDMI_AUDIO_MSG_PRINT_TIME_100S) == 0)
		{
			HDMI_AUDIO("[%d]GetAudioInfo : type = %d, freq = %d, mute = %d, PSC = %d\n", \
						__L__, audioType, samplingFreq, _gM14B0AudioMuteState, _gM14B0HdmiPortStableCount);
		}
	}
	else
	{
		//Set DVI mode
		audioType	 = LX_HDMI_AUDIO_DVI;
		samplingFreq = LX_HDMI_SAMPLING_FREQ_NONE;
	}

	//Check a audio and video stable status
	if ( (audioType	  == _gM14B0HdmiAudioInfo.audioType)
	  &&(samplingFreq == _gM14B0HdmiAudioInfo.samplingFreq)
	  /* &&(_gM14B0HdmiAudioInfo.audioType >= LX_HDMI_AUDIO_PCM) */
	  &&(intrVideoState >= HDMI_STATE_READ) )
	{
		//spin lock for protection : lock
		spin_lock_irqsave(&_gIntrHdmiM14B0AudioLock, flags);

		//Set a _gM14B0IntrAudioState
		_gM14B0IntrAudioState = HDMI_AUDIO_STABLE_STATE;

		//spin lock for protection : unlock
		spin_unlock_irqrestore(&_gIntrHdmiM14B0AudioLock, flags);

		//Set a debug print status
		bDebugEnabled = TRUE;
	}

	//For debug print
	if ( (audioType != _gM14B0HdmiAudioInfo.audioType) || (samplingFreq != _gM14B0HdmiAudioInfo.samplingFreq) )
	{
		if ( ((audioType == LX_HDMI_AUDIO_DVI) || (audioType == LX_HDMI_AUDIO_NO_AUDIO))	\
		  &&((_gM14B0HdmiAudioThreadCount % DEBUG_HDMI_AUDIO_MSG_PRINT_TIME_100S) == 0) )
		{
			//Set a debug print status
			bDebugEnabled = TRUE;
		}
	}

	//Check a debug print status
	if (bDebugEnabled == TRUE)
	{
		//Get a vaild Pc data for payload
		LINK_M14_RdFL(audio_09);
		LINK_M14_Rd01(audio_09, reg_burst_pc_0, reg_burst_pc_0);

		HDMI_AUDIO("[%d]GetAudioInfo : type = %d(%d), freq = %d, mute = %d, PSC = %d\n", \
					__L__, audioType, reg_burst_pc_0 & 0x1F, samplingFreq, _gM14B0AudioMuteState, _gM14B0HdmiPortStableCount);
	}

func_exit:
	if (audioType == LX_HDMI_AUDIO_NO_AUDIO)
	{
		//Mute audio data
		muteCtrl.eMode		= LX_HDMI_AUDIO_MUTE;
		muteCtrl.bAudioMute = TRUE;
		_HDMI_M14B0_SetInternalMute(muteCtrl);

		//Clears a global value for audio info.
		_gM14B0HdmiAudioInfo.audioType	  = LX_HDMI_AUDIO_NO_AUDIO;
		_gM14B0HdmiAudioInfo.samplingFreq = LX_HDMI_SAMPLING_FREQ_NONE;

		//Clear _gM14B0HdmiPortStableCount
		_gM14B0HdmiPortStableCount = 0;

		//Debug print
		if ((_gM14B0HdmiAudioThreadCount % DEBUG_HDMI_AUDIO_MSG_PRINT_TIME_10S) == 0)
			HDMI_AUDIO("[%d]HDMI_GetAudioInfo : HDMI Audio Mute On !!!\n", __L__);
	}
	else
	{
		//Update a global value when audio info. is stable.
		_gM14B0HdmiAudioInfo.audioType	  = audioType;
		_gM14B0HdmiAudioInfo.samplingFreq = samplingFreq;
	}

	HDMI_ATASK("_M14B0_GetAudioInfo : type = %d, freq = %d(A = %d, V = %d)\n", audioType, samplingFreq, intrAudioState, intrVideoState);
	return RET_OK;
}

/**
 *  _HDMI_M14B0_GetAudioTypeAndFreq
 *
 *  @return int
*/
static int _HDMI_M14B0_GetAudioTypeAndFreq(LX_HDMI_AUDIO_TYPE_T *audioType, LX_HDMI_SAMPLING_FREQ_T *samplingFreq)
{
	int ret = RET_OK;

	UINT32	prt_selected, reg_hdmi_mode = 0;
	UINT32	reg_achst_byte0, reg_achst_byte3;
	UINT32	reg_burst_pc_0;

	//Check HDMI /DVI Mode, 0 : DVI, 1 : HDMI
	LINK_M14_RdFL(system_control_00);
	//LINK_M14_Rd01(system_control_00, reg_hdmi_mode, reg_hdmi_mode);
	LINK_M14_Rd01(system_control_00, reg_prt_sel, prt_selected);

	//Check a selected HDMI Port.
	if (prt_selected == 0)
		LINK_M14_Rd01(system_control_00, reg_hdmi_mode_prt0, reg_hdmi_mode);
	else if (prt_selected == 1)
		LINK_M14_Rd01(system_control_00, reg_hdmi_mode_prt1, reg_hdmi_mode);
	else if (prt_selected == 2)
		LINK_M14_Rd01(system_control_00, reg_hdmi_mode_prt2, reg_hdmi_mode);
	else
		LINK_M14_Rd01(system_control_00, reg_hdmi_mode_prt3, reg_hdmi_mode);

	if (reg_hdmi_mode)
	{
		//Check if audio sample word used for other purposes than liner PCM.
		LINK_M14_RdFL(audio_07);
		LINK_M14_Rd01(audio_07, reg_achst_byte0, reg_achst_byte0);

		if (reg_achst_byte0 & HDMI_AUDIO_SAMPLE_NON_PCM_MASK)	//bit 1, 0 : PCM, 1 : non-PCM
		{
			//Check a vaild Pc data for payload
			LINK_M14_RdFL(audio_09);
			LINK_M14_Rd01(audio_09, reg_burst_pc_0, reg_burst_pc_0);

			if ((reg_burst_pc_0 & BURST_INFO_PAYLOAD_ERROR_BIT_MASK) == 0) //bit 7, 0 : No Error, 1 : Error
			{
				//Set Audio Data-Types according to IEC61937-2 Burst Info Preamble C
				switch(reg_burst_pc_0 & BURST_INFO_AUDIO_TYPE_BIT_MASK)	//bit 4 ~ 0
				{
					case BURST_INFO_AUDIO_TYPE_AC3:
					case BURST_INFO_AUDIO_TYPE_AC3_ENHANCED:
						*audioType = LX_HDMI_AUDIO_AC3;
						break;

					case BURST_INFO_AUDIO_TYPE_DTS_I:
					case BURST_INFO_AUDIO_TYPE_DTS_II:
					case BURST_INFO_AUDIO_TYPE_DTS_III:
					case BURST_INFO_AUDIO_TYPE_DTS_IV:
						*audioType = LX_HDMI_AUDIO_DTS;
						break;

					case BURST_INFO_AUDIO_TYPE_MPEG2_AAC:
					case BURST_INFO_AUDIO_TYPE_MPEG2_AAC_LOW:
					case BURST_INFO_AUDIO_TYPE_MPEG4_AAC:
						*audioType = LX_HDMI_AUDIO_AAC;
						break;

					case BURST_INFO_AUDIO_TYPE_MPEG1_L1:
					case BURST_INFO_AUDIO_TYPE_MPEG1_L23:
					case BURST_INFO_AUDIO_TYPE_MPEG2_EXT:
					case BURST_INFO_AUDIO_TYPE_MPEG2_L1:
					case BURST_INFO_AUDIO_TYPE_MPEG2_L2:
					case BURST_INFO_AUDIO_TYPE_MPEG2_L3:
						*audioType = LX_HDMI_AUDIO_MPEG;
						break;

					case BURST_INFO_AUDIO_TYPE_NULL:
						*audioType = _gM14B0HdmiAudioInfo.audioType;
						if ((_gM14B0HdmiAudioThreadCount % DEBUG_HDMI_AUDIO_MSG_PRINT_TIME_100S) == 0)
							HDMI_DEBUG("_HDMI_M14B0_GetAudioTypeAndFreq : BURST_INFO_AUDIO_TYPE_NULL(audioType= %d)\n", *audioType);
						break;

					default:
						*audioType = LX_HDMI_AUDIO_NO_AUDIO;
						break;
				}

				//Debug print
				if ((reg_burst_pc_0 & BURST_INFO_AUDIO_TYPE_BIT_MASK) == BURST_INFO_AUDIO_TYPE_PAUSE)
				{
					if ((_gM14B0HdmiAudioThreadCount % DEBUG_HDMI_AUDIO_MSG_PRINT_TIME_100S) == 0)
						HDMI_DEBUG("_HDMI_M14B0_GetAudioTypeAndFreq : BURST_INFO_AUDIO_TYPE_PAUSE(Pc = %d)!!!\n", reg_burst_pc_0);
				}
			}	//if ((reg_burst_pc_0 & BURST_INFO_PAYLOAD_ERROR_BIT_MASK) == 0)
			else
			{
				*audioType = LX_HDMI_AUDIO_PCM;

				HDMI_ERROR("_HDMI_M14B0_GetAudioTypeAndFreq : Burst Info Error = %d\n", (reg_burst_pc_0 & BURST_INFO_PAYLOAD_ERROR_BIT_MASK));
			}
		}	//if (reg_achst_byte0 & HDMI_AUDIO_SAMPLE_NON_PCM_MASK)
		else
		{
			*audioType = LX_HDMI_AUDIO_PCM;

			/* Workaround Code for Skylife and DMT HDMI Repeater(Digital Stream, HDMI 210) Issue.(2013-11-02) */
			if (_gM14B0IntrBurstInfoCount != _gM14B0TaskBurstInfoCount)
			{
				//Update _gM14B0TaskBurstInfoCount value.
				_gM14B0TaskBurstInfoCount = _gM14B0IntrBurstInfoCount;

				/* Check a audio type in IEC61937 Burst Info. Packet. */
				//Check a vaild Pc data for payload
				LINK_M14_RdFL(audio_09);
				LINK_M14_Rd01(audio_09, reg_burst_pc_0, reg_burst_pc_0);

				if ((reg_burst_pc_0 & BURST_INFO_PAYLOAD_ERROR_BIT_MASK) == 0) //bit 7, 0 : No Error, 1 : Error
				{
					//Set Audio Data-Types according to IEC61937-2 Burst Info Preamble C
					switch(reg_burst_pc_0 & BURST_INFO_AUDIO_TYPE_BIT_MASK)	//bit 4 ~ 0
					{
						case BURST_INFO_AUDIO_TYPE_AC3:
						case BURST_INFO_AUDIO_TYPE_AC3_ENHANCED:
							*audioType = LX_HDMI_AUDIO_AC3_IEC60958;
							break;

						case BURST_INFO_AUDIO_TYPE_NULL:
							*audioType = _gM14B0HdmiAudioInfo.audioType;
							if ((_gM14B0HdmiAudioThreadCount % DEBUG_HDMI_AUDIO_MSG_PRINT_TIME_100S) == 0)
								HDMI_DEBUG("_HDMI_M14B0_GetAudioTypeAndFreq : BURST_INFO_AUDIO_TYPE_NULL(audioType= %d)\n", *audioType);
							break;

						default:
							*audioType = _gM14B0HdmiAudioInfo.audioType;
							break;
					}
				}

				//Check a audio type if AC-3 ES exist.
				if (*audioType == LX_HDMI_AUDIO_AC3_IEC60958)
				{
						//Check a audio type if first audio info get state for debug print.
					if (_gM14B0IntrAudioState == HDMI_AUDIO_GET_INFO_STATE)
					{
						HDMI_DEBUG("_HDMI_M14B0_GetAudioTypeAndFreq : Forced AC-3.(byte0 = 0x%X, Pc = 0x%X)\n", reg_achst_byte0, reg_burst_pc_0);
					}
				}
			}
			else
			{
				//Reset _gM14B0IntrBurstInfoCount
				_gM14B0IntrBurstInfoCount = 0;
			}
		}

		//Set Sampling frequency from IEC60958 Channel Status Byte 3
		LINK_M14_RdFL(audio_08);
		LINK_M14_Rd01(audio_08, reg_achst_byte3, reg_achst_byte3);

		switch(reg_achst_byte3 & HDMI_AUDIO_SAMPLE_BIT_MASK) 	//bit 0 ~ 3
		{
			case HDMI_AUDIO_SAMPLE_22_05KHZ:
				*samplingFreq = LX_HDMI_SAMPLING_FREQ_22_05KHZ;
				break;

			case HDMI_AUDIO_SAMPLE_24_KHZ:
				*samplingFreq = LX_HDMI_SAMPLING_FREQ_24_KHZ;
				break;

			case HDMI_AUDIO_SAMPLE_32_KHZ:
				*samplingFreq = LX_HDMI_SAMPLING_FREQ_32_KHZ;
				break;

			case HDMI_AUDIO_SAMPLE_44_1KHZ:
				*samplingFreq = LX_HDMI_SAMPLING_FREQ_44_1KHZ;
				break;

			case HDMI_AUDIO_SAMPLE_48_KHZ:
				*samplingFreq = LX_HDMI_SAMPLING_FREQ_48_KHZ;
				break;

			case HDMI_AUDIO_SAMPLE_88_2KHZ:
				*samplingFreq = LX_HDMI_SAMPLING_FREQ_88_2KHZ;
				break;

			case HDMI_AUDIO_SAMPLE_96_KHZ:
				*samplingFreq = LX_HDMI_SAMPLING_FREQ_96_KHZ;
				break;

			case HDMI_AUDIO_SAMPLE_176_4KHZ:
				*samplingFreq = LX_HDMI_SAMPLING_FREQ_176_4KHZ;
				break;

			case HDMI_AUDIO_SAMPLE_192_KHZ:
				*samplingFreq = LX_HDMI_SAMPLING_FREQ_192_KHZ;
				break;

			case HDMI_AUDIO_SAMPLE_768_KHZ:
				*samplingFreq = LX_HDMI_SAMPLING_FREQ_768_KHZ;
				break;

			default:
				*samplingFreq = LX_HDMI_SAMPLING_FREQ_NONE;

				HDMI_AUDIO("_HDMI_M14B0_GetAudioTypeAndFreq(Channel Status) : samplingFreq = %d\n", *samplingFreq);
				break;
		}
	}
	else
	{
		//Set DVI mode
		*audioType	  = LX_HDMI_AUDIO_DVI;
		*samplingFreq = LX_HDMI_SAMPLING_FREQ_NONE;
	}

	//Check a No Audio Mode
	if (*audioType == LX_HDMI_AUDIO_NO_AUDIO)
	{
		*samplingFreq = LX_HDMI_SAMPLING_FREQ_NONE;
	}

	HDMI_ATASK("_M14B0_GetAudioTypeAndFreq : type = %d, freq = %d\n", *audioType, *samplingFreq);
	return ret;
}

/**
 *  _HDMI_M14B0_GetAudioFreqFromTmdsClock
 *
 *  @return int
*/
static int _HDMI_M14B0_GetAudioFreqFromTmdsClock(LX_HDMI_SAMPLING_FREQ_T *samplingFreqFromTmds)
{
	int ret = RET_OK;

#ifndef M14_THREAD_READ_PHY_REG_VALUE
	UINT8		ui8TmdsClockHigh = 0;		//< HDMI measured clock value of TMDS clock for upper 8 bit
	UINT8		ui8TmdsClockLow  = 0;		//< HDMI measured clock value of TMDS clock for low 8 bit
#endif

	UINT64		ui64TmdsClock = 0;			//< HDMI measured clock value of TMDS clock
	UINT64		ui64AcrN = 0;				//< HDMI ACR N value
	UINT64		ui64AcrCts = 0;				//< HDMI ACR CTS value
	UINT64		ui64TmdsSamplingFreq = 0;	//< HDMI sampling frequency in source device from TMDS clock

	//Get measured frequency value of TMDS clock.
#ifdef M14_THREAD_READ_PHY_REG_VALUE
	ui64TmdsClock = (UINT64)(_gM14B0HDMIPhyInform.tmds_clock ) * 10000;
	HDMI_AUDIO("Get TDMS Clock : ui64TmdsClock = %llu\n", ui64TmdsClock);
#else
	PHY_REG_M14B0_RdFL(tmds_freq_1);
	PHY_REG_M14B0_RdFL(tmds_freq_2);

	PHY_REG_M14B0_Rd01(tmds_freq_1, tmds_freq, ui8TmdsClockHigh);
	PHY_REG_M14B0_Rd01(tmds_freq_2, tmds_freq, ui8TmdsClockLow);

	ui64TmdsClock = (UINT64)((ui8TmdsClockHigh << 8) | ui8TmdsClockLow) * 10000;
	HDMI_AUDIO("Get TDMS Clock : ui64TmdsClock = %llu, ui8TmdsClockHigh = 0x%X, ui8TmdsClockLow = %d\n", ui64TmdsClock, ui8TmdsClockHigh, ui8TmdsClockLow);
#endif

	//Get ACR N H/W value.
	LINK_M14_RdFL(audio_01);
	//LINK_M14_Rd01(audio_01, reg_acr_n_tx, ui64AcrN);
	ui64AcrN = LINK_M14_Rd(audio_01) & 0xFFFFF;		//20 bits

	//Get ACR CTS H/W value.
	LINK_M14_RdFL(audio_03);
	//LINK_M14_Rd01(audio_03, reg_acr_cts_tx, ui64AcrCts);
	ui64AcrCts = LINK_M14_Rd(audio_03) & 0xFFFFF;	//20 bits
	HDMI_AUDIO("Get TDMS ACR  : ui64AcrN = %llu, ui64AcrCts = %llu\n", ui64AcrN, ui64AcrCts);

	//Compute a sampling frequency from TMDS clock
	ui64TmdsSamplingFreq = ui64AcrN * ui64TmdsClock;

	//Check divide by zero value.
	if ( (ui64TmdsSamplingFreq > 0) && (ui64AcrCts > 0) )
	{
		do_div(ui64TmdsSamplingFreq, ui64AcrCts * 128);
	}

	//Mapping a sampling frequency from measuring from TMDS clock and ACR N & CTS H/W value
	if (ui64TmdsSamplingFreq == 0)
	{
		*samplingFreqFromTmds = LX_HDMI_SAMPLING_FREQ_NONE;
		HDMI_PRINT("Get Fs from TDMS clock => LX_HDMI_SAMPLING_FREQ_NONE\n");
	}
	else if (ui64TmdsSamplingFreq < 22983)
	{
		*samplingFreqFromTmds = LX_HDMI_SAMPLING_FREQ_22_05KHZ;	//  22.05 kHz(not supported)
		HDMI_PRINT("Get Fs from TDMS clock => LX_HDMI_SAMPLING_FREQ_22_05KHZ(not supported)\n");
	}
	else if (ui64TmdsSamplingFreq < 30000)
	{
		*samplingFreqFromTmds = LX_HDMI_SAMPLING_FREQ_24_KHZ;	//  24 kHz(not supported)
		HDMI_PRINT("Get Fs from TDMS clock => LX_HDMI_SAMPLING_FREQ_24_KHZ(not supported)\n");
	}
	else if (ui64TmdsSamplingFreq < 33800)
	{
		*samplingFreqFromTmds = LX_HDMI_SAMPLING_FREQ_32_KHZ;	//  32 kHz
		HDMI_PRINT("Get Fs from TDMS clock => LX_HDMI_SAMPLING_FREQ_32KHZ\n");
	}
	else if (ui64TmdsSamplingFreq < 45965)
	{
		*samplingFreqFromTmds = LX_HDMI_SAMPLING_FREQ_44_1KHZ;	//  44.1 kHz
		HDMI_PRINT("Get Fs from TDMS clock => LX_HDMI_SAMPLING_FREQ_44_1KHZ\n");
	}
	else if (ui64TmdsSamplingFreq < 67000)
	{
		*samplingFreqFromTmds = LX_HDMI_SAMPLING_FREQ_48_KHZ;	//  48 kHz
		HDMI_PRINT("Get Fs from TDMS clock => LX_HDMI_SAMPLING_FREQ_48_KHZ\n");
	}
	else if (ui64TmdsSamplingFreq < 91935)
	{
		*samplingFreqFromTmds = LX_HDMI_SAMPLING_FREQ_88_2KHZ;	//  88.2 kHz
		HDMI_PRINT("Get Fs from TDMS clock => LX_HDMI_SAMPLING_FREQ_88_2KHZ\n");
	}
	else if (ui64TmdsSamplingFreq < 135000)
	{
		*samplingFreqFromTmds = LX_HDMI_SAMPLING_FREQ_96_KHZ;	//  96 kHz
		HDMI_PRINT("Get Fs from TDMS clock => LX_HDMI_SAMPLING_FREQ_96_KHZ\n");
	}
	else if (ui64TmdsSamplingFreq < 183870)
	{
		*samplingFreqFromTmds = LX_HDMI_SAMPLING_FREQ_176_4KHZ;	//  176.4 kHz
		HDMI_PRINT("Get Fs from TDMS clock => LX_HDMI_SAMPLING_FREQ_176_4KHZ\n");
	}
	else if (ui64TmdsSamplingFreq < 210000)
	{
		*samplingFreqFromTmds = LX_HDMI_SAMPLING_FREQ_192_KHZ;	//  192 kHz
		HDMI_PRINT("Get Fs from TDMS clock => LX_HDMI_SAMPLING_FREQ_192_KHZ\n");
	}
	else
	{
		*samplingFreqFromTmds = LX_HDMI_SAMPLING_FREQ_768_KHZ;	//  768 kHz(not supported)
		HDMI_PRINT("Get Fs from TDMS clock => LX_HDMI_SAMPLING_FREQ_768_KHZ(not supported)\n");
	}

	HDMI_ATASK("Get Fs from TDMS clock : %llu => %d\n", ui64TmdsSamplingFreq, *samplingFreqFromTmds);
//	HDMI_ERROR("Get Fs from TDMS clock : %llu => %d\n", ui64TmdsSamplingFreq, *samplingFreqFromTmds);

	return ret;
}

/**
 *  _HDMI_M14B0_DebugAudioInfo
 *
 *  @return int
*/
static int _HDMI_M14B0_DebugAudioInfo(LX_HDMI_DEBUG_AUDIO_INFO_T *pAudioDebugInfo)
{
	int ret = RET_OK;

#ifndef M14_THREAD_READ_PHY_REG_VALUE
	UINT8		ui8TmdsClockHigh = 0;		//< HDMI measured clock value of TMDS clock for upper 8 bit
	UINT8		ui8TmdsClockLow = 0;		//< HDMI measured clock value of TMDS clock for low 8 bit
#endif

	UINT64		ui64TmdsClock = 0;			//< HDMI measured clock value of TMDS clock
	UINT64		ui64AcrN = 0;				//< HDMI ACR N value
	UINT64		ui64AcrCts = 0;				//< HDMI ACR CTS value
	UINT64		ui64TmdsSamplingFreq = 0;	//< HDMI sampling frequency in source device from TMDS clock

	UINT32		reg_achst_byte0, reg_achst_byte1, reg_achst_byte2, reg_achst_byte3, reg_achst_byte4;
	UINT32		reg_burst_pc_0, reg_burst_pc_1, reg_burst_pd_0, reg_burst_pd_1;

	UINT32		sampling_freq, ext_sampling_freq;
	UINT16		burst_info_pc = 0, burst_info_pd = 0;

	//Get measured frequency value of TMDS clock.
#ifdef M14_THREAD_READ_PHY_REG_VALUE
	ui64TmdsClock = (UINT64)(_gM14B0HDMIPhyInform.tmds_clock ) * 10000;
HDMI_DEBUG("Get TDMS Clock : ui64TmdsClock = %llu\n", ui64TmdsClock);
#else
	PHY_REG_M14B0_RdFL(tmds_freq_1);
	PHY_REG_M14B0_RdFL(tmds_freq_2);

	PHY_REG_M14B0_Rd01(tmds_freq_1, tmds_freq, ui8TmdsClockHigh);
	PHY_REG_M14B0_Rd01(tmds_freq_2, tmds_freq, ui8TmdsClockLow);
	ui64TmdsClock = (UINT64)((ui8TmdsClockHigh << 8) | ui8TmdsClockLow) * 10000;
	HDMI_DEBUG("Get TDMS Clock : ui64TmdsClock = %llu, ui8TmdsClockHigh = 0x%X, ui8TmdsClockLow = %d\n", ui64TmdsClock, ui8TmdsClockHigh, ui8TmdsClockLow);
#endif

	//Get ACR N H/W value.
	LINK_M14_RdFL(audio_01);
	//LINK_M14_Rd01(audio_01, reg_acr_n_tx, ui64AcrN);
	ui64AcrN = LINK_M14_Rd(audio_01) & 0xFFFFF;		//20 bits

	//Get ACR CTS H/W value.
	LINK_M14_RdFL(audio_03);
	//LINK_M14_Rd01(audio_03, reg_acr_cts_tx, ui64AcrCts);
	ui64AcrCts = LINK_M14_Rd(audio_03) & 0xFFFFF;	//20 bits
	HDMI_DEBUG("Get TDMS ACR  : ui64AcrN = %llu, ui64AcrCts = %llu\n", ui64AcrN, ui64AcrCts);

	//Compute a sampling frequency from TMDS clock
	ui64TmdsSamplingFreq = ui64AcrN * ui64TmdsClock;

	//Check divide by zero value.
	if ( (ui64TmdsSamplingFreq > 0) && (ui64AcrCts > 0) )
	{
		do_div(ui64TmdsSamplingFreq, ui64AcrCts * 128);
	}

	HDMI_DEBUG("TMDS Clock = %llu, ACR N = %llu, ACR CTS = %llu, Tmds Fs = %llu\n",	\
				ui64TmdsSamplingFreq, ui64AcrN, ui64AcrCts, ui64TmdsSamplingFreq);

	//Read  reg_achst_byte0 reg.
	LINK_M14_RdFL(audio_07);
	LINK_M14_Rd01(audio_07, reg_achst_byte0, reg_achst_byte0);

	//Check IEC60958 Channel Status Byte0
	HDMI_DEBUG("IEC60958 Channel Status Byte0 = 0x%X(0x%X)\n", reg_achst_byte0, LINK_M14_Rd(audio_07));

	//Read  reg_achst_byte1 ~ 4 reg.
	LINK_M14_RdFL(audio_08);
	LINK_M14_Rd01(audio_08, reg_achst_byte1, reg_achst_byte1);
	LINK_M14_Rd01(audio_08, reg_achst_byte2, reg_achst_byte2);
	LINK_M14_Rd01(audio_08, reg_achst_byte3, reg_achst_byte3);
	LINK_M14_Rd01(audio_08, reg_achst_byte4, reg_achst_byte4);

	//Check IEC60958 Channel Status Byte1 ~ 4
	HDMI_DEBUG("IEC60958 Channel Status Byte1 = 0x%X, Byte2 = 0x%X, Byte3 = 0x%X, Byte4 = 0x%X(0x%X)\n",	\
				reg_achst_byte1, reg_achst_byte2, reg_achst_byte3, reg_achst_byte4, LINK_M14_Rd(audio_08));


	if (reg_achst_byte0 & 0x1)
		HDMI_DEBUG("Consumer use of channel status block is error!!!\n");
	else
		HDMI_DEBUG("Consumer use of channel status block.\n");


	if (reg_achst_byte0 & 0x2)
		HDMI_DEBUG("Audio sample word used for other purposes than liner PCM.\n");
	else
		HDMI_DEBUG("Audio sample word used for liner PCM.\n");


	if (reg_achst_byte0 & 0x4)
		HDMI_DEBUG("Software for which no copyright is asserted.\n");
	else
		HDMI_DEBUG("Software for which copyright is asserted.\n");


	HDMI_DEBUG("Category code  = 0x%X\n", reg_achst_byte1);

	HDMI_DEBUG("Source number  = %d\n", reg_achst_byte2 & 0x0F);


	HDMI_DEBUG("Channel number = %d\n", reg_achst_byte2 & 0xF0);
	if ((reg_achst_byte2 & 0xF0) == 0x10)
		HDMI_DEBUG("Left channel for stereo channel format.\n");

	if ((reg_achst_byte2 & 0xF0) == 0x20)
		HDMI_DEBUG("Right channel for stereo channel format.\n");

	//Set Sampling frequency from IEC60958 Channel Status Byte 3
	sampling_freq = reg_achst_byte3 & HDMI_AUDIO_SAMPLE_BIT_MASK; 	//bit 0 ~ 3
	HDMI_DEBUG("Sampling frequency = %d\n", sampling_freq);

	//Get a extension sampling frequency from IEC60958 Channel Status Byte 3
	ext_sampling_freq = (reg_achst_byte3 >> 6) | (reg_achst_byte3 & HDMI_AUDIO_SAMPLE_BIT_MASK);
	HDMI_DEBUG("Extension sampling frequency = %d\n", ext_sampling_freq & HDMI_AUDIO_EXT_SAMPLE_BIT_MASK);

	switch(ext_sampling_freq & HDMI_AUDIO_EXT_SAMPLE_BIT_MASK) 	//bit 0 ~ 3, bit 6 ~7
	{
		case HDMI_AUDIO_EXT_SAMPLE_384_KHZ:
			HDMI_DEBUG("HDMI_AUDIO_EXT_SAMPLE_384_KHZ\n");
			break;

		case HDMI_AUDIO_EXT_SAMPLE_1536_KHZ:
			HDMI_DEBUG("HDMI_AUDIO_EXT_SAMPLE_1536_KHZ\n");
			break;

		case HDMI_AUDIO_EXT_SAMPLE_1024_KHZ:
			HDMI_DEBUG("HDMI_AUDIO_EXT_SAMPLE_1024_KHZ\n");
			break;

		case HDMI_AUDIO_EXT_SAMPLE_3528_KHZ:
			HDMI_DEBUG("HDMI_AUDIO_EXT_SAMPLE_3528_KHZ\n");
			break;

		case HDMI_AUDIO_EXT_SAMPLE_7056_KHZ:
			HDMI_DEBUG("HDMI_AUDIO_EXT_SAMPLE_7056_KHZ\n");
			break;

		case HDMI_AUDIO_EXT_SAMPLE_14112_KHZ:
			HDMI_DEBUG("HDMI_AUDIO_EXT_SAMPLE_14112_KHZ\n");
			break;

		case HDMI_AUDIO_EXT_SAMPLE_64_KHZ:
			HDMI_DEBUG("HDMI_AUDIO_EXT_SAMPLE_64_KHZ\n");
			break;

		case HDMI_AUDIO_EXT_SAMPLE_128_KHZ:
			HDMI_DEBUG("HDMI_AUDIO_EXT_SAMPLE_128_KHZ\n");
			break;

		case HDMI_AUDIO_EXT_SAMPLE_256_KHZ:
			HDMI_DEBUG("HDMI_AUDIO_EXT_SAMPLE_256_KHZ\n");
			break;

		case HDMI_AUDIO_EXT_SAMPLE_512_KHZ:
			HDMI_DEBUG("HDMI_AUDIO_EXT_SAMPLE_512_KHZ\n");
			break;

		default:
			HDMI_DEBUG("ext_sampling_freq = %d\n", ext_sampling_freq);
			break;
	}


	if ((reg_achst_byte3 & 0x0F) == 0)
		HDMI_DEBUG("Symbol frequency = 64 X 44.1KHz = 2.8224 MHz.\n");

	if ((reg_achst_byte3 & 0x0F) == 2)
		HDMI_DEBUG("Symbol frequency = 64 X 48KHz = 3.072 MHz.\n");

	if ((reg_achst_byte3 & 0x0F) == 3)
		HDMI_DEBUG("Symbol frequency = 64 X 32KHz = 2.048 MHz.\n");


	if ((reg_achst_byte3 & 0x30) == 0x00)
		HDMI_DEBUG("Clock accuracy is Level II.\n");

	if ((reg_achst_byte3 & 0x30) == 0x10)
		HDMI_DEBUG("Clock accuracy is Level I.\n");

	if ((reg_achst_byte3 & 0x30) == 0x20)
		HDMI_DEBUG("Clock accuracy is Level III.\n");

	if ((reg_achst_byte3 & 0x30) == 0x30)
		HDMI_DEBUG("Interface frame rate not matched to sampling frequency.\n");


	if (reg_achst_byte4 & 0x01)
		HDMI_DEBUG("Maximum audio sample word length is 24 bits.\n");
	else
		HDMI_DEBUG("Maximum audio sample word length is 20 bits.\n");


	if ((reg_achst_byte4 & 0x01) == 0)
		HDMI_DEBUG("Sample word lenth is not indicated.\n");

	if ((reg_achst_byte4 & 0x01) == 1)
		HDMI_DEBUG("Sample word lenth is 20 or 16 bits.\n");

	if ((reg_achst_byte4 & 0x0E) == 2)
		HDMI_DEBUG("Sample word lenth is 22 or 18 bits.\n");

	if ((reg_achst_byte4 & 0x0E) == 4)
		HDMI_DEBUG("Sample word lenth is 23 or 19 bits.\n");

	if ((reg_achst_byte4 & 0x0E) == 10)
		HDMI_DEBUG("Sample word lenth is 24 or 20 bits.\n");

	if ((reg_achst_byte4 & 0x0E) == 12)
		HDMI_DEBUG("Sample word lenth is 21 or 17 bits.\n");


	if ((reg_achst_byte4 >> 4) == 16)
		HDMI_DEBUG("Original sampling frequency is 44.1 kHz.\n");

	if ((reg_achst_byte4 >> 4) == 7)
		HDMI_DEBUG("Original sampling frequency is 88.2 kHz.\n");

	if ((reg_achst_byte4 >> 4) == 11)
		HDMI_DEBUG("Original sampling frequency is 22.05 kHz.\n");

	if ((reg_achst_byte4 >> 4) == 3)
		HDMI_DEBUG("Original sampling frequency is 176.4 kHz.\n");

	if ((reg_achst_byte4 >> 4) == 13)
		HDMI_DEBUG("Original sampling frequency is 48 kHz.\n");

	if ((reg_achst_byte4 >> 4) == 5)
		HDMI_DEBUG("Original sampling frequency is 96 kHz.\n");

	if ((reg_achst_byte4 >> 4) == 9)
		HDMI_DEBUG("Original sampling frequency is 24 kHz.\n");

	if ((reg_achst_byte4 >> 4) == 1)
		HDMI_DEBUG("Original sampling frequency is 192 kHz.\n");

	if ((reg_achst_byte4 >> 4) == 14)
		HDMI_DEBUG("Original sampling frequency is 128 kHz.\n");

	if ((reg_achst_byte4 >> 4) == 6)
		HDMI_DEBUG("Original sampling frequency is 8 kHz.\n");

	if ((reg_achst_byte4 >> 4) == 10)
		HDMI_DEBUG("Original sampling frequency is 11.025 kHz.\n");

	if ((reg_achst_byte4 >> 4) == 2)
		HDMI_DEBUG("Original sampling frequency is 12 kHz.\n");

	if ((reg_achst_byte4 >> 4) == 12)
		HDMI_DEBUG("Original sampling frequency is 32 kHz.\n");

	if ((reg_achst_byte4 >> 4) == 4)
		HDMI_DEBUG("Original sampling frequency is 64 kHz.\n");

	if ((reg_achst_byte4 >> 4) == 8)
		HDMI_DEBUG("Original sampling frequency is 16 kHz.\n");

	if ((reg_achst_byte4 >> 4) == 0)
		HDMI_DEBUG("Original sampling frequency is not indicated.\n");


	//Check IEC61937 Burst Info
	if (reg_achst_byte0 & HDMI_AUDIO_SAMPLE_NON_PCM_MASK)
	{
		//Get a vaild Pc, Pd data for payload
		LINK_M14_RdFL(audio_09);
		LINK_M14_Rd01(audio_09, reg_burst_pc_0, reg_burst_pc_0);
		LINK_M14_Rd01(audio_09, reg_burst_pc_1, reg_burst_pc_1);
		LINK_M14_Rd01(audio_09, reg_burst_pd_0, reg_burst_pd_0);
		LINK_M14_Rd01(audio_09, reg_burst_pd_1, reg_burst_pd_1);

		burst_info_pc = (UINT16)((reg_burst_pc_1 << 8)|reg_burst_pc_0);
		burst_info_pd = (UINT16)((reg_burst_pd_1 << 8)|reg_burst_pd_0);

		HDMI_DEBUG("IEC61937 Burst Info Pc = 0x%X, Pd = 0x%X\n", burst_info_pc, burst_info_pd);

		if (burst_info_pc & BURST_INFO_PAYLOAD_ERROR_BIT_MASK)
			HDMI_DEBUG("Error-flag indicationg that the burst-payload may contain errors.\n");
		else
			HDMI_DEBUG("Error-flag indicationg a vaild burst-payload.\n");


		//Set Audio Data-Types according to IEC61937-2 Burst Info Preamble C
		switch(burst_info_pc & BURST_INFO_AUDIO_TYPE_BIT_MASK)	//bit 4 ~ 0
		{
			case BURST_INFO_AUDIO_TYPE_AC3:
			case BURST_INFO_AUDIO_TYPE_AC3_ENHANCED:
				HDMI_DEBUG("BURST_INFO_AUDIO_TYPE_AC3_X(Audio type in Pc = %d)\n", burst_info_pc & 0x001F);
				break;

			case BURST_INFO_AUDIO_TYPE_DTS_I:
			case BURST_INFO_AUDIO_TYPE_DTS_II:
			case BURST_INFO_AUDIO_TYPE_DTS_III:
			case BURST_INFO_AUDIO_TYPE_DTS_IV:
				HDMI_DEBUG("BURST_INFO_AUDIO_TYPE_DTS_X(Audio type in Pc = %d)\n", burst_info_pc & 0x001F);
				break;

			case BURST_INFO_AUDIO_TYPE_MPEG1_L1:
			case BURST_INFO_AUDIO_TYPE_MPEG1_L23:
			case BURST_INFO_AUDIO_TYPE_MPEG2_EXT:
			case BURST_INFO_AUDIO_TYPE_MPEG2_L1:
			case BURST_INFO_AUDIO_TYPE_MPEG2_L2:
			case BURST_INFO_AUDIO_TYPE_MPEG2_L3:
				HDMI_DEBUG("BURST_INFO_AUDIO_TYPE_MPEG1_XX(Audio type in Pc = %d)\n", burst_info_pc & 0x001F);
				break;

			case BURST_INFO_AUDIO_TYPE_MPEG2_AAC:
			case BURST_INFO_AUDIO_TYPE_MPEG2_AAC_LOW:
			case BURST_INFO_AUDIO_TYPE_MPEG4_AAC:
				HDMI_DEBUG("BURST_INFO_AUDIO_TYPE_MPEG2_AAC_XX(Audio type in Pc = %d)\n", burst_info_pc & 0x001F);
				break;

			case BURST_INFO_AUDIO_TYPE_ATRAC:
			case BURST_INFO_AUDIO_TYPE_ATRAC_23:
			case BURST_INFO_AUDIO_TYPE_ATRAC_X:
				HDMI_DEBUG("BURST_INFO_AUDIO_TYPE_ATRAC_XX(Audio type in Pc = %d)\n", burst_info_pc & 0x001F);
				break;

			case BURST_INFO_AUDIO_TYPE_WMA_I_IV:
				HDMI_DEBUG("BURST_INFO_AUDIO_TYPE_WMA_I_IV(Audio type in Pc = %d)\n", burst_info_pc & 0x001F);
				break;

			case BURST_INFO_AUDIO_TYPE_MAT:
				HDMI_DEBUG("BURST_INFO_AUDIO_TYPE_MAT(Audio type in Pc = %d)\n", burst_info_pc & 0x001F);
				break;

			case BURST_INFO_AUDIO_TYPE_NULL:
				HDMI_DEBUG("BURST_INFO_AUDIO_TYPE_NULL(Audio type in Pc = %d)\n", burst_info_pc & 0x001F);
				break;

			case BURST_INFO_AUDIO_TYPE_SMPTE_338M:
				HDMI_DEBUG("BURST_INFO_AUDIO_TYPE_SMPTE_338M(Audio type in Pc = %d)\n", burst_info_pc & 0x001F);
				break;

			case BURST_INFO_AUDIO_TYPE_PAUSE:
				HDMI_DEBUG("BURST_INFO_AUDIO_TYPE_PAUSE(Audio type in Pc = %d)\n", burst_info_pc & 0x001F);
				break;

			default:
				HDMI_DEBUG("LX_HDMI_DEBUG_NO_AUDIO(Audio type in Pc = %d)\n", burst_info_pc & 0x001F);
				break;
		}

		HDMI_DEBUG("Data-type-dependent info. = %d\n", burst_info_pc & BURST_INFO_DEPENDENT_INFO_BIT_MASK);
		HDMI_DEBUG("Bitstream number = %d\n", burst_info_pc & BURST_INFO_STREAM_NUMBER_BIT_MASK);


		HDMI_DEBUG("Length-code : Number of bits = %d\n", burst_info_pd);
	}

	//Copy a HMDI audio debug information.
	pAudioDebugInfo->tmdsClock	 	  = ui64TmdsClock;
	pAudioDebugInfo->tmdsSamplingFreq = ui64TmdsSamplingFreq;

	//Copy a HMDI audio channel status byte debug information.
	pAudioDebugInfo->chanStatusByte0  = (UINT8)reg_achst_byte0;
	pAudioDebugInfo->chanStatusByte1  = (UINT8)reg_achst_byte1;
	pAudioDebugInfo->chanStatusByte2  = (UINT8)reg_achst_byte2;
	pAudioDebugInfo->chanStatusByte3  = (UINT8)reg_achst_byte3;
	pAudioDebugInfo->chanStatusByte4  = (UINT8)reg_achst_byte4;

	//Copy a HMDI audio burst info. debug information.
	pAudioDebugInfo->burstInfoPc	  = burst_info_pc;
	pAudioDebugInfo->burstInfoPd	  = burst_info_pd;

	return ret;
}


static int _HDMI_M14B0_Thread(void *data)
{

	ULONG	flags = 0;
	int	count, port_num;
	int hdmi5v[4];
	int cd_sense;

	//*  2012-06-15 : HDMI Initail ÁÖÀÇ »çÇ×
	//*  Ctop reset ÀÌÈÄ phy pdb & restn µ¿ÀÛ ÇØ¾ß ÇÔ(Ctop ¿¡¼­ phy pdb¸¸ ÇÏ´Â °Í °°À½.
	//*  After phy pdb, phy register initi value °¡ ¿À¿°µÊ -> pdb ÈÄ¿¡´Â rstn resetÇØÁÖ¾î¾ß ÇÔ.
	//*  Phy pdb & restn ÇÏ¸é phy register initi value·Î º¯°æ µÊ,  ´Ù½Ã phy register setting ÇØ¾ß ÇÔ.
	//*
	_gMHL_RCP_RCV_MSG.rcp_receive_cmd = HDMI_RCP_RECEIVE_CMD_NONE;
	_gMHL_RCP_RCV_MSG.rcp_receive_flag = FALSE;
	_gMHL_RCP_RCV_MSG.rcp_msg = 0x7F;

	_HDMI_M14B0_ClearSWResetAll();			//M14B0D Chip - Ctop control

	OS_MsecSleep(10);	// ms delay

#ifdef ODT_PDB_OFF_ON_WORKAROUND

	_HDMI_M14B0_Get_HDMI5V_Info_A4P(&hdmi5v[0], &hdmi5v[1] ,&hdmi5v[2], &hdmi5v[3]);

	for (port_num = 0; port_num < 4;port_num ++)
	{
		if(hdmi5v[port_num])
		{
			_HDMI_M14B0_Phy_Enable_Register_Access(port_num);
			_HDMI_M14B0_EQ_PDB_control(1);
			_g_ODT_PDB_Off_Time = jiffies_to_msecs(jiffies);
		}
	}


#ifdef HPD_ON_OFF_CONTROL_WORKAROUND
	OS_LockMutex(&g_HDMI_Sema);

	_g_ODT_PDB_Off_Time = jiffies_to_msecs(jiffies);

	for (port_num = 0; port_num < 4;port_num ++)
	{
		if(hdmi5v[port_num])
		{
			_HDMI_M14B0_Set_HPD_Out(port_num, 1);	// enable HPD
		}
	}

	if( hdmi5v[0] || hdmi5v[1] || hdmi5v[2] || hdmi5v[3] )
	{
		for(count =0; count < 20; count ++)
		{
			_g_ODT_PDB_On_Time = jiffies_to_msecs(jiffies);
			if ( (_g_ODT_PDB_On_Time - _g_ODT_PDB_Off_Time) > 200 )
				break;
			OS_MsecSleep(20);	// ms delay
		}

		for (port_num = 0; port_num < 4;port_num ++)
		{
			if(hdmi5v[port_num])
			{
				_HDMI_M14B0_Set_HPD_Out(port_num, 0);	// disable HPD
			}
		}
		_g_ODT_PDB_Off_Time = jiffies_to_msecs(jiffies);
	}
	OS_UnlockMutex(&g_HDMI_Sema);
#endif

	for(count =0; count < 20; count ++)
	{
		_g_ODT_PDB_On_Time = jiffies_to_msecs(jiffies);
		if ( ( (_g_ODT_PDB_On_Time - _g_ODT_PDB_Off_Time) > 600 ) && (_gM14B0HDMIPhyInform.hpd_enable == 1) )
			break;
		OS_MsecSleep(100);	// ms delay
	}

	for (port_num = 0; port_num < 4;port_num ++)
	{
		if(hdmi5v[port_num])
		{
			_HDMI_M14B0_Phy_Enable_Register_Access(port_num);
			_HDMI_M14B0_EQ_PDB_control(0);
		}
	}
#ifdef HPD_OFF_WORKAROUND_FOR_EXT_EDID
	/* HDMI port2 connected and using External EDID */
	if ( ( _gM14B0HDMIPhyInform.hpd_pol[1] == 0 ) && ( hdmi5v[1] == 1) )
	{
		HDMI_NOTI("---- HDMI_PHY:Port_2 Ext EDID and cable connected !!! : long HPD off time ");
		_g_HPD_Off_Time[1] = jiffies_to_msecs(jiffies);
	}
#endif
#endif

	_HDMI_M14B0_PhyOff(0);
	_HDMI_M14B0_PhyOff(1);
	_HDMI_M14B0_PhyOff(2);
	_HDMI_M14B0_PhyOff(3);

	LINK_M14_RdFL(system_control_00);
	LINK_M14_Rd01(system_control_00, reg_cd_sense_prt3, cd_sense);

	if (cd_sense)
		_HDMI_M14B0_Set_ManMHLMode(1, 0);

	//_HDMI_M14B0_Set_ManMHLMode(1, 0);
	_HDMI_M14B0_ResetPortControl(1);	// reset phy_link

	/* PHY_PDB=1, PHY_RSTN=1 À» µ¿½Ã¿¡ ÀÎ°¡ registerµéÀÌ Á¦´ë·Î pre-setµÇÁö ¾Ê´Â Çö»ó ¹ß»ýÇÔ. */
	/* PHY_PDB=1  -> 10us  -> PHY_RSTN=1  */
	OS_MsecSleep(2);	// ms delay

	//_HDMI_M14B0_PhyRunReset();	// init Phy register
	_HDMI_M14B0_RunReset();		// init Link register

	//audio setting
	_HDMI_M14B0_SetAudio();

	_HDMI_M14B0_ResetPortControl(0);	// Release reset phy_link

	_HDMI_M14B0_EnableInterrupt();


	/*
	_HDMI_M14B0_EnablePort(3, 1);	// Enable MHL Port

	OS_MsecSleep(10); // delay after enable port and phy register write


	_HDMI_M14B0_EnablePort(3, 0);
	*/

	while(1)
	{
		OS_LockMutex(&g_HDMI_Sema);

		_HDMI_M14B0_GetMHLConection();	//check MHL Connection
		if (gHDMI_Phy_Control.check_an_data)
			_HDMI_M14B0_SetPortConnection();	// port change after AN data valid
		_HDMI_M14B0_GetPortConnection();	//check connection
		_HDMI_M14B0_Periodic_Task();	//Video Task
		_HDMI_M14B0_GetAudioInfo();	//Audio Task
		_HDMI_M14B0_HDCP_Check_Status();	// check HDCP status

		OS_UnlockMutex(&g_HDMI_Sema);

		if ( _gM14B0HDMIPhyInform.hdmi5v[0] || _gM14B0HDMIPhyInform.hdmi5v[1] || _gM14B0HDMIPhyInform.hdmi5v[2] || _gM14B0HDMIPhyInform.hdmi5v[3])
			OS_MsecSleep(HDMI_THREAD_TIMEOUT);
		else
		{
			_gHdmi_no_connection_count ++;
			OS_MsecSleep(HDMI_THREAD_TIMEOUT << 2);

		}

		//	HDMI_TASK("HDMI Thread\n");

		if (_gM14B0Force_thread_sleep > 1)
		{
			HDMI_NOTI("Force sleep HDMI Thread\n");
			_gM14B0Force_thread_sleep  = 1;
			_gM14B0HDMI_thread_running = 0;
			interruptible_sleep_on(&WaitQueue_HDMI_M14B0);
			//gWait_return = wait_event_interruptible(WaitQueue_CVD, gForce_thread_sleep>0);
		}

		if (_gHdmi_no_connection_count > 10)
		{
			spin_lock_irqsave(&_gIntrHdmiM14B0VideoLock, flags);
			_gHdmi_no_connection_count = 0;
			_gM14B0Force_thread_sleep  = 1;
			_gM14B0HDMI_thread_running = 0;
			_gM14B0HDMIState = HDMI_STATE_DISABLE;
			spin_unlock_irqrestore(&_gIntrHdmiM14B0VideoLock, flags);

			if ( _gM14B0HDMIState == HDMI_STATE_DISABLE )
				interruptible_sleep_on(&WaitQueue_HDMI_M14B0);
			else
			{
				HDMI_NOTI("HDMI thread not sleeping : wakeup by interrupt\n");
			}
		}
	}
	return RET_OK;
}

static int _HDMI_M14B0_EnableInterrupt(void)
{
	LINK_M14_RdFL(interrupt_enable_00);
	LINK_M14_Wr01(interrupt_enable_00, intr_new_avi_enable, 0x0);			///< 0 intr_new_avi_enable
	LINK_M14_Wr01(interrupt_enable_00, intr_new_vsi_enable, 0x0);			///< 1 intr_new_vsi_enable
	LINK_M14_Wr01(interrupt_enable_00, intr_chg_vsi_vformat_enable, 0x1);	///< 2 intr_chg_vsi_vformat_enable
	LINK_M14_Wr01(interrupt_enable_00, intr_new_gcp_enable, 0x0);			///< 3 intr_new_gcp_enable
	LINK_M14_Wr01(interrupt_enable_00, intr_new_asp_enable, 0x0);			///< 4 intr_new_asp_enable
	LINK_M14_Wr01(interrupt_enable_00, intr_new_acr_enable, 0x0);			///< 5 intr_new_acr_enable
	LINK_M14_Wr01(interrupt_enable_00, intr_new_gbd_enable, 0x0);			///< 6 intr_new_gbd_enable
	LINK_M14_Wr01(interrupt_enable_00, intr_new_acp_enable, 0x0);			///< 7 intr_new_acp_enable
	LINK_M14_Wr01(interrupt_enable_00, intr_new_isrc1_enable, 0x0);			///< 8 intr_new_isrc1_enable
	LINK_M14_Wr01(interrupt_enable_00, intr_new_isrc2_enable, 0x0);			///< 9 intr_new_isrc2_enable
	LINK_M14_Wr01(interrupt_enable_00, intr_new_mpg_enable, 0x0);			///< 10 intr_new_mpg_enable
	LINK_M14_Wr01(interrupt_enable_00, intr_new_aud_enable, 0x0);			///< 11 intr_new_aud_enable
	LINK_M14_Wr01(interrupt_enable_00, intr_new_spd_enable, 0x0);			///< 12 intr_new_spd_enable
	LINK_M14_Wr01(interrupt_enable_00, intr_new_unr_enable, 0x0);			///< 13 intr_new_unr_enable
	LINK_M14_Wr01(interrupt_enable_00, intr_cts_chg_enable, 0x0);			///< 14 intr_cts_chg_enable
	LINK_M14_Wr01(interrupt_enable_00, intr_n_chg_enable, 0x0);				///< 15 intr_n_chg_enable
#ifdef M14_DISABLE_AUDIO_INTERRUPT
	LINK_M14_Wr01(interrupt_enable_00, intr_fs_chg_enable, 0x0);			///< 16 intr_fs_chg_enable
	LINK_M14_Wr01(interrupt_enable_00, intr_2pcm_chg_enable, 0x0);			///< 17 intr_2pcm_chg_enable
	LINK_M14_Wr01(interrupt_enable_00, intr_2npcm_chg_enable, 0x0);			///< 18 intr_2npcm_chg_enable
#else
	LINK_M14_Wr01(interrupt_enable_00, intr_fs_chg_enable, 0x1);			///< 16 intr_fs_chg_enable
	LINK_M14_Wr01(interrupt_enable_00, intr_2pcm_chg_enable, 0x1);			///< 17 intr_2pcm_chg_enable
	LINK_M14_Wr01(interrupt_enable_00, intr_2npcm_chg_enable, 0x1);			///< 18 intr_2npcm_chg_enable
#endif
	LINK_M14_Wr01(interrupt_enable_00, intr_spdif_err_enable, 0x0);			///< 19 intr_spdif_err_enable
	LINK_M14_Wr01(interrupt_enable_00, intr_afifo_undr_enable, 0x0);		///< 20 intr_afifo_undr_enable
	LINK_M14_Wr01(interrupt_enable_00, intr_afifo_ovrr_enable, 0x0);		///< 21 intr_afifo_ovrr_enable
	LINK_M14_Wr01(interrupt_enable_00, intr_burst_info_enable, 0x1);		///< 22 intr_burst_info_enable
	LINK_M14_Wr01(interrupt_enable_00, intr_vf_stable_enable, 0x1);			///< 23 intr_vf_stable_enable
	LINK_M14_Wr01(interrupt_enable_00, intr_vid_chg_enable, 0x0);			///< 24 intr_vid_chg_enable
	LINK_M14_Wr01(interrupt_enable_00, intr_vr_chg_enable, 0x0);			///< 25 intr_vr_chg_enable
	LINK_M14_Wr01(interrupt_enable_00, intr_hr_chg_enable, 0x0);			///< 26 intr_hr_chg_enable
	LINK_M14_Wr01(interrupt_enable_00, intr_po_chg_enable, 0x0);			///< 27 intr_po_chg_enable
	LINK_M14_Wr01(interrupt_enable_00, intr_il_chg_enable, 0x0);			///< 28 intr_il_chg_enable
	LINK_M14_Wr01(interrupt_enable_00, intr_vfifo_undr_enable, 0x0);		///< 29 intr_vfifo_undr_enable
	LINK_M14_Wr01(interrupt_enable_00, intr_vfifo_ovrr_enable, 0x0);		///< 30 intr_vfifo_ovrr_enable
	//intr_achst_5b_chg_int_enable is not working, so disabled.
	//no intr_achst_5b_chg_int_enable register in M14
	//LINK_M14_Wr01(interrupt_enable_00, intr_achst_5b_chg_int_enable, 0x0);	///< 21 intr_achst_5b_chg_int_enable
	LINK_M14_WrFL(interrupt_enable_00);

	LINK_M14_RdFL(interrupt_enable_01);
	LINK_M14_Wr01(interrupt_enable_01, intr_hdmi5v_redge_prt3_enable, 0x1);		///< 0 intr_hdmi5v_redge_prt3_enable
	LINK_M14_Wr01(interrupt_enable_01, intr_hdmi5v_redge_prt2_enable, 0x1);		///< 1 intr_hdmi5v_redge_prt2_enable
	LINK_M14_Wr01(interrupt_enable_01, intr_hdmi5v_redge_prt1_enable, 0x1);		///< 2 intr_hdmi5v_redge_prt1_enable
	LINK_M14_Wr01(interrupt_enable_01, intr_hdmi5v_redge_prt0_enable, 0x1);		///< 3 intr_hdmi5v_redge_prt0_enable
	LINK_M14_Wr01(interrupt_enable_01, intr_scdt_fedge_prt3_enable, 0x1);		///< 4 intr_scdt_fedge_prt3_enable
	LINK_M14_Wr01(interrupt_enable_01, intr_scdt_fedge_prt2_enable, 0x1);		///< 5 intr_scdt_fedge_prt2_enable
	LINK_M14_Wr01(interrupt_enable_01, intr_scdt_fedge_prt1_enable, 0x1);		///< 6 intr_scdt_fedge_prt1_enable
	LINK_M14_Wr01(interrupt_enable_01, intr_scdt_fedge_prt0_enable, 0x1);		///< 7 intr_scdt_fedge_prt0_enable
	LINK_M14_Wr01(interrupt_enable_01, intr_scdt_redge_prt3_enable, 0x0);		///< 8 intr_scdt_redge_prt3_enable
	LINK_M14_Wr01(interrupt_enable_01, intr_scdt_redge_prt2_enable, 0x0);		///< 9 intr_scdt_redge_prt2_enable
	LINK_M14_Wr01(interrupt_enable_01, intr_scdt_redge_prt1_enable, 0x0);		///< 10 intr_scdt_redge_prt1_enable
	LINK_M14_Wr01(interrupt_enable_01, intr_scdt_redge_prt0_enable, 0x0);		///< 11 intr_scdt_redge_prt0_enable
	LINK_M14_Wr01(interrupt_enable_01, intr_cd_sense_fedge_prt3_enable, 0x0);	///< 12 intr_cd_sense_fedge_prt3_enable
	LINK_M14_Wr01(interrupt_enable_01, intr_cd_sense_redge_prt3_enable, 0x0);	///< 13 intr_cd_sense_redge_prt3_enable
	LINK_M14_Wr01(interrupt_enable_01, intr_auth_init_prt0_enable, 0x0);				///< 14 intr_auth_init_enable
	LINK_M14_Wr01(interrupt_enable_01, intr_auth_done_prt0_enable, 0x0);				///< 15 intr_auth_done_enable
	LINK_M14_Wr01(interrupt_enable_01, intr_hdcp_err_enable, 0x0);				///< 16 intr_hdcp_err_enable
	LINK_M14_Wr01(interrupt_enable_01, intr_ecc_err_enable, 0x0);				///< 17 intr_ecc_err_enable
	LINK_M14_Wr01(interrupt_enable_01, intr_terc4_err_prt3_enable, 0x0);		///< 18 intr_terc4_err_prt3_enable
	LINK_M14_Wr01(interrupt_enable_01, intr_terc4_err_prt2_enable, 0x0);		///< 19 intr_terc4_err_prt2_enable
	LINK_M14_Wr01(interrupt_enable_01, intr_terc4_err_prt1_enable, 0x0);		///< 20 intr_terc4_err_prt1_enable
	LINK_M14_Wr01(interrupt_enable_01, intr_terc4_err_prt0_enable, 0x0);		///< 21 intr_terc4_err_prt0_enable
	LINK_M14_Wr01(interrupt_enable_01, intr_acr_err_enable, 0x0);				///< 22 intr_acr_err_enable
	LINK_M14_Wr01(interrupt_enable_01, intr_asp_err_enable, 0x0);				///< 23 intr_asp_err_enable
	LINK_M14_Wr01(interrupt_enable_01, intr_no_avi_enable, 0x0);				///< 24 intr_no_avi_enable
	LINK_M14_Wr01(interrupt_enable_01, intr_no_vsi_enable, 0x0);				///< 25 intr_no_vsi_enable
	LINK_M14_Wr01(interrupt_enable_01, intr_no_gcp_enable, 0x0);				///< 26 intr_no_gcp_enable
	LINK_M14_Wr01(interrupt_enable_01, intr_chg_avi_b12345_enable, 0x1);		///< 27 intr_chg_avi_b12345_enable
	LINK_M14_Wr01(interrupt_enable_01, intr_chg_avi_to_rgb_enable, 0x1);		///< 28 intr_chg_avi_to_rgb_enable
	LINK_M14_Wr01(interrupt_enable_01, intr_chg_avi_to_ycc444_enable, 0x1);		///< 29 intr_chg_avi_to_ycc444_enable
	LINK_M14_Wr01(interrupt_enable_01, intr_chg_avi_to_ycc422_enable, 0x1);		///< 30 intr_chg_avi_to_ycc422_enable
	LINK_M14_WrFL(interrupt_enable_01);

	LINK_M14_RdFL(interrupt_enable_02);
	LINK_M14_Wr01(interrupt_enable_02, intr_plug_in_prt1_enable, 0x0);			///<  0 intr_plug_in_prt1_enable
	LINK_M14_Wr01(interrupt_enable_02, intr_plug_in_prt0_enable, 0x0);			///<  1 intr_plug_in_prt0_enable
	LINK_M14_Wr01(interrupt_enable_02, int_hdmi5v_fall_prt3_enable, 0x0);		///<  2 int_hdmi5v_fall_prt3_enable
	LINK_M14_Wr01(interrupt_enable_02, int_hdmi5v_fall_prt2_enable, 0x0);		///<  3 int_hdmi5v_fall_prt2_enable
	LINK_M14_Wr01(interrupt_enable_02, int_hdmi5v_fall_prt1_enable, 0x0);		///<  4 int_hdmi5v_fall_prt1_enable
	LINK_M14_Wr01(interrupt_enable_02, int_hdmi5v_fall_prt0_enable, 0x0);		///<  5 int_hdmi5v_fall_prt0_enable
	LINK_M14_Wr01(interrupt_enable_02, int_hdmi5v_rise_prt3_enable, 0x0);		///<  6 int_hdmi5v_rise_prt3_enable
	LINK_M14_Wr01(interrupt_enable_02, int_hdmi5v_rise_prt2_enable, 0x0);		///<  7 int_hdmi5v_rise_prt2_enable
	LINK_M14_Wr01(interrupt_enable_02, int_hdmi5v_rise_prt1_enable, 0x0);		///<  8 int_hdmi5v_rise_prt1_enable
	LINK_M14_Wr01(interrupt_enable_02, int_hdmi5v_rise_prt0_enable, 0x0);		///<  9 int_hdmi5v_rise_prt0_enable
	LINK_M14_Wr01(interrupt_enable_02, intr_scdt_fall_prt3_enable, 0x0);		///< 10 intr_scdt_fall_prt3_enable
	LINK_M14_Wr01(interrupt_enable_02, intr_scdt_fall_prt2_enable, 0x0);		///< 11 intr_scdt_fall_prt2_enable
	LINK_M14_Wr01(interrupt_enable_02, intr_scdt_fall_prt1_enable, 0x0);		///< 12 intr_scdt_fall_prt1_enable
	LINK_M14_Wr01(interrupt_enable_02, intr_scdt_fall_prt0_enable, 0x0);		///< 13 intr_scdt_fall_prt0_enable
	LINK_M14_Wr01(interrupt_enable_02, intr_scdt_rise_prt3_enable, 0x0);		///< 14 intr_scdt_rise_prt3_enable
	LINK_M14_Wr01(interrupt_enable_02, intr_scdt_rise_prt2_enable, 0x0);		///< 15 intr_scdt_rise_prt2_enable
	LINK_M14_Wr01(interrupt_enable_02, intr_scdt_rise_prt1_enable, 0x0);		///< 16 intr_scdt_rise_prt1_enable
	LINK_M14_Wr01(interrupt_enable_02, intr_scdt_rise_prt0_enable, 0x0);		///< 17 intr_scdt_rise_prt0_enable
	LINK_M14_Wr01(interrupt_enable_02, intr_hdmi_mode_prt3_enable, 0x0);		///< 18 intr_hdmi_mode_prt3_enable
	LINK_M14_Wr01(interrupt_enable_02, intr_hdmi_mode_prt2_enable, 0x0);		///< 19 intr_hdmi_mode_prt2_enable
	LINK_M14_Wr01(interrupt_enable_02, intr_hdmi_mode_prt1_enable, 0x0);		///< 20 intr_hdmi_mode_prt1_enable
	LINK_M14_Wr01(interrupt_enable_02, intr_hdmi_mode_prt0_enable, 0x0);		///< 21 intr_hdmi_mode_prt0_enable
	LINK_M14_Wr01(interrupt_enable_02, intr_set_mute_enable, 0x1);				///< 22 intr_set_mute_enable
	LINK_M14_Wr01(interrupt_enable_02, intr_clr_mute_enable, 0x0);				///< 23 intr_clr_mute_enable
	LINK_M14_Wr01(interrupt_enable_02, intr_vsync_prt3_enable, 0x0);			///< 24 intr_vsync_prt3_enable
	LINK_M14_Wr01(interrupt_enable_02, intr_vsync_prt2_enable, 0x0);			///< 25 intr_vsync_prt2_enable
	LINK_M14_Wr01(interrupt_enable_02, intr_vsync_prt1_enable, 0x0);			///< 26 intr_vsync_prt1_enable
	LINK_M14_Wr01(interrupt_enable_02, intr_vsync_prt0_enable, 0x0);			///< 27 intr_vsync_prt0_enable
	LINK_M14_Wr01(interrupt_enable_02, intr_hdmi5v_fedge_prt3_enable, 0x0);		///< 28 intr_hdmi5v_fedge_prt3_enable
	LINK_M14_Wr01(interrupt_enable_02, intr_hdmi5v_fedge_prt2_enable, 0x0);		///< 29 intr_hdmi5v_fedge_prt2_enable
	LINK_M14_Wr01(interrupt_enable_02, intr_hdmi5v_fedge_prt1_enable, 0x0);		///< 30 intr_hdmi5v_fedge_prt1_enable
	LINK_M14_Wr01(interrupt_enable_02, intr_hdmi5v_fedge_prt0_enable, 0x0);		///< 31 intr_hdmi5v_fedge_prt0_enable
	LINK_M14_WrFL(interrupt_enable_02);

	LINK_M14_RdFL(interrupt_enable_03);
	LINK_M14_Wr01(interrupt_enable_03, intr_cbus_pathen_clr_enable     , 0x0);		///<  0 intr_cbus_pathen_clr_enable
	LINK_M14_Wr01(interrupt_enable_03, intr_cbus_mute_set_enable       , 0x0);		///<  1 intr_cbus_mute_set_enable
	LINK_M14_Wr01(interrupt_enable_03, intr_cbus_mute_clr_enable       , 0x0);		///<  2 intr_cbus_mute_clr_enable
	LINK_M14_Wr01(interrupt_enable_03, intr_cbus_new_mscerr_enable     , 0x0);		///<  3 intr_cbus_new_mscerr_enable
	LINK_M14_Wr01(interrupt_enable_03, intr_cbus_new_rcp_enable        , 0x1);		///<  4 intr_cbus_new_rcp_enable
	LINK_M14_Wr01(interrupt_enable_03, intr_cbus_cmd_set_hpd_enable    , 0x0);		///<  5 intr_cbus_cmd_set_hpd_enable
	LINK_M14_Wr01(interrupt_enable_03, intr_cbus_cmd_get_state_enable  , 0x0);		///<  6 intr_cbus_cmd_get_state_enable
	LINK_M14_Wr01(interrupt_enable_03, intr_cbus_cmd_get_vdid_enable   , 0x0);		///<  7 intr_cbus_cmd_get_vdid_enable
	LINK_M14_Wr01(interrupt_enable_03, intr_cbus_cmd_rd_devcap_enable  , 0x0);		///<  8 intr_cbus_cmd_rd_devcap_enable
	LINK_M14_Wr01(interrupt_enable_03, intr_cbus_cmd_rd_devintr_enable , 0x0);		///<  9 intr_cbus_cmd_rd_devintr_enable
	LINK_M14_Wr01(interrupt_enable_03, intr_cbus_cmd_pathen_set_enable , 0x0);		///< 10 intr_cbus_cmd_pathen_set_enable
	LINK_M14_Wr01(interrupt_enable_03, intr_cbus_cmd_pathen_clr_enable , 0x0);		///< 11 intr_cbus_cmd_pathen_clr_enable
	LINK_M14_Wr01(interrupt_enable_03, intr_cbus_cmd_dcaprd_set_enable , 0x0);		///< 12 intr_cbus_cmd_dcaprd_set_enable
	LINK_M14_Wr01(interrupt_enable_03, intr_cbus_cmd_3dreq_set_enable  , 0x0);		///< 13 intr_cbus_cmd_3dreq_set_enable
	LINK_M14_Wr01(interrupt_enable_03, intr_cbus_cmd_grtwrt_set_enable , 0x0);		///< 14 intr_cbus_cmd_grtwrt_set_enable
	LINK_M14_Wr01(interrupt_enable_03, intr_cbus_cmd_reqwrt_set_enable , 0x0);		///< 15 intr_cbus_cmd_reqwrt_set_enable
	LINK_M14_Wr01(interrupt_enable_03, intr_cbus_cmd_wrt_burst_enable  , 0x0);		///< 16 intr_cbus_cmd_wrt_burst_enable
	LINK_M14_Wr01(interrupt_enable_03, intr_cbus_cmd_dscrchg_set_enable, 0x0);		///< 17 intr_cbus_cmd_dscrchg_set_enable
	LINK_M14_Wr01(interrupt_enable_03, intr_cbus_cmd_dcapchg_set_enable, 0x0);		///< 18 intr_cbus_cmd_dcapchg_set_enable
	LINK_M14_Wr01(interrupt_enable_03, intr_cbus_cmd_edidchg_set_enable, 0x0);		///< 19 intr_cbus_cmd_edidchg_set_enable
	LINK_M14_Wr01(interrupt_enable_03, intr_cbus_cmd_clr_hpd_enable    , 0x0);		///< 20 intr_cbus_cmd_clr_hpd_enable
	LINK_M14_Wr01(interrupt_enable_03, intr_cbus_cmd_rap_poll_enable   , 0x0);		///< 21 intr_cbus_cmd_rap_poll_enable
	LINK_M14_Wr01(interrupt_enable_03, intr_cbus_cmd_rap_on_enable     , 0x0);		///< 22 intr_cbus_cmd_rap_on_enable
	LINK_M14_Wr01(interrupt_enable_03, intr_cbus_cmd_rap_off_enable    , 0x0);		///< 23 intr_cbus_cmd_rap_off_enable
	LINK_M14_Wr01(interrupt_enable_03, intr_cbus_cmd_rcp_enable        , 0x1);		///< 24 intr_cbus_cmd_rcp_enable
	LINK_M14_Wr01(interrupt_enable_03, intr_cbus_cmd_msc_err_enable    , 0x0);		///< 25 intr_cbus_cmd_msc_err_enable
	LINK_M14_Wr01(interrupt_enable_03, intr_plug_out_prt3_enable       , 0x0);		///< 26 intr_plug_out_prt3_enable
	LINK_M14_Wr01(interrupt_enable_03, intr_plug_out_prt2_enable       , 0x0);		///< 27 intr_plug_out_prt2_enable
	LINK_M14_Wr01(interrupt_enable_03, intr_plug_out_prt1_enable       , 0x0);		///< 28 intr_plug_out_prt1_enable
	LINK_M14_Wr01(interrupt_enable_03, intr_plug_out_prt0_enable       , 0x0);		///< 29 intr_plug_out_prt0_enable
	LINK_M14_Wr01(interrupt_enable_03, intr_plug_in_prt3_enable        , 0x0);		///< 30 intr_plug_in_prt3_enable
	LINK_M14_Wr01(interrupt_enable_03, intr_plug_in_prt2_enable        , 0x0);		///< 31 intr_plug_in_prt2_enable
	LINK_M14_WrFL(interrupt_enable_03);

	LINK_M14_RdFL(interrupt_enable_04);
	LINK_M14_Wr01(interrupt_enable_04, intr_cbus_ready_enable           , 0x0);		///< 0 intr_cbus_ready_enable
	LINK_M14_Wr01(interrupt_enable_04, intr_cbus_disconn_enable         , 0x1);		///< 1 intr_cbus_disconn_enable
	LINK_M14_Wr01(interrupt_enable_04, intr_cbus_max_nack_enable        , 0x0);		///< 2 intr_cbus_max_nack_enable
	LINK_M14_Wr01(interrupt_enable_04, intr_cbus_max_retx_enable        , 0x0);		///< 3 intr_cbus_max_retx_enable
	LINK_M14_Wr01(interrupt_enable_04, intr_cbus_max_rerx_enable        , 0x0);		///< 4 intr_cbus_max_rerx_enable
	LINK_M14_Wr01(interrupt_enable_04, intr_cbus_new_devstate_enable    , 0x0);		///< 5 intr_cbus_new_devstate_enable
	LINK_M14_Wr01(interrupt_enable_04, intr_cbus_new_vdid_enable        , 0x0);		///< 6 intr_cbus_new_vdid_enable
	LINK_M14_Wr01(interrupt_enable_04, intr_cbus_new_devcap_enable      , 0x0);		///< 7 intr_cbus_new_devcap_enable
	LINK_M14_Wr01(interrupt_enable_04, intr_cbus_chg_dscr_src_enable    , 0x0);		///< 8 intr_cbus_chg_dscr_src_enable
	LINK_M14_Wr01(interrupt_enable_04, intr_cbus_chg_dscr_sink_enable   , 0x0);		///< 9 intr_cbus_chg_dscr_sink_enable
	LINK_M14_Wr01(interrupt_enable_04, intr_cbus_chg_ppmode_enable      , 0x0);		///< 10 intr_cbus_chg_ppmode_enable
	LINK_M14_Wr01(interrupt_enable_04, intr_cbus_chg_24mode_enable      , 0x0);		///< 11 intr_cbus_chg_24mode_enable
	LINK_M14_Wr01(interrupt_enable_04, intr_cbus_pathen_set_enable      , 0x0);		///< 12 intr_cbus_pathen_set_enable
	LINK_M14_Wr01(interrupt_enable_04, intr_cbus_new_dscr_enable         , 0x0);		///< 13 intr_cbus_new_dscr_enable
	LINK_M14_Wr01(interrupt_enable_04, intr_cbus_new_intstat_enable      , 0x0);		///< 14 intr_cbus_new_intstat_enable
	LINK_M14_Wr01(interrupt_enable_04, intr_cbus_new_dcap_enable         , 0x0);		///< 15 intr_cbus_new_dcap_enable
	LINK_M14_Wr01(interrupt_enable_04, intr_cbus_new_ucp_enable          , 0x0);		///< 16 intr_cbus_new_ucp_enable
	LINK_M14_Wr01(interrupt_enable_04, intr_cbus_cmd_ucp_enable          , 0x0);		///< 17 intr_cbus_cmd_ucp_enable
	LINK_M14_Wr01(interrupt_enable_04, intr_auth_init_prt3_enable        , 0x0);		///< 18 intr_auth_init_prt3_enable
	LINK_M14_Wr01(interrupt_enable_04, intr_auth_init_prt2_enable        , 0x0);		///< 19 intr_auth_init_prt2_enable
	LINK_M14_Wr01(interrupt_enable_04, intr_auth_init_prt1_enable        , 0x0);		///< 20 intr_auth_init_prt1_enable
	LINK_M14_Wr01(interrupt_enable_04, intr_auth_done_prt3_enable        , 0x0);		///< 21 intr_auth_done_prt3_enable
	LINK_M14_Wr01(interrupt_enable_04, intr_auth_done_prt2_enable        , 0x0);		///< 22 intr_auth_done_prt2_enable
	LINK_M14_Wr01(interrupt_enable_04, intr_auth_done_prt1_enable        , 0x0);		///< 23 intr_auth_done_prt1_enable
	LINK_M14_WrFL(interrupt_enable_04);

	if (_gM14B0HDMI_interrupt_registered < 1)
	{
		if (request_irq(M14_B0_IRQ_HDMI_LINK, _HDMI_M14B0_IRQHandler, 0, "hdmi", NULL))
		{
			HDMI_ERROR("request_irq is failed\n");
		}
		else
			_gM14B0HDMI_interrupt_registered = 1;
	}

	HDMI_NOTI("HDMI_M14B0_EnableInterrupt\n");
	return RET_OK;
}

int HDMI_M14B0_DisableInterrupt(void)
{
	LINK_M14_RdFL(interrupt_enable_00);
	LINK_M14_Wr(interrupt_enable_00, 0x0);
	LINK_M14_WrFL(interrupt_enable_00);

	LINK_M14_RdFL(interrupt_enable_01);
	LINK_M14_Wr(interrupt_enable_01, 0x0);
	LINK_M14_WrFL(interrupt_enable_01);

	LINK_M14_RdFL(interrupt_enable_02);
	LINK_M14_Wr(interrupt_enable_02, 0x0);
	LINK_M14_WrFL(interrupt_enable_02);

	LINK_M14_RdFL(interrupt_enable_03);
	LINK_M14_Wr(interrupt_enable_03, 0x0);
	LINK_M14_WrFL(interrupt_enable_03);

	LINK_M14_RdFL(interrupt_enable_04);
	LINK_M14_Wr(interrupt_enable_04, 0x0);
	LINK_M14_WrFL(interrupt_enable_04);

	//free_irq(M14_IRQ_HDMI, NULL);
	HDMI_NOTI("HDMI_M14B0_DisableInterrupt\n");

	return RET_OK;
}

/**
 *
 * HDMI_L9Bx_irq_handler irq handler
 *
 * @param	irq , device id , regs
 * @return	0 : OK , -1 : NOK
 *
*/
irqreturn_t _HDMI_M14B0_IRQHandler(int irq, void *dev)
{
	ULONG	flags = 0;
	LX_HDMI_MUTE_CTRL_T 		muteCtrl = {FALSE, FALSE, LX_HDMI_MUTE_NONE};
	LX_HDMI_AVI_COLORSPACE_T	csc = LX_HDMI_AVI_COLORSPACE_RGB;

	UINT32 intra0 = 0;
	UINT32 intra1 = 0;
	UINT32 intra2 = 0;
	UINT32 intra3 = 0;
	UINT32 intra4 = 0;
	int	prt_selected;

	UINT64	elapsedTime = 0;

	LINK_M14_RdFL(interrupt_00);
	intra0 = LINK_M14_Rd(interrupt_00);

	LINK_M14_RdFL(interrupt_01);
	intra1 = LINK_M14_Rd(interrupt_01);

	LINK_M14_RdFL(interrupt_02);
	intra2 = LINK_M14_Rd(interrupt_02);

	LINK_M14_RdFL(interrupt_03);
	intra3 = LINK_M14_Rd(interrupt_03);

	LINK_M14_RdFL(interrupt_04);
	intra4 = LINK_M14_Rd(interrupt_04);

#if 1
	if ( LINK_M14_Rd_fld(interrupt_01, intr_ecc_err) )
	{
		_HDMI_M14B0_Disable_TMDS_Error_Interrupt();

		if (_gM14B0_TMDS_ERROR_EQ != -1)
			_gM14B0_TMDS_ERROR_EQ ++;
		if (_gM14B0_TMDS_ERROR_EQ_2nd != -1)
			_gM14B0_TMDS_ERROR_EQ_2nd ++;
		
		_gM14B0_TMDS_ERROR_intr_count ++;

		HDMI_INTR("intr_ecc_err \n");
	}
	if (LINK_M14_Rd_fld(interrupt_01, intr_scdt_fedge_prt3))
	{
		_HDMI_M14B0_TMDS_ResetPort_Control(3 ,0);
		_SCDT_Fall_Detected[3]++;
		HDMI_INTR("intr_scdt_fedge_prt3\n");
	}
	if (LINK_M14_Rd_fld(interrupt_01, intr_scdt_fedge_prt2))
	{
		_HDMI_M14B0_TMDS_ResetPort_Control(2 ,0);
		_SCDT_Fall_Detected[2]++;
		HDMI_INTR("intr_scdt_fedge_prt2\n");
	}
	if (LINK_M14_Rd_fld(interrupt_01, intr_scdt_fedge_prt1))
	{
		_HDMI_M14B0_TMDS_ResetPort_Control(1 ,0);
		_SCDT_Fall_Detected[1]++;
		HDMI_INTR("intr_scdt_fedge_prt1\n");
	}
	if (LINK_M14_Rd_fld(interrupt_01, intr_scdt_fedge_prt0))
	{
		_HDMI_M14B0_TMDS_ResetPort_Control(0 ,0);
		_SCDT_Fall_Detected[0]++;
		HDMI_INTR("intr_scdt_fedge_prt0\n");
	}

	LINK_M14_RdFL(system_control_00);
	LINK_M14_Rd01(system_control_00, reg_prt_sel, prt_selected);

	if ( ( (prt_selected == 0) && LINK_M14_Rd_fld(interrupt_01, intr_scdt_fedge_prt0)) \
			|| ( (prt_selected == 1) && LINK_M14_Rd_fld(interrupt_01, intr_scdt_fedge_prt1)) \
			|| ( (prt_selected == 2) && LINK_M14_Rd_fld(interrupt_01, intr_scdt_fedge_prt2)) \
			|| ( (prt_selected == 3) && LINK_M14_Rd_fld(interrupt_01, intr_scdt_fedge_prt3)) )
	{
		spin_lock_irqsave(&_gIntrHdmiM14B0VideoLock, flags);
		_gM14B0Intr_vf_stable = HDMI_VIDEO_INIT_STATE;
		_gM14B0Intr_avi = HDMI_AVI_INIT_STATE;
		_gM14B0Intr_vsi = HDMI_VSI_INIT_STATE;
		_gM14B0Intr_src_mute = HDMI_SOURCE_MUTE_CLEAR_STATE;
		_gM14B0TimingReadCnt = 0;
		_gM14B0AVIReadState = FALSE;
		_gM14B0VSIState = FALSE;
		_gM14B0AVIChangeState = FALSE;
		spin_unlock_irqrestore(&_gIntrHdmiM14B0VideoLock, flags);
	}

	if (LINK_M14_Rd_fld(interrupt_04, intr_cbus_disconn))
	{
		LINK_M14_RdFL(cbus_00);
		LINK_M14_Wr01(cbus_00, reg_cbus_disconn_en, 0x0);
		LINK_M14_WrFL(cbus_00);

		// set CBUS_04 to initiali state
		_HDMI_M14B0_MHL_Check_Status(1);

#ifdef M14_CBUS_PDB_CTRL
		wake_up_interruptible(&WaitQueue_MHL_PDB_M14B0);
#endif

		HDMI_DEBUG("---- MHL Clear cbus_disconn_en\n");
		HDMI_INTR("intr_cbus_disconn\n");
	}

	HDMI_INTR("--- intra0 [0x%x] - intra1 [0x%x] - intra2 [0x%x] - intra3 [0x%x] - intra4 [0x%x]  -----\n",intra0, intra1, intra2, intra3, intra4);

	if (LINK_M14_Rd_fld(interrupt_00, intr_vf_stable))
	{
		HDMI_DEBUG("intr_vf_stable\n");

		spin_lock_irqsave(&_gIntrHdmiM14B0VideoLock, flags);
		_gM14B0Intr_vf_stable = HDMI_VIDEO_INTERRUPT_STATE;
		spin_unlock_irqrestore(&_gIntrHdmiM14B0VideoLock, flags);

		//spin lock for protection for audio
		spin_lock_irqsave(&_gIntrHdmiM14B0AudioLock, flags);
		_gM14B0IntrAudioState = HDMI_AUDIO_INTERRUPT_STATE;
		_gM14B0HdmiFreqErrorCount = 0;
		spin_unlock_irqrestore(&_gIntrHdmiM14B0AudioLock, flags);

		if (_gM14B0HDMIState >= HDMI_STATE_READ)
		{
			HDMI_TASK("[%d] %s : -Changed -  _gM14B0Intr_vf_stable [STABLE_STATE] => [READ_STATE]     /   HDMI_STABLE[%d] / PSC[%d]\n", __L__, __F__, _gM14B0HDMIState, _gM14B0HdmiPortStableCount );
			spin_lock_irqsave(&_gIntrHdmiM14B0VideoLock, flags);
			_gM14B0Intr_vf_stable = HDMI_VIDEO_READ_STATE;
			//_gM14B0HDMIState = HDMI_STATE_READ;
			_gM14B0TimingReadCnt = 0;
			_gM14B0Intr_avi = HDMI_AVI_INTERRUPT_STATE;
			_gM14B0AVIReadState = FALSE;
			spin_unlock_irqrestore(&_gIntrHdmiM14B0VideoLock, flags);

			//intr_vf_stable is abnormally toggled from UP player source in UD Model for abnormal chip only.(2013-06-25)
			//Check a HDMI Port Stable Count
			if ( (CHK_UHD_BD() && _gM14B0HdmiPortStableCount > HDMI_AUDIO_RECHECK_TIME_500MS)	\
			  ||(CHK_FHD_BD() && _gM14B0HdmiPortStableCount > HDMI_AUDIO_PORT_STABLE_TIME_5S) )
			{
				//spin lock for protection for audio
				spin_lock_irqsave(&_gIntrHdmiM14B0AudioLock, flags);
				_gM14B0IntrAudioState = HDMI_AUDIO_STABLE_STATE;
				spin_unlock_irqrestore(&_gIntrHdmiM14B0AudioLock, flags);
			}
			else
			{
				//Mute audio data
				muteCtrl.eMode		= LX_HDMI_AUDIO_MUTE;
				muteCtrl.bAudioMute = TRUE;
				_HDMI_M14B0_SetInternalMute(muteCtrl);
			}
		}
		else
		{
			//Mute audio data
			muteCtrl.eMode		= LX_HDMI_AUDIO_MUTE;
			muteCtrl.bAudioMute = TRUE;
			_HDMI_M14B0_SetInternalMute(muteCtrl);
		}
	}

	if (LINK_M14_Rd_fld(interrupt_01, intr_chg_avi_b12345))
	{
		HDMI_DEBUG("intr_chg_avi_b12345\n");
		spin_lock_irqsave(&_gIntrHdmiM14B0VideoLock, flags);
		_gM14B0Intr_avi = HDMI_AVI_INTERRUPT_STATE;
		_gM14B0AVIReadState = FALSE;
		if (_gM14B0CHG_AVI_count_MHL != -1)
			_gM14B0CHG_AVI_count_MHL ++;
		if (_gM14B0CHG_AVI_count_EQ != -1)
			_gM14B0CHG_AVI_count_EQ ++;
		spin_unlock_irqrestore(&_gIntrHdmiM14B0VideoLock, flags);

		if (LINK_M14_Rd_fld(interrupt_01, intr_chg_avi_to_rgb))
		{
			HDMI_INTR("intr_chg_avi_to_rgb\n");
			csc = LX_HDMI_AVI_COLORSPACE_RGB;
		}

		if (LINK_M14_Rd_fld(interrupt_01, intr_chg_avi_to_ycc444))
		{
			HDMI_INTR("intr_chg_avi_to_ycc444\n");
			csc = LX_HDMI_AVI_COLORSPACE_YCBCR444;
		}

		if (LINK_M14_Rd_fld(interrupt_01, intr_chg_avi_to_ycc422))
		{
			HDMI_INTR("intr_chg_avi_to_ycc422\n");
			csc = LX_HDMI_AVI_COLORSPACE_YCBCR422;
		}

		HDMI_TASK("[%d][HDMI_AVI] state [%d]  preCSS[%d] CSC = %d \n", __L__, _gM14B0HDMIState, _gM14B0PrevPixelEncoding, csc);

		if (_gM14B0HDMIState >= HDMI_STATE_READ)//(_gM14B0Intr_vf_stable == HDMI_VIDEO_STABLE_STATE) 			// for PS3 CSC
		{
			if (_gM14B0PrevPixelEncoding != csc)
			{
				//spin lock for protection
				spin_lock_irqsave(&_gIntrHdmiM14B0VideoLock, flags);
				_gM14B0Intr_avi = HDMI_AVI_CHANGE_STATE;
				_gM14B0TimingReadCnt = 0;
				_gM14B0AVIReadState = FALSE;
				_gM14B0AVIChangeState = TRUE;
				spin_unlock_irqrestore(&_gIntrHdmiM14B0VideoLock, flags);

				HDMI_TASK("[%d] %s : -Changed  - _gM14B0Intr_avi [STABLE_STATE] => [CHANGE_STATE]     /   HDMI_STABLE[%d] \n", __L__, __F__,_gM14B0HDMIState );
				muteCtrl.eMode		= LX_HDMI_VIDEO_MUTE;
				muteCtrl.bVideoMute = TRUE;
				_HDMI_M14B0_SetInternalMute(muteCtrl);
			}
		}
	}

	if (LINK_M14_Rd_fld(interrupt_02, intr_set_mute))
	{
		HDMI_INTR("intr_set_mute\n");

		//if (_gM14B0Intr_vf_stable > HDMI_VIDEO_INIT_STATE)
		{
			if (_gM14B0Intr_src_mute != HDMI_SOURCE_MUTE_STATE)
			{
				muteCtrl.eMode		= LX_HDMI_VIDEO_MUTE;
				muteCtrl.bVideoMute = TRUE;
				_HDMI_M14B0_SetInternalMute(muteCtrl);

				spin_lock_irqsave(&_gIntrHdmiM14B0VideoLock, flags);
				_gM14B0Intr_src_mute = HDMI_SOURCE_MUTE_STATE;
				_gM14B0TimingReadCnt = 0;
				spin_unlock_irqrestore(&_gIntrHdmiM14B0VideoLock, flags);

				HDMI_DEBUG("[%d] intr_set_mute : HDMI_SOURCE MUTE Enable[%d] \n", __L__, _gM14B0Intr_src_mute );
			}
		}
	}

	if (LINK_M14_Rd_fld(interrupt_01, intr_no_vsi))
	{
		HDMI_TASK("intr_no_vsi\n");
		HDMI_TASK("[%d] %s : -Changed  - _gM14B0Intr_vsi [STABLE_STATE] => [NO_DATA_STAT]     /   HDMI_STABLE[%d] \n", __L__, __F__,_gM14B0HDMIState );
		HDMI_TASK("[%d] %s : -Changed  - _gM14B0Intr_vf_stable [STABLE_STATE] => [READ_STATE]     /   HDMI_STABLE[%d] \n", __L__, __F__,_gM14B0HDMIState );
		spin_lock_irqsave(&_gIntrHdmiM14B0VideoLock, flags);
		_gM14B0Intr_vsi = HDMI_VSI_NO_DATA_STATE;
		_gM14B0Intr_vf_stable = HDMI_VIDEO_READ_STATE;
		_gM14B0TimingReadCnt = 0;
		_gM14B0Intr_avi = HDMI_AVI_INTERRUPT_STATE;
		_gM14B0AVIReadState = FALSE;
		spin_unlock_irqrestore(&_gIntrHdmiM14B0VideoLock, flags);

		LINK_M14_RdFL(interrupt_enable_01);
		LINK_M14_Wr01(interrupt_enable_01, intr_no_vsi_enable, 0x0);			///< 20 intr_no_vsi_enable
		LINK_M14_WrFL(interrupt_enable_01);

		HDMI_DEBUG("[%d] %s : No VSI intra disable  \n", __LINE__, __func__);
	}

	if (LINK_M14_Rd_fld(interrupt_00, intr_chg_vsi_vformat))
	{
		HDMI_DEBUG("intr_chg_vsi_vformat\n");
		spin_lock_irqsave(&_gIntrHdmiM14B0VideoLock, flags);
		_gM14B0Intr_vsi = HDMI_VSI_INTERRUPT_STATE;
		_gM14B0VSIState = TRUE;

		LINK_M14_RdFL(interrupt_enable_01);
		LINK_M14_Wr01(interrupt_enable_01, intr_no_vsi_enable, 0x1);			///< 20 intr_no_vsi_enable
		LINK_M14_WrFL(interrupt_enable_01);
		_gM14B0VSIState = FALSE;
		HDMI_DEBUG("[%d] %s : No VSI intra enable  \n", __LINE__, __func__);

		spin_unlock_irqrestore(&_gIntrHdmiM14B0VideoLock, flags);

		// when  don't change Sync and change VSI on 3D for OPTIMUS Phone ( 3D / LTE /MHL )
		if (_gM14B0HDMIState > HDMI_STATE_READ)
		{
			HDMI_TASK("[%d] %s : -Changed  - _gM14B0Intr_vf_stable [STABLE_STATE] => [READ_STATE]     /   HDMI_STABLE[%d] \n", __L__, __F__,_gM14B0HDMIState );
			spin_lock_irqsave(&_gIntrHdmiM14B0VideoLock, flags);
			_gM14B0Intr_vf_stable = HDMI_VIDEO_READ_STATE;
			_gM14B0TimingReadCnt = 0;
			_gM14B0Intr_avi = HDMI_AVI_INTERRUPT_STATE;
			_gM14B0AVIReadState = FALSE;
			spin_unlock_irqrestore(&_gIntrHdmiM14B0VideoLock, flags);
		}
	}

//audio related. => start
	if (LINK_M14_Rd_fld(interrupt_00, intr_new_acr))
	{
		HDMI_INTR("intr_new_acr\n");

#ifndef M14_DISABLE_AUDIO_INTERRUPT
		/* Disable a intr_new_acr_enable and Enable a intr_scdt_fall_enable interrupt. */
		LINK_M14_RdFL(interrupt_enable_01);
		LINK_M14_Wr01(interrupt_enable_01, intr_scdt_fall_enable, 0x1);		///< 3 intr_scdt_fall_enable
		LINK_M14_Wr01(interrupt_enable_01, intr_new_acr_enable,   0x0);		///< 26 intr_new_acr_enable
		LINK_M14_WrFL(interrupt_enable_01);
#endif

		//To protect EMI issue, ACR is disabled in DVI mode because of avoiding abnormal clock generation.
		LINK_M14_RdFL(audio_00);
		LINK_M14_Wr01(audio_00, reg_acr_en, 0x1);	//ACR Enable(Audio Clock Generation Function Activation)
		LINK_M14_WrFL(audio_00);

		HDMI_INTR("[%d]IRQHandler : ACR ON.\n", __L__);
	}

	if (LINK_M14_Rd_fld(interrupt_02, intr_scdt_fall_prt0))
	{
		HDMI_INTR("intr_scdt_fall\n");

#ifndef M14_DISABLE_AUDIO_INTERRUPT
		/* Disable a intr_scdt_fall_enable and Enable a intr_new_acr_enable interrupt. */
		LINK_M14_RdFL(interrupt_enable_01);
		LINK_M14_Wr01(interrupt_enable_01, intr_scdt_fall_enable, 0x0);		///< 3 intr_scdt_fall_enable
		LINK_M14_Wr01(interrupt_enable_01, intr_new_acr_enable,   0x1);		///< 26 intr_new_acr_enable
		LINK_M14_WrFL(interrupt_enable_01);
#endif

		//To protect EMI issue, ACR is disabled in DVI mode because of avoiding abnormal clock generation.
		LINK_M14_RdFL(audio_00);
		LINK_M14_Wr01(audio_00, reg_acr_en, 0x0);	//ACR Disable(Audio Clock Generation Function Activation)
		LINK_M14_WrFL(audio_00);

		HDMI_INTR("[%d]IRQHandler : ACR OFF.\n", __L__);
	}

	//intr_fs_chg is normal toggled in FHD Model.
	if (CHK_FHD_BD() && (intra0 & HDMI_AUDIO_INTERRUPT_BIT_MASK))
	{
		//spin lock for protection for audio
		spin_lock_irqsave(&_gIntrHdmiM14B0AudioLock, flags);
		_gM14B0IntrAudioState = HDMI_AUDIO_INTERRUPT_STATE;
		_gM14B0HdmiFreqErrorCount = 0;
		spin_unlock_irqrestore(&_gIntrHdmiM14B0AudioLock, flags);

		//Mute audio data
		muteCtrl.eMode		= LX_HDMI_AUDIO_MUTE;
		muteCtrl.bAudioMute = TRUE;
		_HDMI_M14B0_SetInternalMute(muteCtrl);

		if (LINK_M14_Rd_fld(interrupt_00, intr_fs_chg))
			HDMI_INTR("intr_fs_chg\n");

		if (LINK_M14_Rd_fld(interrupt_00, intr_2pcm_chg))
			HDMI_INTR("intr_2pcm_chg\n");

		if (LINK_M14_Rd_fld(interrupt_00, intr_2npcm_chg))
			HDMI_INTR("intr_2npcm_chg\n");
	}

	//intr_fs_chg is abnormally toggled from UD player source in UD Model.
	if (CHK_UHD_BD() && (intra0 & HDMI_AUDIO_INTERRUPT_BIT_MASK))
	{
		if (LINK_M14_Rd_fld(interrupt_00, intr_fs_chg))
			HDMI_INTR("intr_fs_chg\n");

		if (LINK_M14_Rd_fld(interrupt_00, intr_2pcm_chg))
			HDMI_INTR("intr_2pcm_chg\n");

		if (LINK_M14_Rd_fld(interrupt_00, intr_2npcm_chg))
			HDMI_INTR("intr_2npcm_chg\n");
	}

	if (LINK_M14_Rd_fld(interrupt_00, intr_burst_info))
	{
		//Get a elapsed mili-second time.
		elapsedTime = jiffies_to_msecs(jiffies - _gM14B0BurstInfoPrevJiffies);

		//Save _gM14B0BurstInfoPrevJiffies
		_gM14B0BurstInfoPrevJiffies = jiffies;

		if (_gM14B0IntrBurstInfoCount == 0 && elapsedTime > HDMI_AUDIO_BURST_INFO_RECHECK_80MS)
		{
			//spin lock for protection for audio
			spin_lock_irqsave(&_gIntrHdmiM14B0AudioLock, flags);
			_gM14B0IntrAudioState = HDMI_AUDIO_INTERRUPT_STATE;
			_gM14B0HdmiFreqErrorCount = 0;
			spin_unlock_irqrestore(&_gIntrHdmiM14B0AudioLock, flags);

			//Mute audio data
			muteCtrl.eMode		= LX_HDMI_AUDIO_MUTE;
			muteCtrl.bAudioMute = TRUE;
			_HDMI_M14B0_SetInternalMute(muteCtrl);

			if (_gM14B0HdmiPortStableCount > HDMI_AUDIO_PORT_STABLE_TIME_5S)
			{
				//Check a vaild Pc data for payload
				LINK_M14_RdFL(audio_09);
				HDMI_DEBUG("intr_burst_info(%llu), Pd_Pc = 0x%X, elapsedTime = %llu\n",	\
							_gM14B0IntrBurstInfoCount, LINK_M14_Rd(audio_09), elapsedTime);
			}
		}

		//Increase _gM14B0IntrBurstInfoCount
		_gM14B0IntrBurstInfoCount++;

		HDMI_INTR("intr_burst_info(%llu)\n", _gM14B0IntrBurstInfoCount);

		//if ((_gM14B0HdmiAudioThreadCount % DEBUG_HDMI_AUDIO_MSG_PRINT_TIME_100S) == 0)
			//HDMI_DEBUG("intr_burst_info(%llu), elapsedTime(%llu)\n", _gM14B0IntrBurstInfoCount, elapsedTime);
	}
	else
	{
		//_gM14B0IntrBurstInfoCount is normal toggled in FHD Model.
		if (CHK_FHD_BD())
		{
			//Reset _gM14B0IntrBurstInfoCount
			_gM14B0IntrBurstInfoCount = 0;
		}
		else
		{
			//Check a vaild Pc data for payload
			LINK_M14_RdFL(audio_09);
			HDMI_INTR("_gM14B0IntrBurstInfoCount(%llu), Pd_Pc = 0x%X\n", _gM14B0IntrBurstInfoCount, LINK_M14_Rd(audio_09));
		}
	}
//audio related. => end.

	if ( LINK_M14_Rd_fld(interrupt_01, intr_hdmi5v_redge_prt3) || LINK_M14_Rd_fld(interrupt_01, intr_hdmi5v_redge_prt2) \
		|| LINK_M14_Rd_fld(interrupt_01, intr_hdmi5v_redge_prt1) || LINK_M14_Rd_fld(interrupt_01, intr_hdmi5v_redge_prt0) )
	{
		spin_lock_irqsave(&_gIntrHdmiM14B0VideoLock, flags);

		_gHdmi_no_connection_count = 0;

		if ( _gM14B0HDMI_thread_running < 1)
		{
			wake_up_interruptible(&WaitQueue_HDMI_M14B0);
			_gM14B0HDMIState = HDMI_STATE_IDLE;
			_gM14B0Force_thread_sleep = 0;
			_gM14B0HDMI_thread_running = 1;
		}
		spin_unlock_irqrestore(&_gIntrHdmiM14B0VideoLock, flags);
		HDMI_DEBUG("HDMI 5V Detected !!!\n");
	}

	if (LINK_M14_Rd_fld(interrupt_03, intr_cbus_new_rcp))
	{
		_gMHL_RCP_RCV_MSG.rcp_receive_flag = TRUE;
		LINK_M14_RdFL(cbus_11);
		LINK_M14_Rd01(cbus_11, reg_rx_rcpk_rtn_kcode, _gMHL_RCP_RCV_MSG.rcp_msg);
		HDMI_INTR("intr_cbus_new_rcp(0x%x)\n", _gMHL_RCP_RCV_MSG.rcp_msg);
	}

	if (LINK_M14_Rd_fld(interrupt_03, intr_cbus_cmd_rcp))
	{
		spin_lock_irqsave(&_gIntrHdmiM14B0VideoLock, flags);
		if(_g_rcp_send_buffer <2)
			_g_rcp_send_buffer ++;
		spin_unlock_irqrestore(&_gIntrHdmiM14B0VideoLock, flags);
		HDMI_INTR("intr_cbus_cmd_rcp:buffer[%d]\n", _g_rcp_send_buffer);
	}

#endif	//#if 0	//Not used interrupt is disabled.

	if (intra0)
	{
		LINK_M14_Wr(interrupt_00, intra0);
		LINK_M14_WrFL(interrupt_00);
	}

	if (intra1)
	{
		LINK_M14_Wr(interrupt_01, intra1);
		LINK_M14_WrFL(interrupt_01);
	}

	if (intra2)
	{
		LINK_M14_Wr(interrupt_02, intra2);
		LINK_M14_WrFL(interrupt_02);
	}

	if (intra3)
	{
		LINK_M14_Wr(interrupt_03, intra3);
		LINK_M14_WrFL(interrupt_03);
	}

	if (intra4)
	{
		LINK_M14_Wr(interrupt_04, intra4);
		LINK_M14_WrFL(interrupt_04);
	}

	return IRQ_HANDLED;
}

static void _HDMI_M14B0_Periodic_Task(void)
{
	static UINT32 gPeriodicState = HDMI_STATE_IDLE;
	static UINT32 gCheakCnt = 0;		// check count after vf stable interrupt
	static UINT32 gMuteCnt = 0;			// check count for  mute
	static UINT32 gAviCnt = 0;			// check count for checking AVI Data	in the HDMI mode
	static UINT32 gAviChgCnt = 0;		// check count after changing CSC in stable status
	static UINT32 gScdtCnt = 0;		// check count after connected
	static UINT32 hdcp_authed_info[4] = {0,};
	static UINT32 hdcp_enc_en_info[4] = {0,};
	static int hdcp_check_count = HDMI_CHEAK_COUNT;

	UINT32 muteClear = 0;
	UINT32 aviHeader = 0;
	UINT32 temp = 0;
	ULONG	flags = 0;
	LX_HDMI_MUTE_CTRL_T 	muteCtrl = {FALSE, FALSE, LX_HDMI_MUTE_NONE};

	do {
		if (_gM14B0HDMIState == HDMI_STATE_IDLE && _gM14B0HDMIState == HDMI_STATE_DISABLE)
			gPeriodicState = _gM14B0HDMIState;

		if (_gM14B0HDMIConnectState == HDMI_PORT_NOT_CONNECTED)			// change check connection in the thread function by 20121206
		{
			gCheakCnt = gMuteCnt = gAviCnt = gAviChgCnt = gScdtCnt = 0;
			gPeriodicState = HDMI_STATE_NO_SCDT;
		}
		else
		{
			if (_gM14B0HDMIState <= HDMI_STATE_NO_SCDT)
			{
				gPeriodicState = HDMI_STATE_SCDT;
				HDMI_TASK("[%d] %s : -Changed- HDMI_STATE_DISABLE  => HDMI_STATE_SCDT[%d] \n", __L__, __F__, gPeriodicState);
			}

			///* start Source Mute Control  *///
			if (_gM14B0Intr_src_mute == HDMI_SOURCE_MUTE_STATE)
			{
				LINK_M14_RdFL(packet_33);
				LINK_M14_Rd01(packet_33, reg_pkt_gcp_cmute, muteClear);
				LINK_M14_Rd01(packet_33, reg_pkt_gcp_smute, temp);

				gMuteCnt++;
				if (muteClear == 1  || gMuteCnt >= 150)
				{
					spin_lock_irqsave(&_gIntrHdmiM14B0VideoLock, flags);
					_gM14B0Intr_src_mute = HDMI_SOURCE_MUTE_CLEAR_STATE;
					spin_unlock_irqrestore(&_gIntrHdmiM14B0VideoLock, flags);

					gPeriodicState = HDMI_STATE_CHECK_MODE;
					_HDMI_Check_State_StartTime = jiffies_to_msecs(jiffies);

					muteCtrl.eMode		= LX_HDMI_VIDEO_MUTE;
					muteCtrl.bVideoMute = FALSE;
					_HDMI_M14B0_SetInternalMute(muteCtrl);

					HDMI_DEBUG("[%d] %s : -source mute clear  cnt [%d] state[%d]  \n", __L__, __F__, gMuteCnt, gPeriodicState);
					gCheakCnt =  gMuteCnt = 0;
				}
				else
				{
					if (gPeriodicState != HDMI_STATE_CHECK_SOURCE_MUTE)
					{
						HDMI_TASK("[%d] %s : -Changed- HDMI_STATE [%d]  => HDMI_STATE_CHECK_SOURCE_MUTE \n", __L__, __F__, gPeriodicState);
						gPeriodicState = HDMI_STATE_CHECK_SOURCE_MUTE;
						_HDMI_Check_State_StartTime = jiffies_to_msecs(jiffies);
					}
				}
			}
			///* end Source Mute Control  *///
			if (_gM14B0AVIChangeState == TRUE)
			{
				HDMI_TASK("[%d] %s : -Changed- HDMI_STATE_[%d]  => HDMI_STATE_CHECK_AVI_CHG \n", __L__, __F__, gPeriodicState);
				spin_lock_irqsave(&_gIntrHdmiM14B0VideoLock, flags);
				_gM14B0AVIChangeState = FALSE;
				//_gM14B0Intr_avi = HDMI_AVI_INTERRUPT_STATE;
				spin_unlock_irqrestore(&_gIntrHdmiM14B0VideoLock, flags);
				gPeriodicState = HDMI_STATE_CHECK_AVI_CHG;
				gAviChgCnt = 0;
			}

			//if (_gM14B0Intr_vf_stable == HDMI_VIDEO_READ_STATE)
			if ( (_gM14B0Intr_vf_stable == HDMI_VIDEO_READ_STATE) \
					&& ( ( _gM14B0HDMIPhyInform.hdcp_authed[_gM14B0HDMIPhyInform.prt_sel] \
							&& _HDMI_M14B0_Check_Aksv_Exist(_gM14B0HDMIPhyInform.prt_sel) \
							&& (hdcp_check_count == HDMI_CHEAK_COUNT) )
						|| (_HDMI_M14B0_Check_Aksv_Exist(_gM14B0HDMIPhyInform.prt_sel) == 0) )
			  )
			{
				HDMI_TASK("[%d] %s : -Changed- HDMI_STATE_[%d]  => HDMI_STATE_READ && _gM14B0Intr_vf_stable[READ_STATE => INTERRUPT_STATE] \n", __L__, __F__, gPeriodicState);
				gPeriodicState = HDMI_STATE_READ;
				spin_lock_irqsave(&_gIntrHdmiM14B0VideoLock, flags);
				_gM14B0Intr_vf_stable = HDMI_VIDEO_INTERRUPT_STATE;
				spin_unlock_irqrestore(&_gIntrHdmiM14B0VideoLock, flags);
			}

			/* M14 HDCP Authentication Information */
			if (gM14BootData->mode == 1)		// for HDMI Mode
			{
				if ( ( memcmp(_gM14B0HDMIPhyInform.hdcp_authed, hdcp_authed_info, sizeof(hdcp_authed_info)) ) \
						|| ( memcmp(_gM14B0HDMIPhyInform.hdcp_enc_en, hdcp_enc_en_info, sizeof(hdcp_authed_info)) ) )
				{
					HDMI_TASK("HDMI HDCP Information Changed !!!\n");

					for (temp = 0; temp < 4;temp++)
						HDMI_TASK("Port[%d] : HDCP_ENC_EN[%d]=>[%d], HDCP_AUTHED[%d]=>[%d]\n", temp, hdcp_enc_en_info[temp], _gM14B0HDMIPhyInform.hdcp_enc_en[temp], \
								hdcp_authed_info[temp], _gM14B0HDMIPhyInform.hdcp_authed[temp]);

					memcpy(hdcp_authed_info, _gM14B0HDMIPhyInform.hdcp_authed, sizeof(hdcp_authed_info));
					memcpy(hdcp_enc_en_info, _gM14B0HDMIPhyInform.hdcp_enc_en, sizeof(hdcp_authed_info));
				}

			}
		}

		switch(gPeriodicState)
		{
			case HDMI_STATE_NO_SCDT:
				break;

			case HDMI_STATE_SCDT:		// if (HDMI_SCDT_COUNT == 4)  5 call
				{
					hdcp_check_count = HDMI_CHEAK_COUNT;
									// x2 for M14B0 (no external switch)
					if (gScdtCnt < (HDMI_SCDT_COUNT<<1) )	gScdtCnt++;
					else
					{
						gScdtCnt++;
						if (_gM14B0Intr_vf_stable == HDMI_VIDEO_INTERRUPT_STATE)
						{
							gPeriodicState = HDMI_STATE_CHECK_MODE;
							_HDMI_Check_State_StartTime = jiffies_to_msecs(jiffies);
							HDMI_TASK("[%d] %s : -Changed- HDMI_STATE_SCDT  => HDMI_STATE_CHECK_MODE with HDMI_VIDEO_INTERRUPT_STATE \n", __L__, __F__);
						}

						if (gScdtCnt > 50)			// when no vf_stable Interrupt
						{
							gPeriodicState = HDMI_STATE_CHECK_MODE;
							_HDMI_Check_State_StartTime = jiffies_to_msecs(jiffies);
							spin_lock_irqsave(&_gIntrHdmiM14B0VideoLock, flags);
							_gM14B0Intr_vf_stable = HDMI_VIDEO_INTERRUPT_STATE;
							if (_gM14B0Intr_avi == HDMI_AVI_INIT_STATE)		_gM14B0Intr_avi = HDMI_AVI_INTERRUPT_STATE;
							if (_gM14B0Intr_vsi == HDMI_VSI_INIT_STATE)		_gM14B0Intr_vsi = HDMI_VSI_INTERRUPT_STATE;
							spin_unlock_irqrestore(&_gIntrHdmiM14B0VideoLock, flags);

							HDMI_TASK("[%d] %s : -Changed- HDMI_STATE_SCDT  => HDMI_STATE_CHECK_MODE with No vf_stable Interrupt \n", __L__, __F__);
						}
					}
					HDMI_TASK("[%d] %s : HDMI_STATE_SCDT scdtCnt = [%d] \n", __L__, __F__, gScdtCnt);
				}
				break;

			case HDMI_STATE_INTE_CHECK:
			case HDMI_STATE_CHECK_MODE:		// all interrupt cheak
			case HDMI_STATE_CHECK_SOURCE_MUTE:
			case HDMI_STATE_CHECK_AVI_NO:
				{
					///*  HDMI_STATE_CHECK_SOURCE_MUTE *///
					if (_gM14B0Intr_src_mute == HDMI_SOURCE_MUTE_STATE)
					{
						HDMI_TASK("[%d] %s : HDMI_STATE [HDMI_STATE_CHECK_SOURCE_MUTE(%d)] \n", __L__, __F__, gPeriodicState);
						break;
					}

					///*  HDMI_STATE_CHECK_NO_AVI   //If HDMI mode and AVI no packet, timing info is all '0'*///
					if (gM14BootData->mode == 1)
					{
						gAviCnt ++;

						if ( !_gM14B0HDMIPhyInform.hdcp_authed[_gM14B0HDMIPhyInform.prt_sel]	\
								&& (_HDMI_M14B0_Check_Aksv_Exist(_gM14B0HDMIPhyInform.prt_sel) != 0) )
						{
							hdcp_check_count = 100; //maximum 2sec
							HDMI_TASK("[%d] %s : HDCP Auth : HDMI_STATE [HDMI_STATE_CHECK_SOURCE_MUTE(%d)] \n", __L__, __F__, gPeriodicState);
						}
						else if ( (hdcp_check_count == 100) && _gM14B0HDMIPhyInform.hdcp_authed[_gM14B0HDMIPhyInform.prt_sel]	\
								&& (_HDMI_M14B0_Check_Aksv_Exist(_gM14B0HDMIPhyInform.prt_sel) != 0) )
						{
							hdcp_check_count = gCheakCnt + 10;	// 200msec added
							HDMI_TASK("HDMI_STATE [%d] : HDCP Authed set hdcp_check_cout to [%d] \n", gPeriodicState, hdcp_check_count);
						}
						/*
						else if ( _(hdcp_check_count == HDMI_CHEAK_COUNT) && gM14B0HDMIPhyInform.hdcp_authed[_gM14B0HDMIPhyInform.prt_sel]	\
								&& (_HDMI_M14B0_Check_Aksv_Exist(_gM14B0HDMIPhyInform.prt_sel) != 0) )
						{
							hdcp_check_count = HDMI_CHEAK_COUNT;	// No AKSV Received : HDCP not encoded?
							HDMI_TASK("HDMI_STATE [%d] : HDCP Already Authed \n", gPeriodicState);
						}
						*/
						else if ( _HDMI_M14B0_Check_Aksv_Exist(_gM14B0HDMIPhyInform.prt_sel) == 0)
						{
							hdcp_check_count = HDMI_CHEAK_COUNT;	// No AKSV Received : HDCP not encoded?
							HDMI_TASK("HDMI_STATE [%d] : HDCP no encoded ??? \n", gPeriodicState);
						}
																					// x2 for M14B0 (no external switch)
						if (_gM14B0Intr_avi <= HDMI_AVI_INTERRUPT_STATE &&  gAviCnt < (50<<1) )		// No read AVI
						{
							LINK_M14_RdFL(packet_18);
							aviHeader = LINK_M14_Rd(packet_18);		// reg_pkt_avi_hdr_0 (AVI Packet Version), reg_pkt_avi_hdr_1 (AVI Packet Length)
							aviHeader = (aviHeader &0xffff);

							if (aviHeader == 0)
							{
								gPeriodicState = HDMI_STATE_CHECK_AVI_NO;
								HDMI_TASK("*[%d] %s : No AVI Packet - before AVI Init state\n", __L__, __F__);
								break;
							}
						}
					}
					else
						hdcp_check_count = HDMI_CHEAK_COUNT;

					///*  HDMI_STATE_CHECK_MODE && HDMI_STATE_CHECK_AVI_CHG *///
					//if (gCheakCnt > HDMI_CHEAK_COUNT)
					_HDMI_Check_State_ElaspedTime = jiffies_to_msecs(jiffies);

					if ( ( gCheakCnt > hdcp_check_count ) || ( (_HDMI_Check_State_ElaspedTime - _HDMI_Check_State_StartTime) > 2500) )	//MAX 2.5 Sec
					{
						gPeriodicState = HDMI_STATE_READ;
						HDMI_TASK("[%d] %s : -Changed- HDMI_STATE_INTE_CHECK[%d] => HDMI_STATE_READ : [%d msec] )\n" \
								, __L__, __F__, gPeriodicState, (_HDMI_Check_State_ElaspedTime - _HDMI_Check_State_StartTime) );
					}
					gCheakCnt++;

					HDMI_TASK("[%d] %s : HDMI State = [%d]  cheakCnt = %d\n", __L__, __F__, gPeriodicState, gCheakCnt);
				}
				break;

			case HDMI_STATE_CHECK_AVI_CHG:
				{
					if (gAviChgCnt > 4)
					{
						muteCtrl.eMode	= LX_HDMI_VIDEO_MUTE;
						muteCtrl.bVideoMute = FALSE;			// mute Clear
						_HDMI_M14B0_SetInternalMute(muteCtrl);

						gPeriodicState = HDMI_STATE_CHECK_MODE;
						gAviChgCnt = gCheakCnt =  0;
						_HDMI_Check_State_StartTime = jiffies_to_msecs(jiffies);
						HDMI_TASK("[%d] %s : -Changed- HDMI_STATE_CHECK_AVI_CHG  => HDMI_STATE_CHECK_MODE[%d] \n", __L__, __F__, gPeriodicState);
					}
					gAviChgCnt++;
				}
				break;

			case HDMI_STATE_READ:
				{
					unsigned int elasped_time;

					if (_gM14B0TimingReadCnt < 2)
					{
						muteCtrl.eMode	= LX_HDMI_VIDEO_MUTE;
						muteCtrl.bVideoMute = FALSE;			// mute Clear
						_HDMI_M14B0_SetInternalMute(muteCtrl);
						gAviChgCnt = gCheakCnt =  0;
					}
					else if (_gM14B0TimingReadCnt > HDMI_READ_COUNT)
					{
						gPeriodicState = HDMI_STATE_STABLE;
						elasped_time = jiffies_to_msecs(jiffies) - _Port_Change_StartTime;
						HDMI_DEBUG("### Port Change to Stable Time [%d msec] ### \n", elasped_time);

#ifdef M14_HUMAX_SETTOP_HDCP_WORKAROUND
						/* HUMAX IR1020HD : snow noise when resolution change */
						if ( !((_gM14B0HDMIPhyInform.prt_sel == 3) && (_gM14B0HDMIPhyInform.cd_sense) ) )
							_HDMI_M14B0_Set_HDCP_Unauth(_gM14B0HDMIPhyInform.prt_sel, 0, 0);
#endif
					}

					HDMI_TASK("[%d] %s : HDMI State = [%d]  _gM14B0TimingReadCnt = %d \n", __L__, __F__, gPeriodicState, _gM14B0TimingReadCnt);
				}
				break;

			case HDMI_STATE_STABLE:
				{
					spin_lock_irqsave(&_gIntrHdmiM14B0VideoLock, flags);
					_gM14B0Intr_vf_stable = HDMI_VIDEO_STABLE_STATE;
					spin_unlock_irqrestore(&_gIntrHdmiM14B0VideoLock, flags);
				}
				break;

			case HDMI_STATE_IDLE:
			case HDMI_STATE_DISABLE:
			case HDMI_STATE_MAX:
			default :
				break;
		}
		//if (ret) break;

	} while(0);

	_gM14B0HDMIState = gPeriodicState;

	if (_gM14B0HDMIState < HDMI_STATE_STABLE)
		HDMI_PRINT("[%d]  %s :  State  = %d  \n\n\n", __L__, __F__, _gM14B0HDMIState);
}


int HDMI_M14B0_GetRegister(UINT32 addr , UINT32 *value)
{
	unsigned long ulAddr;
	UINT8 slave =0;
	UINT8 reg =0;
	UINT8 data =0;
	UINT32 prt_selected, phy_en_prt0, phy_en_prt1, phy_en_prt2, phy_en_prt3;
	int phy_tmp_enabled = -1;

	if (addr <= 0x824)
	{
		if (pstLinkReg == NULL)	return RET_ERROR;

		ulAddr = (unsigned long) pstLinkReg;
		*value = *((volatile UINT32*)(ulAddr + addr));
		*value = *((volatile UINT32*)(ulAddr + addr));
		//HDMI_DEBUG("[HDMI]  %s : %d  Addr  = 0x%x , Value = 0x%x  \n", __F__, __L__, addr, *value);
	}
	else if (addr >= 0x3800 && addr <=0x38FA)
	{
		LINK_M14_RdFL(system_control_00);
		LINK_M14_Rd01(system_control_00, reg_prt_sel, prt_selected);

		LINK_M14_RdFL(phy_link_00);
		LINK_M14_Rd01(phy_link_00, phy_enable_prt0, phy_en_prt0);		//PHY Enable

		LINK_M14_RdFL(phy_link_02);
		LINK_M14_Rd01(phy_link_02, phy_enable_prt1, phy_en_prt1);		//PHY Enable

		LINK_M14_RdFL(phy_link_04);
		LINK_M14_Rd01(phy_link_04, phy_enable_prt2, phy_en_prt2);		//PHY Enable

		LINK_M14_RdFL(phy_link_06);
		LINK_M14_Rd01(phy_link_06, phy_enable_prt3, phy_en_prt3);		//PHY Enable

		if (!(phy_en_prt0 ||phy_en_prt1 ||phy_en_prt2 ||phy_en_prt3))
		{
			HDMI_TRACE("[HDMI] %s : All ports are Not Enabled \n", __F__);
			HDMI_TRACE("[HDMI] Enable selected port [%d] \n", prt_selected);

			phy_tmp_enabled = prt_selected;

			if (prt_selected == 0)
			{
				LINK_M14_RdFL(phy_link_00);
				LINK_M14_Wr01(phy_link_00, phy_enable_prt0, 0x1);		//PHY Enable
				LINK_M14_WrFL(phy_link_00);
			}
			else if (prt_selected == 1)
			{
				LINK_M14_RdFL(phy_link_02);
				LINK_M14_Wr01(phy_link_02, phy_enable_prt1, 0x1);		//PHY Enable
				LINK_M14_WrFL(phy_link_02);
			}
			else if (prt_selected == 2)
			{
				LINK_M14_RdFL(phy_link_04);
				LINK_M14_Wr01(phy_link_04, phy_enable_prt2, 0x1);		//PHY Enable
				LINK_M14_WrFL(phy_link_04);
			}
			else
			{
				LINK_M14_RdFL(phy_link_06);
				LINK_M14_Wr01(phy_link_06, phy_enable_prt3, 0x1);		//PHY Enable
				LINK_M14_WrFL(phy_link_06);
			}

			OS_MsecSleep(5);	// ms delay
		}


		slave = ((addr &0xff00)>>8)<<1;
		reg = (addr &0xff);

		REG_ReadI2C(PHY_REG_M14_I2C_IDX, slave, reg, &data);
		*value = (UINT32)data;
		//HDMI_DEBUG("[HDMI]  %s : %d  slave = 0x%x reg = 0x%x, Value = 0x%x \n", __F__, __L__, slave, reg,data);

		if (phy_tmp_enabled != -1)
		{
			HDMI_TRACE("[HDMI] Disable selected port [%d] \n", prt_selected);

			if (prt_selected == 0)
			{
				LINK_M14_RdFL(phy_link_00);
				LINK_M14_Wr01(phy_link_00, phy_enable_prt0, 0x0);		//PHY Enable
				LINK_M14_WrFL(phy_link_00);
			}
			else if (prt_selected == 1)
			{
				LINK_M14_RdFL(phy_link_02);
				LINK_M14_Wr01(phy_link_02, phy_enable_prt1, 0x0);		//PHY Enable
				LINK_M14_WrFL(phy_link_02);
			}
			else if (prt_selected == 2)
			{
				LINK_M14_RdFL(phy_link_04);
				LINK_M14_Wr01(phy_link_04, phy_enable_prt2, 0x0);		//PHY Enable
				LINK_M14_WrFL(phy_link_04);
			}
			else
			{
				LINK_M14_RdFL(phy_link_06);
				LINK_M14_Wr01(phy_link_06, phy_enable_prt3, 0x0);		//PHY Enable
				LINK_M14_WrFL(phy_link_06);
			}
		}
	}
	else
	{
		HDMI_WARN("[HDMI]  %s : %d  Addr  = 0x%x  INVALID \n", __F__, __L__, addr);
		return RET_ERROR;
	}

	return RET_OK;
}

int HDMI_M14B0_SetRegister(UINT32 addr , UINT32 value)
{
	unsigned long ulAddr;
	UINT8 slave =0;
	UINT8 reg =0;
	UINT8 data =0;
	UINT32 prt_selected, phy_en_prt0, phy_en_prt1, phy_en_prt2, phy_en_prt3;
	int phy_tmp_enabled = -1;

	if (addr <= 0x824)
	{
		if (pstLinkReg == NULL)	return RET_ERROR;

		ulAddr = (unsigned long) pstLinkReg;
		*((volatile UINT32*)(ulAddr + addr)) = value ;
		//HDMI_DEBUG("[HDMI]  %s : %d  Addr  = 0x%x , Value = 0x%x  \n", __F__, __L__, addr, value);
	}
	else if (addr >= 0x3800 && addr <=0x38FA)
	{
		LINK_M14_RdFL(system_control_00);
		LINK_M14_Rd01(system_control_00, reg_prt_sel, prt_selected);

		LINK_M14_RdFL(phy_link_00);
		LINK_M14_Rd01(phy_link_00, phy_enable_prt0, phy_en_prt0);		//PHY Enable

		LINK_M14_RdFL(phy_link_02);
		LINK_M14_Rd01(phy_link_02, phy_enable_prt1, phy_en_prt1);		//PHY Enable

		LINK_M14_RdFL(phy_link_04);
		LINK_M14_Rd01(phy_link_04, phy_enable_prt2, phy_en_prt2);		//PHY Enable

		LINK_M14_RdFL(phy_link_06);
		LINK_M14_Rd01(phy_link_06, phy_enable_prt3, phy_en_prt3);		//PHY Enable

		if (!(phy_en_prt0 ||phy_en_prt1 ||phy_en_prt2 ||phy_en_prt3))
		{
			HDMI_TRACE("[HDMI] %s : All ports are Not Enabled \n", __F__);
			HDMI_TRACE("[HDMI] Enable selected port [%d] \n", prt_selected);

			phy_tmp_enabled = prt_selected;

			if (prt_selected == 0)
			{
				LINK_M14_RdFL(phy_link_00);
				LINK_M14_Wr01(phy_link_00, phy_enable_prt0, 0x1);		//PHY Enable
				LINK_M14_WrFL(phy_link_00);
			}
			else if (prt_selected == 1)
			{
				LINK_M14_RdFL(phy_link_02);
				LINK_M14_Wr01(phy_link_02, phy_enable_prt1, 0x1);		//PHY Enable
				LINK_M14_WrFL(phy_link_02);
			}
			else if (prt_selected == 2)
			{
				LINK_M14_RdFL(phy_link_04);
				LINK_M14_Wr01(phy_link_04, phy_enable_prt2, 0x1);		//PHY Enable
				LINK_M14_WrFL(phy_link_04);
			}
			else
			{
				LINK_M14_RdFL(phy_link_06);
				LINK_M14_Wr01(phy_link_06, phy_enable_prt3, 0x1);		//PHY Enable
				LINK_M14_WrFL(phy_link_06);
			}

			OS_MsecSleep(5);	// ms delay
		}
		slave = ((addr &0xff00)>>8)<<1;
		reg = (addr &0xff);
		data = (UINT8)(value &0xff);

		REG_WriteI2C(PHY_REG_M14_I2C_IDX, slave, reg, data);
		//HDMI_DEBUG("[HDMI]  %s : %d  slave = 0x%x reg = 0x%x, data = 0x%x \n", __F__, __L__, slave, reg, data);

		if (phy_tmp_enabled != -1)
		{
			HDMI_TRACE("[HDMI] Disable selected port [%d] \n", prt_selected);

			if (prt_selected == 0)
			{
				LINK_M14_RdFL(phy_link_00);
				LINK_M14_Wr01(phy_link_00, phy_enable_prt0, 0x0);		//PHY Enable
				LINK_M14_WrFL(phy_link_00);
			}
			else if (prt_selected == 1)
			{
				LINK_M14_RdFL(phy_link_02);
				LINK_M14_Wr01(phy_link_02, phy_enable_prt1, 0x0);		//PHY Enable
				LINK_M14_WrFL(phy_link_02);
			}
			else if (prt_selected == 2)
			{
				LINK_M14_RdFL(phy_link_04);
				LINK_M14_Wr01(phy_link_04, phy_enable_prt2, 0x0);		//PHY Enable
				LINK_M14_WrFL(phy_link_04);
			}
			else
			{
				LINK_M14_RdFL(phy_link_06);
				LINK_M14_Wr01(phy_link_06, phy_enable_prt3, 0x0);		//PHY Enable
				LINK_M14_WrFL(phy_link_06);
			}
		}
	}
	else
	{
		HDMI_WARN("[HDMI]  %s : %d  Addr  = 0x%x  INVALID \n", __F__, __L__, addr);
		return RET_ERROR;
	}

	return RET_OK;
}

/**
 * @brief PhyOn function when hdmi5v detected (power on with out run-reset)
 *
 * @param port_num
 *
 * @return
 */
static int _HDMI_M14B0_PhyOn_5V(int port_num)
{
	int ret = RET_OK;

	HDMI_DEBUG("[%s] Entered with [%d] \n",__func__, port_num);

	switch(port_num)
	{
		case 0:

			LINK_M14_RdFL(phy_link_00);
			LINK_M14_Wr01(phy_link_00, phy_pdb_prt0, 0x1);			//PHY PDB ON
		//	LINK_M14_Wr01(phy_link_00, phy_enable_prt0, 0x1);		//PHY Enable
			LINK_M14_Wr01(phy_link_00, phy_eq_odt_sel_prt0, 0x2);
			LINK_M14_WrFL(phy_link_00);

			OS_UsecDelay(100);
			LINK_M14_RdFL(phy_link_00);
			LINK_M14_Wr01(phy_link_00, phy_rstn_prt0, 0x1);			//PHY RESET
			LINK_M14_WrFL(phy_link_00);

			_HDMI_M14B0_PhyRunReset();	// init Phy register

			LINK_M14_RdFL(phy_link_00);
		//	LINK_M14_Wr01(phy_link_00, phy_enable_prt0, 0x0);		//PHY Enable
			LINK_M14_WrFL(phy_link_00);
			break;

		case 1:
			LINK_M14_RdFL(phy_link_02);
			LINK_M14_Wr01(phy_link_02, phy_pdb_prt1, 0x1);			//PHY PDB ON
		//	LINK_M14_Wr01(phy_link_02, phy_enable_prt1, 0x1);		//PHY Enable
			LINK_M14_Wr01(phy_link_02, phy_eq_odt_sel_prt1, 0x2);
			LINK_M14_WrFL(phy_link_02);
			OS_UsecDelay(100);
			LINK_M14_RdFL(phy_link_02);
			LINK_M14_Wr01(phy_link_02, phy_rstn_prt1, 0x1);			//PHY RESET
			LINK_M14_WrFL(phy_link_02);

			_HDMI_M14B0_PhyRunReset();	// init Phy register
			LINK_M14_RdFL(phy_link_02);
		//	LINK_M14_Wr01(phy_link_02, phy_enable_prt1, 0x0);		//PHY Enable
			LINK_M14_WrFL(phy_link_02);
			break;

		case 2:
			LINK_M14_RdFL(phy_link_04);
			LINK_M14_Wr01(phy_link_04, phy_pdb_prt2, 0x1);			//PHY PDB ON
		//	LINK_M14_Wr01(phy_link_04, phy_enable_prt2, 0x1);		//PHY Enable
			LINK_M14_Wr01(phy_link_04, phy_eq_odt_sel_prt2, 0x2);
			LINK_M14_WrFL(phy_link_04);
			OS_UsecDelay(100);
			LINK_M14_RdFL(phy_link_04);
			LINK_M14_Wr01(phy_link_04, phy_rstn_prt2, 0x1);			//PHY RESET
			LINK_M14_WrFL(phy_link_04);

			_HDMI_M14B0_PhyRunReset();	// init Phy register
			LINK_M14_RdFL(phy_link_04);
		//	LINK_M14_Wr01(phy_link_04, phy_enable_prt2, 0x0);		//PHY Enable
			LINK_M14_WrFL(phy_link_04);
			break;

		case 3:
			LINK_M14_RdFL(phy_link_06);
			LINK_M14_Wr01(phy_link_06, phy_pdb_prt3, 0x1);			//PHY PDB ON
		//	LINK_M14_Wr01(phy_link_06, phy_enable_prt3, 0x1);		//PHY Enable
			LINK_M14_Wr01(phy_link_06, phy_eq_odt_sel_prt3, 0x2);
			LINK_M14_WrFL(phy_link_06);
			OS_UsecDelay(100);
			LINK_M14_RdFL(phy_link_06);
			LINK_M14_Wr01(phy_link_06, phy_rstn_prt3, 0x1);			//PHY RESET
			LINK_M14_WrFL(phy_link_06);

			_HDMI_M14B0_PhyRunReset();	// init Phy register
			LINK_M14_RdFL(phy_link_06);
		//	LINK_M14_Wr01(phy_link_06, phy_enable_prt3, 0x0);		//PHY Enable
			LINK_M14_WrFL(phy_link_06);
			break;

		default :
			ret = RET_ERROR;
			break;


	}

	return ret;
}

static int _HDMI_M14B0_PhyOn(int port_num)
{
	int ret = RET_OK;

	HDMI_DEBUG("[%s] Entered with [%d] \n",__func__, port_num);

	switch(port_num)
	{
		case 0:

			LINK_M14_RdFL(phy_link_00);
			LINK_M14_Wr01(phy_link_00, phy_pdb_prt0, 0x1);			//PHY PDB ON
			LINK_M14_Wr01(phy_link_00, phy_enable_prt0, 0x1);		//PHY Enable
			LINK_M14_Wr01(phy_link_00, phy_eq_odt_sel_prt0, 0x2);
			LINK_M14_WrFL(phy_link_00);

			OS_UsecDelay(100);
			LINK_M14_RdFL(phy_link_00);
			LINK_M14_Wr01(phy_link_00, phy_rstn_prt0, 0x1);			//PHY RESET
			LINK_M14_WrFL(phy_link_00);

			_HDMI_M14B0_PhyRunReset();	// init Phy register
			break;

		case 1:
			LINK_M14_RdFL(phy_link_02);
			LINK_M14_Wr01(phy_link_02, phy_pdb_prt1, 0x1);			//PHY PDB ON
			LINK_M14_Wr01(phy_link_02, phy_enable_prt1, 0x1);		//PHY Enable
			LINK_M14_Wr01(phy_link_02, phy_eq_odt_sel_prt1, 0x2);
			LINK_M14_WrFL(phy_link_02);
			OS_UsecDelay(100);
			LINK_M14_RdFL(phy_link_02);
			LINK_M14_Wr01(phy_link_02, phy_rstn_prt1, 0x1);			//PHY RESET
			LINK_M14_WrFL(phy_link_02);

			_HDMI_M14B0_PhyRunReset();	// init Phy register
			break;

		case 2:
			LINK_M14_RdFL(phy_link_04);
			LINK_M14_Wr01(phy_link_04, phy_pdb_prt2, 0x1);			//PHY PDB ON
			LINK_M14_Wr01(phy_link_04, phy_enable_prt2, 0x1);		//PHY Enable
			LINK_M14_Wr01(phy_link_04, phy_eq_odt_sel_prt2, 0x2);
			LINK_M14_WrFL(phy_link_04);
			OS_UsecDelay(100);
			LINK_M14_RdFL(phy_link_04);
			LINK_M14_Wr01(phy_link_04, phy_rstn_prt2, 0x1);			//PHY RESET
			LINK_M14_WrFL(phy_link_04);

			_HDMI_M14B0_PhyRunReset();	// init Phy register
			break;

		case 3:
			LINK_M14_RdFL(phy_link_06);
			LINK_M14_Wr01(phy_link_06, phy_pdb_prt3, 0x1);			//PHY PDB ON
			LINK_M14_Wr01(phy_link_06, phy_enable_prt3, 0x1);		//PHY Enable
			LINK_M14_Wr01(phy_link_06, phy_eq_odt_sel_prt3, 0x2);
			LINK_M14_WrFL(phy_link_06);
			OS_UsecDelay(100);
			LINK_M14_RdFL(phy_link_06);
			LINK_M14_Wr01(phy_link_06, phy_rstn_prt3, 0x1);			//PHY RESET
			LINK_M14_WrFL(phy_link_06);

			_HDMI_M14B0_PhyRunReset();	// init Phy register
			break;

		default :
			ret = RET_ERROR;
			break;


	}

	return ret;
}

static int _HDMI_M14B0_PhyOff(int port_num)
{
	int ret = RET_OK;
#if 1
	int cd_sense;
#endif

	switch(port_num)
	{
		case 0:
			//ARC source
			LINK_M14_RdFL(edid_heac_00);
			LINK_M14_Wr01(edid_heac_00, reg_arc_src, 0x0);
			LINK_M14_WrFL(edid_heac_00);

			LINK_M14_RdFL(phy_link_00);
			LINK_M14_Wr01(phy_link_00, phy_pdb_prt0, 0x0);			//PHY PDB ON
			LINK_M14_Wr01(phy_link_00, phy_enable_prt0, 0x0);		//PHY Enable
			LINK_M14_Wr01(phy_link_00, phy_eq_odt_sel_prt0, 0x0);
			LINK_M14_Wr01(phy_link_00, phy_rstn_prt0, 0x0);			//PHY RESET
			LINK_M14_WrFL(phy_link_00);
			break;

		case 1:
			LINK_M14_RdFL(phy_link_02);
			LINK_M14_Wr01(phy_link_02, phy_pdb_prt1, 0x0);			//PHY PDB ON
			LINK_M14_Wr01(phy_link_02, phy_enable_prt1, 0x0);		//PHY Enable
			LINK_M14_Wr01(phy_link_02, phy_eq_odt_sel_prt1, 0x0);
			LINK_M14_Wr01(phy_link_02, phy_rstn_prt1, 0x0);			//PHY RESET
			LINK_M14_WrFL(phy_link_02);
			break;

		case 2:
			LINK_M14_RdFL(phy_link_04);
			LINK_M14_Wr01(phy_link_04, phy_pdb_prt2, 0x0);			//PHY PDB ON
			LINK_M14_Wr01(phy_link_04, phy_enable_prt2, 0x0);		//PHY Enable
			LINK_M14_Wr01(phy_link_04, phy_eq_odt_sel_prt2, 0x0);
			LINK_M14_Wr01(phy_link_04, phy_rstn_prt2, 0x0);			//PHY RESET
			LINK_M14_WrFL(phy_link_04);
			break;

		case 3:
			LINK_M14_RdFL(phy_link_06);
#if 1
			LINK_M14_RdFL(system_control_00);
			LINK_M14_Rd01(system_control_00, reg_cd_sense_prt3, cd_sense);
			if (!cd_sense)
				LINK_M14_Wr01(phy_link_06, phy_pdb_prt3, 0x0);			//PHY PDB ON
#else
			LINK_M14_Wr01(phy_link_06, phy_pdb_prt3, 0x0);			//PHY PDB ON
#endif
			LINK_M14_Wr01(phy_link_06, phy_enable_prt3, 0x0);		//PHY Enable
			LINK_M14_Wr01(phy_link_06, phy_eq_odt_sel_prt3, 0x0);
			LINK_M14_Wr01(phy_link_06, phy_rstn_prt3, 0x0);			//PHY RESET
			LINK_M14_WrFL(phy_link_06);
			break;

		default :
			ret = RET_ERROR;
			break;

	}

	return ret;
}

#ifdef ODT_PDB_OFF_ON_WORKAROUND
static int _HDMI_M14B0_Phy_Enable_Register_Access(int port_num)
{
	int ret = RET_OK;

//	HDMI_DEBUG("[%s] Entered with [%d] \n",__func__, port_num);

	switch(port_num)
	{
		case 0:
			LINK_M14_RdFL(phy_link_02);
			LINK_M14_Wr01(phy_link_02, phy_enable_prt1, 0x0);		//PHY Enable
			LINK_M14_WrFL(phy_link_02);
			LINK_M14_RdFL(phy_link_04);
			LINK_M14_Wr01(phy_link_04, phy_enable_prt2, 0x0);		//PHY Enable
			LINK_M14_WrFL(phy_link_04);
			LINK_M14_RdFL(phy_link_06);
			LINK_M14_Wr01(phy_link_06, phy_enable_prt3, 0x0);		//PHY Enable
			LINK_M14_WrFL(phy_link_06);

			LINK_M14_RdFL(phy_link_00);
			LINK_M14_Wr01(phy_link_00, phy_pdb_prt0, 0x1);			//PHY PDB ON
			LINK_M14_Wr01(phy_link_00, phy_enable_prt0, 0x1);		//PHY Enable
			LINK_M14_WrFL(phy_link_00);

			OS_UsecDelay(100);
			LINK_M14_RdFL(phy_link_00);
			LINK_M14_Wr01(phy_link_00, phy_rstn_prt0, 0x1);			//PHY RESET
			LINK_M14_WrFL(phy_link_00);

			break;

		case 1:
			LINK_M14_RdFL(phy_link_00);
			LINK_M14_Wr01(phy_link_00, phy_enable_prt0, 0x0);		//PHY Enable
			LINK_M14_WrFL(phy_link_00);
			LINK_M14_RdFL(phy_link_04);
			LINK_M14_Wr01(phy_link_04, phy_enable_prt2, 0x0);		//PHY Enable
			LINK_M14_WrFL(phy_link_04);
			LINK_M14_RdFL(phy_link_06);
			LINK_M14_Wr01(phy_link_06, phy_enable_prt3, 0x0);		//PHY Enable
			LINK_M14_WrFL(phy_link_06);

			LINK_M14_RdFL(phy_link_02);
			LINK_M14_Wr01(phy_link_02, phy_pdb_prt1, 0x1);			//PHY PDB ON
			LINK_M14_Wr01(phy_link_02, phy_enable_prt1, 0x1);		//PHY Enable
			LINK_M14_WrFL(phy_link_02);
			OS_UsecDelay(100);
			LINK_M14_RdFL(phy_link_02);
			LINK_M14_Wr01(phy_link_02, phy_rstn_prt1, 0x1);			//PHY RESET
			LINK_M14_WrFL(phy_link_02);

			break;

		case 2:
			LINK_M14_RdFL(phy_link_00);
			LINK_M14_Wr01(phy_link_00, phy_enable_prt0, 0x0);		//PHY Enable
			LINK_M14_WrFL(phy_link_00);
			LINK_M14_RdFL(phy_link_02);
			LINK_M14_Wr01(phy_link_02, phy_enable_prt1, 0x0);		//PHY Enable
			LINK_M14_WrFL(phy_link_02);
			LINK_M14_RdFL(phy_link_06);
			LINK_M14_Wr01(phy_link_06, phy_enable_prt3, 0x0);		//PHY Enable
			LINK_M14_WrFL(phy_link_06);

			LINK_M14_RdFL(phy_link_04);
			LINK_M14_Wr01(phy_link_04, phy_pdb_prt2, 0x1);			//PHY PDB ON
			LINK_M14_Wr01(phy_link_04, phy_enable_prt2, 0x1);		//PHY Enable
			LINK_M14_WrFL(phy_link_04);
			OS_UsecDelay(100);
			LINK_M14_RdFL(phy_link_04);
			LINK_M14_Wr01(phy_link_04, phy_rstn_prt2, 0x1);			//PHY RESET
			LINK_M14_WrFL(phy_link_04);

			break;

		case 3:
			LINK_M14_RdFL(phy_link_00);
			LINK_M14_Wr01(phy_link_00, phy_enable_prt0, 0x0);		//PHY Enable
			LINK_M14_WrFL(phy_link_00);
			LINK_M14_RdFL(phy_link_02);
			LINK_M14_Wr01(phy_link_02, phy_enable_prt1, 0x0);		//PHY Enable
			LINK_M14_WrFL(phy_link_02);
			LINK_M14_RdFL(phy_link_04);
			LINK_M14_Wr01(phy_link_04, phy_enable_prt2, 0x0);		//PHY Enable
			LINK_M14_WrFL(phy_link_04);

			LINK_M14_RdFL(phy_link_06);
			LINK_M14_Wr01(phy_link_06, phy_pdb_prt3, 0x1);			//PHY PDB ON
			LINK_M14_Wr01(phy_link_06, phy_enable_prt3, 0x1);		//PHY Enable
			LINK_M14_WrFL(phy_link_06);
			OS_UsecDelay(100);
			LINK_M14_RdFL(phy_link_06);
			LINK_M14_Wr01(phy_link_06, phy_rstn_prt3, 0x1);			//PHY RESET
			LINK_M14_WrFL(phy_link_06);

			break;

		default :
			ret = RET_ERROR;
			break;


	}

	return ret;
}

static int _HDMI_M14B0_EQ_PDB_control(int reset)
{
	int ret = RET_OK;
	HDMI_DEBUG("[%s] Entered with [%d]\n",__func__, reset);

	if(reset)
	{
		PHY_REG_M14B0_RdFL(pdb_d0_man_sel);
		PHY_REG_M14B0_Wr01(pdb_d0_man_sel,pdb_d0_man_sel,0x1);
		PHY_REG_M14B0_Wr01(pdb_d0_man_sel,pdb_d0_man,0x0);
		PHY_REG_M14B0_WrFL(pdb_d0_man_sel);

		PHY_REG_M14B0_RdFL(pdb_dck_man_sel);
		PHY_REG_M14B0_Wr01(pdb_dck_man_sel,pdb_dck_man_sel,0x1);
		PHY_REG_M14B0_Wr01(pdb_dck_man_sel,pdb_dck_man,0x0);
		PHY_REG_M14B0_WrFL(pdb_dck_man_sel);
	}
	else
	{
		PHY_REG_M14B0_RdFL(pdb_d0_man_sel);
		PHY_REG_M14B0_Wr01(pdb_d0_man_sel,pdb_d0_man_sel,0x0);
		PHY_REG_M14B0_Wr01(pdb_d0_man_sel,pdb_d0_man,0x1);
		PHY_REG_M14B0_WrFL(pdb_d0_man_sel);

		PHY_REG_M14B0_RdFL(pdb_dck_man_sel);
		PHY_REG_M14B0_Wr01(pdb_dck_man_sel,pdb_dck_man_sel,0x0);
		PHY_REG_M14B0_Wr01(pdb_dck_man_sel,pdb_dck_man,0x1);
		PHY_REG_M14B0_WrFL(pdb_dck_man_sel);
	}

	return ret;
}
#endif

static int _HDMI_M14B0_ResetPortControl(int reset_enable)
{
	int ret = RET_OK;

	HDMI_PRINT("[%s] Entered with [%d] \n",__func__, reset_enable);

	if (reset_enable)
	{
		LINK_M14_RdFL(phy_link_00);
		LINK_M14_Wr01(phy_link_00, link_sw_rstn_tmds_prt0, 0x0);
		LINK_M14_Wr01(phy_link_00, link_sw_rstn_hdcp_prt0, 0x0);
		LINK_M14_Wr01(phy_link_00, link_sw_rstn_edid_prt0, 0x0);
		LINK_M14_WrFL(phy_link_00);

		LINK_M14_RdFL(phy_link_02);
		LINK_M14_Wr01(phy_link_02, link_sw_rstn_tmds_prt1, 0x0);
		LINK_M14_Wr01(phy_link_02, link_sw_rstn_hdcp_prt1, 0x0);
		if(_gM14B0HDMIPhyInform.hpd_pol[1])
			LINK_M14_Wr01(phy_link_02, link_sw_rstn_edid_prt1, 0x0);
		LINK_M14_WrFL(phy_link_02);
		LINK_M14_RdFL(phy_link_04);
		LINK_M14_Wr01(phy_link_04, link_sw_rstn_tmds_prt2, 0x0);
		LINK_M14_Wr01(phy_link_04, link_sw_rstn_hdcp_prt2, 0x0);
		LINK_M14_Wr01(phy_link_04, link_sw_rstn_edid_prt2, 0x0);
		LINK_M14_WrFL(phy_link_04);

		LINK_M14_RdFL(phy_link_06);
		LINK_M14_Wr01(phy_link_06, link_sw_rstn_tmds_prt3, 0x0);
		LINK_M14_Wr01(phy_link_06, link_sw_rstn_hdcp_prt3, 0x0);
		LINK_M14_Wr01(phy_link_06, link_sw_rstn_edid_prt3, 0x0);
		LINK_M14_Wr01(phy_link_06, link_sw_rstn_cbus_prt3, 0x0);
		LINK_M14_WrFL(phy_link_06);
	}
	else
	{
		LINK_M14_RdFL(phy_link_00);
		LINK_M14_Wr01(phy_link_00, link_sw_rstn_tmds_prt0, 0x1);
		LINK_M14_Wr01(phy_link_00, link_sw_rstn_hdcp_prt0, 0x1);
		LINK_M14_Wr01(phy_link_00, link_sw_rstn_edid_prt0, 0x1);
		LINK_M14_WrFL(phy_link_00);

		LINK_M14_RdFL(phy_link_02);
		LINK_M14_Wr01(phy_link_02, link_sw_rstn_tmds_prt1, 0x1);
		LINK_M14_Wr01(phy_link_02, link_sw_rstn_hdcp_prt1, 0x1);
		if(_gM14B0HDMIPhyInform.hpd_pol[1])
			LINK_M14_Wr01(phy_link_02, link_sw_rstn_edid_prt1, 0x1);
		LINK_M14_WrFL(phy_link_02);

		LINK_M14_RdFL(phy_link_04);
		LINK_M14_Wr01(phy_link_04, link_sw_rstn_tmds_prt2, 0x1);
		LINK_M14_Wr01(phy_link_04, link_sw_rstn_hdcp_prt2, 0x1);
		LINK_M14_Wr01(phy_link_04, link_sw_rstn_edid_prt2, 0x1);
		LINK_M14_WrFL(phy_link_04);

		LINK_M14_RdFL(phy_link_06);
		LINK_M14_Wr01(phy_link_06, link_sw_rstn_tmds_prt3, 0x1);
		LINK_M14_Wr01(phy_link_06, link_sw_rstn_hdcp_prt3, 0x1);
		LINK_M14_Wr01(phy_link_06, link_sw_rstn_edid_prt3, 0x1);
		LINK_M14_Wr01(phy_link_06, link_sw_rstn_cbus_prt3, 0x1);
		LINK_M14_WrFL(phy_link_06);
	}

	return ret;
}

#ifdef NOT_USED_NOW
static int _HDMI_M14B0_ResetPort(int port_num)
{
	int ret = RET_OK;
	int cd_sense, cbus_conn_done, cbus_warb_done, hpd_in_prt3;

	HDMI_PRINT("[%s] Entered with [%d] \n",__func__, port_num);

	switch(port_num)
	{
		case 0:
			LINK_M14_RdFL(phy_link_00);
			LINK_M14_Wr01(phy_link_00, link_sw_rstn_tmds_prt0, 0x0);
			LINK_M14_Wr01(phy_link_00, link_sw_rstn_hdcp_prt0, 0x0);
			LINK_M14_Wr01(phy_link_00, link_sw_rstn_edid_prt0, 0x0);
			LINK_M14_WrFL(phy_link_00);
			OS_MsecSleep(10);	// ms delay
			LINK_M14_RdFL(phy_link_00);
			LINK_M14_Wr01(phy_link_00, link_sw_rstn_tmds_prt0, 0x1);
			LINK_M14_Wr01(phy_link_00, link_sw_rstn_hdcp_prt0, 0x1);
			LINK_M14_Wr01(phy_link_00, link_sw_rstn_edid_prt0, 0x1);
			LINK_M14_WrFL(phy_link_00);
			break;
		case 1:
			LINK_M14_RdFL(phy_link_02);
			LINK_M14_Wr01(phy_link_02, link_sw_rstn_tmds_prt1, 0x0);
			LINK_M14_Wr01(phy_link_02, link_sw_rstn_hdcp_prt1, 0x0);
			LINK_M14_Wr01(phy_link_02, link_sw_rstn_edid_prt1, 0x0);
			LINK_M14_WrFL(phy_link_02);
			OS_MsecSleep(10);	// ms delay
			LINK_M14_RdFL(phy_link_02);
			LINK_M14_Wr01(phy_link_02, link_sw_rstn_tmds_prt1, 0x1);
			LINK_M14_Wr01(phy_link_02, link_sw_rstn_hdcp_prt1, 0x1);
			LINK_M14_Wr01(phy_link_02, link_sw_rstn_edid_prt1, 0x1);
			LINK_M14_WrFL(phy_link_02);
			break;
		case 2:
			LINK_M14_RdFL(phy_link_04);
			LINK_M14_Wr01(phy_link_04, link_sw_rstn_tmds_prt2, 0x0);
			LINK_M14_Wr01(phy_link_04, link_sw_rstn_hdcp_prt2, 0x0);
			LINK_M14_Wr01(phy_link_04, link_sw_rstn_edid_prt2, 0x0);
			LINK_M14_WrFL(phy_link_04);
			OS_MsecSleep(10);	// ms delay
			LINK_M14_RdFL(phy_link_04);
			LINK_M14_Wr01(phy_link_04, link_sw_rstn_tmds_prt2, 0x1);
			LINK_M14_Wr01(phy_link_04, link_sw_rstn_hdcp_prt2, 0x1);
			LINK_M14_Wr01(phy_link_04, link_sw_rstn_edid_prt2, 0x1);
			LINK_M14_WrFL(phy_link_04);
			break;
		case 3:
			LINK_M14_RdFL(phy_link_06);
			LINK_M14_Wr01(phy_link_06, link_sw_rstn_tmds_prt3, 0x0);
			LINK_M14_Wr01(phy_link_06, link_sw_rstn_hdcp_prt3, 0x0);
			LINK_M14_Wr01(phy_link_06, link_sw_rstn_edid_prt3, 0x0);
			LINK_M14_RdFL(system_control_00);
			LINK_M14_Rd01(system_control_00, reg_cd_sense_prt3, cd_sense);
			LINK_M14_RdFL(cbus_01);
			LINK_M14_Rd01(cbus_01, reg_cbus_conn_done, cbus_conn_done);
			LINK_M14_Rd01(cbus_01, reg_cbus_warb_done, cbus_warb_done);
			LINK_M14_RdFL(phy_link_06);
			LINK_M14_Rd01(phy_link_06, hpd_in_prt3, hpd_in_prt3);			//PHY HPD IN
			if (cd_sense && (!cbus_conn_done || !cbus_warb_done ))	// CD_SENSE, but no cbus connection
			{
				HDMI_NOTI("---- MHL CD Sense , but no CBUS connection : SWRST CBUS\n");
				LINK_M14_Wr01(phy_link_06, link_sw_rstn_cbus_prt3, 0x0);
			}
			LINK_M14_WrFL(phy_link_06);
			OS_MsecSleep(10);	// ms delay
			LINK_M14_RdFL(phy_link_06);
			LINK_M14_Wr01(phy_link_06, link_sw_rstn_tmds_prt3, 0x1);
			LINK_M14_Wr01(phy_link_06, link_sw_rstn_hdcp_prt3, 0x1);
			LINK_M14_Wr01(phy_link_06, link_sw_rstn_edid_prt3, 0x1);
			LINK_M14_Wr01(phy_link_06, link_sw_rstn_cbus_prt3, 0x1);
			LINK_M14_WrFL(phy_link_06);
			break;
		default :
			ret = RET_ERROR;
			break;

	}

	return ret;
}
#endif	//#ifdef NOT_USED_NOW

static int _HDMI_M14B0_TMDS_ResetPort(int port_num)
{
	int ret = RET_OK;

	HDMI_DEBUG("[%s] Entered with [%d] \n",__func__, port_num);

	switch(port_num)
	{
		case 0:
			LINK_M14_RdFL(phy_link_00);
			LINK_M14_Wr01(phy_link_00, link_sw_rstn_tmds_prt0, 0x0);
			LINK_M14_WrFL(phy_link_00);
			OS_MsecSleep(5);	// ms delay
			LINK_M14_RdFL(phy_link_00);
			LINK_M14_Wr01(phy_link_00, link_sw_rstn_tmds_prt0, 0x1);
			LINK_M14_WrFL(phy_link_00);
			break;
		case 1:
			LINK_M14_RdFL(phy_link_02);
			LINK_M14_Wr01(phy_link_02, link_sw_rstn_tmds_prt1, 0x0);
			LINK_M14_WrFL(phy_link_02);
			OS_MsecSleep(5);	// ms delay
			LINK_M14_RdFL(phy_link_02);
			LINK_M14_Wr01(phy_link_02, link_sw_rstn_tmds_prt1, 0x1);
			LINK_M14_WrFL(phy_link_02);
			break;
		case 2:
			LINK_M14_RdFL(phy_link_04);
			LINK_M14_Wr01(phy_link_04, link_sw_rstn_tmds_prt2, 0x0);
			LINK_M14_WrFL(phy_link_04);
			OS_MsecSleep(5);	// ms delay
			LINK_M14_RdFL(phy_link_04);
			LINK_M14_Wr01(phy_link_04, link_sw_rstn_tmds_prt2, 0x1);
			LINK_M14_WrFL(phy_link_04);
			break;
		case 3:
			LINK_M14_RdFL(phy_link_06);
			LINK_M14_Wr01(phy_link_06, link_sw_rstn_tmds_prt3, 0x0);
			OS_MsecSleep(5);	// ms delay
			LINK_M14_RdFL(phy_link_06);
			LINK_M14_Wr01(phy_link_06, link_sw_rstn_tmds_prt3, 0x1);
			LINK_M14_WrFL(phy_link_06);
			break;
		default :
			ret = RET_ERROR;
			break;

	}

	return ret;
}

static int _HDMI_M14B0_TMDS_ResetPort_Control(int port_num, int reset)
{
	int ret = RET_OK;

//	HDMI_DEBUG("[%s] Entered with [%d][%d] \n",__func__, port_num, reset);

	switch(port_num)
	{
		case 0:
			LINK_M14_RdFL(phy_link_00);
			LINK_M14_Wr01(phy_link_00, link_sw_rstn_tmds_prt0, reset);
			LINK_M14_WrFL(phy_link_00);
			break;
		case 1:
			LINK_M14_RdFL(phy_link_02);
			LINK_M14_Wr01(phy_link_02, link_sw_rstn_tmds_prt1, reset);
			LINK_M14_WrFL(phy_link_02);
			break;
		case 2:
			LINK_M14_RdFL(phy_link_04);
			LINK_M14_Wr01(phy_link_04, link_sw_rstn_tmds_prt2, reset);
			LINK_M14_WrFL(phy_link_04);
			break;
		case 3:
			LINK_M14_RdFL(phy_link_06);
			LINK_M14_Wr01(phy_link_06, link_sw_rstn_tmds_prt3, reset);
			LINK_M14_WrFL(phy_link_06);
			break;
		default :
			ret = RET_ERROR;
			break;

	}

	return ret;
}

static int _HDMI_M14B0_HDCP_ResetPort_Control(int port_num, int reset)
{
	int ret = RET_OK;

	HDMI_DEBUG("[%s] Entered with [%d][%d] \n",__func__, port_num, reset);

	switch(port_num)
	{
		case 0:
			LINK_M14_RdFL(phy_link_00);
			LINK_M14_Wr01(phy_link_00, link_sw_rstn_hdcp_prt0, reset);
			LINK_M14_WrFL(phy_link_00);
			break;
		case 1:
			LINK_M14_RdFL(phy_link_02);
			LINK_M14_Wr01(phy_link_02, link_sw_rstn_hdcp_prt1, reset);
			LINK_M14_WrFL(phy_link_02);
			break;
		case 2:
			LINK_M14_RdFL(phy_link_04);
			LINK_M14_Wr01(phy_link_04, link_sw_rstn_hdcp_prt2, reset);
			LINK_M14_WrFL(phy_link_04);
			break;
		case 3:
			LINK_M14_RdFL(phy_link_06);
			LINK_M14_Wr01(phy_link_06, link_sw_rstn_hdcp_prt3, reset);
			LINK_M14_WrFL(phy_link_06);
			break;
		default :
			ret = RET_ERROR;
			break;

	}

	return ret;
}

#ifdef NOT_USED_NOW
static int _HDMI_M14B0_TMDS_HDCP_ResetPort_Control(int port_num, int reset)
{
	int ret = RET_OK;

	HDMI_DEBUG("[%s] Entered with [%d][%d] \n",__func__, port_num, reset);

	switch(port_num)
	{
		case 0:
			LINK_M14_RdFL(phy_link_00);
			LINK_M14_Wr01(phy_link_00, link_sw_rstn_tmds_prt0, reset);
			LINK_M14_Wr01(phy_link_00, link_sw_rstn_hdcp_prt0, reset);
			LINK_M14_WrFL(phy_link_00);
			break;
		case 1:
			LINK_M14_RdFL(phy_link_02);
			LINK_M14_Wr01(phy_link_02, link_sw_rstn_tmds_prt1, reset);
			LINK_M14_Wr01(phy_link_02, link_sw_rstn_hdcp_prt1, reset);
			LINK_M14_WrFL(phy_link_02);
			break;
		case 2:
			LINK_M14_RdFL(phy_link_04);
			LINK_M14_Wr01(phy_link_04, link_sw_rstn_tmds_prt2, reset);
			LINK_M14_Wr01(phy_link_04, link_sw_rstn_hdcp_prt2, reset);
			LINK_M14_WrFL(phy_link_04);
			break;
		case 3:
			LINK_M14_RdFL(phy_link_06);
			LINK_M14_Wr01(phy_link_06, link_sw_rstn_tmds_prt3, reset);
			LINK_M14_Wr01(phy_link_06, link_sw_rstn_hdcp_prt3, reset);
			LINK_M14_WrFL(phy_link_06);
			break;
		default :
			ret = RET_ERROR;
			break;

	}

	return ret;
}
#endif	//#ifdef NOT_USED_NOW

static int _HDMI_M14B0_HDCP_ResetPort(int port_num)
{
	int ret = RET_OK;

	HDMI_DEBUG("[%s] Entered with [%d] \n",__func__, port_num);

	switch(port_num)
	{
		case 0:
			LINK_M14_RdFL(phy_link_00);
			LINK_M14_Wr01(phy_link_00, link_sw_rstn_hdcp_prt0, 0x0);
			LINK_M14_WrFL(phy_link_00);
			OS_MsecSleep(5);	// ms delay
			LINK_M14_RdFL(phy_link_00);
			LINK_M14_Wr01(phy_link_00, link_sw_rstn_hdcp_prt0, 0x1);
			LINK_M14_WrFL(phy_link_00);
			break;
		case 1:
			LINK_M14_RdFL(phy_link_02);
			LINK_M14_Wr01(phy_link_02, link_sw_rstn_hdcp_prt1, 0x0);
			LINK_M14_WrFL(phy_link_02);
			OS_MsecSleep(5);	// ms delay
			LINK_M14_RdFL(phy_link_02);
			LINK_M14_Wr01(phy_link_02, link_sw_rstn_hdcp_prt1, 0x1);
			LINK_M14_WrFL(phy_link_02);
			break;
		case 2:
			LINK_M14_RdFL(phy_link_04);
			LINK_M14_Wr01(phy_link_04, link_sw_rstn_hdcp_prt2, 0x0);
			LINK_M14_WrFL(phy_link_04);
			OS_MsecSleep(5);	// ms delay
			LINK_M14_RdFL(phy_link_04);
			LINK_M14_Wr01(phy_link_04, link_sw_rstn_hdcp_prt2, 0x1);
			LINK_M14_WrFL(phy_link_04);
			break;
		case 3:
			LINK_M14_RdFL(phy_link_06);
			LINK_M14_Wr01(phy_link_06, link_sw_rstn_hdcp_prt3, 0x0);
			LINK_M14_WrFL(phy_link_06);
			OS_MsecSleep(5);	// ms delay
			LINK_M14_RdFL(phy_link_06);
			LINK_M14_Wr01(phy_link_06, link_sw_rstn_hdcp_prt3, 0x1);
			LINK_M14_WrFL(phy_link_06);
			break;
		default :
			ret = RET_ERROR;
			break;

	}

	return ret;
}

#ifdef NOT_USED_NOW
static int _HDMI_M14B0_TMDS_HDCP_ResetPort(int port_num)
{
	int ret = RET_OK;

	HDMI_DEBUG("[%s] Entered with [%d] \n",__func__, port_num);

	switch(port_num)
	{
		case 0:
			LINK_M14_RdFL(phy_link_00);
			LINK_M14_Wr01(phy_link_00, link_sw_rstn_tmds_prt0, 0x0);
			LINK_M14_Wr01(phy_link_00, link_sw_rstn_hdcp_prt0, 0x0);
			LINK_M14_WrFL(phy_link_00);
			OS_MsecSleep(5);	// ms delay
			LINK_M14_RdFL(phy_link_00);
			LINK_M14_Wr01(phy_link_00, link_sw_rstn_tmds_prt0, 0x1);
			LINK_M14_Wr01(phy_link_00, link_sw_rstn_hdcp_prt0, 0x1);
			LINK_M14_WrFL(phy_link_00);
			break;
		case 1:
			LINK_M14_RdFL(phy_link_02);
			LINK_M14_Wr01(phy_link_02, link_sw_rstn_tmds_prt1, 0x0);
			LINK_M14_Wr01(phy_link_02, link_sw_rstn_hdcp_prt1, 0x0);
			LINK_M14_WrFL(phy_link_02);
			OS_MsecSleep(5);	// ms delay
			LINK_M14_RdFL(phy_link_02);
			LINK_M14_Wr01(phy_link_02, link_sw_rstn_tmds_prt1, 0x1);
			LINK_M14_Wr01(phy_link_02, link_sw_rstn_hdcp_prt1, 0x1);
			LINK_M14_WrFL(phy_link_02);
			break;
		case 2:
			LINK_M14_RdFL(phy_link_04);
			LINK_M14_Wr01(phy_link_04, link_sw_rstn_tmds_prt2, 0x0);
			LINK_M14_Wr01(phy_link_04, link_sw_rstn_hdcp_prt2, 0x0);
			LINK_M14_WrFL(phy_link_04);
			OS_MsecSleep(5);	// ms delay
			LINK_M14_RdFL(phy_link_04);
			LINK_M14_Wr01(phy_link_04, link_sw_rstn_tmds_prt2, 0x1);
			LINK_M14_Wr01(phy_link_04, link_sw_rstn_hdcp_prt2, 0x1);
			LINK_M14_WrFL(phy_link_04);
			break;
		case 3:
			LINK_M14_RdFL(phy_link_06);
			LINK_M14_Wr01(phy_link_06, link_sw_rstn_tmds_prt3, 0x0);
			LINK_M14_Wr01(phy_link_06, link_sw_rstn_hdcp_prt3, 0x0);
			LINK_M14_WrFL(phy_link_06);
			OS_MsecSleep(5);	// ms delay
			LINK_M14_RdFL(phy_link_06);
			LINK_M14_Wr01(phy_link_06, link_sw_rstn_tmds_prt3, 0x1);
			LINK_M14_Wr01(phy_link_06, link_sw_rstn_hdcp_prt3, 0x1);
			LINK_M14_WrFL(phy_link_06);
			break;
		default :
			ret = RET_ERROR;
			break;

	}

	return ret;
}
#endif

static int _HDMI_M14B0_Get_EDID_Rd(int port_num , int *edid_rd_done, int *edid_rd_cnt)
{
	int ret = RET_OK;

	switch(port_num)
	{
		case 0:
			LINK_M14_RdFL(edid_heac_00);
			LINK_M14_Rd01(edid_heac_00, reg_edid_rd_done_prt0, *edid_rd_done);
			LINK_M14_Rd01(edid_heac_00, reg_edid_rd_cnt_prt0, *edid_rd_cnt);
			break;
		case 1:
			LINK_M14_RdFL(edid_heac_00);
			LINK_M14_Rd01(edid_heac_00, reg_edid_rd_done_prt1, *edid_rd_done);
			LINK_M14_Rd01(edid_heac_00, reg_edid_rd_cnt_prt1, *edid_rd_cnt);
			break;
		case 2:
			LINK_M14_RdFL(edid_heac_00);
			LINK_M14_Rd01(edid_heac_00, reg_edid_rd_done_prt2, *edid_rd_done);
			LINK_M14_Rd01(edid_heac_00, reg_edid_rd_cnt_prt2, *edid_rd_cnt);
			break;
		case 3:
			LINK_M14_RdFL(edid_heac_00);
			LINK_M14_Rd01(edid_heac_00, reg_edid_rd_done_prt3, *edid_rd_done);
			LINK_M14_Rd01(edid_heac_00, reg_edid_rd_cnt_prt3, *edid_rd_cnt);
			break;
		default :
			ret = RET_ERROR;
			break;

	}

	return ret;
}

static int _HDMI_M14B0_Get_HDCP_info(int port_num , UINT32 *hdcp_authed, UINT32 *hdcp_enc_en)
{
	int ret = RET_OK;

	switch(port_num)
	{
		case 0:
			LINK_M14_RdFL(hdcp_00);
			LINK_M14_Rd01(hdcp_00, reg_hdcp_authed_prt0, *hdcp_authed);
			LINK_M14_Rd01(hdcp_00, reg_hdcp_enc_en_prt0, *hdcp_enc_en);
			break;
		case 1:
			LINK_M14_RdFL(hdcp_06);
			LINK_M14_Rd01(hdcp_06, reg_hdcp_authed_prt1, *hdcp_authed);
			LINK_M14_Rd01(hdcp_06, reg_hdcp_enc_en_prt1, *hdcp_enc_en);
			break;
		case 2:
			LINK_M14_RdFL(hdcp_12);
			LINK_M14_Rd01(hdcp_12, reg_hdcp_authed_prt2, *hdcp_authed);
			LINK_M14_Rd01(hdcp_12, reg_hdcp_enc_en_prt2, *hdcp_enc_en);
			break;
		case 3:
			LINK_M14_RdFL(hdcp_18);
			LINK_M14_Rd01(hdcp_18, reg_hdcp_authed_prt3, *hdcp_authed);
			LINK_M14_Rd01(hdcp_18, reg_hdcp_enc_en_prt3, *hdcp_enc_en);
			break;
		default :
			ret = RET_ERROR;
			break;

	}

	return ret;
}

int HDMI_M14B0_Write_HDCP_Key(UINT8 *hdcp_key_data)
{
	int ret = RET_OK;

	unsigned char hdcp_data_wr[LX_HDCP_KEY_SIZE];
	int count;

	OS_LockMutex(&g_HDMI_Sema);

	HDMI_DEBUG("[%s] Entered \n",__func__);

	if (hdcp_key_data == NULL)
	{
		OS_UnlockMutex(&g_HDMI_Sema);

		return RET_ERROR;
	}

	/*
	for (count = 0; count < LX_HDCP_KEY_SIZE; count += 4) {
		memcpy((void *)(hdcp_data_wr+count), (void *)(hdcp_key_data+284-count), 4);
	}
	*/
		/*
	for (count = 0; count < LX_HDCP_KEY_SIZE; count ++)
		hdcp_data_wr[count] = hdcp_key_data[LX_HDCP_KEY_SIZE -1 -count];
*/
	hdcp_data_wr[0] = 0; 	// check-sum ???

	for (count = 1; count < 6; count ++)
		hdcp_data_wr[count] = hdcp_key_data[count - 1];

	for (count = 6; count < (LX_HDCP_KEY_SIZE - 2); count ++)
		hdcp_data_wr[count] = hdcp_key_data[count + 2];
	hdcp_data_wr[286] = 0;
	hdcp_data_wr[287] = 0;

	memcpy((void *)pHDCP_Key_addr, (void *)hdcp_data_wr, 288);

	// Reset HPD out to '0', for source device to restart HDCP Authentification.
	_HDMI_M14B0_HDCP_ResetPort_Control(0,0);
	_HDMI_M14B0_HDCP_ResetPort_Control(1,0);
	_HDMI_M14B0_HDCP_ResetPort_Control(2,0);
	_HDMI_M14B0_HDCP_ResetPort_Control(3,0);

	OS_MsecSleep(5);	// ms delay

	_HDMI_M14B0_HDCP_ResetPort_Control(0,1);
	_HDMI_M14B0_HDCP_ResetPort_Control(1,1);
	_HDMI_M14B0_HDCP_ResetPort_Control(2,1);
	_HDMI_M14B0_HDCP_ResetPort_Control(3,1);

	//_HDMI_M14B0_Set_HPD_Out_A4P(0,0,0,0);
	_HDMI_M14B0_Set_HPD_Out_A4P(!_gM14B0HDMIPhyInform.hpd_pol[0], \
			!_gM14B0HDMIPhyInform.hpd_pol[1], \
			!_gM14B0HDMIPhyInform.hpd_pol[2], \
			!_gM14B0HDMIPhyInform.hpd_pol[3]);

	OS_MsecSleep(10);	// ms delay

	OS_UnlockMutex(&g_HDMI_Sema);

	return ret;
}

int HDMI_M14B0_Read_HDCP_Key(UINT8 *hdcp_key_data)
{
	int ret = RET_OK;

	unsigned char hdcp_data_rd[LX_HDCP_KEY_SIZE];
	int count;

	HDMI_DEBUG("[%s] Entered \n",__func__);

	if (hdcp_key_data == NULL)
		return RET_ERROR;

	memcpy((void *)hdcp_data_rd, (void *)pHDCP_Key_addr, 288);

	/*
	for (count = 0; count < LX_HDCP_KEY_SIZE; count += 4) {
		memcpy((void *)(hdcp_key_data+count), (void *)(hdcp_data_rd+284-count), 4);
	}
	*/
	/*
	for (count = 0; count < LX_HDCP_KEY_SIZE; count ++)
		hdcp_key_data[count] = hdcp_data_rd[LX_HDCP_KEY_SIZE -1 -count];
		*/
	for (count = 0; count < 5; count ++)
		hdcp_key_data[count] = hdcp_data_rd[count + 1];
	for (count = 5; count < 8; count ++)
		hdcp_key_data[count] = 0;
	for (count = 8; count < LX_HDCP_KEY_SIZE; count ++)
		hdcp_key_data[count] = hdcp_data_rd[count - 2];

	return ret;
}

int HDMI_M14B0_Read_EDID_Data(int port_num , UINT8 *pedid_data)
{
	int ret = RET_OK;
	int count = 0;

	unsigned char edid_data_rd[LX_EDID_DATA_SIZE];

	HDMI_DEBUG("[%s] Entered with port [%d] \n",__func__, port_num);

	if (pedid_data == NULL)
		return RET_ERROR;

	if (port_num == 0)
		memcpy((void *)edid_data_rd, (void *)pEDID_data_addr_port0, 256);
	else if (port_num == 1)
		memcpy((void *)edid_data_rd, (void *)pEDID_data_addr_port1, 256);
	else if (port_num == 2)
		memcpy((void *)edid_data_rd, (void *)pEDID_data_addr_port2, 256);
	else if (port_num == 3)
		memcpy((void *)edid_data_rd, (void *)pEDID_data_addr_port3, 256);
	else
		return RET_ERROR;

	/*
	for (count = 0; count < 256; count += 4) {
		memcpy((void *)(pedid_data+count), (void *)(edid_data_rd+252-count), 4);
	}
	*/
	for (count = 0; count < LX_EDID_DATA_SIZE; count ++)
		pedid_data[count] = edid_data_rd[LX_EDID_DATA_SIZE - 1 - count];

	return ret;
}

int HDMI_M14B0_Write_EDID_Data(int port_num , UINT8 *pedid_data)
{
	int ret = RET_OK;
	int count = 0;
	UINT8 cs;

	unsigned char edid_data_wr[LX_EDID_DATA_SIZE];

	HDMI_DEBUG("[%s] Entered with port [%d] \n",__func__, port_num);

	if (pedid_data == NULL)
		return RET_ERROR;

	//-----------------------------
	//First Block checksum
	//-----------------------------
	for (cs = 0, count = 0x00; count < 0x7F; count++)
		cs += pedid_data[count];
	pedid_data[0x7F] = 0x100 -cs;

	HDMI_DEBUG("First Block Check Sum : port [%d] : [0x%x]\n",port_num, pedid_data[0x7F]);

	//-----------------------------
	//Second Block checksum
	//-----------------------------
	for (cs = 0, count = 0x80; count < 0xFF; count++)
		cs += pedid_data[count];
	pedid_data[0xFF] = 0x100 -cs;

	HDMI_DEBUG("Second Block Check Sum : port [%d] : [0x%x]\n",port_num, pedid_data[0xFF]);
	/*
	for (count = 0; count < 256; count += 4)
		memcpy((void *)(edid_data_wr+count), (void *)(pedid_data+252-count), 4);
		*/
	for (count = 0; count < LX_EDID_DATA_SIZE; count ++)
		edid_data_wr[count] = pedid_data[LX_EDID_DATA_SIZE - 1 - count];

	if (port_num == 0)
		memcpy((void *)pEDID_data_addr_port0, (void *)edid_data_wr, 256);
	else if (port_num == 1)
		memcpy((void *)pEDID_data_addr_port1, (void *)edid_data_wr, 256);
	else if (port_num == 2)
		memcpy((void *)pEDID_data_addr_port2, (void *)edid_data_wr, 256);
	else if (port_num == 3)
		memcpy((void *)pEDID_data_addr_port3, (void *)edid_data_wr, 256);
	else
		return RET_ERROR;

	return ret;
}

#ifdef NOT_USED_NOW
static int _HDMI_M14B0_Write_EDID_Data_A4P(UINT32 *pedid_data)
{
	int ret = RET_OK;

	if (pedid_data == NULL)
		return RET_ERROR;

	memcpy((void *)pEDID_data_addr_port0, (void *)pedid_data, 256*4);

	return ret;
}
#endif	//#ifdef NOT_USED_NOW

static int _HDMI_M14B0_Get_HDMI5V_Info_A4P(int *pHDMI5V_Status_PRT0, int *pHDMI5V_Status_PRT1 ,int *pHDMI5V_Status_PRT2, int *pHDMI5V_Status_PRT3)
{
	int ret = RET_OK;

	LINK_M14_RdFL(system_control_00);
	LINK_M14_Rd01(system_control_00, reg_hdmi5v_prt0, *pHDMI5V_Status_PRT0);
	LINK_M14_Rd01(system_control_00, reg_hdmi5v_prt1, *pHDMI5V_Status_PRT1);
	LINK_M14_Rd01(system_control_00, reg_hdmi5v_prt2, *pHDMI5V_Status_PRT2);
	LINK_M14_Rd01(system_control_00, reg_hdmi5v_prt3, *pHDMI5V_Status_PRT3);

	return ret;
}

static int _HDMI_M14B0_Get_HDMI5V_Info(int port_num, int *pHDMI5V_Status)
{
	int ret = RET_OK;

	LINK_M14_RdFL(system_control_00);

	if (port_num == 0)
		LINK_M14_Rd01(system_control_00, reg_hdmi5v_prt0, *pHDMI5V_Status);
	else if (port_num == 1)
		LINK_M14_Rd01(system_control_00, reg_hdmi5v_prt1, *pHDMI5V_Status);
	else if (port_num == 2)
		LINK_M14_Rd01(system_control_00, reg_hdmi5v_prt2, *pHDMI5V_Status);
	else if (port_num == 3)
		LINK_M14_Rd01(system_control_00, reg_hdmi5v_prt3, *pHDMI5V_Status);
	else
		return RET_ERROR;

	return ret;
}

static int _HDMI_M14B0_Get_HPD_Out_A4P(int *pHPD_Out_PRT0, int *pHPD_Out_PRT1 ,int *pHPD_Out_PRT2, int *pHPD_Out_PRT3)
{
	int ret = RET_OK;

	LINK_M14_RdFL(phy_link_00);
	LINK_M14_Rd01(phy_link_00, hpd_out_prt0, *pHPD_Out_PRT0);

	LINK_M14_RdFL(phy_link_02);
	LINK_M14_Rd01(phy_link_02, hpd_out_prt1, *pHPD_Out_PRT1);

	LINK_M14_RdFL(phy_link_04);
	LINK_M14_Rd01(phy_link_04, hpd_out_prt2, *pHPD_Out_PRT2);

	LINK_M14_RdFL(phy_link_06);
	LINK_M14_Rd01(phy_link_06, hpd_out_prt3, *pHPD_Out_PRT3);

	return ret;
}

static int _HDMI_M14B0_Set_HPD_Out(int prt_sel, int hpd_out_value)
{
	int ret = RET_OK;

	if (prt_sel == 0)
	{
		LINK_M14_RdFL(phy_link_00);
		LINK_M14_Wr01(phy_link_00, hpd_out_prt0, hpd_out_value);
		LINK_M14_WrFL(phy_link_00);
	}
	else if (prt_sel == 1)
	{
		LINK_M14_RdFL(phy_link_02);
		LINK_M14_Wr01(phy_link_02, hpd_out_prt1, hpd_out_value);
		LINK_M14_WrFL(phy_link_02);
	}
	else if (prt_sel == 2)
	{
		LINK_M14_RdFL(phy_link_04);
		LINK_M14_Wr01(phy_link_04, hpd_out_prt2, hpd_out_value);
		LINK_M14_WrFL(phy_link_04);
	}
	else if (prt_sel == 3)
	{
		LINK_M14_RdFL(phy_link_06);
		LINK_M14_Wr01(phy_link_06, hpd_out_prt3, hpd_out_value);
		LINK_M14_WrFL(phy_link_06);
	}
	else
		ret = RET_ERROR;

	return ret;
}
static int _HDMI_M14B0_Set_HPD_Out_A4P(int HPD_Out_PRT0, int HPD_Out_PRT1 ,int HPD_Out_PRT2, int HPD_Out_PRT3)
{
	int ret = RET_OK;

	HDMI_DEBUG("[%s] Entered with [%d][%d][%d][%d] \n",__func__, HPD_Out_PRT0, HPD_Out_PRT1, HPD_Out_PRT2, HPD_Out_PRT3);

	LINK_M14_RdFL(phy_link_00);
	LINK_M14_Wr01(phy_link_00, hpd_out_prt0, HPD_Out_PRT0);
	LINK_M14_WrFL(phy_link_00);

	LINK_M14_RdFL(phy_link_02);
	LINK_M14_Wr01(phy_link_02, hpd_out_prt1, HPD_Out_PRT1);
	LINK_M14_WrFL(phy_link_02);

	LINK_M14_RdFL(phy_link_04);
	LINK_M14_Wr01(phy_link_04, hpd_out_prt2, HPD_Out_PRT2);
	LINK_M14_WrFL(phy_link_04);

	LINK_M14_RdFL(phy_link_06);
	LINK_M14_Wr01(phy_link_06, hpd_out_prt3, HPD_Out_PRT3);
	LINK_M14_WrFL(phy_link_06);

	return ret;
}

int HDMI_M14B0_Get_Phy_Status(LX_HDMI_PHY_INFORM_T *sp_hdmi_phy_status)
{
	if (sp_hdmi_phy_status == NULL)
		return RET_ERROR;

	if (!_gM14B0HDMI_thread_running)
	{
		int port_num;
		UINT32 temp;
		int	access_phy[4];
		UINT32 up_freq = 0,	down_freq = 0;

		LINK_M14_RdFL(system_control_00);
		LINK_M14_Rd01(system_control_00, reg_prt_sel, _gM14B0HDMIPhyInform.prt_sel);

		LINK_M14_RdFL(phy_link_00);
		LINK_M14_Rd01(phy_link_00, phy_pdb_prt0, _gM14B0HDMIPhyInform.phy_pdb[0]);			//PHY PDB ON
		LINK_M14_Rd01(phy_link_00, phy_enable_prt0, _gM14B0HDMIPhyInform.phy_enable[0]);		//PHY Enable
		LINK_M14_Rd01(phy_link_00, phy_rstn_prt0, _gM14B0HDMIPhyInform.phy_rstn[0]);			//PHY RESET
		LINK_M14_Rd01(phy_link_00, hpd_in_prt0, _gM14B0HDMIPhyInform.hpd_in[0]);			//PHY HPD IN

		LINK_M14_RdFL(phy_link_02);
		LINK_M14_Rd01(phy_link_02, phy_pdb_prt1, _gM14B0HDMIPhyInform.phy_pdb[1]);			//PHY PDB ON
		LINK_M14_Rd01(phy_link_02, phy_enable_prt1, _gM14B0HDMIPhyInform.phy_enable[1]);		//PHY Enable
		LINK_M14_Rd01(phy_link_02, phy_rstn_prt1, _gM14B0HDMIPhyInform.phy_rstn[1]);			//PHY RESET
		LINK_M14_Rd01(phy_link_02, hpd_in_prt1, _gM14B0HDMIPhyInform.hpd_in[1]);			//PHY HPD IN

		LINK_M14_RdFL(phy_link_04);
		LINK_M14_Rd01(phy_link_04, phy_pdb_prt2, _gM14B0HDMIPhyInform.phy_pdb[2]);			//PHY PDB ON
		LINK_M14_Rd01(phy_link_04, phy_enable_prt2, _gM14B0HDMIPhyInform.phy_enable[2]);		//PHY Enable
		LINK_M14_Rd01(phy_link_04, phy_rstn_prt2, _gM14B0HDMIPhyInform.phy_rstn[2]);			//PHY RESET
		LINK_M14_Rd01(phy_link_04, hpd_in_prt2, _gM14B0HDMIPhyInform.hpd_in[2]);			//PHY HPD IN

		LINK_M14_RdFL(phy_link_06);
		LINK_M14_Rd01(phy_link_06, phy_pdb_prt3, _gM14B0HDMIPhyInform.phy_pdb[3]);			//PHY PDB ON
		LINK_M14_Rd01(phy_link_06, phy_enable_prt3, _gM14B0HDMIPhyInform.phy_enable[3]);		//PHY Enable
		LINK_M14_Rd01(phy_link_06, phy_rstn_prt3, _gM14B0HDMIPhyInform.phy_rstn[3]);			//PHY RESET
		LINK_M14_Rd01(phy_link_06, hpd_in_prt3, _gM14B0HDMIPhyInform.hpd_in[3]);			//PHY HPD IN

		LINK_M14_Rd01(system_control_00, reg_scdt_prt0, _gM14B0HDMIPhyInform.scdt[0]);
		LINK_M14_Rd01(system_control_00, reg_scdt_prt1, _gM14B0HDMIPhyInform.scdt[1]);
		LINK_M14_Rd01(system_control_00, reg_scdt_prt2, _gM14B0HDMIPhyInform.scdt[2]);
		LINK_M14_Rd01(system_control_00, reg_scdt_prt3, _gM14B0HDMIPhyInform.scdt[3]);

		LINK_M14_Rd01(system_control_00, reg_hdmi_mode_sel, _gM14B0HDMIPhyInform.hdmi_mode);

		_HDMI_M14B0_Get_HDMI5V_Info_A4P(&_gM14B0HDMIPhyInform.hdmi5v[0], &_gM14B0HDMIPhyInform.hdmi5v[1] ,&_gM14B0HDMIPhyInform.hdmi5v[2], &_gM14B0HDMIPhyInform.hdmi5v[3]);

		_HDMI_M14B0_Get_HPD_Out_A4P(&_gM14B0HDMIPhyInform.hpd_out[0], &_gM14B0HDMIPhyInform.hpd_out[1] ,&_gM14B0HDMIPhyInform.hpd_out[2], &_gM14B0HDMIPhyInform.hpd_out[3]);

		for (port_num = 0; port_num < 4;port_num ++)
		{
			_HDMI_M14B0_Get_EDID_Rd(port_num , &_gM14B0HDMIPhyInform.edid_rd_done[port_num], &temp);
			_HDMI_M14B0_Get_HDCP_info(port_num , &_gM14B0HDMIPhyInform.hdcp_authed[port_num], &_gM14B0HDMIPhyInform.hdcp_enc_en[port_num]);
		}

		LINK_M14_RdFL(system_control_00);
		LINK_M14_Rd01(system_control_00, reg_cd_sense_prt3, _gM14B0HDMIPhyInform.cd_sense);
		LINK_M14_RdFL(cbus_01);
		LINK_M14_Rd01(cbus_01, reg_cbus_conn_done, _gM14B0HDMIPhyInform.cbus_conn_done);
		LINK_M14_Rd01(cbus_01, reg_cbus_warb_done, _gM14B0HDMIPhyInform.cbus_warb_done);
		LINK_M14_Rd01(cbus_01, reg_cbus_disconn, _gM14B0HDMIPhyInform.cbus_disconn);			// CBUS DisConnected ???
		LINK_M14_RdFL(cbus_00);
		LINK_M14_Rd01(cbus_00, reg_phy_sink_cbus_zdis, _gM14B0HDMIPhyInform.phy_sink_cbus_zdis);
		LINK_M14_Rd01(cbus_00, reg_cbus_st, _gM14B0HDMIPhyInform.cbus_st);

		for (port_num=0;port_num<4;port_num++)
			access_phy[port_num] = _gM14B0HDMIPhyInform.phy_pdb[port_num] && _gM14B0HDMIPhyInform.phy_enable[port_num] && _gM14B0HDMIPhyInform.phy_rstn[port_num] ;

		if (access_phy[0] || access_phy[1] ||access_phy[2] ||access_phy[3] )
		{
			PHY_REG_M14B0_RdFL(tmds_freq_1);
			PHY_REG_M14B0_RdFL(tmds_freq_2);

			PHY_REG_M14B0_Rd01(tmds_freq_1,tmds_freq,up_freq);
			PHY_REG_M14B0_Rd01(tmds_freq_2,tmds_freq,down_freq);

			_gM14B0HDMIPhyInform.tmds_clock = ((up_freq << 8) + down_freq); 	// XXX.XX KHz
		}
		else
			_gM14B0HDMIPhyInform.tmds_clock = 0;

		LINK_M14_RdFL(cbus_13);
		LINK_M14_Rd01(cbus_13, reg_state_sink_rcp, _gM14B0HDMIPhyInform.state_sink_rcp);
	}

	LINK_M14_RdFL(phy_link_00);
	LINK_M14_Rd01(phy_link_00, link_sw_rstn_edid_prt0, _gM14B0HDMIPhyInform.rstn_edid[0]);
	LINK_M14_RdFL(phy_link_02);
	LINK_M14_Rd01(phy_link_02, link_sw_rstn_edid_prt1, _gM14B0HDMIPhyInform.rstn_edid[1]);
	LINK_M14_RdFL(phy_link_04);
	LINK_M14_Rd01(phy_link_04, link_sw_rstn_edid_prt2, _gM14B0HDMIPhyInform.rstn_edid[2]);
	LINK_M14_RdFL(phy_link_06);
	LINK_M14_Rd01(phy_link_06, link_sw_rstn_edid_prt3, _gM14B0HDMIPhyInform.rstn_edid[3]);

	memcpy(sp_hdmi_phy_status, &_gM14B0HDMIPhyInform, sizeof(LX_HDMI_PHY_INFORM_T));


	return RET_OK;
}

static int _HDMI_M14B0_GetMHLConection(void)
{
	int cd_sense, /*hpd_in_prt3, */prt_selected, phy_en_prt0, phy_en_prt1, phy_en_prt2, phy_en_prt3, /* cbus_disconn, */ cbus_st;
	int cbus_warb_done;
	static int _mhl_stable_count = 0;
	static int cd_sense_changed = 0;
	static int mhl_pdb_delayed_control = 0;
	static unsigned int cdsense_detection_time = 0;
	static unsigned int cdsense_current_time = 0;
	/* Port 3 MHL Connection */
	LINK_M14_RdFL(system_control_00);
	LINK_M14_Rd01(system_control_00, reg_cd_sense_prt3, cd_sense);
	LINK_M14_Rd01(system_control_00, reg_prt_sel, prt_selected);

	LINK_M14_RdFL(cbus_01);
	LINK_M14_Rd01(cbus_01, reg_cbus_conn_done, _gM14B0HDMIPhyInform.cbus_conn_done);
	//	LINK_M14_Rd01(cbus_01, reg_cbus_warb_done, _gM14B0HDMIPhyInform.cbus_warb_done);
	LINK_M14_Rd01(cbus_01, reg_cbus_warb_done, cbus_warb_done);
	LINK_M14_Rd01(cbus_01, reg_cbus_disconn, _gM14B0HDMIPhyInform.cbus_disconn);			// CBUS DisConnected ???
	LINK_M14_RdFL(cbus_00);
	LINK_M14_Rd01(cbus_00, reg_phy_sink_cbus_zdis, _gM14B0HDMIPhyInform.phy_sink_cbus_zdis);
	//LINK_M14_Rd01(cbus_00, reg_cbus_st, _gM14B0HDMIPhyInform.cbus_st);
	LINK_M14_Rd01(cbus_00, reg_cbus_st, cbus_st);

	_gM14B0HDMIPhyInform.cbus_00 = LINK_M14_Rd(cbus_00);
	_gM14B0HDMIPhyInform.cbus_01 = LINK_M14_Rd(cbus_01);

	LINK_M14_RdFL(cbus_13);
	LINK_M14_Rd01(cbus_13, reg_state_sink_rcp, _gM14B0HDMIPhyInform.state_sink_rcp);

	if (cd_sense != _gM14B0HDMIPhyInform.cd_sense)
	{
		HDMI_NOTI("---- MHL CD SENSE : [%d] => [%d]\n", _gM14B0HDMIPhyInform.cd_sense, cd_sense);
		_gM14B0HDMIPhyInform.cd_sense = cd_sense;

		if (!gHDMI_Phy_Control.all_port_pdb_enable && _gM14B0HDMIPhyInform.cd_sense && !_gM14B0HDMIPhyInform.phy_pdb[3] )
		{
			HDMI_DEBUG("---- MHL port 3 phy pdb force ON : [%d] => [%d]\n", _gM14B0HDMIPhyInform.phy_pdb[3], 1);
			LINK_M14_RdFL(phy_link_06);
			LINK_M14_Wr01(phy_link_06, phy_pdb_prt3, 1);			//PHY PDB ON
			LINK_M14_WrFL(phy_link_06);
			LINK_M14_RdFL(phy_link_06);
			LINK_M14_Rd01(phy_link_06, phy_pdb_prt3, _gM14B0HDMIPhyInform.phy_pdb[3]);			//PHY PDB ON
		}

		LINK_M14_RdFL(phy_link_06);
		LINK_M14_Rd01(phy_link_06, phy_enable_prt3, phy_en_prt3);

		if (!gHDMI_Phy_Control.all_port_pdb_enable)		// not used
		{
			if ( (prt_selected==0x3) && phy_en_prt3)
			{
				PHY_REG_M14B0_RdFL(dr_filter_ch0);
				PHY_REG_M14B0_RdFL(cr_icp_adj);
				if (cd_sense)
				{
#ifdef M14_CODE_FOR_MHL_CTS
#ifdef M14_CBUS_PDB_CTRL
					HDMI_DEBUG("---- PHY INIT for MHL,  PDB_D0_MAN_SEL :0x10, PDB_DCK_MAN_SEL :0x10\n");
					PHY_REG_M14B0_RdFL(pdb_d0_man_sel);
					PHY_REG_M14B0_Wr01(pdb_d0_man_sel,pdb_d0_man_sel,0x1);
					PHY_REG_M14B0_Wr01(pdb_d0_man_sel,pdb_d0_man,0x0);
					PHY_REG_M14B0_WrFL(pdb_d0_man_sel);

					PHY_REG_M14B0_RdFL(pdb_dck_man_sel);
					PHY_REG_M14B0_Wr01(pdb_dck_man_sel,pdb_dck_man_sel,0x1);
					PHY_REG_M14B0_Wr01(pdb_dck_man_sel,pdb_dck_man,0x0);
					PHY_REG_M14B0_WrFL(pdb_dck_man_sel);
#endif
#endif
					HDMI_DEBUG("---- MHL port : cd sense ON [%s]\n", __func__);
					HDMI_DEBUG("---- MHL CD Sense dr_filter_ch0 : [%d]\n", 0x1);
					PHY_REG_M14B0_Wr01(dr_filter_ch0,dr_filter_ch0,0x1);
					HDMI_DEBUG("---- MHL CR_PLL Charge Pump Current Adjust : [%d]\n", 0x3);
					PHY_REG_M14B0_Wr01(cr_icp_adj,cr_icp_adj,0x3);
				}
				else
				{
					HDMI_DEBUG("---- HDMI Mode dr_filter_ch0 : [%d]\n", 0x7);
					PHY_REG_M14B0_Wr01(dr_filter_ch0,dr_filter_ch0,0x7);
					HDMI_DEBUG("---- HDMI Mode  CR_PLL Charge Pump Current Adjust : [%d]\n", 0x4);
					PHY_REG_M14B0_Wr01(cr_icp_adj,cr_icp_adj,0x4);
				}
				PHY_REG_M14B0_WrFL(dr_filter_ch0);
				PHY_REG_M14B0_WrFL(cr_icp_adj);
			}
		}
		else	// all port pdb enabled condition, if cbus change detected ?
		{
			cd_sense_changed = 1;

			if(cd_sense)
			{
				mhl_pdb_delayed_control = 1;
				cdsense_detection_time = jiffies_to_msecs(jiffies);
			}
			else
			{
				mhl_pdb_delayed_control = 0;
			}
		}
	}
	if(cd_sense_changed)
		{
			int phy_pdb_prt3, phy_rstn_prt3;
			LINK_M14_RdFL(phy_link_06);
			LINK_M14_Rd01(phy_link_06, phy_pdb_prt3, phy_pdb_prt3);			//PHY PDB ON
			LINK_M14_Rd01(phy_link_06, phy_rstn_prt3, phy_rstn_prt3);			//PHY RESET

			if (phy_pdb_prt3 && phy_rstn_prt3)
			{

				LINK_M14_RdFL(phy_link_06);
				LINK_M14_Rd01(phy_link_06, phy_enable_prt3, phy_en_prt3);		//PHY Enable

				if (!phy_en_prt3)	// IF port3(C-BUS) is not enabled?
				{
					LINK_M14_RdFL(phy_link_00);
					LINK_M14_Rd01(phy_link_00, phy_enable_prt0, phy_en_prt0);		//PHY Enable

					LINK_M14_RdFL(phy_link_02);
					LINK_M14_Rd01(phy_link_02, phy_enable_prt1, phy_en_prt1);		//PHY Enable

					LINK_M14_RdFL(phy_link_04);
					LINK_M14_Rd01(phy_link_04, phy_enable_prt2, phy_en_prt2);		//PHY Enable

					_HDMI_M14B0_EnablePort(0, 0);
					_HDMI_M14B0_EnablePort(1, 0);
					_HDMI_M14B0_EnablePort(2, 0);
					_HDMI_M14B0_EnablePort(3, 1);
				}
				OS_MsecSleep(10); // delay after enable port and phy register write

				PHY_REG_M14B0_RdFL(dr_filter_ch0);
				PHY_REG_M14B0_RdFL(cr_icp_adj);
				PHY_REG_M14B0_RdFL(eq_rs_man);
				if (cd_sense)
				{
					HDMI_DEBUG("---- MHL port : cd sense ON [%s]\n", __func__);
					HDMI_DEBUG("---- MHL CD Sense dr_filter_ch0 : [%d]\n", 0x1);
					PHY_REG_M14B0_Wr01(dr_filter_ch0,dr_filter_ch0,0x1);
					HDMI_DEBUG("---- MHL CR_PLL Charge Pump Current Adjust : [%d]\n", 0x2);
					PHY_REG_M14B0_Wr01(cr_icp_adj,cr_icp_adj,0x2);	// 0x3 //140514 : MHL CT B2 and new board
					HDMI_DEBUG("---- MHL EQ_RS_MAN  : [%d]\n", 0x3);
					PHY_REG_M14B0_Wr01(eq_rs_man,eq_rs_man,0x3);	//from 0x3(default) // 0x5 //140514 : MHL CT B2 and new board


#ifdef M14_CODE_FOR_MHL_CTS

					HDMI_DEBUG("---- MHL ODC CTRL  : [%d]\n", 0x0);
					PHY_REG_M14B0_RdFL(odt_ctrl);
					PHY_REG_M14B0_Wr01(odt_ctrl,odt_ctrl,0x1);		// 0x0 //140514 : new board impedance 55=>50Ohm
					PHY_REG_M14B0_WrFL(odt_ctrl);
#endif
					HDMI_DEBUG("---- MHL DR_N1  : [%d]\n", 0x0);
					PHY_REG_M14B0_RdFL(dr_n1);
					PHY_REG_M14B0_Wr01(dr_n1,dr_n1,0x0);	// for 15m cable (0x38/0x93)
					PHY_REG_M14B0_WrFL(dr_n1);
				}
				else
				{
					HDMI_DEBUG("---- HDMI Mode dr_filter_ch0 : [%d]\n", 0x7);
					PHY_REG_M14B0_Wr01(dr_filter_ch0,dr_filter_ch0,0x7);
					HDMI_DEBUG("---- HDMI Mode  CR_PLL Charge Pump Current Adjust : [%d]\n", 0x4);
					PHY_REG_M14B0_Wr01(cr_icp_adj,cr_icp_adj,0x4);
#ifdef M14_HDMI_SEMI_AUTO_EQ_CONTROL
					HDMI_DEBUG("---- HDMI EQ_RS_MAN  : [%d]\n", 0x3);
					PHY_REG_M14B0_Wr01(eq_rs_man,eq_rs_man,0x3);
#else
					HDMI_DEBUG("---- HDMI EQ_RS_MAN  : [%d]\n", 0x3);
					/* BD570 EQ error : EQ RS value 0x7 (default 0x3) , 131114 */
					PHY_REG_M14B0_Wr01(eq_rs_man,eq_rs_man,0x3);
#endif
#ifdef M14_CODE_FOR_MHL_CTS
					HDMI_DEBUG("---- HDMI ODC CTRL  : [%d]\n", 0x1);
					PHY_REG_M14B0_RdFL(odt_ctrl);
					PHY_REG_M14B0_Wr01(odt_ctrl,odt_ctrl,0x1);
					PHY_REG_M14B0_WrFL(odt_ctrl);
#endif
					HDMI_DEBUG("---- HDMI DR_N1  : [%d]\n", 0x3);
					PHY_REG_M14B0_RdFL(dr_n1);
					PHY_REG_M14B0_Wr01(dr_n1,dr_n1,0x3);	// for 15m cable (0x38/0x93)
					PHY_REG_M14B0_WrFL(dr_n1);
				}
				PHY_REG_M14B0_WrFL(dr_filter_ch0);
				PHY_REG_M14B0_WrFL(cr_icp_adj);
				PHY_REG_M14B0_WrFL(eq_rs_man);

				if (!phy_en_prt3)
				{
					_HDMI_M14B0_EnablePort(0, phy_en_prt0);
					_HDMI_M14B0_EnablePort(1, phy_en_prt1);
					_HDMI_M14B0_EnablePort(2, phy_en_prt2);
					_HDMI_M14B0_EnablePort(3, phy_en_prt3);
				}
			cd_sense_changed = 0;
		}
		else
		{
			HDMI_DEBUG("---- MHL cd sense , but port is off\n");
		}
	}
	if(mhl_pdb_delayed_control)
	{
		int phy_pdb_prt3, phy_rstn_prt3;
		cdsense_current_time = jiffies_to_msecs(jiffies);
		if( (cdsense_current_time - cdsense_detection_time) > 2000)		// odt pdb power down after 2sec (for PRADA, optimus 3D phones)
		{
			HDMI_DEBUG("---- MHL [%d sec] after cdsense \n", (cdsense_current_time - cdsense_detection_time));
			LINK_M14_RdFL(phy_link_06);
			LINK_M14_Rd01(phy_link_06, phy_pdb_prt3, phy_pdb_prt3);			//PHY PDB ON
			LINK_M14_Rd01(phy_link_06, phy_rstn_prt3, phy_rstn_prt3);			//PHY RESET
			if(phy_pdb_prt3 && phy_rstn_prt3)
			{
				LINK_M14_RdFL(phy_link_06);
				LINK_M14_Rd01(phy_link_06, phy_enable_prt3, phy_en_prt3);		//PHY Enable
				if(!phy_en_prt3)	// IF port3(C-BUS) is not enabled?
				{
					LINK_M14_RdFL(phy_link_00);
					LINK_M14_Rd01(phy_link_00, phy_enable_prt0, phy_en_prt0);		//PHY Enable
					LINK_M14_RdFL(phy_link_02);
					LINK_M14_Rd01(phy_link_02, phy_enable_prt1, phy_en_prt1);		//PHY Enable
					LINK_M14_RdFL(phy_link_04);
					LINK_M14_Rd01(phy_link_04, phy_enable_prt2, phy_en_prt2);		//PHY Enable
					_HDMI_M14B0_EnablePort(0, 0);
					_HDMI_M14B0_EnablePort(1, 0);
					_HDMI_M14B0_EnablePort(2, 0);
					_HDMI_M14B0_EnablePort(3, 1);
				}
				OS_MsecSleep(10); // delay after enable port and phy register write
#ifdef M14_CODE_FOR_MHL_CTS
#ifdef M14_CBUS_PDB_CTRL
				HDMI_DEBUG("---- CD_SENSE for MHL,  PDB_D0_MAN_SEL :0x10, PDB_DCK_MAN_SEL :0x10\n");
				PHY_REG_M14B0_RdFL(pdb_d0_man_sel);
				PHY_REG_M14B0_Wr01(pdb_d0_man_sel,pdb_d0_man_sel,0x1);
				PHY_REG_M14B0_Wr01(pdb_d0_man_sel,pdb_d0_man,0x0);
				PHY_REG_M14B0_WrFL(pdb_d0_man_sel);
				PHY_REG_M14B0_RdFL(pdb_dck_man_sel);
				PHY_REG_M14B0_Wr01(pdb_dck_man_sel,pdb_dck_man_sel,0x1);
				PHY_REG_M14B0_Wr01(pdb_dck_man_sel,pdb_dck_man,0x0);
				PHY_REG_M14B0_WrFL(pdb_dck_man_sel);
#endif
#endif
				if(!phy_en_prt3)
				{
					_HDMI_M14B0_EnablePort(0, phy_en_prt0);
					_HDMI_M14B0_EnablePort(1, phy_en_prt1);
					_HDMI_M14B0_EnablePort(2, phy_en_prt2);
					_HDMI_M14B0_EnablePort(3, phy_en_prt3);
				}
				mhl_pdb_delayed_control = 0;
			}
			else
			{
				HDMI_DEBUG("---- MHL cd sense , but port is off\n");
			}
		}
	}

	if (cbus_warb_done != _gM14B0HDMIPhyInform.cbus_warb_done)
	{
		HDMI_NOTI("---- MHL warb done : [%d] => [%d]\n", _gM14B0HDMIPhyInform.cbus_warb_done, cbus_warb_done);
		_gM14B0HDMIPhyInform.cbus_warb_done = cbus_warb_done;

#ifdef M14_CODE_FOR_MHL_CTS
#ifdef M14_CBUS_PDB_CTRL
		// Move to Interrupt
		if ( cbus_warb_done == 1)
		{
			LINK_M14_RdFL(phy_link_06);
			LINK_M14_Rd01(phy_link_06, phy_enable_prt3, phy_en_prt3);		//PHY Enable

			if (!phy_en_prt3)	// IF port3(C-BUS) is not enabled?
			{
				LINK_M14_RdFL(phy_link_00);
				LINK_M14_Rd01(phy_link_00, phy_enable_prt0, phy_en_prt0);		//PHY Enable

				LINK_M14_RdFL(phy_link_02);
				LINK_M14_Rd01(phy_link_02, phy_enable_prt1, phy_en_prt1);		//PHY Enable

				LINK_M14_RdFL(phy_link_04);
				LINK_M14_Rd01(phy_link_04, phy_enable_prt2, phy_en_prt2);		//PHY Enable

				_HDMI_M14B0_EnablePort(0, 0);
				_HDMI_M14B0_EnablePort(1, 0);
				_HDMI_M14B0_EnablePort(2, 0);
				_HDMI_M14B0_EnablePort(3, 1);
			}

			/*
			   HDMI_DEBUG("---- MHL warb_done:1, Set ODT PDB Mode to 0x00\n");
			   PHY_REG_M14B0_RdFL(eq_i2c_odt_pdb_mode);
			   PHY_REG_M14B0_Wr01(eq_i2c_odt_pdb_mode,eq_i2c_odt_pdb,0x0);
			   PHY_REG_M14B0_Wr01(eq_i2c_odt_pdb_mode,eq_i2c_odt_pdb_mode,0x0);
			   PHY_REG_M14B0_WrFL(eq_i2c_odt_pdb_mode);
			 */

			HDMI_DEBUG("---- MHL warb_done:1, PDB_D0_MAN_SEL :0x11, PDB_DCK_MAN_SEL :0x10\n");
			PHY_REG_M14B0_RdFL(pdb_d0_man_sel);
			PHY_REG_M14B0_Wr01(pdb_d0_man_sel,pdb_d0_man_sel,0x1);
			PHY_REG_M14B0_Wr01(pdb_d0_man_sel,pdb_d0_man,0x1);
			PHY_REG_M14B0_WrFL(pdb_d0_man_sel);

			PHY_REG_M14B0_RdFL(pdb_dck_man_sel);
			PHY_REG_M14B0_Wr01(pdb_dck_man_sel,pdb_dck_man_sel,0x1);
			PHY_REG_M14B0_Wr01(pdb_dck_man_sel,pdb_dck_man,0x0);
			PHY_REG_M14B0_WrFL(pdb_dck_man_sel);

			if (!phy_en_prt3)
			{
				_HDMI_M14B0_EnablePort(0, phy_en_prt0);
				_HDMI_M14B0_EnablePort(1, phy_en_prt1);
				_HDMI_M14B0_EnablePort(2, phy_en_prt2);
				_HDMI_M14B0_EnablePort(3, phy_en_prt3);
			}
		}
		else if (_gM14B0HDMIPhyInform.cd_sense)	// cd_sense, but not connection lost
#endif
		{
#if 0
			int phy_pdb_prt3, phy_rstn_prt3;
			LINK_M14_RdFL(phy_link_06);
			LINK_M14_Rd01(phy_link_06, phy_pdb_prt3, phy_pdb_prt3);			//PHY PDB ON
			LINK_M14_Rd01(phy_link_06, phy_rstn_prt3, phy_rstn_prt3);			//PHY RESET

			if (phy_pdb_prt3 && phy_rstn_prt3)
			{

				LINK_M14_RdFL(phy_link_06);
				LINK_M14_Rd01(phy_link_06, phy_enable_prt3, phy_en_prt3);		//PHY Enable

				if (!phy_en_prt3)	// IF port3(C-BUS) is not enabled?
				{
					LINK_M14_RdFL(phy_link_00);
					LINK_M14_Rd01(phy_link_00, phy_enable_prt0, phy_en_prt0);		//PHY Enable

					LINK_M14_RdFL(phy_link_02);
					LINK_M14_Rd01(phy_link_02, phy_enable_prt1, phy_en_prt1);		//PHY Enable

					LINK_M14_RdFL(phy_link_04);
					LINK_M14_Rd01(phy_link_04, phy_enable_prt2, phy_en_prt2);		//PHY Enable

					_HDMI_M14B0_EnablePort(0, 0);
					_HDMI_M14B0_EnablePort(1, 0);
					_HDMI_M14B0_EnablePort(2, 0);
					_HDMI_M14B0_EnablePort(3, 1);
				}

				/*
				   HDMI_DEBUG("---- MHL warb done:0 ,Set ODT PDB Mode to 0x10\n");
				   PHY_REG_M14B0_RdFL(eq_i2c_odt_pdb_mode);
				   PHY_REG_M14B0_Wr01(eq_i2c_odt_pdb_mode,eq_i2c_odt_pdb,0x0);
				   PHY_REG_M14B0_Wr01(eq_i2c_odt_pdb_mode,eq_i2c_odt_pdb_mode,0x1);
				   PHY_REG_M14B0_WrFL(eq_i2c_odt_pdb_mode);
				 */

				HDMI_DEBUG("---- MHL warb_done:0, PDB_D0_MAN_SEL :0x10, PDB_DCK_MAN_SEL :0x10\n");
				PHY_REG_M14B0_RdFL(pdb_d0_man_sel);
				PHY_REG_M14B0_Wr01(pdb_d0_man_sel,pdb_d0_man_sel,0x1);
				PHY_REG_M14B0_Wr01(pdb_d0_man_sel,pdb_d0_man,0x0);
				PHY_REG_M14B0_WrFL(pdb_d0_man_sel);

				PHY_REG_M14B0_RdFL(pdb_dck_man_sel);
				PHY_REG_M14B0_Wr01(pdb_dck_man_sel,pdb_dck_man_sel,0x1);
				PHY_REG_M14B0_Wr01(pdb_dck_man_sel,pdb_dck_man,0x0);
				PHY_REG_M14B0_WrFL(pdb_dck_man_sel);

				if (!phy_en_prt3)
				{
					_HDMI_M14B0_EnablePort(0, phy_en_prt0);
					_HDMI_M14B0_EnablePort(1, phy_en_prt1);
					_HDMI_M14B0_EnablePort(2, phy_en_prt2);
					_HDMI_M14B0_EnablePort(3, phy_en_prt3);
				}
			}
#endif
		}
#endif
	}

	if (cbus_st != _gM14B0HDMIPhyInform.cbus_st)
	{
		HDMI_NOTI("---- MHL CBUS_ST : [%d] => [%d]\n", _gM14B0HDMIPhyInform.cbus_st, cbus_st);
		_gM14B0HDMIPhyInform.cbus_st = cbus_st;

		if (_gM14B0HDMIPhyInform.cbus_st == 3)
		{
			_gM14B0CHG_AVI_count_MHL = 0;
			_mhl_stable_count = 0;
		}
#ifdef M14_CODE_FOR_MHL_CTS
		else if (_gM14B0HDMIPhyInform.cbus_st  < 2)
			_HDMI_M14B0_Check_RAP_Content(1);
#endif
	}

	if ( _gM14B0HDMIPhyInform.cd_sense)	//MHL Mode
	{
		LINK_M14_RdFL(cbus_00);

		if (LINK_M14_Rd_fld(cbus_00, reg_cbus_disconn_en) == 0)
		{
			OS_MsecSleep(10);	// ms delay
			LINK_M14_RdFL(cbus_00);
			LINK_M14_Wr01(cbus_00, reg_cbus_disconn_en, 0x1);
			LINK_M14_WrFL(cbus_00);
		}

#ifdef M14_CODE_FOR_MHL_CTS
		_HDMI_M14B0_Check_RAP_Content(0);
#if 0
		{

			LINK_M14_RdFL(cbus_04);
			LINK_M14_RdFL(cbus_05);
			LINK_M14_RdFL(cbus_09);

			if ( LINK_M14_Rd_fld(cbus_05, reg_man_msc_wrt_stat_pathen_clr) && ( LINK_M14_Rd_fld(cbus_09, reg_state_sink_wrt_stat_pathen_clr) == 1) )
			{
				LINK_M14_Wr01(cbus_05, reg_man_msc_wrt_stat_pathen_clr, 0x0);
				LINK_M14_WrFL(cbus_05);
				HDMI_DEBUG("---- MHL pathen clr : 0x0 \n");
			}
			if ( LINK_M14_Rd_fld(cbus_04, reg_man_msc_wrt_stat_pathen_set) )
			{
				if ( LINK_M14_Rd_fld(cbus_09, reg_state_sink_wrt_stat_pathen_set) == 1)
				{
					LINK_M14_Wr01(cbus_04, reg_man_msc_wrt_stat_pathen_set, 0x0);
					LINK_M14_WrFL(cbus_04);
					HDMI_DEBUG("---- MHL pathen set : 0x0 \n");
				}
				else
				{
					_gPathen_set_count ++;
					if (_gPathen_set_count > 50)
					{
						LINK_M14_Wr01(cbus_04, reg_man_msc_wrt_stat_pathen_set, 0x0);
						LINK_M14_WrFL(cbus_04);
						HDMI_DEBUG("---- MHL pathen set by time out: 0x0 \n");
						_gPathen_set_count = 0;
					}
				}

			}
		}
#endif
#endif
		if (_gM14B0HDMIPhyInform.cbus_warb_done && _gM14B0HDMIPhyInform.cbus_conn_done && (_gM14B0HDMIPhyInform.cbus_st == 3) )
		{
			/*
			   Workaround code for RCP Send Error
			 */
			 _HDMI_M14B0_MHL_Check_Status(0);
			/*
			   Workaround code for HTC beatsaudio phone
			 */
			if (_gM14B0CHG_AVI_count_MHL > 20)
			{
				HDMI_NOTI("---- MHL CHG_AVI_COUNT : [%d], mhl_stable_count : [%d] \n", _gM14B0CHG_AVI_count_MHL, _mhl_stable_count);

				_gM14B0CHG_AVI_count_MHL = -1;

				wake_up_interruptible(&WaitQueue_MHL_M14B0);
			}
			else if ( _gM14B0CHG_AVI_count_MHL != -1)
			{
				_mhl_stable_count ++;
				if (_mhl_stable_count > 3000) // for about 1-minute MHL stable
					_gM14B0CHG_AVI_count_MHL = -1;
			}

		}

#if 0
		LINK_M14_RdFL(cbus_01);
		LINK_M14_Rd01(cbus_01, reg_cbus_disconn, cbus_disconn);			// CBUS DisConnected ???
		//		LINK_M14_RdFL(phy_link_06);
		//		LINK_M14_Rd01(phy_link_06, hpd_in_prt3, hpd_in_prt3);			//PHY HPD IN

		//		if (hpd_in_prt3 != _gM14B0HDMIPhyInform.hpd_in[3])
		if (cbus_disconn)
		{
			//	HDMI_DEBUG("---- MHL HPD IN PORT3 : [%d] => [%d]\n",_gM14B0HDMIPhyInform.hpd_in[3], hpd_in_prt3);
			//	_gM14B0HDMIPhyInform.hpd_in[3] = hpd_in_prt3;

			//		if (_gM14B0HDMIPhyInform.hpd_in[3])
			if (1)
			{
				HDMI_DEBUG("---- MHL Clear cbus_disconn_en\n");
				LINK_M14_RdFL(cbus_00);
				LINK_M14_Wr01(cbus_00, reg_cbus_disconn_en, 0x0);
				LINK_M14_WrFL(cbus_00);
				OS_MsecSleep(10);	// ms delay
				LINK_M14_RdFL(cbus_00);
				LINK_M14_Wr01(cbus_00, reg_cbus_disconn_en, 0x1);
				LINK_M14_WrFL(cbus_00);
			}
		}
#endif
	}
#ifdef M14B0_MHL_CONNECTION_BUG_WORKAROUND
	if ( _gM14B0HDMIPhyInform.cd_sense)	//MHL Mode
	{
		LINK_M14_RdFL(cbus_01);
		LINK_M14_Rd01(cbus_01, reg_cbus_disconn, cbus_disconn);			// CBUS DisConnected ???
		//		LINK_M14_RdFL(phy_link_06);
		//		LINK_M14_Rd01(phy_link_06, hpd_in_prt3, hpd_in_prt3);			//PHY HPD IN

		//		if (hpd_in_prt3 != _gM14B0HDMIPhyInform.hpd_in[3])
		if (cbus_disconn)
		{
			//	HDMI_DEBUG("---- MHL HPD IN PORT3 : [%d] => [%d]\n",_gM14B0HDMIPhyInform.hpd_in[3], hpd_in_prt3);
			//	_gM14B0HDMIPhyInform.hpd_in[3] = hpd_in_prt3;

			//		if (_gM14B0HDMIPhyInform.hpd_in[3])
			if (1)
			{
				HDMI_NOTI("---- MHL SW Reset CBUS\n");
				LINK_M14_RdFL(phy_link_06);
				LINK_M14_Wr01(phy_link_06, link_sw_rstn_tmds_prt3, 0x0);
				LINK_M14_Wr01(phy_link_06, link_sw_rstn_hdcp_prt3, 0x0);
				LINK_M14_Wr01(phy_link_06, link_sw_rstn_edid_prt3, 0x0);
				LINK_M14_Wr01(phy_link_06, link_sw_rstn_cbus_prt3, 0x0);
				LINK_M14_WrFL(phy_link_06);
				OS_MsecSleep(10);	// ms delay
				LINK_M14_RdFL(phy_link_06);
				LINK_M14_Wr01(phy_link_06, link_sw_rstn_tmds_prt3, 0x1);
				LINK_M14_Wr01(phy_link_06, link_sw_rstn_hdcp_prt3, 0x1);
				LINK_M14_Wr01(phy_link_06, link_sw_rstn_edid_prt3, 0x1);
				LINK_M14_Wr01(phy_link_06, link_sw_rstn_cbus_prt3, 0x1);
				LINK_M14_WrFL(phy_link_06);
			}
		}
	}
#endif
	return RET_OK;
}

int HDMI_M14B0_Get_Aksv_Data(int port_num, UINT8 *pAksv_Data)
{
	if (pAksv_Data == NULL)
		return RET_ERROR;

	switch(port_num)
	{
		case 0:
			LINK_M14_RdFL(hdcp_02);
			LINK_M14_Rd01(hdcp_02, reg_hdcp_aksv_0_prt0, pAksv_Data[0]);
			LINK_M14_RdFL(hdcp_03);
			LINK_M14_Rd01(hdcp_03, reg_hdcp_aksv_1_prt0, pAksv_Data[1]);
			LINK_M14_Rd01(hdcp_03, reg_hdcp_aksv_2_prt0, pAksv_Data[2]);
			LINK_M14_Rd01(hdcp_03, reg_hdcp_aksv_3_prt0, pAksv_Data[3]);
			LINK_M14_Rd01(hdcp_03, reg_hdcp_aksv_4_prt0, pAksv_Data[4]);
			break;
		case 1:
			LINK_M14_RdFL(hdcp_08);
			LINK_M14_Rd01(hdcp_08, reg_hdcp_aksv_0_prt1, pAksv_Data[0]);
			LINK_M14_RdFL(hdcp_09);
			LINK_M14_Rd01(hdcp_09, reg_hdcp_aksv_1_prt1, pAksv_Data[1]);
			LINK_M14_Rd01(hdcp_09, reg_hdcp_aksv_2_prt1, pAksv_Data[2]);
			LINK_M14_Rd01(hdcp_09, reg_hdcp_aksv_3_prt1, pAksv_Data[3]);
			LINK_M14_Rd01(hdcp_09, reg_hdcp_aksv_4_prt1, pAksv_Data[4]);
			break;
		case 2:
			LINK_M14_RdFL(hdcp_14);
			LINK_M14_Rd01(hdcp_14, reg_hdcp_aksv_0_prt2, pAksv_Data[0]);
			LINK_M14_RdFL(hdcp_15);
			LINK_M14_Rd01(hdcp_15, reg_hdcp_aksv_1_prt2, pAksv_Data[1]);
			LINK_M14_Rd01(hdcp_15, reg_hdcp_aksv_2_prt2, pAksv_Data[2]);
			LINK_M14_Rd01(hdcp_15, reg_hdcp_aksv_3_prt2, pAksv_Data[3]);
			LINK_M14_Rd01(hdcp_15, reg_hdcp_aksv_4_prt2, pAksv_Data[4]);
			break;
		case 3:
			LINK_M14_RdFL(hdcp_20);
			LINK_M14_Rd01(hdcp_20, reg_hdcp_aksv_0_prt3, pAksv_Data[0]);
			LINK_M14_RdFL(hdcp_21);
			LINK_M14_Rd01(hdcp_21, reg_hdcp_aksv_1_prt3, pAksv_Data[1]);
			LINK_M14_Rd01(hdcp_21, reg_hdcp_aksv_2_prt3, pAksv_Data[2]);
			LINK_M14_Rd01(hdcp_21, reg_hdcp_aksv_3_prt3, pAksv_Data[3]);
			LINK_M14_Rd01(hdcp_21, reg_hdcp_aksv_4_prt3, pAksv_Data[4]);
			break;
		default :
			return RET_ERROR;
			break;

	}

	return RET_OK;
}

/**
 * @brief Check if AKSV Data Received
 *
 * @param port_num
 *
 * @return 0: AKSV is Zero (Not Received), 1: AKSV is Received
 */
static int _HDMI_M14B0_Check_Aksv_Exist(int port_num)
{
	UINT8 Aksv_Data[5];

	switch(port_num)
	{
		case 0:
			LINK_M14_RdFL(hdcp_02);
			LINK_M14_Rd01(hdcp_02, reg_hdcp_aksv_0_prt0, Aksv_Data[0]);
			LINK_M14_RdFL(hdcp_03);
			LINK_M14_Rd01(hdcp_03, reg_hdcp_aksv_1_prt0, Aksv_Data[1]);
			LINK_M14_Rd01(hdcp_03, reg_hdcp_aksv_2_prt0, Aksv_Data[2]);
			LINK_M14_Rd01(hdcp_03, reg_hdcp_aksv_3_prt0, Aksv_Data[3]);
			LINK_M14_Rd01(hdcp_03, reg_hdcp_aksv_4_prt0, Aksv_Data[4]);
			break;
		case 1:
			LINK_M14_RdFL(hdcp_08);
			LINK_M14_Rd01(hdcp_08, reg_hdcp_aksv_0_prt1, Aksv_Data[0]);
			LINK_M14_RdFL(hdcp_09);
			LINK_M14_Rd01(hdcp_09, reg_hdcp_aksv_1_prt1, Aksv_Data[1]);
			LINK_M14_Rd01(hdcp_09, reg_hdcp_aksv_2_prt1, Aksv_Data[2]);
			LINK_M14_Rd01(hdcp_09, reg_hdcp_aksv_3_prt1, Aksv_Data[3]);
			LINK_M14_Rd01(hdcp_09, reg_hdcp_aksv_4_prt1, Aksv_Data[4]);
			break;
		case 2:
			LINK_M14_RdFL(hdcp_14);
			LINK_M14_Rd01(hdcp_14, reg_hdcp_aksv_0_prt2, Aksv_Data[0]);
			LINK_M14_RdFL(hdcp_15);
			LINK_M14_Rd01(hdcp_15, reg_hdcp_aksv_1_prt2, Aksv_Data[1]);
			LINK_M14_Rd01(hdcp_15, reg_hdcp_aksv_2_prt2, Aksv_Data[2]);
			LINK_M14_Rd01(hdcp_15, reg_hdcp_aksv_3_prt2, Aksv_Data[3]);
			LINK_M14_Rd01(hdcp_15, reg_hdcp_aksv_4_prt2, Aksv_Data[4]);
			break;
		case 3:
			LINK_M14_RdFL(hdcp_20);
			LINK_M14_Rd01(hdcp_20, reg_hdcp_aksv_0_prt3, Aksv_Data[0]);
			LINK_M14_RdFL(hdcp_21);
			LINK_M14_Rd01(hdcp_21, reg_hdcp_aksv_1_prt3, Aksv_Data[1]);
			LINK_M14_Rd01(hdcp_21, reg_hdcp_aksv_2_prt3, Aksv_Data[2]);
			LINK_M14_Rd01(hdcp_21, reg_hdcp_aksv_3_prt3, Aksv_Data[3]);
			LINK_M14_Rd01(hdcp_21, reg_hdcp_aksv_4_prt3, Aksv_Data[4]);
			break;
		default :
			return RET_ERROR;
			break;

	}

	if (memcmp(Aksv_Data, HDCP_AKSV_Data_Zero, sizeof(HDCP_AKSV_Data_Zero) ) )
	{
		return 1;
	}
	else
	{
		return 0;	// equal means AKSV not received
	}
}

static int _HDMI_M14B0_Get_AN_Data(int port_num, UINT8 *AN_Data)
{
	if (AN_Data == NULL)
		return RET_ERROR;

	switch(port_num)
	{
		case 0:
			LINK_M14_RdFL(hdcp_00);
			LINK_M14_Rd01(hdcp_00, reg_hdcp_an_0_prt0, AN_Data[0]);
			LINK_M14_RdFL(hdcp_01);
			LINK_M14_Rd01(hdcp_01, reg_hdcp_an_4_prt0, AN_Data[4]);
			LINK_M14_Rd01(hdcp_01, reg_hdcp_an_3_prt0, AN_Data[3]);
			LINK_M14_Rd01(hdcp_01, reg_hdcp_an_2_prt0, AN_Data[2]);
			LINK_M14_Rd01(hdcp_01, reg_hdcp_an_1_prt0, AN_Data[1]);
			LINK_M14_RdFL(hdcp_02);
			LINK_M14_Rd01(hdcp_02, reg_hdcp_an_7_prt0, AN_Data[7]);
			LINK_M14_Rd01(hdcp_02, reg_hdcp_an_6_prt0, AN_Data[6]);
			LINK_M14_Rd01(hdcp_02, reg_hdcp_an_5_prt0, AN_Data[5]);
			break;
		case 1:
			LINK_M14_RdFL(hdcp_06);
			LINK_M14_Rd01(hdcp_06, reg_hdcp_an_0_prt1, AN_Data[0]);
			LINK_M14_RdFL(hdcp_07);
			LINK_M14_Rd01(hdcp_07, reg_hdcp_an_4_prt1, AN_Data[4]);
			LINK_M14_Rd01(hdcp_07, reg_hdcp_an_3_prt1, AN_Data[3]);
			LINK_M14_Rd01(hdcp_07, reg_hdcp_an_2_prt1, AN_Data[2]);
			LINK_M14_Rd01(hdcp_07, reg_hdcp_an_1_prt1, AN_Data[1]);
			LINK_M14_RdFL(hdcp_08);
			LINK_M14_Rd01(hdcp_08, reg_hdcp_an_7_prt1, AN_Data[7]);
			LINK_M14_Rd01(hdcp_08, reg_hdcp_an_6_prt1, AN_Data[6]);
			LINK_M14_Rd01(hdcp_08, reg_hdcp_an_5_prt1, AN_Data[5]);
			break;
		case 2:
			LINK_M14_RdFL(hdcp_12);
			LINK_M14_Rd01(hdcp_12, reg_hdcp_an_0_prt2, AN_Data[0]);
			LINK_M14_RdFL(hdcp_13);
			LINK_M14_Rd01(hdcp_13, reg_hdcp_an_4_prt2, AN_Data[4]);
			LINK_M14_Rd01(hdcp_13, reg_hdcp_an_3_prt2, AN_Data[3]);
			LINK_M14_Rd01(hdcp_13, reg_hdcp_an_2_prt2, AN_Data[2]);
			LINK_M14_Rd01(hdcp_13, reg_hdcp_an_1_prt2, AN_Data[1]);
			LINK_M14_RdFL(hdcp_14);
			LINK_M14_Rd01(hdcp_14, reg_hdcp_an_7_prt2, AN_Data[7]);
			LINK_M14_Rd01(hdcp_14, reg_hdcp_an_6_prt2, AN_Data[6]);
			LINK_M14_Rd01(hdcp_14, reg_hdcp_an_5_prt2, AN_Data[5]);
			break;
		case 3:
			LINK_M14_RdFL(hdcp_18);
			LINK_M14_Rd01(hdcp_18, reg_hdcp_an_0_prt3, AN_Data[0]);
			LINK_M14_RdFL(hdcp_19);
			LINK_M14_Rd01(hdcp_19, reg_hdcp_an_4_prt3, AN_Data[4]);
			LINK_M14_Rd01(hdcp_19, reg_hdcp_an_3_prt3, AN_Data[3]);
			LINK_M14_Rd01(hdcp_19, reg_hdcp_an_2_prt3, AN_Data[2]);
			LINK_M14_Rd01(hdcp_19, reg_hdcp_an_1_prt3, AN_Data[1]);
			LINK_M14_RdFL(hdcp_20);
			LINK_M14_Rd01(hdcp_20, reg_hdcp_an_7_prt3, AN_Data[7]);
			LINK_M14_Rd01(hdcp_20, reg_hdcp_an_6_prt3, AN_Data[6]);
			LINK_M14_Rd01(hdcp_20, reg_hdcp_an_5_prt3, AN_Data[5]);
			break;
		default :
			return RET_ERROR;
			break;

	}

	return RET_OK;
}

/**
 * @brief Phy Enable/Disable Selected port
 *
 * @param prt_num : port number
 * @param enable : 1:Enable, 0:Disable port
 *
 * @return
 */
static int _HDMI_M14B0_EnablePort(int prt_num, int enable)
{
	/*
	if (enable)
		HDMI_DEBUG("%s : port [%d] : Enable[%d]\n",__func__ ,prt_num, enable);
		*/

	if (prt_num == 0)
	{
			LINK_M14_RdFL(phy_link_00);
			LINK_M14_Wr01(phy_link_00, phy_enable_prt0, enable);		//PHY Enable
			LINK_M14_WrFL(phy_link_00);
	}
	else if (prt_num == 1)
	{
			LINK_M14_RdFL(phy_link_02);
			LINK_M14_Wr01(phy_link_02, phy_enable_prt1, enable);		//PHY Enable
			LINK_M14_WrFL(phy_link_02);
	}
	else if (prt_num == 2)
	{
			LINK_M14_RdFL(phy_link_04);
			LINK_M14_Wr01(phy_link_04, phy_enable_prt2, enable);		//PHY Enable
			LINK_M14_WrFL(phy_link_04);
	}
	else if (prt_num == 3)
	{
			LINK_M14_RdFL(phy_link_06);
			LINK_M14_Wr01(phy_link_06, phy_enable_prt3, enable);		//PHY Enable
			LINK_M14_WrFL(phy_link_06);
	}
	else
		return RET_ERROR;
	return RET_OK;
}

#ifdef NOT_USED_NOW
static int _HDMI_M14B0_GetTMDSClock(int prt_selected, UINT32 *pTmds_clock)
{
	return RET_OK;
}
#endif	//#ifdef NOT_USED_NOW

int HDMI_M14B0_MHL_Send_RCP(UINT8 rcp_data)
{
	int buffer_size;
	int buffer_full_release_code = 0;
	int buffer_full_press_code = 0;
	int stored_rcp_code = 0;
	ULONG	flags = 0;

	HDMI_DEBUG("[%s] Entered with [0x%x] \n",__func__, rcp_data);

	spin_lock_irqsave(&_gIntrHdmiM14B0VideoLock, flags);

	if ( (_g_rcp_send_buffer == 0) && (rcp_data & 0x80) )
	{
		buffer_full_release_code = 1;
	}
	else
	{
		if(_g_rcp_send_buffer)
			_g_rcp_send_buffer --;
		else
			buffer_full_press_code = 1;

		LINK_M14_RdFL(cbus_10);
		LINK_M14_Rd01(cbus_10, reg_man_msg_rcp_kcode, stored_rcp_code);
		LINK_M14_Wr01(cbus_10, reg_man_msg_rcp_kcode, rcp_data);
		LINK_M14_WrFL(cbus_10);

		LINK_M14_RdFL(cbus_11);
		LINK_M14_Wr01(cbus_11, reg_man_msg_rcp_cmd, 0);
		LINK_M14_WrFL(cbus_11);

		LINK_M14_RdFL(cbus_11);
		LINK_M14_Wr01(cbus_11, reg_man_msg_rcp_cmd, 1);	// Edge Trigger write
		LINK_M14_WrFL(cbus_11);
	}

	buffer_size = _g_rcp_send_buffer;

	spin_unlock_irqrestore(&_gIntrHdmiM14B0VideoLock, flags);

	if(buffer_full_release_code)
		HDMI_DEBUG("!!!!! RCP : Release code NOT Sent [buffer_full]");
	else if(buffer_full_press_code)
		HDMI_DEBUG("!!!!! RCP : OVERWRITE RCP code [0x%x]=>[0x%x]\n", stored_rcp_code, rcp_data);
	else
		HDMI_DEBUG("~~~~~ RCP : Buffer Size [%d]\n", buffer_size);

	return RET_OK;
}

int HDMI_M14B0_MHL_Send_WriteBurst(LX_HDMI_MHL_WRITEBURST_DATA_T *spWriteburst_data)
{
	HDMI_DEBUG("[%s] Entered \n",__func__);

	return RET_OK;
}

int HDMI_M14B0_MHL_Read_WriteBurst(LX_HDMI_MHL_WRITEBURST_DATA_T *spWriteburst_data)
{
	HDMI_DEBUG("[%s] Entered \n",__func__);

	return RET_OK;
}

int HDMI_M14B0_Module_Call_Type(LX_HDMI_CALL_TYPE_T	hdmi_call_type)
{
	HDMI_DEBUG("[%s] Entered with [0x%x] \n",__func__, hdmi_call_type);

	OS_LockMutex(&g_HDMI_Sema);

	switch(hdmi_call_type)
	{
		case HDMI_CALL_TYPE_INIT :
			_gM14B0HDMIPhyInform.module_init = 1;
			break;
		case HDMI_CALL_TYPE_UNINIT :
			_gM14B0HDMIPhyInform.module_init = 0;
			break;
		case HDMI_CALL_TYPE_OPEN :
			_gM14B0HDMIPhyInform.module_open = 1;


			/* If hpd enable is not called until module open */
			/* Following code should be removed when HPD Enable HAL is called properly */
	//		_gM14B0HDMIPhyInform.hpd_enable = 1;

			break;
		case HDMI_CALL_TYPE_CLOSE :
			_gM14B0HDMIPhyInform.module_open = 0;

			/* For Fast Port Switching, Do Nothing when module closed */
/*
			_HDMI_M14B0_PhyOff(0);
			_HDMI_M14B0_PhyOff(1);
			_HDMI_M14B0_PhyOff(2);
			_HDMI_M14B0_PhyOff(3);

			_HDMI_M14B0_Set_HPD_Out_A4P(0,0,0,0);
			*/

			break;
		case HDMI_CALL_TYPE_CONN :
			_gM14B0HDMIPhyInform.module_conn = 1;
			break;
		case HDMI_CALL_TYPE_DISCONN :
			_gM14B0HDMIPhyInform.module_conn = 0;
			break;
		default :
			return RET_ERROR;
	}

	OS_UnlockMutex(&g_HDMI_Sema);

	return RET_OK;
}

/**
* @brief Disable internal EDID, in case of using external DDC EEPROM.
*
* @param port_num
* @param edid_resetn
*
* @return
*/
int HDMI_M14B0_Reset_Internal_Edid(int port_num, int edid_resetn)
{
	int ret = RET_OK;

	if (port_num >= 4)
		return RET_ERROR;

	HDMI_DEBUG("[%s] Entered with port[%d] resetn[%d]\n",__func__, port_num, edid_resetn);

	OS_LockMutex(&g_HDMI_Sema);

	if (_gM14B0HDMIPhyInform.hpd_pol[port_num] != edid_resetn)
		HDMI_NOTI("HPD Polarity of port[%d] Changed [%d]=>[%d] \n", port_num, _gM14B0HDMIPhyInform.hpd_pol[port_num], edid_resetn);

	switch(port_num)
	{
		case 0:
			LINK_M14_RdFL(phy_link_00);
			LINK_M14_Wr01(phy_link_00, link_sw_rstn_edid_prt0, edid_resetn);
			LINK_M14_WrFL(phy_link_00);
			_gM14B0HDMIPhyInform.hpd_pol[0] = edid_resetn;
			_HDMI_M14B0_Set_HPD_Out(0, !edid_resetn);	// disable HPD
			break;
		case 1:
			LINK_M14_RdFL(phy_link_02);
			LINK_M14_Wr01(phy_link_02, link_sw_rstn_edid_prt1, edid_resetn);
			LINK_M14_WrFL(phy_link_02);
			_gM14B0HDMIPhyInform.hpd_pol[1] = edid_resetn;
			_HDMI_M14B0_Set_HPD_Out(1, !edid_resetn);	// disable HPD
			break;
		case 2:
			LINK_M14_RdFL(phy_link_04);
			LINK_M14_Wr01(phy_link_04, link_sw_rstn_edid_prt2, edid_resetn);
			LINK_M14_WrFL(phy_link_04);
			_gM14B0HDMIPhyInform.hpd_pol[2] = edid_resetn;
			_HDMI_M14B0_Set_HPD_Out(2, !edid_resetn);	// disable HPD
			break;
		case 3:
			LINK_M14_RdFL(phy_link_06);
			LINK_M14_Wr01(phy_link_06, link_sw_rstn_edid_prt3, edid_resetn);
			LINK_M14_RdFL(phy_link_06);
			_gM14B0HDMIPhyInform.hpd_pol[3] = edid_resetn;
			_HDMI_M14B0_Set_HPD_Out(3, !edid_resetn);	// disable HPD
			break;
		default :
			ret = RET_ERROR;
			break;
	}

	OS_UnlockMutex(&g_HDMI_Sema);

	return ret;
}

/**
* @brief CTOP Register Setting for External DDC EDID Write
*
* @param port_num
* @param enable
*
* @return
*/
int HDMI_M14B0_Enable_External_DDC_Access(int port_num, int enable)
{
	int ret = RET_OK;
	int ctop_tmp;

	HDMI_PRINT("[%s] Entered with port[%d] enable[%d]\n",__func__, port_num, enable);

	CTOP_CTRL_M14B0_RdFL(RIGHT, ctr130);
	CTOP_CTRL_M14B0_Wr01(RIGHT, ctr130, ddc_write_enb, enable);
	CTOP_CTRL_M14B0_WrFL(RIGHT, ctr130);

	if (enable)
	{
		if ( ( port_num >= 0 ) && ( port_num < 4 ) )
		{
			CTOP_CTRL_M14B0_RdFL(RIGHT, ctr130);
			ctop_tmp = CTOP_CTRL_M14B0_Rd(RIGHT, ctr130);
			ctop_tmp = ( ctop_tmp & 0xCFFFFFFF ) | ( port_num << 28);
			CTOP_CTRL_M14B0_Wr(RIGHT, ctr130, ctop_tmp);
			CTOP_CTRL_M14B0_WrFL(RIGHT, ctr130);
		}
		else
			ret = RET_ERROR;
	}

	return ret;
}

/**
* @brief get MHL/HDMI auto/manual mode
*
* @return man_mhl_mode
*/
static int _HDMI_M14B0_Get_ManMHLMode(void)
{
	int ret;

	LINK_M14_RdFL(phy_link_06);
	LINK_M14_Rd01(phy_link_06, reg_man_mhl_mode, ret);

	return ret;
}
/**
* @brief set MHL/HDMI mode by manual
*
* @param man_mhl_mode : 0:auto, 1:Manual
* @param man_mhl_value : 0:HDMI, 1:MHL
*
* @return
*/
static int _HDMI_M14B0_Set_ManMHLMode(int man_mhl_mode, int man_mhl_value)
{
	int ret = RET_OK;

	LINK_M14_RdFL(phy_link_06);
	LINK_M14_Wr01(phy_link_06, reg_man_mhl_mode, man_mhl_mode);
	LINK_M14_Wr01(phy_link_06, reg_man_mhl_value, man_mhl_value);
	LINK_M14_WrFL(phy_link_06);

	return ret;
}

static int _HDMI_M14B0_MHL_Reconnect_Thread(void *data)
{
	int cbus_st_count, state_sink_clr_hpd, state_sink_set_hpd;

	HDMI_DEBUG("---- MHL Reconnect Thread ---- \n");

	while(1)
	{
		interruptible_sleep_on(&WaitQueue_MHL_M14B0);

		HDMI_NOTI("---- !!!!!!!!!!!! MHL TMDS Unstable !!!!!!!!!!!!!!!!!!!! \n");

		LINK_M14_RdFL(cbus_04);
		LINK_M14_Wr01(cbus_04, reg_man_msc_clr_hpd, 0x1);
		LINK_M14_Wr01(cbus_04, reg_man_msc_set_hpd, 0x0);
		LINK_M14_WrFL(cbus_04);

		for (cbus_st_count = 0; cbus_st_count < 10; cbus_st_count++)
		{
			OS_MsecSleep(3000);	// ms delay

			LINK_M14_RdFL(cbus_06);
			LINK_M14_Rd01(cbus_06, reg_state_sink_clr_hpd, state_sink_clr_hpd);

			if (state_sink_clr_hpd > 0)
				break;
		}
		HDMI_NOTI("---- MHL CBUS_ST : state_sink_clr_hpd [%d] \n", state_sink_clr_hpd);

		LINK_M14_RdFL(cbus_04);
		LINK_M14_Wr01(cbus_04, reg_man_msc_clr_hpd, 0x0);
		LINK_M14_Wr01(cbus_04, reg_man_msc_set_hpd, 0x1);
		LINK_M14_WrFL(cbus_04);

		for (cbus_st_count = 0; cbus_st_count < 10; cbus_st_count++)
		{
			OS_MsecSleep(10);	// ms delay

			LINK_M14_RdFL(cbus_05);
			LINK_M14_Rd01(cbus_05, reg_state_sink_set_hpd, state_sink_set_hpd);

			if (state_sink_set_hpd > 0)
				break;
		}
		HDMI_NOTI("---- MHL CBUS_ST : state_sink_set_hpd [%d] \n", state_sink_set_hpd);

		LINK_M14_RdFL(cbus_04);
		LINK_M14_Wr01(cbus_04, reg_man_msc_clr_hpd, 0x0);
		LINK_M14_Wr01(cbus_04, reg_man_msc_set_hpd, 0x0);
		LINK_M14_WrFL(cbus_04);


	}

	return RET_OK;
}

static int _HDMI_M14B0_MHL_PDB_Thread(void *data)
{

	int phy_en_prt0, phy_en_prt1, phy_en_prt2, phy_en_prt3;
	int phy_pdb_prt3, phy_rstn_prt3;

	HDMI_DEBUG("---- MHL PDB Thread ---- \n");

	while(1)
	{
		interruptible_sleep_on(&WaitQueue_MHL_PDB_M14B0);

		LINK_M14_RdFL(phy_link_06);
		LINK_M14_Rd01(phy_link_06, phy_pdb_prt3, phy_pdb_prt3);			//PHY PDB ON
		LINK_M14_Rd01(phy_link_06, phy_rstn_prt3, phy_rstn_prt3);			//PHY RESET

		if (phy_pdb_prt3 && phy_rstn_prt3)
		{

			LINK_M14_RdFL(phy_link_06);
			LINK_M14_Rd01(phy_link_06, phy_enable_prt3, phy_en_prt3);		//PHY Enable

			if (!phy_en_prt3)	// IF port3(C-BUS) is not enabled?
			{
				LINK_M14_RdFL(phy_link_00);
				LINK_M14_Rd01(phy_link_00, phy_enable_prt0, phy_en_prt0);		//PHY Enable

				LINK_M14_RdFL(phy_link_02);
				LINK_M14_Rd01(phy_link_02, phy_enable_prt1, phy_en_prt1);		//PHY Enable

				LINK_M14_RdFL(phy_link_04);
				LINK_M14_Rd01(phy_link_04, phy_enable_prt2, phy_en_prt2);		//PHY Enable

				_HDMI_M14B0_EnablePort(0, 0);
				_HDMI_M14B0_EnablePort(1, 0);
				_HDMI_M14B0_EnablePort(2, 0);
				_HDMI_M14B0_EnablePort(3, 1);
			}

			/*
			   HDMI_DEBUG("---- MHL warb done:0 ,Set ODT PDB Mode to 0x10\n");
			   PHY_REG_M14B0_RdFL(eq_i2c_odt_pdb_mode);
			   PHY_REG_M14B0_Wr01(eq_i2c_odt_pdb_mode,eq_i2c_odt_pdb,0x0);
			   PHY_REG_M14B0_Wr01(eq_i2c_odt_pdb_mode,eq_i2c_odt_pdb_mode,0x1);
			   PHY_REG_M14B0_WrFL(eq_i2c_odt_pdb_mode);
			 */

			PHY_REG_M14B0_RdFL(pdb_d0_man_sel);
			PHY_REG_M14B0_Wr01(pdb_d0_man_sel,pdb_d0_man_sel,0x1);
			PHY_REG_M14B0_Wr01(pdb_d0_man_sel,pdb_d0_man,0x0);
			PHY_REG_M14B0_WrFL(pdb_d0_man_sel);

			PHY_REG_M14B0_RdFL(pdb_dck_man_sel);
			PHY_REG_M14B0_Wr01(pdb_dck_man_sel,pdb_dck_man_sel,0x1);
			PHY_REG_M14B0_Wr01(pdb_dck_man_sel,pdb_dck_man,0x0);
			PHY_REG_M14B0_WrFL(pdb_dck_man_sel);

			if (!phy_en_prt3)
			{
				_HDMI_M14B0_EnablePort(0, phy_en_prt0);
				_HDMI_M14B0_EnablePort(1, phy_en_prt1);
				_HDMI_M14B0_EnablePort(2, phy_en_prt2);
				_HDMI_M14B0_EnablePort(3, phy_en_prt3);
			}
		}

		HDMI_DEBUG("---- MHL disconn, PDB_D0_MAN_SEL :0x10, PDB_DCK_MAN_SEL :0x10\n");
	}

	return RET_OK;
}

static int _HDMI_M14B0_Disable_TMDS_Error_Interrupt(void)
{
	LINK_M14_RdFL(interrupt_enable_01);
//	LINK_M14_Wr01(interrupt_enable_01, intr_hdcp_err_enable, 0x0);				///< 16 intr_hdcp_err_enable
	LINK_M14_Wr01(interrupt_enable_01, intr_ecc_err_enable, 0x0);				///< 17 intr_ecc_err_enable
//	LINK_M14_Wr01(interrupt_enable_01, intr_terc4_err_prt3_enable, 0x0);		///< 18 intr_terc4_err_prt3_enable
//	LINK_M14_Wr01(interrupt_enable_01, intr_terc4_err_prt2_enable, 0x0);		///< 19 intr_terc4_err_prt2_enable
//	LINK_M14_Wr01(interrupt_enable_01, intr_terc4_err_prt1_enable, 0x0);		///< 20 intr_terc4_err_prt1_enable
//	LINK_M14_Wr01(interrupt_enable_01, intr_terc4_err_prt0_enable, 0x0);		///< 21 intr_terc4_err_prt0_enable
	LINK_M14_WrFL(interrupt_enable_01);

	return RET_OK;
}

static int _HDMI_M14B0_Enable_TMDS_Error_Interrupt(void)
{
	LINK_M14_RdFL(interrupt_enable_01);
//	LINK_M14_Wr01(interrupt_enable_01, intr_hdcp_err_enable, 0x0);				///< 16 intr_hdcp_err_enable
	LINK_M14_Wr01(interrupt_enable_01, intr_ecc_err_enable, 0x1);				///< 17 intr_ecc_err_enable
//	LINK_M14_Wr01(interrupt_enable_01, intr_terc4_err_prt3_enable, 0x0);		///< 18 intr_terc4_err_prt3_enable
//	LINK_M14_Wr01(interrupt_enable_01, intr_terc4_err_prt2_enable, 0x0);		///< 19 intr_terc4_err_prt2_enable
//	LINK_M14_Wr01(interrupt_enable_01, intr_terc4_err_prt1_enable, 0x0);		///< 20 intr_terc4_err_prt1_enable
//	LINK_M14_Wr01(interrupt_enable_01, intr_terc4_err_prt0_enable, 0x0);		///< 21 intr_terc4_err_prt0_enable
	LINK_M14_WrFL(interrupt_enable_01);

	return RET_OK;
}

int HDMI_M14B0_MHL_Receive_RCP(LX_HDMI_RCP_RECEIVE_T *sp_MHL_RCP_rcv_msg)
{
	if (sp_MHL_RCP_rcv_msg->rcp_receive_cmd == HDMI_RCP_RECEIVE_CMD_CHECK_RCP)
	{
		sp_MHL_RCP_rcv_msg->rcp_receive_flag = _gMHL_RCP_RCV_MSG.rcp_receive_flag;
	}
	else if (sp_MHL_RCP_rcv_msg->rcp_receive_cmd == HDMI_RCP_RECEIVE_CMD_CLEAR)
	{
		if (_gMHL_RCP_RCV_MSG.rcp_receive_flag)
		{
			_gMHL_RCP_RCV_MSG.rcp_receive_cmd = HDMI_RCP_RECEIVE_CMD_NONE;
			_gMHL_RCP_RCV_MSG.rcp_receive_flag = FALSE;
			_gMHL_RCP_RCV_MSG.rcp_msg = 0x7F;
		}
		else
		{
			HDMI_WARN("[%s] No RCP msg to clear !!! \n",__func__);
			_gMHL_RCP_RCV_MSG.rcp_receive_cmd = HDMI_RCP_RECEIVE_CMD_NONE;
			_gMHL_RCP_RCV_MSG.rcp_msg = 0x7F;
		}
	}
	else if (sp_MHL_RCP_rcv_msg->rcp_receive_cmd == HDMI_RCP_RECEIVE_CMD_GET_MSG)
	{
		if (_gMHL_RCP_RCV_MSG.rcp_receive_flag)
		{
			sp_MHL_RCP_rcv_msg->rcp_msg = _gMHL_RCP_RCV_MSG.rcp_msg;
			HDMI_DEBUG("[%s] received msg [0x%x] \n",__func__, sp_MHL_RCP_rcv_msg->rcp_msg);

			/* RCP Reg Auto Clear after read */
			_gMHL_RCP_RCV_MSG.rcp_receive_cmd = HDMI_RCP_RECEIVE_CMD_NONE;
			_gMHL_RCP_RCV_MSG.rcp_receive_flag = FALSE;
			_gMHL_RCP_RCV_MSG.rcp_msg = 0x7F;
		}
		else
		{
			sp_MHL_RCP_rcv_msg->rcp_msg = _gMHL_RCP_RCV_MSG.rcp_msg;
			HDMI_WARN("[%s] No New RCP msg received !!! :return  [0x%x] \n",__func__, sp_MHL_RCP_rcv_msg->rcp_msg);
		}

	}
	else
	{
		HDMI_WARN("[%s] Not a valid cmd !!! [0x%x] \n",__func__, sp_MHL_RCP_rcv_msg->rcp_receive_cmd);
		return RET_ERROR;
	}

	return RET_OK;
}

#ifdef NOT_USED_NOW
/**
* @brief set HDCP Unauth Nosync/Mode Change
*
* @param unauth_nosync
* @param unauth_mode_chg
*
* @return
*/
static int _HDMI_M14B0_Set_HDCP_Unauth(int port_num, int unauth_nosync, int unauth_mode_chg)
{
	int ret = RET_OK;

	switch(port_num)
	{
		case 0:

			LINK_M14_RdFL(hdcp_00);
			LINK_M14_Wr01(hdcp_00, reg_hdcp_unauth_nosync_prt0, unauth_nosync);
			LINK_M14_Wr01(hdcp_00, reg_hdcp_unauth_mode_chg_prt0, unauth_mode_chg);
			LINK_M14_WrFL(hdcp_00);
			break;

		case 1:
			LINK_M14_RdFL(hdcp_06);
			LINK_M14_Wr01(hdcp_06, reg_hdcp_unauth_nosync_prt1, unauth_nosync);
			LINK_M14_Wr01(hdcp_06, reg_hdcp_unauth_mode_chg_prt1, unauth_mode_chg);
			LINK_M14_WrFL(hdcp_06);
			break;

		case 2:
			LINK_M14_RdFL(hdcp_12);
			LINK_M14_Wr01(hdcp_12, reg_hdcp_unauth_nosync_prt2, unauth_nosync);
			LINK_M14_Wr01(hdcp_12, reg_hdcp_unauth_mode_chg_prt2, unauth_mode_chg);
			LINK_M14_WrFL(hdcp_12);
			break;

		case 3:
			LINK_M14_RdFL(hdcp_18);
			LINK_M14_Wr01(hdcp_18, reg_hdcp_unauth_nosync_prt3, unauth_nosync);
			LINK_M14_Wr01(hdcp_18, reg_hdcp_unauth_mode_chg_prt3, unauth_mode_chg);
			LINK_M14_WrFL(hdcp_18);
			break;

		default :
			ret = RET_ERROR;
			break;
	}
	return ret;
}
#endif	//#ifdef NOT_USED_NOW

static int _HDMI_M14B0_Phy_Reset(int port_num)
{
	int ret = RET_OK;

	switch(port_num)
	{
		case 0:
			LINK_M14_RdFL(phy_link_00);
			LINK_M14_Wr01(phy_link_00, phy_pdb_prt0, 0x0);			//PHY PDB ON
			LINK_M14_Wr01(phy_link_00, phy_rstn_prt0, 0x0);			//PHY RESET
			LINK_M14_WrFL(phy_link_00);
			break;

		case 1:
			LINK_M14_RdFL(phy_link_02);
			LINK_M14_Wr01(phy_link_02, phy_pdb_prt1, 0x0);			//PHY PDB ON
			LINK_M14_Wr01(phy_link_02, phy_rstn_prt1, 0x0);			//PHY RESET
			LINK_M14_WrFL(phy_link_02);
			break;

		case 2:
			LINK_M14_RdFL(phy_link_04);
			LINK_M14_Wr01(phy_link_04, phy_pdb_prt2, 0x0);			//PHY PDB ON
			LINK_M14_Wr01(phy_link_04, phy_rstn_prt2, 0x0);			//PHY RESET
			LINK_M14_WrFL(phy_link_04);
			break;

		case 3:
			LINK_M14_RdFL(phy_link_06);
			LINK_M14_Wr01(phy_link_06, phy_pdb_prt3, 0x0);			//PHY PDB ON
			LINK_M14_Wr01(phy_link_06, phy_rstn_prt3, 0x0);			//PHY RESET
			LINK_M14_WrFL(phy_link_06);
			break;

		default :
			ret = RET_ERROR;
			break;

	}

	return ret;
}

static int _HDMI_M14B0_Check_RAP_Content(int clear)
{
	static int prev_content_on_status = 0;
	int content_on_status;
	int ret = RET_OK;

#if 1
	LINK_M14_RdFL(cbus_00);
	LINK_M14_Rd01(cbus_00, reg_cbus_rap_content_on, content_on_status);
#else	// to test RAP Function : JUST TEST ONLY !!!!!
	LINK_M14_RdFL(hdcp_12);
	LINK_M14_Rd01(hdcp_12, reg_hdcp_unauth_mode_chg_prt2, content_on_status);
#endif

	if (clear)
	{
		prev_content_on_status = 0;
		_gM14B0MHLContentOff = 0;
	}
	else if ( (content_on_status != prev_content_on_status) && (_gM14B0HDMIPhyInform.cbus_st > 1) )
	{
		HDMI_DEBUG("---- MHL RAP Contents On CHG: [%d] => [%d] \n", prev_content_on_status, content_on_status);

		if (content_on_status)	// RAP : content ON
		{
			if (_gM14B0MHLContentOff)	// av mute state
			{
				HDMI_DEBUG("---- MHL RAP Set UnMute AV !!! \n");
				_gM14B0MHLContentOff = 0;
			}
			else	// av unmute state
			{
				HDMI_DEBUG("---- MHL RAP AV already unmuted \n");
			}
		}
		else	// RAP : content Off
		{
			if (_gM14B0MHLContentOff)	// av mute state
			{
				HDMI_DEBUG("---- MHL RAP AV already muted \n");
			}
			else	// av unmute state
			{
				HDMI_DEBUG("---- MHL RAP Set Mute AV \n");
				_gM14B0MHLContentOff = 1;
			}
		}
		prev_content_on_status = content_on_status;
	}

	return ret;
}

static int _HDMI_M14B0_MHL_Check_Status(int init)
{
	int state_sink_wrt_stat_pathen_set, seq_st;
	int man_msc_clr_hpd, man_msc_set_hpd;
	static int mhl_status_invalid = 0;

	if (init)
	{
		HDMI_DEBUG("[%s] Entered with [%d] \n",__func__, init);
		LINK_M14_RdFL(cbus_04);
		LINK_M14_Wr(cbus_04, 0x20003100);	//initial CBUS-04 register value
		LINK_M14_WrFL(cbus_04);

		mhl_status_invalid = 0;
	}
	else
	{
		LINK_M14_RdFL(cbus_09);
		LINK_M14_Rd01(cbus_09, reg_state_sink_wrt_stat_pathen_set, state_sink_wrt_stat_pathen_set);
		LINK_M14_RdFL(cbus_05);
		LINK_M14_Rd01(cbus_05, reg_seq_st, seq_st);

		if ( (state_sink_wrt_stat_pathen_set == 1) && (seq_st == 0) )
		{
			HDMI_NOTI("MHL Connected BUT invalid status : reg_state_sink_wrt_stat_pathen_set : [0x%d], reg_seq_st : [0x%x] \n", state_sink_wrt_stat_pathen_set, seq_st);

			LINK_M14_RdFL(cbus_04);
			LINK_M14_Wr01(cbus_04, reg_man_msc_wrt_stat_pathen_set, 0x0);
			LINK_M14_Wr01(cbus_04, reg_man_msc_wrt_stat_dcaprd_set, 0x0);
			LINK_M14_Wr01(cbus_04, reg_man_msc_set_intr_3dreq_set, 0x0);
			LINK_M14_Wr01(cbus_04, reg_man_msc_rd_devcap, 0x0);
			LINK_M14_Wr01(cbus_04, reg_man_msc_get_vdid, 0x0);
			LINK_M14_Wr01(cbus_04, reg_man_msc_get_state, 0x0);
			LINK_M14_Wr01(cbus_04, reg_seq_msc_wrt_stat_dcaprd_set, 0x0);
			LINK_M14_Wr01(cbus_04, reg_seq_msc_wrt_stat_pathen_set, 0x0);
			LINK_M14_Wr01(cbus_04, reg_seq_msc_rd_devcap, 0x0);
			LINK_M14_Wr01(cbus_04, reg_seq_msc_get_vdid, 0x0);
			LINK_M14_Wr01(cbus_04, reg_seq_msc_get_state, 0x0);
			LINK_M14_Wr01(cbus_04, reg_seq_msc_set_hpd, 0x0);

			LINK_M14_Wr01(cbus_04, reg_man_msc_clr_hpd, 0x1);	// 0x40
			LINK_M14_Wr01(cbus_04, reg_man_msc_set_hpd, 0x0);

			LINK_M14_WrFL(cbus_04);

			mhl_status_invalid = 1;
		}
		else if (mhl_status_invalid)
		{
			LINK_M14_RdFL(cbus_04);
			LINK_M14_Rd01(cbus_04, reg_man_msc_clr_hpd, man_msc_clr_hpd);
			LINK_M14_Rd01(cbus_04, reg_man_msc_set_hpd, man_msc_set_hpd);

			if ( (man_msc_clr_hpd == 0x1) && (man_msc_set_hpd == 0) && !_gM14B0HDMIPhyInform.scdt[3])
			{
				HDMI_NOTI("MHL invalid status : SCDT lost\n");
				LINK_M14_RdFL(cbus_04);
				LINK_M14_Wr01(cbus_04, reg_man_msc_clr_hpd, 0x0);	// 0x80
				LINK_M14_Wr01(cbus_04, reg_man_msc_set_hpd, 0x1);
				LINK_M14_WrFL(cbus_04);

				OS_MsecSleep(2);	// ms delay

				LINK_M14_RdFL(cbus_04);
				LINK_M14_Wr01(cbus_04, reg_man_msc_clr_hpd, 0x0);	// 0x00
				LINK_M14_Wr01(cbus_04, reg_man_msc_set_hpd, 0x0);
				LINK_M14_WrFL(cbus_04);

				mhl_status_invalid = 0;
			}
		}

	}


	return RET_OK;
}

int HDMI_M14B0_Get_Driver_Status(LX_HDMI_DRIVER_STATUS_T *spHDMI_Driver_Status)
{
	if (spHDMI_Driver_Status == NULL)
		return RET_ERROR;

	memcpy(&spHDMI_Driver_Status->PrevTiming, &_gM14B0PrevTiming, sizeof(LX_HDMI_TIMING_INFO_T));
	spHDMI_Driver_Status->TimingReadCnt = _gM14B0TimingReadCnt;
	spHDMI_Driver_Status->HDMIState = _gM14B0HDMIState;
	spHDMI_Driver_Status->PrevPixelEncoding = _gM14B0PrevPixelEncoding;
	memcpy(&spHDMI_Driver_Status->PrevAVIPacket, &_gM14B0PrevAVIPacket, sizeof(LX_HDMI_INFO_PACKET_T));
	memcpy(&spHDMI_Driver_Status->PrevVSIPacket, &_gM14B0PrevVSIPacket, sizeof(LX_HDMI_INFO_PACKET_T));
	spHDMI_Driver_Status->AVIReadState = _gM14B0AVIReadState;
	spHDMI_Driver_Status->VSIState = _gM14B0VSIState;
	spHDMI_Driver_Status->AVIChangeState = _gM14B0AVIChangeState;
	spHDMI_Driver_Status->PortSelected = _gM14B0PortSelected;
	spHDMI_Driver_Status->MHLContentOff = _gM14B0MHLContentOff;

	spHDMI_Driver_Status->AudioMuteState = _gM14B0AudioMuteState;
	spHDMI_Driver_Status->AudioArcStatus = _gM14B0AudioArcStatus;
	spHDMI_Driver_Status->IntrAudioState = _gM14B0IntrAudioState;
	spHDMI_Driver_Status->HdmiPortStableCount = _gM14B0HdmiPortStableCount;
	spHDMI_Driver_Status->HdmiFreqErrorCount = _gM14B0HdmiFreqErrorCount;
	spHDMI_Driver_Status->HdmiAudioThreadCount = _gM14B0HdmiAudioThreadCount;
	spHDMI_Driver_Status->IntrBurstInfoCount = _gM14B0IntrBurstInfoCount;
	spHDMI_Driver_Status->TaskBurstInfoCount = _gM14B0TaskBurstInfoCount;
	memcpy(&spHDMI_Driver_Status->HdmiAudioInfo, &_gM14B0HdmiAudioInfo, sizeof(LX_HDMI_AUDIO_INFO_T));

	spHDMI_Driver_Status->Intr_vf_stable = _gM14B0Intr_vf_stable;
	spHDMI_Driver_Status->Intr_avi = _gM14B0Intr_avi;
	spHDMI_Driver_Status->Intr_src_mute = _gM14B0Intr_src_mute;
	spHDMI_Driver_Status->Intr_vsi = _gM14B0Intr_vsi;

	spHDMI_Driver_Status->Force_thread_sleep = _gM14B0Force_thread_sleep;
	spHDMI_Driver_Status->HDMI_thread_running = _gM14B0HDMI_thread_running;
	spHDMI_Driver_Status->HDMI_interrupt_registered = _gM14B0HDMI_interrupt_registered;

	spHDMI_Driver_Status->ChipPlatform = _gM14B0ChipPlatform;

	spHDMI_Driver_Status->CHG_AVI_count_MHL = _gM14B0CHG_AVI_count_MHL;
	spHDMI_Driver_Status->CHG_AVI_count_EQ = _gM14B0CHG_AVI_count_EQ;
	spHDMI_Driver_Status->TMDS_ERROR_EQ = _gM14B0_TMDS_ERROR_EQ;

	return RET_OK;
}

static int _HDMI_M14B0_HDCP_Check_Status(void)
{
	static int hdcp_unauthed_count[4] = {0,};
	int port_num;

	for (port_num = 0; port_num <4; port_num++)
	{
		if ( (_gM14B0HDMIPhyInform.hdcp_authed[port_num] == 0) \
				&& (_HDMI_M14B0_Check_Aksv_Exist(port_num) )
				&& (_gM14B0HDMIPhyInform.scdt[port_num] ) )
		{
			hdcp_unauthed_count[port_num]++;

			if (hdcp_unauthed_count[port_num] > 200)	//for about 4-sec
			{
				HDMI_NOTI("Port [%d] unauthed for long time : reset HDCP !!!!! \n", port_num);

				_HDMI_M14B0_HDCP_ResetPort(port_num);

				hdcp_unauthed_count[port_num] = 0;
			}
		}
		else
			hdcp_unauthed_count[port_num] = 0;

	}

	return RET_OK;
}

static int _HDMI_M14B0_swrst_TMDS_sel_control(int reset)
{
	int ret = RET_OK;
	HDMI_DEBUG("[%s] Entered with [%d]\n",__func__, reset);
	CTOP_CTRL_M14B0_RdFL(DEI, ctr02);
	CTOP_CTRL_M14B0_Wr01(DEI, ctr02, swrst_tmds_sel, reset);
	CTOP_CTRL_M14B0_WrFL(DEI, ctr02);
	return ret;
}


#ifdef	KDRV_CONFIG_PM
int HDMI_M14B0_RunSuspend(void)
{
	int ret = RET_OK;
	return ret;
}

int HDMI_M14B0_RunResume(void)
{
	int ret = RET_OK;

	_HDMI_M14B0_ClearSWResetAll();			//M14D Chip - Ctop control

	_HDMI_M14B0_PhyOff(0);
	_HDMI_M14B0_PhyOff(1);
	_HDMI_M14B0_PhyOff(2);
	_HDMI_M14B0_PhyOff(3);

	OS_MsecSleep(2);	// ms delay

	_HDMI_M14B0_RunReset();		// init Link register

	//audio setting
	_HDMI_M14B0_SetAudio();

	_HDMI_M14B0_EnableInterrupt();

//	HDMI_M14B0_SetPowerControl(0);

	return ret;
}
#endif


