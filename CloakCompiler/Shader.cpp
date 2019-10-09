#include "stdafx.h"
#include "CloakCompiler/Shader.h"

#include "Engine/TempHandler.h"
#include "Engine/LibWriter.h"

#include <d3dcompiler.h>
#include <dxcapi.h>

#include <spirv_cross/spirv.hpp>
#include <spirv_cross/spirv_glsl.hpp>
#include <spirv_cross/spirv_hlsl.hpp>
#include <spirv_cross/spirv_msl.hpp>

#include <comdef.h>
#include <vector>
#include <algorithm>
#include <codecvt>
#include <fstream>
#include <sstream>

#pragma comment(lib,"dxcompiler.lib")

namespace CloakCompiler {
	namespace API {
		namespace Shader {
			namespace Shader_v1003 {
				template<typename T> struct INFO_AND_USAGE { T Value; USAGE_MASK Usage; };

				constexpr LPCWSTR DEFAULT_SHADER_VERSION = L"_6_4";
				constexpr OPTION OPTION_OPTIMIZATION_MASK = OPTION_OPTIMIZATION_LEVEL_0 | OPTION_OPTIMIZATION_LEVEL_1 | OPTION_OPTIMIZATION_LEVEL_2 | OPTION_OPTIMIZATION_LEVEL_3;
				constexpr INFO_AND_USAGE<uint8_t> WAVE_SIZES[] = { 
					{ 4,	USAGE_MASK_WAVE_SIZE_4	 }, 
					{ 32,	USAGE_MASK_WAVE_SIZE_32	 }, 
					{ 64,	USAGE_MASK_WAVE_SIZE_64	 }, 
					{ 128,	USAGE_MASK_WAVE_SIZE_128 },
				};
				constexpr INFO_AND_USAGE<uint8_t> MSAA_COUNT[] = { 
					{ 1, USAGE_MASK_MSAA_1 }, 
					{ 2, USAGE_MASK_MSAA_2 },
					{ 4, USAGE_MASK_MSAA_4 },
					{ 8, USAGE_MASK_MSAA_8 },
				};

				constexpr const char* SHADER_GET_START = "void __fastcall Get";
				constexpr const char* SHADER_GET_ARGS_MAIN = "ShaderModel model, std::uint32_t waveSize, bool tessellation, std::uint32_t msaaCount, bool complex, const void** result, std::size_t* resultLength";
				constexpr const char* SHADER_GET_VARIANT_MAIN = "In const CloakEngine::API::Rendering::SHADER_MODEL& model, In_opt std::uint32_t waveSize = 4, In_opt bool tessellation = false, In_opt std::uint32_t msaaCount = 1, In_opt bool complex = false";
				constexpr const char* SHADER_GET_VARIANT_MAIN_CALL = "waveSize, tessellation, msaaCount, complex";
				//Allways pairs of parameter list and how to pass them to main function
				constexpr const char* SHADER_GET_ARGS_VARIANTS[] = { 
					"In const CloakEngine::API::Rendering::Hardware& hardware, In_opt bool tessellation = false, In_opt std::uint32_t msaaCount = 1, In_opt bool complex = false",									"hardware.ShaderModel, hardware.LaneCount, tessellation, msaaCount, complex",
					"In const CloakEngine::API::Rendering::SHADER_MODEL& model, In const CloakEngine::API::Global::Graphic::Settings& settings, In_opt std::uint32_t waveSize = 4, In_opt bool complex = false",		"model, waveSize, settings.Tessellation, static_cast<uint32_t>(settings.MSAA), complex",
					"In const CloakEngine::API::Rendering::Hardware& hardware, In const CloakEngine::API::Global::Graphic::Settings& settings, In_opt bool complex = false",										"hardware.ShaderModel, hardware.LaneCount, settings.Tessellation, static_cast<uint32_t>(settings.MSAA), complex",
				};
				constexpr const char* SHADER_GET_START_VARAINTS = "CloakEngine::API::Rendering::SHADER_CODE CLOAK_CALL Get";

				static_assert(ARRAYSIZE(SHADER_GET_ARGS_VARIANTS) % 2 == 0, "Shader getter variants must be pairs of parameter list and parameter pass");

				struct ShaderCountInfo {
					uint64_t Count;
					USAGE_MASK Usage;
				};
				struct ShaderPassCount {
					union {
						struct {
							//Must be in sync with ShaderPass enumeration
							ShaderCountInfo VS;
							ShaderCountInfo HS;
							ShaderCountInfo DS;
							ShaderCountInfo GS;
							ShaderCountInfo PS;
							ShaderCountInfo CS;
						};
						ShaderCountInfo Pass[6];
					};
					ShaderCountInfo Summed;
					CLOAK_CALL ShaderPassCount() 
					{ 
						for (size_t a = 0; a < ARRAYSIZE(Pass); a++) { Pass[a].Count = 0; Pass[a].Usage = USAGE_MASK_NEVER; } 
						Summed.Count = 0;
						Summed.Usage = USAGE_MASK_NEVER;
					}
				};
				static_assert(sizeof(ShaderPassCount) == sizeof(ShaderCountInfo) * (ARRAYSIZE(ShaderPassCount::Pass) + 1), "Pass array must be the same size as supported passes");

				CLOAK_FORCEINLINE std::string CLOAK_CALL IntToHex(In uint64_t i, In_opt size_t minLen = 0)
				{
					std::stringstream r;
					size_t p = 0;
					do {
						switch (i % 16)
						{
							case 0x0: r << '0'; break;
							case 0x1: r << '1'; break;
							case 0x2: r << '2'; break;
							case 0x3: r << '3'; break;
							case 0x4: r << '4'; break;
							case 0x5: r << '5'; break;
							case 0x6: r << '6'; break;
							case 0x7: r << '7'; break;
							case 0x8: r << '8'; break;
							case 0x9: r << '9'; break;
							case 0xA: r << 'A'; break;
							case 0xB: r << 'B'; break;
							case 0xC: r << 'C'; break;
							case 0xD: r << 'D'; break;
							case 0xE: r << 'E'; break;
							case 0xF: r << 'F'; break;
							default:
								break;
						}
						i /= 16;
						p++;
					} while (i != 0);
					for (; p < minLen; p++) { r << '0'; }
					return r.str();
				}
				CLOAK_FORCEINLINE std::string CLOAK_CALL ComposeShaderName(In CE::Rendering::SHADER_MODEL model, In SHADER_TYPE shader, In const Configuration& config)
				{
					std::string r = "SB";
					switch (model.Language)
					{
						case CloakEngine::API::Rendering::SHADER_DXIL: r += "DI"; break;
						case CloakEngine::API::Rendering::SHADER_SPIRV: r += "SV"; break;
						case CloakEngine::API::Rendering::SHADER_HLSL: r += "HS"; break;
						case CloakEngine::API::Rendering::SHADER_GLSL: r += "GS"; break;
						case CloakEngine::API::Rendering::SHADER_ESSL: r += "GE"; break;
						case CloakEngine::API::Rendering::SHADER_MSL_MacOS: r += "MM"; break;
						case CloakEngine::API::Rendering::SHADER_MSL_iOS: r += "MI"; break;
						default: CLOAK_ASSUME(false); break;
					}
					r += IntToHex(model.Major) + "V" + IntToHex(model.Minor) + "P" + IntToHex(model.Patch);
					if (IsFlagSet(shader, SHADER_TYPE_TESSELLATION)) { r += config.GetTessellation() == true ? "T" : "F"; }
					if (IsFlagSet(shader, SHADER_TYPE_WAVESIZE)) { r += "W" + IntToHex(config.GetWaveSize(), 2); }
					if (IsFlagSet(shader, SHADER_TYPE_MSAA)) { r += "M" + IntToHex(config.GetMSAA()); }
					if (IsFlagSet(shader, SHADER_TYPE_COMPLEX)) { r += config.GetComplex() == true ? "T" : "F"; }
					return r;
				}
				CLOAK_FORCEINLINE void CLOAK_CALL ToString(Inout std::string* s, In char32_t c)
				{
					if ((c & 0x1F0000) != 0)
					{
						s->push_back(0xF0 | ((c >> 18) & 0x07));
						s->push_back(0x80 | ((c >> 12) & 0x3F));
						s->push_back(0x80 | ((c >> 6) & 0x3F));
						s->push_back(0x80 | ((c >> 0) & 0x3F));
					}
					else if ((c & 0x00F800) != 0)
					{
						s->push_back(0xE0 | ((c >> 12) & 0x0F));
						s->push_back(0x80 | ((c >> 6) & 0x3F));
						s->push_back(0x80 | ((c >> 0) & 0x3F));
					}
					else if ((c & 0x000780) != 0)
					{
						s->push_back(0xC0 | ((c >> 6) & 0x1F));
						s->push_back(0x80 | ((c >> 0) & 0x3F));
					}
					else
					{
						s->push_back(c & 0x7F);
					}
				}

				struct Define {
					std::wstring Name;
					std::wstring Value;
				};
				class ResponseHandler {
					private:
						struct Page {
							Page* Next;
						};
						struct PageList {
							PageList* Next;
							size_t Size;
						};

						static constexpr size_t COUNT_PER_PAGE = 128;
						static constexpr size_t PAGE_SIZE = sizeof(PageList) + sizeof(Page) + (COUNT_PER_PAGE * sizeof(Response));

						class LocalList {
							private:
								CE::RefPointer<CE::Helper::ISyncSection> m_sync;
								PageList* m_list;
								Page* m_curPage;
								Page* m_cache;
								size_t m_pos;

								Page* CLOAK_CALL_THIS GetPage()
								{
									Page* p = m_cache;
									if (p != nullptr) { m_cache = p->Next; return p; }
									p = reinterpret_cast<Page*>(CE::Global::Memory::MemoryHeap::Allocate(PAGE_SIZE));
									return p;
								}
								void CLOAK_CALL_THIS DestroyPage(In Page* p, In size_t count)
								{
									Response* rsp = reinterpret_cast<Response*>(reinterpret_cast<uintptr_t>(p) + sizeof(Page));
									for (size_t a = 0; a < count; a++) { rsp[a].~Response(); }
								}
							public:
								CLOAK_CALL LocalList()
								{
									m_list = nullptr;
									m_curPage = nullptr;
									m_cache = nullptr;
									m_pos = 0;

									CREATE_INTERFACE(CE_QUERY_ARGS(&m_sync));
								}
								CLOAK_CALL ~LocalList()
								{
									Page* p = m_cache;
									while (p != nullptr)
									{
										Page* n = p->Next;
										CE::Global::Memory::MemoryHeap::Free(p);
										p = n;
									}
								}
								void CLOAK_CALL_THIS SendResponse(In const Response& response)
								{
									CE::Helper::Lock lock(m_sync);
									if (m_list == nullptr)
									{
										m_list = reinterpret_cast<PageList*>(GetPage());
										m_list->Next = nullptr;
										m_list->Size = 0;
										m_curPage = reinterpret_cast<Page*>(reinterpret_cast<uintptr_t>(m_list) + sizeof(PageList));
										m_curPage->Next = nullptr;
										m_pos = 0;
									}
									if (m_pos == COUNT_PER_PAGE) 
									{
										m_list->Size += COUNT_PER_PAGE;
										Page* np = GetPage();
										m_curPage->Next = np;
										m_curPage = np;
										m_curPage->Next = nullptr;
										m_pos = 0;
									}
									Response* rsp = reinterpret_cast<Response*>(reinterpret_cast<uintptr_t>(m_curPage) + sizeof(Page));
									::new(&rsp[m_pos++])Response(response);
								}
								PageList* CLOAK_CALL_THIS FlipList()
								{
									CE::Helper::Lock lock(m_sync);
									PageList* r = m_list;
									if (r != nullptr) { r->Size += m_pos; }
									m_list = nullptr;
									return r;
								}
								void CLOAK_CALL_THIS CacheList(In PageList* list)
								{
									size_t remCount = list->Size;
									Page* n = reinterpret_cast<Page*>(reinterpret_cast<uintptr_t>(list) + sizeof(PageList));
									size_t delCount = min(remCount, COUNT_PER_PAGE);
									DestroyPage(n, delCount);
									remCount -= delCount;
									n = n->Next;
									Page* f = reinterpret_cast<Page*>(list);
									f->Next = n;
									if (n != nullptr)
									{
										while (n->Next != nullptr)
										{
											delCount = min(remCount, COUNT_PER_PAGE);
											DestroyPage(n, delCount);
											remCount -= delCount;
											n = n->Next;
										}
										delCount = min(remCount, COUNT_PER_PAGE);
										DestroyPage(n, delCount);
										remCount -= delCount;
									}
									else { n = f; }
									CE::Helper::Lock lock(m_sync);
									n->Next = m_cache;
									m_cache = f;
								}
						};

						std::function<void(const Response&)> m_response;
						std::atomic<bool> m_hasTask;
						CE::Global::Task m_task;
						LocalList* m_lists;
						size_t m_listCount;
					public:
						CLOAK_CALL ResponseHandler(In std::function<void(const Response&)> response) : m_response(response), m_task(nullptr), m_hasTask(false)
						{
							m_listCount = CE::Global::GetExecutionThreadCount();
							m_lists = reinterpret_cast<LocalList*>(CE::Global::Memory::MemoryHeap::Allocate(sizeof(LocalList) * m_listCount));
							for (size_t a = 0; a < m_listCount; a++) { ::new(&m_lists[a])LocalList(); }
						}
						CLOAK_CALL ~ResponseHandler()
						{
							m_task.WaitForExecution();
							for (size_t a = 0; a < m_listCount; a++) { m_lists[a].~LocalList(); }
							CE::Global::Memory::MemoryHeap::Free(m_lists);
						}

