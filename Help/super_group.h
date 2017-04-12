//description: read super block and validate the file system is ext2
//parameter:
//return:
int super_block()
{
	get_block(dev, 1, buf);							//read super block into buffer
	sp = (SUPER *)buf;							//cast buffer as super block

	if (sp->s_magic != 0xEF53){						//if the file system is not ext2 display error and exit
	  printf("ERROR: Device is not an EXT2 file system\n");
	  exit(1);
	}

	printf("s_magic = %x\n", sp->s_magic);

	printf("EXT2 FS OK\n\n");

	ninodes = sp->s_inodes_count;
	nblocks = sp->s_blocks_count;
	printf("s_inodes_count = %d\n", sp->s_inodes_count);
	printf("s_blocks_count = %d\n\n", sp->s_blocks_count);

	inodes_per_block = (1024 << sp->s_log_block_size) / sizeof(INODE);	//calculate inodes per block
	printf("inodes_per_block = %d\n", inodes_per_block);
}

//description: read group descriptor block
//parameter:
//return:
int group_descriptor()
{
	get_block(dev, 2, buf);		//read group descriptor block into buffer
	gp = (GD *)buf;			//cast buffer as group descriptor


	bmap = gp->bg_block_bitmap;
	imap = gp->bg_inode_bitmap;
	printf("bg_block_bitmap = %d\n", gp->bg_block_bitmap);
	printf("bg_inode_bitmap = %d\n\n", gp->bg_inode_bitmap);

	iblock = gp->bg_inode_table;
	printf("bg_inode_table = %d\n\n", iblock);

	get_block(dev, iblock, buf);	//read beginning inode block into buffer
	ip = (INODE *)buf + 1;		//cast buffer as inode pointer to the second inode (the root's inode)
}
