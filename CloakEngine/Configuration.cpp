#include "stdafx.h"
#include "Implementation/Files/Configuration.h"

#include "CloakEngine/Global/Debug.h"
#include "CloakEngine/Global/Game.h"
#include "CloakEngine/Files/Compressor.h"
#include "CloakEngine/Helper/StringConvert.h"

#include <algorithm>
#include <sstream>

namespace CloakEngine {
	namespace Impl {
		namespace Files {
			inline namespace Configuration_v1 {
				constexpr API::Files::FileType g_fileTypeINI{ "","",1 };
				constexpr API::Files::FileType g_fileTypeJSON{ "","",1 };
				constexpr API::Files::FileType g_fileTypeXML{ "","",1 };

#ifdef _DEBUG
				std::string CLOAK_CALL print(API::Files::IValue* v, std::string tabs, std::string name)
				{
					std::string r = tabs + name + " = ";
					if (v->isArray())
					{
						r += "{\n";
						API::Files::IValue* w = nullptr;
						for (size_t a = 0; v->enumerateChildren(a, &w); a++)
						{
							r += print(w, tabs + "\t", "[" + std::to_string(a) + "]") + "\n";
						}
						r += tabs + "}";
					}
					else if (v->isObjekt())
					{
						r += "{\n";
						API::Files::IValue* w = nullptr;
						for (size_t a = 0; v->enumerateChildren(a, &w); a++)
						{
							r += print(w, tabs + "\t", "\"" + w->getName() + "\"") + "\n";
						}
						r += tabs + "}";
					}
					else if (v->isFloat()) { r += std::to_string(v->toFloat()); }
					else if (v->isInt()) { r += std::to_string(v->toInt()); }
					else if (v->isBool()) { r += v->toBool() ? "true" : "false"; }
					else if (v->isString()) { r += "\"" + v->toString() + "\""; }
					else { r += "null"; }
					return r;
				}
				void CLOAK_CALL print(API::Files::IValue* v, std::string text)
				{
					CloakDebugLog(text+"\n\n" + print(v, "", "root"));
				}
#endif

				namespace Parser {
					void CLOAK_CALL toString(Inout std::string* s, In char32_t c)
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

					namespace INI {
						enum class InputType { 
							SPACE = 0, 
							SECTION_START = 1, 
							SECTION_END = 2, 
							STRING = 3, 
							EQUAL = 4, 
							LETTER = 5, 
							NUMBER = 6, 
							POINT = 7, 
							COMMENT = 8, 
							SPECIAL = 9, 
							SIGN = 10, 
							NEWLINE = 11,
							INVALID = 12,
						};
						enum State { INVALID = 0, LN_BEGIN = 1, SECTION = 2, LN_END = 3, VAR_NAME = 4, SET = 5, VAL_BEGIN = 6, VAL_STR = 7, VAL_KEY = 8, VAL_INT = 9, VAL_FLO = 10, COMMENT = 11, STR_START = 12, SEC_START = 13 };
						constexpr size_t g_typeCount = static_cast<size_t>(InputType::INVALID);
						constexpr State g_machine[][g_typeCount] = {
							/*				  SPACE		SECTION_START	SECTION_END	STRING		EQUAL		LETTER		NUMBER		POINT		COMMENT		SPECIAL		SIGN		NEWLINE*/	
							/*INVALID*/		{ INVALID,	INVALID,		INVALID,	INVALID,	INVALID,	INVALID,	INVALID,	INVALID,	INVALID,	INVALID,	INVALID,	INVALID },
							/*LN_BEGIN*/	{ LN_BEGIN,	SEC_START,		INVALID,	INVALID,	INVALID,	VAR_NAME,	INVALID,	INVALID,	COMMENT,	INVALID,	INVALID,	LN_BEGIN },
							/*SECTION*/		{ INVALID,	INVALID,		LN_END,		INVALID,	INVALID,	SECTION,	SECTION,	INVALID,	INVALID,	SECTION,	INVALID,	INVALID },
							/*LN_END*/		{ LN_END,	INVALID,		INVALID,	INVALID,	INVALID,	INVALID,	INVALID,	INVALID,	INVALID,	INVALID,	INVALID,	LN_BEGIN },
							/*VAR_NAME*/	{ SET,		INVALID,		INVALID,	INVALID,	VAL_BEGIN,	VAR_NAME,	VAR_NAME,	INVALID,	INVALID,	VAR_NAME,	INVALID,	INVALID },
							/*SET*/			{ SET,		INVALID,		INVALID,	INVALID,	VAL_BEGIN,	INVALID,	INVALID,	INVALID,	INVALID,	INVALID,	INVALID,	INVALID },
							/*VAL_BEGIN*/	{ VAL_BEGIN,INVALID,		INVALID,	STR_START,	INVALID,	VAL_KEY,	VAL_INT,	VAL_FLO,	INVALID,	INVALID,	VAL_INT,	INVALID },
							/*VAL_STR*/		{ VAL_STR,	VAL_STR,		VAL_STR,	LN_END,		VAL_STR,	VAL_STR, 	VAL_STR, 	VAL_STR, 	VAL_STR,	VAL_STR,	VAL_STR,	INVALID },
							/*VAL_KEY*/		{ LN_END,	INVALID,		INVALID,	INVALID,	INVALID,	VAL_KEY,	INVALID,	INVALID,	INVALID,	INVALID,	INVALID,	LN_BEGIN },
							/*VAL_INT*/		{ LN_END,	INVALID,		INVALID,	INVALID,	INVALID,	INVALID,	VAL_INT,	VAL_FLO,	INVALID,	INVALID,	INVALID,	LN_BEGIN },
							/*VAL_FLO*/		{ LN_END,	INVALID,		INVALID,	INVALID,	INVALID,	INVALID,	VAL_FLO,	INVALID,	INVALID,	INVALID,	INVALID,	LN_BEGIN },
							/*COMMENT*/		{ COMMENT,	COMMENT,		COMMENT,	COMMENT,	COMMENT,	COMMENT,	COMMENT,	COMMENT,	COMMENT,	COMMENT,	COMMENT,	LN_BEGIN },
							/*STR_START*/	{ VAL_STR,	VAL_STR,		VAL_STR,	LN_END,		VAL_STR,	VAL_STR,	VAL_STR,	VAL_STR,	VAL_STR,	VAL_STR,	VAL_STR,	INVALID },
							/*SEC_START*/	{ INVALID,	INVALID,		INVALID,	INVALID,	INVALID,	SECTION,	SECTION,	INVALID,	INVALID,	SECTION,	INVALID,	INVALID },
						};
						bool CLOAK_CALL ReadBuffer(In API::Files::IReader* read, Out Value* out)
						{
							bool readErr = false;
							if (read != nullptr && out != nullptr)
							{
								out->clear();
								API::Files::Configuration_v1::IValue* section = out;
								API::Files::Configuration_v1::IValue* val = nullptr;
								State curState = State::LN_BEGIN;
								std::string r = "";
								while (!read->IsAtEnd() && readErr == false)
								{
									char32_t c = static_cast<char32_t>(read->ReadBits(32));
									InputType t = InputType::INVALID;
									if (c == '[') { t = InputType::SECTION_START; }
									else if (c == ']') { t = InputType::SECTION_END; }
									else if (c == '"') { t = InputType::STRING; }
									else if (c == '=') { t = InputType::EQUAL; }
									else if (c == '.') { t = InputType::POINT; }
									else if (c == ';') { t = InputType::COMMENT; }
									else if (c == ' ' || c == '\t' || c == 0xA0) { t = InputType::SPACE; }
									else if (c == '+' || c == '-') { t = InputType::SIGN; }
									else if (c == 0x0D || c == 0x0A) { t = InputType::NEWLINE; }
									else if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= 'ü' && c <= 'Ü')) { t = InputType::LETTER; }
									else if (c >= '0' && c <= '9') { t = InputType::NUMBER; }
									else if ((c > ' ' && c < '0') || (c > '9' && c < 'A') || (c > 'Z' && c < 'a') || (c > 'z' && c < 'ü') || c > 0x7F) { t = InputType::SPECIAL; }

									if (t == InputType::INVALID)
									{
										curState = State::INVALID;
									}
									else
									{
										const State lastS = curState;
										curState = g_machine[curState][static_cast<size_t>(t)];

										switch (curState)
										{
											case CloakEngine::Impl::Files::Configuration_v1::Parser::INI::VAL_STR:
											case CloakEngine::Impl::Files::Configuration_v1::Parser::INI::VAL_KEY:
											case CloakEngine::Impl::Files::Configuration_v1::Parser::INI::VAL_INT:
											case CloakEngine::Impl::Files::Configuration_v1::Parser::INI::VAL_FLO:
											case CloakEngine::Impl::Files::Configuration_v1::Parser::INI::VAR_NAME:
											case CloakEngine::Impl::Files::Configuration_v1::Parser::INI::SECTION:
												toString(&r, c);
												break;
											case CloakEngine::Impl::Files::Configuration_v1::Parser::INI::LN_BEGIN:
											case CloakEngine::Impl::Files::Configuration_v1::Parser::INI::LN_END:
												if (lastS == State::SECTION)
												{
													if (r.length() == 0 || out->has(r)) { curState = State::INVALID; }
													else
													{
														section = out->get(r);
													}
												}
												else if (lastS == State::VAL_FLO)
												{
													try {
														if (val == nullptr) { curState = State::INVALID; }
														else
														{
															val->set(static_cast<float>(std::atof(r.c_str())));
														}
													}
													catch (...) { curState = State::INVALID; }
												}
												else if (lastS == State::VAL_INT)
												{
													try {
														if (val == nullptr) { curState = State::INVALID; }
														else
														{
															val->set(static_cast<int>(std::stoi(r)));
														}
													}
													catch (...) { curState = State::INVALID; }
												}
												else if (lastS == State::VAL_STR)
												{
													if (val == nullptr) { curState = State::INVALID; }
													else { val->set(r); }
												}
												else if (lastS == State::VAL_KEY)
												{
													std::transform(r.begin(), r.end(), r.begin(), tolower);
													if (val == nullptr) { curState = State::INVALID; }
													else if (r.compare("true") == 0 || r.compare("yes") == 0) { val->set(true); }
													else if (r.compare("false") == 0 || r.compare("no") == 0) { val->set(false); }
													else { curState = State::INVALID; }
												}
												r.clear();
												break;
											case CloakEngine::Impl::Files::Configuration_v1::Parser::INI::VAL_BEGIN:
												if (lastS != curState)
												{
													if (r.length() == 0 || section->has(r)) { curState = State::INVALID; }
													else
													{
														val = section->get(r);
													}
												}
												r.clear();
												break;
											default:
												break;
										}
									}
								}
								if (curState != LN_BEGIN && curState != LN_END && curState != COMMENT) { readErr = true; }
							}
							else { readErr = true; }
							return !readErr;
						}
						bool CLOAK_CALL readFile(In const API::Files::UFI& path, Out Value* out)
						{
							API::Files::IReader* read = nullptr;
							CREATE_INTERFACE(CE_QUERY_ARGS(&read));
							bool readErr = false;
							if (read->SetTarget(path, g_fileTypeINI, true) > 0) { readErr = !ReadBuffer(read, out); }
							else { readErr = true; }
							SAVE_RELEASE(read);
							return !readErr;
						}
						void CLOAK_CALL WriteBuffer(In API::Files::IWriter* write, In Value* root)
						{
							for (size_t a = 0; a < root->getChildCount(); a++)
							{
								Value* c = root->getChild(a);
								if (c->isInt()) { write->WriteString(c->getName() + "=" + std::to_string(c->toInt()) + "\n"); }
								else if (c->isFloat()) { write->WriteString(c->getName() + "=" + std::to_string(c->toFloat()) + "\n"); }
								else if (c->isString()) { write->WriteString(c->getName() + "=\"" + c->toString() + "\"\n"); }
								else if (c->isBool()) { write->WriteString(c->getName() + "=" + std::to_string(c->toBool()) + "\n"); }
							}
							for (size_t a = 0; a < root->getChildCount(); a++)
							{
								Value* c = root->getChild(a);
								if (c->isObjekt())
								{
									if (write->GetPosition() > 0) { write->WriteString("\n"); }
									write->WriteString("[" + c->getName() + "]\n");
									for (size_t b = 0; b < c->getChildCount(); b++)
									{
										Value* d = c->getChild(b);
										if (d->isInt()) { write->WriteString(d->getName() + "=" + std::to_string(d->toInt()) + "\n"); }
										else if (d->isFloat()) { write->WriteString(d->getName() + "=" + std::to_string(d->toFloat()) + "\n"); }
										else if (d->isString()) { write->WriteString(d->getName() + "=\"" + d->toString() + "\"\n"); }
										else if (d->isBool()) { write->WriteString(d->getName() + "=" + std::to_string(d->toBool()) + "\n"); }
									}
								}
							}
						}
						void CLOAK_CALL writeFile(In const API::Files::UFI& str, In Value* root)
						{
							API::Files::IWriter* write = nullptr;
							CREATE_INTERFACE(CE_QUERY_ARGS(&write));
							write->SetTarget(str, g_fileTypeINI, API::Files::CompressType::NONE, true);
							WriteBuffer(write, root);
							write->Save();
							SAVE_RELEASE(write);
						}
					}
					namespace JSON {
						enum class InputType {
							SPACE = 0,
							STRING = 1,
							OBJ_BEGIN =2,
							OBJ_END = 3,
							ARR_BEGIN = 4,
							ARR_END = 5,
							SIGN = 6,
							POINT = 7,
							E = 8,
							LETTER = 9,
							NUMBER = 10,
							EQUAL = 11,
							COMMA = 12,
							SPECIAL = 13, 
							INVALID = 14,
						};
						enum State {
							INVALID = 0,
							STR_BEGIN=1,
							VAL_STR=2,
							VAL_INT=3,
							VAL_FLO=4,
							VAL_EXP_B=5,
							VAL_EXP=6,
							VAL_KEY=7,
							EXP_VALUE_OBJ=8,
							EXP_VALUE_ARR=9,
							EXP_LNEND=10,
							EXP_KEY=11,
							EXP_EQUAL=12,
							KEY_BEGIN=13,
							KEY=14,
							FINISHED=15,

