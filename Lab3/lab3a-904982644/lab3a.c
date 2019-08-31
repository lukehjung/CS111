/*
NAME: Luke Jung, Justin Jeon
EMAIL: lukejung@ucla.edu, justinjeon151@gmail.com
ID: 904982644, 205008698
*/

#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <time.h>
#include <sys/types.h>
#include "ext2_fs.h"

#define BYTE_SIZE 8

// globals
int fd;
struct ext2_super_block super;
uint32_t groups;
uint32_t blocks_count;
uint32_t inodes_count;
uint32_t block_size;
uint32_t inode_size;
uint32_t blocks_per_group;
uint32_t inodes_per_group;
uint32_t first_data_block;

// function declaration
void superblock();
void format_time(time_t sec, char * output);
void group_summary(uint32_t group);
void free_block_entries(uint32_t block_bitmap, uint32_t group);
void free_inode_entries(uint32_t inode_bitmap, uint32_t inode_table, uint32_t group);
void inode_summary(uint32_t inode_table, uint32_t index, uint32_t curr);
void directory_entries(uint32_t inode, uint32_t block);
void indirect_blocks(uint32_t curr, uint32_t i_block, int level, uint32_t offset);

int main(int argc, char ** argv)
{
    if (argc > 2) 
    {
        fprintf(stderr, "Error: Too many arguments.\n");
        exit(1);
    }

    fd = open(argv[1], O_RDONLY);

    if (fd == -1) 
    {
        fprintf(stderr, "Error: Unable to open file.\n");
        exit(1);
    }

    superblock();

    for (uint32_t i = 0; i < groups; i++)
        group_summary(i);

    exit(0);
}

void superblock()
{
	// pread to read offset of where the superblock is
	pread(fd, &super, sizeof(super), 1024);
	blocks_count = super.s_blocks_count;
    inodes_count = super.s_inodes_count;
    // equation found from http://cs.smith.edu/~nhowe/262/oldlabs/ext2.html
    block_size = EXT2_MIN_BLOCK_SIZE << super.s_log_block_size; 
    inode_size = super.s_inode_size;
    blocks_per_group = super.s_blocks_per_group;
    inodes_per_group = super.s_inodes_per_group;
    first_data_block = super.s_first_data_block;
    printf("SUPERBLOCK,%u,%u,%u,%u,%u,%u,%u\n",
    	blocks_count, // total number of blocks
        inodes_count, // total number of inodes
    	block_size, // block size
        inode_size,	 // inode size
        blocks_per_group,  // blocks per group
        inodes_per_group, // inodes per group
        super.s_first_ino // first non-reserved inode
        );

    // round up number of groups
   	groups = ceil((double) blocks_count/blocks_per_group); 

}

void format_time(time_t sec, char * output)
{
	struct tm * time = gmtime(&sec);
	strftime(output, 18, "%m/%d/%y %H:%M:%S", time);
}

void group_summary(uint32_t group)
{
	struct ext2_group_desc group_desc;
	// pread offset away from superblock, then however many group sizes
	pread(fd, &group_desc, sizeof(group_desc), 1024 + block_size + group * sizeof(group_desc));

	uint32_t num_blocks = super.s_blocks_per_group;
	uint32_t remainder_blocks = super.s_blocks_count % num_blocks; 
	if (remainder_blocks && group == groups - 1)
	{
		num_blocks = remainder_blocks;
	}

	uint32_t num_inodes = super.s_inodes_per_group;
	uint32_t remainder_inodes = super.s_inodes_count % num_inodes; 
	if (remainder_inodes && group == groups - 1 )
	{
		num_inodes = remainder_inodes;
	}

	fprintf(stdout, "GROUP,%u,%u,%u,%u,%u,%u,%u,%u\n",
			group, // group number 
			num_blocks, // total number of blocks in this group
			num_inodes, //total number of inodes in this group
			group_desc.bg_free_blocks_count, // number of free blcoks
			group_desc.bg_free_inodes_count, //number of free inodes
			group_desc.bg_block_bitmap, // block number of free blcok bitmap
			group_desc.bg_inode_bitmap, // block number of free inode bitmap
			group_desc.bg_inode_table // block number of first  block of inode in group
		);

	// check for free entries now that group bitmaps have been extracted 
	free_block_entries(group_desc.bg_block_bitmap, group);
    free_inode_entries(group_desc.bg_inode_bitmap, group_desc.bg_inode_table, group);
}

void free_block_entries(uint32_t block_bitmap, uint32_t group)
{
	char buf[block_size];
	// pread at offset where bitmap is located
	pread(fd, buf, block_size, block_bitmap * block_size);
	uint32_t start = first_data_block + group * blocks_per_group;

	// doubly listed for loop to parse through bitmap
	for(uint32_t i = 0; i < block_size; i ++)
	{
		char c = buf[i];
		for(uint32_t j = 0; j < BYTE_SIZE; j ++)
		{
			// if c == 0, report which are free
			if(!(c & 1))
				fprintf(stdout, "BFREE,%u\n", start);
			start++;
			c >>= 1;
		}
	}
}

