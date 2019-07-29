#include "stdafx.h"
#include "CloakEngine/CloakEngine.h"

#include "CloakCompiler/Mesh.h"

#include "ShaderCompiler.h"
#include "ImageCompiler.h"
#include "FontCompiler.h"
#include "LanguageCompiler.h"
#include "MeshCompiler.h"
#include "DebugCommands.h"
#include "Lang/Test.h"

#include <vector>

#include <sstream>
#include <queue>
#include <stack>
#include <cmath>
#include <sstream>
#include <atomic>
#include <random>
#include <iomanip>

//ENABLE_NV_OPTIMUS()

//#define NO_FILE_LOAD
//#define NO_INTERFACE

struct Paths {
	std::string Input = "root:";
	std::string Output = "root:";
	std::string Temp = "root:";
};
struct PathCollection {
	Paths ShaderPath;
	Paths ImagePath;
	Paths FontPath;
	Paths LangPath;
	Paths MeshPath;
};

PathCollection* g_paths;

float camYaw = 0;
float camPitch = 0;

CloakEngine::Interface::IButton* g_button = nullptr;
CloakEngine::Interface::ILabel* g_fpsText = nullptr;
CloakEngine::Interface::IEditBox* g_edbTest = nullptr;

CloakEngine::Global::Input::IKeyBinding* g_keyToggleFPS = nullptr;
CloakEngine::Global::Input::IKeyBinding* g_showMouse = nullptr;
CloakEngine::Global::Input::IKeyBinding* g_moveForward = nullptr;
CloakEngine::Global::Input::IKeyBinding* g_moveLeft = nullptr;
CloakEngine::Global::Input::IKeyBinding* g_moveRight = nullptr;
CloakEngine::Global::Input::IKeyBinding* g_moveBack = nullptr;
CloakEngine::Global::Input::IKeyBinding* g_moveTop = nullptr;
CloakEngine::Global::Input::IKeyBinding* g_moveBottom = nullptr;

CloakEngine::Files::IString* g_fpsString = nullptr;

CloakEngine::Files::IGeneratedScene* g_tstScene = nullptr;
CloakEngine::Components::IMoveable* g_tstCamera = nullptr;
CloakEngine::Components::IDirectionalLight* g_tstLight = nullptr;

enum MoveState {
	None	= 0x00,
	Left	= 0x01,
	Right	= 0x02,
	Top		= 0x04,
	Bottom	= 0x08,
	Front	= 0x10,
	Back	= 0x20,
	MoveX	= Left | Right,
	MoveY	= Top | Bottom,
	MoveZ	= Front | Back,
};

std::atomic<uint8_t> g_curMoveState = MoveState::None;

void CLOAK_CALL keepInAngle(float* angle)
{
	const float pi = static_cast<float>(2 * CloakEngine::Global::Math::PI);
	bool n = false;
	float f = (*angle);
	if (f < 0) { n = true; f = -f; }
	if (f > pi)
	{
		f = f - (pi*(floorf(f / (pi))));
		if (n) { f = -f; }
		*angle = f;
	}
}

void updateMoveDir()
{
	const uint8_t ms = g_curMoveState;
	CloakEngine::Global::Math::Vector dir(0, 0, 0);
	const uint8_t mx = ms & MoveState::MoveX;
	const uint8_t my = ms & MoveState::MoveY;
	const uint8_t mz = ms & MoveState::MoveZ;
	if (mx == MoveState::Left) { dir += CloakEngine::Global::Math::Vector(-1, 0, 0); }
	else if (mx == MoveState::Right) { dir += CloakEngine::Global::Math::Vector(1, 0, 0); }
	if (my == MoveState::Top) { dir += CloakEngine::Global::Math::Vector(0, 1, 0); }
	else if (my == MoveState::Bottom) { dir += CloakEngine::Global::Math::Vector(0, -1, 0); }
	if (mz == MoveState::Front) { dir += CloakEngine::Global::Math::Vector(0, 0, 1); }
	else if (mz == MoveState::Back) { dir += CloakEngine::Global::Math::Vector(0, 0, -1); }
	if (dir.Length() > 0) { dir = dir.Normalize(); }
	g_tstCamera->SetRelativeVelocity(5 * dir);
}

