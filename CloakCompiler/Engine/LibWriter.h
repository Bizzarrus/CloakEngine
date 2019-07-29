#pragma once
#ifndef CC_E_LIBWRITER_H
#define CC_E_LIBWRITER_H

#include "CloakCompiler/Defines.h"
#include "CloakEngine/CloakEngine.h"
#include "CloakCompiler/EncoderBase.h"

#include <atomic>

namespace CloakCompiler {
	namespace Engine {
		namespace Lib {
			enum class Type {
				Shader,
				Image,
			};

			class CMakeFile {
				public:
					explicit CLOAK_CALL CMakeFile(In const std::string& projName);
					CLOAK_CALL CMakeFile(In Type type, In const CE::Files::UFI& path, In const std::string& projName, In bool shared);
					
					void CLOAK_CALL Create(In Type type, In const CE::Files::UFI& path, In bool shared);
					virtual void CLOAK_CALL AddFile(In const std::string& name, In_opt bool publicFile = false) const;
				protected:
					const std::string m_projName;
					std::atomic<bool> m_ready;
					CE::RefPointer<CE::Files::IWriter> m_writer;
			};
			class CMakeFileAsync : public CMakeFile {
				public:
					explicit CLOAK_CALL CMakeFileAsync(In const std::string& projName);
					CLOAK_CALL CMakeFileAsync(In Type type, In const CE::Files::UFI& path, In const std::string& projName, In bool shared, In const CE::Global::Task& finishTask);

					void CLOAK_CALL Create(In Type type, In const CE::Files::UFI& path, In bool shared, In const CE::Global::Task& finishTask);
					void CLOAK_CALL AddFile(In const std::string& name, In_opt bool publicFile = false) const override;
				private:
					CE::RefPointer<CE::Files::IMultithreadedWriteBuffer> m_mtw;
			};

			CE::RefPointer<CE::Files::IWriter> CLOAK_CALL CreateSourceFile(In const CE::Files::UFI& path, In const std::string& name, In const CloakEngine::Files::FileType& type);
			size_t CLOAK_CALL WriteCopyright(In const CE::RefPointer<CE::Files::IWriter>& file, In_opt size_t tabCount = 0);
			size_t CLOAK_CALL WriteHeaderIntro(In const CE::RefPointer<CE::Files::IWriter>& file, In Type type, In const std::string& name, In size_t hash, In_opt size_t tabCount = 0);
			size_t CLOAK_CALL WriteHeaderOutro(In const CE::RefPointer<CE::Files::IWriter>& file, In size_t tabCount);
			size_t CLOAK_CALL WriteCommonIntro(In const CE::RefPointer<CE::Files::IWriter>& file, In Type type, In const std::string& name, In_opt size_t tabCount = 0);
			size_t CLOAK_CALL WriteCommonOutro(In const CE::RefPointer<CE::Files::IWriter>& file, In size_t tabCount);
			void CLOAK_CALL WriteWithTabs(In const CE::RefPointer<CE::Files::IWriter>& header, In size_t tabs, In const std::string& text);
			void CLOAK_CALL Compile(In const CE::Files::UFI& path, In API::LibGenerator generator);
		}
	}
}

#endif