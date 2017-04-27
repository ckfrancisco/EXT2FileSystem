//description: print permissions of a minode
//parameter: minode
//return:
int ls_permissions(MINODE *mip)
{
	printf( (S_ISDIR(mip->inode.i_mode)) ? "d" : (S_ISLNK(mip->inode.i_mode)) ? "l" : "-");
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
	ls_permissions(mip);						//print permission of minode
	printf(" %3d", mip->inode.i_links_count);	//print information of minode
	printf(" %3d", mip->inode.i_uid);
	printf(" %3d", mip->inode.i_gid);

	char time[64];
	strcpy(time, ctime(&mip->inode.i_mtime));	//create and display time string
	time[strlen(time) - 1] = 0;
	printf(" %13s", time);


	printf(" %7d", mip->inode.i_size);

	printf(" %s", name);							//use name to print name of minode

	if(S_ISLNK(mip->inode.i_mode))				//if minode is a link display contents
		printf(" -> %s\n", (char*)mip->inode.i_block);
	else
		printf("\n");
}

//description: show information about a single file or all the files within a directory
//parameter: path name
//return:
int local_ls(char *path)
{
	int ino;
	MINODE *mip;

	if(path[0])							//if path is not empty determine device via path
	{
		if(path[0] == '/')				//if path is absolute set device to root's device
			dev = root->dev;
		else
			dev = running->cwd->dev;	//else use cwd's device

		ino = getino(&dev, path);		//NOTE: when path is not empty we use iget() so we must end with iput()
		mip = iget(dev, ino);
	}

	else								//else use cwd's' device and minode
	{ 									//NOTE: used in cases when path is not declared
		dev = running->cwd->dev;
		mip = iget(dev, running->cwd->ino);
	}

	if(S_ISREG(mip->inode.i_mode))		//if minode is a single file ls the file
	{
		char *name = strrchr(path, '/');
		if(name)
			ls_file(mip, strrchr(path, '/') + 1);
		else
			ls_file(mip, path);
	}

	if(S_ISDIR(mip->inode.i_mode))		//if minode is a directory ls all files within
	{
		for(int dblk = 0; dblk < 12; dblk++)				//execute across all direct blocks within inode's inode table
		{
			if(!(mip->inode.i_block[dblk]))					//if empty block found break
				break;

			
			char buf[BLKSIZE];
			get_block(dev, mip->inode.i_block[dblk], buf);	//read a directory block from the inode table into buffer
			DIR *dp = (DIR*)buf;							//cast buffer as directory pointer

			printf("Data Block = %d\n", mip->inode.i_block[dblk]);

			while(dp < &buf[BLKSIZE])						//execute while there is another directory struct ahead
			{
				MINODE *dmip = iget(mip->dev, dp->inode);

				char name[MAXNAME];
				strncpy(name, dp->name, dp->name_len);		//copy directory name into string buffer
				name[dp->name_len] = 0;

				ls_file(dmip, name);						//ls minode

				dp = (char*)dp + dp->rec_len;				//point to next directory struct within buffer

				iput(dmip);
			}
		}
	}

	iput(mip);
}

