#include "stdafx.h"
#include "CloakEngine/Global/Debug.h"
#include "CloakEngine/Helper/SyncSection.h"
#include "CloakEngine/Global/Graphic.h"
#include "CloakEngine/Global/Math.h"
#include "CloakEngine/Helper/StringConvert.h"
#include "CloakEngine/Global/Task.h"

#include "Implementation/Global/Debug.h"
#include "Implementation/Global/Game.h"

#include <atomic>
#include <conio.h>
#include <iostream>
#include <cctype>
#include <algorithm>
#include <comdef.h>
#include <codecvt>
#include <sstream>
#include <iomanip>
#include <DbgHelp.h>

#define CMD_BEGIN_ENTRY "> "
#define CMD_BEGIN_LENGTH 2

#define EXE_CMD_FAILED(isOpt,Exec,arg,offset,nfoa) if(isOpt){arg.Type=API::Global::Debug::CommandArgumentType::NOT_SET; offset++;nfoa++;} else {Exec=false;}

namespace CloakEngine {
	namespace API {
		namespace Global {
			namespace Debug {
				enum class CommandType { STRING, OPTION, INT, FLOAT };
				struct CommandSegment {
					CommandType Type;
					uint32_t Optional;
					API::List<std::string> Options;
				};
				struct Command {
					API::List<CommandSegment> Segments;
					std::string Name;
					std::string Description;
					std::function<bool(In const CommandArgument* args, In size_t count)> Function;
				};

				FILE* g_ConInFile;
				FILE* g_ConOutFile;
				FILE* g_ConErrFile;
				Helper::ISyncSection* g_syncWrite = nullptr;
				Helper::ISyncSection* g_syncCMD = nullptr;
				std::atomic<bool> g_useConsole = false;
				std::atomic<bool> g_symInitialized = false;
				std::atomic<size_t> g_longestCmd = 0;
				API::List<Command>* g_Cmds;
				std::atomic<Error> g_lastCritErr = Error::OK;

				namespace CommandRegistration {
					enum class InputType {
						SPACE,
						ESCAPE,
						ROUND_OPEN,
						ROUND_CLOSE,
						OPT_OPEN,
						OPT_CLOSE,
						PIPE,
						TYPE_TEXT,
						TYPE_INT,
						TYPE_FLOAT,
						TYPE_OPTION,
						QMARKS,
						OTHER,
					};
					enum State {
						WAIT,
						BEGIN,
						TYPE_FIXED,
						TYPE_TEXT,
						TYPE_INT,
						TYPE_FLOAT,
						TYPE_OPTION,
						NAME,
						NAME_OPTION_S,
						NAME_OPTION,
						NAME_END,
						O_WAIT,
						O_BEGIN,
						O_TYPE_FIXED,
						O_TYPE_TEXT,
						O_TYPE_INT,
						O_TYPE_FLOAT,
						O_TYPE_OPTION,
						O_NAME,
						O_NAME_OPTION_S,
						O_NAME_OPTION,
						O_NAME_END,
						INVALID,
					};

					constexpr State MACHINE[][static_cast<size_t>(InputType::OTHER)+1] = {
						//						SPACE		ESCAPE			ROUND_OPEN			ROUND_CLOSE		OPT_OPEN		OPT_CLOSE		PIPE				TYPE_TEXT			TYPE_INT		TYPE_FLOAT		TYPE_OPTION		QMARKS		OTHER
						/*WAIT				*/{	WAIT,		BEGIN,			INVALID,			INVALID,		O_WAIT,			INVALID,		INVALID,			TYPE_FIXED,			TYPE_FIXED,		TYPE_FIXED,		TYPE_FIXED,		INVALID,	TYPE_FIXED,		},
						/*BEGIN				*/{ INVALID,	TYPE_FIXED,		INVALID,			INVALID,		INVALID,		INVALID,		INVALID,			TYPE_TEXT,			TYPE_INT,		TYPE_FLOAT,		TYPE_OPTION,	INVALID,	INVALID,		},
						/*TYPE_FIXED		*/{ WAIT,		INVALID,		INVALID,			INVALID,		INVALID,		INVALID,		INVALID,			TYPE_FIXED,			TYPE_FIXED,		TYPE_FIXED,		TYPE_FIXED,		INVALID,	TYPE_FIXED,		},
						/*TYPE_TEXT			*/{ WAIT,		INVALID,		NAME,				INVALID,		INVALID,		INVALID,		INVALID,			INVALID,			INVALID,		INVALID,		INVALID,		INVALID,	INVALID,		},
						/*TYPE_INT			*/{ WAIT,		INVALID,		NAME,				INVALID,		INVALID,		INVALID,		INVALID,			INVALID,			INVALID,		INVALID,		INVALID,		INVALID,	INVALID,		},
						/*TYPE_FLOAT		*/{ WAIT,		INVALID,		NAME,				INVALID,		INVALID,		INVALID,		INVALID,			INVALID,			INVALID,		INVALID,		INVALID,		INVALID,	INVALID,		},
						/*TYPE_OPTION		*/{ INVALID,	INVALID,		NAME_OPTION_S,		INVALID,		INVALID,		INVALID,		INVALID,			INVALID,			INVALID,		INVALID,		INVALID,		INVALID,	INVALID,		},
						/*NAME				*/{ NAME,		NAME,			INVALID,			NAME_END,		INVALID,		INVALID,		INVALID,			NAME,				NAME,			NAME,			NAME,			INVALID,	NAME,			},
						/*NAME_OPTION_S		*/{ INVALID,	NAME_OPTION,	INVALID,			INVALID,		INVALID,		INVALID,		INVALID,			NAME_OPTION,		NAME_OPTION,	NAME_OPTION,	NAME_OPTION,	INVALID,	NAME_OPTION,	},
						/*NAME_OPTION		*/{ INVALID,	NAME_OPTION,	INVALID,			NAME_END,		INVALID,		INVALID,		NAME_OPTION_S,		NAME_OPTION,		NAME_OPTION,	NAME_OPTION,	NAME_OPTION,	INVALID,	NAME_OPTION,	},
						/*NAME_END			*/{ WAIT,		INVALID,		INVALID,			INVALID,		INVALID,		INVALID,		INVALID,			INVALID,			INVALID,		INVALID,		INVALID,		INVALID,	INVALID,		},

						/*O_WAIT			*/{ O_WAIT,		O_BEGIN,		INVALID,			INVALID,		INVALID,		WAIT,			INVALID,			O_TYPE_FIXED,		O_TYPE_FIXED,	O_TYPE_FIXED,	O_TYPE_FIXED,	INVALID,	O_TYPE_FIXED,	},
						/*O_BEGIN			*/{ INVALID,	O_TYPE_FIXED,	INVALID,			INVALID,		INVALID,		INVALID,		INVALID,			O_TYPE_TEXT,		O_TYPE_INT,		O_TYPE_FLOAT,	O_TYPE_OPTION,	INVALID,	INVALID,		},
						/*O_TYPE_FIXED		*/{ O_WAIT,		INVALID,		INVALID,			INVALID,		INVALID,		WAIT,			INVALID,			O_TYPE_FIXED,		O_TYPE_FIXED,	O_TYPE_FIXED,	O_TYPE_FIXED,	INVALID,	O_TYPE_FIXED,	},
						/*O_TYPE_TEXT		*/{ O_WAIT,		INVALID,		O_NAME,				INVALID,		INVALID,		WAIT,			INVALID,			INVALID,			INVALID,		INVALID,		INVALID,		INVALID,	INVALID,		},
						/*O_TYPE_INT		*/{ O_WAIT,		INVALID,		O_NAME,				INVALID,		INVALID,		WAIT,			INVALID,			INVALID,			INVALID,		INVALID,		INVALID,		INVALID,	INVALID,		},
						/*O_TYPE_FLOAT		*/{ O_WAIT,		INVALID,		O_NAME,				INVALID,		INVALID,		WAIT,			INVALID,			INVALID,			INVALID,		INVALID,		INVALID,		INVALID,	INVALID,		},
						/*O_TYPE_OPTION		*/{ INVALID,	INVALID,		O_NAME_OPTION_S,	INVALID,		INVALID,		INVALID,		INVALID,			INVALID,			INVALID,		INVALID,		INVALID,		INVALID,	INVALID,		},
						/*O_NAME			*/{ O_NAME,		O_NAME,			INVALID,			O_NAME_END,		INVALID,		INVALID,		INVALID,			O_NAME,				O_NAME,			O_NAME,			O_NAME,			INVALID,	O_NAME,			},
						/*O_NAME_OPTION_S	*/{ INVALID,	O_NAME_OPTION,	INVALID,			INVALID,		INVALID,		INVALID,		INVALID,			O_NAME_OPTION,		O_NAME_OPTION,	O_NAME_OPTION,	O_NAME_OPTION,	INVALID,	O_NAME_OPTION,	},
						/*O_NAME_OPTION		*/{ INVALID,	O_NAME_OPTION,	INVALID,			O_NAME_END,		INVALID,		INVALID,		O_NAME_OPTION_S,	O_NAME_OPTION,		O_NAME_OPTION,	O_NAME_OPTION,	O_NAME_OPTION,	INVALID,	O_NAME_OPTION,	},
						/*O_NAME_END		*/{ O_WAIT,		INVALID,		INVALID,			INVALID,		INVALID,		WAIT,			INVALID,			INVALID,			INVALID,		INVALID,		INVALID,		INVALID,	INVALID,		},

						/*INVALID			*/{	INVALID,	INVALID,		INVALID,			INVALID,		INVALID,		INVALID,		INVALID,			INVALID,			INVALID,		INVALID,		INVALID,		INVALID,	INVALID,		},
					};

