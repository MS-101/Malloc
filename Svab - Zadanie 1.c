#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct memoryBlock_Header {
    unsigned short int size;
    //last bite of size determines whether the block is free or not
    //0 = free
    //1 = occupied
    unsigned short int prev;
    unsigned short int prevFromList;
    unsigned short int nextFromList;
} memoryBlock_Header;

void *memoryStart;

int prevPowerOf2(unsigned long int number) {
    int counter;

    counter = 0;

    while (number > 0) {
        number >>= 1;
        counter++;
    }

    counter--;

    number = 1 << counter;

    return number;
}

int nextPowerOf2(unsigned long int number) {
    int counter;

    counter = 0;

    while (number > 0) {
        number >>= 1;
        counter++;
    }

    number = 1 << counter;

    return number;
}

int logBaseOf2(unsigned long int number) {
    int counter;

    counter = 0;

    while (number > 0) {
        number >>= 1;
        counter++;
    }

    counter--;

    return counter;
}

int getAmountOfDataLists(int size) {
    return logBaseOf2(prevPowerOf2(size - sizeof(short int) - sizeof(memoryBlock_Header)));
}

void *memory_alloc(unsigned int size) {
    printf("ALLOCATING MEMORY(%d)\n\n", size);

    void *ptr, *headerReader;
    int i, fragmentedBytes;
    int curList, numberOfDataLists, memoryOffset;
    int memorySize, realMemorySize, recommendedSize, realSize, remainingSize;

    if (prevPowerOf2(size) == size) {
        recommendedSize = size;
    } else {
        recommendedSize = nextPowerOf2(size);
    }

    if (recommendedSize == 1) {
        recommendedSize = 2;
    }

    headerReader = memoryStart;

    memorySize = *(unsigned short int *)headerReader;
    numberOfDataLists = getAmountOfDataLists(memorySize);
    realMemorySize = memorySize - (numberOfDataLists + 1) * sizeof(short int);
    realSize = recommendedSize + sizeof(memoryBlock_Header);

    if (realMemorySize < recommendedSize + sizeof(memoryBlock_Header)) {
        printf("ERROR: Requested memory block is too large for the memory capacity!\n");
        printf("\n");
        printf("Additional error information:\n");
        printf("memory size = %d\n", memorySize);
        printf("real memory size = %d\n", realMemorySize);
        printf("requested memory size = %d\n", size);
        printf("requested real memory size = %d\n", realSize);
        printf("\n");

        return NULL;
    }

    curList = logBaseOf2(recommendedSize);

    headerReader += curList * sizeof(short int);

    if (*(short unsigned int *)headerReader != 0) {
        //allocating memory to a data block of equally large size
        printf("Allocating %d B of data in equally large data block.\n", recommendedSize);
        printf("\n");

        memoryOffset = *(short unsigned int*)headerReader;

        ptr = memoryStart + memoryOffset;

        (*(memoryBlock_Header *)ptr).size ^= 1;
        (*(memoryBlock_Header *)ptr).prevFromList = 0;
        (*(memoryBlock_Header *)ptr).nextFromList = 0;

        *(short unsigned int *)headerReader = (*(memoryBlock_Header *)ptr).nextFromList;

        ptr += sizeof(memoryBlock_Header);

        return ptr;
    } else {
        curList++;
        headerReader += sizeof(short int);

        while (curList <= numberOfDataLists) {
            if (*(short int *)headerReader != 0) {
                memoryOffset = *(short unsigned int *)headerReader;

                ptr = memoryStart + memoryOffset;

                *(short int*)headerReader = (*(memoryBlock_Header *)ptr).nextFromList;

                (*(memoryBlock_Header *)ptr).nextFromList = 0;

                if ((*(memoryBlock_Header *)ptr).size - recommendedSize - sizeof(memoryBlock_Header) < 2) {
                    //allocating memory to a data block of larger size that cannot be divided
                    printf("Allocating %d B of data in %d B data block without dividing it.\n", recommendedSize, 1 << curList);
                    printf("\n");

                    (*(memoryBlock_Header *)ptr).size ^= 1;

                    ptr += sizeof(memoryBlock_Header);

                    return ptr;
                } else {
                    //allocating memory to a data block of larger size that can be divided
                    printf("Allocating %d B of data in %d B data block dividing it into smaller pieces.\n", recommendedSize, 1 << curList);
                    printf("\n");

                    remainingSize = (*(memoryBlock_Header *)ptr).size;

                    (*(memoryBlock_Header *)ptr).size = recommendedSize ^ 1;

                    remainingSize -= recommendedSize;

                    while (remainingSize >= 2 + sizeof(memoryBlock_Header)) {
                        ptr += sizeof(memoryBlock_Header) + recommendedSize;

                        (*(memoryBlock_Header *)ptr).prev = ptr - memoryStart - recommendedSize - sizeof(memoryBlock_Header);
                        (*(memoryBlock_Header *)ptr).prevFromList = 0;

                        remainingSize -= sizeof(memoryBlock_Header);
                        recommendedSize = prevPowerOf2(remainingSize);

                        (*(memoryBlock_Header *)ptr).size = recommendedSize;

                        curList = logBaseOf2(recommendedSize);
                        headerReader = memoryStart + curList * sizeof(short int);
                        (*(memoryBlock_Header *)ptr).nextFromList = *(short unsigned int *)headerReader;
                        *(short unsigned int *)headerReader = ptr - memoryStart;

                        remainingSize -= recommendedSize;
                    }

                    //fragmentation happens here
                    fragmentedBytes = 0;
                    ptr += sizeof(memoryBlock_Header) + recommendedSize;
                    while (remainingSize > 0) {
                        *(char *)ptr = 0;
                        ptr++;
                        fragmentedBytes++;
                        remainingSize--;
                    }

                    while (*(char *)ptr == 0 && ptr < memoryStart + memorySize) {
                        fragmentedBytes++;
                        ptr++;
                    }

                    if (ptr < memoryStart + memorySize) {
                        (*(memoryBlock_Header *)ptr).prev = ptr - memoryStart - fragmentedBytes - recommendedSize - sizeof(memoryBlock_Header);
                    }

                    ptr = memoryStart + memoryOffset + sizeof(memoryBlock_Header);

                    return ptr;
                }
            }

            curList++;
            headerReader += sizeof(short int);
        }

        printf("ERROR: There is no available memory for the requested memory block!\n");
        printf("\n");
        printf("Additional error information:\n");
        printf("requested memory size = %d\n", size);
        printf("recommended memory size = %d\n", recommendedSize);
        printf("available memory lists =");

        ptr = memoryStart;

        for (i = 1; i <= numberOfDataLists; i++) {
            ptr += sizeof(short int);
            if (*(short int *)ptr != 0) {
                printf(" %d", 1 << i);
            }
        }
        printf("\n");
        printf("\n");

        return NULL;
    }
}

