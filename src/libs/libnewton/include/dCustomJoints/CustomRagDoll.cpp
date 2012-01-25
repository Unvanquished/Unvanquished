#include "CustomJointLibraryStdAfx.h"

#include "CustomRagDoll.h"

#define RAGDOLL_JOINT_ID	    0x45fEAD84



#include "CustomBallAndSocket.h"


class CustomRagDollLimbJoint: public CustomLimitBallAndSocket
{
	public:
	CustomRagDollLimbJoint (
		const dMatrix& pinAndPivotFrame, 
		const NewtonBody* child, 
		const NewtonBody* parent)
		:CustomLimitBallAndSocket(pinAndPivotFrame, child, parent)
	{
	}

	virtual void SubmitConstrainst (dFloat timestep, int threadIndex)
	{
		CustomLimitBallAndSocket::SubmitConstrainst (timestep, threadIndex);
	}
};







CustomRagDoll::CustomRagDoll ()
	:NewtonCustomJoint()
{
	m_bonesCount = 0;
	m_rtti = RAGDOLL_JOINT_ID;

	m_joints[0] = NULL;
	m_bonesBodies[0] = NULL;
	m_bonesParents[0] = NULL;
}



CustomRagDoll::~CustomRagDoll(void)
{
}

int CustomRagDoll::AddBone (
	NewtonWorld* world,
	int parentBoneIndex, 
	void *userData, 
	const dMatrix& boneMatrix, 
	dFloat mass, 
	const dMatrix& pivotInGlobalSpace, 
	const NewtonCollision* boneCollisionShape)
{
	dFloat Ixx;
	dFloat Iyy;
	dFloat Izz;
	const NewtonBody* bone;
	CustomRagDollLimbJoint *joint;

	// create the rigid body that will make this bone
	bone = NewtonCreateBody (world, boneCollisionShape);

	// calculate the moment of inertia and the relative center of mass of the solid
	dVector origin(0.0f, 0.0f, 0.0f, 1.0f);
	dVector inertia(0.0f, 0.0f, 0.0f, 1.0f);
	NewtonConvexCollisionCalculateInertialMatrix (boneCollisionShape, &inertia[0], &origin[0]);	

	// set the body centert of mass
	NewtonBodySetCentreOfMass (bone, &origin[0]);


	Ixx = mass * inertia[0];
	Iyy = mass * inertia[1];
	Izz = mass * inertia[2];

	// set the mass matrix
	NewtonBodySetMassMatrix (bone, mass, Ixx, Iyy, Izz);

	// set the body matrix
	NewtonBodySetMatrix (bone, &boneMatrix[0][0]);

	// save the user data with the bone body (usually the visual geometry)
	NewtonBodySetUserData(bone, userData);

	joint = NULL;
	if (m_bonesCount == 0) {
		// this is the root bone, initialize ragdoll with the root Bone
		Init (1, bone, NULL);
		m_rtti = RAGDOLL_JOINT_ID;
		NewtonBodySetTransformCallback(bone, TransformCallBack);
		parentBoneIndex = 0;
	} else {
		// this is a child bone, need to be connected to its parent by a joint 
		const NewtonBody* parent;
		parent = m_bonesBodies[parentBoneIndex];
		joint = new CustomRagDollLimbJoint (pivotInGlobalSpace, bone, parent);
	}

	m_joints[m_bonesCount] = joint;
	m_bonesBodies[m_bonesCount] = bone;
	m_bonesParents[m_bonesCount] = m_bonesBodies[parentBoneIndex];
	m_bonesCount ++;

	return m_bonesCount - 1;

}


void CustomRagDoll::GetInfo (NewtonJointRecord* info) const
{

}

void CustomRagDoll::SubmitConstrainst (dFloat timestep, int threadIndex)
{

}


const NewtonBody* CustomRagDoll::GetBone (int bodenIndex) const
{
	return m_bonesBodies[bodenIndex];
}

const NewtonBody* CustomRagDoll::GetParentBone (int bodenIndex) const
{
    return m_bonesParents[bodenIndex];
}

const NewtonCustomJoint *CustomRagDoll::GetJoint (int bodenIndex) const
{
	return m_joints[bodenIndex];
}

int CustomRagDoll::GetBoneCount () const
{
	return m_bonesCount;
}

void CustomRagDoll::SetBoneTwistLimits (int bodenIndex, dFloat minAngle, dFloat maxAngle)
{
	if (bodenIndex > 0) {
		m_joints[bodenIndex]->SetTwistAngle (minAngle, maxAngle);
	}

}


void CustomRagDoll::SetBoneConeLimits (int bodenIndex, dFloat angle)
{
	if (bodenIndex > 0) {
		m_joints[bodenIndex]->SetConeAngle (angle);
	}
}

void CustomRagDoll::SetCollisionState (int bodenIndex, int state)
{
	NewtonBodySetJointRecursiveCollision (m_bonesBodies[bodenIndex], state);
}


void CustomRagDoll::TransformCallBack(const NewtonBody* body, const dFloat* matrix, int threadIndex)
{
	NewtonJoint* joint;

	// find there ragdoll joint and call the transform call back for this joint
	for (joint = NewtonBodyGetFirstJoint(body); joint; joint = NewtonBodyGetNextJoint(body, joint)) {
		NewtonCustomJoint* customJoint;
		customJoint = (NewtonCustomJoint*) NewtonJointGetUserData(joint);
		if (customJoint->GetJointID() == RAGDOLL_JOINT_ID) {
			CustomRagDoll* ragDoll;
			ragDoll = (CustomRagDoll*)customJoint;
			_ASSERTE (body == ragDoll->m_bonesBodies[0]);
			ragDoll->CalculateLocalMatrices ();
			break;
		}
	}
}

void CustomRagDoll::CalculateLocalMatrices () const
{
	void* userData;
	dMatrix boneMatrix;

	userData = NewtonBodyGetUserData(m_bonesBodies[0]);
	NewtonBodyGetMatrix(m_bonesBodies[0], &boneMatrix[0][0]);
	ApplyBoneMatrix (0, userData, boneMatrix);

	for (int i = 1; i < m_bonesCount; i ++) {
		dMatrix parentMatrix;

		userData = NewtonBodyGetUserData(m_bonesBodies[i]);
		NewtonBodyGetMatrix(m_bonesBodies[i], &boneMatrix[0][0]);
		NewtonBodyGetMatrix(m_bonesParents[i], &parentMatrix[0][0]);
//		boneMatrix = m_joints[i]->m_boneOffetMatrix * boneMatrix * parentMatrix.Inverse();
		boneMatrix = boneMatrix * parentMatrix.Inverse();
		ApplyBoneMatrix (i, userData, boneMatrix);
	}
}


