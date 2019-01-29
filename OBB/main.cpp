// Created by zoi@3map.snu.ac.kr
// for SNU CG DirectX lecture

#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <atltrace.h>
#include "CollisionDetection.hpp"

// 함수 선언
int WINAPI WinMain( HINSTANCE hinstance, HINSTANCE hprevinstance, LPSTR lpcmdline, int ncmdshow );
LRESULT WINAPI WndProc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp );

HWND handle_window = 0;

void registerClass();
void createWindow();
void messagePump();

bool setupDevice();
void setupModel();
void setupLight();
void setupCamera();

void render();
void finalize();

#define HOUSE_SCALING	0.05F

// bounding box 관련 함수들
D3DXMATRIX* GetBoxTransform(D3DXMATRIX *pMat, CBox* pBox);
void SetBoxTransform(const D3DXMATRIX *pMat, CBox* pBox);
void initBox(CBox *pBox, const D3DXVECTOR3& vecMin, const D3DXVECTOR3& vecMax);
void moveBox(CBox *pBox, const D3DXMATRIX& mat);

// 변수 선언
IDirect3D9* d3d9 = 0;
IDirect3DDevice9* d3d9_device = 0;

// 메쉬
ID3DXMesh* d3dx_mesh_car = 0;
D3DMATERIAL9* materials_car = 0;
IDirect3DTexture9** textures_car = 0;
DWORD num_materials_car = 0;

ID3DXMesh* d3dx_mesh_house = 0;
D3DMATERIAL9* materials_house = 0;
IDirect3DTexture9** textures_house = 0;
DWORD num_materials_house = 0;

// 바운딩 박스
CBox box_car;
CBox box_house;

// 카메라 애니메이션
FLOAT theta = D3DXToRadian(20.0F);
FLOAT distance = 80.0F;

// 함수 정의
int WINAPI WinMain( HINSTANCE hinstance, HINSTANCE hprevinstance, LPSTR lpcmdline, int ncmdshow )
{
	registerClass();
	createWindow();

	setupDevice();
	setupModel();
	setupLight();

	ShowWindow( handle_window, SW_SHOW );

	// set the timer
	SetTimer( handle_window, 1, 50, NULL );

	messagePump();

	finalize();

	return 0;
}

void registerClass()
{
	HINSTANCE hInstance = GetModuleHandle( NULL );

	WNDCLASSEX wc = { 
		sizeof(WNDCLASSEX), 
		CS_CLASSDC, 
		WndProc, 
		0L, 
		0L,
		hInstance, 
		NULL,
		LoadCursor( NULL, IDC_ARROW ),
		( HBRUSH )GetStockObject( WHITE_BRUSH ),
		NULL,
		"direct3d", 
		NULL 
	};
	RegisterClassEx( &wc );
}

void createWindow()
{
	HINSTANCE hInstance = GetModuleHandle( NULL );

	handle_window = CreateWindow( 
		"direct3d", 
		"DirectX Graphics",
		WS_OVERLAPPEDWINDOW, 
		0, 0, 800, 600,
		GetDesktopWindow(), 
		NULL, 
		hInstance, 
		NULL 
	);
}

void messagePump()
{
	MSG  msg;

	while( GetMessage( &msg, NULL, 0U, 0U ) )
	{
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}
}

//
LRESULT WINAPI WndProc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
	switch( msg )
	{

		case WM_DESTROY:
			{
				PostQuitMessage( 0 );
				return 0;
			}
			break;

		case WM_PAINT:
			{
				render();
				ValidateRect( hwnd, NULL );

				return 0;
			}
			break;

		case WM_KEYDOWN:
			{
				int nVirtKey = (int)wp;
				switch( nVirtKey )
				{
				case VK_UP:
					distance -= 0.25F;
					break;
				case VK_DOWN:
					distance += 0.25F;
					break;
				case VK_LEFT:
					theta += D3DXToRadian( 5.0F );
					break;
				case VK_RIGHT:
					theta -= D3DXToRadian( 5.0F );
					break;
				default:
					return 0;
				}

				InvalidateRect( hwnd, NULL, FALSE );
				return 0;
			}
			break;

		case WM_TIMER:
			{
				// move the car
				D3DXMATRIX matMove;
				D3DXMatrixTranslation(&matMove, 0, 0, -0.3F);
				moveBox(&box_car, matMove);
				// Collision Detection Test!
				int nRet = BoxBoxIntersectionTest(box_car, box_house);
				if(nRet == 1) 
				{
					// 복잡한 collision handling을 할 수 있겠지만, 지금은 단순히 멈추기
					D3DXMatrixTranslation(&matMove, 0, 0, 0.3F);
					moveBox(&box_car, matMove);
				}
				else {
					render();
					ValidateRect( hwnd, NULL );
				}
			}

		default :
			break;
   }

	return DefWindowProc( hwnd, msg, wp, lp );
}

