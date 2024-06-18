@a = external global i44

define void @f() {
  %1 = load i44, ptr @a, align 1
  %x = add i44 %1, 1
  store i44 %x, ptr @a, align 1
  ret void
}
