/*
 * linux-4.9/drivers/media/platform/sunxi-vin/vin-isp/isp_server/isp_server.c
 *
 * Copyright (c) 2007-2022 Allwinnertech Co., Ltd.
 *
 * Authors:  Yang Feng <yangfeng@allwinnertech.com>
 *         Zheng ZeQun <zequnzheng@allwinnertech.com>
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
#include <hal_mem.h>
#include <math.h>

#include "isp_tuning/isp_tuning_priv.h"
#include "isp_math/isp_math_util.h"
#include "isp_server.h"

extern unsigned int isp_default_reg[ISP_LOAD_REG_SIZE>>2];

struct isp_lib_context isp_ctx[HW_ISP_DEVICE_NUM] = {
	[0] = {
		.isp_ini_cfg = {
			.isp_test_settings = {
				.ae_en = 1,
				.awb_en = 1,
				.wb_en = 1,
				.hist_en = 1,
			},
			.isp_3a_settings = {
				.ae_stat_sel = 1,
				.ae_delay_frame = 0,
				.exp_delay_frame = 1,
				.gain_delay_frame = 1,

				.awb_interval = 2,
				.awb_speed = 32,
				.awb_stat_sel = 1,
				.awb_light_num = 9,
				.awb_light_info = {
					  254,	 256,	104,   256,   256,   256,    50,  1900,    32,	  80,
					  234,	 256,	108,   256,   256,   256,    50,  2500,    32,	  85,
					  217,	 256,	114,   256,   256,   256,    50,  2800,    32,	  90,
					  160,	 256,	153,   256,   256,   256,    50,  4000,    64,	  90,
					  141,	 256,	133,   256,   256,   256,    50,  4100,    64,	 100,
					  142,	 256,	174,   256,   256,   256,    50,  5000,   100,	 100,
					  118,	 256,	156,   256,   256,   256,    50,  5300,    64,	 100,
					  127,	 256,	195,   256,   256,   256,    50,  6500,    64,	 90,
					  115,	 256,	215,   256,   256,   256,    50,  8000,    64,	 80
				},
			},
		},
	},
};
struct isp_tuning *tuning[HW_ISP_DEVICE_NUM];
struct hw_isp_paras hw_isp_paras;

static void __ae_done(struct isp_lib_context *lib,
			ae_result_t *result __attribute__((__unused__)))
{
#if (HW_ISP_DEVICE_NUM > 1)
	if (hw_isp_paras.isp_sync_mode && (lib->isp_id == 0))
		*result = isp_ctx[1].ae_entity_ctx.ae_result;
#endif
	FUNCTION_LOG;
}
static void __af_done(struct isp_lib_context *lib,
			af_result_t *result __attribute__((__unused__)))
{
#if (HW_ISP_DEVICE_NUM > 1)
	if (hw_isp_paras.isp_sync_mode && (lib->isp_id == 0))
		*result = isp_ctx[1].af_entity_ctx.af_result;
#endif
	FUNCTION_LOG;
}
static void __awb_done(struct isp_lib_context *lib,
			awb_result_t *result __attribute__((__unused__)))
{
#if (HW_ISP_DEVICE_NUM > 1)
	if (hw_isp_paras.isp_sync_mode && (lib->isp_id == 0))
		*result = isp_ctx[1].awb_entity_ctx.awb_result;
#endif
	FUNCTION_LOG;
}
static void __afs_done(struct isp_lib_context *lib,
			afs_result_t *result __attribute__((__unused__)))
{
#if (HW_ISP_DEVICE_NUM > 1)
	if (hw_isp_paras.isp_sync_mode && (lib->isp_id == 0))
		*result = isp_ctx[1].afs_entity_ctx.afs_result;
#endif
	FUNCTION_LOG;
}

static void __md_done(struct isp_lib_context *lib,
			md_result_t *result __attribute__((__unused__)))
{
	FUNCTION_LOG;
}

static void __pltm_done(struct isp_lib_context *lib,
			pltm_result_t *result __attribute__((__unused__)))
{
#if (HW_ISP_DEVICE_NUM > 1)
	if (hw_isp_paras.isp_sync_mode && (lib->isp_id == 0))
		*result = isp_ctx[1].pltm_entity_ctx.pltm_result;
#endif
	FUNCTION_LOG;
}


static struct isp_ctx_operations isp_ctx_ops = {
	.ae_done = __ae_done,
	.af_done = __af_done,
	.awb_done = __awb_done,
	.afs_done = __afs_done,
	.md_done = __md_done,
	.pltm_done = __pltm_done,
};

void isp_stats_process(struct hw_isp_device *isp, void *isp_stat_buf)
{
	struct isp_lib_context *ctx;

	ctx = isp_dev_get_ctx(isp);
	if (ctx == NULL)
		return;

	ctx->isp_stat_buf = isp_stat_buf;

	isp_ctx_stats_prepare(ctx, ctx->isp_stat_buf);

	isp_stat_save_run(ctx);

	FUNCTION_LOG;
	isp_ctx_algo_run(ctx);
	FUNCTION_LOG;

	isp_log_save_run(ctx);
}

void isp_fsync_process(struct hw_isp_device *isp, unsigned int *data)
{
	struct isp_lib_context *ctx = isp_dev_get_ctx(isp);

	if (ctx->sensor_info.color_space != data[1]) {
		ctx->sensor_info.color_space = data[1];
		ctx->isp_3a_change_flags |= ISP_SET_HUE;
	}

	isp_lib_log_param = (data[3] << 8) | data[2] | ctx->isp_ini_cfg.isp_test_settings.isp_log_param;
}

#if 0
void isp_ctrl_process(struct hw_isp_device *isp, struct v4l2_ctrl *ctrl)
{
	struct isp_lib_context *isp_gen = isp_dev_get_ctx(isp);
	HW_S32 iso_qmenu[] = { 100, 200, 400, 800, 1600, 3200, 6400};
	HW_S32 exp_bias_qmenu[] = { -4, -3, -2, -1, 0, 1, 2, 3, 4, };

	if (isp_gen == NULL)
		return;

	switch (ctrl->id) {
	case V4L2_CID_BRIGHTNESS:
		isp_s_brightness(isp_gen, ctrl->val);
		break;
	case V4L2_CID_CONTRAST:
		isp_s_contrast(isp_gen, ctrl->val);
		break;
	case V4L2_CID_SATURATION:
		isp_s_saturation(isp_gen, ctrl->val);
		break;
	case V4L2_CID_HUE:
		isp_s_hue(isp_gen, ctrl->val);
		break;
	case V4L2_CID_AUTO_WHITE_BALANCE:
		isp_s_auto_white_balance(isp_gen, ctrl->val);
		break;
	case V4L2_CID_EXPOSURE:
		isp_s_exposure(isp_gen, ctrl->val);
		break;
	case V4L2_CID_AUTOGAIN:
		isp_s_auto_gain(isp_gen, ctrl->val);
		break;
	case V4L2_CID_GAIN:
		isp_s_gain(isp_gen, ctrl->val);
		break;
	case V4L2_CID_POWER_LINE_FREQUENCY:
		isp_s_power_line_frequency(isp_gen, ctrl->val);
		break;
	case V4L2_CID_WHITE_BALANCE_TEMPERATURE:
		isp_s_white_balance_temperature(isp_gen, ctrl->val);
		break;
	case V4L2_CID_SHARPNESS:
		isp_s_sharpness(isp_gen, ctrl->val);
		break;
	case V4L2_CID_AUTOBRIGHTNESS:
		isp_s_auto_brightness(isp_gen, ctrl->val);
		break;
	case V4L2_CID_BAND_STOP_FILTER:
		isp_s_band_stop_filter(isp_gen, ctrl->val);
		break;
	case V4L2_CID_ILLUMINATORS_1:
		isp_s_illuminators_1(isp_gen, ctrl->val);
		break;
	case V4L2_CID_ILLUMINATORS_2:
		isp_s_illuminators_2(isp_gen, ctrl->val);
		break;
	case V4L2_CID_EXPOSURE_AUTO:
		isp_s_exposure_auto(isp_gen, ctrl->val);
		break;
	case V4L2_CID_EXPOSURE_ABSOLUTE:
		isp_s_exposure_absolute(isp_gen, ctrl->val);
		break;
	case V4L2_CID_FOCUS_ABSOLUTE:
		isp_s_focus_absolute(isp_gen, ctrl->val);
		break;
	case V4L2_CID_FOCUS_RELATIVE:
		isp_s_focus_relative(isp_gen, ctrl->val);
		break;
	case V4L2_CID_FOCUS_AUTO:
		isp_s_focus_auto(isp_gen, ctrl->val);
		break;
	case V4L2_CID_AUTO_EXPOSURE_BIAS:
		isp_s_auto_exposure_bias(isp_gen, exp_bias_qmenu[ctrl->val]);
		break;
	case V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE:
		isp_s_auto_n_preset_white_balance(isp_gen, ctrl->val);
		break;
	case V4L2_CID_ISO_SENSITIVITY:
		isp_s_iso_sensitivity(isp_gen, iso_qmenu[ctrl->val]);
		break;
	case V4L2_CID_ISO_SENSITIVITY_AUTO:
		isp_s_iso_sensitivity_auto(isp_gen, ctrl->val);
		break;
	case V4L2_CID_EXPOSURE_METERING:
		isp_s_ae_metering_mode(isp_gen, ctrl->val);
		break;
	case V4L2_CID_SCENE_MODE:
		isp_s_scene_mode(isp_gen, ctrl->val);
		break;
	case V4L2_CID_3A_LOCK:
		//isp_s_3a_lock(isp_gen,ctrl->val);
		break;
	case V4L2_CID_AUTO_FOCUS_START:
		isp_s_auto_focus_start(isp_gen, ctrl->val);
		break;
	case V4L2_CID_AUTO_FOCUS_STOP:
		isp_s_auto_focus_stop(isp_gen, ctrl->val);
		break;
	case V4L2_CID_AUTO_FOCUS_RANGE:
		isp_s_auto_focus_range(isp_gen, ctrl->val);
		break;
	case V4L2_CID_TAKE_PICTURE:
		isp_s_take_picture(isp_gen, ctrl->val);
		break;
	case V4L2_CID_FLASH_LED_MODE:
		isp_s_flash_mode(isp_gen, ctrl->val);
		break;
	default:
		ISP_ERR("Unknown ctrl.\n");
		break;
	}
}
#endif

struct hw_isp_device *isp_server_init(unsigned int id)
{
	struct hw_isp_device *hw_isp = NULL;

	if (id >= HW_ISP_DEVICE_NUM)
		return NULL;

	isp_version_info();

	hw_isp = hal_malloc(sizeof(struct hw_isp_device));
	if (!hw_isp)
		goto ekzalloc;
	memset(hw_isp, 0, sizeof(struct hw_isp_device));

	hw_isp->id = id;
	isp_ctx[id].isp_id = id;
	isp_dev_banding_ctx(hw_isp, &isp_ctx[id]);

	return hw_isp;
ekzalloc:
	ISP_ERR("isp server init err!\n");
	return NULL;
}

int isp_server_save_ctx(unsigned int id)
{
	return isp_ctx_save_init(&isp_ctx[id]);
}

int isp_server_save_reg(unsigned int id, int ir)
{
	return isp_reg_save_init(&isp_ctx[id], ir);
}

int isp_server_set_paras(struct hw_isp_device *hw_isp, int ir, unsigned int id)
{
	int ret = 0;

	if (id >= HW_ISP_DEVICE_NUM)
		return -1;

	hw_isp_paras.isp_sync_mode = 0;
	hw_isp_paras.ir_flag = ir;
	isp_ctx[id].isp_ir_flag = hw_isp_paras.ir_flag;
	ret = isp_params_parse(hw_isp, &isp_ctx[id].isp_ini_cfg, hw_isp_paras.ir_flag, hw_isp_paras.isp_sync_mode);
	if (ret < 0)
		ISP_PRINT("isp param parse fail!!\n");

	FUNCTION_LOG;
	if (isp_ctx_algo_init(&isp_ctx[id], &isp_ctx_ops))
		return -1;
	FUNCTION_LOG;

	tuning[id] = isp_tuning_init(hw_isp, &isp_ctx[id].isp_ini_cfg);
	if (tuning[id] == NULL) {
		ISP_ERR("error: unable to initialize isp tuning\n");
		return -1;
	}
	FUNCTION_LOG;

	return ret;
}

int isp_server_reset(struct hw_isp_device *hw_isp, int dev_id, int mode_flag)
{
	int ret = 0;

	if (dev_id >= HW_ISP_DEVICE_NUM)
		return -1;

	if (mode_flag & 0x02) {
		hw_isp_paras.ir_flag = 1;
		ISP_PRINT("ISP select ir config\n");
	} else
		hw_isp_paras.ir_flag = 0;

	//isp_ctx[dev_id].isp_load_flag = 0;
	isp_ctx[dev_id].isp_ir_flag = hw_isp_paras.ir_flag;  /*change hue and color, when isp reset*/
	isp_params_parse(hw_isp, &isp_ctx[dev_id].isp_ini_cfg, hw_isp_paras.ir_flag, hw_isp_paras.isp_sync_mode);
	ret = isp_tuning_reset(hw_isp, &isp_ctx[dev_id].isp_ini_cfg);
	if (ret) {
		ISP_ERR("error: unable to reset isp tuning\n");
	}

	return ret;
}

