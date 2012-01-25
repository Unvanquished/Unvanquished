//********************************************************************
// Newton Game dynamics 
// copyright 2000-2004
// By Julio Jerez
// VC: 6.0
//********************************************************************

// CustomRagDoll.h: interface for the CustomRagDoll class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CUSTOM_RAGDOLL__INCLUDED_) 
#define AFX_CUSTOM_RAGDOLL__INCLUDED_

#include "NewtonCustomJoint.h"

#define MAX_BONES	128

class CustomRagDollLimbJoint;

class JOINTLIBRARY_API CustomRagDoll: public NewtonCustomJoint  
{
	public:
	CustomRagDoll();
	virtual ~CustomRagDoll(void);

	const NewtonBody* GetBone (int bodenIndex) const;
	const NewtonBody* GetParentBone (int bodenIndex) const;
	const NewtonCustomJoint  *GetJoint (int bodenIndex) const;
	int AddBone (NewtonWorld* world, int parentBoneIndex, void *userData, const dMatrix& boneMatrix, dFloat mass, const dMatrix& pivotInGlobalSpace, const NewtonCollision* collisionShape);

	int GetBoneCount () const;
	void SetBoneConeLimits (int bodenIndex, dFloat angle);
	void SetBoneTwistLimits (int bodenIndex, dFloat minAngle, dFloat maxAngle);

	void SetCollisionState (int bodenIndex, int state);


	protected:
	virtual void ApplyBoneMatrix (int boneIndex, void* userData, const dMatrix& matrix) const = 0;

	private:
	void CalculateLocalMatrices () const;
	static void TransformCallBack(const NewtonBody* body, const dFloat* matrix, int threadIndex);

	protected:
	virtual void SubmitConstrainst (dFloat timestep, int threadIndex);
	virtual void GetInfo (NewtonJointRecord* info) const;

	int m_bonesCount;
	const NewtonBody* m_bonesBodies[MAX_BONES];
	const NewtonBody* m_bonesParents[MAX_BONES];
	CustomRagDollLimbJoint* m_joints[MAX_BONES];
};


#endif