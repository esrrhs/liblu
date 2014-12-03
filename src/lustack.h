#pragma once

#include "lutypes.h"

struct lu;
template <typename T>
struct lustack
{
    void ini(lu * _l, size_t len)
    {
        l = _l;
        N = len;
        data = (T*)safelumalloc(l, sizeof(T) * len);
		used = 0;
    }

    void fini()
    {
        safelufree(l, data);
    }
    
	void clear()
	{
		used = 0;
	}

	bool pop(T & t)
	{
		if (empty())
		{
			return false;
		}
		t = data[used - 1];
		used--;
		return true;
	}

	bool push(const T & t)
	{
		if (full())
		{
			return false;
		}

		data[used] = t;
		used++;

		return true;
	}

	uint32_t size() const
	{
		return used;
	}

	bool empty() const
	{
		return used == 0;
	}

	bool full() const
	{
		return used == N;
	}
	
	T& operator [](uint32_t index)
	{
		assert(index < N);
		return data[index];
	}

	const T& operator [](uint32_t index) const
	{
		assert(index < N);
		return data[index];
	}

    lu * l;
	T * data;
	size_t N;
	uint32_t used;
};

