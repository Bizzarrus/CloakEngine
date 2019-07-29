#pragma once
#ifndef CE_IMPL_FILES_CONFIGURATION_H
#define CE_IMPL_FILES_CONFIGURATION_H

#include "CloakEngine/Files/Configuration.h"
#include "CloakEngine/Helper/SyncSection.h"
#include "Implementation/Helper/SavePtr.h"

#include <unordered_map>
#include <atomic>

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Files {
			inline namespace Configuration_v1 {
				class Configuration;
				class Value : public API::Files::Configuration_v1::IValue {
					public:
						CLOAK_CALL Value(In Configuration* parent, In_opt Value* root, In_opt std::string name = "");
						virtual CLOAK_CALL ~Value();

						virtual std::string CLOAK_CALL_THIS toString(In_opt std::string def = "") override;
						virtual bool CLOAK_CALL_THIS toBool(In_opt bool def = false) override;
						virtual int CLOAK_CALL_THIS toInt(In_opt int def = 0) override;
						virtual float CLOAK_CALL_THIS toFloat(In_opt float def = 0) override;

						virtual IValue* CLOAK_CALL_THIS get(In const std::string& name) override;
						virtual IValue* CLOAK_CALL_THIS get(In size_t index) override;
						virtual IValue* CLOAK_CALL_THIS getRoot() override;
						virtual std::string CLOAK_CALL_THIS getName() const override;

						Success(return == true) virtual bool CLOAK_CALL_THIS enumerateChildren(In size_t i, Out IValue** res) override;

						virtual bool CLOAK_CALL_THIS has(In const std::string& name) const override;
						virtual bool CLOAK_CALL_THIS has(In unsigned int index) const override;

						virtual bool CLOAK_CALL_THIS is(In const std::string& s) const override;
						virtual bool CLOAK_CALL_THIS is(In bool s) const override;
						virtual bool CLOAK_CALL_THIS is(In int s) const override;
						virtual bool CLOAK_CALL_THIS is(In float s) const override;
						virtual bool CLOAK_CALL_THIS is(In std::nullptr_t s) const override;
						virtual bool CLOAK_CALL_THIS isObjekt() const override;
						virtual bool CLOAK_CALL_THIS isArray() const override;
						virtual bool CLOAK_CALL_THIS isString() const override;
						virtual bool CLOAK_CALL_THIS isBool() const override;
						virtual bool CLOAK_CALL_THIS isInt() const override;
						virtual bool CLOAK_CALL_THIS isFloat() const override;

						virtual size_t CLOAK_CALL_THIS size() const override;

						virtual void CLOAK_CALL_THIS set(In const std::string& s) override;
						virtual void CLOAK_CALL_THIS set(In bool s) override;
						virtual void CLOAK_CALL_THIS set(In int s) override;
						virtual void CLOAK_CALL_THIS set(In float s) override;
						virtual void CLOAK_CALL_THIS set(In std::nullptr_t s) override;
						virtual void CLOAK_CALL_THIS setToArray() override;
						virtual void CLOAK_CALL_THIS setToObject() override;

						virtual void CLOAK_CALL_THIS forceUsed() override;

						Value* CLOAK_CALL_THIS getChild(In size_t index);
						size_t CLOAK_CALL_THIS getChildCount() const;
						void CLOAK_CALL_THIS removeUnused();
						void CLOAK_CALL_THIS resetUnused();
						void CLOAK_CALL_THIS clear();
						Value& CLOAK_CALL_THIS operator=(In const Value& v);
					protected:
						enum class Type { STRING, BOOL, INT, FLOAT, OBJ, ARRAY, UNDEF };

						void CLOAK_CALL_THIS changeType(In Type type);

						Configuration* m_parent;
						Type m_type;
						bool m_used;
						bool m_forceUsed;
						std::string m_name;
						std::string m_valStr;
						union {
							bool m_valBool;
							int m_valInt;
							float m_valFloat;
						};
						Value* m_root;
						API::List<Value*> m_childs;
				};

				class CLOAK_UUID("{50B907D4-EEF2-49B6-AEBA-34732BC7D37E}") Configuration : public API::Files::Configuration_v1::IConfiguration, public Impl::Helper::SavePtr_v1::SavePtr {
					friend class Value;
					public:
						CLOAK_CALL Configuration();
						virtual CLOAK_CALL ~Configuration();
						virtual void CLOAK_CALL_THIS readFile(In const API::Files::UFI& filePath, In API::Files::Configuration_v1::ConfigType type) override;
						virtual void CLOAK_CALL_THIS saveFile(In const API::Files::UFI& filePath, In API::Files::Configuration_v1::ConfigType type) override;
						virtual void CLOAK_CALL_THIS ReadBuffer(In API::Files::IReadBuffer* buffer, In API::Files::Configuration_v1::ConfigType type) override;
						virtual void CLOAK_CALL_THIS saveBuffer(In API::Files::IWriteBuffer* buffer, In API::Files::Configuration_v1::ConfigType type) override;
						virtual void CLOAK_CALL_THIS saveFile() override;
						virtual void CLOAK_CALL_THIS removeUnused() override;
						virtual API::Files::Configuration_v1::IValue* CLOAK_CALL_THIS getRootValue() override;
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;

						API::Helper::ISyncSection* m_sync = nullptr;
						Value m_root;
						bool m_changed;
						API::Files::UFI m_lastPath;
						API::Files::Configuration_v1::ConfigType m_lastType;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif