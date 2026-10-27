#include <stdio.h>

int sum_next(int limit);

int next(int value) {
  return value + 1;
}

int main(void) {
  if (sum_next(-2) != 0 || sum_next(0) != 0 || sum_next(1) != 1 ||
      sum_next(4) != 10 || sum_next(8) != 36) {
    return 1;
  }
  printf("%d\n", sum_next(4));
  return 0;
}
