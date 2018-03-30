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
/*
	@para
	line: the command in a single line, need to be seperated;
	cmd: the command represented in integers, seperated.
		cmd[0]: the command type, 0 for add class, 1 for add meeting, 2 for add gathering, 3 for add batch
		cmd[1]: the caller, specified using the index of array users[].
		cmd[2]: the date, range from 0 - 13, corresponding to Apr 1 to Apr 14.
		cmd[3]: the start time, range from 0 - 10, corresponding to 08:00 to 18:00
		cmd[4]: the durationg time, range from 0 - 10.
		cmd[5]: the number of callees, may be empty when the command is addClass.
		cmd[6] - cmd[@return]: the calles, specified useing the index of array users[].
	
	@return
	return the length of cmd after transformation.
	return -1 if the command is invalid.
*/
int sep_line(char * line, int * cmd){
	char sep[10][20];
	memset(sep, 0, sizeof(sep));
	int cur = 0, p = 0;
	for(i = 0;; i++){
		char c = line[i];
		if(c == ' ' || c == 0){
			cur++;
			if(c == 0) break; // break the loop when read to the end of line
			p = 0;
		} else {
			sep[cur][p++] = c;
		}
	}
	if(cur < 5) return -1; 
	// change the command type to integer
	if(strcmp(sep[0], "addClass") == 0) cmd[0] = 0;
	else if(strcmp(sep[0], "addMeeting") == 0) cmd[0] = 1;
	else if(strcmp(sep[0], "addGathering") == 0) cmd[0] = 2;
	else if(strcmp(sep[0], "addBatch") == 0) cmd[0] = 3;
	else return -1; // invalid command specifier

	// change the caller to integer
	for(i = 0; i < user_num; i++){
		if(strcmp(users[i], format(sep[1])) == 0){ // compare the name with the user
			cmd[1] = i;
			break;
		}
	}
	if(i == user_num) return -1; // there are no compitable user name.

	// change the date to integer
	if(strncmp(sep[2], "201804", 6) != 0) return -1; // the date is not in 2018/04
	cmd[2] = atoi(sep[2] + 6) - 1;
	if(cmd[2] < 0 || cmd[2] > 13) return -1; // the date is not in Apr 1 to Apr 14

	// change the start time to integer
	if(!(sep[3][2] == '0' && sep[3][3] == '0')) return -1; // the start time is not in hours
	sep[3][2] = 0;
	cmd[3] = atoi(sep[3]) - 8;
	if(cmd[3] < 0 || cmd[3] > 10) return -1; // the start time is not in 8am to 6pm

	// change the duration time to integer
	cmd[4] = atoi(sep[4]);
	if(cmd[4] < 0 || cmd[4] > 10) return -1; // the duration is too long and over a day

	if(cur == 5) return 5; // there are only 5 sub-cmd, i.e. addClass. 

	// calculate the callee num
	cmd[5] = cur - 5;

	// change the callees name to integer using index.
	int j;
	for(i = 0; i < cmd[5]; i++){
		for(j = 0; j < user_num; j++){
			if(strcmp(users[j], format(sep[5 + i])) == 0){ // compare the name with the user
				cmd[6 + i] = j;
				break;
			}
		}
		if(j == user_num) return -1; // cannot find the corresponding name
	}

	return 6 + i;

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

		//printf("entered %s\n", line);
		if (strcmp(line, "endProgram") == 0) {
			printf("-> Bye!\n");
			break;
		} else {
			int len = sep_line(line, cmd);
			for(i = 0; i < len; i++){
				printf("%d ", cmd[i]);
			}
			printf("\n");
		}
		printf("Please enter ->\n");
	}

	return 0;
}
