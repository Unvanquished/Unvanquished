//********************************************************************
// Newton Game dynamics 
// copyright 2000-2004
// By Julio Jerez
// VC: 6.0
// simple demo list vector class with iterators
//********************************************************************

#ifndef __dTree__
#define __dTree__

#include <dContainersStdAfx.h>
#include <stdlib.h>


// Note: this is a low level class for dTree use only
// unpredictable result will happen if you attempt to manipulate
// any member of this class
class dRedBackNode
{
	public:
	enum REDBLACK_COLOR
	{
		RED = true,
		BLACK = false
	};

	public:
	dRedBackNode *GetLeft() const;
	dRedBackNode *GetRight() const;
	dRedBackNode *GetParent() const;
	dRedBackNode (dRedBackNode *parent);
	dRedBackNode *Prev() const;
	dRedBackNode *Next() const;
	dRedBackNode *Minimum() const;
	dRedBackNode *Maximum() const;

	void *operator new (size_t size) 
	{
		//return m_allocator->malloc(size_t);
		return malloc (size) ;
	}

	void operator delete (void *ptr) 
	{
		#ifdef	_DEBUG
		//memset (ptr, 0xcc, m_size);
		#endif
		free (ptr);
	}


	protected:
	~dRedBackNode () 
	{
	}

	void Initdata (dRedBackNode *parent);
	void SetColor (bool color);
	bool GetColor () const;
	bool IsInTree () const;
	void SetInTreeFlag (bool flag);
	void RemoveAllLow ();
	void RotateLeft(dRedBackNode **head); 
	void RotateRight(dRedBackNode **head); 
	void RemoveFixup (dRedBackNode *node, dRedBackNode **head); 
	void RemoveAll ();
	void Remove (dRedBackNode **head);
	void InsertFixup(dRedBackNode **head); 

	bool m_color;
	bool m_inTree;
	dRedBackNode *m_left;
	dRedBackNode *m_right;
	dRedBackNode *m_parent;

};

template<class OBJECT, class KEY>
class dTree 
{
	public:
	class dTreeNode: public dRedBackNode
	{
		~dTreeNode () 
		{
		}
	
		dTreeNode (
			const OBJECT &info, 
			const KEY &key, 
			dTreeNode *parentNode)
			:dRedBackNode(parentNode), m_info (info), m_key (key)
		{
		}
/*
		void *operator new (size_t size) 
		{
			//return m_allocator->malloc(size_t);
			return malloc (size) ;
		}

		void operator delete (void *ptr) 
		{
			#ifdef	_DEBUG
			//memset (ptr, 0xcc, m_size);
			#endif
			//m_allocator->free(ptr);
			free (ptr);
		}
*/
		dTreeNode *GetLeft () const
		{
			return (dTreeNode *)dRedBackNode::m_left;
		}

		dTreeNode *GetRight () const
		{
			return (dTreeNode *)dRedBackNode::m_right;
		}

		dTreeNode *GetParent ()
		{
			return (dTreeNode *)dRedBackNode::m_parent;
		}

		void SetLeft (dTreeNode *node)
		{
			dRedBackNode::m_left = node;
		}

		void SetRight (dTreeNode *node)
		{
			dRedBackNode::m_right = node;
		}

		void SetParent (dTreeNode *node)
		{
			dRedBackNode::m_parent = node;
		}

		public:
		const KEY& GetKey() const
		{
			return m_key;
		}

		OBJECT& GetInfo()
		{
			return m_info;
		}

		private:
		OBJECT m_info;
		KEY m_key; 
		friend class dTree<OBJECT, KEY>;
	};

	class Iterator
	{
		dRedBackNode *m_ptr;
		const dTree *m_tree;

		public:
		Iterator(const dTree<OBJECT,KEY> &me)
		{
			m_ptr = NULL;
			m_tree = &me;
		}

		~Iterator()
		{
		}

		void Begin() 
		{
			m_ptr = m_tree->Minimum();
		}

