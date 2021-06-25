// Copyright (c) 2020 XMOS LIMITED. This Software is subject to the terms of the
// XMOS Public License: Version 1

#include "FreeRTOS.h"
#include "stream_buffer.h"

#include "app_conf.h"
#include "platform/driver_instances.h"

static StreamBufferHandle_t samples_to_host_stream_buf;
static SemaphoreHandle_t mutex;
static rtos_gpio_port_id_t spi_irq_port;

typedef int16_t samp_t;

#define SPI_CHANNELS 1

void spi_audio_send(rtos_intertile_t *intertile_ctx,
                    size_t frame_count,
                    int32_t (*processed_audio_frame)[2])
{
    samp_t spi_audio_in_frame[appconfAUDIO_PIPELINE_FRAME_ADVANCE][SPI_CHANNELS];


    const int src_32_shift = 32 - 8 * sizeof(samp_t);
    const int src_32_offset = sizeof(samp_t) == 1 ? 128 : 0;

    xassert(frame_count == appconfAUDIO_PIPELINE_FRAME_ADVANCE);

    for (int i = 0; i < appconfAUDIO_PIPELINE_FRAME_ADVANCE; i++) {
        spi_audio_in_frame[i][0] = src_32_offset + (processed_audio_frame[i][0] >> src_32_shift);
#if SPI_CHANNELS > 1
        spi_audio_in_frame[i][1] = src_32_offset + (processed_audio_frame[i][1] >> src_32_shift);
#endif
    }

    rtos_intertile_tx(intertile_ctx,
                      appconfSPI_AUDIO_PORT,
                      spi_audio_in_frame,
                      sizeof(spi_audio_in_frame));
}

static void spi_audio_in_task(void *arg)
{
    for (;;) {
        samp_t spi_audio_in_frame[appconfAUDIO_PIPELINE_FRAME_ADVANCE][SPI_CHANNELS];
        size_t frame_length;

        frame_length = rtos_intertile_rx_len(
                intertile_ctx,
                appconfSPI_AUDIO_PORT,
                portMAX_DELAY);

        xassert(frame_length == sizeof(spi_audio_in_frame));

        rtos_intertile_rx_data(
                intertile_ctx,
                spi_audio_in_frame,
                frame_length);

        xSemaphoreTake(mutex, portMAX_DELAY);
        if (xStreamBufferSend(samples_to_host_stream_buf, spi_audio_in_frame, sizeof(spi_audio_in_frame), 0) == sizeof(spi_audio_in_frame)) {
            rtos_gpio_port_out(gpio_ctx_t0, spi_irq_port, 1);
        } else {
//            rtos_printf("lost VFE output samples\n");
        }

        xSemaphoreGive(mutex);
    }
}

RTOS_SPI_SLAVE_CALLBACK_ATTR
void spi_slave_start_cb(rtos_spi_slave_t *ctx, void *app_data)
{
    static samp_t tx_buf[appconfAUDIO_PIPELINE_FRAME_ADVANCE][SPI_CHANNELS];

    mutex =  xSemaphoreCreateMutex();
    samples_to_host_stream_buf = xStreamBufferCreate(16 * appconfAUDIO_PIPELINE_FRAME_ADVANCE * SPI_CHANNELS * sizeof(samp_t), 0);

    spi_irq_port = rtos_gpio_port(XS1_PORT_1D);
    rtos_gpio_port_enable(gpio_ctx_t0, spi_irq_port);
    rtos_gpio_port_out(gpio_ctx_t0, spi_irq_port, 0);

    xTaskCreate((TaskFunction_t) spi_audio_in_task, "spi_audio_in_task", portTASK_STACK_DEPTH(spi_audio_in_task), NULL, 16, NULL);

    spi_slave_xfer_prepare(ctx, NULL, 0, tx_buf, sizeof(tx_buf));
}

RTOS_SPI_SLAVE_CALLBACK_ATTR
void spi_slave_xfer_done_cb(rtos_spi_slave_t *ctx, void *app_data)
{
    uint8_t *tx_buf;
    uint8_t *rx_buf;
    size_t rx_len;
    size_t tx_len;

    if (spi_slave_xfer_complete(ctx, &rx_buf, &rx_len, &tx_buf, &tx_len, 0) == 0) {

        xSemaphoreTake(mutex, portMAX_DELAY);

        if (xStreamBufferIsFull(samples_to_host_stream_buf)) {
            xStreamBufferReset(samples_to_host_stream_buf);
        }

        if (xStreamBufferReceive(samples_to_host_stream_buf, tx_buf, tx_len, 0) != tx_len) {
            rtos_printf("SPI audio buffer empty, will send zeros\n");
            memset(tx_buf, 0, tx_len);
        }

        if (xStreamBufferBytesAvailable(samples_to_host_stream_buf) > 0) {
            rtos_gpio_port_out(gpio_ctx_t0, spi_irq_port, 1);
        } else {
            rtos_printf("SPI audio buffer drained\n");
            rtos_gpio_port_out(gpio_ctx_t0, spi_irq_port, 0);
        }

        xSemaphoreGive(mutex);
    }
}