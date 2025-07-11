/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_POWERPC_BOOK3S_32_PGTABLE_H
#define _ASM_POWERPC_BOOK3S_32_PGTABLE_H

#include <asm-generic/pgtable-nopmd.h>

/*
 * The "classic" 32-bit implementation of the PowerPC MMU uses a hash
 * table containing PTEs, together with a set of 16 segment registers,
 * to define the virtual to physical address mapping.
 *
 * We use the hash table as an extended TLB, i.e. a cache of currently
 * active mappings.  We maintain a two-level page table tree, much
 * like that used by the i386, for the sake of the Linux memory
 * management code.  Low-level assembler code in hash_low_32.S
 * (procedure hash_page) is responsible for extracting ptes from the
 * tree and putting them into the hash table when necessary, and
 * updating the accessed and modified bits in the page table tree.
 */

#define _PAGE_PRESENT	0x001	/* software: pte contains a translation */
#define _PAGE_HASHPTE	0x002	/* hash_page has made an HPTE for this pte */
#define _PAGE_READ	0x004	/* software: read access allowed */
#define _PAGE_GUARDED	0x008	/* G: prohibit speculative access */
#define _PAGE_COHERENT	0x010	/* M: enforce memory coherence (SMP systems) */
#define _PAGE_NO_CACHE	0x020	/* I: cache inhibit */
#define _PAGE_WRITETHRU	0x040	/* W: cache write-through */
#define _PAGE_DIRTY	0x080	/* C: page changed */
#define _PAGE_ACCESSED	0x100	/* R: page referenced */
#define _PAGE_EXEC	0x200	/* software: exec allowed */
#define _PAGE_WRITE	0x400	/* software: user write access allowed */
#define _PAGE_SPECIAL	0x800	/* software: Special page */

#ifdef CONFIG_PTE_64BIT
/* We never clear the high word of the pte */
#define _PTE_NONE_MASK	(0xffffffff00000000ULL | _PAGE_HASHPTE)
#else
#define _PTE_NONE_MASK	_PAGE_HASHPTE
#endif

#define _PMD_PRESENT	0
#define _PMD_PRESENT_MASK (PAGE_MASK)
#define _PMD_BAD	(~PAGE_MASK)

/* We borrow the _PAGE_READ bit to store the exclusive marker in swap PTEs. */
#define _PAGE_SWP_EXCLUSIVE	_PAGE_READ

/* And here we include common definitions */

#define _PAGE_HPTEFLAGS _PAGE_HASHPTE

/*
 * Location of the PFN in the PTE. Most 32-bit platforms use the same
 * as _PAGE_SHIFT here (ie, naturally aligned).
 * Platform who don't just pre-define the value so we don't override it here.
 */
#define PTE_RPN_SHIFT	(PAGE_SHIFT)

/*
 * The mask covered by the RPN must be a ULL on 32-bit platforms with
 * 64-bit PTEs.
 */
#ifdef CONFIG_PTE_64BIT
#define PTE_RPN_MASK	(~((1ULL << PTE_RPN_SHIFT) - 1))
#define MAX_POSSIBLE_PHYSMEM_BITS 36
#else
#define PTE_RPN_MASK	(~((1UL << PTE_RPN_SHIFT) - 1))
#define MAX_POSSIBLE_PHYSMEM_BITS 32
#endif

/*
 * _PAGE_CHG_MASK masks of bits that are to be preserved across
 * pgprot changes.
 */
#define _PAGE_CHG_MASK	(PTE_RPN_MASK | _PAGE_HASHPTE | _PAGE_DIRTY | \
			 _PAGE_ACCESSED | _PAGE_SPECIAL)

/*
 * We define 2 sets of base prot bits, one for basic pages (ie,
 * cacheable kernel and user pages) and one for non cacheable
 * pages. We always set _PAGE_COHERENT when SMP is enabled or
 * the processor might need it for DMA coherency.
 */
#define _PAGE_BASE_NC	(_PAGE_PRESENT | _PAGE_ACCESSED)
#define _PAGE_BASE	(_PAGE_BASE_NC | _PAGE_COHERENT)

#include <asm/pgtable-masks.h>

