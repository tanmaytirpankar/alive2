define ptr @f(ptr %0, ptr %1) {
  %3 = getelementptr inbounds i16, ptr %0, i64 1
  %4 = load i16, ptr %3, align 4
  %5 = zext i16 %4 to i32
  store i32 %5, ptr %1, align 4
  ret ptr %3
}