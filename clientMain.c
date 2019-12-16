#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>


#define MAX_EXPRESSION_LENGTH 16
#define MAX_DATA_LENGTH 1024
#define BLOCK_SIZE 512
#define MAX_NAME_LENGTH 16
#define MAX_INODE_BLOCKS 8

char outData[MAX_INODE_BLOCKS * BLOCK_SIZE + MAX_NAME_LENGTH + MAX_EXPRESSION_LENGTH + 2];

int main(int argc, char *argv[]) {
    char message[MAX_DATA_LENGTH];
    int sock;
    struct sockaddr_in addr;

    if (argc != 2) {
        return 1;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[1]));
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if(connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        return 2;
    }


    char expression[MAX_EXPRESSION_LENGTH];
    while (true) {
        size_t outDataPos = 0;
        scanf("%s", expression);
        sprintf(outData, "%s ", expression);
        outDataPos += strlen(expression) + 1;
        if (strcmp(expression, "exit") == 0) {
            send(sock, outData, sizeof(outData), 0);
            break;

        } else if (strcmp(expression, "create_dir") == 0) {

            char name[MAX_NAME_LENGTH];
            scanf("%s", name);

            sprintf(outData + outDataPos, "%s", name);

            printf("%s\n", outData);
            send(sock, outData, sizeof(outData), 0);

            if (recv(sock, outData, sizeof(outData), 0) != 0) {
                printf("%s\n", outData);
            } else {
                printf("error\n");
            }

        } else if (strcmp(expression, "create_file") == 0) {

            char name[MAX_NAME_LENGTH];
            char data[MAX_DATA_LENGTH];

            scanf("%s", name);
            scanf("%s", data);

            sprintf(outData + outDataPos, "%s ", name);
            outDataPos += strlen(name) + 1;

            sprintf(outData + outDataPos, "%s", data);

            printf("%s\n", outData);
            send(sock, outData, sizeof(outData), 0);

            if (recv(sock, outData, sizeof(outData), 0) != 0) {
                printf("%s\n", outData);
            } else {
                printf("error\n");
            }

        } else if (strcmp(expression, "delete") == 0) {

            char name[MAX_NAME_LENGTH];
            scanf("%s", name);

            sprintf(outData + outDataPos, "%s", name);

            printf("%s\n", outData);
            send(sock, outData, sizeof(outData), 0);

            if (recv(sock, outData, sizeof(outData), 0) != 0) {
                printf("%s\n", outData);
            } else {
                printf("error\n");
            }

        } else if (strcmp(expression, "read_from_file") == 0) {

            char fromName[MAX_NAME_LENGTH];
            char fsFileName[MAX_NAME_LENGTH];

            scanf("%s", fromName);
            scanf("%s", fsFileName);

            sprintf(outData + outDataPos, "%s ", fromName);
            outDataPos += strlen(fromName) + 1;

            sprintf(outData + outDataPos, "%s", fsFileName);

            printf("%s\n", outData);
            send(sock, outData, sizeof(outData), 0);

            if (recv(sock, outData, sizeof(outData), 0) != 0) {
                printf("%s\n", outData);
            } else {
                printf("error\n");
            }

        } else if (strcmp(expression, "write_to_disk") == 0) {

            char fsFileName[MAX_NAME_LENGTH];
            char diskFileName[MAX_NAME_LENGTH];

            scanf("%s", fsFileName);
            scanf("%s", diskFileName);

            sprintf(outData + outDataPos, "%s ", fsFileName);
            outDataPos += strlen(fsFileName) + 1;

            sprintf(outData + outDataPos, "%s", diskFileName);

            printf("%s\n", outData);
            send(sock, outData, sizeof(outData), 0);

            if (recv(sock, outData, sizeof(outData), 0) != 0) {
                printf("%s\n", outData);
            } else {
                printf("error\n");
            }

        } else if (strcmp(expression, "display_dir") == 0) {

            char name[MAX_NAME_LENGTH];
            scanf("%s", name);

            sprintf(outData + outDataPos, "%s", name);

            send(sock, outData, sizeof(outData), 0);

            if (recv(sock, outData, sizeof(outData), 0) != 0) {
                printf("%s\n", outData);
            } else {
                printf("error\n");
            }

        } else if (strcmp(expression, "display_file") == 0){

            char name[MAX_NAME_LENGTH];
            scanf("%s", name);

            sprintf(outData + outDataPos, "%s", name);

            send(sock, outData, sizeof(outData), 0);

            if (recv(sock, outData, sizeof(outData), 0) != 0) {
                printf("%s\n", outData);
            } else {
                printf("error\n");
            }

        } else {
            printf("unknown command\n");
        }
    }

    close(sock);
    return 0;
}
