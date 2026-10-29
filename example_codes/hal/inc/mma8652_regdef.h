/*
 * Copyright 2018-2019,2024 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file MMA8652_regdef.h
 * @brief This header file file contains the register definitions for MMA8652, the 3-axis sensor
 *        with integrated linear accelerometer.
 */

#ifndef MMA8652_REGDEF_H_
#define MMA8652_REGDEF_H_

#include <stdint.h>
/**
 * @brief MMA8652 register addresses map explained in section 6 of the MMA8652 data sheet.
*/
enum {
    MMA8652_STATUS           = 0x00, /*!< Alias for ::MMA8652_DR_STATUS or ::MMA8652_F_STATUS. */
    MMA8652_OUT_X_MSB        = 0x01, /*!< 14-bit X-axis measurement data bits 13:6. */
    MMA8652_OUT_X_LSB        = 0x02, /*!< 14-bit X-axis measurement data bits 5:0. */
    MMA8652_OUT_Y_MSB        = 0x03, /*!< 14-bit Y-axis measurement data bits 13:6. */
    MMA8652_OUT_Y_LSB        = 0x04, /*!< 14-bit Y-axis measurement data bits 5:0. */
    MMA8652_OUT_Z_MSB        = 0x05, /*!< 14-bit Z-axis measurement data bits 13:6. */
    MMA8652_OUT_Z_LSB        = 0x06, /*!< 14-bit Z-axis measurement data bits 5:0. */
    MMA8652_F_SETUP          = 0x09, /*!< FIFO setup. */
    MMA8652_TRIG_CFG         = 0x0A, /*!< FIFO event trigger configuration register. */
    MMA8652_SYSMOD           = 0x0B, /*!< Current system mode. */
    MMA8652_INT_SOURCE       = 0x0C, /*!< Interrupt status. */
    MMA8652_WHO_AM_I         = 0x0D, /*!< Device ID. */
    MMA8652_XYZ_DATA_CFG     = 0x0E, /*!< Acceleration dynamic range and filter enable settings. */
    MMA8652_HP_FILTER_CUTOFF = 0x0F, /*!< Pulse detection highpass and lowpass filter enabling bits. */
    MMA8652_PL_STATUS        = 0x10, /*!< Landscape/portrait orientation status. */
    MMA8652_PL_CFG           = 0x11, /*!< Landscape/portrait configuration. */
    MMA8652_PL_COUNT         = 0x12, /*!< Landscape/portrait debounce counter. */
    MMA8652_PL_BF_ZCOMP      = 0x13, /*!< Back/front trip angle threshold. */
    MMA8652_PL_THS_REG       = 0x14, /*!< Portrait to landscape trip threshold angle and hysteresis settings. */
    MMA8652_A_FFMT_CFG       = 0x15, /*!< Freefall/motion function configuration. */
    MMA8652_A_FFMT_SRC       = 0x16, /*!< Freefall/motion event source register. */
    MMA8652_A_FFMT_THS       = 0x17, /*!< Freefall/motion threshold register. */
    MMA8652_A_FFMT_COUNT     = 0x18, /*!< Freefall/motion debounce counter. */
    MMA8652_TRANSIENT_CFG    = 0x1D, /*!< Transient function configuration. */
    MMA8652_TRANSIENT_SRC    = 0x1E, /*!< Transient event status register. */
    MMA8652_TRANSIENT_THS    = 0x1F, /*!< Transient event threshold. */
    MMA8652_TRANSIENT_COUNT  = 0x20, /*!< Transient debounce counter. */
    MMA8652_PULSE_CFG        = 0x21, /*!< Pulse function configuration. */
    MMA8652_PULSE_SRC        = 0x22, /*!< Pulse function source register. */
    MMA8652_PULSE_THSX       = 0x23, /*!< X-axis pulse threshold. */
    MMA8652_PULSE_THSY       = 0x24, /*!< Y-axis pulse threshold. */
    MMA8652_PULSE_THSZ       = 0x25, /*!< Z-axis pulse threshold. */
    MMA8652_PULSE_TMLT       = 0x26, /*!< Time limit for pulse detection. */
    MMA8652_PULSE_LTCY       = 0x27, /*!< Latency time for second pulse detection. */
    MMA8652_PULSE_WIND       = 0x28, /*!< Window time for second pulse detection. */
    MMA8652_ASLP_COUNT       = 0x29, /*!< The counter setting for auto-sleep period. */
    MMA8652_CTRL_REG1        = 0x2A, /*!< System ODR, accelerometer OSR (Output sample rate), operating mode. */
    MMA8652_CTRL_REG2        = 0x2B, /*!< Self-test, reset, accelerometer OSR, and sleep mode settings. */
    MMA8652_CTRL_REG3        = 0x2C, /*!< Sleep mode interrupt wake enable, interrupt polarity, push-pull/open drain configuration. */
    MMA8652_CTRL_REG4        = 0x2D, /*!< Interrupt enable register. */
    MMA8652_CTRL_REG5        = 0x2E, /*!< Interrupt pin (INT1/INT2) map. */
    MMA8652_OFF_X            = 0x2F, /*!< X-axis accelerometer offset adjust. */
    MMA8652_OFF_Y            = 0x30, /*!< Y-axis accelerometer offset adjust. */
    MMA8652_OFF_Z            = 0x31, /*!< Z-axis accelerometer offset adjust. */
};

#define MMA8652_DEVICE_ADDR_SA		(0x1D)
#define MMA8652_WHO_AM_I_PROD_VALUE (0x4A)

#define MMA8652_SENSITIVITY_MG_PER_LSB_2G (1000.0/1024)
#define MMA8652_SENSITIVITY_MG_PER_LSB_4G (1000.0/512)
#define MMA8652_SENSITIVITY_MG_PER_LSB_8G (1000.0/256)

/**
 * The following are the macro definitions to address each bit and its value in the hardware registers.
 */

/*-----------------------------------------------------------------------
** Register: DR_STATUS
** Enum: MMA8652_DR_STATUS
** --
** Offset : 0x00 - Alias for ::MMA8652_DR_STATUS or ::MMA8652_F_STATUS.
** ---------------------------------------------------------------------*/
typedef union {
    struct {
        uint8_t                   xdr : 1;
        uint8_t                   ydr : 1;
        uint8_t                   zdr : 1;
        uint8_t                 zyxdr : 1;
        uint8_t                   xow : 1;
        uint8_t                   yow : 1;
        uint8_t                   zow : 1;
        uint8_t                 zyxow : 1;
    } b;
    uint8_t w;
} MMA8652_DR_STATUS_t;


/*
** DR_STATUS - Bit field mask definitions
*/
#define MMA8652_DR_STATUS_XDR_MASK     ((uint8_t) 0x01)
#define MMA8652_DR_STATUS_XDR_SHIFT    ((uint8_t)    0)

#define MMA8652_DR_STATUS_YDR_MASK     ((uint8_t) 0x02)
#define MMA8652_DR_STATUS_YDR_SHIFT    ((uint8_t)    1)

#define MMA8652_DR_STATUS_ZDR_MASK     ((uint8_t) 0x04)
#define MMA8652_DR_STATUS_ZDR_SHIFT    ((uint8_t)    2)

#define MMA8652_DR_STATUS_ZYXDR_MASK   ((uint8_t) 0x08)
#define MMA8652_DR_STATUS_ZYXDR_SHIFT  ((uint8_t)    3)

#define MMA8652_DR_STATUS_XOW_MASK     ((uint8_t) 0x10)
#define MMA8652_DR_STATUS_XOW_SHIFT    ((uint8_t)    4)

#define MMA8652_DR_STATUS_YOW_MASK     ((uint8_t) 0x20)
#define MMA8652_DR_STATUS_YOW_SHIFT    ((uint8_t)    5)

#define MMA8652_DR_STATUS_ZOW_MASK     ((uint8_t) 0x40)
#define MMA8652_DR_STATUS_ZOW_SHIFT    ((uint8_t)    6)

#define MMA8652_DR_STATUS_ZYXOW_MASK   ((uint8_t) 0x80)
#define MMA8652_DR_STATUS_ZYXOW_SHIFT  ((uint8_t)    7)


/*
** DR_STATUS - Bit field value definitions
*/
#define MMA8652_DR_STATUS_XDR_DRDY              ((uint8_t) 0x01)  /*  xdr is set to 1 whenever a new X-axis data         */
                                                                   /*  acquisition is completed. xdr is cleared anytime   */
                                                                   /*  the OUT_X_MSB register is read.                    */
#define MMA8652_DR_STATUS_YDR_DRDY              ((uint8_t) 0x02)  /*  ydr is set to 1 whenever a new Y-axis data         */
                                                                   /*  acquisition is completed. xdr is cleared anytime   */
                                                                   /*  the OUT_Y_MSB register is read.                    */
#define MMA8652_DR_STATUS_ZDR_DRDY              ((uint8_t) 0x04)  /*  zdr is set to 1 whenever a new Z-axis data         */
                                                                   /*  acquisition is completed. xdr is cleared anytime   */
                                                                   /*  the OUT_Z_MSB register is read.                    */
#define MMA8652_DR_STATUS_ZYXDR_DRDY            ((uint8_t) 0x08)  /*  zyxdr signals that a new acquisition for any of    */
                                                                   /*  the enabled channels is available. zyxdr is        */
                                                                   /*  cleared when the high-bytes of the acceleration    */
                                                                   /*  data (OUT_X_MSB, OUT_Y_MSB, OUT_Z_MSB) are read.   */
#define MMA8652_DR_STATUS_XOW_OWR               ((uint8_t) 0x10)
#define MMA8652_DR_STATUS_YOW_OWR               ((uint8_t) 0x20)
#define MMA8652_DR_STATUS_ZOW_OWR               ((uint8_t) 0x40)
#define MMA8652_DR_STATUS_ZYXOW_OWR             ((uint8_t) 0x80)
/*------------------------------*/



/*--------------------------------
** Register: F_STATUS
** Enum: MMA8652_F_STATUS
** --
** Offset : 0x00 - Fifo Status register
** ------------------------------*/
typedef union {
    struct {
        uint8_t                 f_cnt : 6; /*  These bits indicate the number of acceleration samples currently stored in */
                                           /*  the FIFO buffer. Count 0b00_0000 indicates that the FIFO is empty          */

        uint8_t           f_wmrk_flag : 1;
        uint8_t                 f_ovf : 1;
    } b;
    uint8_t w;
} MMA8652_F_STATUS_t;


/*
** F_STATUS - Bit field mask definitions
*/
#define MMA8652_F_STATUS_F_CNT_MASK         ((uint8_t) 0x3F)
#define MMA8652_F_STATUS_F_CNT_SHIFT        ((uint8_t)    0)

#define MMA8652_F_STATUS_F_WMRK_FLAG_MASK   ((uint8_t) 0x40)
#define MMA8652_F_STATUS_F_WMRK_FLAG_SHIFT  ((uint8_t)    6)

