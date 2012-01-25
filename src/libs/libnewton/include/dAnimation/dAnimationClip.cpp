#include <dAnimationStdAfx.h>
#include <dAnimationClip.h>
#include <dModel.h>
#include <tinyxml.h>


#if (_MSC_VER >= 1400)
#pragma warning (disable: 4996) // for 2005 users declared deprecated
#pragma warning (disable: 4100) // unreferenced formal parameter
#endif


int dKeyFrames::FindKey(float entry) const
{
	// do using a binary search 
	int low;
	int high;
	low = 0;
	high = m_keyFrameCounts - 1;
	while (low < high) {
		int mid;
		mid = ((low + high)>>1) + 1;
		if (m_keys[mid] < entry) {
			low = mid; 
		} else {
			high = mid - 1; 
		}
	}

#ifdef _DEBUG 
	// check the  search result
	for (int i = 0; i < (m_keyFrameCounts - 1); i ++) {
		if (m_keys[i + 1] > entry) {
			_ASSERTE (i == low);
			break;
		}
	}
#endif
	return low;
}

dAnimationClip::dAnimationClip(void)
{
	m_framesCount = 0;
}

dAnimationClip::~dAnimationClip(void)
{
}


int dAnimationClip::GetFramesCount() const
{
	return m_framesCount;
}

void dAnimationClip::SetFramesCount(int count)
{
	m_framesCount = count;
}


void dAnimationClip::RemoveNode (const char* name)
{
	for (dListNode* node = GetFirst(); node; node = node->GetNext()) {        
		if (!strcmp (node->GetInfo().m_bindName, name)) {
			Remove (node);
			break;
		}
	}
}


dKeyFrames::dKeyFrames()
{
	m_keys = NULL;
	m_posit = NULL;
	m_rotation = NULL;

}

dKeyFrames::~dKeyFrames()
{
	AllocaFrames (0);
}

void dKeyFrames::AllocaFrames (int count)
{
	if (m_keys) {
		delete[] m_keys;
		delete[] m_posit;
		delete[] m_rotation;

		m_keys = NULL;
		m_posit = NULL;;
		m_rotation = NULL;;
	}

	m_keyFrameCounts = count;
	if (m_keyFrameCounts) {
		m_keys = new int[m_keyFrameCounts];
		m_posit = new dVector[m_keyFrameCounts];
		m_rotation = new dQuaternion[m_keyFrameCounts];
	}
}


void dAnimationClip::Save (const char* fileName) const
{
	TiXmlDocument doc;
	TiXmlElement *mesh;
	TiXmlElement *animation;
	TiXmlDeclaration * decl;

	decl = new TiXmlDeclaration( "1.0", "", "" );
	doc.LinkEndChild( decl );

	mesh = new TiXmlElement( "Mesh" );
	mesh->SetAttribute("vendor", "newton 2.0 file format");
	mesh->SetAttribute("version", "1.0");
	doc.LinkEndChild(mesh);


	animation = new TiXmlElement( "MeshAnimation" );
	mesh->LinkEndChild(animation);

	animation->SetAttribute ("framesCount", m_framesCount);
	char buffer[1024 * 64];
	for (dListNode* node = GetFirst(); node; node = node->GetNext()) {
		dKeyFrames& keyFrames = node->GetInfo();
		TiXmlElement *keyFramesNode;

		keyFramesNode = new TiXmlElement( "node" );

		keyFramesNode->SetAttribute ("name", keyFrames.m_bindName);
		keyFramesNode->SetAttribute ("keyFramesCount", keyFrames.m_keyFrameCounts);

		TiXmlElement *keys;
		TiXmlElement *positions;
		TiXmlElement *rotations;

		keys = new TiXmlElement( "keys" );
		positions = new TiXmlElement( "positions" );
		rotations = new TiXmlElement( "rotations" );

		keyFramesNode->LinkEndChild(keys);
		keyFramesNode->LinkEndChild(positions);
		keyFramesNode->LinkEndChild(rotations);

		dModel::IntsToString (buffer, keyFrames.m_keys, keyFrames.m_keyFrameCounts);
		keys->SetAttribute("int", &buffer[0]);

		dModel::FloatsToString (buffer, &keyFrames.m_posit[0].m_x, keyFrames.m_keyFrameCounts * 4);
		positions->SetAttribute("float4", &buffer[0]);

		dModel::FloatsToString (buffer, &keyFrames.m_rotation[0].m_q0, keyFrames.m_keyFrameCounts * 4);
		rotations->SetAttribute("float4", &buffer[0]);
		animation->LinkEndChild(keyFramesNode);
	}

	doc.SaveFile (fileName);
}


void dAnimationClip::Load (const char* fileName)
{
	const TiXmlElement* root;
	TiXmlDocument doc (fileName);
	doc.LoadFile();

	root = doc.RootElement();
	if (root && !strcmp (root->Attribute ("vendor"), "newton 2.0 file format")){
		const TiXmlElement* anim;

		anim = root->FirstChildElement ("MeshAnimation");
		if (anim) {
			anim->Attribute ("framesCount", &m_framesCount);  
			for (const TiXmlElement* keyFrames = (TiXmlElement*)anim->FirstChild("node"); keyFrames; keyFrames = (TiXmlElement*)keyFrames->NextSibling()) {
				dKeyFrames* node;
				node = &Append()->GetInfo();

				sprintf (node->m_bindName, "%s", keyFrames->Attribute ("name"));
				keyFrames->Attribute ("keyFramesCount", &node->m_keyFrameCounts);  

				node->AllocaFrames (node->m_keyFrameCounts);

				TiXmlElement* keyFramesSrc;
				keyFramesSrc = (TiXmlElement*)keyFrames->FirstChild("keys");
				dModel::StringToInts (keyFramesSrc->Attribute ("int"), node->m_keys);

				keyFramesSrc = (TiXmlElement*)keyFrames->FirstChild("positions");
				dModel::StringToFloats (keyFramesSrc->Attribute ("float4"), &node->m_posit[0].m_x);

				keyFramesSrc = (TiXmlElement*)keyFrames->FirstChild("rotations");
				dModel::StringToFloats (keyFramesSrc->Attribute ("float4"), &node->m_rotation[0].m_q0);
			}
		}
	}
}
