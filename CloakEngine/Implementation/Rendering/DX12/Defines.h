#pragma once
#ifndef CE_IMPL_RENDERING_DX12_DEFINES_H
#define CE_IMPL_RENDERING_DX12_DEFINES_H

#include "Implementation/Rendering/Defines.h"

#define RENDERING_QUERY_IMPL(ptr, riid, name, ver, super) if(riid==__uuidof(Impl::Rendering::ver::I##name)){*ptr=(Impl::Rendering::ver::I##name*)this; return true;} else if(riid==__uuidof(Impl::Rendering::DX12::ver::name*)){*ptr=(Impl::Rendering::DX12::ver::name*)this; return true;} return super::iQueryInterface(riid,ptr)
#define RENDERING_QUERY(ptr, riid, name, ver, super) if(riid==__uuidof(API::Rendering::ver::I##name)){*ptr=(API::Rendering::ver::I##name*)this; return true;} else RENDERING_QUERY_IMPL(ptr,riid,name,ver,super)

#ifndef _DEBUG
#define D3D12_IGNORE_SDK_LAYERS
#endif

#endif