void isp_set_firstframe_wb(int dev_id)
{
	isp_awb_entity_context_t *isp_awb_cxt = &isp_ctx[dev_id].awb_entity_ctx;

	if (dev_id >= HW_ISP_DEVICE_NUM)
		return;

	/* when liner high frame rate run first and get now wb gain, and then run in wdr low frame rate, wb gain must be sqrt*/
	if (isp_ctx[dev_id].otp_enable == 1) {
		isp_ctx[dev_id].module_cfg.wb_gain_cfg.wb_gain.r_gain = isp_awb_cxt->awb_result.wb_gain_output.r_gain*isp_ctx[dev_id].wb_golden_ratio[0];
		isp_ctx[dev_id].module_cfg.wb_gain_cfg.wb_gain.gr_gain = isp_awb_cxt->awb_result.wb_gain_output.gr_gain*isp_ctx[dev_id].wb_golden_ratio[1];
		isp_ctx[dev_id].module_cfg.wb_gain_cfg.wb_gain.gb_gain = isp_awb_cxt->awb_result.wb_gain_output.gb_gain*isp_ctx[dev_id].wb_golden_ratio[1];
		isp_ctx[dev_id].module_cfg.wb_gain_cfg.wb_gain.b_gain = isp_awb_cxt->awb_result.wb_gain_output.b_gain*isp_ctx[dev_id].wb_golden_ratio[2];
	} else {
		isp_ctx[dev_id].module_cfg.wb_gain_cfg.wb_gain = isp_awb_cxt->awb_result.wb_gain_output;
	}
}

