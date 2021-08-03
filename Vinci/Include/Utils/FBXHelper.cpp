#include "FBXHelper.h"
#include "meshes.h"

FBXHelper* FBXHelper::m_Instance = nullptr;

FBXHelper::~FBXHelper()
{
	SAFE_DELETE(m_Instance);
}

FBXHelper * FBXHelper::GetInstance()
{
	if (!m_Instance)
	{
		m_Instance = new FBXHelper;
		if (m_Instance && m_Instance->Initialize())
		{
			return m_Instance;
		}
		else
		{
			// log err
		}
	}
	return nullptr;
}

bool FBXHelper::Initialize()
{
	// Fbx manager
	m_FbxMgr = FbxManager::Create();
	if (!m_FbxMgr)
	{
		return false;
	}
	FbxIOSettings* ios = FbxIOSettings::Create(m_FbxMgr, IOSROOT);
	m_FbxMgr->SetIOSettings(ios);

	// Fbx scene 
	m_SceneRoot = FbxScene::Create(m_FbxMgr, "");
 	if (!m_SceneRoot)
	{
		return false;
	}

	return true;
}

bool FBXHelper::LoadFBX(const char * fbxFileName, Meshes* meshes)
{
	// Fbx importer
	m_Importer = FbxImporter::Create(m_FbxMgr, "");
	if (!m_Importer)
	{
		return false;
	}
	m_SceneRoot->Clear();

	if (!m_Importer->Initialize(fbxFileName, -1, m_FbxMgr->GetIOSettings()))
	{
		std::string err = m_Importer->GetStatus().GetErrorString();
		return false;
	}

	if (!m_Importer->Import(m_SceneRoot))
	{
		std::string err = m_Importer->GetStatus().GetErrorString();
		return false;
	}

	ProcessNode(m_SceneRoot->GetRootNode(), meshes);
  
	return true;
}

void FBXHelper::ProcessNode(FbxNode* node, Meshes* meshes)
{
	FbxMesh* mesh = node->GetMesh();

	FbxNodeAttribute* nodeAttribute = node->GetNodeAttribute();
	if(nodeAttribute)
	{
		switch (node->GetNodeAttribute()->GetAttributeType())
		{
		case FbxNodeAttribute::eMesh:
			ProcessMesh(node->GetMesh(), meshes);
			break;
		default:
			break;
		}
	}

	for (int i = 0; i< node->GetChildCount(); ++i)
	{
		ProcessNode(node->GetChild(i), meshes);
	}
}

void FBXHelper::ProcessMesh(FbxMesh* mesh, Meshes* meshes)
{
	if (!mesh->GetNode())
		return;

	const int polygonCount = mesh->GetPolygonCount();

	// Congregate all the data of a mesh to be cached in VBOs.
	// If normal or UV is by polygon vertex, record all vertex attributes by polygon vertex.
	bool hasNormal = mesh->GetElementNormalCount() > 0;
	bool hasUV = mesh->GetElementUVCount() > 0;
	FbxGeometryElement::EMappingMode normalMappingMode = FbxGeometryElement::eNone;
	FbxGeometryElement::EMappingMode uvMappingMode = FbxGeometryElement::eNone;
	bool allByControlPoint = true;
	if (hasNormal)
	{
		normalMappingMode = mesh->GetElementNormal(0)->GetMappingMode();
		if (normalMappingMode == FbxGeometryElement::eNone)
		{
			hasNormal = false;
		}
		if (hasNormal && normalMappingMode != FbxGeometryElement::eByControlPoint)
		{
			allByControlPoint = false;
		}
	}
	if (hasUV)
	{
		uvMappingMode = mesh->GetElementUV(0)->GetMappingMode();
		if (uvMappingMode == FbxGeometryElement::eNone)
		{
			hasUV = false;
		}
		if (hasUV && uvMappingMode != FbxGeometryElement::eByControlPoint)
		{
			allByControlPoint = false;
		}
	}

	// Allocate the array memory, by control point or by polygon vertex.
	int polygonVertexCount = mesh->GetControlPointsCount();
	if (!allByControlPoint)
	{
		polygonVertexCount = polygonCount * TRIANGLE_VERTEX_COUNT;
	}

	// Populate the array with vertex attribute, if by control point.
	const FbxVector4 * controlPoints = mesh->GetControlPoints();
	FbxVector4 currentVertex;
	FbxVector4 currentNormal;
	FbxVector2 currentUV;
	if (allByControlPoint)
	{
		const FbxGeometryElementNormal * normalElement = NULL;
		const FbxGeometryElementUV * uvElement = NULL;
		if (hasNormal)
		{
			normalElement = mesh->GetElementNormal(0);
		}
		if (hasUV)
		{
			uvElement = mesh->GetElementUV(0);
		}
		for (int index = 0; index < polygonVertexCount; ++index)
		{
			// Save the vertex position.
			currentVertex = controlPoints[index];
			meshes->AddVertex(Vec4(currentVertex[0], currentVertex[1], currentVertex[2], 1.0f));
			// Save the normal.
			if (hasNormal)
			{
				int normalIndex = index;
				if (normalElement->GetReferenceMode() == FbxLayerElement::eIndexToDirect)
				{
					normalIndex = normalElement->GetIndexArray().GetAt(index);
				}
				currentNormal = normalElement->GetDirectArray().GetAt(normalIndex);
				meshes->AddNormal(Vec3(currentNormal[0], currentNormal[1], currentNormal[2]));
			}

			// Save the UV.
			if (hasUV)
			{
				int uvIndex = index;
				if (uvElement->GetReferenceMode() == FbxLayerElement::eIndexToDirect)
				{
					uvIndex = uvElement->GetIndexArray().GetAt(index);
				}
				currentUV = uvElement->GetDirectArray().GetAt(uvIndex);
				meshes->AddUV(Vec2(currentUV[0], currentUV[1]));
			}
		}

	}

	int vertexCount = 0;
	for (int polygonIndex = 0; polygonIndex < polygonCount; ++polygonIndex)
	{
		for (int verticeIndex = 0; verticeIndex < TRIANGLE_VERTEX_COUNT; ++verticeIndex)
		{
			const int controlPointIndex = mesh->GetPolygonVertex(polygonIndex, verticeIndex);

			if (allByControlPoint)
			{
				meshes->AddIndex(controlPointIndex);
			}
			// Populate the array with vertex attribute, if by polygon vertex.
			else
			{
				currentVertex = controlPoints[controlPointIndex];

				meshes->AddIndex(vertexCount);
				meshes->AddVertex(Vec4(currentVertex[0], currentVertex[1], currentVertex[2], 1.0f));

				if (hasNormal)
				{
					mesh->GetPolygonVertexNormal(polygonIndex, verticeIndex, currentNormal);				
					meshes->AddNormal(Vec3(currentNormal[0], currentNormal[1], currentNormal[2]));
				}

				if (hasUV)
				{
					bool unmappedUV;
					mesh->GetPolygonVertexUV(polygonIndex, verticeIndex, "", currentUV, unmappedUV);
					meshes->AddUV(Vec2(currentUV[0], currentUV[1]));
				}
			}
			++vertexCount;
		}
	}
}

