#include "utilities.h"
#include <stdlib.h>
#include <stdio.h>

// Reads a whole file and returns a pointer to the data chunk.
// Needs to be free()'d.
void *blob(char *filename, size_t *size) {
  void *data;
  FILE *f;
  f = fopen(filename, "rb");
  if(f) {
    fseek(f, 0, SEEK_END);
    *size = ftell(f);
    fseek(f, 0, 0);
    data = malloc(*size);
    if(data) fread(data, 1, *size, f);
    fclose(f);
  }
  return data;
}
