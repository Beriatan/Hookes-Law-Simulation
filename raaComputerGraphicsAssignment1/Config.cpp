#include "Config.h"

raaConfig g_Config;
raaStatus g_Status;

void initConfig() {
	g_Config.m_fDampingFactor = 0.97f;
	g_Config.m_fSimulationSpeedMultiplier = 1.0f;
	g_Config.m_fBaseDampingFactor = 0.97f;
	g_Config.m_fDefaultSimulationSpeed = 0.8f;
	g_Config.m_fKineticEnergyThreshold = 40000000000000.0f;
	g_Config.m_fNodeSpacingInCube = 150.0f;
	g_Config.m_fNodeSpacingInLine = 50.0f;
	g_Config.m_fArcLength = 200.0f;
	g_Config.m_fMaxArcLength = 600.0f;

}

void initStatus() {
	g_Status.m_bIsSimulationRunning = false;
	g_Status.m_fDeltaTime = 0.0f;
	g_Status.m_fSimulationKineticEnergy = 0.0f;
}