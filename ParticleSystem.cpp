//------------------------------------------------------------------------------
// File: ParticleSystem.cpp
// Desc: Particle system for a cloth model, with constraints solver
//
// Created: 11 December 2002 20:35:51
//
// (c)2002 Neil Wakefield
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Included files:
//------------------------------------------------------------------------------
#include "ParticleSystem.h"


//------------------------------------------------------------------------------
// Definitions:
//------------------------------------------------------------------------------
const float ParticleSystem::EDGE_CORRECTION = 0.3f / ParticleSystem::PRTS_PER_DIM;
const float ParticleSystem::SPHERE_RADIUS = 0.3f;
const D3DXVECTOR3 ParticleSystem::SPHERE_POSITION =
	D3DXVECTOR3( 0.0f, - SPHERE_RADIUS - ParticleSystem::EDGE_CORRECTION, 0.0f );

//------------------------------------------------------------------------------
// Name: ParticleSystem()
// Desc: Constructor for the cloth particle system
//------------------------------------------------------------------------------
ParticleSystem::ParticleSystem()
{
	//initialise simulation values
	m_gravity = D3DXVECTOR3( 0.0f, -2.0f, 0.0f );
	m_timeStep = 0.002f;

	Initialise();
}

//------------------------------------------------------------------------------
// Name: ~ParticleSystem()
// Desc: Destructor for the cloth particle system
//------------------------------------------------------------------------------
ParticleSystem::~ParticleSystem()
{
}

//------------------------------------------------------------------------------
// Name: Initialise()
// Desc: Sets up initial values for the particle system
//------------------------------------------------------------------------------
void ParticleSystem::Initialise()
{
	//calculate the distance between particles
	const float SURFACE_SIZE = 1.0f;
	const float PARTICLE_SPACE = SURFACE_SIZE / ( PRTS_PER_DIM - 1 );

	//work out which will be the center particle in the cloth
	m_constraintParticle = ( PRTS_PER_DIM / 2 ) * PRTS_PER_DIM;	//row
	m_constraintParticle += ( ( PRTS_PER_DIM - 1 ) / 2 );	//column

	//initialise particles in a grid pattern...
	for( int row = 0; row < PRTS_PER_DIM; ++row )
	{
		for( int column = 0; column < PRTS_PER_DIM; ++column )
		{
			//calculate this particle's position
			D3DXVECTOR3 vParticlePosition = D3DXVECTOR3( PARTICLE_SPACE * column,
														 0.0f,
														 PARTICLE_SPACE * row );

			//translate so that (0,0,0) is at the center
			vParticlePosition[ 0 ] -= 0.5f;
			vParticlePosition[ 2 ] -= 0.5f;

			//set particle variables
			int index			= ( row * PRTS_PER_DIM ) + column;
			m_pos[ index ]		= vParticlePosition;
			m_oldPos[ index ]	= vParticlePosition;
			m_acc[ index ]		= D3DXVECTOR3( 0.0f, 0.0f, 0.0f );

			//set the constraint point if needed
			if( m_constraintParticle == index )
				m_constraintPosition = vParticlePosition;
		}
	}

	//initialise constraints...
	int constraintIndex = 0;

	//first set: one step in lateral directions - preserves size
	//rows
	for( int row = 0; row < PRTS_PER_DIM; ++row )
	{
		for( int column = 0; column < ( PRTS_PER_DIM - 1 ); ++column )
		{
			int particleNumber = ( row * PRTS_PER_DIM ) + column;

			ClothConstraint c;
			c.particleA		= particleNumber;
			c.particleB		= particleNumber + 1;
			c.restLength	= PARTICLE_SPACE;
			m_constraints[ constraintIndex++ ] = c;
		}
	}

	//columns
	for( int row = 0; row < ( PRTS_PER_DIM - 1 ); ++row )
	{
		for( int column = 0; column < PRTS_PER_DIM; ++column )		
		{
			int particleNumber = ( row * PRTS_PER_DIM ) + column;

			ClothConstraint c;
			c.particleA		= particleNumber;
			c.particleB		= particleNumber + PRTS_PER_DIM;
			c.restLength	= PARTICLE_SPACE;
			m_constraints[ constraintIndex++ ] = c;
		}
	}

	//second set: one step in one diagonal direction - prevents shearing
	const float diagonalLength = float( sqrt( PARTICLE_SPACE * PARTICLE_SPACE +
											  PARTICLE_SPACE * PARTICLE_SPACE ) );

	//first diagonal direction - matches with triangulation
	for( int row = 0; row < ( PRTS_PER_DIM - 1 ); ++row )
	{
		for( int column = 1; column < PRTS_PER_DIM; ++column )		
		{
			int particleNumber = ( row * PRTS_PER_DIM ) + column;

			ClothConstraint c;
			c.particleA		= particleNumber;
			c.particleB		= ( particleNumber + PRTS_PER_DIM ) - 1;
			c.restLength	= diagonalLength;
			m_constraints[ constraintIndex++ ] = c;
		}
	}

	//first set: two steps in lateral directions - preserves stiffness
	//rows
	for( int row = 0; row < PRTS_PER_DIM; ++row )
	{
		for( int column = 0; column < ( PRTS_PER_DIM - 2 ); ++column )
		{
			int particleNumber = ( row * PRTS_PER_DIM ) + column;

			ClothConstraint c;
			c.particleA		= particleNumber;
			c.particleB		= particleNumber + 2;
			c.restLength	= PARTICLE_SPACE * 2.0f;
			m_constraints[ constraintIndex++ ] = c;
		}
	}

	//columns
	for( int row = 0; row < ( PRTS_PER_DIM - 2 ); ++row )
	{
		for( int column = 0; column < PRTS_PER_DIM; ++column )			
		{
			int particleNumber = ( row * PRTS_PER_DIM ) + column;

			ClothConstraint c;
			c.particleA		= particleNumber;
			c.particleB		= particleNumber + PRTS_PER_DIM + PRTS_PER_DIM;
			c.restLength	= PARTICLE_SPACE * 2.0f;
			m_constraints[ constraintIndex++ ] = c;
		}
	}
}

