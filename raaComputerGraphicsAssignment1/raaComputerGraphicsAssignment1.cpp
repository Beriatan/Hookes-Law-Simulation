#include <windows.h>
#include <gl/gl.h>
#include <gl/glu.h>
#include <gl/glut.h>
#include <map>
#include <conio.h>
#include <iostream>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <raaCamera/raaCamera.h>
#include <raaUtilities/raaUtilities.h>
#include <raaMaths/raaMaths.h>
#include <raaMaths/raaVector.h>
#include <raaSystem/raaSystem.h>
#include <raaPajParser/raaPajParser.h>
#include <raaText/raaText.h>
#include "raaSystem/raaSystem.h"
#include "raaConstants.h"
#include "Config.h"
//#include "Graphics.h"


#include "raaConstants.h"
#include "raaParse.h"
#include "raaControl.h"

// NOTES
// look should look through the libraries and additional files I have provided to familarise yourselves with the functionallity and code.
// The data is loaded into the data structure, managed by the linked list library, and defined in the raaSystem library.
// You will need to expand the definitions for raaNode and raaArc in the raaSystem library to include additional attributes for the siumulation process
// If you wish to implement the mouse selection part of the assignment you may find the camProject and camUnProject functions usefull


// core system global data
raaCameraInput g_Input; // structure to hadle input to the camera comming from mouse/keyboard events
raaSystem g_System; // data structure holding the imported graph of data - you may need to modify and extend this to support your functionallity
raaControl g_Control; // set of flag controls used in my implmentation to retain state of key actions
raaCamera g_Camera; // structure holding the camera position and orientation attributes
extern raaConfig g_Config;
extern raaStatus g_Status;

//bool g_bIsSimulationRunning = false; //global variable to control the simulation state
//float g_fSimulationKineticEnergy = 0.0f;;
//float g_fSimulationSpeedMultiplier = 1.0f;
//float g_fDampingFactor = 0.97f;
//float g_fBaseDampingFactor = g_fDampingFactor;

float g_fMouseWorldX = 0.0f;
float g_fMouseWorldY = 0.0f;
float g_fMouseWorldZ = 0.0f;

LARGE_INTEGER frequency;
LARGE_INTEGER lastFrameTime;
//float g_deltaTime = 0.0f; // Time it takes for the program to tick. Calculated when idle

GLfloat afColours[][4] = {
	{1.0f, 0.0f, 0.0f, 1.0f}, // 0 - Red - Africa
	{0.0f, 1.0f, 0.0f, 1.0f}, // 1 - Green - Asia
	{0.0f, 0.0f, 1.0f, 1.0f}, // 2 -  Blue - Europe
	{1.0f, 1.0f, 0.0f, 1.0f}, // 3 - Yellow - North America
	{1.0f, 0.0f, 1.0f, 1.0f}, // 4 - Magenta - Oceania
	{0.0f, 1.0f, 1.0f, 1.0f}, // 5 - Cyan - South America
	{1.0f, 1.0f, 1.0f, 1.0f} // 6 - White 
};

// global var: parameter name for the file to load
const static char csg_acFileParam[] = { "-input" };
// global var: file to load data from
char g_acFile[256];



// core functions -> reduce to just the ones needed by glut as pointers to functions to fulfill tasks
void display(); // The rendering function. This is called once for each frame and you should put rendering code here
void idle(); // The idle function is called at least once per frame and is where all simulation and operational code should be placed
void reshape(int iWidth, int iHeight); // called each time the window is moved or resived
void keyboard(unsigned char c, int iXPos, int iYPos); // called for each keyboard press with a standard ascii key
void keyboardUp(unsigned char c, int iXPos, int iYPos); // called for each keyboard release with a standard ascii key
void sKeyboard(int iC, int iXPos, int iYPos); // called for each keyboard press with a non ascii key (eg shift)
void sKeyboardUp(int iC, int iXPos, int iYPos); // called for each keyboard release with a non ascii key (eg shift)
void mouse(int iKey, int iEvent, int iXPos, int iYPos); // called for each mouse key event
void motion(int iXPos, int iYPos); // called for each mouse motion event

// Non glut functions
void myInit(); // the myinit function runs once, before rendering starts and should be used for setup
void buildGrid(); // 
void printPause(); //Outputs the running status of the simulation to the console
void nodeDisplay(raaNode* pNode); // callled by the display function to draw nodes
void arcDisplay(raaArc* pArc); // called by the display function to draw arcs

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

