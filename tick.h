#pragma once

#include <ctime>

class Tick {
private:
	time_t previous, now;
	bool triggered;

public:
	Tick() {
		triggered = false;
	}

	void start() {
		previous = time(NULL);
		triggered = false;
	}

	bool is(int seconds, bool once) {
		if (once && triggered)
			return true;

		now = time(NULL);
		long long diff = now - previous;
		if(diff >= seconds) {
			previous = diff;
			if(once && !triggered) {
				triggered = true;
			}
			return true;
		}
		return false;
	}
};
