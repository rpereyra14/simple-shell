#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>

char * input_string;
char * args[256];

/**
* Parse the input for command name and arguments.
*/
void parseInput( size_t bytes_read ){

	//Replace the last character in the input string (i.e. \n) with \0.
	*(input_string + bytes_read - 1) = '\0';

	int i = 0;

	//Initilize tokenization of input string by whitespace (tabs and spaces).
	char * pch;
	char delims[] = "\t ";
	pch = strtok (input_string, delims);

	//Build args array with input words. args will be passed to execv.
	while ( pch != NULL ){

		args [ i ] = pch;
		++i;

		if ( i >= 256 ){
			puts("Argument limit (256) exceeded. Try again.");
			exit(1);
		}

		pch = strtok (NULL, delims);

	}

	free( pch );

}

/**
* Determine command location in the file system and execute command once found.
* Special case implemented for cd.
*/
void execute( void ){

	//Special case (cd)
	if ( !strcmp( args[0], "cd" ) ){

		puts("The cd command is not supported.");
		exit(1);

	}

	char * envpath = getenv("PATH");
	char * path = malloc ( strlen(envpath) * sizeof(char) );

	//Prevent the rewriting of the environment variable.
	strcpy( path, envpath );
	
	strcat(path,":.");

	//Initialize parsing of paths with strtok.
	char * ptr;
	char delims[] = ":";
	ptr = strtok (path, delims);

	//Full path to be checked by stat.
	char * fpath;

	struct stat BUF;

	//Check if user inputted a full path.
	fpath = args[0];
	if ( stat( fpath, &BUF ) == 0 ){	
		execv( fpath, args );
	}else{

		while ( ptr != NULL ){

			//Build full path to be checked.
			fpath = (char *) malloc ( strlen(ptr) + strlen(args[0]) + 2 );

			strcat(fpath, ptr);

			if ( strcmp(ptr, "/") ){
				strcat(fpath, "/");
			}

			strcat(fpath, args[0]);

			if ( stat( fpath, &BUF ) == 0 ){
				if ( execv( fpath, args ) == -1 ){
					puts("A critical error occurred. Please try again.");
					exit(1);
				}
				break;
			}

			free( fpath );

			ptr = strtok (NULL, delims);

			//strtok returns null once the string to be tokenized is finished.
			//Since loop has not broken out yet, pathname was not found by stat.
			if ( ptr == NULL ){
				puts("Command not found. Try again.");
			}

		}

	}

}

/**
* Read input line and fork. Then, parse input and execute the specified command.
*/
void main ( void ){

	//bytes_read will be used to locate \n in parseInput.
	size_t bytes_read, nbytes = 100;

	input_string = (char *) malloc ( nbytes + 1 );

	printf("%%");

	//Read input line. Getline will realloc input_string as needed.
	while ( (bytes_read = getline( &input_string, &nbytes, stdin )) != -1 ){

		pid_t pid;
		pid = fork();

		if ( !strcmp(input_string, "exit\n") ){
			exit(0);
		}

		//Child parses and executes command.
		if ( pid == 0 ){

			parseInput( bytes_read );
			execute();

			exit(0);

		}else if ( pid < 0 ){

			puts("Process fork failed. Please try again.");
			exit(1);

		}else{

			//Parent should ignore Ctrl+Z and Ctrl+C while child is running.
			signal(SIGTSTP, SIG_IGN);
			signal(SIGINT, SIG_IGN);

			int status;

			if ( waitpid( -1, &status, WUNTRACED ) == -1 ){

				puts("A critical error occurred. Please try again.");
				exit(1);

			//Child has stopped running. Parent should accept signals now.
			}else{

				signal(SIGTSTP, SIG_DFL);
				signal(SIGINT, SIG_DFL);

			}

		}

		printf("%%");

	}

}