int memory_check(void *ptr) {
    ptr -= sizeof(memoryBlock_Header);

    if (((*(memoryBlock_Header *)ptr).size & 1) == 0) {
        return 0;
    } else {
        return 1;
    }
}

int memory_free(void *valid_ptr) {
    printf("FREEING MEMORY\n\n");

    void *nextPtr, *curPtr, *headerReader;
    void *leftBorder, *rightBorder, *memoryBorder;
    int dataSize, curList, memorySize, size, recommendedSize, prev;
    memoryBlock_Header curDataBlock;

    memorySize = *(short unsigned int *)memoryStart;
    memoryBorder = memoryStart + memorySize;

    valid_ptr -= sizeof(memoryBlock_Header);
    *(short unsigned int *)valid_ptr &= ~1;

    //i calculated left and right border of the newly acquired free space (fragmentation is also taken into account)
    //during the calculation i removed free blocks from their lists

    //checking left of ptr

    curPtr = valid_ptr;
    leftBorder = valid_ptr;

    while (1) {
        if ((*(memoryBlock_Header *)curPtr).prev != 0) {
            nextPtr = memoryStart + (*(memoryBlock_Header *)curPtr).prev;

            if ((*(short unsigned int *)nextPtr & 1) == 0) {
                leftBorder = nextPtr;

                if ((*(memoryBlock_Header *)leftBorder).prevFromList != 0) {
                    curPtr = memoryStart + (*(memoryBlock_Header *)leftBorder).prevFromList;
                    (*(memoryBlock_Header *)curPtr).nextFromList = (*(memoryBlock_Header *)leftBorder).nextFromList;
                } else {
                    dataSize = (*(memoryBlock_Header *)nextPtr).size;

                    curList = logBaseOf2(dataSize);
                    curPtr = memoryStart + curList * sizeof(short int);

                    *(short unsigned int *)curPtr = (*(memoryBlock_Header *)nextPtr).nextFromList;
                }

                if ((*(memoryBlock_Header *)leftBorder).nextFromList != 0) {
                    curPtr = memoryStart + (*(memoryBlock_Header *)leftBorder).nextFromList;
                    (*(memoryBlock_Header *)curPtr).prevFromList = (*(memoryBlock_Header *)leftBorder).prevFromList;
                }
            } else {
                break;
            }

            curPtr = nextPtr;
        } else {
            break;
        }
    }

    //checking right of ptr

    curPtr = valid_ptr;
    rightBorder = valid_ptr + sizeof(memoryBlock_Header) + (*(memoryBlock_Header *)valid_ptr).size;

    while (1) {
        nextPtr = curPtr + sizeof(memoryBlock_Header) + (int)((*(memoryBlock_Header *)curPtr).size & ~1);

        while (*(char *)nextPtr == 0 && nextPtr < memoryBorder) {
            nextPtr++;
            rightBorder++;
        }

        if (nextPtr < memoryBorder) {
            if ((*(short unsigned int *)nextPtr & 1) == 0) {
                rightBorder = nextPtr + sizeof(memoryBlock_Header) + (*(memoryBlock_Header *)nextPtr).size;

                if ((*(memoryBlock_Header *)nextPtr).prevFromList != 0) {
                    curPtr = memoryStart + (*(memoryBlock_Header *)nextPtr).prevFromList;
                    (*(memoryBlock_Header *)curPtr).nextFromList = (*(memoryBlock_Header *)nextPtr).nextFromList;
                } else {
                    dataSize = (*(memoryBlock_Header *)nextPtr).size;

                    curList = logBaseOf2(dataSize);
                    curPtr = memoryStart + curList * sizeof(short int);

                    *(short unsigned int *)curPtr = (*(memoryBlock_Header *)nextPtr).nextFromList;
                }

                if ((*(memoryBlock_Header *)nextPtr).nextFromList != 0) {
                    curPtr = memoryStart + (*(memoryBlock_Header *)nextPtr).nextFromList;
                    (*(memoryBlock_Header *)curPtr).prevFromList = (*(memoryBlock_Header *)nextPtr).prevFromList;
                }
            } else {
                break;
            }
        } else {
            break;
        }

        curPtr = nextPtr;
    }

    //now i need to create the largest possible free data blocks in the space designated by left and right border

    size = rightBorder - leftBorder;
    prev = (*(memoryBlock_Header *)leftBorder).prev;
    curDataBlock.prevFromList = 0;
    curPtr = leftBorder;

    while (size >= sizeof(memoryBlock_Header) + 2) {
        size -= sizeof(memoryBlock_Header);

        recommendedSize = prevPowerOf2(size);

        curDataBlock.size = recommendedSize;

        curList = logBaseOf2(curDataBlock.size);

        curDataBlock.prev = prev;

        headerReader = memoryStart + curList * sizeof(short int);

        if (*(short unsigned int *)headerReader != 0) {
            nextPtr = memoryStart + *(short unsigned int *)headerReader;
        } else {
            nextPtr = 0;
        }

        if (memoryStart + *(short unsigned int *)headerReader != curPtr) {
            curDataBlock.nextFromList = *(short unsigned int *)headerReader;
            if (nextPtr != 0) {
                (*(memoryBlock_Header *)nextPtr).prevFromList = curPtr - memoryStart;
            }
        } else {
            curDataBlock.nextFromList = 0;
        }

        *(short unsigned int *)headerReader = curPtr - memoryStart;

        prev = curPtr - memoryStart;

        *(memoryBlock_Header *)curPtr = curDataBlock;
        size -= curDataBlock.size;
        curPtr += sizeof(memoryBlock_Header) + curDataBlock.size;
    }

    //fragmentation happens here
    while (size > 0) {
        *(char *)curPtr = 0;
        curPtr++;
        size--;
    }

    //dataBlock following the free space is redirected to the correct previous dataBlock

    if (rightBorder < memoryBorder) {
        (*(memoryBlock_Header *)curPtr).prev = prev;
    }

    return 0;
}

