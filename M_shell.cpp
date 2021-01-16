/*
	INSTRUCTIONS:
	In order to run this project:
		Step 0: Install the history library:
				sudo apt-get install libreadline6 libreadline6-dev
				
		Step 0.5: Install the libboost library
				sudo apt-get install libboost-dev
				
		Step 1: g++ shellFinalVersionBijanMarius.cpp -lreadline
		
		Step 2: g++ -o shellFinalVersionBijanMarius shellFinalVersionBijanMarius.cpp -lreadline
		
		Step 3: ./a.out
*/

#include <stdio.h>
#include <string.h>
#include <string>
#include <ctype.h>
#include <fcntl.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sys/wait.h>
#include <boost/algorithm/string/predicate.hpp>

//the defined items below are used only for parsing
#define LINE '|'
#define SPACE ' '
#define ARROW '>'
#define B_ARROW '<'

int checkForWhiteSpaces(char *command_buff);
int checkForPipe(char *command_buff);
int checkForRedirect(char *command_buff);
int executeCommand(int nr, char **command_buff);
int numberOfWords(char *command_buff);
int countPipes(char *command_buff);
int isCommand(char *command_buff);
int exist(const char *fname);
int countLinesInAFile(char *path);
int countCharactersInAFile(char *path);
char **readAndSplit(char *path);
char **stringParsing(char *command_buff, char parseSymbol);
char *getArgument(int nr, char **command_buff);

//Functions for uniq command
void executeUniqCommandWithoutArguments(char *path);
void executeUniqCommandWithIArgument(char *path);
void executeUniqCommandWithUIArgument(char *path);
void executeUniqCommandWithIDArgument(char *path);
void executeUniqCommandWithDArgument(char *path);
void executeUniqCommandWithUArgument(char *path);

//Functions for tail command
void executeTailCommandWithoutArguments(char *path);
void executeTailCommandWithNArguments(int n, char *path);
void executeTailCommandWithCArguments(int c, char *path);
void executeTailCommandWithCArguments(int q, char *path);
void executeTailCommandWithVArguments(char *path);
void executeTailCommandWithCAndNArguments(int c, char *path);


char *command;
char root[100] = "\033[1m\033[34m_&>\033[0m"; //the "_&>" will be colored blue
char currentDirectory[150];

int main(int argc, char **argv){
	
	while(1){
		command = readline(strcat(getcwd(currentDirectory, 100), root));
		int nr = numberOfWords(command);
		char **comms = stringParsing(command, SPACE);
		
		if(strlen(command) > 0) add_history(command);
		
		//the characters before and after EXIT are for coloring the text red and then reseting in back to normal
		if(!strcmp(command, "exit")){
			std::cout << "\033[1m\033[31mEXIT\n\033[0m";
			exit(0);
		}
		
		//it just change the directory
		else if(!strcmp(comms[0], "cd")){
			if(chdir(comms[1]) != 0) 
				perror("Error: ");
		}
		
		//if there are no pipes and no redirection ...
		else if(checkForPipe(command) != 1 && checkForRedirect(command) == 0) {
			//it checks for a command. If there is a command that I had to implement, I execute it...
			if(isCommand(command)){
				int p = fork();
				if(p<0){
					perror("Error");
					abort();
				}
				else if(p == 0){
					executeCommand(nr, comms);
					exit(0);
				}
				wait(NULL);
			}
			
			//...otherwise the command is executed using execvp function
			else{
				int pid = fork();
				if(pid == -1)
					return 1;
				if(pid==0){
					if(checkForWhiteSpaces){
						char **commands = stringParsing(command, SPACE);
						execvp(commands[0], &commands[0]);
						perror("Invalid");
						abort();
					}
				}
				waitpid(pid, NULL, 0);
			}
		}
		//If the received input has pipes, then...
		else if(checkForPipe(command) == 1){	
			int p = fork();
			if(p<0){
				perror("Error");
				abort();
			}
			else if(p == 0){
				//the string is parsed
				// the command is parse in an array, each command being an element of the array
				char **commands = stringParsing(command, LINE); 
				
				int rows =  countPipes(command); // the number of commands from commands
				int index = 0;
				
				//the while checks every command from the array
				while(index < rows)
				{
					char **commands_and_options = stringParsing(commands[index], SPACE); 
					//it parsed every command in the vector, like that: {ls, -l}
					int fd[2]; 
					pipe(fd);
					if(!fork()){	
						dup2(fd[1], 1);
						close(fd[0]); 
						execvp(commands_and_options[0], &commands_and_options[0]);
						perror("Exec");
						abort();
					
					}
					dup2(fd[0], 0);
					close(fd[1]);
					index++;
				}
				
				char temp[1000];
				while(fgets(temp, 1000, stdin)!=NULL){
					std::cout << temp;
				}
				exit(0);
			}
			wait(NULL);
		}
		
		else if(checkForRedirect(command) != 0){
			int response = checkForRedirect(command);
			int pid;
			if ((pid = fork()) < 0)
				perror("Error: ");
			else if (pid == 0){
				char **commands;
				response == 1 ? commands = stringParsing(command, ARROW) : commands = stringParsing(command, B_ARROW);
				char **commands_and_options = stringParsing(commands[0], SPACE);
			
				if (response == 1){
					int fd1 = creat(commands[1] , 0644) ;
					dup2(fd1, STDOUT_FILENO);
					close(fd1);
				}
				
				if (response == 2){
					int fd0 = open(commands[1], O_RDONLY, 0);
					dup2(fd0, 0);
					close(fd0);
				}
				std::cout << commands_and_options[0];
				execvp(commands_and_options[0], &commands_and_options[0]);
				perror("Error");
				exit(1);
			}
			wait(NULL);
		}
	}
	return 0;
}

