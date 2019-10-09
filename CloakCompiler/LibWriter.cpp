#include "stdafx.h"
#include "Engine/LibWriter.h"

#include <string_view>
#include <sstream>

namespace CloakCompiler {
	namespace Engine {
		namespace Lib {
			constexpr std::string_view CODE_SUB_DIR = ".src/code/";
			constexpr std::string_view SOURCE_SUB_DIR = CODE_SUB_DIR.substr(0, 5);
			constexpr std::string_view CODE_SUB_DIR_NAME = CODE_SUB_DIR.substr(5, 4);

			static_assert(SOURCE_SUB_DIR[SOURCE_SUB_DIR.length() - 1] == '/', "SOURCE_SUB_DIR must be a dictionary (end with '/')");
			static_assert(CODE_SUB_DIR[CODE_SUB_DIR.length() - 1] == '/', "CODE_SUB_DIR must be a dictionary (end with '/')");
			static_assert(CODE_SUB_DIR_NAME[0] != '/' && CODE_SUB_DIR_NAME[CODE_SUB_DIR_NAME.length() - 1] != '/', "CODE_SUB_DIR_NAME must not start or end with '/'");

			enum class Platform {
				Windows_10,
				Windows_8_1,
				Windows_8,
			};
			enum class LibConfiguration {
				Debug = 1 << 0,
				Release = 1 << 1,
			};
			DEFINE_ENUM_FLAG_OPERATORS(LibConfiguration);
			struct CommandVariant {
				std::string BuildDir = "";
				std::string Generator = "";
				std::string Options = "";
			};

			inline std::string CLOAK_CALL CMakeString(In const std::string& s)
			{
				std::stringstream r;
				for (size_t a = 0; a < s.length(); a++)
				{
					switch (s[a])
					{
						case ' ': r << "\\ "; break;
						default: r << s[a]; break;
					}
				}
				return r.str();
			}
			inline void CLOAK_CALL ExecuteCompile(In const CommandVariant& baseSetting, In const CommandVariant& variant, In LibConfiguration config, In const CE::Files::UFI& srcPath)
			{
				std::string buildDir = baseSetting.BuildDir + variant.BuildDir;
				std::string generator = baseSetting.Generator + variant.Generator;
				std::string options = baseSetting.Options + variant.Options;

				if (IsFlagSet(config, LibConfiguration::Debug | LibConfiguration::Release, LibConfiguration::Debug))
				{
					buildDir += "bD";
					options += "-DCMAKE_BUILD_TYPE:STRING=Debug ";
				}
				if (IsFlagSet(config, LibConfiguration::Debug | LibConfiguration::Release, LibConfiguration::Release))
				{
					buildDir += "bR";
					options += "-DCMAKE_BUILD_TYPE:STRING=Release ";
				}
				std::string cmd = "";
#if CHECK_OS(WINDOWS,0)
				cmd = "start cmd /k \"" + srcPath.GetRoot(); //keep the cmd-box open in case any error occures (option /k)
				cmd += " && cd " + srcPath.ToString();
				cmd += " && rd /s /q " + buildDir;
				cmd += " & md " + buildDir; //Only one '&' since md might fail with "Dictory not found"
				cmd += " & cd " + buildDir; //Only one '&' since md might fail with "Dictory already exists"
				cmd += " && cmake " + options + "-G \"" + generator + "\" ..";
				if (IsFlagSet(config, LibConfiguration::Debug))
				{
					cmd += " && cmake --build . --config Debug -- /m";
				}
				if (IsFlagSet(config, LibConfiguration::Release))
				{
					cmd += " && cmake --build . --config Release -- /m";
				}
				cmd += " && exit\""; //exit the cmd-box manually if no error occured
#endif
				if (cmd.length() > 0) { std::system(cmd.c_str()); }
			}
			inline void CLOAK_CALL Compile(In const CE::Files::UFI& path, In API::LibGenerator generator, In Platform platform)
			{
				CE::Files::UFI srcPath = path.Append(SOURCE_SUB_DIR);
				CommandVariant BaseSettings;
				CE::List<CommandVariant> variants;
				bool multiConfigGenerator = true;

				switch (generator)
				{
					case API::LibGenerator::Ninja:
						BaseSettings.BuildDir = "N";
						BaseSettings.Generator = "Ninja";
						multiConfigGenerator = false;
						break;
					case API::LibGenerator::VisualStudio_2015:
						BaseSettings.BuildDir = "VS14";
						BaseSettings.Generator = "Visual Studio 14 2015";
						BaseSettings.Options = "";
						variants.push_back({ "x64", " Win64", "" });
						variants.push_back({ "x86", " Win32", "" });
						break;
					case API::LibGenerator::VisualStudio_2017:
						BaseSettings.BuildDir = "VS15";
						BaseSettings.Generator = "Visual Studio 15 2017";
						BaseSettings.Options = "";
						variants.push_back({ "x64", " Win64", "" });
						variants.push_back({ "x86", " Win32", "" });
						break;
					case API::LibGenerator::VisualStudio_2019:
						BaseSettings.BuildDir = "VS16";
						BaseSettings.Generator = "Visual Studio 16 2019";
						BaseSettings.Options = "";
						variants.push_back({ "x64", "", "-A x64 " });
						variants.push_back({ "x86", "", "-A Win32 " });
						break;
					default: CLOAK_ASSUME(false); return;
				}
				if (variants.size() == 0) { variants.push_back({ "", "", "" }); }

				switch (platform)
				{
					case Platform::Windows_10:
						BaseSettings.Options += "-DCMAKE_SYSTEM_NAME=Windows -DCMAKE_SYSTEM_VERSION=10 ";
						break;
					case Platform::Windows_8_1:
						BaseSettings.Options += "-DCMAKE_SYSTEM_NAME=Windows -DCMAKE_SYSTEM_VERSION=8.1 ";
						break;
					case Platform::Windows_8:
						BaseSettings.Options += "-DCMAKE_SYSTEM_NAME=Windows -DCMAKE_SYSTEM_VERSION=8 ";
						break;
					default:
						CLOAK_ASSUME(false);
						break;
				}

				for (size_t a = 0; a < variants.size(); a++)
				{
					if (multiConfigGenerator == true)
					{
						ExecuteCompile(BaseSettings, variants[a], LibConfiguration::Debug | LibConfiguration::Release, srcPath);
					}
					else
					{
						ExecuteCompile(BaseSettings, variants[a], LibConfiguration::Debug, srcPath);
						ExecuteCompile(BaseSettings, variants[a], LibConfiguration::Release, srcPath);
					}
				}
			}

