//description: remove directory determined by path
//parameter: path
//return: success or fail
int rm_dir(char *path)
{
	if(path[0] == '/')										//initialize device depending on absolute or relative path
		dev = root->dev;
	else
		dev = running->cwd->dev;

	int ino = getino(&dev, path);							//determine inode number of path
	if(ino < 0)												//if inode number not found return fail
	{
		return -1;
	}

	char rmdirectory[MAXNAME];
	char rmbase[MAXNAME];
	det_dirname(path, rmdirectory);
	det_basename(path, rmbase);

	MINODE *mip = iget(dev, ino);							//create minode
	if(!S_ISDIR(mip->inode.i_mode))							//if minode is not a directory display error and return fail
	{
		printf("ERROR: %s is not a directory\n", rmbase);
		iput(mip);
		return -1;
	}

	if(mip->refCount > 2)									//if minode is still being referenced display error and return fail
	{
		printf("ERROR: %s is still being used\n", rmbase);
		iput(mip);
		return -1;
	}

	if(mip->inode.i_links_count > 2)						//if minode contains other directories display error and return fail
	{
		printf("ERROR: %s still contains directories\n", rmbase);
		iput(mip);
		return -1;
	}

	if(mip->inode.i_links_count == 2)						//if minode does not contain other directories check for files
	{
		char buf[BLKSIZE];
		get_block(mip->dev, mip->inode.i_block[0], buf);	//read a directory block from the inode table into buffer

		DIR *dp = (DIR*)buf;								//cast buffer as directory pointer
		int prev_len = dp->rec_len;
		dp = (char*)dp + dp->rec_len;						//point to next directory struct within buffer

		if(prev_len + dp->rec_len < BLKSIZE)				//if minode contains files display error and return fail
		{
			printf("ERROR: %s still contains files\n", rmbase);
			iput(mip);
			return -1;
		}
	}

	for (int i = 0; i < 12; i++)							//deallocate all block numbers
	{
		if (mip->inode.i_block[i] == 0)
			continue;
		bdealloc(mip->dev, mip->inode.i_block[i]);
	}

	idealloc(mip->dev, mip->ino);							//deallocate inode number

	MINODE *pmip = iget_parent(mip);						//determine parent minode
	iput(mip);

	rm_child(pmip, rmbase);									//remove name from parent minode directory
	pmip->inode.i_links_count--;							//update paren minode
	pmip->dirty = 1;
	pmip->inode.i_atime = pmip->inode.i_mtime = time(0L);
	iput(pmip);

	return 1;
}

//description: remove directory determined by path
//parameter: path
//return: success or fail
int rm_file(char *path)
{
	if(path[0] == '/')												//initialize device depending on absolute or relative path
		dev = root->dev;
	else
		dev = running->cwd->dev;

	int ino = getino(&dev, path);									//determine inode number of path
	if(ino < 0)														//if inode number not found return fail
	{
		return -1;
	}

	char rmdirectory[MAXNAME];
	char rmbase[MAXNAME];
	det_dirname(path, rmdirectory);
	det_basename(path, rmbase);

	MINODE *mip = iget(dev, ino);									//create minode
	if(!S_ISREG(mip->inode.i_mode) && !S_ISLNK(mip->inode.i_mode))	//if minode is not a directory display error and return fail
	{
		printf("ERROR: %s is not a file\n", rmbase);
		iput(mip);
		return -1;
	}

	mip->inode.i_links_count--;										//decrement amount of links
	mip->dirty = 1;													//mark minode as dirty

	if(mip->inode.i_links_count == 0)								//if file no longer used deallocate inode number
	{
		truncate(mip);												//truncate data blocks and deallocate inode number
		idealloc(mip->dev, mip->ino);
	}

	int pino = getino(&dev, rmdirectory);							//determine parent inode of file
	if(pino < 0)													//if parent inode not found display error and return fail
	{
		iput(mip);
		return -1;
	}

	MINODE *pmip = iget(dev, pino);									//get parent minode of file
	if(!S_ISDIR(pmip->inode.i_mode))								//if parent minode is not a directory display error and return fail
	{
		printf("ERROR: %s is not a directory\n", rmdirectory);
		iput(mip);
		iput(pmip);
		return -1;
	}

	iput(mip);

	rm_child(pmip, rmbase);											//remove name from parent minode directory
	pmip->inode.i_links_count--;									//update paren minode
	pmip->dirty = 1;
	pmip->inode.i_atime = pmip->inode.i_mtime = time(0L);
	iput(pmip);
}

//description: remove child from parent directory
//parameter: parent minode and path
//return: success or fail
int rm_child(MINODE *pmip, char *base)
{
	for(int dblk = 0; dblk < 12; dblk++)												//execute across all direct blocks within inode's inode table
	{
		if(!pmip->inode.i_block[dblk])													//break if empty block found
			return -1;

		char buf[BLKSIZE];
		get_block(pmip->dev, pmip->inode.i_block[dblk], buf);							//read a directory block from the inode table into buffer
		DIR *pdp = (DIR*)buf;															//cast buffer as previous directory pointer
		DIR *dp = (DIR*)buf;															//cast buffer as directory pointer

		if(!strncmp(dp->name, base, dp->rec_len)
			&& dp->name_len == strlen(base)
			&& (char*)dp + dp->rec_len >= &buf[BLKSIZE])								//remove first and only directory in data block
		{
			bdealloc(pmip->dev, pmip->inode.i_block[dblk]);								//deallocate block number
			pmip->inode.i_size -= BLKSIZE;												//decrement parent minode size by BLKSIZE

			for(dblk; pmip->inode.i_block[dblk + 1] && dblk < 11; dblk++)				//shift all directory blocks back one index
			{
				pmip->inode.i_block[dblk] = pmip->inode.i_block[dblk + 1];
			}

			pmip->inode.i_block[dblk] = 0;												//set last directory block to 0
			
			put_block(pmip->dev, pmip->inode.i_block[dblk], buf);						//write data block to device
			return 1;
		}

		while((char*)dp + dp->rec_len < &buf[BLKSIZE])									//execute until last directory encountered
		{
			if(!strncmp(dp->name, base, dp->rec_len) && dp->name_len == strlen(base))	//remove middle directory in data block
			{
				int remove_len = dp->rec_len;											//record the rec_len of the directory being removed
				pdp = dp;																//remember previous directory struct
				dp = (char*)dp + dp->rec_len;											//point to next directory struct within buffer

				while((char*)pdp < &buf[BLKSIZE])
				{
					pdp->inode = dp->inode;												//move directory back
					pdp->rec_len = dp->rec_len;

					if((char*)dp + dp->rec_len >= &buf[BLKSIZE])
						pdp->rec_len += remove_len;

					pdp->name_len = dp->name_len;

					strncpy(pdp->name, dp->name, pdp->name_len);

					pdp = (char*)pdp + pdp->rec_len;									//point to next directory struct within buffer
					dp = (char*)pdp + remove_len;
				}
				
				put_block(pmip->dev, pmip->inode.i_block[dblk], buf);					//write data block to device
				return 1;
			}

			pdp = dp;																	//remember previous directory struct
			dp = (char*)dp + dp->rec_len;												//point to next directory struct within buffer
		}

		if(!strncmp(dp->name, base, dp->name_len) && dp->name_len == strlen(base))		//remove last directory in data block
		{
			pdp->rec_len = pdp->rec_len + dp->rec_len;

			put_block(pmip->dev, pmip->inode.i_block[dblk], buf);						//write data block to device
			return 1;
		}
	}

	return -1;
}
