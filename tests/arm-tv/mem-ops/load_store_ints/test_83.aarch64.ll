@a = external global i83

define void @f() {
  %1 = load i83, ptr @a, align 1
  %x = add i83 %1, 1
  store i83 %x, ptr @a, align 1
  ret void
}