void positionNodeInLine(raaNode* pNode, int nodeIndex, float spacing) {
	float startX = -spacing * nodeIndex / 2.0f;
	pNode->m_afPosition[0] = startX + nodeIndex * spacing;
	pNode->m_afPosition[1] = 0.0f; // You can adjust Y and Z as needed
	pNode->m_afPosition[2] = 0.0f;
}


static int g_nodeIndex = 0; // Global index to keep track of node positioning

static int g_lineNodeIndex = 0; // Global index for line positioning

void applyLineLayout(raaNode* pNode) {
	if (!pNode) return;

	float spacing = 50.0f; // Spacing between nodes
	positionNodeInLine(pNode, g_lineNodeIndex++, spacing);
}

void arrangeNodesInLine() {
	g_lineNodeIndex = 0; // Reset the index
	visitNodes(&g_System, applyLineLayout);
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
	sprintf_s(dampingFactorText, "Damping factor: %.2fX, base: %.2f", g_Config.m_fDampingFactor , g_Config.m_fBaseDampingFactor);
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

float distanceBetweenNodes(raaNode* pNode1, raaNode* pNode2) {
	if (!pNode1 || !pNode2) return 0.0f;

	float dx = pNode2->m_afPosition[0] - pNode1->m_afPosition[0];
	float dy = pNode2->m_afPosition[1] - pNode1->m_afPosition[1];
	float dz = pNode2->m_afPosition[2] - pNode1->m_afPosition[2];

	return sqrt(dx * dx + dy * dy + dz * dz);
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

void applyAccumulatedForces(raaNode* pNode) {
	if (!pNode) return;
	applyForceToNode(pNode, pNode->m_afAccumulatedForce[0], pNode->m_afAccumulatedForce[1], pNode->m_afAccumulatedForce[2]);

	// Reset the accumulated force for the next frame
	pNode->m_afAccumulatedForce[0] = 0.0f;
	pNode->m_afAccumulatedForce[1] = 0.0f;
	pNode->m_afAccumulatedForce[2] = 0.0f;
}

void calculateSpringForce(raaArc* pArc) {
	if (!pArc || !pArc->m_pNode0 || !pArc->m_pNode1) return;

	// Calculate displacement vector and its magnitude
	float dx = pArc->m_pNode1->m_afPosition[0] - pArc->m_pNode0->m_afPosition[0];
	float dy = pArc->m_pNode1->m_afPosition[1] - pArc->m_pNode0->m_afPosition[1];
	float dz = pArc->m_pNode1->m_afPosition[2] - pArc->m_pNode0->m_afPosition[2];
	float distance = sqrt(dx * dx + dy * dy + dz * dz);

	// Calculate displacement from the spring's ideal length
	float displacement = distance - pArc->m_fIdealLen;

	// Calculate force magnitude using modified Hooke's law with non-linear term
	float forceMagnitude = pArc->m_fSpringCoef * displacement + pArc->m_fNonLinearFactor * displacement * displacement;

	// Normalize the displacement vector to get the direction
	float nx = dx / distance;
	float ny = dy / distance;
	float nz = dz / distance;

	// Calculate the force vector
	float forceX = forceMagnitude * nx;
	float forceY = forceMagnitude * ny;
	float forceZ = forceMagnitude * nz;

	// Accumulate forces on the nodes
	pArc->m_pNode0->m_afAccumulatedForce[0] += forceX;
	pArc->m_pNode0->m_afAccumulatedForce[1] += forceY;
	pArc->m_pNode0->m_afAccumulatedForce[2] += forceZ;

	pArc->m_pNode1->m_afAccumulatedForce[0] -= forceX;
	pArc->m_pNode1->m_afAccumulatedForce[1] -= forceY;
	pArc->m_pNode1->m_afAccumulatedForce[2] -= forceZ;
}

void updateCameraToCenterOfMass(raaSystem* pSystem, raaCamera* pCamera) {
	if (!pSystem || !pCamera) return;

	float totalMass = 0.0f;
	float centerOfMass[3] = { 0.0f, 0.0f, 0.0f };

	// Calculate the total mass and weighted position sum
	for (raaLinkedListElement* pElement = head(&(pSystem->m_llNodes)); pElement != NULL; pElement = pElement->m_pNext) {
		raaNode* pNode = static_cast<raaNode*>(pElement->m_pData);
		if (pNode) {
			totalMass += pNode->m_fMass;
			centerOfMass[0] += pNode->m_fMass * pNode->m_afPosition[0];
			centerOfMass[1] += pNode->m_fMass * pNode->m_afPosition[1];
			centerOfMass[2] += pNode->m_fMass * pNode->m_afPosition[2];
		}
	}

	// Divide by total mass to get the center of mass
	if (totalMass > 0.0f) {
		centerOfMass[0] /= totalMass;
		centerOfMass[1] /= totalMass;
		centerOfMass[2] /= totalMass;

		// Update the camera to look at the center of mass
		camExploreUpdateTarget(*pCamera, centerOfMass);
	}
}

float g_fOrigin[3] = { 0.0f,0.0f,0.0f };
float g_fDirection[3] = { 0.0f,0.0f,0.0f };




// draw the scene. Called once per frame and should only deal with scene drawing (not updating the simulator)
void display() 
{
	glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT); // clear the rendering buffers


	glLoadIdentity(); // clear the current transformation state
	glMultMatrixf(camObjMat(g_Camera)); // apply the current camera transform

	// draw the grid if the control flag for it is true	
	if (controlActive(g_Control, csg_uiControlDrawGrid)) glCallList(gs_uiGridDisplayList);

	glPushAttrib(GL_ALL_ATTRIB_BITS); // push attribute state to enable constrained state changes
	visitNodes(&g_System, nodeDisplay); // loop through all of the nodes and draw them with the nodeDisplay function
	visitArcs(&g_System, arcDisplay); // loop through all of the arcs and draw them with the arcDisplay function
	glPopAttrib();


	// draw a simple sphere
	float afCol[] = { 0.3f, 1.0f, 0.5f, 1.0f };
	utilitiesColourToMat(afCol, 2.0f);

	glPushMatrix();
	glTranslatef(0.0f, 30.0f, 0.0f);
	glutSolidSphere(5.0f, 10, 10);
	glPopMatrix();

	renderHUD();
	glFlush(); // ensure all the ogl instructions have been processed

	glutSwapBuffers(); // present the rendered scene to the screen
}

// processing of system and camera data outside of the renderng loop
void idle() 
{
	// Calculate the ticker time, so we can modify the distance of each object based on velocity
	LARGE_INTEGER currentTime;
	QueryPerformanceCounter(&currentTime);
	g_Status.m_fDeltaTime = static_cast<float>(currentTime.QuadPart - lastFrameTime.QuadPart) / (frequency.QuadPart * g_Config.m_fSimulationSpeedMultiplier);
	lastFrameTime = currentTime;

	controlChangeResetAll(g_Control); // re-set the update status for all of the control flags
	camProcessInput(g_Input, g_Camera); // update the camera pos/ori based on changes since last render
	camResetViewportChanged(g_Camera); // re-set the camera's viwport changed flag after all events have been processed

	// Check if the simulation is running
	if (g_Status.m_bIsSimulationRunning) {
		// Calculate spring forces and accumulate them on nodes
		visitArcs(&g_System, calculateSpringForce);

		// Apply accumulated forces to nodes and reset them
		visitNodes(&g_System, applyAccumulatedForces);
		g_Status.m_fSimulationKineticEnergy = calculateTotalKineticEnergy(&g_System);
	}

	glutPostRedisplay();// ask glut to update the screen
}

// respond to a change in window position or shape
void reshape(int iWidth, int iHeight)  
{
	glViewport(0, 0, iWidth, iHeight);  // re-size the rendering context to match window
	camSetViewport(g_Camera, 0, 0, iWidth, iHeight); // inform the camera of the new rendering context size
	glMatrixMode(GL_PROJECTION); // switch to the projection matrix stack 
	glLoadIdentity(); // clear the current projection matrix state
	gluPerspective(csg_fCameraViewAngle, ((float)iWidth)/((float)iHeight), csg_fNearClip, csg_fFarClip); // apply new state based on re-sized window
	glMatrixMode(GL_MODELVIEW); // swap back to the model view matrix stac
	glGetFloatv(GL_PROJECTION_MATRIX, g_Camera.m_afProjMat); // get the current projection matrix and sort in the camera model
	glutPostRedisplay(); // ask glut to update the screen
}

// detect key presses and assign them to actions
void keyboard(unsigned char c, int iXPos, int iYPos)
{
	switch(c)
	{
	case 'w':
		camInputTravel(g_Input, tri_pos); // mouse zoom
		break;
	case 's':
		camInputTravel(g_Input, tri_neg); // mouse zoom
		break;
	case 'p':
		g_Status.m_bIsSimulationRunning = !g_Status.m_bIsSimulationRunning;
		printPause();
		break;
	case 'c':
		camPrint(g_Camera); // print the camera data to the comsole
		break;
	case 'g':
		controlToggle(g_Control, csg_uiControlDrawGrid); // toggle the drawing of the grid
		break;
	}
}

// detect standard key releases
void keyboardUp(unsigned char c, int iXPos, int iYPos) 
{
	switch(c)
	{
		// end the camera zoom action
		case 'w': 
		case 's':
			camInputTravel(g_Input, tri_null);
			break;
	}
}

void sKeyboard(int iC, int iXPos, int iYPos)
{
	// detect the pressing of arrow keys for ouse zoom and record the state for processing by the camera
	switch(iC)
	{
		case GLUT_KEY_UP:
			camInputTravel(g_Input, tri_pos);
			break;
		case GLUT_KEY_DOWN:
			camInputTravel(g_Input, tri_neg);
			break;
	}
}

void sKeyboardUp(int iC, int iXPos, int iYPos)
{
	// detect when mouse zoom action (arrow keys) has ended
	switch(iC)
	{
		case GLUT_KEY_UP:
		case GLUT_KEY_DOWN:
			camInputTravel(g_Input, tri_null);
			break;
	}
}

raaNode* g_pSelectedNode = nullptr; // Global pointer to store the selected node

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

raaNode* findClosestIntersectedNode(const float* rayOrigin, const float* rayDir, raaLinkedList* pList, float radius) {
	raaNode* closestNode = nullptr;
	float minT = FLT_MAX;

	for (raaLinkedListElement* pElement = head(pList); pElement != NULL; pElement = pElement->m_pNext) {
		raaNode* pNode = static_cast<raaNode*>(pElement->m_pData);
		float t;
		if (raySphereIntersect(rayOrigin, rayDir, *pNode, radius, t)) {
			if (t < minT) {
				minT = t;
				closestNode = pNode;
			}
		}
	}

	return closestNode;
}

void selectNode(int iXPos, int iYPos, raaLinkedList* pList, float radius) {
	// Similar to the hover function, but stores the selected node globally
	// ... existing hover function code ...

	g_pSelectedNode = findClosestIntersectedNode(g_fOrigin, g_fDirection, pList, 20.0f);

	// Highlight the selected node
	if (g_pSelectedNode) {
		g_pSelectedNode->m_bHighlighted = true;
	}
}

void dragNode(int iXPos, int iYPos) {
    if (g_pSelectedNode) {
        int viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        int adjustedYPos = viewport[3] - iYPos;

        float nearPlaneWorldCoords[3], farPlaneWorldCoords[3];
        renderUnProject((float)iXPos, (float)adjustedYPos, 0.0f, camObjMat(g_Camera), g_Camera.m_afProjMat, camViewport(g_Camera), nearPlaneWorldCoords);
        renderUnProject((float)iXPos, (float)adjustedYPos, 1.0f, camObjMat(g_Camera), g_Camera.m_afProjMat, camViewport(g_Camera), farPlaneWorldCoords);

        float rayDir[3];
        vecSub(farPlaneWorldCoords, nearPlaneWorldCoords, rayDir);
        vecNormalise(rayDir, rayDir);

        // Plane normal is the negative of camera view direction
        float planeNormal[3];
        memcpy(planeNormal, g_Camera.m_fVD, sizeof(float) * 3);
        vecScalarProduct(planeNormal, -1.0f, planeNormal); // Reverse the direction

        float planePoint[3];
        memcpy(planePoint, g_pSelectedNode->m_afPosition, sizeof(float) * 3); 

        float v[3];
        vecSub(planePoint, g_Camera.m_fVP, v);
        float numerator = vecDotProduct(planeNormal, v);
        float denominator = vecDotProduct(planeNormal, rayDir);
        
        if (denominator != 0) {
            float t = numerator / denominator;
            float newPosition[3];
            vecScalarProduct(rayDir, t, newPosition);
            vecAdd(g_Camera.m_fVP, newPosition, newPosition);

            memcpy(g_pSelectedNode->m_afPosition, newPosition, sizeof(float) * 3);
        }
    }
}




void releaseNode() {
	g_pSelectedNode = nullptr;
}




void mouse(int iKey, int iEvent, int iXPos, int iYPos) {
	if (iKey == GLUT_LEFT_BUTTON) {
		if (iEvent == GLUT_DOWN) {
			// Select node on left button press
			selectNode(iXPos, iYPos, &(g_System.m_llNodes), 20.0f);
		}
		else if (iEvent == GLUT_UP) {
			// Release the node on left button release
			releaseNode();
		}
	}
	else if (iKey == GLUT_MIDDLE_BUTTON) {
		camInputMousePan(g_Input, (iEvent == GLUT_DOWN));
		if (iEvent == GLUT_DOWN) camInputSetMouseStart(g_Input, iXPos, iYPos);
	}

	// Existing camera control code
	if (!g_pSelectedNode) { // Only control camera if no node is selected
		camInputMouse(g_Input, (iEvent == GLUT_DOWN));
		if (iEvent == GLUT_DOWN) camInputSetMouseStart(g_Input, iXPos, iYPos);
	}
}


void motion(int iXPos, int iYPos) {
	if (g_pSelectedNode) {
		// Drag node if one is selected
		dragNode(iXPos, iYPos);
	}
	else {
		// Update camera movement if no node is selected
		if (g_Input.m_bMouse || g_Input.m_bMousePan) camInputSetMouseLast(g_Input, iXPos, iYPos);
	}
}


void hover(int iXPos, int iYPos) {
	// Near plane
	float nearPlaneWorldCoords[3];
	int viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport); // Get the current viewport
	int adjustedYPos = viewport[3] - iYPos;
	
	renderUnProject((float)iXPos, (float)adjustedYPos, 0.0f, camObjMat(g_Camera), g_Camera.m_afProjMat, camViewport(g_Camera), nearPlaneWorldCoords);

	// Far plane
	float farPlaneWorldCoords[3];
	renderUnProject((float)iXPos, (float)adjustedYPos, 1.0f, camObjMat(g_Camera), g_Camera.m_afProjMat, camViewport(g_Camera), farPlaneWorldCoords);

	// Ray direction (normalized)
	float rayDir[3];
	vecSub(farPlaneWorldCoords, nearPlaneWorldCoords, rayDir);
	vecNormalise(rayDir, rayDir);
	memcpy(g_fOrigin, nearPlaneWorldCoords, 3 * sizeof(float));
	memcpy(g_fDirection, rayDir, 3 * sizeof(float));

	raaNode* selectedNode = findClosestIntersectedNode(g_fOrigin, g_fDirection, &(g_System.m_llNodes), 20.0f);
	
	// Reset highlighting for all nodes
	for (raaLinkedListElement* pElement = head(&(g_System.m_llNodes)); pElement != NULL; pElement = pElement->m_pNext) {
		raaNode* pNode = static_cast<raaNode*>(pElement->m_pData);
		pNode->m_bHighlighted = false;
	}

	// Highlight the selected node
	if (selectedNode) {
		selectedNode->m_bHighlighted = true;
	}


	// Now you have the ray originating from 'nearPlaneWorldCoords' in the direction 'rayDir'
	// Next step is to check for intersection with objects (nodes) in your scene
}


