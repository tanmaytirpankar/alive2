@a = external global i43

define void @f() {
  %1 = load i43, ptr @a, align 1
  %x = add i43 %1, 1
  store i43 %x, ptr @a, align 1
  ret void
}
