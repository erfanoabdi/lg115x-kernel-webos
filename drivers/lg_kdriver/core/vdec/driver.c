/*
 * SIC LABORATORY, LG ELECTRONICS INC., SEOUL, KOREA
 * Copyright(c) 2013 by LG Electronics Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

/**
 * @file
 *
 *  main driver implementation for vdec device.
 *	vdec device will teach you how to make device driver with new platform.
 *
 *  author		seokjoo.lee (seokjoo.lee@lge.com)
 *  version		1.0
 *  date		2009.12.30
 *  note		Additional information.
 *
 *  @addtogroup lg1152_vdec
 * @{
 */


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/irqreturn.h>
#include <linux/mm.h>
#include <linux/rmap.h>
#include <linux/poll.h>
#include <linux/version.h>
#include <linux/smp.h>
#include <linux/vmalloc.h>
#include <linux/file.h>
#include <linux/platform_device.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>

#if defined(CHIP_NAME_m14) || defined(CHIP_NAME_h13) || defined(CHIP_NAME_h14)
#define SUPPORT_ION
#include <linux/ion.h>
#endif

#if defined(CHIP_NAME_d13) || defined(CHIP_NAME_d14)
#include "../dxipc/dxipc_drv.h"
#endif

#include "hma_alloc.h"

#include "include/vdec_kapi.h"
#include "include/vo_kapi.h"

#include "ves/ves_drv.h"

#include "vdc/vdc_drv.h"

#include "vds/vdisp_drv.h"
#include "vds/disp_q.h"

#include "mcu/ipc_req.h"
#include "mcu/ipc_callback.h"

#include "hal/vdec_base.h"
#include "hal/top_hal_api.h"
#include "hal/ipc_reg_api.h"
#include "hal/av_lipsync_hal_api.h"

#include "decoder.h"
#include "output.h"
#include "proc.h"
#include "hal/hal_proc.h"


#include "log.h"



logm_define (vdec, log_level_warning);
logm_define (vdec_userdata, log_level_warning);

static int vdec_enable_userdata = 1;
module_param (vdec_enable_userdata, int, 0644);

static int vdec_force_raster_memory;
module_param (vdec_force_raster_memory, int, 0644);

static int vdec_default_cpb_size = VDEC_CPB_SIZE;
module_param (vdec_default_cpb_size, int, 0644);

static int vdec_force_frame_alternation;
module_param (vdec_force_frame_alternation, int, 0644);

int vdec_force_framerate_num;
int vdec_force_framerate_den;
module_param (vdec_force_framerate_num, int, 0644);
module_param (vdec_force_framerate_den, int, 0644);

static int vdec_force_progressive;
module_param (vdec_force_progressive, int, 0644);

static int vdec_test_user_alloc;
module_param (vdec_test_user_alloc, int, 0644);

static int vdec_test_copy_mode;
module_param (vdec_test_copy_mode, int, 0644);

static int vdec_test_dish_trick;
module_param (vdec_test_dish_trick, int, 0644);

static int vdec_test_dish_trick_set;
module_param (vdec_test_dish_trick_set, int, 0644);

static int vdec_test_wrong_fr;
module_param (vdec_test_wrong_fr, int, 0644);

static int vdec_test_dual_sync;
module_param (vdec_test_dual_sync, int, 0644);

#if (LINUX_VERSION_CODE < KERNEL_VERSION (3,7,0))
extern struct workqueue_struct *system_wq;
#define vdec_wq	system_wq
#else
extern struct workqueue_struct *system_highpri_wq;
#define vdec_wq	system_highpri_wq
#endif

static vdec_t *device_list;
static DEFINE_SPINLOCK (device_list_lock);

static DEFINE_MUTEX (io_lock);


#define FLAGS_INTER_MASK	0xffff0000
#define FLAGS_INTER_WAIT_EOS	(1<<31)
#define FLAGS_INTER_DUAL_OUT	(1<<30)


/**
 * Allocates Notification Queue.
 *
 * After open() before Enable notification via ioctl ( LX_VDEC_IO_SET_NOTIFY )
 *
 * @param size	[IN] queue size in number of events.
 *
 * @return 1 really allocated, zero for previously allocated. negative when error
 */
static int noti_alloc (noti_t *noti, int size)
{
	vdec_t *vdec = container_of (noti, vdec_t, noti);

	noti->mask = 0;
	noti->size = size;
	noti->head = noti->tail = 0;
	noti->data = vmalloc (size);
	init_waitqueue_head(&noti->wq);

	spin_lock_init(&noti->write_lock);
	mutex_init (&noti->read_lock);

	logm_debug (vdec, "vdec:%p, size %d\n", vdec, size);

	if (noti->data == NULL)
	{
		logm_warning (vdec, "vdec:%p, kmalloc failed\n",
				vdec);
		return -ENOMEM;
	}

	return 0;
}

/**
 * Frees a Notification queue.
 * [NOTE]
 * This should be
 * called AFTER detached from notification list.
 * called unlocked for notification list.
 *
 * @param noti	[IN] pointer to a notification queue to be freed.
 */
// [task context]
static void noti_free (noti_t *noti)
{
	vdec_t *vdec = container_of (noti, vdec_t, noti);

	logm_debug (vdec, "vdec:%p\n", vdec);
	vfree (noti->data);
}

/**
 * save notify paramter
 */
// [interrupt context]
static void noti_save(noti_t *noti, unsigned int id,
		void *data, unsigned int real_size)
{
	vdec_t *vdec = container_of (noti, vdec_t, noti);

	logm_debug (vdec, "vdec:%p, %c%c%c%c, %d bytes, %d %d\n", vdec,
			(id>>0)&0xff,
			(id>>8)&0xff,
			(id>>16)&0xff,
			(id>>24)&0xff,
			real_size, noti->tail, noti->head);

#if !defined(CHIP_NAME_d13) && !defined(CHIP_NAME_d14)
	{
		int filled;
		int head;
		int count1, count2;
		unsigned int size;
		unsigned long flag;

		spin_lock_irqsave (&noti->write_lock, flag);
		head = noti->head;

		/* align 4 bytes */
		size = (real_size+3)&~3;

		/* check buffer overflow */
		filled = head - noti->tail;
		if (filled < 0)
			filled += noti->size;

		if (filled + size + 8 >= noti->size)
		{
			logm_noti (vdec, "vdec:%p, noti queue full %d, %d, "
					"size %d, %c%c%c%c\n",
					vdec, noti->tail, head,
					real_size,
					(id>>0)&0xff,
					(id>>8)&0xff,
					(id>>16)&0xff,
					(id>>24)&0xff
				  );
			goto failed;
		}

		/* put size, size includes id */
		*(unsigned int*)(noti->data + head) = real_size+4;
		head += 4;
		head %= noti->size;

		/* put id */
		*(unsigned int*)(noti->data + head) = id;
		head += 4;
		head %= noti->size;

		/* put data */
		if (head + real_size <= noti->size)
		{
			count1 = real_size;
			count2 = 0;
		}
		else
		{
			count1 = noti->size - head;
			count2 = real_size - count1;
		}
		if (count1)
			memcpy (noti->data+head, data, count1);
		if (count2)
			memcpy (noti->data, data+count1, count2);
		head += size;
		head %= noti->size;
		noti->head = head;

		wake_up (&noti->wq);

failed:
		spin_unlock_irqrestore (&noti->write_lock, flag);
	}
#endif
}


/**
 * copy to user buffer from saved notification queue.
 *
 * @param noti		[IN] channel for search.
 * @param notifyID	[IN] notification ID
 * @return non-negative number of bytes copied, negative when error.
 */
// [task context]
static int noti_copy(noti_t *noti, char __user *buffer, size_t buffer_size)
{
	vdec_t *vdec = container_of (noti, vdec_t, noti);
	int ret;
	int tail;
	int count1, count2;
	unsigned int unit_size;

	mutex_lock (&noti->read_lock);

	tail = noti->tail;
	unit_size = *(unsigned int*)(noti->data+tail);
	tail += 4;
	tail %= noti->size;

	logm_debug (vdec, "vdec:%p, read %d bytes, %d %d\n", vdec,
			unit_size, noti->tail, noti->head);

	if (tail + unit_size <= noti->size)
	{
		count1 = unit_size;
		count2 = 0;
	}
	else
	{
		count1 = noti->size - tail;
		count2 = unit_size - count1;
	}

	if (buffer_size < unit_size)
	{
		logm_warning (vdec, "vdec:%p, too small buffer size %d<%d\n",
				vdec, buffer_size, unit_size);
		ret = -EAGAIN;
		goto failed;
	}

	ret = copy_to_user(buffer, noti->data+tail, count1);
	if (ret)
	{
		logm_warning (vdec, "vdec:%p, oops\n", vdec);
		ret = -EFAULT;
		goto failed;
	}
	if (count2)
	{
		ret = copy_to_user(buffer+count1, noti->data, count2);
		if (ret)
		{
			logm_warning (vdec, "vdec:%p, oops\n", vdec);
			ret = -EFAULT;
			goto failed;
		}
	}

	tail += unit_size+3;
	tail &= ~3;
	tail %= noti->size;
	noti->tail = tail;

	ret = unit_size;

failed:
	mutex_unlock (&noti->read_lock);

	return ret;
}


static void VDEC_CH_Init(void)
{
	// HAL_Init
	TOP_HAL_Init();
	PDEC_HAL_Init();
	IPC_REG_Init();
	AV_LipSync_HAL_Init();

	IPC_REQ_Init();
	IPC_CALLBACK_Init();

	VES_Init();
	VES_AUIB_Init();
	VES_CPB_Init();

	VDC_Init();

	DISP_CLEAR_Q_Init();
}

#if 0
#include <asm/processor.h>

void d13_set_hwbreak (int num, unsigned long addr, int size, int enable)
{
	unsigned long ba;
	unsigned int bc;

	if ((size-1)&size)
	{
		printk ("size is not power of 2. %d\n", size);
		return;
	}
	if (size > 64)
	{
		printk ("too big size %d\n", size);
		return;
	}

	if (enable)
	{
		ba = addr;
		bc = (~(size-1)) & 63;
		bc |= 0xc0000000;

		printk ("set bp%d @%08lx(%d) c%p, b%08lx\n", num, ba, size, &ba, ((unsigned long)&ba)&~((1<<13)-1));
	}
	else
	{
		ba = 0;
		bc = 0;

		//printk ("clear bp%d\n", num);
	}

	switch (num)
	{
	case 0:
		set_sr (ba, 144);
		set_sr (bc, 160);
		break;
	case 1:
		set_sr (ba, 145);
		set_sr (bc, 161);
		break;
	default:
		printk ("not supported bp number %d\n", num);
		return;
	}

#if 0
	printk ("%s.%d: break address %08x, %08x\n",
			__func__, __LINE__,
			get_sr (144), get_sr (145));
	printk ("%s.%d: break control %08x, %08x\n",
			__func__, __LINE__,
			get_sr (160), get_sr (161));
#endif
}
#endif

static int kick_decoder_locked (vdec_t *vdec)
{
	if (vdec->vdc < 0)
	{
		logm_debug (vdec, "vdec:%p, no vdc\n", vdec);
		return 0;
	}

	if (
			vdec->frame_requested < 0 ||
			vdec->frame_requested > vdec->frame_displayed
	   )
	{
		logm_debug (vdec, "vdec:%p, kick decoder\n", vdec);
		VDC_Update (vdec->vdc);
	}
	else
		logm_debug (vdec, "vdec:%p, no more picture needed\n",
				vdec);

	return 0;
}

static void kick_decoder_ws (struct work_struct *work)
{
	unsigned long flag;
	vdec_t *vdec;
	vdec_t *now;

check_again:
	spin_lock_irqsave (&device_list_lock, flag);
	now = device_list;
	vdec = NULL;
	while (now)
	{
		if (now->kick_decoder_count)
		{
			vdec = now;
			logm_debug (vdec, "vdec:%p, kick decoder count %d\n",
					vdec, vdec->kick_decoder_count);
			vdec->kick_decoder_count = 0;
			break;
		}

		now = now->next;
	}
	spin_unlock_irqrestore (&device_list_lock, flag);

	if (vdec == NULL)
		return;

	mutex_lock (&vdec->submodule_lock);
	kick_decoder_locked (vdec);
	mutex_unlock (&vdec->submodule_lock);

	goto check_again;
}

static DECLARE_WORK (kick_decoder_work, kick_decoder_ws);

static int unref_frame (vdec_t *vdec, unsigned long y)
{
	struct decoded_info *now, *pre = NULL, *selected = NULL, *selected_pre = NULL;
	int a;
	int ret;
	unsigned long flag;

	logm_debug (vdec, "vdec:%p, y:%07lx\n", vdec, y);

	if (vdec->vdc < 0)
	{
		logm_noti (vdec, "vdec:%p, unref while closing vdc\n",
				vdec);
		return -EFAULT;
	}

	spin_lock_irqsave (&vdec->decoded_buffers_lock, flag);
	/* search in the decoded buffer list */
	for (a=0,now=vdec->decoded_infos; now; now=now->next,a++)
	{
		if (now->buffer.addr_y == y &&
				now->user > 0)
		{

			logm_debug (vdec, "vdec:%p, ""info index %d, "
					"push index:%x, y:%07lx, user %d\n",
					vdec, a,
					now->buffer.buffer_index,
					y, now->user);

			selected = now;
			selected_pre = pre;
		}

		pre = now;
	}

	/* free it */
	if (selected)
	{
		selected->user --;
		if (selected->user == 0)
		{
			logm_debug (vdec, "vdec:%p, clear selected\n", vdec);

			if (vdec->flags & lx_vdec_flags_copy_dpb)
			{
				if (vdec->flags & lx_vdec_flags_user_dpb)
					logm_warning (vdec, "vdec:%p, Oops\n", vdec);
				else
				{
					struct dpb *dpb, **pre;

					logm_debug (vdec, "vdec:%p, copy dpb mode. free the buffer\n",
							vdec);

					spin_lock_irqsave (&vdec->dpb_lock, flag);
					dpb = vdec->dpb;
					pre = &vdec->dpb;
					while (dpb)
					{
						if (dpb->fb.y == y)
						{
							vdec_free (dpb->fb.y);

							*pre = dpb->next;
							kfree (dpb);
							break;
						}

						pre = &dpb->next;
						dpb = dpb->next;
					}
					spin_unlock_irqrestore (&vdec->dpb_lock, flag);

					if (!dpb)
						logm_warning (vdec, "vdec:%p, Oops\n", vdec);
				}
			}
			else
			{
				logm_debug (vdec, "vdec:%p, "
						"push to clear queue\n",
						vdec);

				DISP_CLEAR_Q_Push_Index (vdec->clear_q,
						selected->buffer.buffer_index);
			}

			if (selected == vdec->decoded_infos)
				vdec->decoded_infos = selected->next;
			else
				selected_pre->next = selected->next;

			kfree (selected);
		}

		ret = 0;
	}
	else
	{
		logm_warning (vdec, "vdec:%p, unknown y address %07lx\n",
				vdec, y);
		ret = -EFAULT;
	}
	spin_unlock_irqrestore (&vdec->decoded_buffers_lock, flag);

	return 0;
}

int unref_frame_and_kick_decoder_locked (vdec_t *vdec, unsigned long y)
{
	int ret;

	ret = unref_frame (vdec, y);
	if (ret < 0)
		return ret;
	return kick_decoder_locked (vdec);
}


