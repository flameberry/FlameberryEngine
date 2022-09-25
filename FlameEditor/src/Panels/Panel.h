#pragma once

class Panel
{
public:
    Panel() = default;
    virtual ~Panel() {};

    virtual void OnUIRender() = 0;
};