						void CLOAK_CALL ExecuteResponse()
						{
							if (m_hasTask.exchange(false) == true)
							{
								for (size_t a = 0; a < m_listCount; a++)
								{
									PageList* pl = m_lists[a].FlipList();
									if (pl != nullptr)
									{
										if (m_response)
										{
											Page* p = reinterpret_cast<Page*>(reinterpret_cast<uintptr_t>(pl) + sizeof(PageList));
											size_t rs = pl->Size;
											do {
												Response* rsp = reinterpret_cast<Response*>(reinterpret_cast<uintptr_t>(p) + sizeof(Page));
												const size_t s = min(rs, COUNT_PER_PAGE);
												for (size_t a = 0; a < s; a++) { m_response(rsp[a]); }
												p = p->Next;
												rs -= s;
											} while (p != nullptr);
										}
										m_lists[a].CacheList(pl);
									}
								}
							}
						}

						void CLOAK_CALL SendResponse(In size_t threadID, In const Response& response)
						{
							m_lists[threadID].SendResponse(response);
							if (m_hasTask.exchange(true) == false)
							{
								CE::Global::Task t = [this](In size_t threadID) { this->ExecuteResponse(); };
								t.AddDependency(m_task);
								m_task = t;
								m_task.Schedule(CE::Global::Threading::Priority::High, threadID);
							}
						}
				};
				class Compiler {
					private:
						class Intern {
							private:
								CE::RefPointer<IDxcLibrary> m_lib;
								CE::RefPointer<IDxcCompiler> m_compiler;
							public:
								CLOAK_CALL Intern()
								{
									DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&m_lib));
									DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_compiler));
								}
								const CE::RefPointer<IDxcLibrary>& CLOAK_CALL_THIS GetLibrary() const { return m_lib; }
								const CE::RefPointer<IDxcCompiler>& CLOAK_CALL_THIS GetCompiler() const { return m_compiler; }
						};

						Intern* m_intern;
						size_t m_size;
						ResponseHandler* m_response;
					public:
						CLOAK_CALL Compiler(In ResponseHandler* response) : m_response(response)
						{
							m_size = CE::Global::GetExecutionThreadCount();
							m_intern = reinterpret_cast<Intern*>(CE::Global::Memory::MemoryHeap::Allocate(sizeof(Intern) * m_size));
							for (size_t a = 0; a < m_size; a++) { ::new(&m_intern[a])Intern(); }
						}
						CLOAK_CALL ~Compiler()
						{
							for (size_t a = 0; a < m_size; a++) { m_intern[a].~Intern(); }
							CE::Global::Memory::MemoryHeap::Free(m_intern);
						}
						const CE::RefPointer<IDxcLibrary>& CLOAK_CALL_THIS GetLibrary(In size_t threadID) const { return m_intern[threadID].GetLibrary(); }
						const CE::RefPointer<IDxcCompiler>& CLOAK_CALL_THIS GetCompiler(In size_t threadID) const { return m_intern[threadID].GetCompiler(); }