static int BUF2buf (vdec_t *vdec, S_DISPQ_BUF_T *BUF, decoded_buffer_t *buf)
{
	memset (buf, 0, sizeof (*buf));

	if (BUF->bDispResult == 0)
		buf->report = DISPLAY_REPORT_DROPPED;

	buf->addr_y = BUF->ui32Y_FrameBaseAddr;
	buf->addr_cb = BUF->ui32C_FrameBaseAddr;
	buf->buffer_index = BUF->ui32FrameIdx;
	buf->framerate_num = BUF->ui32FrameRateRes;
	buf->framerate_den = BUF->ui32FrameRateDiv;
	buf->crop_left = BUF->ui32H_Offset >> 16;
	buf->crop_right = BUF->ui32H_Offset & 0xffff;
	buf->crop_top = BUF->ui32V_Offset >> 16;
	buf->crop_bottom = BUF->ui32V_Offset & 0xffff;
	buf->stride = BUF->ui32Stride;
	buf->width = BUF->ui32PicWidth;
	buf->height = BUF->ui32PicHeight;
	switch (BUF->ui32DisplayInfo)
	{
	default:
		logm_warning (vdec, "vdec:%p, unknown scan type. %d\n", vdec,
				BUF->ui32DisplayInfo);
		// fallthrough
	case DISPQ_SCAN_PROG:
		buf->interlace = decoded_buffer_interlace_none;
		break;
	case DISPQ_SCAN_TFF:
		buf->interlace = decoded_buffer_interlace_top_first;
		break;
	case DISPQ_SCAN_BFF:
		buf->interlace = decoded_buffer_interlace_bottom_first;
		break;
	}
	buf->active_format = BUF->ui8ActiveFormatDescription;
	buf->frame_packing_arrangement = BUF->i32FramePackArrange;
	buf->par_w = BUF->ui16ParW;
	buf->par_h = BUF->ui16ParH;
	switch (BUF->ui32LR_Order)
	{
	default:
		logm_warning (vdec, "vdec:%p, unknown lr order. %d\n", vdec,
				BUF->ui32LR_Order);
		// fallthrough
	case DISPQ_3D_FRAME_NONE:
		buf->multi_picture = decoded_buffer_multi_picture_none;
		break;
	case DISPQ_3D_FRAME_LEFT:
		buf->multi_picture = decoded_buffer_multi_picture_left;
		break;
	case DISPQ_3D_FRAME_RIGHT:
		buf->multi_picture = decoded_buffer_multi_picture_right;
		break;
	}
	buf->uid = BUF->ui32UId;
	buf->pts = BUF->ui32PTS;

	logm_debug (vdec, "vdec:%p, %dx%d(%d,%d,%d,%d) fpa%d\n", vdec,
			buf->width, buf->height,
			buf->crop_left, buf->crop_right,
			buf->crop_top, buf->crop_bottom,
			buf->frame_packing_arrangement);

	return 0;
}


static void unref_frame_fromsync (void *arg, S_DISPQ_BUF_T *BUF)
{
	vdec_t *vdec = (vdec_t *)arg;
	decoded_buffer_t buf;
	unsigned long flag;

	BUF2buf (vdec, BUF, &buf);

	noti_save (&vdec->noti,
			LX_VDEC_READ_MAGIC("DISP"),
			&buf, sizeof (buf));

	if (BUF->bDispResult != 0)
		logm_debug (vdec, "vdec:%p, displayed. uid %08x, pts %08x, fr%d/%d\n",
				vdec, BUF->ui32UId, BUF->ui32PTS,
				BUF->ui32FrameRateRes,
				BUF->ui32FrameRateDiv);
	else
		logm_debug (vdec, "vdec:%p, dropped, fr%d/%d\n", vdec,
				BUF->ui32FrameRateRes,
				BUF->ui32FrameRateDiv);

	spin_lock_irqsave (&vdec->submodule_vdc_spin_lock, flag);
	unref_frame (vdec, BUF->ui32Y_FrameBaseAddr);

	/* kick decoder */
	vdec->kick_decoder_count ++;
	queue_work (vdec_wq, &kick_decoder_work);
	spin_unlock_irqrestore (&vdec->submodule_vdc_spin_lock, flag);
}

static int decoded_queue_flush (decoded_q_t *q)
{
	vdec_t *vdec = container_of (q, vdec_t, decoded_q);

	logm_debug (vdec, "vdec:%p, flushing\n", vdec);

	noti_save (&vdec->noti, LX_VDEC_READ_MAGIC("FLSH"),
			NULL, 0);

	vdec->frame_displayed = 0;

	return 0;
}


struct userdata
{
	int type;
	int size;

	struct userdata *next;

	unsigned char data[];
};

static struct userdata *read_userdata (vdec_t *vdec, decoded_buffer_t *buf)
{
	struct userdata *ret, *pre;

	ret = pre = NULL;

	/* log print */
	if (logm_enabled (vdec_userdata, log_level_trace))
	{
		int a, len;

		len = buf->user_data_size;
		if (len > 256)
			len = 256;

		logm_trace (vdec_userdata, "vdec:%p, user data %p, size %d\n",
				vdec, buf->user_data_addr, buf->user_data_size);
		for (a=0; a<len; )
		{
			char tmp[32*3+1];
			int b;

			for (b=0; b<32 && a+b<len; b++)
				sprintf (tmp+b*3, "%02x ",
						((unsigned char*)buf->user_data_addr)[a+b]);
			tmp[b*3] = 0;;

			logm_trace (vdec_userdata, "%3d: %s\n",
					a, tmp);

			a += b;
		}
	}

	if (buf->user_data_size > 8+8*16)
	{
		unsigned char *p = buf->user_data_addr;
		int segs;
		int off, a;

		segs = p[1] | (p[0]<<8);
		segs &= 0x0f;
		p += 8;
		logm_trace (vdec_userdata, "%d segments\n", segs);

		off = 0;
		for (a=0; a<segs; a++)
		{
			int type = p[1] | (p[0]<<8);
			int size = p[3] | (p[2]<<8);
			int i;
			unsigned char *d;
			struct userdata *new;


			/* check size and get the pointer */
			if ((buf->user_data_size < 8 + 8*16 + off + size) ||
					(size > 4096))
			{
				logm_warning (vdec_userdata, "wrong user data size.. "
						"segs:%d, size:%d\n",
						segs, size);
				break;
			}

			logm_trace (vdec_userdata, "type %d, offset %d, size %d\n",
					type, off, size);
			d = (char*)buf->user_data_addr + 8+8*16+off;


			/* save the data to new structure */
			new = kcalloc (sizeof(struct userdata) + size, 1, GFP_ATOMIC);
			if (new == NULL)
			{
				logm_warning (vdec_userdata, "kcalloc() failed\n");
				break;
			}

			if (ret == NULL)
				ret = new;
			else
				pre->next = new;
			new->type = type;
			new->size = size;
			memcpy (new->data, d, size);
			pre = new;

			/* mpeg2 Annex D. 3D frame packing arrangement check */
			if (vdec->codec == LX_VDEC_CODEC_MPEG2_HP && new->size >= 8)
			{
				if (
						new->data[0] == 0x4a &&
						new->data[1] == 0x50 &&
						new->data[2] == 0x33 &&
						new->data[3] == 0x44
				   )
				{
					int arrangement_type;

					arrangement_type = new->data[5]&0x7f;
					logm_info (vdec_userdata, "got mpeg2 fpa. 0x%02x\n",
							arrangement_type);

					switch (arrangement_type)
					{
					default:
						logm_warning (vdec_userdata, "unknown mpeg2 fpa. %02x\n",
								arrangement_type);
						break;
					case 0x03:
					case 0x13:
						buf->frame_packing_arrangement =
							vo_3d_type_side_by_side;
						break;
					case 0x04:
						buf->frame_packing_arrangement =
							vo_3d_type_top_bottom;
						break;
					case 0x08:
						buf->frame_packing_arrangement =
							vo_3d_type_none;
						break;
					}
				}
			}

			if (logm_enabled (vdec_userdata, log_level_debug))
			{
				for (i=0; i<size; )
				{
					char tmp[32*3+1];
					int b;

					for (b=0; b<32 && i+b<size; b++)
						sprintf (tmp+b*3, "%02x ", new->data[i+b]);
					tmp[b*3] = 0;;

					logm_debug (vdec_userdata, "%3d: %s\n",
							i, tmp);

					i += b;
				}
			}

			off += size;
			off = (off+7)&~7;
			p += 8;
		}
	}

	return ret;
}


static int save_userdata (vdec_t *vdec, struct userdata *userdata)
{
	vdec_t *thief;
	unsigned long flag;

	/* search user data thief */
	spin_lock_irqsave (&device_list_lock, flag);
	thief = device_list;
	while (thief)
	{
		if (thief->userdata_thief && thief != vdec)
		{
			logm_debug (vdec, "vdec:%p, thief found. %p\n",
					vdec, thief);

			break;
		}

		thief = thief->next;
	}

	/* send to upper layer */
	noti_save (&vdec->noti, LX_VDEC_READ_MAGIC("USID"),
			&vdec->id, sizeof (vdec->id));
	if (thief)
		noti_save (&thief->noti, LX_VDEC_READ_MAGIC("USID"),
				&vdec->id, sizeof (vdec->id));

	do
	{
		noti_save (&vdec->noti, LX_VDEC_READ_MAGIC ("USRD"),
				userdata->data, userdata->size);
		if (thief)
			noti_save (&thief->noti, LX_VDEC_READ_MAGIC ("USRD"),
					userdata->data, userdata->size);

		userdata = userdata->next;
	}
	while (userdata);

	spin_unlock_irqrestore (&device_list_lock, flag);

	return 0;
}


static int buf2BUF (vdec_t *vdec, decoded_buffer_t *buf, S_DISPQ_BUF_T *BUF)
{
	struct vo_vdo_info *vdo_info =
		(struct vo_vdo_info*)buf->addr_tile_base;

	memset (BUF, 0, sizeof (*BUF));

	BUF->ui32Tiled_FrameBaseAddr = vdo_info->tile_base;
	BUF->ui32DpbMapMode = vdo_info->map_type;
	BUF->ui32Y_FrameBaseAddr = buf->addr_y;
	BUF->ui32C_FrameBaseAddr = buf->addr_cb;
	//BUF->ui32Y_FrameBaseAddrBot = buf->addr_y_bot;
	//BUF->ui32C_FrameBaseAddrBot = buf->addr_cb_bot;
	BUF->ui32Stride = buf->stride;
	BUF->FrameFormat = DISPQ_FRAME_FORMAT_420;
	if (
			vdec->framerate_num &&
			(
			 buf->framerate_num != vdec->framerate_num ||
			 buf->framerate_den != vdec->framerate_den
			)
	   )
	{
		logm_debug (vdec, "vdec:%p, change framerate "
				"from %d/%d to %d/%d\n",
				vdec,
				buf->framerate_num,
				buf->framerate_den,
				vdec->framerate_num,
				vdec->framerate_den);
		BUF->ui32FrameRateRes = vdec->framerate_num;
		BUF->ui32FrameRateDiv = vdec->framerate_den;
	}
	else
	{
		BUF->ui32FrameRateRes = buf->framerate_num;
		BUF->ui32FrameRateDiv = buf->framerate_den;
	}
	if (vdec_force_framerate_num)
	{
		logm_debug (vdec, "vdec:%p, force framerate %d/%d\n",
				vdec,
				vdec_force_framerate_num,
				vdec_force_framerate_den);
		BUF->ui32FrameRateRes = vdec_force_framerate_num;
		BUF->ui32FrameRateDiv = vdec_force_framerate_den;
	}
	BUF->bVariableFrameRate = 0;
	BUF->ui32FrameIdx = buf->buffer_index;
	BUF->ui32AspectRatio = 0;
	BUF->ui32PicWidth = buf->width;
	BUF->ui32PicHeight = buf->height;
	BUF->ui32H_Offset = buf->crop_left<<16 | buf->crop_right;
	BUF->ui32V_Offset = buf->crop_top<<16 | buf->crop_bottom;
	BUF->ui32DisplayPeriod = buf->display_period;
	BUF->ui8ActiveFormatDescription = buf->active_format;
	switch (buf->interlace)
	{
	default:
		// fallthrough
	case decoded_buffer_interlace_none:
		BUF->ui32DisplayInfo = DISPQ_SCAN_PROG;
		break;

	case decoded_buffer_interlace_top_only:
		BUF->ui32DisplayPeriod = 1;
		// fallthrough
	case decoded_buffer_interlace_top_first:
		BUF->ui32DisplayInfo = DISPQ_SCAN_TFF;
		break;

	case decoded_buffer_interlace_bottom_only:
		BUF->ui32DisplayPeriod = 1;
		// fallthrough
	case decoded_buffer_interlace_bottom_first:
		BUF->ui32DisplayInfo = DISPQ_SCAN_BFF;
		break;
	}
	BUF->ui32DTS = buf->dts;
	if (buf->pts != VDEC_UNKNOWN_PTS)
		BUF->ui32PTS = buf->pts;
	else if (buf->timestamp != -1)
	{
		unsigned long long _pts = buf->timestamp*9;

		do_div(_pts,100000);
		BUF->ui32PTS = _pts;
	}
	else
		BUF->ui32PTS = VDEC_UNKNOWN_PTS;
	BUF->ui64TimeStamp = buf->timestamp;
	BUF->ui32UId = buf->uid;
	BUF->i32FramePackArrange = buf->frame_packing_arrangement;
	switch (buf->multi_picture)
	{
	default:
		// fallthrough
	case decoded_buffer_multi_picture_none:
		BUF->ui32LR_Order = DISPQ_3D_FRAME_NONE;
		break;

	case decoded_buffer_multi_picture_left:
		BUF->ui32LR_Order = DISPQ_3D_FRAME_LEFT;
		break;

	case decoded_buffer_multi_picture_right:
		BUF->ui32LR_Order = DISPQ_3D_FRAME_RIGHT;
		break;
	}
	BUF->ui16ParW = buf->par_w;
	BUF->ui16ParH = buf->par_h;

	BUF->bDiscont = buf->stc_discontinuity;

	return 0;
}

static int set_basetime(vdec_t *vdec,
		unsigned int base_stc, unsigned int base_pts)
{
	logm_noti (vdec, "vdec:%p, basetime %08x, pts %08x\n", vdec,
			base_stc, base_pts);

	if (vdec->sync < 0)
	{
		logm_warning (vdec, "vdec:%p, no sync\n", vdec);
		return -1;
	}

	if (vdec->use_gstc && vdec->port_adec >= 0)
	{
		AV_Set_AVLipSyncBase (vdec->port, vdec->port_adec,
				&base_stc, &base_pts);

		logm_noti (vdec, "vdec:%p, real basetime %08x, pts %08x\n",
				vdec, base_stc, base_pts);
	}

	vdec->use_gstc_have_cut = 1;

	output_cmd (vdec->sync, output_cmd_basetime_stc, base_stc);
	output_cmd (vdec->sync, output_cmd_basetime_pts, base_pts);

	return 0;
}


static int cut_base_adecmaster (vdec_t *vdec)
{
	unsigned int base_stc, base_pts;

	base_stc = base_pts = (unsigned int)-1;
	AV_LipSync_HAL_Adec_GetBase (vdec->port_adec,
			&base_stc, &base_pts);

	logm_debug (vdec, "vdec:%p, adec%d master, basestc %08x, basepts %08x\n",
			vdec, vdec->port_adec, base_stc, base_pts);
	if (base_stc == (unsigned int)-1 &&
			base_pts == (unsigned int)-1)
	{
		logm_debug (vdec, "vdec:%p, no adec base yet\n", vdec);
		return -1;
	}

	if (vdec->adec_base_stc != base_stc ||
			vdec->adec_base_pts != base_pts)
	{
		logm_noti (vdec, "vdec:%p, cut basetime from adec%d, "
				"basestc %08x, basepts %08x\n",
				vdec, vdec->port_adec,
				base_stc, base_pts);

		output_cmd (vdec->sync, output_cmd_basetime_stc, base_stc);
		output_cmd (vdec->sync, output_cmd_basetime_pts, base_pts);

		vdec->adec_base_stc = base_stc;
		vdec->adec_base_pts = base_pts;
	}

	return 0;
}


