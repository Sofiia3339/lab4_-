#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <vector>
#include <sstream>
#include <chrono>
#include <mutex>
#include <random>

using namespace std;
using namespace chrono;

enum CommandType { READ, WRITE, TO_STRING };

struct Command {
    CommandType type;
    int field;
    int value;
};

class A {
    int first = 1;
    int second = 1;
    mutex m1;
    mutex m2;

public:
    int getFirst() {
        lock_guard<mutex> lock(m1);
        return first;
    }

    int getSecond() {
        lock_guard<mutex> lock(m2);
        return second;
    }

    void setFirst(int a) {
        lock_guard<mutex> lock(m1);
        first = a;
    }

    void setSecond(int a) {
        lock_guard<mutex> lock(m2);
        second = a;
    }

    operator string() {
        lock_guard<mutex> l1(m1);
        lock_guard<mutex> l2(m2);
        return to_string(first) + ", " + to_string(second);
    }
};

vector<Command> loadCommands(const string& filename) {
    vector<Command> commands;
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: Cannot open file " << filename << endl;
        return commands;
    }
    string line, cmdStr;
    while (getline(file, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        ss >> cmdStr;
        Command cmd;

        if (cmdStr == "write") {
            cmd.type = WRITE;
            ss >> cmd.field >> cmd.value;
        }
        else if (cmdStr == "read") {
            cmd.type = READ;
            ss >> cmd.field;
        }
        else if (cmdStr == "string") {
            cmd.type = TO_STRING;
        }
        commands.push_back(cmd);
    }
    return commands;
}

void workerFunction(const vector<Command>& commands, A& obj) {
    for (const auto& cmd : commands) {
        switch (cmd.type) {
        case WRITE:
            if (cmd.field == 0) obj.setFirst(cmd.value);
            else obj.setSecond(cmd.value);
            break;
        case READ:
            if (cmd.field == 0) obj.getFirst();
            else obj.getSecond();
            break;
        case TO_STRING:
        {
            string s = string(obj);
        }
        break;
        }
    }
}

void runBenchmark(const vector<string>& filenames, const string& description) {
    A obj;

    vector<vector<Command>> threadTasks;
    for (const auto& file : filenames) {
        threadTasks.push_back(loadCommands(file));
    }

    if (threadTasks.empty() || threadTasks[0].empty()) {
        cout << "Warning: No commands loaded for " << description << endl;
        return;
    }

    cout << description << " execution (" << filenames.size() << " threads):" << endl;

    auto start = high_resolution_clock::now();

    vector<thread> threads;
    for (int i = 0; i < filenames.size(); ++i) {
        threads.emplace_back(workerFunction, ref(threadTasks[i]), ref(obj));
    }

    for (auto& t : threads) {
        t.join();
    }

    auto end = high_resolution_clock::now();

    cout << "Time taken: " << duration_cast<milliseconds>(end - start).count() << " ms" << endl;
    cout << "------------------------------------------------------------------" << endl;
}

void generateTestFile(const string& filename, int count) {
    ofstream file(filename);
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> percent(0, 99);
    uniform_int_distribution<> val(1, 100);

    for (int i = 0; i < count; ++i) {
        int p = percent(gen);

        if (p < 10) file << "read 0" << endl;
        else if (p < 20) file << "write 0 " << val(gen) << endl;
        else if (p < 70) file << "read 1" << endl;
        else if (p < 80) file << "write 1 " << val(gen) << endl;
        else file << "string" << endl;
    }
    file.close();
    cout << "Generated " << filename << " with " << count << " commands." << endl;
}

int main() {
    int commandsCount = 100000;
    vector<string> allFiles = { "input1.txt", "input2.txt", "input3.txt" };

    cout << "Generating test files..." << endl;
    for (const auto& f : allFiles) {
        generateTestFile(f, commandsCount);
    }
    cout << "------------------------------------------------------------------" << endl;

    vector<string> set1 = { "input1.txt" };
    vector<string> set2 = { "input1.txt", "input2.txt" };
    vector<string> set3 = { "input1.txt", "input2.txt", "input3.txt" };

    runBenchmark(set1, "Single-threaded");
    runBenchmark(set2, "Two-threaded");
    runBenchmark(set3, "Three-threaded");

    return 0;
}