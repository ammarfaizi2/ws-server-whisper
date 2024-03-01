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

	void produce(const std::vector<float> &pcm)
	{
		std::unique_lock<std::mutex> lock(mtx);
		q.push(pcm);
		cv.notify_one();
	}

	void consume(std::vector<float> &pcm)
	{
		std::unique_lock<std::mutex> lock(mtx);
		cv.wait(lock, [this]{ return !q.empty(); });
		pcm = q.front();
		q.pop();
	}
};

#endif // WHISPER_CHANNEL_HPP
