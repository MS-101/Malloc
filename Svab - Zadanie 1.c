#include <stdio.h>
#include <unistd.h>
#include <math.h>

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
    int i;
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

        return NULL;
    }

    curList = logBaseOf2(recommendedSize);

    headerReader += curList * sizeof(short int);

    if (*(short unsigned int *)headerReader != 0) {
        //allocating memory to a data block of equally large size
        memoryOffset = *(short unsigned int*)headerReader;

        ptr = memoryStart + memoryOffset;

        (*(memoryBlock_Header *)ptr).size ^= 1;
        (*(memoryBlock_Header *)ptr).prevFromList = 0;
        (*(memoryBlock_Header *)ptr).nextFromList = 0;

        *(short unsigned int *)headerReader = (*(memoryBlock_Header *)ptr).nextFromList;

        ptr += sizeof(memoryBlock_Header);

        printf("Successfully allocated %d B of data in equally large data block.\n", recommendedSize);
        printf("\n");

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

                    (*(memoryBlock_Header *)ptr).size ^= 1;

                    ptr += sizeof(memoryBlock_Header);

                    printf("Successfully allocated %d B of data in %d B data block without dividing it.\n", recommendedSize, 1 << curList);
                    printf("\n");

                    return ptr;
                } else {
                    //allocating memory to a data block of larger size that can be divided

                    printf("Successfully allocated %d B of data in %d B data block dividing it into smaller pieces.\n", recommendedSize, 1 << curList);
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
                    ptr += sizeof(memoryBlock_Header) + recommendedSize;
                    while (remainingSize > 0) {
                        *(char *)ptr = 0;
                        ptr++;
                        remainingSize--;
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
        printf("requested real memory size = %d\n", realSize);
        printf("available memory lists =");

        ptr = memoryStart;

        for (i = 1; i <= numberOfDataLists; i++) {
            ptr += sizeof(short int);
            if (*(short int *)ptr != 0) {
                printf(" %d", 1 << i);
            }
        }
        printf("\n");
    }
}

int memory_check(void *ptr) {

}

int memory_free(void *valid_ptr) {
    if (memory_check(valid_ptr)) {
        return 0;
    } else {
        return 1;
    }
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
    int i, j, counter;
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

    printf("\n");
}

int main() {
    int memorySize = 50;
    char myMemory[memorySize], *buf;

    memoryStart = myMemory;

    memory_init(myMemory, memorySize);

    print_memory();

    buf = (char *)memory_alloc(4);

    print_memory();

    buf = (char *)memory_alloc(4);

    print_memory();

    return 0;
}
