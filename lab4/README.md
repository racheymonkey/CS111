# Hey! I'm Filing Here

In this lab, I created a 1 MiB ext2 file system with two directories, one regular file, and one symbolic link. By modifying critical functions in 'ext2-create.c', I constructed an ext2 filesystem image that adheres to the filesystem structure expected by 'fsck.ext2'.

## Building

1. Navigate to the 'lab4' directory where the 'Makefile' is located
2. Run the 'make' command to compile the 'ext2-create' executable

## Running

1. Create the ext2 filesystem image:

   ./ext2-create
   
   This command generates 'cs111-base.img'
   
3. Inspect the filesystem structure for debugging purposes:

   dumpe2fs cs111-base.img

   'dumpe2fs' provides detailed information about the filesystem.

4. Check the filesystem for consistency and correctness:

   fsck.ext2 cs111-base.img
   
   Ensure that 'fsck.ext2' reports no errors.

6. Prepare a mount point and mount the filesystem:

   mkdir mnt
   sudo mount -o loop cs111-base.img mnt

## Cleaning up

To clean up the build artifacts and reset the lab environment:

1. Unmount the filesystem:
   
   sudo umount mnt

   This safely detaches the filesystem from the mount point,

2. Clean up the mount point directory:

   rmdir mnt

3.  Clean all compiled binaries:

    make clean