							AUTO_NXTVAL=100,
							AUTO_NXLEND=101,
							AUTO_BCKOBJ=102,
							AUTO_BCKARR=103
						};
						
						constexpr size_t g_typeSize = static_cast<size_t>(InputType::INVALID);

						constexpr State g_machine[][g_typeSize] = {
							/*					SPACE			STRING		OBJ_BEGIN	OBJ_END		ARR_BEGIN		ARR_END		SIGN		POINT		E			LETTER		NUMBER		EQUAL			COMMA			SPECIAL	*/
							/*INVALID*/			{ INVALID,		INVALID,	INVALID,	INVALID,	INVALID,		INVALID,	INVALID,	INVALID,	INVALID,	INVALID,	INVALID,	INVALID,		INVALID,		INVALID },
							/*STR_BEGIN*/		{ VAL_STR,		EXP_LNEND,	VAL_STR,	VAL_STR, 	VAL_STR, 		VAL_STR, 	VAL_STR, 	VAL_STR, 	VAL_STR, 	VAL_STR, 	VAL_STR, 	VAL_STR, 		VAL_STR, 		VAL_STR },
							/*VAL_STR*/			{ VAL_STR,		EXP_LNEND,	VAL_STR,	VAL_STR, 	VAL_STR, 		VAL_STR, 	VAL_STR, 	VAL_STR, 	VAL_STR, 	VAL_STR, 	VAL_STR, 	VAL_STR, 		VAL_STR, 		VAL_STR },
							/*VAL_INT*/			{ EXP_LNEND,	INVALID,	INVALID,	AUTO_BCKOBJ,INVALID,		AUTO_BCKARR,INVALID,	VAL_FLO,	VAL_EXP_B,	INVALID,	VAL_INT,	INVALID,		AUTO_NXTVAL,	INVALID },
							/*VAL_FLO*/			{ EXP_LNEND,	INVALID,	INVALID,	AUTO_BCKOBJ,INVALID,		AUTO_BCKARR,INVALID,	INVALID,	VAL_EXP_B,	INVALID,	VAL_FLO,	INVALID,		AUTO_NXTVAL,	INVALID },
							/*VAL_EXP_B*/		{ INVALID,		INVALID,	INVALID,	INVALID,	INVALID,		INVALID,	VAL_EXP,	INVALID,	INVALID,	INVALID,	INVALID,	INVALID,		INVALID,		INVALID },
							/*VAL_EXP*/			{ EXP_LNEND,	INVALID,	INVALID,	AUTO_BCKOBJ,INVALID,		AUTO_BCKARR,INVALID,	INVALID,	INVALID,	INVALID,	VAL_EXP,	INVALID,		AUTO_NXTVAL,	INVALID },
							/*VAL_KEY*/			{ EXP_LNEND,	INVALID,	INVALID,	AUTO_BCKOBJ,INVALID,		AUTO_BCKARR,INVALID,	INVALID,	VAL_KEY,	VAL_KEY,	INVALID,	INVALID,		AUTO_NXTVAL,	INVALID },
							/*EXP_VALUE_OBJ*/	{ EXP_VALUE_OBJ,STR_BEGIN,	EXP_KEY,	INVALID,	EXP_VALUE_ARR,	INVALID,	VAL_INT,	INVALID,	VAL_KEY,	VAL_KEY,	VAL_INT,	INVALID,		INVALID,		INVALID },
							/*EXP_VALUE_ARR*/	{ EXP_VALUE_ARR,STR_BEGIN,	EXP_KEY,	INVALID,	EXP_VALUE_ARR,	AUTO_NXLEND,VAL_INT,	INVALID,	VAL_KEY,	VAL_KEY,	VAL_INT,	INVALID,		INVALID,		INVALID },
							/*EXP_LNEND*/		{ EXP_LNEND,	INVALID,	INVALID,	AUTO_BCKOBJ,INVALID,		AUTO_BCKARR,INVALID,	INVALID,	INVALID,	INVALID,	INVALID,	INVALID,		AUTO_NXTVAL,	INVALID },
							/*EXP_KEY*/			{ EXP_KEY,		KEY_BEGIN,	INVALID,	AUTO_NXLEND,INVALID,		INVALID,	INVALID,	INVALID,	INVALID,	INVALID,	INVALID,	INVALID,		INVALID,		INVALID },
							/*EXP_EQUAL*/		{ EXP_EQUAL,	INVALID,	INVALID,	INVALID,	INVALID,		INVALID,	INVALID,	INVALID,	INVALID,	INVALID,	INVALID,	EXP_VALUE_OBJ,	INVALID,		INVALID },
							/*KEY_BEGIN*/		{ KEY,			EXP_EQUAL,	KEY,		KEY,		KEY,			KEY,		KEY,		KEY,		KEY,		KEY,		KEY,		KEY,			KEY,			KEY		},
							/*KEY*/				{ KEY,			EXP_EQUAL,	KEY,		KEY,		KEY,			KEY,		KEY,		KEY,		KEY,		KEY,		KEY,		KEY,			KEY,			KEY		},
							/*FINISHED*/		{ FINISHED,		INVALID,	INVALID,	INVALID,	INVALID,		INVALID,	INVALID,	INVALID,	INVALID,	INVALID,	INVALID,	INVALID,		FINISHED,		INVALID },
						};

#define READ_JSON_ERR(txt) readErr=true; CloakDebugLog(txt)
						bool CLOAK_CALL ReadBuffer(In API::Files::IReader* read, Out Value* out)
						{
							bool readErr = false;
							if (read != nullptr && out != nullptr)
							{
								out->clear();
								State curState = EXP_VALUE_OBJ;
								State lvState = curState;
								State lState = curState;
								char32_t c = 0;
								API::Files::IValue* curVal = out;
								bool esc = false;
								std::string r = "";
								bool doBreak = false;
								bool eol = false;
								while (doBreak == false && readErr == false)
								{
									if (!read->IsAtEnd())
									{
										c = static_cast<char32_t>(read->ReadBits(32));
									}
									else
									{
										doBreak = true;
										c = ',';
									}
									if (readErr == false)
									{
										InputType it = InputType::INVALID;
										if (c == '\\') { esc = !esc; it = InputType::SPECIAL; }
										else if (c == 0x0009 || c == 0x000A || c == 0x000D || c == 0x0020 || c == 0x0085 || c == 0x200E || c == 0x200F || c == 0x2028 || c == 0x2029) { it = InputType::SPACE; }
										else if (c == '+' || c == '-') { it = InputType::SIGN; }
										else if (c == '"') { it = esc ? InputType::SPECIAL : InputType::STRING; }
										else if (c == '{') { it = InputType::OBJ_BEGIN; }
										else if (c == '}') { it = InputType::OBJ_END; }
										else if (c == '[') { it = InputType::ARR_BEGIN; }
										else if (c == ']') { it = InputType::ARR_END; }
										else if (c == '.') { it = InputType::POINT; }
										else if (c == 'e' || c == 'E') { it = InputType::E; }
										else if (c >= '0' && c <= '9') { it = InputType::NUMBER; }
										else if (c == ':') { it = InputType::EQUAL; }
										else if (c == ',') { it = InputType::COMMA; }
										else if ((c > ' ' && c < '0') || (c > '9' && c < 'A') || (c > 'Z' && c < 'a') || (c > 'z' && c < 'ü')) { it = InputType::SPECIAL; }
										else if (c >= 'A') { it = InputType::LETTER; }
										if (it == InputType::INVALID) { READ_JSON_ERR("Invalid char '" + std::to_string(c) + "'"); }
										else
										{
											if (curState == VAL_EXP || curState == VAL_FLO || curState == VAL_INT || curState == VAL_KEY || curState == VAL_STR || curState == EXP_VALUE_ARR || curState == EXP_KEY)
											{
												lvState = curState;
											}
											lState = curState;
											curState = g_machine[curState][static_cast<size_t>(it)];
											if (curState == State::INVALID) { READ_JSON_ERR("Invalid JSON State. lState = " + std::to_string(lState) + " input = " + std::to_string(static_cast<size_t>(it))); }
											else if (curState >= 100)//AUTO_* states
											{
												if (curState == AUTO_NXLEND) { curState = curVal == out ? State::FINISHED : State::EXP_LNEND; }
												else
												{
													if (eol == false)
													{
														switch (lvState)
														{
															case VAL_STR:
																curVal->set(r);
																break;
															case VAL_INT:
																try {
																	curVal->set(static_cast<int>(std::stoi(r)));
																}
																catch (...) { READ_JSON_ERR("Can't cast to int"); }
																break;
															case VAL_FLO:
															case VAL_EXP:
																try {
																	curVal->set(static_cast<float>(std::stof(r)));
																}
																catch (...) { READ_JSON_ERR("Can't cast to float"); }
																break;
															case VAL_KEY:
																if (r.compare("true") == 0) { curVal->set(true); }
																else if (r.compare("false") == 0) { curVal->set(false); }
																else if (r.compare("null") == 0) { curVal->set(nullptr); }
																else { READ_JSON_ERR("Unknown keyword '" + r + "'"); }
																break;
															default:
																break;
														}
														r = "";
													}
													eol = true;
													API::Files::IValue* root = curVal->getRoot();
													if (root != nullptr)
													{
														curVal = root;
														if (root->isObjekt() && curState == State::AUTO_NXTVAL) { curState = State::EXP_KEY; }
														else if (root->isObjekt() && curState == State::AUTO_BCKOBJ) { curState = root == out ? State::FINISHED : State::EXP_LNEND; }
														else if (root->isArray() && curState == State::AUTO_NXTVAL) { curState = State::EXP_VALUE_ARR; }
														else if (root->isArray() && curState == State::AUTO_BCKARR) { curState = root == out ? State::FINISHED : State::EXP_LNEND; }
														else { READ_JSON_ERR("Root can't be root"); }
													}
													else if (!read->IsAtEnd()) { READ_JSON_ERR("reached root too early!"); }
												}
											}
											else if (curState != State::EXP_LNEND) { eol = false; }
											if (readErr == false)
											{
												switch (curState)
												{
													case CloakEngine::Impl::Files::Configuration_v1::Parser::JSON::STR_BEGIN:
														if (lState == State::EXP_VALUE_ARR) { curVal = curVal->get(curVal->size()); }
														break;
													case CloakEngine::Impl::Files::Configuration_v1::Parser::JSON::VAL_FLO:
													case CloakEngine::Impl::Files::Configuration_v1::Parser::JSON::VAL_INT:
													case CloakEngine::Impl::Files::Configuration_v1::Parser::JSON::VAL_STR:
													case CloakEngine::Impl::Files::Configuration_v1::Parser::JSON::VAL_KEY:
													case CloakEngine::Impl::Files::Configuration_v1::Parser::JSON::KEY:
													case CloakEngine::Impl::Files::Configuration_v1::Parser::JSON::VAL_EXP:
													case CloakEngine::Impl::Files::Configuration_v1::Parser::JSON::VAL_EXP_B:
														if (lState == State::EXP_VALUE_ARR) { curVal = curVal->get(curVal->size()); }
														toString(&r, c);
														break;
													case CloakEngine::Impl::Files::Configuration_v1::Parser::JSON::EXP_EQUAL:
														if (lState != curState) { curVal = curVal->get(r); }
														r.clear();
														break;
													case CloakEngine::Impl::Files::Configuration_v1::Parser::JSON::EXP_KEY:
														if (lState == State::EXP_VALUE_ARR) { curVal = curVal->get(curVal->size()); }
														if (lState != curState) { curVal->setToObject(); }
														break;
													case CloakEngine::Impl::Files::Configuration_v1::Parser::JSON::EXP_VALUE_ARR:
														if (lState == State::EXP_VALUE_ARR && it != InputType::SPACE) { curVal = curVal->get(curVal->size()); }
														if (lState != curState) { curVal->setToArray(); }
														break;
													default:
														break;
												}
											}
										}
										c = 0;
									}
								}
								if (curState != State::FINISHED) { READ_JSON_ERR("Unexpected EOF (JSON)"); }
							}
							else { readErr = true; }
							return !readErr;
						}
						bool CLOAK_CALL readFile(In const API::Files::UFI& path, Out Value* out)
						{
							API::Files::IReader* read = nullptr;
							CREATE_INTERFACE(CE_QUERY_ARGS(&read));
							bool readErr = false;
							if (read->SetTarget(path, g_fileTypeJSON, true) > 0) { readErr = !ReadBuffer(read, out); }
							SAVE_RELEASE(read);
							return !readErr;
						}
						void CLOAK_CALL WriteBuffer(In API::Files::IWriter* write, In Value* root)
						{
							API::Stack<std::pair<API::Files::IValue*, size_t>> stack;
							stack.push(std::make_pair(root, 0));
							do {
								const size_t tc = stack.size() - 1;
								std::stringstream tabs;
								for (size_t a = 0; a < tc; a++) { tabs << "\t"; }
								const std::pair<API::Files::IValue*, size_t> w = stack.top();
								stack.pop();
								API::Files::IValue* r = nullptr;
								bool lir = stack.size() == 0;
								if (lir == false)
								{
									const std::pair<API::Files::IValue*, size_t> rw = stack.top();
									CLOAK_ASSUME(rw.first == w.first->getRoot());
									r = rw.first;
									API::Files::IValue* n = nullptr;
									lir = r->enumerateChildren(rw.second, &n) == false;
								}
								if (w.second == 0)
								{
									write->WriteString(tabs.str());
									if (r != nullptr && r->isObjekt()) { write->WriteString("\"" + w.first->getName() + "\" : "); }
									if (w.first->isArray()) { write->WriteString("["); }
									else if (w.first->isObjekt()) { write->WriteString("{"); }
									else
									{
										if (w.first->isInt()) { write->WriteString(std::to_string(w.first->toInt())); }
										else if (w.first->isFloat()) { write->WriteString(std::to_string(w.first->toFloat())); }
										else if (w.first->isBool()) { write->WriteString(w.first->toBool() ? "true" : "false"); }
										else if (w.first->isString()) { write->WriteString("\"" + w.first->toString() + "\""); }
										else { write->WriteString("null"); }
										if (lir == false) { write->WriteString(","); }
									}
									write->WriteString("\n");
								}
								API::Files::IValue* v = nullptr;
								if (w.first->enumerateChildren(w.second, &v))
								{
									stack.push(std::make_pair(w.first, w.second + 1));
									stack.push(std::make_pair(v, 0));
								}
								else if(w.first->isArray() || w.first->isObjekt())
								{
									if (w.first->isArray()) { write->WriteString(tabs.str() + "]"); }
									else { write->WriteString(tabs.str() + "}"); }
									if (lir == false) { write->WriteString(","); }
									write->WriteString("\n");
								}
							} while (stack.size() > 0);
						}
						void CLOAK_CALL writeFile(In const API::Files::UFI& path, In Value* root)
						{
							API::Files::IWriter* write = nullptr;
							CREATE_INTERFACE(CE_QUERY_ARGS(&write));
							write->SetTarget(path, g_fileTypeJSON, API::Files::CompressType::NONE, true);
							WriteBuffer(write, root);
							write->Save();
							SAVE_RELEASE(write);
						}
					}
					namespace XML {
						enum class InputType {
							SPACE = 0,
							TAG_OPEN = 1,
							TAG_CLOSE = 2,
							SLASH = 3,
							STRING = 4,
							ESCAPE_START = 5, //Escape looks like &amp; -> start = & and end = ;
							ESCAPE_END = 6,
							ASSIGN = 7,
							MINUS = 8,
							EX_MARK = 9,
							OTHER = 10,
							INVALID = 11,
						};
						enum State {
							INVALID = 0,
							LN_START = 1,
							TAG_START = 2,
							TAG_NAME = 3,
							TAG_END = 4,
							TAG_END_WAIT = 5,
							TAG_END_INLINE = 6,
							TAG_META_S = 7,
							TAG_META = 8,
							TAG_COMM_START = 9,
							TAG_COMM = 10,
							TAG_COMM_E1 = 11,
							TAG_COMM_E2 = 12,
							TEXT = 13,
							TEXT_ESCAPE = 14,
							ATTRB_START = 15,
							ATTRB_NAME = 16,
							ATTRB_NAME_END = 17,
							ATTRB_VALUE_START = 18,
							ATTRB_VALUE = 19,
							ATTRB_ESCAPE = 20,
						};
						constexpr size_t g_typeSize = static_cast<size_t>(InputType::INVALID);

