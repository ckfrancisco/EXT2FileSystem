//description: read block from file descriptor into buffer
//parameter: device file descriptor, block number, buffer
//return:
int get_block(int fd, int blk, char buf[ ])
{
	lseek(fd, (long)blk*BLKSIZE, 0);
	read(fd, buf, BLKSIZE);
}

//description: write buffer to block in file descriptor
//parameter: device file descriptor, block number, buffer
//return:
int put_block(int fd, int blk, char buf[ ])
{
	lseek(fd, (long)blk*BLKSIZE, 0);
	write(fd, buf, BLKSIZE);
}

//description: read inode from device into an minode
//parameter: device file descriptor and inode number
//return: pointer to an minode
MINODE *iget(int dev, int ino)
{
	for(int i = 0; i < NMINODE; i++)				//iterate through to find if inode from device exists within current minodes
	{
		MINODE *mip = &minode[i];

		if(mip->dev == dev && mip->ino == ino)		//if found increment reference count and return minode
		{
			mip->refCount++;
			return mip;
		}
	}
													
	for(int i = 0; i < NMINODE; i++)				//if not found allocate new minode to the first free space
	{												//iterate through minodes to find free space
		MINODE *mip = &minode[i];

		if(mip->refCount == 0)						//if space found assign minode values and return minode
		{
			mip->refCount = 1;
			mip->dev = dev;
			mip->ino = ino;
			mip->dirty = mip->mounted = mip->mntptr = 0;

			int blk  = (ino-1)/8 + iblock;			//calculate block and offset values
			int disp = (ino-1) % 8;					//NOTE: a set of inodes are stored in a blk
													//	use disp to determine offset of ino
													//	within blk

			char buf[BLKSIZE];
			get_block(dev, blk, buf);				//determine inode with blk and disp
			mip->inode = *((INODE *)buf + disp);	//assign inode to minode

			return mip;								//return minode
		}
	}

	printf("ERROR: No free space to allocate new minode\n");
	return 0;										//no space found display error and return null
}

//description: write inode from minode to device
//parameter: minode
//return:
int iput(MINODE *mip)
{
	mip->refCount--;

	if(mip->refCount > 0) return;			//if minode is being referenced or used return
	if(!mip->dirty) return;

	int blk  = (mip->ino-1)/8 + iblock;		//calculate block and offset values
	int disp = (mip->ino-1) % 8;			//NOTE: a set of inodes are stored in a blk
											//	use disp to determine offset of ino
											//	within blk

	char buf[BLKSIZE];
	get_block(mip->dev, blk, buf);			//determine inode with blk and disp
	*((INODE *)buf + disp) = mip->inode;	//copy minode into inode pointer within buffer

	put_block(mip->dev, blk, buf);			//write back to device
}

//description: write all inodes from minodes to device regardless of reference count or dirty
//parameter:
//return:
int iput_all()
{
	for(int i = 0; i < NMINODE; i++)		//write all minodes if they were used
		if(minode[i].dirty)
			iput(&minode[i]);
}

//description: determine the inode number of a name within an minode's directories
//parameter: minode and name
//return: inode number of name
int search(MINODE *mip, char *name)
{
	for(int dblk = 0; dblk < 12; dblk++)					//execute across all direct blocks within inode's inode table
	{
		if(!(mip->inode.i_block[dblk]))						//if empty block found return fail
			return -1;
	
		char buf[BLKSIZE];
		get_block(mip->dev, mip->inode.i_block[dblk], buf);	//read a directory block from the inode table into buffer
		DIR *dp = (DIR*)buf;								//cast buffer as directory pointer

		while((char*)dp < &buf[BLKSIZE])					//execute while there is another directory struct ahead
		{
			if(!strncmp(dp->name, name, dp->name_len)		//check if directory name matches name
				&& strlen(name) == dp->name_len)			//prevents . == ..
				return dp->inode;							//return inode number if found

			dp = (char*)dp + dp->rec_len;					//point to next directory struct within buffer
		}
	}

	return -1;												//return fail if name never found
}

//description: determine the inode number of a path on the deive
//parameter: device file descriptor and path name
//return: inode number of path name
int getino(int *dev, char *path)
{
	char names[NNAME][MAXNAME];								//tokenized names within pathname
	MINODE *mip;

	if (path[0] == '/')										//if absolute path set current minode to root inode
		mip = iget(*dev, 2);
	else
		mip = iget((running->cwd->dev), running->cwd->ino);	//else set current minode to cwd inode

	int n = tokenize(path, names);									//n = number of token strings
	int ino;

	for (int i = 0; i < n; i++)								//iterate through path tokens
	{
		if(mip->mounted)
		{
			if(mip == mip->mntptr->pmip)
			{
				
			}
		}

		ino = search(mip, names[i]);						//find inode number of name within minode's directories
		if(ino < 0)											//if name not found display error and return fail
		{
			printf("ERROR: %s not found\n", names[i]);
			iput(mip);
			return -1;
		}

		iput(mip);											//put back current minode and get the next minode
		mip = iget(*dev, ino);
	}

	ino = mip->ino;											//record inode number and put minode back
	iput(mip);
	return ino;												//return inode number
}

//description: determine name of minode using the parent minode
//parameter: parent minode, minode, and name buffer
//return:
int get_name(MINODE *pmip, int ino, char *name)
{
	if(ino == 2)												//if mip is root copy "/" to avoid when name is "." cases
	{ 															//	and return success
		strcpy(name, "/");
		return 1;
	}

	for(int dblk = 0; dblk < 12; dblk++)						//execute across all direct blocks within parent inode's inode table
	{
		if(!(pmip->inode.i_block[dblk]))						//if empty block found return fail
			return -1;
	
		char buf[BLKSIZE];
		get_block(pmip->dev, pmip->inode.i_block[dblk], buf);	//read a directory block from the inode table into buffer
		DIR *dp = (DIR*)buf;									//cast buffer as directory pointer

		while((char*)dp < &buf[BLKSIZE])						//execute while there is another directory struct ahead
		{
			if(dp->inode == ino)								//if inode found copy name into name and return
			{
				strncpy(name, dp->name, dp->name_len);
				name[dp->name_len] = 0;
				return 1;
			}

			dp = (char*)dp + dp->rec_len;						//point to next directory struct within buffer
		}
	}

	return -1;													//if all direct blocks searched return fail
}

//description: return the parent minode of an minode
//parameter: minode
//return: parent minode
MINODE *iget_parent(MINODE *mip)
{
	char buf[BLKSIZE];
	get_block(mip->dev, mip->inode.i_block[0], buf);	//read a directory block from the inode table into buffer

	DIR *dp = (DIR*)buf;								//cast buffer as directory pointer
	dp = (char*)dp + dp->rec_len;						//point to next directory struct within buffer

	MINODE *pmip = iget(mip->dev, dp->inode);			//get parent minode

	return pmip;										//return parent minode
}
