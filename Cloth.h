//------------------------------------------------------------------------------
// File: Cloth.h
// Desc: Simulation of a cloth
//
// Created: 11 December 2002 19:51:52
//
// (c)2002 Neil Wakefield
//------------------------------------------------------------------------------


#ifndef INCLUSIONGUARD_CLOTH_H
#define INCLUSIONGUARD_CLOTH_H


//------------------------------------------------------------------------------
// Included files:
//------------------------------------------------------------------------------
#include <new>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3dx9.h>
#include "DXUtil.h"
#include "D3DUtil.h"
#include "D3DEnumeration.h"
#include "D3DSettings.h"
#include "D3DApp.h"
#include "D3DFont.h"

#include "Resource.h"


//------------------------------------------------------------------------------
// Prototypes and declarations:
//------------------------------------------------------------------------------
class ParticleSystem;

//-----------------------------------------------------------------------------
// Name: struct MESH_VERTEX
// Desc: A single vertex in a mesh
//-----------------------------------------------------------------------------
struct MESH_VERTEX
{
    D3DXVECTOR3 p; // Position
    D3DXVECTOR3 n; // Normal
};

//------------------------------------------------------------------------------
// Name: class App
// Desc: The main application class
//------------------------------------------------------------------------------
class App : public CD3DApplication
{
public:
	App();
	~App();

	HRESULT OneTimeSceneInit();
	HRESULT FinalCleanup();
	HRESULT InitDeviceObjects();
	HRESULT DeleteDeviceObjects();
	HRESULT RestoreDeviceObjects();
	HRESULT InvalidateDeviceObjects();
	HRESULT Render();
	HRESULT FrameMove();

private:
	bool m_wireframe;

	CD3DFont* m_pFont;

	ParticleSystem* m_pParticleSystem;

	LPDIRECT3DVERTEXBUFFER9 m_pClothVB;
	LPDIRECT3DINDEXBUFFER9 m_pClothIB;
	LPDIRECT3DTEXTURE9 m_pClothTexture;
	D3DMATERIAL9 m_matCloth;

	LPD3DXMESH m_pSphereMesh;
	LPDIRECT3DVERTEXBUFFER9 m_pSphereVB;
	DWORD m_numSphereVertices;
	LPDIRECT3DINDEXBUFFER9 m_pSphereIB;
	DWORD m_numSphereFaces;
	DWORD m_sphereFVF;
	D3DMATERIAL9 m_matSphere;
};


#endif //INCLUSIONGUARD_CLOTH_H
