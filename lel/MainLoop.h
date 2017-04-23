#pragma once

#include "RenderWindow.h"
#include "EntitySystem.h"
#include "EntitySystemRenderer.h"
#include "GameControllers.h"

#include <unordered_map>

namespace LEL {

typedef void (*RenderCallback)(void);
typedef void (*ControllerHandler)(GameControllerState* state);
typedef void (*KeyboardHandler)(
    bool downup,
    int keycode);

class MainLoop {
public:
    MainLoop(RenderWindow* win) : mWindow(win) { }
    void setSystem(EntitySystem* sys,
                   EntitySystemRenderer* renderer) {
        mSystem = sys;
        mRenderer = renderer;
    }
    int run();
    void addControl(int id, ControllerHandler c) {
        mControllerHandlers[id] = c;
    }
    void addKB(int id, KeyboardHandler k) {
        mKeyboardHandlers[id] = k;
    }
    void addDraw(int id, RenderCallback r) {
        mDraws[id] = r;
    }
private:
    RenderWindow* mWindow;
    EntitySystem* mSystem;
    EntitySystemRenderer* mRenderer;
    std::unordered_map<int, ControllerHandler> mControllerHandlers;
    std::unordered_map<int, KeyboardHandler> mKeyboardHandlers;
    std::unordered_map<int, RenderCallback> mDraws;
};

} // namespace LEL