int isp_server_exit(struct hw_isp_device **hw_isp, unsigned int id)
{
	if (id >= HW_ISP_DEVICE_NUM)
		return -1;

	isp_log_save_exit(&isp_ctx[id]);
	isp_stat_save_exit(&isp_ctx[id]);
	isp_reg_save_exit(&isp_ctx[id]);
	isp_ctx_save_exit(&isp_ctx[id]);
	isp_tuning_exit(*hw_isp);
	isp_ctx_algo_exit(&isp_ctx[id]);
	hal_free(*hw_isp);
	*hw_isp = NULL;

	ISP_PRINT("isp%d exit end!!!\n", id);

	return 0;
}

void isp_save_exit(struct hw_isp_device *hw_isp, int id)
{
	if (id >= HW_ISP_DEVICE_NUM)
		return;

	isp_log_save_exit(&isp_ctx[id]);
	isp_stat_save_exit(&isp_ctx[id]);
	isp_reg_save_exit(&isp_ctx[id]);
	isp_ctx_save_exit(&isp_ctx[id]);
}

HW_S32 isp_stats_req(struct hw_isp_device *hw_isp, int dev_id, struct isp_stats_context *stats_ctx)
{
	struct isp_lib_context *ctx = NULL;

	if (dev_id >= HW_ISP_DEVICE_NUM)
		return -1;

	ctx = isp_dev_get_ctx(hw_isp);
	if (ctx == NULL)
		return -1;

	return isp_ctx_stats_req(ctx, stats_ctx);
}

int isp_get_lv(struct hw_isp_device *hw_isp, int dev_id)
{
	struct isp_lib_context *ctx;

	ctx = isp_dev_get_ctx(hw_isp);
	if (ctx == NULL) {
		ISP_ERR("isp%d get isp ctx failed!\n", dev_id);
		return -1;
	}

	return ctx->ae_entity_ctx.ae_result.ev_lv_adj;
}

void isp_ldci_video_en(int id, int en)
{
	/*if (isp_ctx[id].isp_ini_cfg.isp_tunning_settings.gtm_type == 4) {
		if (en)
			enable_ldci_video(LDCI_VIDEO_CHN);
		else
			disable_ldci_video(LDCI_VIDEO_CHN);
	}*/
}

int isp_get_attr_cfg(struct hw_isp_device *hw_isp, HW_U32 ctrl_id, void *value)
{
	struct isp_lib_context *isp_gen = isp_dev_get_ctx(hw_isp);
	if (isp_gen == NULL)
		return -1;

	switch (ctrl_id) {
	case ISP_CTRL_MODULE_EN:
		break;
	case ISP_CTRL_DIGITAL_GAIN:
		*(HW_S32 *)value = isp_gen->ae_entity_ctx.ae_result.sensor_set.ev_set_curr.ev_digital_gain;
		break;
	case ISP_CTRL_PLTMWDR_STR:
		*(HW_S32 *)value = isp_gen->tune.pltmwdr_level;
		break;
	case ISP_CTRL_DN_STR:
		*(HW_S32 *)value = isp_gen->tune.denoise_level;
		break;
	case ISP_CTRL_3DN_STR:
		*(HW_S32 *)value = isp_gen->tune.tdf_level;
		break;
	case ISP_CTRL_HIGH_LIGHT:
		*(HW_S32 *)value = isp_gen->tune.highlight_level;
		break;
	case ISP_CTRL_BACK_LIGHT:
		*(HW_S32 *)value = isp_gen->tune.backlight_level;
		break;
	case ISP_CTRL_WB_MGAIN:
		*(struct isp_wb_gain *)value = isp_gen->awb_entity_ctx.awb_result.wb_gain_output;
		break;
	case ISP_CTRL_AGAIN_DGAIN:
		*(struct gain_cfg *)value = isp_gen->tune.gains;
		break;
	case ISP_CTRL_COLOR_EFFECT:
		*(HW_S32 *)value = isp_gen->tune.effect;
		break;
	case ISP_CTRL_AE_ROI:
		*(struct isp_h3a_coor_win *)value = isp_gen->ae_settings.ae_coor;
		break;
	case ISP_CTRL_COLOR_TEMP:
		*(HW_S32 *)value = isp_gen->awb_entity_ctx.awb_result.color_temp_output;
		break;
	case ISP_CTRL_EV_IDX:
		*(HW_S32 *)value = isp_gen->ae_entity_ctx.ae_result.sensor_set.ev_set.ev_idx;
		break;
	default:
		ISP_ERR("Unknown ctrl.\n");
		break;
	}
	return 0;
}

