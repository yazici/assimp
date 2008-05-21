/*
---------------------------------------------------------------------------
Free Asset Import Library (ASSIMP)
---------------------------------------------------------------------------

Copyright (c) 2006-2008, ASSIMP Development Team

All rights reserved.

Redistribution and use of this software in source and binary forms, 
with or without modification, are permitted provided that the following 
conditions are met:

* Redistributions of source code must retain the above
  copyright notice, this list of conditions and the
  following disclaimer.

* Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the
  following disclaimer in the documentation and/or other
  materials provided with the distribution.

* Neither the name of the ASSIMP team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the ASSIMP Development Team.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
---------------------------------------------------------------------------
*/

/** @file Implementation of the helper class to quickly find 
 vertices close to a given position. Special implementation for 
 the 3ds loader handling smooth groups correctly  */

#include <algorithm>
#include "3DSSpatialSort.h"

#include "../include/aiAssert.h"

using namespace Assimp;
using namespace Assimp::Dot3DS;


// ------------------------------------------------------------------------------------------------
D3DSSpatialSorter::D3DSSpatialSorter()
	{
	// define the reference plane. We choose some arbitrary vector away from all basic axises 
	// in the hope that no model spreads all its vertices along this plane.
	mPlaneNormal.Set( 0.8523f, 0.34321f, 0.5736f);
	mPlaneNormal.Normalize();
	}
// ------------------------------------------------------------------------------------------------
// Destructor
D3DSSpatialSorter::~D3DSSpatialSorter()
	{
	// nothing to do here, everything destructs automatically
	}
// ------------------------------------------------------------------------------------------------
void D3DSSpatialSorter::AddFace(const Dot3DS::Face* pcFace,
	const std::vector<aiVector3D>& vPositions)
{
	ai_assert(NULL != pcFace);

	// store position by index and distance
	float distance = vPositions[pcFace->i1] * mPlaneNormal;
	mPositions.push_back( Entry( pcFace->i1, vPositions[pcFace->i1], 
		distance, pcFace->iSmoothGroup));

	// triangle vertex 2
	distance = vPositions[pcFace->i2] * mPlaneNormal;
	mPositions.push_back( Entry( pcFace->i2, vPositions[pcFace->i2], 
		distance, pcFace->iSmoothGroup));

	// triangle vertex 3
	distance = vPositions[pcFace->i3] * mPlaneNormal;
	mPositions.push_back( Entry( pcFace->i3, vPositions[pcFace->i3], 
		distance, pcFace->iSmoothGroup));
}
// ------------------------------------------------------------------------------------------------
void D3DSSpatialSorter::Prepare()
{
	// now sort the array ascending by distance.
	std::sort( this->mPositions.begin(), this->mPositions.end());
}
// ------------------------------------------------------------------------------------------------
// Returns an iterator for all positions close to the given position.
void D3DSSpatialSorter::FindPositions( const aiVector3D& pPosition, 
	uint32_t pSG,
	float pRadius,
	std::vector<unsigned int>& poResults) const
	{
	float dist = pPosition * mPlaneNormal;
	float minDist = dist - pRadius, maxDist = dist + pRadius;

	// clear the array in this strange fashion because a simple clear() would also deallocate
	// the array which we want to avoid
	poResults.erase( poResults.begin(), poResults.end());

	// quick check for positions outside the range
	if( mPositions.size() == 0)
		return;
	if( maxDist < mPositions.front().mDistance)
		return;
	if( minDist > mPositions.back().mDistance)
		return;

	// do a binary search for the minimal distance to start the iteration there
	unsigned int index = mPositions.size() / 2;
	unsigned int binaryStepSize = mPositions.size() / 4;
	while( binaryStepSize > 1)
		{
		if( mPositions[index].mDistance < minDist)
			index += binaryStepSize;
		else
			index -= binaryStepSize;

		binaryStepSize /= 2;
		}

	// depending on the direction of the last step we need to single step a bit back or forth
	// to find the actual beginning element of the range
	while( index > 0 && mPositions[index].mDistance > minDist)
		index--;
	while( index < (mPositions.size() - 1) && mPositions[index].mDistance < minDist)
		index++;

	// Mow start iterating from there until the first position lays outside of the distance range.
	// Add all positions inside the distance range within the given radius to the result aray

	float squareEpsilon = pRadius * pRadius;
	std::vector<Entry>::const_iterator it = mPositions.begin() + index;
	if (0 == pSG)
		{
		while( it->mDistance < maxDist)
			{
			if((it->mPosition - pPosition).SquareLength() < squareEpsilon)
				{
				poResults.push_back( it->mIndex);
				}
			++it;
			if( it == mPositions.end())
				break;
			}
		}
	else
		{
		while( it->mDistance < maxDist)
			{
			if((it->mPosition - pPosition).SquareLength() < squareEpsilon &&
				(it->mSmoothGroups & pSG || 0 == it->mSmoothGroups))
				{
				poResults.push_back( it->mIndex);
				}
			++it;
			if( it == mPositions.end())
				break;
			}
		}
	return;
	}

