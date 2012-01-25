#ifndef _dBonesToPoseBinding_
#define _dBonesToPoseBinding_

#include <dList.h>
#include <dVector.h>
#include <dRefCounter.h>

class dAnimationClip;
class dPoseGenerator;
class dPoseTransform;

typedef void (*PosetUpdateCallback) (void* useData, dFloat* posit, dFloat* rotation);

class dBindFrameToNode
{
    public:
    void* m_userData;
    dPoseTransform* m_sourceTranform;
};

class dBonesToPoseBinding: public dList<dBindFrameToNode>, virtual public dRefCounter
{
    public:
	dBonesToPoseBinding(dAnimationClip* clip);
    dBonesToPoseBinding(dPoseGenerator* pose);
	dBonesToPoseBinding(const char* clipFileName);

	dPoseGenerator* GetPoseGenerator () const;
	void SetPoseGenerator(dPoseGenerator* pose);

    void UpdatePose() const;
    void GeneratePose (dFloat param);
    void SetUpdateCallback (PosetUpdateCallback callback);
    
    protected:
    ~dBonesToPoseBinding(void);

    dPoseGenerator* m_pose;
    PosetUpdateCallback m_updateCallback;
};


#endif