#ifndef H_MYPTHREAD
#define H_MYPTHREAD

// Types

// typedef before struct allows consistancy preventing incompatable pointer type warnings
//typedef struct _mypthread_t mypthread_t;

typedef enum { UNUSED, RUNNING, READY, EXITED, WAITING } threadStatus;

typedef struct _mypthread_t{
	// Define any fields you might need inside here.

	ucontext_t context;
	int index;		//index of this thread in the array
	int beingJoinedOnBy;	//index of the thread that is waiting on this thread -> only one thread can wait on a single thread, additional joins = error
	threadStatus status;	//current status of the thread
	void *retval;
	void** retadd;
	
}mypthread_t;

// Using a queue implementation to determine which thread to run next. 
//extern mypthread_t *head;
//extern mypthread_t *tail;

typedef struct {
	// Define any fields you might need inside here.
	// Unused because the creator does not have the option of creating preemptive threads -> cooperative only.
} mypthread_attr_t;

// Functions
int mypthread_create(mypthread_t *thread, const mypthread_attr_t *attr,
			void *(*start_routine) (void *), void *arg);

void mypthread_exit(void *retval);

int mypthread_yield(void);

int mypthread_join(mypthread_t thread, void **retval);


/* Don't touch anything after this line.
 *
 * This is included just to make the mtsort.c program compatible
 * with both your ULT implementation as well as the system pthreads
 * implementation. The key idea is that mutexes are essentially
 * useless in a cooperative implementation, but are necessary in
 * a preemptive implementation.
 */

typedef int mypthread_mutex_t;
typedef int mypthread_mutexattr_t;

static inline int mypthread_mutex_init(mypthread_mutex_t *mutex,
			const mypthread_mutexattr_t *attr) { return 0; }

static inline int mypthread_mutex_destroy(mypthread_mutex_t *mutex) { return 0; }

static inline int mypthread_mutex_lock(mypthread_mutex_t *mutex) { return 0; }

static inline int mypthread_mutex_trylock(mypthread_mutex_t *mutex) { return 0; }

static inline int mypthread_mutex_unlock(mypthread_mutex_t *mutex) { return 0; }

#endif
