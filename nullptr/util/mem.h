#pragma once
#include <cstdint>
#include <Windows.h>

class Memory {
public:
    inline static uintptr_t client = 0x0;

    template<typename T>
    static T Read(uintptr_t address) {
        if (!IsValidAddress(address)) {
            return T{};
        }

        __try {
            return *reinterpret_cast<T*>(address);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return T{};
        }
    }

    template<typename T>
    static bool Write(uintptr_t address, const T& value) {
        if (!IsValidAddress(address)) {
            return false;
        }

        __try {
            *reinterpret_cast<T*>(address) = value;
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    template <typename T, typename... Args>
    static T CallVFunc(std::uintptr_t instance, int index, Args... args) {
        using Fn = T(__thiscall*)(void*, Args...);
        void* vfunc = (*reinterpret_cast<void***>(instance))[index];
        return reinterpret_cast<Fn>(vfunc)((void*)instance, args...);
    }

    static void loadModules() {
        client = reinterpret_cast<uintptr_t>(GetModuleHandleA("client.dll"));
    }

private:
    static bool IsValidAddress(uintptr_t address) {
        if (address == 0 || address == (uintptr_t)nullptr) {
            return false;
        }

        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(reinterpret_cast<LPCVOID>(address), &mbi, sizeof(mbi))) {
            return (mbi.Protect & (PAGE_READWRITE | PAGE_EXECUTE_READWRITE | PAGE_READONLY)) &&
                !(mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD));
        }
        return false;
    }
};

inline Memory mem;