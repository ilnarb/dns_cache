#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <memory>

#include "lru_cache.h"

#include <tbb/queuing_mutex.h>
typedef tbb::queuing_mutex mutex_type;

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
			mutex_type::scoped_lock lock(mutex);
			cache.set(name, ip);
		}
		std::string resolve(const std::string& name)
		{
			std::string ip;
			{
				mutex_type::scoped_lock lock(mutex);
				cache.get(name, ip);
			}
			return ip;
		}
		//
		lru_cache_t<std::string, std::string> cache;
		mutex_type mutex;
	};
	// shard whole cache into pieces in order to reduce lock contention
	const static int shards_count = 8;
	// set local instance under lock
	static void instance(size_t max_size, int shard, std::shared_ptr<impl_t> &impl)
	{
		static mutex_type _mutex;
		//
		mutex_type::scoped_lock lock(_mutex);
		//
		static std::shared_ptr<impl_t> _impl[shards_count];
		if (!_impl[shard])
		{
			_impl[shard] = std::make_shared<impl_t>(max_size);
		}
		impl = _impl[shard];
	}
	// make local copy in order to eliminate frequent calls to instance(...)
	std::shared_ptr<impl_t> _impl[shards_count];
	// shard uqing FNV32 hash
	std::shared_ptr<impl_t> get_shard(const std::string& name)
	{
		const unsigned FNV_32_PRIME = 0x01000193;
		unsigned int hval = 0x811c9dc5; // FNV0 hval = 0
		for(size_t i = 0, size = name.size(); i < size; i++)
		{
			hval *= FNV_32_PRIME;
			hval ^= (unsigned int) name[i];
		}
		//
		return _impl[hval % shards_count];
	}
public:
	explicit DNSCache(size_t max_size)
	{
		double size = 0, delta = 1. * max_size / shards_count;
		for(int i = 0; i < shards_count; i++)
		{
			int piece = int(size + delta) - int(size);
			if (piece < 1) piece = 1;
			size += delta;
			//
			instance(piece, i, _impl[i]);
		}
	}
	void update(const std::string& name, const std::string& ip)
	{
		get_shard(name)->update(name, ip);
	}
	std::string resolve(const std::string& name)
	{
		return get_shard(name)->resolve(name);
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
		strings[i] = std::to_string(i);
	}

	std::vector<std::thread> threads;
	for(int k = 0; k < num_threads; k++)
	{
		threads.push_back(std::thread([&](){
			DNSCache cache(max_size);
			for(int i = 0; i < 100000; i++)
			{
				cache.update(strings[i % max_strings], strings[i % max_strings]);
				auto ip = cache.resolve(strings[(i + max_size/3) % max_strings]);
			}
		});
	}

	for(auto &th : threads)
	{
		th.join();
	}

	return 0;
}
