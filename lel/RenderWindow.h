#pragma once

namespace LEL {

class RenderWindow {
public:
    RenderWindow(int w, int h);
    void post();
private:
    void* mWindow;
};

} // namespace LEL

