#ifndef FCRREXPANSIONPOLICY_H
#define FCRREXPANSIONPOLICY_H

// FCRRExpansionPolicy.h
//
// Four Connected Rectangular Room expansion policy.
// Iterates over all neighbours of the target node which are reachable by 
// taking a step in one cardinal directions (up, down, left, right).
//
// Also iterates over a macro neighbour on the directly opposite side of the rectangular
// room.
//
// @author: dharabor
// @created: 28/10/2010

#include "ExpansionPolicy.h"

class EmptyClusterAbstraction;
class TileExpansionPolicy;
class FCRRExpansionPolicy : public ExpansionPolicy
{
	public:
		FCRRExpansionPolicy(EmptyClusterAbstraction*);
		virtual ~FCRRExpansionPolicy();

		virtual node* n();
		virtual node* first(); 
		virtual node* next();
		virtual bool hasNext();

	private:
		int which_macro;
		EmptyClusterAbstraction* map;
		TileExpansionPolicy* policy;
};

#endif