#define MMA8652_F_STATUS_F_OVF_MASK         ((uint8_t) 0x80)
#define MMA8652_F_STATUS_F_OVF_SHIFT        ((uint8_t)    7)


/*
** F_STATUS - Bit field value definitions
*/
#define MMA8652_F_STATUS_F_WMRK_FLAG_NOEVT     ((uint8_t) 0x00)  /*  No FIFO watermark event detected                    */
#define MMA8652_F_STATUS_F_WMRK_FLAG_EVTDET    ((uint8_t) 0x40)
#define MMA8652_F_STATUS_F_OVF_NOOVFL          ((uint8_t) 0x00)  /*  No FIFO overflow events detected                    */
#define MMA8652_F_STATUS_F_OVF_OVFLDET         ((uint8_t) 0x80)  /*  FIFO overflow event detected                        */
/*------------------------------*/



/*--------------------------------
** Register: OUT_X_MSB
** Enum: MMA8652_OUT_X_MSB
** --
** Offset : 0x01 - MSB of Accelerometer X value
** ------------------------------*/
typedef union {
    struct {
        uint8_t                    xd; /*  - 12-bit X-axis measurement data bits 11:4.                                    */

    } b;
    uint8_t w;
} MMA8652_OUT_X_MSB_t;


/*
** OUT_X_MSB - Bit field mask definitions
*/
#define MMA8652_OUT_X_MSB_XD_MASK   ((uint8_t) 0xFF)
#define MMA8652_OUT_X_MSB_XD_SHIFT  ((uint8_t)    0)


/*------------------------------*/



/*--------------------------------
** Register: OUT_X_LSB
** Enum: MMA8652_OUT_X_LSB
** --
** Offset : 0x02 - LSB of Accelerometer X value
** ------------------------------*/
typedef union {
    struct {
        uint8_t _reserved_            : 4;
        uint8_t                    xd : 4; /*  - 12-bit X-axis measurement data bits 3:0.                                 */

    } b;
    uint8_t w;
} MMA8652_OUT_X_LSB_t;


/*
** OUT_X_LSB - Bit field mask definitions
*/
#define MMA8652_OUT_X_LSB_XD_MASK   ((uint8_t) 0xF0)
#define MMA8652_OUT_X_LSB_XD_SHIFT  ((uint8_t)    4)


/*------------------------------*/



/*--------------------------------
** Register: OUT_Y_MSB
** Enum: MMA8652_OUT_Y_MSB
** --
** Offset : 0x03 - 12-bit Y-axis measurement data bits 11:4.
** ------------------------------*/
typedef union {
    struct {
        uint8_t                    yd; /*  - 12-bit Y-axis measurement data bits 11:4.                                    */

    } b;
    uint8_t w;
} MMA8652_OUT_Y_MSB_t;


/*
** OUT_Y_MSB - Bit field mask definitions
*/
#define MMA8652_OUT_Y_MSB_YD_MASK   ((uint8_t) 0xFF)
#define MMA8652_OUT_Y_MSB_YD_SHIFT  ((uint8_t)    0)


/*------------------------------*/



/*--------------------------------
** Register: OUT_Y_LSB
** Enum: MMA8652_OUT_Y_LSB
** --
** Offset : 0x04 - 12-bit Y-axis measurement data bits 3:0.
** ------------------------------*/
typedef union {
    struct {
        uint8_t _reserved_            : 4;
        uint8_t                    yd : 4; /*  - 12-bit Y-axis measurement data bits 3:0.                                 */

    } b;
    uint8_t w;
} MMA8652_OUT_Y_LSB_t;


/*
** OUT_Y_LSB - Bit field mask definitions
*/
#define MMA8652_OUT_Y_LSB_YD_MASK   ((uint8_t) 0xF0)
#define MMA8652_OUT_Y_LSB_YD_SHIFT  ((uint8_t)    4)


/*------------------------------*/



/*--------------------------------
** Register: OUT_Z_MSB
** Enum: MMA8652_OUT_Z_MSB
** --
** Offset : 0x05 - 12-bit Z-axis measurement data bits 11:4.
** ------------------------------*/
typedef union {
    struct {
        uint8_t                    zd; /*  - 12-bit Z-axis measurement data bits 11:4.                                    */

    } b;
    uint8_t w;
} MMA8652_OUT_Z_MSB_t;


/*
** OUT_Z_MSB - Bit field mask definitions
*/
#define MMA8652_OUT_Z_MSB_ZD_MASK   ((uint8_t) 0xFF)
#define MMA8652_OUT_Z_MSB_ZD_SHIFT  ((uint8_t)    0)


/*------------------------------*/



/*--------------------------------
** Register: OUT_Z_LSB
** Enum: MMA8652_OUT_Z_LSB
** --
** Offset : 0x06 - 12-bit Z-axis measurement data bits 3:0.
** ------------------------------*/
typedef union {
    struct {
        uint8_t _reserved_            : 4;
        uint8_t                    zd : 4; /*  - 12-bit Z-axis measurement data bits 3:0.                                 */

    } b;
    uint8_t w;
} MMA8652_OUT_Z_LSB_t;


/*
** OUT_Z_LSB - Bit field mask definitions
*/
#define MMA8652_OUT_Z_LSB_ZD_MASK   ((uint8_t) 0xF0)
#define MMA8652_OUT_Z_LSB_ZD_SHIFT  ((uint8_t)    4)


/*------------------------------*/



/*--------------------------------
** Register: F_SETUP
** Enum: MMA8652_F_SETUP
** --
** Offset : 0x09 - FIFO setup.
** ------------------------------*/
typedef union {
    struct {
        uint8_t                f_wmrk : 6; /*  FIFO sample count watermark                                                */

        uint8_t                f_mode : 2; /*  - FIFO Buffer operating mode                                               */

    } b;
    uint8_t w;
} MMA8652_F_SETUP_t;


/*
** F_SETUP - Bit field mask definitions
*/
#define MMA8652_F_SETUP_F_WMRK_MASK   ((uint8_t) 0x3F)
#define MMA8652_F_SETUP_F_WMRK_SHIFT  ((uint8_t)    0)

#define MMA8652_F_SETUP_F_MODE_MASK   ((uint8_t) 0xC0)
#define MMA8652_F_SETUP_F_MODE_SHIFT  ((uint8_t)    6)


/*
** F_SETUP - Bit field value definitions
*/
#define MMA8652_F_SETUP_F_MODE_FIFO_DISABLE   ((uint8_t) 0x00)  /*  FIFO is disabled                                     */
#define MMA8652_F_SETUP_F_MODE_FIFO_CIRC      ((uint8_t) 0x40)  /*  FIFO contains the most recent samples when           */
                                                                 /*  overflowed (circular buffer). Oldest sample is       */
                                                                 /*  discarded to be replaced by new sample               */
#define MMA8652_F_SETUP_F_MODE_FIFO_STOP_OVF  ((uint8_t) 0x80)  /*  FIFO stops accepting new samples when overflowed     */
#define MMA8652_F_SETUP_F_MODE_FIFO_TRIGGER   ((uint8_t) 0xc0)  /*  FIFO trigger mode                                    */
/*------------------------------*/



/*--------------------------------
** Register: TRIG_CFG
** Enum: MMA8652_TRIG_CFG
** --
** Offset : 0x0A - FIFO event trigger configuration register.
** ------------------------------*/
typedef union {
    struct {
        uint8_t 		  _reserved_  : 2;
        uint8_t           trig_a_ffmt : 1;
        uint8_t           trig_pulse  : 1;
        uint8_t           trig_lndprt : 1;
        uint8_t           trig_trans  : 1;
    } b;
    uint8_t w;
} MMA8652_TRIG_CFG_t;


/*
** TRIG_CFG - Bit field mask definitions
*/
#define MMA8652_TRIG_CFG_TRIG_A_FFMT_MASK   ((uint8_t) 0x04)
#define MMA8652_TRIG_CFG_TRIG_A_FFMT_SHIFT  ((uint8_t)    2)

#define MMA8652_TRIG_CFG_TRIG_PULSE_MASK    ((uint8_t) 0x08)
#define MMA8652_TRIG_CFG_TRIG_PULSE_SHIFT   ((uint8_t)    3)

#define MMA8652_TRIG_CFG_TRIG_LNDPRT_MASK   ((uint8_t) 0x10)
#define MMA8652_TRIG_CFG_TRIG_LNDPRT_SHIFT  ((uint8_t)    4)

#define MMA8652_TRIG_CFG_TRIG_TRANS_MASK    ((uint8_t) 0x20)
#define MMA8652_TRIG_CFG_TRIG_TRANS_SHIFT   ((uint8_t)    5)


/*
** TRIG_CFG - Bit field value definitions
*/
#define MMA8652_TRIG_CFG_TRIG_A_FFMT_EN        ((uint8_t) 0x04)  /*  Freefall/motion interrupt FIFO trigger enable       */
#define MMA8652_TRIG_CFG_TRIG_A_FFMT_DIS       ((uint8_t) 0x00)  /*  Freefall/motion interrupt FIFO trigger disable      */
#define MMA8652_TRIG_CFG_TRIG_PULSE_EN         ((uint8_t) 0x08)  /*  Pluse interrupt FIFO trigger enable                 */
#define MMA8652_TRIG_CFG_TRIG_PULSE_DIS        ((uint8_t) 0x00)  /*  Pluse FIFO trigger enable                           */
#define MMA8652_TRIG_CFG_TRIG_LNDPRT_EN        ((uint8_t) 0x10)  /*  Landscape/portrait orientation interrupt FIFO       */
                                                                  /*  trigger enable                                      */
#define MMA8652_TRIG_CFG_TRIG_LNDPRT_DIS       ((uint8_t) 0x00)  /*  Landscape/portrait orientation interrupt FIFO       */
                                                                  /*  trigger disable                                     */
#define MMA8652_TRIG_CFG_TRIG_TRANS_EN         ((uint8_t) 0x20)  /*  Transient interrupt FIFO trigger enable             */
#define MMA8652_TRIG_CFG_TRIG_TRANS_DIS        ((uint8_t) 0x00)  /*  Transient interrupt FIFO trigger disable            */
/*------------------------------*/



/*--------------------------------
** Register: SYSMOD
** Enum: MMA8652_SYSMOD
** --
** Offset : 0x0B - Current system mode.
** ------------------------------*/
typedef union {
    struct {
        uint8_t                sysmod : 2;
        uint8_t                   fgt : 5;
        uint8_t                 fgerr : 1;
    } b;
    uint8_t w;
} MMA8652_SYSMOD_t;


/*
** SYSMOD - Bit field mask definitions
*/
#define MMA8652_SYSMOD_SYSMOD_MASK   ((uint8_t) 0x03)
#define MMA8652_SYSMOD_SYSMOD_SHIFT  ((uint8_t)    0)

#define MMA8652_SYSMOD_FGT_MASK      ((uint8_t) 0x7C)
#define MMA8652_SYSMOD_FGT_SHIFT     ((uint8_t)    2)

