#pragma once
#include "raaSystem/raaSystem.h"

// Nodes arrangement functions
void arrangeNodesInContinentLineUsingVectorClass(); // Arranges all nodes on a line, grouped by continent
void arrangeNodesInLine(); // Arranges all nodes to be placed in a line 
void arrangeNodesInCuboid(); //Arranbges all nodes into a cuboid shape
void randomizeNodePositions(raaSystem* pSystem, float fMinRange, float fMaxRange); // Randomises all node positions

// Simulation forces
float calculateTotalKineticEnergy(raaSystem* pSystem); // Calculates total kinetic energy to be displayed in the HUD
void accumulateAllSpringForces(); // Done in a single tick: calculates all forces exerted on springs at the moment
void applyAccumulatedSpringForcesToNodes(); // Apply the calculated forces onto the nodes

// Node interaction functions

void selectNode(int iXPos, int iYPos, float radius); // Finds the closest node that's hovered over with a mouse and highlights it. 

void dragNode(int iXPos, int iYPos); // Drags the nearest node that's pointed on the screen coordinates. 
void releaseNode(); // Deselects the node
raaNode* findClosestIntersectedNode(const float* rayOrigin, const float* rayDir, float hitSphereRadius);


extern raaNode* g_pSelectedNode; // Global pointer to store the selected node
extern float g_fOrigin[3]; // Global pointer to the initial location of the mouse intersection ray
extern float g_fDirection[3]; // Global pointer to the furthest location of the mouse intersection ray