void sortSubmenuAction(int option) {
	switch (option) {
	case 1:
		g_Status.m_bIsSimulationRunning = false;
		arrangeNodesInCuboid(); // Assuming 50 is the desired spacing
		break;
	case 2:
		g_Status.m_bIsSimulationRunning = true;
		randomizeNodePositions(&g_System, 0.0001f, 0.002f);
		g_Status.m_bIsSimulationRunning = false;
		arrangeNodesInLine();
		break;
	case 3:
		g_Status.m_bIsSimulationRunning = true;
		randomizeNodePositions(&g_System, 0.0001f, 0.002f);
		g_Status.m_bIsSimulationRunning = false;
		arrangeNodesInContinentLineUsingVectorClass();
		break;
	}
}

void dampingSubmenuAction(int option) {
	switch (option) {
	case 1:
		g_Config.m_fBaseDampingFactor = 0.97f;
		break;
	case 2:
		g_Config.m_fBaseDampingFactor = 1.00f;
		break;
	case 3:
		g_Config.m_fBaseDampingFactor = 0.90f;
		break;
	case 4:
		g_Config.m_fBaseDampingFactor = 0.75f;
		break;
	case 5:
		g_Config.m_fBaseDampingFactor = 0.50f;
		break;
	case 6:
		g_Config.m_fBaseDampingFactor = 0.25f;
		break;
	case 7:
		g_Config.m_fBaseDampingFactor = 0.01f;
		break;
	
	}
}

