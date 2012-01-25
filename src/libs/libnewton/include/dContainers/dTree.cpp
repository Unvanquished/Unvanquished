//********************************************************************
// Direct geometry Hardware independent library
//	By Julio Jerez
// VC: 6.0
//********************************************************************
#include <dContainersStdAfx.h>
#include "dTree.h"


dRedBackNode *dRedBackNode::Minimum () const
{
	dRedBackNode *ptr;

	for (ptr = (dRedBackNode *)this; ptr->m_left; ptr = ptr->m_left);
	return ptr;
}

dRedBackNode *dRedBackNode::Maximum () const
{
	dRedBackNode *ptr;

	for (ptr = (dRedBackNode *)this; ptr->m_right; ptr = ptr->m_right);
	return ptr;
}


dRedBackNode *dRedBackNode::Prev () const
{
	dRedBackNode *me;
	dRedBackNode *ptr;

	if (m_left) {
		return m_left->Maximum ();
	}

	me = (dRedBackNode *)this;
	for (ptr = m_parent; ptr && me == ptr->m_left; ptr = ptr->m_parent) {
		me = ptr;
	}
	return ptr;

}

dRedBackNode *dRedBackNode::Next () const
{
	dRedBackNode *ptr;
	dRedBackNode *node;

	if (m_right) {
		return m_right->Minimum ();
	}

	node = (dRedBackNode *)this;
	for (ptr = m_parent; ptr && node == ptr->m_right; ptr = ptr->m_parent) {
		node = ptr;
	}
	return ptr;
}

// rotate node me to left 
void dRedBackNode::RotateLeft(dRedBackNode **head) 
{
	dRedBackNode *me;
	dRedBackNode *child;

	me = this;
	child = me->m_right;
	
	//establish me->m_right link 
	me->m_right = child->m_left;
	if (child->m_left != NULL) {
		child->m_left->m_parent = me;
	}
	
	// establish child->m_parent link 
	if (child != NULL) {
		child->m_parent = me->m_parent;
	}
	if (me->m_parent) {
		if (me == me->m_parent->m_left) {
			me->m_parent->m_left = child;
		} else {
			me->m_parent->m_right = child;
		}
	} else {
		*head = child;
	}
	
	// link child and me 
	child->m_left = me;
	if (me != NULL) {
		me->m_parent = child;
	}
}


// rotate node me to right  *
void dRedBackNode::RotateRight(dRedBackNode **head) 
{
	dRedBackNode *me;
	dRedBackNode *child;

	me = this;
	child = me->m_left;

	// establish me->m_left link 
	me->m_left = child->m_right;
	if (child->m_right != NULL) {
		child->m_right->m_parent = me;
	}

	// establish child->m_parent link 
	if (child != NULL) {
		child->m_parent = me->m_parent;
	}
	if (me->m_parent) {
		if (me == me->m_parent->m_right) {
			me->m_parent->m_right = child;
		} else {
			me->m_parent->m_left = child;
		}
	} else {
		*head = child;
	}

	// link me and child 
	child->m_right = me;
	if (me != NULL) {
		me->m_parent = child;
	}
}


// maintain Red-Black tree balance after inserting node ptr   
void dRedBackNode::InsertFixup(dRedBackNode **head) 
{
	dRedBackNode *ptr;
	dRedBackNode *tmp;

	ptr = this;
	// check Red-Black properties 
	while ((ptr != *head) && (ptr->m_parent->GetColor() == RED)) {
		// we have a violation 
		if (ptr->m_parent == ptr->m_parent->m_parent->m_left) {
			tmp = ptr->m_parent->m_parent->m_right;
			if (tmp && (tmp->GetColor() == RED)) {
				// uncle is RED 
				ptr->m_parent->SetColor(BLACK);
				tmp->SetColor(BLACK) ;
				ptr->m_parent->m_parent->SetColor(RED) ;
				ptr = ptr->m_parent->m_parent;
			} else {
				// uncle is BLACK 
				if (ptr == ptr->m_parent->m_right) {
					// make ptr a left child 
					ptr = ptr->m_parent;
					ptr->RotateLeft(head);
				}

				ptr->m_parent->SetColor(BLACK);
				if (ptr->m_parent->m_parent) {
					ptr->m_parent->m_parent->SetColor(RED);
					ptr->m_parent->m_parent->RotateRight(head);
				}
			}
		} else {
			//_ASSERTE (ptr->m_parent == ptr->m_parent->m_parent->m_right);
			// mirror image of above code 
			tmp = ptr->m_parent->m_parent->m_left;
			if (tmp && (tmp->GetColor() == RED)) {
				//uncle is RED 
				ptr->m_parent->SetColor(BLACK);
				tmp->SetColor(BLACK) ;
				ptr->m_parent->m_parent->SetColor(RED) ;
				ptr = ptr->m_parent->m_parent;
			} else {
				// uncle is BLACK 
				if (ptr == ptr->m_parent->m_left) {
					ptr = ptr->m_parent;
					ptr->RotateRight(head);
				}
				ptr->m_parent->SetColor(BLACK);
				if (ptr->m_parent->m_parent->GetColor() == BLACK) {
					ptr->m_parent->m_parent->SetColor(RED) ;
				   ptr->m_parent->m_parent->RotateLeft (head); 
				}
			}
		}
	}
	(*head)->SetColor(BLACK);
}


