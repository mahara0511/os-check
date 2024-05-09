// #ifdef MM_PAGING
/*
 * PAGING based Memory Management
 * Virtual memory module mm/mm-vm.c
 */

#include "string.h"
#include "mm.h"
#include <stdlib.h>
#include <stdio.h>

/*enlist_vm_freerg_list - add new rg to freerg_list
 *@mm: memory region
 *@rg_elmt: new region
 *
 */
int enlist_vm_freerg_list(struct mm_struct *mm, struct vm_rg_struct rg_elmt)
{
	struct vm_rg_struct *rg_node = mm->mmap->vm_freerg_list;
	struct vm_rg_struct *new_rg = malloc(sizeof(struct vm_rg_struct));

	if (rg_elmt.rg_start >= rg_elmt.rg_end)
		return -1;
	rg_elmt.rg_next = rg_node;

	new_rg->rg_start = rg_elmt.rg_start;
	new_rg->rg_end = rg_elmt.rg_end;
	new_rg->rg_next = rg_elmt.rg_next;

	/* Enlist the new region */
	mm->mmap->vm_freerg_list = new_rg;

	return 0;
}

/*get_vma_by_num - get vm area by numID
 *@mm: memory region
 *@vmaid: ID vm area to alloc memory region
 *
 */
struct vm_area_struct *get_vma_by_num(struct mm_struct *mm, int vmaid)
{
	struct vm_area_struct *pvma = mm->mmap;

	if (mm->mmap == NULL)
		return NULL;

	int vmait = 0;

	while (vmait < vmaid)
	{
		if (pvma == NULL)
			return NULL;

		pvma = pvma->vm_next;
		++vmait;
	}

	return pvma;
}

/*get_symrg_byid - get mem region by region ID
 *@mm: memory region
 *@rgid: region ID act as symbol index of variable
 *
 */
struct vm_rg_struct *get_symrg_byid(struct mm_struct *mm, int rgid)
{
	if (rgid < 0 || rgid >= PAGING_MAX_SYMTBL_SZ)
		return NULL;

	return &mm->symrgtbl[rgid];
}

/*__alloc - allocate a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *@alloc_addr: address of allocated memory region
 *
 */
int __alloc(struct pcb_t *caller, int vmaid, int rgid, int size, int *alloc_addr)
{
	/*Allocate at the toproof */
	if (caller->mm->symrgtbl[rgid].rg_start < caller->mm->symrgtbl[rgid].rg_end)
		pgfree_data(caller, rgid); // free region if there are some data in it

	pthread_mutex_lock(&mem_lock);
	struct vm_rg_struct rgnode;

	if (get_free_vmrg_area(caller, vmaid, size, &rgnode) == 0)
	{
		caller->mm->symrgtbl[rgid].rg_start = rgnode.rg_start;
		caller->mm->symrgtbl[rgid].rg_end = rgnode.rg_end;

		*alloc_addr = rgnode.rg_start;

		pthread_mutex_unlock(&mem_lock);
		return 0;
	}

	/* TODO IMPLEMENTED: get_free_vmrg_area FAILED handle the region management (Fig.6)*/

	/*Attempt to increate limit to get space */
	struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
	int inc_sz = PAGING_PAGE_ALIGNSZ(size);
	// int inc_limit_ret
	int old_sbrk = cur_vma->sbrk;
	int new_sbrk = old_sbrk + size;

	/* TODO IMPLEMENTED: increase the limit
	 * inc_vma_limit(caller, vmaid, inc_sz)
	 */
	if (new_sbrk > cur_vma->vm_end)
		if (inc_vma_limit(caller, vmaid, inc_sz) < 0)
		{
			pthread_mutex_unlock(&mem_lock);
			return -1; // Fail to increase limit
		}

	/*Successful increase limit */
	cur_vma->sbrk = new_sbrk;
	caller->mm->symrgtbl[rgid].rg_start = old_sbrk;
	caller->mm->symrgtbl[rgid].rg_end = new_sbrk;

	*alloc_addr = old_sbrk;

	pthread_mutex_unlock(&mem_lock);
	return 0;
}

