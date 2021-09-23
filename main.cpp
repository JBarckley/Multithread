#include <iostream>
#include <thread> // for multithreading
#include <random> // for random numbers
#include <queue> // account for line of students
#include <map> // account for topic data
#include <vector> // for storing data like duration
#include <unordered_set> // store only single instances with quick search time
#include <fstream> // file i/o
#include <assert.h> // to check for errors

using namespace std;

// Constant determining the wait length
// This is purely a debugging feature, don't mind it.
static const int _WAIT_LENGTH_SCALE = 0;

// For random numbers (Need C++11)
random_device rd;
mt19937_64 gen(rd());

int genRand(int min, int max);

struct Student{
    int service_rate = (genRand(10, 20)); // random number between 10 and 20 seconds
    chrono::steady_clock::time_point curr = chrono::steady_clock::now();
    int priority = genRand(0, 1000);
    int ID = genRand(10, 99);
    string name = "Student" + to_string(ID); // 89 different possible students (lmao don't want to deal with non-two digits #s)
    int topic = genRand(0, 30); // 30 different topics

    bool operator <(const Student& rhs) const{  // needed for priority_queue from STL
        return priority < rhs.priority;
    }

};

void threadTiming(){
    this_thread::sleep_for(chrono::minutes(1) - chrono::seconds(5 * _WAIT_LENGTH_SCALE)); // sleep one minute
}

void spawnStudents(thread& timer, priority_queue<Student>& wait);

void serveStudents(thread& timer, priority_queue<Student>& wait, vector<chrono::seconds>& times,
        vector<chrono::seconds>& withProf, multimap<string, int>& topics);

void instantiateOfficeHours(vector<chrono::seconds>& compWait, vector<chrono::seconds>& compWithProf,
        vector<chrono::seconds>& compOT, map<string, pair<int, unordered_set<int>>>& timesVisited);

void sortData(bool column, bool dir, map<string, pair<int, unordered_set<int>>> comp_topic_data, string studentList[], int& size,
        unordered_set<string> topic_pairing[]);

void searchName(string name, map<string, pair<int, unordered_set<int>>> comp_topic_data, string studentList[], int& size);

void searchTopic(int ID, map<string, pair<int, unordered_set<int>>> comp_topic_data, unordered_set<string> topic_pairing[]);

