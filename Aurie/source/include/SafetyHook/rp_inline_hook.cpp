#include <iterator>

#if __has_include("Zydis/Zydis.h")
#include "Zydis/Zydis.h"
#elif __has_include("Zydis.h")
#include "Zydis.h"
#else
#error "Zydis not found"
#endif

#include "safetyhook/allocator.hpp"
#include "safetyhook/common.hpp"
#include "safetyhook/os.hpp"
#include "safetyhook/utility.hpp"

#include "safetyhook/rp_inline_hook.hpp"

namespace safetyhook {

#pragma pack(push, 1)
struct JmpE9 {
    uint8_t opcode{0xE9};
    uint32_t offset{0};
};

#if SAFETYHOOK_ARCH_X86_64
struct JmpFF {
    uint8_t opcode0{0xFF};
    uint8_t opcode1{0x25};
    uint32_t offset{0};
};

struct TrampolineEpilogueE9RP {
    JmpE9 jmp_to_original{};
    uint8_t shellcode[0x58]{};
    JmpFF jmp_to_destination{};
    uint64_t destination_address{};
};

#elif SAFETYHOOK_ARCH_X86_32

struct TrampolineEpilogueE9 {
    JmpE9 jmp_to_original{};
    JmpE9 jmp_to_destination{};
};

struct TrampolineEpilogueE9RP {
    JmpE9 jmp_to_original{};
    uint8_t shellcode[0x26]{};
    JmpE9 jmp_to_destination{};
};
#endif
#pragma pack(pop)

#if SAFETYHOOK_ARCH_X86_64
static auto make_jmp_ff(uint8_t* src, uint8_t* dst, uint8_t* data) {
    JmpFF jmp{};

    jmp.offset = static_cast<uint32_t>(data - src - sizeof(jmp));
    store(data, dst);

    return jmp;
}

[[nodiscard]] static std::expected<void, RpInlineHook::Error> emit_jmp_ff(
    uint8_t* src, uint8_t* dst, uint8_t* data, size_t size = sizeof(JmpFF)) {
    if (size < sizeof(JmpFF)) {
        return std::unexpected{RpInlineHook::Error::not_enough_space(dst)};
    }

    if (size > sizeof(JmpFF)) {
        std::fill_n(src, size, static_cast<uint8_t>(0x90));
    }

    store(src, make_jmp_ff(src, dst, data));

    return {};
}
#endif

constexpr auto make_jmp_e9(uint8_t* src, uint8_t* dst) {
    JmpE9 jmp{};

    jmp.offset = static_cast<uint32_t>(dst - src - sizeof(jmp));

    return jmp;
}

[[nodiscard]] static std::expected<void, RpInlineHook::Error> emit_jmp_e9(
    uint8_t* src, uint8_t* dst, size_t size = sizeof(JmpE9)) {
    if (size < sizeof(JmpE9)) {
        return std::unexpected{RpInlineHook::Error::not_enough_space(dst)};
    }

    if (size > sizeof(JmpE9)) {
        std::fill_n(src, size, static_cast<uint8_t>(0x90));
    }

    store(src, make_jmp_e9(src, dst));

    return {};
}

static bool decode(ZydisDecodedInstruction* ix, uint8_t* ip) {
    ZydisDecoder decoder{};
    ZyanStatus status;

#if SAFETYHOOK_ARCH_X86_64
    status = ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);
#elif SAFETYHOOK_ARCH_X86_32
    status = ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LEGACY_32, ZYDIS_STACK_WIDTH_32);
#endif

    if (!ZYAN_SUCCESS(status)) {
        return false;
    }

    return ZYAN_SUCCESS(ZydisDecoderDecodeInstruction(&decoder, nullptr, ip, 15, ix));
}

std::expected<RpInlineHook, RpInlineHook::Error> RpInlineHook::create(void* target, void* destination, Flags flags) {
    return create(Allocator::global(), target, destination, flags);
}

std::expected<RpInlineHook, RpInlineHook::Error> RpInlineHook::create(
    const std::shared_ptr<Allocator>& allocator, void* target, void* destination, Flags flags) {
    RpInlineHook hook{};

    if (const auto setup_result =
            hook.setup(allocator, reinterpret_cast<uint8_t*>(target), reinterpret_cast<uint8_t*>(destination));
        !setup_result) {
        return std::unexpected{setup_result.error()};
    }

    if (!(flags & StartDisabled)) {
        if (auto enable_result = hook.enable(); !enable_result) {
            return std::unexpected{enable_result.error()};
        }
    }

    return hook;
}