		void End()  
		{
			m_ptr = m_tree->Maximum();
		}

		void Set (dTreeNode *node)
		{
			m_ptr = node;
		}

		operator int() const 
		{
			return m_ptr != NULL;
		}

		void operator++ ()
		{
			//_ASSERTE (m_ptr);
			m_ptr = m_ptr->Next();
		}

		void operator++ (int)
		{
			//_ASSERTE (m_ptr);
			m_ptr = m_ptr->Next();
		}

		void operator-- () 
		{
			//_ASSERTE (m_ptr);
			m_ptr = m_ptr->Prev();
		}

		void operator-- (int) 
		{
			//_ASSERTE (m_ptr);
			m_ptr = m_ptr->Prev();
		}

		OBJECT &operator* () const 
		{
			return ((dTreeNode*)m_ptr)->GetInfo();
		}

		dTreeNode *GetNode() const
		{
			return (dTreeNode*)m_ptr;
		}

		KEY GetKey () const
		{
			dTreeNode* tmp;

			tmp = (dTreeNode*)m_ptr;
			return tmp ? tmp->GetKey() : KEY(0);
		}
	};


	// ***********************************************************
	// member funstions
	// ***********************************************************
	public:
	dTree ();
	~dTree (); 

	void* operator new (size_t size);
	void operator delete (void *ptr);

	operator int() const;
	int GetCount() const;

	dTreeNode *GetRoot () const;
	dTreeNode *Minimum () const;
	dTreeNode *Maximum () const;

	dTreeNode *Find (KEY key) const;
	dTreeNode *FindGreater (KEY key) const;
	dTreeNode *FindGreaterEqual (KEY key) const;
	dTreeNode *FindLessEqual (KEY key) const;

	dTreeNode *GetNodeFromInfo (OBJECT &info) const;

	dTreeNode *Insert (const OBJECT &element, KEY key, bool& elementWasInTree);
	dTreeNode *Insert (const OBJECT &element, KEY key);
	dTreeNode *Insert (dTreeNode *node, KEY key);

	dTreeNode *Replace (OBJECT &element, KEY key);
	dTreeNode *ReplaceKey (KEY oldKey, KEY newKey);
	dTreeNode *ReplaceKey (dTreeNode *node, KEY key);

	void Remove (KEY key);
	void Remove (dTreeNode *node);
	void RemoveAll (); 

	bool SanityCheck () const;


	// ***********************************************************
	// member variables
	// ***********************************************************
	private:
	int m_count;
	dTreeNode *m_head;
//	static int m_size;

	int CompareKeys (const KEY &key0, const KEY &key1) const;
	bool SanityCheck (dTreeNode *ptr, int height) const;

	friend class dTreeNode;
};


inline dRedBackNode::dRedBackNode (dRedBackNode *parent)
{
	Initdata (parent);
}

inline void dRedBackNode::Initdata (dRedBackNode *parent)
{
	SetColor (RED);
	SetInTreeFlag (true);
	m_left = NULL;
	m_right = NULL;
	m_parent = parent;
}

inline void dRedBackNode::SetColor (bool color)
{
	m_color = color;
}

inline bool dRedBackNode::GetColor () const
{
	return m_color;
}

inline bool dRedBackNode::IsInTree () const
{
	return m_inTree;
}

inline void dRedBackNode::SetInTreeFlag (bool flag)
{
	m_inTree = flag;
}

template<class OBJECT, class KEY>
dTree<OBJECT, KEY>::dTree ()
{
	m_count	= 0;
	m_head = NULL;
}


template<class OBJECT, class KEY>
dTree<OBJECT, KEY>::~dTree () 
{
	RemoveAll();
}


template<class OBJECT, class KEY>
void* dTree<OBJECT, KEY>::operator new (size_t size)
{
	return malloc (size);
}

template<class OBJECT, class KEY>
void dTree<OBJECT, KEY>::operator delete (void *ptr)
{
	free (ptr);
}


