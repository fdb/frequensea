// NDBX Radio Frequency functions, based on HackRF

#ifndef NRF_H
#define NRF_H

void nrf_start(double freq_mhz);
void nrf_set_freq(double freq_mhz);
void nrf_read(uint8_t *dst);
void nrf_stop();

#endif // NRF_H