#define MMA8652_SYSMOD_FGERR_MASK    ((uint8_t) 0x80)
#define MMA8652_SYSMOD_FGERR_SHIFT   ((uint8_t)    7)


/*
** SYSMOD - Bit field value definitions
*/
#define MMA8652_SYSMOD_SYSMOD_STANDBY        ((uint8_t) 0x00)  /*  Standby mode                                          */
#define MMA8652_SYSMOD_SYSMOD_WAKE           ((uint8_t) 0x01)  /*  Wake mode                                             */
#define MMA8652_SYSMOD_SYSMOD_SLEEP          ((uint8_t) 0x02)  /*  Sleep mode                                            */
/*------------------------------*/




/*--------------------------------
** Register: INT_SOURCE
** Enum: MMA8652_INT_SOURCE
** --
** Offset : 0x0C - Interrupt status.
** ------------------------------*/
typedef union {
    struct {
        uint8_t              src_drdy : 1;
        uint8_t            _reserved_ : 1;
        uint8_t              src_ffmt : 1;
        uint8_t             src_pulse : 1;
        uint8_t            src_lndprt : 1;
        uint8_t             src_trans : 1;
        uint8_t              src_fifo : 1;
        uint8_t              src_aslp : 1;
    } b;
    uint8_t w;
} MMA8652_INT_SOURCE_t;


/*
** INT_SOURCE - Bit field mask definitions
*/
#define MMA8652_INT_SOURCE_SRC_DRDY_MASK     ((uint8_t) 0x01)
#define MMA8652_INT_SOURCE_SRC_DRDY_SHIFT    ((uint8_t)    0)

#define MMA8652_INT_SOURCE_SRC_FFMT_MASK     ((uint8_t) 0x04)
#define MMA8652_INT_SOURCE_SRC_FFMT_SHIFT    ((uint8_t)    2)

#define MMA8652_INT_SOURCE_SRC_PULSE_MASK    ((uint8_t) 0x08)
#define MMA8652_INT_SOURCE_SRC_PULSE_SHIFT   ((uint8_t)    3)

#define MMA8652_INT_SOURCE_SRC_LNDPRT_MASK   ((uint8_t) 0x10)
#define MMA8652_INT_SOURCE_SRC_LNDPRT_SHIFT  ((uint8_t)    4)

#define MMA8652_INT_SOURCE_SRC_TRANS_MASK    ((uint8_t) 0x20)
#define MMA8652_INT_SOURCE_SRC_TRANS_SHIFT   ((uint8_t)    5)

#define MMA8652_INT_SOURCE_SRC_FIFO_MASK     ((uint8_t) 0x40)
#define MMA8652_INT_SOURCE_SRC_FIFO_SHIFT    ((uint8_t)    6)

#define MMA8652_INT_SOURCE_SRC_ASLP_MASK     ((uint8_t) 0x80)
#define MMA8652_INT_SOURCE_SRC_ASLP_SHIFT    ((uint8_t)    7)


/*------------------------------*/



/*--------------------------------
** Register: WHO_AM_I
** Enum: MMA8652_WHO_AM_I
** --
** Offset : 0x0D - Device ID.
** ------------------------------*/
typedef uint8_t MMA8652_WHO_AM_I_t;



/*--------------------------------
** Register: XYZ_DATA_CFG
** Enum: MMA8652_XYZ_DATA_CFG
** --
** Offset : 0x0E - Acceleration dynamic range and filter enable settings.
** ------------------------------*/
typedef union {
    struct {
        uint8_t                    fs : 2;
        uint8_t _reserved_            : 2;
        uint8_t               hpf_out : 1; /*  - Enable high-pass filter on acceleration output data                      */

    } b;
    uint8_t w;
} MMA8652_XYZ_DATA_CFG_t;


/*
** XYZ_DATA_CFG - Bit field mask definitions
*/
#define MMA8652_XYZ_DATA_CFG_FS_MASK        ((uint8_t) 0x03)
#define MMA8652_XYZ_DATA_CFG_FS_SHIFT       ((uint8_t)    0)

#define MMA8652_XYZ_DATA_CFG_HPF_OUT_MASK   ((uint8_t) 0x10)
#define MMA8652_XYZ_DATA_CFG_HPF_OUT_SHIFT  ((uint8_t)    4)


/*
** XYZ_DATA_CFG - Bit field value definitions
*/
#define MMA8652_XYZ_DATA_CFG_HPF_OUT_EN            ((uint8_t) 0x10)  /*  Enable high-pass filter on acceleration output  */
                                                                      /*  data                                            */
#define MMA8652_XYZ_DATA_CFG_HPF_OUT_DISABLE       ((uint8_t) 0x00)  /*  High-pass filter is disabled                    */
#define MMA8652_XYZ_DATA_CFG_FS_2G           	   ((uint8_t) 0x00)  /*  full scacle is 2g                                   */
#define MMA8652_XYZ_DATA_CFG_FS_4G           	   ((uint8_t) 0x01)  /*  full scacle is 4g                                     */
#define MMA8652_XYZ_DATA_CFG_FS_8G           	   ((uint8_t) 0x02)  /*  full scacle is 8g                                     */
/*------------------------------*/



/*--------------------------------
** Register: HP_FILTER_CUTOFF
** Enum: MMA8652_HP_FILTER_CUTOFF
** --
** Offset : 0x0F - Pulse detection highpass and lowpass filter enabling bits.
** ------------------------------*/
typedef union {
    struct {
        uint8_t                   sel : 2;
        uint8_t _reserved_            : 2;
        uint8_t          pulse_lpf_en : 1;
        uint8_t         pulse_hpf_byp : 1;
    } b;
    uint8_t w;
} MMA8652_HP_FILTER_CUTOFF_t;


/*
** HP_FILTER_CUTOFF - Bit field mask definitions
*/
#define MMA8652_HP_FILTER_CUTOFF_SEL_MASK             ((uint8_t) 0x03)
#define MMA8652_HP_FILTER_CUTOFF_SEL_SHIFT            ((uint8_t)    0)

#define MMA8652_HP_FILTER_CUTOFF_PULSE_LPF_EN_MASK    ((uint8_t) 0x10)
#define MMA8652_HP_FILTER_CUTOFF_PULSE_LPF_EN_SHIFT   ((uint8_t)    4)

#define MMA8652_HP_FILTER_CUTOFF_PULSE_HPF_BYP_MASK   ((uint8_t) 0x20)
#define MMA8652_HP_FILTER_CUTOFF_PULSE_HPF_BYP_SHIFT  ((uint8_t)    5)


/*
** HP_FILTER_CUTOFF - Bit field value definitions
*/
#define MMA8652_HP_FILTER_CUTOFF_PULSE_HPF_BYP_EN      ((uint8_t) 0x00)  /*  HPF enabled for pulse processing            */
#define MMA8652_HP_FILTER_CUTOFF_PULSE_HPF_BYP_BYPASS  ((uint8_t) 0x20)  /*  HPF bypassed for pulse processing           */
#define MMA8652_HP_FILTER_CUTOFF_PULSE_LPF_EN_EN       ((uint8_t) 0x10)  /*  LPF enabled for pulse processing            */
#define MMA8652_HP_FILTER_CUTOFF_PULSE_LPF_EN_DISABLE  ((uint8_t) 0x00)  /*  LPF bypassed for pulse processing           */
#define MMA8652_HP_FILTER_CUTOFF_SEL_00                ((uint8_t) 0x00)  /*  HPF cutoff frequency selection       */
#define MMA8652_HP_FILTER_CUTOFF_SEL_01           	   ((uint8_t) 0x01)  /*  HPF cutoff frequency selection      */
#define MMA8652_HP_FILTER_CUTOFF_SEL_10                ((uint8_t) 0x02)  /*  HPF cutoff frequency selection       */
#define MMA8652_HP_FILTER_CUTOFF_SEL_11           	   ((uint8_t) 0x03)  /*  HPF cutoff frequency selection      */
/*------------------------------*/



/*--------------------------------
** Register: PL_STATUS
** Enum: MMA8652_PL_STATUS
** --
** Offset : 0x10 - Landscape/portrait orientation status.
** ------------------------------*/
typedef union {
    struct {
        uint8_t                 bafro : 1;
        uint8_t                  lapo : 2;
        uint8_t _reserved_            : 3;
        uint8_t                    lo : 1;
        uint8_t                 newlp : 1;
    } b;
    uint8_t w;
} MMA8652_PL_STATUS_t;


/*
** PL_STATUS - Bit field mask definitions
*/
#define MMA8652_PL_STATUS_BAFRO_MASK   ((uint8_t) 0x01)
#define MMA8652_PL_STATUS_BAFRO_SHIFT  ((uint8_t)    0)

#define MMA8652_PL_STATUS_LAPO_MASK    ((uint8_t) 0x06)
#define MMA8652_PL_STATUS_LAPO_SHIFT   ((uint8_t)    1)

#define MMA8652_PL_STATUS_LO_MASK      ((uint8_t) 0x40)
#define MMA8652_PL_STATUS_LO_SHIFT     ((uint8_t)    6)

#define MMA8652_PL_STATUS_NEWLP_MASK   ((uint8_t) 0x80)
#define MMA8652_PL_STATUS_NEWLP_SHIFT  ((uint8_t)    7)


/*------------------------------*/



/*--------------------------------
** Register: PL_CFG
** Enum: MMA8652_PL_CFG
** --
** Offset : 0x11 - Landscape/portrait configuration.
** ------------------------------*/
typedef union {
    struct {
        uint8_t _reserved_            : 6;
        uint8_t                 pl_en : 1;
        uint8_t                dbcntm : 1;
    } b;
    uint8_t w;
} MMA8652_PL_CFG_t;


/*
** PL_CFG - Bit field mask definitions
*/
#define MMA8652_PL_CFG_PL_EN_MASK    ((uint8_t) 0x40)
#define MMA8652_PL_CFG_PL_EN_SHIFT   ((uint8_t)    6)

#define MMA8652_PL_CFG_DBCNTM_MASK   ((uint8_t) 0x80)
#define MMA8652_PL_CFG_DBCNTM_SHIFT  ((uint8_t)    7)


/*
** PL_CFG - Bit field value definitions
*/
#define MMA8652_PL_CFG_DBCNTM_DECREMENT_MODE ((uint8_t) 0x00)  /*  Decrements debounce whenever condition of interest is */
                                                                /*  no longer valid                                       */
#define MMA8652_PL_CFG_DBCNTM_CLEAR_MODE     ((uint8_t) 0x80)  /*  Clears counter whenever condition of interest is no   */
                                                                /*  longer valid                                          */
#define MMA8652_PL_CFG_PL_EN_DISABLE         ((uint8_t) 0x00)  /*  Portrait/Landscape detection is disabled.             */
#define MMA8652_PL_CFG_PL_EN_ENABLE          ((uint8_t) 0x40)  /*  Portrait/Landscape detection is enabled.              */
/*------------------------------*/