int isp_set_attr_cfg(struct hw_isp_device *hw_isp, HW_U32 ctrl_id, void *value)
{
	struct isp_lib_context *isp_gen = isp_dev_get_ctx(hw_isp);
	if (isp_gen == NULL)
		return -1;

	switch (ctrl_id) {
	case ISP_CTRL_MODULE_EN:
		break;
	case ISP_CTRL_DIGITAL_GAIN:
		break;
	case ISP_CTRL_PLTMWDR_STR:
		isp_gen->tune.pltmwdr_level = *(HW_S32 *)value;
		break;
	case ISP_CTRL_DN_STR:
		isp_gen->tune.denoise_level = *(HW_S32 *)value;
		break;
	case ISP_CTRL_3DN_STR:
		isp_gen->tune.tdf_level = *(HW_S32 *)value;
		break;
	case ISP_CTRL_HIGH_LIGHT:
		isp_gen->tune.highlight_level = clamp(*(HW_S32 *)value, -31, 35);
		break;
	case ISP_CTRL_BACK_LIGHT:
		isp_gen->tune.backlight_level = clamp(*(HW_S32 *)value, -31, 35);
		break;
	case ISP_CTRL_WB_MGAIN:
		isp_gen->awb_settings.wb_gain_manual = *(struct isp_wb_gain *)value;
		break;
	case ISP_CTRL_AGAIN_DGAIN:
		if (memcmp(&isp_gen->tune.gains, value, sizeof(struct gain_cfg))) {
			isp_gen->tune.gains = *(struct gain_cfg *)value;
			isp_gen->isp_3a_change_flags |= ISP_SET_GAIN_STR;
		}
		break;
	case ISP_CTRL_COLOR_EFFECT:
		if (isp_gen->tune.effect != *(HW_S32 *)value) {
			isp_gen->tune.effect = *(HW_S32 *)value;
			isp_gen->isp_3a_change_flags |= ISP_SET_EFFECT;
		}
		break;
	case ISP_CTRL_AE_ROI:
		isp_s_ae_roi(isp_gen, AE_METERING_MODE_SPOT, value);
		break;
	case ISP_CTRL_AF_METERING:
		isp_s_af_metering_mode(isp_gen, value);
		break;
	case ISP_CTRL_VENC2ISP_PARAM:
		isp_gen->VencVe2IspParam = *(struct enc_VencVe2IspParam *)value;
		break;
	case ISP_CTRL_NPU_NR_PARAM:
		isp_gen->npu_nr_cfg = *(struct npu_face_nr_config *)value;
		break;
	default:
		ISP_ERR("Unknown ctrl.\n");
		break;
	}
	return 0;
}

#if 1
int isp_rpmsg_ept_send(struct rpmsg_endpoint *ept, void *data, int len);
void isp_save_ae_gainexp(struct hw_isp_device *hw_isp)
{
#ifdef CONFIG_ISP_READ_THRESHOLD
	struct isp_lib_context *ctx = isp_dev_get_ctx(hw_isp);
	unsigned int rpmsg_data[3];
	ae_result_t *ae_result = NULL;

	memset(rpmsg_data, 0, 3 * sizeof(unsigned int));
	ae_result = &ctx->ae_entity_ctx.ae_result;
	rpmsg_data[0] = ISP_SET_SAVE_AE;
	rpmsg_data[1] = (ae_result->sensor_set.ev_set_curr.ev_idx << 16) | ae_result->sensor_set.ev_set_curr.ev_analog_gain >> 4;
	rpmsg_data[2] = ae_result->sensor_set.ev_set_curr.ev_sensor_exp_line;
	isp_rpmsg_ept_send(hw_isp->ept, rpmsg_data, 3*4);
#endif
}

