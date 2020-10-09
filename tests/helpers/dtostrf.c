
#include <stdio.h>
#include <stdint.h>
#include <math.h>

char *dtostrf (double val, signed char width, unsigned char prec, char *sout) {
  uint32_t iPart = (uint32_t)val;

  if (prec > 0) {
    uint32_t dPart = (uint32_t)((val - (double)iPart) * pow(10, prec));
    sprintf(sout, "%d.%d", iPart, dPart);
  }
  else {
    sprintf(sout, "%d", iPart);
  }

  return sout;
}