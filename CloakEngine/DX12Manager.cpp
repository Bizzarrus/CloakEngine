#include "stdafx.h"
#if CHECK_OS(WINDOWS,10)
#include "Implementation/Rendering/DX12/Manager.h"
#include "Implementation/Rendering/DX12/Allocator.h"
#include "Implementation/Rendering/DX12/ColorBuffer.h"
#include "Implementation/Rendering/DX12/DepthBuffer.h"
#include "Implementation/Rendering/DX12/PipelineState.h"
#include "Implementation/Rendering/DX12/RootSignature.h"
#include "Implementation/Rendering/DX12/StructuredBuffer.h"
#include "Implementation/Rendering/DX12/SwapChain.h"
#include "Implementation/Rendering/DX12/Defines.h"
#include "Implementation/Rendering/DX12/Lib.h"
#include "Implementation/Rendering/DX12/Casting.h"
#include "Implementation/Rendering/DX12/VideoEncoder.h"
#include "Implementation/Rendering/DX12/Query.h"
#include "Implementation/Global/Game.h"
#include "Implementation/Rendering/Context.h"
#include "Implementation/Global/Video.h"

#include "CloakEngine/Global/Game.h"
#include "CloakEngine/Helper/StringConvert.h"

#include <limits>

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

//#define USE_DX_DEBUG

#if (defined(_DEBUG) && defined(USE_DX_DEBUG))
#include <dxgidebug.h>
#pragma comment(lib,"dxguid.lib")
typedef HRESULT(__stdcall *PFN_DXGI_GET_DEBUG_INTERFACE)(const IID&, void**);
#endif

#define CE_MAX(a,b)            (((a) > (b)) ? (a) : (b))
#define CE_MIN(a,b)            (((a) < (b)) ? (a) : (b))

