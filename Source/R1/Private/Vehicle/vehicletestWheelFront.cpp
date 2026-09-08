


#include "Vehicle/vehicletestWheelFront.h"

UvehicletestWheelFront::UvehicletestWheelFront()
{
	AxleType = EAxleType::Front;
	bAffectedBySteering = true;
	MaxSteerAngle = 40.f;
	FrictionForceMultiplier = 2.0f;
	WheelRadius = 55.f;
}