template<class OBJECT, class KEY>
dTree<OBJECT, KEY>::operator int() const
{
	return m_head != NULL;
}

template<class OBJECT, class KEY>
int dTree<OBJECT, KEY>::GetCount() const
{
	return m_count;
}

template<class OBJECT, class KEY>
typename dTree<OBJECT, KEY>::dTreeNode *dTree<OBJECT, KEY>::Minimum () const
{
	return m_head ? (dTreeNode *)m_head->Minimum() : NULL;
}

template<class OBJECT, class KEY>
typename dTree<OBJECT, KEY>::dTreeNode *dTree<OBJECT, KEY>::Maximum () const
{
	return m_head ? (dTreeNode *)m_head->Maximum() : NULL;
}

template<class OBJECT, class KEY>
typename dTree<OBJECT, KEY>::dTreeNode *dTree<OBJECT, KEY>::GetRoot () const
{
	return m_head;
}

template<class OBJECT, class KEY>
typename dTree<OBJECT, KEY>::dTreeNode *dTree<OBJECT, KEY>::Find (KEY key) const
{
	int val;
	dTreeNode *ptr;

	if (m_head == NULL) {
		return NULL;
	}

	ptr = m_head;
	while (ptr != NULL) {
		val = CompareKeys (ptr->m_key, key);
		if (!val) {
			break;
		}
		if (val < 0) {
			ptr = ptr->GetLeft();
		} else {
			ptr = ptr->GetRight();
		}
	}
	return ptr;
}

template<class OBJECT, class KEY>
typename dTree<OBJECT, KEY>::dTreeNode *dTree<OBJECT, KEY>::GetNodeFromInfo (OBJECT &info) const
{
	int offset;
	dTreeNode *node;

	node = (dTreeNode *) &info;
	offset = ((char*) &node->m_info) - ((char *) node);
	node = (dTreeNode *) (((char *) node) - offset);

//	_ASSERTE (node->IsInTree ());
	_ASSERTE (&node->GetInfo () == &info);
	return (node->IsInTree ()) ? node : NULL;
}

template<class OBJECT, class KEY>
typename dTree<OBJECT, KEY>::dTreeNode *dTree<OBJECT, KEY>::FindGreater (KEY key) const
{
	int val;
	dTreeNode *ptr;
	dTreeNode *prev;

	if (m_head == NULL) {
		return NULL;
	}

	prev = NULL;
	ptr = m_head;
	val = 0;
	while (ptr != NULL) {
		val = CompareKeys (ptr->m_key, key);
		if (!val) {
			return (dTreeNode *)ptr->Next();
		}
		prev = ptr;
		if (val < 0) {
			ptr = ptr->GetLeft();
		} else {
			ptr = ptr->GetRight();
		}
	}

	if (val > 0) {
		while (prev->m_parent && (prev->m_parent->m_right == prev)) {
			prev = prev->GetParent(); 
		}
		prev = prev->GetParent(); 
	}
	return (dTreeNode *)prev; 
}

template<class OBJECT, class KEY>
typename dTree<OBJECT, KEY>::dTreeNode *dTree<OBJECT, KEY>::FindGreaterEqual (KEY key) const
{
	int val;
	dTreeNode *ptr;
	dTreeNode *prev;

	if (m_head == NULL) {
		return NULL;
	}

	prev = NULL;
	ptr = m_head;
	val = 0;
	while (ptr != NULL) {
		val = CompareKeys (ptr->m_key, key);
		if (!val) {
			return ptr;
		}
		prev = ptr;
		if (val < 0) {
			ptr = ptr->GetLeft();
		} else {
			ptr = ptr->GetRight();
		}
	}

	if (val > 0) {
		while (prev->m_parent && (prev->m_parent->m_right == prev)) {
			prev = prev->GetParent(); 
		}
		prev = prev->GetParent(); 
	}
	return (dTreeNode *)prev; 
}