/*
 * cut sink basetime.
 * returns:
 *   0 : success.
 *  -1 : failed.
 */
static int cut_base (vdec_t *vdec, unsigned int pts)
{
	unsigned long flag;

	if (likely (!vdec->use_gstc))
	{
		/* use STC. we dont need to cut basetime. */
		return 0;
	}

	/* adec master mode... */
	if (vdec->flags & lx_vdec_flags_adec_master)
		return cut_base_adecmaster (vdec);;

	/* vdec master, or first comes. */
	spin_lock_irqsave (&vdec->speed_change, flag);
	if (unlikely (
				vdec->speed &&
				!vdec->use_gstc_have_cut
		     ))
		set_basetime (vdec, TOP_HAL_GetGSTCC(), pts);
	spin_unlock_irqrestore (&vdec->speed_change, flag);

	return 0;
}


static int display_picture (vdec_t *vdec,
		struct decoded_info *info, struct userdata *userdata)
{
	unsigned long flag;
	decoded_buffer_t *buf = &info->buffer;

	vdec->frame_displayed ++;
	logm_debug (vdec, "vdec:%p, pts %08x, ts %llu, %d\n", vdec,
			buf->pts, buf->timestamp,
			vdec->frame_displayed);

	/* send user data */
	if (userdata)
		save_userdata (vdec, userdata);


	if (vdec->sync >= 0)
	{
		S_DISPQ_BUF_T _buf;
		int de_id;
		enum vo_3d_type trid_type;

		if (vdec->opmode == LX_VDEC_OPMOD_LOW_LATENCY &&
				buf->height <= 720)
		{
			logm_debug (vdec, "vdec:%p, low latency, "
					"change framerate %d/%d -> %d/%d\n",
					vdec,
					buf->framerate_num, vdec->framerate_den,
					60, 1);

			buf->framerate_num = 60;
			buf->framerate_den = 1;
		}

		/* convert to display buffer */
		buf2BUF (vdec, buf, &_buf);


		/* cut base_stc and base_pts now if we use gstc */
		if (cut_base (vdec, _buf.ui32PTS) < 0)
		{
			logm_info (vdec, "vdec:%p, ignore pts.\n", vdec);
			_buf.ui32PTS = VDEC_UNKNOWN_PTS;
		}


		/* set the information to output */
		if (unlikely (
					buf->interlace == decoded_buffer_interlace_top_only ||
					buf->interlace == decoded_buffer_interlace_bottom_only
			     ))
		{
			logm_debug (vdec, "vdec:%p, dish trick.\n", vdec);

			output_cmd (vdec->sync, output_cmd_display,
					(unsigned long)&_buf);
			output_cmd (vdec->sync, output_cmd_display,
					(unsigned long)&_buf);
		}
		else
		{
			output_cmd (vdec->sync, output_cmd_display,
					(unsigned long)&_buf);
			if (vdec->flags & FLAGS_INTER_DUAL_OUT)
				info->user ++;
		}

		logm_debug (vdec, "vdec:%p, display queue pushed. qlen %ld\n",
				vdec,
				output_cmd (vdec->sync,
					output_cmd_display_queued, 0));

		/* update output status */
		switch (vdec->dest)
		{
		default:
			logm_warning (vdec, "vdec:%p, unknown dest %d\n",
					vdec, vdec->dest);
			de_id = 0;
			break;

		case LX_VDEC_DST_DE0: de_id = 0; break;
		case LX_VDEC_DST_DE1: de_id = 1; break;
		}

		if (vdec->trid_type != vo_3d_type_none)
			trid_type = vdec->trid_type;
		else if (buf->frame_packing_arrangement < 0)
			trid_type = vo_3d_type_none;
		else if (buf->frame_packing_arrangement > vo_3d_type_fpa_end)
			trid_type = vo_3d_type_none;
		else
			trid_type = buf->frame_packing_arrangement;

		vo_set_output_info (de_id,
				buf->width - buf->crop_left - buf->crop_right,
				buf->height - buf->crop_top - buf->crop_bottom,
				buf->framerate_num, buf->framerate_den,
				_buf.ui16ParH, _buf.ui16ParH,
				buf->interlace == decoded_buffer_interlace_none,
				trid_type
				);
	}

	spin_lock_irqsave (&vdec->dpbdump_lock, flag);
	if (vdec->dpbdump.data)
	{
		int head, tail;

		head = vdec->dpbdump.head;
		tail = vdec->dpbdump.tail;

		if ((head+1)%vdec->dpbdump.size != tail)
		{
			/* give additional user count for dpb dup */
			info->user ++;

			vdec->dpbdump.data[head] = buf;
			head ++;
			head %= vdec->dpbdump.size;
			vdec->dpbdump.head = head;

			wake_up (&vdec->dpbdump.wq);
		}
		else
			logm_warning (vdec, "vdec:%p, dpbdump queue full\n",
					vdec);
	}
	spin_unlock_irqrestore (&vdec->dpbdump_lock, flag);

	return 0;
}

static int free_dpb (vdec_t *vdec)
{
	struct dpb *dpb;
	unsigned long flag;

	spin_lock_irqsave (&vdec->dpb_lock, flag);
	while (vdec->dpb)
	{
		dpb = vdec->dpb;

		logm_info (vdec, "vdec:%p, free %lx\n",
				vdec, dpb->fb.y);
		if (!(vdec->flags & lx_vdec_flags_user_dpb))
		{
			logm_info (vdec, "vdec:%p, my buffer.\n", vdec);
			vdec_free (dpb->fb.y);
		}
		else
			logm_info (vdec, "vdec:%p, user buffer.\n", vdec);
		vdec->dpb = dpb->next;

		kfree (dpb);
	}
	spin_unlock_irqrestore (&vdec->dpb_lock, flag);

	if (vdec->wait_dpb)
	{
		kfree (vdec->wait_dpb);
		vdec->wait_dpb = NULL;
	}

	return 0;
}

static int add_dpb (vdec_t *vdec, struct dpb *dpb)
{
	VDC_LINEAR_FRAME_BUF_T info;

	if (unlikely (vdec->vdc < 0))
	{
		logm_warning (vdec, "vdec:%p, no vdc\n", vdec);
		return -1;
	}

	memset (&info, 0, sizeof (info));
	info.ui32Stride = dpb->fb.stride;
	info.ui32Height = 1088;
	info.ui32BufAddr = dpb->fb.y;
	logm_debug (vdec, "vdec:%p, add %lx(stride %d)\n", vdec,
			dpb->fb.y, dpb->fb.stride);
	VDC_SetExternDpb (vdec->vdc, info);

	return 0;
}

static int set_dpb (vdec_t *vdec)
{
	struct dpb *dpb;
	unsigned long flag;

	spin_lock_irqsave (&vdec->dpb_lock, flag);
	dpb = vdec->dpb;
	while (dpb)
	{
		add_dpb (vdec, dpb);

		dpb = dpb->next;
	}
	spin_unlock_irqrestore (&vdec->dpb_lock, flag);

	if (vdec->wait_dpb)
	{
		kfree (vdec->wait_dpb);
		vdec->wait_dpb = NULL;
	}

	/* kick decoder */
	vdec->kick_decoder_count ++;
	queue_work (vdec_wq, &kick_decoder_work);

	return 0;
}

static int allocate_dpb (vdec_t *vdec,
		int width, int height, int max_dpbs)
{
	int a;
	int fb_size;
	struct dpb *dpb;
	unsigned long flag;

	width = (width + 15)&~15;
	height = (height + 15)&~15;
	fb_size = width * height + width * height/2;
	logm_debug (vdec, "vdec:%p, buffer %dx%d, fb_size %d, %d dpbs\n",
			vdec, width, height,
			fb_size, max_dpbs);

	for (a=0; a<max_dpbs; a++)
	{
		dpb = kzalloc (sizeof (*dpb), GFP_ATOMIC);
		if (!dpb)
		{
			logm_warning (vdec, "vdec:%p, no mem\n", vdec);
			break;
		}

		dpb->fb.y = vdec_alloc (fb_size, 1<<16, "raster dpb");
		dpb->fb.stride = width;
		dpb->fb.width = width;
		dpb->fb.height = height;

		logm_debug (vdec, "vdec:%p, dpb %lx\n", vdec, dpb->fb.y);

		spin_lock_irqsave (&vdec->dpb_lock, flag);
		dpb->next = vdec->dpb;
		vdec->dpb = dpb;
		spin_unlock_irqrestore (&vdec->dpb_lock, flag);
	}

	if (a != max_dpbs)
	{
		logm_warning (vdec, "vdec:%p, failed to allocate dpb\n",
				vdec);

		free_dpb (vdec);

		return -1;
	}

	return 0;
}

static int print_buffer_info (vdec_t *vdec, decoded_buffer_t *buf)
{
	struct decoded_info *now;
	char *tmp;
	unsigned long flag;
	int tmp_len;
	const char *field_info, *pic_type;
	int a;
	struct vo_vdo_info *vdo_info =
		(struct vo_vdo_info*)buf->addr_tile_base;

	spin_lock_irqsave (&vdec->decoded_buffers_lock, flag);
	for (
			tmp_len=1, now=vdec->decoded_infos;
			now;
			now=now->next, tmp_len++
	    );
	tmp = kmalloc (tmp_len, GFP_ATOMIC);
	for (
			a=0, now=vdec->decoded_infos;
			now;
			a++, now=now->next
	    )
	{
		if (now->buffer.buffer_index < 10)
			tmp[a] = now->buffer.buffer_index + '0';
		else
			tmp[a] = now->buffer.buffer_index - 10 + 'a';
	}
	tmp[a] = 0;
	spin_unlock_irqrestore (&vdec->decoded_buffers_lock, flag);

	switch (buf->interlace)
	{
	default:
		logm_warning (vdec, "unknown field info %d\n",
				buf->interlace);
		field_info = "XXXX";
		break;

	case decoded_buffer_interlace_none:
		field_info = "pro.";
		break;

	case decoded_buffer_interlace_top_first:
		field_info = "top.";
		break;

	case decoded_buffer_interlace_bottom_first:
		field_info = "bot.";
		break;
	}

	switch (buf->picture_type)
	{
	default:
		logm_warning (vdec, "unknown picture type %d\n",
				buf->picture_type);
		pic_type = "X";
		break;

	case decoded_buffer_picture_type_i:
		pic_type = "I";
		break;

	case decoded_buffer_picture_type_p:
		pic_type = "P";
		break;

	case decoded_buffer_picture_type_b:
		pic_type = "B";
		break;

	case decoded_buffer_picture_type_bi:
		pic_type = "BI";
		break;

	case decoded_buffer_picture_type_d:
		pic_type = "D";
		break;

	case decoded_buffer_picture_type_s:
		pic_type = "S";
		break;

	case decoded_buffer_picture_type_pskip:
		pic_type = "PSKIP";
		break;

	}

	logm_debug (vdec, "vdec:%p, base:%lx(%d),y:%07lx,cb:%07lx, "
			"%2d,%dx%d(%d,%d,%d,%d,%d),(%dx%d)"
			"%lld\n",
			vdec,
			buf->addr_tile_base, buf->vdo_map_type,
			buf->addr_y,
			buf->addr_cb,

			buf->buffer_index,
			buf->width, buf->height, buf->stride,
			buf->crop_left, buf->crop_right,
			buf->crop_top, buf->crop_bottom,
			buf->display_width, buf->display_height,

			buf->timestamp);

	logm_debug (vdec, "vdec:%p, "
			"fr:%d/%d,error blocks:%d,afd:%x,fpa:%d, "
			"tilebase:%lx,maptype:%d, "
			"par:%dx%d,flag:%x,lr:%d,%s(%d),%s,%s\n",
			vdec,
			buf->framerate_num,
			buf->framerate_den,
			buf->error_blocks,
			buf->active_format,
			buf->frame_packing_arrangement,

			vdo_info?vdo_info->tile_base:0,
			vdo_info?vdo_info->map_type:0,

			buf->par_w, buf->par_h,
			buf->report,
			buf->multi_picture,
			field_info, buf->display_period&7,
			pic_type,
			tmp
		   );

	if (vdec->dpb)
	{
		int a;
		struct dpb *dpb;

		for (a=0, dpb=vdec->dpb; dpb; dpb=dpb->next, a++);
		logm_debug (vdec, "vdec:%p, user dpbs, %d, registered\n",
				vdec, a);
	}

	kfree (tmp);

	return 0;
}

static struct decoded_info *make_decoded_info (vdec_t *vdec,
		decoded_buffer_t *buf)
{
	struct decoded_info *info;
	struct vo_vdo_info *vdo_info;
	int map_type;
	unsigned long flag;

	map_type = buf->vdo_map_type;

	/* save VDO information on local memory */
	vdo_info = vdec->vdo_info;
	if (
			vdo_info == NULL ||
			vdo_info->tile_base != buf->addr_tile_base ||
			vdo_info->map_type != map_type
	   )
	{
		vdo_info = kcalloc (1, sizeof (*vdec->vdo_info),
				GFP_ATOMIC);
		if (vdo_info == NULL)
		{
			logm_warning (vdec, "vdec:%p, no memory?\n",
					vdec);
			return NULL;
		}

		vdo_info->next = vdec->vdo_info;
		vdec->vdo_info = vdo_info;

		/* save VDO information */
		vdo_info->tile_base = buf->addr_tile_base;
		vdo_info->map_type = map_type;

		logm_noti (vdec, "vdec:%p, new vdo %p, base %08lx, type %d\n",
				vdec, vdo_info,
				vdo_info->tile_base, vdo_info->map_type);
	}

	buf->addr_tile_base = (unsigned long)vdo_info;

	/* wake cpbdump */
	if (vdec->cpbdump.addr)
		wake_up (&vdec->cpbdump.wq);

	/*
	 * register the index and buffer address to the table that will
	 * be used in pushing to clear queue
	 */
	info = kmalloc (sizeof (*info), GFP_ATOMIC);
	if (info == NULL)
	{
		logm_error (vdec, "vdec:%p, kmalloc failed\n", vdec);
		return NULL;
	}
	info->buffer = *buf;
	info->user = 1;

	if (!(vdec->flags & lx_vdec_flags_user_dpb))
	{
		spin_lock_irqsave (&vdec->decoded_buffers_lock, flag);
		info->next = vdec->decoded_infos;
		vdec->decoded_infos = info;
		spin_unlock_irqrestore (&vdec->decoded_buffers_lock, flag);
	}
	else
		logm_debug (vdec, "vdec:%p, info, %p, is not registered to decoded_infos.\n",
				vdec, info);

	return info;
}

