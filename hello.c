// hello.c — Sample program for MMUKO time-payload scheduling
// This program runs for a fixed duration, simulating a real workload

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

int main(int argc, char** argv) {
    (void)argc; (void)argv;

    printf("[HELLO] Process started: pid=%d\n", getpid());
    printf("[HELLO] Simulating work for 3 seconds...\n");

    // Simulate work: 3 seconds of CPU time
    for (int i = 0; i < 3; i++) {
        printf("[HELLO] Working... tick %d/3\n", i + 1);
        sleep(1);
    }

    printf("[HELLO] Process completed successfully.\n");
    return 0;
}
