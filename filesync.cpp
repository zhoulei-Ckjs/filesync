#include <iostream>
#include <string>
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#include <filesystem>
#include <chrono>
#include <thread>


std::string ws2s(const std::wstring& wstr) 
{
    int size_needed = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(),
        (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string str(size_needed, 0);
    WideCharToMultiByte(CP_ACP, 0, wstr.c_str(),
        (int)wstr.size(), &str[0], size_needed, NULL, NULL);
    return str;
}

int main()
{
    std::wstring src_dir = L"E:\\文档\\";
    std::wstring dest_dir = L"E:\\backup\\";
    HANDLE src_handle = CreateFileW(src_dir.c_str(), FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (src_handle == INVALID_HANDLE_VALUE)
    {
        std::cerr << "无法打开目录: " << ws2s(src_dir) << std::endl;
        return -1;
    }
    std::cout << "正在监控目录：" << ws2s(src_dir) << std::endl;

    char buffer[1024];
    DWORD bytesReturned;

    while (true)
    {
        if (ReadDirectoryChangesW(
            src_handle,
            &buffer,
            sizeof(buffer),
            TRUE,  // 是否递归监控子目录
            FILE_NOTIFY_CHANGE_FILE_NAME |
            FILE_NOTIFY_CHANGE_DIR_NAME |
            FILE_NOTIFY_CHANGE_SIZE |
            FILE_NOTIFY_CHANGE_LAST_WRITE,
            &bytesReturned,
            NULL,
            NULL))
        {
            FILE_NOTIFY_INFORMATION* pNotify;
            size_t offset = 0;

            do {
                pNotify = (FILE_NOTIFY_INFORMATION*)((char*)buffer + offset);

                std::wstring fileName(pNotify->FileName,
                    pNotify->FileNameLength / sizeof(WCHAR));

                switch (pNotify->Action) 
                {
                case FILE_ACTION_ADDED:
                    std::cout << "[新增] " << ws2s(fileName) << std::endl;
                    try
                    {
                        std::this_thread::sleep_for(std::chrono::seconds(1));   ///< 原进程占用了文件，睡眠等待文件解放。
                        std::filesystem::copy_file(src_dir + fileName, dest_dir + fileName, std::filesystem::copy_options::overwrite_existing);
                    }
                    catch (std::filesystem::filesystem_error e)
                    {
                        std::wcout << e.what() << std::endl;
                    }
                    catch (const std::exception& e)
                    {
                        std::wcout << e.what() << std::endl;
                    }
                    
                    break;
                case FILE_ACTION_REMOVED:
                    std::cout << "[删除] " << ws2s(fileName) << std::endl;
                    break;
                case FILE_ACTION_MODIFIED:
                    std::cout << "[修改] " << ws2s(fileName) << std::endl;
                    break;
                case FILE_ACTION_RENAMED_OLD_NAME:
                    std::cout << "[重命名-旧名字] " << ws2s(fileName) << std::endl;
                    break;
                case FILE_ACTION_RENAMED_NEW_NAME:
                    std::cout << "[重命名-新名字] " << ws2s(fileName) << std::endl;
                    break;
                default:
                    std::cout << "[未知操作] " << ws2s(fileName) << std::endl;
                }

                offset += pNotify->NextEntryOffset;
            } while (pNotify->NextEntryOffset != 0);
        }
        else 
        {
            std::cerr << "ReadDirectoryChangesW 调用失败！" << std::endl;
            break;
        }
    }
    return 0;
}
