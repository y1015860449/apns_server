#ifndef __BUFFER_H__
#define __BUFFER_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "basetype.h"

class CSimpleBuffer
{
public:
	CSimpleBuffer();
	~CSimpleBuffer();
	uchar_t*  GetBuffer() { return m_buffer; }
	uint32_t GetAllocSize() { return m_alloc_size; }
	uint32_t GetWriteOffset() { return m_write_offset; }
	void IncWriteOffset(uint32_t len) { m_write_offset += len; }

	void Extend(uint32_t len);
	uint32_t Write(void* buf, uint32_t len);
	uint32_t Read(void* buf, uint32_t len);
private:
	uchar_t*	m_buffer;
	uint32_t	m_alloc_size;
	uint32_t	m_write_offset;
};

#endif