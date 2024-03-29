#pragma once
#ifndef CE_API_HELPER_UIDS_H
#define CE_API_HELPER_UIDS_H

#include "CloakEngine/Defines.h"

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Files {
			// {CA9FAF4C-31FE-4B08-949E-E07FAF8AF88B}
			DEFINE_GUID(IID_ICompressor1, 0xca9faf4c, 0x31fe, 0x4b08, 0x94, 0x9e, 0xe0, 0x7f, 0xaf, 0x8a, 0xf8, 0x8b);
			// {B6B14472-75DE-4C65-8E5E-E60356E9AA8B}
			DEFINE_GUID(IID_IWriter1, 0xb6b14472, 0x75de, 0x4c65, 0x8e, 0x5e, 0xe6, 0x3, 0x56, 0xe9, 0xaa, 0x8b);
			// {4998C310-7066-464A-B2F0-DE2389E7459A}
			DEFINE_GUID(IID_IReader1, 0x4998c310, 0x7066, 0x464a, 0xb2, 0xf0, 0xde, 0x23, 0x89, 0xe7, 0x45, 0x9a);
			// {27D778B8-EFE4-44FF-B49D-18044E8815ED}
			DEFINE_GUID(IID_IConfiguration1, 0x27d778b8, 0xefe4, 0x44ff, 0xb4, 0x9d, 0x18, 0x4, 0x4e, 0x88, 0x15, 0xed);
			// {F1E296A3-9E8E-494B-874C-0F450E91D09A}
			DEFINE_GUID(IID_IFileHandler1, 0xf1e296a3, 0x9e8e, 0x494b, 0x87, 0x4c, 0xf, 0x45, 0xe, 0x91, 0xd0, 0x9a);
			// {BC5A6595-7A98-486B-88B0-B7EAE0522424}
			DEFINE_GUID(IID_IFont1, 0xbc5a6595, 0x7a98, 0x486b, 0x88, 0xb0, 0xb7, 0xea, 0xe0, 0x52, 0x24, 0x24);
			// {F2DCE5E1-A1F5-450A-B05B-5B1449A3D66A}
			DEFINE_GUID(IID_IImage1, 0xf2dce5e1, 0xa1f5, 0x450a, 0xb0, 0x5b, 0x5b, 0x14, 0x49, 0xa3, 0xd6, 0x6a);
			// {E9D3A546-FD32-4356-945F-B9938DA40CEC}
			DEFINE_GUID(IID_ILanguage1, 0xe9d3a546, 0xfd32, 0x4356, 0x94, 0x5f, 0xb9, 0x93, 0x8d, 0xa4, 0xc, 0xec);
			// {F222F0A5-CB16-4C05-9D94-DE031496AD14}
			DEFINE_GUID(IID_IShader1, 0xf222f0a5, 0xcb16, 0x4c05, 0x9d, 0x94, 0xde, 0x3, 0x14, 0x96, 0xad, 0x14);

		}
		namespace Global {
			// {F95AAE5B-CA8C-49A8-9056-2511DD6A9444}
			DEFINE_GUID(IID_IGame1, 0xf95aae5b, 0xca8c, 0x49a8, 0x90, 0x56, 0x25, 0x11, 0xdd, 0x6a, 0x94, 0x44);
		}
		namespace Helper {
			// {EEC9FA96-0416-49B0-8EA9-C09169559A2E}
			DEFINE_GUID(IID_ISavePtr1, 0xeec9fa96, 0x416, 0x49b0, 0x8e, 0xa9, 0xc0, 0x91, 0x69, 0x55, 0x9a, 0x2e);
			// {6DE8C4C5-06C9-4F93-866A-7E5D1AF4B083}
			DEFINE_GUID(IID_ISyncSection1, 0x6de8c4c5, 0x6c9, 0x4f93, 0x86, 0x6a, 0x7e, 0x5d, 0x1a, 0xf4, 0xb0, 0x83);
		}
		namespace Interface {
			// {2E3DD009-6472-4AAB-9F45-F5B1DE10D309}
			DEFINE_GUID(IID_IBasicGUI1, 0x2e3dd009, 0x6472, 0x4aab, 0x9f, 0x45, 0xf5, 0xb1, 0xde, 0x10, 0xd3, 0x9);
			// {31E97730-FFDA-410E-95F2-1F1BE353FA46}
			DEFINE_GUID(IID_IButton1, 0x31e97730, 0xffda, 0x410e, 0x95, 0xf2, 0x1f, 0x1b, 0xe3, 0x53, 0xfa, 0x46);
			// {9C24DCE7-1CEE-4578-A6A2-0A6A0CBBC2DD}
			DEFINE_GUID(IID_ICheckButton1, 0x9c24dce7, 0x1cee, 0x4578, 0xa6, 0xa2, 0xa, 0x6a, 0xc, 0xbb, 0xc2, 0xdd);
			// {6632D679-0237-4379-809F-1FD8D08AF905}
			DEFINE_GUID(IID_IDropDownMenu1, 0x6632d679, 0x237, 0x4379, 0x80, 0x9f, 0x1f, 0xd8, 0xd0, 0x8a, 0xf9, 0x5);
			// {3BED4520-581A-499E-9EAA-43BF40C62BA7}
			DEFINE_GUID(IID_IDropDownButton1, 0x3bed4520, 0x581a, 0x499e, 0x9e, 0xaa, 0x43, 0xbf, 0x40, 0xc6, 0x2b, 0xa7);
			// {9EA4A6FE-BA4B-4C50-B5AC-81FC6F2A6027}
			DEFINE_GUID(IID_IEditBox1, 0x9ea4a6fe, 0xba4b, 0x4c50, 0xb5, 0xac, 0x81, 0xfc, 0x6f, 0x2a, 0x60, 0x27);
			// {0AA73F69-4714-4C44-B5F3-265A9FFDE347}
			DEFINE_GUID(IID_IFrame1, 0xaa73f69, 0x4714, 0x4c44, 0xb5, 0xf3, 0x26, 0x5a, 0x9f, 0xfd, 0xe3, 0x47);
			// {18BFEBF9-C58E-4902-86F1-10C6DAD4BBFB}
			DEFINE_GUID(IID_ILabel1, 0x18bfebf9, 0xc58e, 0x4902, 0x86, 0xf1, 0x10, 0xc6, 0xda, 0xd4, 0xbb, 0xfb);
			// {ECA3C42A-05B0-42FE-9496-6F54F3F5F422}
			DEFINE_GUID(IID_ISlider1, 0xeca3c42a, 0x5b0, 0x42fe, 0x94, 0x96, 0x6f, 0x54, 0xf3, 0xf5, 0xf4, 0x22);
			// {53F3F215-359B-4871-B291-719A60EC5CC8}
			DEFINE_GUID(IID_IVSlider1, 0x53f3f215, 0x359b, 0x4871, 0xb2, 0x91, 0x71, 0x9a, 0x60, 0xec, 0x5c, 0xc8);
			// {D62B5E39-0DFB-457D-9EA8-85DCE79241FD}
			DEFINE_GUID(IID_IHSlider1, 0xd62b5e39, 0xdfb, 0x457d, 0x9e, 0xa8, 0x85, 0xdc, 0xe7, 0x92, 0x41, 0xfd);
		}
	}
}


#endif