						void CLOAK_CALL SendResponse(In size_t threadID, In size_t shaderID, In ShaderPass type, In CE::Rendering::SHADER_MODEL version, In const Configuration& config, In Status status, In_opt std::string msg = "") const
						{
							Response r;
							r.Status = status;
							r.Message = msg;
							r.Shader.ID = shaderID;
							r.Shader.Pass = type;
							r.Shader.Model = version;
							r.Shader.Configuration = config;
							m_response->SendResponse(threadID, r);
						}
						CLOAK_FORCEINLINE void CLOAK_CALL SendResponse(In size_t threadID, In size_t shaderID, In ShaderPass type, In CE::Rendering::SHADER_MODEL version, In const Configuration& config, In std::string msg) const { SendResponse(In threadID, shaderID, type, version, config, Status::INFO, msg); }
						CLOAK_FORCEINLINE void CLOAK_CALL SendResponse(In size_t threadID, In size_t shaderID, In const SHADER_NAME& shader, In const Configuration& config, In Status status, In_opt std::string msg = "") const { SendResponse(In threadID, shaderID, shader.Pass, shader.Model, config, status, msg); }
						CLOAK_FORCEINLINE void CLOAK_CALL SendResponse(In size_t threadID, In size_t shaderID, In const SHADER_NAME& shader, In const Configuration& config, In std::string msg) const { SendResponse(In threadID, shaderID, shader.Pass, shader.Model, config, Status::INFO, msg); }
				};
				class Blob {
					private:
						void* m_data;
						size_t m_size;
						std::atomic<uint64_t> m_ref;

						CLOAK_CALL ~Blob() { CE::Global::Memory::MemoryHeap::Free(m_data); }
					public:
						CLOAK_CALL Blob(In size_t size, In const void* data, In_opt uint64_t rv = 1) : m_ref(rv), m_size(size)
						{
							m_data = CE::Global::Memory::MemoryHeap::Allocate(size);
							memcpy(m_data, data, size);
						}
						CLOAK_CALL Blob(In const CE::RefPointer<IDxcBlob>& blob, In_opt uint64_t rv = 1) : Blob(blob->GetBufferSize(), blob->GetBufferPointer(), rv) {}
						CLOAK_CALL Blob(In const CE::RefPointer<Blob>& blob, In_opt uint64_t rv = 1) : Blob(blob->GetBufferSize(), blob->GetBufferPointer(), rv) {}
						CLOAK_CALL Blob(In const CE::RefPointer<ID3DBlob>& blob, In_opt uint64_t rv = 1) : Blob(blob->GetBufferSize(), blob->GetBufferPointer(), rv) {}
						const void* CLOAK_CALL_THIS GetBufferPointer() const { return m_data; }
						size_t CLOAK_CALL_THIS GetBufferSize() const { return m_size; }
						uint64_t CLOAK_CALL_THIS AddRef() { return m_ref.fetch_add(1) + 1; }
						uint64_t CLOAK_CALL_THIS Release()
						{
							const uint64_t r = m_ref.fetch_sub(1);
							if (r == 1) { delete this; }
							else if (r == 0) { m_ref = 0; return 0; }
							return r - 1;
						}

						void* CLOAK_CALL operator new(In size_t size) { return CE::Global::Memory::MemoryPool::Allocate(size); }
						void CLOAK_CALL operator delete(In void* ptr) { CE::Global::Memory::MemoryPool::Free(ptr); }
				};
				class IncludeHandler : public IDxcIncludeHandler {
					private:
						std::atomic<ULONG> m_ref;
						const CE::Files::UFI m_srcPath;
						const Compiler* m_compiler;
						const size_t m_threadID;
					public:
						CLOAK_CALL IncludeHandler(In size_t threadID, In const Compiler* compiler, In const CE::Files::UFI& ufi, In_opt ULONG rv = 1) : m_srcPath(ufi, 1), m_threadID(threadID), m_compiler(compiler), m_ref(rv) {}

						HRESULT STDMETHODCALLTYPE LoadSource(_In_ LPCWSTR pFilename, _COM_Outptr_result_maybenull_ IDxcBlob** ppIncludeSource) override
						{
							if (ppIncludeSource == nullptr) { return E_INVALIDARG; }
							*ppIncludeSource = nullptr;

							CE::RefPointer<CE::Files::IReader> read = nullptr;
							CREATE_INTERFACE(CE_QUERY_ARGS(&read));
							if (read->SetTarget(CE::Files::UFI(m_srcPath, pFilename), CE::Files::CompressType::NONE, true) > 0)
							{
								std::string srcFile = "";
								while (read->IsAtEnd() == false) { ToString(&srcFile, static_cast<char32_t>(read->ReadBits(32))); }

								CE::RefPointer<IDxcBlobEncoding> source = nullptr;
								HRESULT hRet = m_compiler->GetLibrary(m_threadID)->CreateBlobWithEncodingOnHeapCopy(srcFile.c_str(), static_cast<UINT32>(srcFile.length()), CP_UTF8, &source);
								if (SUCCEEDED(hRet))
								{
									*ppIncludeSource = static_cast<IDxcBlob*>(source);
									source->AddRef();
									return S_OK;
								}
							}
							return ERROR_NOT_FOUND;
						}
						HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) override
						{
							if (ppvObject == nullptr) { return E_INVALIDARG; }
							*ppvObject = nullptr;
							if (riid == __uuidof(IDxcIncludeHandler)) { *ppvObject = (IDxcIncludeHandler*)this; AddRef(); return S_OK; }
							else if (riid == __uuidof(IUnknown)) { *ppvObject = (IUnknown*)this; AddRef(); return S_OK; }
							return E_NOINTERFACE;
						}
						ULONG STDMETHODCALLTYPE AddRef() override
						{
							return m_ref.fetch_add(1) + 1;
						}
						ULONG STDMETHODCALLTYPE Release() override
						{
							ULONG r = m_ref.fetch_sub(1);
							if (r == 1) { delete this; }
							else if (r == 0) { m_ref = 0; return 0; }
							return r - 1;
						}

						void* CLOAK_CALL operator new(In size_t size) { return CE::Global::Memory::MemoryLocal::Allocate(size); }
						void CLOAK_CALL operator delete(In void* ptr) { CE::Global::Memory::MemoryLocal::Free(ptr); }
				};
				bool CLOAK_CALL FindShader(In const SHADER_DESC& shader, In const CE::Rendering::SHADER_MODEL& model, In ShaderPass target, In const Configuration& config, Out const SHADER_NAME** result)
				{
					size_t f = shader.ShaderCount;
					for (size_t a = 0; a < shader.ShaderCount; a++)
					{
						if (shader.Shaders[a].Pass == target && shader.Shaders[a].Model == model)
						{
							if (IsFlagSet(shader.Type, SHADER_TYPE_TESSELLATION))
							{
								if (IsFlagSet(shader.Shaders[a].Use, config.GetTessellation() == true ? USAGE_MASK_TESSELLATION_ON : USAGE_MASK_TESSELLATION_OFF) == false) { continue; }
							}
							if (IsFlagSet(shader.Type, SHADER_TYPE_WAVESIZE))
							{
								const uint8_t waveSize = config.GetWaveSize();
								if (waveSize == 4 && IsFlagSet(shader.Shaders[a].Use, USAGE_MASK_WAVE_SIZE_4) == false) { continue; }
								else if (waveSize == 32 && IsFlagSet(shader.Shaders[a].Use, USAGE_MASK_WAVE_SIZE_32) == false) { continue; }
								else if (waveSize == 64 && IsFlagSet(shader.Shaders[a].Use, USAGE_MASK_WAVE_SIZE_64) == false) { continue; }
								else if (waveSize == 128 && IsFlagSet(shader.Shaders[a].Use, USAGE_MASK_WAVE_SIZE_128) == false) { continue; }
							}
							if (IsFlagSet(shader.Type, SHADER_TYPE_MSAA))
							{
								const uint8_t msaaCount = config.GetMSAA();
								if (msaaCount == 1 && IsFlagSet(shader.Shaders[a].Use, USAGE_MASK_MSAA_1) == false) { continue; }
								else if (msaaCount == 2 && IsFlagSet(shader.Shaders[a].Use, USAGE_MASK_MSAA_2) == false) { continue; }
								else if (msaaCount == 4 && IsFlagSet(shader.Shaders[a].Use, USAGE_MASK_MSAA_4) == false) { continue; }
								else if (msaaCount == 8 && IsFlagSet(shader.Shaders[a].Use, USAGE_MASK_MSAA_8) == false) { continue; }
							}
							if (IsFlagSet(shader.Type, SHADER_TYPE_COMPLEX))
							{
								if (IsFlagSet(shader.Shaders[a].Use, config.GetComplex() == true ? USAGE_MASK_COMPLEX_ON : USAGE_MASK_COMPLEX_OFF) == false) { continue; }
							}


							if (f < shader.ShaderCount) { return false; } //We found multiple shader that we could use, but we can only allow one! (as they would all have the same name)
							f = a;
						}
					}
					if (f < shader.ShaderCount) 
					{ 
						*result = &shader.Shaders[f]; 
					}
					else { *result = nullptr; }
					return true;
				}
				bool CLOAK_CALL IsValidConfig(In SHADER_TYPE type, In const Configuration& config)
				{
					if (IsFlagSet(type, SHADER_TYPE_COMPLEX))
					{
						if (config.GetComplex() == true && config.GetMSAA() == 1) { return false; }
					}
					return true;
				}
				bool CLOAK_CALL IsUsed(In SHADER_TYPE type, In const Configuration& config, In const ShaderCountInfo& passCount)
				{
					if (IsFlagSet(type, SHADER_TYPE_COMPLEX))
					{
						if (config.GetComplex() == true && IsFlagSet(passCount.Usage, USAGE_MASK_COMPLEX_ON) == false) { return false; }
						if (config.GetComplex() == false && IsFlagSet(passCount.Usage, USAGE_MASK_COMPLEX_OFF) == false) { return false; }
					}
					if (IsFlagSet(type, SHADER_TYPE_MSAA))
					{
						if (config.GetMSAA() == 1 && IsFlagSet(passCount.Usage, USAGE_MASK_MSAA_1) == false) { return false; }
						if (config.GetMSAA() == 2 && IsFlagSet(passCount.Usage, USAGE_MASK_MSAA_2) == false) { return false; }
						if (config.GetMSAA() == 4 && IsFlagSet(passCount.Usage, USAGE_MASK_MSAA_4) == false) { return false; }
						if (config.GetMSAA() == 8 && IsFlagSet(passCount.Usage, USAGE_MASK_MSAA_8) == false) { return false; }
					}
					if (IsFlagSet(type, SHADER_TYPE_TESSELLATION))
					{
						if (config.GetTessellation() == true && IsFlagSet(passCount.Usage, USAGE_MASK_TESSELLATION_ON) == false) { return false; }
						if (config.GetTessellation() == false && IsFlagSet(passCount.Usage, USAGE_MASK_TESSELLATION_OFF) == false) { return false; }
					}
					if (IsFlagSet(type, SHADER_TYPE_WAVESIZE))
					{
						if (config.GetWaveSize() == 4 && IsFlagSet(passCount.Usage, USAGE_MASK_WAVE_SIZE_4) == false) { return false; }
						if (config.GetWaveSize() == 32 && IsFlagSet(passCount.Usage, USAGE_MASK_WAVE_SIZE_32) == false) { return false; }
						if (config.GetWaveSize() == 64 && IsFlagSet(passCount.Usage, USAGE_MASK_WAVE_SIZE_64) == false) { return false; }
						if (config.GetWaveSize() == 128 && IsFlagSet(passCount.Usage, USAGE_MASK_WAVE_SIZE_128) == false) { return false; }
					}

					return true;
				}
				std::wstring CLOAK_CALL ModelToMacroName(In const CE::Rendering::SHADER_MODEL& model)
				{
					std::wstring mn = L"TARGET_";
					switch (model.Language)
					{
						case CE::Rendering::SHADER_DXIL: mn += L"DX"; break;
						case CE::Rendering::SHADER_SPIRV: mn += L"SPIRV"; break;
						case CE::Rendering::SHADER_HLSL: mn += L"DX"; break;
						case CE::Rendering::SHADER_GLSL: mn += L"GL"; break;
						case CE::Rendering::SHADER_ESSL: mn += L"GLES"; break;
						case CE::Rendering::SHADER_MSL_MacOS: mn += L"MSL_PC"; break;
						case CE::Rendering::SHADER_MSL_iOS: mn += L"MSL_M"; break;
						default: CLOAK_ASSUME(false); break;
					}
					mn += L"_" + std::to_wstring(model.Major) + L"_" + std::to_wstring(model.Minor);
					return mn;
				}
				std::string CLOAK_CALL ToString(In size_t threadID, In const Compiler& compiler, In const void* ptr, In size_t size)
				{
					if (ptr != nullptr && size > 0)
					{
						 std::string res(reinterpret_cast<const char*>(ptr), size);
						 while (res[res.length() - 1] == '\n' || res[res.length() - 1] == '\r' || res[res.length() - 1] == '\0') { res = res.substr(0, res.length() - 1); }
						 return res;
					}
					return "";
				}
				std::string CLOAK_CALL ToString(In size_t threadID, In const Compiler& compiler, In const CE::RefPointer<IDxcBlob>& msg)
				{
					if (msg != nullptr)
					{
						IDxcBlobEncoding* enc = nullptr;
						IDxcBlob* b = msg;
						HRESULT hRet = compiler.GetLibrary(threadID)->GetBlobAsUtf8(b, &enc);
						b->Release();
						if (FAILED(hRet)) { return ToString(threadID, compiler, msg->GetBufferPointer(), msg->GetBufferSize()); }
						std::string r = ToString(threadID, compiler, enc->GetBufferPointer(), enc->GetBufferSize());
						enc->Release();
						return r;
					}
					return "";
				}
				std::string CLOAK_CALL ToString(In size_t threadID, In const Compiler& compiler, In const CE::RefPointer<IDxcBlobEncoding>& msg)
				{
					if (msg != nullptr)
					{
						IDxcBlob* b = msg;
						std::string r = ToString(threadID, compiler, b);
						b->Release();
						return r;
					}
					return "";
				}
				std::string CLOAK_CALL ToString(In size_t threadID, In const Compiler& compiler, In const CE::RefPointer<ID3DBlob>& msg) { if (msg != nullptr) { return ToString(threadID, compiler, msg->GetBufferPointer(), msg->GetBufferSize()); } return ""; }
				std::string CLOAK_CALL ToString(In size_t threadID, In const Compiler& compiler, In const CE::RefPointer<Blob>& msg) { if (msg != nullptr) { return ToString(threadID, compiler, msg->GetBufferPointer(), msg->GetBufferSize()); } return ""; }

				namespace Library {
					/*
						Expected Order in Code::List array:
							| Model  | Complex | MSAA | WaveSize | Tessellation
						----|--------|---------|------|----------|--------------
							| DX_5_0 |  false  |  1   |     4    |      false
							| DX_5_0 |  false  |  1   |     4    |      true
							| DX_5_0 |  false  |  1   |    32    |      false
							| DX_5_0 |  false  |  1   |    32    |      true
							| DX_5_0 |  false  |  1   |    64    |      false
							| DX_5_0 |  false  |  1   |    64    |      true
							| DX_5_0 |  false  |  1   |   128    |      false
							| DX_5_0 |  false  |  1   |   128    |      true
							| DX_5_0 |  false  |  2   |     4    |      false
							| DX_5_0 |  false  |  2   |     4    |      true
							| DX_5_0 |  false  |  2   |    32    |      false
							| DX_5_0 |  false  |  2   |    32    |      true
							| DX_5_0 |  false  |  2   |    64    |      false
							| DX_5_0 |  false  |  2   |    64    |      true
							| DX_5_0 |  false  |  2   |   128    |      false
							| DX_5_0 |  false  |  2   |   128    |      true
							| DX_5_0 |  false  |  4   |     4    |      false
							| DX_5_0 |  false  |  4   |     4    |      true
							| DX_5_0 |  false  |  4   |    32    |      false
							| DX_5_0 |  false  |  4   |    32    |      true
							| DX_5_0 |  false  |  4   |    64    |      false
							| DX_5_0 |  false  |  4   |    64    |      true
							| DX_5_0 |  false  |  4   |   128    |      false
							| DX_5_0 |  false  |  4   |   128    |      true
							| DX_5_0 |  false  |  8   |     4    |      false
							| DX_5_0 |  false  |  8   |     4    |      true
							| DX_5_0 |  false  |  8   |    32    |      false
							| DX_5_0 |  false  |  8   |    32    |      true
							| DX_5_0 |  false  |  8   |    64    |      false
							| DX_5_0 |  false  |  8   |    64    |      true
							| DX_5_0 |  false  |  8   |   128    |      false
							| DX_5_0 |  false  |  8   |   128    |      true
							| DX_5_0 |   true  |  1   |     4    |      false
							| DX_5_0 |   true  |  1   |     4    |      true
							| DX_5_0 |   true  |  1   |    32    |      false
							| DX_5_0 |   true  |  1   |    32    |      true
							| DX_5_0 |   true  |  1   |    64    |      false
							| DX_5_0 |   true  |  1   |    64    |      true
							| DX_5_0 |   true  |  1   |   128    |      false
							| DX_5_0 |   true  |  1   |   128    |      true
							| DX_5_0 |   true  |  2   |     4    |      false
							| DX_5_0 |   true  |  2   |     4    |      true
							| DX_5_0 |   true  |  2   |    32    |      false
							| DX_5_0 |   true  |  2   |    32    |      true
							| DX_5_0 |   true  |  2   |    64    |      false
							| DX_5_0 |   true  |  2   |    64    |      true
							| DX_5_0 |   true  |  2   |   128    |      false
							| DX_5_0 |   true  |  2   |   128    |      true
							| DX_5_0 |   true  |  4   |     4    |      false
							| DX_5_0 |   true  |  4   |     4    |      true
							| DX_5_0 |   true  |  4   |    32    |      false
							| DX_5_0 |   true  |  4   |    32    |      true
							| DX_5_0 |   true  |  4   |    64    |      false
							| DX_5_0 |   true  |  4   |    64    |      true
							| DX_5_0 |   true  |  4   |   128    |      false
							| DX_5_0 |   true  |  4   |   128    |      true
							| DX_5_0 |   true  |  8   |     4    |      false
							| DX_5_0 |   true  |  8   |     4    |      true
							| DX_5_0 |   true  |  8   |    32    |      false
							| DX_5_0 |   true  |  8   |    32    |      true
							| DX_5_0 |   true  |  8   |    64    |      false
							| DX_5_0 |   true  |  8   |    64    |      true
							| DX_5_0 |   true  |  8   |   128    |      false
							| DX_5_0 |   true  |  8   |   128    |      true
						*/
					inline std::string CLOAK_CALL ModelName(In const CE::Rendering::SHADER_MODEL& model)
					{
						std::stringstream s;
						switch (model.Language)
						{
							case CE::Rendering::SHADER_DXIL:
							case CE::Rendering::SHADER_HLSL:
								s << "HLSL";
								break;
							case CE::Rendering::SHADER_SPIRV:
								s << "SPIRV";
								break;
							case CE::Rendering::SHADER_GLSL:
								s << "GLSL";
								break;
							case CE::Rendering::SHADER_ESSL:
								s << "ESSL";
								break;
							case CE::Rendering::SHADER_MSL_iOS:
								s << "MSL_IOS";
								break;
							case CE::Rendering::SHADER_MSL_MacOS:
								s << "MSL_MAC";
								break;
							default:
								break;
						}
						s << "_" << model.Major << "_" << model.Minor;
						if (model.Patch != 0) { s << "_" << model.Patch; }
						return s.str();
					}

					void CLOAK_CALL WriteShaderArrayVersion(In const CE::RefPointer<CloakEngine::Files::IWriter>& header, In size_t tabCount, In SHADER_TYPE shader, In const CE::Rendering::SHADER_MODEL& model, In const ShaderCountInfo& passCount)
					{
						bool complex = false;
						bool tess = false;
						size_t msaa = 0;
						size_t wave = 0;
#ifdef _DEBUG
						size_t written = 0;
#endif
						while (true)
						{
							Configuration config;
							config.SetComplex(complex);
							config.SetTessellation(tess);
							config.SetMSAA(MSAA_COUNT[msaa].Value);
							config.SetWaveSize(WAVE_SIZES[wave].Value);

							if (IsUsed(shader, config, passCount) == true)
							{
#ifdef _DEBUG
								written++;
#endif
								if (IsValidConfig(shader, config) == true)
								{
									std::string name = ComposeShaderName(model, shader, config);
									Engine::Lib::WriteWithTabs(header, tabCount, "{ " + name + "D, " + name + "L },");
								}
								else
								{
									Engine::Lib::WriteWithTabs(header, tabCount, "{ nullptr, 0 },");
								}
							}

							if (IsFlagSet(shader, SHADER_TYPE_TESSELLATION))
							{
								tess = !tess;
								if (tess != false) { continue; }
							}
							if (IsFlagSet(shader, SHADER_TYPE_WAVESIZE))
							{
								wave++;
								if (wave < ARRAYSIZE(WAVE_SIZES)) { continue; }
								wave = 0;
							}
							if (IsFlagSet(shader, SHADER_TYPE_MSAA))
							{
								msaa++;
								if (msaa < ARRAYSIZE(MSAA_COUNT)) { continue; }
								msaa = 0;
							}
							if (IsFlagSet(shader, SHADER_TYPE_COMPLEX))
							{
								complex = !complex;
								if (complex != false) { continue; }
							}
							break;
						}
#ifdef _DEBUG
						CLOAK_ASSUME(written > 0);
#endif
					}
					void CLOAK_CALL WriteShaderArray(In const CE::RefPointer<CloakEngine::Files::IWriter>& header, In size_t tabCount, In SHADER_TYPE shader, In const ShaderCountInfo& passCount)
					{
						Engine::Lib::WriteWithTabs(header, tabCount, "constexpr ByteCode List[] = {");
						for (size_t a = 0; a < ARRAYSIZE(CE::Rendering::SHADER_VERSIONS); a++) { WriteShaderArrayVersion(header, tabCount + 1, shader, CE::Rendering::SHADER_VERSIONS[a], passCount); }
						Engine::Lib::WriteWithTabs(header, tabCount, "};");
					}
					void CLOAK_CALL WriteShaderGetter(In const CE::RefPointer<CloakEngine::Files::IWriter>& header, In size_t tabCount, In SHADER_TYPE shader, In const ShaderPassCount& passCounts, In ShaderPass pass)
					{
						std::string name = "";
						ShaderCountInfo sci;
						switch (pass)
						{
							case CloakCompiler::API::Shader::Shader_v1003::ShaderPass::VS: name = "VS"; sci = passCounts.VS; break;
							case CloakCompiler::API::Shader::Shader_v1003::ShaderPass::HS: name = "HS"; sci = passCounts.HS; break;
							case CloakCompiler::API::Shader::Shader_v1003::ShaderPass::DS: name = "DS"; sci = passCounts.DS; break;
							case CloakCompiler::API::Shader::Shader_v1003::ShaderPass::GS: name = "GS"; sci = passCounts.GS; break;
							case CloakCompiler::API::Shader::Shader_v1003::ShaderPass::PS: name = "PS"; sci = passCounts.PS; break;
							case CloakCompiler::API::Shader::Shader_v1003::ShaderPass::CS: name = "CS"; sci = passCounts.CS; break;
							default: CLOAK_ASSUME(false); return;
						}
						Engine::Lib::WriteWithTabs(header, tabCount, std::string(SHADER_GET_START) + name + "(" + std::string(SHADER_GET_ARGS_MAIN) + ")");
						Engine::Lib::WriteWithTabs(header, tabCount, "{");
						Engine::Lib::WriteWithTabs(header, tabCount + 1, "if(result != nullptr) { *result = nullptr; }");
						Engine::Lib::WriteWithTabs(header, tabCount + 1, "if(resultLength != nullptr) { *resultLength = 0; }");
						if (sci.Count > 0)
						{
							Engine::Lib::WriteWithTabs(header, tabCount + 1, "size_t v = 0;");
							size_t offset = 1;
							if (IsFlagSet(shader, SHADER_TYPE_TESSELLATION))
							{
								if (IsFlagSet(sci.Usage, USAGE_MASK_TESSELLATION_ALLWAYS))
								{
									Engine::Lib::WriteWithTabs(header, tabCount + 1, "if(tessellation == true) { v += " + std::to_string(offset) + "; }");
									offset *= 2;
								}
								else
								{
									CLOAK_ASSUME(IsFlagSet(sci.Usage, USAGE_MASK_TESSELLATION_ALLWAYS, USAGE_MASK_NEVER) == false);
									const bool u = IsFlagSet(sci.Usage, USAGE_MASK_TESSELLATION_OFF);
									Engine::Lib::WriteWithTabs(header, tabCount + 1, "if (tessellation == " + std::string(u == true ? "true" : "false") + ") { return; }");
								}
							}
							if (IsFlagSet(shader, SHADER_TYPE_WAVESIZE))
							{
								std::string pre = "";
								size_t ofc = 0;
								for (size_t a = ARRAYSIZE(WAVE_SIZES); a > 0; a--)
								{
									const size_t b = a - 1;
									if (IsFlagSet(sci.Usage, WAVE_SIZES[b].Usage))
									{
										Engine::Lib::WriteWithTabs(header, tabCount + 1, pre + "if(waveSize >= " + std::to_string(WAVE_SIZES[b].Value) + ") { v += " + std::to_string(b * offset) + "; }");
										pre = "else ";
										ofc++;
									}
								}
								Engine::Lib::WriteWithTabs(header, tabCount + 1, pre + "{ return; }");
								offset *= ofc;
							}
							if (IsFlagSet(shader, SHADER_TYPE_MSAA))
							{
								std::string pre = "";
								size_t ofc = 0;
								for (size_t a = 0; a < ARRAYSIZE(MSAA_COUNT); a++)
								{
									if (IsFlagSet(sci.Usage, MSAA_COUNT[a].Usage))
									{
										Engine::Lib::WriteWithTabs(header, tabCount + 1, pre + "if(msaaCount == " + std::to_string(MSAA_COUNT[a].Value) + ") { v += " + std::to_string(a * offset) + "; }");
										pre = "else ";
										ofc++;
									}
								}
								Engine::Lib::WriteWithTabs(header, tabCount + 1, pre + "{ return; }");
								offset *= ofc;
							}
							if (IsFlagSet(shader, SHADER_TYPE_COMPLEX))
							{
								if (IsFlagSet(sci.Usage, USAGE_MASK_COMPLEX_ALLWAYS))
								{
									Engine::Lib::WriteWithTabs(header, tabCount + 1, "if(complex == true) { v += " + std::to_string(offset) + "; }");
									offset *= 2;
								}
								else
								{
									CLOAK_ASSUME(IsFlagSet(sci.Usage, USAGE_MASK_COMPLEX_ALLWAYS, USAGE_MASK_NEVER) == false);
									const bool u = IsFlagSet(sci.Usage, USAGE_MASK_COMPLEX_OFF);
									Engine::Lib::WriteWithTabs(header, tabCount + 1, "if (complex == " + std::string(u == true ? "true" : "false") + ") { return; }");
								}
							}
							Engine::Lib::WriteWithTabs(header, tabCount + 1, "switch(model)");
							Engine::Lib::WriteWithTabs(header, tabCount + 1, "{");
							for (size_t a = 0; a < ARRAYSIZE(CE::Rendering::SHADER_VERSIONS); a++)
							{
								Engine::Lib::WriteWithTabs(header, tabCount + 2, "case ShaderModel::" + ModelName(CE::Rendering::SHADER_VERSIONS[a]) + ": v += " + std::to_string(a * offset) + "; break;");
							}
							Engine::Lib::WriteWithTabs(header, tabCount + 2, "default: return;");
							Engine::Lib::WriteWithTabs(header, tabCount + 1, "}");
							offset *= ARRAYSIZE(CE::Rendering::SHADER_VERSIONS);

							Engine::Lib::WriteWithTabs(header, tabCount + 1, "if(result != nullptr) { *result = Code::" + name + "::List[v].Code; }");
							Engine::Lib::WriteWithTabs(header, tabCount + 1, "if(resultLength != nullptr) { *resultLength = Code::" + name + "::List[v].Length; }");
						}
						Engine::Lib::WriteWithTabs(header, tabCount, "}");
					}
					void CLOAK_CALL WriteShaderGetter(In const CE::RefPointer<CloakEngine::Files::IWriter>& header, In size_t tabCount, In SHADER_TYPE shader, In const ShaderPassCount& passCounts)
					{
						WriteShaderGetter(header, tabCount, shader, passCounts, ShaderPass::VS);
						WriteShaderGetter(header, tabCount, shader, passCounts, ShaderPass::HS);
						WriteShaderGetter(header, tabCount, shader, passCounts, ShaderPass::DS);
						WriteShaderGetter(header, tabCount, shader, passCounts, ShaderPass::GS);
						WriteShaderGetter(header, tabCount, shader, passCounts, ShaderPass::PS);
						WriteShaderGetter(header, tabCount, shader, passCounts, ShaderPass::CS);
					}
					void CLOAK_CALL WriteShaderGetterDeclaration(In const CE::RefPointer<CloakEngine::Files::IWriter>& header, In size_t tabCount, In ShaderPass pass, In_opt bool writeMain = true, In_opt bool writeVariants = true)
					{
						std::string name = "";
						switch (pass)
						{
							case CloakCompiler::API::Shader::Shader_v1003::ShaderPass::VS: name = "VS"; break;
							case CloakCompiler::API::Shader::Shader_v1003::ShaderPass::HS: name = "HS"; break;
							case CloakCompiler::API::Shader::Shader_v1003::ShaderPass::DS: name = "DS"; break;
							case CloakCompiler::API::Shader::Shader_v1003::ShaderPass::GS: name = "GS"; break;
							case CloakCompiler::API::Shader::Shader_v1003::ShaderPass::PS: name = "PS"; break;
							case CloakCompiler::API::Shader::Shader_v1003::ShaderPass::CS: name = "CS"; break;
							default: CLOAK_ASSUME(false); return;
						}
						if (writeMain == true)
						{
							Engine::Lib::WriteWithTabs(header, tabCount, std::string(SHADER_GET_START) + name + +"(" + std::string(SHADER_GET_ARGS_MAIN) + ");");
						}
						if (writeVariants == true)
						{
							Engine::Lib::WriteWithTabs(header, 0, "#ifdef CLOAKENGINE_VERSION");
							Engine::Lib::WriteWithTabs(header, tabCount, "inline " + std::string(SHADER_GET_START_VARAINTS) + name + "(" + std::string(SHADER_GET_VARIANT_MAIN) + ")");
							Engine::Lib::WriteWithTabs(header, tabCount, "{");
							Engine::Lib::WriteWithTabs(header, tabCount + 1, "CloakEngine::API::Rendering::SHADER_CODE res;");
							Engine::Lib::WriteWithTabs(header, tabCount + 1, "Get" + name + "(GetModel(model), " + std::string(SHADER_GET_VARIANT_MAIN_CALL) + ", &res.Code, &res.Length);");
							Engine::Lib::WriteWithTabs(header, tabCount + 1, "return res;");
							Engine::Lib::WriteWithTabs(header, tabCount, "}");
							for (size_t a = 0; a < ARRAYSIZE(SHADER_GET_ARGS_VARIANTS); a += 2)
							{
								Engine::Lib::WriteWithTabs(header, tabCount, "CLOAK_FORCEINLINE " + std::string(SHADER_GET_START_VARAINTS) + name + "(" + std::string(SHADER_GET_ARGS_VARIANTS[a]) + ") { return Get" + name + "(" + std::string(SHADER_GET_ARGS_VARIANTS[a + 1]) + "); }");
							}
							Engine::Lib::WriteWithTabs(header, 0, "#endif");
						}
					}
					void CLOAK_CALL WriteShaderGetterDeclaration(In const CE::RefPointer<CloakEngine::Files::IWriter>& header, In size_t tabCount, In_opt bool writeMain = true, In_opt bool writeVariants = true)
					{
						WriteShaderGetterDeclaration(header, tabCount, ShaderPass::VS, writeMain, writeVariants);
						WriteShaderGetterDeclaration(header, tabCount, ShaderPass::HS, writeMain, writeVariants);
						WriteShaderGetterDeclaration(header, tabCount, ShaderPass::DS, writeMain, writeVariants);
						WriteShaderGetterDeclaration(header, tabCount, ShaderPass::GS, writeMain, writeVariants);
						WriteShaderGetterDeclaration(header, tabCount, ShaderPass::PS, writeMain, writeVariants);
						WriteShaderGetterDeclaration(header, tabCount, ShaderPass::CS, writeMain, writeVariants);
					}
					void CLOAK_CALL WriteShaderModels(In const CE::RefPointer<CloakEngine::Files::IWriter>& header, In size_t tabCount, In bool cloakConverter)
					{
						Engine::Lib::WriteWithTabs(header, tabCount, "enum ShaderModel {");
						for (size_t a = 0; a < ARRAYSIZE(CE::Rendering::SHADER_VERSIONS); a++)
						{
							Engine::Lib::WriteWithTabs(header, tabCount + 1, ModelName(CE::Rendering::SHADER_VERSIONS[a]) + " = " + std::to_string(a) + ",");
						}
						Engine::Lib::WriteWithTabs(header, tabCount, "};");
						if (cloakConverter == true)
						{
							Engine::Lib::WriteWithTabs(header, 0, "#ifdef CLOAKENGINE_VERSION");
							Engine::Lib::WriteWithTabs(header, tabCount, "inline ShaderModel CLOAK_CALL GetModel(In const CloakEngine::API::Rendering::SHADER_MODEL& model)");
							Engine::Lib::WriteWithTabs(header, tabCount, "{");

							//TODO: This could be improved using DisjointSet and RADIX sort
							size_t sortedV[ARRAYSIZE(CE::Rendering::SHADER_VERSIONS)];
							for (size_t a = 0; a < ARRAYSIZE(sortedV); a++) { sortedV[a] = a; }
							for (size_t a = 1; a < ARRAYSIZE(sortedV); a++)
							{
								size_t b = 0;
								for (; b < a; b++)
								{
									if (CE::Rendering::SHADER_VERSIONS[sortedV[b]].Language == CE::Rendering::SHADER_VERSIONS[a].Language) { break; }
								}
								for (; b < a; b++)
								{
									if (CE::Rendering::SHADER_VERSIONS[sortedV[b]] < CE::Rendering::SHADER_VERSIONS[a]) { continue; }
									break;
								}
								for (size_t c = a; c > b; c--) { sortedV[c] = sortedV[c - 1]; }
								sortedV[b] = a;
							}
							std::string pre = "";
							for (size_t a = 0; a < ARRAYSIZE(sortedV); a++)
							{
								const size_t p = sortedV[a];
								std::string eq = "==";
								if (a + 1 == ARRAYSIZE(sortedV) || CE::Rendering::SHADER_VERSIONS[p].Language != CE::Rendering::SHADER_VERSIONS[sortedV[a + 1]].Language) { eq = ">="; }
								std::string rc = "if(model " + eq + " CloakEngine::API::Rendering::SHADER_MODEL(CloakEngine::API::Rendering::";
								switch (CE::Rendering::SHADER_VERSIONS[p].Language)
								{
									case CE::Rendering::SHADER_DXIL: rc += "SHADER_DXIL"; break;
									case CE::Rendering::SHADER_SPIRV: rc += "SHADER_SPIRV"; break;
									case CE::Rendering::SHADER_HLSL: rc += "SHADER_HLSL"; break;
									case CE::Rendering::SHADER_GLSL: rc += "SHADER_GLSL"; break;
									case CE::Rendering::SHADER_ESSL: rc += "SHADER_ESSL"; break;
									case CE::Rendering::SHADER_MSL_MacOS: rc += "SHADER_MSL_MacOS"; break;
									case CE::Rendering::SHADER_MSL_iOS: rc += "SHADER_MSL_iOS"; break;
									default: CLOAK_ASSUME(false); break;
								}
								rc += ", " + std::to_string(CE::Rendering::SHADER_VERSIONS[p].Major);
								rc += ", " + std::to_string(CE::Rendering::SHADER_VERSIONS[p].Minor);
								rc += ", " + std::to_string(CE::Rendering::SHADER_VERSIONS[p].Patch);
								rc += ")) { return ShaderModel::" + ModelName(CE::Rendering::SHADER_VERSIONS[p]) + "; }";
								Engine::Lib::WriteWithTabs(header, tabCount + 1, pre + rc);
								pre = "else ";
							}
							Engine::Lib::WriteWithTabs(header, tabCount + 1, "return static_cast<ShaderModel>(" + std::to_string(ARRAYSIZE(CE::Rendering::SHADER_VERSIONS)) + ");");
							Engine::Lib::WriteWithTabs(header, tabCount, "}");
							Engine::Lib::WriteWithTabs(header, 0, "#endif");
						}
					}
					size_t CLOAK_CALL WriteCommonIntro(In const CE::RefPointer<CloakEngine::Files::IWriter>& header, In const std::string& name, In size_t tabCount, In bool writeCloakModelConverter)
					{
						tabCount = Engine::Lib::WriteCommonIntro(header, Engine::Lib::Type::Shader, name, tabCount);
						Engine::Lib::WriteWithTabs(header, tabCount, "constexpr std::uint32_t Version = " + std::to_string(FileType.Version) + ";");
						WriteShaderModels(header, tabCount, writeCloakModelConverter);
						return tabCount;
					}
					CE::Global::Task CLOAK_CALL WriteCommonFile(In const CE::Files::UFI& path, In const std::string& name)
					{
						CE::Global::Task t = [&path, &name](In size_t threadID)
						{
							CE::RefPointer<CloakEngine::Files::IWriter> writer = Engine::Lib::CreateSourceFile(path, "Common", "H");
							Engine::Lib::WriteHeaderIntro(writer, Engine::Lib::Type::Shader, name + "_COMMON", path.GetHash());
							size_t tabCount = WriteCommonIntro(writer, name, 0, false);
							Engine::Lib::WriteWithTabs(writer, tabCount, "struct ByteCode {");
							Engine::Lib::WriteWithTabs(writer, tabCount + 1, "const void* Code;");
							Engine::Lib::WriteWithTabs(writer, tabCount + 1, "std::size_t Length;");
							Engine::Lib::WriteWithTabs(writer, tabCount, "};");
							tabCount = Engine::Lib::WriteCommonOutro(writer, tabCount);
							Engine::Lib::WriteHeaderOutro(writer, 0);

							writer->Save();
						};
						t.Schedule(CE::Global::Threading::Flag::IO);
						return t;
					}
					CE::Global::Task CLOAK_CALL WriteCMakeFile(In const CE::List<SHADER>& shaders, In const CE::Files::UFI& path, In const std::string& name, In bool shared)
					{
						CE::Global::Task t = [&shaders, &path, &name, shared](In size_t threadID)
						{
							Engine::Lib::CMakeFile file(Engine::Lib::Type::Shader, path, name, shared);
							file.AddFile("Common.h", true);
							for (size_t a = 0; a < shaders.size(); a++) { file.AddFile(shaders[a].Name); }
						};
						t.Schedule(CE::Global::Threading::Flag::IO);
						return t;
					}
					CE::RefPointer<CloakEngine::Files::IWriter> CLOAK_CALL WriteSourceFile(In const CE::RefPointer<CloakEngine::Files::IWriter>& header, In const CE::Files::UFI& path, In const std::string& name, In const std::string& shaderName, In size_t tabCount)
					{
						Engine::Lib::WriteWithTabs(header, tabCount, "namespace " + name + " {");
						WriteShaderGetterDeclaration(header, tabCount + 1);
						Engine::Lib::WriteWithTabs(header, tabCount, "}");

						CE::RefPointer<CloakEngine::Files::IWriter> srcFile = Engine::Lib::CreateSourceFile(path, name, "CPP");
						Engine::Lib::WriteCopyright(srcFile);
						Engine::Lib::WriteWithTabs(srcFile, 0, "#include \"Common.h\"");
						size_t tc = Engine::Lib::WriteCommonIntro(srcFile, Engine::Lib::Type::Shader, shaderName);
						CLOAK_ASSUME(tc + 1 == tabCount);
						Engine::Lib::WriteWithTabs(srcFile, tc, "namespace Shaders {");
						Engine::Lib::WriteWithTabs(srcFile, tc + 1, "namespace " + name + " {");
						return srcFile;
					}
					void CLOAK_CALL CloseSourceFile(const CE::RefPointer<CloakEngine::Files::IWriter>& srcFile, In size_t tabCount)
					{
						if (tabCount >= 1) { Engine::Lib::WriteWithTabs(srcFile, tabCount - 1, "}"); }
						if (tabCount >= 2)
						{
							Engine::Lib::WriteWithTabs(srcFile, tabCount - 2, "}");
							Engine::Lib::WriteCommonOutro(srcFile, tabCount - 2);
						}
						Engine::Lib::WriteWithTabs(srcFile, 0, "");
					}

					inline void CLOAK_CALL WriteShader(In const CE::RefPointer<CloakEngine::Files::IWriter>& header, In std::string name, In size_t tabCount, In size_t byteCount, In_reads(byteCount) const uint8_t* bytes)
					{
						if (header != nullptr)
						{
							std::stringstream d;
							std::stringstream l;
							if (byteCount > 0)
							{
								d << "const std::uint8_t " << name << "D[" << byteCount << "] = {";
								l << "constexpr std::size_t " << name << "L = " << byteCount << ";";
								for (size_t a = 0; a < byteCount; a++)
								{
									if (a > 0) { d << ","; }
									d << static_cast<uint32_t>(bytes[a]);
								}
								d << "};";
							}
							else
							{
								d << "constexpr std::uint8_t* " << name << "D = nullptr;";
								l << "constexpr std::size_t " << name << "L = 0;";
							}
							Engine::Lib::WriteWithTabs(header, tabCount, d.str());
							Engine::Lib::WriteWithTabs(header, tabCount, l.str());
						}
					}
				}
				namespace File {
					void CLOAK_CALL WriteShaderStart(In const CE::RefPointer<CloakEngine::Files::IWriter>& file, In const SHADER& shader, In uint64_t codeSize)
					{
						//TODO
					}
					void CLOAK_CALL WriteShaderPass(In const CE::RefPointer<CloakEngine::Files::IWriter>& file, In ShaderPass pass, In uint64_t codeSize)
					{
						//TODO
					}
					void CLOAK_CALL WriteShaderConfig(In const CE::RefPointer<CloakEngine::Files::IWriter>& file, In CE::Rendering::SHADER_MODEL model, In const Configuration& config, In uint64_t codeSize)
					{
						//TODO
					}

					inline void CLOAK_CALL WriteShader(In const CE::RefPointer<CloakEngine::Files::IWriter>& file, In size_t byteCount, In_reads(byteCount) const uint8_t* bytes)
					{
						if (file != nullptr)
						{
							//TODO
						}
					}
				}
				void CLOAK_CALL WriteShader(In const CE::RefPointer<CloakEngine::Files::IWriter>& file, In const CE::RefPointer<CloakEngine::Files::IWriter>& header, In std::string name, In size_t tabCount, In size_t byteCount, In_reads(byteCount) const uint8_t* bytes)
				{
					File::WriteShader(file, byteCount, bytes);
					Library::WriteShader(header, name, tabCount, byteCount, bytes);
				}

				namespace ConvertedCompiler {
					bool CLOAK_CALL HLSL(In size_t threadID, In const Compiler& compiler, In size_t srcSize, In_reads_bytes(srcSize) const void* src, In const std::wstring& profile, In const SHADER_NAME& shader, In const Configuration& config, In OPTION options, In const std::string& shaderName, In size_t shaderID, Out Blob** res)
					{
						CLOAK_ASSUME(res != nullptr);
						*res = nullptr;
						std::string target = CE::Helper::StringConvert::ConvertToU8(profile);
						D3D_SHADER_MACRO macro = { nullptr, nullptr };

						UINT flags = D3DCOMPILE_ENABLE_UNBOUNDED_DESCRIPTOR_TABLES | D3DCOMPILE_PACK_MATRIX_ROW_MAJOR | D3DCOMPILE_ALL_RESOURCES_BOUND;
						if (IsFlagSet(options, OPTION_DEBUG)) { flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_DEBUG_NAME_FOR_SOURCE; }
						if (IsFlagSet(options, OPTION_OPTIMIZATION_MASK, OPTION_NONE)) { flags |= D3DCOMPILE_SKIP_OPTIMIZATION; }
						else if (IsFlagSet(options, OPTION_OPTIMIZATION_MASK, OPTION_OPTIMIZATION_LEVEL_0)) { flags |= D3DCOMPILE_OPTIMIZATION_LEVEL0; }
						else if (IsFlagSet(options, OPTION_OPTIMIZATION_MASK, OPTION_OPTIMIZATION_LEVEL_1)) { flags |= D3DCOMPILE_OPTIMIZATION_LEVEL1; }
						else if (IsFlagSet(options, OPTION_OPTIMIZATION_MASK, OPTION_OPTIMIZATION_LEVEL_2)) { flags |= D3DCOMPILE_OPTIMIZATION_LEVEL2; }
						else if (IsFlagSet(options, OPTION_OPTIMIZATION_MASK, OPTION_OPTIMIZATION_LEVEL_3)) { flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3; }
						else { compiler.SendResponse(threadID, shaderID, shader, config, Status::FAILED, "Optimization setting is unsupported!"); return false; }

						CE::RefPointer<ID3DBlob> codeBlob = nullptr;
						CE::RefPointer<ID3DBlob> errBlob = nullptr;
						HRESULT hRet = D3DCompile(src, srcSize, shaderName.c_str(), &macro, nullptr, shader.MainFuncName.c_str(), target.c_str(), flags, 0, &codeBlob, &errBlob);
						if (SUCCEEDED(hRet) && codeBlob != nullptr)
						{
							if (errBlob != nullptr)
							{
								compiler.SendResponse(threadID, shaderID, shader, config, Status::WARNING, ToString(threadID, compiler, errBlob));
							}
							compiler.SendResponse(threadID, shaderID, shader, config, Status::FINISHED);
							*res = new Blob(codeBlob);
							return true;
						}
						else if (errBlob != nullptr)
						{
							compiler.SendResponse(threadID, shaderID, shader, config, Status::FAILED, ToString(threadID, compiler, errBlob));
						}
						else
						{
							_com_error hErr(hRet);
							std::wstring hEMsg = hErr.ErrorMessage() == nullptr ? L"" : hErr.ErrorMessage();
							std::string err = CloakEngine::Helper::StringConvert::ConvertToU8(hEMsg);
							if (err.length() == 0) { err = "Unknown Error!"; }
							compiler.SendResponse(threadID, shaderID, shader, config, Status::FAILED, err);
						}
						return false;
					}
				}

				//Converts SPIR-V binary code into readable, fast GLSL/HLSL/ESSL/MSL source code
				bool CLOAK_CALL ConvertBinary(In const Compiler& compiler, In size_t threadID, In const CE::RefPointer<IDxcBlob>& code, In const SHADER_NAME& shader, In const Configuration& config, In size_t shaderID, Out Blob** res)
				{
					//Based on Shader Conductor: https://github.com/microsoft/ShaderConductor/blob/master/Source/Core/ShaderConductor.cpp

					*res = nullptr;
					spirv_cross::CompilerGLSL* spComp = nullptr;
					bool combinedSamplers = false;
					bool dummySamler = false;
					uint32_t intVersion = 0;
					switch (shader.Model.Language)
					{
						case CloakEngine::API::Rendering::SHADER_HLSL:
							if (shader.Pass == ShaderPass::GS && shader.Model < CE::Rendering::SHADER_MODEL(CE::Rendering::SHADER_HLSL, 4, 0))
							{
								compiler.SendResponse(threadID, shaderID, shader, config, Status::FAILED, "GS is only aviable to HLSL 4.0 and higher");
								return false;
							}
							if ((shader.Pass == ShaderPass::CS || shader.Pass == ShaderPass::HS || shader.Pass == ShaderPass::DS) && shader.Model < CE::Rendering::SHADER_MODEL(CE::Rendering::SHADER_HLSL, 5, 0))
							{
								compiler.SendResponse(threadID, shaderID, shader, config, Status::FAILED, "CS, HS and DS are only aviable to HLSL 5.0 and higher");
								return false;
							}
							if (shader.Model < CE::Rendering::SHADER_MODEL(CE::Rendering::SHADER_HLSL, 3, 0))
							{
								compiler.SendResponse(threadID, shaderID, shader, config, Status::FAILED, "HLSL earlier then 3.0 is not supporeted by SPIR-V Cross.");
								return false;
							}
							intVersion = (shader.Model.Major * 10Ui32) + shader.Model.Minor;
							spComp = new spirv_cross::CompilerHLSL(reinterpret_cast<const uint32_t*>(code->GetBufferPointer()), code->GetBufferSize() / sizeof(uint32_t));
							break;
						case CloakEngine::API::Rendering::SHADER_GLSL:
							switch (shader.Model.Major)
							{
								case 1: compiler.SendResponse(threadID, shaderID, shader, config, Status::FAILED, "GLSL < 2.0 is not aviable!"); return false;
								case 2: intVersion = shader.Model.Minor == 0 ? 110 : 120; break;
								case 3: if (shader.Model.Minor < 3) { intVersion = 130 + (shader.Model.Minor * 10Ui32); break; }
								default: intVersion = ((shader.Model.Major * 10Ui32) + shader.Model.Minor) * 10Ui32; break;
							}
							spComp = new spirv_cross::CompilerGLSL(reinterpret_cast<const uint32_t*>(code->GetBufferPointer()), code->GetBufferSize() / sizeof(uint32_t));
							combinedSamplers = true;
							dummySamler = true;
							break;
						case CloakEngine::API::Rendering::SHADER_ESSL:
							switch (shader.Model.Major)
							{
								case 1: compiler.SendResponse(threadID, shaderID, shader, config, Status::FAILED, "ESSL < 2.0 is not aviable!"); return false;
								case 2: intVersion = 100; break;
								default: intVersion = shader.Model.Major * 100Ui32; break;
							}
							spComp = new spirv_cross::CompilerGLSL(reinterpret_cast<const uint32_t*>(code->GetBufferPointer()), code->GetBufferSize() / sizeof(uint32_t));
							combinedSamplers = true;
							dummySamler = true;
							break;
						case CloakEngine::API::Rendering::SHADER_MSL_MacOS:
						case CloakEngine::API::Rendering::SHADER_MSL_iOS:
							if (shader.Pass == ShaderPass::GS)
							{
								compiler.SendResponse(threadID, shaderID, shader, config, Status::FAILED, "GS is not supported by Metal Shading Language");
								return nullptr;
							}
							intVersion = (((shader.Model.Major * 100Ui32) + shader.Model.Minor) * 100Ui32) + shader.Model.Patch;
							spComp = new spirv_cross::CompilerMSL(reinterpret_cast<const uint32_t*>(code->GetBufferPointer()), code->GetBufferSize() / sizeof(uint32_t));
							break;
						case CloakEngine::API::Rendering::SHADER_DXIL:
						case CloakEngine::API::Rendering::SHADER_SPIRV:
						default: CLOAK_ASSUME(false); return false;
					}
					CLOAK_ASSUME(spComp != nullptr);
					spv::ExecutionModel spvModel;
					switch (shader.Pass)
					{
						case ShaderPass::VS: spvModel = spv::ExecutionModel::ExecutionModelVertex; break;
						case ShaderPass::HS: spvModel = spv::ExecutionModel::ExecutionModelTessellationControl; break;
						case ShaderPass::DS: spvModel = spv::ExecutionModel::ExecutionModelTessellationEvaluation; break;
						case ShaderPass::GS: spvModel = spv::ExecutionModel::ExecutionModelGeometry; break;
						case ShaderPass::PS: spvModel = spv::ExecutionModel::ExecutionModelFragment; break;
						case ShaderPass::CS: spvModel = spv::ExecutionModel::ExecutionModelGLCompute; break;
						default: CLOAK_ASSUME(false); break;
					}
					spComp->set_entry_point(shader.MainFuncName, spvModel);

					auto co = spComp->get_common_options();
					co.version = intVersion;
					co.es = shader.Model.Language == CE::Rendering::SHADER_ESSL;
					co.force_temporary = false;
					co.separate_shader_objects = true;
					co.flatten_multidimensional_arrays = false;
					co.enable_420pack_extension = shader.Model.Language == CE::Rendering::SHADER_GLSL && intVersion >= 420;
					co.vulkan_semantics = false;
					co.vertex.fixup_clipspace = false;
					co.vertex.flip_vert_y = false;
					co.vertex.support_nonzero_base_instance = true;
					spComp->set_common_options(co);

					switch (shader.Model.Language)
					{
						case CloakEngine::API::Rendering::SHADER_HLSL:
						{
							auto* hlsl = static_cast<spirv_cross::CompilerHLSL*>(spComp);
							auto opt = hlsl->get_hlsl_options();
							opt.shader_model = intVersion;
							if (intVersion <= 30)
							{
								combinedSamplers = true;
								dummySamler = true;
							}
							hlsl->set_hlsl_options(opt);
							break;
						}
						case CloakEngine::API::Rendering::SHADER_MSL_MacOS:
						case CloakEngine::API::Rendering::SHADER_MSL_iOS:
						{
							auto* msl = static_cast<spirv_cross::CompilerMSL*>(spComp);
							auto opt = msl->get_msl_options();
							opt.msl_version = intVersion;
							opt.platform = shader.Model.Language == CE::Rendering::SHADER_MSL_MacOS ? spirv_cross::CompilerMSL::Options::macOS : spirv_cross::CompilerMSL::Options::iOS;
							msl->set_msl_options(opt);

							const auto& rsc = msl->get_shader_resources();

							uint32_t binding = 0;
							for (const auto& i : rsc.separate_images) { msl->set_decoration(i.id, spv::DecorationBinding, binding++); }
							binding = 0;
							for (const auto& s : rsc.separate_samplers) { msl->set_decoration(s.id, spv::DecorationBinding, binding++); }
							break;
						}
						case CloakEngine::API::Rendering::SHADER_GLSL:
						case CloakEngine::API::Rendering::SHADER_ESSL:
							break;
						case CloakEngine::API::Rendering::SHADER_DXIL:
						case CloakEngine::API::Rendering::SHADER_SPIRV:
						default: CLOAK_ASSUME(false); return nullptr;
					}

					if (dummySamler == true)
					{
						const uint32_t sc = spComp->build_dummy_sampler_for_combined_images();
						if (sc > 0)
						{
							spComp->set_decoration(sc, spv::DecorationDescriptorSet, 0);
							spComp->set_decoration(sc, spv::DecorationBinding, 0);
						}
					}
					if (combinedSamplers == true)
					{
						spComp->build_combined_image_samplers();

						for (auto& s : spComp->get_combined_image_samplers())
						{
							spComp->set_name(s.combined_id, "SPIRV_Cross_Combined" + spComp->get_name(s.image_id) + spComp->get_name(s.sampler_id));
						}
					}
					if (shader.Model.Language == CE::Rendering::SHADER_HLSL)
					{
						auto* hlsl = static_cast<spirv_cross::CompilerHLSL*>(spComp);
						const uint32_t b = hlsl->remap_num_workgroups_builtin();
						if (b > 0)
						{
							spComp->set_decoration(b, spv::DecorationDescriptorSet, 0);
							spComp->set_decoration(b, spv::DecorationBinding, 0);
						}
					}

					bool r = false;
					try {
						const std::string s = spComp->compile();
						*res = new Blob(s.size(), s.data());
						r = true;
					}
					catch (spirv_cross::CompilerError& error)
					{
						compiler.SendResponse(threadID, shaderID, shader, config, Status::FAILED, error.what());
					}
					delete spComp;
					return r;
				}
				bool CLOAK_CALL CompileSingleShader(In size_t threadID, In const Compiler& compiler, In const CE::RefPointer<CloakEngine::Files::IWriter>& file, In const CE::RefPointer<CloakEngine::Files::IWriter>& header, In const EncodeDesc& encode, In const SHADER_NAME& shader, In const Configuration& config, In SHADER_TYPE type, In OPTION options, In const CE::List<Define>& macros, In std::wstring profile, In const std::string& shaderName, In size_t tabCount, In size_t shaderID)
				{
					std::wstring funcName = CE::Helper::StringConvert::ConvertToW(shader.MainFuncName);
					if (funcName.length() == 0) { funcName = L"main"; }
					//Define additional per-shader macros:
					CE::List<Define> addMacros;
					addMacros.push_back({ L"TARGET", ModelToMacroName(shader.Model) });
					addMacros.push_back({ L"SHADER_FUNC_" + funcName, L"" });
					
					for (size_t a = 0; a < shader.DefineCount; a++)
					{
						addMacros.push_back({ CE::Helper::StringConvert::ConvertToT(shader.Defines[a].Name), CE::Helper::StringConvert::ConvertToT(shader.Defines[a].Value) });
					}

					bool skipDXC = false;
					//Define argument list:
					CE::List<std::wstring> args;
					args.push_back(L"-Zpr"); //Row Major matrix packing
					switch (shader.Model.Language)
					{
						case CE::Rendering::SHADER_HLSL:
							skipDXC = true;
							//Fall-Through
						case CE::Rendering::SHADER_DXIL:
							profile += L"_" + std::to_wstring(shader.Model.Major) + L"_" + std::to_wstring(shader.Model.Minor);
							addMacros.push_back({ L"COMPILER",L"COMPILER_" + std::to_wstring(shader.Model.Major) + L"_" + std::to_wstring(shader.Model.Minor) });
							break;
						case CE::Rendering::SHADER_SPIRV:
						case CE::Rendering::SHADER_GLSL:
						case CE::Rendering::SHADER_ESSL:
						case CE::Rendering::SHADER_MSL_MacOS:
						case CE::Rendering::SHADER_MSL_iOS:
							args.push_back(L"-spirv");
							if (shader.Model == CE::Rendering::MODEL_VULKAN_1_0) 
							{ 
								args.push_back(L"-fspv-target-env=vulkan1.0"); 
							}
							else 
							{ 
								args.push_back(L"-fspv-target-env=vulkan1.1"); 
							}
							args.push_back(L"-fvk-invert-y"); //Invert y-coordinate to mirror DirectX coordinate system
							args.push_back(L"-fvk-use-dx-position-w"); //Reciprocate SV_Position.w in PS to mirror how it's working DirectX
							//List of available extensions: https://www.khronos.org/registry/spir-v/#extensions
							args.push_back(L"-fspv-extension=SPV_EXT_shader_stencil_export"); // SV_StencilRef
							args.push_back(L"-fspv-extension=SPV_EXT_shader_viewport_index_layer"); // Viewport Index
							args.push_back(L"-fspv-extension=SPV_EXT_fragment_fully_covered"); // Fragment fully covered
							args.push_back(L"-fspv-extension=SPV_EXT_descriptor_indexing"); // Non-uniform variables/instructions
							args.push_back(L"-fspv-extension=SPV_GOOGLE_hlsl_functionality1"); // CounterBuffer and Semantics
							args.push_back(L"-fspv-extension=KHR"); //Enable all KHR extensions

							profile += DEFAULT_SHADER_VERSION;
							addMacros.push_back({ L"COMPILER",std::wstring(L"COMPILER") + DEFAULT_SHADER_VERSION });
							break;
						default:
							CLOAK_ASSUME(false);
							break;
					}

					if (IsFlagSet(options, OPTION_OPTIMIZATION_MASK, OPTION_NONE)) { options |= OPTION_OPTIMIZATION_LEVEL_HIGHEST; }
					if (IsFlagSet(options, OPTION_DEBUG)) 
					{ 
						args.push_back(L"-Zi"); //Enable debug information output
						args.push_back(L"-Zss"); //Build debug names based on source code information
						args.push_back(L"-Qembed_debug"); 
					}  
					if (IsFlagSet(options, OPTION_DEBUG_SKIP_OPTIMIZATION)) 
					{ 
						if (IsFlagSet(options, OPTION_DEBUG)) { args.push_back(L"-Od"); options &= ~OPTION_OPTIMIZATION_MASK; }
						else { compiler.SendResponse(threadID, shaderID, shader, config, Status::WARNING, "Optimization can only be skipped during Debugging!"); }
					}
					if (IsFlagSet(options, OPTION_OPTIMIZATION_MASK, OPTION_NONE)) { }
					else if (IsFlagSet(options, OPTION_OPTIMIZATION_MASK, OPTION_OPTIMIZATION_LEVEL_0)) { args.push_back(L"-O0"); }
					else if (IsFlagSet(options, OPTION_OPTIMIZATION_MASK, OPTION_OPTIMIZATION_LEVEL_1)) { args.push_back(L"-O1"); }
					else if (IsFlagSet(options, OPTION_OPTIMIZATION_MASK, OPTION_OPTIMIZATION_LEVEL_2)) { args.push_back(L"-O2"); }
					else if (IsFlagSet(options, OPTION_OPTIMIZATION_MASK, OPTION_OPTIMIZATION_LEVEL_3)) { args.push_back(L"-O3"); }
					else { compiler.SendResponse(threadID, shaderID, shader, config, Status::FAILED, "Optimization setting is unsupported!"); return false; }

					if (IsFlagSet(options, OPTION_16_BIT_TYPES) && shader.Model >= CE::Rendering::MODEL_HLSL_6_2) { args.push_back(L"-enable-16bit-types"); }

					//Read file:
					CE::RefPointer<CE::Files::IReader> read = nullptr;
					CREATE_INTERFACE(CE_QUERY_ARGS(&read));
					if (read->SetTarget(shader.File, CE::Files::CompressType::NONE, true) > 0)
					{
						compiler.SendResponse(threadID, shaderID, shader, config, "Reading file...");

						std::string srcFile = "";
						while (read->IsAtEnd() == false) { ToString(&srcFile, static_cast<char32_t>(read->ReadBits(32))); }

						CE::RefPointer<IDxcBlobEncoding> source = nullptr;
						HRESULT hRet = compiler.GetLibrary(threadID)->CreateBlobWithEncodingOnHeapCopy(srcFile.c_str(), static_cast<UINT32>(srcFile.length()), CP_UTF8, &source);
						if (FAILED(hRet) || source->GetBufferSize() < 4) { compiler.SendResponse(threadID, shaderID, shader, config, Status::FAILED, "Failed to read source file"); goto failed; }

						const std::wstring fileName = CE::Helper::StringConvert::ConvertToW(shader.File.GetName());

						//Create actual DXC defines:
						CE::List<DxcDefine> dxcMacros;
						dxcMacros.reserve(macros.size() + addMacros.size());
						for (size_t a = 0; a < macros.size(); a++) { dxcMacros.push_back({ macros[a].Name.c_str(), macros[a].Value.c_str() }); }
						for (size_t a = 0; a < addMacros.size(); a++) { dxcMacros.push_back({ addMacros[a].Name.c_str(), addMacros[a].Value.c_str() }); }
						//Create actual DXC argument list:
						CE::List<const wchar_t*> dxcArgs;
						for (size_t a = 0; a < args.size(); a++) { dxcArgs.push_back(args[a].c_str()); }

						compiler.SendResponse(threadID, shaderID, shader, config, "Preprocessing...");

						CE::RefPointer<IncludeHandler> include = new IncludeHandler(threadID, &compiler, shader.File, 0);
						CE::RefPointer<IDxcOperationResult> result = nullptr;
						hRet = compiler.GetCompiler(threadID)->Preprocess(source, fileName.c_str(), dxcArgs.data(), static_cast<UINT32>(dxcArgs.size()), dxcMacros.data(), static_cast<UINT32>(dxcMacros.size()), include, &result);
						include->Release();
						if (FAILED(hRet)) { CLOAK_ASSUME(false); return false; }

						HRESULT hStatus;
						hRet = result->GetStatus(&hStatus);
						if (FAILED(hRet)) { CLOAK_ASSUME(false); return false; }

						CE::RefPointer<IDxcBlobEncoding> error = nullptr;
						hRet = result->GetErrorBuffer(&error);
						if (FAILED(hRet)) { CLOAK_ASSUME(false); return false; }
						std::string msg = ToString(threadID, compiler, error);
						
						if (FAILED(hStatus))
						{
							compiler.SendResponse(threadID, shaderID, shader, config, Status::FAILED, msg);
							goto failed;
						}
						else if (msg.length() > 0)
						{
							compiler.SendResponse(threadID, shaderID, shader, config, Status::WARNING, msg);
						}
						error = nullptr;
						
						CE::RefPointer<IDxcBlob> processedSource = nullptr;
						hRet = result->GetResult(&processedSource);
						if (FAILED(hRet)) { CLOAK_ASSUME(false); return false; }
						result = nullptr;

						CE::RefPointer<Blob> resCode = nullptr;
						//TODO: Check temp file
						{
							CE::RefPointer<IDxcBlob> code = nullptr;
							compiler.SendResponse(threadID, shaderID, shader, config, "Compiling...");
							if (skipDXC == false)
							{
								hRet = compiler.GetCompiler(threadID)->Compile(processedSource, fileName.c_str(), funcName.c_str(), profile.c_str(), dxcArgs.data(), static_cast<UINT32>(dxcArgs.size()), nullptr, 0, nullptr, &result);
								if (FAILED(hRet)) { CLOAK_ASSUME(false); return false; }

								hRet = result->GetStatus(&hStatus);
								if (FAILED(hRet)) { CLOAK_ASSUME(false); return false; }

								hRet = result->GetErrorBuffer(&error);
								if (FAILED(hRet)) { CLOAK_ASSUME(false); return false; }

								msg = ToString(threadID, compiler, error);

								if (FAILED(hStatus))
								{
									compiler.SendResponse(threadID, shaderID, shader, config, Status::FAILED, msg);
									goto failed;
								}
								else if (msg.length() > 0)
								{
									compiler.SendResponse(threadID, shaderID, shader, config, Status::WARNING, msg);
								}

								hRet = result->GetResult(&code);
								if (FAILED(hRet)) { CLOAK_ASSUME(false); return false; }
							}

							switch (shader.Model.Language)
							{
								case CloakEngine::API::Rendering::SHADER_DXIL: 
								case CloakEngine::API::Rendering::SHADER_SPIRV:
								{
									compiler.SendResponse(threadID, shaderID, shader, config, Status::FINISHED);
									resCode = new Blob(code, 0);
									goto write_shader;
								}
								case CloakEngine::API::Rendering::SHADER_HLSL:
								{
									CE::RefPointer<IDxcBlobEncoding> processedSourceUtf8 = nullptr;
									hRet = compiler.GetLibrary(threadID)->GetBlobAsUtf8(processedSource, &processedSourceUtf8);
									processedSource->Release();
									if (FAILED(hRet)) { CLOAK_ASSUME(false); return false; }

									if (ConvertedCompiler::HLSL(threadID, compiler, processedSourceUtf8->GetBufferSize(), processedSourceUtf8->GetBufferPointer(), profile, shader, config, options, shaderName, shaderID, &resCode) == true) { goto write_shader; }
									goto failed;
								}
								case CloakEngine::API::Rendering::SHADER_GLSL:
								case CloakEngine::API::Rendering::SHADER_ESSL:
								case CloakEngine::API::Rendering::SHADER_MSL_MacOS:
								case CloakEngine::API::Rendering::SHADER_MSL_iOS:
								{
									CE::RefPointer<Blob> convertedSource = nullptr;
									if (ConvertBinary(compiler, threadID, code, shader, config, shaderID, &convertedSource) == false) { goto failed; }
									switch (shader.Model.Language)
									{
										case CloakEngine::API::Rendering::SHADER_GLSL:
										{
											compiler.SendResponse(threadID, shaderID, shader, config, Status::FAILED, "GLSL Compiler is currently not implemented!");
											goto failed;
										}
										case CloakEngine::API::Rendering::SHADER_ESSL:
										{
											compiler.SendResponse(threadID, shaderID, shader, config, Status::FAILED, "ESSL Compiler is currently not implemented!");
											goto failed;
										}
										case CloakEngine::API::Rendering::SHADER_MSL_MacOS:
										{
											compiler.SendResponse(threadID, shaderID, shader, config, Status::FAILED, "MSL Compiler is currently not implemented!");
											goto failed;
										}
										case CloakEngine::API::Rendering::SHADER_MSL_iOS:
										{
											compiler.SendResponse(threadID, shaderID, shader, config, Status::FAILED, "MSL Compiler is currently not implemented!");
											goto failed;
										}
										case CloakEngine::API::Rendering::SHADER_HLSL:
										case CloakEngine::API::Rendering::SHADER_DXIL:
										case CloakEngine::API::Rendering::SHADER_SPIRV:
										default: CLOAK_ASSUME(false); break;
									}
									break;
								}
								default: CLOAK_ASSUME(false); return false;
							}
						write_shader:
							if (resCode == nullptr) { goto failed; }
							//TODO: Write temp file
						}
						CLOAK_ASSUME(resCode != nullptr);
						WriteShader(file, header, ComposeShaderName(shader.Model, type, config), tabCount, resCode->GetBufferSize(), reinterpret_cast<const uint8_t*>(resCode->GetBufferPointer()));
						return true;
					}
					compiler.SendResponse(threadID, shaderID, shader, config, Status::FAILED, "Source file not found");
				failed:
					WriteShader(file, header, ComposeShaderName(shader.Model, type, config), tabCount, 0, nullptr);
					return true;
				}
				void CLOAK_CALL CompileShaderConfiguration(In const Compiler& compiler, In size_t threadID, In CE::Files::IMultithreadedWriteBuffer* fileBuffer, In CE::Files::IMultithreadedWriteBuffer* headBuffer, In const CE::Global::Task& finish, In const EncodeDesc& encode, In const SHADER_DESC& shader, In const Configuration& config, In size_t tabCount, In ShaderPass target, In const std::string& shaderName, In size_t shaderID)
				{
					//Create profile name:
					std::wstring profile = L"";
					switch (target)
					{
						case CloakCompiler::API::Shader::Shader_v1003::ShaderPass::VS: profile = L"vs"; break;
						case CloakCompiler::API::Shader::Shader_v1003::ShaderPass::HS: profile = L"hs"; break;
						case CloakCompiler::API::Shader::Shader_v1003::ShaderPass::DS: profile = L"ds"; break;
						case CloakCompiler::API::Shader::Shader_v1003::ShaderPass::GS: profile = L"gs"; break;
						case CloakCompiler::API::Shader::Shader_v1003::ShaderPass::PS: profile = L"ps"; break;
						case CloakCompiler::API::Shader::Shader_v1003::ShaderPass::CS: profile = L"cs"; break;
						default: CLOAK_ASSUME(false); return; //Unknown target. Should not happen
					}
					//Create macro list:
					CE::List<Define> macros;
					if (IsFlagSet(shader.Options, OPTION_DEBUG)) 
					{ 
						macros.push_back({ L"DEBUG", L"" });
					}
					if (IsFlagSet(shader.Type, SHADER_TYPE_COMPLEX))
					{
						if (config.GetComplex()) { macros.push_back({ L"OPTION_COMPLEX", L"" }); }
						else { macros.push_back({ L"OPTION_SIMPLE", L"" }); }
					}
					if (IsFlagSet(shader.Type, SHADER_TYPE_TESSELLATION) && config.GetTessellation())
					{
						macros.push_back({ L"OPTION_TESSELLATION", L"" });
					}
					if (IsFlagSet(shader.Type, SHADER_TYPE_MSAA) && config.GetMSAA() > 1)
					{
						macros.push_back({ L"OPTION_MSAA", L"" });
						macros.push_back({ L"SHADER_MSAA", L"(" + std::to_wstring(config.GetMSAA()) + L")" });
						macros.push_back({ L"MAX_COVERAGE", L"(" + std::to_wstring((1 << config.GetMSAA()) - 1) + L")" });
						macros.push_back({ L"MSAA_FACTOR", L"(" + std::to_wstring(1.0f / config.GetMSAA()) + L"f)" });
					}
					else
					{
						macros.push_back({ L"MAX_COVERAGE", L"(1)" });
						macros.push_back({ L"MSAA_FACTOR", L"(1.0f)" });
					}
					if(IsFlagSet(shader.Type, SHADER_TYPE_WAVESIZE))
					{
						macros.push_back({ L"LANE_COUNT", L"(" + std::to_wstring(config.GetWaveSize()) + L")" });
					}
					for (size_t a = 0; a < ARRAYSIZE(CE::Rendering::SHADER_VERSIONS); a++) 
					{ 
						macros.push_back({ ModelToMacroName(CE::Rendering::SHADER_VERSIONS[a]), L"(" + std::to_wstring(a) + L")" }); 
						if (CE::Rendering::SHADER_VERSIONS[a].Language == CE::Rendering::SHADER_DXIL || CE::Rendering::SHADER_VERSIONS[a].Language == CE::Rendering::SHADER_HLSL)
						{
							if (CE::Rendering::SHADER_VERSIONS[a].Patch == 0)
							{
								macros.push_back({ L"COMPILER_" + std::to_wstring(CE::Rendering::SHADER_VERSIONS[a].Major) + L"_" + std::to_wstring(CE::Rendering::SHADER_VERSIONS[a].Minor), L"(" + std::to_wstring(CE::Rendering::SHADER_VERSIONS[a].FullVersion()) + L")" });
							}
							else
							{
								macros.push_back({ L"COMPILER_" + std::to_wstring(CE::Rendering::SHADER_VERSIONS[a].Major) + L"_" + std::to_wstring(CE::Rendering::SHADER_VERSIONS[a].Minor) + L"_" + std::to_wstring(CE::Rendering::SHADER_VERSIONS[a].Patch), L"(" + std::to_wstring(CE::Rendering::SHADER_VERSIONS[a].FullVersion()) + L")" });
							}
						}
					}

					for (size_t a = 0; a < ARRAYSIZE(CE::Rendering::SHADER_VERSIONS); a++)
					{
						const SHADER_NAME* shn = nullptr;
						if (FindShader(shader, CE::Rendering::SHADER_VERSIONS[a], target, config, &shn) == true)
						{
							CE::Global::Task t = [fileBuffer, headBuffer, a, &compiler, &encode, shn, config, &shader, macros, profile, &shaderName, tabCount, target, shaderID](In size_t threadID) {
								CE::RefPointer<CE::Files::IWriter> f = fileBuffer == nullptr ? nullptr : fileBuffer->CreateLocalWriter(reinterpret_cast<void*>(static_cast<uintptr_t>(a)));
								CE::RefPointer<CE::Files::IWriter> h = headBuffer == nullptr ? nullptr : headBuffer->CreateLocalWriter();
								if (shn != nullptr)
								{
									CompileSingleShader(threadID, compiler, f, h, encode, *shn, config, shader.Type, shader.Options, macros, profile, shaderName, tabCount, shaderID);
								}
								else
								{
									compiler.SendResponse(threadID, shaderID, target, CE::Rendering::SHADER_VERSIONS[a], config, Status::SKIPPED, "Not used");
									Library::WriteShader(h, ComposeShaderName(CE::Rendering::SHADER_VERSIONS[a], shader.Type, config), tabCount, 0, nullptr);
								}
								if (f != nullptr) { f->Save(); }
								if (h != nullptr) { h->Save(); }
								CLOAK_ASSUME(fileBuffer == nullptr || fileBuffer->GetRefCount() > 1);
								CLOAK_ASSUME(headBuffer == nullptr || headBuffer->GetRefCount() > 1);
							};
							finish.AddDependency(t);
							t.Schedule(threadID);
						}
						else
						{
							compiler.SendResponse(threadID, shaderID, target, CE::Rendering::SHADER_VERSIONS[a], config, Status::FAILED, "Usage mask is ambiguous");
						}
					}
				}
				CE::Global::Task CLOAK_CALL CompileShaderType(In const Compiler& compiler, In size_t threadID, In const CE::RefPointer<CloakEngine::Files::IWriter>& file, In const CE::RefPointer<CloakEngine::Files::IWriter>& header, In const EncodeDesc& encode, In const SHADER_DESC& shader, In size_t tabCount, In ShaderPass target, In const std::string& shaderName, In const ShaderCountInfo& passCount, In size_t shaderID)
				{
					if (header != nullptr)
					{
						std::string n = "";
						switch (target)
						{
							case CloakCompiler::API::Shader::Shader_v1003::ShaderPass::VS: n = "VS"; break;
							case CloakCompiler::API::Shader::Shader_v1003::ShaderPass::HS: n = "HS"; break;
							case CloakCompiler::API::Shader::Shader_v1003::ShaderPass::DS: n = "DS"; break;
							case CloakCompiler::API::Shader::Shader_v1003::ShaderPass::GS: n = "GS"; break;
							case CloakCompiler::API::Shader::Shader_v1003::ShaderPass::PS: n = "PS"; break;
							case CloakCompiler::API::Shader::Shader_v1003::ShaderPass::CS: n = "CS"; break;
							default: CLOAK_ASSUME(false); break;
						}
						Engine::Lib::WriteWithTabs(header, tabCount, "namespace " + n + " {");
					}

					CE::Files::IMultithreadedWriteBuffer* fileBuffer = nullptr;
					CE::Files::IMultithreadedWriteBuffer* headBuffer = nullptr;
					if (file != nullptr) { fileBuffer = CE::Files::CreateMultithreadedWriteBuffer(file); }
					if (header != nullptr) { headBuffer = CE::Files::CreateMultithreadedWriteBuffer(header); }
					CE::Global::Task finish = [fileBuffer, headBuffer, header, file, tabCount, &shader, passCount](In size_t threadID) {
						CLOAK_ASSUME(fileBuffer == nullptr || fileBuffer->GetRefCount() == 1);
						CLOAK_ASSUME(headBuffer == nullptr || headBuffer->GetRefCount() == 1);
						SAVE_RELEASE(fileBuffer);
						SAVE_RELEASE(headBuffer);

						if (header != nullptr)
						{
							Library::WriteShaderArray(header, tabCount + 1, shader.Type, passCount);
							Engine::Lib::WriteWithTabs(header, tabCount, "}");
							header->Save();
						}
						if (file != nullptr) { file->Save(); }
					};
					if (fileBuffer != nullptr) { fileBuffer->SetFinishTask(finish); }
					if (headBuffer != nullptr) { headBuffer->SetFinishTask(finish); }

					bool complex = false;
					bool tess = false;
					size_t msaa = 0;
					size_t wave = 0;
					while (true)
					{
						Configuration config;
						config.SetComplex(complex);
						config.SetTessellation(tess);
						config.SetMSAA(MSAA_COUNT[msaa].Value);
						config.SetWaveSize(WAVE_SIZES[wave].Value);

						if (IsValidConfig(shader.Type, config) == true && IsUsed(shader.Type, config, passCount) == true)
						{
							CompileShaderConfiguration(compiler, threadID, fileBuffer, headBuffer, finish, encode, shader, config, tabCount + 1, target, shaderName, shaderID);
						}

						if (IsFlagSet(shader.Type, SHADER_TYPE_TESSELLATION))
						{
							tess = !tess;
							if (tess != false) { continue; }
						}
						if (IsFlagSet(shader.Type, SHADER_TYPE_WAVESIZE))
						{
							wave++;
							if (wave < ARRAYSIZE(WAVE_SIZES)) { continue; }
							wave = 0;
						}
						if (IsFlagSet(shader.Type, SHADER_TYPE_MSAA))
						{
							msaa++;
							if (msaa < ARRAYSIZE(MSAA_COUNT)) { continue; }
							msaa = 0;
						}
						if (IsFlagSet(shader.Type, SHADER_TYPE_COMPLEX))
						{
							complex = !complex;
							if (complex != false) { continue; }
						}
						break;
					}
					finish.Schedule(threadID);
					return finish;
				}
				CE::Global::Task CLOAK_CALL CompileShader(In size_t threadID, In const Compiler& compiler, In const CE::RefPointer<CloakEngine::Files::IWriter>& file, In const CE::RefPointer<CloakEngine::Files::IWriter>& header, In const EncodeDesc& encode, In const SHADER_DESC& shader, In const std::string& shaderName, In size_t tabCount, In const ShaderPassCount& passCounts, In size_t shaderID)
				{
					CE::Files::IMultithreadedWriteBuffer* fileBuffer = nullptr;
					CE::Files::IMultithreadedWriteBuffer* headBuffer = nullptr;
					if (file != nullptr) { fileBuffer = CE::Files::CreateMultithreadedWriteBuffer(file, [file](void* userData, uint64_t size) { File::WriteShaderPass(file, static_cast<ShaderPass>(reinterpret_cast<uintptr_t>(userData)), size); }); }
					if (header != nullptr) { headBuffer = CE::Files::CreateMultithreadedWriteBuffer(header); }
					CE::Global::Task finish = [fileBuffer, headBuffer](In size_t threadID) {
						CLOAK_ASSUME(fileBuffer == nullptr || fileBuffer->GetRefCount() == 1);
						CLOAK_ASSUME(headBuffer == nullptr || headBuffer->GetRefCount() == 1);
						SAVE_RELEASE(fileBuffer);
						SAVE_RELEASE(headBuffer);
					};
					if (fileBuffer != nullptr) { fileBuffer->SetFinishTask(finish); }
					if (headBuffer != nullptr) { headBuffer->SetFinishTask(finish); }
					for (size_t a = 0; a < ARRAYSIZE(ALL_PASSES); a++)
					{
						if (passCounts.Pass[static_cast<uint32_t>(ALL_PASSES[a])].Count > 0)
						{
							CE::Global::Task t = [finish, fileBuffer, headBuffer, a, &compiler, &encode, &shader, tabCount, &shaderName, passCounts, shaderID](In size_t threadID) {
								CE::RefPointer<CE::Files::IWriter> f = fileBuffer == nullptr ? nullptr : fileBuffer->CreateLocalWriter(reinterpret_cast<void*>(static_cast<uintptr_t>(ALL_PASSES[a])));
								CE::RefPointer<CE::Files::IWriter> h = headBuffer == nullptr ? nullptr : headBuffer->CreateLocalWriter();
								finish.AddDependency(CompileShaderType(compiler, threadID, f, h, encode, shader, tabCount, ALL_PASSES[a], shaderName, passCounts.Pass[static_cast<uint32_t>(ALL_PASSES[a])], shaderID));
							};
							finish.AddDependency(t);
							t.Schedule(threadID);
						}
					}
					finish.Schedule(threadID);
					return finish;
				}
				ShaderCountInfo CLOAK_CALL CountShaderPass(In const SHADER_DESC& shader, In ShaderPass pass)
				{
					ShaderCountInfo r;
					r.Count = 0;
					r.Usage = USAGE_MASK_NEVER;
					for (size_t a = 0; a < shader.ShaderCount; a++)
					{
						if (shader.Shaders[a].Pass == pass && shader.Shaders[a].Use != USAGE_MASK_NEVER) 
						{ 
							r.Count++; 
							r.Usage |= shader.Shaders[a].Use;
						}
					}
					return r;
				}
				ShaderPassCount CLOAK_CALL CountShaderPasses(In const SHADER_DESC& shader)
				{
					ShaderPassCount res;
					res.CS = CountShaderPass(shader, ShaderPass::CS);
					res.VS = CountShaderPass(shader, ShaderPass::VS);
					res.GS = CountShaderPass(shader, ShaderPass::GS);
					res.DS = CountShaderPass(shader, ShaderPass::DS);
					res.HS = CountShaderPass(shader, ShaderPass::HS);
					res.PS = CountShaderPass(shader, ShaderPass::PS);

					res.Summed.Count = 0;
					res.Summed.Usage = USAGE_MASK_NEVER;
					for (size_t a = 0; a < ARRAYSIZE(res.Pass); a++)
					{
						res.Summed.Count += res.Pass[a].Count;
						res.Summed.Usage |= res.Pass[a].Usage;
					}
					return res;
				}

				CLOAKCOMPILER_API void CLOAK_CALL Encode(In CloakEngine::Files::IWriter* output, In const EncodeDesc& encode, In const SHADER_ENCODE& desc, In const CE::List<SHADER>& shaders, In std::function<void(const Response&)> response)
				{
					CE::RefPointer<CloakEngine::Files::IWriter> header = nullptr;
					CE::RefPointer<CloakEngine::Files::IWriter> file = nullptr;
					size_t tabCount = 0;
					if ((desc.Mode & WRITE_MODE_FILE) != 0) { file = output; }
					if ((desc.Mode & WRITE_MODE_LIB) != 0)
					{
						CREATE_INTERFACE(CE_QUERY_ARGS(&header));
						header->SetTarget(desc.Lib.Path.Append(desc.ShaderName), "H", CloakEngine::Files::CompressType::NONE, true);
						Engine::Lib::WriteHeaderIntro(header, Engine::Lib::Type::Shader, desc.ShaderName, desc.Lib.Path.GetHash());

						Engine::Lib::WriteWithTabs(header, 0, "#ifdef CLOAKENGINE_VERSION");
						Engine::Lib::WriteWithTabs(header, 1, "#include \"CloakEngine/Defines.h\"");
						Engine::Lib::WriteWithTabs(header, 1, "#include \"CloakEngine/Helper/Types.h\"");
						Engine::Lib::WriteWithTabs(header, 1, "#include \"CloakEngine/Rendering/Defines.h\"");
						Engine::Lib::WriteWithTabs(header, 1, "#include \"CloakEngine/Global/Graphic.h\"");
						Engine::Lib::WriteWithTabs(header, 0, "#endif");
						Engine::Lib::WriteWithTabs(header, 0, "");
						Engine::Lib::WriteWithTabs(header, 0, "#ifdef _MSC_VER");
						Engine::Lib::WriteWithTabs(header, 1, "#pragma comment(lib, \"" + desc.ShaderName + ".lib\")");
						Engine::Lib::WriteWithTabs(header, 0, "#endif");
						Engine::Lib::WriteWithTabs(header, 0, "");
						tabCount = Library::WriteCommonIntro(header, desc.ShaderName, 0, true);
						Engine::Lib::WriteWithTabs(header, tabCount, "inline namespace Shaders {");
					}
					if (file != nullptr) { file->WriteDynamic(shaders.size()); }

					CE::Files::IMultithreadedWriteBuffer* fileBuffer = nullptr;
					if (file != nullptr) { fileBuffer = CE::Files::CreateMultithreadedWriteBuffer(file, [file, &shaders](void* data, uint64_t size) { File::WriteShaderStart(file, shaders[reinterpret_cast<uintptr_t>(data)], size); }, CE::Global::Threading::Flag::IO); }
					CE::Files::IMultithreadedWriteBuffer* headBuffer = nullptr;
					if (header != nullptr) { headBuffer = CE::Files::CreateMultithreadedWriteBuffer(header); }
					CE::Global::Task finish = [fileBuffer, headBuffer, header, file, desc, tabCount](In size_t threadID) {
						CLOAK_ASSUME(fileBuffer == nullptr || fileBuffer->GetRefCount() == 1);
						CLOAK_ASSUME(headBuffer == nullptr || headBuffer->GetRefCount() == 1);
						SAVE_RELEASE(fileBuffer);
						SAVE_RELEASE(headBuffer);
						if (header != nullptr)
						{
							Engine::Lib::WriteWithTabs(header, tabCount, "}");
							Engine::Lib::WriteCommonOutro(header, 3);
							if (desc.Lib.WriteNamespaceAlias == true)
							{
								Engine::Lib::WriteWithTabs(header, 0, "//Namespace Alias:");
								Engine::Lib::WriteWithTabs(header, 0, "namespace " + desc.ShaderName + " = CloakEngine::Shader::" + desc.ShaderName + ";");
							}

							Engine::Lib::WriteHeaderOutro(header, 0);
							header->Save();
						}
					};
					if (fileBuffer != nullptr) { fileBuffer->SetFinishTask(finish); }
					if (headBuffer != nullptr) { headBuffer->SetFinishTask(finish); }
					
					//Shader compiling:
					ResponseHandler rspH = ResponseHandler(response);
					Compiler compiler = Compiler(&rspH);
					for (size_t a = 0; a < shaders.size(); a++)
					{
						CE::Global::Task task = [finish, &compiler, &shaders, a, headBuffer, fileBuffer, &desc, &encode, tabCount](In size_t threadID) {
							CE::RefPointer<CE::Files::IWriter> f = fileBuffer == nullptr ? nullptr : fileBuffer->CreateLocalWriter(reinterpret_cast<void*>(static_cast<uintptr_t>(a)));
							CE::RefPointer<CE::Files::IWriter> h = headBuffer == nullptr ? nullptr : headBuffer->CreateLocalWriter();
							ShaderPassCount passCounts = CountShaderPasses(shaders[a].Desc);
							CE::RefPointer<CloakEngine::Files::IWriter> srcHeader = nullptr;
							if (h != nullptr) { srcHeader = Library::WriteSourceFile(h, desc.Lib.Path, shaders[a].Name, desc.ShaderName, tabCount + 1); }
							CE::Global::Task cmp = [srcHeader, passCounts, a, &shaders, f, h, tabCount](In size_t threadID) {
								if (srcHeader != nullptr)
								{
									if (passCounts.Summed.Count > 0) { Engine::Lib::WriteWithTabs(srcHeader, tabCount + 2, "}"); }
								
									Library::WriteShaderGetter(srcHeader, tabCount + 2, shaders[a].Desc.Type, passCounts);
									Library::CloseSourceFile(srcHeader, tabCount + 2);
									srcHeader->Save();
								}
								if (f != nullptr) { f->Save(); }
								if (h != nullptr) { h->Save(); }
							};
							if (passCounts.Summed.Count > 0 && (f != nullptr || srcHeader != nullptr))
							{
								if (srcHeader != nullptr) { Engine::Lib::WriteWithTabs(srcHeader, tabCount + 2, "namespace Code {"); }
								cmp.AddDependency(CompileShader(threadID, compiler, f, srcHeader, encode, shaders[a].Desc, shaders[a].Name, tabCount + 3, passCounts, a));
							}
							finish.AddDependency(cmp);
							cmp.Schedule(CE::Global::Threading::Flag::IO, threadID);
						};
						finish.AddDependency(task);
						task.Schedule(CE::Global::Threading::Flag::IO);
					}
					if (header != nullptr) 
					{ 
						finish.AddDependency(Library::WriteCMakeFile(shaders, desc.Lib.Path, desc.ShaderName, desc.Lib.SharedLib)); 
						finish.AddDependency(Library::WriteCommonFile(desc.Lib.Path, desc.ShaderName)); 
					}
					finish.Schedule(CE::Global::Threading::Flag::IO);
					finish.WaitForExecution();
					if ((desc.Mode & WRITE_MODE_LIB) != 0) { Engine::Lib::Compile(desc.Lib.Path, desc.Lib.Generator); }
				}
			}
		}
	}
}
