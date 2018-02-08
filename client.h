/*
Macros
*/
#define BLOCK_SIZE 2000 //Buffer size of msg data
#define METADATA_SIZE 10 //Buffer size of msg metadata
#define PORTNO 9000
#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR 2

/*
 * Client side functions
 */

/*
 * net_server_connection_init description:
 * Takes a hostname as an argument and a file connection mode and verifies that
 * the host exists.
 * Returns -1 on failure and 0 on success.
 * If a net file command is called when 'net_server_connection_init' either has not been
 * run, or has been run on a host name that does not exist, the net file
 * command returns '-1' and set h_errno to HOST_NOT_FOUND. 'net_server_connection_init' is
 * also responsible for making sure the library attaches the file connection
 * mode to every net file command a library sends.
 *
 * RETURN
 * 0 on success, -1 on error and h_errnor set correctly
 *
 * ERRORS
 * HOST_NOT_FOUND
 * INVALID_FILE_MODE (not implimented yet, but be sure to include #define of
 * this error code in the .h if I implement it)
 */
int net_server_connection_init(char *hostname, int filemode);

/*
 * The argument flags must include one of the following access modes: O_RDONLY,
 * O_WRONLY, or O_RDWR. These request opening the file read-only, write-only,
 * or read/write, respectively.
 *
 * RETURN VALUE
 * net_open() returns the new file descriptor, or -1 if an error occurred (in
 * which case, errno is set appropriately).
 *
 * ERRORS (check open manpage for definition)
 * EACCES
 * EINTR
 * EISDIR
 * ENOENT
 * EROFS
 *
 * Planning on implimenting these...
 * ENFILE
 * EWOULDBLOCK
 * EPERM
 */
int net_open(const char *pathname, int flag);

/*
 * Upon successful completion, net_read() returns a non-negative integer
 * indicating the number of bytes actually read. Otherwise, the function
 * returns -1 and sets errno to indicate the error.
 *
 * ERRORS (check manpage for definitions)
 * ETIMEDOUT
 * EBADF
 * ECONNRESET
 */
ssize_t net_read(int fildes, void *buf, size_t nbyte);

/*
 * RETURN VALUE
 * Upon successful completion, net_write() returns the number of bytes actually
 * written to the file associated with fildes. This number should never be
 * greater than nbyte. Otherwise, -1 is returned and errno set to
 * indicate the error.
 *
 * ERRORS
 * EBADF
 * ETIMEOUT
 * ECONNRESET
 */
ssize_t net_write(int fildes, const void *buf, size_t nbyte);

/*
 * RETURN VALUE
 * net_close() returns zero on success. On error, -1 is returned, and errno is
 * set appropriately.
 *
 * ERRORS
 * EBADF
 */
int net_close(int fd);
