all: dns_cache dns_cache_sharded dns_cache_tbb

dns_cache: dns_cache.cpp lru_cache.h Makefile
	$(CXX) -o$@ -std=c++11 -pthread -lpthread dns_cache.cpp
dns_cache_sharded: dns_cache_sharded.cpp lru_cache.h Makefile
	$(CXX) -o$@ -std=c++11 -pthread -lpthread dns_cache_sharded.cpp
dns_cache_tbb: dns_cache_tbb.cpp lru_cache.h Makefile
	$(CXX) -o$@ -std=c++11 -pthread -lpthread -ltbb dns_cache_tbb.cpp
