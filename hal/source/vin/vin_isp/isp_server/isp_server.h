/*
 * linux-4.9/drivers/media/platform/sunxi-vin/vin-isp/isp_server/include/isp_server.h
 *
 * Copyright (c) 2007-2020 Allwinnertech Co., Ltd.
 *
 * Authors: Zheng ZeQun <zequnzheng@allwinnertech.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _ISP_ALGO_H_
#define _ISP_ALGO_H_

#include <openamp/sunxi_helper/openamp.h>
#include <rpbuf.h>

#include "include/isp_type.h"
#include "include/isp_manage.h"
#include "include/isp_tuning.h"
#include "include/isp_cmd_intf.h"
#include "isp_version.h"

#if (ISP_VERSION >= 600)
#define HW_ISP_DEVICE_NUM	CONFIG_ISP_NUMBER
#else
#define HW_ISP_DEVICE_NUM	1
#endif

#define NORFLASH_SAVE 0x43BFFE00

////////////////////////isp rp//////////////////////////////
/* need set the same with linux */
#define ISP_RPBUF_CONTROLLER_ID 0
#define ISP0_RPBUF_LOAD_NAME "isp0_load_rpbuf"
#define ISP0_RPBUF_SAVE_NAME "isp0_save_rpbuf"
#define ISP1_RPBUF_LOAD_NAME "isp1_load_rpbuf"
#define ISP1_RPBUF_SAVE_NAME "isp1_save_rpbuf"
#define ISP_RPBUF_LOAD_LEN ISP_LOAD_DRAM_SIZE
#define ISP_RPBUF_SAVE_LEN (ISP_STAT_TOTAL_SIZE + ISP_SAVE_LOAD_STATISTIC_SIZE)

#define ISP_RPMSG_CONTROLLER_ID 0
#define ISP0_RPMSG_NAME "sunxi,isp0_rpmsg"
#define ISP1_RPMSG_NAME "sunxi,isp1_rpmsg"

enum rpmsg_cmd {
	ISP_REQUEST_SENSOR_INFO = 0,
	ISP_SET_SENSOR_EXP_GAIN,
	ISP_SET_STAT_EN,
	ISP_SET_SAVE_AE,
	ISP_SET_ENCPP_DATA,

	VIN_SET_SENSOR_INFO,
	VIN_SET_FRAME_SYNC,
	VIN_SET_IOCTL,
	VIN_SET_CLOSE_ISP,
	VIN_SET_ISP_RESET,
};
//////////////////////////end////////////////////////////

struct hw_isp_device {
	unsigned int id;
	int load_type;

	char sensor_name[16];

	unsigned int size;
	void *buffer;

	struct isp_lib_context *ctx;
	struct isp_tuning *tuning;

	struct rpbuf_buffer *load_buffer;
	struct rpbuf_buffer *save_buffer;
	struct rpmsg_endpoint *ept;
};

struct hw_isp_paras {
	int isp_sync_mode;
	int ir_flag;
};

void isp_stats_process(struct hw_isp_device *isp, void *isp_stat_buf);
//void isp_ctrl_process(struct hw_isp_device *isp, struct v4l2_ctrl *ctrl);
void isp_fsync_process(struct hw_isp_device *isp, unsigned int *data);
struct hw_isp_device *isp_server_init(unsigned int id);
int isp_server_save_ctx(unsigned int id);
int isp_server_save_reg(unsigned int id, int ir);
int isp_server_set_paras(struct hw_isp_device *hw_isp, int ir, unsigned int id);
int isp_server_reset(struct hw_isp_device *hw_isp, int dev_id, int mode_flag);
void isp_set_firstframe_wb(int dev_id);
int isp_server_exit(struct hw_isp_device **hw_isp, unsigned int id);
void isp_save_exit(struct hw_isp_device *hw_isp, int id);
HW_S32 isp_stats_req(struct hw_isp_device *hw_isp, int dev_id, struct isp_stats_context *stats_ctx);
int isp_get_lv(struct hw_isp_device *hw_isp, int dev_id);
void isp_ldci_video_en(int id, int en);
int isp_set_attr_cfg(struct hw_isp_device *hw_isp, HW_U32 ctrl_id, void *value);
int isp_get_attr_cfg(struct hw_isp_device *hw_isp, HW_U32 ctrl_id, void *value);
int isp_get_encpp_cfg(struct hw_isp_device *hw_isp, HW_U32 ctrl_id, void *value);

void isp_reset(struct hw_isp_device *hw_isp);

#endif

