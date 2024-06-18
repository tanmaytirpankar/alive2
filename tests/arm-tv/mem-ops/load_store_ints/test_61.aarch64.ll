@a = external global i61

define void @f() {
  %1 = load i61, ptr @a, align 1
  %x = add i61 %1, 1
  store i61 %x, ptr @a, align 1
  ret void
}
