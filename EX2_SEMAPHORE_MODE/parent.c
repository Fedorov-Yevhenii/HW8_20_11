#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

/*
 * Модифікуйте програми із Завдання No1, синхронізувавши їх роботу за допомогою семафорів.
 * Розв'яжіть задачу в наступній модифікації: є два процеси — серверний,
 * який створює загальні області пам'яті, ключів для засобів міжпроцесної взаємодії.
 * Потрібно продумати механізм зупинення роботи сервера - щоб засоби взаємодії процесів були видалені із системи.
*/

/*
 * Батьківський процес зчитує введені числа, передає їх дочірньому процесу через спільний сегмент пам'яті та семафор, отримує результат обчислення та виводить її.
*/

#define SHM_SIZE 1024

union semun_parent { // Структура для об'єднання семафорів
    int val;
    struct semid_ds *sem_buf;
    unsigned short *sem_array;
};

int shm_id_parent;
void *shm_ptr_parent;
pid_t child_pid_parent;
int sem_id_parent;

void signal_handler_parent(int sig) { // Обробник сигналів для батьківського процесу
    if (sig == SIGUSR1) {
        int sum;
        memcpy(&sum, shm_ptr_parent, sizeof(int));
        printf("Sum: %d\n", sum);
    }
}

void init_semaphore_parent(int sem_id, int sem_value) { // Ініціалізація семафора для батьківського процесу
    union semun_parent argument;
    argument.val = sem_value;
    if (semctl(sem_id, 0, SETVAL, argument) == -1) {
        perror("semctl");
        exit(1);
    }
}

void P_parent(int sem_id) { // Операція P(S) для батьківського процесу перевіряє стан семафору. Якщо семафор не рівний нулю, то виконується операція S:=S-1
    struct sembuf operations[1] = {{0, -1, 0}};
    if (semop(sem_id, operations, 1) == -1) {
        perror("semop P");
        exit(1);
    }
}

void V_parent(int sem_id) { // Операція V(S) для батьківського процесу виконує S=S+1
    struct sembuf operations[1] = {{0, 1, 0}};
    if (semop(sem_id, operations, 1) == -1) {
        perror("semop V");
        exit(1);
    }
}

int main(int argc, char *argv[]) {

    // Створення спільної пам'яті для батьківського процесу
    shm_id_parent = shmget(IPC_PRIVATE, SHM_SIZE, IPC_CREAT | 0666);
    shm_ptr_parent = shmat(shm_id_parent, NULL, 0);
    printf("Server: Creating shared memory segment...\n");
    if (shm_ptr_parent == (void *) -1) {
        perror("shmat");
        exit(1);
    }

    // Створення семафору для батьківського процесу
    printf("Server: Creating semaphore...\n");
    sem_id_parent = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
    if (sem_id_parent == -1) {
        perror("semget");
        exit(1);
    }
    printf("Server: Initializing semaphore...\n");
    init_semaphore_parent(sem_id_parent, 1);

    signal(SIGUSR1, signal_handler_parent); // Функція для обробки сигналу

    // Роздвоєння процесу
    printf("Server: Creating child process...\n");
    child_pid_parent = fork();
    if (child_pid_parent == 0) {
        char shm_id_str[10];
        char sem_id_str[10];
        sprintf(shm_id_str, "%d", shm_id_parent);
        sprintf(sem_id_str, "%d", sem_id_parent);
        // Виконання дочірнього процесу
        execlp("./child_process", "child_process", shm_id_str, sem_id_str, (char *) NULL);
        perror("Parent: execlp:");
        exit(1);
    }

    while (1) {
        int n, i, input;
        printf("Enter quantity of nums to sum (0 - exit): ");
        scanf("%d", &n);

        if (n <= 0) {
            break;
        }
        // Блокування семафору перед записом даних
        printf("Server: Locking semaphore before writing data...\n");
        P_parent(sem_id_parent);

        for (i = 0; i < n; i++) {
            printf("Enter number: ");
            scanf("%d", &input);
            memcpy((int *)shm_ptr_parent + i, &input, sizeof(int));
        }
        printf("Server: Releasing semaphore after writing...\n");
        V_parent(sem_id_parent);
        printf("Server: Sending signal to child process...\n");
        kill(child_pid_parent, SIGUSR1);
        printf("Server: Waiting for response from child...\n");
        pause();
    }

    // Завершення дочірнього процесу
    kill(child_pid_parent, SIGTERM);
    waitpid(child_pid_parent, NULL, 0);
    shmdt(shm_ptr_parent);
    shmctl(shm_id_parent, IPC_RMID, NULL);
    semctl(sem_id_parent, 0, IPC_RMID, 0);

    return 0;
}