					inline void CLOAK_CALL AppendSegment(Inout Command* res, Inout CommandSegment* seg, In const std::string& name)
					{
						if (res->Segments.size() > 0 && res->Segments.back().Optional != seg->Optional)
						{
							if (seg->Optional == 0) { res->Name += "] "; }
							else if (res->Name.length() > 0) { res->Name += " ["; }
							else { res->Name += '['; }
						}
						else if (res->Name.length() > 0) { res->Name += ' '; }
						res->Name += name;
						res->Segments.push_back(*seg);
						seg->Optional = 0;
						seg->Options.clear();
					}
					inline bool CLOAK_CALL ParseCommand(In const std::string& cmd, Out Command* res)
					{
						State curS = WAIT;
						State last = curS;

						std::stringstream s;
						CommandSegment seg;
						seg.Optional = 0;
						uint32_t optGroup = 0;
						for (size_t a = 0; a <= cmd.length(); a++)
						{
							const char c = a < cmd.length() ? cmd[a] : ' ';
							InputType it = InputType::OTHER;
							switch (c)
							{
								case ' ':
								case '\t':	it = InputType::SPACE; break;
								case '%':	it = InputType::ESCAPE; break;
								case '(':	it = InputType::ROUND_OPEN; break;
								case ')':	it = InputType::ROUND_CLOSE; break;
								case '[':	it = InputType::OPT_OPEN; break;
								case ']':	it = InputType::OPT_CLOSE; break;
								case '|':	it = InputType::PIPE; break;
								case 's':	it = InputType::TYPE_TEXT; break;
								case 'd':	it = InputType::TYPE_INT; break;
								case 'f':	it = InputType::TYPE_FLOAT; break;
								case 'o':	it = InputType::TYPE_OPTION; break;
								case '"':
								case '\'':	it = InputType::QMARKS; break;
								default:
									break;
							}

							last = curS;
							curS = MACHINE[curS][static_cast<size_t>(it)];

							switch (curS)
							{
								case CloakEngine::API::Global::Debug::CommandRegistration::O_WAIT:
									if (last == CloakEngine::API::Global::Debug::CommandRegistration::WAIT) { optGroup++; }
									seg.Optional = optGroup;
									//Fall through
								case CloakEngine::API::Global::Debug::CommandRegistration::WAIT:
									switch (last)
									{
										case CloakEngine::API::Global::Debug::CommandRegistration::O_TYPE_FIXED:
											seg.Optional = optGroup;
											//Fall through
										case CloakEngine::API::Global::Debug::CommandRegistration::TYPE_FIXED:
										{
											const std::string r = s.str();
											s.str("");

											seg.Type = CommandType::OPTION;
											seg.Options.push_back(r);

											AppendSegment(res, &seg, r);
											break;
										}
										case CloakEngine::API::Global::Debug::CommandRegistration::O_TYPE_TEXT:
											seg.Optional = optGroup;
											//Fall through
										case CloakEngine::API::Global::Debug::CommandRegistration::TYPE_TEXT:
										{
											seg.Type = CommandType::STRING;
											AppendSegment(res, &seg, "(string)");
											break;
										}
										case CloakEngine::API::Global::Debug::CommandRegistration::O_TYPE_INT:
											seg.Optional = optGroup;
											//Fall through
										case CloakEngine::API::Global::Debug::CommandRegistration::TYPE_INT:
										{
											seg.Type = CommandType::INT;
											AppendSegment(res, &seg, "(integer)");
											break;
										}
										case CloakEngine::API::Global::Debug::CommandRegistration::O_TYPE_FLOAT:
											seg.Optional = optGroup;
											//Fall through
										case CloakEngine::API::Global::Debug::CommandRegistration::TYPE_FLOAT:
										{
											seg.Type = CommandType::FLOAT;
											AppendSegment(res, &seg, "(float)");
											break;
										}
										default:
											break;
									}
									break;
								case CloakEngine::API::Global::Debug::CommandRegistration::O_TYPE_FIXED:
								case CloakEngine::API::Global::Debug::CommandRegistration::TYPE_FIXED:
									s << c;
									break;
								case CloakEngine::API::Global::Debug::CommandRegistration::O_NAME:
								case CloakEngine::API::Global::Debug::CommandRegistration::NAME:
									switch (last)
									{
										case CloakEngine::API::Global::Debug::CommandRegistration::TYPE_TEXT:
										case CloakEngine::API::Global::Debug::CommandRegistration::O_TYPE_TEXT:
											seg.Type = CommandType::STRING;
											break;
										case CloakEngine::API::Global::Debug::CommandRegistration::TYPE_INT:
										case CloakEngine::API::Global::Debug::CommandRegistration::O_TYPE_INT:
											seg.Type = CommandType::INT;
											break;
										case CloakEngine::API::Global::Debug::CommandRegistration::TYPE_FLOAT:
										case CloakEngine::API::Global::Debug::CommandRegistration::O_TYPE_FLOAT:
											seg.Type = CommandType::FLOAT;
											break;
										case CloakEngine::API::Global::Debug::CommandRegistration::NAME:
										case CloakEngine::API::Global::Debug::CommandRegistration::O_NAME:
											s << c;
											break;
										default:
											break;
									}
									break;
								case CloakEngine::API::Global::Debug::CommandRegistration::O_NAME_OPTION_S:
								case CloakEngine::API::Global::Debug::CommandRegistration::NAME_OPTION_S:
									switch (last)
									{
										case CloakEngine::API::Global::Debug::CommandRegistration::O_NAME_OPTION:
										case CloakEngine::API::Global::Debug::CommandRegistration::NAME_OPTION:
										{
											const std::string r = s.str();
											s.str("");

											seg.Options.push_back(r);
											break;
										}
										case CloakEngine::API::Global::Debug::CommandRegistration::O_TYPE_OPTION:
										case CloakEngine::API::Global::Debug::CommandRegistration::TYPE_OPTION:
										{
											seg.Type = CommandType::OPTION;
											break;
										}
									}
									break;
								case CloakEngine::API::Global::Debug::CommandRegistration::O_NAME_OPTION:
								case CloakEngine::API::Global::Debug::CommandRegistration::NAME_OPTION:
									s << c;
									break;
								case CloakEngine::API::Global::Debug::CommandRegistration::O_NAME_END:
								case CloakEngine::API::Global::Debug::CommandRegistration::NAME_END:
								{
									std::string r = s.str();
									s.str("");
									if (last == CloakEngine::API::Global::Debug::CommandRegistration::NAME_OPTION || last == CloakEngine::API::Global::Debug::CommandRegistration::O_NAME_OPTION)
									{ 
										seg.Options.push_back(r); 
										r = "";
										for (size_t a = 0; a < seg.Options.size(); a++) 
										{
											if (a > 0) { r += "|"; }
											r += seg.Options[a];
										}
									}
									AppendSegment(res, &seg, "(" + r + ")");
									break;
								}
								default:
									break;
							}
						}
						if (res->Segments.size() > 0 && res->Segments.back().Optional != 0) { res->Name += "]"; }
						return curS == WAIT;
					}
				}
				namespace CommandParsing {
					enum class InputType {
						SPACE,
						NUMBER_ZERO,
						NUMBER_OCT,
						NUMBER_ELSE,
						X,
						HEX_LETTER,
						POINT,
						MINUS,
						QMARKS,
						OTHER,
					};
					enum State {
						WAIT,
						NEG_NUMBER,
						NUMBER_HEX_1,
						NUMBER_HEX_2,
						NUMBER_HEX,
						NUMBER_OCT,
						NUMBER_INT,
						NUMBER_FLOAT,
						STRING,
						LONG_STRING,
						LONG_STRING_E,
						INVALID,
					};
					enum class TokenType {
						STRING,
						INTEGER,
						FLOAT,
					};
					struct Token {
						TokenType Type;
						struct {
							std::string String;
							int Integer;
							float Float;
						} Value;
					};

