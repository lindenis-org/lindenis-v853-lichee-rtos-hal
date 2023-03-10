/*
 * top_reg.h for all v4l2 subdev manage
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zhao Wei <zhaowei@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __CSIC__TOP__REG__H__
#define __CSIC__TOP__REG__H__

/*register value*/

/*register data struct*/

struct csic_feature_list {
	unsigned int dma_num;
	unsigned int vipp_num;
	unsigned int isp_num;
	unsigned int ncsi_num;
	unsigned int mcsi_num;
	unsigned int parser_num;
};

struct csic_version {
	unsigned int ver_big;
	unsigned int ver_small;
};

int csic_top_set_base_addr(unsigned long addr);
void csic_top_enable(void);
void csic_top_disable(void);
void csic_isp_bridge_enable(void);
void csic_isp_bridge_disable(void);
void csic_top_sram_pwdn(unsigned int en);
void csic_top_version_read_en(unsigned int en);

void csic_isp_input_select(unsigned int isp, unsigned int in,
				unsigned int psr, unsigned int ch);
void csic_vipp_input_select(unsigned int vipp,
				unsigned int isp, unsigned int ch);

void csic_feature_list_get(struct csic_feature_list *fl);
void csic_version_get(struct csic_version *v);
void csic_mbus_req_mex_set(unsigned int data);
void csic_ptn_generation_en(unsigned int en);
void csic_ptn_control(int mode, int dw, int port);
void csic_ptn_length(unsigned int len);
void csic_ptn_addr(unsigned long dma_addr);
void csic_ptn_size(unsigned int w, unsigned int h);

/*
 * functions about ccu register
 */
int csic_ccu_set_base_addr(unsigned long addr);
void csic_ccu_clk_gating_enable(void);
void csic_ccu_clk_gating_disable(void);
void csic_ccu_mcsi_clk_mode(unsigned int mode);
void csic_ccu_mcsi_combo_clk_en(unsigned int sel, unsigned int en);
void csic_ccu_mcsi_mipi_clk_en(unsigned int sel, unsigned int en);
void csic_ccu_mcsi_parser_clk_en(unsigned int sel, unsigned int en);
void csic_ccu_misp_isp_clk_en(unsigned int sel, unsigned int en);
void csic_ccu_mcsi_post_clk_enable(unsigned int sel);
void csic_ccu_mcsi_post_clk_disable(unsigned int sel);
void csic_ccu_bk_clk_en(unsigned int sel, unsigned int en);
void csic_ccu_vipp_clk_en(unsigned int sel, unsigned int en);

#endif /* __CSIC__TOP__REG__H__ */