static int save_decoded_info (vdec_t *vdec,
		struct decoded_info *info, decoded_buffer_t *buf)
{
	if (vdec_force_frame_alternation &&
			buf->buffer_index >= 0)
	{
		int org;

		org = buf->multi_picture;
		buf->multi_picture = decoded_buffer_multi_picture_left;
		noti_save (&vdec->noti, LX_VDEC_READ_MAGIC("DECO"),
				buf, sizeof(*buf));
		buf->multi_picture = decoded_buffer_multi_picture_right;
		noti_save (&vdec->noti, LX_VDEC_READ_MAGIC("DECO"),
				buf, sizeof(*buf));

		buf->multi_picture = org;

		/* one more frame user */
		info->user ++;
	}
	else
	{
		if (unlikely(
					(buf->buffer_index >= 0) &&
					(
					 (buf->interlace == decoded_buffer_interlace_top_only) ||
					 (buf->interlace == decoded_buffer_interlace_bottom_only)
					)
			    ))
		{
			decoded_buffer_t _buf;

			/* DISH CP exception
			 *
			 * DISH CP에서 배속 재생 시 TOP field만
			 * 반복해서 전달되는 경우가 있다. 이 경우 top,
			 * bottom 출력을 위해 두장의 event로
			 * 만들어준다.
			 *
			 * top을 두번 전달할 때 VDISP 쪽에서 예외처리를
			 * 할 것이며, 두번째 top field를 bottom으로
			 * 바꿀 것이다.
			 */

			_buf = *buf;

			if (_buf.interlace == decoded_buffer_interlace_top_only)
				_buf.interlace = decoded_buffer_interlace_top_first;
			else
				_buf.interlace = decoded_buffer_interlace_bottom_first;

			_buf.display_period = 1;

			noti_save (&vdec->noti,
					LX_VDEC_READ_MAGIC("DECO"),
					&_buf, sizeof(_buf));
			noti_save (&vdec->noti,
					LX_VDEC_READ_MAGIC("DECO"),
					&_buf, sizeof(_buf));

			/* one more frame user */
			info->user ++;
			logm_debug (vdec, "vdec:%p, brainfart dish trick.\n", vdec);
		}
		else
			noti_save (&vdec->noti,
					LX_VDEC_READ_MAGIC("DECO"),
					buf, sizeof(*buf));
	}

	return 0;
}

static int decoded_queue_push_locked (vdec_t *vdec,
		decoded_q_t *q, decoded_buffer_t *buf)
{
	struct decoded_info *info = NULL;
	struct userdata *userdata = NULL;

	if (unlikely (vdec->vdc < 0))
	{
		logm_noti (vdec, "vdec:%p, callback while closed vdc\n",
				vdec);
		return 0;
	}

	/* ignore repeated EOS */
	if (unlikely (buf->report & DECODED_REPORT_EOS))
	{
		if (!(vdec->flags & FLAGS_INTER_WAIT_EOS))
		{
			logm_debug (vdec, "vdec:%p, unexpected EOS. ignore\n",
					vdec);

			buf->report &= ~DECODED_REPORT_EOS;
		}
	}

	if (unlikely (buf->buffer_index < 0 &&
			buf->report == 0))
	{
		logm_debug (vdec, "vdec:%p, no information\n",
				vdec);

		return 0;
	}

	/* trick test for DISH CP */
	if (unlikely(
				vdec_test_dish_trick &&
				buf->interlace != decoded_buffer_interlace_none
		    ))
		buf->interlace = decoded_buffer_interlace_top_only;

	/* fill user_dpb_priv */
	if (unlikely (vdec->flags & lx_vdec_flags_user_dpb) &&
			buf->buffer_index >= 0)
	{
		struct dpb *dpb, *pre;
		unsigned long flag;

		spin_lock_irqsave (&vdec->dpb_lock, flag);
		dpb = vdec->dpb;
		pre = dpb;	// prevent compiler warning
		while (dpb)
		{
			if (dpb->fb.y == buf->addr_y)
			{
				break;
			}

			pre = dpb;
			dpb = dpb->next;
		}

		if (dpb)
		{
			buf->user_dpb_priv = dpb->fb.priv;

			/* remove the dpb */
			if (dpb == vdec->dpb)
				vdec->dpb = dpb->next;
			else
				pre->next = dpb->next;
			kfree (dpb);
		}
		else
		{
			buf->user_dpb_priv = 0;
			logm_error (vdec, "vdec:%p, unknown decoded buffer\n",
					vdec);
		}
		spin_unlock_irqrestore (&vdec->dpb_lock, flag);
	}
	else
		buf->user_dpb_priv = 0;

	/* progressive test */
	if (unlikely (vdec_force_progressive))
		buf->interlace = decoded_buffer_interlace_none;

	/* wrong framerate test */
	if (unlikely (vdec_test_wrong_fr))
	{
		logm_debug (vdec, "vdec:%p, change framerate %d/%d to %d\n",
				vdec, buf->framerate_num, buf->framerate_den,
				vdec_test_wrong_fr);

		buf->framerate_num = vdec_test_wrong_fr;
		buf->framerate_den = 1;
	}

	if (likely (buf->buffer_index >= 0))
	{
		/* check wrong encoded stream.
		 * change screen size 1920x1088 to 1920x1080 */
		if (unlikely (buf->height == 1088) &&
				buf->crop_top == 0 &&
				buf->crop_bottom == 0 &&
				(buf->width - buf->crop_left - buf->crop_right) == 1920)
		{
			logm_debug (vdec, "vdec:%p, 1088 height. fixup to 1080\n", vdec);
			buf->crop_bottom = 8;
		}

		/* read userdata */
		if (buf->user_data_addr && buf->user_data_size > 0)
			userdata = read_userdata (vdec, buf);

		/* save decoded info to local info structure */
		info = make_decoded_info (vdec, buf);
		if (!info)
			goto done;
	}

	/* log decoded information... */
	if (logm_enabled (vdec, log_level_debug))
		print_buffer_info (vdec, buf);


	/* handle status report */
	if (buf->report & DECODED_REPORT_HW_RESET)
	{
		logm_debug (vdec, "vdec:%p, do hardware reset\n",
				vdec);
		//noti_save (&vdec->noti, LX_VDEC_READ_MAGIC("RSET"),
		//		NULL, 0);
		goto done;
	}

	if (buf->report & DECODED_REPORT_REQUEST_DPB)
	{
		logm_debug (vdec, "vdec:%p, request %d dpbs\n",
				vdec, buf->num_of_buffer_required);

		if (vdec->flags & lx_vdec_flags_user_dpb)
		{
			logm_debug (vdec, "vdec:%p, user should supply dpb memory\n", vdec);
			if (vdec->wait_dpb)
				kfree (vdec->wait_dpb);
			vdec->wait_dpb = kmalloc (sizeof (*buf), GFP_ATOMIC);
			if (!vdec->wait_dpb)
			{
				logm_warning (vdec, "vdec:%p, no mem\n", vdec);
				goto done;
			}
			*vdec->wait_dpb = *buf;
		}
		else if (vdec->flags & lx_vdec_flags_copy_dpb)
		{
			logm_debug (vdec, "vdec:%p, copy dpb mode\n", vdec);
			if (allocate_dpb (vdec, buf->width, buf->height, 1)
					>= 0)
				add_dpb (vdec, vdec->dpb);
		}
		else if (vdec->output_memory_format != LX_VDEC_MEMORY_FORMAT_RASTER)
		{
			logm_warning (vdec, "vdec:%p, "
					"request dpb on non raster dpb mode\n",
					vdec);
			goto done;
		}
		else
		{
			if (allocate_dpb (vdec, buf->width, buf->height,
					buf->num_of_buffer_required) >= 0)
				set_dpb (vdec);
		}
	}

#if defined(CHIP_NAME_d13) || defined(CHIP_NAME_d14)
	if (buf->report & DECODED_REPORT_ERROR_MASK)
	{
		logm_warning (vdec, "vdec:%p, decoder error. 0x%x\n", vdec,
				buf->report&DECODED_REPORT_ERROR_MASK);
		DX_IPC_HAL_SetReg (DX_REG_START_ERROR,
				buf->report&DECODED_REPORT_ERROR_MASK);
	}
#endif

	/* save decoded information */
	save_decoded_info (vdec, info, buf);

	if (buf->buffer_index >= 0)
		display_picture (vdec, info, userdata);
	else
		logm_debug (vdec, "vdec:%p, no display\n", vdec);

done:
	while (userdata)
	{
		struct userdata *n;

		n = userdata->next;

		kfree (userdata);
		userdata = n;
	}

	/*
	 * info is not managed by vdec->decoded_infos in case of user_dpb.
	 *
	 * see make_decoded_info()
	 */
	if (vdec->flags & lx_vdec_flags_user_dpb)
	{
		logm_debug (vdec, "vdec:%p, free info buffer, %p\n",
				vdec, info);
		if (info)
			kfree (info);
	}

	if (unlikely (buf->report & DECODED_REPORT_EOS))
	{
		if (vdec->flags & FLAGS_INTER_WAIT_EOS)
		{
			unsigned long flags;

			logm_debug (vdec, "vdec:%p, remove wait_eos flag\n", vdec);
			spin_lock_irqsave (&vdec->flags_lock, flags);
			vdec->flags &= ~FLAGS_INTER_WAIT_EOS;
			spin_unlock_irqrestore (&vdec->flags_lock, flags);
		}
	}

	return 0;
}

static int decoded_queue_push (decoded_q_t *q, decoded_buffer_t *buf)
{
	unsigned long flag;
	vdec_t *vdec = container_of (q, vdec_t, decoded_q);
	int ret;

	spin_lock_irqsave (&vdec->submodule_spin_lock, flag);
	ret = decoded_queue_push_locked (vdec, q, buf);
	spin_unlock_irqrestore (&vdec->submodule_spin_lock, flag);

	return ret;
}

static int decoded_queue_max_size (decoded_q_t *q)
{
	vdec_t *vdec = container_of (q, vdec_t, decoded_q);
	int maxsize;

	if (vdec->dest == LX_VDEC_DST_BUFF)
		maxsize = vdec->noti.size;
	else
		maxsize = vdec->display_q_maxsize;

	logm_trace (vdec, "vdec:%p, %d\n", vdec, maxsize);

	return maxsize-1;
}

static int decoded_queue_size (decoded_q_t *q)
{
	vdec_t *vdec = container_of (q, vdec_t, decoded_q);

	if (vdec->dest == LX_VDEC_DST_BUFF)
	{
		int head, tail, size;
		int ret;

		head = vdec->noti.head;
		tail = vdec->noti.tail;
		size = vdec->noti.size;

		if (head >= tail)
			ret = head-tail;
		else
			ret = size-tail+head;

		logm_trace (vdec, "vdec:%p, size %d\n", vdec, ret);
		return ret;
	}

	if (vdec->sync >= 0)
		return output_cmd (vdec->sync, output_cmd_display_queued, 0);

	logm_noti (vdec, "Closing??\n");

	return INT_MAX;
}

static void initialize_instance (vdec_t *vdec)
{
	S_DISPCLEARQ_BUF_T cq;
	unsigned long flag;

	logm_debug (vdec, "vdec:%p, reset instance vairables\n", vdec);

	/* clear internal decoded buffer */
	while (DISP_CLEAR_Q_Pop (vdec->clear_q, &cq));
	spin_lock_irqsave (&vdec->decoded_buffers_lock, flag);
	while (vdec->decoded_infos)
	{
		struct decoded_info *next;

		next = vdec->decoded_infos->next;
		kfree (vdec->decoded_infos);
		vdec->decoded_infos = next;
	}
	spin_unlock_irqrestore (&vdec->decoded_buffers_lock, flag);

	while (vdec->vdo_info)
	{
		struct vo_vdo_info *t;

		t = vdec->vdo_info->next;

		logm_noti (vdec, "vdec:%p, free vdo %p\n", vdec,
				vdec->vdo_info);
		kfree (vdec->vdo_info);

		vdec->vdo_info = t;
	}

	vdec->output_memory_format = LX_VDEC_MEMORY_FORMAT_TILED;
	vdec->scan_mode = LX_VDEC_PIC_SCAN_ALL;
	vdec->frame_displayed = 0;
	vdec->frame_requested = -1;

	vdec->cpbdump.now = 0;
	vdec->cpbdump.readed = 0;

	vdec->cpb_addr = 0;
	vdec->cpb_size = 0;
	vdec->cpb_allocated = 0;

	vdec->trid_type = vo_3d_type_none;
	vdec->kick_decoder_count = 0;

	vdec->use_gstc = 0;
	vdec->use_gstc_have_cut = 0;
}


static unsigned int active_pdec;
static unsigned int active_de;


static int destroy_submodules_locked (vdec_t *vdec)
{
	unsigned long flag, flag_vdc;
	int ves, vdc, sync;

	logm_noti (vdec, "vdec:%p\n", vdec);

	if (
			vdec->use_gstc &&
			vdec->use_gstc_have_cut &&
			vdec->port_adec >= 0
	   )
	{
		unsigned int a = -1;
		AV_Set_AVLipSyncBase (vdec->port, vdec->port_adec,
				&a, &a);
	}

	spin_lock_irqsave (&vdec->submodule_spin_lock, flag);
	spin_lock_irqsave (&vdec->submodule_vdc_spin_lock, flag_vdc);
	ves = vdec->ves;
	vdc = vdec->vdc;
	sync = vdec->sync;

	vdec->ves = 0xff;
	vdec->vdc = -1;
	vdec->sync = -1;
	spin_unlock_irqrestore (&vdec->submodule_vdc_spin_lock, flag_vdc);
	spin_unlock_irqrestore (&vdec->submodule_spin_lock, flag);

	if (sync >= 0)
	{
		int de_id;

		logm_info (vdec, "close sync %d\n", sync);
		output_close (sync);

		switch (vdec->dest)
		{
		default:
		// fallthrough
		case LX_VDEC_DST_DE0: de_id = 0; break;
		case LX_VDEC_DST_DE1: de_id = 1; break;
		}

		vo_set_output_info (de_id, 0, 0, 0, 0,
				0, 0, 0, vo_3d_type_none);

		active_de &= ~(1 << de_id);
		if (vdec->flags & FLAGS_INTER_DUAL_OUT)
			active_de &= ~(1 << 1);
	}
	if(vdc >= 0)
	{
		logm_info (vdec, "vdec:%p, close vdc %d\n",
				vdec, vdc);
		VDC_Close(vdc);

		free_dpb (vdec);
	}
	if(ves != 0xFF)
	{
		logm_info (vdec, "vdec:%p, close ves %d\n",
				vdec, ves);
		VES_Close(ves);

		active_pdec &= ~(1 << vdec->src);
	}

	if (vdec->cpb_allocated)
	{
		logm_info (vdec, "vdec:%p, free cpb\n", vdec);
		vdec_free (vdec->cpb_addr);
		vdec->cpb_allocated = 0;
	}

	if (vdec->ion_client)
	{
		logm_info (vdec, "destroy ion client\n");
#ifdef SUPPORT_ION
		ion_client_destroy (vdec->ion_client);
#endif
		vdec->ion_client = NULL;
	}

	initialize_instance (vdec);

	/* wake up any pending poll() or read() system call */
	wake_up (&vdec->noti.wq);

	return 0;
}

static int destroy_submodules (vdec_t *vdec)
{
	int ret;

	logm_info (vdec, "vdec:%p, destroy submodule..\n", vdec);

	/* 먼저 submodule에 대해 동작을 멈춘 후 lock을 잡도록 한다. lock 안에서
	 * 동작을 멈추면 submodule의 callback이 호출돼 deadlock이 발생할 수
	 * 있다 */
	if (vdec->ves != 0xff)
		VES_Stop (vdec->ves);

	/* destroy module instance */
	mutex_lock (&io_lock);
	mutex_lock (&vdec->submodule_lock);
	ret = destroy_submodules_locked (vdec);
	mutex_unlock (&vdec->submodule_lock);
	mutex_unlock (&io_lock);

	return ret;
}

