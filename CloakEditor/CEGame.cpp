#include "stdafx.h"
#include "CloakCallback/CEGame.h"
#include "CloakEditor.h"

namespace Editor {
	// {6EFD9477-F866-40A7-8280-11FE5AD5287D}
	static const GUID GAME_ID = { 0x6efd9477, 0xf866, 0x40a7, { 0x82, 0x80, 0x11, 0xfe, 0x5a, 0xd5, 0x28, 0x7d } };

	void CLOAK_CALL_THIS CEGame::Delete() { delete this; }
	void CLOAK_CALL_THIS CEGame::OnInit()
	{
		CloakEngine::Global::Graphic::HideWindow();//TODO: DEBUG
		CloakEngine::Global::Mouse::SetKeepInScreen(false);
		CloakEngine::Global::Graphic::Settings gset;
		CloakEngine::Global::Audio::Settings aset;

		CloakEngine::Files::IConfiguration* config = nullptr;
		CREATE_INTERFACE(CE_QUERY_ARGS(&config));
		config->readFile(u"$APPDATA//CloakEditor//settings.ini", CloakEngine::Files::ConfigType::INI);

		CloakEngine::Files::IValue* root = config->getRootValue();
		CloakEngine::Files::IValue* graphic = root->get("Graphic");
		CloakEngine::Files::IValue* audio = root->get("Audio");

		gset.Resolution.Width = graphic->get("iWidth")->toInt(640);
		gset.Resolution.Height = graphic->get("iHeight")->toInt(480);
		gset.Resolution.XOffset = 0;
		gset.Resolution.YOffset = 0;
		gset.BackBufferCount = graphic->get("iBackBuffer")->toInt(1);
		gset.VSync = graphic->get("bVSync")->toBool(true);
		gset.WindowMode = static_cast<CloakEngine::Global::Graphic::WindowMode>(graphic->get("iMode")->toInt(static_cast<int>(CloakEngine::Global::Graphic::WindowMode::WINDOW)));
		gset.MSAA = static_cast<CloakEngine::Global::Graphic::MSAA>(graphic->get("iMSAA")->toInt(static_cast<int>(CloakEngine::Global::Graphic::MSAA::DISABLED)));
		gset.FXAA = graphic->get("bFXAA")->toBool(false);
		gset.FieldOfView = graphic->get("fFOV")->toFloat(static_cast<float>(CloakEngine::Global::Math::PI / 4));
		gset.ShadowMapResolution = graphic->get("iShadowMap")->toInt(1024);
		gset.TextureFilter = static_cast<CloakEngine::Global::Graphic::TextureFilter>(graphic->get("iFilter")->toInt(static_cast<int>(CloakEngine::Global::Graphic::TextureFilter::TRILINIEAR)));
		gset.MultiThreadedRendering = graphic->get("bMTR")->toBool(true);
		gset.TextureQuality = static_cast<CloakEngine::Global::Graphic::TextureQuality>(graphic->get("iTextureQuality")->toInt(static_cast<int>(CloakEngine::Global::Graphic::TextureQuality::ULTRA)));
		gset.Gamma = graphic->get("fGamma")->toFloat(1 / 2.2f);
		gset.RenderMode = CloakEngine::Global::Graphic::RenderMode::DX12;
		gset.AdapterID = static_cast<uint32_t>(graphic->get("iGPU")->toInt(0));
		gset.Tessellation = graphic->get("bTessellation")->toBool(false);
		gset.Bloom = static_cast<CloakEngine::Global::Graphic::BloomMode>(graphic->get("iBloom")->toInt(static_cast<int>(CloakEngine::Global::Graphic::BloomMode::NORMAL)));

		aset.volumeEffects = audio->get("fVolumeEffects")->toFloat(1);
		aset.volumeMaster = audio->get("fVolumeMaster")->toFloat(1);
		aset.volumeMusic = audio->get("fVolumeMusic")->toFloat(1);
		aset.volumeSpeech = audio->get("fVolumeSpeech")->toFloat(1);

		config->removeUnused();
		config->saveFile(u"$APPDATA//CloakEditor//settings.ini", CloakEngine::Files::ConfigType::INI);

		//CloakEngine::Global::Localization::setWindowText(u"CloakEngineTest");
		CloakEngine::Files::SetDefaultGameID(GAME_ID);

		SAVE_RELEASE(config);
		CloakEngine::Global::Graphic::SetSettings(gset);
		CloakEngine::Global::Log::SetLogFilePath(u".\\log.txt");
	}
	void CLOAK_CALL_THIS CEGame::OnStart()
	{

	}
	void CLOAK_CALL_THIS CEGame::OnStop()
	{
		Editor::Stop();
	}
	void CLOAK_CALL_THIS CEGame::OnUpdate(unsigned long long elapsedTime)
	{

	}
	void CLOAK_CALL_THIS CEGame::OnPause()
	{

	}
	void CLOAK_CALL_THIS CEGame::OnResume()
	{

	}
	void CLOAK_CALL_THIS CEGame::OnCrash(CloakEngine::Global::Debug::Error err)
	{

	}
	void CLOAK_CALL_THIS CEGame::OnUpdateLanguage()
	{

	}
}