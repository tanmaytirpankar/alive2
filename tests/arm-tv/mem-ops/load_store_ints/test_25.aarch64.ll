@a = external global i25

define void @f() {
  %1 = load i25, ptr @a, align 1
  %x = add i25 %1, 1
  store i25 %x, ptr @a, align 1
  ret void
}
