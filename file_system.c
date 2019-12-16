#include "file_system.h"

char outData[MAX_INODE_BLOCKS * BLOCK_SIZE + MAX_NAME_LENGTH + MAX_NAME_LENGTH + 2];
size_t outDataPos = 0;

// just calculating iNode and blocks number
void calculate (double iNodesNumPerBlock, size_t minBlockNum, size_t minINodeNum, size_t* resultBlocksNum, size_t* resultINodesNum) {
    if (sizeof(struct SuperBlock) + (sizeof(size_t) + sizeof(struct INode)) * minINodeNum +
       (sizeof(size_t) + BLOCK_SIZE) * minBlockNum >= MAX_FS_SIZE) {
        *resultBlocksNum = 0;
        *resultINodesNum = 0;
        return;
    }

    //getting a lines intersection
    double interBlockNum = (MAX_FS_SIZE - sizeof(struct SuperBlock)) /
                           (sizeof(size_t) + sizeof(struct INode) * iNodesNumPerBlock +
                           (sizeof(size_t) + BLOCK_SIZE));
    double interINodeNum = (MAX_FS_SIZE - sizeof(struct SuperBlock) -
                           (sizeof(size_t) + BLOCK_SIZE) * interBlockNum /
                           (sizeof(size_t) + sizeof(struct INode)));

    if ((double) minBlockNum > interBlockNum) {
        *resultBlocksNum = minBlockNum;
        *resultINodesNum = (size_t) (MAX_FS_SIZE - sizeof(struct SuperBlock) -
                                    (sizeof(size_t) + BLOCK_SIZE) * minBlockNum) /
                                    (sizeof(size_t) + sizeof(struct INode));
        return;
    }

    if ((double) minINodeNum > interINodeNum) {
        *resultBlocksNum = (MAX_FS_SIZE - sizeof(struct SuperBlock) -
                           sizeof(size_t) + sizeof(struct INode) * minINodeNum) /
                           (sizeof(size_t) + BLOCK_SIZE);
        *resultINodesNum = (size_t) (MAX_FS_SIZE - sizeof(struct SuperBlock) -
                                    (sizeof(size_t) + BLOCK_SIZE) * (*resultBlocksNum)) /
                                    (sizeof(size_t) + sizeof(struct INode));
        return;
    }

    size_t firstResultBlocksNumCandidate = (size_t) interBlockNum;
    size_t secondResultBlocksNumCandidate = firstResultBlocksNumCandidate + 1;

    size_t firstResultINodesNumCandidate = (size_t) (MAX_FS_SIZE - sizeof(struct SuperBlock) -
                                  (sizeof(size_t) + BLOCK_SIZE) * firstResultBlocksNumCandidate) /
                                  (sizeof(size_t) + sizeof(struct INode));
    size_t secondResultINodesNumCandidate = (size_t) (MAX_FS_SIZE - sizeof(struct SuperBlock) -
                                            (sizeof(size_t) + BLOCK_SIZE) * secondResultBlocksNumCandidate) /
                                            (sizeof(size_t) + sizeof(struct INode));

    if (firstResultBlocksNumCandidate == 0) {
        *resultBlocksNum = secondResultBlocksNumCandidate;
        *resultINodesNum = secondResultINodesNumCandidate;
        return;
    }
    if (secondResultBlocksNumCandidate >= (MAX_FS_SIZE / BLOCK_SIZE)) {
        *resultBlocksNum = firstResultBlocksNumCandidate;
        *resultINodesNum = firstResultINodesNumCandidate;
        return;
    }

    double firstINodePerBlock = ((double)firstResultINodesNumCandidate) / firstResultBlocksNumCandidate;
    double secondINodePerBlock = ((double)secondResultINodesNumCandidate) / secondResultBlocksNumCandidate;

    if ((iNodesNumPerBlock - firstINodePerBlock) > (iNodesNumPerBlock - secondINodePerBlock)) {
        *resultBlocksNum = secondResultBlocksNumCandidate;
        *resultINodesNum = secondResultINodesNumCandidate;
    } else {
        *resultBlocksNum = firstResultBlocksNumCandidate;
        *resultINodesNum = firstResultINodesNumCandidate;
    }
}

