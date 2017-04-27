//description: mount file system to mount point path
//parameter: path to mount point and mount
//return: success or fail
int display_mount()
{
	printf("[%6s, %6s] %5s %s\n", "mp dev", "mp ino", "m dev", "file system");

	for(int i = 0; i < NMNT; i++)
	{
		if(mntable[i])
		{
			printf("[%6d, %6d] %5d %s\n", mntable[i]->pmip->dev, mntable[i]->pmip->ino, mntable[i]->dev, mntable[i]->name);;
		}
	}
}

//description: mount file system to mount point path
//parameter: path to mount point and mount
//return: success or fail
int local_mount(char *mount, char *mount_point)
{
	if(!mount_point[0] || !mount[0])
	{
		display_mount();
		return;
	}

	if(mount_point[0] == '/')							//initialize device depending on absolute or relative path
		dev = root->dev;
	else
		dev = running->cwd->dev;

	for(int i = 0; i < NMNT; i++)
	{
		if(mntable[i] && !strcmp(mntable[i]->name, mount))
		{
			printf("ERROR: %s is already mounted\n", mount);
			return -1;
		}
	}

	int ino = getino(&dev, mount_point);
	if(ino < 0)
		return -1;

	MINODE *mip = iget(dev, ino);
	if(!S_ISDIR(mip->inode.i_mode))					//if parent minode is not a directory display error and  return fail
	{
		printf("ERROR: %s is not a directory\n", mount_point);
		iput(mip);
		return -1;
	}

	if(mip->mounted)
	{
		printf("ERROR: %s is already mounted on\n", mount_point);
		iput(mip);
		return -1;
	}

	int mdev;
	if ((mdev = open(mount, O_RDWR)) < 0){		//if device fails to open display error and exit
		printf("ERROR: failed to open %s\n", mount);
		iput(mip);
		return(1);
	}

	int i = 0;
	for(i = 0; i < NMNT; i++)
		if(!mntable[i])
			break;

	if(i == NMNT)
	{		
		printf("ERROR: no memory avialable to mount\n");
		iput(mip);
		close(mdev);
		return(1);
	}

	MINODE *mmip = iget(mdev, 2);
	if(!mmip)
	{
		iput(mip);
		close(mdev);
		return(1);
	}

	char buf[BLKSIZE];
	get_block(mdev, 1, buf);			//read super block into buffer
	SUPER *msp = (SUPER *)buf;				//cast buffer as super block

	if (msp->s_magic != 0xEF53){		//if the file system is not ext2 display error and exit
	  printf("ERROR: Device is not an EXT2 file system\n");
		iput(mip);
		iput(mmip);
		close(mdev);
	  return -1;
	}

	mntable[i] = (MNTABLE*)malloc(sizeof(MNTABLE));
	mntable[i]->dev = mdev;
	strcpy(mntable[i]->name, mount);
	mntable[i]->pmip = mip;

	mip->mounted = mmip->mounted = 1;
	mip->mntptr = mmip->mntptr = mntable[i];
	iput(mmip);

	return 1;
}

//description: unmount file system
//parameter: path to mount
//return: success or fail
int local_umount(char *filesystem)
{
	if(!filesystem[0])								//if path is null then display error and return fail
	{
		printf("ERROR: file system name not specified\n");
		return -1;
	}

	int i;
	for(i = 0; i < NMNT; i++)
	{
		if(mntable[i] && !strcmp(mntable[i]->name, filesystem))
			break;
	}

	if(i == NFD)
	{
		printf("ERROR: file system is not mounted\n");
		return -1;
	}

	for(int n = 0; n < NMINODE; n++)
	{
		if(minode[n].refCount && minode[n].dev == mntable[i]->dev)
		{
			printf("ERROR: file system is still in use\n");
			return -1;
		}
	}

	MINODE *mip = mntable[i]->pmip;
	mip->mntptr = 0;
	mip->mounted = 0;

	iput(mip);
	free(mntable[i]);
	mntable[i] = 0;

	return 0;
}