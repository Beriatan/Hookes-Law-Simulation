#pragma once

typedef struct _raaConfig{
	float m_fSimulationSpeedMultiplier;
	float m_fDampingFactor;
	float m_fBaseDampingFactor;
	float m_fDefaultSimulationSpeed;
	float m_fKineticEnergyThreshold;
	float m_fNodeSpacingInCube;
	float m_fNodeSpacingInLine;
	float m_fArcLength;
	float m_fMaxArcLength;
} raaConfig;

typedef struct _raaStatus {
	bool m_bIsSimulationRunning;
	float m_fSimulationKineticEnergy;
	float m_fDeltaTime;
} raaStatus;

void initConfig();
void initStatus();