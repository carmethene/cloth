//------------------------------------------------------------------------------
// File: Cloth.cpp
// Desc: Simulation of a cloth
//
// Created: 11 December 2002 19:59:15
//
// (c)2002 Neil Wakefield
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Included files:
//------------------------------------------------------------------------------
#include "Cloth.h"
#include "ParticleSystem.h"


//------------------------------------------------------------------------------
// Definitions:
//------------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Name: struct SPHERE_VERTEX
// Desc: A single vertex in the sphere mesh
//-----------------------------------------------------------------------------
struct SPHERE_VERTEX
{
    D3DXVECTOR3 p; // Position
    D3DXVECTOR3 n; // Normal
};

//------------------------------------------------------------------------------
// Name: App()
// Desc: Constructor for the application class
//------------------------------------------------------------------------------
App::App()
{
	//window settings
	m_strWindowTitle	= _T( "Cloth simulation using Jakobsen's method - Neil Wakefield" );
	m_dwCreationHeight	= 600;
	m_dwCreationWidth	= 800;

	//enable z-buffering
	m_d3dEnumeration.AppUsesDepthBuffer	= true;

	//create a font
	try{ m_pFont = new CD3DFont( _T( "Arial" ), 12, D3DFONT_BOLD ); }
	catch( std::bad_alloc& )
	{
		MessageBox( NULL, "Out of memory", "Error", MB_ICONEXCLAMATION | MB_OK );
		exit( 1 );
	}

	//create a particle system
	try{ m_pParticleSystem = new ParticleSystem();	}
	catch( std::bad_alloc& )
	{
		MessageBox( NULL, "Out of memory", "Error", MB_ICONEXCLAMATION | MB_OK );
		exit( 1 );
	}

	//initialise member variables
	m_pClothVB		= NULL;
	m_pClothIB		= NULL;
	m_pClothTexture	= NULL;

	m_pSphereMesh	= NULL;
	m_pSphereVB		= NULL;
	m_pSphereIB		= NULL;

	m_numSphereVertices	= 0;
	m_numSphereFaces	= 0;
	m_sphereFVF			= 0;

	m_wireframe = false;
}

//------------------------------------------------------------------------------
// Name: ~App()
// Desc: Destructor for the application class
//------------------------------------------------------------------------------
App::~App()
{
	//tidy up the particle system
	SAFE_DELETE( m_pParticleSystem );

	//tidy up the font
	SAFE_DELETE( m_pFont );
}

//------------------------------------------------------------------------------
// Name: OneTimeSceneInit()
// Desc: Sets up application-specific data on first run
//------------------------------------------------------------------------------
HRESULT App::OneTimeSceneInit()
{
	return S_OK;
}

//------------------------------------------------------------------------------
// Name: InitDeviceObjects
// Desc: Sets up device-specific data on startup and device change
//------------------------------------------------------------------------------
HRESULT App::InitDeviceObjects()
{
	//create the cloth vertex buffer
	if( FAILED( m_pd3dDevice->CreateVertexBuffer( ParticleSystem::NUM_PARTICLES *
									sizeof( CLOTH_VERTEX ),
									D3DUSAGE_WRITEONLY, D3DFVF_CLOTHVERTEX,
									D3DPOOL_MANAGED, &m_pClothVB, NULL ) ) )
		return E_FAIL;

	//create the cloth index buffer
	const static int NUM_INDICES = ( ParticleSystem::PRTS_PER_DIM - 1 ) *
								   ( ParticleSystem::PRTS_PER_DIM - 1 ) * 6;

	if( FAILED( m_pd3dDevice->CreateIndexBuffer( NUM_INDICES * sizeof( int ),
									D3DUSAGE_WRITEONLY, D3DFMT_INDEX32,
									D3DPOOL_MANAGED, &m_pClothIB, NULL ) ) )
		return E_FAIL;

	//fill out the buffers
	if( FAILED( m_pParticleSystem->FillVertexBuffer( m_pClothVB ) ) )
		return E_FAIL;

	if( FAILED( m_pParticleSystem->FillIndexBuffer( m_pClothIB ) ) )
		return E_FAIL;

	//create the cloth texture...
	HRSRC hRes;
	HGLOBAL hGlobal;
	LPVOID pData = NULL;
	ULONG dataSize = 0;
	HMODULE hModule = GetModuleHandle( NULL );
	
	//load the resource
	hRes = FindResource( hModule, MAKEINTRESOURCE( IDR_CLOTH_TEXTURE ), "jpg" );
	if( hRes == NULL )
		return E_FAIL;
	hGlobal = LoadResource( hModule, hRes );
	if( hGlobal == NULL )
		return E_FAIL;
	pData = LockResource( hGlobal );
	if( pData == NULL )
		return E_FAIL;
	dataSize = SizeofResource( hModule, hRes );

	//create the texture object from memory
	if( FAILED( D3DXCreateTextureFromFileInMemory( m_pd3dDevice, pData, dataSize,
												   &m_pClothTexture ) ) )
		return E_FAIL;

	//unload the resource
	UnlockResource( hGlobal );
	FreeResource( hGlobal );
	

	//create the sphere buffers
	if( FAILED( D3DXCreateSphere( m_pd3dDevice, ParticleSystem::SPHERE_RADIUS,
								  30, 30, &m_pSphereMesh, NULL ) ) )
		return E_FAIL;
	if( FAILED( m_pSphereMesh->GetVertexBuffer( &m_pSphereVB ) ) )
		return E_FAIL;
	if( FAILED( m_pSphereMesh->GetIndexBuffer( &m_pSphereIB ) ) )
		return E_FAIL;

	m_numSphereVertices	= m_pSphereMesh->GetNumVertices();
	m_numSphereFaces	= m_pSphereMesh->GetNumFaces();
	m_sphereFVF			= m_pSphereMesh->GetFVF();

	//initialise the font
	if( FAILED( m_pFont->InitDeviceObjects( m_pd3dDevice ) ) )
		return E_FAIL;

	return S_OK;
}

