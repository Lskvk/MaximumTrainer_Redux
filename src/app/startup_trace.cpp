#include "startup_trace.h"

#include <cstdlib>

#ifdef Q_OS_WIN
#include <QDir>
#include <QFileInfo>
#include <windows.h>
#include <dbghelp.h>
#endif

static QString currentStepDescription()
{
    const char *step = StartupStep::current();
    return step ? QLatin1String(step) : QStringLiteral("normal operation (after startup)");
}

static void onTerminate()
{
    QString what = QStringLiteral("terminate");
    if (const std::exception_ptr ex = std::current_exception()) {
        try {
            std::rethrow_exception(ex);
        } catch (const std::exception &e) {
            what = QStringLiteral("unhandled exception (%1)").arg(QString::fromLocal8Bit(e.what()));
        } catch (...) {
            what = QStringLiteral("unhandled non-standard exception");
        }
    }
    Logger::instance().emergencyLog(
        QStringLiteral("fatal: %1 during %2").arg(what, currentStepDescription()));
    std::abort();
}

#ifdef Q_OS_WIN
static DWORD s_mainThreadId = 0;
static std::atomic<bool> s_sehReported { false };

/// "Qt6WebEngineCore.dll+0x1a2b3c" for a code address, so a crash names the
/// DLL that faulted (the GPU driver, an injected hook, a Qt module…).
static QString describeAddress(const void *address)
{
    HMODULE module = nullptr;
    if (!address
        || !GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
                                   | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               static_cast<LPCWSTR>(address), &module))
        return QStringLiteral("0x%1 (no module)")
            .arg(reinterpret_cast<quintptr>(address), 0, 16);

    wchar_t path[MAX_PATH];
    const DWORD len = GetModuleFileNameW(module, path, MAX_PATH);
    const QString name = len ? QFileInfo(QString::fromWCharArray(path, int(len))).fileName()
                             : QStringLiteral("?");
    const quintptr offset = reinterpret_cast<quintptr>(address) - reinterpret_cast<quintptr>(module);
    return QStringLiteral("%1+0x%2").arg(name).arg(offset, 0, 16);
}

/// Writes MaximumTrainer-crash.dmp next to the log (the latest crash only, so
/// repeated crashes do not fill the disk).  Returns the path, or "" on failure.
static QString writeMiniDump(EXCEPTION_POINTERS *info)
{
    using MiniDumpWriteDumpFn = BOOL(WINAPI *)(HANDLE, DWORD, HANDLE, MINIDUMP_TYPE,
                                               PMINIDUMP_EXCEPTION_INFORMATION,
                                               PMINIDUMP_USER_STREAM_INFORMATION,
                                               PMINIDUMP_CALLBACK_INFORMATION);
    HMODULE dbghelp = LoadLibraryW(L"dbghelp.dll");
    if (!dbghelp)
        return QString();
    const auto writeDump = reinterpret_cast<MiniDumpWriteDumpFn>(
        reinterpret_cast<void *>(GetProcAddress(dbghelp, "MiniDumpWriteDump")));
    if (!writeDump)
        return QString();

    const QString logPath = Logger::instance().logFilePath();
    const QString dir = logPath.isEmpty() ? QDir::tempPath() : QFileInfo(logPath).absolutePath();
    const QString dumpPath = QDir::toNativeSeparators(dir + QStringLiteral("/MaximumTrainer-crash.dmp"));

    HANDLE file = CreateFileW(reinterpret_cast<LPCWSTR>(dumpPath.utf16()), GENERIC_WRITE, 0,
                              nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return QString();

    MINIDUMP_EXCEPTION_INFORMATION exceptionInfo;
    exceptionInfo.ThreadId = GetCurrentThreadId();
    exceptionInfo.ExceptionPointers = info;
    exceptionInfo.ClientPointers = FALSE;
    const BOOL ok = writeDump(GetCurrentProcess(), GetCurrentProcessId(), file,
                              static_cast<MINIDUMP_TYPE>(MiniDumpNormal | MiniDumpWithThreadInfo),
                              info ? &exceptionInfo : nullptr, nullptr, nullptr);
    CloseHandle(file);
    return ok ? dumpPath : QString();
}

static LONG WINAPI onUnhandledSehException(EXCEPTION_POINTERS *info)
{
    if (s_sehReported.exchange(true))
        return EXCEPTION_CONTINUE_SEARCH;

    const EXCEPTION_RECORD *record = info ? info->ExceptionRecord : nullptr;
    const DWORD code = record ? record->ExceptionCode : 0;
    Logger::instance().emergencyLog(
        QStringLiteral("fatal: unhandled exception 0x%1 during %2")
            .arg(static_cast<qulonglong>(code), 8, 16, QLatin1Char('0'))
            .arg(currentStepDescription()));

    if (record) {
        QString detail = QStringLiteral("fault at %1 on %2 thread %3")
            .arg(describeAddress(record->ExceptionAddress),
                 GetCurrentThreadId() == s_mainThreadId ? QStringLiteral("main")
                                                        : QStringLiteral("worker"))
            .arg(GetCurrentThreadId());
        if (code == EXCEPTION_ACCESS_VIOLATION && record->NumberParameters >= 2) {
            const ULONG_PTR kind = record->ExceptionInformation[0];
            detail += QStringLiteral(" (%1 of address 0x%2)")
                .arg(kind == 1 ? QStringLiteral("write") : kind == 8 ? QStringLiteral("execute")
                                                                     : QStringLiteral("read"))
                .arg(static_cast<qulonglong>(record->ExceptionInformation[1]), 0, 16);
        }
        Logger::instance().emergencyLog(detail);
    }

    const QString dump = writeMiniDump(info);
    Logger::instance().emergencyLog(dump.isEmpty()
        ? QStringLiteral("crash dump could not be written")
        : QStringLiteral("crash dump written to %1").arg(dump));
    return EXCEPTION_CONTINUE_SEARCH;
}
#endif

void StartupTrace::installCrashHandlers()
{
    std::set_terminate(onTerminate);
#ifdef Q_OS_WIN
    s_mainThreadId = GetCurrentThreadId();
    SetUnhandledExceptionFilter(onUnhandledSehException);
#endif
}
