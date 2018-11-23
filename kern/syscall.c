/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>

#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/trap.h>
#include <kern/syscall.h>
#include <kern/console.h>
#include <kern/sched.h>

// Print a string to the system console.
// The string is exactly 'len' characters long.
// Destroys the environment on memory errors.
static void
sys_cputs(const char *s, size_t len)
{
	// Check that the user has permission to read memory [s, s+len).
	// Destroy the environment if not.

	// LAB 3: Your code here.
	user_mem_assert(curenv, (const void *) s, len, PTE_U);

	// Print the string supplied by the user.
	cprintf("%.*s", len, s);
}

// Read a character from the system console without blocking.
// Returns the character, or 0 if there is no input waiting.
static int
sys_cgetc(void)
{
	return cons_getc();
}

// Returns the current environment's envid.
static envid_t
sys_getenvid(void)
{
	return curenv->env_id;
}

// Destroy a given environment (possibly the currently running environment).
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_destroy(envid_t envid)
{
	int r;
	struct Env *e;

	if ((r = envid2env(envid, &e, 1)) < 0)
		return r;
	if (e == curenv)
		cprintf("[%08x] exiting gracefully\n", curenv->env_id);
	else
		cprintf("[%08x] destroying %08x\n", curenv->env_id, e->env_id);
	env_destroy(e);
	return 0;
}

// Deschedule current environment and pick a different one to run.
static void
sys_yield(void)
{
	sched_yield();
}

// Allocate a new environment.
// Returns envid of new environment, or < 0 on error.  Errors are:
//	-E_NO_FREE_ENV if no free environment is available.
//	-E_NO_MEM on memory exhaustion.
static envid_t
sys_exofork(void)
{
	// Create the new environment with env_alloc(), from kern/env.c.
	// It should be left as env_alloc created it, except that
	// status is set to ENV_NOT_RUNNABLE, and the register set is copied
	// from the current environment -- but tweaked so sys_exofork
	// will appear to return 0.

	// LAB 4: Your code here.
	// panic("sys_exofork not implemented");

	struct Env *new_env;
	envid_t parent_id = curenv->env_id;
	int r;
	// Inicializo un nuevo proceso
	if ((r = env_alloc(&new_env, parent_id)) < 0) {
		return r;
	}
	// Seteo el status del nuevo proceso
	new_env->env_status = ENV_NOT_RUNNABLE;
	// Le cargo los registros del curenv
	new_env->env_tf = curenv->env_tf;
	// Seteo el valor de retorno en 0 para el hijo
	new_env->env_tf.tf_regs.reg_eax = 0;
	
	return new_env->env_id;
}

// Set envid's env_status to status, which must be ENV_RUNNABLE
// or ENV_NOT_RUNNABLE.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if status is not a valid status for an environment.
static int
sys_env_set_status(envid_t envid, int status)
{
	// Hint: Use the 'envid2env' function from kern/env.c to translate an
	// envid to a struct Env.
	// You should set envid2env's third argument to 1, which will
	// check whether the current environment has permission to set
	// envid's status.

	// LAB 4: Your code here.
	// panic("sys_env_set_status not implemented");
	
	struct Env *e;
	// Chequeo que el status sea valido
	if ((status != ENV_RUNNABLE) && (status != ENV_NOT_RUNNABLE)) {
		return -E_INVAL;
	}
	// Obtengo el proceso y lo guardo en 'e'
	if (envid2env(envid, &e, 1) < 0) {
		return -E_BAD_ENV;
	}
	e->env_status = status;

	return 0;
}

// Set the page fault upcall for 'envid' by modifying the corresponding struct
// Env's 'env_pgfault_upcall' field.  When 'envid' causes a page fault, the
// kernel will push a fault record onto the exception stack, then branch to
// 'func'.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_set_pgfault_upcall(envid_t envid, void *func)
{
	// LAB 4: Your code here.
	// panic("sys_env_set_pgfault_upcall not implemented");

	struct Env *e;
	if (envid2env(envid, &e, 1) < 0) {
		return -E_BAD_ENV;
	}
	// Seteo el page fault entry point
	e->env_pgfault_upcall = func;

	return 0;
}