static void ves_event_cpb_full_ws (struct work_struct *work)
{
	unsigned long flag;
	vdec_t *vdec;
	vdec_t *now;

check_again:
	spin_lock_irqsave (&device_list_lock, flag);
	now = device_list;
	vdec = NULL;
	while (now)
	{
		if (now->cpb_full_count)
		{
			vdec = now;
			logm_debug (vdec, "vdec:%p, cpb full count %d\n",
					vdec, vdec->cpb_full_count);
			vdec->cpb_full_count = 0;
			break;
		}

		now = now->next;
	}
	spin_unlock_irqrestore (&device_list_lock, flag);

	if (vdec == NULL)
		return;

	mutex_lock (&vdec->submodule_lock);
	logm_noti (vdec, "vdec:%p, cpb full.\n", vdec);
	if (vdec->ves != 0xff)
		VES_Flush (vdec->ves);
	mutex_unlock (&vdec->submodule_lock);

	goto check_again;
}

static DECLARE_WORK (ves_event_cpb_full_work, ves_event_cpb_full_ws);

static void ves_event (unsigned char ves_ch,
		void *arg, E_VES_UPDATED_REASON_T reason)
{
	unsigned long flag;
	vdec_t *vdec = arg;

	logm_debug (vdec, "vdec:%p, ves%d reason:%d\n",
			vdec, vdec->ves, reason);

	spin_lock_irqsave (&vdec->submodule_spin_lock, flag);

	if (vdec->ves == 0xff || vdec->vdc < 0)
	{
		logm_warning (vdec, "vdec:%p, no ves %d, vdc %d\n", vdec,
				vdec->ves, vdec->vdc);
		spin_unlock_irqrestore (&vdec->submodule_spin_lock, flag);
		return;
	}

	switch (reason)
	{
	case VES_PIC_DETECTED:
		logm_debug (vdec, "vdec:%p, auib %d\n", vdec,
				VES_AUIB_NumActive (vdec->ves));

		/* kick decoder */
		vdec->kick_decoder_count ++;
		queue_work (vdec_wq, &kick_decoder_work);

		/* wake cpbdump */
		if (vdec->cpbdump.addr)
			wake_up (&vdec->cpbdump.wq);
		break;

	case VES_CPB_ALMOST_FULL:
		logm_noti (vdec, "vdec:%p, cpb almost full\n", vdec);
		vdec->cpb_full_count ++;
		queue_work (vdec_wq, &ves_event_cpb_full_work);
		break;
	}

	spin_unlock_irqrestore (&vdec->submodule_spin_lock, flag);
}

static int set_scan_picture(vdec_t *vdec, LX_VDEC_PIC_SCAN_T scan)
{
	vdec->scan_mode = scan;
	logm_noti (vdec, "vdec:%p, scanmode %d\n", vdec, scan);

	if (vdec->vdc >= 0)
	{
		VDC_SKIP_T _scan = VDC_SKIP_NONE;

		switch(scan)
		{
		case LX_VDEC_PIC_SCAN_ALL:
			_scan = VDC_SKIP_NONE;
			break;
		case LX_VDEC_PIC_SCAN_I:
			if (vdec_test_dish_trick_set)
				_scan = VDC_SKIP_DISH;
			else
				_scan = VDC_SKIP_PB;
			break;
		case LX_VDEC_PIC_SCAN_IP:
			_scan = VDC_SKIP_B;
			break;

		case LX_VDEC_PIC_SCAN_I_BRAINFART_DISH_TRICK:
			_scan = VDC_SKIP_DISH;
			break;

		default :
			logm_warning (vdec, "vdec:%p, unknown scanmode %d\n", vdec, scan);
			return -EINVAL;
		}

		VDC_SetSkipMode(vdec->vdc, _scan);
	}

	return 0;
}


const char *src_name (LX_VDEC_SRC_T s)
{
	switch (s)
	{
	case LX_VDEC_SRC_SDEC0:		return "sdec0";
	case LX_VDEC_SRC_SDEC1:		return "sdec1";
	case LX_VDEC_SRC_SDEC2:		return "sdec2";
	case LX_VDEC_SRC_BUFF:		return "fileplay";
	case LX_VDEC_SRC_BUFFTVP:	return "tvp";
	default:			return "unknown";
	}
}


const char *codec_name (LX_VDEC_CODEC_T c)
{
	switch (c)
	{
	case LX_VDEC_CODEC_H264_HP:	return "h264_hp";
	case LX_VDEC_CODEC_H264_MVC:	return "h264_mvc";
	case LX_VDEC_CODEC_H263_SORENSON: return "h263_sorenson";
	case LX_VDEC_CODEC_VC1_RCV_V1:	return "vc1_rcv_v1";
	case LX_VDEC_CODEC_VC1_RCV_V2:	return "vc1_rcv_v2";
	case LX_VDEC_CODEC_VC1_ES:	return "vc1_es";
	case LX_VDEC_CODEC_MPEG2_HP:	return "mpeg2_hp";
	case LX_VDEC_CODEC_MPEG4_ASP:	return "mpeg4_asp";
	case LX_VDEC_CODEC_XVID:	return "xvid";
	case LX_VDEC_CODEC_DIVX3:	return "divx3";
	case LX_VDEC_CODEC_DIVX4:	return "divx4";
	case LX_VDEC_CODEC_DIVX5:	return "divx5";
	case LX_VDEC_CODEC_RVX:		return "rvx";
	case LX_VDEC_CODEC_AVS:		return "avs";
	case LX_VDEC_CODEC_VP8:		return "vp8";
	case LX_VDEC_CODEC_THEORA:	return "theora";
	case LX_VDEC_CODEC_HEVC:	return "hevc";
	default:			return "unknown";
	}
}


static int init_input (vdec_t *vdec)
{
	E_VES_SRC_T src;

	/* check input and output */
	{
		struct
		{
			LX_VDEC_SRC_T s1;
			E_VES_SRC_T s2;
		} srcs[] =
		{
			{ LX_VDEC_SRC_SDEC0,    VES_SRC_SDEC0, },
			{ LX_VDEC_SRC_SDEC1,    VES_SRC_SDEC1, },
			{ LX_VDEC_SRC_SDEC2,    VES_SRC_SDEC2, },
			{ LX_VDEC_SRC_BUFF,     VES_SRC_FILEPLAY, },
			{ LX_VDEC_SRC_BUFFTVP,  VES_SRC_TVP, },
		};
		int a;

		for (a=0; a<ARRAY_SIZE (srcs); a++)
		{
			if (vdec->src == srcs[a].s1)
			{
				src = srcs[a].s2;
				break;
			}
		}
		if (a == ARRAY_SIZE (srcs))
		{
			logm_warning (vdec, "vdec:%p, unknown input device %d\n",
					vdec, vdec->src);
			return -1;
		}

		logm_noti (vdec, "vdec:%p, input %s\n",
				vdec, src_name(vdec->src));
	}


	/* open input device */
	if (
			LX_VDEC_SRC_SDEC0 <= vdec->src &&
			vdec->src <= LX_VDEC_SRC_SDEC2 &&
			active_pdec & (1 << vdec->src)
	   )
	{
		logm_warning (vdec, "vdec:%p, input device, %s, "
				"already occupied. 0x%x\n",
				vdec, src_name(vdec->src), active_pdec);
		return -1;
	}

	if (!vdec->cpb_size)
	{
		if( vdec->opmode == LX_VDEC_OPMOD_ONE_FRAME )
			vdec->cpb_size = 0x1F0000;
		else
		{
			vdec->cpb_size = vdec_default_cpb_size;
			vdec->cpb_size &= PAGE_MASK;
		}
	}

	if (!vdec->cpb_addr)
	{
		vdec->cpb_addr = vdec_alloc (vdec->cpb_size, 1<<12, "cpb");
		vdec->cpb_allocated = 1;
	}

	logm_noti (vdec, "vdec:%p, opening ves, cpb_size %x\n",
			vdec, vdec->cpb_size);
	vdec->ves = VES_Open(src, vdec->codec,
			vdec->cpb_addr, vdec->cpb_size,
			ves_event, vdec);
	if(vdec->ves == 0xFF)
	{
		logm_warning (vdec, "vdec:%p, ves open failed\n", vdec);
		return -1;
	}
	active_pdec |= 1<<vdec->src;

	return 0;
}


static int init_output (vdec_t *vdec, int deid,
		enum output_clock_source clock_src)
{
	if (active_de & (1<<deid))
	{
		logm_warning (vdec, "vdec:%p, output device, %d, "
				"already occupied. 0x%x\n",
				vdec, deid, active_de);
		return -1;
	}

	vdec->sync = output_open (deid, clock_src,
			vdec->display_q_maxsize,
			vdec->trid_type == vo_3d_type_dual,
			NULL);

	if(vdec->sync < 0)
	{
		logm_warning (vdec, "vdec:%p, "
				"sync open failed\n", vdec);

		return -1;
	}

	output_cmd (vdec->sync, output_cmd_pts_match_mode,
			vdec->match_mode);
	output_cmd (vdec->sync, output_cmd_display_offset,
			vdec->display_offset);
	output_cmd (vdec->sync, output_cmd_unref_callback,
			(long)unref_frame_fromsync);
	output_cmd (vdec->sync, output_cmd_unref_callback_arg,
			(long)vdec);

	if (vdec->opmode == LX_VDEC_OPMOD_LOW_LATENCY)
	{
		output_cmd (vdec->sync,
				output_cmd_framerate_num, 60);
		output_cmd (vdec->sync,
				output_cmd_framerate_den, 1);
	}

	active_de |= 1<<deid;

	return 0;
}


static int init_codec (vdec_t *vdec)
{
	VDC_OPEN_PARAM_T stVdcOpenParam;
	VDC_CODEC_T codec;

	/* check codec */
	{
		const struct
		{
			LX_VDEC_CODEC_T c1;
			VDC_CODEC_T c2;
		} cs[] =
		{
			{ LX_VDEC_CODEC_H264_HP,	VDC_CODEC_H264_HP,	},
			{ LX_VDEC_CODEC_H264_MVC,	VDC_CODEC_H264_MVC,	},
			{ LX_VDEC_CODEC_H263_SORENSON,	VDC_CODEC_H263_SORENSON,},
			{ LX_VDEC_CODEC_VC1_RCV_V1,	VDC_CODEC_VC1_RCV_V1,	},
			{ LX_VDEC_CODEC_VC1_RCV_V2,	VDC_CODEC_VC1_RCV_V2,	},
			{ LX_VDEC_CODEC_VC1_ES,		VDC_CODEC_VC1_ES,	},
			{ LX_VDEC_CODEC_MPEG2_HP,	VDC_CODEC_MPEG2_HP,	},
			{ LX_VDEC_CODEC_MPEG4_ASP,	VDC_CODEC_MPEG4_ASP,	},
			{ LX_VDEC_CODEC_XVID,		VDC_CODEC_XVID,		},
			{ LX_VDEC_CODEC_DIVX3,		VDC_CODEC_DIVX3,	},
			{ LX_VDEC_CODEC_DIVX4,		VDC_CODEC_DIVX4,	},
			{ LX_VDEC_CODEC_DIVX5,		VDC_CODEC_DIVX5,	},
			{ LX_VDEC_CODEC_RVX,		VDC_CODEC_RVX,		},
			{ LX_VDEC_CODEC_AVS,		VDC_CODEC_AVS,		},
			{ LX_VDEC_CODEC_VP8,		VDC_CODEC_VP8,		},
			{ LX_VDEC_CODEC_THEORA,		VDC_CODEC_THEORA,	},
			{ LX_VDEC_CODEC_HEVC,		VDC_CODEC_HEVC,		},
		};
		int a;

		for (a=0; a<ARRAY_SIZE(cs); a++)
		{
			if (cs[a].c1 == vdec->codec)
			{
				codec = cs[a].c2;
				break;
			}
		}
		if (a == ARRAY_SIZE(cs))
		{
			logm_warning (vdec, "vdec:%p, unknown codec %d\n",
					vdec, vdec->codec);
			return -1;
		}

		logm_noti (vdec, "vdec:%p, codec %s\n", vdec, codec_name(vdec->codec));
	}

	/* open decoder */
	memset (&stVdcOpenParam, 0, sizeof (stVdcOpenParam));
	stVdcOpenParam.ui8VesCh = vdec->ves;
	stVdcOpenParam.priv = vdec;
	stVdcOpenParam.ui8Src = vdec->src;
	stVdcOpenParam.ui8Vcodec = codec;
	stVdcOpenParam.ui32CpbBufAddr = vdec->cpb_addr;
	stVdcOpenParam.ui32CpbBufSize = vdec->cpb_size;
	stVdcOpenParam.clear_q = vdec->clear_q;
	stVdcOpenParam.decoded_q = &vdec->decoded_q;
	if (vdec->frame_requested == 1)
	{
		stVdcOpenParam.eOpenConf |= VDC_SINGLE_FRAME_DECODE;
	}
	if (vdec->frame_requested == 1 || vdec->opmode == LX_VDEC_OPMOD_LOW_LATENCY)
	{
		stVdcOpenParam.eOpenConf |= VDC_NO_DELAY;
	}
	if (vdec->flags & lx_vdec_flags_uhd_seemless)
	{
		stVdcOpenParam.eOpenConf |= VDC_ULTRA_HD;
	}
	if (vdec->dest < LX_VDEC_DST_BUFF && !vdec->use_gstc)
		stVdcOpenParam.eOpenConf |= VDC_LIVE_CHANNEL;
	logm_noti (vdec, "vdec:%p, lowdelay%d, uhd%d, live%d\n",
			vdec,
			!!(stVdcOpenParam.eOpenConf&VDC_NO_DELAY),
			!!(stVdcOpenParam.eOpenConf&VDC_ULTRA_HD),
			!!(stVdcOpenParam.eOpenConf&VDC_LIVE_CHANNEL)
			);

	if (vdec_test_copy_mode)
	{
		logm_noti (vdec, "vdec:%p, vdec_test_copy_mode\n", vdec);
		vdec->output_memory_format = LX_VDEC_MEMORY_FORMAT_RASTER;
		vdec->flags |= lx_vdec_flags_copy_dpb;
	}

	switch (vdec->output_memory_format)
	{
	default:
		// fallthrough
	case LX_VDEC_MEMORY_FORMAT_RASTER:
		stVdcOpenParam.output_memory_format = VDC_MEMORY_FORMAT_RASTER;
		if (vdec_test_user_alloc)
			stVdcOpenParam.extern_linear_buf_mode =
				VDC_LINEAR_MODE_STATIC;
		else if (vdec->flags & lx_vdec_flags_copy_dpb)
			stVdcOpenParam.extern_linear_buf_mode =
				VDC_LINEAR_MODE_DYNAMIC;
		else
			stVdcOpenParam.extern_linear_buf_mode =
				VDC_LINEAR_MODE_NONE;

		logm_debug (vdec, "vdec:%p, raster, buffer mode %d\n",
				vdec, stVdcOpenParam.extern_linear_buf_mode);
		break;

	case LX_VDEC_MEMORY_FORMAT_TILED:
		stVdcOpenParam.output_memory_format = VDC_MEMORY_FORMAT_TILED;
		stVdcOpenParam.extern_linear_buf_mode = VDC_LINEAR_MODE_NONE;

		logm_debug (vdec, "vdec:%p, tiled, buffer mode %d\n",
				vdec, stVdcOpenParam.extern_linear_buf_mode);
		break;
	}

	vdec->vdc = VDC_Open(&stVdcOpenParam);
	if( vdec->vdc < 0)
	{
		logm_warning (vdec, "vdec:%p, vdc open failed\n", vdec);
		return -1;
	}
	VDC_SetUserDataOutput(vdec->vdc, TRUE);
	if (vdec->scan_mode != LX_VDEC_PIC_SCAN_ALL)
		set_scan_picture (vdec, vdec->scan_mode);

	return 0;
}


