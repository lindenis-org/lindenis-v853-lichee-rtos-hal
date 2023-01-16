obj-y += modules/sensor/gc2053_mipi.o
obj-y += modules/sensor/gc4663_mipi.o
obj-y += platform/vin_config_sun8iw21p1.o
obj-y += utility/vin_supply.o
obj-y += vin_cci/cci_helper.o
obj-y += vin_mipi/combo_csi/combo_csi_reg.o
obj-y += vin_mipi/sunxi_mipi.o
obj-y += vin_csi/parser_reg.o
obj-y += vin_csi/sunxi_csi.o
obj-y += vin_tdm/tdm200/tdm200_reg.o
obj-y += vin_tdm/vin_tdm.o
obj-y += vin_isp/isp600/isp600_reg_cfg.o
obj-y += vin_isp/sunxi_isp.o
obj-y += vin_stat/vin_h3a.o
obj-y += vin_vipp/vipp200/vipp200_reg.o
obj-y += vin_vipp/sunxi_scaler.o
obj-y += vin_video/dma140/dma140_reg.o
obj-y += vin_video/vin_core.o
obj-y += vin_video/vin_video.o
obj-y += top_reg.o
obj-y += vin.o

obj-y += vin_isp/vin_link_server.o
obj-y += vin_isp/isp_server/isp_server.o
obj-y += vin_isp/isp_server/isp_math/matrix.o
obj-y += vin_isp/isp_server/isp_math/isp_math_util.o
obj-y += vin_isp/isp_server/isp_cfg/isp_ini_parse.o
obj-y += vin_isp/isp_server/isp_tuning/isp_tuning.o
obj-y += vin_isp/isp_server/isp_manage/isp_helper.o
obj-y += vin_isp/isp_server/isp_manage/isp_manage.o

#obj-y += vin_isp/isp_server/isp_algo/isp600/isp600_reg_cfg.o
#obj-y += vin_isp/isp_server/isp_algo/isp_ae_data.o
#obj-y += vin_isp/isp_server/isp_algo/isp_awb_data.o
#obj-y += vin_isp/isp_server/isp_algo/isp_base.o
#obj-y += vin_isp/isp_server/isp_algo/isp_module_cfg.o
#obj-y += vin_isp/isp_server/isp_algo/isp_3a_ae.o
#obj-y += vin_isp/isp_server/isp_algo/isp_3a_af.o
#obj-y += vin_isp/isp_server/isp_algo/isp_3a_afs.o
#obj-y += vin_isp/isp_server/isp_algo/isp_3a_awb.o
#obj-y += vin_isp/isp_server/isp_algo/isp_iso_config.o
#obj-y += vin_isp/isp_server/isp_algo/isp_pltm.o

ifeq ($(CONFIG_FF_TRACE_DRIVER_VIN),y)
#Qsubdir-ccflags-y += -finstrument-functions
CFLAGS_vin.o += -finstrument-functions
CFLAGS_sunxi_isp.o += -finstrument-functions
CFLAGS_vin_tdm.o += -finstrument-functions
CFLAGS_sunxi_csi.o += -finstrument-functions
CFLAGS_vin_video.o += -finstrument-functions
endif
