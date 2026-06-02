/* 最小 syscall stubs — newlib bare-metal */
#include <sys/stat.h>
#include <errno.h>

#undef errno
extern int errno;

void _exit(int code) { while (1) { } }

int _kill(int pid, int sig) { errno = EINVAL; return -1; }
int _getpid(void) { return 1; }

void* _sbrk(int incr)
{
    extern char _end;      /* 由链接脚本定义 */
    extern char _estack;   /* 栈顶 */
    static char* heap_end = &_end;
    char* prev = heap_end;

    /* 堆栈碰撞检测: 留 4KB 安全余量 */
    if (heap_end + incr > &_estack - 4096)
    {
        errno = ENOMEM;
        return (void*)-1;
    }
    heap_end += incr;
    return prev;
}

int _close(int fd) { errno = EBADF; return -1; }
int _fstat(int fd, struct stat* st) { st->st_mode = S_IFCHR; return 0; }
int _isatty(int fd) { return 1; }
int _lseek(int fd, int ptr, int dir) { return 0; }
int _read(int fd, char* buf, int len) { return 0; }
int _write(int fd, const char* buf, int len) { return len; }
