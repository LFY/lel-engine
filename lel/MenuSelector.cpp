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

#include "MenuSelector.h"

#include "lel.h"

#include <string>
#include <vector>

namespace LEL {

void MenuSelector::init(void* opaque, entity_handle_t _selector) {
	fprintf(stderr, "%s: call\n", __func__);
    sys = (EntitySystem*)opaque;
    if (!sys) abort();
    selector = _selector;
    options.clear();
    std::string opts = sys->mEntities[selector].stringProps["menuselector_opt"];
    std::vector<std::string> opts_split =
        splitBy(opts, " ");

	char lvlBasename[256] = {};

	uint32_t i = 0;
    for (const auto& o : opts_split) {
        fprintf(stderr, "%s: opt %s\n", __func__, o.c_str());
		options.push_back(sys->getEntityIdByName(o));
		if (sscanf(o.c_str(), "menuselector_start_%s", lvlBasename) > 0) {
			levelStart_basenames[i] = lvlBasename;
		}
		i++;
    }
    initialized = true;
}

void MenuSelector::deinit() {
    initialized = false;
}

void MenuSelector::update() {
    // have customized control path that updates menuselector.
    // if target changed, change the parent entity or something
    // to make it visually obv.
	int discreteY = -sys->mEntities[selector].discreteY;

	discreteY = discreteY % options.size();

	selectorYPos = discreteY;

	sys->mEntities[selector].resetPos(
		sys->mEntities[options[selectorYPos]].frame.pos);

	if (sys->mEntities[selector].discreteButtonA) {
		ChangeEntitySystem change(sys, levelStart_basenames[selectorYPos]);
		change.doAction();
	}
}

} // namespace LEL
