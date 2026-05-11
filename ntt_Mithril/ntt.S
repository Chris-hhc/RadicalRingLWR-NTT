.intel_syntax noprefix

.equ NTT_AVX_PARAM_TOP1_QINV_BYTES, 4
.equ NTT_AVX_PARAM_BOTTOM6_ZETA_BYTES, 8
.equ NTT_AVX_PARAM_BOTTOM6_QINV_BYTES, 648

.macro shuffle8 r0, r1, r2, r3
    vperm2i128 ymm\r2, ymm\r0, ymm\r1, 0x20
    vperm2i128 ymm\r3, ymm\r0, ymm\r1, 0x31
.endm

.macro shuffle4 r0, r1, r2, r3
    vpunpcklqdq ymm\r2, ymm\r0, ymm\r1
    vpunpckhqdq ymm\r3, ymm\r0, ymm\r1
.endm

.macro shuffle2 r0, r1, r2, r3
    vmovsldup ymm\r2, ymm\r1
    vpblendd ymm\r2, ymm\r0, ymm\r2, 0xaa
    vpsrlq ymm\r0, ymm\r0, 32
    vpblendd ymm\r3, ymm\r0, ymm\r1, 0xaa
.endm

.macro reduce_low_oct x, tmp, tmp2
    vpsrad ymm\tmp, ymm\x, 31
    vpaddd ymm\tmp2, ymm0, ymm0
    vpand ymm\tmp, ymm\tmp, ymm\tmp2
    vpsubd ymm\tmp, ymm\tmp, ymm0
    vpaddd ymm\x, ymm\x, ymm\tmp
.endm

.macro butterfly l, h, pre_e=1, pre_o=1, zeta_e=2, zeta_o=2
    vmovshdup ymm12, ymm\h
    vpmuldq ymm13, ymm\h, ymm\zeta_e
    vpmuldq ymm14, ymm12, ymm\zeta_o
    vpmuldq ymm\h, ymm\h, ymm\pre_e
    vpmuldq ymm12, ymm12, ymm\pre_o
    vpmuldq ymm\h, ymm\h, ymm0
    vpmuldq ymm12, ymm12, ymm0
    vpaddq ymm13, ymm13, ymm\h
    vpaddq ymm14, ymm14, ymm12
    vmovshdup ymm13, ymm13
    vpblendd ymm\h, ymm13, ymm14, 0xaa
    reduce_low_oct \l, 12, 14
    vpsubd ymm12, ymm\l, ymm\h
    vpaddd ymm\l, ymm\l, ymm\h
    vmovdqa ymm\h, ymm12
.endm

.macro final_reduce r
    reduce_low_oct \r, 12, 14
.endm

.macro final_permute_store_natural
    vpunpckldq ymm12, ymm9, ymm8
    vpunpckhdq ymm13, ymm9, ymm8
    vpunpckldq ymm14, ymm7, ymm6
    vpunpckhdq ymm15, ymm7, ymm6
    vpunpcklqdq ymm9, ymm12, ymm14
    vpunpckhqdq ymm8, ymm12, ymm14
    vpunpcklqdq ymm7, ymm13, ymm15
    vpunpckhqdq ymm6, ymm13, ymm15

    vpunpckldq ymm12, ymm5, ymm4
    vpunpckhdq ymm13, ymm5, ymm4
    vpunpckldq ymm14, ymm3, ymm11
    vpunpckhdq ymm15, ymm3, ymm11
    vpunpcklqdq ymm5, ymm12, ymm14
    vpunpckhqdq ymm4, ymm12, ymm14
    vpunpcklqdq ymm3, ymm13, ymm15
    vpunpckhqdq ymm11, ymm13, ymm15

    vperm2i128 ymm12, ymm9, ymm5, 0x20
    vmovdqu YMMWORD PTR [r11 +   0], ymm12
    vperm2i128 ymm12, ymm8, ymm4, 0x20
    vmovdqu YMMWORD PTR [r11 +  32], ymm12
    vperm2i128 ymm12, ymm7, ymm3, 0x20
    vmovdqu YMMWORD PTR [r11 +  64], ymm12
    vperm2i128 ymm12, ymm6, ymm11, 0x20
    vmovdqu YMMWORD PTR [r11 +  96], ymm12
    vperm2i128 ymm12, ymm9, ymm5, 0x31
    vmovdqu YMMWORD PTR [r11 + 128], ymm12
    vperm2i128 ymm12, ymm8, ymm4, 0x31
    vmovdqu YMMWORD PTR [r11 + 160], ymm12
    vperm2i128 ymm12, ymm7, ymm3, 0x31
    vmovdqu YMMWORD PTR [r11 + 192], ymm12
    vperm2i128 ymm12, ymm6, ymm11, 0x31
    vmovdqu YMMWORD PTR [r11 + 224], ymm12
