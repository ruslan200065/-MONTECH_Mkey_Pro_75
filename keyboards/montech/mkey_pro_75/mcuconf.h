/* Copyright 2026 @ MONTECH community port
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once
#include_next <mcuconf.h>

#undef WB32_I2C_USE_I2C1
#define WB32_I2C_USE_I2C1 TRUE

#undef WB32_SPI_USE_QSPI
#define WB32_SPI_USE_QSPI TRUE

#undef WB32_LSI_ENABLED
#define WB32_LSI_ENABLED TRUE