/*__free - remove a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *
 */
int __free(struct pcb_t *caller, int vmaid, int rgid)
{
	struct vm_rg_struct *rgnode;

	if (rgid < 0 || rgid >= PAGING_MAX_SYMTBL_SZ)
		return -1;

	pthread_mutex_lock(&mem_lock);

	/* TODO IMPLEMENTED: Manage the collect freed region to freerg_list */
	rgnode = get_symrg_byid(caller->mm, rgid);

	/* Enlist the obsoleted memory region */
	enlist_vm_freerg_list(caller->mm, *rgnode);

	rgnode->rg_start = rgnode->rg_end = 0;
	rgnode->rg_next = NULL;

	pthread_mutex_unlock(&mem_lock);
	return 0;
}

/*pgalloc - PAGING-based allocate a region memory
 *@proc:  Process executing the instruction
 *@size: allocated size
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */
int pgalloc(struct pcb_t *proc, uint32_t size, uint32_t reg_index)
{
	int addr;

	/* By default using vmaid = 0 */
	return __alloc(proc, 0, reg_index, size, &addr);
}

/*pgfree - PAGING-based free a region memory
 *@proc: Process executing the instruction
 *@size: allocated size
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */

int pgfree_data(struct pcb_t *proc, uint32_t reg_index)
{
	return __free(proc, 0, reg_index);
}

/*pg_getpage - get the page in ram
 *@mm: memory region
 *@pagenum: PGN
 *@framenum: return FPN
 *@caller: caller
 *
 */
int pg_getpage(struct mm_struct *mm, int pgn, int *fpn, struct pcb_t *caller)
{
	uint32_t pte = mm->pgd[pgn];

	if (!PAGING_PAGE_PRESENT(pte))
	{ /* Page is not online, make it actively living */
		// int vicfpn;
		// uint32_t vicpte;

		// int tgtfpn = PAGING_SWP(pte); // the target frame storing our variable

		/* TODO IMPLEMENTED: Play with your paging theory here */

		/* Find victim page */
		int vicpgn;
		if (find_victim_page(caller->mm, &vicpgn) < 0)
			return -1;		  // There's no available page in page table
		while (vicpgn == pgn) // In case the page is not usuable
			if (find_victim_page(caller->mm, &vicpgn) < 0)
				return -1;

		int tgtfpn = PAGING_SWPOFF(pte); // target swap offset
		int tgttyp = PAGING_SWPTYP(pte); // target swap type

		/* Get swap type where victim frame goes to */

		int swptyp = tgttyp;
		int swpfpn;
		if (caller->mswp[swptyp]->maxsz <= 0)
			return -1;

		if (MEMPHY_get_freefp(caller->mswp[swptyp], &swpfpn) < 0 || swpfpn == tgtfpn)
			for (swptyp = 0; swptyp < PAGING_MAX_MMSWP; ++swptyp)
			{
				if (swptyp == tgttyp || caller->mswp[swptyp]->maxsz <= 0)
					continue;

				if (MEMPHY_get_freefp(caller->mswp[swptyp], &swpfpn) == 0)
					break;
				else
					swpfpn = -1;
			}

		if (swpfpn == -1)
			return -1;

		/* Do swap frame from MEMRAM to MEMSWP and vice versa*/

		/* Copy victim frame to swap */
		int vicfpn = PAGING_FPN(mm->pgd[vicpgn]);
		__swap_cp_page(caller->mram, vicfpn, caller->mswp[swptyp], swpfpn);

		/* Copy target frame from swap to mem */
		__swap_cp_page(caller->mswp[tgttyp], tgtfpn, caller->mram, vicfpn);

		/* Update page table */

		// pte_set_swap() &mm->pgd[vicpgn];
		pte_set_swap(&mm->pgd[vicpgn], swptyp, swpfpn);

		/* Update its online status of the target page */
		// pte_set_fpn() & mm->pgd[pgn];
		pte_set_fpn(&mm->pgd[pgn], vicfpn);

		enlist_pgn_node(&caller->mm->fifo_pgn, pgn);
		pte = mm->pgd[pgn];
	}

	*fpn = PAGING_FPN(pte);

	return 0;
}

