#pragma once

#include <ctime>

class Tick {
private:
	time_t when, period;
	bool once, triggered;

public:
	Tick(int period, bool once) {
		this->period = period;
		this->once = once;
		triggered = false;
	}

	void start() {
		when = time(NULL) + period;
		triggered = false;
	}

	bool is() {
		if (once && triggered)
			return true;

		time_t now = time(NULL);
		if(now >= when) {
			when = when + period;
			if(now >= when) {
				when = now;
			}
			if(once && !triggered) {
				triggered = true;
			}
			return true;
		}
		return false;
	}
};
