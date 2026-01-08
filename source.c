//gash - gregs atrocious/abhorrent/awful shell
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <wait.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

//TODO:
//overhaul the way argify works so we can use airquotes and backslash spaces
//
//lowpriority:
//bin path searching
//redirects
//bin hashing
//script
//

//TODO: make this count words with backslashing literals and quotes
uint32_t count_words(char* string){
	bool counting_command = false;
	char quote_mode = 0;
	uint32_t word_count = 0;
	for(int i = 0; string[i] != 0; ++i){
		if(quote_mode){
			if(string[i] == quote_mode){
				quote_mode = 0;
			}		
		} else {
			if(string[i] == '\"' || string[i] == '\''){
				quote_mode = string[i];
			}
			if(!counting_command && isgraph(string[i])){
				counting_command = true;
				++word_count;
			} else if (counting_command && isspace(string[i])) {
				counting_command = false;
			}
		}
	}
	return word_count;
}

struct argified{
	char** args;
	uint32_t count;
};

void free_argified(struct argified freeobj){
	if(freeobj.args != NULL){
		for(int i = 0; i < freeobj.count; ++i){
			free(freeobj.args[i]);
		}
		free(freeobj.args);
	}
}

//TODO: make this count words with backslashing literals and quotes
struct argified argify(char* command){

	//count arguments
	uint32_t arg_count = count_words(command);

	if(arg_count == 0){
		struct argified ret = {
			NULL,
			arg_count,
		};
		return ret;
	}

	//find where arguments sit
	uint32_t* arg_sizes = calloc(arg_count, sizeof(uint32_t));
	uint32_t* arg_pos = calloc(arg_count, sizeof(uint32_t));
	bool counting_command = false;
	char quote_mode = 0;
	int arg_num = 0;
	for(int i = 0; command[i] != 0; ++i){
		if(quote_mode){
			if(command[i] == quote_mode){
				quote_mode = 0;
			} else {
				++arg_sizes[arg_num];
			}
		} else {
			if(command[i] == '\"' || command[i] == '\''){
				quote_mode = command[i];
				i++;
			}
			if(!counting_command && isgraph(command[i])){
				counting_command = true;
				arg_pos[arg_num] = i;
				++arg_sizes[arg_num];
			} else if (counting_command && isspace(command[i])) {
				counting_command = false;
				++arg_num;
			} else if (counting_command) {
				++arg_sizes[arg_num];
			}
		}
	}

	//create argument array
	char** arglist = calloc(arg_count + 1, sizeof(char*));
	for(int i = 0; i < arg_count; ++i){
		arglist[i] = calloc(arg_sizes[i], sizeof(char));
		for(int j = 0; j < arg_sizes[i]; ++j){
			arglist[i][j] = command[(arg_pos[i]) + j];
		}
	}
	arglist[arg_count] = NULL;

	//clear intermediates
	free(arg_sizes);
	free(arg_pos);

	struct argified ret = {
		arglist,
		arg_count,
	};

	return ret;
}



int main(int argc, char** argv){
	bool exiting = false;
	char* input = malloc(512 * sizeof(char));
	while(!exiting){
		memset(input, 0, 512);
		printf("gash >> ", getpid());
		fgets(input, 512, stdin);

		//cram everything into a function which just makes a neat bunch of args
		struct argified arglist = argify(input);

		for(int i = 0; i < arglist.count; i++){
			printf("%d: %s\n", i, arglist.args[i]);
		}

		//fork and execv
		if(arglist.count == 1 && !strcmp(arglist.args[0], "exit")){
			exiting = true;
		} else if (arglist.count > 0) {
			int pid = fork();
			if (pid == 0){
				execv(arglist.args[0], arglist.args);
				exit(0);
			} else {
				int status;
				waitpid(pid, &status, 0);
			}
		}

		free_argified(arglist);
	}
	free(input);
	return 0;
}