/*pg_getval - read value at given offset
 *@mm: memory region
 *@addr: virtual address to acess
 *@value: value
 *
 */
int pg_getval(struct mm_struct *mm, int addr, BYTE *data, struct pcb_t *caller)
{
	int pgn = PAGING_PGN(addr);
	int off = PAGING_OFFST(addr);
	int fpn;

	/* Get the page to MEMRAM, swap from MEMSWAP if needed */
	if (pg_getpage(mm, pgn, &fpn, caller) != 0)
	{
#ifdef MMDBG
		printf("//////// Fail to get frame number from page address %d\n", addr);
#endif
		return -1; /* invalid page access */
	}

	int phyaddr = (fpn << PAGING_ADDR_FPN_LOBIT) + off;

	return MEMPHY_read(caller->mram, phyaddr, data);
}

/*pg_setval - write value to given offset
 *@mm: memory region
 *@addr: virtual address to acess
 *@value: value
 *
 */
int pg_setval(struct mm_struct *mm, int addr, BYTE value, struct pcb_t *caller)
{
	int pgn = PAGING_PGN(addr);
	int off = PAGING_OFFST(addr);
	int fpn;

	/* Get the page to MEMRAM, swap from MEMSWAP if needed */
	if (pg_getpage(mm, pgn, &fpn, caller) != 0)
	{
#ifdef MMDBG
		printf("//////// Fail to get frame number from page address %d\n", addr);
#endif
		return -1; /* invalid page access */
	}

	int phyaddr = (fpn << PAGING_ADDR_FPN_LOBIT) + off;

	return MEMPHY_write(caller->mram, phyaddr, value);
}

/*__read - read value in region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@offset: offset to acess in memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *
 */
int __read(struct pcb_t *caller, int vmaid, int rgid, int offset, BYTE *data)
{
	struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);

	struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

	if (currg == NULL || cur_vma == NULL) /* Invalid memory identify */
		return -1;

	if (currg->rg_start >= currg->rg_end) /* Region hasn't been allocated */
	{
#ifdef MMDBG
		printf("//////// Can't execute 'read region=%d offset=%d'\n", rgid, offset);
		printf("//////// since process %d has not allocated register %d yet\n", caller->pid, rgid);
#endif
		return -1;
	}

	if (currg->rg_start + offset >= currg->rg_end) /* Offset outside of region */
	{
#ifdef MMDBG
		printf("//////// Can't execute 'read region=%d offset=%d'\n", rgid, offset);
		printf("//////// since address is out of range in register %d of process %d\n", rgid, caller->pid);
#endif
		return -1;
	}

	if (pg_getval(caller->mm, currg->rg_start + offset, data, caller) < 0)
	{
#ifdef MMDBG
		printf("//////// Can't execute 'read region=%d offset=%d'\n", rgid, offset);
		printf("//////// since error occurs in pg_getval call from process %d\n", caller->pid);
#endif
		return -1;
	}

	return 0;
}

/*pgwrite - PAGING-based read a region memory */
int pgread(
	struct pcb_t *proc, // Process executing the instruction
	uint32_t source,	// Index of source register
	uint32_t offset,	// Source address = [source] + [offset]
	uint32_t destination)
{
	pthread_mutex_lock(&mem_lock);

	BYTE data;
	int val = __read(proc, 0, source, offset, &data);
	if (val < 0)
	{
		pthread_mutex_unlock(&mem_lock);
		return val;
	}
	
	destination = (uint32_t)data;
#ifdef IODUMP
	printf("read region=%d offset=%d value=%d\n", source, offset, data);
#ifdef PAGETBL_DUMP
	print_pgtbl(proc, 0, -1); // print max TBL
#endif
	MEMPHY_dump(proc->mram);
#endif

	pthread_mutex_unlock(&mem_lock);
	return val;
}