/* Permission masks used for kernel mappings */
#define PAGE_KERNEL	__pgprot(_PAGE_BASE | _PAGE_KERNEL_RW)
#define PAGE_KERNEL_NC	__pgprot(_PAGE_BASE_NC | _PAGE_KERNEL_RW | _PAGE_NO_CACHE)
#define PAGE_KERNEL_NCG	__pgprot(_PAGE_BASE_NC | _PAGE_KERNEL_RW | _PAGE_NO_CACHE | _PAGE_GUARDED)
#define PAGE_KERNEL_X	__pgprot(_PAGE_BASE | _PAGE_KERNEL_RWX)
#define PAGE_KERNEL_RO	__pgprot(_PAGE_BASE | _PAGE_KERNEL_RO)
#define PAGE_KERNEL_ROX	__pgprot(_PAGE_BASE | _PAGE_KERNEL_ROX)

#define PTE_INDEX_SIZE	PTE_SHIFT
#define PMD_INDEX_SIZE	0
#define PUD_INDEX_SIZE	0
#define PGD_INDEX_SIZE	(32 - PGDIR_SHIFT)

#define PMD_CACHE_INDEX	PMD_INDEX_SIZE
#define PUD_CACHE_INDEX	PUD_INDEX_SIZE

#ifndef __ASSEMBLY__
#define PTE_TABLE_SIZE	(sizeof(pte_t) << PTE_INDEX_SIZE)
#define PMD_TABLE_SIZE	0
#define PUD_TABLE_SIZE	0
#define PGD_TABLE_SIZE	(sizeof(pgd_t) << PGD_INDEX_SIZE)

/* Bits to mask out from a PMD to get to the PTE page */
#define PMD_MASKED_BITS		(PTE_TABLE_SIZE - 1)
#endif	/* __ASSEMBLY__ */

#define PTRS_PER_PTE	(1 << PTE_INDEX_SIZE)
#define PTRS_PER_PGD	(1 << PGD_INDEX_SIZE)

/*
 * The normal case is that PTEs are 32-bits and we have a 1-page
 * 1024-entry pgdir pointing to 1-page 1024-entry PTE pages.  -- paulus
 *
 * For any >32-bit physical address platform, we can use the following
 * two level page table layout where the pgdir is 8KB and the MS 13 bits
 * are an index to the second level table.  The combined pgdir/pmd first
 * level has 2048 entries and the second level has 512 64-bit PTE entries.
 * -Matt
 */
/* PGDIR_SHIFT determines what a top-level page table entry can map */
#define PGDIR_SHIFT	(PAGE_SHIFT + PTE_INDEX_SIZE)
#define PGDIR_SIZE	(1UL << PGDIR_SHIFT)
#define PGDIR_MASK	(~(PGDIR_SIZE-1))

#define USER_PTRS_PER_PGD	(TASK_SIZE / PGDIR_SIZE)

#ifndef __ASSEMBLY__

int map_kernel_page(unsigned long va, phys_addr_t pa, pgprot_t prot);
void unmap_kernel_page(unsigned long va);

#endif /* !__ASSEMBLY__ */

/*
 * This is the bottom of the PKMAP area with HIGHMEM or an arbitrary
 * value (for now) on others, from where we can start layout kernel
 * virtual space that goes below PKMAP and FIXMAP
 */

#define FIXADDR_SIZE	0
#ifdef CONFIG_KASAN
#include <asm/kasan.h>
#define FIXADDR_TOP	(KASAN_SHADOW_START - PAGE_SIZE)
#else
#define FIXADDR_TOP	((unsigned long)(-PAGE_SIZE))
#endif

/*
 * ioremap_bot starts at that address. Early ioremaps move down from there,
 * until mem_init() at which point this becomes the top of the vmalloc
 * and ioremap space
 */
#ifdef CONFIG_HIGHMEM
#define IOREMAP_TOP	PKMAP_BASE
#else
#define IOREMAP_TOP	FIXADDR_START
#endif

/* PPC32 shares vmalloc area with ioremap */
#define IOREMAP_START	VMALLOC_START
#define IOREMAP_END	VMALLOC_END

