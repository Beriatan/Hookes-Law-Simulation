#include <windows.h>
#include <map>
#include <vector>
#include <gl/gl.h>


#include "raaSystem/raaSystem.h"
#include "raaMaths/raaMaths.h"
#include "raaMaths/raaVector.h"
#include "Config.h"
#include "Physics.h"
#include "raaCamera/raaCamera.h"


extern raaSystem g_System;
extern raaConfig g_Config;
extern raaStatus g_Status;
extern raaCamera g_Camera;



static int g_nodeIndex = 0; // Global index to keep track of node positioning
static int g_lineNodeIndex = 0; // Global index for line positioning
raaNode* g_pSelectedNode = nullptr; // Global pointer to store the selected node
float g_fOrigin[3] = { 0.0f,0.0f,0.0f };
float g_fDirection[3] = { 0.0f,0.0f,0.0f };



void arrangeNodesInContinentLineUsingVectorClass() {
	std::map<unsigned int, std::vector<raaNode*>> continentGroups;
	float spacing = 50.0f; // Spacing between nodes
	float continentSpacing = 300.0f; // Spacing between continents

	// Group nodes by continent
	for (raaLinkedListElement* pElement = head(&g_System.m_llNodes); pElement != NULL; pElement = pElement->m_pNext) {
		raaNode* pNode = static_cast<raaNode*>(pElement->m_pData);
		continentGroups[pNode->m_uiContinent].push_back(pNode);
	}

	float overallPosition = 0.0f;
	for (auto& continentGroup : continentGroups) {
		float continentPosition = overallPosition;
		for (raaNode* pNode : continentGroup.second) {
			vecSet(continentPosition, 0.0f, 0.0f, pNode->m_afPosition); // Set X, keep Y and Z at 0
			continentPosition += spacing;
			overallPosition = continentPosition + continentSpacing;
		}
	}
}

void positionNodeInLine(raaNode* pNode, int nodeIndex, float spacing) {
	float startX = -spacing * nodeIndex / 2.0f;
	pNode->m_afPosition[0] = startX + nodeIndex * spacing;
	pNode->m_afPosition[1] = 0.0f; // You can adjust Y and Z as needed
	pNode->m_afPosition[2] = 0.0f;
}

void applyLineLayout(raaNode* pNode) {
	if (!pNode) return;

	float spacing = 50.0f; // Spacing between nodes
	positionNodeInLine(pNode, g_lineNodeIndex++, spacing);
}

void arrangeNodesInLine() {
	g_lineNodeIndex = 0; // Reset the index
	visitNodes(&g_System, applyLineLayout);
}

void calculateCuboidDimensions(int totalNodes, int* dimensions) {
	int baseDimension = (int)round(pow((double)totalNodes, 1.0 / 3.0));
	dimensions[0] = baseDimension;
	dimensions[1] = baseDimension;
	dimensions[2] = baseDimension;

	while (dimensions[0] * dimensions[1] * dimensions[2] < totalNodes) {
		dimensions[0]++;
		if (dimensions[0] * dimensions[1] * dimensions[2] < totalNodes) dimensions[1]++;
		if (dimensions[0] * dimensions[1] * dimensions[2] < totalNodes) dimensions[2]++;
	}
}

void positionNodeInCuboid(raaNode* pNode, int* dimensions, int nodeIndex, float spacing) {
	int x = nodeIndex % dimensions[0];
	int y = (nodeIndex / dimensions[0]) % dimensions[1];
	int z = nodeIndex / (dimensions[0] * dimensions[1]);

	float startX = -(dimensions[0] - 1) * spacing / 2.0f;
	float startY = -(dimensions[1] - 1) * spacing / 2.0f;
	float startZ = -(dimensions[2] - 1) * spacing / 2.0f;

	pNode->m_afPosition[0] = startX + x * spacing;
	pNode->m_afPosition[1] = startY + y * spacing;
	pNode->m_afPosition[2] = startZ + z * spacing;
}

void applyCuboidLayout(raaNode* pNode) {
	if (!pNode) return;

	int dimensions[3];
	int totalNodes = count(&(g_System.m_llNodes));
	calculateCuboidDimensions(totalNodes, dimensions);
	float spacing = 100.0f; // Spacing between nodes

	positionNodeInCuboid(pNode, dimensions, g_nodeIndex++, spacing);
}