#define COLOR_ORDER_SET(p,color,L,type) if constexpr(p>0){t[p-1]=ToType<type, expLen, std::is_signed_v<type>>::Call(color.L);}
#define COLOR_ORDER_GET(p,color,L,type) if constexpr(p>0){color->L=FromType<type, expLen, std::is_signed_v<type>>::Call(t[p-1]);} else {color->L=0;}

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			namespace DX12 {
				namespace Manager_v1 {
					constexpr DWORD g_ThreadSyncWait = 125;
					constexpr DWORD g_ThreadSyncMaxWait = 500;
					constexpr uint32_t CONTEXT_TYPE_COUNT = 2;
					constexpr D3D_SHADER_MODEL SUPPORTED_SHADER_MODELS[] = {
						D3D_SHADER_MODEL_6_5,
						D3D_SHADER_MODEL_6_4,
						D3D_SHADER_MODEL_6_3,
						D3D_SHADER_MODEL_6_2,
						D3D_SHADER_MODEL_6_1,
						D3D_SHADER_MODEL_6_0,
						D3D_SHADER_MODEL_5_1,
					};
					constexpr D3D_FEATURE_LEVEL SUPPORTED_FEATURE_LEVELS[] = {
								D3D_FEATURE_LEVEL_12_1,
								D3D_FEATURE_LEVEL_12_0,
								D3D_FEATURE_LEVEL_11_1,
								D3D_FEATURE_LEVEL_11_0,
					};
					constexpr D3D_FEATURE_LEVEL MINIMAL_FEATURE_LEVEL = SUPPORTED_FEATURE_LEVELS[ARRAYSIZE(SUPPORTED_FEATURE_LEVELS) - 1];

#if (defined(_DEBUG) && defined(USE_DX_DEBUG))
					constexpr UINT FACTORY_FLAGS = DXGI_CREATE_FACTORY_DEBUG;
#else
					constexpr UINT FACTORY_FLAGS = 0;
#endif

					CLOAK_FORCEINLINE std::string CLOAK_CALL FEATURE_LEVEL_NAME(In D3D_FEATURE_LEVEL lvl)
					{
						switch (lvl)
						{
							case D3D_FEATURE_LEVEL_9_1: return "DX 9.1";
							case D3D_FEATURE_LEVEL_9_2: return "DX 9.2";
							case D3D_FEATURE_LEVEL_9_3: return "DX 9.3";
							case D3D_FEATURE_LEVEL_10_0: return "DX 10.0";
							case D3D_FEATURE_LEVEL_10_1: return "DX 10.1";
							case D3D_FEATURE_LEVEL_11_0: return "DX 11.0";
							case D3D_FEATURE_LEVEL_11_1: return "DX 11.1";
							case D3D_FEATURE_LEVEL_12_0: return "DX 12.0";
							case D3D_FEATURE_LEVEL_12_1: return "DX 12.1";
							default: return std::to_string(lvl);
						}
					}
					CLOAK_FORCEINLINE constexpr bool CLOAK_CALL __CheckFeatureLevelArray()
					{
						for (size_t a = 1; a < ARRAYSIZE(SUPPORTED_FEATURE_LEVELS); a++)
						{
							if (SUPPORTED_FEATURE_LEVELS[a] >= SUPPORTED_FEATURE_LEVELS[a - 1]) { return false; }
						}
						return true;
					}
					static_assert(__CheckFeatureLevelArray(), "List of supported feature levels must be sorted from highest to lowest!");

					struct AdapterInfo {
						IDXGIAdapter1* Adapter;
						IDXGIOutput* Output;
						HMONITOR Monitor;
					};

#define CREATE_AND_POPULATE(Name, Prev) if(SUCCEEDED(hRet) && device->Prev != nullptr) { device->Name = device->Prev; } else { device->Prev = nullptr; hRet = Lib::D3D12CreateDevice(adapter, MINIMAL_FEATURE_LEVEL, CE_QUERY_ARGS(&device->Name)); if(SUCCEEDED(hRet)){ API::Global::Log::WriteToLog("DirectX 12 API Version: " #Name); }}
					inline HRESULT CreateDevice(In IDXGIAdapter* adapter, Out GraphicDevice* device)
					{
						HRESULT hRet = E_NOINTERFACE;
						CREATE_AND_POPULATE(V6, V6); //Highest supported version, so Prev == Name
						CREATE_AND_POPULATE(V5, V6);
						CREATE_AND_POPULATE(V4, V5);
						CREATE_AND_POPULATE(V3, V4);
						CREATE_AND_POPULATE(V2, V3);
						CREATE_AND_POPULATE(V1, V2);
						CREATE_AND_POPULATE(V0, V1);
						return hRet;
					}
#undef CREATE_AND_POPULATE

					CLOAK_CALL Manager::Manager()
					{
						m_factory = nullptr;
						m_device = nullptr;
						m_isReady = false;
						m_init = false;
						m_isReady = false;
						m_queues = nullptr;
						m_adapterCount = 0;
						DEBUG_NAME(Manager);
					}
					CLOAK_CALL Manager::~Manager()
					{
						
					}
					HRESULT CLOAK_CALL_THIS Manager::Create(In UINT adapterID)
					{
						VideoEncoder* ve = new VideoEncoder();
						Impl::Global::Video::Initialize(ve);
						SAVE_RELEASE(ve);
						HRESULT hRet = E_FAIL;

#if (defined(_DEBUG) && defined(USE_DX_DEBUG))
						ID3D12Debug* debug = nullptr;
						hRet = Lib::D3D12GetDebugInterface(CE_QUERY_ARGS(&debug));
						if (CloakDebugCheckOK(hRet, API::Global::Debug::Error::GRAPHIC_ENABLE_DEBUG, false, "Could not load debug layer"))
						{
							debug->EnableDebugLayer();
						}
						SAVE_RELEASE(debug);
#endif
						if (!IsReady() && m_init.exchange(true) == false)
						{
							IDXGIFactory5* factory;
							hRet = Lib::CreateDXGIFactory2(FACTORY_FLAGS, CE_QUERY_ARGS(&factory));
							if (CloakCheckError(hRet, API::Global::Debug::Error::GRAPHIC_INIT_FACTORY, true)) { return hRet; }
							m_factory = factory;

							API::List<AdapterInfo> adapters;
							GraphicDevice device;
							AdapterInfo adi;
							for (UINT b = 0; factory->EnumAdapters1(b, &adi.Adapter) == S_OK; b++)
							{
								DXGI_ADAPTER_DESC1 desc;
								hRet = adi.Adapter->GetDesc1(&desc);
								if (SUCCEEDED(hRet) && (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0)
								{
									//Check for DX12 support:
									hRet = Lib::D3D12CreateDevice(adi.Adapter, MINIMAL_FEATURE_LEVEL, __uuidof(ID3D12Device), nullptr);
									if (SUCCEEDED(hRet))
									{
										std::string adn = std::to_string(b + 1) + ": " + API::Helper::StringConvert::ConvertToU8(desc.Description);
										for (UINT c = 0; adi.Adapter->EnumOutputs(c, &adi.Output) == S_OK; c++)
										{
											DXGI_OUTPUT_DESC od;
											adi.Output->GetDesc(&od);
											adi.Monitor = od.Monitor;
											adapters.push_back(adi);
											adi.Adapter->AddRef();
											m_adapterNames.push_back(adn + " (Display " + std::to_string(c + 1) + ")");
										}
									}
								}
								adi.Adapter->Release();
							}
							hRet = E_FAIL;
							m_monitor = static_cast<HMONITOR>(INVALID_HANDLE_VALUE);
							size_t adapID = adapters.size();
							if (adapterID < adapters.size()) 
							{ 
								adapID = adapterID;
								hRet = CreateDevice(adapters[adapterID].Adapter, &device);
								if (SUCCEEDED(hRet)) { goto created_device; }
							}
							for (size_t b = 0; b < adapters.size(); b++)
							{
								adapID = b;
								if (b != adapterID)
								{
									hRet = CreateDevice(adapters[b].Adapter, &device);
									if (SUCCEEDED(hRet)) { goto created_device; }
								}
							}
							//WARP (sorftware) device:
							IDXGIAdapter* warp = nullptr;
							hRet = m_factory.load()->EnumWarpAdapter(CE_QUERY_ARGS(&warp));
							if (CloakCheckOK(hRet, API::Global::Debug::Error::GRAPHIC_NO_ADAPTER, true))
							{
								adapID = adapters.size();
								hRet = CreateDevice(warp, &device);
								if (SUCCEEDED(hRet))
								{
									warp->Release();
									goto created_device;
								}
							}
							warp->Release();

						created_device:
							if (SUCCEEDED(hRet))
							{
								if (adapID < adapters.size())
								{
									API::Global::Log::WriteToLog("GPU: " + m_adapterNames[adapID]);
#ifdef _DEBUG
									CloakDebugLog("All aviable gpus:");
									for (size_t a = 0; a < m_adapterNames.size(); a++)
									{
										CloakDebugLog("\t" + std::to_string(a) + ": " + m_adapterNames[a]);
									}
#endif
									m_monitor = adapters[adapID].Monitor;
									IDXGIAdapter3* ad = nullptr;
									if (SUCCEEDED(adapters[adapID].Adapter->QueryInterface(CE_QUERY_ARGS(&ad))))
									{
										//TODO: Query for memory budget and register memory budget change event
										//See https://docs.microsoft.com/en-us/windows/desktop/direct3d12/residency
										
										ad->Release();
									}
									//TODO: Query aviable display modes of adapter adapID
								}
								else
								{
									API::Global::Log::WriteToLog("GPU: Windows Advanced Rasterization Platform");
									m_monitor = static_cast<HMONITOR>(INVALID_HANDLE_VALUE);
								}
							}
							for (size_t b = 0; b < adapters.size(); b++) 
							{ 
								adapters[b].Adapter->Release(); 
								adapters[b].Output->Release(); 
							}
							if (CloakCheckError(hRet, API::Global::Debug::Error::GRAPHIC_NO_DEVICE, true)) { return hRet; }

							m_device = new Device(device);

							//Feature Level:
							{
								D3D12_FEATURE_DATA_FEATURE_LEVELS options;
								options.MaxSupportedFeatureLevel = MINIMAL_FEATURE_LEVEL;
								options.NumFeatureLevels = ARRAYSIZE(SUPPORTED_FEATURE_LEVELS);
								options.pFeatureLevelsRequested = SUPPORTED_FEATURE_LEVELS;
								hRet = device.V0->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &options, sizeof(options));
								if (CloakCheckError(hRet, API::Global::Debug::Error::GRAPHIC_FEATURE_CHECK, true)) { return hRet; }
								m_featureLevel = options.MaxSupportedFeatureLevel;

								API::Global::Log::WriteToLog("Feature Level: " + FEATURE_LEVEL_NAME(m_featureLevel));
							}
							//Hardware Options
							{
								D3D12_FEATURE_DATA_D3D12_OPTIONS options;
								hRet = device.V0->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options));
								if (CloakCheckError(hRet, API::Global::Debug::Error::GRAPHIC_FEATURE_CHECK, true)) { return hRet; }
								m_hardwareSettings.ResourceBindingTier = Casting::CastBackward(options.ResourceBindingTier);
								m_hardwareSettings.TypedUAV = options.TypedUAVLoadAdditionalFormats;
								m_hardwareSettings.TiledResourcesSupport = Casting::CastBackward(options.TiledResourcesTier);
								m_hardwareSettings.CrossAdapterSupport = Casting::CastBackward(options.CrossNodeSharingTier);
								m_hardwareSettings.ConservativeRasterizationSupport = Casting::CastBackward(options.ConservativeRasterizationTier);
								m_hardwareSettings.StandardSwizzle64KBSupported = options.StandardSwizzle64KBSupported == TRUE;
								m_hardwareSettings.ResourceHeapTier = Casting::CastBackward(options.ResourceHeapTier);
								if (m_adapterCount == 1) { m_hardwareSettings.CrossAdapterSupport = API::Rendering::CROSS_ADAPTER_NOT_SUPPORTED; }

								API::Global::Log::WriteToLog("Resource Binding Tier: " + std::to_string(m_hardwareSettings.ResourceBindingTier));
								API::Global::Log::WriteToLog("Tiled Resources Tier: " + std::to_string(m_hardwareSettings.TiledResourcesSupport));
								API::Global::Log::WriteToLog("Typed UAV Formats support: " + std::to_string(m_hardwareSettings.TypedUAV));
								API::Global::Log::WriteToLog("Cross Adapter support: " + std::to_string(m_hardwareSettings.CrossAdapterSupport));
								API::Global::Log::WriteToLog("Conservative Rasterization support: " + std::to_string(m_hardwareSettings.ConservativeRasterizationSupport));
								API::Global::Log::WriteToLog("Standard swizzle 64 kb supported: " + std::to_string(m_hardwareSettings.StandardSwizzle64KBSupported));
								API::Global::Log::WriteToLog("Resource Heap Tier: " + std::to_string(m_hardwareSettings.ResourceHeapTier));
							}
							//Hardware Options 1
							{
								D3D12_FEATURE_DATA_D3D12_OPTIONS1 options;
								HRESULT h = device.V0->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS1, &options, sizeof(options));
								
								m_hardwareSettings.LaneCount = SUCCEEDED(h) ? options.WaveLaneCountMin : 4;
								API::Global::Log::WriteToLog("Wave size: " + std::to_string(m_hardwareSettings.LaneCount));
								if (m_hardwareSettings.LaneCount < 4) { hRet = E_FAIL; }
							}
							//Hardware Options 2
							{
								D3D12_FEATURE_DATA_D3D12_OPTIONS2 options;
								HRESULT h = device.V0->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS2, &options, sizeof(options));

								if (SUCCEEDED(h))
								{
									m_hardwareSettings.AllowDepthBoundsTests = options.DepthBoundsTestSupported == TRUE;
								}
								else
								{
									m_hardwareSettings.AllowDepthBoundsTests = false;
								}

								API::Global::Log::WriteToLog("Depth Bounds Tests: " + std::to_string(m_hardwareSettings.AllowDepthBoundsTests));
							}
							//Hardware Options 3
							{
								D3D12_FEATURE_DATA_D3D12_OPTIONS3 options;
								HRESULT h = device.V0->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS3, &options, sizeof(options));

								if (SUCCEEDED(h))
								{
									m_hardwareSettings.ViewInstanceSupport = Casting::CastBackward(options.ViewInstancingTier);
									m_hardwareSettings.CopyContextAllowTimestampQueries = options.CopyQueueTimestampQueriesSupported == TRUE;
								}
								else
								{
									m_hardwareSettings.ViewInstanceSupport = API::Rendering::VIEW_INSTANCE_NOT_SUPPORTED;
									m_hardwareSettings.CopyContextAllowTimestampQueries = false;
								}
								API::Global::Log::WriteToLog("View Instancing Tier: " + std::to_string(m_hardwareSettings.ViewInstanceSupport));
								API::Global::Log::WriteToLog("Copy Context allow timestamp queries: " + std::to_string(m_hardwareSettings.CopyContextAllowTimestampQueries));
							}
							//Hardware Options 5
							{
								D3D12_FEATURE_DATA_D3D12_OPTIONS5 options;
								HRESULT h = device.V0->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options, sizeof(options));

								if (SUCCEEDED(h))
								{
									m_hardwareSettings.Raytracing.Supported = Casting::CastBackward(options.RaytracingTier);
									m_hardwareSettings.Raytracing.TiledSRVTier3 = options.SRVOnlyTiledResourceTier3 == TRUE;
								}
								else
								{
									m_hardwareSettings.Raytracing.Supported = API::Rendering::RAYTRACING_NOT_SUPPORTED;
									m_hardwareSettings.Raytracing.TiledSRVTier3 = false;
								}

								API::Global::Log::WriteToLog("Raytracing Tier: " + std::to_string(m_hardwareSettings.Raytracing.Supported));
								API::Global::Log::WriteToLog("Raytracing Support Tier 3 Tiled SRV: " + std::to_string(m_hardwareSettings.Raytracing.TiledSRVTier3));
							}
							//Hardware Options 6
							{
								D3D12_FEATURE_DATA_D3D12_OPTIONS6 options;
								HRESULT h = device.V0->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &options, sizeof(options));

								if (SUCCEEDED(h))
								{
									m_hardwareSettings.VariableShadingRate.Supported = Casting::CastBackward(options.VariableShadingRateTier);
									m_hardwareSettings.VariableShadingRate.ImageTileSize = options.ShadingRateImageTileSize;
									if (options.VariableShadingRateTier != D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED) { m_hardwareSettings.VariableShadingRate.MaxShadingRate = options.AdditionalShadingRatesSupported ? API::Rendering::SHADING_RATE_X4 : API::Rendering::SHADING_RATE_X2; }
									else { m_hardwareSettings.VariableShadingRate.MaxShadingRate = API::Rendering::SHADING_RATE_X1; }
								}
								else
								{
									m_hardwareSettings.VariableShadingRate.Supported = API::Rendering::VARIABLE_SHADING_RATE_NOT_SUPPORTED;
									m_hardwareSettings.VariableShadingRate.MaxShadingRate = API::Rendering::SHADING_RATE_X1;
									m_hardwareSettings.VariableShadingRate.ImageTileSize = 0;
								}

								API::Global::Log::WriteToLog("VSR Tier: " + std::to_string(m_hardwareSettings.VariableShadingRate.Supported));
								API::Global::Log::WriteToLog("VSR Image Tile Size: " + std::to_string(m_hardwareSettings.VariableShadingRate.ImageTileSize));
								API::Global::Log::WriteToLog("VSR Max Shading Rate: " + std::to_string(m_hardwareSettings.VariableShadingRate.MaxShadingRate));
							}
							//Shader Model
							{
								D3D12_FEATURE_DATA_SHADER_MODEL options;
								HRESULT h = E_FAIL;
								for (size_t a = 0; a < ARRAYSIZE(SUPPORTED_SHADER_MODELS); a++)
								{
									options.HighestShaderModel = SUPPORTED_SHADER_MODELS[a];
									h = device.V0->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &options, sizeof(options));
									if (SUCCEEDED(h)) { break; }
								}
								if (SUCCEEDED(h))
								{
									switch (options.HighestShaderModel)
									{
										case D3D_SHADER_MODEL_6_5: m_hardwareSettings.ShaderModel = API::Rendering::MODEL_HLSL_6_5; API::Global::Log::WriteToLog("Shader Model: DX 6.5"); break;
										case D3D_SHADER_MODEL_6_4: m_hardwareSettings.ShaderModel = API::Rendering::MODEL_HLSL_6_4; API::Global::Log::WriteToLog("Shader Model: DX 6.4"); break;
										case D3D_SHADER_MODEL_6_3: m_hardwareSettings.ShaderModel = API::Rendering::MODEL_HLSL_6_3; API::Global::Log::WriteToLog("Shader Model: DX 6.3"); break;
										case D3D_SHADER_MODEL_6_2: m_hardwareSettings.ShaderModel = API::Rendering::MODEL_HLSL_6_2; API::Global::Log::WriteToLog("Shader Model: DX 6.2"); break;
										case D3D_SHADER_MODEL_6_1: m_hardwareSettings.ShaderModel = API::Rendering::MODEL_HLSL_6_1; API::Global::Log::WriteToLog("Shader Model: DX 6.1"); break;
										case D3D_SHADER_MODEL_6_0: m_hardwareSettings.ShaderModel = API::Rendering::MODEL_HLSL_6_0; API::Global::Log::WriteToLog("Shader Model: DX 6.0"); break;
										case D3D_SHADER_MODEL_5_1: 
										default: m_hardwareSettings.ShaderModel = API::Rendering::MODEL_HLSL_5_1; API::Global::Log::WriteToLog("Shader Model: DX 5.1"); break;
									}
								}
								else
								{
									m_hardwareSettings.ShaderModel = API::Rendering::MODEL_HLSL_5_1;
									API::Global::Log::WriteToLog("Shader Model: DX 5.1");
								}
							}
							//Root Signature Version
							{
								D3D12_FEATURE_DATA_ROOT_SIGNATURE options;
								options.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
								HRESULT h = device.V0->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &options, sizeof(options));
								if (SUCCEEDED(h) && Lib::D3D12SerializeVersionedRootSignature != nullptr) { m_hardwareSettings.RootVersion = Casting::CastBackward(options.HighestVersion); }
								else { m_hardwareSettings.RootVersion = API::Rendering::ROOT_SIGNATURE_VERSION_1_0; }

								API::Global::Log::WriteToLog("Root Signature Version: " + std::to_string(m_hardwareSettings.RootVersion));
							}
							//Allow Tearing
							{
								BOOL tearing = FALSE;
								HRESULT h = factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &tearing, sizeof(tearing));

								m_hardwareSettings.AllowTearing = SUCCEEDED(h) && tearing == TRUE;
								API::Global::Log::WriteToLog("Allow Tearing: " + std::to_string(m_hardwareSettings.AllowTearing));
							}

							//Prepare multi adapter system:
							m_adapterCount = device.V0->GetNodeCount();
							API::Global::Log::WriteToLog("Adapter Count: " + std::to_string(m_adapterCount));
							if (m_hardwareSettings.CrossAdapterSupport == API::Rendering::CROSS_ADAPTER_NOT_SUPPORTED) { m_adapterCount = 1; }
							m_queues = reinterpret_cast<Queue*>(API::Global::Memory::MemoryHeap::Allocate(sizeof(Queue)*m_adapterCount));
							for (size_t a = 0; a < m_adapterCount; a++)
							{
								::new(&m_queues[a])Queue(m_device, m_adapterCount == 1 ? 0 : static_cast<UINT>(1 << a), a + 1, this, m_hardwareSettings.CrossAdapterSupport);
							}

							m_isReady = true;
						}
						return hRet;
					}
					void CLOAK_CALL_THIS Manager::Update()
					{
						for (size_t a = 0; a < m_adapterCount; a++) { m_queues[a].Update(); }
					}
					HMONITOR CLOAK_CALL_THIS Manager::GetMonitor() const { return m_monitor; }
					void CLOAK_CALL_THIS Manager::ShutdownAdapters()
					{
						if (m_init.exchange(false) == true)
						{
							m_isReady = false;
							for (size_t a = 0; a < m_adapterCount; a++) 
							{
								m_queues[a].Shutdown(); 
							}
						}
					}
					void CLOAK_CALL_THIS Manager::ShutdownMemory()
					{
						for (size_t a = 0; a < m_adapterCount; a++) { m_queues[a].~Queue(); }
						API::Global::Memory::MemoryHeap::Free(m_queues);
					}
					void CLOAK_CALL_THIS Manager::ShutdownDevice()
					{
						SAVE_RELEASE(m_device);
						IDXGIFactory3* factory = m_factory.exchange(nullptr);
						RELEASE(factory);
#if (defined(_DEBUG) && defined(USE_DX_DEBUG))
						HMODULE hMod = GetModuleHandle(L"Dxgidebug.dll");
						if (hMod == NULL) { CloakDebugLog("No DXGIDebug.dll aviable!"); }
						else
						{
							PFN_DXGI_GET_DEBUG_INTERFACE DXGIGetDebugInterface = (PFN_DXGI_GET_DEBUG_INTERFACE)GetProcAddress(hMod, "DXGIGetDebugInterface");
							IDXGIDebug* debug = nullptr;
							HRESULT hRet = DXGIGetDebugInterface(CE_QUERY_ARGS(&debug));
							if (FAILED(hRet)) { CloakDebugLog("Could not create dxgi debug interface!"); }
							else
							{
								debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
							}
							SAVE_RELEASE(debug);
						}
#endif
					}
					void CLOAK_CALL_THIS Manager::InitializeBuffer(In API::Rendering::IResource* dst, In size_t size, In_reads_bytes(size) const void* data, In_opt bool wait)
					{
						ICopyContext* graphic = nullptr;
						IResource* rsc = nullptr;
						if (SUCCEEDED(dst->QueryInterface(CE_QUERY_ARGS(&rsc))))
						{
							m_queues[rsc->GetNodeID() - 1].CreateContext(CE_QUERY_ARGS(&graphic), QUEUE_TYPE_COPY);
#ifdef _DEBUG
							graphic->SetSourceFile(std::string(__FILE__) + " @ " + std::to_string(__LINE__));
#endif
							graphic->WriteBuffer(dst, 0, data, size);
							graphic->CloseAndExecute(wait);
							SAVE_RELEASE(graphic);
						}
						SAVE_RELEASE(rsc);
					}
					void CLOAK_CALL_THIS Manager::InitializeTexture(In API::Rendering::IResource* dst, In UINT numSubresources, In_reads(numSubresources) API::Rendering::SUBRESOURCE_DATA data[], In_opt bool wait) const
					{
						UpdateTextureSubresource(dst, 0, numSubresources, data, wait);
					}
					void CLOAK_CALL_THIS Manager::UpdateTextureSubresource(In API::Rendering::IResource* dst, In UINT firstSubresource, In UINT numSubresources, In_reads(numSubresources) API::Rendering::SUBRESOURCE_DATA data[], In_opt bool wait) const
					{
						ICopyContext* graphic = nullptr;
						IResource* rsc = nullptr;
						if (SUCCEEDED(dst->QueryInterface(CE_QUERY_ARGS(&rsc))))
						{
							m_queues[rsc->GetNodeID() - 1].CreateContext(CE_QUERY_ARGS(&graphic), QUEUE_TYPE_COPY);
#ifdef _DEBUG
							graphic->SetSourceFile(std::string(__FILE__) + " @ " + std::to_string(__LINE__));
#endif
							graphic->WriteBuffer(dst, firstSubresource, numSubresources, data);
							graphic->CloseAndExecute(wait);
							SAVE_RELEASE(graphic);
						}
						SAVE_RELEASE(rsc);
					}
					void CLOAK_CALL_THIS Manager::GenerateViews(In_reads(bufferSize) API::Rendering::IResource* buffer[], In_opt size_t bufferSize, In_opt API::Rendering::VIEW_TYPE Type, In_opt API::Rendering::DescriptorPageType page)
					{
						Resource** buf = NewArray(Resource*, bufferSize);
						size_t ns = 0;
						for (size_t a = 0; a < bufferSize; a++)
						{
							buf[a] = nullptr;
							if (SUCCEEDED(buffer[a]->QueryInterface(CE_QUERY_ARGS(&buf[ns])))) { ns++; }
						}
						Resource::GenerateViews(buf, ns, Type, page);
						for (size_t a = 0; a < ns; a++) { buf[a]->Release(); }
						DeleteArray(buf);
					}
					void CLOAK_CALL_THIS Manager::GenerateViews(In_reads(bufferSize) API::Rendering::IPixelBuffer* buffer[], In_opt size_t bufferSize, In_opt API::Rendering::VIEW_TYPE Type, In_opt API::Rendering::DescriptorPageType page)
					{
						Resource** buf = NewArray(Resource*, bufferSize);
						size_t ns = 0;
						for (size_t a = 0; a < bufferSize; a++)
						{
							buf[a] = nullptr;
							if (SUCCEEDED(buffer[a]->QueryInterface(CE_QUERY_ARGS(&buf[ns])))) { ns++; }
						}
						Resource::GenerateViews(buf, ns, Type, page);
						for (size_t a = 0; a < ns; a++) { buf[a]->Release(); }
						DeleteArray(buf);
					}
					void CLOAK_CALL_THIS Manager::GenerateViews(In_reads(bufferSize) API::Rendering::IColorBuffer* buffer[], In_opt size_t bufferSize, In_opt API::Rendering::VIEW_TYPE Type, In_opt API::Rendering::DescriptorPageType page)
					{
						Resource** buf = NewArray(Resource*, bufferSize);
						size_t ns = 0;
						for (size_t a = 0; a < bufferSize; a++)
						{
							buf[a] = nullptr;
							if (SUCCEEDED(buffer[a]->QueryInterface(CE_QUERY_ARGS(&buf[ns])))) { ns++; }
						}
						Resource::GenerateViews(buf, ns, Type, page);
						for (size_t a = 0; a < ns; a++) { buf[a]->Release(); }
						DeleteArray(buf);
					}
					void CLOAK_CALL_THIS Manager::GenerateViews(In_reads(bufferSize) API::Rendering::IDepthBuffer* buffer[], In_opt size_t bufferSize, In_opt API::Rendering::VIEW_TYPE Type)
					{
						Resource** buf = NewArray(Resource*, bufferSize);
						size_t ns = 0;
						for (size_t a = 0; a < bufferSize; a++)
						{
							buf[a] = nullptr;
							if (SUCCEEDED(buffer[a]->QueryInterface(CE_QUERY_ARGS(&buf[ns])))) { ns++; }
						}
						Resource::GenerateViews(buf, ns, Type);
						for (size_t a = 0; a < ns; a++) { buf[a]->Release(); }
						DeleteArray(buf);
					}
					void CLOAK_CALL_THIS Manager::GenerateViews(In_reads(bufferSize) API::Rendering::IGraphicBuffer* buffer[], In_opt size_t bufferSize, In_opt API::Rendering::DescriptorPageType page)
					{
						Resource** buf = NewArray(Resource*, bufferSize);
						size_t ns = 0;
						for (size_t a = 0; a < bufferSize; a++)
						{
							buf[a] = nullptr;
							if (SUCCEEDED(buffer[a]->QueryInterface(CE_QUERY_ARGS(&buf[ns])))) { ns++; }
						}
						Resource::GenerateViews(buf, ns, API::Rendering::VIEW_TYPE::CBV, page);
						for (size_t a = 0; a < ns; a++) { buf[a]->Release(); }
						DeleteArray(buf);
					}
					void CLOAK_CALL_THIS Manager::GenerateViews(In_reads(bufferSize) API::Rendering::IStructuredBuffer* buffer[], In_opt size_t bufferSize, In_opt API::Rendering::DescriptorPageType page)
					{
						Resource** buf = NewArray(Resource*, bufferSize);
						size_t ns = 0;
						for (size_t a = 0; a < bufferSize; a++)
						{
							buf[a] = nullptr;
							if (SUCCEEDED(buffer[a]->QueryInterface(CE_QUERY_ARGS(&buf[ns])))) { ns++; }
						}
						Resource::GenerateViews(buf, ns, API::Rendering::VIEW_TYPE::CBV, page);
						for (size_t a = 0; a < ns; a++) { buf[a]->Release(); }
						DeleteArray(buf);
					}
					void CLOAK_CALL_THIS Manager::GenerateViews(In_reads(bufferSize) API::Rendering::IConstantBuffer* buffer[], In_opt size_t bufferSize, In_opt API::Rendering::DescriptorPageType page)
					{
						Resource** buf = NewArray(Resource*, bufferSize);
						size_t ns = 0;
						for (size_t a = 0; a < bufferSize; a++)
						{
							buf[a] = nullptr;
							if (SUCCEEDED(buffer[a]->QueryInterface(CE_QUERY_ARGS(&buf[ns])))) { ns++; }
						}
						Resource::GenerateViews(buf, ns, API::Rendering::VIEW_TYPE::CBV, page);
						for (size_t a = 0; a < ns; a++) { buf[a]->Release(); }
						DeleteArray(buf);
					}
					void CLOAK_CALL_THIS Manager::GenerateViews(In_reads(bufferSize) IColorBuffer* buffer[], In_opt size_t bufferSize, In_opt API::Rendering::VIEW_TYPE Type, In_opt API::Rendering::DescriptorPageType page)
					{
						Resource** buf = NewArray(Resource*, bufferSize);
						size_t ns = 0;
						for (size_t a = 0; a < bufferSize; a++)
						{
							buf[a] = nullptr;
							if (SUCCEEDED(buffer[a]->QueryInterface(CE_QUERY_ARGS(&buf[ns])))) { ns++; }
						}
						Resource::GenerateViews(buf, ns, Type, page);
						for (size_t a = 0; a < ns; a++) { buf[a]->Release(); }
						DeleteArray(buf);
					}
