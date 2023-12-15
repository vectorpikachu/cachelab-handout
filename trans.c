// 吕杭州 2200013126
/*
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */
#include <stdio.h>
#include "cachelab.h"
#include "contracts.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/*
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. The REQUIRES and ENSURES from 15-122 are included
 *     for your convenience. They can be removed if you like.
 */
void transpose_32(int M, int N, int A[N][M], int B[M][N])
{
    int i, tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
    int ii, jj;
    for (ii = 0; ii < N; ii += 8) {
        for (jj = 0; jj < M; jj += 8) {
            // 枚举8*8的小块
            // A[ii ~ ii+7][jj ~ jj+7] -> B[jj ~ jj+7][ii ~ ii+7]
            /*
            * A[ii][jj]读入时，会把A[ii][jj]~A[ii][jj+7]都读入
            * B[jj][ii]写入时，会把B[jj][ii]~B[jj][ii+7]都写入
            * 后面第二次i = 1的时候就要用到B[jj][ii+1]了
            * 这样后面就不会miss了
            * S 0014d0a0,4 set_index = 00 101
            * S 0014d120,4 set_index = 01 001
            * S 0014d1a0,4 set_index = 01 101
            * S 0014d220,4 set_index = 10 001
            * S 0014d2a0,4 set_index = 10 101
            * S 0014d320,4 set_index = 11 001
            * S 0014d3a0,4 set_index = 11 101
            * S 0014d420,4 set_index = 00 001
            */
            for (i = ii; i < ii + 8; i++) {
                tmp0 = A[i][jj];
                tmp1 = A[i][jj + 1];
                tmp2 = A[i][jj + 2];
                tmp3 = A[i][jj + 3];
                tmp4 = A[i][jj + 4];
                tmp5 = A[i][jj + 5];
                tmp6 = A[i][jj + 6];
                tmp7 = A[i][jj + 7];
                B[jj][i] = tmp0;
                B[jj + 1][i] = tmp1;
                B[jj + 2][i] = tmp2;
                B[jj + 3][i] = tmp3;
                B[jj + 4][i] = tmp4;
                B[jj + 5][i] = tmp5;
                B[jj + 6][i] = tmp6;
                B[jj + 7][i] = tmp7;
            }
        }
    }
}
void transpose_64(int M,int N, int A[N][M], int B[M][N])
{
    int i, tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
    int ii, jj;
    int bsize = 8;
    for (ii = 0; ii < N; ii += bsize) {
        for (jj = 0; jj < M; jj += bsize) {
            // 枚举8*8的小块
            // A[ii ~ ii+7][jj ~ jj+7] -> B[jj ~ jj+7][ii ~ ii+7]
            /*
            * A[ii][jj]读入时，会把A[ii][jj]~A[ii][jj+7]都读入
            * B[jj][ii]写入时，会把B[jj][ii]~B[jj][ii+7]都写入
            * 后面第二次i = 1的时候就要用到B[jj][ii+1]了
            * 这样后面就不会miss了
            * 但是在64 * 64这里的问题是
            * S 150198,4 miss eviction set_index = 01 100
            * S 150298,4 miss eviction set_index = 10 100
            * S 150398,4 miss eviction set_index = 11 100
            * S 150498,4 miss eviction set_index = 00 100
            * S 150598,4 miss eviction set_index = 01 100
            * S 150698,4 miss eviction set_index = 10 100
            * S 150798,4 miss eviction set_index = 11 100
            * S 150898,4 miss eviction set_index = 00 100
            * 此时读入后面的会覆盖前面的四个 4644
            * 既然这样，试着用4 * 4的小块 这样是比之前要好一点的 miss = 1700
            * 后面的思路是分成四小块来干活
            * A = [A1 A2]   B = [B1 B2]
            *     [A3 A4]       [B3 B4]
            * 首先，我们让B1 = A1^T, B2 = A2^T 这里面A只有一行之间的问题，B4列
            * 接着让B3 = B2,  B2 = A3^T 这里面最多四个列，不会有冲突的问题
            * 最后让B4 = A4^T 这里也是A只有行的问题 B四个列 miss = 1228
            */
            for (i = ii; i < ii + 4; i++) {
                
                tmp0 = A[i][jj];
                tmp1 = A[i][jj + 1];
                tmp2 = A[i][jj + 2];
                tmp3 = A[i][jj + 3];
                tmp4 = A[i][jj + 4];
                tmp5 = A[i][jj + 5];
                tmp6 = A[i][jj + 6];
                tmp7 = A[i][jj + 7];

                // B1 = A1^T
                B[jj][i] = tmp0;
                B[jj + 1][i] = tmp1;
                B[jj + 2][i] = tmp2;
                B[jj + 3][i] = tmp3;
                // B2 = A2^T 这里存储的四行不会miss因为前面四行已经读入了8个数了
                B[jj][i + 4] = tmp4;
                B[jj + 1][i + 4] = tmp5;
                B[jj + 2][i + 4] = tmp6;
                B[jj + 3][i + 4] = tmp7;
            }
            // B3 = B2 B2 = A3^T
            for (i = jj; i < jj + 4; i++) {
                // 这是第i行的B2
                tmp0 = B[i][ii + 4];
                tmp1 = B[i][ii + 5];
                tmp2 = B[i][ii + 6];
                tmp3 = B[i][ii + 7];
                // 这是第i+4行的A3
                tmp4 = A[ii + 4][i];
                tmp5 = A[ii + 5][i];
                tmp6 = A[ii + 6][i];
                tmp7 = A[ii + 7][i];
                // B2 = A3^T
                B[i][ii + 4] = tmp4;
                B[i][ii + 5] = tmp5;
                B[i][ii + 6] = tmp6;
                B[i][ii + 7] = tmp7;
                // B3 = B2(original)
                B[i + 4][ii] = tmp0;
                B[i + 4][ii + 1] = tmp1;
                B[i + 4][ii + 2] = tmp2;
                B[i + 4][ii + 3] = tmp3;
            }
            // B4 = A4^T
            for (i = ii + 4; i < ii + 8; i++) {
                tmp0 = A[i][jj + 4];
                tmp1 = A[i][jj + 5];
                tmp2 = A[i][jj + 6];
                tmp3 = A[i][jj + 7];
                B[jj + 4][i] = tmp0;
                B[jj + 5][i] = tmp1;
                B[jj + 6][i] = tmp2;
                B[jj + 7][i] = tmp3;
            }
        }
    }
}
void transpose_60(int M, int N, int A[N][M], int B[M][N])
{
    int ii, jj, tmp;
    int i, j;
    int diag;
    // 60 * 68的数组
    // bsize = 8 miss = 1761 1656
    // bsize = 16 miss = 1871
    // bsize = 4 miss = 1833
    // bsize = 12 miss = 1737 1618
    /*
    * L 110ca0,4 miss eviction
    * S 14d1a0,4 miss eviction 
    * L 110ca4,4 hit 
    * S 14d2b0,4 miss eviction 
    * L 110ca8,4 hit 
    * S 14d3c0,4 miss eviction 
    * L 110cac,4 hit 
    * S 14d4d0,4 miss eviction 
    * L 110cb0,4 hit 
    * S 14d5e0,4 miss eviction 
    * L 110cb4,4 hit 
    * S 14d6f0,4 miss eviction 
    * L 110cb8,4 hit 
    * S 14d800,4 miss eviction 
    * L 110cbc,4 hit 
    * S 14d910,4 miss eviction 
    * L 110d90,4 miss eviction
    */
    int bsize = 12;
    for (jj = 0; jj < M; jj += bsize) {
        for (ii = 0; ii < N ; ii += bsize) {
            for (i = ii; i < N && i < ii + bsize; i++) {
                for (j = jj; j < M && j < jj + bsize; j++) {
                    if (i != j)
                        B[j][i] = A[i][j];
                    else {
                        tmp = A[i][i];
                        diag = i;
                    }
                } 
                if (ii == jj)
                    B[diag][diag] = tmp;
                // 每次循环最多只可能有一个对角线的元素
            }
        }
    }
}


char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    REQUIRES(M > 0);
    REQUIRES(N > 0);
    /*
     * You are allowed to define at most 12 local variables of type int per transpose function.
     * Your transpose function may not use recursion.
    */
    // Think about the potential for conflict misses in your code, especially along the diagonal.
    // Try to think of access patterns that will decrease the number of these conflict misses.
    // 但是如果是对角线的，可能会映射到同一个地方，发生抖动
    // 之前妄图使用一个函数解决好像不太行
    if (M == 32 && N == 32)
        transpose_32(M, N, A, B);
    else if (M == 64 && N == 64)
        transpose_64(M, N, A, B);
    else
        transpose_60(M, N, A, B);

    ENSURES(is_transpose(M, N, A, B));
}

/*
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started.
 */

 /*
  * trans - A simple baseline transpose function, not optimized for the cache.
  */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    REQUIRES(M > 0);
    REQUIRES(N > 0);

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }

    ENSURES(is_transpose(M, N, A, B));
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc);

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc);

}

/*
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

