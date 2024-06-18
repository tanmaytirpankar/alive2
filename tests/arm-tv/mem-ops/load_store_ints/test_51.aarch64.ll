@a = external global i51

define void @f() {
  %1 = load i51, ptr @a, align 1
  %x = add i51 %1, 1
  store i51 %x, ptr @a, align 1
  ret void
}