void arrangeNodesInCuboid() {
	g_nodeIndex = 0; // Reset the index
	visitNodes(&g_System, applyCuboidLayout);
}

void randomizeNodePositions(raaSystem* pSystem, float fMinRange, float fMaxRange) {
	if (!pSystem) return;

	for (raaLinkedListElement* pElement = head(&(pSystem->m_llNodes)); pElement != NULL; pElement = pElement->m_pNext) {
		raaNode* pNode = static_cast<raaNode*>(pElement->m_pData);
		if (pNode) {
			// Randomize position within the specified range
			pNode->m_afPosition[0] = randFloat(fMinRange, fMaxRange);
			pNode->m_afPosition[1] = randFloat(fMinRange, fMaxRange);
			pNode->m_afPosition[2] = randFloat(fMinRange, fMaxRange);
		}
	}
}

float calculateTotalKineticEnergy(raaSystem* pSystem) {
	float totalKineticEnergy = 0.0f;

	for (raaLinkedListElement* pElement = head(&(pSystem->m_llNodes)); pElement != NULL; pElement = pElement->m_pNext) {
		raaNode* pNode = static_cast<raaNode*>(pElement->m_pData);
		if (pNode) {
			float velocitySquared = pNode->m_afVelocity[0] * pNode->m_afVelocity[0] +
				pNode->m_afVelocity[1] * pNode->m_afVelocity[1] +
				pNode->m_afVelocity[2] * pNode->m_afVelocity[2];
			totalKineticEnergy += 0.5f * pNode->m_fMass * velocitySquared;
		}
	}

	return totalKineticEnergy;
}

static void applyForceToNode(raaNode* pNode, float forceX, float forceY, float forceZ) {
	if (!pNode) return;

	// Calculate acceleration (a = F/m)
	float ax = forceX / pNode->m_fMass;
	float ay = forceY / pNode->m_fMass;
	float az = forceZ / pNode->m_fMass;

	// Base damping factor
	g_Config.m_fDampingFactor = g_Config.m_fBaseDampingFactor;

	// Define a threshold for kinetic energy
	float kineticEnergyThreshold = 40000000000000.0f;

	// If kinetic energy is above the threshold, decrease damping exponentially
	if (g_Status.m_fSimulationKineticEnergy > kineticEnergyThreshold) {
		float exponentFactor = 0.05f; // Control how quickly damping decreases
		g_Config.m_fDampingFactor -= exponentFactor * (g_Status.m_fSimulationKineticEnergy - kineticEnergyThreshold);
	}

	// Ensure dampingFactor doesn't go negative

	g_Config.m_fDampingFactor = max(0.0f, g_Config.m_fDampingFactor);


	pNode->m_afVelocity[0] *= g_Config.m_fDampingFactor;
	pNode->m_afVelocity[1] *= g_Config.m_fDampingFactor;
	pNode->m_afVelocity[2] *= g_Config.m_fDampingFactor;

	// Update velocity (v = v + a * deltaTime)
	pNode->m_afVelocity[0] += ax * g_Status.m_fDeltaTime;
	pNode->m_afVelocity[1] += ay * g_Status.m_fDeltaTime;
	pNode->m_afVelocity[2] += az * g_Status.m_fDeltaTime;

	// Update position (p = p + v * deltaTime)
	pNode->m_afPosition[0] += pNode->m_afVelocity[0] * g_Status.m_fDeltaTime;
	pNode->m_afPosition[1] += pNode->m_afVelocity[1] * g_Status.m_fDeltaTime;
	pNode->m_afPosition[2] += pNode->m_afVelocity[2] * g_Status.m_fDeltaTime;
}



void calculateSpringForce(raaArc* pArc) {
	if (!pArc || !pArc->m_pNode0 || !pArc->m_pNode1) return;

	float displacementVec[3], forceVec[3];
	vecSub(pArc->m_pNode1->m_afPosition, pArc->m_pNode0->m_afPosition, displacementVec);
	float distance = vecLength(displacementVec);

	float displacement = distance - pArc->m_fIdealLen;
	float forceMagnitude = pArc->m_fSpringCoef * displacement + pArc->m_fNonLinearFactor * displacement * displacement;
	vecScalarProduct(displacementVec, forceMagnitude / distance, forceVec);

	vecAdd(pArc->m_pNode0->m_afAccumulatedForce, forceVec, pArc->m_pNode0->m_afAccumulatedForce);
	vecScalarProduct(forceVec, -1, forceVec); // Inverting the force for the other node
	vecAdd(pArc->m_pNode1->m_afAccumulatedForce, forceVec, pArc->m_pNode1->m_afAccumulatedForce);
}


