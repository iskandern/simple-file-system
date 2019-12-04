#include <stdio.h>
#include <malloc.h>
#include <mem.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef FS_FILE_SYSTEM_H
#define FS_FILE_SYSTEM_H

#define FILE_NAME "fs.txt"
#define MAX_FS_SIZE 4084
#define BLOCK_SIZE 512
#define MAX_NAME_LENGTH 32
#define MAX_INODE_BLOCKS 8


enum FSObjectType {
    ANY_TYPE,
    FILE_TYPE,
    DIRECTORY_TYPE
};

struct INode {
    enum FSObjectType type;
    size_t size;
    size_t linksNum;
    size_t blocks[MAX_INODE_BLOCKS];
};

// don't touch the superBlock!
struct SuperBlock {
    size_t blocksNum;
    size_t iNodeNum;
    size_t blockSize;
    size_t iNodeSize;
    size_t freeINodePointer;
    size_t freeBlockPointer;
    size_t iNodePointer;
    size_t blocksPointer;
    size_t possibleBlocksNum;
    size_t possibleINodeNum;
};

#define SUPER_BLOCK_ITEM_NUM 10
enum SuperBlockItem {
    BLOCKS_NUM_ITEM = 0,
    INODE_NUM_ITEM = 1,
    BLOCK_SIZE_ITEM = 2,
    INODE_SIZE_ITEM = 3,
    FREE_INODE_POINTER_ITEM = 4,
    FREE_BLOCK_POINTER_ITEM = 5,
    INODE_POINTER_ITEM = 6,
    BLOCK_POINTER_ITEM = 7,
    POSSIBLE_BLOCKS_NUM_ITEM = 8,
    POSSIBLE_INODE_NUM_ITEM = 9
};

struct ObjectRecord {
    size_t iNodeIndex;
    char name[MAX_NAME_LENGTH];
};

struct FileSystem {
    FILE* file;
    char name[MAX_NAME_LENGTH];
};

struct FileSystem fileSystem;

void initFS();

void closeFS();

size_t getItemFromSuperBlock(enum SuperBlockItem type);

void writeItemIntoSuperBlock(enum SuperBlockItem type, size_t data);

size_t resetFreeINodeNum();

size_t resetFreeBlocksNum();

void deleteINode(size_t newFreeINodeNum);

void deleteBlock(size_t newFreeBlockNum);

bool checkFreeBlocksExistence(size_t blocksNum);

struct INode readINode(size_t INodeInd);

void writeINode(struct INode* iNode, size_t INodeInd);

enum OperateType {
    FIND_NAME,
    CREATE_LINK_TO_NAME,
};
size_t OperateWithNameInINode(enum OperateType operation, size_t INodeInd, const char* name);

bool createINode(size_t INodeInd, enum FSObjectType type, const void* data, size_t sizeOfData);

enum ActionType {
    CREATE_ACTION,
    WRITE_ACTION,
    GET_INDEXES_ACTION
};
bool actWithObject(const char* path, enum FSObjectType type, enum ActionType actionType,
                   const void* data, size_t dataSize, size_t* pathINodeInd);

void getInfo();

void removeObject(size_t INodeId);

void removeRecursion(size_t INodeId);

void printRecursion(size_t INodeId, size_t deep);

void removeRecordFromFather(size_t thisPathInd, size_t fatherPathInd);

void removeName(const char* path);

bool printDir(const char* path);

bool createFileInFS(const char* fileName, const char* fsFileName);

void calculate (double iNodesNumPerBlock, size_t minBlockNum, size_t minINodeNum,
                size_t* resultBlocksNum, size_t* resultINodesNum);

#endif //FS_FILE_SYSTEM_H
