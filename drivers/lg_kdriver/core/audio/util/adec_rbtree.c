/*
	SIC LABORATORY, LG ELECTRONICS INC., SEOUL, KOREA
	Copyright(c) 2013 by LG Electronics Inc.

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	version 2 as published by the Free Software Foundation.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.
*/

#include "adec_rbtree.h"
#include "common/adec_common.h"

/******************************************************************************
  ����ü
 ******************************************************************************/

AdecMemStat *IMC_MEM_MOD;

typedef struct _RBNode
{
	RBKey key;					// Ű
	void* info;					// ����
	RB_COLOR color;				// ��� ���� : RED or BLACK
	struct _RBNode* parent;		// �θ� ���
	struct _RBNode* left;		// ���� �ڽ� ���
	struct _RBNode* right;		// ������ �ڽ� ���
}RBNode;

typedef struct _RBTree
{
	RBNode* root;
	RBNode* sentinal;
	void	(*terminator)(void*);
}RBTree;

/******************************************************************************
  ���� �Լ� ����
 ******************************************************************************/
// Ʈ�� �� ��� ����
static ADEC_RESULT _RBTree_DeleteAllRBNode(
	RBTree* _tree,
	RBNode* _node);
// ���� ȸ��
static ADEC_INLINE ADEC_RESULT _RBTree_LeftRotate(
	RBTree* _tree,
	RBNode* _node);

// ������ ȸ��
static ADEC_INLINE ADEC_RESULT _RBTree_RightRotate(
	RBTree* _tree,
	RBNode* _node);
// Terminator
static ADEC_INLINE void _list_default_terminator(void *_item);

#if 0
// ��ȸ
static void _RBTree_Traverse(
	RBNode* _node,
	VisitFuncPtr_Key _key,
	VisitFuncPtr_Info _info);
#endif

/******************************************************************************
  ���� �Լ�
 ******************************************************************************/
// Ʈ�� �� ��� ����
static ADEC_RESULT _RBTree_DeleteAllRBNode(RBTree* _tree, RBNode* _node)
{
	void	(*trm)(void*);

	trm = _tree->terminator;

	if(_node != _tree->sentinal && _node != NULL)
	{
		_RBTree_DeleteAllRBNode(_tree, _node->left);
		_RBTree_DeleteAllRBNode(_tree, _node->right);
		trm(_node->info);
		ADEC_FREE(IMC_MEM_MOD, _node);
	}

	return ADEC_ERR_NONE;
}

// ���� ȸ��
static ADEC_INLINE ADEC_RESULT _RBTree_LeftRotate(
	RBTree* _tree,
	RBNode* _node)
{
	RBNode* parentNode = _node->parent;
	RBNode* childNode = _node->left;
	RBNode* sentinal = _tree->sentinal;

	if(parentNode == parentNode->parent->left)	// �ҹ�(�θ�)��		��� ���
	{
		parentNode->parent->left = _node;
	}
	else if(parentNode == parentNode->parent->right)
	{
		parentNode->parent->right = _node;
	}

	_node->parent = parentNode->parent;			// �����			�ҹ�(�θ�) ���
	_node->left = parentNode;					// �����			�θ�(�ڽ�) ���
	parentNode->parent = parentNode->right;		// �θ�(�ڽ�)��		���(�θ�) ���
	parentNode->right = childNode;				// �θ���			�ڽ� ���
	if(childNode != sentinal)
	{
		childNode->parent = parentNode;			// �ڽ���			�θ� ��� (��Ƽ���� �ƴѰ��)
	}

	// ��Ʈ ��� ��ü�� ���
	if(parentNode == _tree->root)
	{
		_node->color = RB_COLOR_BLACK;
		_tree->root = _node;
	}

	return ADEC_ERR_NONE;
}
// ������ ȸ��
static ADEC_INLINE ADEC_RESULT _RBTree_RightRotate(
	RBTree* _tree,
	RBNode* _node)
{
	RBNode* parentNode = _node->parent;
	RBNode* childNode = _node->right;
	RBNode* sentinal = _tree->sentinal;

	if(parentNode == parentNode->parent->left)	// �ҹ�(�θ�)��		��� ���
	{
		parentNode->parent->left = _node;
	}
	else if(parentNode == parentNode->parent->right)
	{
		parentNode->parent->right = _node;
	}

	_node->parent = parentNode->parent;			// �����			�ҹ�(�θ�) ���
	_node->right = parentNode;					// �����			�θ�(�ڽ�) ���
	parentNode->parent = parentNode->left;		// �θ�(�ڽ�)��		���(�θ�) ���
	parentNode->left = childNode;				// �θ���			�ڽ� ���
	if(childNode != sentinal)
	{
		childNode->parent = parentNode;			// �ڽ���			�θ� ��� (��Ƽ���� �ƴѰ��)
	}

	// ��Ʈ ��� ��ü�� ���
	if(parentNode == _tree->root)
	{
		_node->color = RB_COLOR_BLACK;
		_tree->root = _node;
	}

	return ADEC_ERR_NONE;
}

