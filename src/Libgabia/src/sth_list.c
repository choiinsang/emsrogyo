/* *************************************************
�ۼ��� : ������
�ۼ��� : ?
��� ���� : LIST
************************************************* */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sth_list.h"

/* *************************************************
Function: FreeList
Parameters: LPMEMBER *start
Return: void
Comment: LIST �� �����Ѵ�.
************************************************* */
void
FreeList( LPMEMBER *start )
{
  if (!(*start))
    return;

  FreeList(&(*start)->nextnode);

  free((*start)->data);
  free(*start);
  *start = NULL;
}

/* *************************************************
Function: GetStartMemberNDel
Parameters: LPRXLIST list: LIST
Return: LPMEMBER
Comment: ù° �ɹ��� �������� �����Ѵ�. (POP)
************************************************* */
LPMEMBER
GetStartMemberNDel( LPRXLIST list )
{
	LPMEMBER ret;
	
	if (!list || !list->start)
		return NULL;

	ret = list->start;

	list->start = list->start->nextnode;
	list->total_count--;

	return ret;
}

/* *************************************************
Function: GetStartMember
Parameters: LPRXLIST list: LIST
Return: LPMEMBER
Comment: ù° �ɹ��� �����´�
************************************************* */
LPMEMBER
GetStartMember( LPRXLIST list )
{
	return (list) ? list->start : NULL;
}

/* *************************************************
Function: GetListCount
Parameters: LPRXLIST list: LIST
Return: long : LIST ����
Comment: LIST �� ���Ե� ������ ��ȯ�Ѵ�.
************************************************* */
long
GetListCount( LPRXLIST list )
{
	return (list) ? list->total_count : 0;
}

/* *************************************************
Function: InitList
Parameters: LPRXLIST list: LIST
Return: void
Comment: LIST �� �ʱ�ȭ�Ѵ�.
************************************************* */
void
InitList( LPRXLIST list )
{
	list->start=NULL;
	list->total_count=0;
}

/* *************************************************
Function: ListNodeAdd
Parameters: LPRXLIST list: LIST
			const void *data: DATA
			size_t size: DATA Length
Return: LPMEMBER : LIST Start member or �߰��� ������
Comment: LIST �� �ʱ�ȭ�Ѵ�.
************************************************* */
LPMEMBER
ListNodeAdd( LPRXLIST list, const void *data, size_t size)
{
	LPMEMBER p;

	if (!list)
		return NULL;

	p=list->start;

	if( ( list->start = (LPMEMBER)Malloc(sizeof(MEMBER)) ) == NULL )
	{
		return NULL;
	}

	if( ( list->start->data=(void *)Malloc(size) ) == NULL )
	{
		free(list->start);
		list->start=p;
		return NULL;
	}

	list->start->data = memcpy(list->start->data,data,size);
	list->start->size = size;
	list->start->nextnode=p;
	list->total_count++;

	return list->start;
}

/* *************************************************
Function: ListNodeDel
Parameters: LPRXLIST list: LIST
			const void *data: DATA
Return: int : 0,1
Comment: LIST �� �����͸� �����Ѵ�.
************************************************* */
int
ListNodeDel( LPRXLIST list, const void *data )
{
	LPMEMBER temp;
	LPMEMBER item;

	item=NULL;

	if (!list)
		return 0;

	if( !list->start ) return 0;

	temp=list->start;

	while (temp!=NULL)
	{
		if (temp->data==data)
		{
			if (item==NULL)
			{
				list->start=temp->nextnode;
				free(temp->data);
				free((LPMEMBER)temp);
			}
			else
			{
				item->nextnode=temp->nextnode;
				free(temp->data);
				free((LPMEMBER)temp);
			}

			list->total_count--;
		}

		item=temp;
		temp=temp->nextnode;
	}

	return 1;
}

/* *************************************************
Function: ReverseList
Parameters: LPMEMBER start: LIST Start member
Return: LPMEMBER : ����� LIST Start member
Comment: LIST �� �������Ѵ�.
	LPRXLIST �� ���� �����̹Ƿ� ���� ������� �о� �帮���� ������ ������ �ʿ��ϴ�.
************************************************* */
LPMEMBER 
ReverseList( LPMEMBER start)
{
	LPMEMBER newstart;
	
	if (!start)
		return NULL;
	
	if (!start->nextnode)
		return start;
	
	newstart = ReverseList(start->nextnode);
	start->nextnode->nextnode = start;
	start->nextnode = NULL;

	return newstart;
}
