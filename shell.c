#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>

/*
  Author(s):  Andrew Foerst, Ethan Kesterholt 
  Course:  COMP 340, Operating Systems
  Date:    23 February 2024
  Description:   This file implements the
                 Shell program
  Compile with:  gcc -o shell shell.c
  Run with:      ./shell

*/

int shell_change_dir(char *dir_path) {
	if(chdir(dir_path) == 0) return 0;

	return -1;
}


int shell_file_exists(char *file_path) {
    struct stat buf;
    return stat(file_path, &buf);
}


int shell_find_file(char *file_name, char *file_path, int file_path_size) {
	// Saving the actual current working directory for later use. 
	char pwd[1024];
	getcwd(pwd,sizeof(pwd));

	//Getting PATH Environment variable 
	char *path = getenv("PATH");
	char *copy = (char*) malloc(strlen(path)+1);
	
	//Create a copy of path so that we can tokenize 
	strcpy(copy, path);

	char* token = strtok(copy, ":");
	
	while(token != NULL){
		shell_change_dir(token);
		
		if(shell_file_exists(file_name) == 0) break; 

		token = strtok(NULL, ":");
	}

	free(copy);

	if(token == NULL){
		shell_change_dir(pwd);	
		return -1;
	} 
	
	// Adding the full path to the variable 
	strcat(token, "/");
	strcat(token, file_name);

	strncpy(file_path, token, strlen(token)); 
	file_path[strlen(token)] = '\0';
	
	// Go back to original present working directory 
	shell_change_dir(pwd);	
	
	return 0; 
}


int shell_execute(char *file_path, char **argv) {
    pid_t pid = fork();

    if(pid == 0){ // Child 
		execv(file_path,argv);

		printf("Command not found!\n");
		exit(EXIT_FAILURE);
    }else{ // Parent
		wait(NULL);
        return 1; 
    }
  // execute the file with the command line arguments
  // use the fork() and exec() system call 
}

char* remove_whitespace(char *input){
	while(*input == ' ' || *input == '\n') input++; // Remove all whitespace from the front 

	int end = strlen(input);

   	while(end >0 && input[end-1] == ' ' || input[end-1] == '\n') end--; // Remove all whitespace from the end 

	char* returnable = (char*)malloc(end+1);
	strncpy(returnable, input, end);
	returnable[end]='\0';

   	return returnable;
}

int indexOfSpace(char *str){
	size_t len = strlen(str);

	for(int i = 0; i<len; i++){
		if(str[i]== ' ') return i;
	}

	return len;
}

char* substring(char* str, int start, int end){
	char * returnable = (char*) malloc((end-start)+1);
	int index = 0; // Index for iterating through returnable 

	// Copy all data over 
	for(int i = start; i<end; i++){
		returnable[index]=str[i];
		index++;
	}

	// Adding NULL character
	returnable[index] = '\0';
	return returnable;
}

int countSpaces(char * command){
	int numSpaces = 0;
	
	// Counts the number of spaces in a command.
	for(int i = 0; i<strlen(command); i++){
		if(command[i-1] == ' ' && command[i]!=' ') numSpaces++;
	}

	return numSpaces;
}

int collect_args(char **args, char *command){
	int size = 0;
	int numString = 0;
	int index = 0;

	for(int i =0; i<strlen(command);i++){
		if(size == 0 && command[i] == ' '){ // Get's rid of previous any extra whitespace
			index = i+1;
			continue;
		}

		// If it is not whitespace, increase length of string to be copied 
		if(command[i] != ' ' && command[i] != '\n' && command[i] != '\0') size++; 

		else{ // Otherwise, it is the end of the word and we should copy it over. 
			args[numString] = substring(command, index, index+size);
			index=i+1;
			numString++;
			size=0;
		}
	}
	// Once it hits the end, copy over the remaining word. 
	args[numString] = substring(command, index, index+size);
	numString++; // Increment the number of strings to return
	args[numString] = NULL; // Set the final spot of args to NULL so that execv can handle it properly. 
	return numString;
}

