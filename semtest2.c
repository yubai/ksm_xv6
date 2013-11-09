#include "types.h"
#include "user.h"
#include "sem.h"

int sem_full, sem_empty, sem_mutex;
int buf[10];

void
producer(void)
{
  int i = 1;
  int cursor1 = 0;
  while (1) {
    sem_wait(sem_empty);
    sem_wait(sem_mutex);
    printf(1, "producer writing index(%d) %d.\n", cursor1%10, i);
    buf[(cursor1++)%10] = (i++)%10;
    sem_signal(sem_mutex);
    sem_signal(sem_full);
  }
}

void
comsumer()
{
  int j=1;
  int cursor2 = 0;
  while (1) {
    sem_wait(sem_full);
    sem_wait(sem_mutex);
    buf[cursor2%10] = (j++)%10;
    printf(1, "\tcomsumer reading index(%d) %d.\n",
           (cursor2%10), buf[(cursor2%10)]);
    cursor2++;
    sem_signal(sem_mutex);
    sem_signal(sem_empty);
  }
}

int main(int argc, char *argv[])
{
  memset(buf, 0, sizeof(buf));
  sem_full = sem_get(1, 0);
  sem_empty = sem_get(2, 10);
  sem_mutex = sem_get(3, 1);

  int pid;

  pid = fork();

  if (pid == 0) {
    producer();
  } else {
    comsumer();
  }
    
  return 0;
}

