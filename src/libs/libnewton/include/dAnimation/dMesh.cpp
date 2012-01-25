// dMesh.cpp: implementation of the dMesh class.
//
//////////////////////////////////////////////////////////////////////
#include "dAnimationStdAfx.h"
#include "dTree.h"
#include "dCRC.h"
#include "dMesh.h"
#include "dBone.h"
#include "dModel.h"
#include "tinyxml.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
dSubMesh::dSubMesh ()
{
	m_indexes = NULL;
	m_indexCount = 0;
	m_textureName[0] = 0;
	m_textureHandle = 0;
	m_shiness = 80.0f;
	m_ambient = dVector (0.2f, 0.2f, 0.2f, 1.0f);
	m_diffuse = dVector (0.7f, 0.7f, 0.7f, 1.0f);
	m_specular = dVector (1.0f, 1.0f, 1.0f, 1.0f);
}

dSubMesh::~dSubMesh ()
{
	if (m_indexes) {
		free (m_indexes);
	}
}

void dSubMesh::AllocIndexData (int indexCount)
{
	m_indexCount = indexCount;
	m_indexes = (unsigned short *) malloc (m_indexCount * sizeof (unsigned short )); 
}





dMesh::dWeightList::dWeightList(int vertexCount)
{
	m_rootBone = NULL;
	m_bindingMatrices = NULL;
	m_vertexWeight = (dVector*) malloc (vertexCount * sizeof (dVector));
	m_boneWeightIndex = (dBoneWeightIndex*) malloc (vertexCount * sizeof (dBoneWeightIndex));
	memset (m_vertexWeight, 0, vertexCount * sizeof (dVector));
	memset (m_boneWeightIndex, 0, vertexCount * sizeof (dBoneWeightIndex));
}

dMesh::dWeightList::~dWeightList()
{
	free (m_boneWeightIndex);
	free (m_vertexWeight);
	if (m_bindingMatrices) {
		free (m_bindingMatrices);
	}
}


void dMesh::dWeightList::SetBindingPose(dMesh* mesh, const dModel& model)  
{
	int bonesCount;

	bonesCount = 0;
//	for (dModel::dSkeleton::dListNode* node = model.m_skeleton.GetFirst(); node; node = node->GetNext()) {
	for (ModelComponentList<dList<dBone*> >::dListNode* list = model.m_skeleton.GetFirst(); list; list = list->GetNext()) {
		for (dList<dBone*>::dListNode* node = list->GetInfo().m_data.GetFirst(); node; node = node->GetNext()) { 
			for (dBone* bone = node->GetInfo()->GetFirst(); bone; bone = bone->GetNext()) {
				bonesCount ++;
			}
		}
	}

	m_bonesCount = bonesCount;
	m_rootBone = model.FindBone (mesh->m_boneID);
	m_boneNodes = (const dBone**) malloc (bonesCount * sizeof (dBone*));
	m_bindingMatrices = (dMatrix*) malloc (bonesCount * sizeof (dMatrix));

//	for (dModel::dSkeleton::dListNode* node = model.m_skeleton.GetFirst(); node; node = node->GetNext()) {
	for (ModelComponentList<dList<dBone*> >::dListNode* list = model.m_skeleton.GetFirst(); list; list = list->GetNext()) {
		for (dList<dBone*>::dListNode* node = list->GetInfo().m_data.GetFirst(); node; node = node->GetNext()) { 
			for (dBone* bone = node->GetInfo()->GetFirst(); bone; bone = bone->GetNext()) {
				int boneID;
				boneID = bone->GetBoneID();
				m_boneNodes[boneID] = bone;
				dMatrix matrix (bone->CalcGlobalMatrix ());
				m_bindingMatrices[boneID] = matrix.Inverse();
			}
		}
	}
}



dMesh::dMesh(dMesh::dMeshType type)
	:dList<dSubMesh>()
{
	m_type = type;
	m_boneID = 0;
	m_vertexCount = 0;
	m_uv = NULL;
	m_vertex = NULL;
	m_normal = NULL;
	m_weighList = NULL;
	m_name[0] = 0;

}

dMesh::~dMesh()
{
	if (m_vertex) {
		free (m_vertex);
		free (m_normal);
		free (m_uv);
	}

	if (m_weighList) {
		delete m_weighList;
	}
}