void free_inode_entries(uint32_t inode_bitmap, uint32_t inode_table, uint32_t group)
{
	uint32_t length = inodes_per_group / BYTE_SIZE;
	char buf[length];
	// pread to where inode bitmpap is located
	pread(fd, buf, length, inode_bitmap * block_size);
	uint32_t first_block = group * inodes_per_group + 1;
	uint32_t curr_block = first_block;

	for(uint32_t i = 0; i < length; i ++)
	{
		char c = buf[i];
		for(uint32_t j = 0; j < BYTE_SIZE; j ++)
		{
			if(!(c & 1)) // if first bit is 0, print block
				fprintf(stdout, "IFREE,%u\n", curr_block);
			else // if block is being used, then print summary info for it
				inode_summary(inode_table, curr_block - first_block, curr_block);
			curr_block ++;
			c >>= 1;
		}
	}
}

void inode_summary(uint32_t inode_table, uint32_t index, uint32_t curr)
{
	struct ext2_inode inode;
	// pread inode that is being used to see what kind of inode it is
	pread(fd, &inode, sizeof(inode), inode_table * block_size + index * sizeof(inode));

	// check what kind of inode it is and how many links
	uint16_t mode = inode.i_mode;
	uint16_t links = inode.i_links_count;
	char file = 'n';

	if (mode && links)
	{
		// Use '&' because the mode can be combined. List of constants can be found in 
		// https://www.nongnu.org/ext2-doc/ext2.html#I-MODE
		if((mode & 0x8000) == 0x8000)  
			file = 'f';
		else if((mode & 0x4000) == 0x4000)
			file = 'd';
		else if((mode & 0xA000) == 0xA000)
			file = 's';

		char create[18], modified[18], access[18];
		format_time(inode.i_ctime, create);
		format_time(inode.i_mtime, modified);
		format_time(inode.i_atime, access);

		fprintf(stdout, "INODE,%u,%c,%o,%u,%u,%u,%s,%s,%s,%u,%u",
				curr, // inode number
				file, // file type
				inode.i_mode & 0x0FFF, // mode (take only the low 12 bits)
				inode.i_uid, // owner
				inode.i_gid, // group
				inode.i_links_count, // link count
				create, //time of last I-Node change
				modified, // modification time
				access, //time of last access
				inode.i_size, // file size
				inode.i_blocks //number of blocks
			);

		// in spec, if a symbolic link with more than 60 bytes, a file type, or a directory,
		// print next 15 parameters
		if ( (file == 's' && inode.i_size > 60) || file == 'f' || file == 'd' ) 
		{
			for(uint32_t i = 0; i < EXT2_N_BLOCKS; i ++) 
			{
				fprintf(stdout, ",%u", inode.i_block[i]); // prints block addresses
			}
		}

		fprintf(stdout, "\n");

		// print directory entry blocks
		for (uint32_t i = 0; i < EXT2_NDIR_BLOCKS; i ++)
		{
			if(inode.i_block[i])
				if(file == 'd')
					directory_entries(curr, inode.i_block[i]);
		}

		// print indirect references, whether it be single, double, or triple indirect
		if (inode.i_block[EXT2_IND_BLOCK])
	        indirect_blocks(curr, inode.i_block[EXT2_IND_BLOCK], 1, 12);
	    if (inode.i_block[EXT2_DIND_BLOCK])
	        indirect_blocks(curr, inode.i_block[EXT2_DIND_BLOCK], 2, 268);
	    if (inode.i_block[EXT2_TIND_BLOCK])
	        indirect_blocks(curr, inode.i_block[EXT2_TIND_BLOCK], 3, 65804);
	}
}

void directory_entries(uint32_t inode, uint32_t block)
{
	struct ext2_dir_entry dir;
	for(uint32_t i = 0; i < block_size; i += dir.rec_len)
	{
		// pread each inode to extract info of each directory
		pread(fd, &dir, sizeof(dir), block * block_size + i);
		if(dir.inode)
		{
			fprintf(stdout, "DIRENT,%u,%u,%u,%u,%u,'%s'\n",
					inode, // parent inod enumber
					i, // logical byte offset
					dir.inode, // inode number of referenced file
					dir.rec_len, // entry length
					dir.name_len, // name length
					dir.name // name
					);
		}
	}
}

void indirect_blocks(uint32_t curr, uint32_t i_block, int level, uint32_t offset) 
{
    uint32_t blocks_count = block_size/sizeof(uint32_t);
    uint32_t buf[blocks_count];

    // set each value of buf to 0
    for (uint32_t i = 0; i < blocks_count; i++)
        buf[i] = 0;
    // pread into where each path is located
    pread(fd, buf, block_size, 1024 + (i_block - 1) * block_size);
    for (uint32_t i = 0; i < blocks_count; i++) 
    {
		if (buf[i]) 
		{
			printf("INDIRECT,%u,%u,%u,%u,%u\n",
				   curr, // I-node number of owning file
				   level, // level of indirection
 				   offset+i, // logical block offset
				   i_block, // block number of the indirect block
				   buf[i] // block number of the referenced block
				   );

			if (level == 2)
			{
				// use recursion to find next address and subtract level by 1 
				// and increase offset on basis of level from Piazza post
				indirect_blocks(curr, buf[i], level-1, offset); 
				offset+=256;
			}

			else if (level == 3) 
			{
				indirect_blocks(curr, buf[i], level-1, offset);
				offset+= 65536;
			}
		}
	}
}