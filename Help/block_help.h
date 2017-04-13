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

//description: initialize global variables
//parameter:
//return:
int init_global()
{
	int i = 0;

	proc[0] = (PROC){.uid = 0, .cwd = 0};
	proc[1] = (PROC){.uid = 1, .cwd = 0};

	for(i = 0; i < 100; i++)
		minode[i] = (MINODE){.refCount = 0};

	root = 0;
}

//description: read inode from device into an minode
//parameter: device file descriptor and inode number
//return: pointer to an minode
MINODE *iget(int dev, int ino)
{
	int i;
	int blk;
	int disp;
	char buf[BLKSIZE];
	MINODE *mip;

	for(i = 0; i < NMINODE; i++)					//iterate through to find if inode from device exists within current minodes
	{
		mip = &minode[i];

		if(mip->dev == dev && mip->ino == ino)		//if found increment reference count and return *mip
		{
			mip->refCount++;
			return mip;
		}
	}
													//if not found allocate new minode to the first free space
	for(i = 0; i < NMINODE; i++)					//iterate through minodes to find free space
	{
		mip = &minode[i];

		if(mip->refCount == 0)						//if space found assign minode values and return *mip
		{
			mip->refCount = 1;						//assign minode values
			mip->dev = dev;
			mip->ino = ino;
			mip->dirty = mip->mounted = mip->mptr = 0;

			blk  = (ino-1)/8 + iblock;				//calculate block and offset values
			disp = (ino-1) % 8;						//NOTE: a set of inodes are stored in a blk
													//	use disp to determine offset of ino
													//	within blk

			get_block(dev, blk, buf);				//find inode with blk and disp
			(mip->inode) = *((INODE *)buf + disp);	//assign inode to minode

			return mip;								//return minode
		}
	}

	printf("ERROR: No free space to allocate new minode\n");	//no space found display error and return null
	return 0;
}

//description: write inode from minode to device
//parameter: minode
//return:
int iput(MINODE *mip)
{
	int blk;
	int disp;
	char buf[BLKSIZE];
	INODE *ip;

	mip->refCount--;

	if(mip->refCount > 0) return;	//if minode is being referenced or used return
	if(!mip->dirty) return;

	blk  = (mip->ino-1)/8 + iblock;	//calculate block and offset values
	disp = (mip->ino-1) % 8;		//NOTE: a set of inodes are stored in a blk
									//	use disp to determine offset of ino
									//	within blk

	get_block(mip->dev, blk, buf);	//copy minode into inode pointer within buffer
	ip = (INODE *)buf + disp;
	*ip = mip->inode;

	put_block(mip->dev, blk, buf);	//write back to deivde
}

//description: determine the inode number of a name within an inode's directories
//parameter: minode and name
//return: inode number of name
int search(MINODE *mip, char *name)
{
	int dblk;
	char buf[BLKSIZE];
	DIR *dp;
	char *cp;

	for(dblk = 0; dblk < 12; dblk++)						//execute across all direct blocks within inode's inode table
	{
		if(!(mip->inode.i_block[dblk]))						//return fail if empty block found
			return -1;

		get_block(mip->dev, mip->inode.i_block[dblk], buf);	//read a directory block from the inode table into buffer
		dp = (DIR*)buf;										//cast buffer as directory pointer
		cp = buf;											//cast buffer as "byte" pointer

		while(cp < &buf[BLKSIZE])							//execute while there is another directory struct ahead
		{
			if(!strncmp(dp->name, name, dp->name_len)		//check if directory name matches name
				&& strlen(name) == dp->name_len)			//prevents . == ..
				return dp->inode;							//return inode number if found

			cp += dp->rec_len;								//set variables to the next directory struct
			dp = (DIR*)cp;
		}
	}

	return -1;												//return fail if name not found
}

//description: determine the inode number of a path on the deive
//parameter: device file descriptor and path name
//return: inode number of path
int getino(int *dev, char *path)
{
	int i;
	int n;
	int ino;
	MINODE *mip;

	if (path[0] == '/')										//if absolute path set current minode to root inode
		mip = iget(*dev, 2);
	else
		mip = iget((running->cwd->dev), running->cwd->ino);	//else set current minode to cwd inode

	n = tokenize(path);										//n = number of token strings

	for (i = 0; i < n; i++)									//iterate through path tokens
	{
		ino = search(mip, names[i]);						//find inode number of name within minode's directories

		if(ino < 0)											//name not found display error and return fail
		{
			printf("\nERROR: %s not found\n", names[i]);
			return -1;
		}

		iput(mip);											//put back current minode and get the next
		mip = iget(*dev, ino);
	}

	ino = mip->ino;											//record inode number and put minode back
	iput(mip);
	return ino;												//return inode number
}

//description: determine name of minode using the parent minode
//parameter: parent minode, minode, and name buffer
//return:
int get_name(MINODE *pmip, MINODE *mip, char *name)
{
	int dblk;
	char buf[BLKSIZE];
	DIR *dp;
	char *cp;

	if(mip->ino == 2)											//if mip is root copy "/" to avoid when name is "." cases
	{ 															//	and return success
		strcpy(name, "/");
		return 1;
	}

	for(dblk = 0; dblk < 12; dblk++)							//execute across all direct blocks within inode's inode table
	{
		if(!(mip->inode.i_block[dblk]))							//return fail if empty block found
			return -1;

		get_block(pmip->dev, pmip->inode.i_block[dblk], buf);	//read a directory block from the inode table into buffer
		dp = (DIR*)buf;											//cast buffer as directory pointer
		cp = buf;												//cast buffer as "byte" pointer

		while(cp < &buf[BLKSIZE])								//execute while there is another directory struct ahead
		{
			if(dp->inode == mip->ino)							//if inode found copy name into name and return
			{
				strncpy(name, dp->name, dp->name_len);			//if inode found copy name and return success
				name[dp->name_len] = 0;
				return 1;
			}

			cp += dp->rec_len;									//set variables to the next directory struct
			dp = (DIR*)cp;
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
	MINODE *pmip;
	DIR *dp;
	char *cp;

	get_block(mip->dev, mip->inode.i_block[0], buf);	//read a directory block from the inode table into buffer

	dp = (DIR*)buf;										//cast buffer as directory pointer
	cp = buf;											//cast buffer as "byte" pointer
	cp += dp->rec_len;									//iterate to ".." directory
	dp = (DIR*)cp;

	pmip = iget(mip->dev, dp->inode);					//get parent minode

	return pmip;										//return parent minode
}