void initFS() {
    fileSystem.file = fopen(FILE_NAME, "w+b");
    if (fileSystem.file == NULL) {
        sprintf(outData, "can't open the file %s\n", FILE_NAME);
        return;
    }

    strncpy(fileSystem.name, FILE_NAME, strlen(FILE_NAME));

    struct SuperBlock superBlock;

    size_t possibleBlocksNum;
    size_t possibleINodesNum;
    calculate(1.1, 5, 5, &possibleBlocksNum, &possibleINodesNum);

    size_t freeINodeNumbersTableSize = possibleINodesNum * sizeof(size_t);
    size_t iNodeTableSize = possibleINodesNum * sizeof(struct INode);
    size_t freeBlockNumbersTableSize = possibleBlocksNum * sizeof(size_t);

    size_t* freeBlocksTable = (size_t*) malloc(freeBlockNumbersTableSize);
    size_t* freeINodesTable = (size_t*) malloc(freeINodeNumbersTableSize);


    superBlock.blocksNum = 0;
    superBlock.iNodeNum = 0;
    superBlock.possibleBlocksNum = possibleBlocksNum;
    superBlock.possibleINodeNum = possibleINodesNum;
    superBlock.blockSize = BLOCK_SIZE;
    superBlock.iNodeSize = sizeof(struct INode);
    superBlock.freeBlockPointer = sizeof(superBlock);
    superBlock.freeINodePointer = superBlock.freeBlockPointer + freeBlockNumbersTableSize;
    superBlock.iNodePointer = superBlock.freeINodePointer + freeINodeNumbersTableSize;
    superBlock.blocksPointer = superBlock.iNodePointer + iNodeTableSize;

    for (size_t ind = 0; ind < freeBlockNumbersTableSize / sizeof(size_t); ++ind) {
        freeBlocksTable[ind] = ind;
    }
    for (size_t ind = 0; ind < freeINodeNumbersTableSize / sizeof(size_t); ++ind) {
        freeINodesTable[ind] = ind;
    }

    // creating a root directory
    struct INode rootINode;
    rootINode.type = DIRECTORY_TYPE;
    rootINode.linksNum = 1;
    rootINode.size = sizeof(size_t); // first element is number of objects in this directory
    memset(rootINode.blocks, SIZE_MAX, MAX_INODE_BLOCKS * sizeof(size_t));
    rootINode.blocks[0] = 0;
    superBlock.freeINodePointer += sizeof(size_t); // like in freeINodesTable elements
    superBlock.freeBlockPointer += sizeof(size_t); // like in freeBlocksTable elements
    superBlock.iNodeNum++;
    superBlock.blocksNum++;

    void* rootNullBlock = calloc(BLOCK_SIZE, 1);

    fseek(fileSystem.file, 0, SEEK_SET);

    fwrite(&superBlock, sizeof(superBlock), 1, fileSystem.file);
    fwrite(freeBlocksTable, sizeof(size_t), freeBlockNumbersTableSize / sizeof(size_t), fileSystem.file);
    fwrite(freeINodesTable, sizeof(size_t), freeINodeNumbersTableSize / sizeof(size_t), fileSystem.file);
    fwrite(&rootINode, sizeof(rootINode), 1, fileSystem.file);

    // offset for iNodes
    fseek(fileSystem.file, superBlock.blocksPointer, SEEK_SET);
    fwrite(rootNullBlock, BLOCK_SIZE, 1, fileSystem.file);

    struct INode iNode;
    fseek(fileSystem.file, superBlock.iNodePointer, SEEK_SET);
    fread(&iNode, sizeof(iNode), 1, fileSystem.file);

    fflush(fileSystem.file);

    free(freeBlocksTable);
    free(freeINodesTable);
    free(rootNullBlock);
}

void closeFS() {
    fclose(fileSystem.file);
}

size_t getItemFromSuperBlock(enum SuperBlockItem type) {
    fflush(fileSystem.file);

    size_t item;
    fseek(fileSystem.file, type * sizeof(size_t), SEEK_SET); // it gives a possibleBlocksNum
    fread(&item, sizeof(item), 1, fileSystem.file);

    return item;
}

void writeItemIntoSuperBlock(enum SuperBlockItem type, size_t data) {
    fflush(fileSystem.file);

    fseek(fileSystem.file, type * sizeof(size_t), SEEK_SET); // it gives a possibleBlocksNum
    fwrite(&data, sizeof(data), 1, fileSystem.file);
}



size_t resetFreeINodeNum() {
    size_t freeINodeNumPos;
    size_t freeINodeNum;

    fflush(fileSystem.file);
    freeINodeNumPos = getItemFromSuperBlock (FREE_INODE_POINTER_ITEM);

    size_t freeINodeNumPosMax = getItemFromSuperBlock(INODE_POINTER_ITEM) - 1;
    if (freeINodeNumPos >= freeINodeNumPosMax) {
        sprintf(outData, "out of memory (iNodes)\n");
        return SIZE_MAX;
    }

    fseek(fileSystem.file, freeINodeNumPos, SEEK_SET);
    fread(&freeINodeNum, sizeof(freeINodeNum), 1, fileSystem.file);

    // change to next free
    freeINodeNumPos += sizeof(size_t); // like in freeINodesTable elements

    writeItemIntoSuperBlock(FREE_INODE_POINTER_ITEM, freeINodeNumPos);
    writeItemIntoSuperBlock(INODE_NUM_ITEM, getItemFromSuperBlock(INODE_NUM_ITEM) + 1);

    return freeINodeNum;
}

