#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>

#define PID_FILE "/tmp/auto.pid"

void quit(int i) {
    remove(PID_FILE);
    exit(0);
}

int main() {
    const struct sigaction act = {.sa_handler = quit};    
    sigaction(SIGTERM, &act, NULL);
    
    FILE* f = fopen(PID_FILE, "w");
    fprintf(f, "%i\n", getpid());
    fclose(f);
    
    const struct timespec req = {.tv_sec = 4};
    nanosleep(&req, NULL);
    
    printf("boot !\n");

    return 0;
}
