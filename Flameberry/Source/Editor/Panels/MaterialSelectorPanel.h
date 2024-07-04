#pragma once

#include "Flameberry.h"

namespace Flameberry {
	class MaterialSelectorPanel
	{
	public:
		using CallBackType = std::function<void(const Ref<MaterialAsset>&)>;

	public:
		void OpenPanel(const CallBackType& selectCallBack);
		void OnUIRender();

	private:
		bool m_Open = false;
		Ref<MaterialAsset> m_SelectedMaterial;
		CallBackType m_SelectCallBack;
	};
} // namespace Flameberry
