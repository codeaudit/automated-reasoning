#include "sat_api.h"
#include <fcntl.h>
#include <assert.h>
#include "ParseDIMACS.h"
#include "LiteralWatch.h"
#include "VSIDS.h"
#include "ConflictAlgorithms.h"

#define MALLOC_GROWTH_RATE 	   	2

/* GLOBALS */
BOOLEAN FLAG_CASE1_UNIT_RESOLUTION = 0;
BOOLEAN FLAG_CASE2_UNIT_RESOLUTION = 0;
BOOLEAN FLAG_CASE3_UNIT_RESOLUTION = 0;  //TODO: set this to 1 in the final version




/******************************************************************************
 * We explain here the functions you need to implement
 *
 * Rules:
 * --You cannot change any parts of the function signatures
 * --You can/should define auxiliary functions to help implementation
 * --You can implement the functions in different files if you wish
 * --That is, you do not need to put everything in a single file
 * --You should carefully read the descriptions and must follow each requirement
 ******************************************************************************/

/******************************************************************************
 * Variables
 ******************************************************************************/

//returns a variable structure for the corresponding index
// index starts from 1 to n
Var* sat_index2var(c2dSize index, const SatState* sat_state) {
	return &sat_state->variables[index-1];
}

//returns the index of a variable
c2dSize sat_var_index(const Var* var) {
	return var->index;
}

//returns the variable of a literal
Var* sat_literal_var(const Lit* lit) {
	return lit->variable;
}

//returns 1 if the variable is instantiated, 0 otherwise
//a variable is instantiated either by decision or implication (by unit resolution)
BOOLEAN sat_instantiated_var(const Var* var) {
	// if the positive literal and the negative literal of the variable are set then the variable is instantiated
	if(var->negLit->LitState == 1 && var->posLit->LitState == 1)
		return 1;
	else
		return 0;
}

//returns 1 if all the clauses mentioning the variable are subsumed, 0 otherwise
BOOLEAN sat_irrelevant_var(const Var* var) {
	c2dSize i;
	for(i =0; i < var->num_of_clauses_of_variables; i++){
		if(sat_subsumed_clause(var->list_clause_of_variables[i] ))
			continue;
		else
			break;
	}
	if(i == var->num_of_clauses_of_variables)
		return 1;
	else
		return 0;
}

//returns the number of variables in the cnf of sat state
c2dSize sat_var_count(const SatState* sat_state) {
	return sat_state->num_variables_in_cnf;
}

//returns the number of clauses mentioning a variable
//a variable is mentioned by a clause if one of its literals appears in the clause
c2dSize sat_var_occurences(const Var* var) {
	return var->num_of_clauses_of_variables;
}

//returns the index^th clause that mentions a variable
//index starts from 0, and is less than the number of clauses mentioning the variable
//this cannot be called on a variable that is not mentioned by any clause
Clause* sat_clause_of_var(c2dSize index, const Var* var) {
	assert(index < var->num_of_clauses_of_variables);

	if(index < var->num_of_clauses_of_variables){
		return var->list_clause_of_variables[index];
	}
	else{
		return NULL;
	}

}

/******************************************************************************
 * Literals 
 ******************************************************************************/

BOOLEAN sat_is_asserted_literal(Lit* lit){
	if(lit->sindex<0){
		switch(lit->LitValue){
		case 0:
			assert(lit->LitState == 1); // asserted --> ie negative literal and has value false then it will be evaluated to true
			return 1;
			break;
		case 1:
			assert(lit->LitState == 1);
			return 0;
			break;
		default:
			assert(lit->LitState == 0);
			return 0;
			break;
		}
	}
	else if(lit->sindex > 0){
		switch(lit->LitValue){
		case 1:
			assert(lit->LitState == 1);
			return 1;
			break;
		case 0:
			assert(lit->LitState == 1);
			return 0;
			break;
		default:
			assert(lit->LitState == 0);
			return 0;
			break;
		}
	}

	return 0; // this should never happen

}

