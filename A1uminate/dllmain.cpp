// ======================================== //
//				  Inclusion					//
// ======================================== //
#include <stdio.h>
#include <Windows.h>

// Input
#include <random>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <regex>
#include <set>
//#include <signal.h>

// This shit is getting out of hand
#include <cassert> // Registry.h

// Custom WinAPI Functions
#include "CustomWinApi.h"

// File Management
#include <filesystem>
#include <fstream>

// Fivem Related
#include "EventCore.h"
#include "Registry.h"
#include "ICoreGameInit.h"
#include "ComponentHolder.h"
#include <msgpack.hpp>
#include "Resource.h"
#include "ResourceCache.h"
#include "ResourceManager.h"
#include "ResourceMetaDataComponent.h"
#include "VFSManager.h"



// ======================================== //
//			  Important variables			//
// ======================================== //

/* DLL Variables */
bool DLL_ACTIVE = true;
std::regex illegalChars ("[\\w\\s\\(\\)@\\-]");
std::set<std::string> blacklist {
	"anti", "cheat", "snow", "ggac", "toko"
};

/* Path Variables */
std::string prefix = "/";
std::string resourcename = "memesource";
std::filesystem::path path_to_resource = "C:\\memes\\" + resourcename;
std::filesystem::path path_to_loadedresource = "C:\\Users\\xxure\\AppData\\Local\\FiveM\\FiveM.app\\citizen\\scripting\\" + resourcename;
std::filesystem::path path_to_dumps = "C:\\memes\\dumps";

/* Miscellaneous Variables */
#define ENDLINE "\n\n"

enum CMD_CODE {
	CMD_QUIT,
	CMD_KILL,
	CMD_CLEAR,
	CMD_NOT_FOUND
};

enum INSTANCE_CMD_CODE {
	ICMD_DUMP,
	ICMD_BLOCK,
	ICMD_EXECUTE,
	ICMD_NOT_FOUND
};



// ======================================== //
//			  Important functions			//
// ======================================== //
/*
void signint_handler(int sigCount) {
	std::cout << "Can not escape with Control + C" << std::endl;
}
*/

/* Core functions */
char* getWindowTitle(HWND hwnd)
{
	int len = GetWindowTextLength(hwnd) + 1;
	std::vector<char> windowTitle(len);
	GetWindowTextA(hwnd, &windowTitle[0], len);
	return &windowTitle[0];
}

std::string cleanseFilename(std::string clean)
{
	for (int i = 0; i < clean.length(); i++) {
		std::string curr(1, clean.at(i));
		if (!std::regex_match(curr, illegalChars)) {
			clean[i] = '@';
		}
	}

	return clean;
}

template <class Container>
void readCmdLine(const std::string& str, Container& cont)
{
	std::istringstream iss(str);
	std::copy(std::istream_iterator<std::string>(iss),
		std::istream_iterator<std::string>(),
		std::back_inserter(cont));
}

CMD_CODE tokenify(std::string arg) {
	if (arg == prefix + "quit") return CMD_QUIT;
	if (arg == prefix + "kill") return CMD_KILL;
	if (arg == prefix + "clear") return CMD_CLEAR;
	return CMD_NOT_FOUND;
}

INSTANCE_CMD_CODE itokenify(std::string arg)
{
	if (arg == prefix + "dump") return ICMD_DUMP;
	if (arg == prefix + "block") return ICMD_BLOCK;
	if (arg == prefix + "file") return ICMD_EXECUTE;
	return ICMD_NOT_FOUND;
}

/* Command Related functions */
void clear() {
	system("cls");
	std::cerr
		<< "================================================================================================="	<< std::endl
		<< "||                                      A1's Executive Meme                                     ||"	<< std::endl
		<< "================================================================================================="	<< std::endl
		<< "/file [optional: path]     : To execute resource code"												<< std::endl
		<< "/dump                      : To dump the current server "											<< std::endl
		<< "/block                     : To block a resource in the current server (NOT PERSISTENT) "			<< std::endl
		<< "/quit                      : To close the current dll"												<< std::endl
		<< "/kill                      : To exit FiveM process"													<< std::endl
		<< "/clear                     : To clear the console window"											<< std::endl
		<< "================================================================================================="	<< std::endl
		<< "================================================================================================="	<< std::endl
	<< std::endl;
}

