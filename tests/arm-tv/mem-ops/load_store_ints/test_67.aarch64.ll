@a = external global i67

define void @f() {
  %1 = load i67, ptr @a, align 1
  %x = add i67 %1, 1
  store i67 %x, ptr @a, align 1
  ret void
}
