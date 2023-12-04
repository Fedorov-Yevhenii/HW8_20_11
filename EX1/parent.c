#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

/*
 * Напишіть дві програми, які обмінюються інформацією за допомогою розділеної пам'яті. Синхронізуйте їхню спільну роботу за допомогою сигналів.
 * +Одна програма під час запуску отримує необхідний розмір буфера, створює сегмент пам'яті відповідного розміру, а потім створює процес-нащадок і завантажує у нього бінарний образ другої програми.
 * +Перша програма отримує у користувача кількість даних, що вводяться, зчитує їх і поміщає в сегмент загальної пам'яті.
 * +Потім повідомляє про це другий процес за допомогою сигналу (виберіть для цього відповідний сигнал) і «засинає» - чекає на відповідь другого процесу.
 * +Другий процес «прокидається» отримавши сигнал від першого процесу, обчислює суму записаних у сегмент загальної пам'яті чисел і також записує її в сегмент загальної пам'яті, посилає сигнал першому процесу та засинає.
 * +Перший процес виводить отриману суму стандартний потік виведення.
 * Процеси поперемінно працюють таким чином, поки користувач не припинить цей процес.
 * Для цього слід ввести ознаку припинення роботи з клавіатури.
 * +Перший процес посилає сигнал про завершення роботи другого процесу, видаляє сегмент загальної пам'яті і завершується.
*/

#define SHM_SIZE 1024

int shm_id;
void *shm_ptr;
pid_t child_pid;

void parent_signal_handler(int sig) {

    // ... виводить отриману суму стандартний потік виведення.

    printf("Parent: Signal handler called with signal: %d\n", sig);
    if (sig == SIGUSR1) {
        int sum;
        memcpy(&sum, shm_ptr, sizeof(int));
        printf("Parent: Sum is: %d\n", sum);
    }
}

int main() {
    // ... Отримує необхідний розмір буфера (1), створює сегмент пам'яті відповідного розміру (2), а потім створює процес-нащадок (3) і завантажує у нього бінарний образ другої програми (4).
    shm_id = shmget(IPC_PRIVATE, SHM_SIZE, IPC_CREAT | 0666); // (1)
    if (shm_id == -1) {
        perror("Parent: shmget");
        exit(1);
    }
    shm_ptr = shmat(shm_id, NULL, 0); // (2)
    if (shm_ptr == (void *) -1) {
        perror("Parent: shmat");
        exit(1);
    }
    signal(SIGUSR1, parent_signal_handler);
    child_pid = fork(); // (3)
    if (child_pid == -1) {
        perror("Parent: fork");
        exit(1);
    }
    if (child_pid == 0) {
        char shm_id_str[10];
        sprintf(shm_id_str, "%d", shm_id);
        execlp("./child_process", "child_process", shm_id_str, NULL); // (4) ідентифікатор загальної області пам'яті передається другому процесу за допомогою додаткової інформації сигналу, що ініціалізує роботу;
        perror("Parent: execlp");
        exit(1);
    }
    else {
        while (1) {

            //... отримує у користувача кількість даних, що вводяться, зчитує їх і поміщає в сегмент загальної пам'яті.
            int n, i, input;
            printf("Parent: Enter quantity of numbers to sum (0 to exit): ");
            scanf("%d", &n);
            if (n <= 0) break;
            for (i = 0; i < n; i++) {
                printf("Parent: Input number: ");
                scanf("%d", &input);
                memcpy(shm_ptr + i * sizeof(int), &input, sizeof(int));
            }
            int end_marker = 0;
            memcpy(shm_ptr + n * sizeof(int), &end_marker, sizeof(int));

            // ... повідомляє про це другий процес за допомогою сигналу (виберіть для цього відповідний сигнал) і «засинає» - чекає на відповідь другого процесу.
            kill(child_pid, SIGUSR1);
            pause();
        }

        // ... посилає сигнал про завершення роботи другого процесу, видаляє сегмент загальної пам'яті і завершується
        kill(child_pid, SIGTERM);
        waitpid(child_pid, NULL, 0);
        shmdt(shm_ptr);
        shmctl(shm_id, IPC_RMID, NULL);
    }

    return 0;
}