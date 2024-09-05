#include <crescent/common.h>
#include <crescent/asm/segment.h>
#include <crescent/core/panic.h>
#include <crescent/core/int/idt.h>
#include <crescent/lib/string.h>
#include "isr.h"

struct idt_entry {
    u16 handler_low;
    u16 cs;
    u8 ist;
    u8 flags;
    u16 handler_mid;
    u32 handler_high;
    u32 reserved;
} __attribute__((packed));

__attribute__((aligned(16)))
static struct idt_entry idt_entries[256] = { 0 };
static void (*const isr_table[256])(void) = {
    _isr_0, _isr_1, _isr_2, _isr_3, _isr_4, _isr_5, _isr_6, _isr_7, _isr_8, _isr_9, _isr_10,
    _isr_11, _isr_12, _isr_13, _isr_14, _isr_15, _isr_16, _isr_17, _isr_18, _isr_19, _isr_20,
    _isr_21, _isr_22, _isr_23, _isr_24, _isr_25, _isr_26, _isr_27, _isr_28, _isr_29, _isr_30,
    _isr_31, _isr_32, _isr_33, _isr_34, _isr_35, _isr_36, _isr_37, _isr_38, _isr_39, _isr_40,
    _isr_41, _isr_42, _isr_43, _isr_44, _isr_45, _isr_46, _isr_47, _isr_48, _isr_49, _isr_50,
    _isr_51, _isr_52, _isr_53, _isr_54, _isr_55, _isr_56, _isr_57, _isr_58, _isr_59, _isr_60,
    _isr_61, _isr_62, _isr_63, _isr_64, _isr_65, _isr_66, _isr_67, _isr_68, _isr_69, _isr_70,
    _isr_71, _isr_72, _isr_73, _isr_74, _isr_75, _isr_76, _isr_77, _isr_78, _isr_79, _isr_80,
    _isr_81, _isr_82, _isr_83, _isr_84, _isr_85, _isr_86, _isr_87, _isr_88, _isr_89, _isr_90,
    _isr_91, _isr_92, _isr_93, _isr_94, _isr_95, _isr_96, _isr_97, _isr_98, _isr_99, _isr_100,
    _isr_101, _isr_102, _isr_103, _isr_104, _isr_105, _isr_106, _isr_107, _isr_108, _isr_109, _isr_110,
    _isr_111, _isr_112, _isr_113, _isr_114, _isr_115, _isr_116, _isr_117, _isr_118, _isr_119, _isr_120,
    _isr_121, _isr_122, _isr_123, _isr_124, _isr_125, _isr_126, _isr_127, _isr_128, _isr_129, _isr_130,
    _isr_131, _isr_132, _isr_133, _isr_134, _isr_135, _isr_136, _isr_137, _isr_138, _isr_139, _isr_140,
    _isr_141, _isr_142, _isr_143, _isr_144, _isr_145, _isr_146, _isr_147, _isr_148, _isr_149, _isr_150,
    _isr_151, _isr_152, _isr_153, _isr_154, _isr_155, _isr_156, _isr_157, _isr_158, _isr_159, _isr_160,
    _isr_161, _isr_162, _isr_163, _isr_164, _isr_165, _isr_166, _isr_167, _isr_168, _isr_169, _isr_170,
    _isr_171, _isr_172, _isr_173, _isr_174, _isr_175, _isr_176, _isr_177, _isr_178, _isr_179, _isr_180,
    _isr_181, _isr_182, _isr_183, _isr_184, _isr_185, _isr_186, _isr_187, _isr_188, _isr_189, _isr_190,
    _isr_191, _isr_192, _isr_193, _isr_194, _isr_195, _isr_196, _isr_197, _isr_198, _isr_199, _isr_200,
    _isr_201, _isr_202, _isr_203, _isr_204, _isr_205, _isr_206, _isr_207, _isr_208, _isr_209, _isr_210,
    _isr_211, _isr_212, _isr_213, _isr_214, _isr_215, _isr_216, _isr_217, _isr_218, _isr_219, _isr_220,
    _isr_221, _isr_222, _isr_223, _isr_224, _isr_225, _isr_226, _isr_227, _isr_228, _isr_229, _isr_230,
    _isr_231, _isr_232, _isr_233, _isr_234, _isr_235, _isr_236, _isr_237, _isr_238, _isr_239, _isr_240,
    _isr_241, _isr_242, _isr_243, _isr_244, _isr_245, _isr_246, _isr_247, _isr_248, _isr_249, _isr_250,
    _isr_251, _isr_252, _isr_253, _isr_254, _isr_255
};

static bool initialized = false;

static inline void idt_load(void) {
    struct {
        u16 limit;
        struct idt_entry* idt_ptr;
    } __attribute__((packed)) idtr = {
        .limit = sizeof(idt_entries) - 1,
        .idt_ptr = idt_entries
    };

    __asm__ volatile("lidt %0" : : "m"(idtr) : "memory");
}

void idt_init(void) {
    memset(idt_entries, 0, sizeof(idt_entries));

    if (!initialized) {
        for (size_t i = 0; i < ARRAY_SIZE(isr_table); i++) {
            idt_entries[i].handler_low = (uintptr_t)isr_table[i] & 0xFFFF;
            idt_entries[i].cs = 0x08;
            idt_entries[i].ist = i == 18 ? 1 : i == 8 ? 2 : i == 2 ? 3 : 0;
            idt_entries[i].flags = 0x8e;
            idt_entries[i].handler_mid = ((uintptr_t)isr_table[i] >> 16) & 0xFFFF;
            idt_entries[i].handler_high = (uintptr_t)isr_table[i] >> 32;
            idt_entries[i].reserved = 0;
        }

        initialized = true;
    }

    idt_load();
}