int isp_rpmsg_encpp_send(struct hw_isp_device *hw_isp)
{
	struct isp_lib_context *isp_gen = isp_dev_get_ctx(hw_isp);
	unsigned short rpmsg_data[209];
	unsigned int *rpmsg_head = (unsigned int *)&rpmsg_data[0];
	unsigned short *ptr = NULL;
	int i;
	if (isp_gen == NULL)
		return -1;

	rpmsg_head[0] = ISP_SET_ENCPP_DATA;
	rpmsg_data[2] = isp_gen->isp_ini_cfg.isp_test_settings.encpp_en;
	rpmsg_data[3] = isp_gen->encpp_static_sharp_cfg.ss_shp_ratio;
	rpmsg_data[4] = isp_gen->encpp_static_sharp_cfg.ls_shp_ratio;
	rpmsg_data[5] = isp_gen->encpp_static_sharp_cfg.ss_dir_ratio;
	rpmsg_data[6] = isp_gen->encpp_static_sharp_cfg.ls_dir_ratio;
	rpmsg_data[7] = isp_gen->encpp_static_sharp_cfg.ss_crc_stren;
	rpmsg_data[8] = isp_gen->encpp_static_sharp_cfg.ss_crc_min;
	rpmsg_data[9] = isp_gen->encpp_static_sharp_cfg.wht_sat_ratio;
	rpmsg_data[10] = isp_gen->encpp_static_sharp_cfg.blk_sat_ratio;
	rpmsg_data[11] = isp_gen->encpp_static_sharp_cfg.wht_slp_bt;
	rpmsg_data[12] = isp_gen->encpp_static_sharp_cfg.blk_slp_bt;
	ptr = &rpmsg_data[13];
	for (i = 0; i < ISP_REG_TBL_LENGTH; i++, ptr++) {
		*ptr = isp_gen->encpp_static_sharp_cfg.sharp_ss_value[i];
	}
	ptr = &rpmsg_data[46];
	for (i = 0; i < ISP_REG_TBL_LENGTH; i++, ptr++) {
		*ptr = isp_gen->encpp_static_sharp_cfg.sharp_ls_value[i];
	}
	ptr = &rpmsg_data[79];
	for (i = 0; i < 46; i++, ptr++) {
		*ptr = isp_gen->encpp_static_sharp_cfg.sharp_hsv[i];
	}
	rpmsg_data[125] = isp_gen->iso_entity_ctx.iso_result.encpp_dynamic_sharp_cfg.ss_ns_lw;
	rpmsg_data[126] = isp_gen->iso_entity_ctx.iso_result.encpp_dynamic_sharp_cfg.ss_ns_hi;
	rpmsg_data[127] = isp_gen->iso_entity_ctx.iso_result.encpp_dynamic_sharp_cfg.ls_ns_lw;
	rpmsg_data[128] = isp_gen->iso_entity_ctx.iso_result.encpp_dynamic_sharp_cfg.ls_ns_hi;
	rpmsg_data[129] = isp_gen->iso_entity_ctx.iso_result.encpp_dynamic_sharp_cfg.ss_lw_cor;
	rpmsg_data[130] = isp_gen->iso_entity_ctx.iso_result.encpp_dynamic_sharp_cfg.ss_hi_cor;
	rpmsg_data[131] = isp_gen->iso_entity_ctx.iso_result.encpp_dynamic_sharp_cfg.ls_lw_cor;
	rpmsg_data[132] = isp_gen->iso_entity_ctx.iso_result.encpp_dynamic_sharp_cfg.ls_hi_cor;
	rpmsg_data[133] = isp_gen->iso_entity_ctx.iso_result.encpp_dynamic_sharp_cfg.ss_blk_stren;
	rpmsg_data[134] = isp_gen->iso_entity_ctx.iso_result.encpp_dynamic_sharp_cfg.ss_wht_stren;
	rpmsg_data[135] = isp_gen->iso_entity_ctx.iso_result.encpp_dynamic_sharp_cfg.ls_blk_stren;
	rpmsg_data[136] = isp_gen->iso_entity_ctx.iso_result.encpp_dynamic_sharp_cfg.ls_wht_stren;
	rpmsg_data[137] = isp_gen->iso_entity_ctx.iso_result.encpp_dynamic_sharp_cfg.ss_avg_smth;
	rpmsg_data[138] = isp_gen->iso_entity_ctx.iso_result.encpp_dynamic_sharp_cfg.ss_dir_smth;
	ptr = &rpmsg_data[139];
	for (i = 0; i < 4; i++, ptr++) {
		*ptr = isp_gen->iso_entity_ctx.iso_result.encpp_dynamic_sharp_cfg.dir_smth[i];
	}
	rpmsg_data[143] = isp_gen->iso_entity_ctx.iso_result.encpp_dynamic_sharp_cfg.hfr_smth_ratio;
	rpmsg_data[144] = isp_gen->iso_entity_ctx.iso_result.encpp_dynamic_sharp_cfg.hfr_hf_wht_stren;
	rpmsg_data[145] = isp_gen->iso_entity_ctx.iso_result.encpp_dynamic_sharp_cfg.hfr_hf_blk_stren;
	rpmsg_data[146] = isp_gen->iso_entity_ctx.iso_result.encpp_dynamic_sharp_cfg.hfr_mf_wht_stren;
	rpmsg_data[147] = isp_gen->iso_entity_ctx.iso_result.encpp_dynamic_sharp_cfg.hfr_mf_blk_stren;
	rpmsg_data[148] = isp_gen->iso_entity_ctx.iso_result.encpp_dynamic_sharp_cfg.hfr_hf_cor_ratio;
	rpmsg_data[149] = isp_gen->iso_entity_ctx.iso_result.encpp_dynamic_sharp_cfg.hfr_mf_cor_ratio;
	rpmsg_data[150] = isp_gen->iso_entity_ctx.iso_result.encpp_dynamic_sharp_cfg.hfr_hf_mix_ratio;
	rpmsg_data[151] = isp_gen->iso_entity_ctx.iso_result.encpp_dynamic_sharp_cfg.hfr_mf_mix_ratio;
	rpmsg_data[152] = isp_gen->iso_entity_ctx.iso_result.encpp_dynamic_sharp_cfg.hfr_hf_mix_min_ratio;
	rpmsg_data[153] = isp_gen->iso_entity_ctx.iso_result.encpp_dynamic_sharp_cfg.hfr_mf_mix_min_ratio;
	rpmsg_data[154] = isp_gen->iso_entity_ctx.iso_result.encpp_dynamic_sharp_cfg.hfr_hf_wht_clp;
	rpmsg_data[155] = isp_gen->iso_entity_ctx.iso_result.encpp_dynamic_sharp_cfg.hfr_hf_blk_clp;
	rpmsg_data[156] = isp_gen->iso_entity_ctx.iso_result.encpp_dynamic_sharp_cfg.hfr_mf_wht_clp;
	rpmsg_data[157] = isp_gen->iso_entity_ctx.iso_result.encpp_dynamic_sharp_cfg.hfr_mf_blk_clp;
	rpmsg_data[158] = isp_gen->iso_entity_ctx.iso_result.encpp_dynamic_sharp_cfg.wht_clp_para;
	rpmsg_data[159] = isp_gen->iso_entity_ctx.iso_result.encpp_dynamic_sharp_cfg.blk_clp_para;
	rpmsg_data[160] = isp_gen->iso_entity_ctx.iso_result.encpp_dynamic_sharp_cfg.wht_clp_slp;
	rpmsg_data[161] = isp_gen->iso_entity_ctx.iso_result.encpp_dynamic_sharp_cfg.blk_clp_slp;
	rpmsg_data[162] = isp_gen->iso_entity_ctx.iso_result.encpp_dynamic_sharp_cfg.max_clp_ratio;
	ptr = &rpmsg_data[163];
	for (i = 0; i < ISP_REG_TBL_LENGTH; i++, ptr++) {
		*ptr = isp_gen->iso_entity_ctx.iso_result.encpp_dynamic_sharp_cfg.sharp_edge_lum[i];
	}
	rpmsg_data[196] = isp_gen->iso_entity_ctx.iso_result.encoder_3dnr_cfg.enable_3d_fliter;
	rpmsg_data[197] = isp_gen->iso_entity_ctx.iso_result.encoder_3dnr_cfg.adjust_pix_level_enable;
	rpmsg_data[198] = isp_gen->iso_entity_ctx.iso_result.encoder_3dnr_cfg.smooth_filter_enable;
	rpmsg_data[199] = isp_gen->iso_entity_ctx.iso_result.encoder_3dnr_cfg.max_pix_diff_th;
	rpmsg_data[200] = isp_gen->iso_entity_ctx.iso_result.encoder_3dnr_cfg.max_mad_th;
	rpmsg_data[201] = isp_gen->iso_entity_ctx.iso_result.encoder_3dnr_cfg.max_mv_th;
	rpmsg_data[202] = isp_gen->iso_entity_ctx.iso_result.encoder_3dnr_cfg.min_coef;
	rpmsg_data[203] = isp_gen->iso_entity_ctx.iso_result.encoder_3dnr_cfg.max_coef;
	rpmsg_data[204] = isp_gen->iso_entity_ctx.iso_result.encoder_2dnr_cfg.enable_2d_fliter;
	rpmsg_data[205] = isp_gen->iso_entity_ctx.iso_result.encoder_2dnr_cfg.filter_strength_uv;
	rpmsg_data[206] = isp_gen->iso_entity_ctx.iso_result.encoder_2dnr_cfg.filter_strength_y;
	rpmsg_data[207] = isp_gen->iso_entity_ctx.iso_result.encoder_2dnr_cfg.filter_th_uv;
	rpmsg_data[208] = isp_gen->iso_entity_ctx.iso_result.encoder_2dnr_cfg.filter_th_y;

	isp_rpmsg_ept_send(hw_isp->ept, rpmsg_data, sizeof(rpmsg_data));
	return 0;
}

void isp_get_ae_ev(struct hw_isp_device *hw_isp)
{
#ifdef CONFIG_ISP_READ_THRESHOLD
	struct isp_lib_context *ctx = isp_dev_get_ctx(hw_isp);
	ae_result_t *ae_result = NULL;

	ae_result = &ctx->ae_entity_ctx.ae_result;
	ae_result->sensor_set.ev_set_curr.ev_idx =  clamp(*((unsigned int *)NORFLASH_SAVE) >> 16, 0, ae_result->sensor_set.ev_idx_max);
	ae_result->sensor_set.ev_set_last.ev_idx =  ae_result->sensor_set.ev_set_curr.ev_idx;
	ae_result->sensor_set.ev_set.ev_idx = ae_result->sensor_set.ev_set_curr.ev_idx;
	ctx->ae_frame_cnt  = 0;
	ISP_PRINT("get ae idx is %d\n", ae_result->sensor_set.ev_set_curr.ev_idx);
#endif
}