					constexpr State MACHINE[][static_cast<size_t>(InputType::OTHER) + 1] = {
						//						SPACE			NUMBER_ZERO		NUMBER_OCT		NUMBER_ELSE		X				HEX_LETTER		POINT			MINUS			QMARKS			OTHER
						/*WAIT*/			{	WAIT,			NUMBER_HEX_1,	NUMBER_INT,		NUMBER_INT,		STRING,			STRING,			NUMBER_FLOAT,	NEG_NUMBER,		LONG_STRING,	STRING		},
						/*NEG_NUMBER*/		{	WAIT,			NUMBER_HEX_1,	NUMBER_INT,		NUMBER_INT,		STRING,			STRING,			NUMBER_FLOAT,	STRING,			INVALID,		STRING		},
						/*NUMBER_HEX_1*/	{	WAIT,			NUMBER_OCT,		NUMBER_OCT,		STRING,			NUMBER_HEX_2,	STRING,			NUMBER_FLOAT,	STRING,			INVALID,		STRING		},
						/*NUMBER_HEX_2*/	{	WAIT,			NUMBER_HEX,		NUMBER_HEX,		NUMBER_HEX,		STRING,			NUMBER_HEX,		STRING,			STRING,			INVALID,		STRING		},
						/*NUMBER_HEX*/		{	WAIT,			NUMBER_HEX,		NUMBER_HEX,		NUMBER_HEX,		STRING,			NUMBER_HEX,		STRING,			STRING,			INVALID,		STRING		},
						/*NUMBER_OCT*/		{	WAIT,			NUMBER_OCT,		NUMBER_OCT,		STRING,			STRING,			STRING,			STRING,			STRING,			STRING,			STRING		},
						/*NUMBER_INT*/		{	WAIT,			NUMBER_INT,		NUMBER_INT,		NUMBER_INT,		STRING,			STRING,			NUMBER_FLOAT,	STRING,			INVALID,		STRING		},
						/*NUMBER_FLOAT*/	{	WAIT,			NUMBER_FLOAT,	NUMBER_FLOAT,	NUMBER_FLOAT,	STRING,			STRING,			STRING,			STRING,			INVALID,		STRING		},
						/*STRING*/			{	WAIT,			STRING,			STRING,			STRING,			STRING,			STRING,			STRING,			STRING,			INVALID,		STRING		},
						/*LONG_STRING*/		{	LONG_STRING,	LONG_STRING,	LONG_STRING,	LONG_STRING,	LONG_STRING,	LONG_STRING,	LONG_STRING,	LONG_STRING,	LONG_STRING_E,	LONG_STRING	},
						/*LONG_STRING_E*/	{	WAIT,			INVALID,		INVALID,		INVALID,		INVALID,		INVALID,		INVALID,		INVALID,		INVALID,		INVALID		},
						/*INVALID*/			{	INVALID,		INVALID,		INVALID,		INVALID,		INVALID,		INVALID,		INVALID,		INVALID,		INVALID,		INVALID		},
					};

