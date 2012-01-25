#include "dAnimationStdAfx.h"
#include "dPoseGenerator.h"
#include "dAnimationClip.h"
#include "dBonesToPoseBinding.h"


dBonesToPoseBinding::dBonesToPoseBinding(dPoseGenerator* pose)
{
	m_pose = NULL;
	m_updateCallback = NULL;
	SetPoseGenerator(pose);
}

dBonesToPoseBinding::dBonesToPoseBinding(dAnimationClip* clip)
{
	dPoseGenerator* pose;

	m_pose = NULL;
	m_updateCallback = NULL;

	pose = new dPoseGenerator(clip);
	SetPoseGenerator(pose);
	pose->Release(); 
}

dBonesToPoseBinding::dBonesToPoseBinding(const char* clipFileName)
{
	dPoseGenerator* pose;

	m_pose = NULL;
	m_updateCallback = NULL;

	pose = new dPoseGenerator (clipFileName);
	SetPoseGenerator(pose);
	pose->Release(); 
}

dBonesToPoseBinding::~dBonesToPoseBinding(void)
{
	SetPoseGenerator (NULL);
}

dPoseGenerator* dBonesToPoseBinding::GetPoseGenerator () const
{
	return m_pose;
}

void dBonesToPoseBinding::SetPoseGenerator(dPoseGenerator* pose)
{
	if (m_pose) {
		m_pose->Release();
		RemoveAll();
	}
	
	m_pose = pose;
	if (m_pose) {
		pose->AddRef();
		for (dPoseGenerator::dListNode* node = m_pose->GetFirst(); node; node = node->GetNext()) {
			dBindFrameToNode& poseBind = Append()->GetInfo();
			poseBind.m_userData = NULL;
			poseBind.m_sourceTranform = &node->GetInfo();
		}
	}
}


void dBonesToPoseBinding::SetUpdateCallback (PosetUpdateCallback callback)
{
	m_updateCallback = callback;
}


void dBonesToPoseBinding::GeneratePose (dFloat param)
{
	m_pose->Generate (param);
}


void dBonesToPoseBinding::UpdatePose() const
{
	if (m_updateCallback) {
		for (dListNode* node = GetFirst(); node; node = node->GetNext()) {
			dBindFrameToNode& bindFrame = node->GetInfo();
			m_updateCallback (bindFrame.m_userData, &bindFrame.m_sourceTranform->m_posit[0], &bindFrame.m_sourceTranform->m_rotation.m_q0); 
//break;
		}
	}
}