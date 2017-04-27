#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <ext2fs/ext2_fs.h>

#include "system.h"
#include "Help/help.h"
#include "LevelOne/level_one.h"
#include "LevelTwo/level_two.h"
#include "LevelThree/level_three.h"

//description: initialize global variables
//parameter:
//return:
int init_global()
{
	proc[0] = (PROC){.uid = 0, .cwd = 0};
	proc[1] = (PROC){.uid = 1, .cwd = 0};

	for(int i = 0; i < 100; i++)
		minode[i] = (MINODE){.refCount = 0};

	root = 0;
}

//description: mount root directory of file system to an minode
//parameter:
//return:
int mount_root()
{
	printf("MOUNTING ROOT...\n");
	root = iget(dev, 2);
	root->mounted = 1;
	
	mntable[0] = (MNTABLE*)malloc(sizeof(MNTABLE));
	mntable[0]->dev = dev;
	strcpy(mntable[0]->name, device);
	mntable[0]->pmip = root;
	
	root->mntptr = mntable[0];
}

//description: initialize proccess and cwd
//parameter:
//return:
int init_proc()
{
	printf("INITIALIZING PROCESSES...\n");
	running = &proc[0];
	running->cwd = iget(dev, 2);
}

//description: determine index of command
//parameter: command
//return: index of command
int find_cmd(char *cmd)
{
	int i = 0;

	for(i = 0; cmds[i]; i++)
	{
		if(!strcmp(cmd, cmds[i]))	//if command found return index
			return i;
	}

	return -1;						//return false if command never found
}

//description: parse path variables from line
//parameter: line, cmd, path, directory, and base pointers
//return: 
int parse_path(char *line, char *cmd, char *path, char *directory, char *base)
{
		sscanf(line, "%s %s", cmd, path);

		printf("icmd = %d cmd = %s pathname = %s\n", icmd, cmd, path);

		det_dirname(path, directory);	//parse path into directory and base
		det_basename(path, base);

		printf("dir = %s base = %s\n", directory, base);
}