					inline void CLOAK_CALL PushToken(Inout API::List<Token>* tokens, In TokenType type, In const std::string& str, In_opt int base = 10)
					{
						Token tk;
						tk.Type = type;
						switch (type)
						{
							case CloakEngine::API::Global::Debug::CommandParsing::TokenType::STRING:
								tk.Value.String = str;
								break;
							case CloakEngine::API::Global::Debug::CommandParsing::TokenType::FLOAT:
								try {
									tk.Value.Float = std::stof(str);
								}
								catch (...) { CLOAK_ASSUME(false); }
								tk.Value.String = std::to_string(tk.Value.Float);
								break;
							case CloakEngine::API::Global::Debug::CommandParsing::TokenType::INTEGER:
								try {
									tk.Value.Integer = std::stoi(str, nullptr, base);
								}
								catch (...) { CLOAK_ASSUME(false); }
								tk.Value.Float = static_cast<float>(tk.Value.Integer);
								tk.Value.String = std::to_string(tk.Value.Integer);
								break;
							default:
								break;
						}
						tokens->push_back(tk);
					}
					inline bool CLOAK_CALL ParseCommand(In const std::string& cmd, In const API::List<Command>& registered)
					{
						//Tokenize:
						State curS = WAIT;
						State last = curS;
						API::List<Token> tokens;
						std::stringstream s;
						for (size_t a = 0; a <= cmd.length(); a++)
						{
							const char c = a < cmd.length() ? cmd[a] : ' ';
							InputType it = InputType::OTHER;
							switch (c)
							{
								case ' ':
								case '\t':	it = InputType::SPACE; break;
								case '0':	it = InputType::NUMBER_ZERO; break;
								case '1':
								case '2':
								case '3':
								case '4':
								case '5':
								case '6':
								case '7':	it = InputType::NUMBER_OCT; break;
								case '8':
								case '9':	it = InputType::NUMBER_ELSE; break;
								case 'a':
								case 'b':
								case 'c':
								case 'd':
								case 'e':
								case 'f':
								case 'A':
								case 'B':
								case 'C':
								case 'D':
								case 'E':
								case 'F':	it = InputType::HEX_LETTER; break;
								case 'X':
								case 'x':	it = InputType::X; break;
								case '.':	it = InputType::POINT; break;
								case '-':	it = InputType::MINUS; break;
								case '"':	it = InputType::QMARKS; break;
								default:
									break;
							}
							last = curS;
							curS = MACHINE[curS][static_cast<size_t>(it)];
							switch (curS)
							{
								case CloakEngine::API::Global::Debug::CommandParsing::WAIT:
									switch (last)
									{
										case CloakEngine::API::Global::Debug::CommandParsing::NEG_NUMBER:
											PushToken(&tokens, TokenType::STRING, s.str());
											break;
										case CloakEngine::API::Global::Debug::CommandParsing::NUMBER_HEX_1:
											PushToken(&tokens, TokenType::INTEGER, s.str());
											break;
										case CloakEngine::API::Global::Debug::CommandParsing::NUMBER_HEX_2:
											PushToken(&tokens, TokenType::STRING, s.str());
											break;
										case CloakEngine::API::Global::Debug::CommandParsing::NUMBER_HEX:
											PushToken(&tokens, TokenType::INTEGER, s.str(), 16);
											break;
										case CloakEngine::API::Global::Debug::CommandParsing::NUMBER_OCT:
											PushToken(&tokens, TokenType::INTEGER, s.str(), 8);
											break;
										case CloakEngine::API::Global::Debug::CommandParsing::NUMBER_INT:
											PushToken(&tokens, TokenType::INTEGER, s.str());
											break;
										case CloakEngine::API::Global::Debug::CommandParsing::NUMBER_FLOAT:
											PushToken(&tokens, TokenType::FLOAT, s.str());
											break;
										case CloakEngine::API::Global::Debug::CommandParsing::STRING:
											PushToken(&tokens, TokenType::STRING, s.str());
											break;
										case CloakEngine::API::Global::Debug::CommandParsing::LONG_STRING_E:
											PushToken(&tokens, TokenType::STRING, s.str());
											break;
										default:
											break;
									}
									s.str("");
									break;
								case CloakEngine::API::Global::Debug::CommandParsing::NEG_NUMBER:
								case CloakEngine::API::Global::Debug::CommandParsing::NUMBER_HEX_1:
								case CloakEngine::API::Global::Debug::CommandParsing::NUMBER_HEX_2:
								case CloakEngine::API::Global::Debug::CommandParsing::NUMBER_HEX:
								case CloakEngine::API::Global::Debug::CommandParsing::NUMBER_OCT:
								case CloakEngine::API::Global::Debug::CommandParsing::NUMBER_INT:
								case CloakEngine::API::Global::Debug::CommandParsing::NUMBER_FLOAT:
								case CloakEngine::API::Global::Debug::CommandParsing::STRING:
									s << c;
									break;
								case CloakEngine::API::Global::Debug::CommandParsing::LONG_STRING:
									if (last == curS) { s << c; }
									break;
								case CloakEngine::API::Global::Debug::CommandParsing::LONG_STRING_E:
								case CloakEngine::API::Global::Debug::CommandParsing::INVALID:
									break;
								default:
									break;
							}
						}
						if (curS != WAIT || tokens.size() == 0) { return false; }

						const size_t maxUsage = g_longestCmd;
						CommandArgument* args = NewArray(CommandArgument, maxUsage);
						//Find suitable command:
						bool foundCmd = false;
						for (size_t a = 0; a < registered.size(); a++)
						{
							const Command& cmd = registered[a];
							if (tokens.size() <= cmd.Segments.size())
							{
								CLOAK_ASSUME(cmd.Segments.size() <= maxUsage);
								for (size_t b = 0; b < maxUsage; b++) { args[b].Type = API::Global::Debug::CommandArgumentType::NOT_SET; }
								bool suc = true;

								size_t nextCmd = 0;
								for (size_t b = 0; b < tokens.size(); b++)
								{
									if (nextCmd >= cmd.Segments.size()) 
									{ 
										suc = false; 
										break; 
									}
									else
									{
										//Find possible segment:
										size_t fcmd = nextCmd;
										do {
											if (args[fcmd].Type == API::Global::Debug::CommandArgumentType::NOT_SET)
											{
												const CommandType cmdt = cmd.Segments[fcmd].Type;
												switch (tokens[b].Type)
												{
													case TokenType::INTEGER:
														if (cmdt == CommandType::INT)
														{
															args[fcmd].Type = API::Global::Debug::CommandArgumentType::INT;
															args[fcmd].Value.Integer = tokens[b].Value.Integer;
															goto parse_command_found_segment;
														}
													case TokenType::FLOAT:
														if (cmdt == CommandType::FLOAT)
														{
															args[fcmd].Type = API::Global::Debug::CommandArgumentType::FLOAT;
															args[fcmd].Value.Float = tokens[b].Value.Float;
															goto parse_command_found_segment;
														}
													case TokenType::STRING:
														if (cmdt == CommandType::STRING)
														{
															args[fcmd].Type = API::Global::Debug::CommandArgumentType::STRING;
															args[fcmd].Value.String = tokens[b].Value.String;
															goto parse_command_found_segment;
														}
														else if (cmdt == CommandType::OPTION)
														{
															for (size_t c = 0; c < cmd.Segments[fcmd].Options.size(); c++)
															{
																if (cmd.Segments[fcmd].Options[c].compare(tokens[b].Value.String) == 0)
																{
																	args[fcmd].Type = cmd.Segments[fcmd].Options.size() == 1 ? API::Global::Debug::CommandArgumentType::FIXED : API::Global::Debug::CommandArgumentType::OPTION;
																	args[fcmd].Value.Option = c;
																	goto parse_command_found_segment;
																}
															}
														}
													default:
														break;
												}
											}
											if (cmd.Segments[fcmd].Optional == 0) { break; }
											fcmd++;
										} while (fcmd < cmd.Segments.size());
										//Parsing failed:
										suc = false;
										break;

									parse_command_found_segment:
										//Found suitable segment, move to next possible segment:
										if (cmd.Segments[fcmd].Optional == 0 || fcmd == nextCmd) { nextCmd = fcmd + 1; }
										else if (cmd.Segments[fcmd].Optional != cmd.Segments[nextCmd].Optional)
										{
											for (nextCmd = fcmd; nextCmd > 0 && cmd.Segments[nextCmd - 1].Optional == cmd.Segments[fcmd].Optional; nextCmd--) {}
										}
									}
								}
								//Find missing non-optional segments:
								if (suc == true)
								{
									for (; nextCmd < cmd.Segments.size(); nextCmd++)
									{
										if (cmd.Segments[nextCmd].Optional == 0) 
										{ 
											suc = false; 
											break;
										}
									}
								}
								//command fully parsed:
								if (suc == true)
								{
									foundCmd = cmd.Function(args, cmd.Segments.size()) || foundCmd;
								}
							}
						}
						DeleteArray(args);
						return foundCmd;
					}
				}


