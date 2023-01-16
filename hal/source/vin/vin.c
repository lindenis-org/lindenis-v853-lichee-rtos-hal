/*
 * vin.c
 *
 * Copyright (c) 2018 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zequn Zheng <zequnzhengi@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <hal_clk.h>
#include <hal_timer.h>
#include <openamp/sunxi_helper/openamp.h>

#include "vin.h"

struct rt_memheap isp_mempool;

unsigned int vin_log_mask;// = 0xffff - VIN_LOG_ISP - VIN_LOG_STAT - VIN_LOG_STAT1;
unsigned int wdr_on = 0;
extern struct csi_dev *global_csi[VIN_MAX_CSI];
extern struct vin_core *global_vinc[VIN_MAX_VIDEO];

static int vin_md_clk_en(unsigned int en)
{
	struct vin_clk_info top_clk_src;

	hal_writel(0x00000001, 0x02001c2c);
	hal_writel(0x00010001, 0x02001c2c);

	if (en) {
		if (vind_default_clk[VIN_TOP_CLK].frequency > 300000000) {
			top_clk_src.clock = vind_default_clk[VIN_TOP_CLK_SRC].clock;
			top_clk_src.frequency = VIN_PLL_CSI_RATE;

		} else {
			top_clk_src.clock = vind_default_clk[VIN_TOP_CLK_SRC1].clock;
			top_clk_src.frequency = vind_default_clk[VIN_TOP_CLK_SRC1].frequency;
		}

		if (hal_clk_set_parent(vind_default_clk[VIN_TOP_CLK].clock, top_clk_src.clock)) {
			vin_err("set top_clk source failed!\n");
			return -1;
		}

		if (hal_clk_set_rate(top_clk_src.clock, top_clk_src.frequency)) {
			vin_err("set top_clk src clock error\n");
			//return -1;
		}

		if (hal_clk_set_rate(vind_default_clk[VIN_TOP_CLK].clock, vind_default_clk[VIN_TOP_CLK].frequency)) {
			vin_err("set top_clk clock error\n");
			return -1;
		}
	}

	if (en) {
		if (hal_clock_enable(vind_default_clk[VIN_TOP_CLK].clock)) {
			vin_err("top_clk clock enable error\n");
			return -1;
		}

		if (hal_clock_enable(vind_default_isp_clk[VIN_ISP_CLK].clock)) {
			vin_err("isp clock enable error\n");
			return -1;
		}
	} else {
		if (hal_clock_disable(vind_default_clk[VIN_TOP_CLK].clock)) {
			vin_err("top_clk clock disable error\n");
			return -1;
		}

		if (hal_clock_disable(vind_default_isp_clk[VIN_ISP_CLK].clock)) {
			vin_err("isp clock disable error\n");
			return -1;
		}
	}

	hal_writel(0x00000001, 0x02001c2c);
	hal_writel(0x00010001, 0x02001c2c);

	vin_print("set clk end\n");

	return 0;
}

static void vin_ccu_clk_gating_en(unsigned int en)
{
	/*if (en) {
		csic_ccu_clk_gating_enable();
		csic_ccu_mcsi_clk_mode(1);
		csic_ccu_mcsi_post_clk_enable(0);
		csic_ccu_mcsi_post_clk_enable(1);
	} else {
		csic_ccu_mcsi_post_clk_disable(1);
		csic_ccu_mcsi_post_clk_disable(0);
		csic_ccu_mcsi_clk_mode(0);
		csic_ccu_clk_gating_disable();
	}*/

	csic_ccu_clk_gating_disable();
}

static void vin_md_set_power(int on)
{
	if (on) {
		vin_md_clk_en(on);
		vin_ccu_clk_gating_en(on);
		hal_usleep(120);
		csic_top_enable();
		csic_mbus_req_mex_set(0xf);
	} else {
		csic_top_disable();
		vin_ccu_clk_gating_en(on);
		vin_md_clk_en(on);
	}
}

static void vin_subdev_ccu_en(unsigned int id, unsigned int en)
{

}