						constexpr State g_machine[][g_typeSize] = {
							//							SPACE				TAG_OPEN	TAG_CLOSE	SLASH			STRING			ESCAPE_START	ESCAPE_END		ASSIGN				MINUS			EX_MARK			OTHER
							/*INVALID			*/	{	INVALID,			INVALID,	INVALID,	INVALID,		INVALID,		INVALID,		INVALID,		INVALID,			INVALID,		INVALID,		INVALID,		},
							/*LN_START			*/	{	LN_START,			TAG_START,	INVALID,	INVALID,		TEXT,			TEXT_ESCAPE,	TEXT,			TEXT,				TEXT,			TEXT,			TEXT,			},
							/*TAG_START			*/	{	INVALID,			INVALID,	INVALID,	TAG_END,		INVALID,		INVALID,		INVALID,		INVALID,			INVALID,		TAG_META,		TAG_NAME,		},
							/*TAG_NAME			*/	{	ATTRB_START,		INVALID,	LN_START,	TAG_END_INLINE,	INVALID,		INVALID,		INVALID,		INVALID,			INVALID,		INVALID,		TAG_NAME,		},
							/*TAG_END			*/	{	TAG_END_WAIT,		INVALID,	LN_START,	INVALID,		INVALID,		INVALID,		INVALID,		INVALID,			INVALID,		INVALID,		TAG_END,		},
							/*TAG_END_WAIT		*/	{	TAG_END_WAIT,		INVALID,	LN_START,	INVALID,		INVALID,		INVALID,		INVALID,		INVALID,			INVALID,		INVALID,		INVALID,		},
							/*TAG_END_INLINE	*/	{	TAG_END_INLINE,		INVALID,	LN_START,	INVALID,		INVALID,		INVALID,		INVALID,		INVALID,			INVALID,		INVALID,		INVALID,		},
							/*TAG_META_S		*/	{	TAG_META_S,			INVALID,	LN_START,	TAG_META,		TAG_META,		TAG_META,		TAG_META,		TAG_META,			TAG_COMM_START,	TAG_META,		TAG_META,		},
							/*TAG_META			*/	{	TAG_META,			INVALID,	LN_START,	TAG_META,		TAG_META,		TAG_META,		TAG_META,		TAG_META,			TAG_META,		TAG_META,		TAG_META,		},
							/*TAG_COMM_START	*/	{	TAG_META,			INVALID,	LN_START,	TAG_META,		TAG_META,		TAG_META,		TAG_META,		TAG_META,			TAG_COMM,		TAG_META,		TAG_META,		},
							/*TAG_COMM			*/	{	TAG_COMM,			TAG_COMM,	TAG_COMM,	TAG_COMM,		TAG_COMM,		TAG_COMM,		TAG_COMM,		TAG_COMM,			TAG_COMM_E1,	TAG_COMM,		TAG_COMM,		},
							/*TAG_COMM_E1		*/	{	TAG_COMM,			TAG_COMM,	TAG_COMM,	TAG_COMM,		TAG_COMM,		TAG_COMM,		TAG_COMM,		TAG_COMM,			TAG_COMM_E2,	TAG_COMM,		TAG_COMM,		},
							/*TAG_COMM_E2		*/	{	TAG_COMM,			TAG_COMM,	LN_START,	TAG_COMM,		TAG_COMM,		TAG_COMM,		TAG_COMM,		TAG_COMM,			TAG_COMM,		TAG_COMM,		TAG_COMM,		},
							/*TEXT				*/	{	TEXT,				TAG_START,	INVALID,	TEXT,			TEXT,			TEXT_ESCAPE,	TEXT,			TEXT,				TEXT,			TEXT,			TEXT,			},
							/*TEXT_ESCAPE		*/	{	INVALID,			INVALID,	INVALID,	INVALID,		INVALID,		INVALID,		TEXT,			INVALID,			INVALID,		INVALID,		TEXT_ESCAPE,	},
							/*ATTRB_START		*/	{	ATTRB_START,		INVALID,	LN_START,	TAG_END_INLINE,	INVALID,		INVALID,		INVALID,		INVALID,			INVALID,		INVALID,		ATTRB_NAME,		},
							/*ATTRB_NAME		*/	{	ATTRB_NAME_END,		INVALID,	INVALID,	INVALID,		INVALID,		INVALID,		INVALID,		ATTRB_VALUE_START,	INVALID,		INVALID,		ATTRB_NAME,		},
							/*ATTRB_NAME_END	*/	{	ATTRB_NAME_END,		INVALID,	LN_START,	TAG_END_INLINE,	INVALID,		INVALID,		INVALID,		ATTRB_VALUE_START,	INVALID,		INVALID,		ATTRB_NAME,		},
							/*ATTRB_VALUE_START	*/	{	ATTRB_VALUE_START,	INVALID,	INVALID,	INVALID,		ATTRB_VALUE,	INVALID,		INVALID,		INVALID,			INVALID,		INVALID,		INVALID,		},
							/*ATTRB_VALUE		*/	{	ATTRB_VALUE,		INVALID,	INVALID,	ATTRB_VALUE,	ATTRB_START,	ATTRB_ESCAPE,	ATTRB_VALUE,	INVALID,			ATTRB_VALUE,	ATTRB_VALUE,	ATTRB_VALUE,	},
							/*ATTRB_ESCAPE		*/	{	INVALID,			INVALID,	INVALID,	INVALID,		INVALID,		INVALID,		ATTRB_VALUE,	INVALID,			INVALID,		INVALID,		ATTRB_ESCAPE,	},
						};

