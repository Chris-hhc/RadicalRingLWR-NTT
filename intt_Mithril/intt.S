.intel_syntax noprefix

.equ INTT_AVX_PARAM_FIRST6_PRE_BYTES, 640
.equ INTT_AVX_PARAM_TAIL_ZETAS_BYTES, 1280
.equ INTT_AVX_PARAM_TAIL_PRE_BYTES, 1288
.equ INTT_AVX_PARAM_META_BYTES, 1296
.equ INTT_AVX_FIRST6_BLOCK_BYTES, 320

.macro reduce_low_oct x, tmp, tmp2
    vpsrad ymm\tmp, ymm\x, 31
    vpaddd ymm\tmp2, ymm0, ymm0
    vpand ymm\tmp, ymm\tmp, ymm\tmp2
    vpsubd ymm\tmp, ymm\tmp, ymm0
    vpaddd ymm\x, ymm\x, ymm\tmp
.endm

.macro montmul_oct_inplace x
    vmovshdup ymm12, ymm\x
    vpmuldq ymm13, ymm\x, ymm2
    vpmuldq ymm14, ymm12, ymm2
    vpmuldq ymm\x, ymm\x, ymm1
    vpmuldq ymm12, ymm12, ymm1
    vpmuldq ymm\x, ymm\x, ymm0
    vpmuldq ymm12, ymm12, ymm0
    vpaddq ymm13, ymm13, ymm\x
    vpaddq ymm14, ymm14, ymm12
    vmovshdup ymm13, ymm13
    vpblendd ymm\x, ymm13, ymm14, 0xaa
.endm

.macro gs_cross l, h
    vpsubd ymm3, ymm\h, ymm\l
    vpaddd ymm\l, ymm\l, ymm\h
    montmul_oct_inplace 3
    reduce_low_oct \l, 12, 14
    vmovdqa ymm\h, ymm3
.endm

.macro shuffle8 r0, r1, r2, r3
    vperm2i128 ymm\r2, ymm\r0, ymm\r1, 0x20
    vperm2i128 ymm\r3, ymm\r0, ymm\r1, 0x31
.endm

.macro shuffle4 r0, r1, r2, r3
    vpunpcklqdq ymm\r2, ymm\r0, ymm\r1
    vpunpckhqdq ymm\r3, ymm\r0, ymm\r1
.endm


/* 
    ymm\r0 = [A0, A1, A2, A3, A4, A5, A6, A7]
    ymm\r1 = [B0, B1, B2, B3, B4, B5, B6, B7]
*/

.macro shuffle2 r0, r1, r2, r3
    vmovsldup ymm\r2, ymm\r1 # ymm\r2 = [B0, B0, B2, B2, B4, B4, B6, B6]
    vpblendd ymm\r2, ymm\r0, ymm\r2, 0xaa # ymm\r2 = [A0, B0, A2, B2, A4, B4, A6, B6]
    vpsrlq ymm\r0, ymm\r0, 32  # ymm\r0 = [A1, 0, A3, 0, A5, 0, A7, 0]
    vpblendd ymm\r3, ymm\r0, ymm\r1, 0xaa #ymm\r3 = [A1, B1, A3, B3, A5, B5, A7, B7]
.endm

.macro gs_packed l, h, pre_e=1, pre_o=3, zeta_e=2, zeta_o=15
    vpsubd ymm12, ymm\h, ymm\l
    vpaddd ymm\l, ymm\l, ymm\h
    vmovshdup ymm13, ymm12
    vpmuldq ymm14, ymm12, ymm\zeta_e  # ymm14 odd = [D0*Z0, D2*Z2, D4*Z4, D6*Z6]
    vpmuldq ymm\h, ymm13, ymm\zeta_o  # ymm\h even = [D1*Z1, D3*Z3, D5*Z5, D7*Z7]
    vpmuldq ymm12, ymm12, ymm\pre_e   # ymm12 odd = [D0*P0, D2*P2, D4*P4, D6*P6]
    vpmuldq ymm13, ymm13, ymm\pre_o   # ymm13 even = [D1*P1, D3*P3, D5*P5, D7*P7]
    vpmuldq ymm12, ymm12, ymm0
    vpmuldq ymm13, ymm13, ymm0
    vpaddq ymm14, ymm14, ymm12        # even T0,T2,T4,T6
    vpaddq ymm\h, ymm\h, ymm13        # odd  T1,T3,T5,T7
    vmovshdup ymm14, ymm14
    vpblendd ymm\h, ymm14, ymm\h, 0xaa
    reduce_low_oct \l, 12, 13
.endm

