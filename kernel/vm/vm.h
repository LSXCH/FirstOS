#ifndef _VM_H
#define _VM_H

#include <zjunix/vm.h>
#define NULL 0
struct vm_area_struct* find_vma(struct mm_struct* mm, unsigned long addr);
unsigned long get_unmapped_area(unsigned long addr, unsigned long len, unsigned long flags);
struct vm_area_struct* find_vma_intersection(struct mm_struct* mm, unsigned long start_addr, unsigned long end_addr);
struct vm_area_struct* find_vma_and_prev(struct mm_struct* mm, unsigned long addr, struct vm_area_struct** prev);
void insert_vma_struct(struct mm_struct* mm, struct vm_area_struct* area);
void exit_map(struct mm_struct* mm);
void pgd_delete(pgd_t* pgd);




#endif