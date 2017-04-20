//description: create a soft link to an old file
//parameter: path to old file and new file
//return: success or fail
int link(char *oldpath, char *newpath)
{
	int oino = getino(&dev, oldpath);									//determine inode of old file
	if(oino < 0)														//if file not found display error and return fail
	{
		return -1;
	}

	MINODE *omip = iget(dev, oino);										//get minode of old file
	if(!S_ISREG(omip->inode.i_mode) && !S_ISLNK(omip->inode.i_mode))	//if minode is not a file or link display error and return fail
	{
		printf("ERROR: %s is not a file or link\n", oldpath);
		iput(omip);
		return -1;
	}

	char linkdirectory[MAXPATH];										//parse new path
	char linkbase[MAXPATH];
	det_dirname(newpath, linkdirectory);
	det_basename(newpath, linkbase);

	int pino = getino(&dev, linkdirectory);								//determine parent inode of new file
	if(pino < 0)														//if parent inode not found display error and return fail
	{
		iput(omip);
		return -1;
	}

	MINODE *pmip = iget(dev, pino);										//get parent minode of new file
	if(!S_ISDIR(pmip->inode.i_mode))									//if parent minode is not a directory display error and return fail
	{
		printf("ERROR: %s is not a directory\n", linkdirectory);
		iput(omip);
		iput(pmip);
		return -1;
	}

	if(search(pmip, linkbase) > 0)										//if name of new file already exists display error and return fail
	{
		printf("ERROR: %s already exists\n", linkbase);
		iput(omip);
		iput(pmip);
		return -1;
	}

	int result =  enter_name(pmip, oino, linkbase);						//enter name of new file into parent directory

	if(result > 0)														//if name entry is successful increment link count
		omip->inode.i_links_count++;

	iput(omip);															//put back minode
	iput(pmip);															//put back parent minode

	return result;														//return result of name entry
}

//description: unlink a file or link
//parameter: path to file or link
//return: success or fail
int unlink(char *path)
{
	int ino = getino(&dev, path);									//determine inode of link
	if(ino < 0)														//if link not found display error and return fail
	{
		return -1;
	}

	MINODE *mip = iget(dev, ino);									//get minode of link
	if(!S_ISREG(mip->inode.i_mode) && !S_ISLNK(mip->inode.i_mode))	//if minode is not a file or link display error and return fail
	{
		printf("ERROR: %s is not a file or link\n", path);
		iput(mip);
		return -1;
	}

	mip->inode.i_links_count--;										//decrement minode link count

	if(mip->inode.i_links_count == 0)								//if link count is 0 truncate all data blocks
	{	
		truncate(mip);												//write 0s across data blocks and deallocate them
		idealloc(mip->dev, mip->ino);								//deallocate inode number
	}

	int pino = getino(&dev, directory);								//determine parent inode of link
	MINODE *pmip = iget(dev, pino);									//determine parent minode of link

	int result =  rm_child(pmip, base);								//remove link from parent directory

	iput(mip);														//put back minode
	iput(pmip);														//put back parent minode

	return result;
}

//description: create a hard link to an old file
//parameter: path to old file and new file
//return: success or fail
int symlink(char *oldpath, char *newpath)
{
	int oino = getino(&dev, oldpath);									//determine inode of old file
	if(oino < 0)														//if old file not found display error and return fail
	{
		return -1;
	}

	MINODE *omip = iget(dev, oino);										//get minode of old file

	char linkdirectory[MAXPATH];										//parse new path
	char linkbase[MAXPATH];
	det_dirname(newpath, linkdirectory);
	det_basename(newpath, linkbase);

	int pino = getino(&dev, linkdirectory);								//determine parent inode of new file
	if(pino < 0)														//if parent inode not found display error and return fail
	{
		iput(omip);
		return -1;
	}

	MINODE *pmip = iget(dev, pino);										//get parent minode of link
	if(!S_ISDIR(pmip->inode.i_mode))									//if parent minode is not a directory display error and return fail
	{
		printf("ERROR: %s is not a directory\n", linkdirectory);
		iput(omip);
		iput(pmip);
		return -1;
	}

	if(search(pmip, linkbase) > 0)										//if name of new file already exists display error and return fail
	{
		printf("ERROR: %s already exists\n", linkbase);
		iput(omip);
		iput(pmip);
		return -1;
	}

	int ino = creat_file(newpath);										//create new file and record inode of hard link
	if(ino < 0)															//if file not created error was displayed in previous funtion so return fail
	{
		iput(omip);
		iput(pmip);
		return -1;
	}

	MINODE *mip = iget(dev, ino);										//determine minode of hard link
	mip->inode.i_mode = 0xA1FF;											//adjust from file to hard link

	strcpy((char*)mip->inode.i_block, oldpath);							//copy old path to block
	mip->inode.i_size = strlen(oldpath);								//adjust size of hard link to length of old path

	mip->dirty = 1;
	
	iput(mip);
	iput(pmip);

	return 1;															//return success
}

//description: read contents of symlink
//parameter: soft link path
//return: success or fail
int readlink(char *path, char *contents)
{
	int ino = getino(&dev, path);							//determine inode of link
	if(ino < 0)												//if link not found display error and return fail
	{
		return -1;
	}

	MINODE *mip = iget(dev, ino);							//get minode of link
	if(!S_ISLNK(mip->inode.i_mode))							//if minode is not a link display error and return fail
	{
		printf("ERROR: %s is not a link\n", path);
		iput(mip);
		return -1;
	}

	strcpy(contents, (char*)mip->inode.i_block);			//copy contents of soft link and return success
	printf("%s\n", contents);
	return 1;
}

//description: clear all of an minode's data blocks
//parameter: minode
//return: 
int truncate(MINODE *mip)
{
	char nbuf[BLKSIZE];
	memset(nbuf, 0, BLKSIZE);

	for(int dblk = 0; dblk < 14; dblk++)							//reset and deallocate all used data blocks
	{
		if(dblk < 12)												//direct data blocks
		{
			bdealloc(mip->dev, mip->inode.i_block[dblk]);
			put_block(mip->dev, mip->inode.i_block[dblk], nbuf);
		}

		else if(dblk == 12)											//access indirect block
		{
			char ibuf[BLKSIZE];
			get_block(mip->dev, mip->inode.i_block[dblk], ibuf);
			int *iblk = (int*)ibuf;

			for(int i = 0; i < 256; i++)							//truncate all blocks pointed to from indirect block
			{
				if(!iblk[i])
					break;
				bdealloc(mip->dev, iblk[i]);
				put_block(mip->dev, iblk[i], nbuf);
			}
		}

		else														//access double indirect block
		{
			char dibuf[BLKSIZE];
			get_block(mip->dev, mip->inode.i_block[dblk], dibuf);
			int *diblk = (int*)dibuf;

			for(int di = 0; di < 256; di++)							//truncate all indirect blocks pointed to from double indirect block
			{
				if(!diblk[di])
					break;

				char ibuf[BLKSIZE];
				get_block(mip->dev, diblk[di], ibuf);
				int *iblk = (int*)ibuf;

				for(int i = 0; i < 256; i++)						//truncate all blocks pointed to from indirect block
				{
					if(!iblk[i])
						break;
					bdealloc(mip->dev, iblk[i]);
					put_block(mip->dev, iblk[i], nbuf);
				}
			}
		}
	}
}