BOOLEAN sat_is_resolved_literal(Lit* lit){
	if(lit->sindex<0){
		switch(lit->LitValue){
		case 1:
			assert(lit->LitState == 1); // resolved --> ie negative literal and has value true then it will be evaluated to false
			return 1;
			break;
		case 0:
			assert(lit->LitState == 1);
			return 0;
			break;
		default:
			assert(lit->LitState == 0);
			return 0;
			break;
		}
	}
	else if(lit->sindex > 0){
		switch(lit->LitValue){
		case 0:
			assert(lit->LitState == 1);
			return 1;
			break;
		case 1:
			assert(lit->LitState == 1);
			return 0;
			break;
		default:
			assert(lit->LitState == 0);
			return 0;
			break;
		}
	}

	return 0; // this should never happen
}

//returns a literal structure for the corresponding index
Lit* sat_index2literal(c2dLiteral index, const SatState* sat_state) {
	//TODO: What is this!!!!
	//sat_state can have positive and negative literal!?
	return 0; //dummy value
}

//returns the index of a literal
c2dLiteral sat_literal_index(const Lit* lit) {
	return lit->sindex;
}

//returns the positive literal of a variable
Lit* sat_pos_literal(const Var* var) {
	return var->posLit;
}

//returns the negative literal of a variable
Lit* sat_neg_literal(const Var* var) {
	return var->negLit;
}

//returns 1 if the literal is implied, 0 otherwise
//a literal is implied by deciding its variable, or by inference using unit resolution
BOOLEAN sat_implied_literal(const Lit* lit) {
	return lit->LitState;
}

//sets the literal to true, and then runs unit resolution
//returns a learned clause if unit resolution detected a contradiction, NULL otherwise
//
//if the current decision level is L in the beginning of the call, it should be updated 
//to L+1 so that the decision level of lit and all other literals implied by unit resolution is L+1
Clause* sat_decide_literal(Lit* lit, SatState* sat_state) {

	assert(lit!=NULL);
	//update the literal parameters
	//Set lit values
	if(lit->sindex < 0)
		lit->LitValue = 0;
	else if (lit->sindex > 0)
		lit->LitValue = 1;

	//TODO:  flip the opposite literal -->handled in two literal watch

	lit->LitState = 1;

	// Update the decision level
	sat_state->current_decision_level ++;
	lit->decision_level = sat_state->current_decision_level;

	// here update the decision array of the sat_state
	sat_state->decisions[sat_state->num_literals_in_decision++] = lit;

	FLAG_CASE1_UNIT_RESOLUTION = 1;

	sat_unit_resolution(sat_state);

	return sat_state->alpha;
}

//undoes the last literal decision and the corresponding implications obtained by unit resolution
//
//if the current decision level is L in the beginning of the call, it should be updated 
//to L-1 before the call ends
void sat_undo_decide_literal(SatState* sat_state) {
	sat_undo_unit_resolution(sat_state);
}

/******************************************************************************
 * Clauses 
 ******************************************************************************/

//returns a clause structure for the corresponding index
Clause* sat_index2clause(c2dSize index, const SatState* sat_state) {
	assert(index < sat_state->num_clauses_in_delta);
	return &(sat_state->delta[index]);
}

//returns the index of a clause
c2dSize sat_clause_index(const Clause* clause) {
	return clause->cindex;
}

//returns the literals of a clause
Lit** sat_clause_literals(const Clause* clause) {
	return clause->literals;
}

//returns the number of literals in a clause
c2dSize sat_clause_size(const Clause* clause) {
	return clause->num_literals_in_clause;
}

//returns 1 if the clause is subsumed, 0 otherwise
BOOLEAN sat_subsumed_clause(const Clause* clause) {
	return clause->is_subsumed; //TODO: don't forget to update the state of the clause after unit resolution
}

//returns the number of clauses in the cnf of sat state
c2dSize sat_clause_count(const SatState* sat_state) {
	return sat_state->num_clauses_in_cnf;
}

//returns the number of learned clauses in a sat state (0 when the sat state is constructed)
c2dSize sat_learned_clause_count(const SatState* sat_state) {
	return sat_state->num_clauses_in_gamma;
}

