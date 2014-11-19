#ifndef PIT_H
#define PIT_H

#include <board.h>

void pit_init(unsigned int period, unsigned int pit_frequency);
void pit_enable(void);
void pit_enable_it(void);
void pit_disable_it(void);

#endif /* PIT_H */
