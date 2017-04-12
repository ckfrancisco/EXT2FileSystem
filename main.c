#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <ext2fs/ext2_fs.h>

#include "system.h"
#include "Help/help.h"
#include "Commands/commands.h"

//description: mount root directory of filesystem to an minode
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

int main(int argc, char *argv[ ])	// run as a.out [diskname]
{
	int i;

	if (argc > 1)
		disk = argv[1];

	if ((dev = open(disk, O_RDWR)) < 0){
		printf("ERROR: failed to open %s\n", disk);
		exit(1);
	}

	printf("CHECKING EXT2 FS...\n");
	super_block();
	group_descriptor();
	init_global();
	mount_root();
	init_proc();

	while(1){

		for(i = 0; i < 66; i++)
			printf("=");
		printf("\n\n\ncf: ");

		fgets(line, 128, stdin);
		line[strlen(line)] = 0;

		printf("line=%s", line);
		sscanf(line, "%s %s", cmd, pathname);

		icmd = find_cmd(cmd);

		printf("icmd = %d cmd = %s pathname = %s\n", icmd, cmd, pathname);

		det_dirname(pathname, directory);
		det_basename(pathname, base);

		printf("dir = %s base = %s\n", directory, base);

		switch(icmd)
		{
			case 0:
				ls(pathname);
				break;
			case 1:
				lsdir(pathname);
				break;
			case 2:
				chdir(pathname);
				break;
			case 3:
				pwd(running->cwd);
				break;
			case 4:
				mk_dir(pathname);
				break;
			case 5:
				creat_file(pathname);
				break;
			case 6:
				//rm_dir(pathname);
				break;
			case 7:
				exit(1);
				break;
			default:
				printf("What in the god damn hell you talking about?\n");
				break;
		}

		cmd[0] = 0;
		pathname[0] = 0;
	}
}
