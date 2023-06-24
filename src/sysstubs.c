#include <errno.h>
#include <sys/stat.h>

int __attribute__((weak)) _close(int fildes)
{
	errno = ENOSYS;
	return -1;
}

int __attribute__((weak)) _read(int file, char *ptr, int len)
{
	errno = ENOSYS;
	return -1;
}

int __attribute__((weak)) _write(int file, char *ptr, int len)
{
	errno = ENOSYS;
	return -1;
}

int __attribute__((weak)) _fstat(int fildes, struct stat *st)
{
	errno = ENOSYS;
	return -1;
}

int __attribute__((weak)) _lseek(int file, int ptr, int dir)
{
	errno = ENOSYS;
	return -1;
}

int __attribute__((weak)) _getpid(void)
{
	errno = ENOSYS;
	return -1;
}

int __attribute__((weak)) _isatty(int file)
{
	errno = ENOSYS;
	return 0;
}

int __attribute__((weak)) _kill(int pid, int sig)
{
	errno = ENOSYS;
	return -1;
}