@a = external global i74

define void @f() {
  %1 = load i74, ptr @a, align 1
  %x = add i74 %1, 1
  store i74 %x, ptr @a, align 1
  ret void
}
