#pragma once

namespace Flameberry {

    struct ScriptEngineData;

    class ScriptEngine
    {
    public:
        static void Init();
        static void Shutdown();
    private:
        static void InitMono();
    private:
        static ScriptEngineData* s_Data;
    };

}