void memory_init(void *ptr, unsigned int size) {
    printf("INITIALIZING MEMORY\n\n");

    int i;
    void *headerReader;

    //main memory header creation

    *(int *)ptr = size;

    short unsigned int numberOfDataLists;

    numberOfDataLists = getAmountOfDataLists(size);

    size -= sizeof(short int);
    size -= numberOfDataLists * sizeof(short int);
    ptr += sizeof(short int);

    for (i = 0; i < numberOfDataLists; i++) {
        *(int *)ptr = 0;
        ptr += sizeof(short int);
    }

    //memory segregation

    memoryBlock_Header curDataBlock;
    short unsigned int prev = 0;
    short unsigned int nextFromList;
    short unsigned int curList;
    short unsigned int recommendedSize;

    while (size >= sizeof(memoryBlock_Header) + 2) {
        size -= sizeof(memoryBlock_Header);

        recommendedSize = prevPowerOf2(size);

        curDataBlock.size = recommendedSize;

        curList = logBaseOf2(curDataBlock.size);

        curDataBlock.prev = prev;
        curDataBlock.prevFromList = 0;

        headerReader = memoryStart + curList * sizeof(short int);

        curDataBlock.nextFromList = *(short unsigned int *)headerReader;

        *(short unsigned int *)headerReader = ptr - memoryStart;

        prev = ptr - memoryStart;

        *(memoryBlock_Header *)ptr = curDataBlock;
        size -= curDataBlock.size;
        ptr += sizeof(memoryBlock_Header) + curDataBlock.size;
    }

    //fragmentation happens here
    while (size > 0) {
        *(char *)ptr = 0;
        ptr++;
        size--;
    }
}

