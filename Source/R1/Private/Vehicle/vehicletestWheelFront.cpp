


#include "Vehicle/vehicletestWheelFront.h"

UvehicletestWheelFront::UvehicletestWheelFront()
{
	AxleType = EAxleType::Front;
	bAffectedBySteering = true;
	MaxSteerAngle = 40.f;

	WheelRadius = 55.f;
}
