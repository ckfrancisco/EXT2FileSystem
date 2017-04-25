//description: write specified number of bytes to file descriptor from buffer
//parameter: file descriptor index, buffer, and number of bytes to write
//return: number of bytes written
int write_file(int fd, char buf[], int nbytes)
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

	if(running->fd[fd]->mode != 1 && 											//if file descriptor is not open then display error and return 0
		running->fd[fd]->mode != 2 &&
		running->fd[fd]->mode != 3)
	{
		printf("ERROR: file descriptor is not open for write\n");
		return 0;
	}

	int lblk = running->fd[fd]->offset / BLKSIZE;								//logical block number determined by offset and block size
	int start = running->fd[fd]->offset % BLKSIZE;								//start position within logic block
	int remain = BLKSIZE - start;												//remaining bytes available within block

	int count = 0;

	while(nbytes)																//execute loop until nbytes written
	{
		char writebuf[BLKSIZE];
		int dblk = 0;

		if(lblk < 12)															//read direct block
		{
			dblk = running->fd[fd]->mptr->inode.i_block[lblk];					//determine data block

			if(dblk == 0)														//if block is empty then allocate and assign block number
			{
				dblk = running->fd[fd]->mptr->inode.i_block[lblk] = balloc(running->fd[fd]->mptr->dev);
			}

			get_block(running->fd[fd]->mptr->dev, dblk, writebuf);				//read data block in read buffer
		}

		else if(lblk >= 12 && lblk < 256 + 12)									//read indirect block
		{
			int i = running->fd[fd]->mptr->inode.i_block[12];					//determine indirect block number

			if(i == 0)															//if indirect block is empty then allocate and assign block number
			{
				i = running->fd[fd]->mptr->inode.i_block[12] = balloc(running->fd[fd]->mptr->dev);
			}

			char ibuf[BLKSIZE];
			get_block(running->fd[fd]->mptr->dev, i, ibuf);						//read indirect block into indirect buffer
			dblk = ((int*)ibuf)[lblk - 12];										//determine data block

			if(dblk == 0)														//if block is empty then allocate and assign block number
			{
				dblk = ((int*)ibuf)[lblk - 12] = balloc(running->fd[fd]->mptr->dev);
			}

			get_block(running->fd[fd]->mptr->dev, dblk, writebuf);				//read data block in read buffer
		}

		else if(lblk >= 256 + 12 && lblk < (256 + 12) + (256 * 256))			//read double indirect block
		{
			int di = running->fd[fd]->mptr->inode.i_block[13];

			if(di == 0)															//if double indirect block is empty then allocate and assign block number
			{
				di = running->fd[fd]->mptr->inode.i_block[13] = balloc(running->fd[fd]->mptr->dev);
			}

			char dibuf[BLKSIZE];
			get_block(running->fd[fd]->mptr->dev, di, dibuf);					//read double indirect block into double indirect buffer
			int i = ((int*)dibuf)[(lblk - 268) / 256];							//determine indirect block

			if(i == 0)															//if indirect block is empty then allocate and assign block number
			{
				i = ((int*)dibuf)[(lblk - 268) / 256] = balloc(running->fd[fd]->mptr->dev);
			}

			char ibuf[BLKSIZE];
			get_block(running->fd[fd]->mptr->dev, i, ibuf);						//read indirect block into indirect buffer
			dblk = ((int*)ibuf)[(lblk - 268) % 256];							//determine data block

			if(dblk == 0)														//if block is empty then allocate and assign block number
			{
				dblk = ((int*)ibuf)[(lblk - 268) % 256] = balloc(running->fd[fd]->mptr->dev);
			}

			get_block(running->fd[fd]->mptr->dev, dblk, writebuf);				//read data block in read buffer
		}

		if(nbytes <= remain)													//if number of bytes to be read is less than remain
		{
			strncpy(&writebuf[start], &buf[count], nbytes);						//copy specified number of bytes
			count += nbytes;													//increment count by number of bytes read
			nbytes = 0;

			put_block(running->fd[fd]->mptr->dev, dblk, writebuf);				//write data block back into memory
			break;
		}

		else																	//else copy remaining number of bytes in block
		{
			strncpy(&writebuf[start], &buf[count], remain);

			lblk++;																//update trackers
			start = 0;
			count += remain;													//must update available bytes and count before
			nbytes -= remain;													//	updating remaining bytes
			remain = BLKSIZE;

			put_block(running->fd[fd]->mptr->dev, dblk, writebuf);				//write data block back into memory
		}
	}

	running->fd[fd]->offset += count;											//increment offset by number of bytes read
	if (running->fd[fd]->mptr->inode.i_size < running->fd[fd]->offset)			//if size is less than offset than update
		running->fd[fd]->mptr->inode.i_size = running->fd[fd]->offset;
	running->fd[fd]->mptr->dirty = 1;
	//printf("READ: read %d char from file descriptor %d\n", count, fd);
	return count;
}

//description: write to file opened in file descriptor
//parameter: file descriptor index and number of bytes to read
//return: number of bytes read or fail
int local_write(int fd, char buf[])
{
	int count = 0;

	int nbytes = strlen(buf);

	count = write_file(fd, buf, nbytes);	//execute while number of bytes read is above 0

	printf("WRITE: read %d char from file descriptor %d\n", count, fd);

	return 1;
}

//description: copy file from source to destination
//parameter: source and destination path
//return: success or fail
int local_cp(char *spath, char *dpath)
{
	int fd = local_open(spath, 0);					//open file descriptor for source in read mode
	if(fd < 0)										//if opening source fails return fail
		return -1;

	int gd = local_open(dpath, 1);					//open file descriptor for source in read mode
	if(gd < 0)										//if opening destination fails return fail
	{												// must decide if file simply doesn't exist

		if(dpath[0] == '/')							//initialize device depending on absolute or relative path
			dev = root->dev;
		else
			dev = running->cwd->dev;

		int dino = getino(dev, dpath);
		if(dino >= 0)								//if destination exists then opening didn't fail because
		{											//	destination didn't exist so close fd and return fail
			close(fd);
			return -1;
		}

		printf("FIX: attempting to create destination file\n");

		dino = local_creat(dpath);
		if(dino < 0)								//if creating destination failed then close fd and return fail
		{											//	destination didn't exist
			close(fd);
			return -1;
		}

		gd = local_open(dpath, 1);					//open file descriptor for new destination file
	}

	char buf[BLKSIZE];
	int n;
	while(n = read_file(fd, buf, BLKSIZE))			//continue to read and write until end of source file
       write_file(gd, buf, n);

	local_close(fd);								//close both file descriptors
	local_close(gd);
	return 1;
}

//description: move file from source to destination
//parameter: source and destination path
//return: success or fail
int local_mv(char *spath, char *dpath)
{
	int sdev;
	int ddev;

	if(spath[0] == '/')							//determine device of source
		sdev = root->dev;
	else
		sdev = running->cwd->dev;

	if(dpath[0] == '/')							//determine device of source
		ddev = root->dev;
	else
		ddev = running->cwd->dev;

	if(sdev == ddev)
	{
		if(local_link(spath, dpath) < 0)
			return -1;
		local_unlink(spath);
	}

	else
	{
		if(local_cp(spath, dpath) < 0)
			return -1;
		local_unlink(spath);
	}
}