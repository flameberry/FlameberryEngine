#include "EditorContext.h"

namespace Flameberry {

	EditorContext* EditorContext::s_Ctx = nullptr;

	void EditorContext::Create()
	{
		s_Ctx = new EditorContext();
	}

	void EditorContext::Destroy()
	{
		delete s_Ctx;
	}

} // namespace Flameberry
