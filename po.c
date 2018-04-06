#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <string.h>
#define scheduler_num 4

typedef struct TIME {
	int occupied;
	int event;
}Time;

typedef struct EVENT {
	int id;
	int date, time, dur, type, valid;
	int users[10], num;
}Event;

Time t[10][15][11] = {0};
Event e[20000] = {0};



char * users[20];
int i, j, user_num, e_num = 0;
const char * sch_alg[10] = {"fcfs", "pr", "sjf", "re"};
const char * cmd_name[10] = {"Class", "Meeting", "Gathering"};
const char * sch_name[10] = {"First come first served", "Priortiy", "Shortest job first", "Reschedule"};

/*
p2s: pipe from parent to scheduler process.
s2o: pipe from scheduler to output module.
p2o: pipe from parent to output module.
*/
int p2s_fd[scheduler_num][2];
int s2o_fd[scheduler_num][2];
int p2o_fd[2];

void add_event(const int * cmd){
	int date = cmd[2], time = cmd[3], dur = cmd[4], type = cmd[0];
	e[e_num].date = date; e[e_num].time = time;
	e[e_num].dur = dur; e[e_num].type = type;
	e[e_num].users[e[e_num].num++] = cmd[1];
	if(cmd[0]){
		for(i = 6; i < 6 + cmd[5]; i++){ //cmd[i] be one of the callee
			e[e_num].users[e[e_num].num++] = cmd[i];
		}
	}
	e[e_num].valid = 0;
	e[e_num].id = e_num;
	e_num++;
}




void close_pipe(const int rfd, const int wfd) { // close a pair of file descriptors
    if (close(rfd) == -1 || close(wfd) == -1) {
        perror("couldn't close file\n");
        exit(EXIT_FAILURE);
    }
}

int cmp_time(const void * a, const void * b){
	Event * ea = (Event *) a;
	Event * eb = (Event *) b;
	if(ea -> date == eb -> date){
		if(ea -> time == eb -> time){
			return (ea -> dur) - (eb -> dur);
		}
		return (ea -> time) - (eb -> time);
	}
	return (ea -> date) - (eb -> date);
}

int cmp_dur(const void * a, const void * b){
	Event * ea = (Event *) a;
	Event * eb = (Event *) b;
	return (ea -> dur) - (eb -> dur);
}

