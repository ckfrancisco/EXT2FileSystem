//description: create directory determined by path
//parameter: path name
//return: success or fail
int mk_dir(char *path)
{
	if(path[0] == '/')						//initialize device depending on absolute or relative path
		dev = root->dev;
	else
		dev = running->cwd->dev;

	char newdirectory[MAXNAME];
	char newbase[MAXNAME];
	det_dirname(path, newdirectory);
	det_basename(path, newbase);

	int pino = getino(&dev, newdirectory);	//detmine parent inode number
	if(pino < 0)							//if parent directory not found display error and return fail
	{
		return -1;
	}

	MINODE *pmip = iget(dev, pino);			//determine parent minode
	if(!S_ISDIR(pmip->inode.i_mode))		//if parent minode is not a directory display error and  return fail
	{
		printf("ERROR: %s is not a directory\n", newdirectory);
		iput(pmip);
		return -1;
	}

	if(search(pmip, newbase) > 0)			//if name already exists display error and return fail
	{
		printf("ERROR: %s already exists\n", newbase);
		iput(pmip);
		return -1;
	}

	pmip->inode.i_links_count++;			//update parent minode
	pmip->inode.i_atime = time(0L);
	pmip->dirty = 1;

	int result =  my_mkdir(pmip, newbase);	//return the success or fail of making the directory

	iput(pmip);								//put parent minode back

	return result;
}

//description: allocate directory determined by path
//parameter: parent minode and base name
//return: success or fail
int my_mkdir(MINODE *pmip, char *base)
{
	int ino = ialloc(pmip->dev);			//find free inode
	if(ino < 0)								//if none avialble return fail
		return -1;

	int bno = balloc(pmip->dev);			//find free block
	if(bno < 0)								//if none available deallocate inode and return fail
	{
		idealloc(pmip->dev,  ino);
		return -1;
	}

	MINODE *mip= iget(pmip->dev, ino);		//determine minode of new directory
	if(!mip)								//if minode not found deallocate and return fail
	{
		idealloc(pmip->dev, ino);
		bdealloc(pmip->dev, bno);
		return -1;
	}

	mip->inode.i_mode = 0x41ED;				//assign new minode directory values
	mip->inode.i_uid = running->cwd->inode.i_uid;
	mip->inode.i_gid = running->cwd->inode.i_gid;
	mip->inode.i_size = BLKSIZE;			//set size to BLKSIZE since it contains only one block
	mip->inode.i_links_count = 2;
	mip->inode.i_atime = mip->inode.i_ctime = mip->inode.i_mtime = time(0L);
	mip->inode.i_blocks = 2;				//two for two 512 bytes
	mip->inode.i_block[0] = bno;			//set first data block to block number

	for(int i = 1; i < 15; i++)				//set all other blocks to null
		mip->inode.i_block[i] = 0;

	mip->dirty = 1;
	iput(mip);								//write minode to device

	char buf[BLKSIZE];
	DIR *dp = (DIR*)buf;

	dp->inode = mip->ino;					//assign this and parent directories
	dp->name_len = 1;
	dp->rec_len = 4 * ((8 + dp->name_len + 3) / 4);
	strcpy(dp->name, ".");

	dp = (char*)dp + dp->rec_len;			//point to next directory struct within buffer

	dp->inode = pmip->ino;
	dp->name_len = 2;
	dp->rec_len = 4 * ((8 + dp->name_len + 3) / 4);
	strcpy(dp->name, "..");

	put_block(mip->dev, bno, buf);			//write data block to device

	return enter_name(pmip, ino, base);		//return success or fail of entering name in parent minode
}

//description: create file detmined by path
//parameter: path name
//return: success or fail
int creat_file(char *path)
{
	if(path[0] == '/')							//initialize device depending on absolute or relative path
		dev = root->dev;
	else
		dev = running->cwd->dev;

	char newdirectory[MAXNAME];
	char newbase[MAXNAME];
	det_dirname(path, newdirectory);
	det_basename(path, newbase);

	int pino = getino(&dev, newdirectory);		//detmine parent inode number
	if(pino < 0)								//if parent directory not found return fail
	{
		return -1;
	}

	MINODE *pmip = iget(dev, pino);				//determine parent minode
	if(!S_ISDIR(pmip->inode.i_mode))			//if parent minode is not a directory display error and return fail
	{
		printf("ERROR: %s is not a directory\n", newdirectory);
		iput(pmip);
		return -1;
	}

	if(search(pmip, newbase) > 0)				//if name already exists display error and return fail
	{
		printf("ERROR: %s already exists\n", newbase);
		iput(pmip);
		return -1;
	}

	pmip->inode.i_atime = time(0L);				//update parent minode
	pmip->dirty = 1;

	int result =  my_creat(pmip, newbase);		//return the success or fail of creating the file

	iput(pmip);									//put parent minode back

	return result;
}