void processMenuEvents(int option) {
	switch (option) {
	case 1: 
		g_Status.m_bIsSimulationRunning = false;
		break;
	case 2:
		g_Status.m_bIsSimulationRunning = true;
		break;
	case 3:
		g_Status.m_bIsSimulationRunning = false;
		// initialise the data system and load the data file
		initSystem(&g_System);
		parse(g_acFile, parseSection, parseNetwork, parseArc, parsePartition, parseVector);
		updateArcs(&g_System);
		g_Config.m_fSimulationSpeedMultiplier = g_Config.m_fDefaultSimulationSpeed;
		updateCameraToCenterOfMass(&g_System, &g_Camera);
		break;
	case 4:
		randomizeNodePositions(&g_System, 200.0f, 800.0f);
		updateCameraToCenterOfMass(&g_System, &g_Camera);
		break; 
	case 5: // Slow down
		g_Config.m_fSimulationSpeedMultiplier *= 1.1f;
		break;
	case 6: // Speed up - but do not go above 1.8x multiplier, as it breaks the simulation
		if (1 / g_Config.m_fSimulationSpeedMultiplier < 1.8f) g_Config.m_fSimulationSpeedMultiplier /= 1.1f;
		break;
	case 7: // Reset Speed
		g_Config.m_fSimulationSpeedMultiplier = g_Config.m_fDefaultSimulationSpeed;
		break;
	case 8:
		updateCameraToCenterOfMass(&g_System, &g_Camera);
		break;
	}
	

}

