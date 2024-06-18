@a = external global i29

define void @f() {
  %1 = load i29, ptr @a, align 1
  %x = add i29 %1, 1
  store i29 %x, ptr @a, align 1
  ret void
}
