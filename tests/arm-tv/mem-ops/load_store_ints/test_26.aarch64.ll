@a = external global i26

define void @f() {
  %1 = load i26, ptr @a, align 1
  %x = add i26 %1, 1
  store i26 %x, ptr @a, align 1
  ret void
}
