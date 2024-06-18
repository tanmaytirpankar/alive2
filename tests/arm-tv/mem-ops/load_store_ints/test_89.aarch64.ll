@a = external global i89

define void @f() {
  %1 = load i89, ptr @a, align 1
  %x = add i89 %1, 1
  store i89 %x, ptr @a, align 1
  ret void
}