//adds clause to the set of learned clauses, and runs unit resolution
//returns a learned clause if unit resolution finds a contradiction, NULL otherwise
//
//this function is called on a clause returned by sat_decide_literal() or sat_assert_clause()
//moreover, it should be called only if sat_at_assertion_level() succeeds
Clause* sat_assert_clause(Clause* clause, SatState* sat_state) {

	//TODO: Check the sanity if this
	//TODO check the requirements in the comments
	FLAG_CASE2_UNIT_RESOLUTION = 1;

	//learn clause
	CDCL_non_chronological_backtracking_first_UIP(sat_state); // will assign alpha

	// update the gamma list with the new alpha (just for performance analysis)
	add_clause_to_gamma(sat_state);

	update_vsids_scores(sat_state);

	sat_unit_resolution(sat_state);

	return sat_state->alpha;
}


//added API: Update the state of the clause if a literal is decided or implied
void sat_update_clauses_state(Lit* lit){
	if(sat_is_asserted_literal(lit)){
		Var* corresponding_var = lit->variable;
		for(unsigned long i =0; i<corresponding_var->num_of_clauses_of_variables; i++){
			corresponding_var->list_clause_of_variables[i]->is_subsumed = 1;
		}
	}
}
//added API: Undo state of clause of undecided literal which was asserted called at undo unit resolution took place
void sat_undo_clauses_state(Lit* lit){
	if(sat_is_asserted_literal(lit)){
		Var* corresponding_var = lit->variable;
		for(unsigned long i =0; i<corresponding_var->num_of_clauses_of_variables; i++){
			if(corresponding_var->list_clause_of_variables[i]->is_subsumed == 1){
				corresponding_var->list_clause_of_variables[i]->is_subsumed = 0; //clear the state of the clause
			}
		}
	}

}


/******************************************************************************
 * A SatState should keep track of pretty much everything you will need to
 * condition/uncondition variables, perform unit resolution, and do clause learning
 *
 * Given an input cnf file you should construct a SatState
 *
 * This construction will depend on how you define a SatState
 * Still, you should at least do the following:
 * --read a cnf (in DIMACS format, possible with weights) from the file
 * --initialize variables (n of them)
 * --initialize literals  (2n of them)
 * --initialize clauses   (m of them)
 *
 * Once a SatState is constructed, all of the functions that work on a SatState
 * should be ready to use
 *
 * You should also write a function that frees the memory allocated by a
 * SatState (sat_state_free)
 ******************************************************************************/

//constructs a SatState from an input cnf file
SatState* sat_state_new(const char* file_name) {
	//initialization
  SatState* sat_state = (SatState *) malloc (sizeof (SatState));

  //sat_state->decisions, sat_state->variables, sat_state->implications, sat_state->delta
  // and their corresponding parameters are set in parseDIMACS

  sat_state->gamma = (Clause *) malloc(sizeof(Clause));
  sat_state->alpha = (Clause *) malloc(sizeof(Clause));
  sat_state->conflict_clause = (Clause*) malloc(sizeof(Clause));

  sat_state->current_decision_level = 1; // this is by description
  sat_state->num_clauses_in_delta = 0;
  sat_state->num_clauses_in_gamma = 0;
  sat_state->num_literals_in_decision = 0;
  sat_state->num_literals_in_implications = 0;
  sat_state->num_variables_in_cnf = 0;


  FILE* cnf_file = fopen(file_name, "r");
  
  if (cnf_file == 0){
	  perror("Cannot open the CNF file");
	  exit(-1);
  }
  else{
	  // call the parser
	  parseDIMACS(cnf_file, sat_state);
  }

  fclose(cnf_file);


  initialize_vsids_scores(sat_state);

  return sat_state;
}

