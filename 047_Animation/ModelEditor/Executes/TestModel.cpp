#include "stdafx.h"
#include "TestModel.h"
#include "Fbx/FbxLoader.h"
#include "Environment/Terrain.h"

void TestModel::Initialize()
{
	FbxLoader* loader = NULL;
	vector<wstring> clipList;

	/*loader = new FbxLoader
	(
	Assets + L"Paladin/Mesh.fbx",
	Models + L"Paladin/", L"Paladin"
	);
	loader->ExportMaterial();
	loader->ExportMesh();
	loader->ExportAnimation(0);

	loader->GetClipList(&clipList);
	SAFE_DELETE(loader);


	gameModel = new GameModel
	(
	Shaders + L"039_Model.fx",
	Models + L"Paladin/Paladin.material",
	Models + L"Paladin/Paladin.mesh"
	);*/

	/*gameModel = new GameModel
	(
	Shaders + L"039_Model.fx",
	Models + L"Paladin/Paladin.material",
	Models + L"Paladin/Paladin.mesh"
	);*/

	terrainMaterial = new Material(Shaders + L"028_Terrain.fx");
	terrainMaterial->SetDiffuseMap(Textures + L"Dirt2.png");
	terrain = new Terrain(terrainMaterial, Textures + L"HeightMap256.png");
}

void TestModel::Ready()
{
	gameModel = new GameAnimator
	(
		Shaders + L"046_Model.fx",
		Models + L"Kachujin/Kachujin.material",
		Models + L"Kachujin/Kachujin.mesh"
	);
	gameModel->AddClip(Models + L"Kachujin/Idle.animation");
	gameModel->Ready();

	gameModel->Scale(0.01f, 0.01f, 0.01f);
}

void TestModel::Destroy()
{

}

void TestModel::Update()
{
	gameModel->Update();


}

void TestModel::PreRender()
{

}

void TestModel::Render()
{
	gameModel->Render();
}