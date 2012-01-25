#include <dAnimationStdAfx.h>
#include <dBone.h>
#include <dMesh.h>
#include <dModel.h>
#include <dAnimationClip.h>
#include <tinyxml.h>



dMesh* dLoaderContext::CreateMesh (int type)
{
	return new dMesh (dMesh::dMeshType (type));
}

dBone* dLoaderContext::CreateBone (dBone* parent)
{
	return new dBone (parent);
}

void dLoaderContext::LoaderFixup (dModel* model)
{
}


dModel::dModel(void)
{
}

dModel::~dModel(void)
{
//	for (dMeshList::dListNode* node = m_meshList.GetFirst(); node; node = node->GetNext()) {
	for (ModelComponentList<dList<dMesh*> >::dListNode* list = m_meshList.GetFirst(); list; list = list->GetNext()) {
		for (dList<dMesh*>::dListNode* node = list->GetInfo().m_data.GetFirst(); node; node = node->GetNext()) { 
			node->GetInfo()->Release();
		}
	}

//	for (dSkeleton::dListNode* node = m_skeleton.GetFirst(); node; node = node->GetNext()) {
	for (ModelComponentList<dList<dBone*> >::dListNode* list = m_skeleton.GetFirst(); list; list = list->GetNext()) {
		for (dList<dBone*>::dListNode* node = list->GetInfo().m_data.GetFirst(); node; node = node->GetNext()) { 
			delete node->GetInfo();
		}
	}

	for (ModelComponentList<dAnimationClip*>::dListNode* node = m_animations.GetFirst(); node; node = node->GetNext()) {
		node->GetInfo().m_data->Release();
	}

}

void dModel::AddMesh(dMesh* mesh)
{	
	if (!m_meshList.GetCount()) {
		m_meshList.Append();
	}
	ModelComponentList<dList<dMesh*> >::dListNode* list = m_meshList.GetFirst();
	list->GetInfo().m_data.Append (mesh);
	mesh->AddRef();
}


void dModel::RemoveMesh(dMesh* mesh)
{
//	for (dMeshList::dListNode* node = m_meshList.GetFirst(); node; node = node->GetNext()) {
	for (ModelComponentList<dList<dMesh*> >::dListNode* list = m_meshList.GetFirst(); list; list = list->GetNext()) {
		for (dList<dMesh*>::dListNode* node = list->GetInfo().m_data.GetFirst(); node; node = node->GetNext()) { 
			if (mesh == node->GetInfo()) {
				mesh->Release();
				list->GetInfo().m_data.Remove(node);
				return;
			}
		}
	}
}

void dModel::AddAnimation(const char* name, dAnimationClip* anim)
{
	ModelComponent<dAnimationClip*>& clip = m_animations.Append()->GetInfo();
	strncpy (clip.m_name, name, MODEL_COMPONENT_NAME_SIZE - 1);
	clip.m_data = anim;
	anim->AddRef();
}

void dModel::RemoveAnimation(dAnimationClip* anim)
{
	for (ModelComponentList<dAnimationClip*>::dListNode* node = m_animations.GetFirst(); node; node = node->GetNext()) {
		if (anim == node->GetInfo().m_data) {
			node->GetInfo().m_data->Release();
			m_animations.Remove(node);
			break;
		}
	}
}

dAnimationClip* dModel::FindAnimationClip (const char* name) const
{
	_ASSERTE (0);
	return NULL;
}


void dModel::BindMeshToBonesByName () const
{	
//	for (dMeshList::dListNode* node = m_meshList.GetFirst(); node; node = node->GetNext()) {
	for (ModelComponentList<dList<dMesh*> >::dListNode* list = m_meshList.GetFirst(); list; list = list->GetNext()) {
		for (dList<dMesh*>::dListNode* node = list->GetInfo().m_data.GetFirst(); node; node = node->GetNext()) { 
			dMesh* mesh;
			dBone* bone;
			mesh = node->GetInfo();
			bone = FindBone (mesh->m_name);
			if (bone) {
				mesh->m_boneID = bone->m_boneID;
			} else {
				mesh->m_boneID = 0;
			}
		}
	}
}