void createGLUTMenus() {
	// Create a submenu for sorting
	int sort_submenu_id = glutCreateMenu(sortSubmenuAction);
	glutAddMenuEntry("Cuboid", 1); // The second parameter is the ID sent to submenuAction
	glutAddMenuEntry("Line", 2);
	glutAddMenuEntry("Line By Continent", 3);
	// Add more entries to the submenu here if needed
	int damping_submenu_id = glutCreateMenu(dampingSubmenuAction);
	glutAddMenuEntry("Default", 1);
	glutAddMenuEntry("1.00", 2);
	glutAddMenuEntry("0.90", 3);
	glutAddMenuEntry("0.75", 4);
	glutAddMenuEntry("0.50", 5);
	glutAddMenuEntry("0.25", 6);
	glutAddMenuEntry("0.01", 7);


	// Create the main menu
	int menu_id = glutCreateMenu(processMenuEvents);
	glutAddMenuEntry("Pause", 1);
	glutAddMenuEntry("Play", 2);
	glutAddMenuEntry("Reset the Simulation", 3);
	glutAddMenuEntry("Randomise the Simulation", 4);
	glutAddMenuEntry("Slow Down", 5);
	glutAddMenuEntry("Speed Up", 6);
	glutAddMenuEntry("Reset Speed", 7);
	glutAddMenuEntry("Center the camera", 8);
	glutAddSubMenu("Sort", sort_submenu_id); // Add the submenu to the main menu
	glutAddSubMenu("Change Damping Force", damping_submenu_id);


	// Finally, attach the menu to the right button
	glutAttachMenu(GLUT_RIGHT_BUTTON);
}

