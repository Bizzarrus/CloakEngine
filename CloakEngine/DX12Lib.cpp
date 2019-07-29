#include "stdafx.h"
#if CHECK_OS(WINDOWS,10)
#define DEFINE_DX12_FUNCS
#include "Implementation/Rendering/DX12/Lib.h"
#include "Implementation/Rendering/DX12/Manager.h"

#include <atomic>

#define LOAD_FUNC_SINGLE(Name) || loadFunc(lib,&Name,#Name)
#define LOAD_FUNC(cl,First,...) cl = cl && (loadFunc(lib,&First,#First) FOREACH(LOAD_FUNC_SINGLE,(__VA_ARGS__)))

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			namespace DX12 {
				namespace Lib {
					std::atomic<bool> g_loaded = false;
					std::atomic<bool> g_couldLoad = false;

					template<typename T> inline bool CLOAK_CALL loadFunc(In HMODULE lib, Out T* func, In LPCSTR name)
					{
						if (lib == NULL) { return false; }
						*func = (T)(GetProcAddress(lib, name));
						if ((*func) == NULL) { API::Global::Log::WriteToLog("Failed to load function '" + std::string(name) + "'", API::Global::Log::Type::Warning); }
						return (*func) != NULL;
					}

					inline bool CLOAK_CALL load()
					{
						if (g_loaded.exchange(true) == false)
						{
							bool cl = true;
							API::List<HMODULE> libs;
							if (cl)
							{
								HMODULE lib = LoadLibraryEx(TEXT("d3d12.dll"), NULL, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
								LOAD_FUNC(cl, D3D12CreateDevice);
								LOAD_FUNC(cl, D3D12GetDebugInterface);
								LOAD_FUNC(cl, D3D12SerializeVersionedRootSignature, D3D12SerializeRootSignature);
								LOAD_FUNC(cl, D3D12CreateRootSignatureDeserializer);
								libs.push_back(lib);
							}
							/*
							if (cl)
							{
								HMODULE lib = LoadLibraryEx(TEXT("d3d11.dll"), NULL, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
								LOAD_FUNC(cl, D3D11On12CreateDevice);
								libs.push_back(lib);
							}
							*/
							if(cl)
							{
								HMODULE lib = LoadLibraryEx(TEXT("dxgi.dll"), NULL, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
								LOAD_FUNC(cl, CreateDXGIFactory2);
								libs.push_back(lib);
							}
							if (!cl) 
							{
								for (size_t a = 0; a < libs.size(); a++) { FreeLibrary(libs[a]); }
							}
							g_couldLoad = cl;
						}
						return g_couldLoad;
					}

					IManager* CLOAK_CALL CreateManager()
					{
						if (load()) { return new DX12::Manager(); }
						return nullptr;
					}
				}
			}
		}
	}
}

#else

#include "Implementation/Rendering/DX12/Lib.h"
namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			namespace DX12 {
				namespace Lib {
					IManager* CLOAK_CALL CreateManager() { return nullptr; }
				}
			}
		}
	}
}

#endif