int main (int argc, char *argv[]) {
	// Getting hostname
   char hostname[HOST_NAME_MAX];
   gethostname(hostname, sizeof(hostname));
   
   // Getting username
   char *username = getenv("USER");

   while(1){
	// Getting pwd to display to user. 
    char pwd[1024]; 
    getcwd(pwd, sizeof(pwd));

	// Prompting user
 	printf("%s@%s:%s $ ", username, hostname, pwd);


	// Getting user input 
    char *input = (char*)malloc(100);
   	fgets(input, 99, stdin);

	if(strlen(input)==1){ // If the user puts nothing in (i.e. "\n").
		free(input);
		continue;
	} 

   	// Call to remove whitespace
   	char* command = remove_whitespace(input);

	// If the user wants to exit.
    if(strcmp(command, "exit") == 0) break;

	// Getting the command (i.e. "cd, ls, gcc, pwd, ...")
    char* firstWord = substring(command, 0, indexOfSpace(command));

	if(strcmp(firstWord, "cd") == 0){ // If the command is cd 
		char* secondWord = substring(command, indexOfSpace(command)+1, strlen(command));
		
		if(shell_change_dir(secondWord) != 0) printf("No such file or directory\n");

		free(secondWord);
	}else{
		int length = countSpaces(command)+1;
		char *args[countSpaces(command)+2];

		// If the user prompts to see if the directory exists 
		if(firstWord[0] == '/' && firstWord[strlen(firstWord)-1] == '/'){
			if(shell_file_exists(firstWord) == 0) printf("Is a directory\n");
			else printf("Is not a directory\n");
		}
		else if(firstWord[0] == '/'){ // If the user gives an absolute path
			// Collect all arguments
			collect_args(args, command); 
			
			// Run the command 
			shell_execute(firstWord, args);	

			// Free up all allocated space on the heap within collect_args 
			for(int i = 0; i<length; i++) free(args[i]);
		}else if(firstWord[0] == '.'){
			// Defining lengths for operation 
			int pwd_length = strlen(pwd);
			int command_length = strlen(command);
			int total_length = pwd_length + command_length - 1;
			
			// Defining lengths for new command
			char new_command[total_length];

			// Copying over present working directory 
			for(int i = 0; i<pwd_length; i++) new_command[i] = pwd[i];
			
			// Copying over length of the first word
			for(int i = 1; i < command_length; i++) new_command[pwd_length+(i-1)] = command[i];

			new_command[total_length] = '\0';
			// Parse the string and get all arguments
			int length = collect_args(args, new_command);
			
			// Run the command 
			shell_execute(args[0], args);

			// Free up all allocated space on the heap. 
			for(int i = 0; i<length; i++) free(args[i]);
		}else{
			// Making a temp file to get a tempory path
			char temp[1024];

			// If command is not found on the path 
			if(shell_find_file(firstWord, temp, 0) == -1){ // Length is unneeded in implementation 
				printf("command not found\n");
				
				// Free all dynamically allocated memory and prompt the user again 
				free(firstWord);
				free(input);
				free(command);
				continue; 
			} 

			// Copying over the entire file_path of temp
			char file_path[strlen(temp)];
   			for(int i =0; i < strlen(temp); i++){
				file_path[i] = temp[i];
			}
			file_path[strlen(temp)] = '\0';

			// Parse the text and return the number of ars
			collect_args(args, command);

			//Replace the command with the absolute path to the exe of the command 
			args[0] = (char*)malloc(strlen(file_path)+1);
			strcpy(args[0], file_path);
			args[0][strlen(file_path)] = '\0';

			// Run the command 
			shell_execute(file_path, args);

			// Free up all allocated space on the heap. 
			for(int i = 0; i<length; i++) free(args[i]);
		}

		
	}
	// Free all dynamically allocated memory. 
	free(firstWord);
	free(input);
	free(command);
	}
}
