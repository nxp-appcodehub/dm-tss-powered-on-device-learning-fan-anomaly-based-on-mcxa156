/*
 * Copyright 2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file bmi323.h
 * @brief The bmi323.h file describes the Bosch BMI323 registers and bit fields.
 *
 * The BMI323 is the IMU fitted on the MikroElektronika 6DOF IMU 20 Click board.
 * Unlike the FXLS8974, its register file is organised as 16-bit little-endian words:
 * a read returns the LSB first, then the MSB, and a write must supply both bytes.
 * Register reads are also preceded by dummy bytes on the wire
 * (see ::BMI323_I2C_DUMMY_BYTES / ::BMI323_SPI_DUMMY_BYTES).
 *
 * Provenance of the values below
 * ------------------------------
 * CONFIRMED against the Bosch BMI323 SensorAPI (bmi3_defs.h / bmi3.c):
 *   - dummy byte counts (2 on I2C, 1 on SPI) and the 0x80 SPI read mask
 *   - 16-bit little-endian register layout
 *   - accelerometer sensitivity (8192 LSB/g at +/-4 g)
 *   - CMD soft reset value 0xDEAF (SensorAPI waits 1500 us afterwards)
 *   - STATUS flag bit positions
 *
 * NOT independently confirmed - derived by reading the vendor click-board driver.
 * Verify against BST-BMI323-DS000 before relying on these:
 *   - ::BMI323_FIFO_ACC_FRAME_WORDS and the accel-only FIFO frame layout
 *   - FIFO_WATERMARK units (this driver programs the value in 16-bit words)
 *   - ::BMI323_FIFO_INVALID_SAMPLE placeholder value
 *   - ACC_CONF / GYR_CONF field encodings, INT_MAP2 and IO_INT_CTRL bits
 *   - ::BMI323_CHIP_ID_VALUE (the SensorAPI defines no expected-ID constant)
 */

#ifndef BMI323_H_
#define BMI323_H_

#include <stdint.h>

/*******************************************************************************
 * Bus characteristics
 ******************************************************************************/
/*! @brief Dummy bytes the device emits before real data on an I2C register read.
 *
 *  Confirmed: Bosch SensorAPI bmi3.c sets dev->dummy_byte = 2 for the I2C/I3C
 *  interface, and bmi3_get_regs() reads len + dummy_byte bytes then skips them. */
#define BMI323_I2C_DUMMY_BYTES (2U)

/*! @brief Dummy bytes the device emits before real data on a SPI register read.
 *
 *  Confirmed: Bosch SensorAPI bmi3.c sets dev->dummy_byte = 1 for SPI. */
#define BMI323_SPI_DUMMY_BYTES (1U)

/*! @brief Bytes per register. All BMI323 registers are 16-bit. */
#define BMI323_REG_SIZE (2U)

/*! @brief Read bit OR-ed into the register address for SPI reads.
 *
 *  Confirmed: BMI3_SPI_RD_MASK = 0x80 in the Bosch SensorAPI. */
#define BMI323_SPI_READ_MASK (0x80U)

/*******************************************************************************
 * Register addresses
 ******************************************************************************/
#define BMI323_CHIP_ID            (0x00U)
#define BMI323_ERR_REG            (0x01U)
#define BMI323_STATUS             (0x02U)
#define BMI323_ACC_DATA_X         (0x03U)
#define BMI323_ACC_DATA_Y         (0x04U)
#define BMI323_ACC_DATA_Z         (0x05U)
#define BMI323_GYR_DATA_X         (0x06U)
#define BMI323_GYR_DATA_Y         (0x07U)
#define BMI323_GYR_DATA_Z         (0x08U)
#define BMI323_TEMP_DATA          (0x09U)
#define BMI323_SENSOR_TIME_0      (0x0AU)
#define BMI323_SENSOR_TIME_1      (0x0BU)
#define BMI323_SAT_FLAGS          (0x0CU)
#define BMI323_INT_STATUS_INT1    (0x0DU)
#define BMI323_INT_STATUS_INT2    (0x0EU)
#define BMI323_INT_STATUS_IBI     (0x0FU)
#define BMI323_FEATURE_IO0        (0x10U)
#define BMI323_FEATURE_IO1        (0x11U)
#define BMI323_FEATURE_IO2        (0x12U)
#define BMI323_FEATURE_IO3        (0x13U)
#define BMI323_FEATURE_IO_STATUS  (0x14U)
#define BMI323_FIFO_FILL_LEVEL    (0x15U)
#define BMI323_FIFO_DATA          (0x16U)
#define BMI323_ACC_CONF           (0x20U)
#define BMI323_GYR_CONF           (0x21U)
#define BMI323_FIFO_WATERMARK     (0x35U)
#define BMI323_FIFO_CONF          (0x36U)
#define BMI323_FIFO_CTRL          (0x37U)
#define BMI323_IO_INT_CTRL        (0x38U)
#define BMI323_INT_CONF           (0x39U)
#define BMI323_INT_MAP1           (0x3AU)
#define BMI323_INT_MAP2           (0x3BU)
#define BMI323_FEATURE_CTRL       (0x40U)
#define BMI323_IO_PDN_CTRL        (0x4FU)
#define BMI323_IO_SPI_IF          (0x50U)
#define BMI323_IO_PAD_STRENGTH    (0x51U)
#define BMI323_IO_I2C_IF          (0x52U)
#define BMI323_IO_ODR_DEVIATION   (0x53U)
#define BMI323_CMD                (0x7EU)
#define BMI323_CFG_RES            (0x7FU)

