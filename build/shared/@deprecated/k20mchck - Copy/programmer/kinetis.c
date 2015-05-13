#include <stddef.h>
#include <stdint.h>

void kinetisbase_init() {
}

void kinetisbase_mass_erase() {
}

void kinetis_init() {
  kinetisbase_init();
}

void kinetis_halt_core() {
}

void kinetis_program(uint32_t address, void * firmware, size_t total) {
}

void kinetis_reset_system() {
}

void kinetis_disable_debug() {
}

void kinetis_continue() {
}
