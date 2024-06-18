@a = external global i91

define void @f() {
  %1 = load i91, ptr @a, align 1
  %x = add i91 %1, 1
  store i91 %x, ptr @a, align 1
  ret void
}