//frees the SatState
void sat_state_free(SatState* sat_state) {
//TODO: Need to check this again to deep clean everything
	//deep cleaning of variables and implications
	#ifdef DEBUG
		printf("number of variables in sat_state %ld\n",sat_state->num_variables_in_cnf );
	#endif
		for(unsigned long i = 0; i<sat_state->num_variables_in_cnf; i++){
			free(sat_state->variables[i].negLit->list_of_watched_clauses);
			free(sat_state->variables[i].posLit->list_of_watched_clauses);
		//	free(sat_state->implications[i]->list_of_watched_clauses); // this causes segmentation fault!
		}
		free(sat_state->variables);
		free(sat_state->implications); // this does not!


	//deep cleaning of decisions
		for(unsigned long i = 0; i<sat_state->num_literals_in_decision; i++){
			free(sat_state->decisions[i]->list_of_watched_clauses);
		}
		free(sat_state->decisions);


	//deep cleaning of delta
		for(unsigned long i =0; i<sat_state->num_clauses_in_delta; i++){
			free(sat_state->delta[i].L1->list_of_watched_clauses);
			free(sat_state->delta[i].L2->list_of_watched_clauses);
			free(sat_state->delta[i].L1);
			free(sat_state->delta[i].L2);
			for(unsigned long j=0; j<sat_state->delta[i].num_literals_in_clause; j++){
				free(sat_state->delta[i].literals[j]->list_of_watched_clauses);
				free(sat_state->delta[i].literals[j]);
			}
		}
		free(sat_state->delta);


	//deep cleaning of gamma
		for(unsigned long i =0; i<sat_state->num_clauses_in_gamma; i++){
			free(sat_state->gamma[i].L1->list_of_watched_clauses);
			free(sat_state->gamma[i].L2->list_of_watched_clauses);
			free(sat_state->gamma[i].L1);
			free(sat_state->delta[i].L2);
			for(unsigned long j=0; j<sat_state->gamma[i].num_literals_in_clause; j++){
				free(sat_state->gamma[i].literals[j]->list_of_watched_clauses);
				free(sat_state->gamma[i].literals[j]);
			}
		}
		free(sat_state->gamma);

		//TODO: needs deep cleaning of conflict clause
		//free(sat_state->conflict_clause);

		free(sat_state);
}

/******************************************************************************
 * Given a SatState, which should contain data related to the current setting
 * (i.e., decided literals, subsumed clauses, decision level, etc.), this function 
 * should perform unit resolution at the current decision level 
 *
 * It returns 1 if succeeds, 0 otherwise (after constructing an asserting
 * clause)
 *
 * There are three possible places where you should perform unit resolution:
 * (1) after deciding on a new literal (i.e., in sat_decide_literal())
 * (2) after adding an asserting clause (i.e., in sat_assert_clause(...)) 
 * (3) neither the above, which would imply literals appearing in unit clauses
 *
 * (3) would typically happen only once and before the other two cases
 * It may be useful to distinguish between the above three cases
 * 
 * Note if the current decision level is L, then the literals implied by unit
 * resolution must have decision level L
 *
 * This implies that there must be a start level S, which will be the level
 * where the decision sequence would be empty
 *
 * We require you to choose S as 1, then literals implied by (3) would have 1 as
 * their decision level (this level will also be the assertion level of unit
 * clauses)
 *
 * Yet, the first decided literal must have 2 as its decision level
 ******************************************************************************/
static BOOLEAN unit_resolution_case_1(SatState* sat_state){
	// This is called after decide literal
		FLAG_CASE1_UNIT_RESOLUTION = 0;
		// run the two literal watch based on the new decision
		return two_literal_watch(sat_state);
}

static BOOLEAN unit_resolution_case_2(SatState* sat_state){
	// this is called after adding an asserting clause
	//reset the flag
	FLAG_CASE2_UNIT_RESOLUTION = 0;
	//TODO: // something has to be done with the new conflict clause
	return  two_literal_watch(sat_state);
}

