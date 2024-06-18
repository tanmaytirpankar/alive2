@a = external global i22

define void @f() {
  %1 = load i22, ptr @a, align 1
  %x = add i22 %1, 1
  store i22 %x, ptr @a, align 1
  ret void
}
