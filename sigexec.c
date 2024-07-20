#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#ifdef __linux__
#include <linux/prctl.h>
#endif

static void handle_signal(int sig);

static int sig_pipe[2];

int main(int argc, char **argv) {
    int rv;

    // create signal pipe
    if (pipe(sig_pipe) != 0) {
        perror("pipe");
        return EXIT_FAILURE;
    }

    // install signal handler
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = handle_signal;
    sigaction(SIGUSR1, &sa, NULL);

    // fork
    pid_t pid = fork();
    if (pid == 0) {
        // ignore USR1
        sa.sa_handler = SIG_IGN;
        sigaction(SIGUSR1, &sa, NULL);

#ifdef __linux__
        // ask for HUP on parent death
        prctl(PR_SET_PDEATHSIG, SIGHUP);
#endif
        // wait on pipe
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(sig_pipe[0], &rfds);
        rv = select(sig_pipe[0] + 1, &rfds, NULL, NULL, NULL);
        if (rv < 0) {
            perror("select");
            return EXIT_FAILURE;
        } else if (rv == 0) {
            fprintf(stderr, "select: Unexpected time out\n");
            return EXIT_FAILURE;
        }
        read(sig_pipe[0], &rv, sizeof(int));

        // exec
        if (argc < 2) {
            return EXIT_SUCCESS;
        } else {
            execvp(argv[1], argv + 1);
        }
        return EXIT_FAILURE; // unreachable
    } else if (pid < 0) {
        // fork errored
        perror("fork");
        return EXIT_FAILURE;
    }

    printf("sigexec: Forked %d\n", pid);

    // wait for child
    int status;
    while (1) {
        rv = waitpid(pid, &status, 0);
        if (rv == -1) {
            if (errno == EINTR || errno == EAGAIN) {
                continue;
            } else {
                perror("waitpid");
                return EXIT_FAILURE;
            }
        }
        if (WIFEXITED(status) || WIFSIGNALED(status)) break;
    }
    close(sig_pipe[0]);
    close(sig_pipe[1]);

    // forward exit status
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        fprintf(stderr, "Child terminated by signal %d\n", WTERMSIG(status));
        return EXIT_FAILURE;
    }
    return EXIT_FAILURE;
}

static void handle_signal(int sig) {
    ssize_t rv = write(sig_pipe[1], &sig, sizeof(int));
    if (rv < 0) {
        perror("write");
    } else if (rv != sizeof(int)) {
        fprintf(stderr, "write: Partial write\n");
    }
}
