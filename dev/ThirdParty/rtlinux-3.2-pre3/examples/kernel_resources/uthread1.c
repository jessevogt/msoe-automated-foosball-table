#include <unistd.h>
	
char cmd_path[256] = "/bin/echo";
char main_arg[] ="Hello User World";

main(){
	static char * envp[] = { "HOME=/root", 
		"TERM=linux", 
		"PATH=/bin", 
		NULL };
	char *argv[] = { 
		cmd_path,
		main_arg,
		NULL };
	int ret;

	printf("calling execve for %s \n",cmd_path); 
	ret = execve(cmd_path, argv, envp);

	/* if we ever get here - execve failed */
	printf("failed to exec %s, ret = %d\n", cmd_path,ret);
	return -1;
}