static void isp_update_encpp_cfg(struct hw_isp_device *hw_isp) {
	struct isp_lib_context *isp_gen = isp_dev_get_ctx(hw_isp);
	isp_iso_entity_context_t *isp_iso_cxt = &isp_gen->iso_entity_ctx;

	isp_gen->iso_entity_ctx.iso_param->cnr_adjust = false;
	isp_gen->iso_entity_ctx.iso_param->sharpness_adjust = false;
	isp_gen->iso_entity_ctx.iso_param->brightness_adjust = false;
	isp_gen->iso_entity_ctx.iso_param->contrast_adjust = false;
	isp_gen->iso_entity_ctx.iso_param->cem_adjust = false;
	isp_gen->iso_entity_ctx.iso_param->denoise_adjust = false;
	isp_gen->iso_entity_ctx.iso_param->black_level_adjust = false;
	isp_gen->iso_entity_ctx.iso_param->dpc_adjust = false;
	isp_gen->iso_entity_ctx.iso_param->defog_value_adjust = false;
	isp_gen->iso_entity_ctx.iso_param->pltm_dynamic_cfg_adjust = false;
	isp_gen->iso_entity_ctx.iso_param->tdnr_adjust = false;
	isp_gen->iso_entity_ctx.iso_param->ae_cfg_adjust = false;
	isp_gen->iso_entity_ctx.iso_param->gtm_cfg_adjust = false;
	isp_gen->iso_entity_ctx.iso_param->lca_cfg_adjust = false;
	isp_gen->iso_entity_ctx.iso_param->wdr_cfg_adjust = false;
	isp_gen->iso_entity_ctx.iso_param->cfa_adjust = false;

	isp_iso_cxt->ops->isp_iso_run(isp_iso_cxt->iso_entity, &isp_iso_cxt->iso_result);

	isp_gen->iso_entity_ctx.iso_param->cnr_adjust = true;
	isp_gen->iso_entity_ctx.iso_param->sharpness_adjust = true;
	isp_gen->iso_entity_ctx.iso_param->brightness_adjust = true;
	isp_gen->iso_entity_ctx.iso_param->contrast_adjust = true;
	isp_gen->iso_entity_ctx.iso_param->cem_adjust = true;
	isp_gen->iso_entity_ctx.iso_param->denoise_adjust = true;
	isp_gen->iso_entity_ctx.iso_param->black_level_adjust = true;
	isp_gen->iso_entity_ctx.iso_param->dpc_adjust = true;
	isp_gen->iso_entity_ctx.iso_param->defog_value_adjust = true;
	isp_gen->iso_entity_ctx.iso_param->pltm_dynamic_cfg_adjust = true;
	isp_gen->iso_entity_ctx.iso_param->tdnr_adjust = true;
	isp_gen->iso_entity_ctx.iso_param->ae_cfg_adjust = true;
	isp_gen->iso_entity_ctx.iso_param->gtm_cfg_adjust = true;
	isp_gen->iso_entity_ctx.iso_param->lca_cfg_adjust = true;
	isp_gen->iso_entity_ctx.iso_param->wdr_cfg_adjust = true;
	isp_gen->iso_entity_ctx.iso_param->cfa_adjust = true;
}

static int isp_alloc_reg_tbl(struct hw_isp_device *hw_isp)
{
	struct isp_lib_context *isp_gen = isp_dev_get_ctx(hw_isp);

	/* REG LOAD*/
	isp_gen->load_reg_base = rpbuf_buffer_va(hw_isp->load_buffer);
	if (isp_gen->load_reg_base == NULL) {
		ISP_ERR("load_reg_base alloc failed, no memory!");
		return -1;
	}
	memset(isp_gen->load_reg_base, 0, ISP_LOAD_REG_SIZE);
	memcpy(isp_gen->load_reg_base, &isp_default_reg[0], ISP_LOAD_REG_SIZE);

	isp_gen->module_cfg.isp_dev_id = isp_gen->isp_id;
	//isp_gen->module_cfg.isp_platform_id = ISP_PLATFORM_SUN8IW21P1;

	isp_map_addr(&isp_gen->module_cfg, (unsigned long)isp_gen->load_reg_base);
#if (ISP_VERSION >= 600)
	/*FE TABLE*/
	isp_gen->module_cfg.fe_table = isp_gen->load_reg_base + ISP_LOAD_REG_SIZE;
	memset(isp_gen->module_cfg.fe_table, 0, ISP_FE_TABLE_SIZE);

	isp_gen->module_cfg.ch0_msc_table = isp_gen->module_cfg.fe_table + ISP_CH0_MSC_FE_MEM_OFS;
	isp_gen->module_cfg.ch1_msc_table = isp_gen->module_cfg.fe_table + ISP_CH1_MSC_FE_MEM_OFS;
	isp_gen->module_cfg.ch2_msc_table = isp_gen->module_cfg.fe_table + ISP_CH2_MSC_FE_MEM_OFS;

	/*BAYER TABLE*/
	isp_gen->module_cfg.bayer_table = isp_gen->module_cfg.fe_table + ISP_FE_TABLE_SIZE;
	memset(isp_gen->module_cfg.bayer_table, 0, ISP_BAYER_TABLE_SIZE);

	isp_gen->module_cfg.lens_table = isp_gen->module_cfg.bayer_table + ISP_LSC_MEM_OFS;
	isp_gen->module_cfg.msc_table = isp_gen->module_cfg.bayer_table + ISP_MSC_MEM_OFS;
	isp_gen->module_cfg.nr_msc_table = isp_gen->module_cfg.bayer_table + ISP_MSC_NR_MEM_OFS;
	isp_gen->module_cfg.tdnf_table = isp_gen->module_cfg.bayer_table + ISP_TDNF_DK_MEM_OFS;
	isp_gen->module_cfg.pltm_table = isp_gen->module_cfg.bayer_table + ISP_PLTM_TM_MEM_OFS;

	/*RGB TABLE*/
	isp_gen->module_cfg.rgb_table = isp_gen->module_cfg.bayer_table + ISP_BAYER_TABLE_SIZE;
	memset(isp_gen->module_cfg.rgb_table, 0, ISP_RGB_TABLE_SIZE);

	isp_gen->module_cfg.drc_table = isp_gen->module_cfg.rgb_table + ISP_DRC_MEM_OFS;
	isp_gen->module_cfg.gamma_table = isp_gen->module_cfg.rgb_table + ISP_GAMMA_MEM_OFS;

	/*YUV TABLE*/
	isp_gen->module_cfg.yuv_table = isp_gen->module_cfg.rgb_table + ISP_RGB_TABLE_SIZE;
	memset(isp_gen->module_cfg.yuv_table, 0, ISP_YUV_TABLE_SIZE);

	isp_gen->module_cfg.cem_table = isp_gen->module_cfg.yuv_table + ISP_CEM_MEM_OFS;
#endif
	return 0;
}