bool setupDevice()
{
	// 다이렉트3D 생성
	d3d9 = Direct3DCreate9( D3D_SDK_VERSION );

	//
	HRESULT hResult;

	// 현재 디스플레이 모드 얻어 오기
	D3DDISPLAYMODE d3ddm;
	hResult = d3d9->GetAdapterDisplayMode( D3DADAPTER_DEFAULT, &d3ddm );
	if( FAILED( hResult ) ) {
		return false;
	}

	// 프리젠테이션 파라미터 설정
	D3DPRESENT_PARAMETERS d3dpp; 
	ZeroMemory( &d3dpp, sizeof(d3dpp) );
	d3dpp.SwapEffect				= D3DSWAPEFFECT_DISCARD;
	d3dpp.Windowed					= TRUE;
	d3dpp.BackBufferWidth			= 800;
	d3dpp.BackBufferHeight			= 600;
	d3dpp.BackBufferFormat			= d3ddm.Format;
	d3dpp.EnableAutoDepthStencil	= TRUE;
	d3dpp.AutoDepthStencilFormat	= D3DFMT_D16;

	hResult = d3d9->CreateDevice
	( 
		D3DADAPTER_DEFAULT, 
		D3DDEVTYPE_HAL,						// 디바이스 타입
		handle_window,						// 윈도우 핸들
		D3DCREATE_SOFTWARE_VERTEXPROCESSING,
		&d3dpp,								// 프리젠테이션 파라미터
		&d3d9_device						// 생성된 디바이스
	);
	if( FAILED( hResult ) ) {
		return false;
	}

	// render states
	d3d9_device->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
	d3d9_device->SetRenderState( D3DRS_NORMALIZENORMALS, TRUE );
	d3d9_device->SetRenderState( D3DRS_SPECULARENABLE, TRUE );
	d3d9_device->SetRenderState( D3DRS_AMBIENT, 0x00808080 );

	// texture stage states
	d3d9_device->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_DISABLE );
	d3d9_device->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	d3d9_device->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
	d3d9_device->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE );

	return true;
}

void finalize()
{
	if( materials_car )
	{
		delete[] materials_car;
	}
	if( textures_car )
	{
		for(UINT i=0; i < num_materials_car; ++i)
			if(textures_car[i]) textures_car[i]->Release();
		delete[] textures_car;
	}
	if( d3dx_mesh_car )
	{
		d3dx_mesh_car->Release();
	}
	if( materials_house )
	{
		delete[] materials_house;
	}
	if( textures_house )
	{
		for(UINT i=0; i < num_materials_house; ++i)
			if(textures_house[i]) textures_house[i]->Release();
		delete[] textures_house;
	}
	if( d3dx_mesh_house )
	{
		d3dx_mesh_house->Release();
	}
	if( d3d9_device )
	{
		d3d9_device->Release();
	}
	if( d3d9 )
	{
		d3d9->Release();
	}
}

D3DXMATRIX* GetBoxTransform(D3DXMATRIX *pMat, CBox* pBox)
{
	int i,j;
	real fMat[16];
	pBox->GetTransform(fMat);
	for(i=0; i < 4; ++i)
		for(j=0; j < 4; ++j)
			(*pMat)(i,j) = fMat[i*4 + j];
	return pMat;
}

void SetBoxTransform(const D3DXMATRIX* pMat, CBox* pBox)
{
	int i,j;
	for(i=0; i < 3; ++i) {
		for(j=0; j < 3; ++j)
			pBox->axis[i][j] = (*pMat)(i, j);
		pBox->translation[i] = (*pMat)(3, i);
	}
}