//description: allocate file detmined by path
//parameter: parent minode and base name
//return: success or fail
int my_creat(MINODE *pmip, char *base)
{
	int ino = ialloc(pmip->dev);			//find free inode

	MINODE *mip = iget(pmip->dev, ino);
	if(!mip)								//if minode not found return fail
		return -1;

	mip->inode.i_mode = 0x81A4;				//assign minode file values
	mip->inode.i_uid = running->cwd->inode.i_uid;
	mip->inode.i_gid = running->cwd->inode.i_gid;
	mip->inode.i_size = 0;
	mip->inode.i_links_count = 1;
	mip->inode.i_atime = mip->inode.i_ctime = mip->inode.i_mtime = time(0L);
	mip->inode.i_blocks = 0;				//set all data blocks to null

	for(int i = 0; i < 15; i++)
		mip->inode.i_block[i] = 0;

	mip->dirty = 1;
	iput(mip);

	return enter_name(pmip, ino, base);		//return sucess or fail of enterting name in parent minode
}

//description: enter inode in parent minode
//parameter: parent minodem, inode number, and path
//return: success or fail
int enter_name(MINODE *pmip, int ino, char *name)
{
	int dblk;
	char buf[BLKSIZE];
	int need_len = 4 * ((8 + strlen(name) + 3) / 4);					//calculate required length for directory

	for(dblk = 0; dblk < 12; dblk++)									//execute across all direct blocks within inode's inode table
	{
		if(!pmip->inode.i_block[dblk])									//break if empty block found
			break;

		get_block(pmip->dev, pmip->inode.i_block[dblk], buf);			//read a directory block from the inode table into buffer
		DIR *dp = (DIR*)buf;											//cast buffer as directory pointer

		while((char*)dp + dp->rec_len < &buf[BLKSIZE])					//iterate to last directory in data block
			dp = (char*)dp + dp->rec_len;								//point to next directory struct within buffer

		int remain = dp->rec_len - (4 * ((8 + dp->name_len + 3) / 4));	//calculate actual remaining space in data block

		if(remain >= need_len)											//if remaining space is greater than the required space then insert
		{
			dp->rec_len = 4 * ((8 + dp->name_len + 3) / 4);				//update the rec_len of the directory prior

			dp = (char*)dp + dp->rec_len;								//point to next directory struct within buffer

			dp->inode = ino;											//create new directory entry with values
			dp->rec_len = remain;
			dp->name_len = strlen(name);

			strncpy(dp->name, name, dp->name_len);

			put_block(pmip->dev, pmip->inode.i_block[dblk], buf);		//write data block to device

			return ino;													//return success
		}
	}

	if(dblk < 12 && pmip->inode.i_block[dblk] == 0)						//if empty block found then create a new data block
	{
		int pbno = balloc(pmip->dev);									//allocate new data block
		if(pbno < 0)													//if none available return fail
			return -1;

		pmip->inode.i_block[dblk] = pbno;								//assign block number to minode data table

		pmip->inode.i_size += BLKSIZE;

		get_block(pmip->dev, pmip->inode.i_block[dblk], buf);			//read a directory block from the inode table into buffer
		DIR* dp = (DIR*)buf;													//cast buffer as directory pointer

		dp->inode = ino;												//create new directory with values
		dp->rec_len = BLKSIZE;
		dp->name_len = strlen(name);

		strncpy(dp->name, name, dp->name_len);

		put_block(pmip->dev, pbno, buf);								//write data block to device

		return ino;														//return success
	}

	printf("ERROR: no directory space for %s\n", name);					//no directory space available display error and return fail
	return -1;
}