//description: show directory struct information stored within a directory minode
//parameter: path
//return:
int local_stat(char *path)
{
	int ino;
	MINODE *mip;

	if(path[0])							//if path is not empty determine device via path
	{
		if(path[0] == '/')				//if path is absolute set device to root's device
			dev = root->dev;
		else
			dev = running->cwd->dev;	//else use cwd's device

		ino = getino(&dev, path);		//NOTE: when path is not empty we use iget() so we must end with iput()
		mip = iget(dev, ino);
	}

	else								//else use cwd's' device and minode
	{ 									//NOTE: used in cases when path is not declared
		dev = running->cwd->dev;
		mip = running->cwd;
	}

	if(S_ISREG(mip->inode.i_mode))
	{
		char directory[MAXPATH];
		det_dirname(path, directory);							//parse path directory
		int pino = getino(&dev, directory);						//determine parent inode
		MINODE *pmip = iget(dev, pino);							//get parent minode

		printf("%3s %7s %8s %s", "ino", "rec_len", "name_len", "name\n");

		for(int dblk = 0; dblk < 12; dblk++)					//execute across all direct blocks within inode's inode table
		{
			if(!(pmip->inode.i_block[dblk]))					//if empty block found break
				break;

			char buf[BLKSIZE];
			get_block(dev, pmip->inode.i_block[dblk], buf);		//read a directory block from the inode table into buffer
			DIR *dp = (DIR*)buf;								//cast buffer as directory pointer

			while((char*)dp < &buf[BLKSIZE])					//execute while there is another directory struct ahead
			{
				char name[MAXNAME];
				strncpy(name, dp->name, dp->name_len);			//copy directory name into string buffer
				name[dp->name_len] = 0;

				if(mip->ino == dp->inode)
				{
					printf("%3d %7d %8d %s\n", dp->inode, dp->rec_len, dp->name_len, name);
					break;
				}

				dp = (char*)dp + dp->rec_len;					//point to next directory struct within buffer
			}
		}

		iput(pmip);
	}

	if(S_ISDIR(mip->inode.i_mode))
	{
		printf("%3s %7s %8s %s", "ino", "rec_len", "name_len", "name\n");

		for(int dblk = 0; dblk < 12; dblk++)					//execute across all direct blocks within inode's inode table
		{
			if(!(mip->inode.i_block[dblk]))						//if empty block found break
				break;

			char buf[BLKSIZE];
			get_block(dev, mip->inode.i_block[dblk], buf);		//read a directory block from the inode table into buffer
			DIR *dp = (DIR*)buf;								//cast buffer as directory pointer

			while((char*)dp < &buf[BLKSIZE])					//execute while there is another directory struct ahead
			{
				char name[MAXNAME];
				strncpy(name, dp->name, dp->name_len);			//copy directory name into string buffer
				name[dp->name_len] = 0;

				printf("%3d %7d %8d %s\n", dp->inode, dp->rec_len, dp->name_len, name);

				dp = (char*)dp + dp->rec_len;					//point to next directory struct within buffer
			}
		}
	}

	if(path[0])													//if we used iget() before then put minode back
		iput(mip);
}

//description: change current working directory to path
//parameter: path
//return:
int local_chdir(char *path)
{
	if(!path[0])						//if path is null then display error and return fail
	{
		printf("ERROR: path name not specified\n");
		return -1;
	}

	int ino;

	if(path[0] == '/')					//initialize device depending on absolute or relative path
		dev = root->dev;
	else
		dev = running->cwd->dev;

	ino = getino(&dev, path);			//determine inode number of path
	if(ino < 0)							//if minode of path not found return fail
		return -1; 

	MINODE *mip = iget(dev, ino);		//get minode of inode number
	if(!S_ISDIR(mip->inode.i_mode))		//if the minode is not a directory then return fail
	{
		printf("ERROR: %s is not a directory\n", path);
		iput(mip);
		return -1;
	}

	iput(running->cwd);					//put previous cwd minode back
	running->cwd->dev = mip->dev;
	running->cwd = mip;					//assign minode to cwd

	local_pwd(mip);						//print new cwd path
	return 1;							//return success
}

//description: print path name to root recursively from minode
//parameter: minode
//return:
int rpwd(MINODE *mip)
{
	if(mip->ino == 2)					//if minode is the root then return
		return;

	MINODE *pmip = iget_parent(mip);	//determine parent of minode

	rpwd(pmip);							//print the name of the parent minode

	char name[MAXNAME];
	get_name(pmip, mip->ino, name);		//determine name of minode

	iput(pmip);							//put parent minode back

	printf("/%s", name);				//print the name of minode
}

//description: print root or path name to root recursively from minode
//parameter: minode
//return:
int local_pwd(MINODE *mip)
{
	printf("cwd = ");

	if(mip->ino == 2)	//if minode is the root then print '/' and return
	{
		printf("/\n");
		return;
	}

	rpwd(mip);			//else recursively print path to minode

	printf("\n");
}
