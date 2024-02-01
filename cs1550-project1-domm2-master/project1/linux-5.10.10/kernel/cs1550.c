#include <linux/syscalls.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/spinlock.h>
#include <linux/stddef.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/cs1550.h>

static DEFINE_RWLOCK(rwlock);
static LIST_HEAD(list);
static LIST_HEAD(waiting_tasks);
static unsigned long main_sem_id = 0;


/**
 * Creates a new semaphore. The long integer value is used to
 * initialize the semaphore's value.
 *
 * The initial `value` must be greater than or equal to zero.
 *
 * On success, returns the identifier of the created 
 * semaphore, which can be used with up() and down().
 *
 * On failure, returns -EINVAL or -ENOMEM, depending on the
 * failure condition.
 * EINVAL invalid argument
 * ENOMEM data space is not large enough
 */
SYSCALL_DEFINE1(cs1550_create, long, value)
{
	//value is a type long and is the max semaphone value u can have 

	/* initialize and allocate new sem */
	struct cs1550_sem *sem = kmalloc(sizeof(struct cs1550_sem), GFP_KERNEL);
	if(sem == NULL) return -EINVAL;
	if(value < 0) return -EINVAL;
	//assign sem id as global int
	sem->sem_id = main_sem_id; 
	sem->value = value;
	main_sem_id += 1;

	/* initialize sem lock. param is memory address of sem's lock */ 
	spin_lock_init(&sem->lock);

	/* initialize queue for pending tasks */
	INIT_LIST_HEAD(&sem->list);
	/* initialize waiting task list. initialize head and tail */
	INIT_LIST_HEAD(&sem->waiting_tasks); 

	/* initialize and allocate task node */
	//struct cs1550_task *task_node = kmalloc(sizeof(struct cs1550_task), GFP_KERNEL);
	/* initialize list. previous and next pointers */	
	//INIT_LIST_HEAD(&task_node->list); 

	/* lock writing rwlock */
	write_lock(&rwlock);
	/* add sem to sem_list */
	list_add(&sem->list, &list);
	write_unlock(&rwlock);

	return sem->sem_id;
}

/**
 * Performs the down() operation on an existing semaphore
 * using the semaphore identifier obtained from a previous call
 * to cs1550_create().
 *
 * This decrements the value of the semaphore, and *may cause* the
 * calling process to sleep (if the semaphore's value goes below 0)
 * until up() is called on the semaphore by another process.
 *
 * Returns 0 when successful, or -EINVAL or -ENOMEM if an error
 * occurred.
 */
SYSCALL_DEFINE1(cs1550_down, long, sem_id)
{
	//lock reading rwlock
	read_lock(&rwlock);
	/* search through the sem_list for the given sem_id */
	int b = 0;
	struct cs1550_sem *sem = NULL;
	list_for_each_entry(sem, &list, list) {
		if(sem->sem_id == sem_id)
		{
			b=b+1; break;
		}
	}//endloop
	if(b==0){ read_unlock(&rwlock); return -EINVAL; }
	/* lock */
	spin_lock(&sem->lock);
	/* decrement value */
	sem->value = sem->value - 1; 
	if(sem->value < 0)
	{
		/* allocated and add task to list of waiting processes */
		struct cs1550_task *task_node = kmalloc(sizeof(struct cs1550_task), GFP_ATOMIC);
		if(task_node==NULL){ read_unlock(&rwlock); spin_unlock(&sem->lock); return -ENOMEM; }
		task_node->task = current;
		INIT_LIST_HEAD(&task_node->list); 
		//add task to end of waiting list
		list_add_tail(&task_node->list, &sem->waiting_tasks);
		//append the pointer that points to it to the wiating task list using current var
		spin_unlock(&sem->lock);
		/* sleep(); */
		set_current_state(TASK_INTERRUPTIBLE);
		/* select diff process to run */
		schedule(); 
	}
	else spin_unlock(&sem->lock);
	read_unlock(&rwlock);
	return 0; 
}
	// ------------------------------------------------------------
	// struct cs1550_sem *sem = NULL;
	// list_for_each_entry(sem, &list, list) {
	// 	if(sem->sem_id == sem_id)
	// 	{
	// 		/* lock */
	// 		spin_lock(&sem->lock);
	// 		/* decrement value */
	// 		sem->value -= 1; 

	// 		if(sem->value < 0)
	// 		{
	// 			/* allocated and add task to list of waiting processes */
	// 			struct cs1550_task *task_node = kmalloc(sizeof(struct cs1550_task), GFP_ATOMIC);
	// 			task_node->task = current;
	// 			INIT_LIST_HEAD(&task_node->list); 

	// 			list_add_tail(&task_node->list, &sem->waiting_tasks);

	// 			/* sleep(); */
	// 			set_current_state(TASK_INTERRUPTIBLE);
	// 			spin_unlock(&sem->lock);
	// 			/* select diff process to run */
	// 			schedule(); 
	// 			read_unlock(&rwlock);
	// 			return 0;
	// 		} 
	// 		else { spin_unlock(&sem->lock); read_unlock(&rwlock); return 0; }
	// 	}
	// }
	// /* if sem doesnt exist return error */
	// read_unlock(&rwlock);
	// return -EINVAL; 

