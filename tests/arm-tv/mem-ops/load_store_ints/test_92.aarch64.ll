@a = external global i92

define void @f() {
  %1 = load i92, ptr @a, align 1
  %x = add i92 %1, 1
  store i92 %x, ptr @a, align 1
  ret void
}