static int vin_pipeline_set_mbus_config(unsigned int id)
{
#if 1
	struct vin_core *vinc = global_vinc[id];
	struct v4l2_mbus_config mcfg;
	struct mbus_framefmt_res res;
	int sensor_id = vinc->mipi_sel;


	global_sensors[sensor_id].sensor_core->g_mbus_config(&mcfg, &res);

	/* s_mbus_config on all mipi and csi */
	if (vinc->mipi_sel != 0xff)
		sunxi_mipi_s_mbus_config(vinc->mipi_sel, &mcfg, &res);

	sunxi_csi_s_mbus_config(vinc->csi_sel, &mcfg);
	vinc->total_rx_ch = global_csi[vinc->csi_sel]->bus_info.ch_total_num;

	if (vinc->tdm_rx_sel != 0xff)
		sunxi_tdm_s_mbus_config(vinc->tdm_rx_sel, &res);
	sunxi_isp_s_mbus_config(vinc->isp_sel, &res);
#endif

	return 0;
}


static void sunxi_vin_reset(unsigned int id, int enable)
{
#if 1
	struct vin_core *vinc = global_vinc[id];
	struct prs_cap_mode mode = {.mode = VCAP};
#endif
}

static void vin_s_stream(unsigned int id, int enable)
{
#if 1
	struct vin_core *vinc = global_vinc[id];
	int sensor_id = vinc->mipi_sel;

	vin_log(VIN_LOG_MD, "vinc%d:tdm_rx_sel = %d,  mipi_sel = %d, isp_sel = %d, vincid = %d, csi_sel = %d\n",
			id, vinc->tdm_rx_sel, vinc->mipi_sel, vinc->isp_sel, vinc->id, vinc->csi_sel);

	if (enable) {
		if (vinc->tdm_rx_sel != 0xff)
			sunxi_tdm_subdev_s_stream(vinc->mipi_sel, vinc->id, enable);
		hal_usleep(120);
		if (vinc->mipi_sel != 0xff)
			sunxi_mipi_subdev_s_stream(vinc->mipi_sel, enable);
		hal_usleep(120);
		sunxi_isp_subdev_s_stream(vinc->isp_sel, vinc->id, enable);
		hal_usleep(120);
		sunxi_scaler_subdev_s_stream(vinc->id, enable);
		hal_usleep(120);
		vin_subdev_s_stream(vinc->id, enable);
		hal_usleep(120);
		sunxi_csi_subdev_s_stream(vinc->csi_sel, enable);
		hal_usleep(120);
		global_sensors[sensor_id].sensor_core->s_stream(sensor_id, enable);
	} else {
		vin_subdev_s_stream(vinc->id, enable);
		hal_usleep(120);
		sunxi_scaler_subdev_s_stream(vinc->id, enable);
		hal_usleep(120);
		sunxi_isp_subdev_s_stream(vinc->isp_sel, vinc->id, enable);
		hal_usleep(120);
		if (vinc->tdm_rx_sel != 0xff)
			sunxi_tdm_subdev_s_stream(vinc->mipi_sel, vinc->id, enable);
		hal_usleep(120);
		sunxi_csi_subdev_s_stream(vinc->csi_sel, enable);
#ifndef ISP_SERVER_FASTINIT
		hal_usleep(120);
		if (vinc->mipi_sel != 0xff)
			sunxi_mipi_subdev_s_stream(vinc->mipi_sel, enable);
		hal_usleep(120);
		global_sensors[sensor_id].sensor_core->s_stream(sensor_id, enable);
#endif
	}
#endif
}

static void vin_probe(unsigned int id)
{
#if 1
	struct vin_core *vinc = &global_video[id];

	if (vinc->mipi_sel != 0xff)
		mipi_probe(vinc->mipi_sel);
	csi_probe(vinc->csi_sel);
	if (vinc->tdm_rx_sel != 0xff)
		tdm_probe(vinc->mipi_sel);
	isp_probe(vinc->isp_sel);
	scaler_probe(vinc->id);
	vin_core_probe(vinc->id);
#endif
}