void print_memory() {
    printf("PRINTING MEMORY\n\n");
    int i, j, fragmentedBytes;
    short unsigned int size, numberOfDataLists, memoryOffset;
    void *ptr = memoryStart;

    printf("Main memory header:\n");
    printf("===================\n");

    size = *(int *)ptr;
    printf("Size: %hu\n", size);
    ptr += sizeof(short int);

    numberOfDataLists = getAmountOfDataLists(size);
    for (i = 1; i <= numberOfDataLists; i++) {
        printf("2^%d bytes: %hu\n", i, *(short unsigned int *)ptr);
        ptr += sizeof(short int);
    }

    printf("\nLists of free data blocks:\n");
    printf("==========================\n");

    for (i = 0; i < numberOfDataLists; i++) {
        ptr = memoryStart + sizeof(short int);
        for (j = 0; j < i; j++) {
            ptr += sizeof(short int);
        }

        memoryOffset = *(short int *)ptr;

        if (memoryOffset != 0) {
            printf("\nMemory list - 2^%d bytes:\n", i + 1);
        }

        while (memoryOffset != 0) {
            ptr = memoryStart + memoryOffset;

            printf("\nData block\n");
            printf("size: %hu\n", (*(memoryBlock_Header *)ptr).size);
            printf("prev: %hu\n", (*(memoryBlock_Header *)ptr).prev);
            printf("prevFromList: %hu\n", (*(memoryBlock_Header *)ptr).prevFromList);
            printf("nextFromList: %hu\n", (*(memoryBlock_Header *)ptr).nextFromList);

            memoryOffset = (*(memoryBlock_Header *)ptr).nextFromList;
        }
    }

    printf("\nAll data blocks:\n");
    printf("==========================\n");

    ptr = memoryStart + (numberOfDataLists + 1) * sizeof(short int);
    size -= (numberOfDataLists + 1) * sizeof(short int);
    while (size > 0) {
        fragmentedBytes = 0;
        while (*(char *)ptr == 0 && size > 0) {
            fragmentedBytes++;
            ptr++;
            size--;
        }

        if (fragmentedBytes == 0) {
            printf("\nData block\n");
            printf("isOccupied: %d\n", (*(memoryBlock_Header *)ptr).size & 1);
            printf("size: %hu\n", (*(memoryBlock_Header *)ptr).size & ~1);
            printf("prev: %hu\n", (*(memoryBlock_Header *)ptr).prev);
            printf("prevFromList: %hu\n", (*(memoryBlock_Header *)ptr).prevFromList);
            printf("nextFromList: %hu\n", (*(memoryBlock_Header *)ptr).nextFromList);

            size -= sizeof(memoryBlock_Header) + (*(memoryBlock_Header *)ptr).size & ~1;
            ptr += sizeof(memoryBlock_Header) + (*(memoryBlock_Header *)ptr).size & ~1;
        } else {
            printf("\nFragmented bytes: %d\n", fragmentedBytes);
        }
    }

    printf("\n");
}

