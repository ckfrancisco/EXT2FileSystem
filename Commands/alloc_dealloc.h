//description: test bit value in byte array
//parameter: byte array and bit location
//return: return bit value
int tst_bit(char *buf, int bit)
{
	int i;
	int j;
	i = bit/8; j=bit%8;
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
	i = bit/8; j=bit%8;
	buf[i] |= (1 << j);
}

//description: clear bit value in byte array
//parameter: byte array and bit location
//return:
int clr_bit(char *buf, int bit)
{
	int i;
	int j;
	i = bit/8; j=bit%8;
	buf[i] &= ~(1 << j);
}

//description: decrement amount of free inodes on device
//parameter: device file descriptor
//return:
int decFreeInodes(int dev)
{
	char buf[BLKSIZE];

	// dec free inodes count in SUPER and GD
	get_block(dev, 1, buf);
	sp = (SUPER *)buf;
	sp->s_free_inodes_count--;
	put_block(dev, 1, buf);

	get_block(dev, 2, buf);
	gp = (GD *)buf;
	gp->bg_free_inodes_count--;
	put_block(dev, 2, buf);
}

//description: decrement amount of free blocks on device
//parameter: device file descriptor
//return:
int decFreeBlocks(int dev)
{
	char buf[BLKSIZE];

	// dec free inodes count in SUPER and GD
	get_block(dev, 1, buf);
	sp = (SUPER *)buf;
	sp->s_free_blocks_count--;
	put_block(dev, 1, buf);

	get_block(dev, 2, buf);
	gp = (GD *)buf;
	gp->bg_free_blocks_count--;
	put_block(dev, 2, buf);
}

//description: increment amount of free inodes on device
//parameter: device file descriptor
//return:
int incFreeInodes(int dev)
{
	char buf[BLKSIZE];

	// dec free inodes count in SUPER and GD
	get_block(dev, 1, buf);
	sp = (SUPER *)buf;
	sp->s_free_inodes_count++;
	put_block(dev, 1, buf);

	get_block(dev, 2, buf);
	gp = (GD *)buf;
	gp->bg_free_inodes_count++;
	put_block(dev, 2, buf);
}

//description: increment amount of free blocks on device
//parameter: device file descriptor
//return:
int incFreeBlocks(int dev)
{
	char buf[BLKSIZE];

	// dec free inodes count in SUPER and GD
	get_block(dev, 1, buf);
	sp = (SUPER *)buf;
	sp->s_free_blocks_count++;
	put_block(dev, 1, buf);

	get_block(dev, 2, buf);
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

	// read inode_bitmap block
	get_block(dev, imap, buf);			//read imap to buffer

	for (i=0; i < ninodes; i++){			//set first 0 bit
		if (tst_bit(buf, i)==0){
			set_bit(buf,i);
			decFreeInodes(dev);		//decrement amount of free inodes

			put_block(dev, imap, buf);	//write buffer to imap

			return i+1;
		}
	}
	printf("ialloc(): no more free inodes\n");
	return 0;
}

//description: allocate block on bmap
//parameter: device file descriptor
//return:
int balloc(int dev)
{
	int  i;
	char buf[BLKSIZE];

	// read block_bitmap block
	get_block(dev, bmap, buf);			//read bmap into buffer

	for (i=0; i < nblocks; i++){			//set first 0 bit
		if (tst_bit(buf, i)==0){
			set_bit(buf,i);
			decFreeBlocks(dev);		//decrement amount of free block

			put_block(dev, bmap, buf);	//write buffer to bmip

			return i+1;
		}
	}
	printf("balloc(): no more free blocks\n");
	return 0;
}

//description: allocate inode on imap
//parameter: device file descriptor
//return:
int idealloc(int dev, int ino)
{
	char buf[BLKSIZE];

	get_block(dev, imap, buf);			//read imap into buffer

	clr_bit(buf,ino);				//clear ino bit
	incFreeInodes(dev);				//increment amount of free inode

	put_block(dev, imap, buf);			//write buffer to imap

	printf("ialloc(): no more free inodes\n");
	return 0;
}

//description: allocate block on bmap
//parameter: device file descriptor
//return:
int bdealloc(int dev, int bno)
{
	char buf[BLKSIZE];

	get_block(dev, bmap, buf);			//read bmap into buffer

	clr_bit(buf,bno);				//clear bno bit
	incFreeBlocks(dev);				//increment amount of free block

	put_block(dev, bmap, buf);			//write buffer to imap

	printf("balloc(): no more free blocks\n");
	return 0;
}
