#include <iostream>
#include <thread> // for multithreading
#include <random> // for random numbers
#include <queue> // account for line of students
#include <vector> // for storing data like duration
#include <assert.h> // to check for errors

using namespace std;

// For random numbers
random_device rd;
mt19937_64 gen(rd());

int genRand(int min, int max);

struct Student{
    int service_rate = (genRand(10, 20)); // random number between 10 and 20 seconds
    chrono::steady_clock::time_point curr = chrono::steady_clock::now();
};

void threadTiming(){
    this_thread::sleep_for(chrono::minutes(1)); // sleep one minute
}

void spawnStudents(thread& timer, queue<Student>& wait);

void serveStudents(thread& timer, queue<Student>& wait, vector<chrono::seconds>& times,
        vector<chrono::seconds>& withProf);

void instantiateOfficeHours(vector<chrono::seconds>& compWait, vector<chrono::seconds>& compWithProf,
        vector<chrono::seconds>& compOT);

int main(){
    vector<chrono::seconds> compiled_waits;
    vector<chrono::seconds> compiled_time_with_prof;
    vector<chrono::seconds> compiled_overtime;
    vector<thread> threadVector;

    for(int i = 0; i < 100; i++){   // Run the simulation 100 times in parallel
        thread officeHours(instantiateOfficeHours, ref(compiled_waits), ref(compiled_time_with_prof),
                ref(compiled_overtime)); // Need C++11
        threadVector.push_back(move(officeHours));
    }

    for(thread& th : threadVector){  // end all 100 threads
        th.join();
    }

    int total = 0;
    for(auto e : compiled_waits){
        cout << e.count() << endl;
        total += e.count();
    }
    cout << "Average time spent waiting: " << total / compiled_waits.size() << " seconds" << endl;

    total = 0;
    for(auto e : compiled_time_with_prof){
        cout << e.count() << endl;
        total += e.count();
    }
    cout << "Average time with prof: " << total / compiled_time_with_prof.size() << " seconds" << endl;

    total = 0;
    for(auto e : compiled_overtime){
        cout << e.count() << endl;
        total += e.count();
    }
    cout << "Average overtime: " << total / compiled_overtime.size() << " seconds" << endl;
    return 0;
}
void instantiateOfficeHours(vector<chrono::seconds>& compWait, vector<chrono::seconds>& compWithProf,
        vector<chrono::seconds>& compOT) {

    queue<Student> waiting_room;
    vector<chrono::seconds> waiting_times;
    vector<chrono::seconds> time_spent_with_prof;

    thread timer(threadTiming);
    thread spawn(spawnStudents, ref(timer), ref(waiting_room)); // Need C++11 for ref
    //this_thread::sleep_for(chrono::milliseconds(3)); // Let a student spawn
    thread serve(serveStudents, ref(timer), ref(waiting_room), ref(waiting_times),
            ref(time_spent_with_prof)); // Need C++11 for ref

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

}

void spawnStudents(thread& timer, queue<Student>& wait){
    while(timer.joinable()){
        wait.push(Student());
        //cout << chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now().time_since_epoch()).count() << endl;
        cout << "Student Spawned" << endl;
        this_thread::sleep_for(chrono::seconds(genRand(5, 15))); // random number between 5 and 15 seconds
    }
}

void serveStudents(thread& timer, queue<Student>& wait, vector<chrono::seconds>& times,
        vector<chrono::seconds>& withProf){
    while(timer.joinable()) {
        while (!wait.empty()) {
            times.push_back(chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - wait.front().curr));
            this_thread::sleep_for(chrono::seconds(wait.front().service_rate));
            withProf.emplace_back(wait.front().service_rate); // need C++11
            //cout << chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now().time_since_epoch()).count() << endl;
            wait.pop();
            cout << "Student Served" << endl;
        }
    }
}

int genRand(int min, int max){
    uniform_int_distribution<> dis(min, max);
    return dis(gen);
}
