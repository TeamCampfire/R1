


#include "Vehicle/vehicletestWheelRear.h"

UvehicletestWheelRear::UvehicletestWheelRear()
{
	AxleType = EAxleType::Rear;
	bAffectedByHandbrake = true;
	bAffectedByEngine = true;
	FrictionForceMultiplier = 2.0f;
	WheelRadius = 55.f;
}
