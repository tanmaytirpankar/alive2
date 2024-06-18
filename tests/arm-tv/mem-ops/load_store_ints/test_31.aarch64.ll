@a = external global i31

define void @f() {
  %1 = load i31, ptr @a, align 1
  %x = add i31 %1, 1
  store i31 %x, ptr @a, align 1
  ret void
}
