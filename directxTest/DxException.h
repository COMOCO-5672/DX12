#ifndef __DXEXCEPTION__
#define __DXEXCEPTION__

#include <string>
#include <comdef.h>
#include <d3d12.h>
#include <Windows.h>

inline std::wstring AnsiToWString(const std::string& str)
{
	WCHAR buffer[512];
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
	return std::wstring(buffer);
}


class DxException {
public:
	DxException() = default;
	DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& fileName, int lineNumber);

	std::wstring ToString()const;

	HRESULT ErrorCode = S_OK;
	std::wstring FunctionName;
	std::wstring FileName;

	int LineNumber = -1;
};

inline  DxException::DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber) :
	ErrorCode(hr),
	FunctionName(functionName),
	FileName(filename),
	LineNumber(lineNumber)
{

}

inline std::wstring DxException::ToString()const
{
	_com_error err(ErrorCode);
	std::wstring msg = err.ErrorMessage();

	return FunctionName + L" failed in " + FileName + L"; line" + std::to_wstring(LineNumber) + L"; error: " + msg;
}

#endif
