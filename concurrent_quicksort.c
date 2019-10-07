#define _POSIX_C_SOURCE 199309L  // required for clock
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <limits.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <inttypes.h>
#include <math.h>

int *shareMem(size_t size) {
  key_t mem_key = IPC_PRIVATE;
  int shm_id = shmget(mem_key, size, IPC_CREAT | 0666);
  return (int *)shmat(shm_id, NULL, 0);
}

void merge(int *arr, int l, int r) {
  int mid = (l + r) / 2;
  int *sorted = (int *)malloc(sizeof(int) * (r - l + 1));
  int j = mid + 1, c = 0, i = 0;

  for (i = l; i <= mid; i++) {
    for (; j <= r && arr[j] <= arr[i]; j++) sorted[c++] = arr[j];
    sorted[c++] = arr[i];
  }

  while (i <= mid) sorted[c++] = arr[i++];
  while (j <= r) sorted[c++] = arr[j++];

  for (i = 0; i < c; i++) arr[l + i] = sorted[i];
  free(sorted);
}

void swap(int *a, int *b) {
  int t = *a;
  *a = *b;
  *b = t;
}

int partition(int *arr, int l, int r) {
  int pivot = rand() % (r - l + 1) + l;
  swap(&arr[l], &arr[pivot]);
  int reached = l + 1;
  for (int i = l + 1; i <= r; i++) {
    if (arr[i] <= arr[l]) {
      swap(&arr[reached], &arr[i]);
      reached++;
    }
  }
  // for (int i = l; i <= r; i++) {
  //   printf("%d ", arr[i]);
  // }
  swap(&arr[l], &arr[reached - 1]);
  // printf("\n");s
  return reached - 1;
}

void normal_quicksort(int *arr, int l, int r) {
  if (l > r) return;

  // insertion sort
  if (r - l + 1 <= 5) {
    int a[5], mi = INT_MAX, mid = -1;
    for (int i = l; i < r; i++) {
      int j = i + 1;
      for (; j <= r; j++)
        if (arr[j] < arr[i] && j <= r) {
          int temp = arr[i];
          arr[i] = arr[j];
          arr[j] = temp;
        }
    }
    return;
  }
  int part = partition(arr, l, r);
  normal_quicksort(arr, l, part - 1);
  normal_quicksort(arr, part + 1, r);
}

void quicksort(int *arr, int l, int r) {
  if (l > r) _exit(1);

  // insertion sort
  if (r - l + 1 <= 5) {
    int a[5], mi = INT_MAX, mid = -1;
    for (int i = l; i < r; i++) {
      int j = i + 1;
      for (; j <= r; j++)
        if (arr[j] < arr[i] && j <= r) {
          int temp = arr[i];
          arr[i] = arr[j];
          arr[j] = temp;
        }
    }
    return;
  }

  int p = partition(arr, l, r);
  int pid1 = fork();
  int pid2;
  if (pid1 == 0) {
    // sort left half array
    quicksort(arr, l, p - 1);
    _exit(1);
  } else {
    pid2 = fork();
    if (pid2 == 0) {
      // sort right half array
      quicksort(arr, p + 1, r);
      _exit(1);
    } else {
      // wait for the right and the left half to get sorted
      int status;
      waitpid(pid1, &status, 0);
      waitpid(pid2, &status, 0);
    }
  }
  return;
}

struct arg {
  int l;
  int r;
  int *arr;
};

void *threaded_quicksort(void *a) {
  // note that we are passing a struct to the threads for simplicity.
  struct arg *args = (struct arg *)a;

  int l = args->l;
  int r = args->r;
  int *arr = args->arr;
  if (l > r) return NULL;

  // insertion sort
  if (r - l + 1 <= 5) {
    int a[5], mi = INT_MAX, mid = -1;
    for (int i = l; i < r; i++) {
      int j = i + 1;
      for (; j <= r; j++)
        if (arr[j] < arr[i] && j <= r) {
          int temp = arr[i];
          arr[i] = arr[j];
          arr[j] = temp;
        }
    }
    return NULL;
  }

  int p = partition(arr, l, r);

  // sort left half array
  struct arg a1;
  a1.l = l;
  a1.r = p - 1;
  a1.arr = arr;
  pthread_t tid1;
  pthread_create(&tid1, NULL, threaded_quicksort, &a1);

  // sort right half array
  struct arg a2;
  a2.l = p + 1;
  a2.r = r;
  a2.arr = arr;
  pthread_t tid2;
  pthread_create(&tid2, NULL, threaded_quicksort, &a2);

  // wait for the two halves to get sorted
  pthread_join(tid1, NULL);
  pthread_join(tid2, NULL);
}

void runSorts(long long int n) {
  struct timespec ts;
  // getting shared memory
  int *arr = shareMem(sizeof(int) * (n + 1));
  for (int i = 0; i < n; i++) scanf("%d", arr + i);
  srand(n + 1);

  long double en, st;

  printf("Running concurrent_mergesort for n = %lld\n", n);
  clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
  st = ts.tv_nsec / (1e9) + ts.tv_sec;

  // multiprocess mergesort
  quicksort(arr, 0, n - 1);

  clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
  en = ts.tv_nsec / (1e9) + ts.tv_sec;
  printf("time = %Lf\n", en - st);
  long double t1 = en - st;

  int brr[n + 1];
  for (int i = 0; i < n; i++) brr[i] = arr[i];

  pthread_t tid;
  struct arg a;
  a.l = 0;
  a.r = n - 1;
  a.arr = brr;
  printf("Running threaded_concurrent_mergesort for n = %lld\n", n);
  clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
  st = ts.tv_nsec / (1e9) + ts.tv_sec;

  // multithreaded mergesort
  pthread_create(&tid, NULL, threaded_quicksort, &a);
  pthread_join(tid, NULL);

  clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
  en = ts.tv_nsec / (1e9) + ts.tv_sec;
  printf("time = %Lf\n", en - st);
  long double t2 = en - st;

  printf("Running normal_mergesort for n = %lld\n", n);
  clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
  st = ts.tv_nsec / (1e9) + ts.tv_sec;

  // normal mergesort
  normal_quicksort(arr, 0, n - 1);

  clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
  en = ts.tv_nsec / (1e9) + ts.tv_sec;
  printf("time = %Lf\n", en - st);
  long double t3 = en - st;

  // for (int i = 0; i < n; i++) printf("%d ", arr[i]);
  printf(
      "normal_mergesort ran:\n\t[ %Lf ] times faster than "
      "concurrent_mergesort\n\t[ %Lf ] times faster than "
      "threaded_concurrent_mergesort\n\n\n",
      t1 / t3, t2 / t3);
  shmdt(arr);
}

int main() {
  long long int n;
  scanf("%lld", &n);
  runSorts(n);
  return 0;
}