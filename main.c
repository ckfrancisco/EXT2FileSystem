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
}

//description: initialize proccess and cwd
//parameter:
//return:
int init_proc()
{
	printf("INITIALIZING PROCESSES...\n");
	running = &proc[0];
	running->cwd = &minode[0];
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

//description: run as main.out [device name]
//parameter: argument count and values
//return:
int main(int argc, char *argv[ ])
{
	char directory[MAXPATH];
	char base[MAXPATH];

	if (argc > 1)							//if device name supplied overwrite default
		device = argv[1];

	if ((dev = open(device, O_RDWR)) < 0){	//if device fails to open display error and exit
		printf("ERROR: failed to open %s\n", device);
		exit(1);
	}

	printf("CHECKING EXT2 FS...\n");		//read device and initialize globals
	super_block();
	group_descriptor();
	init_global();
	mount_root();
	init_proc();

	while(1){

		for(int i = 0; i < 66; i++)			//display border
			printf("=");
		printf("\n\n\ncf: ");

		fgets(line, 128, stdin);			//read line
		line[strlen(line)] = 0;

		printf("line = %s", line);			//display line and parse values
		sscanf(line, "%s %s %s", cmd, pathname, arg);

		icmd = find_cmd(cmd);				//determine index of command

		printf("icmd = %d cmd = %s pathname = %s\n", icmd, cmd, pathname);

		det_dirname(pathname, directory);	//parse path into directory and base
		det_basename(pathname, base);

		printf("dir = %s base = %s\n", directory, base);

		switch(icmd)
		{
			case QUIT:							//put back all minodes before quitting
				printf("Saving...\n");
				for(int i = 0; i < 66; i++)
					printf("=");
				printf("\n\n\n");
				iput_all();
				exit(1);
				break;
			case LS:							//list contents within directory
				ls(pathname);
				break;
			case LSDIR:							//list directory struct infomation within directory
				lsdir(pathname);
				break;
			case CD:							//change directory
				chdir(pathname);
				break;
			case PWD:							//print working directory
				pwd(running->cwd);
				break;
			case MKDIR:							//make directory
				mk_dir(pathname);
				break;
			case CREAT:							//create file
				creat_file(pathname);
				break;
			case RMDIR:							//remove directory
				rm_dir(pathname);
				break;
			case RM:							//remove directory
				rm_file(pathname);
				break;
			case LINK:							//create hard link to file or link
				link(pathname, arg);
				break;
			case UNLINK:						//remove hard link
				unlink(pathname);
				break;
			case SYMLINK:						//creat soft link to directory or file
				symlink(pathname, arg);
				break;
			case READLINK:						//read contents of soft link
				readlink(pathname, arg);
				break;
			/*case OPEN:							//open file
				int mode = atoi(arg);
				open(pathname, mode);
				break;
			case CLOSE:							//close file
				int fd = 0;
				close(fd);
				break;
			case LSEEK:							//change offset of file
				int fd = atoi(pathname);
				int pos = atoi(arg);
				lseek(fd, pos);
				break;
			case PFD:							//display all open files
				pfd();
				break;*/
			default:
				printf("What in the god damn hell you talking about?\n");
				break;
		}

		cmd[0] = 0;
		pathname[0] = 0;
		arg[0] = 0;
	}
}
