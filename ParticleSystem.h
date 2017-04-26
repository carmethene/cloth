//------------------------------------------------------------------------------
// File: ParticleSystem.h
// Desc: Particle system for a cloth model, with constraints solver
//
// Created: 11 December 2002 20:30:09
//
// (c)2002 Neil Wakefield
//------------------------------------------------------------------------------


#ifndef INCLUSIONGUARD_PARTICLESYSTEM_H
#define INCLUSIONGUARD_PARTICLESYSTEM_H


//------------------------------------------------------------------------------
// Included files:
//------------------------------------------------------------------------------
#include <d3dx9.h>


//------------------------------------------------------------------------------
// Prototypes and declarations:
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Name: struct CLOTH_VERTEX
// Desc: A single vertex in the cloth model
//------------------------------------------------------------------------------
struct CLOTH_VERTEX
{
    D3DXVECTOR3 p;	//untransformed position
	D3DXVECTOR3 n;	//vertex normal
	float tu, tv;	//texture coordinates
};
const DWORD D3DFVF_CLOTHVERTEX = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1;

//------------------------------------------------------------------------------
// Name: struct ClothConstraint
// Desc: A structure representing an infinite spring constraint
//------------------------------------------------------------------------------
struct ClothConstraint
{
	int particleA, particleB;
	float restLength;
};

//------------------------------------------------------------------------------
// Name: class ParticleSystem
// Desc: The cloth model particle system
//------------------------------------------------------------------------------
class ParticleSystem
{
public:
	const static int PRTS_PER_DIM = 64;
	const static int NUM_PARTICLES = PRTS_PER_DIM * PRTS_PER_DIM;

	const static float SPHERE_RADIUS;
	const static D3DXVECTOR3 SPHERE_POSITION;
	const static float EDGE_CORRECTION;

	ParticleSystem();
	~ParticleSystem();

	void Initialise();

	HRESULT FillVertexBuffer( const LPDIRECT3DVERTEXBUFFER9 pVB ) const;
	HRESULT FillIndexBuffer( const LPDIRECT3DINDEXBUFFER9 pIB ) const;

	void TimeStep();

	void SetTimeStep( const float timeStep ) { m_timeStep = timeStep; }
	D3DXVECTOR3 GetPosition() const { return m_pos[ m_constraintParticle ]; }

private:
	const static int NUM_CONSTRAINTS = ( ( PRTS_PER_DIM - 1 ) * PRTS_PER_DIM * 2 ) +
									   ( ( PRTS_PER_DIM - 1 ) * ( PRTS_PER_DIM - 1 ) ) +
									   ( ( PRTS_PER_DIM - 2 ) * PRTS_PER_DIM * 2 );
	const static int NUM_ITERATIONS = 1;

	void Verlet();
	void SatisfyConstraints();
	void AccumulateForces();

	D3DXVECTOR3 GetFaceNormal( const D3DXVECTOR3& v1, const D3DXVECTOR3& v2,
							   const D3DXVECTOR3& v3 ) const;

	D3DXVECTOR3 m_pos[ PRTS_PER_DIM * PRTS_PER_DIM ];		//current particle positions
	D3DXVECTOR3 m_oldPos[ PRTS_PER_DIM * PRTS_PER_DIM ];	//old particle positions
	D3DXVECTOR3 m_acc[ PRTS_PER_DIM * PRTS_PER_DIM ];		//force accumulators

	ClothConstraint m_constraints[ NUM_CONSTRAINTS ];

	//fixed particle
	int			m_constraintParticle;
	D3DXVECTOR3 m_constraintPosition;

	D3DXVECTOR3 m_gravity;
	float		m_timeStep;

};


#endif //INCLUSIONGUARD_PARTICLESYSTEM_H