						namespace EscapeSequences {
							struct EscapeChar {
								const char* Sequence;
								char32_t Char;
							};
							//Needed to split up sequences in multiple arrays, as IntelliSense would crash otherwise...
							constexpr EscapeChar Sequence1[] = {
								{"Tab",0x00009},					{"NewLine",0x0000A},
								{"excl",0x00021},					{"quot",0x00022},
								{"num",0x00023},					{"dollar",0x00024},
								{"percnt",0x00025},					{"amp",0x00026},
								{"apos",0x00027},					{"lpar",0x00028},
								{"rpar",0x00029},					{"ast",0x0002A},
								{"plus",0x0002B},					{"comma",0x0002C},
								{"period",0x0002E},					{"sol",0x0002F},
								{"colon",0x0003A},					{"semi",0x0003B},
								{"lt",0x0003C},						{"equals",0x0003D},
								{"gt",0x0003E},						{"quest",0x0003F},
								{"commat",0x00040},					{"lsqb",0x0005B},
								{"bsol",0x0005C},					{"rsqb",0x0005D},
								{"Hat",0x0005E},					{"lowbar",0x0005F},
								{"grave",0x00060},					{"lcub",0x0007B},
								{"verbar",0x0007C},					{"rcub",0x0007D},
								{"nbsp",0x000A0},					{"iexcl",0x000A1},
								{"cent",0x000A2},					{"pound",0x000A3},
								{"curren",0x000A4},					{"yen",0x000A5},
								{"brvbar",0x000A6},					{"sect",0x000A7},
								{"Dot",0x000A8},					{"copy",0x000A9},
								{"ordf",0x000AA},					{"laquo",0x000AB},
								{"not",0x000AC},					{"shy",0x000AD},
								{"reg",0x000AE},					{"macr",0x000AF},
								{"deg",0x000B0},					{"plusmn",0x000B1},
								{"sup2",0x000B2},					{"sup3",0x000B3},
								{"acute",0x000B4},					{"micro",0x000B5},
								{"para",0x000B6},					{"middot",0x000B7},
								{"cedil",0x000B8},					{"sup1",0x000B9},
								{"ordm",0x000BA},					{"raquo",0x000BB},
								{"frac14",0x000BC},					{"frac12",0x000BD},
								{"frac34",0x000BE},					{"iquest",0x000BF},
								{"Agrave",0x000C0},					{"Aacute",0x000C1},
								{"Acirc",0x000C2},					{"Atilde",0x000C3},
								{"Auml",0x000C4},					{"Aring",0x000C5},
								{"AElig",0x000C6},					{"Ccedil",0x000C7},
								{"Egrave",0x000C8},					{"Eacute",0x000C9},
								{"Ecirc",0x000CA},					{"Euml",0x000CB},
								{"Igrave",0x000CC},					{"Iacute",0x000CD},
								{"Icirc",0x000CE},					{"Iuml",0x000CF},
								{"ETH",0x000D0},					{"Ntilde",0x000D1},
								{"Ograve",0x000D2},					{"Oacute",0x000D3},
								{"Ocirc",0x000D4},					{"Otilde",0x000D5},
								{"Ouml",0x000D6},					{"times",0x000D7},
								{"Oslash",0x000D8},					{"Ugrave",0x000D9},
								{"Uacute",0x000DA},					{"Ucirc",0x000DB},
								{"Uuml",0x000DC},					{"Yacute",0x000DD},
								{"THORN",0x000DE},					{"szlig",0x000DF},
								{"agrave",0x000E0},					{"aacute",0x000E1},
								{"acirc",0x000E2},					{"atilde",0x000E3},
								{"auml",0x000E4},					{"aring",0x000E5},
								{"aelig",0x000E6},					{"ccedil",0x000E7},
								{"egrave",0x000E8},					{"eacute",0x000E9},
								{"ecirc",0x000EA},					{"euml",0x000EB},
								{"igrave",0x000EC},					{"iacute",0x000ED},
								{"icirc",0x000EE},					{"iuml",0x000EF},
								{"eth",0x000F0},					{"ntilde",0x000F1},
								{"ograve",0x000F2},					{"oacute",0x000F3},
								{"ocirc",0x000F4},					{"otilde",0x000F5},
								{"ouml",0x000F6},					{"divide",0x000F7},
								{"oslash",0x000F8},					{"ugrave",0x000F9},
								{"uacute",0x000FA},					{"ucirc",0x000FB},
								{"uuml",0x000FC},					{"yacute",0x000FD},
								{"thorn",0x000FE},					{"yuml",0x000FF},
								{"Amacr",0x00100},					{"amacr",0x00101},
								{"Abreve",0x00102},					{"abreve",0x00103},
								{"Aogon",0x00104},					{"aogon",0x00105},
								{"Cacute",0x00106},					{"cacute",0x00107},
								{"Ccirc",0x00108},					{"ccirc",0x00109},
								{"Cdot",0x0010A},					{"cdot",0x0010B},
								{"Ccaron",0x0010C},					{"ccaron",0x0010D},
								{"Dcaron",0x0010E},					{"dcaron",0x0010F},
								{"Dstrok",0x00110},					{"dstrok",0x00111},
								{"Emacr",0x00112},					{"emacr",0x00113},
								{"Edot",0x00116},					{"edot",0x00117},
								{"Eogon",0x00118},					{"eogon",0x00119},
								{"Ecaron",0x0011A},					{"ecaron",0x0011B},
								{"Gcirc",0x0011C},					{"gcirc",0x0011D},
								{"Gbreve",0x0011E},					{"gbreve",0x0011F},
								{"Gdot",0x00120},					{"gdot",0x00121},
								{"Gcedil",0x00122},					{"Hcirc",0x00124},
								{"hcirc",0x00125},					{"Hstrok",0x00126},
								{"hstrok",0x00127},					{"Itilde",0x00128},
								{"itilde",0x00129},					{"Imacr",0x0012A},
								{"imacr",0x0012B},					{"Iogon",0x0012E},
								{"iogon",0x0012F},					{"Idot",0x00130},
								{"imath",0x00131},					{"IJlig",0x00132},
								{"ijlig",0x00133},					{"Jcirc",0x00134},
								{"jcirc",0x00135},					{"Kcedil",0x00136},
								{"kcedil",0x00137},					{"kgreen",0x00138},
								{"Lacute",0x00139},					{"lacute",0x0013A},
								{"Lcedil",0x0013B},					{"lcedil",0x0013C},
								{"Lcaron",0x0013D},					{"lcaron",0x0013E},
								{"Lmidot",0x0013F},					{"lmidot",0x00140},
								{"Lstrok",0x00141},					{"lstrok",0x00142},
								{"Nacute",0x00143},					{"nacute",0x00144},
								{"Ncedil",0x00145},					{"ncedil",0x00146},
								{"Ncaron",0x00147},					{"ncaron",0x00148},
								{"napos",0x00149},					{"ENG",0x0014A},
								{"eng",0x0014B},					{"Omacr",0x0014C},
								{"omacr",0x0014D},					{"Odblac",0x00150},
								{"odblac",0x00151},					{"OElig",0x00152},
								{"oelig",0x00153},					{"Racute",0x00154},
								{"racute",0x00155},					{"Rcedil",0x00156},
								{"rcedil",0x00157},					{"Rcaron",0x00158},
								{"rcaron",0x00159},					{"Sacute",0x0015A},
								{"sacute",0x0015B},					{"Scirc",0x0015C},
								{"scirc",0x0015D},					{"Scedil",0x0015E},
								{"scedil",0x0015F},					{"Scaron",0x00160},
								{"scaron",0x00161},					{"Tcedil",0x00162},
								{"tcedil",0x00163},					{"Tcaron",0x00164},
								{"tcaron",0x00165},					{"Tstrok",0x00166},
								{"tstrok",0x00167},					{"Utilde",0x00168},
								{"utilde",0x00169},					{"Umacr",0x0016A},
								{"umacr",0x0016B},					{"Ubreve",0x0016C},
								{"ubreve",0x0016D},					{"Uring",0x0016E},
								{"uring",0x0016F},					{"Udblac",0x00170},
								{"udblac",0x00171},					{"Uogon",0x00172},
								{"uogon",0x00173},					{"Wcirc",0x00174},
								{"wcirc",0x00175},					{"Ycirc",0x00176},
								{"ycirc",0x00177},					{"Yuml",0x00178},
								{"Zacute",0x00179},					{"zacute",0x0017A},
								{"Zdot",0x0017B},					{"zdot",0x0017C},
								{"Zcaron",0x0017D},					{"zcaron",0x0017E},
								{"fnof",0x00192},					{"imped",0x001B5},
								{"gacute",0x001F5},					{"jmath",0x00237},
								{"circ",0x002C6},					{"caron",0x002C7},
								{"breve",0x002D8},					{"dot",0x002D9},
								{"ring",0x002DA},					{"ogon",0x002DB},
								{"tilde",0x002DC},					{"dblac",0x002DD},
								{"DownBreve",0x00311},				{"UnderBar",0x00332},
								{"Alpha",0x00391},					{"Beta",0x00392},
								{"Gamma",0x00393},					{"Delta",0x00394},
								{"Epsilon",0x00395},				{"Zeta",0x00396},
								{"Eta",0x00397},					{"Theta",0x00398},
								{"Iota",0x00399},					{"Kappa",0x0039A},
								{"Lambda",0x0039B},					{"Mu",0x0039C},
								{"Nu",0x0039D},						{"Xi",0x0039E},
								{"Omicron",0x0039F},				{"Pi",0x003A0},
								{"Rho",0x003A1},					{"Sigma",0x003A3},
								{"Tau",0x003A4},					{"Upsilon",0x003A5},
								{"Phi",0x003A6},					{"Chi",0x003A7},
								{"Psi",0x003A8},					{"Omega",0x003A9},
								{"alpha",0x003B1},					{"beta",0x003B2},
								{"gamma",0x003B3},					{"delta",0x003B4},
								{"epsiv",0x003B5},					{"zeta",0x003B6},
								{"eta",0x003B7},					{"theta",0x003B8},
								{"iota",0x003B9},					{"kappa",0x003BA},
								{"lambda",0x003BB},					{"mu",0x003BC},
								{"nu",0x003BD},						{"xi",0x003BE},
								{"omicron",0x003BF},				{"pi",0x003C0},
								{"rho",0x003C1},					{"sigmav",0x003C2},
								{"sigma",0x003C3},					{"tau",0x003C4},
								{"upsi",0x003C5},					{"phi",0x003C6},
								{"chi",0x003C7},					{"psi",0x003C8},
								{"omega",0x003C9},					{"thetav",0x003D1},
								{"Upsi",0x003D2},					{"straightphi",0x003D5},
								{"piv",0x003D6},					{"Gammad",0x003DC},
								{"gammad",0x003DD},					{"kappav",0x003F0},
								{"rhov",0x003F1},					{"epsi",0x003F5},
								{"bepsi",0x003F6},					{"IOcy",0x00401},
								{"DJcy",0x00402},					{"GJcy",0x00403},
								{"Jukcy",0x00404},					{"DScy",0x00405},
								{"Iukcy",0x00406},					{"YIcy",0x00407},
								{"Jsercy",0x00408},					{"LJcy",0x00409},
								{"NJcy",0x0040A},					{"TSHcy",0x0040B},
								{"KJcy",0x0040C},					{"Ubrcy",0x0040E},
								{"DZcy",0x0040F},					{"Acy",0x00410},
								{"Bcy",0x00411},					{"Vcy",0x00412},
								{"Gcy",0x00413},					{"Dcy",0x00414},
								{"IEcy",0x00415},					{"ZHcy",0x00416},
								{"Zcy",0x00417},					{"Icy",0x00418},
								{"Jcy",0x00419},					{"Kcy",0x0041A},
								{"Lcy",0x0041B},					{"Mcy",0x0041C},
								{"Ncy",0x0041D},					{"Ocy",0x0041E},
								{"Pcy",0x0041F},					{"Rcy",0x00420},
								{"Scy",0x00421},					{"Tcy",0x00422},
								{"Ucy",0x00423},					{"Fcy",0x00424},
								{"KHcy",0x00425},					{"TScy",0x00426},
								{"CHcy",0x00427},					{"SHcy",0x00428},
								{"SHCHcy",0x00429},					{"HARDcy",0x0042A},
								{"Ycy",0x0042B},					{"SOFTcy",0x0042C},
								{"Ecy",0x0042D},					{"YUcy",0x0042E},
								{"YAcy",0x0042F},					{"acy",0x00430},
								{"bcy",0x00431},					{"vcy",0x00432},
								{"gcy",0x00433},					{"dcy",0x00434},
								{"iecy",0x00435},					{"zhcy",0x00436},
								{"zcy",0x00437},					{"icy",0x00438},
								{"jcy",0x00439},					{"kcy",0x0043A},
								{"lcy",0x0043B},					{"mcy",0x0043C},
								{"ncy",0x0043D},					{"ocy",0x0043E},
								{"pcy",0x0043F},					{"rcy",0x00440},
								{"scy",0x00441},					{"tcy",0x00442},
								{"ucy",0x00443},					{"fcy",0x00444},
								{"khcy",0x00445},					{"tscy",0x00446},
								{"chcy",0x00447},					{"shcy",0x00448},
								{"shchcy",0x00449},					{"hardcy",0x0044A},
								{"ycy",0x0044B},					{"softcy",0x0044C},
							};
							constexpr EscapeChar Sequence2[] = {
								{"ecy",0x0044D},					{"yucy",0x0044E},
								{"yacy",0x0044F},					{"iocy",0x00451},
								{"djcy",0x00452},					{"gjcy",0x00453},
								{"jukcy",0x00454},					{"dscy",0x00455},
								{"iukcy",0x00456},					{"yicy",0x00457},
								{"jsercy",0x00458},					{"ljcy",0x00459},
								{"njcy",0x0045A},					{"tshcy",0x0045B},
								{"kjcy",0x0045C},					{"ubrcy",0x0045E},
								{"dzcy",0x0045F},					{"ensp",0x02002},
								{"emsp",0x02003},					{"emsp13",0x02004},
								{"emsp14",0x02005},					{"numsp",0x02007},
								{"puncsp",0x02008},					{"thinsp",0x02009},
								{"hairsp",0x0200A},					{"ZeroWidthSpace",0x0200B},
								{"zwnj",0x0200C},					{"zwj",0x0200D},
								{"lrm",0x0200E},					{"rlm",0x0200F},
								{"hyphen",0x02010},					{"ndash",0x02013},
								{"mdash",0x02014},					{"horbar",0x02015},
								{"Verbar",0x02016},					{"lsquo",0x02018},
								{"rsquo",0x02019},					{"lsquor",0x0201A},
								{"ldquo",0x0201C},					{"rdquo",0x0201D},
								{"ldquor",0x0201E},					{"dagger",0x02020},
								{"Dagger",0x02021},					{"bull",0x02022},
								{"nldr",0x02025},					{"hellip",0x02026},
								{"permil",0x02030},					{"pertenk",0x02031},
								{"prime",0x02032},					{"Prime",0x02033},
								{"tprime",0x02034},					{"bprime",0x02035},
								{"lsaquo",0x02039},					{"rsaquo",0x0203A},
								{"oline",0x0203E},					{"caret",0x02041},
								{"hybull",0x02043},					{"frasl",0x02044},
								{"bsemi",0x0204F},					{"qprime",0x02057},
								{"MediumSpace",0x0205F},			{"NoBreak",0x02060},
								{"ApplyFunction",0x02061},			{"InvisibleTimes",0x02062},
								{"InvisibleComma",0x02063},			{"euro",0x020AC},
								{"tdot",0x020DB},					{"DotDot",0x020DC},
								{"Copf",0x02102},					{"incare",0x02105},
								{"gscr",0x0210A},					{"hamilt",0x0210B},
								{"Hfr",0x0210C},					{"quaternions",0x0210D},
								{"planckh",0x0210E},				{"planck",0x0210F},
								{"Iscr",0x02110},					{"image",0x02111},
								{"Lscr",0x02112},					{"ell",0x02113},
								{"Nopf",0x02115},					{"numero",0x02116},
								{"copysr",0x02117},					{"weierp",0x02118},
								{"Popf",0x02119},					{"rationals",0x0211A},
								{"Rscr",0x0211B},					{"real",0x0211C},
								{"reals",0x0211D},					{"rx",0x0211E},
								{"trade",0x02122},					{"integers",0x02124},
								{"ohm",0x02126},					{"mho",0x02127},
								{"Zfr",0x02128},					{"iiota",0x02129},
								{"angst",0x0212B},					{"bernou",0x0212C},
								{"Cfr",0x0212D},					{"escr",0x0212F},
								{"Escr",0x02130},					{"Fscr",0x02131},
								{"phmmat",0x02133},					{"order",0x02134},
								{"alefsym",0x02135},				{"beth",0x02136},
								{"gimel",0x02137},					{"daleth",0x02138},
								{"CapitalDifferentialD",0x02145},	{"DifferentialD",0x02146},
								{"ExponentialE",0x02147},			{"ImaginaryI",0x02148},
								{"frac13",0x02153},					{"frac23",0x02154},
								{"frac15",0x02155},					{"frac25",0x02156},
								{"frac35",0x02157},					{"frac45",0x02158},
								{"frac16",0x02159},					{"frac56",0x0215A},
								{"frac18",0x0215B},					{"frac38",0x0215C},
								{"frac58",0x0215D},					{"frac78",0x0215E},
								{"larr",0x02190},					{"uarr",0x02191},
								{"rarr",0x02192},					{"darr",0x02193},
								{"harr",0x02194},					{"varr",0x02195},
								{"nwarr",0x02196},					{"nearr",0x02197},
								{"searr",0x02198},					{"swarr",0x02199},
								{"nlarr",0x0219A},					{"nrarr",0x0219B},
								{"rarrw",0x0219D},					{"Larr",0x0219E},
								{"Uarr",0x0219F},					{"Rarr",0x021A0},
								{"Darr",0x021A1},					{"larrtl",0x021A2},
								{"rarrtl",0x021A3},					{"LeftTeeArrow",0x021A4},
								{"UpTeeArrow",0x021A5},				{"map",0x021A6},
								{"DownTeeArrow",0x021A7},			{"larrhk",0x021A9},
								{"rarrhk",0x021AA},					{"larrlp",0x021AB},
								{"rarrlp",0x021AC},					{"harrw",0x021AD},
								{"nharr",0x021AE},					{"lsh",0x021B0},
								{"rsh",0x021B1},					{"ldsh",0x021B2},
								{"rdsh",0x021B3},					{"crarr",0x021B5},
								{"cularr",0x021B6},					{"curarr",0x021B7},
								{"olarr",0x021BA},					{"orarr",0x021BB},
								{"lharu",0x021BC},					{"lhard",0x021BD},
								{"uharr",0x021BE},					{"uharl",0x021BF},
								{"rharu",0x021C0},					{"rhard",0x021C1},
								{"dharr",0x021C2},					{"dharl",0x021C3},
								{"rlarr",0x021C4},					{"udarr",0x021C5},
								{"lrarr",0x021C6},					{"llarr",0x021C7},
								{"uuarr",0x021C8},					{"rrarr",0x021C9},
								{"ddarr",0x021CA},					{"lrhar",0x021CB},
								{"rlhar",0x021CC},					{"nlArr",0x021CD},
								{"nhArr",0x021CE},					{"nrArr",0x021CF},
								{"lArr",0x021D0},					{"uArr",0x021D1},
								{"rArr",0x021D2},					{"dArr",0x021D3},
								{"hArr",0x021D4},					{"vArr",0x021D5},
								{"nwArr",0x021D6},					{"neArr",0x021D7},
								{"seArr",0x021D8},					{"swArr",0x021D9},
								{"lAarr",0x021DA},					{"rAarr",0x021DB},
								{"zigrarr",0x021DD},				{"larrb",0x021E4},
								{"rarrb",0x021E5},					{"duarr",0x021F5},
								{"loarr",0x021FD},					{"roarr",0x021FE},
								{"hoarr",0x021FF},					{"forall",0x02200},
								{"comp",0x02201},					{"part",0x02202},
								{"exist",0x02203},					{"nexist",0x02204},
								{"empty",0x02205},					{"nabla",0x02207},
								{"isin",0x02208},					{"notin",0x02209},
								{"niv",0x0220B},					{"notni",0x0220C},
								{"prod",0x0220F},					{"coprod",0x02210},
								{"sum",0x02211},					{"minus",0x02212},
								{"mnplus",0x02213},					{"plusdo",0x02214},
								{"setmn",0x02216},					{"lowast",0x02217},
								{"compfn",0x02218},					{"radic",0x0221A},
								{"prop",0x0221D},					{"infin",0x0221E},
								{"angrt",0x0221F},					{"ang",0x02220},
								{"angmsd",0x02221},					{"angsph",0x02222},
								{"mid",0x02223},					{"nmid",0x02224},
								{"par",0x02225},					{"npar",0x02226},
								{"and",0x02227},					{"or",0x02228},
								{"cap",0x02229},					{"cup",0x0222A},
								{"int",0x0222B},					{"Int",0x0222C},
								{"tint",0x0222D},					{"conint",0x0222E},
								{"Conint",0x0222F},					{"Cconint",0x02230},
								{"cwint",0x02231},					{"cwconint",0x02232},
								{"awconint",0x02233},				{"there4",0x02234},
								{"becaus",0x02235},					{"ratio",0x02236},
								{"Colon",0x02237},					{"minusd",0x02238},
								{"mDDot",0x0223A},					{"homtht",0x0223B},
								{"sim",0x0223C},					{"bsim",0x0223D},
								{"ac",0x0223E},						{"acd",0x0223F},
								{"wreath",0x02240},					{"nsim",0x02241},
								{"esim",0x02242},					{"sime",0x02243},
								{"nsime",0x02244},					{"cong",0x02245},
								{"simne",0x02246},					{"ncong",0x02247},
								{"asymp",0x02248},					{"nap",0x02249},
								{"ape",0x0224A},					{"apid",0x0224B},
								{"bcong",0x0224C},					{"asympeq",0x0224D},
								{"bump",0x0224E},					{"bumpe",0x0224F},
								{"esdot",0x02250},					{"eDot",0x02251},
								{"efDot",0x02252},					{"erDot",0x02253},
								{"colone",0x02254},					{"ecolon",0x02255},
								{"ecir",0x02256},					{"cire",0x02257},
								{"wedgeq",0x02259},					{"veeeq",0x0225A},
								{"trie",0x0225C},					{"equest",0x0225F},
								{"ne",0x02260},						{"equiv",0x02261},
								{"nequiv",0x02262},					{"le",0x02264},
								{"ge",0x02265},						{"lE",0x02266},
								{"gE",0x02267},						{"lnE",0x02268},
								{"gnE",0x02269},					{"Lt",0x0226A},
								{"Gt",0x0226B},						{"twixt",0x0226C},
								{"NotCupCap",0x0226D},				{"nlt",0x0226E},
								{"ngt",0x0226F},					{"nle",0x02270},
								{"nge",0x02271},					{"lsim",0x02272},
								{"gsim",0x02273},					{"nlsim",0x02274},
								{"ngsim",0x02275},					{"lg",0x02276},
								{"gl",0x02277},						{"ntlg",0x02278},
								{"ntgl",0x02279},					{"pr",0x0227A},
								{"sc",0x0227B},						{"prcue",0x0227C},
								{"sccue",0x0227D},					{"prsim",0x0227E},
								{"scsim",0x0227F},					{"npr",0x02280},
								{"nsc",0x02281},					{"sub",0x02282},
								{"sup",0x02283},					{"nsub",0x02284},
								{"nsup",0x02285},					{"sube",0x02286},
								{"supe",0x02287},					{"nsube",0x02288},
								{"nsupe",0x02289},					{"subne",0x0228A},
								{"supne",0x0228B},					{"cupdot",0x0228D},
								{"uplus",0x0228E},					{"sqsub",0x0228F},
								{"sqsup",0x02290},					{"sqsube",0x02291},
								{"sqsupe",0x02292},					{"sqcap",0x02293},
								{"sqcup",0x02294},					{"oplus",0x02295},
								{"ominus",0x02296},					{"otimes",0x02297},
								{"osol",0x02298},					{"odot",0x02299},
								{"ocir",0x0229A},					{"oast",0x0229B},
								{"odash",0x0229D},					{"plusb",0x0229E},
								{"minusb",0x0229F},					{"timesb",0x022A0},
								{"sdotb",0x022A1},					{"vdash",0x022A2},
								{"dashv",0x022A3},					{"top",0x022A4},
								{"bottom",0x022A5},					{"models",0x022A7},
								{"vDash",0x022A8},					{"Vdash",0x022A9},
								{"Vvdash",0x022AA},					{"VDash",0x022AB},
								{"nvdash",0x022AC},					{"nvDash",0x022AD},
								{"nVdash",0x022AE},					{"nVDash",0x022AF},
								{"prurel",0x022B0},					{"vltri",0x022B2},
								{"vrtri",0x022B3},					{"ltrie",0x022B4},
								{"rtrie",0x022B5},					{"origof",0x022B6},
								{"imof",0x022B7},					{"mumap",0x022B8},
								{"hercon",0x022B9},					{"intcal",0x022BA},
								{"veebar",0x022BB},					{"barvee",0x022BD},
								{"angrtvb",0x022BE},				{"lrtri",0x022BF},
								{"xwedge",0x022C0},					{"xvee",0x022C1},
								{"xcap",0x022C2},					{"xcup",0x022C3},
								{"diam",0x022C4},					{"sdot",0x022C5},
								{"sstarf",0x022C6},					{"divonx",0x022C7},
								{"bowtie",0x022C8},					{"ltimes",0x022C9},
								{"rtimes",0x022CA},					{"lthree",0x022CB},
								{"rthree",0x022CC},					{"bsime",0x022CD},
								{"cuvee",0x022CE},					{"cuwed",0x022CF},
								{"Sub",0x022D0},					{"Sup",0x022D1},
								{"Cap",0x022D2},					{"Cup",0x022D3},
								{"fork",0x022D4},					{"epar",0x022D5},
								{"ltdot",0x022D6},					{"gtdot",0x022D7},
								{"Ll",0x022D8},						{"Gg",0x022D9},
								{"leg",0x022DA},					{"gel",0x022DB},
								{"cuepr",0x022DE},					{"cuesc",0x022DF},
								{"nprcue",0x022E0},					{"nsccue",0x022E1},
								{"nsqsube",0x022E2},				{"nsqsupe",0x022E3},
								{"lnsim",0x022E6},					{"gnsim",0x022E7},
								{"prnsim",0x022E8},					{"scnsim",0x022E9},
								{"nltri",0x022EA},					{"nrtri",0x022EB},
								{"nltrie",0x022EC},					{"nrtrie",0x022ED},
								{"vellip",0x022EE},					{"ctdot",0x022EF},
								{"utdot",0x022F0},					{"dtdot",0x022F1},
								{"disin",0x022F2},					{"isinsv",0x022F3},
								{"isins",0x022F4},					{"isindot",0x022F5},
								{"notinvc",0x022F6},				{"notinvb",0x022F7},
								{"isinE",0x022F9},					{"nisd",0x022FA},
								{"xnis",0x022FB},					{"nis",0x022FC},
								{"notnivc",0x022FD},				{"notnivb",0x022FE},
								{"barwed",0x02305},					{"Barwed",0x02306},
								{"lceil",0x02308},					{"rceil",0x02309},
								{"lfloor",0x0230A},					{"rfloor",0x0230B},
								{"drcrop",0x0230C},					{"dlcrop",0x0230D},
								{"urcrop",0x0230E},					{"ulcrop",0x0230F},
								{"bnot",0x02310},					{"profline",0x02312},
								{"profsurf",0x02313},				{"telrec",0x02315},
								{"target",0x02316},					{"ulcorn",0x0231C},
								{"urcorn",0x0231D},					{"dlcorn",0x0231E},
								{"drcorn",0x0231F},					{"frown",0x02322},
								{"smile",0x02323},					{"cylcty",0x0232D},
								{"profalar",0x0232E},				{"topbot",0x02336},
								{"ovbar",0x0233D},					{"solbar",0x0233F},
								{"angzarr",0x0237C},				{"lmoust",0x023B0},
								{"rmoust",0x023B1},					{"tbrk",0x023B4},
								{"bbrk",0x023B5},					{"bbrktbrk",0x023B6},
								{"OverParenthesis",0x023DC},		{"UnderParenthesis",0x023DD},
								{"OverBrace",0x023DE},				{"UnderBrace",0x023DF},
								{"trpezium",0x023E2},				{"elinters",0x023E7},
								{"blank",0x02423},					{"oS",0x024C8},
							};
							constexpr EscapeChar Sequence3[] = {
								{"boxh",0x02500},					{"boxv",0x02502},
								{"boxdr",0x0250C},					{"boxdl",0x02510},
								{"boxur",0x02514},					{"boxul",0x02518},
								{"boxvr",0x0251C},					{"boxvl",0x02524},
								{"boxhd",0x0252C},					{"boxhu",0x02534},
								{"boxvh",0x0253C},					{"boxH",0x02550},
								{"boxV",0x02551},					{"boxdR",0x02552},
								{"boxDr",0x02553},					{"boxDR",0x02554},
								{"boxdL",0x02555},					{"boxDl",0x02556},
								{"boxDL",0x02557},					{"boxuR",0x02558},
								{"boxUr",0x02559},					{"boxUR",0x0255A},
								{"boxuL",0x0255B},					{"boxUl",0x0255C},
								{"boxUL",0x0255D},					{"boxvR",0x0255E},
								{"boxVr",0x0255F},					{"boxVR",0x02560},
								{"boxvL",0x02561},					{"boxVl",0x02562},
								{"boxVL",0x02563},					{"boxHd",0x02564},
								{"boxhD",0x02565},					{"boxHD",0x02566},
								{"boxHu",0x02567},					{"boxhU",0x02568},
								{"boxHU",0x02569},					{"boxvH",0x0256A},
								{"boxVh",0x0256B},					{"boxVH",0x0256C},
								{"uhblk",0x02580},					{"lhblk",0x02584},
								{"block",0x02588},					{"blk14",0x02591},
								{"blk12",0x02592},					{"blk34",0x02593},
								{"squ",0x025A1},					{"squf",0x025AA},
								{"EmptyVerySmallSquare",0x025AB},	{"rect",0x025AD},
								{"marker",0x025AE},					{"fltns",0x025B1},
								{"xutri",0x025B3},					{"utrif",0x025B4},
								{"utri",0x025B5},					{"rtrif",0x025B8},
								{"rtri",0x025B9},					{"xdtri",0x025BD},
								{"dtrif",0x025BE},					{"dtri",0x025BF},
								{"ltrif",0x025C2},					{"ltri",0x025C3},
								{"loz",0x025CA},					{"cir",0x025CB},
								{"tridot",0x025EC},					{"xcirc",0x025EF},
								{"ultri",0x025F8},					{"urtri",0x025F9},
								{"lltri",0x025FA},					{"EmptySmallSquare",0x025FB},
								{"FilledSmallSquare",0x025FC},		{"starf",0x02605},
								{"star",0x02606},					{"phone",0x0260E},
								{"female",0x02640},					{"male",0x02642},
								{"spades",0x02660},					{"clubs",0x02663},
								{"hearts",0x02665},					{"diams",0x02666},
								{"sung",0x0266A},					{"flat",0x0266D},
								{"natur",0x0266E},					{"sharp",0x0266F},
								{"check",0x02713},					{"cross",0x02717},
								{"malt",0x02720},					{"sext",0x02736},
								{"VerticalSeparator",0x02758},		{"lbbrk",0x02772},
								{"rbbrk",0x02773},					{"lobrk",0x027E6},
								{"robrk",0x027E7},					{"lang",0x027E8},
								{"rang",0x027E9},					{"Lang",0x027EA},
								{"Rang",0x027EB},					{"loang",0x027EC},
								{"roang",0x027ED},					{"xlarr",0x027F5},
								{"xrarr",0x027F6},					{"xharr",0x027F7},
								{"xlArr",0x027F8},					{"xrArr",0x027F9},
								{"xhArr",0x027FA},					{"xmap",0x027FC},
								{"dzigrarr",0x027FF},				{"nvlArr",0x02902},
								{"nvrArr",0x02903},					{"nvHarr",0x02904},
								{"Map",0x02905},					{"lbarr",0x0290C},
								{"rbarr",0x0290D},					{"lBarr",0x0290E},
								{"rBarr",0x0290F},					{"RBarr",0x02910},
								{"DDotrahd",0x02911},				{"UpArrowBar",0x02912},
								{"DownArrowBar",0x02913},			{"Rarrtl",0x02916},
								{"latail",0x02919},					{"ratail",0x0291A},
								{"lAtail",0x0291B},					{"rAtail",0x0291C},
								{"larrfs",0x0291D},					{"rarrfs",0x0291E},
								{"larrbfs",0x0291F},				{"rarrbfs",0x02920},
								{"nwarhk",0x02923},					{"nearhk",0x02924},
								{"searhk",0x02925},					{"swarhk",0x02926},
								{"nwnear",0x02927},					{"nesear",0x02928},
								{"seswar",0x02929},					{"swnwar",0x0292A},
								{"rarrc",0x02933},					{"cudarrr",0x02935},
								{"ldca",0x02936},					{"rdca",0x02937},
								{"cudarrl",0x02938},				{"larrpl",0x02939},
								{"curarrm",0x0293C},				{"cularrp",0x0293D},
								{"rarrpl",0x02945},					{"harrcir",0x02948},
								{"Uarrocir",0x02949},				{"lurdshar",0x0294A},
								{"ldrushar",0x0294B},				{"LeftRightVector",0x0294E},
								{"RightUpDownVector",0x0294F},		{"DownLeftRightVector",0x02950},
								{"LeftUpDownVector",0x02951},		{"LeftVectorBar",0x02952},
								{"RightVectorBar",0x02953},			{"RightUpVectorBar",0x02954},
								{"RightDownVectorBar",0x02955},		{"DownLeftVectorBar",0x02956},
								{"DownRightVectorBar",0x02957},		{"LeftUpVectorBar",0x02958},
								{"LeftDownVectorBar",0x02959},		{"LeftTeeVector",0x0295A},
								{"RightTeeVector",0x0295B},			{"RightUpTeeVector",0x0295C},
								{"RightDownTeeVector",0x0295D},		{"DownLeftTeeVector",0x0295E},
								{"DownRightTeeVector",0x0295F},		{"LeftUpTeeVector",0x02960},
								{"LeftDownTeeVector",0x02961},		{"lHar",0x02962},
								{"uHar",0x02963},					{"rHar",0x02964},
								{"dHar",0x02965},					{"luruhar",0x02966},
								{"ldrdhar",0x02967},				{"ruluhar",0x02968},
								{"rdldhar",0x02969},				{"lharul",0x0296A},
								{"llhard",0x0296B},					{"rharul",0x0296C},
								{"lrhard",0x0296D},					{"udhar",0x0296E},
								{"duhar",0x0296F},					{"RoundImplies",0x02970},
								{"erarr",0x02971},					{"simrarr",0x02972},
								{"larrsim",0x02973},				{"rarrsim",0x02974},
								{"rarrap",0x02975},					{"ltlarr",0x02976},
								{"gtrarr",0x02978},					{"subrarr",0x02979},
								{"suplarr",0x0297B},				{"lfisht",0x0297C},
								{"rfisht",0x0297D},					{"ufisht",0x0297E},
								{"dfisht",0x0297F},					{"lopar",0x02985},
								{"ropar",0x02986},					{"lbrke",0x0298B},
								{"rbrke",0x0298C},					{"lbrkslu",0x0298D},
								{"rbrksld",0x0298E},				{"lbrksld",0x0298F},
								{"rbrkslu",0x02990},				{"langd",0x02991},
								{"rangd",0x02992},					{"lparlt",0x02993},
								{"rpargt",0x02994},					{"gtlPar",0x02995},
								{"ltrPar",0x02996},					{"vzigzag",0x0299A},
								{"vangrt",0x0299C},					{"angrtvbd",0x0299D},
								{"ange",0x029A4},					{"range",0x029A5},
								{"dwangle",0x029A6},				{"uwangle",0x029A7},
								{"angmsdaa",0x029A8},				{"angmsdab",0x029A9},
								{"angmsdac",0x029AA},				{"angmsdad",0x029AB},
								{"angmsdae",0x029AC},				{"angmsdaf",0x029AD},
								{"angmsdag",0x029AE},				{"angmsdah",0x029AF},
								{"bemptyv",0x029B0},				{"demptyv",0x029B1},
								{"cemptyv",0x029B2},				{"raemptyv",0x029B3},
								{"laemptyv",0x029B4},				{"ohbar",0x029B5},
								{"omid",0x029B6},					{"opar",0x029B7},
								{"operp",0x029B9},					{"olcross",0x029BB},
								{"odsold",0x029BC},					{"olcir",0x029BE},
								{"ofcir",0x029BF},					{"olt",0x029C0},
								{"ogt",0x029C1},					{"cirscir",0x029C2},
								{"cirE",0x029C3},					{"solb",0x029C4},
								{"bsolb",0x029C5},					{"boxbox",0x029C9},
								{"trisb",0x029CD},					{"rtriltri",0x029CE},
								{"LeftTriangleBar",0x029CF},		{"RightTriangleBar",0x029D0},
								{"race",0x029DA},					{"iinfin",0x029DC},
								{"infintie",0x029DD},				{"nvinfin",0x029DE},
								{"eparsl",0x029E3},					{"smeparsl",0x029E4},
								{"eqvparsl",0x029E5},				{"lozf",0x029EB},
								{"RuleDelayed",0x029F4},			{"dsol",0x029F6},
								{"xodot",0x02A00},					{"xoplus",0x02A01},
								{"xotime",0x02A02},					{"xuplus",0x02A04},
								{"xsqcup",0x02A06},					{"qint",0x02A0C},
								{"fpartint",0x02A0D},				{"cirfnint",0x02A10},
								{"awint",0x02A11},					{"rppolint",0x02A12},
								{"scpolint",0x02A13},				{"npolint",0x02A14},
								{"pointint",0x02A15},				{"quatint",0x02A16},
								{"intlarhk",0x02A17},				{"pluscir",0x02A22},
								{"plusacir",0x02A23},				{"simplus",0x02A24},
								{"plusdu",0x02A25},					{"plussim",0x02A26},
								{"plustwo",0x02A27},				{"mcomma",0x02A29},
								{"minusdu",0x02A2A},				{"loplus",0x02A2D},
								{"roplus",0x02A2E},					{"Cross",0x02A2F},
								{"timesd",0x02A30},					{"timesbar",0x02A31},
								{"smashp",0x02A33},					{"lotimes",0x02A34},
								{"rotimes",0x02A35},				{"otimesas",0x02A36},
								{"Otimes",0x02A37},					{"odiv",0x02A38},
								{"triplus",0x02A39},				{"triminus",0x02A3A},
								{"tritime",0x02A3B},				{"iprod",0x02A3C},
								{"amalg",0x02A3F},					{"capdot",0x02A40},
								{"ncup",0x02A42},					{"ncap",0x02A43},
								{"capand",0x02A44},					{"cupor",0x02A45},
								{"cupcap",0x02A46},					{"capcup",0x02A47},
								{"cupbrcap",0x02A48},				{"capbrcup",0x02A49},
								{"cupcup",0x02A4A},					{"capcap",0x02A4B},
								{"ccups",0x02A4C},					{"ccaps",0x02A4D},
								{"ccupssm",0x02A50},				{"And",0x02A53},
								{"Or",0x02A54},						{"andand",0x02A55},
								{"oror",0x02A56},					{"orslope",0x02A57},
								{"andslope",0x02A58},				{"andv",0x02A5A},
								{"orv",0x02A5B},					{"andd",0x02A5C},
								{"ord",0x02A5D},					{"wedbar",0x02A5F},
								{"sdote",0x02A66},					{"simdot",0x02A6A},
								{"congdot",0x02A6D},				{"easter",0x02A6E},
								{"apacir",0x02A6F},					{"apE",0x02A70},
								{"eplus",0x02A71},					{"pluse",0x02A72},
								{"Esim",0x02A73},					{"Colone",0x02A74},
								{"Equal",0x02A75},					{"eDDot",0x02A77},
								{"equivDD",0x02A78},				{"ltcir",0x02A79},
								{"gtcir",0x02A7A},					{"ltquest",0x02A7B},
								{"gtquest",0x02A7C},				{"les",0x02A7D},
								{"ges",0x02A7E},					{"lesdot",0x02A7F},
								{"gesdot",0x02A80},					{"lesdoto",0x02A81},
								{"gesdoto",0x02A82},				{"lesdotor",0x02A83},
								{"gesdotol",0x02A84},				{"lap",0x02A85},
								{"gap",0x02A86},					{"lne",0x02A87},
								{"gne",0x02A88},					{"lnap",0x02A89},
								{"gnap",0x02A8A},					{"lEg",0x02A8B},
								{"gEl",0x02A8C},					{"lsime",0x02A8D},
								{"gsime",0x02A8E},					{"lsimg",0x02A8F},
								{"gsiml",0x02A90},					{"lgE",0x02A91},
								{"glE",0x02A92},					{"lesges",0x02A93},
								{"gesles",0x02A94},					{"els",0x02A95},
								{"egs",0x02A96},					{"elsdot",0x02A97},
								{"egsdot",0x02A98},					{"el",0x02A99},
								{"eg",0x02A9A},						{"siml",0x02A9D},
								{"simg",0x02A9E},					{"simlE",0x02A9F},
								{"simgE",0x02AA0},					{"LessLess",0x02AA1},
								{"GreaterGreater",0x02AA2},			{"glj",0x02AA4},
								{"gla",0x02AA5},					{"ltcc",0x02AA6},
								{"gtcc",0x02AA7},					{"lescc",0x02AA8},
								{"gescc",0x02AA9},					{"smt",0x02AAA},
								{"lat",0x02AAB},					{"smte",0x02AAC},
								{"late",0x02AAD},					{"bumpE",0x02AAE},
								{"pre",0x02AAF},					{"sce",0x02AB0},
								{"prE",0x02AB3},					{"scE",0x02AB4},
								{"prnE",0x02AB5},					{"scnE",0x02AB6},
								{"prap",0x02AB7},					{"scap",0x02AB8},
								{"prnap",0x02AB9},					{"scnap",0x02ABA},
								{"Pr",0x02ABB},						{"Sc",0x02ABC},
								{"subdot",0x02ABD},					{"supdot",0x02ABE},
								{"subplus",0x02ABF},				{"supplus",0x02AC0},
								{"submult",0x02AC1},				{"supmult",0x02AC2},
								{"subedot",0x02AC3},				{"supedot",0x02AC4},
								{"subE",0x02AC5},					{"supE",0x02AC6},
								{"subsim",0x02AC7},					{"supsim",0x02AC8},
								{"subnE",0x02ACB},					{"supnE",0x02ACC},
								{"csub",0x02ACF},					{"csup",0x02AD0},
								{"csube",0x02AD1},					{"csupe",0x02AD2},
								{"subsup",0x02AD3},					{"supsub",0x02AD4},
								{"subsub",0x02AD5},					{"supsup",0x02AD6},
								{"suphsub",0x02AD7},				{"supdsub",0x02AD8},
								{"forkv",0x02AD9},					{"topfork",0x02ADA},
								{"mlcp",0x02ADB},					{"Dashv",0x02AE4},
								{"Vdashl",0x02AE6},					{"Barv",0x02AE7},
								{"vBar",0x02AE8},					{"vBarv",0x02AE9},
								{"Vbar",0x02AEB},					{"Not",0x02AEC},
								{"bNot",0x02AED},					{"rnmid",0x02AEE},
								{"cirmid",0x02AEF},					{"midcir",0x02AF0},
								{"topcir",0x02AF1},					{"nhpar",0x02AF2},
								{"parsim",0x02AF3},					{"parsl",0x02AFD},
								{"fflig",0x0FB00},					{"filig",0x0FB01},
								{"fllig",0x0FB02},					{"ffilig",0x0FB03},
								{"ffllig",0x0FB04},					{"Ascr",0x1D49C},
								{"Cscr",0x1D49E},					{"Dscr",0x1D49F},
								{"Gscr",0x1D4A2},					{"Jscr",0x1D4A5},
								{"Kscr",0x1D4A6},					{"Nscr",0x1D4A9},
								{"Oscr",0x1D4AA},					{"Pscr",0x1D4AB},
								{"Qscr",0x1D4AC},					{"Sscr",0x1D4AE},
								{"Tscr",0x1D4AF},					{"Uscr",0x1D4B0},
								{"Vscr",0x1D4B1},					{"Wscr",0x1D4B2},
								{"Xscr",0x1D4B3},					{"Yscr",0x1D4B4},
								{"Zscr",0x1D4B5},					{"ascr",0x1D4B6},
								{"bscr",0x1D4B7},					{"cscr",0x1D4B8},
								{"dscr",0x1D4B9},					{"fscr",0x1D4BB},
								{"hscr",0x1D4BD},					{"iscr",0x1D4BE},
								{"jscr",0x1D4BF},					{"kscr",0x1D4C0},
								{"lscr",0x1D4C1},					{"mscr",0x1D4C2},
								{"nscr",0x1D4C3},					{"pscr",0x1D4C5},
								{"qscr",0x1D4C6},					{"rscr",0x1D4C7},
								{"sscr",0x1D4C8},					{"tscr",0x1D4C9},
								{"uscr",0x1D4CA},					{"vscr",0x1D4CB},
								{"wscr",0x1D4CC},					{"xscr",0x1D4CD},
								{"yscr",0x1D4CE},					{"zscr",0x1D4CF},
								{"Afr",0x1D504},					{"Bfr",0x1D505},
								{"Dfr",0x1D507},					{"Efr",0x1D508},
								{"Ffr",0x1D509},					{"Gfr",0x1D50A},
								{"Jfr",0x1D50D},					{"Kfr",0x1D50E},
								{"Lfr",0x1D50F},					{"Mfr",0x1D510},
								{"Nfr",0x1D511},					{"Ofr",0x1D512},
								{"Pfr",0x1D513},					{"Qfr",0x1D514},
								{"Sfr",0x1D516},					{"Tfr",0x1D517},
								{"Ufr",0x1D518},					{"Vfr",0x1D519},
								{"Wfr",0x1D51A},					{"Xfr",0x1D51B},
								{"Yfr",0x1D51C},					{"afr",0x1D51E},
								{"bfr",0x1D51F},					{"cfr",0x1D520},
								{"dfr",0x1D521},					{"efr",0x1D522},
								{"ffr",0x1D523},					{"gfr",0x1D524},
								{"hfr",0x1D525},					{"ifr",0x1D526},
								{"jfr",0x1D527},					{"kfr",0x1D528},
								{"lfr",0x1D529},					{"mfr",0x1D52A},
								{"nfr",0x1D52B},					{"ofr",0x1D52C},
								{"pfr",0x1D52D},					{"qfr",0x1D52E},
								{"rfr",0x1D52F},					{"sfr",0x1D530},
								{"tfr",0x1D531},					{"ufr",0x1D532},
								{"vfr",0x1D533},					{"wfr",0x1D534},
								{"xfr",0x1D535},					{"yfr",0x1D536},
								{"zfr",0x1D537},					{"Aopf",0x1D538},
								{"Bopf",0x1D539},					{"Dopf",0x1D53B},
								{"Eopf",0x1D53C},					{"Fopf",0x1D53D},
								{"Gopf",0x1D53E},					{"Iopf",0x1D540},
								{"Jopf",0x1D541},					{"Kopf",0x1D542},
								{"Lopf",0x1D543},					{"Mopf",0x1D544},
								{"Oopf",0x1D546},					{"Sopf",0x1D54A},
								{"Topf",0x1D54B},					{"Uopf",0x1D54C},
								{"Vopf",0x1D54D},					{"Wopf",0x1D54E},
								{"Xopf",0x1D54F},					{"Yopf",0x1D550},
								{"aopf",0x1D552},					{"bopf",0x1D553},
								{"copf",0x1D554},					{"dopf",0x1D555},
								{"eopf",0x1D556},					{"fopf",0x1D557},
								{"gopf",0x1D558},					{"hopf",0x1D559},
								{"iopf",0x1D55A},					{"jopf",0x1D55B},
								{"kopf",0x1D55C},					{"lopf",0x1D55D},
								{"mopf",0x1D55E},					{"nopf",0x1D55F},
								{"oopf",0x1D560},					{"popf",0x1D561},
								{"qopf",0x1D562},					{"ropf",0x1D563},
								{"sopf",0x1D564},					{"topf",0x1D565},
								{"uopf",0x1D566},					{"vopf",0x1D567},
								{"wopf",0x1D568},					{"xopf",0x1D569},
								{"yopf",0x1D56A},					{"zopf",0x1D56B},
							};
						}

