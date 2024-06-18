@a = external global i94

define void @f() {
  %1 = load i94, ptr @a, align 1
  %x = add i94 %1, 1
  store i94 %x, ptr @a, align 1
  ret void
}