void dModel::UpdateMatrixPalette (const dMatrix& parentMatrix, dMatrix* const matrixOut, int maxCount) const
{
//	for (dSkeleton::dListNode* node = m_skeleton.GetFirst(); node; node = node->GetNext()) {
	for (ModelComponentList<dList<dBone*> >::dListNode* list = m_skeleton.GetFirst(); list; list = list->GetNext()) {
		for (dList<dBone*>::dListNode* node = list->GetInfo().m_data.GetFirst(); node; node = node->GetNext()) { 
			node->GetInfo()->UpdateMatrixPalette (parentMatrix, matrixOut, maxCount);
		}
	}

}

void dModel::IntsToString (char* string, const int* ints, int count)
{
	for (int i =0; i < (count - 1); i ++) {
		sprintf (string, "%d ", ints[i]);
		string += strlen (string);
	}
	sprintf (string, "%d", ints[count - 1]);

}

void dModel::FloatsToString (char* string, const dFloat* floats, int count)
{
	for (int i =0; i < (count - 1); i ++) {
		sprintf (string, "%f ", floats[i]);
		string += strlen (string);
	}
	sprintf (string, "%f", floats[count - 1]);
}

int dModel::StringToInts (const char* string, int* ints)
{
	int count;
	count = 0;

	do {
		ints[count] = atoi (string);
		count ++;
		while (string[0] && !isspace(string[0])) string ++;
		while (string[0] && isspace(string[0])) string ++;
	} while (string[0]);

	return count;
}

int dModel::StringToFloats (const char* string, dFloat* floats)
{
	int count;
	count = 0;

	do {
		floats[count] = dFloat (atof (string));
		count ++;
		while (string[0] && !isspace(string[0])) string ++;
		while (string[0] && isspace(string[0])) string ++;
	} while (string[0]);

	return count;
}

void dModel::GetFileName (const char* pathNamenode, char* name)
{
	for (int i = int (strlen (pathNamenode)); i >= 0; i --) {
		if ((pathNamenode[i] == '/') || (pathNamenode[i] == '\\')) {
			strcpy (name, &pathNamenode[i + 1]); 
			return ;
		}
	}
	strcpy (name, pathNamenode);
}

void dModel::GetFilePath (const char* pathNamenode, char* name)
{
	for (int i = int (strlen (pathNamenode)); i >= 0; i --) {
		if ((pathNamenode[i] == '/') || (pathNamenode[i] == '\\')) {
			strcpy (name, pathNamenode); 
			name[i + 1] = 0;
			return ;
		}
	}
	name[0] = 0;
}


static int SortVertexArray (const void *A, const void *B) 
{
	const dFloat* vertexA = (dFloat*) A;
	const dFloat* vertexB = (dFloat*) B;

	if (vertexA[0] < vertexB[0]) {
		return -1;
	} else if (vertexA[0] > vertexB[0]) {
		return 1;
	} else {
		return 0;
	}
}


int dModel::PackVertexArray (dFloat* vertexList, int compareElements, int strideInBytes, int vertexCount, int* indexListOut)
{
	int stride;
	int indexCount;
	dFloat errorTol;

	stride = strideInBytes / sizeof (dFloat);
	errorTol = dFloat (1.0e-4f);

	dFloat* array;
	array = new dFloat[(stride + 2) * vertexCount];

	for (int i = 0; i < vertexCount; i ++) {
		memcpy (&array[i * (stride + 2)], &vertexList[i * stride], strideInBytes);
		array[i * (stride + 2) + stride + 0] = dFloat(i);
		array[i * (stride + 2) + stride + 1] = 0.0f;
	}

	qsort(array, vertexCount, (stride + 2) * sizeof (dFloat), SortVertexArray);
	indexCount = 0;
	for (int i = 0; i < vertexCount; i ++) {
		int index;
		index = i * (stride + 2);
		if (array[index + stride + 1] == 0.0f) {
			dFloat window;
			window = array[index] + errorTol; 
			for (int j = i + 1; j < vertexCount; j ++) {
				int index2;
				index2 = j * (stride + 2);
				if (array[index2] >= window) {
					break;
				}
				if (array[index2 + stride + 1] == 0.0f) {
					int k;
					for (k = 0; k < compareElements; k ++) {
						dFloat error;
						error = array[index + k] - array[index2+ k];
						if (dAbs (error) >= errorTol) {
							break;
						}
					}
					if (k >= compareElements) {
						int m;
						m = int (array[index2 + stride]);
						memcpy (&array[indexCount * (stride + 2)], &array[index2], sizeof (dFloat) * stride);
						indexListOut[m] = indexCount;
						array[index2 + stride + 1] = 1.0f;
					}
				}
			}
			int m;
			m = int (array[index + stride]);
			memcpy (&array[indexCount * (stride + 2)], &array[index], sizeof (dFloat) * stride);
			indexListOut[m] = indexCount;
			array[indexCount * (stride + 2) + stride + 1] = 1.0f;
			indexCount ++;
		}
	}

	for (int i = 0; i < indexCount; i ++) {
		memcpy (&vertexList[i * stride], &array[i * (stride + 2)], sizeof (dFloat) * stride);
	}

	delete[] array;
	return indexCount;
}

