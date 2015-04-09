/*
*********************************************************************************************************
*                                                uC/OS-II
*                                          The Real-Time Kernel
*                                            MEMORY MANAGEMENT
*
*                              (c) Copyright 1992-2013, Micrium, Weston, FL
*                                           All Rights Reserved
*
* File    : OS_MEM.C
* By      : Jean J. Labrosse
* Version : V2.92.11
*
* LICENSING TERMS:
* ---------------
*   uC/OS-II is provided in source form for FREE evaluation, for educational use or for peaceful research.
* If you plan on using  uC/OS-II  in a commercial product you need to contact Micrium to properly license
* its use in your product. We provide ALL the source code for your convenience and to help you experience
* uC/OS-II.   The fact that the  source is provided does  NOT  mean that you can use it without  paying a
* licensing fee.
*********************************************************************************************************
*/

#define  MICRIUM_SOURCE

#ifndef  OS_MASTER_FILE
#include <ucos_ii.h>
#endif

#if (OS_MEM_EN > 0u) && (OS_MAX_MEM_PART > 0u)
/*
*********************************************************************************************************
*                                      CREATE A MEMORY PARTITION
*
* Description : Create a fixed-sized memory partition that will be managed by uC/OS-II.
*
* Arguments   : addr     is the starting address of the memory partition
*
*               nblks    is the number of memory blocks to create from the partition.
*
*               blksize  is the size (in bytes) of each block in the memory partition.
*
*               perr     is a pointer to a variable containing an error message which will be set by
*                        this function to either:
*
*                        OS_ERR_NONE              if the memory partition has been created correctly.
*                        OS_ERR_MEM_INVALID_ADDR  if you are specifying an invalid address for the memory
*                                                 storage of the partition or, the block does not align
*                                                 on a pointer boundary
*                        OS_ERR_MEM_INVALID_PART  no free partitions available
*                        OS_ERR_MEM_INVALID_BLKS  user specified an invalid number of blocks (must be >= 2)
*                        OS_ERR_MEM_INVALID_SIZE  user specified an invalid block size
*                                                   - must be greater than the size of a pointer
*                                                   - must be able to hold an integral number of pointers
* Returns    : != (OS_MEM *)0  is the partition was created
*              == (OS_MEM *)0  if the partition was not created because of invalid arguments or, no
*                              free partition is available.
*********************************************************************************************************
*/

