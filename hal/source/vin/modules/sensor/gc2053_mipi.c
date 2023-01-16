/*
 * A V4L2 driver for Raw cameras.
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zhao Wei <zhaowei@allwinnertech.com>
 *    Liang WeiJie <liangweijie@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <hal_timer.h>

#include "../../vin_mipi/combo_common.h"
#include "camera.h"
#include "../../utility/sunxi_camera_v2.h"
#include "../../utility/media-bus-format.h"
#include "../../utility/vin_supply.h"

#define MCLK              (24*1000*1000)
#define V4L2_IDENT_SENSOR  0x2053

//define the registers
#define EXP_HIGH		0xff
#define EXP_MID			0x03
#define EXP_LOW			0x04
#define GAIN_HIGH		0xff
#define GAIN_LOW		0x24
/*
 * Our nominal (default) frame rate.
 */
#define ID_REG_HIGH		0xf0
#define ID_REG_LOW		0xf1
#define ID_VAL_HIGH		((V4L2_IDENT_SENSOR) >> 8)
#define ID_VAL_LOW		((V4L2_IDENT_SENSOR) & 0xff)
#define SENSOR_FRAME_RATE 20

/*
 * The GC2053 i2c address
 */
#define I2C_ADDR 0x6e

#define SENSOR_NUM 0x2
#define SENSOR_NAME "gc2053_mipi"
#define SENSOR_NAME_2 "gc2053_mipi_2"

static int sensor_power_count[2];
static int sensor_stream_count[2];

/*
 * The default register settings
 */

static struct regval_list sensor_default_regs[] = {

};

/*
 * window_size=1920*1080 mipi@2lane
 * mclk=24mhz, mipi_clk=594Mbps
 * pixel_line_total=3300, line_frame_total=1125
 * row_time=44.44us, frame_rate=20fps
 */
static struct regval_list sensor_1080p20_regs[] = {
	/*  1928*1088@20fps */
	/****system****/
	{0xfe, 0x80},
	{0xfe, 0x80},
	{0xfe, 0x80},
	{0xfe, 0x00},
	{0xf2, 0x00},
	{0xf3, 0x00},
	{0xf4, 0x36},
	{0xf5, 0xc0},
	{0xf6, 0x44},
	{0xf7, 0x01},
	{0xf8, 0x63},
	{0xf9, 0x40},
	{0xfc, 0x8e},
/****CISCTL & ANALOG****/
	{0xfe, 0x00},
	{0x87, 0x18},
	{0xee, 0x30},
	{0xd0, 0xb7},
	{0x03, 0x04},
	{0x04, 0x60},
	{0x05, 0x06},
	{0x06, 0x72},
	{0x07, 0x00},
	{0x08, 0x11},
	{0x09, 0x00},
	{0x0a, 0x02},
	{0x0b, 0x00},
	{0x0c, 0x02},
	{0x0d, 0x04},
	{0x0e, 0x48},//{0x0e, 0x40},
	{0x12, 0xe2},
	{0x13, 0x16},
	{0x19, 0x0a},
	{0x21, 0x1c},
	{0x28, 0x0a},
	{0x29, 0x24},
	{0x2b, 0x04},
	{0x32, 0xf8},
	{0x37, 0x03},
	{0x39, 0x15},
	{0x43, 0x07},
	{0x44, 0x40},
	{0x46, 0x0b},
	{0x4b, 0x20},
	{0x4e, 0x08},
	{0x55, 0x20},
	{0x66, 0x05},
	{0x67, 0x05},
	{0x77, 0x01},
	{0x78, 0x00},
	{0x7c, 0x93},
	{0x8c, 0x12},
	{0x8d, 0x92},
	{0x90, 0x01},
	{0x9d, 0x10},
	{0xce, 0x7c},
	{0xd2, 0x41},
	{0xd3, 0xdc},
	{0xe6, 0x50},
	/*gain*/
	{0xb6, 0xc0},
	{0xb0, 0x70},
	{0xb1, 0x01},
	{0xb2, 0x00},
	{0xb3, 0x00},
	{0xb4, 0x00},
	{0xb8, 0x01},
	{0xb9, 0x00},
	/*blk*/
	{0x26, 0x30},
	{0xfe, 0x01},
	{0x40, 0x23},
	{0x55, 0x07},
	{0x60, 0x40},
	{0xfe, 0x04},
	{0x14, 0x78},
	{0x15, 0x78},
	{0x16, 0x78},
	{0x17, 0x78},
	/*window*/
	{0xfe, 0x01},
	{0x92, 0x00},
	{0x94, 0x03},
	{0x95, 0x04},
	{0x96, 0x48},//{0x96, 0x40},
	{0x97, 0x07},
	{0x98, 0x88},
	/*ISP*/
	{0xfe, 0x01},
	{0x01, 0x05},
	{0x02, 0x89},
	{0x04, 0x01},
	{0x07, 0xa6},
	{0x08, 0xa9},
	{0x09, 0xa8},
	{0x0a, 0xa7},
	{0x0b, 0xff},
	{0x0c, 0xff},
	{0x0f, 0x00},
	{0x50, 0x1c},
	{0x89, 0x03},
	{0xfe, 0x04},
	{0x28, 0x86},
	{0x29, 0x86},
	{0x2a, 0x86},
	{0x2b, 0x68},
	{0x2c, 0x68},
	{0x2d, 0x68},
	{0x2e, 0x68},
	{0x2f, 0x68},
	{0x30, 0x4f},
	{0x31, 0x68},
	{0x32, 0x67},
	{0x33, 0x66},
	{0x34, 0x66},
	{0x35, 0x66},
	{0x36, 0x66},
	{0x37, 0x66},
	{0x38, 0x62},
	{0x39, 0x62},
	{0x3a, 0x62},
	{0x3b, 0x62},
	{0x3c, 0x62},
	{0x3d, 0x62},
	{0x3e, 0x62},
	{0x3f, 0x62},
	/****DVP & MIPI****/
	{0xfe, 0x01},
	{0x9a, 0x06},
	{0xfe, 0x00},
	{0x7b, 0x2a},
	{0x23, 0x2d},
	{0xfe, 0x03},
	{0x01, 0x27},
	{0x02, 0x56},
	{0x03, 0x8e},
	{0x12, 0x80},
	{0x13, 0x07},
	{0x15, 0x12},
	{0xfe, 0x00},
	{0x17, 0x83},
	{0x3e, 0x91},
	/* close rgb adapt*/
//	{0xfe, 0x01},
//	{0x99, 0x00},
//	{0xfe, 0x00},
};

