#ifndef __LRU_CACHE_H__
#define __LRU_CACHE_H__

#include <list>
#include <unordered_map>

template<
	typename Key,
	typename Value,
	// lets allow to change hasher and equal predicate
	typename Hash = std::hash<Key>,
	typename Pred = std::equal_to<Key>
>
class lru_cache_t
{
public:
	typedef Key key_type;
	typedef Value value_type;
	typedef Hash hasher_type;
	typedef Pred predicate_type;
	typedef typename std::pair<key_type, value_type> pair_type;
	typedef typename std::list<pair_type>::iterator iterator;
	typedef typename std::list<pair_type>::const_iterator const_iterator;
	//
	lru_cache_t(size_t max_size): _max_size(max_size)
	{
	}
	void set(const key_type &key, const value_type &value)
	{
		auto it = _mapper.find(key);
		if (it != _mapper.end())
		{
			auto lit = (*it).second;
			// move to the head
			_elements.splice(_elements.begin(), _elements, lit);
			// set new value
			(*lit).second = value;
		}
		else
		{
			displace_if();
			//
			_elements.emplace_front(key, value);
			auto lit = _elements.begin();
			_mapper.insert(std::make_pair(key, lit));
		}
	}
	value_type& operator[](const key_type &key)
	{
		auto it = _mapper.find(key);
		if (it != _mapper.end())
		{
			auto lit = (*it).second;
			// move to the head
			_elements.splice(_elements.begin(), _elements, lit);
			//
			return (*lit).second;
		}
		else
		{
			displace_if();
			//
			_elements.emplace_front(key, value_type());
			auto lit = _elements.begin();
			_mapper.insert(std::make_pair(key, lit));
			return (*lit).second;
		}
	}
	bool get(const key_type &key, value_type &value)
	{
		auto it = _mapper.find(key);
		if (it != _mapper.end())
		{
			auto lit = (*it).second;
			// move to the head
			_elements.splice(_elements.begin(), _elements, lit);
			value = (*lit).second;
			return true;
		}
		return false;
	}
	bool get(const key_type &key, value_type &value) const
	{
		auto it = _mapper.find(key);
		if (it != _mapper.end())
		{
			auto lit = (*it).second;
			value = (*lit).second;
			return true;
		}
		return false;
	}
	void erase(const key_type &key)
	{
		auto it = _mapper.find(key);
		if (it != _mapper.end())
		{
			auto lit = (*it).second;
			_elements.erase(lit);
			_mapper.erase(it);
		}
	}
	// iterators
	iterator begin()
	{
		return _elements.begin();
	}
	iterator end()
	{
		return _elements.end();
	}
	const_iterator begin() const
	{
		return _elements.begin();
	}
	const_iterator end() const
	{
		return _elements.end();
	}
	// 
	size_t size() const
	{
		return _elements.size();
	}
	size_t max_size() const
	{
		return _max_size;
	}
	size_t empty() const
	{
		return _elements.empty();
	}

private:
	bool displace_if()
	{
		if (_elements.size() >= _max_size)
		{
			auto &pair = _elements.back();
			_mapper.erase(pair.first);
			_elements.pop_back();
			return true;
		}
		return false;
	}

private:
	size_t _max_size;
	std::list<pair_type> _elements;
	std::unordered_map<key_type, iterator, hasher_type, predicate_type> _mapper;
};

#endif // __LRU_CACHE_H__
