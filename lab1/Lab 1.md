# Lab 1

### A

- pipe vs. redirection
  - Pipe - pass output to another program or utility
  - Redirection - pass output to either a file or stream
- pipexec
  - Create a directed graph of processes and pipes
- Only 3 oflags need to use `open`, others just need to set the flag with bitwise or `|=` , which will combine all oflags and once the `open` is called, all of these flags will be execute.
  - O_RDONLY
    - Open for reading only.
  - O_WRONLY
    - Open for writing only.
  - O_RDWR
    - Open for reading and writing. The result is undefined if this flag is applied to a FIFO.



### B

- check path exists or not:

  ```c
  #include <unistd.h>
  if( access( fname, F_OK ) != -1 ) {
      // file exists
  } else {
      // file doesn't exist
  }
  ```

  

| Tag                            | Description                                                  |
| :----------------------------- | :----------------------------------------------------------- |
| **O_APPEND**                   | The file is opened in append mode. Before each **write**(), the file offset is positioned at the end of the file, as if with**lseek**(). **O_APPEND** may lead to corrupted files on NFS file systems if more than one process appends data to a file at once. This is because NFS does not support appending to a file, so the client kernel has to simulate it, which can’t be done without a race condition. |
| **O_CREAT**                    | If the file does not exist it will be created. The owner (user ID) of the file is set to the effective user ID of the process. The group ownership (group ID) is set either to the effective group ID of the process or to the group ID of the parent directory (depending on filesystem type and mount options, and the mode of the parent directory, see, e.g., the mount options *bsdgroups* and *sysvgroups* of the ext2 filesystem, as described in **mount**(8)). |
| **O_DIRECTORY**                | If *pathname* is not a directory, cause the open to fail. This flag is Linux-specific, and was added in kernel version 2.1.126, to avoid denial-of-service problems if **opendir**(3) is called on a FIFO or tape device, but should not be used outside of the implementation of **opendir**. |
| **O_EXCL**                     | Ensure that this call creates the file: When used with **O_CREAT**, if the file already exists it is an error and the **open**() will fail. In this context, a symbolic link exists, regardless of where it points to. |
| **O_NOFOLLOW**                 | If *pathname* is a symbolic link, then the open fails. This ensures that symbolic links are not followed, preventing the sort of race condition attack in which use is made of symbolic links. |
| **O_NONBLOCK** or **O_NDELAY** | When possible, the file is opened in non-blocking mode. Neither the **open**() nor any subsequent operations on the file descriptor which is returned will cause the calling process to wait. |
| **O_SYNC**                     | The file is opened for synchronous I/O. Any **write**()s on the resulting file descriptor will block the calling process until the data has been physically written to the underlying hardware.*But see RESTRICTIONS below*. |
| **O_TRUNC**                    | If the file already exists and is a regular file and the open mode allows writing (i.e., is O_RDWR or O_WRONLY) it will be truncated to length 0. If the file is a FIFO or terminal device file, the O_TRUNC flag is ignored. Otherwise the effect of O_TRUNC is unspecified. |
| **O_CLOEXEC**                  | sets the *close-on-exec* flag for the file descriptor, which causes the file descriptor to be automatically (and atomically) closed when any of the `exec`-family functions succeed. |
| **O_DSYNC**                    | makes every write() to the file return only when the contents of the file have been written to disk.  This provides the guarantee that when the system call returns the file data is on disk. |
| **O_SYNC**                     | The file is opened for synchronous I/O. Have each `write` wait for physical I/O to complete, including I/O necessary to update file attributes modified as a result of the `write`.It guarantees not only that the file data has been written to disk but also the file metadata. |
| **O_RSYNC**                    | Have each `read` operation on the file descriptor wait until any pending writes for the same portion of the file are complete. |