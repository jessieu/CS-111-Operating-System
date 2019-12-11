#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#include "ext2_fs.h"

#define BASE_OFFSET 1024

void dirent (int fd, unsigned int inode_index, unsigned int block_size, unsigned int inode_block)
{
  //Each entry in the directory contains an inode number for that entry, a filename for the entry, and an offset to the next entry in the linked list.
  struct ext2_dir_entry dir;
  unsigned int offset = BASE_OFFSET + block_size * (inode_block - 1);
  unsigned int size = 0;

  while(size < block_size)
    {
      char file_name[EXT2_NAME_LEN + 1];
      if (pread(fd, &dir, sizeof(dir), offset + size) == -1)
        {
	  fprintf(stderr, "Directory entry read failed.\n");
	  exit(2);
        }

      if (dir.inode != 0)
        {
	  memcpy(file_name, dir.name, dir.name_len);
	  file_name[dir.name_len] = 0;
	  printf("DIRENT,%u,%u,%u,%u,%u,'%s'\n", inode_index, size,dir.inode, dir.rec_len, dir.name_len, file_name);
        }

      size += dir.rec_len;
    }
}

void indirect(int fd, unsigned int block_size, unsigned int inode_num, unsigned int logical_offset, unsigned int indirect_block)
{

  if (indirect_block != 0)
    {
      unsigned int offset = BASE_OFFSET + block_size * (indirect_block - 1);
      unsigned i_num = block_size / sizeof(__u32);
      unsigned int i_block[block_size / 4];

      // read all indirect block entries
      if (pread(fd, &i_block, block_size, offset) == -1)
        {
	  fprintf(stderr, "Indirect inode entry read failed.\n");
	  exit(2);
        }

      for (unsigned int i = 0; i < i_num; i++)
        {
	  if (i_block[i] != 0)
            {
	      // refer to data block(direct block)
	      // offset is first 12 direct blocks + i (in 13th block)
	      printf("INDIRECT,%u,1,%u,%u,%u\n", inode_num, i + logical_offset, indirect_block, i_block[i]);
            }
        }
    }

}

void double_indirect(int fd, unsigned int block_size, unsigned int inode_num, unsigned int logical_offset, unsigned int d_indirect)
{
  if (d_indirect != 0)
    {
      unsigned int double_indirect[block_size / 4];
      unsigned int offset = BASE_OFFSET + block_size * (d_indirect - 1);
      unsigned int d_num = block_size / sizeof(__u32); //how many double indirect block entries

      // get all double indirect blocks
      if (pread(fd, &double_indirect, block_size, offset) == -1)
        {
	  fprintf(stderr, "Double indirect entry read failed.\n");
	  exit(2);
        }

      for (unsigned int i = 0; i < d_num; i++)
        {
	  if (double_indirect[i] != 0)
            {
	      printf("INDIRECT,%u,2,%u,%u,%u\n", inode_num, logical_offset + i, d_indirect,  double_indirect[i]);
	      // get indirect block
	      indirect(fd, block_size, inode_num, logical_offset + i, double_indirect[i]);
            }
        }
    }

}

void triple_indirect(int fd, unsigned int block_size, unsigned int inode_num, unsigned int logical_offset, unsigned int t_indirect)
{
  if (t_indirect != 0)
    {
      unsigned int triple_indirect[block_size / 4];
      unsigned int offset = BASE_OFFSET + block_size * (t_indirect - 1);
      unsigned int t_num = block_size / sizeof(__u32); //how many triple indirect block entries

      // get all double indirect blocks
      if (pread(fd, &triple_indirect, block_size, offset) == -1)
        {
	  fprintf(stderr, "Triple indirect entry read failed.\n");
	  exit(2);
        }

      for (unsigned int i = 0; i < t_num; i++)
        {
	  if (triple_indirect[i] != 0)
            {
	      printf("INDIRECT,%u,3,%u,%u,%u\n", inode_num, logical_offset + i, t_indirect,  triple_indirect[i]);
	      // get indirect block
	      double_indirect(fd, block_size, inode_num, logical_offset + i, triple_indirect[i]);
            }
        }
    }
}