OS_MEM  *OSMemCreate (void   *addr,
                      INT32U  nblks,
                      INT32U  blksize,
                      INT8U  *perr)
{
    OS_MEM    *pmem;
    INT8U     *pblk;
    void     **plink;
    INT32U     loops;
    INT32U     i;
#if OS_CRITICAL_METHOD == 3u                          /* Allocate storage for CPU status register      */
    OS_CPU_SR  cpu_sr = 0u;
#endif



#ifdef OS_SAFETY_CRITICAL
    if (perr == (INT8U *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return ((OS_MEM *)0);
    }
#endif

#ifdef OS_SAFETY_CRITICAL_IEC61508
    if (OSSafetyCriticalStartFlag == OS_TRUE) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return ((OS_MEM *)0);
    }
#endif

#if OS_ARG_CHK_EN > 0u
    if (addr == (void *)0) {                          /* Must pass a valid address for the memory part.*/
        *perr = OS_ERR_MEM_INVALID_ADDR;
        return ((OS_MEM *)0);
    }
    if (((INT32U)addr & (sizeof(void *) - 1u)) != 0u){  /* Must be pointer size aligned                */
        *perr = OS_ERR_MEM_INVALID_ADDR;
        return ((OS_MEM *)0);
    }
    if (nblks < 2u) {                                 /* Must have at least 2 blocks per partition     */
        *perr = OS_ERR_MEM_INVALID_BLKS;
        return ((OS_MEM *)0);
    }
    if (blksize < sizeof(void *)) {                   /* Must contain space for at least a pointer     */
        *perr = OS_ERR_MEM_INVALID_SIZE;
        return ((OS_MEM *)0);
    }
#endif
    OS_ENTER_CRITICAL();
    pmem = OSMemFreeList;                             /* Get next free memory partition                */
    if (OSMemFreeList != (OS_MEM *)0) {               /* See if pool of free partitions was empty      */
        OSMemFreeList = (OS_MEM *)OSMemFreeList->OSMemFreeList;
    }
    OS_EXIT_CRITICAL();
    if (pmem == (OS_MEM *)0) {                        /* See if we have a memory partition             */
        *perr = OS_ERR_MEM_INVALID_PART;
        return ((OS_MEM *)0);
    }
    plink = (void **)addr;                            /* Create linked list of free memory blocks      */
    pblk  = (INT8U *)addr;
    loops  = nblks - 1u;
    for (i = 0u; i < loops; i++) {
        pblk +=  blksize;                             /* Point to the FOLLOWING block                  */
       *plink = (void  *)pblk;                        /* Save pointer to NEXT block in CURRENT block   */
        plink = (void **)pblk;                        /* Position to  NEXT      block                  */
    }
    *plink              = (void *)0;                  /* Last memory block points to NULL              */
    pmem->OSMemAddr     = addr;                       /* Store start address of memory partition       */
    pmem->OSMemFreeList = addr;                       /* Initialize pointer to pool of free blocks     */
    pmem->OSMemNFree    = nblks;                      /* Store number of free blocks in MCB            */
    pmem->OSMemNBlks    = nblks;
    pmem->OSMemBlkSize  = blksize;                    /* Store block size of each memory blocks        */
    *perr               = OS_ERR_NONE;
    return (pmem);
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                         GET A MEMORY BLOCK
*
* Description : Get a memory block from a partition
*
* Arguments   : pmem    is a pointer to the memory partition control block
*
*               perr    is a pointer to a variable containing an error message which will be set by this
*                       function to either:
*
*                       OS_ERR_NONE             if the memory partition has been created correctly.
*                       OS_ERR_MEM_NO_FREE_BLKS if there are no more free memory blocks to allocate to caller
*                       OS_ERR_MEM_INVALID_PMEM if you passed a NULL pointer for 'pmem'
*
* Returns     : A pointer to a memory block if no error is detected
*               A pointer to NULL if an error is detected
*********************************************************************************************************
*/

void  *OSMemGet (OS_MEM  *pmem,
                 INT8U   *perr)
{
    void      *pblk;
#if OS_CRITICAL_METHOD == 3u                          /* Allocate storage for CPU status register      */
    OS_CPU_SR  cpu_sr = 0u;
#endif



#ifdef OS_SAFETY_CRITICAL
    if (perr == (INT8U *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return ((void *)0);
    }
#endif

#if OS_ARG_CHK_EN > 0u
    if (pmem == (OS_MEM *)0) {                        /* Must point to a valid memory partition        */
        *perr = OS_ERR_MEM_INVALID_PMEM;
        return ((void *)0);
    }
#endif
    OS_ENTER_CRITICAL();
    if (pmem->OSMemNFree > 0u) {                      /* See if there are any free memory blocks       */
        pblk                = pmem->OSMemFreeList;    /* Yes, point to next free memory block          */
        pmem->OSMemFreeList = *(void **)pblk;         /*      Adjust pointer to new free list          */
        pmem->OSMemNFree--;                           /*      One less memory block in this partition  */
        OS_EXIT_CRITICAL();
        *perr = OS_ERR_NONE;                          /*      No error                                 */
        return (pblk);                                /*      Return memory block to caller            */
    }
    OS_EXIT_CRITICAL();
    *perr = OS_ERR_MEM_NO_FREE_BLKS;                  /* No,  Notify caller of empty memory partition  */
    return ((void *)0);                               /*      Return NULL pointer to caller            */
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                 GET THE NAME OF A MEMORY PARTITION
*
* Description: This function is used to obtain the name assigned to a memory partition.
*
* Arguments  : pmem      is a pointer to the memory partition
*
*              pname     is a pointer to a pointer to an ASCII string that will receive the name of the memory partition.
*
*              perr      is a pointer to an error code that can contain one of the following values:
*
*                        OS_ERR_NONE                if the name was copied to 'pname'
*                        OS_ERR_MEM_INVALID_PMEM    if you passed a NULL pointer for 'pmem'
*                        OS_ERR_PNAME_NULL          You passed a NULL pointer for 'pname'
*                        OS_ERR_NAME_GET_ISR        You called this function from an ISR
*
* Returns    : The length of the string or 0 if 'pmem' is a NULL pointer.
*********************************************************************************************************
*/

#if OS_MEM_NAME_EN > 0u
INT8U  OSMemNameGet (OS_MEM   *pmem,
                     INT8U   **pname,
                     INT8U    *perr)
{
    INT8U      len;
#if OS_CRITICAL_METHOD == 3u                     /* Allocate storage for CPU status register           */
    OS_CPU_SR  cpu_sr = 0u;
#endif



#ifdef OS_SAFETY_CRITICAL
    if (perr == (INT8U *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return (0u);
    }
#endif

#if OS_ARG_CHK_EN > 0u
    if (pmem == (OS_MEM *)0) {                   /* Is 'pmem' a NULL pointer?                          */
        *perr = OS_ERR_MEM_INVALID_PMEM;
        return (0u);
    }
    if (pname == (INT8U **)0) {                  /* Is 'pname' a NULL pointer?                         */
        *perr = OS_ERR_PNAME_NULL;
        return (0u);
    }
#endif
    if (OSIntNesting > 0u) {                     /* See if trying to call from an ISR                  */
        *perr = OS_ERR_NAME_GET_ISR;
        return (0u);
    }
    OS_ENTER_CRITICAL();
    *pname = pmem->OSMemName;
    len    = OS_StrLen(*pname);
    OS_EXIT_CRITICAL();
    *perr  = OS_ERR_NONE;
    return (len);
}
#endif

/*$PAGE*/
/*
*********************************************************************************************************
*                                 ASSIGN A NAME TO A MEMORY PARTITION
*
* Description: This function assigns a name to a memory partition.
*
* Arguments  : pmem      is a pointer to the memory partition
*
*              pname     is a pointer to an ASCII string that contains the name of the memory partition.
*
*              perr      is a pointer to an error code that can contain one of the following values:
*
*                        OS_ERR_NONE                if the name was copied to 'pname'
*                        OS_ERR_MEM_INVALID_PMEM    if you passed a NULL pointer for 'pmem'
*                        OS_ERR_PNAME_NULL          You passed a NULL pointer for 'pname'
*                        OS_ERR_MEM_NAME_TOO_LONG   if the name doesn't fit in the storage area
*                        OS_ERR_NAME_SET_ISR        if you called this function from an ISR
*
* Returns    : None
*********************************************************************************************************
*/

#if OS_MEM_NAME_EN > 0u
void  OSMemNameSet (OS_MEM  *pmem,
                    INT8U   *pname,
                    INT8U   *perr)
{
#if OS_CRITICAL_METHOD == 3u                     /* Allocate storage for CPU status register           */
    OS_CPU_SR  cpu_sr = 0u;
#endif



#ifdef OS_SAFETY_CRITICAL
    if (perr == (INT8U *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return;
    }
#endif

#if OS_ARG_CHK_EN > 0u
    if (pmem == (OS_MEM *)0) {                   /* Is 'pmem' a NULL pointer?                          */
        *perr = OS_ERR_MEM_INVALID_PMEM;
        return;
    }
    if (pname == (INT8U *)0) {                   /* Is 'pname' a NULL pointer?                         */
        *perr = OS_ERR_PNAME_NULL;
        return;
    }
#endif
    if (OSIntNesting > 0u) {                     /* See if trying to call from an ISR                  */
        *perr = OS_ERR_NAME_SET_ISR;
        return;
    }
    OS_ENTER_CRITICAL();
    pmem->OSMemName = pname;
    OS_EXIT_CRITICAL();
    *perr           = OS_ERR_NONE;
}
#endif

/*$PAGE*/
/*
*********************************************************************************************************
*                                       RELEASE A MEMORY BLOCK
*
* Description : Returns a memory block to a partition
*
* Arguments   : pmem    is a pointer to the memory partition control block
*
*               pblk    is a pointer to the memory block being released.
*
* Returns     : OS_ERR_NONE              if the memory block was inserted into the partition
*               OS_ERR_MEM_FULL          if you are returning a memory block to an already FULL memory
*                                        partition (You freed more blocks than you allocated!)
*               OS_ERR_MEM_INVALID_PMEM  if you passed a NULL pointer for 'pmem'
*               OS_ERR_MEM_INVALID_PBLK  if you passed a NULL pointer for the block to release.
*********************************************************************************************************
*/

INT8U  OSMemPut (OS_MEM  *pmem,
                 void    *pblk)
{
#if OS_CRITICAL_METHOD == 3u                     /* Allocate storage for CPU status register           */
    OS_CPU_SR  cpu_sr = 0u;
#endif



#if OS_ARG_CHK_EN > 0u
    if (pmem == (OS_MEM *)0) {                   /* Must point to a valid memory partition             */
        return (OS_ERR_MEM_INVALID_PMEM);
    }
    if (pblk == (void *)0) {                     /* Must release a valid block                         */
        return (OS_ERR_MEM_INVALID_PBLK);
    }
#endif
    OS_ENTER_CRITICAL();
    if (pmem->OSMemNFree >= pmem->OSMemNBlks) {  /* Make sure all blocks not already returned          */
        OS_EXIT_CRITICAL();
        return (OS_ERR_MEM_FULL);
    }
    *(void **)pblk      = pmem->OSMemFreeList;   /* Insert released block into free block list         */
    pmem->OSMemFreeList = pblk;
    pmem->OSMemNFree++;                          /* One more memory block in this partition            */
    OS_EXIT_CRITICAL();
    return (OS_ERR_NONE);                        /* Notify caller that memory block was released       */
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                       QUERY MEMORY PARTITION
*
* Description : This function is used to determine the number of free memory blocks and the number of
*               used memory blocks from a memory partition.
*
* Arguments   : pmem        is a pointer to the memory partition control block
*
*               p_mem_data  is a pointer to a structure that will contain information about the memory
*                           partition.
*
* Returns     : OS_ERR_NONE               if no errors were found.
*               OS_ERR_MEM_INVALID_PMEM   if you passed a NULL pointer for 'pmem'
*               OS_ERR_MEM_INVALID_PDATA  if you passed a NULL pointer to the data recipient.
*********************************************************************************************************
*/

#if OS_MEM_QUERY_EN > 0u
INT8U  OSMemQuery (OS_MEM       *pmem,
                   OS_MEM_DATA  *p_mem_data)
{
#if OS_CRITICAL_METHOD == 3u                     /* Allocate storage for CPU status register           */
    OS_CPU_SR  cpu_sr = 0u;
#endif



#if OS_ARG_CHK_EN > 0u
    if (pmem == (OS_MEM *)0) {                   /* Must point to a valid memory partition             */
        return (OS_ERR_MEM_INVALID_PMEM);
    }
    if (p_mem_data == (OS_MEM_DATA *)0) {        /* Must release a valid storage area for the data     */
        return (OS_ERR_MEM_INVALID_PDATA);
    }
#endif
    OS_ENTER_CRITICAL();
    p_mem_data->OSAddr     = pmem->OSMemAddr;
    p_mem_data->OSFreeList = pmem->OSMemFreeList;
    p_mem_data->OSBlkSize  = pmem->OSMemBlkSize;
    p_mem_data->OSNBlks    = pmem->OSMemNBlks;
    p_mem_data->OSNFree    = pmem->OSMemNFree;
    OS_EXIT_CRITICAL();
    p_mem_data->OSNUsed    = p_mem_data->OSNBlks - p_mem_data->OSNFree;
    return (OS_ERR_NONE);
}
#endif                                           /* OS_MEM_QUERY_EN                                    */
/*$PAGE*/
/*
*********************************************************************************************************
*                                 INITIALIZE MEMORY PARTITION MANAGER
*
* Description : This function is called by uC/OS-II to initialize the memory partition manager.  Your
*               application MUST NOT call this function.
*
* Arguments   : none
*
* Returns     : none
*
* Note(s)    : This function is INTERNAL to uC/OS-II and your application should not call it.
*********************************************************************************************************
*/

void  OS_MemInit (void)
{
#if OS_MAX_MEM_PART == 1u
    OS_MemClr((INT8U *)&OSMemTbl[0], sizeof(OSMemTbl));   /* Clear the memory partition table          */
    OSMemFreeList               = (OS_MEM *)&OSMemTbl[0]; /* Point to beginning of free list           */
#if OS_MEM_NAME_EN > 0u
    OSMemFreeList->OSMemName    = (INT8U *)"?";           /* Unknown name                              */
#endif
#endif

#if OS_MAX_MEM_PART >= 2u
    OS_MEM  *pmem;
    INT16U   i;


    OS_MemClr((INT8U *)&OSMemTbl[0], sizeof(OSMemTbl));   /* Clear the memory partition table          */
    for (i = 0u; i < (OS_MAX_MEM_PART - 1u); i++) {       /* Init. list of free memory partitions      */
        pmem                = &OSMemTbl[i];               /* Point to memory control block (MCB)       */
        pmem->OSMemFreeList = (void *)&OSMemTbl[i + 1u];  /* Chain list of free partitions             */
#if OS_MEM_NAME_EN > 0u
        pmem->OSMemName  = (INT8U *)(void *)"?";
#endif
    }
    pmem                = &OSMemTbl[i];
    pmem->OSMemFreeList = (void *)0;                      /* Initialize last node                      */
#if OS_MEM_NAME_EN > 0u
    pmem->OSMemName = (INT8U *)(void *)"?";
#endif

    OSMemFreeList   = &OSMemTbl[0];                       /* Point to beginning of free list           */
#endif
}
#endif                                                    /* OS_MEM_EN                                 */


/*------------------------------------------------------------------*/
INT32U two_exp(INT8U num){
	int exps[]={1,2,4,8,16,32,64,128,256,512,1024};
	if(num>10){
		return exps[10];
	}else{
		return exps[num];
	}
}

void display_memory(){
	INT16U i;
	INT16U j;
	OS_MEM_NEW* pmem;
	void* pblk;
	APP_TRACE("**********display_memory*********\n");
	for(i=0;i<OS_MAX_MEM_PART;i++){
		pmem=(OSMemTblNew+i);	
		APP_TRACE("blksum:%d,blksize:%d,freelist:%d,tail:%d\n",pmem->OSMemNBlks,pmem->OSMemBlkSize,pmem->OSMemFreeList,pmem->OSMemBlkTail);
		pblk=pmem->OSMemFreeList;
		j=0;
		while(pblk!=pmem->OSMemBlkTail&&j<pmem->OSMemNBlks){
			APP_TRACE("%d\n",pblk);
			pblk=*(void**)pblk;
			j++;
		}
	}
	APP_TRACE("*********************************\n");
}


void OSMemCreateNew(void* addr, INT32U nblks, INT32U granularity,INT8U *err ){
	INT8U i;
	INT8U j;
	INT32U mem_addition=0;
	OS_MEM_NEW *pmem;
	INT8U *pblk;
	void **plink;

	plink=(void**)addr;
	pblk=(INT8U*)addr+granularity*two_exp(0);

	for(i=0;i<OS_MAX_MEM_PART;i++){
		mem_addition+=two_exp(i)*granularity;
	}

	for(j=0;j<(nblks-1);j++){
		for(i=0;i<OS_MAX_MEM_PART;i++){
			pmem=(OSMemTblNew+i);
			if(j==0){
				pmem->OSMemBlkSize=granularity*two_exp(i); //the unit is B
				pmem->OSMemNBlks=nblks;
				pmem->OSMemFreeList=(void*)plink;
				//pmem->OSMemBlkTail=(void*)plink;
			}
			*plink=(void*)((INT8U*)plink+mem_addition);
			plink=(void**)pblk;
			pblk=pblk+two_exp((i+1)%OS_MAX_MEM_PART)*granularity;
		}
	}
	for(i=0;i<OS_MAX_MEM_PART;i++){
		APP_TRACE("index:%d tail:%d\n",i,(void*)plink);
		*plink=(void*)0;
		pmem=(OSMemTblNew+i);
		pmem->OSMemBlkTail=(void*)plink;
		plink=(void**)((INT8U*)plink+two_exp((i)%OS_MAX_MEM_PART)*granularity);
	}
	display_memory();
}

INT8U two_log(INT32U num){
	INT16U i;
	int exps[]={1,2,4,8,16,32,64,128,256,512,1024};
	int logs[]={0,1,2,3,4,5,6,7,8,9,10};
	if(num>1024){
		return 10;
	}else{
		if(num==1){
			return 0;
		}
		for(i=1;i<11;i++){
			if(num>exps[i-1]&&num<=exps[i]){
				return logs[i];
			}
		}
	}
	return 10;
}

void* OSMemGetNew(INT32U size, INT32U granularity, INT8U *err){
	INT8U *blk;
	INT8U index;
	INT16U i;
	OS_MEM_NEW *pmem;
	void* pblk;

	index=two_log(size);
	pmem=(OSMemTblNew+index);
	
	OS_ENTER_CRITICAL();
	if(pmem->OSMemNBlks>0){
		pmem->OSMemNBlks--;
		pblk=pmem->OSMemFreeList;
		pmem->OSMemFreeList=*(void**)pblk;
		if(pmem->OSMemNBlks==0){
			pmem->OSMemBlkTail=*(void**)pblk;
		}
		OS_EXIT_CRITICAL();
		APP_TRACE("In OSMemGetNew: size: %d\n",pmem->OSMemBlkSize);
		display_memory();
		return (pblk);
	}else{
		for(i=index+1;i<OS_MAX_MEM_PART;i++){
			pmem=(OSMemTblNew+i);
			APP_TRACE("need bigger index:%d, pmem blksize:%d, remain_blk_sum:%d\n",i,pmem->OSMemBlkSize,pmem->OSMemNBlks);
			if(pmem->OSMemNBlks>0){
				pmem->OSMemNBlks--;
				pblk=pmem->OSMemFreeList;
				pmem->OSMemFreeList=*(void**)pblk;
				APP_TRACE("In OSMemGetNew and for bigger: size: %d\n",pmem->OSMemBlkSize);
				pmem=(OSMemTblNew+i-1);
				blk=(INT8U*)pblk+pmem->OSMemBlkSize;
				pmem->OSMemBlkTail=(void*)blk;
				if(pmem->OSMemNBlks==0){
					pmem->OSMemFreeList=pmem->OSMemBlkTail;
				}else{
					*(void**)(pmem->OSMemBlkTail)=(void*)blk;
				}
				pmem->OSMemNBlks++;
				OS_EXIT_CRITICAL();
				display_memory();
				return (pblk);
			}
		}

	}
	OS_EXIT_CRITICAL();
	*err=OS_ERR_MEM_NO_FREE_BLKS;
	return ((void*)0);	
}
INT8U OSMemPutNew(INT32U size, INT32U granularity,void *pblk0){
	INT8U index;
	INT8U has_merge;
	INT16U i;
	INT16U j;
	OS_MEM_NEW *pmem;
	void *pblk;
	void *p_preblk;

	index=two_log(size);	
	OS_ENTER_CRITICAL();
	for(i=index;i<OS_MAX_MEM_PART-1;i++){
		pmem=(OSMemTblNew+i);
		pblk=pmem->OSMemFreeList;
		p_preblk=pmem->OSMemFreeList;
		j=0;
		has_merge=0;
		APP_TRACE("pmem->OSMemBlkTail:%d,blksize:%d,OSMemBlkFree:%d\n",pmem->OSMemBlkTail,pmem->OSMemBlkSize,pmem->OSMemFreeList);
		while(1){
			APP_TRACE("pblk:%d\n",pblk);
			if(pblk0==((void*)((INT8U*)pblk+pmem->OSMemBlkSize))){
				has_merge=1;
				//APP_TRACE("in put memory: operation merge block\n");
				pmem->OSMemNBlks--;
				if(pmem->OSMemNBlks==0){
					pmem->OSMemBlkTail=(void*)0;
					pmem->OSMemFreeList=(void*)0;
				}else{
					if(pmem->OSMemNBlks==1){
						if(pmem->OSMemFreeList==pblk){
							pmem->OSMemFreeList=pmem->OSMemBlkTail;
							*(void**)pmem->OSMemFreeList=(void*)0;
						}else{
							pmem->OSMemBlkTail=pmem->OSMemFreeList;
							*(void**)pmem->OSMemFreeList=(void*)0;
						}
					}else{
						*(void**)p_preblk=*(void**)pblk;
					}
				}
			/**	pmem=(OSMemTblNew+i+1);
				*(void**)pblk=pmem->OSMemFreeList;
				pmem->OSMemFreeList=pblk;
				if(pmem->OSMemNBlks==0){
					pmem->OSMemBlkTail=pmem->OSMemFreeList;
				}
				pmem->OSMemNBlks++;**/
				pblk0=pblk;
				break;
			}
			if(pblk0==((void*)((INT8U*)pblk-pmem->OSMemBlkSize))){
				has_merge=1;
				pmem->OSMemNBlks--;
				if(pmem->OSMemNBlks==0){
					pmem->OSMemBlkTail=(void*)0;
					pmem->OSMemFreeList=(void*)0;
				}else{
					if(pmem->OSMemNBlks==1){
						if(pmem->OSMemFreeList==pblk){
							pmem->OSMemFreeList=pmem->OSMemBlkTail;
							*(void**)pmem->OSMemFreeList=(void*)0;
						}else{
							pmem->OSMemBlkTail=pmem->OSMemFreeList;
							*(void**)pmem->OSMemFreeList=(void*)0;
						}
					}else{
						*(void**)p_preblk=*(void**)pblk;
					}
				}
				//pmem=(OSMemTblNew+i+1);
				//*(void**)pblk0=pmem->OSMemFreeList;
				//pmem->OSMemFreeList=pblk0;
				break;
			}
			if(pblk==pmem->OSMemBlkTail){
				break;
			}
			j++;
			p_preblk=pblk;
			pblk=*(void**)pblk;
		}
		//APP_TRACE("in put memory: %d\n",pblk);
		//display_memory();
		if(!has_merge){
			*(void**)pblk0=pmem->OSMemFreeList;
			pmem->OSMemFreeList=pblk0;
			if(pmem->OSMemNBlks==0){
				pmem->OSMemBlkTail=pmem->OSMemFreeList;
			}
			pmem->OSMemNBlks++;
			break;
		}
	}
	OS_EXIT_CRITICAL();
	APP_TRACE("return from put\n");
	display_memory();
	return 0;
}
/*------------------------------------------------------------------*/

void OS_MemInitNew(void){
	//OS_MEM_NEW *pmem;
	//INT16U i;

}

