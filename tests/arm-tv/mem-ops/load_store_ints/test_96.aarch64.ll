@a = external global i96

define void @f() {
  %1 = load i96, ptr @a, align 1
  %x = add i96 %1, 1
  store i96 %x, ptr @a, align 1
  ret void
}