void FBXHelper::ReadVertex(FbxMesh* mesh, int ctrlPointIndex, Vec3* vertex)
{
	FbxVector4* ctrlPoint = mesh->GetControlPoints();

	vertex->x = ctrlPoint[ctrlPointIndex][0];
	vertex->y = ctrlPoint[ctrlPointIndex][1];
	vertex->z = ctrlPoint[ctrlPointIndex][2];
}

void FBXHelper::ReadNormal(FbxMesh* mesh, int ctrlPointIndex, int vertexCounter, Vec3* normal)
{
	if (mesh->GetElementNormalCount() < 1)
	{
		return;
	}

	FbxGeometryElementNormal* leNormal = mesh->GetElementNormal(0);
	switch (leNormal->GetMappingMode())
	{
	case FbxGeometryElement::eByControlPoint:
	{
		switch (leNormal->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
		{
			normal->x = leNormal->GetDirectArray().GetAt(ctrlPointIndex)[0];
			normal->y = leNormal->GetDirectArray().GetAt(ctrlPointIndex)[1];
			normal->z = leNormal->GetDirectArray().GetAt(ctrlPointIndex)[2];
		}
		break;

		case FbxGeometryElement::eIndexToDirect:
		{
			int id = leNormal->GetIndexArray().GetAt(ctrlPointIndex);
			normal->x = leNormal->GetDirectArray().GetAt(id)[0];
			normal->y = leNormal->GetDirectArray().GetAt(id)[1];
			normal->z = leNormal->GetDirectArray().GetAt(id)[2];
		}
		break;

		default:
			break;
		}
	}
	break;

	case FbxGeometryElement::eByPolygonVertex:
	{
		switch (leNormal->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
		{
			normal->x = leNormal->GetDirectArray().GetAt(vertexCounter)[0];
			normal->y = leNormal->GetDirectArray().GetAt(vertexCounter)[1];
			normal->z = leNormal->GetDirectArray().GetAt(vertexCounter)[2];
		}
		break;

		case FbxGeometryElement::eIndexToDirect:
		{
			int id = leNormal->GetIndexArray().GetAt(vertexCounter);
			normal->x = leNormal->GetDirectArray().GetAt(id)[0];
			normal->y = leNormal->GetDirectArray().GetAt(id)[1];
			normal->z = leNormal->GetDirectArray().GetAt(id)[2];
		}
		break;

		default:
			break;
		}
	}
	break;
	}
}

void FBXHelper::ReadUV(FbxMesh* mesh, int ctrlPointIndex, int textureUVIndex, int uvLayer, Vec2* UV)
{
	if (uvLayer >= 2 || mesh->GetElementUVCount() <= uvLayer)
	{
		return;
	}

	FbxGeometryElementUV* vertexUV = mesh->GetElementUV(uvLayer);

	switch (vertexUV->GetMappingMode())
	{
	case FbxGeometryElement::eByControlPoint:
	{
		switch (vertexUV->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
		{
			UV->x = vertexUV->GetDirectArray().GetAt(ctrlPointIndex)[0];
			UV->y = vertexUV->GetDirectArray().GetAt(ctrlPointIndex)[1];
		}
		break;

		case FbxGeometryElement::eIndexToDirect:
		{
			int id = vertexUV->GetIndexArray().GetAt(ctrlPointIndex);
			UV->x = vertexUV->GetDirectArray().GetAt(id)[0];
			UV->y = vertexUV->GetDirectArray().GetAt(id)[1];
		}
		break;

		default:
			break;
		}
	}
	break;

	case FbxGeometryElement::eByPolygonVertex:
	{
		switch (vertexUV->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
		case FbxGeometryElement::eIndexToDirect:
		{
			UV->x = vertexUV->GetDirectArray().GetAt(textureUVIndex)[0];
			UV->y = vertexUV->GetDirectArray().GetAt(textureUVIndex)[1];
		}
		break;

		default:
			break;
		}
	}
	break;
	}
}