static int init_submodules_locked (vdec_t *vdec)
{
	if (init_input (vdec) < 0)
		return -1;

	/* open output device */
	if (vdec->dest < LX_VDEC_DST_BUFF)
	{
		int deid;
		enum output_clock_source clock_src;

		/*
		 * check input parameters
		 */

		if (vdec->trid_type == vo_3d_type_dual)
			logm_noti (vdec, "vdec:%p, dual decoding\n", vdec);


		/* check match mode */
		if (vdec->opmode == LX_VDEC_OPMOD_LOW_LATENCY)
			vdec->match_mode = output_pts_freerun_ignore_sync;

		logm_noti (vdec, "vdec:%p, match mode %d\n",
				vdec, vdec->match_mode);

		/* check clock source */
		if( vdec->use_gstc )
			clock_src = output_clock_gstc;
		else if (LX_VDEC_SRC_SDEC0 <= vdec->src &&
				vdec->src <= LX_VDEC_SRC_SDEC2)
			clock_src = output_clock_stc0 + vdec->src;
		else
		{
			logm_warning (vdec, "vdec:%p, "
					"use STC while input source %s\n",
					vdec, src_name(vdec->src));
			return -1;
		}

		switch (vdec->dest)
		{
		default:
			logm_error (vdec, "vdec:%p, unknown dest %d\n",
					vdec, vdec->dest);
			// fallthrough
		case LX_VDEC_DST_DE0: deid = 0; break;
		case LX_VDEC_DST_DE1: deid = 1; break;
		}


		/*
		 * open sync
		 */
		if (init_output (vdec, deid, clock_src) < 0)
			return -1;

		if ((vdec_test_dual_sync || (vdec->flags & lx_vdec_flags_uhd_seemless)) &&
					deid == 0)
		{
			int sync1;
			unsigned long flags;

			logm_noti (vdec, "vdec:%p, open dual sync\n",
					vdec);

			sync1 = vdec->sync;

			if (init_output (vdec, 1, clock_src) < 0)
			{
				logm_warning (vdec, "vdec:%p, sync2 open failed.\n",
						vdec);

				vdec->sync = sync1;
				return -1;
			}

			output_bind (sync1, vdec->sync);

			vdec->sync = sync1;

			spin_lock_irqsave (&vdec->flags_lock, flags);
			vdec->flags |= FLAGS_INTER_DUAL_OUT;
			spin_unlock_irqrestore (&vdec->flags_lock, flags);

#if defined (CHIP_NAME_d14)
			VDISP_SeamlessModeNoti(1);
#endif
		}
#if defined (CHIP_NAME_d14)
		else
			VDISP_SeamlessModeNoti(0);
#endif


#if defined(CHIP_NAME_h13) && (CHIP_REV<0xb0)
		if( vdec->src <= LX_VDEC_SRC_SDEC2 )
		{
			TOP_HAL_SetLQInputSelection(vdec->sync, vdec->src);
			TOP_HAL_EnableLQInput(vdec->sync);
		}
		else
		{
			TOP_HAL_DisableLQInput(vdec->sync);
		}
#endif
	}

	if (init_codec (vdec) < 0)
		return -1;

	logm_noti (vdec, "opened vdc %d\n", vdec->vdc);
	logm_noti (vdec, "opened ves %d\n", vdec->ves);
	logm_noti (vdec, "opened sync %d\n", vdec->sync);

	return 0;
}


static int init_submodules (vdec_t *vdec)
{
	int ret;

#define printvar(f,v)	logm_noti (vdec, #v": %"#f"\n", v)
	printvar (p, vdec);
	printvar (d, vdec->src);
	printvar (d, vdec->dest);
	printvar (d, vdec->output_memory_format);
	printvar (d, vdec->opmode);
	printvar (d, vdec->frame_requested);
#undef printvar

	if (vdec_force_raster_memory)
	{
		logm_noti (vdec, "vdec:%p, force to use raster memory\n",
				vdec);
		vdec->output_memory_format = LX_VDEC_MEMORY_FORMAT_RASTER;
	}

	if (vdec->use_gstc)
		logm_noti (vdec, "vdec:%p, use gstc\n", vdec);
	if (vdec->src == LX_VDEC_SRC_BUFF && !vdec->use_gstc)
	{
		logm_noti (vdec, "vdec:%p, use STC while memory input source. use GSTC!\n",
				vdec);
		vdec->use_gstc = 1;
	}

	mutex_lock (&io_lock);
	ret = init_submodules_locked (vdec);
	mutex_unlock (&io_lock);

	if(ret < 0)
	{
		logm_warning (vdec, "vdec:%p, failed to open ch ret %d\n",
				vdec, ret);

		destroy_submodules (vdec);

		return -1;
	}

	logm_noti (vdec, "vdec:%p\n", vdec);
	return 0;
}

static void hardware_reset(vdec_t *vdec)
{
	/*
	 * FIXME
	 *
	 * not yet implemented.
	 */

	S_DISPCLEARQ_BUF_T buf;

	logm_warning (vdec, "vdec:%p, do decoder reset\n", vdec);

	if (vdec->ves != 0xff)
	{
		//VES_Reset(vdec->ves);
		//VES_Flush(vdec->ves);
	}

	if (vdec->vdc >= 0)
		VDC_Reset(vdec->vdc);

	if (vdec->sync >= 0)
		output_cmd (vdec->sync, output_cmd_reset, 0);

	/* empty cleal_q */
	while (DISP_CLEAR_Q_Pop (vdec->clear_q, &buf) == TRUE);
}


static int _flush(vdec_t *vdec)
{
	int ret = 0;

	logm_noti (vdec, "vdec:%p\n", vdec);

	mutex_lock (&io_lock);

	vdec->flushed = 1;
	if (vdec->vdc >= 0){
		VDC_Stop(vdec->vdc);
		VDC_Flush(vdec->vdc);
	}
	if (vdec->ves != 0xff)
	{
		VES_Flush(vdec->ves);
	}

	if (
			vdec->use_gstc &&
			vdec->port_adec >= 0
	   )
	{
		unsigned int a = -1;

		/* clear base time */
		AV_Set_AVLipSyncBase (vdec->port, vdec->port_adec,
				&a, &a);
	}

	/* cut gstc again */
	vdec->use_gstc_have_cut = 0;

	if (vdec->sync >= 0)
		output_cmd (vdec->sync, output_cmd_flush, 0);

	//VDC_CheckFlushed(vdec->vdc);
	decoded_queue_flush (&vdec->decoded_q);

	if (vdec->vdc >= 0)
		VDC_Start(vdec->vdc);

	mutex_unlock (&io_lock);

	return (ret);
}

static int get_buffer_status(vdec_t *vdec, LX_VDEC_IO_BUFFER_STATUS_T *status)
{
	mutex_lock (&vdec->submodule_lock);

	if (vdec->ves == 0xff)
	{
		logm_debug (vdec, "no channel. use default\n");

		if (vdec->cpb_size)
			status->cpb_size = vdec->cpb_size;
		else
			status->cpb_size = vdec_default_cpb_size;
		status->cpb_depth = 0;
		status->auib_size = VES_AUIB_NUM_OF_NODE;
		status->auib_depth = 0;
	}
	else
	{
		status->cpb_size = VES_CPB_GetBufferSize(vdec->ves);
		status->cpb_depth = VES_CPB_GetUsedBuffer(vdec->ves);
		status->auib_size = VES_AUIB_NUM_OF_NODE;
		status->auib_depth = VES_AUIB_NumActive(vdec->ves);
	}

	mutex_unlock (&vdec->submodule_lock);

	return 0;
}

static int update_buffer(vdec_t *vdec, LX_VDEC_IO_UPDATE_BUFFER_T *buffer)
{
	int ret = 0;
	wait_queue_head_t *wq;

	if (vdec->vdc < 0)
	{
		logm_warning (vdec, "vdec:%p, no vdc\n", vdec);
		return -EFAULT;
	}

	if(buffer->au_size==0)
	{
		logm_warning (vdec, "vdec:%p, no data\n", vdec);
		return -EFAULT;
	}

	/* flush check */
	if (vdec->flushed)
		vdec->flushed = 0;

	wq = VES_AUIB_GetWaitQueueHead (vdec->ves);
	ret = wait_event_interruptible (*wq,
			vdec->decoding_queue_size > VES_CPB_GetUsedBuffer (vdec->ves) &&
			vdec->decoding_queue_slots > VES_AUIB_NumActive (vdec->ves));
	if (ret < 0)
		return ret;

	/* check this buffer should be flushed */
	if (vdec->flushed)
	{
		logm_debug (vdec, "vdec:%p, flush this buffer\n",
				vdec);
		vdec->flushed = 0;
		return 0;
	}

	mutex_lock (&io_lock);

	if (logm_enabled (vdec, log_level_debug))
	{
		char tmp[64];
		int a, len = 16;
		int qlen;

		qlen = VES_AUIB_NumActive (vdec->ves);

		if (len > buffer->au_size)
			len = buffer->au_size;

		if (vdec->src != LX_VDEC_SRC_BUFFTVP)
		{
			for (a=0; a<len; a++)
				sprintf (tmp+a*3, "%02x ",
						buffer->au_ptr[a]);
			tmp[a*3] = 0;
		}
		else
			tmp[0] = 0;

		logm_debug (vdec, "vdec:%p, qlen:%d, %llu(%08llx), data(%d):%s\n", vdec,
				qlen,
				buffer->timestamp, buffer->timestamp,
				buffer->au_size,
				tmp);
	}

	if (vdec->ves != 0xff)
	{
		if (!access_ok (VERIFY_READ, buffer->au_ptr, buffer->au_size))
		{
			logm_warning (vdec, "Oops\n");
			ret = -EFAULT;
		}
		else
		{
			E_PDEC_AU_T au_type;
			int ret;

			switch (buffer->au_type)
			{
			default:
				logm_warning (vdec, "vdec:%p, wrong au type %d\n",
						vdec, buffer->au_type);

				// fallthrough
			case LX_VAU_DATA:
				//au_type = AU_PICTURE_I;
				au_type = AU_HEVC_SLICE_SEGMENT;	// workaround for TVP decoder need au type HEVC : 5, others : 3 ~ 6
				break;
			case LX_VAU_SEQH:
				au_type = AU_SEQUENCE_HEADER;
				break;
			case LX_VAU_SEQE:
				au_type = AU_SEQUENCE_END;
				break;
			}

			ret = VES_UpdatePicture(vdec->ves,
					au_type,
					(unsigned int)buffer->au_ptr,
					buffer->au_size,
					buffer->UId,
					buffer->timestamp);

			if (ret == FALSE)
			{
				logm_warning (vdec, "Oops\n");
				ret = -EBUSY;
			}
		}
	}

	mutex_unlock (&io_lock);

	return (ret);
}

static int do_eos (vdec_t *vdec)
{
	unsigned long flags;
	int ret;

	logm_info (vdec, "vdec:%p, do eos.\n", vdec);

	if (vdec->ves == 0xff)
	{
		logm_warning (vdec, "vdec:%p, no ves\n", vdec);
		return -EINVAL;
	}

	spin_lock_irqsave (&vdec->flags_lock, flags);
	vdec->flags |= FLAGS_INTER_WAIT_EOS;
	spin_unlock_irqrestore (&vdec->flags_lock, flags);

	ret = VES_UpdatePicture (vdec->ves, AU_EOS,
			0, 0, 0, VDEC_UNKNOWN_TIMESTAMP);

	return ret;
}

static void VDEC_IO_UpdateCpbStatus(S_IPC_CALLBACK_BODY_CPBSTATUS_T *pCpbStatus)
{
	vdec_t *vdec = (vdec_t *)pCpbStatus->priv;

	logm_debug (vdec, "vdec:%p, cpb status %d\n", vdec, pCpbStatus->eBufStatus);
	if (vdec == NULL)
	{
		logm_warning (vdec, "no vdec???\n");
		return;
	}

	switch( pCpbStatus->eBufStatus )
	{
	case CPB_STATUS_ALMOST_FULL :
		VES_Flush(vdec->ves);
		break;
	case CPB_STATUS_NORMAL :
	case CPB_STATUS_ALMOST_EMPTH :
	default :
		break;
	}
}

static void VDEC_IO_RequestCmdReset(S_IPC_CALLBACK_BODY_REQUEST_CMD_T *pReqCmd)
{
	vdec_t *vdec = (vdec_t *)pReqCmd->priv;

	logm_warning (vdec, "vdec:%p, %d, %08x, %d\n", vdec,
			pReqCmd->bReset,
			pReqCmd->ui32Addr,
			pReqCmd->ui32Size);

	if (vdec->vdc < 0)
	{
		logm_warning (vdec, "no channel\n");
		return;
	}

	mutex_lock (&io_lock);
	//if(pReqCmd->bReset)
	//	reset(vdec);

	if(pReqCmd->ui32Addr == 0 || pReqCmd->ui32Size ==0)
		logm_debug (vdec, "vdec:%p, no seq data\n", vdec);
	else
	{
		logm_debug (vdec, "vdec:%p, seq data %08x, %08x\n", vdec,
				pReqCmd->ui32Addr,
				pReqCmd->ui32Size);

		if(
				vdec->ves != 0xff &&
				VES_UpdatePicture(vdec->ves,
					AU_SEQUENCE_HEADER,
					(unsigned int)pReqCmd->ui32Addr,
					pReqCmd->ui32Size,
					0 /*ioUpdateBuffer.UId*/ ,
					-1 /*ioUpdateBuffer.timestamp*/
					) == FALSE
		  )
			logm_warning (vdec, "Oops\n");
	}
	mutex_unlock (&io_lock);
}


#ifdef SUPPORT_ION
static unsigned long ion2phys (vdec_t *vdec, int ion)
{
	struct ion_handle *handle;
	ion_phys_addr_t addr;
	size_t size;
	int ret;

	if (!vdec->ion_client)
	{
		extern struct ion_client *lg115x_ion_client_create(unsigned int heap_mask,
				const char *name);

		logm_noti (vdec, "vdec:%p, make ion client\n", vdec);
		vdec->ion_client = lg115x_ion_client_create (0, "vdec");
	}
	if (!vdec->ion_client)
	{
		logm_warning (vdec, "vdec:%p, no ion client\n", vdec);
		return -EFAULT;
	}

	handle = ion_import_dma_buf (vdec->ion_client, ion);
	if (!handle)
	{
		logm_warning (vdec, "vdec:%p, no handle for fd %d\n", vdec, ion);
		return -EFAULT;
	}

	ret = ion_phys (vdec->ion_client, handle,
			&addr, &size);
	if (ret < 0)
	{
		logm_warning (vdec, "vdec:%p, ion_phys failed. %d\n",
				vdec, ret);
		return ret;
	}
	logm_info (vdec, "vdec:%p, dpb %lx(%d) from ion\n", vdec,
			addr, size);

	return addr;
}
#else
#define ion2phys(vdec,ion)	(0UL)
#endif


