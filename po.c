#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <string.h>
char * users[20];
int i, user_num;

char * format(char * s){
	if(s[0] >= 97 && s[0] <= 122){
		s[0] = s[0] - 32;
	}
	return s;
}

int _getline(char * s, int lim){
	int i; char c;
	i = 0;
	while((c = getchar()) != EOF && c != '\n' && i < lim - 1){
		s[i++] = c;
	}
	s[i] = 0;
	return i;
}

void sep_line(char * line, int * cmd){
	char sep[10][20];
	int cur = 0, p = 0;
	for(i = 0; line[i] != 0; i++){
		char c = line[i];
		if(c == ' '){
			cur++;
			p = 0;
		} else {
			sep[cur][p++] = c;
		}
	}
	// change the command type to integer
	if(strcmp(sep[0], "addClass") == 0) cmd[0] = 0;
	else if(strcmp(sep[0], "addMeeting") == 0) cmd[0] = 1;
	else if(strcmp(sep[0], "addGathering") == 0) cmd[0] = 2;
	else if(strcmp(sep[0], "addBatch") == 0) cmd[0] = 3;
	else {cmd[0] = -1; return;}

	// change the caller to integer
	for(i = 0; i < user_num; i++){
		if(strcmp(users[i] == format(sep[1])) == 0){ // compare the name with the user
			cmd[i] = i;
			break;
		}
	}
	if(i == -1) {cmd[0] = -1; return;}

	//

}

int main(int argc, char *argv[]) {

	char line[110];
	int cmd[15];

	// read the user num and user name
	user_num = argc - 1;
	if(user_num < 3 || user_num > 9){
		printf("Invalid User Number! (3 - 9)\n");
		exit(1);
	}
	for(i = 0; i < user_num; i++){
		users[i] = format(argv[i + 1]);
	}
	// print initial message
	printf("~~WELCOME TO PO~~\n");
	printf("Please enter ->\n");
	// make pipes and child scheduler, child output output module.


	//
	while(_getline(line, 100)){

		printf("entered %s\n", line);
		if (strcmp(line, "endProgram") == 0) {
			printf("-> Bye!\n");
			break;
		} else {
			sep_line(line, cmd);
		}
		printf("Please enter ->\n");
	}

	return 0;
}
