#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <raaSystem/raaSystem.h>
#include <raaMaths/raaMaths.h>

#include "raaConstants.h"
#include "raaParse.h"
#include "raaComputerGraphicsAssignment1/Config.h"


extern raaSystem g_System;
extern raaConfig g_Config;

unsigned int g_uiParseMode = 0;
unsigned int g_uiParseField = 0;
unsigned int g_uiParseCount = 0;

void parseSection(const char* acRaw, const char* acSection, const char* acDescription, const char* acType, const char* acData) 
{
	if (!strcmp(acSection, "*Network")) g_uiParseMode = csg_uiParseNetwork;
	else if (!strcmp(acSection, "*Vector"))
	{
		g_uiParseMode = csg_uiParseVector;
		g_uiParseCount = 1;

		if (!strcmp(acDescription, "x_coordinates")) g_uiParseField = csg_uiParseXCoord;
		else if (!strcmp(acDescription, "GDP_1995.vec")) g_uiParseField = csg_uiParseGDP;
	}
	else if (!strcmp(acSection, "*Partition"))
	{
		g_uiParseMode = csg_uiParsePartition;
		g_uiParseCount = 1;

		if (!strcmp(acDescription, "Continent")) g_uiParseField = csg_uiParseContinent;
		else if (!strcmp(acDescription, "World_system")) g_uiParseField = csg_uiParseWorldSystem;
	}
	else g_uiParseMode = 0;
}

void parseNetwork(const char* acRaw, const char* acId, const char* acName, const char* acY, const char* acZ) 
{
	float afPos[] = { 0.0f, (float)atof(acY)*csg_afParseLayoutScale[csg_uiY], (float)atof(acZ)*csg_afParseLayoutScale[csg_uiZ], 1.0f };
	addNode(&g_System, initNode(new raaNode, atoi(acId), afPos, randFloat(0.0f, 300.0f), acName));
}

void parseArc(const char* acRaw, const char* acId0, const char* acId1, const char* acStrength) 
{
	raaNode *pN0 = nodeById(&g_System, atoi(acId0));
	raaNode *pN1 = nodeById(&g_System, atoi(acId1));
	float springCoefficient = (float)atof(acStrength);

	if (pN0 && pN1) addArc(&g_System, initArc(new raaArc, pN0, pN1, springCoefficient, randFloat(100.0f, 500.0f), randFloat(0.1f, 1.0f)));
}

void parsePartition(const char* acRaw, const char* acValue) 
{
	int iValue = atoi(acValue);

	if (g_uiParseField == csg_uiParseContinent)
	{
		raaNode *pNode = nodeById(&g_System, g_uiParseCount++);
		if (pNode) pNode->m_uiContinent = iValue;

	}
	else if (g_uiParseField == csg_uiParseWorldSystem)
	{
		raaNode *pNode = nodeById(&g_System, g_uiParseCount++);
		if (pNode) {
			pNode->m_uiWorldSystem = iValue;
			// Modify the mass based on the world system. The assumption is that world system 2 means a large country, whilst 0 - a small one. 
			if (iValue == 0) pNode->m_fMass = randFloat(0.0f, 100.0f);
			else if (iValue == 1) pNode->m_fMass = randFloat(100.0f, 200.0f);
			else if (iValue == 2) pNode->m_fMass = randFloat(200.0f, 400.0f);
		}
	}
}

void parseVector(const char* acRaw, const char* acValue)
{
	float fValue = (float)atof(acValue);

	if (g_uiParseField == csg_uiParseXCoord)
	{
		raaNode* pNode = nodeById(&g_System, g_uiParseCount++);

		if (pNode) pNode->m_afPosition[csg_uiX] = fValue * 800.0f;
	}
	else if (g_uiParseField == csg_uiParseGDP)
	{
		raaNode* pNode = nodeById(&g_System, g_uiParseCount++);

		if (pNode) pNode->m_fMass = fValue;
	}
}

// Function to update ideal length of arcs
void arcsLengthByContinent(raaArc* pArc) {
	if (pArc && pArc->m_pNode0 && pArc->m_pNode1) {
		pArc->m_fIdealLen = (pArc->m_pNode0->m_uiContinent == pArc->m_pNode1->m_uiContinent) ? 200.0f : 400.0f;
	}
}

// Function to update non-linear factor of arcs. This means that depeding on how heavy it is, the spring will get tighter with more force applied. 
void applySpringNonLinearFactor(raaArc* pArc) {
	if (pArc && pArc->m_pNode0 && pArc->m_pNode1) {
		float totalMass = pArc->m_pNode0->m_fMass + pArc->m_pNode1->m_fMass;
		pArc->m_fNonLinearFactor = 1.0f / (totalMass + 1.0f); // Avoid division by zero
	}
}

void applyFlatArcLength(raaArc* pArc) {
	pArc->m_fIdealLen = g_Config.m_fArcLength;
}

void applyRandomArcLength(raaArc* pArc) {
	pArc->m_fIdealLen = randFloat(g_Config.m_fArcLength, g_Config.m_fMaxArcLength);
}