/*--------------------------------
** Register: PL_COUNT
** Enum: MMA8652_PL_COUNT
** --
** Offset : 0x12 - Landscape/portrait debounce counter.
** ------------------------------*/
typedef union {
    struct {
        uint8_t                 dbnce;
    } b;
    uint8_t w;
} MMA8652_PL_COUNT_t;


/*
** PL_COUNT - Bit field mask definitions
*/
#define MMA8652_PL_COUNT_DBNCE_MASK   ((uint8_t) 0xFF)
#define MMA8652_PL_COUNT_DBNCE_SHIFT  ((uint8_t)    0)


/*------------------------------*/



/*--------------------------------
** Register: PL_BF_ZCOMP
** Enum: MMA8652_PL_BF_ZCOMP
** --
** Offset : 0x13 - Back/front trip angle threshold.
** ------------------------------*/
typedef union {
    struct {
        uint8_t                 zlock : 3;
        uint8_t _reserved_            : 3;
        uint8_t                  bkfr : 2;
    } b;
    uint8_t w;
} MMA8652_PL_BF_ZCOMP_t;


/*
** PL_BF_ZCOMP - Bit field mask definitions
*/
#define MMA8652_PL_BF_ZCOMP_ZLOCK_MASK   ((uint8_t) 0x07)
#define MMA8652_PL_BF_ZCOMP_ZLOCK_SHIFT  ((uint8_t)    0)

#define MMA8652_PL_BF_ZCOMP_BKFR_MASK    ((uint8_t) 0xC0)
#define MMA8652_PL_BF_ZCOMP_BKFR_SHIFT   ((uint8_t)    6)


/*
** PL_BF_ZCOMP - Bit field value definitions
*/
#define MMA8652_PL_BF_ZCOMP_BKFR_BF_LT80_GT280__FB_LT260_GT100 ((uint8_t) 0x00)
#define MMA8652_PL_BF_ZCOMP_BKFR_BF_LT75_GT285__FB_LT255_GT105 ((uint8_t) 0x40)
#define MMA8652_PL_BF_ZCOMP_BKFR_BF_LT70_GT290__FB_LT250_GT110 ((uint8_t) 0x80)
#define MMA8652_PL_BF_ZCOMP_BKFR_BF_LT65_GT295__FB_LT245_GT115 ((uint8_t) 0xc0)
#define MMA8652_PL_BF_ZCOMP_ZLOCK_13P6MIN_14P5MAX ((uint8_t) 0x00)
#define MMA8652_PL_BF_ZCOMP_ZLOCK_17P1MIN_18P2MAX ((uint8_t) 0x01)
#define MMA8652_PL_BF_ZCOMP_ZLOCK_20P7MIN_22P0MAX ((uint8_t) 0x02)
#define MMA8652_PL_BF_ZCOMP_ZLOCK_24P4MIN_25P9MAX ((uint8_t) 0x04)
#define MMA8652_PL_BF_ZCOMP_ZLOCK_28P1MIN_30P0MAX ((uint8_t) 0x04)
#define MMA8652_PL_BF_ZCOMP_ZLOCK_32P0MIN_34P2MAX ((uint8_t) 0x05)
#define MMA8652_PL_BF_ZCOMP_ZLOCK_36P1MIN_38P7MAX ((uint8_t) 0x06)
#define MMA8652_PL_BF_ZCOMP_ZLOCK_40P4MIN_43P4MAX ((uint8_t) 0x07)
/*------------------------------*/



/*--------------------------------
** Register: PL_THS_REG
** Enum: MMA8652_PL_THS_REG
** --
** Offset : 0x14 - Portrait to landscape trip threshold angle and hysteresis settings.
** ------------------------------*/
typedef union {
    struct {
        uint8_t                   hys : 3;
        uint8_t                pl_ths : 5;
    } b;
    uint8_t w;
} MMA8652_PL_THS_REG_t;


/*
** PL_THS_REG - Bit field mask definitions
*/
#define MMA8652_PL_THS_REG_HYS_MASK      ((uint8_t) 0x07)
#define MMA8652_PL_THS_REG_HYS_SHIFT     ((uint8_t)    0)

#define MMA8652_PL_THS_REG_PL_THS_MASK   ((uint8_t) 0xF8)
#define MMA8652_PL_THS_REG_PL_THS_SHIFT  ((uint8_t)    3)


/*
** PL_THS_REG - Bit field value definitions
*/
#define MMA8652_PL_THS_REG_PL_THS_15DEG          ((uint8_t) 0x38)
#define MMA8652_PL_THS_REG_PL_THS_20DEG          ((uint8_t) 0x48)
#define MMA8652_PL_THS_REG_PL_THS_30DEG          ((uint8_t) 0x60)
#define MMA8652_PL_THS_REG_PL_THS_35DEG          ((uint8_t) 0x68)
#define MMA8652_PL_THS_REG_PL_THS_40DEG          ((uint8_t) 0x78)
#define MMA8652_PL_THS_REG_PL_THS_45DEG          ((uint8_t) 0x80)
#define MMA8652_PL_THS_REG_PL_THS_55DEG          ((uint8_t) 0x98)
#define MMA8652_PL_THS_REG_PL_THS_60DEG          ((uint8_t) 0xa0)
#define MMA8652_PL_THS_REG_PL_THS_70DEG          ((uint8_t) 0xb8)
#define MMA8652_PL_THS_REG_PL_THS_75DEG          ((uint8_t) 0xc8)
#define MMA8652_PL_THS_REG_HYS_LP45_PL45         ((uint8_t) 0x00)
#define MMA8652_PL_THS_REG_HYS_LP49_PL41         ((uint8_t) 0x01)
#define MMA8652_PL_THS_REG_HYS_LP52_PL38         ((uint8_t) 0x02)
#define MMA8652_PL_THS_REG_HYS_LP56_PL34         ((uint8_t) 0x03)
#define MMA8652_PL_THS_REG_HYS_LP59_PL31         ((uint8_t) 0x04)
#define MMA8652_PL_THS_REG_HYS_LP62_PL28         ((uint8_t) 0x05)
#define MMA8652_PL_THS_REG_HYS_LP66_PL24         ((uint8_t) 0x06)
#define MMA8652_PL_THS_REG_HYS_LP69_PL21         ((uint8_t) 0x07)
/*------------------------------*/



/*--------------------------------
** Register: A_FFMT_CFG
** Enum: MMA8652_A_FFMT_CFG
** --
** Offset : 0x15 - Freefall/motion function configuration.
** ------------------------------*/
typedef union {
    struct {
        uint8_t _reserved_            : 3;
        uint8_t                  xefe : 1;
        uint8_t                  yefe : 1;
        uint8_t                  zefe : 1;
        uint8_t                   oae : 1;
        uint8_t                   ele : 1;
    } b;
    uint8_t w;
} MMA8652_A_FFMT_CFG_t;


/*
** A_FFMT_CFG - Bit field mask definitions
*/
#define MMA8652_A_FFMT_CFG_XEFE_MASK   ((uint8_t) 0x08)
#define MMA8652_A_FFMT_CFG_XEFE_SHIFT  ((uint8_t)    3)

#define MMA8652_A_FFMT_CFG_YEFE_MASK   ((uint8_t) 0x10)
#define MMA8652_A_FFMT_CFG_YEFE_SHIFT  ((uint8_t)    4)

#define MMA8652_A_FFMT_CFG_ZEFE_MASK   ((uint8_t) 0x20)
#define MMA8652_A_FFMT_CFG_ZEFE_SHIFT  ((uint8_t)    5)

#define MMA8652_A_FFMT_CFG_OAE_MASK    ((uint8_t) 0x40)
#define MMA8652_A_FFMT_CFG_OAE_SHIFT   ((uint8_t)    6)

#define MMA8652_A_FFMT_CFG_ELE_MASK    ((uint8_t) 0x80)
#define MMA8652_A_FFMT_CFG_ELE_SHIFT   ((uint8_t)    7)


/*
** A_FFMT_CFG - Bit field value definitions
*/
#define MMA8652_A_FFMT_CFG_ELE_EN                ((uint8_t) 0x80)  /*  Event flag latch enabled                          */
#define MMA8652_A_FFMT_CFG_ELE_DIS               ((uint8_t) 0x00)  /*  Event flag latch disabled                         */
#define MMA8652_A_FFMT_CFG_OAE_FREEFALL          ((uint8_t) 0x00)  /*  Freefall flag                                     */
#define MMA8652_A_FFMT_CFG_OAE_MOTION            ((uint8_t) 0x40)  /*  Motion flag                                       */
#define MMA8652_A_FFMT_CFG_ZEFE_DIS              ((uint8_t) 0x00)  /*  Event detection disabled                          */
#define MMA8652_A_FFMT_CFG_ZEFE_RAISE_EVENT      ((uint8_t) 0x20)  /*  Raise event flag on measured Z-axis acceleration  */
                                                                    /*  above/below threshold                             */
#define MMA8652_A_FFMT_CFG_YEFE_DIS              ((uint8_t) 0x00)  /*  Event detection disabled                          */
#define MMA8652_A_FFMT_CFG_YEFE_RAISE_EVENT      ((uint8_t) 0x10)  /*  Raise event flag on measured Y-axis acceleration  */
                                                                    /*  above/below threshold                             */
#define MMA8652_A_FFMT_CFG_XEFE_DIS              ((uint8_t) 0x00)  /*  Event detection disabled                          */
#define MMA8652_A_FFMT_CFG_XEFE_RAISE_EVENT      ((uint8_t) 0x08)  /*  Raise event flag on measured X-axis acceleration  */
                                                                    /*  above/below threshold                             */
/*------------------------------*/



/*--------------------------------
** Register: A_FFMT_SRC
** Enum: MMA8652_A_FFMT_SRC
** --
** Offset : 0x16 - Freefall/motion event source register.
** ------------------------------*/
typedef union {
    struct {
        uint8_t                   xhp : 1;
        uint8_t                   xhe : 1;
        uint8_t                   yhp : 1;
        uint8_t                   yhe : 1;
        uint8_t                   zhp : 1;
        uint8_t                   zhe : 1;
        uint8_t _reserved_            : 1;
        uint8_t                    ea : 1;
    } b;
    uint8_t w;
} MMA8652_A_FFMT_SRC_t;


/*
** A_FFMT_SRC - Bit field mask definitions
*/
#define MMA8652_A_FFMT_SRC_XHP_MASK   ((uint8_t) 0x01)
#define MMA8652_A_FFMT_SRC_XHP_SHIFT  ((uint8_t)    0)

#define MMA8652_A_FFMT_SRC_XHE_MASK   ((uint8_t) 0x02)
#define MMA8652_A_FFMT_SRC_XHE_SHIFT  ((uint8_t)    1)

#define MMA8652_A_FFMT_SRC_YHP_MASK   ((uint8_t) 0x04)
#define MMA8652_A_FFMT_SRC_YHP_SHIFT  ((uint8_t)    2)

