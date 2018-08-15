/****************************************************************
* Name        :                                                *
* Class       : CSC 415                                        *
* Date        :                                                *
* Description :  Writting a simple bash shell program          *
* 	          that will execute simple commands. The main   *
*                goal of the assignment is working with        *
*                fork, pipes and exec system calls.            *
****************************************************************/
//May add more includes
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <dirent.h>
//Max amount allowed to read from input
#define BUFFERSIZE 256
//Shell prompt
#define PROMPT "myShell >> "
//sizeof shell prompt
#define PROMPTSIZE sizeof(PROMPT)

/********************
Resources used to help: (Will add as I go along)
https://youtu.be/QUCSyDFPbOI
https://www.quora.com/What-are-some-good-tutorials-for-writing-a-shell-in-C
https://stackoverflow.com/questions/1461331/writing-my-own-shell-stuck-on-pipes?rq=1


*********************/

void printDir() //Did this so I know where I am 
{
	char cwd[BUFFERSIZE];
	getcwd(cwd, sizeof(cwd));
	printf("\nDir: %s\n", cwd);
}

int shellCD(char **args);
int shellPWD(char **args);
int shellHELP(char **args);
int shellEXIT(char **args);


char *builtString[] = 
{
	"cd",
	"pwd",
	"help",
	"exit"
};

int(*builtFunc[]) (char **) = //Argument swapper I found on internet a while back
{
	&shellCD,
	&shellPWD,
	&shellHELP,
	&shellEXIT
};

int builtIn()
{
	return sizeof(builtString) / sizeof(char *);
}

int shellCD(char **args)
{
	if (args[1] == NULL)
	{
		chdir(".."); //Goes up root directory
	}
	else
	{
		int value;
		value = chdir(args[1]); //Goes to specified directory
		if (value != 0)
		{
			perror("cd failed");
		}
	}
	return 1;
}

int shellPWD(char **args)
{
	char cwd[BUFFERSIZE];
	getcwd(cwd, sizeof(cwd));
	printf("\n%s\n", cwd);
	return 1;
}

int shellHELP(char **args)
{
	printf("The following are built in:\n");

	for (int i = 0; i < builtIn(); i++) {
		printf("%d.  %s\n", (i + 1), builtString[i]);
	}

	printf("Use the man command for information on other programs.\n");
	return 1;
}

int shellEXIT(char **args)
{
	return 0;
}

int launchPipeProgram(char **command1, char **command2) //Doesn't work "Segmentation Error"
{
	int fd[2];
	pid_t childPid;
	if (pipe(fd) != 0)
		perror("failed to create pipe");

	if ((childPid = fork()) == -1)
		perror("failed to fork");

	if (childPid == 0)
	{
		dup2(fd[1], 1);
		close(fd[0]);
		close(fd[1]);
		execvp(command1[0], command1);
		perror("failed to exec command 1");
	}
	else
	{
		dup2(fd[0], 0);
		close(fd[0]);
		close(fd[1]);
		execvp(command2[0], command2);
		perror("failed to exec command 2");
	}
	return 1;
}

int launchProgram(char **args)
{
	pid_t pid, wpid;
	int status;

	pid = fork();
	if (pid == 0)
	{
		if (execvp(args[0], args) == -1)
		{
			perror("Error starting fork");
		}
		exit(EXIT_FAILURE);
	}
	else if (pid < 0)
	{
		perror("Error forking");
	}
	else
	{
		do
		{
			wpid = waitpid(pid, &status, WUNTRACED);
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
	}
	return 1;
}

int execute(char **args)
{
	int i;

	if (args[0] == NULL) //Empty string, go back to loop
	{
		return 1;
	}

	for (i = 0; i < builtIn(); i++) //If the command is one of the built in commands then return that otherwise go to launch program

	{
		if (strcmp(args[0], builtString[i]) == 0)
		{
			return (*builtFunc[i])(args);
		}
	}
	return launchProgram(args);
}

#define DELIM " \t\n"

int checkForPipe(char *line)
{
	int flag = 0, i = 0;
	int length = strlen(line);
	for(i = 0; i < length; i++)
	{
		if (line[i] == '|')
		{
			flag = 1;
		}
	}
	return flag;
}

void **splitPipeCommand(char *line, char **command1, char **command2)
{
	int bufsize = BUFFERSIZE;
	int position = 0;
	char *token;

	int length = strlen(line);
	int pipeLocation = 0, i = 0;
	for(i = 0; i < length; i++)
	{
		if (line[i] == '|')
		{
			pipeLocation = i;
		}
	}
	
	token = strtok(line, DELIM);
	while (position < pipeLocation)
	{
		command1[position] = token;
		position++;

		token = strtok(NULL, DELIM);
	}
	command1[position] = NULL;
	
	
	position = pipeLocation + 1;
	token = strtok(line, DELIM);
	while (position < length)
	{
		command2[position] = token;
		position++;

		token = strtok(NULL, DELIM);
	}

	command2[position] = NULL;

}

char *readLine()
{
	char *line = NULL;
	ssize_t bufferSize = 0;
	getline(&line, &bufferSize, stdin);
	return line;
}


char **splitLine(char *line) //Tokenizes the string input by user
{
	int bufsize = BUFFERSIZE;
	int position = 0;
	char **tokens = malloc(bufsize * sizeof(char*));
	char *token;

	if (!tokens)
	{
		fprintf(stderr, "Error with tokens!");
		exit(EXIT_FAILURE);
	}

	token = strtok(line, DELIM);
	while (token != NULL)
	{
		tokens[position] = token;
		position++;

		token = strtok(NULL, DELIM);
	}
	tokens[position] = NULL;
	return tokens;
}

//#define ARGSIZEMAX 32

void shellLoop()
{
	char *buff, **myargv1, **myargv2;
	int status;
	int hasPipe = 0;

	do
	{

		printf("\n");
		printDir();
		printf("\n");
		printf(PROMPT);
		buff = readLine();
		hasPipe = checkForPipe(buff);
		if(hasPipe == 0)
		{
			myargv1 = splitLine(buff);
			status = execute(myargv1);
		}
		if (hasPipe == 1)
		{
			splitPipeCommand(buff, myargv1, myargv2);
			status = launchPipeProgram(myargv1, myargv2);
		}


	} while (status);
}

int main(int* argc, char** argv)
{
	//Run loop; this will be where the shell starts and continues to be run until exited
	shellLoop();

	return 0;
}