//maintain Red-Black tree balance after deleting node x 
void dRedBackNode::RemoveFixup (
	dRedBackNode *node, 
	dRedBackNode **head) 
{
	dRedBackNode *ptr;
	dRedBackNode *tmp; 

	ptr = this;
	while ((node != *head) && (!node || node->GetColor() == BLACK)) {
		if (node == ptr->m_left) {
			if (!ptr) {
				return;
			}
			tmp = ptr->m_right;
			if (!tmp) {
				return;
			}
			if (tmp->GetColor() == RED) {
				tmp->SetColor(BLACK) ;
				ptr->SetColor(RED) ;
				ptr->RotateLeft (head);
				tmp = ptr->m_right;
				if (!ptr || !tmp) {
					return;
				}
			}
			if ((!tmp->m_left  || (tmp->m_left->GetColor() == BLACK)) && 
				 (!tmp->m_right || (tmp->m_right->GetColor() == BLACK))) {
				tmp->SetColor(RED);
				node = ptr;
				ptr = ptr->m_parent;
				continue;
			} else if (!tmp->m_right || (tmp->m_right->GetColor() == BLACK)) {
				tmp->m_left->SetColor(BLACK);
				tmp->SetColor(RED);
				tmp->RotateRight (head);
				tmp = ptr->m_right;
				if (!ptr || !tmp) {
					return;
				}
			}
			tmp->SetColor (ptr->GetColor());
			if (tmp->m_right) {
				tmp->m_right->SetColor(BLACK) ;
			}
			if (ptr) {
				ptr->SetColor(BLACK) ;
				ptr->RotateLeft (head);
			}
			node = *head;

		} else {
		  	if (!ptr) {
				return;
		  	}
			tmp = ptr->m_left;
			if (!tmp) {
				return;
			}
			if (tmp->GetColor() == RED) {
				tmp->SetColor(BLACK) ;
				ptr->SetColor(RED) ;
				ptr->RotateRight (head);
				tmp = ptr->m_left;
				if (!ptr || !tmp) {
					return;
				}
			}

			if ((!tmp->m_right || (tmp->m_right->GetColor() == BLACK)) && 
				 (!tmp->m_left  || (tmp->m_left->GetColor() == BLACK))) {
				tmp->SetColor(RED) ;
				node = ptr;
				ptr = ptr->m_parent;
				continue;
			} else if (!tmp->m_left || (tmp->m_left->GetColor() == BLACK)) {
				tmp->m_right->SetColor(BLACK) ;
				tmp->SetColor(RED) ;
				tmp->RotateLeft (head);
				tmp = ptr->m_left;
				if (!ptr || !tmp) {
					return;
				}
			}
			tmp->SetColor (ptr->GetColor());
			if (tmp->m_left) {
				tmp->m_left->SetColor(BLACK);
			}
			if (ptr) {
				ptr->SetColor(BLACK) ;
				ptr->RotateRight (head);
			}
			node = *head;
		}
	}
	if (node) {
		node->SetColor(BLACK);
	}
}


void dRedBackNode::Remove (dRedBackNode **head) 
{
	bool oldColor;
	dRedBackNode *node;
	dRedBackNode *child;
	dRedBackNode *endNode;
	dRedBackNode *endNodeParent;

	node = this;
//	node->Kill();
	node->SetInTreeFlag(false);

	if (!node->m_left || !node->m_right) {
		// y has a NULL node as a child 
		endNode = node;
	
		// x is y's only child 
		child = endNode->m_right;
		if (endNode->m_left) {
			child = endNode->m_left;
		}
		
		// remove y from the parent chain 
		if (child) {
			child->m_parent = endNode->m_parent;
		}
		
		if (endNode->m_parent)	{
			if (endNode == endNode->m_parent->m_left) {
				endNode->m_parent->m_left = child;
			} else {
				endNode->m_parent->m_right = child;
			}
		} else {
			*head = child;
		}
		
		if (endNode->GetColor() == BLACK) {
			endNode->m_parent->RemoveFixup (child, head);
		}
		delete endNode;
	
	} else {

	  	// find tree successor with a NULL node as a child 
		endNode = node->m_right;
		while (endNode->m_left != NULL) {
			endNode = endNode->m_left;
		}
		
//		_ASSERTE (endNode);
//		_ASSERTE (endNode->m_parent);
//		_ASSERTE (!endNode->m_left);
		
		// x is y's only child 
		child = endNode->m_right;
		
//		_ASSERTE ((endNode != node->m_right) || !child || (child->m_parent == endNode));

		endNode->m_left = node->m_left;
		node->m_left->m_parent = endNode;
		
		endNodeParent = endNode;
		if (endNode	!= node->m_right) {
			if (child) {
				child->m_parent = endNode->m_parent;
			}
			endNode->m_parent->m_left = child;
			endNode->m_right = node->m_right;
			node->m_right->m_parent = endNode;
			endNodeParent = endNode->m_parent;
		}
		

		if (node == *head) {
			*head = endNode;
		} else if (node == node->m_parent->m_left) {
			node->m_parent->m_left = endNode;
		} else {
			node->m_parent->m_right = endNode;
		}
		endNode->m_parent = node->m_parent;
		
		oldColor = endNode->GetColor();
		endNode->SetColor (node->GetColor());
		node->SetColor	(oldColor);
		
		if (oldColor == BLACK) {
		  	endNodeParent->RemoveFixup (child, head);
		}
		delete node;
	}
}


void dRedBackNode::RemoveAllLow ()
{
	if (m_left) {
		m_left->RemoveAllLow();
	}
	if (m_right) {
		m_right->RemoveAllLow ();
	}
	SetInTreeFlag(false);
	delete this;
}

void dRedBackNode::RemoveAll ()
{
	dRedBackNode *root;
	for (root = this; root->m_parent; root = root->m_parent);
	root->RemoveAllLow();
}