/*
 * window_size=1920*1080 mipi@2lane
 * mclk=24mhz, mipi_clk=594Mbps
 * pixel_line_total=2640, line_frame_total=1125
 * row_time=35.55us, frame_rate=25fps
 */
static struct regval_list sensor_1080p25_regs[] = {
	/*  1928*1088@25fps */
	/****system****/
	{0xfe, 0x80},
	{0xfe, 0x80},
	{0xfe, 0x80},
	{0xfe, 0x00},
	{0xf2, 0x00},
	{0xf3, 0x00},
	{0xf4, 0x36},
	{0xf5, 0xc0},
	{0xf6, 0x44},
	{0xf7, 0x01},
	{0xf8, 0x63},
	{0xf9, 0x40},
	{0xfc, 0x8e},
/****CISCTL & ANALOG****/
	{0xfe, 0x00},
	{0x87, 0x18},
	{0xee, 0x30},
	{0xd0, 0xb7},
	{0x03, 0x04},
	{0x04, 0x60},
	{0x05, 0x05},
	{0x06, 0x28},
	{0x07, 0x00},
	{0x08, 0x11},
	{0x09, 0x00},
	{0x0a, 0x02},
	{0x0b, 0x00},
	{0x0c, 0x02},
	{0x0d, 0x04},
	{0x0e, 0x48},//{0x0e, 0x40},
	{0x12, 0xe2},
	{0x13, 0x16},
	{0x19, 0x0a},
	{0x21, 0x1c},
	{0x28, 0x0a},
	{0x29, 0x24},
	{0x2b, 0x04},
	{0x32, 0xf8},
	{0x37, 0x03},
	{0x39, 0x15},
	{0x43, 0x07},
	{0x44, 0x40},
	{0x46, 0x0b},
	{0x4b, 0x20},
	{0x4e, 0x08},
	{0x55, 0x20},
	{0x66, 0x05},
	{0x67, 0x05},
	{0x77, 0x01},
	{0x78, 0x00},
	{0x7c, 0x93},
	{0x8c, 0x12},
	{0x8d, 0x92},
	{0x90, 0x01},
	{0x9d, 0x10},
	{0xce, 0x7c},
	{0xd2, 0x41},
	{0xd3, 0xdc},
	{0xe6, 0x50},
	/*gain*/
	{0xb6, 0xc0},
	{0xb0, 0x70},
	{0xb1, 0x01},
	{0xb2, 0x00},
	{0xb3, 0x00},
	{0xb4, 0x00},
	{0xb8, 0x01},
	{0xb9, 0x00},
	/*blk*/
	{0x26, 0x30},
	{0xfe, 0x01},
	{0x40, 0x23},
	{0x55, 0x07},
	{0x60, 0x40},
	{0xfe, 0x04},
	{0x14, 0x78},
	{0x15, 0x78},
	{0x16, 0x78},
	{0x17, 0x78},
	/*window*/
	{0xfe, 0x01},
	{0x92, 0x00},
	{0x94, 0x03},
	{0x95, 0x04},
	{0x96, 0x48},//{0x96, 0x40},
	{0x97, 0x07},
	{0x98, 0x88},
	/*ISP*/
	{0xfe, 0x01},
	{0x01, 0x05},
	{0x02, 0x89},
	{0x04, 0x01},
	{0x07, 0xa6},
	{0x08, 0xa9},
	{0x09, 0xa8},
	{0x0a, 0xa7},
	{0x0b, 0xff},
	{0x0c, 0xff},
	{0x0f, 0x00},
	{0x50, 0x1c},
	{0x89, 0x03},
	{0xfe, 0x04},
	{0x28, 0x86},
	{0x29, 0x86},
	{0x2a, 0x86},
	{0x2b, 0x68},
	{0x2c, 0x68},
	{0x2d, 0x68},
	{0x2e, 0x68},
	{0x2f, 0x68},
	{0x30, 0x4f},
	{0x31, 0x68},
	{0x32, 0x67},
	{0x33, 0x66},
	{0x34, 0x66},
	{0x35, 0x66},
	{0x36, 0x66},
	{0x37, 0x66},
	{0x38, 0x62},
	{0x39, 0x62},
	{0x3a, 0x62},
	{0x3b, 0x62},
	{0x3c, 0x62},
	{0x3d, 0x62},
	{0x3e, 0x62},
	{0x3f, 0x62},
	/****DVP & MIPI****/
	{0xfe, 0x01},
	{0x9a, 0x06},
	{0xfe, 0x00},
	{0x7b, 0x2a},
	{0x23, 0x2d},
	{0xfe, 0x03},
	{0x01, 0x27},
	{0x02, 0x56},
	{0x03, 0x8e},
	{0x12, 0x80},
	{0x13, 0x07},
	{0x15, 0x12},
	{0xfe, 0x00},
	{0x17, 0x83},
	{0x3e, 0x91},
	/* close rgb adapt*/
//	{0xfe, 0x01},
//	{0x99, 0x00},
//	{0xfe, 0x00},
};