dBone* dModel::FindBone (int index) const
{
//	for (dSkeleton::dListNode* node = m_skeleton.GetFirst(); node; node = node->GetNext()) {
	for (ModelComponentList<dList<dBone*> >::dListNode* list = m_skeleton.GetFirst(); list; list = list->GetNext()) {
		for (dList<dBone*>::dListNode* node = list->GetInfo().m_data.GetFirst(); node; node = node->GetNext()) { 
			for (dBone* bone = node->GetInfo()->GetFirst(); bone; bone = bone->GetNext()) {
				if (bone->GetBoneID() == index) {
					return bone;
				}
			}
		}
	}

	return NULL;
}

dBone* dModel::FindBone (const char* name) const
{
	for (ModelComponentList<dList<dBone*> >::dListNode* list = m_skeleton.GetFirst(); list; list = list->GetNext()) {
		for (dList<dBone*>::dListNode* node = list->GetInfo().m_data.GetFirst(); node; node = node->GetNext()) { 
			dBone* bone;
			bone = node->GetInfo()->Find(name);
			if (bone) {
				return bone;
			}
		}
	}
	return NULL;
}

dMesh* dModel::FindMesh (const char* name) const
{
	for (ModelComponentList<dList<dMesh*> >::dListNode* list = m_meshList.GetFirst(); list; list = list->GetNext()) {
		for (dList<dMesh*>::dListNode* node = list->GetInfo().m_data.GetFirst(); node; node = node->GetNext()) { 
			if (!strcmp (node->GetInfo()->m_name, name)) {
				return node->GetInfo();
			}
		}
	}
	return NULL;
}


void dModel::Save (const char* modelName, bool exportSkeleton, bool exportMesh, bool exportAnimations)
{
	TiXmlText* header;
	TiXmlElement *root;
	TiXmlDeclaration* decl;

	char mdlName[256];
	strcpy (mdlName, modelName);
	*strrchr (mdlName, '.') = 0;
	strcat (mdlName, ".mdl");

	TiXmlDocument out (mdlName);
	decl = new TiXmlDeclaration( "1.0", "", "" );
	out.LinkEndChild( decl );

	root = new TiXmlElement( "root" );
	out.LinkEndChild(root);

	header = new TiXmlText (XML_HEADER);
	root->LinkEndChild(header);


	for (ModelComponentList<dList<dMesh*> >::dListNode* list = m_meshList.GetFirst(); list; list = list->GetNext()) {
		char name[256];
		TiXmlElement *mesh;
		strcpy (name, list->GetInfo().m_name);

		mesh = new TiXmlElement( "mesh" );
		root->LinkEndChild(mesh);

		header = new TiXmlText (list->GetInfo().m_name);
		mesh->LinkEndChild(header);
	}

	for (ModelComponentList<dList<dBone*> >::dListNode* list = m_skeleton.GetFirst(); list; list = list->GetNext()) {
		TiXmlElement *skeleton;
		skeleton = new TiXmlElement( "skeleton" );
		root->LinkEndChild(skeleton);

		header = new TiXmlText (list->GetInfo().m_name);
		skeleton->LinkEndChild(header);
	}

	for (ModelComponentList<dAnimationClip*>::dListNode* node = m_animations.GetFirst(); node; node = node->GetNext()) {
		TiXmlElement *anim;
		anim = new TiXmlElement( "animation" );
		root->LinkEndChild(anim);
		header = new TiXmlText (node->GetInfo().m_name);
		anim->LinkEndChild(header);
	}


	if (exportSkeleton && m_skeleton.GetCount()) {
		char pathName[256];
		dModel::GetFilePath (modelName, pathName);
		for (ModelComponentList<dList<dBone*> >::dListNode* list = m_skeleton.GetFirst(); list; list = list->GetNext()) {
			char name[256];
			strcpy (name, pathName);
			strcat (name, list->GetInfo().m_name);
			dBone::Save (name, list->GetInfo().m_data);
		}
	}

	if (exportMesh && m_meshList.GetCount()) {

		char pathName[256];
		dModel::GetFilePath (modelName, pathName);
		for (ModelComponentList<dList<dMesh*> >::dListNode* list = m_meshList.GetFirst(); list; list = list->GetNext()) {
			char name[256];
			strcpy (name, pathName);
			strcat (name, list->GetInfo().m_name);
			dMesh::Save (name, list->GetInfo().m_data);
		}
	}

	if (exportAnimations && m_animations.GetCount()) {

		char pathName[256];
		dModel::GetFilePath (modelName, pathName);
		for (ModelComponentList<dAnimationClip*>::dListNode* node = m_animations.GetFirst(); node; node = node->GetNext()) {
			char name[256];
			strcpy (name, pathName);
			strcat (name, node->GetInfo().m_name);
			node->GetInfo().m_data->Save (name);
		}

	}


	out.SaveFile (mdlName);
}