.macro load_roots_levels0t5_early off
    vmovdqu ymm1, YMMWORD PTR [r13 + \off] # ymm1  = [P0,P1,P2,P3,P4,P5,P6,P7]
    vmovdqu ymm2, YMMWORD PTR [r12 + \off] # ymm2  = [Z0,Z1,Z2,Z3,Z4,Z5,Z6,Z7]
    vmovshdup ymm3, ymm1 # ymm3  = [P1,P1,P3,P3,P5,P5,P7,P7]
    vmovshdup ymm15, ymm2 # ymm15 = [Z1,Z1,Z3,Z3,Z5,Z5,Z7,Z7]
.endm

.macro load_roots_levels0t5_late off, pre_o
    vmovdqu ymm1, YMMWORD PTR [r13 + \off]
    vmovdqu ymm2, YMMWORD PTR [r12 + \off]
    vmovshdup ymm\pre_o, ymm1
    vmovshdup ymm15, ymm2
.endm

.macro transpose8x8_dwords_for_levels0t5
    vpunpckldq ymm12, ymm4, ymm5
    vpunpckhdq ymm13, ymm4, ymm5
    vpunpckldq ymm14, ymm6, ymm7
    vpunpckhdq ymm15, ymm6, ymm7
    vpunpcklqdq ymm1, ymm12, ymm14
    vpunpckhqdq ymm2, ymm12, ymm14
    vpunpcklqdq ymm3, ymm13, ymm15
    vpunpckhqdq ymm4, ymm13, ymm15

    vpunpckldq ymm12, ymm8, ymm9
    vpunpckhdq ymm13, ymm8, ymm9
    vpunpckldq ymm14, ymm10, ymm11
    vpunpckhdq ymm15, ymm10, ymm11
    vpunpcklqdq ymm5, ymm12, ymm14
    vpunpckhqdq ymm6, ymm12, ymm14
    vpunpcklqdq ymm7, ymm13, ymm15
    vpunpckhqdq ymm8, ymm13, ymm15

    vperm2i128 ymm12, ymm1, ymm5, 0x20
    vperm2i128 ymm13, ymm1, ymm5, 0x31
    vperm2i128 ymm14, ymm2, ymm6, 0x20
    vperm2i128 ymm15, ymm2, ymm6, 0x31
    vperm2i128 ymm5, ymm3, ymm7, 0x20
    vperm2i128 ymm9, ymm3, ymm7, 0x31
    vperm2i128 ymm6, ymm4, ymm8, 0x20
    vperm2i128 ymm10, ymm4, ymm8, 0x31

    vmovdqa ymm4, ymm12
    vmovdqa ymm8, ymm13
    vmovdqa ymm7, ymm6
    vmovdqa ymm6, ymm5
    vmovdqa ymm5, ymm14
    vmovdqa ymm11, ymm10
    vmovdqa ymm10, ymm9
    vmovdqa ymm9, ymm15
.endm

.macro store_levels0t5_natural
    vmovdqu YMMWORD PTR [r11 +   0], ymm9
    vmovdqu YMMWORD PTR [r11 +  32], ymm10
    vmovdqu YMMWORD PTR [r11 +  64], ymm6
    vmovdqu YMMWORD PTR [r11 +  96], ymm4
    vmovdqu YMMWORD PTR [r11 + 128], ymm3
    vmovdqu YMMWORD PTR [r11 + 160], ymm5
    vmovdqu YMMWORD PTR [r11 + 192], ymm8
    vmovdqu YMMWORD PTR [r11 + 224], ymm11
.endm

.macro gs_final_cross_normal l, h
    vpsubd ymm3, ymm\h, ymm\l
    vpaddd ymm\l, ymm\l, ymm\h
    montmul_oct_inplace 3
    reduce_low_oct \l, 12, 14
    vmovdqa ymm10, ymm1
    vmovdqa ymm11, ymm2
    vmovdqa ymm1, ymm8
    vmovdqa ymm2, ymm9
    montmul_oct_inplace \l
    montmul_oct_inplace 3
    vmovdqa ymm1, ymm10
    vmovdqa ymm2, ymm11
    vmovdqa ymm\h, ymm3
.endm

.macro gs_final_cross_fused l, h
    vpsubd ymm3, ymm\h, ymm\l
    vpaddd ymm\l, ymm\l, ymm\h
    reduce_low_oct \l, 12, 14
    vmovdqa ymm10, ymm1
    vmovdqa ymm11, ymm2
    vmovdqa ymm1, ymm8
    vmovdqa ymm2, ymm9
    montmul_oct_inplace \l
    vmovdqa ymm1, ymm10
    vmovdqa ymm2, ymm11
    montmul_oct_inplace 3
    vmovdqa ymm\h, ymm3
.endm