static ADEC_INLINE void _list_default_terminator(void *_item)
{
	return;
}
#if 0
// ��ȸ
static void _RBTree_Traverse(
	RBNode* _node,
	VisitFuncPtr_Key _key,
	VisitFuncPtr_Info _info)
{
	if(_node ==  NULL)
	{
		return;
	}

	_key(_node->key);
	_info(_node->info);
	_RBTree_Traverse(_node->left, _key, _info);
	_RBTree_Traverse(_node->right, _key, _info);
}
#endif

/******************************************************************************
  �����Լ�
 ******************************************************************************/
// Ʈ�� ����
ADEC_RESULT RBTree_CreateTree(void** _tree, void (*_terminator)(void*))
{
	RBTree* tree;
	int structSize = sizeof(RBTree);

	// memory allocation
	*_tree = tree = ADEC_MALLOC(IMC_MEM_MOD, structSize, RBTree);

	return RBTree_InitTree(tree, _terminator);
}

// Ʈ�� �ʱ�ȭ
ADEC_RESULT	RBTree_InitTree(void* _tree, void (*_terminator)(void*))
{
	RBTree* tree;

	tree = (RBTree*)_tree;

#if 0
	tree->root = ADEC_MALLOC(IMC_MEM_MOD, sizeof(RBNode), RBNode);
#endif
	tree->sentinal = ADEC_MALLOC(IMC_MEM_MOD, sizeof(RBNode), RBNode);

	if (_terminator == NULL)
	    tree->terminator = _list_default_terminator;
	else
	    tree->terminator = _terminator;

	// ��Ƽ�� ��� �ʱ�ȭ
	tree->sentinal->key = 0;
	tree->sentinal->color = RB_COLOR_BLACK;
	tree->sentinal->parent = tree->sentinal;
	tree->sentinal->left = tree->sentinal;
	tree->sentinal->right = tree->sentinal;

	tree->root = NULL;
#if 0
	// ��Ʈ ��Ʈ �ʱ�ȭ
	tree->root->key = 0;
	tree->root->color = RB_COLOR_BLACK;
	tree->root->parent = tree->sentinal;
	tree->root->left = tree->sentinal;
	tree->root->right = tree->sentinal;
#endif

	ADEC_ASSERT( _tree!= NULL, return ADEC_ERR_NULL);

	return ADEC_ERR_NONE;
}

// Ʈ�� ����
ADEC_RESULT RBTree_DeleteTree(void* _tree)
{
	RBTree* tree;

	tree = (RBTree*)_tree;

	ADEC_ASSERT(
		_tree != NULL,
		return ADEC_ERR_NULL);

	_RBTree_DeleteAllRBNode(tree, tree->root);
	ADEC_FREE(IMC_MEM_MOD,tree->sentinal);
	ADEC_FREE(IMC_MEM_MOD,tree);

	return ADEC_ERR_NONE;
}

