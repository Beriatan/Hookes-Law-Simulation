#include <windows.h>
#include <gl/gl.h>
#include <gl/glu.h>
#include <gl/glut.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <raaCamera/raaCamera.h>
#include <raaUtilities/raaUtilities.h>
#include <raaMaths/raaMaths.h>
#include <raaText/raaText.h>
#include "raaSystem/raaSystem.h"
#include "Config.h"
#include "raaConstants.h"


extern raaConfig g_Config;
extern raaStatus g_Status;
extern raaCamera g_Camera; // structure holding the camera position and orientation attributes



GLfloat afColours[][4] = {
	{1.0f, 0.0f, 0.0f, 1.0f}, // 0 - Red - Africa
	{0.0f, 1.0f, 0.0f, 1.0f}, // 1 - Green - Asia
	{0.0f, 0.0f, 1.0f, 1.0f}, // 2 -  Blue - Europe
	{1.0f, 1.0f, 0.0f, 1.0f}, // 3 - Yellow - North America
	{1.0f, 0.0f, 1.0f, 1.0f}, // 4 - Magenta - Oceania
	{0.0f, 1.0f, 1.0f, 1.0f}, // 5 - Cyan - South America
	{1.0f, 1.0f, 1.0f, 1.0f} // 6 - White 
};

void printPause()
{
	if (g_Status.m_bIsSimulationRunning) {
		printf("Simulation running\n");
	}
	else {
		printf("Simulation paused\n");
	}
}


void nodeDisplay(raaNode* pNode) {
	float* position = pNode->m_afPosition;
	unsigned int continent = pNode->m_uiContinent;
	unsigned int worldSystem = pNode->m_uiWorldSystem;

	glPushMatrix();
	glPushAttrib(GL_ALL_ATTRIB_BITS);

	// Check if the node is highlighted
	if (pNode->m_bHighlighted) {
		utilitiesColourToMat(afColours[6], 2.0f, true);
	}
	else {
		// Set color based on continent
		if (continent > 0 && continent <= 6) {
			utilitiesColourToMat(afColours[continent - 1], 2.0f, true);
		}
		else {
			GLfloat defaultColor[] = { 0.5f, 0.5f, 0.5f, 1.0f }; // Grey for unknown continent
			utilitiesColourToMat(defaultColor, 2.0f, true);
		}
	}

	glTranslated(position[0], position[1], position[2]);

	// Draw different shapes based on world system
	switch (worldSystem) {
	case 1:
		glutSolidSphere(mathsRadiusOfSphereFromVolume(pNode->m_fMass), 15, 15); // Sphere
		break;
	case 2:
		glutSolidCube(mathsDimensionOfCubeFromVolume(pNode->m_fMass)); // Cube
		break;
	case 3: {
		float coneVolume = pNode->m_fMass;
		float coneRadius = mathsRadiusOfConeFromVolume(coneVolume);
		float coneHeight = (3.0f * coneVolume) / (M_PI * coneRadius * coneRadius);
		glutSolidCone(coneRadius, coneHeight, 15, 15); // Cone
	}
		  break;
	default:
		glutSolidSphere(mathsRadiusOfSphereFromVolume(pNode->m_fMass), 15, 15); // Default to sphere
		break;
	}

	glMultMatrixf(camRotMatInv(g_Camera));
	// Render the node's name
	glScalef(16, 16, 0.1f);
	glTranslatef(0.0f, 2.0f, 0.0f); // Move above the node
	outlinePrint(pNode->m_acName, true); // Print the name

	glPopMatrix();
	glPopAttrib();
}



void arcDisplay(raaArc* pArc) {
	raaNode* m_pNode0 = pArc->m_pNode0;
	raaNode* m_pNode1 = pArc->m_pNode1;

	float* arcPos0 = m_pNode0->m_afPosition;
	float* arcPos1 = m_pNode1->m_afPosition;

	bool flowDirection = pArc->m_bFlowDirection; // true for Node0 to Node1, false for Node1 to Node0


	// Define colors
	GLfloat red[3] = { 0.8f, 0.0f, 0.0f };
	GLfloat brightGreen[3] = { 0.0f, 0.8f, 0.0f };

	glEnable(GL_COLOR_MATERIAL);
	glDisable(GL_LIGHTING);
	glBegin(GL_LINES);
	if (flowDirection) {

		glColor3fv(brightGreen);
		glVertex3fv(arcPos1);
		glColor3fv(red);
		glVertex3fv(arcPos0);
	}
	else {
		glColor3fv(red);
		glVertex3fv(arcPos0);
		glColor3fv(brightGreen);
		glVertex3fv(arcPos1);
	}
	glEnd();
}

void renderHUD() {
	// Save the current OpenGL states
	glPushAttrib(GL_ALL_ATTRIB_BITS);

	// Save the current projection matrix and set up an orthogonal projection
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	int width = glutGet(GLUT_WINDOW_WIDTH);
	int height = glutGet(GLUT_WINDOW_HEIGHT);
	gluOrtho2D(0, width, 0, height);

	// Switch to modelview matrix and save it
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	// Disable depth testing for HUD rendering
	glDisable(GL_DEPTH_TEST);

	// Display simulation control text
	char controlText[256];
	sprintf_s(controlText, "Press P to %s the simulation", g_Status.m_bIsSimulationRunning ? "pause" : "play");
	glRasterPos2i(10, height - 20);
	for (char* c = controlText; *c != '\0'; c++) {
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
	}

	// Display kinetic energy
	char energyText[256];
	sprintf_s(energyText, "Total Kinetic Energy: %.2f Joules", g_Status.m_fSimulationKineticEnergy);
	glRasterPos2i(10, height - 40);
	for (char* c = energyText; *c != '\0'; c++) {
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
	}

	// Display Speed multiplier
	char multiplierText[256];
	sprintf_s(multiplierText, "Speed multiplier: %.2fX", 1 / g_Config.m_fSimulationSpeedMultiplier);
	glRasterPos2i(10, height - 60);
	for (char* c = multiplierText; *c != '\0'; c++) {
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
	}

	// Display Damping Factor
	char dampingFactorText[256];
	sprintf_s(dampingFactorText, "Damping factor: %.2fX, base: %.2f", g_Config.m_fDampingFactor, g_Config.m_fBaseDampingFactor);
	glRasterPos2i(10, height - 80);
	for (char* c = dampingFactorText; *c != '\0'; c++) {
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
	}

	// Restore the modelview matrix
	glPopMatrix();

	// Restore the projection matrix
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	// Restore the saved OpenGL states
	glPopAttrib();

	// Switch back to modelview matrix
	glMatrixMode(GL_MODELVIEW);
}