//Return true(1) if any space found in the string that was received as argument
//Return false(0) otherwise
int checkForWhiteSpaces(char *command_buff){
	for(int i=0; i < strlen(command_buff); i++){
		if(command_buff[i] == ' ')
			return 1;
	}
	return 0;
}

//This method search for the '|' character that imply that there is a pipe(or more)
int checkForPipe(char *command_buff){
	for(int i = 0;  i < strlen(command_buff); i++)
		if(command_buff[i] == LINE) return 1;
	return 0;
}

//This method search for the '>' character that imply that there is a pipe(or more)
int checkForRedirect(char *command_buff){
	for(int i = 0;  i < strlen(command_buff); i++){
		if(command_buff[i] == ARROW){
			return 1;
		}
		else if(command_buff[i] == B_ARROW){
			return 2;
		}
	}
	return 0;
}

//return the number of words in a string
int numberOfWords(char *str){
	int words = 0;
	for(int i = 0; i < strlen(str); i++)
		if(str[i] == ' ')
			words++;
	return words + 1;
}

//this method execute the commands...
int executeCommand(int nr, char **command_buff){

	//if the command was help, then...
	if(!strcmp(command_buff[0], "help")){
		std::cout << "\033[1m\033[33m" << "HELP" << "\033[0m" << std::endl << "Linux shell created by M. --- Version 1.0\nIt supports any linux command, including pipes and redirection";
		std::cout << "\n"<< "\e[1m" << "NOTE:" << "\e[0m" <<" The current version does not supports piping WITH redirection!\n";
		std::cout << "The implemented comments are: " << std::endl;
		std::cout << "	\033[1m\033[34mcd arg -->\033[0m command used to change the current directory" << std::endl;
		std::cout << "	\033[1m\033[34mtail -opt -arg  -->\033[0m command used to print the last rows from a file" <<std::endl;
		std::cout << "		-> opt -n --> Prints the last 'n' lines instead of last 10 lines" <<std::endl;
		std::cout << "		-> opt -c --> Prints the last 'c' bytes instead of last 10 lines" <<std::endl;
		std::cout << "		-> opt -q --> It is used if more than 1 file is given.\n		Because of this command, data from each file is not precedes by its file name" <<std::endl;
		std::cout << "		-> opt -v --> By using this option, data from the specified file is always preceded by its file name." <<std::endl;
		std::cout << "	\033[1m\033[34muniq -opt -arg  -->\033[0m The uniq command in Linux is a command line utility that reports or filters out the repeated lines in a file" <<std::endl;
		std::cout << "		-> opt -d --> It only prints the repeated lines and not the lines which aren’t repeated." <<std::endl;
		std::cout << "		-> opt -i --> By default, comparisons done are case sensitive but with this option case insensitive comparisons can be made." <<std::endl;
		std::cout << "		-> opt -u --> It allows you to print only unique lines" <<std::endl;
		std::cout << "\033[1m\033[31mIf any error occurs try switching it off and on again\033[0m" <<std::endl;
		
	}
	
	//if the command is tail, then it checks for every option of the command given
	//the values are stored in the variables n, c, q, v;
	if(!strcmp(command_buff[0], "tail")){
		char opt;
		int n = 0, c = 0, q = 0, v = 0;
		int test = 1;
		while ((opt = getopt (nr, command_buff, "n:c:qv")) != -1){
			switch(opt){
				case 'n':{
					n = atoi(optarg);
					test = 0;
					break;
				}
				case 'c':{
					c = atoi(optarg);
					test = 0;
					break;
				}
				case 'q':{
					q = 1;
					test = 0;
					break;
				}
				
				case 'v':{
					v = 1;
					test = 0;
					break;
				}
				case '?':{
					perror("Error");
					break;
				}
			}
		}
		int sum = 0;
		for(int i = optind; i < nr; i++){
			sum++;
		}
		
		//it checks for every possible combination of options. Not very professional, but it does the job
		if(n && !c && !q && !v) {
			if(sum > 1) 
				for(int i = optind; i < nr; i++){
					executeTailCommandWithVArguments(command_buff[i]);
					executeTailCommandWithNArguments(n, command_buff[i]);
				}
				
			else for(int i = optind; i < nr; i++) executeTailCommandWithNArguments(n, command_buff[i]);
		}
					
			
		if(!n && c && !q && !v) {
			if(sum > 1) 
				for(int i = optind; i < nr; i++){
					executeTailCommandWithVArguments(command_buff[i]);
					executeTailCommandWithCArguments(c, command_buff[i]);
				}
				
			else for(int i = optind; i < nr; i++) executeTailCommandWithCArguments(c, command_buff[i]);
		}
				
		if(!n && !c && !q && v) {
			for(int i = optind; i < nr; i++){
				executeTailCommandWithVArguments(command_buff[i]);
				executeTailCommandWithoutArguments(command_buff[i]);
			}
		}
		
		if(!n && !c && q && !v) {
			for(int i = optind; i < nr; i++)
				executeTailCommandWithoutArguments(command_buff[i]);
		}
		
		if(n && !c  && v){  // without q because it does not matter, because v already replace q
			for(int i = optind; i < nr; i++){
				executeTailCommandWithVArguments(command_buff[i]);
				executeTailCommandWithNArguments(n, command_buff[i]);
			}
		}
		
		else if(!n && c && v){ // without q because it does not matter, because v already replace q
			for(int i = optind; i < nr; i++){
				executeTailCommandWithVArguments(command_buff[i]);
				executeTailCommandWithCArguments(c, command_buff[i]);
			}
		}
		
		else if (q && v){
			if (sum > 1)
				for(int i = optind; i < nr; i++){
					executeTailCommandWithVArguments(command_buff[i]);
					executeTailCommandWithoutArguments(command_buff[i]);
					std::cout << std::endl;
				}
			else for(int i = optind; i < nr; i++) executeTailCommandWithoutArguments(command_buff[i]);
		}
		
		else if(n && c){
			if (sum > 1)
				for(int i = optind; i < nr; i++){
					executeTailCommandWithVArguments(command_buff[i]);
					executeTailCommandWithNArguments(n, command_buff[i]);
				}
			else for(int i = optind; i < nr; i++) executeTailCommandWithNArguments(n, command_buff[i]);
		}
		
		else if(n && q){
			if (sum > 1)
				for(int i = optind; i < nr; i++){
					executeTailCommandWithNArguments(n, command_buff[i]);
				}
			else for(int i = optind; i < nr; i++) executeTailCommandWithNArguments(n, command_buff[i]);
		}
		
		else if(c && q){
			if (sum > 1)
				for(int i = optind; i < nr; i++){
					executeTailCommandWithCArguments(n, command_buff[i]);
				}
			else for(int i = optind; i < nr; i++) executeTailCommandWithCArguments(n, command_buff[i]);
		}
		
		if(test) {
			if(sum > 1){ //We do not print the correct values directly in the first for because if we have only a file
			//there is no need of printing the file name.
				for(int i = optind; i < nr; i++){
					executeTailCommandWithVArguments(command_buff[i]);
					executeTailCommandWithoutArguments(command_buff[i]);
					std::cout << std::endl;
				}
			}
			else executeTailCommandWithoutArguments(command_buff[optind]);
		}
		return 1;
	}
	
	//the same approach as at tail
	if(!strcmp(command_buff[0], "uniq")){
			char c;
			int test = 1; //test verifies if the code has at least 1 option
			int i = 0, u = 0, d = 0;
			while ((c = getopt (nr, command_buff, "idu")) != -1){
				switch(c){
					case 'i':{
						i = 1;
						test = 0;
						break;
					}
					case 'u':{
						u = 1;
						test = 0;
						break;
					}
					case 'd':{
						d = 1;
						test = 0;
						break;
					}
					case '?':{
						perror("Error");
						break;
					}
				}
			}
			if(i && !u && !d) executeUniqCommandWithIArgument(getArgument(nr, command_buff));
			if(!i && u && !d) executeUniqCommandWithUArgument(getArgument(nr, command_buff));
			if(!i && !u && d) executeUniqCommandWithDArgument(getArgument(nr, command_buff));
			if(i && u && !d) executeUniqCommandWithUIArgument(getArgument(nr, command_buff));
			if(i && !u && d) executeUniqCommandWithIDArgument(getArgument(nr, command_buff));
			
			if(test) executeUniqCommandWithoutArguments(getArgument(nr, command_buff));
				
		return 1;
	}
	
	if(!strcmp(command_buff[0], "version")) std::cout << "Version 1.0" << std::endl;
	
	return 0;
}