/**
 * Performs the up() operation on an existing semaphore
 * using the semaphore identifier obtained from a previous call
 * to cs1550_create().
 *
 * This increments the value of the semaphore, and *may cause* the
 * calling process to wake up a process waiting on the semaphore,
 * if such a process exists in the queue.
 *
 * Returns 0 when successful, or -EINVAL if the semaphore ID is
 * invalid.
 */
SYSCALL_DEFINE1(cs1550_up, long, sem_id)
{
	//lock reading rwlock
	read_lock(&rwlock);

	/* search through the sem_list for the given sem_id */
	int c = 0;
	struct cs1550_sem *sem = NULL;
	struct cs1550_task *entry = NULL;
	list_for_each_entry(sem, &list, list) {
		if(sem->sem_id == sem_id)
		{
			c=c+1; break;
		}
	}//endloop
	if(c==0){ read_unlock(&rwlock); 
	return -EINVAL; }

	spin_lock(&sem->lock);
	sem->value += 1; 
	if(sem->value <= 0) {
		//if (list_empty(&sem->waiting_tasks)){
			//remove a process P from list pl 
			entry = list_first_entry(&sem->waiting_tasks, struct cs1550_task, list);
			list_del(&entry->list);
			
			/* Wakeup (P) */ 
			wake_up_process(entry->task); 
			kfree(entry);
		//}

	}
	spin_unlock(&sem->lock); 
	read_unlock(&rwlock);
	return 0;
}
	//----------------------------------------------------
	/* traverse global sem_list for the given sem_id */
	// struct cs1550_sem *sem = NULL;
	// list_for_each_entry(sem, &list, list) {
	// 	if(sem->sem_id == sem_id)
	// 	{
	// 		//lock(lk);
	// 		spin_lock(&sem->lock);
	// 		sem->value += 1; 
	// 		if(sem->value <= 0) 
	// 		{ 
	// 			//remove a process P from list pl 
	// 			list_del(&list_first_entry(&sem->waiting_tasks, struct cs1550_task, list)->list);
	// 			/* Wakeup (P) */ 
	// 			wake_up_process(list_first_entry(&sem->waiting_tasks, struct cs1550_task, list)->task);
	// 			spin_unlock(&sem->lock); read_unlock(&rwlock);
	// 			return 0;
	// 		}
	// 		else { break; }
	// 	}
	// }
	// spin_unlock(&sem->lock); read_unlock(&rwlock);
	// return -EINVAL;


/**
 * Removes an already-created semaphore from the system-wide
 * semaphore list using the identifier obtained from a previous
 * call to cs1550_create().
 *
 * Returns 0 when successful or -EINVAL if the semaphore ID is
 * invalid or the semaphore's process queue is not empty.
 */
SYSCALL_DEFINE1(cs1550_close, long, sem_id)
{
	/* lock writing rwlock */
	write_lock(&rwlock);
	int c = 0;
	struct cs1550_sem *sem = NULL;
	list_for_each_entry(sem, &list, list) {
		if(sem->sem_id == sem_id)
		{
			c++; break;
		}
	}//endloop
	if(c==0){ write_unlock(&rwlock); return -EINVAL; }

	spin_lock(&sem->lock);
	if (list_empty(&sem->waiting_tasks))
	{
		/* remove the sem from sem_list */
		list_del(&sem->list);
		//main_sem_id -= 1;
		spin_unlock(&sem->lock);
		/* release allocated space for old sem */
		kfree(sem);
	}
	else {spin_unlock(&sem->lock); return -EINVAL;}
	write_unlock(&rwlock);
	return 0;
}
	//----------------------------------------------------
	/* traverse global sem_list for the given sem_id */
// 	struct cs1550_sem *sem = NULL;
// 	list_for_each_entry(sem, &list, list) {
// 		if(sem->sem_id == sem_id){
// 			spin_lock(&sem->lock);
// 			/* if waiting_list is empty */
// 			if (list_empty(&sem->waiting_tasks))
// 			{
// 				/* remove the sem from sem_list */
// 				list_del(&sem->list);
// 				spin_unlock(&sem->lock);
// 				/* release allocated space for old sem */
// 				kfree(sem);
// 				return 0;
// 			}
// 			else
// 			{
// 				/* unlock(lck) & unlock rwlock & return error */
// 				spin_unlock(&sem->lock);
// 				write_unlock(&rwlock);
// 				return -EINVAL; 
// 			}
// 		}
// 	}
// 	/* if sem_id is invalid return error */
// 	write_unlock(&rwlock);
// 	return -EINVAL;
// }