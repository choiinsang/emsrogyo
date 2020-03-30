#ifndef  _STH_LIST_H
#define  _STH_LIST_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C"
{
#endif				/* __cplusplus */

// MEMBER
  typedef struct _member_t
  {
    void *data;			// DATA
    size_t size;		// DATA Length
    struct _member_t *nextnode;	// Next member
  }
  MEMBER, *LPMEMBER;

  typedef struct _list_t
  {
    LPMEMBER start;		// Start member
    long total_count;		// ÃÑ °¹¼ö
  }
  RXLIST, *LPRXLIST;

  void FreeList (LPMEMBER * start);
  LPMEMBER GetStartMemberNDel (LPRXLIST list);
  LPMEMBER GetStartMember (LPRXLIST list);
  long GetListCount (LPRXLIST list);
  void InitList (LPRXLIST list);
  LPMEMBER ListNodeAdd (LPRXLIST list, const void *data, size_t size);
  int ListNodeDel (LPRXLIST list, const void *data);
  LPMEMBER ReverseList (LPMEMBER start);

#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif				// _STH_LIST_H