template<class OBJECT, class KEY>
typename dTree<OBJECT, KEY>::dTreeNode *dTree<OBJECT, KEY>::FindLessEqual (KEY key) const
{
	int val;
	dTreeNode *ptr;
	dTreeNode *prev;

	if (m_head == NULL) {
		return NULL;
	}

	prev = NULL;
	ptr = m_head;
	val = 0;
	while (ptr != NULL) {
		val = CompareKeys (ptr->m_key, key);
		if (!val) {
			return ptr;
		}
		prev = ptr;
		if (val < 0) {
			ptr = ptr->GetLeft();
		} else {
			ptr = ptr->GetRight();
		}
	}

	if (val < 0) {
		while (prev->m_parent && (prev->m_parent->m_left == prev)) {
			prev = prev->GetParent(); 
		}
		prev = prev->GetParent(); 
	}
	return (dTreeNode *)prev; 
}

template<class OBJECT, class KEY>
typename dTree<OBJECT, KEY>::dTreeNode *dTree<OBJECT, KEY>::Insert (const OBJECT &element, KEY key, bool& elementWasInTree)
{
	int val;
	dTreeNode *ptr;
	dTreeNode *parent;

	parent = NULL;
	ptr = m_head;
	val = 0;
	elementWasInTree = false;
	while (ptr != NULL) {
		parent = ptr;
		val = CompareKeys (ptr->m_key, key);

		if (val < 0) {
			ptr = ptr->GetLeft();
		} else if (val > 0) {
			ptr = ptr->GetRight();
		} else {
			elementWasInTree = true;
			return ptr;
		}
	}

	m_count	++;
	ptr = new dTreeNode (element, key, parent);
	if (!parent) {
		m_head = ptr;
	} else {
		if (val < 0) {
			parent->m_left = ptr; 
		} else {
			parent->m_right = ptr;
		}
	}
	ptr->InsertFixup ((dRedBackNode**)&m_head);
	return ptr;
}

template<class OBJECT, class KEY>
typename dTree<OBJECT, KEY>::dTreeNode *dTree<OBJECT, KEY>::Insert (const OBJECT &element, KEY key)
{
	bool foundState;
	dTreeNode *node;

	node = Insert (element, key, foundState);
	if (foundState) {
		node = NULL;
	}
	return node;
}

template<class OBJECT, class KEY>
typename dTree<OBJECT, KEY>::dTreeNode *dTree<OBJECT, KEY>::Insert (typename dTree<OBJECT, KEY>::dTreeNode *node, KEY key)
{
	int val;
	dTreeNode *ptr;
	dTreeNode *parent;

	val = 0;
	ptr = m_head;
	parent = NULL;
	while (ptr != NULL) {
		parent = ptr;
		val = CompareKeys (ptr->m_key, key);

		if (val < 0) {
			ptr = ptr->GetLeft();
		} else if (val > 0) {
			ptr = ptr->GetRight();
		} else {
			return NULL;
		}
	}

	m_count	++;

	ptr = node;
	ptr->m_key = key;
	ptr->Initdata (parent);

	if (!parent) {
		m_head = ptr;
	} else {
		if (val < 0) {
			parent->m_left = ptr; 
		} else {
			parent->m_right = ptr;
		}
	}
	ptr->InsertFixup ((dRedBackNode**)&m_head);
	return ptr;
}

template<class OBJECT, class KEY>
typename dTree<OBJECT, KEY>::dTreeNode *dTree<OBJECT, KEY>::Replace (OBJECT &element, KEY key)
{
	int val;
	dTreeNode *ptr;
	dTreeNode *parent;

	parent = NULL;
	ptr = m_head;
	val = 0;
	while (ptr != NULL) {
		parent = ptr;
		val = CompareKeys (ptr->m_key, key);
		if (val == 0) {
			ptr->m_info = element;
			return ptr;
		}
		if (val < 0) {
			ptr = ptr->GetLeft();
		} else {
			ptr = ptr->GetRight();
		}
	}

	ptr = new dTreeNode (element, key, parent);
	if (!parent) {
		m_head = ptr;
	} else {
		if (val < 0) {
			parent->m_left = ptr; 
		} else {
			parent->m_right = ptr;
		}
	}
	ptr->InsertFixup ((dRedBackNode**)&m_head);
	return ptr;
}