/*__write - write a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@offset: offset to acess in memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *
 */
int __write(struct pcb_t *caller, int vmaid, int rgid, int offset, BYTE value)
{
	struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);

	struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

	if (currg == NULL || cur_vma == NULL) /* Invalid memory identify */
		return -1;

	if (currg->rg_start >= currg->rg_end) /* Region hasn't been allocated */
	{
#ifdef MMDBG
		printf("//////// Can't execute 'write region=%d offset=%d value=%d'\n", rgid, offset, value);
		printf("//////// since process %d has not allocated register %d yet\n", caller->pid, rgid);
#endif
		return -1;
	}

	if (currg->rg_start + offset >= currg->rg_end) /* Offset outside of region */
	{
#ifdef MMDBG
		printf("//////// Can't execute 'write region=%d offset=%d value=%d'\n", rgid, offset, value);
		printf("//////// since address is out of range in register %d of process %d\n", rgid, caller->pid);
#endif
		return -1;
	}

	if (pg_setval(caller->mm, currg->rg_start + offset, value, caller) < 0)
	{
#ifdef MMDBG
		printf("//////// Can't execute 'write region=%d offset=%d value=%d'\n", rgid, offset, value);
		printf("//////// since error occurs in pg_setval call from process %d\n", caller->pid);
#endif
		return -1;
	};

	return 0;
}

/*pgwrite - PAGING-based write a region memory */
int pgwrite(
	struct pcb_t *proc,	  // Process executing the instruction
	BYTE data,			  // Data to be wrttien into memory
	uint32_t destination, // Index of destination register
	uint32_t offset)
{
	pthread_mutex_lock(&mem_lock);

	int val = __write(proc, 0, destination, offset, data);
	if (val < 0)
	{
		pthread_mutex_unlock(&mem_lock);
		return val;
	}

#ifdef IODUMP
	printf("write region=%d offset=%d value=%d\n", destination, offset, data);
#ifdef PAGETBL_DUMP
	print_pgtbl(proc, 0, -1); // print max TBL
#endif
	MEMPHY_dump(proc->mram);
#endif

	pthread_mutex_unlock(&mem_lock);
	return val;
}

/*free_pcb_memphy - collect all memphy of pcb
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@incpgnum: number of page
 */
int free_pcb_memph(struct pcb_t *caller)
{
	int pagenum, fpn, typ;
	uint32_t pte;

	for (pagenum = 0; pagenum < PAGING_MAX_PGN; pagenum++)
	{
		pte = caller->mm->pgd[pagenum];

		if (PAGING_PAGE_PRESENT(pte))
		{
			fpn = PAGING_FPN(pte);
			MEMPHY_put_freefp(caller->mram, fpn);
		}
		else
		{
			fpn = PAGING_SWPOFF(pte);
			typ = PAGING_SWPTYP(pte);
			MEMPHY_put_freefp(caller->mswp[typ], fpn);
		}
	}

	return 0;
}

/*get_vm_area_node - get vm area for a number of pages
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@incpgnum: number of page
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
struct vm_rg_struct *get_vm_area_node_at_brk(struct pcb_t *caller, int vmaid, int size, int alignedsz)
{
	struct vm_rg_struct *newrg;
	struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

	newrg = malloc(sizeof(struct vm_rg_struct));

	newrg->rg_start = cur_vma->sbrk;
	newrg->rg_end = newrg->rg_start + size;

	return newrg;
}

/*validate_overlap_vm_area
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
int validate_overlap_vm_area(struct pcb_t *caller, int vmaid, int vmastart, int vmaend)
{
	// struct vm_area_struct *vma = caller->mm->mmap;
	struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

	/* TODO IMPLEMENTED: validate the planned memory area is not overlapped */

	/* Iterate through vma list to do the checking */
	struct vm_area_struct *vmait;
	for (vmait = caller->mm->mmap; vmait != NULL; vmait = vmait->vm_next)
	{
		/* Only do the checking for other vma */
		if (vmait == cur_vma)
			continue;

		/* It's fine if intervals have only 1 endpoint overlapped */
		if (vmastart == vmait->vm_end && vmaend != vmait->vm_start)
			continue;
		if (vmaend == vmait->vm_start && vmastart != vmait->vm_end)
			continue;

		/* If above cases are not happening, intervals have overlapped */
		return -1;
	}

	/* After the loop end, intervals are guaranteed to be not overlapped */
	return 0;
}