RpInlineHook::RpInlineHook(RpInlineHook&& other) noexcept {
    *this = std::move(other);
}

RpInlineHook& RpInlineHook::operator=(RpInlineHook&& other) noexcept {
    if (this != &other) {
        destroy();

        std::scoped_lock lock{m_mutex, other.m_mutex};

        m_target = other.m_target;
        m_destination = other.m_destination;
        m_trampoline = std::move(other.m_trampoline);
        m_trampoline_size = other.m_trampoline_size;
        m_original_bytes = std::move(other.m_original_bytes);
        m_enabled = other.m_enabled;
        m_type = other.m_type;
        m_register_state = std::move(other.m_register_state);

        other.m_target = nullptr;
        other.m_destination = nullptr;
        other.m_trampoline_size = 0;
        other.m_enabled = false;
        other.m_type = Type::Unset;
    }

    return *this;
}

RpInlineHook::~RpInlineHook() {
    destroy();
}

void RpInlineHook::reset() {
    *this = {};
}

std::expected<void, RpInlineHook::Error> RpInlineHook::setup(
    const std::shared_ptr<Allocator>& allocator, uint8_t* target, uint8_t* destination) {
    m_target = target;
    m_destination = destination;

    auto register_context_allocation = allocator->allocate(sizeof(RegisterContext));
    if (!register_context_allocation) {
        return std::unexpected{ Error::bad_allocation(register_context_allocation.error()) };
    }

    m_register_state = std::move(*register_context_allocation);

    if (auto e9_result = e9_hook(allocator); !e9_result) {
#if SAFETYHOOK_ARCH_X86_32
        return e9_result;
#endif
    }

    return {};
}

