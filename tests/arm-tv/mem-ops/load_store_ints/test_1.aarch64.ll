@a = external global i1

define void @f() {
  %1 = load i1, ptr @a, align 1
  %x = add i1 %1, 1
  store i1 %x, ptr @a, align 1
  ret void
}
