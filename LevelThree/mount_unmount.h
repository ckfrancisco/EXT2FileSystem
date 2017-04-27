//description: mount file system to mount point path
//parameter: path to mount point and mount
//return: success or fail
int local_mount(char *mount_point, char *mount)
{
	if(!mount_point[0] || !mount[0])
	{
		//display_mount();
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
	if(mip->mounted)
	{
		printf("ERROR: %s is already mounted on\n", mount_point);
		iput(mip);
		return -1;
	}

	int mdev;
	if ((mdev = open(device, O_RDWR)) < 0){		//if device fails to open display error and exit
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

	int mino = iget(mdev, 2);
	if(ino < 0)
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
	  exit(1);
	}

	MINODE *mmip = iget(mdev, mino);

	mntable[i] = (MNTABLE*)malloc(sizeof(MNTABLE));
	mntable[i]->dev = mdev;
	strcpy(mntable[i]->name, mount);
	mntable[i]->mntptr = mip;

	mip->mounted = mmip->mounted = 1;
	mip->mntptr = mmip->mntptr = mntable[i];
	
	iput(mip);

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
		if(mntable[i] && !strcmp(mntable[i], filesystem))
			break;
	}

	if(i == NFD)
	{
		printf("ERROR: file system is not mounted\n");
		return -1;
	}

	for(int n = 0; i < NMINODE; i++)
	{
		if(minode[n].refCount && minode[n].dev == mntable[i]->dev)
		{
			printf("ERROR: file system is still in use\n");
			return -1;
		}
	}

	MINODE *mip = mntable[i]->mntptr;
	mip->mntptr = 0;
	mip->mounted = 0;

	free(mntable[i]);
	mntable[i] = 0;

	return 1;
}