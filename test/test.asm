_start:
  push rbp
  mov rbp, rsp
  sub rsp, 16
  mov rax, 150
  mov [rbp - 8], rax
  mov rax, 28
  mov [rbp - 16], rax
  push rdx
  push rbx
  mov rax, [rbp - 8]
  mov rdx, rax
  mov rax, [rbp - 16]
  mov rbx, rax
  xor rax, rax
  cmp rdx, rbx
  jge .lbl_0
  inc rax
.lbl_0:
  pop rbx
  pop rdx
  test rax, rax
  jz .lbl_1
  mov rax, 1
  jmp .lbl_2
.lbl_1:
  mov rax, 2
.lbl_2:
  mov rdi, rax
  mov rax, 60
  syscall
  mov rsp, rbp
  pop rbp
  ret
