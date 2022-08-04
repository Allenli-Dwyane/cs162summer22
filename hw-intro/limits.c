#include <stdio.h>
#include <sys/resource.h>
#include <unistd.h>
#include <stdlib.h>
#define CUR_PROC 0

int main() {
    struct rlimit lim;
    /* get stack size */
    if(prlimit(CUR_PROC, RLIMIT_STACK, NULL, &lim) == -1)
        perror("prlimit-1");
    printf("stack size: %ld\n", lim.rlim_cur);
    if(prlimit(CUR_PROC, RLIMIT_NPROC, NULL, &lim) == -1)
        perror("prlimit-1");
    printf("process limit: %ld\n", lim.rlim_cur);
    if(prlimit(CUR_PROC, RLIMIT_NOFILE, NULL, &lim) == -1)
        perror("prlimit-1");
    printf("max file descriptors: %ld\n", lim.rlim_cur);
    return 0;
}