						bool CLOAK_CALL ReadBuffer(In API::Files::IReader* read, Out Value* out)
						{
							bool readErr = false;
							if (read != nullptr && out != nullptr)
							{
								State curState = LN_START;
								State lstate = curState;
								bool doBreak = false;
								char32_t c;
								API::Files::IValue* curV = out;
								API::Files::IValue* attrb = nullptr;
								API::Files::IValue* av = nullptr;
								std::string r = "";
								std::string ar = "";
								while (doBreak == false && readErr == false)
								{
									if (!read->IsAtEnd())
									{
										c = static_cast<char32_t>(read->ReadBits(32));
										if (readErr == false)
										{
											InputType it = InputType::INVALID;
											if (c == ' ' || c == '\n' || c == '\r' || c == '\t') { it = InputType::SPACE; }
											else if (c == '<') { it = InputType::TAG_OPEN; }
											else if (c == '>') { it = InputType::TAG_CLOSE; }
											else if (c == '/') { it = InputType::SLASH; }
											else if (c == '\"' || c == '\'') { it = InputType::STRING; }
											else if (c == '&') { it = InputType::ESCAPE_START; }
											else if (c == ';') { it = InputType::ESCAPE_END; }
											else if (c == '=') { it = InputType::ASSIGN; }
											else if (c == '-') { it = InputType::MINUS; }
											else if (c == '!') { it = InputType::EX_MARK; }
											else { it = InputType::OTHER; }

											lstate = curState;
											curState = g_machine[curState][static_cast<size_t>(it)];

											if (curState == INVALID) { readErr = true; }
											else if (lstate != curState)
											{
												switch (lstate)
												{
													case TAG_NAME:
														curV = curV->get(curV->size());
														curV->get("Name")->set(r);
														attrb = curV->get("Attributes");
														curV = curV->get("Children");
														r = "";
														break;
													case TAG_END:
														if (curV == out) { readErr = true; }
														else
														{
															curV = curV->getRoot();
															if (r.compare(curV->get("Name")->toString()) != 0) { readErr = true; }
															else 
															{ 
																curV = curV->getRoot(); 
																if (curV == out) { doBreak = true; }
															}
														}
														r = "";
														break;
													case ATTRB_NAME:
														CLOAK_ASSUME(attrb != nullptr);
														av = attrb->get(r);
														av->set(true);
														r = "";
														break;
													case ATTRB_VALUE:
														CLOAK_ASSUME(av != nullptr);
														if (curState != ATTRB_ESCAPE) { av->set(r); r = ""; }
														break;
													case ATTRB_ESCAPE:
													case TEXT_ESCAPE:
														if (ar[0] == '#')
														{
															if (ar[1] == 'x')
															{
																try {
																	char32_t xc = static_cast<char32_t>(std::stoi(ar.substr(2), nullptr, 16));
																	toString(&r, xc);
																}
																catch (...)
																{
																	r += "&" + ar + ";";
																}
															}
															else
															{
																try {
																	char32_t xc = static_cast<char32_t>(std::stoi(ar.substr(1)));
																	toString(&r, xc);
																}
																catch (...)
																{
																	r += "&" + ar + ";";
																}
															}
														}
														else
														{
															for (size_t a = 0; a < ARRAYSIZE(EscapeSequences::Sequence1); a++)
															{
																if (ar.compare(EscapeSequences::Sequence1[a].Sequence) == 0) { toString(&r, EscapeSequences::Sequence1[a].Char); goto xml_found_esc_seq; }
															}
															for (size_t a = 0; a < ARRAYSIZE(EscapeSequences::Sequence2); a++)
															{
																if (ar.compare(EscapeSequences::Sequence2[a].Sequence) == 0) { toString(&r, EscapeSequences::Sequence2[a].Char); goto xml_found_esc_seq; }
															}
															for (size_t a = 0; a < ARRAYSIZE(EscapeSequences::Sequence3); a++)
															{
																if (ar.compare(EscapeSequences::Sequence3[a].Sequence) == 0) { toString(&r, EscapeSequences::Sequence3[a].Char); goto xml_found_esc_seq; }
															}
															r += "&" + ar + ";";

														xml_found_esc_seq:;

														}
														ar = "";
														break;
													case TEXT:
														curV->get(curV->size())->set(r);
														r = "";
														break;
												}
											}
											switch (curState)
											{
												case TEXT:
												case TAG_NAME:
												case ATTRB_NAME:
													toString(&r, c);
													break;
												case TEXT_ESCAPE:
												case ATTRB_ESCAPE:
													if (lstate == curState) { toString(&ar, c); }
												case ATTRB_VALUE:
												case TAG_END:
													if (lstate == curState) { toString(&r, c); }
													break;
												default:
													break;
											}
										}
									}
									else { doBreak = true; }
								}
								if (curState != LN_START || curV != out) { readErr = true; }
							}
							else { readErr = true; }
							return !readErr;
						}
						bool CLOAK_CALL readFile(In const API::Files::UFI& path, Out Value* out)
						{
							API::Files::IReader* read = nullptr;
							CREATE_INTERFACE(CE_QUERY_ARGS(&read));
							bool readErr = false;
							if (read->SetTarget(path, g_fileTypeXML, true) > 0) { readErr = !ReadBuffer(read, out); }
							SAVE_RELEASE(read);
							return !readErr;
						}
					}
				}