static BOOLEAN unit_resolution_case_3(SatState* sat_state){
	//Reset the flag in this case because I don't want to execute this except once
	FLAG_CASE3_UNIT_RESOLUTION = 0;

	// The first time it is called which is case 3 is before any decision or adding assertion clause.
	// This means search in the clauses for unit literals, if found, add it in a list and decide literal on it and run unit resolution.
	for(unsigned long i=0; i<sat_state->num_clauses_in_delta; i++){
		Clause* cur_clause = &(sat_state->delta[i]);
		if (cur_clause->num_literals_in_clause == 1){
			// a unit clause
			Lit* unit_lit = cur_clause->literals[0];
			//update the literal parameters (decide it)
			//Set lit values
			if(unit_lit->sindex < 0)
				unit_lit->LitValue = 0;
			else if (unit_lit->sindex > 0)
				unit_lit->LitValue = 1;

			unit_lit->decision_level = 1; // because it is unit
			unit_lit->LitState = 1;

			//add it in the decision list without incrementing the decision level
			sat_state->decisions[sat_state->num_literals_in_decision++] = unit_lit;
			sat_state->current_decision_level = 1;
		}
	}
	// now the decision list is updated with all unit literals
	// run the two literal watch algorithm
	if(sat_state->num_literals_in_decision == 0){
		// there was no unit clause
		return 1; //just return and takes the next step
	}
	else{
		return two_literal_watch(sat_state);
	}

}
//applies unit resolution to the cnf of sat state
//returns 1 if unit resolution succeeds, 0 if it finds a contradiction
BOOLEAN sat_unit_resolution(SatState* sat_state) {
	//TODO: How to distinguish between the three cases
	if(FLAG_CASE3_UNIT_RESOLUTION == 1){
		return unit_resolution_case_3(sat_state);
	}
	else if(FLAG_CASE2_UNIT_RESOLUTION == 1){
		return unit_resolution_case_2(sat_state);
	}
	else if(FLAG_CASE1_UNIT_RESOLUTION == 1){
		return unit_resolution_case_1(sat_state);
	}
	else
		return 0;
}

//undoes sat_unit_resolution(), leading to un-instantiating variables that have been instantiated
//after sat_unit_resolution()
void sat_undo_unit_resolution(SatState* sat_state) {
	unsigned long num_reduced_decisions = 0;
	// undo the set literals at the current decision level
	for(unsigned long i = sat_state->num_literals_in_decision-1; i <= 0; i--){
		if(sat_state->decisions[i]->decision_level == sat_state->current_decision_level){
			sat_undo_clauses_state(sat_state->decisions[i]);
			sat_state->decisions[i]->decision_level = 1;
			sat_state->decisions[i]->LitState = 0;
			sat_state->decisions[i]->LitValue = 'u';
			num_reduced_decisions ++;
		}
		//TODO (Performance enhancing):
		// we are incrementing the decision level one by one so once the decision level
		// of the literal is less than the current one then you can break. You don't have to
		// continue the loop
		if(sat_state->decisions[i]->decision_level < sat_state->current_decision_level){
			break;
		}

	}
	//update the current decision level
	sat_state->num_literals_in_decision = sat_state->num_literals_in_decision - num_reduced_decisions;
	sat_state->current_decision_level -- ;
}

//returns 1 if the decision level of the sat state equals to the assertion level of clause,
//0 otherwise
//
//this function is called after sat_decide_literal() or sat_assert_clause() returns clause.
//it is used to decide whether the sat state is at the right decision level for adding clause.
BOOLEAN sat_at_assertion_level(const Clause* clause, const SatState* sat_state) {
	//TODO: What is the need of this api actually???
	unsigned long asserting_level = 0;
	for(unsigned long i = 0; i < sat_state->alpha->num_literals_in_clause; i++){
		if(asserting_level < sat_state->alpha->literals[i]->decision_level)
			asserting_level = sat_state->alpha->literals[i]->decision_level;
	}

	if(sat_state->current_decision_level == asserting_level)
		return 1;
	else
		return 0;
}

/******************************************************************************
 * The functions below are already implemented for you and MUST STAY AS IS
 ******************************************************************************/

//returns the weight of a literal (which is 1 for our purposes)
c2dWmc sat_literal_weight(const Lit* lit) {
  return 1;
}

//returns 1 if a variable is marked, 0 otherwise
BOOLEAN sat_marked_var(const Var* var) {
  return var->mark;
}

//marks a variable (which is not marked already)
void sat_mark_var(Var* var) {
  var->mark = 1;
}

//unmarks a variable (which is marked already)
void sat_unmark_var(Var* var) {
  var->mark = 0;
}

//returns 1 if a clause is marked, 0 otherwise
BOOLEAN sat_marked_clause(const Clause* clause) {
  return clause->mark;
}

//marks a clause (which is not marked already)
void sat_mark_clause(Clause* clause) {
  clause->mark = 1;
}
//unmarks a clause (which is marked already)
void sat_unmark_clause(Clause* clause) {
  clause->mark = 0;
}

/******************************************************************************
 * end
 ******************************************************************************/