void inode(int fd, unsigned int block_size, unsigned int inode_table, unsigned int inode_size, unsigned int inodes_count)
{
  struct ext2_inode ino;

  unsigned int offset = BASE_OFFSET + block_size * (inode_table - 1);

  char type;

  for (unsigned int i = 0; i < inodes_count; i++)
    {
      if (pread(fd, &ino, inode_size, offset) == -1)
        {
	  fprintf(stderr, "Inode entry read failed.\n");
	  exit(2);
        }

      unsigned int mode = ino.i_mode;
      unsigned int owner = ino.i_uid;
      unsigned int group = ino.i_gid;
      unsigned int link_count = ino.i_links_count;

      if (mode != 0 && link_count != 0)
        {
	  // check file type
	  if (S_ISDIR(mode))  // directory
            {
	      type = 'd';
            }
	  else if (S_ISREG(mode))   // regular file
            {
	      type = 'f';
            }
	  else if (S_ISLNK(mode))   // symbolic link
            {
	      type = 's';
            }
	  else
            {
	      type = '?';
            }

	  mode = ino.i_mode & 0xFFF; // lower 12 bits

	  time_t t = ino.i_atime;// last access
	  struct tm at = *gmtime(&t);
	  char atime[20];
	  strftime(atime, sizeof(atime), "%m/%d/%y %H:%M:%S", &at); // format date and time

	  t = ino.i_ctime;// last change
	  struct tm ct = *gmtime(&t);
	  char ctime[20];
	  strftime(ctime, sizeof(ctime), "%m/%d/%y %H:%M:%S", &ct);

	  t = ino.i_mtime;// last access
	  struct tm mt = *gmtime(&t);
	  char mtime[20];
	  strftime(mtime, sizeof(mtime), "%m/%d/%y %H:%M:%S", &mt);

	  unsigned int file_size = ino.i_size;
	  unsigned int occupied_block = ino.i_blocks;

	  printf("INODE,%u,%c,%o,%u,%u,%u,%s,%s,%s,%u,%u", i + 1, type, mode, owner, group, link_count, ctime, mtime, atime, file_size, occupied_block);

	  if (type == 's' && file_size <= 60)
            {
	      printf("\n");
            }
	  else
            {
	      for (unsigned int j = 0; j < EXT2_N_BLOCKS; j++)
                {

		  printf(",%u", ino.i_block[j]);
                }
	      printf("\n");
            }

	  /* --------------- Directories --------------- */
	  if (type == 'd')
            {
	      for (unsigned int k = 0; k < 12; k++)
                {
		  if (ino.i_block[k] != 0){
		    dirent(fd, i + 1, block_size, ino.i_block[k]);
		  }
		}
            }

	  /* --------------- Indirect Blocks --------------- */

	  unsigned int logical_offset;

	  if (ino.i_block[12] != 0)
            {   // direct
	      logical_offset = 12;
	      indirect(fd, block_size, i + 1, logical_offset, ino.i_block[12]);
            }

	  if (ino.i_block[13] != 0)
            {   // direct + single indirect
	      logical_offset = 12 + (block_size / 4);
	      double_indirect(fd, block_size, i + 1, logical_offset, ino.i_block[13]);
            }

	  if (ino.i_block[14] != 0)
            {
	      // direct + single indirect + double indirect
	      logical_offset = 12 + (block_size / 4) + (block_size / 4) * (block_size / 4);
	      triple_indirect(fd, block_size, i + 1, logical_offset, ino.i_block[14]);
            }


        }
      offset += inode_size; // get next inode
    }
}

void free_bitmap(int fd, int block_bitmap, int block_size, int data_beg)
{
  // not -1 will be in the offset of inode map
  int bg_offset = BASE_OFFSET + (block_bitmap - 1) * block_size;
  char bitmap[block_size];

  // read bitmap all at once
  if (pread(fd, bitmap, block_size, bg_offset) == -1)
    {
      fprintf(stderr, "Block bitmap read failed.\n");
      exit(2);
    }

  for (int i = 0; i < block_size; i++)
    {
      int offset = 1;
      for (int j = 0; j < 8; j++) // each one is 8 bytes long
        {
	  if (!(bitmap[i] & offset)) // check set bit
            {
	      printf("BFREE,%d\n", data_beg + 8 * i + j); //calculate the actual block number
            }
	  offset <<= 1;
        }
    }
}