				void CLOAK_CALL createOptionsList(In std::string text, Inout API::List<std::string>& list)
				{
					list.clear();
					size_t np;
					std::string sub;
					do {
						np = text.find_first_of('|');
						sub = text.substr(0, np);
						if (np == text.npos) { text = ""; }
						else { text = text.substr(np + 1); }
						list.push_back(sub);
					} while (text.length() > 0);
				}

				CLOAK_CALL PerformanceTimer::PerformanceTimer()
				{
					m_samples = 0;
					m_start = 0;
					m_acc = 0;
				}
				void CLOAK_CALL_THIS PerformanceTimer::Start()
				{
					if (CloakCheckOK(m_start == 0, API::Global::Debug::Error::ILLEGAL_FUNC_CALL, false))
					{
						m_start = Impl::Global::Game::getCurrentTimeMicroSeconds();
					}
				}
				void CLOAK_CALL_THIS PerformanceTimer::Stop()
				{
					if (CloakCheckOK(m_start != 0, API::Global::Debug::Error::ILLEGAL_FUNC_CALL, false))
					{
						const uint64_t d = Impl::Global::Game::getCurrentTimeMicroSeconds() - m_start;
						m_acc += d;
						m_samples++;
						m_start = 0;
					}
				}
				void CLOAK_CALL_THIS PerformanceTimer::Reset()
				{
					m_acc = 0;
					m_samples = 0;
				}
				double CLOAK_CALL_THIS PerformanceTimer::GetPerformanceMicroSeconds() const
				{
					return m_samples == 0 ? 0 : static_cast<double>(m_acc) / m_samples;
				}
				uint64_t CLOAK_CALL_THIS PerformanceTimer::GetSamples() const
				{
					return m_samples;
				}

