1. Update the file: drv/interdrv/sysconfig/sys_config.c,
   It need to use I2S interface, so open the pinmux setting as below.
        i2s_pin_mux;

2. Update the file: mpp/ko/
   It need to insmod tlv_320aic31's driver.
    insmod extdrv/hi_tlv320aic31.ko

3. Modify the makefile parameter: mpp/sample/Makefile.param. Set ACODEC_TYPE to  ACODEC_TYPE_TLV320AIC31.
   It means use the external codec tlv_320aic31 sample code.
    ################ select audio codec type for your sample ################
    #ACODEC_TYPE ?= ACODEC_TYPE_INNER
    #external acodec
    ACODEC_TYPE ?= ACODEC_TYPE_TLV320AIC31

4. Rebuild the sample and get the sample_audio.
