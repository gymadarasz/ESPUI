// #include "env.h"

// void env_init() {
//     env_timer_start = std::chrono::high_resolution_clock::now();
// }

// unsigned long long micros() {
//     auto elapsed = std::chrono::high_resolution_clock::now() - env_timer_start;
//     return std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
// }

// unsigned long long millis() {
//     return (micros() / 1000ULL);
// }