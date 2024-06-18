@a = external global i70

define void @f() {
  %1 = load i70, ptr @a, align 1
  %x = add i70 %1, 1
  store i70 %x, ptr @a, align 1
  ret void
}
