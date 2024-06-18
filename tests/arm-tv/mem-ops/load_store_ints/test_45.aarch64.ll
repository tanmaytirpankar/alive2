@a = external global i45

define void @f() {
  %1 = load i45, ptr @a, align 1
  %x = add i45 %1, 1
  store i45 %x, ptr @a, align 1
  ret void
}
