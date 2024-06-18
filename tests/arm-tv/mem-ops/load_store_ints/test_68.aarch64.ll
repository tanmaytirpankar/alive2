@a = external global i68

define void @f() {
  %1 = load i68, ptr @a, align 1
  %x = add i68 %1, 1
  store i68 %x, ptr @a, align 1
  ret void
}