void free_inode(int fd, int inode_bitmap, int block_size, int data_beg)
{
  int inode_offset = BASE_OFFSET + (inode_bitmap - 1) * block_size;
  char inodemap[block_size];

  // read bitmap all at once
  if (pread(fd, inodemap, block_size, inode_offset) == -1)
    {
      fprintf(stderr, "Inode bitmap read failed.\n");
      exit(2);
    }

  for (int i = 0; i < block_size; i++)
    {
      int offset = 1;
      for (int j = 0; j < 8; j++) // each one is 8 bytes long
        {
	  if (!(inodemap[i] & offset)) // check set bit
            {
	      printf("IFREE,%d\n", data_beg + 8 * i + j); //calculate the actual block number
            }
	  offset <<= 1;
        }
    }
}

int main(int argc, char *argv[])
{
  struct ext2_super_block super;
  unsigned int blocks_count = 0; // number of block
  unsigned int inodes_count = 0; // number of inodes
  unsigned int block_size = 0; // block size
  unsigned int inode_size = 0;
  unsigned int blocks_per_gp = 0; // blocks per group
  unsigned int inodes_per_gp = 0; // inodes per group
  unsigned int first_inode = 0;// first non-reserved inode

  // check user input
  if (argc != 2)
    {
      fprintf(stderr, "Bad argument.\n");
      printf("usage: ./lab3a diskfile_name\n");
      exit(1);
    }
  // open image file
  int fd = open(argv[1], O_RDONLY);
  if (fd == -1)
    {
      fprintf(stderr, "couldn't open file %s\n", argv[1]);
      exit(1);
    }

  /* --------------- super block --------------- */
  if (pread(fd, &super, sizeof(super), BASE_OFFSET) != sizeof(super))
    {
      fprintf(stderr, "Superblock read failed.\n");
      exit(2);
    }

  blocks_count = super.s_blocks_count;
  inodes_count = super.s_inodes_count;
  block_size = EXT2_MIN_BLOCK_SIZE << super.s_log_block_size;
  inode_size = super.s_inode_size;
  blocks_per_gp = super.s_blocks_per_group;
  inodes_per_gp = super.s_inodes_per_group;
  first_inode = super.s_first_ino;

  printf("SUPERBLOCK,%u,%u,%u,%u,%u,%u,%u\n", blocks_count, inodes_count, block_size, inode_size, blocks_per_gp, inodes_per_gp, first_inode);

  /* --------------- Block Group Descriptor --------------- */
  struct ext2_group_desc group;
  unsigned int group_num = 0;
  unsigned int block_bitmap = 0; //block number of free block bitmap for this group
  unsigned int inode_bitmap = 0; //block number of free i-node bitmap for this group
  unsigned int first_blk_inode = 0; //block number of first block of i-nodes in this group

  off_t gdt_offset = BASE_OFFSET + block_size;

  group_num = blocks_count / blocks_per_gp + 1; // start from 0
  // traverse group descriptor table
  for (unsigned int i = 0; i < group_num; i++)
    {
      if (pread(fd, &group, sizeof(group), gdt_offset) != sizeof(group))
        {
	  fprintf(stderr, "Group %d descriptor read failed.\n", i);
	  exit(2);
        }

      block_bitmap = group.bg_block_bitmap;
      inode_bitmap = group.bg_inode_bitmap;
      first_blk_inode = group.bg_inode_table;

      printf("GROUP,%u,%u,%u,%u,%u,%u,%u,%u\n", i, blocks_count, inodes_count, group.bg_free_blocks_count, group.bg_free_inodes_count, block_bitmap, inode_bitmap, first_blk_inode);

      gdt_offset += 32; //next block group
    }

  /* --------------- Free Block Entries --------------- */
  unsigned int data_beg = super.s_first_data_block;
  free_bitmap(fd, block_bitmap, block_size, data_beg);

  /* --------------- Free Inode Entries --------------- */
  free_inode(fd, inode_bitmap, block_size, data_beg);

  /* --------------- Inode Information --------------- */
  unsigned int inode_table = group.bg_inode_table;
  inode(fd, block_size, inode_table, inode_size, inodes_count);

  close(fd);
  exit(0);
}

