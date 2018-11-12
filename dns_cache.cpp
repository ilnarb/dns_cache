#include <iostream>
#include <list>
#include <thread>
#include <mutex>
#include <memory>

#include "lru_cache.h"

class DNSCache
{
	struct impl_t
	{
		impl_t(size_t max_size)
			: cache(max_size)
		{}
		//
		void update(const std::string& name, const std::string& ip)
		{
			std::lock_guard<std::mutex> lock(mutex);
			cache.set(name, ip);
		}
		std::string resolve(const std::string& name)
		{
			std::string ip;
			{
				std::lock_guard<std::mutex> lock(mutex);
				cache.get(name, ip);
			}
			return ip;
		}
		//
		lru_cache_t<std::string, std::string> cache;
		std::mutex mutex;
	};
	//
	static void instance(size_t max_size, std::shared_ptr<impl_t> &impl)
	{
		static std::mutex _mutex;
		//
		std::lock_guard<std::mutex> lock(_mutex);
		//
		static std::shared_ptr<impl_t> _impl;
		if (!_impl)
		{
			_impl = std::make_shared<impl_t>(max_size);
		}
		impl = _impl;
	}
	// in order to eliminate frequent calls to instance(...)
	std::shared_ptr<impl_t> _impl;
public:
	explicit DNSCache(size_t max_size)
	{
		instance(max_size, _impl);
	}
	void update(const std::string& name, const std::string& ip)
	{
		_impl->update(name, ip);
	}
	std::string resolve(const std::string& name)
	{
		return _impl->resolve(name);
	}
};

namespace
{
	std::string to_string(int num)
	{
		bool bneg = false;
		if (num < 0)
		{
			num = -num;
			bneg = true;
		}

		char buff[256];
		int pos = sizeof(buff);
		buff[--pos] = 0;

		do
		{
			int mod = num % 10;
			buff[--pos] = '0' + mod;
			num /= 10;
		}
		while(num > 0 && pos > 0);

		if (bneg) buff[--pos] = '-';
		return (buff + pos);
	}
};

int main(int argc, char* argv[])
{
	int num_threads = 1;
	if (argc > 1)
		num_threads = atoi(argv[1]);

	const size_t max_size = 1000;
	// generate test strings
	const size_t max_strings = 3 * max_size;
	std::string strings[max_strings];
	for(int i = 0; i < max_strings; i++)
	{
		strings[i] = to_string(i);
	}

	std::list<std::thread *> threads;
	for(int k = 0; k < num_threads; k++)
	{
		std::thread *th = new(std::nothrow) std::thread([&](){
			DNSCache cache(max_size);
			for(int i = 0; i < 100000; i++)
			{
				cache.update(strings[i % max_strings], strings[i % max_strings]);
				auto ip = cache.resolve(strings[(i + max_size/3) % max_strings]);
			}
		});
		threads.push_back(th);
	}

	while(!threads.empty())
	{
		std::thread *th = threads.front();
		if (th) th->join();
		delete th;
		threads.pop_front();
	}

	return 0;
}