size_t resetFreeBlocksNum() {
    size_t freeBlocksNumPos;
    size_t freeBlocksNum;

    fflush(fileSystem.file);
    freeBlocksNumPos = getItemFromSuperBlock(FREE_BLOCK_POINTER_ITEM);

    size_t freeBlocksNumPosMax = SUPER_BLOCK_ITEM_NUM * sizeof(size_t) +
                                 getItemFromSuperBlock(POSSIBLE_BLOCKS_NUM_ITEM) * sizeof(size_t);
    if (freeBlocksNumPos >= freeBlocksNumPosMax) {
        sprintf(outData,"out of memory (blocks)\n");
        return SIZE_MAX;
    }

    fseek(fileSystem.file, freeBlocksNumPos, SEEK_SET);
    fread(&freeBlocksNum, sizeof(freeBlocksNum), 1, fileSystem.file);

    // change to next free
    freeBlocksNumPos += sizeof(size_t);

    writeItemIntoSuperBlock(FREE_BLOCK_POINTER_ITEM, freeBlocksNumPos);
    writeItemIntoSuperBlock(BLOCKS_NUM_ITEM, getItemFromSuperBlock(BLOCKS_NUM_ITEM) + 1);
    return freeBlocksNum;
}

void deleteINode(size_t newFreeINodeNum) {
    size_t freeINodeNumPos;

    fflush(fileSystem.file);
    freeINodeNumPos = getItemFromSuperBlock (FREE_INODE_POINTER_ITEM);
    freeINodeNumPos -= sizeof(size_t);

    fseek(fileSystem.file, freeINodeNumPos, SEEK_SET);
    fwrite(&newFreeINodeNum, sizeof(newFreeINodeNum), 1, fileSystem.file);

    writeItemIntoSuperBlock(FREE_INODE_POINTER_ITEM, freeINodeNumPos);
    writeItemIntoSuperBlock(INODE_NUM_ITEM, getItemFromSuperBlock(INODE_NUM_ITEM) - 1);
}

void deleteBlock(size_t newFreeBlockNum) {
    size_t freeBlocksNumPos;

    fflush(fileSystem.file);
    freeBlocksNumPos = getItemFromSuperBlock(FREE_BLOCK_POINTER_ITEM);
    freeBlocksNumPos -= sizeof(size_t);

    fseek(fileSystem.file, freeBlocksNumPos, SEEK_SET);
    fwrite(&newFreeBlockNum, sizeof(newFreeBlockNum), 1, fileSystem.file);

    writeItemIntoSuperBlock(FREE_BLOCK_POINTER_ITEM, freeBlocksNumPos);
    writeItemIntoSuperBlock(BLOCKS_NUM_ITEM, getItemFromSuperBlock(BLOCKS_NUM_ITEM) - 1);
}

bool checkFreeBlocksExistence(size_t blocksNum) {
    if (blocksNum > MAX_INODE_BLOCKS) {
        return false;
    }

    size_t freeBlocksNumPos = getItemFromSuperBlock(FREE_BLOCK_POINTER_ITEM);
    size_t possibleBlocksNum = getItemFromSuperBlock(POSSIBLE_BLOCKS_NUM_ITEM);

    fflush(fileSystem.file);

    size_t newOffset = freeBlocksNumPos + (blocksNum - 1) * sizeof(size_t);
    size_t possibleOffset = SUPER_BLOCK_ITEM_NUM * sizeof(size_t) + possibleBlocksNum * sizeof(size_t);
    if (newOffset > possibleOffset) {
        return false;
    } else {
        return true;
    }
}

struct INode readINode(size_t INodeInd) {
    struct INode iNode;
    size_t iNodeOffset;

    fflush(fileSystem.file);

    fseek(fileSystem.file, sizeof(size_t) * 6, SEEK_SET); // it gives a iNodePointer
    fread(&iNodeOffset, sizeof(size_t), 1, fileSystem.file);

    fseek(fileSystem.file, iNodeOffset + sizeof(struct INode) * INodeInd, SEEK_SET);
    fread(&iNode, sizeof(iNode), 1, fileSystem.file);

    return iNode;
}

void writeINode(struct INode* iNode, size_t INodeInd) {
    size_t iNodeOffset;

    fflush(fileSystem.file);

    fseek(fileSystem.file, sizeof(size_t) * 6, SEEK_SET); // it gives a iNodePointer
    fread(&iNodeOffset, sizeof(size_t), 1, fileSystem.file);

    fseek(fileSystem.file, iNodeOffset + sizeof(struct INode) * INodeInd, SEEK_SET);
    fwrite(iNode, sizeof(struct INode), 1, fileSystem.file);
}