#define MMA8652_A_FFMT_SRC_YHE_MASK   ((uint8_t) 0x08)
#define MMA8652_A_FFMT_SRC_YHE_SHIFT  ((uint8_t)    3)

#define MMA8652_A_FFMT_SRC_ZHP_MASK   ((uint8_t) 0x10)
#define MMA8652_A_FFMT_SRC_ZHP_SHIFT  ((uint8_t)    4)

#define MMA8652_A_FFMT_SRC_ZHE_MASK   ((uint8_t) 0x20)
#define MMA8652_A_FFMT_SRC_ZHE_SHIFT  ((uint8_t)    5)

#define MMA8652_A_FFMT_SRC_EA_MASK    ((uint8_t) 0x80)
#define MMA8652_A_FFMT_SRC_EA_SHIFT   ((uint8_t)    7)


/*------------------------------*/



/*--------------------------------
** Register: A_FFMT_THS
** Enum: MMA8652_A_FFMT_THS
** --
** Offset : 0x17 - Freefall/motion threshold register.
** ------------------------------*/
typedef union {
    struct {
        uint8_t                   ths : 7;
        uint8_t                dbcntm : 1;
    } b;
    uint8_t w;
} MMA8652_A_FFMT_THS_t;


/*
** A_FFMT_THS - Bit field mask definitions
*/
#define MMA8652_A_FFMT_THS_THS_MASK      ((uint8_t) 0x7F)
#define MMA8652_A_FFMT_THS_THS_SHIFT     ((uint8_t)    0)

#define MMA8652_A_FFMT_THS_DBCNTM_MASK   ((uint8_t) 0x80)
#define MMA8652_A_FFMT_THS_DBCNTM_SHIFT  ((uint8_t)    7)


/*------------------------------*/



/*--------------------------------
** Register: A_FFMT_COUNT
** Enum: MMA8652_A_FFMT_COUNT
** --
** Offset : 0x18 - Freefall/motion debounce counter.
** ------------------------------*/
typedef uint8_t MMA8652_A_FFMT_COUNT_t;




/*--------------------------------
** Register: TRANSIENT_CFG
** Enum: MMA8652_TRANSIENT_CFG
** --
** Offset : 0x1D - Transient function configuration.
** ------------------------------*/
typedef union {
    struct {
        uint8_t               hpf_byp : 1;
        uint8_t                 xtefe : 1;
        uint8_t                 ytefe : 1;
        uint8_t                 ztefe : 1;
        uint8_t                  tele : 1;
    } b;
    uint8_t w;
} MMA8652_TRANSIENT_CFG_t;


/*
** TRANSIENT_CFG - Bit field mask definitions
*/
#define MMA8652_TRANSIENT_CFG_HPF_BYP_MASK   ((uint8_t) 0x01)
#define MMA8652_TRANSIENT_CFG_HPF_BYP_SHIFT  ((uint8_t)    0)

#define MMA8652_TRANSIENT_CFG_XTEFE_MASK     ((uint8_t) 0x02)
#define MMA8652_TRANSIENT_CFG_XTEFE_SHIFT    ((uint8_t)    1)

#define MMA8652_TRANSIENT_CFG_YTEFE_MASK     ((uint8_t) 0x04)
#define MMA8652_TRANSIENT_CFG_YTEFE_SHIFT    ((uint8_t)    2)

#define MMA8652_TRANSIENT_CFG_ZTEFE_MASK     ((uint8_t) 0x08)
#define MMA8652_TRANSIENT_CFG_ZTEFE_SHIFT    ((uint8_t)    3)

#define MMA8652_TRANSIENT_CFG_TELE_MASK      ((uint8_t) 0x10)
#define MMA8652_TRANSIENT_CFG_TELE_SHIFT     ((uint8_t)    4)


/*
** TRANSIENT_CFG - Bit field value definitions
*/
#define MMA8652_TRANSIENT_CFG_TELE_EN               ((uint8_t) 0x10)  /*  Event flag latch enabled: the transient        */
                                                                       /*  interrupt event flag is latched and a read of  */
                                                                       /*  the TRANSIENT_SRC register is required to      */
                                                                       /*  clear the event flag                           */
#define MMA8652_TRANSIENT_CFG_TELE_DIS              ((uint8_t) 0x00)  /*  Event flag latch disabled: the transient       */
                                                                       /*  interrupt flag reflects the real-time status   */
                                                                       /*  of the function                                */
#define MMA8652_TRANSIENT_CFG_ZTEFE_EN              ((uint8_t) 0x08)  /*  Z-axis event detection enabled. Raise event    */
                                                                       /*  flag on Z-axis acceleration value greater than */
                                                                       /*  threshold                                      */
#define MMA8652_TRANSIENT_CFG_ZTEFE_DIS             ((uint8_t) 0x00)  /*  Z-axis event detection disabled                */
#define MMA8652_TRANSIENT_CFG_YTEFE_EN              ((uint8_t) 0x04)  /*  Y-axis event detection enabled. Raise event    */
                                                                       /*  flag on Y-axis acceleration value greater than */
                                                                       /*  threshold                                      */
#define MMA8652_TRANSIENT_CFG_YTEFE_DIS             ((uint8_t) 0x00)  /*  Y-axis event detection disabled                */
#define MMA8652_TRANSIENT_CFG_XTEFE_EN              ((uint8_t) 0x02)  /*  X-axis event detection enabled. Raise event    */
                                                                       /*  flag on X-axis acceleration value greater than */
                                                                       /*  threshold                                      */
#define MMA8652_TRANSIENT_CFG_XTEFE_DIS             ((uint8_t) 0x00)  /*  X-axis event detection disabled                */
#define MMA8652_TRANSIENT_CFG_HPF_BYP_EN            ((uint8_t) 0x01)  /*  High-pass filter is not applied to             */
                                                                       /*  accelerometer data input to the transient      */
                                                                       /*  function                                       */
#define MMA8652_TRANSIENT_CFG_HPF_BYP_DIS           ((uint8_t) 0x00)  /*  High-pass filter is applied to accelerometer   */
                                                                       /*  data input to the transient function           */
/*------------------------------*/



/*--------------------------------
** Register: TRANSIENT_SRC
** Enum: MMA8652_TRANSIENT_SRC
** --
** Offset : 0x1E - Transient event status register.
** ------------------------------*/
typedef union {
    struct {
        uint8_t            trans_xpol : 1;
        uint8_t              tran_xef : 1;
        uint8_t             tran_ypol : 1;
        uint8_t              tran_yef : 1;
        uint8_t             tran_zpol : 1;
        uint8_t              tran_zef : 1;
        uint8_t               tran_ea : 1;
    } b;
    uint8_t w;
} MMA8652_TRANSIENT_SRC_t;


/*
** TRANSIENT_SRC - Bit field mask definitions
*/
#define MMA8652_TRANSIENT_SRC_TRANS_XPOL_MASK   ((uint8_t) 0x01)
#define MMA8652_TRANSIENT_SRC_TRANS_XPOL_SHIFT  ((uint8_t)    0)

#define MMA8652_TRANSIENT_SRC_TRAN_XEF_MASK     ((uint8_t) 0x02)
#define MMA8652_TRANSIENT_SRC_TRAN_XEF_SHIFT    ((uint8_t)    1)

#define MMA8652_TRANSIENT_SRC_TRAN_YPOL_MASK    ((uint8_t) 0x04)
#define MMA8652_TRANSIENT_SRC_TRAN_YPOL_SHIFT   ((uint8_t)    2)

#define MMA8652_TRANSIENT_SRC_TRAN_YEF_MASK     ((uint8_t) 0x08)
#define MMA8652_TRANSIENT_SRC_TRAN_YEF_SHIFT    ((uint8_t)    3)

#define MMA8652_TRANSIENT_SRC_TRAN_ZPOL_MASK    ((uint8_t) 0x10)
#define MMA8652_TRANSIENT_SRC_TRAN_ZPOL_SHIFT   ((uint8_t)    4)

#define MMA8652_TRANSIENT_SRC_TRAN_ZEF_MASK     ((uint8_t) 0x20)
#define MMA8652_TRANSIENT_SRC_TRAN_ZEF_SHIFT    ((uint8_t)    5)

#define MMA8652_TRANSIENT_SRC_TRAN_EA_MASK      ((uint8_t) 0x40)
#define MMA8652_TRANSIENT_SRC_TRAN_EA_SHIFT     ((uint8_t)    6)


/*------------------------------*/



/*--------------------------------
** Register: TRANSIENT_THS
** Enum: MMA8652_TRANSIENT_THS
** --
** Offset : 0x1F - Transient event threshold.
** ------------------------------*/
typedef union {
    struct {
        uint8_t                tr_ths : 7;
        uint8_t             tr_dbcntm : 1;
    } b;
    uint8_t w;
} MMA8652_TRANSIENT_THS_t;


/*
** TRANSIENT_THS - Bit field mask definitions
*/
#define MMA8652_TRANSIENT_THS_TR_THS_MASK      ((uint8_t) 0x7F)
#define MMA8652_TRANSIENT_THS_TR_THS_SHIFT     ((uint8_t)    0)

#define MMA8652_TRANSIENT_THS_TR_DBCNTM_MASK   ((uint8_t) 0x80)
#define MMA8652_TRANSIENT_THS_TR_DBCNTM_SHIFT  ((uint8_t)    7)


/*
** TRANSIENT_THS - Bit field value definitions
*/
#define MMA8652_TRANSIENT_THS_TR_THS_DECREMENTS     ((uint8_t) 0x00)  /*  Decrements debounce counter when the transient */
                                                                       /*  event condition is not true during the current */
                                                                       /*  ODR period                                     */
#define MMA8652_TRANSIENT_THS_TR_THS_CLEAR          ((uint8_t) 0x01)  /*  Clears debounce counter when the transient     */
                                                                       /*  event condition is not true during the current */
                                                                       /*  ODR period                                     */
/*------------------------------*/



/*--------------------------------
** Register: TRANSIENT_COUNT
** Enum: MMA8652_TRANSIENT_COUNT
** --
** Offset : 0x20 - Transient debounce counter.
** ------------------------------*/
typedef uint8_t MMA8652_TRANSIENT_COUNT_t;





/*--------------------------------
** Register: PULSE_CFG
** Enum: MMA8652_PULSE_CFG
** --
** Offset : 0x21 - Pulse function configuration.
** ------------------------------*/
typedef union {
    struct {
        uint8_t            pls_xspefe : 1;
        uint8_t            pls_xdpefe : 1;
        uint8_t            pls_yspefe : 1;
        uint8_t            pls_ydpefe : 1;
        uint8_t            pls_zspefe : 1;
        uint8_t            pls_zdpefe : 1;
        uint8_t               pls_ele : 1;
        uint8_t               pls_dpa : 1;
    } b;
    uint8_t w;
} MMA8652_PULSE_CFG_t;


/*
** PULSE_CFG - Bit field mask definitions
*/
#define MMA8652_PULSE_CFG_PLS_XSPEFE_MASK   ((uint8_t) 0x01)
#define MMA8652_PULSE_CFG_PLS_XSPEFE_SHIFT  ((uint8_t)    0)

