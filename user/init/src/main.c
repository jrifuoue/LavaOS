#include <minos/sysstd.h>
#include <minos/status.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int main(void);

void _start(int argc, const char** argv, const char** envp) {
    const char* std = "/devices/tty0";
    if(open(std, O_RDONLY) < 0 ||   /*STDIN  (fd 0)*/
       open(std, O_WRONLY) < 0 ||   /*STDOUT (fd 1)*/
       open(std, O_WRONLY) < 0) {   /*STDERR (fd 2)*/
        exit(1); 
    }
    _libc_init_environ(envp);
    _libc_init_streams();
    int code = main();
    close(STDOUT_FILENO);
    if(STDIN_FILENO != STDOUT_FILENO) {
        close(STDIN_FILENO);
    }
    exit(code);
}

#define MAX_SESSION_RESTARTS 5

int main(void) {
    printf("\033[2J\033[H");
    fflush(stdout);

    printf("[INIT] '/sbin/init' started!\n");

    printf("[INIT] Setting environment...\n");
    setenv("PATH", "/user:/sbin:", 0);
    setenv("HOSTNAME", "lavaos", 1);
    setenv("SESSION", "desktop", 1);

    const char* services[] = {
        "/etc/init.d/login",
    };
    size_t num_services = sizeof(services) / sizeof(services[0]);

    int restarts = 0;

    for(;;) {
        printf("[INIT] Starting services...\n");

        intptr_t pids[16];
        size_t running = 0;

        for(size_t i = 0; i < num_services && running < 16; ++i) {
            intptr_t pid = fork();
            if(pid == 0) {
                const char* argv[] = { services[i], NULL };
                execve(services[i], (char*const*)argv, (char*const*)environ);
                printf("[INIT] Failed to exec %s\n", services[i]);
                exit(1);
            }
            if(pid < 0) {
                printf("[INIT] fork failed for %s\n", services[i]);
                continue;
            }
            pids[running++] = pid;
            printf("[INIT] Started %s (pid %ld)\n", services[i], (long)pid);
        }

        if(running == 0) {
            printf("[INIT] No services to run. Halting.\n");
            for(;;);
        }

        printf("[INIT] Waiting for services... (%zu running)\n", running);

        for(size_t i = 0; i < running; ++i) {
            intptr_t code = wait_pid(pids[i]);
            printf("[INIT] Service %zu exited with code %ld\n", i, (long)code);
        }

        restarts++;
        if(restarts >= MAX_SESSION_RESTARTS) {
            printf("[INIT] Too many restarts (%d). System halted.\n", restarts);
            for(;;);
        }

        printf("[INIT] Session ended. Restarting in 2 seconds... (attempt %d/%d)\n",
               restarts, MAX_SESSION_RESTARTS);

        // Brief delay before restart (busy-wait since sleep may not be available)
        for(volatile int i = 0; i < 20000000; i++) {}

        printf("\033[2J\033[H");
        fflush(stdout);
    }
}