std::expected<void, RpInlineHook::Error> RpInlineHook::e9_hook(const std::shared_ptr<Allocator>& allocator) {
    m_original_bytes.clear();
    m_trampoline_size = sizeof(TrampolineEpilogueE9RP);

    std::vector<uint8_t*> desired_addresses{m_target};
    ZydisDecodedInstruction ix{};

    for (auto ip = m_target; ip < m_target + sizeof(JmpE9); ip += ix.length) {
        if (!decode(&ix, ip)) {
            return std::unexpected{Error::failed_to_decode_instruction(ip)};
        }

        m_trampoline_size += ix.length;
        m_original_bytes.insert(m_original_bytes.end(), ip, ip + ix.length);

        const auto is_relative = (ix.attributes & ZYDIS_ATTRIB_IS_RELATIVE) != 0;

        if (is_relative) {
            if (ix.raw.disp.size == 32) {
                const auto target_address = ip + ix.length + static_cast<int32_t>(ix.raw.disp.value);
                desired_addresses.emplace_back(target_address);
            } else if (ix.raw.imm[0].size == 32) {
                const auto target_address = ip + ix.length + static_cast<int32_t>(ix.raw.imm[0].value.s);
                desired_addresses.emplace_back(target_address);
            } else if (ix.meta.category == ZYDIS_CATEGORY_COND_BR && ix.meta.branch_type == ZYDIS_BRANCH_TYPE_SHORT) {
                const auto target_address = ip + ix.length + static_cast<int32_t>(ix.raw.imm[0].value.s);
                desired_addresses.emplace_back(target_address);
                m_trampoline_size += 4; // near conditional branches are 4 bytes larger.
            } else if (ix.meta.category == ZYDIS_CATEGORY_UNCOND_BR && ix.meta.branch_type == ZYDIS_BRANCH_TYPE_SHORT) {
                const auto target_address = ip + ix.length + static_cast<int32_t>(ix.raw.imm[0].value.s);
                desired_addresses.emplace_back(target_address);
                m_trampoline_size += 3; // near unconditional branches are 3 bytes larger.
            } else {
                return std::unexpected{Error::unsupported_instruction_in_trampoline(ip)};
            }
        }
    }

    auto trampoline_allocation = allocator->allocate_near(desired_addresses, m_trampoline_size);

    if (!trampoline_allocation) {
        return std::unexpected{Error::bad_allocation(trampoline_allocation.error())};
    }

    m_trampoline = std::move(*trampoline_allocation);

    for (auto ip = m_target, tramp_ip = m_trampoline.data(); ip < m_target + m_original_bytes.size(); ip += ix.length) {
        if (!decode(&ix, ip)) {
            m_trampoline.free();
            return std::unexpected{Error::failed_to_decode_instruction(ip)};
        }

        const auto is_relative = (ix.attributes & ZYDIS_ATTRIB_IS_RELATIVE) != 0;

        if (is_relative && ix.raw.disp.size == 32) {
            std::copy_n(ip, ix.length, tramp_ip);
            const auto target_address = ip + ix.length + ix.raw.disp.value;
            const auto new_disp = target_address - (tramp_ip + ix.length);
            store(tramp_ip + ix.raw.disp.offset, static_cast<int32_t>(new_disp));
            tramp_ip += ix.length;
        } else if (is_relative && ix.raw.imm[0].size == 32) {
            std::copy_n(ip, ix.length, tramp_ip);
            const auto target_address = ip + ix.length + ix.raw.imm[0].value.s;
            const auto new_disp = target_address - (tramp_ip + ix.length);
            store(tramp_ip + ix.raw.imm[0].offset, static_cast<int32_t>(new_disp));
            tramp_ip += ix.length;
        } else if (ix.meta.category == ZYDIS_CATEGORY_COND_BR && ix.meta.branch_type == ZYDIS_BRANCH_TYPE_SHORT) {
            const auto target_address = ip + ix.length + ix.raw.imm[0].value.s;
            auto new_disp = target_address - (tramp_ip + 6);

            // Handle the case where the target is now in the trampoline.
            if (target_address >= m_target && target_address < m_target + m_original_bytes.size()) {
                new_disp = static_cast<ptrdiff_t>(ix.raw.imm[0].value.s);
            }

            *tramp_ip = 0x0F;
            *(tramp_ip + 1) = 0x10 + ix.opcode;
            store(tramp_ip + 2, static_cast<int32_t>(new_disp));
            tramp_ip += 6;
        } else if (ix.meta.category == ZYDIS_CATEGORY_UNCOND_BR && ix.meta.branch_type == ZYDIS_BRANCH_TYPE_SHORT) {
            const auto target_address = ip + ix.length + ix.raw.imm[0].value.s;
            auto new_disp = target_address - (tramp_ip + 5);

            // Handle the case where the target is now in the trampoline.
            if (target_address >= m_target && target_address < m_target + m_original_bytes.size()) {
                new_disp = static_cast<ptrdiff_t>(ix.raw.imm[0].value.s);
            }

            *tramp_ip = 0xE9;
            store(tramp_ip + 1, static_cast<int32_t>(new_disp));
            tramp_ip += 5;
        } else {
            std::copy_n(ip, ix.length, tramp_ip);
            tramp_ip += ix.length;
        }
    }

    auto trampoline_epilogue = reinterpret_cast<TrampolineEpilogueE9RP*>(
        m_trampoline.address() + m_trampoline_size - sizeof(TrampolineEpilogueE9RP));

    // jmp from trampoline to original.
    auto src = reinterpret_cast<uint8_t*>(&trampoline_epilogue->jmp_to_original);
    auto dst = m_target + m_original_bytes.size();

    if (auto result = emit_jmp_e9(src, dst); !result) {
        return std::unexpected{result.error()};
    }

    // jmp from trampoline to destination.
    src = reinterpret_cast<uint8_t*>(&trampoline_epilogue->jmp_to_destination);
    dst = m_destination;

    if (auto result = write_rp_code(&trampoline_epilogue->shellcode); !result) {
        return std::unexpected{ result.error() };
    }

#if SAFETYHOOK_ARCH_X86_64
    auto data = reinterpret_cast<uint8_t*>(&trampoline_epilogue->destination_address);

    if (auto result = emit_jmp_ff(src, dst, data); !result) {
        return std::unexpected{result.error()};
    }
#elif SAFETYHOOK_ARCH_X86_32
    if (auto result = emit_jmp_e9(src, dst); !result) {
        return std::unexpected{result.error()};
    }
#endif

    m_type = Type::E9;

    return {};
}