.endm

.macro levels4t9_dilithium_redlow
    vmovdqu ymm4,  YMMWORD PTR [r11 +   0]
    vmovdqu ymm5,  YMMWORD PTR [r11 +  32]
    vmovdqu ymm6,  YMMWORD PTR [r11 +  64]
    vmovdqu ymm7,  YMMWORD PTR [r11 +  96]
    vmovdqu ymm8,  YMMWORD PTR [r11 + 128]
    vmovdqu ymm9,  YMMWORD PTR [r11 + 160]
    vmovdqu ymm10, YMMWORD PTR [r11 + 192]
    vmovdqu ymm11, YMMWORD PTR [r11 + 224]

    vmovdqu ymm1, YMMWORD PTR [r13 +   0]
    vmovdqu ymm2, YMMWORD PTR [r12 +   0]
    butterfly 4, 8
    butterfly 5, 9
    butterfly 6, 10
    butterfly 7, 11

    shuffle8 4, 8, 3, 8
    shuffle8 5, 9, 4, 9
    shuffle8 6, 10, 5, 10
    shuffle8 7, 11, 6, 11

    vmovdqu ymm1, YMMWORD PTR [r13 +  32]
    vmovdqu ymm2, YMMWORD PTR [r12 +  32]
    butterfly 3, 5
    butterfly 8, 10
    butterfly 4, 6
    butterfly 9, 11

    shuffle4 3, 5, 7, 5
    shuffle4 8, 10, 3, 10
    shuffle4 4, 6, 8, 6
    shuffle4 9, 11, 4, 11

    vmovdqu ymm1, YMMWORD PTR [r13 +  64]
    vmovdqu ymm2, YMMWORD PTR [r12 +  64]
    butterfly 7, 8
    butterfly 5, 6
    butterfly 3, 4
    butterfly 10, 11

    shuffle2 7, 8, 9, 8
    shuffle2 5, 6, 7, 6
    shuffle2 3, 4, 5, 4
    shuffle2 10, 11, 3, 11

    vmovdqu ymm1, YMMWORD PTR [r13 +  96]
    vmovdqu ymm2, YMMWORD PTR [r12 +  96]
    vpsrlq ymm10, ymm1, 32
    vmovshdup ymm15, ymm2
    butterfly 9, 5, 1, 10, 2, 15
    butterfly 8, 4, 1, 10, 2, 15
    butterfly 7, 3, 1, 10, 2, 15
    butterfly 6, 11, 1, 10, 2, 15

    vmovdqu ymm1, YMMWORD PTR [r13 + 128]
    vmovdqu ymm2, YMMWORD PTR [r12 + 128]
    vpsrlq ymm10, ymm1, 32
    vmovshdup ymm15, ymm2
    butterfly 9, 7, 1, 10, 2, 15
    butterfly 8, 6, 1, 10, 2, 15

    vmovdqu ymm1, YMMWORD PTR [r13 + 160]
    vmovdqu ymm2, YMMWORD PTR [r12 + 160]
    vpsrlq ymm10, ymm1, 32
    vmovshdup ymm15, ymm2
    butterfly 5, 3, 1, 10, 2, 15
    butterfly 4, 11, 1, 10, 2, 15

    vmovdqu ymm1, YMMWORD PTR [r13 + 192]
    vmovdqu ymm2, YMMWORD PTR [r12 + 192]
    vpsrlq ymm10, ymm1, 32
    vmovshdup ymm15, ymm2
    butterfly 9, 8, 1, 10, 2, 15

    vmovdqu ymm1, YMMWORD PTR [r13 + 224]
    vmovdqu ymm2, YMMWORD PTR [r12 + 224]
    vpsrlq ymm10, ymm1, 32
    vmovshdup ymm15, ymm2
    butterfly 7, 6, 1, 10, 2, 15

    vmovdqu ymm1, YMMWORD PTR [r13 + 256]
    vmovdqu ymm2, YMMWORD PTR [r12 + 256]
    vpsrlq ymm10, ymm1, 32
    vmovshdup ymm15, ymm2
    butterfly 5, 4, 1, 10, 2, 15

    vmovdqu ymm1, YMMWORD PTR [r13 + 288]
    vmovdqu ymm2, YMMWORD PTR [r12 + 288]
    vpsrlq ymm10, ymm1, 32
    vmovshdup ymm15, ymm2
    butterfly 3, 11, 1, 10, 2, 15

    final_reduce 9
    final_reduce 8
    final_reduce 7
    final_reduce 6
    final_reduce 5
    final_reduce 4
    final_reduce 3
    final_reduce 11

    final_permute_store_natural
