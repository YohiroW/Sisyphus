#ifndef _FBXHELPER_H_
#define _FBXHELPER_H_
#include "fbxsdk.h"
#include "Globals.h"

class Meshes;

class FBXHelper
{
public:
	FBXHelper() {}
	~FBXHelper();

	static FBXHelper* GetInstance();

	bool Initialize();
	bool LoadFBX(const char* fbxFileName, Meshes* meshes);

protected:    
	void ProcessNode(FbxNode* node, Meshes* meshes);
	void ProcessMesh(FbxMesh* mesh, Meshes* meshes);

	void ReadVertex(FbxMesh* mesh, int ctrlPointIndex, Vec3* vertex);
	void ReadNormal(FbxMesh* mesh, int ctrlPointIndex, int vertexCounter, Vec3* normal);
	void ReadUV(FbxMesh* mesh, int ctrlPointIndex, int textureUVIndex, int uvLayer, Vec2* UV);

private:
	static FBXHelper* m_Instance;

	FbxManager * m_FbxMgr{ nullptr };
	FbxScene* m_SceneRoot{ nullptr };
	FbxImporter* m_Importer{ nullptr };
};


#endif
