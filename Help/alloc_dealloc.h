//description: test bit value in byte array
//parameter: byte array and bit location
//return: return bit value
int tst_bit(char *buf, int bit)
{
	int i;
	int j;
	i = bit/8; j=bit%8;	//determine index of byte and bit
	if (buf[i] & (1 << j))
		return 1;
	return 0;
}

//description: set bit value in byte array
//parameter: byte array and bit location
//return:
int set_bit(char *buf, int bit)
{
	int i;
	int j;
	i = bit/8; j=bit%8;	//determine index of byte and bit
	buf[i] |= (1 << j);
}

//description: clear bit value in byte array
//parameter: byte array and bit location
//return:
int clr_bit(char *buf, int bit)
{
	int i;
	int j;
	i = bit/8; j=bit%8;	//determine index of byte and bit
	buf[i] &= ~(1 << j);
}

//description: decrement amount of free inodes on device
//parameter: device file descriptor
//return:
int dec_free_inodes(int dev)
{
	char buf[BLKSIZE];

	get_block(dev, 1, buf);		//find super block
	sp = (SUPER *)buf;
	sp->s_free_inodes_count--;	//decrement amount free inodes
	put_block(dev, 1, buf);		//write back to device

	get_block(dev, 2, buf);		//repeat for group descriptor block
	gp = (GD *)buf;
	gp->bg_free_inodes_count--;
	put_block(dev, 2, buf);
}

//description: decrement amount of free blocks on device
//parameter: device file descriptor
//return:
int dec_free_blocks(int dev)
{
	char buf[BLKSIZE];

	get_block(dev, 1, buf);		//find super block
	sp = (SUPER *)buf;
	sp->s_free_blocks_count--;	//decrement amount free blocks
	put_block(dev, 1, buf);		//write back to device

	get_block(dev, 2, buf);		//repeat for group descriptor block
	gp = (GD *)buf;
	gp->bg_free_blocks_count--;
	put_block(dev, 2, buf);
}

//description: increment amount of free inodes on device
//parameter: device file descriptor
//return:
int inc_free_inodes(int dev)
{
	char buf[BLKSIZE];

	get_block(dev, 1, buf);		//find super block
	sp = (SUPER *)buf;
	sp->s_free_inodes_count++;	//increment amount free inodes
	put_block(dev, 1, buf);		//write back to device

	get_block(dev, 2, buf);		//repeat for group descriptor block
	gp = (GD *)buf;
	gp->bg_free_inodes_count++;
	put_block(dev, 2, buf);
}

//description: increment amount of free blocks on device
//parameter: device file descriptor
//return:
int inc_free_blocks(int dev)
{
	char buf[BLKSIZE];

	get_block(dev, 1, buf);		//find super block
	sp = (SUPER *)buf;
	sp->s_free_blocks_count++;	//increment amount free blocks
	put_block(dev, 1, buf);		//write back to device

	get_block(dev, 2, buf);		//repeat for group descriptor block
	gp = (GD *)buf;
	gp->bg_free_blocks_count++;
	put_block(dev, 2, buf);
}

//description: allocate inode on imap
//parameter: device file descriptor
//return:
int ialloc(int dev)
{
	int  i;
	char buf[BLKSIZE];

	get_block(dev, imap, buf);			//read imap to buffer

	for (i=0; i < ninodes; i++){		//set first 0 bit
		if (tst_bit(buf, i)==0){
			set_bit(buf,i);
			dec_free_inodes(dev);		//decrement amount of free inodes

			put_block(dev, imap, buf);	//write back to device

			return i+1;					//return index of inode bit allocated
		}
	}
	printf("ialloc(): no more free inodes\n");	//if no available inodes display error and return fail
	return -1;
}

//description: allocate block on bmap
//parameter: device file descriptor
//return:
int balloc(int dev)
{
	int  i;
	char buf[BLKSIZE];

	get_block(dev, bmap, buf);			//read bmap into buffer

	for (i=0; i < nblocks; i++){		//set first 0 bit
		if (tst_bit(buf, i)==0){
			set_bit(buf,i);
			dec_free_blocks(dev);		//decrement amount of free blocks

			put_block(dev, bmap, buf);	//write back to device

			return i+1;					//return index of block bit allocated
		}
	}

	printf("balloc(): no more free blocks\n");	//if no available blocks display error and return fail
	return -1;
}

//description: allocate inode on imap
//parameter: device file descriptor
//return:
int idealloc(int dev, int ino)
{
	char buf[BLKSIZE];

	get_block(dev, imap, buf);		//read imap into buffer

	clr_bit(buf,ino);				//clear ino bit
	inc_free_inodes(dev);			//increment amount of free inode

	put_block(dev, imap, buf);		//write back to device

	return 0;
}

//description: allocate block on bmap
//parameter: device file descriptor
//return:
int bdealloc(int dev, int bno)
{
	char buf[BLKSIZE];

	get_block(dev, bmap, buf);		//read bmap into buffer

	clr_bit(buf,bno);				//clear bno bit
	inc_free_blocks(dev);			//increment amount of free block

	put_block(dev, bmap, buf);		//write back to device

	return 0;
}
