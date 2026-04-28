#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#define ESC_KEY "\x1b"
#define ENTER_KEY "\x0a"
#define TIMEOUT_SEC 2

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define BEFEDIT_PATH "/usr/local/bin/befedit"

#define TYPING_SPEED_WPM 100
#define TYPING_CHAR_INTERVAL_MS (1000 * 60 / (TYPING_SPEED_WPM * 5))

static void test(const char *keys, const char *filename) {
    int master_fd;
    int slave_fd;
    int unlock;
    unsigned int pty_num;
    char slave_path[64];
    pid_t pid;
    int status;
    int elapsed;
    int exited;
    pid_t result;
    int devnull = open("/dev/null", O_WRONLY);

    struct winsize ws;

    printf("Running test: %s\n", filename);

    master_fd = open("/dev/ptmx", O_RDWR);
    if (master_fd < 0) {
        perror("open /dev/ptmx");
        return;
    }

    /* unlock the slave PTY */
    unlock = 0;
    if (ioctl(master_fd, TIOCSPTLCK, &unlock) < 0) {
        perror("ioctl TIOCSPTLCK");
        close(master_fd);
        return;
    }

    /* get the PTY number */
    if (ioctl(master_fd, TIOCGPTN, &pty_num) < 0) {
        perror("ioctl TIOCGPTN");
        close(master_fd);
        return;
    }

    /* get the real terminal's dimensions */
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) < 0) {
        perror("ioctl TIOCGWINSZ");
        close(master_fd);
        return;
    }

    /* set them on the master PTY so the slave inherits them */
    if (ioctl(master_fd, TIOCSWINSZ, &ws) < 0) {
        perror("ioctl TIOCSWINSZ");
        close(master_fd);
        return;
    }

    snprintf(slave_path, sizeof(slave_path), "/dev/pts/%u", pty_num);

    pid = fork();
    if (pid < 0) {
        perror("fork");
        close(master_fd);
        return;
    }

    if (pid == 0) {
        /* child: become session leader and attach to slave PTY */
        if (setsid() < 0) {
            perror("setsid");
            _exit(1);
        }

        slave_fd = open(slave_path, O_RDWR);
        if (slave_fd < 0) {
            perror("open slave pty");
            _exit(1);
        }

        /* set them on the master PTY so the slave inherits them */
        if (ioctl(slave_fd, TIOCSWINSZ, &ws) < 0) {
            perror("ioctl TIOCSWINSZ");
            _exit(1);
        }

        close(master_fd);

        dup2(slave_fd, STDIN_FILENO);
        dup2(devnull, STDOUT_FILENO);
        /*
         * Errors are probably useful, but can be blocked with:
         * dup2(devnull, STDERR_FILENO);
         */
        if (devnull > STDERR_FILENO) close(devnull);
        if (slave_fd > STDERR_FILENO) close(slave_fd);

        execl(BEFEDIT_PATH, "befedit", filename, (char *)NULL);
        perror("execl");
        _exit(1);
    }

    /* parent: give Befedit a moment to start, then send keystrokes */
    usleep(100 * 1000);

    while (*keys) {
        if (write(master_fd, keys, 1) < 0) {
            perror("write to pty");
        }
        usleep(TYPING_CHAR_INTERVAL_MS * 1000);
        keys++;
    }

    /* wait up to `TIMEOUT_SEC` for Befedit to exit */
    exited = 0;
    for (elapsed = 0; elapsed < TIMEOUT_SEC; elapsed++) {
        result = waitpid(pid, &status, WNOHANG);
        if (result == pid) {
            exited = 1;
            break;
        } else if (result < 0) {
            perror("waitpid");
            break;
        }
        usleep(100 * 1000);
    }

    if (!exited) {
        printf("Befedit did not exit within %d seconds.\n", TIMEOUT_SEC);
        kill(pid, SIGKILL);
        waitpid(pid, &status, 0);
    } else {
        if (WIFEXITED(status) && WEXITSTATUS(status)) {
            printf("Befedit return code: %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Befedit killed by signal: %d\n", WTERMSIG(status));
        }
    }

    close(master_fd);
    printf("Done.\n");
}

int main(void) {
    struct stat st = {0};
    puts("Ensuring `output` folder exists...");
    if (stat("output", &st) == -1) {
        mkdir("output", 0700);
    }
    test("ihello" ESC_KEY ":wq" ENTER_KEY, "output/test_1.txt");
    test("iRight! " ESC_KEY

         "jiDown! " ESC_KEY

         "hiLeft! " ESC_KEY

         "kiUp! " ESC_KEY

         ":wq" ENTER_KEY

         ,
         "output/test_2.txt");
    return 0;
}