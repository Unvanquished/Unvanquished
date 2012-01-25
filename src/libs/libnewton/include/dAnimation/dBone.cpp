#include "dAnimationStdAfx.h"
#include "dBone.h"
#include "dModel.h"
#include "tinyxml.h"

dBone::dBone(dBone* parent)
	:dHierarchy<dBone>(), m_localMatrix(GetIdentityMatrix())
{
	if (parent) {
		Attach (parent);
	}

	m_boneID = -1;
}

dBone::~dBone(void)
{
}

int dBone::GetBoneID() const
{
	return m_boneID;
}

const dMatrix& dBone::GetMatrix () const
{
	return m_localMatrix;
}

void dBone::SetMatrix (const dMatrix& matrix)
{
	m_localMatrix = matrix;
}

dMatrix dBone::CalcGlobalMatrix (const dBone* root) const
{
	dBone* ptr;
	
	if (root != this) {
		dMatrix matrix (m_localMatrix);
		for (ptr = GetParent(); ptr != root; ptr = ptr->GetParent()) {
			matrix = matrix * ptr->m_localMatrix;
		}
		return matrix;
	} else {
		return GetIdentityMatrix();
	}
}


void dBone::Save(const char* fileName, const dList<dBone*>& list)
{
	TiXmlText* header;
	TiXmlElement *root;
	
	TiXmlDeclaration* decl;

	TiXmlDocument out (fileName);
	decl = new TiXmlDeclaration( "1.0", "", "" );
	out.LinkEndChild( decl );

	root = new TiXmlElement( "root" );
	out.LinkEndChild(root);

	header = new TiXmlText (XML_HEADER);
	root->LinkEndChild(header);
	for (dList<dBone*>::dListNode* node = list.GetFirst(); node; node = node->GetNext()) {
		int stack;
		TiXmlElement *skeleton;
		const dBone* nodeArray[1024];

		skeleton = new TiXmlElement( "skeleton" );

		stack = 1;
		nodeArray[0] = node->GetInfo();
		while (stack) {
			const char* name;
			
			const dBone* node;
			TiXmlElement* xmlNode;

			stack --;
			node = nodeArray[stack];

			name = node->GetName();

			xmlNode = new TiXmlElement( "bone" );
			skeleton->LinkEndChild(xmlNode);

			xmlNode->SetAttribute("name", name);
			if (node->GetParent()) {
				const char* parentName;
				parentName = node->GetParent()->GetName();
				xmlNode->SetAttribute("parent", parentName);
			}
			xmlNode->SetAttribute("boneID", node->GetBoneID());

			char matrixBuffer[512];
			dModel::FloatsToString (matrixBuffer, &node->m_localMatrix[0][0], 16);
			xmlNode->SetAttribute("matrix", matrixBuffer);
			for (node = node->GetChild(); node; node = node->GetSibling()) {
				nodeArray[stack] = node;
				stack ++;
			}
		}
		root->LinkEndChild(skeleton);
	}

	out.SaveFile (fileName);
}


void dBone::Load(const char* fileName, dList<dBone*>& list, dLoaderContext& context)
{
	const TiXmlElement* root;
	TiXmlDocument doc (fileName);
	doc.LoadFile();

	root = doc.RootElement();
	if (root && !strcmp (root->GetText (), "newton 2.0 file format")){
		for (const TiXmlElement* skeleton = (TiXmlElement*)root->FirstChild("skeleton"); skeleton; skeleton = (TiXmlElement*)skeleton->NextSibling()) {
			dBone* rootBone;

			rootBone = NULL;
			for (const TiXmlElement* node = (TiXmlElement*)skeleton->FirstChild("bone"); node; node = (TiXmlElement*)node->NextSibling()) {
				dBone* bone;
				const char* name;
				const char* parent;

				name = node->Attribute ("name");
				parent = node->Attribute ("parent");

				if (parent) {
					dBone* parentBone;
					parentBone = rootBone->Find (parent);
					_ASSERTE (parentBone);
					bone = context.CreateBone (parentBone);
				} else {
					bone = context.CreateBone (NULL);
					rootBone = bone;
				}

				dMatrix matrix;
				bone->SetNameID(name);
				node->Attribute ("boneID", &bone->m_boneID);
				dModel::StringToFloats (node->Attribute("matrix"), &matrix[0][0]);
				bone->SetMatrix (matrix);
			} 

			list.Append(rootBone);
		}
	}
}

void dBone::UpdateMatrixPalette (const dMatrix& parentMatrix, dMatrix* const matrixOut, int maxCount) const
{
	int stack;
	const dBone* nodeArray[1024];

	_ASSERTE (m_boneID >= 0);
	stack = 0;
	matrixOut[m_boneID] = m_localMatrix * parentMatrix;
	for (const dBone* node = GetChild(); node; node = node->GetSibling()) {
		nodeArray[stack] = node;
		stack ++;
	}

	while (stack) {
		const dBone* bone;
		const dBone* parent;

		stack --;
		bone = nodeArray[stack];

		parent = bone->GetParent();
		matrixOut[bone->m_boneID] = bone->m_localMatrix * matrixOut[parent->m_boneID];

		maxCount --;
		_ASSERTE (maxCount);
		for (const dBone* node = bone->GetChild(); node; node = node->GetSibling()) {
			nodeArray[stack] = node;
			stack ++;
		}
	} 
}

