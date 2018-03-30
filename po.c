#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <string.h>
char * users[20];
int i, user_num;
const char * sch_alg[10] = {"fcfs", "pr", "sjf"};
const int scheduler_num = 3;


/*
p2s: pipe from parent to scheduler process.
s2o: pipe from scheduler to output module.
p2o: pipe from parent to output module.
*/
int p2s_fd[scheduler_num][2];
int s2o_fd[scheduler_num][2];
int p2o_fd[2];

void close_pipe(const int rfd, const int wfd) { // close a pair of file descriptors
    if (close(rfd) == -1 || close(wfd) == -1) {
        printf("couldn't close file");
        exit(EXIT_FAILURE);
    }
}

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
		cmd[0]: the command type, 0 for add class, 1 for add meeting, 2 for add gathering, 3 for add batch, 4 for print schedule, 5 for print report
		
		----- for command type 0, 1, 2 -------------
		cmd[1]: the caller, specified using the index of array users[].
		cmd[2]: the date, range from 0 - 13, corresponding to Apr 1 to Apr 14.
		cmd[3]: the start time, range from 0 - 10, corresponding to 08:00 to 18:00
		cmd[4]: the durationg time, range from 0 - 10.
		cmd[5]: the number of callees, may be empty when the command is addClass.
		cmd[6] - cmd[@return]: the calles, specified useing the index of array users[].

		----- for command type 3, 5 -----
		file: the name of the file to import commands

		----- for command type 4 -----
		cmd[1]: the user to print the schedule
		cmd[2]: the type of scheduling algorithm, specified using the index of array sch_alg[].
		file: the name of the file to be saved.
	
	@return
	return the length of cmd after transformation.
	return -1 if the command is invalid.