int cmp_priority(const void * a, const void * b){
	Event * ea = (Event *) a;
	Event * eb = (Event *) b;
    return (ea -> type) - (eb -> type);
}
int cmp_short(const void * a, const void * b){
	Event * ea = (Event *) a;
	Event * eb = (Event *) b;
	if(ea -> date == eb -> date){
            if ( ea -> dur == eb -> dur){
                return (ea -> id) - (eb -> id);
            }
			return (ea -> dur) - (eb -> dur);
	}
	return (ea -> date) - (eb -> date);
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
		----- for command 1, 2 ---------------------
		cmd[5]: the number of callees, may be empty when the command is addClass.
		cmd[6] - cmd[@return]: the calles, specified useing the index of array users[].

		----- for command type 3, 5 ----------------
		file: the name of the file to import commands

		----- for command type 4 -------------------
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
		if(c == ' ' || c == 0 || c == '\n' || c == '\r'){
			cur++;
			if(c == 0 || c == '\n' || c == '\r') break; // break the loop when read to the end of line
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



int gen_msg(char * msg){ // each part has 4 bytes: id(2 bytes), date, time
	int msg_p = 0;
	//printf("e_num: %d \n", e_num);
	for(i = 0; i < e_num; i++){
		if(e[i].valid){
			int id = e[i].id;
			msg[msg_p++] = id / 256;
			msg[msg_p++] = id % 256;
			msg[msg_p++] = e[i].date;
			msg[msg_p++] = e[i].time;
			//printf(">>>%d %d %d\n", id, e[i].date, e[i].time);
		}
	}
	//printf("msg_p: %d\n", msg_p);
	return msg_p;
}

void FCFS_scheduler(const int rp_pipe, const int wo_pipe) {
	int n, is_working = 1;
	char buf[100];
	int cmd[20];

	// time is the time schedule for each user, time[i][j][k] is the kth hour of the jth day of ith user.
	
	while(is_working){
		if((n = read(rp_pipe, buf, 50)) < 0) {perror("Read Error 1"); exit(EXIT_FAILURE);}
		else if(n == 0) {printf("pipe from parent closed 1\n"); break;}
		//printf("FCFS receive from parent: ");
		for(i = 0; i < n; i++){
			cmd[i] = buf[i];
			//printf("%d ", cmd[i]);
		}
		//printf("\n");
		if(buf[0] == '6') {
			//printf("closed!\n");
			break;
		}
		if(cmd[0] < 3){
			// if the comand is an add event.
			int date = cmd[2], time = cmd[3], dur = cmd[4];
			int cur_num = e_num;
			add_event(cmd);

			int is_empty = 1;
			// check if the time slot is avaliable
			// for the caller
			for(i = time; i < time + dur; i++){
				if(t[cmd[1]][date][i].occupied){
					is_empty = 0;
					break;
				}
			}
			// for the callee
			if(cmd[0] != 0){
				for(i = 6; i < 6 + cmd[5]; i++){ //cmd[i] be one of the callee
					for(j = time; j < time + dur; j++){
						if(t[cmd[i]][date][j].occupied){
							is_empty = 0;
							break;
						}
					}
				}
			}
			if(cmd[3] + cmd[4] > 10){
				is_empty = 0;
			}
			if(is_empty){
				
				e[cur_num].valid = 1;
				// fill the time slot.
				// for the caller
				for(i = time; i < time + dur; i++){
					t[cmd[1]][date][i].occupied = 1;
					t[cmd[1]][date][i].event = cur_num;
				}
				// for the callee
				if(cmd[0] != 0){
					for(i = 6; i < 6 + cmd[5]; i++){ //cmd[i] be one of the callee
						for(j = time; j < time + dur; j++){
							t[cmd[i]][date][j].occupied = 1;
							t[cmd[i]][date][i].event = cur_num;
						}
					}
				}
				//printf("FCFS: valid event!\n");
			} else {
				//printf("FCFS: invalid event!\n");
			}
		} 
		// send messages to the output module. the message are formed by event numbers, using 256-based representation, each 2 bytes formed a number.
		// i.e. a two-byte number 'ab' indicates that a event with number 97 * 256 + 98 = 24930, the max event number could represent is 65535.
		// the total string would be look like 010204ab. which means there are 4 valid event, their number are 1, 2, 4, 24930. 
		else if(cmd[0] == 4 || cmd[0] == 5) {
			if(cmd[0] == 4 && cmd[2] != 0) continue;
			
			//printf("writing something\n");
			char msg[400];
			int n = gen_msg(msg);
			//printf("%d\n", n);
			if(write(wo_pipe, msg, n) < 0) perror("write failed 3");
			//printf("write finished\n");		
		}

	}

}

void PR_scheduler(const int rp_pipe, const int wo_pipe) {
	int n, is_working = 1;
	char buf[100];
	int cmd[20], time[150];
	memset(time, 0, sizeof(time));
	while(is_working){
		if((n = read(rp_pipe, buf, 50)) < 0) {perror("Read Error 3"); exit(EXIT_FAILURE);}
		else if(n == 0) {printf("pipe from parent closed 3\n"); break;}
		//printf("PR receive from parent: ");
		for(i = 0; i < n; i++){
			cmd[i] = buf[i];
			//printf("%d ", cmd[i]);
		}
		//printf("\n");
		if(buf[0] == '6') {
			//printf("closed!\n");
			break;
		}
		if (cmd[0] < 3){
			add_event(cmd);
		}
        else if(cmd[0] == 4 || cmd[0] == 5) {
        	if(cmd[0] == 4 && cmd[2] != 1) continue;
            qsort(e, e_num, sizeof(Event), cmp_priority);//to sort all events by 1.time length 2. arrival order 3. day
            int y;
        	//iterate over all events to determine validation
            for (y = 0; y < e_num; y++){
                //int cur_num = y;
                int ori_num=e[y].id;
                //current number: the current order
                int i, j;
                int date = e[y].date, time = e[y].time, dur = e[y].dur;
                int is_empty = 1;
                // check if the time slot is avaliable
                // for the caller
                for(i = 0; i < e[y].num; i++){
                	for(j = time; j < time + dur; j++){
                        if(t[e[y].users[i]][date][j].occupied){
                            is_empty = 0;
                            break;
                        }
                    }
                }
                if(time + dur > 10){
					is_empty = 0;
				}

                if(is_empty){
                    e[y].valid = 1;

                    for(i = 0; i < e[y].num; i++){
	                	for(j = time; j < time + dur; j++){
	                        t[e[y].users[i]][date][j].occupied = 1;
	                        t[e[y].users[i]][date][j].event = ori_num;
	                    }
	                }

                    //printf("PR: valid event! %d %d %d %d\n", ori_num,e[y].type, date, time);
                } else {
                   // printf("PR: invalid event! %d %d %d %d\n", ori_num,e[y].type, date, time);
                }
            }
			//printf("writing something\n");
			char msg[400];
			int n = gen_msg(msg);
			if(write(wo_pipe, msg, n) < 0) perror("write failed 3");
		}
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
		//printf("SJF receive from parent: ");
		for(i = 0; i < n; i++){
			cmd[i] = buf[i];
			//printf("%d ", cmd[i]);
		}
		//printf("\n");
		if(buf[0] == '6') {
			//printf("closed!\n");
			break;
		}
		if (cmd[0]<3){
			add_event(cmd);
		}
        else if(cmd[0] == 4 || cmd[0] == 5) {
        	if(cmd[0] == 4 && cmd[2] != 2) continue;
            qsort(e, e_num, sizeof(Event), cmp_short);//to sort all events by 1.time length 2. arrival order 3. day
            int y;
        //iterate over all events to determine validation
            for (y = 0; y < e_num; y++){
                //int cur_num = y;
                int ori_num = e[y].id;
                //current number: the current order
                int i, j;
                int date = e[y].date, time = e[y].time, dur = e[y].dur;
                int is_empty = 1;
                // check if the time slot is avaliable
                // for the caller
                for(i = 0; i < e[y].num; i++){
                	for(j = time; j < time + dur; j++){
                        if(t[e[y].users[i]][date][j].occupied){
                            is_empty = 0;
                            break;
                        }
                    }
                }
                if(time + dur > 10){
					is_empty = 0;
				}

                if(is_empty){
                    e[y].valid = 1;

                    for(i = 0; i < e[y].num; i++){
	                	for(j = time; j < time + dur; j++){
	                        t[e[y].users[i]][date][j].occupied = 1;
	                        t[e[y].users[i]][date][j].event = ori_num;
	                    }
	                }
                    //printf("SJF: valid event!\n");
                } else {
                    //printf("SJF: invalid event!\n");
                }
            }
			//printf("writing something\n");
			char msg[400];
			//printf("sjf st\n");
			int n = gen_msg(msg);
			//printf("sjf ed\n");
			if(write(wo_pipe, msg, n) < 0) perror("write failed 3");
		}
	}

}


void re_scheduler(const int rp_pipe, const int wo_pipe){
	int n, is_working = 1;
	char buf[100];
	int cmd[20];

	// time is the time schedule for each user, time[i][j][k] is the kth hour of the jth day of ith user.
	
	while(is_working){
		if((n = read(rp_pipe, buf, 50)) < 0) {perror("Read Error 4"); exit(EXIT_FAILURE);}
		else if(n == 0) {printf("pipe from parent closed 4\n"); break;}
		//printf("RE receive from parent: ");
		for(i = 0; i < n; i++){
			cmd[i] = buf[i];
			//printf("%d ", cmd[i]);
		}
		//printf("\n");
		if(buf[0] == '6') {
			//printf("closed!\n");
			break;
		}
		if(cmd[0] < 3){
			// if the comand is an add event.
			int date = cmd[2], time = cmd[3], dur = cmd[4];
			int cur_num = e_num;
			add_event(cmd);

			int is_empty = 1;
			// check if the time slot is avaliable
			// for the caller, and only judge class currently
			if(cmd[0] == 0){
				for(i = time; i < time + dur; i++){
					if(t[cmd[1]][date][i].occupied){
						is_empty = 0;
						break;
					}
				}
				if(is_empty){
					e[cur_num].valid = 1;
					// fill the time slot.
					// for the caller
					for(i = time; i < time + dur; i++){
						t[cmd[1]][date][i].occupied = 1;
						t[cmd[1]][date][i].event = cur_num;
					}
					//printf("RE: valid class!\n");
				} else {
					//printf("RE: invalid class!\n");
				}
			}
		} 
		// send messages to the output module. the event ids are using 256-based representation, each 2 bytes formed a id.
		// i.e. a two-byte number 'ab' indicates that a event with number 97 * 256 + 98 = 24930, the max event number could represent is 65535.
		// the total string would be look like 010204ab. which means there are 4 valid event, their number are 1, 2, 4, 24930. 
		else if(cmd[0] == 4 || cmd[0] == 5) {
			if(cmd[0] == 4 && cmd[2] != 3) continue;
			qsort(e, e_num, sizeof(Event), cmp_dur); // sort by duration time.
			int p;
			for(p = 0; p < e_num; p++){
				if(e[p].type == 0) continue;
				int has_found = 0;
				int date, time, user;
				for(date = 0; date < 14; date++){
					int dur = 0;
					for(time = 0; time < 10; time++){
						int valid = 1;
						for(user = 0; user < e[p].num; user++){
							int u = e[p].users[user];
							if(t[u][date][time].occupied){
								valid = 0;
								break;
							}
						}
						if(valid){
							dur++;
							if(dur == e[p].dur){ // the time slot is avaliable
								for(user = 0; user < e[p].num; user++){
									int u = e[p].users[user];
									for(j = time - dur + 1; j <= time; j++){
										t[u][date][j].occupied = 1;
										t[u][date][j].event = e[p].id;
									}
								}
								//printf(">>>>>%d %d |- %d. %d %d %d %d\n", date, time, e[p].type, e[p].id, e[p].date, e[p].time, e[p].dur);
								has_found = 1;
								e[p].valid = 1;
								e[p].date = date;
								e[p].time = time - dur + 1;
								break;
							}
						} else {
							dur = 0;
						}
					}
					if(has_found) break;
				}
			}


			
			//printf("writing something re\n");
			char msg[800];
			int n = gen_msg(msg);
			//printf("%d msg: %c%c%c%c\n", n, msg[0], msg[1], msg[2], msg[3]);
			if(write(wo_pipe, msg, n) < 0) perror("write failed 3");
			//printf("write finished\n");			
		}

	}

}

/*
	@para
	
	@return
	return 0 if there are no corresponding user.
	else return the ulist length 

*/
int has_user(Event e, int u){
	int i;
	for(i = 0; i < e.num; i++){
		if(e.users[i] == u)
			return 1;
	}
	return 0;
}

void output(const int * rs_pipe, const int rp_pipe){
	int n, is_working = 1;
	char buf[500];
	int cmd[20];
	char * file;
	FILE *fp;
	// time is the time schedule for each user, time[i][j][k] is the kth hour of the jth day of ith user.
	
	while(is_working){
		if((n = read(rp_pipe, buf, 500)) < 0) {perror("Read Error 4"); exit(EXIT_FAILURE);}
		else if(n == 0) {printf("pipe from parent closed 1\n"); break;}
		//printf("OutPut receive from parent: ");
		if(buf[0] == '6') {
			//printf("closed!\n");
			break;
		}
		n = buf[0] >= 4 ? (buf[0] == 4 ? 3 : 1) : n;
		
		for(i = 0; i < n; i++){
			cmd[i] = buf[i];
			//printf("%d ", cmd[i]);
		}
		//printf("\n");
		if(buf[0] >= 4){
			file = buf + 5;
			fp = fopen(file, "w");
		}
		
		if(cmd[0] < 3){
			// if the comand is an add event.
			int date = cmd[2], time = cmd[3], dur = cmd[4];
			add_event(cmd);
			int is_empty = 1;
		} else if(cmd[0] == 4){ // print schedule
			int x;
			for(x = 0; x < e_num; x++){
				e[x].valid = 0;
			}
			int sch = cmd[2], user = cmd[1];
			buf[0] = 0;
			if((n = read(rs_pipe[sch], buf, 500)) < 0) {perror("Read Error from sch 1"); exit(EXIT_FAILURE);}
			else if(n == 0) {printf("pipe from schedule closed 1\n"); break;}
			//printf("OutPut receive from schdule %s: \n", sch_alg[sch]);
			//printf("\n\n", n);
			//printf("User: %s\n", users[user]);
			Event valid[2000];
			int vnum = 0;
			int ts = 0;
			for(i = 0; i < n - 1; i += 4){
				int eid = (buf[i] * 256) + buf[i + 1];
				int date = buf[i + 2];
				int time = buf[i + 3];
				if(has_user(e[eid], user)){
					valid[vnum] = e[eid];
					valid[vnum].date = date; 
					valid[vnum++].time = time; 
					ts += e[eid].dur;
					e[eid].valid = 1;
				}
			}

			qsort(valid, vnum, sizeof(Event), cmp_time);

			fprintf(fp, "Personal Organizer\n***Appointment Schedule***\n");
			fprintf(fp, "%s, you have %d appointments\n", users[user], vnum);
			fprintf(fp, "Algorithm used: %s\n", sch_name[sch]);
			fprintf(fp, "%d timeslots occupied.\n\n", ts);
			fprintf(fp, "%-13s%-8s%-8s%-13s   %-20s\n", "Date", "Start", "End", "Type", "Remarks");
			fprintf(fp, "======================================================================\n");
			for(i = 0; i < vnum; i++){
				Event ce = valid[i];
				fprintf(fp, "2018-04-%02d%3s%02d:00%3s%02d:00%3s%-13s%3s", ce.date + 1, "", ce.time + 8, "", ce.time + ce.dur + 8, "", cmd_name[ce.type], "");
				if(ce.num == 1) fprintf(fp, "-\n");
				else{
					for(j = 0; j < ce.num; j++){
						if(ce.users[j] == user) continue;
						fprintf(fp, "%s ", users[ce.users[j]]);
					}
					fprintf(fp, "\n");
				}
			}
			fprintf(fp, "    - End -\n");
			fprintf(fp, "======================================================================\n");
		} else if(cmd[0] == 5){
			//printf("receive print report\n");
			int s;
			fprintf(fp, "Personal Organizer\n***Schedule Report***\n\n");
			for(s = 0; s < scheduler_num; s++){
				//int sch = cmd[2], user = cmd[1];
				buf[0] = 0;
				if((n = read(rs_pipe[s], buf, 500)) < 0) {perror("Read Error from sch 2"); exit(EXIT_FAILURE);}
				else if(n == 0) {printf("pipe from schedule closed 2\n"); break;}
				//printf("OutPut receive from schdule %s: \n", sch_alg[s]);
				//printf("\n\n", n);
				//printf("User: %s\n", users[user]);
				Event valid[2000], invalid[2000];
				int vnum = 0, inum = 0;
				int ts = 0;
				int x;
				for(x = 0; x < e_num; x++){
					e[x].valid = 0;
				}
				for(i = 0; i < n - 1; i += 4){
					int eid = (buf[i] * 256) + buf[i + 1];
					int date = buf[i + 2];
					int time = buf[i + 3];
					
					valid[vnum] = e[eid];
					valid[vnum].date = date; 
					valid[vnum++].time = time; 
					ts += e[eid].dur * e[eid].num;
					e[eid].valid = 1;
					//printf("eid: %d %d %d\n", eid, date, time);
				}

				for(i = 0; i < e_num; i++){
					//printf("id: %d, valid: %d\n", e[i].id, e[i].valid);
					if(!e[i].valid){
						invalid[inum++] = e[i];
					}
				}
				qsort(valid, vnum, sizeof(Event), cmp_time);
				qsort(invalid, inum, sizeof(Event), cmp_time);

				
				
				fprintf(fp, "Algorithm used: %s\n\n", sch_name[s]);
				fprintf(fp, "***Accepted List***\n");
				fprintf(fp, "There are %d requests accepted.\n\n", vnum);
				fprintf(fp, "%-13s%-8s%-8s%-13s   %-20s\n", "Date", "Start", "End", "Type", "Remarks");
				fprintf(fp, "======================================================================\n");
				for(i = 0; i < vnum; i++){
					Event ce = valid[i];
					fprintf(fp, "2018-04-%02d%3s%02d:00%3s%02d:00%3s%-13s%3s", ce.date + 1, "", ce.time + 8, "", ce.time + ce.dur + 8, "", cmd_name[ce.type], "");
					for(j = 0; j < ce.num; j++){
						fprintf(fp, "%s ", users[ce.users[j]]);
					}
					fprintf(fp, "\n");
				}
				fprintf(fp, "======================================================================\n\n");
				fprintf(fp, "There are %d requests rejected.\n\n", inum);
				fprintf(fp, "%-13s%-8s%-8s%-13s%-20s\n", "Date", "Start", "End", "Type", "Remarks");
				fprintf(fp, "======================================================================\n");
				for(i = 0; i < inum; i++){
					Event ce = invalid[i];
					fprintf(fp, "2018-04-%02d%3s%02d:00%3s%02d:00%3s%-13s%3s", ce.date + 1, "", ce.time + 8, "", ce.time + ce.dur + 8, "", cmd_name[ce.type], "");
					for(j = 0; j < ce.num; j++){
						fprintf(fp, "%s ", users[ce.users[j]]);
					}
					fprintf(fp, "\n");
				}
				fprintf(fp, "======================================================================\n\n");
				fprintf(fp, "Total number of request:\t%d\n", e_num);
				fprintf(fp, "Timeslot in use:        \t%d hours\n", ts);
				fprintf(fp, "Timeslot not in use:    \t%d hours\n", 560 - ts);
				fprintf(fp, "Utilization:            \t%d %\n", ts * 100 / 560);
				//printf("%s\n", sch_name[s]);
				fprintf(fp, "   - End -\n");
				fprintf(fp, "======================================================================\n\n");
				
			}
		}
		if(cmd[0] >= 4){
			//printf("file closed\n");
			fclose(fp);	
		}
	}
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

	/*pid = fork();
	if(pid == 0){
		close_pipe(p2s_fd[1][1], s2o_fd[1][0]);

		// the main scheduler function
		PR1_scheduler(p2s_fd[1][0], s2o_fd[1][1]);
		return 0;
	}*/

	// the third SJF scheduler
	pid = fork();
	if(pid == 0){
		close_pipe(p2s_fd[2][1], s2o_fd[2][0]);

		// the main scheduler function
		SJF_scheduler(p2s_fd[2][0], s2o_fd[2][1]);
		return 0;
	}

	pid = fork();
	if(pid == 0){
		close_pipe(p2s_fd[3][1], s2o_fd[3][0]);

		// the main scheduler function
		re_scheduler(p2s_fd[3][0], s2o_fd[3][1]);
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
	// close all the pipes
	for(i = 0; i < scheduler_num; i++){
		//printf("closing %d\n", i);
		close_pipe(p2s_fd[i][0], s2o_fd[i][1]);
		//printf("%d closed\n", i);
	}
	close(p2o_fd[0]);
	

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
				continue;
			}
			for(i = 0; i < n; i++){
				//printf("%d ", cmd[i]);
			}

			if(cmd[0] == 3){ // command add Batch
				FILE * fp = fopen(file, "r");
				char * fl;
				size_t len = 0;
				while(getline(&fl, &len, fp) != -1){
					//printf("%s\n", fl);
					int cmd[15], n;
					n = sep_line(fl, cmd, file);
					if(n == -1) {printf("invalid Command\n"); continue;}
					for(i = 0; i < n; i++) buf[i] = cmd[i];
					write_all(buf, n);
					usleep(2500);
				}
				fclose(fp);
				if(fl) free(fl);
			}

			if(cmd[0] >= 3){
				//printf("filename: %s", file);
			}
			//printf("\n");

			if(cmd[0] < 3){
				for(i = 0; i < n; i++) buf[i] = cmd[i];
				write_all(buf, n);
			}
			if(cmd[0] >= 4){
				for(i = 0; i < n; i++) buf[i] = cmd[i];
				for(i = 0; i < scheduler_num; i++)
					if(write(p2s_fd[i][1], buf, n) < 0) perror("write failed 3");
				int len = strlen(file);
				for(j = 0; j < len; j++){
					buf[j + 5] = file[j];
				}
				buf[j + 5] = 0;
				n = 6 + len;
				if(write(p2o_fd[1], buf, n) < 0) perror("write failed 4");
			}

		}
		printf("Please enter ->\n");
	}


	
	while(wait(NULL) > 0);
	return 0;
}
