#pragma once

class NBMQTTCountdown
{
public:
	NBMQTTCountdown()
    {
		init();
    }

	NBMQTTCountdown(int ms)
    {
		init();
		countdown_ms(ms);
    }

    ~NBMQTTCountdown() {
    }

    bool expired()
    {
    	return (left_ms() == 0);
    }


    void countdown_ms(int ms)
    {
    	startTime = static_cast<int>((TimeTick/TICKS_PER_SECOND) * 1000); // convert to ms
    	delay = ms;
    }


    void countdown(int seconds)
    {
    	countdown_ms(seconds * 1000);
    }


    int left_ms()
    {
    	int time = static_cast<int>((TimeTick/TICKS_PER_SECOND) * 1000); // convert to ms
    	return ((time-startTime) > delay) ? 0 : delay - (time-startTime);
    }

private:

    void init()
    {
    	delay = 0;
    	startTime = TimeTick;
    }

    uint32_t delay;
    uint32_t startTime;
};