// Allocate a page of memory and map it at 'va' with permission
// 'perm' in the address space of 'envid'.
// The page's contents are set to 0.
// If a page is already mapped at 'va', that page is unmapped as a
// side effect.
//
// perm -- PTE_U | PTE_P must be set, PTE_AVAIL | PTE_W may or may not be set,
//         but no other bits may be set.  See PTE_SYSCALL in inc/mmu.h.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if va >= UTOP, or va is not page-aligned.
//	-E_INVAL if perm is inappropriate (see above).
//	-E_NO_MEM if there's no memory to allocate the new page,
//		or to allocate any necessary page tables.
static int
sys_page_alloc(envid_t envid, void *va, int perm)
{
	// Hint: This function is a wrapper around page_alloc() and
	//   page_insert() from kern/pmap.c.
	//   Most of the new code you write should be to check the
	//   parameters for correctness.
	//   If page_insert() fails, remember to free the page you
	//   allocated!

	// LAB 4: Your code here.
	// panic("sys_page_alloc not implemented");

	// Obtengo el env asociado al envid
	struct Env *e;
	if (envid2env(envid, &e, 1) < 0) {
		return -E_BAD_ENV;
	}
	// Chequeo la va
	bool va_ok = ((uintptr_t) va < UTOP) && ((uintptr_t) va % PGSIZE == 0);

	// Chequeo que (PTE_U | PTE_P) pertenezcan a perm
	// Y que perm pertenezca a PTE_SYSCALL.
	bool perm_ok = (perm == (perm | (PTE_U | PTE_P))) && (PTE_SYSCALL == (perm | PTE_SYSCALL));

	if ((!va_ok) || (!perm_ok)) {
		return -E_INVAL;
	}
	// Aloco una nueva pagina y la cargo con 0s
	struct PageInfo *new_page = page_alloc(ALLOC_ZERO);
	if (!new_page) {
		return -E_NO_MEM;
	}
	// Mapeo la nueva pagina a la direccion virtual va
	if (page_insert(e->env_pgdir, new_page, va, perm) < 0) {
		// Si falla page_insert, libero la pagina alocada
		page_free(new_page);
		return -E_NO_MEM;
	}
	return 0;
}

// Map the page of memory at 'srcva' in srcenvid's address space
// at 'dstva' in dstenvid's address space with permission 'perm'.
// Perm has the same restrictions as in sys_page_alloc, except
// that it also must not grant write access to a read-only
// page.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if srcenvid and/or dstenvid doesn't currently exist,
//		or the caller doesn't have permission to change one of them.
//	-E_INVAL if srcva >= UTOP or srcva is not page-aligned,
//		or dstva >= UTOP or dstva is not page-aligned.
//	-E_INVAL is srcva is not mapped in srcenvid's address space.
//	-E_INVAL if perm is inappropriate (see sys_page_alloc).
//	-E_INVAL if (perm & PTE_W), but srcva is read-only in srcenvid's
//		address space.
//	-E_NO_MEM if there's no memory to allocate any necessary page tables.
static int
sys_page_map(envid_t srcenvid, void *srcva, envid_t dstenvid, void *dstva, int perm)
{
	// Hint: This function is a wrapper around page_lookup() and
	//   page_insert() from kern/pmap.c.
	//   Again, most of the new code you write should be to check the
	//   parameters for correctness.
	//   Use the third argument to page_lookup() to
	//   check the current permissions on the page.

	// LAB 4: Your code here.
	// panic("sys_page_map not implemented");

	// Obtengo los env asociados a los envid
	struct Env *src_env;
	struct Env *dst_env;	
	if ((envid2env(srcenvid, &src_env, 1) < 0) || (envid2env(dstenvid, &dst_env, 1) < 0)) {
		return -E_BAD_ENV;
	}
	// Chequeo la va y los permisos
	bool srcva_ok = ((uintptr_t) srcva < UTOP) && ((uintptr_t) srcva % PGSIZE == 0);
	bool dstva_ok = ((uintptr_t) dstva < UTOP) && ((uintptr_t) dstva % PGSIZE == 0);
	bool perm_ok = (perm == (perm | (PTE_U | PTE_P))) && (PTE_SYSCALL == (perm | PTE_SYSCALL));
	if ((!srcva_ok) || (!dstva_ok) || (!perm_ok)) {
		return -E_INVAL;
	}
	// Obtengo la pagina mapeada en srcva
	pte_t *pgtab_entry;
	struct PageInfo *src_page = page_lookup(src_env->env_pgdir, srcva, &pgtab_entry);
	// Si page_lookup devuelve NULL quiere decir que srcva no esta mapeada
	// en el address space de srcenvid
	if (!src_page) {
		return -E_INVAL;
	}
	// Chequeo que el proceso no quiera mapear una pagina con PTE_W en una pagina sin PTE_W
	bool not_writeable = (perm == (perm | PTE_W)) && !(*pgtab_entry == (*pgtab_entry | PTE_W));
	if (not_writeable) {
		return -E_INVAL;
	}
	// Mapeo la pagina de srcva en dstva
	if (page_insert(dst_env->env_pgdir, src_page, dstva, perm) < 0) {
		return -E_NO_MEM;
	}
	return 0;
}