void dMesh::AllocVertexData (int vertexCount)
{
	m_vertexCount = vertexCount;

	m_vertex = (dFloat*) malloc (3 * m_vertexCount * sizeof (dFloat)); 
	m_normal = (dFloat*) malloc (3 * m_vertexCount * sizeof (dFloat)); 
	m_uv = (dFloat*) malloc (2 * m_vertexCount * sizeof (dFloat)); 
	memset (m_uv, 0, 2 * m_vertexCount * sizeof (dFloat));

	if (m_type == D_SKIN_MESH) {
		m_weighList = new dWeightList (vertexCount);
	}
}

dMesh::dMeshType dMesh::GetType() const
{
	return m_type;
}

dSubMesh* dMesh::AddSubMesh()
{
	return &Append()->GetInfo();
}


TiXmlElement* dMesh::ConvertToXMLNode () const
{
	int* indexList;
	dFloat* pointList;
	TiXmlElement *facesNode;
	TiXmlElement *vertexNode;
	TiXmlElement *geometry;

	geometry = new TiXmlElement( "mesh" );

	geometry->SetAttribute("type", (GetType() == D_STATIC_MESH) ? "static" : "skinMesh");
	geometry->SetAttribute("name", m_name);
	geometry->SetAttribute ("boneID", m_boneID);

	vertexNode = new TiXmlElement( "Vertex" );
	vertexNode->SetAttribute("count", m_vertexCount);
	geometry->LinkEndChild (vertexNode);

	if (m_vertexCount) {

		indexList = new int[m_vertexCount];
		pointList = new dFloat[m_vertexCount * 3];
		{
			int positCount;
			char* buffer;
			TiXmlText *indices;
			TiXmlElement *positArray;
			TiXmlElement *positArrayIndex;

			positArray = new TiXmlElement( "posit" );
			vertexNode->LinkEndChild (positArray);

			positArrayIndex = new TiXmlElement( "indices" );
			positArray->LinkEndChild (positArrayIndex);
			for (int i = 0; i < m_vertexCount * 3; i ++) {
				pointList[i] = m_vertex[i];
			}
			positCount = dModel::PackVertexArray (pointList, 3, 3 * sizeof (dFloat), m_vertexCount, indexList);

			buffer = new char [16 * 3 * m_vertexCount];
			dModel::FloatsToString (buffer, pointList, positCount * 3);
			positArray->SetAttribute("count", positCount);
			positArray->SetAttribute("float3", buffer);

			dModel::IntsToString (buffer, indexList, m_vertexCount);
			indices = new TiXmlText(buffer);
			positArrayIndex->LinkEndChild (indices);
			delete[] buffer;
		}

		{
			int positCount;
			char* buffer;
			TiXmlText *indices;
			TiXmlElement *positArray;
			TiXmlElement *positArrayIndex;

			positArray = new TiXmlElement( "normal" );
			vertexNode->LinkEndChild (positArray);

			positArrayIndex = new TiXmlElement( "indices" );
			positArray->LinkEndChild (positArrayIndex);
			for (int i = 0; i < m_vertexCount * 3; i ++) {
				pointList[i] = m_normal[i];
			}
			positCount = dModel::PackVertexArray (pointList, 3, 3 * sizeof (dFloat), m_vertexCount, indexList);

			buffer = new char [16 * 3 * m_vertexCount];
			dModel::FloatsToString (buffer, pointList, positCount * 3);
			positArray->SetAttribute("count", positCount);
			positArray->SetAttribute("float3", buffer);

			dModel::IntsToString (buffer, indexList, m_vertexCount);
			indices = new TiXmlText(buffer);
			positArrayIndex->LinkEndChild (indices);

			delete[] buffer;
		}

		{
			int positCount;
			char* buffer;
			TiXmlText *indices;
			TiXmlElement *positArray;
			TiXmlElement *positArrayIndex;

			positArray = new TiXmlElement( "uv0" );
			vertexNode->LinkEndChild (positArray);

			positArrayIndex = new TiXmlElement( "indices" );
			positArray->LinkEndChild (positArrayIndex);
			for (int i = 0; i < m_vertexCount; i ++) {
				pointList[i * 2 + 0] = m_uv[i * 2 + 0];
				pointList[i * 2 + 1] = m_uv[i * 2 + 1];
			}
			positCount = dModel::PackVertexArray (pointList, 2, 2 * sizeof (dFloat), m_vertexCount, indexList);

			buffer = new char [16 * 2 * m_vertexCount];
			dModel::FloatsToString (buffer, pointList, positCount * 2);
			positArray->SetAttribute("count", positCount);
			positArray->SetAttribute("float2", buffer);

			dModel::IntsToString (buffer, indexList, m_vertexCount);
			indices = new TiXmlText(buffer);
			positArrayIndex->LinkEndChild (indices);

			delete[] buffer;
		}


		facesNode = new TiXmlElement( "faces" );
		geometry->LinkEndChild (facesNode);
		for (dListNode* segment = GetFirst(); segment; segment = segment->GetNext()) {
			char* buffer;
			int* intArray;
			TiXmlElement *xmlSegment;
			TiXmlElement *materialSegment;

			dSubMesh& seg = segment->GetInfo();

			xmlSegment = new TiXmlElement( "triangles" );
			facesNode->LinkEndChild (xmlSegment);
			xmlSegment->SetAttribute("count", seg.m_indexCount);


			char materialBuffer[256];
			materialSegment = new TiXmlElement( "material" );
			xmlSegment->LinkEndChild (materialSegment);

			dModel::FloatsToString (materialBuffer, &seg.m_ambient.m_x, 4);
			materialSegment->SetAttribute("ambient", materialBuffer);

			dModel::FloatsToString (materialBuffer, &seg.m_diffuse.m_x, 4);
			materialSegment->SetAttribute("diffuse", materialBuffer);

			dModel::FloatsToString (materialBuffer, &seg.m_specular.m_x, 4);
			materialSegment->SetAttribute("specular", materialBuffer);

			dModel::FloatsToString (materialBuffer, &seg.m_shiness, 1);
			materialSegment->SetAttribute("shiness", materialBuffer);

			if (seg.m_textureName[0]) {
				materialSegment->SetAttribute("texture", seg.m_textureName);
			}

			buffer = new char [seg.m_indexCount * 16];
			intArray = new int[seg.m_indexCount];
			for (int i = 0; i < seg.m_indexCount; i ++) {
				intArray[i] = seg.m_indexes[i]; 
			}
			dModel::IntsToString (buffer, intArray, seg.m_indexCount);
			xmlSegment->SetAttribute("indices", buffer);
			delete[] intArray; 
			delete[] buffer;
		}

		delete[] pointList;
		delete[] indexList;

		if (GetType() == D_SKIN_MESH) {
			int index;
			char* buffer;
			int* indexArray;
			dFloat* floatArray;
			TiXmlElement* vertex;
			TiXmlElement* boneIndex;
			TiXmlElement* vertexIndex;
			TiXmlElement* weightValue;
			TiXmlElement* boneWeightMap;

			vertex = (TiXmlElement*)geometry->FirstChild("Vertex");
			boneWeightMap = new TiXmlElement("boneWeightMap");
			vertex->LinkEndChild (boneWeightMap);

			int uniqueWeight;
			uniqueWeight = 0;
			for (int i = 0; i < m_vertexCount; i ++) {
				for (int j = 0; j < 4; j ++) {
					if (m_weighList->m_vertexWeight[i][j] > 0.0f) {
						uniqueWeight ++;
					}
				}
			}

			boneWeightMap->SetAttribute ("weightCount", uniqueWeight);

			if (uniqueWeight) {
				buffer = new char [16 * uniqueWeight];
				floatArray = new dFloat [uniqueWeight];
				indexArray = new int [uniqueWeight];

				index = 0;
				vertexIndex = new TiXmlElement( "vertexIndex" );
				boneWeightMap->LinkEndChild (vertexIndex);
				for (int i = 0; i < m_vertexCount; i ++) {
					for (int j = 0; j < 4; j ++) {
						if (m_weighList->m_vertexWeight[i][j] > 0.0f) {
							indexArray[index] = i;
							index ++;
						}
					}
				}
				dModel::IntsToString (buffer, indexArray, index);
				vertexIndex->SetAttribute("indices", buffer);

				index = 0;
				boneIndex = new TiXmlElement( "boneIndex" );
				boneWeightMap->LinkEndChild (boneIndex);
				for (int i = 0; i < m_vertexCount; i ++) {
					for (int j = 0; j < 4; j ++) {
						if (m_weighList->m_vertexWeight[i][j] > 0.0f) {
							indexArray[index] = m_weighList->m_boneWeightIndex[i].m_index[j];
							index ++;
						}
					}
				}
				dModel::IntsToString (buffer, indexArray, index);
				boneIndex->SetAttribute("indices", buffer);

				index = 0;
				weightValue = new TiXmlElement( "weightValue" );
				boneWeightMap->LinkEndChild (weightValue);
				for (int i = 0; i < m_vertexCount; i ++) {
					for (int j = 0; j < 4; j ++) {
						if (m_weighList->m_vertexWeight[i][j] > 0.0f) {
							floatArray[index] = m_weighList->m_vertexWeight[i][j];
							index ++;
						}
					}
				}
				dModel::FloatsToString (buffer, floatArray, index);
				weightValue->SetAttribute("float", buffer);

				delete[] indexArray;
				delete[] floatArray;
				delete[] buffer;
			}
		}
	}

	return geometry;

}


