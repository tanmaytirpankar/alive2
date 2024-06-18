@a = external global i47

define void @f() {
  %1 = load i47, ptr @a, align 1
  %x = add i47 %1, 1
  store i47 %x, ptr @a, align 1
  ret void
}