			CLOAK_CALL CMakeFile::CMakeFile(In const std::string& projName) : m_projName(CMakeString(projName)), m_ready(false) {}
			CLOAK_CALL CMakeFile::CMakeFile(In Type type, In const CE::Files::UFI& path, In const std::string& projName, In bool shared) : CMakeFile(projName) { Create(type, path, shared); }
			void CLOAK_CALL CMakeFile::Create(In Type type, In const CE::Files::UFI& path, In bool shared)
			{
				if (m_ready == false)
				{
					const std::string targetDirX64 = CMakeString(path.Append("$<CMAKE_SYSTEM_NAME>/x64/$<TARGET_SHORT_NAME> $<CONFIG>").ToString());
					const std::string targetDirX86 = CMakeString(path.Append("$<CMAKE_SYSTEM_NAME>/$<$<CMAKE_SYSTEM_NAME:Windows>:Win32>$<$<NOT:$<CMAKE_SYSTEM_NAME:Windows>>:x86>/$<TARGET_SHORT_NAME> $<CONFIG>").ToString());

					std::string pName = m_projName;
					switch (type)
					{
						case CloakCompiler::Engine::Lib::Type::Shader: pName += "_ShaderLib"; break;
						case CloakCompiler::Engine::Lib::Type::Image: pName += "_ImageLib"; break;
						default: CLOAK_ASSUME(false); break;
					}

					CE::RefPointer<CE::Files::IWriter> mainWriter = nullptr;
					CREATE_INTERFACE(CE_QUERY_ARGS(&mainWriter));
					mainWriter->SetTarget(path.Append(std::string(SOURCE_SUB_DIR) + "CMakeLists"), "TXT", CE::Files::CompressType::NONE, true);
					WriteWithTabs(mainWriter, 0, "cmake_minimum_required(VERSION 3.13)\n");
					WriteWithTabs(mainWriter, 0, "cmake_policy(SET CMP0076 NEW)");
					WriteWithTabs(mainWriter, 0, "");
					WriteWithTabs(mainWriter, 0, "project(" + CMakeString(pName) + " VERSION 1.0 LANGUAGES CXX)\n");
					WriteWithTabs(mainWriter, 0, "");
					WriteWithTabs(mainWriter, 0, "add_library(" + m_projName + " " + std::string(shared == true ? "SHARED" : "STATIC") + ")");
					WriteWithTabs(mainWriter, 0, "target_compile_features(" + m_projName + " PUBLIC cxx_std_14)");
					WriteWithTabs(mainWriter, 0, "set_target_properties(" + m_projName + " PROPERTIES CXX_EXTENSIONS OFF)");

					WriteWithTabs(mainWriter, 0, "string(COMPARE EQUAL \"${CMAKE_SYSTEM_NAME}\" \"Windows\" _res0)");
					WriteWithTabs(mainWriter, 0, "if(_res0)");
					WriteWithTabs(mainWriter, 1, "if(CMAKE_SYSTEM_VERSION VERSION_EQUAL 10)");
					WriteWithTabs(mainWriter, 2, "set(TARGET_SHORT_NAME \"W10\"");
					WriteWithTabs(mainWriter, 1, "elseif(CMAKE_SYSTEM_VERSION VERSION_EQUAL 8.1)");
					WriteWithTabs(mainWriter, 2, "set(TARGET_SHORT_NAME \"W8.1\"");
					WriteWithTabs(mainWriter, 1, "elseif(CMAKE_SYSTEM_VERSION VERSION_EQUAL 8)");
					WriteWithTabs(mainWriter, 2, "set(TARGET_SHORT_NAME \"W8\"");
					WriteWithTabs(mainWriter, 1, "else()");
					WriteWithTabs(mainWriter, 2, "message(FATAL_ERROR \"Unknown Platform Version, can't configurate folder path!\")");
					WriteWithTabs(mainWriter, 1, "endif()");
					WriteWithTabs(mainWriter, 0, "endif()");

					WriteWithTabs(mainWriter, 0, "if(CMAKE_SIZEOF_VOID_P EQUAL 8)");
					WriteWithTabs(mainWriter, 1, "set_target_properties(" + m_projName + " PROPERTIES ARCHIVE_OUTPUT_DIRECTORY " + targetDirX64 + ")");
					WriteWithTabs(mainWriter, 1, "set_target_properties(" + m_projName + " PROPERTIES LIBRARY_OUTPUT_DIRECTORY " + targetDirX64 + ")");
					WriteWithTabs(mainWriter, 0, "else()");
					WriteWithTabs(mainWriter, 1, "set_target_properties(" + m_projName + " PROPERTIES ARCHIVE_OUTPUT_DIRECTORY " + targetDirX86 + ")");
					WriteWithTabs(mainWriter, 1, "set_target_properties(" + m_projName + " PROPERTIES LIBRARY_OUTPUT_DIRECTORY " + targetDirX86 + ")");
					WriteWithTabs(mainWriter, 0, "endif()");
					WriteWithTabs(mainWriter, 0, "");
					WriteWithTabs(mainWriter, 0, "add_subdirectory(" + CMakeString(std::string(CODE_SUB_DIR_NAME)) + ")");
					mainWriter->Save();

					CREATE_INTERFACE(CE_QUERY_ARGS(&m_writer));
					m_writer->SetTarget(path.Append(std::string(CODE_SUB_DIR) + "CMakeLists"), "TXT", CE::Files::CompressType::NONE, true);
					m_ready = true;
				}
			}
			void CLOAK_CALL CMakeFile::AddFile(In const std::string& name, In_opt bool publicFile) const
			{
				WriteWithTabs(m_writer, 0, "target_sources(" + m_projName + " " + (publicFile == true ? "PUBLIC" : "PRIVATE") + " " + CMakeString(name));
			}

