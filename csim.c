// 吕杭州 2200013126
// Usage: ./csim [-hv] -s <s> -E <E> -b <b> -t <tracefile>
// • -h: Optional help flag that prints usage info
// • -v: Optional verbose flag that displays trace info
// • -s <s>: Number of set index bits (S = 2s is the number of sets)
// • -E <E>: Associativity (number of lines per set)
// • -b <b>: Number of block bits (B = 2b is the block size)
// • -t <tracefile>: Name of the valgrind trace to replay
/*
“I” denotes an instruction load, “L” a data load,
“S” a data store, 
and “M” a data modify (i.e., a data load followed by a data store).
*/
/*
For this lab, you should assume that memory accesses are aligned properly, such that a single memory
access never crosses block boundaries. By making this assumption, you can ignore the request sizes
in the valgrind traces.
这意味着我们不需要考虑数据的大小问题
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h> // getopt函数的库
#include "cachelab.h"

// 最后的printSummary要用的三个参数
int miss_count = 0;
int hit_count = 0;
int eviction_count = 0;

// 考虑模拟一个高速缓存
// 这是高速缓存里的一个组里面的一行
typedef struct
{
    int valid; // 有效位
    int tag;   // 标记
    int time;  // 记录最近一次使用的时间
} Line;
// 最后是高速缓存
typedef struct
{
    int S, E, B; // S组，每组E行，每行B字节
    Line **lines;// 这一个行的二维数组
                 // 第一维是组的索引，第二维是行的索引
} Cache;

Cache* cache;
// 初始化高速缓存
void initCache(int S, int E, int B)
{
    cache = (Cache *)malloc(sizeof(Cache));
    cache->S = S;
    cache->E = E;
    cache->B = B;
    cache->lines = (Line **)malloc(sizeof(Line *) * S);
    // cache->lines指向了一个Line*的数组，实际上Set = Line*
    for (int i = 0; i < S; i++)
    {
        // cahce->lines[i]意味着第i组，所以要分配E行
        cache->lines[i] = (Line *)malloc(sizeof(Line) * E);
        for (int j = 0; j < E; j++)
        {
            cache->lines[i][j].valid = 0;
            cache->lines[i][j].tag = 0;
            cache->lines[i][j].time = 0;
        }
    }
}

// 判断当前传入的地址是否命中
int HitIndexOrMiss(int set_index, int tag)
{
    // 首先遍历这一组里面的所有行
    for (int i = 0; i < cache->E; i++)
    {
        // 如果这一行是有效的，并且标记和传入的标记一样，就是命中
        if (cache->lines[set_index][i].valid == 1 && cache->lines[set_index][i].tag == tag)
            return i;
        // 如果命中了，直接返回当前的行号
    }
    return -1;
}
// 找到空的行
int findEmptyLine(int set_index)
{
    for (int i = 0; i < cache->E; i++)
    {
        if (cache->lines[set_index][i].valid == 0)
            return i;
    }
    return -1;
}
// 找到最近最久未使用的行
int findLRU(int set_index) {
    int maxTime = cache->lines[set_index][0].time;
    int maxIndex = 0;
    for (int i = 1; i < cache->E; i++) 
    {
        if (cache->lines[set_index][i].time > maxTime) 
        {
            maxTime = cache->lines[set_index][i].time;
            maxIndex = i;
        }
    }
    return maxIndex;
}
// 更新信息
void updateInfo(int set_index, int tag, int verbose)
{
    int hitIndex = HitIndexOrMiss(set_index, tag);
    // 如果没有命中的话
    if (hitIndex == -1)
    {
        miss_count++;
        if (verbose)
            printf(" miss");
        // 既然没有命中的话，意味着我们需要把当前的这个加入到缓存中去
        // 首先看看有没有空的行
        int emptyLine = findEmptyLine(set_index);
        if (emptyLine == -1)
        {
            // 如果没有找到空的行，意味着我们需要驱逐一个
            eviction_count++;
            if (verbose)
                printf(" eviction");
            // LRU的意思是：驱逐我们这次访问的这个组的时间最久远的那一个
            int LRUIndex = findLRU(set_index);
            cache->lines[set_index][LRUIndex].tag = tag;
            cache->lines[set_index][LRUIndex].time = 0;
            cache->lines[set_index][LRUIndex].valid = 1;
            // 还要给这个组里有效的行的时间都加1
            for (int i = 0; i < cache->E; i++)
            {
                if (cache->lines[set_index][i].valid == 1 && i != LRUIndex)
                    cache->lines[set_index][i].time++;
            }
        }
        else
        {
            // 找到了空的行
            cache->lines[set_index][emptyLine].tag = tag;
            cache->lines[set_index][emptyLine].time = 0;
            cache->lines[set_index][emptyLine].valid = 1;
            // 还要给这个组里有效的行的时间都加1
            for (int i = 0; i < cache->E; i++)
            {
                if (cache->lines[set_index][i].valid == 1 && i != emptyLine)
                    cache->lines[set_index][i].time++;
            }
        }
    }
    else
    {
        // 命中了
        hit_count++;
        if (verbose)
            printf(" hit");
        // 还要给这个组里有效的行的时间都加1
        cache->lines[set_index][hitIndex].time = 0;
        for (int i = 0; i < cache->E; i++)
        {
            if (cache->lines[set_index][i].valid == 1 && i != hitIndex)
                cache->lines[set_index][i].time++;
        }
    }
}

int main(int argc, char *argv[])
{
    extern char *optarg; // getopt函数的全局参数
    extern int optind, opterr, optopt; // optind = index, 
    int s, E, b;
    char *tracefile;  // 要读取的文件名称
    int verbose = 0; // 是不是要输出详细的信息
    int help = 0;   // 是不是要输出帮助信息
    int opt;
    // getopt(int argc, char * const argv[], const char *optstring)
    while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1)
    {
        switch (opt)
        {
        case 'h':
            help = 1;
            break;
        case 'v':
            verbose = 1;
            break;
        case 's':
            s = atoi(optarg);
            break;
        case 'E':
            E = atoi(optarg);
            break;
        case 'b':
            b = atoi(optarg);
            break;
        case 't':
            tracefile = optarg;
            break;
        default:
        // 如果输入的参数不是上面的几个，就会输出错误信息
            opterr = 1;
            printf("wrong input\n");
            break;
        }
    }
    if (help == 1)
    {
        printf("Usage: ./csim [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n");
        printf("  -h: Optional help flag that prints usage info\n");
        printf("  -v: Optional verbose flag that displays trace info\n");
        printf("  -s <s>: Number of set index bits (S = 2s is the number of sets)\n");
        printf("  -E <E>: Associativity (number of lines per set)\n");
        printf("  -b <b>: Number of block bits (B = 2b is the block size)\n");
        printf("  -t <tracefile>: Name of the valgrind trace to replay\n");
        exit(0); // 打印完帮助信息就退出
    }

// tracefile文件里面是这样的
// I 0400d7d4,8
//  M 0421c7f0,4
//  L 04f6b868,8
//  S 7ff0005c8,8
//  [space]operation address,size

    initCache(1 << s, E, 1 << b);
    char operation;
    unsigned long long address;
    int size;
    FILE *fp;
    fp = fopen(tracefile, "r");
    if (fp == NULL)
    {
        printf("can't open file\n");
        exit(1);
    }
    char temp_ch;
    while ((temp_ch = fgetc(fp)) != EOF)
    {
        if (temp_ch == '\n')
            continue;
        // 直接利用fscanf格式化的读入operation, address, size
        if (temp_ch == ' ') 
            fscanf(fp, "%c %llx,%d", &operation, &address, &size);
        else
        {
            operation = temp_ch;
            fscanf(fp, "%llx,%d", &address, &size);
        }
        if (verbose)
            printf("%c %llx,%d", operation, address, size);

        // 如果只考虑是否命中的话，不用担心块偏移
        // 首先得到组索引
        int set_index = (address >> b) & ((1 << s) - 1);
        // 然后得到标记
        int tag = address >> (s + b);
        // 接着在cache里面寻找是否命中，是否需要被驱逐等
        switch (operation)
        {
        case 'I':
            break;
        case 'L':
            updateInfo(set_index, tag, verbose);
            break;
        case 'S':
            updateInfo(set_index, tag, verbose);
            break;
        case 'M':
            updateInfo(set_index, tag, verbose);
            updateInfo(set_index, tag, verbose);
            // 文件里说到：“M” a data modify (i.e., a data load followed by a data store).
            break;
        default:
            break;
        }
        if (verbose)
            printf("\n");
    }
    fclose(fp);
    printSummary(hit_count, miss_count, eviction_count);
    return 0;
}