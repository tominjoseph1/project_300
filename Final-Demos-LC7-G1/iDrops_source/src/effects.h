#ifndef EFFECTS
#define EFFECTS
#include <math.h>

void flipSamples(float *arr, int n)
{
  float *end;
  float tmp;
  for (end = arr + n; end > arr; end--, arr++) {
    tmp = *end;
    *end = *arr;
    *arr = tmp;
  }
}

void invertSamples(float *arr, int n)
{
  float *end;
  for (end = arr + n; end >= arr; end--)
    *end = -*end;
}

void bitCrush(float *arr, int n, float amount)
{
  float *end;
 
  for (end = arr + n; end >= arr; end--)
    *end = round(pow(10, amount) * *end) / amount;
}

#endif