/*inc_vma_limit - increase vm area limits to reserve space for new variable
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@inc_sz: increment size
 *
 */
int inc_vma_limit(struct pcb_t *caller, int vmaid, int inc_sz)
{
	struct vm_rg_struct *newrg = malloc(sizeof(struct vm_rg_struct));
	int inc_amt = PAGING_PAGE_ALIGNSZ(inc_sz);
	int incnumpage = inc_amt / PAGING_PAGESZ;

	struct vm_rg_struct *area = get_vm_area_node_at_brk(caller, vmaid, inc_sz, inc_amt);
	struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

	int old_end = cur_vma->vm_end;

	/* Validate overlap of obtained region */
	if (validate_overlap_vm_area(caller, vmaid, area->rg_start, area->rg_end) < 0)
	{ /* Overlap and failed allocation */
		free(newrg);
		free(area);
		return -1;
	}

	/* The obtained vm area (only)
	 * now will be alloc real ram region */
	cur_vma->vm_end += inc_sz;
	if (vm_map_ram(caller, area->rg_start, area->rg_end,
				   old_end, incnumpage, newrg) < 0) /* Map the memory to MEMRAM */
	{												/* Failed to map mem */
		free(newrg);
		free(area);
		return -1;
	}

	return 0;
}

/*find_victim_page - find victim page
 *@caller: caller
 *@pgn: return page number
 *
 */
int find_victim_page(struct mm_struct *mm, int *retpgn)
{
	struct pgn_t *pg = mm->fifo_pgn;

	/* TODO IMPLEMENTED: Implement the theorical mechanism to find the victim page */
	if (pg == NULL)
		return -1;

	*retpgn = pg->pgn;
	mm->fifo_pgn = pg->pg_next;
	free(pg);

	return 0;
}

/*get_free_vmrg_area - get a free vm region
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@size: allocated size
 *
 */
int get_free_vmrg_area(struct pcb_t *caller, int vmaid, int size, struct vm_rg_struct *newrg)
{
	struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

	struct vm_rg_struct *rgit = cur_vma->vm_freerg_list;

	if (rgit == NULL)
		return -1;

	/* Probe unintialized newrg */
	newrg->rg_start = newrg->rg_end = -1;

	/* Traverse on list of free vm region to find a fit space */
	while (rgit != NULL)
	{
		if (rgit->rg_start + size <= rgit->rg_end)
		{ /* Current region has enough space */
			newrg->rg_start = rgit->rg_start;
			newrg->rg_end = rgit->rg_start + size;

			/* Update left space in chosen region */
			if (rgit->rg_start + size < rgit->rg_end)
			{
				rgit->rg_start = rgit->rg_start + size;
			}
			else
			{ /*Use up all space, remove current node */
				/*Clone next rg node */
				struct vm_rg_struct *nextrg = rgit->rg_next;

				/*Cloning */
				if (nextrg != NULL)
				{
					rgit->rg_start = nextrg->rg_start;
					rgit->rg_end = nextrg->rg_end;

					rgit->rg_next = nextrg->rg_next;

					free(nextrg);
				}
				else
				{								   /*End of free list */
					rgit->rg_start = rgit->rg_end; // dummy, size 0 region
					rgit->rg_next = NULL;
				}
			}
			return 0;
		}
		else
		{
			rgit = rgit->rg_next; // Traverse next rg
		}
	}

	if (newrg->rg_start == -1) // new region not found
		return -1;

	return 0;
}

// #endif
