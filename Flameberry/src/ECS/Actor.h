#pragma once

#include <memory>

#include "Scene.h"

namespace Flameberry {
    class Actor
    {
    public:
        virtual ~Actor() = default;

        template<typename... T>
        decltype(auto) GetComponent()
        {
            if (m_SceneRef)
                return m_SceneRef->GetRegistry()->get<T...>(m_Entity);
        }

        virtual void OnInstanceCreated() = 0;
        virtual void OnInstanceDeleted() = 0;
        virtual void OnUpdate(float delta) = 0;

    private:
        Scene* m_SceneRef;
        fbentt::entity m_Entity;

        friend class Scene;
    };
}