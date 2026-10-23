#include <stdlib.h>
double *merge_array(int n, double *a, int m, double *b);
void merge_sort(int n, double *a);
void swap(double *a, double *b);

// function to merge the array a with n elements with the array b with m
// elements function returns the nerged array
double *merge_array(int n, double *a, int m, double *b) {
  int i, j, k;
  double *c = (double *)calloc(n + m, sizeof(double));
  for (i = j = k = 0; (i < n) && (j < m);)
    if (a[i] <= b[j])
      c[k++] = a[i++];
    else
      c[k++] = b[j++];
  if (i == n)
    for (; j < m;)
      c[k++] = b[j++];
  else
    for (; i < n;)
      c[k++] = a[i++];
  return c;
}
// function to merge sort the array a with n elements
void merge_sort(int n, double *a) {
  double *c;
  int i;
  if (n <= 1)
    return;
  if (n == 2) {
    if (a[0] > a[1])
      swap(&a[0], &a[1]);
    return;
  }
  merge_sort(n / 2, a);
  merge_sort(n - n / 2, a + n / 2);
  c = merge_array(n / 2, a, n - n / 2, a + n / 2);
  for (i = 0; i < n; i++)
    a[i] = c[i];
  return;
}
// swap two doubles
void swap(double *a, double *b) {
  double temp;
  temp = *a;
  *a = *b;
  *b = temp;
}
