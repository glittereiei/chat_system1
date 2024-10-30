#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>

#define FIFO1 "fifo1to2"
#define FIFO2 "fifo2to1"
#define BUFFER_SIZE 1024

int is_running = 1;

void *read_messages(void *arg) {
    int fd = *(int *)arg;
    char buffer[BUFFER_SIZE];

    while (is_running) {
        memset(buffer, '\0', sizeof(buffer));
        if (read(fd, buffer, BUFFER_SIZE) > 0) {
            if (strcmp(buffer, "end chat") == 0) {
                printf("Chat ended by the other user.\n");
                is_running = 0;
                break;
            }
            printf("Received: %s\n", buffer);
        }
    }
    close(fd);
    return NULL;
}

void *send_messages(void *arg) {
    int fd = *(int *)arg;
    char buffer[BUFFER_SIZE];

    while (is_running) {
        memset(buffer, '\0', sizeof(buffer));
        printf("Enter message: ");
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = 0;  // Remove newline character
        write(fd, buffer, strlen(buffer) + 1);
        if (strcmp(buffer, "end chat") == 0) {
            printf("Ending chat...\n");
            is_running = 0;
            break;
        }
    }
    close(fd);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <1 or 2>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int process_id = atoi(argv[1]);
    if (process_id != 1 && process_id != 2) {
        fprintf(stderr, "Invalid argument. Use 1 or 2 to specify process.\n");
        exit(EXIT_FAILURE);
    }

    // สร้าง FIFO ถ้ายังไม่มี
    if (access(FIFO1, F_OK) == -1) mkfifo(FIFO1, 0666);
    if (access(FIFO2, F_OK) == -1) mkfifo(FIFO2, 0666);

    int read_fd, write_fd;
    if (process_id == 1) {
        read_fd = open(FIFO1, O_RDONLY);
        write_fd = open(FIFO2, O_WRONLY);
        printf("Process P1 started. You can send messages.\n");
    } else {
        read_fd = open(FIFO2, O_RDONLY);
        write_fd = open(FIFO1, O_WRONLY);
        printf("Process P2 started. You can send messages.\n");
    }

    // สร้าง thread เพื่อแยกการทำงานระหว่างการส่งและรับข้อความ
    pthread_t read_thread, write_thread;
    pthread_create(&read_thread, NULL, read_messages, &read_fd);
    pthread_create(&write_thread, NULL, send_messages, &write_fd);

    // รอให้ thread ทำงานเสร็จ
    pthread_join(read_thread, NULL);
    pthread_join(write_thread, NULL);

    // ลบ FIFO หลังจากใช้งานเสร็จ (ลบเฉพาะฝั่งที่เป็น P1 เพื่อป้องกันการลบก่อนอีกโปรเซสจะเสร็จสิ้น)
    if (process_id == 1) {
        unlink(FIFO1);
        unlink(FIFO2);
    }

    printf("Chat program ended.\n");
    return 0;
}
