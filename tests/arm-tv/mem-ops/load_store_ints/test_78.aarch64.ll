@a = external global i78

define void @f() {
  %1 = load i78, ptr @a, align 1
  %x = add i78 %1, 1
  store i78 %x, ptr @a, align 1
  ret void
}