class Game : public CloakEngine::Global::IGameEvent {
	public:
		Game() {}
		~Game() {}
		virtual void CLOAK_CALL_THIS Delete() override { delete this; }
		virtual void CLOAK_CALL_THIS OnInit() override
		{
			g_paths = new PathCollection();
			CloakEngine::Global::Graphic::Settings gset;
			CloakEngine::Global::Audio::Settings aset;

			CloakEngine::Files::IConfiguration* config = nullptr;
			CREATE_INTERFACE(CE_QUERY_ARGS(&config));
			//config->readFile(L"?HOMEDRIVE?/?HOMEPATH?/Documents/My Games/CloakEngineTest/settings.ini", CloakEngine::Files::ConfigType::INI);
			config->readFile(u"?APPDATA?/CloakEngineTest/settings.ini", CloakEngine::Files::ConfigType::INI);

			CloakEngine::Files::IValue* root = config->getRootValue();
			CloakEngine::Files::IValue* graphic = root->get("Graphic");
			CloakEngine::Files::IValue* audio = root->get("Audio");
			CloakEngine::Files::IValue* shaderPath = root->get("ShaderPath");
			CloakEngine::Files::IValue* imgPath = root->get("ImagePath");
			CloakEngine::Files::IValue* fontPath = root->get("FontPath");
			CloakEngine::Files::IValue* langPath = root->get("LangPath");
			CloakEngine::Files::IValue* meshPath = root->get("MeshPath");

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

			g_paths->ShaderPath.Input = shaderPath->get("sInput")->toString(COMP_PATH("Input\\Shader"));
			g_paths->ShaderPath.Output = shaderPath->get("sOutput")->toString(COMP_PATH("Output\\Shader"));
			g_paths->ShaderPath.Temp = shaderPath->get("sTemp")->toString(COMP_PATH("Temp\\Shader"));

			g_paths->ImagePath.Input = imgPath->get("sInput")->toString(COMP_PATH("Input\\Image"));
			g_paths->ImagePath.Output = imgPath->get("sOutput")->toString(COMP_PATH("Output\\Image"));
			g_paths->ImagePath.Temp = imgPath->get("sTemp")->toString(COMP_PATH("Temp\\Image"));

			g_paths->FontPath.Input = fontPath->get("sInput")->toString(COMP_PATH("Input\\Font"));
			g_paths->FontPath.Output = fontPath->get("sOutput")->toString(COMP_PATH("Output\\Font"));
			g_paths->FontPath.Temp = fontPath->get("sTemp")->toString(COMP_PATH("Temp\\Font"));

			g_paths->LangPath.Input = langPath->get("sInput")->toString(COMP_PATH("Input\\Language"));
			g_paths->LangPath.Output = langPath->get("sOutput")->toString(COMP_PATH("Output\\Language"));
			g_paths->LangPath.Temp = langPath->get("sTemp")->toString(COMP_PATH("Temp\\Language"));

			g_paths->MeshPath.Input = meshPath->get("sInput")->toString(COMP_PATH("Input\\Mesh"));
			g_paths->MeshPath.Output = meshPath->get("sOutput")->toString(COMP_PATH("Output\\Mesh"));
			g_paths->MeshPath.Temp = meshPath->get("sTemp")->toString(COMP_PATH("Temp\\Mesh"));

			CloakEngine::Global::Mouse::SetMouseSpeed(root->get("fMouseSpeed")->toFloat(0.4f));

			config->removeUnused();
			config->saveFile();
			SAVE_RELEASE(config);

			CloakEngine::Files::IString* windowName = nullptr;
			CREATE_INTERFACE(CE_QUERY_ARGS(&windowName));
			windowName->Set(0, "CloakEngineTest");

			CloakEngine::Global::Localization::setWindowText(windowName);
			CloakEngine::Files::SetDefaultGameID(GameID::Test);

			SAVE_RELEASE(windowName);

			CloakEngine::Global::Graphic::SetSettings(gset);
			CloakEngine::Global::Log::SetLogFilePath(u"module:log.txt");
#ifdef _DEBUG
			CloakEngine::Global::Game::SetFPS(30);
#endif

#ifndef NO_FILE_LOAD
			CloakEngine::Files::ILanguage* lang = nullptr;
			CloakEngine::Files::OpenFile(DATA_PATH("lang\\Test.cett"), CE_QUERY_ARGS(&lang));
			const CloakEngine::Global::Localization::LanguageFileID lfid = CloakEngine::Global::Localization::registerLanguageFile(lang);
			lang->Release();
			CloakEngine::Global::Localization::setLanguage(0);
#endif

			CloakEngine::Interface::Anchor anchor;
			CloakEngine::World::IEntity* tstObject = nullptr;
			
			CREATE_INTERFACE(CE_QUERY_ARGS(&g_tstScene));

#ifndef NO_FILE_LOAD
			CloakEngine::Files::IImage* fimg = nullptr;
			CloakEngine::Files::OpenFile(DATA_PATH("textures\\test.ceii"), CE_QUERY_ARGS(&fimg));

			CloakEngine::Files::IImage* skybox = nullptr;
			CloakEngine::Files::OpenFile(DATA_PATH("textures\\skybox.ceii"), CE_QUERY_ARGS(&skybox));
			CloakEngine::Files::ISkybox* skyCube = skybox->GetSkybox(0);
			g_tstScene->SetSkybox(skyCube, 4.0f);
			skyCube->Release();
			skybox->Release();

			CloakEngine::Files::IMesh* fmesh = nullptr;
			CloakEngine::Files::OpenFile(DATA_PATH("mesh\\test.cemf"), CE_QUERY_ARGS(&fmesh));
			CloakEngine::Files::IImage* fmat = nullptr;
			CloakEngine::Files::OpenFile(DATA_PATH("mesh\\test.ceii"), CE_QUERY_ARGS(&fmat));
			CloakEngine::Files::IMaterial* mats[6];
			for (size_t a = 0; a < 6; a++) { mats[a] = fmat->GetMaterial(3); }
			
			CREATE_INTERFACE(CE_QUERY_ARGS(&tstObject));
			CloakEngine::Components::IRender* renderer = tstObject->AddComponent<CloakEngine::Components::IRender>();
			tstObject->AddComponent<CloakEngine::Components::IPosition>();
			renderer->SetMesh(fmesh);
			renderer->SetMaterials(mats, 6);

			g_tstScene->Add(tstObject, -0.5f, 0, -0.5f);
			SAVE_RELEASE(tstObject);
			
			CREATE_INTERFACE(CE_QUERY_ARGS(&tstObject));
			tstObject->AddComponent<CloakEngine::Components::IRender>(renderer);
			tstObject->AddComponent<CloakEngine::Components::IPosition>();

			g_tstScene->Add(tstObject, -5, -1, -5, 10, 1, 10);
			SAVE_RELEASE(tstObject);

			CREATE_INTERFACE(CE_QUERY_ARGS(&tstObject));
			tstObject->AddComponent<CloakEngine::Components::IRender>(renderer);
			tstObject->AddComponent<CloakEngine::Components::IPosition>();

			g_tstScene->Add(tstObject, -5, -4, -5, 1, 3, 1);
			SAVE_RELEASE(tstObject);

			CREATE_INTERFACE(CE_QUERY_ARGS(&tstObject));
			tstObject->AddComponent<CloakEngine::Components::IRender>(renderer);
			tstObject->AddComponent<CloakEngine::Components::IPosition>();

			g_tstScene->Add(tstObject, -5, -4, 4, 1, 3, 1);
			SAVE_RELEASE(tstObject);

			CREATE_INTERFACE(CE_QUERY_ARGS(&tstObject));
			tstObject->AddComponent<CloakEngine::Components::IRender>(renderer);
			tstObject->AddComponent<CloakEngine::Components::IPosition>();

			g_tstScene->Add(tstObject, 4, -4, -5, 1, 3, 1);
			SAVE_RELEASE(tstObject);

			CREATE_INTERFACE(CE_QUERY_ARGS(&tstObject));
			tstObject->AddComponent<CloakEngine::Components::IRender>(renderer);
			tstObject->AddComponent<CloakEngine::Components::IPosition>();

			g_tstScene->Add(tstObject, 4, -4, 4, 1, 3, 1);
			SAVE_RELEASE(tstObject);

			renderer->Release();
			
			CREATE_INTERFACE(CE_QUERY_ARGS(&tstObject));
			renderer = tstObject->AddComponent<CloakEngine::Components::IRender>();
			tstObject->AddComponent<CloakEngine::Components::IPosition>();
			renderer->SetMesh(fmesh);
			renderer->SetMaterialCount(6);
			for (size_t a = 0; a < 6; a++) 
			{
				renderer->SetMaterial(a, fmat->GetMaterial(4));
			}

			g_tstScene->Add(tstObject, -25, -5, -25, 50, 1, 50);
			SAVE_RELEASE(tstObject);

			renderer->Release();

			fmesh->Release();
			CloakEngine::Files::OpenFile(DATA_PATH("mesh\\teapot.cemf"), CE_QUERY_ARGS(&fmesh));

			CREATE_INTERFACE(CE_QUERY_ARGS(&tstObject));
			renderer = tstObject->AddComponent<CloakEngine::Components::IRender>();
			tstObject->AddComponent<CloakEngine::Components::IPosition>();
			renderer->SetMesh(fmesh);
			renderer->SetMaterialCount(1);
			renderer->SetMaterial(0, fmat->GetMaterial(3));
			
			g_tstScene->Add(tstObject, -3, 1.2f, 0, 0.08f, 0.08f, 0.08f);
			SAVE_RELEASE(tstObject);

			renderer->Release();

			for (size_t a = 0; a < 6; a++) { mats[a]->Release(); }
			fmat->Release();

			CloakEngine::Files::OpenFile(DATA_PATH("mesh\\teapot.ceii"), CE_QUERY_ARGS(&fmat));

			CREATE_INTERFACE(CE_QUERY_ARGS(&tstObject));
			renderer = tstObject->AddComponent<CloakEngine::Components::IRender>();
			tstObject->AddComponent<CloakEngine::Components::IPosition>();
			renderer->SetMesh(fmesh);
			renderer->SetMaterialCount(1);
			renderer->SetMaterial(0, fmat->GetMaterial(0));

			g_tstScene->Add(tstObject, 3, 1.2f, 0, 0.08f, 0.08f, 0.08f);
			SAVE_RELEASE(tstObject);

			renderer->Release();
			fmat->Release();
			fmesh->Release();

			CREATE_INTERFACE(CE_QUERY_ARGS(&tstObject));
			g_tstLight = tstObject->AddComponent<CloakEngine::Components::IDirectionalLight>();
			g_tstLight->SetColor(CloakEngine::Helper::Color::RGBX(1, 1, 1), 4.0f);
			g_tstLight->SetDirection(-1.0f, -1.0f, 1.0f);
			g_tstScene->Add(tstObject);
			SAVE_RELEASE(tstObject);
#endif

			CREATE_INTERFACE(CE_QUERY_ARGS(&tstObject));
			g_tstCamera = tstObject->AddComponent<CloakEngine::Components::IMoveable>();
			CloakEngine::Components::ICamera* cam = tstObject->AddComponent<CloakEngine::Components::ICamera>();
			cam->SetViewDistance(128, 0.1f, 0, 24);
			cam->SetRenderTarget(CloakEngine::Components::RenderTargetType::Screen);

			g_tstScene->Add(tstObject, 0, 1, -5);
			cam->Release();
			SAVE_RELEASE(tstObject);

			//g_tstScene->SetAmbientLight(CloakEngine::Helper::Color::RGBX(1, 1, 1), 0.05f);
			g_tstScene->SetAmbientLight(CloakEngine::Helper::Color::RGBX(0, 0, 0), 0.05f);

			//CloakEngine::Global::Player::SetCamera(0, g_tstCamera);

#ifndef NO_INTERFACE
			CREATE_INTERFACE(CE_QUERY_ARGS(&g_button));
			anchor.offset.X = 0.0f;
			anchor.offset.Y = 0.0f;
			anchor.point = CloakEngine::Interface::AnchorPoint::TOPLEFT;
			anchor.relativePoint = CloakEngine::Interface::AnchorPoint::TOPLEFT;
			anchor.relativeTarget = nullptr;
			g_button->SetAnchor(anchor);
			g_button->SetSize(0.2f, 0.2f);
#ifndef NO_FILE_LOAD
			CloakEngine::Files::ITexture* frameTex = fimg->GetTexture(0);
			g_button->SetBackgroundTexture(frameTex);
			frameTex->Release();
#endif
			g_button->registerListener([](In const CloakEngine::Global::Events::onClick& click) 
			{
				CloakDebugLog("Register click of user " + std::to_string(click.userID));
			});
			g_button->Show();

#ifndef NO_FILE_LOAD
			fimg->Release();

			CloakEngine::Files::IFont* font = nullptr;
			CloakEngine::Files::OpenFile(DATA_PATH("fonts\\DefaultFont.cebf"), CE_QUERY_ARGS(&font));
#endif
			CREATE_INTERFACE(CE_QUERY_ARGS(&g_fpsString));

			CREATE_INTERFACE(CE_QUERY_ARGS(&g_fpsText));
			anchor.offset.X = 0.0f;
			anchor.offset.Y = 0.0f;
			anchor.point = CloakEngine::Interface::AnchorPoint::BOTTOM;
			anchor.relativePoint = CloakEngine::Interface::AnchorPoint::BOTTOM;
			anchor.relativeTarget = nullptr;
			g_fpsText->SetAnchor(anchor);
			CloakEngine::Interface::FontColor fcol;
			fcol.Color[0] = CloakEngine::Helper::Color::Red;
			fcol.Color[1] = CloakEngine::Helper::Color::Black;
			fcol.Color[2] = CloakEngine::Helper::Color::Black;
			g_fpsText->SetFontColor(fcol);
			g_fpsText->SetFontSize(20);
#ifndef NO_FILE_LOAD
			g_fpsText->SetFont(font);
#endif
			g_fpsText->SetSize(0.35f, 0.1f);
			g_fpsText->SetJustify(CloakEngine::Files::Justify::CENTER, CloakEngine::Files::AnchorPoint::BOTTOM);
			g_fpsText->SetLetterSpace(0.05f);
			g_fpsText->SetLineSpace(0.1f);
			g_fpsText->SetText(g_fpsString);
			g_fpsText->Show();

#ifdef _DEBUG
			CREATE_INTERFACE(CE_QUERY_ARGS(&g_edbTest));
			anchor.offset.X = -0.1f;
			anchor.offset.Y = 0.1f;
			anchor.point = CloakEngine::Interface::AnchorPoint::TOPRIGHT;
			anchor.relativePoint = CloakEngine::Interface::AnchorPoint::TOPRIGHT;
			anchor.relativeTarget = nullptr;
			g_edbTest->SetAnchor(anchor);
			g_edbTest->SetFontColor(fcol);
			g_edbTest->SetFontSize(20);
#ifndef NO_FILE_LOAD
			g_edbTest->SetFont(font);
#endif
			g_edbTest->SetSize(0.35f, 0.1f);
			g_edbTest->SetJustify(CloakEngine::Files::Justify::LEFT, CloakEngine::Files::AnchorPoint::CENTER);
			g_edbTest->SetLetterSpace(0.05f);
			g_edbTest->SetLineSpace(0.1f);
			g_edbTest->SetText(u"Test");
			g_edbTest->Show();
#endif

			CloakEngine::Interface::ILabel* loadScreenLabel = nullptr;
			CREATE_INTERFACE(CE_QUERY_ARGS(&loadScreenLabel));
			anchor.offset.X = 0;
			anchor.offset.Y = 0;
			anchor.point = CloakEngine::Interface::AnchorPoint::CENTER;
			anchor.relativePoint = CloakEngine::Interface::AnchorPoint::CENTER;
			anchor.relativeTarget = nullptr;
			fcol.Color[0] = CloakEngine::Helper::Color::White;
			fcol.Color[1] = CloakEngine::Helper::Color::White;
			fcol.Color[2] = CloakEngine::Helper::Color::White;
			loadScreenLabel->SetAnchor(anchor);
			loadScreenLabel->SetFontColor(fcol);
#ifndef NO_FILE_LOAD
			loadScreenLabel->SetFont(font);
#endif
			loadScreenLabel->SetFontSize(50);
			loadScreenLabel->SetSize(0.2f, 0.1f);
			loadScreenLabel->SetJustify(CloakEngine::Files::Justify::CENTER, CloakEngine::Files::AnchorPoint::CENTER);
			loadScreenLabel->SetLetterSpace(0.05f);
			loadScreenLabel->SetLineSpace(0.1f);
#ifndef NO_FILE_LOAD
			CloakEngine::Files::IString* loadText = CloakEngine::Global::Localization::getText(lfid, CloakEngine::Language::Test::LOAD_TEXT);
			loadScreenLabel->SetText(loadText);
			SAVE_RELEASE(loadText);
#endif

			CloakEngine::Interface::ILoadScreen* loadScreen = nullptr;
			CREATE_INTERFACE(CE_QUERY_ARGS(&loadScreen));
			loadScreen->SetGUI(loadScreenLabel);
			loadScreen->AddToPool();
			loadScreen->Release();

			loadScreenLabel->Release();
#ifndef NO_FILE_LOAD
			font->Release();
#endif
#endif
		}
		virtual void CLOAK_CALL_THIS OnStart() override
		{
			g_keyToggleFPS = CloakEngine::Global::Input::CreateGlobalKeyBinding(1, true, [=](In bool down, In CloakEngine::Global::Input::User user) {
				if (down)
				{
					if (g_fpsText->IsVisible()) { g_fpsText->Hide(); }
					else { g_fpsText->Show(); }
				}
			});
			g_showMouse = CloakEngine::Global::Input::CreateGlobalKeyBinding(1, false, [=](In bool down, In CloakEngine::Global::Input::User user) {
				if (down) { CloakEngine::Global::Mouse::Show(); }
				else { CloakEngine::Global::Mouse::Hide(); }
			});
			g_moveForward = CloakEngine::Global::Input::CreateGlobalKeyBinding(1, false, [=](In bool down, In CloakEngine::Global::Input::User user) {
				if (down)
				{
					g_curMoveState |= MoveState::Front;
					updateMoveDir();
				}
				else
				{
					g_curMoveState &= ~MoveState::Front;
					updateMoveDir();
				}
			});
			g_moveBack = CloakEngine::Global::Input::CreateGlobalKeyBinding(1, false, [=](In bool down, In CloakEngine::Global::Input::User user) {
				if (down)
				{
					g_curMoveState |= MoveState::Back;
					updateMoveDir();
				}
				else
				{
					g_curMoveState &= ~MoveState::Back;
					updateMoveDir();
				}
			});
			g_moveLeft = CloakEngine::Global::Input::CreateGlobalKeyBinding(1, false, [=](In bool down, In CloakEngine::Global::Input::User user) {
				if (down)
				{
					g_curMoveState |= MoveState::Left;
					updateMoveDir();
				}
				else
				{
					g_curMoveState &= ~MoveState::Left;
					updateMoveDir();
				}
			});
			g_moveRight = CloakEngine::Global::Input::CreateGlobalKeyBinding(1, false, [=](In bool down, In CloakEngine::Global::Input::User user) {
				if (down)
				{
					g_curMoveState |= MoveState::Right;
					updateMoveDir();
				}
				else
				{
					g_curMoveState &= ~MoveState::Right;
					updateMoveDir();
				}
			});
			g_moveTop = CloakEngine::Global::Input::CreateGlobalKeyBinding(1, false, [=](In bool down, In CloakEngine::Global::Input::User user) {
				if (down)
				{
					g_curMoveState |= MoveState::Top;
					updateMoveDir();
				}
				else
				{
					g_curMoveState &= ~MoveState::Top;
					updateMoveDir();
				}
			});
			g_moveBottom = CloakEngine::Global::Input::CreateGlobalKeyBinding(1, false, [=](In bool down, In CloakEngine::Global::Input::User user) {
				if (down)
				{
					g_curMoveState |= MoveState::Bottom;
					updateMoveDir();
				}
				else
				{
					g_curMoveState &= ~MoveState::Bottom;
					updateMoveDir();
				}
			});

			g_keyToggleFPS->SetKeys(0, CloakEngine::Global::Input::Keys::Keyboard::F);
			g_showMouse->SetKeys(0, CloakEngine::Global::Input::Keys::Keyboard::ALT);
			g_moveForward->SetKeys(0, CloakEngine::Global::Input::Keys::Keyboard::W);
			g_moveBack->SetKeys(0, CloakEngine::Global::Input::Keys::Keyboard::S);
			g_moveLeft->SetKeys(0, CloakEngine::Global::Input::Keys::Keyboard::A);
			g_moveRight->SetKeys(0, CloakEngine::Global::Input::Keys::Keyboard::D);
			g_moveTop->SetKeys(0, CloakEngine::Global::Input::Keys::Keyboard::SPACE);
			g_moveBottom->SetKeys(0, CloakEngine::Global::Input::Keys::Keyboard::CONTROL);


			CloakEngine::Global::Mouse::SetPos(0.5f, 0.5f);
			CloakEngine::Global::Mouse::Hide();
			CloakEngine::Global::Debug::RegisterConsoleArgument("cloak compile %o(all|shader|image|font|lang|mesh) [%o(-none|-huffman|-lzc|-lzw|-deflate|-zlib) -rc -nt]", "Compiles all files of the given type. Use -rc to force recompile and -nt to prevent temporary file creation. The first optional parameter enforces the specific compression system", [=](In const CloakEngine::Global::Debug::CommandArgument* args, In size_t argCount)
			{
				CompilerFlag flag = CompilerFlags::None;
				CloakEngine::Files::CompressType ct;
				bool udct = true;
				if (args[3].Type == CloakEngine::Global::Debug::CommandArgumentType::OPTION)
				{
					udct = false;
					switch (args[3].Value.Option)
					{
						case 0:
							ct = CloakEngine::Files::CompressType::NONE;
							break;
						case 1:
							ct = CloakEngine::Files::CompressType::HUFFMAN;
							break;
						case 2:
							ct = CloakEngine::Files::CompressType::LZC;
							break;
						case 3:
							ct = CloakEngine::Files::CompressType::LZW;
							break;
						case 4:
							ct = CloakEngine::Files::CompressType::LZH;
							break;
						case 5:
							ct = CloakEngine::Files::CompressType::ZLIB;
							break;
						default:
							break;
					}
				}
				if (args[4].Type == CloakEngine::Global::Debug::CommandArgumentType::FIXED) { flag |= CompilerFlags::ForceRecompile; }
				if (args[5].Type == CloakEngine::Global::Debug::CommandArgumentType::FIXED) { flag |= CompilerFlags::NoTempFile; }
				const size_t target = args[2].Value.Option;
				CloakEngine::Global::Log::WriteToLog("", CloakEngine::Global::Log::Type::Console);
				if (target == 0 || target == 1) { CloakEngine::Global::Log::WriteToLog("Start compiling shaders...", CloakEngine::Global::Log::Type::Console); ShaderCompiler::compileIn(g_paths->ShaderPath.Input, g_paths->ShaderPath.Output, g_paths->ShaderPath.Temp, flag, udct ? nullptr : &ct); }
				if (target == 0 || target == 2) { CloakEngine::Global::Log::WriteToLog("Start compiling images...", CloakEngine::Global::Log::Type::Console); ImageCompiler::compileIn(g_paths->ImagePath.Input, g_paths->ImagePath.Output, g_paths->ImagePath.Temp, flag, udct ? nullptr : &ct); }
				if (target == 0 || target == 3) { CloakEngine::Global::Log::WriteToLog("Start compiling fonts...", CloakEngine::Global::Log::Type::Console); FontCompiler::compileIn(g_paths->FontPath.Input, g_paths->FontPath.Output, g_paths->FontPath.Temp, flag, udct ? nullptr : &ct); }
				if (target == 0 || target == 4) { CloakEngine::Global::Log::WriteToLog("Start compiling languages...", CloakEngine::Global::Log::Type::Console); LanguageCompiler::compileIn(g_paths->LangPath.Input, g_paths->LangPath.Output, g_paths->LangPath.Temp, flag, udct ? nullptr : &ct); }
				if (target == 0 || target == 5) { CloakEngine::Global::Log::WriteToLog("Start compiling meshes...", CloakEngine::Global::Log::Type::Console); MeshCompiler::compileIn(g_paths->MeshPath.Input, g_paths->MeshPath.Output, g_paths->MeshPath.Temp, flag, udct ? nullptr : &ct); }
				return true;
			});
			CloakEngine::Global::Debug::RegisterConsoleArgument("cloak compile paths", "Prints all source paths the compiler searches", [=](In const CloakEngine::Global::Debug::CommandArgument* args, In size_t argCount)
			{
				CloakEngine::Global::Log::WriteToLog(u"Shader: \"" + CloakEngine::Files::GetFullFilePath(CloakEngine::Helper::StringConvert::ConvertToU16(g_paths->ShaderPath.Input)) + u"\"", CloakEngine::Global::Log::Type::Console);
				CloakEngine::Global::Log::WriteToLog(u"Images: \"" + CloakEngine::Files::GetFullFilePath(CloakEngine::Helper::StringConvert::ConvertToU16(g_paths->ImagePath.Input)) + u"\"", CloakEngine::Global::Log::Type::Console);
				CloakEngine::Global::Log::WriteToLog(u"Fonts: \"" + CloakEngine::Files::GetFullFilePath(CloakEngine::Helper::StringConvert::ConvertToU16(g_paths->FontPath.Input)) + u"\"", CloakEngine::Global::Log::Type::Console);
				CloakEngine::Global::Log::WriteToLog(u"Languages: \"" + CloakEngine::Files::GetFullFilePath(CloakEngine::Helper::StringConvert::ConvertToU16(g_paths->LangPath.Input)) + u"\"", CloakEngine::Global::Log::Type::Console);
				CloakEngine::Global::Log::WriteToLog(u"Meshes: \"" + CloakEngine::Files::GetFullFilePath(CloakEngine::Helper::StringConvert::ConvertToU16(g_paths->MeshPath.Input)) + u"\"", CloakEngine::Global::Log::Type::Console);
				return true;
			});
#ifdef _DEBUG
			DebugCommands::InitCommands();
#endif
		}
		virtual void CLOAK_CALL_THIS OnStop() override
		{
#ifdef _DEBUG
			DebugCommands::Stop();
#endif
			SAVE_RELEASE(g_button);
			SAVE_RELEASE(g_fpsText);
			SAVE_RELEASE(g_edbTest);
			SAVE_RELEASE(g_tstCamera);
			SAVE_RELEASE(g_tstLight);
			SAVE_RELEASE(g_tstScene);
			SAVE_RELEASE(g_fpsString);
			delete g_paths;
		}
		virtual void CLOAK_CALL_THIS OnUpdate(unsigned long long elapsedTime) override
		{
#ifndef NO_INTERFACE
			if (g_fpsText->IsVisible())
			{
				CloakEngine::Global::FPSInfo fpsi;
				CloakEngine::Global::Game::GetFPS(&fpsi);
				std::stringstream fps;
				for (size_t a = 0; a < fpsi.Used; a++)
				{
					float f = fpsi.Thread[a].FPS;
					unsigned int fl = static_cast<unsigned int>(floorf(f));
					unsigned int fm = static_cast<unsigned int>(floorf(10 * (f - floorf(f))));
					fps << ": " << fl << "." << fm << " ";
					g_fpsString->Set((a << 2) + 0, 0, CloakEngine::Helper::Color::Green);
					g_fpsString->Set((a << 2) + 1, fpsi.Thread[a].ID);
					g_fpsString->SetDefaultColor((a << 2) + 2);
					g_fpsString->Set((a << 2) + 3, fps.str());
					fps.str("");
				}
			}
#endif
		}
		virtual void CLOAK_CALL_THIS OnPause() override
		{

		}
		virtual void CLOAK_CALL_THIS OnResume() override
		{

		}
		virtual void CLOAK_CALL_THIS OnCrash(CloakEngine::Global::Debug::Error err) override
		{
			std::wstringstream txt;
			txt << L"The program has crashed with error code 0x";
			txt << std::hex << static_cast<uint64_t>(err);
			std::wstring t = txt.str();
			MessageBox(NULL, t.c_str(), L"CloakEngine has crashed!", MB_OK);
		}
		virtual void CLOAK_CALL_THIS OnUpdateLanguage() override
		{

		}
};