/*
 * window_size=1920*1080 mipi@2lane
 * mclk=24mhz, mipi_clk=594Mbps
 * pixel_line_total=2200, line_frame_total=1125
 * row_time=29.629us, frame_rate=30fps
 */
static struct regval_list sensor_1080p30_regs[] = {
	/*  1928*1088@30fps */
	/****system****/
	{0xfe, 0x80},
	{0xfe, 0x80},
	{0xfe, 0x80},
	{0xfe, 0x00},
	{0xf2, 0x00},
	{0xf3, 0x00},
	{0xf4, 0x36},
	{0xf5, 0xc0},
	{0xf6, 0x44},
	{0xf7, 0x01},
	{0xf8, 0x63},
	{0xf9, 0x40},
	{0xfc, 0x8e},
	/****CISCTL & ANALOG****/
	{0xfe, 0x00},
	{0x87, 0x18},
	{0xee, 0x30},
	{0xd0, 0xb7},
	{0x03, 0x04},
	{0x04, 0x60},
	{0x05, 0x04},
	{0x06, 0x4c},
	{0x07, 0x00},
	{0x08, 0x11},
	{0x09, 0x00},
	{0x0a, 0x02},
	{0x0b, 0x00},
	{0x0c, 0x02},
	{0x0d, 0x04},
	{0x0e, 0x48},//{0x0e, 0x40},
	{0x12, 0xe2},
	{0x13, 0x16},
	{0x19, 0x0a},
	{0x21, 0x1c},
	{0x28, 0x0a},
	{0x29, 0x24},
	{0x2b, 0x04},
	{0x32, 0xf8},
	{0x37, 0x03},
	{0x39, 0x15},
	{0x43, 0x07},
	{0x44, 0x40},
	{0x46, 0x0b},
	{0x4b, 0x20},
	{0x4e, 0x08},
	{0x55, 0x20},
	{0x66, 0x05},
	{0x67, 0x05},
	{0x77, 0x01},
	{0x78, 0x00},
	{0x7c, 0x93},
	{0x8c, 0x12},
	{0x8d, 0x92},
	{0x90, 0x01},
	{0x9d, 0x10},
	{0xce, 0x7c},
	{0xd2, 0x41},
	{0xd3, 0xdc},
	{0xe6, 0x50},
	/*gain*/
	{0xb6, 0xc0},
	{0xb0, 0x70},
	{0xb1, 0x01},
	{0xb2, 0x00},
	{0xb3, 0x00},
	{0xb4, 0x00},
	{0xb8, 0x01},
	{0xb9, 0x00},
	/*blk*/
	{0x26, 0x30},
	{0xfe, 0x01},
	{0x40, 0x23},
	{0x55, 0x07},
	{0x60, 0x40},
	{0xfe, 0x04},
	{0x14, 0x78},
	{0x15, 0x78},
	{0x16, 0x78},
	{0x17, 0x78},
	/*window*/
	{0xfe, 0x01},
	{0x92, 0x00},
	{0x94, 0x03},
	{0x95, 0x04},
	{0x96, 0x48},//{0x96, 0x40},
	{0x97, 0x07},
	{0x98, 0x88},
	/*ISP*/
	{0xfe, 0x01},
	{0x01, 0x05},
	{0x02, 0x89},
	{0x04, 0x01},
	{0x07, 0xa6},
	{0x08, 0xa9},
	{0x09, 0xa8},
	{0x0a, 0xa7},
	{0x0b, 0xff},
	{0x0c, 0xff},
	{0x0f, 0x00},
	{0x50, 0x1c},
	{0x89, 0x03},
	{0xfe, 0x04},
	{0x28, 0x86},
	{0x29, 0x86},
	{0x2a, 0x86},
	{0x2b, 0x68},
	{0x2c, 0x68},
	{0x2d, 0x68},
	{0x2e, 0x68},
	{0x2f, 0x68},
	{0x30, 0x4f},
	{0x31, 0x68},
	{0x32, 0x67},
	{0x33, 0x66},
	{0x34, 0x66},
	{0x35, 0x66},
	{0x36, 0x66},
	{0x37, 0x66},
	{0x38, 0x62},
	{0x39, 0x62},
	{0x3a, 0x62},
	{0x3b, 0x62},
	{0x3c, 0x62},
	{0x3d, 0x62},
	{0x3e, 0x62},
	{0x3f, 0x62},
	/****DVP & MIPI****/
	{0xfe, 0x01},
	{0x9a, 0x06},
	{0xfe, 0x00},
	{0x7b, 0x2a},
	{0x23, 0x2d},
	{0xfe, 0x03},
	{0x01, 0x27},
	{0x02, 0x56},
	{0x03, 0x8e},
	{0x12, 0x80},
	{0x13, 0x07},
	{0x15, 0x12},
	{0xfe, 0x00},
	{0x17, 0x83},
	{0x3e, 0x91},
};

