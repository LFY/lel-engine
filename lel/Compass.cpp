// Copyright 2017 Lingfeng Yang
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its contributors
// may be used to endorse or promote products derived from this software without
// specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//         SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
// OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "Compass.h"

namespace LEL {

Compass::Compass(EntitySystem* sys) : mSys(sys) { }

void Compass::setEntities(entity_handle_t pointer,
                          entity_handle_t pointed) {
    mPointer = pointer;
    mPointed = pointed;
}

void Compass::update() {
	if (!mPointer || !mPointed) return;

    // get world frames of each
	Entity& ptr = mSys->getEntityById(mPointer);
	const Entity& ptd = mSys->getEntityById(mPointed);
	RefFrame ptrFrame = ptr.getWorldFrame();
	RefFrame ptdFrame = ptd.getWorldFrame();

    // get translation vector
	// diff with current position of pointer
	vector4 tr = ptdFrame.pos - ptrFrame.pos;
    
    // it's not over yet, we need to figure out the _local_
    // transformation to apply to the object so that the
    // global transformation works out.
    // in other words, get the inverse orthogonal matrix
    // of the parent object and apply it.
    
    RefFrame targetFrame;
	targetFrame.pos = makevector4(0, 0, 0, 1);
    targetFrame.fwd = v4normed(tr);
	targetFrame.up = v4normed(ptdFrame.up);
    
    RefFrame parentRotation = ptr.getParentRotationFrame();
    RefFrame newLocalRotation =
        parentRotation.getInverseFrame().preapplyFrame(targetFrame);

    ptr.frame.fwd = newLocalRotation.fwd;
    ptr.frame.up = newLocalRotation.up;
}

} // namespace