std::expected<void, RpInlineHook::Error> RpInlineHook::write_rp_code(void* data)
{
#if SAFETYHOOK_ARCH_X86_32
    /*
        push    ebx
        push    eax
        mov     eax, dword ptr [0x11223344]
        mov     dword ptr [eax + 0x04], ebx
        lea     ebx, [esp + 8]
        mov     dword ptr [eax + 0x18], ebx
        mov     dword ptr [eax + 0x08], ecx
        mov     dword ptr [eax + 0x0C], edx
        mov     dword ptr [eax + 0x10], esi
        mov     dword ptr [eax + 0x14], edi
        mov     dword ptr [eax + 0x1C], ebp
        mov     ebx, eax
        pop     eax
        mov     dword ptr [ebx + 0x00], eax
        pop     ebx
    */

    const char shellcode[] =
        "\x53"
        "\x50"
        "\xA1\x44\x33\x22\x11"
        "\x89\x58\x04"
        "\x8D\x5C\x24\x08"
        "\x89\x58\x18"
        "\x89\x48\x08"
        "\x89\x50\x0C"
        "\x89\x70\x10"
        "\x89\x78\x14"
        "\x89\x68\x1C"
        "\x89\xC3"
        "\x58"
        "\x89\x03"
        "\x5B";

#elif SAFETYHOOK_ARCH_X86_64
    /*
        push rbx
        push rax
        sub rsp, 16
        movabs rax, 0x1122334455667788
        mov qword ptr [rax + 0x08], rbx
        mov qword ptr [rax + 0x10], rcx
        mov qword ptr [rax + 0x18], rdx
        mov qword ptr [rax + 0x20], rsi
        mov qword ptr [rax + 0x28], rdi
        mov qword ptr [rax + 0x30], rsp
        mov qword ptr [rax + 0x38], rbp
        mov qword ptr [rax + 0x40], r8
        mov qword ptr [rax + 0x48], r9
        mov qword ptr [rax + 0x50], r10
        mov qword ptr [rax + 0x58], r11
        mov qword ptr [rax + 0x60], r12
        mov qword ptr [rax + 0x68], r13
        mov qword ptr [rax + 0x70], r14
        mov qword ptr [rax + 0x78], r15
        add rsp, 16
        mov rbx, rax
        pop rax
        mov qword ptr [rbx + 0x00], rax
        pop rbx
    */

    const char shellcode[] =
        "\x53"
        "\x50"
        "\x48\x83\xEC\x10"
        "\x48\xB8\x88\x77\x66\x55\x44\x33\x22\x11"
        "\x48\x89\x58\x08"
        "\x48\x89\x48\x10"
        "\x48\x89\x50\x18"
        "\x48\x89\x70\x20"
        "\x48\x89\x78\x28"
        "\x48\x89\x60\x30"
        "\x48\x89\x68\x38"
        "\x4C\x89\x40\x40"
        "\x4C\x89\x48\x48"
        "\x4C\x89\x50\x50"
        "\x4C\x89\x58\x58"
        "\x4C\x89\x60\x60"
        "\x4C\x89\x68\x68"
        "\x4C\x89\x70\x70"
        "\x4C\x89\x78\x78"
        "\x48\x83\xC4\x10"
        "\x48\x89\xC3"
        "\x58"
        "\x48\x89\x03"
        "\x5B";

#endif
    auto register_buffer = m_register_state.data();

    memcpy(data, shellcode, sizeof(shellcode) - 1);
    memcpy(static_cast<char*>(data) + 8, &register_buffer, sizeof(void*));

    return {};
}

std::expected<void, RpInlineHook::Error> RpInlineHook::enable() {
    std::scoped_lock lock{m_mutex};

    if (m_enabled) {
        return {};
    }

    std::optional<Error> error;

    // jmp from original to trampoline.
    trap_threads(m_target, m_trampoline.data(), m_original_bytes.size(), [this, &error] {
        if (m_type == Type::E9) {
            auto trampoline_epilogue = reinterpret_cast<TrampolineEpilogueE9RP*>(
                m_trampoline.address() + m_trampoline_size - sizeof(TrampolineEpilogueE9RP));

            if (auto result = emit_jmp_e9(m_target,
                    reinterpret_cast<uint8_t*>(&trampoline_epilogue->shellcode), m_original_bytes.size());
                !result) {
                error = result.error();
            }
        }
    });

    if (error) {
        return std::unexpected{*error};
    }

    m_enabled = true;

    return {};
}

std::expected<void, RpInlineHook::Error> RpInlineHook::disable() {
    std::scoped_lock lock{m_mutex};

    if (!m_enabled) {
        return {};
    }

    trap_threads(m_trampoline.data(), m_target, m_original_bytes.size(),
        [this] { std::copy(m_original_bytes.begin(), m_original_bytes.end(), m_target); });

    m_enabled = false;

    return {};
}

const RpInlineHook::RegisterContext& RpInlineHook::register_state() const
{
    return *reinterpret_cast<RegisterContext*>(this->m_register_state.address());
}

void RpInlineHook::destroy() {
    [[maybe_unused]] auto disable_result = disable();

    std::scoped_lock lock{m_mutex};

    if (m_register_state)
        m_register_state.free();

    if (m_trampoline)
        m_trampoline.free();
}
} // namespace safetyhook