int main(){
    vector<chrono::seconds> compiled_waits;
    vector<chrono::seconds> compiled_time_with_prof;
    vector<chrono::seconds> compiled_overtime;
    map<string, pair<int, unordered_set<int>>> comp_topic_data;
    vector<thread> threadVector;

    cout << string("Student40").compare("Student56") << endl;
    cout << string("Student56").compare("Student40") << endl;

    for(int i = 0; i < 100; i++){   // Run the simulation 100 times in parallel
        thread officeHours(instantiateOfficeHours, ref(compiled_waits), ref(compiled_time_with_prof),
                ref(compiled_overtime), ref(comp_topic_data)); // Need C++11
        threadVector.push_back(move(officeHours));
    }

    for(thread& th : threadVector){  // end all 100 threads
        th.join();
    }

    double total = 0;
    for(auto e : compiled_waits){
        cout << e.count() << endl;
        total += e.count();
    }
    cout << "Average time spent waiting: " << total / double(compiled_waits.size()) << " seconds" << endl;

    total = 0;
    for(auto e : compiled_time_with_prof){
        cout << e.count() << endl;
        total += e.count();
    }
    cout << "Average time with prof: " << total / double(compiled_time_with_prof.size()) << " seconds" << endl;

    total = 0;
    for(auto e : compiled_overtime){
        cout << e.count() << endl;
        total += e.count();
    }
    cout << "Average overtime: " << total / double(compiled_overtime.size()) << " seconds" << endl;

    /* IDEA:
    * Dir: true means ascending sort (as usual) : false means descending sort
    *
    * Column: true means sort by student name : false means sort by first topic asked about
    */
    bool DIR = false;
    bool COLUMN = false;
    string studentList[100];
    unordered_set<string> topic_pairing[31];
    int student_list_size = 0;
    sortData(COLUMN, DIR, comp_topic_data, studentList, student_list_size, topic_pairing);

    /* IDEA:
     * name: Student's name to search for and find statistics *STRING*
     *
     * topic: Topic ID to search for and find statistics *INT*
     */
    if(COLUMN) {
        searchName("Student40", comp_topic_data, studentList, student_list_size);
    }
    else {
        searchTopic(13, comp_topic_data, topic_pairing);
    }

    return 0;
}
void instantiateOfficeHours(vector<chrono::seconds>& compWait, vector<chrono::seconds>& compWithProf,
        vector<chrono::seconds>& compOT, map<string, pair<int, unordered_set<int>>>& timesVisited) {

    priority_queue<Student> waiting_room;
    vector<chrono::seconds> waiting_times;
    vector<chrono::seconds> time_spent_with_prof;
    multimap<string, int> studentTopics; // intent is <Student.name, Student.topic>

    thread timer(threadTiming);
    thread spawn(spawnStudents, ref(timer), ref(waiting_room)); // Need C++11 for ref
    //this_thread::sleep_for(chrono::milliseconds(3)); // Let a student spawn
    thread serve(serveStudents, ref(timer), ref(waiting_room), ref(waiting_times),
            ref(time_spent_with_prof), ref(studentTopics)); // Need C++11 for ref

    timer.join();
    auto oneHourElapsed = chrono::steady_clock::now();
    spawn.join();
    serve.join();

    chrono::seconds overtime = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - oneHourElapsed);
    //cout << "Time spent post one hour: " << overtime.count() << " milliseconds" << endl;
    compOT.push_back(overtime);

    chrono::seconds seconds_waiting;
    chrono::seconds sec_with_prof;
    for(auto e : waiting_times){
        //cout << e.count() << endl;
        seconds_waiting += e;
    }
    //cout << "Average time spent waiting: " << seconds_waiting / waiting_times.size() << " seconds" << endl;
    compWait.emplace_back(seconds_waiting / waiting_times.size()); // Need C++11

    for(auto e : time_spent_with_prof){
        //cout << e.count() << endl;
        sec_with_prof += e;
    }
    //cout << "Average time with prof: " << sec_with_prof / time_spent_with_prof.size() << " seconds" << endl;
    compWithProf.emplace_back(sec_with_prof / time_spent_with_prof.size()); // Need C++11

    // intent: <Student.name, <times_visited, set<topics asked about>>>
    for(auto e : studentTopics){
        timesVisited[e.first].first++;
        timesVisited[e.first].second.insert(e.second); // such an ugly line lmao
    }
    //topic_data.push_back(timesVisited);

}

void spawnStudents(thread& timer, priority_queue<Student>& wait){
    while(timer.joinable()){
        Student newStudent = Student();
        wait.push(newStudent);
        //cout << chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now().time_since_epoch()).count() << endl;
        cout << "Student Spawned: Priority #" << newStudent.priority << endl;
        this_thread::sleep_for(chrono::seconds(genRand(5, 15) - _WAIT_LENGTH_SCALE)); // random number between 5 and 15 seconds
    }
}

void serveStudents(thread& timer, priority_queue<Student>& wait, vector<chrono::seconds>& times,
        vector<chrono::seconds>& withProf, multimap<string, int>& topics){
    while(timer.joinable()) {
        while (!wait.empty()) {
            Student first = wait.top();
            times.push_back(chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - wait.top().curr));
            this_thread::sleep_for(chrono::seconds(wait.top().service_rate - _WAIT_LENGTH_SCALE));
            withProf.emplace_back(wait.top().service_rate); // need C++11
            topics.insert(pair<string, int>(wait.top().name, wait.top().topic));
            cout << wait.top().name << " Served: Priority #" << wait.top().priority << endl;
            wait.pop();
        }
    }
}

int genRand(int min, int max){
    uniform_int_distribution<> dis(min, max);
    return dis(gen);
}

template<typename T>
void insertion_sort(T data[], int size, bool dir){
    if(dir) {
        T to_insert = T();
        for (int i = 1; i < size; i++) {
            to_insert = data[i];
            for (int k = i; k > 0; k--) {
                data[k] = data[k - 1];
                if (dir ? to_insert > data[k - 1] : to_insert < data[k - 1]) {  // perform ascending or descending sort based on variable
                    data[k] = to_insert;
                    break;  // insertion spot found, insert new number
                } else if (k == 1) {  // value being inserted is less than all
                    data[0] = to_insert;
                }
            }
        }
    }
}

