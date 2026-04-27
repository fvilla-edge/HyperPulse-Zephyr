#include <zephyr/drivers/adc.h>
#include <hal/nrf_saadc.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "potentiometer.h"

LOG_MODULE_REGISTER(potentiometer, LOG_LEVEL_INF);

#define POT_NODE DT_PATH(zephyr_user)

static bool is_initialised;
static uint16_t sample_buffer;
/* Reused read sequence for single-sample reads on one ADC channel. */
static struct adc_sequence sequence;
#if DT_NODE_EXISTS(POT_NODE) && DT_NODE_HAS_PROP(POT_NODE, io_channels)
/* ADC channel comes from zephyr,user io-channels in devicetree overlay. */
static const struct adc_dt_spec pot_adc = ADC_DT_SPEC_GET_BY_IDX(POT_NODE, 0);
#endif
/* Channel parameters selected for A0 (P0.13 / SAADC AIN0) on this board. */
static struct adc_channel_cfg channel_cfg = {
	.gain = ADC_GAIN_1_6,
	.reference = ADC_REF_INTERNAL,
	.acquisition_time = ADC_ACQ_TIME_DEFAULT,
	.channel_id = 0,
#if defined(CONFIG_ADC_CONFIGURABLE_INPUTS)
	.input_positive = NRF_SAADC_INPUT_AIN0,
#endif
};

int potentiometer_init(void)
{
#if DT_NODE_EXISTS(POT_NODE) && DT_NODE_HAS_PROP(POT_NODE, io_channels)
	if (!adc_is_ready_dt(&pot_adc)) {
		LOG_ERR("Potentiometer ADC is not ready");
		return -ENODEV;
	}

	channel_cfg.channel_id = pot_adc.channel_id;
	/* 12-bit raw sample in sample_buffer; channel mask is one bit only. */
	sequence = (struct adc_sequence){
		.channels = BIT(channel_cfg.channel_id),
		.buffer = &sample_buffer,
		.buffer_size = sizeof(sample_buffer),
		.resolution = 12,
	};

	int err = adc_channel_setup(pot_adc.dev, &channel_cfg);
	if (err != 0) {
		LOG_ERR("Failed to setup potentiometer ADC channel (err: %d)", err);
		return err;
	}

	is_initialised = true;
	return 0;
#else
	LOG_ERR("Potentiometer ADC DT binding is missing");
	return -ENODEV;
#endif
}

int potentiometer_read_raw(uint16_t *raw_value)
{
#if DT_NODE_EXISTS(POT_NODE) && DT_NODE_HAS_PROP(POT_NODE, io_channels)
	if (raw_value == NULL) {
		return -EINVAL;
	}

	if (!is_initialised) {
		return -EACCES;
	}

	int err = adc_read_dt(&pot_adc, &sequence);
	if (err != 0) {
		return err;
	}

	*raw_value = sample_buffer;
	return 0;
#else
	ARG_UNUSED(raw_value);
	return -ENODEV;
#endif
}
