@a = external global i90

define void @f() {
  %1 = load i90, ptr @a, align 1
  %x = add i90 %1, 1
  store i90 %x, ptr @a, align 1
  ret void
}