// Unmap the page of memory at 'va' in the address space of 'envid'.
// If no page is mapped, the function silently succeeds.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if va >= UTOP, or va is not page-aligned.
static int
sys_page_unmap(envid_t envid, void *va)
{
	// Hint: This function is a wrapper around page_remove().

	// LAB 4: Your code here.
	//panic("sys_page_unmap not implemented");

	// Obtengo el env asociado al envid
	struct Env *e;
	if (envid2env(envid, &e, 1) < 0) {
		return -E_BAD_ENV;
	}
	// Chequeo la va
	bool va_ok = ((uintptr_t) va < UTOP) && ((uintptr_t) va % PGSIZE == 0);
	if (!va_ok) {
		return -E_INVAL;
	}
	// Unmapeo la pagina en va. Si no hay pagina mapeada, no hace nada.
	page_remove(e->env_pgdir, va);
	return 0;
}

// Try to send 'value' to the target env 'envid'.
// If srcva < UTOP, then also send page currently mapped at 'srcva',
// so that receiver gets a duplicate mapping of the same page.
//
// The send fails with a return value of -E_IPC_NOT_RECV if the
// target is not blocked, waiting for an IPC.
//
// The send also can fail for the other reasons listed below.
//
// Otherwise, the send succeeds, and the target's ipc fields are
// updated as follows:
//    env_ipc_recving is set to 0 to block future sends;
//    env_ipc_from is set to the sending envid;
//    env_ipc_value is set to the 'value' parameter;
//    env_ipc_perm is set to 'perm' if a page was transferred, 0 otherwise.
// The target environment is marked runnable again, returning 0
// from the paused sys_ipc_recv system call.  (Hint: does the
// sys_ipc_recv function ever actually return?)
//
// If the sender wants to send a page but the receiver isn't asking for one,
// then no page mapping is transferred, but no error occurs.
// The ipc only happens when no errors occur.
//
// Returns 0 on success, < 0 on error.
// Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist.
//		(No need to check permissions.)
//	-E_IPC_NOT_RECV if envid is not currently blocked in sys_ipc_recv,
//		or another environment managed to send first.
//	-E_INVAL if srcva < UTOP but srcva is not page-aligned.
//	-E_INVAL if srcva < UTOP and perm is inappropriate
//		(see sys_page_alloc).
//	-E_INVAL if srcva < UTOP but srcva is not mapped in the caller's
//		address space.
//	-E_INVAL if (perm & PTE_W), but srcva is read-only in the
//		current environment's address space.
//	-E_NO_MEM if there's not enough memory to map srcva in envid's
//		address space.
static int
sys_ipc_try_send(envid_t envid, uint32_t value, void *srcva, unsigned perm)
{
	// LAB 4: Your code here.
	// panic("sys_ipc_try_send not implemented");

	int r;
	struct Env *e;
	// Chequeo que el exista el envid
	if ((r = envid2env(envid, &e, 0)) < 0) {
		return r;
	}
	// Chequeo que el reciever efectivamente este esperando un mensaje
	if (!e->env_ipc_recving) {
		return -E_IPC_NOT_RECV;
	}
	// Chequeo que srcva este alineada
	if (((uintptr_t) srcva < UTOP) && ((uintptr_t) srcva % PGSIZE != 0)) {
		return -E_INVAL;
	}
	// Chequeo los permisos
	bool perm_ok = (perm == (perm | (PTE_U | PTE_P))) && (PTE_SYSCALL == (perm | PTE_SYSCALL));
	if (((uintptr_t) srcva < UTOP) && (!perm_ok)) {
		return -E_INVAL;
	}
	// Chequeo que srcva este mapeada en el address space de curenv (caller)
	pte_t *pgtab_entry;
	struct PageInfo *src_page = page_lookup(curenv->env_pgdir, srcva, &pgtab_entry);
	if (((uintptr_t) srcva < UTOP) && (!src_page)) {
		return -E_INVAL;
	}
	// Chequeo que el proceso no quiera mapear una pagina con PTE_W en una pagina sin PTE_W
	bool not_writeable = (perm == (perm | PTE_W)) && !(*pgtab_entry == (*pgtab_entry | PTE_W));
	if (not_writeable) {
		return -E_INVAL;
	}
	// Comparto la pagina entre el caller y el receiver
	bool map_page = 0;
	if (((uintptr_t) srcva == UTOP) || ((uintptr_t) e->env_ipc_dstva == UTOP)) {
		if ((r = sys_page_map(curenv->env_id, srcva, envid, e->env_ipc_dstva, perm)) < 0) {
			return r;
		} else {
			map_page = 1;
		}
	}
	// Cargo los campos correspondientes del receiver
	e->env_ipc_recving = 0;
	e->env_ipc_from = curenv->env_id;
	e->env_ipc_value = value;
	e->env_ipc_perm = map_page ? perm : 0;
	e->env_status = ENV_RUNNABLE;

	return 0;
}

