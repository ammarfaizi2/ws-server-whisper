#ifndef WHISPER_CHANNEL_HPP
#define WHISPER_CHANNEL_HPP

#include <mutex>
#include <queue>
#include <string>
#include <vector>
#include <condition_variable>

struct whisper_channel {
	std::mutex mtx;
	std::queue<std::vector<float>> q;
	std::condition_variable cv;
	volatile bool stop_flag;

	void produce(const std::vector<float> &pcm)
	{
		std::unique_lock<std::mutex> lock(mtx);
		q.push(pcm);
		cv.notify_one();
	}

	bool consume(std::vector<float> &pcm)
	{
		std::unique_lock<std::mutex> lock(mtx);
		cv.wait(lock, [this] { return !q.empty() || stop_flag; });

		if (stop_flag || q.empty())
			return false;

		pcm.clear();
		for (auto &v : q.front())
			pcm.push_back(v);

		q.pop();
		return true;
	}

	void stop(void)
	{
		std::unique_lock<std::mutex> lock(mtx);
		stop_flag = true;
		cv.notify_one();
	}
};

#endif // WHISPER_CHANNEL_HPP
