//********************************************************************
// Newton Game dynamics 
// copyright 2000-2004
// By Julio Jerez
// VC: 6.0
// simple demo list vector class with iterators
//********************************************************************

// NewtonCustomJoint.h: interface for the NewtonCustomJoint class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_NEWTONCUSTOMJOINT_H__47435A34_E604_4336_A471_2179B7F60721__INCLUDED_)
#define AFX_NEWTONCUSTOMJOINT_H__47435A34_E604_4336_A471_2179B7F60721__INCLUDED_

#include "CustomJointLibraryStdAfx.h"
#include "dMathDefines.h"
#include "dVector.h"
#include "dMatrix.h"
#include "dQuaternion.h"


#ifdef _MSC_VER
	#ifdef _DEBUG
	#include <stdio.h>
		inline void dExpandTraceMessage (const char *fmt, ...)
		{
			va_list v_args;
			char text[1024];

			text[0] = 0;
			va_start (v_args, fmt);     
			vsprintf(text, fmt, v_args);
			va_end (v_args);            

			OutputDebugStringA (text);
		}

		#define dTrace(x)										\
		{														\
			dExpandTraceMessage x;								\
		}																	
	#else
		#define dTrace(x)
	#endif
#endif



#ifdef _MSC_VER 
#pragma warning (disable: 4251) //class 'dMatrix' needs to have dll-interface to be used by clients of class 'CustomWormGear'
#endif

// this is the base class to implement custom joints, it is not a joint it just provide functionality
// for the user to implement it own joints
class JOINTLIBRARY_API NewtonCustomJoint  
{
	public:
	NewtonCustomJoint();
	NewtonCustomJoint(int maxDOF, const NewtonBody* body0, const NewtonBody* body1);
	virtual ~NewtonCustomJoint();


	void *operator new (size_t size);
	void operator delete (void *ptr);


	void SetBodiesCollisionState (int state);
	int GetBodiesCollisionState () const;

	const NewtonBody* GetBody0 () const;
	const NewtonBody* GetBody1 () const;
	const NewtonJoint* GetJoint () const;

	int GetJointID () const;
	void SetJointID (int rttI);

	// the application needs to implement this function for serialization
	virtual void GetInfo (NewtonJointRecord* info) const;

	protected:
	void Init (int maxDOF, const NewtonBody* body0, const NewtonBody* body1);

	// the application needs to implement this function for each derived joint. See examples for more detail
	virtual void SubmitConstrainst (dFloat timestep, int threadIndex) = 0;

	void CalculateGlobalMatrix (const dMatrix& localMatrix0, const dMatrix& localMatrix1, dMatrix& matrix0, dMatrix& matrix1) const;
//	void CalculateLocalMatrix (const dVector& pivot, const dVector& dir, dMatrix& localMatrix0, dMatrix& localMatrix1) const;
	void CalculateLocalMatrix (const dMatrix& pinsAndPivotFrame, dMatrix& localMatrix0, dMatrix& localMatrix1) const;

	int m_rtti;
	int m_maxDox;
	const NewtonBody* m_body0;
	const NewtonBody* m_body1;
	NewtonJoint* m_joint;
	NewtonWorld* m_world;

	private:
	// this are the callback needed to have transparent c++ method interfaces 
	static void Destructor (const NewtonJoint* me);	
	static void SubmitConstrainst (const NewtonJoint* me, dFloat timestep, int threadIndex);
	static void GetInfo (const NewtonJoint* me, NewtonJointRecord* info);
};



#endif 