				CLOAKENGINE_API void CLOAK_CALL SendError(In Error toThrow, In bool critical, In_opt Log::Source source, In_opt std::string file, In_opt int line, In_opt unsigned int framesToSkip)
				{
					SendError(toThrow, critical, "", source, file, line, framesToSkip + 1);
				}
				CLOAKENGINE_API void CLOAK_CALL SendError(In Error toThrow, In bool critical, In std::string info, In_opt Log::Source source, In_opt std::string file, In_opt int line, In_opt unsigned int framesToSkip)
				{
					const unsigned long long tThrow = static_cast<unsigned long long>(toThrow) & 0xffffffff;
					std::stringstream e;
					e << std::setfill('0') << std::setw(8) << std::hex << tThrow;
					std::stringstream s;
					s << "Got";
					if (critical) { s << " critical"; }
					s << " Error 0x" << e.str() << std::endl;
					if (info.length() > 0) { s << info << std::endl; }
#ifdef _DEBUG
					if (file.length() > 0)
#else
					if (file.length() > 0 && API::Global::Game::IsDebugMode() && source != Log::Source::Engine)
#endif
					{
						s << " in " << file << " at line " << line << std::endl;
					}
#ifndef _DEBUG
					if (API::Global::Game::IsDebugMode())
#endif
					{
						if (g_symInitialized == true)
						{
							HANDLE proc = GetCurrentProcess();
							void* stack[5];
							unsigned short frames = CaptureStackBackTrace(framesToSkip + 1, ARRAYSIZE(stack), stack, NULL);
							uint8_t frameData[sizeof(SYMBOL_INFO) + (sizeof(char) * 256)];
							SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO*>(frameData);
							symbol->MaxNameLen = 255;
							symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
							for (unsigned short a = 0; a < frames; a++)
							{
								if (SymFromAddr(proc, reinterpret_cast<DWORD64>(stack[a]), 0, symbol))
								{
									e.str("");
									e << std::setfill('0') << std::setw(8) << std::hex << symbol->Address;
									s << (frames - (a + 1)) << ": at " << symbol->Name << " (0x" << e.str() << ")" << std::endl;
								}
								else
								{
									s << (frames - (a + 1)) << ": at unknown source" << std::endl;
								}
							}
						}
					}
					Log::WriteToLog(s.str(), critical ? Log::Type::Error : Log::Type::Warning, source);
					if (critical) { g_lastCritErr = toThrow; API::Global::Game::Stop(); }
				}
				CLOAKENGINE_API Error CLOAK_CALL RegisterConsoleArgument(In std::string parseCmd, In const std::string& description, Inout std::function<bool(In const CommandArgument* args, In size_t count)>&& func)
				{
					if (parseCmd.length() == 0) { return Error::CMD_TOO_SHORT; }
					if (func == false) { return Error::ILLEGAL_ARGUMENT; }
					if (API::Global::Debug::g_useConsole == true)
					{
						Helper::Lock lock(g_syncCMD);
						
						Command ncmd;
						if (CommandRegistration::ParseCommand(parseCmd, &ncmd))
						{
							size_t ov = g_longestCmd;
							while (ov < ncmd.Segments.size() && g_longestCmd.compare_exchange_weak(ov, ncmd.Segments.size()) == false) {}
							ncmd.Description = "  -> " + description;
							ncmd.Function = std::move(func);
							g_Cmds->push_back(ncmd);
							return Error::OK;
						}
						return Error::CMD_UNKNOWN_PARSE;
					}
					return Error::OK;
				}
			}
		}
	}
	namespace Impl {
		namespace Global {
			namespace Debug {
				std::string g_curCmd = "";
				size_t g_cmdPos = 0;
				size_t g_inCmdPos = 0;
				API::List<std::string>* g_enteredCmds;
				std::atomic<API::Global::IDebugEvent*> g_dbgEvent = nullptr;

#ifdef _DEBUG
				Engine::LinkedList<ThreadDebugData>* g_threadList;
				API::Helper::ISyncSection* g_syncThreadList = nullptr;

				void OnStartThreadDebug()
				{
					g_threadList = new Engine::LinkedList<ThreadDebugData>();
					CREATE_INTERFACE(CE_QUERY_ARGS(&g_syncThreadList));
				}
				void OnStopThreadDebug()
				{
					SAVE_RELEASE(g_syncThreadList);
					delete g_threadList;
				}
				void OnUpdateThreadDebug()
				{
					API::Helper::Lock lock(g_syncThreadList);
					const unsigned long long time = Impl::Global::Game::getCurrentTimeMilliSeconds();
					for (ThreadDebug d = g_threadList->begin(); d != nullptr; d++)
					{
						if (d->time < time && time - d->time >= 2000 && d->print == false)
						{
							d->print = true;
							API::Global::Log::WriteToLog("Thread stopped at line " + std::to_string(d->line) + " in file '" + d->file + "' (Step = " + std::to_string(d->step) + ")", API::Global::Log::Type::Debug);
						}
					}
				}
				ThreadDebug CLOAK_CALL StartThreadDebug(In unsigned int step, In_opt std::string file, In_opt int line)
				{
					API::Helper::Lock lock(g_syncThreadList);
					ThreadDebugData dd;
					dd.file = file;
					dd.line = line;
					dd.step = step;
					dd.time = Impl::Global::Game::getCurrentTimeMilliSeconds();
					dd.print = false;
					return g_threadList->push(dd);
				}
				void CLOAK_CALL StepThreadDebug(In ThreadDebug debug, In unsigned int step, In_opt int line)
				{
					API::Helper::Lock lock(g_syncThreadList);
					if (debug != nullptr)
					{
						debug->step = step;
						debug->line = line;
						debug->time = Impl::Global::Game::getCurrentTimeMilliSeconds();
					}
				}
				void CLOAK_CALL StopThreadDebug(In ThreadDebug debug)
				{
					API::Helper::Lock lock(g_syncThreadList);
					g_threadList->remove(debug);
				}
#define OnStartTD() OnStartThreadDebug()
#define OnStopTD() OnStopThreadDebug()
#define OnUpdateTD() OnUpdateThreadDebug()
#else
#define OnStartTD()
#define OnStopTD()
#define OnUpdateTD()
#endif

				inline bool CLOAK_CALL isInt(In const std::string& t, In_opt bool onlyPositive = false)
				{
					if (t.length() == 0) { return false; }
					char c = t.at(0);
					return std::find_if((c == '-' && onlyPositive == false && t.length() > 1) ? t.begin() + 1 : t.begin(), t.end(), [](char c) {return !std::isdigit(c); }) == t.end();
				}
				void CLOAK_CALL printFPS()
				{
					API::Global::FPSInfo fps;
					API::Global::Game::GetFPS(&fps);
					for (size_t a = 0; a < fps.Count; a++)
					{
						API::Global::Log::WriteToLog("#" + std::to_string(a) + ": " + std::to_string(fps.Thread[a].FPS), API::Global::Log::Type::Console);
					}
				}