static int set_framebuffer (vdec_t *vdec, LX_VDEC_FB_T *fb)
{
	struct dpb *dpb;
	unsigned long flag;

	if (!(vdec->flags & lx_vdec_flags_user_dpb))
	{
		logm_warning (vdec, "vdec:%p, we dont need dpb\n",
				vdec);
		return -EINVAL;
	}

#if 0
	if (vdec->flags & lx_vdec_flags_copy_dpb)
	{
		logm_debug (vdec, "vdec:%p, add dpb to copy list\n",
				vdec);
		/* TODO: not implemented yet */

		return 0;
	}

	if (!vdec->wait_dpb)
	{
		logm_warning (vdec, "vdec:%p, not waiting state.\n",
				vdec);

		return -EINVAL;
	}
#endif

	if (!fb->y && !fb->ion_y)
	{
		logm_debug (vdec, "vdec:%p, set dpbs\n", vdec);
		set_dpb (vdec);

		return 0;
	}

#if 0
	if (
			vdec->wait_dpb->width != fb->width ||
			vdec->wait_dpb->height != fb->height
	   )
	{
		logm_warning (vdec, "vdec:%p, wrong size\n",
				vdec);
		return -EINVAL;
	}
#endif

	logm_debug (vdec, "vdec:%p, add dpb y:%lx, iy:%d\n", vdec,
			fb->y, fb->ion_y);

	dpb = kzalloc (sizeof (*dpb), GFP_ATOMIC);
	if (!dpb)
	{
		logm_warning (vdec, "vdec:%p, no mem\n",
				vdec);
		return -ENOMEM;
	}

	dpb->fb = *fb;
	/* get from ion memory */
	if (dpb->fb.ion_y)
		dpb->fb.y = ion2phys (vdec, dpb->fb.ion_y);

	logm_debug (vdec, "vdec:%p, set %lx, %dx%d(%d)\n",
			vdec, dpb->fb.y, dpb->fb.width, dpb->fb.height,
			dpb->fb.stride);
	spin_lock_irqsave (&vdec->dpb_lock, flag);
	dpb->next = vdec->dpb;
	vdec->dpb = dpb;
	spin_unlock_irqrestore (&vdec->dpb_lock, flag);

	return 0;
}


static int convert_frame (vdec_t *vdec, LX_VDEC_CONVERT_FRAME_T *c)
{
	unsigned long flag;
	struct decoded_info *info;

	if (vdec->vdc  < 0)
	{
		logm_warning (vdec, "vdec:%p, no decoder\n", vdec);
		return -1;
	}

	/* search matching info */
	spin_lock_irqsave (&vdec->decoded_buffers_lock, flag);
	info = vdec->decoded_infos;
	while (info && info->buffer.addr_y != c->y)
		info = info->next;
	spin_unlock_irqrestore (&vdec->decoded_buffers_lock, flag);

	if (!info)
	{
		logm_warning (vdec, "vdec:%p, unknown buffer. y %lx\n",
				vdec, c->y);
		return -1;
	}

	VDC_GetFrame (vdec->vdc,
			info->buffer.buffer_index, c->result);

	return 0;
}


static int set_speed (vdec_t *vdec, unsigned long speed)
{
	unsigned long flag;

	if (vdec->sync < 0)
	{
		logm_warning (vdec, "vdec:%p, we dont have output\n",
				vdec);
		return -1;
	}

	spin_lock_irqsave (&vdec->speed_change, flag);
	vdec->speed = speed;

	if (speed == 0)
	{
		output_cmd (vdec->sync, output_cmd_stop, 0);
		if (vdec->use_gstc && vdec->port_adec >= 0)
		{
			unsigned int na = -1;

			logm_noti (vdec, "vdec:%p, "
					"reset basetime, adec%d\n",
					vdec, vdec->port_adec);

			AV_Set_AVLipSyncBase (vdec->port, vdec->port_adec,
					&na, &na);
		}

		/* cut gstc again */
		vdec->use_gstc_have_cut = 0;
	}
	else
		output_cmd (vdec->sync, output_cmd_start, FALSE);

	output_cmd (vdec->sync, output_cmd_speed, speed*1000/0x10000);
	spin_unlock_irqrestore (&vdec->speed_change, flag);

	logm_noti (vdec,"vdec:%p, sync %d, speed %x(%d)\n",
			vdec, vdec->sync,
			(int)speed, (int)speed*1000/0x10000);

	return 0;
}


static int set_flags (vdec_t *vdec, unsigned int flags)
{
	unsigned long f;

	logm_debug (vdec, "vdec:%p, set flags %08x on %08x\n",
			vdec, flags, vdec->flags);

	spin_lock_irqsave (&vdec->flags_lock, f);
	flags &= ~FLAGS_INTER_MASK;
	vdec->flags = (vdec->flags&FLAGS_INTER_MASK) | flags;
	spin_unlock_irqrestore (&vdec->flags_lock, f);

	return 0;
}


static int add_flags (vdec_t *vdec, unsigned int flags)
{
	unsigned long f;

	logm_debug (vdec, "vdec:%p, add flags %08x on %08x\n",
			vdec, flags, vdec->flags);

	spin_lock_irqsave (&vdec->flags_lock, f);
	vdec->flags |= flags;
	spin_unlock_irqrestore (&vdec->flags_lock, f);

	return 0;
}


static int del_flags (vdec_t *vdec, unsigned int flags)
{
	unsigned long f;

	logm_debug (vdec, "vdec:%p, del flags %08x on %08x\n",
			vdec, flags, vdec->flags);

	spin_lock_irqsave (&vdec->flags_lock, f);
	vdec->flags &= ~flags;
	spin_unlock_irqrestore (&vdec->flags_lock, f);

	return 0;
}


static int vdec_users;
static int open_count;

/*
 * open handler for vdec device
 */
vdec_t *vdec_open (struct file *filp)
{
	vdec_t *vdec;
	int ret;

	vdec = kcalloc (1, sizeof(*vdec), GFP_KERNEL);
	if (vdec == NULL)
	{
		logm_error (vdec, "kcmalloc failed\n");
		return NULL;
	}

	vdec_users ++;
	logm_noti (vdec, "vdec:%p, opening.. user %d\n",
			vdec,
			vdec_users);

	filp->private_data = vdec;

	vdec->file = filp;

	/* default values */
	ret = noti_alloc (&vdec->noti, 64*1024);
	if (ret < 0)
	{
		kfree (vdec);
		return NULL;
	}

	vdec->codec = LX_VDEC_CODEC_INVALID;
	vdec->src = LX_VDEC_SRC_BUFF;
	vdec->dest = LX_VDEC_DST_BUFF;
	vdec->output_memory_format = LX_VDEC_MEMORY_FORMAT_TILED;
	vdec->id = (unsigned long)-1;
	vdec->port_adec = -1;
	vdec->port = -1;

	vdec->ves = 0xff;
	vdec->vdc = -1;
	vdec->sync = -1;

	vdec->clear_q = DISP_CLEAR_Q_Open (32);
	vdec->opmode = LX_VDEC_OPMOD_NORMAL;
	vdec->framerate_num = 0;

	vdec->display_q_maxsize = 0x20;

	/* decoding q */
	vdec->decoding_queue_size = INT_MAX;
	vdec->decoding_queue_slots = INT_MAX;

	/* decoded q */
	vdec->decoded_q.push = decoded_queue_push;
	vdec->decoded_q.max_size = decoded_queue_max_size;
	vdec->decoded_q.size = decoded_queue_size;

	/* display part */
	vdec->match_mode = output_pts_enable;

	spin_lock_init (&vdec->decoded_buffers_lock);
	mutex_init (&vdec->submodule_lock);
	spin_lock_init (&vdec->submodule_spin_lock);
	spin_lock_init (&vdec->submodule_vdc_spin_lock);
	spin_lock_init (&vdec->dpb_lock);
	spin_lock_init (&vdec->flags_lock);

	/* dpbdump q */
	spin_lock_init (&vdec->dpbdump_lock);

	initialize_instance (vdec);

	/* initialize proc directory */
	sprintf (vdec->proc_name, "%08d_%p", open_count ++, vdec);
	logm_debug (vdec, "vdec:%p, proc dir name %s\n",
			vdec, vdec->proc_name);
	proc_make_instance (vdec);

	/* add to device list */
	vdec->next = device_list;
	device_list = vdec;

	logm_trace (vdec, "vdec:%p, opened\n", vdec);

	return vdec;
}

/*
 * release handler for vdec device
 */
int vdec_close (vdec_t *vdec)
{
	vdec_t *now;
	unsigned long flag;

	logm_trace (vdec, "vdec:%p, closing...\n",
			vdec);
	/* del from device list */
	spin_lock_irqsave (&device_list_lock, flag);
	if (device_list == vdec)
		device_list = vdec->next;
	else
	{
		now = device_list;
		while (now->next != NULL && now->next != vdec)
			now = now->next;
		if (now->next == vdec)
			now->next = vdec->next;
		else
			logm_error (vdec, "vdec:%p, BUG???\n", vdec);
	}
	spin_unlock_irqrestore (&device_list_lock, flag);

	vdec_users --;
	logm_noti (vdec, "vdec:%p, users %d\n", vdec, vdec_users);

	/* remove proc entry */
	proc_rm_instance (vdec);

	/* destroy submodules */
	destroy_submodules (vdec);

	/* make sure the workqueue empty */
	flush_workqueue (vdec_wq);

	noti_free (&vdec->noti);
	DISP_CLEAR_Q_Close (vdec->clear_q);

	logm_noti (vdec, "vdec:%p, closed.\n", vdec);
	kfree (vdec);

	return 0;
}

/*
 * ioctl handler for vdec device.
 */
long vdec_ioctl (vdec_t *vdec, unsigned int cmd, unsigned long arg, int from_user)
{
	int ret = 0;

	union
	{
		LX_VDEC_IO_BUFFER_STATUS_T buffer_status;
		LX_VDEC_IO_UPDATE_BUFFER_T update_buffer;
		unsigned long unref_decodebuffer;
		unsigned int enable_log;
		int queue_size;
		LX_VDEC_MEMORY_FORMAT_T memory_format;
		LX_VDEC_BASETIME_T basetime;
		LX_VDEC_STC_T stc;
		LX_VDEC_CPB_INFO_T cpb_info;
		LX_VDEC_BUFFER_T buffer;
		LX_VDEC_FB_T fb;
		LX_VDEC_CONVERT_FRAME_T convert_frame;
	} a;

	logm_trace (vdec, "vdec:%p, cmd = %08x (cmd_idx=%d)\n",
			vdec, cmd, _IOC_NR(cmd));

	if (_IOC_TYPE(cmd) != LX_VDEC_IOC_MAGIC)
	{
		logm_warning (vdec, "invalid magic. magic=0x%02x\n",
				_IOC_TYPE(cmd) );
		return -ENOIOCTLCMD;
	}

	if (_IOC_DIR(cmd) & _IOC_WRITE)
	{
		int r;

		if (from_user)
		{
			r = copy_from_user (&a, (void*)arg, _IOC_SIZE(cmd));
			if (r)
			{
				logm_warning (vdec, "copy_from_user failed. "
						"cmd %08x, arg %08lx\n",
						cmd, arg);
				return -EFAULT;
			}
		}
		else
			memcpy (&a, (void*)arg, _IOC_SIZE(cmd));
	}

	switch(cmd)
	{
	case LX_VDEC_INIT:
		if (vdec->vdc >= 0)
		{
			logm_warning (vdec, "vdec:%p, already initialized\n",
					vdec);
			break;
		}

		ret = init_submodules (vdec);

		if (ret < 0)
		{
			logm_warning (vdec, "vdec:%p, "
					"cannot initialize submodules\n",
					vdec);
			return ret;
		}
		VES_Start(vdec->ves);
		VDC_Start(vdec->vdc);

		break;

	case LX_VDEC_DESTROY:
		destroy_submodules (vdec);
		break;

	case LX_VDEC_IO_FLUSH:
		ret = _flush(vdec);
		break;

	case LX_VDEC_IO_GET_BUFFER_STATUS:
		ret = get_buffer_status(vdec, &a.buffer_status);
		break;

	case LX_VDEC_IO_UPDATE_BUFFER:
		ret = update_buffer(vdec, &a.update_buffer);
		break;

	case LX_VDEC_EOS:
		ret = do_eos (vdec);
		break;

	case LX_VDEC_UNREF_DECODEBUFFER:
		mutex_lock (&vdec->submodule_lock);
		ret = unref_frame_and_kick_decoder_locked (vdec,
				a.unref_decodebuffer);
		mutex_unlock (&vdec->submodule_lock);
		break;

	case LX_VDEC_SET_DECODING_QUEUE_SIZE:
		vdec->decoding_queue_slots = a.queue_size;
		break;

	case LX_VDEC_SET_OUTPUT_MEMORY_FORMAT:
		if (vdec->vdc >= 0)
		{
			logm_warning (vdec, "vdec:%p, setting memory format after vdc open\n",
					vdec);
			ret = -EBUSY;
			break;
		}

		vdec->output_memory_format = a.memory_format;
		logm_debug (vdec, "vdec:%p, output_memory_format %d\n",
				vdec, vdec->output_memory_format);
		break;

	case LX_VDEC_SET_SCAN_PICTURE:
		ret = set_scan_picture (vdec, arg);
		break;

	case LX_VDEC_GET_DECODED_QUEUE_SIZE:
		{
			int head, tail;

			head = vdec->noti.head;
			tail = vdec->noti.tail;

			if (head >= tail)
				a.queue_size = head - tail;
			else
				a.queue_size = vdec->noti.size - tail + head;

			logm_trace (vdec, "vdec:%p, decoded queue size %d\n",
					vdec, a.queue_size);
		}
		break;

	case LX_VDEC_SET_CODEC:
		vdec->codec = arg;
		logm_debug (vdec, "vdec:%p, set codec %d\n", vdec,
				vdec->codec);
		break;

	case LX_VDEC_SET_DISPLAY_OFFSET:
		vdec->display_offset = arg;
		logm_info (vdec, "vdec;%p, set display offset %dms\n",
				vdec, vdec->display_offset);
		if (vdec->sync >= 0)
		{
			logm_debug (vdec, "vdec;%p, set to VDISP\n", vdec);
			output_cmd (vdec->sync, output_cmd_display_offset,
					vdec->display_offset);
		}
		break;

	case LX_VDEC_SET_INPUT_DEVICE:
		vdec->src = arg;
		break;

	case LX_VDEC_SET_OUTPUT_DEVICE:
		vdec->dest = arg;
		break;

	case LX_VDEC_SET_BASETIME:
		ret = set_basetime (vdec,
				a.basetime.base_stc, a.basetime.base_pts);
		break;

	case LX_VDEC_GET_GLOBAL_STC:
		a.stc.stc = TOP_HAL_GetGSTCC();
		a.stc.mask = 0xffffffff;
		logm_debug (vdec, "vdec:%p, stc %08x\n", vdec, a.stc.stc);
		break;

	case LX_VDEC_SET_SPEED:
		ret = set_speed (vdec, arg);
		break;

	case LX_VDEC_SET_STEP:
		if (vdec->sync < 0)
		{
			logm_warning (vdec, "vdec:%p, we dont have output\n",
					vdec);
			break;
		}

		output_cmd (vdec->sync, output_cmd_start, TRUE);
		break;

	case LX_VDEC_SET_ID:
		vdec->id = arg;
		logm_debug (vdec, "vdec:%p, id %ld\n",
				vdec, vdec->id);
		break;

	case LX_VDEC_STEAL_USERDATA:
		vdec->userdata_thief = !!arg;
		logm_debug (vdec, "vdec:%p, userdata thief %d\n",
				vdec, vdec->userdata_thief);
		break;

	case LX_VDEC_SET_LOW_LATENCY:
		if (vdec->sync >= 0)
		{
			logm_warning (vdec, "vdec:%p, we have sync already\n",
					vdec);
			break;
		}

		if (arg)
			vdec->opmode = LX_VDEC_OPMOD_LOW_LATENCY;
		else
			vdec->opmode = LX_VDEC_OPMOD_NORMAL;
		logm_debug (vdec, "vdec:%p, set low latency %d\n",
				vdec, vdec->opmode);

		break;

	case LX_VDEC_SET_3D_TYPE:
		logm_debug (vdec, "vdec:%p, set 3d type %ld\n",
				vdec, arg);
		if (arg > vo_3d_type_none)
		{
			logm_warning (vdec, "vdec:%p, unknown 3d type %08lx\n",
					vdec, arg);
			ret = -EINVAL;
			break;
		}
		vdec->trid_type = arg;
		break;

	case LX_VDEC_SET_CPB:
		if (vdec->cpb_addr)
		{
			logm_warning (vdec, "vdec:%p, we have cpb already.\n",
					vdec);
			ret = -EBUSY;
			break;
		}
		logm_debug (vdec, "vdec:%p, set cpb %08lx, %d\n", vdec,
				a.buffer.addr, a.buffer.size);
		vdec->cpb_addr = a.buffer.addr;
		vdec->cpb_size = a.buffer.size;
		break;

	case LX_VDEC_GET_CPB_INFO:
		a.cpb_info.addr = vdec->cpb_addr;
		a.cpb_info.size = vdec->cpb_size;
		if (vdec->ves != 0xff)
		{
			a.cpb_info.read_offset =
				VES_CPB_GetPhyRdPtr (vdec->ves) -
				vdec->cpb_addr;
			a.cpb_info.write_offset =
				VES_CPB_GetPhyWrPtr (vdec->ves) -
				vdec->cpb_addr;
		}
		else
		{
			a.cpb_info.read_offset = 0;
			a.cpb_info.write_offset = 0;
		}

		logm_debug (vdec, "vdec:%p, cpb info %08lx(%x), %d, %d\n",
				vdec,
				a.cpb_info.addr,
				a.cpb_info.size,
				a.cpb_info.read_offset,
				a.cpb_info.write_offset);
		break;

	case LX_VDEC_SET_REQUEST_PICTURES:
		vdec->frame_requested = arg;
		logm_debug (vdec, "vdec:%p, requested frame %d\n",
				vdec, vdec->frame_requested);
		break;

	case LX_VDEC_RESET:
		hardware_reset (vdec);
		logm_debug (vdec, "vdec:%p, do hardware_reset\n", vdec);
		break;

	case LX_VDEC_USE_GSTC:
		logm_debug (vdec, "vdec:%p, use gstc %ld\n", vdec, arg);
		vdec->use_gstc = !!arg;
		break;

	//case LX_VDEC_CONVERT_TO_RASTER:
	//	convert_to_raster (vdec, a.convert_to_raster);
	//	break;

	case LX_VDEC_SET_FLAGS:
		/* dangerous. internal flags can be cleared */
		set_flags (vdec, arg);
		break;

	case LX_VDEC_ADD_FLAGS:
		add_flags (vdec, arg);
		break;

	case LX_VDEC_DEL_FLAGS:
		del_flags (vdec, arg);
		break;

	case LX_VDEC_SET_FRAMERATE:
		logm_debug (vdec, "set framerate. %d/%d\n",
				(int)(arg>>16), (int)(arg&0xffff));
		if (vdec->sync >= 0)
		{
			output_cmd (vdec->sync,
					output_cmd_framerate_num, arg>>16);
			output_cmd (vdec->sync,
					output_cmd_framerate_den, arg&0xffff);
		}
		else
			logm_warning (vdec, "no sync\n");
		break;

	case LX_VDEC_DISPLAY_FREEZE:
		if (vdec->sync < 0)
		{
			logm_warning (vdec, "vdec:%p, no sync for freeze\n",
					vdec);
			ret = -EINVAL;
			break;
		}

		logm_debug (vdec, "vdec:%p, freeze %ld\n",
				vdec, arg);
		switch (arg)
		{
		case LX_VDEC_DISPLAY_FREEZE_UNSET:
			output_cmd (vdec->sync, output_cmd_freeze, 0);
			break;

		case LX_VDEC_DISPLAY_FREEZE_SET:
			output_cmd (vdec->sync, output_cmd_freeze, 1);
			break;
		}
		break;

	case LX_VDEC_DISPLAY_SYNC:
		logm_info (vdec, "vdec:%p, match mode %ld\n", vdec, arg);

		switch (arg)
		{
		default:
			logm_warning (vdec, "vdec:%p, unknown param, %ld\n",
					vdec, arg);
			// fallthrough
		case LX_VDEC_DISPLAY_SYNC_MATCH:
			vdec->match_mode = output_pts_enable;
			break;

		case LX_VDEC_DISPLAY_SYNC_FREERUN:
			vdec->match_mode = output_pts_freerun_based_sync;
			break;
		}

		if (vdec->sync >= 0)
		{
			logm_info (vdec, "vdec:%p, set sync, mode %d\n",
					vdec, vdec->match_mode);
			output_cmd (vdec->sync, output_cmd_pts_match_mode,
					vdec->match_mode);
		}
		break;

	case LX_VDEC_SET_CHANNEL_NUMBER:
		logm_info (vdec, "vdec:%p, set vdec port %ld\n",
				vdec, arg);
		vdec->port = arg;
		break;

	case LX_VDEC_SET_ADEC_CHANNEL:
		logm_info (vdec, "vdec:%p, set adec port %ld\n",
				vdec, arg);
		vdec->port_adec = arg;
		break;

	case LX_VDEC_SET_FRAMEBUFFER:
		ret = set_framebuffer (vdec, &a.fb);
		break;

	case LX_VDEC_CONVERT_FRAME:
		ret = convert_frame (vdec, &a.convert_frame);
		break;

	default:
		/* not supported more */
		logm_warning (vdec, "unknown ioctl\n");
		ret = -ENOIOCTLCMD;
		break;
	}

	if(ret < 0 && cmd != LX_VDEC_IO_GET_BUFFER_STATUS)
	{
		logm_warning (vdec, "ioctl failed (cmd:%d, ret:%d)\n",
				_IOC_NR(cmd),
				ret);
	}

	if (ret >= 0 && _IOC_DIR(cmd) & _IOC_READ)
	{
		int r;

		if (from_user)
		{
			r = copy_to_user ((void*)arg, &a, _IOC_SIZE(cmd));
			if (r)
			{
				logm_warning (vdec, "copy_to_user failed. "
						"cmd %08x, arg %08lx\n",
						cmd, arg);
				return -EFAULT;
			}
		}
		else
			memcpy ((void*)arg, &a, _IOC_SIZE(cmd));
	}

	logm_trace (vdec, "vdec:%p, cmd = %08x (cmd_idx=%d) ret %08x\n",
			vdec, cmd, _IOC_NR(cmd), ret);
	return ret;
}

