@a = external global i11

define void @f() {
  %1 = load i11, ptr @a, align 1
  %x = add i11 %1, 1
  store i11 %x, ptr @a, align 1
  ret void
}
