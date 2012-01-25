#ifndef _dPoseGenerator_
#define _dPoseGenerator_

#include <dList.h>
#include <dVector.h>
#include <dQuaternion.h>
#include <dRefCounter.h>

class dKeyFrames;
class dAnimationClip;
class dPoseGeneratorBind;


class dPoseTransform
{
    public:
    dQuaternion m_rotation;
    dFloat m_posit[3];
    dKeyFrames* m_source;
};

class dPoseGenerator: public dList<dPoseTransform>, virtual public dRefCounter
{
    public:
    dPoseGenerator(dAnimationClip* clip);
	dPoseGenerator(const char* clipFileNameName);

	dAnimationClip* GetAnimationClip() const;
	void SetAnimationClip(dAnimationClip* clip);
    void Generate (dFloat param);

    protected:
    ~dPoseGenerator(void);

    dAnimationClip* m_clip;
    friend class dPoseGeneratorBind;
};


#endif