static int isp_config_sensor_info(struct hw_isp_device *hw_isp, void *data)
{
	struct isp_lib_context *ctx = isp_dev_get_ctx(hw_isp);
	unsigned int *sensor_data = data;
	char *sensor_name;
	int i;

	if (ctx == NULL)
		return -1;

	ISP_LIB_LOG(ISP_LOG_RP, "isp get sensor info from linux\n");

	ctx->sensor_info.sensor_width = sensor_data[1];
	ctx->sensor_info.sensor_height = sensor_data[2];
	ctx->sensor_info.fps_fixed = sensor_data[3];
	ctx->sensor_info.wdr_mode = sensor_data[4];
	// BT601_FULL_RANGE
	ctx->sensor_info.color_space = sensor_data[5];

	ctx->sensor_info.bayer_bit = sensor_data[6];
	ctx->sensor_info.input_seq = sensor_data[7];

	ctx->sensor_info.hts = sensor_data[8];
	ctx->sensor_info.vts = sensor_data[9];
	ctx->sensor_info.pclk = sensor_data[10];
	ctx->sensor_info.bin_factor = sensor_data[11];
	ctx->sensor_info.gain_min = sensor_data[12];
	ctx->sensor_info.gain_max = sensor_data[13];
	ctx->sensor_info.hoffset = sensor_data[14];
	ctx->sensor_info.voffset = sensor_data[15];

	sensor_name = (char *)&sensor_data[16];
	memcpy(hw_isp->sensor_name, sensor_name, 4 * sizeof(unsigned int));
	ctx->sensor_info.name = hw_isp->sensor_name;

	ctx->stat.pic_size.width = ctx->sensor_info.sensor_width;
	ctx->stat.pic_size.height = ctx->sensor_info.sensor_height;

	ctx->stats_ctx.pic_w = ctx->sensor_info.sensor_width;
	ctx->stats_ctx.pic_h = ctx->sensor_info.sensor_height;
#if 1
	// update otp infomation
	if (ctx->otp_enable == -1) {
		ISP_PRINT("otp disabled, msc use 1024\n");
		for (i = 0; i < 16*16*3; i++) {
			((unsigned short *)(ctx->pmsc_table))[i] = 1024;
		}
	}
#endif
	ctx->sensor_info_init = 1;
	return 0;
}

/* isp rpbuf */
int isp_load_rpbuf_send(struct hw_isp_device *hw_isp)
{
	int ret;
	const char *buf_name;
	void *buf_va;
	int buf_len, offset;
	int data_len = ISP_LOAD_DRAM_SIZE;

	if (!hw_isp->load_buffer) {
		ISP_ERR("save rpbuf is null\n");
		return -ENOENT;
	}

	buf_name = rpbuf_buffer_name(hw_isp->load_buffer);
	buf_va = rpbuf_buffer_va(hw_isp->load_buffer);
	buf_len = rpbuf_buffer_len(hw_isp->load_buffer);

	if (buf_len < data_len) {
		ISP_ERR("%s: data size(0x%x) out of buffer length(0x%x)\n",
			buf_name, data_len, buf_len);
		return -EINVAL;
	}

	if (!rpbuf_buffer_is_available(hw_isp->load_buffer)) {
		ISP_ERR("%s not available\n", buf_name);
		return -EACCES;
	}

	offset = 0;
	ret = rpbuf_transmit_buffer(hw_isp->load_buffer, offset, data_len);
	if (ret < 0) {
		ISP_ERR("%s: rpbuf_transmit_buffer failed\n", buf_name);
		return ret;
	}

	ISP_LIB_LOG(ISP_LOG_RP, "load buffer send to linux!\n");

	return data_len;
}

static void isp_rpbuf_buffer_available_cb(struct rpbuf_buffer *buffer, void *priv)
{
}

static int isp_rpbuf_buffer_rx_cb(struct rpbuf_buffer *buffer,
		void *data, int data_len, void *priv)
{
	struct hw_isp_device *hw_isp = priv;
	struct isp_lib_context *ctx = isp_dev_get_ctx(hw_isp);
	unsigned int rpmsg_data[5];
	ae_result_t *ae_result = NULL;
	awb_result_t *awb_result = NULL;
	void *buf_va;

	ISP_LIB_LOG(ISP_LOG_RP, "save buffer receive from linux!\n");

	buf_va = rpbuf_buffer_va(buffer);
	isp_stats_process(hw_isp, rpbuf_buffer_va(buffer));

	ae_result = &ctx->ae_entity_ctx.ae_result;
	awb_result = &ctx->awb_entity_ctx.awb_result;
	rpmsg_data[0] = ISP_SET_SENSOR_EXP_GAIN;
	rpmsg_data[1] = ae_result->sensor_set.ev_set_curr.ev_sensor_exp_line;
	rpmsg_data[2] = ae_result->sensor_set.ev_set_curr.ev_analog_gain >> 4;
	rpmsg_data[3] = awb_result->wb_gain_output.r_gain * 256 / awb_result->wb_gain_output.gr_gain;
	rpmsg_data[4] = awb_result->wb_gain_output.b_gain * 256 / awb_result->wb_gain_output.gb_gain;
	isp_rpmsg_ept_send(hw_isp->ept, rpmsg_data, 5*4);

	isp_rpmsg_encpp_send(hw_isp);

	isp_load_rpbuf_send(hw_isp);

	return 0;
}

static int isp_rpbuf_buffer_destroyed_cb(struct rpbuf_buffer *buffer, void *priv)
{
	return 0;
}

static const struct rpbuf_buffer_cbs isp_rpbuf_cbs = {
	.available_cb = isp_rpbuf_buffer_available_cb,
	.rx_cb = isp_rpbuf_buffer_rx_cb,
	.destroyed_cb = isp_rpbuf_buffer_destroyed_cb,
};

static int isp_rpbuf_create(struct hw_isp_device *hw_isp, int controller_id)
{
	const char *load_name, *save_name;
	int load_len = roundup(ISP_RPBUF_LOAD_LEN, 0x1000);
	int save_len = roundup(ISP_RPBUF_SAVE_LEN, 0x1000);
	struct rpbuf_controller *controller = NULL;

	controller = rpbuf_get_controller_by_id(controller_id);
	if (!controller) {
		ISP_ERR("Failed to get controller%d\n", controller_id);
		return -ENOENT;
	}

	if(hw_isp->id == 0){
		load_name = ISP0_RPBUF_LOAD_NAME;
		save_name = ISP0_RPBUF_SAVE_NAME;
	} else if (hw_isp->id == 1) {
		load_name = ISP1_RPBUF_LOAD_NAME;
		save_name = ISP1_RPBUF_SAVE_NAME;
	} else{
		ISP_ERR("hw_isp%d not support rpbuf\n", hw_isp->id);
		return -ENOENT;
	}

	hw_isp->load_buffer = rpbuf_alloc_buffer(controller, load_name, load_len, NULL, NULL, hw_isp);
	if (!hw_isp->load_buffer) {
		ISP_ERR("rpbuf_alloc_buffer load failed\n");
		return -ENOENT;
	}

	hw_isp->save_buffer = rpbuf_alloc_buffer(controller, save_name, save_len, NULL, &isp_rpbuf_cbs, hw_isp);
	if (!hw_isp->save_buffer) {
		ISP_ERR("rpbuf_alloc_buffer save failed\n");
		return -ENOENT;
	}

	return 0;
}

