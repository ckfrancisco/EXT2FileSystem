//description:
//parameter:
//return:
int open_file(char* path, int mode)
{
	if(path[0] == '/')								//initialize device depending on absolute or relative path
		dev = root->dev;
	else
		dev = running->cwd->dev;

	int ino = getino(&dev, path);					//determine inode of link
	if(ino < 0)										//if link not found display error and return fail
	{
		return -1;
	}

	MINODE *mip = iget(dev, ino);					//get minode of link
	if(!S_ISREG(mip->inode.i_mode))					//if minode is not a file or link display error and return fail
	{
		printf("ERROR: %s is not a file\n", path);
		iput(mip);
		return -1;
	}

	if(mip->inode.i_uid != running->uid)
	{
		printf("ERROR: permission to file denied\n", path);
		iput(mip);
		return -1;
	}

	for(int i = 0; i < NFD; i++)
	{
		if(running->fd[i]->mptr == mip)
		{
			if(running->fd[i]->mode == 0)
			{
				running->fd[i]->mode = mode;
				return 1;
			}
			else
				return -1;
		}
	}

	int i = 0;
	for(i = 0; i < NFD; i++)
		if(!running->fd[i])
			break;

	if(i == NFD)
	{
		printf("ERROR: no free file descriptors\n", path);
		iput(mip);
		return -1;
	}

	OFT *oftp = (OFT*)malloc(sizeof(OFT));

	oftp->mode = mode;
	oftp->refCount = 1;
	oftp->mptr = mip;

	switch(mode)
	{
		case 0:					//read
			oftp->offset = 0;
			break;
		case 1:					//write
			truncate(mip);
			oftp->offset = 0;
			break;
		case 2:					//read write
			oftp->offset = 0;
			break;
		case 3:					//append
			oftp->offset = mip->inode.i_size;
			break;
		default:
			break;
	}

	running->fd[i] = oftp;

	if(oftp->mode == 0)
		mip->inode.i_atime = time(0L);
	else
		mip->inode.i_atime = mip->inode.i_mtime = time(0L);

	return i;
}

int close_file(int i)
{
	if(i < 0 || i > NFD - 1)
	{
		printf("ERROR: File descriptor is not within available range\n");
		return -1;
	}

	if(!running->fd[i])
	{
		printf("ERROR: File descriptor is not open\n");
		return -	1;
	}

	OFT *oftp = running->fd[i];
	running->fd[i] = 0;
	oftp->refCount--;

	if(oftp->refCount > 0)
		return 1;

	iput(oftp->mptr);
	free(oftp);

	return 1;
}

int l_seek(int i, int pos)
{
	if(i < 0 || i > NFD - 1)
	{
		printf("ERROR: File descriptor is not within available range\n");
		return -1;
	}

	if(!running->fd[i])
	{
		printf("ERROR: File descriptor is not open\n");
		return -	1;
	}

	MINODE *mip = running->fd[i]->mptr;
	if(pos < 0 || pos > mip->inode.i_size - 1)
	{
		printf("ERROR: Position is not within available range\n");
		return -	1;
	}



}