/*
 * Here we'll try to encapsulate the changes for just the output
 * video format.
 *
 */

static struct regval_list sensor_fmt_raw[] = {

};


/*
 * Code for dealing with controls.
 * fill with different sensor module
 * different sensor module has different settings here
 * if not support the follow function , retrun -EINVAL
 */
#if 0
static int sensor_g_exp(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);
	*value = info->exp;
	sensor_dbg("sensor_get_exposure = %d\n", info->exp);
	return 0;
}
static int sensor_g_gain(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);
	*value = info->gain;
	sensor_dbg("sensor_get_gain = %d\n", info->gain);
	return 0;
}
#endif

static int sensor_s_exp(int id, unsigned int exp_val)
{
	int tmp_exp_val = exp_val / 16;

	sensor_dbg("exp_val:%d\n", exp_val);
	sensor_write(id, 0x03, (tmp_exp_val >> 8) & 0xFF);
	sensor_write(id, 0x04, (tmp_exp_val & 0xFF));

	return 0;
}

unsigned char gc2053_regValTable[29][4] = {
	{0x00, 0x00, 0x01, 0x00},
	{0x00, 0x10, 0x01, 0x0c},
	{0x00, 0x20, 0x01, 0x1b},
	{0x00, 0x30, 0x01, 0x2c},
	{0x00, 0x40, 0x01, 0x3f},
	{0x00, 0x50, 0x02, 0x16},
	{0x00, 0x60, 0x02, 0x35},
	{0x00, 0x70, 0x03, 0x16},
	{0x00, 0x80, 0x04, 0x02},
	{0x00, 0x90, 0x04, 0x31},
	{0x00, 0xa0, 0x05, 0x32},
	{0x00, 0xb0, 0x06, 0x35},
	{0x00, 0xc0, 0x08, 0x04},
	{0x00, 0x5a, 0x09, 0x19},
	{0x00, 0x83, 0x0b, 0x0f},
	{0x00, 0x93, 0x0d, 0x12},
	{0x00, 0x84, 0x10, 0x00},
	{0x00, 0x94, 0x12, 0x3a},
	{0x01, 0x2c, 0x1a, 0x02},
	{0x01, 0x3c, 0x1b, 0x20},
	{0x00, 0x8c, 0x20, 0x0f},
	{0x00, 0x9c, 0x26, 0x07},
	{0x02, 0x64, 0x36, 0x21},
	{0x02, 0x74, 0x37, 0x3a},
	{0x00, 0xc6, 0x3d, 0x02},
	{0x00, 0xdc, 0x3f, 0x3f},
	{0x02, 0x85, 0x3f, 0x3f},
	{0x02, 0x95, 0x3f, 0x3f},
	{0x00, 0xce, 0x3f, 0x3f}
};