class Input : public CloakEngine::Global::IInputEvent {
	public:
		virtual void CLOAK_CALL_THIS Delete() override { delete this; }
		virtual void CLOAK_CALL_THIS OnMouseMove(In CloakEngine::Global::Input::User user, In const CloakEngine::Global::Math::Space2D::Vector& direction, In const CloakEngine::Global::Math::Space2D::Point& resPos, In unsigned long long etime) override
		{
			if (direction.X != 0 || direction.Y != 0)
			{
				camYaw += 5 * direction.Y;
				camPitch += 5 * direction.X;
				keepInAngle(&camPitch);
				if (camYaw > static_cast<float>(CloakEngine::Global::Math::PI) / 2) { camYaw = static_cast<float>(CloakEngine::Global::Math::PI) / 2; }
				if (camYaw < -static_cast<float>(CloakEngine::Global::Math::PI) / 2) { camYaw = -static_cast<float>(CloakEngine::Global::Math::PI) / 2; }
				//g_tstCamera->SetAngularVelocity(-50 * direction.Y, -50 * direction.X, 0);
				//g_tstCamera->SetAngularVelocity(CloakEngine::Global::Math::EulerAngles(-50 * direction.Y, -50 * direction.X, 0).ToAngularVelocity());
				g_tstCamera->SetRotation(-camYaw, -camPitch, 0, CloakEngine::Global::Math::RotationOrder::ZYX);
			}
			//else { g_tstCamera->SetAngularVelocity(0, 0, 0); }
		}
		virtual void CLOAK_CALL_THIS OnGamepadConnect(In CloakEngine::Global::Input::User user, In CloakEngine::Global::Input::CONNECT_EVENT connect) override
		{

		}
		virtual void CLOAK_CALL_THIS OnGamepadThumbMove(In CloakEngine::Global::Input::User user, In const CloakEngine::Global::Input::ThumbMoveData& data, In CloakEngine::Global::Time etime) override
		{

		}
		virtual void CLOAK_CALL_THIS OnKeyEnter(In CloakEngine::Global::Input::User user, In const CloakEngine::Global::Input::KeyCode& down, In const CloakEngine::Global::Input::KeyCode& up) override
		{

		}
		virtual void CLOAK_CALL_THIS OnScroll(In CloakEngine::Global::Input::User user, In CloakEngine::Global::Input::SCROLL_STATE button, In float delta) override
		{

		}
		virtual void CLOAK_CALL_THIS OnWindowClose() override 
		{
			CloakEngine::Global::Graphic::HideWindow();
			CloakEngine::Global::Game::Stop(); 
		}
};

