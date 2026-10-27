/* Keep the callee in another file so the IR retains a call. */
int next(int value);

int sum_next(int limit) {
  int total = 0;
  int index = 0;
  while (index < limit) {
    total = total + next(index);
    index = index + 1;
  }
  return total;
}