size_t OperateWithNameInINode(enum OperateType operation, size_t iNodeInd, const char* name) {
    size_t blockOffset;
    size_t objectRecordNum = SIZE_MAX;

    struct INode iNode = readINode(iNodeInd);

    fflush(fileSystem.file);
    fseek(fileSystem.file, sizeof(size_t) * 7, SEEK_SET); // it gives a blocksPointer
    fread(&blockOffset, sizeof(size_t), 1, fileSystem.file);

    for (size_t iNodeBlocksInd = 0; iNodeBlocksInd < MAX_INODE_BLOCKS; ++iNodeBlocksInd) {
        if (iNode.blocks[iNodeBlocksInd] == SIZE_MAX) {
            if (operation == FIND_NAME) {
                return SIZE_MAX;

            } else if (operation == CREATE_LINK_TO_NAME) {
                iNode.blocks[iNodeBlocksInd] = resetFreeBlocksNum();
                if (iNode.blocks[iNodeBlocksInd] == SIZE_MAX) {
                    return SIZE_MAX;
                }

                objectRecordNum = 0;
                iNode.size += sizeof(objectRecordNum);
                writeINode(&iNode, iNodeInd);
            }
        }

        if (objectRecordNum == SIZE_MAX) {
            size_t objectRecordNumOffset = blockOffset + iNode.blocks[iNodeBlocksInd] * BLOCK_SIZE;
            fseek(fileSystem.file, objectRecordNumOffset, SEEK_SET);
            fread(&objectRecordNum, sizeof(objectRecordNum), 1, fileSystem.file);
        }

        if (operation == CREATE_LINK_TO_NAME) {
            objectRecordNum++;

            if (objectRecordNum * sizeof(struct ObjectRecord) + sizeof(objectRecordNum) > BLOCK_SIZE) {
                objectRecordNum = SIZE_MAX;
                continue;
            }

            struct ObjectRecord newRecord;
            strncpy(newRecord.name, name, MAX_NAME_LENGTH);
            newRecord.iNodeIndex = resetFreeINodeNum();
            if (newRecord.iNodeIndex == SIZE_MAX) {
                return SIZE_MAX;
            }

            size_t objectRecordNumPos = blockOffset + iNode.blocks[iNodeBlocksInd] * BLOCK_SIZE;
            fseek(fileSystem.file, objectRecordNumPos, SEEK_SET);
            fwrite(&objectRecordNum, sizeof(objectRecordNum), 1, fileSystem.file);

            size_t objectRecordPos = objectRecordNumPos + sizeof(objectRecordNum) +
                                     (objectRecordNum - 1) * sizeof(struct ObjectRecord);
            fseek(fileSystem.file, objectRecordPos, SEEK_SET);
            fwrite(&newRecord, sizeof(newRecord), 1, fileSystem.file);

            iNode.size += sizeof(newRecord);
            writeINode(&iNode, iNodeInd);

            return newRecord.iNodeIndex;
        }

        if (objectRecordNum == 0) {
            return SIZE_MAX;
        }

        struct ObjectRecord objectRecord;
        for (size_t objectInd = 0; objectInd < objectRecordNum; ++objectInd) {
            size_t objectRecordNumPos = blockOffset + iNode.blocks[iNodeBlocksInd] * BLOCK_SIZE;
            size_t objectRecordPos = objectRecordNumPos + sizeof(objectRecordNum) + objectInd * sizeof(objectRecord);

            fseek(fileSystem.file, objectRecordPos, SEEK_SET);
            fread(&objectRecord, sizeof(objectRecord), 1, fileSystem.file);

            if (strcmp(name, objectRecord.name) == 0) {
                return objectRecord.iNodeIndex;
            }
        }
    }

    if (operation == CREATE_LINK_TO_NAME) {
        sprintf(outData,"segFault\n");
    }

    return SIZE_MAX;
}