void vin_free(unsigned int id)
{
#if 1
	struct vin_core *vinc = &global_video[id];

	if (vinc->mipi_sel != 0xff)
		mipi_remove(vinc->mipi_sel);
	csi_remove(vinc->csi_sel);
	if (vinc->tdm_rx_sel != 0xff)
		tdm_remove(vinc->mipi_sel);
	isp_remove(vinc->isp_sel);
	scaler_remove(vinc->id);
	vin_core_remove(vinc->id);
#endif
}

int vin_g_status(void)
{
	int i, ret = -1;

	for (i = 0; i < VIN_MAX_VIDEO; i++) {
		if (global_video[i].used == 1)
			ret = 0;
	}

	return ret;
}

int csi_init(int argc, const char **argv)
{
	struct vin_core *vinc = NULL;
	unsigned long reg_base;
	unsigned long ccu_base;
	int ret;
	int i, j;
	int sensor_id;

	vin_log(VIN_LOG_MD, "CSI start!\n");

	ret = vin_g_status();
	if (ret != 0) {
		vin_err("There is no open CSI\n");
		return -1;
	}

	rt_memheap_init(&isp_mempool, "isp-mempool", (void *)MEMRESERVE, MEMRESERVE_SIZE);

	reg_base = sunxi_vin_get_top_base();
	csic_top_set_base_addr(reg_base);
	vin_log(VIN_LOG_MD, "reg is 0x%lx\n", reg_base);

	ccu_base = sunxi_vin_get_ccu_base();
	csic_ccu_set_base_addr(ccu_base);
	vin_log(VIN_LOG_MD, "reg is 0x%lx\n", ccu_base);

	vin_md_set_power(PWR_ON);

	sunxi_twi_init(0);
	sunxi_twi_init(1);

#if 0
	i = 0;
	ret = global_sensors[i].sensor_core->sensor_test_i2c(i);
	if (ret)
		return -1;
#endif
	//vin_probe(0);
	for (i = 0; i < VIN_MAX_VIDEO; i++) {
		if (global_video[i].used == 1) {
			vin_probe(i);
			vinc = global_vinc[i];

			vin_subdev_ccu_en(i, PWR_ON);

			vin_pipeline_set_mbus_config(i);
			for (j = 0; j < vinc->total_rx_ch; j++) {
				csic_isp_input_select(vinc->isp_sel/ISP_VIRT_NUM, vinc->isp_sel%ISP_VIRT_NUM + j, vinc->csi_sel, j);
			}
			csic_vipp_input_select(vinc->vipp_sel/VIPP_VIRT_NUM, vinc->isp_sel/ISP_VIRT_NUM, vinc->isp_tx_ch);

			sensor_id = vinc->mipi_sel;
			global_sensors[sensor_id].sensor_core->sensor_power(sensor_id, PWR_ON);
			vin_s_stream(i, PWR_ON);
		}
	}

	for (i = 0; i < VIN_MAX_VIDEO; i++) {
		if (global_video[i].used == 1) {
			vinc = &global_video[i];

			while (sunxi_isp_ae_done(vinc->isp_sel, 8) != 0);
			vin_print("close vdieo%d\n", vinc->id);

			vin_s_stream(i, PWR_OFF);
			vin_free(i);
		}
	}
	//vin_free(0);

	sunxi_twi_exit(0);
	sunxi_twi_exit(1);

	rt_memheap_detach(&isp_mempool);

	openamp_init(); //wait rpmsg init

	for (i = 0; i < VIN_MAX_VIDEO; i++) {
		if (global_video[i].used == 1) {
			vinc = &global_video[i];

			sunxi_isp_reset_server(vinc->isp_sel);
		}
	}

	vin_log(VIN_LOG_MD, "GoodBye CSI!\n");
	return 0;
}
FINSH_FUNCTION_EXPORT_ALIAS(csi_init, csi_init, rtthread vin run code);

int vin_exit(void)
{
	/* sunxi_vin_reset(0); */
	/*sensor_power(PWR_OFF); */
	/*vin_md_set_power(PWR_OFF);*/
	/* vin_free(); */

	return 0;
}