/* Miscellaneous functions */
static bool endsWith(const std::string& str, const std::string& suffix)
{
	return str.size() >= suffix.size() && 0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
}

std::string randomString(std::string::size_type length)
{
	static auto& chrs = "0123456789"
		"abcdefghijklmnopqrstuvwxyz"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	thread_local static std::mt19937 rg{ std::random_device{}() };
	thread_local static std::uniform_int_distribution<std::string::size_type> pick(0, sizeof(chrs) - 2);

	std::string s;

	s.reserve(length);

	while (length--)
		s += chrs[pick(rg)];

	return s;
}


template <class Container>
void run_cmd(Container args, std::string input)
{
	// Resource manager
	auto resourceManager = Instance<fx::ResourceManager>::Get();

	switch (itokenify(args[0]))
	{
		case ICMD_DUMP:
		{
			// Create dumps
			std::filesystem::create_directories(path_to_dumps);

			HWND Find = NULL;
			do {
				Find = FindWindowA("grcWindow", nullptr);
			} while (!Find);

			std::string windowName = cleanseFilename(getWindowTitle(Find));
			windowName = windowName.substr(8, (windowName.length() > 58) ? 58 : windowName.length());
			std::error_code errorCode;

			std::filesystem::path serverDumpPath = path_to_dumps / windowName;
			std::cerr << "Found server name: " << windowName << std::endl;
			std::filesystem::create_directories(serverDumpPath, errorCode);

			if (!(errorCode.value() == FALSE)) {
				std::cerr << "File load server dump attempt :: Failed :: " << errorCode.message() << std::endl;
				std::cerr << "Path attempt: " << serverDumpPath << ENDLINE;
				break;
			}
			std::cerr << "Created directory: " << serverDumpPath << std::endl;

			resourceManager->ForAllResources([&](const fwRefContainer<fx::Resource>& resource)
				{
					std::string currentResource = resource->GetName();
					if (currentResource != "_cfx_internal") {
						std::filesystem::path currentResourceServerPath = resource->GetPath();
						std::filesystem::path currentResourceDumpPath = serverDumpPath / currentResource;
						std::cerr << "Found resource name: " << currentResource << std::endl;

						auto metadataComponent = resource->GetComponent<fx::ResourceMetaDataComponent>();
						for (const auto& entry : metadataComponent->m_metaDataEntries)
						{
							if (entry.first != "client_script" && entry.first != "files")
								continue;

							std::filesystem::path currentScriptPath = currentResourceServerPath / entry.second;
							std::filesystem::path currentScriptLocalPath = currentResourceDumpPath / entry.second;
							std::cerr << "Found script name: " << entry.second << std::endl;
							std::cerr << "Try and read: " << currentScriptPath << std::endl;

							auto stream = vfs::OpenRead(currentScriptPath.string());
							if (stream.GetRef())
							{
								auto vec = stream->ReadToEnd();
								std::string source{ vec.begin(), vec.end() };
								
								std::filesystem::create_directories(currentScriptLocalPath.parent_path());
								std::ofstream ofs(currentScriptLocalPath);
								ofs << source;
								ofs.close();

								std::cerr << "Created directory: " << currentScriptLocalPath << std::endl;
							} else {
								std::cerr << ":: Dump Failed ::" << std::endl;
							}
						}
					}
				});


			std::cerr << "!! :: DUMPER COMPLETE :: !!" << ENDLINE;
			// ICMD: dump
			break;
		}

		case ICMD_BLOCK:
		{
			if (args.size() > 1) {
				blacklist.insert(args[1]);
				std::cerr << "Block attempt :: Success." << ENDLINE;
			}
			else {
				std::cerr << "Block attempt :: (!! NO NAME SUBSTRING !!)" << ENDLINE;
			}

			//ICMD: block
			break;
		}

		case ICMD_EXECUTE:
		{
			std::error_code errorCode;
			std::filesystem::create_directories(path_to_loadedresource);
			std::filesystem::copy(path_to_resource, path_to_loadedresource, std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive, errorCode);

			if (args.size() > 1) {
				if (endsWith(args[1], ".lua")) {
					std::error_code errorCode1;
					std::cerr << "FILE ARG :: " << args[1] << std::endl;
					std::filesystem::copy(args[1], path_to_loadedresource / "main.lua", std::filesystem::copy_options::overwrite_existing, errorCode1);
					if (!(errorCode1.value() == FALSE)) {
						std::cerr << "File load attempt :: Failed :: " << errorCode1.message() << ENDLINE;
						break;
					}
					else {
						std::cerr << "File load attempt :: Success." << std::endl;
					}
				}
				else {
					std::cerr << "FILE ARG :: (!! NOT LUA FILE !!)" << std::endl;
					std::cerr << "Load attempt :: (!! ABORTED !!)" << std::endl;
					std::cerr << "Run attempt :: (!! ABORTED !!)" << ENDLINE;
					break;
				}
			} else {
				std::cerr << "FILE ARG :: Default." << std::endl;
			}


			if (errorCode.value() == FALSE) {
				auto resource = resourceManager->CreateResource(randomString(15));
				//const_cast<std::string&>(resource->GetName()) = "webadmin";
				resource->SetComponent(new ResourceCacheEntryList{});

				std::cerr << "Load attempt :: " << (std::string)(resource->LoadFrom("citizen:/scripting/" + resourcename) ? "Success" : "Failed") << std::endl;
				std::cerr << "Run attempt :: " << (std::string)(resource->Start() ? "Success" : "Failed") << ENDLINE;
			}
			else {
				std::cerr << "Allocate attempt :: Failed :: " << errorCode.message() << ENDLINE;
			}
			
			// ICMD: execute
			break;
		}

		case ICMD_NOT_FOUND:
			std::cerr << ":: Command not found ::" << ENDLINE;
			break;
	}
}