static int open(struct inode *inode, struct file *filp)
{
	vdec_t *vdec;

	vdec = vdec_open (filp);
	if (vdec)
	{
		filp->private_data = vdec;
		return 0;
	}

	return -EINVAL;
}

static int close(struct inode *inode, struct file *file)
{
	return vdec_close (file->private_data);
}

static long ioctl (struct file *file, unsigned int cmd, unsigned long arg)
{
	return vdec_ioctl (file->private_data, cmd, arg, 1);
}

static unsigned int poll (struct file *file, struct poll_table_struct *pt)
{
	vdec_t *vdec = file->private_data;
	int ret = 0;
	wait_queue_head_t *wait_decoding;
	wait_queue_head_t *wait_decoder;
	VDC_CODEC_STATE_T decoder_state;

	mutex_lock (&vdec->submodule_lock);

	/* get wait queue from submodules */
	if (vdec->ves != 0xff)
		wait_decoding = VES_AUIB_GetWaitQueueHead (vdec->ves);
	else
		wait_decoding = NULL;

	if (vdec->vdc >= 0)
	{
		wait_decoder = NULL;
		decoder_state = VDC_GetCoreState (vdec->vdc, &wait_decoder);
	}
	else
	{
		decoder_state = VDC_CODEC_STATE_NULL;
		wait_decoder = NULL;
	}

	/* add the waits to poll table */
	poll_wait (file, &vdec->noti.wq, pt);
	if (wait_decoding)
		poll_wait (file, wait_decoding, pt);
	if (wait_decoder)
		poll_wait (file, wait_decoder, pt);

	/* check event */
	if (vdec->noti.head != vdec->noti.tail)
		ret |= POLLIN | POLLRDNORM;

	if (vdec->ves == 0xff)
	{
		ret |= POLLOUT | POLLWRNORM;

		if (vdec->flags & lx_vdec_flags_pollerr)
			ret |= POLLERR;
	}
	else if (
			vdec->decoding_queue_size > VES_CPB_GetUsedBuffer (vdec->ves) &&
			vdec->decoding_queue_slots > VES_AUIB_NumActive (vdec->ves)
		)
			ret |= POLLOUT | POLLWRNORM;

	if (
			vdec->ves != 0xff &&
			VES_AUIB_NumActive(vdec->ves) == 0 &&
			/*VES_CPB_GetUsedBuffer (vdec->ves) == 0 &&*/
			vdec->noti.head == vdec->noti.tail &&
			!(vdec->flags & FLAGS_INTER_WAIT_EOS) &&
			decoder_state == VDC_CODEC_STATE_READY
	   )
	{
		/* vdec doing nothing. all queues are empty. report error to
		 * the application the end of decoding.
		 */
		if (vdec->flags & lx_vdec_flags_pollerr)
			ret |= POLLERR;
	}

	mutex_unlock (&vdec->submodule_lock);

	return ret;
}

static ssize_t read( struct file *file,
			char __user *data,
			size_t size,
			loff_t *off  )
{
	vdec_t *vdec = file->private_data;
	int timeout;
	int ret;

	logm_trace (vdec, "vdec:%p, size %d\n", vdec, size);

	timeout = wait_event_interruptible_timeout (vdec->noti.wq,
			vdec->noti.head != vdec->noti.tail,
			HZ/1
			);
	if (timeout < 0)
	{
		logm_debug (vdec, "vdec:%p, signaled\n", vdec);
		return timeout;
	}

	if (timeout == 0)
	{
		logm_trace (vdec, "vdec:%p, timeout\n", vdec);
		return -ETIME;
	}

	ret = noti_copy(&vdec->noti, data, size);
	logm_debug (vdec, "vdec:%p, copied %d bytes\n", vdec, ret);

	return ret;
}

#if 0
static int flush (struct file *file, fl_owner_t id)
{
	vdec_t *vdec;

	vdec = file->private_data;

	logm_noti (vdec, "vdec:%p\n", vdec);

	return _flush (vdec);
}
#endif


static struct file_operations vdec_fops =
{
	.open = open,
	.release = close,
	.unlocked_ioctl = ioctl,
	//.flush = flush,
	.read = read,
	//.mmap = mmap,
	.poll = poll,
};


static int vdec_major = 161;
module_param (vdec_major, int, 0644);

static unsigned long pool_base;
static int pool_size =
#if defined(CHIP_NAME_h13)
    160*1024*1024
#else
	124*1024*1024
#endif
#ifdef INCLUDE_KDRV_MMCU
	- 0x110000
#endif
	;

module_param (pool_size, int, 0444);

static int probe (struct platform_device *dev)
{
	int ret;

	printk ("initialize LG video decoder\n");

	/* proc debug interface */
	proc_init();
	hal_proc_init();

	/* memory pool */
	logm_info (vdec, "pool size, 0x%x\n", pool_size);
	if (pool_size)
	{
		pool_base = hma_alloc_user ("ddr0", pool_size, 1<<12, "vdec");
		if (pool_base == 0)
		{
			logm_error (vdec, "no memory for vdec driver %d\n",
					pool_size);
			return -ENOMEM;
		}
		ret = hma_pool_register ("vdec",
				pool_base, pool_size);
		if (ret < 0)
			return ret;
	}
	else
		logm_info (vdec, "no vdec memory pool. "
				"kernel should initialize vdec memory pool\n");

	/* character device */
	ret = register_chrdev (vdec_major, "vdec", &vdec_fops);
	if (ret < 0)
	{
		logm_error (vdec, "register_chrdev failed.\n");
		return ret;
	}
	if (vdec_major == 0)
	{
		vdec_major = ret;
		logm_info (vdec, "vdec major %d\n", ret);
	}

	VDEC_CH_Init();

	//IPC_CALLBACK_Register_DecInfo(VDEC_IO_UpdateDecodingInfo);
	IPC_CALLBACK_Register_CpbStatus(VDEC_IO_UpdateCpbStatus);
	IPC_CALLBACK_Register_ReqReset(VDEC_IO_RequestCmdReset);

	logm_info (vdec, "vdec initialized\n");

	return 0;
}

static int remove (struct platform_device *dev)
{
	logm_info (vdec, "remove vdec driver\n");

	proc_cleanup ();
	hal_proc_cleanup ();

	/* character device */
	unregister_chrdev (vdec_major, "vdec");

	free_irq(VDEC_IRQ_NUM_PDEC, dev);

	/* memory pool */
	if (pool_base)
	{
		hma_pool_unregister ("vdec");
		hma_free ("ddr0", pool_base);
	}

	return 0;
}

static int suspend (struct platform_device *dev, pm_message_t state)
{
	logm_info (vdec, "suspend... state.event %d\n", state.event);

	/* suspend submodules */
	VES_Suspend ();
	VDC_Suspend ();
	VDISP_Suspend ();

	/* suspend hals */
	TOP_HAL_Suspend ();
	PDEC_HAL_Suspend ();

	return 0;
}

static int resume (struct platform_device *dev)
{
	logm_info (vdec, "resume...\n");

	/* resume hals */
	PDEC_HAL__Resume ();
	TOP_HAL_Resume ();

	/* resume submodules */
	VDISP_Resume ();
	VDC_Resume ();
	VES_Resume ();

	return 0;
}

static struct platform_driver platform_driver =
{
	.probe = probe,
	.remove = remove,
	.suspend = suspend,
	.resume = resume,
	.driver =
	{
		.name = "vdec",
	},
};

static struct platform_device platform_device =
{
	.name = "vdec",
	.id = -1,
};

static int VDEC_Init(void)
{
	int ret;

#if 0
	/* workaround for android.
	 * android udev does not make device node for platform device.
	 */
	if (!vdec_major)
	{
		if (alloc_chrdev_region (&platform_device.dev.devt,
					0, 1, "vdec") < 0)
			logm_warning (vdec, "alloc_chrdev_region failed\n");
		vdec_major = MAJOR (platform_device.dev.devt);
	}
	else
	{
		logm_info (vdec, "vdec major %d\n", vdec_major);
		platform_device.dev.devt = MKDEV (vdec_major, 0);
	}
#else
	{
		static struct class* class = NULL;

		class = class_create(THIS_MODULE, "vdec" );
		device_create (class, NULL, MKDEV(vdec_major, 0), NULL, "vdec");
	}
#endif

	ret = platform_device_register (&platform_device);
	if (ret)
	{
		logm_error (vdec,
				"platform_device_register failed, %d\n", ret);
		return ret;
	}

	ret = platform_driver_register (&platform_driver);
	if (ret)
	{
		logm_error (vdec,
				"platform_driver_register failed, %d\n", ret);
		return ret;
	}

	/* vo device */
	{
		extern int vo_init (void);
		vo_init ();
	}

	return 0;
}

static void VDEC_Cleanup(void)
{
	/* vo device */
	extern void vo_exit (void);
	vo_exit ();

	platform_device_unregister (&platform_device);
	platform_driver_unregister (&platform_driver);

	logm_info (vdec, "vdec device cleanup\n");
}


module_init(VDEC_Init);
module_exit(VDEC_Cleanup);

MODULE_AUTHOR("LGE");
MODULE_DESCRIPTION("LG video decoder driver");
MODULE_LICENSE("GPL");

/**
 * @} */