// data for FILE_TYPE
// before this function iNode memory was allocated, but blocks were not allocated
bool createINode(size_t INodeInd, enum FSObjectType type, const void* data, size_t sizeOfData) {
    struct INode newINode = readINode(INodeInd);

    fflush(fileSystem.file);

    newINode.type = type;
    newINode.linksNum = 1;
    if (type == DIRECTORY_TYPE) {
        newINode.size = 0;
        memset(newINode.blocks, SIZE_MAX, MAX_INODE_BLOCKS * sizeof(size_t));
        writeINode(&newINode, INodeInd);
        return true;

    } else if (type == FILE_TYPE) {
        size_t blocksNum = sizeOfData / BLOCK_SIZE ;
        size_t remainder = sizeOfData - blocksNum * BLOCK_SIZE;

        if (!checkFreeBlocksExistence((remainder == 0) ? blocksNum : (blocksNum + 1))) {
            sprintf(outData,"not enough blocks to write file\n");
            return false;
        }

        newINode.size = sizeOfData;
        memset(newINode.blocks, SIZE_MAX, MAX_INODE_BLOCKS * sizeof(size_t));
        for (int blockInd = 0; blockInd < blocksNum; ++blockInd) {
            size_t blockNum = resetFreeBlocksNum();
            newINode.blocks[blockInd] = blockNum;

            size_t blocksBeginPointer = getItemFromSuperBlock(BLOCK_POINTER_ITEM);
            size_t currentBlockPointer = blocksBeginPointer + blockNum * BLOCK_SIZE;

            fseek(fileSystem.file, currentBlockPointer, SEEK_SET);
            fwrite(data + blockInd * BLOCK_SIZE, 1, BLOCK_SIZE, fileSystem.file);

            fflush(fileSystem.file);
        }
        if (remainder > 0) {
            size_t blockNum = resetFreeBlocksNum();
            newINode.blocks[blocksNum] = blockNum;

            size_t blocksBeginPointer = getItemFromSuperBlock(BLOCK_POINTER_ITEM);
            size_t currentBlockPointer = blocksBeginPointer + blockNum * BLOCK_SIZE;

            fseek(fileSystem.file, currentBlockPointer, SEEK_SET);
            fwrite(data + blocksNum * BLOCK_SIZE, 1, remainder, fileSystem.file);

            fflush(fileSystem.file);
        }
        writeINode(&newINode, INodeInd);
        return true;
    }
}

bool actWithObject(const char* path, enum FSObjectType type, enum ActionType actionType,
                   const void* data, size_t dataSize, size_t* pathINodeInd) {
    if (path[0] != '/') {
        sprintf(outData,"unknown path, use '/' \n");
        return false;
    }
    if (path[0] == '/' && path[1] == '\0') {
        sprintf(outData,"you can not create path '/' \n");
        return false;
    }

    size_t currentObjectINodeInd = 0;
    char currentObjectName[MAX_NAME_LENGTH];
    size_t currentDirectoryNameCharInd = 0;

    for (size_t pathCharInd = 1; pathCharInd < strlen(path); ++pathCharInd) {
        if (path[pathCharInd] != '/') {
            currentObjectName[currentDirectoryNameCharInd] = path[pathCharInd];
            currentDirectoryNameCharInd++;
            continue;
        }
        currentObjectName[currentDirectoryNameCharInd] = '\0';
        currentDirectoryNameCharInd = 0;

        size_t newINodeInd = OperateWithNameInINode(FIND_NAME, currentObjectINodeInd, currentObjectName);

        if (newINodeInd == SIZE_MAX) {
            if (actionType == WRITE_ACTION || actionType == GET_INDEXES_ACTION) {
                sprintf(outData,"no such object\n");
                return false;
            }
            newINodeInd = OperateWithNameInINode(CREATE_LINK_TO_NAME, currentObjectINodeInd, currentObjectName);
            if (newINodeInd == SIZE_MAX) {
                return false;
            }

            if (!createINode(newINodeInd, DIRECTORY_TYPE, NULL, 0)) {
                removeRecordFromFather(newINodeInd, currentObjectINodeInd);
                return false;
            }
        }

        currentObjectINodeInd = newINodeInd;
    }
    currentObjectName[currentDirectoryNameCharInd] = '\0';

    size_t foundINodeInd = OperateWithNameInINode(FIND_NAME, currentObjectINodeInd, currentObjectName);


    if (actionType == GET_INDEXES_ACTION) {
        if (foundINodeInd == SIZE_MAX) {
            sprintf(outData,"no such file\n");
            return false;
        }

        pathINodeInd[0] = foundINodeInd;
        pathINodeInd[1] = currentObjectINodeInd;
        return true;

    } else if (actionType == CREATE_ACTION) {
        if (foundINodeInd != SIZE_MAX) {
            sprintf(outData,"directory or file exists\n");
            return false;
        }

        foundINodeInd = OperateWithNameInINode(CREATE_LINK_TO_NAME, currentObjectINodeInd, currentObjectName);
        if (foundINodeInd == SIZE_MAX) {
            return false;
        }

        if (!createINode(foundINodeInd, type, data, dataSize)) {
            removeRecordFromFather(foundINodeInd, currentObjectINodeInd);
            return false;
        }
        sprintf(outData, "OK");

    } else if (actionType == WRITE_ACTION) {
        if (foundINodeInd == SIZE_MAX) {
            sprintf(outData,"no such file\n");
            return false;
        }

        FILE* file;
        if (dataSize != 0) {
            file = fopen((char*) data, "wb");
            if (file == NULL) {
                sprintf(outData,"can't open the file %s\n", (char*) data);
                return false;
            }
        }

        struct INode fileINode = readINode(foundINodeInd);
        if (fileINode.type != FILE_TYPE) {
            return false;
        }

        void* fileData = malloc(fileINode.size + 1);
        size_t blocksNum = fileINode.size / BLOCK_SIZE ;
        size_t remainder = fileINode.size - blocksNum * BLOCK_SIZE;


        for (int blockInd = 0; blockInd < blocksNum; ++blockInd) {
            size_t blocksBeginPointer = getItemFromSuperBlock(BLOCK_POINTER_ITEM);
            size_t currentBlockPointer = blocksBeginPointer + fileINode.blocks[blockInd] * BLOCK_SIZE;

            fseek(fileSystem.file, currentBlockPointer, SEEK_SET);
            fread(fileData + blockInd * BLOCK_SIZE, 1, BLOCK_SIZE, fileSystem.file);

            fflush(fileSystem.file);
        }
        if (remainder > 0) {
            size_t blocksBeginPointer = getItemFromSuperBlock(BLOCK_POINTER_ITEM);
            size_t currentBlockPointer = blocksBeginPointer + fileINode.blocks[blocksNum] * BLOCK_SIZE;

            fseek(fileSystem.file, currentBlockPointer, SEEK_SET);
            fread(fileData + blocksNum * BLOCK_SIZE, 1, remainder, fileSystem.file);

            fflush(fileSystem.file);
        }

        memset(fileData + fileINode.size, '\0', 1);

        if (dataSize == 0) {
            sprintf(outData,"%s: %s\n", path, (char*) fileData);
        } else {
            fprintf(file, "%s", (char*) fileData);
            fclose(file);
            sprintf(outData, "OK\n");
        }
        free(fileData);
    }
    return true;
}

