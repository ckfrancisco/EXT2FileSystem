#define BLKSIZE     1024	//number of bytes per block

#define NMINODE      100	//number of minodes to allocate
#define NFD           16	//max number of open files
#define NPROC          4	//number of processes able to run

#define MAXLINE      128	//max length of line
#define MAXCMD        64	//max length of command
#define MAXPATH      128	//max length of a path name
#define MAXNAME	     128	//max length of a name
#define NNAME         32	//maximum number of names to a paths
#define NARGS          1	//maximum number of args follwing path in a command

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

typedef struct ext2_super_block SUPER;
typedef struct ext2_group_desc  GD;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;

typedef struct minode{
	INODE inode;
	int dev, ino;
	int refCount;
	int dirty;
	int mounted;
	struct mntable *mptr;
}MINODE;

typedef struct oft{
	int     mode;
	int     refCount;
	MINODE *mptr;
	int     offset;
}OFT;

typedef struct proc{
	struct proc *next;
	int          pid;
	int          uid;
	MINODE      *cwd;
	OFT         *fd[NFD];
}PROC;

MINODE minode[NMINODE];	//array of minodes
MINODE *root;			//pointer to the root minode
PROC   proc[NPROC];		//array of process
PROC   *running;		//pointer to the running process

SUPER *sp;				//super block pointer
GD    *gp;				//group descriptor pointer

int dev;				//file descriptor of device
int nblocks;			//number of blocks on device
int ninodes;			//number of inodes on device
int bmap;				//block bitmap block number
int imap;				//inode bitmap block number
int iblock;				//beginning inode block number
int inodes_per_block;	//inodes per block
						//NOTE: Calculated in super_block()

char *device = "cf";	//default device name to read/write

char line[MAXLINE];				//line entered by user
char cmd[MAXCMD];				//command parsed from line
char pathname[MAXPATH];			//path name parsed from line
char names[NNAME][MAXNAME];		//tokenized names within pathname
char directory[MAXPATH];		//directory name of path
char base[MAXPATH];				//base name of path
char arg[MAXPATH];				//secondary command argument

int icmd;						//index of command in command name array
char *cmds[] = {				//array of command names
		"quit",
		"ls",
		"lsdir",
		"cd",
		"pwd",
		"mkdir",
		"creat",
		"rmdir", 
		"link", 
		"unlink", 
		"symlink", 				//end of level one
		"open", 
		"close", 
		"lseek", 
		"pfd",
		"read", 
		"cat", 
		"write",
		"cp",
		"mv", 					//end of level two
		NULL};