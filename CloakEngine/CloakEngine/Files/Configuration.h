#pragma once
#ifndef CE_API_FILES_CONFIGURATION_H
#define CE_API_FILES_CONFIGURATION_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Helper/SavePtr.h"
#include "CloakEngine/Files/Buffer.h"

#include <string>

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Files {
			inline namespace Configuration_v1 {

				CLOAK_INTERFACE IValue {
					public:
						virtual std::string CLOAK_CALL_THIS toString(In_opt std::string def = "") = 0;
						virtual bool CLOAK_CALL_THIS toBool(In_opt bool def = false) = 0;
						virtual int CLOAK_CALL_THIS toInt(In_opt int def = 0) = 0;
						virtual float CLOAK_CALL_THIS toFloat(In_opt float def = 0) = 0;

						virtual IValue* CLOAK_CALL_THIS get(In const std::string& name) = 0;
						virtual IValue* CLOAK_CALL_THIS get(In size_t index) = 0;
						virtual IValue* CLOAK_CALL_THIS getRoot() = 0;
						virtual std::string CLOAK_CALL_THIS getName() const = 0;

						Success(return == true) virtual bool CLOAK_CALL_THIS enumerateChildren(In size_t i, Out IValue** res) = 0;

						virtual bool CLOAK_CALL_THIS has(In const std::string& name) const = 0;
						virtual bool CLOAK_CALL_THIS has(In unsigned int index) const = 0;

						virtual bool CLOAK_CALL_THIS is(In const std::string& s) const = 0;
						virtual bool CLOAK_CALL_THIS is(In bool s) const = 0;
						virtual bool CLOAK_CALL_THIS is(In int s) const = 0;
						virtual bool CLOAK_CALL_THIS is(In float s) const = 0;
						virtual bool CLOAK_CALL_THIS is(In std::nullptr_t s) const = 0;
						virtual bool CLOAK_CALL_THIS isObjekt() const = 0;
						virtual bool CLOAK_CALL_THIS isArray() const = 0;
						virtual bool CLOAK_CALL_THIS isString() const = 0;
						virtual bool CLOAK_CALL_THIS isBool() const = 0;
						virtual bool CLOAK_CALL_THIS isInt() const = 0;
						virtual bool CLOAK_CALL_THIS isFloat() const = 0;

						virtual size_t CLOAK_CALL_THIS size() const = 0;

						virtual void CLOAK_CALL_THIS set(In const std::string& s) = 0;
						virtual void CLOAK_CALL_THIS set(In bool s) = 0;
						virtual void CLOAK_CALL_THIS set(In int s) = 0;
						virtual void CLOAK_CALL_THIS set(In float s) = 0;
						virtual void CLOAK_CALL_THIS set(In std::nullptr_t s) = 0;
						virtual void CLOAK_CALL_THIS setToArray() = 0;
						virtual void CLOAK_CALL_THIS setToObject() = 0;

						virtual void CLOAK_CALL_THIS forceUsed() = 0;
				};

				enum class ConfigType { INI, JSON, XML };

				CLOAK_INTERFACE_ID("{27D778B8-EFE4-44FF-B49D-18044E8815ED}") IConfiguration : public virtual API::Helper::SavePtr_v1::ISavePtr{
					public:
						virtual void CLOAK_CALL_THIS readFile(In const UFI& filePath, In ConfigType type) = 0;
						virtual void CLOAK_CALL_THIS saveFile(In const UFI& filePath, In ConfigType type) = 0;
						virtual void CLOAK_CALL_THIS ReadBuffer(In API::Files::IReadBuffer* buffer, In ConfigType type) = 0;
						virtual void CLOAK_CALL_THIS saveBuffer(In API::Files::IWriteBuffer* buffer, In ConfigType type) = 0;
						virtual void CLOAK_CALL_THIS saveFile() = 0;
						virtual void CLOAK_CALL_THIS removeUnused() = 0;
						virtual IValue* CLOAK_CALL_THIS getRootValue() = 0;
				};
			}
		}
	}
}

#endif