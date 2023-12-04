#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <string.h>

union semun_child {
    int val;
    struct semid_ds *sem_buf;
    unsigned short *sem_array;
    struct seminfo *__buf;
};

void P_child(int sem_id) {
    struct sembuf op = { 0, -1, SEM_UNDO };
    if (semop(sem_id, &op, 1) == -1) {
        perror("semop P failed");
        exit(EXIT_FAILURE);
    }
}

void V_child(int sem_id) {
    struct sembuf op = { 0, 1, SEM_UNDO };
    if (semop(sem_id, &op, 1) == -1) {
        perror("semop V failed");
        exit(EXIT_FAILURE);
    }
}

int shm_id_child;
void *shm_ptr_child;
int sem_id_child;

void signal_handler_child(int sig) {
    printf("Client: Signal handler received signal %d\n", sig);
    if (sig == SIGUSR1) {
        int sum = 0, i = 0, val;

        // Блокування семафору перед зчитуванням даних
        printf("Client: Locking semaphore before reading data...\n");
        P_child(sem_id_child);

        // Зчитування даних зі спільної пам'яті та обчислення суми
        do {
            memcpy(&val, (int *)shm_ptr_child + i, sizeof(int));
            if (val == 0) break;
            sum += val;
            i++;
        } while (1);

        // Запис суми в спільну пам'ять
        memcpy(shm_ptr_child, &sum, sizeof(int));
        // Виконуємо S=S+1 після запису
        printf("Client: Releasing semaphore after reading...\n");
        V_child(sem_id_child);
        printf("Client: Sending signal back to server...\n");
        kill(getppid(), SIGUSR1);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <shm_id> <sem_id>\n", argv[0]);
        return 1;
    }

    printf("Client: Connecting to shared memory...\n");
    shm_id_child = atoi(argv[1]);
    sem_id_child = atoi(argv[2]);

    shm_ptr_child = shmat(shm_id_child, NULL, 0);
    if (shm_ptr_child == (void *) -1) {
        perror("shmat");
        exit(1);
    }

    printf("Client: Connecting to semaphore...\n");
    signal(SIGUSR1, signal_handler_child);

    while (1) {
        pause();
    }
}