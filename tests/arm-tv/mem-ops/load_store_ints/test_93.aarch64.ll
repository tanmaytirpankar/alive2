@a = external global i93

define void @f() {
  %1 = load i93, ptr @a, align 1
  %x = add i93 %1, 1
  store i93 %x, ptr @a, align 1
  ret void
}