//------------------------------------------------------------------------------
// Name: RestoreDeviceObjects
// Desc: Sets up device-specific data on res change
//------------------------------------------------------------------------------
HRESULT App::RestoreDeviceObjects()
{
	//restore the font
	if( FAILED( m_pFont->RestoreDeviceObjects() ) )
		return E_FAIL;

	//set up the projection transform
	D3DXMATRIX matProj;
	float fAspect = m_d3dsdBackBuffer.Width / float(m_d3dsdBackBuffer.Height);
	D3DXMatrixPerspectiveFovLH( &matProj, D3DX_PI/4, fAspect, 1.0f, 500.0f );
	m_pd3dDevice->SetTransform( D3DTS_PROJECTION, &matProj );

	//set lighting
	D3DLIGHT9 d3dLight;
	memset( &d3dLight, 0, sizeof( D3DLIGHT9 ) );
	d3dLight.Type		= D3DLIGHT_POINT;
	d3dLight.Diffuse.r  = 0.9f;
	d3dLight.Diffuse.g  = 0.9f;
	d3dLight.Diffuse.b  = 0.9f;
	d3dLight.Position.x	= -0.5f;
	d3dLight.Position.y	= 1.0f;
	d3dLight.Position.z	= 1.0f;
	d3dLight.Range		= 1000.0f;
	d3dLight.Attenuation0 = 1.0f;
	m_pd3dDevice->SetLight( 0, &d3dLight );
	m_pd3dDevice->LightEnable( 0, TRUE );

	//create materials
	memset( &m_matCloth, 0, sizeof( D3DMATERIAL9 ) );
	m_matCloth.Diffuse.r = 0.9f;
	m_matCloth.Diffuse.g = 0.9f;
	m_matCloth.Diffuse.b = 0.9f;
	m_matCloth.Diffuse.a = 1.0f;
	m_matCloth.Ambient.r = 0.9f;
	m_matCloth.Ambient.g = 0.9f;
	m_matCloth.Ambient.b = 0.9f;
	m_matCloth.Ambient.a = 1.0f;

	memset( &m_matSphere, 0, sizeof( D3DMATERIAL9 ) );
	m_matSphere.Diffuse.r = 0.6f;
	m_matSphere.Diffuse.g = 0.6f;
	m_matSphere.Diffuse.b = 0.6f;
	m_matSphere.Diffuse.a = 1.0f;
	m_matSphere.Ambient.r = 0.6f;
	m_matSphere.Ambient.g = 0.6f;
	m_matSphere.Ambient.b = 0.6f;
	m_matSphere.Ambient.a = 1.0f;

	//set render states
	m_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
	m_pd3dDevice->SetRenderState( D3DRS_LIGHTING, TRUE );
	m_pd3dDevice->SetRenderState( D3DRS_AMBIENT, 0x66666666 );

	return S_OK;
}

