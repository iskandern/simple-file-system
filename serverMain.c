#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <syslog.h>
#include <signal.h>
#include <sys/stat.h>
#include "file_system.h"

char buf[MAX_INODE_BLOCKS * BLOCK_SIZE + MAX_NAME_LENGTH + MAX_NAME_LENGTH + 2];

void daemonize() {
    pid_t pid;
    pid = fork();
    if (pid < 0)
        exit(EXIT_FAILURE);
    if (pid > 0)
        exit(EXIT_SUCCESS);

    if (setsid() < 0)
        exit(EXIT_FAILURE);

    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    pid = fork();
    if (pid < 0)
        exit(EXIT_FAILURE);
    if (pid > 0)
        exit(EXIT_SUCCESS);

    umask(0);
    chdir("/");
    openlog("mydaemon",
            LOG_PID,
            LOG_DAEMON);
}

int main(int argc, char *argv[]) {
    int sock, listener, bytesRead;
    struct sockaddr_in addr;

    if (argc != 2) {
        return 1;
    }

    listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) {
        perror("socket");
        return 1;
    }

    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[1]));
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    printf("%d\n", atoi(argv[1]));

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }

    listen(listener, 1);

//    daemonize();
    while (true) {
        sock = accept(listener, NULL, NULL);
        if (sock < 0) {
            perror("accept");
            return 1;
        }

        initFS();

        while (true) {

            bytesRead = recv(sock, buf, MAX_INODE_BLOCKS * BLOCK_SIZE + MAX_NAME_LENGTH + MAX_NAME_LENGTH + 2, 0);
            if (bytesRead <= 0) break;

            size_t pos = 0;
            char operation[MAX_NAME_LENGTH];
            while (buf[pos] != ' ') {
                operation[pos] = buf[pos];
                pos++;
            }
            operation[pos] = '\0';
            pos++;
//            printf("%s\n%s ", buf, operation);

            if (strcmp(operation, "exit") == 0) {
                break;

            } else if (strcmp(operation, "create_dir") == 0) {
                char name[MAX_NAME_LENGTH];

                size_t namePos = 0;
                while (buf[pos] != '\0') {
                    name[namePos] = buf[pos];
                    pos++;
                    namePos++;
                    if (namePos >= MAX_NAME_LENGTH - 1) {
                        break;
                    }
                }
                name[namePos] = '\0';
                actWithObject(name, DIRECTORY_TYPE, CREATE_ACTION, NULL, 0, NULL);

                send(sock, outData, sizeof(outData), 0);

//                printf("|%s|\n", outData);

            } else if (strcmp(operation, "create_file") == 0) {
                char name[MAX_NAME_LENGTH];
                char data[MAX_INODE_BLOCKS * BLOCK_SIZE];

                size_t namePos = 0;
                while (buf[pos] != ' ') {
                    name[namePos] = buf[pos];
                    pos++;
                    namePos++;
                    if (namePos >= MAX_NAME_LENGTH - 1 || buf[pos] == '\0') {
                        break;
                    }
                }
                name[namePos] = '\0';
                pos++;

                size_t dataPos = 0;
                while (buf[pos] != '\0') {
                    data[dataPos] = buf[pos];
                    pos++;
                    dataPos++;
                    if (dataPos >= MAX_INODE_BLOCKS * BLOCK_SIZE - 1) {
                        break;
                    }
                }
                data[dataPos] = '\0';

                actWithObject(name, FILE_TYPE, CREATE_ACTION, data, strlen(data), NULL);

                send(sock, outData, sizeof(outData), 0);

            } else if (strcmp(operation, "delete") == 0) {
                char name[MAX_NAME_LENGTH];

                size_t namePos = 0;
                while (buf[pos] != '\0') {
                    name[namePos] = buf[pos];
                    pos++;
                    namePos++;
                    if (namePos >= MAX_NAME_LENGTH - 1) {
                        break;
                    }
                }
                name[namePos] = '\0';

                removeName(name);

                send(sock, outData, sizeof(outData), 0);

            } else if (strcmp(operation, "read_from_file") == 0) {
                char fromName[MAX_NAME_LENGTH];
                char fsFileName[MAX_NAME_LENGTH];

                size_t fromNamePos = 0;
                while (buf[pos] != ' ') {
                    fromName[fromNamePos] = buf[pos];
                    pos++;
                    fromNamePos++;
                    if (fromNamePos >= MAX_NAME_LENGTH - 1 || buf[pos] == '\0') {
                        break;
                    }
                }
                fromName[fromNamePos] = '\0';
                pos++;

                size_t fsFileNamePos = 0;
                while (buf[pos] != '\0') {
                    fsFileName[fsFileNamePos] = buf[pos];
                    pos++;
                    fsFileNamePos++;
                    if (fsFileNamePos >= MAX_INODE_BLOCKS * BLOCK_SIZE - 1) {
                        break;
                    }
                }
                fsFileName[fsFileNamePos] = '\0';

//                printf("{%s}\n{%s}\n", fromName, fsFileName);

                createFileInFS(fromName, fsFileName);

                send(sock, outData, sizeof(outData), 0);

            } else if (strcmp(operation, "write_to_disk") == 0) {
                char fsFileName[MAX_NAME_LENGTH];
                char diskFileName[MAX_NAME_LENGTH];

                size_t fsFileNamePos = 0;
                while (buf[pos] != ' ') {
                    fsFileName[fsFileNamePos] = buf[pos];
                    pos++;
                    fsFileNamePos++;
                    if (fsFileNamePos >= MAX_NAME_LENGTH - 1 || buf[pos] == '\0') {
                        break;
                    }
                }
                fsFileName[fsFileNamePos] = '\0';
                pos++;

                size_t diskFileNamePos = 0;
                while (buf[pos] != '\0') {
                    diskFileName[diskFileNamePos] = buf[pos];
                    pos++;
                    diskFileNamePos++;
                    if (diskFileNamePos >= MAX_INODE_BLOCKS * BLOCK_SIZE - 1) {
                        break;
                    }
                }
                diskFileName[diskFileNamePos] = '\0';
//                printf("%s\n", diskFileName);

                actWithObject(fsFileName, FILE_TYPE, WRITE_ACTION, diskFileName, strlen(diskFileName), NULL);

                send(sock, outData, sizeof(outData), 0);

            } else if (strcmp(operation, "display_dir") == 0) {
                char name[MAX_NAME_LENGTH];

                size_t namePos = 0;
                while (buf[pos] != '\0') {
                    name[namePos] = buf[pos];
                    pos++;
                    namePos++;
                    if (namePos >= MAX_NAME_LENGTH) {
                        break;
                    }
                }
                name[namePos] = '\0';

                printDir(name);

                send(sock, outData, sizeof(outData), 0);

            } else if (strcmp(operation, "display_file") == 0){
                char name[MAX_NAME_LENGTH];

                size_t namePos = 0;
                while (buf[pos] != '\0') {
                    name[namePos] = buf[pos];
                    pos++;
                    namePos++;
                    if (namePos >= MAX_NAME_LENGTH) {
                        break;
                    }
                }
                name[namePos] = '\0';

                actWithObject(name, FILE_TYPE, WRITE_ACTION, NULL, 0, NULL);

                send(sock, outData, sizeof(outData), 0);

            } else {
                break;
            }
        }

        closeFS();
        close(sock);
    }

    closelog();
    return EXIT_SUCCESS;
}
