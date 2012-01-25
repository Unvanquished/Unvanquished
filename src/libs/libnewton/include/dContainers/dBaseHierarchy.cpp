//********************************************************************
// Newton Game dynamics 
// copyright 2000-2004
// By Julio Jerez
// VC: 6.0
// basic Hierarchical container Class  
//********************************************************************
#include <dContainersStdAfx.h>
#include "dBaseHierarchy.h"

/*
dBaseHierarchy::dBaseHierarchy (const dBaseHierarchy &clone)
{
//	_ASSERTE (0);

	dBaseHierarchy *obj;
	dBaseHierarchy *newObj;

	Clear ();

	for (obj = clone.m_child; obj; obj = obj->m_sibling) {
		newObj = obj->CreateClone ();
		newObj->Attach (this);
		newObj->Release();
	}

}
*/

dBaseHierarchy::~dBaseHierarchy () 
{
/*
	if (m_child) {
		delete m_child;
	}

	if (m_sibling) {
		delete m_sibling;
	}
*/

	Detach();
	while (m_child) {
		delete m_child;
	}
}


void dBaseHierarchy::Attach (dBaseHierarchy *parentArg, bool addFirst)
{
	dBaseHierarchy *obj;
//	_ASSERTE (!m_parent);
//	_ASSERTE (!m_sibling);
//	_ASSERTE (parentArg);
	
	m_parent = parentArg;
	if (m_parent->m_child) {
		if (addFirst) {
			m_sibling = m_parent->m_child;
			m_parent->m_child = this;
		} else {
			for (obj = m_parent->m_child; obj->m_sibling; obj = obj->m_sibling);
			obj->m_sibling = this;
		}
	} else {
		m_parent->m_child = this;
	}
}


void dBaseHierarchy::Detach ()
{
//	_ASSERTE (m_parent || !m_sibling);
 	if (m_parent) {
		if (m_parent->m_child == this) {
			m_parent->m_child = m_sibling;
		} else {
			dBaseHierarchy *ptr;
			for (ptr = m_parent->m_child; ptr->m_sibling != this; ptr = ptr->m_sibling);
			ptr->m_sibling = m_sibling;
		}
		m_parent = NULL;
		m_sibling = NULL;
	}
}
	

dBaseHierarchy* dBaseHierarchy::GetRoot() const
{
	const dBaseHierarchy *root;
	for (root = this; root->m_parent; root = root->m_parent);
	return (dBaseHierarchy*)root;
}


dBaseHierarchy* dBaseHierarchy::GetFirst() const
{
	dBaseHierarchy *ptr;

	for (ptr = (dBaseHierarchy *)this; ptr->m_child; ptr = ptr->m_child);
	return ptr;
}

dBaseHierarchy* dBaseHierarchy::GetNext() const
{
	dBaseHierarchy *x;
	dBaseHierarchy *ptr;

	if (m_sibling) {
		return m_sibling->GetFirst();
	}

	x = (dBaseHierarchy *)this;
	for (ptr = m_parent; ptr && (x == ptr->m_sibling); ptr = ptr->m_parent) {
		x = ptr;
	}
	return ptr;
}



dBaseHierarchy* dBaseHierarchy::GetLast() const
{
	dBaseHierarchy *ptr;
		
	for (ptr = (dBaseHierarchy *)this; ptr->m_sibling; ptr = ptr->m_sibling);
	return ptr;
}


dBaseHierarchy* dBaseHierarchy::GetPrev() const
{
	dBaseHierarchy *x;
	dBaseHierarchy *ptr;

	if (m_child) {
		return m_child->GetNext();
	}

	x = (dBaseHierarchy *)this;
	for (ptr = m_parent; ptr && (x == ptr->m_child); ptr = ptr->m_child) {
		x = ptr;
	}
	return ptr;
}


dBaseHierarchy* dBaseHierarchy::Find (unsigned nameCRC) const 
{
	dBaseHierarchy *ptr;

	if (nameCRC == GetNameID()) {
		return (dBaseHierarchy*)this;
	} else {
		for (ptr = GetFirst(); ptr && (ptr != this); ptr = ptr->GetNext()) {
			if (nameCRC == ptr->GetNameID()) {
				return ptr;
			}
		}
	}

	return NULL;
}