void test1() {
    int datablockSize;
    char command;
    char myMemory1[50], myMemory2[100], myMemory3[150];
    char *buf1, *buf2, *buf3, *buf4, *buf5, *buf6;

    do {
        system("cls");

        printf("TEST 1:\n");
        printf("\n");

        printf("Input one of the commands for the required memory size:\n");
        printf("1 - 50 B memory\n");
        printf("2 - 100 B memory\n");
        printf("3 - 150 B memory\n");
        printf("\n");

        printf("INPUT COMMAND: ");
        command = getchar();
    } while (command < '1' || command > '3');
    printf("\n");

    do {
        printf("INPUT DATA BLOCK SIZE (8 - 24 B): ");
        scanf("%d", &datablockSize);

        if (datablockSize < 8 || datablockSize > 24) {
            printf("Data block size must be within the given range!\n");
            printf("\n");
        }
    } while (datablockSize < 8 || datablockSize > 24);
    printf("\n");

    switch (command) {
        case '1':
            memoryStart = myMemory1;

            memory_init(memoryStart, 50);
            print_memory();

            buf1 = memory_alloc(datablockSize);
            buf2 = memory_alloc(datablockSize);
            print_memory();

            if (buf1 != NULL) {
                memory_free(buf1);
            }
            if (buf2 != NULL) {
                memory_free(buf2);
            }
            print_memory();

            buf1 = memory_alloc(datablockSize);
            buf2 = memory_alloc(datablockSize);
            print_memory();
            break;
        case '2':
            memoryStart = myMemory2;

            memory_init(memoryStart, 100);
            print_memory();

            buf1 = memory_alloc(datablockSize);
            print_memory();
            buf2 = memory_alloc(datablockSize);
            print_memory();
            buf3 = memory_alloc(datablockSize);
            buf4 = memory_alloc(datablockSize);
            print_memory();

            if (buf1 != NULL) {
                memory_free(buf1);
            }
            if (buf2 != NULL) {
                memory_free(buf2);
            }
            if (buf3 != NULL) {
                memory_free(buf3);
            }
            if (buf4 != NULL) {
                memory_free(buf4);
            }
            print_memory();

            buf1 = memory_alloc(datablockSize);
            buf2 = memory_alloc(datablockSize);
            buf3 = memory_alloc(datablockSize);
            buf4 = memory_alloc(datablockSize);
            print_memory();
            break;
        case '3':
            memoryStart = myMemory3;

            memory_init(memoryStart, 150);
            print_memory();

            buf1 = memory_alloc(datablockSize);
            buf2 = memory_alloc(datablockSize);
            buf3 = memory_alloc(datablockSize);
            buf4 = memory_alloc(datablockSize);
            buf5 = memory_alloc(datablockSize);
            buf6 = memory_alloc(datablockSize);
            print_memory();

            if (buf1 != NULL) {
                memory_free(buf1);
            }
            if (buf2 != NULL) {
                memory_free(buf2);
            }
            if (buf3 != NULL) {
                memory_free(buf3);
            }
            if (buf4 != NULL) {
                memory_free(buf4);
            }
            if (buf5 != NULL) {
                memory_free(buf5);
            }
            if (buf6 != NULL) {
                memory_free(buf6);
            }
            print_memory();

            buf1 = memory_alloc(datablockSize);
            buf2 = memory_alloc(datablockSize);
            buf3 = memory_alloc(datablockSize);
            buf4 = memory_alloc(datablockSize);
            buf5 = memory_alloc(datablockSize);
            buf6 = memory_alloc(datablockSize);
            print_memory();
            break;
    }

    printf("PRESS ANY KEY TO CONTINUE.\n");
    getchar();
    getchar();
}

