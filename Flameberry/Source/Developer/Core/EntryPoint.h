#pragma once

#include "Core/Log.h"
#include "Application.h"

int main(int argc, char const* argv[])
{
	Flameberry::Application* clientApp;
	{
		FBY_SCOPED_TIMER("Initialisation");
		Flameberry::Ref<Flameberry::Logger> logger = Flameberry::CreateRef<Flameberry::Logger>("FLAMEBERRY");
		Flameberry::Logger::SetCoreLogger(logger);
		logger->SetLogLevel(Flameberry::LogLevel::Log);

		clientApp = Flameberry::Application::CreateClientApp({ argc, argv });
	}

	clientApp->Run();
	delete clientApp;
	return 0;
}