void getInfo() {
    size_t blocksNum = getItemFromSuperBlock(BLOCKS_NUM_ITEM);
    size_t iNodeNum = getItemFromSuperBlock(INODE_NUM_ITEM);
    size_t blockSize = getItemFromSuperBlock(BLOCK_SIZE_ITEM);
    size_t iNodeSize = getItemFromSuperBlock(INODE_SIZE_ITEM);
    size_t freeINodePointer = getItemFromSuperBlock(FREE_INODE_POINTER_ITEM);
    size_t freeBlockPointer = getItemFromSuperBlock(FREE_BLOCK_POINTER_ITEM);
    size_t iNodePointer = getItemFromSuperBlock(INODE_POINTER_ITEM);
    size_t blocksPointer = getItemFromSuperBlock(BLOCK_POINTER_ITEM);
    size_t possibleBlocksNum = getItemFromSuperBlock(POSSIBLE_BLOCKS_NUM_ITEM);
    size_t possibleINodeNum = getItemFromSuperBlock(POSSIBLE_INODE_NUM_ITEM);

    sprintf(outData, "blocksNum: %d\niNodeNum: %d\niNodeSize: %d\nfreeINodePointer: %d\nfreeBlockPointer: %d\n"
                     "iNodePointer: %d\nblocksPointer: %d\npossibleBlocksNum: %d\npossibleINodeNum: %d\n",
            blocksNum, iNodeNum, blockSize, iNodeSize, freeINodePointer, freeBlockPointer, iNodePointer,
            blocksPointer, possibleBlocksNum, possibleINodeNum);

//    printf("blocksNum: %d\n", blocksNum);
//    printf("iNodeNum: %d\n", iNodeNum);
//    printf("blockSize: %d\n", blockSize);
//    printf("iNodeSize: %d\n", iNodeSize);
//    printf("freeINodePointer: %d\n", freeINodePointer);
//    printf("freeBlockPointer: %d\n", freeBlockPointer);
//    printf("iNodePointer: %d\n", iNodePointer);
//    printf("blocksPointer: %d\n", blocksPointer);
//    printf("possibleBlocksNum: %d\n", possibleBlocksNum);
//    printf("possibleINodeNum: %d\n", possibleINodeNum);
}

void removeObject(size_t INodeId) {
    struct INode iNode = readINode(INodeId);
    iNode.linksNum--;

    if (iNode.linksNum > 0) {
        writeINode(&iNode, INodeId);
    }

    for (size_t iNodeBlocksInd = 0; iNodeBlocksInd < MAX_INODE_BLOCKS; ++iNodeBlocksInd) {
        if (iNode.blocks[iNodeBlocksInd] == SIZE_MAX) {
            break;
        }

        size_t blockNum = iNode.blocks[iNodeBlocksInd];
        deleteBlock(blockNum);
    }

    deleteINode(INodeId);
}

