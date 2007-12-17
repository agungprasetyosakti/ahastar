/*
 *  AnnotatedMapAbstraction.cpp
 
	Creates a graph abstraction of a map and annotates that graph with clearance values and terrain types.

	Clerance is defined as the amount of space (measured in tiles) around any given location on a map. We take as a measure of clearance the minimum of 
	three rays drawn from any given map tile to the edge of the area in the S, E, and SE directions.
	We add  +1 (the minimum clearance any node on the map is guaranteed to have) to arrive at the final value.      
	 _ _ _ _ _ 
	|x|_|_|_|_|
	|_|_|_|_|_|
	|_|_|_|y|_|
	|_|_|_|_|z|
	
	Eg. the clearance values for the three tiles above are: x=4, y=2, z=1.
	
	Each node has 1 clearance value annotation for each combination of individual terrain types.
	Eg. If the basic terrain types are Ground and Trees then 2^n - 1 combinations of terrain exist and hence 2^n - n annotations are needed.

 *  hog
 *
 *  Created by Daniel Harabor on 5/12/07.
 *  Copyright 2007 __MyCompanyName__. All rights reserved.
 *
 */

#include "AnnotatedMapAbstraction.h"
#include "AnnotatedAStar.h"
//#ifdef OSMAC
#include "GLUT/glut.h"
#include <OpenGL/gl.h>
/*#else
#include <GL/glut.h>
#include <GL/gl.h>
#endif*/

using namespace std;

AnnotatedMapAbstraction::AnnotatedMapAbstraction(Map* m) : mapAbstraction(m) 
{
	validterrains[0] = kGround; 
	validterrains[1] = kTrees; 
	validterrains[2] = (kGround|kTrees);
	
	abstractions.push_back(getMapGraph(m));
	addMissingEdges();
	annotateMap();
	
	drawCV=false; // disable drawing of clearance values
}

/* annotateMap
	Annotates the mapAbstraction with terrain and clearance value annotations 
*/
void AnnotatedMapAbstraction::annotateMap() 
{

	// clearance values is a recursive process; the bottom-right corner of the grid is the recursive base for all other calculations.
	for(int x=getMap()->getMapWidth()-1;x>=0; x--)
	{
		for(int y=getMap()->getMapHeight()-1;y>=0; y--)
		{
			node* n = getNodeFromMap(x,y);
			if(n) // some tiles may not have corresponding nodes (obstacle terrain type ('@'))
			{
				n->setTerrainType(getMap()->getTerrainType(x,y)); //NB: duplicates map data but much easier to access; separate tiles/nodes sucks

				node *adj1, *adj2, *adj3; // adjacent neighbours
				adj1 = getNodeFromMap(x+1, y+1);
				adj2 = getNodeFromMap(x+1,y);
				adj3 = getNodeFromMap(x,y+1);
				
				if(adj1 && adj2 && adj3)
				{	
					for(int i=0; i<3; i++) // NB: hard-coded assumption about # of valid terrains
					{
						/* only want to calculate annotations for combinations that include the basic (single) terrain type node has*/
						if((validterrains[i]&n->getTerrainType())==n->getTerrainType())
						{
							int min = adj1->getClearance(validterrains[i]);
							min = adj2->getClearance(validterrains[i])<min?adj2->getClearance(validterrains[i]):min;
							min = adj3->getClearance(validterrains[i])<min?adj3->getClearance(validterrains[i]):min;
							n->setClearance(validterrains[i], min+1); // NB: +1 for minimum tile clearance
						}
						else
							n->setClearance(validterrains[i], 0);

					}
				}
				else // tile is on a border or perimeter boundary. clearance = 1
				{
					for(int i=0; i<3; i++) 
					{
						if((validterrains[i]&n->getTerrainType())==n->getTerrainType())
							n->setClearance(validterrains[i], 1);
						else
							n->setClearance(validterrains[i], 0);
					}

				}
			}
			
		}
	}

}

/* addMissingEdges
        Overcomes the limitations present in HOG's mapAbstraction class: neighbouring nodes of different terrain types do not have an edge between 
        them; presumably because the intention is that such nodes should not be traversable. The only problem is that this decision shouldn't be made
        by the mapAbstraction class but the agent traversing the map. 
        
        So, this method adds the missing edges between neighboring nodes and leaves it up to the agent to figure out what nodes are traversable vs not 
        according to its unique characteristics/abilities        
*/
void AnnotatedMapAbstraction::addMissingEdges()
{
	int mheight = this->getMap()->getMapHeight();
	int mwidth = this->getMap()->getMapWidth();
	graph *g = this->getAbstractGraph(0);
	
	for(int x=0; x<mwidth; x++)
		for(int y=0;y<mheight;y++)
		{
			node *n = this->getNodeFromMap(x,y);
			if(n)
			{	
				int nid = n->getNum();
				node *neighbours[3];
				neighbours[0] = getNodeFromMap(x-1,y);
				neighbours[1] = getNodeFromMap(x,y-1);
				neighbours[2] = getNodeFromMap(x-1,y-1);
				
				for(int i=0;i<3;i++)
				{
					if(neighbours[i] && !(g->findEdge(nid,neighbours[i]->getNum())))
					{
						edge *e = new edge(nid, neighbours[i]->getNum(), 1.0);
						g->addEdge(e);
					}
				}
			}
		}
}

/*	Determines if a valid solution exists between two locations given some size and terrain constraints 
	NB: Unlike other HOG mapAbstractions, pathable is really difficult to do here; there's no actual abstraction going on; 
	only a 1:1 node/tile representation which we annotate with clearance values for navigation.
	Consequently, the only way to determine pathability is to run the search and see if it's OK -- so, not very useful for quickly
	evaluating sets of locations!
	
	Class is a building-block for AnnotatedClusterAbstraction (which handles the above much better)
*/

bool AnnotatedMapAbstraction::pathable(node* from, node* to, int terrain, int agentsize)
{
	if(!((from->getTerrainType()&terrain) == terrain && (to->getTerrainType()&terrain) == terrain
		&& from->getClearance(from->getTerrainType()) >= agentsize && to->getClearance(to->getTerrainType()) >= agentsize))
		return false;
	
	AnnotatedAStar aastar; 
	path *p = aastar.getPath(this, from, to, terrain, agentsize);
	
	if(p)
		return true;
	
	return false;	
}

bool AnnotatedMapAbstraction::pathable(node* from, node* to)
{
	return pathable(from, to, (kGround|kTrees), 1); // default values if none specified
}

void AnnotatedMapAbstraction::openGLDraw()
{
	drawClearanceInfo();
	mapAbstraction::openGLDraw();
}

void AnnotatedMapAbstraction::drawClearanceInfo()
{
        char clearance[2];
        node* n;
        recVec rv;
        double r;

        glColor3f (0.51F, 1.0F, 0.0F);
        for(int i=0;i<this->getMap()->getMapWidth(); i++)
                for(int j=0; j<this->getMap()->getMapHeight();j++)
                {
				
                        n = this->getNodeFromMap(i,j,kNone);
						if(n)
						{
							sprintf(clearance, "%x", n->getClearance(n->getTerrainType()));
							this->getMap()->getOpenGLCoord(i,j,rv.x,rv.y,rv.z,r);
							glRasterPos3f((float)rv.x-0.02, (float)rv.y+0.01, rv.z-0.011);
							glutBitmapCharacter (GLUT_BITMAP_HELVETICA_12, clearance[0]);
							glutBitmapCharacter (GLUT_BITMAP_HELVETICA_12, clearance[1]);
						}
                }
}

