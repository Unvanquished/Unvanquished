#if !defined(AFX_CUSTOM_PLAYER_CONTROLLER_INCLUDED)
#define AFX_CUSTOM_PLAYER_CONTROLLER_INCLUDED


#include "NewtonCustomJoint.h"

class JOINTLIBRARY_API CustomPlayerController: public NewtonCustomJoint
{
	public:
	CustomPlayerController(const dVector& pin, const NewtonBody* child, dFloat maxStairStepFactor);
	virtual ~CustomPlayerController();

	
	void SetVelocity (dFloat forwardSpeed, dFloat sideSpeed, dFloat heading);
	void SetMaxSlope (dFloat maxSlopeAngleIndRadian);
	dFloat GetMaxSlope () const;

	virtual bool CanPushBody (const NewtonBody* hitBody) const {return true;}

	const NewtonCollision* GetVerticalSensorShape () const;
	const NewtonCollision* GetHorizontalSensorShape () const;
	const NewtonCollision* GetDynamicsSensorShape () const;
	

	protected:

	struct FilterData
	{	
		FilterData (const NewtonBody* me)
		{
			m_count = 1;
			m_filter[0] = me;
		}
		int m_count;
		const NewtonBody* m_filter[8];
	};
	virtual void SubmitConstrainst (dFloat timestep, int threadIndex);
	virtual void GetInfo (NewtonJointRecord* info) const;

	struct FindFloorData
	{
		FindFloorData (const NewtonBody* me)
			:m_normal (0.0f, 1.0f, 0.0f, 0.0f)
		{
			m_me = me;
			m_param = 2.0f;
			m_hitBody = NULL;
		}

		dVector m_normal;
		dFloat m_param;
		const NewtonBody* m_me; 
		const NewtonBody* m_hitBody; 
	};

	static dFloat FindFloorCallback(const NewtonBody* body, const dFloat* hitNormal, int collisionID, void* userData, dFloat intersetParam);
	static unsigned ConvexStaticCastPrefilter(const NewtonBody* body, const NewtonCollision* collision, void* userData);
	static unsigned ConvexDynamicCastPrefilter(const NewtonBody* body, const NewtonCollision* collision, void* userData);
	static unsigned ConvexAllBodyCastPrefilter(const NewtonBody* body, const NewtonCollision* collision, void* userData);

	dMatrix m_localMatrix0;
	dMatrix m_localMatrix1;
	dFloat m_maxSlope;
	dFloat m_heading;
	dFloat m_sideSpeed;
	dFloat m_forwardSpeed;
	dFloat m_stairHeight;
	dFloat m_playerHeight;
	dFloat m_restitution;

	int m_isInJumpState;
	NewtonCollision* m_verticalSensorShape;
	NewtonCollision* m_horizontalSensorShape;
	NewtonCollision* m_dynamicsCollisionShape;
};

#endif