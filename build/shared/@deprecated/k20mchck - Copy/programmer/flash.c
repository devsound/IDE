#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "adiv5.h"
#include "kinetis.h"
#include "utilities.h"

int main(int argc, char **argv) {
  printf("Attaching debugger... ");
  adiv5_init();
  if(argc > 1 && !strcmp(argv[1], "--mass-erase")) {
    kinetisbase_init();
    printf("done.\nMass erasing chip... ");
    kinetisbase_mass_erase();
    printf("done.\n");
  } else {
    kinetis_init();
    printf("done.\n");
    
    if(argc > 3) {
      size_t total;
      char *firmware = blob(argv[2], &total);
      uint32_t address = atol(argv[3]);

      printf("Programming %ul bytes of firmware to address 0x%02x...\n", total, address);
      kinetis_halt_core();
      kinetis_program(address, firmware, total);
      printf("done.\n");
      
      free(firmware); // release memory
    }
  }
  
  printf("Resetting... ");
  kinetis_reset_system();
  kinetis_disable_debug();
  kinetis_continue();
  printf("done.\n");
}