/*
 * Just any arbitrary offset to the start of the vmalloc VM area: the
 * current 16MB value just means that there will be a 64MB "hole" after the
 * physical memory until the kernel virtual memory starts.  That means that
 * any out-of-bounds memory accesses will hopefully be caught.
 * The vmalloc() routines leaves a hole of 4kB between each vmalloced
 * area for the same reason. ;)
 *
 * We no longer map larger than phys RAM with the BATs so we don't have
 * to worry about the VMALLOC_OFFSET causing problems.  We do have to worry
 * about clashes between our early calls to ioremap() that start growing down
 * from ioremap_base being run into the VM area allocations (growing upwards
 * from VMALLOC_START).  For this reason we have ioremap_bot to check when
 * we actually run into our mappings setup in the early boot with the VM
 * system.  This really does become a problem for machines with good amounts
 * of RAM.  -- Cort
 */
#define VMALLOC_OFFSET (0x1000000) /* 16M */

#define VMALLOC_START ((((long)high_memory + VMALLOC_OFFSET) & ~(VMALLOC_OFFSET-1)))

#ifdef CONFIG_KASAN_VMALLOC
#define VMALLOC_END	ALIGN_DOWN(ioremap_bot, PAGE_SIZE << KASAN_SHADOW_SCALE_SHIFT)
#else
#define VMALLOC_END	ioremap_bot
#endif

#define MODULES_END	ALIGN_DOWN(PAGE_OFFSET, SZ_256M)
#define MODULES_SIZE	(CONFIG_MODULES_SIZE * SZ_1M)
#define MODULES_VADDR	(MODULES_END - MODULES_SIZE)

#ifndef __ASSEMBLY__
#include <linux/sched.h>
#include <linux/threads.h>

/* Bits to mask out from a PGD to get to the PUD page */
#define PGD_MASKED_BITS		0

#define pgd_ERROR(e) \
	pr_err("%s:%d: bad pgd %08lx.\n", __FILE__, __LINE__, pgd_val(e))
/*
 * Bits in a linux-style PTE.  These match the bits in the
 * (hardware-defined) PowerPC PTE as closely as possible.
 */

#define pte_clear(mm, addr, ptep) \
	do { pte_update(mm, addr, ptep, ~_PAGE_HASHPTE, 0, 0); } while (0)

#define pmd_none(pmd)		(!pmd_val(pmd))
#define	pmd_bad(pmd)		(pmd_val(pmd) & _PMD_BAD)
#define	pmd_present(pmd)	(pmd_val(pmd) & _PMD_PRESENT_MASK)
static inline void pmd_clear(pmd_t *pmdp)
{
	*pmdp = __pmd(0);
}


/*
 * When flushing the tlb entry for a page, we also need to flush the hash
 * table entry.  flush_hash_pages is assembler (for speed) in hashtable.S.
 */
extern int flush_hash_pages(unsigned context, unsigned long va,
			    unsigned long pmdval, int count);

/* Add an HPTE to the hash table */
extern void add_hash_page(unsigned context, unsigned long va,
			  unsigned long pmdval);

/* Flush an entry from the TLB/hash table */
static inline void flush_hash_entry(struct mm_struct *mm, pte_t *ptep, unsigned long addr)
{
	if (mmu_has_feature(MMU_FTR_HPTE_TABLE)) {
		unsigned long ptephys = __pa(ptep) & PAGE_MASK;

		flush_hash_pages(mm->context.id, addr, ptephys, 1);
	}
}

/*
 * PTE updates. This function is called whenever an existing
 * valid PTE is updated. This does -not- include set_pte_at()
 * which nowadays only sets a new PTE.
 *
 * Depending on the type of MMU, we may need to use atomic updates
 * and the PTE may be either 32 or 64 bit wide. In the later case,
 * when using atomic updates, only the low part of the PTE is
 * accessed atomically.
 */
