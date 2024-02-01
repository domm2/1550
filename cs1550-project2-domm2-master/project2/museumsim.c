#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#include "museumsim.h"

//
// In all of the definitions below, some code has been provided as an example
// to get you started, but you do not have to use it. You may change anything
// in this file except the function signatures.
//


struct shared_data {
	// Add any relevant synchronization constructs and shared state here.
	// For example:

	//tickets is being used by both threads so it needs a lock to protect it 
	int tickets;
	pthread_mutex_t mutexTicket; //lock
	pthread_cond_t condTicket; 

	int guidwait;
	int guides_in; //counter
	pthread_cond_t condGuides;
	
	int adminnum;
	int enternum;
	int viswait;
	int visitors_in; //counter
	pthread_cond_t condVisitors;
};

static struct shared_data shared;


/**
 * Set up the shared variables for your implementation.
 * 
 * `museum_init` will be called before any threads of the simulation
 * are spawned.
 */
void museum_init(int num_guides, int num_visitors)
{
	shared.tickets = MIN(VISITORS_PER_GUIDE * num_guides, num_visitors);
	pthread_mutex_init(&shared.mutexTicket, NULL);
	pthread_cond_init(&shared.condTicket, NULL);	

	shared.guidwait = 0;
	shared.guides_in = 0; 
	pthread_cond_init(&shared.condGuides, NULL);
	
	shared.adminnum = 0;
	shared.enternum = 0;
	shared.viswait = 0;
	shared.visitors_in = 0; 
	pthread_cond_init(&shared.condVisitors, NULL);
}


/**
 * Tear down the shared variables for your implementation.
 * 
 * `museum_destroy` will be called after all threads of the simulation
 * are done executing.
 */
void museum_destroy()
{
	pthread_mutex_destroy(&shared.mutexTicket);
	pthread_cond_destroy(&shared.condTicket);
}


/**
 * Implements the visitor arrival, touring, and leaving sequence.
 */
void visitor(int id)
{
	pthread_mutex_lock(&shared.mutexTicket);
	visitor_arrives(id); //visitor V arrives at time T.

	//if no tickets visitor leaves 
	if(shared.tickets <= 0){
		printf("vis in and out \n");
		visitor_leaves(id); //Visitor V leaves the museum at time T.
		pthread_mutex_unlock(&shared.mutexTicket);
	}
	else{ 
		//set the order # 
		//printf("enternum: %d\n", shared.enternum);
		//if shared.enternum >= guides allowed to be admitted reset it
		// if(shared.enternum >= VISITORS_PER_GUIDE * shared.guides_in){
		// 	shared.enternum = 0;
		// }
		shared.enternum += 1; 
		printf("enternum: %d\n", shared.enternum);
		int curenternum = shared.enternum;
		printf("cur enternum: %d\n", curenternum);

		//takes a ticket. dec ticket. inc waiting visitors 
		printf("More Tickets: %d\n", shared.tickets);
		shared.tickets -= 1; 
		printf("Took a ticket. Now: %d\n", shared.tickets);
		shared.viswait += 1;

		//wakes up guide when a new guest comes 
		pthread_cond_broadcast(&shared.condGuides);

		//thread blocks until guide admits it
		while(curenternum > shared.adminnum){ 
			printf("Not admitted yet \n"); 
			pthread_cond_wait(&shared.condVisitors, &shared.mutexTicket); 
		}
		
		//thread admitted so it can start touring whenever
		printf("about to tour. visitors in: %d\n", shared.visitors_in);
		pthread_mutex_unlock(&shared.mutexTicket);
		visitor_tours(id); //Visitor V tours the museum at time T.
		pthread_mutex_lock(&shared.mutexTicket);

		//thread leaves museum. dec visitors in. broadcast guides to
		//check if museum is empty so guides can leave too
		printf("about to leave... \n");
		shared.visitors_in -= 1;
		visitor_leaves(id); //Visitor V leaves the museum at time T.
		printf("Visistor left... %d\n", shared.visitors_in);
		pthread_cond_broadcast(&shared.condGuides);
		pthread_mutex_unlock(&shared.mutexTicket);
	}
}

