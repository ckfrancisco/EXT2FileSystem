int rm_dir(char *path)
{
	//path name tokenized by directory and base in main()
	int ino;
	MINODE *mip;
	char name[MAXNAME];
	MINODE *pmip;
	int i;

	if(path[0] && path[0] == '/')				//if path is not empty determine device via path
		dev = root->dev;				//if path is absolute set device to root's device

	else							//else use running for device and minode
		dev = running->cwd->dev;

	ino = getino(dev, directory);

	if(ino < 0)
		return -1;

	mip = iget(dev, ino);

	if(!S_ISDIR(mip->inode.i_mode))
	{
		printf("ERROR: %s is not a directory\n", directory);
		iput(mip);
		return -1;
	}

	if(mip->refCount == 0)
	{
		printf("ERROR: %s is still being used\n", directory);
		iput(mip);
		return -1;
	}

	if(mip->inode.i_links_count > 2)
	{
		printf("ERROR: %s contains items\n", directory);
		iput(mip);
		return -1;
	}

	for (i = 0; i < 12; i++)
	{
		if (mip->inode.i_block[i] == 0)
			continue;

         	bdealloc(mip->dev, mip->inode.i_block[i]);
 	}
	idealloc(mip->dev, mip->ino);

	pmip = iget_parent(mip);
	iput(mip);

	rm_child(pmip, base);
	pmip->inode.i_links_count--;
	pmip->inode.i_atime = pmip->inode.i_mtime = time(0L);

	return 1;
}

//
//
//
int rm_child(MINODE *pmip, char *path)
{
	DIR *dp;
	char *cp;

	for(dblk = 0; dblk < 12; dblk++)					//execute across all direct blocks within inode's inode table
	{
		if(!(mip->inode.i_block[dblk]))
			return -1;

		get_block(mip->dev, mip->inode.i_block[dblk], buf);		//read a directory block from the inode table into buffer
		dp = (DIR*)buf;				/			//cast buffer as directory pointer
		cp = buf;							//record byte location of directory pointer



		while(cp < &buf[BLKSIZE])					//execute while there is another directory struct ahead
		{
			if(!strncmp(dp->name, name, dp->name_len)		//check if directory name matches name
				&& strlen(name) == dp->name_len)		//prevents . == ..
				return dp->inode;				//return inode number if true

			cp += dp->rec_len;					//set variables to the next directory struct
			dp = (DIR*)cp;
		}
	}
}