static inline pte_basic_t pte_update(struct mm_struct *mm, unsigned long addr, pte_t *p,
				     unsigned long clr, unsigned long set, int huge)
{
	pte_basic_t old;

	if (mmu_has_feature(MMU_FTR_HPTE_TABLE)) {
		unsigned long tmp;

		asm volatile(
#ifndef CONFIG_PTE_64BIT
	"1:	lwarx	%0, 0, %3\n"
	"	andc	%1, %0, %4\n"
#else
	"1:	lwarx	%L0, 0, %3\n"
	"	lwz	%0, -4(%3)\n"
	"	andc	%1, %L0, %4\n"
#endif
	"	or	%1, %1, %5\n"
	"	stwcx.	%1, 0, %3\n"
	"	bne-	1b"
		: "=&r" (old), "=&r" (tmp), "=m" (*p)
#ifndef CONFIG_PTE_64BIT
		: "r" (p),
#else
		: "b" ((unsigned long)(p) + 4),
#endif
		  "r" (clr), "r" (set), "m" (*p)
		: "cc" );
	} else {
		old = pte_val(*p);

		*p = __pte((old & ~(pte_basic_t)clr) | set);
	}

	return old;
}

/*
 * 2.6 calls this without flushing the TLB entry; this is wrong
 * for our hash-based implementation, we fix that up here.
 */
#define __HAVE_ARCH_PTEP_TEST_AND_CLEAR_YOUNG
static inline int __ptep_test_and_clear_young(struct mm_struct *mm,
					      unsigned long addr, pte_t *ptep)
{
	unsigned long old;
	old = pte_update(mm, addr, ptep, _PAGE_ACCESSED, 0, 0);
	if (old & _PAGE_HASHPTE)
		flush_hash_entry(mm, ptep, addr);

	return (old & _PAGE_ACCESSED) != 0;
}
#define ptep_test_and_clear_young(__vma, __addr, __ptep) \
	__ptep_test_and_clear_young((__vma)->vm_mm, __addr, __ptep)

#define __HAVE_ARCH_PTEP_GET_AND_CLEAR
static inline pte_t ptep_get_and_clear(struct mm_struct *mm, unsigned long addr,
				       pte_t *ptep)
{
	return __pte(pte_update(mm, addr, ptep, ~_PAGE_HASHPTE, 0, 0));
}

#define __HAVE_ARCH_PTEP_SET_WRPROTECT
static inline void ptep_set_wrprotect(struct mm_struct *mm, unsigned long addr,
				      pte_t *ptep)
{
	pte_update(mm, addr, ptep, _PAGE_WRITE, 0, 0);
}

static inline void __ptep_set_access_flags(struct vm_area_struct *vma,
					   pte_t *ptep, pte_t entry,
					   unsigned long address,
					   int psize)
{
	unsigned long set = pte_val(entry) &
		(_PAGE_DIRTY | _PAGE_ACCESSED | _PAGE_RW | _PAGE_EXEC);

	pte_update(vma->vm_mm, address, ptep, 0, set, 0);

	flush_tlb_page(vma, address);
}

#define __HAVE_ARCH_PTE_SAME
#define pte_same(A,B)	(((pte_val(A) ^ pte_val(B)) & ~_PAGE_HASHPTE) == 0)

#define pmd_pfn(pmd)		(pmd_val(pmd) >> PAGE_SHIFT)
#define pmd_page(pmd)		pfn_to_page(pmd_pfn(pmd))

/*
 * Encode/decode swap entries and swap PTEs. Swap PTEs are all PTEs that
 * are !pte_none() && !pte_present().
 *
 * Format of swap PTEs (32bit PTEs):
 *
 *                         1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2 3 3
 *   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *   <----------------- offset --------------------> < type -> E H P
 *
 *   E is the exclusive marker that is not stored in swap entries.
 *   _PAGE_PRESENT (P) and __PAGE_HASHPTE (H) must be 0.
 *
 * For 64bit PTEs, the offset is extended by 32bit.
 */
#define __swp_type(entry)		((entry).val & 0x1f)
#define __swp_offset(entry)		((entry).val >> 5)
#define __swp_entry(type, offset)	((swp_entry_t) { ((type) & 0x1f) | ((offset) << 5) })
#define __pte_to_swp_entry(pte)		((swp_entry_t) { pte_val(pte) >> 3 })
#define __swp_entry_to_pte(x)		((pte_t) { (x).val << 3 })

static inline bool pte_swp_exclusive(pte_t pte)
{
	return pte_val(pte) & _PAGE_SWP_EXCLUSIVE;
}

static inline pte_t pte_swp_mkexclusive(pte_t pte)
{
	return __pte(pte_val(pte) | _PAGE_SWP_EXCLUSIVE);
}

