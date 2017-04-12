//description: print permissions of a minode
//parameter: minode
//return:
int ls_permissions(MINODE *mip)
{
	printf( (S_ISDIR(mip->inode.i_mode)) ? "d" : "-");
	printf( (mip->inode.i_mode & S_IRUSR) ? "r" : "-");
	printf( (mip->inode.i_mode & S_IWUSR) ? "w" : "-");
	printf( (mip->inode.i_mode & S_IXUSR) ? "x" : "-");
	printf( (mip->inode.i_mode & S_IRGRP) ? "r" : "-");
	printf( (mip->inode.i_mode & S_IWGRP) ? "w" : "-");
	printf( (mip->inode.i_mode & S_IXGRP) ? "x" : "-");
	printf( (mip->inode.i_mode & S_IROTH) ? "r" : "-");
	printf( (mip->inode.i_mode & S_IWOTH) ? "w" : "-");
	printf( (mip->inode.i_mode & S_IXOTH) ? "x" : "-");
}

//description: show information about a single file
//parameter: minode and name of minode
//return:
int ls_file(MINODE *mip, char *name)
{
	char time[64];

	ls_permissions(mip);				//print permission of minode
	printf(" %3d", mip->inode.i_links_count);	//print information of minode
	printf(" %3d", mip->inode.i_uid);
	printf(" %3d", mip->inode.i_gid);

	strcpy(time, ctime(&mip->inode.i_ctime));
	time[strlen(time)-1] = 0;
	printf(" %13s",time);


	printf(" %7d", mip->inode.i_size);

	printf(" %s\n", name);				//use name to print name of minode
}

//description: show information about a single file or all the files within a directory
//parameter: path
//return:
int ls(char *path)
{
	int ino;
	MINODE *mip;
	MINODE *dmip;
	int dblk;
	char buf[BLKSIZE];
	DIR *dp;
	char *cp;
	char name[MAXNAME];

	if(path[0])					//if path is not empty determine device via path
	{
		if(path[0] == '/')			//if path is absolute set device to root's device
			dev = root->dev;
		else
			dev = running->cwd->dev;	//else use cwd's device

		ino = getino(&dev, path);
		mip = iget(dev, ino);
	}

	else						//else use running for device and minode
	{
		dev = running->cwd->dev;
		mip = running->cwd;
	}

	if(S_ISREG(mip->inode.i_mode))			//if minode is a single file ls the file
	{
		ls_file(mip, strrchr(path, '/') + 1);
	}

	if(S_ISDIR(mip->inode.i_mode))			//if minode is a directory ls all files within
	{
		for(dblk = 0; dblk < 12; dblk++)				//execute across all direct blocks within inode's inode table
		{
			if(!(mip->inode.i_block[dblk]))				//return fail if empty block found
			{
				iput(mip);
				return -1;
			}

			get_block(dev, mip->inode.i_block[dblk], buf);		//read a directory block from the inode table into buffer
			dp = (DIR*)buf;						//cast buffer as directory pointer
			cp = buf;						//cast buffer as "byte" pointer

			while(cp < &buf[BLKSIZE])				//execute while there is another directory struct ahead
			{
				dmip = iget(mip->dev, dp->inode);

				strncpy(name, dp->name, dp->name_len);
				name[dp->name_len] = 0;

				ls_file(dmip, name);				//ls minode

				cp += dp->rec_len;				//set variables to the next directory struct
				dp = (DIR*)cp;

				iput(dmip);
			}
		}
	}
	iput(mip);
}

//description: show directory struct information stored within a directory minode
//parameter: path
//return:
int lsdir(char *path)
{
	int ino;
	MINODE *mip;
	int dblk;
	char buf[BLKSIZE];
	DIR *dp;
	char *cp;
	char name[MAXNAME];

	if(path[0])					//if path is not empty determine device via path
	{
		if(path[0] == '/')			//if path is absolute set device to root's device
			dev = root->dev;
		else
			dev = running->cwd->dev;

		ino = getino(&dev, path);
		mip = iget(dev, ino);
	}

	else						//else use running for device and minode
	{
		dev = running->cwd->dev;
		mip = running->cwd;
	}

	if(S_ISDIR(mip->inode.i_mode))
	{
		printf("%3s %7s %8s %s", "ino", "rec_len", "name_len", "name\n");

		for(dblk = 0; dblk < 12; dblk++)				//execute across all direct blocks within inode's inode table
		{
			if(!(mip->inode.i_block[dblk]))				//return fail if empty block found
			{
				iput(mip);
				return -1;
			}

			get_block(dev, mip->inode.i_block[dblk], buf);		//read a directory block from the inode table into buffer
			dp = (DIR*)buf;						//cast buffer as directory pointer
			cp = buf;						//cast buffer as "byte" pointer

			while(cp < &buf[BLKSIZE])				//execute while there is another directory struct ahead
			{
				strncpy(name, dp->name, dp->name_len);
				name[dp->name_len] = 0;

				printf("%3d %7d %8d %s\n", dp->inode, dp->rec_len, dp->name_len, name);

				cp += dp->rec_len;				//set variables to the next directory struct
				dp = (DIR*)cp;
			}
		}
	}
	iput(mip);
}

//description: change current working directory to path
//parameter: path
//return:
int chdir(char *path)
{
	int ino;
	MINODE *mip;
	char name[64];

	if(path[0] == '/')			//initialize device depending on absolute or relative path
		dev = root->dev;
	else
		dev = running->cwd->dev;

	ino = getino(&dev, path);		//determine inode number of path

	if(ino < 0)				//if minode of path not found return fail
		return -1;

	mip = iget(dev, ino);			//get minode of inode number

	if(!S_ISDIR(mip->inode.i_mode))		//if the minode is not a directory then return
	{
		printf("ERROR: %s is not a directory\n", path);
		return -1;
	}

	running->cwd->dev = mip->dev;
	running->cwd = mip;			//assign minode to cwd

	pwd(mip);
}

//description: print path name to root recursively from minode
//parameter: minode
//return:
int rpwd(MINODE *mip)
{
	MINODE *pmip;
	char name[MAXNAME];

	if(mip->ino == 2)		//if minode is the root then return
		return;

	pmip = iget_parent(mip);	//determine parent of minode

	rpwd(pmip);			//print the name of the parent minode

	get_name(pmip, mip, name);	//determine name of minode

	printf("/%s", name);		//print the name of minode
}

//description: print root or path name to root recursively from minode
//parameter: minode
//return:
int pwd(MINODE *mip)
{
	char name[64];

	printf("cwd = ");

	if(mip->ino == 2)	//if minode is the root then print '/' and return
	{
		printf("/\n");
		return;
	}

	rpwd(mip);		//else recursively print path to minode

	printf("\n");
}