// Override previous template IF sort is passed string
template<>
void insertion_sort<string>(string data[], int size, bool dir){
    if(dir) {
        string to_insert = "";
        for (int i = 1; i < size; i++) {
            to_insert = data[i];
            for (int k = i; k > 0; k--) {
                data[k] = data[k - 1];
                if (dir ? to_insert.compare(data[k - 1]) : data[k - 1].compare(to_insert)) {  // perform ascending or descending sort based on variable
                    data[k] = to_insert;
                    break;  // insertion spot found, insert new number
                } else if (k == 1) {  // value being inserted is less than all
                    data[0] = to_insert;
                }
            }
        }
    }
}

/* IDEA:
 * Dir: true means ascending sort (as usual) : false means descending sort
 *
 * Column: true means sort by student name : false means sort by first topic asked about
 */
void sortData(bool column, bool dir, map<string, pair<int, unordered_set<int>>> comp_topic_data, string studentList[],
        int& i, unordered_set<string> topic_pairing[]){

    ofstream out("student_data.txt");

    if(column) {
        i = 0; // current index of student array
        for (auto student : comp_topic_data) {
            studentList[i] = student.first;
            i++;
        }
        insertion_sort<string>(studentList, i, dir); // uses dir variable to determine order of sort
        for(int k = 0; k < i; k++){
            string e = studentList[k];
            out << e << ":" << endl;
            out << "Visited office hours " << comp_topic_data.at(e).first << " times" << endl;
            out << "Asked about topics: ";
            for(auto topicNumbers : comp_topic_data.at(e).second) {  // loop through unordered_set in data pair
                out << topicNumbers << ", ";
            }
            out << endl;
        }
    }
    else {
        // Step 1: create list of possible topics (done in function params)

        // Step 2: loop through students and place each student in corresponding unordered set per topic
        for(auto student : comp_topic_data){
            for(auto topic : student.second.second){
                topic_pairing[topic].insert(student.first);
            }
        }

        // Step 3: loop through topic student pairing and output sorted results
        // if dir is true, loop forwards, if dir is false, loop backwards
        if(dir) {
            for (int k = 0; k < 31; k++) {
                if (!(topic_pairing[k].empty())) {  // only record topics included in the data set
                    out << "Topic" << k << ": " << endl;
                    out << "Asked by students: ";
                    for (auto student : topic_pairing[k]) {
                        out << student << ", ";
                    }
                    out << endl;
                }
            }
        }
        else{
            for (int k = 30; k >= 0; k--) {
                if (!(topic_pairing[k].empty())) {  // only record topics included in the data set
                    out << "Topic" << k << ": " << endl;
                    out << "Asked by students: ";
                    for (auto student : topic_pairing[k]) {
                        out << student << ", ";
                    }
                    out << endl;
                }
            }
        }

        out.close();

    }
}

int binarySearch(string studentList[], int left, int right, string target)
{
    if (right >= left) {
        int middle = (left + (right - left)) / 2;

        // target found :)
        if (studentList[middle] == target) {
            return middle;
        }

        // target in left side
        if (studentList[middle].compare(target) > 0) {
            return binarySearch(studentList, left, middle - 1, target);
        }

        // target in right side
        return binarySearch(studentList, middle + 1, right, target);
    }

    // target not found
    return -1;
}

void searchName(string name, map<string, pair<int, unordered_set<int>>> comp_topic_data, string studentList[], int& size){
    if(name != ""){
        int target_index = binarySearch(studentList, 0, size - 1, name);
        if(target_index >= 0) {
            string e = studentList[target_index];
            cout << e << ":" << endl;
            cout << "Visited office hours " << comp_topic_data.at(e).first << " times" << endl;
            cout << "Asked about topics: ";
            for (auto topicNumbers : comp_topic_data.at(e).second) {  // loop through unordered_set in data pair
                cout << topicNumbers << ", ";
            }
            cout << endl;
        }
        else {
            cout << "target not found!" << endl;
        }
    }
}

void searchTopic(int ID, map<string, pair<int, unordered_set<int>>> comp_topic_data, unordered_set<string> topic_pairing[]){
    if (!(topic_pairing[ID].empty())) {  // only record topics included in the data set
        cout << "Topic" << ID << ": " << endl;
        cout << "Asked by students: ";
        for (auto student : topic_pairing[ID]) {
            cout << student << ", ";
        }
        cout << endl;
    }
    else{
        cout << "Topic was not asked" << endl;
    }
}
