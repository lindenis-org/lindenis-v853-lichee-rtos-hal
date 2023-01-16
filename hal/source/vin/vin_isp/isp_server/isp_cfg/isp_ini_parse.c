/*
 * linux-4.9/drivers/media/platform/sunxi-vin/vin-isp/isp_server/isp_cfg/isp_ini_parse.c
 *
 * Copyright (c) 2007-2022 Allwinnertech Co., Ltd.
 *
 * Authors:  Yang Feng <yangfeng@allwinnertech.com>
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

#include "../include/isp_ini_parse.h"
#include "../include/isp_manage.h"
#include "../include/isp_debug.h"

#if (ISP_VERSION >= 600)
#include "SENSOR_H/gc2053_mipi_default_ini_v853.h"
#include "SENSOR_H/gc4663_120fps_mipi_default_ini_v853.h"
#include "SENSOR_REG_H/gc4663_mipi_2560_1440_15fps_day_reg.h"
#include "SENSOR_REG_H/gc2053_mipi_1080p_day_reg.h"
#endif

unsigned int isp_cfg_log_param = ISP_LOG_CFG;

#define SIZE_OF_LSC_TBL     (12*768*2)
#define SIZE_OF_GAMMA_TBL   (5*1024*3*2)

struct isp_cfg_array cfg_arr[] = {
#if (ISP_VERSION >= 600)
	{"gc4663_mipi", "gc4663_mipi_default_ini_v853", 1920, 1080, 20, 0, 0, &gc2053_mipi_v853_isp_cfg},
	//{"gc4663_mipi", "gc4663_120fps_mipi_default_ini_v853", 1280, 720, 120, 0, 0, &gc4663_120fps_mipi_v853_isp_cfg},
	//{"gc2053_mipi", "gc2053_mipi_default_ini_v853", 1920, 1080, 20, 0, 0, &gc2053_mipi_v853_isp_cfg},
	//{"gc4663_mipi", "gc4663_120fps_mipi_default_ini_v853", 1280, 720, 120, 0, 0, &gc4663_120fps_mipi_v853_isp_cfg},
	//{"gc4663_mipi", "gc4663_mipi_wdr_default_v853", 2560, 1440, 20, 1, 0, &gc4663_mipi_wdr_v853_isp_cfg},
#endif
};

#if 0
int find_nearest_index(int mod, int temp)
{
	int i = 0, index = 0;
	int min_dist = 1 << 30, tmp_dist;
	int temp_array1[4] = {2800, 4000, 5000, 6500};
	int temp_array2[6] = {2200, 2800, 4000, 5000, 5500, 6500};

	if(mod == 1) {
		for(i = 0; i < 4; i++) {
			tmp_dist = temp_array1[i] - temp;
			tmp_dist = (tmp_dist < 0) ? -tmp_dist : tmp_dist;
			if(tmp_dist < min_dist) {
				min_dist = tmp_dist;
				index = i;
			}
			ISP_CFG_LOG(ISP_LOG_CFG, "mode: %d, tmp_dist: %d, min_dist: %d, index: %d.\n", mod, tmp_dist, min_dist, index);
		}
	} else if (mod == 2) {
		for(i = 0; i < 6; i++) {
			tmp_dist = temp_array2[i] - temp;
			tmp_dist = (tmp_dist < 0) ? -tmp_dist : tmp_dist;
			if(tmp_dist < min_dist) {
				min_dist = tmp_dist;
				index = i;
			}
			ISP_CFG_LOG(ISP_LOG_CFG, "mode: %d, tmp_dist: %d, min_dist: %d, index: %d.\n", mod, tmp_dist, min_dist, index);
		}
	} else {
		ISP_ERR("mod error.\n");
	}
	ISP_CFG_LOG(ISP_LOG_CFG, "nearest temp index: %d.\n", index);
	return index;
}

int parser_sync_info(struct isp_param_config *param, char *isp_cfg_name, int isp_id)
{
	FILE *file_fd = NULL;
	char fdstr[50];
	int sync_info[775];
	int lsc_ind = 0, lsc_cnt = 0, lsc_tab_cnt = 0;
	int version_num = 0;
	int lsc_temp = 0;
	int enable = 0;

	sprintf(fdstr, "/mnt/extsd/%s_%d.bin", isp_cfg_name, isp_id);
	file_fd = fopen(fdstr, "rb");
	if (file_fd == NULL) {
		ISP_ERR("open bin failed.\n");
		return -1;
	} else {
		fread(&sync_info[0], sizeof(int)*775, 1, file_fd);
	}
	fclose(file_fd);

	version_num = sync_info[0];
	enable = sync_info[1];

	ISP_CFG_LOG(ISP_LOG_CFG, "%s enable mode = %d.\n", __func__, enable);

	if (0 == enable) {
		return 0;
	} else if (1 == enable) {
		memcpy(param->isp_tunning_settings.bayer_gain, &sync_info[2], sizeof(int)*ISP_RAW_CH_MAX);
		return 0;
	} else if (2 == enable) {
		goto lsc_tbl;
	}
	memcpy(param->isp_tunning_settings.bayer_gain, &sync_info[2], sizeof(int)*ISP_RAW_CH_MAX);

	ISP_CFG_LOG(ISP_LOG_CFG, "%s bayer_gain: %d, %d, %d, %d.\n", __func__,
		param->isp_tunning_settings.bayer_gain[0], param->isp_tunning_settings.bayer_gain[1],
		param->isp_tunning_settings.bayer_gain[2], param->isp_tunning_settings.bayer_gain[3]);
lsc_tbl:
	lsc_temp = sync_info[6];
	lsc_ind = find_nearest_index(param->isp_tunning_settings.ff_mod, lsc_temp);
	ISP_CFG_LOG(ISP_LOG_CFG, "%s lsc_ind: %d.\n", __func__, lsc_ind);

	if(param->isp_tunning_settings.ff_mod == 1) {
		for(lsc_tab_cnt = 0; lsc_tab_cnt < 4; lsc_tab_cnt++) {
			if(lsc_tab_cnt == lsc_ind)
				continue;
			for(lsc_cnt = 0; lsc_cnt < 768; lsc_cnt++) {
				param->isp_tunning_settings.lsc_tbl[lsc_tab_cnt][lsc_cnt]
					= sync_info[7+lsc_cnt]*param->isp_tunning_settings.lsc_tbl[lsc_tab_cnt][lsc_cnt]/param->isp_tunning_settings.lsc_tbl[lsc_ind][lsc_cnt];
			}
		}

		for(lsc_tab_cnt = 4; lsc_tab_cnt < 8; lsc_tab_cnt++) {
			if(lsc_tab_cnt == (lsc_ind+4))
				continue;
			for(lsc_cnt = 0; lsc_cnt < 768; lsc_cnt++) {
				param->isp_tunning_settings.lsc_tbl[lsc_tab_cnt][lsc_cnt]
					= sync_info[7+lsc_cnt]*param->isp_tunning_settings.lsc_tbl[lsc_tab_cnt][lsc_cnt]/param->isp_tunning_settings.lsc_tbl[lsc_ind+4][lsc_cnt];
			}
		}
		for(lsc_cnt = 0; lsc_cnt < 768; lsc_cnt++)
			param->isp_tunning_settings.lsc_tbl[lsc_ind][lsc_cnt] = param->isp_tunning_settings.lsc_tbl[lsc_ind+4][lsc_cnt]
				= sync_info[7+lsc_cnt];

		ISP_CFG_LOG(ISP_LOG_CFG, "%s lsc_tbl_1 0: %d, 1: %d, 766: %d, 767: %d.\n", __func__,
			param->isp_tunning_settings.lsc_tbl[1][0], param->isp_tunning_settings.lsc_tbl[1][1],
			param->isp_tunning_settings.lsc_tbl[1][766], param->isp_tunning_settings.lsc_tbl[1][767]);
	} else if(param->isp_tunning_settings.ff_mod == 2) {
		for(lsc_tab_cnt = 0; lsc_tab_cnt < 6; lsc_tab_cnt++) {
			if(lsc_tab_cnt == lsc_ind)
				continue;
			for(lsc_cnt = 0; lsc_cnt < 768; lsc_cnt++) {
				if(param->isp_tunning_settings.lsc_tbl[lsc_ind][lsc_cnt] == 0) {
					ISP_ERR("lsc_ind: %d, lsc_cnt: %d is zero.\n", lsc_ind, lsc_cnt);
					continue;
				} else
					param->isp_tunning_settings.lsc_tbl[lsc_tab_cnt][lsc_cnt]
						= sync_info[7+lsc_cnt]*param->isp_tunning_settings.lsc_tbl[lsc_tab_cnt][lsc_cnt]/param->isp_tunning_settings.lsc_tbl[lsc_ind][lsc_cnt];
				if(param->isp_tunning_settings.lsc_tbl[lsc_tab_cnt][lsc_cnt] == 0) {
					ISP_ERR("result------>lsc_ind: %d, lsc_cnt: %d is zero.\n", lsc_tab_cnt, lsc_cnt);
				}
			}
		}

		for(lsc_tab_cnt = 6; lsc_tab_cnt < 12; lsc_tab_cnt++) {
			if(lsc_tab_cnt == (lsc_ind+6))
				continue;
			for(lsc_cnt = 0; lsc_cnt < 768; lsc_cnt++) {
				if(param->isp_tunning_settings.lsc_tbl[lsc_ind + 7][lsc_cnt] == 0) {
					ISP_ERR("lsc_ind: %d, lsc_cnt: %d is zero.\n", lsc_ind+6, lsc_cnt);
					continue;
				} else
					param->isp_tunning_settings.lsc_tbl[lsc_tab_cnt][lsc_cnt]
						= sync_info[7+lsc_cnt]*param->isp_tunning_settings.lsc_tbl[lsc_tab_cnt][lsc_cnt]/param->isp_tunning_settings.lsc_tbl[lsc_ind+6][lsc_cnt];
				if(param->isp_tunning_settings.lsc_tbl[lsc_tab_cnt][lsc_cnt] == 0) {
					ISP_ERR("result------>lsc_ind: %d, lsc_cnt: %d is zero.\n", lsc_tab_cnt, lsc_cnt);
				}
			}
		}
		for(lsc_cnt = 0; lsc_cnt < 768; lsc_cnt++)
			param->isp_tunning_settings.lsc_tbl[lsc_ind][lsc_cnt] = param->isp_tunning_settings.lsc_tbl[lsc_ind+6][lsc_cnt]
				= sync_info[7+lsc_cnt];

		ISP_CFG_LOG(ISP_LOG_CFG, "%s lsc_tbl_1 0: %d, 1: %d, 766: %d, 767: %d.\n", __func__,
			param->isp_tunning_settings.lsc_tbl[1][0], param->isp_tunning_settings.lsc_tbl[1][1],
			param->isp_tunning_settings.lsc_tbl[1][766], param->isp_tunning_settings.lsc_tbl[1][767]);
	} else {
		ISP_ERR("isp ff_mod error.\n");
	}
	return 0;
}

int isp_save_tbl(struct isp_param_config *param, char *tbl_patch)
{
	FILE *file_fd = NULL;
	char fdstr[50];

	sprintf(fdstr, "%s/gamma_tbl.bin", tbl_patch);
	file_fd = fopen(fdstr, "wb");
	if (file_fd == NULL) {
		ISP_WARN("open %s failed!!!\n", fdstr);
		return -1;
	} else {
		fwrite(param->isp_tunning_settings.gamma_tbl_ini, SIZE_OF_GAMMA_TBL, 1, file_fd);
		ISP_PRINT("save isp_ctx to %s success!!!\n", fdstr);
	}
	fclose(file_fd);

	sprintf(fdstr, "%s/lsc_tbl.bin", tbl_patch);
	file_fd = fopen(fdstr, "wb");
	if (file_fd == NULL) {
		ISP_WARN("open %s failed!!!\n", fdstr);
		return -1;
	} else {
		fwrite(param->isp_tunning_settings.lsc_tbl, SIZE_OF_LSC_TBL, 1, file_fd);
		ISP_PRINT("save isp_ctx to %s success!!!\n", fdstr);
	}
	fclose(file_fd);

	sprintf(fdstr, "%s/cem_tbl.bin", tbl_patch);
	file_fd = fopen(fdstr, "wb");
	if (file_fd == NULL) {
		ISP_WARN("open %s failed!!!\n", fdstr);
		return -1;
	} else {
		fwrite(param->isp_tunning_settings.isp_cem_table, ISP_CEM_MEM_SIZE, 1, file_fd);
		ISP_PRINT("save isp_ctx to %s success!!!\n", fdstr);
	}
	fclose(file_fd);

	sprintf(fdstr, "%s/cem_tbl.bin", tbl_patch);
	file_fd = fopen(fdstr, "wb");
	if (file_fd == NULL) {
		ISP_WARN("open %s failed!!!\n", fdstr);
		return -1;
	} else {
		fwrite(param->isp_tunning_settings.isp_cem_table1, ISP_CEM_MEM_SIZE, 1, file_fd);
		ISP_PRINT("save isp_ctx to %s success!!!\n", fdstr);
	}
	fclose(file_fd);

	return 0;
}
#endif

int parser_ini_info(struct isp_param_config *param, char *sensor_name,
			int w, int h, int fps, int wdr, int ir, int sync_mode, int isp_id)
{
	int i;
	struct isp_cfg_pt *cfg = NULL;

	//load header parameter
	for (i = 0; i < ARRAY_SIZE(cfg_arr); i++) {
		if (!strncmp(sensor_name, cfg_arr[i].sensor_name, 6) &&
		    (w == cfg_arr[i].width) && (h == cfg_arr[i].height) &&
		    (fps == cfg_arr[i].fps) && (wdr == cfg_arr[i].wdr) &&
		    (ir == cfg_arr[i].ir)) {
				cfg = cfg_arr[i].cfg;
				ISP_PRINT("find %s_%d_%d_%d_%d [%s] isp config\n", cfg_arr[i].sensor_name,
					cfg_arr[i].width, cfg_arr[i].height, cfg_arr[i].fps, cfg_arr[i].wdr, cfg_arr[i].isp_cfg_name);
				break;
		}
	}

	if (i == ARRAY_SIZE(cfg_arr)) {
		for (i = 0; i < ARRAY_SIZE(cfg_arr); i++) {
			if (!strncmp(sensor_name, cfg_arr[i].sensor_name, 6) && (wdr == cfg_arr[i].wdr)) {
				cfg = cfg_arr[i].cfg;
				ISP_WARN("cannot find %s_%d_%d_%d_%d_%d isp config, use %s_%d_%d_%d_%d_%d -> [%s]\n", sensor_name, w, h, fps, wdr, ir,
					cfg_arr[i].sensor_name,	cfg_arr[i].width, cfg_arr[i].height, cfg_arr[i].fps, cfg_arr[i].wdr,
					cfg_arr[i].ir, cfg_arr[i].isp_cfg_name);
				break;
			}
		}
		for (i = 0; i < ARRAY_SIZE(cfg_arr); i++) {
			if (wdr == cfg_arr[i].wdr) {
				cfg = cfg_arr[i].cfg;
				ISP_WARN("cannot find %s_%d_%d_%d_%d_%d isp config, use %s_%d_%d_%d_%d_%d -> [%s]\n", sensor_name, w, h, fps, wdr, ir,
					cfg_arr[i].sensor_name,	cfg_arr[i].width, cfg_arr[i].height, cfg_arr[i].fps, cfg_arr[i].wdr,
					cfg_arr[i].ir, cfg_arr[i].isp_cfg_name);
				break;
			}
		}
		if (i == ARRAY_SIZE(cfg_arr)) {
			ISP_WARN("cannot find %s_%d_%d_%d_%d_%d isp config, use default config [%s]\n",
				sensor_name, w, h, fps, wdr, ir, cfg_arr[i-1].isp_cfg_name);
			cfg = cfg_arr[i-1].cfg;// use the last one
		}
	}

	if (cfg != NULL) {
		param->isp_test_settings = *cfg->isp_test_settings;
		param->isp_3a_settings = *cfg->isp_3a_settings;
		param->isp_iso_settings = *cfg->isp_iso_settings;
		param->isp_tunning_settings = *cfg->isp_tunning_settings;
	}

	//isp_save_tbl(param, "/mnt/extsd");

	//if(sync_mode)
	//	parser_sync_info(param, cfg_arr[i].isp_cfg_name, isp_id);

	return 0;
}

struct isp_reg_array reg_arr[] = {
#if (ISP_VERSION >= 600)
	{"gc4663_mipi", "gc4663_mipi_1440p_15fps_day_reg", 2560, 1440, 15, 0, 0, &gc4663_mipi_isp_day_reg},
	{"gc2053_mipi", "gc2053_mipi_1800p_20fps_day_reg", 1920, 1088, 20, 0, 0, &gc2053_mipi_isp_day_reg},
#endif
};

int parser_ini_regs_info(struct isp_lib_context *ctx, char *sensor_name,
			int w, int h, int fps, int wdr, int ir)
{
	int i;
	struct isp_reg_pt *reg = NULL;

	for (i = 0; i < ARRAY_SIZE(reg_arr); i++) {
		if (!strncmp(sensor_name, reg_arr[i].sensor_name, 6) &&
			(w == reg_arr[i].width) && (h == reg_arr[i].height) &&// (fps == reg_arr[i].fps) &&
			(wdr == reg_arr[i].wdr)) {

			if (reg_arr[i].ir == ir) {
				reg = reg_arr[i].reg;
				ISP_PRINT("find %s_%d_%d_%d_%d ---- [%s] isp reg\n", reg_arr[i].sensor_name,
					reg_arr[i].width, reg_arr[i].height, reg_arr[i].fps, reg_arr[i].wdr, reg_arr[i].isp_cfg_name);
				break;
			}
		}
	}

	if (i == ARRAY_SIZE(reg_arr)) {
		ISP_WARN("cannot find %s_%d_%d_%d_%d_%d isp reg!!!\n", sensor_name, w, h, fps, wdr, ir);
		return -1;
	}

	if (reg != NULL) {
		if (reg->isp_save_reg)
			memcpy(ctx->load_reg_base, reg->isp_save_reg, ISP_LOAD_REG_SIZE);
		if (reg->isp_save_fe_table)
			memcpy(ctx->module_cfg.fe_table, reg->isp_save_fe_table, ISP_FE_TABLE_SIZE);
		if (reg->isp_save_bayer_table)
			memcpy(ctx->module_cfg.bayer_table, reg->isp_save_bayer_table, ISP_BAYER_TABLE_SIZE);
		if (reg->isp_save_rgb_table)
			memcpy(ctx->module_cfg.rgb_table, reg->isp_save_rgb_table, ISP_RGB_TABLE_SIZE);
		if (reg->isp_save_yuv_table)
			memcpy(ctx->module_cfg.yuv_table, reg->isp_save_yuv_table, ISP_YUV_TABLE_SIZE);
	}

	return 0;
}
