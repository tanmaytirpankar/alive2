define i32 @f(ptr %p) {
  %lv = load i32, ptr %p, align 4
  ret i32 %lv
}