//------------------------------------------------------------------------------
// Name: Render()
// Desc: Renders the current frame
//------------------------------------------------------------------------------
HRESULT App::Render()
{
	//blank the screen
	if( FAILED( m_pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
									 D3DCOLOR_ARGB(0xff,0,0,0), 1.0f, 0 ) ) )
		 return E_FAIL;

	//render the scene...
	if( SUCCEEDED( m_pd3dDevice->BeginScene() ) )
	{
		D3DXMATRIX matWorld, matRotate;
		D3DXMatrixRotationY( &matRotate, m_fTime * 0.5f );

		//render the sphere
		D3DXMatrixTranslation( &matWorld, ParticleSystem::SPHERE_POSITION[ 0 ],
										  ParticleSystem::SPHERE_POSITION[ 1 ],
										  ParticleSystem::SPHERE_POSITION[ 2 ] );
		D3DXMatrixMultiply( &matWorld, &matWorld, &matRotate );
		m_pd3dDevice->SetTransform( D3DTS_WORLD, &matWorld );
		m_pd3dDevice->SetStreamSource( 0, m_pSphereVB, 0, sizeof( SPHERE_VERTEX ) );
		m_pd3dDevice->SetIndices( m_pSphereIB );
		m_pd3dDevice->SetFVF( m_sphereFVF );
		m_pd3dDevice->SetMaterial( &m_matSphere );
		m_pd3dDevice->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, 0, m_numSphereVertices,
											0, m_numSphereFaces );

		//render the cloth model
		D3DXMatrixIdentity( &matWorld );
		D3DXMatrixMultiply( &matWorld, &matWorld, &matRotate );
		m_pd3dDevice->SetTransform( D3DTS_WORLD, &matWorld );
		m_pd3dDevice->SetStreamSource( 0, m_pClothVB, 0, sizeof( CLOTH_VERTEX ) );
		m_pd3dDevice->SetIndices( m_pClothIB );
		m_pd3dDevice->SetFVF( D3DFVF_CLOTHVERTEX );
		m_pd3dDevice->SetMaterial( &m_matCloth );
		m_pd3dDevice->SetTexture( 0, m_pClothTexture );

		const static int NUM_TRIANGLES = ( ParticleSystem::PRTS_PER_DIM - 1 ) *
										 ( ParticleSystem::PRTS_PER_DIM - 1 ) * 2;

		m_pd3dDevice->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, 0,
											ParticleSystem::NUM_PARTICLES, 0,
											NUM_TRIANGLES );

		m_pd3dDevice->SetTexture( 0, NULL );

		//render the statistics
		m_pFont->DrawText( 5.0f, 5.0f, 0xffffffff, m_strDeviceStats );
		m_pFont->DrawText( 5.0f, 25.0f, 0xffffffff, m_strFrameStats );
		m_pFont->DrawText( 5.0f, 45.0f, 0xffffffff, _T( "Press R to reset cloth (syncs timestep to framerate)" ) );
		m_pFont->DrawText( 5.0f, 65.0f, 0xffffffff, _T( "Press 1 for solid rendering mode" ) );
		m_pFont->DrawText( 5.0f, 85.0f, 0xffffffff, _T( "Press 2 for wireframe mode" ) );

		m_pd3dDevice->EndScene();
	}

	return S_OK;
}

//------------------------------------------------------------------------------
// Name: FrameMove()
// Desc: Performs between-frame animation
//------------------------------------------------------------------------------
HRESULT App::FrameMove()
{
	//if the R key is held down, reset the simulation
	if( GetKeyState( 82 ) & 0x8000 )
	{
		if( m_fFPS > 0.0f )
			m_pParticleSystem->SetTimeStep( 1.0f / m_fFPS );

		m_pParticleSystem->Initialise();		
	}

	if( GetKeyState( 49 ) & 0x8000 )	//1
        m_pd3dDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
	else if( GetKeyState( 50 ) & 0x8000 )	//2
		m_pd3dDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_WIREFRAME );

	//set up the view transform
	D3DXMATRIX matView;
	D3DXVECTOR3 vEyePt		= D3DXVECTOR3( 1.1f, 0.6f, 1.1f );
	D3DXVECTOR3 vLookAtPt	= m_pParticleSystem->GetPosition();
	vLookAtPt[ 1 ]			-= 0.35f;
	D3DXVECTOR3 vUp			= D3DXVECTOR3( 0.0f, 1.0f, 0.0f );
    D3DXMatrixLookAtLH( &matView, &vEyePt, &vLookAtPt, &vUp );
	m_pd3dDevice->SetTransform( D3DTS_VIEW, &matView );	

	//update the cloth model
	m_pParticleSystem->TimeStep();
	m_pParticleSystem->FillVertexBuffer( m_pClothVB );

    return S_OK;
}

//------------------------------------------------------------------------------
// Name: InvalidateDeviceObjects
// Desc: Tidies up device-specific data on res change
//------------------------------------------------------------------------------
HRESULT App::InvalidateDeviceObjects()
{
	//invalidate the font
	if( FAILED(	m_pFont->InvalidateDeviceObjects() ) )
		return E_FAIL;

	return S_OK;
}

//------------------------------------------------------------------------------
// Name: DeleteDeviceObjects
// Desc: Tidies up device-specific data on device change
//------------------------------------------------------------------------------
HRESULT App::DeleteDeviceObjects()
{
	//delete the sphere buffers
	SAFE_RELEASE( m_pSphereVB );
	SAFE_RELEASE( m_pSphereIB );
	SAFE_RELEASE( m_pSphereMesh );
	m_numSphereVertices	= 0;
	m_numSphereFaces	= 0;
	m_sphereFVF			= 0;

	//delete the cloth buffers
	SAFE_RELEASE( m_pClothTexture );
	SAFE_RELEASE( m_pClothVB );
	SAFE_RELEASE( m_pClothIB );

	//delete the font
	if( FAILED(	m_pFont->DeleteDeviceObjects() ) )
		return E_FAIL;

	return S_OK;
}

//------------------------------------------------------------------------------
// Name: FinalCleanup()
// Desc: Tidies up application-specific data on shutdown
//------------------------------------------------------------------------------
HRESULT App::FinalCleanup()
{
	return S_OK;
}

//------------------------------------------------------------------------------
// Name: WinMain()
// Desc: Entry point for the application
//------------------------------------------------------------------------------
INT WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, INT nShowCmd )
{
	App theApp;
	theApp.Create( hInstance );
	return theApp.Run();
}