// ��� ����
ADEC_RESULT RBTree_CreateNode(void* _tree, RBKey _key, void* _info)
{
	RBTree* tree;
	RBNode* parentNode;			// �θ� ���
	RBNode* uncleNode = NULL;	// ���� ���
	RBNode* currentNode;		// ���� ���
	RBNode* newNode;			// ���� ���

	tree = (RBTree*)_tree;

	ADEC_ASSERT(
		_tree != NULL && _info != NULL,
		return ADEC_ERR_NULL);

	// �߸��� �� ����
	if (_key == 0)
	{
	    return ADEC_ERR_RANGE;
	}

	parentNode = tree->sentinal;
	currentNode = tree->root;

	// ���ο� ��尡 �߰��� ��ġ ã��
	while(currentNode != tree->sentinal && tree->root != NULL)
	{
		// �ߺ�Ű üũ
		if(_key == currentNode->key)
		{
			currentNode->info = _info;
			return ADEC_ERR_DUPLICATED;
		}

		parentNode = currentNode;

		// Ű ũ��񱳸� ���� �ּ� ã��
		currentNode = (currentNode->key > _key) ? currentNode->left : currentNode->right;
	}

	// ���ο� �ڽ� ��� ����
	newNode = ADEC_MALLOC(IMC_MEM_MOD, sizeof(RBNode), RBNode);
	newNode->key =  _key;
	newNode->left = tree->sentinal;
	newNode->right = tree->sentinal;
	newNode->info = _info;

	if(parentNode != tree->sentinal)	// ��Ʈ ��尡 �ƴѰ��
	{
#if 0
		newNode = ADEC_MALLOC(IMC_MEM_MOD, sizeof(RBNode), RBNode);
		newNode->key =  _key;
		newNode->left = tree->sentinal;
		newNode->right = tree->sentinal;
		newNode->info = _info;
#endif
		newNode->color = RB_COLOR_RED;
		newNode->parent = parentNode;

		// ������ ���� �θ�-�ڽ� ���� ���
		if(_key < newNode->parent->key)
		{
			newNode->parent->left = newNode;
		}
		else
		{
			newNode->parent->right = newNode;
		}

		currentNode = newNode;

		while(currentNode->parent->color == RB_COLOR_RED)					// �θ� RED�� ���
		{
			// �θ� �ҹ��� ���� �ڽ��� ���
			if(currentNode->parent == currentNode->parent->parent->left)
			{
				uncleNode = currentNode->parent->parent->right;				// ������ �ҹ��� ������ �ڽ�
				// case 1: ������ RED �� ���
				if(uncleNode->color == RB_COLOR_RED)
				{
					if(currentNode->parent->parent != tree->root)
					{
						currentNode->parent->parent->color = RB_COLOR_RED;	// �ҹ� ����
					}
					currentNode->parent->color = RB_COLOR_BLACK;			// �θ� ����
					uncleNode->color = RB_COLOR_BLACK;						// ���� ����

					currentNode = currentNode->parent->parent;				// ���
				}
				// case 2 : ������ BLACK �� ���
				else
				{
					if(currentNode == currentNode->parent->right)			// ���� ��尡 ������ ���
					{
						_RBTree_LeftRotate(tree, currentNode);				// �ٱ������� ���� ȸ��
						currentNode = currentNode->left;
					}
					currentNode->parent->color = RB_COLOR_BLACK;			// �θ� ����
					currentNode->parent->parent->color = RB_COLOR_RED;		// �ҹ� ����
					_RBTree_RightRotate(tree, currentNode->parent);			// ū ȸ��
				}
			}
			// �θ� �ҹ��� ������ �ڽ��� ���
			else
			{
				uncleNode = currentNode->parent->parent->left;				// ������ �ҹ��� ���� �ڽ�
				// case 1 : ������ RED �� ���
				if(uncleNode->color == RB_COLOR_RED)
				{
					if(currentNode->parent->parent != tree->root)
					{
						currentNode->parent->parent->color = RB_COLOR_RED;	// �ҹ� ����
					}
					currentNode->parent->color = RB_COLOR_BLACK;			// �θ� ����
					uncleNode->color = RB_COLOR_BLACK;						// ���� ����

					currentNode = currentNode->parent->parent;				// ���
				}
				// case 2 : ������ BLACK �� ���
				else
				{
					if(currentNode == currentNode->parent->left)			// ���� ��尡 ������ ���
					{
						_RBTree_RightRotate(tree, currentNode);				// �ٱ������� ���� ȸ��
						currentNode = currentNode->right;
					}
					currentNode->parent->color = RB_COLOR_BLACK;			// �θ� ����
					currentNode->parent->parent->color = RB_COLOR_RED;		// �ҹ� ����
					_RBTree_LeftRotate(tree, currentNode->parent);			// ū ȸ��
				}
			}
		}
	}
	else	// �� ��尡 ��Ʈ�� ���
	{
		newNode->color = RB_COLOR_BLACK;
		newNode->parent = parentNode;

		tree->root =  newNode;
#if 0
		tree->root->key =  _key;
		tree->root->color = RB_COLOR_BLACK;
		tree->root->info = _info;
#endif
	}

	return ADEC_ERR_NONE;
}