.text
.globl poly_intt32_first6_levels0t5_redlow_avx2
.type poly_intt32_first6_levels0t5_redlow_avx2, @function
poly_intt32_first6_levels0t5_redlow_avx2:
    push r12
    push r13
    vmovd xmm0, esi
    vpbroadcastd ymm0, xmm0

    mov r8d, 2
    mov r11, rdi
    mov r12, rdx
    mov r13, rcx

.Lfirst6_levels0t5_block_loop:
    vmovdqu ymm4,  YMMWORD PTR [r11 +   0]
    vmovdqu ymm5,  YMMWORD PTR [r11 +  32]
    vmovdqu ymm6,  YMMWORD PTR [r11 +  64]
    vmovdqu ymm7,  YMMWORD PTR [r11 +  96]
    vmovdqu ymm8,  YMMWORD PTR [r11 + 128]
    vmovdqu ymm9,  YMMWORD PTR [r11 + 160]
    vmovdqu ymm10, YMMWORD PTR [r11 + 192]
    vmovdqu ymm11, YMMWORD PTR [r11 + 224]

    transpose8x8_dwords_for_levels0t5

    load_roots_levels0t5_early 0
    gs_packed 4, 5, 1, 3, 2, 15
    load_roots_levels0t5_early 32
    gs_packed 6, 7, 1, 3, 2, 15
    load_roots_levels0t5_early 64
    gs_packed 8, 9, 1, 3, 2, 15
    load_roots_levels0t5_early 96
    gs_packed 10, 11, 1, 3, 2, 15

    load_roots_levels0t5_early 128
    gs_packed 4, 6, 1, 3, 2, 15
    gs_packed 5, 7, 1, 3, 2, 15
    load_roots_levels0t5_early 160
    gs_packed 8, 10, 1, 3, 2, 15
    gs_packed 9, 11, 1, 3, 2, 15

    load_roots_levels0t5_early 192
    gs_packed 4, 8, 1, 3, 2, 15
    gs_packed 5, 9, 1, 3, 2, 15
    gs_packed 6, 10, 1, 3, 2, 15
    gs_packed 7, 11, 1, 3, 2, 15

    shuffle2 4, 5, 3, 5
    shuffle2 6, 7, 4, 7
    shuffle2 8, 9, 6, 9
    shuffle2 10, 11, 8, 11
    /*
        Y3  = [ a0   a1   a16  a17  a32  a33  a48  a49 ]
        Y4  = [ a2   a3   a18  a19  a34  a35  a50  a51 ]
        Y6  = [ a4   a5   a20  a21  a36  a37  a52  a53 ]
        Y8  = [ a6   a7   a22  a23  a38  a39  a54  a55 ]
        Y5  = [ a8   a9   a24  a25  a40  a41  a56  a57 ]
        Y7  = [ a10  a11  a26  a27  a42  a43  a58  a59 ]
        Y9  = [ a12  a13  a28  a29  a44  a45  a60  a61 ]
        Y11 = [ a14  a15  a30  a31  a46  a47  a62  a63 ]
    */

    load_roots_levels0t5_late 224, 10
    gs_packed 3, 5, 1, 10, 2, 15
    gs_packed 4, 7, 1, 10, 2, 15
    gs_packed 6, 9, 1, 10, 2, 15
    gs_packed 8, 11, 1, 10, 2, 15

    shuffle4 3, 4, 10, 4
    shuffle4 6, 8, 3, 8
    shuffle4 5, 7, 6, 7
    shuffle4 9, 11, 5, 11

    /* 
        Y10 = [ a0   a1   a2   a3   a32  a33  a34  a35 ]
        Y4  = [ a16  a17  a18  a19  a48  a49  a50  a51 ]
        Y3  = [ a4   a5   a6   a7   a36  a37  a38  a39 ]
        Y8  = [ a20  a21  a22  a23  a52  a53  a54  a55 ]
        Y6  = [ a8   a9   a10  a11  a40  a41  a42  a43 ]
        Y7  = [ a24  a25  a26  a27  a56  a57  a58  a59 ]
        Y5  = [ a12  a13  a14  a15  a44  a45  a46  a47 ]
        Y11 = [ a28  a29  a30  a31  a60  a61  a62  a63 ]
    */

    load_roots_levels0t5_late 256, 9
    gs_packed 10, 4, 1, 9, 2, 15
    gs_packed 3, 8, 1, 9, 2, 15
    gs_packed 6, 7, 1, 9, 2, 15
    gs_packed 5, 11, 1, 9, 2, 15

    shuffle8 10, 3, 9, 3
    shuffle8 6, 5, 10, 5
    shuffle8 4, 8, 6, 8
    shuffle8 7, 11, 4, 11

    /*
        Y9  = [ a0   a1   a2   a3   a4   a5   a6   a7  ]
        Y10 = [ a8   a9   a10  a11  a12  a13  a14  a15 ]
        Y6  = [ a16  a17  a18  a19  a20  a21  a22  a23 ]
        Y4  = [ a24  a25  a26  a27  a28  a29  a30  a31 ]
        Y3  = [ a32  a33  a34  a35  a36  a37  a38  a39 ]
        Y5  = [ a40  a41  a42  a43  a44  a45  a46  a47 ]
        Y8  = [ a48  a49  a50  a51  a52  a53  a54  a55 ]
        Y11 = [ a56  a57  a58  a59  a60  a61  a62  a63 ]
    */

    load_roots_levels0t5_late 288, 7
    gs_packed 9, 3, 1, 7, 2, 15
    gs_packed 10, 5, 1, 7, 2, 15
    gs_packed 6, 8, 1, 7, 2, 15
    gs_packed 4, 11, 1, 7, 2, 15

    store_levels0t5_natural

    add r11, 256
    add r12, INTT_AVX_FIRST6_BLOCK_BYTES
    add r13, INTT_AVX_FIRST6_BLOCK_BYTES
    dec r8d
    jne .Lfirst6_levels0t5_block_loop

    vzeroupper
    pop r13
    pop r12
    ret