class Lobby : public CloakEngine::Global::ILobbyEvent {
	public:
		virtual void CLOAK_CALL_THIS Delete() override { delete this; }
		virtual void CLOAK_CALL_THIS OnConnect(In CloakEngine::Global::IConnection* con) override 
		{
			const std::string type = con->GetType() == CloakEngine::Global::Lobby::ConnectionType::Client ? "Client" : "Server";
			const std::string prot = con->GetProtocoll() == CloakEngine::Global::Lobby::TPType::TCP ? "TCP" : "UDP";
			CloakDebugLog(prot + "-" + type + ": OnConnect");

			CloakEngine::Files::IWriter* write = nullptr;
			CREATE_INTERFACE(CE_QUERY_ARGS(&write));
			write->SetTarget(con->GetWriteBuffer(), CloakEngine::Files::CompressType::NONE, true);
			write->WriteString("GET /html HTTP/1.1\r\nHost: httpbin.org//r\nConnection: close\r\n\r\n");
			write->Save();
			write->Release();
		}
		virtual void CLOAK_CALL_THIS OnDisconnect(In CloakEngine::Global::IConnection* con) override
		{
			const std::string type = con->GetType() == CloakEngine::Global::Lobby::ConnectionType::Client ? "Client" : "Server";
			const std::string prot = con->GetProtocoll() == CloakEngine::Global::Lobby::TPType::TCP ? "TCP" : "UDP";
			CloakDebugLog(prot + "-" + type + ": OnDisconnect");
		}
		virtual void CLOAK_CALL_THIS OnReceivedData(In CloakEngine::Global::IConnection* con) override
		{
			const std::string type = con->GetType() == CloakEngine::Global::Lobby::ConnectionType::Client ? "Client" : "Server";
			const std::string prot = con->GetProtocoll() == CloakEngine::Global::Lobby::TPType::TCP ? "TCP" : "UDP";
			CloakDebugLog(prot + "-" + type + ": OnReceivedData");

			CloakEngine::Files::IReader* read = nullptr;
			CREATE_INTERFACE(CE_QUERY_ARGS(&read));
			read->SetTarget(con->GetReadBuffer(), CloakEngine::Files::CompressType::NONE, true);
			std::string line = "";
			while (read->ReadLine(&line))
			{
				CloakDebugLog("Got answer: " + line);
			}
			read->Release();
		}
		virtual void CLOAK_CALL_THIS OnConnectionFailed(In CloakEngine::Global::IConnection* con) override
		{
			const std::string type = con->GetType() == CloakEngine::Global::Lobby::ConnectionType::Client ? "Client" : "Server";
			const std::string prot = con->GetProtocoll() == CloakEngine::Global::Lobby::TPType::TCP ? "TCP" : "UDP";
			CloakDebugLog(prot + "-" + type + ": OnConnectionFailed");
			con->Release();
		}
};

class Factory : public CloakEngine::Global::IGameEventFactory {
	public:
		virtual void CLOAK_CALL_THIS Delete() const override { delete this; }
		virtual CloakEngine::Global::IGameEvent* CLOAK_CALL_THIS CreateGame() const override { return new Game(); }
		virtual CloakEngine::Global::ILauncherEvent* CLOAK_CALL_THIS CreateLauncher() const override { return nullptr; }
		virtual CloakEngine::Global::IInputEvent* CLOAK_CALL_THIS CreateInput() const override { return new Input(); }
		virtual CloakEngine::Global::ILobbyEvent* CLOAK_CALL_THIS CreateLobby() const override { return new Lobby(); }
		virtual CloakEngine::Global::IDebugEvent* CLOAK_CALL_THIS CreateDebug() const override { return nullptr; }
};

_Use_decl_annotations_ int CALLBACK WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR cmds, int cmdShow)
{
	CloakEngine::Global::GameInfo info;
#ifdef _DEBUG
	info.debugMode = true;
#else
	info.debugMode = false;
#endif
	info.useWindow = true;
	info.useConsole = true;
	info.useSteam = false;
	CloakEngine::Global::Game::StartEngine(new Factory(), info);
	return 0;
}