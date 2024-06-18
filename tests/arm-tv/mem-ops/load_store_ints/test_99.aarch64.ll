@a = external global i99

define void @f() {
  %1 = load i99, ptr @a, align 1
  %x = add i99 %1, 1
  store i99 %x, ptr @a, align 1
  ret void
}