static inline pte_t pte_swp_clear_exclusive(pte_t pte)
{
	return __pte(pte_val(pte) & ~_PAGE_SWP_EXCLUSIVE);
}

/* Generic accessors to PTE bits */
static inline bool pte_read(pte_t pte)
{
	return !!(pte_val(pte) & _PAGE_READ);
}

static inline bool pte_write(pte_t pte)
{
	return !!(pte_val(pte) & _PAGE_WRITE);
}

static inline int pte_dirty(pte_t pte)		{ return !!(pte_val(pte) & _PAGE_DIRTY); }
static inline int pte_young(pte_t pte)		{ return !!(pte_val(pte) & _PAGE_ACCESSED); }
static inline int pte_special(pte_t pte)	{ return !!(pte_val(pte) & _PAGE_SPECIAL); }
static inline int pte_none(pte_t pte)		{ return (pte_val(pte) & ~_PTE_NONE_MASK) == 0; }
static inline bool pte_exec(pte_t pte)		{ return pte_val(pte) & _PAGE_EXEC; }

static inline int pte_present(pte_t pte)
{
	return pte_val(pte) & _PAGE_PRESENT;
}

static inline bool pte_hw_valid(pte_t pte)
{
	return pte_val(pte) & _PAGE_PRESENT;
}

static inline bool pte_hashpte(pte_t pte)
{
	return !!(pte_val(pte) & _PAGE_HASHPTE);
}

static inline bool pte_ci(pte_t pte)
{
	return !!(pte_val(pte) & _PAGE_NO_CACHE);
}

/*
 * We only find page table entry in the last level
 * Hence no need for other accessors
 */
#define pte_access_permitted pte_access_permitted
static inline bool pte_access_permitted(pte_t pte, bool write)
{
	/*
	 * A read-only access is controlled by _PAGE_READ bit.
	 * We have _PAGE_READ set for WRITE
	 */
	if (!pte_present(pte) || !pte_read(pte))
		return false;

	if (write && !pte_write(pte))
		return false;

	return true;
}

/* Conversion functions: convert a page and protection to a page entry,
 * and a page entry and page directory to the page they refer to.
 *
 * Even if PTEs can be unsigned long long, a PFN is always an unsigned
 * long for now.
 */
static inline pte_t pfn_pte(unsigned long pfn, pgprot_t pgprot)
{
	return __pte(((pte_basic_t)(pfn) << PTE_RPN_SHIFT) |
		     pgprot_val(pgprot));
}

/* Generic modifiers for PTE bits */
static inline pte_t pte_wrprotect(pte_t pte)
{
	return __pte(pte_val(pte) & ~_PAGE_WRITE);
}

static inline pte_t pte_exprotect(pte_t pte)
{
	return __pte(pte_val(pte) & ~_PAGE_EXEC);
}

static inline pte_t pte_mkclean(pte_t pte)
{
	return __pte(pte_val(pte) & ~_PAGE_DIRTY);
}

static inline pte_t pte_mkold(pte_t pte)
{
	return __pte(pte_val(pte) & ~_PAGE_ACCESSED);
}

static inline pte_t pte_mkexec(pte_t pte)
{
	return __pte(pte_val(pte) | _PAGE_EXEC);
}

static inline pte_t pte_mkpte(pte_t pte)
{
	return pte;
}

static inline pte_t pte_mkwrite_novma(pte_t pte)
{
	/*
	 * write implies read, hence set both
	 */
	return __pte(pte_val(pte) | _PAGE_RW);
}

static inline pte_t pte_mkdirty(pte_t pte)
{
	return __pte(pte_val(pte) | _PAGE_DIRTY);
}

static inline pte_t pte_mkyoung(pte_t pte)
{
	return __pte(pte_val(pte) | _PAGE_ACCESSED);
}

static inline pte_t pte_mkspecial(pte_t pte)
{
	return __pte(pte_val(pte) | _PAGE_SPECIAL);
}

static inline pte_t pte_mkhuge(pte_t pte)
{
	return pte;
}

static inline pte_t pte_modify(pte_t pte, pgprot_t newprot)
{
	return __pte((pte_val(pte) & _PAGE_CHG_MASK) | pgprot_val(newprot));
}



