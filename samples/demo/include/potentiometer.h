#ifndef POTENTIOMETER_H
#define POTENTIOMETER_H

#include <stdint.h>

/* Configure ADC channel resources used by the potentiometer input. */
int potentiometer_init(void);
/* Read one raw ADC sample from the potentiometer channel. */
int potentiometer_read_raw(uint16_t *raw_value);

#endif /* POTENTIOMETER_H */