#define MMA8652_PULSE_CFG_PLS_XDPEFE_MASK   ((uint8_t) 0x02)
#define MMA8652_PULSE_CFG_PLS_XDPEFE_SHIFT  ((uint8_t)    1)

#define MMA8652_PULSE_CFG_PLS_YSPEFE_MASK   ((uint8_t) 0x04)
#define MMA8652_PULSE_CFG_PLS_YSPEFE_SHIFT  ((uint8_t)    2)

#define MMA8652_PULSE_CFG_PLS_YDPEFE_MASK   ((uint8_t) 0x08)
#define MMA8652_PULSE_CFG_PLS_YDPEFE_SHIFT  ((uint8_t)    3)

#define MMA8652_PULSE_CFG_PLS_ZSPEFE_MASK   ((uint8_t) 0x10)
#define MMA8652_PULSE_CFG_PLS_ZSPEFE_SHIFT  ((uint8_t)    4)

#define MMA8652_PULSE_CFG_PLS_ZDPEFE_MASK   ((uint8_t) 0x20)
#define MMA8652_PULSE_CFG_PLS_ZDPEFE_SHIFT  ((uint8_t)    5)

#define MMA8652_PULSE_CFG_PLS_ELE_MASK      ((uint8_t) 0x40)
#define MMA8652_PULSE_CFG_PLS_ELE_SHIFT     ((uint8_t)    6)

#define MMA8652_PULSE_CFG_PLS_DPA_MASK      ((uint8_t) 0x80)
#define MMA8652_PULSE_CFG_PLS_DPA_SHIFT     ((uint8_t)    7)


/*
** PULSE_CFG - Bit field value definitions
*/
#define MMA8652_PULSE_CFG_PLS_DPA_DIS           ((uint8_t) 0x80)  /*  Setting the pls_dpa bit momentarily suspends the   */
                                                                   /*  double-tap detection if the start of a pulse is    */
                                                                   /*  detected during the time period specified by the   */
                                                                   /*  PULSE_LTCY register and the pulse ends before the  */
                                                                   /*  end of the time period specified by the PULSE_LTCY */
                                                                   /*  register                                           */
#define MMA8652_PULSE_CFG_PLS_DPA_EN            ((uint8_t) 0x00)  /*  Double-pulse detection is not aborted if the start */
                                                                   /*  of a pulse is detected during the time period      */
                                                                   /*  specified by the PULSE_LTCY register               */
#define MMA8652_PULSE_CFG_PLS_ELE_DIS           ((uint8_t) 0x00)  /*  Event flag latch disabled                          */
#define MMA8652_PULSE_CFG_PLS_ELE_EN            ((uint8_t) 0x40)  /*  Event flag latch enabled                           */
#define MMA8652_PULSE_CFG_PLS_ZDPEFE_DIS        ((uint8_t) 0x00)  /*  Event flag latch disabled                          */
#define MMA8652_PULSE_CFG_PLS_ZDPEFE_EN         ((uint8_t) 0x20)  /*  Raise event flag on detection of double-pulse      */
                                                                   /*  event on Z-axis                                    */
#define MMA8652_PULSE_CFG_PLS_ZSPEFE_DIS        ((uint8_t) 0x00)  /*  Event flag latch disabled                          */
#define MMA8652_PULSE_CFG_PLS_ZSPEFE_EN         ((uint8_t) 0x10)  /*  Raise event flag on detection of single-pulse      */
                                                                   /*  event on Z-axis                                    */
#define MMA8652_PULSE_CFG_PLS_YDPEFE_DIS        ((uint8_t) 0x00)  /*  Event flag latch disabled                          */
#define MMA8652_PULSE_CFG_PLS_YDPEFE_EN         ((uint8_t) 0x08)  /*  Raise event flag on detection of double-pulse      */
                                                                   /*  event on Y-axis                                    */
#define MMA8652_PULSE_CFG_PLS_YSPEFE_DIS        ((uint8_t) 0x00)  /*  Event flag latch disabled                          */
#define MMA8652_PULSE_CFG_PLS_YSPEFE_EN         ((uint8_t) 0x04)  /*  Raise event flag on detection of single-pulse      */
                                                                   /*  event on Y-axis                                    */
#define MMA8652_PULSE_CFG_PLS_XDPEFE_DIS        ((uint8_t) 0x00)  /*  Event flag latch disabled                          */
#define MMA8652_PULSE_CFG_PLS_XDPEFE_EN         ((uint8_t) 0x02)  /*  Raise event flag on detection of double-pulse      */
                                                                   /*  event on X-axis                                    */
#define MMA8652_PULSE_CFG_PLS_XSPEFE_DIS        ((uint8_t) 0x00)  /*  Event flag latch disabled                          */
#define MMA8652_PULSE_CFG_PLS_XSPEFE_EN         ((uint8_t) 0x01)  /*  Raise event flag on detection of single-pulse      */
                                                                   /*  event on X-axis                                    */
/*------------------------------*/



/*--------------------------------
** Register: PULSE_SRC
** Enum: MMA8652_PULSE_SRC
** --
** Offset : 0x22 - Pulse function source register.
** ------------------------------*/
typedef union {
    struct {
        uint8_t          pls_src_polx : 1;
        uint8_t          pls_src_poly : 1;
        uint8_t          pls_src_polz : 1;
        uint8_t           pls_src_dpe : 1;
        uint8_t           pls_src_axx : 1;
        uint8_t           pls_src_axy : 1;
        uint8_t           pls_src_axz : 1;
        uint8_t            pls_src_ea : 1;
    } b;
    uint8_t w;
} MMA8652_PULSE_SRC_t;


/*
** PULSE_SRC - Bit field mask definitions
*/
#define MMA8652_PULSE_SRC_PLS_SRC_POLX_MASK   ((uint8_t) 0x01)
#define MMA8652_PULSE_SRC_PLS_SRC_POLX_SHIFT  ((uint8_t)    0)

#define MMA8652_PULSE_SRC_PLS_SRC_POLY_MASK   ((uint8_t) 0x02)
#define MMA8652_PULSE_SRC_PLS_SRC_POLY_SHIFT  ((uint8_t)    1)

#define MMA8652_PULSE_SRC_PLS_SRC_POLZ_MASK   ((uint8_t) 0x04)
#define MMA8652_PULSE_SRC_PLS_SRC_POLZ_SHIFT  ((uint8_t)    2)

#define MMA8652_PULSE_SRC_PLS_SRC_DPE_MASK    ((uint8_t) 0x08)
#define MMA8652_PULSE_SRC_PLS_SRC_DPE_SHIFT   ((uint8_t)    3)

#define MMA8652_PULSE_SRC_PLS_SRC_AXX_MASK    ((uint8_t) 0x10)
#define MMA8652_PULSE_SRC_PLS_SRC_AXX_SHIFT   ((uint8_t)    4)

#define MMA8652_PULSE_SRC_PLS_SRC_AXY_MASK    ((uint8_t) 0x20)
#define MMA8652_PULSE_SRC_PLS_SRC_AXY_SHIFT   ((uint8_t)    5)

#define MMA8652_PULSE_SRC_PLS_SRC_AXZ_MASK    ((uint8_t) 0x40)
#define MMA8652_PULSE_SRC_PLS_SRC_AXZ_SHIFT   ((uint8_t)    6)

#define MMA8652_PULSE_SRC_PLS_SRC_EA_MASK     ((uint8_t) 0x80)
#define MMA8652_PULSE_SRC_PLS_SRC_EA_SHIFT    ((uint8_t)    7)


/*------------------------------*/



/*--------------------------------
** Register: PULSE_THSX
** Enum: MMA8652_PULSE_THSX
** --
** Offset : 0x23 - X-axis pulse threshold.
** ------------------------------*/
typedef union {
    struct {
        uint8_t              pls_thsx : 7;
    } b;
    uint8_t w;
} MMA8652_PULSE_THSX_t;


/*
** PULSE_THSX - Bit field mask definitions
*/
#define MMA8652_PULSE_THSX_PLS_THSX_MASK   ((uint8_t) 0x7F)
#define MMA8652_PULSE_THSX_PLS_THSX_SHIFT  ((uint8_t)    0)


/*------------------------------*/



/*--------------------------------
** Register: PULSE_THSY
** Enum: MMA8652_PULSE_THSY
** --
** Offset : 0x24 - Y-axis pulse threshold.
** ------------------------------*/
typedef union {
    struct {
        uint8_t              pls_thsy : 7;
    } b;
    uint8_t w;
} MMA8652_PULSE_THSY_t;


/*
** PULSE_THSY - Bit field mask definitions
*/
#define MMA8652_PULSE_THSY_PLS_THSY_MASK   ((uint8_t) 0x7F)
#define MMA8652_PULSE_THSY_PLS_THSY_SHIFT  ((uint8_t)    0)


/*------------------------------*/



/*--------------------------------
** Register: PULSE_THSZ
** Enum: MMA8652_PULSE_THSZ
** --
** Offset : 0x25 - Z-axis pulse threshold.
** ------------------------------*/
typedef union {
    struct {
        uint8_t              pls_thsz : 7;
    } b;
    uint8_t w;
} MMA8652_PULSE_THSZ_t;


/*
** PULSE_THSZ - Bit field mask definitions
*/
#define MMA8652_PULSE_THSZ_PLS_THSZ_MASK   ((uint8_t) 0x7F)
#define MMA8652_PULSE_THSZ_PLS_THSZ_SHIFT  ((uint8_t)    0)


/*------------------------------*/



/*--------------------------------
** Register: PULSE_TMLT
** Enum: MMA8652_PULSE_TMLT
** --
** Offset : 0x26 - Time limit for pulse detection.
** ------------------------------*/
typedef uint8_t MMA8652_PULSE_TMLT_t;



/*--------------------------------
** Register: PULSE_LTCY
** Enum: MMA8652_PULSE_LTCY
** --
** Offset : 0x27 - Latency time for second pulse detection.
** ------------------------------*/
typedef uint8_t MMA8652_PULSE_LTCY_t;


/*--------------------------------
** Register: PULSE_WIND
** Enum: MMA8652_PULSE_WIND
** --
** Offset : 0x28 - Window time for second pulse detection.
** ------------------------------*/
typedef uint8_t MMA8652_PULSE_WIND_t;



/*--------------------------------
** Register: ASLP_COUNT
** Enum: MMA8652_ASLP_COUNT
** --
** Offset : 0x29 - The counter setting for auto-sleep period.
** ------------------------------*/
typedef uint8_t MMA8652_ASLP_COUNT_t;



/*--------------------------------
** Register: CTRL_REG1
** Enum: MMA8652_CTRL_REG1
** --
** Offset : 0x2A - System ODR, accelerometer OSR (Output sample rate), operating mode.
** ------------------------------*/
typedef union {
    struct {
        uint8_t                active : 1;
        uint8_t                f_read : 1;
        uint8_t            _reserved_ : 1;
        uint8_t                    dr : 3;
        uint8_t             aslp_rate : 2;
    } b;
    uint8_t w;
} MMA8652_CTRL_REG1_t;