unsigned int gc2053_gainLevelTable[29] = {
	64,    74,    89,    102,   127,   147,
	177,   203,   260,   300,   361,   415,
	504,   581,   722,   832,   1027,  1182,
	1408,  1621,  1990,  2291,  2850,  3282,
	4048,  5180,  5500,  6744,  7073,
};

static int total = sizeof(gc2053_gainLevelTable) / sizeof(unsigned int);
static int Gain_Flag;
static int setSensorGain(int id, int gain)
{
	int i;
	data_type temperature_value;
	unsigned int tol_dig_gain = 0;

	sensor_write(id, 0xfe, 0x04);
	sensor_read(id, 0x0c, &temperature_value);
	sensor_write(id, 0xfe, 0x00);

	if (temperature_value >= 0x50) {
		total = 12;
		Gain_Flag = 1;
	}

	if (temperature_value < 0x18) {
		total = sizeof(gc2053_gainLevelTable) / sizeof(unsigned int);
		Gain_Flag = 0;
	}

	if (Gain_Flag == 1) {
		if (gain > 40 * 64) {
			sensor_write(id, 0xfe, 0x04);
			sensor_write(id, 0x24, 0x02);
			sensor_write(id, 0x25, 0x01);
			sensor_write(id, 0x26, 0x00);
			sensor_write(id, 0x27, 0x02);
			sensor_write(id, 0xfe, 0x00);
		} else if (gain > 25 * 64) {
			sensor_write(id, 0xfe, 0x04);
			sensor_write(id, 0x24, 0x01);
			sensor_write(id, 0x25, 0x00);
			sensor_write(id, 0x26, 0x00);
			sensor_write(id, 0x27, 0x00);
			sensor_write(id, 0xfe, 0x00);
		} else {
			sensor_write(id, 0xfe, 0x04);
			sensor_write(id, 0x24, 0x00);
			sensor_write(id, 0x25, 0x00);
			sensor_write(id, 0x26, 0x00);
			sensor_write(id, 0x27, 0x00);
			sensor_write(id, 0xfe, 0x00);
		}
	} else {
		if (gain < 30 * 64) {
			sensor_write(id, 0xfe, 0x4);
			sensor_write(id, 0x24, 0x0);
			sensor_write(id, 0x25, 0x0);
			sensor_write(id, 0x27, 0x0);
			sensor_write(id, 0xfe, 0x0);
		} else {
			sensor_write(id, 0xfe, 0x4);
			sensor_write(id, 0x24, 0x1);
			sensor_write(id, 0x25, 0x0);
			sensor_write(id, 0x27, 0x0);
			sensor_write(id, 0xfe, 0x0);
		}
	}

	for (i = 0; (total > 1) && (i < (total - 1)); i++) {
		if ((gc2053_gainLevelTable[i] <= gain) && (gain < gc2053_gainLevelTable[i+1]))
			break;
	}

	tol_dig_gain = gain * 64 / gc2053_gainLevelTable[i];
	sensor_write(id, 0xfe, 0x00);
	sensor_write(id, 0xb4, gc2053_regValTable[i][0]);
	sensor_write(id, 0xb3, gc2053_regValTable[i][1]);
	sensor_write(id, 0xb8, gc2053_regValTable[i][2]);
	sensor_write(id, 0xb9, gc2053_regValTable[i][3]);
	sensor_write(id, 0xb1, (tol_dig_gain>>6));
	sensor_write(id, 0xb2, ((tol_dig_gain&0x3f)<<2));

	return 0;
}