void initBox(CBox *pBox, const D3DXVECTOR3& vecMin, const D3DXVECTOR3& vecMax)
{
	pBox->center[0] = (vecMin.x + vecMax.x)/2.0F;
	pBox->center[1] = (vecMin.y + vecMax.y)/2.0F;
	pBox->center[2] = (vecMin.z + vecMax.z)/2.0F;

	pBox->extent[0] = vecMax.x - pBox->center[0];
	pBox->extent[1] = vecMax.y - pBox->center[1];
	pBox->extent[2] = vecMax.z - pBox->center[2];

	// identity world coordinate axis
	pBox->axis[0][0] = 1.0F; pBox->axis[0][1] = 0.0F; pBox->axis[0][2] = 0.0F;
	pBox->axis[1][0] = 0.0F; pBox->axis[1][1] = 1.0F; pBox->axis[1][2] = 0.0F;
	pBox->axis[2][0] = 0.0F; pBox->axis[2][1] = 0.0F; pBox->axis[2][2] = 1.0F;

	pBox->translation[0] = 0.0F; pBox->translation[1] = 0.0F; pBox->translation[2] = 0.0F;
}

void moveBox(CBox *pBox, const D3DXMATRIX& mat)
{
	D3DXMATRIX matBox;
	// 박스의 transform을 가져온다.
	GetBoxTransform(&matBox, pBox);
	// 박스의 transform을 바꾼다.
	matBox *= mat;
	SetBoxTransform(&matBox, pBox);
	// 박스의 center 좌표도 바꾼다.
	D3DXVECTOR3 vecCenter(pBox->center[0], pBox->center[1], pBox->center[2]);
	D3DXVec3TransformCoord(&vecCenter, &vecCenter, &mat);
	pBox->center[0] = vecCenter.x; pBox->center[1] = vecCenter.y; pBox->center[2] = vecCenter.z;
}

void setupModel()
{
	ID3DXBuffer* d3dx_material_buffer = 0;

	// 메쉬 로딩
	HRESULT hr = D3DXLoadMeshFromX(
		"car.x",
		D3DXMESH_SYSTEMMEM,
		d3d9_device,
		NULL,
		&d3dx_material_buffer,
		NULL,
		(DWORD*)&num_materials_car,
		&d3dx_mesh_car
	);
	if( FAILED( hr ) ) {
		MessageBox( handle_window, "Failed in loading mesh", "error", MB_OK );
	}
	else {
		// 재질 및 텍스처 생성
		materials_car = new D3DMATERIAL9[ num_materials_car ];
		textures_car = new IDirect3DTexture9*[ num_materials_car ];

		D3DXMATERIAL* d3dx_materials = (D3DXMATERIAL*)d3dx_material_buffer->GetBufferPointer();
		for( DWORD i=0; i < num_materials_car; i++ )
		{
			materials_car[i] = d3dx_materials[i].MatD3D;
			
			HRESULT hr = D3DXCreateTextureFromFile(
				d3d9_device,
				d3dx_materials[i].pTextureFilename,
				&textures_car[i]
			);
			if( FAILED( hr ) ) {
				textures_car[i] = 0;
			}
		}
		d3dx_material_buffer->Release();
	}
	// 바운딩 박스 계산
	DWORD dwVertexNum=d3dx_mesh_car->GetNumVertices();
	DWORD dwFvfSize=D3DXGetFVFVertexSize(d3dx_mesh_car->GetFVF());
	D3DXVECTOR3 vecMin, vecMax, vecMinWorld, vecMaxWorld;
	VOID *ptr=NULL;
	d3dx_mesh_car->LockVertexBuffer(0,&ptr);
	D3DXComputeBoundingBox((D3DXVECTOR3*)ptr, dwVertexNum, dwFvfSize, &vecMin, &vecMax);
	d3dx_mesh_car->UnlockVertexBuffer();
	// Geometry 계산
	D3DXMATRIX matCar;
	D3DXMatrixTranslation(&matCar, 0, 0, 20);
	// CBox 계산
	initBox(&box_car, vecMin, vecMax);
	moveBox(&box_car, matCar);
	
	// 메쉬 로딩
	hr = D3DXLoadMeshFromX(
		"cryotank1.x",
		D3DXMESH_SYSTEMMEM,
		d3d9_device,
		NULL,
		&d3dx_material_buffer,
		NULL,
		(DWORD*)&num_materials_house,
		&d3dx_mesh_house
	);
	if( FAILED( hr ) ) {
		MessageBox( handle_window, "Failed in loading mesh", "error", MB_OK );
	}
	else {
		// 재질 및 텍스처 생성
		materials_house = new D3DMATERIAL9[ num_materials_house ];
		textures_house = new IDirect3DTexture9*[ num_materials_house ];

		D3DXMATERIAL* d3dx_materials = (D3DXMATERIAL*)d3dx_material_buffer->GetBufferPointer();
		for( DWORD i=0; i < num_materials_house; i++ )
		{
			materials_house[i] = d3dx_materials[i].MatD3D;
			
			HRESULT hr = D3DXCreateTextureFromFile(
				d3d9_device,
				d3dx_materials[i].pTextureFilename,
				&textures_house[i]
			);
			if( FAILED( hr ) ) {
				textures_house[i] = 0;
			}
		}
		d3dx_material_buffer->Release();
	}
	// 바운딩 박스 계산
	dwVertexNum=d3dx_mesh_house->GetNumVertices();
	dwFvfSize=D3DXGetFVFVertexSize(d3dx_mesh_house->GetFVF());
	d3dx_mesh_house->LockVertexBuffer(0,&ptr);
	D3DXComputeBoundingBox((D3DXVECTOR3*)ptr, dwVertexNum, dwFvfSize, &vecMin, &vecMax);
	d3dx_mesh_house->UnlockVertexBuffer();
	// Geometry 계산
	D3DXMATRIX matTranslation, matScaling;
	D3DXMatrixScaling(&matScaling, HOUSE_SCALING, HOUSE_SCALING, HOUSE_SCALING);
	D3DXMatrixTranslation(&matTranslation, 0.0F, 0.0F, -10.0F);
	// CBox 계산
	initBox(&box_house, vecMin*HOUSE_SCALING, vecMax*HOUSE_SCALING);
	moveBox(&box_house, matTranslation);
}