/*
** CTRL_REG1 - Bit field mask definitions
*/
#define MMA8652_CTRL_REG1_ACTIVE_MASK      ((uint8_t) 0x01)
#define MMA8652_CTRL_REG1_ACTIVE_SHIFT     ((uint8_t)    0)

#define MMA8652_CTRL_REG1_F_READ_MASK      ((uint8_t) 0x02)
#define MMA8652_CTRL_REG1_F_READ_SHIFT     ((uint8_t)    1)

#define MMA8652_CTRL_REG1_DR_MASK          ((uint8_t) 0x38)
#define MMA8652_CTRL_REG1_DR_SHIFT         ((uint8_t)    3)

#define MMA8652_CTRL_REG1_ASLP_RATE_MASK   ((uint8_t) 0xC0)
#define MMA8652_CTRL_REG1_ASLP_RATE_SHIFT  ((uint8_t)    6)


/*
** CTRL_REG1 - Bit field value definitions
*/
#define MMA8652_CTRL_REG1_ASLP_RATE_50_HZ       ((uint8_t) 0x00)
#define MMA8652_CTRL_REG1_ASLP_RATE_12P5_HZ     ((uint8_t) 0x40)
#define MMA8652_CTRL_REG1_ASLP_RATE_6P25_HZ     ((uint8_t) 0x80)
#define MMA8652_CTRL_REG1_ASLP_RATE_1P56_HZ     ((uint8_t) 0xc0)
#define MMA8652_CTRL_REG1_DR_SINGLE_800_HZ      ((uint8_t) 0x00)
#define MMA8652_CTRL_REG1_DR_SINGLE_400_HZ      ((uint8_t) 0x08)
#define MMA8652_CTRL_REG1_DR_SINGLE_200_HZ      ((uint8_t) 0x10)
#define MMA8652_CTRL_REG1_DR_SINGLE_100_HZ      ((uint8_t) 0x18)
#define MMA8652_CTRL_REG1_DR_SINGLE_50_HZ       ((uint8_t) 0x20)
#define MMA8652_CTRL_REG1_DR_SINGLE_12P5_HZ     ((uint8_t) 0x28)
#define MMA8652_CTRL_REG1_DR_SINGLE_6P25_HZ     ((uint8_t) 0x30)
#define MMA8652_CTRL_REG1_DR_SINGLE_1P56_HZ   	((uint8_t) 0x38)
#define MMA8652_CTRL_REG1_F_READ_NORMAL         ((uint8_t) 0x00)  /*  Normal mode                                        */
#define MMA8652_CTRL_REG1_F_READ_FAST           ((uint8_t) 0x02)  /*  Fast-read mode                                     */
#define MMA8652_CTRL_REG1_ACTIVE_ACTIVE_MODE    ((uint8_t) 0x01)  /*  Active mode                                        */
#define MMA8652_CTRL_REG1_ACTIVE_STANDBY_MODE   ((uint8_t) 0x00)  /*  Standby mode                                       */
/*------------------------------*/



/*--------------------------------
** Register: CTRL_REG2
** Enum: MMA8652_CTRL_REG2
** --
** Offset : 0x2B - Self-test, reset, accelerometer OSR, and sleep mode settings.
** ------------------------------*/
typedef union {
    struct {
        uint8_t                  mods : 2;
        uint8_t                  slpe : 1;
        uint8_t                 smods : 2;
        uint8_t _reserved_            : 1;
        uint8_t                   rst : 1;
        uint8_t                    st : 1;
    } b;
    uint8_t w;
} MMA8652_CTRL_REG2_t;


/*
** CTRL_REG2 - Bit field mask definitions
*/
#define MMA8652_CTRL_REG2_MODS_MASK    ((uint8_t) 0x03)
#define MMA8652_CTRL_REG2_MODS_SHIFT   ((uint8_t)    0)

#define MMA8652_CTRL_REG2_SLPE_MASK    ((uint8_t) 0x04)
#define MMA8652_CTRL_REG2_SLPE_SHIFT   ((uint8_t)    2)

#define MMA8652_CTRL_REG2_SMODS_MASK   ((uint8_t) 0x18)
#define MMA8652_CTRL_REG2_SMODS_SHIFT  ((uint8_t)    3)

#define MMA8652_CTRL_REG2_RST_MASK     ((uint8_t) 0x40)
#define MMA8652_CTRL_REG2_RST_SHIFT    ((uint8_t)    6)

#define MMA8652_CTRL_REG2_ST_MASK      ((uint8_t) 0x80)
#define MMA8652_CTRL_REG2_ST_SHIFT     ((uint8_t)    7)


/*
** CTRL_REG2 - Bit field value definitions
*/
#define MMA8652_CTRL_REG2_ST_DIS                ((uint8_t) 0x00)  /*  Self-test disabled                                 */
#define MMA8652_CTRL_REG2_ST_EN                 ((uint8_t) 0x80)  /*  Self-test enabled                                  */
#define MMA8652_CTRL_REG2_RST_EN                ((uint8_t) 0x40)  /*  Device reset enabled                               */
#define MMA8652_CTRL_REG2_RST_DIS               ((uint8_t) 0x00)  /*  Device reset disabled                              */
#define MMA8652_CTRL_REG2_SMODS_NORMAL          ((uint8_t) 0x00)
#define MMA8652_CTRL_REG2_SMODS_LOW_NOISE_LOW_POWER ((uint8_t) 0x08)
#define MMA8652_CTRL_REG2_SMODS_HIGH_RES        ((uint8_t) 0x10)
#define MMA8652_CTRL_REG2_SMODS_LOW_POWER       ((uint8_t) 0x18)
#define MMA8652_CTRL_REG2_SLPE_EN               ((uint8_t) 0x04)
#define MMA8652_CTRL_REG2_SLPE_DISABLE          ((uint8_t) 0x00)
#define MMA8652_CTRL_REG2_MODS_NORMAL           ((uint8_t) 0x00)
#define MMA8652_CTRL_REG2_MODS_LOW_NOISE_LOW_POWER ((uint8_t) 0x01)
#define MMA8652_CTRL_REG2_MODS_HIGH_RES         ((uint8_t) 0x02)
#define MMA8652_CTRL_REG2_MODS_LOW_POWER        ((uint8_t) 0x03)
/*------------------------------*/



/*--------------------------------
** Register: CTRL_REG3
** Enum: MMA8652_CTRL_REG3
** --
** Offset : 0x2C - Sleep mode interrupt wake enable, interrupt polarity, push-pull/open drain configuration.
** ------------------------------*/
typedef union {
    struct {
        uint8_t                 pp_od : 1;
        uint8_t                  ipol : 1;
        uint8_t            _reserved_ : 1;
        uint8_t             wake_ffmt : 1;
        uint8_t            wake_pulse : 1;
        uint8_t           wake_lndprt : 1;
        uint8_t            wake_trans : 1;
        uint8_t             fifo_gate : 1;
    } b;
    uint8_t w;
} MMA8652_CTRL_REG3_t;


/*
** CTRL_REG3 - Bit field mask definitions
*/
#define MMA8652_CTRL_REG3_PP_OD_MASK         ((uint8_t) 0x01)
#define MMA8652_CTRL_REG3_PP_OD_SHIFT        ((uint8_t)    0)

#define MMA8652_CTRL_REG3_IPOL_MASK          ((uint8_t) 0x02)
#define MMA8652_CTRL_REG3_IPOL_SHIFT         ((uint8_t)    1)

#define MMA8652_CTRL_REG3_WAKE_FFMT_MASK     ((uint8_t) 0x08)
#define MMA8652_CTRL_REG3_WAKE_FFMT_SHIFT    ((uint8_t)    3)

#define MMA8652_CTRL_REG3_WAKE_PULSE_MASK    ((uint8_t) 0x10)
#define MMA8652_CTRL_REG3_WAKE_PULSE_SHIFT   ((uint8_t)    4)

#define MMA8652_CTRL_REG3_WAKE_LNDPRT_MASK   ((uint8_t) 0x20)
#define MMA8652_CTRL_REG3_WAKE_LNDPRT_SHIFT  ((uint8_t)    5)

#define MMA8652_CTRL_REG3_WAKE_TRANS_MASK    ((uint8_t) 0x40)
#define MMA8652_CTRL_REG3_WAKE_TRANS_SHIFT   ((uint8_t)    6)

#define MMA8652_CTRL_REG3_FIFO_GATE_MASK     ((uint8_t) 0x80)
#define MMA8652_CTRL_REG3_FIFO_GATE_SHIFT    ((uint8_t)    7)


/*
** CTRL_REG3 - Bit field value definitions
*/
#define MMA8652_CTRL_REG3_FIFO_GATE_BYPASSED    ((uint8_t) 0x00)  /*  FIFO gate is bypassed                              */
#define MMA8652_CTRL_REG3_FIFO_GATE_BLOCKED     ((uint8_t) 0x80)  /*  The FIFO input buffer is blocked from accepting    */
                                                                   /*  new samples when transitioning from wake-to-sleep  */
                                                                   /*  mode or from sleep-to-wake mode until the FIFO is  */
                                                                   /*  flushed                                            */
#define MMA8652_CTRL_REG3_WAKE_TRANS_DIS        ((uint8_t) 0x00)  /*  Transient function is disabled in sleep mode       */
#define MMA8652_CTRL_REG3_WAKE_TRANS_EN         ((uint8_t) 0x40)  /*  Transient function is enabled in sleep mode and    */
                                                                   /*  can generate an interrupt to wake the system       */
#define MMA8652_CTRL_REG3_WAKE_LNDPRT_DIS       ((uint8_t) 0x00)  /*  Orientation function is disabled sleep mode        */
#define MMA8652_CTRL_REG3_WAKE_LNDPRT_EN        ((uint8_t) 0x20)  /*  Orientation function is enabled in sleep mode and  */
                                                                   /*  can generate an interrupt to wake the system       */
#define MMA8652_CTRL_REG3_WAKE_PULSE_DIS        ((uint8_t) 0x00)  /*  Pulse function is disabled in sleep mode           */
#define MMA8652_CTRL_REG3_WAKE_PULSE_EN         ((uint8_t) 0x10)  /*  Pulse function is enabled in sleep mode and can    */
                                                                   /*  generate an interrupt to wake the system           */
#define MMA8652_CTRL_REG3_WAKE_FFMT_DIS         ((uint8_t) 0x00)  /*  Freefall/motion function is disabled in sleep mode */
#define MMA8652_CTRL_REG3_WAKE_FFMT_EN          ((uint8_t) 0x08)  /*  Freefall/motion function is enabled in sleep mode  */
                                                                   /*  and can generate an interrupt to wake the system   */