void myInit()
{
	// setup my event control structure
	controlInit(g_Control);

	// initalise the maths library
	initMaths();
	initConfig();
	initStatus();

	// Camera setup
	camInit(g_Camera); // initalise the camera model
	camInputInit(g_Input); // initialise the persistant camera input data 
	camInputExplore(g_Input, true); // define the camera navigation mode
	

	// opengl setup - this is a basic default for all rendering in the render loop
	glClearColor(csg_afColourClear[0], csg_afColourClear[1], csg_afColourClear[2], csg_afColourClear[3]); // set the window background colour
	glEnable(GL_DEPTH_TEST); // enables occusion of rendered primatives in the window
	glEnable(GL_LIGHT0); // switch on the primary light
	glEnable(GL_LIGHTING); // enable lighting calculations to take place
	glEnable(GL_BLEND); // allows transparency and fine lines to be drawn
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // defines a basic transparency blending mode
	glEnable(GL_NORMALIZE); // normalises the normal vectors used for lighting - you may be able to switch this iff (performance gain) is you normalise all normals your self
	glEnable(GL_CULL_FACE); // switch on culling of unseen faces
	glCullFace(GL_BACK); // set culling to not draw the backfaces of primatives

	// build the grid display list - display list are a performance optimization 
	buildGrid();
	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&lastFrameTime);

	// initialise the data system and load the data file
	initSystem(&g_System);
	parse(g_acFile, parseSection, parseNetwork, parseArc, parsePartition, parseVector);
	

	updateArcs(&g_System);

}