				CLOAK_CALL Value::Value(In Configuration* parent, In_opt Value* root, In_opt std::string name) : m_parent(parent), m_name(name), m_root(root)
				{
					m_type = Type::UNDEF;
					m_used = false;
					m_forceUsed = false;
				}
				CLOAK_CALL Value::~Value()
				{
					clear();
				}

				std::string CLOAK_CALL_THIS Value::toString(In_opt std::string def)
				{
					API::Helper::Lock lock(m_parent->m_sync);
					if (m_type == Type::UNDEF) { set(def); }
					m_used = true;
					if (m_type == Type::STRING) { return m_valStr; }
					return def;
				}
				bool CLOAK_CALL_THIS Value::toBool(In_opt bool def)
				{
					API::Helper::Lock lock(m_parent->m_sync);
					if (m_type == Type::UNDEF) { set(def); }
					m_used = true;
					if (m_type == Type::BOOL) { return m_valBool; }
					else if (m_type == Type::INT) 
					{
						if (m_valInt == 0) { return false; }
						else if (m_valInt == 1) { return true; }
					}
					else if (m_type == Type::STRING)
					{
						if (m_valStr.compare("true") == 0 || m_valStr.compare("yes") == 0) { return true; }
						else if (m_valStr.compare("false") == 0 || m_valStr.compare("no") == 0) { return false; }
					}
					return def;
				}
				int CLOAK_CALL_THIS Value::toInt(In_opt int def)
				{
					API::Helper::Lock lock(m_parent->m_sync);
					if (m_type == Type::UNDEF) { set(def); }
					m_used = true;
					if (m_type == Type::INT) { return m_valInt; }
					return def;
				}
				float CLOAK_CALL_THIS Value::toFloat(In_opt float def)
				{
					API::Helper::Lock lock(m_parent->m_sync);
					if (m_type == Type::UNDEF) { set(def); }
					m_used = true;
					if (m_type == Type::FLOAT) { return m_valFloat; }
					return def;
				}