			CLOAK_CALL CMakeFileAsync::CMakeFileAsync(In const std::string& projName) : CMakeFile(projName) {}
			CLOAK_CALL CMakeFileAsync::CMakeFileAsync(In Type type, In const CE::Files::UFI& path, In const std::string& projName, In bool shared, In const CE::Global::Task& finishTask) : CMakeFile(projName)
			{
				Create(type, path, shared, finishTask);
			}
			void CLOAK_CALL CMakeFileAsync::Create(In Type type, In const CE::Files::UFI& path, In bool shared, In const CE::Global::Task& finishTask)
			{
				if (m_ready == false)
				{
					CMakeFile::Create(type, path, shared);
					m_mtw = CE::Files::CreateMultithreadedWriteBuffer(m_writer);
					m_mtw->SetFinishTask(finishTask);
				}
			}
			void CLOAK_CALL CMakeFileAsync::AddFile(In const std::string& name, In_opt bool publicFile) const
			{
				if (m_ready == true)
				{
					CE::RefPointer<CE::Files::IWriter> w = m_mtw->CreateLocalWriter();
					WriteWithTabs(w, 0, "target_sources(" + m_projName + " " + (publicFile == true ? "PUBLIC" : "PRIVATE") + " " + CMakeString(name));
					w->Save();
				}
			}