int main(int argc, char* argv[])
{
	// check parameters to pull out the path and file name for the data file
	for (int i = 0; i<argc; i++) if (!strcmp(argv[i], csg_acFileParam)) sprintf_s(g_acFile, "%s", argv[++i]);


	if (strlen(g_acFile)) 
	{ 
		// if there is a data file

		glutInit(&argc, (char**)argv); // start glut (opengl window and rendering manager)

		glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA); // define buffers to use in ogl
		glutInitWindowPosition(csg_uiWindowDefinition[csg_uiX], csg_uiWindowDefinition[csg_uiY]);  // set rendering window position
		glutInitWindowSize(csg_uiWindowDefinition[csg_uiWidth], csg_uiWindowDefinition[csg_uiHeight]); // set rendering window size
		glutCreateWindow("Hooke's Law - Michal Brenda");  // create rendering window and give it a name

		buildFont(); // setup text rendering (use outline print function to render 3D text


		myInit(); // application specific initialisation

		// provide glut with callback functions to enact tasks within the event loop
		glutDisplayFunc(display);
		glutIdleFunc(idle);
		glutReshapeFunc(reshape);
		glutKeyboardFunc(keyboard);
		glutKeyboardUpFunc(keyboardUp);
		glutSpecialFunc(sKeyboard);
		glutSpecialUpFunc(sKeyboardUp);
		glutMouseFunc(mouse);
		glutPassiveMotionFunc(hover);
		glutMotionFunc(motion);
		createGLUTMenus();
		glutMainLoop(); // start the rendering loop running, this will only ext when the rendering window is closed 

		
		killFont(); // cleanup the text rendering process

		return 0; // return a null error code to show everything worked
	}
	else
	{
		// if there isn't a data file 

		printf("The data file cannot be found, press any key to exit...\n");
		_getch();
		return 1; // error code
	}
}

void buildGrid()
{
	if (!gs_uiGridDisplayList) gs_uiGridDisplayList= glGenLists(1); // create a display list

	glNewList(gs_uiGridDisplayList, GL_COMPILE); // start recording display list

	glPushAttrib(GL_ALL_ATTRIB_BITS); // push attrib marker
	glDisable(GL_LIGHTING); // switch of lighting to render lines

	glColor4fv(csg_afDisplayListGridColour); // set line colour

	// draw the grid lines
	glBegin(GL_LINES);
	for (int i = (int)csg_fDisplayListGridMin; i <= (int)csg_fDisplayListGridMax; i++)
	{
		glVertex3f(((float)i)*csg_fDisplayListGridSpace, 0.0f, csg_fDisplayListGridMin*csg_fDisplayListGridSpace);
		glVertex3f(((float)i)*csg_fDisplayListGridSpace, 0.0f, csg_fDisplayListGridMax*csg_fDisplayListGridSpace);
		glVertex3f(csg_fDisplayListGridMin*csg_fDisplayListGridSpace, 0.0f, ((float)i)*csg_fDisplayListGridSpace);
		glVertex3f(csg_fDisplayListGridMax*csg_fDisplayListGridSpace, 0.0f, ((float)i)*csg_fDisplayListGridSpace);
	}
	glEnd(); // end line drawing

	glPopAttrib(); // pop attrib marker (undo switching off lighting)

	glEndList(); // finish recording the displaylist
}

void printPause()
{
	if (g_Status.m_bIsSimulationRunning) {
		printf("Simulation running\n");
	}
	else {
		printf("Simulation paused\n");
	}
}