				API::Files::Configuration_v1::IValue* CLOAK_CALL_THIS Value::get(In const std::string& name)
				{
					API::Helper::Lock lock(m_parent->m_sync);
					m_used = true;
					changeType(Type::OBJ);
					for (size_t a = 0; a < m_childs.size(); a++)
					{
						Value* v = m_childs[a];
						if (v->m_name.compare(name) == 0) { return v; }
					}
					m_childs.push_back(new Value(m_parent, this, name));
					m_parent->m_changed = true;
					return m_childs[m_childs.size() - 1];
				}
				API::Files::Configuration_v1::IValue* CLOAK_CALL_THIS Value::get(In size_t index)
				{
					API::Helper::Lock lock(m_parent->m_sync);
					m_used = true;
					changeType(Type::ARRAY);
					while (index >= m_childs.size()) 
					{ 
						m_childs.push_back(new Value(m_parent, this));
						m_parent->m_changed = true;
					}
					return m_childs[index];
				}
				API::Files::Configuration_v1::IValue* CLOAK_CALL_THIS Value::getRoot() 
				{
					API::Helper::Lock lock(m_parent->m_sync);
					return m_root; 
				}
				Success(return==true) bool CLOAK_CALL_THIS Value::enumerateChildren(In size_t i, Out IValue** res)
				{
					API::Helper::Lock lock(m_parent->m_sync);
					if (res != nullptr && (m_type == Type::OBJ || m_type == Type::ARRAY) && i < m_childs.size())
					{
						m_used = true;
						*res = m_childs[i];
						return true;
					}
					return false;
				}
				bool CLOAK_CALL_THIS Value::has(In const std::string& name) const
				{
					API::Helper::Lock lock(m_parent->m_sync);
					if (m_type == Type::OBJ)
					{
						for (size_t a = 0; a < m_childs.size(); a++) { if (m_childs[a]->m_name.compare(name) == 0) { return true; } }
					}
					return false;
				}
				bool CLOAK_CALL_THIS Value::has(In unsigned int index) const
				{
					API::Helper::Lock lock(m_parent->m_sync);
					if (m_type == Type::ARRAY) { return index < m_childs.size(); }
					return false;
				}