void removeRecursion(size_t INodeId) {
    struct ObjectRecord record;
    size_t directoryObjectsNum;
    struct INode iNode = readINode(INodeId);
    if (iNode.type == FILE_TYPE) {
        removeObject(INodeId);
        return;
    }

    for (size_t iNodeBlocksInd = 0; iNodeBlocksInd < MAX_INODE_BLOCKS; ++iNodeBlocksInd) {
        if (iNode.blocks[iNodeBlocksInd] == SIZE_MAX) {
            break;
        }

        size_t blockOffset = getItemFromSuperBlock(BLOCK_POINTER_ITEM);
        size_t offset = blockOffset + (iNode.blocks[iNodeBlocksInd] * BLOCK_SIZE);
        fseek(fileSystem.file, offset, SEEK_SET);
        fread(&directoryObjectsNum, sizeof(directoryObjectsNum), 1, fileSystem.file);

        size_t objectsOffset = offset + sizeof(directoryObjectsNum);
        for (int objectNum = 0; objectNum < directoryObjectsNum; ++objectNum) {
            fseek(fileSystem.file, objectsOffset, SEEK_SET);
            fread(&record, sizeof(record), 1, fileSystem.file);

            removeRecursion(record.iNodeIndex);

            objectsOffset += sizeof(struct ObjectRecord);
        }
    }
    removeObject(INodeId);
}

void printRecursion(size_t INodeId, size_t deep) {
    struct ObjectRecord record;
    size_t directoryObjectsNum;
    struct INode iNode = readINode(INodeId);
    if (iNode.type == FILE_TYPE) {
        sprintf(outData + outDataPos, "file\n");
        outDataPos += 5;
//        printf("file\n");
        for (size_t spaceNum = 0; spaceNum < deep - 1; ++spaceNum) {
            sprintf(outData + outDataPos, "  ");
            outDataPos += 2;
        }
        sprintf(outData + outDataPos, "size: %5d\n", iNode.size);
        outDataPos += 12;
        return;
    }

    sprintf(outData + outDataPos, "directory\n");
    outDataPos += 10;
    for (size_t spaceNum = 0; spaceNum < deep - 1; ++spaceNum) {
        sprintf(outData + outDataPos,"  ");
        outDataPos += 2;
    }
    sprintf(outData + outDataPos,"size: %5d\n", iNode.size);
    outDataPos += 12;
    size_t elementsNum = 0;
    for (size_t iNodeBlocksInd = 0; iNodeBlocksInd < MAX_INODE_BLOCKS; ++iNodeBlocksInd) {
        if (iNode.blocks[iNodeBlocksInd] == SIZE_MAX) {
            break;
        }

        size_t blockOffset = getItemFromSuperBlock(BLOCK_POINTER_ITEM);
        size_t offset = blockOffset + (iNode.blocks[iNodeBlocksInd] * BLOCK_SIZE);
        fseek(fileSystem.file, offset, SEEK_SET);
        fread(&directoryObjectsNum, sizeof(directoryObjectsNum), 1, fileSystem.file);

        elementsNum += directoryObjectsNum;

        size_t objectsOffset = offset + sizeof(directoryObjectsNum);
        for (int objectNum = 0; objectNum < directoryObjectsNum; ++objectNum) {
            fseek(fileSystem.file, objectsOffset, SEEK_SET);
            fread(&record, sizeof(record), 1, fileSystem.file);

            sprintf(outData + outDataPos,"\n");
            outDataPos += 1;
            for (size_t spaceNum = 0; spaceNum < deep; ++spaceNum) {
                sprintf(outData + outDataPos,"  ");
                outDataPos += 2;
            }
            sprintf(outData + outDataPos,"%s | ", record.name);
            outDataPos += strlen(record.name) + 3;

            deep++;
            printRecursion(record.iNodeIndex, deep);
            deep--;

            objectsOffset += sizeof(struct ObjectRecord);
        }
    }

    for (size_t spaceNum = 0; spaceNum < deep - 1; ++spaceNum) {
        sprintf(outData + outDataPos,"  ");
        outDataPos += 2;
    }
}

void swapObjectRecord(size_t firstOffset, size_t secondOffset) {
    struct ObjectRecord first;
    struct ObjectRecord second;

    fseek(fileSystem.file, firstOffset, SEEK_SET);
    fread(&first, sizeof(first), 1, fileSystem.file);

    fseek(fileSystem.file, secondOffset, SEEK_SET);
    fread(&second, sizeof(second), 1, fileSystem.file);

    fseek(fileSystem.file, firstOffset, SEEK_SET);
    fwrite(&second, sizeof(second), 1, fileSystem.file);

    fseek(fileSystem.file, secondOffset, SEEK_SET);
    fwrite(&first, sizeof(first), 1, fileSystem.file);
}