static int sensor_s_gain(int id, int gain_val)
{
	sensor_dbg("gain_val:%d\n", gain_val);
	setSensorGain(id, gain_val * 4);

	return 0;
}

static int gc2053_sensor_vts;
static int sensor_s_exp_gain(int id, struct sensor_exp_gain *exp_gain)
{
	int exp_val, gain_val;
	int shutter = 0, frame_length = 0;

	exp_val = exp_gain->exp_val;
	gain_val = exp_gain->gain_val;

	if (gain_val < (1 * 16)) {
		gain_val = 16;
	}

	if (exp_val > 0xfffff)
		exp_val = 0xfffff;

	shutter = exp_val >> 4;
	if (shutter > gc2053_sensor_vts - 16)
		frame_length = shutter + 16;
	else
		frame_length = gc2053_sensor_vts;
	sensor_dbg("frame_length = %d\n", frame_length);
	sensor_write(id, 0x42, frame_length & 0xff);
	sensor_write(id, 0x41, frame_length >> 8);

	sensor_s_exp(id, exp_val);
	sensor_s_gain(id, gain_val);

	sensor_print("gain_val:%d, exp_val:%d\n", gain_val, exp_val);

	return 0;
}

#if 0
static int sensor_flip_status;
static int sensor_s_vflip(int id, int enable)
{
	data_type get_value;
	data_type set_value;

	if (!(enable == 0 || enable == 1))
		return -1;

	sensor_read(id, 0x17, &get_value);
	sensor_dbg("ready to vflip, regs_data = 0x%x\n", get_value);

	if (enable) {
		set_value = get_value | 0x02;
		sensor_flip_status |= 0x02;
	} else {
		set_value = get_value & 0xFD;
		sensor_flip_status &= 0xFD;
	}
	sensor_write(id, 0x17, set_value);
	usleep_range(80000, 100000);
	sensor_read(id, 0x17, &get_value);
	sensor_dbg("after vflip, regs_data = 0x%x, sensor_flip_status = %d\n",
				get_value, sensor_flip_status);

	return 0;
}

static int sensor_s_hflip(int id, int enable)
{
	data_type get_value;
	data_type set_value;

	if (!(enable == 0 || enable == 1))
		return -1;

	sensor_read(id, 0x17, &get_value);
	sensor_dbg("ready to hflip, regs_data = 0x%x\n", get_value);

	if (enable) {
		set_value = get_value | 0x01;
		sensor_flip_status |= 0x01;
	} else {
		set_value = get_value & 0xFE;
		sensor_flip_status &= 0xFE;
	}
	sensor_write(id, 0x17, set_value);
	usleep_range(80000, 100000);
	sensor_read(id, 0x17, &get_value);
	sensor_dbg("after hflip, regs_data = 0x%x, sensor_flip_status = %d\n",
				get_value, sensor_flip_status);

	return 0;
}

static int sensor_get_fmt_mbus_core(struct v4l2_subdev *sd, int *code)
{
//	struct sensor_info *info = to_state(sd);
//	data_type get_value = 0, check_value = 0;

//	sensor_read(sd, 0x17, &get_value);
//	check_value = get_value & 0x03;
//	check_value = sensor_flip_status & 0x3;
//	sensor_dbg("0x17 = 0x%x, check_value = 0x%x\n", get_value, check_value);

//	switch (check_value) {
//	case 0x00:
//		sensor_dbg("RGGB\n");
//		*code = MEDIA_BUS_FMT_SRGGB10_1X10;
//		break;
//	case 0x01:
//		sensor_dbg("GRBG\n");
//		*code = MEDIA_BUS_FMT_SGRBG10_1X10;
//		break;
//	case 0x02:
//		sensor_dbg("GBRG\n");
//		*code = MEDIA_BUS_FMT_SGBRG10_1X10;
//		break;
//	case 0x03:
//		sensor_dbg("BGGR\n");
//		*code = MEDIA_BUS_FMT_SBGGR10_1X10;
//		break;
//	default:
//		 *code = info->fmt->mbus_code;
//	}
	*code = MEDIA_BUS_FMT_SRGGB10_1X10; // gc2053 support change the rgb format by itself

	return 0;
}
#endif

