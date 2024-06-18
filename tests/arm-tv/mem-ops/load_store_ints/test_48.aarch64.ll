@a = external global i48

define void @f() {
  %1 = load i48, ptr @a, align 1
  %x = add i48 %1, 1
  store i48 %x, ptr @a, align 1
  ret void
}