.endm

.text
.globl poly_ntt32_top1_redlow_avx2
.type poly_ntt32_top1_redlow_avx2, @function
poly_ntt32_top1_redlow_avx2:
    vmovd xmm0, esi
    vpbroadcastd ymm0, xmm0
    vpbroadcastd ymm1, DWORD PTR [rcx]
    vpbroadcastd ymm2, DWORD PTR [rdx]

    mov r8d, 8
    mov r9, rdi
.Ltop1_loop:
    vmovdqu ymm4, YMMWORD PTR [r9]
    vmovdqu ymm5, YMMWORD PTR [r9 + 256]
    butterfly 4, 5
    vmovdqu YMMWORD PTR [r9], ymm4
    vmovdqu YMMWORD PTR [r9 + 256], ymm5
    add r9, 32
    dec r8d
    jne .Ltop1_loop
    ret
.size poly_ntt32_top1_redlow_avx2, .-poly_ntt32_top1_redlow_avx2

.globl poly_ntt32_bottom6_dilithium_redlow_avx2
.type poly_ntt32_bottom6_dilithium_redlow_avx2, @function
poly_ntt32_bottom6_dilithium_redlow_avx2:
    push r12
    push r13

    vmovd xmm0, esi
    vpbroadcastd ymm0, xmm0

    mov r8d, 2
.Lbottom6_block_loop:
    mov r11, rdi
    mov r12, rdx
    mov r13, rcx
    levels4t9_dilithium_redlow
    add rdi, 256
    add rdx, 320
    add rcx, 320
    dec r8d
    jne .Lbottom6_block_loop

    pop r13
    pop r12
    ret
.size poly_ntt32_bottom6_dilithium_redlow_avx2, .-poly_ntt32_bottom6_dilithium_redlow_avx2

.globl ntt_avx
.type ntt_avx, @function
ntt_avx:
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 8

    mov r12, rdi
    mov ebx, esi
    mov r13, rdx
    lea r14, [rdx + NTT_AVX_PARAM_TOP1_QINV_BYTES]
    lea r15, [rdx + NTT_AVX_PARAM_BOTTOM6_ZETA_BYTES]
    lea rbp, [rdx + NTT_AVX_PARAM_BOTTOM6_QINV_BYTES]

    mov rdx, r13
    mov rcx, r14
    call poly_ntt32_top1_redlow_avx2

    mov rdi, r12
    mov esi, ebx
    mov rdx, r15
    mov rcx, rbp
    call poly_ntt32_bottom6_dilithium_redlow_avx2

    vzeroupper
    add rsp, 8
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret
.size ntt_avx, .-ntt_avx
