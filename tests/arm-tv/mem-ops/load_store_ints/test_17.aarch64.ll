@a = external global i17

define void @f() {
  %1 = load i17, ptr @a, align 1
  %x = add i17 %1, 1
  store i17 %x, ptr @a, align 1
  ret void
}
