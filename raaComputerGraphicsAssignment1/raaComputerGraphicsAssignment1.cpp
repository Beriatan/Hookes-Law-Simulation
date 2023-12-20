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

#include "raaCamera/raaCamera.h"
#include "raaUtilities/raaUtilities.h"
#include "raaMaths/raaMaths.h"
#include "raaMaths/raaVector.h"
#include "raaPajParser/raaPajParser.h"
#include "raaText/raaText.h"
#include "raaSystem/raaSystem.h"

#include "raaConstants.h"
#include "raaParse.h"
#include "raaControl.h"
#include "raaConstants.h"
#include "Config.h"
#include "Graphics.h"
#include "Physics.h"


// core system global data
raaCameraInput g_Input; // structure to hadle input to the camera comming from mouse/keyboard events
raaSystem g_System; // data structure holding the imported graph of data - you may need to modify and extend this to support your functionallity
raaControl g_Control; // set of flag controls used in my implmentation to retain state of key actions
raaCamera g_Camera; // structure holding the camera position and orientation attributes
extern raaConfig g_Config;
extern raaStatus g_Status;


LARGE_INTEGER frequency;
LARGE_INTEGER lastFrameTime;


// global var: parameter name for the file to load
const static char csg_acFileParam[] = { "-input" };
// global var: file to load data from
char g_acFile[256];

// core functions -> reduce to just the ones needed by glut as pointers to functions to fulfill tasks
void display(); // The rendering function. This is called once for each frame and you should put rendering code here
void idle(); // The idle function is called at least once per frame and is where all simulation and operational code should be placed
void reshape(int iWidth, int iHeight); // called each time the window is moved or resized
void keyboard(unsigned char c, int iXPos, int iYPos); // called for each keyboard press with a standard ascii key
void keyboardUp(unsigned char c, int iXPos, int iYPos); // called for each keyboard release with a standard ascii key
void sKeyboard(int iC, int iXPos, int iYPos); // called for each keyboard press with a non ascii key (eg shift)
void sKeyboardUp(int iC, int iXPos, int iYPos); // called for each keyboard release with a non ascii key (eg shift)
void mouse(int iKey, int iEvent, int iXPos, int iYPos); // called for each mouse key event
void motion(int iXPos, int iYPos); // called for each mouse motion event

// Non glut functions
void myInit(); // the myinit function runs once, before rendering starts and should be used for setup
void buildGrid(); // 


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
		accumulateAllSpringForces();

		// Apply accumulated forces to nodes and reset them
		applyAccumulatedSpringForcesToNodes();
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

void mouse(int iKey, int iEvent, int iXPos, int iYPos) {
	if (iKey == GLUT_LEFT_BUTTON) {
		if (iEvent == GLUT_DOWN) {
			// Select node on left button press
			selectNode(iXPos, iYPos, 20.0f);
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

	raaNode* selectedNode = findClosestIntersectedNode(g_fOrigin, g_fDirection, 20.0f);
	
	// Reset highlighting for all nodes
	for (raaLinkedListElement* pElement = head(&(g_System.m_llNodes)); pElement != NULL; pElement = pElement->m_pNext) {
		raaNode* pNode = static_cast<raaNode*>(pElement->m_pData);
		pNode->m_bHighlighted = false;
	}

	// Highlight the selected node
	if (selectedNode) {
		selectedNode->m_bHighlighted = true;
	}

}


void sortSubmenuAction(int option) {
	switch (option) {
	case 1:
		g_Status.m_bIsSimulationRunning = false;
		arrangeNodesInCuboid(); // Assuming 50 is the desired spacing
		updateCameraToCenterOfMass(&g_System, &g_Camera);
		break;
	case 2:
		g_Status.m_bIsSimulationRunning = true;
		randomizeNodePositions(&g_System, 0.0001f, 0.002f);
		g_Status.m_bIsSimulationRunning = false;
		arrangeNodesInLine();
		updateCameraToCenterOfMass(&g_System, &g_Camera);
		break;
	case 3:
		g_Status.m_bIsSimulationRunning = true;
		randomizeNodePositions(&g_System, 0.0001f, 0.002f);
		g_Status.m_bIsSimulationRunning = false;
		arrangeNodesInContinentLineUsingVectorClass();
		updateCameraToCenterOfMass(&g_System, &g_Camera);
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