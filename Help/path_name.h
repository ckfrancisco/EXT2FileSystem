//description: tokenize path name
//parameter: path
//return:
int tokenize(char *path)
{
	int i = 1;
	char buf[MAXPATH];

	if(!path[0] || !strcmp(path, "/"))				//if path is empty or root return 0
		return 0;

	strcpy(buf, path);								//copy path to buffer to avoid destruction

	char *token = strtok(buf, "/");					//parse path by directories

	strcpy(names[0], token);

	for(i = 1; token = strtok(NULL, "/"); i++)		//tokenize until null token
		strcpy(names[i], token);

	names[i][0] = 0;								//null terminate collection of path names

	return i;										//return number of tokens
}

//description: tokenize directory of path name
//parameter: path
//return:
int det_dirname(char *path, char *directory)
{
	int i = strlen(path) - 1;

	if(!path[0] || !strcmp(path, "/"))			//if path is empty or root return empty parent
	{
		directory[0] = 0;
		return;
	}

	if(path[i] == '/')							//if path ends with '/' remove it
	{
		path[i] = 0;
		i--;
	}

	for(i; i > -1 && path[i] != '/'; i--);		//find last index of '/'

	if(i < 0)									//if none return empty parent
		directory[0] = 0;

	else if(i == 0)								//if path[0] is the only '/' just copy "/"
		strcpy(directory, "/");
	else										//else copy to index of last '/'
		strncpy(directory, path, i);
}

//description: tokenize base of path name
//parameter: path
//return:
int det_basename(char *path, char *base)
{
	int i = strlen(path) - 1;

	if(!path[0])							//if path is empty return empty base
	{
		base[0] = 0;
		return;
	}

	if(!strcmp(path, "/"))					//if path is root copy entire path to base
	{
		strcpy(base, path);
		return;
	}

	if(path[i] == '/')						//if path ends with '/' remove it
	{
		path[i] = 0;
		i--;
	}

	for(i; i > -1 && path[i] != '/'; i--);	//find last index of '/'

	if(i < 0)								//if none copy entire path to base
		strcpy(base, path);

	else									//else copy from index of last '/'
		strcpy(base, &path[i+1]);
}
