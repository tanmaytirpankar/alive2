@a = external global i46

define void @f() {
  %1 = load i46, ptr @a, align 1
  %x = add i46 %1, 1
  store i46 %x, ptr @a, align 1
  ret void
}