			CE::RefPointer<CE::Files::IWriter> CLOAK_CALL CreateSourceFile(In const CE::Files::UFI& path, In const std::string& name, In const CloakEngine::Files::FileType& type)
			{
				CE::RefPointer<CE::Files::IWriter> res = nullptr;
				CREATE_INTERFACE(CE_QUERY_ARGS(&res));
				res->SetTarget(path.Append(std::string(CODE_SUB_DIR) + name), type, CE::Files::CompressType::NONE, true);
				return res;
			}
			void CLOAK_CALL WriteWithTabs(In const CE::RefPointer<CE::Files::IWriter>& header, In size_t tabs, In const std::string& text) 
			{ 
				header->WriteString(std::string(tabs, '\t') + text + "\n"); 
			}
			size_t CLOAK_CALL WriteCopyright(In const CE::RefPointer<CE::Files::IWriter>& file, In_opt size_t tabCount)
			{
				WriteWithTabs(file, tabCount, "///////////////////////////////////////////////////////////////////////////////////////////////////////////////");
				WriteWithTabs(file, tabCount, "//  This file was automaticly created with CloakEditor©. Do not change this file!                            //");
				WriteWithTabs(file, tabCount, "//                                                                                                           //");
				WriteWithTabs(file, tabCount, "//  Copyright 2019 Thomas Degener                                                                            //");
				WriteWithTabs(file, tabCount, "//                                                                                                           //");
				WriteWithTabs(file, tabCount, "//  THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,                      //");
				WriteWithTabs(file, tabCount, "//  INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR                 //");
				WriteWithTabs(file, tabCount, "//  PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE                 //");
				WriteWithTabs(file, tabCount, "//  FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,          //");
				WriteWithTabs(file, tabCount, "//  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.    //");
				WriteWithTabs(file, tabCount, "///////////////////////////////////////////////////////////////////////////////////////////////////////////////");
				WriteWithTabs(file, tabCount, "");
				return tabCount;
			}
			size_t CLOAK_CALL WriteHeaderIntro(In const CE::RefPointer<CE::Files::IWriter>& file, In Type type, In const std::string& name, In size_t hash, In_opt size_t tabCount)
			{
				std::string def = "CE_";
				switch (type)
				{
					case CloakCompiler::Engine::Lib::Type::Shader: def += "SHADER"; break;
					case CloakCompiler::Engine::Lib::Type::Image: def += "IMAGE"; break;
					default: CLOAK_ASSUME(false); return 0;
				}
				def += "_" + name + "_" + std::to_string(hash) + "_H";

				WriteCopyright(file);
				file->WriteString("#pragma once\n");
				file->WriteString("#ifndef " + def + "\n");
				file->WriteString("#define " + def + "\n");
				file->WriteString("\n");
				return 0;
			}
			size_t CLOAK_CALL WriteHeaderOutro(In const CE::RefPointer<CE::Files::IWriter>& file, In size_t tabCount)
			{
				file->WriteString("#endif\n");
				return 0;
			}
			size_t CLOAK_CALL WriteCommonIntro(In const CE::RefPointer<CE::Files::IWriter>& file, In Type type, In const std::string& name, In_opt size_t tabCount)
			{
				file->WriteString("#include <cstddef>\n");
				file->WriteString("#include <cstdint>\n");
				file->WriteString("\n");
				WriteWithTabs(file, tabCount, "namespace CloakEngine {");
				switch (type)
				{
					case CloakCompiler::Engine::Lib::Type::Shader: WriteWithTabs(file, tabCount + 1, "namespace Shader {"); break;
					case CloakCompiler::Engine::Lib::Type::Image: WriteWithTabs(file, tabCount + 1, "namespace Image {"); break;
					default: CLOAK_ASSUME(false); return tabCount + 1;
				}
				WriteWithTabs(file, tabCount + 2, "namespace " + name + " {");
				return tabCount + 3;
			}
			size_t CLOAK_CALL WriteCommonOutro(In const CE::RefPointer<CE::Files::IWriter>& file, In size_t tabCount)
			{
				if (tabCount >= 1) { WriteWithTabs(file, tabCount - 1, "}"); }
				if (tabCount >= 2) { WriteWithTabs(file, tabCount - 2, "}"); }
				if (tabCount >= 3) { WriteWithTabs(file, tabCount - 3, "}"); }
				return tabCount > 3 ? tabCount - 3 : 0;
			}
			void CLOAK_CALL Compile(In const CE::Files::UFI& path, In API::LibGenerator generator)
			{
				Compile(path, generator, Platform::Windows_10);
				//TODO: Add more platforms
			}
		}
	}
}