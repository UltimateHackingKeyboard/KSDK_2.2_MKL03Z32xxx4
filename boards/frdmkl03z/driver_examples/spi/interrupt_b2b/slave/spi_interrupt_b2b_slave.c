/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "fsl_spi.h"
#include "board.h"
#include "fsl_debug_console.h"

#include "pin_mux.h"
#include "clock_config.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define EXAMPLE_SPI_SLAVE SPI0
#define EXAMPLE_SPI_SLAVE_IRQ SPI0_IRQn
#define SPI_SLAVE_IRQHandler SPI0_IRQHandler

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
#define BUFFER_SIZE (64)
static uint8_t srcBuff[BUFFER_SIZE];
static uint8_t destBuff[BUFFER_SIZE];
static uint32_t txIndex = BUFFER_SIZE;
static uint32_t rxIndex = BUFFER_SIZE;
static volatile bool slaveFinished = false;

/*******************************************************************************
 * Code
 ******************************************************************************/

void SPI_SLAVE_IRQHandler(void)
{
    if ((SPI_GetStatusFlags(EXAMPLE_SPI_SLAVE) & kSPI_RxBufferFullFlag) && (rxIndex > 0U))
    {
        destBuff[BUFFER_SIZE - rxIndex] = SPI_ReadData(EXAMPLE_SPI_SLAVE);
        rxIndex--;
    }

    if ((SPI_GetStatusFlags(EXAMPLE_SPI_SLAVE) & kSPI_TxBufferEmptyFlag) && (txIndex > 0U))
    {
        SPI_WriteData(EXAMPLE_SPI_SLAVE, (uint16_t)(srcBuff[BUFFER_SIZE - txIndex]));
        txIndex--;
    }

    if ((rxIndex == 0U) && (txIndex == 0U))
    {
        slaveFinished = true;
        SPI_DisableInterrupts(EXAMPLE_SPI_SLAVE, kSPI_RxFullAndModfInterruptEnable | kSPI_TxEmptyInterruptEnable);
    }
}

int main(void)
{
    spi_slave_config_t slaveConfig = {0};
    uint32_t i = 0U;
    uint32_t err = 0U;

    /* Init the boards */
    BOARD_InitPins();
    BOARD_BootClockRUN();
    BOARD_InitDebugConsole();

    PRINTF("\r\nSPI board to board interrupt slave example started!\r\n");

    /* Init SPI slave */
    /*
     * slaveConfig->polarity = kSPI_ClockPolarityActiveHigh;
     * slaveConfig->phase = kSPI_ClockPhaseFirstEdge;
     * slaveConfig->direction = kSPI_MsbFirst;
     * slaveConfig->enableStopInWaitMode = false;
     * slaveConfig->dataMode = kSPI_8BitMode;
     * slaveConfig->txWatermark = kSPI_TxFifoOneHalfEmpty;
     * slaveConfig->rxWatermark = kSPI_RxFifoOneHalfFull;
     */
    SPI_SlaveGetDefaultConfig(&slaveConfig);
    SPI_SlaveInit(EXAMPLE_SPI_SLAVE, &slaveConfig);

/* Disable FIFO */
#if defined(FSL_FEATURE_SPI_HAS_FIFO) && (FSL_FEATURE_SPI_HAS_FIFO)
    SPI_EnableFIFO(EXAMPLE_SPI_SLAVE, false);
#endif /* FSL_FEATURE_SPI_HAS_FIFO */

    /* Init source buffer */
    for (i = 0U; i < BUFFER_SIZE; i++)
    {
        srcBuff[i] = i;
    }

    /* Enable interrupt, first enable slave and then master. */
    EnableIRQ(EXAMPLE_SPI_SLAVE_IRQ);
    SPI_EnableInterrupts(EXAMPLE_SPI_SLAVE, kSPI_RxFullAndModfInterruptEnable | kSPI_TxEmptyInterruptEnable);

    while (slaveFinished != true)
    {
    }

    /* Check the data received */
    for (i = 0U; i < BUFFER_SIZE; i++)
    {
        if (destBuff[i] != srcBuff[i])
        {
            PRINTF("\r\nThe %d data is wrong, the data received is %d \r\n", i, destBuff[i]);
            err++;
        }
    }
    if (err == 0U)
    {
        PRINTF("\r\nSPI transfer finished!\r\n");
    }

    while (1)
    {
    }
}