/*
* Stuff that knows about the sensor.
*/
static int sensor_power(int id, int on)
{
	if (on && (sensor_power_count[id])++ > 0)
		return 0;
	else if (!on && (sensor_power_count[id] == 0 || --(sensor_power_count[id]) > 0))
		return 0;

	switch (on) {
	case PWR_ON:
		sensor_dbg("PWR_ON!\n");
		vin_set_mclk_freq(id, MCLK);
		vin_set_mclk(id, 1);
		hal_usleep(1000);
		vin_gpio_set_status(id, PWDN, 1);
		vin_gpio_set_status(id, RESET, 1);
		//vin_gpio_set_status(sd, POWER_EN, 1);
		vin_gpio_write(id, PWDN, CSI_GPIO_LOW);
		vin_gpio_write(id, RESET, CSI_GPIO_LOW);
		hal_usleep(1000);
		vin_gpio_write(id, PWDN, CSI_GPIO_HIGH);
		vin_gpio_write(id, RESET, CSI_GPIO_HIGH);
		hal_usleep(1000);
		break;
	case PWR_OFF:
		sensor_dbg("PWR_OFF!do nothing\n");
		vin_set_mclk(id, 0);
		hal_usleep(1000);
		vin_gpio_set_status(id, PWDN, 1);
		vin_gpio_set_status(id, RESET, 1);
		vin_gpio_write(id, PWDN, CSI_GPIO_LOW);
		vin_gpio_write(id, RESET, CSI_GPIO_LOW);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

#if 0
static int sensor_reset(int id, u32 val)
{

	sensor_dbg("%s: val=%d\n", __func__);
	switch (val) {
	case 0:
		vin_gpio_write(id, RESET, CSI_GPIO_HIGH);
		hal_usleep(1000);
		break;
	case 1:
		vin_gpio_write(id, RESET, CSI_GPIO_LOW);
		hal_usleep(1000);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}
#endif

static int sensor_detect(int id)
{
	data_type rdval;
	int eRet;
	int times_out = 3;
	do {
		eRet = sensor_read(id, ID_REG_HIGH, &rdval);
		sensor_dbg("eRet:%d, ID_VAL_HIGH:0x%x, times_out:%d\n", eRet, rdval, times_out);
		hal_usleep(200);
		times_out--;
	} while (eRet < 0  &&  times_out > 0);

	sensor_read(id, ID_REG_HIGH, &rdval);
	sensor_dbg("ID_VAL_HIGH = %2x, Done!\n", rdval);
	if (rdval != ID_VAL_HIGH)
		return -ENODEV;

	sensor_read(id, ID_REG_LOW, &rdval);
	sensor_dbg("ID_VAL_LOW = %2x, Done!\n", rdval);
	if (rdval != ID_VAL_LOW)
		return -ENODEV;

	sensor_dbg("Done!\n");
	return 0;
}

static int sensor_init(int id)
{
	int ret;

	sensor_dbg("sensor_init\n");

	/*Make sure it is a target sensor */
	ret = sensor_detect(id);
	if (ret) {
		sensor_err("chip found is not an target chip.\n");
		return ret;
	}

	return 0;
}

/*
 * Store information about the video data format.
 */
static struct sensor_format_struct sensor_format = {
		.desc      = "Raw RGB Bayer",
		.mbus_code = MEDIA_BUS_FMT_SRGGB10_1X10, /*.mbus_code = MEDIA_BUS_FMT_SBGGR10_1X10, */
#if 1
		.width      = 1920,
		.height     = 1088,//1080,
		//.hoffset    = 4,//0,
		//.voffset    = 4,//0,
		.hts        = 3300,
		.vts        = 1125,
		.pclk       = 74250000,
		.mipi_bps   = 297 * 1000 * 1000,
		.fps_fixed  = 20,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = 1125 << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 110 << 4,
#else
		.width	    = 1920,
		.height     = 1088,//1080,
		//.hoffset    = 4,//0,
		//.voffset    = 4,//0,
		.hts	    = 2640,
		.vts	    = 1125,
		.pclk	    = 74250000,
		.mipi_bps   = 297 * 1000 * 1000,
		.fps_fixed  = 25,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = (1125 - 16) << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 110 << 4,

		.width      = 1920,
		.height     = 1088,//1080,
		//.hoffset    = 4,//0,
		//.voffset    = 4,//0,
		.hts        = 2200,
		.vts        = 1125,
		.pclk       = 74250000,
		.mipi_bps   = 297 * 1000 * 1000,
		.fps_fixed  = 30,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = 1125 << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 110 << 4,
#endif
};

/*
 * Then there is the issue of window sizes.  Try to capture the info here.
 */
#if 0
static struct sensor_win_size sensor_win_sizes[] = {
	{
		.width      = 1920,
		.height     = 1088,//1080,
		.hoffset    = 4,//0,
		.voffset    = 4,//0,
		.hts        = 3300,
		.vts        = 1125,
		.pclk       = 74250000,
		.mipi_bps   = 297 * 1000 * 1000,
		.fps_fixed  = 20,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = 1125 << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 110 << 4,
		.regs       = sensor_1080p20_regs,
		.regs_size  = ARRAY_SIZE(sensor_1080p20_regs),
		.set_size   = NULL,
	},
	{
		.width	    = 1920,
		.height     = 1088,//1080,
		.hoffset    = 4,//0,
		.voffset    = 4,//0,
		.hts	    = 2640,
		.vts	    = 1125,
		.pclk	    = 74250000,
		.mipi_bps   = 297 * 1000 * 1000,
		.fps_fixed  = 25,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = (1125 - 16) << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 110 << 4,
		.regs	    = sensor_1080p25_regs,
		.regs_size  = ARRAY_SIZE(sensor_1080p25_regs),
		.set_size   = NULL,
	},
	{
		.width      = 1920,
		.height     = 1088,//1080,
		.hoffset    = 4,//0,
		.voffset    = 4,//0,
		.hts        = 2200,
		.vts        = 1125,
		.pclk       = 74250000,
		.mipi_bps   = 297 * 1000 * 1000,
		.fps_fixed  = 30,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = 1125 << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 110 << 4,
		.regs       = sensor_1080p30_regs,
		.regs_size  = ARRAY_SIZE(sensor_1080p30_regs),
		.set_size   = NULL,
	},
};
#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))
#endif

static int sensor_g_mbus_config(struct v4l2_mbus_config *cfg, struct mbus_framefmt_res *res)
{
	//struct sensor_info *info = to_state(sd);

	cfg->type  = V4L2_MBUS_CSI2;
	cfg->flags = 0 | V4L2_MBUS_CSI2_2_LANE | V4L2_MBUS_CSI2_CHANNEL_0;
	res->res_time_hs = 0x28;

	return 0;
}

static int sensor_reg_init(int id)
{
	int ret;
	struct sensor_exp_gain exp_gain;

	ret = sensor_write_array(id, sensor_default_regs,
				 ARRAY_SIZE(sensor_default_regs));
	if (ret < 0) {
		sensor_err("write sensor_default_regs error\n");
		return ret;
	}

	sensor_write_array(id, sensor_1080p20_regs, ARRAY_SIZE(sensor_1080p20_regs));
#ifndef CONFIG_ISP_READ_THRESHOLD
	exp_gain.exp_val = 15408;
	exp_gain.gain_val = 16;
#else
	exp_gain.exp_val = clamp(*((unsigned int *)NORFLASH_SAVE + 1), 16, 3000 << 4);
	exp_gain.gain_val = clamp(*((unsigned int *)NORFLASH_SAVE)  & 0xffff, 16, 110 << 4);
#endif
	sensor_s_exp_gain(id, &exp_gain);

	//sensor_flip_status = 0x0;
	//gc2053_sensor_vts = wsize->vts;
	gc2053_sensor_vts = 1125;
	//sensor_dbg("gc2053_sensor_vts = %d\n", gc2053_sensor_vts);

	return 0;
}

static int sensor_s_stream(int id, int enable)
{
	if (enable && sensor_stream_count[id]++ > 0)
		return 0;
	else if (!enable && (sensor_stream_count[id] == 0 || --sensor_stream_count[id] > 0))
		return 0;

	if (!enable)
		return 0;

	return sensor_reg_init(id);
}

static int sensor_test_i2c(int id)
{
	int ret;
	sensor_power(id, PWR_ON);
	ret = sensor_init(id);
	sensor_power(id, PWR_OFF);

	return ret;
}

struct sensor_fuc_core gc2053_core  = {
	.g_mbus_config = sensor_g_mbus_config,
	.sensor_test_i2c = sensor_test_i2c,
	.sensor_power = sensor_power,
	.s_stream = sensor_s_stream,
	.s_exp_gain = sensor_s_exp_gain,
	.sensor_formats = &sensor_format,
};