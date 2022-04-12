// OpenFileIntoVirtualMemory.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <windows.h>
#include <atlbase.h>
#include <memory>
#include <stdio.h>

#define THROW_FILE_ERROR(message) {throw file_exception(message, __FILE__, __FUNCTION__ , __LINE__);} 

class file_exception : public  std::exception
{
public:
	file_exception(const char* message, const char* file, const char* function, int line) noexcept : errorMsg{}
	{
		sprintf_s(errorMsg, 256, "%s %s %d %s", file, function, line, message);
	}

	const char* what() const noexcept override
	{
		return errorMsg;
	}
private:
	char errorMsg[256];
};

// custom deleter for unique_ptr
static inline void deleter(void* tmpPtr) {
	UnmapViewOfFile(tmpPtr);
}

class MappingFile {
public:
	MappingFile(LPCWSTR pathFile) 
		: m_lpMap(nullptr, &deleter)
		, m_liSize{ 0,0 }
	{
		// open file
		ATL::CHandle FileHandle(CreateFile(pathFile, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0));
		if (INVALID_HANDLE_VALUE == FileHandle)	{
			THROW_FILE_ERROR("Cannot open file");
		}
		// get file size
		if (!GetFileSizeEx(FileHandle, &m_liSize)) {
			THROW_FILE_ERROR("Can not get size of file");
		}
		//open a file mapping object for a specified file
		HANDLE tmpH = CreateFileMapping(FileHandle, nullptr, PAGE_READONLY, 0, 0, nullptr);
		if (INVALID_HANDLE_VALUE == tmpH || nullptr == tmpH) {
			THROW_FILE_ERROR("Can not open file mapping object");
		}

		m_hFile.Attach(tmpH);

		// maps a view of a file mapping into the adress space of a calling process
		std::unique_ptr<void, void(*)(void*)> tmpPtr(MapViewOfFile(m_hFile, FILE_MAP_READ, 0, 0, 0), &deleter);
		m_lpMap = std::move(tmpPtr);

		if (nullptr == m_lpMap)	{
			THROW_FILE_ERROR("Can not map view of file");
		}		
	}

	// get pointer to the beginning of file
	const void * const BeginFile() const noexcept {
		return m_lpMap.get();
	}

	size_t SizeOfFile() const noexcept {
		return static_cast<size_t>(m_liSize.QuadPart);
	}

	~MappingFile() {}
private:
	std::unique_ptr<void, void(*)(void*)>  m_lpMap;
	LARGE_INTEGER m_liSize;
	ATL::CHandle m_hFile;
};

int main()
{
	try {
		MappingFile file(L"D:\\111.txt");

		auto pos = reinterpret_cast<const char *>(file.BeginFile());
		
		for (auto i = 0u; i < file.SizeOfFile(); ++i)
			printf("%c", pos[i]);
	} 
	catch (const std::exception &e) {
		printf_s("%s", e.what());
	}

	return 0;
}

