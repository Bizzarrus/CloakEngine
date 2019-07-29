#pragma once
#ifndef CE_IMPL_FILES_IMAGE_H
#define CE_IMPL_FILES_IMAGE_H

#include "CloakEngine/Files/Image.h"
#include "CloakEngine/Files/Animation.h"
#include "CloakEngine/Global/Math.h"
#include "Implementation/Files/FileHandler.h"
#include "Implementation/Rendering/ColorBuffer.h"
#include "Engine/BitMap.h"

#include <atomic>

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Files {
			inline namespace Image_v2 {
				class Image;
				namespace Proxys {
					class EntryProxy : public virtual API::Files::FileHandler_v1::IFileHandler, public virtual API::Files::FileHandler_v1::ILODFile, public Helper::SavePtr_v1::SavePtr, IMPL_EVENT(onLoad), IMPL_EVENT(onUnload) {
						public:
							CLOAK_CALL EntryProxy(In Image* parent, In size_t entry);
							virtual CLOAK_CALL ~EntryProxy();
							virtual ULONG CLOAK_CALL_THIS load(In_opt API::Files::Priority prio = API::Files::Priority::LOW) override;
							virtual ULONG CLOAK_CALL_THIS unload() override;
							virtual bool CLOAK_CALL_THIS isLoaded(In API::Files::Priority prio) override;
							virtual bool CLOAK_CALL_THIS isLoaded() const override;

							virtual void CLOAK_CALL_THIS RequestLOD(In API::Files::LOD lod) override;
						protected:
							Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
							Image* const m_parent;
							const size_t m_entry;
							std::atomic<API::Files::LOD> m_lod;
							API::Helper::ISyncSection* m_sync;
					};
					class CLOAK_UUID("{26E6A911-3AE5-4434-80AD-CD8C49E5711A}") TextureProxy : public virtual API::Files::Image_v2::ITexture, public EntryProxy {
						public:
							CLOAK_CALL TextureProxy(In Image* parent, In size_t entry, In_opt size_t buffer = 0);
							virtual CLOAK_CALL ~TextureProxy();
							virtual size_t CLOAK_CALL_THIS GetBufferCount() const override;
							virtual API::Rendering::IColorBuffer* CLOAK_CALL_THIS GetBuffer(In_opt size_t buffer = 0) const override;
							virtual bool CLOAK_CALL_THIS IsTransparentAt(In float X, In float Y, In_opt size_t buffer = 0) const override;
							virtual bool CLOAK_CALL_THIS IsBorderedImage(In_opt size_t buffer = 0) const override;
						protected:
							Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
							const size_t m_buffer;
					};
					class CLOAK_UUID("{E262E8A2-E735-4EA3-8CFF-8EC4A62031E6}") TextureArrayProxy : public virtual API::Files::Image_v2::ITextureArray, public EntryProxy {
						public:
							CLOAK_CALL TextureArrayProxy(In Image* parent, In size_t entry);
							virtual CLOAK_CALL ~TextureArrayProxy();
							virtual API::Rendering::IColorBuffer* CLOAK_CALL_THIS GetBuffer() const override;
							virtual size_t CLOAK_CALL_THIS GetSize() const override;
						protected:
							Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
					};
					class CLOAK_UUID("{75CC293D-2CB8-4964-8A95-56C49EE4F06F}") CubeMapProxy : public virtual API::Files::Image_v2::ICubeMap, public TextureArrayProxy {
						public:
							CLOAK_CALL CubeMapProxy(In Image* parent, In size_t entry);
							virtual CLOAK_CALL ~CubeMapProxy();
							virtual API::Rendering::IColorBuffer* CLOAK_CALL_THIS GetBuffer() const override;
							virtual size_t CLOAK_CALL_THIS GetSize() const override;
						protected:
							Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
					};
					class CLOAK_UUID("{8AF54F55-6B0F-4BC7-9A70-88DE37D0E1DF}") SkyboxProxy : public virtual API::Files::Image_v2::ISkybox, public CubeMapProxy {
						public:
							CLOAK_CALL SkyboxProxy(In Image* parent, In size_t entry);
							virtual CLOAK_CALL ~SkyboxProxy();
							virtual API::Rendering::IColorBuffer* CLOAK_CALL GetLUT() const override;
							virtual API::Rendering::IColorBuffer* CLOAK_CALL GetRadianceMap() const override;
							virtual API::Rendering::IConstantBuffer* CLOAK_CALL GetIrradiance() const override;
							virtual API::Rendering::IColorBuffer* CLOAK_CALL_THIS GetBuffer() const override;
						protected:
							Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
					};
					class CLOAK_UUID("{5D919AFE-196B-44AC-9F0E-7E38711345DC}") TextureAnimationProxy : public virtual API::Files::Image_v2::ITextureAnmiation, public TextureProxy {
						public:
							CLOAK_CALL TextureAnimationProxy(In Image* parent, In size_t entry);
							virtual CLOAK_CALL ~TextureAnimationProxy();
							virtual size_t CLOAK_CALL_THIS GetBufferCount() const override;
							virtual API::Rendering::IColorBuffer* CLOAK_CALL_THIS GetBuffer(In_opt size_t buffer = 0) const override;
							virtual bool CLOAK_CALL_THIS IsTransparentAt(In float X, In float Y, In_opt size_t buffer = 0) const override;
							virtual bool CLOAK_CALL_THIS IsBorderedImage(In_opt size_t buffer = 0) const override;
							virtual API::Global::Time CLOAK_CALL_THIS GetLength() const override;
							virtual void CLOAK_CALL_THIS Start() override;
							virtual void CLOAK_CALL_THIS Pause() override;
							virtual void CLOAK_CALL_THIS Resume() override;
							virtual void CLOAK_CALL_THIS Stop() override;
							virtual void CLOAK_CALL_THIS SetAutoRepeat(In bool repeat) override;
							virtual bool CLOAK_CALL_THIS GetAutoRepeat() const override;
							virtual bool CLOAK_CALL_THIS IsPaused() override;
							virtual bool CLOAK_CALL_THIS IsRunning() override;
						protected:
							Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;

							enum class AnimState { STOPED, PAUSED, RUNNING };

							AnimState m_state;
							API::Global::Time m_started = 0;
							API::Global::Time m_stoped = 0;
							API::Global::Time m_offset = 0;
							bool m_autoRepeat = false;
					};
					class CLOAK_UUID("{4FC7FC9E-91FD-44A1-B715-C1404AE933D5}") MaterialProxy : public virtual API::Files::Image_v2::IMaterial, public EntryProxy {
						public:
							CLOAK_CALL MaterialProxy(In Image* parent, In size_t entry);
							virtual CLOAK_CALL ~MaterialProxy();
							virtual API::Files::Image_v2::MaterialType CLOAK_CALL_THIS GetType() const override;
							virtual bool CLOAK_CALL_THIS SupportTesslation() const override;
							virtual API::Rendering::IColorBuffer* CLOAK_CALL_THIS GetAlbedo() const override;
							virtual API::Rendering::IColorBuffer* CLOAK_CALL_THIS GetNormalMap() const override;
							virtual API::Rendering::IColorBuffer* CLOAK_CALL_THIS GetSpecularMap() const override;
							virtual API::Rendering::IColorBuffer* CLOAK_CALL_THIS GetDisplacementMap() const override;
							virtual API::Rendering::ResourceView CLOAK_CALL_THIS GetSRVMaterial() const override;
							virtual API::Rendering::ResourceView CLOAK_CALL_THIS GetSRVDisplacement() const override;
						protected:
							Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
					};
				}
				namespace FileData {
					enum class EntryType {Texture,Array,Material,Cube,Skybox};
					struct BitData {
						uint8_t Len[4];
						uint8_t Bits[4];
						uint64_t MaxValue[4];
					};
					struct BitList {
						BitData RGBA;
						BitData RGBE;
						BitData YCCA;
						BitData Material;
					};
					enum class ColorSpace { RGBA, YCCA, Material };
					struct Meta {
						ColorSpace ColorSpace;
						uint8_t Flags;
						bool HR;
					};
					class BitBuffer;
					class BitBufferPos {
						friend class BitBuffer;
						public:
							CLOAK_CALL BitBufferPos(In const BitBufferPos& p);
							BitBufferPos& CLOAK_CALL_THIS operator=(In const BitBufferPos& p);
							float CLOAK_CALL_THIS Get(In size_t c) const;
							void CLOAK_CALL_THIS Set(In float v, In size_t c);
						protected:
							CLOAK_CALL BitBufferPos(In void* ptr, In uint8_t channels, In uint8_t mbw, In bool hr);

							uintptr_t m_ptr;
							uint8_t m_mbw;
							bool m_hr;
							uint8_t m_off[4];
					};
					class BitBuffer {
						public:
							CLOAK_CALL BitBuffer();
							CLOAK_CALL BitBuffer(In const BitBuffer& o);
							CLOAK_CALL ~BitBuffer();
							BitBuffer& CLOAK_CALL_THIS operator=(In const BitBuffer& o);
							void CLOAK_CALL_THIS Reset(In UINT W, In UINT H, In uint8_t channels, In const BitData& bits, In bool hr);
							void CLOAK_CALL_THIS Reset();
							void* CLOAK_CALL_THIS GetBinaryData() const;
							LONG_PTR CLOAK_CALL_THIS GetRowPitch() const;
							LONG_PTR CLOAK_CALL_THIS GetSlicePitch() const;
							BitBufferPos CLOAK_CALL_THIS Get(In UINT x, In UINT y);
							const BitBufferPos CLOAK_CALL_THIS Get(In UINT x, In UINT y) const;
							UINT CLOAK_CALL_THIS GetWidth() const;
							UINT CLOAK_CALL_THIS GetHeight() const;
						protected:
							uint8_t* m_data;
							UINT m_W;
							UINT m_H;
							uint8_t m_channels;
							uint8_t m_mbw;
							size_t m_boff;
							bool m_hr;
					};
					struct DataBasic {
						size_t MaxLayerCount = 0;
						size_t LoadedLayers = 0;
					};
					struct BufferData : public DataBasic {
						BitBuffer LastLayer;
					};
					struct ArrayData : public DataBasic {
						size_t ArraySize = 0;
						BitBuffer* LastLayers = nullptr;
					};
					template<typename T> struct ColorBufferData : public T {
						Impl::Rendering::IColorBuffer* Buffer = nullptr;
					};
					class AlphaMap {
						public:
							CLOAK_CALL AlphaMap();
							CLOAK_CALL ~AlphaMap();
							void CLOAK_CALL_THIS Resize(In UINT W, In UINT H, In API::Files::IReader* read, In API::Files::FileVersion version);
							bool CLOAK_CALL_THIS IsTransparentAt(In float X, In float Y) const;
						private:
							API::Helper::ISyncSection* m_sync;
							Engine::BitMap* m_map;
							size_t m_S;
							UINT m_W;
							UINT m_H;
					};
					class Entry {
						public:
							CLOAK_CALL Entry();
							virtual CLOAK_CALL ~Entry();
							virtual EntryType CLOAK_CALL_THIS GetType() const = 0;
							void* CLOAK_CALL operator new(In size_t size);
							void CLOAK_CALL operator delete(In void* ptr);
							virtual void CLOAK_CALL_THIS readHeader(In API::Files::IReader* read, In API::Files::FileVersion version, In const BitList& bits, In API::Global::Graphic::TextureQuality quality, Out API::Global::Graphic::TextureQuality* maxQuality) = 0;
							//return true, if there are still more layers to load
							virtual bool CLOAK_CALL_THIS readLayer(In API::Files::IReader* read, In API::Files::FileVersion version, In const BitList& bits, In API::Files::LOD reqLOD) = 0;
							virtual bool CLOAK_CALL_THIS RequireLoad(In API::Files::LOD reqLOD) = 0;
					};
					class Texture : public Entry {
						public:
							CLOAK_CALL Texture();
							virtual CLOAK_CALL ~Texture();
							virtual EntryType CLOAK_CALL_THIS GetType() const override { return EntryType::Texture; }
							virtual size_t CLOAK_CALL_THIS GetBufferCount() const;
							virtual API::Rendering::IColorBuffer* CLOAK_CALL_THIS GetBuffer(In_opt size_t buffer = 0) const;
							virtual bool CLOAK_CALL_THIS IsTransparentAt(In float X, In float Y, In_opt size_t buffer = 0) const;
							virtual bool CLOAK_CALL_THIS IsBorderedImage(In_opt size_t buffer = 0) const;
							virtual size_t CLOAK_CALL_THIS GetBufferByTime(In API::Global::Time time) const;
							virtual API::Global::Time CLOAK_CALL_THIS GetAnimationLength() const;
							virtual void CLOAK_CALL_THIS readHeader(In API::Files::IReader* read, In API::Files::FileVersion version, In const BitList& bits, In API::Global::Graphic::TextureQuality quality, Out API::Global::Graphic::TextureQuality* maxQuality) override;
							virtual bool CLOAK_CALL_THIS readLayer(In API::Files::IReader* read, In API::Files::FileVersion version, In const BitList& bits, In API::Files::LOD reqLOD) override;
							virtual bool CLOAK_CALL_THIS RequireLoad(In API::Files::LOD reqLOD) override;
						protected:
							ColorBufferData<BufferData>* m_buffer;
							AlphaMap* m_alpha;
							Meta* m_meta;
							size_t m_bufferSize;
							uint8_t m_channels;
					};
					class TextureArray : public Entry {
						public:
							CLOAK_CALL TextureArray();
							virtual CLOAK_CALL ~TextureArray();
							virtual EntryType CLOAK_CALL_THIS GetType() const override { return EntryType::Array; }
							virtual API::Rendering::IColorBuffer* CLOAK_CALL_THIS GetBuffer() const;
							virtual size_t CLOAK_CALL_THIS GetSize() const;
							virtual void CLOAK_CALL_THIS readHeader(In API::Files::IReader* read, In API::Files::FileVersion version, In const BitList& bits, In API::Global::Graphic::TextureQuality quality, Out API::Global::Graphic::TextureQuality* maxQuality) override;
							virtual bool CLOAK_CALL_THIS readLayer(In API::Files::IReader* read, In API::Files::FileVersion version, In const BitList& bits, In API::Files::LOD reqLOD) override;
							virtual bool CLOAK_CALL_THIS RequireLoad(In API::Files::LOD reqLOD) override;
						protected:
							ColorBufferData<ArrayData> m_buffer;
							Meta* m_meta;
							uint8_t m_channels;
					};
					class CubeMap : public Entry {
						public:
							CLOAK_CALL CubeMap();
							virtual CLOAK_CALL ~CubeMap();
							virtual EntryType CLOAK_CALL_THIS GetType() const override { return EntryType::Cube; }
							virtual API::Rendering::IColorBuffer* CLOAK_CALL_THIS GetBuffer() const;
							virtual void CLOAK_CALL_THIS readHeader(In API::Files::IReader* read, In API::Files::FileVersion version, In const BitList& bits, In API::Global::Graphic::TextureQuality quality, Out API::Global::Graphic::TextureQuality* maxQuality) override;
							virtual bool CLOAK_CALL_THIS readLayer(In API::Files::IReader* read, In API::Files::FileVersion version, In const BitList& bits, In API::Files::LOD reqLOD) override;
							virtual bool CLOAK_CALL_THIS RequireLoad(In API::Files::LOD reqLOD) override;
						protected:
							ColorBufferData<ArrayData> m_buffer;
							Meta m_meta[6];
							uint8_t m_channels;
					};
					class Skybox : public CubeMap {
						public:
							CLOAK_CALL Skybox();
							virtual CLOAK_CALL ~Skybox();
							virtual EntryType CLOAK_CALL_THIS GetType() const override { return EntryType::Skybox; }
							virtual API::Rendering::IColorBuffer* CLOAK_CALL_THIS GetBuffer() const override;
							virtual API::Rendering::IColorBuffer* CLOAK_CALL_THIS GetRadiance() const;
							virtual API::Rendering::IColorBuffer* CLOAK_CALL_THIS GetLUT() const;
							virtual API::Rendering::IConstantBuffer* CLOAK_CALL_THIS GetIrradiance() const;
							virtual void CLOAK_CALL_THIS readHeader(In API::Files::IReader* read, In API::Files::FileVersion version, In const BitList& bits, In API::Global::Graphic::TextureQuality quality, Out API::Global::Graphic::TextureQuality* maxQuality) override;
							virtual bool CLOAK_CALL_THIS readLayer(In API::Files::IReader* read, In API::Files::FileVersion version, In const BitList& bits, In API::Files::LOD reqLOD) override;
							virtual bool CLOAK_CALL_THIS RequireLoad(In API::Files::LOD reqLOD) override;
						protected:
							ColorBufferData<ArrayData> m_radiance;
							BufferData m_lut;
							API::Rendering::IConstantBuffer* m_irradiance;
					};
					class TextureAnimation : public Texture {
						public:
							CLOAK_CALL TextureAnimation();
							virtual CLOAK_CALL ~TextureAnimation();
							virtual size_t CLOAK_CALL_THIS GetBufferByTime(In API::Global::Time time) const override;
							virtual API::Global::Time CLOAK_CALL_THIS GetAnimationLength() const override;
							virtual void CLOAK_CALL_THIS readHeader(In API::Files::IReader* read, In API::Files::FileVersion version, In const BitList& bits, In API::Global::Graphic::TextureQuality quality, Out API::Global::Graphic::TextureQuality* maxQuality) override;
							virtual bool CLOAK_CALL_THIS readLayer(In API::Files::IReader* read, In API::Files::FileVersion version, In const BitList& bits, In API::Files::LOD reqLOD) override;
							virtual bool CLOAK_CALL_THIS RequireLoad(In API::Files::LOD reqLOD) override;
						protected:
							typedef API::Global::Math::SectionFunctions::Point<API::Global::Time, size_t> KeyFrame;
							API::Global::Math::SectionFunctions::LinearFunction<API::Global::Time, size_t> m_frames;
					};
					class Material : public Entry {
						public:
							CLOAK_CALL Material();
							virtual CLOAK_CALL ~Material();
							virtual EntryType CLOAK_CALL_THIS GetType() const override { return EntryType::Material; }
							virtual API::Rendering::IColorBuffer* CLOAK_CALL_THIS GetBuffer(In size_t num) const;
							virtual API::Files::Image_v2::MaterialType CLOAK_CALL_THIS GetMaterialType() const;
							virtual bool CLOAK_CALL_THIS SupportTesslation() const;
							virtual void CLOAK_CALL_THIS readHeader(In API::Files::IReader* read, In API::Files::FileVersion version, In const BitList& bits, In API::Global::Graphic::TextureQuality quality, Out API::Global::Graphic::TextureQuality* maxQuality) override;
							virtual bool CLOAK_CALL_THIS readLayer(In API::Files::IReader* read, In API::Files::FileVersion version, In const BitList& bits, In API::Files::LOD reqLOD) override;
							virtual bool CLOAK_CALL_THIS RequireLoad(In API::Files::LOD reqLOD) override;
						protected:
							API::Files::Image_v2::MaterialType m_type;
							Meta m_meta[4];
							bool m_constant[4];
							ColorBufferData<BufferData> m_buffer[4];
							bool m_useDisplacement;
					};
				}


				class CLOAK_UUID("{485B1363-8498-4868-889E-6E993CAAD746}") Image : public virtual API::Files::Image_v2::IImage, public virtual FileHandler_v1::RessourceHandler {
					friend class Proxys::EntryProxy;
					friend class Proxys::TextureProxy;
					friend class Proxys::TextureArrayProxy;
					friend class Proxys::CubeMapProxy;
					friend class Proxys::TextureAnimationProxy;
					friend class Proxys::MaterialProxy;
					friend class Proxys::SkyboxProxy;
					public:
						static void CLOAK_CALL Initialize();
						static void CLOAK_CALL Shutdown();

						CLOAK_CALL Image();
						virtual CLOAK_CALL ~Image();
						virtual API::RefPointer<API::Files::Image_v2::ITexture> CLOAK_CALL_THIS GetTexture(In size_t id, In_opt size_t buffer = 0) override;
						virtual API::RefPointer<API::Files::Image_v2::ITextureAnmiation> CLOAK_CALL_THIS GetAnimation(In size_t id) override;
						virtual API::RefPointer<API::Files::Image_v2::ITextureArray> CLOAK_CALL_THIS GetTextureArray(In size_t id) override;
						virtual API::RefPointer<API::Files::Image_v2::ICubeMap> CLOAK_CALL_THIS GetCubeMap(In size_t id) override;
						virtual API::RefPointer<API::Files::Image_v2::IMaterial> CLOAK_CALL_THIS GetMaterial(In size_t id)  override;
						virtual API::RefPointer<API::Files::Image_v2::ISkybox> CLOAK_CALL_THIS GetSkybox(In size_t id) override;
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
						virtual bool CLOAK_CALL_THIS iCheckSetting(In const API::Global::Graphic::Settings& nset) const override;
						virtual LoadResult CLOAK_CALL_THIS iLoad(In API::Files::IReader* read, In API::Files::FileVersion version) override;
						virtual LoadResult CLOAK_CALL_THIS iLoadDelayed(In API::Files::IReader* read, In API::Files::FileVersion version) override;
						virtual void CLOAK_CALL_THIS iUnload() override;

						LoadResult CLOAK_CALL_THIS LoadLayer(In API::Files::IReader* read, In API::Files::FileVersion version);
						FileData::Texture* CLOAK_CALL_THIS GetTextureEntry(In size_t entry);
						FileData::TextureArray* CLOAK_CALL_THIS GetArrayEntry(In size_t entry);
						FileData::CubeMap* CLOAK_CALL_THIS GetCubeMapEntry(In size_t entry);
						FileData::Material* CLOAK_CALL_THIS GetMaterialEntry(In size_t entry);
						FileData::Skybox* CLOAK_CALL_THIS GetSkyboxEntry(In size_t entry);
						void CLOAK_CALL_THIS RegisterLOD(In API::Files::LOD lod);
						void CLOAK_CALL_THIS UnregisterLOD(In API::Files::LOD lod);

						API::Global::Graphic::TextureQuality m_curQuality;
						API::Global::Graphic::TextureQuality m_maxQuality;
						FileData::BitList m_bits;
						FileData::Entry** m_entries;
						FileData::Texture** m_textures;
						FileData::TextureArray** m_arrays;
						FileData::CubeMap** m_cubes;
						FileData::Material** m_materials;
						FileData::Skybox** m_skybox;
						int32_t m_lodReg[3];
						size_t m_entSize;
						size_t m_texSize;
						size_t m_arrSize;
						size_t m_cubSize;
						size_t m_matSize;
						size_t m_skySize;
						API::Files::LOD m_requestedLOD;
						API::Files::LOD m_curLOD;
						bool m_noBackgroundLoading;
				};
				DECLARE_FILE_HANDLER(Image, "{D58DEDFC-C827-4945-ACFB-9C0762664C34}");
			}
		}
	}
}

#pragma warning(pop)

#endif