static int isp_rpbuf_destroy(struct hw_isp_device *hw_isp)
{
	int ret;

	ret = rpbuf_free_buffer(hw_isp->load_buffer);
	if (ret < 0) {
		ISP_ERR("rpbuf_free_buffer load failed\n");
		return -ENOENT;
	}

	ret = rpbuf_free_buffer(hw_isp->save_buffer);
	if (ret < 0) {
		ISP_ERR("rpbuf_free_buffer save failed\n");
		return -ENOENT;
	}

	return 0;
}

/* isp rpmsg */
static void isp_rpmsg_destroy(struct hw_isp_device *hw_isp);
int isp_rpmsg_ept_send(struct rpmsg_endpoint *ept, void *data, int len)
{
	int ret = 0;

	if (!ept)
		return -1;

	ISP_LIB_LOG(ISP_LOG_RP, "rpmsg_ept_send, cmd is %d\n", *((unsigned int *)data + 0));

	ret = openamp_rpmsg_send(ept, data, len);
	if (ret < 0) {
		ISP_ERR("rpmsg_ept_send failed, cmd is %d\n", *((unsigned int *)data + 0));
	}

	return ret;
}

static void isp_rpmsg_unbind_callback(struct rpmsg_endpoint *ept)
{

}

static int isp_rpmsg_ept_callback(struct rpmsg_endpoint *ept, void *data,
		size_t len, uint32_t src, void *priv)
{
	struct hw_isp_device *hw_isp = priv;
	struct isp_lib_context *isp_gen = isp_dev_get_ctx(hw_isp);
	unsigned int *p = data;
	unsigned int rpdata[2];

	ISP_LIB_LOG(ISP_LOG_RP, "isp_rpmsg_ept_callback, cmd is %d\n", p[0]);

	switch (p[0]) {
	case VIN_SET_SENSOR_INFO:
		isp_config_sensor_info(hw_isp, data);
		break;
	case VIN_SET_FRAME_SYNC:
		isp_fsync_process(hw_isp, &p[1]);
		break;
	case VIN_SET_IOCTL:
		break;
	case VIN_SET_CLOSE_ISP:
		rpdata[0] = ISP_SET_STAT_EN;
		rpdata[1] = DISABLE;
		isp_rpmsg_ept_send(hw_isp->ept, rpdata, 2*4);

		isp_save_ae_gainexp(hw_isp);

		isp_gen->sensor_info_init = 0;
#if 0 //not be free,next time set VIN_SET_ISP_RESET to open isp server
		isp_rpbuf_destroy(hw_isp);
		isp_rpmsg_destroy(hw_isp);
		isp_server_exit(&hw_isp, hw_isp->id);
#endif
		break;
	case VIN_SET_ISP_RESET:
		isp_version_info();
		isp_config_sensor_info(hw_isp, data);
		while(!isp_gen->sensor_info_init); //wait sensor info init from vin
		//isp_alloc_reg_tbl(hw_isp);

		isp_server_reset(hw_isp, hw_isp->id, 0);

		isp_reg_save_init(isp_gen, 0);
		isp_load_rpbuf_send(hw_isp);

		rpdata[0] = ISP_SET_STAT_EN;
		rpdata[1] = ENABLE;
		isp_rpmsg_ept_send(hw_isp->ept, rpdata, 2*4);
		break;
	}

	return 0;
}

static int isp_rpmsg_create(struct hw_isp_device *hw_isp, int controller_id)
{
	const char *msg_name;
	uint32_t src_addr = RPMSG_ADDR_ANY;
	uint32_t dst_addr = RPMSG_ADDR_ANY;

	if (hw_isp->id == 0)
		msg_name = ISP0_RPMSG_NAME;
	else if (hw_isp->id == 1)
		msg_name = ISP1_RPMSG_NAME;
	else {
		ISP_ERR("hw_isp%d not support rpmsg\n", hw_isp->id);
		return -1;
	}
	hw_isp->ept = openamp_ept_open(msg_name, controller_id, src_addr, dst_addr,
					hw_isp, isp_rpmsg_ept_callback, isp_rpmsg_unbind_callback);
	if(hw_isp->ept == NULL) {
		ISP_ERR("Failed to Create Endpoint\r\n");
		return -1;
	}

	return 0;
}

static void isp_rpmsg_destroy(struct hw_isp_device *hw_isp)
{
	 openamp_ept_close(hw_isp->ept);
}

void isp_reset(struct hw_isp_device *hw_isp)
{
#ifdef CONFIG_ISP_READ_THRESHOLD
	unsigned int data[2];
	struct isp_lib_context *isp_gen = isp_dev_get_ctx(hw_isp);

	isp_rpmsg_create(hw_isp, ISP_RPMSG_CONTROLLER_ID);
	isp_rpbuf_create(hw_isp, ISP_RPBUF_CONTROLLER_ID);

	isp_alloc_reg_tbl(hw_isp);
	isp_reg_save_init(isp_gen, 0);
	isp_load_rpbuf_send(hw_isp);

	isp_get_ae_ev(hw_isp);

	data[0] = ISP_SET_STAT_EN;
	data[1] = ENABLE;
	isp_rpmsg_ept_send(hw_isp->ept, data, 2*4);

	isp_update_encpp_cfg(hw_isp);
	isp_rpmsg_encpp_send(hw_isp);
#elif
	ISP_PRINT("now not support higt frame rate mode!\n");
#endif
}

int isp_init(int argc, const char **argv)
{
	struct hw_isp_device *hw_isp;
	unsigned int isp_id = 0;
	unsigned int data[2];
	struct isp_lib_context *isp_gen;

	hw_isp = isp_server_init(isp_id);

	isp_gen = isp_dev_get_ctx(hw_isp);

	isp_rpmsg_create(hw_isp, ISP_RPMSG_CONTROLLER_ID);
	isp_rpbuf_create(hw_isp, ISP_RPBUF_CONTROLLER_ID);

	data[0] = ISP_REQUEST_SENSOR_INFO;
	isp_rpmsg_ept_send(hw_isp->ept, data, 1*4);
	while(!isp_gen->sensor_info_init);//wait sensor info init from vin

	isp_alloc_reg_tbl(hw_isp);

	isp_server_set_paras(hw_isp, 0, isp_id);

	data[0] = ISP_SET_STAT_EN;
	data[1] = ENABLE;
	isp_rpmsg_ept_send(hw_isp->ept, data, 2*4);

	return 0;
}
FINSH_FUNCTION_EXPORT_ALIAS(isp_init, isp_init, rtthread isp run code);
#endif
