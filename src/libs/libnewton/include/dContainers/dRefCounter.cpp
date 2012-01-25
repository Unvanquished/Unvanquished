#include "dContainersStdAfx.h"
#include "dRefCounter.h"

dRefCounter::dRefCounter(void)
{
	m_refCount = 1;
}

dRefCounter::~dRefCounter(void)
{
}


int dRefCounter::GetRef() const
{
	return m_refCount;
}

int dRefCounter::Release()
{
	m_refCount --;
	if (!m_refCount) {
		delete this;
		return 0;
	}
	return m_refCount;
}

void dRefCounter::AddRef()
{
	m_refCount ++;
}
