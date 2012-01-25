//********************************************************************
// Newton Game dynamics 
// copyright 2000-2004
// By Julio Jerez
// VC: 6.0
// basic Hierarchical Scene Node Class  
//********************************************************************

#ifndef _D_MODEL_H_
#define _D_MODEL_H_

#include <dAnimationStdAfx.h>
#include <dList.h>
#include <dBaseHierarchy.h>
#include <dMathDefines.h>

class dMesh;
class dBone;
class dModel;
class dMatrix;
class dAnimationClip;

#define XML_HEADER "newton 2.0 file format"


class dLoaderContext
{
	public:
	dLoaderContext() {}
	virtual ~dLoaderContext() {}
	virtual dMesh* CreateMesh (int type);
	virtual dBone* CreateBone (dBone* parent);
	virtual void LoaderFixup (dModel* model);
};

#define MODEL_COMPONENT_NAME_SIZE 32

template<class T>
class ModelComponent
{
	public:
	char m_name[MODEL_COMPONENT_NAME_SIZE];
	T m_data;
};

template<class T>
class ModelComponentList: public dList < ModelComponent<T> >
{
};

class dModel
{
	public:
	dModel();
	virtual ~dModel();

	void AddMesh(dMesh* mesh);
	void RemoveMesh(dMesh* mesh);

	void AddAnimation(const char *name, dAnimationClip* mesh);
	void RemoveAnimation(dAnimationClip* mesh);

	void UpdateMatrixPalette (const dMatrix& parentMatrix, dMatrix* const matrixOut, int maxCount) const;
	void BindMeshToBonesByName () const;

	dBone* FindBone (int index) const;
	dBone* FindBone (const char* name) const;
	dMesh* FindMesh (const char* name) const;
	dAnimationClip* FindAnimationClip (const char* name) const;

	static void GetFileName (const char* pathNamenode, char* name);
	static void GetFilePath (const char* pathNamenode, char* name);
	static int StringToInts (const char* string, int* ints);
	static int StringToFloats (const char* string, dFloat* floats);
	static void IntsToString (char* string, const int* ints, int count);
	static void FloatsToString (char* string, const dFloat* floats, int count);
	static int PackVertexArray (dFloat* vertexList, int compareElements, int strideInBytes, int vertexCount, int* indexListOut);

	virtual void Load (const char* name, dLoaderContext& context); 
	virtual void Save (const char* name, bool exportSkeleton = true, bool exportMesh = true, bool exportAnimations = true); 

	ModelComponentList<dList<dBone*> > m_skeleton;
	ModelComponentList<dList< dMesh*> > m_meshList;
	ModelComponentList<dAnimationClip*> m_animations;
	
};


#endif