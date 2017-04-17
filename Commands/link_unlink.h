//description: 
//parameter: 
//return: 
int link(char *path, char *linkpath)
{
	int oino = getino(&dev, path);
	if(oino < 0)
	{
		printf("ERROR: %s does not exist\n", path);
		return -1;
	}

	MINODE *omip = iget(dev, oino);
	if(!S_ISREG(omip->inode.i_mode) && !S_ISLNK(omip->inode.i_mode))
	{
		printf("ERROR: %s is not a file or link\n", path);
		iput(omip);
		return -1;
	}

	char linkdirectory[MAXPATH];
	char linkbase[MAXPATH];
	det_dirname(linkpath, linkdirectory);
	det_basename(linkpath, linkbase);

	int pino = getino(&dev, linkdirectory);
	if(pino < 0)
	{
		printf("ERROR: %s does not exist\n", linkdirectory);
		iput(omip);
		return -1;
	}

	MINODE *pmip = iget(dev, pino);
	if(!S_ISDIR(pmip->inode.i_mode))
	{
		printf("ERROR: %s is not a directory\n", linkdirectory);
		iput(omip);
		iput(pmip);
		return -1;
	}

	if(search(pmip, linkbase) > 0)
	{
		printf("ERROR: %s already exists\n", linkbase);
		iput(omip);
		iput(pmip);
		return -1;
	}

	int result =  enter_name(pmip, oino, linkbase);

	if(result > 0)
		omip->inode.i_links_count++;

	iput(omip);
	iput(pmip);

	return result;
}

//description: 
//parameter: 
//return: 
int unlink(char *path)
{
	int ino = getino(&dev, path);
	if(ino < 0)
	{
		printf("ERROR: %s does not exist\n", path);
		return -1;
	}

	MINODE *mip = iget(dev, ino);
	if(!S_ISREG(mip->inode.i_mode) && !S_ISLNK(mip->inode.i_mode))
	{
		printf("ERROR: %s is not a file or link\n", path);
		iput(mip);
		return -1;
	}

	mip->inode.i_links_count--;

	if(mip->inode.i_links_count == 0)
	{
		//
	}

	int pino = getino(&dev, directory);
	MINODE *pmip = iget(dev, pino);

	int result =  rm_child(pmip, base);

	iput(mip);

	return result;
}

//description: 
//parameter: 
//return: 
int symlink(char *path, char *linkpath)
{
	int oino = getino(&dev, path);
	if(oino < 0)
	{
		printf("ERROR: %s does not exist\n", path);
		return -1;
	}

	MINODE *omip = iget(dev, oino);
	if(!S_ISREG(omip->inode.i_mode) && !S_ISDIR(omip->inode.i_mode))
	{
		printf("ERROR: %s is not a file or directory\n", path);
		iput(omip);
		return -1;
	}

	int ino = creat_file(linkpath);
	if(ino < 0)
	{
		iput(omip);
		return -1;
	}

	MINODE *mip = iget(dev, ino);
	mip->inode.i_mode = 0xA000;

	
}