void test2() {
    char command;
    char myMemory1[50], myMemory2[100], myMemory3[150];
    char *buf1, *buf2, *buf3, *buf4, *buf5, *buf6;
    int datablockSize1, datablockSize2, datablockSize3, datablockSize4, datablockSize5, datablockSize6;
    int upper, lower;

    do {
        system("cls");

        printf("TEST 2:\n");
        printf("\n");

        printf("Input one of the commands for the required memory size:\n");
        printf("1 - 50 B memory\n");
        printf("2 - 100 B memory\n");
        printf("3 - 150 B memory\n");
        printf("\n");

        printf("INPUT COMMAND: ");
        command = getchar();
    } while (command < '1' || command > '3');
    printf("\n");

    upper = 24;
    lower = 8;
    srand(time(0));

    datablockSize1 = (rand() % (upper - lower + 1)) + lower;
    datablockSize2 = (rand() % (upper - lower + 1)) + lower;
    datablockSize3 = (rand() % (upper - lower + 1)) + lower;
    datablockSize4 = (rand() % (upper - lower + 1)) + lower;
    datablockSize5 = (rand() % (upper - lower + 1)) + lower;
    datablockSize6 = (rand() % (upper - lower + 1)) + lower;

    switch (command) {
        case '1':
            memoryStart = myMemory1;

            memory_init(memoryStart, 50);
            print_memory();

            buf1 = memory_alloc(datablockSize1);
            buf2 = memory_alloc(datablockSize2);
            print_memory();

            if (buf1 != NULL) {
                memory_free(buf1);
            }
            if (buf2 != NULL) {
                memory_free(buf2);
            }
            print_memory();

            buf1 = memory_alloc(datablockSize1);
            buf2 = memory_alloc(datablockSize2);
            print_memory();
            break;
        case '2':
            memoryStart = myMemory2;

            memory_init(memoryStart, 100);
            print_memory();

            buf1 = memory_alloc(datablockSize1);
            buf2 = memory_alloc(datablockSize2);
            buf3 = memory_alloc(datablockSize3);
            buf4 = memory_alloc(datablockSize4);
            print_memory();

            if (buf1 != NULL) {
                memory_free(buf1);
            }
            if (buf2 != NULL) {
                memory_free(buf2);
            }
            if (buf3 != NULL) {
                memory_free(buf3);
            }
            if (buf4 != NULL) {
                memory_free(buf4);
            }
            print_memory();

            buf1 = memory_alloc(datablockSize1);
            buf2 = memory_alloc(datablockSize2);
            buf3 = memory_alloc(datablockSize3);
            buf4 = memory_alloc(datablockSize4);
            print_memory();
            break;
        case '3':
            memoryStart = myMemory3;

            memory_init(memoryStart, 150);
            print_memory();

            buf1 = memory_alloc(datablockSize1);
            buf2 = memory_alloc(datablockSize2);
            buf3 = memory_alloc(datablockSize3);
            buf4 = memory_alloc(datablockSize4);
            buf5 = memory_alloc(datablockSize5);
            buf6 = memory_alloc(datablockSize6);
            print_memory();

            if (buf1 != NULL) {
                memory_free(buf1);
            }
            if (buf2 != NULL) {
                memory_free(buf2);
            }
            if (buf3 != NULL) {
                memory_free(buf3);
            }
            if (buf4 != NULL) {
                memory_free(buf4);
            }
            if (buf5 != NULL) {
                memory_free(buf5);
            }
            if (buf6 != NULL) {
                memory_free(buf6);
            }
            print_memory();

            buf1 = memory_alloc(datablockSize1);
            buf2 = memory_alloc(datablockSize2);
            buf3 = memory_alloc(datablockSize3);
            buf4 = memory_alloc(datablockSize4);
            buf5 = memory_alloc(datablockSize5);
            buf6 = memory_alloc(datablockSize6);
            print_memory();
            break;
    }

    printf("PRESS ANY KEY TO CONTINUE.\n");
    getchar();
    getchar();
}

void test3() {
    char myMemory1[50000];
    int upper, lower;


    system("cls");

    printf("TEST 2:\n");
    printf("\n");
    print_memory();

    printf("PRESS ANY KEY TO CONTINUE.\n");
    getchar();
    getchar();
}

void test4() {

}

int main() {
    char command;

    while (1) {
        system("cls");
        printf("Input one of the commands for the corresponding test:\n");
        printf("1 - 50/100/200 B memory, 8 - 24 B data blocks of the same size\n");
        printf("2 - 50/100/200 B memory, 8 - 24 B data blocks of different sizes\n");
        printf("3 - 50000 B memory, 500 - 5000 B data blocks of different sizes\n");
        printf("4 - 50000 B memory, 8 - 5000 B data blocks of different sizes\n");
        printf("\n");

        printf("INPUT COMMAND: ");
        command = getchar();

        switch (command) {
            case '1':
                test1();
                break;
            case '2':
                test2();
                break;
            case '3':
                test3();
                break;
            case '4':
                test4();
                break;
        }
    }

    return 0;
}