.size poly_intt32_first6_levels0t5_redlow_avx2, .-poly_intt32_first6_levels0t5_redlow_avx2

.globl poly_intt32_last1_final_normal_avx2
.type poly_intt32_last1_final_normal_avx2, @function
poly_intt32_last1_final_normal_avx2:
    vmovd xmm0, esi
    vpbroadcastd ymm0, xmm0
    vpbroadcastd ymm1, DWORD PTR [rcx]
    vpbroadcastd ymm2, DWORD PTR [rdx]
    vpbroadcastd ymm8, DWORD PTR [rcx + 4]
    vpbroadcastd ymm9, DWORD PTR [rdx + 4]

    mov r9, rdi
    mov r8d, 8
.Llast1_normal_loop:
    vmovdqu ymm4, YMMWORD PTR [r9]
    vmovdqu ymm5, YMMWORD PTR [r9 + 256]
    gs_final_cross_normal 4, 5
    vmovdqu YMMWORD PTR [r9], ymm4
    vmovdqu YMMWORD PTR [r9 + 256], ymm5
    add r9, 32
    dec r8d
    jne .Llast1_normal_loop

    vzeroupper
    ret
.size poly_intt32_last1_final_normal_avx2, .-poly_intt32_last1_final_normal_avx2

.globl poly_intt32_last1_final_fused_avx2
.type poly_intt32_last1_final_fused_avx2, @function
poly_intt32_last1_final_fused_avx2:
    vmovd xmm0, esi
    vpbroadcastd ymm0, xmm0
    vpbroadcastd ymm1, DWORD PTR [rcx]
    vpbroadcastd ymm2, DWORD PTR [rdx]
    vpbroadcastd ymm8, DWORD PTR [rcx + 4]
    vpbroadcastd ymm9, DWORD PTR [rdx + 4]

    mov r9, rdi
    mov r8d, 8
.Llast1_fused_loop:
    vmovdqu ymm4, YMMWORD PTR [r9]
    vmovdqu ymm5, YMMWORD PTR [r9 + 256]
    gs_final_cross_fused 4, 5
    vmovdqu YMMWORD PTR [r9], ymm4
    vmovdqu YMMWORD PTR [r9 + 256], ymm5
    add r9, 32
    dec r8d
    jne .Llast1_fused_loop

    vzeroupper
    ret
.size poly_intt32_last1_final_fused_avx2, .-poly_intt32_last1_final_fused_avx2

.globl intt_avx
.type intt_avx, @function
intt_avx:
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
    lea r14, [rdx + INTT_AVX_PARAM_TAIL_ZETAS_BYTES]
    lea r15, [rdx + INTT_AVX_PARAM_TAIL_PRE_BYTES]

    lea rcx, [r13 + INTT_AVX_PARAM_FIRST6_PRE_BYTES]
    call poly_intt32_first6_levels0t5_redlow_avx2

    mov rdi, r12
    mov esi, ebx
    mov rdx, r14
    mov rcx, r15
    cmp DWORD PTR [r13 + INTT_AVX_PARAM_META_BYTES], 0
    jne .Lintt_avx_fused_tail
    call poly_intt32_last1_final_normal_avx2
    jmp .Lintt_avx_done
.Lintt_avx_fused_tail:
    call poly_intt32_last1_final_fused_avx2

.Lintt_avx_done:
    add rsp, 8
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret
.size intt_avx, .-intt_avx

.section .note.GNU-stack,"",@progbits
