@a = external global i86

define void @f() {
  %1 = load i86, ptr @a, align 1
  %x = add i86 %1, 1
  store i86 %x, ptr @a, align 1
  ret void
}