void removeRecordFromFather(size_t thisPathInd, size_t fatherPathInd) {
    struct INode fatherINode = readINode(fatherPathInd);
    struct ObjectRecord record;
    size_t blockObjectsNum;
    size_t recordToDeleteOffset = SIZE_MAX;
    size_t lastRecordOffset = SIZE_MAX;
    size_t lastINodeBlocksInd;

    for (size_t iNodeBlocksInd = 0; iNodeBlocksInd < MAX_INODE_BLOCKS; ++iNodeBlocksInd) {
        if (fatherINode.blocks[iNodeBlocksInd] == SIZE_MAX) {
            break;
        }

        size_t blockOffset = getItemFromSuperBlock(BLOCK_POINTER_ITEM);
        size_t offset = blockOffset + (fatherINode.blocks[iNodeBlocksInd] * BLOCK_SIZE);
        fseek(fileSystem.file, offset, SEEK_SET);
        fread(&blockObjectsNum, sizeof(blockObjectsNum), 1, fileSystem.file);

        size_t objectsOffset = offset + sizeof(blockObjectsNum);
        for (int objectNum = 0; objectNum < blockObjectsNum; ++objectNum) {
            fseek(fileSystem.file, objectsOffset, SEEK_SET);
            fread(&record, sizeof(record), 1, fileSystem.file);

            if (record.iNodeIndex == thisPathInd) {
                recordToDeleteOffset = objectsOffset;
            }
            lastRecordOffset = objectsOffset;

            objectsOffset += sizeof(struct ObjectRecord);
        }
        lastINodeBlocksInd = iNodeBlocksInd;
    }
    if (recordToDeleteOffset == SIZE_MAX || lastRecordOffset == SIZE_MAX) {
        sprintf(outData,"fatal error\n");
        return;
    }

    // deleting record
    if (recordToDeleteOffset != lastRecordOffset) {
        swapObjectRecord(recordToDeleteOffset, lastRecordOffset);
    }

    blockObjectsNum--;
    size_t blockOffset = getItemFromSuperBlock(BLOCK_POINTER_ITEM);
    size_t offset = blockOffset + (fatherINode.blocks[lastINodeBlocksInd] * BLOCK_SIZE);
    fseek(fileSystem.file, offset, SEEK_SET);
    fwrite(&blockObjectsNum, sizeof(blockObjectsNum), 1, fileSystem.file);
    fatherINode.size -= sizeof(record);

    // it means directory is empty
    if (fatherINode.size == sizeof(size_t)) {
        deleteBlock(fatherINode.blocks[0]);
        fatherINode.blocks[0] = SIZE_MAX;
        fatherINode.size = 0;
    }
    writeINode(&fatherINode, fatherPathInd);
}

void removeName(const char* path) {
    size_t pathINodeInd[2];

    if (strcmp(path, "/") == 0) {
        sprintf(outData,"you can not delete /\n");
    }

    bool success = actWithObject(path, ANY_TYPE, GET_INDEXES_ACTION, NULL, 0, pathINodeInd);
    if (!success) {
        return;
    }

    size_t thisPathInd = pathINodeInd[0];
    size_t fatherPathInd = pathINodeInd[1];

    removeRecursion(thisPathInd);

    removeRecordFromFather(thisPathInd, fatherPathInd);
    sprintf(outData,"OK\n");
}

bool printDir(const char* path) {
    size_t pathINodeInd[2];

    if (strcmp(path, "/") == 0) {
        sprintf(outData + outDataPos, "%s | ", path);
        outDataPos += strlen(path) + 3;

        printRecursion(0, 1);
        sprintf(outData + outDataPos, "\n");
        outDataPos = 0;
        return true;
    }

    bool success = actWithObject(path, ANY_TYPE, GET_INDEXES_ACTION, NULL, 0, pathINodeInd);
    if (!success) {
        return success;
    }

    size_t thisPathInd = pathINodeInd[0];

    struct INode fileINode = readINode(thisPathInd);
    if (fileINode.type != DIRECTORY_TYPE) {
        sprintf(outData, "it is not a directory\n");
        return false;
    }

    sprintf(outData + outDataPos, "%s | ", path);
    outDataPos += strlen(path) + 3;

    printRecursion(thisPathInd, 1);
    sprintf(outData + outDataPos,"\n");
    outDataPos += 1;

    sprintf(outData + outDataPos, '\0');
    outDataPos = 0;
    return true;
}

bool createFileInFS(const char* fileName, const char* fsFileName) {
    FILE* file = fopen(fileName, "rb");

    if (file == NULL) {
        sprintf(outData, "can't open the file %s\n", fileName);
        return false;
    }

    fseek(file, 0, SEEK_END);
    size_t fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (fileSize > BLOCK_SIZE * MAX_INODE_BLOCKS) {
        sprintf(outData,"file is too large file size: %d, maximum: %d\n", fileSize, BLOCK_SIZE * MAX_INODE_BLOCKS);
        return false;
    }

    char* data = malloc(fileSize);

    for (size_t dataInd = 0; dataInd < fileSize; ++dataInd) {
        data[dataInd] = (char) fgetc(file);
    }

    fclose(file);
    return actWithObject(fsFileName, FILE_TYPE, CREATE_ACTION, data, fileSize, NULL);
}
