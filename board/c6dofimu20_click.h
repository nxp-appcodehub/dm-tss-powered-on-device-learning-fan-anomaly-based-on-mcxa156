/*
 * Copyright 2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*! File: c6dofimu20_click.h
 * @brief The c6dofimu20_click.h file declares the board information for the
 *        6DOF IMU 20 Click (Bosch BMI323) expansion board on a mikroBUS socket.
 */

#ifndef _C6DOFIMU20_CLICK_H_
#define _C6DOFIMU20_CLICK_H_

/* The shield name */
#define SHIELD_NAME "6DOFIMU20-CLICK"

/* BMI323 I2C slave address.
 * The click board ties the ASDX/SDO pin low by default, selecting 0x68.
 * Move the ADDR SEL jumper to VCC to use 0x69 instead. */
#define BMI323_I2C_ADDR 0x68

#endif /* _C6DOFIMU20_CLICK_H_ */