//------------------------------------------------------------------------------
// Name: FillVertexBuffer()
// Desc: Fills the vertex buffer with the vertices formed by the particles
//------------------------------------------------------------------------------
HRESULT ParticleSystem::FillVertexBuffer( const LPDIRECT3DVERTEXBUFFER9 pVB ) const
{
	//lock the buffer
	CLOTH_VERTEX* pBuffer = NULL;
    if( FAILED( pVB->Lock( 0, NUM_PARTICLES * sizeof( CLOTH_VERTEX ),
						   (void**)&pBuffer, 0 ) ) )
		return( E_FAIL );

	//calculate the texture coord spacing for the vertices
	const float TEXTURE_SIZE = 1.0f;
	const float TEXTURE_SPACE = TEXTURE_SIZE / ( PRTS_PER_DIM - 1 );

	//build and copy the vertices...
	for( int row = 0; row < PRTS_PER_DIM; ++row )
	{
		for( int column = 0; column < PRTS_PER_DIM; ++column )
		{
			int particle = column + ( row * PRTS_PER_DIM );

			//calculate vertex normal...
			D3DXVECTOR3 vertexNormal = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );

			//upper left face...
			if( column != 0 && row != 0 )
				vertexNormal += GetFaceNormal( m_pos[ particle ],
											   m_pos[ particle - PRTS_PER_DIM ],
											   m_pos[ particle - 1 ] );

			//upper right face...
			if( column != ( PRTS_PER_DIM - 1 ) && row != 0 )
				vertexNormal += GetFaceNormal( m_pos[ particle ],
											   m_pos[ particle + 1 ],
											   m_pos[ particle - PRTS_PER_DIM ] );

			//lower left face...
			if( column != 0 && row != ( PRTS_PER_DIM - 1 ) )
				vertexNormal += GetFaceNormal( m_pos[ particle ],
											   m_pos[ particle - 1 ],
											   m_pos[ particle + PRTS_PER_DIM ] );
											   

			//lower right face...
			if( column != ( PRTS_PER_DIM - 1 ) && row != ( PRTS_PER_DIM - 1 ) )
				vertexNormal += GetFaceNormal( m_pos[ particle ],
											   m_pos[ particle + PRTS_PER_DIM ],
											   m_pos[ particle + 1 ] );

			//normalize result
			D3DXVec3Normalize( &vertexNormal, &vertexNormal );

			CLOTH_VERTEX v;
			v.p = m_pos[ particle ];
			v.n = vertexNormal;
			v.tu = TEXTURE_SPACE * column;
			v.tv = TEXTURE_SPACE * row;

			pBuffer[ particle ] = v;
		}
	}

	//unlock the buffer
	pVB->Unlock();

	return S_OK;
}

//------------------------------------------------------------------------------
// Name: FillIndexBuffer()
// Desc: Fills the index buffer with values to render a triangle list
//------------------------------------------------------------------------------
HRESULT ParticleSystem::FillIndexBuffer( const LPDIRECT3DINDEXBUFFER9 pIB ) const
{
	//lock the buffer
	int* pBuffer = NULL;
	const static int NUM_INDICES = ( ParticleSystem::PRTS_PER_DIM - 1 ) *
								   ( ParticleSystem::PRTS_PER_DIM - 1 ) * 6;

	if( FAILED( pIB->Lock( 0, NUM_INDICES * sizeof( WORD ), (void**)&pBuffer, 0 ) ) )
		return( E_FAIL );

	int currentIndex = 0;

	for( int row = 0; row < ( PRTS_PER_DIM - 1 ); ++row )
	{
		for( int column = 0; column < ( PRTS_PER_DIM - 1 ); ++column )
		{
			//this is per-square - fill out 6 indices = 3 triangles...
			//calculate the start of the square's indices
			int firstIndex = ( row * PRTS_PER_DIM ) + column;

			//triangle 1
			pBuffer[ currentIndex++ ] = firstIndex;
			pBuffer[ currentIndex++ ] = firstIndex + 1;
			pBuffer[ currentIndex++ ] = firstIndex + PRTS_PER_DIM;

			//triangle 2
			pBuffer[ currentIndex++ ] = firstIndex + PRTS_PER_DIM;
			pBuffer[ currentIndex++ ] = firstIndex + 1;
			pBuffer[ currentIndex++ ] = firstIndex + PRTS_PER_DIM + 1;
		}
	}

	//unlock the buffer
	pIB->Unlock();

	return S_OK;
}

