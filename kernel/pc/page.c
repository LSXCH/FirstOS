#include <zjunix/page.h>
#include <zjunix/slab.h>
#include <zjunix/utils.h>
#include <driver/vga.h>

//map the physics addr pa to the vitural addr va, pgd is the page table
//attr is the attribution the page have
int do_one_mapping(pgd_t *pgd, unsigned int va, unsigned int pa, unsigned int attr)
{
	unsigned int pde_index, pte_index;
	unsigned int pde, *pt;

	//get the index of two level page table
	pde_index = va>>PGD_SHIFT;
	pte_index = (va>>PGD_SHIFT)&INDEX_MASK;

	//search the index
	pde = pgd[pde_index];
	pde = pde & PAGE_MASK;

	if(pde==0)//THE two level pagetable not exists
	{
		pde = (unsigned int)kmalloc(PAGE_SIZE);
		if(pde==0)
			return 1;
		kernel_memset((void*)pde, 0 ,PAGE_SIZE);
		pgd[pde_index] = pde;
		pgd[pde_index] = pgd[pde_index] & PAGE_MASK;
		pgd[pde_index] = pgd[pde_index] | attr;
	}
	else
	{
		pgd[pde_index] = pgd[pde_index] & PAGE_MASK;
		pgd[pde_index] = pgd[pde_index] | attr;
	}
	pt = (unsigned int*)pde;

#ifdef VMA_AREA_DEBUG
	kernel_printf("MAP VA:%x  PA:%x pde_index:%x  pde:%x\n", va,pa,pde_index,pde);
#endif
	//insert phy address into two level page
	pt[pte_index] = pa &PAGE_SIZE;
	pt[pte_index] = pt[pte_index] | attr;
	return 0;
}

//map to the phy page
int do_mapping(pgd_t *pgd, unsigned int va, unsigned int npage, unsigned int pa, unsigned int attr)
{
	int res;
	int i = 0;
	while(i<npage)
	{
		res = do_one_mapping(pgd, va ,pa,attr);
		if(res)
			return 1;

		va = va + PAGE_SIZE;
		pa = pa + PAGE_SIZE;
		i++;
	}
	return 0;
}