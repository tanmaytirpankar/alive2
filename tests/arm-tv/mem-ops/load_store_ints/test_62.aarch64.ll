@a = external global i62

define void @f() {
  %1 = load i62, ptr @a, align 1
  %x = add i62 %1, 1
  store i62 %x, ptr @a, align 1
  ret void
}
