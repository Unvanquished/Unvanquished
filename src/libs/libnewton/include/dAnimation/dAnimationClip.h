#ifndef _dAnimationClip_
#define _dAnimationClip_

#include <dList.h>
#include <dVector.h>
#include <dQuaternion.h>
#include <dRefCounter.h>


class TiXmlElement;
class dPoseGenerator;

class dKeyFrames
{
    public:
    dKeyFrames();
    ~dKeyFrames();

	void AllocaFrames (int count);

	int FindKey(float entry) const;

    char m_bindName[32];
    int m_keyFrameCounts;
    int* m_keys;
    dVector* m_posit;
    dQuaternion* m_rotation;
};

class dAnimationClip: public dList<dKeyFrames>, virtual public dRefCounter
{
	public:
	dAnimationClip(void);
	
	void Load (const char* fileName);
	void Save (const char* fileName) const;
	void RemoveNode (const char* fileName);

	int GetFramesCount() const;
	void SetFramesCount(int count);

	protected:
	~dAnimationClip(void);

	int m_framesCount;
	friend class dPoseGenerator;
};


#endif