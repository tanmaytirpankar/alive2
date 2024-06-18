@a = external global i100

define void @f() {
  %1 = load i100, ptr @a, align 1
  %x = add i100 %1, 1
  store i100 %x, ptr @a, align 1
  ret void
}
