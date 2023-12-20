#include "Config.h"

raaConfig g_Config;
raaStatus g_Status;

void initConfig() {
	g_Config.m_fDampingFactor = 0.97f;
	g_Config.m_fSimulationSpeedMultiplier = 1.0f;
	g_Config.m_fBaseDampingFactor = 0.97f;
	g_Config.m_fDefaultSimulationSpeed = 0.8f;

}

void initStatus() {
	g_Status.m_bIsSimulationRunning = false;
	g_Status.m_fDeltaTime = 0.0f;
	g_Status.m_fSimulationKineticEnergy = 0.0f;
}