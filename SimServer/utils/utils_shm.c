/*
 * utils_shm.c
 *
 * implement the very basic shared memory utility functions.
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include "utils_shm.h"

#if defined(__GNU_LIBRARY__) && !defined(_SEM_SEMUN_UNDEFINED)
	/* union semun is defined by including <sys/sem.h> */
#else
       /* according to X/OPEN we have to define it ourselves */
       union semun {
             int val;                  /* value for SETVAL */
             struct semid_ds *buf;     /* buffer for IPC_STAT, IPC_SET */
             unsigned short *array;    /* array for GETALL, SETALL */
                                       /* Linux specific part: */
             struct seminfo *__buf;    /* buffer for IPC_INFO */
       };
#endif

#define NUMSEM	2
#define SEM_ASYNC			0			// initial value shall be 0 means ready
#define SEM_SPINLOCK	1			// initial value shall be 1 means unlock
#define TRACE(fmt...)	printf(fmt)

/*
 * create shared memory, if it exits, return NULL 
 */
PSHMContext SHMC_Create( const char *path, int projid, int size )
{
	PSHMContext	shmc = NULL;
	int	dev_key;
	union semun arg;
	int	i;

	if ( (shmc = malloc( sizeof( SHMContext ) ) ) == NULL )
		return NULL;
	memset( shmc, 0, sizeof( SHMContext ) );
	shmc->shm_size = size;

	// create shared memory and semaphore for asynchronize access control.
	dev_key = ftok(path, projid);
	if ( dev_key < 0 ) {
		TRACE("SHMC_Create ftok return error: %s\n", strerror(errno) );
		free( shmc );
		return NULL;
	}
	shmc->sem_id = semget( dev_key, NUMSEM, 0644 );
	if( shmc->sem_id == -1 )
	{
		/* semaphore not exist, create it */
		shmc->sem_id = semget( dev_key, NUMSEM, 0644 | IPC_CREAT );
		if ( shmc->sem_id < 0 ) {
			TRACE("[SHMC_Create] semaphore create error: %s\n", strerror(errno) );
			free( shmc );
			return NULL;
		}
		/*
		 * we initial two semephores here.
		 * semaphore 0: New frame is ready. Wake-up all clients waiting on new frame.
		 * semaphone 1: spin-lock for shared video frame access control.
		 */
		for (i=0; i<NUMSEM; i++)
		{
			arg.val = i;
			semctl( shmc->sem_id, i, SETVAL, arg );	
		}
	}
	else
	{
		// semaphore exist. means it has been created. return failure
		TRACE("[SHMC_Create] semaphore exist. shared memory shall exist.\n");
		free( shmc );
		return NULL;
	}
	
	shmc->shm_id = shmget( dev_key, shmc->shm_size, 0644 );
	if ( shmc->shm_id == -1 )
	{
		// shared memory not exist, create it.
		if ( (shmc->shm_id=shmget( dev_key, shmc->shm_size, IPC_CREAT | 0644 )) != -1 )
		{
			shmc->shm_ptr = shmat( shmc->shm_id, 0, 0 );
			memset(	shmc->shm_ptr, 0, size );	
		}
		else
		{
			TRACE( "[SHMC_Create] shmget error: %s!\n", strerror(errno) );
			shmctl( shmc->sem_id, IPC_RMID, NULL);
			free( shmc );
			return NULL;
		}
	}
	else
	{
		// shared memory exist, return failure
		free( shmc );
		return NULL;
	}
	
	return shmc;	
}

PSHMContext SHMC_Attach( const char *path, int projid, int size )
{
	int		dev_key;
	
	PSHMContext shmc = malloc( sizeof( SHMContext ) );
	if ( shmc != NULL ) 
	{
		memset( shmc, 0, sizeof( SHMContext ) );
		dev_key = ftok( path, projid );
		shmc->sem_id = semget( dev_key, NUMSEM, 0644 );
    shmc->shm_id = shmget( dev_key, size, 0444 );
		if ( shmc->sem_id == -1 || shmc->shm_id == -1 ) {
			free (shmc);
			shmc = NULL;
		} else {
			/* now, attach the shared memory area */
			shmc->shm_ptr = shmat(shmc->shm_id, 0, 0);
		}
	}
	return shmc;
}

void SHMC_Destroy( PSHMContext pshmc )
{
	if( pshmc->shm_ptr ) shmdt( pshmc->shm_ptr );
	if( pshmc->shm_id != -1 )
	{
		if ( shmctl( pshmc->shm_id, IPC_RMID, NULL) < 0 )
			TRACE("\tremove shared memory error: %s\n",strerror(errno) );
	}
	if( pshmc->sem_id != -1 ) {
		if ( semctl( pshmc->sem_id, 0, IPC_RMID, NULL ) < 0 )
			TRACE("\tremove semophore error: %s\n", strerror(errno) );
	}	
	free( pshmc );
}

void SHMC_Detach( PSHMContext pshmc )
{
	if( pshmc->shm_ptr ) shmdt( pshmc->shm_ptr );
	free( pshmc );
}

int SHMC_Wait(PSHMContext pshmc)
{
	struct sembuf sb[1] = {
		{ SEM_ASYNC, 0, 0 },				// semaphore 0: wait for semaphore becomes 0
	};
	
	return semop( pshmc->sem_id, sb, 1 );
}

int SHMC_Ready(PSHMContext pshmc)
{
	struct sembuf sb[1] = {
		{ SEM_ASYNC, -1, 0 },		// semaphore 0: -1 decrease it, so all process wait in SHMC_Wait will be resumed.
	};
	
	// decrease semophore. all reader wait for it becomes zero will be waken
	semop( pshmc->sem_id, sb, 1 );

	// set the semaphore 0 to 1 again. So that clients can wait for next frame.
	sb[0].sem_op = 1;
	return semop( pshmc->sem_id, sb, 1 );
}

int SHMC_Lock(PSHMContext pshmc)
{
	struct sembuf sb[1] = {
		{ SEM_SPINLOCK, -1, 0 }		// semaphore for spin-lock mechanism
	};

	/* get the spin-lock */
	return semop( pshmc->sem_id, sb, 1 );
}

int SHMC_Unlock(PSHMContext pshmc)
{
	struct sembuf sb[1] = {
		{ SEM_SPINLOCK, 1, 0 }		// semaphore for spin-lock mechanism
	};

	/* get the spin-lock */
	return semop( pshmc->sem_id, sb, 1 );
}

int SHMC_UnlockAndReady(PSHMContext pshmc)
{
	struct sembuf sb[2] = {
		{ SEM_SPINLOCK, 1, 0 },		// semaphore for spin-lock mechanism
		{ SEM_ASYNC, -1, 0 },			// semaphore 0: -1 decrease it, so all process wait in SHMC_Wait will be resumed.
	};
	return semop( pshmc->sem_id, sb, 2 );
}

int SHMC_WaitAndLock(PSHMContext pshmc)
{
	struct sembuf sb[2] = {
		{ SEM_ASYNC, 0, 0 },				// semaphore 0: wait for semaphore becomes 0
		{ SEM_SPINLOCK, -1, 0 }	
	};
	return semop( pshmc->sem_id, sb, 2 );

}

