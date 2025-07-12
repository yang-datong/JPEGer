#if defined(__APPLE__) && defined(__MACH__)
    #define C_SYMBOL_NAME(name) _##name
#else
    #define C_SYMBOL_NAME(name) name
#endif

.arch armv8-a

// MCU::startIDCT_neon 函数实现
// 移除了堆栈保护相关代码。
// 使用了 PIC (Position-Independent Code) 安全的地址加载方式。

.data
.align 4 

constant_pi_div_16_related:
    .double 0.19634954084936207 // M_PI / 16.0 的近似值

constant_vector_1:
    .quad 0, 0 // 未知常量的占位符，填充为0

constant_vector_v7:
    .quad 0x400921fb54442d18, 0x400921fb54442d18

constant_vector_v16:
    .quad 0x3fb0000000000000, 0x3fb0000000000000

.text
.global C_SYMBOL_NAME(startIDCT_asm)
.align 4

C_SYMBOL_NAME(startIDCT_asm):
    // --- 函数序言 (Function Prologue) ---
    stp    d15, d14, [sp, #-0x90]!
    stp    d13, d12, [sp, #0x10]
    stp    d11, d10, [sp, #0x20]
    stp    d9, d8, [sp, #0x30]
    stp    x26, x25, [sp, #0x40]
    stp    x24, x23, [sp, #0x50]
    stp    x22, x21, [sp, #0x60]
    stp    x20, x19, [sp, #0x70]
    stp    x29, x30, [sp, #0x80]
    add    x29, sp, #0x80
    sub    sp, sp, #0x350

    // --- 初始化 ---
    mov    x19, x0                 // 保存 this 指针
    mov    x20, #0x0               // 初始化最外层循环计数器
    add    x21, x0, #0x300

    // --- 加载常量 (PIC-safe) ---
    adrp   x8, constant_pi_div_16_related@PAGE
    add    x8, x8, constant_pi_div_16_related@PAGEOFF
    ldr    d0, [x8]
    str    d0, [sp, #0x18]

    adrp   x8, constant_vector_v7@PAGE
    add    x8, x8, constant_vector_v7@PAGEOFF
    ldr    q7, [x8]

    adrp   x8, constant_vector_v16@PAGE
    add    x8, x8, constant_vector_v16@PAGEOFF
    ldr    q16, [x8]

    add    x22, sp, #0x248
    add    x23, sp, #0x148

    adrp   x8, constant_vector_1@PAGE
    add    x8, x8, constant_vector_1@PAGEOFF
    ldr    q17, [x8]

    mov    w24, #0x4f3
    movk   w24, #0x3f35, lsl #16

    fmov   s9, #0.25000000

    str    q17, [sp]
    stp    q16, q7, [sp, #0x120]

// 对应 <+172>
loop_outer_start:
    movi.2s v12, #0x5
    movi.2s v13, #0x7
    movi.2s v14, #0x9
    movi.2s v15, #0xb
    movi.2s v8, #0xd
    movi.2s v11, #0xf
    mov    x25, #0x0

// 对应 <+180>
loop_precompute_1:
    ldr    d10, [sp, #0x18]
    sshll.2d v0, v10, #0x0
    scvtf.2d v0, v0
    fmul.2d v0, v0, v7
    fmul.2d v0, v0, v16
    fcvtn  v0.2s, v0.2d
    str    q0, [sp, #0x100]
    bl     _cosf
    str    d0, [sp, #0x110]
    ldr    q0, [sp, #0x100]
    mov    s0, v0[1]
    bl     _cosf
    str    d0, [sp, #0x100]
    movi.2s v0, #0x3
    mul.2s v0, v10, v0
    sshll.2d v0, v0, #0x0
    scvtf.2d v0, v0
    ldr    q1, [sp, #0x130]
    fmul.2d v0, v0, v1
    ldr    q1, [sp, #0x120]
    fmul.2d v0, v0, v1
    fcvtn  v0.2s, v0.2d
    str    q0, [sp, #0xe0]
    mov    s0, v0[1]
    bl     _cosf
    str    d0, [sp, #0xf0]
    ldr    q0, [sp, #0xe0]
    bl     _cosf
    str    d0, [sp, #0xe0]
    mul.2s v0, v10, v12
    sshll.2d v0, v0, #0x0
    scvtf.2d v0, v0
    ldr    q1, [sp, #0x130]
    fmul.2d v0, v0, v1
    ldr    q1, [sp, #0x120]
    fmul.2d v0, v0, v1
    fcvtn  v0.2s, v0.2d
    str    q0, [sp, #0xc0]
    bl     _cosf
    str    d0, [sp, #0xd0]
    ldr    q0, [sp, #0xc0]
    mov    s0, v0[1]
    bl     _cosf
    str    d0, [sp, #0xc0]
    mul.2s v0, v10, v13
    sshll.2d v0, v0, #0x0
    scvtf.2d v0, v0
    ldr    q1, [sp, #0x130]
    fmul.2d v0, v0, v1
    ldr    q1, [sp, #0x120]
    fmul.2d v0, v0, v1
    fcvtn  v0.2s, v0.2d
    str    q0, [sp, #0xa0]
    mov    s0, v0[1]
    bl     _cosf
    str    d0, [sp, #0xb0]
    ldr    q0, [sp, #0xa0]
    bl     _cosf
    str    d0, [sp, #0xa0]
    mul.2s v0, v10, v14
    sshll.2d v0, v0, #0x0
    scvtf.2d v0, v0
    ldr    q1, [sp, #0x130]
    fmul.2d v0, v0, v1
    ldr    q1, [sp, #0x120]
    fmul.2d v0, v0, v1
    fcvtn  v0.2s, v0.2d
    str    q0, [sp, #0x80]
    bl     _cosf
    str    d0, [sp, #0x90]
    ldr    q0, [sp, #0x80]
    mov    s0, v0[1]
    bl     _cosf
    str    d0, [sp, #0x80]
    mul.2s v0, v10, v15
    sshll.2d v0, v0, #0x0
    scvtf.2d v0, v0
    ldr    q1, [sp, #0x130]
    fmul.2d v0, v0, v1
    ldr    q1, [sp, #0x120]
    fmul.2d v0, v0, v1
    fcvtn  v0.2s, v0.2d
    str    q0, [sp, #0x60]
    mov    s0, v0[1]
    bl     _cosf
    str    d0, [sp, #0x70]
    ldr    q0, [sp, #0x60]
    bl     _cosf
    str    d0, [sp, #0x60]
    mul.2s v0, v10, v8
    sshll.2d v0, v0, #0x0
    scvtf.2d v0, v0
    ldr    q1, [sp, #0x130]
    fmul.2d v0, v0, v1
    ldr    q1, [sp, #0x120]
    fmul.2d v0, v0, v1
    fcvtn  v0.2s, v0.2d
    str    q0, [sp, #0x40]
    bl     _cosf
    str    d0, [sp, #0x50]
    ldr    q0, [sp, #0x40]
    mov    s0, v0[1]
    bl     _cosf
    str    d0, [sp, #0x40]
    mul.2s v0, v10, v11
    sshll.2d v0, v0, #0x0
    scvtf.2d v0, v0
    ldr    q1, [sp, #0x130]
    fmul.2d v0, v0, v1
    ldr    q1, [sp, #0x120]
    fmul.2d v0, v0, v1
    fcvtn  v0.2s, v0.2d
    str    q0, [sp, #0x20]
    mov    s0, v0[1]
    bl     _cosf
    str    d0, [sp, #0x30]
    ldr    q0, [sp, #0x20]
    bl     _cosf
    ldp    q16, q7, [sp, #0x120]
    movi.2s v4, #0x2
    ldp    q2, q1, [sp, #0x100]
    mov.s  v1[1], v2[0]
    ldr    q2, [sp, #0xe0]
    mov.s  v1[2], v2[0]
    ldr    q2, [sp, #0xf0]
    mov.s  v1[3], v2[0]
    mov.16b v2, v1
    ldp    q3, q1, [sp, #0xc0]
    mov.s  v1[1], v3[0]
    ldr    q3, [sp, #0xa0]
    mov.s  v1[2], v3[0]
    ldr    q3, [sp, #0xb0]
    mov.s  v1[3], v3[0]
    mov.16b v3, v1
    ldp    q5, q1, [sp, #0x80]
    mov.s  v1[1], v5[0]
    ldp    q6, q5, [sp, #0x50] // Note: original used q5 again here, may be intended or typo in disasm
    mov.s  v1[2], v5[0]
    ldr    q5, [sp, #0x70]
    mov.s  v1[3], v5[0]
    mov.16b v5, v1
    ldr    q1, [sp, #0x40]
    mov.s  v6[1], v1[0]
    mov.s  v6[2], v0[0]
    ldr    q0, [sp, #0x30]
    mov.s  v6[3], v0[0]
    add    x8, x22, x25
    uzp1.4s v0, v2, v3
    uzp1.4s v1, v5, v6
    uzp2.4s v2, v2, v3
    uzp2.4s v3, v5, v6
    stp    q2, q3, [x8, #0x20]
    stp    q0, q1, [x8]
    add.2s v10, v10, v4
    add    x25, x25, #0x40
    cmp    x25, #0x100
    b.ne   loop_precompute_1

    mov    x25, #0x0

// 对应 <+812>
loop_precompute_2:
    ldr    d10, [sp, #0x18]
    sshll.2d v0, v10, #0x0
    scvtf.2d v0, v0
    fmul.2d v0, v0, v7
    fmul.2d v0, v0, v16
    fcvtn  v0.2s, v0.2d
    str    q0, [sp, #0x100]
    bl     _cosf
    str    d0, [sp, #0x110]
    ldr    q0, [sp, #0x100]
    mov    s0, v0[1]
    bl     _cosf
    str    d0, [sp, #0x100]
    movi.2s v0, #0x3
    mul.2s v0, v10, v0
    sshll.2d v0, v0, #0x0
    scvtf.2d v0, v0
    ldr    q1, [sp, #0x130]
    fmul.2d v0, v0, v1
    ldr    q1, [sp, #0x120]
    fmul.2d v0, v0, v1
    fcvtn  v0.2s, v0.2d
    str    q0, [sp, #0xe0]
    mov    s0, v0[1]
    bl     _cosf
    str    d0, [sp, #0xf0]
    ldr    q0, [sp, #0xe0]
    bl     _cosf
    str    d0, [sp, #0xe0]
    mul.2s v0, v10, v12
    sshll.2d v0, v0, #0x0
    scvtf.2d v0, v0
    ldr    q1, [sp, #0x130]
    fmul.2d v0, v0, v1
    ldr    q1, [sp, #0x120]
    fmul.2d v0, v0, v1
    fcvtn  v0.2s, v0.2d
    str    q0, [sp, #0xc0]
    bl     _cosf
    str    d0, [sp, #0xd0]
    ldr    q0, [sp, #0xc0]
    mov    s0, v0[1]
    bl     _cosf
    str    d0, [sp, #0xc0]
    mul.2s v0, v10, v13
    sshll.2d v0, v0, #0x0
    scvtf.2d v0, v0
    ldr    q1, [sp, #0x130]
    fmul.2d v0, v0, v1
    ldr    q1, [sp, #0x120]
    fmul.2d v0, v0, v1
    fcvtn  v0.2s, v0.2d
    str    q0, [sp, #0xa0]
    mov    s0, v0[1]
    bl     _cosf
    str    d0, [sp, #0xb0]
    ldr    q0, [sp, #0xa0]
    bl     _cosf
    str    d0, [sp, #0xa0]
    mul.2s v0, v10, v14
    sshll.2d v0, v0, #0x0
    scvtf.2d v0, v0
    ldr    q1, [sp, #0x130]
    fmul.2d v0, v0, v1
    ldr    q1, [sp, #0x120]
    fmul.2d v0, v0, v1
    fcvtn  v0.2s, v0.2d
    str    q0, [sp, #0x80]
    bl     _cosf
    str    d0, [sp, #0x90]
    ldr    q0, [sp, #0x80]
    mov    s0, v0[1]
    bl     _cosf
    str    d0, [sp, #0x80]
    mul.2s v0, v10, v15
    sshll.2d v0, v0, #0x0
    scvtf.2d v0, v0
    ldr    q1, [sp, #0x130]
    fmul.2d v0, v0, v1
    ldr    q1, [sp, #0x120]
    fmul.2d v0, v0, v1
    fcvtn  v0.2s, v0.2d
    str    q0, [sp, #0x60]
    mov    s0, v0[1]
    bl     _cosf
    str    d0, [sp, #0x70]
    ldr    q0, [sp, #0x60]
    bl     _cosf
    str    d0, [sp, #0x60]
    mul.2s v0, v10, v8
    sshll.2d v0, v0, #0x0
    scvtf.2d v0, v0
    ldr    q1, [sp, #0x130]
    fmul.2d v0, v0, v1
    ldr    q1, [sp, #0x120]
    fmul.2d v0, v0, v1
    fcvtn  v0.2s, v0.2d
    str    q0, [sp, #0x40]
    bl     _cosf
    str    d0, [sp, #0x50]
    ldr    q0, [sp, #0x40]
    mov    s0, v0[1]
    bl     _cosf
    str    d0, [sp, #0x40]
    mul.2s v0, v10, v11
    sshll.2d v0, v0, #0x0
    scvtf.2d v0, v0
    ldr    q1, [sp, #0x130]
    fmul.2d v0, v0, v1
    ldr    q1, [sp, #0x120]
    fmul.2d v0, v0, v1
    fcvtn  v0.2s, v0.2d
    str    q0, [sp, #0x20]
    mov    s0, v0[1]
    bl     _cosf
    str    d0, [sp, #0x30]
    ldr    q0, [sp, #0x20]
    bl     _cosf
    ldp    q16, q7, [sp, #0x120]
    movi.2s v4, #0x2
    ldp    q2, q1, [sp, #0x100]
    mov.s  v1[1], v2[0]
    ldr    q2, [sp, #0xe0]
    mov.s  v1[2], v2[0]
    ldr    q2, [sp, #0xf0]
    mov.s  v1[3], v2[0]
    mov.16b v2, v1
    ldp    q3, q1, [sp, #0xc0]
    mov.s  v1[1], v3[0]
    ldr    q3, [sp, #0xa0]
    mov.s  v1[2], v3[0]
    ldr    q3, [sp, #0xb0]
    mov.s  v1[3], v3[0]
    mov.16b v3, v1
    ldp    q5, q1, [sp, #0x80]
    mov.s  v1[1], v5[0]
    ldp    q6, q5, [sp, #0x50]
    mov.s  v1[2], v5[0]
    ldr    q5, [sp, #0x70]
    mov.s  v1[3], v5[0]
    mov.16b v5, v1
    ldr    q1, [sp, #0x40]
    mov.s  v6[1], v1[0]
    mov.s  v6[2], v0[0]
    ldr    q0, [sp, #0x30]
    mov.s  v6[3], v0[0]
    add    x8, x23, x25
    uzp1.4s v0, v2, v3
    uzp1.4s v1, v5, v6
    uzp2.4s v2, v2, v3
    uzp2.4s v3, v5, v6
    stp    q2, q3, [x8, #0x20]
    stp    q0, q1, [x8]
    add.2s v10, v10, v4
    add    x25, x25, #0x40
    cmp    x25, #0x100
    b.ne   loop_precompute_2

    mov    x8, #0x0

// 对应 <+1456>
loop_idct_outer:
    add    x9, x19, x20, lsl #7
    mov    x10, x21
    ldr    q17, [sp]
    movi.2d v18, #0
    add    x11, x23, x8, lsl #2

// 对应 <+1524>
loop_idct_inner:
    ldr    s0, [x11]
    add    x12, x11, #0x20
    add    x13, x11, #0x40
    add    x14, x11, #0x60
    ldr    s1, [x11, #0x80]
    add    x15, x11, #0xa0
    add    x16, x11, #0xc0
    add    x11, x11, #0xe0
    ld1.s  { v1 }[1], [x15]
    ld1.s  { v1 }[2], [x16]
    ld1.s  { v1 }[3], [x11]
    ld1.s  { v0 }[1], [x12]
    ld1.s  { v0 }[2], [x13]
    ld1.s  { v0 }[3], [x14]
    mov    x11, #-0x20
    mov    x12, x10

    // Core IDCT computation blocks
    add    x13, x22, x11
    ldr    s2, [x13, #0x20]
    fmov   s3, w24
    fmul   s2, s2, s3
    ldp    d3, d4, [x9]
    sshll.4s v3, v3, #0x0
    scvtf.4s v3, v3
    fmul.4s v5, v17, v2[0]
    fmul.4s v5, v5, v0
    fmul.4s v3, v5, v3
    fadd.4s v3, v3, v18
    sshll.4s v4, v4, #0x0
    scvtf.4s v4, v4
    fmul.4s v2, v1, v2[0]
    fmul.4s v2, v2, v4
    fadd.4s v2, v3, v2
    ldr    s3, [x13, #0x40]
    ldp    d4, d5, [x9, #0x10]
    sshll.4s v4, v4, #0x0
    scvtf.4s v4, v4
    fmul.4s v6, v17, v3[0]
    fmul.4s v6, v6, v0
    fmul.4s v4, v6, v4
    fadd.4s v2, v2, v4
    sshll.4s v4, v5, #0x0
    scvtf.4s v4, v4
    fmul.4s v3, v1, v3[0]
    fmul.4s v3, v3, v4
    fadd.4s v2, v2, v3
    ldr    s3, [x13, #0x60]
    ldp    d4, d5, [x9, #0x20]
    sshll.4s v4, v4, #0x0
    scvtf.4s v4, v4
    fmul.4s v6, v17, v3[0]
    fmul.4s v6, v6, v0
    fmul.4s v4, v6, v4
    fadd.4s v2, v2, v4
    sshll.4s v4, v5, #0x0
    scvtf.4s v4, v4
    fmul.4s v3, v1, v3[0]
    fmul.4s v3, v3, v4
    fadd.4s v2, v2, v3
    ldr    s3, [x13, #0x80]
    ldp    d4, d5, [x9, #0x30]
    sshll.4s v4, v4, #0x0
    scvtf.4s v4, v4
    fmul.4s v6, v17, v3[0]
    fmul.4s v6, v6, v0
    fmul.4s v4, v6, v4
    fadd.4s v2, v2, v4
    sshll.4s v4, v5, #0x0
    scvtf.4s v4, v4
    fmul.4s v3, v1, v3[0]
    fmul.4s v3, v3, v4
    fadd.4s v2, v2, v3
    ldr    s3, [x13, #0xa0]
    ldp    d4, d5, [x9, #0x40]
    sshll.4s v4, v4, #0x0
    scvtf.4s v4, v4
    fmul.4s v6, v17, v3[0]
    fmul.4s v6, v6, v0
    fmul.4s v4, v6, v4
    fadd.4s v2, v2, v4
    sshll.4s v4, v5, #0x0
    scvtf.4s v4, v4
    fmul.4s v3, v1, v3[0]
    fmul.4s v3, v3, v4
    fadd.4s v2, v2, v3
    ldr    s3, [x13, #0xc0]
    ldp    d4, d5, [x9, #0x50]
    sshll.4s v4, v4, #0x0
    scvtf.4s v4, v4
    fmul.4s v6, v17, v3[0]
    fmul.4s v6, v6, v0
    fmul.4s v4, v6, v4
    fadd.4s v2, v2, v4
    sshll.4s v4, v5, #0x0
    scvtf.4s v4, v4
    fmul.4s v3, v1, v3[0]
    fmul.4s v3, v3, v4
    fadd.4s v2, v2, v3
    ldr    s3, [x13, #0xe0]
    ldp    d4, d5, [x9, #0x60]
    sshll.4s v4, v4, #0x0
    scvtf.4s v4, v4
    fmul.4s v6, v17, v3[0]
    fmul.4s v6, v6, v0
    fmul.4s v4, v6, v4
    fadd.4s v2, v2, v4
    sshll.4s v4, v5, #0x0
    scvtf.4s v4, v4
    fmul.4s v3, v1, v3[0]
    fmul.4s v3, v3, v4
    fadd.4s v2, v2, v3
    ldr    s3, [x13, #0x100]
    ldp    d4, d5, [x9, #0x70]
    sshll.4s v4, v4, #0x0
    scvtf.4s v4, v4
    fmul.4s v6, v17, v3[0]
    fmul.4s v6, v6, v0
    fmul.4s v4, v6, v4
    fadd.4s v2, v2, v4
    sshll.4s v4, v5, #0x0
    scvtf.4s v4, v4
    fmul.4s v3, v1, v3[0]
    fmul.4s v3, v3, v4
    fadd.4s v2, v2, v3
    faddp.4s v2, v2, v2
    faddp.2s s2, v2
    fmul   s2, s2, s9
    frinta s2, s2
    str    s2, [x12], #0x20
    adds   x11, x11, #0x4
    b.ne   loop_idct_inner

    add    x8, x8, #0x1
    add    x10, x10, #0x4
    cmp    x8, #0x8
    b.ne   loop_idct_outer

    add    x20, x20, #0x1
    add    x21, x21, #0x100
    cmp    x20, #0x3
    b.ne   loop_outer_start

    // --- 函数尾声 (Function Epilogue) ---
    add    sp, sp, #0x350
    ldp    x29, x30, [sp, #0x80]
    ldp    x20, x19, [sp, #0x70]
    ldp    x22, x21, [sp, #0x60]
    ldp    x24, x23, [sp, #0x50]
    ldp    x26, x25, [sp, #0x40]
    ldp    d9, d8, [sp, #0x30]
    ldp    d11, d10, [sp, #0x20]
    ldp    d13, d12, [sp, #0x10]
    ldp    d15, d14, [sp], #0x90

    ret