void dMesh::Save(const char* fileName, const dList<dMesh*>& list)
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
	for (dList<dMesh*>::dListNode* node = list.GetFirst(); node; node = node->GetNext()) {
		TiXmlElement *geometry;
		geometry = node->GetInfo()->ConvertToXMLNode();
		root->LinkEndChild(geometry);
	}
	out.SaveFile (fileName);

}

void dMesh::Load(const char* fileName, dList<dMesh*>& list, dLoaderContext& context)
{
	dBone* rootBone;
	const TiXmlElement* root;
	TiXmlDocument doc (fileName);
	doc.LoadFile();

	rootBone = NULL;
	root = doc.RootElement();
	if (root && !strcmp (root->GetText (), "newton 2.0 file format")){
		for (const TiXmlElement* meshNode = (TiXmlElement*)root->FirstChild("mesh"); meshNode; meshNode = (TiXmlElement*)meshNode->NextSibling()) {
			const char* type;
			dMesh* mesh;
			
			mesh = NULL;
			type = meshNode->Attribute ("type");
			mesh = context.CreateMesh (strcmp (type, "static") ? D_SKIN_MESH : D_STATIC_MESH);

			meshNode->Attribute ("boneID", &mesh->m_boneID);
			strcpy (mesh->m_name, meshNode->Attribute ("name"));

			int vertexCount;
			const TiXmlElement* vertex;
			vertex = (TiXmlElement*)meshNode->FirstChild("Vertex");
			_ASSERTE (vertex);
			vertex->Attribute ("count", &vertexCount);  

			mesh->AllocVertexData(vertexCount);
			{
				int count;
				dFloat* data;
				int* indices;
				const TiXmlElement* posit;
				const TiXmlElement* positIndex;

				posit = (TiXmlElement*)vertex->FirstChild("posit");
				posit->Attribute ("count", &count);
				data = (dFloat*) malloc (3 * count * sizeof (dFloat));
				dModel::StringToFloats (posit->Attribute ("float3"), data);

				positIndex = (TiXmlElement*)posit->FirstChild("indices");
				indices = (int*) malloc (vertexCount * sizeof (int));;
				dModel::StringToInts (positIndex->GetText(), indices);
				for (int i = 0; i < vertexCount; i ++) {
					int index;
					index = indices[i];
					mesh->m_vertex[i * 3 + 0] = data[index * 3 + 0];
					mesh->m_vertex[i * 3 + 1] = data[index * 3 + 1];
					mesh->m_vertex[i * 3 + 2] = data[index * 3 + 2];
				}
				free (indices);
				free (data);
			}

			{
				int count;
				dFloat* data;
				int* indices;
				const TiXmlElement* posit;
				const TiXmlElement* positIndex;

				posit = (TiXmlElement*)vertex->FirstChild("normal");
				posit->Attribute ("count", &count);
				data = (dFloat*) malloc (3 * count * sizeof (dFloat));
				dModel::StringToFloats (posit->Attribute ("float3"), data);

				positIndex = (TiXmlElement*)posit->FirstChild("indices");
				indices = (int*) malloc (vertexCount * sizeof (int));
				dModel::StringToInts (positIndex->GetText(), indices);
				for (int i = 0; i < vertexCount; i ++) {
					int index;
					index = indices[i];
					mesh->m_normal[i * 3 + 0] = data[index * 3 + 0];
					mesh->m_normal[i * 3 + 1] = data[index * 3 + 1];
					mesh->m_normal[i * 3 + 2] = data[index * 3 + 2];
				}
				free (indices);
				free (data);
			}

			{
				int count;
				dFloat* data;
				int* indices;
				const TiXmlElement* posit;
				const TiXmlElement* positIndex;

				posit = (TiXmlElement*)vertex->FirstChild("uv0");
				posit->Attribute ("count", &count);
				data = (dFloat*) malloc (2 * count * sizeof (dFloat));
				dModel::StringToFloats (posit->Attribute ("float2"), data);

				positIndex = (TiXmlElement*)posit->FirstChild("indices");
				indices = (int*) malloc (vertexCount * sizeof (int));
				dModel::StringToInts (positIndex->GetText(), indices);
				for (int i = 0; i < vertexCount; i ++) {
					int index;
					index = indices[i];
					mesh->m_uv[i * 2 + 0] = data[index * 2 + 0];
					mesh->m_uv[i * 2 + 1] = data[index * 2 + 1];
				}
				free (indices);
				free (data);
			}

			if (mesh->GetType() == dMesh::D_SKIN_MESH) {
				int weightCount;
				int* boneIndex;
				int* vertexIndex;
				dFloat* vertexWeigh;
				const TiXmlElement* bones;
				const TiXmlElement* vertexs;
				const TiXmlElement* weights;
				const TiXmlElement* weightsMap;
				dMesh::dWeightList::dBoneWeightIndex* boneIndexWeight;
				dVector*  weightsPtr;

				weightsPtr = mesh->m_weighList->m_vertexWeight;
				boneIndexWeight = mesh->m_weighList->m_boneWeightIndex;

				weightsMap = (TiXmlElement*)vertex->FirstChild("boneWeightMap");
				weightsMap->Attribute ("weightCount", &weightCount);

				bones = (TiXmlElement*)weightsMap->FirstChild("boneIndex");
				vertexs = (TiXmlElement*)weightsMap->FirstChild("vertexIndex");
				weights = (TiXmlElement*)weightsMap->FirstChild("weightValue");
				
				boneIndex = (int*) malloc (weightCount * sizeof (int));
				vertexIndex = (int*) malloc (weightCount * sizeof (int));
				vertexWeigh = (dFloat*) malloc (weightCount * sizeof (dFloat));

				dModel::StringToFloats (weights->Attribute ("float"), vertexWeigh);
				dModel::StringToInts (bones->Attribute ("indices"), boneIndex);
				dModel::StringToInts (vertexs->Attribute ("indices"), vertexIndex);
				for (int i = 0; i < weightCount; i ++) {
					int bone; 
					int index; 
					dFloat weight; 

					bone = boneIndex[i];
					index = vertexIndex[i];
					weight = vertexWeigh[i];
					for (int j = 0; j < 4; j ++) {
						if (weightsPtr[index][j] == dFloat (0.0f)) {
							boneIndexWeight[index].m_index[j] = bone;
							weightsPtr[index][j] = weight;
							break;
						}
					}
				}
				free (vertexWeigh);
				free (vertexIndex);
				free (boneIndex);
			}

			// add the triangles to the mesh 
			const TiXmlElement* segments;
			segments = (TiXmlElement*)meshNode->FirstChild("faces");
			_ASSERTE (segments);

			for (const TiXmlElement* seg = (TiXmlElement*)segments->FirstChild("triangles"); seg; seg = (TiXmlElement*)seg->NextSibling()) {

				int indexCount;
				int materialID;
				int* indices;
				
				dSubMesh* segment;
				TiXmlElement* material;

				segment = mesh->AddSubMesh();

				materialID = 0;
				seg->Attribute ("count", &indexCount);

				material = (TiXmlElement*)seg->FirstChild("material");
				if (material) {
					const char* texName;
					dModel::StringToFloats (material->Attribute("ambient"), &segment->m_ambient.m_x);
					dModel::StringToFloats (material->Attribute("diffuse"), &segment->m_diffuse.m_x);
					dModel::StringToFloats (material->Attribute("specular"), &segment->m_specular.m_x);
					dModel::StringToFloats (material->Attribute("shiness"), &segment->m_shiness);
					texName = material->Attribute ("texture");
					if (texName) {
						strcpy (segment->m_textureName, texName);
					}
				}

				segment->AllocIndexData (indexCount);

				indices = (int*) malloc (indexCount * sizeof (int)); 
				dModel::StringToInts (seg->Attribute ("indices"), indices);
				for (int i = 0; i < indexCount; i ++) {
					segment->m_indexes[i] = short (indices[i]);
				}
				free (indices);
			}

			list.Append (mesh);
		} 
	}
}