//------------------------------------------------------------------------------
// Name: TimeStep()
// Desc: Updates the cloth model by one timestep
//------------------------------------------------------------------------------
void ParticleSystem::TimeStep()
{
	AccumulateForces();
	Verlet();
	SatisfyConstraints();
}

//------------------------------------------------------------------------------
// Name: Verlet()
// Desc: Performs verlet integration on the particles to find the new positions
//------------------------------------------------------------------------------
void ParticleSystem::Verlet()
{
	for( int i = 0; i < NUM_PARTICLES; ++i )
	{
		//get the current values for this particle
		D3DXVECTOR3& vPos = m_pos[ i ];
		D3DXVECTOR3 vTemp = vPos;
		D3DXVECTOR3& vOld = m_oldPos[ i ];
		D3DXVECTOR3& vAcc = m_acc[ i ];

		//calculate the new position
		vPos += vPos - vOld + vAcc * m_timeStep * m_timeStep;

		//store the last position
		vOld = vTemp;
	}
}

//------------------------------------------------------------------------------
// Name: SatisfyConstraints()
// Desc: Solves constraints for the simulation
//------------------------------------------------------------------------------
void ParticleSystem::SatisfyConstraints()
{
	for( int iteration = 0; iteration < NUM_ITERATIONS; ++iteration )
	{
		//constrain distances between particles
		for( int constraint = 0; constraint < NUM_CONSTRAINTS; ++constraint )
		{
			//get the information for the current constraint
			ClothConstraint& c	= m_constraints[ constraint ];
			D3DXVECTOR3& v1		= m_pos[ c.particleA ];
			D3DXVECTOR3& v2		= m_pos[ c.particleB ];

			//calculate the constraint
			D3DXVECTOR3 vDelta	= v2 - v1;
			float deltaLength	= D3DXVec3Length( &vDelta );
			float difference	= ( deltaLength - c.restLength ) / deltaLength;

			//move the particles to meet the constraint
			difference *= 0.5f;
			v1 += vDelta * difference;
			v2 -= vDelta * difference;
		}

		//constrain points to be outside the sphere
		for( int particle = 0; particle < NUM_PARTICLES; ++particle )
		{
			//get the information for the current constraint
			D3DXVECTOR3& v1	= m_pos[ particle ];
			D3DXVECTOR3 v2	= ParticleSystem::SPHERE_POSITION;
			float minLength	= ParticleSystem::SPHERE_RADIUS;

			minLength += EDGE_CORRECTION;

			//calculate the constraint
			D3DXVECTOR3 vDelta	= v2 - v1;
			float deltaLength	= D3DXVec3Length( &vDelta );
			
			//if point is inside the sphere, place it on the surface
			if( deltaLength < minLength )
			{
				float difference = ( deltaLength - minLength ) / deltaLength;
				v1 += vDelta * difference;
			}
		
		}
	}
	
	//fix one point of the cloth in space
	//m_pos[ m_constraintParticle ] = m_constraintPosition;
}

//------------------------------------------------------------------------------
// Name: AccumulateForces()
// Desc: Accumulates the forces for each particle
//------------------------------------------------------------------------------
void ParticleSystem::AccumulateForces()
{
	//all particles are under the influence of gravity
	for( int particle = 0; particle < NUM_PARTICLES; ++particle )
	{
		this->m_acc[ particle ] = m_gravity;
	}
}

//------------------------------------------------------------------------------
// Name: GetFaceNormal()
// Desc: Returns the face normal of a given triangle
//------------------------------------------------------------------------------
D3DXVECTOR3 ParticleSystem::GetFaceNormal( const D3DXVECTOR3& v1,
										   const D3DXVECTOR3& v2,
                                           const D3DXVECTOR3& v3 ) const
{
	D3DXVECTOR3 e1 = v2 - v1;
	D3DXVECTOR3 e2 = v3 - v2;
	D3DXVECTOR3 vNormal;

	D3DXVec3Cross( &vNormal, &e1, &e2 );
	D3DXVec3Normalize( &vNormal, &vNormal );

	return vNormal;
}