void applyAccumulatedForces(raaNode* pNode) {
	if (!pNode) return;
	applyForceToNode(pNode, pNode->m_afAccumulatedForce[0], pNode->m_afAccumulatedForce[1], pNode->m_afAccumulatedForce[2]);

	// Reset the accumulated force for the next frame
	pNode->m_afAccumulatedForce[0] = 0.0f;
	pNode->m_afAccumulatedForce[1] = 0.0f;
	pNode->m_afAccumulatedForce[2] = 0.0f;
}

void accumulateAllSpringForces() {
	visitArcs(&g_System, calculateSpringForce);
}

void applyAccumulatedSpringForcesToNodes() {
	visitNodes(&g_System, applyAccumulatedForces);
}

bool raySphereIntersect(const float* rayOrigin, const float* rayDir, const raaNode& node, float radius, float& t) {
	float v[3];
	vecSub(const_cast<float*>(node.m_afPosition), const_cast<float*>(rayOrigin), v);
	float b = vecDotProduct(const_cast<float*>(v), const_cast<float*>(rayDir));
	float vDotV = vecDotProduct(const_cast<float*>(v), const_cast<float*>(v));
	float det = (b * b) - vDotV + (radius * radius);

	if (det < 0) {
		return false;
	}

	det = sqrtf(det);
	t = b - det;  // Closest intersection

	return t >= 0; // Check if intersection is in the positive ray direction
}

void dragNode(int iXPos, int iYPos) {
	if (g_pSelectedNode) {
		int viewport[4];
		glGetIntegerv(GL_VIEWPORT, viewport);
		int adjustedYPos = viewport[3] - iYPos;

		float nearPlaneWorldCoords[3], farPlaneWorldCoords[3];
		renderUnProject((float)iXPos, (float)adjustedYPos, 0.0f, camObjMat(g_Camera), g_Camera.m_afProjMat, camViewport(g_Camera), nearPlaneWorldCoords);
		renderUnProject((float)iXPos, (float)adjustedYPos, 1.0f, camObjMat(g_Camera), g_Camera.m_afProjMat, camViewport(g_Camera), farPlaneWorldCoords);

		float rayDir[3], planeNormal[3], newPosition[3];
		vecSub(farPlaneWorldCoords, nearPlaneWorldCoords, rayDir);
		vecNormalise(rayDir, rayDir);

		memcpy(planeNormal, g_Camera.m_fVD, sizeof(float) * 3);
		vecScalarProduct(planeNormal, -1.0f, planeNormal);

		float v[3];
		vecSub(g_pSelectedNode->m_afPosition, g_Camera.m_fVP, v);
		float numerator = vecDotProduct(planeNormal, v);
		float denominator = vecDotProduct(planeNormal, rayDir);

		if (denominator != 0) {
			float t = numerator / denominator;
			vecScalarProduct(rayDir, t, newPosition);
			vecAdd(g_Camera.m_fVP, newPosition, newPosition);
			memcpy(g_pSelectedNode->m_afPosition, newPosition, sizeof(float) * 3);
		}
	}
}


raaNode* findClosestIntersectedNode(const float* rayOrigin, const float* rayDir, float hitSphereRadius) {
	raaNode* closestNode = nullptr;
	float minT = FLT_MAX;

	for (raaLinkedListElement* pElement = head(&(g_System.m_llNodes)); pElement != NULL; pElement = pElement->m_pNext) {
		raaNode* pNode = static_cast<raaNode*>(pElement->m_pData);
		float t;
		if (raySphereIntersect(rayOrigin, rayDir, *pNode, hitSphereRadius, t)) {
			if (t < minT) {
				minT = t;
				closestNode = pNode;
			}
		}
	}

	return closestNode;
}

void selectNode(int iXPos, int iYPos, float hitSphereRadius) {

	g_pSelectedNode = findClosestIntersectedNode(g_fOrigin, g_fDirection, hitSphereRadius);

	// Highlight the selected node
	if (g_pSelectedNode) {
		g_pSelectedNode->m_bHighlighted = true;
	}
}

void releaseNode() {
	g_pSelectedNode = nullptr;
}
