@a = external global i32

define void @f() {
  %1 = load i32, ptr @a, align 1
  %x = add i32 %1, 1
  store i32 %x, ptr @a, align 1
  ret void
}
