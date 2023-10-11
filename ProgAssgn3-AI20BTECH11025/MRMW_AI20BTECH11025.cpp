#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <chrono>
#include <random>
#include <fstream>
#include <atomic>

using namespace std;

int MIN_VALUE = -1;
double total_time[16]; 
double p = 0.5;
int numOps;
int lambda;
int capacity;

// To sample a random number from exp distribution
double random_expo(double lambda){
    double u = rand()/(RAND_MAX + 1.0);
    return -log(1 - u)/lambda;
}

struct StampedValue 
{
    long stamp;
    int value;

    StampedValue() : stamp(0), value(MIN_VALUE) {} // Default constructor
    StampedValue(int init) : stamp(0), value(init) {} // initial value with zero timestamp
    StampedValue(long ts, int v) : stamp(ts), value(v) {} // later values with timestamp provided
};

StampedValue a_table[16];

// Write function for MRMW register
void write(int me, int value) 
{
    int max_stamp = MIN_VALUE;
    for (int i = 0; i < capacity; i++) {
        if (a_table[i].stamp > max_stamp) {
            max_stamp = a_table[i].stamp;
        }
    }
    a_table[me] = StampedValue(max_stamp + 1, value);
}

// Read function for MRMW register
int read() 
{
    StampedValue max_value(-1, -1);
    for (int i = 0; i < capacity; i++) {
        StampedValue value = a_table[i];
        if (value.stamp > max_value.stamp) {
            max_value = value;
        }
    }
    return max_value.value;
}

void testAtomic(int id) 
{
    int k = numOps;
    int lVar;

    for (int i = 0; i < k; ++i) 
    {
        // Start time
        std::chrono::time_point<std::chrono::high_resolution_clock> reqTime, complTime;
        reqTime = std::chrono::high_resolution_clock::now();

        // Randomly generate action to choose read or write
        double action = (double) rand()/RAND_MAX;

        if (action < p) 
        {
            // Read action
            lVar = read();
            cout << "Value read: " << lVar << endl;
        } 
        else 
        {
            // Write action
            lVar = k * id;
            write(id, lVar);
            cout << "Value written: " << lVar << endl;
        }

        // End time
        complTime = std::chrono::high_resolution_clock::now();
        double time = std::chrono::duration_cast<std::chrono::nanoseconds>(complTime - reqTime).count();
        cout << i << "th action completed at " << chrono::duration_cast<chrono::milliseconds>(complTime.time_since_epoch()).count() << " by thread " << id << endl;

        // Simulate performing some other operations (t1 is the delay value)
        double t1 = random_expo(lambda);
        total_time[id] += t1;
        std::this_thread::sleep_for(std::chrono::nanoseconds(static_cast<long long>(t1 * 1e9)));
    }
}

int main() 
{
    // Take input capacity, numOps and lambda
    ifstream inputfile("inp-params.txt");
    inputfile >> capacity >> numOps >> lambda;
    inputfile.close();

    int N = capacity;

    float time = 0;
    
    // Initialize thread times
    for(int i = 0; i < N; i++) total_time[i] = 0; 

    thread t[N];

    // Launch threads
    for (int i = 0; i < N; i++) {
        t[i] = thread(testAtomic, i);
    }

    // Join threads
    for (int i = 0; i < N; i++) {
        t[i].join();
    }

    // Total average time for a single operation (read/write)
    for (int i = 0; i < N; i++) 
    {
        time += total_time[i]/numOps;
        cout << "Average time by thread " << i+1 << " is " << total_time[i]/numOps << " " << endl;
    }
    
    cout << "Total average time = " << time/N << endl;

    return 0;
}
