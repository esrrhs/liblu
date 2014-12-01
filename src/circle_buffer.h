#pragma once

#include "lutypes.h"

/*
type:           [1]
iter:     begin(2)     end(8)
            |           |
data:   _ _ * * * * * * _ _ _ 
buffer: _ _ _ _ _ _ _ _ _ _ _ 
index:  0 1 2 3 4 5 6 7 8 9 10

type:           [2]
iter:      end(2)   begin(7)
            |         |
data:   * * _ _ _ _ _ * * * *  
buffer: _ _ _ _ _ _ _ _ _ _ _ 
index:  0 1 2 3 4 5 6 7 8 9 10


type:           [3]
iter:      begin(4),end(4)
                |
data:   _ _ _ _ _ _ _ _ _ _ _ 
buffer: _ _ _ _ _ _ _ _ _ _ _ 
index:  0 1 2 3 4 5 6 7 8 9 10

type:           [4]
iter:      begin(4),end(4)
|				 |
data:   * * * * * * * * * * *
buffer: _ _ _ _ _ _ _ _ _ _ _
index:  0 1 2 3 4 5 6 7 8 9 10

*/
struct lu;
struct circle_buffer
{
    void ini(lu * l, size_t len)
    {
        m_l = l;
        m_size = len;
        m_buffer = (char*)safelumalloc(m_l, len);
    }
    void fini()
    {   
        safelufree(m_l, m_buffer);
    }
	bool can_write(size_t size)
	{
		return m_datasize + size <= m_size;
	}
	void skip_write(size_t size)
	{
		m_datasize += size;
		m_end += size;
		if (m_end >= m_size)
		{
			m_end -= m_size;
		}
	}
	bool write(const char * p, size_t size)
	{
		if (!can_write(size))
		{
			return false;
		}

		real_write(p, size);

		skip_write(size);

		return true;
	}
	bool can_read(size_t size)
	{
		return m_datasize >= size;
	}
	void skip_read(size_t size)
	{
		m_datasize -= size;
		m_begin += size;
		if (m_begin >= m_size)
		{
			m_begin -= m_size;
		}
	}
	bool read(char * out, size_t size)
	{
		if (!can_read(size))
		{
			return false;
		}

		real_read(out, size);

		skip_read(size);

		return true;
	}
	void store()
	{
		m_store_datasize = m_datasize;
		m_store_begin = m_begin;
		m_store_end = m_end;
	}
	void restore()
	{
		m_datasize = m_store_datasize;
		m_begin = m_store_begin;
		m_end = m_store_end;
	}
	bool clear()
	{
		m_datasize = 0;
		m_begin = 0;
		m_end = 0;
		return true;
	}
	size_t size() const
	{
		return m_datasize;
	}
	size_t capacity() const
	{
		return m_size;
	}
	bool empty() const
	{
		return size() == 0;
	}
	bool full() const
	{
		return size() == capacity();
	}
	char * get_read_line_buffer()
	{
		return m_buffer + m_begin;
	}
	size_t get_read_line_size()
	{
		return LUMIN(m_datasize, m_size - m_begin);
	}
	char * get_write_line_buffer()
	{
		return m_buffer + m_end;
	}
	size_t get_write_line_size()
	{
		return LUMIN(m_size - m_datasize, m_size - m_end);
	}
private:
	void real_write(const char * p, size_t size)
	{
		if (m_end >= m_begin)	// [1][3]
		{
			// 能装下
			if (m_size - m_end >= size)
			{
				memcpy(m_buffer + m_end, p, size * sizeof(char));
			}
			else
			{
				memcpy(m_buffer + m_end, p, (m_size - m_end) * sizeof(char));
				memcpy(m_buffer, p + (m_size - m_end), (size - (m_size - m_end)) * sizeof(char));
			}
		}
		else	//[2]
		{
			memcpy(m_buffer + m_end, p, size * sizeof(char));
		}
	}
	void real_read(char * out, size_t size)
	{
		if (m_begin >= m_end)	// [2][4]
		{
			// 能读完
			if (m_size - m_begin >= size)
			{
				memcpy(out, m_buffer + m_begin, size * sizeof(char));
			}
			else
			{
				memcpy(out, m_buffer + m_begin, (m_size - m_begin) * sizeof(char));
				memcpy(out + (m_size - m_begin), m_buffer, (size - (m_size - m_begin)) * sizeof(char));
			}
		}
		else	// [1]
		{
			memcpy(out, m_buffer + m_begin, size * sizeof(char));
		}
	}
private:
    lu * m_l;
	char * m_buffer;
	size_t m_size;
	size_t m_datasize;
	size_t m_begin;
	size_t m_end;
	size_t m_store_datasize;
	size_t m_store_begin;
	size_t m_store_end;
};

