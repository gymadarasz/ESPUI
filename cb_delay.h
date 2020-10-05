#ifndef CB_DELAY_H
#define CB_DELAY_H

typedef void (*cb_delay_func_t)(void);

void cb_delay(unsigned long ms, cb_delay_func_t callback = nullptr);

#endif // CB_DELAY_H