#include <iostream>
#include <vector>
#include <fstream>
#include <chrono>
#include <thread>

using namespace std;

struct logData{
  double fps;
  double cpu;
  double gpu;
  double previous;
};

double fps;
std::vector<logData> logArray;

ofstream out;
const char* mango_output = std::getenv("MANGO_OUTPUT");
int duration, num, prevTick, currentTick;
time_t now_log = time(0);
tm *log_time = localtime(&now_log);
bool loggingOn;

void writeFile(){
	now_log = time(0);
	log_time = localtime(&now_log);
	string date = to_string(log_time->tm_year + 1900) + "-" + to_string(1 + log_time->tm_mon) + "-" + to_string(log_time->tm_mday) + "_" + to_string(1 + log_time->tm_hour) + "-" + to_string(1 + log_time->tm_min) + "-" + to_string(1 + log_time->tm_sec);

	out.open(mango_output + date, ios::out | ios::app);
	for (size_t i = 0; i < logArray.size(); i++) {
     out << logArray[i].fps << endl;
  }
	out.close();
	logArray.clear();
}

void *logging(void *){
  num = 0;
	while (loggingOn){
		logArray.push_back({fps, 0.0f, 0.0f, 0.0f});
    num++;
    this_thread::sleep_for(chrono::milliseconds(100));
  }
  writeFile();
  return NULL; 
}