#pragma once

#include "pers/core/Application.h"
#include <memory>

class BufferWriteRenderer;

class PersBufferWriteApp : public pers::Application {
public:
    PersBufferWriteApp();
    ~PersBufferWriteApp() override;
    
protected:
    // Override virtual methods from Application
    bool onInitialize() override;
    void onUpdate(float deltaTime) override;
    void onRender() override;
    void onResize(int width, int height) override;
    void onKeyPress(int key, int scancode, int action, int mods) override;
    void onCleanup() override;
    
private:
    // BufferWrite-specific initialization
    bool initializeRenderer();
    bool createBufferWrite();
    
private:
    // Renderer
    std::unique_ptr<BufferWriteRenderer> _renderer;
    
    // CI environment tracking
    bool _isCI = false;
    float _ciElapsedTime = 0.0f;
    static constexpr float CI_MAX_SECONDS = 5.0f;
};