*/
int sep_line(char * line, int * cmd, char * file){
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

	// change the command type to integer
	if(strcmp(sep[0], "addClass") == 0) cmd[0] = 0;
	else if(strcmp(sep[0], "addMeeting") == 0) cmd[0] = 1;
	else if(strcmp(sep[0], "addGathering") == 0) cmd[0] = 2;
	else if(strcmp(sep[0], "addBatch") == 0) cmd[0] = 3;
	else if(strcmp(sep[0], "printSchd") == 0) cmd[0] = 4;
	else if(strcmp(sep[0], "printReport") == 0) cmd[0] = 5;
	else return -1; // invalid command specifier

	// add batch or print report.
	if(cmd[0] == 3 || cmd[0] == 5) {
		strcpy(file, sep[1]);
		return 1;
	}

	// change the caller to integer
	for(i = 0; i < user_num; i++){
		if(strcmp(users[i], format(sep[1])) == 0){ // compare the name with the user
			cmd[1] = i;
			break;
		}
	}
	if(i == user_num) return -1; // there are no compitable user name.

	// print schedule
	if(cmd[0] == 4){
		for(i = 0; i < scheduler_num; i++){
			if(strcmp(sep[2], sch_alg[i]) == 0){
				cmd[2] = i;
				break;
			}
		}
		if(i == scheduler_num) return -1;
		strcpy(file, sep[3]);
		return 3;
	}

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

void FCFS_scheduler(const int rp_pipe, const int wo_pipe) {
	int n, is_working = 1;
	char buf[100];
	int cmd[20], time[10][15][11];

	// time is the time schedule for each user, time[i][j][k] is the kth hour of the jth day of ith user.
	memset(time, 0, sizeof(time)); 
	
	while(is_working){
		if((n = read(rp_pipe, buf, 50)) < 0) {perror("Read Error 1"); exit(EXIT_FAILURE);}
		else if(n == 0) {printf("pipe from parent closed 1\n"); break;}
		printf("FCFS receive from parent: ");
		for(i = 0; i < n; i++){
			cmd[i] = buf[i];
			printf("%d ", cmd[i]);
		}
		printf("\n");
		if(buf[0] == '6') {
			printf("closed!\n");
			break;
		}
		int date = cmd[2], time = cmd[3];



	}

}

void PR_scheduler(const int rp_pipe, const int wo_pipe) {
	int n, is_working = 1;
	char buf[100];
	int cmd[20], time[10][15][11];
	memset(time, 0, sizeof(time));
	while(is_working){
		if((n = read(rp_pipe, buf, 50)) < 0) {perror("Read Error 2"); exit(EXIT_FAILURE);}
		else if(n == 0) {printf("pipe from parent closed 2\n"); break;}
		printf("PR receive from parent: ");
		for(i = 0; i < n; i++){
			cmd[i] = buf[i];
			printf("%d ", cmd[i]);
		}
		printf("\n");
		if(buf[0] == '6') {
			printf("closed!\n");
			break;
		}

		// detail implementation needed
	}

}

void SJF_scheduler(const int rp_pipe, const int wo_pipe) {
	int n, is_working = 1;
	char buf[100];
	int cmd[20], time[150];
	memset(time, 0, sizeof(time));
	while(is_working){
		if((n = read(rp_pipe, buf, 50)) < 0) {perror("Read Error 3"); exit(EXIT_FAILURE);}
		else if(n == 0) {printf("pipe from parent closed 3\n"); break;}
		printf("SJF receive from parent: ");
		for(i = 0; i < n; i++){
			cmd[i] = buf[i];
			printf("%d ", cmd[i]);
		}
		printf("\n");
		if(buf[0] == '6') {
			printf("closed!\n");
			break;
		}

		// detail implementation needed


	}

}

void output(const int * rs_pipe, const int rp_pipe){

}

void write_all(char * buf, int n){
	for(i = 0; i < scheduler_num; i++){
		if(write(p2s_fd[i][1], buf, n) < 0) perror("write failed 1");
	}
	if(write(p2o_fd[1], buf, n) < 0) perror("write failed 2");
}


int main(int argc, char *argv[]) {

	char line[110], file[100], buf[100];
	int cmd[15], n;
	
	//int 


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

	/* make pipes
		
	*/

	for(i = 0; i < scheduler_num; i++){
		if(pipe(p2s_fd[i]) < 0 || pipe(s2o_fd[i]) < 0)
			perror("pipe create failed");
	}
	if(pipe(p2o_fd) < 0) perror("pipe create failed");

	//fork child scheduler, child output output module.
	int pid;

	// the first FCFS scheduler
	pid = fork();
	if(pid == 0){
		close_pipe(p2s_fd[0][1], s2o_fd[0][0]);

		// the main scheduler function
		FCFS_scheduler(p2s_fd[0][0], s2o_fd[0][1]);
		return 0;
	}

	// the second PR scheduler
	pid = fork();
	if(pid == 0){
		close_pipe(p2s_fd[1][1], s2o_fd[1][0]);

		// the main scheduler function
		PR_scheduler(p2s_fd[1][0], s2o_fd[1][1]);
		return 0;
	}

	// the third SJF scheduler
	pid = fork();
	if(pid == 0){
		close_pipe(p2s_fd[2][1], s2o_fd[2][0]);

		// the main scheduler function
		SJF_scheduler(p2s_fd[2][0], s2o_fd[2][1]);
		return 0;
	}

	// fork the output module
	pid = fork();
	if(pid == 0){
		int rp[10];
		for(i = 0; i < scheduler_num; i++){
			if(close(s2o_fd[i][1]) == -1) perror("close failed 1");
			rp[i] = s2o_fd[i][0];
		}
		if(close(p2o_fd[1]) == -1) perror("close failed 2");

		// the main function
		output(rp, p2o_fd[0]);
		return 0;
	}


	// parent process
	// close the read pipe with schedulers.
	for(i = 0; i < scheduler_num; i++)
		if(close(p2s_fd[i][0]) == -1) perror("close failed 3");
	// close the read pipe with output module
	if(close(p2o_fd[0]) == -1) perror("close failed 4");
	

	// read from the command line.
	while(_getline(line, 100)){

		//printf("entered %s\n", line);
		if (strcmp(line, "endProgram") == 0) {
			printf("-> Bye!\n");
			write_all("6", 1);
			break;
		} else {
			if((n = sep_line(line, cmd, file)) == -1){
				printf("Invalid Command!\n");
				break;
			}

			for(i = 0; i < n; i++){
				printf("%d ", cmd[i]);
			}
			if(cmd[0] >= 3){
				printf("filename: %s", file);
			}
			printf("\n");

			if(cmd[0] < 3){
				for(i = 0; i < n; i++) buf[i] = cmd[i];
				write_all(buf, n);
			}

		}
		printf("Please enter ->\n");
	}


	// close all the pipes
	for(i = 0; i < scheduler_num; i++){
		close_pipe(p2s_fd[i][0], s2o_fd[i][1]);
	}
	close(p2o_fd[0]);
	while(wait(NULL) > 0);
	return 0;
}