/**
 * Implements the guide arrival, entering, admitting, and leaving sequence.
 */
void guide(int id)
{
	pthread_mutex_lock(&shared.mutexTicket);
	guide_arrives(id); //Guide G arrives at time T.

	//If there are no visitors left to serve, 
	//the tour guides should enter and then immediately leave
	//no more vis waiting & no more tickets means no more visitors are coming
	if(shared.viswait <= 0 && shared.tickets <= 0){ 
		printf("No tickets left: %d\n", shared.tickets); 
		guide_enters(id); guide_leaves(id); 
	}

	// while guides == 2 || guides waiting > 0 ) 
	while( (shared.guides_in == GUIDES_ALLOWED_INSIDE) || (shared.guidwait > 0)){ 
		printf("max guides inside or need old guide to leave. Waiting... \n"); 
		pthread_cond_wait(&shared.condGuides, &shared.mutexTicket); 
	}

	//guide enters. inc guides in. 
	guide_enters(id); 
	shared.guides_in += 1;
	printf("guide entered. guides in now: %d\n", shared.guides_in);

	//waits for and admits visitors until it has either 
	//served a maximum of VISITORS_PER_GUIDE visitors, 
	//or there are no visitors waiting and no tickets remaining.

	int i =0; //amount of visitors this specific guide admits
	//loops 10 times and there must be either visitors waiting or tickets left 
	while(i < VISITORS_PER_GUIDE && (shared.viswait > 0 || shared.tickets > 0)){

		//while there are no visitors waiting but there are tickets left 
		//when waiting visitor increments, broadcast a guide 
		while(shared.viswait <= 0 && shared.tickets > 0){
			printf("no visitors waiting but more tickets. Waiting... \n");
			pthread_cond_wait(&shared.condGuides, &shared.mutexTicket);
		}

		//if theres visitors waiting, admit them and signal
		//inc visitors in dec visitors waiting
		//inc i 
		if(shared.viswait > 0){
			printf("about to admit visitor. \n");
			shared.visitors_in += 1; shared.viswait -= 1;
			guide_admits(id); //Guide G admits a visitor at time T.
			i++; shared.adminnum += 1;
			printf("admitted visitor: %d", shared.visitors_in);
			printf("  t left: %d\n", shared.tickets);
			pthread_cond_signal(&shared.condVisitors);
		}
	}
	
	//guide is done and waiting on vis to leave so it can leave
	//inc waiting guides 
	shared.guidwait += 1;

	//wait until visitors in all leave and both guides are done
	//all are done when amount of guides in == guides waiting
	//each time a vis leaves it broadcasts guides
	while(shared.visitors_in > 0 || shared.guidwait < shared.guides_in){
		pthread_cond_wait(&shared.condGuides, &shared.mutexTicket);
		printf("visitors in: %d\n", shared.visitors_in);
		printf("visitors waiting: %d\n", shared.viswait);
		printf("tickets: %d\n", shared.tickets);
	}

	//After the tour guide has admitted all the visitors it can, 
	//it must leave as soon as possible given the following constraints: 
	//it must wait for all visitors inside to leave, 
	//and for any other tour guide inside to finish before it can leave.
	printf("guide about to leave. guides in: %d\n", shared.guides_in);
	guide_leaves(id); //Guide G leaves the museum at time T.
	shared.guides_in -= 1; shared.guidwait -= 1;
	printf("guide left. guides in now: %d\n", shared.guides_in);

	//for each one leaving signal one to wake
	pthread_cond_broadcast(&shared.condGuides); 
			
	printf("endddd of guidee \n");
	pthread_mutex_unlock(&shared.mutexTicket);
}
