#ifndef TRIGGER_H
#define TRIGGER_H

/**
 * Sets the trigger divider.
 * n: trigger divider, divides trigger by n, i.e. trigger once for every
 *    n external triggers
 */
void trigger_set(uint8_t n);

/**
 * Resets the trigger. Call this before starting a scan.
 */
void trigger_reset(void);

/**
 * Initialises the trigger handling mechanism, which uses counter0
 * connected to T0 to count external triggers.
 *
 */
void trigger_init(void);

#endif