int countPipes(char *command_buff){
	int n = 1;
	for(int i = 0; i < strlen(command_buff); i++){
		if(command_buff[i] == LINE)
			n++;
	}
	return n;
}

int isCommand(char *command_buff){
	char **commands = stringParsing(command_buff, SPACE);
	
	if(!strcmp(commands[0], "help"))return 1;
	
	if(!strcmp(commands[0], "tail"))return 1;

	if(!strcmp(commands[0], "uniq")) return 1;
	
	if(!strcmp(commands[0], "version")) return 1;
	
	return 0;
	
}

//check if a file exists
int exist(const char * filename){
    FILE *file;
    if (file = fopen(filename, "r")){
        fclose(file);
        return 1;
    }
    return 0;
}

//this method parse a string after a given symbol. Usually a '|' or a ' '
char **stringParsing(char *command_buff, char parseSymbol){
	int j = 0, k = 0;
	char **commands;
	if(parseSymbol == '|')
		commands = (char**)malloc((countPipes(command_buff) + 1) * sizeof(char *));
	else if(parseSymbol == '>' || parseSymbol == '<')
		commands = (char**)malloc(2 * sizeof(char *));
	else 
		commands = (char**)malloc((numberOfWords(command_buff) + 1) * sizeof(char *));
	commands[0] = (char*)malloc(sizeof(char));
	for(int i = 0; i < strlen(command_buff); i++){
		if(command_buff[i] == parseSymbol){
			commands[j][k] = '\0';
			j++;
			commands[j] = (char*)malloc(sizeof(char));
			k = 0;
		}
	
		else{
			commands[j][k] = command_buff[i];
			k++;
		}
	}
	commands[j][k] = '\0';
	commands[j+1] = NULL;
	
	return commands;
}

