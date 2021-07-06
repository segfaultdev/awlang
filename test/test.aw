func _start decl x y begin
  set x 150
  set y 28

  if less x y begin
    1
  end
  else begin
    2
  end

  asm "mov rdi, rax"
  asm "mov rax, 60"
  asm "syscall"
end