/*******************************************************************************
 * CHIP_ID (0x00)
 ******************************************************************************/
/*! @brief Chip ID of the BMI323, found in the low byte of CHIP_ID.
 *
 *  NOT confirmed from the Bosch SensorAPI: it reads the ID into dev->chip_id without
 *  comparing it against any expected constant, so no #define exists there to cite.
 *  Verify against BST-BMI323-DS000. Note the upper nibble of the high byte is a
 *  revision ID, so always mask with ::BMI323_CHIP_ID_MASK before comparing. */
#define BMI323_CHIP_ID_VALUE (0x43U)
#define BMI323_CHIP_ID_MASK  (0x00FFU)

/*******************************************************************************
 * ERR_REG (0x01)
 ******************************************************************************/
#define BMI323_ERR_FATAL_ERR          (0x0001U)
#define BMI323_ERR_FEAT_ENG_OVRLD     (0x0004U)
#define BMI323_ERR_FEAT_ENG_WD        (0x0010U)
#define BMI323_ERR_ACC_CONF_ERR       (0x0020U)
#define BMI323_ERR_GYR_CONF_ERR       (0x0040U)

/*******************************************************************************
 * STATUS (0x02)
 ******************************************************************************/
#define BMI323_STATUS_POR_DETECTED (0x0001U)
#define BMI323_STATUS_DRDY_TEMP    (0x0020U)
#define BMI323_STATUS_DRDY_GYR     (0x0040U)
#define BMI323_STATUS_DRDY_ACC     (0x0080U)

/*******************************************************************************
 * ACC_CONF (0x20)
 ******************************************************************************/
/*! acc_odr, bits [3:0] */
#define BMI323_ACC_CONF_ODR_MASK        (0x000FU)
#define BMI323_ACC_CONF_ODR_0P78125HZ   (0x0001U)
#define BMI323_ACC_CONF_ODR_1P5625HZ    (0x0002U)
#define BMI323_ACC_CONF_ODR_3P125HZ     (0x0003U)
#define BMI323_ACC_CONF_ODR_6P25HZ      (0x0004U)
#define BMI323_ACC_CONF_ODR_12P5HZ      (0x0005U)
#define BMI323_ACC_CONF_ODR_25HZ        (0x0006U)
#define BMI323_ACC_CONF_ODR_50HZ        (0x0007U)
#define BMI323_ACC_CONF_ODR_100HZ       (0x0008U)
#define BMI323_ACC_CONF_ODR_200HZ       (0x0009U)
#define BMI323_ACC_CONF_ODR_400HZ       (0x000AU)
#define BMI323_ACC_CONF_ODR_800HZ       (0x000BU)
#define BMI323_ACC_CONF_ODR_1P6KHZ      (0x000CU)
#define BMI323_ACC_CONF_ODR_3P2KHZ      (0x000DU)
#define BMI323_ACC_CONF_ODR_6P4KHZ      (0x000EU)

/*! acc_range, bits [6:4] */
#define BMI323_ACC_CONF_RANGE_MASK (0x0070U)
#define BMI323_ACC_CONF_RANGE_2G   (0x0000U)
#define BMI323_ACC_CONF_RANGE_4G   (0x0010U)
#define BMI323_ACC_CONF_RANGE_8G   (0x0020U)
#define BMI323_ACC_CONF_RANGE_16G  (0x0030U)

/*! acc_bw, bit [7] : 3dB cut-off as a fraction of the ODR */
#define BMI323_ACC_CONF_BW_MASK  (0x0080U)
#define BMI323_ACC_CONF_BW_ODR_2 (0x0000U) /*!< ODR/2 */
#define BMI323_ACC_CONF_BW_ODR_4 (0x0080U) /*!< ODR/4 */