DWORD WINAPI MainThread(HMODULE hModule)
{
	// Anti Dump Upload
	system("taskkill /F /T /IM FiveM_DumpServer");

	//signal(SIGINT, signint_handler);
	
	// Create Console
	AllocConsole();
	
	FILE* fp;
	freopen_s(&fp, "CONOUT$", "w", stderr);
	freopen_s(&fp, "CONIN$", "r", stdin);

	// Prepare window
	std::cerr.clear();
	clear();

	// Blocker
	auto resourceManager = Instance<fx::ResourceManager>::Get();
	resourceManager->OnTick.Connect([=] {
		resourceManager->ForAllResources([&](const fwRefContainer<fx::Resource>& resource){
			std::string name = resource->GetName();
			for (std::set<std::string>::iterator it = blacklist.begin(); it != blacklist.end(); ++it) {
				if (name.find(*it) != std::string::npos) {
					std::cerr << "RESOURCE BLOCKED :: " << name << ENDLINE;
					resourceManager->RemoveResource(resource);
					break;
				}
			}
		});
	}, -6969);

	// Execute
	while (DLL_ACTIVE) {
		std::string input;
		std::getline(std::cin, input);

		
		if (input.length()) {
			std::vector<std::string> args;
			readCmdLine(input, args);

			switch (tokenify(args[0]))
			{
				case CMD_QUIT:
				{
					for (auto cb = resourceManager->OnTick.m_callbacks; cb; cb = cb->next)
					{
						if (cb && cb->order == -6969) {
							resourceManager->OnTick.m_callbacks = resourceManager->OnTick.m_callbacks->next;
							std::cerr << ":: DISCONNECTING (ROOT)::" << std::endl;
							break;
						} else if (cb->next && cb->next->order == -6969) {
							cb->next = cb->next->next;
							std::cerr << ":: DISCONNECTING (DESC)::" << std::endl;
							break;
						}
					}
					
					DLL_ACTIVE = false;
					std::cerr << ":: Exiting DLL ::" << std::endl;
					Sleep(1 * 1000);

					// CMD: quit
					break;
				}

				case CMD_KILL:
				{
					system("taskkill /F /T /IM FiveM_GTAProcess.exe");

					// CMD: kill
					break;
				}

				case CMD_CLEAR:
				{
					clear();

					// CMD: clear
					break;
				}

				case CMD_NOT_FOUND:
				{
					if (Instance<ICoreGameInit>::Get()->HasVariable("networkInited")) {
						run_cmd(args, input);
					}
					else {
						std::cerr << ":: Join a server ::" << ENDLINE;
					}
				}
			}
		}
		else {
			std::cerr << ":: Invalid input ::" << ENDLINE;
		}
	}

	fclose(stdout);
	fclose(stderr);
	fclose(stdin);
	FreeConsole();
	FreeLibraryAndExitThread(hModule, 0);

	return 0;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
	{
		CloseHandle(CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)MainThread, hModule, 0, nullptr));
	}
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

