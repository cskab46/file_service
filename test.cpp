#include <mutex>
#include <map>
#include <iostream>
#include <thread>

using namespace std;




int main(int argc, char **argv) {
	map<int,mutex> mutes;
	
	cout << mutes[0].try_lock();
	thread t([&mutes] (int a) { cout << mutes[0].try_lock() << endl;}, 0);
	t.join();
	
	return 0;
}