// Block until a value is ready.  Record that you want to receive
// using the env_ipc_recving and env_ipc_dstva fields of struct Env,
// mark yourself not runnable, and then give up the CPU.
//
// If 'dstva' is < UTOP, then you are willing to receive a page of data.
// 'dstva' is the virtual address at which the sent page should be mapped.
//
// This function only returns on error, but the system call will eventually
// return 0 on success.
// Return < 0 on error.  Errors are:
//	-E_INVAL if dstva < UTOP but dstva is not page-aligned.
static int
sys_ipc_recv(void *dstva)
{
	// LAB 4: Your code here.
	// panic("sys_ipc_recv not implemented");
	
	bool dstva_not_aligned = ((uintptr_t) dstva < UTOP) && ((uintptr_t) dstva % PGSIZE != 0);
	if (dstva_not_aligned) {
		return -E_INVAL;
	}
	// Marco el proceso como NOT_RUNNABLE
	curenv->env_status = ENV_NOT_RUNNABLE;
	curenv->env_ipc_recving = true;

	// Mapeo la pagina recibida
	curenv->env_ipc_dstva = dstva;

	// Cargo el valor de retorno
	curenv->env_tf.tf_regs.reg_eax = 0;

	sys_yield();

	return 0;
}

// Dispatches to the correct kernel function, passing the arguments.
int32_t
syscall(uint32_t syscallno, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5)
{
	// Call the function corresponding to the 'syscallno' parameter.
	// Return any appropriate return value.
	// LAB 3: Your code here.

	// panic("syscall not implemented");

	// Los cases son segun la convencion definida en inc\syscall.h
	switch (syscallno) {
		case SYS_cputs:
			sys_cputs((const char *) a1, (size_t) a2);
		case SYS_cgetc:
			return sys_cgetc();
		case SYS_getenvid:
			return sys_getenvid();
		case SYS_env_destroy:
			return sys_env_destroy((envid_t) a1);
		case SYS_yield:
			sched_yield();
		case SYS_exofork:
			return sys_exofork();
		case SYS_page_alloc:
			return sys_page_alloc((envid_t) a1, (void *) a2, (int) a3);
		case SYS_page_map:
			return sys_page_map((envid_t) a1, (void *) a2, (envid_t) a3, (void *) a4, (int) a5);
		case SYS_page_unmap:
			return sys_page_unmap((envid_t) a1, (void *) a2);
		case SYS_env_set_status:
			return sys_env_set_status((envid_t) a1, (int) a2);
		case SYS_ipc_try_send:
			return sys_ipc_try_send((envid_t) a1, (uint32_t) a2, (void *) a3, (unsigned) a4);
		case SYS_ipc_recv:
			return sys_ipc_recv((void *) a1);
		case SYS_env_set_pgfault_upcall:
			return sys_env_set_pgfault_upcall((envid_t) a1, (void *) a2);
		default:
			return -E_INVAL;
	}
}
