/*Copyright (c) 2014 The Paradox Game Converters Project

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.*/



#include "WinUtils.h"

#ifdef _WIN32
#include <Windows.h>
#endif
#ifdef __unix__
#include <sys/stat.h>
#include <dirent.h>
#endif

#include "Log.h"


namespace WinUtils {

#ifdef _WIN32

bool TryCreateFolder(const std::string& path)
{
	BOOL success = ::CreateDirectory(path.c_str(), NULL);
	if (success || GetLastError() == 183)	// 183 is if the folder already exists
	{
		return true;
	}
	else
	{
		LOG(LogLevel::Warning) << "Could not create folder " << path << " - " << GetLastWindowsError();
		return false;
	}
}

void GetAllFilesInFolder(const std::string& path, std::set<std::string>& fileNames)
{
	WIN32_FIND_DATA findData;	// the structure to hold the file data
	HANDLE findHandle = FindFirstFile((path + "\\*").c_str(), &findData);	// the results of the file search
	if (findHandle == INVALID_HANDLE_VALUE)
	{
		return;
	}
	do
	{
		if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			fileNames.insert(findData.cFileName);
		}
	} while (FindNextFile(findHandle, &findData) != 0);
	FindClose(findHandle);
}

bool TryCopyFile(const std::string& sourcePath, const std::string& destPath)
{
	BOOL success = ::CopyFile(sourcePath.c_str(), destPath.c_str(), FALSE);	// whether or not the copy succeeded
	if (success)
	{
		return true;
	}
	else
	{
		LOG(LogLevel::Warning) << "Could not copy file " << sourcePath << " to " << destPath << " - " << GetLastWindowsError();
		return false;
	}
}

bool DoesFileExist(const std::string& path)
{
	DWORD attributes = GetFileAttributes(path.c_str());	// the file attributes
	return (attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY));
}

i
bool doesFolderExist(const std::string& path)
{
	DWORD attributes = GetFileAttributes(path.c_str());	// the file attributes
	return (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY));
}


std::string GetLastWindowsError()
{
	DWORD errorCode = ::GetLastError();	// the code for the latest error
	const DWORD errorBufferSize = 256;	// the size of the textbuffer for the error
	CHAR errorBuffer[errorBufferSize];	// the text buffer for the error
	BOOL success = ::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,		// whether or not the error could be formatted
		NULL,
		errorCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		errorBuffer,
		errorBufferSize - 1,
		NULL);
	if (success)
	{
		return errorBuffer;
	}
	else
	{
		return "Unknown error";
	}
}

int DeleteFolder(const std::string &refcstrRootDirectory,
	bool              bDeleteSubdirectories)
{
	bool            bSubdirectory = false;       // Flag, indicating whether
	// subdirectories have been found
	HANDLE          hFile;                       // Handle to directory
	std::string     strFilePath;                 // Filepath
	std::string     strPattern;                  // Pattern
	WIN32_FIND_DATA FileInformation;             // File information


	strPattern = refcstrRootDirectory + "\\*.*";
	hFile = ::FindFirstFile(strPattern.c_str(), &FileInformation);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (FileInformation.cFileName[0] != '.')
			{
				strFilePath.erase();
				strFilePath = refcstrRootDirectory + "\\" + FileInformation.cFileName;

				if (FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					if (bDeleteSubdirectories)
					{
						// Delete subdirectory
						int iRC = DeleteFolder(strFilePath, bDeleteSubdirectories);
						if (iRC)
							return iRC;
					}
					else
						bSubdirectory = true;
				}
				else
				{
					// Set file attributes
					if (::SetFileAttributes(strFilePath.c_str(),
						FILE_ATTRIBUTE_NORMAL) == FALSE)
						return ::GetLastError();

					// Delete file
					if (::DeleteFile(strFilePath.c_str()) == FALSE)
						return ::GetLastError();
				}
			}
		} while (::FindNextFile(hFile, &FileInformation) == TRUE);

		// Close handle
		::FindClose(hFile);

		DWORD dwError = ::GetLastError();
		if (dwError != ERROR_NO_MORE_FILES)
			return dwError;
		else
		{
			if (!bSubdirectory)
			{
				// Set directory attributes
				if (::SetFileAttributes(refcstrRootDirectory.c_str(),
					FILE_ATTRIBUTE_NORMAL) == FALSE)
					return ::GetLastError();

				// Delete directory
				if (::RemoveDirectory(refcstrRootDirectory.c_str()) == FALSE)
					return ::GetLastError();
			}
		}
	}

	return 0;
}

#endif // _WIN32
#ifdef __unix__

bool TryCreateFolder(const std::string& path)
{
	if (doesFolderExist(path))
		return true;
	const int dir_err = mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	if (dir_err == -1)
	{
		LOG(LogLevel::Warning) << "Could not create folder " << path;
		return false;
	}
	return true;
}

void GetAllFilesInFolder(const std::string& path, std::set<std::string>& fileNames)
{
	DIR * dir;
	dirent * ent;
	if ((dir = opendir(path.c_str())) == NULL)
	{
		LOG(LogLevel::Warning) << "Could not read folder " << path;
		return;
	}
	while ((ent = readdir(dir)) != NULL)
	{
		if (doesFolderExist(ent->d_name) == false)
			fileNames.insert(ent->d_name);
	}
	closedir(dir);
}

bool TryCopyFile(const std::string& sourcePath, const std::string& destPath)
{
	FILE * in, * out;
	char ch;
	if ((in = fopen(sourcePath.c_str(), "r")) == NULL)
	{
		LOG(LogLevel::Warning) << "Could not copy file " << sourcePath << " to " << destPath;
		return false;
	}
	if ((out = fopen(destPath.c_str(), "w")) == NULL)
	{
		LOG(LogLevel::Warning) << "Could not copy file " << sourcePath << " to " << destPath;
		return false;
	}
	while((ch = fgetc(in)) != EOF)
	{
		fputc(ch, out);
	}
	fclose(in);
	fclose(out);
	return true;
}

bool DoesFileExist(const std::string& path)
{
	struct stat st;
	if (stat(path.c_str(), &st) == 0)
		return true;
	return false;
}

// BUGGED
bool doesFolderExist(const std::string& path)
{
	struct stat st;
	if (stat(path.c_str(), &st) != -1)
	{
		if(S_ISDIR(st.st_mode))
		{
			return true;
		}
	}
	return false;
}

#endif // __unix__

} // namespace WinUtils