// Ű Ž��
ADEC_RESULT RBTree_Search(void* _tree, RBKey _key, void** _info)
{
	RBTree* tree;
	RBNode* currentNode;

	ADEC_ASSERT(_info != NULL, return ADEC_ERR_NULL);

	tree = (RBTree*)_tree;
	currentNode = tree->root;

	while(currentNode != tree->sentinal && tree->root != NULL)
	{
		// ã������ �ش� ��� ����
		if(_key == currentNode->key)
		{
			*_info = currentNode->info;
			return ADEC_ERR_NONE;
		}
		// ��ã������ Ű ũ�� �� �� �̵�
		else
		{
			currentNode = (_key < currentNode->key) ? currentNode->left : currentNode->right;
		}
	}

	// Ʈ���� ã�� Ű�� ���� ���
	*_info = NULL;

	return ADEC_ERR_NOT_FOUND;
}

// Ű ����
ADEC_RESULT RBTree_Delete(void* _tree, RBKey _key)
{
	RBTree* tree;
	RBNode* currentNode;
	RBNode* deleteNode;
	RBNode* childNode;	// ���� ����� �ڽ� ���
	RBNode* replaceNode;
	void	(*trm)(void*);

	// Check a NULL pointer.
	if((void*)_tree == NULL)
	{
		return ADEC_ERR_NOT_FOUND;
	}

	tree = (RBTree*)_tree;
	trm = tree->terminator;

	// Check a NULL pointer.
	if(tree->root == NULL)
	{
		return ADEC_ERR_NOT_FOUND;
	}

	currentNode = tree->root;

	// ���� ��� ��� Ž��
	while(currentNode != tree->sentinal && currentNode->key != _key) // ��Ƽ�α��� ���ų�, Ű�� ã��������
	{
		currentNode = (_key < currentNode->key) ? currentNode->left : currentNode->right;
	}

	// Ž�� ���н�
	if(currentNode == tree->sentinal)
	{
		return ADEC_ERR_NOT_FOUND;
	}

	// Ž�� ������
	deleteNode = currentNode;
	trm(currentNode->info);

	// case 1 : ���� ����� �ܸ� ���
	if(deleteNode->left == tree->sentinal && deleteNode->right == tree->sentinal)
	{

		if(deleteNode == tree->root)	// ��Ʈ ����� ���
		{
//			tree->root->key = 0;	// Ű �ʱ�ȭ
			ADEC_FREE(IMC_MEM_MOD,tree->root);
			tree->root = NULL;
			return ADEC_ERR_NONE;
		}
		else
		{
			// ��� ���� : ��Ƽ
			if(deleteNode->parent->left == deleteNode)
			{
				deleteNode->parent->left = tree->sentinal;
			}
			else
			{
				deleteNode->parent->right = tree->sentinal;
			}
		}

		replaceNode = deleteNode;
	}
	// case 2 : ���� ����� �ϳ��� �ڽ� ��带 ���� ���
	else if(deleteNode->left == tree->sentinal || deleteNode->right == tree->sentinal)
	{
		// ���� ����� �ڽ� ��� �ľ�
		childNode = (deleteNode->left != tree->sentinal) ? deleteNode->left : deleteNode->right;

		// ���� ��� ��ġ�� ���� ��� �ڽ����� ��ü
		if (deleteNode == tree->root)					// ���� ����� ��Ʈ
		{
			childNode->color = RB_COLOR_BLACK;
			childNode->parent = tree->root->parent;
			tree->root = childNode;						// ���ο� ��Ʈ ���
		}
		else
		{
			if(deleteNode->parent->left == deleteNode)	// ���� ����� �θ��� ���� �ڽ�
			{
				deleteNode->parent->left = childNode;
				childNode->parent = deleteNode->parent;
			}
			else											// ���� ����� �θ��� ������ �ڽ�
			{
				deleteNode->parent->right = childNode;
				childNode->parent = deleteNode->parent;
			}
		}

		replaceNode = deleteNode;
	}
	// case 3 : ���� ����� �ΰ��� �ڽ� ��带 ���� ��� : ������ ���� Ʈ���� �ּҰ����� ��ü �� ��ü ��� ����.
	else
	{
		// ��ü ��� ���� ���� ��� ���� ����
		replaceNode = deleteNode->right;	// ��ü ���

		// ��ü ��� Ž�� : ������ ���� Ʈ���� �ּҰ�
		while(replaceNode->left != tree->sentinal)
		{
			replaceNode = replaceNode->left;
		}

		currentNode = replaceNode->right;						// ��ü ����� �ڽ�

		// ��ü ����� �θ� ���� �ڽ� ��� ����
		if(replaceNode->parent->left == replaceNode)			// ��ü ��尡 ��ü ��� �θ��� ���� �ڽ�
		{
			replaceNode->parent->left = currentNode;
			if(currentNode != tree->sentinal)
			{
				currentNode->parent= replaceNode->parent;
			}
		}
		else													// ��ü ��尡 ��ü ��� �θ��� ������ �ڽ�
		{
			replaceNode->parent->right = currentNode;
			if(currentNode != tree->sentinal)
			{
				currentNode->parent = replaceNode->parent;
			}
		}

		// ���� ��忡 ��ü ��� �� ����
		deleteNode->key = replaceNode->key;
		deleteNode->info = replaceNode->info;

		// �뷱�� �� �� ����
		if(replaceNode->color == RB_COLOR_BLACK)
		{
			while(currentNode->color == RB_COLOR_BLACK && currentNode != tree->sentinal && currentNode != tree->root)
			{
				// case 1 : �θ� ����, ��ī���� ���(��Ʈ ���� �ݺ�)
				if(currentNode->parent->color == RB_COLOR_RED
					&& currentNode->parent->left->left->color == RB_COLOR_BLACK
					&& currentNode->parent->left->right->color == RB_COLOR_BLACK)
				{
					currentNode->parent->color = RB_COLOR_BLACK;
					currentNode->parent->left->color = RB_COLOR_RED;

					currentNode = currentNode->parent;
				}
				else
				{
					// case 1: ������ ����
					if(currentNode->parent->left->color == RB_COLOR_RED)
					{
						_RBTree_RightRotate(tree, currentNode->parent->left);
						currentNode->parent->color = RB_COLOR_RED;
						currentNode->parent->parent->color = RB_COLOR_BLACK;
					}

					// case 2 : �θ� ���, ��ī���� ���
					else if(currentNode->parent->color == RB_COLOR_BLACK
						&& currentNode->parent->left->left->color == RB_COLOR_BLACK
						&& currentNode->parent->left->right->color == RB_COLOR_BLACK)
					{
						currentNode->parent->left->color = RB_COLOR_RED;
					}

					// case 3 : ������ ���� �ڽ��� ����, �ٱ��� �ڽ��� ���
					else if(currentNode->parent->left->left->color == RB_COLOR_BLACK
						&& currentNode->parent->left->right->color == RB_COLOR_RED)
					{
						_RBTree_LeftRotate(tree, currentNode->parent->left->right);

						currentNode->parent->left->color = RB_COLOR_BLACK;
						currentNode->parent->left->left->color = RB_COLOR_RED;
					}
					// case 4 : ������ �ٱ��� �ڽ��� ����
					else if(currentNode->parent->left->left->color == RB_COLOR_RED)
					{
						currentNode->parent->left->left->color = RB_COLOR_BLACK;

						_RBTree_RightRotate(tree, currentNode->parent->left);
					}
					currentNode = tree->root;
				}
			}
			currentNode->color = RB_COLOR_BLACK;
		}
	}

	ADEC_FREE(IMC_MEM_MOD,replaceNode);

	return ADEC_ERR_NONE;
}