#ifdef _DEBUG
					void CLOAK_CALL_THIS Manager::BeginContextFile(In API::Rendering::CONTEXT_TYPE type, In uint32_t nodeID, In REFIID riid, Outptr void** ptr, In const std::string& file) const
					{
						UINT node;
						QUEUE_TYPE qtype;
						GetNodeInfo(type, nodeID, &qtype, &node);
						IContext* c = nullptr;
						m_queues[node].CreateContext(CE_QUERY_ARGS(&c), qtype);
						c->SetSourceFile(file);
						c->QueryInterface(riid, ptr);
						c->Release();

					}
#else
					void CLOAK_CALL_THIS Manager::BeginContext(In API::Rendering::CONTEXT_TYPE type, In uint32_t nodeID, In REFIID riid, Outptr void** ptr) const
					{
						UINT node;
						QUEUE_TYPE qtype;
						GetNodeInfo(type, nodeID, &qtype, &node);
						m_queues[node].CreateContext(riid, ptr, qtype);
					}
#endif
					uint64_t CLOAK_CALL_THIS Manager::FlushExecutions(In API::Rendering::CONTEXT_TYPE type, In uint32_t nodeID, In_opt bool wait)
					{
						UINT node;
						QUEUE_TYPE qtype;
						GetNodeInfo(type, nodeID, &qtype, &node);
						return m_queues[node].FlushExecutions(nullptr, wait, qtype);
					}
					API::Rendering::SHADER_MODEL CLOAK_CALL_THIS Manager::GetShaderModel() const { return m_hardwareSettings.ShaderModel; }
					bool CLOAK_CALL_THIS Manager::IsReady() const { return m_isReady; }
					uint64_t CLOAK_CALL_THIS Manager::IncrementFence(In API::Rendering::CONTEXT_TYPE type, In uint32_t nodeID)
					{
						UINT node;
						QUEUE_TYPE qtype;
						GetNodeInfo(type, nodeID, &qtype, &node);
						return m_queues[node].IncrementFence(qtype);
					}
					bool CLOAK_CALL_THIS Manager::IsFenceComplete(In API::Rendering::CONTEXT_TYPE type, In uint32_t nodeID, In uint64_t val)
					{
						UINT node;
						QUEUE_TYPE qtype;
						GetNodeInfo(type, nodeID, &qtype, &node);
						return m_queues[node].IsFenceComplete(qtype, val);
					}
					bool CLOAK_CALL_THIS Manager::WaitForFence(In API::Rendering::CONTEXT_TYPE type, In uint32_t nodeID, In uint64_t val)
					{
						UINT node;
						QUEUE_TYPE qtype;
						GetNodeInfo(type, nodeID, &qtype, &node);
						return m_queues[node].WaitForFence(qtype, val);
					}
					void CLOAK_CALL_THIS Manager::WaitForGPU()
					{
						uint64_t* vals = reinterpret_cast<uint64_t*>(alloca(sizeof(uint64_t) * m_adapterCount * 2));
						for (size_t a = 0; a < m_adapterCount; a++)
						{
							vals[(a << 1) + 0] = m_queues[a].IncrementFence(Impl::Rendering::QUEUE_TYPE_COPY);
							vals[(a << 1) + 1] = m_queues[a].IncrementFence(Impl::Rendering::QUEUE_TYPE_DIRECT);
						}
						for (size_t a = 0; a < m_adapterCount; a++)
						{
							m_queues[a].WaitForFence(Impl::Rendering::QUEUE_TYPE_COPY, vals[(a << 1) + 0]);
							m_queues[a].WaitForFence(Impl::Rendering::QUEUE_TYPE_DIRECT, vals[(a << 1) + 1]);
						}
					}
					bool CLOAK_CALL_THIS Manager::SupportMultiThreadedRendering() const { return true; }
					bool CLOAK_CALL_THIS Manager::GetFactory(In REFIID riid, Outptr void** ptr) const 
					{ 
						if (IsReady())
						{
							return SUCCEEDED(m_factory.load()->QueryInterface(riid, ptr));
						}
						return false;
					}
					bool CLOAK_CALL_THIS Manager::UpdateFactory()
					{
						if (IsReady())
						{
							IDXGIFactory5* factory = m_factory.load();
							if (factory->IsCurrent() == FALSE)
							{
								IDXGIFactory5* nf = nullptr;
								HRESULT hRet = Lib::CreateDXGIFactory2(FACTORY_FLAGS, CE_QUERY_ARGS(&nf));
								if (SUCCEEDED(hRet)) 
								{ 
									m_factory = nf; 
									SAVE_RELEASE(factory);
									return true;
								}
							}
						}
						return false;
					}
					Ret_maybenull IDevice* CLOAK_CALL_THIS Manager::GetDevice() const
					{
						if (m_device != nullptr && IsReady())
						{
							m_device->AddRef();
							return m_device;
						}
						return nullptr;
					}
					API::Rendering::IPipelineState* CLOAK_CALL_THIS Manager::CreatePipelineState() const
					{
						return PipelineState::Create(this);
					}
					API::Rendering::IQuery* CLOAK_CALL_THIS Manager::CreateQuery(In API::Rendering::CONTEXT_TYPE contextType, In uint32_t nodeID, In API::Rendering::QUERY_TYPE type) const
					{
						UINT node;
						QUEUE_TYPE qtype;
						GetNodeInfo(contextType, nodeID, &qtype, &node);
						return new Query(m_queues[node].GetQueryPool(), qtype, type);
					}
					IRootSignature* CLOAK_CALL_THIS Manager::CreateRootSignature() const
					{
						return new RootSignature();
					}
					IColorBuffer* CLOAK_CALL_THIS Manager::CreateColorBuffer(In ISwapChain* base, In uint32_t textureNum) const
					{
						return ColorBuffer::CreateFromSwapChain(base, textureNum);
					}
					IColorBuffer* CLOAK_CALL_THIS Manager::CreateColorBuffer(In const API::Rendering::TEXTURE_DESC& desc, In size_t nodeID, In UINT nodeMask) const
					{
						return ColorBuffer::Create(desc, nodeID, nodeMask);
					}
					IDepthBuffer* CLOAK_CALL_THIS Manager::CreateDepthBuffer(In const API::Rendering::DEPTH_DESC& desc, In size_t nodeID, In UINT nodeMask) const
					{
						return DepthBuffer::Create(desc, nodeID, nodeMask);
					}
					CE::RefPointer<ISwapChain> CLOAK_CALL_THIS Manager::CreateSwapChain(In const API::Global::Graphic::Settings& gset, Out_opt HRESULT* hRet)
					{
						CE::RefPointer<ISwapChain> sc = CE::RefPointer<ISwapChain>::Construct<SwapChain>(this, m_hardwareSettings);
						HRESULT hr = sc->Recreate(gset);
						if (hRet != nullptr) { *hRet = hr; }
						return sc;
					}
					bool CLOAK_CALL_THIS Manager::EnumerateAdapter(In size_t id, Out std::string* name) const
					{
						if (name != nullptr && id < m_adapterNames.size())
						{
							*name = m_adapterNames[id];
							return true;
						}
						return false;
					}
					size_t CLOAK_CALL_THIS Manager::GetNodeCount() const { return m_adapterCount; }
					const API::Rendering::Hardware& CLOAK_CALL_THIS Manager::GetHardwareSetting() const { return m_hardwareSettings; }
					

					namespace APIManager {
						CLOAK_INTERFACE_BASIC IColorOrder{
							public:
								virtual size_t CLOAK_CALL_THIS GetByteSize() const = 0;
								virtual void CLOAK_CALL_THIS CopyColor(In const API::Helper::Color::RGBA& color, Out uint8_t* bytes) = 0;
								virtual void CLOAK_CALL_THIS CopyBytes(In const uint8_t* bytes, Out API::Helper::Color::RGBA* color) = 0;
								virtual void CLOAK_CALL_THIS Delete() = 0;
						};
						template<typename type, size_t expLen, bool sign> struct ToType {
							static type CLOAK_CALL Call(In float f)
							{
								int64_t e;
								int64_t s;
								const int64_t maxEXP = (1ULL << expLen) - 1;
								if (f == 0)
								{
									e = 0;
									s = 0;
								}
								else if (f != f)//NaN
								{
									e = maxEXP;
									s = (1 << ((sizeof(type) << 3) - (expLen + 1))) - 1;
								}
								else if (std::abs(f) == std::numeric_limits<float>::infinity())//Infinity
								{
									e = maxEXP;
									s = 0;
								}
								else
								{
									int ex;
									float m = std::frexp(std::abs(f), &ex) * 2;
									e = (static_cast<int64_t>(ex) - 1) + ((1i64 << (expLen - 1)) - 1);
									if (e <= 0)//Denormalized
									{
										m /= (1ULL << (-e));
										e = 0;
										s = static_cast<int64_t>((1i64 << ((sizeof(type) << 3) - (expLen + 1))) * static_cast<double>(m));
									}
									else if (e >= maxEXP)//Overflow
									{
										e = maxEXP;
										s = 0;
									}
									else
									{
										s = static_cast<int64_t>((1i64 << ((sizeof(type) << 3) - (expLen + 1))) * static_cast<double>(m - 1));
									}
								}
								const type sr = static_cast<type>((std::copysign(1.0f, f) < 0) ? (1ULL << ((sizeof(type) << 3) - 1)) : 0);
								const type er = static_cast<type>(e << ((sizeof(type) << 3) - (expLen + 1)));
								return static_cast<type>(sr | er | s);
							}
						};
						template<typename type, bool sign> struct ToType<type, 0, sign> {
							static type CLOAK_CALL Call(In float col)
							{
								if (col != col) { return 0; }
								else if (col == std::numeric_limits<float>::infinity()) { return std::numeric_limits<type>::max(); }
								return static_cast<type>(CE_MIN(1, CE_MAX(0, col))*std::numeric_limits<type>::max());
							}
						};
						template<typename type> struct ToType<type, 0, true> {
							static type CLOAK_CALL Call(In float col)
							{
								if (col != col) { return 0; }
								else if (col == std::numeric_limits<float>::infinity()) { return std::numeric_limits<type>::max(); }
								return static_cast<type>(CE_MIN(1, CE_MAX(-1, col))*std::numeric_limits<type>::max());
							}
						};
						template<> struct ToType<float, 8, true> {
							static float CLOAK_CALL Call(In float col) { return col; }
						};
						template<typename type, size_t expLen, bool sign> struct FromType {
							static float CLOAK_CALL Call(In type h)
							{
								const bool s = (h & (1ULL << ((sizeof(type) << 3) - 1))) != 0;
								const uint64_t e = (h >> ((sizeof(type) << 3) - (expLen + 1))) & ((1ULL << expLen) - 1);
								const uint64_t m = h & ((1ULL << ((sizeof(type) << 3) - (expLen + 1))) - 1);
								if (e == 0 && m == 0) { return 0; }
								if (e == (1ULL << expLen) - 1)
								{
									if (m == 0) { return std::numeric_limits<float>::infinity(); }
									return std::numeric_limits<float>::quiet_NaN();
								}
								const float x = (static_cast<float>(m) / (1ULL << ((sizeof(type) << 3) - (expLen + 1)))) + (e == 0 ? 0 : 1);
								const int shift = static_cast<int>(e - ((1ULL << (expLen - 1)) - 1));
								return std::copysign(std::ldexpf(x, shift), s ? -1.0f : 1.0f);
							}
						};
						template<typename type, bool sign> struct FromType<type, 0, sign> {
							static float CLOAK_CALL Call(In type col)
							{
								const float t = static_cast<float>(static_cast<long double>(col) / std::numeric_limits<type>::max());
								return CE_MIN(1, CE_MAX(0, t));
							}
						};
						template<typename type> struct FromType<type, 0, true> {
							static float CLOAK_CALL Call(In type col)
							{
								const float t = static_cast<float>(static_cast<long double>(col) / std::numeric_limits<type>::max());
								return CE_MIN(1, CE_MAX(-1, t));
							}
						};
						template<> struct FromType<float, 8, true> {
							static float CLOAK_CALL Call(In float col) { return col; }
						};
						template<typename type, size_t inR, size_t inG, size_t inB, size_t inA, size_t expLen> class ColorOrder : public IColorOrder {
							public:
								static constexpr size_t ByteSize = (inR > 0 ? sizeof(type) : 0) + (inG > 0 ? sizeof(type) : 0) + (inB > 0 ? sizeof(type) : 0) + (inA > 0 ? sizeof(type) : 0);
								size_t CLOAK_CALL_THIS GetByteSize() const override { return ByteSize; }
								void CLOAK_CALL_THIS CopyColor(In const API::Helper::Color::RGBA& color, Out uint8_t* bytes)  override
								{
									type* t = reinterpret_cast<type*>(bytes);
									COLOR_ORDER_SET(inR, color, R, type);
									COLOR_ORDER_SET(inG, color, G, type);
									COLOR_ORDER_SET(inB, color, B, type);
									COLOR_ORDER_SET(inA, color, A, type);
								}
								void CLOAK_CALL_THIS CopyBytes(In const uint8_t* bytes, Out API::Helper::Color::RGBA* color) override
								{
									const type* t = reinterpret_cast<const type*>(bytes);
									COLOR_ORDER_GET(inR, color, R, type);
									COLOR_ORDER_GET(inG, color, G, type);
									COLOR_ORDER_GET(inB, color, B, type);
									COLOR_ORDER_GET(inA, color, A, type);
								}
								void* CLOAK_CALL operator new(In size_t s) { return API::Global::Memory::MemoryPool::Allocate(s); }
								void CLOAK_CALL operator delete(In void* ptr) { return API::Global::Memory::MemoryPool::Free(ptr); }
								void CLOAK_CALL_THIS Delete() { delete this; }
						};
						inline IColorOrder* GetColorOrderByFormat(In API::Rendering::Format format)
						{
							switch (format)
							{
								case CloakEngine::API::Rendering::Format::R32G32B32A32_FLOAT:
									return new ColorOrder<float, 1, 2, 3, 4, 8>();
								case CloakEngine::API::Rendering::Format::R32G32B32A32_UINT:
									return new ColorOrder<uint32_t, 1, 2, 3, 4, 0>();
								case CloakEngine::API::Rendering::Format::R32G32B32A32_SINT:
									return new ColorOrder<int32_t, 1, 2, 3, 4, 0>();
								case CloakEngine::API::Rendering::Format::R32G32B32_FLOAT:
									return new ColorOrder<float, 1, 2, 3, 0, 8>();
								case CloakEngine::API::Rendering::Format::R32G32B32_UINT:
									return new ColorOrder<uint32_t, 1, 2, 3, 0, 0>();
								case CloakEngine::API::Rendering::Format::R32G32B32_SINT:
									return new ColorOrder<int32_t, 1, 2, 3, 0, 0>();
								case CloakEngine::API::Rendering::Format::R16G16B16A16_UNORM:
									return new ColorOrder<uint16_t, 1, 2, 3, 4, 0>();
								case CloakEngine::API::Rendering::Format::R16G16B16A16_UINT:
									return new ColorOrder<uint16_t, 1, 2, 3, 4, 0>();
								case CloakEngine::API::Rendering::Format::R16G16B16A16_SNORM:
									return new ColorOrder<int16_t, 1, 2, 3, 4, 0>();
								case CloakEngine::API::Rendering::Format::R16G16B16A16_SINT:
									return new ColorOrder<int16_t, 1, 2, 3, 4, 0>();
								case CloakEngine::API::Rendering::Format::R16G16B16A16_FLOAT:
									return new ColorOrder<uint16_t, 1, 2, 3, 4, 5>();
								case CloakEngine::API::Rendering::Format::R32G32_FLOAT:
									return new ColorOrder<float, 1, 2, 0, 0, 8>();
								case CloakEngine::API::Rendering::Format::R32G32_UINT:
									return new ColorOrder<uint32_t, 1, 2, 0, 0, 0>();
								case CloakEngine::API::Rendering::Format::R32G32_SINT:
									return new ColorOrder<int32_t, 1, 2, 0, 0, 0>();
								case CloakEngine::API::Rendering::Format::R8G8B8A8_UNORM:
									return new ColorOrder<uint8_t, 1, 2, 3, 4, 0>();
								case CloakEngine::API::Rendering::Format::R8G8B8A8_UNORM_SRGB:
									return new ColorOrder<uint8_t, 1, 2, 3, 4, 0>();
								case CloakEngine::API::Rendering::Format::R8G8B8A8_UINT:
									return new ColorOrder<uint8_t, 1, 2, 3, 4, 0>();
								case CloakEngine::API::Rendering::Format::R8G8B8A8_SNORM:
									return new ColorOrder<int8_t, 1, 2, 3, 4, 0>();
								case CloakEngine::API::Rendering::Format::R8G8B8A8_SINT:
									return new ColorOrder<int8_t, 1, 2, 3, 4, 0>();
								case CloakEngine::API::Rendering::Format::R16G16_UNORM:
									return new ColorOrder<uint16_t, 1, 2, 0, 0, 0>();
								case CloakEngine::API::Rendering::Format::R16G16_UINT:
									return new ColorOrder<uint16_t, 1, 2, 0, 0, 0>();
								case CloakEngine::API::Rendering::Format::R16G16_SNORM:
									return new ColorOrder<int16_t, 1, 2, 0, 0, 0>();
								case CloakEngine::API::Rendering::Format::R16G16_SINT:
									return new ColorOrder<int16_t, 1, 2, 0, 0, 0>();
								case CloakEngine::API::Rendering::Format::R16G16_FLOAT:
									return new ColorOrder<uint16_t, 1, 2, 0, 0, 5>();
								case CloakEngine::API::Rendering::Format::R32_FLOAT:
									return new ColorOrder<float, 1, 0, 0, 0, 8>();
								case CloakEngine::API::Rendering::Format::R32_UINT:
									return new ColorOrder<uint32_t, 1, 0, 0, 0, 0>();
								case CloakEngine::API::Rendering::Format::R32_SINT:
									return new ColorOrder<int32_t, 1, 0, 0, 0, 0>();
								case CloakEngine::API::Rendering::Format::R8G8_UNORM:
									return new ColorOrder<uint8_t, 1, 2, 0, 0, 0>();
								case CloakEngine::API::Rendering::Format::R8G8_UINT:
									return new ColorOrder<uint8_t, 1, 2, 0, 0, 0>();
								case CloakEngine::API::Rendering::Format::R8G8_SNORM:
									return new ColorOrder<int8_t, 1, 2, 0, 0, 0>();
								case CloakEngine::API::Rendering::Format::R8G8_SINT:
									return new ColorOrder<int8_t, 1, 2, 0, 0, 0>();
								case CloakEngine::API::Rendering::Format::R16_UNORM:
									return new ColorOrder<uint16_t, 1, 0, 0, 0, 0>();
								case CloakEngine::API::Rendering::Format::R16_UINT:
									return new ColorOrder<uint16_t, 1, 0, 0, 0, 0>();
								case CloakEngine::API::Rendering::Format::R16_SNORM:
									return new ColorOrder<int16_t, 1, 0, 0, 0, 0>();
								case CloakEngine::API::Rendering::Format::R16_SINT:
									return new ColorOrder<int16_t, 1, 0, 0, 0, 0>();
								case CloakEngine::API::Rendering::Format::R16_FLOAT:
									return new ColorOrder<uint16_t, 1, 0, 0, 0, 5>();
								case CloakEngine::API::Rendering::Format::R8_UNORM:
									return new ColorOrder<uint8_t, 1, 0, 0, 0, 0>();
								case CloakEngine::API::Rendering::Format::R8_UINT:
									return new ColorOrder<uint8_t, 1, 0, 0, 0, 0>();
								case CloakEngine::API::Rendering::Format::R8_SNORM:
									return new ColorOrder<int8_t, 1, 0, 0, 0, 0>();
								case CloakEngine::API::Rendering::Format::R8_SINT:
									return new ColorOrder<int8_t, 1, 0, 0, 0, 0>();
								case CloakEngine::API::Rendering::Format::A8_UNORM:
									return new ColorOrder<uint8_t, 0, 0, 0, 1, 0>();
								case CloakEngine::API::Rendering::Format::B8G8R8A8_UNORM:
									return new ColorOrder<uint8_t, 3, 2, 1, 4, 0>();
								case CloakEngine::API::Rendering::Format::B8G8R8X8_UNORM:
									return new ColorOrder<uint8_t, 3, 2, 1, 4, 0>();
								case CloakEngine::API::Rendering::Format::B8G8R8A8_UNORM_SRGB:
									return new ColorOrder<uint8_t, 3, 2, 1, 4, 0>();
								case CloakEngine::API::Rendering::Format::B8G8R8X8_UNORM_SRGB:
									return new ColorOrder<uint8_t, 3, 2, 1, 4, 0>();
								default:
									break;
							}
							return nullptr;
						}
						inline bool CLOAK_CALL NextMipMap(Inout uint32_t* W, Inout uint32_t* H)
						{
							if (*W == 1 && *H == 1) { return false; }
							if (*W > 1) { *W >>= 1; }
							if (*H > 1) { *H >>= 1; }
							return true;
						}
						inline void CLOAK_CALL CopyDataToSRD(Inout API::Rendering::SUBRESOURCE_DATA* srd, In uint32_t W, In uint32_t H, In API::Rendering::Format format, In const API::Helper::Color::RGBA* data)
						{
							IColorOrder* ord = GetColorOrderByFormat(format);
							if (CloakCheckOK(ord != nullptr, API::Global::Debug::Error::GRAPHIC_FORMAT_UNSUPPORTED, true))
							{
								const size_t bys = ord->GetByteSize();
								srd->RowPitch = W*bys;
								srd->SlicePitch = H*srd->RowPitch;
								uint8_t* bytes = NewArray(uint8_t, srd->SlicePitch);
								srd->pData = bytes;
								size_t byp = 0;
								for (uint32_t y = 0; y < H; y++)
								{
									for (uint32_t x = 0; x < W; x++, byp += bys)
									{
										const size_t p = (static_cast<size_t>(y)*W) + x;
										ord->CopyColor(data[p], &bytes[byp]);
									}
								}
							}
							ord->Delete();
						}
					}

					API::Rendering::IColorBuffer* CLOAK_CALL_THIS Manager::CreateColorBuffer(In API::Rendering::CONTEXT_TYPE contextType, In uint32_t nodeID, In const API::Rendering::TEXTURE_DESC& desc, In const API::Helper::Color::RGBA* const* colorData) const
					{
						IColorBuffer* res = nullptr;
						API::Rendering::SUBRESOURCE_DATA* srd = nullptr;
						size_t srdSize = 0;
						uint32_t usedSrdSize = 0;
						uint32_t W = desc.Width;
						uint32_t H = desc.Height;

						const size_t nid = GetNodeID(contextType, nodeID);
						const UINT nodeMask = m_adapterCount == 1 ? 0 : static_cast<UINT>(1 << nid);

						switch (desc.Type)
						{
							case API::Rendering::TEXTURE_TYPE::TEXTURE:
							{
								srdSize = desc.Texture.MipMaps;
								srd = NewArray(API::Rendering::SUBRESOURCE_DATA, srdSize);
								uint32_t lastMipMap = 0;
								for (size_t a = 0; a < desc.Texture.MipMaps; a++)
								{
									APIManager::CopyDataToSRD(&(srd[a]), W, H, desc.Format, colorData[a]);
									lastMipMap = static_cast<uint32_t>(a);
									if (APIManager::NextMipMap(&W, &H) == false) { break; }
								}
								usedSrdSize = lastMipMap + 1;
								res = CreateColorBuffer(desc, nid, nodeMask);
								break;
							}
							case API::Rendering::TEXTURE_TYPE::TEXTURE_ARRAY:
							{
								srdSize = desc.TextureArray.Images*desc.TextureArray.MipMaps;
								srd = NewArray(API::Rendering::SUBRESOURCE_DATA, srdSize);
								uint32_t lastMip = desc.TextureArray.MipMaps - 1;
								for (size_t a = 0; a < desc.TextureArray.Images; a++)
								{
									uint32_t iW = W;
									uint32_t iH = H;
									uint32_t lmip = 0;
									for (uint32_t b = 0; b < lastMip + 1; b++)
									{
										lmip = b;
										if (APIManager::NextMipMap(&iW, &iH) == false) { break; }
									}
									lastMip = CE_MIN(lastMip, lmip);
								}
								usedSrdSize = desc.TextureArray.Images*(lastMip + 1);
								for (size_t a = 0; a < desc.TextureArray.Images; a++)
								{
									uint32_t iW = W;
									uint32_t iH = H;
									for (size_t b = 0; b < lastMip + 1; b++)
									{
										size_t pd = b + (a*desc.TextureArray.MipMaps);
										size_t pi = b + (a*(lastMip + 1));
										APIManager::CopyDataToSRD(&srd[pi], iW, iH, desc.Format, colorData[pd]);
										APIManager::NextMipMap(&iW, &iH);
									}
								}
								res = CreateColorBuffer(desc, nid, nodeMask);
								break;
							}
							case API::Rendering::TEXTURE_TYPE::TEXTURE_CUBE:
							{
								srdSize = 6 * desc.TextureCube.MipMaps;
								uint32_t lastMip = desc.TextureCube.MipMaps - 1;
								for (size_t a = 0; a < 6; a++)
								{
									uint32_t iW = W;
									uint32_t iH = H;
									uint32_t lmip = 0;
									for (uint32_t b = 0; b < lastMip + 1; b++)
									{
										lmip = b;
										if (APIManager::NextMipMap(&iW, &iH) == false) { break; }
									}
									lastMip = CE_MIN(lastMip, lmip);
								}
								usedSrdSize = 6 * (lastMip + 1);
								srd = NewArray(API::Rendering::SUBRESOURCE_DATA, srdSize);
								for (size_t a = 0; a < srdSize; a++)
								{
									APIManager::CopyDataToSRD(&srd[a], W, H, desc.Format, colorData[a]);
								}
								res = CreateColorBuffer(desc, nid, nodeMask);
								break;
							}
							default:
								break;
						}
						InitializeTexture(res, static_cast<UINT>(usedSrdSize), srd, true);
						for (size_t a = 0; a < srdSize; a++) { DeleteArray(reinterpret_cast<const uint8_t*>(srd[a].pData)); }
						DeleteArray(srd);
						return res;
					}
					API::Rendering::IColorBuffer* CLOAK_CALL_THIS Manager::CreateColorBuffer(In API::Rendering::CONTEXT_TYPE contextType, In uint32_t nodeID, In const API::Rendering::TEXTURE_DESC& desc, In API::Helper::Color::RGBA** colorData) const
					{
						IColorBuffer* res = nullptr;
						API::Rendering::SUBRESOURCE_DATA* srd = nullptr;
						size_t srdSize = 0;
						uint32_t usedSrdSize = 0;
						uint32_t W = desc.Width;
						uint32_t H = desc.Height;

						const size_t nid = GetNodeID(contextType, nodeID);
						const UINT nodeMask = m_adapterCount == 1 ? 0 : static_cast<UINT>(1 << nid);

						switch (desc.Type)
						{
							case API::Rendering::TEXTURE_TYPE::TEXTURE:
							{
								srdSize = desc.Texture.MipMaps;
								srd = NewArray(API::Rendering::SUBRESOURCE_DATA, srdSize);
								uint32_t lastMipMap = 0;
								for (size_t a = 0; a < desc.Texture.MipMaps; a++)
								{
									APIManager::CopyDataToSRD(&(srd[a]), W, H, desc.Format, colorData[a]);
									lastMipMap = static_cast<uint32_t>(a);
									if (APIManager::NextMipMap(&W, &H) == false) { break; }
								}
								usedSrdSize = lastMipMap + 1;
								res = CreateColorBuffer(desc, nid, nodeMask);
								break;
							}
							case API::Rendering::TEXTURE_TYPE::TEXTURE_ARRAY:
							{
								srdSize = desc.TextureArray.Images*desc.TextureArray.MipMaps;
								srd = NewArray(API::Rendering::SUBRESOURCE_DATA, srdSize);
								uint32_t lastMip = desc.TextureArray.MipMaps - 1;
								for (size_t a = 0; a < desc.TextureArray.Images; a++)
								{
									uint32_t iW = W;
									uint32_t iH = H;
									uint32_t lmip = 0;
									for (uint32_t b = 0; b < lastMip + 1; b++)
									{
										lmip = b;
										if (APIManager::NextMipMap(&iW, &iH) == false) { break; }
									}
									lastMip = CE_MIN(lastMip, lmip);
								}
								usedSrdSize = desc.TextureArray.Images*(lastMip + 1);
								for (size_t a = 0; a < desc.TextureArray.Images; a++)
								{
									uint32_t iW = W;
									uint32_t iH = H;
									for (size_t b = 0; b < lastMip + 1; b++)
									{
										size_t pd = b + (a*desc.TextureArray.MipMaps);
										size_t pi = b + (a*(lastMip + 1));
										APIManager::CopyDataToSRD(&srd[pi], iW, iH, desc.Format, colorData[pd]);
										APIManager::NextMipMap(&iW, &iH);
									}
								}
								res = CreateColorBuffer(desc, nid, nodeMask);
								break;
							}
							case API::Rendering::TEXTURE_TYPE::TEXTURE_CUBE:
							{
								srdSize = 6 * desc.TextureCube.MipMaps;
								uint32_t lastMip = desc.TextureCube.MipMaps - 1;
								for (size_t a = 0; a < 6; a++)
								{
									uint32_t iW = W;
									uint32_t iH = H;
									uint32_t lmip = 0;
									for (uint32_t b = 0; b < lastMip + 1; b++)
									{
										lmip = b;
										if (APIManager::NextMipMap(&iW, &iH) == false) { break; }
									}
									lastMip = CE_MIN(lastMip, lmip);
								}
								usedSrdSize = 6 * (lastMip + 1);
								srd = NewArray(API::Rendering::SUBRESOURCE_DATA, usedSrdSize);
								for (size_t a = 0; a < 6; a++)
								{
									uint32_t iW = W;
									uint32_t iH = H;

									for (uint32_t b = 0; b <= lastMip; b++)
									{
										const size_t p = (a*(lastMip + 1)) + b;
										CLOAK_ASSUME(p < usedSrdSize);
										APIManager::CopyDataToSRD(&srd[p], iW, iH, desc.Format, colorData[p]);
										const bool r = APIManager::NextMipMap(&iW, &iH);
										CLOAK_ASSUME(r == true || b == lastMip);
									}
								}
								res = CreateColorBuffer(desc, nid, nodeMask);
								break;
							}
							default:
								break;
						}
						InitializeTexture(res, static_cast<UINT>(usedSrdSize), srd, true);
						for (size_t a = 0; a < srdSize; a++) { DeleteArray(reinterpret_cast<const uint8_t*>(srd[a].pData)); }
						DeleteArray(srd);
						return res;
					}
					API::Rendering::IColorBuffer* CLOAK_CALL_THIS Manager::CreateColorBuffer(In API::Rendering::CONTEXT_TYPE contextType, In uint32_t nodeID, In const API::Rendering::TEXTURE_DESC& desc) const
					{
						const size_t nid = GetNodeID(contextType, nodeID);
						const UINT nodeMask = m_adapterCount == 1 ? 0 : static_cast<UINT>(1 << nid);
						return CreateColorBuffer(desc, nid, nodeMask);
					}
					API::Rendering::IDepthBuffer* CLOAK_CALL_THIS Manager::CreateDepthBuffer(In API::Rendering::CONTEXT_TYPE contextType, In uint32_t nodeID, In const API::Rendering::DEPTH_DESC& desc) const
					{
						const size_t nid = GetNodeID(contextType, nodeID);
						const UINT nodeMask = m_adapterCount == 1 ? 0 : static_cast<UINT>(1 << nid);
						return CreateDepthBuffer(desc, nid, nodeMask);
					}
					API::Rendering::IVertexBuffer* CLOAK_CALL_THIS Manager::CreateVertexBuffer(In API::Rendering::CONTEXT_TYPE contextType, In uint32_t nodeID, In UINT vertexCount, In UINT vertexSize, In_reads(vertexCount*vertexSize) const void* data) const
					{
						const size_t nid = GetNodeID(contextType, nodeID);
						const UINT nodeMask = m_adapterCount == 1 ? 0 : static_cast<UINT>(1 << nid);
						return VertexBuffer::Create(nodeMask, nid, vertexCount, vertexSize, data);
					}
					API::Rendering::IIndexBuffer* CLOAK_CALL_THIS Manager::CreateIndexBuffer(In API::Rendering::CONTEXT_TYPE contextType, In uint32_t nodeID, In UINT size, In_reads(size) const uint16_t* data) const
					{
						const size_t nid = GetNodeID(contextType, nodeID);
						const UINT nodeMask = m_adapterCount == 1 ? 0 : static_cast<UINT>(1 << nid);
						return IndexBuffer::Create(nodeMask, nid, size, false, data);
					}
					API::Rendering::IIndexBuffer* CLOAK_CALL_THIS Manager::CreateIndexBuffer(In API::Rendering::CONTEXT_TYPE contextType, In uint32_t nodeID, In UINT size, In_reads(size) const uint32_t* data) const
					{
						const size_t nid = GetNodeID(contextType, nodeID);
						const UINT nodeMask = m_adapterCount == 1 ? 0 : static_cast<UINT>(1 << nid);
						return IndexBuffer::Create(nodeMask, nid, size, true, data);
					}
					API::Rendering::IConstantBuffer* CLOAK_CALL_THIS Manager::CreateConstantBuffer(In API::Rendering::CONTEXT_TYPE contextType, In uint32_t nodeID, In UINT elementSize, In_reads_opt(elementSize) const void* data) const
					{
						const size_t nid = GetNodeID(contextType, nodeID);
						const UINT nodeMask = m_adapterCount == 1 ? 0 : static_cast<UINT>(1 << nid);
						return ConstantBuffer::Create(nodeMask, nid, elementSize, data);
					}
					API::Rendering::IStructuredBuffer* CLOAK_CALL_THIS Manager::CreateStructuredBuffer(In API::Rendering::CONTEXT_TYPE contextType, In uint32_t nodeID, In UINT elementCount, In UINT elementSize, In_reads(elementSize) const void* data, In_opt bool counterBuffer) const
					{
						const size_t nid = GetNodeID(contextType, nodeID);
						const UINT nodeMask = m_adapterCount == 1 ? 0 : static_cast<UINT>(1 << nid);
						return StructuredBuffer::Create(nodeMask, nid, counterBuffer, elementCount, elementSize, data);
					}
					API::Rendering::IStructuredBuffer* CLOAK_CALL_THIS Manager::CreateStructuredBuffer(In API::Rendering::CONTEXT_TYPE contextType, In uint32_t nodeID, In UINT elementCount, In UINT elementSize, In_opt bool counterBuffer, In_reads_opt(elementSize) const void* data) const
					{
						const size_t nid = GetNodeID(contextType, nodeID);
						const UINT nodeMask = m_adapterCount == 1 ? 0 : static_cast<UINT>(1 << nid);
						return StructuredBuffer::Create(nodeMask, nid, counterBuffer, elementCount, elementSize, data);
					}
					API::Rendering::IByteAddressBuffer* CLOAK_CALL_THIS Manager::CreateByteAddressBuffer(In API::Rendering::CONTEXT_TYPE contextType, In uint32_t nodeID, In_opt UINT count) const
					{
						const size_t nid = GetNodeID(contextType, nodeID);
						const UINT nodeMask = m_adapterCount == 1 ? 0 : static_cast<UINT>(1 << nid);
						return ByteAddressBuffer::Create(nodeMask, nid, count);
					}
					size_t CLOAK_CALL_THIS Manager::GetDataPos(In const API::Rendering::TEXTURE_DESC& desc, In uint32_t MipMap, In_opt uint32_t Image) const
					{
						uint32_t mipMaps = 0;
						uint32_t images = 0;
						switch (desc.Type)
						{
							case API::Rendering::TEXTURE_TYPE::TEXTURE:
								mipMaps = desc.Texture.MipMaps;
								images = 0;
								break;
							case API::Rendering::TEXTURE_TYPE::TEXTURE_ARRAY:
								mipMaps = desc.TextureArray.MipMaps;
								images = desc.TextureArray.Images;
								break;
							case API::Rendering::TEXTURE_TYPE::TEXTURE_CUBE:
								mipMaps = desc.TextureCube.MipMaps;
								images = 6;
								break;
							default:
								break;
						}
						return CE_MIN(mipMaps, MipMap) + (CE_MIN(Image, images)*mipMaps);
					}
					size_t CLOAK_CALL_THIS Manager::GetNodeID(In API::Rendering::CONTEXT_TYPE type, In uint32_t nodeID) const
					{
						UINT r = 0;
						GetNodeInfo(type, nodeID, nullptr, &r);
						return r;
					}
					IQueue* CLOAK_CALL_THIS Manager::GetQueueByNodeID(In size_t nodeID) const 
					{
						assert(nodeID < m_adapterCount);
						return &m_queues[nodeID];
					}
					ISingleQueue* CLOAK_CALL_THIS Manager::GetQueue(In API::Rendering::CONTEXT_TYPE type, In uint32_t nodeID) const
					{
						UINT node;
						QUEUE_TYPE qtype;
						GetNodeInfo(type, nodeID, &qtype, &node);
						return m_queues[node].GetSingleQueue(qtype);
					}
					void CLOAK_CALL_THIS Manager::ReadBuffer(In size_t dstByteSize, Out_writes(dstByteSize) void* dst, In API::Rendering::IPixelBuffer* src, In_opt UINT subresource) const
					{
						if (dstByteSize > 0 && dst != nullptr)
						{
							CopyContext* graphic = nullptr;
							Resource* srcRc = nullptr;
							if (SUCCEEDED(src->QueryInterface(CE_QUERY_ARGS(&srcRc))))
							{
								m_queues[srcRc->GetNodeID() - 1].CreateContext(CE_QUERY_ARGS(&graphic), QUEUE_TYPE_COPY);
#ifdef _DEBUG
								graphic->SetSourceFile(std::string(__FILE__) + " @ " + std::to_string(__LINE__));
#endif
								API::Rendering::ReadBack temp = graphic->ReadBack(src, subresource);
								graphic->CloseAndExecute(true);
								graphic->Release();

								const size_t rs = temp.GetSize();
								memcpy(dst, temp.GetData(), std::min(rs, dstByteSize));
								if (rs < dstByteSize) { memset(dst, 0, dstByteSize - rs); }
								srcRc->Release();
							}
						}
					}
					void CLOAK_CALL_THIS Manager::ReadBuffer(In size_t dstByteSize, Out_writes(dstByteSize) void* dst, In API::Rendering::IGraphicBuffer* src) const
					{
						if (dstByteSize > 0 && dst != nullptr)
						{
							CopyContext* graphic = nullptr;
							Resource* srcRc = nullptr;
							if (SUCCEEDED(src->QueryInterface(CE_QUERY_ARGS(&srcRc))))
							{
								m_queues[srcRc->GetNodeID() - 1].CreateContext(CE_QUERY_ARGS(&graphic), QUEUE_TYPE_COPY);
#ifdef _DEBUG
								graphic->SetSourceFile(std::string(__FILE__) + " @ " + std::to_string(__LINE__));
#endif
								API::Rendering::ReadBack temp = graphic->ReadBack(src);
								graphic->CloseAndExecute(true);
								graphic->Release();

								const size_t rs = temp.GetSize();
								memcpy(dst, temp.GetData(), std::min(rs, dstByteSize));
								if (rs < dstByteSize) { memset(dst, 0, dstByteSize - rs); }
								srcRc->Release();
							}
						}
					}
					void CLOAK_CALL_THIS Manager::ReadBuffer(In size_t dstSize, Out_writes(dstSize) API::Helper::Color::RGBA* color, In API::Rendering::IColorBuffer* src, In_opt UINT subresource) const
					{
						if (dstSize > 0 && color != nullptr)
						{
							CopyContext* graphic = nullptr;
							Resource* srcRc = nullptr;
							if (SUCCEEDED(src->QueryInterface(CE_QUERY_ARGS(&srcRc))))
							{
								APIManager::IColorOrder* ord = APIManager::GetColorOrderByFormat(src->GetFormat());
								if (CloakCheckOK(ord != nullptr, API::Global::Debug::Error::GRAPHIC_FORMAT_UNSUPPORTED, true))
								{
									m_queues[srcRc->GetNodeID() - 1].CreateContext(CE_QUERY_ARGS(&graphic), QUEUE_TYPE_COPY);
#ifdef _DEBUG
									graphic->SetSourceFile(std::string(__FILE__) + " @ " + std::to_string(__LINE__));
#endif
									API::Rendering::ReadBack temp = graphic->ReadBack(src, subresource);
									graphic->CloseAndExecute(true);
									graphic->Release();

									uint8_t* data = reinterpret_cast<uint8_t*>(temp.GetData());
									const size_t bs = static_cast<size_t>(temp.GetSize() / ord->GetByteSize());
									const size_t ds = bs < dstSize ? bs : dstSize;
									for (size_t a = 0; a < ds; a++) { ord->CopyBytes(&data[a*ord->GetByteSize()], &color[a]); }
									for (size_t a = bs; a < dstSize; a++) { color[a] = API::Helper::Color::RGBA(0, 0, 0, 0); }
								}
								ord->Delete();
							}
							srcRc->Release();
						}
					}
					ID3D12CommandQueue* CLOAK_CALL_THIS Manager::GetCommandQueue() const
					{
						return m_queues[0].GetCommandQueue(QUEUE_TYPE_DIRECT);
					}

					Success(return == true) bool CLOAK_CALL_THIS Manager::iQueryInterface(In REFIID riid, Outptr void** ptr)
					{
						if (riid == __uuidof(API::Rendering::Manager_v1::IManager)) { *ptr = (API::Rendering::Manager_v1::IManager*)this; return true; }
						else RENDERING_QUERY_IMPL(ptr, riid, Manager, Manager_v1, SavePtr);
					}
					void CLOAK_CALL_THIS Manager::GetNodeInfo(In API::Rendering::CONTEXT_TYPE type, In uint32_t nodeID, Out QUEUE_TYPE* qtype, Out UINT* node) const
					{
						UINT b = 0;
						switch (type)
						{
							case CloakEngine::API::Rendering::CONTEXT_TYPE_GRAPHIC:
								if (qtype != nullptr) { *qtype = QUEUE_TYPE_DIRECT; }
								b = 0;
								break;
							case CloakEngine::API::Rendering::CONTEXT_TYPE_GRAPHIC_COPY:
								if (qtype != nullptr) { *qtype = QUEUE_TYPE_COPY; }
								b = 0;
								break;
							case CloakEngine::API::Rendering::CONTEXT_TYPE_PHYSICS:
								if (qtype != nullptr) { *qtype = QUEUE_TYPE_DIRECT; }
								b = 1;
								break;
							case CloakEngine::API::Rendering::CONTEXT_TYPE_PHYSICS_COPY:
								if (qtype != nullptr) { *qtype = QUEUE_TYPE_COPY; }
								b = 1;
								break;
							default:
								break;
						}
						if (node != nullptr)
						{
							if (m_adapterCount <= 1) { *node = 0; }
							else
							{
								UINT c = b + static_cast<UINT>(nodeID * CONTEXT_TYPE_COUNT);
								if (m_adapterCount % CONTEXT_TYPE_COUNT == 0) { c += c / m_adapterCount; }
								*node = c % m_adapterCount;
							}
						}
					}
				}
			}
		}
	}
}
#endif