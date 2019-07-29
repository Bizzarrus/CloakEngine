#include "stdafx.h"
#include "DebugCommands.h"
#include "CompilerBase.h"
#include "CloakCompiler/Mesh.h"

#include <ctime>
#include <sstream>

namespace DebugCommands {
	CloakEngine::Files::Image_v2::IImage* g_debugImg = nullptr;
	CloakEngine::Files::Mesh_v1::IMesh* g_debugMesh = nullptr;
	CloakEngine::Files::IFont* g_debugFont = nullptr;

	void CLOAK_CALL InitCommands()
	{
		CloakEngine::Global::Debug::RegisterConsoleArgument("debug load img", "Loads the file test.ceii", [=](In const CloakEngine::Global::Debug::CommandArgument* args, In size_t argCount) {
			CloakDebugLog("Try to begin loading...");
			CloakEngine::Files::Image_v2::IImage* img = nullptr;
			std::u16string path = CloakEngine::Helper::StringConvert::ConvertToU16(COMP_PATH("Output/Image/Test.ceii"));
			CloakEngine::Files::OpenFile(path, CE_QUERY_ARGS(&img));
			//img->RequestLOD(CloakEngine::Files::LOD::ULTRA);
			img->load();
			SAVE_RELEASE(g_debugImg);
			g_debugImg = img;
			return true;
		});
		CloakEngine::Global::Debug::RegisterConsoleArgument("debug load mesh", "Loads the file test.cemf", [=](In const CloakEngine::Global::Debug::CommandArgument* args, In size_t argCount) {
			CloakDebugLog("Try to begin loading...");
			CloakEngine::Files::Mesh_v1::IMesh* mesh = nullptr;
			std::u16string path = CloakEngine::Helper::StringConvert::ConvertToU16(COMP_PATH("Output/Mesh/Test.cemf"));
			CloakEngine::Files::OpenFile(path, CE_QUERY_ARGS(&mesh));
			mesh->load();
			SAVE_RELEASE(g_debugMesh);
			g_debugMesh = mesh;
			return true;
		});
		CloakEngine::Global::Debug::RegisterConsoleArgument("debug file [%o(-normal|-huffman|-lzc|-lzw|-lzh|-lz4|-zlib)]", "debugs file reading", [=](In const CloakEngine::Global::Debug::CommandArgument* args, In size_t argCount) {
			CloakEngine::Files::FileType type{ "TestFile","CETF",1000 };
			/*
			For 20.000 values of Data with 1-16 bits size:

			Write times:	PC (Debug)	|	PC (Release)	|	Laptop
			- Normal:			 29 ms	|					|	 68 ms
			- Huffman:			 35 ms	|					|	 50 ms
			- LZC:				306 ms	|					|	725 ms
			- LZW:				287 ms	|					|	671 ms
			- LZH:				284 ms	|					|	774 ms
			- LZ4:				 27 ms	|					|
			- zLib:				 32 ms	|					|

			Read times:					|					|
			- Normal:			 24 ms	|					|	 26 ms
			- Huffman:			740 ms	|					|	 63 ms
			- LZC:				 38 ms	|					|	 46 ms
			- LZW:				933 ms	|					|	236 ms
			- LZH:				928 ms	|					|	123 ms
			- LZ4:				 25 ms	|					|
			- zLib:				 25 ms	|					|
			*/
			constexpr size_t count = 20000;
			constexpr uint32_t maxL = 15;
			CloakEngine::Global::Log::WriteToLog("Generate data...");
			std::srand(static_cast<unsigned int>(std::time(nullptr)));
			uint32_t data[count];
			uint32_t length[count];
			for (size_t a = 0; a < count; a++)
			{
				length[a] = static_cast<uint32_t>(maxL*(static_cast<long double>(std::rand()) / RAND_MAX)) + 1;
				const uint32_t maxV = (1 << length[a]) - 1;
				data[a] = static_cast<uint32_t>(maxV*(static_cast<long double>(std::rand()) / RAND_MAX));
			}
			for (size_t c = 0; c < 7; c++)
			{
				bool bas = false;
				std::u16string filePath = u"module:test_";
				if (args[2].Type == CloakEngine::Global::Debug::CommandArgumentType::OPTION) { c = args[2].Value.Option; bas = true; }
				CloakEngine::Files::CompressType ct = CloakEngine::Files::CompressType::NONE;
				std::string name = "Unknown";
				if (c == 0) { name = "Normal"; filePath += u"Normal"; }
				else if (c == 1) { ct = CloakEngine::Files::CompressType::HUFFMAN; name = "Huffman"; filePath += u"Huffman"; }
				else if (c == 2) { ct = CloakEngine::Files::CompressType::LZC; name = "LZC"; filePath += u"LZC"; }
				else if (c == 3) { ct = CloakEngine::Files::CompressType::LZW; name = "LZW"; filePath += u"LZW"; }
				else if (c == 4) { ct = CloakEngine::Files::CompressType::LZH; name = "LZH (LZC + Huffman)"; filePath += u"LZH"; }
				//else if (c == 5) { ct = CloakEngine::Files::CompressType::ARITHMETIC; name = "Arithmetic"; filePath += u"Arithmetic"; }
				else if (c == 5) { ct = CloakEngine::Files::CompressType::LZ4; name = "LZ4"; filePath += u"LZ4"; }
				else if (c == 6) { ct = CloakEngine::Files::CompressType::ZLIB; name = "zLib"; filePath += u"ZLIB"; }
				filePath += u".cetf";
				CloakEngine::Global::Time start = CloakEngine::Global::Game::GetTimeStamp();
				CloakEngine::Global::Log::WriteToLog(name + ": Write data...");
				CloakEngine::Files::IWriter* write = nullptr;
				CREATE_INTERFACE(CE_QUERY_ARGS(&write));
				write->SetTarget(filePath, type, ct);
				for (size_t a = 0; a < count; a++) { write->WriteBits(length[a], data[a]); }
				write->Save();
				write->Release();

				CloakEngine::Global::Time stop = CloakEngine::Global::Game::GetTimeStamp();
				CloakEngine::Global::Log::WriteToLog(name + ": Write took " + std::to_string(stop - start) + " ms, Read data...");
				start = stop;

				size_t errC = 0;
				CloakEngine::Files::IReader* read = nullptr;
				CREATE_INTERFACE(CE_QUERY_ARGS(&read));
				if (read->SetTarget(filePath, type) > 0)
				{
					for (size_t a = 0; a < count; a++)
					{
						const unsigned long long byP = read->GetPosition();
						const unsigned long long biP = read->GetBitPosition();
						const uint32_t r = static_cast<uint32_t>(read->ReadBits(length[a]));
						if (r != data[a])
						{
							CloakEngine::Global::Log::WriteToLog("\t[" + std::to_string(a) + "]: " + std::to_string(data[a]) + " = " + std::to_string(r) + " (Len = " + std::to_string(length[a]) + " bitPos = " + std::to_string(biP) + " bytePos = " + std::to_string(byP) + ")");
							errC++;
						}
					}
				}
				else
				{
					CloakEngine::Global::Log::WriteToLog(name + ": Failed to read file!");
				}
				read->Release();
				stop = CloakEngine::Global::Game::GetTimeStamp();
				CloakEngine::Global::Log::WriteToLog(name + ": Read complete, took " + std::to_string(stop - start) + " ms, " + std::to_string(errC) + " Errors found!");
				if (bas) { break; }
			}

			return true;
		});
		CloakEngine::Global::Debug::RegisterConsoleArgument("debug font load", "debugging font laoding", [=](In const CloakEngine::Global::Debug::CommandArgument* args, In size_t argCount) {
			SAVE_RELEASE(g_debugFont);
			CloakEngine::Files::OpenFile(u".\\fonts\\DefaultFont.cebf", CE_QUERY_ARGS(&g_debugFont));
			g_debugFont->load();
			return true;
		});
		CloakEngine::Global::Debug::RegisterConsoleArgument("debug CGID", "generates a random CGID", [=](In const CloakEngine::Global::Debug::CommandArgument* args, In size_t argCount) {
			std::random_device r;
			std::default_random_engine e(r());
			std::uniform_int_distribution<uint16_t> adist(0, 255);
			std::uniform_int_distribution<uint32_t> bdist(0);
			std::uniform_int_distribution<uint16_t> cdist(0);

			std::stringstream ss;
			ss << "Generating CGID...\nKey: {";
			uint32_t key[8], vec[8];
			uint16_t id[24];
			uint16_t moved = cdist(e);
			for (size_t a = 0; a < 8; a++) { key[a] = bdist(e); ss << key[a] << ", "; }
			ss << "}\nVector: {";
			for (size_t a = 0; a < 8; a++) { vec[a] = bdist(e); ss << vec[a] << ", "; }
			ss << "}\nID: {";
			for (size_t a = 0; a < 24; a++) { id[a] = adist(e); ss << id[a] << ", "; }
			ss << "}\nMoved = " << moved << "\n Final:\n CGID(";
			for (size_t a = 0; a < 8; a++) { ss << key[a] << ", "; }
			for (size_t a = 0; a < 8; a++) { ss << vec[a] << ", "; }
			for (size_t a = 0; a < 24; a++) { ss << id[a] << ", "; }
			ss << moved << ")";
			CloakDebugLog(ss.str());
			return true;
		});
		CloakEngine::Global::Debug::RegisterConsoleArgument("debug config", "test reading config file", [=](In const CloakEngine::Global::Debug::CommandArgument* args, In size_t argCount) {
			CloakEngine::Files::IConfiguration* config = nullptr;
			CREATE_INTERFACE(CE_QUERY_ARGS(&config));
			CloakDebugLog("Start test");
			config->readFile(u"$APPDATA\\CloakEngineTest\\test.json", CloakEngine::Files::ConfigType::JSON);
			CloakDebugLog("Save file");
			config->saveFile(u".\\test_out.json", CloakEngine::Files::ConfigType::JSON);
			CloakDebugLog("Saved!");
			config->readFile(u".\\test_out.json", CloakEngine::Files::ConfigType::JSON);
			CloakDebugLog("Reloaded!");
			/*
			std::vector<std::string> sections;
			config->listSections(&sections);
			size_t f = 0;
			for (size_t a = 0; a < sections.size(); a++)
			{
			std::vector<std::string> keys;
			std::string sec = sections[a];
			config->listKeys(sec, &keys);
			for (size_t b = 0; b < keys.size(); b++)
			{
			std::string secK = sec + "." + keys[b];
			CloakDebugLog("Found entrance '" + secK + "'");
			f++;
			}
			}
			CloakDebugLog("Config test completed, found " + std::to_string(f) + " values!");
			*/
			SAVE_RELEASE(config);
			return true;
		});
		CloakEngine::Global::Debug::RegisterConsoleArgument("debug kmp", "test knuth-morris-pratt algorithm", [=](In const CloakEngine::Global::Debug::CommandArgument* args, In size_t argCount) {
			constexpr size_t n = 18;
			constexpr size_t m = 20;
			BYTE w[n] = { 'a','b','a','b','c','a','b','a','b','a','b','c','a','b','a','b','c','a' };
			BYTE t[m] = { 'a','b','a','b','a','b','c','b','a','b','a','b','c','a','b','a','b','c','a','b' };
			int N[n + 1];

			for (size_t o = 0; o < n; o++)
			{
				int len = 1;
				int off = 0;

				int i = 0;
				int j = -1;
				N[i] = j;
				while (i < 0 || static_cast<size_t>(i) < static_cast<size_t>(n - o))
				{
					while (j >= 0 && w[i + o] != w[j + o]) { j = N[j]; }
					j++;
					i++;
					N[i] = j;
				}

				std::stringstream ss;
				ss << "Prefixes:\n";
				for (size_t a = 0; a < n - o; a++) { ss << "   " << w[a + o]; }
				ss << "\n";
				for (size_t a = 0; a < n + 1 - o; a++)
				{
					std::string b = std::to_string(N[a]);
					for (size_t c = 0; c < 4 - b.length(); c++) { ss << " "; }
					ss << b;
				}
				CloakDebugLog(ss.str());
			}
			return true;
		});
		CloakEngine::Global::Debug::RegisterConsoleArgument("debug lobby %o(open|close)", "test lobby open/close", [=](In const CloakEngine::Global::Debug::CommandArgument* args, In size_t argCount) {
			switch (args[2].Value.Option)
			{
				case 0:
					CloakDebugLog("Open Lobby...");
					CloakEngine::Global::Lobby::Open(GameID::Test);
					break;
				case 1:
					CloakDebugLog("Close Lobby");
					CloakEngine::Global::Lobby::Close();
					break;
				default:
					break;
			}
			return true;
		});
		CloakEngine::Global::Debug::RegisterConsoleArgument("debug lobby connect %o(tcp|udp)", "test lobby client connection", [=](In const CloakEngine::Global::Debug::CommandArgument* args, In size_t argCount) {
			CloakEngine::Global::Lobby::TPType tpt = CloakEngine::Global::Lobby::TPType::TCP;
			std::string tptName = "";
			switch (args[3].Value.Option)
			{
				case 0:
					tpt = CloakEngine::Global::Lobby::TPType::TCP;
					tptName = "TCP";
					break;
				case 1:
					tpt = CloakEngine::Global::Lobby::TPType::UDP;
					tptName = "UDP";
					break;
			}
			CloakDebugLog("Try to connect to " + tptName + " Lobby");
			CloakEngine::Global::Lobby::Connect(CloakEngine::Global::Lobby::IPAddress("127.0.0.1"), tpt, GameID::Test);
			return true;
		});
		CloakEngine::Global::Debug::RegisterConsoleArgument("debug connect http %s(host)", "test connection to extern http address", [=](In const CloakEngine::Global::Debug::CommandArgument* args, In size_t argCount) {
			CloakDebugLog("Try connecting via http...");
			//CloakEngine::Global::Lobby::Connect(CloakEngine::Files::IPAddress("34.224.230.241", 80), CloakEngine::Global::Lobby::TPType::TCP);
			//CloakEngine::Global::Lobby::Connect(CloakEngine::Files::IPAddress("http://httpbin.org/", 80), CloakEngine::Global::Lobby::TPType::TCP);
			CloakEngine::Files::IHTTPReadBuffer* buffer = CloakEngine::Files::CreateHTTPRead(args[3].Value.String);
			//CloakEngine::Files::IHTTPReadBuffer* buffer = CloakEngine::Files::CreateHHTPRead("34.224.230.241", "httpbin.org", 80, "/gzip");

			while (buffer->WaitForNextData())
			{
				CloakEngine::Files::IReader* read = nullptr;
				CREATE_INTERFACE(CE_QUERY_ARGS(&read));
				read->SetTarget(buffer, CloakEngine::Files::CompressType::NONE, true);

				CloakDebugLog("Status Code: " + std::to_string(buffer->GetStatusCode()));
				std::string line = "";
				while (read->ReadLine(&line))
				{
					CloakDebugLog("Got answer: " + line);
				}
				read->Release();
			}
			CloakDebugLog("Connection close");

			SAVE_RELEASE(buffer);
			return true;
		});
		CloakEngine::Global::Debug::RegisterConsoleArgument("debug math", "test new math", [=](In const CloakEngine::Global::Debug::CommandArgument* args, In size_t argCount) {
			std::function<void(std::string, const CloakEngine::Global::Math::Matrix&)> mprintm = [](std::string st, const CloakEngine::Global::Math::Matrix& m)
			{
				float d[4][4];
				for (size_t a = 0; a < 4; a++) { _mm_store_ps(d[a], m.Row[a]); }
				std::string s = st;
				for (size_t a = 0; a < 4; a++)
				{
					s += "\n| ";
					for (size_t b = 0; b < 4; b++)
					{
						uint32_t* i = reinterpret_cast<uint32_t*>(&d[a][b]);
						if (((*i) & 0x80000000) == 0) { s += " "; }
						s += std::to_string(d[a][b]) + " | ";
					}
				}
				CloakDebugLog(s);
			};
			std::function<void(std::string, const CloakEngine::Global::Math::Vector&)> mprintv = [](std::string st, const CloakEngine::Global::Math::Vector& m)
			{
				std::string s = st;
				float d[4];
				_mm_store_ps(d, m.Data);
				for (size_t a = 0; a < 4; a++) { s += " " + std::to_string(d[a]) + " |"; }
				CloakDebugLog(s);
			};
			std::function<void(std::string, const CloakEngine::Global::Math::Quaternion&)> mprintq = [](std::string st, const CloakEngine::Global::Math::Quaternion& m)
			{
				std::string s = st;
				float d[4];
				_mm_store_ps(d, m.Data);
				s += "I: " + std::to_string(d[0]) + " J: " + std::to_string(d[1]) + " K: " + std::to_string(d[2]) + " R: " + std::to_string(d[3]);
				CloakDebugLog(s);
			};
			std::function<void(std::string, const CloakEngine::Global::Math::DualQuaternion&)> mprintd = [=](std::string st, const CloakEngine::Global::Math::DualQuaternion& q)
			{
				/*
				std::string s = st+"\nReal:";
				float d[4];
				float r[4];
				_mm_store_ps(d, q.Dual.Data);
				_mm_store_ps(r, q.Real.Data);
				for (size_t a = 0; a < 4; a++) { s += " " + std::to_string(r[a]) + " |"; }
				s += "\nDual:";
				for (size_t a = 0; a < 4; a++) { s += " " + std::to_string(d[a]) + " |"; }
				CloakDebugLog(s);
				*/
				//CloakEngine::Global::Math::DualQuaternion dq(q);
				mprintm(st, static_cast<CloakEngine::Global::Math::Matrix>(q));
			};
			std::function<void(std::string, const CloakEngine::Global::Math::Frustum&)> mprintf = [=](std::string st, const CloakEngine::Global::Math::Frustum& q)
			{
				std::string s = st;
				for (size_t a = 0; a < 6; a++)
				{
					s += "\n\tPlane[" + std::to_string(a) + "]: Normal = ";
					float d[4];
					CloakEngine::Global::Math::Vector n = q.Data[a].GetNormal();
					_mm_store_ps(d, n.Data);
					for (size_t b = 0; b < 4; b++)
					{
						uint32_t* i = reinterpret_cast<uint32_t*>(&d[b]);
						if (((*i) & 0x80000000) == 0) { s += " "; }
						s += std::to_string(d[b]) + " | ";
					}
					s += " Length = " + std::to_string(n.Length()) + " Data = ";
					_mm_store_ps(d, q.Data[a].Data);
					for (size_t b = 0; b < 4; b++)
					{
						uint32_t* i = reinterpret_cast<uint32_t*>(&d[b]);
						if (((*i) & 0x80000000) == 0) { s += " "; }
						s += std::to_string(d[b]) + " | ";
					}
				}
				CloakDebugLog(s);
			};

			CloakEngine::Global::Math::Matrix m1 = CloakEngine::Global::Math::Matrix::Translate(1, 2, 3);
			CloakEngine::Global::Math::Matrix m2 = CloakEngine::Global::Math::Matrix::Translate(8, 13, 21);
			CloakEngine::Global::Math::Matrix m3 = CloakEngine::Global::Math::Matrix::Rotation(0.3f, -1.2f, 2.1f, CloakEngine::Global::Math::RotationOrder::XYZ);

			mprintm("M1: ", m1);
			mprintm("M2: ", m2);
			mprintm("M3: ", m3);
			mprintm("M1 * M2: ", m1*m2);
			mprintm("M2 * M1: ", m2*m1);
			mprintm("M1 * M3: ", m1*m3);
			mprintm("M1 * M3 * M2:", m1*m3*m2);

			return true;
		});
		CloakEngine::Global::Debug::RegisterConsoleArgument("debug settings texQuality %o(low|medium|high|ultra)", "test texture quality changing", [=](In const CloakEngine::Global::Debug::CommandArgument* args, In size_t argCount) {
			CloakEngine::Global::Graphic::Settings gset;
			CloakEngine::Global::Graphic::GetSettings(&gset);
			switch (args[3].Value.Option)
			{
				case 0:
					gset.TextureQuality = CloakEngine::Global::Graphic::TextureQuality::LOW;
					break;
				case 1:
					gset.TextureQuality = CloakEngine::Global::Graphic::TextureQuality::MEDIUM;
					break;
				case 2:
					gset.TextureQuality = CloakEngine::Global::Graphic::TextureQuality::HIGH;
					break;
				case 3:
					gset.TextureQuality = CloakEngine::Global::Graphic::TextureQuality::ULTRA;
					break;
			}
			CloakEngine::Global::Graphic::SetSettings(gset);

			return true;
		});
		CloakEngine::Global::Debug::RegisterConsoleArgument("debug settings tessellation %o(on|off)", "test tessellation changing", [=](In const CloakEngine::Global::Debug::CommandArgument* args, In size_t argCount) {
			CloakEngine::Global::Graphic::Settings gset;
			CloakEngine::Global::Graphic::GetSettings(&gset);
			switch (args[3].Value.Option)
			{
				case 0:
					gset.Tessellation = true;
					break;
				case 1:
					gset.Tessellation = false;
					break;
			}
			CloakEngine::Global::Graphic::SetSettings(gset);

			return true;
		});
		CloakEngine::Global::Debug::RegisterConsoleArgument("debug settings bloom %o(on|off|hq)", "test bloom changing", [=](In const CloakEngine::Global::Debug::CommandArgument* args, In size_t argCount) {
			CloakEngine::Global::Graphic::Settings gset;
			CloakEngine::Global::Graphic::GetSettings(&gset);
			switch (args[3].Value.Option)
			{
				case 0:
					gset.Bloom = CloakEngine::Global::Graphic::BloomMode::NORMAL;
					break;
				case 1:
					gset.Bloom = CloakEngine::Global::Graphic::BloomMode::DISABLED;
					break;
				case 2:
					gset.Bloom = CloakEngine::Global::Graphic::BloomMode::HIGH_QUALITY;
					break;
			}
			CloakEngine::Global::Graphic::SetSettings(gset);

			return true;
		});
		CloakEngine::Global::Debug::RegisterConsoleArgument("debug dynamicWrite %d(start) %d(end)", "test dynamicWrite in range from [start, end[", [=](In const CloakEngine::Global::Debug::CommandArgument* args, In size_t argCount) {
			/*
			Best Result (Range 0 to 2^24): Configuration 16 With averange ~4 bits
			*/
			const int start = args[2].Value.Integer;
			const int end = args[2].Value.Integer;
			if (start >= 0 && end > start)
			{

				typedef std::function<size_t(uint64_t)> WriteMode;

				WriteMode m1 = [](In uint64_t a) {
					if (a == 0 || a == 1) { return static_cast<size_t>(3); }
					uint64_t ld = 0;
					size_t r = 0;
					while (a != 1)
					{
						const uint64_t f = a & 0x1;
						if (f == ld) { r += 1; }
						else { r += 2; ld = f; }
						a >>= 1;
					}
					r += 2;
					return r;
				};
				WriteMode m2 = [=](In uint64_t a) {
					if (a == 0 || a == 1) { return static_cast<size_t>(3); }
					size_t r = 0;
					while (a != 1)
					{
						const uint64_t f = a & 0x1;
						if (f == 0) { r += 1; }
						else { r += 2; }
						a >>= 1;
					}
					r += 2;
					return r;
				};
				WriteMode m3 = [=](In uint64_t a) {
					if (a == 0 || a == 1) { return static_cast<size_t>(3); }
					size_t r = 0;
					while (a != 1)
					{
						const uint64_t f = a & 0x1;
						if (f != 0) { r += 1; }
						else { r += 2; }
						a >>= 1;
					}
					r += 2;
					return r;
				};
				WriteMode m4 = [=](In uint64_t a) {
					size_t r = 6;
					do {
						r += 1;
						a >>= 1;
					} while (a != 0);
					return r;
				};
				WriteMode m5 = [=](In uint64_t a) {
					size_t r = 3;
					do {
						r += 8;
						a >>= 8;
					} while (a != 0);
					return r;
				};

				WriteMode modes[] = { m1,m2,m3,m4,m5 };
				constexpr size_t modec = ARRAYSIZE(modes);
				constexpr uint64_t mmd = (1ULL << modec);

				long double minCnf = 0;
				uint64_t minMd = 0;
				for (uint64_t md = 1; md < mmd; md++)
				{
					CloakEngine::Global::Log::WriteToLog("Configuration " + std::to_string(md), CloakEngine::Global::Log::Type::Console);
					size_t cnfCnt = 0;
					uint64_t cnf = md;
					while (cnf != 0)
					{
						if ((cnf & 0x1) != 0) { cnfCnt++; }
						cnf >>= 1;
					}
					size_t cnfS = 0;
					while (cnfCnt != 0)
					{
						cnfS++;
						cnfCnt >>= 1;
					}
					uint64_t bitSize = 0;
					for (uint64_t data = start; data < end; data++)
					{
						cnf = md;
						std::vector<size_t> bitS;
						for (size_t i = 0; cnf != 0; i++, cnf >>= 1)
						{
							if ((cnf & 0x1) != 0)
							{
								bitS.push_back(modes[i](data));
							}
						}
						size_t minBs = 0;
						for (size_t i = 0; i < bitS.size(); i++)
						{
							size_t bs = bitS[i];
							if (i == 0 || bs < minBs) { minBs = bs; }
						}
						size_t dms = 0;
						for (uint64_t di = data; di != 0; di >>= 1)
						{
							dms++;
						}
						bitSize += cnfS + minBs - dms;
					}
					const long double avg = static_cast<long double>(bitSize) / (end - start);
					CloakEngine::Global::Log::WriteToLog("\tAverange: " + std::to_string(avg) + " Configuration Size: " + std::to_string(cnfS), CloakEngine::Global::Log::Type::Console);
					if (md == 1 || avg < minCnf)
					{
						minCnf = avg;
						minMd = md;
					}
				}
				CloakEngine::Global::Log::WriteToLog("Best Result: Configuration " + std::to_string(minMd) + " with averange size " + std::to_string(minCnf), CloakEngine::Global::Log::Type::Info);

				return true;
			}
			return false;
		});
		CloakEngine::Global::Debug::RegisterConsoleArgument("debug mesh compile", "test mesh creation", [=](In const CloakEngine::Global::Debug::CommandArgument* args, In size_t argCount) {
			CloakCompiler::Mesh::Desc desc;
			CloakCompiler::Mesh::Vertex vert;
			//face 1:
			vert.Position = CloakEngine::Global::Math::Vector(0, 0, 0);
			vert.TexCoord.U = 0;
			vert.TexCoord.V = 1;
			desc.Vertices.push_back(vert);
			vert.Position = CloakEngine::Global::Math::Vector(0, 4, 0);
			vert.TexCoord.U = 0;
			vert.TexCoord.V = 0;
			desc.Vertices.push_back(vert);
			vert.Position = CloakEngine::Global::Math::Vector(5, 0, 0);
			vert.TexCoord.U = 1;
			vert.TexCoord.V = 1;
			desc.Vertices.push_back(vert);
			vert.Position = CloakEngine::Global::Math::Vector(5, 4, 0);
			vert.TexCoord.U = 1;
			vert.TexCoord.V = 0;
			desc.Vertices.push_back(vert);
			//face 2:
			vert.Position = CloakEngine::Global::Math::Vector(0, 0, 6);
			vert.TexCoord.U = 1;
			vert.TexCoord.V = 1;
			desc.Vertices.push_back(vert);
			vert.Position = CloakEngine::Global::Math::Vector(0, 4, 6);
			vert.TexCoord.U = 1;
			vert.TexCoord.V = 0;
			desc.Vertices.push_back(vert);
			vert.Position = CloakEngine::Global::Math::Vector(5, 0, 6);
			vert.TexCoord.U = 0;
			vert.TexCoord.V = 1;
			desc.Vertices.push_back(vert);
			vert.Position = CloakEngine::Global::Math::Vector(5, 4, 6);
			vert.TexCoord.U = 0;
			vert.TexCoord.V = 0;
			desc.Vertices.push_back(vert);
			//face 3:
			vert.Position = CloakEngine::Global::Math::Vector(0, 0, 0);
			vert.TexCoord.U = 1;
			vert.TexCoord.V = 1;
			desc.Vertices.push_back(vert);
			vert.Position = CloakEngine::Global::Math::Vector(0, 0, 6);
			vert.TexCoord.U = 0;
			vert.TexCoord.V = 1;
			desc.Vertices.push_back(vert);
			vert.Position = CloakEngine::Global::Math::Vector(0, 4, 0);
			vert.TexCoord.U = 1;
			vert.TexCoord.V = 0;
			desc.Vertices.push_back(vert);
			vert.Position = CloakEngine::Global::Math::Vector(0, 4, 6);
			vert.TexCoord.U = 0;
			vert.TexCoord.V = 0;
			desc.Vertices.push_back(vert);
			//face 4:
			vert.Position = CloakEngine::Global::Math::Vector(5, 0, 0);
			vert.TexCoord.U = 0;
			vert.TexCoord.V = 1;
			desc.Vertices.push_back(vert);
			vert.Position = CloakEngine::Global::Math::Vector(5, 0, 6);
			vert.TexCoord.U = 1;
			vert.TexCoord.V = 1;
			desc.Vertices.push_back(vert);
			vert.Position = CloakEngine::Global::Math::Vector(5, 4, 0);
			vert.TexCoord.U = 0;
			vert.TexCoord.V = 0;
			desc.Vertices.push_back(vert);
			vert.Position = CloakEngine::Global::Math::Vector(5, 4, 6);
			vert.TexCoord.U = 1;
			vert.TexCoord.V = 0;
			desc.Vertices.push_back(vert);
			//face 5:
			vert.Position = CloakEngine::Global::Math::Vector(0, 0, 0);
			vert.TexCoord.U = 0;
			vert.TexCoord.V = 0;
			desc.Vertices.push_back(vert);
			vert.Position = CloakEngine::Global::Math::Vector(0, 0, 6);
			vert.TexCoord.U = 1;
			vert.TexCoord.V = 0;
			desc.Vertices.push_back(vert);
			vert.Position = CloakEngine::Global::Math::Vector(5, 0, 0);
			vert.TexCoord.U = 0;
			vert.TexCoord.V = 1;
			desc.Vertices.push_back(vert);
			vert.Position = CloakEngine::Global::Math::Vector(5, 0, 6);
			vert.TexCoord.U = 1;
			vert.TexCoord.V = 1;
			desc.Vertices.push_back(vert);
			//face 6:
			vert.Position = CloakEngine::Global::Math::Vector(0, 4, 0);
			vert.TexCoord.U = 0;
			vert.TexCoord.V = 0;
			desc.Vertices.push_back(vert);
			vert.Position = CloakEngine::Global::Math::Vector(0, 4, 6);
			vert.TexCoord.U = 1;
			vert.TexCoord.V = 0;
			desc.Vertices.push_back(vert);
			vert.Position = CloakEngine::Global::Math::Vector(5, 4, 0);
			vert.TexCoord.U = 0;
			vert.TexCoord.V = 1;
			desc.Vertices.push_back(vert);
			vert.Position = CloakEngine::Global::Math::Vector(5, 4, 6);
			vert.TexCoord.U = 1;
			vert.TexCoord.V = 1;
			desc.Vertices.push_back(vert);

			CloakCompiler::Mesh::Polygon poly;
			poly.AutoGenerateNormal = true;
			//face 1:
			poly.Material = 0;
			poly.Point[0] = 0;
			poly.Point[1] = 1;
			poly.Point[2] = 3;
			desc.Polygons.push_back(poly);
			poly.Point[0] = 0;
			poly.Point[1] = 3;
			poly.Point[2] = 2;
			desc.Polygons.push_back(poly);
			//face 2:
			poly.Material = 1;
			poly.Point[0] = 6;
			poly.Point[1] = 7;
			poly.Point[2] = 4;
			desc.Polygons.push_back(poly);
			poly.Point[0] = 7;
			poly.Point[1] = 5;
			poly.Point[2] = 4;
			desc.Polygons.push_back(poly);
			//face 3:
			poly.Material = 2;
			poly.Point[0] = 9;
			poly.Point[1] = 11;
			poly.Point[2] = 10;
			desc.Polygons.push_back(poly);
			poly.Point[0] = 9;
			poly.Point[1] = 10;
			poly.Point[2] = 8;
			desc.Polygons.push_back(poly);
			//face 4:
			poly.Material = 3;
			poly.Point[0] = 12;
			poly.Point[1] = 14;
			poly.Point[2] = 13;
			desc.Polygons.push_back(poly);
			poly.Point[0] = 13;
			poly.Point[1] = 14;
			poly.Point[2] = 15;
			desc.Polygons.push_back(poly);
			//face 5:
			poly.Material = 4;
			poly.Point[0] = 17;
			poly.Point[1] = 16;
			poly.Point[2] = 18;
			desc.Polygons.push_back(poly);
			poly.Point[0] = 17;
			poly.Point[1] = 18;
			poly.Point[2] = 19;
			desc.Polygons.push_back(poly);
			//face 6:
			poly.Material = 5;
			poly.Point[0] = 20;
			poly.Point[1] = 21;
			poly.Point[2] = 22;
			desc.Polygons.push_back(poly);
			poly.Point[0] = 22;
			poly.Point[1] = 21;
			poly.Point[2] = 23;
			desc.Polygons.push_back(poly);

			desc.Bounding = CloakCompiler::Mesh::BoundingVolume::OOBB;

			CloakCompiler::EncodeDesc encode;
			encode.tempPath = u"";
			encode.flags = CloakCompiler::EncodeFlags::NO_TEMP_READ | CloakCompiler::EncodeFlags::NO_TEMP_WRITE;
			encode.targetGameID = GameID::Test;

			CloakEngine::Files::IWriter* output = nullptr;
			CloakEngine::Files::IWriteBuffer* outBuf = CloakEngine::Files::CreateVirtualWriteBuffer();
			CREATE_INTERFACE(CE_QUERY_ARGS(&output));
			CloakCompiler::Mesh::EncodeToFile(output, encode, desc, [=](const CloakCompiler::Mesh::RespondeInfo& i) {
				CloakDebugLog("Got response: " + std::to_string(static_cast<unsigned int>(i.Code)) + ": '" + i.Msg + "' (P: " + std::to_string(i.Polygon) + " V: " + std::to_string(i.Vertex) + ")");
			});

			SAVE_RELEASE(output);
			SAVE_RELEASE(outBuf);

			return true;
		});
		CloakEngine::Global::Debug::RegisterConsoleArgument("debug readback", "test readback functionallity", [=](In const CloakEngine::Global::Debug::CommandArgument* args, In size_t argCount) {
			uint8_t testData[256];
			for (size_t a = 0; a < ARRAYSIZE(testData); a++) { testData[a] = static_cast<uint8_t>(((a << 1) + (ARRAYSIZE(testData) >> 1)) & 0xFF); }
			uint8_t td2[ARRAYSIZE(testData)];
			for (size_t a = 0; a < ARRAYSIZE(td2); a++) { td2[a] = static_cast<uint8_t>((static_cast<uint32_t>(testData[a]) + 13) & 0xFF); }
			uint8_t td3[ARRAYSIZE(td2)];
			for (size_t a = 0; a < ARRAYSIZE(td3); a++) { td3[a] = static_cast<uint8_t>((static_cast<uint32_t>(td2[a]) + 17) & 0xFF); }
			CloakEngine::Rendering::IManager* man = nullptr;
			CloakEngine::Global::Graphic::GetManager(CE_QUERY_ARGS(&man));
			CloakEngine::Rendering::IContext* con = nullptr;
			man->BeginContext(CloakEngine::Rendering::CONTEXT_TYPE_GRAPHIC, 0, CE_QUERY_ARGS(&con));

			//Test Constant Buffer:
			CloakEngine::Rendering::IConstantBuffer* sb = man->CreateConstantBuffer(CloakEngine::Rendering::CONTEXT_TYPE_GRAPHIC, 0, ARRAYSIZE(testData), testData);
			CloakEngine::Rendering::ReadBack sbr = con->ReadBack(sb);
			CloakEngine::Rendering::IConstantBuffer* sb2 = man->CreateConstantBuffer(CloakEngine::Rendering::CONTEXT_TYPE_GRAPHIC, 0, ARRAYSIZE(td2), td2);
			CloakEngine::Rendering::ReadBack sbr2 = con->ReadBack(sb2);

			//Test Color Buffer:
			const UINT W = 16;
			const UINT H = 16;
			assert(W*H <= ARRAYSIZE(td3));
			CloakEngine::Helper::Color::RGBA colData[ARRAYSIZE(td3)];
			CloakEngine::Helper::Color::RGBA* cdp = colData;
			for (size_t a = 0; a < ARRAYSIZE(colData); a++)
			{
				colData[a].R = static_cast<float>(td3[a]) / 255;
				colData[a].G = colData[a].B = colData[a].A = 0;
			}
			CloakEngine::Rendering::TEXTURE_DESC desc;
			desc.AccessType = CloakEngine::Rendering::VIEW_TYPE::SRV;
			desc.clearValue = CloakEngine::Helper::Color::RGBA(0, 0, 0, 0);
			desc.Format = CloakEngine::Rendering::Format::R8_UINT;
			desc.Height = H;
			desc.Width = W;
			desc.Type = CloakEngine::Rendering::TEXTURE_TYPE::TEXTURE;
			desc.Texture.MipMaps = 1;
			desc.Texture.SampleCount = 1;
			CloakEngine::Rendering::IColorBuffer* cb = man->CreateColorBuffer(CloakEngine::Rendering::CONTEXT_TYPE_GRAPHIC, 0, desc, &cdp);
			CloakEngine::Rendering::ReadBack cbr = con->ReadBack(cb);

			con->CloseAndExecute();

			//Constant Buffer:
			CloakDebugLog("Test Constant Buffer:");
			uint8_t* sbd = reinterpret_cast<uint8_t*>(sbr.GetData());
			size_t errc = 0;
			size_t s = sbr.GetSize();
			if (s != ARRAYSIZE(testData)) { CloakDebugLog("\tWrong size (s = " + std::to_string(s) + ")"); }
			if (s >= ARRAYSIZE(testData))
			{
				for (size_t a = 0; a < ARRAYSIZE(testData); a++)
				{
					if (sbd[a] != testData[a]) { errc++; }
				}
				CloakDebugLog("\tFound " + std::to_string(errc) + " errors ( " + std::to_string((100.0f*errc) / ARRAYSIZE(testData)) + "% )");
			}
			//Constant Buffer 2:
			CloakDebugLog("Test Constant Buffer 2:");
			sbd = reinterpret_cast<uint8_t*>(sbr2.GetData());
			errc = 0;
			s = sbr2.GetSize();
			if (s != ARRAYSIZE(td2)) { CloakDebugLog("\tWrong size (s = " + std::to_string(s) + ")"); }
			if (s >= ARRAYSIZE(td2))
			{
				for (size_t a = 0; a < ARRAYSIZE(td2); a++)
				{
					if (sbd[a] != td2[a]) { errc++; }
				}
				CloakDebugLog("\tFound " + std::to_string(errc) + " errors ( " + std::to_string((100.0f*errc) / ARRAYSIZE(testData)) + "% )");
			}
			//Color Buffer:
			CloakDebugLog("Test Color Buffer:");
			errc = 0;
			sbd = reinterpret_cast<uint8_t*>(cbr.GetData());
			s = cbr.GetSize();
			if (s != ARRAYSIZE(colData)) { CloakDebugLog("\tWrong size (s = " + std::to_string(s) + ")"); }
			if (s >= ARRAYSIZE(colData))
			{
				for (size_t a = 0; a < ARRAYSIZE(colData); a++)
				{
					if (sbd[a] != td3[a]) { errc++; }
				}
				CloakDebugLog("\tFound " + std::to_string(errc) + " errors ( " + std::to_string((100.0f*errc) / ARRAYSIZE(testData)) + "% )");
				if (errc > 0)
				{
					std::stringstream str;
					str << "Comparrison:" << std::endl << "Should be:";
					for (size_t y = 0; y < H; y++)
					{
						str << std::endl << "\t";
						for (size_t x = 0; x < W; x++)
						{
							size_t p = (y * W) + x;
							if (x != 0) { str << " "; }
							if (td3[p] < 100) { str << " "; }
							if (td3[p] < 10) { str << " "; }
							str << " " << static_cast<uint32_t>(td3[p]) << " ";
						}
					}
					str << std::endl << "Read Data:";
					for (size_t y = 0; y < H; y++)
					{
						str << std::endl << "\t";
						for (size_t x = 0; x < W; x++)
						{
							size_t p = (y * W) + x;
							if (x != 0) { str << " "; }
							if (sbd[p] != td3[p])
							{
								if (sbd[p] < 100) { str << "_"; }
								if (sbd[p] < 10) { str << "_"; }
								str << "_" << static_cast<uint32_t>(sbd[p]) << "_";
							}
							else
							{
								if (sbd[p] < 100) { str << " "; }
								if (sbd[p] < 10) { str << " "; }
								str << " " << static_cast<uint32_t>(sbd[p]) << " ";
							}
						}
					}
					CloakEngine::Global::Log::WriteToLog(str.str(), CloakEngine::Global::Log::Type::File);
				}
			}


			SAVE_RELEASE(sb);
			SAVE_RELEASE(sb2);
			SAVE_RELEASE(cb);
			SAVE_RELEASE(man);
			SAVE_RELEASE(con);
			return true;
		});
		CloakEngine::Global::Debug::RegisterConsoleArgument("debug hammersley", "test hammersley", [=](In const CloakEngine::Global::Debug::CommandArgument* args, In size_t argCount) {
			const uint32_t numSamples = 1024;

			for (uint32_t a = 0; a < numSamples; a++)
			{
				float tA = static_cast<float>(a) / numSamples;

				uint32_t revA = a;
				revA = ((revA >> 1) & 0x55555555) | ((revA << 1) & 0xAAAAAAAA);
				revA = ((revA >> 2) & 0x33333333) | ((revA << 2) & 0xCCCCCCCC);
				revA = ((revA >> 4) & 0x0F0F0F0F) | ((revA << 4) & 0xF0F0F0F0);
				revA = ((revA >> 8) & 0x00FF00FF) | ((revA << 8) & 0xFF00FF00);
				revA = ((revA >> 16) & 0x0000FFFF) | ((revA << 16) & 0xFFFF0000);
				float tB = (1.0f / 4294967296.0f) * revA;

				float rA = static_cast<float>(a % (numSamples + 1)) / numSamples;
				float rB = 0;

				float pInv = 0.5f;
				uint32_t t = a;
				while (t > 0)
				{
					uint32_t d = t % 2;
					rB += d * pInv;
					pInv /= 2;
					t >>= 1;
				}

				CloakDebugLog("Hammersley(" + std::to_string(a) + "): Fast = [ " + std::to_string(tA) + " | " + std::to_string(tB) + " ] Detailed = [ " + std::to_string(rA) + " | " + std::to_string(rB) + " ]");
			}


			return true;
		});
	}
	void CLOAK_CALL Stop()
	{
		SAVE_RELEASE(g_debugFont);
		SAVE_RELEASE(g_debugImg);
		SAVE_RELEASE(g_debugMesh);
	}
}