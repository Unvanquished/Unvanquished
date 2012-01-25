#include "dAnimationStdAfx.h"
#include "dPoseGenerator.h"
#include "dAnimationClip.h"

dPoseGenerator::dPoseGenerator(dAnimationClip* clip)
{
	m_clip = NULL;
	SetAnimationClip(clip);
}

dPoseGenerator::dPoseGenerator(const char* clipFileName)
{
	dAnimationClip* clip;
	m_clip = NULL;

	clip = new dAnimationClip();
	clip->Load (clipFileName);
	SetAnimationClip(clip);
	clip->Release();
}

dPoseGenerator::~dPoseGenerator(void)
{
//	m_clip->Release();
	SetAnimationClip(NULL);
}

dAnimationClip* dPoseGenerator::GetAnimationClip() const
{
	return m_clip; 
}

void dPoseGenerator::SetAnimationClip(dAnimationClip* clip)
{
	if (m_clip) {
		m_clip->Release();
		RemoveAll();
	}

	m_clip = clip;
	if (m_clip) {
		clip->AddRef();
		for (dAnimationClip::dListNode* node = m_clip->GetFirst(); node; node = node->GetNext()) {
			dPoseTransform& pose = Append()->GetInfo();
			pose.m_source = &node->GetInfo();
			pose.m_posit[0] = 0.0f;
			pose.m_posit[1] = 0.0f;
			pose.m_posit[2] = 0.0f;
			pose.m_rotation = dQuaternion();
		}
	}
}


void dPoseGenerator::Generate (dFloat param)
{
	// find the inetvale of the clip

	if (GetCount()) {
		dFloat frame;
		frame = dMod (param, 1.0000f);
		if (frame < 0.0f) {
			frame += 1.0f;
		}
		_ASSERTE (frame >= 0.0f);

		if (frame >= 1.0f) {
			frame = 0.99999f;
		}
		frame *= (m_clip->m_framesCount - 1);
		for (dListNode* node = GetFirst(); node; node = node->GetNext()) {
			dPoseTransform& transformPose = node->GetInfo();

			const dKeyFrames* const source = transformPose.m_source;

			
			dFloat t;
			int key0;
			int key1;
			key0 = source->FindKey(frame);
			_ASSERTE (key0 < (source->m_keyFrameCounts - 1));
			key1 = key0 + 1;
			t = (frame - source->m_keys[key0]) / (source->m_keys[key1] - source->m_keys[key0]);
			dVector posit (source->m_posit[key0] + (source->m_posit[key1] - source->m_posit[key0]).Scale (t));
			transformPose.m_rotation = source->m_rotation[key0].Slerp (source->m_rotation[key1], t);
			transformPose.m_posit[0] = posit.m_x;
			transformPose.m_posit[1] = posit.m_y;
			transformPose.m_posit[2] = posit.m_z;
		}
	}

}