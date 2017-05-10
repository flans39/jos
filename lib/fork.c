// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	if (!(err&FEC_WR) || !(uvpt[PGNUM(addr)]&PTE_COW))
		panic("the faulting access was not a write, or not to a copy-on-write page");

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.
	addr = ROUNDDOWN(addr,PGSIZE);
	sys_page_alloc(0, PFTEMP, PTE_U|PTE_W|PTE_P);
	memcpy(PFTEMP, addr, PGSIZE);
	if (uvpt[PGNUM(addr)] & PTE_U)
		r = PTE_P|PTE_W|PTE_U;
	else
		r = PTE_P|PTE_W;
	sys_page_map(0, PFTEMP, 0, addr, r);
	sys_page_unmap(0, PFTEMP);
	// panic("pgfault not implemented");
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	// int r;

	// LAB 4: Your code here.
	// panic("duppage not implemented");
	void *va=(void*)(pn*PGSIZE);
	int perm=PTE_P|PTE_COW;
	if (uvpt[pn] & PTE_U)
		perm |= PTE_U;
	sys_page_map(0, va, envid, va, perm);
	sys_page_map(0, va, 0, va, perm);
	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	// panic("fork not implemented");
	set_pgfault_handler(pgfault);
	envid_t envid=sys_exofork();
	if (envid == 0)
		thisenv = envs+ENVX(sys_getenvid());
	else if (envid > 0) {
		for (unsigned i=0; i<UXSTACKTOP-PGSIZE; i+=PGSIZE) {
			if ((uvpd[PDX(i)]&PTE_P) && (uvpt[PGNUM(i)]&PTE_P)) {
				if ((uvpt[PGNUM(i)]&PTE_W) || (uvpt[PGNUM(i)]&PTE_COW))
					duppage(envid, PGNUM(i));
				else
					sys_page_map(0, (void*)(i*PGSIZE),
						envid, (void*)(i*PGSIZE), PTE_P|(uvpt[PGNUM(i)]&PTE_U));
			}
		}
		sys_page_alloc(envid, (void*)(UXSTACKTOP-PGSIZE), PTE_U|PTE_W|PTE_P);
		sys_env_set_pgfault_upcall(envid, thisenv->env_pgfault_upcall);
		sys_env_set_status(envid, ENV_RUNNABLE);
	}
	return envid;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
