#ifndef _D_SKELETON_H_
#define _D_SKELETON_H_

#include <dAnimationStdAfx.h>
#include <dBaseHierarchy.h>
#include <dMatrix.h>
#include <dModel.h>



class TiXmlElement;


class dBone: public dHierarchy<dBone>
{
	public:
	dBone(dBone* parent);
	~dBone(void);

	const dMatrix& GetMatrix () const;
	void SetMatrix (const dMatrix& matrix);
	dMatrix CalcGlobalMatrix (const dBone* root = NULL) const;

	void UpdateMatrixPalette (const dMatrix& parentMatrix, dMatrix* const matrixOut, int maxCount) const;

	int GetBoneID() const;
	static void Load(const char* fileName, dList<dBone*>& list, dLoaderContext& context);
	static void Save(const char* fileName, const dList<dBone*>& list);

	dMatrix m_localMatrix;
	int m_boneID;
};

#endif
