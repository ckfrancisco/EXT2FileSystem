//description: read specified number of bytes from file descriptor into buffer
//parameter: file descriptor index, buffer, and number of bytes to read
//return: number of bytes read
int read_file(int fd, char buf[], int nbytes)
{
	if(fd < 0 || fd > NFD - 1)													//if file descriptor not within range display error and return 0
	{
		printf("ERROR: File descriptor is not within available range\n");
		return 0;
	}

	if(!running->fd[fd])														//if file descriptor is not open then display error and return 0
	{
		printf("ERROR: file descriptor is not open\n");
		return 0;
	}

	if(running->fd[fd]->mode != 0 && 											//if file descriptor is not open then display error and return 0
		running->fd[fd]->mode != 2)
	{
		printf("ERROR: file descriptor is not open for read\n");
		return 0;
	}

	int lblk = running->fd[fd]->offset / BLKSIZE;								//logical block number determined by offset and block size
	int start = running->fd[fd]->offset % BLKSIZE;								//start position within logic block
	int remain = BLKSIZE - start;												//remaining bytes available within block
	int avail = running->fd[fd]->mptr->inode.i_size - running->fd[fd]->offset;	//remaining bytes in file

	int count = 0;

	while(nbytes && avail)														//execute loop until nbytes read or end of file
	{
		char readbuf[BLKSIZE];
		
		if(remain > avail)														//if the amount of bytes remaining in the block
			remain = avail;														// is greater then available bytes then set to avail

		if(lblk < 12)															//read direct block
		{
			int dblk = running->fd[fd]->mptr->inode.i_block[lblk];				//determine data block
			get_block(running->fd[fd]->mptr->dev, dblk, readbuf);				//read data block in read buffer
		}

		else if(lblk >= 12 && lblk < 256 + 12)									//read indirect block
		{
			int i = running->fd[fd]->mptr->inode.i_block[12];					//determine indirect block number

			char ibuf[BLKSIZE];
			get_block(running->fd[fd]->mptr->dev, i, ibuf);						//read indirect block into indirect buffer
			int dblk = ((int*)ibuf)[lblk - 12];									//determine data block

			get_block(running->fd[fd]->mptr->dev, dblk, readbuf);				//read data block in read buffer
		}

		else if(lblk >= 256 + 12 && lblk < (256 + 12) + (256 * 256))			//read double indirect block
		{
			int di = running->fd[fd]->mptr->inode.i_block[13];

			char dibuf[BLKSIZE];
			get_block(running->fd[fd]->mptr->dev, di, dibuf);					//read double indirect block into double indirect buffer
			int i = ((int*)dibuf)[(lblk - 268) / 256];							//determine indirect block

			char ibuf[BLKSIZE];
			get_block(running->fd[fd]->mptr->dev, i, ibuf);						//read indirect block into indirect buffer
			int dblk = ((int*)ibuf)[(lblk - 268) % 256];						//determine data block

			get_block(running->fd[fd]->mptr->dev, dblk, readbuf);				//read data block in read buffer
		}

		if(nbytes <= remain)													//if number of bytes to be read is less than remain
		{
			strncpy(&buf[count], &readbuf[start], nbytes);						//copy specified number of  bytes
			count += nbytes;													//increment count by number of bytes read
			nbytes = 0;
			break;
		}

		else																	//else copy remaining number of bytes in block
		{
			strncpy(&buf[count], &readbuf[start], remain);

			lblk++;																//update trackers
			start = 0;
			avail -= remain;													//must update available bytes and count before
			count += remain;													//	updating remaining bytes
			nbytes -= remain;
			remain = BLKSIZE;
		}
	}

	running->fd[fd]->offset += count;											//increment offset by number of bytes read
	//printf("READ: read %d char from file descriptor %d\n", count, fd);
	return count;
}

//description: display file opened in file descriptor to screen
//parameter: file descriptor index and number of bytes to read
//return: number of bytes read or fail
int local_read(int fd, int nbytes)
{
	char buf[BLKSIZE + 1];
	int n;
	int count = 0;

	for(int i = 0; i < 66; i++)				//display border
			printf("*");
	printf("\n");

	while(n = (nbytes < 1024) ? 			//execute while number of bytes read is above 0
		read_file(fd, buf, nbytes) :
		read_file(fd, buf, 1024))
	{
		buf[n] = 0;							//null terminate buffer
		printf("%s", buf);
		nbytes -= n;						//decrement number of bytes to be read by n
		count += n;							//increment number of bytes read by n
	}

	printf("\n");
	for(int i = 0; i < 66; i++)				//display border
			printf("*");
	printf("\n");

	printf("READ: read %d char from file descriptor %d\n", count, fd);

	return 1;
}

//description: display file to screen
//parameter: path
//return: success or fail
int local_cat(char *path)
{
	if(!path[0])							//if path is null then display error and return fail
	{
		printf("ERROR: path name not specified\n");
		return -1;
	}

	char buf[BLKSIZE];
	int n;
	int count = 0;
	int offset = 0;

	int fd;

	if(path[0] == '/')						//initialize device depending on absolute or relative path
		dev = root->dev;
	else
		dev = running->cwd->dev;

	int ino = getino(&dev, path);			//determine inode number of path
	if(ino < 0)								//if inode number not found return fail
		return -1;

	MINODE *mip = iget(dev, ino);			//create minode
	
	for(fd = 0; fd < NFD; fd++)				//check if file is already open
		if(running->fd[fd] && mip == running->fd[fd]->mptr)
			break;

	iput(mip);

	if(fd < NFD)							//if file is open simply increment reference count
		running->fd[fd]->refCount++;

	else									//else open the file for read
	{
		fd = local_open(path, 0);			//open file descriptor to file
		if(fd < 0)							//if open failed then return fail
			return -1;
	}

	

	if(running->fd[fd]->refCount > 1)		//if file is already open remember offset and set to 0
	{
		offset = running->fd[fd]->offset;
		running->fd[fd]->offset = 0;
	}

	for(int i = 0; i < 66; i++)				//display border
			printf("*");
	printf("\n");

	while(n = read_file(fd, buf, 1024))		//execute while number of bytes read is above 0
	{
		buf[n] = 0;							//null terminate buffer
		printf("%s", buf);
		count += n;							//increment number of bytes read by n
	}

	printf("\n");
	for(int i = 0; i < 66; i++)				//display border
			printf("*");
	printf("\n");

	printf("CAT: read %d char from %s\n", count, path);

	if(running->fd[fd]->refCount > 1)		//if file was already open then set offset back
		running->fd[fd]->offset = offset;

	local_close(fd);						//close file descriptor

	return 1;
}