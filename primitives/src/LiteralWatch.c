/*
 * LiteralWatch.c
 *
 *  Created on: May 19, 2015
 *      Author: salma
 */

#include "LiteralWatch.h"


#define MALLOC_GROWTH_RATE 	   	2

/** Global variable for this file **/
BOOLEAN literal_watched_initialized = 0;

static void add_watching_clause(Clause* clause, Lit* lit){

	//get clause index.
	unsigned long index = clause->cindex;

	if(lit->num_watched_clauses >= lit->max_size_list_watched_clauses){
			// needs to realloc the size
			lit->max_size_list_watched_clauses =(lit->num_watched_clauses * MALLOC_GROWTH_RATE);
			lit->list_of_watched_clauses = (unsigned long*) realloc( lit->list_of_watched_clauses, sizeof(unsigned long) * lit->max_size_list_watched_clauses);
	}

	lit->list_of_watched_clauses[lit->num_watched_clauses] = index;

	lit->num_watched_clauses ++;
}

static void intitialize_watching_clauses(SatState* sat_state){

	for(unsigned long i =0; i< sat_state->num_clauses_in_delta; i++){
		Clause watching_clause = sat_state->delta[i];
		Lit* watched_literal1 = watching_clause.L1;
		Lit* watched_literal2 = watching_clause.L2;
		add_watching_clause(&watching_clause, watched_literal1);
		add_watching_clause(&watching_clause, watched_literal2);
	}
	// Set the initialization flag;
	literal_watched_initialized = 1;
}

static Lit* get_resolved_lit(Lit* decided_literal, SatState* sat_state){
	Var* corresponding_var = index2varp(abs(decided_literal->sindex), sat_state);
	Lit* resolved_literal = NULL;
	if(decided_literal->sindex <0){
		resolved_literal = corresponding_var->posLit;
		resolved_literal->LitValue = 0;
		resolved_literal->LitState = 1;
	}
	else if(decided_literal->sindex >0){
		resolved_literal = corresponding_var->negLit;
		resolved_literal->LitValue = 1;
		resolved_literal->LitValue = 1;
	}

	return resolved_literal;
}

static void add_pending_literal(Lit** pending_list, Lit* lit, unsigned long* capacity, unsigned long* num_elements){

	unsigned long cap = *capacity;
	unsigned long num = *num_elements;

	if(num >= cap){
		// needs to realloc the size
		cap =num * MALLOC_GROWTH_RATE;
		pending_list = (Lit**) realloc(pending_list , sizeof(Lit*) * cap);

	}

		pending_list[num++] = lit;

}



/******************************************************************************
	Two literal watch algorithm for unit resolution
The algorithm taken from the Class Notes for CS264A, UCLA

******************************************************************************/

BOOLEAN two_literal_watch(SatState* sat_state){

	// initialize the list of watched clauses
	if(!literal_watched_initialized)
		intitialize_watching_clauses(sat_state);


	Clause* conflict_clause = NULL;

	// I know here that the decision array is filled with at least one element
	// get the last decision in the decision array and try to propagate from there
	// The decide literal can be positive or negative.
	// if u decide a negative literal then its positive version is resolved.
	// then I need the other literal version!!

	//Hence from the literal absolute index I can get the variable and then access the other literal which will be the resolved literal
	// get the tail of the decisions list
	//TODO: Due to recursion of pending list we may need to consider more than one decided literal
	Lit* decided_literal = sat_state->decisions[sat_state->num_literals_in_decision -1];

	Lit* resolved_literal = get_resolved_lit(decided_literal, sat_state);


	// Create pending literal list
	Lit** pending_list = (Lit**)malloc(sizeof(Lit*));
	unsigned long max_size_pending_list = 1;
	unsigned long num_pending_lit = 0;


	//If no watching clauses on the resolved literal then do nothing and record the decision
	if(resolved_literal->num_watched_clauses == 0){
		// The implications list already has malloc with number of variables.
		sat_state->implications[sat_state->num_literals_in_implications++] = decided_literal;
		//wait for a new decision
		return 1;
	}
	else{
		//Get the watched clauses for the resolved literal
		for(unsigned long i = 0; i < resolved_literal->num_watched_clauses; i++){
				Clause* wclause = index2clausep( resolved_literal->list_of_watched_clauses[i], sat_state);

				Lit* free_lit = NULL;
				// if unit (only one free)
				unsigned long num_free_literals = 0;
				for(unsigned long j = 0; j < wclause->num_literals_in_clause; j++){
					if(is_free_literal(wclause->literals[j])){
						num_free_literals++;
						free_lit = wclause->literals[j];
					}
				}
				if(num_free_literals == 1){
					//I have a unit clause take an implication
					// the last free literal is the only free literal in this case
					add_pending_literal(pending_list,  free_lit , &max_size_pending_list, &num_pending_lit);
					continue; // go to the next clause
				}
				if (num_free_literals == 0){
					//contradiction
					return 0;
				}



				// find another literal to watch that is free
				for(unsigned long j = 0; j < wclause->num_literals_in_clause; j++){
					if(is_free_literal(wclause->literals[j]) && wclause->literals[j] != wclause->L1 && wclause->literals[j] != wclause->L2){
						Lit* new_watched_lit = wclause->literals[j];
						//add the watching clause to the free literal
						add_watching_clause(wclause, new_watched_lit);

						// remove the resolved literal from the watch and update the clause watch list
						if(resolved_literal == wclause->L1)
							wclause->L1 = new_watched_lit;
						else if (resolved_literal == wclause->L2)
							wclause->L2 = new_watched_lit;
						break;
					}
				}
		} //end of for

		if(num_pending_lit > 0){
		// Pass over the pending list and add the literals to the decision while maintaining the level
			for(unsigned long i =0; i < num_pending_lit; i++){
				// pending literal was an already watched clause so the associated list of watching clauses is already handled
				// fix values of pending literal before putting in decision
				Lit* pending_lit = pending_list[i];
				if(pending_lit->sindex <0){
					pending_lit->LitValue = 0;
					pending_lit->LitState = 1;
				}
				else if(pending_lit->sindex >0){
					pending_lit->LitValue = 1;
					pending_lit->LitValue = 1;
				}
				pending_lit->decision_level = sat_state->current_decision_level;

				sat_state->decisions[sat_state->num_literals_in_decision++] = pending_lit;
			}

			//TODO: two_literal_watch(sat_state);
		}

	}
	return 1 ;
}