//description: run as main.out [device name]
//parameter: argument count and values
//return:
int main(int argc, char *argv[ ])
{
	char line[MAXLINE] = "";		//line entered by user
	char cmd[MAXCMD] = "";			//command parsed from line

	char pathname[MAXPATH] = "";	//path name
	char directory[MAXPATH] = "";	//directory of path name
	char base[MAXPATH] = "";		//base of path name



	if (argc > 1)								//if device name supplied overwrite default
		device = argv[1];

	if ((dev = open(device, O_RDWR)) < 0){		//if device fails to open display error and exit
		printf("ERROR: failed to open %s\n", device);
		exit(1);
	}

	printf("CHECKING EXT2 FS...\n");			//read device and initialize globals
	super_block();
	group_descriptor();
	init_global();
	mount_root();
	init_proc();

	while(1){

		for(int i = 0; i < 66; i++)				//display border
			printf("=");
		printf("\n\n\ncf: ");

		fgets(line, 128, stdin);				//read line
		line[strlen(line)] = 0;

		printf("line = %s", line);				//display line and parse values
		sscanf(line, "%s", cmd);

		icmd = find_cmd(cmd);					//determine index of command

		switch(icmd)
		{
			case QUIT:							//put back all minodes before quitting
				printf("\nSaving...\n");
				for(int i = 0; i < 66; i++)
					printf("=");
				printf("\n\n\n");
				iput_all();
				exit(1);
				break;
			case LS:										//list contents within directory
				parse_path(line, cmd, pathname, directory, base);
				local_ls(pathname);
				break;
			case STAT:										//list directory struct infomation within directory
				parse_path(line, cmd, pathname, directory, base);
				local_stat(pathname);
				break;
			case CD:										//change directory
				parse_path(line, cmd, pathname, directory, base);
				local_chdir(pathname);
				break;
			case PWD:										//print working directory
				local_pwd(running->cwd);
				break;
			case MKDIR:										//make directory
				parse_path(line, cmd, pathname, directory, base);
				local_mkdir(pathname);
				break;
			case CREAT:										//create file
				parse_path(line, cmd, pathname, directory, base);
				local_creat(pathname);
				break;
			case RMDIR:										//remove directory
				parse_path(line, cmd, pathname, directory, base);
				local_rmdir(pathname);
				break;
			case RM:										//remove directory
				parse_path(line, cmd, pathname, directory, base);
				local_rm(pathname);
				break;
			case LINK:										//create hard link to file or link
				{
					char linkname[MAXPATH] = "";
					char linkdirectory[MAXPATH] = "";
					char linkbase[MAXPATH] = "";
					sscanf(line, "%s %s %s", cmd, pathname, linkname);

					printf("icmd = %d cmd = %s pathname = %s linkname = %s\n", icmd, cmd, pathname, linkname);

					det_dirname(pathname, directory);		//parse path into directory and base
					det_basename(pathname, base);
					printf("dir = %s base = %s\n", directory, base);

					det_dirname(linkname, linkdirectory);	//parse link path into directory and base
					det_basename(linkname, linkbase);
					printf("link dir = %s link base = %s\n", linkdirectory, linkbase);

					local_link(pathname, linkname);
					break;
				}
			case UNLINK:									//remove hard link
				parse_path(line, cmd, pathname, directory, base);
				local_unlink(pathname);
				break;
			case SYMLINK:									//creat soft link to directory or file
				{
					char linkname[MAXPATH] = "";
					char linkdirectory[MAXPATH] = "";
					char linkbase[MAXPATH] = "";
					sscanf(line, "%s %s %s", cmd, pathname, linkname);

					printf("icmd = %d cmd = %s pathname = %s linkname = %s\n", icmd, cmd, pathname, linkname);

					det_dirname(pathname, directory);		//parse path into directory and base
					det_basename(pathname, base);
					printf("dir = %s base = %s\n", directory, base);

					det_dirname(linkname, linkdirectory);	//parse link path into directory and base
					det_basename(linkname, linkbase);
					printf("link dir = %s link base = %s\n", linkdirectory, linkbase);

					local_symlink(pathname, linkname);
					break;
				}
			case READLINK:									//read contents of soft link
				{
					char contents[MAXPATH] = "";
					parse_path(line, cmd, pathname, directory, base);
					local_readlink(pathname, contents);
					break;
				}
			case CHMOD:
				{
					int mode = -1;
					sscanf(line, "%s %d %s", cmd, &mode, pathname);

					printf("icmd = %d cmd = %s mode = %d pathname = %s\n", icmd, cmd, mode, pathname);

					det_dirname(pathname, directory);		//parse path into directory and base
					det_basename(pathname, base);
					printf("dir = %s base = %s\n", directory, base);

					local_chmod(mode, pathname);
					break;
				}
			case TOUCH:							
				parse_path(line, cmd, pathname, directory, base);
				local_touch(pathname);
				break;
			case OPEN:										//open file
				{
					int mode = -1;
					sscanf(line, "%s %s %d", cmd, pathname, &mode);

					printf("icmd = %d cmd = %s pathname = %s mode = %d\n", icmd, cmd, pathname, mode);

					det_dirname(pathname, directory);		//parse path into directory and base
					det_basename(pathname, base);
					printf("dir = %s base = %s\n", directory, base);

					local_open(pathname, mode);
					break;
				}
			case CLOSE:										//close file
				{
					int fd = -1;
					sscanf(line, "%s %d", cmd, &fd);

					printf("icmd = %d cmd = %s fd = %d\n", icmd, cmd, fd);

					local_close(fd);
					break;
				}
			case LSEEK:										//change offset of file
				{
					int fd = -1;
					int offset = -1;
					sscanf(line, "%s %d %d", cmd, &fd, &offset);

					printf("icmd = %d cmd = %s fd = %d offset = %d\n", icmd, cmd, fd, offset);

					local_lseek(fd, offset);
					break;
				}
			case PFD:										//display all open files
				local_pfd();
				break;
			case READ:										//change offset of file
				{
					int fd = -1;
					int nbytes = -1;
					sscanf(line, "%s %d %d", cmd, &fd, &nbytes);

					printf("icmd = %d cmd = %s fd = %d nbytes = %d\n", icmd, cmd, fd, nbytes);

					local_read(fd, nbytes);
					break;
				}
			case CAT:										//display file to screen
				parse_path(line, cmd, pathname, directory, base);
				local_cat(pathname);
				break;
			case WRITE:										//display string to file
				{
					int fd = -1;
					char buf[MAXLINE] = "";
					sscanf(line, "%s %d %[^\n]c", cmd, &fd, buf);

					printf("icmd = %d cmd = %s fd = %d data = %s\n", icmd, cmd, fd, buf);

					local_write(fd, buf);
					break;
				}
			case CP:										//copy file
				{
					char destname[MAXPATH] = "";
					char destdirectory[MAXPATH] = "";
					char destbase[MAXPATH] = "";
					sscanf(line, "%s %s %s", cmd, pathname, destname);

					printf("icmd = %d cmd = %s pathname = %s destname = %s\n", icmd, cmd, pathname, destname);

					det_dirname(pathname, directory);		//parse path into directory and base
					det_basename(pathname, base);
					printf("dir = %s base = %s\n", directory, base);

					det_dirname(destname, destdirectory);	//parse link path into directory and base
					det_basename(destname, destbase);
					printf("dest dir = %s dest base = %s\n", destdirectory, destbase);

					local_cp(pathname, destname);
					break;
				}
			case MV:										//move file
				{
					char destname[MAXPATH] = "";
					char destdirectory[MAXPATH] = "";
					char destbase[MAXPATH] = "";
					sscanf(line, "%s %s %s", cmd, pathname, destname);

					printf("icmd = %d cmd = %s pathname = %s destname = %s\n", icmd, cmd, pathname, destname);

					det_dirname(pathname, directory);		//parse path into directory and base
					det_basename(pathname, base);
					printf("dir = %s base = %s\n", directory, base);

					det_dirname(destname, destdirectory);	//parse link path into directory and base
					det_basename(destname, destbase);
					printf("dest dir = %s dest base = %s\n", destdirectory, destbase);

					local_mv(pathname, destname);
					break;
				}
			case MOUNT:										//mount file system to path
				{
					char filesystem[MAXPATH] = "";
					char fsdirectory[MAXPATH] = "";
					char fsbase[MAXPATH] = "";
					sscanf(line, "%s %s %s", cmd, filesystem, pathname);

					printf("icmd = %d cmd = %s filesystem = %s pathname = %s \n", icmd, cmd, filesystem, pathname);

					det_dirname(filesystem, fsdirectory);	//parse link path into directory and base
					det_basename(filesystem, fsbase);
					printf("fs dir = %s fs base = %s\n", fsdirectory, fsbase);

					det_dirname(pathname, directory);		//parse path into directory and base
					det_basename(pathname, base);
					printf("dir = %s base = %s\n", directory, base);

					local_mount(filesystem, pathname);
					break;
				}
			case UMOUNT:									//unmount file system
				{
					char filesystem[MAXPATH];
					sscanf(line, "%s %s", cmd, filesystem);

					printf("icmd = %d cmd = %s filesystem = %s\n", icmd, cmd, filesystem);

					local_umount(filesystem);
					break;
				}
			default:
				printf("What in the god damn hell you talking about?\n");
				break;
		}

		cmd[0] = 0;
		pathname[0] = 0;
	}
}
