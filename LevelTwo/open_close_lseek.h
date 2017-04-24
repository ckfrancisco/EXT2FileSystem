//description: opens a file for a specified mode
//parameter: path and mode
//return: success or fail
int local_open(char* path, int mode)
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

	if(mip->inode.i_uid != running->uid)			//if minode uid does not match running uid display error and return fail
	{
		printf("ERROR: permission to %s denied\n", path);
		iput(mip);
		return -1;
	}

	for(int i = 0; i < NFD; i++)					//check if file is already open
	{
		if(running->fd[i] && running->fd[i]->mptr== mip)
		{
			if(running->fd[i]->mode == 0)			//if file is open for read then change mode
			{
				running->fd[i]->mode = mode;
				running->fd[i]->refCount++;			//increment reference count and return index
				return i;
			}
			else									//else display error and return fail
			{
				printf("ERROR: %s is already open in another mode\n", path);
				return -1;
			}
		}
	}

	int i = 0;
	for(i = 0; i < NFD; i++)						//determine available file descriptor index
		if(!running->fd[i])
			break;

	if(i == NFD)									//if no remaining file descriptors display error and return fail
	{
		printf("ERROR: no free file descriptors\n", path);
		iput(mip);
		return -1;
	}

	OFT *oftp = (OFT*)malloc(sizeof(OFT));			//allocate new open file descriptor struct

	oftp->mode = mode;								//assign values
	oftp->refCount = 1;
	oftp->mptr = mip;

	switch(mode)
	{
		case 0:										//read
			oftp->offset = 0;
			break;
		case 1:										//write
			truncate(mip);
			oftp->offset = 0;
			break;
		case 2:										//read write
			oftp->offset = 0;
			break;
		case 3:										//append
			oftp->offset = mip->inode.i_size;
			break;
		default:
			printf("ERROR: %d is not a supported mode\n", mode);
			return -1;
			break;
	}

	running->fd[i] = oftp;							//assign new file descriptor to index

	if(oftp->mode == 0)								//if file opened for read update time last accessed
		mip->inode.i_atime = time(0L);
	else											//else update time last modified as well
		mip->inode.i_atime = mip->inode.i_mtime = time(0L);

	mip->dirty = 1;

	return i;										//return index of new file descriptor
}

//description: close a file descriptor
//parameter: file descriptor index
//return: success or fail
int local_close(int i)
{
	if(i < 0 || i > NFD - 1)	//if file descriptor not within range display error and return fail
	{
		printf("ERROR: File descriptor is not within available range\n");
		return -1;
	}

	if(!running->fd[i])			//if file descriptor not open display error and return fail
	{
		printf("ERROR: File descriptor is not open\n");
		return -1;
	}

	OFT *oftp = running->fd[i];
	oftp->refCount--;			//decrement file descriptor reference count

	if(oftp->refCount > 0)		//if file descriptor is still in use return success
		return 1;

	running->fd[i] = 0;			//disable file descriptor
	iput(oftp->mptr);			//else put back minode and free file descriptor and return success
	free(oftp);

	return 1;
}

//description: change offset of a file descriptor
//parameter: file descriptor index and new offset
//return: success or fail
int local_lseek(int i, int pos)
{
	if(i < 0 || i > NFD - 1)					//if file descriptor not within range display error and return fail
	{
		printf("ERROR: File descriptor is not within available range\n");
		return -1;
	}

	if(!running->fd[i])							//if file descriptor not open display error and return fail
	{
		printf("ERROR: File descriptor is not open\n");
		return -1;
	}

	MINODE *mip = running->fd[i]->mptr;
	if(pos < 0 || pos > mip->inode.i_size - 1)	//if offset not within range display error and return fail
	{
		printf("ERROR: Position is not within available range\n");
		return -1;
	}

	running->fd[i]->offset = pos;				//update file descriptor offset and return success
	return 1;
}

//description: display current file descriptor values
//parameter: 
//return: 
int local_pfd()
{
	printf("%2s %6s %5s %6s [%3s, %3s]\n", "fd", "mode", "count", "offset", "dev", "ino");

	for(int i = 0; i < NFD; i++)
	{
		if(running->fd[i])
		{
			printf("%2d ", i);

			switch(running->fd[i]->mode)
			{
				case 0:										//read
					printf("%6s ", "READ");
					break;
				case 1:										//write
					printf("%6s ", "WRITE");
					break;
				case 2:										//read write
					printf("%6s ", "RDWR");
					break;
				case 3:										//append
					printf("%6s ", "APPEND");
					break;
			}

			printf("%5d %6d [%3d, %3d]\n", running->fd[i]->refCount, running->fd[i]->offset, running->fd[i]->mptr->dev, running->fd[i]->mptr->ino);
		}
	}
}