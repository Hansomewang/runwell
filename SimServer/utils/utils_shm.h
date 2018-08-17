/*
 * utils_shm.h
 *
 *  shared memory utility - for simple application to create, destroy, attach 
 *  a shared memory. optional semephores are created for exclusive, asynchronus
 *  control among processes.
 */
#ifndef _UTILS_SHM_INCLUDED_
#define _UTILS_SHM_INCLUDED_

typedef struct tagShmContext
{
	void *shm_ptr;	// shared memory virtual address.
	int		shm_size;	// shared memory size in number of bytes
	int		sem_id;		// semaphore ID for asynchronize control
	int		shm_id;		// shared memory ID
} SHMContext, *PSHMContext;

PSHMContext SHMC_Create( const char *path, int projid, int size );
PSHMContext SHMC_Attach( const char *path, int projid, int size );
void SHMC_Destroy( PSHMContext pshmc );
void SHMC_Detach( PSHMContext pshmc );
int SHMC_Wait(PSHMContext pshmc);		// reader side wait for data ready
int SHMC_Ready(PSHMContext pshmc);		// writer side wait-up all reader's wait in SHMC_Wait		
// Note - this is cooperative lock. every process invoke lock before access and unlock after access
int SHMC_Lock(PSHMContext pshmc);		// lock share memory for exclusive access
int SHMC_Unlock(PSHMContext pshmc);

// atomically operation on wait + lock, unlock + ready
int SHMC_UnlockAndReady(PSHMContext pshmc);
int SHMC_WaitAndLock(PSHMContext pshmc);

#endif