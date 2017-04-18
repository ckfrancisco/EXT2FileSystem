//description: create a soft link to an existing file
//parameter: path to existing file and link file
//return: success or fail
int link(char *path, char *linkpath)
{
	int oino = getino(&dev, path);										//determine inode of existing file
	if(oino < 0)														//if file not found display error and return fail
	{
		printf("ERROR: %s does not exist\n", path);
		return -1;
	}

	MINODE *omip = iget(dev, oino);										//get minode of existing file
	if(!S_ISREG(omip->inode.i_mode) && !S_ISLNK(omip->inode.i_mode))	//if minode is not a file or link display error and return fail
	{
		printf("ERROR: %s is not a file or link\n", path);
		iput(omip);
		return -1;
	}

	char linkdirectory[MAXPATH];										//parse link path
	char linkbase[MAXPATH];
	det_dirname(linkpath, linkdirectory);
	det_basename(linkpath, linkbase);

	int pino = getino(&dev, linkdirectory);								//determinode parent inode of link
	if(pino < 0)														//if parent inode not found display error and return fail
	{
		printf("ERROR: %s does not exist\n", linkdirectory);
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

	if(search(pmip, linkbase) > 0)										//if name of link already exists display error and return fail
	{
		printf("ERROR: %s already exists\n", linkbase);
		iput(omip);
		iput(pmip);
		return -1;
	}

	int result =  enter_name(pmip, oino, linkbase);						//enter name of link into parent directory

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
	int ino = getino(&dev, path);									//determine inode of existing file
	if(ino < 0)														//if file not found display error and return fail
	{
		printf("ERROR: %s does not exist\n", path);
		return -1;
	}

	MINODE *mip = iget(dev, oino);									//get minode of existing file
	if(!S_ISREG(mip->inode.i_mode) && !S_ISLNK(mip->inode.i_mode))	//if minode is not a file or link display error and return fail
	{
		printf("ERROR: %s is not a file or link\n", path);
		iput(omip);
		return -1;
	}

	mip->inode.i_links_count--;										//decrement minode link count

	if(mip->inode.i_links_count == 0)								//if link count is 0 truncate all data blocks
	{
		
	}

	int pino = getino(&dev, directory);								//determine parent inode of link
	MINODE *pmip = iget(dev, pino);									//determine parent minode of link

	int result =  rm_child(pmip, base);								//remove link from parent directory

	iput(mip);														//put back minode
	iput(pmip);														//put back parent minode

	return result;
}

//description: create a hard link to an existing file
//parameter: path to existing file and link file
//return: success or fail
int symlink(char *path, char *linkpath)
{
	int oino = getino(&dev, path);										//determine inode of existing file
	if(oino < 0)														//if file not found display error and return fail
	{
		printf("ERROR: %s does not exist\n", path);
		return -1;
	}

	MINODE *omip = iget(dev, oino);										//get minode of existing file
	if(!S_ISREG(omip->inode.i_mode) && !S_ISLNK(omip->inode.i_mode))	//if minode is not a file or link display error and return fail
	{
		printf("ERROR: %s is not a file or link\n", path);
		iput(omip);
		return -1;
	}

	int ino = creat_file(linkpath);										//create file and record inode of hard link
	if(ino < 0)															//if file not created error was displayed in previous funtion so return fail
	{
		iput(omip);
		return -1;
	}

	MINODE *mip = iget(dev, ino);										//determine minode of link
	mip->inode.i_mode = 0xA1FF;											//adjust from file to hard link
	strcpy((char*)mip->inode.i_block, base);							//copy base of existing path to block
	mip->inode.i_size = strlen(base);									//adjust size of hard link to length of base
	iput(mip);

	return 1;															//return success
}

//
//
//