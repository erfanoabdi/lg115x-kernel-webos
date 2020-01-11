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

#ifndef __ADEC_RBTREE_H__
#define __ADEC_RBTREE_H__

#include "common/adec_common.h"

/* �ڷ��� */
typedef unsigned int RBKey;

typedef enum _RB_COLOR
{
	RB_COLOR_RED,
	RB_COLOR_BLACK,
}RB_COLOR;

ADEC_RESULT	RBTree_CreateTree(void** _tree, void (*_terminator)(void*));					// Ʈ�� ����
ADEC_RESULT	RBTree_InitTree(void* _tree, void (*_terminator)(void*));						// Ʈ�� �ʱ�ȭ
ADEC_RESULT	RBTree_DeleteTree(void* _tree);						// Ʈ�� ����
ADEC_RESULT	RBTree_CreateNode(void* _Tree, RBKey _key, void* _info);// ��� ����

typedef void VisitFuncPtr_Key(RBKey _key);
typedef void VisitFuncPtr_Info(void* _info);

ADEC_RESULT	RBTree_Search(void* _Tree, RBKey _key, void** _info);// ������ Ž��
ADEC_RESULT	RBTree_Delete(void* _Tree, RBKey _key);				// ������ ����

#endif

