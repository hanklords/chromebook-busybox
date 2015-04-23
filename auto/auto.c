#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>

#define FW_PATH "/sbin/fw"
#define PID_FILE "/tmp/auto.pid"
#define WAIT_TIME 2

void quit(int i) {
    remove(PID_FILE);
    exit(0);
}

int main() {
    const struct sigaction act = {.sa_handler = quit};    
    sigaction(SIGTERM, &act, NULL);
    
    daemon(0, 0);
    
    FILE* f = fopen(PID_FILE, "w");
    fprintf(f, "%i\n", getpid());
    fclose(f);
    
    const struct timespec req = {.tv_sec = WAIT_TIME};
    nanosleep(&req, NULL);
    
    execl(FW_PATH, "fw", "boot", NULL);
    return 0;
}