template<class OBJECT, class KEY>
typename dTree<OBJECT, KEY>::dTreeNode *dTree<OBJECT, KEY>::ReplaceKey (typename dTree<OBJECT, KEY>::dTreeNode *node, KEY key)
{
	dTreeNode *ptr;

	_ASSERTE (node->IsAlive());
	Remove (node);
	node->Unkill();
	ptr = Insert (node, key);

	_ASSERTE (ptr);
	return ptr;
}

template<class OBJECT, class KEY>
typename dTree<OBJECT, KEY>::dTreeNode *dTree<OBJECT, KEY>::ReplaceKey (KEY oldKey, KEY newKey)
{
	dTreeNode *node;

	node = Find (oldKey);
	return node ? ReplaceKey (node, newKey) : NULL;
}

template<class OBJECT, class KEY>
void dTree<OBJECT, KEY>::Remove (typename dTree<OBJECT, KEY>::dTreeNode *node)
{
	m_count	--;
	node->Remove ((dRedBackNode **)&m_head);
}

template<class OBJECT, class KEY>
void dTree<OBJECT, KEY>::Remove (KEY key) 
{
	dTreeNode *node;

	// find node in tree 
	node = Find (key);
	if (node == NULL) {
		return;
	}
	Remove (node);
}

template<class OBJECT, class KEY>
void dTree<OBJECT, KEY>::RemoveAll () 
{
	if (m_head) {
		m_count	 = 0;
		m_head->RemoveAll ();
		m_head = NULL;
	}
}

template<class OBJECT, class KEY>
bool dTree<OBJECT, KEY>::SanityCheck () const
{
	return SanityCheck (m_head, 0);
}


template<class OBJECT, class KEY>
bool dTree<OBJECT, KEY>::SanityCheck (typename dTree<OBJECT, KEY>::dTreeNode *ptr, int height) const
{
	int bh;
	dTreeNode *x;

	if (!ptr) {
		return true;
	}

	if (ptr->m_left) {
		if (CompareKeys (ptr->m_key, ptr->GetLeft()->m_key) > 0) {
			return false;
		}
	}

	if (ptr->m_right) {
		if (CompareKeys (ptr->m_key, ptr->GetRight()->m_key) < 0) {
			return false;
		}
	}

	if (ptr->GetColor() == dTreeNode::BLACK) {
		height ++;
	} else if (!((!ptr->m_left  || (ptr->m_left->GetColor() == dTreeNode::BLACK)) &&
			       (!ptr->m_right || (ptr->m_right->GetColor() == dTreeNode::BLACK)))) {
	  	return false;
	}

	if (!ptr->m_left && !ptr->m_right) {
		bh = 0;
		for (x = ptr; x; x = x->GetParent()) {
	 		if (x->GetColor() == dTreeNode::BLACK) {
				bh ++;
			}
		}
		if (bh != height) {
			return false;
		}
	}

	if (ptr->m_left && !SanityCheck (ptr->GetLeft(), height)) {
		return false;
	}

	if (ptr->m_right && !SanityCheck (ptr->GetRight(), height)) {
		return false;
	}
	return true;
}

template<class OBJECT, class KEY>
int dTree<OBJECT, KEY>::CompareKeys (const KEY &key0, const KEY &key1) const
{
	if (key1 < key0) {
		return - 1;
	}
	if (key1 > key0) {
		return 1;
	}
	return 0;
}

//template<class OBJECT, class KEY> int dTree<OBJECT,KEY>::m_size = 0;
//template<class OBJECT, class KEY> dgMemoryAllocator* dTree<OBJECT,KEY>::m_allocator = NULL;


#endif

