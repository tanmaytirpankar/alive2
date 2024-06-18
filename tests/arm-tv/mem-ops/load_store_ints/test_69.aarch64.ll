@a = external global i69

define void @f() {
  %1 = load i69, ptr @a, align 1
  %x = add i69 %1, 1
  store i69 %x, ptr @a, align 1
  ret void
}