/* This low level function performs the actual PTE insertion
 * Setting the PTE depends on the MMU type and other factors.
 *
 * First case is 32-bit in UP mode with 32-bit PTEs, we need to preserve
 * the _PAGE_HASHPTE bit since we may not have invalidated the previous
 * translation in the hash yet (done in a subsequent flush_tlb_xxx())
 * and see we need to keep track that this PTE needs invalidating.
 *
 * Second case is 32-bit with 64-bit PTE.  In this case, we
 * can just store as long as we do the two halves in the right order
 * with a barrier in between. This is possible because we take care,
 * in the hash code, to pre-invalidate if the PTE was already hashed,
 * which synchronizes us with any concurrent invalidation.
 * In the percpu case, we fallback to the simple update preserving
 * the hash bits (ie, same as the non-SMP case).
 *
 * Third case is 32-bit in SMP mode with 32-bit PTEs. We use the
 * helper pte_update() which does an atomic update. We need to do that
 * because a concurrent invalidation can clear _PAGE_HASHPTE. If it's a
 * per-CPU PTE such as a kmap_atomic, we also do a simple update preserving
 * the hash bits instead.
 */
static inline void __set_pte_at(struct mm_struct *mm, unsigned long addr,
				pte_t *ptep, pte_t pte, int percpu)
{
	if ((!IS_ENABLED(CONFIG_SMP) && !IS_ENABLED(CONFIG_PTE_64BIT)) || percpu) {
		*ptep = __pte((pte_val(*ptep) & _PAGE_HASHPTE) |
			      (pte_val(pte) & ~_PAGE_HASHPTE));
	} else if (IS_ENABLED(CONFIG_PTE_64BIT)) {
		if (pte_val(*ptep) & _PAGE_HASHPTE)
			flush_hash_entry(mm, ptep, addr);

		asm volatile("stw%X0 %2,%0; eieio; stw%X1 %L2,%1" :
			     "=m" (*ptep), "=m" (*((unsigned char *)ptep+4)) :
			     "r" (pte) : "memory");
	} else {
		pte_update(mm, addr, ptep, ~_PAGE_HASHPTE, pte_val(pte), 0);
	}
}

/*
 * Macro to mark a page protection value as "uncacheable".
 */

#define _PAGE_CACHE_CTL	(_PAGE_COHERENT | _PAGE_GUARDED | _PAGE_NO_CACHE | \
			 _PAGE_WRITETHRU)

#define pgprot_noncached pgprot_noncached
static inline pgprot_t pgprot_noncached(pgprot_t prot)
{
	return __pgprot((pgprot_val(prot) & ~_PAGE_CACHE_CTL) |
			_PAGE_NO_CACHE | _PAGE_GUARDED);
}

#define pgprot_noncached_wc pgprot_noncached_wc
static inline pgprot_t pgprot_noncached_wc(pgprot_t prot)
{
	return __pgprot((pgprot_val(prot) & ~_PAGE_CACHE_CTL) |
			_PAGE_NO_CACHE);
}

#define pgprot_cached pgprot_cached
static inline pgprot_t pgprot_cached(pgprot_t prot)
{
	return __pgprot((pgprot_val(prot) & ~_PAGE_CACHE_CTL) |
			_PAGE_COHERENT);
}

#define pgprot_cached_wthru pgprot_cached_wthru
static inline pgprot_t pgprot_cached_wthru(pgprot_t prot)
{
	return __pgprot((pgprot_val(prot) & ~_PAGE_CACHE_CTL) |
			_PAGE_COHERENT | _PAGE_WRITETHRU);
}

#define pgprot_cached_noncoherent pgprot_cached_noncoherent
static inline pgprot_t pgprot_cached_noncoherent(pgprot_t prot)
{
	return __pgprot(pgprot_val(prot) & ~_PAGE_CACHE_CTL);
}

#define pgprot_writecombine pgprot_writecombine
static inline pgprot_t pgprot_writecombine(pgprot_t prot)
{
	return pgprot_noncached_wc(prot);
}

#endif /* !__ASSEMBLY__ */

#endif /*  _ASM_POWERPC_BOOK3S_32_PGTABLE_H */
