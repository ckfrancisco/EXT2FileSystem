//description: 
//parameter: 
//return: 
int link(char *path, char *link_path)
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

	char link_directory[MAXPATH];
	char link_base[MAXPATH];
	det_dirname(link_path, link_directory);
	det_basename(link_path, link_base);

	int pnino = getino(&dev, link_directory);
	if(pnino < 0)
	{
		printf("ERROR: %s does not exist\n", link_directory);
		iput(omip);
		return -1;
	}

	MINODE *pnmip = iget(dev, pnino);
	if(!S_ISDIR(pnmip->inode.i_mode))
	{
		printf("ERROR: %s is not a directory\n", link_directory);
		iput(omip);
		iput(pnmip);
		return -1;
	}

	if(search(pnmip, link_base) > 0)
	{
		printf("ERROR: %s already exists\n", link_base);
		iput(omip);
		iput(pnmip);
		return -1;
	}

	int result =  enter_name(pnmip, oino, link_base);

	if(result > 0)
		omip->inode.i_links_count++;

	iput(omip);
	iput(pnmip);

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

	MINODE *pmip = iget_parent(mip);

	int result =  rm_child(pmip, base);

	iput(mip);

	return result;
}

//description: 
//parameter: 
//return: 