/*! acc_avg_num, bits [10:8] : number of samples averaged per output */
#define BMI323_ACC_CONF_AVG_MASK (0x0700U)
#define BMI323_ACC_CONF_AVG_1    (0x0000U)
#define BMI323_ACC_CONF_AVG_2    (0x0100U)
#define BMI323_ACC_CONF_AVG_4    (0x0200U)
#define BMI323_ACC_CONF_AVG_8    (0x0300U)
#define BMI323_ACC_CONF_AVG_16   (0x0400U)
#define BMI323_ACC_CONF_AVG_32   (0x0500U)
#define BMI323_ACC_CONF_AVG_64   (0x0600U)

/*! acc_mode, bits [14:12] */
#define BMI323_ACC_CONF_MODE_MASK         (0x7000U)
#define BMI323_ACC_CONF_MODE_DISABLE      (0x0000U)
#define BMI323_ACC_CONF_MODE_DUTY_CYCLING (0x3000U)
#define BMI323_ACC_CONF_MODE_CONTINUOUS   (0x4000U)
#define BMI323_ACC_CONF_MODE_HIGH_PERF    (0x7000U)

/*******************************************************************************
 * GYR_CONF (0x21) - same field layout as ACC_CONF
 ******************************************************************************/
#define BMI323_GYR_CONF_ODR_MASK          (0x000FU)
#define BMI323_GYR_CONF_ODR_100HZ         (0x0008U)
#define BMI323_GYR_CONF_ODR_200HZ         (0x0009U)
#define BMI323_GYR_CONF_ODR_400HZ         (0x000AU)
#define BMI323_GYR_CONF_RANGE_MASK        (0x0070U)
#define BMI323_GYR_CONF_RANGE_125DPS      (0x0000U)
#define BMI323_GYR_CONF_RANGE_250DPS      (0x0010U)
#define BMI323_GYR_CONF_RANGE_500DPS      (0x0020U)
#define BMI323_GYR_CONF_RANGE_1000DPS     (0x0030U)
#define BMI323_GYR_CONF_RANGE_2000DPS     (0x0040U)
#define BMI323_GYR_CONF_BW_MASK           (0x0080U)
#define BMI323_GYR_CONF_BW_ODR_2          (0x0000U)
#define BMI323_GYR_CONF_BW_ODR_4          (0x0080U)
#define BMI323_GYR_CONF_AVG_MASK          (0x0700U)
#define BMI323_GYR_CONF_AVG_1             (0x0000U)
#define BMI323_GYR_CONF_MODE_MASK         (0x7000U)
#define BMI323_GYR_CONF_MODE_DISABLE      (0x0000U)
#define BMI323_GYR_CONF_MODE_DUTY_CYCLING (0x3000U)
#define BMI323_GYR_CONF_MODE_CONTINUOUS   (0x4000U)
#define BMI323_GYR_CONF_MODE_HIGH_PERF    (0x7000U)

/*******************************************************************************
 * FIFO_FILL_LEVEL (0x15)
 ******************************************************************************/
/*! @brief Fill level, in 16-bit words, bits [10:0]. */
#define BMI323_FIFO_FILL_LEVEL_MASK (0x07FFU)

/*! @brief Total FIFO capacity in 16-bit words (2048 bytes). */
#define BMI323_FIFO_WORDS_MAX (1024U)

/*******************************************************************************
 * FIFO_CONF (0x36)
 ******************************************************************************/
#define BMI323_FIFO_CONF_STOP_ON_FULL (0x0001U)
#define BMI323_FIFO_CONF_TIME_EN      (0x0100U)
#define BMI323_FIFO_CONF_ACC_EN       (0x0200U)
#define BMI323_FIFO_CONF_GYR_EN       (0x0400U)
#define BMI323_FIFO_CONF_TEMP_EN      (0x0800U)
#define BMI323_FIFO_CONF_ALL_EN       (0x0F00U)

/*******************************************************************************
 * FIFO_CTRL (0x37)
 ******************************************************************************/
#define BMI323_FIFO_CTRL_FLUSH (0x0001U)

/*******************************************************************************
 * FIFO frame layout
 *
 * UNVERIFIED - derived from the vendor click-board driver, not the datasheet.
 * If the frame stride or the placeholder value is wrong, samples are silently
 * mis-decoded rather than failing loudly. Check BST-BMI323-DS000 before trusting
 * acquisition on hardware.
 ******************************************************************************/
/*! @brief Words per FIFO frame when only the accelerometer is stored (X, Y, Z). */
#define BMI323_FIFO_ACC_FRAME_WORDS (3U)

/*! @brief Bytes per FIFO frame when only the accelerometer is stored. */
#define BMI323_FIFO_ACC_FRAME_BYTES (BMI323_FIFO_ACC_FRAME_WORDS * BMI323_REG_SIZE)

/*! @brief Placeholder word the FIFO returns for a sample that was not available. */
#define BMI323_FIFO_INVALID_SAMPLE ((int16_t)0x8000)

