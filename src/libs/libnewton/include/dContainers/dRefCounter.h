#pragma once

class dRefCounter
{
	public:
	dRefCounter(void);
	int GetRef() const;
	int Release();
	void AddRef();

	protected:
	virtual ~dRefCounter(void);

	private:
	int m_refCount;
};
