#include <stdio.h>
#include "file_system.h"

#define MAX_EXPRESSION_LENGTH 16
#define MAX_DATA_LENGTH 1024

int main() {
    initFS();

    char expression[MAX_EXPRESSION_LENGTH];
    while (true) {
        scanf("%s", expression);
        if (strcmp(expression, "exit") == 0) {
            break;

        } else if (strcmp(expression, "create_dir") == 0) {
            char name[MAX_NAME_LENGTH];
            scanf("%s", name);
            actWithObject(name, DIRECTORY_TYPE, CREATE_ACTION, NULL, 0, NULL);

        } else if (strcmp(expression, "create_file") == 0) {
            char name[MAX_NAME_LENGTH];
            char data[MAX_DATA_LENGTH];

            scanf("%s", name);
            scanf("%s", data);
            actWithObject(name, FILE_TYPE, CREATE_ACTION, data, strlen(data), NULL);

        } else if (strcmp(expression, "delete") == 0) {
            char name[MAX_NAME_LENGTH];
            scanf("%s", name);
            removeName(name);

        } else if (strcmp(expression, "read_from_file") == 0) {
            char fromName[MAX_NAME_LENGTH];
            char fsFileName[MAX_NAME_LENGTH];

            scanf("%s", fromName);
            scanf("%s", fsFileName);
            createFileInFS(fromName, fsFileName);

        } else if (strcmp(expression, "write_to_disk") == 0) {
            char fsFileName[MAX_NAME_LENGTH];
            char diskFileName[MAX_NAME_LENGTH];

            scanf("%s", fsFileName);
            scanf("%s", diskFileName);
            actWithObject(fsFileName, FILE_TYPE, WRITE_ACTION, diskFileName, strlen(diskFileName), NULL);

        } else if (strcmp(expression, "display_dir") == 0) {
            char name[MAX_NAME_LENGTH];
            scanf("%s", name);
            printDir(name);
        } else if (strcmp(expression, "display_file") == 0){
            char name[MAX_NAME_LENGTH];
            scanf("%s", name);
            actWithObject(name, FILE_TYPE, WRITE_ACTION, NULL, 0, NULL);

        } else {
            printf("unknown command");
        }
    }

    closeFS();
    return 0;
}