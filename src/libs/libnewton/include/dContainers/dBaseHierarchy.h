//********************************************************************
// Newton Game dynamics 
// copyright 2000-2004
// By Julio Jerez
// VC: 6.0
// basic Hierarchical container Class  
//********************************************************************

#ifndef __Hierarchy__
#define __Hierarchy__

#include "dCRC.h"

#define D_NAME_STRING_LENGTH 128

#if (_MSC_VER >= 1400)
	#pragma warning (disable: 4996) // for 2005 users declared deprecated
#endif


class dBaseHierarchy
{
	public:
	dBaseHierarchy *GetChild () const;
	dBaseHierarchy *GetParent () const;
	dBaseHierarchy *GetSibling () const;

	void Detach ();
	void Attach (dBaseHierarchy *parent, bool addFirst = false);
	
	dBaseHierarchy *GetRoot () const;
	dBaseHierarchy *GetFirst() const;
	dBaseHierarchy *GetLast() const;
	dBaseHierarchy *GetNext() const;
	dBaseHierarchy *GetPrev() const;

	dBaseHierarchy *Find (unsigned nameCRC) const; 
	dBaseHierarchy *Find (const char *name) const;


	unsigned GetNameID() const;
	const char* GetName() const;
	void SetNameID(const char* name);
	


	protected:
	dBaseHierarchy ();
	dBaseHierarchy (const char *name);
	dBaseHierarchy (const dBaseHierarchy &clone);
	virtual ~dBaseHierarchy ();

//	virtual void CloneFixUp (const dBaseHierarchy &clone);

	private:
	inline void Clear();

	dBaseHierarchy *m_parent;
	dBaseHierarchy *m_child;
	dBaseHierarchy *m_sibling;

	unsigned m_nameID;
	char m_name[D_NAME_STRING_LENGTH];

};

template<class T>
class dHierarchy: public dBaseHierarchy
{
	public:
	dHierarchy ();
	dHierarchy (const char *name);
	void Attach (T *parent, bool addFirst = false);
	void Detach ();
	T *GetChild () const;
	T *GetParent () const;
	T *GetSibling () const;
	T *GetRoot () const;
	T *GetFirst() const;
	T *GetLast() const;
	T *GetNext() const;
	T *GetPrev() const;
	T *Find (unsigned nameCRC) const;
	T *Find (const char *name) const;

	protected:
	dHierarchy (const T &clone);
	virtual ~dHierarchy ();
	dBaseHierarchy *CreateClone () const;
};




inline dBaseHierarchy::dBaseHierarchy ()
{
	Clear ();
}

inline dBaseHierarchy::dBaseHierarchy (const char *name)
{
	Clear ();
	SetNameID (name);
}


inline void dBaseHierarchy::Clear()
{
	m_child = NULL;
	m_parent = NULL;
	m_sibling = NULL;
	m_nameID = 0;
	m_name[0] = 0;
}


inline dBaseHierarchy *dBaseHierarchy::GetChild () const
{
	return m_child;
}

inline dBaseHierarchy *dBaseHierarchy::GetSibling () const
{
	return m_sibling;
}

inline dBaseHierarchy *dBaseHierarchy::GetParent () const
{
	return m_parent;
}


inline dBaseHierarchy *dBaseHierarchy::Find (const char *name) const
{
	return Find (dCRC (name)); 
} 

inline void dBaseHierarchy::SetNameID(const char* name)
{
	m_nameID = dCRC (name);
	strcpy (m_name, name);
}

inline unsigned dBaseHierarchy::GetNameID() const
{
	return m_nameID;
}

inline const char* dBaseHierarchy::GetName() const
{
	return m_name;
}


template<class T>
dHierarchy<T>::dHierarchy ()
	:dBaseHierarchy ()
{
}

template<class T>
dHierarchy<T>::dHierarchy (const T &clone)
	:dBaseHierarchy (clone)
{
}

template<class T>
dHierarchy<T>::dHierarchy (const char *name)
	:dBaseHierarchy (name)
{
}

template<class T>
dHierarchy<T>::~dHierarchy () 
{
}


template<class T>
dBaseHierarchy *dHierarchy<T>::CreateClone () const
{
	return new T (*(T*)this);
}

template<class T>
void dHierarchy<T>::Attach (T *parent, bool addFirst)
{
	dBaseHierarchy::Attach(parent, addFirst);
}

template<class T>
void dHierarchy<T>::Detach ()
{
	dBaseHierarchy::Detach ();
}

template<class T>
T *dHierarchy<T>::GetChild () const
{
	return (T*) dBaseHierarchy::GetChild();
}

template<class T>
T *dHierarchy<T>::GetSibling () const
{
	return (T*) dBaseHierarchy::GetSibling ();
}

template<class T>
T *dHierarchy<T>::GetParent () const
{
	return (T*) dBaseHierarchy::GetParent ();
}


template<class T>
T *dHierarchy<T>::GetRoot () const
{
	return (T*) dBaseHierarchy::GetRoot ();
}


template<class T>
T *dHierarchy<T>::GetFirst() const
{
	return (T*) dBaseHierarchy::GetFirst ();
}

template<class T>
T *dHierarchy<T>::GetLast() const
{
	return (T*) dBaseHierarchy::GetLast ();
}


template<class T>
T *dHierarchy<T>::GetNext() const
{
	return (T*) dBaseHierarchy::GetNext ();
}

template<class T>
T *dHierarchy<T>::GetPrev() const
{
	return (T*) dBaseHierarchy::GetPrev ();
}


template<class T>
T *dHierarchy<T>::Find (unsigned nameCRC) const 
{
	return (T*) dBaseHierarchy::Find (nameCRC);
}

template<class T>
T *dHierarchy<T>::Find (const char *name) const
{
	return (T*) dBaseHierarchy::Find (name);
} 


#endif