				void CLOAK_CALL setDebugEvent(In API::Global::IDebugEvent* ev)
				{
					g_dbgEvent = ev;
				}
				void CLOAK_CALL writeToConsole(In const std::string& text, In_opt API::Global::Log::Source src)
				{
					if (API::Global::Debug::g_useConsole == true)
					{
						API::Helper::Lock lock(API::Global::Debug::g_syncWrite);
						for (size_t a = 0; a < g_curCmd.length() + CMD_BEGIN_LENGTH; a++) { std::cout << "\b \b"; }
						std::cout << text << CMD_BEGIN_ENTRY << g_curCmd;
					}
					if (src == API::Global::Log::Source::Game || src == API::Global::Log::Source::Modification)
					{
						API::Global::IDebugEvent* ev = g_dbgEvent;
						if (ev != nullptr) { ev->OnLogWrite(text); }
					}
				}
				void CLOAK_CALL updateConsole(In size_t threadID, In API::Global::Time etime)
				{
					if (API::Global::Debug::g_useConsole == true)
					{
						OnUpdateTD();
						//Check for input in console:
						if (_kbhit())
						{
							char c = _getch();
							if (c == '\n' || c == '\r')
							{
								API::Helper::Lock lock(API::Global::Debug::g_syncWrite);
								std::string lcmd = g_curCmd;
								g_curCmd = "";
								g_inCmdPos = 0;
								g_enteredCmds->push_back(lcmd);
								g_cmdPos = g_enteredCmds->size();
								std::cout << "\r\n" << CMD_BEGIN_ENTRY;
								lock.unlock();

								//cut off empty spaces:
								while (lcmd.length() > 0 && lcmd.at(0) == ' ') { lcmd = lcmd.substr(1); }
								while (lcmd.length() > 0 && lcmd.at(lcmd.length() - 1) == ' ') { lcmd = lcmd.substr(0, lcmd.length() - 1); }

								//check entered cmd:

								API::Global::PushTask([=](In size_t id) {
									API::Global::Log::WriteToLog("User entered command '" + lcmd + "'", API::Global::Log::Type::File);
									if (API::Global::Debug::CommandParsing::ParseCommand(lcmd, *API::Global::Debug::g_Cmds) == false)
									{
										API::Global::Log::WriteToLog("Unknown command! Type '?' to see a list of all commands.", API::Global::Log::Type::Console);
									}
								});
							}
							else if (c == '\b')
							{
								API::Helper::Lock lock(API::Global::Debug::g_syncWrite);
								if (g_curCmd.length() > 0)
								{
									std::string ns = "";
									g_inCmdPos--;
									if (g_inCmdPos == g_curCmd.length()) { ns = "\b \b"; }
									else
									{
										ns = "\b";
										std::string ti = (g_inCmdPos + 1 < g_curCmd.length() ? g_curCmd.substr(g_inCmdPos + 1) : "") + " ";
										ns += ti;
										for (size_t a = 0; a < ti.length(); a++) { ns += "\b"; }
										g_curCmd = (g_inCmdPos > 0 ? g_curCmd.substr(0, g_inCmdPos) : "") + (g_inCmdPos + 1 < g_curCmd.length() ? g_curCmd.substr(g_inCmdPos + 1) : "");
										std::cout << ns;
									}
								}
							}
							else if (c == (char)-32)//special chars
							{
								c = _getch();
								if (c == (char)77)//arrow right
								{
									API::Helper::Lock lock(API::Global::Debug::g_syncWrite);
									if (g_inCmdPos < g_curCmd.length())
									{
										std::cout << g_curCmd.at(g_inCmdPos);
										g_inCmdPos++;
									}
								}
								else if (c == (char)75)//arrow left
								{
									API::Helper::Lock lock(API::Global::Debug::g_syncWrite);
									if (g_inCmdPos > 0)
									{
										std::cout << "\b";
										g_inCmdPos--;
									}
								}
								else if (c == (char)72)//arrow up
								{
									API::Helper::Lock lock(API::Global::Debug::g_syncWrite);
									std::string ns = "";
									if (g_inCmdPos < g_curCmd.length()) { ns += g_curCmd.substr(g_inCmdPos); }
									for (size_t a = 0; a < g_curCmd.length(); a++) { ns += "\b \b"; }
									if (g_cmdPos > 0)
									{
										g_cmdPos--;
										g_curCmd = g_enteredCmds->at(g_cmdPos);
										ns += g_curCmd;
									}
									else { g_curCmd = ""; }
									std::cout << ns;
									g_inCmdPos = g_curCmd.length();
								}
								else if (c == (char)80)//arrow down
								{
									API::Helper::Lock lock(API::Global::Debug::g_syncWrite);
									std::string ns = "";
									if (g_inCmdPos < g_curCmd.length()) { ns += g_curCmd.substr(g_inCmdPos); }
									for (size_t a = 0; a < g_curCmd.length(); a++) { ns += "\b \b"; }
									if (g_cmdPos + 1 < g_enteredCmds->size())
									{
										g_cmdPos++;
										g_curCmd = g_enteredCmds->at(g_cmdPos);
										ns += g_curCmd;
									}
									else
									{
										g_cmdPos = g_enteredCmds->size();
										g_curCmd = "";
									}
									std::cout << ns;
									g_inCmdPos = g_curCmd.length();
								}
								else if (c == (char)71)//pos1
								{
									API::Helper::Lock lock(API::Global::Debug::g_syncWrite);
									std::string ns = "";
									for (size_t a = 0; a < g_inCmdPos; a++) { ns += "\b"; }
									std::cout << ns;
									g_inCmdPos = 0;
								}
								else if (c == (char)79)//ende
								{
									API::Helper::Lock lock(API::Global::Debug::g_syncWrite);
									if (g_inCmdPos < g_curCmd.length()) { std::cout << g_curCmd.substr(g_inCmdPos); }
									g_inCmdPos = g_curCmd.length();
								}
								else if (c == (char)83)//entf
								{
									API::Helper::Lock lock(API::Global::Debug::g_syncWrite);
									if (g_inCmdPos < g_curCmd.length())
									{
										std::string ti = (g_inCmdPos + 1 < g_curCmd.length() ? g_curCmd.substr(g_inCmdPos + 1) : "");
										std::string ns = ti + " ";
										for (size_t a = 0; a <= ti.length(); a++) { ns += "\b"; }
										std::cout << ns;
										g_curCmd = (g_inCmdPos > 0 ? g_curCmd.substr(0, g_inCmdPos) : "") + ti;
									}
								}
							}
							else if (c >= (char)32 && c <= (char)127)//writeable chars
							{
								API::Helper::Lock lock(API::Global::Debug::g_syncWrite);
								std::cout << c;
								g_curCmd = (g_inCmdPos>0 ? g_curCmd.substr(0, g_inCmdPos) : "") + c + (g_inCmdPos + 1 < g_curCmd.length() ? g_curCmd.substr(g_inCmdPos + 1) : "");
								g_inCmdPos++;
							}
						}
					}
				}
				void CLOAK_CALL Initialize()
				{
					API::Global::Debug::g_Cmds = new API::List<API::Global::Debug::Command>();
					g_enteredCmds = new API::List<std::string>();
#ifndef _DEBUG
					if (API::Global::Game::IsDebugMode()) 
#endif
					{ 
						API::Global::Debug::g_symInitialized = SymInitialize(GetCurrentProcess(), NULL, TRUE) == TRUE; 
					}
				}
				void CLOAK_CALL Release()
				{
					delete g_enteredCmds;
					delete API::Global::Debug::g_Cmds;
				}
				void CLOAK_CALL releaseConsole(In size_t threadID)
				{
					if (API::Global::Debug::g_useConsole == true)
					{
						OnStopTD();
						API::Global::Debug::g_useConsole = false;
						std::fclose(API::Global::Debug::g_ConInFile);
						std::fclose(API::Global::Debug::g_ConOutFile);
						std::fclose(API::Global::Debug::g_ConErrFile);
						SAVE_RELEASE(API::Global::Debug::g_syncCMD);
						SAVE_RELEASE(API::Global::Debug::g_syncWrite);
						API::Global::Debug::g_Cmds->clear();
					}
				}
				void CLOAK_CALL enableConsole(In size_t threadID)
				{
					if (API::Global::Debug::g_useConsole.exchange(true) == false)
					{
						OnStartTD();
						CREATE_INTERFACE(CE_QUERY_ARGS(&API::Global::Debug::g_syncWrite));
						CREATE_INTERFACE(CE_QUERY_ARGS(&API::Global::Debug::g_syncCMD));
						BOOL res = AllocConsole();
						if (CloakCheckError(res != FALSE, API::Global::Debug::Error::GAME_DEBUG_LAYER, false)) { return; }
						res = SetConsoleTitle(L"Debug");
						if (CloakCheckError(res != FALSE, API::Global::Debug::Error::GAME_DEBUG_LAYER, false)) { return; }
						errno_t err = freopen_s(&API::Global::Debug::g_ConInFile, "CONIN$", "r", stdin);
						if (CloakCheckError(err == 0, API::Global::Debug::Error::GAME_DEBUG_LAYER, false)) { return; }
						err = freopen_s(&API::Global::Debug::g_ConOutFile, "CONOUT$", "w", stdout);
						if (CloakCheckError(err == 0, API::Global::Debug::Error::GAME_DEBUG_LAYER, false)) { return; }
						err = freopen_s(&API::Global::Debug::g_ConErrFile, "CONOUT$", "w", stderr);
						if (CloakCheckError(err == 0, API::Global::Debug::Error::GAME_DEBUG_LAYER, false)) { return; }
						std::cout << CMD_BEGIN_ENTRY << g_curCmd;

						//register default cmds:
						API::Global::Debug::RegisterConsoleArgument("%o(?|help)", "", [=](In const API::Global::Debug::CommandArgument* args, In size_t count) {
							API::Helper::Lock lock(API::Global::Debug::g_syncCMD);
							for (size_t a = 0; a < API::Global::Debug::g_Cmds->size(); a++)
							{
								const API::Global::Debug::Command& d = API::Global::Debug::g_Cmds->at(a);
								if (d.Name.compare("(?|help)") != 0)
								{
									writeToConsole(d.Name + "\r\n" + d.Description + "\r\n");
								}
							}
							return true;
						});
						API::Global::Debug::RegisterConsoleArgument("cloak stop", "Stops the game", [=](In const API::Global::Debug::CommandArgument* args, In size_t count) { API::Global::Game::Stop(); return true; });
						API::Global::Debug::RegisterConsoleArgument("cloak pause", "Pauses the game", [=](In const API::Global::Debug::CommandArgument* args, In size_t count) { return true; });
						API::Global::Debug::RegisterConsoleArgument("cloak resume", "Resumes the game", [=](In const API::Global::Debug::CommandArgument* args, In size_t count) { return true; });
						API::Global::Debug::RegisterConsoleArgument("cloak fps", "Shows the current FPS-Rates of all running threads", [=](In const API::Global::Debug::CommandArgument* args, In size_t count) {printFPS(); return true; });
						if (API::Global::Game::HasWindow())
						{
							API::Global::Debug::RegisterConsoleArgument("cloak fps %f(X)", "Sets the target FPS-Rate of all running threads to X", [=](In const API::Global::Debug::CommandArgument* args, In size_t count) { API::Global::Game::SetFPS(args[2].Value.Float); return true; });
							API::Global::Debug::RegisterConsoleArgument("cloak fov %f(X)", "Sets the field of view to X", [=](In const API::Global::Debug::CommandArgument* args, In size_t count)
							{ 
								const float nfov = args[2].Value.Float;
								if (nfov >= 0.1f && nfov < API::Global::Math::PI)
								{
									API::Global::Graphic::Settings set;
									API::Global::Graphic::GetSettings(&set);
									set.FieldOfView = nfov;
									API::Global::Graphic::SetSettings(set);
								}
								else
								{
									API::Global::Log::WriteToLog("Illegal argument: FOV is too low or too heigh (must be between 0.1 and pi)", API::Global::Log::Type::Console);
								}
								return true; 
							});
							API::Global::Debug::RegisterConsoleArgument("cloak window %o(fullscreen|windows|fullwindow)", "Switches between fullscreen, windowed and maximized windowed mode", [=](In const API::Global::Debug::CommandArgument* args, In size_t count)
							{ 
								API::Global::Graphic::Settings set;
								API::Global::Graphic::GetSettings(&set);
								switch (args[2].Value.Option)
								{
									case 0:
										set.WindowMode= API::Global::Graphic::WindowMode::FULLSCREEN;
										break;
									case 1:
										set.WindowMode = API::Global::Graphic::WindowMode::WINDOW;
										break;
									case 2:
										set.WindowMode = API::Global::Graphic::WindowMode::FULLSCREEN_WINDOW;
										break;
									default:
										break;
								}
								API::Global::Graphic::SetSettings(set);
								return true; 
							});
							API::Global::Debug::RegisterConsoleArgument("cloak size %d(Width) %d(Height)", "Resizes the game to the given size", [=](In const API::Global::Debug::CommandArgument* args, In size_t count)
							{
								const int nw = args[2].Value.Integer;
								const int nh = args[3].Value.Integer;
								if (nw > 8 && nh > 8)
								{
									API::Global::Graphic::Settings set;
									API::Global::Graphic::GetSettings(&set);
									set.Resolution.Width = static_cast<unsigned int>(nw);
									set.Resolution.Height = static_cast<unsigned int>(nh);
									API::Global::Graphic::SetSettings(set);
								}
								else
								{
									API::Global::Log::WriteToLog("Illegal argument: Width or Height is too low (must be greater than 8)", API::Global::Log::Type::Console);
								}
								return true; 
							});
							//API::Global::Debug::RegisterConsoleArgument("cloak light toggle %o(ambient|directional|point|spot|shadow) %o(on|off)", "Toggles the given light effect on or off", [=](const API::Global::Debug::ConsoleCommand& args) { return true; });
						}
					}
				}
				API::Global::Debug::Error CLOAK_CALL getLastError() { return API::Global::Debug::g_lastCritErr; }
			}
		}
	}
}