				bool CLOAK_CALL_THIS Value::is(In const std::string& s) const
				{
					API::Helper::Lock lock(m_parent->m_sync);
					return m_type == Type::STRING && m_valStr.compare(s) == 0;
				}
				bool CLOAK_CALL_THIS Value::is(In bool s) const
				{
					API::Helper::Lock lock(m_parent->m_sync);
					return (m_type == Type::BOOL && m_valBool == s) || (m_type == Type::INT && ((m_valInt == 0 && s == false) || (m_valInt == 1 && s == true)));
				}
				bool CLOAK_CALL_THIS Value::is(In int s) const
				{
					API::Helper::Lock lock(m_parent->m_sync);
					return m_type == Type::INT && m_valInt == s;
				}
				bool CLOAK_CALL_THIS Value::is(In float s) const
				{
					API::Helper::Lock lock(m_parent->m_sync);
					return m_type == Type::FLOAT && m_valFloat == s;
				}
				bool CLOAK_CALL_THIS Value::is(In std::nullptr_t s) const
				{
					API::Helper::Lock lock(m_parent->m_sync);
					return m_type == Type::UNDEF;
				}
				bool CLOAK_CALL_THIS Value::isObjekt() const
				{
					API::Helper::Lock lock(m_parent->m_sync);
					return m_type == Type::OBJ;
				}
				bool CLOAK_CALL_THIS Value::isArray() const
				{
					API::Helper::Lock lock(m_parent->m_sync);
					return m_type == Type::ARRAY;
				}
				bool CLOAK_CALL_THIS Value::isString() const
				{
					API::Helper::Lock lock(m_parent->m_sync);
					return m_type == Type::STRING;
				}
				bool CLOAK_CALL_THIS Value::isBool() const
				{
					API::Helper::Lock lock(m_parent->m_sync);
					return m_type == Type::BOOL || (m_type == Type::INT && (m_valInt == 0 || m_valInt == 1)) || (m_type == Type::STRING && (m_valStr.compare("true") == 0 || m_valStr.compare("false") == 0 || m_valStr.compare("yes") == 0 || m_valStr.compare("no") == 0));
				}
				bool CLOAK_CALL_THIS Value::isInt() const
				{
					API::Helper::Lock lock(m_parent->m_sync);
					return m_type == Type::INT;
				}
				bool CLOAK_CALL_THIS Value::isFloat() const
				{
					API::Helper::Lock lock(m_parent->m_sync);
					return m_type == Type::FLOAT;
				}

				size_t CLOAK_CALL_THIS Value::size() const
				{
					API::Helper::Lock lock(m_parent->m_sync);
					if (m_type == Type::ARRAY) { return m_childs.size(); }
					return 0;
				}

				void CLOAK_CALL_THIS Value::set(In const std::string& s)
				{
					API::Helper::Lock lock(m_parent->m_sync);
					m_used = true;
					m_parent->m_changed = true;
					changeType(Type::STRING);
					m_valStr = s;
				}
				void CLOAK_CALL_THIS Value::set(In bool s)
				{
					API::Helper::Lock lock(m_parent->m_sync);
					m_used = true;
					m_parent->m_changed = true;
					changeType(Type::BOOL);
					m_valBool = s;
				}
				void CLOAK_CALL_THIS Value::set(In int s)
				{
					API::Helper::Lock lock(m_parent->m_sync);
					m_used = true;
					m_parent->m_changed = true;
					changeType(Type::INT);
					m_valInt = s;
				}
				void CLOAK_CALL_THIS Value::set(In float s)
				{
					API::Helper::Lock lock(m_parent->m_sync);
					m_used = true;
					m_parent->m_changed = true;
					changeType(Type::FLOAT);
					m_valFloat = s;
				}
				void CLOAK_CALL_THIS Value::set(In std::nullptr_t s)
				{
					API::Helper::Lock lock(m_parent->m_sync);
					m_used = true;
					m_parent->m_changed = true;
					changeType(Type::UNDEF);
				}
				void CLOAK_CALL_THIS Value::setToArray()
				{
					API::Helper::Lock lock(m_parent->m_sync);
					changeType(Type::ARRAY);
					m_used = true;
				}
				void CLOAK_CALL_THIS Value::setToObject()
				{
					API::Helper::Lock lock(m_parent->m_sync);
					changeType(Type::OBJ);
					m_used = true;
				}

				void CLOAK_CALL_THIS Value::forceUsed()
				{
					API::Helper::Lock lock(m_parent->m_sync);
					m_used = true;
					m_forceUsed = true;
				}

				Value* CLOAK_CALL_THIS Value::getChild(In size_t index)
				{
					API::Helper::Lock lock(m_parent->m_sync);
					if ((m_type == Type::OBJ || m_type == Type::ARRAY) && index < m_childs.size()) { return m_childs[index]; }
					return nullptr;
				}
				size_t CLOAK_CALL_THIS Value::getChildCount() const
				{
					API::Helper::Lock lock(m_parent->m_sync);
					if (m_type == Type::OBJ || m_type == Type::ARRAY) { return m_childs.size(); }
					return 0;
				}
				std::string CLOAK_CALL_THIS Value::getName() const 
				{
					API::Helper::Lock lock(m_parent->m_sync);
					return m_name; 
				}
				void CLOAK_CALL_THIS Value::removeUnused()
				{
					API::Helper::Lock lock(m_parent->m_sync);
					if (m_type == Type::OBJ || m_type == Type::ARRAY)
					{
						size_t m = m_childs.size();
						for (size_t a = 0; a < m; a++)
						{
							Value* v = m_childs[a];
							v->removeUnused();
							if (v->m_used == false || v->m_type == Type::UNDEF)
							{
								m_childs[a] = m_childs[m - 1];
								m_childs[m - 1] = v;
								a--;
								m--;
							}
						}
						for (size_t a = m; a < m_childs.size(); a++) { delete m_childs[a]; }
						if (m == 0) { m_childs.clear(); }
						else if (m < m_childs.size()) { m_childs.erase(m_childs.begin() + m, m_childs.end()); }
						m_used = m_forceUsed || m > 0;
					}
				}
				void CLOAK_CALL_THIS Value::resetUnused()
				{
					API::Helper::Lock lock(m_parent->m_sync);
					m_used = false;
					if (m_type == Type::OBJ || m_type == Type::ARRAY)
					{
						for (size_t a = 0; a < m_childs.size(); a++) { m_childs[a]->resetUnused(); }
					}
				}
				void CLOAK_CALL_THIS Value::clear()
				{
					API::Helper::Lock lock(m_parent->m_sync);
					if (m_type == Type::OBJ || m_type == Type::ARRAY) 
					{ 
						for (size_t a = 0; a < m_childs.size(); a++) { delete m_childs[a]; }
						m_childs.clear(); 
					}
				}
				Value& CLOAK_CALL_THIS Value::operator=(In const Value& v)
				{
					API::Helper::Lock lock(m_parent->m_sync);
					changeType(v.m_type);
					m_used = v.m_used;
					m_name = v.m_name;
					m_root = v.m_root;
					switch (m_type)
					{
						case CloakEngine::Impl::Files::Configuration_v1::Value::Type::STRING:
							m_valStr = v.m_valStr;
							break;
						case CloakEngine::Impl::Files::Configuration_v1::Value::Type::BOOL:
							m_valBool = v.m_valBool;
							break;
						case CloakEngine::Impl::Files::Configuration_v1::Value::Type::INT:
							m_valInt = v.m_valInt;
							break;
						case CloakEngine::Impl::Files::Configuration_v1::Value::Type::FLOAT:
							m_valFloat = v.m_valFloat;
							break;
						case CloakEngine::Impl::Files::Configuration_v1::Value::Type::OBJ:
						case CloakEngine::Impl::Files::Configuration_v1::Value::Type::ARRAY:
							m_childs = v.m_childs;
							break;
						case CloakEngine::Impl::Files::Configuration_v1::Value::Type::UNDEF:
						default:
							break;
					}
					return *this;
				}

				void CLOAK_CALL_THIS Value::changeType(In Value::Type type)
				{
					API::Helper::Lock lock(m_parent->m_sync);
					if (type != m_type)
					{
						if (m_type == Type::ARRAY || m_type == Type::OBJ) { m_childs.clear(); }
						m_type = type;
					}
				}

				CLOAK_CALL Configuration::Configuration() : m_root(this, nullptr)
				{
					DEBUG_NAME(Configuration);
					CREATE_INTERFACE(CE_QUERY_ARGS(&m_sync));
					m_lastPath = u"";
				}
				CLOAK_CALL Configuration::~Configuration()
				{
					SAVE_RELEASE(m_sync);
				}
				void CLOAK_CALL_THIS Configuration::readFile(In const API::Files::UFI& filePath, In API::Files::Configuration_v1::ConfigType type)
				{
					bool res = true;
					switch (type)
					{
						case CloakEngine::API::Files::Configuration_v1::ConfigType::INI:
							res = Parser::INI::readFile(filePath, &m_root);
							break;
						case CloakEngine::API::Files::Configuration_v1::ConfigType::JSON:
							res = Parser::JSON::readFile(filePath, &m_root);
							break;
						case CloakEngine::API::Files::Configuration_v1::ConfigType::XML:
							res = Parser::XML::readFile(filePath, &m_root);
							break;
						default:
							break;
					}
					if (res) { m_root.resetUnused(); }
					else { m_root.clear(); }
					m_changed = false;
					m_lastPath = filePath;
					m_lastType = type;
				}
				void CLOAK_CALL_THIS Configuration::saveFile(In const API::Files::UFI& filePath, In API::Files::Configuration_v1::ConfigType type)
				{
					if (m_changed || m_lastPath != filePath || type != m_lastType)
					{
						m_lastPath = filePath;
						switch (type)
						{
							case CloakEngine::API::Files::Configuration_v1::ConfigType::INI:
								Parser::INI::writeFile(filePath, &m_root);
								break;
							case CloakEngine::API::Files::Configuration_v1::ConfigType::JSON:
								Parser::JSON::writeFile(filePath, &m_root);
								break;
							case CloakEngine::API::Files::Configuration_v1::ConfigType::XML:
								CloakCheckOK(false, API::Global::Debug::Error::NOT_IMPLEMENTED, false);
								break;
							default:
								break;
						}
					}
				}
				void CLOAK_CALL_THIS Configuration::ReadBuffer(In API::Files::IReadBuffer* buffer, In API::Files::Configuration_v1::ConfigType type)
				{
					API::Files::IReader* read = nullptr;
					CREATE_INTERFACE(CE_QUERY_ARGS(&read));
					read->SetTarget(buffer);
					bool res = true;
					switch (type)
					{
						case CloakEngine::API::Files::Configuration_v1::ConfigType::INI:
							res = Parser::INI::ReadBuffer(read, &m_root);
							break;
						case CloakEngine::API::Files::Configuration_v1::ConfigType::JSON:
							res = Parser::JSON::ReadBuffer(read, &m_root);
							break;
						case CloakEngine::API::Files::Configuration_v1::ConfigType::XML:
							res = Parser::XML::ReadBuffer(read, &m_root);
							break;
						default:
							break;
					}
					SAVE_RELEASE(read);
					if (res) { m_root.resetUnused(); }
					else { m_root.clear(); }
					m_changed = false;
					m_lastPath = "";
					m_lastType = type;
				}
				void CLOAK_CALL_THIS Configuration::saveBuffer(In API::Files::IWriteBuffer* buffer, In API::Files::Configuration_v1::ConfigType type)
				{
					if (m_changed || type != m_lastType)
					{
						API::Files::IWriter* write = nullptr;
						CREATE_INTERFACE(CE_QUERY_ARGS(&write));
						write->SetTarget(buffer);
						switch (type)
						{
							case CloakEngine::API::Files::Configuration_v1::ConfigType::INI:
								Parser::INI::WriteBuffer(write, &m_root);
								break;
							case CloakEngine::API::Files::Configuration_v1::ConfigType::JSON:
								Parser::JSON::WriteBuffer(write, &m_root);
								break;
							case CloakEngine::API::Files::Configuration_v1::ConfigType::XML:
								CloakCheckOK(false, API::Global::Debug::Error::NOT_IMPLEMENTED, false);
								break;
							default:
								break;
						}
						SAVE_RELEASE(write);
					}
				}
				void CLOAK_CALL_THIS Configuration::saveFile()
				{
					saveFile(m_lastPath, m_lastType);
				}
				void CLOAK_CALL_THIS Configuration::removeUnused()
				{
					m_root.removeUnused();
				}
				API::Files::Configuration_v1::IValue* CLOAK_CALL_THIS Configuration::getRootValue()
				{
					return &m_root;
				}
				
				Success(return == true) bool CLOAK_CALL_THIS Configuration::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					CLOAK_QUERY(riid, ptr, SavePtr, Files::Configuration_v1, Configuration);
				}
			}
		}
	}
}