void setupLight()
{
	// 빛
	D3DLIGHT9 light;
	ZeroMemory( &light, sizeof(D3DLIGHT9) );

	light.Type = D3DLIGHT_DIRECTIONAL;
	light.Direction = D3DXVECTOR3( -1.0F, -1.0F, -1.0F );
	light.Ambient = D3DXCOLOR( 0.5F, 0.5F, 0.5F, 1.0F );
	light.Diffuse = D3DXCOLOR( 1.0F, 1.0F, 1.0F, 1.0F );
	light.Specular = D3DXCOLOR( 1.0F, 1.0F, 1.0F, 1.0F );

	d3d9_device->SetLight( 0, &light );
	d3d9_device->LightEnable( 0, TRUE );
}

void setupCamera()
{
	D3DXMATRIX matWorld;
	D3DXMatrixIdentity( &matWorld );

	FLOAT camera_x = distance * cosf( theta );
	FLOAT camera_z = distance * sinf( theta );

	D3DXMATRIX matView;
	D3DXMatrixLookAtLH
	( 
		&matView, 
		&D3DXVECTOR3( camera_x, 10.0F, camera_z ),
		&D3DXVECTOR3( 0.0F, 0.0F, 0.0F ),
		&D3DXVECTOR3( 0.0F, 1.0F, 0.0F )
	);

	D3DXMATRIX matProjection;
	D3DXMatrixPerspectiveFovLH
	( 
		&matProjection, 
		D3DXToRadian( 45.0F ), 
		1.0f, 
		0.1f, 
		1000.0f 
	);
	
	d3d9_device->SetTransform( D3DTS_WORLD, &matWorld );
	d3d9_device->SetTransform( D3DTS_VIEW, &matView );
	d3d9_device->SetTransform( D3DTS_PROJECTION, &matProjection );
}

void render()
{
	if( !d3d9_device )
	{
		return;
	}
	setupCamera();

	d3d9_device->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(255, 255, 255), 1.0F, 0 );
	d3d9_device->BeginScene();

	D3DXMATRIX matCar, matHouse, matHouseScaling;
	d3d9_device->SetTransform( D3DTS_WORLD, GetBoxTransform(&matCar, &box_car) );

	// 메쉬 렌더링
	for(UINT i=0; i < num_materials_car; ++i)
	{
		d3d9_device->SetMaterial( &materials_car[i] );
		d3d9_device->SetTexture( 0, textures_car[i] );
		d3dx_mesh_car->DrawSubset( i );
	}

	GetBoxTransform(&matHouse, &box_house);
	D3DXMatrixScaling(&matHouseScaling, HOUSE_SCALING, HOUSE_SCALING, HOUSE_SCALING);
	matHouse = matHouseScaling * matHouse;
	d3d9_device->SetTransform( D3DTS_WORLD, &matHouse );

	// 메쉬 렌더링
	for(UINT i=0; i < num_materials_house; ++i)
	{
		d3d9_device->SetMaterial( &materials_house[i] );
		d3d9_device->SetTexture( 0, textures_house[i] );
		d3dx_mesh_house->DrawSubset( i );
	}

	d3d9_device->EndScene();
	d3d9_device->Present( NULL, NULL, NULL, NULL );
}