//very silly method of obtaining the argument of an command.
char *getArgument(int nr, char **command_buff){
	return command_buff[nr - 1];
}

void executeUniqCommandWithoutArguments(char *path){
	std::string line_;
	std::string previous_line;
	std::ifstream file_(path);
	if(file_.is_open()){
		while(getline(file_, line_)){
			
			if(previous_line.empty()){
				std::cout << line_ << std::endl;
				previous_line = line_;
			}
			
			else if(previous_line != line_){
				std::cout << line_ << std::endl;
				previous_line = line_;
			}
		}
		file_.close();
	}
	
	else perror("File is not open");
}

void executeUniqCommandWithIArgument(char *path){
	std::string line_;
	std::ifstream file_(path);
	if(file_.is_open()){
		getline(file_, line_);
		std::string firstLine = line_;
		int checkFirstLine = 1;
		while(getline(file_, line_)){
			if(boost::iequals(firstLine, line_)) continue;
			else{ 
				std::cout << line_ << std::endl;
				firstLine = line_;
			}
		}
		file_.close();
	}
	
	else perror("Error: File not found or not opened");
}


void executeUniqCommandWithDArgument(char *path){
	std::string line_;
	std::ifstream file_(path);
	int oneEqual = 0; //with this variable we check if we already printed a line
	if(file_.is_open()){
		getline(file_, line_);
		std::string firstLine = line_;
		int check = 0;
		while(getline(file_, line_)){
			if(firstLine != line_) {
				firstLine = line_;
				check = 0;
			}
			else if(!check && firstLine == line_){
				std::cout << firstLine << std::endl;
				check = 1;
			}	
		}
		file_.close();
	}
	
	else perror("Error: File not found or not opened");
}