void dModel::Load (const char* name, dLoaderContext& context)
{
	const TiXmlElement* root;
	TiXmlDocument doc (name);
	doc.LoadFile();

	char path[256];
	char* tmpName;
	strcpy (path, name);
	tmpName = strrchr (path, '/');
	if (!tmpName) {
		tmpName = strrchr (path, '\\');
	} 
	if (tmpName) {
		tmpName[0] = 0;
	}

	root = doc.RootElement();
	
	for (TiXmlElement* skeleton = (TiXmlElement*)root->FirstChildElement ("skeleton"); skeleton; skeleton = (TiXmlElement*)root->NextSibling ("skeleton")) {
		char pathName[256];
		ModelComponent<dList<dBone*> >& data = m_skeleton.Append()->GetInfo();
		strncpy (data.m_name, skeleton->GetText(), sizeof (data.m_name) - 1);

		strcpy (pathName, path);
		strcat (pathName, "/");
		strcat (pathName, data.m_name);
		dBone::Load (pathName, data.m_data, context);
	}

	for (TiXmlElement* meshes = (TiXmlElement*)root->FirstChildElement ("mesh"); meshes; meshes = (TiXmlElement*)root->NextSibling ("mesh")) {
		char pathName[256];
		ModelComponent<dList<dMesh*> >& data = m_meshList.Append()->GetInfo();

		strncpy (data.m_name, meshes->GetText(), sizeof (data.m_name) - 1);
		strcpy (pathName, path);
		strcat (pathName, "/");
		strcat (pathName, data.m_name);
		dMesh::Load (pathName, data.m_data, context);
	}

	for (ModelComponentList<dList<dMesh*> >::dListNode* list = m_meshList.GetFirst(); list; list = list->GetNext()) {
		for (dList<dMesh*>::dListNode* node = list->GetInfo().m_data.GetFirst(); node; node = node->GetNext()) { 
			if (node->GetInfo()->GetType() == dMesh::D_SKIN_MESH) {
				node->GetInfo()->m_weighList->SetBindingPose(node->GetInfo(), *this); 
			}
		}
	}

	for (TiXmlElement* anim = (TiXmlElement*)root->FirstChildElement("animation"); anim; anim = (TiXmlElement*)anim->NextSibling("animation")) {
		char pathName[256];
		ModelComponent<dAnimationClip*>& data = m_animations.Append()->GetInfo();

		strncpy (data.m_name, anim->GetText(), sizeof (data.m_name) - 1);
		strcpy (pathName, path);
		strcat (pathName, "/");
		strcat (pathName, data.m_name);
		
		data.m_data = new dAnimationClip ();
		data.m_data->Load (pathName);
	}

	context.LoaderFixup (this);
}

