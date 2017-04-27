//description: change permissions of an item
//parameter: mode and path to item
//return: success or fail
int local_chmod(int mode, char *path)
{
	if(!path[0])														//if path is null then display error and return fail
	{
		printf("ERROR: path name not specified\n");
		return -1;
	}

	if((mode / 100) < 0 || (mode / 100) > 7 ||							//if permission is invalid display error and return fail
		((mode % 100) / 10) < 0 || ((mode % 100) / 10) > 7 ||
		(mode % 10) < 0 || (mode % 10) > 7)
	{
		printf("ERROR: permission is invalid\n");
		return -1;
	}
	
	int ino = getino(&dev, path);										//determine inode of link
	if(ino < 0)															//if link not found display error and return fail
		return -1;

	MINODE *mip = iget(dev, ino);										//get minode of item
	
																		//if mode contains bit then OR bits if not then AND with the INVERSE of the bit
	mip->inode.i_mode = (mode / 100) & 4 ? mip->inode.i_mode | S_IRUSR : mip->inode.i_mode & ~S_IRUSR;
	mip->inode.i_mode = (mode / 100) & 2 ? mip->inode.i_mode | S_IWUSR : mip->inode.i_mode & ~S_IWUSR;
	mip->inode.i_mode = (mode / 100) & 1 ? mip->inode.i_mode | S_IXUSR : mip->inode.i_mode & ~S_IXUSR;
	mip->inode.i_mode = ((mode % 100) / 10) & 4 ? mip->inode.i_mode | S_IRGRP : mip->inode.i_mode & ~S_IRGRP;
	mip->inode.i_mode = ((mode % 100) / 10) & 2 ? mip->inode.i_mode | S_IWGRP : mip->inode.i_mode & ~S_IWGRP;
	mip->inode.i_mode = ((mode % 100) / 10) & 1 ? mip->inode.i_mode | S_IXGRP : mip->inode.i_mode & ~S_IXGRP;
	mip->inode.i_mode = (mode % 10) & 4 ? mip->inode.i_mode | S_IROTH : mip->inode.i_mode & ~S_IROTH;
	mip->inode.i_mode = (mode % 10) & 2 ? mip->inode.i_mode | S_IWOTH : mip->inode.i_mode & ~S_IWOTH;
	mip->inode.i_mode = (mode % 10) & 1 ? mip->inode.i_mode | S_IXOTH : mip->inode.i_mode & ~S_IXOTH;

	mip->dirty = 1;														//mark minode as dity and put back
	iput(mip);
	return 1;
}

//description: update access time of an item
//parameter: path to item
//return: success or fail
int local_touch(char *path)
{
	if(!path[0])									//if path is null then display error and return fail
	{
		printf("ERROR: path name not specified\n");
		return -1;
	}
	
	int ino = getino(&dev, path);					//determine inode of link
	if(ino < 0)										//if link not found display error and return fail
		return -1;

	MINODE *mip = iget(dev, ino);

	mip->inode.i_mtime = time(0L);					//update modification time of the item		

	mip->dirty = 1;									//mark minode as dity and put back
	iput(mip);
}