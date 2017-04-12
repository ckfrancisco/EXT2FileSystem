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

	ino = getino(dev, path);					//determine inode number of path

	if(ino < 0)									//if inode number not found return fail
		return -1;

	mip = iget(dev, ino);						//create minode

	if(!S_ISDIR(mip->inode.i_mode))				//if minode is not a directory display error and return fail
	{
		printf("ERROR: %s is not a directory\n", directory);
		iput(mip);
		return -1;
	}

	if(mip->refCount > 0)						//if minode is still being referenced display error and return fail
	{
		printf("ERROR: %s is still being used\n", directory);
		iput(mip);
		return -1;
	}

	if(mip->inode.i_links_count > 2)			//if minode contains data blocks display error and return fail
	{
		printf("ERROR: %s contains items\n", directory);
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

	return 1;
}

//description: remove child from parent directory
//parameter: parent minode path
//return: success or fail
int rm_child(MINODE *pmip, char *path)
{
	int dblk;
	DIR *dp;
	char *cp;

	for(dblk = 0; dblk < 12; dblk++)							//execute across all direct blocks within inode's inode table
	{
		if(!(mip->inode.i_block[dblk]))
			return -1;

		get_block(mip->dev, mip->inode.i_block[dblk], buf);		//read a directory block from the inode table into buffer
		dp = (DIR*)buf;											//cast buffer as directory pointer
		cp = buf;												//cast buffer as "byte" pointer



		while(cp < &buf[BLKSIZE])								//execute while there is another directory struct ahead
		{
			cp += dp->rec_len;									//set variables to the next directory struct
			dp = (DIR*)cp;
		}
	}
}
