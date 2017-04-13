//description: remove directory determined by path
//parameter: path
//return: success or fail
int rm_dir(char *path)
{
	int ino;
	MINODE *mip;
	char name[MAXNAME];
	MINODE *pmip;
	int i;

	if(path[0] && path[0] == '/')				//if path is not empty determine device via path
		dev = root->dev;						//if path is absolute set device to root's device

	else										//else use running for device and minode
		dev = running->cwd->dev;

	ino = getino(&dev, path);					//determine inode number of path

	if(ino < 0)									//if inode number not found return fail
		return -1;

	mip = iget(dev, ino);						//create minode

	if(!S_ISDIR(mip->inode.i_mode))				//if minode is not a directory display error and return fail
	{
		printf("ERROR: %s is not a directory\n", base);
		iput(mip);
		return -1;
	}

	if(mip->refCount > 1)						//if minode is still being referenced display error and return fail
	{
		printf("ERROR: %s is still being used\n", base);
		iput(mip);
		return -1;
	}

	if(mip->inode.i_links_count > 2)			//if minode contains data blocks display error and return fail
	{
		printf("ERROR: %s contains items\n", base);
		iput(mip);
		return -1;
	}

	for (i = 0; i < 12; i++)					//deallocate all block numbers
	{
		if (mip->inode.i_block[i] == 0)
			continue;
		bdealloc(mip->dev, mip->inode.i_block[i]);
	}

	idealloc(mip->dev, mip->ino);				//deallocate inode number

	pmip = iget_parent(mip);					//determine parent minode
	iput(mip);

	rm_child(pmip, base);
	pmip->inode.i_links_count--;
	pmip->inode.i_atime = pmip->inode.i_mtime = time(0L);
	iput(pmip);

	return 1;
}

//description: remove child from parent directory
//parameter: parent minode and path
//return: success or fail
int rm_child(MINODE *pmip, char *base)
{
	int dblk;
	DIR *pdp;
	DIR *dp;
	char *cp;

	for(dblk = 0; dblk < 12; dblk++)													//execute across all direct blocks within inode's inode table
	{
		if(!pmip->inode.i_block[dblk])													//break if empty block found
			return -1;

		get_block(pmip->dev, pmip->inode.i_block[dblk], buf);							//read a directory block from the inode table into buffer
		pdp = (DIR*)buf;																//cast buffer as previous directory pointer
		dp = (DIR*)buf;																	//cast buffer as directory pointer
		cp = buf;																		//cast buffer as "byte" pointer

		if(!strncmp(dp->name, base, dp->rec_len)
			&& dp->name_len == strlen(base)
			&& cp + dp->rec_len <= &buf[BLKSIZE])										//remove first and only directory in data block
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

		while(cp + dp->rec_len < &buf[BLKSIZE])											//execute until last directory encountered
		{
			if(!strncmp(dp->name, base, dp->rec_len) && dp->name_len == strlen(base))	//remove middle directory in data block
			{
				put_block(pmip->dev, pmip->inode.i_block[dblk], buf);					//write data block to device
				return 1;
			}

			cp += dp->rec_len;															//set variables to the next directory struct
			pdp = dp;																	//remember previous directory struct
			dp = (DIR*)cp;
		}

		if(!strncmp(dp->name, base, dp->name_len) && dp->name_len == strlen(base))		//remove last directory in data block
		{
			pdp->rec_len = pdp->rec_len + dp->rec_len;

			put_block(pmip->dev, pmip->inode.i_block[dblk], buf);						//write data block to device
			return 1;
		}
	}
}