/*******************************************************************************
 * IO_INT_CTRL (0x38)
 ******************************************************************************/
#define BMI323_IO_INT_CTRL_INT1_LVL_LOW    (0x0000U)
#define BMI323_IO_INT_CTRL_INT1_LVL_HIGH   (0x0001U)
#define BMI323_IO_INT_CTRL_INT1_PUSH_PULL  (0x0000U)
#define BMI323_IO_INT_CTRL_INT1_OPEN_DRAIN (0x0002U)
#define BMI323_IO_INT_CTRL_INT1_OUTPUT_EN  (0x0004U)
#define BMI323_IO_INT_CTRL_INT2_LVL_LOW    (0x0000U)
#define BMI323_IO_INT_CTRL_INT2_LVL_HIGH   (0x0100U)
#define BMI323_IO_INT_CTRL_INT2_PUSH_PULL  (0x0000U)
#define BMI323_IO_INT_CTRL_INT2_OPEN_DRAIN (0x0200U)
#define BMI323_IO_INT_CTRL_INT2_OUTPUT_EN  (0x0400U)

/*******************************************************************************
 * INT_CONF (0x39)
 ******************************************************************************/
#define BMI323_INT_CONF_LATCH_DIS (0x0000U)
#define BMI323_INT_CONF_LATCH_EN  (0x0001U)

/*******************************************************************************
 * INT_MAP2 (0x3B) - data/FIFO interrupt routing
 ******************************************************************************/
#define BMI323_INT_MAP2_TEMP_DRDY_DIS  (0x0000U)
#define BMI323_INT_MAP2_TEMP_DRDY_INT1 (0x0040U)
#define BMI323_INT_MAP2_TEMP_DRDY_INT2 (0x0080U)
#define BMI323_INT_MAP2_GYR_DRDY_DIS   (0x0000U)
#define BMI323_INT_MAP2_GYR_DRDY_INT1  (0x0100U)
#define BMI323_INT_MAP2_GYR_DRDY_INT2  (0x0200U)
#define BMI323_INT_MAP2_ACC_DRDY_DIS   (0x0000U)
#define BMI323_INT_MAP2_ACC_DRDY_INT1  (0x0400U)
#define BMI323_INT_MAP2_ACC_DRDY_INT2  (0x0800U)
#define BMI323_INT_MAP2_FWM_DIS        (0x0000U)
#define BMI323_INT_MAP2_FWM_INT1       (0x1000U)
#define BMI323_INT_MAP2_FWM_INT2       (0x2000U)
#define BMI323_INT_MAP2_FFULL_DIS      (0x0000U)
#define BMI323_INT_MAP2_FFULL_INT1     (0x4000U)
#define BMI323_INT_MAP2_FFULL_INT2     (0x8000U)

/*******************************************************************************
 * INT_STATUS_INT1 (0x0D) / INT_STATUS_INT2 (0x0E)
 ******************************************************************************/
#define BMI323_INT_STATUS_ERR_STATUS (0x0400U)
#define BMI323_INT_STATUS_TEMP_DRDY  (0x0800U)
#define BMI323_INT_STATUS_GYR_DRDY   (0x1000U)
#define BMI323_INT_STATUS_ACC_DRDY   (0x2000U)
#define BMI323_INT_STATUS_FWM        (0x4000U)
#define BMI323_INT_STATUS_FFULL      (0x8000U)

/*******************************************************************************
 * CMD (0x7E)
 ******************************************************************************/
/*! @brief Soft reset command. */
#define BMI323_CMD_SOFT_RESET (0xDEAFU)

/*! @brief Milliseconds to wait after a soft reset before talking to the device again.
 *
 *  The Bosch SensorAPI waits BMI3_SOFT_RESET_DELAY = 1500 us (1.5 ms). This driver
 *  rounds up to a whole millisecond count because BOARD_DELAY_ms() is ms-granular;
 *  waiting longer than required is harmless. */
#define BMI323_TBOOT_MAX (10U)

/*******************************************************************************
 * Temperature conversion (TEMP_DATA, 0x09)
 ******************************************************************************/
#define BMI323_TEMP_DIVIDER (512)
#define BMI323_TEMP_OFFSET  (23)

/*******************************************************************************
 * Accelerometer sensitivity, in LSB/g, at 16-bit resolution
 *
 * Confirmed against the Bosch SensorAPI: lsb_per_g = 2^resolution / (2 * range),
 * matching BMI3_ACC_FOC_{2,4,8,16}G_REF = 16384 / 8192 / 4096 / 2048.
 ******************************************************************************/
#define BMI323_ACC_SENS_2G  (16384)
#define BMI323_ACC_SENS_4G  (8192)
#define BMI323_ACC_SENS_8G  (4096)
#define BMI323_ACC_SENS_16G (2048)

#endif /* BMI323_H_ */
