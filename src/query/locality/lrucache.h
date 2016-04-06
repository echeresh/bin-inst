#pragma once
#include <cassert>
#include <list>
#include <unordered_map>
#include <utility>

template <class key_type, class value_type = int>
class LRUCache
{
    typedef typename std::list<std::pair<key_type, value_type>>::iterator list_iterator_t;

    std::unordered_map<key_type, list_iterator_t> keyMap;
    std::list<std::pair<key_type, value_type>> keyList;
    size_t maxSize;

public:
    LRUCache(size_t maxSize) : maxSize(maxSize)
    {
    }

    void put(const key_type& key, const value_type& value = value_type())
    {
        auto it = keyMap.find(key);
        if (it != keyMap.end())
        {
            keyList.erase(it->second);
            keyMap.erase(it);
        }
        keyList.push_front(std::make_pair(key, value));
        keyMap[key] = keyList.begin();

        if (keyMap.size() > maxSize)
        {
            auto it = keyList.end();
            it--;
            keyMap.erase(it->first);
            keyList.pop_back();
        }
    }

    value_type contains(const key_type& key) const
    {
        return keyMap.find(key) != keyMap.end();
    }

    size_t size() const
    {
        return keyMap.size();
    }
};