#define MMA8652_CTRL_REG3_IPOL_ACTIVE_LOW       ((uint8_t) 0x00)  /*  Active Low                                         */
#define MMA8652_CTRL_REG3_IPOL_ACTIVE_HIGH      ((uint8_t) 0x02)  /*  Active High                                        */
#define MMA8652_CTRL_REG3_PP_OD_PUSH_PULL       ((uint8_t) 0x00)  /*  Push-pull                                          */
#define MMA8652_CTRL_REG3_PP_OD_OPEN_DRAIN      ((uint8_t) 0x01)  /*  Open Drain                                         */
/*------------------------------*/



/*--------------------------------
** Register: CTRL_REG4
** Enum: MMA8652_CTRL_REG4
** --
** Offset : 0x2D - Interrupt enable register.
** ------------------------------*/
typedef union {
    struct {
        uint8_t           int_en_drdy : 1;
        uint8_t            _reserved_ : 1;
        uint8_t           int_en_ffmt : 1;
        uint8_t          int_en_pulse : 1;
        uint8_t         int_en_lndprt : 1;
        uint8_t          int_en_trans : 1;
        uint8_t           int_en_fifo : 1;
        uint8_t           int_en_aslp : 1;
    } b;
    uint8_t w;
} MMA8652_CTRL_REG4_t;


/*
** CTRL_REG4 - Bit field mask definitions
*/
#define MMA8652_CTRL_REG4_INT_EN_DRDY_MASK     ((uint8_t) 0x01)
#define MMA8652_CTRL_REG4_INT_EN_DRDY_SHIFT    ((uint8_t)    0)

#define MMA8652_CTRL_REG4_INT_EN_FFMT_MASK     ((uint8_t) 0x04)
#define MMA8652_CTRL_REG4_INT_EN_FFMT_SHIFT    ((uint8_t)    2)

#define MMA8652_CTRL_REG4_INT_EN_PULSE_MASK    ((uint8_t) 0x08)
#define MMA8652_CTRL_REG4_INT_EN_PULSE_SHIFT   ((uint8_t)    3)

#define MMA8652_CTRL_REG4_INT_EN_LNDPRT_MASK   ((uint8_t) 0x10)
#define MMA8652_CTRL_REG4_INT_EN_LNDPRT_SHIFT  ((uint8_t)    4)

#define MMA8652_CTRL_REG4_INT_EN_TRANS_MASK    ((uint8_t) 0x20)
#define MMA8652_CTRL_REG4_INT_EN_TRANS_SHIFT   ((uint8_t)    5)

#define MMA8652_CTRL_REG4_INT_EN_FIFO_MASK     ((uint8_t) 0x40)
#define MMA8652_CTRL_REG4_INT_EN_FIFO_SHIFT    ((uint8_t)    6)

#define MMA8652_CTRL_REG4_INT_EN_ASLP_MASK     ((uint8_t) 0x80)
#define MMA8652_CTRL_REG4_INT_EN_ASLP_SHIFT    ((uint8_t)    7)


/*
** CTRL_REG4 - Bit field value definitions
*/
#define MMA8652_CTRL_REG4_INT_EN_ASLP_DIS       ((uint8_t) 0x00)  /*  Auto-sleep/wake interrupt disabled                 */
#define MMA8652_CTRL_REG4_INT_EN_ASLP_EN        ((uint8_t) 0x80)  /*  Auto-sleep/wake interrupt Enabled                  */
#define MMA8652_CTRL_REG4_INT_EN_FIFO_DIS       ((uint8_t) 0x00)  /*  FIFO interrupt disabled                            */
#define MMA8652_CTRL_REG4_INT_EN_FIFO_EN        ((uint8_t) 0x40)  /*  FIFO interrupt enabled                             */
#define MMA8652_CTRL_REG4_INT_EN_TRANS_DIS      ((uint8_t) 0x00)  /*  Transient interrupt disabled                       */
#define MMA8652_CTRL_REG4_INT_EN_TRANS_EN       ((uint8_t) 0x20)  /*  Transient interrupt enabled                        */
#define MMA8652_CTRL_REG4_INT_EN_LNDPRT_DIS     ((uint8_t) 0x00)  /*  Orientation (landscape/portrait) interrupt         */
                                                                   /*  disabled                                           */
#define MMA8652_CTRL_REG4_INT_EN_LNDPRT_EN      ((uint8_t) 0x10)  /*  Orientation (landscape/portrait) interrupt enabled */
#define MMA8652_CTRL_REG4_INT_EN_PULSE_DIS      ((uint8_t) 0x00)  /*  Pulse detection interrupt disabled                 */
#define MMA8652_CTRL_REG4_INT_EN_PULSE_EN       ((uint8_t) 0x08)  /*  Pulse detection interrupt enabled                  */
#define MMA8652_CTRL_REG4_INT_EN_FFMT_DIS       ((uint8_t) 0x00)  /*  Freefall/motion interrupt disabled                 */
#define MMA8652_CTRL_REG4_INT_EN_FFMT_EN        ((uint8_t) 0x04)  /*  Freefall/motion interrupt enabled                  */
#define MMA8652_CTRL_REG4_INT_EN_DRDY_DIS       ((uint8_t) 0x00)  /*  Data-ready interrupt disabled                      */
#define MMA8652_CTRL_REG4_INT_EN_DRDY_EN        ((uint8_t) 0x01)  /*  Data-ready interrupt Enabled                       */
/*------------------------------*/



/*--------------------------------
** Register: CTRL_REG5
** Enum: MMA8652_CTRL_REG5
** --
** Offset : 0x2E - Interrupt pin (INT1/INT2) map.
** ------------------------------*/
typedef union {
    struct {
        uint8_t          int_cfg_drdy : 1;
        uint8_t        	   _reserved_ : 1;
        uint8_t          int_cfg_ffmt : 1;
        uint8_t         int_cfg_pulse : 1;
        uint8_t        int_cfg_lndprt : 1;
        uint8_t         int_cfg_trans : 1;
        uint8_t          int_cfg_fifo : 1;
        uint8_t          int_cfg_aslp : 1;
    } b;
    uint8_t w;
} MMA8652_CTRL_REG5_t;


/*
** CTRL_REG5 - Bit field mask definitions
*/
#define MMA8652_CTRL_REG5_INT_CFG_DRDY_MASK     ((uint8_t) 0x01)
#define MMA8652_CTRL_REG5_INT_CFG_DRDY_SHIFT    ((uint8_t)    0)

#define MMA8652_CTRL_REG5_INT_CFG_FFMT_MASK     ((uint8_t) 0x04)
#define MMA8652_CTRL_REG5_INT_CFG_FFMT_SHIFT    ((uint8_t)    2)

#define MMA8652_CTRL_REG5_INT_CFG_PULSE_MASK    ((uint8_t) 0x08)
#define MMA8652_CTRL_REG5_INT_CFG_PULSE_SHIFT   ((uint8_t)    3)

#define MMA8652_CTRL_REG5_INT_CFG_LNDPRT_MASK   ((uint8_t) 0x10)
#define MMA8652_CTRL_REG5_INT_CFG_LNDPRT_SHIFT  ((uint8_t)    4)

#define MMA8652_CTRL_REG5_INT_CFG_TRANS_MASK    ((uint8_t) 0x20)
#define MMA8652_CTRL_REG5_INT_CFG_TRANS_SHIFT   ((uint8_t)    5)

#define MMA8652_CTRL_REG5_INT_CFG_FIFO_MASK     ((uint8_t) 0x40)
#define MMA8652_CTRL_REG5_INT_CFG_FIFO_SHIFT    ((uint8_t)    6)

#define MMA8652_CTRL_REG5_INT_CFG_ASLP_MASK     ((uint8_t) 0x80)
#define MMA8652_CTRL_REG5_INT_CFG_ASLP_SHIFT    ((uint8_t)    7)


/*
** CTRL_REG5 - Bit field value definitions
*/
#define MMA8652_CTRL_REG5_INT_CFG_ASLP_INT2     ((uint8_t) 0x00)  /*  Interrupt is routed to INT2 pin                    */
#define MMA8652_CTRL_REG5_INT_CFG_ASLP_INT1     ((uint8_t) 0x80)  /*  Interrupt is routed to INT1 pin                    */
#define MMA8652_CTRL_REG5_INT_CFG_FIFO_INT2     ((uint8_t) 0x00)  /*  Interrupt is routed to INT2 pin                    */
#define MMA8652_CTRL_REG5_INT_CFG_FIFO_INT1     ((uint8_t) 0x40)  /*  Interrupt is routed to INT1 pin                    */
#define MMA8652_CTRL_REG5_INT_CFG_TRANS_INT2    ((uint8_t) 0x00)  /*  Interrupt is routed to INT2 pin                    */
#define MMA8652_CTRL_REG5_INT_CFG_TRANS_INT1    ((uint8_t) 0x20)  /*  Interrupt is routed to INT1 pin                    */
#define MMA8652_CTRL_REG5_INT_CFG_LNDPRT_INT2   ((uint8_t) 0x00)  /*  Interrupt is routed to INT2 pin                    */
#define MMA8652_CTRL_REG5_INT_CFG_LNDPRT_INT1   ((uint8_t) 0x10)  /*  Interrupt is routed to INT1 pin                    */
#define MMA8652_CTRL_REG5_INT_CFG_PULSE_INT2    ((uint8_t) 0x00)  /*  Interrupt is routed to INT2 pin                    */
#define MMA8652_CTRL_REG5_INT_CFG_PULSE_INT1    ((uint8_t) 0x08)  /*  Interrupt is routed to INT1 pin                    */
#define MMA8652_CTRL_REG5_INT_CFG_FFMT_INT2     ((uint8_t) 0x00)  /*  Interrupt is routed to INT2 pin                    */
#define MMA8652_CTRL_REG5_INT_CFG_FFMT_INT1     ((uint8_t) 0x04)  /*  Interrupt is routed to INT1 pin                    */
#define MMA8652_CTRL_REG5_INT_CFG_DRDY_INT2     ((uint8_t) 0x00)  /*  Interrupt is routed to INT2 pin                    */
#define MMA8652_CTRL_REG5_INT_CFG_DRDY_INT1     ((uint8_t) 0x01)  /*  Interrupt is routed to INT1 pin                    */
/*------------------------------*/



/*--------------------------------
** Register: OFF_X
** Enum: MMA8652_OFF_X
** --
** Offset : 0x2F - X-axis accelerometer offset adjust.
** ------------------------------*/
typedef uint8_t MMA8652_OFF_X_t;


/*--------------------------------
** Register: OFF_Y
** Enum: MMA8652_OFF_Y
** --
** Offset : 0x30 - Y-axis accelerometer offset adjust.
** ------------------------------*/
typedef uint8_t MMA8652_OFF_Y_t;


/*--------------------------------
** Register: OFF_Z
** Enum: MMA8652_OFF_Z
** --
** Offset : 0x31 - Z-axis accelerometer offset adjust.
** ------------------------------*/
typedef uint8_t MMA8652_OFF_Z_t;




/*------------------------------*/


#endif /* MMA8652_REGDEF_H_ */