void executeUniqCommandWithUArgument(char *path){
	std::string line_;
	std::ifstream file_(path);
	std::string fileData[1000];
	if(file_.is_open()){
		int index = -1; // the size of the array
		while(getline(file_, line_)){
			index++;
			fileData[index] = line_;
		}
		file_.close();
		for(int i = 0; i <= index; i++){
			int check = 0;
			for(int j = 0; j <= index; j++){
				if(i == j) continue;
				else if(fileData[i] == fileData[j]) check = 1;
			}
			if(!check) std::cout << fileData[i] << std::endl;
		}
	}
	else perror("Error: File not found or not opened");
}

void executeUniqCommandWithUIArgument(char *path){
	std::string line_;
	std::ifstream file_(path);
	std::string fileData[1000];
	if(file_.is_open()){
		int index = -1; // the size of the array
		while(getline(file_, line_)){
			index++;
			fileData[index] = line_;
		}
		file_.close();
		for(int i = 0; i <= index; i++){
			int check = 0;
			for(int j = 0; j <= index; j++){
				if(i == j) continue;
				else if(boost::iequals(fileData[i], fileData[j])) check = 1;
			}
			if(!check) std::cout << fileData[i] << std::endl;
		}
	}
	else perror("Error: File not found or not opened");
}

void executeUniqCommandWithIDArgument(char *path){
	std::string line_;
	std::ifstream file_(path);
	int oneEqual = 0; //with this variable we check if we already printed a line
	if(file_.is_open()){
		getline(file_, line_);
		std::string firstLine = line_;
		int check = 0;
		while(getline(file_, line_)){
			if(!boost::iequals(firstLine,line_)) {
				firstLine = line_;
				check = 0;
			}
			else if(!check && boost::iequals(firstLine, line_)){
				std::cout << firstLine << std::endl;
				check = 1;
			}
		}
		file_.close();
	}
	
	else perror("Error: File not found or not opened");
}

//return the number of lines from a file. Need this for tail command
int countLinesInAFile(char *path){
	std::string line_;
	std::ifstream file_(path);
	int i = 0;
	if(file_.is_open()){
		while(getline(file_, line_)){
			i++;
		}
		file_.close();
	}
	
	else perror("Error: File not found or not opened");
	
	return i;
}

//Counts the character from a file. Need this from tail -c command
int countCharactersInAFile(char *path){
	std::string line_;
	std::ifstream file_(path);
	int i = 0;
	if(file_.is_open()){
		while(getline(file_, line_)){
			i += line_.size();
		}
		file_.close();
	}
	
	else perror("Error: File not found or not opened");
	
	return i;
}

void executeTailCommandWithoutArguments(char *path){
	std::string line_;
	std::ifstream file_(path);
	if(file_.is_open()){
		int index = 0;
		int n = 10;
		if(n > countLinesInAFile(path)) n = countLinesInAFile(path);
		
		int number = countLinesInAFile(path) - n;
		while(index != number){		
			getline(file_, line_);
			index++;
		}
		while(getline(file_, line_)){
			std::cout << line_ << std::endl;
		}
		file_.close();
	}
	else perror("Error: File not found or not opened");
}

void executeTailCommandWithNArguments(int n, char *path){
	std::string line_;
	std::ifstream file_(path);
	if(file_.is_open()){
		int index = 0;
		if(n > countLinesInAFile(path)) n = countLinesInAFile(path);
		
		int number = countLinesInAFile(path) - n;
		while(index != number){		
			getline(file_, line_);
			index++;
		}
		while(getline(file_, line_)){
			std::cout << line_ << std::endl;
		}
		file_.close();
	}
	else perror("Error: File not found or not opened");
}

void executeTailCommandWithCArguments(int c, char *path){
	std::string line_;
	std::ifstream file_(path);
	if(file_.is_open()){
		int index = 0;
		if(c > countCharactersInAFile(path)) c = countCharactersInAFile(path);
		getline(file_, line_);
		std::string s = line_;
		while(getline(file_, line_)){
			s += ("\n" + line_);
		}
		s+="\n";
		std::cout << s.substr(s.size() - c);
		file_.close();
	}
	else perror("Error: File not found or not opened");
}

void executeTailCommandWithVArguments(char *path){
	std::cout << "==> " << path << " <==" <<std::endl;
}


void executeTailCommandWithCAndNArguments(int c, char *path){
	std::string line_;
	std::ifstream file_(path);
	if(file_.is_open()){
		int index = 0;
		if(c > countCharactersInAFile(path)) c = countCharactersInAFile(path);
		getline(file_, line_);
		std::string s = line_;
		while(getline(file_, line_)){
			s += ("\n" + line_);
		}
		s+="\n";
		std::cout << s.substr(s.size() - c);
		file_.close();
	}
	else perror("Error: File not found or not opened");
}