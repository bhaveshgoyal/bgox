#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#define COMM_LEN  100
#define BIN_LEN  10
#define ARG_LEN  20
#define ENV_LEN 1000
#define PATH_LEN 1000
#define PS1_DEFAULT "sbush> "
void sig_handler(int signal){
		printf("\n+ done\n");

}
void str_cpy(char *to_str, char *frm_str){
		int i=0;
		for(i=0;frm_str[i] != '\0'; i++){
				to_str[i] = frm_str[i];
		}
		to_str[i] = '\0';
}
int str_len(char *str){
		int i=0;
		for(i=0;str[i] != '\0';i++){
				// spin
		}
		return i;
}
int str_cmp(char* str1, char* str2){

		return (*str1 == *str2 && *str1 == '\0') ? 1 : (*str1 == *str2 ? str_cmp(++str1, ++str2) : -1);

}
int strfind_occurence(char *str, char query, int occr){
		int find_ctr = 0;
		int i=0;
		for(i=0; str[i] != '\0'; i++){
				if (str[i] == query)
						find_ctr++;
				if (find_ctr == occr)
						break;
		}
		return i; // Returns found index or else null index;
}
int str_contains(char *str, char *query){
		int j=0, i=0;
		int startIdx = -1, found = 0;
		for(i=0;str[i] != '\0';i++){
				if (str[i] == query[j]){
						j++;
						startIdx = i;
						if (j == str_len(query)){
								found = 1;
								break;
						}
				}
				else if (startIdx >= 0){
						j = 0;
						startIdx = -1;
				}
		}

		return (found == 1) ?  startIdx - str_len(query) + 1: -1; // Start Index of query substring
}
int strfind_delim(char *str, int frm){
		int i=0;
		for(i=frm+1; str[i] != '\0'; i++){
				if (str[i] == ' ' || str[i] == '\t' || str[i] == '\n')
						break;
		}
		return str[i] == '\0' ? i-1 : i;
}
void str_substr(char *str, int from, int to, char *out_str){
		int index = 0;
		for(int i = from; i <= to; i++){
				out_str[index] = str[i];
				index++;
		}
		out_str[index] = '\0';
}

int split(char *str, char out[][COMM_LEN]){
		int prev_ptr = 0;
		int i=0;
		int arg_ctr = 0;
		for(i=0; str[i] != '\0'; i++){
				if (str[i] == '\t' || str[i] == ' '){
						str_substr(str, prev_ptr, i-1, out[arg_ctr++]);
						prev_ptr = i+1;
				}
		}
		str_substr(str, prev_ptr, i-1, out[arg_ctr++]);
		return arg_ctr;
}

void str_concat(char *prev, char *current, char *after, char *dest){
		int i=0;
		int j=0;
		int k=0;
		for(i = 0;prev[i] != '\0';i++){
				dest[i] = prev[i];
		}
		dest[i] = '\0';
		for(j = 0;current[j] != '\0';j++){
				dest[j + i] = current[j];
		}
		dest[i+j] = '\0';
		for(k = 0;after[k] != '\0';k++){
				dest[j + i + k] = after[k];
		}
		dest[i+j+k] = '\0';
}

int comm_parser(char **comm, char *argv[COMM_LEN]){

		char args[ARG_LEN][COMM_LEN];
		char *comms = *comm;
		int arg_ctr = split(comms, args);
		*comm = args[0];
		for(int i=0; i < arg_ctr; i++){
				argv[i] = args[i] + '\0';
		}
		return arg_ctr;
}
int exec_custom(char command[], char *args[], int arg_ctr){

		if (str_cmp(args[0], "cd") > 0){
				if (chdir(args[1]) < 0)
						puts("sbush: error executing command: cd");
				return 1;
		}
		else if (str_cmp(args[0], "export") > 0){ //TODO Make export persistant by wrinting to file 
				//TODO Check valid string enclosed within ""
				int delim_idx = strfind_occurence(command, '=', 1);
				char env_var[BIN_LEN];
				str_substr(command, str_len(args[0])+1, delim_idx-1, env_var);
				char env_val[ENV_LEN];
				str_substr(command, delim_idx+2, str_len(command)-2, env_val); //Null and New line were two extra chars
				if (str_cmp(env_var, "PS1") > 0){
						setenv("PS1", env_val, 1);
				}
				else if (str_cmp(env_var, "PATH") > 0){
						int idx = str_contains(env_val, "$PATH");
						if (idx < 0)
								setenv("PATH", env_val, 1);
						char temp[ENV_LEN];
						str_cpy(temp, env_val);
						char *current = getenv("PATH");
						while(idx >= 0){
								char prev[PATH_LEN];
								str_substr(temp, 0, idx-1, prev);
								char after[PATH_LEN];
								str_substr(temp, idx+5, str_len(temp)-1, after);
								str_concat(prev, current, after, temp);
								setenv("PATH", temp, 1);
								idx = str_contains(temp, "$PATH");
						}
				}
				return 1;
		}
		return 0;
}
int main(int argc, char *argv[], char *envp[]) {

		int pid, status;
		char command[COMM_LEN] = {'\0'};


		if (argc > 1){
				char *const fileName = argv[1];
				FILE* file = fopen(fileName, "r"); 
				if (!fgets(command, COMM_LEN, file)){
						puts("sbush: error executing script");
				}
				while (fgets(command, COMM_LEN, file)){
						command[str_len(command)-1] = '\0';
						char *args[COMM_LEN];
						char *comm = command;
						int arg_ctr = comm_parser(&comm, args);
						args[arg_ctr] = NULL;
						int BACKGROUND = str_cmp(args[arg_ctr-1], "&");
						if (BACKGROUND > 0){
								args[arg_ctr - 1] = NULL;
								arg_ctr--;
						} 
						if (exec_custom(command, args, arg_ctr) == 0){
								pid = fork();
								if (pid != 0){
										if (BACKGROUND > 0){
												signal(SIGCHLD, SIG_IGN);
												waitpid(pid, &status, WNOHANG);
										}
										else
												waitpid(pid, &status, WUNTRACED);
								}
								else{
										execvp(args[0], args);
										if(WIFEXITED(status)){
												fputs("sbush: command not found: ", stdout);
												puts(args[0]);
										}
								}

						}
				}
				fclose(file); 	
		}
		else{
				while(1){
						(getenv("PS1") == NULL) ? setenv("PS1", PS1_DEFAULT, 1) : setenv("PS1", getenv("PS1"), 1);
						char *PS1 = getenv("PS1");
						fputs(PS1, stdout);
						if (fgets(command, COMM_LEN, stdin) != NULL){
								command[str_len(command)-1] = '\0';
								char *args[COMM_LEN];
								char *comm = command;
								int arg_ctr = comm_parser(&comm, args);
								args[arg_ctr] = NULL;
								int BACKGROUND = str_cmp(args[arg_ctr-1], "&");
								if (BACKGROUND > 0){
										args[arg_ctr - 1] = NULL;
										arg_ctr--;
								}
								if (exec_custom(command, args, arg_ctr) == 0){
										pid = fork();
										if (pid != 0){
												if (BACKGROUND > 0){
														signal(SIGCHLD, SIG_IGN);
														waitpid(pid, &status, WNOHANG);
												}
												else
														waitpid(pid, &status, WUNTRACED);
										}
										else{
												execvp(args[0], args);
												if(WIFEXITED(status)){
														fputs("sbush: command not found: ", stdout);
														puts(args[0]);
												}
										}
								}
						}
						else{
								puts("Error reading input from stdin");
						}

				}

		} 
		return 0;
}
