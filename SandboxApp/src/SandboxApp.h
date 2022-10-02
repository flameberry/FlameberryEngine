#pragma once

#include "Flameberry.h"

class SandboxApp : public Flameberry::Application
{
public:
    SandboxApp();
    virtual ~SandboxApp();
    void OnUpdate(float delta) override;
    void OnUIRender() override;
};